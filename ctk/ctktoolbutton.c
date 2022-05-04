/* ctktoolbutton.c
 *
 * Copyright (C) 2002 Anders Carlsson <andersca@gnome.org>
 * Copyright (C) 2002 James Henstridge <james@daa.com.au>
 * Copyright (C) 2003 Soeren Sandmann <sandmann@daimi.au.dk>
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

#include "config.h"
#include "ctktoolbutton.h"
#include "ctkbutton.h"
#include "ctkimage.h"
#include "ctkimagemenuitem.h"
#include "ctklabel.h"
#include "ctkstock.h"
#include "ctkbox.h"
#include "ctkintl.h"
#include "ctktoolbarprivate.h"
#include "ctkactivatable.h"
#include "ctkactionable.h"
#include "ctkprivate.h"

#include <string.h>


/**
 * SECTION:ctktoolbutton
 * @Short_description: A CtkToolItem subclass that displays buttons
 * @Title: CtkToolButton
 * @See_also: #CtkToolbar, #CtkMenuToolButton, #CtkToggleToolButton,
 *   #CtkRadioToolButton, #CtkSeparatorToolItem
 *
 * #CtkToolButtons are #CtkToolItems containing buttons.
 *
 * Use ctk_tool_button_new() to create a new #CtkToolButton.
 *
 * The label of a #CtkToolButton is determined by the properties
 * #CtkToolButton:label-widget, #CtkToolButton:label, and
 * #CtkToolButton:stock-id. If #CtkToolButton:label-widget is
 * non-%NULL, then that widget is used as the label. Otherwise, if
 * #CtkToolButton:label is non-%NULL, that string is used as the label.
 * Otherwise, if #CtkToolButton:stock-id is non-%NULL, the label is
 * determined by the stock item. Otherwise, the button does not have a label.
 *
 * The icon of a #CtkToolButton is determined by the properties
 * #CtkToolButton:icon-widget and #CtkToolButton:stock-id. If
 * #CtkToolButton:icon-widget is non-%NULL, then
 * that widget is used as the icon. Otherwise, if #CtkToolButton:stock-id is
 * non-%NULL, the icon is determined by the stock item. Otherwise,
 * the button does not have a icon.
 *
 * # CSS nodes
 *
 * CtkToolButton has a single CSS node with name toolbutton.
 */


#define MENU_ID "ctk-tool-button-menu-id"

enum {
  CLICKED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_LABEL,
  PROP_USE_UNDERLINE,
  PROP_LABEL_WIDGET,
  PROP_STOCK_ID,
  PROP_ICON_NAME,
  PROP_ICON_WIDGET,
  PROP_ACTION_NAME,
  PROP_ACTION_TARGET
};

static void ctk_tool_button_set_property  (GObject            *object,
					   guint               prop_id,
					   const GValue       *value,
					   GParamSpec         *pspec);
static void ctk_tool_button_get_property  (GObject            *object,
					   guint               prop_id,
					   GValue             *value,
					   GParamSpec         *pspec);
static void ctk_tool_button_property_notify (GObject          *object,
					     GParamSpec       *pspec);
static void ctk_tool_button_finalize      (GObject            *object);

static void ctk_tool_button_toolbar_reconfigured (CtkToolItem *tool_item);
static gboolean   ctk_tool_button_create_menu_proxy (CtkToolItem     *item);
static void       button_clicked                    (CtkWidget       *widget,
						     CtkToolButton   *button);
static void ctk_tool_button_style_updated  (CtkWidget          *widget);

static void ctk_tool_button_construct_contents (CtkToolItem *tool_item);

static void ctk_tool_button_actionable_iface_init      (CtkActionableInterface *iface);
static void ctk_tool_button_activatable_interface_init (CtkActivatableIface  *iface);
static void ctk_tool_button_update                     (CtkActivatable       *activatable,
							CtkAction            *action,
							const gchar          *property_name);
static void ctk_tool_button_sync_action_properties     (CtkActivatable       *activatable,
							CtkAction            *action);


struct _CtkToolButtonPrivate
{
  CtkWidget *button;

  gchar *stock_id;
  gchar *icon_name;
  gchar *label_text;
  CtkWidget *label_widget;
  CtkWidget *icon_widget;

  CtkSizeGroup *text_size_group;

  guint use_underline : 1;
  guint contents_invalid : 1;
};

static CtkActivatableIface *parent_activatable_iface;
static guint                toolbutton_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_CODE (CtkToolButton, ctk_tool_button, CTK_TYPE_TOOL_ITEM,
                         G_ADD_PRIVATE (CtkToolButton)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_ACTIONABLE, ctk_tool_button_actionable_iface_init)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_ACTIVATABLE, ctk_tool_button_activatable_interface_init))

static void
ctk_tool_button_class_init (CtkToolButtonClass *klass)
{
  GObjectClass *object_class;
  CtkWidgetClass *widget_class;
  CtkToolItemClass *tool_item_class;

  object_class = (GObjectClass *)klass;
  widget_class = (CtkWidgetClass *)klass;
  tool_item_class = (CtkToolItemClass *)klass;
  
  object_class->set_property = ctk_tool_button_set_property;
  object_class->get_property = ctk_tool_button_get_property;
  object_class->notify = ctk_tool_button_property_notify;
  object_class->finalize = ctk_tool_button_finalize;

  widget_class->style_updated = ctk_tool_button_style_updated;

  tool_item_class->create_menu_proxy = ctk_tool_button_create_menu_proxy;
  tool_item_class->toolbar_reconfigured = ctk_tool_button_toolbar_reconfigured;
  
  klass->button_type = CTK_TYPE_BUTTON;

  /* Properties are interpreted like this:
   *
   *          - if the tool button has an icon_widget, then that widget
   *            will be used as the icon. Otherwise, if the tool button
   *            has a stock id, the corresponding stock icon will be
   *            used. Otherwise, if the tool button has an icon name,
   *            the corresponding icon from the theme will be used.
   *            Otherwise, the tool button will not have an icon.
   *
   *          - if the tool button has a label_widget then that widget
   *            will be used as the label. Otherwise, if the tool button
   *            has a label text, that text will be used as label. Otherwise,
   *            if the toolbutton has a stock id, the corresponding text
   *            will be used as label. Otherwise, if the tool button has
   *            an icon name, the corresponding icon name from the theme will
   *            be used. Otherwise, the toolbutton will have an empty label.
   *
   *	      - The use_underline property only has an effect when the label
   *            on the toolbutton comes from the label property (ie. not from
   *            label_widget or from stock_id).
   *
   *            In that case, if use_underline is set,
   *
   *			- underscores are removed from the label text before
   *                      the label is shown on the toolbutton unless the
   *                      underscore is followed by another underscore
   *
   *			- an underscore indicates that the next character when
   *                      used in the overflow menu should be used as a
   *                      mnemonic.
   *
   *		In short: use_underline = TRUE means that the label text has
   *            the form "_Open" and the toolbar should take appropriate
   *            action.
   */

  g_object_class_install_property (object_class,
				   PROP_LABEL,
				   g_param_spec_string ("label",
							P_("Label"),
							P_("Text to show in the item."),
							NULL,
							CTK_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_USE_UNDERLINE,
				   g_param_spec_boolean ("use-underline",
							 P_("Use underline"),
							 P_("If set, an underline in the label property indicates that the next character should be used for the mnemonic accelerator key in the overflow menu"),
							 FALSE,
							 CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
  g_object_class_install_property (object_class,
				   PROP_LABEL_WIDGET,
				   g_param_spec_object ("label-widget",
							P_("Label widget"),
							P_("Widget to use as the item label"),
							CTK_TYPE_WIDGET,
							CTK_PARAM_READWRITE));
  /**
   * CtkToolButton:stock-id:
   *
   * Deprecated: 3.10: Use #CtkToolButton:icon-name instead.
   */
  g_object_class_install_property (object_class,
				   PROP_STOCK_ID,
				   g_param_spec_string ("stock-id",
							P_("Stock Id"),
							P_("The stock icon displayed on the item"),
							NULL,
							CTK_PARAM_READWRITE | G_PARAM_DEPRECATED));

  /**
   * CtkToolButton:icon-name:
   * 
   * The name of the themed icon displayed on the item.
   * This property only has an effect if not overridden by
   * #CtkToolButton:label-widget, #CtkToolButton:icon-widget or
   * #CtkToolButton:stock-id properties.
   *
   * Since: 2.8 
   */
  g_object_class_install_property (object_class,
				   PROP_ICON_NAME,
				   g_param_spec_string ("icon-name",
							P_("Icon name"),
							P_("The name of the themed icon displayed on the item"),
							NULL,
							CTK_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_ICON_WIDGET,
				   g_param_spec_object ("icon-widget",
							P_("Icon widget"),
							P_("Icon widget to display in the item"),
							CTK_TYPE_WIDGET,
							CTK_PARAM_READWRITE));

  g_object_class_override_property (object_class, PROP_ACTION_NAME, "action-name");
  g_object_class_override_property (object_class, PROP_ACTION_TARGET, "action-target");

  /**
   * CtkButton:icon-spacing:
   * 
   * Spacing in pixels between the icon and label.
   * 
   * Since: 2.10
   */
  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("icon-spacing",
							     P_("Icon spacing"),
							     P_("Spacing in pixels between the icon and label"),
							     0,
							     G_MAXINT,
							     3,
							     CTK_PARAM_READWRITE));

/**
 * CtkToolButton::clicked:
 * @toolbutton: the object that emitted the signal
 *
 * This signal is emitted when the tool button is clicked with the mouse
 * or activated with the keyboard.
 **/
  toolbutton_signals[CLICKED] =
    g_signal_new (I_("clicked"),
		  G_OBJECT_CLASS_TYPE (klass),
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkToolButtonClass, clicked),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);
  
  ctk_widget_class_set_css_name (widget_class, "toolbutton");
}

static void
ctk_tool_button_init (CtkToolButton      *button)
{
  CtkToolItem *toolitem = CTK_TOOL_ITEM (button);

  button->priv = ctk_tool_button_get_instance_private (button);

  button->priv->contents_invalid = TRUE;

  ctk_tool_item_set_homogeneous (toolitem, TRUE);

  /* create button */
  button->priv->button = g_object_new (CTK_TYPE_BUTTON, NULL);
  ctk_widget_set_focus_on_click (CTK_WIDGET (button->priv->button), FALSE);
  g_signal_connect_object (button->priv->button, "clicked",
			   G_CALLBACK (button_clicked), button, 0);

  ctk_container_add (CTK_CONTAINER (button), button->priv->button);
  ctk_widget_show (button->priv->button);
}

static void
ctk_tool_button_construct_contents (CtkToolItem *tool_item)
{
  CtkToolButton *button = CTK_TOOL_BUTTON (tool_item);
  CtkWidget *child;
  CtkWidget *label = NULL;
  CtkWidget *icon = NULL;
  CtkToolbarStyle style;
  gboolean need_label = FALSE;
  gboolean need_icon = FALSE;
  CtkIconSize icon_size;
  CtkWidget *box = NULL;
  guint icon_spacing;
  CtkOrientation text_orientation = CTK_ORIENTATION_HORIZONTAL;
  CtkSizeGroup *size_group = NULL;
  CtkWidget *parent;

  button->priv->contents_invalid = FALSE;

  ctk_widget_style_get (CTK_WIDGET (tool_item), 
			"icon-spacing", &icon_spacing,
			NULL);

  if (button->priv->icon_widget)
    {
      parent = ctk_widget_get_parent (button->priv->icon_widget);
      if (parent)
        {
          ctk_container_remove (CTK_CONTAINER (parent),
                                button->priv->icon_widget);
        }
    }

  if (button->priv->label_widget)
    {
      parent = ctk_widget_get_parent (button->priv->label_widget);
      if (parent)
        {
          ctk_container_remove (CTK_CONTAINER (parent),
                                button->priv->label_widget);
        }
    }

  child = ctk_bin_get_child (CTK_BIN (button->priv->button));
  if (child)
    {
      /* Note: we are not destroying the label_widget or icon_widget
       * here because they were removed from their containers above
       */
      ctk_widget_destroy (child);
    }

  style = ctk_tool_item_get_toolbar_style (CTK_TOOL_ITEM (button));
  
  if (style != CTK_TOOLBAR_TEXT)
    need_icon = TRUE;

  if (style != CTK_TOOLBAR_ICONS && style != CTK_TOOLBAR_BOTH_HORIZ)
    need_label = TRUE;

  if (style == CTK_TOOLBAR_BOTH_HORIZ &&
      (ctk_tool_item_get_is_important (CTK_TOOL_ITEM (button)) ||
       ctk_tool_item_get_orientation (CTK_TOOL_ITEM (button)) == CTK_ORIENTATION_VERTICAL ||
       ctk_tool_item_get_text_orientation (CTK_TOOL_ITEM (button)) == CTK_ORIENTATION_VERTICAL))
    {
      need_label = TRUE;
    }
  
  if (style != CTK_TOOLBAR_TEXT && button->priv->icon_widget == NULL &&
      button->priv->stock_id == NULL && button->priv->icon_name == NULL)
    {
      need_label = TRUE;
      need_icon = FALSE;
      style = CTK_TOOLBAR_TEXT;
    }

  if (style == CTK_TOOLBAR_TEXT && button->priv->label_widget == NULL &&
      button->priv->stock_id == NULL && button->priv->label_text == NULL)
    {
      need_label = FALSE;
      need_icon = TRUE;
      style = CTK_TOOLBAR_ICONS;
    }

  if (need_label)
    {
      if (button->priv->label_widget)
	{
	  label = button->priv->label_widget;
	}
      else
	{
	  CtkStockItem stock_item;
	  gboolean elide;
	  gchar *label_text;

	  if (button->priv->label_text)
	    {
	      label_text = button->priv->label_text;
	      elide = button->priv->use_underline;
	    }
	  else if (button->priv->stock_id && ctk_stock_lookup (button->priv->stock_id, &stock_item))
	    {
	      label_text = stock_item.label;
	      elide = TRUE;
	    }
	  else
	    {
	      label_text = "";
	      elide = FALSE;
	    }

	  if (elide)
	    label_text = _ctk_toolbar_elide_underscores (label_text);
	  else
	    label_text = g_strdup (label_text);

	  label = ctk_label_new (label_text);

	  g_free (label_text);
	  
	  ctk_widget_show (label);
	}

      if (CTK_IS_LABEL (label))
        {
          ctk_label_set_ellipsize (CTK_LABEL (label),
			           ctk_tool_item_get_ellipsize_mode (CTK_TOOL_ITEM (button)));
          text_orientation = ctk_tool_item_get_text_orientation (CTK_TOOL_ITEM (button));
          if (text_orientation == CTK_ORIENTATION_HORIZONTAL)
	    {
              gfloat align;

              ctk_label_set_angle (CTK_LABEL (label), 0);
              align = ctk_tool_item_get_text_alignment (CTK_TOOL_ITEM (button));
              if (align < 0.4)
                ctk_widget_set_halign (label, CTK_ALIGN_START);
              else if (align > 0.6)
                ctk_widget_set_halign (label, CTK_ALIGN_END);
              else
                ctk_widget_set_halign (label, CTK_ALIGN_CENTER);
            }
          else
            {
              gfloat align;

              ctk_label_set_ellipsize (CTK_LABEL (label), PANGO_ELLIPSIZE_NONE);
	      if (ctk_widget_get_direction (CTK_WIDGET (tool_item)) == CTK_TEXT_DIR_RTL)
	        ctk_label_set_angle (CTK_LABEL (label), -90);
	      else
	        ctk_label_set_angle (CTK_LABEL (label), 90);
              align = ctk_tool_item_get_text_alignment (CTK_TOOL_ITEM (button));
              if (align < 0.4)
                ctk_widget_set_valign (label, CTK_ALIGN_END);
              else if (align > 0.6)
                ctk_widget_set_valign (label, CTK_ALIGN_START);
              else
                ctk_widget_set_valign (label, CTK_ALIGN_CENTER);
            }
        }
    }

  icon_size = ctk_tool_item_get_icon_size (CTK_TOOL_ITEM (button));
  if (need_icon)
    {
      CtkIconSet *icon_set = NULL;

      if (button->priv->stock_id)
        {
          icon_set = ctk_icon_factory_lookup_default (button->priv->stock_id);
        }

      if (button->priv->icon_widget)
	{
	  icon = button->priv->icon_widget;
	  
	  if (CTK_IS_IMAGE (icon))
	    {
	      g_object_set (button->priv->icon_widget,
			    "icon-size", icon_size,
			    NULL);
	    }
	}
      else if (icon_set != NULL)
	{
          G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
	  icon = ctk_image_new_from_stock (button->priv->stock_id, icon_size);
          G_GNUC_END_IGNORE_DEPRECATIONS;
	  ctk_widget_show (icon);
	}
      else if (button->priv->icon_name)
	{
	  icon = ctk_image_new_from_icon_name (button->priv->icon_name, icon_size);
	  ctk_widget_show (icon);
	}

      if (icon)
	{
          if (text_orientation == CTK_ORIENTATION_HORIZONTAL)
            {
              gfloat align;

              align = ctk_tool_item_get_text_alignment (CTK_TOOL_ITEM (button));
              if (align > 0.6) 
                ctk_widget_set_halign (icon, CTK_ALIGN_START);
              else if (align < 0.4)
                ctk_widget_set_halign (icon, CTK_ALIGN_END);
              else
                ctk_widget_set_halign (icon, CTK_ALIGN_CENTER);
            }
          else
            {
              gfloat align;

              align = ctk_tool_item_get_text_alignment (CTK_TOOL_ITEM (button));
              if (align > 0.6) 
                ctk_widget_set_valign (icon, CTK_ALIGN_END);
              else if (align < 0.4)
                ctk_widget_set_valign (icon, CTK_ALIGN_START);
              else
               ctk_widget_set_valign (icon, CTK_ALIGN_CENTER);
            }

	  size_group = ctk_tool_item_get_text_size_group (CTK_TOOL_ITEM (button));
	  if (size_group != NULL)
	    ctk_size_group_add_widget (size_group, icon);
	}
    }

  switch (style)
    {
    case CTK_TOOLBAR_ICONS:
      if (icon)
        ctk_container_add (CTK_CONTAINER (button->priv->button), icon);
      ctk_style_context_add_class (ctk_widget_get_style_context (button->priv->button), "image-button");
      ctk_style_context_remove_class (ctk_widget_get_style_context (button->priv->button), "text-button");
      break;

    case CTK_TOOLBAR_BOTH:
      if (text_orientation == CTK_ORIENTATION_HORIZONTAL)
	box = ctk_box_new (CTK_ORIENTATION_VERTICAL, icon_spacing);
      else
	box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, icon_spacing);
      if (icon)
	ctk_box_pack_start (CTK_BOX (box), icon, TRUE, TRUE, 0);
      ctk_box_pack_end (CTK_BOX (box), label, FALSE, TRUE, 0);
      ctk_container_add (CTK_CONTAINER (button->priv->button), box);
      ctk_style_context_add_class (ctk_widget_get_style_context (button->priv->button), "image-button");
      ctk_style_context_add_class (ctk_widget_get_style_context (button->priv->button), "text-button");
      break;

    case CTK_TOOLBAR_BOTH_HORIZ:
      if (text_orientation == CTK_ORIENTATION_HORIZONTAL)
	{
	  box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, icon_spacing);
	  if (icon)
	    ctk_box_pack_start (CTK_BOX (box), icon, label? FALSE : TRUE, TRUE, 0);
	  if (label)
	    ctk_box_pack_end (CTK_BOX (box), label, TRUE, TRUE, 0);
	}
      else
	{
	  box = ctk_box_new (CTK_ORIENTATION_VERTICAL, icon_spacing);
	  if (icon)
	    ctk_box_pack_end (CTK_BOX (box), icon, label ? FALSE : TRUE, TRUE, 0);
	  if (label)
	    ctk_box_pack_start (CTK_BOX (box), label, TRUE, TRUE, 0);
	}
      ctk_container_add (CTK_CONTAINER (button->priv->button), box);
      ctk_style_context_add_class (ctk_widget_get_style_context (button->priv->button), "image-button");
      ctk_style_context_add_class (ctk_widget_get_style_context (button->priv->button), "text-button");
      break;

    case CTK_TOOLBAR_TEXT:
      ctk_container_add (CTK_CONTAINER (button->priv->button), label);
      ctk_style_context_add_class (ctk_widget_get_style_context (button->priv->button), "text-button");
      ctk_style_context_remove_class (ctk_widget_get_style_context (button->priv->button), "image-button");
      break;
    }

  if (box)
    ctk_widget_show (box);

  ctk_button_set_relief (CTK_BUTTON (button->priv->button),
			 ctk_tool_item_get_relief_style (CTK_TOOL_ITEM (button)));

  ctk_tool_item_rebuild_menu (tool_item);
  
  ctk_widget_queue_resize (CTK_WIDGET (button));
}

static void
ctk_tool_button_set_property (GObject         *object,
			      guint            prop_id,
			      const GValue    *value,
			      GParamSpec      *pspec)
{
  CtkToolButton *button = CTK_TOOL_BUTTON (object);
  
  switch (prop_id)
    {
    case PROP_LABEL:
      ctk_tool_button_set_label (button, g_value_get_string (value));
      break;
    case PROP_USE_UNDERLINE:
      ctk_tool_button_set_use_underline (button, g_value_get_boolean (value));
      break;
    case PROP_LABEL_WIDGET:
      ctk_tool_button_set_label_widget (button, g_value_get_object (value));
      break;
    case PROP_STOCK_ID:
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      ctk_tool_button_set_stock_id (button, g_value_get_string (value));
      G_GNUC_END_IGNORE_DEPRECATIONS;
      break;
    case PROP_ICON_NAME:
      ctk_tool_button_set_icon_name (button, g_value_get_string (value));
      break;
    case PROP_ICON_WIDGET:
      ctk_tool_button_set_icon_widget (button, g_value_get_object (value));
      break;
    case PROP_ACTION_NAME:
      g_object_set_property (G_OBJECT (button->priv->button), "action-name", value);
      break;
    case PROP_ACTION_TARGET:
      g_object_set_property (G_OBJECT (button->priv->button), "action-target", value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_tool_button_property_notify (GObject          *object,
				 GParamSpec       *pspec)
{
  CtkToolButton *button = CTK_TOOL_BUTTON (object);

  if (button->priv->contents_invalid ||
      strcmp ("is-important", pspec->name) == 0)
    ctk_tool_button_construct_contents (CTK_TOOL_ITEM (object));

  if (G_OBJECT_CLASS (ctk_tool_button_parent_class)->notify)
    G_OBJECT_CLASS (ctk_tool_button_parent_class)->notify (object, pspec);
}

static void
ctk_tool_button_get_property (GObject         *object,
			      guint            prop_id,
			      GValue          *value,
			      GParamSpec      *pspec)
{
  CtkToolButton *button = CTK_TOOL_BUTTON (object);

  switch (prop_id)
    {
    case PROP_LABEL:
      g_value_set_string (value, ctk_tool_button_get_label (button));
      break;
    case PROP_LABEL_WIDGET:
      g_value_set_object (value, ctk_tool_button_get_label_widget (button));
      break;
    case PROP_USE_UNDERLINE:
      g_value_set_boolean (value, ctk_tool_button_get_use_underline (button));
      break;
    case PROP_STOCK_ID:
      g_value_set_string (value, button->priv->stock_id);
      break;
    case PROP_ICON_NAME:
      g_value_set_string (value, button->priv->icon_name);
      break;
    case PROP_ICON_WIDGET:
      g_value_set_object (value, button->priv->icon_widget);
      break;
    case PROP_ACTION_NAME:
      g_object_get_property (G_OBJECT (button->priv->button), "action-name", value);
      break;
    case PROP_ACTION_TARGET:
      g_object_get_property (G_OBJECT (button->priv->button), "action-target", value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static const gchar *
ctk_tool_button_get_action_name (CtkActionable *actionable)
{
  CtkToolButton *button = CTK_TOOL_BUTTON (actionable);

  return ctk_actionable_get_action_name (CTK_ACTIONABLE (button->priv->button));
}

static void
ctk_tool_button_set_action_name (CtkActionable *actionable,
                                 const gchar   *action_name)
{
  CtkToolButton *button = CTK_TOOL_BUTTON (actionable);

  ctk_actionable_set_action_name (CTK_ACTIONABLE (button->priv->button), action_name);
}

static GVariant *
ctk_tool_button_get_action_target_value (CtkActionable *actionable)
{
  CtkToolButton *button = CTK_TOOL_BUTTON (actionable);

  return ctk_actionable_get_action_target_value (CTK_ACTIONABLE (button->priv->button));
}

static void
ctk_tool_button_set_action_target_value (CtkActionable *actionable,
                                         GVariant      *action_target)
{
  CtkToolButton *button = CTK_TOOL_BUTTON (actionable);

  ctk_actionable_set_action_target_value (CTK_ACTIONABLE (button->priv->button), action_target);
}

static void
ctk_tool_button_actionable_iface_init (CtkActionableInterface *iface)
{
  iface->get_action_name = ctk_tool_button_get_action_name;
  iface->set_action_name = ctk_tool_button_set_action_name;
  iface->get_action_target_value = ctk_tool_button_get_action_target_value;
  iface->set_action_target_value = ctk_tool_button_set_action_target_value;
}

static void
ctk_tool_button_finalize (GObject *object)
{
  CtkToolButton *button = CTK_TOOL_BUTTON (object);

  g_free (button->priv->stock_id);
  g_free (button->priv->icon_name);
  g_free (button->priv->label_text);

  if (button->priv->label_widget)
    g_object_unref (button->priv->label_widget);

  if (button->priv->icon_widget)
    g_object_unref (button->priv->icon_widget);
  
  G_OBJECT_CLASS (ctk_tool_button_parent_class)->finalize (object);
}

static CtkWidget *
clone_image_menu_size (CtkImage *image)
{
  CtkImageType storage_type = ctk_image_get_storage_type (image);

  if (storage_type == CTK_IMAGE_STOCK)
    {
      gchar *stock_id;
      CtkWidget *widget;
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      ctk_image_get_stock (image, &stock_id, NULL);
      widget = ctk_image_new_from_stock (stock_id, CTK_ICON_SIZE_MENU);
      G_GNUC_END_IGNORE_DEPRECATIONS;
      return widget;
    }
  else if (storage_type == CTK_IMAGE_ICON_NAME)
    {
      const gchar *icon_name;
      ctk_image_get_icon_name (image, &icon_name, NULL);
      return ctk_image_new_from_icon_name (icon_name, CTK_ICON_SIZE_MENU);
    }
  else if (storage_type == CTK_IMAGE_ICON_SET)
    {
      CtkWidget *widget;
      CtkIconSet *icon_set;
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      ctk_image_get_icon_set (image, &icon_set, NULL);
      widget = ctk_image_new_from_icon_set (icon_set, CTK_ICON_SIZE_MENU);
      G_GNUC_END_IGNORE_DEPRECATIONS;
      return widget;
    }
  else if (storage_type == CTK_IMAGE_GICON)
    {
      GIcon *icon;
      ctk_image_get_gicon (image, &icon, NULL);
      return ctk_image_new_from_gicon (icon, CTK_ICON_SIZE_MENU);
    }
  else if (storage_type == CTK_IMAGE_PIXBUF)
    {
      gint width, height;
      
      if (ctk_icon_size_lookup (CTK_ICON_SIZE_MENU, &width, &height))
	{
	  GdkPixbuf *src_pixbuf, *dest_pixbuf;
	  CtkWidget *cloned_image;

	  src_pixbuf = ctk_image_get_pixbuf (image);
	  dest_pixbuf = gdk_pixbuf_scale_simple (src_pixbuf, width, height,
						 GDK_INTERP_BILINEAR);

	  cloned_image = ctk_image_new_from_pixbuf (dest_pixbuf);
	  g_object_unref (dest_pixbuf);

	  return cloned_image;
	}
    }

  return NULL;
}
      
static gboolean
ctk_tool_button_create_menu_proxy (CtkToolItem *item)
{
  CtkToolButton *button = CTK_TOOL_BUTTON (item);
  CtkWidget *menu_item;
  CtkWidget *menu_image = NULL;
  CtkStockItem stock_item;
  gboolean use_mnemonic = TRUE;
  const char *label;

  if (_ctk_tool_item_create_menu_proxy (item))
    return TRUE;


  if (CTK_IS_LABEL (button->priv->label_widget))
    {
      label = ctk_label_get_label (CTK_LABEL (button->priv->label_widget));
      use_mnemonic = ctk_label_get_use_underline (CTK_LABEL (button->priv->label_widget));
    }
  else if (button->priv->label_text)
    {
      label = button->priv->label_text;
      use_mnemonic = button->priv->use_underline;
    }
  else if (button->priv->stock_id && ctk_stock_lookup (button->priv->stock_id, &stock_item))
    {
      label = stock_item.label;
    }
  else
    {
      label = "";
    }

  if (use_mnemonic)
    menu_item = ctk_image_menu_item_new_with_mnemonic (label);
  else
    menu_item = ctk_image_menu_item_new_with_label (label);

  if (CTK_IS_IMAGE (button->priv->icon_widget))
    {
      menu_image = clone_image_menu_size (CTK_IMAGE (button->priv->icon_widget));
    }
  else if (button->priv->stock_id)
    {
      menu_image = ctk_image_new_from_stock (button->priv->stock_id, CTK_ICON_SIZE_MENU);
    }

  if (menu_image)
    ctk_image_menu_item_set_image (CTK_IMAGE_MENU_ITEM (menu_item), menu_image);

  g_signal_connect_closure_by_id (menu_item,
				  g_signal_lookup ("activate", G_OBJECT_TYPE (menu_item)), 0,
				  g_cclosure_new_object_swap (G_CALLBACK (ctk_button_clicked),
							      G_OBJECT (CTK_TOOL_BUTTON (button)->priv->button)),
				  FALSE);

  ctk_tool_item_set_proxy_menu_item (CTK_TOOL_ITEM (button), MENU_ID, menu_item);
  
  return TRUE;
}

static void
button_clicked (CtkWidget     *widget,
		CtkToolButton *button)
{
  CtkAction *action;

  action = ctk_activatable_get_related_action (CTK_ACTIVATABLE (button));
  
  if (action)
    ctk_action_activate (action);

  g_signal_emit_by_name (button, "clicked");
}

static void
ctk_tool_button_toolbar_reconfigured (CtkToolItem *tool_item)
{
  ctk_tool_button_construct_contents (tool_item);
}

static void 
ctk_tool_button_update_icon_spacing (CtkToolButton *button)
{
  CtkWidget *box;
  guint spacing;

  box = ctk_bin_get_child (CTK_BIN (button->priv->button));
  if (CTK_IS_BOX (box))
    {
      ctk_widget_style_get (CTK_WIDGET (button), 
			    "icon-spacing", &spacing,
			    NULL);
      ctk_box_set_spacing (CTK_BOX (box), spacing);      
    }
}

static void
ctk_tool_button_style_updated (CtkWidget *widget)
{
  CTK_WIDGET_CLASS (G_OBJECT_CLASS (ctk_tool_button_parent_class))->style_updated (widget);

  ctk_tool_button_update_icon_spacing (CTK_TOOL_BUTTON (widget));
}

static void 
ctk_tool_button_activatable_interface_init (CtkActivatableIface  *iface)
{
  parent_activatable_iface = g_type_interface_peek_parent (iface);
  iface->update = ctk_tool_button_update;
  iface->sync_action_properties = ctk_tool_button_sync_action_properties;
}

static void
ctk_tool_button_update (CtkActivatable *activatable,
			CtkAction      *action,
			const gchar    *property_name)
{
  CtkToolButton *button;
  CtkWidget *image;
  gboolean use_action_appearance;

  parent_activatable_iface->update (activatable, action, property_name);

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  use_action_appearance = ctk_activatable_get_use_action_appearance (activatable);
  G_GNUC_END_IGNORE_DEPRECATIONS;

  if (!use_action_appearance)
    return;

  button = CTK_TOOL_BUTTON (activatable);
  
  if (strcmp (property_name, "short-label") == 0)
    ctk_tool_button_set_label (button, ctk_action_get_short_label (action));
  else if (strcmp (property_name, "stock-id") == 0)
    ctk_tool_button_set_stock_id (button, ctk_action_get_stock_id (action));
  else if (strcmp (property_name, "gicon") == 0)
    {
      const gchar *stock_id = ctk_action_get_stock_id (action);
      GIcon *icon = ctk_action_get_gicon (action);
      CtkIconSize icon_size = CTK_ICON_SIZE_BUTTON;
      CtkIconSet *icon_set = NULL;

      if (stock_id)
        {
          icon_set = ctk_icon_factory_lookup_default (stock_id);
        }

      if (icon_set != NULL || !icon)
	image = NULL;
      else 
	{   
	  image = ctk_tool_button_get_icon_widget (button);
	  icon_size = ctk_tool_item_get_icon_size (CTK_TOOL_ITEM (button));

	  if (!image)
	    image = ctk_image_new ();
	}

      ctk_tool_button_set_icon_widget (button, image);
      ctk_image_set_from_gicon (CTK_IMAGE (image), icon, icon_size);

    }
  else if (strcmp (property_name, "icon-name") == 0)
    ctk_tool_button_set_icon_name (button, ctk_action_get_icon_name (action));
}

static void
ctk_tool_button_sync_action_properties (CtkActivatable *activatable,
				        CtkAction      *action)
{
  CtkToolButton *button;
  GIcon         *icon;
  const gchar   *stock_id;
  CtkIconSet    *icon_set = NULL;

  parent_activatable_iface->sync_action_properties (activatable, action);

  if (!action)
    return;

  if (!ctk_activatable_get_use_action_appearance (activatable))
    return;

  button = CTK_TOOL_BUTTON (activatable);
  stock_id = ctk_action_get_stock_id (action);

  ctk_tool_button_set_label (button, ctk_action_get_short_label (action));
  ctk_tool_button_set_use_underline (button, TRUE);
  ctk_tool_button_set_stock_id (button, stock_id);
  ctk_tool_button_set_icon_name (button, ctk_action_get_icon_name (action));

  if (stock_id)
    {
      icon_set = ctk_icon_factory_lookup_default (stock_id);
    }

  if (icon_set != NULL)
      ctk_tool_button_set_icon_widget (button, NULL);
  else if ((icon = ctk_action_get_gicon (action)) != NULL)
    {
      CtkIconSize icon_size = ctk_tool_item_get_icon_size (CTK_TOOL_ITEM (button));
      CtkWidget  *image = ctk_tool_button_get_icon_widget (button);
      
      if (!image)
	{
	  image = ctk_image_new ();
	  ctk_widget_show (image);
	  ctk_tool_button_set_icon_widget (button, image);
	}

      ctk_image_set_from_gicon (CTK_IMAGE (image), icon, icon_size);
    }
  else if (ctk_action_get_icon_name (action))
    ctk_tool_button_set_icon_name (button, ctk_action_get_icon_name (action));
  else
    ctk_tool_button_set_label (button, ctk_action_get_short_label (action));
}

/**
 * ctk_tool_button_new_from_stock:
 * @stock_id: the name of the stock item 
 *
 * Creates a new #CtkToolButton containing the image and text from a
 * stock item. Some stock ids have preprocessor macros like #CTK_STOCK_OK
 * and #CTK_STOCK_APPLY.
 *
 * It is an error if @stock_id is not a name of a stock item.
 * 
 * Returns: A new #CtkToolButton
 * 
 * Since: 2.4
 *
 * Deprecated: 3.10: Use ctk_tool_button_new() together with
 * ctk_image_new_from_icon_name() instead.
 **/
CtkToolItem *
ctk_tool_button_new_from_stock (const gchar *stock_id)
{
  CtkToolButton *button;

  g_return_val_if_fail (stock_id != NULL, NULL);
    
  button = g_object_new (CTK_TYPE_TOOL_BUTTON,
			 "stock-id", stock_id,
			 NULL);

  return CTK_TOOL_ITEM (button);
}

/**
 * ctk_tool_button_new:
 * @label: (allow-none): a string that will be used as label, or %NULL
 * @icon_widget: (allow-none): a widget that will be used as the button contents, or %NULL
 *
 * Creates a new #CtkToolButton using @icon_widget as contents and @label as
 * label.
 *
 * Returns: A new #CtkToolButton
 * 
 * Since: 2.4
 **/
CtkToolItem *
ctk_tool_button_new (CtkWidget	 *icon_widget,
		     const gchar *label)
{
  CtkToolButton *button;

  g_return_val_if_fail (icon_widget == NULL || CTK_IS_WIDGET (icon_widget), NULL);

  button = g_object_new (CTK_TYPE_TOOL_BUTTON,
                         "label", label,
                         "icon-widget", icon_widget,
			 NULL);

  return CTK_TOOL_ITEM (button);  
}

/**
 * ctk_tool_button_set_label:
 * @button: a #CtkToolButton
 * @label: (allow-none): a string that will be used as label, or %NULL.
 *
 * Sets @label as the label used for the tool button. The #CtkToolButton:label
 * property only has an effect if not overridden by a non-%NULL 
 * #CtkToolButton:label-widget property. If both the #CtkToolButton:label-widget
 * and #CtkToolButton:label properties are %NULL, the label is determined by the
 * #CtkToolButton:stock-id property. If the #CtkToolButton:stock-id property is
 * also %NULL, @button will not have a label.
 * 
 * Since: 2.4
 **/
void
ctk_tool_button_set_label (CtkToolButton *button,
			   const gchar   *label)
{
  gchar *old_label;
  gchar *elided_label;
  AtkObject *accessible;
  
  g_return_if_fail (CTK_IS_TOOL_BUTTON (button));

  old_label = button->priv->label_text;

  button->priv->label_text = g_strdup (label);
  button->priv->contents_invalid = TRUE;     

  if (label)
    {
      elided_label = _ctk_toolbar_elide_underscores (label);
      accessible = ctk_widget_get_accessible (CTK_WIDGET (button->priv->button));
      atk_object_set_name (accessible, elided_label);
      g_free (elided_label);
    }

  g_free (old_label);
 
  g_object_notify (G_OBJECT (button), "label");
}

/**
 * ctk_tool_button_get_label:
 * @button: a #CtkToolButton
 * 
 * Returns the label used by the tool button, or %NULL if the tool button
 * doesn’t have a label. or uses a the label from a stock item. The returned
 * string is owned by CTK+, and must not be modified or freed.
 * 
 * Returns: (nullable): The label, or %NULL
 * 
 * Since: 2.4
 **/
const gchar *
ctk_tool_button_get_label (CtkToolButton *button)
{
  g_return_val_if_fail (CTK_IS_TOOL_BUTTON (button), NULL);

  return button->priv->label_text;
}

/**
 * ctk_tool_button_set_use_underline:
 * @button: a #CtkToolButton
 * @use_underline: whether the button label has the form “_Open”
 *
 * If set, an underline in the label property indicates that the next character
 * should be used for the mnemonic accelerator key in the overflow menu. For
 * example, if the label property is “_Open” and @use_underline is %TRUE,
 * the label on the tool button will be “Open” and the item on the overflow
 * menu will have an underlined “O”.
 * 
 * Labels shown on tool buttons never have mnemonics on them; this property
 * only affects the menu item on the overflow menu.
 * 
 * Since: 2.4
 **/
void
ctk_tool_button_set_use_underline (CtkToolButton *button,
				   gboolean       use_underline)
{
  g_return_if_fail (CTK_IS_TOOL_BUTTON (button));

  use_underline = use_underline != FALSE;

  if (use_underline != button->priv->use_underline)
    {
      button->priv->use_underline = use_underline;
      button->priv->contents_invalid = TRUE;

      g_object_notify (G_OBJECT (button), "use-underline");
    }
}

/**
 * ctk_tool_button_get_use_underline:
 * @button: a #CtkToolButton
 * 
 * Returns whether underscores in the label property are used as mnemonics
 * on menu items on the overflow menu. See ctk_tool_button_set_use_underline().
 * 
 * Returns: %TRUE if underscores in the label property are used as
 * mnemonics on menu items on the overflow menu.
 * 
 * Since: 2.4
 **/
gboolean
ctk_tool_button_get_use_underline (CtkToolButton *button)
{
  g_return_val_if_fail (CTK_IS_TOOL_BUTTON (button), FALSE);

  return button->priv->use_underline;
}

/**
 * ctk_tool_button_set_stock_id:
 * @button: a #CtkToolButton
 * @stock_id: (allow-none): a name of a stock item, or %NULL
 *
 * Sets the name of the stock item. See ctk_tool_button_new_from_stock().
 * The stock_id property only has an effect if not overridden by non-%NULL 
 * #CtkToolButton:label-widget and #CtkToolButton:icon-widget properties.
 * 
 * Since: 2.4
 *
 * Deprecated: 3.10: Use ctk_tool_button_set_icon_name() instead.
 **/
void
ctk_tool_button_set_stock_id (CtkToolButton *button,
			      const gchar   *stock_id)
{
  gchar *old_stock_id;
  
  g_return_if_fail (CTK_IS_TOOL_BUTTON (button));

  old_stock_id = button->priv->stock_id;

  button->priv->stock_id = g_strdup (stock_id);
  button->priv->contents_invalid = TRUE;

  g_free (old_stock_id);
  
  g_object_notify (G_OBJECT (button), "stock-id");
}

/**
 * ctk_tool_button_get_stock_id:
 * @button: a #CtkToolButton
 * 
 * Returns the name of the stock item. See ctk_tool_button_set_stock_id().
 * The returned string is owned by CTK+ and must not be freed or modifed.
 * 
 * Returns: the name of the stock item for @button.
 * 
 * Since: 2.4
 *
 * Deprecated: 3.10: Use ctk_tool_button_get_icon_name() instead.
 **/
const gchar *
ctk_tool_button_get_stock_id (CtkToolButton *button)
{
  g_return_val_if_fail (CTK_IS_TOOL_BUTTON (button), NULL);

  return button->priv->stock_id;
}

/**
 * ctk_tool_button_set_icon_name:
 * @button: a #CtkToolButton
 * @icon_name: (allow-none): the name of the themed icon
 *
 * Sets the icon for the tool button from a named themed icon.
 * See the docs for #CtkIconTheme for more details.
 * The #CtkToolButton:icon-name property only has an effect if not
 * overridden by non-%NULL #CtkToolButton:label-widget, 
 * #CtkToolButton:icon-widget and #CtkToolButton:stock-id properties.
 * 
 * Since: 2.8
 **/
void
ctk_tool_button_set_icon_name (CtkToolButton *button,
			       const gchar   *icon_name)
{
  gchar *old_icon_name;

  g_return_if_fail (CTK_IS_TOOL_BUTTON (button));

  old_icon_name = button->priv->icon_name;

  button->priv->icon_name = g_strdup (icon_name);
  button->priv->contents_invalid = TRUE; 

  g_free (old_icon_name);

  g_object_notify (G_OBJECT (button), "icon-name");
}

/**
 * ctk_tool_button_get_icon_name:
 * @button: a #CtkToolButton
 *
 * Returns the name of the themed icon for the tool button,
 * see ctk_tool_button_set_icon_name().
 *
 * Returns: (nullable): the icon name or %NULL if the tool button has
 * no themed icon
 *
 * Since: 2.8
 **/
const gchar*
ctk_tool_button_get_icon_name (CtkToolButton *button)
{
  g_return_val_if_fail (CTK_IS_TOOL_BUTTON (button), NULL);

  return button->priv->icon_name;
}

/**
 * ctk_tool_button_set_icon_widget:
 * @button: a #CtkToolButton
 * @icon_widget: (allow-none): the widget used as icon, or %NULL
 *
 * Sets @icon as the widget used as icon on @button. If @icon_widget is
 * %NULL the icon is determined by the #CtkToolButton:stock-id property. If the
 * #CtkToolButton:stock-id property is also %NULL, @button will not have an icon.
 * 
 * Since: 2.4
 **/
void
ctk_tool_button_set_icon_widget (CtkToolButton *button,
				 CtkWidget     *icon_widget)
{
  g_return_if_fail (CTK_IS_TOOL_BUTTON (button));
  g_return_if_fail (icon_widget == NULL || CTK_IS_WIDGET (icon_widget));

  if (icon_widget != button->priv->icon_widget)
    {
      if (button->priv->icon_widget)
	{
          CtkWidget *parent;

          parent = ctk_widget_get_parent (button->priv->icon_widget);
	  if (parent)
            ctk_container_remove (CTK_CONTAINER (parent),
                                  button->priv->icon_widget);

	  g_object_unref (button->priv->icon_widget);
	}
      
      if (icon_widget)
	g_object_ref_sink (icon_widget);

      button->priv->icon_widget = icon_widget;
      button->priv->contents_invalid = TRUE;
      
      g_object_notify (G_OBJECT (button), "icon-widget");
    }
}

/**
 * ctk_tool_button_set_label_widget:
 * @button: a #CtkToolButton
 * @label_widget: (allow-none): the widget used as label, or %NULL
 *
 * Sets @label_widget as the widget that will be used as the label
 * for @button. If @label_widget is %NULL the #CtkToolButton:label property is used
 * as label. If #CtkToolButton:label is also %NULL, the label in the stock item
 * determined by the #CtkToolButton:stock-id property is used as label. If
 * #CtkToolButton:stock-id is also %NULL, @button does not have a label.
 * 
 * Since: 2.4
 **/
void
ctk_tool_button_set_label_widget (CtkToolButton *button,
				  CtkWidget     *label_widget)
{
  g_return_if_fail (CTK_IS_TOOL_BUTTON (button));
  g_return_if_fail (label_widget == NULL || CTK_IS_WIDGET (label_widget));

  if (label_widget != button->priv->label_widget)
    {
      if (button->priv->label_widget)
	{
          CtkWidget *parent;

          parent = ctk_widget_get_parent (button->priv->label_widget);
          if (parent)
            ctk_container_remove (CTK_CONTAINER (parent),
                                  button->priv->label_widget);

	  g_object_unref (button->priv->label_widget);
	}
      
      if (label_widget)
	g_object_ref_sink (label_widget);

      button->priv->label_widget = label_widget;
      button->priv->contents_invalid = TRUE;
      
      g_object_notify (G_OBJECT (button), "label-widget");
    }
}

/**
 * ctk_tool_button_get_label_widget:
 * @button: a #CtkToolButton
 *
 * Returns the widget used as label on @button.
 * See ctk_tool_button_set_label_widget().
 *
 * Returns: (nullable) (transfer none): The widget used as label
 *     on @button, or %NULL.
 *
 * Since: 2.4
 **/
CtkWidget *
ctk_tool_button_get_label_widget (CtkToolButton *button)
{
  g_return_val_if_fail (CTK_IS_TOOL_BUTTON (button), NULL);

  return button->priv->label_widget;
}

/**
 * ctk_tool_button_get_icon_widget:
 * @button: a #CtkToolButton
 *
 * Return the widget used as icon widget on @button.
 * See ctk_tool_button_set_icon_widget().
 *
 * Returns: (nullable) (transfer none): The widget used as icon
 *     on @button, or %NULL.
 *
 * Since: 2.4
 **/
CtkWidget *
ctk_tool_button_get_icon_widget (CtkToolButton *button)
{
  g_return_val_if_fail (CTK_IS_TOOL_BUTTON (button), NULL);

  return button->priv->icon_widget;
}

CtkWidget *
_ctk_tool_button_get_button (CtkToolButton *button)
{
  g_return_val_if_fail (CTK_IS_TOOL_BUTTON (button), NULL);

  return button->priv->button;
}
