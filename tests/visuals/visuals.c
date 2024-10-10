/* visuals: UI runner for visual CtkBuilder files
 *
 * Copyright (C) 2012 Red Hat, Inc.
 *
 * This  library is free  software; you can  redistribute it and/or
 * modify it  under  the terms  of the  GNU Lesser  General  Public
 * License  as published  by the Free  Software  Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed  in the hope that it will be useful,
 * but  WITHOUT ANY WARRANTY; without even  the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License  along  with  this library;  if not,  write to  the Free
 *
 * Author: Cosimo Cecchi <cosimoc@gnome.org>
 *
 */

#include <ctk/ctk.h>

static void
dark_button_toggled_cb (CtkToggleButton *button,
                        gpointer         user_data G_GNUC_UNUSED)
{
  gboolean active = ctk_toggle_button_get_active (button);
  CtkSettings *settings = ctk_settings_get_default ();

  g_object_set (settings,
                "ctk-application-prefer-dark-theme", active,
                NULL);
}

static void
create_dark_popup (CtkWidget *parent)
{
  CtkWidget *popup = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  CtkWidget *button = ctk_toggle_button_new_with_label ("Dark");

  ctk_window_set_decorated (CTK_WINDOW (popup), FALSE);
  ctk_widget_set_size_request (popup, 100, 100);
  ctk_window_set_resizable (CTK_WINDOW (popup), FALSE);

  g_signal_connect (popup, "delete-event",
                    G_CALLBACK (ctk_true), NULL);

  ctk_container_add (CTK_CONTAINER (popup), button);
  g_signal_connect (button, "toggled",
                    G_CALLBACK (dark_button_toggled_cb), NULL);

  ctk_window_set_transient_for (CTK_WINDOW (popup), CTK_WINDOW (parent));

  ctk_widget_show_all (popup);
}

int
main (int argc, char *argv[])
{
  CtkBuilder *builder;
  CtkWidget  *window;
  const gchar *filename;

  ctk_init (&argc, &argv);

  if (argc < 2)
    return 1;
  filename = argv[1];

  builder = ctk_builder_new ();
  ctk_builder_add_from_file (builder, filename, NULL);
  ctk_builder_connect_signals (builder, NULL);

  window = CTK_WIDGET (ctk_builder_get_object (builder, "window1"));
  g_object_unref (G_OBJECT (builder));
  g_signal_connect (window, "destroy",
                    G_CALLBACK (ctk_main_quit), NULL);
  ctk_widget_show (window);

  create_dark_popup (window);
  ctk_main ();

  return 0;
}
