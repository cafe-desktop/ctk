/* Tree View/Tree Store
 *
 * The GtkTreeStore is used to store data in tree form, to be
 * used later on by a GtkTreeView to display it. This demo builds
 * a simple GtkTreeStore and displays it. If you're new to the
 * GtkTreeView widgets and associates, look into the GtkListStore
 * example first.
 *
 */

#include <ctk/ctk.h>

/* TreeItem structure */
typedef struct _TreeItem TreeItem;
struct _TreeItem
{
  const gchar    *label;
  gboolean        alex;
  gboolean        havoc;
  gboolean        tim;
  gboolean        owen;
  gboolean        dave;
  gboolean        world_holiday; /* shared by the European hackers */
  TreeItem       *children;
};

/* columns */
enum
{
  HOLIDAY_NAME_COLUMN = 0,
  ALEX_COLUMN,
  HAVOC_COLUMN,
  TIM_COLUMN,
  OWEN_COLUMN,
  DAVE_COLUMN,

  VISIBLE_COLUMN,
  WORLD_COLUMN,
  NUM_COLUMNS
};

/* tree data */
static TreeItem january[] =
{
  {"New Years Day", TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, NULL },
  {"Presidential Inauguration", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  {"Martin Luther King Jr. day", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  { NULL }
};

static TreeItem february[] =
{
  { "Presidents' Day", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  { "Groundhog Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Valentine's Day", FALSE, FALSE, FALSE, FALSE, TRUE, TRUE, NULL },
  { NULL }
};

static TreeItem march[] =
{
  { "National Tree Planting Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "St Patrick's Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { NULL }
};
static TreeItem april[] =
{
  { "April Fools' Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { "Army Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Earth Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { "Administrative Professionals' Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { NULL }
};

static TreeItem may[] =
{
  { "Nurses' Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "National Day of Prayer", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Mothers' Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { "Armed Forces Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Memorial Day", TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, NULL },
  { NULL }
};

static TreeItem june[] =
{
  { "June Fathers' Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { "Juneteenth (Liberation of Slaves)", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Flag Day", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  { NULL }
};

static TreeItem july[] =
{
  { "Parents' Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { "Independence Day", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  { NULL }
};

static TreeItem august[] =
{
  { "Air Force Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Coast Guard Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Friendship Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { NULL }
};

static TreeItem september[] =
{
  { "Grandparents' Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { "Citizenship Day or Constitution Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Labor Day", TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, NULL },
  { NULL }
};

static TreeItem october[] =
{
  { "National Children's Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Bosses' Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Sweetest Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Mother-in-Law's Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Navy Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Columbus Day", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  { "Halloween", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { NULL }
};

static TreeItem november[] =
{
  { "Marine Corps Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Veterans' Day", TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, NULL },
  { "Thanksgiving", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  { NULL }
};

static TreeItem december[] =
{
  { "Pearl Harbor Remembrance Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Christmas", TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, NULL },
  { "Kwanzaa", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { NULL }
};


static TreeItem toplevel[] =
{
  {"January", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, january},
  {"February", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, february},
  {"March", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, march},
  {"April", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, april},
  {"May", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, may},
  {"June", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, june},
  {"July", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, july},
  {"August", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, august},
  {"September", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, september},
  {"October", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, october},
  {"November", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, november},
  {"December", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, december},
  {NULL}
};


static GtkTreeModel *
create_model (void)
{
  GtkTreeStore *model;
  GtkTreeIter iter;
  TreeItem *month = toplevel;

  /* create tree store */
  model = ctk_tree_store_new (NUM_COLUMNS,
                              G_TYPE_STRING,
                              G_TYPE_BOOLEAN,
                              G_TYPE_BOOLEAN,
                              G_TYPE_BOOLEAN,
                              G_TYPE_BOOLEAN,
                              G_TYPE_BOOLEAN,
                              G_TYPE_BOOLEAN,
                              G_TYPE_BOOLEAN);

  /* add data to the tree store */
  while (month->label)
    {
      TreeItem *holiday = month->children;

      ctk_tree_store_append (model, &iter, NULL);
      ctk_tree_store_set (model, &iter,
                          HOLIDAY_NAME_COLUMN, month->label,
                          ALEX_COLUMN, FALSE,
                          HAVOC_COLUMN, FALSE,
                          TIM_COLUMN, FALSE,
                          OWEN_COLUMN, FALSE,
                          DAVE_COLUMN, FALSE,
                          VISIBLE_COLUMN, FALSE,
                          WORLD_COLUMN, FALSE,
                          -1);

      /* add children */
      while (holiday->label)
        {
          GtkTreeIter child_iter;

          ctk_tree_store_append (model, &child_iter, &iter);
          ctk_tree_store_set (model, &child_iter,
                              HOLIDAY_NAME_COLUMN, holiday->label,
                              ALEX_COLUMN, holiday->alex,
                              HAVOC_COLUMN, holiday->havoc,
                              TIM_COLUMN, holiday->tim,
                              OWEN_COLUMN, holiday->owen,
                              DAVE_COLUMN, holiday->dave,
                              VISIBLE_COLUMN, TRUE,
                              WORLD_COLUMN, holiday->world_holiday,
                              -1);

          holiday++;
        }

      month++;
    }

  return CTK_TREE_MODEL (model);
}

static void
item_toggled (GtkCellRendererToggle *cell,
              gchar                 *path_str,
              gpointer               data)
{
  GtkTreeModel *model = (GtkTreeModel *)data;
  GtkTreePath *path = ctk_tree_path_new_from_string (path_str);
  GtkTreeIter iter;
  gboolean toggle_item;

  gint *column;

  column = g_object_get_data (G_OBJECT (cell), "column");

  /* get toggled iter */
  ctk_tree_model_get_iter (model, &iter, path);
  ctk_tree_model_get (model, &iter, column, &toggle_item, -1);

  /* do something with the value */
  toggle_item ^= 1;

  /* set new value */
  ctk_tree_store_set (CTK_TREE_STORE (model), &iter, column,
                      toggle_item, -1);

  /* clean up */
  ctk_tree_path_free (path);
}

static void
add_columns (GtkTreeView *treeview)
{
  gint col_offset;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeModel *model = ctk_tree_view_get_model (treeview);

  /* column for holiday names */
  renderer = ctk_cell_renderer_text_new ();
  g_object_set (renderer, "xalign", 0.0, NULL);

  col_offset = ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (treeview),
                                                            -1, "Holiday",
                                                            renderer, "text",
                                                            HOLIDAY_NAME_COLUMN,
                                                            NULL);
  column = ctk_tree_view_get_column (CTK_TREE_VIEW (treeview), col_offset - 1);
  ctk_tree_view_column_set_clickable (CTK_TREE_VIEW_COLUMN (column), TRUE);

  /* alex column */
  renderer = ctk_cell_renderer_toggle_new ();
  g_object_set (renderer, "xalign", 0.0, NULL);
  g_object_set_data (G_OBJECT (renderer), "column", (gint *)ALEX_COLUMN);

  g_signal_connect (renderer, "toggled", G_CALLBACK (item_toggled), model);

  col_offset = ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (treeview),
                                                            -1, "Alex",
                                                            renderer,
                                                            "active",
                                                            ALEX_COLUMN,
                                                            "visible",
                                                            VISIBLE_COLUMN,
                                                            "activatable",
                                                            WORLD_COLUMN, NULL);

  column = ctk_tree_view_get_column (CTK_TREE_VIEW (treeview), col_offset - 1);
  ctk_tree_view_column_set_sizing (CTK_TREE_VIEW_COLUMN (column),
                                   CTK_TREE_VIEW_COLUMN_FIXED);
  ctk_tree_view_column_set_clickable (CTK_TREE_VIEW_COLUMN (column), TRUE);

  /* havoc column */
  renderer = ctk_cell_renderer_toggle_new ();
  g_object_set (renderer, "xalign", 0.0, NULL);
  g_object_set_data (G_OBJECT (renderer), "column", (gint *)HAVOC_COLUMN);

  g_signal_connect (renderer, "toggled", G_CALLBACK (item_toggled), model);

  col_offset = ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (treeview),
                                                            -1, "Havoc",
                                                            renderer,
                                                            "active",
                                                            HAVOC_COLUMN,
                                                            "visible",
                                                            VISIBLE_COLUMN,
                                                            NULL);

  column = ctk_tree_view_get_column (CTK_TREE_VIEW (treeview), col_offset - 1);
  ctk_tree_view_column_set_sizing (CTK_TREE_VIEW_COLUMN (column),
                                   CTK_TREE_VIEW_COLUMN_FIXED);
  ctk_tree_view_column_set_clickable (CTK_TREE_VIEW_COLUMN (column), TRUE);

  /* tim column */
  renderer = ctk_cell_renderer_toggle_new ();
  g_object_set (renderer, "xalign", 0.0, NULL);
  g_object_set_data (G_OBJECT (renderer), "column", (gint *)TIM_COLUMN);

  g_signal_connect (renderer, "toggled", G_CALLBACK (item_toggled), model);

  col_offset = ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (treeview),
                                                            -1, "Tim",
                                                            renderer,
                                                            "active",
                                                            TIM_COLUMN,
                                                            "visible",
                                                            VISIBLE_COLUMN,
                                                            "activatable",
                                                            WORLD_COLUMN, NULL);

  column = ctk_tree_view_get_column (CTK_TREE_VIEW (treeview), col_offset - 1);
  ctk_tree_view_column_set_sizing (CTK_TREE_VIEW_COLUMN (column),
                                   CTK_TREE_VIEW_COLUMN_FIXED);
  ctk_tree_view_column_set_clickable (CTK_TREE_VIEW_COLUMN (column), TRUE);

  /* owen column */
  renderer = ctk_cell_renderer_toggle_new ();
  g_object_set (renderer, "xalign", 0.0, NULL);
  g_object_set_data (G_OBJECT (renderer), "column", (gint *)OWEN_COLUMN);

  g_signal_connect (renderer, "toggled", G_CALLBACK (item_toggled), model);

  col_offset = ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (treeview),
                                                            -1, "Owen",
                                                            renderer,
                                                            "active",
                                                            OWEN_COLUMN,
                                                            "visible",
                                                            VISIBLE_COLUMN,
                                                            NULL);

  column = ctk_tree_view_get_column (CTK_TREE_VIEW (treeview), col_offset - 1);
  ctk_tree_view_column_set_sizing (CTK_TREE_VIEW_COLUMN (column),
                                   CTK_TREE_VIEW_COLUMN_FIXED);
  ctk_tree_view_column_set_clickable (CTK_TREE_VIEW_COLUMN (column), TRUE);

  /* dave column */
  renderer = ctk_cell_renderer_toggle_new ();
  g_object_set (renderer, "xalign", 0.0, NULL);
  g_object_set_data (G_OBJECT (renderer), "column", (gint *)DAVE_COLUMN);

  g_signal_connect (renderer, "toggled", G_CALLBACK (item_toggled), model);

  col_offset = ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (treeview),
                                                            -1, "Dave",
                                                            renderer,
                                                            "active",
                                                            DAVE_COLUMN,
                                                            "visible",
                                                            VISIBLE_COLUMN,
                                                            NULL);

  column = ctk_tree_view_get_column (CTK_TREE_VIEW (treeview), col_offset - 1);
  ctk_tree_view_column_set_sizing (CTK_TREE_VIEW_COLUMN (column),
                                   CTK_TREE_VIEW_COLUMN_FIXED);
  ctk_tree_view_column_set_clickable (CTK_TREE_VIEW_COLUMN (column), TRUE);
}

GtkWidget *
do_tree_store (GtkWidget *do_widget)
{
  static GtkWidget *window = NULL;

  if (!window)
    {
      GtkWidget *vbox;
      GtkWidget *sw;
      GtkWidget *treeview;
      GtkTreeModel *model;

      /* create window, etc */
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window),
                             ctk_widget_get_screen (do_widget));
      ctk_window_set_title (CTK_WINDOW (window), "Tree Store");
      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);

      vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 8);
      ctk_container_set_border_width (CTK_CONTAINER (vbox), 8);
      ctk_container_add (CTK_CONTAINER (window), vbox);

      ctk_box_pack_start (CTK_BOX (vbox),
                          ctk_label_new ("Jonathan's Holiday Card Planning Sheet"),
                          FALSE, FALSE, 0);

      sw = ctk_scrolled_window_new (NULL, NULL);
      ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (sw),
                                           CTK_SHADOW_ETCHED_IN);
      ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (sw),
                                      CTK_POLICY_AUTOMATIC,
                                      CTK_POLICY_AUTOMATIC);
      ctk_box_pack_start (CTK_BOX (vbox), sw, TRUE, TRUE, 0);

      /* create model */
      model = create_model ();

      /* create tree view */
      treeview = ctk_tree_view_new_with_model (model);
      g_object_unref (model);
      ctk_tree_selection_set_mode (ctk_tree_view_get_selection (CTK_TREE_VIEW (treeview)),
                                   CTK_SELECTION_MULTIPLE);

      add_columns (CTK_TREE_VIEW (treeview));

      ctk_container_add (CTK_CONTAINER (sw), treeview);

      /* expand all rows after the treeview widget has been realized */
      g_signal_connect (treeview, "realize",
                        G_CALLBACK (ctk_tree_view_expand_all), NULL);
      ctk_window_set_default_size (CTK_WINDOW (window), 650, 400);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);

  return window;
}
