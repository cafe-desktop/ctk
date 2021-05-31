/*
 * Copyright © 2015 Red Hat Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#ifndef __CTK_CSS_CUSTOM_GADGET_PRIVATE_H__
#define __CTK_CSS_CUSTOM_GADGET_PRIVATE_H__

#include "ctk/ctkcssgadgetprivate.h"

G_BEGIN_DECLS

#define CTK_TYPE_CSS_CUSTOM_GADGET           (ctk_css_custom_gadget_get_type ())
#define CTK_CSS_CUSTOM_GADGET(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, CTK_TYPE_CSS_CUSTOM_GADGET, CtkCssCustomGadget))
#define CTK_CSS_CUSTOM_GADGET_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, CTK_TYPE_CSS_CUSTOM_GADGET, CtkCssCustomGadgetClass))
#define CTK_IS_CSS_CUSTOM_GADGET(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, CTK_TYPE_CSS_CUSTOM_GADGET))
#define CTK_IS_CSS_CUSTOM_GADGET_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, CTK_TYPE_CSS_CUSTOM_GADGET))
#define CTK_CSS_CUSTOM_GADGET_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CSS_CUSTOM_GADGET, CtkCssCustomGadgetClass))

typedef struct _CtkCssCustomGadget           CtkCssCustomGadget;
typedef struct _CtkCssCustomGadgetClass      CtkCssCustomGadgetClass;

typedef void    (* CtkCssPreferredSizeFunc)             (CtkCssGadget           *gadget,
                                                         CtkOrientation          orientation,
                                                         gint                    for_size,
                                                         gint                   *minimum,
                                                         gint                   *natural,
                                                         gint                   *minimum_baseline,
                                                         gint                   *natural_baseline,
                                                         gpointer                data);
typedef void    (* CtkCssAllocateFunc)                  (CtkCssGadget           *gadget,
                                                         const CtkAllocation    *allocation,
                                                         int                     baseline,
                                                         CtkAllocation          *out_clip,
                                                         gpointer                data);
typedef gboolean (* CtkCssDrawFunc)                     (CtkCssGadget           *gadget,
                                                         cairo_t                *cr,
                                                         int                     x,
                                                         int                     y,
                                                         int                     width,
                                                         int                     height,
                                                         gpointer                data);
struct _CtkCssCustomGadget
{
  CtkCssGadget parent;
};

struct _CtkCssCustomGadgetClass
{
  CtkCssGadgetClass  parent_class;
};

GType           ctk_css_custom_gadget_get_type                 (void) G_GNUC_CONST;

CtkCssGadget *  ctk_css_custom_gadget_new                      (const char                      *name,
                                                                CtkWidget                       *owner,
                                                                CtkCssGadget                    *parent,
                                                                CtkCssGadget                    *next_sibling,
                                                                CtkCssPreferredSizeFunc          get_preferred_size_func,
                                                                CtkCssAllocateFunc               allocate_func,
                                                                CtkCssDrawFunc                   draw_func,
                                                                gpointer                         data,
                                                                GDestroyNotify                   destroy_func);
CtkCssGadget *  ctk_css_custom_gadget_new_for_node             (CtkCssNode                      *node,
                                                                CtkWidget                       *owner,
                                                                CtkCssPreferredSizeFunc          preferred_size_func,
                                                                CtkCssAllocateFunc               allocate_func,
                                                                CtkCssDrawFunc                   draw_func,
                                                                gpointer                         data,
                                                                GDestroyNotify                   destroy_func);

G_END_DECLS

#endif /* __CTK_CSS_CUSTOM_GADGET_PRIVATE_H__ */
