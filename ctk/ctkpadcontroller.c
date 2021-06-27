/* CTK - The GIMP Toolkit
 * Copyright (C) 2016, Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author(s): Carlos Garnacho <carlosg@gnome.org>
 */

/**
 * SECTION:ctkpadcontroller
 * @Short_description: Controller for drawing tablet pads
 * @Title: CtkPadController
 * @See_also: #CtkEventController, #GdkDevicePad
 *
 * #CtkPadController is an event controller for the pads found in drawing
 * tablets (The collection of buttons and tactile sensors often found around
 * the stylus-sensitive area).
 *
 * These buttons and sensors have no implicit meaning, and by default they
 * perform no action, this event controller is provided to map those to
 * #GAction objects, thus letting the application give those a more semantic
 * meaning.
 *
 * Buttons and sensors are not constrained to triggering a single action, some
 * %GDK_SOURCE_TABLET_PAD devices feature multiple "modes", all these input
 * elements have one current mode, which may determine the final action
 * being triggered. Pad devices often divide buttons and sensors into groups,
 * all elements in a group share the same current mode, but different groups
 * may have different modes. See gdk_device_pad_get_n_groups() and
 * gdk_device_pad_get_group_n_modes().
 *
 * Each of the actions that a given button/strip/ring performs for a given
 * mode is defined by #CtkPadActionEntry, it contains an action name that
 * will be looked up in the given #GActionGroup and activated whenever the
 * specified input element and mode are triggered.
 *
 * A simple example of #CtkPadController usage, assigning button 1 in all
 * modes and pad devices to an "invert-selection" action:
 * |[
 *   CtkPadActionEntry *pad_actions[] = {
 *     { CTK_PAD_ACTION_BUTTON, 1, -1, "Invert selection", "pad-actions.invert-selection" },
 *     …
 *   };
 *
 *   …
 *   action_group = g_simple_action_group_new ();
 *   action = g_simple_action_new ("pad-actions.invert-selection", NULL);
 *   g_signal_connect (action, "activate", on_invert_selection_activated, NULL);
 *   g_action_map_add_action (G_ACTION_MAP (action_group), action);
 *   …
 *   pad_controller = ctk_pad_controller_new (window, action_group, NULL);
 * ]|
 *
 * The actions belonging to rings/strips will be activated with a parameter
 * of type %G_VARIANT_TYPE_DOUBLE bearing the value of the given axis, it
 * is required that those are made stateful and accepting this #GVariantType.
 */

#include "config.h"

#include "ctkeventcontrollerprivate.h"
#include "ctkpadcontroller.h"
#include "ctkwindow.h"
#include "ctkprivate.h"
#include "ctkintl.h"

#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/wayland/gdkwayland.h>
#endif

struct _CtkPadController {
  CtkEventController parent_instance;
  GActionGroup *action_group;
  GdkDevice *pad;

  GList *entries;
};

struct _CtkPadControllerClass {
  CtkEventControllerClass parent_class;
};

enum {
  PROP_0,
  PROP_ACTION_GROUP,
  PROP_PAD,
  N_PROPS
};

static GParamSpec *pspecs[N_PROPS] = { NULL };

G_DEFINE_TYPE (CtkPadController, ctk_pad_controller, CTK_TYPE_EVENT_CONTROLLER)

static CtkPadActionEntry *
ctk_pad_action_entry_copy (const CtkPadActionEntry *entry)
{
  CtkPadActionEntry *copy;

  copy = g_slice_new0 (CtkPadActionEntry);
  *copy = *entry;
  copy->label = g_strdup (entry->label);
  copy->action_name = g_strdup (entry->action_name);

  return copy;
}

static void
ctk_pad_action_entry_free (CtkPadActionEntry *entry)
{
  g_free (entry->label);
  g_free (entry->action_name);
  g_slice_free (CtkPadActionEntry, entry);
}

static const CtkPadActionEntry *
ctk_pad_action_find_match (CtkPadController *controller,
                           CtkPadActionType  type,
                           gint              index,
                           gint              mode)
{
  GList *l;

  for (l = controller->entries; l; l = l->next)
    {
      CtkPadActionEntry *entry = l->data;
      gboolean match_index = FALSE, match_mode = FALSE;

      if (entry->type != type)
        continue;

      match_index = entry->index < 0 || entry->index == index;
      match_mode = entry->mode < 0 || entry->mode == mode;

      if (match_index && match_mode)
        return entry;
    }

  return NULL;
}

static void
ctk_pad_controller_activate_action (CtkPadController        *controller,
                                    const CtkPadActionEntry *entry)
{
  g_action_group_activate_action (controller->action_group,
                                  entry->action_name,
                                  NULL);
}

static void
ctk_pad_controller_activate_action_with_axis (CtkPadController        *controller,
                                              const CtkPadActionEntry *entry,
                                              gdouble                  value)
{
  g_action_group_activate_action (controller->action_group,
                                  entry->action_name,
                                  g_variant_new_double (value));
}

static void
ctk_pad_controller_handle_mode_switch (CtkPadController *controller,
                                       GdkDevice        *pad,
                                       guint             group,
                                       guint             mode)
{
#ifdef GDK_WINDOWING_WAYLAND
  if (GDK_IS_WAYLAND_DISPLAY (gdk_device_get_display (pad)))
    {
      const CtkPadActionEntry *entry;
      gint elem, idx, n_features;

      for (elem = CTK_PAD_ACTION_BUTTON; elem <= CTK_PAD_ACTION_STRIP; elem++)
        {
          n_features = gdk_device_pad_get_n_features (GDK_DEVICE_PAD (pad),
                                                      elem);

          for (idx = 0; idx < n_features; idx++)
            {
              if (gdk_device_pad_get_feature_group (GDK_DEVICE_PAD (pad),
                                                    elem, idx) != group)
                continue;

              entry = ctk_pad_action_find_match (controller, elem, idx, mode);
              if (!entry)
                continue;
              if (!g_action_group_has_action (controller->action_group,
                                              entry->action_name))
                continue;

              gdk_wayland_device_pad_set_feedback (pad, elem, idx,
                                                   g_dgettext (NULL, entry->label));
            }
        }
    }
#endif
}

static gboolean
ctk_pad_controller_filter_event (CtkEventController *controller,
                                 const GdkEvent     *event)
{
  CtkPadController *pad_controller = CTK_PAD_CONTROLLER (controller);

  if (event->type != GDK_PAD_BUTTON_PRESS &&
      event->type != GDK_PAD_BUTTON_RELEASE &&
      event->type != GDK_PAD_RING &&
      event->type != GDK_PAD_STRIP &&
      event->type != GDK_PAD_GROUP_MODE)
    return TRUE;

  if (pad_controller->pad &&
      gdk_event_get_source_device (event) != pad_controller->pad)
    return TRUE;

  return FALSE;
}

static gboolean
ctk_pad_controller_handle_event (CtkEventController *controller,
                                 const GdkEvent     *event)
{
  CtkPadController *pad_controller = CTK_PAD_CONTROLLER (controller);
  const CtkPadActionEntry *entry;
  CtkPadActionType type;
  gint index, mode;

  if (event->type == GDK_PAD_GROUP_MODE)
    {
      ctk_pad_controller_handle_mode_switch (pad_controller,
                                             gdk_event_get_source_device (event),
                                             event->pad_group_mode.group,
                                             event->pad_group_mode.mode);
      return GDK_EVENT_PROPAGATE;
    }

  switch (event->type)
    {
    case GDK_PAD_BUTTON_PRESS:
      type = CTK_PAD_ACTION_BUTTON;
      index = event->pad_button.button;
      mode = event->pad_button.mode;
      break;
    case GDK_PAD_RING:
    case GDK_PAD_STRIP:
      type = event->type == GDK_PAD_RING ?
        CTK_PAD_ACTION_RING : CTK_PAD_ACTION_STRIP;
      index = event->pad_axis.index;
      mode = event->pad_axis.mode;
      break;
    default:
      return GDK_EVENT_PROPAGATE;
    }

  entry = ctk_pad_action_find_match (pad_controller,
                                     type, index, mode);
  if (!entry)
    return GDK_EVENT_PROPAGATE;

  if (event->type == GDK_PAD_RING ||
      event->type == GDK_PAD_STRIP)
    {
      ctk_pad_controller_activate_action_with_axis (pad_controller, entry,
                                                    event->pad_axis.value);
    }
  else
    {
      ctk_pad_controller_activate_action (pad_controller, entry);
    }

  return GDK_EVENT_STOP;
}

static void
ctk_pad_controller_set_pad (CtkPadController *controller,
                            GdkDevice        *pad)
{
  g_return_if_fail (!pad || GDK_IS_DEVICE (pad));
  g_return_if_fail (!pad || gdk_device_get_source (pad) == GDK_SOURCE_TABLET_PAD);

  g_set_object (&controller->pad, pad);
}

static void
ctk_pad_controller_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  CtkPadController *controller = CTK_PAD_CONTROLLER (object);

  switch (prop_id)
    {
    case PROP_ACTION_GROUP:
      controller->action_group = g_value_dup_object (value);
      break;
    case PROP_PAD:
      ctk_pad_controller_set_pad (controller, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ctk_pad_controller_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  CtkPadController *controller = CTK_PAD_CONTROLLER (object);

  switch (prop_id)
    {
    case PROP_ACTION_GROUP:
      g_value_set_object (value, controller->action_group);
      break;
    case PROP_PAD:
      g_value_set_object (value, controller->pad);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ctk_pad_controller_dispose (GObject *object)
{
  CtkPadController *controller = CTK_PAD_CONTROLLER (object);

  g_clear_object (&controller->action_group);
  g_clear_object (&controller->pad);

  G_OBJECT_CLASS (ctk_pad_controller_parent_class)->dispose (object);
}

static void
ctk_pad_controller_finalize (GObject *object)
{
  CtkPadController *controller = CTK_PAD_CONTROLLER (object);

  g_list_free_full (controller->entries, (GDestroyNotify) ctk_pad_action_entry_free);

  G_OBJECT_CLASS (ctk_pad_controller_parent_class)->finalize (object);
}

static void
ctk_pad_controller_class_init (CtkPadControllerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CtkEventControllerClass *controller_class = CTK_EVENT_CONTROLLER_CLASS (klass);

  controller_class->filter_event = ctk_pad_controller_filter_event;
  controller_class->handle_event = ctk_pad_controller_handle_event;

  object_class->set_property = ctk_pad_controller_set_property;
  object_class->get_property = ctk_pad_controller_get_property;
  object_class->dispose = ctk_pad_controller_dispose;
  object_class->finalize = ctk_pad_controller_finalize;

  pspecs[PROP_ACTION_GROUP] =
    g_param_spec_object ("action-group",
                         P_("Action group"),
                         P_("Action group to launch actions from"),
                         G_TYPE_ACTION_GROUP,
                         CTK_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  pspecs[PROP_PAD] =
    g_param_spec_object ("pad",
                         P_("Pad device"),
                         P_("Pad device to control"),
                         GDK_TYPE_DEVICE,
                         CTK_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, N_PROPS, pspecs);
}

static void
ctk_pad_controller_init (CtkPadController *controller)
{
}

/**
 * ctk_pad_controller_new:
 * @window: a #CtkWindow
 * @group: #GActionGroup to trigger actions from
 * @pad: (nullable): A %GDK_SOURCE_TABLET_PAD device, or %NULL to handle all pads
 *
 * Creates a new #CtkPadController that will associate events from @pad to
 * actions. A %NULL pad may be provided so the controller manages all pad devices
 * generically, it is discouraged to mix #CtkPadController objects with %NULL
 * and non-%NULL @pad argument on the same @window, as execution order is not
 * guaranteed.
 *
 * The #CtkPadController is created with no mapped actions. In order to map pad
 * events to actions, use ctk_pad_controller_set_action_entries() or
 * ctk_pad_controller_set_action().
 *
 * Returns: A newly created #CtkPadController
 *
 * Since: 3.22
 **/
CtkPadController *
ctk_pad_controller_new (CtkWindow    *window,
                        GActionGroup *group,
                        GdkDevice    *pad)
{
  g_return_val_if_fail (CTK_IS_WINDOW (window), NULL);
  g_return_val_if_fail (G_IS_ACTION_GROUP (group), NULL);
  g_return_val_if_fail (!pad || GDK_IS_DEVICE (pad), NULL);
  g_return_val_if_fail (!pad || gdk_device_get_source (pad) == GDK_SOURCE_TABLET_PAD, NULL);

  return g_object_new (CTK_TYPE_PAD_CONTROLLER,
                       "propagation-phase", CTK_PHASE_CAPTURE,
                       "widget", window,
                       "action-group", group,
                       "pad", pad,
                       NULL);
}

static gint
entry_compare_func (gconstpointer a,
                    gconstpointer b)
{
  const CtkPadActionEntry *entry1 = a, *entry2 = b;

  if (entry1->mode > entry2->mode)
    return -1;
  else if (entry1->mode < entry2->mode)
    return 1;
  else if (entry1->index > entry2->index)
    return -1;
  else if (entry1->index < entry2->index)
    return 1;

  return 0;
}

static void
ctk_pad_controller_add_entry (CtkPadController        *controller,
                              const CtkPadActionEntry *entry)
{
  CtkPadActionEntry *copy;

  copy = ctk_pad_action_entry_copy (entry);
  controller->entries = g_list_insert_sorted (controller->entries, copy,
                                              (GCompareFunc) entry_compare_func);
}

/**
 * ctk_pad_controller_set_action_entries:
 * @controller: a #CtkPadController
 * @entries: (array length=n_entries): the action entries to set on @controller
 * @n_entries: the number of elements in @entries
 *
 * This is a convenience function to add a group of action entries on
 * @controller. See #CtkPadActionEntry and ctk_pad_controller_set_action().
 *
 * Since: 3.22
 **/
void
ctk_pad_controller_set_action_entries (CtkPadController        *controller,
                                       const CtkPadActionEntry *entries,
                                       gint                     n_entries)
{
  gint i;

  g_return_if_fail (CTK_IS_PAD_CONTROLLER (controller));
  g_return_if_fail (entries != NULL);

  for (i = 0; i < n_entries; i++)
    ctk_pad_controller_add_entry (controller, &entries[i]);
}

/**
 * ctk_pad_controller_set_action:
 * @controller: a #CtkPadController
 * @type: the type of pad feature that will trigger this action
 * @index: the 0-indexed button/ring/strip number that will trigger this action
 * @mode: the mode that will trigger this action, or -1 for all modes.
 * @label: Human readable description of this action, this string should
 *   be deemed user-visible.
 * @action_name: action name that will be activated in the #GActionGroup
 *
 * Adds an individual action to @controller. This action will only be activated
 * if the given button/ring/strip number in @index is interacted while
 * the current mode is @mode. -1 may be used for simple cases, so the action
 * is triggered on all modes.
 *
 * The given @label should be considered user-visible, so internationalization
 * rules apply. Some windowing systems may be able to use those for user
 * feedback.
 *
 * Since: 3.22
 **/
void
ctk_pad_controller_set_action (CtkPadController *controller,
                               CtkPadActionType  type,
                               gint              index,
                               gint              mode,
                               const gchar      *label,
                               const gchar      *action_name)
{
  CtkPadActionEntry entry = { type, index, mode,
                              (gchar *) label, (gchar *) action_name };

  g_return_if_fail (CTK_IS_PAD_CONTROLLER (controller));
  g_return_if_fail (type <= CTK_PAD_ACTION_STRIP);

  ctk_pad_controller_add_entry (controller, &entry);
}
