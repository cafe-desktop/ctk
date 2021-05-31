/* gtkplacesviewrow.h
 *
 * Copyright (C) 2015 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CTK_PLACES_VIEW_ROW_H
#define CTK_PLACES_VIEW_ROW_H

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include "gtkwidget.h"
#include "gtksizegroup.h"
#include "gtklistbox.h"

G_BEGIN_DECLS

#define CTK_TYPE_PLACES_VIEW_ROW (ctk_places_view_row_get_type())

G_DECLARE_FINAL_TYPE (GtkPlacesViewRow, ctk_places_view_row, GTK, PLACES_VIEW_ROW, GtkListBoxRow)

GtkWidget*         ctk_places_view_row_new                       (GVolume            *volume,
                                                                  GMount             *mount);

GtkWidget*         ctk_places_view_row_get_eject_button          (GtkPlacesViewRow   *row);

GtkWidget*         ctk_places_view_row_get_event_box             (GtkPlacesViewRow   *row);

GMount*            ctk_places_view_row_get_mount                 (GtkPlacesViewRow   *row);

GVolume*           ctk_places_view_row_get_volume                (GtkPlacesViewRow   *row);

GFile*             ctk_places_view_row_get_file                  (GtkPlacesViewRow   *row);

void               ctk_places_view_row_set_busy                  (GtkPlacesViewRow   *row,
                                                                  gboolean            is_busy);

gboolean           ctk_places_view_row_get_is_network            (GtkPlacesViewRow   *row);

void               ctk_places_view_row_set_is_network            (GtkPlacesViewRow   *row,
                                                                  gboolean            is_network);

void               ctk_places_view_row_set_path_size_group       (GtkPlacesViewRow   *row,
                                                                  GtkSizeGroup       *group);

void               ctk_places_view_row_set_space_size_group      (GtkPlacesViewRow   *row,
                                                                  GtkSizeGroup       *group);

G_END_DECLS

#endif /* CTK_PLACES_VIEW_ROW_H */
