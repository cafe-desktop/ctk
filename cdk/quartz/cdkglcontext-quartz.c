/* CDK - The GIMP Drawing Kit
 *
 * cdkglcontext-quartz.c: Quartz specific OpenGL wrappers
 *
 * Copyright © 2014  Emmanuele Bassi
 * Copyright © 2014  Alexander Larsson
 * Copyright © 2014  Brion Vibber
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

#include "cdkglcontext-quartz.h"

#include "cdkquartzdisplay.h"
#include "cdkquartzglcontext.h"
#include "cdkquartzwindow.h"
#include "cdkprivate-quartz.h"
#include "cdkquartz-ctk-only.h"

#include "cdkinternals.h"

#include "cdkintl.h"

G_DEFINE_TYPE (CdkQuartzGLContext, cdk_quartz_gl_context, CDK_TYPE_GL_CONTEXT)

static void cdk_quartz_gl_context_dispose (GObject *gobject);

void
cdk_quartz_window_invalidate_for_new_frame (CdkWindow      *window,
                                            cairo_region_t *update_area)
{
  cairo_rectangle_int_t window_rect;

  /* Minimal update is ok if we're not drawing with gl */
  if (window->gl_paint_context == NULL)
    return;

  window_rect.x = 0;
  window_rect.y = 0;
  window_rect.width = cdk_window_get_width (window);
  window_rect.height = cdk_window_get_height (window);

  /* If nothing else is known, repaint everything so that the back
  buffer is fully up-to-date for the swapbuffer */
  cairo_region_union_rectangle (update_area, &window_rect);
}

static gboolean
cdk_quartz_gl_context_realize (CdkGLContext *context,
                               GError      **error)
{
  return TRUE;
}

static void
cdk_quartz_gl_context_end_frame (CdkGLContext *context,
                                 cairo_region_t *painted,
                                 cairo_region_t *damage)
{
  CdkQuartzGLContext *context_quartz = CDK_QUARTZ_GL_CONTEXT (context);

  [context_quartz->gl_context flushBuffer];
}

static void
cdk_quartz_gl_context_class_init (CdkQuartzGLContextClass *klass)
{
  CdkGLContextClass *context_class = CDK_GL_CONTEXT_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  context_class->realize = cdk_quartz_gl_context_realize;
  context_class->end_frame = cdk_quartz_gl_context_end_frame;
  gobject_class->dispose = cdk_quartz_gl_context_dispose;
}

static void
cdk_quartz_gl_context_init (CdkQuartzGLContext *self)
{
}

gboolean
cdk_quartz_display_init_gl (CdkDisplay *display)
{
  return TRUE;
}

CdkGLContext *
cdk_quartz_window_create_gl_context (CdkWindow     *window,
                                     gboolean       attached,
                                     CdkGLContext  *share,
                                     GError       **error)
{
  CdkDisplay *display = cdk_window_get_display (window);
  CdkQuartzGLContext *context;
  NSOpenGLContext *ctx;
  NSOpenGLPixelFormatAttribute attrs[] =
    {
      NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
      NSOpenGLPFADoubleBuffer,
      NSOpenGLPFAColorSize, 24,
      NSOpenGLPFAAlphaSize, 8,
      0
    };
  NSOpenGLPixelFormat *format = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];

  if (format == NULL)
    {
      g_set_error_literal (error, CDK_GL_ERROR,
                           CDK_GL_ERROR_NOT_AVAILABLE,
                           _("Unable to create a GL pixel format"));
      return NULL;
    }

  ctx = [[NSOpenGLContext alloc] initWithFormat:format
                                 shareContext:share ? CDK_QUARTZ_GL_CONTEXT (share)->gl_context : nil];
  if (ctx == NULL)
    {
      g_set_error_literal (error, CDK_GL_ERROR,
                           CDK_GL_ERROR_NOT_AVAILABLE,
                           _("Unable to create a GL context"));
      return NULL;
    }

  [format release];

  if (attached)
    {
      NSView *view = cdk_quartz_window_get_nsview (window);

      if ([view respondsToSelector:@selector(setWantsBestResolutionOpenGLSurface:)])
        [view setWantsBestResolutionOpenGLSurface:YES];

      GLint sync_to_framerate = 1;
      [ctx setValues:&sync_to_framerate forParameter:NSOpenGLCPSwapInterval];

      [ctx setView:view];
    }

  CDK_NOTE (OPENGL,
            g_print ("Created NSOpenGLContext[%p]\n", ctx));

  context = g_object_new (CDK_TYPE_QUARTZ_GL_CONTEXT,
                          "window", window,
                          "display", display,
                          "shared-context", share,
                          NULL);

  context->gl_context = ctx;
  context->is_attached = attached;

  return CDK_GL_CONTEXT (context);
}

static void
cdk_quartz_gl_context_dispose (GObject *gobject)
{
  CdkQuartzGLContext *context_quartz = CDK_QUARTZ_GL_CONTEXT (gobject);

  if (context_quartz->gl_context != NULL)
    {
      [context_quartz->gl_context clearDrawable];
      [context_quartz->gl_context release];
      context_quartz->gl_context = NULL;
    }

  G_OBJECT_CLASS (cdk_quartz_gl_context_parent_class)->dispose (gobject);
}

gboolean
cdk_quartz_display_make_gl_context_current (CdkDisplay   *display,
                                            CdkGLContext *context)
{
  CdkQuartzGLContext *context_quartz;

  if (context == NULL)
    {
      [NSOpenGLContext clearCurrentContext];
      return TRUE;
    }

  context_quartz = CDK_QUARTZ_GL_CONTEXT (context);

  [context_quartz->gl_context makeCurrentContext];

  return TRUE;
}
