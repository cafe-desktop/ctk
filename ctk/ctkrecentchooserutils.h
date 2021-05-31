/* ctkrecentchooserutils.h - Private utility functions for implementing a
 *                           GtkRecentChooser interface
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
 *
 * Based on ctkfilechooserutils.h:
 *	Copyright (C) 2003 Red Hat, Inc.
 */
 
#ifndef __CTK_RECENT_CHOOSER_UTILS_H__
#define __CTK_RECENT_CHOOSER_UTILS_H__

#include "ctkrecentchooserprivate.h"

G_BEGIN_DECLS


#define CTK_RECENT_CHOOSER_DELEGATE_QUARK	(_ctk_recent_chooser_delegate_get_quark ())

typedef enum {
  CTK_RECENT_CHOOSER_PROP_FIRST           = 0x3000,
  CTK_RECENT_CHOOSER_PROP_RECENT_MANAGER,
  CTK_RECENT_CHOOSER_PROP_SHOW_PRIVATE,
  CTK_RECENT_CHOOSER_PROP_SHOW_NOT_FOUND,
  CTK_RECENT_CHOOSER_PROP_SHOW_TIPS,
  CTK_RECENT_CHOOSER_PROP_SHOW_ICONS,
  CTK_RECENT_CHOOSER_PROP_SELECT_MULTIPLE,
  CTK_RECENT_CHOOSER_PROP_LIMIT,
  CTK_RECENT_CHOOSER_PROP_LOCAL_ONLY,
  CTK_RECENT_CHOOSER_PROP_SORT_TYPE,
  CTK_RECENT_CHOOSER_PROP_FILTER,
  CTK_RECENT_CHOOSER_PROP_LAST
} GtkRecentChooserProp;

void   _ctk_recent_chooser_install_properties  (GObjectClass          *klass);

void   _ctk_recent_chooser_delegate_iface_init (GtkRecentChooserIface *iface);
void   _ctk_recent_chooser_set_delegate        (GtkRecentChooser      *receiver,
						GtkRecentChooser      *delegate);

GQuark _ctk_recent_chooser_delegate_get_quark  (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __CTK_RECENT_CHOOSER_UTILS_H__ */
