/* ctkstatusicon-x11.c:
 *
 * Copyright (C) 2003 Sun Microsystems, Inc.
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
 *
 * Authors:
 *	Mark McLoughlin <mark@skynet.ie>
 */

#define CDK_DISABLE_DEPRECATION_WARNINGS
#include <ctk/ctk.h>
#include <stdlib.h>

typedef enum
{
  TEST_STATUS_INFO,
  TEST_STATUS_QUESTION
} TestStatus;

static TestStatus status = TEST_STATUS_INFO;
static gint timeout = 0;
static GSList *icons = NULL;

static gboolean
size_changed_cb (CtkStatusIcon *icon,
		 int size)
{
  g_print ("status icon %p size-changed size = %d\n", icon, size);

  return FALSE;
}

static void
embedded_changed_cb (CtkStatusIcon *icon)
{
  g_print ("status icon %p embedded changed to %d\n", icon,
	   ctk_status_icon_is_embedded (icon));
}

static void
orientation_changed_cb (CtkStatusIcon *icon)
{
  CtkOrientation orientation;

  g_object_get (icon, "orientation", &orientation, NULL);
  g_print ("status icon %p orientation changed to %d\n", icon, orientation);
}

static void
screen_changed_cb (CtkStatusIcon *icon)
{
  g_print ("status icon %p screen changed to %p\n", icon,
	   ctk_status_icon_get_screen (icon));
}

static void
update_icon (void)
{
  GSList *l;
  gchar *icon_name;
  gchar *tooltip;

  if (status == TEST_STATUS_INFO)
    {
      icon_name = "dialog-information";
      tooltip = "Some Information ...";
    }
  else
    {
      icon_name = "dialog-question";
      tooltip = "Some Question ...";
    }

  for (l = icons; l; l = l->next)
    {
      CtkStatusIcon *status_icon = l->data;

      ctk_status_icon_set_from_icon_name (status_icon, icon_name);
      ctk_status_icon_set_tooltip_text (status_icon, tooltip);
    }
}

static gboolean
timeout_handler (gpointer data G_GNUC_UNUSED)
{
  if (status == TEST_STATUS_INFO)
    status = TEST_STATUS_QUESTION;
  else
    status = TEST_STATUS_INFO;

  update_icon ();

  return TRUE;
}

static void
visible_toggle_toggled (CtkToggleButton *toggle)
{
  GSList *l;

  for (l = icons; l; l = l->next)
    ctk_status_icon_set_visible (CTK_STATUS_ICON (l->data),
                                 ctk_toggle_button_get_active (toggle));
}

static void
timeout_toggle_toggled (CtkToggleButton *toggle G_GNUC_UNUSED)
{
  if (timeout)
    {
      g_source_remove (timeout);
      timeout = 0;
    }
  else
    {
      timeout = cdk_threads_add_timeout (2000, timeout_handler, NULL);
    }
}

static void
icon_activated (CtkStatusIcon *icon)
{
  CtkWidget *dialog;

  dialog = g_object_get_data (G_OBJECT (icon), "test-status-icon-dialog");

  if (dialog == NULL)
    {
      CtkWidget *content_area;
      CtkWidget *toggle;

      dialog = ctk_message_dialog_new (NULL, 0,
				       CTK_MESSAGE_QUESTION,
				       CTK_BUTTONS_CLOSE,
				       "You wanna test the status icon ?");

      ctk_window_set_screen (CTK_WINDOW (dialog), ctk_status_icon_get_screen (icon));
      ctk_window_set_position (CTK_WINDOW (dialog), CTK_WIN_POS_CENTER);

      g_object_set_data_full (G_OBJECT (icon), "test-status-icon-dialog",
			      dialog, (GDestroyNotify) ctk_widget_destroy);

      g_signal_connect (dialog, "response", 
			G_CALLBACK (ctk_widget_hide), NULL);
      g_signal_connect (dialog, "delete_event", 
			G_CALLBACK (ctk_widget_hide_on_delete), NULL);

      content_area = ctk_dialog_get_content_area (CTK_DIALOG (dialog));

      toggle = ctk_toggle_button_new_with_mnemonic ("_Show the icon");
      ctk_box_pack_end (CTK_BOX (content_area), toggle, TRUE, TRUE, 6);
      ctk_widget_show (toggle);

      ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (toggle),
				    ctk_status_icon_get_visible (icon));
      g_signal_connect (toggle, "toggled", 
			G_CALLBACK (visible_toggle_toggled), NULL);

      toggle = ctk_toggle_button_new_with_mnemonic ("_Change images");
      ctk_box_pack_end (CTK_BOX (content_area), toggle, TRUE, TRUE, 6);
      ctk_widget_show (toggle);

      ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (toggle),
				    timeout != 0);
      g_signal_connect (toggle, "toggled", 
			G_CALLBACK (timeout_toggle_toggled), NULL);
    }

  ctk_window_present (CTK_WINDOW (dialog));
}

static void
do_quit (CtkMenuItem *item G_GNUC_UNUSED)
{
  GSList *l;

  for (l = icons; l; l = l->next)
    {
      CtkStatusIcon *icon = l->data;

      ctk_status_icon_set_visible (icon, FALSE);
      g_object_unref (icon);
    }

  g_slist_free (icons);
  icons = NULL;

  ctk_main_quit ();
}

static void
do_exit (CtkMenuItem *item G_GNUC_UNUSED)
{
  exit (0);
}

static void 
popup_menu (CtkStatusIcon *icon,
	    guint          button,
	    guint32        activate_time)
{
  CtkWidget *menu, *menuitem;

  menu = ctk_menu_new ();

  ctk_menu_set_screen (CTK_MENU (menu),
                       ctk_status_icon_get_screen (icon));

  menuitem = ctk_menu_item_new_with_label ("Quit");
  g_signal_connect (menuitem, "activate", G_CALLBACK (do_quit), NULL);

  ctk_menu_shell_append (CTK_MENU_SHELL (menu), menuitem);

  ctk_widget_show (menuitem);

  menuitem = ctk_menu_item_new_with_label ("Exit abruptly");
  g_signal_connect (menuitem, "activate", G_CALLBACK (do_exit), NULL);

  ctk_menu_shell_append (CTK_MENU_SHELL (menu), menuitem);

  ctk_widget_show (menuitem);

  ctk_menu_popup (CTK_MENU (menu), 
		  NULL, NULL,
		  ctk_status_icon_position_menu, icon,
		  button, activate_time);
}

int
main (int argc, char **argv)
{
  CtkStatusIcon *icon;

  ctk_init (&argc, &argv);

  icon = ctk_status_icon_new ();

  g_signal_connect (icon, "size-changed", G_CALLBACK (size_changed_cb), NULL);
  g_signal_connect (icon, "notify::embedded", G_CALLBACK (embedded_changed_cb), NULL);
  g_signal_connect (icon, "notify::orientation", G_CALLBACK (orientation_changed_cb), NULL);
  g_signal_connect (icon, "notify::screen", G_CALLBACK (screen_changed_cb), NULL);
  g_print ("icon size %d\n", ctk_status_icon_get_size (icon));

  g_signal_connect (icon, "activate",
                    G_CALLBACK (icon_activated), NULL);

  g_signal_connect (icon, "popup-menu",
                    G_CALLBACK (popup_menu), NULL);

  icons = g_slist_append (icons, icon);

  update_icon ();

  timeout = cdk_threads_add_timeout (2000, timeout_handler, icon);

  ctk_main ();

  return 0;
}
