/*
 * Copyright © 2012 Canonical Limited
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * licence or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Ryan Lortie <desrt@desrt.ca>
 */

#ifndef __CTK_ACTION_HELPER_H__
#define __CTK_ACTION_HELPER_H__

#include <ctk/ctkapplication.h>
#include <ctk/ctkactionable.h>

#define CTK_TYPE_ACTION_HELPER                              (ctk_action_helper_get_type ())
#define CTK_ACTION_HELPER(inst)                             (G_TYPE_CHECK_INSTANCE_CAST ((inst),                      \
                                                             CTK_TYPE_ACTION_HELPER, CtkActionHelper))
#define CTK_IS_ACTION_HELPER(inst)                          (G_TYPE_CHECK_INSTANCE_TYPE ((inst),                      \
                                                             CTK_TYPE_ACTION_HELPER))

typedef struct _CtkActionHelper                             CtkActionHelper;

G_GNUC_INTERNAL
GType                   ctk_action_helper_get_type                      (void);

G_GNUC_INTERNAL
CtkActionHelper *       ctk_action_helper_new                           (CtkActionable   *widget);

G_GNUC_INTERNAL
void                    ctk_action_helper_set_action_name               (CtkActionHelper *helper,
                                                                         const gchar     *action_name);
G_GNUC_INTERNAL
void                    ctk_action_helper_set_action_target_value       (CtkActionHelper *helper,
                                                                         GVariant        *action_target);
G_GNUC_INTERNAL
const gchar *           ctk_action_helper_get_action_name               (CtkActionHelper *helper);
G_GNUC_INTERNAL
GVariant *              ctk_action_helper_get_action_target_value       (CtkActionHelper *helper);

G_GNUC_INTERNAL
gboolean                ctk_action_helper_get_enabled                   (CtkActionHelper *helper);
G_GNUC_INTERNAL
gboolean                ctk_action_helper_get_active                    (CtkActionHelper *helper);

G_GNUC_INTERNAL
void                    ctk_action_helper_activate                      (CtkActionHelper *helper);

#endif /* __CTK_ACTION_HELPER_H__ */
