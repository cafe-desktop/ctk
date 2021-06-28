/*
 * Copyright © 2014 Red Hat, Inc.
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
 * Author: Matthias Clasen
 */

#include "config.h"

#include "ctkmodelbutton.h"

#include "ctkbutton.h"
#include "ctkbuttonprivate.h"
#include "ctkwidgetprivate.h"
#include "ctkmenutrackeritem.h"
#include "ctkimage.h"
#include "ctklabel.h"
#include "ctkbox.h"
#include "ctkrender.h"
#include "ctkstylecontext.h"
#include "ctktypebuiltins.h"
#include "ctkstack.h"
#include "ctkpopover.h"
#include "ctkintl.h"
#include "ctkcssnodeprivate.h"
#include "ctkcsscustomgadgetprivate.h"
#include "ctkbuiltiniconprivate.h"
#include "ctkcsstypesprivate.h"
#include "ctkstylecontextprivate.h"
#include "ctkcontainerprivate.h"

/**
 * SECTION:ctkmodelbutton
 * @Short_description: A button that uses a GAction as model
 * @Title: CtkModelButton
 *
 * CtkModelButton is a button class that can use a #GAction as its model.
 * In contrast to #CtkToggleButton or #CtkRadioButton, which can also
 * be backed by a #GAction via the #CtkActionable:action-name property,
 * CtkModelButton will adapt its appearance according to the kind of
 * action it is backed by, and appear either as a plain, check or
 * radio button.
 *
 * Model buttons are used when popovers from a menu model with
 * ctk_popover_new_from_model(); they can also be used manually in
 * a #CtkPopoverMenu.
 *
 * When the action is specified via the #CtkActionable:action-name
 * and #CtkActionable:action-target properties, the role of the button
 * (i.e. whether it is a plain, check or radio button) is determined by
 * the type of the action and doesn't have to be explicitly specified
 * with the #CtkModelButton:role property.
 *
 * The content of the button is specified by the #CtkModelButton:text
 * and #CtkModelButton:icon properties.
 *
 * The appearance of model buttons can be influenced with the
 * #CtkModelButton:centered and #CtkModelButton:iconic properties.
 *
 * Model buttons have built-in support for submenus in #CtkPopoverMenu.
 * To make a CtkModelButton that opens a submenu when activated, set
 * the #CtkModelButton:menu-name property. To make a button that goes
 * back to the parent menu, you should set the #CtkModelButton:inverted
 * property to place the submenu indicator at the opposite side.
 *
 * # Example
 *
 * |[
 * <object class="CtkPopoverMenu">
 *   <child>
 *     <object class="CtkBox">
 *       <property name="visible">True</property>
 *       <property name="margin">10</property>
 *       <child>
 *         <object class="CtkModelButton">
 *           <property name="visible">True</property>
 *           <property name="action-name">view.cut</property>
 *           <property name="text" translatable="yes">Cut</property>
 *         </object>
 *       </child>
 *       <child>
 *         <object class="CtkModelButton">
 *           <property name="visible">True</property>
 *           <property name="action-name">view.copy</property>
 *           <property name="text" translatable="yes">Copy</property>
 *         </object>
 *       </child>
 *       <child>
 *         <object class="CtkModelButton">
 *           <property name="visible">True</property>
 *           <property name="action-name">view.paste</property>
 *           <property name="text" translatable="yes">Paste</property>
 *         </object>
 *       </child>
 *     </object>
 *   </child>
 * </object>
 * ]|
 *
 * # CSS nodes
 *
 * |[<!-- language="plain" -->
 * modelbutton
 * ├── <child>
 * ╰── check
 * ]|
 *
 * |[<!-- language="plain" -->
 * modelbutton
 * ├── <child>
 * ╰── radio
 * ]|
 *
 * |[<!-- language="plain" -->
 * modelbutton
 * ├── <child>
 * ╰── arrow
 * ]|
 *
 * CtkModelButton has a main CSS node with name modelbutton, and a subnode,
 * which will have the name check, radio or arrow, depending on the role
 * of the button and whether it has a menu name set.
 *
 * The subnode is positioned before or after the content nodes and gets the
 * .left or .right style class, depending on where it is located.
 *
 * |[<!-- language="plain" -->
 * button.model
 * ├── <child>
 * ╰── check
 * ]|
 *
 * Iconic model buttons (see #CtkModelButton:iconic) change the name of
 * their main node to button and add a .model style class to it. The indicator
 * subnode is invisible in this case.
 */

struct _CtkModelButton
{
  CtkButton parent_instance;

  CtkWidget *box;
  CtkWidget *image;
  CtkWidget *label;
  CtkCssGadget *gadget;
  CtkCssGadget *indicator_gadget;
  gboolean active;
  gboolean centered;
  gboolean inverted;
  gboolean iconic;
  gchar *menu_name;
  CtkButtonRole role;
};

typedef CtkButtonClass CtkModelButtonClass;

G_DEFINE_TYPE (CtkModelButton, ctk_model_button, CTK_TYPE_BUTTON)

enum
{
  PROP_0,
  PROP_ROLE,
  PROP_ICON,
  PROP_TEXT,
  PROP_USE_MARKUP,
  PROP_ACTIVE,
  PROP_MENU_NAME,
  PROP_INVERTED,
  PROP_CENTERED,
  PROP_ICONIC,
  LAST_PROPERTY
};

static GParamSpec *properties[LAST_PROPERTY] = { NULL, };

static gboolean
indicator_is_left (CtkWidget *widget)
{
  CtkModelButton *button = CTK_MODEL_BUTTON (widget);

  return ((ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL && !button->inverted) ||
          (ctk_widget_get_direction (widget) == CTK_TEXT_DIR_LTR && button->inverted));
}

static void
ctk_model_button_update_state (CtkModelButton *button)
{
  CtkStateFlags state;
  CtkStateFlags indicator_state;
  CtkCssImageBuiltinType image_type = CTK_CSS_IMAGE_BUILTIN_NONE;

  state = ctk_widget_get_state_flags (CTK_WIDGET (button));
  indicator_state = state;

  ctk_css_gadget_set_state (button->gadget, state);

  if (button->role == CTK_BUTTON_ROLE_CHECK)
    {
      if (button->active && !button->menu_name)
        {
          indicator_state |= CTK_STATE_FLAG_CHECKED;
          image_type = CTK_CSS_IMAGE_BUILTIN_CHECK;
        }
      else
        {
          indicator_state &= ~CTK_STATE_FLAG_CHECKED;
        }
    }
  if (button->role == CTK_BUTTON_ROLE_RADIO)
    {
      if (button->active && !button->menu_name)
        {
          indicator_state |= CTK_STATE_FLAG_CHECKED;
          image_type = CTK_CSS_IMAGE_BUILTIN_OPTION;
        }
      else
        {
          indicator_state &= ~CTK_STATE_FLAG_CHECKED;
        }
    }

  if (button->menu_name)
    {
      if (indicator_is_left (CTK_WIDGET (button)))
        image_type = CTK_CSS_IMAGE_BUILTIN_ARROW_LEFT;
      else
        image_type = CTK_CSS_IMAGE_BUILTIN_ARROW_RIGHT;
    }

  ctk_builtin_icon_set_image (CTK_BUILTIN_ICON (button->indicator_gadget), image_type);

  if (button->iconic)
    ctk_css_gadget_set_state (button->gadget, indicator_state);
  else
    ctk_css_gadget_set_state (button->gadget, state);

  ctk_css_gadget_set_state (button->indicator_gadget, indicator_state);

  if (button->role == CTK_BUTTON_ROLE_CHECK ||
      button->role == CTK_BUTTON_ROLE_RADIO)
    {
      AtkObject *object = _ctk_widget_peek_accessible (CTK_WIDGET (button));
      if (object)
        atk_object_notify_state_change (object, ATK_STATE_CHECKED,
                                        (indicator_state & CTK_STATE_FLAG_CHECKED));
    }
}

static void
update_node_ordering (CtkModelButton *button)
{
  CtkCssNode *widget_node, *indicator_node, *node;

  widget_node = ctk_widget_get_css_node (CTK_WIDGET (button));
  indicator_node = ctk_css_gadget_get_node (button->indicator_gadget);

  if (indicator_is_left (CTK_WIDGET (button)))
    {
      ctk_css_gadget_add_class (button->indicator_gadget, CTK_STYLE_CLASS_LEFT);
      ctk_css_gadget_remove_class (button->indicator_gadget, CTK_STYLE_CLASS_RIGHT);

      node = ctk_css_node_get_first_child (widget_node);
      if (node != indicator_node)
        ctk_css_node_insert_before (widget_node, indicator_node, node);
    }
  else
    {
      ctk_css_gadget_remove_class (button->indicator_gadget, CTK_STYLE_CLASS_LEFT);
      ctk_css_gadget_add_class (button->indicator_gadget, CTK_STYLE_CLASS_RIGHT);

      node = ctk_css_node_get_last_child (widget_node);
      if (node != indicator_node)
        ctk_css_node_insert_after (widget_node, indicator_node, node);
    }
}

static void
ctk_model_button_state_flags_changed (CtkWidget     *widget,
                                      CtkStateFlags  previous_flags)
{
  ctk_model_button_update_state (CTK_MODEL_BUTTON (widget));

  CTK_WIDGET_CLASS (ctk_model_button_parent_class)->state_flags_changed (widget, previous_flags);
}

static void
ctk_model_button_direction_changed (CtkWidget        *widget,
                                    CtkTextDirection  previous_dir)
{
  CtkModelButton *button = CTK_MODEL_BUTTON (widget);

  ctk_model_button_update_state (button);
  update_node_ordering (button);

  CTK_WIDGET_CLASS (ctk_model_button_parent_class)->direction_changed (widget, previous_dir);
}

static void
update_node_name (CtkModelButton *button)
{
  AtkObject *accessible;
  AtkRole a11y_role;
  const gchar *indicator_name;
  gboolean indicator_visible;
  CtkCssNode *indicator_node;

  accessible = ctk_widget_get_accessible (CTK_WIDGET (button));
  switch (button->role)
    {
    case CTK_BUTTON_ROLE_NORMAL:
      a11y_role = ATK_ROLE_PUSH_BUTTON;
      if (button->menu_name)
        {
          indicator_name = I_("arrow");
          indicator_visible = TRUE;
        }
      else
        {
          indicator_name = I_("check");
          indicator_visible = FALSE;
        }
      break;

    case CTK_BUTTON_ROLE_CHECK:
      a11y_role = ATK_ROLE_CHECK_BOX;
      indicator_name = I_("check");
      indicator_visible = TRUE;
      break;

    case CTK_BUTTON_ROLE_RADIO:
      a11y_role = ATK_ROLE_RADIO_BUTTON;
      indicator_name = I_("radio");
      indicator_visible = TRUE;
      break;

    default:
      g_assert_not_reached ();
    }

  if (button->iconic)
    indicator_visible = FALSE;

  atk_object_set_role (accessible, a11y_role);

  indicator_node = ctk_css_gadget_get_node (button->indicator_gadget);
  ctk_css_node_set_name (indicator_node, indicator_name);
  ctk_css_node_set_visible (indicator_node, indicator_visible);
}

static void
ctk_model_button_set_role (CtkModelButton *button,
                           CtkButtonRole   role)
{
  if (role == button->role)
    return;

  button->role = role;

  update_node_name (button);

  ctk_model_button_update_state (button);
  ctk_widget_queue_draw (CTK_WIDGET (button));
  g_object_notify_by_pspec (G_OBJECT (button), properties[PROP_ROLE]);
}

static void
update_visibility (CtkModelButton *button)
{
  gboolean has_icon;
  gboolean has_text;

  has_icon = ctk_image_get_storage_type (CTK_IMAGE (button->image)) != CTK_IMAGE_EMPTY;
  has_text = ctk_label_get_text (CTK_LABEL (button->label))[0] != '\0';

  ctk_widget_set_visible (button->image, has_icon && (button->iconic || !has_text));
  ctk_widget_set_visible (button->label, has_text && (!button->iconic || !has_icon));
}

static void
ctk_model_button_set_icon (CtkModelButton *button,
                           GIcon          *icon)
{
  ctk_image_set_from_gicon (CTK_IMAGE (button->image), icon, CTK_ICON_SIZE_MENU);
  update_visibility (button);
  g_object_notify_by_pspec (G_OBJECT (button), properties[PROP_ICON]);
}

static void
ctk_model_button_set_text (CtkModelButton *button,
                           const gchar    *text)
{
  ctk_label_set_text_with_mnemonic (CTK_LABEL (button->label), text);
  update_visibility (button);
  g_object_notify_by_pspec (G_OBJECT (button), properties[PROP_TEXT]);
}

static void
ctk_model_button_set_use_markup (CtkModelButton *button,
                                 gboolean        use_markup)
{
  use_markup = !!use_markup;
  if (ctk_label_get_use_markup (CTK_LABEL (button->label)) == use_markup)
    return;

  ctk_label_set_use_markup (CTK_LABEL (button->label), use_markup);
  update_visibility (button);
  g_object_notify_by_pspec (G_OBJECT (button), properties[PROP_USE_MARKUP]);
}

static void
ctk_model_button_set_active (CtkModelButton *button,
                             gboolean        active)
{
  active = !!active;
  if (button->active == active)
    return;

  button->active = active;
  ctk_model_button_update_state (button);
  ctk_widget_queue_draw (CTK_WIDGET (button));
  g_object_notify_by_pspec (G_OBJECT (button), properties[PROP_ACTIVE]);
}

static void
ctk_model_button_set_menu_name (CtkModelButton *button,
                                const gchar    *menu_name)
{
  g_free (button->menu_name);
  button->menu_name = g_strdup (menu_name);

  update_node_name (button);
  ctk_model_button_update_state (button);

  ctk_widget_queue_resize (CTK_WIDGET (button));
  g_object_notify_by_pspec (G_OBJECT (button), properties[PROP_MENU_NAME]);
}

static void
ctk_model_button_set_inverted (CtkModelButton *button,
                               gboolean        inverted)
{
  inverted = !!inverted;
  if (button->inverted == inverted)
    return;

  button->inverted = inverted;
  ctk_model_button_update_state (button);
  update_node_ordering (button);
  ctk_widget_queue_resize (CTK_WIDGET (button));
  g_object_notify_by_pspec (G_OBJECT (button), properties[PROP_INVERTED]);
}

static void
ctk_model_button_set_centered (CtkModelButton *button,
                               gboolean        centered)
{
  centered = !!centered;
  if (button->centered == centered)
    return;

  button->centered = centered;
  ctk_widget_set_halign (button->box, button->centered ? CTK_ALIGN_CENTER : CTK_ALIGN_FILL);
  ctk_widget_queue_draw (CTK_WIDGET (button));
  g_object_notify_by_pspec (G_OBJECT (button), properties[PROP_CENTERED]);
}

static void
ctk_model_button_set_iconic (CtkModelButton *button,
                             gboolean        iconic)
{
  CtkCssNode *widget_node;
  CtkCssNode *indicator_node;

  iconic = !!iconic;
  if (button->iconic == iconic)
    return;

  button->iconic = iconic;

  widget_node = ctk_widget_get_css_node (CTK_WIDGET (button));
  indicator_node = ctk_css_gadget_get_node (button->indicator_gadget);
  if (iconic)
    {
      ctk_css_node_set_name (widget_node, I_("button"));
      ctk_css_gadget_add_class (button->gadget, "model");
      ctk_css_gadget_add_class (button->gadget, "image-button");
      ctk_button_set_relief (CTK_BUTTON (button), CTK_RELIEF_NORMAL);
      ctk_css_node_set_visible (indicator_node, FALSE);
    }
  else
    {
      ctk_css_node_set_name (widget_node, I_("modelbutton"));
      ctk_css_gadget_remove_class (button->gadget, "model");
      ctk_css_gadget_remove_class (button->gadget, "image-button");
      ctk_button_set_relief (CTK_BUTTON (button), CTK_RELIEF_NONE);
      ctk_css_node_set_visible (indicator_node,
                                button->role != CTK_BUTTON_ROLE_NORMAL ||
                                button->menu_name == NULL);
    }

  update_visibility (button);
  ctk_widget_queue_resize (CTK_WIDGET (button));
  g_object_notify_by_pspec (G_OBJECT (button), properties[PROP_ICONIC]);
}

static void
ctk_model_button_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  CtkModelButton *button = CTK_MODEL_BUTTON (object);

  switch (prop_id)
    {
    case PROP_ROLE:
      g_value_set_enum (value, button->role);
      break;

    case PROP_ICON:
      {
        GIcon *icon;
        ctk_image_get_gicon (CTK_IMAGE (button->image), &icon, NULL);
        g_value_set_object (value, icon);
      }
      break;

    case PROP_TEXT:
      g_value_set_string (value, ctk_label_get_text (CTK_LABEL (button->label)));
      break;

    case PROP_USE_MARKUP:
      g_value_set_boolean (value, ctk_label_get_use_markup (CTK_LABEL (button->label)));
      break;

    case PROP_ACTIVE:
      g_value_set_boolean (value, button->active);
      break;

    case PROP_MENU_NAME:
      g_value_set_string (value, button->menu_name);
      break;

    case PROP_INVERTED:
      g_value_set_boolean (value, button->inverted);
      break;

    case PROP_CENTERED:
      g_value_set_boolean (value, button->centered);
      break;

    case PROP_ICONIC:
      g_value_set_boolean (value, button->iconic);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_model_button_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  CtkModelButton *button = CTK_MODEL_BUTTON (object);

  switch (prop_id)
    {
    case PROP_ROLE:
      ctk_model_button_set_role (button, g_value_get_enum (value));
      break;

    case PROP_ICON:
      ctk_model_button_set_icon (button, g_value_get_object (value));
      break;

    case PROP_TEXT:
      ctk_model_button_set_text (button, g_value_get_string (value));
      break;

    case PROP_USE_MARKUP:
      ctk_model_button_set_use_markup (button, g_value_get_boolean (value));
      break;

    case PROP_ACTIVE:
      ctk_model_button_set_active (button, g_value_get_boolean (value));
      break;

    case PROP_MENU_NAME:
      ctk_model_button_set_menu_name (button, g_value_get_string (value));
      break;

    case PROP_INVERTED:
      ctk_model_button_set_inverted (button, g_value_get_boolean (value));
      break;

    case PROP_CENTERED:
      ctk_model_button_set_centered (button, g_value_get_boolean (value));
      break;

    case PROP_ICONIC:
      ctk_model_button_set_iconic (button, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static gboolean
has_sibling_with_indicator (CtkWidget *button)
{
  CtkWidget *parent;
  gboolean has_indicator;
  GList *children, *l;
  CtkModelButton *sibling;

  has_indicator = FALSE;

  parent = ctk_widget_get_parent (button);
  children = ctk_container_get_children (CTK_CONTAINER (parent));

  for (l = children; l; l = l->next)
    {
      sibling = l->data;

      if (!CTK_IS_MODEL_BUTTON (sibling))
        continue;

      if (!ctk_widget_is_visible (CTK_WIDGET (sibling)))
        continue;

      if (!sibling->centered &&
          (sibling->menu_name || sibling->role != CTK_BUTTON_ROLE_NORMAL))
        {
          has_indicator = TRUE;
          break;
        }
    }

  g_list_free (children);

  return has_indicator;
}

static gboolean
needs_indicator (CtkModelButton *button)
{
  if (button->role != CTK_BUTTON_ROLE_NORMAL)
    return TRUE;

  return has_sibling_with_indicator (CTK_WIDGET (button));
}

static void
ctk_model_button_get_preferred_width_for_height (CtkWidget *widget,
                                                 gint       height,
                                                 gint      *minimum,
                                                 gint      *natural)
{
  CtkCssGadget *gadget;

  if (CTK_MODEL_BUTTON (widget)->iconic)
    gadget = CTK_BUTTON (widget)->priv->gadget;
  else
    gadget = CTK_MODEL_BUTTON (widget)->gadget;

  ctk_css_gadget_get_preferred_size (gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     height,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_model_button_get_preferred_width (CtkWidget *widget,
                                      gint      *minimum,
                                      gint      *natural)
{
  CtkCssGadget *gadget;

  if (CTK_MODEL_BUTTON (widget)->iconic)
    gadget = CTK_BUTTON (widget)->priv->gadget;
  else
    gadget = CTK_MODEL_BUTTON (widget)->gadget;

  ctk_css_gadget_get_preferred_size (gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_model_button_get_preferred_height_and_baseline_for_width (CtkWidget *widget,
                                                              gint       width,
                                                              gint      *minimum,
                                                              gint      *natural,
                                                              gint      *minimum_baseline,
                                                              gint      *natural_baseline)
{
  CtkCssGadget *gadget;

  if (CTK_MODEL_BUTTON (widget)->iconic)
    gadget = CTK_BUTTON (widget)->priv->gadget;
  else
    gadget = CTK_MODEL_BUTTON (widget)->gadget;

  ctk_css_gadget_get_preferred_size (gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     width,
                                     minimum, natural,
                                     minimum_baseline, natural_baseline);
}

static void
ctk_model_button_get_preferred_height_for_width (CtkWidget *widget,
                                                 gint       width,
                                                 gint      *minimum,
                                                 gint      *natural)
{
  CtkCssGadget *gadget;

  if (CTK_MODEL_BUTTON (widget)->iconic)
    gadget = CTK_BUTTON (widget)->priv->gadget;
  else
    gadget = CTK_MODEL_BUTTON (widget)->gadget;

  ctk_css_gadget_get_preferred_size (gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     width,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_model_button_get_preferred_height (CtkWidget *widget,
                                       gint      *minimum,
                                       gint      *natural)
{
  CtkCssGadget *gadget;

  if (CTK_MODEL_BUTTON (widget)->iconic)
    gadget = CTK_BUTTON (widget)->priv->gadget;
  else
    gadget = CTK_MODEL_BUTTON (widget)->gadget;

  ctk_css_gadget_get_preferred_size (gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_model_button_measure (CtkCssGadget   *gadget,
                          CtkOrientation  orientation,
                          int             for_size,
                          int            *minimum,
                          int            *natural,
                          int            *minimum_baseline,
                          int            *natural_baseline,
                          gpointer        data)
{
  CtkWidget *widget;
  CtkModelButton *button;
  CtkWidget *child;

  widget = ctk_css_gadget_get_owner (gadget);
  button = CTK_MODEL_BUTTON (widget);
  child = ctk_bin_get_child (CTK_BIN (widget));

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      gint check_min, check_nat;

      ctk_css_gadget_get_preferred_size (button->indicator_gadget,
                                         CTK_ORIENTATION_HORIZONTAL,
                                         -1,
                                         &check_min, &check_nat,
                                         NULL, NULL);

      if (child && ctk_widget_get_visible (child))
        {
          _ctk_widget_get_preferred_size_for_size (child,
                                                   CTK_ORIENTATION_HORIZONTAL,
                                                   for_size,
                                                   minimum, natural,
                                                   minimum_baseline, natural_baseline);
        }
      else
        {
          *minimum = 0;
          *natural = 0;
        }

      if (button->centered)
        {
          *minimum += 2 * check_min;
          *natural += 2 * check_nat;
        }
      else if (needs_indicator (button))
        {
          *minimum += check_min;
          *natural += check_nat;
        }
    }
  else
    {
      gint check_min, check_nat;

      ctk_css_gadget_get_preferred_size (button->indicator_gadget,
                                         CTK_ORIENTATION_VERTICAL,
                                         -1,
                                         &check_min, &check_nat,
                                         NULL, NULL);

      if (child && ctk_widget_get_visible (child))
        {
          gint child_min, child_nat;
          gint child_min_baseline = -1, child_nat_baseline = -1;

          if (for_size > -1)
            {
              if (button->centered)
                for_size -= 2 * check_nat;
              else if (needs_indicator (button))
                for_size -= check_nat;
            }

          ctk_widget_get_preferred_height_and_baseline_for_width (child, for_size,
                                                                  &child_min, &child_nat,
                                                                  &child_min_baseline, &child_nat_baseline);

          if (button->centered)
            {
              *minimum = MAX (2 * check_min, child_min);
              *natural = MAX (2 * check_nat, child_nat);
            }
          else if (needs_indicator (button))
            {
              *minimum = MAX (check_min, child_min);
              *natural = MAX (check_nat, child_nat);
            }
          else
            {
              *minimum = child_min;
              *natural = child_nat;
            }

          if (minimum_baseline && child_min_baseline >= 0)
            *minimum_baseline = child_min_baseline + (*minimum - child_min) / 2;
          if (natural_baseline && child_nat_baseline >= 0)
            *natural_baseline = child_nat_baseline + (*natural - child_nat) / 2;
        }
      else
        {
          if (button->centered)
            {
              *minimum = 2 * check_min;
              *natural = 2 * check_nat;
            }
          else if (needs_indicator (button))
            {
              *minimum = check_min;
              *natural = check_nat;
            }
          else
            {
              *minimum = 0;
              *natural = 0;
            }
        }
    }
}

static void
ctk_model_button_size_allocate (CtkWidget     *widget,
                                CtkAllocation *allocation)
{
  CtkCssGadget *gadget;
  GdkRectangle clip;

  if (CTK_MODEL_BUTTON (widget)->iconic)
    gadget = CTK_BUTTON (widget)->priv->gadget;
  else
    gadget = CTK_MODEL_BUTTON (widget)->gadget;

  ctk_widget_set_allocation (widget, allocation);
  ctk_css_gadget_allocate (gadget,
                           allocation,
                           ctk_widget_get_allocated_baseline (widget),
                           &clip);

  ctk_widget_set_clip (widget, &clip);
}

static void
ctk_model_button_allocate (CtkCssGadget        *gadget,
                           const CtkAllocation *allocation,
                           int                  baseline,
                           CtkAllocation       *out_clip,
                           gpointer             unused)
{
  CtkWidget *widget;
  CtkModelButton *button;
  PangoContext *pango_context;
  PangoFontMetrics *metrics;
  CtkAllocation child_allocation;
  CtkWidget *child;
  gint check_min_width, check_nat_width;
  gint check_min_height, check_nat_height;
  GdkRectangle check_clip;

  widget = ctk_css_gadget_get_owner (gadget);
  button = CTK_MODEL_BUTTON (widget);
  child = ctk_bin_get_child (CTK_BIN (widget));



  ctk_css_gadget_get_preferred_size (button->indicator_gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     &check_min_width, &check_nat_width,
                                     NULL, NULL);
  ctk_css_gadget_get_preferred_size (button->indicator_gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     -1,
                                     &check_min_height, &check_nat_height,
                                     NULL, NULL);

  if (indicator_is_left (widget))
    child_allocation.x = allocation->x;
  else
    child_allocation.x = allocation->x + allocation->width - check_nat_width;
  child_allocation.y = allocation->y + (allocation->height - check_nat_height) / 2;
  child_allocation.width = check_nat_width;
  child_allocation.height = check_nat_height;

  ctk_css_gadget_allocate (button->indicator_gadget,
                           &child_allocation,
                           baseline,
                           &check_clip);

  if (child && ctk_widget_get_visible (child))
    {
      CtkBorder border = { 0, };

      if (button->centered)
        {
          border.left = check_nat_width;
          border.right = check_nat_width;
        }
      else if (needs_indicator (button))
        {
          if (indicator_is_left (widget))
            border.left += check_nat_width;
          else
            border.right += check_nat_width;
        }

      child_allocation.x = allocation->x + border.left;
      child_allocation.y = allocation->y + border.top;
      child_allocation.width = allocation->width - border.left - border.right;
      child_allocation.height = allocation->height - border.top - border.bottom;

      baseline = ctk_widget_get_allocated_baseline (widget);
      if (baseline != -1)
        baseline -= border.top;

      ctk_widget_size_allocate_with_baseline (child, &child_allocation, baseline);
    }

  pango_context = ctk_widget_get_pango_context (widget);
  metrics = pango_context_get_metrics (pango_context,
                                       pango_context_get_font_description (pango_context),
                                       pango_context_get_language (pango_context));
  CTK_BUTTON (button)->priv->baseline_align =
    (double)pango_font_metrics_get_ascent (metrics) /
    (pango_font_metrics_get_ascent (metrics) + pango_font_metrics_get_descent (metrics));
  pango_font_metrics_unref (metrics);


  if (ctk_widget_get_realized (widget))
    {
      CtkAllocation border_allocation;
      ctk_css_gadget_get_border_allocation (gadget, &border_allocation, NULL);

      cdk_window_move_resize (ctk_button_get_event_window (CTK_BUTTON (widget)),
                              border_allocation.x,
                              border_allocation.y,
                              border_allocation.width,
                              border_allocation.height);
    }

  ctk_container_get_children_clip (CTK_CONTAINER (widget), out_clip);
  cdk_rectangle_union (out_clip, &check_clip, out_clip);
}

static gint
ctk_model_button_draw (CtkWidget *widget,
                       cairo_t   *cr)
{
  CtkCssGadget *gadget;

  if (CTK_MODEL_BUTTON (widget)->iconic)
    gadget = CTK_BUTTON (widget)->priv->gadget;
  else
    gadget = CTK_MODEL_BUTTON (widget)->gadget;

  ctk_css_gadget_draw (gadget, cr);

  return FALSE;
}

static gboolean
ctk_model_button_render (CtkCssGadget *gadget,
                         cairo_t      *cr,
                         int           x,
                         int           y,
                         int           width,
                         int           height,
                         gpointer      data)
{
  CtkWidget *widget;
  CtkModelButton *button;
  CtkWidget *child;

  widget = ctk_css_gadget_get_owner (gadget);
  button = CTK_MODEL_BUTTON (widget);

  if (ctk_css_node_get_visible (ctk_css_gadget_get_node (button->indicator_gadget)))
    ctk_css_gadget_draw (button->indicator_gadget, cr);

  child = ctk_bin_get_child (CTK_BIN (widget));
  if (child)
    ctk_container_propagate_draw (CTK_CONTAINER (widget), child, cr);

  return ctk_widget_has_visible_focus (widget);
}

static void
ctk_model_button_destroy (CtkWidget *widget)
{
  CtkModelButton *model_button = CTK_MODEL_BUTTON (widget);

  g_clear_pointer (&model_button->menu_name, g_free);

  CTK_WIDGET_CLASS (ctk_model_button_parent_class)->destroy (widget);
}

static void
ctk_model_button_clicked (CtkButton *button)
{
  CtkModelButton *model_button = CTK_MODEL_BUTTON (button);

  if (model_button->menu_name != NULL)
    {
      CtkWidget *stack;

      stack = ctk_widget_get_ancestor (CTK_WIDGET (button), CTK_TYPE_STACK);
      if (stack != NULL)
        ctk_stack_set_visible_child_name (CTK_STACK (stack), model_button->menu_name);
    }
  else if (model_button->role == CTK_BUTTON_ROLE_NORMAL)
    {
      CtkWidget *popover;

      popover = ctk_widget_get_ancestor (CTK_WIDGET (button), CTK_TYPE_POPOVER);
      if (popover != NULL)
        ctk_popover_popdown (CTK_POPOVER (popover));
    }
}

static void
ctk_model_button_finalize (GObject *object)
{
  CtkModelButton *button = CTK_MODEL_BUTTON (object);

  g_clear_object (&button->indicator_gadget);
  g_clear_object (&button->gadget);

  G_OBJECT_CLASS (ctk_model_button_parent_class)->finalize (object);
}

static AtkObject *
ctk_model_button_get_accessible (CtkWidget *widget)
{
  AtkObject *object;

  object = CTK_WIDGET_CLASS (ctk_model_button_parent_class)->get_accessible (widget);

  ctk_model_button_update_state (CTK_MODEL_BUTTON (widget));

  return object;
}

static void
ctk_model_button_class_init (CtkModelButtonClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (class);
  CtkButtonClass *button_class = CTK_BUTTON_CLASS (class);

  object_class->finalize = ctk_model_button_finalize;
  object_class->get_property = ctk_model_button_get_property;
  object_class->set_property = ctk_model_button_set_property;

  widget_class->get_preferred_width = ctk_model_button_get_preferred_width;
  widget_class->get_preferred_width_for_height = ctk_model_button_get_preferred_width_for_height;
  widget_class->get_preferred_height = ctk_model_button_get_preferred_height;
  widget_class->get_preferred_height_for_width = ctk_model_button_get_preferred_height_for_width;
  widget_class->get_preferred_height_and_baseline_for_width = ctk_model_button_get_preferred_height_and_baseline_for_width;
  widget_class->size_allocate = ctk_model_button_size_allocate;
  widget_class->draw = ctk_model_button_draw;
  widget_class->destroy = ctk_model_button_destroy;
  widget_class->state_flags_changed = ctk_model_button_state_flags_changed;
  widget_class->direction_changed = ctk_model_button_direction_changed;
  widget_class->get_accessible = ctk_model_button_get_accessible;

  button_class->clicked = ctk_model_button_clicked;

  /**
   * CtkModelButton:role:
   *
   * Specifies whether the button is a plain, check or radio button.
   * When #CtkActionable:action-name is set, the role will be determined
   * from the action and does not have to be set explicitly.
   *
   * Since: 3.16
   */
  properties[PROP_ROLE] =
    g_param_spec_enum ("role",
                       P_("Role"),
                       P_("The role of this button"),
                       CTK_TYPE_BUTTON_ROLE,
                       CTK_BUTTON_ROLE_NORMAL,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * CtkModelButton:icon:
   *
   * A #GIcon that will be used if iconic appearance for the button is
   * desired.
   *
   * Since: 3.16
   */
  properties[PROP_ICON] = 
    g_param_spec_object ("icon",
                         P_("Icon"),
                         P_("The icon"),
                         G_TYPE_ICON,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * CtkModelButton:text:
   *
   * The label for the button.
   *
   * Since: 3.16
   */
  properties[PROP_TEXT] =
    g_param_spec_string ("text",
                         P_("Text"),
                         P_("The text"),
                         "",
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * CtkModelButton:use-markup:
   *
   * If %TRUE, XML tags in the text of the button are interpreted as by
   * pango_parse_markup() to format the enclosed spans of text. If %FALSE, the
   * text will be displayed verbatim.
   *
   * Since: 3.24
   */
  properties[PROP_USE_MARKUP] =
    g_param_spec_boolean ("use-markup",
                          P_("Use markup"),
                          P_("The text of the button includes XML markup. See pango_parse_markup()"),
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * CtkModelButton:active:
   *
   * The state of the button. This is reflecting the state of the associated
   * #GAction.
   *
   * Since: 3.16
   */
  properties[PROP_ACTIVE] =
    g_param_spec_boolean ("active",
                          P_("Active"),
                          P_("Active"),
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * CtkModelButton:menu-name:
   *
   * The name of a submenu to open when the button is activated.
   * If this is set, the button should not have an action associated with it.
   *
   * Since: 3.16
   */
  properties[PROP_MENU_NAME] =
    g_param_spec_string ("menu-name",
                         P_("Menu name"),
                         P_("The name of the menu to open"),
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * CtkModelButton:inverted:
   *
   * Whether to show the submenu indicator at the opposite side than normal.
   * This property should be set for model buttons that 'go back' to a parent
   * menu.
   *
   * Since: 3.16
   */
  properties[PROP_INVERTED] =
    g_param_spec_boolean ("inverted",
                          P_("Inverted"),
                          P_("Whether the menu is a parent"),
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * CtkModelButton:centered:
   *
   * Whether to render the button contents centered instead of left-aligned.
   * This property should be set for title-like items.
   *
   * Since: 3.16
   */
  properties[PROP_CENTERED] =
    g_param_spec_boolean ("centered",
                          P_("Centered"),
                          P_("Whether to center the contents"),
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * CtkModelButton:iconic:
   *
   * If this property is set, the button will show an icon if one is set.
   * If no icon is set, the text will be used. This is typically used for
   * horizontal sections of linked buttons.
   *
   * Since: 3.16
   */
  properties[PROP_ICONIC] =
    g_param_spec_boolean ("iconic",
                          P_("Iconic"),
                          P_("Whether to prefer the icon over text"),
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  g_object_class_install_properties (object_class, LAST_PROPERTY, properties);

  ctk_widget_class_set_accessible_role (CTK_WIDGET_CLASS (class), ATK_ROLE_PUSH_BUTTON);
  ctk_widget_class_set_css_name (CTK_WIDGET_CLASS (class), "modelbutton");
}

static void
ctk_model_button_init (CtkModelButton *button)
{
  CtkCssNode *widget_node;

  button->role = CTK_BUTTON_ROLE_NORMAL;
  ctk_button_set_relief (CTK_BUTTON (button), CTK_RELIEF_NONE);
  button->box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 6);
  ctk_widget_set_halign (button->box, CTK_ALIGN_FILL);
  ctk_widget_show (button->box);
  button->image = ctk_image_new ();
  ctk_widget_set_no_show_all (button->image, TRUE);
  button->label = ctk_label_new ("");
  ctk_widget_set_no_show_all (button->label, TRUE);
  ctk_container_add (CTK_CONTAINER (button->box), button->image);
  ctk_container_add (CTK_CONTAINER (button->box), button->label);
  ctk_container_add (CTK_CONTAINER (button), button->box);

  widget_node = ctk_widget_get_css_node (CTK_WIDGET (button));
  button->gadget = ctk_css_custom_gadget_new_for_node (widget_node,
                                                       CTK_WIDGET (button),
                                                       ctk_model_button_measure,
                                                       ctk_model_button_allocate,
                                                       ctk_model_button_render,
                                                       NULL,
                                                       NULL);
  button->indicator_gadget = ctk_builtin_icon_new ("check",
                                                   CTK_WIDGET (button),
                                                   button->gadget,
                                                   NULL);
  ctk_builtin_icon_set_default_size (CTK_BUILTIN_ICON (button->indicator_gadget), 16);
  update_node_ordering (button);
  ctk_css_node_set_visible (ctk_css_gadget_get_node (button->indicator_gadget), FALSE);
}

/**
 * ctk_model_button_new:
 *
 * Creates a new CtkModelButton.
 *
 * Returns: the newly created #CtkModelButton widget
 *
 * Since: 3.16
 */
CtkWidget *
ctk_model_button_new (void)
{
  return g_object_new (CTK_TYPE_MODEL_BUTTON, NULL);
}
