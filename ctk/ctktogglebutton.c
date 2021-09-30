/* CTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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
 */

/*
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/. 
 */

#include "config.h"

#include "ctktogglebutton.h"

#include "ctkbuttonprivate.h"
#include "ctktogglebuttonprivate.h"
#include "ctklabel.h"
#include "ctkmain.h"
#include "ctkmarshalers.h"
#include "ctktoggleaction.h"
#include "ctkactivatable.h"
#include "ctkprivate.h"
#include "ctkintl.h"
#include "a11y/ctktogglebuttonaccessible.h"


/**
 * SECTION:ctktogglebutton
 * @Short_description: Create buttons which retain their state
 * @Title: CtkToggleButton
 * @See_also: #CtkButton, #CtkCheckButton, #CtkCheckMenuItem
 *
 * A #CtkToggleButton is a #CtkButton which will remain “pressed-in” when
 * clicked. Clicking again will cause the toggle button to return to its
 * normal state.
 *
 * A toggle button is created by calling either ctk_toggle_button_new() or
 * ctk_toggle_button_new_with_label(). If using the former, it is advisable to
 * pack a widget, (such as a #CtkLabel and/or a #CtkImage), into the toggle
 * button’s container. (See #CtkButton for more information).
 *
 * The state of a #CtkToggleButton can be set specifically using
 * ctk_toggle_button_set_active(), and retrieved using
 * ctk_toggle_button_get_active().
 *
 * To simply switch the state of a toggle button, use ctk_toggle_button_toggled().
 *
 * # CSS nodes
 *
 * CtkToggleButton has a single CSS node with name button. To differentiate
 * it from a plain #CtkButton, it gets the .toggle style class.
 *
 * ## Creating two #CtkToggleButton widgets.
 *
 * |[<!-- language="C" -->
 * static void output_state (CtkToggleButton *source, gpointer user_data) {
 *   printf ("Active: %d\n", ctk_toggle_button_get_active (source));
 * }
 *
 * void make_toggles (void) {
 *   CtkWidget *window, *toggle1, *toggle2;
 *   CtkWidget *box;
 *   const char *text;
 *
 *   window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
 *   box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 12);
 *
 *   text = "Hi, I’m a toggle button.";
 *   toggle1 = ctk_toggle_button_new_with_label (text);
 *
 *   // Makes this toggle button invisible
 *   ctk_toggle_button_set_mode (CTK_TOGGLE_BUTTON (toggle1),
 *                               TRUE);
 *
 *   g_signal_connect (toggle1, "toggled",
 *                     G_CALLBACK (output_state),
 *                     NULL);
 *   ctk_container_add (CTK_CONTAINER (box), toggle1);
 *
 *   text = "Hi, I’m a toggle button.";
 *   toggle2 = ctk_toggle_button_new_with_label (text);
 *   ctk_toggle_button_set_mode (CTK_TOGGLE_BUTTON (toggle2),
 *                               FALSE);
 *   g_signal_connect (toggle2, "toggled",
 *                     G_CALLBACK (output_state),
 *                     NULL);
 *   ctk_container_add (CTK_CONTAINER (box), toggle2);
 *
 *   ctk_container_add (CTK_CONTAINER (window), box);
 *   ctk_widget_show_all (window);
 * }
 * ]|
 */


#define DEFAULT_LEFT_POS  4
#define DEFAULT_TOP_POS   4
#define DEFAULT_SPACING   7

struct _CtkToggleButtonPrivate
{
  guint active         : 1;
  guint draw_indicator : 1;
  guint inconsistent   : 1;
};

enum {
  TOGGLED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_ACTIVE,
  PROP_INCONSISTENT,
  PROP_DRAW_INDICATOR,
  NUM_PROPERTIES
};

static GParamSpec *toggle_button_props[NUM_PROPERTIES] = { NULL, };

static gboolean ctk_toggle_button_mnemonic_activate  (CtkWidget            *widget,
                                                      gboolean              group_cycling);
static void ctk_toggle_button_clicked       (CtkButton            *button);
static void ctk_toggle_button_set_property  (GObject              *object,
					     guint                 prop_id,
					     const GValue         *value,
					     GParamSpec           *pspec);
static void ctk_toggle_button_get_property  (GObject              *object,
					     guint                 prop_id,
					     GValue               *value,
					     GParamSpec           *pspec);


static void ctk_toggle_button_activatable_interface_init (CtkActivatableIface  *iface);
static void ctk_toggle_button_update         	     (CtkActivatable       *activatable,
					 	      CtkAction            *action,
						      const gchar          *property_name);
static void ctk_toggle_button_sync_action_properties (CtkActivatable       *activatable,
						      CtkAction            *action);

static CtkActivatableIface *parent_activatable_iface;
static guint                toggle_button_signals[LAST_SIGNAL] = { 0 };

G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
G_DEFINE_TYPE_WITH_CODE (CtkToggleButton, ctk_toggle_button, CTK_TYPE_BUTTON,
                         G_ADD_PRIVATE (CtkToggleButton)
			 G_IMPLEMENT_INTERFACE (CTK_TYPE_ACTIVATABLE,
						ctk_toggle_button_activatable_interface_init))
G_GNUC_END_IGNORE_DEPRECATIONS;

static void
ctk_toggle_button_class_init (CtkToggleButtonClass *class)
{
  GObjectClass *gobject_class;
  CtkWidgetClass *widget_class;
  CtkButtonClass *button_class;

  gobject_class = G_OBJECT_CLASS (class);
  widget_class = (CtkWidgetClass*) class;
  button_class = (CtkButtonClass*) class;

  gobject_class->set_property = ctk_toggle_button_set_property;
  gobject_class->get_property = ctk_toggle_button_get_property;

  widget_class->mnemonic_activate = ctk_toggle_button_mnemonic_activate;

  button_class->clicked = ctk_toggle_button_clicked;

  class->toggled = NULL;

  toggle_button_props[PROP_ACTIVE] =
      g_param_spec_boolean ("active",
                            P_("Active"),
                            P_("If the toggle button should be pressed in"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  toggle_button_props[PROP_INCONSISTENT] =
      g_param_spec_boolean ("inconsistent",
                            P_("Inconsistent"),
                            P_("If the toggle button is in an \"in between\" state"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  toggle_button_props[PROP_DRAW_INDICATOR] =
      g_param_spec_boolean ("draw-indicator",
                            P_("Draw Indicator"),
                            P_("If the toggle part of the button is displayed"),
                            FALSE,
                            CTK_PARAM_READWRITE);

  g_object_class_install_properties (gobject_class, NUM_PROPERTIES, toggle_button_props);

  /**
   * CtkToggleButton::toggled:
   * @togglebutton: the object which received the signal.
   *
   * Should be connected if you wish to perform an action whenever the
   * #CtkToggleButton's state is changed.
   */
  toggle_button_signals[TOGGLED] =
    g_signal_new (I_("toggled"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (CtkToggleButtonClass, toggled),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_TOGGLE_BUTTON_ACCESSIBLE);
  ctk_widget_class_set_css_name (widget_class, "button");
}

static void
ctk_toggle_button_init (CtkToggleButton *toggle_button)
{
  CtkStyleContext *context;

  toggle_button->priv = ctk_toggle_button_get_instance_private (toggle_button);
  toggle_button->priv->active = FALSE;
  toggle_button->priv->draw_indicator = FALSE;

  context = ctk_widget_get_style_context (CTK_WIDGET (toggle_button));
  ctk_style_context_add_class (context, "toggle");
}

static void
ctk_toggle_button_activatable_interface_init (CtkActivatableIface *iface)
{
  parent_activatable_iface = g_type_interface_peek_parent (iface);
  iface->update = ctk_toggle_button_update;
  iface->sync_action_properties = ctk_toggle_button_sync_action_properties;
}

static void
ctk_toggle_button_update (CtkActivatable *activatable,
			  CtkAction      *action,
			  const gchar    *property_name)
{
  CtkToggleButton *button;

  parent_activatable_iface->update (activatable, action, property_name);

  button = CTK_TOGGLE_BUTTON (activatable);

  if (strcmp (property_name, "active") == 0)
    {
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      ctk_action_block_activate (action);
      ctk_toggle_button_set_active (button, ctk_toggle_action_get_active (CTK_TOGGLE_ACTION (action)));
      ctk_action_unblock_activate (action);
      G_GNUC_END_IGNORE_DEPRECATIONS;
    }

}

static void
ctk_toggle_button_sync_action_properties (CtkActivatable *activatable,
				          CtkAction      *action)
{
  CtkToggleButton *button;
  gboolean is_toggle_action;

  parent_activatable_iface->sync_action_properties (activatable, action);

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  is_toggle_action = CTK_IS_TOGGLE_ACTION (action);
  G_GNUC_END_IGNORE_DEPRECATIONS;

  if (!is_toggle_action)
    return;

  button = CTK_TOGGLE_BUTTON (activatable);

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  ctk_action_block_activate (action);
  ctk_toggle_button_set_active (button, ctk_toggle_action_get_active (CTK_TOGGLE_ACTION (action)));
  ctk_action_unblock_activate (action);
  G_GNUC_END_IGNORE_DEPRECATIONS;
}

/**
 * ctk_toggle_button_new:
 *
 * Creates a new toggle button. A widget should be packed into the button, as in ctk_button_new().
 *
 * Returns: a new toggle button.
 */
CtkWidget*
ctk_toggle_button_new (void)
{
  return g_object_new (CTK_TYPE_TOGGLE_BUTTON, NULL);
}

/**
 * ctk_toggle_button_new_with_label:
 * @label: a string containing the message to be placed in the toggle button.
 *
 * Creates a new toggle button with a text label.
 *
 * Returns: a new toggle button.
 */
CtkWidget*
ctk_toggle_button_new_with_label (const gchar *label)
{
  return g_object_new (CTK_TYPE_TOGGLE_BUTTON, "label", label, NULL);
}

/**
 * ctk_toggle_button_new_with_mnemonic:
 * @label: the text of the button, with an underscore in front of the
 *         mnemonic character
 *
 * Creates a new #CtkToggleButton containing a label. The label
 * will be created using ctk_label_new_with_mnemonic(), so underscores
 * in @label indicate the mnemonic for the button.
 *
 * Returns: a new #CtkToggleButton
 */
CtkWidget*
ctk_toggle_button_new_with_mnemonic (const gchar *label)
{
  return g_object_new (CTK_TYPE_TOGGLE_BUTTON, 
		       "label", label, 
		       "use-underline", TRUE, 
		       NULL);
}

static void
ctk_toggle_button_set_property (GObject      *object,
				guint         prop_id,
				const GValue *value,
				GParamSpec   *pspec)
{
  CtkToggleButton *tb;

  tb = CTK_TOGGLE_BUTTON (object);

  switch (prop_id)
    {
    case PROP_ACTIVE:
      ctk_toggle_button_set_active (tb, g_value_get_boolean (value));
      break;
    case PROP_INCONSISTENT:
      ctk_toggle_button_set_inconsistent (tb, g_value_get_boolean (value));
      break;
    case PROP_DRAW_INDICATOR:
      ctk_toggle_button_set_mode (tb, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_toggle_button_get_property (GObject      *object,
				guint         prop_id,
				GValue       *value,
				GParamSpec   *pspec)
{
  CtkToggleButton *tb = CTK_TOGGLE_BUTTON (object);
  CtkToggleButtonPrivate *priv = tb->priv;

  switch (prop_id)
    {
    case PROP_ACTIVE:
      g_value_set_boolean (value, priv->active);
      break;
    case PROP_INCONSISTENT:
      g_value_set_boolean (value, priv->inconsistent);
      break;
    case PROP_DRAW_INDICATOR:
      g_value_set_boolean (value, priv->draw_indicator);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * ctk_toggle_button_set_mode:
 * @toggle_button: a #CtkToggleButton
 * @draw_indicator: if %TRUE, draw the button as a separate indicator
 * and label; if %FALSE, draw the button like a normal button
 *
 * Sets whether the button is displayed as a separate indicator and label.
 * You can call this function on a checkbutton or a radiobutton with
 * @draw_indicator = %FALSE to make the button look like a normal button.
 *
 * This can be used to create linked strip of buttons that work like
 * a #CtkStackSwitcher.
 *
 * This function only affects instances of classes like #CtkCheckButton
 * and #CtkRadioButton that derive from #CtkToggleButton,
 * not instances of #CtkToggleButton itself.
 */
void
ctk_toggle_button_set_mode (CtkToggleButton *toggle_button,
			    gboolean         draw_indicator)
{
  CtkToggleButtonPrivate *priv;

  g_return_if_fail (CTK_IS_TOGGLE_BUTTON (toggle_button));

  priv = toggle_button->priv;

  draw_indicator = draw_indicator ? TRUE : FALSE;

  if (priv->draw_indicator != draw_indicator)
    {
      priv->draw_indicator = draw_indicator;

      if (ctk_widget_get_visible (CTK_WIDGET (toggle_button)))
	ctk_widget_queue_resize (CTK_WIDGET (toggle_button));

      g_object_notify_by_pspec (G_OBJECT (toggle_button), toggle_button_props[PROP_DRAW_INDICATOR]);
    }
}

/**
 * ctk_toggle_button_get_mode:
 * @toggle_button: a #CtkToggleButton
 *
 * Retrieves whether the button is displayed as a separate indicator
 * and label. See ctk_toggle_button_set_mode().
 *
 * Returns: %TRUE if the togglebutton is drawn as a separate indicator
 *   and label.
 **/
gboolean
ctk_toggle_button_get_mode (CtkToggleButton *toggle_button)
{
  g_return_val_if_fail (CTK_IS_TOGGLE_BUTTON (toggle_button), FALSE);

  return toggle_button->priv->draw_indicator;
}

/**
 * ctk_toggle_button_set_active:
 * @toggle_button: a #CtkToggleButton.
 * @is_active: %TRUE or %FALSE.
 *
 * Sets the status of the toggle button. Set to %TRUE if you want the
 * CtkToggleButton to be “pressed in”, and %FALSE to raise it.
 * This action causes the #CtkToggleButton::toggled signal and the
 * #CtkButton::clicked signal to be emitted.
 */
void
ctk_toggle_button_set_active (CtkToggleButton *toggle_button,
			      gboolean         is_active)
{
  CtkToggleButtonPrivate *priv;

  g_return_if_fail (CTK_IS_TOGGLE_BUTTON (toggle_button));

  priv = toggle_button->priv;

  is_active = is_active != FALSE;

  if (priv->active != is_active)
    {
      ctk_button_clicked (CTK_BUTTON (toggle_button));
      g_object_notify_by_pspec (G_OBJECT (toggle_button), toggle_button_props[PROP_ACTIVE]);
    }
}

void
_ctk_toggle_button_set_active (CtkToggleButton *toggle_button,
                               gboolean         is_active)
{
  toggle_button->priv->active = is_active;

  if (is_active)
    ctk_widget_set_state_flags (CTK_WIDGET (toggle_button), CTK_STATE_FLAG_CHECKED, FALSE);
  else
    ctk_widget_unset_state_flags (CTK_WIDGET (toggle_button), CTK_STATE_FLAG_CHECKED);

}

/**
 * ctk_toggle_button_get_active:
 * @toggle_button: a #CtkToggleButton.
 *
 * Queries a #CtkToggleButton and returns its current state. Returns %TRUE if
 * the toggle button is pressed in and %FALSE if it is raised.
 *
 * Returns: a #gboolean value.
 */
gboolean
ctk_toggle_button_get_active (CtkToggleButton *toggle_button)
{
  g_return_val_if_fail (CTK_IS_TOGGLE_BUTTON (toggle_button), FALSE);

  return toggle_button->priv->active;
}

/**
 * ctk_toggle_button_toggled:
 * @toggle_button: a #CtkToggleButton.
 *
 * Emits the #CtkToggleButton::toggled signal on the
 * #CtkToggleButton. There is no good reason for an
 * application ever to call this function.
 */
void
ctk_toggle_button_toggled (CtkToggleButton *toggle_button)
{
  g_return_if_fail (CTK_IS_TOGGLE_BUTTON (toggle_button));

  g_signal_emit (toggle_button, toggle_button_signals[TOGGLED], 0);
}

/**
 * ctk_toggle_button_set_inconsistent:
 * @toggle_button: a #CtkToggleButton
 * @setting: %TRUE if state is inconsistent
 *
 * If the user has selected a range of elements (such as some text or
 * spreadsheet cells) that are affected by a toggle button, and the
 * current values in that range are inconsistent, you may want to
 * display the toggle in an “in between” state. This function turns on
 * “in between” display.  Normally you would turn off the inconsistent
 * state again if the user toggles the toggle button. This has to be
 * done manually, ctk_toggle_button_set_inconsistent() only affects
 * visual appearance, it doesn’t affect the semantics of the button.
 * 
 **/
void
ctk_toggle_button_set_inconsistent (CtkToggleButton *toggle_button,
                                    gboolean         setting)
{
  CtkToggleButtonPrivate *priv;

  g_return_if_fail (CTK_IS_TOGGLE_BUTTON (toggle_button));

  priv = toggle_button->priv;

  setting = setting != FALSE;

  if (setting != priv->inconsistent)
    {
      priv->inconsistent = setting;

      if (setting)
        ctk_widget_set_state_flags (CTK_WIDGET (toggle_button), CTK_STATE_FLAG_INCONSISTENT, FALSE);
      else
        ctk_widget_unset_state_flags (CTK_WIDGET (toggle_button), CTK_STATE_FLAG_INCONSISTENT);

      g_object_notify_by_pspec (G_OBJECT (toggle_button), toggle_button_props[PROP_INCONSISTENT]);
    }
}

/**
 * ctk_toggle_button_get_inconsistent:
 * @toggle_button: a #CtkToggleButton
 * 
 * Gets the value set by ctk_toggle_button_set_inconsistent().
 * 
 * Returns: %TRUE if the button is displayed as inconsistent, %FALSE otherwise
 **/
gboolean
ctk_toggle_button_get_inconsistent (CtkToggleButton *toggle_button)
{
  g_return_val_if_fail (CTK_IS_TOGGLE_BUTTON (toggle_button), FALSE);

  return toggle_button->priv->inconsistent;
}

static gboolean
ctk_toggle_button_mnemonic_activate (CtkWidget *widget,
                                     gboolean   group_cycling)
{
  /*
   * We override the standard implementation in 
   * ctk_widget_real_mnemonic_activate() in order to focus the widget even
   * if there is no mnemonic conflict.
   */
  if (ctk_widget_get_can_focus (widget))
    ctk_widget_grab_focus (widget);

  if (!group_cycling)
    ctk_widget_activate (widget);

  return TRUE;
}

static void
ctk_toggle_button_clicked (CtkButton *button)
{
  CtkToggleButton *toggle_button = CTK_TOGGLE_BUTTON (button);
  CtkToggleButtonPrivate *priv = toggle_button->priv;

  _ctk_toggle_button_set_active (toggle_button, !priv->active);

  ctk_toggle_button_toggled (toggle_button);

  g_object_notify_by_pspec (G_OBJECT (toggle_button), toggle_button_props[PROP_ACTIVE]);

  if (CTK_BUTTON_CLASS (ctk_toggle_button_parent_class)->clicked)
    CTK_BUTTON_CLASS (ctk_toggle_button_parent_class)->clicked (button);
}

