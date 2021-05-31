/* Links
 *
 * GtkLabel can show hyperlinks. The default action is to call
 * ctk_show_uri_on_window() on their URI, but it is possible to override
 * this with a custom handler.
 */

#include <gtk/gtk.h>

static void
response_cb (GtkWidget *dialog,
             gint       response_id,
             gpointer   data)
{
  ctk_widget_destroy (dialog);
}

static gboolean
activate_link (GtkWidget   *label,
               const gchar *uri,
               gpointer     data)
{
  if (g_strcmp0 (uri, "keynav") == 0)
    {
      GtkWidget *dialog;
      GtkWidget *parent;

      parent = ctk_widget_get_toplevel (label);
      dialog = ctk_message_dialog_new_with_markup (GTK_WINDOW (parent),
                 GTK_DIALOG_DESTROY_WITH_PARENT,
                 GTK_MESSAGE_INFO,
                 GTK_BUTTONS_OK,
                 "The term <i>keynav</i> is a shorthand for "
                 "keyboard navigation and refers to the process of using "
                 "a program (exclusively) via keyboard input.");
      ctk_window_set_modal (GTK_WINDOW (dialog), TRUE);

      ctk_window_present (GTK_WINDOW (dialog));
      g_signal_connect (dialog, "response", G_CALLBACK (response_cb), NULL);

      return TRUE;
    }

  return FALSE;
}

GtkWidget *
do_links (GtkWidget *do_widget)
{
  static GtkWidget *window = NULL;
  GtkWidget *label;

  if (!window)
    {
      window = ctk_window_new (GTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (GTK_WINDOW (window),
                             ctk_widget_get_screen (do_widget));
      ctk_window_set_title (GTK_WINDOW (window), "Links");
      ctk_container_set_border_width (GTK_CONTAINER (window), 12);
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
      ctk_label_set_use_markup (GTK_LABEL (label), TRUE);
      g_signal_connect (label, "activate-link", G_CALLBACK (activate_link), NULL);
      ctk_container_add (GTK_CONTAINER (window), label);
      ctk_widget_show (label);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show (window);
  else
    ctk_widget_destroy (window);

  return window;
}
