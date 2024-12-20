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

#include "config.h"

#include "ctkactionmuxer.h"

#include "ctkactionobservable.h"
#include "ctkactionobserver.h"
#include "ctkintl.h"
#include "ctkmarshalers.h"

#include <string.h>

/*< private >
 * SECTION:ctkactionmuxer
 * @short_description: Aggregate and monitor several action groups
 *
 * #CtkActionMuxer is a #GActionGroup and #CtkActionObservable that is
 * capable of containing other #GActionGroup instances.
 *
 * The typical use is aggregating all of the actions applicable to a
 * particular context into a single action group, with namespacing.
 *
 * Consider the case of two action groups -- one containing actions
 * applicable to an entire application (such as “quit”) and one
 * containing actions applicable to a particular window in the
 * application (such as “fullscreen”).
 *
 * In this case, each of these action groups could be added to a
 * #CtkActionMuxer with the prefixes “app” and “win”, respectively.  This
 * would expose the actions as “app.quit” and “win.fullscreen” on the
 * #GActionGroup interface presented by the #CtkActionMuxer.
 *
 * Activations and state change requests on the #CtkActionMuxer are wired
 * through to the underlying action group in the expected way.
 *
 * This class is typically only used at the site of “consumption” of
 * actions (eg: when displaying a menu that contains many actions on
 * different objects).
 */

static void     ctk_action_muxer_group_iface_init         (GActionGroupInterface        *iface);
static void     ctk_action_muxer_observable_iface_init    (CtkActionObservableInterface *iface);

typedef GObjectClass CtkActionMuxerClass;

struct _CtkActionMuxer
{
  GObject parent_instance;

  GHashTable *observed_actions;
  GHashTable *groups;
  GHashTable *primary_accels;
  CtkActionMuxer *parent;
};

G_DEFINE_TYPE_WITH_CODE (CtkActionMuxer, ctk_action_muxer, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_ACTION_GROUP, ctk_action_muxer_group_iface_init)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_ACTION_OBSERVABLE, ctk_action_muxer_observable_iface_init))

enum
{
  PROP_0,
  PROP_PARENT,
  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES];

guint accel_signal;

typedef struct
{
  CtkActionMuxer *muxer;
  GSList       *watchers;
  gchar        *fullname;
} Action;

typedef struct
{
  CtkActionMuxer *muxer;
  GActionGroup *group;
  gchar        *prefix;
  gulong        handler_ids[4];
} Group;

static void
ctk_action_muxer_append_group_actions (gpointer key,
                                       gpointer value,
                                       gpointer user_data)
{
  const gchar *prefix = key;
  Group *group = value;
  GArray *actions = user_data;
  gchar **group_actions;
  gchar **action;

  group_actions = g_action_group_list_actions (group->group);
  for (action = group_actions; *action; action++)
    {
      gchar *fullname;

      fullname = g_strconcat (prefix, ".", *action, NULL);
      g_array_append_val (actions, fullname);
    }

  g_strfreev (group_actions);
}

static gchar **
ctk_action_muxer_list_actions (GActionGroup *action_group)
{
  CtkActionMuxer *muxer = CTK_ACTION_MUXER (action_group);
  GArray *actions;

  actions = g_array_new (TRUE, FALSE, sizeof (gchar *));

  for ( ; muxer != NULL; muxer = muxer->parent)
    {
      g_hash_table_foreach (muxer->groups,
                            ctk_action_muxer_append_group_actions,
                            actions);
    }

  return (gchar **)(void *) g_array_free (actions, FALSE);
}

static Group *
ctk_action_muxer_find_group (CtkActionMuxer  *muxer,
                             const gchar     *full_name,
                             const gchar    **action_name)
{
  const gchar *dot;
  gchar *prefix;
  Group *group;

  dot = strchr (full_name, '.');

  if (!dot)
    return NULL;

  prefix = g_strndup (full_name, dot - full_name);
  group = g_hash_table_lookup (muxer->groups, prefix);
  g_free (prefix);

  if (action_name)
    *action_name = dot + 1;

  return group;
}

static void
ctk_action_muxer_action_enabled_changed (CtkActionMuxer *muxer,
                                         const gchar    *action_name,
                                         gboolean        enabled)
{
  Action *action;
  GSList *node;

  action = g_hash_table_lookup (muxer->observed_actions, action_name);
  for (node = action ? action->watchers : NULL; node; node = node->next)
    ctk_action_observer_action_enabled_changed (node->data, CTK_ACTION_OBSERVABLE (muxer), action_name, enabled);
  g_action_group_action_enabled_changed (G_ACTION_GROUP (muxer), action_name, enabled);
}

static void
ctk_action_muxer_group_action_enabled_changed (GActionGroup *action_group G_GNUC_UNUSED,
                                               const gchar  *action_name,
                                               gboolean      enabled,
                                               gpointer      user_data)
{
  Group *group = user_data;
  gchar *fullname;

  fullname = g_strconcat (group->prefix, ".", action_name, NULL);
  ctk_action_muxer_action_enabled_changed (group->muxer, fullname, enabled);

  g_free (fullname);
}

static void
ctk_action_muxer_parent_action_enabled_changed (GActionGroup *action_group G_GNUC_UNUSED,
                                                const gchar  *action_name,
                                                gboolean      enabled,
                                                gpointer      user_data)
{
  CtkActionMuxer *muxer = user_data;

  ctk_action_muxer_action_enabled_changed (muxer, action_name, enabled);
}

static void
ctk_action_muxer_action_state_changed (CtkActionMuxer *muxer,
                                       const gchar    *action_name,
                                       GVariant       *state)
{
  Action *action;
  GSList *node;

  action = g_hash_table_lookup (muxer->observed_actions, action_name);
  for (node = action ? action->watchers : NULL; node; node = node->next)
    ctk_action_observer_action_state_changed (node->data, CTK_ACTION_OBSERVABLE (muxer), action_name, state);
  g_action_group_action_state_changed (G_ACTION_GROUP (muxer), action_name, state);
}

static void
ctk_action_muxer_group_action_state_changed (GActionGroup *action_group G_GNUC_UNUSED,
                                             const gchar  *action_name,
                                             GVariant     *state,
                                             gpointer      user_data)
{
  Group *group = user_data;
  gchar *fullname;

  fullname = g_strconcat (group->prefix, ".", action_name, NULL);
  ctk_action_muxer_action_state_changed (group->muxer, fullname, state);

  g_free (fullname);
}

static void
ctk_action_muxer_parent_action_state_changed (GActionGroup *action_group G_GNUC_UNUSED,
                                              const gchar  *action_name,
                                              GVariant     *state,
                                              gpointer      user_data)
{
  CtkActionMuxer *muxer = user_data;

  ctk_action_muxer_action_state_changed (muxer, action_name, state);
}

static void
ctk_action_muxer_action_added (CtkActionMuxer *muxer,
                               const gchar    *action_name,
                               GActionGroup   *original_group,
                               const gchar    *orignal_action_name)
{
  const GVariantType *parameter_type;
  gboolean enabled;
  GVariant *state;
  Action *action;

  action = g_hash_table_lookup (muxer->observed_actions, action_name);

  if (action && action->watchers &&
      g_action_group_query_action (original_group, orignal_action_name,
                                   &enabled, &parameter_type, NULL, NULL, &state))
    {
      GSList *node;

      for (node = action->watchers; node; node = node->next)
        ctk_action_observer_action_added (node->data,
                                        CTK_ACTION_OBSERVABLE (muxer),
                                        action_name, parameter_type, enabled, state);

      if (state)
        g_variant_unref (state);
    }

  g_action_group_action_added (G_ACTION_GROUP (muxer), action_name);
}

static void
ctk_action_muxer_action_added_to_group (GActionGroup *action_group,
                                        const gchar  *action_name,
                                        gpointer      user_data)
{
  Group *group = user_data;
  gchar *fullname;

  fullname = g_strconcat (group->prefix, ".", action_name, NULL);
  ctk_action_muxer_action_added (group->muxer, fullname, action_group, action_name);

  g_free (fullname);
}

static void
ctk_action_muxer_action_added_to_parent (GActionGroup *action_group,
                                         const gchar  *action_name,
                                         gpointer      user_data)
{
  CtkActionMuxer *muxer = user_data;

  ctk_action_muxer_action_added (muxer, action_name, action_group, action_name);
}

static void
ctk_action_muxer_action_removed (CtkActionMuxer *muxer,
                                 const gchar    *action_name)
{
  Action *action;
  GSList *node;

  action = g_hash_table_lookup (muxer->observed_actions, action_name);
  for (node = action ? action->watchers : NULL; node; node = node->next)
    ctk_action_observer_action_removed (node->data, CTK_ACTION_OBSERVABLE (muxer), action_name);
  g_action_group_action_removed (G_ACTION_GROUP (muxer), action_name);
}

static void
ctk_action_muxer_action_removed_from_group (GActionGroup *action_group G_GNUC_UNUSED,
                                            const gchar  *action_name,
                                            gpointer      user_data)
{
  Group *group = user_data;
  gchar *fullname;

  fullname = g_strconcat (group->prefix, ".", action_name, NULL);
  ctk_action_muxer_action_removed (group->muxer, fullname);

  g_free (fullname);
}

static void
ctk_action_muxer_action_removed_from_parent (GActionGroup *action_group G_GNUC_UNUSED,
                                             const gchar  *action_name,
                                             gpointer      user_data)
{
  CtkActionMuxer *muxer = user_data;

  ctk_action_muxer_action_removed (muxer, action_name);
}

static void
ctk_action_muxer_primary_accel_changed (CtkActionMuxer *muxer,
                                        const gchar    *action_name,
                                        const gchar    *action_and_target)
{
  Action *action;
  GSList *node;

  if (!action_name)
    action_name = strrchr (action_and_target, '|') + 1;

  action = g_hash_table_lookup (muxer->observed_actions, action_name);
  for (node = action ? action->watchers : NULL; node; node = node->next)
    ctk_action_observer_primary_accel_changed (node->data, CTK_ACTION_OBSERVABLE (muxer),
                                               action_name, action_and_target);
  g_signal_emit (muxer, accel_signal, 0, action_name, action_and_target);
}

static void
ctk_action_muxer_parent_primary_accel_changed (CtkActionMuxer *parent G_GNUC_UNUSED,
                                               const gchar    *action_name,
                                               const gchar    *action_and_target,
                                               gpointer        user_data)
{
  CtkActionMuxer *muxer = user_data;

  /* If it's in our table then don't let the parent one filter through */
  if (muxer->primary_accels && g_hash_table_lookup (muxer->primary_accels, action_and_target))
    return;

  ctk_action_muxer_primary_accel_changed (muxer, action_name, action_and_target);
}

static gboolean
ctk_action_muxer_query_action (GActionGroup        *action_group,
                               const gchar         *action_name,
                               gboolean            *enabled,
                               const GVariantType **parameter_type,
                               const GVariantType **state_type,
                               GVariant           **state_hint,
                               GVariant           **state)
{
  CtkActionMuxer *muxer = CTK_ACTION_MUXER (action_group);
  Group *group;
  const gchar *unprefixed_name;

  group = ctk_action_muxer_find_group (muxer, action_name, &unprefixed_name);

  if (group)
    return g_action_group_query_action (group->group, unprefixed_name, enabled,
                                        parameter_type, state_type, state_hint, state);

  if (muxer->parent)
    return g_action_group_query_action (G_ACTION_GROUP (muxer->parent), action_name,
                                        enabled, parameter_type,
                                        state_type, state_hint, state);

  return FALSE;
}

static void
ctk_action_muxer_activate_action (GActionGroup *action_group,
                                  const gchar  *action_name,
                                  GVariant     *parameter)
{
  CtkActionMuxer *muxer = CTK_ACTION_MUXER (action_group);
  Group *group;
  const gchar *unprefixed_name;

  group = ctk_action_muxer_find_group (muxer, action_name, &unprefixed_name);

  if (group)
    g_action_group_activate_action (group->group, unprefixed_name, parameter);
  else if (muxer->parent)
    g_action_group_activate_action (G_ACTION_GROUP (muxer->parent), action_name, parameter);
}

static void
ctk_action_muxer_change_action_state (GActionGroup *action_group,
                                      const gchar  *action_name,
                                      GVariant     *state)
{
  CtkActionMuxer *muxer = CTK_ACTION_MUXER (action_group);
  Group *group;
  const gchar *unprefixed_name;

  group = ctk_action_muxer_find_group (muxer, action_name, &unprefixed_name);

  if (group)
    g_action_group_change_action_state (group->group, unprefixed_name, state);
  else if (muxer->parent)
    g_action_group_change_action_state (G_ACTION_GROUP (muxer->parent), action_name, state);
}

static void
ctk_action_muxer_unregister_internal (Action   *action,
                                      gpointer  observer)
{
  CtkActionMuxer *muxer = action->muxer;
  GSList **ptr;

  for (ptr = &action->watchers; *ptr; ptr = &(*ptr)->next)
    if ((*ptr)->data == observer)
      {
        *ptr = g_slist_remove (*ptr, observer);

        if (action->watchers == NULL)
            g_hash_table_remove (muxer->observed_actions, action->fullname);

        break;
      }
}

static void
ctk_action_muxer_weak_notify (gpointer  data,
                              GObject  *where_the_object_was)
{
  Action *action = data;

  ctk_action_muxer_unregister_internal (action, where_the_object_was);
}

static void
ctk_action_muxer_register_observer (CtkActionObservable *observable,
                                    const gchar         *name,
                                    CtkActionObserver   *observer)
{
  CtkActionMuxer *muxer = CTK_ACTION_MUXER (observable);
  Action *action;

  action = g_hash_table_lookup (muxer->observed_actions, name);

  if (action == NULL)
    {
      action = g_slice_new (Action);
      action->muxer = muxer;
      action->fullname = g_strdup (name);
      action->watchers = NULL;

      g_hash_table_insert (muxer->observed_actions, action->fullname, action);
    }

  action->watchers = g_slist_prepend (action->watchers, observer);
  g_object_weak_ref (G_OBJECT (observer), ctk_action_muxer_weak_notify, action);
}

static void
ctk_action_muxer_unregister_observer (CtkActionObservable *observable,
                                      const gchar         *name,
                                      CtkActionObserver   *observer)
{
  CtkActionMuxer *muxer = CTK_ACTION_MUXER (observable);
  Action *action;

  action = g_hash_table_lookup (muxer->observed_actions, name);
  g_object_weak_unref (G_OBJECT (observer), ctk_action_muxer_weak_notify, action);
  ctk_action_muxer_unregister_internal (action, observer);
}

static void
ctk_action_muxer_free_group (gpointer data)
{
  Group *group = data;
  gint i;

  /* 'for loop' or 'four loop'? */
  for (i = 0; i < 4; i++)
    g_signal_handler_disconnect (group->group, group->handler_ids[i]);

  g_object_unref (group->group);
  g_free (group->prefix);

  g_slice_free (Group, group);
}

static void
ctk_action_muxer_free_action (gpointer data)
{
  Action *action = data;
  GSList *it;

  for (it = action->watchers; it; it = it->next)
    g_object_weak_unref (G_OBJECT (it->data), ctk_action_muxer_weak_notify, action);

  g_slist_free (action->watchers);
  g_free (action->fullname);

  g_slice_free (Action, action);
}

static void
ctk_action_muxer_finalize (GObject *object)
{
  CtkActionMuxer *muxer = CTK_ACTION_MUXER (object);

  g_assert_cmpint (g_hash_table_size (muxer->observed_actions), ==, 0);
  g_hash_table_unref (muxer->observed_actions);
  g_hash_table_unref (muxer->groups);
  if (muxer->primary_accels)
    g_hash_table_unref (muxer->primary_accels);

  G_OBJECT_CLASS (ctk_action_muxer_parent_class)
    ->finalize (object);
}

static void
ctk_action_muxer_dispose (GObject *object)
{
  CtkActionMuxer *muxer = CTK_ACTION_MUXER (object);

  if (muxer->parent)
  {
    g_signal_handlers_disconnect_by_func (muxer->parent, ctk_action_muxer_action_added_to_parent, muxer);
    g_signal_handlers_disconnect_by_func (muxer->parent, ctk_action_muxer_action_removed_from_parent, muxer);
    g_signal_handlers_disconnect_by_func (muxer->parent, ctk_action_muxer_parent_action_enabled_changed, muxer);
    g_signal_handlers_disconnect_by_func (muxer->parent, ctk_action_muxer_parent_action_state_changed, muxer);
    g_signal_handlers_disconnect_by_func (muxer->parent, ctk_action_muxer_parent_primary_accel_changed, muxer);

    g_clear_object (&muxer->parent);
  }

  g_hash_table_remove_all (muxer->observed_actions);

  G_OBJECT_CLASS (ctk_action_muxer_parent_class)
    ->dispose (object);
}

static void
ctk_action_muxer_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  CtkActionMuxer *muxer = CTK_ACTION_MUXER (object);

  switch (property_id)
    {
    case PROP_PARENT:
      g_value_set_object (value, ctk_action_muxer_get_parent (muxer));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
ctk_action_muxer_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  CtkActionMuxer *muxer = CTK_ACTION_MUXER (object);

  switch (property_id)
    {
    case PROP_PARENT:
      ctk_action_muxer_set_parent (muxer, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
ctk_action_muxer_init (CtkActionMuxer *muxer)
{
  muxer->observed_actions = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, ctk_action_muxer_free_action);
  muxer->groups = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, ctk_action_muxer_free_group);
}

static void
ctk_action_muxer_observable_iface_init (CtkActionObservableInterface *iface)
{
  iface->register_observer = ctk_action_muxer_register_observer;
  iface->unregister_observer = ctk_action_muxer_unregister_observer;
}

static void
ctk_action_muxer_group_iface_init (GActionGroupInterface *iface)
{
  iface->list_actions = ctk_action_muxer_list_actions;
  iface->query_action = ctk_action_muxer_query_action;
  iface->activate_action = ctk_action_muxer_activate_action;
  iface->change_action_state = ctk_action_muxer_change_action_state;
}

static void
ctk_action_muxer_class_init (GObjectClass *class)
{
  class->get_property = ctk_action_muxer_get_property;
  class->set_property = ctk_action_muxer_set_property;
  class->finalize = ctk_action_muxer_finalize;
  class->dispose = ctk_action_muxer_dispose;

  accel_signal = g_signal_new (I_("primary-accel-changed"),
                               CTK_TYPE_ACTION_MUXER,
                               G_SIGNAL_RUN_LAST,
                               0,
                               NULL, NULL,
                               _ctk_marshal_VOID__STRING_STRING,
                               G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_STRING);
  g_signal_set_va_marshaller (accel_signal,
                              G_TYPE_FROM_CLASS (class),
                              _ctk_marshal_VOID__STRING_STRINGv);

  properties[PROP_PARENT] = g_param_spec_object ("parent", "Parent",
                                                 "The parent muxer",
                                                 CTK_TYPE_ACTION_MUXER,
                                                 G_PARAM_READWRITE |
                                                 G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (class, NUM_PROPERTIES, properties);
}

/*< private >
 * ctk_action_muxer_insert:
 * @muxer: a #CtkActionMuxer
 * @prefix: the prefix string for the action group
 * @action_group: a #GActionGroup
 *
 * Adds the actions in @action_group to the list of actions provided by
 * @muxer.  @prefix is prefixed to each action name, such that for each
 * action `x` in @action_group, there is an equivalent
 * action @prefix`.x` in @muxer.
 *
 * For example, if @prefix is “`app`” and @action_group
 * contains an action called “`quit`”, then @muxer will
 * now contain an action called “`app.quit`”.
 *
 * If any #CtkActionObservers are registered for actions in the group,
 * “action_added” notifications will be emitted, as appropriate.
 *
 * @prefix must not contain a dot ('.').
 */
void
ctk_action_muxer_insert (CtkActionMuxer *muxer,
                         const gchar    *prefix,
                         GActionGroup   *action_group)
{
  gchar **actions;
  Group *group;
  gint i;

  g_object_ref (action_group);

  /* TODO: diff instead of ripout and replace */
  ctk_action_muxer_remove (muxer, prefix);

  group = g_slice_new (Group);
  group->muxer = muxer;
  group->group = action_group;
  group->prefix = g_strdup (prefix);

  g_hash_table_insert (muxer->groups, group->prefix, group);

  actions = g_action_group_list_actions (group->group);
  for (i = 0; actions[i]; i++)
    ctk_action_muxer_action_added_to_group (group->group, actions[i], group);
  g_strfreev (actions);

  group->handler_ids[0] = g_signal_connect (group->group, "action-added",
                                            G_CALLBACK (ctk_action_muxer_action_added_to_group), group);
  group->handler_ids[1] = g_signal_connect (group->group, "action-removed",
                                            G_CALLBACK (ctk_action_muxer_action_removed_from_group), group);
  group->handler_ids[2] = g_signal_connect (group->group, "action-enabled-changed",
                                            G_CALLBACK (ctk_action_muxer_group_action_enabled_changed), group);
  group->handler_ids[3] = g_signal_connect (group->group, "action-state-changed",
                                            G_CALLBACK (ctk_action_muxer_group_action_state_changed), group);
}

/*< private >
 * ctk_action_muxer_remove:
 * @muxer: a #CtkActionMuxer
 * @prefix: the prefix of the action group to remove
 *
 * Removes a #GActionGroup from the #CtkActionMuxer.
 *
 * If any #CtkActionObservers are registered for actions in the group,
 * “action_removed” notifications will be emitted, as appropriate.
 */
void
ctk_action_muxer_remove (CtkActionMuxer *muxer,
                         const gchar    *prefix)
{
  Group *group;

  group = g_hash_table_lookup (muxer->groups, prefix);

  if (group != NULL)
    {
      gchar **actions;
      gint i;

      g_hash_table_steal (muxer->groups, prefix);

      actions = g_action_group_list_actions (group->group);
      for (i = 0; actions[i]; i++)
        ctk_action_muxer_action_removed_from_group (group->group, actions[i], group);
      g_strfreev (actions);

      ctk_action_muxer_free_group (group);
    }
}

static void
ctk_action_muxer_append_prefixes (gpointer key,
                                  gpointer value G_GNUC_UNUSED,
                                  gpointer user_data)
{
  const gchar *prefix = key;
  GArray *prefixes= user_data;

  g_array_append_val (prefixes, prefix);
}

const gchar **
ctk_action_muxer_list_prefixes (CtkActionMuxer *muxer)
{
  GArray *prefixes;

  prefixes = g_array_new (TRUE, FALSE, sizeof (gchar *));

  for ( ; muxer != NULL; muxer = muxer->parent)
    {
      g_hash_table_foreach (muxer->groups,
                            ctk_action_muxer_append_prefixes,
                            prefixes);
    }

  return (const gchar **)(void *) g_array_free (prefixes, FALSE);
}

GActionGroup *
ctk_action_muxer_lookup (CtkActionMuxer *muxer,
                         const gchar    *prefix)
{
  Group *group;

  for ( ; muxer != NULL; muxer = muxer->parent)
    {
      group = g_hash_table_lookup (muxer->groups, prefix);

      if (group != NULL)
        return group->group;
    }

  return NULL;
}

/*< private >
 * ctk_action_muxer_new:
 *
 * Creates a new #CtkActionMuxer.
 */
CtkActionMuxer *
ctk_action_muxer_new (void)
{
  return g_object_new (CTK_TYPE_ACTION_MUXER, NULL);
}

/*< private >
 * ctk_action_muxer_get_parent:
 * @muxer: a #CtkActionMuxer
 *
 * Returns: (transfer none): the parent of @muxer, or NULL.
 */
CtkActionMuxer *
ctk_action_muxer_get_parent (CtkActionMuxer *muxer)
{
  g_return_val_if_fail (CTK_IS_ACTION_MUXER (muxer), NULL);

  return muxer->parent;
}

static void
emit_changed_accels (CtkActionMuxer  *muxer,
                     CtkActionMuxer  *parent)
{
  while (parent)
    {
      if (parent->primary_accels)
        {
          GHashTableIter iter;
          gpointer key;

          g_hash_table_iter_init (&iter, parent->primary_accels);
          while (g_hash_table_iter_next (&iter, &key, NULL))
            ctk_action_muxer_primary_accel_changed (muxer, NULL, key);
        }

      parent = parent->parent;
    }
}

/*< private >
 * ctk_action_muxer_set_parent:
 * @muxer: a #CtkActionMuxer
 * @parent: (allow-none): the new parent #CtkActionMuxer
 *
 * Sets the parent of @muxer to @parent.
 */
void
ctk_action_muxer_set_parent (CtkActionMuxer *muxer,
                             CtkActionMuxer *parent)
{
  g_return_if_fail (CTK_IS_ACTION_MUXER (muxer));
  g_return_if_fail (parent == NULL || CTK_IS_ACTION_MUXER (parent));

  if (muxer->parent == parent)
    return;

  if (muxer->parent != NULL)
    {
      gchar **actions;
      gchar **it;

      actions = g_action_group_list_actions (G_ACTION_GROUP (muxer->parent));
      for (it = actions; *it; it++)
        ctk_action_muxer_action_removed (muxer, *it);
      g_strfreev (actions);

      emit_changed_accels (muxer, muxer->parent);

      g_signal_handlers_disconnect_by_func (muxer->parent, ctk_action_muxer_action_added_to_parent, muxer);
      g_signal_handlers_disconnect_by_func (muxer->parent, ctk_action_muxer_action_removed_from_parent, muxer);
      g_signal_handlers_disconnect_by_func (muxer->parent, ctk_action_muxer_parent_action_enabled_changed, muxer);
      g_signal_handlers_disconnect_by_func (muxer->parent, ctk_action_muxer_parent_action_state_changed, muxer);
      g_signal_handlers_disconnect_by_func (muxer->parent, ctk_action_muxer_parent_primary_accel_changed, muxer);

      g_object_unref (muxer->parent);
    }

  muxer->parent = parent;

  if (muxer->parent != NULL)
    {
      gchar **actions;
      gchar **it;

      g_object_ref (muxer->parent);

      actions = g_action_group_list_actions (G_ACTION_GROUP (muxer->parent));
      for (it = actions; *it; it++)
        ctk_action_muxer_action_added (muxer, *it, G_ACTION_GROUP (muxer->parent), *it);
      g_strfreev (actions);

      emit_changed_accels (muxer, muxer->parent);

      g_signal_connect (muxer->parent, "action-added",
                        G_CALLBACK (ctk_action_muxer_action_added_to_parent), muxer);
      g_signal_connect (muxer->parent, "action-removed",
                        G_CALLBACK (ctk_action_muxer_action_removed_from_parent), muxer);
      g_signal_connect (muxer->parent, "action-enabled-changed",
                        G_CALLBACK (ctk_action_muxer_parent_action_enabled_changed), muxer);
      g_signal_connect (muxer->parent, "action-state-changed",
                        G_CALLBACK (ctk_action_muxer_parent_action_state_changed), muxer);
      g_signal_connect (muxer->parent, "primary-accel-changed",
                        G_CALLBACK (ctk_action_muxer_parent_primary_accel_changed), muxer);
    }

  g_object_notify_by_pspec (G_OBJECT (muxer), properties[PROP_PARENT]);
}

void
ctk_action_muxer_set_primary_accel (CtkActionMuxer *muxer,
                                    const gchar    *action_and_target,
                                    const gchar    *primary_accel)
{
  if (!muxer->primary_accels)
    muxer->primary_accels = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

  if (primary_accel)
    g_hash_table_insert (muxer->primary_accels, g_strdup (action_and_target), g_strdup (primary_accel));
  else
    g_hash_table_remove (muxer->primary_accels, action_and_target);

  ctk_action_muxer_primary_accel_changed (muxer, NULL, action_and_target);
}

const gchar *
ctk_action_muxer_get_primary_accel (CtkActionMuxer *muxer,
                                    const gchar    *action_and_target)
{
  if (muxer->primary_accels)
    {
      const gchar *primary_accel;

      primary_accel = g_hash_table_lookup (muxer->primary_accels, action_and_target);

      if (primary_accel)
        return primary_accel;
    }

  if (!muxer->parent)
    return NULL;

  return ctk_action_muxer_get_primary_accel (muxer->parent, action_and_target);
}

gchar *
ctk_print_action_and_target (const gchar *action_namespace,
                             const gchar *action_name,
                             GVariant    *target)
{
  GString *result;

  g_return_val_if_fail (strchr (action_name, '|') == NULL, NULL);
  g_return_val_if_fail (action_namespace == NULL || strchr (action_namespace, '|') == NULL, NULL);

  result = g_string_new (NULL);

  if (target)
    g_variant_print_string (target, result, TRUE);
  g_string_append_c (result, '|');

  if (action_namespace)
    {
      g_string_append (result, action_namespace);
      g_string_append_c (result, '.');
    }

  g_string_append (result, action_name);

  return g_string_free (result, FALSE);
}

gchar *
ctk_normalise_detailed_action_name (const gchar *detailed_action_name)
{
  GError *error = NULL;
  gchar *action_and_target;
  gchar *action_name;
  GVariant *target;

  g_action_parse_detailed_name (detailed_action_name, &action_name, &target, &error);
  g_assert_no_error (error);

  action_and_target = ctk_print_action_and_target (NULL, action_name, target);

  if (target)
    g_variant_unref (target);

  g_free (action_name);

  return action_and_target;
}
