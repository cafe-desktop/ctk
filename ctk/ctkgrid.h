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
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkcontainer.h>


G_BEGIN_DECLS

#define CTK_TYPE_GRID                   (ctk_grid_get_type ())
#define CTK_GRID(obj)                   (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_GRID, CtkGrid))
#define CTK_GRID_CLASS(klass)           (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_GRID, CtkGridClass))
#define CTK_IS_GRID(obj)                (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_GRID))
#define CTK_IS_GRID_CLASS(klass)        (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_GRID))
#define CTK_GRID_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_GRID, CtkGridClass))


typedef struct _CtkGrid              CtkGrid;
typedef struct _CtkGridPrivate       CtkGridPrivate;
typedef struct _CtkGridClass         CtkGridClass;

struct _CtkGrid
{
  /*< private >*/
  CtkContainer container;

  CtkGridPrivate *priv;
};

/**
 * CtkGridClass:
 * @parent_class: The parent class.
 */
struct _CtkGridClass
{
  CtkContainerClass parent_class;

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
CtkWidget* ctk_grid_new                    (void);
GDK_AVAILABLE_IN_ALL
void       ctk_grid_attach                 (CtkGrid         *grid,
                                            CtkWidget       *child,
                                            gint             left,
                                            gint             top,
                                            gint             width,
                                            gint             height);
GDK_AVAILABLE_IN_ALL
void       ctk_grid_attach_next_to         (CtkGrid         *grid,
                                            CtkWidget       *child,
                                            CtkWidget       *sibling,
                                            CtkPositionType  side,
                                            gint             width,
                                            gint             height);
GDK_AVAILABLE_IN_3_2
CtkWidget *ctk_grid_get_child_at           (CtkGrid         *grid,
                                            gint             left,
                                            gint             top);
GDK_AVAILABLE_IN_3_2
void       ctk_grid_insert_row             (CtkGrid         *grid,
                                            gint             position);
GDK_AVAILABLE_IN_3_2
void       ctk_grid_insert_column          (CtkGrid         *grid,
                                            gint             position);
GDK_AVAILABLE_IN_3_10
void       ctk_grid_remove_row             (CtkGrid         *grid,
                                            gint             position);
GDK_AVAILABLE_IN_3_10
void       ctk_grid_remove_column          (CtkGrid         *grid,
                                            gint             position);
GDK_AVAILABLE_IN_3_2
void       ctk_grid_insert_next_to         (CtkGrid         *grid,
                                            CtkWidget       *sibling,
                                            CtkPositionType  side);
GDK_AVAILABLE_IN_ALL
void       ctk_grid_set_row_homogeneous    (CtkGrid         *grid,
                                            gboolean         homogeneous);
GDK_AVAILABLE_IN_ALL
gboolean   ctk_grid_get_row_homogeneous    (CtkGrid         *grid);
GDK_AVAILABLE_IN_ALL
void       ctk_grid_set_row_spacing        (CtkGrid         *grid,
                                            guint            spacing);
GDK_AVAILABLE_IN_ALL
guint      ctk_grid_get_row_spacing        (CtkGrid         *grid);
GDK_AVAILABLE_IN_ALL
void       ctk_grid_set_column_homogeneous (CtkGrid         *grid,
                                            gboolean         homogeneous);
GDK_AVAILABLE_IN_ALL
gboolean   ctk_grid_get_column_homogeneous (CtkGrid         *grid);
GDK_AVAILABLE_IN_ALL
void       ctk_grid_set_column_spacing     (CtkGrid         *grid,
                                            guint            spacing);
GDK_AVAILABLE_IN_ALL
guint      ctk_grid_get_column_spacing     (CtkGrid         *grid);
GDK_AVAILABLE_IN_3_10
void       ctk_grid_set_row_baseline_position (CtkGrid      *grid,
					       gint          row,
					       CtkBaselinePosition pos);
GDK_AVAILABLE_IN_3_10
CtkBaselinePosition ctk_grid_get_row_baseline_position (CtkGrid      *grid,
							gint          row);
GDK_AVAILABLE_IN_3_10
void       ctk_grid_set_baseline_row       (CtkGrid         *grid,
					    gint             row);
GDK_AVAILABLE_IN_3_10
gint       ctk_grid_get_baseline_row       (CtkGrid         *grid);


G_END_DECLS

#endif /* __CTK_GRID_H__ */
