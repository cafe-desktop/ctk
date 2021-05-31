/* gtkcellareabox.h
 *
 * Copyright (C) 2010 Openismus GmbH
 *
 * Authors:
 *      Tristan Van Berkom <tristanvb@openismus.com>
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

#ifndef __CTK_CELL_AREA_BOX_H__
#define __CTK_CELL_AREA_BOX_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gtk/gtkcellarea.h>

G_BEGIN_DECLS

#define CTK_TYPE_CELL_AREA_BOX            (ctk_cell_area_box_get_type ())
#define CTK_CELL_AREA_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_CELL_AREA_BOX, GtkCellAreaBox))
#define CTK_CELL_AREA_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_CELL_AREA_BOX, GtkCellAreaBoxClass))
#define CTK_IS_CELL_AREA_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_CELL_AREA_BOX))
#define CTK_IS_CELL_AREA_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_CELL_AREA_BOX))
#define CTK_CELL_AREA_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CELL_AREA_BOX, GtkCellAreaBoxClass))

typedef struct _GtkCellAreaBox              GtkCellAreaBox;
typedef struct _GtkCellAreaBoxClass         GtkCellAreaBoxClass;
typedef struct _GtkCellAreaBoxPrivate       GtkCellAreaBoxPrivate;

struct _GtkCellAreaBox
{
  /*< private >*/
  GtkCellArea parent_instance;

  GtkCellAreaBoxPrivate *priv;
};

/**
 * GtkCellAreaBoxClass:
 */
struct _GtkCellAreaBoxClass
{
  /*< private >*/
  GtkCellAreaClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
GType        ctk_cell_area_box_get_type    (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
GtkCellArea *ctk_cell_area_box_new         (void);
GDK_AVAILABLE_IN_ALL
void         ctk_cell_area_box_pack_start  (GtkCellAreaBox  *box,
                                            GtkCellRenderer *renderer,
                                            gboolean         expand,
                                            gboolean         align,
                                            gboolean         fixed);
GDK_AVAILABLE_IN_ALL
void         ctk_cell_area_box_pack_end    (GtkCellAreaBox  *box,
                                            GtkCellRenderer *renderer,
                                            gboolean         expand,
                                            gboolean         align,
                                            gboolean         fixed);
GDK_AVAILABLE_IN_ALL
gint         ctk_cell_area_box_get_spacing (GtkCellAreaBox  *box);
GDK_AVAILABLE_IN_ALL
void         ctk_cell_area_box_set_spacing (GtkCellAreaBox  *box,
                                            gint             spacing);

/* Private interaction with GtkCellAreaBoxContext */
gboolean    _ctk_cell_area_box_group_visible (GtkCellAreaBox  *box,
                                              gint             group_idx);

G_END_DECLS

#endif /* __CTK_CELL_AREA_BOX_H__ */
