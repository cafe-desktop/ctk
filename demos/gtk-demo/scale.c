/* Scale
 *
 * GtkScale is a way to select a value from a range.
 * Scales can have marks to help pick special values,
 * and they can also restrict the values that can be
 * chosen.
 */

#include <gtk/gtk.h>

GtkWidget *
do_scale (GtkWidget *do_widget)
{
  static GtkWidget *window = NULL;

  if (!window)
    {
      GtkBuilder *builder;

      builder = ctk_builder_new_from_resource ("/scale/scale.ui");
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
