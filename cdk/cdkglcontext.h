/* CDK - The GIMP Drawing Kit
 *
 * cdkglcontext.h: GL context abstraction
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

#ifndef __CDK_GL_CONTEXT_H__
#define __CDK_GL_CONTEXT_H__

#if !defined (__CDK_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdk.h> can be included directly."
#endif

#include <cdk/cdkversionmacros.h>
#include <cdk/cdktypes.h>

G_BEGIN_DECLS

#define CDK_TYPE_GL_CONTEXT             (cdk_gl_context_get_type ())
#define CDK_GL_CONTEXT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), CDK_TYPE_GL_CONTEXT, CdkGLContext))
#define CDK_IS_GL_CONTEXT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CDK_TYPE_GL_CONTEXT))

#define CDK_GL_ERROR       (cdk_gl_error_quark ())

CDK_AVAILABLE_IN_3_16
GQuark cdk_gl_error_quark (void);

CDK_AVAILABLE_IN_3_16
GType cdk_gl_context_get_type (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_3_16
CdkDisplay *            cdk_gl_context_get_display              (CdkGLContext  *context);
CDK_AVAILABLE_IN_3_16
CdkWindow *             cdk_gl_context_get_window               (CdkGLContext  *context);
CDK_AVAILABLE_IN_3_16
CdkGLContext *          cdk_gl_context_get_shared_context       (CdkGLContext  *context);
CDK_AVAILABLE_IN_3_16
void                    cdk_gl_context_get_version              (CdkGLContext  *context,
                                                                 int           *major,
                                                                 int           *minor);
CDK_AVAILABLE_IN_3_20
gboolean                cdk_gl_context_is_legacy                (CdkGLContext  *context);

CDK_AVAILABLE_IN_3_16
void                    cdk_gl_context_set_required_version     (CdkGLContext  *context,
                                                                 int            major,
                                                                 int            minor);
CDK_AVAILABLE_IN_3_16
void                    cdk_gl_context_get_required_version     (CdkGLContext  *context,
                                                                 int           *major,
                                                                 int           *minor);
CDK_AVAILABLE_IN_3_16
void                    cdk_gl_context_set_debug_enabled        (CdkGLContext  *context,
                                                                 gboolean       enabled);
CDK_AVAILABLE_IN_3_16
gboolean                cdk_gl_context_get_debug_enabled        (CdkGLContext  *context);
CDK_AVAILABLE_IN_3_16
void                    cdk_gl_context_set_forward_compatible   (CdkGLContext  *context,
                                                                 gboolean       compatible);
CDK_AVAILABLE_IN_3_16
gboolean                cdk_gl_context_get_forward_compatible   (CdkGLContext  *context);
CDK_AVAILABLE_IN_3_22
void                    cdk_gl_context_set_use_es               (CdkGLContext  *context,
                                                                 int            use_es);
CDK_AVAILABLE_IN_3_22
gboolean                cdk_gl_context_get_use_es               (CdkGLContext  *context);

CDK_AVAILABLE_IN_3_16
gboolean                cdk_gl_context_realize                  (CdkGLContext  *context,
                                                                 GError       **error);
CDK_AVAILABLE_IN_3_16
void                    cdk_gl_context_make_current             (CdkGLContext  *context);
CDK_AVAILABLE_IN_3_16
CdkGLContext *          cdk_gl_context_get_current              (void);
CDK_AVAILABLE_IN_3_16
void                    cdk_gl_context_clear_current            (void);

G_END_DECLS

#endif /* __CDK_GL_CONTEXT_H__ */
