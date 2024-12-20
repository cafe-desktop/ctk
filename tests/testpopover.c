#include <ctk/ctk.h>

static void
activate (GSimpleAction *action,
          GVariant      *parameter G_GNUC_UNUSED,
          gpointer       user_data G_GNUC_UNUSED)
{
  g_print ("%s activated\n", g_action_get_name (G_ACTION (action)));
}

static GActionEntry entries[] = {
  { .name = "cut", .activate = activate },
  { .name = "copy", .activate = activate },
  { .name = "paste", .activate = activate },
  { .name = "bold", .state = "false" },
  { .name = "italic", .state = "false" },
  { .name = "strikethrough", .state = "false" },
  { .name = "underline", .state = "false" },
  { .name = "set-view", .parameter_type = "s", .state = "'list'" },
  { .name = "action1", .activate = activate },
  { .name = "action2", .state = "true" },
  { .name = "action2a", .state = "false" },
  { .name = "action3", .parameter_type = "s", .state = "'three'" },
  { .name = "action4", .activate = activate },
  { .name = "action5", .activate = activate },
  { .name = "action6", .activate = activate },
  { .name = "action7", .activate = activate },
  { .name = "action8", .activate = activate },
  { .name = "action9", .activate = activate },
  { .name = "action10", .activate = activate }
};

int
main (int argc, char *argv[])
{
  CtkWidget *win;
  CtkWidget *box;
  CtkWidget *button;
  CtkWidget *button2;
  CtkBuilder *builder;
  GMenuModel *model;
  GSimpleActionGroup *actions;
  CtkWidget *overlay;
  CtkWidget *grid;
  CtkWidget *popover;
  CtkWidget *popover2;
  CtkWidget *label;
  CtkWidget *check;
  CtkWidget *combo;
  CtkWidget *header_bar;

  ctk_init (&argc, &argv);

  win = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_default_size (CTK_WINDOW (win), 400, 600);
  header_bar = ctk_header_bar_new ();
  ctk_header_bar_set_show_close_button (CTK_HEADER_BAR (header_bar), TRUE);
  ctk_window_set_titlebar (CTK_WINDOW (win), header_bar);
  ctk_window_set_title (CTK_WINDOW (win), "Test CtkPopover");
  actions = g_simple_action_group_new ();
  g_action_map_add_action_entries (G_ACTION_MAP (actions), entries, G_N_ELEMENTS (entries), NULL);

  ctk_widget_insert_action_group (win, "top", G_ACTION_GROUP (actions));

  overlay = ctk_overlay_new ();
  ctk_container_add (CTK_CONTAINER (win), overlay);

  grid = ctk_grid_new ();
  ctk_widget_set_halign (grid, CTK_ALIGN_FILL);
  ctk_widget_set_valign (grid, CTK_ALIGN_FILL);
  ctk_grid_set_row_spacing (CTK_GRID (grid), 10);
  ctk_grid_set_column_spacing (CTK_GRID (grid), 10);
  ctk_container_add (CTK_CONTAINER (overlay), grid);

  label = ctk_label_new ("");
  ctk_widget_set_hexpand (label, TRUE);
  ctk_widget_set_vexpand (label, TRUE);
  ctk_grid_attach (CTK_GRID (grid), label, 0, 0, 1, 1);

  label = ctk_label_new ("");
  ctk_widget_set_hexpand (label, TRUE);
  ctk_widget_set_vexpand (label, TRUE);
  ctk_grid_attach (CTK_GRID (grid), label, 3, 6, 1, 1);

  builder = ctk_builder_new_from_file ("popover.ui");
  model = (GMenuModel *)ctk_builder_get_object (builder, "menu");

  box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 6);
  button = ctk_menu_button_new ();
  ctk_container_add (CTK_CONTAINER (box), button);
  button2 = ctk_menu_button_new ();
  ctk_container_add (CTK_CONTAINER (box), button2);

  ctk_menu_button_set_menu_model (CTK_MENU_BUTTON (button), model);
  ctk_menu_button_set_use_popover (CTK_MENU_BUTTON (button), TRUE);
  popover = CTK_WIDGET (ctk_menu_button_get_popover (CTK_MENU_BUTTON (button)));

  builder = ctk_builder_new_from_file ("popover2.ui");
  popover2 = (CtkWidget *)ctk_builder_get_object (builder, "popover");
  ctk_menu_button_set_popover (CTK_MENU_BUTTON (button2), popover2);

  g_object_set (box, "margin", 10, NULL);
  ctk_overlay_add_overlay (CTK_OVERLAY (overlay), box);

  label = ctk_label_new ("Popover hexpand");
  check = ctk_check_button_new ();
  g_object_bind_property (check, "active", popover, "hexpand", G_BINDING_SYNC_CREATE);
  g_object_bind_property (check, "active", popover2, "hexpand", G_BINDING_SYNC_CREATE);
  ctk_grid_attach (CTK_GRID (grid), label , 1, 1, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), check, 2, 1, 1, 1);

  label = ctk_label_new ("Popover vexpand");
  check = ctk_check_button_new ();
  g_object_bind_property (check, "active", popover, "vexpand", G_BINDING_SYNC_CREATE);
  g_object_bind_property (check, "active", popover2, "vexpand", G_BINDING_SYNC_CREATE);
  ctk_grid_attach (CTK_GRID (grid), label , 1, 2, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), check, 2, 2, 1, 1);

  label = ctk_label_new ("Button direction");
  combo = ctk_combo_box_text_new ();
  ctk_combo_box_text_append (CTK_COMBO_BOX_TEXT (combo), "up", "Up");
  ctk_combo_box_text_append (CTK_COMBO_BOX_TEXT (combo), "down", "Down");
  ctk_combo_box_text_append (CTK_COMBO_BOX_TEXT (combo), "left", "Left");
  ctk_combo_box_text_append (CTK_COMBO_BOX_TEXT (combo), "right", "Right");
  ctk_combo_box_set_active (CTK_COMBO_BOX (combo), 1);
  g_object_bind_property (combo, "active", button, "direction", G_BINDING_SYNC_CREATE);
  g_object_bind_property (combo, "active", button2, "direction", G_BINDING_SYNC_CREATE);
  ctk_grid_attach (CTK_GRID (grid), label , 1, 3, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), combo, 2, 3, 1, 1);

  label = ctk_label_new ("Button halign");
  combo = ctk_combo_box_text_new ();
  ctk_combo_box_text_append (CTK_COMBO_BOX_TEXT (combo), "fill", "Fill");
  ctk_combo_box_text_append (CTK_COMBO_BOX_TEXT (combo), "start", "Start");
  ctk_combo_box_text_append (CTK_COMBO_BOX_TEXT (combo), "end", "End");
  ctk_combo_box_text_append (CTK_COMBO_BOX_TEXT (combo), "center", "Center");
  ctk_combo_box_set_active (CTK_COMBO_BOX (combo), 2);
  g_object_bind_property (combo, "active", box, "halign", G_BINDING_SYNC_CREATE);
  ctk_grid_attach (CTK_GRID (grid), label , 1, 4, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), combo, 2, 4, 1, 1);

  label = ctk_label_new ("Button valign");
  combo = ctk_combo_box_text_new ();
  ctk_combo_box_text_append (CTK_COMBO_BOX_TEXT (combo), "fill", "Fill");
  ctk_combo_box_text_append (CTK_COMBO_BOX_TEXT (combo), "start", "Start");
  ctk_combo_box_text_append (CTK_COMBO_BOX_TEXT (combo), "end", "End");
  ctk_combo_box_text_append (CTK_COMBO_BOX_TEXT (combo), "center", "Center");
  ctk_combo_box_set_active (CTK_COMBO_BOX (combo), 1);
  g_object_bind_property (combo, "active", box, "valign", G_BINDING_SYNC_CREATE);
  ctk_grid_attach (CTK_GRID (grid), label , 1, 5, 1, 1);
  ctk_grid_attach (CTK_GRID (grid), combo, 2, 5, 1, 1);


  ctk_widget_show_all (win);

  ctk_main ();

  return 0;
}
