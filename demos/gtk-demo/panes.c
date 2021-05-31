/* Paned Widgets
 *
 * The GtkPaned Widget divides its content area into two panes
 * with a divider in between that the user can adjust. A separate
 * child is placed into each pane. GtkPaned widgets can be split
 * horizontally or vertially.
 *
 * There are a number of options that can be set for each pane.
 * This test contains both a horizontal and a vertical GtkPaned
 * widget, and allows you to adjust the options for each side of
 * each widget.
 */

#include <gtk/gtk.h>

void
toggle_resize (GtkWidget *widget,
               GtkWidget *child)
{
  GtkWidget *parent;
  GtkPaned *paned;
  gboolean is_child1;
  gboolean resize, shrink;

  parent = ctk_widget_get_parent (child);
  paned = GTK_PANED (parent);

  is_child1 = (child == ctk_paned_get_child1 (paned));

  ctk_container_child_get (GTK_CONTAINER (paned), child,
                           "resize", &resize,
                           "shrink", &shrink,
                           NULL);

  g_object_ref (child);
  ctk_container_remove (GTK_CONTAINER (parent), child);
  if (is_child1)
    ctk_paned_pack1 (paned, child, !resize, shrink);
  else
    ctk_paned_pack2 (paned, child, !resize, shrink);
  g_object_unref (child);
}

void
toggle_shrink (GtkWidget *widget,
               GtkWidget *child)
{
  GtkWidget *parent;
  GtkPaned *paned;
  gboolean is_child1;
  gboolean resize, shrink;

  parent = ctk_widget_get_parent (child);
  paned = GTK_PANED (parent);

  is_child1 = (child == ctk_paned_get_child1 (paned));

  ctk_container_child_get (GTK_CONTAINER (paned), child,
                           "resize", &resize,
                           "shrink", &shrink,
                           NULL);

  g_object_ref (child);
  ctk_container_remove (GTK_CONTAINER (parent), child);
  if (is_child1)
    ctk_paned_pack1 (paned, child, resize, !shrink);
  else
    ctk_paned_pack2 (paned, child, resize, !shrink);
  g_object_unref (child);
}

GtkWidget *
create_pane_options (GtkPaned    *paned,
                     const gchar *frame_label,
                     const gchar *label1,
                     const gchar *label2)
{
  GtkWidget *child1, *child2;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *check_button;

  child1 = ctk_paned_get_child1 (paned);
  child2 = ctk_paned_get_child2 (paned);

  frame = ctk_frame_new (frame_label);
  ctk_container_set_border_width (GTK_CONTAINER (frame), 4);

  table = ctk_grid_new ();
  ctk_container_add (GTK_CONTAINER (frame), table);

  label = ctk_label_new (label1);
  ctk_grid_attach (GTK_GRID (table), label, 0, 0, 1, 1);

  check_button = ctk_check_button_new_with_mnemonic ("_Resize");
  ctk_grid_attach (GTK_GRID (table), check_button, 0, 1, 1, 1);
  g_signal_connect (check_button, "toggled",
                    G_CALLBACK (toggle_resize), child1);

  check_button = ctk_check_button_new_with_mnemonic ("_Shrink");
  ctk_grid_attach (GTK_GRID (table), check_button, 0, 2, 1, 1);
  ctk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button), TRUE);
  g_signal_connect (check_button, "toggled",
                    G_CALLBACK (toggle_shrink), child1);

  label = ctk_label_new (label2);
  ctk_grid_attach (GTK_GRID (table), label, 1, 0, 1, 1);

  check_button = ctk_check_button_new_with_mnemonic ("_Resize");
  ctk_grid_attach (GTK_GRID (table), check_button, 1, 1, 1, 1);
  ctk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button), TRUE);
  g_signal_connect (check_button, "toggled",
                    G_CALLBACK (toggle_resize), child2);

  check_button = ctk_check_button_new_with_mnemonic ("_Shrink");
  ctk_grid_attach (GTK_GRID (table), check_button, 1, 2, 1, 1);
  ctk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button), TRUE);
  g_signal_connect (check_button, "toggled",
                    G_CALLBACK (toggle_shrink), child2);

  return frame;
}

GtkWidget *
do_panes (GtkWidget *do_widget)
{
  static GtkWidget *window = NULL;
  GtkWidget *frame;
  GtkWidget *hpaned;
  GtkWidget *vpaned;
  GtkWidget *button;
  GtkWidget *vbox;

  if (!window)
    {
      window = ctk_window_new (GTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (GTK_WINDOW (window),
                             ctk_widget_get_screen (do_widget));

      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);

      ctk_window_set_title (GTK_WINDOW (window), "Paned Widgets");
      ctk_container_set_border_width (GTK_CONTAINER (window), 0);

      vbox = ctk_box_new (GTK_ORIENTATION_VERTICAL, 0);
      ctk_container_add (GTK_CONTAINER (window), vbox);

      vpaned = ctk_paned_new (GTK_ORIENTATION_VERTICAL);
      ctk_box_pack_start (GTK_BOX (vbox), vpaned, TRUE, TRUE, 0);
      ctk_container_set_border_width (GTK_CONTAINER(vpaned), 5);

      hpaned = ctk_paned_new (GTK_ORIENTATION_HORIZONTAL);
      ctk_paned_add1 (GTK_PANED (vpaned), hpaned);

      frame = ctk_frame_new (NULL);
      ctk_frame_set_shadow_type (GTK_FRAME(frame), GTK_SHADOW_IN);
      ctk_widget_set_size_request (frame, 60, 60);
      ctk_paned_add1 (GTK_PANED (hpaned), frame);

      button = ctk_button_new_with_mnemonic ("_Hi there");
      ctk_container_add (GTK_CONTAINER(frame), button);

      frame = ctk_frame_new (NULL);
      ctk_frame_set_shadow_type (GTK_FRAME(frame), GTK_SHADOW_IN);
      ctk_widget_set_size_request (frame, 80, 60);
      ctk_paned_add2 (GTK_PANED (hpaned), frame);

      frame = ctk_frame_new (NULL);
      ctk_frame_set_shadow_type (GTK_FRAME(frame), GTK_SHADOW_IN);
      ctk_widget_set_size_request (frame, 60, 80);
      ctk_paned_add2 (GTK_PANED (vpaned), frame);

      /* Now create toggle buttons to control sizing */

      ctk_box_pack_start (GTK_BOX (vbox),
                          create_pane_options (GTK_PANED (hpaned),
                                               "Horizontal",
                                               "Left",
                                               "Right"),
                          FALSE, FALSE, 0);

      ctk_box_pack_start (GTK_BOX (vbox),
                          create_pane_options (GTK_PANED (vpaned),
                                               "Vertical",
                                               "Top",
                                               "Bottom"),
                          FALSE, FALSE, 0);

      ctk_widget_show_all (vbox);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show (window);
  else
    ctk_widget_destroy (window);

  return window;
}
