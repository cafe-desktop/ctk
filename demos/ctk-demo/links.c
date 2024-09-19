/* Links
 *
 * CtkLabel can show hyperlinks. The default action is to call
 * ctk_show_uri_on_window() on their URI, but it is possible to override
 * this with a custom handler.
 */

#include <ctk/ctk.h>

static void
response_cb (CtkWidget *dialog,
             gint       response_id G_GNUC_UNUSED,
             gpointer   data G_GNUC_UNUSED)
{
  ctk_widget_destroy (dialog);
}

static gboolean
activate_link (CtkWidget   *label,
               const gchar *uri,
               gpointer     data G_GNUC_UNUSED)
{
  if (g_strcmp0 (uri, "keynav") == 0)
    {
      CtkWidget *dialog;
      CtkWidget *parent;

      parent = ctk_widget_get_toplevel (label);
      dialog = ctk_message_dialog_new_with_markup (CTK_WINDOW (parent),
                 CTK_DIALOG_DESTROY_WITH_PARENT,
                 CTK_MESSAGE_INFO,
                 CTK_BUTTONS_OK,
                 "The term <i>keynav</i> is a shorthand for "
                 "keyboard navigation and refers to the process of using "
                 "a program (exclusively) via keyboard input.");
      ctk_window_set_modal (CTK_WINDOW (dialog), TRUE);

      ctk_window_present (CTK_WINDOW (dialog));
      g_signal_connect (dialog, "response", G_CALLBACK (response_cb), NULL);

      return TRUE;
    }

  return FALSE;
}

CtkWidget *
do_links (CtkWidget *do_widget)
{
  static CtkWidget *window = NULL;

  if (!window)
    {
      CtkWidget *label;

      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window),
                             ctk_widget_get_screen (do_widget));
      ctk_window_set_title (CTK_WINDOW (window), "Links");
      ctk_container_set_border_width (CTK_CONTAINER (window), 12);
      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);

      label = ctk_label_new ("Some <a href=\"http://en.wikipedia.org/wiki/Text\""
                             "title=\"plain text\">text</a> may be marked up\n"
                             "as hyperlinks, which can be clicked\n"
                             "or activated via <a href=\"keynav\">keynav</a>\n"
                             "and they work fine with other markup, like when\n"
                             "searching on <a href=\"http://www.google.com/\">"
                             "<span color=\"#0266C8\">G</span><span color=\"#F90101\">o</span>"
                             "<span color=\"#F2B50F\">o</span><span color=\"#0266C8\">g</span>"
                             "<span color=\"#00933B\">l</span><span color=\"#F90101\">e</span>"
                             "</a>.");
      ctk_label_set_use_markup (CTK_LABEL (label), TRUE);
      g_signal_connect (label, "activate-link", G_CALLBACK (activate_link), NULL);
      ctk_container_add (CTK_CONTAINER (window), label);
      ctk_widget_show (label);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show (window);
  else
    ctk_widget_destroy (window);

  return window;
}
