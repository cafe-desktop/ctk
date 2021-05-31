/* testappchooser.c
 * Copyright (C) 2010 Red Hat, Inc.
 * Authors: Cosimo Cecchi
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

#include <stdlib.h>
#include <ctk/ctk.h>

static GtkWidget *toplevel;
static GFile *file;
static GtkWidget *grid, *file_l, *open;
static GtkWidget *radio_file, *radio_content, *dialog;
static GtkWidget *app_chooser_widget;
static GtkWidget *def, *recommended, *fallback, *other, *all;

static void
dialog_response (GtkDialog *d,
                 gint       response_id,
                 gpointer   user_data)
{
  GAppInfo *app_info;
  const gchar *name;

  g_print ("Response: %d\n", response_id);

  if (response_id == CTK_RESPONSE_OK)
    {
      app_info = ctk_app_chooser_get_app_info (CTK_APP_CHOOSER (d));
      if (app_info)
        {
          name = g_app_info_get_name (app_info);
          g_print ("Application selected: %s\n", name);
          g_object_unref (app_info);
        }
      else
        g_print ("No application selected\n");
    }

  ctk_widget_destroy (CTK_WIDGET (d));
  dialog = NULL;
}

static void
bind_props (void)
{
  g_object_bind_property (def, "active",
                          app_chooser_widget, "show-default",
                          G_BINDING_SYNC_CREATE);
  g_object_bind_property (recommended, "active",
                          app_chooser_widget, "show-recommended",
                          G_BINDING_SYNC_CREATE);
  g_object_bind_property (fallback, "active",
                          app_chooser_widget, "show-fallback",
                          G_BINDING_SYNC_CREATE);
  g_object_bind_property (other, "active",
                          app_chooser_widget, "show-other",
                          G_BINDING_SYNC_CREATE);
  g_object_bind_property (all, "active",
                          app_chooser_widget, "show-all",
                          G_BINDING_SYNC_CREATE);
}

static void
prepare_dialog (void)
{
  gboolean use_file = FALSE;
  gchar *content_type = NULL;

  if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (radio_file)))
    use_file = TRUE;
  else if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (radio_content)))
    use_file = FALSE;

  if (use_file)
    {
      dialog = ctk_app_chooser_dialog_new (CTK_WINDOW (toplevel), 0, file);
    }
  else
    {
      GFileInfo *info;

      info = g_file_query_info (file,
                                G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                                0, NULL, NULL);
      content_type = g_strdup (g_file_info_get_content_type (info));

      g_object_unref (info);

      dialog = ctk_app_chooser_dialog_new_for_content_type (CTK_WINDOW (toplevel),
                                                            0, content_type);
    }

  ctk_app_chooser_dialog_set_heading (CTK_APP_CHOOSER_DIALOG (dialog), "Select one already, you <i>fool</i>");

  g_signal_connect (dialog, "response",
                    G_CALLBACK (dialog_response), NULL);

  g_free (content_type);

  app_chooser_widget = ctk_app_chooser_dialog_get_widget (CTK_APP_CHOOSER_DIALOG (dialog));
  bind_props ();
}

static void
display_dialog (void)
{
  if (dialog == NULL)
    prepare_dialog ();

  ctk_widget_show (dialog);
}

static void
button_clicked (GtkButton *b,
                gpointer   user_data)
{
  GtkWidget *w;
  gchar *path;

  w = ctk_file_chooser_dialog_new ("Select file",
                                   CTK_WINDOW (toplevel),
                                   CTK_FILE_CHOOSER_ACTION_OPEN,
                                   "_Cancel", CTK_RESPONSE_CANCEL,
                                   "_Open", CTK_RESPONSE_ACCEPT,
                                   NULL);

  ctk_dialog_run (CTK_DIALOG (w));
  file = ctk_file_chooser_get_file (CTK_FILE_CHOOSER (w));
  path = g_file_get_path (file);
  ctk_button_set_label (CTK_BUTTON (file_l), path);

  ctk_widget_destroy (w);

  ctk_widget_set_sensitive (open, TRUE);

  g_free (path);
}

int
main (int argc, char **argv)
{
  GtkWidget *w1;
  gchar *path;

  ctk_init (&argc, &argv);

  toplevel = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_container_set_border_width (CTK_CONTAINER (toplevel), 12);
  grid = ctk_grid_new ();

  w1 = ctk_label_new ("File:");
  ctk_widget_set_halign (w1, CTK_ALIGN_START);
  ctk_grid_attach (CTK_GRID (grid),
                   w1, 0, 0, 1, 1);

  file_l = ctk_button_new ();
  path = g_build_filename (g_get_current_dir (), "apple-red.png", NULL);
  file = g_file_new_for_path (path);
  ctk_button_set_label (CTK_BUTTON (file_l), path);
  g_free (path);

  ctk_widget_set_halign (file_l, CTK_ALIGN_START);
  ctk_grid_attach_next_to (CTK_GRID (grid), file_l,
                           w1, CTK_POS_RIGHT, 3, 1);
  g_signal_connect (file_l, "clicked",
                    G_CALLBACK (button_clicked), NULL);

  radio_file = ctk_radio_button_new_with_label (NULL, "Use GFile");
  radio_content = ctk_radio_button_new_with_label_from_widget (CTK_RADIO_BUTTON (radio_file),
                                                               "Use content type");

  ctk_grid_attach (CTK_GRID (grid), radio_file,
                   0, 1, 1, 1);
  ctk_grid_attach_next_to (CTK_GRID (grid), radio_content,
                           radio_file, CTK_POS_BOTTOM, 1, 1);

  open = ctk_button_new_with_label ("Trigger App Chooser dialog");
  ctk_grid_attach_next_to (CTK_GRID (grid), open,
                           radio_content, CTK_POS_BOTTOM, 1, 1);

  recommended = ctk_check_button_new_with_label ("Show recommended");
  ctk_grid_attach_next_to (CTK_GRID (grid), recommended,
                           open, CTK_POS_BOTTOM, 1, 1);
  g_object_set (recommended, "active", TRUE, NULL);

  fallback = ctk_check_button_new_with_label ("Show fallback");
  ctk_grid_attach_next_to (CTK_GRID (grid), fallback,
                           recommended, CTK_POS_RIGHT, 1, 1);

  other = ctk_check_button_new_with_label ("Show other");
  ctk_grid_attach_next_to (CTK_GRID (grid), other,
                           fallback, CTK_POS_RIGHT, 1, 1);

  all = ctk_check_button_new_with_label ("Show all");
  ctk_grid_attach_next_to (CTK_GRID (grid), all,
                           other, CTK_POS_RIGHT, 1, 1);

  def = ctk_check_button_new_with_label ("Show default");
  ctk_grid_attach_next_to (CTK_GRID (grid), def,
                           all, CTK_POS_RIGHT, 1, 1);

  g_object_set (recommended, "active", TRUE, NULL);
  prepare_dialog ();
  g_signal_connect (open, "clicked",
                    G_CALLBACK (display_dialog), NULL);

  ctk_container_add (CTK_CONTAINER (toplevel), grid);

  ctk_widget_show_all (toplevel);
  g_signal_connect (toplevel, "delete-event",
                    G_CALLBACK (ctk_main_quit), NULL);

  ctk_main ();

  return EXIT_SUCCESS;
}
