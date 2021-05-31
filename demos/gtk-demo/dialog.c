/* Dialogs and Message Boxes
 *
 * Dialog widgets are used to pop up a transient window for user feedback.
 */

#include <glib/gi18n.h>
#include <gtk/gtk.h>

static GtkWidget *window = NULL;
static GtkWidget *entry1 = NULL;
static GtkWidget *entry2 = NULL;

static void
message_dialog_clicked (GtkButton *button,
                        gpointer   user_data)
{
  GtkWidget *dialog;
  static gint i = 1;

  dialog = ctk_message_dialog_new (GTK_WINDOW (window),
                                   GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_INFO,
                                   GTK_BUTTONS_OK_CANCEL,
                                   "This message box has been popped up the following\n"
                                   "number of times:");
  ctk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                            "%d", i);
  ctk_dialog_run (GTK_DIALOG (dialog));
  ctk_widget_destroy (dialog);
  i++;
}

static void
interactive_dialog_clicked (GtkButton *button,
                            gpointer   user_data)
{
  GtkWidget *content_area;
  GtkWidget *dialog;
  GtkWidget *hbox;
  GtkWidget *image;
  GtkWidget *table;
  GtkWidget *local_entry1;
  GtkWidget *local_entry2;
  GtkWidget *label;
  gint response;

  dialog = ctk_dialog_new_with_buttons ("Interactive Dialog",
                                        GTK_WINDOW (window),
                                        GTK_DIALOG_MODAL| GTK_DIALOG_DESTROY_WITH_PARENT,
                                        _("_OK"),
                                        GTK_RESPONSE_OK,
                                        "_Cancel",
                                        GTK_RESPONSE_CANCEL,
                                        NULL);

  content_area = ctk_dialog_get_content_area (GTK_DIALOG (dialog));

  hbox = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 8);
  ctk_container_set_border_width (GTK_CONTAINER (hbox), 8);
  ctk_box_pack_start (GTK_BOX (content_area), hbox, FALSE, FALSE, 0);

  image = ctk_image_new_from_icon_name ("dialog-question", GTK_ICON_SIZE_DIALOG);
  ctk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);

  table = ctk_grid_new ();
  ctk_grid_set_row_spacing (GTK_GRID (table), 4);
  ctk_grid_set_column_spacing (GTK_GRID (table), 4);
  ctk_box_pack_start (GTK_BOX (hbox), table, TRUE, TRUE, 0);
  label = ctk_label_new_with_mnemonic ("_Entry 1");
  ctk_grid_attach (GTK_GRID (table), label, 0, 0, 1, 1);
  local_entry1 = ctk_entry_new ();
  ctk_entry_set_text (GTK_ENTRY (local_entry1), ctk_entry_get_text (GTK_ENTRY (entry1)));
  ctk_grid_attach (GTK_GRID (table), local_entry1, 1, 0, 1, 1);
  ctk_label_set_mnemonic_widget (GTK_LABEL (label), local_entry1);

  label = ctk_label_new_with_mnemonic ("E_ntry 2");
  ctk_grid_attach (GTK_GRID (table), label, 0, 1, 1, 1);

  local_entry2 = ctk_entry_new ();
  ctk_entry_set_text (GTK_ENTRY (local_entry2), ctk_entry_get_text (GTK_ENTRY (entry2)));
  ctk_grid_attach (GTK_GRID (table), local_entry2, 1, 1, 1, 1);
  ctk_label_set_mnemonic_widget (GTK_LABEL (label), local_entry2);

  ctk_widget_show_all (hbox);
  response = ctk_dialog_run (GTK_DIALOG (dialog));

  if (response == GTK_RESPONSE_OK)
    {
      ctk_entry_set_text (GTK_ENTRY (entry1), ctk_entry_get_text (GTK_ENTRY (local_entry1)));
      ctk_entry_set_text (GTK_ENTRY (entry2), ctk_entry_get_text (GTK_ENTRY (local_entry2)));
    }

  ctk_widget_destroy (dialog);
}

GtkWidget *
do_dialog (GtkWidget *do_widget)
{
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *hbox;
  GtkWidget *button;
  GtkWidget *table;
  GtkWidget *label;

  if (!window)
    {
      window = ctk_window_new (GTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (GTK_WINDOW (window),
                             ctk_widget_get_screen (do_widget));
      ctk_window_set_title (GTK_WINDOW (window), "Dialogs and Message Boxes");

      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);

      ctk_container_set_border_width (GTK_CONTAINER (window), 8);

      frame = ctk_frame_new ("Dialogs");
      ctk_container_add (GTK_CONTAINER (window), frame);

      vbox = ctk_box_new (GTK_ORIENTATION_VERTICAL, 8);
      ctk_container_set_border_width (GTK_CONTAINER (vbox), 8);
      ctk_container_add (GTK_CONTAINER (frame), vbox);

      /* Standard message dialog */
      hbox = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 8);
      ctk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      button = ctk_button_new_with_mnemonic ("_Message Dialog");
      g_signal_connect (button, "clicked",
                        G_CALLBACK (message_dialog_clicked), NULL);
      ctk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

      ctk_box_pack_start (GTK_BOX (vbox), ctk_separator_new (GTK_ORIENTATION_HORIZONTAL),
                          FALSE, FALSE, 0);

      /* Interactive dialog*/
      hbox = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 8);
      ctk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      vbox2 = ctk_box_new (GTK_ORIENTATION_VERTICAL, 0);

      button = ctk_button_new_with_mnemonic ("_Interactive Dialog");
      g_signal_connect (button, "clicked",
                        G_CALLBACK (interactive_dialog_clicked), NULL);
      ctk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, FALSE, 0);
      ctk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);

      table = ctk_grid_new ();
      ctk_grid_set_row_spacing (GTK_GRID (table), 4);
      ctk_grid_set_column_spacing (GTK_GRID (table), 4);
      ctk_box_pack_start (GTK_BOX (hbox), table, FALSE, FALSE, 0);

      label = ctk_label_new_with_mnemonic ("_Entry 1");
      ctk_grid_attach (GTK_GRID (table), label, 0, 0, 1, 1);

      entry1 = ctk_entry_new ();
      ctk_grid_attach (GTK_GRID (table), entry1, 1, 0, 1, 1);
      ctk_label_set_mnemonic_widget (GTK_LABEL (label), entry1);

      label = ctk_label_new_with_mnemonic ("E_ntry 2");
      ctk_grid_attach (GTK_GRID (table), label, 0, 1, 1, 1);

      entry2 = ctk_entry_new ();
      ctk_grid_attach (GTK_GRID (table), entry2, 1, 1, 1, 1);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);

  return window;
}
