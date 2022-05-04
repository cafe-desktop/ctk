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
 * Modified by the CTK+ Team and others 1997-2001.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#include "config.h"
#include "ctkbuiltiniconprivate.h"
#include "ctkcheckmenuitem.h"
#include "ctkmenuitemprivate.h"
#include "ctkaccellabel.h"
#include "ctkactivatable.h"
#include "ctktoggleaction.h"
#include "ctkmarshalers.h"
#include "ctkprivate.h"
#include "ctkintl.h"
#include "a11y/ctkcheckmenuitemaccessible.h"
#include "ctkcssnodeprivate.h"
#include "ctkcssstylepropertyprivate.h"
#include "ctkwidgetprivate.h"

/**
 * SECTION:ctkcheckmenuitem
 * @Short_description: A menu item with a check box
 * @Title: CtkCheckMenuItem
 *
 * A #CtkCheckMenuItem is a menu item that maintains the state of a boolean
 * value in addition to a #CtkMenuItem usual role in activating application
 * code.
 *
 * A check box indicating the state of the boolean value is displayed
 * at the left side of the #CtkMenuItem.  Activating the #CtkMenuItem
 * toggles the value.
 *
 * # CSS nodes
 *
 * |[<!-- language="plain" -->
 * menuitem
 * ├── check.left
 * ╰── <child>
 * ]|
 *
 * CtkCheckMenuItem has a main CSS node with name menuitem, and a subnode
 * with name check, which gets the .left or .right style class.
 */


#define INDICATOR_SIZE 16

struct _CtkCheckMenuItemPrivate
{
  CtkCssGadget *indicator_gadget;

  guint active             : 1;
  guint draw_as_radio      : 1;
  guint inconsistent       : 1;
};

enum {
  TOGGLED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_ACTIVE,
  PROP_INCONSISTENT,
  PROP_DRAW_AS_RADIO
};

static gint ctk_check_menu_item_draw                 (CtkWidget             *widget,
                                                      cairo_t               *cr);
static void ctk_check_menu_item_activate             (CtkMenuItem           *menu_item);
static void ctk_check_menu_item_toggle_size_request  (CtkMenuItem           *menu_item,
                                                      gint                  *requisition);
static void ctk_real_check_menu_item_draw_indicator  (CtkCheckMenuItem      *check_menu_item,
                                                      cairo_t               *cr);
static void ctk_check_menu_item_set_property         (GObject               *object,
                                                      guint                  prop_id,
                                                      const GValue          *value,
                                                      GParamSpec            *pspec);
static void ctk_check_menu_item_get_property         (GObject               *object,
                                                      guint                  prop_id,
                                                      GValue                *value,
                                                      GParamSpec            *pspec);

static void ctk_check_menu_item_state_flags_changed (CtkWidget        *widget,
                                                     CtkStateFlags     previous_state);
static void ctk_check_menu_item_direction_changed   (CtkWidget        *widget,
                                                     CtkTextDirection  previous_dir);

static void ctk_check_menu_item_activatable_interface_init (CtkActivatableIface  *iface);
static void ctk_check_menu_item_update                     (CtkActivatable       *activatable,
                                                            CtkAction            *action,
                                                            const gchar          *property_name);
static void ctk_check_menu_item_sync_action_properties     (CtkActivatable       *activatable,
                                                            CtkAction            *action);

static CtkActivatableIface *parent_activatable_iface;
static guint                check_menu_item_signals[LAST_SIGNAL] = { 0 };

G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
G_DEFINE_TYPE_WITH_CODE (CtkCheckMenuItem, ctk_check_menu_item, CTK_TYPE_MENU_ITEM,
                         G_ADD_PRIVATE (CtkCheckMenuItem)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_ACTIVATABLE,
                                                ctk_check_menu_item_activatable_interface_init))
G_GNUC_END_IGNORE_DEPRECATIONS;

static void
ctk_check_menu_item_size_allocate (CtkWidget     *widget,
                                   CtkAllocation *allocation)
{
  CtkAllocation clip, widget_clip;
  CtkAllocation content_alloc, indicator_alloc;
  CtkCssGadget *menu_item_gadget;
  CtkCheckMenuItem *check_menu_item = CTK_CHECK_MENU_ITEM (widget);
  CtkCheckMenuItemPrivate *priv = check_menu_item->priv;
  gint content_baseline, toggle_size;

  CTK_WIDGET_CLASS (ctk_check_menu_item_parent_class)->size_allocate
    (widget, allocation);

  menu_item_gadget = _ctk_menu_item_get_gadget (CTK_MENU_ITEM (widget));
  ctk_css_gadget_get_content_allocation (menu_item_gadget,
                                         &content_alloc, &content_baseline);

  ctk_css_gadget_get_preferred_size (priv->indicator_gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     &indicator_alloc.width, NULL,
                                     NULL, NULL);
  ctk_css_gadget_get_preferred_size (priv->indicator_gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     -1,
                                     &indicator_alloc.height, NULL,
                                     NULL, NULL);
  toggle_size = CTK_MENU_ITEM (check_menu_item)->priv->toggle_size;

  if (ctk_widget_get_direction (widget) == CTK_TEXT_DIR_LTR)
    indicator_alloc.x = content_alloc.x +
      (toggle_size - indicator_alloc.width) / 2;
  else
    indicator_alloc.x = content_alloc.x + content_alloc.width - toggle_size +
      (toggle_size - indicator_alloc.width) / 2;

  indicator_alloc.y = content_alloc.y +
    (content_alloc.height - indicator_alloc.height) / 2;

  ctk_css_gadget_allocate (check_menu_item->priv->indicator_gadget,
                           &indicator_alloc,
                           content_baseline,
                           &clip);

  ctk_widget_get_clip (widget, &widget_clip);
  cdk_rectangle_union (&widget_clip, &clip, &widget_clip);
  ctk_widget_set_clip (widget, &widget_clip);
}

static void
ctk_check_menu_item_finalize (GObject *object)
{
  CtkCheckMenuItemPrivate *priv = CTK_CHECK_MENU_ITEM (object)->priv;

  g_clear_object (&priv->indicator_gadget);

  G_OBJECT_CLASS (ctk_check_menu_item_parent_class)->finalize (object);
}

static void
ctk_check_menu_item_class_init (CtkCheckMenuItemClass *klass)
{
  GObjectClass *gobject_class;
  CtkWidgetClass *widget_class;
  CtkMenuItemClass *menu_item_class;
  
  gobject_class = G_OBJECT_CLASS (klass);
  widget_class = (CtkWidgetClass*) klass;
  menu_item_class = (CtkMenuItemClass*) klass;
  
  gobject_class->set_property = ctk_check_menu_item_set_property;
  gobject_class->get_property = ctk_check_menu_item_get_property;
  gobject_class->finalize = ctk_check_menu_item_finalize;

  widget_class->size_allocate = ctk_check_menu_item_size_allocate;
  widget_class->state_flags_changed = ctk_check_menu_item_state_flags_changed;
  widget_class->direction_changed = ctk_check_menu_item_direction_changed;

  g_object_class_install_property (gobject_class,
                                   PROP_ACTIVE,
                                   g_param_spec_boolean ("active",
                                                         P_("Active"),
                                                         P_("Whether the menu item is checked"),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
  
  g_object_class_install_property (gobject_class,
                                   PROP_INCONSISTENT,
                                   g_param_spec_boolean ("inconsistent",
                                                         P_("Inconsistent"),
                                                         P_("Whether to display an \"inconsistent\" state"),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
  
  g_object_class_install_property (gobject_class,
                                   PROP_DRAW_AS_RADIO,
                                   g_param_spec_boolean ("draw-as-radio",
                                                         P_("Draw as radio menu item"),
                                                         P_("Whether the menu item looks like a radio menu item"),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkCheckMenuItem:indicator-size:
   *
   * The size of the check or radio indicator.
   *
   * Deprecated: 3.20: Use the standard CSS property min-width on the check or
   *   radio nodes; the value of this style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("indicator-size",
                                                             P_("Indicator Size"),
                                                             P_("Size of check or radio indicator"),
                                                             0,
                                                             G_MAXINT,
                                                             INDICATOR_SIZE,
                                                             CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  widget_class->draw = ctk_check_menu_item_draw;

  menu_item_class->activate = ctk_check_menu_item_activate;
  menu_item_class->hide_on_activate = FALSE;
  menu_item_class->toggle_size_request = ctk_check_menu_item_toggle_size_request;
  
  klass->toggled = NULL;
  klass->draw_indicator = ctk_real_check_menu_item_draw_indicator;

  /**
   * CtkCheckMenuItem::toggled:
   * @checkmenuitem: the object which received the signal.
   *
   * This signal is emitted when the state of the check box is changed.
   *
   * A signal handler can use ctk_check_menu_item_get_active()
   * to discover the new state.
   */
  check_menu_item_signals[TOGGLED] =
    g_signal_new (I_("toggled"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (CtkCheckMenuItemClass, toggled),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_CHECK_MENU_ITEM_ACCESSIBLE);
  ctk_widget_class_set_css_name (widget_class, "menuitem");
}

static void 
ctk_check_menu_item_activatable_interface_init (CtkActivatableIface  *iface)
{
  parent_activatable_iface = g_type_interface_peek_parent (iface);
  iface->update = ctk_check_menu_item_update;
  iface->sync_action_properties = ctk_check_menu_item_sync_action_properties;
}

static void
ctk_check_menu_item_update (CtkActivatable *activatable,
                            CtkAction      *action,
                            const gchar    *property_name)
{
  CtkCheckMenuItem *check_menu_item;
  gboolean use_action_appearance;

  check_menu_item = CTK_CHECK_MENU_ITEM (activatable);

  parent_activatable_iface->update (activatable, action, property_name);

  if (strcmp (property_name, "active") == 0)
    {
      ctk_action_block_activate (action);
      ctk_check_menu_item_set_active (check_menu_item, ctk_toggle_action_get_active (CTK_TOGGLE_ACTION (action)));
      ctk_action_unblock_activate (action);
    }

  use_action_appearance = ctk_activatable_get_use_action_appearance (activatable);

  if (!use_action_appearance)
    return;

  if (strcmp (property_name, "draw-as-radio") == 0)
    {
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      ctk_check_menu_item_set_draw_as_radio (check_menu_item,
                                             ctk_toggle_action_get_draw_as_radio (CTK_TOGGLE_ACTION (action)));
      G_GNUC_END_IGNORE_DEPRECATIONS;
    }
}

static void
ctk_check_menu_item_sync_action_properties (CtkActivatable *activatable,
                                            CtkAction      *action)
{
  CtkCheckMenuItem *check_menu_item;
  gboolean use_action_appearance;
  gboolean is_toggle_action;

  check_menu_item = CTK_CHECK_MENU_ITEM (activatable);

  parent_activatable_iface->sync_action_properties (activatable, action);

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  is_toggle_action = CTK_IS_TOGGLE_ACTION (action);
  G_GNUC_END_IGNORE_DEPRECATIONS;

  if (!is_toggle_action)
    return;

  ctk_action_block_activate (action);

  ctk_check_menu_item_set_active (check_menu_item, ctk_toggle_action_get_active (CTK_TOGGLE_ACTION (action)));
  ctk_action_unblock_activate (action);
  use_action_appearance = ctk_activatable_get_use_action_appearance (activatable);

  if (!use_action_appearance)
    return;

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  ctk_check_menu_item_set_draw_as_radio (check_menu_item,
                                         ctk_toggle_action_get_draw_as_radio (CTK_TOGGLE_ACTION (action)));
  G_GNUC_END_IGNORE_DEPRECATIONS;
}

/**
 * ctk_check_menu_item_new:
 *
 * Creates a new #CtkCheckMenuItem.
 *
 * Returns: a new #CtkCheckMenuItem.
 */
CtkWidget*
ctk_check_menu_item_new (void)
{
  return g_object_new (CTK_TYPE_CHECK_MENU_ITEM, NULL);
}

/**
 * ctk_check_menu_item_new_with_label:
 * @label: the string to use for the label.
 *
 * Creates a new #CtkCheckMenuItem with a label.
 *
 * Returns: a new #CtkCheckMenuItem.
 */
CtkWidget*
ctk_check_menu_item_new_with_label (const gchar *label)
{
  return g_object_new (CTK_TYPE_CHECK_MENU_ITEM, 
                       "label", label,
                       NULL);
}


/**
 * ctk_check_menu_item_new_with_mnemonic:
 * @label: The text of the button, with an underscore in front of the
 *     character
 *
 * Creates a new #CtkCheckMenuItem containing a label. The label
 * will be created using ctk_label_new_with_mnemonic(), so underscores
 * in @label indicate the mnemonic for the menu item.
 *
 * Returns: a new #CtkCheckMenuItem
 */
CtkWidget*
ctk_check_menu_item_new_with_mnemonic (const gchar *label)
{
  return g_object_new (CTK_TYPE_CHECK_MENU_ITEM, 
                       "label", label,
                       "use-underline", TRUE,
                       NULL);
}

/**
 * ctk_check_menu_item_set_active:
 * @check_menu_item: a #CtkCheckMenuItem.
 * @is_active: boolean value indicating whether the check box is active.
 *
 * Sets the active state of the menu item’s check box.
 */
void
ctk_check_menu_item_set_active (CtkCheckMenuItem *check_menu_item,
                                gboolean          is_active)
{
  CtkCheckMenuItemPrivate *priv;

  g_return_if_fail (CTK_IS_CHECK_MENU_ITEM (check_menu_item));

  priv = check_menu_item->priv;

  is_active = is_active != 0;

  if (priv->active != is_active)
    ctk_menu_item_activate (CTK_MENU_ITEM (check_menu_item));
}

/**
 * ctk_check_menu_item_get_active:
 * @check_menu_item: a #CtkCheckMenuItem
 * 
 * Returns whether the check menu item is active. See
 * ctk_check_menu_item_set_active ().
 * 
 * Returns: %TRUE if the menu item is checked.
 */
gboolean
ctk_check_menu_item_get_active (CtkCheckMenuItem *check_menu_item)
{
  g_return_val_if_fail (CTK_IS_CHECK_MENU_ITEM (check_menu_item), FALSE);

  return check_menu_item->priv->active;
}

static void
ctk_check_menu_item_toggle_size_request (CtkMenuItem *menu_item,
                                         gint        *requisition)
{
  CtkCheckMenuItem *check_menu_item;

  g_return_if_fail (CTK_IS_CHECK_MENU_ITEM (menu_item));

  check_menu_item = CTK_CHECK_MENU_ITEM (menu_item);
  ctk_css_gadget_get_preferred_size (check_menu_item->priv->indicator_gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     requisition, NULL,
                                     NULL, NULL);
}

/**
 * ctk_check_menu_item_toggled:
 * @check_menu_item: a #CtkCheckMenuItem.
 *
 * Emits the #CtkCheckMenuItem::toggled signal.
 */
void
ctk_check_menu_item_toggled (CtkCheckMenuItem *check_menu_item)
{
  g_signal_emit (check_menu_item, check_menu_item_signals[TOGGLED], 0);
}

static void
update_node_state (CtkCheckMenuItem *check_menu_item)
{
  CtkCheckMenuItemPrivate *priv = check_menu_item->priv;
  CtkStateFlags state;

  state = ctk_widget_get_state_flags (CTK_WIDGET (check_menu_item));

  if (priv->inconsistent)
    state |= CTK_STATE_FLAG_INCONSISTENT;
  if (priv->active)
    state |= CTK_STATE_FLAG_CHECKED;

  ctk_css_gadget_set_state (priv->indicator_gadget, state);
}

/**
 * ctk_check_menu_item_set_inconsistent:
 * @check_menu_item: a #CtkCheckMenuItem
 * @setting: %TRUE to display an “inconsistent” third state check
 *
 * If the user has selected a range of elements (such as some text or
 * spreadsheet cells) that are affected by a boolean setting, and the
 * current values in that range are inconsistent, you may want to
 * display the check in an “in between” state. This function turns on
 * “in between” display.  Normally you would turn off the inconsistent
 * state again if the user explicitly selects a setting. This has to be
 * done manually, ctk_check_menu_item_set_inconsistent() only affects
 * visual appearance, it doesn’t affect the semantics of the widget.
 * 
 **/
void
ctk_check_menu_item_set_inconsistent (CtkCheckMenuItem *check_menu_item,
                                      gboolean          setting)
{
  CtkCheckMenuItemPrivate *priv;

  g_return_if_fail (CTK_IS_CHECK_MENU_ITEM (check_menu_item));

  priv = check_menu_item->priv;
  
  setting = setting != FALSE;

  if (setting != priv->inconsistent)
    {
      priv->inconsistent = setting;
      update_node_state (check_menu_item);
      ctk_widget_queue_draw (CTK_WIDGET (check_menu_item));
      g_object_notify (G_OBJECT (check_menu_item), "inconsistent");
    }
}

/**
 * ctk_check_menu_item_get_inconsistent:
 * @check_menu_item: a #CtkCheckMenuItem
 * 
 * Retrieves the value set by ctk_check_menu_item_set_inconsistent().
 * 
 * Returns: %TRUE if inconsistent
 **/
gboolean
ctk_check_menu_item_get_inconsistent (CtkCheckMenuItem *check_menu_item)
{
  g_return_val_if_fail (CTK_IS_CHECK_MENU_ITEM (check_menu_item), FALSE);

  return check_menu_item->priv->inconsistent;
}

/**
 * ctk_check_menu_item_set_draw_as_radio:
 * @check_menu_item: a #CtkCheckMenuItem
 * @draw_as_radio: whether @check_menu_item is drawn like a #CtkRadioMenuItem
 *
 * Sets whether @check_menu_item is drawn like a #CtkRadioMenuItem
 *
 * Since: 2.4
 **/
void
ctk_check_menu_item_set_draw_as_radio (CtkCheckMenuItem *check_menu_item,
                                       gboolean          draw_as_radio)
{
  CtkCheckMenuItemPrivate *priv;
  CtkCssNode *indicator_node;

  g_return_if_fail (CTK_IS_CHECK_MENU_ITEM (check_menu_item));

  priv = check_menu_item->priv;

  draw_as_radio = draw_as_radio != FALSE;

  if (draw_as_radio != priv->draw_as_radio)
    {
      priv->draw_as_radio = draw_as_radio;
      indicator_node = ctk_css_gadget_get_node (priv->indicator_gadget);
      if (draw_as_radio)
        ctk_css_node_set_name (indicator_node, I_("radio"));
      else
        ctk_css_node_set_name (indicator_node, I_("check"));

      ctk_widget_queue_draw (CTK_WIDGET (check_menu_item));

      g_object_notify (G_OBJECT (check_menu_item), "draw-as-radio");
    }
}

/**
 * ctk_check_menu_item_get_draw_as_radio:
 * @check_menu_item: a #CtkCheckMenuItem
 * 
 * Returns whether @check_menu_item looks like a #CtkRadioMenuItem
 * 
 * Returns: Whether @check_menu_item looks like a #CtkRadioMenuItem
 * 
 * Since: 2.4
 **/
gboolean
ctk_check_menu_item_get_draw_as_radio (CtkCheckMenuItem *check_menu_item)
{
  g_return_val_if_fail (CTK_IS_CHECK_MENU_ITEM (check_menu_item), FALSE);
  
  return check_menu_item->priv->draw_as_radio;
}

static void
ctk_check_menu_item_init (CtkCheckMenuItem *check_menu_item)
{
  CtkCheckMenuItemPrivate *priv;

  priv = check_menu_item->priv = ctk_check_menu_item_get_instance_private (check_menu_item);
  priv->active = FALSE;

  priv->indicator_gadget =
    ctk_builtin_icon_new ("check",
                          CTK_WIDGET (check_menu_item),
                          _ctk_menu_item_get_gadget (CTK_MENU_ITEM (check_menu_item)),
                          NULL);
  update_node_state (check_menu_item);
}

static gint
ctk_check_menu_item_draw (CtkWidget *widget,
                          cairo_t   *cr)
{
  CtkCheckMenuItem *check_menu_item = CTK_CHECK_MENU_ITEM (widget);

  if (CTK_WIDGET_CLASS (ctk_check_menu_item_parent_class)->draw)
    CTK_WIDGET_CLASS (ctk_check_menu_item_parent_class)->draw (widget, cr);

  if (CTK_CHECK_MENU_ITEM_GET_CLASS (check_menu_item)->draw_indicator)
    CTK_CHECK_MENU_ITEM_GET_CLASS (check_menu_item)->draw_indicator (check_menu_item, cr);

  return FALSE;
}

static void
ctk_check_menu_item_activate (CtkMenuItem *menu_item)
{
  CtkCheckMenuItemPrivate *priv;

  CtkCheckMenuItem *check_menu_item = CTK_CHECK_MENU_ITEM (menu_item);
  priv = check_menu_item->priv;

  priv->active = !priv->active;

  ctk_check_menu_item_toggled (check_menu_item);
  update_node_state (check_menu_item);
  ctk_widget_queue_draw (CTK_WIDGET (check_menu_item));

  CTK_MENU_ITEM_CLASS (ctk_check_menu_item_parent_class)->activate (menu_item);

  g_object_notify (G_OBJECT (check_menu_item), "active");
}

static void
ctk_check_menu_item_state_flags_changed (CtkWidget     *widget,
                                         CtkStateFlags  previous_state)

{
  CtkCheckMenuItem *check_menu_item = CTK_CHECK_MENU_ITEM (widget);

  update_node_state (check_menu_item);

  CTK_WIDGET_CLASS (ctk_check_menu_item_parent_class)->state_flags_changed (widget, previous_state);
}

static void
ctk_check_menu_item_direction_changed (CtkWidget        *widget,
                                       CtkTextDirection  previous_dir)
{
  CtkCheckMenuItem *check_menu_item = CTK_CHECK_MENU_ITEM (widget);
  CtkCheckMenuItemPrivate *priv = check_menu_item->priv;
  CtkCssNode *indicator_node, *widget_node, *node;

  indicator_node = ctk_css_gadget_get_node (priv->indicator_gadget);
  widget_node = ctk_widget_get_css_node (widget);

  if (ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL)
    {
      ctk_css_node_remove_class (indicator_node, g_quark_from_static_string (CTK_STYLE_CLASS_LEFT));
      ctk_css_node_add_class (indicator_node, g_quark_from_static_string (CTK_STYLE_CLASS_RIGHT));

      node = ctk_css_node_get_last_child (widget_node);
      if (node != indicator_node)
        ctk_css_node_insert_after (widget_node, indicator_node, node);
    }
  else
    {
      ctk_css_node_add_class (indicator_node, g_quark_from_static_string (CTK_STYLE_CLASS_LEFT));
      ctk_css_node_remove_class (indicator_node, g_quark_from_static_string (CTK_STYLE_CLASS_RIGHT));

      node = ctk_css_node_get_first_child (widget_node);
      if (node != indicator_node)
        ctk_css_node_insert_before (widget_node, indicator_node, node);
    }

  CTK_WIDGET_CLASS (ctk_check_menu_item_parent_class)->direction_changed (widget, previous_dir);
}

static void
ctk_real_check_menu_item_draw_indicator (CtkCheckMenuItem *check_menu_item,
                                         cairo_t          *cr)
{
  ctk_css_gadget_draw (check_menu_item->priv->indicator_gadget, cr);
}

static void
ctk_check_menu_item_get_property (GObject     *object,
                                  guint        prop_id,
                                  GValue      *value,
                                  GParamSpec  *pspec)
{
  CtkCheckMenuItem *checkitem = CTK_CHECK_MENU_ITEM (object);
  CtkCheckMenuItemPrivate *priv = checkitem->priv;
  
  switch (prop_id)
    {
    case PROP_ACTIVE:
      g_value_set_boolean (value, priv->active);
      break;
    case PROP_INCONSISTENT:
      g_value_set_boolean (value, priv->inconsistent);
      break;
    case PROP_DRAW_AS_RADIO:
      g_value_set_boolean (value, priv->draw_as_radio);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
ctk_check_menu_item_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  CtkCheckMenuItem *checkitem = CTK_CHECK_MENU_ITEM (object);
  
  switch (prop_id)
    {
    case PROP_ACTIVE:
      ctk_check_menu_item_set_active (checkitem, g_value_get_boolean (value));
      break;
    case PROP_INCONSISTENT:
      ctk_check_menu_item_set_inconsistent (checkitem, g_value_get_boolean (value));
      break;
    case PROP_DRAW_AS_RADIO:
      ctk_check_menu_item_set_draw_as_radio (checkitem, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/* Private */

/*
 * _ctk_check_menu_item_set_active:
 * @check_menu_item: a #CtkCheckMenuItem
 * @is_active: whether the action is active or not
 *
 * Sets the #CtkCheckMenuItem:active property directly. This function does
 * not emit signals or notifications: it is left to the caller to do so.
 */
void
_ctk_check_menu_item_set_active (CtkCheckMenuItem *check_menu_item,
                                 gboolean          is_active)
{
  CtkCheckMenuItemPrivate *priv = check_menu_item->priv;

  priv->active = is_active;
  update_node_state (check_menu_item);
}

CtkCssGadget *
_ctk_check_menu_item_get_indicator_gadget (CtkCheckMenuItem *check_menu_item)
{
  return check_menu_item->priv->indicator_gadget;
}
