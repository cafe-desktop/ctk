/* testfontchooser.c
 * Copyright (C) 2011 Alberto Ruiz <aruiz@gnome.org>
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
notify_font_cb (CtkFontChooser *fontchooser, GParamSpec *pspec, gpointer data)
{
  PangoFontFamily *family;
  PangoFontFace *face;

  g_debug ("Changed font name %s", ctk_font_chooser_get_font (fontchooser));

  family = ctk_font_chooser_get_font_family (fontchooser);
  face = ctk_font_chooser_get_font_face (fontchooser);
  if (family)
    {
       g_debug ("  Family: %s is-monospace:%s",
                pango_font_family_get_name (family),
                pango_font_family_is_monospace (family) ? "true" : "false");
    }
  else
    g_debug ("  No font family!");

  if (face)
    g_debug ("  Face description: %s", pango_font_face_get_face_name (face));
  else
    g_debug ("  No font face!");
}

static void
notify_preview_text_cb (GObject *fontchooser, GParamSpec *pspec, gpointer data)
{
  g_debug ("Changed preview text %s", ctk_font_chooser_get_preview_text (CTK_FONT_CHOOSER (fontchooser)));
}

static void
font_activated_cb (CtkFontChooser *chooser, const gchar *font_name, gpointer data)
{
  g_debug ("font-activated: %s", font_name);
}

int
main (int argc, char *argv[])
{
  CtkWidget *window;
  CtkWidget *box;
  CtkWidget *fontchooser;

  ctk_init (NULL, NULL);

  fontchooser = ctk_font_chooser_widget_new ();

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_widget_set_size_request (window, 600, 600);
  box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 6);
  ctk_container_add (CTK_CONTAINER (window), box);
  ctk_container_add (CTK_CONTAINER (box), fontchooser);

  ctk_widget_show_all (window);

  g_signal_connect (window, "delete-event",
                    G_CALLBACK (ctk_main_quit), NULL);
  g_signal_connect (fontchooser, "notify::font",
                    G_CALLBACK (notify_font_cb), NULL);
  g_signal_connect (fontchooser, "notify::preview-text",
                    G_CALLBACK (notify_preview_text_cb), NULL);
  g_signal_connect (fontchooser, "font-activated",
                    G_CALLBACK (font_activated_cb), NULL);

  ctk_font_chooser_set_font (CTK_FONT_CHOOSER (fontchooser), "Bitstream Vera Sans 45");
  ctk_font_chooser_set_preview_text (CTK_FONT_CHOOSER (fontchooser), "[user@host ~]$ &>>");
  ctk_font_chooser_set_show_preview_entry (CTK_FONT_CHOOSER (fontchooser), FALSE);

  ctk_main ();

  return 0;
}
