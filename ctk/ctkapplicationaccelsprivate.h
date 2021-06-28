/*
 * Copyright © 2013 Canonical Limited
 * Copyright © 2016 Sébastien Wilmet
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
 * Authors: Ryan Lortie <desrt@desrt.ca>
 *          Sébastien Wilmet <swilmet@gnome.org>
 */

#ifndef __CTK_APPLICATION_ACCELS_H__
#define __CTK_APPLICATION_ACCELS_H__

#include <gio/gio.h>
#include "ctkwindowprivate.h"

G_BEGIN_DECLS

#define CTK_TYPE_APPLICATION_ACCELS (ctk_application_accels_get_type ())
G_DECLARE_FINAL_TYPE (CtkApplicationAccels, ctk_application_accels,
                      CTK, APPLICATION_ACCELS,
                      GObject)

CtkApplicationAccels *
                ctk_application_accels_new                          (void);

void            ctk_application_accels_set_accels_for_action        (CtkApplicationAccels *accels,
                                                                     const gchar          *detailed_action_name,
                                                                     const gchar * const  *accelerators);

gchar **        ctk_application_accels_get_accels_for_action        (CtkApplicationAccels *accels,
                                                                     const gchar          *detailed_action_name);

gchar **        ctk_application_accels_get_actions_for_accel        (CtkApplicationAccels *accels,
                                                                     const gchar          *accel);

gchar **        ctk_application_accels_list_action_descriptions     (CtkApplicationAccels *accels);

void            ctk_application_accels_foreach_key                  (CtkApplicationAccels     *accels,
                                                                     CtkWindow                *window,
                                                                     CtkWindowKeysForeachFunc  callback,
                                                                     gpointer                  user_data);

gboolean        ctk_application_accels_activate                     (CtkApplicationAccels *accels,
                                                                     GActionGroup         *action_group,
                                                                     guint                 key,
                                                                     CdkModifierType       modifier);

G_END_DECLS

#endif /* __CTK_APPLICATION_ACCELS_H__ */
