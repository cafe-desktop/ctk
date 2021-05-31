/* Stack
 *
 * GtkStack is a container that shows a single child at a time,
 * with nice transitions when the visible child changes.
 *
 * GtkStackSwitcher adds buttons to control which child is visible.
 */

#include <gtk/gtk.h>

GtkWidget *
do_stack (GtkWidget *do_widget)
{
  static GtkWidget *window = NULL;

  if (!window)
    {
      GtkBuilder *builder;

      builder = ctk_builder_new_from_resource ("/stack/stack.ui");
      ctk_builder_connect_signals (builder, NULL);
      window = GTK_WIDGET (ctk_builder_get_object (builder, "window1"));
      ctk_window_set_screen (GTK_WINDOW (window),
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
