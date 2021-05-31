/* GTK - The GIMP Toolkit
 * Copyright (C) 2010 Red Hat, Inc.
 * Author: Matthias Clasen
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

#ifndef __CTK_GRID_H__
#define __CTK_GRID_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gtk/gtkcontainer.h>


G_BEGIN_DECLS

#define CTK_TYPE_GRID                   (ctk_grid_get_type ())
#define CTK_GRID(obj)                   (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_GRID, GtkGrid))
#define CTK_GRID_CLASS(klass)           (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_GRID, GtkGridClass))
#define CTK_IS_GRID(obj)                (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_GRID))
#define CTK_IS_GRID_CLASS(klass)        (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_GRID))
#define CTK_GRID_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_GRID, GtkGridClass))


typedef struct _GtkGrid              GtkGrid;
typedef struct _GtkGridPrivate       GtkGridPrivate;
typedef struct _GtkGridClass         GtkGridClass;

struct _GtkGrid
{
  /*< private >*/
  GtkContainer container;

  GtkGridPrivate *priv;
};

/**
 * GtkGridClass:
 * @parent_class: The parent class.
 */
struct _GtkGridClass
{
  GtkContainerClass parent_class;

  /*< private >*/

  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
  void (*_ctk_reserved5) (void);
  void (*_ctk_reserved6) (void);
  void (*_ctk_reserved7) (void);
  void (*_ctk_reserved8) (void);
};

GDK_AVAILABLE_IN_ALL
GType      ctk_grid_get_type               (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
GtkWidget* ctk_grid_new                    (void);
GDK_AVAILABLE_IN_ALL
void       ctk_grid_attach                 (GtkGrid         *grid,
                                            GtkWidget       *child,
                                            gint             left,
                                            gint             top,
                                            gint             width,
                                            gint             height);
GDK_AVAILABLE_IN_ALL
void       ctk_grid_attach_next_to         (GtkGrid         *grid,
                                            GtkWidget       *child,
                                            GtkWidget       *sibling,
                                            GtkPositionType  side,
                                            gint             width,
                                            gint             height);
GDK_AVAILABLE_IN_3_2
GtkWidget *ctk_grid_get_child_at           (GtkGrid         *grid,
                                            gint             left,
                                            gint             top);
GDK_AVAILABLE_IN_3_2
void       ctk_grid_insert_row             (GtkGrid         *grid,
                                            gint             position);
GDK_AVAILABLE_IN_3_2
void       ctk_grid_insert_column          (GtkGrid         *grid,
                                            gint             position);
GDK_AVAILABLE_IN_3_10
void       ctk_grid_remove_row             (GtkGrid         *grid,
                                            gint             position);
GDK_AVAILABLE_IN_3_10
void       ctk_grid_remove_column          (GtkGrid         *grid,
                                            gint             position);
GDK_AVAILABLE_IN_3_2
void       ctk_grid_insert_next_to         (GtkGrid         *grid,
                                            GtkWidget       *sibling,
                                            GtkPositionType  side);
GDK_AVAILABLE_IN_ALL
void       ctk_grid_set_row_homogeneous    (GtkGrid         *grid,
                                            gboolean         homogeneous);
GDK_AVAILABLE_IN_ALL
gboolean   ctk_grid_get_row_homogeneous    (GtkGrid         *grid);
GDK_AVAILABLE_IN_ALL
void       ctk_grid_set_row_spacing        (GtkGrid         *grid,
                                            guint            spacing);
GDK_AVAILABLE_IN_ALL
guint      ctk_grid_get_row_spacing        (GtkGrid         *grid);
GDK_AVAILABLE_IN_ALL
void       ctk_grid_set_column_homogeneous (GtkGrid         *grid,
                                            gboolean         homogeneous);
GDK_AVAILABLE_IN_ALL
gboolean   ctk_grid_get_column_homogeneous (GtkGrid         *grid);
GDK_AVAILABLE_IN_ALL
void       ctk_grid_set_column_spacing     (GtkGrid         *grid,
                                            guint            spacing);
GDK_AVAILABLE_IN_ALL
guint      ctk_grid_get_column_spacing     (GtkGrid         *grid);
GDK_AVAILABLE_IN_3_10
void       ctk_grid_set_row_baseline_position (GtkGrid      *grid,
					       gint          row,
					       GtkBaselinePosition pos);
GDK_AVAILABLE_IN_3_10
GtkBaselinePosition ctk_grid_get_row_baseline_position (GtkGrid      *grid,
							gint          row);
GDK_AVAILABLE_IN_3_10
void       ctk_grid_set_baseline_row       (GtkGrid         *grid,
					    gint             row);
GDK_AVAILABLE_IN_3_10
gint       ctk_grid_get_baseline_row       (GtkGrid         *grid);


G_END_DECLS

#endif /* __CTK_GRID_H__ */