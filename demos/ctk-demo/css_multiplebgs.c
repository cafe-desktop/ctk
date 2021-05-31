/* Theming/Multiple Backgrounds
 *
 * Gtk themes are written using CSS. Every widget is build of multiple items
 * that you can style very similarly to a regular website.
 *
 */

#include <ctk/ctk.h>

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

  if (g_error_matches (error, CTK_CSS_PROVIDER_ERROR, CTK_CSS_PROVIDER_ERROR_DEPRECATED))
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

static gboolean
drawing_area_draw (GtkWidget *widget,
                   cairo_t *cr)
{
  GtkStyleContext *context = ctk_widget_get_style_context (widget);

  ctk_render_background (context, cr,
                         0, 0,
                         ctk_widget_get_allocated_width (widget),
                         ctk_widget_get_allocated_height (widget));
  ctk_render_frame (context, cr,
                    0, 0,
                    ctk_widget_get_allocated_width (widget),
                    ctk_widget_get_allocated_height (widget));

  return FALSE;
}

static void
apply_css (GtkWidget *widget, GtkStyleProvider *provider)
{
  ctk_style_context_add_provider (ctk_widget_get_style_context (widget), provider, G_MAXUINT);
  if (CTK_IS_CONTAINER (widget))
    ctk_container_forall (CTK_CONTAINER (widget), (GtkCallback) apply_css, provider);
}

GtkWidget *
do_css_multiplebgs (GtkWidget *do_widget)
{
  static GtkWidget *window = NULL;

  if (!window)
    {
      GtkWidget *paned, *container, *child;
      GtkStyleProvider *provider;
      GtkTextBuffer *text;
      GBytes *bytes;

      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_title (CTK_WINDOW (window), "Multiple Backgrounds");
      ctk_window_set_transient_for (CTK_WINDOW (window), CTK_WINDOW (do_widget));
      ctk_window_set_default_size (CTK_WINDOW (window), 400, 300);
      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);

      container = ctk_overlay_new ();
      ctk_widget_add_events (container,
                             GDK_ENTER_NOTIFY_MASK |
                             GDK_LEAVE_NOTIFY_MASK |
                             GDK_POINTER_MOTION_MASK);
      ctk_container_add (CTK_CONTAINER (window), container);

      child = ctk_drawing_area_new ();
      ctk_widget_set_name (child, "canvas");
      g_signal_connect (child, "draw",
                        G_CALLBACK (drawing_area_draw), NULL);
      ctk_container_add (CTK_CONTAINER (container), child);

      child = ctk_button_new ();
      ctk_widget_add_events (child,
                             GDK_ENTER_NOTIFY_MASK |
                             GDK_LEAVE_NOTIFY_MASK |
                             GDK_POINTER_MOTION_MASK);
      ctk_overlay_add_overlay (CTK_OVERLAY (container), child);
      ctk_widget_set_name (child, "bricks-button");
      ctk_widget_set_halign (child, CTK_ALIGN_CENTER);
      ctk_widget_set_valign (child, CTK_ALIGN_CENTER);
      ctk_widget_set_size_request (child, 250, 84);

      paned = ctk_paned_new (CTK_ORIENTATION_VERTICAL);
      ctk_overlay_add_overlay (CTK_OVERLAY (container), paned);

      /* Need a filler so we get a handle */
      child = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_container_add (CTK_CONTAINER (paned), child);

      text = ctk_text_buffer_new (NULL);
      ctk_text_buffer_create_tag (text,
                                  "warning",
                                  "underline", PANGO_UNDERLINE_SINGLE,
                                  NULL);
      ctk_text_buffer_create_tag (text,
                                  "error",
                                  "underline", PANGO_UNDERLINE_ERROR,
                                  NULL);

      provider = CTK_STYLE_PROVIDER (ctk_css_provider_new ());

      container = ctk_scrolled_window_new (NULL, NULL);
      ctk_container_add (CTK_CONTAINER (paned), container);
      child = ctk_text_view_new_with_buffer (text);
      ctk_container_add (CTK_CONTAINER (container), child);
      g_signal_connect (text,
                        "changed",
                        G_CALLBACK (css_text_changed),
                        provider);

      bytes = g_resources_lookup_data ("/css_multiplebgs/css_multiplebgs.css", 0, NULL);
      ctk_text_buffer_set_text (text, g_bytes_get_data (bytes, NULL), g_bytes_get_size (bytes));
      g_bytes_unref (bytes);

      g_signal_connect (provider,
                        "parsing-error",
                        G_CALLBACK (show_parsing_error),
                        ctk_text_view_get_buffer (CTK_TEXT_VIEW (child)));

      apply_css (window, provider);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);

  return window;
}
