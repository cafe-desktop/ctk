/* cdkdrawable-quartz.h
 *
 * Copyright (C) 2005 Imendio AB
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

#ifndef __CDK_WINDOW_QUARTZ_H__
#define __CDK_WINDOW_QUARTZ_H__

#import <cdk/quartz/CdkQuartzView.h>
#import <cdk/quartz/CdkQuartzNSWindow.h>
#include "cdk/cdkwindowimpl.h"
#include "cdkinternal-quartz.h"

G_BEGIN_DECLS

/* Window implementation for Quartz
 */

typedef struct _CdkWindowImplQuartz CdkWindowImplQuartz;
typedef struct _CdkWindowImplQuartzClass CdkWindowImplQuartzClass;

#define CDK_TYPE_WINDOW_IMPL_QUARTZ              (_cdk_window_impl_quartz_get_type ())
#define CDK_WINDOW_IMPL_QUARTZ(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), CDK_TYPE_WINDOW_IMPL_QUARTZ, CdkWindowImplQuartz))
#define CDK_WINDOW_IMPL_QUARTZ_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CDK_TYPE_WINDOW_IMPL_QUARTZ, CdkWindowImplQuartzClass))
#define CDK_IS_WINDOW_IMPL_QUARTZ(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), CDK_TYPE_WINDOW_IMPL_QUARTZ))
#define CDK_IS_WINDOW_IMPL_QUARTZ_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CDK_TYPE_WINDOW_IMPL_QUARTZ))
#define CDK_WINDOW_IMPL_QUARTZ_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CDK_TYPE_WINDOW_IMPL_QUARTZ, CdkWindowImplQuartzClass))

struct _CdkWindowImplQuartz
{
  CdkWindowImpl parent_instance;

  CdkWindow *wrapper;

  NSWindow *toplevel;
  NSTrackingRectTag tracking_rect;
  CdkQuartzView *view;

  CdkWindowTypeHint type_hint;

  gint in_paint_rect_count;

  CdkWindow *transient_for;

  /* Sorted by z-order */
  GList *sorted_children;

  cairo_region_t *needs_display_region;

  cairo_surface_t *cairo_surface;

  gint shadow_top;

  gint shadow_max;
};
 
struct _CdkWindowImplQuartzClass 
{
  CdkWindowImplClass parent_class;

  CGContextRef  (* get_context)     (CdkWindowImplQuartz *window,
                                     gboolean             antialias);
  void          (* release_context) (CdkWindowImplQuartz *window,
                                     CGContextRef         cg_context);
};

GType _cdk_window_impl_quartz_get_type (void);

CGContextRef cdk_quartz_window_get_context     (CdkWindowImplQuartz *window,
                                                gboolean             antialias);
void         cdk_quartz_window_release_context (CdkWindowImplQuartz *window,
                                                CGContextRef         context);

/* Root window implementation for Quartz
 */

typedef struct _CdkRootWindowImplQuartz CdkRootWindowImplQuartz;
typedef struct _CdkRootWindowImplQuartzClass CdkRootWindowImplQuartzClass;

#define CDK_TYPE_ROOT_WINDOW_IMPL_QUARTZ              (_cdk_root_window_impl_quartz_get_type ())
#define CDK_ROOT_WINDOW_IMPL_QUARTZ(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), CDK_TYPE_ROOT_WINDOW_IMPL_QUARTZ, CdkRootWindowImplQuartz))
#define CDK_ROOT_WINDOW_IMPL_QUARTZ_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CDK_TYPE_ROOT_WINDOW_IMPL_QUARTZ, CdkRootWindowImplQuartzClass))
#define CDK_IS_ROOT_WINDOW_IMPL_QUARTZ(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), CDK_TYPE_ROOT_WINDOW_IMPL_QUARTZ))
#define CDK_IS_ROOT_WINDOW_IMPL_QUARTZ_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CDK_TYPE_ROOT_WINDOW_IMPL_QUARTZ))
#define CDK_ROOT_WINDOW_IMPL_QUARTZ_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CDK_TYPE_ROOT_WINDOW_IMPL_QUARTZ, CdkRootWindowImplQuartzClass))

struct _CdkRootWindowImplQuartz
{
  CdkWindowImplQuartz parent_instance;
};
 
struct _CdkRootWindowImplQuartzClass 
{
  CdkWindowImplQuartzClass parent_class;
};

GType _cdk_root_window_impl_quartz_get_type (void);

G_END_DECLS

#endif /* __CDK_WINDOW_QUARTZ_H__ */
