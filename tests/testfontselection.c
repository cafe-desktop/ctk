/* testfontselection.c
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

#include "config.h"

#define CDK_DISABLE_DEPRECATION_WARNINGS

#include <ctk/ctk.h>

static void
notify_font_name_cb (GObject *fontsel, GParamSpec *pspec, gpointer data)
{
  g_debug ("Changed font name %s", ctk_font_selection_get_font_name (CTK_FONT_SELECTION (fontsel)));
}

static void
notify_preview_text_cb (GObject *fontsel, GParamSpec *pspec, gpointer data)
{
  g_debug ("Changed preview text %s", ctk_font_selection_get_preview_text (CTK_FONT_SELECTION (fontsel)));
}

int
main (int argc, char *argv[])
{
  CtkWidget *window;
  CtkWidget *hbox;
  CtkWidget *fontsel;
  
  ctk_init (NULL, NULL);
    
  fontsel = ctk_font_selection_new ();

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_widget_set_size_request (window, 600, 600);
  hbox = ctk_hbox_new (FALSE, 6);
  ctk_container_add (CTK_CONTAINER (window), hbox);

#ifndef CTK_DISABLE_DEPRECATED
  g_object_ref (ctk_font_selection_get_size_list (CTK_FONT_SELECTION (fontsel)));
  g_object_ref (ctk_font_selection_get_family_list (CTK_FONT_SELECTION (fontsel)));
  g_object_ref (ctk_font_selection_get_face_list (CTK_FONT_SELECTION (fontsel)));

  ctk_container_add (CTK_CONTAINER (hbox), ctk_font_selection_get_size_list (CTK_FONT_SELECTION (fontsel)));
  ctk_container_add (CTK_CONTAINER (hbox), ctk_font_selection_get_family_list (CTK_FONT_SELECTION (fontsel)));
  ctk_container_add (CTK_CONTAINER (hbox), ctk_font_selection_get_face_list (CTK_FONT_SELECTION (fontsel)));
#endif 

  ctk_container_add (CTK_CONTAINER (hbox), fontsel);

  ctk_widget_show_all (window);

  g_signal_connect (G_OBJECT (window), "delete-event",          G_CALLBACK(ctk_main_quit), NULL);
  g_signal_connect (G_OBJECT (fontsel), "notify::font-name",    G_CALLBACK(notify_font_name_cb), NULL);
  g_signal_connect (G_OBJECT (fontsel), "notify::preview-text", G_CALLBACK(notify_preview_text_cb), NULL);

  ctk_font_selection_set_font_name (CTK_FONT_SELECTION (fontsel), "Bitstream Vera Sans 45");
  ctk_font_selection_set_preview_text (CTK_FONT_SELECTION (fontsel), "[user@host ~]$ ");

  ctk_main ();

  ctk_widget_destroy (window);

  return 0;
}
