/*
 * Copyright © 2011, 2013 Canonical Limited
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

#include "ctkmodelmenuitem.h"

#include "ctkaccellabel.h"
#include "ctkcheckmenuitemprivate.h"
#include "ctkimage.h"
#include "ctkbox.h"

struct _CtkModelMenuItem
{
  CtkCheckMenuItem parent_instance;
  CtkMenuTrackerItemRole role;
  gboolean has_indicator;
};

typedef CtkCheckMenuItemClass CtkModelMenuItemClass;

G_DEFINE_TYPE (CtkModelMenuItem, ctk_model_menu_item, CTK_TYPE_CHECK_MENU_ITEM)

enum
{
  PROP_0,
  PROP_ACTION_ROLE,
  PROP_ICON,
  PROP_TEXT,
  PROP_TOGGLED,
  PROP_ACCEL
};

static void
ctk_model_menu_item_toggle_size_request (CtkMenuItem *menu_item,
                                         gint        *requisition)
{
  CtkModelMenuItem *item = CTK_MODEL_MENU_ITEM (menu_item);

  if (item->has_indicator)
    CTK_MENU_ITEM_CLASS (ctk_model_menu_item_parent_class)
      ->toggle_size_request (menu_item, requisition);

  else
    *requisition = 0;
}

static void
ctk_model_menu_item_activate (CtkMenuItem *item)
{
  /* block the automatic toggle behaviour -- just do nothing */
}

static void
ctk_model_menu_item_draw_indicator (CtkCheckMenuItem *check_item,
                                    cairo_t          *cr)
{
  CtkModelMenuItem *item = CTK_MODEL_MENU_ITEM (check_item);

  if (item->has_indicator)
    CTK_CHECK_MENU_ITEM_CLASS (ctk_model_menu_item_parent_class)
      ->draw_indicator (check_item, cr);
}

static void
ctk_model_menu_item_set_has_indicator (CtkModelMenuItem *item,
                                       gboolean          has_indicator)
{
  if (has_indicator == item->has_indicator)
    return;

  item->has_indicator = has_indicator;

  ctk_widget_queue_resize (CTK_WIDGET (item));
}

static void
ctk_model_menu_item_set_action_role (CtkModelMenuItem       *item,
                                     CtkMenuTrackerItemRole  role)
{
  AtkObject *accessible;
  AtkRole a11y_role;

  if (role == item->role)
    return;

  ctk_check_menu_item_set_draw_as_radio (CTK_CHECK_MENU_ITEM (item), role == CTK_MENU_TRACKER_ITEM_ROLE_RADIO);
  ctk_model_menu_item_set_has_indicator (item, role != CTK_MENU_TRACKER_ITEM_ROLE_NORMAL);

  accessible = ctk_widget_get_accessible (CTK_WIDGET (item));
  switch (role)
    {
    case CTK_MENU_TRACKER_ITEM_ROLE_NORMAL:
      a11y_role = ATK_ROLE_MENU_ITEM;
      break;

    case CTK_MENU_TRACKER_ITEM_ROLE_CHECK:
      a11y_role = ATK_ROLE_CHECK_MENU_ITEM;
      break;

    case CTK_MENU_TRACKER_ITEM_ROLE_RADIO:
      a11y_role = ATK_ROLE_RADIO_MENU_ITEM;
      break;

    default:
      g_assert_not_reached ();
    }

  atk_object_set_role (accessible, a11y_role);
  g_object_notify (G_OBJECT (item), "action-role");
}

static void
ctk_model_menu_item_set_icon (CtkModelMenuItem *item,
                              GIcon            *icon)
{
  CtkWidget *child;

  g_return_if_fail (CTK_IS_MODEL_MENU_ITEM (item));
  g_return_if_fail (icon == NULL || G_IS_ICON (icon));

  child = ctk_bin_get_child (CTK_BIN (item));

  /* There are only three possibilities here:
   *
   *   - no child
   *   - accel label child
   *   - already a box
   *
   * Handle the no-child case by having CtkMenuItem create the accel
   * label, then we will only have two possible cases.
   */
  if (child == NULL)
    {
      ctk_menu_item_get_label (CTK_MENU_ITEM (item));
      child = ctk_bin_get_child (CTK_BIN (item));
      g_assert (CTK_IS_LABEL (child));
    }

  /* If it is a box, make sure there are no images inside of it already.
   */
  if (CTK_IS_BOX (child))
    {
      GList *children;

      children = ctk_container_get_children (CTK_CONTAINER (child));
      while (children)
        {
          if (CTK_IS_IMAGE (children->data))
            ctk_widget_destroy (children->data);

          children = g_list_delete_link (children, children);
        }
    }
  else
    {
      CtkWidget *box;

      if (icon == NULL)
        return;

      box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 6);

      /* Reparent the child without destroying it */
      g_object_ref (child);
      ctk_container_remove (CTK_CONTAINER (item), child);
      ctk_box_pack_end (CTK_BOX (box), child, TRUE, TRUE, 0);
      g_object_unref (child);

      ctk_container_add (CTK_CONTAINER (item), box);
      ctk_widget_show (box);

      /* Now we have a box */
      child = box;
    }

  g_assert (CTK_IS_BOX (child));

  /* child is now a box containing a label and no image.  Add the icon,
   * if appropriate.
   */
  if (icon != NULL)
    {
      CtkWidget *image;

      image = ctk_image_new_from_gicon (icon, CTK_ICON_SIZE_MENU);
      ctk_image_set_pixel_size (CTK_IMAGE (image), 16);
      ctk_box_pack_start (CTK_BOX (child), image, FALSE, FALSE, 0);
      ctk_widget_show (image);
    }

  g_object_notify (G_OBJECT (item), "icon");
}

static GIcon *
ctk_model_menu_item_get_icon (CtkModelMenuItem *item)
{
  CtkWidget *child;
  GIcon *icon = NULL;

  child = ctk_bin_get_child (CTK_BIN (item));
  if (CTK_IS_BOX (child))
    {
      GList *children, *l;

      children = ctk_container_get_children (CTK_CONTAINER (child));
      for (l = children; l; l = l->next)
        {
          if (CTK_IS_IMAGE (l->data))
            {
              ctk_image_get_gicon (CTK_IMAGE (l->data), &icon, NULL);
              break;
            }
        }
      g_list_free (children);
    }

  return icon;
}

static void
ctk_model_menu_item_set_text (CtkModelMenuItem *item,
                              const gchar      *text)
{
  CtkWidget *child;
  GList *children;

  child = ctk_bin_get_child (CTK_BIN (item));
  if (child == NULL)
    {
      ctk_menu_item_get_label (CTK_MENU_ITEM (item));
      child = ctk_bin_get_child (CTK_BIN (item));
      g_assert (CTK_IS_LABEL (child));
    }

  if (CTK_IS_LABEL (child))
    {
      ctk_label_set_text_with_mnemonic (CTK_LABEL (child), text);
      return;
    }

  if (!CTK_IS_CONTAINER (child))
    return;

  children = ctk_container_get_children (CTK_CONTAINER (child));

  while (children)
    {
      if (CTK_IS_LABEL (children->data))
        ctk_label_set_label (CTK_LABEL (children->data), text);

      children = g_list_delete_link (children, children);
    }

  g_object_notify (G_OBJECT (item), "text");
}

static const gchar *
ctk_model_menu_item_get_text (CtkModelMenuItem *item)
{
  CtkWidget *child;

  child = ctk_bin_get_child (CTK_BIN (item));

  if (CTK_IS_LABEL (child))
    return ctk_label_get_text (CTK_LABEL (child));

  if (CTK_IS_CONTAINER (child))
    {
      GList *children, *l;
      const gchar *text = NULL;

      children = ctk_container_get_children (CTK_CONTAINER (child));
      for (l = children; l; l = l->next)
        {
          if (CTK_IS_LABEL (l->data))
            {
              text = ctk_label_get_text (CTK_LABEL (l->data));
              break;
            }
        }
      g_list_free (children);

      return text;
    }

  return NULL;
}

static void
ctk_model_menu_item_set_accel (CtkModelMenuItem *item,
                               const gchar      *accel)
{
  CtkWidget *child;
  GList *children;
  CdkModifierType modifiers;
  guint key;

  if (accel)
    {
      ctk_accelerator_parse (accel, &key, &modifiers);
      if (!key)
        modifiers = 0;
    }
  else
    {
      key = 0;
      modifiers = 0;
    }

  child = ctk_bin_get_child (CTK_BIN (item));
  if (child == NULL)
    {
      ctk_menu_item_get_label (CTK_MENU_ITEM (item));
      child = ctk_bin_get_child (CTK_BIN (item));
      g_assert (CTK_IS_LABEL (child));
    }

  if (CTK_IS_ACCEL_LABEL (child))
    {
      ctk_accel_label_set_accel (CTK_ACCEL_LABEL (child), key, modifiers);
      return;
    }

  if (!CTK_IS_CONTAINER (child))
    return;

  children = ctk_container_get_children (CTK_CONTAINER (child));

  while (children)
    {
      if (CTK_IS_ACCEL_LABEL (children->data))
        ctk_accel_label_set_accel (children->data, key, modifiers);

      children = g_list_delete_link (children, children);
    }
}

static gchar *
ctk_model_menu_item_get_accel (CtkModelMenuItem *item)
{
  CtkWidget *child;
  CtkWidget *accel_label = NULL;

  child = ctk_bin_get_child (CTK_BIN (item));

  if (CTK_IS_ACCEL_LABEL (child))
    accel_label = child;
  else if (CTK_IS_CONTAINER (child))
    {
      GList *children, *l;

      children = ctk_container_get_children (CTK_CONTAINER (child));
      for (l = children; l; l = l->next)
        {
          if (CTK_IS_ACCEL_LABEL (l->data))
            {
              accel_label = CTK_WIDGET (l->data);
              break;
            }
        }
      g_list_free (children);
    }

  if (accel_label)
    {
      guint key;
      CdkModifierType mods;

      ctk_accel_label_get_accel (CTK_ACCEL_LABEL (accel_label), &key, &mods);

      return ctk_accelerator_name (key, mods);
    }

  return NULL;
}

static void
ctk_model_menu_item_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  CtkModelMenuItem *item = CTK_MODEL_MENU_ITEM (object);

  switch (prop_id)
    {
    case PROP_ACTION_ROLE:
      g_value_set_enum (value, item->role);
      break;

    case PROP_ICON:
      g_value_set_object (value, ctk_model_menu_item_get_icon (item));
      break;

    case PROP_TEXT:
      g_value_set_string (value, ctk_model_menu_item_get_text (item));
      break;

    case PROP_TOGGLED:
      g_value_set_boolean (value, ctk_check_menu_item_get_active (CTK_CHECK_MENU_ITEM (item)));
      break;

    case PROP_ACCEL:
      g_value_take_string (value, ctk_model_menu_item_get_accel (item));
      break;

    default:
      g_assert_not_reached ();
    }
}

static void
ctk_model_menu_item_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  CtkModelMenuItem *item = CTK_MODEL_MENU_ITEM (object);

  switch (prop_id)
    {
    case PROP_ACTION_ROLE:
      ctk_model_menu_item_set_action_role (item, g_value_get_enum (value));
      break;

    case PROP_ICON:
      ctk_model_menu_item_set_icon (item, g_value_get_object (value));
      break;

    case PROP_TEXT:
      ctk_model_menu_item_set_text (item, g_value_get_string (value));
      break;

    case PROP_TOGGLED:
      _ctk_check_menu_item_set_active (CTK_CHECK_MENU_ITEM (item), g_value_get_boolean (value));
      g_object_notify (object, "active");
      break;

    case PROP_ACCEL:
      ctk_model_menu_item_set_accel (item, g_value_get_string (value));
      break;

    default:
      g_assert_not_reached ();
    }
}

static void
ctk_model_menu_item_init (CtkModelMenuItem *item)
{
}

static void
ctk_model_menu_item_class_init (CtkModelMenuItemClass *class)
{
  CtkCheckMenuItemClass *check_class = CTK_CHECK_MENU_ITEM_CLASS (class);
  CtkMenuItemClass *item_class = CTK_MENU_ITEM_CLASS (class);
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  check_class->draw_indicator = ctk_model_menu_item_draw_indicator;

  item_class->toggle_size_request = ctk_model_menu_item_toggle_size_request;
  item_class->activate = ctk_model_menu_item_activate;

  object_class->get_property = ctk_model_menu_item_get_property;
  object_class->set_property = ctk_model_menu_item_set_property;

  g_object_class_install_property (object_class, PROP_ACTION_ROLE,
                                   g_param_spec_enum ("action-role", "action role", "action role",
                                                      CTK_TYPE_MENU_TRACKER_ITEM_ROLE,
                                                      CTK_MENU_TRACKER_ITEM_ROLE_NORMAL,
                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY));
  g_object_class_install_property (object_class, PROP_ICON,
                                   g_param_spec_object ("icon", "icon", "icon", G_TYPE_ICON,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY));
  g_object_class_install_property (object_class, PROP_TEXT,
                                   g_param_spec_string ("text", "text", "text", NULL,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY));
  g_object_class_install_property (object_class, PROP_TOGGLED,
                                   g_param_spec_boolean ("toggled", "toggled", "toggled", FALSE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY));
  g_object_class_install_property (object_class, PROP_ACCEL,
                                   g_param_spec_string ("accel", "accel", "accel", NULL,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY));

  ctk_widget_class_set_accessible_role (CTK_WIDGET_CLASS (class), ATK_ROLE_MENU_ITEM);
}

CtkWidget *
ctk_model_menu_item_new (void)
{
  return g_object_new (CTK_TYPE_MODEL_MENU_ITEM, NULL);
}
