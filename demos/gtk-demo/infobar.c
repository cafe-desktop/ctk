/* Info Bars
 *
 * Info bar widgets are used to report important messages to the user.
 */

#include <glib/gi18n.h>
#include <gtk/gtk.h>

static void
on_bar_response (GtkInfoBar *info_bar,
                 gint        response_id,
                 gpointer    user_data)
{
  GtkWidget *dialog;
  GtkWidget *window;

  if (response_id == GTK_RESPONSE_CLOSE)
    {
      ctk_widget_hide (GTK_WIDGET (info_bar));
      return;
    }

  window = ctk_widget_get_toplevel (GTK_WIDGET (info_bar));
  dialog = ctk_message_dialog_new (GTK_WINDOW (window),
                                   GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_INFO,
                                   GTK_BUTTONS_OK,
                                   "You clicked a button on an info bar");
  ctk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                            "Your response has id %d", response_id);

  g_signal_connect_swapped (dialog,
                            "response",
                            G_CALLBACK (ctk_widget_destroy),
                            dialog);

  ctk_widget_show_all (dialog);
}

GtkWidget *
do_infobar (GtkWidget *do_widget)
{
  static GtkWidget *window = NULL;
  GtkWidget *frame;
  GtkWidget *bar;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *label;
  GtkWidget *actions;
  GtkWidget *button;

  if (!window)
    {
      actions = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

      window = ctk_window_new (GTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (GTK_WINDOW (window),
                             ctk_widget_get_screen (do_widget));
      ctk_window_set_title (GTK_WINDOW (window), "Info Bars");

      g_signal_connect (window, "destroy", G_CALLBACK (ctk_widget_destroyed), &window);
      ctk_container_set_border_width (GTK_CONTAINER (window), 8);

      vbox = ctk_box_new (GTK_ORIENTATION_VERTICAL, 0);
      ctk_container_add (GTK_CONTAINER (window), vbox);

      bar = ctk_info_bar_new ();
      ctk_box_pack_start (GTK_BOX (vbox), bar, FALSE, FALSE, 0);
      ctk_info_bar_set_message_type (GTK_INFO_BAR (bar), GTK_MESSAGE_INFO);
      label = ctk_label_new ("This is an info bar with message type GTK_MESSAGE_INFO");
      ctk_label_set_line_wrap (GTK_LABEL (label), TRUE);
      ctk_label_set_xalign (GTK_LABEL (label), 0);
      ctk_box_pack_start (GTK_BOX (ctk_info_bar_get_content_area (GTK_INFO_BAR (bar))), label, FALSE, FALSE, 0);

      button = ctk_toggle_button_new_with_label ("Message");
      g_object_bind_property (button, "active", bar, "visible", G_BINDING_BIDIRECTIONAL);
      ctk_container_add (GTK_CONTAINER (actions), button);

      bar = ctk_info_bar_new ();
      ctk_box_pack_start (GTK_BOX (vbox), bar, FALSE, FALSE, 0);
      ctk_info_bar_set_message_type (GTK_INFO_BAR (bar), GTK_MESSAGE_WARNING);
      label = ctk_label_new ("This is an info bar with message type GTK_MESSAGE_WARNING");
      ctk_label_set_line_wrap (GTK_LABEL (label), TRUE);
      ctk_label_set_xalign (GTK_LABEL (label), 0);
      ctk_box_pack_start (GTK_BOX (ctk_info_bar_get_content_area (GTK_INFO_BAR (bar))), label, FALSE, FALSE, 0);

      button = ctk_toggle_button_new_with_label ("Warning");
      g_object_bind_property (button, "active", bar, "visible", G_BINDING_BIDIRECTIONAL);
      ctk_container_add (GTK_CONTAINER (actions), button);

      bar = ctk_info_bar_new_with_buttons (_("_OK"), GTK_RESPONSE_OK, NULL);
      ctk_info_bar_set_show_close_button (GTK_INFO_BAR (bar), TRUE);
      g_signal_connect (bar, "response", G_CALLBACK (on_bar_response), window);
      ctk_box_pack_start (GTK_BOX (vbox), bar, FALSE, FALSE, 0);
      ctk_info_bar_set_message_type (GTK_INFO_BAR (bar), GTK_MESSAGE_QUESTION);
      label = ctk_label_new ("This is an info bar with message type GTK_MESSAGE_QUESTION");
      ctk_label_set_line_wrap (GTK_LABEL (label), TRUE);
      ctk_label_set_xalign (GTK_LABEL (label), 0);
      ctk_box_pack_start (GTK_BOX (ctk_info_bar_get_content_area (GTK_INFO_BAR (bar))), label, FALSE, FALSE, 0);
      ctk_info_bar_set_default_response (GTK_INFO_BAR (bar), GTK_RESPONSE_OK);

      button = ctk_toggle_button_new_with_label ("Question");
      g_object_bind_property (button, "active", bar, "visible", G_BINDING_BIDIRECTIONAL);
      ctk_container_add (GTK_CONTAINER (actions), button);

      bar = ctk_info_bar_new ();
      ctk_box_pack_start (GTK_BOX (vbox), bar, FALSE, FALSE, 0);
      ctk_info_bar_set_message_type (GTK_INFO_BAR (bar), GTK_MESSAGE_ERROR);
      label = ctk_label_new ("This is an info bar with message type GTK_MESSAGE_ERROR");
      ctk_label_set_line_wrap (GTK_LABEL (label), TRUE);
      ctk_label_set_xalign (GTK_LABEL (label), 0);
      ctk_box_pack_start (GTK_BOX (ctk_info_bar_get_content_area (GTK_INFO_BAR (bar))), label, FALSE, FALSE, 0);

      button = ctk_toggle_button_new_with_label ("Error");
      g_object_bind_property (button, "active", bar, "visible", G_BINDING_BIDIRECTIONAL);
      ctk_container_add (GTK_CONTAINER (actions), button);

      bar = ctk_info_bar_new ();
      ctk_box_pack_start (GTK_BOX (vbox), bar, FALSE, FALSE, 0);
      ctk_info_bar_set_message_type (GTK_INFO_BAR (bar), GTK_MESSAGE_OTHER);
      label = ctk_label_new ("This is an info bar with message type GTK_MESSAGE_OTHER");
      ctk_label_set_line_wrap (GTK_LABEL (label), TRUE);
      ctk_label_set_xalign (GTK_LABEL (label), 0);
      ctk_box_pack_start (GTK_BOX (ctk_info_bar_get_content_area (GTK_INFO_BAR (bar))), label, FALSE, FALSE, 0);

      button = ctk_toggle_button_new_with_label ("Other");
      g_object_bind_property (button, "active", bar, "visible", G_BINDING_BIDIRECTIONAL);
      ctk_container_add (GTK_CONTAINER (actions), button);

      frame = ctk_frame_new ("Info bars");
      ctk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 8);

      vbox2 = ctk_box_new (GTK_ORIENTATION_VERTICAL, 8);
      ctk_container_set_border_width (GTK_CONTAINER (vbox2), 8);
      ctk_container_add (GTK_CONTAINER (frame), vbox2);

      /* Standard message dialog */
      label = ctk_label_new ("An example of different info bars");
      ctk_box_pack_start (GTK_BOX (vbox2), label, FALSE, FALSE, 0);

      ctk_widget_show_all (actions);
      ctk_box_pack_start (GTK_BOX (vbox2), actions, FALSE, FALSE, 0);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);

  return window;
}
