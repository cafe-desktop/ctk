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

#ifndef __CTK_CSS_GADGET_PRIVATE_H__
#define __CTK_CSS_GADGET_PRIVATE_H__

#include <cairo.h>
#include <glib-object.h>

#include "ctk/ctkwidget.h"
#include "ctk/ctkcssstylechangeprivate.h"
#include "ctk/ctkcsstypesprivate.h"

G_BEGIN_DECLS

#define CTK_TYPE_CSS_GADGET           (ctk_css_gadget_get_type ())
#define CTK_CSS_GADGET(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, CTK_TYPE_CSS_GADGET, CtkCssGadget))
#define CTK_CSS_GADGET_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, CTK_TYPE_CSS_GADGET, CtkCssGadgetClass))
#define CTK_IS_CSS_GADGET(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, CTK_TYPE_CSS_GADGET))
#define CTK_IS_CSS_GADGET_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, CTK_TYPE_CSS_GADGET))
#define CTK_CSS_GADGET_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CSS_GADGET, CtkCssGadgetClass))

typedef struct _CtkCssGadget           CtkCssGadget;
typedef struct _CtkCssGadgetClass      CtkCssGadgetClass;

struct _CtkCssGadget
{
  GObject parent;
};

struct _CtkCssGadgetClass
{
  GObjectClass  parent_class;

  void          (* get_preferred_size)                  (CtkCssGadget           *gadget,
                                                         CtkOrientation          orientation,
                                                         gint                    for_size,
                                                         gint                   *minimum,
                                                         gint                   *natural,
                                                         gint                   *minimum_baseline,
                                                         gint                   *natural_baseline);

  void          (* allocate)                            (CtkCssGadget           *gadget,
                                                         const CtkAllocation    *allocation,
                                                         int                     baseline,
                                                         CtkAllocation          *out_clip);

  gboolean      (* draw)                                (CtkCssGadget           *gadget,
                                                         cairo_t                *cr,
                                                         int                     x,
                                                         int                     y,
                                                         int                     width,
                                                         int                     height);

  void          (* style_changed)                       (CtkCssGadget           *gadget,
                                                         CtkCssStyleChange      *change);
};

GType           ctk_css_gadget_get_type                 (void) G_GNUC_CONST;

CtkCssNode *    ctk_css_gadget_get_node                 (CtkCssGadget           *gadget);
CtkCssStyle *   ctk_css_gadget_get_style                (CtkCssGadget           *gadget);
CtkWidget *     ctk_css_gadget_get_owner                (CtkCssGadget           *gadget);

void            ctk_css_gadget_set_node                 (CtkCssGadget           *gadget,
                                                         CtkCssNode             *node);
void            ctk_css_gadget_set_visible              (CtkCssGadget           *gadget,
                                                         gboolean                visible);
gboolean        ctk_css_gadget_get_visible              (CtkCssGadget           *gadget);

gboolean        ctk_css_gadget_margin_box_contains_point (CtkCssGadget          *gadget,
                                                          int                    x,
                                                          int                    y);
gboolean        ctk_css_gadget_border_box_contains_point (CtkCssGadget          *gadget,
                                                          int                    x,
                                                          int                    y);
gboolean        ctk_css_gadget_content_box_contains_point (CtkCssGadget         *gadget,
                                                           int                   x,
                                                           int                   y);
void            ctk_css_gadget_get_margin_box           (CtkCssGadget           *gadget,
                                                         CtkAllocation          *box);
void            ctk_css_gadget_get_border_box           (CtkCssGadget           *gadget,
                                                         CtkAllocation          *box);
void            ctk_css_gadget_get_content_box          (CtkCssGadget           *gadget,
                                                         CtkAllocation          *box);

void            ctk_css_gadget_add_class                (CtkCssGadget           *gadget,
                                                         const char             *name);
void            ctk_css_gadget_remove_class             (CtkCssGadget           *gadget,
                                                         const char             *name);
void            ctk_css_gadget_set_state                (CtkCssGadget           *gadget,
                                                         CtkStateFlags           state);
void            ctk_css_gadget_add_state                (CtkCssGadget           *gadget,
                                                         CtkStateFlags           state);
void            ctk_css_gadget_remove_state             (CtkCssGadget           *gadget,
                                                         CtkStateFlags           state);

void            ctk_css_gadget_get_preferred_size       (CtkCssGadget           *gadget,
                                                         CtkOrientation          orientation,
                                                         gint                    for_size,
                                                         gint                   *minimum,
                                                         gint                   *natural,
                                                         gint                   *minimum_baseline,
                                                         gint                   *natural_baseline);
void            ctk_css_gadget_allocate                 (CtkCssGadget           *gadget,
                                                         const CtkAllocation    *allocation,
                                                         int                     baseline,
                                                         CtkAllocation          *out_clip);
void            ctk_css_gadget_draw                     (CtkCssGadget           *gadget,
                                                         cairo_t                *cr);

void            ctk_css_gadget_queue_resize             (CtkCssGadget           *gadget);
void            ctk_css_gadget_queue_allocate           (CtkCssGadget           *gadget);
void            ctk_css_gadget_queue_draw               (CtkCssGadget           *gadget);

void            ctk_css_gadget_get_margin_allocation    (CtkCssGadget           *gadget,
                                                         CtkAllocation          *allocation,
                                                         int                    *baseline);
void            ctk_css_gadget_get_border_allocation    (CtkCssGadget           *gadget,
                                                         CtkAllocation          *allocation,
                                                         int                    *baseline);
void            ctk_css_gadget_get_content_allocation   (CtkCssGadget           *gadget,
                                                         CtkAllocation          *allocation,
                                                         int                    *baseline);

G_END_DECLS

#endif /* __CTK_CSS_GADGET_PRIVATE_H__ */
