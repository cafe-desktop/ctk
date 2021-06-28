/* testgiconpixbuf.c
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

#include <ctk/ctk.h>

int
main (int argc,
      char **argv)
{
  GdkPixbuf *pixbuf, *otherpix;
  CtkWidget *image, *image2, *hbox, *vbox, *label, *toplevel;
  GIcon *emblemed;
  GEmblem *emblem;
  gchar *str;

  ctk_init (&argc, &argv);

  pixbuf = cdk_pixbuf_new_from_file ("apple-red.png", NULL);
  toplevel = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 12);
  ctk_container_add (CTK_CONTAINER (toplevel), hbox);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 12);
  ctk_box_pack_start (CTK_BOX (hbox), vbox, FALSE, FALSE, 0);

  image = ctk_image_new_from_gicon (G_ICON (pixbuf), CTK_ICON_SIZE_DIALOG);
  ctk_box_pack_start (CTK_BOX (vbox), image, FALSE, FALSE, 0);

  label = ctk_label_new (NULL);
  str = g_strdup_printf ("Normal icon, hash %u", g_icon_hash (G_ICON (pixbuf)));
  ctk_label_set_label (CTK_LABEL (label), str);
  ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, FALSE, 0);

  otherpix = cdk_pixbuf_new_from_file ("gnome-textfile.png", NULL);
  emblem = g_emblem_new (G_ICON (otherpix));
  emblemed = g_emblemed_icon_new (G_ICON (pixbuf), emblem);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 12);
  ctk_box_pack_start (CTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  
  image2 = ctk_image_new_from_gicon (emblemed, CTK_ICON_SIZE_DIALOG);
  ctk_box_pack_start (CTK_BOX (vbox), image2, FALSE, FALSE, 0);

  label = ctk_label_new (NULL);
  str = g_strdup_printf ("Emblemed icon, hash %u", g_icon_hash (emblemed));
  ctk_label_set_label (CTK_LABEL (label), str);
  ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, FALSE, 0);

  ctk_widget_show_all (toplevel);

  g_signal_connect (toplevel, "delete-event",
		    G_CALLBACK (ctk_main_quit), NULL);

  ctk_main ();

  return 0;
}
