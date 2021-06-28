/* ctkscrollable.h
 * Copyright (C) 2008 Tadej Borovšak <tadeboro@gmail.com>
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

#ifndef __CTK_SCROLLABLE_H__
#define __CTK_SCROLLABLE_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <cdk/cdk.h>
#include <ctk/ctkenums.h>
#include <ctk/ctktypes.h>
#include <ctk/ctkborder.h>

G_BEGIN_DECLS

#define CTK_TYPE_SCROLLABLE            (ctk_scrollable_get_type ())
#define CTK_SCROLLABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj),     CTK_TYPE_SCROLLABLE, CtkScrollable))
#define CTK_IS_SCROLLABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj),     CTK_TYPE_SCROLLABLE))
#define CTK_SCROLLABLE_GET_IFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), CTK_TYPE_SCROLLABLE, CtkScrollableInterface))

typedef struct _CtkScrollable          CtkScrollable; /* Dummy */
typedef struct _CtkScrollableInterface CtkScrollableInterface;

struct _CtkScrollableInterface
{
  GTypeInterface base_iface;

  gboolean (* get_border) (CtkScrollable *scrollable,
                           CtkBorder     *border);
};

/* Public API */
CDK_AVAILABLE_IN_ALL
GType                ctk_scrollable_get_type               (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkAdjustment       *ctk_scrollable_get_hadjustment        (CtkScrollable       *scrollable);
CDK_AVAILABLE_IN_ALL
void                 ctk_scrollable_set_hadjustment        (CtkScrollable       *scrollable,
							    CtkAdjustment       *hadjustment);
CDK_AVAILABLE_IN_ALL
CtkAdjustment       *ctk_scrollable_get_vadjustment        (CtkScrollable       *scrollable);
CDK_AVAILABLE_IN_ALL
void                 ctk_scrollable_set_vadjustment        (CtkScrollable       *scrollable,
							    CtkAdjustment       *vadjustment);
CDK_AVAILABLE_IN_ALL
CtkScrollablePolicy  ctk_scrollable_get_hscroll_policy     (CtkScrollable       *scrollable);
CDK_AVAILABLE_IN_ALL
void                 ctk_scrollable_set_hscroll_policy     (CtkScrollable       *scrollable,
							    CtkScrollablePolicy  policy);
CDK_AVAILABLE_IN_ALL
CtkScrollablePolicy  ctk_scrollable_get_vscroll_policy     (CtkScrollable       *scrollable);
CDK_AVAILABLE_IN_ALL
void                 ctk_scrollable_set_vscroll_policy     (CtkScrollable       *scrollable,
							    CtkScrollablePolicy  policy);

CDK_AVAILABLE_IN_3_16
gboolean             ctk_scrollable_get_border             (CtkScrollable       *scrollable,
                                                            CtkBorder           *border);

G_END_DECLS

#endif /* __CTK_SCROLLABLE_H__ */
