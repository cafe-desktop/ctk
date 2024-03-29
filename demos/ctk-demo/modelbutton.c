/* Model Button
 *
 * CtkModelButton is a button widget that is designed to be used with
 * a GAction as model. The button will adjust its appearance according
 * to the kind of action it is connected to.
 *
 * It is also possible to use CtkModelButton without a GAction. In this
 * case, you should set the "role" attribute yourself, and connect to the
 * "clicked" signal as you would for any other button.
 *
 * A common use of CtkModelButton is to implement menu-like content
 * in popovers.
 */

#include <ctk/ctk.h>

static void
tool_clicked (CtkButton *button)
{
  gboolean active;

  g_object_get (button, "active", &active, NULL);
  g_object_set (button, "active", !active, NULL);
}

CtkWidget *
do_modelbutton (CtkWidget *do_widget)
{
  static CtkWidget *window = NULL;
  static GActionEntry win_entries[] = {
    { .name = "color", .parameter_type = "s", .state = "'red'" },
    { .name = "chocolate", .state = "true" },
    { .name = "vanilla", .state = "false" },
    { .name = "sprinkles" }
  };

  if (!window)
    {
      CtkBuilder *builder;
      GActionGroup *actions;

      builder = ctk_builder_new_from_resource ("/modelbutton/modelbutton.ui");
      ctk_builder_add_callback_symbol (builder, "tool_clicked", G_CALLBACK (tool_clicked));
      ctk_builder_connect_signals (builder, NULL);
      window = CTK_WIDGET (ctk_builder_get_object (builder, "window1"));
      ctk_window_set_screen (CTK_WINDOW (window),
                             ctk_widget_get_screen (do_widget));
      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);

      actions = (GActionGroup*)g_simple_action_group_new ();
      g_action_map_add_action_entries (G_ACTION_MAP (actions),
                                       win_entries, G_N_ELEMENTS (win_entries),
                                       window);
      ctk_widget_insert_action_group (window, "win", actions);


      g_object_unref (builder);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);


  return window;
}

