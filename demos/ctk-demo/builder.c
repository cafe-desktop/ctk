/* Builder
 *
 * Demonstrates an interface loaded from a XML description.
 */

#include <ctk/ctk.h>

static void
quit_activate (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
  CtkWidget *window = user_data;

  ctk_widget_destroy (window);
}

static void
about_activate (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  CtkWidget *window = user_data;
  CtkBuilder *builder;
  CtkWidget *about_dlg;

  builder = g_object_get_data (G_OBJECT (window), "builder");
  about_dlg = CTK_WIDGET (ctk_builder_get_object (builder, "aboutdialog1"));
  ctk_dialog_run (CTK_DIALOG (about_dlg));
  ctk_widget_hide (about_dlg);
}

static void
help_activate (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
  g_print ("Help not available\n");
}

static GActionEntry win_entries[] = {
  { "quit", quit_activate, NULL, NULL, NULL },
  { "about", about_activate, NULL, NULL, NULL },
  { "help", help_activate, NULL, NULL, NULL }
};

CtkWidget *
do_builder (CtkWidget *do_widget)
{
  static CtkWidget *window = NULL;
  CtkWidget *toolbar;
  GActionGroup *actions;
  CtkAccelGroup *accel_group;
  CtkWidget *item;

  if (!window)
    {
      CtkBuilder *builder;

      builder = ctk_builder_new_from_resource ("/builder/demo.ui");

      ctk_builder_connect_signals (builder, NULL);
      window = CTK_WIDGET (ctk_builder_get_object (builder, "window1"));
      ctk_window_set_screen (CTK_WINDOW (window),
                             ctk_widget_get_screen (do_widget));
      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);
      toolbar = CTK_WIDGET (ctk_builder_get_object (builder, "toolbar1"));
      ctk_style_context_add_class (ctk_widget_get_style_context (toolbar),
                                   "primary-toolbar");
      actions = (GActionGroup*)g_simple_action_group_new ();
      g_action_map_add_action_entries (G_ACTION_MAP (actions),
                                       win_entries, G_N_ELEMENTS (win_entries),
                                       window);
      ctk_widget_insert_action_group (window, "win", actions);
      accel_group = ctk_accel_group_new ();
      ctk_window_add_accel_group (CTK_WINDOW (window), accel_group);

      item = (CtkWidget*)ctk_builder_get_object (builder, "new_item");
      ctk_widget_add_accelerator (item, "activate", accel_group,
                                  CDK_KEY_n, CDK_CONTROL_MASK, CTK_ACCEL_VISIBLE);

      item = (CtkWidget*)ctk_builder_get_object (builder, "open_item");
      ctk_widget_add_accelerator (item, "activate", accel_group,
                                  CDK_KEY_o, CDK_CONTROL_MASK, CTK_ACCEL_VISIBLE);

      item = (CtkWidget*)ctk_builder_get_object (builder, "save_item");
      ctk_widget_add_accelerator (item, "activate", accel_group,
                                  CDK_KEY_s, CDK_CONTROL_MASK, CTK_ACCEL_VISIBLE);

      item = (CtkWidget*)ctk_builder_get_object (builder, "quit_item");
      ctk_widget_add_accelerator (item, "activate", accel_group,
                                  CDK_KEY_q, CDK_CONTROL_MASK, CTK_ACCEL_VISIBLE);

      item = (CtkWidget*)ctk_builder_get_object (builder, "copy_item");
      ctk_widget_add_accelerator (item, "activate", accel_group,
                                  CDK_KEY_c, CDK_CONTROL_MASK, CTK_ACCEL_VISIBLE);

      item = (CtkWidget*)ctk_builder_get_object (builder, "cut_item");
      ctk_widget_add_accelerator (item, "activate", accel_group,
                                  CDK_KEY_x, CDK_CONTROL_MASK, CTK_ACCEL_VISIBLE);

      item = (CtkWidget*)ctk_builder_get_object (builder, "paste_item");
      ctk_widget_add_accelerator (item, "activate", accel_group,
                                  CDK_KEY_v, CDK_CONTROL_MASK, CTK_ACCEL_VISIBLE);

      item = (CtkWidget*)ctk_builder_get_object (builder, "help_item");
      ctk_widget_add_accelerator (item, "activate", accel_group,
                                  CDK_KEY_F1, 0, CTK_ACCEL_VISIBLE);

      item = (CtkWidget*)ctk_builder_get_object (builder, "about_item");
      ctk_widget_add_accelerator (item, "activate", accel_group,
                                  CDK_KEY_F7, 0, CTK_ACCEL_VISIBLE);

      g_object_set_data_full (G_OBJECT(window), "builder", builder, g_object_unref);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);

  return window;
}
