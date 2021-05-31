/* Spinner
 *
 * CtkSpinner allows to show that background activity is on-going.
 *
 */

#include <glib/gi18n.h>
#include <ctk/ctk.h>

static CtkWidget *spinner_sensitive = NULL;
static CtkWidget *spinner_unsensitive = NULL;

static void
on_play_clicked (CtkButton *button, gpointer user_data)
{
  ctk_spinner_start (CTK_SPINNER (spinner_sensitive));
  ctk_spinner_start (CTK_SPINNER (spinner_unsensitive));
}

static void
on_stop_clicked (CtkButton *button, gpointer user_data)
{
  ctk_spinner_stop (CTK_SPINNER (spinner_sensitive));
  ctk_spinner_stop (CTK_SPINNER (spinner_unsensitive));
}

CtkWidget *
do_spinner (CtkWidget *do_widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *content_area;
  CtkWidget *vbox;
  CtkWidget *hbox;
  CtkWidget *button;
  CtkWidget *spinner;

  if (!window)
  {
    window = ctk_dialog_new_with_buttons ("Spinner",
                                          CTK_WINDOW (do_widget),
                                          0,
                                          _("_Close"),
                                          CTK_RESPONSE_NONE,
                                          NULL);
    ctk_window_set_resizable (CTK_WINDOW (window), FALSE);

    g_signal_connect (window, "response",
                      G_CALLBACK (ctk_widget_destroy), NULL);
    g_signal_connect (window, "destroy",
                      G_CALLBACK (ctk_widget_destroyed), &window);

    content_area = ctk_dialog_get_content_area (CTK_DIALOG (window));

    vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 5);
    ctk_box_pack_start (CTK_BOX (content_area), vbox, TRUE, TRUE, 0);
    ctk_container_set_border_width (CTK_CONTAINER (vbox), 5);

    /* Sensitive */
    hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 5);
    spinner = ctk_spinner_new ();
    ctk_container_add (CTK_CONTAINER (hbox), spinner);
    ctk_container_add (CTK_CONTAINER (hbox), ctk_entry_new ());
    ctk_container_add (CTK_CONTAINER (vbox), hbox);
    spinner_sensitive = spinner;

    /* Disabled */
    hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 5);
    spinner = ctk_spinner_new ();
    ctk_container_add (CTK_CONTAINER (hbox), spinner);
    ctk_container_add (CTK_CONTAINER (hbox), ctk_entry_new ());
    ctk_container_add (CTK_CONTAINER (vbox), hbox);
    spinner_unsensitive = spinner;
    ctk_widget_set_sensitive (hbox, FALSE);

    button = ctk_button_new_with_label (_("Play"));
    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (on_play_clicked), spinner);
    ctk_container_add (CTK_CONTAINER (vbox), button);

    button = ctk_button_new_with_label (_("Stop"));
    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (on_stop_clicked), spinner);
    ctk_container_add (CTK_CONTAINER (vbox), button);

    /* Start by default to test for:
     * https://bugzilla.gnome.org/show_bug.cgi?id=598496 */
    on_play_clicked (NULL, NULL);
  }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);

  return window;
}
