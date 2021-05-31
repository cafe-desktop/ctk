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
 * GTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_DRAG_SOURCE_H__
#define __CTK_DRAG_SOURCE_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkselection.h>
#include <ctk/ctkwidget.h>


G_BEGIN_DECLS

GDK_AVAILABLE_IN_ALL
void ctk_drag_source_set  (CtkWidget            *widget,
			   GdkModifierType       start_button_mask,
			   const CtkTargetEntry *targets,
			   gint                  n_targets,
			   GdkDragAction         actions);

GDK_AVAILABLE_IN_ALL
void ctk_drag_source_unset (CtkWidget        *widget);

GDK_AVAILABLE_IN_ALL
CtkTargetList* ctk_drag_source_get_target_list (CtkWidget     *widget);
GDK_AVAILABLE_IN_ALL
void           ctk_drag_source_set_target_list (CtkWidget     *widget,
                                                CtkTargetList *target_list);
GDK_AVAILABLE_IN_ALL
void           ctk_drag_source_add_text_targets  (CtkWidget     *widget);
GDK_AVAILABLE_IN_ALL
void           ctk_drag_source_add_image_targets (CtkWidget    *widget);
GDK_AVAILABLE_IN_ALL
void           ctk_drag_source_add_uri_targets   (CtkWidget    *widget);

GDK_AVAILABLE_IN_ALL
void ctk_drag_source_set_icon_pixbuf  (CtkWidget       *widget,
				       GdkPixbuf       *pixbuf);
GDK_DEPRECATED_IN_3_10_FOR(ctk_drag_source_set_icon_name)
void ctk_drag_source_set_icon_stock   (CtkWidget       *widget,
				       const gchar     *stock_id);
GDK_AVAILABLE_IN_ALL
void ctk_drag_source_set_icon_name    (CtkWidget       *widget,
				       const gchar     *icon_name);
GDK_AVAILABLE_IN_3_2
void ctk_drag_source_set_icon_gicon   (CtkWidget       *widget,
				       GIcon           *icon);


G_END_DECLS

#endif /* __CTK_DRAG_SOURCE_H__ */
