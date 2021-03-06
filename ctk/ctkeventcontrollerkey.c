/* CTK - The GIMP Toolkit
 * Copyright (C) 2017, Red Hat, Inc.
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
 * SECTION:ctkeventcontrollerkey
 * @Short_description: Event controller for key events
 * @Title: CtkEventControllerKey
 * @See_also: #CtkEventController
 *
 * #CtkEventControllerKey is an event controller meant for situations
 * where you need access to key events.
 *
 * This object was added in 3.24.
 **/

#include "config.h"

#include "ctkintl.h"
#include "ctkmarshalers.h"
#include "ctkprivate.h"
#include "ctkwidgetprivate.h"
#include "ctkeventcontrollerprivate.h"
#include "ctkeventcontrollerkey.h"
#include "ctkbindings.h"

#include <cdk/cdk.h>

struct _CtkEventControllerKey
{
  CtkEventController parent_instance;
  CtkIMContext *im_context;
  GHashTable *pressed_keys;

  CdkModifierType state;

  const CdkEvent *current_event;
};

struct _CtkEventControllerKeyClass
{
  CtkEventControllerClass parent_class;
};

enum {
  KEY_PRESSED,
  KEY_RELEASED,
  MODIFIERS,
  IM_UPDATE,
  FOCUS_IN,
  FOCUS_OUT,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE (CtkEventControllerKey, ctk_event_controller_key,
               CTK_TYPE_EVENT_CONTROLLER)

static void
ctk_event_controller_finalize (GObject *object)
{
  CtkEventControllerKey *key = CTK_EVENT_CONTROLLER_KEY (object);

  g_hash_table_destroy (key->pressed_keys);
  g_clear_object (&key->im_context);

  G_OBJECT_CLASS (ctk_event_controller_key_parent_class)->finalize (object);
}

static gboolean
ctk_event_controller_key_handle_event (CtkEventController *controller,
                                       const CdkEvent     *event)
{
  CtkEventControllerKey *key = CTK_EVENT_CONTROLLER_KEY (controller);
  CdkEventType event_type = cdk_event_get_event_type (event);
  CdkModifierType state;
  guint16 keycode;
  guint keyval;
  gboolean handled = FALSE;

  if (event_type == CDK_FOCUS_CHANGE)
    {
      if (event->focus_change.in)
        g_signal_emit (controller, signals[FOCUS_IN], 0);
      else
        g_signal_emit (controller, signals[FOCUS_OUT], 0);

      return FALSE;
    }

  if (event_type != CDK_KEY_PRESS && event_type != CDK_KEY_RELEASE)
    return FALSE;

  if (key->im_context &&
      ctk_im_context_filter_keypress (key->im_context, (CdkEventKey *) event))
    {
      g_signal_emit (controller, signals[IM_UPDATE], 0);
      return TRUE;
    }

  key->current_event = event;

  cdk_event_get_state (event, &state);
  if (key->state != state)
    {
      gboolean unused;

      key->state = state;
      g_signal_emit (controller, signals[MODIFIERS], 0, state, &unused);
    }

  cdk_event_get_keycode (event, &keycode);
  cdk_event_get_keyval (event, &keyval);

  if (event_type == CDK_KEY_PRESS)
    {
      g_signal_emit (controller, signals[KEY_PRESSED], 0,
                     keyval, keycode, state, &handled);
      if (handled)
        g_hash_table_add (key->pressed_keys, GUINT_TO_POINTER (keyval));
    }
  else if (event_type == CDK_KEY_RELEASE)
    {
      g_signal_emit (controller, signals[KEY_RELEASED], 0,
                     keyval, keycode, state);

      handled = g_hash_table_lookup (key->pressed_keys, GUINT_TO_POINTER (keyval)) != NULL;
      g_hash_table_remove (key->pressed_keys, GUINT_TO_POINTER (keyval));
    }
  else
    handled = FALSE;

  key->current_event = NULL;

  return handled;
}

static void
ctk_event_controller_key_class_init (CtkEventControllerKeyClass *klass)
{
  CtkEventControllerClass *controller_class = CTK_EVENT_CONTROLLER_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = ctk_event_controller_finalize;
  controller_class->handle_event = ctk_event_controller_key_handle_event;

  /**
   * CtkEventControllerKey::key-pressed:
   * @controller: the object which received the signal.
   * @keyval: the pressed key.
   * @keycode: the raw code of the pressed key.
   * @state: the bitmask, representing the state of modifier keys and pointer buttons. See #CdkModifierType.
   *
   * This signal is emitted whenever a key is pressed.
   *
   * Returns: %TRUE if the key press was handled, %FALSE otherwise.
   *
   * Since: 3.24
   */
  signals[KEY_PRESSED] =
    g_signal_new (I_("key-pressed"),
                  CTK_TYPE_EVENT_CONTROLLER_KEY,
                  G_SIGNAL_RUN_LAST,
                  0, _ctk_boolean_handled_accumulator, NULL,
                  _ctk_marshal_BOOLEAN__UINT_UINT_FLAGS,
                  G_TYPE_BOOLEAN, 3, G_TYPE_UINT, G_TYPE_UINT, CDK_TYPE_MODIFIER_TYPE);
  g_signal_set_va_marshaller (signals[KEY_PRESSED],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__UINT_UINT_FLAGSv);

  /**
   * CtkEventControllerKey::key-released:
   * @controller: the object which received the signal.
   * @keyval: the released key.
   * @keycode: the raw code of the released key.
   * @state: the bitmask, representing the state of modifier keys and pointer buttons. See #CdkModifierType.
   *
   * This signal is emitted whenever a key is released.
   *
   * Since: 3.24
   */
  signals[KEY_RELEASED] =
    g_signal_new (I_("key-released"),
                  CTK_TYPE_EVENT_CONTROLLER_KEY,
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  _ctk_marshal_VOID__UINT_UINT_FLAGS,
                  G_TYPE_NONE, 3, G_TYPE_UINT, G_TYPE_UINT, CDK_TYPE_MODIFIER_TYPE);
  g_signal_set_va_marshaller (signals[KEY_RELEASED],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_VOID__UINT_UINT_FLAGSv);

  signals[MODIFIERS] =
    g_signal_new (I_("modifiers"),
                  CTK_TYPE_EVENT_CONTROLLER_KEY,
                  G_SIGNAL_RUN_LAST,
                  0, NULL,
                  NULL,
                  _ctk_marshal_BOOLEAN__FLAGS,
                  G_TYPE_BOOLEAN, 1, CDK_TYPE_MODIFIER_TYPE);
  g_signal_set_va_marshaller (signals[MODIFIERS],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__FLAGSv);

  signals[IM_UPDATE] =
    g_signal_new (I_("im-update"),
                  CTK_TYPE_EVENT_CONTROLLER_KEY,
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);
  signals[FOCUS_IN] =
    g_signal_new (I_("focus-in"),
                  CTK_TYPE_EVENT_CONTROLLER_KEY,
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);
  signals[FOCUS_OUT] =
    g_signal_new (I_("focus-out"),
                  CTK_TYPE_EVENT_CONTROLLER_KEY,
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);
}

static void
ctk_event_controller_key_init (CtkEventControllerKey *controller)
{
  controller->pressed_keys = g_hash_table_new (NULL, NULL);
}

CtkEventController *
ctk_event_controller_key_new (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  return g_object_new (CTK_TYPE_EVENT_CONTROLLER_KEY,
                       "widget", widget,
                       NULL);
}

void
ctk_event_controller_key_set_im_context (CtkEventControllerKey *controller,
                                         CtkIMContext          *im_context)
{
  g_return_if_fail (CTK_IS_EVENT_CONTROLLER_KEY (controller));
  g_return_if_fail (!im_context || CTK_IS_IM_CONTEXT (im_context));

  if (controller->im_context)
    ctk_im_context_reset (controller->im_context);

  g_set_object (&controller->im_context, im_context);
}

/**
 * ctk_event_controller_key_get_im_context:
 * @controller: a #CtkEventControllerKey
 *
 * Gets the IM context of a key controller.
 *
 * Returns: (transfer none): the IM context
 *
 * Since: 3.24
 **/
CtkIMContext *
ctk_event_controller_key_get_im_context (CtkEventControllerKey *controller)
{
  g_return_val_if_fail (CTK_IS_EVENT_CONTROLLER_KEY (controller), NULL);

  return controller->im_context;
}

gboolean
ctk_event_controller_key_forward (CtkEventControllerKey *controller,
                                  CtkWidget             *widget)
{
  g_return_val_if_fail (CTK_IS_EVENT_CONTROLLER_KEY (controller), FALSE);
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (controller->current_event != NULL, FALSE);

  if (!ctk_widget_get_realized (widget))
    ctk_widget_realize (widget);

  if (_ctk_widget_captured_event (widget, (CdkEvent *) controller->current_event))
    return TRUE;
  if (ctk_widget_event (widget, (CdkEvent *) controller->current_event))
    return TRUE;

  return FALSE;
}

guint
ctk_event_controller_key_get_group (CtkEventControllerKey *controller)
{
  g_return_val_if_fail (CTK_IS_EVENT_CONTROLLER_KEY (controller), FALSE);
  g_return_val_if_fail (controller->current_event != NULL, FALSE);

  return controller->current_event->key.group;
}
