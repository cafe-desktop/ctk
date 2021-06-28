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
#include <ctk/ctk.h>

/* This is exactly the style information you've been looking for */
#define CTK_STYLE_PROVIDER_PRIORITY_FORCE G_MAXUINT

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
show_parsing_error (CtkCssProvider *provider,
                    CtkCssSection  *section,
                    const GError   *error,
                    CtkTextBuffer  *buffer)
{
  CtkTextIter start, end;
  const char *tag_name;

  ctk_text_buffer_get_iter_at_line_index (buffer,
                                          &start,
                                          ctk_css_section_get_start_line (section),
                                          ctk_css_section_get_start_position (section));
  ctk_text_buffer_get_iter_at_line_index (buffer,
                                          &end,
                                          ctk_css_section_get_end_line (section),
                                          ctk_css_section_get_end_position (section));

  if (g_error_matches (error, CTK_CSS_PROVIDER_ERROR, CTK_CSS_PROVIDER_ERROR_DEPRECATED))
    tag_name = "warning";
  else
    tag_name = "error";

  ctk_text_buffer_apply_tag_by_name (buffer, tag_name, &start, &end);
}
                    
static void
css_text_changed (CtkTextBuffer  *buffer,
                  CtkCssProvider *provider)
{
  CtkTextIter start, end;
  char *text;

  ctk_text_buffer_get_start_iter (buffer, &start);
  ctk_text_buffer_get_end_iter (buffer, &end);
  ctk_text_buffer_remove_all_tags (buffer, &start, &end);

  text = ctk_text_buffer_get_text (buffer, &start, &end, FALSE);
  ctk_css_provider_load_from_data (provider, text, -1, NULL);
  g_free (text);

  ctk_style_context_reset_widgets (cdk_screen_get_default ());
}

static void
remove_widget (CtkWidget *widget)
{
  ctk_container_remove (CTK_CONTAINER (ctk_widget_get_parent (widget)), widget);
}

static int count = 0;

static void
add_button (CtkBox *box)
{
  CtkWidget* button;
  char *text;

  text = g_strdup_printf ("Remove %d", ++count);
  button = (CtkWidget *)ctk_button_new_with_label (text);
  g_free (text);
  ctk_style_context_add_class (ctk_widget_get_style_context (button), "play");
  g_signal_connect_swapped (button,
                            "clicked",
                            G_CALLBACK (remove_widget),
                            button);
  ctk_widget_show (button);
  ctk_container_add (CTK_CONTAINER (box), button);
}

static void
add_toolbutton (CtkToolbar *toolbar)
{
  CtkWidget* button;
  char *text;

  text = g_strdup_printf ("Remove %d", ++count);
  button = (CtkWidget *)ctk_tool_button_new (NULL, text);
  g_free (text);
  ctk_style_context_add_class (ctk_widget_get_style_context (button), "play");
  g_signal_connect_swapped (button,
                            "clicked",
                            G_CALLBACK (remove_widget),
                            button);
  ctk_widget_show (button);
  ctk_container_add (CTK_CONTAINER (toolbar), button);
}

static void
set_orientation (CtkSwitch *switch_)
{
  ctk_widget_set_default_direction (ctk_switch_get_active (switch_) ? CTK_TEXT_DIR_LTR : CTK_TEXT_DIR_RTL);
}

gint
main (gint argc, gchar **argv)
{
  CtkWidget *window, *main_box, *container, *child;
  CtkWidget *box, *toolbar;
  CtkStyleProvider *provider;
  CtkTextBuffer *css;
  
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

  provider = CTK_STYLE_PROVIDER (ctk_css_provider_new ());
  ctk_style_context_add_provider_for_screen (cdk_screen_get_default (),
                                             provider,
                                             CTK_STYLE_PROVIDER_PRIORITY_FORCE);
  
  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);

  g_signal_connect (window, "destroy", G_CALLBACK(ctk_main_quit), NULL);
  g_signal_connect (window, "delete_event", G_CALLBACK (ctk_main_quit), NULL);

  main_box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_container_add (CTK_CONTAINER (window), main_box);

  toolbar = ctk_toolbar_new ();
  ctk_toolbar_set_style (CTK_TOOLBAR (toolbar), CTK_TOOLBAR_TEXT);
  ctk_box_pack_start (CTK_BOX (main_box), toolbar, FALSE, TRUE, 0);

  box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_box_pack_start (CTK_BOX (main_box), box, FALSE, TRUE, 0);

  container = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_min_content_width (CTK_SCROLLED_WINDOW (container), 200);
  ctk_scrolled_window_set_min_content_height (CTK_SCROLLED_WINDOW (container), 200);
  ctk_box_pack_start (CTK_BOX (main_box), container, TRUE, TRUE, 0);
  child = ctk_text_view_new_with_buffer (css);
  ctk_container_add (CTK_CONTAINER (container), child);
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
                    ctk_text_view_get_buffer (CTK_TEXT_VIEW (child)));

  container = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_box_pack_start (CTK_BOX (main_box), container, FALSE, TRUE, 0);
  child = ctk_switch_new ();
  ctk_switch_set_active (CTK_SWITCH (child), ctk_widget_get_default_direction () == CTK_TEXT_DIR_LTR);
  g_signal_connect (child,
                    "notify::active",
                    G_CALLBACK (set_orientation),
                    NULL);
  ctk_box_pack_start (CTK_BOX (container), child, FALSE, FALSE, 0);
  child = ctk_label_new ("left-to-right");
  ctk_box_pack_start (CTK_BOX (container), child, FALSE, FALSE, 0);
  child = ctk_button_new_with_label ("Add button");
  g_signal_connect_swapped (child,
                            "clicked",
                            G_CALLBACK (add_button),
                            box);
  ctk_box_pack_end (CTK_BOX (container), child, FALSE, FALSE, 0);
  child = ctk_button_new_with_label ("Add toolbutton");
  g_signal_connect_swapped (child,
                            "clicked",
                            G_CALLBACK (add_toolbutton),
                            toolbar);
  ctk_box_pack_end (CTK_BOX (container), child, FALSE, FALSE, 0);

  add_toolbutton (CTK_TOOLBAR (toolbar));
  add_toolbutton (CTK_TOOLBAR (toolbar));
  add_toolbutton (CTK_TOOLBAR (toolbar));
  add_toolbutton (CTK_TOOLBAR (toolbar));

  add_button (CTK_BOX (box));
  add_button (CTK_BOX (box));
  add_button (CTK_BOX (box));
  add_button (CTK_BOX (box));

  ctk_widget_show_all (window);

  ctk_main ();
  
  return 0;
}
