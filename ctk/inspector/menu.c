/*
 * Copyright (c) 2014 Red Hat, Inc.
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
#include <glib/gi18n-lib.h>

#include "menu.h"

#include "ctktreestore.h"
#include "ctkwidgetprivate.h"
#include "ctklabel.h"


enum
{
  COLUMN_TYPE,
  COLUMN_LABEL,
  COLUMN_ACTION,
  COLUMN_TARGET,
  COLUMN_ICON
};

struct _CtkInspectorMenuPrivate
{
  CtkTreeStore *model;
};

G_DEFINE_TYPE_WITH_PRIVATE (CtkInspectorMenu, ctk_inspector_menu, CTK_TYPE_BOX)

static void
ctk_inspector_menu_init (CtkInspectorMenu *sl)
{
  sl->priv = ctk_inspector_menu_get_instance_private (sl);
  ctk_widget_init_template (CTK_WIDGET (sl));
}

static void add_menu (CtkInspectorMenu *sl,
                      GMenuModel       *menu,
                      CtkTreeIter      *parent);

static void
add_item (CtkInspectorMenu *sl,
          GMenuModel       *menu,
          gint              idx,
          CtkTreeIter      *parent)
{
  CtkTreeIter iter;
  GVariant *value;
  gchar *label = NULL;
  gchar *action = NULL;
  gchar *target = NULL;
  gchar *icon = NULL;
  GMenuModel *model;

  g_menu_model_get_item_attribute (menu, idx, G_MENU_ATTRIBUTE_LABEL, "s", &label);
  g_menu_model_get_item_attribute (menu, idx, G_MENU_ATTRIBUTE_ACTION, "s", &action);
  value = g_menu_model_get_item_attribute_value (menu, idx, G_MENU_ATTRIBUTE_TARGET, NULL);
  if (value)
    {
      target = g_variant_print (value, FALSE);
      g_variant_unref (value);
    }

  ctk_tree_store_append (sl->priv->model, &iter, parent);
  ctk_tree_store_set (sl->priv->model, &iter,
                      COLUMN_TYPE, "item",
                      COLUMN_LABEL, label,
                      COLUMN_ACTION, action,
                      COLUMN_TARGET, target,
                      COLUMN_ICON, icon,
                      -1);

  model = g_menu_model_get_item_link (menu, idx, G_MENU_LINK_SECTION);
  if (model)
    {
      if (label == NULL)
        ctk_tree_store_set (sl->priv->model, &iter,
                            COLUMN_LABEL, _("Unnamed section"),
                            -1);
      add_menu (sl, model, &iter);
      g_object_unref (model);
    }

  model = g_menu_model_get_item_link (menu, idx, G_MENU_LINK_SUBMENU);
  if (model)
    {
      add_menu (sl, model, &iter);
      g_object_unref (model);
    }

  g_free (label);
  g_free (action);
  g_free (target);
  g_free (icon);
}

static void
add_menu (CtkInspectorMenu *sl,
          GMenuModel       *menu,
          CtkTreeIter      *parent)
{
  gint n_items;
  gint i;

  ctk_widget_show (CTK_WIDGET (sl));

  n_items = g_menu_model_get_n_items (menu);
  for (i = 0; i < n_items; i++)
    add_item (sl, menu, i, parent);
}

void
ctk_inspector_menu_set_object (CtkInspectorMenu *sl,
                               GObject          *object)
{
  ctk_widget_hide (CTK_WIDGET (sl));
  ctk_tree_store_clear (sl->priv->model);
  
  if (G_IS_MENU_MODEL (object))
    add_menu (sl, G_MENU_MODEL (object), NULL);
}

static void
ctk_inspector_menu_class_init (CtkInspectorMenuClass *klass)
{
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);

  ctk_widget_class_set_template_from_resource (widget_class, "/org/ctk/libctk/inspector/menu.ui");
  ctk_widget_class_bind_template_child_private (widget_class, CtkInspectorMenu, model);
}

// vim: set et sw=2 ts=2:
