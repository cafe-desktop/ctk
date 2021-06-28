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

#ifndef __GDK_QUARTZ_DND_H__
#define __GDK_QUARTZ_DND_H__

#if !defined (CTK_COMPILATION) && !defined (GDK_COMPILATION)
#error "cdkquartzdnd.h is for Ctk's internal use only"
#endif

#include <cdk/cdk.h>

G_BEGIN_DECLS

#define GDK_TYPE_QUARTZ_DRAG_CONTEXT              (cdk_quartz_drag_context_get_type ())
#define GDK_QUARTZ_DRAG_CONTEXT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_QUARTZ_DRAG_CONTEXT, CdkQuartzDragContext))
#define GDK_QUARTZ_DRAG_CONTEXT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_QUARTZ_DRAG_CONTEXT, CdkQuartzDragContextClass))
#define GDK_IS_QUARTZ_DRAG_CONTEXT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_QUARTZ_DRAG_CONTEXT))
#define GDK_IS_QUARTZ_DRAG_CONTEXT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_QUARTZ_DRAG_CONTEXT))
#define GDK_QUARTZ_DRAG_CONTEXT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_QUARTZ_DRAG_CONTEXT, CdkQuartzDragContextClass))

#ifdef GDK_COMPILATION
typedef struct _CdkQuartzDragContext CdkQuartzDragContext;
#else
typedef CdkDragContext CdkQuartzDragContext;
#endif
typedef struct _CdkQuartzDragContextClass CdkQuartzDragContextClass;


GDK_AVAILABLE_IN_ALL
GType     cdk_quartz_drag_context_get_type (void);

GDK_AVAILABLE_IN_ALL
id        cdk_quartz_drag_context_get_dragging_info_libctk_only (CdkDragContext *context);

GDK_AVAILABLE_IN_ALL
CdkDragContext *cdk_quartz_drag_source_context_libctk_only (void);

G_END_DECLS

#endif /* __GDK_QUARTZ_DRAG_CONTEXT_H__ */
