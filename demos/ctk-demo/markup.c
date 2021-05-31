/* Text View/Markup
 *
 * CtkTextBuffer lets you define your own tags that can influence
 * text formatting in a variety of ways. In this example, we show
 * that CtkTextBuffer can load Pango markup and automatically generate
 * suitable tags.
 */

#include <ctk/ctk.h>

static CtkWidget *stack;
static CtkWidget *view;
static CtkWidget *view2;

static void
source_toggled (CtkToggleButton *button)
{
  if (ctk_toggle_button_get_active (button))
    ctk_stack_set_visible_child_name (CTK_STACK (stack), "source");
  else
    {
      CtkTextBuffer *buffer;
      CtkTextIter start, end;
      gchar *markup;

      buffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (view2));
      ctk_text_buffer_get_bounds (buffer, &start, &end);
      markup = ctk_text_buffer_get_text (buffer, &start, &end, FALSE);

      buffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (view));
      ctk_text_buffer_get_bounds (buffer, &start, &end);
      ctk_text_buffer_delete (buffer, &start, &end);
      ctk_text_buffer_insert_markup (buffer, &start, markup, -1);
      g_free (markup);

      ctk_stack_set_visible_child_name (CTK_STACK (stack), "formatted");
    }
}

CtkWidget *
do_markup (CtkWidget *do_widget)
{
  static CtkWidget *window = NULL;

  if (!window)
    {
      CtkWidget *sw;
      CtkTextBuffer *buffer;
      CtkTextIter iter;
      GBytes *bytes;
      const gchar *markup;
      CtkWidget *header;
      CtkWidget *show_source;

      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window),
                             ctk_widget_get_screen (do_widget));
      ctk_window_set_default_size (CTK_WINDOW (window), 450, 450);
      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);

      stack = ctk_stack_new ();
      ctk_widget_show (stack);
      ctk_container_add (CTK_CONTAINER (window), stack);

      show_source = ctk_check_button_new_with_label ("Source");
      ctk_widget_set_valign (show_source, CTK_ALIGN_CENTER);
      g_signal_connect (show_source, "toggled", G_CALLBACK (source_toggled), stack);

      header = ctk_header_bar_new ();
      ctk_header_bar_set_show_close_button (CTK_HEADER_BAR (header), TRUE);
      ctk_header_bar_pack_start (CTK_HEADER_BAR (header), show_source);
      ctk_widget_show_all (header);
      ctk_window_set_titlebar (CTK_WINDOW (window), header);

      ctk_window_set_title (CTK_WINDOW (window), "Markup");

      view = ctk_text_view_new ();
      ctk_text_view_set_editable (CTK_TEXT_VIEW (view), FALSE);
      ctk_text_view_set_wrap_mode (CTK_TEXT_VIEW (view), CTK_WRAP_WORD);
      ctk_text_view_set_left_margin (CTK_TEXT_VIEW (view), 10);
      ctk_text_view_set_right_margin (CTK_TEXT_VIEW (view), 10);

      sw = ctk_scrolled_window_new (NULL, NULL);
      ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (sw),
                                      CTK_POLICY_AUTOMATIC,
                                      CTK_POLICY_AUTOMATIC);
      ctk_container_add (CTK_CONTAINER (sw), view);
      ctk_widget_show_all (sw);

      ctk_stack_add_named (CTK_STACK (stack), sw, "formatted");

      view2 = ctk_text_view_new ();
      ctk_text_view_set_wrap_mode (CTK_TEXT_VIEW (view2), CTK_WRAP_WORD);
      ctk_text_view_set_left_margin (CTK_TEXT_VIEW (view2), 10);
      ctk_text_view_set_right_margin (CTK_TEXT_VIEW (view2), 10);

      sw = ctk_scrolled_window_new (NULL, NULL);
      ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (sw),
                                      CTK_POLICY_AUTOMATIC,
                                      CTK_POLICY_AUTOMATIC);
      ctk_container_add (CTK_CONTAINER (sw), view2);
      ctk_widget_show_all (sw);

      ctk_stack_add_named (CTK_STACK (stack), sw, "source");

      bytes = g_resources_lookup_data ("/markup/markup.txt", 0, NULL);
      markup = (const gchar *)g_bytes_get_data (bytes, NULL);

      buffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (view));
      ctk_text_buffer_get_start_iter (buffer, &iter);
      ctk_text_buffer_insert_markup (buffer, &iter, markup, -1);

      buffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (view2));
      ctk_text_buffer_get_start_iter (buffer, &iter);
      ctk_text_buffer_insert (buffer, &iter, markup, -1);

      g_bytes_unref (bytes);

      ctk_widget_show (stack);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show (window);
  else
    ctk_widget_destroy (window);

  return window;
}
