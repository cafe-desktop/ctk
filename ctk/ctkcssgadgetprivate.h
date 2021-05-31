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

#include "gtk/gtkwidget.h"
#include "gtk/gtkcssstylechangeprivate.h"
#include "gtk/gtkcsstypesprivate.h"

G_BEGIN_DECLS

#define CTK_TYPE_CSS_GADGET           (ctk_css_gadget_get_type ())
#define CTK_CSS_GADGET(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, CTK_TYPE_CSS_GADGET, GtkCssGadget))
#define CTK_CSS_GADGET_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, CTK_TYPE_CSS_GADGET, GtkCssGadgetClass))
#define CTK_IS_CSS_GADGET(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, CTK_TYPE_CSS_GADGET))
#define CTK_IS_CSS_GADGET_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, CTK_TYPE_CSS_GADGET))
#define CTK_CSS_GADGET_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CSS_GADGET, GtkCssGadgetClass))

typedef struct _GtkCssGadget           GtkCssGadget;
typedef struct _GtkCssGadgetClass      GtkCssGadgetClass;

struct _GtkCssGadget
{
  GObject parent;
};

struct _GtkCssGadgetClass
{
  GObjectClass  parent_class;

  void          (* get_preferred_size)                  (GtkCssGadget           *gadget,
                                                         GtkOrientation          orientation,
                                                         gint                    for_size,
                                                         gint                   *minimum,
                                                         gint                   *natural,
                                                         gint                   *minimum_baseline,
                                                         gint                   *natural_baseline);

  void          (* allocate)                            (GtkCssGadget           *gadget,
                                                         const GtkAllocation    *allocation,
                                                         int                     baseline,
                                                         GtkAllocation          *out_clip);

  gboolean      (* draw)                                (GtkCssGadget           *gadget,
                                                         cairo_t                *cr,
                                                         int                     x,
                                                         int                     y,
                                                         int                     width,
                                                         int                     height);

  void          (* style_changed)                       (GtkCssGadget           *gadget,
                                                         GtkCssStyleChange      *change);
};

GType           ctk_css_gadget_get_type                 (void) G_GNUC_CONST;

GtkCssNode *    ctk_css_gadget_get_node                 (GtkCssGadget           *gadget);
GtkCssStyle *   ctk_css_gadget_get_style                (GtkCssGadget           *gadget);
GtkWidget *     ctk_css_gadget_get_owner                (GtkCssGadget           *gadget);

void            ctk_css_gadget_set_node                 (GtkCssGadget           *gadget,
                                                         GtkCssNode             *node);
void            ctk_css_gadget_set_visible              (GtkCssGadget           *gadget,
                                                         gboolean                visible);
gboolean        ctk_css_gadget_get_visible              (GtkCssGadget           *gadget);

gboolean        ctk_css_gadget_margin_box_contains_point (GtkCssGadget          *gadget,
                                                          int                    x,
                                                          int                    y);
gboolean        ctk_css_gadget_border_box_contains_point (GtkCssGadget          *gadget,
                                                          int                    x,
                                                          int                    y);
gboolean        ctk_css_gadget_content_box_contains_point (GtkCssGadget         *gadget,
                                                           int                   x,
                                                           int                   y);
void            ctk_css_gadget_get_margin_box           (GtkCssGadget           *gadget,
                                                         GtkAllocation          *box);
void            ctk_css_gadget_get_border_box           (GtkCssGadget           *gadget,
                                                         GtkAllocation          *box);
void            ctk_css_gadget_get_content_box          (GtkCssGadget           *gadget,
                                                         GtkAllocation          *box);

void            ctk_css_gadget_add_class                (GtkCssGadget           *gadget,
                                                         const char             *name);
void            ctk_css_gadget_remove_class             (GtkCssGadget           *gadget,
                                                         const char             *name);
void            ctk_css_gadget_set_state                (GtkCssGadget           *gadget,
                                                         GtkStateFlags           state);
void            ctk_css_gadget_add_state                (GtkCssGadget           *gadget,
                                                         GtkStateFlags           state);
void            ctk_css_gadget_remove_state             (GtkCssGadget           *gadget,
                                                         GtkStateFlags           state);

void            ctk_css_gadget_get_preferred_size       (GtkCssGadget           *gadget,
                                                         GtkOrientation          orientation,
                                                         gint                    for_size,
                                                         gint                   *minimum,
                                                         gint                   *natural,
                                                         gint                   *minimum_baseline,
                                                         gint                   *natural_baseline);
void            ctk_css_gadget_allocate                 (GtkCssGadget           *gadget,
                                                         const GtkAllocation    *allocation,
                                                         int                     baseline,
                                                         GtkAllocation          *out_clip);
void            ctk_css_gadget_draw                     (GtkCssGadget           *gadget,
                                                         cairo_t                *cr);

void            ctk_css_gadget_queue_resize             (GtkCssGadget           *gadget);
void            ctk_css_gadget_queue_allocate           (GtkCssGadget           *gadget);
void            ctk_css_gadget_queue_draw               (GtkCssGadget           *gadget);

void            ctk_css_gadget_get_margin_allocation    (GtkCssGadget           *gadget,
                                                         GtkAllocation          *allocation,
                                                         int                    *baseline);
void            ctk_css_gadget_get_border_allocation    (GtkCssGadget           *gadget,
                                                         GtkAllocation          *allocation,
                                                         int                    *baseline);
void            ctk_css_gadget_get_content_allocation   (GtkCssGadget           *gadget,
                                                         GtkAllocation          *allocation,
                                                         int                    *baseline);

G_END_DECLS

#endif /* __CTK_CSS_GADGET_PRIVATE_H__ */
