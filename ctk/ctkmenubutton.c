/* CTK - The GIMP Toolkit
 *
 * Copyright (C) 2003 Ricardo Fernandez Pascual
 * Copyright (C) 2004 Paolo Borelli
 * Copyright (C) 2012 Bastien Nocera
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

/**
 * SECTION:ctkmenubutton
 * @short_description: A widget that shows a popup when clicked on
 * @title: CtkMenuButton
 *
 * The #CtkMenuButton widget is used to display a popup when clicked on.
 * This popup can be provided either as a #CtkMenu, a #CtkPopover or an
 * abstract #GMenuModel.
 *
 * The #CtkMenuButton widget can hold any valid child widget. That is, it
 * can hold almost any other standard #CtkWidget. The most commonly used
 * child is #CtkImage. If no widget is explicitely added to the #CtkMenuButton,
 * a #CtkImage is automatically created, using an arrow image oriented
 * according to #CtkMenuButton:direction or the generic “open-menu-symbolic”
 * icon if the direction is not set.
 *
 * The positioning of the popup is determined by the #CtkMenuButton:direction
 * property of the menu button.
 *
 * For menus, the #CtkWidget:halign and #CtkWidget:valign properties of the
 * menu are also taken into account. For example, when the direction is
 * %CTK_ARROW_DOWN and the horizontal alignment is %CTK_ALIGN_START, the
 * menu will be positioned below the button, with the starting edge
 * (depending on the text direction) of the menu aligned with the starting
 * edge of the button. If there is not enough space below the button, the
 * menu is popped up above the button instead. If the alignment would move
 * part of the menu offscreen, it is “pushed in”.
 *
 * ## Direction = Down
 *
 * - halign = start
 *
 *     ![](down-start.png)
 *
 * - halign = center
 *
 *     ![](down-center.png)
 *
 * - halign = end
 *
 *     ![](down-end.png)
 *
 * ## Direction = Up
 *
 * - halign = start
 *
 *     ![](up-start.png)
 *
 * - halign = center
 *
 *     ![](up-center.png)
 *
 * - halign = end
 *
 *     ![](up-end.png)
 *
 * ## Direction = Left
 *
 * - valign = start
 *
 *     ![](left-start.png)
 *
 * - valign = center
 *
 *     ![](left-center.png)
 *
 * - valign = end
 *
 *     ![](left-end.png)
 *
 * ## Direction = Right
 *
 * - valign = start
 *
 *     ![](right-start.png)
 *
 * - valign = center
 *
 *     ![](right-center.png)
 *
 * - valign = end
 *
 *     ![](right-end.png)
 *
 * # CSS nodes
 *
 * CtkMenuButton has a single CSS node with name button. To differentiate
 * it from a plain #CtkButton, it gets the .popup style class.
 */

#include "config.h"

#include "ctkmenubutton.h"
#include "ctkmenubuttonprivate.h"
#include "ctkbuttonprivate.h"
#include "ctktypebuiltins.h"
#include "ctkwindow.h"
#include "ctkmain.h"
#include "ctkaccessible.h"
#include "ctkpopover.h"
#include "a11y/ctkmenubuttonaccessible.h"

#include "ctkprivate.h"
#include "ctkintl.h"

struct _CtkMenuButtonPrivate
{
  CtkWidget *menu;    /* The menu and the popover are mutually exclusive */
  CtkWidget *popover; /* Only one at a time can be set */
  GMenuModel *model;

  CtkMenuButtonShowMenuCallback func;
  gpointer user_data;

  CtkWidget *align_widget;
  CtkWidget *arrow_widget;
  CtkArrowType arrow_type;
  gboolean use_popover;
  guint press_handled : 1;
};

enum
{
  PROP_0,
  PROP_POPUP,
  PROP_MENU_MODEL,
  PROP_ALIGN_WIDGET,
  PROP_DIRECTION,
  PROP_USE_POPOVER,
  PROP_POPOVER,
  LAST_PROP
};

static GParamSpec *menu_button_props[LAST_PROP];

G_DEFINE_TYPE_WITH_PRIVATE (CtkMenuButton, ctk_menu_button, CTK_TYPE_TOGGLE_BUTTON)

static void ctk_menu_button_dispose (GObject *object);

static void
ctk_menu_button_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  CtkMenuButton *self = CTK_MENU_BUTTON (object);

  switch (property_id)
    {
      case PROP_POPUP:
        ctk_menu_button_set_popup (self, g_value_get_object (value));
        break;
      case PROP_MENU_MODEL:
        ctk_menu_button_set_menu_model (self, g_value_get_object (value));
        break;
      case PROP_ALIGN_WIDGET:
        ctk_menu_button_set_align_widget (self, g_value_get_object (value));
        break;
      case PROP_DIRECTION:
        ctk_menu_button_set_direction (self, g_value_get_enum (value));
        break;
      case PROP_USE_POPOVER:
        ctk_menu_button_set_use_popover (self, g_value_get_boolean (value));
        break;
      case PROP_POPOVER:
        ctk_menu_button_set_popover (self, g_value_get_object (value));
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
ctk_menu_button_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  CtkMenuButtonPrivate *priv = CTK_MENU_BUTTON (object)->priv;

  switch (property_id)
    {
      case PROP_POPUP:
        g_value_set_object (value, priv->menu);
        break;
      case PROP_MENU_MODEL:
        g_value_set_object (value, priv->model);
        break;
      case PROP_ALIGN_WIDGET:
        g_value_set_object (value, priv->align_widget);
        break;
      case PROP_DIRECTION:
        g_value_set_enum (value, priv->arrow_type);
        break;
      case PROP_USE_POPOVER:
        g_value_set_boolean (value, priv->use_popover);
        break;
      case PROP_POPOVER:
        g_value_set_object (value, priv->popover);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
ctk_menu_button_state_flags_changed (CtkWidget    *widget,
                                     CtkStateFlags previous_state_flags)
{
  CtkMenuButton *button = CTK_MENU_BUTTON (widget);
  CtkMenuButtonPrivate *priv = button->priv;

  if (!ctk_widget_is_sensitive (widget))
    {
      if (priv->menu)
        ctk_menu_shell_deactivate (CTK_MENU_SHELL (priv->menu));
      else if (priv->popover)
        ctk_widget_hide (priv->popover);
    }
}

static void
popup_menu (CtkMenuButton *menu_button,
            CdkEvent      *event)
{
  CtkMenuButtonPrivate *priv = menu_button->priv;
  CdkGravity widget_anchor = GDK_GRAVITY_SOUTH_WEST;
  CdkGravity menu_anchor = GDK_GRAVITY_NORTH_WEST;

  if (priv->func)
    priv->func (priv->user_data);

  if (!priv->menu)
    return;

  switch (priv->arrow_type)
    {
    case CTK_ARROW_UP:
      g_object_set (priv->menu,
                    "anchor-hints", (GDK_ANCHOR_FLIP_Y |
                                     GDK_ANCHOR_SLIDE |
                                     GDK_ANCHOR_RESIZE),
                    NULL);

      switch (ctk_widget_get_halign (priv->menu))
        {
        case CTK_ALIGN_FILL:
        case CTK_ALIGN_START:
        case CTK_ALIGN_BASELINE:
          widget_anchor = GDK_GRAVITY_NORTH_WEST;
          menu_anchor = GDK_GRAVITY_SOUTH_WEST;
          break;

        case CTK_ALIGN_END:
          widget_anchor = GDK_GRAVITY_NORTH_EAST;
          menu_anchor = GDK_GRAVITY_SOUTH_EAST;
          break;

        case CTK_ALIGN_CENTER:
          widget_anchor = GDK_GRAVITY_NORTH;
          menu_anchor = GDK_GRAVITY_SOUTH;
          break;
        }

      break;

    case CTK_ARROW_DOWN:
      /* In the common case the menu button is showing a dropdown menu, set the
       * corresponding type hint on the toplevel, so the WM can omit the top side
       * of the shadows.
       */
      g_object_set (priv->menu,
                    "anchor-hints", (GDK_ANCHOR_FLIP_Y |
                                     GDK_ANCHOR_SLIDE |
                                     GDK_ANCHOR_RESIZE),
                    "menu-type-hint", GDK_WINDOW_TYPE_HINT_DROPDOWN_MENU,
                    NULL);

      switch (ctk_widget_get_halign (priv->menu))
        {
        case CTK_ALIGN_FILL:
        case CTK_ALIGN_START:
        case CTK_ALIGN_BASELINE:
          widget_anchor = GDK_GRAVITY_SOUTH_WEST;
          menu_anchor = GDK_GRAVITY_NORTH_WEST;
          break;

        case CTK_ALIGN_END:
          widget_anchor = GDK_GRAVITY_SOUTH_EAST;
          menu_anchor = GDK_GRAVITY_NORTH_EAST;
          break;

        case CTK_ALIGN_CENTER:
          widget_anchor = GDK_GRAVITY_SOUTH;
          menu_anchor = GDK_GRAVITY_NORTH;
          break;
        }

      break;

    case CTK_ARROW_LEFT:
      g_object_set (priv->menu,
                    "anchor-hints", (GDK_ANCHOR_FLIP_X |
                                     GDK_ANCHOR_SLIDE |
                                     GDK_ANCHOR_RESIZE),
                    NULL);

      switch (ctk_widget_get_valign (priv->menu))
        {
        case CTK_ALIGN_FILL:
        case CTK_ALIGN_START:
        case CTK_ALIGN_BASELINE:
          widget_anchor = GDK_GRAVITY_NORTH_WEST;
          menu_anchor = GDK_GRAVITY_NORTH_EAST;
          break;

        case CTK_ALIGN_END:
          widget_anchor = GDK_GRAVITY_SOUTH_WEST;
          menu_anchor = GDK_GRAVITY_SOUTH_EAST;
          break;

        case CTK_ALIGN_CENTER:
          widget_anchor = GDK_GRAVITY_WEST;
          menu_anchor = GDK_GRAVITY_EAST;
          break;
        }

      break;

    case CTK_ARROW_RIGHT:
      g_object_set (priv->menu,
                    "anchor-hints", (GDK_ANCHOR_FLIP_X |
                                     GDK_ANCHOR_SLIDE |
                                     GDK_ANCHOR_RESIZE),
                    NULL);

      switch (ctk_widget_get_valign (priv->menu))
        {
        case CTK_ALIGN_FILL:
        case CTK_ALIGN_START:
        case CTK_ALIGN_BASELINE:
          widget_anchor = GDK_GRAVITY_NORTH_EAST;
          menu_anchor = GDK_GRAVITY_NORTH_WEST;
          break;

        case CTK_ALIGN_END:
          widget_anchor = GDK_GRAVITY_SOUTH_EAST;
          menu_anchor = GDK_GRAVITY_SOUTH_WEST;
          break;

        case CTK_ALIGN_CENTER:
          widget_anchor = GDK_GRAVITY_EAST;
          menu_anchor = GDK_GRAVITY_WEST;
          break;
        }

      break;

    case CTK_ARROW_NONE:
      g_object_set (priv->menu,
                    "anchor-hints", (GDK_ANCHOR_FLIP_Y |
                                     GDK_ANCHOR_SLIDE |
                                     GDK_ANCHOR_RESIZE),
                    NULL);

      break;
    }

  ctk_menu_popup_at_widget (CTK_MENU (priv->menu),
                            CTK_WIDGET (menu_button),
                            widget_anchor,
                            menu_anchor,
                            event);
}

static void
ctk_menu_button_toggled (CtkToggleButton *button)
{
  CtkMenuButton *menu_button = CTK_MENU_BUTTON (button);
  CtkMenuButtonPrivate *priv = menu_button->priv;
  gboolean active = ctk_toggle_button_get_active (button);

  if (priv->menu)
    {
      if (active && !ctk_widget_get_visible (priv->menu))
        {
          CdkEvent *event;

          event = ctk_get_current_event ();

          popup_menu (menu_button, event);

          if (!event ||
              event->type == GDK_KEY_PRESS ||
              event->type == GDK_KEY_RELEASE)
            ctk_menu_shell_select_first (CTK_MENU_SHELL (priv->menu), FALSE);

          if (event)
            cdk_event_free (event);
        }
    }
  else if (priv->popover)
    {
      if (active)
        ctk_popover_popup (CTK_POPOVER (priv->popover));
      else
        ctk_popover_popdown (CTK_POPOVER (priv->popover));
    }

  if (CTK_TOGGLE_BUTTON_CLASS (ctk_menu_button_parent_class)->toggled)
    CTK_TOGGLE_BUTTON_CLASS (ctk_menu_button_parent_class)->toggled (button);
}

static void
ctk_menu_button_add (CtkContainer *container,
                     CtkWidget    *child)
{
  CtkMenuButton *button = CTK_MENU_BUTTON (container);

  if (button->priv->arrow_widget)
    ctk_container_remove (container, button->priv->arrow_widget);

  CTK_CONTAINER_CLASS (ctk_menu_button_parent_class)->add (container, child);
}

static void
ctk_menu_button_remove (CtkContainer *container,
                        CtkWidget    *child)
{
  CtkMenuButton *button = CTK_MENU_BUTTON (container);

  if (child == button->priv->arrow_widget)
    button->priv->arrow_widget = NULL;

  CTK_CONTAINER_CLASS (ctk_menu_button_parent_class)->remove (container, child);
}

static void
ctk_menu_button_class_init (CtkMenuButtonClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);
  CtkContainerClass *container_class = CTK_CONTAINER_CLASS (klass);
  CtkToggleButtonClass *toggle_button_class = CTK_TOGGLE_BUTTON_CLASS (klass);

  gobject_class->set_property = ctk_menu_button_set_property;
  gobject_class->get_property = ctk_menu_button_get_property;
  gobject_class->dispose = ctk_menu_button_dispose;

  widget_class->state_flags_changed = ctk_menu_button_state_flags_changed;

  container_class->add = ctk_menu_button_add;
  container_class->remove = ctk_menu_button_remove;

  toggle_button_class->toggled = ctk_menu_button_toggled;

  /**
   * CtkMenuButton:popup:
   *
   * The #CtkMenu that will be popped up when the button is clicked.
   *
   * Since: 3.6
   */
  menu_button_props[PROP_POPUP] =
      g_param_spec_object ("popup",
                           P_("Popup"),
                           P_("The dropdown menu."),
                           CTK_TYPE_MENU,
                           CTK_PARAM_READWRITE);

  /**
   * CtkMenuButton:menu-model:
   *
   * The #GMenuModel from which the popup will be created.
   * Depending on the #CtkMenuButton:use-popover property, that may
   * be a menu or a popover.
   *
   * See ctk_menu_button_set_menu_model() for the interaction with the
   * #CtkMenuButton:popup property.
   *
   * Since: 3.6
   */
  menu_button_props[PROP_MENU_MODEL] =
      g_param_spec_object ("menu-model",
                           P_("Menu model"),
                           P_("The model from which the popup is made."),
                           G_TYPE_MENU_MODEL,
                           CTK_PARAM_READWRITE);

  /**
   * CtkMenuButton:align-widget:
   *
   * The #CtkWidget to use to align the menu with.
   *
   * Since: 3.6
   */
  menu_button_props[PROP_ALIGN_WIDGET] =
      g_param_spec_object ("align-widget",
                           P_("Align with"),
                           P_("The parent widget which the menu should align with."),
                           CTK_TYPE_CONTAINER,
                           CTK_PARAM_READWRITE);

  /**
   * CtkMenuButton:direction:
   *
   * The #CtkArrowType representing the direction in which the
   * menu or popover will be popped out.
   *
   * Since: 3.6
   */
  menu_button_props[PROP_DIRECTION] =
      g_param_spec_enum ("direction",
                         P_("Direction"),
                         P_("The direction the arrow should point."),
                         CTK_TYPE_ARROW_TYPE,
                         CTK_ARROW_DOWN,
                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkMenuButton:use-popover:
   *
   * Whether to construct a #CtkPopover from the menu model,
   * or a #CtkMenu.
   *
   * Since: 3.12
   */
  menu_button_props[PROP_USE_POPOVER] =
      g_param_spec_boolean ("use-popover",
                            P_("Use a popover"),
                            P_("Use a popover instead of a menu"),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkMenuButton:popover:
   *
   * The #CtkPopover that will be popped up when the button is clicked.
   *
   * Since: 3.12
   */
  menu_button_props[PROP_POPOVER] =
      g_param_spec_object ("popover",
                           P_("Popover"),
                           P_("The popover"),
                           CTK_TYPE_POPOVER,
                           G_PARAM_READWRITE);

  g_object_class_install_properties (gobject_class, LAST_PROP, menu_button_props);

  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_MENU_BUTTON_ACCESSIBLE);
  ctk_widget_class_set_css_name (widget_class, "button");
}

static void
set_arrow_type (CtkImage     *image,
                CtkArrowType  arrow_type)
{
  switch (arrow_type)
    {
    case CTK_ARROW_NONE:
      ctk_image_set_from_icon_name (image, "open-menu-symbolic", CTK_ICON_SIZE_BUTTON);
      break;
    case CTK_ARROW_DOWN:
      ctk_image_set_from_icon_name (image, "pan-down-symbolic", CTK_ICON_SIZE_BUTTON);
      break;
    case CTK_ARROW_UP:
      ctk_image_set_from_icon_name (image, "pan-up-symbolic", CTK_ICON_SIZE_BUTTON);
      break;
    case CTK_ARROW_LEFT:
      ctk_image_set_from_icon_name (image, "pan-start-symbolic", CTK_ICON_SIZE_BUTTON);
      break;
    case CTK_ARROW_RIGHT:
      ctk_image_set_from_icon_name (image, "pan-end-symbolic", CTK_ICON_SIZE_BUTTON);
      break;
    }
}

static void
add_arrow (CtkMenuButton *menu_button)
{
  CtkWidget *arrow;

  arrow = ctk_image_new ();
  set_arrow_type (CTK_IMAGE (arrow), menu_button->priv->arrow_type);
  ctk_container_add (CTK_CONTAINER (menu_button), arrow);
  ctk_widget_show (arrow);
  menu_button->priv->arrow_widget = arrow;
}

static void
ctk_menu_button_init (CtkMenuButton *menu_button)
{
  CtkMenuButtonPrivate *priv;
  CtkStyleContext *context;

  priv = ctk_menu_button_get_instance_private (menu_button);
  menu_button->priv = priv;
  priv->arrow_type = CTK_ARROW_DOWN;
  priv->use_popover = TRUE;

  add_arrow (menu_button);

  ctk_widget_set_focus_on_click (CTK_WIDGET (menu_button), FALSE);
  ctk_widget_set_sensitive (CTK_WIDGET (menu_button), FALSE);

  context = ctk_widget_get_style_context (CTK_WIDGET (menu_button));
  ctk_style_context_add_class (context, "popup");
}

/**
 * ctk_menu_button_new:
 *
 * Creates a new #CtkMenuButton widget with downwards-pointing
 * arrow as the only child. You can replace the child widget
 * with another #CtkWidget should you wish to.
 *
 * Returns: The newly created #CtkMenuButton widget
 *
 * Since: 3.6
 */
CtkWidget *
ctk_menu_button_new (void)
{
  return g_object_new (CTK_TYPE_MENU_BUTTON, NULL);
}

/* Callback for the "deactivate" signal on the pop-up menu.
 * This is used so that we unset the state of the toggle button
 * when the pop-up menu disappears.
 * Also used for the "close" signal on the popover.
 */
static gboolean
menu_deactivate_cb (CtkMenuButton *menu_button)
{
  ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (menu_button), FALSE);
  ctk_widget_unset_state_flags (CTK_WIDGET (menu_button), CTK_STATE_FLAG_PRELIGHT);

  return TRUE;
}

static void
menu_detacher (CtkWidget *widget,
               CtkMenu   *menu)
{
  CtkMenuButtonPrivate *priv = CTK_MENU_BUTTON (widget)->priv;

  g_return_if_fail (priv->menu == (CtkWidget *) menu);

  priv->menu = NULL;
}

static void
update_sensitivity (CtkMenuButton *menu_button)
{
  CtkMenuButtonPrivate *priv = menu_button->priv;

  if (CTK_BUTTON (menu_button)->priv->action_helper)
    return;

  ctk_widget_set_sensitive (CTK_WIDGET (menu_button),
                            priv->menu != NULL || priv->popover != NULL);
}

/* This function is used in CtkMenuToolButton, the call back will
 * be called when CtkMenuToolButton would have emitted the “show-menu”
 * signal.
 */
void
_ctk_menu_button_set_popup_with_func (CtkMenuButton                 *menu_button,
                                      CtkWidget                     *menu,
                                      CtkMenuButtonShowMenuCallback  func,
                                      gpointer                       user_data)
{
  CtkMenuButtonPrivate *priv;

  g_return_if_fail (CTK_IS_MENU_BUTTON (menu_button));
  g_return_if_fail (CTK_IS_MENU (menu) || menu == NULL);

  priv = menu_button->priv;
  priv->func = func;
  priv->user_data = user_data;

  if (priv->menu == CTK_WIDGET (menu))
    return;

  if (priv->menu)
    {
      if (ctk_widget_get_visible (priv->menu))
        ctk_menu_shell_deactivate (CTK_MENU_SHELL (priv->menu));

      g_signal_handlers_disconnect_by_func (priv->menu,
                                            menu_deactivate_cb,
                                            menu_button);
      ctk_menu_detach (CTK_MENU (priv->menu));
    }

  priv->menu = menu;

  if (priv->menu)
    {
      ctk_menu_attach_to_widget (CTK_MENU (priv->menu), CTK_WIDGET (menu_button),
                                 menu_detacher);

      ctk_widget_set_visible (priv->menu, FALSE);

      g_signal_connect_swapped (priv->menu, "deactivate",
                                G_CALLBACK (menu_deactivate_cb), menu_button);
    }

  update_sensitivity (menu_button);

  g_object_notify_by_pspec (G_OBJECT (menu_button), menu_button_props[PROP_POPUP]);
  g_object_notify_by_pspec (G_OBJECT (menu_button), menu_button_props[PROP_MENU_MODEL]);
}

/**
 * ctk_menu_button_set_popup:
 * @menu_button: a #CtkMenuButton
 * @menu: (nullable): a #CtkMenu, or %NULL to unset and disable the button
 *
 * Sets the #CtkMenu that will be popped up when the @menu_button is clicked, or
 * %NULL to dissociate any existing menu and disable the button.
 *
 * If #CtkMenuButton:menu-model or #CtkMenuButton:popover are set, those objects
 * are dissociated from the @menu_button, and those properties are set to %NULL.
 *
 * Since: 3.6
 */
void
ctk_menu_button_set_popup (CtkMenuButton *menu_button,
                           CtkWidget     *menu)
{
  CtkMenuButtonPrivate *priv = menu_button->priv;

  g_return_if_fail (CTK_IS_MENU_BUTTON (menu_button));
  g_return_if_fail (CTK_IS_MENU (menu) || menu == NULL);

  g_object_freeze_notify (G_OBJECT (menu_button));

  g_clear_object (&priv->model);

  _ctk_menu_button_set_popup_with_func (menu_button, menu, NULL, NULL);

  if (menu && priv->popover)
    ctk_menu_button_set_popover (menu_button, NULL);

  update_sensitivity (menu_button);

  g_object_thaw_notify (G_OBJECT (menu_button));
}

/**
 * ctk_menu_button_get_popup:
 * @menu_button: a #CtkMenuButton
 *
 * Returns the #CtkMenu that pops out of the button.
 * If the button does not use a #CtkMenu, this function
 * returns %NULL.
 *
 * Returns: (nullable) (transfer none): a #CtkMenu or %NULL
 *
 * Since: 3.6
 */
CtkMenu *
ctk_menu_button_get_popup (CtkMenuButton *menu_button)
{
  g_return_val_if_fail (CTK_IS_MENU_BUTTON (menu_button), NULL);

  return CTK_MENU (menu_button->priv->menu);
}

/**
 * ctk_menu_button_set_menu_model:
 * @menu_button: a #CtkMenuButton
 * @menu_model: (nullable): a #GMenuModel, or %NULL to unset and disable the
 *   button
 *
 * Sets the #GMenuModel from which the popup will be constructed,
 * or %NULL to dissociate any existing menu model and disable the button.
 *
 * Depending on the value of #CtkMenuButton:use-popover, either a
 * #CtkMenu will be created with ctk_menu_new_from_model(), or a
 * #CtkPopover with ctk_popover_new_from_model(). In either case,
 * actions will be connected as documented for these functions.
 *
 * If #CtkMenuButton:popup or #CtkMenuButton:popover are already set, those
 * widgets are dissociated from the @menu_button, and those properties are set
 * to %NULL.
 *
 * Since: 3.6
 */
void
ctk_menu_button_set_menu_model (CtkMenuButton *menu_button,
                                GMenuModel    *menu_model)
{
  CtkMenuButtonPrivate *priv;

  g_return_if_fail (CTK_IS_MENU_BUTTON (menu_button));
  g_return_if_fail (G_IS_MENU_MODEL (menu_model) || menu_model == NULL);

  priv = menu_button->priv;

  g_object_freeze_notify (G_OBJECT (menu_button));

  if (menu_model)
    g_object_ref (menu_model);

  if (menu_model)
    {
      if (priv->use_popover)
        {
          CtkWidget *popover;

          popover = ctk_popover_new_from_model (CTK_WIDGET (menu_button), menu_model);
          ctk_menu_button_set_popover (menu_button, popover);
        }
      else
        {
          CtkWidget *menu;

          menu = ctk_menu_new_from_model (menu_model);
          ctk_widget_show_all (menu);
          ctk_menu_button_set_popup (menu_button, menu);
        }
    }
  else
    {
      ctk_menu_button_set_popup (menu_button, NULL);
      ctk_menu_button_set_popover (menu_button, NULL);
    }

  priv->model = menu_model;
  g_object_notify_by_pspec (G_OBJECT (menu_button), menu_button_props[PROP_MENU_MODEL]);

  g_object_thaw_notify (G_OBJECT (menu_button));
}

/**
 * ctk_menu_button_get_menu_model:
 * @menu_button: a #CtkMenuButton
 *
 * Returns the #GMenuModel used to generate the popup.
 *
 * Returns: (nullable) (transfer none): a #GMenuModel or %NULL
 *
 * Since: 3.6
 */
GMenuModel *
ctk_menu_button_get_menu_model (CtkMenuButton *menu_button)
{
  g_return_val_if_fail (CTK_IS_MENU_BUTTON (menu_button), NULL);

  return menu_button->priv->model;
}

static void
set_align_widget_pointer (CtkMenuButton *menu_button,
                          CtkWidget     *align_widget)
{
  CtkMenuButtonPrivate *priv;

  priv = menu_button->priv;

  if (priv->align_widget)
    g_object_remove_weak_pointer (G_OBJECT (priv->align_widget), (gpointer *) &priv->align_widget);

  priv->align_widget = align_widget;

  if (priv->align_widget)
    g_object_add_weak_pointer (G_OBJECT (priv->align_widget), (gpointer *) &priv->align_widget);
}

/**
 * ctk_menu_button_set_align_widget:
 * @menu_button: a #CtkMenuButton
 * @align_widget: (allow-none): a #CtkWidget
 *
 * Sets the #CtkWidget to use to line the menu with when popped up.
 * Note that the @align_widget must contain the #CtkMenuButton itself.
 *
 * Setting it to %NULL means that the menu will be aligned with the
 * button itself.
 *
 * Note that this property is only used with menus currently,
 * and not for popovers.
 *
 * Since: 3.6
 */
void
ctk_menu_button_set_align_widget (CtkMenuButton *menu_button,
                                  CtkWidget     *align_widget)
{
  CtkMenuButtonPrivate *priv;

  g_return_if_fail (CTK_IS_MENU_BUTTON (menu_button));
  g_return_if_fail (align_widget == NULL || ctk_widget_is_ancestor (CTK_WIDGET (menu_button), align_widget));

  priv = menu_button->priv;
  if (priv->align_widget == align_widget)
    return;

  set_align_widget_pointer (menu_button, align_widget);

  g_object_notify_by_pspec (G_OBJECT (menu_button), menu_button_props[PROP_ALIGN_WIDGET]);
}

/**
 * ctk_menu_button_get_align_widget:
 * @menu_button: a #CtkMenuButton
 *
 * Returns the parent #CtkWidget to use to line up with menu.
 *
 * Returns: (nullable) (transfer none): a #CtkWidget value or %NULL
 *
 * Since: 3.6
 */
CtkWidget *
ctk_menu_button_get_align_widget (CtkMenuButton *menu_button)
{
  g_return_val_if_fail (CTK_IS_MENU_BUTTON (menu_button), NULL);

  return menu_button->priv->align_widget;
}

static void
update_popover_direction (CtkMenuButton *menu_button)
{
  CtkMenuButtonPrivate *priv = menu_button->priv;

  if (!priv->popover)
    return;

  switch (priv->arrow_type)
    {
    case CTK_ARROW_UP:
      ctk_popover_set_position (CTK_POPOVER (priv->popover), CTK_POS_TOP);
      break;
    case CTK_ARROW_DOWN:
    case CTK_ARROW_NONE:
      ctk_popover_set_position (CTK_POPOVER (priv->popover), CTK_POS_BOTTOM);
      break;
    case CTK_ARROW_LEFT:
      ctk_popover_set_position (CTK_POPOVER (priv->popover), CTK_POS_LEFT);
      break;
    case CTK_ARROW_RIGHT:
      ctk_popover_set_position (CTK_POPOVER (priv->popover), CTK_POS_RIGHT);
      break;
    }
}

static void
popover_destroy_cb (CtkMenuButton *menu_button)
{
  ctk_menu_button_set_popover (menu_button, NULL);
}

/**
 * ctk_menu_button_set_direction:
 * @menu_button: a #CtkMenuButton
 * @direction: a #CtkArrowType
 *
 * Sets the direction in which the popup will be popped up, as
 * well as changing the arrow’s direction. The child will not
 * be changed to an arrow if it was customized.
 *
 * If the does not fit in the available space in the given direction,
 * CTK+ will its best to keep it inside the screen and fully visible.
 *
 * If you pass %CTK_ARROW_NONE for a @direction, the popup will behave
 * as if you passed %CTK_ARROW_DOWN (although you won’t see any arrows).
 *
 * Since: 3.6
 */
void
ctk_menu_button_set_direction (CtkMenuButton *menu_button,
                               CtkArrowType   direction)
{
  CtkMenuButtonPrivate *priv = menu_button->priv;
  CtkWidget *child;

  g_return_if_fail (CTK_IS_MENU_BUTTON (menu_button));

  if (priv->arrow_type == direction)
    return;

  priv->arrow_type = direction;
  g_object_notify_by_pspec (G_OBJECT (menu_button), menu_button_props[PROP_DIRECTION]);

  /* Is it custom content? We don't change that */
  child = ctk_bin_get_child (CTK_BIN (menu_button));
  if (priv->arrow_widget != child)
    return;

  set_arrow_type (CTK_IMAGE (child), priv->arrow_type);
  update_popover_direction (menu_button);
}

/**
 * ctk_menu_button_get_direction:
 * @menu_button: a #CtkMenuButton
 *
 * Returns the direction the popup will be pointing at when popped up.
 *
 * Returns: a #CtkArrowType value
 *
 * Since: 3.6
 */
CtkArrowType
ctk_menu_button_get_direction (CtkMenuButton *menu_button)
{
  g_return_val_if_fail (CTK_IS_MENU_BUTTON (menu_button), CTK_ARROW_DOWN);

  return menu_button->priv->arrow_type;
}

static void
ctk_menu_button_dispose (GObject *object)
{
  CtkMenuButtonPrivate *priv = CTK_MENU_BUTTON (object)->priv;

  if (priv->menu)
    {
      g_signal_handlers_disconnect_by_func (priv->menu,
                                            menu_deactivate_cb,
                                            object);
      ctk_menu_detach (CTK_MENU (priv->menu));
      priv->menu = NULL;
    }

  if (priv->popover)
    {
      g_signal_handlers_disconnect_by_func (priv->popover,
                                            menu_deactivate_cb,
                                            object);
      g_signal_handlers_disconnect_by_func (priv->popover,
                                            popover_destroy_cb,
                                            object);
      ctk_popover_set_relative_to (CTK_POPOVER (priv->popover), NULL);
      priv->popover = NULL;
    }

  set_align_widget_pointer (CTK_MENU_BUTTON (object), NULL);

  g_clear_object (&priv->model);

  G_OBJECT_CLASS (ctk_menu_button_parent_class)->dispose (object);
}

/**
 * ctk_menu_button_set_use_popover:
 * @menu_button: a #CtkMenuButton
 * @use_popover: %TRUE to construct a popover from the menu model
 *
 * Sets whether to construct a #CtkPopover instead of #CtkMenu
 * when ctk_menu_button_set_menu_model() is called. Note that
 * this property is only consulted when a new menu model is set.
 *
 * Since: 3.12
 */
void
ctk_menu_button_set_use_popover (CtkMenuButton *menu_button,
                                 gboolean       use_popover)
{
  CtkMenuButtonPrivate *priv;

  g_return_if_fail (CTK_IS_MENU_BUTTON (menu_button));

  priv = menu_button->priv;

  use_popover = use_popover != FALSE;

  if (priv->use_popover == use_popover)
    return;

  priv->use_popover = use_popover;

  g_object_freeze_notify (G_OBJECT (menu_button));

  if (priv->model)
    ctk_menu_button_set_menu_model (menu_button, priv->model);

  g_object_notify_by_pspec (G_OBJECT (menu_button), menu_button_props[PROP_USE_POPOVER]);

  g_object_thaw_notify (G_OBJECT (menu_button));
}

/**
 * ctk_menu_button_get_use_popover:
 * @menu_button: a #CtkMenuButton
 *
 * Returns whether a #CtkPopover or a #CtkMenu will be constructed
 * from the menu model.
 *
 * Returns: %TRUE if using a #CtkPopover
 *
 * Since: 3.12
 */
gboolean
ctk_menu_button_get_use_popover (CtkMenuButton *menu_button)
{
  g_return_val_if_fail (CTK_IS_MENU_BUTTON (menu_button), FALSE);

  return menu_button->priv->use_popover;
}

/**
 * ctk_menu_button_set_popover:
 * @menu_button: a #CtkMenuButton
 * @popover: (nullable): a #CtkPopover, or %NULL to unset and disable the button
 *
 * Sets the #CtkPopover that will be popped up when the @menu_button is clicked,
 * or %NULL to dissociate any existing popover and disable the button.
 *
 * If #CtkMenuButton:menu-model or #CtkMenuButton:popup are set, those objects
 * are dissociated from the @menu_button, and those properties are set to %NULL.
 *
 * Since: 3.12
 */
void
ctk_menu_button_set_popover (CtkMenuButton *menu_button,
                             CtkWidget     *popover)
{
  CtkMenuButtonPrivate *priv = menu_button->priv;

  g_return_if_fail (CTK_IS_MENU_BUTTON (menu_button));
  g_return_if_fail (CTK_IS_POPOVER (popover) || popover == NULL);

  g_object_freeze_notify (G_OBJECT (menu_button));

  g_clear_object (&priv->model);

  if (priv->popover)
    {
      if (ctk_widget_get_visible (priv->popover))
        ctk_widget_hide (priv->popover);

      g_signal_handlers_disconnect_by_func (priv->popover,
                                            menu_deactivate_cb,
                                            menu_button);
      g_signal_handlers_disconnect_by_func (priv->popover,
                                            popover_destroy_cb,
                                            menu_button);

      ctk_popover_set_relative_to (CTK_POPOVER (priv->popover), NULL);
    }

  priv->popover = popover;

  if (popover)
    {
      ctk_popover_set_relative_to (CTK_POPOVER (priv->popover), CTK_WIDGET (menu_button));
      g_signal_connect_swapped (priv->popover, "closed",
                                G_CALLBACK (menu_deactivate_cb), menu_button);
      g_signal_connect_swapped (priv->popover, "destroy",
                                G_CALLBACK (popover_destroy_cb), menu_button);
      update_popover_direction (menu_button);
      ctk_style_context_remove_class (ctk_widget_get_style_context (CTK_WIDGET (menu_button)), "menu-button");
    }

  if (popover && priv->menu)
    ctk_menu_button_set_popup (menu_button, NULL);

  update_sensitivity (menu_button);

  g_object_notify_by_pspec (G_OBJECT (menu_button), menu_button_props[PROP_POPOVER]);
  g_object_notify_by_pspec (G_OBJECT (menu_button), menu_button_props[PROP_MENU_MODEL]);
  g_object_thaw_notify (G_OBJECT (menu_button));
}

/**
 * ctk_menu_button_get_popover:
 * @menu_button: a #CtkMenuButton
 *
 * Returns the #CtkPopover that pops out of the button.
 * If the button is not using a #CtkPopover, this function
 * returns %NULL.
 *
 * Returns: (nullable) (transfer none): a #CtkPopover or %NULL
 *
 * Since: 3.12
 */
CtkPopover *
ctk_menu_button_get_popover (CtkMenuButton *menu_button)
{
  g_return_val_if_fail (CTK_IS_MENU_BUTTON (menu_button), NULL);

  return CTK_POPOVER (menu_button->priv->popover);
}
