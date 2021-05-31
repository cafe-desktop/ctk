/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __CTK_DND_H__
#define __CTK_DND_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gtk/gtkwidget.h>
#include <gtk/gtkselection.h>


G_BEGIN_DECLS

/* Destination side */

GDK_AVAILABLE_IN_ALL
void ctk_drag_get_data (GtkWidget      *widget,
			GdkDragContext *context,
			GdkAtom         target,
			guint32         time_);
GDK_AVAILABLE_IN_ALL
void ctk_drag_finish   (GdkDragContext *context,
			gboolean        success,
			gboolean        del,
			guint32         time_);

GDK_AVAILABLE_IN_ALL
GtkWidget *ctk_drag_get_source_widget (GdkDragContext *context);

GDK_AVAILABLE_IN_ALL
void ctk_drag_highlight   (GtkWidget  *widget);
GDK_AVAILABLE_IN_ALL
void ctk_drag_unhighlight (GtkWidget  *widget);

/* Source side */

GDK_AVAILABLE_IN_3_10
GdkDragContext *ctk_drag_begin_with_coordinates (GtkWidget         *widget,
                                                 GtkTargetList     *targets,
                                                 GdkDragAction      actions,
                                                 gint               button,
                                                 GdkEvent          *event,
                                                 gint               x,
                                                 gint               y);

GDK_DEPRECATED_IN_3_10_FOR(ctk_drag_begin_with_coordinates)
GdkDragContext *ctk_drag_begin (GtkWidget         *widget,
				GtkTargetList     *targets,
				GdkDragAction      actions,
				gint               button,
				GdkEvent          *event);

GDK_AVAILABLE_IN_3_16
void ctk_drag_cancel           (GdkDragContext *context);

GDK_AVAILABLE_IN_ALL
void ctk_drag_set_icon_widget (GdkDragContext *context,
			       GtkWidget      *widget,
			       gint            hot_x,
			       gint            hot_y);
GDK_AVAILABLE_IN_ALL
void ctk_drag_set_icon_pixbuf (GdkDragContext *context,
			       GdkPixbuf      *pixbuf,
			       gint            hot_x,
			       gint            hot_y);
GDK_DEPRECATED_IN_3_10_FOR(ctk_drag_set_icon_name)
void ctk_drag_set_icon_stock  (GdkDragContext *context,
			       const gchar    *stock_id,
			       gint            hot_x,
			       gint            hot_y);
GDK_AVAILABLE_IN_ALL
void ctk_drag_set_icon_surface(GdkDragContext *context,
			       cairo_surface_t *surface);
GDK_AVAILABLE_IN_ALL
void ctk_drag_set_icon_name   (GdkDragContext *context,
			       const gchar    *icon_name,
			       gint            hot_x,
			       gint            hot_y);
GDK_AVAILABLE_IN_3_2
void ctk_drag_set_icon_gicon  (GdkDragContext *context,
			       GIcon          *icon,
			       gint            hot_x,
			       gint            hot_y);

GDK_AVAILABLE_IN_ALL
void ctk_drag_set_icon_default (GdkDragContext    *context);

GDK_AVAILABLE_IN_ALL
gboolean ctk_drag_check_threshold (GtkWidget *widget,
				   gint       start_x,
				   gint       start_y,
				   gint       current_x,
				   gint       current_y);


G_END_DECLS

#endif /* __CTK_DND_H__ */
