/* Theming/Style Classes
 *
 * GTK+ uses CSS for theming. Style classes can be associated
 * with widgets to inform the theme about intended rendering.
 *
 * This demo shows some common examples where theming features
 * of GTK+ are used for certain effects: primary toolbars,
 * inline toolbars and linked buttons.
 */

#include <ctk/ctk.h>

static GtkWidget *window = NULL;

GtkWidget *
do_theming_style_classes (GtkWidget *do_widget)
{
  GtkWidget *grid;
  GtkBuilder *builder;

  if (!window)
    {
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window),
                             ctk_widget_get_screen (do_widget));
      ctk_window_set_title (CTK_WINDOW (window), "Style Classes");
      ctk_window_set_resizable (CTK_WINDOW (window), FALSE);
      ctk_container_set_border_width (CTK_CONTAINER (window), 12);
      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);

      builder = ctk_builder_new_from_resource ("/theming_style_classes/theming.ui");

      grid = (GtkWidget *)ctk_builder_get_object (builder, "grid");
      ctk_widget_show_all (grid);
      ctk_container_add (CTK_CONTAINER (window), grid);
      g_object_unref (builder);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show (window);
  else
    ctk_widget_destroy (window);

  return window;
}
