/* Text View/Multiple Views
 *
 * The CtkTextView widget displays a CtkTextBuffer. One CtkTextBuffer
 * can be displayed by multiple CtkTextViews. This demo has two views
 * displaying a single buffer, and shows off the widget's text
 * formatting features.
 *
 */

#include <ctk/ctk.h>
#include <stdlib.h> /* for exit() */

static void easter_egg_callback (CtkWidget *button, gpointer data);

static void
create_tags (CtkTextBuffer *buffer)
{
  /* Create a bunch of tags. Note that it's also possible to
   * create tags with ctk_text_tag_new() then add them to the
   * tag table for the buffer, ctk_text_buffer_create_tag() is
   * just a convenience function. Also note that you don't have
   * to give tags a name; pass NULL for the name to create an
   * anonymous tag.
   *
   * In any real app, another useful optimization would be to create
   * a CtkTextTagTable in advance, and reuse the same tag table for
   * all the buffers with the same tag set, instead of creating
   * new copies of the same tags for every buffer.
   *
   * Tags are assigned default priorities in order of addition to the
   * tag table.  That is, tags created later that affect the same text
   * property affected by an earlier tag will override the earlier
   * tag.  You can modify tag priorities with
   * ctk_text_tag_set_priority().
   */

  ctk_text_buffer_create_tag (buffer, "heading",
                              "weight", PANGO_WEIGHT_BOLD,
                              "size", 15 * PANGO_SCALE,
                              NULL);

  ctk_text_buffer_create_tag (buffer, "italic",
                              "style", PANGO_STYLE_ITALIC, NULL);

  ctk_text_buffer_create_tag (buffer, "bold",
                              "weight", PANGO_WEIGHT_BOLD, NULL);

  ctk_text_buffer_create_tag (buffer, "big",
                              /* points times the PANGO_SCALE factor */
                              "size", 20 * PANGO_SCALE, NULL);

  ctk_text_buffer_create_tag (buffer, "xx-small",
                              "scale", PANGO_SCALE_XX_SMALL, NULL);

  ctk_text_buffer_create_tag (buffer, "x-large",
                              "scale", PANGO_SCALE_X_LARGE, NULL);

  ctk_text_buffer_create_tag (buffer, "monospace",
                              "family", "monospace", NULL);

  ctk_text_buffer_create_tag (buffer, "blue_foreground",
                              "foreground", "blue", NULL);

  ctk_text_buffer_create_tag (buffer, "red_background",
                              "background", "red", NULL);

  ctk_text_buffer_create_tag (buffer, "big_gap_before_line",
                              "pixels_above_lines", 30, NULL);

  ctk_text_buffer_create_tag (buffer, "big_gap_after_line",
                              "pixels_below_lines", 30, NULL);

  ctk_text_buffer_create_tag (buffer, "double_spaced_line",
                              "pixels_inside_wrap", 10, NULL);

  ctk_text_buffer_create_tag (buffer, "not_editable",
                              "editable", FALSE, NULL);

  ctk_text_buffer_create_tag (buffer, "word_wrap",
                              "wrap_mode", CTK_WRAP_WORD, NULL);

  ctk_text_buffer_create_tag (buffer, "char_wrap",
                              "wrap_mode", CTK_WRAP_CHAR, NULL);

  ctk_text_buffer_create_tag (buffer, "no_wrap",
                              "wrap_mode", CTK_WRAP_NONE, NULL);

  ctk_text_buffer_create_tag (buffer, "center",
                              "justification", CTK_JUSTIFY_CENTER, NULL);

  ctk_text_buffer_create_tag (buffer, "right_justify",
                              "justification", CTK_JUSTIFY_RIGHT, NULL);

  ctk_text_buffer_create_tag (buffer, "wide_margins",
                              "left_margin", 50, "right_margin", 50,
                              NULL);

  ctk_text_buffer_create_tag (buffer, "strikethrough",
                              "strikethrough", TRUE, NULL);

  ctk_text_buffer_create_tag (buffer, "underline",
                              "underline", PANGO_UNDERLINE_SINGLE, NULL);

  ctk_text_buffer_create_tag (buffer, "double_underline",
                              "underline", PANGO_UNDERLINE_DOUBLE, NULL);

  ctk_text_buffer_create_tag (buffer, "superscript",
                              "rise", 10 * PANGO_SCALE,   /* 10 pixels */
                              "size", 8 * PANGO_SCALE,    /* 8 points */
                              NULL);

  ctk_text_buffer_create_tag (buffer, "subscript",
                              "rise", -10 * PANGO_SCALE,   /* 10 pixels */
                              "size", 8 * PANGO_SCALE,     /* 8 points */
                              NULL);

  ctk_text_buffer_create_tag (buffer, "rtl_quote",
                              "wrap_mode", CTK_WRAP_WORD,
                              "direction", CTK_TEXT_DIR_RTL,
                              "indent", 30,
                              "left_margin", 20,
                              "right_margin", 20,
                              NULL);
}

static void
insert_text (CtkTextBuffer *buffer)
{
  CtkTextIter iter;
  CtkTextIter start, end;
  GdkPixbuf *pixbuf;
  CtkIconTheme *icon_theme;

  icon_theme = ctk_icon_theme_get_default ();
  pixbuf = ctk_icon_theme_load_icon (icon_theme,
                                     "ctk3-demo",
                                     32,
                                     CTK_ICON_LOOKUP_GENERIC_FALLBACK,
                                     NULL);
  g_assert (pixbuf);

  /* get start of buffer; each insertion will revalidate the
   * iterator to point to just after the inserted text.
   */
  ctk_text_buffer_get_iter_at_offset (buffer, &iter, 0);

  ctk_text_buffer_insert (buffer, &iter,
      "The text widget can display text with all kinds of nifty attributes. "
      "It also supports multiple views of the same buffer; this demo is "
      "showing the same buffer in two places.\n\n", -1);

  ctk_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "Font styles. ", -1,
                                            "heading", NULL);

  ctk_text_buffer_insert (buffer, &iter, "For example, you can have ", -1);
  ctk_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "italic", -1,
                                            "italic", NULL);
  ctk_text_buffer_insert (buffer, &iter, ", ", -1);
  ctk_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "bold", -1,
                                            "bold", NULL);
  ctk_text_buffer_insert (buffer, &iter, ", or ", -1);
  ctk_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "monospace (typewriter)", -1,
                                            "monospace", NULL);
  ctk_text_buffer_insert (buffer, &iter, ", or ", -1);
  ctk_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "big", -1,
                                            "big", NULL);
  ctk_text_buffer_insert (buffer, &iter, " text. ", -1);
  ctk_text_buffer_insert (buffer, &iter,
      "It's best not to hardcode specific text sizes; you can use relative "
      "sizes as with CSS, such as ", -1);
  ctk_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "xx-small", -1,
                                            "xx-small", NULL);
  ctk_text_buffer_insert (buffer, &iter, " or ", -1);
  ctk_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "x-large", -1,
                                            "x-large", NULL);
  ctk_text_buffer_insert (buffer, &iter,
      " to ensure that your program properly adapts if the user changes the "
      "default font size.\n\n", -1);

  ctk_text_buffer_insert_with_tags_by_name (buffer, &iter, "Colors. ", -1,
                                            "heading", NULL);

  ctk_text_buffer_insert (buffer, &iter, "Colors such as ", -1);
  ctk_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "a blue foreground", -1,
                                            "blue_foreground", NULL);
  ctk_text_buffer_insert (buffer, &iter, " or ", -1);
  ctk_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "a red background", -1,
                                            "red_background", NULL);
  ctk_text_buffer_insert (buffer, &iter, " or even ", -1);
  ctk_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "a blue foreground on red background", -1,
                                            "blue_foreground",
                                            "red_background",
                                            NULL);
  ctk_text_buffer_insert (buffer, &iter, " (select that to read it) can be used.\n\n", -1);

  ctk_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "Underline, strikethrough, and rise. ", -1,
                                            "heading", NULL);

  ctk_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "Strikethrough", -1,
                                            "strikethrough", NULL);
  ctk_text_buffer_insert (buffer, &iter, ", ", -1);
  ctk_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "underline", -1,
                                            "underline", NULL);
  ctk_text_buffer_insert (buffer, &iter, ", ", -1);
  ctk_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "double underline", -1,
                                            "double_underline", NULL);
  ctk_text_buffer_insert (buffer, &iter, ", ", -1);
  ctk_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "superscript", -1,
                                            "superscript", NULL);
  ctk_text_buffer_insert (buffer, &iter, ", and ", -1);
  ctk_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "subscript", -1,
                                            "subscript", NULL);
  ctk_text_buffer_insert (buffer, &iter, " are all supported.\n\n", -1);

  ctk_text_buffer_insert_with_tags_by_name (buffer, &iter, "Images. ", -1,
                                            "heading", NULL);

  ctk_text_buffer_insert (buffer, &iter, "The buffer can have images in it: ", -1);
  ctk_text_buffer_insert_pixbuf (buffer, &iter, pixbuf);
  ctk_text_buffer_insert_pixbuf (buffer, &iter, pixbuf);
  ctk_text_buffer_insert_pixbuf (buffer, &iter, pixbuf);
  ctk_text_buffer_insert (buffer, &iter, " for example.\n\n", -1);

  ctk_text_buffer_insert_with_tags_by_name (buffer, &iter, "Spacing. ", -1,
                                            "heading", NULL);

  ctk_text_buffer_insert (buffer, &iter,
      "You can adjust the amount of space before each line.\n", -1);

  ctk_text_buffer_insert_with_tags_by_name (buffer, &iter,
      "This line has a whole lot of space before it.\n", -1,
      "big_gap_before_line", "wide_margins", NULL);
  ctk_text_buffer_insert_with_tags_by_name (buffer, &iter,
      "You can also adjust the amount of space after each line; "
      "this line has a whole lot of space after it.\n", -1,
      "big_gap_after_line", "wide_margins", NULL);

  ctk_text_buffer_insert_with_tags_by_name (buffer, &iter,
      "You can also adjust the amount of space between wrapped lines; "
      "this line has extra space between each wrapped line in the same "
      "paragraph. To show off wrapping, some filler text: the quick "
      "brown fox jumped over the lazy dog. Blah blah blah blah blah "
      "blah blah blah blah.\n", -1,
      "double_spaced_line", "wide_margins", NULL);

  ctk_text_buffer_insert (buffer, &iter,
      "Also note that those lines have extra-wide margins.\n\n", -1);

  ctk_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "Editability. ", -1,
                                            "heading", NULL);

  ctk_text_buffer_insert_with_tags_by_name (buffer, &iter,
      "This line is 'locked down' and can't be edited by the user - just "
      "try it! You can't delete this line.\n\n", -1,
      "not_editable", NULL);

  ctk_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "Wrapping. ", -1,
                                            "heading", NULL);

  ctk_text_buffer_insert (buffer, &iter,
      "This line (and most of the others in this buffer) is word-wrapped, "
      "using the proper Unicode algorithm. Word wrap should work in all "
      "scripts and languages that GTK+ supports. Let's make this a long "
      "paragraph to demonstrate: blah blah blah blah blah blah blah blah "
      "blah blah blah blah blah blah blah blah blah blah blah\n\n", -1);

  ctk_text_buffer_insert_with_tags_by_name (buffer, &iter,
      "This line has character-based wrapping, and can wrap between any two "
      "character glyphs. Let's make this a long paragraph to demonstrate: "
      "blah blah blah blah blah blah blah blah blah blah blah blah blah blah "
      "blah blah blah blah blah\n\n", -1, "char_wrap", NULL);

  ctk_text_buffer_insert_with_tags_by_name (buffer, &iter,
      "This line has all wrapping turned off, so it makes the horizontal "
      "scrollbar appear.\n\n\n", -1, "no_wrap", NULL);

  ctk_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "Justification. ", -1,
                                            "heading", NULL);

  ctk_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "\nThis line has center justification.\n", -1,
                                            "center", NULL);

  ctk_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "This line has right justification.\n", -1,
                                            "right_justify", NULL);

  ctk_text_buffer_insert_with_tags_by_name (buffer, &iter,
      "\nThis line has big wide margins. Text text text text text text text "
      "text text text text text text text text text text text text text text "
      "text text text text text text text text text text text text text text "
      "text.\n", -1, "wide_margins", NULL);

  ctk_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "Internationalization. ", -1,
                                            "heading", NULL);

  ctk_text_buffer_insert (buffer, &iter,
      "You can put all sorts of Unicode text in the buffer.\n\nGerman "
      "(Deutsch S\303\274d) Gr\303\274\303\237 Gott\nGreek "
      "(\316\225\316\273\316\273\316\267\316\275\316\271\316\272\316\254) "
      "\316\223\316\265\316\271\316\254 \317\203\316\261\317\202\nHebrew      "
      "\327\251\327\234\327\225\327\235\nJapanese "
      "(\346\227\245\346\234\254\350\252\236)\n\nThe widget properly handles "
      "bidirectional text, word wrapping, DOS/UNIX/Unicode paragraph separators, "
      "grapheme boundaries, and so on using the Pango internationalization "
      "framework.\n", -1);

  ctk_text_buffer_insert (buffer, &iter,
      "Here's a word-wrapped quote in a right-to-left language:\n", -1);
  ctk_text_buffer_insert_with_tags_by_name (buffer, &iter,
      "\331\210\331\202\330\257 \330\250\330\257\330\243 "
      "\330\253\331\204\330\247\330\253 \331\205\331\206 "
      "\330\243\331\203\330\253\330\261 \330\247\331\204\331\205\330\244\330\263\330\263\330\247\330\252 "
      "\330\252\331\202\330\257\331\205\330\247 \331\201\331\212 "
      "\330\264\330\250\331\203\330\251 \330\247\331\203\330\263\331\212\331\210\331\206 "
      "\330\250\330\261\330\247\331\205\330\254\331\207\330\247 "
      "\331\203\331\205\331\206\330\270\331\205\330\247\330\252 "
      "\331\204\330\247 \330\252\330\263\330\271\331\211 \331\204\331\204\330\261\330\250\330\255\330\214 "
      "\330\253\331\205 \330\252\330\255\331\210\331\204\330\252 "
      "\331\201\331\212 \330\247\331\204\330\263\331\206\331\210\330\247\330\252 "
      "\330\247\331\204\330\256\331\205\330\263 \330\247\331\204\331\205\330\247\330\266\331\212\330\251 "
      "\330\245\331\204\331\211 \331\205\330\244\330\263\330\263\330\247\330\252 "
      "\331\205\330\247\331\204\331\212\330\251 \331\205\331\206\330\270\331\205\330\251\330\214 "
      "\331\210\330\250\330\247\330\252\330\252 \330\254\330\262\330\241\330\247 "
      "\331\205\331\206 \330\247\331\204\331\206\330\270\330\247\331\205 "
      "\330\247\331\204\331\205\330\247\331\204\331\212 \331\201\331\212 "
      "\330\250\331\204\330\257\330\247\331\206\331\207\330\247\330\214 "
      "\331\210\331\204\331\203\331\206\331\207\330\247 \330\252\330\252\330\256\330\265\330\265 "
      "\331\201\331\212 \330\256\330\257\331\205\330\251 \331\202\330\267\330\247\330\271 "
      "\330\247\331\204\331\205\330\264\330\261\331\210\330\271\330\247\330\252 "
      "\330\247\331\204\330\265\330\272\331\212\330\261\330\251. \331\210\330\243\330\255\330\257 "
      "\330\243\331\203\330\253\330\261 \331\207\330\260\331\207 "
      "\330\247\331\204\331\205\330\244\330\263\330\263\330\247\330\252 "
      "\331\206\330\254\330\247\330\255\330\247 \331\207\331\210 "
      "\302\273\330\250\330\247\331\206\331\203\331\210\330\263\331\210\331\204\302\253 "
      "\331\201\331\212 \330\250\331\210\331\204\331\212\331\201\331\212\330\247.\n\n", -1,
                                                "rtl_quote", NULL);

  ctk_text_buffer_insert (buffer, &iter,
                          "You can put widgets in the buffer: Here's a button: ", -1);
  ctk_text_buffer_create_child_anchor (buffer, &iter);
  ctk_text_buffer_insert (buffer, &iter, " and a menu: ", -1);
  ctk_text_buffer_create_child_anchor (buffer, &iter);
  ctk_text_buffer_insert (buffer, &iter, " and a scale: ", -1);
  ctk_text_buffer_create_child_anchor (buffer, &iter);
  ctk_text_buffer_insert (buffer, &iter, " and an animation: ", -1);
  ctk_text_buffer_create_child_anchor (buffer, &iter);
  ctk_text_buffer_insert (buffer, &iter, " finally a text entry: ", -1);
  ctk_text_buffer_create_child_anchor (buffer, &iter);
  ctk_text_buffer_insert (buffer, &iter, ".\n", -1);

  ctk_text_buffer_insert (buffer, &iter,
      "\n\nThis demo doesn't demonstrate all the CtkTextBuffer features; "
      "it leaves out, for example: invisible/hidden text, tab stops, "
      "application-drawn areas on the sides of the widget for displaying "
      "breakpoints and such...", -1);

  /* Apply word_wrap tag to whole buffer */
  ctk_text_buffer_get_bounds (buffer, &start, &end);
  ctk_text_buffer_apply_tag_by_name (buffer, "word_wrap", &start, &end);

  g_object_unref (pixbuf);
}

static gboolean
find_anchor (CtkTextIter *iter)
{
  while (ctk_text_iter_forward_char (iter))
    {
      if (ctk_text_iter_get_child_anchor (iter))
        return TRUE;
    }
  return FALSE;
}

static void
attach_widgets (CtkTextView *text_view)
{
  CtkTextIter iter;
  CtkTextBuffer *buffer;
  int i;

  buffer = ctk_text_view_get_buffer (text_view);

  ctk_text_buffer_get_start_iter (buffer, &iter);

  i = 0;
  while (find_anchor (&iter))
    {
      CtkTextChildAnchor *anchor;
      CtkWidget *widget;

      anchor = ctk_text_iter_get_child_anchor (&iter);

      if (i == 0)
        {
          widget = ctk_button_new_with_label ("Click Me");

          g_signal_connect (widget, "clicked",
                            G_CALLBACK (easter_egg_callback),
                            NULL);
        }
      else if (i == 1)
        {
          widget = ctk_combo_box_text_new ();

          ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "Option 1");
          ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "Option 2");
          ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (widget), "Option 3");
        }
      else if (i == 2)
        {
          widget = ctk_scale_new (CTK_ORIENTATION_HORIZONTAL, NULL);
          ctk_range_set_range (CTK_RANGE (widget), 0, 100);
          ctk_widget_set_size_request (widget, 70, -1);
        }
      else if (i == 3)
        {
          widget = ctk_image_new_from_resource ("/textview/floppybuddy.gif");
        }
      else if (i == 4)
        {
          widget = ctk_entry_new ();
        }
      else
        {
          widget = NULL; /* avoids a compiler warning */
          g_assert_not_reached ();
        }

      ctk_text_view_add_child_at_anchor (text_view,
                                         widget,
                                         anchor);

      ctk_widget_show_all (widget);

      ++i;
    }
}

CtkWidget *
do_textview (CtkWidget *do_widget)
{
  static CtkWidget *window = NULL;

  if (!window)
    {
      CtkWidget *vpaned;
      CtkWidget *view1;
      CtkWidget *view2;
      CtkWidget *sw;
      CtkTextBuffer *buffer;

      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window),
                             ctk_widget_get_screen (do_widget));
      ctk_window_set_default_size (CTK_WINDOW (window),
                                   450, 450);

      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);

      ctk_window_set_title (CTK_WINDOW (window), "Multiple Views");
      ctk_container_set_border_width (CTK_CONTAINER (window), 0);

      vpaned = ctk_paned_new (CTK_ORIENTATION_VERTICAL);
      ctk_container_set_border_width (CTK_CONTAINER(vpaned), 5);
      ctk_container_add (CTK_CONTAINER (window), vpaned);

      /* For convenience, we just use the autocreated buffer from
       * the first text view; you could also create the buffer
       * by itself with ctk_text_buffer_new(), then later create
       * a view widget.
       */
      view1 = ctk_text_view_new ();
      buffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (view1));
      view2 = ctk_text_view_new_with_buffer (buffer);

      sw = ctk_scrolled_window_new (NULL, NULL);
      ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (sw),
                                      CTK_POLICY_AUTOMATIC,
                                      CTK_POLICY_AUTOMATIC);
      ctk_paned_add1 (CTK_PANED (vpaned), sw);

      ctk_container_add (CTK_CONTAINER (sw), view1);

      sw = ctk_scrolled_window_new (NULL, NULL);
      ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (sw),
                                      CTK_POLICY_AUTOMATIC,
                                      CTK_POLICY_AUTOMATIC);
      ctk_paned_add2 (CTK_PANED (vpaned), sw);

      ctk_container_add (CTK_CONTAINER (sw), view2);

      create_tags (buffer);
      insert_text (buffer);

      attach_widgets (CTK_TEXT_VIEW (view1));
      attach_widgets (CTK_TEXT_VIEW (view2));

      ctk_widget_show_all (vpaned);
    }

  if (!ctk_widget_get_visible (window))
    {
      ctk_widget_show (window);
    }
  else
    {
      ctk_widget_destroy (window);
      window = NULL;
    }

  return window;
}

static void
recursive_attach_view (int                 depth,
                       CtkTextView        *view,
                       CtkTextChildAnchor *anchor)
{
  CtkWidget *child_view, *frame;

  if (depth > 4)
    return;

  child_view = ctk_text_view_new_with_buffer (ctk_text_view_get_buffer (view));

  /* Frame is to add a black border around each child view */
  frame = ctk_frame_new (NULL);
  ctk_container_add (CTK_CONTAINER (frame), child_view);

  ctk_text_view_add_child_at_anchor (view, frame, anchor);

  recursive_attach_view (depth + 1, CTK_TEXT_VIEW (child_view), anchor);
}

static void
easter_egg_callback (CtkWidget *button,
                     gpointer   data)
{
  static CtkWidget *window = NULL;
  gpointer window_ptr;
  CtkTextBuffer *buffer;
  CtkWidget     *view;
  CtkTextIter    iter;
  CtkTextChildAnchor *anchor;
  CtkWidget *sw;

  if (window)
    {
      ctk_window_present (CTK_WINDOW (window));
      return;
    }

  buffer = ctk_text_buffer_new (NULL);

  ctk_text_buffer_get_start_iter (buffer, &iter);

  ctk_text_buffer_insert (buffer, &iter,
      "This buffer is shared by a set of nested text views.\n Nested view:\n", -1);
  anchor = ctk_text_buffer_create_child_anchor (buffer, &iter);
  ctk_text_buffer_insert (buffer, &iter,
                          "\nDon't do this in real applications, please.\n", -1);

  view = ctk_text_view_new_with_buffer (buffer);

  recursive_attach_view (0, CTK_TEXT_VIEW (view), anchor);

  g_object_unref (buffer);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  sw = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (sw),
                                  CTK_POLICY_AUTOMATIC,
                                  CTK_POLICY_AUTOMATIC);

  ctk_container_add (CTK_CONTAINER (window), sw);
  ctk_container_add (CTK_CONTAINER (sw), view);

  window_ptr = &window;
  g_object_add_weak_pointer (G_OBJECT (window), window_ptr);

  ctk_window_set_default_size (CTK_WINDOW (window), 300, 400);

  ctk_widget_show_all (window);
}
