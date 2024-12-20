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

#include "config.h"

#include "ctkactionobserver.h"

G_DEFINE_INTERFACE (CtkActionObserver, ctk_action_observer, G_TYPE_OBJECT)

/*< private >
 * SECTION:ctkactionobserver
 * @short_description: an interface implemented by objects that are
 *                     interested in monitoring actions for changes
 *
 * CtkActionObserver is a simple interface allowing objects that wish to
 * be notified of changes to actions to be notified of those changes.
 *
 * It is also possible to monitor changes to action groups using
 * #GObject signals, but there are a number of reasons that this
 * approach could become problematic:
 *
 *  - there are four separate signals that must be manually connected
 *    and disconnected
 *
 *  - when a large number of different observers wish to monitor a
 *    (usually disjoint) set of actions within the same action group,
 *    there is only one way to avoid having all notifications delivered
 *    to all observers: signal detail.  In order to use signal detail,
 *    each action name must be quarked, which is not always practical.
 *
 *  - even if quarking is acceptable, #GObject signal details are
 *    implemented by scanning a linked list, so there is no real
 *    decrease in complexity
 */

void
ctk_action_observer_default_init (CtkActionObserverInterface *class G_GNUC_UNUSED)
{
}

/*< private >
 * ctk_action_observer_action_added:
 * @observer: a #CtkActionObserver
 * @observable: the source of the event
 * @action_name: the name of the action
 * @enabled: %TRUE if the action is now enabled
 * @parameter_type: the parameter type for action invocations, or %NULL
 *                  if no parameter is required
 * @state: the current state of the action, or %NULL if the action is
 *         stateless
 *
 * This function is called when an action that the observer is
 * registered to receive events for is added.
 *
 * This function should only be called by objects with which the
 * observer has explicitly registered itself to receive events.
 */
void
ctk_action_observer_action_added (CtkActionObserver   *observer,
                                  CtkActionObservable *observable,
                                  const gchar         *action_name,
                                  const GVariantType  *parameter_type,
                                  gboolean             enabled,
                                  GVariant            *state)
{
  g_return_if_fail (CTK_IS_ACTION_OBSERVER (observer));

  CTK_ACTION_OBSERVER_GET_IFACE (observer)
    ->action_added (observer, observable, action_name, parameter_type, enabled, state);
}

/*< private >
 * ctk_action_observer_action_enabled_changed:
 * @observer: a #CtkActionObserver
 * @observable: the source of the event
 * @action_name: the name of the action
 * @enabled: %TRUE if the action is now enabled
 *
 * This function is called when an action that the observer is
 * registered to receive events for becomes enabled or disabled.
 *
 * This function should only be called by objects with which the
 * observer has explicitly registered itself to receive events.
 */
void
ctk_action_observer_action_enabled_changed (CtkActionObserver   *observer,
                                            CtkActionObservable *observable,
                                            const gchar         *action_name,
                                            gboolean             enabled)
{
  g_return_if_fail (CTK_IS_ACTION_OBSERVER (observer));

  CTK_ACTION_OBSERVER_GET_IFACE (observer)
    ->action_enabled_changed (observer, observable, action_name, enabled);
}

/*< private >
 * ctk_action_observer_action_state_changed:
 * @observer: a #CtkActionObserver
 * @observable: the source of the event
 * @action_name: the name of the action
 * @state: the new state of the action
 *
 * This function is called when an action that the observer is
 * registered to receive events for changes to its state.
 *
 * This function should only be called by objects with which the
 * observer has explicitly registered itself to receive events.
 */
void
ctk_action_observer_action_state_changed (CtkActionObserver   *observer,
                                          CtkActionObservable *observable,
                                          const gchar         *action_name,
                                          GVariant            *state)
{
  g_return_if_fail (CTK_IS_ACTION_OBSERVER (observer));

  CTK_ACTION_OBSERVER_GET_IFACE (observer)
    ->action_state_changed (observer, observable, action_name, state);
}

/*< private >
 * ctk_action_observer_action_removed:
 * @observer: a #CtkActionObserver
 * @observable: the source of the event
 * @action_name: the name of the action
 *
 * This function is called when an action that the observer is
 * registered to receive events for is removed.
 *
 * This function should only be called by objects with which the
 * observer has explicitly registered itself to receive events.
 */
void
ctk_action_observer_action_removed (CtkActionObserver   *observer,
                                    CtkActionObservable *observable,
                                    const gchar         *action_name)
{
  g_return_if_fail (CTK_IS_ACTION_OBSERVER (observer));

  CTK_ACTION_OBSERVER_GET_IFACE (observer)
    ->action_removed (observer, observable, action_name);
}

/*< private >
 * ctk_action_observer_primary_accel_changed:
 * @observer: a #CtkActionObserver
 * @observable: the source of the event
 * @action_name: the name of the action
 * @action_and_target: detailed action of the changed accel, in “action and target” format
 *
 * This function is called when an action that the observer is
 * registered to receive events for has one of its accelerators changed.
 *
 * Accelerator changes are reported for all targets associated with the
 * action.  The @action_and_target string should be used to check if the
 * reported target is the one that the observer is interested in.
 */
void
ctk_action_observer_primary_accel_changed (CtkActionObserver   *observer,
                                           CtkActionObservable *observable,
                                           const gchar         *action_name,
                                           const gchar         *action_and_target)
{
  CtkActionObserverInterface *iface;

  g_return_if_fail (CTK_IS_ACTION_OBSERVER (observer));

  iface = CTK_ACTION_OBSERVER_GET_IFACE (observer);

  if (iface->primary_accel_changed)
    iface->primary_accel_changed (observer, observable, action_name, action_and_target);
}
