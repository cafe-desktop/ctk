/* CDK - The GIMP Drawing Kit
 * Copyright (C) 2010 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CDK_X11_DND_H__
#define __CDK_X11_DND_H__

#if !defined (__CDKX_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdkx.h> can be included directly."
#endif

#include <cdk/cdk.h>

G_BEGIN_DECLS

#define CDK_TYPE_X11_DRAG_CONTEXT              (cdk_x11_drag_context_get_type ())
#define CDK_X11_DRAG_CONTEXT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), CDK_TYPE_X11_DRAG_CONTEXT, CdkX11DragContext))
#define CDK_X11_DRAG_CONTEXT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CDK_TYPE_X11_DRAG_CONTEXT, CdkX11DragContextClass))
#define CDK_IS_X11_DRAG_CONTEXT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), CDK_TYPE_X11_DRAG_CONTEXT))
#define CDK_IS_X11_DRAG_CONTEXT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CDK_TYPE_X11_DRAG_CONTEXT))
#define CDK_X11_DRAG_CONTEXT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CDK_TYPE_X11_DRAG_CONTEXT, CdkX11DragContextClass))

#ifdef CDK_COMPILATION
typedef struct _CdkX11DragContext CdkX11DragContext;
#else
typedef CdkDragContext CdkX11DragContext;
#endif
typedef struct _CdkX11DragContextClass CdkX11DragContextClass;

CDK_AVAILABLE_IN_ALL
GType    cdk_x11_drag_context_get_type (void);

G_END_DECLS

#endif /* __CDK_X11_DRAG_CONTEXT_H__ */
