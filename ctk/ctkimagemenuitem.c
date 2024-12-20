/* CTK - The GIMP Toolkit
 * Copyright (C) 2001 Red Hat, Inc.
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

#include "ctkimagemenuitem.h"

#include "ctkmenuitemprivate.h"
#include "ctkaccellabel.h"
#include "ctkstock.h"
#include "ctkiconfactory.h"
#include "ctkimage.h"
#include "ctkmenubar.h"
#include "ctkcontainer.h"
#include "ctkwindow.h"
#include "ctkactivatable.h"

#include "ctkintl.h"
#include "ctkprivate.h"


/**
 * SECTION:ctkimagemenuitem
 * @Short_description: Widget for a menu item with an icon
 * @Title: CtkImageMenuItem
 *
 * A CtkImageMenuItem is a menu item which has an icon next to the text label.
 *
 * This is functionally equivalent to:
 *
 * |[<!-- language="C" -->
 *   CtkWidget *box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 6);
 *   CtkWidget *icon = ctk_image_new_from_icon_name ("folder-music-symbolic", CTK_ICON_SIZE_MENU);
 *   CtkWidget *label = ctk_label_new ("Music");
 *   CtkWidget *menu_item = ctk_menu_item_new ();
 *
 *   ctk_container_add (CTK_CONTAINER (box), icon);
 *   ctk_container_add (CTK_CONTAINER (box), label);
 *
 *   ctk_container_add (CTK_CONTAINER (menu_item), box);
 *
 *   ctk_widget_show_all (menu_item);
 * ]|
 *
 * Note that the user may disable display of menu icons using
 * the #CtkSettings:ctk-menu-images setting, so make sure to still
 * fill in the text label. If you want to ensure that your menu items
 * show an icon you are strongly encouraged to use a #CtkMenuItem
 * with a #CtkImage instead.
 *
 * An alternative way, if you want to
 * display an icon in a menu item, you should use #CtkMenuItem and pack a
 * #CtkBox with a #CtkImage and a #CtkLabel instead. You should also consider
 * using #CtkBuilder and the XML #GMenu description for creating menus, by
 * following the [GMenu guide][https://developer.gnome.org/GMenu/]. You should
 * consider using icons in menu items only sparingly, and for "objects" (or
 * "nouns") elements only, like bookmarks, files, and links; "actions" (or
 * "verbs") should not have icons.
 *
 * Furthermore, if you would like to display keyboard accelerator, you must
 * pack the accel label into the box using ctk_box_pack_end() and align the
 * label, otherwise the accelerator will not display correctly. The following
 * code snippet adds a keyboard accelerator to the menu item, with a key
 * binding of Ctrl+M:
 *
 * |[<!-- language="C" -->
 *   CtkWidget *box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 6);
 *   CtkWidget *icon = ctk_image_new_from_icon_name ("folder-music-symbolic", CTK_ICON_SIZE_MENU);
 *   CtkWidget *label = ctk_accel_label_new ("Music");
 *   CtkWidget *menu_item = ctk_menu_item_new ();
 *   CtkAccelGroup *accel_group = ctk_accel_group_new ();
 *
 *   ctk_container_add (CTK_CONTAINER (box), icon);
 *
 *   ctk_label_set_use_underline (CTK_LABEL (label), TRUE);
 *   ctk_label_set_xalign (CTK_LABEL (label), 0.0);
 *
 *   ctk_widget_add_accelerator (menu_item, "activate", accel_group,
 *                               CDK_KEY_m, CDK_CONTROL_MASK, CTK_ACCEL_VISIBLE);
 *   ctk_accel_label_set_accel_widget (CTK_ACCEL_LABEL (label), menu_item);
 *
 *   ctk_box_pack_end (CTK_BOX (box), label, TRUE, TRUE, 0);
 *
 *   ctk_container_add (CTK_CONTAINER (menu_item), box);
 *
 *   ctk_widget_show_all (menu_item);
 * ]|
 */


struct _CtkImageMenuItemPrivate
{
  CtkWidget     *image;

  gchar         *label;
  guint          use_stock         : 1;
  guint          always_show_image : 1;
};

enum {
  PROP_0,
  PROP_IMAGE,
  PROP_USE_STOCK,
  PROP_ACCEL_GROUP,
  PROP_ALWAYS_SHOW_IMAGE
};

static CtkActivatableIface *parent_activatable_iface;

static void ctk_image_menu_item_destroy              (CtkWidget        *widget);
static void ctk_image_menu_item_get_preferred_width  (CtkWidget        *widget,
                                                      gint             *minimum,
                                                      gint             *natural);
static void ctk_image_menu_item_get_preferred_height (CtkWidget        *widget,
                                                      gint             *minimum,
                                                      gint             *natural);
static void ctk_image_menu_item_get_preferred_height_for_width (CtkWidget *widget,
                                                                gint       width,
                                                                gint      *minimum,
                                                                gint      *natural);
static void ctk_image_menu_item_size_allocate        (CtkWidget        *widget,
                                                      CtkAllocation    *allocation);
static void ctk_image_menu_item_map                  (CtkWidget        *widget);
static void ctk_image_menu_item_remove               (CtkContainer     *container,
                                                      CtkWidget        *child);
static void ctk_image_menu_item_toggle_size_request  (CtkMenuItem      *menu_item,
                                                      gint             *requisition);
static void ctk_image_menu_item_set_label            (CtkMenuItem      *menu_item,
                                                      const gchar      *label);
static const gchar * ctk_image_menu_item_get_label   (CtkMenuItem *menu_item);

static void ctk_image_menu_item_forall               (CtkContainer    *container,
                                                      gboolean         include_internals,
                                                      CtkCallback      callback,
                                                      gpointer         callback_data);

static void ctk_image_menu_item_finalize             (GObject         *object);
static void ctk_image_menu_item_set_property         (GObject         *object,
                                                      guint            prop_id,
                                                      const GValue    *value,
                                                      GParamSpec      *pspec);
static void ctk_image_menu_item_get_property         (GObject         *object,
                                                      guint            prop_id,
                                                      GValue          *value,
                                                      GParamSpec      *pspec);
static void ctk_image_menu_item_screen_changed       (CtkWidget       *widget,
                                                      CdkScreen       *previous_screen);

static void ctk_image_menu_item_recalculate          (CtkImageMenuItem *image_menu_item);

static void ctk_image_menu_item_activatable_interface_init (CtkActivatableIface  *iface);
static void ctk_image_menu_item_update                     (CtkActivatable       *activatable,
                                                            CtkAction            *action,
                                                            const gchar          *property_name);
static void ctk_image_menu_item_sync_action_properties     (CtkActivatable       *activatable,
                                                            CtkAction            *action);


G_DEFINE_TYPE_WITH_CODE (CtkImageMenuItem, ctk_image_menu_item, CTK_TYPE_MENU_ITEM,
                         G_ADD_PRIVATE (CtkImageMenuItem)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_ACTIVATABLE,
                                                ctk_image_menu_item_activatable_interface_init))


static void
ctk_image_menu_item_class_init (CtkImageMenuItemClass *klass)
{
  GObjectClass *gobject_class = (GObjectClass*) klass;
  CtkWidgetClass *widget_class = (CtkWidgetClass*) klass;
  CtkMenuItemClass *menu_item_class = (CtkMenuItemClass*) klass;
  CtkContainerClass *container_class = (CtkContainerClass*) klass;

  widget_class->destroy = ctk_image_menu_item_destroy;
  widget_class->screen_changed = ctk_image_menu_item_screen_changed;
  widget_class->get_preferred_width = ctk_image_menu_item_get_preferred_width;
  widget_class->get_preferred_height = ctk_image_menu_item_get_preferred_height;
  widget_class->get_preferred_height_for_width = ctk_image_menu_item_get_preferred_height_for_width;
  widget_class->size_allocate = ctk_image_menu_item_size_allocate;
  widget_class->map = ctk_image_menu_item_map;

  container_class->forall = ctk_image_menu_item_forall;
  container_class->remove = ctk_image_menu_item_remove;

  menu_item_class->toggle_size_request = ctk_image_menu_item_toggle_size_request;
  menu_item_class->set_label           = ctk_image_menu_item_set_label;
  menu_item_class->get_label           = ctk_image_menu_item_get_label;

  gobject_class->finalize     = ctk_image_menu_item_finalize;
  gobject_class->set_property = ctk_image_menu_item_set_property;
  gobject_class->get_property = ctk_image_menu_item_get_property;

  /**
   * CtkImageMenuItem:image:
   *
   * Child widget to appear next to the menu text.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_IMAGE,
                                   g_param_spec_object ("image",
                                                        P_("Image widget"),
                                                        P_("Child widget to appear next to the menu text"),
                                                        CTK_TYPE_WIDGET,
                                                        CTK_PARAM_READWRITE));
  /**
   * CtkImageMenuItem:use-stock:
   *
   * If %TRUE, the label set in the menuitem is used as a
   * stock id to select the stock item for the item.
   *
   * Since: 2.16
   */
  g_object_class_install_property (gobject_class,
                                   PROP_USE_STOCK,
                                   g_param_spec_boolean ("use-stock",
                                                         P_("Use stock"),
                                                         P_("Whether to use the label text to create a stock menu item"),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  /**
   * CtkImageMenuItem:always-show-image:
   *
   * If %TRUE, the menu item will always show the image, if available.
   *
   * Use this property only if the menuitem would be useless or hard to use
   * without the image.
   *
   * Since: 2.16
   */
  g_object_class_install_property (gobject_class,
                                   PROP_ALWAYS_SHOW_IMAGE,
                                   g_param_spec_boolean ("always-show-image",
                                                         P_("Always show image"),
                                                         P_("Whether the image will always be shown"),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  /**
   * CtkImageMenuItem:accel-group:
   *
   * The Accel Group to use for stock accelerator keys
   *
   * Since: 2.16
   */
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_GROUP,
                                   g_param_spec_object ("accel-group",
                                                        P_("Accel Group"),
                                                        P_("The Accel Group to use for stock accelerator keys"),
                                                        CTK_TYPE_ACCEL_GROUP,
                                                        CTK_PARAM_WRITABLE));

}

static void
ctk_image_menu_item_init (CtkImageMenuItem *image_menu_item)
{
  CtkImageMenuItemPrivate *priv;

  image_menu_item->priv = ctk_image_menu_item_get_instance_private (image_menu_item);
  priv = image_menu_item->priv;

  priv->image = NULL;
  priv->use_stock = FALSE;
  priv->label  = NULL;
}

static void
ctk_image_menu_item_finalize (GObject *object)
{
  CtkImageMenuItemPrivate *priv = CTK_IMAGE_MENU_ITEM (object)->priv;

  g_free (priv->label);
  priv->label  = NULL;

  G_OBJECT_CLASS (ctk_image_menu_item_parent_class)->finalize (object);
}

static void
ctk_image_menu_item_set_property (GObject         *object,
                                  guint            prop_id,
                                  const GValue    *value,
                                  GParamSpec      *pspec)
{
  CtkImageMenuItem *image_menu_item = CTK_IMAGE_MENU_ITEM (object);

  switch (prop_id)
    {
    case PROP_IMAGE:
      ctk_image_menu_item_set_image (image_menu_item, (CtkWidget *) g_value_get_object (value));
      break;
    case PROP_USE_STOCK:
      ctk_image_menu_item_set_use_stock (image_menu_item, g_value_get_boolean (value));
      break;
    case PROP_ALWAYS_SHOW_IMAGE:
      ctk_image_menu_item_set_always_show_image (image_menu_item, g_value_get_boolean (value));
      break;
    case PROP_ACCEL_GROUP:
      ctk_image_menu_item_set_accel_group (image_menu_item, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_image_menu_item_get_property (GObject         *object,
                                  guint            prop_id,
                                  GValue          *value,
                                  GParamSpec      *pspec)
{
  CtkImageMenuItem *image_menu_item = CTK_IMAGE_MENU_ITEM (object);

  switch (prop_id)
    {
    case PROP_IMAGE:
      g_value_set_object (value, ctk_image_menu_item_get_image (image_menu_item));
      break;
    case PROP_USE_STOCK:
      g_value_set_boolean (value, ctk_image_menu_item_get_use_stock (image_menu_item));
      break;
    case PROP_ALWAYS_SHOW_IMAGE:
      g_value_set_boolean (value, ctk_image_menu_item_get_always_show_image (image_menu_item));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static gboolean
show_image (CtkImageMenuItem *image_menu_item)
{
  CtkImageMenuItemPrivate *priv = image_menu_item->priv;
  CtkSettings *settings = ctk_widget_get_settings (CTK_WIDGET (image_menu_item));
  gboolean show;

  if (priv->always_show_image)
    show = TRUE;
  else
    g_object_get (settings, "ctk-menu-images", &show, NULL);

  return show;
}

static void
ctk_image_menu_item_map (CtkWidget *widget)
{
  CtkImageMenuItem *image_menu_item = CTK_IMAGE_MENU_ITEM (widget);
  CtkImageMenuItemPrivate *priv = image_menu_item->priv;

  CTK_WIDGET_CLASS (ctk_image_menu_item_parent_class)->map (widget);

  if (priv->image)
    g_object_set (priv->image,
                  "visible", show_image (image_menu_item),
                  NULL);
}

static void
ctk_image_menu_item_destroy (CtkWidget *widget)
{
  CtkImageMenuItem *image_menu_item = CTK_IMAGE_MENU_ITEM (widget);
  CtkImageMenuItemPrivate *priv = image_menu_item->priv;

  if (priv->image)
    ctk_container_remove (CTK_CONTAINER (image_menu_item),
                          priv->image);

  CTK_WIDGET_CLASS (ctk_image_menu_item_parent_class)->destroy (widget);
}

static void
ctk_image_menu_item_toggle_size_request (CtkMenuItem *menu_item,
                                         gint        *requisition)
{
  CtkImageMenuItem *image_menu_item = CTK_IMAGE_MENU_ITEM (menu_item);
  CtkImageMenuItemPrivate *priv = image_menu_item->priv;
  CtkPackDirection pack_dir;
  CtkWidget *parent;
  CtkWidget *widget = CTK_WIDGET (menu_item);

  parent = ctk_widget_get_parent (widget);

  if (CTK_IS_MENU_BAR (parent))
    pack_dir = ctk_menu_bar_get_child_pack_direction (CTK_MENU_BAR (parent));
  else
    pack_dir = CTK_PACK_DIRECTION_LTR;

  *requisition = 0;

  if (priv->image && ctk_widget_get_visible (priv->image))
    {
      CtkRequisition image_requisition;
      guint toggle_spacing;

      ctk_widget_get_preferred_size (priv->image, &image_requisition, NULL);

      ctk_widget_style_get (CTK_WIDGET (menu_item),
                            "toggle-spacing", &toggle_spacing,
                            NULL);

      if (pack_dir == CTK_PACK_DIRECTION_LTR || pack_dir == CTK_PACK_DIRECTION_RTL)
        {
          if (image_requisition.width > 0)
            *requisition = image_requisition.width + toggle_spacing;
        }
      else
        {
          if (image_requisition.height > 0)
            *requisition = image_requisition.height + toggle_spacing;
        }
    }
}

static void
ctk_image_menu_item_recalculate (CtkImageMenuItem *image_menu_item)
{
  CtkImageMenuItemPrivate    *priv = image_menu_item->priv;
  CtkStockItem             stock_item;
  CtkWidget               *image;
  const gchar             *resolved_label = priv->label;

  if (priv->use_stock && priv->label)
    {
      if (!priv->image)
        {
          image = ctk_image_new_from_stock (priv->label, CTK_ICON_SIZE_MENU);
          ctk_image_menu_item_set_image (image_menu_item, image);
        }

      if (ctk_stock_lookup (priv->label, &stock_item))
          resolved_label = stock_item.label;

      ctk_menu_item_set_use_underline (CTK_MENU_ITEM (image_menu_item), TRUE);
    }

  CTK_MENU_ITEM_CLASS
    (ctk_image_menu_item_parent_class)->set_label (CTK_MENU_ITEM (image_menu_item), resolved_label);

}

static void
ctk_image_menu_item_set_label (CtkMenuItem      *menu_item,
                               const gchar      *label)
{
  CtkImageMenuItemPrivate *priv = CTK_IMAGE_MENU_ITEM (menu_item)->priv;

  if (priv->label != label)
    {
      g_free (priv->label);
      priv->label = g_strdup (label);

      ctk_image_menu_item_recalculate (CTK_IMAGE_MENU_ITEM (menu_item));

      g_object_notify (G_OBJECT (menu_item), "label");

    }
}

static const gchar *
ctk_image_menu_item_get_label (CtkMenuItem *menu_item)
{
  CtkImageMenuItemPrivate *priv = CTK_IMAGE_MENU_ITEM (menu_item)->priv;

  return priv->label;
}

static void
ctk_image_menu_item_get_preferred_width (CtkWidget        *widget,
                                         gint             *minimum,
                                         gint             *natural)
{
  CtkImageMenuItem *image_menu_item = CTK_IMAGE_MENU_ITEM (widget);
  CtkImageMenuItemPrivate *priv = image_menu_item->priv;
  CtkPackDirection pack_dir;
  CtkWidget *parent;

  parent = ctk_widget_get_parent (widget);

  if (CTK_IS_MENU_BAR (parent))
    pack_dir = ctk_menu_bar_get_child_pack_direction (CTK_MENU_BAR (parent));
  else
    pack_dir = CTK_PACK_DIRECTION_LTR;

  CTK_WIDGET_CLASS (ctk_image_menu_item_parent_class)->get_preferred_width (widget, minimum, natural);

  if ((pack_dir == CTK_PACK_DIRECTION_TTB || pack_dir == CTK_PACK_DIRECTION_BTT) &&
      priv->image &&
      ctk_widget_get_visible (priv->image))
    {
      gint child_minimum, child_natural;

      ctk_widget_get_preferred_width (priv->image, &child_minimum, &child_natural);

      *minimum = MAX (*minimum, child_minimum);
      *natural = MAX (*natural, child_natural);
    }
}

static void
ctk_image_menu_item_get_preferred_height (CtkWidget        *widget,
                                          gint             *minimum,
                                          gint             *natural)
{
  CtkImageMenuItem *image_menu_item = CTK_IMAGE_MENU_ITEM (widget);
  CtkImageMenuItemPrivate *priv = image_menu_item->priv;
  gint child_height = 0;
  CtkPackDirection pack_dir;
  CtkWidget *parent;

  parent = ctk_widget_get_parent (widget);

  if (CTK_IS_MENU_BAR (parent))
    pack_dir = ctk_menu_bar_get_child_pack_direction (CTK_MENU_BAR (parent));
  else
    pack_dir = CTK_PACK_DIRECTION_LTR;

  if (priv->image && ctk_widget_get_visible (priv->image))
    {
      CtkRequisition child_requisition;

      ctk_widget_get_preferred_size (priv->image, &child_requisition, NULL);

      child_height = child_requisition.height;
    }

  CTK_WIDGET_CLASS (ctk_image_menu_item_parent_class)->get_preferred_height (widget, minimum, natural);

  if (pack_dir == CTK_PACK_DIRECTION_RTL || pack_dir == CTK_PACK_DIRECTION_LTR)
    {
      *minimum = MAX (*minimum, child_height);
      *natural = MAX (*natural, child_height);
    }
}

static void
ctk_image_menu_item_get_preferred_height_for_width (CtkWidget        *widget,
                                                    gint              width,
                                                    gint             *minimum,
                                                    gint             *natural)
{
  CtkImageMenuItem *image_menu_item = CTK_IMAGE_MENU_ITEM (widget);
  CtkImageMenuItemPrivate *priv = image_menu_item->priv;
  gint child_height = 0;
  CtkPackDirection pack_dir;
  CtkWidget *parent;

  parent = ctk_widget_get_parent (widget);

  if (CTK_IS_MENU_BAR (parent))
    pack_dir = ctk_menu_bar_get_child_pack_direction (CTK_MENU_BAR (parent));
  else
    pack_dir = CTK_PACK_DIRECTION_LTR;

  if (priv->image && ctk_widget_get_visible (priv->image))
    {
      CtkRequisition child_requisition;

      ctk_widget_get_preferred_size (priv->image, &child_requisition, NULL);

      child_height = child_requisition.height;
    }

  CTK_WIDGET_CLASS
    (ctk_image_menu_item_parent_class)->get_preferred_height_for_width (widget, width, minimum, natural);

  if (pack_dir == CTK_PACK_DIRECTION_RTL || pack_dir == CTK_PACK_DIRECTION_LTR)
    {
      *minimum = MAX (*minimum, child_height);
      *natural = MAX (*natural, child_height);
    }
}


static void
ctk_image_menu_item_size_allocate (CtkWidget     *widget,
                                   CtkAllocation *allocation)
{
  CtkImageMenuItem *image_menu_item = CTK_IMAGE_MENU_ITEM (widget);
  CtkImageMenuItemPrivate *priv = image_menu_item->priv;
  CtkAllocation widget_allocation;
  CtkPackDirection pack_dir;
  CtkWidget *parent;

  parent = ctk_widget_get_parent (widget);

  if (CTK_IS_MENU_BAR (parent))
    pack_dir = ctk_menu_bar_get_child_pack_direction (CTK_MENU_BAR (parent));
  else
    pack_dir = CTK_PACK_DIRECTION_LTR;

  CTK_WIDGET_CLASS (ctk_image_menu_item_parent_class)->size_allocate (widget, allocation);

  if (priv->image && ctk_widget_get_visible (priv->image))
    {
      gint x, y, offset;
      CtkStyleContext *context;
      CtkStateFlags state;
      CtkBorder padding;
      CtkRequisition child_requisition;
      CtkAllocation child_allocation;
      guint toggle_spacing;
      gint toggle_size;

      toggle_size = CTK_MENU_ITEM (image_menu_item)->priv->toggle_size;
      ctk_widget_style_get (widget,
                            "toggle-spacing", &toggle_spacing,
                            NULL);

      /* Man this is lame hardcoding action, but I can't
       * come up with a solution that's really better.
       */

      ctk_widget_get_preferred_size (priv->image, &child_requisition, NULL);

      ctk_widget_get_allocation (widget, &widget_allocation);

      context = ctk_widget_get_style_context (widget);
      state = ctk_widget_get_state_flags (widget);
      ctk_style_context_get_padding (context, state, &padding);
      offset = ctk_container_get_border_width (CTK_CONTAINER (image_menu_item));

      if (pack_dir == CTK_PACK_DIRECTION_LTR ||
          pack_dir == CTK_PACK_DIRECTION_RTL)
        {
          if ((ctk_widget_get_direction (widget) == CTK_TEXT_DIR_LTR) ==
              (pack_dir == CTK_PACK_DIRECTION_LTR))
            x = offset + padding.left +
               (toggle_size - toggle_spacing - child_requisition.width) / 2;
          else
            x = widget_allocation.width - offset - padding.right -
              toggle_size + toggle_spacing +
              (toggle_size - toggle_spacing - child_requisition.width) / 2;

          y = (widget_allocation.height - child_requisition.height) / 2;
        }
      else
        {
          if ((ctk_widget_get_direction (widget) == CTK_TEXT_DIR_LTR) ==
              (pack_dir == CTK_PACK_DIRECTION_TTB))
            y = offset + padding.top +
              (toggle_size - toggle_spacing - child_requisition.height) / 2;
          else
            y = widget_allocation.height - offset - padding.bottom -
              toggle_size + toggle_spacing +
              (toggle_size - toggle_spacing - child_requisition.height) / 2;

          x = (widget_allocation.width - child_requisition.width) / 2;
        }

      child_allocation.width = child_requisition.width;
      child_allocation.height = child_requisition.height;
      child_allocation.x = widget_allocation.x + MAX (x, 0);
      child_allocation.y = widget_allocation.y + MAX (y, 0);

      ctk_widget_size_allocate (priv->image, &child_allocation);
    }
}

static void
ctk_image_menu_item_forall (CtkContainer   *container,
                            gboolean        include_internals,
                            CtkCallback     callback,
                            gpointer        callback_data)
{
  CtkImageMenuItem *image_menu_item = CTK_IMAGE_MENU_ITEM (container);
  CtkImageMenuItemPrivate *priv = image_menu_item->priv;

  CTK_CONTAINER_CLASS (ctk_image_menu_item_parent_class)->forall (container,
                                                                  include_internals,
                                                                  callback,
                                                                  callback_data);

  if (include_internals && priv->image)
    (* callback) (priv->image, callback_data);
}


static void
ctk_image_menu_item_activatable_interface_init (CtkActivatableIface  *iface)
{
  parent_activatable_iface = g_type_interface_peek_parent (iface);
  iface->update = ctk_image_menu_item_update;
  iface->sync_action_properties = ctk_image_menu_item_sync_action_properties;
}

static CtkWidget *
ctk_image_menu_item_ensure_image (CtkImageMenuItem *item)
{
  CtkWidget *image;

  image = ctk_image_menu_item_get_image (item);
  if (!CTK_IS_IMAGE (image))
    {
      image = ctk_image_new ();
      ctk_widget_show (image);
      ctk_image_menu_item_set_image (item, image);
    }

  return image;
}

static gboolean
activatable_update_stock_id (CtkImageMenuItem *image_menu_item, CtkAction *action)
{
  const gchar *stock_id  = ctk_action_get_stock_id (action);

  if (stock_id && ctk_icon_factory_lookup_default (stock_id))
    {
      CtkWidget *image;

      image = ctk_image_menu_item_ensure_image (image_menu_item);
      ctk_image_set_from_stock (CTK_IMAGE (image), stock_id, CTK_ICON_SIZE_MENU);
      return TRUE;
    }

  return FALSE;
}

static gboolean
activatable_update_gicon (CtkImageMenuItem *image_menu_item, CtkAction *action)
{
  GIcon       *icon = ctk_action_get_gicon (action);
  const gchar *stock_id;
  gboolean     ret = FALSE;

  stock_id = ctk_action_get_stock_id (action);

  if (icon && !(stock_id && ctk_icon_factory_lookup_default (stock_id)))
    {
      CtkWidget *image;

      image = ctk_image_menu_item_ensure_image (image_menu_item);
      ctk_image_set_from_gicon (CTK_IMAGE (image), icon, CTK_ICON_SIZE_MENU);
      ret = TRUE;
    }

  return ret;
}

static gboolean
activatable_update_icon_name (CtkImageMenuItem *image_menu_item, CtkAction *action)
{
  const gchar *icon_name = ctk_action_get_icon_name (action);

  if (icon_name)
    {
      CtkWidget *image;

      image = ctk_image_menu_item_ensure_image (image_menu_item);
      ctk_image_set_from_icon_name (CTK_IMAGE (image), icon_name, CTK_ICON_SIZE_MENU);
      return TRUE;
    }

  return FALSE;
}

static void
ctk_image_menu_item_update (CtkActivatable *activatable,
                            CtkAction      *action,
                            const gchar    *property_name)
{
  CtkImageMenuItem *image_menu_item;
  gboolean   use_appearance;

  image_menu_item = CTK_IMAGE_MENU_ITEM (activatable);

  parent_activatable_iface->update (activatable, action, property_name);

  use_appearance = ctk_activatable_get_use_action_appearance (activatable);
  if (!use_appearance)
    return;

  if (strcmp (property_name, "stock-id") == 0)
    activatable_update_stock_id (image_menu_item, action);
  else if (strcmp (property_name, "gicon") == 0)
    activatable_update_gicon (image_menu_item, action);
  else if (strcmp (property_name, "icon-name") == 0)
    activatable_update_icon_name (image_menu_item, action);
}

static void
ctk_image_menu_item_sync_action_properties (CtkActivatable *activatable,
                                            CtkAction      *action)
{
  CtkImageMenuItem *image_menu_item;
  gboolean   use_appearance;

  image_menu_item = CTK_IMAGE_MENU_ITEM (activatable);

  parent_activatable_iface->sync_action_properties (activatable, action);

  if (!action)
    return;

  use_appearance = ctk_activatable_get_use_action_appearance (activatable);
  if (!use_appearance)
    return;

  if (!activatable_update_stock_id (image_menu_item, action) &&
      !activatable_update_gicon (image_menu_item, action))
    activatable_update_icon_name (image_menu_item, action);

  ctk_image_menu_item_set_always_show_image (image_menu_item,
                                             ctk_action_get_always_show_image (action));
}


/**
 * ctk_image_menu_item_new:
 *
 * Creates a new #CtkImageMenuItem with an empty label.
 *
 * Returns: a new #CtkImageMenuItem
 */
CtkWidget*
ctk_image_menu_item_new (void)
{
  return g_object_new (CTK_TYPE_IMAGE_MENU_ITEM, NULL);
}

/**
 * ctk_image_menu_item_new_with_label:
 * @label: the text of the menu item.
 *
 * Creates a new #CtkImageMenuItem containing a label.
 *
 * Returns: a new #CtkImageMenuItem.
 */
CtkWidget*
ctk_image_menu_item_new_with_label (const gchar *label)
{
  return g_object_new (CTK_TYPE_IMAGE_MENU_ITEM,
                       "label", label,
                       NULL);
}

/**
 * ctk_image_menu_item_new_with_mnemonic:
 * @label: the text of the menu item, with an underscore in front of the
 *         mnemonic character
 *
 * Creates a new #CtkImageMenuItem containing a label. The label
 * will be created using ctk_label_new_with_mnemonic(), so underscores
 * in @label indicate the mnemonic for the menu item.
 *
 * Returns: a new #CtkImageMenuItem
 */
CtkWidget*
ctk_image_menu_item_new_with_mnemonic (const gchar *label)
{
  return g_object_new (CTK_TYPE_IMAGE_MENU_ITEM,
                       "use-underline", TRUE,
                       "label", label,
                       NULL);
}

/**
 * ctk_image_menu_item_new_from_stock:
 * @stock_id: the name of the stock item.
 * @accel_group: (allow-none): the #CtkAccelGroup to add the menu items
 *   accelerator to, or %NULL.
 *
 * Creates a new #CtkImageMenuItem containing the image and text from a
 * stock item. Some stock ids have preprocessor macros like #CTK_STOCK_OK
 * and #CTK_STOCK_APPLY.
 *
 * If you want this menu item to have changeable accelerators, then pass in
 * %NULL for accel_group. Next call ctk_menu_item_set_accel_path() with an
 * appropriate path for the menu item, use ctk_stock_lookup() to look up the
 * standard accelerator for the stock item, and if one is found, call
 * ctk_accel_map_add_entry() to register it.
 *
 * Returns: a new #CtkImageMenuItem.
 */
CtkWidget*
ctk_image_menu_item_new_from_stock (const gchar   *stock_id,
                                    CtkAccelGroup *accel_group)
{
  return g_object_new (CTK_TYPE_IMAGE_MENU_ITEM,
                       "label", stock_id,
                       "use-stock", TRUE,
                       "accel-group", accel_group,
                       NULL);
}

/**
 * ctk_image_menu_item_set_use_stock:
 * @image_menu_item: a #CtkImageMenuItem
 * @use_stock: %TRUE if the menuitem should use a stock item
 *
 * If %TRUE, the label set in the menuitem is used as a
 * stock id to select the stock item for the item.
 *
 * Since: 2.16
 */
void
ctk_image_menu_item_set_use_stock (CtkImageMenuItem *image_menu_item,
                                   gboolean          use_stock)
{
  CtkImageMenuItemPrivate *priv;

  g_return_if_fail (CTK_IS_IMAGE_MENU_ITEM (image_menu_item));

  priv = image_menu_item->priv;

  if (priv->use_stock != use_stock)
    {
      priv->use_stock = use_stock;

      ctk_image_menu_item_recalculate (image_menu_item);

      g_object_notify (G_OBJECT (image_menu_item), "use-stock");
    }
}

/**
 * ctk_image_menu_item_get_use_stock:
 * @image_menu_item: a #CtkImageMenuItem
 *
 * Checks whether the label set in the menuitem is used as a
 * stock id to select the stock item for the item.
 *
 * Returns: %TRUE if the label set in the menuitem is used as a
 *     stock id to select the stock item for the item
 *
 * Since: 2.16
 */
gboolean
ctk_image_menu_item_get_use_stock (CtkImageMenuItem *image_menu_item)
{
  g_return_val_if_fail (CTK_IS_IMAGE_MENU_ITEM (image_menu_item), FALSE);

  return image_menu_item->priv->use_stock;
}

/**
 * ctk_image_menu_item_set_always_show_image:
 * @image_menu_item: a #CtkImageMenuItem
 * @always_show: %TRUE if the menuitem should always show the image
 *
 * If %TRUE, the menu item will ignore the #CtkSettings:ctk-menu-images
 * setting and always show the image, if available.
 *
 * Use this property if the menuitem would be useless or hard to use
 * without the image.
 *
 * Since: 2.16
 */
void
ctk_image_menu_item_set_always_show_image (CtkImageMenuItem *image_menu_item,
                                           gboolean          always_show)
{
  CtkImageMenuItemPrivate *priv;

  g_return_if_fail (CTK_IS_IMAGE_MENU_ITEM (image_menu_item));

  priv = image_menu_item->priv;

  if (priv->always_show_image != always_show)
    {
      priv->always_show_image = always_show;

      if (priv->image)
        {
          if (show_image (image_menu_item))
            ctk_widget_show (priv->image);
          else
            ctk_widget_hide (priv->image);
        }

      g_object_notify (G_OBJECT (image_menu_item), "always-show-image");
    }
}

/**
 * ctk_image_menu_item_get_always_show_image:
 * @image_menu_item: a #CtkImageMenuItem
 *
 * Returns whether the menu item will ignore the #CtkSettings:ctk-menu-images
 * setting and always show the image, if available.
 *
 * Returns: %TRUE if the menu item will always show the image
 *
 * Since: 2.16
 */
gboolean
ctk_image_menu_item_get_always_show_image (CtkImageMenuItem *image_menu_item)
{
  g_return_val_if_fail (CTK_IS_IMAGE_MENU_ITEM (image_menu_item), FALSE);

  return image_menu_item->priv->always_show_image;
}


/**
 * ctk_image_menu_item_set_accel_group:
 * @image_menu_item: a #CtkImageMenuItem
 * @accel_group: the #CtkAccelGroup
 *
 * Specifies an @accel_group to add the menu items accelerator to
 * (this only applies to stock items so a stock item must already
 * be set, make sure to call ctk_image_menu_item_set_use_stock()
 * and ctk_menu_item_set_label() with a valid stock item first).
 *
 * If you want this menu item to have changeable accelerators then
 * you shouldnt need this (see ctk_image_menu_item_new_from_stock()).
 *
 * Since: 2.16
 */
void
ctk_image_menu_item_set_accel_group (CtkImageMenuItem *image_menu_item,
                                     CtkAccelGroup    *accel_group)
{
  CtkImageMenuItemPrivate    *priv;
  CtkStockItem             stock_item;

  /* Silent return for the constructor */
  if (!accel_group)
    return;

  g_return_if_fail (CTK_IS_IMAGE_MENU_ITEM (image_menu_item));
  g_return_if_fail (CTK_IS_ACCEL_GROUP (accel_group));

  priv = image_menu_item->priv;

  if (priv->use_stock && priv->label && ctk_stock_lookup (priv->label, &stock_item))
    if (stock_item.keyval)
      {
        ctk_widget_add_accelerator (CTK_WIDGET (image_menu_item),
                                    "activate",
                                    accel_group,
                                    stock_item.keyval,
                                    stock_item.modifier,
                                    CTK_ACCEL_VISIBLE);

        g_object_notify (G_OBJECT (image_menu_item), "accel-group");
      }

}

/**
 * ctk_image_menu_item_set_image:
 * @image_menu_item: a #CtkImageMenuItem.
 * @image: (allow-none): a widget to set as the image for the menu item.
 *
 * Sets the image of @image_menu_item to the given widget.
 * Note that it depends on the show-menu-images setting whether
 * the image will be displayed or not.
 */
void
ctk_image_menu_item_set_image (CtkImageMenuItem *image_menu_item,
                               CtkWidget        *image)
{
  CtkImageMenuItemPrivate *priv;

  g_return_if_fail (CTK_IS_IMAGE_MENU_ITEM (image_menu_item));

  priv = image_menu_item->priv;

  if (image == priv->image)
    return;

  if (priv->image)
    ctk_container_remove (CTK_CONTAINER (image_menu_item),
                          priv->image);

  priv->image = image;

  if (image == NULL)
    return;

  ctk_widget_set_parent (image, CTK_WIDGET (image_menu_item));
  g_object_set (image,
                "visible", show_image (image_menu_item),
                "no-show-all", TRUE,
                NULL);
  ctk_image_set_pixel_size (CTK_IMAGE (image), 16);

  g_object_notify (G_OBJECT (image_menu_item), "image");
}

/**
 * ctk_image_menu_item_get_image:
 * @image_menu_item: a #CtkImageMenuItem
 *
 * Gets the widget that is currently set as the image of @image_menu_item.
 * See ctk_image_menu_item_set_image().
 *
 * Returns: (transfer none): the widget set as image of @image_menu_item
 **/
CtkWidget*
ctk_image_menu_item_get_image (CtkImageMenuItem *image_menu_item)
{
  g_return_val_if_fail (CTK_IS_IMAGE_MENU_ITEM (image_menu_item), NULL);

  return image_menu_item->priv->image;
}

static void
ctk_image_menu_item_remove (CtkContainer *container,
                            CtkWidget    *child)
{
  CtkImageMenuItem *image_menu_item = CTK_IMAGE_MENU_ITEM (container);
  CtkImageMenuItemPrivate *priv = image_menu_item->priv;

  if (child == priv->image)
    {
      gboolean widget_was_visible;

      widget_was_visible = ctk_widget_get_visible (child);

      ctk_widget_unparent (child);
      priv->image = NULL;

      if (widget_was_visible &&
          ctk_widget_get_visible (CTK_WIDGET (container)))
        ctk_widget_queue_resize (CTK_WIDGET (container));

      g_object_notify (G_OBJECT (image_menu_item), "image");
    }
  else
    {
      CTK_CONTAINER_CLASS (ctk_image_menu_item_parent_class)->remove (container, child);
    }
}

static void
show_image_change_notify (CtkImageMenuItem *image_menu_item)
{
  CtkImageMenuItemPrivate *priv = image_menu_item->priv;

  if (priv->image)
    {
      if (show_image (image_menu_item))
        ctk_widget_show (priv->image);
      else
        ctk_widget_hide (priv->image);
    }
}

static void
traverse_container (CtkWidget *widget,
                    gpointer   data G_GNUC_UNUSED)
{
  if (CTK_IS_IMAGE_MENU_ITEM (widget))
    show_image_change_notify (CTK_IMAGE_MENU_ITEM (widget));
  else if (CTK_IS_CONTAINER (widget))
    ctk_container_forall (CTK_CONTAINER (widget), traverse_container, NULL);
}

static void
ctk_image_menu_item_setting_changed (CtkSettings *settings G_GNUC_UNUSED)
{
  GList *list, *l;

  list = ctk_window_list_toplevels ();

  for (l = list; l; l = l->next)
    ctk_container_forall (CTK_CONTAINER (l->data),
                          traverse_container, NULL);

  g_list_free (list);
}

static void
ctk_image_menu_item_screen_changed (CtkWidget *widget,
                                    CdkScreen *previous_screen G_GNUC_UNUSED)
{
  CtkSettings *settings;
  gulong show_image_connection;

  if (!ctk_widget_has_screen (widget))
    return;

  settings = ctk_widget_get_settings (widget);

  show_image_connection =
    g_signal_handler_find (settings, G_SIGNAL_MATCH_FUNC, 0, 0,
                           NULL, ctk_image_menu_item_setting_changed, NULL);

  if (show_image_connection)
    return;

  g_signal_connect (settings, "notify::ctk-menu-images",
                    G_CALLBACK (ctk_image_menu_item_setting_changed), NULL);

  show_image_change_notify (CTK_IMAGE_MENU_ITEM (widget));
}
