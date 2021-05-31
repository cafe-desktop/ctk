#include <gtk/gtk.h>

static void
expand_toggled (GtkToggleButton *b, GtkWidget *w)
{
  gboolean active;
  GtkWidget *parent;

  active = ctk_toggle_button_get_active (b);
  parent = ctk_widget_get_parent (w);
  ctk_container_child_set (GTK_CONTAINER (parent), w,
                           "expand", active,
                           NULL);
}

static void
fill_toggled (GtkToggleButton *b, GtkWidget *w)
{
  gboolean active;
  GtkWidget *parent;

  active = ctk_toggle_button_get_active (b);
  parent = ctk_widget_get_parent (w);
  ctk_container_child_set (GTK_CONTAINER (parent), w,
                           "fill", active,
                           NULL);
}

static void
edit_widget (GtkWidget *button)
{
  GtkWidget *dialog;
  GtkWidget *grid;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *check;
  gboolean expand, fill;

  dialog = GTK_WIDGET (g_object_get_data (G_OBJECT (button), "dialog"));

  if (!dialog)
    {
      dialog = ctk_dialog_new_with_buttons ("",
                                            GTK_WINDOW (ctk_widget_get_toplevel (button)),
                                            GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_USE_HEADER_BAR,
                                            NULL, NULL);

      grid = ctk_grid_new ();
      g_object_set (grid,
                    "margin", 20,
                    "row-spacing", 10,
                    "column-spacing", 10,
                    NULL);
      ctk_container_add (GTK_CONTAINER (ctk_dialog_get_content_area (GTK_DIALOG (dialog))), grid);

      label = ctk_label_new ("Label:");
      ctk_widget_set_halign (label, GTK_ALIGN_END);
      entry = ctk_entry_new ();
      g_object_bind_property (button, "label",
                              entry, "text",
                              G_BINDING_BIDIRECTIONAL|G_BINDING_SYNC_CREATE);
      ctk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);
      ctk_grid_attach (GTK_GRID (grid), entry, 1, 0, 1, 1);

      label = ctk_label_new ("Visible:");
      ctk_widget_set_halign (label, GTK_ALIGN_END);
      check = ctk_check_button_new ();
      g_object_bind_property (button, "visible",
                              check, "active",
                              G_BINDING_BIDIRECTIONAL|G_BINDING_SYNC_CREATE);
      ctk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);
      ctk_grid_attach (GTK_GRID (grid), check, 1, 1, 1, 1);

      label = ctk_label_new ("Expand:");
      ctk_widget_set_halign (label, GTK_ALIGN_END);
      check = ctk_check_button_new ();
      ctk_box_query_child_packing (GTK_BOX (ctk_widget_get_parent (button)),
                                   button, &expand, NULL, NULL, NULL);
      ctk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), expand);
      g_signal_connect (check, "toggled",
                        G_CALLBACK (expand_toggled), button);
      ctk_grid_attach (GTK_GRID (grid), label, 0, 2, 1, 1);
      ctk_grid_attach (GTK_GRID (grid), check, 1, 2, 1, 1);

      label = ctk_label_new ("Fill:");
      ctk_widget_set_halign (label, GTK_ALIGN_END);
      check = ctk_check_button_new ();
      ctk_box_query_child_packing (GTK_BOX (ctk_widget_get_parent (button)),
                                   button, NULL, &fill, NULL, NULL);
      ctk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), fill);
      g_signal_connect (check, "toggled",
                        G_CALLBACK (fill_toggled), button);
      ctk_grid_attach (GTK_GRID (grid), label, 0, 3, 1, 1);
      ctk_grid_attach (GTK_GRID (grid), check, 1, 3, 1, 1);
 
      ctk_widget_show_all (grid);

      g_object_set_data (G_OBJECT (button), "dialog", dialog);
    }

  ctk_window_present (GTK_WINDOW (dialog));
}

static GtkWidget *
test_widget (const gchar *label)
{
  GtkWidget *w;

  w = ctk_button_new_with_label (label);
  g_signal_connect (w, "clicked", G_CALLBACK (edit_widget), NULL);

  return w;
}

static void
spacing_changed (GtkSpinButton *spin, GtkBox *box)
{
  gint spacing;

  spacing = ctk_spin_button_get_value_as_int (spin);
  ctk_box_set_spacing (box, spacing);
}

int
main (int argc, char *argv[])
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *box;
  GtkWidget *check;
  GtkWidget *b;
  GtkWidget *label;
  GtkWidget *spin;

  ctk_init (NULL, NULL);

  window = ctk_window_new (GTK_WINDOW_TOPLEVEL);

  vbox = ctk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  ctk_container_add (GTK_CONTAINER (window), vbox);

  box = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  ctk_box_pack_start (GTK_BOX (box), test_widget ("1"), FALSE, TRUE, 0);
  ctk_box_pack_start (GTK_BOX (box), test_widget ("2"), FALSE, TRUE, 0);
  ctk_box_pack_start (GTK_BOX (box), test_widget ("3"), FALSE, TRUE, 0);
  ctk_box_pack_start (GTK_BOX (box), test_widget ("4"), FALSE, TRUE, 0);
  ctk_box_pack_end (GTK_BOX (box), test_widget ("5"), FALSE, TRUE, 0);
  ctk_box_pack_end (GTK_BOX (box), test_widget ("6"), FALSE, TRUE, 0);

  ctk_box_set_center_widget (GTK_BOX (box), test_widget ("center"));
  ctk_container_add (GTK_CONTAINER (vbox), box);

  check = ctk_check_button_new_with_label ("Homogeneous");
  g_object_bind_property (box, "homogeneous",
                          check, "active",
                          G_BINDING_BIDIRECTIONAL|G_BINDING_SYNC_CREATE);
  g_object_set (check, "margin", 10, NULL);
  ctk_widget_set_halign (check, GTK_ALIGN_CENTER);
  ctk_widget_show (check);
  ctk_container_add (GTK_CONTAINER (vbox), check);

  b = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);
  g_object_set (b, "margin", 10, NULL);
  ctk_widget_set_halign (b, GTK_ALIGN_CENTER);
  label = ctk_label_new ("Spacing:");
  ctk_widget_set_halign (label, GTK_ALIGN_END);
  ctk_box_pack_start (GTK_BOX (b), label, FALSE, TRUE, 0);

  spin = ctk_spin_button_new_with_range (0, 10, 1);
  ctk_spin_button_set_numeric (GTK_SPIN_BUTTON (spin), TRUE);
  ctk_widget_set_halign (spin, GTK_ALIGN_START);
  g_signal_connect (spin, "value-changed",
                    G_CALLBACK (spacing_changed), box);
  ctk_box_pack_start (GTK_BOX (b), spin, FALSE, TRUE, 0);
  ctk_container_add (GTK_CONTAINER (vbox), b);
  
  ctk_widget_show_all (window);

  ctk_main ();

  return 0;
}
