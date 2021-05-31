/*
 * Copyright © 2011, 2013 Canonical Limited
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the licence, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#ifndef __CTK_MENU_TRACKER_ITEM_H__
#define __CTK_MENU_TRACKER_ITEM_H__

#include "ctkactionobservable.h"

#define CTK_TYPE_MENU_TRACKER_ITEM                          (ctk_menu_tracker_item_get_type ())
#define CTK_MENU_TRACKER_ITEM(inst)                         (G_TYPE_CHECK_INSTANCE_CAST ((inst), \
                                                             CTK_TYPE_MENU_TRACKER_ITEM, GtkMenuTrackerItem))
#define CTK_IS_MENU_TRACKER_ITEM(inst)                      (G_TYPE_CHECK_INSTANCE_TYPE ((inst), \
                                                             CTK_TYPE_MENU_TRACKER_ITEM))

typedef struct _GtkMenuTrackerItem GtkMenuTrackerItem;

#define CTK_TYPE_MENU_TRACKER_ITEM_ROLE                     (ctk_menu_tracker_item_role_get_type ())

typedef enum  {
  CTK_MENU_TRACKER_ITEM_ROLE_NORMAL,
  CTK_MENU_TRACKER_ITEM_ROLE_CHECK,
  CTK_MENU_TRACKER_ITEM_ROLE_RADIO,
} GtkMenuTrackerItemRole;

GType                   ctk_menu_tracker_item_get_type                  (void) G_GNUC_CONST;

GType                   ctk_menu_tracker_item_role_get_type             (void) G_GNUC_CONST;

GtkMenuTrackerItem *   _ctk_menu_tracker_item_new                       (GtkActionObservable *observable,
                                                                         GMenuModel          *model,
                                                                         gint                 item_index,
                                                                         gboolean             mac_os_mode,
                                                                         const gchar         *action_namespace,
                                                                         gboolean             is_separator);

const gchar *           ctk_menu_tracker_item_get_special               (GtkMenuTrackerItem *self);

const gchar *           ctk_menu_tracker_item_get_display_hint          (GtkMenuTrackerItem *self);

const gchar *           ctk_menu_tracker_item_get_text_direction        (GtkMenuTrackerItem *self);

GtkActionObservable *  _ctk_menu_tracker_item_get_observable            (GtkMenuTrackerItem *self);

gboolean                ctk_menu_tracker_item_get_is_separator          (GtkMenuTrackerItem *self);

gboolean                ctk_menu_tracker_item_get_has_link              (GtkMenuTrackerItem *self,
                                                                         const gchar        *link_name);

const gchar *           ctk_menu_tracker_item_get_label                 (GtkMenuTrackerItem *self);

GIcon *                 ctk_menu_tracker_item_get_icon                  (GtkMenuTrackerItem *self);

GIcon *                 ctk_menu_tracker_item_get_verb_icon             (GtkMenuTrackerItem *self);

gboolean                ctk_menu_tracker_item_get_sensitive             (GtkMenuTrackerItem *self);

GtkMenuTrackerItemRole  ctk_menu_tracker_item_get_role                  (GtkMenuTrackerItem *self);

gboolean                ctk_menu_tracker_item_get_toggled               (GtkMenuTrackerItem *self);

const gchar *           ctk_menu_tracker_item_get_accel                 (GtkMenuTrackerItem *self);

GMenuModel *           _ctk_menu_tracker_item_get_link                  (GtkMenuTrackerItem *self,
                                                                         const gchar        *link_name);

gchar *                _ctk_menu_tracker_item_get_link_namespace        (GtkMenuTrackerItem *self);

gboolean                ctk_menu_tracker_item_may_disappear             (GtkMenuTrackerItem *self);

gboolean                ctk_menu_tracker_item_get_is_visible            (GtkMenuTrackerItem *self);

gboolean                ctk_menu_tracker_item_get_should_request_show   (GtkMenuTrackerItem *self);

void                    ctk_menu_tracker_item_activated                 (GtkMenuTrackerItem *self);

void                    ctk_menu_tracker_item_request_submenu_shown     (GtkMenuTrackerItem *self,
                                                                         gboolean            shown);

gboolean                ctk_menu_tracker_item_get_submenu_shown         (GtkMenuTrackerItem *self);

#endif
