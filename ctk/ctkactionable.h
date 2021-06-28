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

#ifndef __CTK_ACTIONABLE_H__
#define __CTK_ACTIONABLE_H__

#include <glib-object.h>
#include <cdk/cdk.h>

G_BEGIN_DECLS

#define CTK_TYPE_ACTIONABLE                                 (ctk_actionable_get_type ())
#define CTK_ACTIONABLE(inst)                                (G_TYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             CTK_TYPE_ACTIONABLE, CtkActionable))
#define CTK_IS_ACTIONABLE(inst)                             (G_TYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             CTK_TYPE_ACTIONABLE))
#define CTK_ACTIONABLE_GET_IFACE(inst)                      (G_TYPE_INSTANCE_GET_INTERFACE ((inst),                  \
                                                             CTK_TYPE_ACTIONABLE, CtkActionableInterface))

typedef struct _CtkActionableInterface                      CtkActionableInterface;
typedef struct _CtkActionable                               CtkActionable;

struct _CtkActionableInterface
{
  /*< private >*/
  GTypeInterface g_iface;

  /*< public >*/

  const gchar * (* get_action_name)             (CtkActionable *actionable);
  void          (* set_action_name)             (CtkActionable *actionable,
                                                 const gchar   *action_name);
  GVariant *    (* get_action_target_value)     (CtkActionable *actionable);
  void          (* set_action_target_value)     (CtkActionable *actionable,
                                                 GVariant      *target_value);
};

GDK_AVAILABLE_IN_3_4
GType                   ctk_actionable_get_type                         (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_3_4
const gchar *           ctk_actionable_get_action_name                  (CtkActionable *actionable);
GDK_AVAILABLE_IN_3_4
void                    ctk_actionable_set_action_name                  (CtkActionable *actionable,
                                                                         const gchar   *action_name);

GDK_AVAILABLE_IN_3_4
GVariant *              ctk_actionable_get_action_target_value          (CtkActionable *actionable);
GDK_AVAILABLE_IN_3_4
void                    ctk_actionable_set_action_target_value          (CtkActionable *actionable,
                                                                         GVariant      *target_value);

GDK_AVAILABLE_IN_3_4
void                    ctk_actionable_set_action_target                (CtkActionable *actionable,
                                                                         const gchar   *format_string,
                                                                         ...);

GDK_AVAILABLE_IN_3_4
void                    ctk_actionable_set_detailed_action_name         (CtkActionable *actionable,
                                                                         const gchar   *detailed_action_name);

G_END_DECLS

#endif /* __CTK_ACTIONABLE_H__ */
