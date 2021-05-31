#include <gtk/gtk.h>

typedef struct {
  GtkListBoxRow parent;
  GtkWidget *box;
  GtkWidget *revealer;
  GtkWidget *check;
} SelectableRow;

typedef struct {
  GtkListBoxRowClass parent_class;
} SelectableRowClass;

G_DEFINE_TYPE (SelectableRow, selectable_row, GTK_TYPE_LIST_BOX_ROW)

static void
selectable_row_init (SelectableRow *row)
{
  row->box = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);  
  row->revealer = ctk_revealer_new ();
  ctk_revealer_set_transition_type (GTK_REVEALER (row->revealer), GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT);
  row->check = ctk_check_button_new ();
  g_object_set (row->check, "margin", 10, NULL);

  ctk_widget_show (row->box);
  ctk_widget_show (row->check);

  ctk_container_add (GTK_CONTAINER (row), row->box);
  ctk_container_add (GTK_CONTAINER (row->box), row->revealer);
  ctk_container_add (GTK_CONTAINER (row->revealer), row->check);
}

void
selectable_row_add (SelectableRow *row, GtkWidget *child)
{
  ctk_container_add (GTK_CONTAINER (row->box), child);
}

static void
update_selectable (GtkWidget *widget, gpointer data)
{
  SelectableRow *row = (SelectableRow *)widget;
  GtkListBox *list;

  list = GTK_LIST_BOX (ctk_widget_get_parent (widget));

  if (ctk_list_box_get_selection_mode (list) != GTK_SELECTION_NONE)
    ctk_revealer_set_reveal_child (GTK_REVEALER (row->revealer), TRUE);
  else
    ctk_revealer_set_reveal_child (GTK_REVEALER (row->revealer), FALSE);
}

static void
update_selected (GtkWidget *widget, gpointer data)
{
  SelectableRow *row = (SelectableRow *)widget;

  if (ctk_list_box_row_is_selected (GTK_LIST_BOX_ROW (row)))
    {
      ctk_toggle_button_set_active (GTK_TOGGLE_BUTTON (row->check), TRUE);
      ctk_widget_unset_state_flags (widget, GTK_STATE_FLAG_SELECTED);
    }
  else
    ctk_toggle_button_set_active (GTK_TOGGLE_BUTTON (row->check), FALSE);
}

static void
selectable_row_class_init (SelectableRowClass *class)
{
}

GtkWidget *
selectable_row_new (void)
{
  return GTK_WIDGET (g_object_new (selectable_row_get_type (), NULL));
}

static void
add_row (GtkWidget *list, gint i)
{
  GtkWidget *row;
  GtkWidget *label;
  gchar *text;

  row = selectable_row_new ();
  text = g_strdup_printf ("Docker %d", i);
  label = ctk_label_new (text);
  ctk_widget_set_halign (label, GTK_ALIGN_START);
  selectable_row_add ((SelectableRow*)row, label);
  g_free (text);

  ctk_list_box_insert (GTK_LIST_BOX (list), row, -1);
}

static void
selection_mode_enter (GtkButton *button, GtkBuilder *builder)
{
  GtkWidget *header;
  GtkWidget *list;
  GtkWidget *headerbutton;
  GtkWidget *cancelbutton;
  GtkWidget *selectbutton;
  GtkWidget *titlestack;

  header = GTK_WIDGET (ctk_builder_get_object (builder, "header"));
  list = GTK_WIDGET (ctk_builder_get_object (builder, "list"));
  headerbutton = GTK_WIDGET (ctk_builder_get_object (builder, "headerbutton"));
  cancelbutton = GTK_WIDGET (ctk_builder_get_object (builder, "cancel-button"));
  selectbutton = GTK_WIDGET (ctk_builder_get_object (builder, "select-button"));
  titlestack = GTK_WIDGET (ctk_builder_get_object (builder, "titlestack"));
 
  ctk_style_context_add_class (ctk_widget_get_style_context (header), "selection-mode");
  ctk_header_bar_set_show_close_button (GTK_HEADER_BAR (header), FALSE);
  ctk_widget_hide (headerbutton);
  ctk_widget_hide (selectbutton);
  ctk_widget_show (cancelbutton);
  ctk_stack_set_visible_child_name (GTK_STACK (titlestack), "selection");

  ctk_list_box_set_activate_on_single_click (GTK_LIST_BOX (list), FALSE);
  ctk_list_box_set_selection_mode (GTK_LIST_BOX (list), GTK_SELECTION_MULTIPLE);
  ctk_container_forall (GTK_CONTAINER (list), update_selectable, NULL);
}

static void
selection_mode_leave (GtkButton *button, GtkBuilder *builder)
{
  GtkWidget *header;
  GtkWidget *list;
  GtkWidget *headerbutton;
  GtkWidget *cancelbutton;
  GtkWidget *selectbutton;
  GtkWidget *titlestack;

  header = GTK_WIDGET (ctk_builder_get_object (builder, "header"));
  list = GTK_WIDGET (ctk_builder_get_object (builder, "list"));
  headerbutton = GTK_WIDGET (ctk_builder_get_object (builder, "headerbutton"));
  cancelbutton = GTK_WIDGET (ctk_builder_get_object (builder, "cancel-button"));
  selectbutton = GTK_WIDGET (ctk_builder_get_object (builder, "select-button"));
  titlestack = GTK_WIDGET (ctk_builder_get_object (builder, "titlestack"));

  ctk_style_context_remove_class (ctk_widget_get_style_context (header), "selection-mode");
  ctk_header_bar_set_show_close_button (GTK_HEADER_BAR (header), TRUE);
  ctk_widget_show (headerbutton);
  ctk_widget_show (selectbutton);
  ctk_widget_hide (cancelbutton);
  ctk_stack_set_visible_child_name (GTK_STACK (titlestack), "title");

  ctk_list_box_set_activate_on_single_click (GTK_LIST_BOX (list), TRUE);
  ctk_list_box_set_selection_mode (GTK_LIST_BOX (list), GTK_SELECTION_NONE);
  ctk_container_forall (GTK_CONTAINER (list), update_selectable, NULL);
}

static void
select_all (GAction *action, GVariant *param, GtkWidget *list)
{
  ctk_list_box_select_all (GTK_LIST_BOX (list));
}

static void
select_none (GAction *action, GVariant *param, GtkWidget *list)
{
  ctk_list_box_unselect_all (GTK_LIST_BOX (list));
}

static void
selected_rows_changed (GtkListBox *list)
{
  ctk_container_forall (GTK_CONTAINER (list), update_selected, NULL);
}

int
main (int argc, char *argv[])
{
  GtkBuilder *builder;
  GtkWidget *window;
  GtkWidget *list;
  GtkWidget *button;
  gint i;
  GSimpleActionGroup *group;
  GSimpleAction *action;

  ctk_init (NULL, NULL);

  builder = ctk_builder_new_from_file ("selectionmode.ui");
  window = GTK_WIDGET (ctk_builder_get_object (builder, "window"));
  list = GTK_WIDGET (ctk_builder_get_object (builder, "list"));

  group = g_simple_action_group_new ();
  action = g_simple_action_new ("select-all", NULL);
  g_signal_connect (action, "activate", G_CALLBACK (select_all), list);
  g_action_map_add_action (G_ACTION_MAP (group), G_ACTION (action));

  action = g_simple_action_new ("select-none", NULL);
  g_signal_connect (action, "activate", G_CALLBACK (select_none), list);
  g_action_map_add_action (G_ACTION_MAP (group), G_ACTION (action));

  ctk_widget_insert_action_group (window, "win", G_ACTION_GROUP (group));

  for (i = 0; i < 10; i++)
    add_row (list, i);

  button = GTK_WIDGET (ctk_builder_get_object (builder, "select-button"));
  g_signal_connect (button, "clicked", G_CALLBACK (selection_mode_enter), builder);
  button = GTK_WIDGET (ctk_builder_get_object (builder, "cancel-button"));
  g_signal_connect (button, "clicked", G_CALLBACK (selection_mode_leave), builder);

  g_signal_connect (list, "selected-rows-changed", G_CALLBACK (selected_rows_changed), NULL);

  ctk_widget_show_all (window);
 
  ctk_main ();

  return 0;
}
