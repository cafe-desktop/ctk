/* testnumerableicon.c
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

#include <stdlib.h>
#include <gtk/gtk.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

typedef struct {
  GIcon *numerable;
  GtkWidget *image;
  gboolean odd;
  GtkIconSize size;
} PackData;

static void
button_clicked_cb (GtkButton *b,
                   gpointer user_data)
{
  PackData *d = user_data;
  GtkCssProvider *provider;
  GtkStyleContext *style;
  GError *error = NULL;
  gchar *data, *bg_str, *grad1, *grad2;
  const gchar data_format[] = "GtkNumerableIcon { background-color: %s; color: #000000;"
    "background-image: -gtk-gradient (linear, 0 0, 1 1, from(%s), to(%s));"
    "font: Monospace 12;"
    /* "background-image: url('apple-red.png');" */
    "}";

  bg_str = g_strdup_printf ("rgb(%d,%d,%d)", g_random_int_range (0, 255), g_random_int_range (0, 255), g_random_int_range (0, 255));
  grad1 = g_strdup_printf ("rgb(%d,%d,%d)", g_random_int_range (0, 255), g_random_int_range (0, 255), g_random_int_range (0, 255));
  grad2 = g_strdup_printf ("rgb(%d,%d,%d)", g_random_int_range (0, 255), g_random_int_range (0, 255), g_random_int_range (0, 255));

  data = g_strdup_printf (data_format, bg_str, grad1, grad2);

  provider = ctk_css_provider_new ();
  ctk_css_provider_load_from_data (provider, data, -1, &error);

  g_assert (error == NULL);

  style = ctk_widget_get_style_context (d->image);
  ctk_style_context_add_provider (style, CTK_STYLE_PROVIDER (provider),
                                  CTK_STYLE_PROVIDER_PRIORITY_USER);

  if (d->odd) {
    ctk_numerable_icon_set_background_icon_name (CTK_NUMERABLE_ICON (d->numerable), NULL);
    ctk_numerable_icon_set_count (CTK_NUMERABLE_ICON (d->numerable), g_random_int_range (-99, 99));
  } else {
    ctk_numerable_icon_set_background_icon_name (CTK_NUMERABLE_ICON (d->numerable),
                                                 "emblem-favorite");
    ctk_numerable_icon_set_label (CTK_NUMERABLE_ICON (d->numerable), "IVX");
  }
  
  ctk_image_set_from_gicon (CTK_IMAGE (d->image), d->numerable, d->size);

  d->odd = !d->odd;

  g_free (data);
  g_free (bg_str);
  g_free (grad1);
  g_free (grad2);
  
  g_object_unref (provider);
}

static void
refresh_cb (GtkWidget *button,
            gpointer   user_data)
{
  PackData *d = user_data;

  ctk_image_set_from_gicon (CTK_IMAGE (d->image), d->numerable, d->size);
}

static void
pack_numerable (GtkWidget *parent,
                GtkIconSize size)
{
  PackData *d;
  GtkWidget *vbox, *label, *image, *button;
  gchar *str;
  GIcon *icon, *numerable;

  d = g_slice_new0 (PackData);

  image = ctk_image_new ();
  icon = g_themed_icon_new ("system-file-manager");
  numerable = ctk_numerable_icon_new (icon);

  d->image = image;
  d->numerable = numerable;
  d->odd = FALSE;
  d->size = size;

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 12);
  ctk_box_pack_start (CTK_BOX (parent), vbox, FALSE, FALSE, 0);

  ctk_numerable_icon_set_count (CTK_NUMERABLE_ICON (numerable), 42);
  ctk_box_pack_start (CTK_BOX (vbox), image, FALSE, FALSE, 0);
  ctk_numerable_icon_set_style_context (CTK_NUMERABLE_ICON (numerable),
                                        ctk_widget_get_style_context (image));
  ctk_image_set_from_gicon (CTK_IMAGE (image), numerable, size);

  label = ctk_label_new (NULL);
  str = g_strdup_printf ("Numerable icon, hash %u", g_icon_hash (numerable));
  ctk_label_set_label (CTK_LABEL (label), str);
  ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, FALSE, 0);

  button = ctk_button_new_with_label ("Change icon number");
  ctk_box_pack_start (CTK_BOX (vbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (button_clicked_cb), d);

  button = ctk_button_new_with_label ("Refresh");
  ctk_box_pack_start (CTK_BOX (vbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (refresh_cb), d);
}

int
main (int argc,
      char **argv)
{
  GtkWidget *hbox, *toplevel;

  ctk_init (&argc, &argv);

  toplevel = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 12);
  ctk_container_add (CTK_CONTAINER (toplevel), hbox);

  pack_numerable (hbox, CTK_ICON_SIZE_DIALOG);
  pack_numerable (hbox, CTK_ICON_SIZE_BUTTON);

  ctk_widget_show_all (toplevel);

  g_signal_connect (toplevel, "delete-event",
		    G_CALLBACK (ctk_main_quit), NULL);

  ctk_main ();

  return 0;
}

