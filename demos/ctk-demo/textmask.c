/* Pango/Text Mask
 *
 * This demo shows how to use PangoCairo to draw text with more than
 * just a single color.
 */

#include <glib/gi18n.h>
#include <ctk/ctk.h>

static gboolean
draw_text (GtkWidget *da,
           cairo_t   *cr,
           gpointer   data)
{
  cairo_pattern_t *pattern;
  PangoLayout *layout;
  PangoFontDescription *desc;

  cairo_save (cr);

  layout = ctk_widget_create_pango_layout (da, "Pango power!\nPango power!\nPango power!");
  desc = pango_font_description_from_string ("sans bold 34");
  pango_layout_set_font_description (layout, desc);
  pango_font_description_free (desc);

  cairo_move_to (cr, 30, 20);
  pango_cairo_layout_path (cr, layout);
  g_object_unref (layout);

  pattern = cairo_pattern_create_linear (0.0, 0.0,
                                         ctk_widget_get_allocated_width (da),
                                         ctk_widget_get_allocated_height (da));
  cairo_pattern_add_color_stop_rgb (pattern, 0.0, 1.0, 0.0, 0.0);
  cairo_pattern_add_color_stop_rgb (pattern, 0.2, 1.0, 0.0, 0.0);
  cairo_pattern_add_color_stop_rgb (pattern, 0.3, 1.0, 1.0, 0.0);
  cairo_pattern_add_color_stop_rgb (pattern, 0.4, 0.0, 1.0, 0.0);
  cairo_pattern_add_color_stop_rgb (pattern, 0.6, 0.0, 1.0, 1.0);
  cairo_pattern_add_color_stop_rgb (pattern, 0.7, 0.0, 0.0, 1.0);
  cairo_pattern_add_color_stop_rgb (pattern, 0.8, 1.0, 0.0, 1.0);
  cairo_pattern_add_color_stop_rgb (pattern, 1.0, 1.0, 0.0, 1.0);

  cairo_set_source (cr, pattern);
  cairo_fill_preserve (cr);

  cairo_pattern_destroy (pattern);

  cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
  cairo_set_line_width (cr, 0.5);
  cairo_stroke (cr);

  cairo_restore (cr);

  return TRUE;
}

GtkWidget *
do_textmask (GtkWidget *do_widget)
{
  static GtkWidget *window = NULL;
  static GtkWidget *da;

  if (!window)
    {
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_resizable (CTK_WINDOW (window), TRUE);
      ctk_widget_set_size_request (window, 400, 200);
      ctk_window_set_title (CTK_WINDOW (window), "Text Mask");

      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);

      da = ctk_drawing_area_new ();

      ctk_container_add (CTK_CONTAINER (window), da);
      g_signal_connect (da, "draw",
                        G_CALLBACK (draw_text), NULL);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);

  return window;
}
