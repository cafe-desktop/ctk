#include <gtk/gtk.h>

typedef enum {
  RESPONSE_UNREVEAL,
} Response;

static void
on_info_bar_response (GtkInfoBar *info_bar,
                      int         response_id,
                      void       *user_data)
{
  switch (response_id)
  {
  case GTK_RESPONSE_CLOSE:
    ctk_widget_hide (GTK_WIDGET (info_bar));
    break;

  case RESPONSE_UNREVEAL:
    ctk_info_bar_set_revealed (info_bar, FALSE);
    break;

  default:
    g_assert_not_reached ();
  }
}

static void
on_activate (GApplication *application,
             void         *user_data)
{
  GtkWidget *box;
  GtkWidget *info_bar;
  GtkWidget *widget;

  box = ctk_box_new (GTK_ORIENTATION_VERTICAL, 10);

  info_bar = ctk_info_bar_new ();
  ctk_container_add (GTK_CONTAINER (ctk_info_bar_get_content_area (GTK_INFO_BAR (info_bar))),
                     ctk_label_new ("Hello!\nI am a GtkInfoBar"));

  widget = ctk_toggle_button_new_with_label ("Toggle :visible");
  ctk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
  g_object_bind_property (widget, "active",
                          info_bar, "visible",
                          G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  ctk_container_add (GTK_CONTAINER (box), widget);

  widget = ctk_toggle_button_new_with_label ("Toggle :revealed");
  ctk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
  g_object_bind_property (widget, "active",
                          info_bar, "revealed",
                          G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  ctk_container_add (GTK_CONTAINER (box), widget);

  widget = ctk_toggle_button_new_with_label ("Toggle :show-close-button");
  ctk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
  g_object_bind_property (widget, "active",
                          info_bar, "show-close-button",
                          G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  ctk_container_add (GTK_CONTAINER (box), widget);

  widget = ctk_combo_box_text_new ();
  ctk_combo_box_text_append (GTK_COMBO_BOX_TEXT (widget),
                             NULL, "GTK_MESSAGE_INFO");
  ctk_combo_box_text_append (GTK_COMBO_BOX_TEXT (widget),
                             NULL, "GTK_MESSAGE_WARNING");
  ctk_combo_box_text_append (GTK_COMBO_BOX_TEXT (widget),
                             NULL, "GTK_MESSAGE_QUESTION");
  ctk_combo_box_text_append (GTK_COMBO_BOX_TEXT (widget),
                             NULL, "GTK_MESSAGE_ERROR");
  ctk_combo_box_text_append (GTK_COMBO_BOX_TEXT (widget),
                             NULL, "GTK_MESSAGE_OTHER");
  ctk_combo_box_set_active (GTK_COMBO_BOX (widget), 0);
  g_object_bind_property (widget, "active",
                          info_bar, "message-type",
                          G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  ctk_container_add (GTK_CONTAINER (box), widget);

  ctk_container_add (GTK_CONTAINER (box), info_bar);

  widget = ctk_button_new_with_label ("Un-reveal");
  ctk_info_bar_add_action_widget (GTK_INFO_BAR (info_bar), widget,
                                  RESPONSE_UNREVEAL);

  g_signal_connect (info_bar, "response",
                    G_CALLBACK (on_info_bar_response), widget);

  widget = ctk_window_new (GTK_WINDOW_TOPLEVEL);
  ctk_container_add (GTK_CONTAINER (widget), box);
  ctk_widget_show_all (widget);
  ctk_application_add_window (GTK_APPLICATION (application),
                              GTK_WINDOW (widget));
}

int
main (int   argc,
      char *argv[])
{
  GtkApplication *application;
  int result;

  application = ctk_application_new ("org.gtk.test.infobar",
                                     G_APPLICATION_FLAGS_NONE);
  g_signal_connect (application, "activate", G_CALLBACK (on_activate), NULL);

  result = g_application_run (G_APPLICATION (application), argc, argv);
  g_object_unref (application);
  return result;
}
