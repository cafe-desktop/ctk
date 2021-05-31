/* Printing/Page Setup
 *
 * GtkPageSetupUnixDialog can be used if page setup is needed
 * independent of a full printing dialog.
 */

#include <math.h>
#include <ctk/ctk.h>
#include <ctk/ctkunixprint.h>

static void
done_cb (GtkDialog *dialog, gint response, gpointer data)
{
  ctk_widget_destroy (CTK_WIDGET (dialog));
}

GtkWidget *
do_pagesetup (GtkWidget *do_widget)
{
  static GtkWidget *window = NULL;

  if (!window)
    {
      window = ctk_page_setup_unix_dialog_new ("Page Setup", CTK_WINDOW (do_widget));
      g_signal_connect (window, "destroy", G_CALLBACK (ctk_widget_destroyed), &window);
      g_signal_connect (window, "response", G_CALLBACK (done_cb), NULL);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show (window);
  else
    ctk_widget_destroy (window);

  return window;
}
