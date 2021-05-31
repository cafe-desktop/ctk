/* Spinner
 *
 * GtkSpinner allows to show that background activity is on-going.
 *
 */

#include <glib/gi18n.h>
#include <gtk/gtk.h>

static GtkWidget *spinner_sensitive = NULL;
static GtkWidget *spinner_unsensitive = NULL;

static void
on_play_clicked (GtkButton *button, gpointer user_data)
{
  ctk_spinner_start (GTK_SPINNER (spinner_sensitive));
  ctk_spinner_start (GTK_SPINNER (spinner_unsensitive));
}

static void
on_stop_clicked (GtkButton *button, gpointer user_data)
{
  ctk_spinner_stop (GTK_SPINNER (spinner_sensitive));
  ctk_spinner_stop (GTK_SPINNER (spinner_unsensitive));
}

GtkWidget *
do_spinner (GtkWidget *do_widget)
{
  static GtkWidget *window = NULL;
  GtkWidget *content_area;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *button;
  GtkWidget *spinner;

  if (!window)
  {
    window = ctk_dialog_new_with_buttons ("Spinner",
                                          GTK_WINDOW (do_widget),
                                          0,
                                          _("_Close"),
                                          GTK_RESPONSE_NONE,
                                          NULL);
    ctk_window_set_resizable (GTK_WINDOW (window), FALSE);

    g_signal_connect (window, "response",
                      G_CALLBACK (ctk_widget_destroy), NULL);
    g_signal_connect (window, "destroy",
                      G_CALLBACK (ctk_widget_destroyed), &window);

    content_area = ctk_dialog_get_content_area (GTK_DIALOG (window));

    vbox = ctk_box_new (GTK_ORIENTATION_VERTICAL, 5);
    ctk_box_pack_start (GTK_BOX (content_area), vbox, TRUE, TRUE, 0);
    ctk_container_set_border_width (GTK_CONTAINER (vbox), 5);

    /* Sensitive */
    hbox = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
    spinner = ctk_spinner_new ();
    ctk_container_add (GTK_CONTAINER (hbox), spinner);
    ctk_container_add (GTK_CONTAINER (hbox), ctk_entry_new ());
    ctk_container_add (GTK_CONTAINER (vbox), hbox);
    spinner_sensitive = spinner;

    /* Disabled */
    hbox = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
    spinner = ctk_spinner_new ();
    ctk_container_add (GTK_CONTAINER (hbox), spinner);
    ctk_container_add (GTK_CONTAINER (hbox), ctk_entry_new ());
    ctk_container_add (GTK_CONTAINER (vbox), hbox);
    spinner_unsensitive = spinner;
    ctk_widget_set_sensitive (hbox, FALSE);

    button = ctk_button_new_with_label (_("Play"));
    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (on_play_clicked), spinner);
    ctk_container_add (GTK_CONTAINER (vbox), button);

    button = ctk_button_new_with_label (_("Stop"));
    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (on_stop_clicked), spinner);
    ctk_container_add (GTK_CONTAINER (vbox), button);

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
