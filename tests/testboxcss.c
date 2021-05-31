/* testbox.c
 *
 * Copyright (C) 2010 Benjamin Otte <otte@gnome.ogr>
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
#include <gtk/gtk.h>

/* This is exactly the style information you've been looking for */
#define GTK_STYLE_PROVIDER_PRIORITY_FORCE G_MAXUINT

#define DEFAULT_CSS \
  ".play {\n" \
  "  engine: none;\n" \
  "  background-image: none;\n" \
  "  background-color: red;\n" \
  "  border-color: black;\n" \
  "  border-radius: 0;\n" \
  "}\n" \
  "\n" \
  ".play:nth-child(even) {\n" \
  "  background-color: yellow;\n" \
  "  color: green;\n" \
  "}\n" \
  "\n" \
  ".play:nth-child(first) {\n" \
  "  border-radius: 5 0 0 5;\n" \
  "}\n" \
  "\n" \
  ".play:nth-child(last) {\n" \
  "  border-radius: 0 5 5 0;\n" \
  "}\n" \
  "\n"

static void
show_parsing_error (GtkCssProvider *provider,
                    GtkCssSection  *section,
                    const GError   *error,
                    GtkTextBuffer  *buffer)
{
  GtkTextIter start, end;
  const char *tag_name;

  ctk_text_buffer_get_iter_at_line_index (buffer,
                                          &start,
                                          ctk_css_section_get_start_line (section),
                                          ctk_css_section_get_start_position (section));
  ctk_text_buffer_get_iter_at_line_index (buffer,
                                          &end,
                                          ctk_css_section_get_end_line (section),
                                          ctk_css_section_get_end_position (section));

  if (g_error_matches (error, GTK_CSS_PROVIDER_ERROR, GTK_CSS_PROVIDER_ERROR_DEPRECATED))
    tag_name = "warning";
  else
    tag_name = "error";

  ctk_text_buffer_apply_tag_by_name (buffer, tag_name, &start, &end);
}
                    
static void
css_text_changed (GtkTextBuffer  *buffer,
                  GtkCssProvider *provider)
{
  GtkTextIter start, end;
  char *text;

  ctk_text_buffer_get_start_iter (buffer, &start);
  ctk_text_buffer_get_end_iter (buffer, &end);
  ctk_text_buffer_remove_all_tags (buffer, &start, &end);

  text = ctk_text_buffer_get_text (buffer, &start, &end, FALSE);
  ctk_css_provider_load_from_data (provider, text, -1, NULL);
  g_free (text);

  ctk_style_context_reset_widgets (gdk_screen_get_default ());
}

static void
remove_widget (GtkWidget *widget)
{
  ctk_container_remove (GTK_CONTAINER (ctk_widget_get_parent (widget)), widget);
}

static int count = 0;

static void
add_button (GtkBox *box)
{
  GtkWidget* button;
  char *text;

  text = g_strdup_printf ("Remove %d", ++count);
  button = (GtkWidget *)ctk_button_new_with_label (text);
  g_free (text);
  ctk_style_context_add_class (ctk_widget_get_style_context (button), "play");
  g_signal_connect_swapped (button,
                            "clicked",
                            G_CALLBACK (remove_widget),
                            button);
  ctk_widget_show (button);
  ctk_container_add (GTK_CONTAINER (box), button);
}

static void
add_toolbutton (GtkToolbar *toolbar)
{
  GtkWidget* button;
  char *text;

  text = g_strdup_printf ("Remove %d", ++count);
  button = (GtkWidget *)ctk_tool_button_new (NULL, text);
  g_free (text);
  ctk_style_context_add_class (ctk_widget_get_style_context (button), "play");
  g_signal_connect_swapped (button,
                            "clicked",
                            G_CALLBACK (remove_widget),
                            button);
  ctk_widget_show (button);
  ctk_container_add (GTK_CONTAINER (toolbar), button);
}

static void
set_orientation (GtkSwitch *switch_)
{
  ctk_widget_set_default_direction (ctk_switch_get_active (switch_) ? GTK_TEXT_DIR_LTR : GTK_TEXT_DIR_RTL);
}

gint
main (gint argc, gchar **argv)
{
  GtkWidget *window, *main_box, *container, *child;
  GtkWidget *box, *toolbar;
  GtkStyleProvider *provider;
  GtkTextBuffer *css;
  
  ctk_init (&argc, &argv);

  css = ctk_text_buffer_new (NULL);
  ctk_text_buffer_create_tag (css,
                              "warning",
                              "background", "rgba(255,255,0,0.3)",
                              NULL);
  ctk_text_buffer_create_tag (css,
                              "error",
                              "background", "rgba(255,0,0,0.3)",
                              NULL);

  provider = GTK_STYLE_PROVIDER (ctk_css_provider_new ());
  ctk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                             provider,
                                             GTK_STYLE_PROVIDER_PRIORITY_FORCE);
  
  window = ctk_window_new (GTK_WINDOW_TOPLEVEL);

  g_signal_connect (window, "destroy", G_CALLBACK(ctk_main_quit), NULL);
  g_signal_connect (window, "delete_event", G_CALLBACK (ctk_main_quit), NULL);

  main_box = ctk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  ctk_container_add (GTK_CONTAINER (window), main_box);

  toolbar = ctk_toolbar_new ();
  ctk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_TEXT);
  ctk_box_pack_start (GTK_BOX (main_box), toolbar, FALSE, TRUE, 0);

  box = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  ctk_box_pack_start (GTK_BOX (main_box), box, FALSE, TRUE, 0);

  container = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_min_content_width (GTK_SCROLLED_WINDOW (container), 200);
  ctk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (container), 200);
  ctk_box_pack_start (GTK_BOX (main_box), container, TRUE, TRUE, 0);
  child = ctk_text_view_new_with_buffer (css);
  ctk_container_add (GTK_CONTAINER (container), child);
  g_signal_connect (css,
                    "changed",
                    G_CALLBACK (css_text_changed),
                    provider);
  ctk_text_buffer_set_text (css,
                            DEFAULT_CSS,
                            -1);
  g_signal_connect (provider,
                    "parsing-error",
                    G_CALLBACK (show_parsing_error),
                    ctk_text_view_get_buffer (GTK_TEXT_VIEW (child)));

  container = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  ctk_box_pack_start (GTK_BOX (main_box), container, FALSE, TRUE, 0);
  child = ctk_switch_new ();
  ctk_switch_set_active (GTK_SWITCH (child), ctk_widget_get_default_direction () == GTK_TEXT_DIR_LTR);
  g_signal_connect (child,
                    "notify::active",
                    G_CALLBACK (set_orientation),
                    NULL);
  ctk_box_pack_start (GTK_BOX (container), child, FALSE, FALSE, 0);
  child = ctk_label_new ("left-to-right");
  ctk_box_pack_start (GTK_BOX (container), child, FALSE, FALSE, 0);
  child = ctk_button_new_with_label ("Add button");
  g_signal_connect_swapped (child,
                            "clicked",
                            G_CALLBACK (add_button),
                            box);
  ctk_box_pack_end (GTK_BOX (container), child, FALSE, FALSE, 0);
  child = ctk_button_new_with_label ("Add toolbutton");
  g_signal_connect_swapped (child,
                            "clicked",
                            G_CALLBACK (add_toolbutton),
                            toolbar);
  ctk_box_pack_end (GTK_BOX (container), child, FALSE, FALSE, 0);

  add_toolbutton (GTK_TOOLBAR (toolbar));
  add_toolbutton (GTK_TOOLBAR (toolbar));
  add_toolbutton (GTK_TOOLBAR (toolbar));
  add_toolbutton (GTK_TOOLBAR (toolbar));

  add_button (GTK_BOX (box));
  add_button (GTK_BOX (box));
  add_button (GTK_BOX (box));
  add_button (GTK_BOX (box));

  ctk_widget_show_all (window);

  ctk_main ();
  
  return 0;
}
