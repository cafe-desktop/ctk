/* testsocket_common.c
 * Copyright (C) 2001 Red Hat, Inc
 * Author: Owen Taylor
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
#include <ctk/ctk.h>
#if defined (CDK_WINDOWING_X11)
#include <ctk/ctkx.h>
#elif defined (CDK_WINDOWING_WIN32)
#include "win32/cdkwin32.h"
#endif

enum
{
  ACTION_FILE_NEW,
  ACTION_FILE_OPEN,
  ACTION_OK,
  ACTION_HELP_ABOUT
};

static void
print_hello (CtkWidget *w G_GNUC_UNUSED,
	     guint      action)
{
  switch (action)
    {
    case ACTION_FILE_NEW:
      g_message ("File New activated");
      break;
    case ACTION_FILE_OPEN:
      g_message ("File Open activated");
      break;
    case ACTION_OK:
      g_message ("OK activated");
      break;
    case ACTION_HELP_ABOUT:
      g_message ("Help About activated ");
      break;
    default:
      g_assert_not_reached ();
      break;
    }
}

static void
remove_buttons (CtkWidget *widget, CtkWidget *other_button)
{
  ctk_widget_destroy (other_button);
  ctk_widget_destroy (widget);
}

static gboolean
blink_cb (gpointer data)
{
  CtkWidget *widget = data;

  ctk_widget_show (widget);
  g_object_set_data (G_OBJECT (widget), "blink", NULL);

  return FALSE;
}

static void
blink (CtkWidget *widget G_GNUC_UNUSED,
       CtkWidget *window)
{
  guint blink_timeout = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (window), "blink"));
  
  if (!blink_timeout)
    {
      blink_timeout = cdk_threads_add_timeout (1000, blink_cb, window);
      ctk_widget_hide (window);

      g_object_set_data (G_OBJECT (window), "blink", GUINT_TO_POINTER (blink_timeout));
    }
}

static void
local_destroy (CtkWidget *window)
{
  guint blink_timeout = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (window), "blink"));
  if (blink_timeout)
    g_source_remove (blink_timeout);
}

static void
remote_destroy (CtkWidget *window)
{
  local_destroy (window);
  ctk_main_quit ();
}

static void
add_buttons (CtkWidget *widget G_GNUC_UNUSED,
	     CtkWidget *box)
{
  CtkWidget *add_button;
  CtkWidget *remove_button;

  add_button = ctk_button_new_with_mnemonic ("_Add");
  ctk_box_pack_start (CTK_BOX (box), add_button, TRUE, TRUE, 0);
  ctk_widget_show (add_button);

  g_signal_connect (add_button, "clicked",
		    G_CALLBACK (add_buttons),
		    box);

  remove_button = ctk_button_new_with_mnemonic ("_Remove");
  ctk_box_pack_start (CTK_BOX (box), remove_button, TRUE, TRUE, 0);
  ctk_widget_show (remove_button);

  g_signal_connect (remove_button, "clicked",
		    G_CALLBACK (remove_buttons),
		    add_button);
}

static CtkWidget *
create_combo (void)
{
  CtkComboBoxText *combo;
  CtkWidget *entry;

  combo = CTK_COMBO_BOX_TEXT (ctk_combo_box_text_new_with_entry ());

  ctk_combo_box_text_append_text (combo, "item0");
  ctk_combo_box_text_append_text (combo, "item1 item1");
  ctk_combo_box_text_append_text (combo, "item2 item2 item2");
  ctk_combo_box_text_append_text (combo, "item3 item3 item3 item3");
  ctk_combo_box_text_append_text (combo, "item4 item4 item4 item4 item4");
  ctk_combo_box_text_append_text (combo, "item5 item5 item5 item5 item5 item5");
  ctk_combo_box_text_append_text (combo, "item6 item6 item6 item6 item6");
  ctk_combo_box_text_append_text (combo, "item7 item7 item7 item7");
  ctk_combo_box_text_append_text (combo, "item8 item8 item8");
  ctk_combo_box_text_append_text (combo, "item9 item9");

  entry = ctk_bin_get_child (CTK_BIN (combo));
  ctk_entry_set_text (CTK_ENTRY (entry), "hello world");
  ctk_editable_select_region (CTK_EDITABLE (entry), 0, -1);

  return CTK_WIDGET (combo);
}

static CtkWidget *
create_menubar (CtkWindow *window)
{
  CtkAccelGroup *accel_group=NULL;
  CtkWidget *menubar;
  CtkWidget *menuitem;
  CtkWidget *menu;

  accel_group = ctk_accel_group_new ();
  ctk_window_add_accel_group (window, accel_group);

  menubar = ctk_menu_bar_new ();

  menuitem = ctk_menu_item_new_with_mnemonic ("_File");
  ctk_menu_shell_append (CTK_MENU_SHELL (menubar), menuitem);
  menu = ctk_menu_new ();
  ctk_menu_item_set_submenu (CTK_MENU_ITEM (menuitem), menu);
  menuitem = ctk_menu_item_new_with_mnemonic ("_New");
  g_signal_connect (menuitem, "activate", G_CALLBACK (print_hello), window);
  ctk_menu_shell_append (CTK_MENU_SHELL (menu), menuitem);
  menuitem = ctk_separator_menu_item_new ();
  ctk_menu_shell_append (CTK_MENU_SHELL (menu), menuitem);
  menuitem = ctk_menu_item_new_with_mnemonic ("_Quit");
  g_signal_connect (menuitem, "activate", G_CALLBACK (ctk_main_quit), window);
  ctk_menu_shell_append (CTK_MENU_SHELL (menu), menuitem);

  menuitem = ctk_menu_item_new_with_mnemonic ("O_K");
  ctk_menu_shell_append (CTK_MENU_SHELL (menubar), menuitem);

  menuitem = ctk_menu_item_new_with_mnemonic ("_Help");
  ctk_menu_shell_append (CTK_MENU_SHELL (menubar), menuitem);
  menu = ctk_menu_new ();
  ctk_menu_item_set_submenu (CTK_MENU_ITEM (menuitem), menu);
  menuitem = ctk_menu_item_new_with_mnemonic ("_About");
  g_signal_connect (menuitem, "activate", G_CALLBACK (print_hello), window);
  ctk_menu_shell_append (CTK_MENU_SHELL (menu), menuitem);

  return menubar;
}

static CtkWidget *
create_combo_box (void)
{
  CtkComboBoxText *combo_box = CTK_COMBO_BOX_TEXT (ctk_combo_box_text_new ());

  ctk_combo_box_text_append_text (combo_box, "This");
  ctk_combo_box_text_append_text (combo_box, "Is");
  ctk_combo_box_text_append_text (combo_box, "A");
  ctk_combo_box_text_append_text (combo_box, "ComboBox");
  
  return CTK_WIDGET (combo_box);
}

static CtkWidget *
create_content (CtkWindow *window, gboolean local)
{
  CtkWidget *vbox;
  CtkWidget *button;
  CtkWidget *frame;

  frame = ctk_frame_new (local? "Local" : "Remote");
  ctk_container_set_border_width (CTK_CONTAINER (frame), 3);
  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_box_set_homogeneous (CTK_BOX (vbox), TRUE);
  ctk_container_set_border_width (CTK_CONTAINER (vbox), 3);

  ctk_container_add (CTK_CONTAINER (frame), vbox);
  
  /* Combo */
  ctk_box_pack_start (CTK_BOX (vbox), create_combo(), TRUE, TRUE, 0);

  /* Entry */
  ctk_box_pack_start (CTK_BOX (vbox), ctk_entry_new(), TRUE, TRUE, 0);

  /* Close Button */
  button = ctk_button_new_with_mnemonic ("_Close");
  ctk_box_pack_start (CTK_BOX (vbox), button, TRUE, TRUE, 0);
  g_signal_connect_swapped (button, "clicked",
			    G_CALLBACK (ctk_widget_destroy), window);

  /* Blink Button */
  button = ctk_button_new_with_mnemonic ("_Blink");
  ctk_box_pack_start (CTK_BOX (vbox), button, TRUE, TRUE, 0);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (blink),
		    window);

  /* Menubar */
  ctk_box_pack_start (CTK_BOX (vbox), create_menubar (CTK_WINDOW (window)),
		      TRUE, TRUE, 0);

  /* Combo Box */
  ctk_box_pack_start (CTK_BOX (vbox), create_combo_box (), TRUE, TRUE, 0);
  
  add_buttons (NULL, vbox);

  return frame;
}

guint32
create_child_plug (guint32  xid,
		   gboolean local)
{
  CtkWidget *window;
  CtkWidget *content;

  window = ctk_plug_new (xid);

  g_signal_connect (window, "destroy",
		    local ? G_CALLBACK (local_destroy)
			  : G_CALLBACK (remote_destroy),
		    NULL);
  ctk_container_set_border_width (CTK_CONTAINER (window), 0);

  content = create_content (CTK_WINDOW (window), local);
  
  ctk_container_add (CTK_CONTAINER (window), content);

  ctk_widget_show_all (window);

  if (ctk_widget_get_realized (window))
#if defined (CDK_WINDOWING_X11)
    return CDK_WINDOW_XID (ctk_widget_get_window (window));
#elif defined (CDK_WINDOWING_WIN32)
    return (guint32) CDK_WINDOW_HWND (ctk_widget_get_window (window));
#elif defined (CDK_WINDOWING_BROADWAY)
    return (guint32) 0; /* Child windows not supported */
#endif
  else
    return 0;
}
