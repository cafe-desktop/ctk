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
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#include "config.h"

#include "ctkseparatormenuitem.h"

#include "ctkstylecontext.h"

/**
 * SECTION:ctkseparatormenuitem
 * @Short_description: A separator used in menus
 * @Title: CtkSeparatorMenuItem
 *
 * The #CtkSeparatorMenuItem is a separator used to group
 * items within a menu. It displays a horizontal line with a shadow to
 * make it appear sunken into the interface.
 *
 * # CSS nodes
 *
 * CtkSeparatorMenuItem has a single CSS node with name separator.
 */

G_DEFINE_TYPE (CtkSeparatorMenuItem, ctk_separator_menu_item, CTK_TYPE_MENU_ITEM)


static const char *
ctk_separator_menu_item_get_label (CtkMenuItem *item G_GNUC_UNUSED)
{
  return "";
}

static void
ctk_separator_menu_item_class_init (CtkSeparatorMenuItemClass *class)
{
  CTK_CONTAINER_CLASS (class)->child_type = NULL;

  CTK_MENU_ITEM_CLASS (class)->get_label = ctk_separator_menu_item_get_label;

  ctk_widget_class_set_accessible_role (CTK_WIDGET_CLASS (class), ATK_ROLE_SEPARATOR);
  ctk_widget_class_set_css_name (CTK_WIDGET_CLASS (class), "separator");
}

static void
ctk_separator_menu_item_init (CtkSeparatorMenuItem *item G_GNUC_UNUSED)
{
}

/**
 * ctk_separator_menu_item_new:
 *
 * Creates a new #CtkSeparatorMenuItem.
 *
 * Returns: a new #CtkSeparatorMenuItem.
 */
CtkWidget *
ctk_separator_menu_item_new (void)
{
  return g_object_new (CTK_TYPE_SEPARATOR_MENU_ITEM, NULL);
}
