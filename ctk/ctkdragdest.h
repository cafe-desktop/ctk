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

#ifndef __CTK_DRAG_DEST_H__
#define __CTK_DRAG_DEST_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkselection.h>
#include <ctk/ctkwidget.h>


G_BEGIN_DECLS

/**
 * CtkDestDefaults:
 * @CTK_DEST_DEFAULT_MOTION: If set for a widget, CTK+, during a drag over this
 *   widget will check if the drag matches this widget’s list of possible targets
 *   and actions.
 *   CTK+ will then call gdk_drag_status() as appropriate.
 * @CTK_DEST_DEFAULT_HIGHLIGHT: If set for a widget, CTK+ will draw a highlight on
 *   this widget as long as a drag is over this widget and the widget drag format
 *   and action are acceptable.
 * @CTK_DEST_DEFAULT_DROP: If set for a widget, when a drop occurs, CTK+ will
 *   will check if the drag matches this widget’s list of possible targets and
 *   actions. If so, CTK+ will call ctk_drag_get_data() on behalf of the widget.
 *   Whether or not the drop is successful, CTK+ will call ctk_drag_finish(). If
 *   the action was a move, then if the drag was successful, then %TRUE will be
 *   passed for the @delete parameter to ctk_drag_finish().
 * @CTK_DEST_DEFAULT_ALL: If set, specifies that all default actions should
 *   be taken.
 *
 * The #CtkDestDefaults enumeration specifies the various
 * types of action that will be taken on behalf
 * of the user for a drag destination site.
 */
typedef enum {
  CTK_DEST_DEFAULT_MOTION     = 1 << 0,
  CTK_DEST_DEFAULT_HIGHLIGHT  = 1 << 1,
  CTK_DEST_DEFAULT_DROP       = 1 << 2,
  CTK_DEST_DEFAULT_ALL        = 0x07
} CtkDestDefaults;

GDK_AVAILABLE_IN_ALL
void ctk_drag_dest_set   (CtkWidget            *widget,
                          CtkDestDefaults       flags,
                          const CtkTargetEntry *targets,
                          gint                  n_targets,
                          GdkDragAction         actions);

GDK_DEPRECATED_IN_3_22
void ctk_drag_dest_set_proxy (CtkWidget      *widget,
                              GdkWindow      *proxy_window,
                              GdkDragProtocol protocol,
                              gboolean        use_coordinates);

GDK_AVAILABLE_IN_ALL
void ctk_drag_dest_unset (CtkWidget          *widget);

GDK_AVAILABLE_IN_ALL
GdkAtom        ctk_drag_dest_find_target     (CtkWidget      *widget,
                                              GdkDragContext *context,
                                              CtkTargetList  *target_list);
GDK_AVAILABLE_IN_ALL
CtkTargetList* ctk_drag_dest_get_target_list (CtkWidget      *widget);
GDK_AVAILABLE_IN_ALL
void           ctk_drag_dest_set_target_list (CtkWidget      *widget,
                                              CtkTargetList  *target_list);
GDK_AVAILABLE_IN_ALL
void           ctk_drag_dest_add_text_targets  (CtkWidget    *widget);
GDK_AVAILABLE_IN_ALL
void           ctk_drag_dest_add_image_targets (CtkWidget    *widget);
GDK_AVAILABLE_IN_ALL
void           ctk_drag_dest_add_uri_targets   (CtkWidget    *widget);

GDK_AVAILABLE_IN_ALL
void           ctk_drag_dest_set_track_motion  (CtkWidget *widget,
                                                gboolean   track_motion);
GDK_AVAILABLE_IN_ALL
gboolean       ctk_drag_dest_get_track_motion  (CtkWidget *widget);


G_END_DECLS

#endif /* __CTK_DRAG_DEST_H__ */
