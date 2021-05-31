/* Stack
 *
 * CtkStack is a container that shows a single child at a time,
 * with nice transitions when the visible child changes.
 *
 * CtkStackSwitcher adds buttons to control which child is visible.
 */

#include <ctk/ctk.h>

CtkWidget *
do_stack (CtkWidget *do_widget)
{
  static CtkWidget *window = NULL;

  if (!window)
    {
      CtkBuilder *builder;

      builder = ctk_builder_new_from_resource ("/stack/stack.ui");
      ctk_builder_connect_signals (builder, NULL);
      window = CTK_WIDGET (ctk_builder_get_object (builder, "window1"));
      ctk_window_set_screen (CTK_WINDOW (window),
                             ctk_widget_get_screen (do_widget));
      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);

      g_object_unref (builder);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);


  return window;
}
