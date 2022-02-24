/* CDK - The GIMP Drawing Kit
 *
 * cdkglcontext-x11.c: X11 specific OpenGL wrappers
 *
 * Copyright © 2014  Emmanuele Bassi
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "cdkglcontext-x11.h"
#include "cdkdisplay-x11.h"
#include "cdkscreen-x11.h"

#include "cdkx11display.h"
#include "cdkx11glcontext.h"
#include "cdkx11screen.h"
#include "cdkx11window.h"
#include "cdkx11visual.h"
#include "cdkvisualprivate.h"
#include "cdkx11property.h"
#include <X11/Xatom.h>

#include "cdkinternals.h"

#include "cdkintl.h"

#include <cairo/cairo-xlib.h>

#include <epoxy/glx.h>

G_DEFINE_TYPE (CdkX11GLContext, cdk_x11_gl_context, CDK_TYPE_GL_CONTEXT)

typedef struct {
  CdkDisplay *display;

  GLXDrawable glx_drawable;

  Window dummy_xwin;
  GLXWindow dummy_glx;

  guint32 last_frame_counter;
} DrawableInfo;

static void
drawable_info_free (gpointer data_)
{
  DrawableInfo *data = data_;
  Display *dpy;

  cdk_x11_display_error_trap_push (data->display);

  dpy = cdk_x11_display_get_xdisplay (data->display);

  if (data->glx_drawable)
    glXDestroyWindow (dpy, data->glx_drawable);

  if (data->dummy_glx)
    glXDestroyWindow (dpy, data->dummy_glx);

  if (data->dummy_xwin)
    XDestroyWindow (dpy, data->dummy_xwin);

  cdk_x11_display_error_trap_pop_ignored (data->display);

  g_slice_free (DrawableInfo, data);
}

static DrawableInfo *
get_glx_drawable_info (CdkWindow *window)
{
  return g_object_get_data (G_OBJECT (window), "-cdk-x11-window-glx-info");
}

static void
set_glx_drawable_info (CdkWindow    *window,
                       DrawableInfo *info)
{
  g_object_set_data_full (G_OBJECT (window), "-cdk-x11-window-glx-info",
                          info,
                          drawable_info_free);
}

static void
maybe_wait_for_vblank (CdkDisplay  *display,
                       GLXDrawable  drawable)
{
  CdkX11Display *display_x11 = CDK_X11_DISPLAY (display);
  Display *dpy = cdk_x11_display_get_xdisplay (display);

  if (display_x11->has_glx_sync_control)
    {
      gint64 ust, msc, sbc;

      glXGetSyncValuesOML (dpy, drawable, &ust, &msc, &sbc);
      glXWaitForMscOML (dpy, drawable,
                        0, 2, (msc + 1) % 2,
                        &ust, &msc, &sbc);
    }
  else if (display_x11->has_glx_video_sync)
    {
      guint32 current_count;

      glXGetVideoSyncSGI (&current_count);
      glXWaitVideoSyncSGI (2, (current_count + 1) % 2, &current_count);
    }
}

void
cdk_x11_window_invalidate_for_new_frame (CdkWindow      *window,
                                         cairo_region_t *update_area)
{
  cairo_rectangle_int_t window_rect;
  CdkDisplay *display = cdk_window_get_display (window);
  CdkX11Display *display_x11 = CDK_X11_DISPLAY (display);
  Display *dpy = cdk_x11_display_get_xdisplay (display);
  CdkX11GLContext *context_x11;
  unsigned int buffer_age;
  gboolean invalidate_all;

  /* Minimal update is ok if we're not drawing with gl */
  if (window->gl_paint_context == NULL)
    return;

  context_x11 = CDK_X11_GL_CONTEXT (window->gl_paint_context);

  buffer_age = 0;

  context_x11->do_blit_swap = FALSE;

  if (display_x11->has_glx_buffer_age)
    {
      cdk_gl_context_make_current (window->gl_paint_context);
      glXQueryDrawable(dpy, context_x11->drawable,
		       GLX_BACK_BUFFER_AGE_EXT, &buffer_age);
    }


  invalidate_all = FALSE;
  if (buffer_age >= 4)
    {
      cairo_rectangle_int_t whole_window = { 0, 0, cdk_window_get_width (window), cdk_window_get_height (window) };

      if (cdk_gl_context_has_framebuffer_blit (window->gl_paint_context) &&
          cairo_region_contains_rectangle (update_area, &whole_window) != CAIRO_REGION_OVERLAP_IN)
        {
          context_x11->do_blit_swap = TRUE;
        }
      else
        invalidate_all = TRUE;
    }
  else
    {
      if (buffer_age == 0)
        {
          invalidate_all = TRUE;
        }
      if (buffer_age >= 2)
        {
          if (window->old_updated_area[0])
            cairo_region_union (update_area, window->old_updated_area[0]);
          else
            invalidate_all = TRUE;
        }
      if (buffer_age >= 3)
        {
          if (window->old_updated_area[1])
            cairo_region_union (update_area, window->old_updated_area[1]);
          else
            invalidate_all = TRUE;
        }
    }

  if (invalidate_all)
    {
      window_rect.x = 0;
      window_rect.y = 0;
      window_rect.width = cdk_window_get_width (window);
      window_rect.height = cdk_window_get_height (window);

      /* If nothing else is known, repaint everything so that the back
         buffer is fully up-to-date for the swapbuffer */
      cairo_region_union_rectangle (update_area, &window_rect);
    }

}

static void
cdk_gl_blit_region (CdkWindow *window, cairo_region_t *region)
{
  int n_rects, i;
  int scale = cdk_window_get_scale_factor (window);
  int wh = cdk_window_get_height (window);
  cairo_rectangle_int_t rect;

  n_rects = cairo_region_num_rectangles (region);
  for (i = 0; i < n_rects; i++)
    {
      cairo_region_get_rectangle (region, i, &rect);
      glScissor (rect.x * scale, (wh - rect.y - rect.height) * scale, rect.width * scale, rect.height * scale);
      glBlitFramebuffer (rect.x * scale, (wh - rect.y - rect.height) * scale, (rect.x + rect.width) * scale, (wh - rect.y) * scale,
                         rect.x * scale, (wh - rect.y - rect.height) * scale, (rect.x + rect.width) * scale, (wh - rect.y) * scale,
                         GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
}

static void
cdk_x11_gl_context_end_frame (CdkGLContext *context,
                              cairo_region_t *painted,
                              cairo_region_t *damage)
{
  CdkX11GLContext *context_x11 = CDK_X11_GL_CONTEXT (context);
  CdkWindow *window = cdk_gl_context_get_window (context);
  CdkDisplay *display = cdk_gl_context_get_display (context);
  Display *dpy = cdk_x11_display_get_xdisplay (display);
  CdkX11Display *display_x11 = CDK_X11_DISPLAY (display);
  DrawableInfo *info;
  GLXDrawable drawable;

  cdk_gl_context_make_current (context);

  info = get_glx_drawable_info (window);

  drawable = context_x11->drawable;

  CDK_NOTE (OPENGL,
            g_message ("Flushing GLX buffers for drawable %lu (window: %lu), frame sync: %s",
                       (unsigned long) drawable,
                       (unsigned long) cdk_x11_window_get_xid (window),
                       context_x11->do_frame_sync ? "yes" : "no"));

  /* if we are going to wait for the vertical refresh manually
   * we need to flush pending redraws, and we also need to wait
   * for that to finish, otherwise we are going to tear.
   *
   * obviously, this condition should not be hit if we have
   * GLX_SGI_swap_control, and we ask the driver to do the right
   * thing.
   */
  if (context_x11->do_frame_sync)
    {
      guint32 end_frame_counter = 0;
      gboolean has_counter = display_x11->has_glx_video_sync;
      gboolean can_wait = display_x11->has_glx_video_sync || display_x11->has_glx_sync_control;

      if (display_x11->has_glx_video_sync)
        glXGetVideoSyncSGI (&end_frame_counter);

      if (context_x11->do_frame_sync && !display_x11->has_glx_swap_interval)
        {
          glFinish ();

          if (has_counter && can_wait)
            {
              guint32 last_counter = info != NULL ? info->last_frame_counter : 0;

              if (last_counter == end_frame_counter)
                maybe_wait_for_vblank (display, drawable);
            }
          else if (can_wait)
            maybe_wait_for_vblank (display, drawable);
        }
    }

  if (context_x11->do_blit_swap)
    {
      glDrawBuffer(GL_FRONT);
      glReadBuffer(GL_BACK);
      cdk_gl_blit_region (window, painted);
      glDrawBuffer(GL_BACK);
      glFlush();

      if (cdk_gl_context_has_frame_terminator (context))
        glFrameTerminatorGREMEDY ();
    }
  else
    glXSwapBuffers (dpy, drawable);

  if (context_x11->do_frame_sync && info != NULL && display_x11->has_glx_video_sync)
    glXGetVideoSyncSGI (&info->last_frame_counter);
}

typedef struct {
  Display *display;
  GLXDrawable drawable;
  gboolean y_inverted;
} CdkGLXPixmap;

static void
glx_pixmap_destroy (void *data)
{
  CdkGLXPixmap *glx_pixmap = data;

  glXDestroyPixmap (glx_pixmap->display, glx_pixmap->drawable);

  g_slice_free (CdkGLXPixmap, glx_pixmap);
}

static CdkGLXPixmap *
glx_pixmap_get (cairo_surface_t *surface, guint texture_target)
{
  Display *display = cairo_xlib_surface_get_display (surface);
  Screen *screen = cairo_xlib_surface_get_screen (surface);
  Visual *visual = cairo_xlib_surface_get_visual (surface);
  CdkGLXPixmap *glx_pixmap;
  GLXFBConfig *fbconfigs, config;
  int nfbconfigs;
  XVisualInfo *visinfo;
  VisualID visualid;
  int i, value;
  gboolean y_inverted;
  gboolean with_alpha;
  guint target = 0;
  guint format = 0;
  int pixmap_attributes[] = {
    GLX_TEXTURE_TARGET_EXT, 0,
    GLX_TEXTURE_FORMAT_EXT, 0,
    None
  };

  if (visual == NULL)
    return NULL;

  with_alpha = cairo_surface_get_content (surface) == CAIRO_CONTENT_COLOR_ALPHA;

  y_inverted = FALSE;
  fbconfigs = glXGetFBConfigs (display, XScreenNumberOfScreen (screen), &nfbconfigs);
  for (i = 0; i < nfbconfigs; i++)
    {
      visinfo = glXGetVisualFromFBConfig (display, fbconfigs[i]);
      if (!visinfo)
        continue;

      visualid = visinfo->visualid;
      XFree (visinfo);

      if (visualid != XVisualIDFromVisual (visual))
        continue;

      glXGetFBConfigAttrib (display, fbconfigs[i], GLX_DRAWABLE_TYPE, &value);
      if (!(value & GLX_PIXMAP_BIT))
        continue;

      glXGetFBConfigAttrib (display, fbconfigs[i],
                            GLX_BIND_TO_TEXTURE_TARGETS_EXT,
                            &value);
      if (texture_target == GL_TEXTURE_2D)
        {
          if (value & GLX_TEXTURE_2D_BIT_EXT)
            target = GLX_TEXTURE_2D_EXT;
          else
            continue;
        }
      else if (texture_target == GL_TEXTURE_RECTANGLE_ARB)
        {
          if (value & GLX_TEXTURE_RECTANGLE_BIT_EXT)
            target = GLX_TEXTURE_RECTANGLE_EXT;
          else
            continue;
        }
      else
        continue;

      if (!with_alpha)
        {
          glXGetFBConfigAttrib (display, fbconfigs[i],
                                GLX_BIND_TO_TEXTURE_RGB_EXT,
                                &value);
          if (!value)
            continue;

          format = GLX_TEXTURE_FORMAT_RGB_EXT;
        }
      else
        {
          glXGetFBConfigAttrib (display, fbconfigs[i],
                                GLX_BIND_TO_TEXTURE_RGBA_EXT,
                                &value);
          if (!value)
            continue;

          format = GLX_TEXTURE_FORMAT_RGBA_EXT;
        }

      glXGetFBConfigAttrib (display, fbconfigs[i],
                            GLX_Y_INVERTED_EXT,
                            &value);
      if (value == TRUE)
        y_inverted = TRUE;

      config = fbconfigs[i];
      break;
    }

  XFree (fbconfigs);

  if (i == nfbconfigs)
    return NULL;

  pixmap_attributes[1] = target;
  pixmap_attributes[3] = format;

  glx_pixmap = g_slice_new0 (CdkGLXPixmap);
  glx_pixmap->y_inverted = y_inverted;
  glx_pixmap->display = display;
  glx_pixmap->drawable = glXCreatePixmap (display, config,
					  cairo_xlib_surface_get_drawable (surface),
					  pixmap_attributes);

  return glx_pixmap;
}

static gboolean
cdk_x11_gl_context_texture_from_surface (CdkGLContext *paint_context,
					 cairo_surface_t *surface,
					 cairo_region_t *region)
{
  CdkGLXPixmap *glx_pixmap;
  double device_x_offset, device_y_offset;
  cairo_rectangle_int_t rect;
  int n_rects, i;
  CdkWindow *window;
  int unscaled_window_height;
  int window_scale;
  unsigned int texture_id;
  gboolean use_texture_rectangle;
  guint target;
  double sx, sy;
  float uscale, vscale;
  CdkTexturedQuad *quads;
  CdkX11Display *display_x11;

  display_x11 = CDK_X11_DISPLAY (cdk_gl_context_get_display (paint_context));
  if (!display_x11->has_glx_texture_from_pixmap)
    return FALSE;

  if (cairo_surface_get_type (surface) != CAIRO_SURFACE_TYPE_XLIB)
    return FALSE;

  use_texture_rectangle = cdk_gl_context_use_texture_rectangle (paint_context);
  if (use_texture_rectangle)
    target = GL_TEXTURE_RECTANGLE_ARB;
  else
    target = GL_TEXTURE_2D;

  glx_pixmap = glx_pixmap_get (surface, target);
  if (glx_pixmap == NULL)
    return FALSE;

  CDK_NOTE (OPENGL, g_message ("Using GLX_EXT_texture_from_pixmap to draw surface"));

  window = cdk_gl_context_get_window (paint_context)->impl_window;
  window_scale = cdk_window_get_scale_factor (window);
  cdk_window_get_unscaled_size (window, NULL, &unscaled_window_height);

  sx = sy = 1;
  cairo_surface_get_device_scale (window->current_paint.surface, &sx, &sy);
  cairo_surface_get_device_offset (surface, &device_x_offset, &device_y_offset);

  /* Ensure all the X stuff are synced before we read it back via texture-from-pixmap */
  glXWaitX();

  glGenTextures (1, &texture_id);
  glBindTexture (target, texture_id);

  glTexParameteri (target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri (target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri (target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri (target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glXBindTexImageEXT (glx_pixmap->display, glx_pixmap->drawable,
		      GLX_FRONT_LEFT_EXT, NULL);

  glEnable (GL_SCISSOR_TEST);

  n_rects = cairo_region_num_rectangles (region);
  quads = g_new (CdkTexturedQuad, n_rects);

#define FLIP_Y(_y) (unscaled_window_height - (_y))

  cairo_region_get_extents (region, &rect);
  glScissor (rect.x * window_scale, FLIP_Y((rect.y + rect.height) * window_scale),
             rect.width * window_scale, rect.height * window_scale);

  for (i = 0; i < n_rects; i++)
    {
      int src_x, src_y, src_height, src_width;

      cairo_region_get_rectangle (region, i, &rect);

      src_x = rect.x * sx + device_x_offset;
      src_y = rect.y * sy + device_y_offset;
      src_width = rect.width * sx;
      src_height = rect.height * sy;

      if (use_texture_rectangle)
        {
          uscale = 1.0;
          vscale = 1.0;
        }
      else
        {
          uscale = 1.0 / cairo_xlib_surface_get_width (surface);
          vscale = 1.0 / cairo_xlib_surface_get_height (surface);
        }

      {
        CdkTexturedQuad quad = {
          rect.x * window_scale, FLIP_Y(rect.y * window_scale),
          (rect.x + rect.width) * window_scale, FLIP_Y((rect.y + rect.height) * window_scale),
          uscale * src_x, vscale * src_y,
          uscale * (src_x + src_width), vscale * (src_y + src_height),
        };

        quads[i] = quad;
      }
    }

#undef FLIP_Y

  cdk_gl_texture_quads (paint_context, target, n_rects, quads, FALSE);
  g_free (quads);

  glDisable (GL_SCISSOR_TEST);

  glXReleaseTexImageEXT (glx_pixmap->display, glx_pixmap->drawable,
  			 GLX_FRONT_LEFT_EXT);

  glDeleteTextures (1, &texture_id);

  glx_pixmap_destroy(glx_pixmap);

  return TRUE;
}

static XVisualInfo *
find_xvisinfo_for_fbconfig (CdkDisplay  *display,
                            GLXFBConfig  config)
{
  Display *dpy = cdk_x11_display_get_xdisplay (display);

  return glXGetVisualFromFBConfig (dpy, config);
}

static GLXContext
create_gl3_context (CdkDisplay   *display,
                    GLXFBConfig   config,
                    CdkGLContext *share,
                    int           profile,
                    int           flags,
                    int           major,
                    int           minor)
{
  int attrib_list[] = {
    GLX_CONTEXT_PROFILE_MASK_ARB, profile,
    GLX_CONTEXT_MAJOR_VERSION_ARB, major,
    GLX_CONTEXT_MINOR_VERSION_ARB, minor,
    GLX_CONTEXT_FLAGS_ARB, flags,
    None,
  };
  GLXContext res;

  CdkX11GLContext *share_x11 = NULL;

  if (share != NULL)
    share_x11 = CDK_X11_GL_CONTEXT (share);

  cdk_x11_display_error_trap_push (display);

  res = glXCreateContextAttribsARB (cdk_x11_display_get_xdisplay (display),
                                    config,
                                    share_x11 != NULL ? share_x11->glx_context : NULL,
                                    True,
                                    attrib_list);

  if (cdk_x11_display_error_trap_pop (display))
    return NULL;

  return res;
}

static GLXContext
create_legacy_context (CdkDisplay   *display,
                       GLXFBConfig   config,
                       CdkGLContext *share)
{
  CdkX11GLContext *share_x11 = NULL;
  GLXContext res;

  if (share != NULL)
    share_x11 = CDK_X11_GL_CONTEXT (share);

  cdk_x11_display_error_trap_push (display);

  res = glXCreateNewContext (cdk_x11_display_get_xdisplay (display),
                             config,
                             GLX_RGBA_TYPE,
                             share_x11 != NULL ? share_x11->glx_context : NULL,
                             TRUE);

  if (cdk_x11_display_error_trap_pop (display))
    return NULL;

  return res;
}

static gboolean
cdk_x11_gl_context_realize (CdkGLContext  *context,
                            GError       **error)
{
  CdkX11Display *display_x11;
  CdkDisplay *display;
  CdkX11GLContext *context_x11;
  GLXWindow drawable;
  XVisualInfo *xvisinfo;
  Display *dpy;
  DrawableInfo *info;
  CdkGLContext *share;
  CdkWindow *window;
  gboolean debug_bit, compat_bit, legacy_bit, es_bit;
  int major, minor, flags;

  window = cdk_gl_context_get_window (context);
  display = cdk_window_get_display (window);
  dpy = cdk_x11_display_get_xdisplay (display);
  context_x11 = CDK_X11_GL_CONTEXT (context);
  display_x11 = CDK_X11_DISPLAY (display);
  share = cdk_gl_context_get_shared_context (context);

  cdk_gl_context_get_required_version (context, &major, &minor);
  debug_bit = cdk_gl_context_get_debug_enabled (context);
  compat_bit = cdk_gl_context_get_forward_compatible (context);

  /* If there is no glXCreateContextAttribsARB() then we default to legacy */
  legacy_bit = !display_x11->has_glx_create_context ||
               (_cdk_gl_flags & CDK_GL_LEGACY) != 0;

  es_bit = ((_cdk_gl_flags & CDK_GL_GLES) != 0 ||
            (share != NULL && cdk_gl_context_get_use_es (share))) &&
           (display_x11->has_glx_create_context && display_x11->has_glx_create_es2_context);

  /* We cannot share legacy contexts with core profile ones, so the
   * shared context is the one that decides if we're going to create
   * a legacy context or not.
   */
  if (share != NULL && cdk_gl_context_is_legacy (share))
    legacy_bit = TRUE;

  flags = 0;
  if (debug_bit)
    flags |= GLX_CONTEXT_DEBUG_BIT_ARB;
  if (compat_bit)
    flags |= GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;

  CDK_NOTE (OPENGL,
            g_message ("Creating GLX context (version:%d.%d, debug:%s, forward:%s, legacy:%s, es:%s)",
                       major, minor,
                       debug_bit ? "yes" : "no",
                       compat_bit ? "yes" : "no",
                       legacy_bit ? "yes" : "no",
                       es_bit ? "yes" : "no"));

  /* If we have access to GLX_ARB_create_context_profile then we can ask for
   * a compatibility profile; if we don't, then we have to fall back to the
   * old GLX 1.3 API.
   */
  if (legacy_bit && !CDK_X11_DISPLAY (display)->has_glx_create_context)
    {
      CDK_NOTE (OPENGL, g_message ("Creating legacy GL context on request"));
      context_x11->glx_context = create_legacy_context (display, context_x11->glx_config, share);
    }
  else
    {
      int profile;

      if (es_bit)
        profile = GLX_CONTEXT_ES2_PROFILE_BIT_EXT;
      else
        profile = legacy_bit ? GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB
                             : GLX_CONTEXT_CORE_PROFILE_BIT_ARB;

      /* We need to tweak the version, otherwise we may end up requesting
       * a compatibility context with a minimum version of 3.2, which is
       * an error
       */
      if (legacy_bit)
        {
          major = 3;
          minor = 0;
        }

      CDK_NOTE (OPENGL, g_message ("Creating GL3 context"));
      context_x11->glx_context = create_gl3_context (display,
                                                     context_x11->glx_config,
                                                     share,
                                                     profile, flags, major, minor);

      /* Fall back to legacy in case the GL3 context creation failed */
      if (context_x11->glx_context == NULL)
        {
          CDK_NOTE (OPENGL, g_message ("Creating fallback legacy context"));
          context_x11->glx_context = create_legacy_context (display, context_x11->glx_config, share);
          legacy_bit = TRUE;
          es_bit = FALSE;
        }
    }

  if (context_x11->glx_context == NULL)
    {
      g_set_error_literal (error, CDK_GL_ERROR,
                           CDK_GL_ERROR_NOT_AVAILABLE,
                           _("Unable to create a GL context"));
      return FALSE;
    }

  /* Ensure that any other context is created with a legacy bit set */
  cdk_gl_context_set_is_legacy (context, legacy_bit);

  /* Ensure that any other context is created with a ES bit set */
  cdk_gl_context_set_use_es (context, es_bit);

  xvisinfo = find_xvisinfo_for_fbconfig (display, context_x11->glx_config);

  info = get_glx_drawable_info (window->impl_window);
  if (info == NULL)
    {
      XSetWindowAttributes attrs;
      unsigned long mask;

      cdk_x11_display_error_trap_push (display);

      info = g_slice_new0 (DrawableInfo);
      info->display = display;
      info->last_frame_counter = 0;

      attrs.override_redirect = True;
      attrs.colormap = XCreateColormap (dpy, DefaultRootWindow (dpy), xvisinfo->visual, AllocNone);
      attrs.border_pixel = 0;
      mask = CWOverrideRedirect | CWColormap | CWBorderPixel;
      info->dummy_xwin = XCreateWindow (dpy, DefaultRootWindow (dpy),
                                        -100, -100, 1, 1,
                                        0,
                                        xvisinfo->depth,
                                        CopyFromParent,
                                        xvisinfo->visual,
                                        mask,
                                        &attrs);
      XMapWindow(dpy, info->dummy_xwin);

      if (CDK_X11_DISPLAY (display)->glx_version >= 13)
        {
          info->glx_drawable = glXCreateWindow (dpy, context_x11->glx_config,
                                                cdk_x11_window_get_xid (window->impl_window),
                                                NULL);
          info->dummy_glx = glXCreateWindow (dpy, context_x11->glx_config, info->dummy_xwin, NULL);
        }

      if (cdk_x11_display_error_trap_pop (display))
        {
          g_set_error_literal (error, CDK_GL_ERROR,
                               CDK_GL_ERROR_NOT_AVAILABLE,
                               _("Unable to create a GL context"));

          XFree (xvisinfo);
          drawable_info_free (info);
          glXDestroyContext (dpy, context_x11->glx_context);
          context_x11->glx_context = NULL;

          return FALSE;
        }

      set_glx_drawable_info (window->impl_window, info);
    }

  XFree (xvisinfo);

  if (context_x11->is_attached)
    drawable = info->glx_drawable ? info->glx_drawable : cdk_x11_window_get_xid (window->impl_window);
  else
    drawable = info->dummy_glx ? info->dummy_glx : info->dummy_xwin;

  context_x11->is_direct = glXIsDirect (dpy, context_x11->glx_context);
  context_x11->drawable = drawable;

  CDK_NOTE (OPENGL,
            g_message ("Realized GLX context[%p], %s",
                       context_x11->glx_context,
                       context_x11->is_direct ? "direct" : "indirect"));

  return TRUE;
}

static void
cdk_x11_gl_context_dispose (GObject *gobject)
{
  CdkX11GLContext *context_x11 = CDK_X11_GL_CONTEXT (gobject);

  if (context_x11->glx_context != NULL)
    {
      CdkGLContext *context = CDK_GL_CONTEXT (gobject);
      CdkDisplay *display = cdk_gl_context_get_display (context);
      Display *dpy = cdk_x11_display_get_xdisplay (display);

      if (glXGetCurrentContext () == context_x11->glx_context)
        glXMakeContextCurrent (dpy, None, None, NULL);

      CDK_NOTE (OPENGL, g_message ("Destroying GLX context"));
      glXDestroyContext (dpy, context_x11->glx_context);
      context_x11->glx_context = NULL;
    }

  G_OBJECT_CLASS (cdk_x11_gl_context_parent_class)->dispose (gobject);
}

static void
cdk_x11_gl_context_class_init (CdkX11GLContextClass *klass)
{
  CdkGLContextClass *context_class = CDK_GL_CONTEXT_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  context_class->realize = cdk_x11_gl_context_realize;
  context_class->end_frame = cdk_x11_gl_context_end_frame;
  context_class->texture_from_surface = cdk_x11_gl_context_texture_from_surface;

  gobject_class->dispose = cdk_x11_gl_context_dispose;
}

static void
cdk_x11_gl_context_init (CdkX11GLContext *self)
{
  self->do_frame_sync = TRUE;
}

gboolean
cdk_x11_screen_init_gl (CdkScreen *screen)
{
  CdkDisplay *display = cdk_screen_get_display (screen);
  CdkX11Display *display_x11 = CDK_X11_DISPLAY (display);
  Display *dpy;
  int error_base, event_base;
  int screen_num;

  if (display_x11->have_glx)
    return TRUE;

  if (_cdk_gl_flags & CDK_GL_DISABLE)
    return FALSE;

  dpy = cdk_x11_display_get_xdisplay (display);

  if (!epoxy_has_glx (dpy))
    return FALSE;

  if (!glXQueryExtension (dpy, &error_base, &event_base))
    return FALSE;

  screen_num = CDK_X11_SCREEN (screen)->screen_num;

  display_x11->have_glx = TRUE;

  display_x11->glx_version = epoxy_glx_version (dpy, screen_num);
  display_x11->glx_error_base = error_base;
  display_x11->glx_event_base = event_base;

  display_x11->has_glx_create_context =
    epoxy_has_glx_extension (dpy, screen_num, "GLX_ARB_create_context_profile");
  display_x11->has_glx_create_es2_context =
    epoxy_has_glx_extension (dpy, screen_num, "GLX_EXT_create_context_es2_profile");
  display_x11->has_glx_swap_interval =
    epoxy_has_glx_extension (dpy, screen_num, "GLX_SGI_swap_control");
  display_x11->has_glx_texture_from_pixmap =
    epoxy_has_glx_extension (dpy, screen_num, "GLX_EXT_texture_from_pixmap");
  display_x11->has_glx_video_sync =
    epoxy_has_glx_extension (dpy, screen_num, "GLX_SGI_video_sync");
  display_x11->has_glx_buffer_age =
    epoxy_has_glx_extension (dpy, screen_num, "GLX_EXT_buffer_age");
  display_x11->has_glx_sync_control =
    epoxy_has_glx_extension (dpy, screen_num, "GLX_OML_sync_control");
  display_x11->has_glx_multisample =
    epoxy_has_glx_extension (dpy, screen_num, "GLX_ARB_multisample");
  display_x11->has_glx_visual_rating =
    epoxy_has_glx_extension (dpy, screen_num, "GLX_EXT_visual_rating");

  CDK_NOTE (OPENGL,
            g_message ("GLX version %d.%d found\n"
                       " - Vendor: %s\n"
                       " - Checked extensions:\n"
                       "\t* GLX_ARB_create_context_profile: %s\n"
                       "\t* GLX_EXT_create_context_es2_profile: %s\n"
                       "\t* GLX_SGI_swap_control: %s\n"
                       "\t* GLX_EXT_texture_from_pixmap: %s\n"
                       "\t* GLX_SGI_video_sync: %s\n"
                       "\t* GLX_EXT_buffer_age: %s\n"
                       "\t* GLX_OML_sync_control: %s",
                     display_x11->glx_version / 10,
                     display_x11->glx_version % 10,
                     glXGetClientString (dpy, GLX_VENDOR),
                     display_x11->has_glx_create_context ? "yes" : "no",
                     display_x11->has_glx_create_es2_context ? "yes" : "no",
                     display_x11->has_glx_swap_interval ? "yes" : "no",
                     display_x11->has_glx_texture_from_pixmap ? "yes" : "no",
                     display_x11->has_glx_video_sync ? "yes" : "no",
                     display_x11->has_glx_buffer_age ? "yes" : "no",
                     display_x11->has_glx_sync_control ? "yes" : "no"));

  return TRUE;
}

#define MAX_GLX_ATTRS   30

static gboolean
find_fbconfig_for_visual (CdkDisplay   *display,
			  CdkVisual    *visual,
                          GLXFBConfig  *fb_config_out,
                          GError      **error)
{
  static int attrs[MAX_GLX_ATTRS];
  Display *dpy = cdk_x11_display_get_xdisplay (display);
  GLXFBConfig *configs;
  int n_configs, i;
  gboolean use_rgba;
  gboolean retval = FALSE;
  VisualID xvisual_id = XVisualIDFromVisual(cdk_x11_visual_get_xvisual (visual));

  i = 0;
  attrs[i++] = GLX_DRAWABLE_TYPE;
  attrs[i++] = GLX_WINDOW_BIT;

  attrs[i++] = GLX_RENDER_TYPE;
  attrs[i++] = GLX_RGBA_BIT;

  attrs[i++] = GLX_DOUBLEBUFFER;
  attrs[i++] = GL_TRUE;

  attrs[i++] = GLX_RED_SIZE;
  attrs[i++] = 1;
  attrs[i++] = GLX_GREEN_SIZE;
  attrs[i++] = 1;
  attrs[i++] = GLX_BLUE_SIZE;
  attrs[i++] = 1;

  use_rgba = (visual == cdk_screen_get_rgba_visual (cdk_display_get_default_screen (display)));
  if (use_rgba)
    {
      attrs[i++] = GLX_ALPHA_SIZE;
      attrs[i++] = 1;
    }
  else
    {
      attrs[i++] = GLX_ALPHA_SIZE;
      attrs[i++] = GLX_DONT_CARE;
    }

  attrs[i++] = None;

  g_assert (i < MAX_GLX_ATTRS);

  configs = glXChooseFBConfig (dpy, DefaultScreen (dpy), attrs, &n_configs);
  if (configs == NULL || n_configs == 0)
    {
      g_set_error_literal (error, CDK_GL_ERROR,
                           CDK_GL_ERROR_UNSUPPORTED_FORMAT,
                           _("No available configurations for the given pixel format"));
      return FALSE;
    }

  for (i = 0; i < n_configs; i++)
    {
      XVisualInfo *visinfo;

      visinfo = glXGetVisualFromFBConfig (dpy, configs[i]);
      if (visinfo == NULL)
        continue;

      if (visinfo->visualid != xvisual_id)
        {
          XFree (visinfo);
          continue;
        }

      if (fb_config_out != NULL)
        *fb_config_out = configs[i];

      XFree (visinfo);
      retval = TRUE;
      goto out;
    }

  g_set_error (error, CDK_GL_ERROR,
               CDK_GL_ERROR_UNSUPPORTED_FORMAT,
               _("No available configurations for the given RGBA pixel format"));

out:
  XFree (configs);

  return retval;
}

struct glvisualinfo {
  int supports_gl;
  int double_buffer;
  int stereo;
  int alpha_size;
  int depth_size;
  int stencil_size;
  int num_multisample;
  int visual_caveat;
};

static gboolean
visual_compatible (const CdkVisual *a, const CdkVisual *b)
{
  return a->type == b->type &&
    a->depth == b->depth &&
    a->red_mask == b->red_mask &&
    a->green_mask == b->green_mask &&
    a->blue_mask == b->blue_mask &&
    a->colormap_size == b->colormap_size &&
    a->bits_per_rgb == b->bits_per_rgb;
}

static gboolean
visual_is_rgba (const CdkVisual *visual)
{
  return
    visual->depth == 32 &&
    visual->red_mask   == 0xff0000 &&
    visual->green_mask == 0x00ff00 &&
    visual->blue_mask  == 0x0000ff;
}

/* This picks a compatible (as in has the same X visual details) visual
   that has "better" characteristics on the GL side */
static CdkVisual *
pick_better_visual_for_gl (CdkX11Screen *x11_screen,
                           struct glvisualinfo *gl_info,
                           CdkVisual *compatible)
{
  CdkVisual *visual;
  int i;
  gboolean want_alpha = visual_is_rgba (compatible);

  /* First look for "perfect match", i.e:
   * supports gl
   * double buffer
   * alpha iff visual is an rgba visual
   * no unnecessary stuff
   */
  for (i = 0; i < x11_screen->nvisuals; i++)
    {
      visual = x11_screen->visuals[i];
      if (visual_compatible (visual, compatible) &&
          gl_info[i].supports_gl &&
          gl_info[i].double_buffer &&
          !gl_info[i].stereo &&
          (want_alpha ? (gl_info[i].alpha_size > 0) : (gl_info[i].alpha_size == 0)) &&
          (gl_info[i].depth_size == 0) &&
          (gl_info[i].stencil_size == 0) &&
          (gl_info[i].num_multisample == 0) &&
          (gl_info[i].visual_caveat == GLX_NONE_EXT))
        return visual;
    }

  if (!want_alpha)
    {
      /* Next, allow alpha even if we don't want it: */
      for (i = 0; i < x11_screen->nvisuals; i++)
        {
          visual = x11_screen->visuals[i];
          if (visual_compatible (visual, compatible) &&
              gl_info[i].supports_gl &&
              gl_info[i].double_buffer &&
              !gl_info[i].stereo &&
              (gl_info[i].depth_size == 0) &&
              (gl_info[i].stencil_size == 0) &&
              (gl_info[i].num_multisample == 0) &&
              (gl_info[i].visual_caveat == GLX_NONE_EXT))
            return visual;
        }
    }

  /* Next, allow depth and stencil buffers: */
  for (i = 0; i < x11_screen->nvisuals; i++)
    {
      visual = x11_screen->visuals[i];
      if (visual_compatible (visual, compatible) &&
          gl_info[i].supports_gl &&
          gl_info[i].double_buffer &&
          !gl_info[i].stereo &&
          (gl_info[i].num_multisample == 0) &&
          (gl_info[i].visual_caveat == GLX_NONE_EXT))
        return visual;
    }

  /* Next, allow multisample: */
  for (i = 0; i < x11_screen->nvisuals; i++)
    {
      visual = x11_screen->visuals[i];
      if (visual_compatible (visual, compatible) &&
          gl_info[i].supports_gl &&
          gl_info[i].double_buffer &&
          !gl_info[i].stereo &&
          (gl_info[i].visual_caveat == GLX_NONE_EXT))
        return visual;
    }

  return compatible;
}

static gboolean
get_cached_gl_visuals (CdkDisplay *display, int *system, int *rgba)
{
  gboolean found;
  Atom type_return;
  gint format_return;
  gulong nitems_return;
  gulong bytes_after_return;
  guchar *data = NULL;
  Display *dpy;

  dpy = cdk_x11_display_get_xdisplay (display);

  found = FALSE;

  cdk_x11_display_error_trap_push (display);
  if (XGetWindowProperty (dpy, DefaultRootWindow (dpy),
                          cdk_x11_get_xatom_by_name_for_display (display, "CDK_VISUALS"),
                          0, 2, False, XA_INTEGER, &type_return,
                          &format_return, &nitems_return,
                          &bytes_after_return, &data) == Success)
    {
      if (type_return == XA_INTEGER &&
          format_return == 32 &&
          nitems_return == 2 &&
          data != NULL)
        {
          long *visuals = (long *) data;

          *system = (int)visuals[0];
          *rgba = (int)visuals[1];
          found = TRUE;
        }
    }
  cdk_x11_display_error_trap_pop_ignored (display);

  if (data)
    XFree (data);

  return found;
}

static void
save_cached_gl_visuals (CdkDisplay *display, int system, int rgba)
{
  long visualdata[2];
  Display *dpy;

  dpy = cdk_x11_display_get_xdisplay (display);

  visualdata[0] = system;
  visualdata[1] = rgba;

  cdk_x11_display_error_trap_push (display);
  XChangeProperty (dpy, DefaultRootWindow (dpy),
                   cdk_x11_get_xatom_by_name_for_display (display, "CDK_VISUALS"),
                   XA_INTEGER, 32, PropModeReplace,
                   (unsigned char *)visualdata, 2);
  cdk_x11_display_error_trap_pop_ignored (display);
}

void
_cdk_x11_screen_update_visuals_for_gl (CdkScreen *screen)
{
  CdkX11Screen *x11_screen;
  CdkDisplay *display;
  CdkX11Display *display_x11;
  Display *dpy;
  struct glvisualinfo *gl_info;
  int i;
  int system_visual_id, rgba_visual_id;

  x11_screen = CDK_X11_SCREEN (screen);
  display = x11_screen->display;
  display_x11 = CDK_X11_DISPLAY (display);
  dpy = cdk_x11_display_get_xdisplay (display);

  /* We save the default visuals as a property on the root window to avoid
     having to initialize GL each time, as it may not be used later. */
  if (get_cached_gl_visuals (display, &system_visual_id, &rgba_visual_id))
    {
      for (i = 0; i < x11_screen->nvisuals; i++)
        {
          CdkVisual *visual = x11_screen->visuals[i];
          int visual_id = cdk_x11_visual_get_xvisual (visual)->visualid;

          if (visual_id == system_visual_id)
            x11_screen->system_visual = visual;
          if (visual_id == rgba_visual_id)
            x11_screen->rgba_visual = visual;
        }

      return;
    }

  if (!cdk_x11_screen_init_gl (screen))
    return;

  gl_info = g_new0 (struct glvisualinfo, x11_screen->nvisuals);

  for (i = 0; i < x11_screen->nvisuals; i++)
    {
      XVisualInfo *visual_list;
      XVisualInfo visual_template;
      int nxvisuals;

      visual_template.screen = x11_screen->screen_num;
      visual_template.visualid = cdk_x11_visual_get_xvisual (x11_screen->visuals[i])->visualid;
      visual_list = XGetVisualInfo (x11_screen->xdisplay, VisualIDMask| VisualScreenMask, &visual_template, &nxvisuals);

      if (visual_list == NULL)
        continue;

      glXGetConfig (dpy, &visual_list[0], GLX_USE_GL, &gl_info[i].supports_gl);
      glXGetConfig (dpy, &visual_list[0], GLX_DOUBLEBUFFER, &gl_info[i].double_buffer);
      glXGetConfig (dpy, &visual_list[0], GLX_STEREO, &gl_info[i].stereo);
      glXGetConfig (dpy, &visual_list[0], GLX_ALPHA_SIZE, &gl_info[i].alpha_size);
      glXGetConfig (dpy, &visual_list[0], GLX_DEPTH_SIZE, &gl_info[i].depth_size);
      glXGetConfig (dpy, &visual_list[0], GLX_STENCIL_SIZE, &gl_info[i].stencil_size);

      if (display_x11->has_glx_multisample)
        glXGetConfig(dpy, &visual_list[0], GLX_SAMPLE_BUFFERS_ARB, &gl_info[i].num_multisample);

      if (display_x11->has_glx_visual_rating)
        glXGetConfig(dpy, &visual_list[0], GLX_VISUAL_CAVEAT_EXT, &gl_info[i].visual_caveat);
      else
        gl_info[i].visual_caveat = GLX_NONE_EXT;

      XFree (visual_list);
    }

  x11_screen->system_visual = pick_better_visual_for_gl (x11_screen, gl_info, x11_screen->system_visual);
  if (x11_screen->rgba_visual)
    x11_screen->rgba_visual = pick_better_visual_for_gl (x11_screen, gl_info, x11_screen->rgba_visual);

  g_free (gl_info);

  save_cached_gl_visuals (display,
                          cdk_x11_visual_get_xvisual (x11_screen->system_visual)->visualid,
                          x11_screen->rgba_visual ? cdk_x11_visual_get_xvisual (x11_screen->rgba_visual)->visualid : 0);
}

CdkGLContext *
cdk_x11_window_create_gl_context (CdkWindow    *window,
                                  gboolean      attached,
                                  CdkGLContext *share,
                                  GError      **error)
{
  CdkDisplay *display;
  CdkX11GLContext *context;
  CdkVisual *visual;
  GLXFBConfig config;

  display = cdk_window_get_display (window);

  if (!cdk_x11_screen_init_gl (cdk_window_get_screen (window)))
    {
      g_set_error_literal (error, CDK_GL_ERROR,
                           CDK_GL_ERROR_NOT_AVAILABLE,
                           _("No GL implementation is available"));
      return NULL;
    }

  visual = cdk_window_get_visual (window);
  if (!find_fbconfig_for_visual (display, visual, &config, error))
    return NULL;

  context = g_object_new (CDK_TYPE_X11_GL_CONTEXT,
                          "display", display,
                          "window", window,
                          "shared-context", share,
                          NULL);

  context->glx_config = config;
  context->is_attached = attached;

  return CDK_GL_CONTEXT (context);
}

gboolean
cdk_x11_display_make_gl_context_current (CdkDisplay   *display,
                                         CdkGLContext *context)
{
  CdkX11GLContext *context_x11;
  Display *dpy = cdk_x11_display_get_xdisplay (display);
  gboolean do_frame_sync = FALSE;

  if (context == NULL)
    {
      glXMakeContextCurrent (dpy, None, None, NULL);
      return TRUE;
    }

  context_x11 = CDK_X11_GL_CONTEXT (context);
  if (context_x11->glx_context == NULL)
    {
      g_critical ("No GLX context associated to the CdkGLContext; you must "
                  "call cdk_gl_context_realize() first.");
      return FALSE;
    }

  CDK_NOTE (OPENGL,
            g_message ("Making GLX context current to drawable %lu",
                       (unsigned long) context_x11->drawable));

  if (!glXMakeContextCurrent (dpy, context_x11->drawable, context_x11->drawable,
                              context_x11->glx_context))
    {
      CDK_NOTE (OPENGL,
                g_message ("Making GLX context current failed"));
      return FALSE;
    }

  if (context_x11->is_attached && CDK_X11_DISPLAY (display)->has_glx_swap_interval)
    {
      CdkWindow *window;
      CdkScreen *screen;

      window = cdk_gl_context_get_window (context);

      /* If the WM is compositing there is no particular need to delay
       * the swap when drawing on the offscreen, rendering to the screen
       * happens later anyway, and its up to the compositor to sync that
       * to the vblank. */
      screen = cdk_window_get_screen (window);
      do_frame_sync = ! cdk_screen_is_composited (screen);

      if (do_frame_sync != context_x11->do_frame_sync)
        {
          context_x11->do_frame_sync = do_frame_sync;

          if (do_frame_sync)
            glXSwapIntervalSGI (1);
          else
            glXSwapIntervalSGI (0);
        }
    }

  return TRUE;
}

/**
 * cdk_x11_display_get_glx_version:
 * @display: a #CdkDisplay
 * @major: (out): return location for the GLX major version
 * @minor: (out): return location for the GLX minor version
 *
 * Retrieves the version of the GLX implementation.
 *
 * Returns: %TRUE if GLX is available
 *
 * Since: 3.16
 */
gboolean
cdk_x11_display_get_glx_version (CdkDisplay *display,
                                 gint       *major,
                                 gint       *minor)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), FALSE);

  if (!CDK_IS_X11_DISPLAY (display))
    return FALSE;

  if (!cdk_x11_screen_init_gl (cdk_display_get_default_screen (display)))
    return FALSE;

  if (major != NULL)
    *major = CDK_X11_DISPLAY (display)->glx_version / 10;
  if (minor != NULL)
    *minor = CDK_X11_DISPLAY (display)->glx_version % 10;

  return TRUE;
}
