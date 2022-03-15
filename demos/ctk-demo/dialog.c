/* Dialogs and Message Boxes
 *
 * Dialog widgets are used to pop up a transient window for user feedback.
 */

#include <glib/gi18n.h>
#include <ctk/ctk.h>

static CtkWidget *window = NULL;
static CtkWidget *entry1 = NULL;
static CtkWidget *entry2 = NULL;

static void
message_dialog_clicked (CtkButton *button,
                        gpointer   user_data)
{
  CtkWidget *dialog;
  static gint i = 1;

  dialog = ctk_message_dialog_new (CTK_WINDOW (window),
                                   CTK_DIALOG_MODAL | CTK_DIALOG_DESTROY_WITH_PARENT,
                                   CTK_MESSAGE_INFO,
                                   CTK_BUTTONS_OK_CANCEL,
                                   "This message box has been popped up the following\n"
                                   "number of times:");
  ctk_message_dialog_format_secondary_text (CTK_MESSAGE_DIALOG (dialog),
                                            "%d", i);
  ctk_dialog_run (CTK_DIALOG (dialog));
  ctk_widget_destroy (dialog);
  i++;
}

static void
interactive_dialog_clicked (CtkButton *button,
                            gpointer   user_data)
{
  CtkWidget *content_area;
  CtkWidget *dialog;
  CtkWidget *hbox;
  CtkWidget *image;
  CtkWidget *table;
  CtkWidget *local_entry1;
  CtkWidget *local_entry2;
  CtkWidget *label;
  gint response;

  dialog = ctk_dialog_new_with_buttons ("Interactive Dialog",
                                        CTK_WINDOW (window),
                                        CTK_DIALOG_MODAL| CTK_DIALOG_DESTROY_WITH_PARENT,
                                        _("_OK"),
                                        CTK_RESPONSE_OK,
                                        "_Cancel",
                                        CTK_RESPONSE_CANCEL,
                                        NULL);

  content_area = ctk_dialog_get_content_area (CTK_DIALOG (dialog));

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 8);
  ctk_container_set_border_width (CTK_CONTAINER (hbox), 8);
  ctk_box_pack_start (CTK_BOX (content_area), hbox, FALSE, FALSE, 0);

  image = ctk_image_new_from_icon_name ("dialog-question", CTK_ICON_SIZE_DIALOG);
  ctk_box_pack_start (CTK_BOX (hbox), image, FALSE, FALSE, 0);

  table = ctk_grid_new ();
  ctk_grid_set_row_spacing (CTK_GRID (table), 4);
  ctk_grid_set_column_spacing (CTK_GRID (table), 4);
  ctk_box_pack_start (CTK_BOX (hbox), table, TRUE, TRUE, 0);
  label = ctk_label_new_with_mnemonic ("_Entry 1");
  ctk_grid_attach (CTK_GRID (table), label, 0, 0, 1, 1);
  local_entry1 = ctk_entry_new ();
  ctk_entry_set_text (CTK_ENTRY (local_entry1), ctk_entry_get_text (CTK_ENTRY (entry1)));
  ctk_grid_attach (CTK_GRID (table), local_entry1, 1, 0, 1, 1);
  ctk_label_set_mnemonic_widget (CTK_LABEL (label), local_entry1);

  label = ctk_label_new_with_mnemonic ("E_ntry 2");
  ctk_grid_attach (CTK_GRID (table), label, 0, 1, 1, 1);

  local_entry2 = ctk_entry_new ();
  ctk_entry_set_text (CTK_ENTRY (local_entry2), ctk_entry_get_text (CTK_ENTRY (entry2)));
  ctk_grid_attach (CTK_GRID (table), local_entry2, 1, 1, 1, 1);
  ctk_label_set_mnemonic_widget (CTK_LABEL (label), local_entry2);

  ctk_widget_show_all (hbox);
  response = ctk_dialog_run (CTK_DIALOG (dialog));

  if (response == CTK_RESPONSE_OK)
    {
      ctk_entry_set_text (CTK_ENTRY (entry1), ctk_entry_get_text (CTK_ENTRY (local_entry1)));
      ctk_entry_set_text (CTK_ENTRY (entry2), ctk_entry_get_text (CTK_ENTRY (local_entry2)));
    }

  ctk_widget_destroy (dialog);
}

CtkWidget *
do_dialog (CtkWidget *do_widget)
{
  if (!window)
    {
      CtkWidget *frame;
      CtkWidget *vbox;
      CtkWidget *vbox2;
      CtkWidget *hbox;
      CtkWidget *button;
      CtkWidget *table;
      CtkWidget *label;

      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window),
                             ctk_widget_get_screen (do_widget));
      ctk_window_set_title (CTK_WINDOW (window), "Dialogs and Message Boxes");

      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);

      ctk_container_set_border_width (CTK_CONTAINER (window), 8);

      frame = ctk_frame_new ("Dialogs");
      ctk_container_add (CTK_CONTAINER (window), frame);

      vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 8);
      ctk_container_set_border_width (CTK_CONTAINER (vbox), 8);
      ctk_container_add (CTK_CONTAINER (frame), vbox);

      /* Standard message dialog */
      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 8);
      ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      button = ctk_button_new_with_mnemonic ("_Message Dialog");
      g_signal_connect (button, "clicked",
                        G_CALLBACK (message_dialog_clicked), NULL);
      ctk_box_pack_start (CTK_BOX (hbox), button, FALSE, FALSE, 0);

      ctk_box_pack_start (CTK_BOX (vbox), ctk_separator_new (CTK_ORIENTATION_HORIZONTAL),
                          FALSE, FALSE, 0);

      /* Interactive dialog*/
      hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 8);
      ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      vbox2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);

      button = ctk_button_new_with_mnemonic ("_Interactive Dialog");
      g_signal_connect (button, "clicked",
                        G_CALLBACK (interactive_dialog_clicked), NULL);
      ctk_box_pack_start (CTK_BOX (hbox), vbox2, FALSE, FALSE, 0);
      ctk_box_pack_start (CTK_BOX (vbox2), button, FALSE, FALSE, 0);

      table = ctk_grid_new ();
      ctk_grid_set_row_spacing (CTK_GRID (table), 4);
      ctk_grid_set_column_spacing (CTK_GRID (table), 4);
      ctk_box_pack_start (CTK_BOX (hbox), table, FALSE, FALSE, 0);

      label = ctk_label_new_with_mnemonic ("_Entry 1");
      ctk_grid_attach (CTK_GRID (table), label, 0, 0, 1, 1);

      entry1 = ctk_entry_new ();
      ctk_grid_attach (CTK_GRID (table), entry1, 1, 0, 1, 1);
      ctk_label_set_mnemonic_widget (CTK_LABEL (label), entry1);

      label = ctk_label_new_with_mnemonic ("E_ntry 2");
      ctk_grid_attach (CTK_GRID (table), label, 0, 1, 1, 1);

      entry2 = ctk_entry_new ();
      ctk_grid_attach (CTK_GRID (table), entry2, 1, 1, 1, 1);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);

  return window;
}
