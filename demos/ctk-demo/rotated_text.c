/* Pango/Rotated Text
 *
 * This demo shows how to use PangoCairo to draw rotated and transformed
 * text.  The right pane shows a rotated CtkLabel widget.
 *
 * In both cases, a custom PangoCairo shape renderer is installed to draw
 * a red heard using cairo drawing operations instead of the Unicode heart
 * character.
 */

#include <ctk/ctk.h>
#include <string.h>

#define HEART "♥"
const char text[] = "I ♥ CTK+";

static void
fancy_shape_renderer (cairo_t        *cr,
                      PangoAttrShape *attr,
                      gboolean        do_path,
                      gpointer        data G_GNUC_UNUSED)
{
  double x, y;
  cairo_get_current_point (cr, &x, &y);
  cairo_translate (cr, x, y);

  cairo_scale (cr,
               (double) attr->ink_rect.width  / PANGO_SCALE,
               (double) attr->ink_rect.height / PANGO_SCALE);

  switch (GPOINTER_TO_UINT (attr->data))
    {
    case 0x2665: /* U+2665 BLACK HEART SUIT */
      {
        cairo_move_to (cr, .5, .0);
        cairo_line_to (cr, .9, -.4);
        cairo_curve_to (cr, 1.1, -.8, .5, -.9, .5, -.5);
        cairo_curve_to (cr, .5, -.9, -.1, -.8, .1, -.4);
        cairo_close_path (cr);
      }
      break;
    }

  if (!do_path) {
    cairo_set_source_rgb (cr, 1., 0., 0.);
    cairo_fill (cr);
  }
}

PangoAttrList *
create_fancy_attr_list_for_layout (PangoLayout *layout)
{
  PangoAttrList *attrs;
  PangoFontMetrics *metrics;
  int ascent;
  PangoRectangle ink_rect, logical_rect;
  const char *p;

  /* Get font metrics and prepare fancy shape size */
  metrics = pango_context_get_metrics (pango_layout_get_context (layout),
                                       pango_layout_get_font_description (layout),
                                       NULL);
  ascent = pango_font_metrics_get_ascent (metrics);
  logical_rect.x = 0;
  logical_rect.width = ascent;
  logical_rect.y = -ascent;
  logical_rect.height = ascent;
  ink_rect = logical_rect;
  pango_font_metrics_unref (metrics);

  /* Set fancy shape attributes for all hearts */
  attrs = pango_attr_list_new ();
  for (p = text; (p = strstr (p, HEART)); p += strlen (HEART))
    {
      PangoAttribute *attr;

      attr = pango_attr_shape_new_with_data (&ink_rect,
                                             &logical_rect,
                                             GUINT_TO_POINTER (g_utf8_get_char (p)),
                                             NULL, NULL);

      attr->start_index = p - text;
      attr->end_index = attr->start_index + strlen (HEART);

      pango_attr_list_insert (attrs, attr);
    }

  return attrs;
}

static gboolean
rotated_text_draw (CtkWidget *widget,
                   cairo_t   *cr,
                   gpointer   data G_GNUC_UNUSED)
{
#define RADIUS 150
#define N_WORDS 5
#define FONT "Serif 18"

  PangoContext *context;
  PangoLayout *layout;
  PangoFontDescription *desc;

  cairo_pattern_t *pattern;

  PangoAttrList *attrs;

  double device_radius;
  int width, height;
  int i;

  /* Create a cairo context and set up a transformation matrix so that the user
   * space coordinates for the centered square where we draw are [-RADIUS, RADIUS],
   * [-RADIUS, RADIUS].
   * We first center, then change the scale. */
  width = ctk_widget_get_allocated_width (widget);
  height = ctk_widget_get_allocated_height (widget);
  device_radius = MIN (width, height) / 2.;
  cairo_translate (cr,
                   device_radius + (width - 2 * device_radius) / 2,
                   device_radius + (height - 2 * device_radius) / 2);
  cairo_scale (cr, device_radius / RADIUS, device_radius / RADIUS);

  /* Create and a subtle gradient source and use it. */
  pattern = cairo_pattern_create_linear (-RADIUS, -RADIUS, RADIUS, RADIUS);
  cairo_pattern_add_color_stop_rgb (pattern, 0., .5, .0, .0);
  cairo_pattern_add_color_stop_rgb (pattern, 1., .0, .0, .5);
  cairo_set_source (cr, pattern);

  /* Create a PangoContext and set up our shape renderer */
  context = ctk_widget_create_pango_context (widget);
  pango_cairo_context_set_shape_renderer (context,
                                          fancy_shape_renderer,
                                          NULL, NULL);

  /* Create a PangoLayout, set the text, font, and attributes */
  layout = pango_layout_new (context);
  pango_layout_set_text (layout, text, -1);
  desc = pango_font_description_from_string (FONT);
  pango_layout_set_font_description (layout, desc);

  attrs = create_fancy_attr_list_for_layout (layout);
  pango_layout_set_attributes (layout, attrs);
  pango_attr_list_unref (attrs);

  /* Draw the layout N_WORDS times in a circle */
  for (i = 0; i < N_WORDS; i++)
    {
      int width, height;

      /* Inform Pango to re-layout the text with the new transformation matrix */
      pango_cairo_update_layout (cr, layout);

      pango_layout_get_pixel_size (layout, &width, &height);
      cairo_move_to (cr, - width / 2, - RADIUS * .9);
      pango_cairo_show_layout (cr, layout);

      /* Rotate for the next turn */
      cairo_rotate (cr, G_PI*2 / N_WORDS);
    }

  /* free the objects we created */
  pango_font_description_free (desc);
  g_object_unref (layout);
  g_object_unref (context);
  cairo_pattern_destroy (pattern);

  return FALSE;
}

CtkWidget *
do_rotated_text (CtkWidget *do_widget)
{
  static CtkWidget *window = NULL;

  if (!window)
    {
      CtkWidget *box;
      CtkWidget *drawing_area;
      CtkWidget *label;
      PangoLayout *layout;
      PangoAttrList *attrs;

      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window),
                             ctk_widget_get_screen (do_widget));
      ctk_window_set_title (CTK_WINDOW (window), "Rotated Text");
      ctk_window_set_default_size (CTK_WINDOW (window), 4 * RADIUS, 2 * RADIUS);
      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);

      box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
      ctk_box_set_homogeneous (CTK_BOX (box), TRUE);
      ctk_container_add (CTK_CONTAINER (window), box);

      /* Add a drawing area */
      drawing_area = ctk_drawing_area_new ();
      ctk_container_add (CTK_CONTAINER (box), drawing_area);
      ctk_style_context_add_class (ctk_widget_get_style_context (drawing_area),
                                   CTK_STYLE_CLASS_VIEW);

      g_signal_connect (drawing_area, "draw",
                        G_CALLBACK (rotated_text_draw), NULL);

      /* And a label */
      label = ctk_label_new (text);
      ctk_container_add (CTK_CONTAINER (box), label);

      ctk_label_set_angle (CTK_LABEL (label), 45);

      /* Set up fancy stuff on the label */
      layout = ctk_label_get_layout (CTK_LABEL (label));
      pango_cairo_context_set_shape_renderer (pango_layout_get_context (layout),
                                              fancy_shape_renderer,
                                              NULL, NULL);
      attrs = create_fancy_attr_list_for_layout (layout);
      ctk_label_set_attributes (CTK_LABEL (label), attrs);
      pango_attr_list_unref (attrs);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);

  return window;
}
