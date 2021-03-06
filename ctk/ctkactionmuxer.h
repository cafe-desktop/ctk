/*
 * Copyright © 2011 Canonical Limited
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

#ifndef __CTK_ACTION_MUXER_H__
#define __CTK_ACTION_MUXER_H__

#include <gio/gio.h>

G_BEGIN_DECLS

#define CTK_TYPE_ACTION_MUXER                               (ctk_action_muxer_get_type ())
#define CTK_ACTION_MUXER(inst)                              (G_TYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             CTK_TYPE_ACTION_MUXER, CtkActionMuxer))
#define CTK_IS_ACTION_MUXER(inst)                           (G_TYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             CTK_TYPE_ACTION_MUXER))

typedef struct _CtkActionMuxer                              CtkActionMuxer;

GType                   ctk_action_muxer_get_type                       (void);
CtkActionMuxer *        ctk_action_muxer_new                            (void);

void                    ctk_action_muxer_insert                         (CtkActionMuxer *muxer,
                                                                         const gchar    *prefix,
                                                                         GActionGroup   *action_group);

void                    ctk_action_muxer_remove                         (CtkActionMuxer *muxer,
                                                                         const gchar    *prefix);
const gchar **          ctk_action_muxer_list_prefixes                  (CtkActionMuxer *muxer);
GActionGroup *          ctk_action_muxer_lookup                         (CtkActionMuxer *muxer,
                                                                         const gchar    *prefix);
CtkActionMuxer *        ctk_action_muxer_get_parent                     (CtkActionMuxer *muxer);

void                    ctk_action_muxer_set_parent                     (CtkActionMuxer *muxer,
                                                                         CtkActionMuxer *parent);

void                    ctk_action_muxer_set_primary_accel              (CtkActionMuxer *muxer,
                                                                         const gchar    *action_and_target,
                                                                         const gchar    *primary_accel);

const gchar *           ctk_action_muxer_get_primary_accel              (CtkActionMuxer *muxer,
                                                                         const gchar    *action_and_target);

/* No better place for these... */
gchar *                 ctk_print_action_and_target                     (const gchar    *action_namespace,
                                                                         const gchar    *action_name,
                                                                         GVariant       *target);

gchar *                 ctk_normalise_detailed_action_name              (const gchar *detailed_action_name);

G_END_DECLS

#endif /* __CTK_ACTION_MUXER_H__ */
