

#include "config.h"
#include <ctk/ctk.h>

typedef struct {
  CtkTextView parent;
} MyTextView;

typedef struct {
  CtkTextViewClass parent_class;
} MyTextViewClass;

G_DEFINE_TYPE (MyTextView, my_text_view, CTK_TYPE_TEXT_VIEW);

static void draw_background (CtkWidget *widget, cairo_t *cr);

static void
my_text_view_init (MyTextView *text_view G_GNUC_UNUSED)
{
}

static void my_text_view_draw_layer (CtkTextView       *textview,
				     CtkTextViewLayer   layer,
				     cairo_t           *cr)
{
  if (layer == CTK_TEXT_VIEW_LAYER_BELOW)
    draw_background (CTK_WIDGET (textview), cr);
}

static void
my_text_view_class_init (MyTextViewClass *klass)
{
  CTK_TEXT_VIEW_CLASS (klass)->draw_layer = my_text_view_draw_layer;
}

static void
create_tags (CtkTextBuffer *buffer)
{

  ctk_text_buffer_create_tag (buffer, "italic",
                              "style", PANGO_STYLE_ITALIC, NULL);

  ctk_text_buffer_create_tag (buffer, "bold",
                              "weight", PANGO_WEIGHT_BOLD, NULL);

  ctk_text_buffer_create_tag (buffer, "x-large",
                              "scale", PANGO_SCALE_X_LARGE, NULL);

  ctk_text_buffer_create_tag (buffer, "semi_blue_foreground",
                              "foreground", "rgba(0,0,255,0.7)", NULL);

  ctk_text_buffer_create_tag (buffer, "semi_red_background",
                              "background", "rgba(255,0,0,0.5)", NULL);

  ctk_text_buffer_create_tag (buffer, "semi_orange_paragraph_background",
                              "paragraph-background", "rgba(255,165,0,0.5)", NULL);

  ctk_text_buffer_create_tag (buffer, "word_wrap",
                              "wrap_mode", CTK_WRAP_WORD, NULL);
}


static CtkTextChildAnchor *
insert_text (CtkTextBuffer *buffer)
{
  CtkTextIter  iter;
  CtkTextIter  start, end;
  CtkTextMark *para_start;
  CtkTextChildAnchor *anchor;

  /* get start of buffer; each insertion will revalidate the
   * iterator to point to just after the inserted text.
   */
  ctk_text_buffer_get_iter_at_offset (buffer, &iter, 0);

  ctk_text_buffer_insert (buffer, &iter,
      "This test shows text view rendering some text with rgba colors.\n\n", -1);

  ctk_text_buffer_insert (buffer, &iter, "For example, you can have ", -1);
  ctk_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "italic translucent blue text", -1,
                                            "italic", 
					    "semi_blue_foreground",
					    "x-large",
					    NULL);

  ctk_text_buffer_insert (buffer, &iter, ", or ", -1);

  ctk_text_buffer_insert (buffer, &iter, ", ", -1);
  ctk_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "bold text with translucent red background", -1,
                                            "bold", 
					    "semi_red_background",
					    "x-large",
					    NULL);
  ctk_text_buffer_insert (buffer, &iter, ".\n\n", -1);

  anchor = ctk_text_buffer_create_child_anchor (buffer, &iter);

  /* Store the beginning of the other paragraph */
  para_start = ctk_text_buffer_create_mark (buffer, "para_start", &iter, TRUE);

  ctk_text_buffer_insert (buffer, &iter,
      "Paragraph background colors can also be set with rgba color values .\n", -1);

  ctk_text_buffer_insert (buffer, &iter, "For instance, you can have ", -1);
  ctk_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "bold translucent blue text", -1,
                                            "bold", 
					    "semi_blue_foreground",
					    "x-large",
					    NULL);

  ctk_text_buffer_insert (buffer, &iter, ", or ", -1);

  ctk_text_buffer_insert (buffer, &iter, ", ", -1);
  ctk_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "italic text with translucent red background", -1,
                                            "italic", 
					    "semi_red_background",
					    "x-large",
					    NULL);

  ctk_text_buffer_insert (buffer, &iter, " all rendered onto a translucent orange paragraph background.\n", -1);

  ctk_text_buffer_get_bounds (buffer, &start, &end);

  ctk_text_buffer_get_iter_at_mark (buffer, &iter, para_start);
  ctk_text_buffer_apply_tag_by_name (buffer, "semi_orange_paragraph_background", &iter, &end);

  /* Apply word_wrap tag to whole buffer */
  ctk_text_buffer_get_bounds (buffer, &start, &end);
  ctk_text_buffer_apply_tag_by_name (buffer, "word_wrap", &start, &end);

  return anchor;
}


/* Size of checks and gray levels for alpha compositing checkerboard */
#define CHECK_SIZE  10
#define CHECK_DARK  (1.0 / 3.0)
#define CHECK_LIGHT (2.0 / 3.0)

static cairo_pattern_t *
get_checkered (void)
{
  static cairo_surface_t *checkered = NULL;
  cairo_pattern_t *pattern;

  if (checkered == NULL)
    {
      /* need to respect pixman's stride being a multiple of 4 */
      static unsigned char data[8] = { 0xFF, 0x00, 0x00, 0x00,
                                       0x00, 0xFF, 0x00, 0x00 };

      checkered = cairo_image_surface_create_for_data (data,
                                                       CAIRO_FORMAT_A8,
                                                       2, 2, 4);
    }

  pattern = cairo_pattern_create_for_surface (checkered);
  cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);
  cairo_pattern_set_filter (pattern, CAIRO_FILTER_NEAREST);

  return pattern;
}

static void
draw_background (CtkWidget *widget, cairo_t *cr)
{
  CdkRectangle visible_rect;
  cairo_pattern_t *pat;

  cairo_save (cr);

  ctk_text_view_get_visible_rect (CTK_TEXT_VIEW (widget), &visible_rect);
  cairo_translate (cr, -visible_rect.x, -visible_rect.y);

  cairo_set_source_rgb (cr, CHECK_DARK, CHECK_DARK, CHECK_DARK);
  cairo_paint (cr);

  cairo_set_source_rgb (cr, CHECK_LIGHT, CHECK_LIGHT, CHECK_LIGHT);
  cairo_scale (cr, CHECK_SIZE, CHECK_SIZE);

  pat = get_checkered ();
  cairo_mask (cr, pat);
  cairo_pattern_destroy (pat);

  cairo_restore (cr);
}

int
main (int argc, char **argv)
{
  CtkWidget *window, *textview, *sw, *button, *button2;
  CtkTextBuffer *buffer;
  CtkTextChildAnchor *anchor;

  ctk_init (&argc, &argv);

  window   = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  sw       = ctk_scrolled_window_new (NULL, NULL);
  textview = g_object_new (my_text_view_get_type (), NULL);
  buffer   = ctk_text_view_get_buffer (CTK_TEXT_VIEW (textview));
  button   = ctk_button_new_with_label ("Fixed Child");
  button2   = ctk_button_new_with_label ("Flowed Child");

  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (sw),
                                  CTK_POLICY_AUTOMATIC,
                                  CTK_POLICY_AUTOMATIC);

  ctk_window_set_default_size (CTK_WINDOW (window), 400, 400);

  create_tags (buffer);
  anchor = insert_text (buffer);

  ctk_widget_show (button);
  ctk_widget_show (button2);
  ctk_widget_show (textview);
  ctk_widget_show (sw);

  ctk_container_add (CTK_CONTAINER (window), sw);
  ctk_container_add (CTK_CONTAINER (sw), textview);
  ctk_text_view_add_child_in_window (CTK_TEXT_VIEW (textview),
                                     button,
                                     CTK_TEXT_WINDOW_TEXT,
                                     50, 150);

  ctk_text_view_add_child_at_anchor (CTK_TEXT_VIEW (textview),
                                     button2, anchor);

  ctk_widget_show (window);

  ctk_main ();

  return 0;
}
