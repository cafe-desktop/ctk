/* CTK+ - accessibility implementations
 * Copyright 2001 Sun Microsystems Inc.
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

#include <ctk/ctk.h>
#include <glib/gi18n-lib.h>
#include "ctkmenubuttonaccessible.h"


G_DEFINE_TYPE (CtkMenuButtonAccessible, ctk_menu_button_accessible, CTK_TYPE_TOGGLE_BUTTON_ACCESSIBLE)

static void
ctk_menu_button_accessible_initialize (AtkObject *accessible,
                                        gpointer   data)
{
  ATK_OBJECT_CLASS (ctk_menu_button_accessible_parent_class)->initialize (accessible, data);
}

static gint
ctk_menu_button_accessible_get_n_children (AtkObject* obj)
{
  CtkWidget *widget;
  CtkWidget *submenu;
  gint count = 0;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return count;

  submenu = CTK_WIDGET (ctk_menu_button_get_popup (CTK_MENU_BUTTON (widget)));
  if (submenu)
    {
      GList *children;

      children = ctk_container_get_children (CTK_CONTAINER (submenu));
      count = g_list_length (children);
      g_list_free (children);
    }
  return count;
}

static AtkObject *
ctk_menu_button_accessible_ref_child (AtkObject *obj,
                                      gint       i)
{
  AtkObject *accessible = NULL;
  CtkWidget *widget;
  CtkWidget *submenu;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return NULL;

  submenu = CTK_WIDGET (ctk_menu_button_get_popup (CTK_MENU_BUTTON (widget)));
  if (submenu)
    {
      GList *children;
      GList *tmp_list;

      children = ctk_container_get_children (CTK_CONTAINER (submenu));
      tmp_list = g_list_nth (children, i);
      if (tmp_list)
        {
          accessible = ctk_widget_get_accessible (CTK_WIDGET (tmp_list->data));
          g_object_ref (accessible);
        }
      g_list_free (children);
    }

  return accessible;
}

static const gchar *
ctk_menu_button_accessible_get_name (AtkObject *obj)
{
  const gchar *name = NULL;
  CtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return NULL;

  name = ATK_OBJECT_CLASS (ctk_menu_button_accessible_parent_class)->get_name (obj);
  if (name != NULL)
    return name;

  return _("Menu");
}

static void
ctk_menu_button_accessible_class_init (CtkMenuButtonAccessibleClass *klass)
{
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  class->get_name = ctk_menu_button_accessible_get_name;
  class->initialize = ctk_menu_button_accessible_initialize;
  class->get_n_children = ctk_menu_button_accessible_get_n_children;
  class->ref_child = ctk_menu_button_accessible_ref_child;
}

static void
ctk_menu_button_accessible_init (CtkMenuButtonAccessible *menu_button G_GNUC_UNUSED)
{
}
