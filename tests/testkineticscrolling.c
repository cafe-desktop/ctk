#include <ctk/ctk.h>

enum
{
  TARGET_CTK_TREE_MODEL_ROW
};

static CtkTargetEntry row_targets[] =
{
  { "CTK_TREE_MODEL_ROW", CTK_TARGET_SAME_APP, TARGET_CTK_TREE_MODEL_ROW }
};

static void
on_button_clicked (CtkWidget *widget G_GNUC_UNUSED,
		   gpointer   data)
{
  g_print ("Button %d clicked\n", GPOINTER_TO_INT (data));
}

static void
kinetic_scrolling (void)
{
  CtkWidget *window, *swindow, *grid;
  CtkWidget *label;
  CtkWidget *button_grid;
  CtkWidget *treeview;
  CtkCellRenderer *renderer;
  CtkListStore *store;
  CtkWidget *textview;
  gint i;

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_container_set_border_width (CTK_CONTAINER (window), 5);
  ctk_window_set_default_size (CTK_WINDOW (window), 400, 400);
  g_signal_connect (window, "delete_event",
                    G_CALLBACK (ctk_main_quit), NULL);

  grid = ctk_grid_new ();

  label = ctk_label_new ("Non scrollable widget using viewport");
  ctk_grid_attach (CTK_GRID (grid), label, 0, 0, 1, 1);
  ctk_widget_set_hexpand (label, TRUE);
  ctk_widget_show (label);

  label = ctk_label_new ("Scrollable widget: TreeView");
  ctk_grid_attach (CTK_GRID (grid), label, 1, 0, 1, 1);
  ctk_widget_set_hexpand (label, TRUE);
  ctk_widget_show (label);

  label = ctk_label_new ("Scrollable widget: TextView");
  ctk_grid_attach (CTK_GRID (grid), label, 2, 0, 1, 1);
  ctk_widget_set_hexpand (label, TRUE);
  ctk_widget_show (label);

  button_grid = ctk_grid_new ();
  for (i = 0; i < 80; i++)
    {
      CtkWidget *button;

      gchar *label = g_strdup_printf ("Button number %d", i);

      button = ctk_button_new_with_label (label);
      ctk_grid_attach (CTK_GRID (button_grid), button, 0, i, 1, 1);
      ctk_widget_set_hexpand (button, TRUE);
      ctk_widget_show (button);
      g_signal_connect (button, "clicked",
                        G_CALLBACK (on_button_clicked),
                        GINT_TO_POINTER (i));
      g_free (label);
    }

  swindow = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_kinetic_scrolling (CTK_SCROLLED_WINDOW (swindow), TRUE);
  ctk_scrolled_window_set_capture_button_press (CTK_SCROLLED_WINDOW (swindow), TRUE);
  ctk_container_add (CTK_CONTAINER (swindow), button_grid);
  ctk_widget_show (button_grid);

  ctk_grid_attach (CTK_GRID (grid), swindow, 0, 1, 1, 1);
  ctk_widget_show (swindow);

  treeview = ctk_tree_view_new ();
  ctk_tree_view_enable_model_drag_source (CTK_TREE_VIEW (treeview),
                                          CDK_BUTTON1_MASK,
                                          row_targets,
                                          G_N_ELEMENTS (row_targets),
                                          CDK_ACTION_MOVE | CDK_ACTION_COPY);
  ctk_tree_view_enable_model_drag_dest (CTK_TREE_VIEW (treeview),
                                        row_targets,
                                        G_N_ELEMENTS (row_targets),
                                        CDK_ACTION_MOVE | CDK_ACTION_COPY);

  renderer = ctk_cell_renderer_text_new ();
  g_object_set (renderer, "editable", TRUE, NULL);
  ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (treeview),
                                               0, "Title",
                                               renderer,
                                               "text", 0,
                                               NULL);
  store = ctk_list_store_new (1, G_TYPE_STRING);
  for (i = 0; i < 80; i++)
    {
      CtkTreeIter iter;
      gchar *label = g_strdup_printf ("Row number %d", i);

      ctk_list_store_append (store, &iter);
      ctk_list_store_set (store, &iter, 0, label, -1);
      g_free (label);
    }
  ctk_tree_view_set_model (CTK_TREE_VIEW (treeview), CTK_TREE_MODEL (store));
  g_object_unref (store);

  swindow = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_kinetic_scrolling (CTK_SCROLLED_WINDOW (swindow), TRUE);
  ctk_scrolled_window_set_capture_button_press (CTK_SCROLLED_WINDOW (swindow), TRUE);
  ctk_container_add (CTK_CONTAINER (swindow), treeview);
  ctk_widget_show (treeview);

  ctk_grid_attach (CTK_GRID (grid), swindow, 1, 1, 1, 1);
  ctk_widget_set_hexpand (swindow, TRUE);
  ctk_widget_set_vexpand (swindow, TRUE);
  ctk_widget_show (swindow);

  textview = ctk_text_view_new ();
  swindow = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_kinetic_scrolling (CTK_SCROLLED_WINDOW (swindow), TRUE);
  ctk_scrolled_window_set_capture_button_press (CTK_SCROLLED_WINDOW (swindow), TRUE);
  ctk_container_add (CTK_CONTAINER (swindow), textview);
  ctk_widget_show (textview);

  ctk_grid_attach (CTK_GRID (grid), swindow, 2, 1, 1, 1);
  ctk_widget_set_hexpand (swindow, TRUE);
  ctk_widget_set_vexpand (swindow, TRUE);
  ctk_widget_show (swindow);

  ctk_container_add (CTK_CONTAINER (window), grid);
  ctk_widget_show (grid);

  ctk_widget_show (window);
}

int
main (int    argc G_GNUC_UNUSED,
      char **argv G_GNUC_UNUSED)
{
  ctk_init (NULL, NULL);

  kinetic_scrolling ();

  ctk_main ();

  return 0;
}
