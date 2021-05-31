/*
 * Copyright © 2013 Canonical Limited
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#ifndef __CTK_MENU_TRACKER_H__
#define __CTK_MENU_TRACKER_H__

#include "ctkmenutrackeritem.h"

typedef struct _CtkMenuTracker CtkMenuTracker;

typedef void         (* CtkMenuTrackerInsertFunc)                       (CtkMenuTrackerItem       *item,
                                                                         gint                      position,
                                                                         gpointer                  user_data);

typedef void         (* CtkMenuTrackerRemoveFunc)                       (gint                      position,
                                                                         gpointer                  user_data);


CtkMenuTracker *        ctk_menu_tracker_new                            (CtkActionObservable      *observer,
                                                                         GMenuModel               *model,
                                                                         gboolean                  with_separators,
                                                                         gboolean                  merge_sections,
                                                                         gboolean                  mac_os_mode,
                                                                         const gchar              *action_namespace,
                                                                         CtkMenuTrackerInsertFunc  insert_func,
                                                                         CtkMenuTrackerRemoveFunc  remove_func,
                                                                         gpointer                  user_data);

CtkMenuTracker *        ctk_menu_tracker_new_for_item_link              (CtkMenuTrackerItem       *item,
                                                                         const gchar              *link_name,
                                                                         gboolean                  merge_sections,
                                                                         gboolean                  mac_os_mode,
                                                                         CtkMenuTrackerInsertFunc  insert_func,
                                                                         CtkMenuTrackerRemoveFunc  remove_func,
                                                                         gpointer                  user_data);

void                    ctk_menu_tracker_free                           (CtkMenuTracker           *tracker);

#endif /* __CTK_MENU_TRACKER_H__ */
