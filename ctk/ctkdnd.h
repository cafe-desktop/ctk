/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* CTK - The GIMP Toolkit
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
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_DND_H__
#define __CTK_DND_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkwidget.h>
#include <ctk/ctkselection.h>


G_BEGIN_DECLS

/* Destination side */

CDK_AVAILABLE_IN_ALL
void ctk_drag_get_data (CtkWidget      *widget,
			CdkDragContext *context,
			CdkAtom         target,
			guint32         time_);
CDK_AVAILABLE_IN_ALL
void ctk_drag_finish   (CdkDragContext *context,
			gboolean        success,
			gboolean        del,
			guint32         time_);

CDK_AVAILABLE_IN_ALL
CtkWidget *ctk_drag_get_source_widget (CdkDragContext *context);

CDK_AVAILABLE_IN_ALL
void ctk_drag_highlight   (CtkWidget  *widget);
CDK_AVAILABLE_IN_ALL
void ctk_drag_unhighlight (CtkWidget  *widget);

/* Source side */

CDK_AVAILABLE_IN_3_10
CdkDragContext *ctk_drag_begin_with_coordinates (CtkWidget         *widget,
                                                 CtkTargetList     *targets,
                                                 CdkDragAction      actions,
                                                 gint               button,
                                                 CdkEvent          *event,
                                                 gint               x,
                                                 gint               y);

CDK_DEPRECATED_IN_3_10_FOR(ctk_drag_begin_with_coordinates)
CdkDragContext *ctk_drag_begin (CtkWidget         *widget,
				CtkTargetList     *targets,
				CdkDragAction      actions,
				gint               button,
				CdkEvent          *event);

CDK_AVAILABLE_IN_3_16
void ctk_drag_cancel           (CdkDragContext *context);

CDK_AVAILABLE_IN_ALL
void ctk_drag_set_icon_widget (CdkDragContext *context,
			       CtkWidget      *widget,
			       gint            hot_x,
			       gint            hot_y);
CDK_AVAILABLE_IN_ALL
void ctk_drag_set_icon_pixbuf (CdkDragContext *context,
			       CdkPixbuf      *pixbuf,
			       gint            hot_x,
			       gint            hot_y);
CDK_DEPRECATED_IN_3_10_FOR(ctk_drag_set_icon_name)
void ctk_drag_set_icon_stock  (CdkDragContext *context,
			       const gchar    *stock_id,
			       gint            hot_x,
			       gint            hot_y);
CDK_AVAILABLE_IN_ALL
void ctk_drag_set_icon_surface(CdkDragContext *context,
			       cairo_surface_t *surface);
CDK_AVAILABLE_IN_ALL
void ctk_drag_set_icon_name   (CdkDragContext *context,
			       const gchar    *icon_name,
			       gint            hot_x,
			       gint            hot_y);
CDK_AVAILABLE_IN_3_2
void ctk_drag_set_icon_gicon  (CdkDragContext *context,
			       GIcon          *icon,
			       gint            hot_x,
			       gint            hot_y);

CDK_AVAILABLE_IN_ALL
void ctk_drag_set_icon_default (CdkDragContext    *context);

CDK_AVAILABLE_IN_ALL
gboolean ctk_drag_check_threshold (CtkWidget *widget,
				   gint       start_x,
				   gint       start_y,
				   gint       current_x,
				   gint       current_y);


G_END_DECLS

#endif /* __CTK_DND_H__ */
