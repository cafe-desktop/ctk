#include <ctk/ctk.h>

typedef struct
{
  CtkApplication parent_instance;
} TestApp;

typedef CtkApplicationClass TestAppClass;

G_DEFINE_TYPE (TestApp, test_app, CTK_TYPE_APPLICATION)

static CtkWidget *create_row (const gchar *label);

static void
activate_first_row (GSimpleAction *simple,
                    GVariant      *parameter,
                    gpointer       user_data)
{
  const gchar *text = "First row activated (no parameter action)";

  g_print ("%s\n", text);
  ctk_label_set_label (CTK_LABEL (user_data), text);
}

static void
activate_print_string (GSimpleAction *simple,
                       GVariant      *parameter,
                       gpointer       user_data)
{
  const gchar *text = g_variant_get_string (parameter, NULL);

  g_print ("%s\n", text);
  ctk_label_set_label (CTK_LABEL (user_data), text);
}

static void
activate_print_int (GSimpleAction *simple,
                    GVariant      *parameter,
                    gpointer       user_data)
{
  const int value = g_variant_get_int32 (parameter);
  gchar *text;

  text = g_strdup_printf ("Row %d activated (int action)", value);

  g_print ("%s\n", text);
  ctk_label_set_label (CTK_LABEL (user_data), text);
}

static void
row_without_gaction_activated_cb (CtkListBox    *list,
                                  CtkListBoxRow *row,
                                  gpointer       user_data)
{
  int index = ctk_list_box_row_get_index (row);
  gchar *text;

  text = g_strdup_printf ("Row %d activated (signal based)", index);

  g_print ("%s\n", text);
  ctk_label_set_label (CTK_LABEL (user_data), text);
}

static void
add_separator (CtkListBoxRow *row, CtkListBoxRow *before, gpointer data)
{
  if (!before)
    return;

  ctk_list_box_row_set_header (row, ctk_separator_new (CTK_ORIENTATION_HORIZONTAL));
}

static CtkWidget *
create_row (const gchar *text)
{
  CtkWidget *row_content, *label;

  row_content = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);

  label = ctk_label_new (text);
  ctk_container_add (CTK_CONTAINER (row_content), label);

  return row_content;
}

static void
new_window (GApplication *app)
{
  CtkWidget *window, *grid, *sw, *list, *label;
  GSimpleAction *action;

  CtkWidget *row_content;
  CtkListBoxRow *row;

  gint i;
  gchar *text;

  window = ctk_application_window_new (CTK_APPLICATION (app));
  ctk_window_set_default_size (CTK_WINDOW (window), 300, 300);

  /* widget creation */
  grid = ctk_grid_new ();
  ctk_container_add (CTK_CONTAINER (window), grid);
  sw = ctk_scrolled_window_new (NULL, NULL);
  ctk_widget_set_hexpand (CTK_WIDGET (sw), 1);
  ctk_widget_set_vexpand (CTK_WIDGET (sw), 1);
  ctk_grid_attach (CTK_GRID (grid), sw, 0, 0, 1, 1);

  list = ctk_list_box_new ();
  ctk_list_box_set_selection_mode (CTK_LIST_BOX (list), CTK_SELECTION_NONE);
  ctk_list_box_set_header_func (CTK_LIST_BOX (list), add_separator, NULL, NULL);
  ctk_container_add (CTK_CONTAINER (sw), list);

  label = ctk_label_new ("No row activated");
  ctk_grid_attach (CTK_GRID (grid), label, 0, 1, 1, 1);

  /* no parameter action row */
  action = g_simple_action_new ("first-row-action", NULL);
  g_action_map_add_action (G_ACTION_MAP (window), G_ACTION (action));

  row_content = create_row ("First row (no parameter action)");
  ctk_list_box_insert (CTK_LIST_BOX (list), row_content, -1);

  row = ctk_list_box_get_row_at_index (CTK_LIST_BOX (list), 0);
  ctk_actionable_set_action_name (CTK_ACTIONABLE (row), "win.first-row-action");

  g_signal_connect (action, "activate", (GCallback) activate_first_row, label);

  /* string action rows */
  action = g_simple_action_new ("print-string", G_VARIANT_TYPE_STRING);
  g_action_map_add_action (G_ACTION_MAP (window), G_ACTION (action));

  for (i = 1; i < 3; i++)
    {
      gchar *text2;

      text = g_strdup_printf ("Row %d (string action)", i);
      row_content = create_row (text);
      ctk_list_box_insert (CTK_LIST_BOX (list), row_content, -1);

      row = ctk_list_box_get_row_at_index (CTK_LIST_BOX (list), i);
      text2 = g_strdup_printf ("Row %d activated (string action)", i);
      ctk_actionable_set_action_target (CTK_ACTIONABLE (row), "s", text2);
      ctk_actionable_set_action_name (CTK_ACTIONABLE (row), "win.print-string");
    }

  g_signal_connect (action, "activate", (GCallback) activate_print_string, label);

  /* int action rows */
  action = g_simple_action_new ("print-int", G_VARIANT_TYPE_INT32);
  g_action_map_add_action (G_ACTION_MAP (window), G_ACTION (action));

  for (i = 3; i < 5; i++)
    {
      text = g_strdup_printf ("Row %d (int action)", i);
      row_content = create_row (text);
      ctk_list_box_insert (CTK_LIST_BOX (list), row_content, -1);

      row = ctk_list_box_get_row_at_index (CTK_LIST_BOX (list), i);
      ctk_actionable_set_action_target (CTK_ACTIONABLE (row), "i", i);
      ctk_actionable_set_action_name (CTK_ACTIONABLE (row), "win.print-int");
    }

  g_signal_connect (action, "activate", (GCallback) activate_print_int, label);

  /* signal based row */
  for (i = 5; i < 7; i++)
    {
      text = g_strdup_printf ("Row %d (signal based)", i);
      row_content = create_row (text);
      ctk_list_box_insert (CTK_LIST_BOX (list), row_content, -1);
    }

  g_signal_connect (list, "row-activated",
                    G_CALLBACK (row_without_gaction_activated_cb), label);

  /* let the show begin */
  ctk_widget_show_all (CTK_WIDGET (window));
}

static void
test_app_activate (GApplication *application)
{
  new_window (application);
}

static void
test_app_init (TestApp *app)
{
}

static void
test_app_class_init (TestAppClass *class)
{
  G_APPLICATION_CLASS (class)->activate = test_app_activate;
}

TestApp *
test_app_new (void)
{
  TestApp *test_app;

  g_set_application_name ("Test List 4");

  test_app = g_object_new (test_app_get_type (),
                           "application-id", "org.ctk.testlist4",
                           "flags", G_APPLICATION_FLAGS_NONE,
                           NULL);

  return test_app;
}

int
main (int argc, char **argv)
{
  TestApp *test_app;
  int status;

  test_app = test_app_new ();
  status = g_application_run (G_APPLICATION (test_app), argc, argv);

  g_object_unref (test_app);
  return status;
}
