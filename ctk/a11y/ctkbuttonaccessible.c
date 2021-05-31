/* GTK+ - accessibility implementations
 * Copyright 2001, 2002, 2003 Sun Microsystems Inc.
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

#include <string.h>
#include <ctk/ctk.h>
#include <glib/gi18n-lib.h>
#include "ctkbuttonaccessible.h"


static void atk_action_interface_init (AtkActionIface *iface);
static void atk_image_interface_init  (AtkImageIface  *iface);

G_DEFINE_TYPE_WITH_CODE (CtkButtonAccessible, ctk_button_accessible, CTK_TYPE_CONTAINER_ACCESSIBLE,
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_ACTION, atk_action_interface_init)
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_IMAGE, atk_image_interface_init))

static void
ctk_button_accessible_initialize (AtkObject *obj,
                                  gpointer   data)
{
  CtkWidget *parent;

  ATK_OBJECT_CLASS (ctk_button_accessible_parent_class)->initialize (obj, data);

  parent = ctk_widget_get_parent (ctk_accessible_get_widget (CTK_ACCESSIBLE (obj)));
  if (CTK_IS_TREE_VIEW (parent))
    {
      /* Even though the accessible parent of the column header will
       * be reported as the table because the parent widget of the
       * CtkTreeViewColumn's button is the CtkTreeView we set
       * the accessible parent for column header to be the table
       * to ensure that atk_object_get_index_in_parent() returns
       * the correct value; see gail_widget_get_index_in_parent().
       */
      atk_object_set_parent (obj, ctk_widget_get_accessible (parent));
      obj->role = ATK_ROLE_TABLE_COLUMN_HEADER;
    }
  else
    obj->role = ATK_ROLE_PUSH_BUTTON;
}

static CtkWidget *
get_image_from_button (CtkWidget *button)
{
  CtkWidget *image;

  image = ctk_button_get_image (CTK_BUTTON (button));
  if (CTK_IS_IMAGE (image))
    return image;

  return NULL;
}

static CtkWidget *
find_label_child (CtkContainer *container)
{
  GList *children, *tmp_list;
  CtkWidget *child;

  children = ctk_container_get_children (container);

  child = NULL;
  for (tmp_list = children; tmp_list != NULL; tmp_list = tmp_list->next)
    {
      if (CTK_IS_LABEL (tmp_list->data))
        {
          child = CTK_WIDGET (tmp_list->data);
          break;
        }
      else if (CTK_IS_CONTAINER (tmp_list->data))
        {
          child = find_label_child (CTK_CONTAINER (tmp_list->data));
          if (child)
            break;
        }
    }
  g_list_free (children);
  return child;
}

static CtkWidget *
get_label_from_button (CtkWidget *button)
{
  CtkWidget *child;

  child = ctk_bin_get_child (CTK_BIN (button));
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  if (CTK_IS_ALIGNMENT (child))
    child = ctk_bin_get_child (CTK_BIN (child));
G_GNUC_END_IGNORE_DEPRECATIONS

  if (CTK_IS_CONTAINER (child))
    child = find_label_child (CTK_CONTAINER (child));
  else if (!CTK_IS_LABEL (child))
    child = NULL;

  return child;
}

static const gchar *
ctk_button_accessible_get_name (AtkObject *obj)
{
  const gchar *name = NULL;
  CtkWidget *widget;
  CtkWidget *child;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return NULL;

  name = ATK_OBJECT_CLASS (ctk_button_accessible_parent_class)->get_name (obj);
  if (name != NULL)
    return name;

  child = get_label_from_button (widget);
  if (CTK_IS_LABEL (child))
    name = ctk_label_get_text (CTK_LABEL (child));
  else
    {
      CtkWidget *image;

      image = get_image_from_button (widget);
      if (CTK_IS_IMAGE (image))
        {
          AtkObject *atk_obj;

          atk_obj = ctk_widget_get_accessible (image);
          name = atk_object_get_name (atk_obj);
        }
    }

  return name;
}

static gint
ctk_button_accessible_get_n_children (AtkObject* obj)
{
  return 0;
}

static AtkObject *
ctk_button_accessible_ref_child (AtkObject *obj,
                                 gint       i)
{
  return NULL;
}

static AtkStateSet *
ctk_button_accessible_ref_state_set (AtkObject *obj)
{
  AtkStateSet *state_set;
  CtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return NULL;

  state_set = ATK_OBJECT_CLASS (ctk_button_accessible_parent_class)->ref_state_set (obj);

  if ((ctk_widget_get_state_flags (widget) & CTK_STATE_FLAG_ACTIVE) != 0)
    atk_state_set_add_state (state_set, ATK_STATE_ARMED);

  if (!ctk_widget_get_can_focus (widget))
    atk_state_set_remove_state (state_set, ATK_STATE_SELECTABLE);

  return state_set;
}

static void
ctk_button_accessible_notify_ctk (GObject    *obj,
                                  GParamSpec *pspec)
{
  CtkWidget *widget = CTK_WIDGET (obj);
  AtkObject *atk_obj = ctk_widget_get_accessible (widget);

  if (strcmp (pspec->name, "label") == 0)
    {
      if (atk_obj->name == NULL)
        g_object_notify (G_OBJECT (atk_obj), "accessible-name");

      g_signal_emit_by_name (atk_obj, "visible-data-changed");
    }
  else
    CTK_WIDGET_ACCESSIBLE_CLASS (ctk_button_accessible_parent_class)->notify_ctk (obj, pspec);
}

static void
ctk_button_accessible_class_init (CtkButtonAccessibleClass *klass)
{
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);
  CtkContainerAccessibleClass *container_class = (CtkContainerAccessibleClass*)klass;
  CtkWidgetAccessibleClass *widget_class = (CtkWidgetAccessibleClass*)klass;

  class->get_name = ctk_button_accessible_get_name;
  class->get_n_children = ctk_button_accessible_get_n_children;
  class->ref_child = ctk_button_accessible_ref_child;
  class->ref_state_set = ctk_button_accessible_ref_state_set;
  class->initialize = ctk_button_accessible_initialize;

  widget_class->notify_ctk = ctk_button_accessible_notify_ctk;

  container_class->add_ctk = NULL;
  container_class->remove_ctk = NULL;
}

static void
ctk_button_accessible_init (CtkButtonAccessible *button)
{
}

static gboolean
ctk_button_accessible_do_action (AtkAction *action,
                                 gint       i)
{
  CtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (action));
  if (widget == NULL)
    return FALSE;

  if (i != 0)
    return FALSE;

  if (!ctk_widget_is_sensitive (widget) || !ctk_widget_get_visible (widget))
    return FALSE;

  ctk_button_clicked (CTK_BUTTON (widget));
  return TRUE;
}

static gint
ctk_button_accessible_get_n_actions (AtkAction *action)
{
  return 1;
}

static const gchar *
ctk_button_accessible_get_keybinding (AtkAction *action,
                                      gint       i)
{
  gchar *return_value = NULL;
  CtkWidget *widget;
  CtkWidget *label;
  guint key_val;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (action));
  if (widget == NULL)
    return NULL;

  if (i != 0)
    return NULL;

  label = get_label_from_button (widget);
  if (CTK_IS_LABEL (label))
    {
      key_val = ctk_label_get_mnemonic_keyval (CTK_LABEL (label));
      if (key_val != GDK_KEY_VoidSymbol)
        return_value = ctk_accelerator_name (key_val, GDK_MOD1_MASK);
    }
  if (return_value == NULL)
    {
      /* Find labelled-by relation */
      AtkRelationSet *set;
      AtkRelation *relation;
      GPtrArray *target;
      gpointer target_object;

      set = atk_object_ref_relation_set (ATK_OBJECT (action));
      if (set)
        {
          relation = atk_relation_set_get_relation_by_type (set, ATK_RELATION_LABELLED_BY);
          if (relation)
            {
              target = atk_relation_get_target (relation);
              target_object = g_ptr_array_index (target, 0);
              label = ctk_accessible_get_widget (CTK_ACCESSIBLE (target_object));
            }
          g_object_unref (set);
        }

      if (CTK_IS_LABEL (label))
        {
          key_val = ctk_label_get_mnemonic_keyval (CTK_LABEL (label));
          if (key_val != GDK_KEY_VoidSymbol)
            return_value = ctk_accelerator_name (key_val, GDK_MOD1_MASK);
        }
    }
  return return_value;
}

static const gchar *
ctk_button_accessible_action_get_name (AtkAction *action,
                                       gint       i)
{
  if (i == 0)
    return "click";
  return NULL;
}

static const gchar *
ctk_button_accessible_action_get_localized_name (AtkAction *action,
                                                 gint       i)
{
  if (i == 0)
    return C_("Action name", "Click");
  return NULL;
}

static const gchar *
ctk_button_accessible_action_get_description (AtkAction *action,
                                              gint       i)
{
  if (i == 0)
    return C_("Action description", "Clicks the button");
  return NULL;
}

static void
atk_action_interface_init (AtkActionIface *iface)
{
  iface->do_action = ctk_button_accessible_do_action;
  iface->get_n_actions = ctk_button_accessible_get_n_actions;
  iface->get_keybinding = ctk_button_accessible_get_keybinding;
  iface->get_name = ctk_button_accessible_action_get_name;
  iface->get_localized_name = ctk_button_accessible_action_get_localized_name;
  iface->get_description = ctk_button_accessible_action_get_description;
}

static const gchar *
ctk_button_accessible_get_image_description (AtkImage *image)
{
  CtkWidget *widget;
  CtkWidget  *button_image;
  AtkObject *obj;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (image));
  if (widget == NULL)
    return NULL;

  button_image = get_image_from_button (widget);
  if (CTK_IS_IMAGE (button_image))
    {
      obj = ctk_widget_get_accessible (button_image);
      return atk_image_get_image_description (ATK_IMAGE (obj));
    }

  return NULL;
}

static void
ctk_button_accessible_get_image_position (AtkImage     *image,
                                          gint         *x,
                                          gint         *y,
                                          AtkCoordType  coord_type)
{
  CtkWidget *widget;
  CtkWidget *button_image;
  AtkObject *obj;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (image));
  if (widget == NULL)
    {
      *x = G_MININT;
      *y = G_MININT;
      return;
    }

  button_image = get_image_from_button (widget);
  if (button_image != NULL)
    {
      obj = ctk_widget_get_accessible (button_image);
      atk_component_get_extents (ATK_COMPONENT (obj), x, y, NULL, NULL,
                                 coord_type);
    }
  else
    {
      *x = G_MININT;
      *y = G_MININT;
    }
}

static void
ctk_button_accessible_get_image_size (AtkImage *image,
                                      gint     *width,
                                      gint     *height)
{
  CtkWidget *widget;
  CtkWidget *button_image;
  AtkObject *obj;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (image));
  if (widget == NULL)
    {
      *width = -1;
      *height = -1;
      return;
    }

  button_image = get_image_from_button (widget);
  if (CTK_IS_IMAGE (button_image))
    {
      obj = ctk_widget_get_accessible (CTK_WIDGET (button_image));
      atk_image_get_image_size (ATK_IMAGE (obj), width, height);
    }
  else
    {
      *width = -1;
      *height = -1;
    }
}

static gboolean
ctk_button_accessible_set_image_description (AtkImage    *image,
                                             const gchar *description)
{
  CtkWidget *widget;
  CtkWidget *button_image;
  AtkObject *obj;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (image));

  if (widget == NULL)
    return FALSE;

  button_image = get_image_from_button (widget);
  if (CTK_IMAGE (button_image))
    {
      obj = ctk_widget_get_accessible (CTK_WIDGET (button_image));
      return atk_image_set_image_description (ATK_IMAGE (obj), description);
    }

  return FALSE;
}

static void
atk_image_interface_init (AtkImageIface *iface)
{
  iface->get_image_description = ctk_button_accessible_get_image_description;
  iface->get_image_position = ctk_button_accessible_get_image_position;
  iface->get_image_size = ctk_button_accessible_get_image_size;
  iface->set_image_description = ctk_button_accessible_set_image_description;
}
