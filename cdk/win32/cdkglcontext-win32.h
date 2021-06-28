/* GDK - The GIMP Drawing Kit
 *
 * cdkglcontext-win32.h: Private Win32 specific OpenGL wrappers
 *
 * Copyright © 2014 Chun-wei Fan
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GDK_WIN32_GL_CONTEXT__
#define __GDK_WIN32_GL_CONTEXT__

#include <epoxy/gl.h>
#include <epoxy/wgl.h>

#ifdef GDK_WIN32_ENABLE_EGL
# include <epoxy/egl.h>
#endif

#include "cdkglcontextprivate.h"
#include "cdkdisplayprivate.h"
#include "cdkvisual.h"
#include "cdkwindow.h"
#include "cdkinternals.h"
#include "cdkmain.h"

G_BEGIN_DECLS

struct _CdkWin32GLContext
{
  CdkGLContext parent_instance;

  /* WGL Context Items */
  HGLRC hglrc;
  HDC gl_hdc;
  guint need_alpha_bits : 1;

  /* other items */
  guint is_attached : 1;
  guint do_frame_sync : 1;
  guint do_blit_swap : 1;

#ifdef GDK_WIN32_ENABLE_EGL
  /* EGL (Angle) Context Items */
  EGLContext egl_context;
  EGLConfig egl_config;
#endif
};

struct _CdkWin32GLContextClass
{
  CdkGLContextClass parent_class;
};

CdkGLContext *
_cdk_win32_window_create_gl_context (CdkWindow *window,
                                     gboolean attached,
                                     CdkGLContext *share,
                                     GError **error);

void
_cdk_win32_window_invalidate_for_new_frame (CdkWindow *window,
                                            cairo_region_t *update_area);

void
_cdk_win32_gl_context_end_frame (CdkGLContext *context,
                                 cairo_region_t *painted,
                                 cairo_region_t *damage);

gboolean
_cdk_win32_display_make_gl_context_current (CdkDisplay *display,
                                            CdkGLContext *context);

gboolean
_cdk_win32_gl_context_realize (CdkGLContext *context,
                               GError **error);

void
_cdk_win32_window_invalidate_egl_framebuffer (CdkWindow *window);

G_END_DECLS

#endif /* __GDK_WIN32_GL_CONTEXT__ */
