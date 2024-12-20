/* Printing/Page Setup
 *
 * CtkPageSetupUnixDialog can be used if page setup is needed
 * independent of a full printing dialog.
 */

#include <math.h>
#include <ctk/ctk.h>
#include <ctk/ctkunixprint.h>

static void
done_cb (CtkDialog *dialog,
	 gint       response G_GNUC_UNUSED,
	 gpointer   data G_GNUC_UNUSED)
{
  ctk_widget_destroy (CTK_WIDGET (dialog));
}

CtkWidget *
do_pagesetup (CtkWidget *do_widget)
{
  static CtkWidget *window = NULL;

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
