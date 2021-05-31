/*
 * Copyright © 2011 Canonical Limited
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

#ifndef __CTK_ACTION_OBSERVER_H__
#define __CTK_ACTION_OBSERVER_H__

#include <gio/gio.h>

G_BEGIN_DECLS

#define CTK_TYPE_ACTION_OBSERVER                            (ctk_action_observer_get_type ())
#define CTK_ACTION_OBSERVER(inst)                           (G_TYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             CTK_TYPE_ACTION_OBSERVER, CtkActionObserver))
#define CTK_IS_ACTION_OBSERVER(inst)                        (G_TYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             CTK_TYPE_ACTION_OBSERVER))
#define CTK_ACTION_OBSERVER_GET_IFACE(inst)                 (G_TYPE_INSTANCE_GET_INTERFACE ((inst),                  \
                                                             CTK_TYPE_ACTION_OBSERVER, CtkActionObserverInterface))

typedef struct _CtkActionObserverInterface                  CtkActionObserverInterface;
typedef struct _CtkActionObservable                         CtkActionObservable;
typedef struct _CtkActionObserver                           CtkActionObserver;

struct _CtkActionObserverInterface
{
  GTypeInterface g_iface;

  void (* action_added)           (CtkActionObserver    *observer,
                                   CtkActionObservable  *observable,
                                   const gchar          *action_name,
                                   const GVariantType   *parameter_type,
                                   gboolean              enabled,
                                   GVariant             *state);
  void (* action_enabled_changed) (CtkActionObserver    *observer,
                                   CtkActionObservable  *observable,
                                   const gchar          *action_name,
                                   gboolean              enabled);
  void (* action_state_changed)   (CtkActionObserver    *observer,
                                   CtkActionObservable  *observable,
                                   const gchar          *action_name,
                                   GVariant             *state);
  void (* action_removed)         (CtkActionObserver    *observer,
                                   CtkActionObservable  *observable,
                                   const gchar          *action_name);
  void (* primary_accel_changed)  (CtkActionObserver    *observer,
                                   CtkActionObservable  *observable,
                                   const gchar          *action_name,
                                   const gchar          *action_and_target);
};

GType                   ctk_action_observer_get_type                    (void);
void                    ctk_action_observer_action_added                (CtkActionObserver   *observer,
                                                                         CtkActionObservable *observable,
                                                                         const gchar         *action_name,
                                                                         const GVariantType  *parameter_type,
                                                                         gboolean             enabled,
                                                                         GVariant            *state);
void                    ctk_action_observer_action_enabled_changed      (CtkActionObserver   *observer,
                                                                         CtkActionObservable *observable,
                                                                         const gchar         *action_name,
                                                                         gboolean             enabled);
void                    ctk_action_observer_action_state_changed        (CtkActionObserver   *observer,
                                                                         CtkActionObservable *observable,
                                                                         const gchar         *action_name,
                                                                         GVariant            *state);
void                    ctk_action_observer_action_removed              (CtkActionObserver   *observer,
                                                                         CtkActionObservable *observable,
                                                                         const gchar         *action_name);
void                    ctk_action_observer_primary_accel_changed       (CtkActionObserver   *observer,
                                                                         CtkActionObservable *observable,
                                                                         const gchar         *action_name,
                                                                         const gchar         *action_and_target);

G_END_DECLS

#endif /* __CTK_ACTION_OBSERVER_H__ */
