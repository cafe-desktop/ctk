/* CDK - The GIMP Drawing Kit
 *
 * cdkglcontext-x11.h: Private X11 specific OpenGL wrappers
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

#ifndef __CDK_X11_GL_CONTEXT__
#define __CDK_X11_GL_CONTEXT__

#include <X11/X.h>
#include <X11/Xlib.h>

#include <epoxy/gl.h>
#include <epoxy/glx.h>

#include "cdkglcontextprivate.h"
#include "cdkdisplayprivate.h"
#include "cdkvisual.h"
#include "cdkwindow.h"
#include "cdkinternals.h"
#include "cdkmain.h"

G_BEGIN_DECLS

struct _CdkX11GLContext
{
  CdkGLContext parent_instance;

  GLXContext glx_context;
  GLXFBConfig glx_config;
  GLXDrawable drawable;

  guint is_attached : 1;
  guint is_direct : 1;
  guint do_frame_sync : 1;

  guint do_blit_swap : 1;
};

struct _CdkX11GLContextClass
{
  CdkGLContextClass parent_class;
};

gboolean        cdk_x11_screen_init_gl                          (CdkScreen         *screen);
CdkGLContext *  cdk_x11_window_create_gl_context                (CdkWindow         *window,
								 gboolean           attached,
                                                                 CdkGLContext      *share,
                                                                 GError           **error);
void            cdk_x11_window_invalidate_for_new_frame         (CdkWindow         *window,
                                                                 cairo_region_t    *update_area);
gboolean        cdk_x11_display_make_gl_context_current         (CdkDisplay        *display,
                                                                 CdkGLContext      *context);

G_END_DECLS

#endif /* __CDK_X11_GL_CONTEXT__ */
