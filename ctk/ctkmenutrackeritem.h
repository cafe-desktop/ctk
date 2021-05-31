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
                                                             CTK_TYPE_MENU_TRACKER_ITEM, CtkMenuTrackerItem))
#define CTK_IS_MENU_TRACKER_ITEM(inst)                      (G_TYPE_CHECK_INSTANCE_TYPE ((inst), \
                                                             CTK_TYPE_MENU_TRACKER_ITEM))

typedef struct _CtkMenuTrackerItem CtkMenuTrackerItem;

#define CTK_TYPE_MENU_TRACKER_ITEM_ROLE                     (ctk_menu_tracker_item_role_get_type ())

typedef enum  {
  CTK_MENU_TRACKER_ITEM_ROLE_NORMAL,
  CTK_MENU_TRACKER_ITEM_ROLE_CHECK,
  CTK_MENU_TRACKER_ITEM_ROLE_RADIO,
} CtkMenuTrackerItemRole;

GType                   ctk_menu_tracker_item_get_type                  (void) G_GNUC_CONST;

GType                   ctk_menu_tracker_item_role_get_type             (void) G_GNUC_CONST;

CtkMenuTrackerItem *   _ctk_menu_tracker_item_new                       (CtkActionObservable *observable,
                                                                         GMenuModel          *model,
                                                                         gint                 item_index,
                                                                         gboolean             mac_os_mode,
                                                                         const gchar         *action_namespace,
                                                                         gboolean             is_separator);

const gchar *           ctk_menu_tracker_item_get_special               (CtkMenuTrackerItem *self);

const gchar *           ctk_menu_tracker_item_get_display_hint          (CtkMenuTrackerItem *self);

const gchar *           ctk_menu_tracker_item_get_text_direction        (CtkMenuTrackerItem *self);

CtkActionObservable *  _ctk_menu_tracker_item_get_observable            (CtkMenuTrackerItem *self);

gboolean                ctk_menu_tracker_item_get_is_separator          (CtkMenuTrackerItem *self);

gboolean                ctk_menu_tracker_item_get_has_link              (CtkMenuTrackerItem *self,
                                                                         const gchar        *link_name);

const gchar *           ctk_menu_tracker_item_get_label                 (CtkMenuTrackerItem *self);

GIcon *                 ctk_menu_tracker_item_get_icon                  (CtkMenuTrackerItem *self);

GIcon *                 ctk_menu_tracker_item_get_verb_icon             (CtkMenuTrackerItem *self);

gboolean                ctk_menu_tracker_item_get_sensitive             (CtkMenuTrackerItem *self);

CtkMenuTrackerItemRole  ctk_menu_tracker_item_get_role                  (CtkMenuTrackerItem *self);

gboolean                ctk_menu_tracker_item_get_toggled               (CtkMenuTrackerItem *self);

const gchar *           ctk_menu_tracker_item_get_accel                 (CtkMenuTrackerItem *self);

GMenuModel *           _ctk_menu_tracker_item_get_link                  (CtkMenuTrackerItem *self,
                                                                         const gchar        *link_name);

gchar *                _ctk_menu_tracker_item_get_link_namespace        (CtkMenuTrackerItem *self);

gboolean                ctk_menu_tracker_item_may_disappear             (CtkMenuTrackerItem *self);

gboolean                ctk_menu_tracker_item_get_is_visible            (CtkMenuTrackerItem *self);

gboolean                ctk_menu_tracker_item_get_should_request_show   (CtkMenuTrackerItem *self);

void                    ctk_menu_tracker_item_activated                 (CtkMenuTrackerItem *self);

void                    ctk_menu_tracker_item_request_submenu_shown     (CtkMenuTrackerItem *self,
                                                                         gboolean            shown);

gboolean                ctk_menu_tracker_item_get_submenu_shown         (CtkMenuTrackerItem *self);

#endif
