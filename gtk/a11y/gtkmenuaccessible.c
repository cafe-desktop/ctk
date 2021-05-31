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

#include "gtkmenuaccessible.h"
#include "gtkwidgetaccessibleprivate.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (GtkMenuAccessible, ctk_menu_accessible, CTK_TYPE_MENU_SHELL_ACCESSIBLE)

static void
ctk_menu_accessible_initialize (AtkObject *obj,
                                gpointer   data)
{
  ATK_OBJECT_CLASS (ctk_menu_accessible_parent_class)->initialize (obj, data);

  obj->role = ATK_ROLE_MENU;

  _ctk_widget_accessible_set_layer (CTK_WIDGET_ACCESSIBLE (obj), ATK_LAYER_POPUP);
}

static AtkObject *
ctk_menu_accessible_get_parent (AtkObject *accessible)
{
  AtkObject *parent;
  GtkWidget *widget, *parent_widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible));
  if (widget == NULL)
    return NULL;

  parent = accessible->accessible_parent;
  if (parent != NULL)
    return parent;

  /* If the menu is attached to a menu item or a button (Gnome Menu)
   * report the menu item as parent.
   */
  parent_widget = ctk_menu_get_attach_widget (CTK_MENU (widget));

  if (!CTK_IS_MENU_ITEM (parent_widget) &&
      !CTK_IS_BUTTON (parent_widget) &&
      !CTK_IS_COMBO_BOX (parent_widget))
    parent_widget = ctk_widget_get_parent (widget);

  if (parent_widget == NULL)
    return NULL;

  parent = ctk_widget_get_accessible (parent_widget);
  atk_object_set_parent (accessible, parent);

  return parent;
}

static gint
ctk_menu_accessible_get_index_in_parent (AtkObject *accessible)
{
  GtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (accessible));
  if (widget == NULL)
    return -1;

  if (ctk_menu_get_attach_widget (CTK_MENU (widget)))
    return 0;

  return ATK_OBJECT_CLASS (ctk_menu_accessible_parent_class)->get_index_in_parent (accessible);
}

static void
ctk_menu_accessible_class_init (GtkMenuAccessibleClass *klass)
{
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  class->get_parent = ctk_menu_accessible_get_parent;
  class->get_index_in_parent = ctk_menu_accessible_get_index_in_parent;
  class->initialize = ctk_menu_accessible_initialize;
}

static void
ctk_menu_accessible_init (GtkMenuAccessible *accessible)
{
}
