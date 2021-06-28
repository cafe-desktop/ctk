/* CDK - The GIMP Drawing Kit
 *
 * cdkglcontext-wayland.h: Private Wayland specific OpenGL wrappers
 *
 * Copyright © 2014  Emmanuele Bassi
 * Copyright © 2014  Red Hat, Int
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

#ifndef __CDK_WAYLAND_GL_CONTEXT__
#define __CDK_WAYLAND_GL_CONTEXT__

#include "cdkglcontextprivate.h"
#include "cdkdisplayprivate.h"
#include "cdkvisual.h"
#include "cdkwindow.h"
#include "cdkinternals.h"
#include "cdkmain.h"

#include <epoxy/egl.h>

G_BEGIN_DECLS

struct _CdkWaylandGLContext
{
  CdkGLContext parent_instance;

  EGLContext egl_context;
  EGLConfig egl_config;
  gboolean is_attached;
};

struct _CdkWaylandGLContextClass
{
  CdkGLContextClass parent_class;
};

gboolean        cdk_wayland_display_init_gl                         (CdkDisplay        *display);
CdkGLContext *  cdk_wayland_window_create_gl_context                (CdkWindow         *window,
								     gboolean           attach,
                                                                     CdkGLContext      *share,
                                                                     GError           **error);
void            cdk_wayland_window_invalidate_for_new_frame         (CdkWindow         *window,
                                                                     cairo_region_t    *update_area);
gboolean        cdk_wayland_display_make_gl_context_current         (CdkDisplay        *display,
                                                                     CdkGLContext      *context);

G_END_DECLS

#endif /* __CDK_WAYLAND_GL_CONTEXT__ */
