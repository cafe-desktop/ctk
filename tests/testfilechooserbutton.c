/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 2 -*- */

/* GTK+: ctkfilechooserbutton.c
 * 
 * Copyright (c) 2004 James M. Cape <jcape@ignore-your.tv>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#include <string.h>

#include <ctk/ctk.h>

static gchar *backend = "ctk+";
static gboolean rtl = FALSE;
static GOptionEntry entries[] = {
  { "backend", 'b', 0, G_OPTION_ARG_STRING, &backend, "The filesystem backend to use.", "ctk+" },
  { "right-to-left", 'r', 0, G_OPTION_ARG_NONE, &rtl, "Force right-to-left layout.", NULL },
  { NULL }
};

static gchar *ctk_src_dir = NULL;

static gboolean
delete_event_cb (GtkWidget *editor,
		 gint       response,
		 gpointer   user_data)
{
  ctk_widget_hide (editor);

  return TRUE;
}


static void
print_selected_path_clicked_cb (GtkWidget *button,
				gpointer   user_data)
{
  gchar *folder, *filename;

  folder = ctk_file_chooser_get_current_folder (user_data);
  filename = ctk_file_chooser_get_filename (user_data);
  g_message ("Currently Selected:\n\tFolder: `%s'\n\tFilename: `%s'\nDone.\n",
	     folder, filename);
  g_free (folder);
  g_free (filename);
}

static void
add_pwds_parent_as_shortcut_clicked_cb (GtkWidget *button,
					gpointer   user_data)
{
  GError *err = NULL;

  if (!ctk_file_chooser_add_shortcut_folder (user_data, ctk_src_dir, &err))
    {
      g_message ("Couldn't add `%s' as shortcut folder: %s", ctk_src_dir,
		 err->message);
      g_error_free (err);
    }
  else
    {
      g_message ("Added `%s' as shortcut folder.", ctk_src_dir);
    }
}

static void
del_pwds_parent_as_shortcut_clicked_cb (GtkWidget *button,
					gpointer   user_data)
{
  GError *err = NULL;

  if (!ctk_file_chooser_remove_shortcut_folder (user_data, ctk_src_dir, &err))
    {
      g_message ("Couldn't remove `%s' as shortcut folder: %s", ctk_src_dir,
		 err->message);
      g_error_free (err);
    }
  else
    {
      g_message ("Removed `%s' as shortcut folder.", ctk_src_dir);
    }
}

static void
unselect_all_clicked_cb (GtkWidget *button,
                         gpointer   user_data)
{
  ctk_file_chooser_unselect_all (user_data);
}

static void
tests_button_clicked_cb (GtkButton *real_button,
			 gpointer   user_data)
{
  GtkWidget *tests;

  tests = g_object_get_data (user_data, "tests-dialog");

  if (tests == NULL)
    {
      GtkWidget *box, *button;

      tests = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_title (CTK_WINDOW (tests),
			    "Tests - TestFileChooserButton");
      ctk_container_set_border_width (CTK_CONTAINER (tests), 12);
      ctk_window_set_transient_for (CTK_WINDOW (tests),
				    CTK_WINDOW (ctk_widget_get_toplevel (user_data)));

      box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_container_add (CTK_CONTAINER (tests), box);
      ctk_widget_show (box);

      button = ctk_button_new_with_label ("Print Selected Path");
      g_signal_connect (button, "clicked",
			G_CALLBACK (print_selected_path_clicked_cb), user_data);
      ctk_box_pack_start (CTK_BOX (box), button, FALSE, FALSE, 0);
      ctk_widget_show (button);

      button = ctk_button_new_with_label ("Add $PWD's Parent as Shortcut");
      g_signal_connect (button, "clicked",
			G_CALLBACK (add_pwds_parent_as_shortcut_clicked_cb), user_data);
      ctk_box_pack_start (CTK_BOX (box), button, FALSE, FALSE, 0);
      ctk_widget_show (button);

      button = ctk_button_new_with_label ("Remove $PWD's Parent as Shortcut");
      g_signal_connect (button, "clicked",
			G_CALLBACK (del_pwds_parent_as_shortcut_clicked_cb), user_data);
      ctk_box_pack_start (CTK_BOX (box), button, FALSE, FALSE, 0);
      ctk_widget_show (button);

      button = ctk_button_new_with_label ("Unselect all");
      g_signal_connect (button, "clicked",
			G_CALLBACK (unselect_all_clicked_cb), user_data);
      ctk_box_pack_start (CTK_BOX (box), button, FALSE, FALSE, 0);
      ctk_widget_show (button);

      g_signal_connect (tests, "delete-event", G_CALLBACK (delete_event_cb), NULL);
      g_object_set_data (user_data, "tests-dialog", tests);
    }

  ctk_window_present (CTK_WINDOW (tests));
}

static void
chooser_current_folder_changed_cb (GtkFileChooser *chooser,
				   gpointer        user_data)
{
  gchar *folder, *filename;

  folder = ctk_file_chooser_get_current_folder_uri (chooser);
  filename = ctk_file_chooser_get_uri (chooser);
  g_message ("%s::current-folder-changed\n\tFolder: `%s'\n\tFilename: `%s'\nDone.\n",
	     G_OBJECT_TYPE_NAME (chooser), folder, filename);
  g_free (folder);
  g_free (filename);
}

static void
chooser_selection_changed_cb (GtkFileChooser *chooser,
			      gpointer        user_data)
{
  gchar *filename;

  filename = ctk_file_chooser_get_uri (chooser);
  g_message ("%s::selection-changed\n\tSelection:`%s'\nDone.\n",
	     G_OBJECT_TYPE_NAME (chooser), filename);
  g_free (filename);
}

static void
chooser_file_activated_cb (GtkFileChooser *chooser,
			   gpointer        user_data)
{
  gchar *folder, *filename;

  folder = ctk_file_chooser_get_current_folder_uri (chooser);
  filename = ctk_file_chooser_get_uri (chooser);
  g_message ("%s::file-activated\n\tFolder: `%s'\n\tFilename: `%s'\nDone.\n",
	     G_OBJECT_TYPE_NAME (chooser), folder, filename);
  g_free (folder);
  g_free (filename);
}

static void
chooser_update_preview_cb (GtkFileChooser *chooser,
			   gpointer        user_data)
{
  gchar *filename;

  filename = ctk_file_chooser_get_preview_uri (chooser);
  if (filename != NULL)
    {
      g_message ("%s::update-preview\n\tPreview Filename: `%s'\nDone.\n",
		 G_OBJECT_TYPE_NAME (chooser), filename);
      g_free (filename);
    }
}


int
main (int   argc,
      char *argv[])
{
  GtkWidget *win, *vbox, *frame, *group_box;
  GtkWidget *hbox, *label, *chooser, *button;
  GtkSizeGroup *label_group;
  GOptionContext *context;
  gchar *cwd;

  context = g_option_context_new ("- test GtkFileChooserButton widget");
  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
  g_option_context_add_group (context, ctk_get_option_group (TRUE));
  g_option_context_parse (context, &argc, &argv, NULL);
  g_option_context_free (context);

  ctk_init (&argc, &argv);

  /* to test rtl layout, use "--right-to-left" */
  if (rtl)
    ctk_widget_set_default_direction (CTK_TEXT_DIR_RTL);

  cwd = g_get_current_dir();
  ctk_src_dir = g_path_get_dirname (cwd);
  g_free (cwd);

  win = ctk_dialog_new_with_buttons ("TestFileChooserButton", NULL, 0,
				     "_Quit", CTK_RESPONSE_CLOSE, NULL);
  g_signal_connect (win, "response", G_CALLBACK (ctk_main_quit), NULL);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 18);
  g_object_set (vbox, "margin", 6, NULL);
  ctk_container_add (CTK_CONTAINER (ctk_dialog_get_content_area (CTK_DIALOG (win))), vbox);

  frame = ctk_frame_new ("<b>GtkFileChooserButton</b>");
  ctk_frame_set_shadow_type (CTK_FRAME (frame), CTK_SHADOW_NONE);
  ctk_label_set_use_markup (CTK_LABEL (ctk_frame_get_label_widget (CTK_FRAME (frame))), TRUE);
  ctk_box_pack_start (CTK_BOX (vbox), frame, FALSE, FALSE, 0);

  ctk_widget_set_halign (frame, CTK_ALIGN_FILL);
  ctk_widget_set_valign (frame, CTK_ALIGN_FILL);
  g_object_set (frame, "margin-top", 6, "margin-start", 12, NULL);
  
  label_group = ctk_size_group_new (CTK_SIZE_GROUP_HORIZONTAL);
  
  group_box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);
  ctk_container_add (CTK_CONTAINER (frame), group_box);

  /* OPEN */
  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 12);
  ctk_box_pack_start (CTK_BOX (group_box), hbox, FALSE, FALSE, 0);

  label = ctk_label_new_with_mnemonic ("_Open:");
  ctk_size_group_add_widget (CTK_SIZE_GROUP (label_group), label);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  ctk_misc_set_alignment (CTK_MISC (label), 0.0, 0.5);
G_GNUC_END_IGNORE_DEPRECATIONS
  ctk_box_pack_start (CTK_BOX (hbox), label, FALSE, FALSE, 0);

  chooser = ctk_file_chooser_button_new ("Select A File - testfilechooserbutton",
                                         CTK_FILE_CHOOSER_ACTION_OPEN);
  ctk_file_chooser_add_shortcut_folder (CTK_FILE_CHOOSER (chooser), ctk_src_dir, NULL);
  ctk_file_chooser_remove_shortcut_folder (CTK_FILE_CHOOSER (chooser), ctk_src_dir, NULL);
  ctk_label_set_mnemonic_widget (CTK_LABEL (label), chooser);
  g_signal_connect (chooser, "current-folder-changed",
		    G_CALLBACK (chooser_current_folder_changed_cb), NULL);
  g_signal_connect (chooser, "selection-changed", G_CALLBACK (chooser_selection_changed_cb), NULL);
  g_signal_connect (chooser, "file-activated", G_CALLBACK (chooser_file_activated_cb), NULL);
  g_signal_connect (chooser, "update-preview", G_CALLBACK (chooser_update_preview_cb), NULL);
  ctk_box_pack_start (CTK_BOX (hbox), chooser, TRUE, TRUE, 0);

  button = ctk_button_new_with_label ("Tests");
  g_signal_connect (button, "clicked", G_CALLBACK (tests_button_clicked_cb), chooser);
  ctk_box_pack_start (CTK_BOX (hbox), button, FALSE, FALSE, 0);

  /* SELECT_FOLDER */
  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 12);
  ctk_box_pack_start (CTK_BOX (group_box), hbox, FALSE, FALSE, 0);

  label = ctk_label_new_with_mnemonic ("Select _Folder:");
  ctk_size_group_add_widget (CTK_SIZE_GROUP (label_group), label);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  ctk_misc_set_alignment (CTK_MISC (label), 0.0, 0.5);
G_GNUC_END_IGNORE_DEPRECATIONS
  ctk_box_pack_start (CTK_BOX (hbox), label, FALSE, FALSE, 0);

  chooser = ctk_file_chooser_button_new ("Select A Folder - testfilechooserbutton",
                                         CTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
  ctk_file_chooser_add_shortcut_folder (CTK_FILE_CHOOSER (chooser), ctk_src_dir, NULL);
  ctk_file_chooser_remove_shortcut_folder (CTK_FILE_CHOOSER (chooser), ctk_src_dir, NULL);
  ctk_file_chooser_add_shortcut_folder (CTK_FILE_CHOOSER (chooser), ctk_src_dir, NULL);
  ctk_label_set_mnemonic_widget (CTK_LABEL (label), chooser);
  g_signal_connect (chooser, "current-folder-changed",
		    G_CALLBACK (chooser_current_folder_changed_cb), NULL);
  g_signal_connect (chooser, "selection-changed", G_CALLBACK (chooser_selection_changed_cb), NULL);
  g_signal_connect (chooser, "file-activated", G_CALLBACK (chooser_file_activated_cb), NULL);
  g_signal_connect (chooser, "update-preview", G_CALLBACK (chooser_update_preview_cb), NULL);
  ctk_box_pack_start (CTK_BOX (hbox), chooser, TRUE, TRUE, 0);

  button = ctk_button_new_with_label ("Tests");
  g_signal_connect (button, "clicked", G_CALLBACK (tests_button_clicked_cb), chooser);
  ctk_box_pack_start (CTK_BOX (hbox), button, FALSE, FALSE, 0);

  g_object_unref (label_group);

  ctk_widget_show_all (win);
  ctk_window_present (CTK_WINDOW (win));

  ctk_main ();

  return 0;
}
