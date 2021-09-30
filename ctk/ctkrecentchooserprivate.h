/* ctkrecentprivatechooser.h - Interface definitions for recent selectors UI
 *
 * Copyright (C) 2006 Emmanuele Bassi
 *
 * All rights reserved
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CTK_RECENT_CHOOSER_PRIVATE_H__
#define __CTK_RECENT_CHOOSER_PRIVATE_H__

#include "ctkrecentmanager.h"
#include "ctkrecentchooser.h"
#include "ctkactivatable.h"

G_BEGIN_DECLS

CtkRecentManager *_ctk_recent_chooser_get_recent_manager     (CtkRecentChooser  *chooser);
GList *           _ctk_recent_chooser_get_items              (CtkRecentChooser  *chooser,
							      CtkRecentFilter   *filter,
							      CtkRecentSortFunc  func,
							      gpointer           data);

void              _ctk_recent_chooser_item_activated         (CtkRecentChooser  *chooser);
void              _ctk_recent_chooser_selection_changed      (CtkRecentChooser  *chooser);

void              _ctk_recent_chooser_update                 (CtkActivatable       *activatable,
							      CtkAction            *action,
							      const gchar          *property_name);
void              _ctk_recent_chooser_sync_action_properties (CtkActivatable       *activatable,
							      CtkAction            *action);
void              _ctk_recent_chooser_set_related_action     (CtkRecentChooser     *recent_chooser, 
							      CtkAction            *action);
CtkAction        *_ctk_recent_chooser_get_related_action     (CtkRecentChooser     *recent_chooser);
void              _ctk_recent_chooser_set_use_action_appearance (CtkRecentChooser  *recent_chooser, 
								 gboolean           use_appearance);
gboolean          _ctk_recent_chooser_get_use_action_appearance (CtkRecentChooser  *recent_chooser);

G_END_DECLS

#endif /* ! __CTK_RECENT_CHOOSER_PRIVATE_H__ */
