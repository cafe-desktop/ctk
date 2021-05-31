/* testvolumebutton.c
 * Copyright (C) 2007  Red Hat, Inc.
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

#include <ctk/ctk.h>

static void
value_changed (GtkWidget *button,
               gdouble volume,
               gpointer user_data)
{
  g_message ("volume changed to %f", volume);
}

static void
toggle_orientation (GtkWidget *button,
                    GtkWidget *scalebutton)
{
  if (ctk_orientable_get_orientation (CTK_ORIENTABLE (scalebutton)) ==
      CTK_ORIENTATION_HORIZONTAL)
    {
      ctk_orientable_set_orientation (CTK_ORIENTABLE (scalebutton),
                                        CTK_ORIENTATION_VERTICAL);
    }
  else
    {
      ctk_orientable_set_orientation (CTK_ORIENTABLE (scalebutton),
                                        CTK_ORIENTATION_HORIZONTAL);
    }
}

static void
response_cb (GtkDialog *dialog,
             gint       arg1,
             gpointer   user_data)
{
  ctk_widget_destroy (CTK_WIDGET (dialog));
}

static gboolean
show_error (gpointer data)
{
  GtkWindow *window = (GtkWindow *) data;
  GtkWidget *dialog;

  g_message ("showing error");

  dialog = ctk_message_dialog_new (window,
                                   CTK_DIALOG_MODAL,
                                   CTK_MESSAGE_INFO,
                                   CTK_BUTTONS_CLOSE,
                                   "This should have unbroken the grab");
  g_signal_connect (G_OBJECT (dialog),
                    "response",
                    G_CALLBACK (response_cb), NULL);
  ctk_widget_show (dialog);

  return G_SOURCE_REMOVE;
}

int
main (int    argc,
      char **argv)
{
  GtkWidget *window;
  GtkWidget *button;
  GtkWidget *button2;
  GtkWidget *button3;
  GtkWidget *box;
  GtkWidget *vbox;

  ctk_init (&argc, &argv);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_default_size (CTK_WINDOW (window), 400, 300);
  button = ctk_volume_button_new ();
  button2 = ctk_volume_button_new ();
  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);

  g_signal_connect (G_OBJECT (button), "value-changed",
                    G_CALLBACK (value_changed),
                    NULL);

  ctk_container_add (CTK_CONTAINER (window), vbox);
  ctk_container_add (CTK_CONTAINER (vbox), box);
  ctk_container_add (CTK_CONTAINER (box), button);
  ctk_container_add (CTK_CONTAINER (box), button2);

  button3 = ctk_button_new_with_label ("Toggle orientation");
  ctk_container_add (CTK_CONTAINER (box), button3);

  g_signal_connect (G_OBJECT (button3), "clicked",
                    G_CALLBACK (toggle_orientation),
                    button);
  g_signal_connect (G_OBJECT (button3), "clicked",
                    G_CALLBACK (toggle_orientation),
                    button2);

  ctk_widget_show_all (window);
  ctk_button_clicked (CTK_BUTTON (button));
  g_timeout_add (4000, (GSourceFunc) show_error, window);

  ctk_main ();

  return 0;
}
