/* Overlay/Decorative Overlay
 *
 * Another example of an overlay with some decorative
 * and some interactive controls.
 */

#include <gtk/gtk.h>

static GtkTextTag *tag;

static void
margin_changed (GtkAdjustment *adjustment,
                GtkTextView   *text)
{
  gint value;

  value = (gint)ctk_adjustment_get_value (adjustment);
  ctk_text_view_set_left_margin (GTK_TEXT_VIEW (text), value);
  g_object_set (tag, "pixels-above-lines", value, NULL);
}

GtkWidget *
do_overlay2 (GtkWidget *do_widget)
{
  static GtkWidget *window = NULL;

  if (!window)
    {
      GtkWidget *overlay;
      GtkWidget *sw;
      GtkWidget *text;
      GtkWidget *image;
      GtkWidget *scale;
      GtkTextBuffer *buffer;
      GtkTextIter start, end;
      GtkAdjustment *adjustment;

      window = ctk_window_new (GTK_WINDOW_TOPLEVEL);
      ctk_window_set_default_size (GTK_WINDOW (window), 500, 510);
      ctk_window_set_title (GTK_WINDOW (window), "Decorative Overlay");

      overlay = ctk_overlay_new ();
      sw = ctk_scrolled_window_new (NULL, NULL);
      ctk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
                                      GTK_POLICY_AUTOMATIC,
                                      GTK_POLICY_AUTOMATIC);
      text = ctk_text_view_new ();
      buffer = ctk_text_view_get_buffer (GTK_TEXT_VIEW (text));

      ctk_text_buffer_set_text (buffer, "Dear diary...", -1);

      tag = ctk_text_buffer_create_tag (buffer, "top-margin",
                                        "pixels-above-lines", 0,
                                        NULL);
      ctk_text_buffer_get_start_iter (buffer, &start);
      end = start;
      ctk_text_iter_forward_word_end (&end);
      ctk_text_buffer_apply_tag (buffer, tag, &start, &end);

      ctk_container_add (GTK_CONTAINER (window), overlay);
      ctk_container_add (GTK_CONTAINER (overlay), sw);
      ctk_container_add (GTK_CONTAINER (sw), text);

      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);

      image = ctk_image_new_from_resource ("/overlay2/decor1.png");
      ctk_overlay_add_overlay (GTK_OVERLAY (overlay), image);
      ctk_overlay_set_overlay_pass_through (GTK_OVERLAY (overlay), image, TRUE);
      ctk_widget_set_halign (image, GTK_ALIGN_START);
      ctk_widget_set_valign (image, GTK_ALIGN_START);

      image = ctk_image_new_from_resource ("/overlay2/decor2.png");
      ctk_overlay_add_overlay (GTK_OVERLAY (overlay), image);
      ctk_overlay_set_overlay_pass_through (GTK_OVERLAY (overlay), image, TRUE);
      ctk_widget_set_halign (image, GTK_ALIGN_END);
      ctk_widget_set_valign (image, GTK_ALIGN_END);

      adjustment = ctk_adjustment_new (0, 0, 100, 1, 1, 0);
      g_signal_connect (adjustment, "value-changed",
                        G_CALLBACK (margin_changed), text);

      scale = ctk_scale_new (GTK_ORIENTATION_HORIZONTAL, adjustment);
      ctk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
      ctk_widget_set_size_request (scale, 120, -1);
      ctk_widget_set_margin_start (scale, 20);
      ctk_widget_set_margin_end (scale, 20);
      ctk_widget_set_margin_bottom (scale, 20);
      ctk_overlay_add_overlay (GTK_OVERLAY (overlay), scale);
      ctk_widget_set_halign (scale, GTK_ALIGN_START);
      ctk_widget_set_valign (scale, GTK_ALIGN_END);
      ctk_widget_set_tooltip_text (scale, "Margin");

      ctk_adjustment_set_value (adjustment, 100);

      ctk_widget_show_all (overlay);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show (window);
  else
    ctk_widget_destroy (window);

  return window;
}
