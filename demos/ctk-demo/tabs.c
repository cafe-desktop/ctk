/* Text View/Tabs
 *
 * GtkTextView can position text at fixed positions, using tabs.
 */

#include <ctk/ctk.h>
#include <gdk/gdkkeysyms.h>

GtkWidget *
do_tabs (GtkWidget *do_widget)
{
  static GtkWidget *window = NULL;

  if (!window)
    {
      GtkWidget *view;
      GtkWidget *sw;
      GtkTextBuffer *buffer;
      PangoTabArray *tabs;

      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_title (CTK_WINDOW (window), "Tabs");
      ctk_window_set_screen (CTK_WINDOW (window),
                             ctk_widget_get_screen (do_widget));
      ctk_window_set_default_size (CTK_WINDOW (window), 450, 450);

      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);

      ctk_container_set_border_width (CTK_CONTAINER (window), 0);

      view = ctk_text_view_new ();
      ctk_text_view_set_wrap_mode (CTK_TEXT_VIEW (view), CTK_WRAP_WORD);
      ctk_text_view_set_left_margin (CTK_TEXT_VIEW (view), 20);
      ctk_text_view_set_right_margin (CTK_TEXT_VIEW (view), 20);

      tabs = pango_tab_array_new (3, TRUE);
      pango_tab_array_set_tab (tabs, 0, PANGO_TAB_LEFT, 0);
      pango_tab_array_set_tab (tabs, 1, PANGO_TAB_LEFT, 150);
      pango_tab_array_set_tab (tabs, 2, PANGO_TAB_LEFT, 300);
      ctk_text_view_set_tabs (CTK_TEXT_VIEW (view), tabs);
      pango_tab_array_free (tabs);

      buffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (view));
      ctk_text_buffer_set_text (buffer, "one\ttwo\tthree\nfour\tfive\tsix\nseven\teight\tnine", -1);

      sw = ctk_scrolled_window_new (NULL, NULL);
      ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (sw),
                                      CTK_POLICY_AUTOMATIC,
                                      CTK_POLICY_AUTOMATIC);
      ctk_container_add (CTK_CONTAINER (window), sw);
      ctk_container_add (CTK_CONTAINER (sw), view);

      ctk_widget_show_all (sw);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show (window);
  else
    ctk_widget_destroy (window);

  return window;
}
