/* Tree View/List Store
 *
 * The CtkListStore is used to store data in list form, to be used
 * later on by a CtkTreeView to display it. This demo builds a
 * simple CtkListStore and displays it.
 *
 */

#include <ctk/ctk.h>

static CtkWidget *window = NULL;
static CtkTreeModel *model = NULL;
static guint timeout = 0;

typedef struct
{
  const gboolean  fixed;
  const guint     number;
  const gchar    *severity;
  const gchar    *description;
}
Bug;

enum
{
  COLUMN_FIXED,
  COLUMN_NUMBER,
  COLUMN_SEVERITY,
  COLUMN_DESCRIPTION,
  COLUMN_PULSE,
  COLUMN_ICON,
  COLUMN_ACTIVE,
  COLUMN_SENSITIVE,
  NUM_COLUMNS
};

static Bug data[] =
{
  { FALSE, 60482, "Normal",     "scrollable notebooks and hidden tabs" },
  { FALSE, 60620, "Critical",   "cdk_window_clear_area (cdkwindow-win32.c) is not thread-safe" },
  { FALSE, 50214, "Major",      "Xft support does not clean up correctly" },
  { TRUE,  52877, "Major",      "CtkFileSelection needs a refresh method. " },
  { FALSE, 56070, "Normal",     "Can't click button after setting in sensitive" },
  { TRUE,  56355, "Normal",     "CtkLabel - Not all changes propagate correctly" },
  { FALSE, 50055, "Normal",     "Rework width/height computations for TreeView" },
  { FALSE, 58278, "Normal",     "ctk_dialog_set_response_sensitive () doesn't work" },
  { FALSE, 55767, "Normal",     "Getters for all setters" },
  { FALSE, 56925, "Normal",     "Ctkcalender size" },
  { FALSE, 56221, "Normal",     "Selectable label needs right-click copy menu" },
  { TRUE,  50939, "Normal",     "Add shift clicking to CtkTextView" },
  { FALSE, 6112,  "Enhancement","netscape-like collapsable toolbars" },
  { FALSE, 1,     "Normal",     "First bug :=)" },
};

static gboolean
spinner_timeout (gpointer data G_GNUC_UNUSED)
{
  CtkTreeIter iter;
  guint pulse;

  if (model == NULL)
    return G_SOURCE_REMOVE;

  ctk_tree_model_get_iter_first (model, &iter);
  ctk_tree_model_get (model, &iter,
                      COLUMN_PULSE, &pulse,
                      -1);
  if (pulse == G_MAXUINT)
    pulse = 0;
  else
    pulse++;

  ctk_list_store_set (CTK_LIST_STORE (model),
                      &iter,
                      COLUMN_PULSE, pulse,
                      COLUMN_ACTIVE, TRUE,
                      -1);

  return G_SOURCE_CONTINUE;
}

static CtkTreeModel *
create_model (void)
{
  gint i = 0;
  CtkListStore *store;
  CtkTreeIter iter;

  /* create list store */
  store = ctk_list_store_new (NUM_COLUMNS,
                              G_TYPE_BOOLEAN,
                              G_TYPE_UINT,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_UINT,
                              G_TYPE_STRING,
                              G_TYPE_BOOLEAN,
                              G_TYPE_BOOLEAN);

  /* add data to the list store */
  for (i = 0; i < G_N_ELEMENTS (data); i++)
    {
      gchar *icon_name;
      gboolean sensitive;

      if (i == 1 || i == 3)
        icon_name = "battery-caution-charging-symbolic";
      else
        icon_name = NULL;
      if (i == 3)
        sensitive = FALSE;
      else
        sensitive = TRUE;
      ctk_list_store_append (store, &iter);
      ctk_list_store_set (store, &iter,
                          COLUMN_FIXED, data[i].fixed,
                          COLUMN_NUMBER, data[i].number,
                          COLUMN_SEVERITY, data[i].severity,
                          COLUMN_DESCRIPTION, data[i].description,
                          COLUMN_PULSE, 0,
                          COLUMN_ICON, icon_name,
                          COLUMN_ACTIVE, FALSE,
                          COLUMN_SENSITIVE, sensitive,
                          -1);
    }

  return CTK_TREE_MODEL (store);
}

static void
fixed_toggled (CtkCellRendererToggle *cell G_GNUC_UNUSED,
               gchar                 *path_str,
               gpointer               data)
{
  CtkTreeModel *model = (CtkTreeModel *)data;
  CtkTreeIter  iter;
  CtkTreePath *path = ctk_tree_path_new_from_string (path_str);
  gboolean fixed;

  /* get toggled iter */
  ctk_tree_model_get_iter (model, &iter, path);
  ctk_tree_model_get (model, &iter, COLUMN_FIXED, &fixed, -1);

  /* do something with the value */
  fixed ^= 1;

  /* set new value */
  ctk_list_store_set (CTK_LIST_STORE (model), &iter, COLUMN_FIXED, fixed, -1);

  /* clean up */
  ctk_tree_path_free (path);
}

static void
add_columns (CtkTreeView *treeview)
{
  CtkCellRenderer *renderer;
  CtkTreeViewColumn *column;
  CtkTreeModel *model = ctk_tree_view_get_model (treeview);

  /* column for fixed toggles */
  renderer = ctk_cell_renderer_toggle_new ();
  g_signal_connect (renderer, "toggled",
                    G_CALLBACK (fixed_toggled), model);

  column = ctk_tree_view_column_new_with_attributes ("Fixed?",
                                                     renderer,
                                                     "active", COLUMN_FIXED,
                                                     NULL);

  /* set this column to a fixed sizing (of 50 pixels) */
  ctk_tree_view_column_set_sizing (CTK_TREE_VIEW_COLUMN (column),
                                   CTK_TREE_VIEW_COLUMN_FIXED);
  ctk_tree_view_column_set_fixed_width (CTK_TREE_VIEW_COLUMN (column), 50);
  ctk_tree_view_append_column (treeview, column);

  /* column for bug numbers */
  renderer = ctk_cell_renderer_text_new ();
  column = ctk_tree_view_column_new_with_attributes ("Bug number",
                                                     renderer,
                                                     "text",
                                                     COLUMN_NUMBER,
                                                     NULL);
  ctk_tree_view_column_set_sort_column_id (column, COLUMN_NUMBER);
  ctk_tree_view_append_column (treeview, column);

  /* column for severities */
  renderer = ctk_cell_renderer_text_new ();
  column = ctk_tree_view_column_new_with_attributes ("Severity",
                                                     renderer,
                                                     "text",
                                                     COLUMN_SEVERITY,
                                                     NULL);
  ctk_tree_view_column_set_sort_column_id (column, COLUMN_SEVERITY);
  ctk_tree_view_append_column (treeview, column);

  /* column for description */
  renderer = ctk_cell_renderer_text_new ();
  column = ctk_tree_view_column_new_with_attributes ("Description",
                                                     renderer,
                                                     "text",
                                                     COLUMN_DESCRIPTION,
                                                     NULL);
  ctk_tree_view_column_set_sort_column_id (column, COLUMN_DESCRIPTION);
  ctk_tree_view_append_column (treeview, column);

  /* column for spinner */
  renderer = ctk_cell_renderer_spinner_new ();
  column = ctk_tree_view_column_new_with_attributes ("Spinning",
                                                     renderer,
                                                     "pulse",
                                                     COLUMN_PULSE,
                                                     "active",
                                                     COLUMN_ACTIVE,
                                                     NULL);
  ctk_tree_view_column_set_sort_column_id (column, COLUMN_PULSE);
  ctk_tree_view_append_column (treeview, column);

  /* column for symbolic icon */
  renderer = ctk_cell_renderer_pixbuf_new ();
  column = ctk_tree_view_column_new_with_attributes ("Symbolic icon",
                                                     renderer,
                                                     "icon-name",
                                                     COLUMN_ICON,
                                                     "sensitive",
                                                     COLUMN_SENSITIVE,
                                                     NULL);
  ctk_tree_view_column_set_sort_column_id (column, COLUMN_ICON);
  ctk_tree_view_append_column (treeview, column);
}

static gboolean
window_closed (CtkWidget *widget G_GNUC_UNUSED,
               CdkEvent  *event G_GNUC_UNUSED,
               gpointer   user_data G_GNUC_UNUSED)
{
  model = NULL;
  window = NULL;
  if (timeout != 0)
    {
      g_source_remove (timeout);
      timeout = 0;
    }
  return FALSE;
}

CtkWidget *
do_list_store (CtkWidget *do_widget)
{
  if (!window)
    {
      CtkWidget *vbox;
      CtkWidget *label;
      CtkWidget *sw;
      CtkWidget *treeview;

      /* create window, etc */
      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_screen (CTK_WINDOW (window),
                             ctk_widget_get_screen (do_widget));
      ctk_window_set_title (CTK_WINDOW (window), "List Store");

      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);
      ctk_container_set_border_width (CTK_CONTAINER (window), 8);

      vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 8);
      ctk_container_add (CTK_CONTAINER (window), vbox);

      label = ctk_label_new ("This is the bug list (note: not based on real data, it would be nice to have a nice ODBC interface to bugzilla or so, though).");
      ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, FALSE, 0);

      sw = ctk_scrolled_window_new (NULL, NULL);
      ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (sw),
                                           CTK_SHADOW_ETCHED_IN);
      ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (sw),
                                      CTK_POLICY_NEVER,
                                      CTK_POLICY_AUTOMATIC);
      ctk_box_pack_start (CTK_BOX (vbox), sw, TRUE, TRUE, 0);

      /* create tree model */
      model = create_model ();

      /* create tree view */
      treeview = ctk_tree_view_new_with_model (model);
      ctk_tree_view_set_search_column (CTK_TREE_VIEW (treeview),
                                       COLUMN_DESCRIPTION);

      g_object_unref (model);

      ctk_container_add (CTK_CONTAINER (sw), treeview);

      /* add columns to the tree view */
      add_columns (CTK_TREE_VIEW (treeview));

      /* finish & show */
      ctk_window_set_default_size (CTK_WINDOW (window), 280, 250);
      g_signal_connect (window, "delete-event",
                        G_CALLBACK (window_closed), NULL);
    }

  if (!ctk_widget_get_visible (window))
    {
      ctk_widget_show_all (window);
      if (timeout == 0) {
        /* FIXME this should use the animation-duration instead */
        timeout = g_timeout_add (80, spinner_timeout, NULL);
      }
    }
  else
    {
      ctk_widget_destroy (window);
      window = NULL;
      if (timeout != 0)
        {
          g_source_remove (timeout);
          timeout = 0;
        }
    }

  return window;
}
