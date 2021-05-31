#include <ctk/ctk.h>

gint
main (gint argc,
      gchar ** argv)
{
  CtkWidget *window, *revealer, *box, *widget, *entry;

  ctk_init (&argc, &argv);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_widget_set_size_request (window, 300, 300);

  box = ctk_grid_new ();
  ctk_container_add (CTK_CONTAINER (window), box);

  widget = ctk_label_new ("Some filler text just to avoid\nresizing of the window");
  ctk_widget_set_margin_top (widget, 10);
  ctk_widget_set_margin_bottom (widget, 10);
  ctk_widget_set_margin_start (widget, 10);
  ctk_widget_set_margin_end (widget, 10);
  ctk_grid_attach (CTK_GRID (box), widget, 1, 1, 1, 1);

  widget = ctk_label_new ("Some filler text just to avoid\nresizing of the window");
  ctk_widget_set_margin_top (widget, 10);
  ctk_widget_set_margin_bottom (widget, 10);
  ctk_widget_set_margin_start (widget, 10);
  ctk_widget_set_margin_end (widget, 10);
  ctk_grid_attach (CTK_GRID (box), widget, 3, 3, 1, 1);

  widget = ctk_toggle_button_new_with_label ("None");
  ctk_grid_attach (CTK_GRID (box), widget, 0, 0, 1, 1);
  revealer = ctk_revealer_new ();
  ctk_widget_set_halign (revealer, CTK_ALIGN_START);
  ctk_widget_set_valign (revealer, CTK_ALIGN_START);
  entry = ctk_entry_new ();
  ctk_entry_set_text (CTK_ENTRY (entry), "00000");
  ctk_container_add (CTK_CONTAINER (revealer), entry);
  g_object_bind_property (widget, "active", revealer, "reveal-child", 0);
  ctk_revealer_set_transition_type (CTK_REVEALER (revealer), CTK_REVEALER_TRANSITION_TYPE_NONE);
  ctk_revealer_set_transition_duration (CTK_REVEALER (revealer), 2000);
  ctk_grid_attach (CTK_GRID (box), revealer, 1, 0, 1, 1);

  widget = ctk_toggle_button_new_with_label ("Fade");
  ctk_grid_attach (CTK_GRID (box), widget, 4, 4, 1, 1);
  revealer = ctk_revealer_new ();
  ctk_widget_set_halign (revealer, CTK_ALIGN_END);
  ctk_widget_set_valign (revealer, CTK_ALIGN_END);
  entry = ctk_entry_new ();
  ctk_entry_set_text (CTK_ENTRY (entry), "00000");
  ctk_container_add (CTK_CONTAINER (revealer), entry);
  g_object_bind_property (widget, "active", revealer, "reveal-child", 0);
  ctk_revealer_set_transition_type (CTK_REVEALER (revealer), CTK_REVEALER_TRANSITION_TYPE_CROSSFADE);
  ctk_revealer_set_transition_duration (CTK_REVEALER (revealer), 2000);
  ctk_grid_attach (CTK_GRID (box), revealer, 3, 4, 1, 1);

  widget = ctk_toggle_button_new_with_label ("Right");
  ctk_grid_attach (CTK_GRID (box), widget, 0, 2, 1, 1);
  revealer = ctk_revealer_new ();
  ctk_widget_set_hexpand (revealer, TRUE);
  ctk_widget_set_halign (revealer, CTK_ALIGN_START);
  entry = ctk_entry_new ();
  ctk_entry_set_text (CTK_ENTRY (entry), "12345");
  ctk_container_add (CTK_CONTAINER (revealer), entry);
  g_object_bind_property (widget, "active", revealer, "reveal-child", 0);
  ctk_revealer_set_transition_type (CTK_REVEALER (revealer), CTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT);
  ctk_revealer_set_transition_duration (CTK_REVEALER (revealer), 2000);
  ctk_grid_attach (CTK_GRID (box), revealer, 1, 2, 1, 1);

  widget = ctk_toggle_button_new_with_label ("Down");
  ctk_grid_attach (CTK_GRID (box), widget, 2, 0, 1, 1);
  revealer = ctk_revealer_new ();
  ctk_widget_set_vexpand (revealer, TRUE);
  ctk_widget_set_valign (revealer, CTK_ALIGN_START);
  entry = ctk_entry_new ();
  ctk_entry_set_text (CTK_ENTRY (entry), "23456");
  ctk_container_add (CTK_CONTAINER (revealer), entry);
  g_object_bind_property (widget, "active", revealer, "reveal-child", 0);
  ctk_revealer_set_transition_type (CTK_REVEALER (revealer), CTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN);
  ctk_revealer_set_transition_duration (CTK_REVEALER (revealer), 2000);
  ctk_grid_attach (CTK_GRID (box), revealer, 2, 1, 1, 1);

  widget = ctk_toggle_button_new_with_label ("Left");
  ctk_grid_attach (CTK_GRID (box), widget, 4, 2, 1, 1);
  revealer = ctk_revealer_new ();
  ctk_widget_set_hexpand (revealer, TRUE);
  ctk_widget_set_halign (revealer, CTK_ALIGN_END);
  entry = ctk_entry_new ();
  ctk_entry_set_text (CTK_ENTRY (entry), "34567");
  ctk_container_add (CTK_CONTAINER (revealer), entry);
  g_object_bind_property (widget, "active", revealer, "reveal-child", 0);
  ctk_revealer_set_transition_type (CTK_REVEALER (revealer), CTK_REVEALER_TRANSITION_TYPE_SLIDE_LEFT);
  ctk_revealer_set_transition_duration (CTK_REVEALER (revealer), 2000);
  ctk_grid_attach (CTK_GRID (box), revealer, 3, 2, 1, 1);

  widget = ctk_toggle_button_new_with_label ("Up");
  ctk_grid_attach (CTK_GRID (box), widget, 2, 4, 1, 1);
  revealer = ctk_revealer_new ();
  ctk_widget_set_vexpand (revealer, TRUE);
  ctk_widget_set_valign (revealer, CTK_ALIGN_END);
  entry = ctk_entry_new ();
  ctk_entry_set_text (CTK_ENTRY (entry), "45678");
  ctk_container_add (CTK_CONTAINER (revealer), entry);
  g_object_bind_property (widget, "active", revealer, "reveal-child", 0);
  ctk_revealer_set_transition_type (CTK_REVEALER (revealer), CTK_REVEALER_TRANSITION_TYPE_SLIDE_UP);
  ctk_revealer_set_transition_duration (CTK_REVEALER (revealer), 2000);
  ctk_grid_attach (CTK_GRID (box), revealer, 2, 3, 1, 1);

  ctk_widget_show_all (window);
  ctk_main ();

  ctk_widget_destroy (window);

  return 0;
}
