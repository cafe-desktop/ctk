/* testrecentchoosermenu.c - Test GtkRecentChooserMenu
 * Copyright (C) 2007  Emmanuele Bassi  <ebassi@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include <gtk/gtk.h>

static GtkRecentManager *manager = NULL;
static GtkWidget *window = NULL;
static GtkWidget *label = NULL;

static void
item_activated_cb (GtkRecentChooser *chooser,
                   gpointer          data)
{
  GtkRecentInfo *info;
  GString *text;
  gchar *label_text;

  info = ctk_recent_chooser_get_current_item (chooser);
  if (!info)
    {
      g_warning ("Unable to retrieve the current item, aborting...");
      return;
    }

  text = g_string_new ("Selected recent item:\n");
  g_string_append_printf (text, "  URI: %s\n",
                          ctk_recent_info_get_uri (info));
  g_string_append_printf (text, "  MIME Type: %s\n",
                          ctk_recent_info_get_mime_type (info));

  label_text = g_string_free (text, FALSE);
  ctk_label_set_text (CTK_LABEL (label), label_text);
  
  ctk_recent_info_unref (info);
  g_free (label_text);
}

static GtkWidget *
create_recent_chooser_menu (gint limit)
{
  GtkWidget *menu;
  GtkRecentFilter *filter;
  GtkWidget *menuitem;
  
  menu = ctk_recent_chooser_menu_new_for_manager (manager);

  if (limit > 0)
    ctk_recent_chooser_set_limit (CTK_RECENT_CHOOSER (menu), limit);
  ctk_recent_chooser_set_local_only (CTK_RECENT_CHOOSER (menu), TRUE);
  ctk_recent_chooser_set_show_icons (CTK_RECENT_CHOOSER (menu), TRUE);
  ctk_recent_chooser_set_show_tips (CTK_RECENT_CHOOSER (menu), TRUE);
  ctk_recent_chooser_set_sort_type (CTK_RECENT_CHOOSER (menu),
                                    CTK_RECENT_SORT_MRU);
  ctk_recent_chooser_menu_set_show_numbers (CTK_RECENT_CHOOSER_MENU (menu),
                                            TRUE);

  filter = ctk_recent_filter_new ();
  ctk_recent_filter_set_name (filter, "Gedit files");
  ctk_recent_filter_add_application (filter, "gedit");
  ctk_recent_chooser_add_filter (CTK_RECENT_CHOOSER (menu), filter);
  ctk_recent_chooser_set_filter (CTK_RECENT_CHOOSER (menu), filter);

  g_signal_connect (menu, "item-activated",
                    G_CALLBACK (item_activated_cb),
                    NULL);

  menuitem = ctk_separator_menu_item_new ();
  ctk_menu_shell_prepend (CTK_MENU_SHELL (menu), menuitem);
  ctk_widget_show (menuitem);

  menuitem = ctk_menu_item_new_with_label ("Test prepend");
  ctk_menu_shell_prepend (CTK_MENU_SHELL (menu), menuitem);
  ctk_widget_show (menuitem);

  menuitem = ctk_separator_menu_item_new ();
  ctk_menu_shell_append (CTK_MENU_SHELL (menu), menuitem);
  ctk_widget_show (menuitem);

  menuitem = ctk_menu_item_new_with_label ("Test append");
  ctk_menu_shell_append (CTK_MENU_SHELL (menu), menuitem);
  ctk_widget_show (menuitem);

  menuitem = ctk_menu_item_new_with_mnemonic ("Clear");
  ctk_menu_shell_append (CTK_MENU_SHELL (menu), menuitem);
  ctk_widget_show (menuitem);

  ctk_widget_show_all (menu);

  return menu;
}

static GtkWidget *
create_file_menu (GtkAccelGroup *accelgroup)
{
  GtkWidget *menu;
  GtkWidget *menuitem;
  GtkWidget *recentmenu;

  menu = ctk_menu_new ();

  menuitem = ctk_menu_item_new_with_mnemonic ("_New");
  ctk_menu_shell_append (CTK_MENU_SHELL (menu), menuitem);
  ctk_widget_show (menuitem);

  menuitem = ctk_menu_item_new_with_mnemonic ("_Open");
  ctk_menu_shell_append (CTK_MENU_SHELL (menu), menuitem);
  ctk_widget_show (menuitem);

  menuitem = ctk_menu_item_new_with_mnemonic ("_Open Recent");
  recentmenu = create_recent_chooser_menu (-1);
  ctk_menu_item_set_submenu (CTK_MENU_ITEM (menuitem), recentmenu);
  ctk_menu_shell_append (CTK_MENU_SHELL (menu), menuitem);
  ctk_widget_show (menuitem);

  menuitem = ctk_separator_menu_item_new ();
  ctk_menu_shell_append (CTK_MENU_SHELL (menu), menuitem);
  ctk_widget_show (menuitem);

  menuitem = ctk_menu_item_new_with_mnemonic ("_Quit");
  ctk_menu_shell_append (CTK_MENU_SHELL (menu), menuitem);
  ctk_widget_show (menuitem);

  ctk_widget_show (menu);

  return menu;
}

int
main (int argc, char *argv[])
{
  GtkWidget *box;
  GtkWidget *menubar;
  GtkWidget *menuitem;
  GtkWidget *menu;
  GtkWidget *button;
  GtkAccelGroup *accel_group;

  ctk_init (&argc, &argv);

  manager = ctk_recent_manager_get_default ();

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_default_size (CTK_WINDOW (window), -1, -1);
  ctk_window_set_title (CTK_WINDOW (window), "Recent Chooser Menu Test");
  g_signal_connect (window, "destroy", G_CALLBACK (ctk_main_quit), NULL);

  accel_group = ctk_accel_group_new ();
  ctk_window_add_accel_group (CTK_WINDOW (window), accel_group);
  
  box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_container_add (CTK_CONTAINER (window), box);
  ctk_widget_show (box);

  menubar = ctk_menu_bar_new ();
  ctk_box_pack_start (CTK_BOX (box), menubar, FALSE, TRUE, 0);
  ctk_widget_show (menubar);

  menu = create_file_menu (accel_group);
  menuitem = ctk_menu_item_new_with_mnemonic ("_File");
  ctk_menu_item_set_submenu (CTK_MENU_ITEM (menuitem), menu);
  ctk_menu_shell_append (CTK_MENU_SHELL (menubar), menuitem);
  ctk_widget_show (menuitem);

  menu = create_recent_chooser_menu (4);
  menuitem = ctk_menu_item_new_with_mnemonic ("_Recently Used");
  ctk_menu_item_set_submenu (CTK_MENU_ITEM (menuitem), menu);
  ctk_menu_shell_append (CTK_MENU_SHELL (menubar), menuitem);
  ctk_widget_show (menuitem);

  label = ctk_label_new ("No recent item selected");
  ctk_box_pack_start (CTK_BOX (box), label, TRUE, TRUE, 0);
  ctk_widget_show (label);

  button = ctk_button_new_with_label ("Close");
  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (ctk_widget_destroy),
                            window);
  ctk_box_pack_end (CTK_BOX (box), button, TRUE, TRUE, 0);
  ctk_widget_set_can_default (button, TRUE);
  ctk_widget_grab_default (button);
  ctk_widget_show (button);

  ctk_widget_show (window);

  ctk_main ();

  return 0;
}
