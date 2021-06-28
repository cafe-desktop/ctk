/* cdkquartzdnd.h
 *
 * Copyright (C) 2010 Kristian Rietveld  <kris@ctk.org>
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

#ifndef __CDK_QUARTZ_DND_H__
#define __CDK_QUARTZ_DND_H__

#if !defined (CTK_COMPILATION) && !defined (CDK_COMPILATION)
#error "cdkquartzdnd.h is for Ctk's internal use only"
#endif

#include <cdk/cdk.h>

G_BEGIN_DECLS

#define CDK_TYPE_QUARTZ_DRAG_CONTEXT              (cdk_quartz_drag_context_get_type ())
#define CDK_QUARTZ_DRAG_CONTEXT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), CDK_TYPE_QUARTZ_DRAG_CONTEXT, CdkQuartzDragContext))
#define CDK_QUARTZ_DRAG_CONTEXT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CDK_TYPE_QUARTZ_DRAG_CONTEXT, CdkQuartzDragContextClass))
#define CDK_IS_QUARTZ_DRAG_CONTEXT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), CDK_TYPE_QUARTZ_DRAG_CONTEXT))
#define CDK_IS_QUARTZ_DRAG_CONTEXT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CDK_TYPE_QUARTZ_DRAG_CONTEXT))
#define CDK_QUARTZ_DRAG_CONTEXT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CDK_TYPE_QUARTZ_DRAG_CONTEXT, CdkQuartzDragContextClass))

#ifdef CDK_COMPILATION
typedef struct _CdkQuartzDragContext CdkQuartzDragContext;
#else
typedef CdkDragContext CdkQuartzDragContext;
#endif
typedef struct _CdkQuartzDragContextClass CdkQuartzDragContextClass;


CDK_AVAILABLE_IN_ALL
GType     cdk_quartz_drag_context_get_type (void);

CDK_AVAILABLE_IN_ALL
id        cdk_quartz_drag_context_get_dragging_info_libctk_only (CdkDragContext *context);

CDK_AVAILABLE_IN_ALL
CdkDragContext *cdk_quartz_drag_source_context_libctk_only (void);

G_END_DECLS

#endif /* __CDK_QUARTZ_DRAG_CONTEXT_H__ */
