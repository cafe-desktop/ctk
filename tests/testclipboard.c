/*
 * Copyright (C) 2011  Red Hat, Inc.
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

CtkClipboard *clipboard;
CtkWidget *image;
CtkWidget *label;

#define SIZE 256.

static void
image_request_cb (CtkClipboard *clipboard G_GNUC_UNUSED,
                  GdkPixbuf    *pixbuf,
                  gpointer      data G_GNUC_UNUSED)
{
  gdouble factor;
  char *str;

  if (pixbuf != NULL)
    {
      GdkPixbuf *copy;
      int height;
      int width;

      height = gdk_pixbuf_get_height (pixbuf);
      width = gdk_pixbuf_get_width (pixbuf);

      factor = MAX ((SIZE / height), (SIZE / width));

      copy = gdk_pixbuf_scale_simple (pixbuf, width * factor, height * factor, GDK_INTERP_BILINEAR);
      ctk_image_set_from_pixbuf (CTK_IMAGE (image), copy);
      g_object_unref (copy);
      str = g_strdup_printf ("<b>Image</b> %d \342\234\225 %d", width, height);
    }
  else
    {
      str = g_strdup ("<b>No image data</b>");
    }
  ctk_label_set_markup (CTK_LABEL (label), str);
  g_free (str);
}

static void
update_display (void)
{
  ctk_clipboard_request_image (clipboard, image_request_cb, NULL);
}

static void
on_owner_change (CtkClipboard *clipboard G_GNUC_UNUSED,
                 CdkEvent     *event G_GNUC_UNUSED,
                 gpointer      user_data G_GNUC_UNUSED)
{
  update_display ();
}

static void
on_response (CtkDialog *dialog G_GNUC_UNUSED,
             gint       response_id,
             gpointer   user_data G_GNUC_UNUSED)
{
  switch (response_id)
    {
    case 0:
      /* copy large */
      {
        CtkIconTheme *theme;
        GdkPixbuf *pixbuf;
        theme = ctk_icon_theme_get_default ();
        pixbuf = ctk_icon_theme_load_icon (theme, "utilities-terminal", 1600, 0, NULL);
        g_assert_nonnull (pixbuf);
        ctk_clipboard_set_image (clipboard, pixbuf);
      }
      break;
    case 1:
      /* copy small */
      {
        CtkIconTheme *theme;
        GdkPixbuf *pixbuf;
        theme = ctk_icon_theme_get_default ();
        pixbuf = ctk_icon_theme_load_icon (theme, "utilities-terminal", 48, 0, NULL);
        g_assert_nonnull (pixbuf);
        ctk_clipboard_set_image (clipboard, pixbuf);
      }
      break;
    case CTK_RESPONSE_CLOSE:
    default:
      ctk_main_quit ();
      break;
    }
}

int
main (int argc, char **argv)
{
  CtkWidget *window;

  ctk_init (&argc, &argv);

  window = ctk_dialog_new_with_buttons ("Clipboard",
                                        NULL,
                                        0,
                                        "Copy Large", 0,
                                        "Copy Small", 1,
                                        "_Close", CTK_RESPONSE_CLOSE,
                                        NULL);

  image = ctk_image_new ();
  ctk_box_pack_start (CTK_BOX (ctk_dialog_get_content_area (CTK_DIALOG (window))), image, FALSE, FALSE, 0);
  label = ctk_label_new ("No data found");
  ctk_box_pack_start (CTK_BOX (ctk_dialog_get_content_area (CTK_DIALOG (window))), label, FALSE, FALSE, 0);

  g_signal_connect (window, "response", G_CALLBACK (on_response), NULL);

  clipboard = ctk_clipboard_get_for_display (ctk_widget_get_display (window),
                                             CDK_SELECTION_CLIPBOARD);
  g_signal_connect (clipboard, "owner-change", G_CALLBACK (on_owner_change), NULL);

  update_display ();

  ctk_widget_show_all (window);

  ctk_main ();

  return 0;
}
