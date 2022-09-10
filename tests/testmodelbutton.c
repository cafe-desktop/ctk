#include <ctk/ctk.h>

static void
on_action_beep (GSimpleAction *action,
                GVariant      *parameter,
                void          *user_data)
{
  CdkDisplay *display = cdk_display_get_default ();
  g_assert (CDK_IS_DISPLAY (display));
  cdk_display_beep (display);
}

static void
on_application_activate (GApplication *gapplication,
                         void         *user_data)
{
  CtkApplication *application = CTK_APPLICATION (gapplication);
  CtkCssProvider *css_provider = ctk_css_provider_new ();
  CdkScreen *screen = cdk_screen_get_default ();

  GSimpleAction *action;
  CtkWidget *box;
  GIcon *gicon;
  CtkWidget *model_button;
  CtkWidget *widget;

  ctk_css_provider_load_from_data (css_provider,
    "window > box { padding: 0.5em; }"
    "window > box > * { margin: 0.5em; }"
    /* :iconic == FALSE */
    "modelbutton > check { background: red; }"
    "modelbutton > radio { background: green; }"
    "modelbutton > arrow { background: blue; }"
    /* :iconic == TRUE */
    "button.model { background: yellow; }"
    , -1, NULL);
  g_assert (CDK_IS_SCREEN (screen));
  ctk_style_context_add_provider_for_screen (screen,
                                             CTK_STYLE_PROVIDER (css_provider),
                                             CTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  action = g_simple_action_new ("beep", NULL);
  g_signal_connect (action, "activate", G_CALLBACK (on_action_beep), NULL);
  g_action_map_add_action (G_ACTION_MAP (application), G_ACTION (action));

  box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);

  gicon = g_themed_icon_new ("face-smile");

  model_button = g_object_new (CTK_TYPE_MODEL_BUTTON,
                               "action-name", "app.beep",
                               "text", "It’s-a-me! ModelButton",
                               "icon", gicon,
                               NULL);
  ctk_container_add (CTK_CONTAINER (box), model_button);

  g_object_unref (gicon);

  widget = ctk_combo_box_text_new ();
  ctk_combo_box_text_append (CTK_COMBO_BOX_TEXT (widget),
                             NULL, "CTK_BUTTON_ROLE_NORMAL");
  ctk_combo_box_text_append (CTK_COMBO_BOX_TEXT (widget),
                             NULL, "CTK_BUTTON_ROLE_CHECK");
  ctk_combo_box_text_append (CTK_COMBO_BOX_TEXT (widget),
                             NULL, "CTK_BUTTON_ROLE_RADIO");
  ctk_combo_box_set_active (CTK_COMBO_BOX (widget), 0);
  g_object_bind_property (widget, "active",
                          model_button, "role",
                          G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  ctk_container_add (CTK_CONTAINER (box), widget);

  widget = ctk_toggle_button_new_with_label (":centered");
  g_object_bind_property (widget, "active",
                          model_button, "centered",
                          G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  ctk_container_add (CTK_CONTAINER (box), widget);

  widget = ctk_toggle_button_new_with_label (":iconic");
  g_object_bind_property (widget, "active",
                          model_button, "iconic",
                          G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  ctk_container_add (CTK_CONTAINER (box), widget);

  widget = ctk_toggle_button_new_with_label (":inverted");
  g_object_bind_property (widget, "active",
                          model_button, "inverted",
                          G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  ctk_container_add (CTK_CONTAINER (box), widget);

  widget = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_container_add (CTK_CONTAINER (widget), box);
  ctk_widget_show_all (widget);
  ctk_application_add_window (CTK_APPLICATION (application), CTK_WINDOW (widget));
}

int
main (int   argc,
      char *argv[])
{
  CtkApplication *application = ctk_application_new ("org.ctk.test.modelbutton",
                                                     G_APPLICATION_DEFAULT_FLAGS);
  int result;

  g_signal_connect (application, "activate",
                    G_CALLBACK (on_application_activate), NULL);

  result = g_application_run (G_APPLICATION (application), argc, argv);
  g_object_unref (application);
  return result;
}
