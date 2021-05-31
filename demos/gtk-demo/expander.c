/* Expander
 *
 * GtkExpander allows to provide additional content that is initially hidden.
 * This is also known as "disclosure triangle".
 *
 * This example also shows how to make the window resizable only if the expander
 * is expanded.
 */

#include <glib/gi18n.h>
#include <gtk/gtk.h>

static GtkWidget *window = NULL;

static void
response_cb (GtkDialog *dialog, gint response_id)
{
  ctk_widget_destroy (window);
  window = NULL;
}

static void
expander_cb (GtkExpander *expander, GParamSpec *pspec, GtkWindow *dialog)
{
  ctk_window_set_resizable (dialog, ctk_expander_get_expanded (expander));
}

static void
do_not_expand (GtkWidget *child, gpointer data)
{
  ctk_container_child_set (CTK_CONTAINER (ctk_widget_get_parent (child)), child,
                           "expand", FALSE, "fill", FALSE, NULL);
}

GtkWidget *
do_expander (GtkWidget *do_widget)
{
  GtkWidget *toplevel;
  GtkWidget *area;
  GtkWidget *box;
  GtkWidget *expander;
  GtkWidget *sw;
  GtkWidget *tv;
  GtkTextBuffer *buffer;

  if (!window)
    {
      toplevel = ctk_widget_get_toplevel (do_widget);
      window = ctk_message_dialog_new_with_markup (CTK_WINDOW (toplevel),
                                                   0,
                                                   CTK_MESSAGE_ERROR,
                                                   CTK_BUTTONS_CLOSE,
                                                   "<big><b>%s</b></big>",
                                                   "Something went wrong");
      ctk_message_dialog_format_secondary_text (CTK_MESSAGE_DIALOG (window),
                                                "Here are some more details "
                                                "but not the full story.");

      area = ctk_message_dialog_get_message_area (CTK_MESSAGE_DIALOG (window));
      box = ctk_widget_get_parent (area);
      ctk_container_child_set (CTK_CONTAINER (ctk_widget_get_parent (box)), box,
                               "expand", TRUE, "fill", TRUE, NULL);
      ctk_container_foreach (CTK_CONTAINER (area), do_not_expand, NULL);

      expander = ctk_expander_new ("Details:");
      sw = ctk_scrolled_window_new (NULL, NULL);
      ctk_scrolled_window_set_min_content_height (CTK_SCROLLED_WINDOW (sw), 100);
      ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (sw), CTK_SHADOW_IN);
      ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (sw),
                                      CTK_POLICY_NEVER,
                                      CTK_POLICY_AUTOMATIC);

      tv = ctk_text_view_new ();
      buffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (tv));
      ctk_text_view_set_editable (CTK_TEXT_VIEW (tv), FALSE);
      ctk_text_view_set_wrap_mode (CTK_TEXT_VIEW (tv), CTK_WRAP_WORD);
      ctk_text_buffer_set_text (CTK_TEXT_BUFFER (buffer),
                                "Finally, the full story with all details. "
                                "And all the inside information, including "
                                "error codes, etc etc. Pages of information, "
                                "you might have to scroll down to read it all, "
                                "or even resize the window - it works !\n"
                                "A second paragraph will contain even more "
                                "innuendo, just to make you scroll down or "
                                "resize the window. Do it already !", -1);
      ctk_container_add (CTK_CONTAINER (sw), tv);
      ctk_container_add (CTK_CONTAINER (expander), sw);
      ctk_box_pack_end (CTK_BOX (area), expander, TRUE, TRUE, 0);
      ctk_widget_show_all (expander);
      g_signal_connect (expander, "notify::expanded",
                        G_CALLBACK (expander_cb), window);

      g_signal_connect (window, "response", G_CALLBACK (response_cb), NULL);
  }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);

  return window;
}
