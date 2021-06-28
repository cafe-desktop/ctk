/* CDK - The GIMP Drawing Kit
 *
 * cdkglcontext-wayland.c: Wayland specific OpenGL wrappers
 *
 * Copyright © 2014  Emmanuele Bassi
 * Copyright © 2014  Alexander Larsson
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

#include "cdkglcontext-wayland.h"
#include "cdkdisplay-wayland.h"

#include "cdkwaylanddisplay.h"
#include "cdkwaylandglcontext.h"
#include "cdkwaylandwindow.h"
#include "cdkprivate-wayland.h"

#include "cdkinternals.h"

#include "cdkintl.h"

G_DEFINE_TYPE (CdkWaylandGLContext, cdk_wayland_gl_context, CDK_TYPE_GL_CONTEXT)

static void cdk_x11_gl_context_dispose (GObject *gobject);

void
cdk_wayland_window_invalidate_for_new_frame (CdkWindow      *window,
                                             cairo_region_t *update_area)
{
  cairo_rectangle_int_t window_rect;
  CdkDisplay *display = cdk_window_get_display (window);
  CdkWaylandDisplay *display_wayland = CDK_WAYLAND_DISPLAY (display);
  CdkWaylandGLContext *context_wayland;
  int buffer_age;
  gboolean invalidate_all;
  EGLSurface egl_surface;

  /* Minimal update is ok if we're not drawing with gl */
  if (window->gl_paint_context == NULL)
    return;

  context_wayland = CDK_WAYLAND_GL_CONTEXT (window->gl_paint_context);
  buffer_age = 0;

  egl_surface = cdk_wayland_window_get_egl_surface (window->impl_window,
                                                    context_wayland->egl_config);

  if (display_wayland->have_egl_buffer_age)
    {
      cdk_gl_context_make_current (window->gl_paint_context);
      eglQuerySurface (display_wayland->egl_display, egl_surface,
		       EGL_BUFFER_AGE_EXT, &buffer_age);
    }

  invalidate_all = FALSE;
  if (buffer_age == 0 || buffer_age >= 4)
    invalidate_all = TRUE;
  else
    {
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
       * buffer is fully up-to-date for the swapbuffer
       */
      cairo_region_union_rectangle (update_area, &window_rect);
    }
}

#define N_EGL_ATTRS     16

static gboolean
cdk_wayland_gl_context_realize (CdkGLContext *context,
                                GError      **error)
{
  CdkWaylandGLContext *context_wayland = CDK_WAYLAND_GL_CONTEXT (context);
  CdkDisplay *display = cdk_gl_context_get_display (context);
  CdkGLContext *share = cdk_gl_context_get_shared_context (context);
  CdkWaylandDisplay *display_wayland = CDK_WAYLAND_DISPLAY (display);
  EGLContext ctx;
  EGLint context_attribs[N_EGL_ATTRS];
  int major, minor, flags;
  gboolean debug_bit, forward_bit, legacy_bit, use_es;
  int i = 0;

  cdk_gl_context_get_required_version (context, &major, &minor);
  debug_bit = cdk_gl_context_get_debug_enabled (context);
  forward_bit = cdk_gl_context_get_forward_compatible (context);
  legacy_bit = (_cdk_gl_flags & CDK_GL_LEGACY) != 0 ||
               (share != NULL && cdk_gl_context_is_legacy (share));
  use_es = (_cdk_gl_flags & CDK_GL_GLES) != 0 ||
           (share != NULL && cdk_gl_context_get_use_es (share));

  flags = 0;

  if (debug_bit)
    flags |= EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR;
  if (forward_bit)
    flags |= EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR;

  if (!use_es)
    {
      eglBindAPI (EGL_OPENGL_API);

      /* We want a core profile, unless in legacy mode */
      context_attribs[i++] = EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR;
      context_attribs[i++] = legacy_bit
                           ? EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR
                           : EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR;

      /* Specify the version */
      context_attribs[i++] = EGL_CONTEXT_MAJOR_VERSION_KHR;
      context_attribs[i++] = legacy_bit ? 3 : major;
      context_attribs[i++] = EGL_CONTEXT_MINOR_VERSION_KHR;
      context_attribs[i++] = legacy_bit ? 0 : minor;
    }
  else
    {
      eglBindAPI (EGL_OPENGL_ES_API);

      context_attribs[i++] = EGL_CONTEXT_CLIENT_VERSION;
      if (major == 3)
        context_attribs[i++] = 3;
      else
        context_attribs[i++] = 2;
    }

  /* Specify the flags */
  context_attribs[i++] = EGL_CONTEXT_FLAGS_KHR;
  context_attribs[i++] = flags;

  context_attribs[i++] = EGL_NONE;
  g_assert (i < N_EGL_ATTRS);

  CDK_NOTE (OPENGL, g_message ("Creating EGL context version %d.%d (debug:%s, forward:%s, legacy:%s, es:%s)",
                               major, minor,
                               debug_bit ? "yes" : "no",
                               forward_bit ? "yes" : "no",
                               legacy_bit ? "yes" : "no",
                               use_es ? "yes" : "no"));

  ctx = eglCreateContext (display_wayland->egl_display,
                          context_wayland->egl_config,
                          share != NULL ? CDK_WAYLAND_GL_CONTEXT (share)->egl_context
                                        : EGL_NO_CONTEXT,
                          context_attribs);

  /* If context creation failed without the legacy bit, let's try again with it */
  if (ctx == NULL && !legacy_bit)
    {
      /* Ensure that re-ordering does not break the offsets */
      g_assert (context_attribs[0] == EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR);
      context_attribs[1] = EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR;
      context_attribs[3] = 3;
      context_attribs[5] = 0;

      eglBindAPI (EGL_OPENGL_API);

      legacy_bit = TRUE;
      use_es = FALSE;

      CDK_NOTE (OPENGL, g_message ("eglCreateContext failed, switching to legacy"));
      ctx = eglCreateContext (display_wayland->egl_display,
                              context_wayland->egl_config,
                              share != NULL ? CDK_WAYLAND_GL_CONTEXT (share)->egl_context
                                            : EGL_NO_CONTEXT,
                              context_attribs);
    }

  if (ctx == NULL)
    {
      g_set_error_literal (error, CDK_GL_ERROR,
                           CDK_GL_ERROR_NOT_AVAILABLE,
                           _("Unable to create a GL context"));
      return FALSE;
    }

  CDK_NOTE (OPENGL, g_message ("Created EGL context[%p]", ctx));

  context_wayland->egl_context = ctx;

  cdk_gl_context_set_is_legacy (context, legacy_bit);
  cdk_gl_context_set_use_es (context, use_es);

  return TRUE;
}

static void
cdk_wayland_gl_context_end_frame (CdkGLContext   *context,
                                  cairo_region_t *painted,
                                  cairo_region_t *damage)
{
  CdkWindow *window = cdk_gl_context_get_window (context);
  CdkDisplay *display = cdk_window_get_display (window);
  CdkWaylandDisplay *display_wayland = CDK_WAYLAND_DISPLAY (display);
  CdkWaylandGLContext *context_wayland = CDK_WAYLAND_GL_CONTEXT (context);
  EGLSurface egl_surface;

  cdk_gl_context_make_current (context);

  egl_surface = cdk_wayland_window_get_egl_surface (window->impl_window,
                                                    context_wayland->egl_config);

  if (display_wayland->have_egl_swap_buffers_with_damage)
    {
      int i, j, n_rects = cairo_region_num_rectangles (damage);
      EGLint *rects = g_new (EGLint, n_rects * 4);
      cairo_rectangle_int_t rect;
      int window_height = cdk_window_get_height (window);

      for (i = 0, j = 0; i < n_rects; i++)
        {
          cairo_region_get_rectangle (damage, i, &rect);
          rects[j++] = rect.x;
          rects[j++] = window_height - rect.height - rect.y;
          rects[j++] = rect.width;
          rects[j++] = rect.height;
        }
      eglSwapBuffersWithDamageEXT (display_wayland->egl_display, egl_surface, rects, n_rects);
      g_free (rects);
    }
  else
    eglSwapBuffers (display_wayland->egl_display, egl_surface);
}

static void
cdk_wayland_gl_context_class_init (CdkWaylandGLContextClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  CdkGLContextClass *context_class = CDK_GL_CONTEXT_CLASS (klass);

  gobject_class->dispose = cdk_x11_gl_context_dispose;

  context_class->realize = cdk_wayland_gl_context_realize;
  context_class->end_frame = cdk_wayland_gl_context_end_frame;
}

static void
cdk_wayland_gl_context_init (CdkWaylandGLContext *self)
{
}

static EGLDisplay
cdk_wayland_get_display (CdkWaylandDisplay *display_wayland)
{
  EGLDisplay dpy = NULL;

  if (epoxy_has_egl_extension (NULL, "EGL_KHR_platform_base"))
    {
      PFNEGLGETPLATFORMDISPLAYPROC getPlatformDisplay =
	(void *) eglGetProcAddress ("eglGetPlatformDisplay");

      if (getPlatformDisplay)
	dpy = getPlatformDisplay (EGL_PLATFORM_WAYLAND_EXT,
				  display_wayland->wl_display,
				  NULL);
      if (dpy)
	return dpy;
    }

  if (epoxy_has_egl_extension (NULL, "EGL_EXT_platform_base"))
    {
      PFNEGLGETPLATFORMDISPLAYEXTPROC getPlatformDisplay =
	(void *) eglGetProcAddress ("eglGetPlatformDisplayEXT");

      if (getPlatformDisplay)
	dpy = getPlatformDisplay (EGL_PLATFORM_WAYLAND_EXT,
				  display_wayland->wl_display,
				  NULL);
      if (dpy)
	return dpy;
    }

  return eglGetDisplay ((EGLNativeDisplayType) display_wayland->wl_display);
}

gboolean
cdk_wayland_display_init_gl (CdkDisplay *display)
{
  CdkWaylandDisplay *display_wayland = CDK_WAYLAND_DISPLAY (display);
  EGLint major, minor;
  EGLDisplay dpy;

  if (display_wayland->have_egl)
    return TRUE;

  dpy = cdk_wayland_get_display (display_wayland);

  if (dpy == NULL)
    return FALSE;

  if (!eglInitialize (dpy, &major, &minor))
    return FALSE;

  if (!eglBindAPI (EGL_OPENGL_API))
    return FALSE;

  display_wayland->egl_display = dpy;
  display_wayland->egl_major_version = major;
  display_wayland->egl_minor_version = minor;

  display_wayland->have_egl = TRUE;

  display_wayland->have_egl_khr_create_context =
    epoxy_has_egl_extension (dpy, "EGL_KHR_create_context");

  display_wayland->have_egl_buffer_age =
    epoxy_has_egl_extension (dpy, "EGL_EXT_buffer_age");

  display_wayland->have_egl_swap_buffers_with_damage =
    epoxy_has_egl_extension (dpy, "EGL_EXT_swap_buffers_with_damage");

  display_wayland->have_egl_surfaceless_context =
    epoxy_has_egl_extension (dpy, "EGL_KHR_surfaceless_context");

  CDK_NOTE (OPENGL,
            g_message ("EGL API version %d.%d found\n"
                       " - Vendor: %s\n"
                       " - Version: %s\n"
                       " - Client APIs: %s\n"
                       " - Extensions:\n"
                       "\t%s",
                       display_wayland->egl_major_version,
                       display_wayland->egl_minor_version,
                       eglQueryString (dpy, EGL_VENDOR),
                       eglQueryString (dpy, EGL_VERSION),
                       eglQueryString (dpy, EGL_CLIENT_APIS),
                       eglQueryString (dpy, EGL_EXTENSIONS)));

  return TRUE;
}

#define MAX_EGL_ATTRS   30

static gboolean
find_eglconfig_for_window (CdkWindow  *window,
                           EGLConfig  *egl_config_out,
                           EGLint     *min_swap_interval_out,
                           GError    **error)
{
  CdkDisplay *display = cdk_window_get_display (window);
  CdkWaylandDisplay *display_wayland = CDK_WAYLAND_DISPLAY (display);
  CdkVisual *visual = cdk_window_get_visual (window);
  EGLint attrs[MAX_EGL_ATTRS];
  EGLint count;
  EGLConfig *configs, chosen_config;
  gboolean use_rgba;

  int i = 0;

  attrs[i++] = EGL_SURFACE_TYPE;
  attrs[i++] = EGL_WINDOW_BIT;

  attrs[i++] = EGL_COLOR_BUFFER_TYPE;
  attrs[i++] = EGL_RGB_BUFFER;

  attrs[i++] = EGL_RED_SIZE;
  attrs[i++] = 8;
  attrs[i++] = EGL_GREEN_SIZE;
  attrs[i++] = 8;
  attrs[i++] = EGL_BLUE_SIZE;
  attrs[i++] = 8;

  use_rgba = (visual == cdk_screen_get_rgba_visual (cdk_display_get_default_screen (display)));

  if (use_rgba)
    {
      attrs[i++] = EGL_ALPHA_SIZE;
      attrs[i++] = 8;
    }
  else
    {
      attrs[i++] = EGL_ALPHA_SIZE;
      attrs[i++] = 0;
    }

  attrs[i++] = EGL_NONE;
  g_assert (i < MAX_EGL_ATTRS);

  if (!eglChooseConfig (display_wayland->egl_display, attrs, NULL, 0, &count) || count < 1)
    {
      g_set_error_literal (error, CDK_GL_ERROR,
                           CDK_GL_ERROR_UNSUPPORTED_FORMAT,
                           _("No available configurations for the given pixel format"));
      return FALSE;
    }

  configs = g_new (EGLConfig, count);

  if (!eglChooseConfig (display_wayland->egl_display, attrs, configs, count, &count) || count < 1)
    {
      g_set_error_literal (error, CDK_GL_ERROR,
                           CDK_GL_ERROR_UNSUPPORTED_FORMAT,
                           _("No available configurations for the given pixel format"));
      return FALSE;
    }

  /* Pick first valid configuration i guess? */
  chosen_config = configs[0];

  if (!eglGetConfigAttrib (display_wayland->egl_display, chosen_config,
                           EGL_MIN_SWAP_INTERVAL, min_swap_interval_out))
    {
      g_set_error_literal (error, CDK_GL_ERROR,
                           CDK_GL_ERROR_NOT_AVAILABLE,
                           "Could not retrieve the minimum swap interval");
      g_free (configs);
      return FALSE;
    }

  if (egl_config_out != NULL)
    *egl_config_out = chosen_config;

  g_free (configs);

  return TRUE;
}

CdkGLContext *
cdk_wayland_window_create_gl_context (CdkWindow     *window,
				      gboolean       attached,
                                      CdkGLContext  *share,
                                      GError       **error)
{
  CdkDisplay *display = cdk_window_get_display (window);
  CdkWaylandDisplay *display_wayland = CDK_WAYLAND_DISPLAY (display);
  CdkWaylandGLContext *context;
  EGLConfig config;

  if (!cdk_wayland_display_init_gl (display))
    {
      g_set_error_literal (error, CDK_GL_ERROR,
                           CDK_GL_ERROR_NOT_AVAILABLE,
                           _("No GL implementation is available"));
      return NULL;
    }

  if (!display_wayland->have_egl_khr_create_context)
    {
      g_set_error_literal (error, CDK_GL_ERROR,
                           CDK_GL_ERROR_UNSUPPORTED_PROFILE,
                           _("Core GL is not available on EGL implementation"));
      return NULL;
    }

  if (!find_eglconfig_for_window (window, &config,
                                  &display_wayland->egl_min_swap_interval,
                                  error))
    return NULL;

  context = g_object_new (CDK_TYPE_WAYLAND_GL_CONTEXT,
                          "display", display,
                          "window", window,
                          "shared-context", share,
                          NULL);

  context->egl_config = config;
  context->is_attached = attached;

  return CDK_GL_CONTEXT (context);
}

static void
cdk_x11_gl_context_dispose (GObject *gobject)
{
  CdkWaylandGLContext *context_wayland = CDK_WAYLAND_GL_CONTEXT (gobject);

  if (context_wayland->egl_context != NULL)
    {
      CdkGLContext *context = CDK_GL_CONTEXT (gobject);
      CdkWindow *window = cdk_gl_context_get_window (context);
      CdkDisplay *display = cdk_window_get_display (window);
      CdkWaylandDisplay *display_wayland = CDK_WAYLAND_DISPLAY (display);

      if (eglGetCurrentContext () == context_wayland->egl_context)
        eglMakeCurrent(display_wayland->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                       EGL_NO_CONTEXT);

      CDK_NOTE (OPENGL, g_message ("Destroying EGL context"));

      eglDestroyContext (display_wayland->egl_display,
                         context_wayland->egl_context);
      context_wayland->egl_context = NULL;
    }

  G_OBJECT_CLASS (cdk_wayland_gl_context_parent_class)->dispose (gobject);
}

gboolean
cdk_wayland_display_make_gl_context_current (CdkDisplay   *display,
                                             CdkGLContext *context)
{
  CdkWaylandDisplay *display_wayland = CDK_WAYLAND_DISPLAY (display);
  CdkWaylandGLContext *context_wayland;
  CdkWindow *window;
  EGLSurface egl_surface;

  if (context == NULL)
    {
      eglMakeCurrent(display_wayland->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                     EGL_NO_CONTEXT);
      return TRUE;
    }

  context_wayland = CDK_WAYLAND_GL_CONTEXT (context);
  window = cdk_gl_context_get_window (context);

  if (context_wayland->is_attached)
    egl_surface = cdk_wayland_window_get_egl_surface (window->impl_window, context_wayland->egl_config);
  else
    {
      if (display_wayland->have_egl_surfaceless_context)
	egl_surface = EGL_NO_SURFACE;
      else
	egl_surface = cdk_wayland_window_get_dummy_egl_surface (window->impl_window,
								context_wayland->egl_config);
    }

  if (!eglMakeCurrent (display_wayland->egl_display, egl_surface,
                       egl_surface, context_wayland->egl_context))
    {
      g_warning ("eglMakeCurrent failed");
      return FALSE;
    }

  if (display_wayland->egl_min_swap_interval == 0)
    eglSwapInterval (display_wayland->egl_display, 0);
  else
    g_debug ("Can't disable GL swap interval");

  return TRUE;
}
