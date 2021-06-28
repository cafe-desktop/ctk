/* testtreeview.c
 * Copyright (C) 2001 Red Hat, Inc
 * Author: Jonathan Blandford
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <ctk/ctk.h>
#include <stdlib.h>

/* Don't copy this bad example; inline RGB data is always a better
 * idea than inline XPMs.
 */
static char  *book_closed_xpm[] = {
"16 16 6 1",
"       c None s None",
".      c black",
"X      c red",
"o      c yellow",
"O      c #808080",
"#      c white",
"                ",
"       ..       ",
"     ..XX.      ",
"   ..XXXXX.     ",
" ..XXXXXXXX.    ",
".ooXXXXXXXXX.   ",
"..ooXXXXXXXXX.  ",
".X.ooXXXXXXXXX. ",
".XX.ooXXXXXX..  ",
" .XX.ooXXX..#O  ",
"  .XX.oo..##OO. ",
"   .XX..##OO..  ",
"    .X.#OO..    ",
"     ..O..      ",
"      ..        ",
"                "
};

static void run_automated_tests (void);

/* This custom model is to test custom model use. */

#define CTK_TYPE_MODEL_TYPES				(ctk_tree_model_types_get_type ())
#define CTK_TREE_MODEL_TYPES(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_MODEL_TYPES, CtkTreeModelTypes))
#define CTK_TREE_MODEL_TYPES_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_MODEL_TYPES, CtkTreeModelTypesClass))
#define CTK_IS_TREE_MODEL_TYPES(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_MODEL_TYPES))
#define CTK_IS_TREE_MODEL_TYPES_GET_CLASS(klass)	(G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_MODEL_TYPES))

typedef struct _CtkTreeModelTypes       CtkTreeModelTypes;
typedef struct _CtkTreeModelTypesClass  CtkTreeModelTypesClass;

struct _CtkTreeModelTypes
{
  GObject parent;

  gint stamp;
};

struct _CtkTreeModelTypesClass
{
  GObjectClass parent_class;

  guint        (* get_flags)       (CtkTreeModel *tree_model);   
  gint         (* get_n_columns)   (CtkTreeModel *tree_model);
  GType        (* get_column_type) (CtkTreeModel *tree_model,
				    gint          index);
  gboolean     (* get_iter)        (CtkTreeModel *tree_model,
				    CtkTreeIter  *iter,
				    CtkTreePath  *path);
  CtkTreePath *(* get_path)        (CtkTreeModel *tree_model,
				    CtkTreeIter  *iter);
  void         (* get_value)       (CtkTreeModel *tree_model,
				    CtkTreeIter  *iter,
				    gint          column,
				    GValue       *value);
  gboolean     (* iter_next)       (CtkTreeModel *tree_model,
				    CtkTreeIter  *iter);
  gboolean     (* iter_children)   (CtkTreeModel *tree_model,
				    CtkTreeIter  *iter,
				    CtkTreeIter  *parent);
  gboolean     (* iter_has_child)  (CtkTreeModel *tree_model,
				    CtkTreeIter  *iter);
  gint         (* iter_n_children) (CtkTreeModel *tree_model,
				    CtkTreeIter  *iter);
  gboolean     (* iter_nth_child)  (CtkTreeModel *tree_model,
				    CtkTreeIter  *iter,
				    CtkTreeIter  *parent,
				    gint          n);
  gboolean     (* iter_parent)     (CtkTreeModel *tree_model,
				    CtkTreeIter  *iter,
				    CtkTreeIter  *child);
  void         (* ref_iter)        (CtkTreeModel *tree_model,
				    CtkTreeIter  *iter);
  void         (* unref_iter)      (CtkTreeModel *tree_model,
				    CtkTreeIter  *iter);

  /* These will be moved into the CtkTreeModelIface eventually */
  void         (* changed)         (CtkTreeModel *tree_model,
				    CtkTreePath  *path,
				    CtkTreeIter  *iter);
  void         (* inserted)        (CtkTreeModel *tree_model,
				    CtkTreePath  *path,
				    CtkTreeIter  *iter);
  void         (* child_toggled)   (CtkTreeModel *tree_model,
				    CtkTreePath  *path,
				    CtkTreeIter  *iter);
  void         (* deleted)         (CtkTreeModel *tree_model,
				    CtkTreePath  *path);
};

GType              ctk_tree_model_types_get_type      (void) G_GNUC_CONST;
CtkTreeModelTypes *ctk_tree_model_types_new           (void);

typedef enum
{
  COLUMNS_NONE,
  COLUMNS_ONE,
  COLUMNS_LOTS,
  COLUMNS_LAST
} ColumnsType;

static gchar *column_type_names[] = {
  "No columns",
  "One column",
  "Many columns"
};

#define N_COLUMNS 9

static GType*
get_model_types (void)
{
  static GType column_types[N_COLUMNS] = { 0 };
  
  if (column_types[0] == 0)
    {
      column_types[0] = G_TYPE_STRING;
      column_types[1] = G_TYPE_STRING;
      column_types[2] = CDK_TYPE_PIXBUF;
      column_types[3] = G_TYPE_FLOAT;
      column_types[4] = G_TYPE_UINT;
      column_types[5] = G_TYPE_UCHAR;
      column_types[6] = G_TYPE_CHAR;
#define BOOL_COLUMN 7
      column_types[BOOL_COLUMN] = G_TYPE_BOOLEAN;
      column_types[8] = G_TYPE_INT;
    }

  return column_types;
}

static void
toggled_callback (CtkCellRendererToggle *celltoggle,
                  gchar                 *path_string,
                  CtkTreeView           *tree_view)
{
  CtkTreeModel *model = NULL;
  CtkTreeModelSort *sort_model = NULL;
  CtkTreePath *path;
  CtkTreeIter iter;
  gboolean active = FALSE;
  
  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  model = ctk_tree_view_get_model (tree_view);
  
  if (CTK_IS_TREE_MODEL_SORT (model))
    {
      sort_model = CTK_TREE_MODEL_SORT (model);
      model = ctk_tree_model_sort_get_model (sort_model);
    }

  if (model == NULL)
    return;

  if (sort_model)
    {
      g_warning ("FIXME implement conversion from TreeModelSort iter to child model iter");
      return;
    }
      
  path = ctk_tree_path_new_from_string (path_string);
  if (!ctk_tree_model_get_iter (model,
                                &iter, path))
    {
      g_warning ("%s: bad path?", G_STRLOC);
      return;
    }
  ctk_tree_path_free (path);
  
  if (CTK_IS_LIST_STORE (model))
    {
      ctk_tree_model_get (CTK_TREE_MODEL (model),
                          &iter,
                          BOOL_COLUMN,
                          &active,
                          -1);
      
      ctk_list_store_set (CTK_LIST_STORE (model),
                          &iter,
                          BOOL_COLUMN,
                          !active,
                          -1);
    }
  else if (CTK_IS_TREE_STORE (model))
    {
      ctk_tree_model_get (CTK_TREE_MODEL (model),
                          &iter,
                          BOOL_COLUMN,
                          &active,
                          -1);
            
      ctk_tree_store_set (CTK_TREE_STORE (model),
                          &iter,
                          BOOL_COLUMN,
                          !active,
                          -1);
    }
  else
    g_warning ("don't know how to actually toggle value for model type %s",
               g_type_name (G_TYPE_FROM_INSTANCE (model)));
}

static void
edited_callback (CtkCellRendererText *renderer,
		 const gchar   *path_string,
		 const gchar   *new_text,
		 CtkTreeView  *tree_view)
{
  CtkTreeModel *model = NULL;
  CtkTreeModelSort *sort_model = NULL;
  CtkTreePath *path;
  CtkTreeIter iter;
  guint value = atoi (new_text);
  
  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  model = ctk_tree_view_get_model (tree_view);
  
  if (CTK_IS_TREE_MODEL_SORT (model))
    {
      sort_model = CTK_TREE_MODEL_SORT (model);
      model = ctk_tree_model_sort_get_model (sort_model);
    }

  if (model == NULL)
    return;

  if (sort_model)
    {
      g_warning ("FIXME implement conversion from TreeModelSort iter to child model iter");
      return;
    }
      
  path = ctk_tree_path_new_from_string (path_string);
  if (!ctk_tree_model_get_iter (model,
                                &iter, path))
    {
      g_warning ("%s: bad path?", G_STRLOC);
      return;
    }
  ctk_tree_path_free (path);

  if (CTK_IS_LIST_STORE (model))
    {
      ctk_list_store_set (CTK_LIST_STORE (model),
                          &iter,
                          4,
                          value,
                          -1);
    }
  else if (CTK_IS_TREE_STORE (model))
    {
      ctk_tree_store_set (CTK_TREE_STORE (model),
                          &iter,
                          4,
                          value,
                          -1);
    }
  else
    g_warning ("don't know how to actually toggle value for model type %s",
               g_type_name (G_TYPE_FROM_INSTANCE (model)));
}

static ColumnsType current_column_type = COLUMNS_LOTS;

static void
set_columns_type (CtkTreeView *tree_view, ColumnsType type)
{
  CtkTreeViewColumn *col;
  CtkCellRenderer *rend;
  GdkPixbuf *pixbuf;
  CtkWidget *image;
  CtkAdjustment *adjustment;

  current_column_type = type;
  
  col = ctk_tree_view_get_column (tree_view, 0);
  while (col)
    {
      ctk_tree_view_remove_column (tree_view, col);

      col = ctk_tree_view_get_column (tree_view, 0);
    }

  switch (type)
    {
    case COLUMNS_NONE:
      break;

    case COLUMNS_LOTS:
      rend = ctk_cell_renderer_text_new ();

      col = ctk_tree_view_column_new_with_attributes ("Column 1",
                                                      rend,
                                                      "text", 1,
                                                      NULL);
      
      ctk_tree_view_append_column (CTK_TREE_VIEW (tree_view), col);
      
      col = ctk_tree_view_column_new();
      ctk_tree_view_column_set_title (col, "Column 2");
      
      rend = ctk_cell_renderer_pixbuf_new ();
      ctk_tree_view_column_pack_start (col, rend, FALSE);
      ctk_tree_view_column_add_attribute (col, rend, "pixbuf", 2);
      rend = ctk_cell_renderer_text_new ();
      ctk_tree_view_column_pack_start (col, rend, TRUE);
      ctk_tree_view_column_add_attribute (col, rend, "text", 0);

      
      ctk_tree_view_append_column (CTK_TREE_VIEW (tree_view), col);
      ctk_tree_view_set_expander_column (tree_view, col);
      
      rend = ctk_cell_renderer_toggle_new ();

      g_signal_connect (rend, "toggled",
			G_CALLBACK (toggled_callback), tree_view);
      
      col = ctk_tree_view_column_new_with_attributes ("Column 3",
                                                      rend,
                                                      "active", BOOL_COLUMN,
                                                      NULL);

      ctk_tree_view_append_column (CTK_TREE_VIEW (tree_view), col);

      pixbuf = gdk_pixbuf_new_from_xpm_data ((const char **)book_closed_xpm);

      image = ctk_image_new_from_pixbuf (pixbuf);

      g_object_unref (pixbuf);
      
      ctk_widget_show (image);
      
      ctk_tree_view_column_set_widget (col, image);
      
      rend = ctk_cell_renderer_toggle_new ();

      /* you could also set this per-row by tying it to a column
       * in the model of course.
       */
      g_object_set (rend, "radio", TRUE, NULL);
      
      g_signal_connect (rend, "toggled",
			G_CALLBACK (toggled_callback), tree_view);
      
      col = ctk_tree_view_column_new_with_attributes ("Column 4",
                                                      rend,
                                                      "active", BOOL_COLUMN,
                                                      NULL);

      ctk_tree_view_append_column (CTK_TREE_VIEW (tree_view), col);

      rend = ctk_cell_renderer_spin_new ();

      adjustment = ctk_adjustment_new (0, 0, 10000, 100, 100, 100);
      g_object_set (rend, "editable", TRUE, NULL);
      g_object_set (rend, "adjustment", adjustment, NULL);

      g_signal_connect (rend, "edited",
			G_CALLBACK (edited_callback), tree_view);

      col = ctk_tree_view_column_new_with_attributes ("Column 5",
                                                      rend,
                                                      "text", 4,
                                                      NULL);

      ctk_tree_view_append_column (CTK_TREE_VIEW (tree_view), col);
#if 0
      
      rend = ctk_cell_renderer_text_new ();
      
      col = ctk_tree_view_column_new_with_attributes ("Column 6",
                                                      rend,
                                                      "text", 4,
                                                      NULL);

      ctk_tree_view_append_column (CTK_TREE_VIEW (tree_view), col);
      
      rend = ctk_cell_renderer_text_new ();
      
      col = ctk_tree_view_column_new_with_attributes ("Column 7",
                                                      rend,
                                                      "text", 5,
                                                      NULL);

      ctk_tree_view_append_column (CTK_TREE_VIEW (tree_view), col);
      
      rend = ctk_cell_renderer_text_new ();
      
      col = ctk_tree_view_column_new_with_attributes ("Column 8",
                                                      rend,
                                                      "text", 6,
                                                      NULL);

      ctk_tree_view_append_column (CTK_TREE_VIEW (tree_view), col);
      
      rend = ctk_cell_renderer_text_new ();
      
      col = ctk_tree_view_column_new_with_attributes ("Column 9",
                                                      rend,
                                                      "text", 7,
                                                      NULL);

      ctk_tree_view_append_column (CTK_TREE_VIEW (tree_view), col);
      
      rend = ctk_cell_renderer_text_new ();
      
      col = ctk_tree_view_column_new_with_attributes ("Column 10",
                                                      rend,
                                                      "text", 8,
                                                      NULL);

      ctk_tree_view_append_column (CTK_TREE_VIEW (tree_view), col);
      
#endif
      
      /* FALL THRU */
      
    case COLUMNS_ONE:
      rend = ctk_cell_renderer_text_new ();
      
      col = ctk_tree_view_column_new_with_attributes ("Column 0",
                                                      rend,
                                                      "text", 0,
                                                      NULL);

      ctk_tree_view_insert_column (CTK_TREE_VIEW (tree_view), col, 0);
    default:
      break;
    }
}

static ColumnsType
get_columns_type (void)
{
  return current_column_type;
}

static GdkPixbuf *our_pixbuf;
  
typedef enum
{
  /*   MODEL_TYPES, */
  MODEL_TREE,
  MODEL_LIST,
  MODEL_SORTED_TREE,
  MODEL_SORTED_LIST,
  MODEL_EMPTY_LIST,
  MODEL_EMPTY_TREE,
  MODEL_NULL,
  MODEL_LAST
} ModelType;

/* FIXME add a custom model to test */
static CtkTreeModel *models[MODEL_LAST];
static const char *model_names[MODEL_LAST] = {
  "CtkTreeStore",
  "CtkListStore",
  "CtkTreeModelSort wrapping CtkTreeStore",
  "CtkTreeModelSort wrapping CtkListStore",
  "Empty CtkListStore",
  "Empty CtkTreeStore",
  "NULL (no model)"
};

static CtkTreeModel*
create_list_model (void)
{
  CtkListStore *store;
  CtkTreeIter iter;
  gint i;
  GType *t;

  t = get_model_types ();
  
  store = ctk_list_store_new (N_COLUMNS,
			      t[0], t[1], t[2],
			      t[3], t[4], t[5],
			      t[6], t[7], t[8]);

  i = 0;
  while (i < 200)
    {
      char *msg;
      
      ctk_list_store_append (store, &iter);

      msg = g_strdup_printf ("%d", i);
      
      ctk_list_store_set (store, &iter, 0, msg, 1, "Foo! Foo! Foo!",
                          2, our_pixbuf,
                          3, 7.0, 4, (guint) 9000,
                          5, 'f', 6, 'g',
                          7, TRUE, 8, 23245454,
                          -1);

      g_free (msg);
      
      ++i;
    }

  return CTK_TREE_MODEL (store);
}

static void
typesystem_recurse (GType        type,
                    CtkTreeIter *parent_iter,
                    CtkTreeStore *store)
{
  GType* children;
  guint n_children = 0;
  gint i;
  CtkTreeIter iter;
  gchar *str;
  
  ctk_tree_store_append (store, &iter, parent_iter);

  str = g_strdup_printf ("%ld", (glong)type);
  ctk_tree_store_set (store, &iter, 0, str, 1, g_type_name (type),
                      2, our_pixbuf,
                      3, 7.0, 4, (guint) 9000,
                      5, 'f', 6, 'g',
                      7, TRUE, 8, 23245454,
                      -1);
  g_free (str);
  
  children = g_type_children (type, &n_children);

  i = 0;
  while (i < n_children)
    {
      typesystem_recurse (children[i], &iter, store);

      ++i;
    }
  
  g_free (children);
}

static CtkTreeModel*
create_tree_model (void)
{
  CtkTreeStore *store;
  gint i;
  GType *t;
  
  /* Make the tree more interesting */
  /* - we need this magic here so we are sure the type ends up being
   * registered and gcc doesn't optimize away the code */
  g_type_class_unref (g_type_class_ref (ctk_scrolled_window_get_type ()));
  g_type_class_unref (g_type_class_ref (ctk_label_get_type ()));
  g_type_class_unref (g_type_class_ref (ctk_scrollbar_get_type ()));
  g_type_class_unref (g_type_class_ref (pango_layout_get_type ()));

  t = get_model_types ();
  
  store = ctk_tree_store_new (N_COLUMNS,
			      t[0], t[1], t[2],
			      t[3], t[4], t[5],
			      t[6], t[7], t[8]);

  i = 0;
  while (i < G_TYPE_FUNDAMENTAL_MAX)
    {
      typesystem_recurse (i, NULL, store);
      
      ++i;
    }

  return CTK_TREE_MODEL (store);
}

static void
model_selected (CtkComboBox *combo_box, gpointer data)
{
  CtkTreeView *tree_view = CTK_TREE_VIEW (data);
  gint hist;

  hist = ctk_combo_box_get_active (combo_box);

  if (models[hist] != ctk_tree_view_get_model (tree_view))
    {
      ctk_tree_view_set_model (tree_view, models[hist]);
    }
}

static void
columns_selected (CtkComboBox *combo_box, gpointer data)
{
  CtkTreeView *tree_view = CTK_TREE_VIEW (data);
  gint hist;

  hist = ctk_combo_box_get_active (combo_box);

  if (hist != get_columns_type ())
    {
      set_columns_type (tree_view, hist);
    }
}

void
on_row_activated (CtkTreeView       *tree_view,
                  CtkTreePath       *path,
                  CtkTreeViewColumn *column,
                  gpointer           user_data)
{
  g_print ("Row activated\n");
}

enum
{
  TARGET_CTK_TREE_MODEL_ROW
};

static CtkTargetEntry row_targets[] = {
  { "CTK_TREE_MODEL_ROW", CTK_TARGET_SAME_APP,
    TARGET_CTK_TREE_MODEL_ROW }
};

int
main (int    argc,
      char **argv)
{
  CtkWidget *window;
  CtkWidget *sw;
  CtkWidget *tv;
  CtkWidget *box;
  CtkWidget *combo_box;
  CtkTreeModel *model;
  gint i;
  
  ctk_init (&argc, &argv);

  if (g_getenv ("RTL"))
    ctk_widget_set_default_direction (CTK_TEXT_DIR_RTL);

  our_pixbuf = gdk_pixbuf_new_from_xpm_data ((const char **) book_closed_xpm);  
  
#if 0
  models[MODEL_TYPES] = CTK_TREE_MODEL (ctk_tree_model_types_new ());
#endif
  models[MODEL_LIST] = create_list_model ();
  models[MODEL_TREE] = create_tree_model ();

  model = create_list_model ();
  models[MODEL_SORTED_LIST] = ctk_tree_model_sort_new_with_model (model);
  g_object_unref (model);

  model = create_tree_model ();
  models[MODEL_SORTED_TREE] = ctk_tree_model_sort_new_with_model (model);
  g_object_unref (model);

  models[MODEL_EMPTY_LIST] = CTK_TREE_MODEL (ctk_list_store_new (1, G_TYPE_INT));
  models[MODEL_EMPTY_TREE] = CTK_TREE_MODEL (ctk_tree_store_new (1, G_TYPE_INT));
  
  models[MODEL_NULL] = NULL;

  run_automated_tests ();
  
  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "destroy", G_CALLBACK (ctk_main_quit), NULL);
  ctk_window_set_default_size (CTK_WINDOW (window), 430, 400);

  box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);

  ctk_container_add (CTK_CONTAINER (window), box);

  tv = ctk_tree_view_new_with_model (models[0]);
  g_signal_connect (tv, "row-activated", G_CALLBACK (on_row_activated), NULL);

  ctk_tree_view_enable_model_drag_source (CTK_TREE_VIEW (tv),
					  CDK_BUTTON1_MASK,
					  row_targets,
					  G_N_ELEMENTS (row_targets),
					  CDK_ACTION_MOVE | CDK_ACTION_COPY);

  ctk_tree_view_enable_model_drag_dest (CTK_TREE_VIEW (tv),
					row_targets,
					G_N_ELEMENTS (row_targets),
					CDK_ACTION_MOVE | CDK_ACTION_COPY);
  
  /* Model menu */
  combo_box = ctk_combo_box_text_new ();
  ctk_widget_set_halign (combo_box, CTK_ALIGN_CENTER);
  for (i = 0; i < MODEL_LAST; i++)
      ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo_box), model_names[i]);

  ctk_container_add (CTK_CONTAINER (box), combo_box);
  g_signal_connect (combo_box,
                    "changed",
                    G_CALLBACK (model_selected),
		    tv);
  
  /* Columns menu */
  combo_box = ctk_combo_box_text_new ();
  ctk_widget_set_halign (combo_box, CTK_ALIGN_CENTER);
  for (i = 0; i < COLUMNS_LAST; i++)
      ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo_box), column_type_names[i]);

  ctk_container_add (CTK_CONTAINER (box), combo_box);

  set_columns_type (CTK_TREE_VIEW (tv), COLUMNS_LOTS);
  ctk_combo_box_set_active (CTK_COMBO_BOX (combo_box), COLUMNS_LOTS);

  g_signal_connect (combo_box,
                    "changed",
                    G_CALLBACK (columns_selected),
                    tv);
  
  sw = ctk_scrolled_window_new (NULL, NULL);
  ctk_widget_set_hexpand (sw, TRUE);
  ctk_widget_set_vexpand (sw, TRUE);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (sw),
                                  CTK_POLICY_AUTOMATIC,
                                  CTK_POLICY_AUTOMATIC);
  
  ctk_container_add (CTK_CONTAINER (box), sw);
  
  ctk_container_add (CTK_CONTAINER (sw), tv);
  
  ctk_widget_show_all (window);
  
  ctk_main ();

  return 0;
}

/*
 * CtkTreeModelTypes
 */

static void         ctk_tree_model_types_init                 (CtkTreeModelTypes      *model_types);
static void         ctk_tree_model_types_tree_model_init      (CtkTreeModelIface   *iface);
static gint         ctk_real_model_types_get_n_columns   (CtkTreeModel        *tree_model);
static GType        ctk_real_model_types_get_column_type (CtkTreeModel        *tree_model,
							   gint                 index);
static CtkTreePath *ctk_real_model_types_get_path        (CtkTreeModel        *tree_model,
							   CtkTreeIter         *iter);
static void         ctk_real_model_types_get_value       (CtkTreeModel        *tree_model,
							   CtkTreeIter         *iter,
							   gint                 column,
							   GValue              *value);
static gboolean     ctk_real_model_types_iter_next       (CtkTreeModel        *tree_model,
							   CtkTreeIter         *iter);
static gboolean     ctk_real_model_types_iter_children   (CtkTreeModel        *tree_model,
							   CtkTreeIter         *iter,
							   CtkTreeIter         *parent);
static gboolean     ctk_real_model_types_iter_has_child  (CtkTreeModel        *tree_model,
							   CtkTreeIter         *iter);
static gint         ctk_real_model_types_iter_n_children (CtkTreeModel        *tree_model,
							   CtkTreeIter         *iter);
static gboolean     ctk_real_model_types_iter_nth_child  (CtkTreeModel        *tree_model,
							   CtkTreeIter         *iter,
							   CtkTreeIter         *parent,
							   gint                 n);
static gboolean     ctk_real_model_types_iter_parent     (CtkTreeModel        *tree_model,
							   CtkTreeIter         *iter,
							   CtkTreeIter         *child);


GType
ctk_tree_model_types_get_type (void)
{
  static GType model_types_type = 0;

  if (!model_types_type)
    {
      const GTypeInfo model_types_info =
      {
        sizeof (CtkTreeModelTypesClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
        NULL,           /* class_init */
	NULL,		/* class_finalize */
	NULL,		/* class_data */
        sizeof (CtkTreeModelTypes),
	0,
        (GInstanceInitFunc) ctk_tree_model_types_init
      };

      const GInterfaceInfo tree_model_info =
      {
	(GInterfaceInitFunc) ctk_tree_model_types_tree_model_init,
	NULL,
	NULL
      };

      model_types_type = g_type_register_static (G_TYPE_OBJECT,
						 "CtkTreeModelTypes",
						 &model_types_info, 0);
      g_type_add_interface_static (model_types_type,
				   CTK_TYPE_TREE_MODEL,
				   &tree_model_info);
    }

  return model_types_type;
}

CtkTreeModelTypes *
ctk_tree_model_types_new (void)
{
  CtkTreeModelTypes *retval;

  retval = g_object_new (CTK_TYPE_MODEL_TYPES, NULL);

  return retval;
}

static void
ctk_tree_model_types_tree_model_init (CtkTreeModelIface *iface)
{
  iface->get_n_columns = ctk_real_model_types_get_n_columns;
  iface->get_column_type = ctk_real_model_types_get_column_type;
  iface->get_path = ctk_real_model_types_get_path;
  iface->get_value = ctk_real_model_types_get_value;
  iface->iter_next = ctk_real_model_types_iter_next;
  iface->iter_children = ctk_real_model_types_iter_children;
  iface->iter_has_child = ctk_real_model_types_iter_has_child;
  iface->iter_n_children = ctk_real_model_types_iter_n_children;
  iface->iter_nth_child = ctk_real_model_types_iter_nth_child;
  iface->iter_parent = ctk_real_model_types_iter_parent;
}

static void
ctk_tree_model_types_init (CtkTreeModelTypes *model_types)
{
  model_types->stamp = g_random_int ();
}

static GType column_types[] = {
  G_TYPE_STRING, /* GType */
  G_TYPE_STRING  /* type name */
};
  
static gint
ctk_real_model_types_get_n_columns (CtkTreeModel *tree_model)
{
  return G_N_ELEMENTS (column_types);
}

static GType
ctk_real_model_types_get_column_type (CtkTreeModel *tree_model,
                                      gint          index)
{
  g_return_val_if_fail (index < G_N_ELEMENTS (column_types), G_TYPE_INVALID);
  
  return column_types[index];
}

#if 0
/* Use default implementation of this */
static gboolean
ctk_real_model_types_get_iter (CtkTreeModel *tree_model,
                               CtkTreeIter  *iter,
                               CtkTreePath  *path)
{
  
}
#endif

/* The toplevel nodes of the tree are the reserved types, G_TYPE_NONE through
 * G_TYPE_RESERVED_FUNDAMENTAL.
 */

static CtkTreePath *
ctk_real_model_types_get_path (CtkTreeModel *tree_model,
                               CtkTreeIter  *iter)
{
  CtkTreePath *retval;
  GType type;
  GType parent;
  
  g_return_val_if_fail (CTK_IS_TREE_MODEL_TYPES (tree_model), NULL);
  g_return_val_if_fail (iter != NULL, NULL);

  type = GPOINTER_TO_INT (iter->user_data);
  
  retval = ctk_tree_path_new ();
  
  parent = g_type_parent (type);
  while (parent != G_TYPE_INVALID)
    {
      GType* children = g_type_children (parent, NULL);
      gint i = 0;

      if (!children || children[0] == G_TYPE_INVALID)
        {
          g_warning ("bad iterator?");
          return NULL;
        }
      
      while (children[i] != type)
        ++i;

      ctk_tree_path_prepend_index (retval, i);

      g_free (children);
      
      type = parent;
      parent = g_type_parent (parent);
    }

  /* The fundamental type itself is the index on the toplevel */
  ctk_tree_path_prepend_index (retval, type);

  return retval;
}

static void
ctk_real_model_types_get_value (CtkTreeModel *tree_model,
                                CtkTreeIter  *iter,
                                gint          column,
                                GValue       *value)
{
  GType type;

  type = GPOINTER_TO_INT (iter->user_data);

  switch (column)
    {
    case 0:
      {
        gchar *str;
        
        g_value_init (value, G_TYPE_STRING);

        str = g_strdup_printf ("%ld", (long int) type);
        g_value_set_string (value, str);
        g_free (str);
      }
      break;

    case 1:
      g_value_init (value, G_TYPE_STRING);
      g_value_set_string (value, g_type_name (type));
      break;

    default:
      g_warning ("Bad column %d requested", column);
    }
}

static gboolean
ctk_real_model_types_iter_next (CtkTreeModel  *tree_model,
                                CtkTreeIter   *iter)
{
  
  GType parent;
  GType type;

  type = GPOINTER_TO_INT (iter->user_data);

  parent = g_type_parent (type);
  
  if (parent == G_TYPE_INVALID)
    {
      /* find next _valid_ fundamental type */
      do
	type++;
      while (!g_type_name (type) && type <= G_TYPE_FUNDAMENTAL_MAX);
      if (type <= G_TYPE_FUNDAMENTAL_MAX)
	{
	  /* found one */
          iter->user_data = GINT_TO_POINTER (type);
          return TRUE;
        }
      else
        return FALSE;
    }
  else
    {
      GType* children = g_type_children (parent, NULL);
      gint i = 0;

      g_assert (children != NULL);
      
      while (children[i] != type)
        ++i;
  
      ++i;

      if (children[i] != G_TYPE_INVALID)
        {
          g_free (children);
          iter->user_data = GINT_TO_POINTER (children[i]);
          return TRUE;
        }
      else
        {
          g_free (children);
          return FALSE;
        }
    }
}

static gboolean
ctk_real_model_types_iter_children (CtkTreeModel *tree_model,
                                    CtkTreeIter  *iter,
                                    CtkTreeIter  *parent)
{
  GType type;
  GType* children;
  
  type = GPOINTER_TO_INT (parent->user_data);

  children = g_type_children (type, NULL);

  if (!children || children[0] == G_TYPE_INVALID)
    {
      g_free (children);
      return FALSE;
    }
  else
    {
      iter->user_data = GINT_TO_POINTER (children[0]);
      g_free (children);
      return TRUE;
    }
}

static gboolean
ctk_real_model_types_iter_has_child (CtkTreeModel *tree_model,
                                     CtkTreeIter  *iter)
{
  GType type;
  GType* children;
  
  type = GPOINTER_TO_INT (iter->user_data);
  
  children = g_type_children (type, NULL);

  if (!children || children[0] == G_TYPE_INVALID)
    {
      g_free (children);
      return FALSE;
    }
  else
    {
      g_free (children);
      return TRUE;
    }
}

static gint
ctk_real_model_types_iter_n_children (CtkTreeModel *tree_model,
                                      CtkTreeIter  *iter)
{
  if (iter == NULL)
    {
      return G_TYPE_FUNDAMENTAL_MAX;
    }
  else
    {
      GType type;
      GType* children;
      guint n_children = 0;

      type = GPOINTER_TO_INT (iter->user_data);
      
      children = g_type_children (type, &n_children);
      
      g_free (children);
      
      return n_children;
    }
}

static gboolean
ctk_real_model_types_iter_nth_child (CtkTreeModel *tree_model,
                                     CtkTreeIter  *iter,
                                     CtkTreeIter  *parent,
                                     gint          n)
{  
  if (parent == NULL)
    {
      /* fundamental type */
      if (n < G_TYPE_FUNDAMENTAL_MAX)
        {
          iter->user_data = GINT_TO_POINTER (n);
          return TRUE;
        }
      else
        return FALSE;
    }
  else
    {
      GType type = GPOINTER_TO_INT (parent->user_data);      
      guint n_children = 0;
      GType* children = g_type_children (type, &n_children);

      if (n_children == 0)
        {
          g_free (children);
          return FALSE;
        }
      else if (n >= n_children)
        {
          g_free (children);
          return FALSE;
        }
      else
        {
          iter->user_data = GINT_TO_POINTER (children[n]);
          g_free (children);

          return TRUE;
        }
    }
}

static gboolean
ctk_real_model_types_iter_parent (CtkTreeModel *tree_model,
                                  CtkTreeIter  *iter,
                                  CtkTreeIter  *child)
{
  GType type;
  GType parent;
  
  type = GPOINTER_TO_INT (child->user_data);
  
  parent = g_type_parent (type);
  
  if (parent == G_TYPE_INVALID)
    {
      if (type > G_TYPE_FUNDAMENTAL_MAX)
        g_warning ("no parent for %ld %s\n",
                   (long int) type,
                   g_type_name (type));
      return FALSE;
    }
  else
    {
      iter->user_data = GINT_TO_POINTER (parent);
      
      return TRUE;
    }
}

/*
 * Automated testing
 */

#if 0

static void
treestore_torture_recurse (CtkTreeStore *store,
                           CtkTreeIter  *root,
                           gint          depth)
{
  CtkTreeModel *model;
  gint i;
  CtkTreeIter iter;  
  
  model = CTK_TREE_MODEL (store);    

  if (depth > 2)
    return;

  ++depth;

  ctk_tree_store_append (store, &iter, root);
  
  ctk_tree_model_iter_children (model, &iter, root);
  
  i = 0;
  while (i < 100)
    {
      ctk_tree_store_append (store, &iter, root);
      ++i;
    }

  while (ctk_tree_model_iter_children (model, &iter, root))
    ctk_tree_store_remove (store, &iter);

  ctk_tree_store_append (store, &iter, root);

  /* inserts before last node in tree */
  i = 0;
  while (i < 100)
    {
      ctk_tree_store_insert_before (store, &iter, root, &iter);
      ++i;
    }

  /* inserts after the node before the last node */
  i = 0;
  while (i < 100)
    {
      ctk_tree_store_insert_after (store, &iter, root, &iter);
      ++i;
    }

  /* inserts after the last node */
  ctk_tree_store_append (store, &iter, root);
    
  i = 0;
  while (i < 100)
    {
      ctk_tree_store_insert_after (store, &iter, root, &iter);
      ++i;
    }

  /* remove everything again */
  while (ctk_tree_model_iter_children (model, &iter, root))
    ctk_tree_store_remove (store, &iter);


    /* Prepends */
  ctk_tree_store_prepend (store, &iter, root);
    
  i = 0;
  while (i < 100)
    {
      ctk_tree_store_prepend (store, &iter, root);
      ++i;
    }

  /* remove everything again */
  while (ctk_tree_model_iter_children (model, &iter, root))
    ctk_tree_store_remove (store, &iter);

  ctk_tree_store_append (store, &iter, root);
  ctk_tree_store_append (store, &iter, root);
  ctk_tree_store_append (store, &iter, root);
  ctk_tree_store_append (store, &iter, root);

  while (ctk_tree_model_iter_children (model, &iter, root))
    {
      treestore_torture_recurse (store, &iter, depth);
      ctk_tree_store_remove (store, &iter);
    }
}

#endif

static void
run_automated_tests (void)
{
  g_print ("Running automated tests...\n");
  
  /* FIXME TreePath basic verification */

  /* FIXME generic consistency checks on the models */

  {
    /* Make sure list store mutations don't crash anything */
    CtkListStore *store;
    CtkTreeModel *model;
    gint i;
    CtkTreeIter iter;
    
    store = ctk_list_store_new (1, G_TYPE_INT);

    model = CTK_TREE_MODEL (store);
    
    i = 0;
    while (i < 100)
      {
        ctk_list_store_append (store, &iter);
        ++i;
      }

    while (ctk_tree_model_get_iter_first (model, &iter))
      ctk_list_store_remove (store, &iter);

    ctk_list_store_append (store, &iter);

    /* inserts before last node in list */
    i = 0;
    while (i < 100)
      {
        ctk_list_store_insert_before (store, &iter, &iter);
        ++i;
      }

    /* inserts after the node before the last node */
    i = 0;
    while (i < 100)
      {
        ctk_list_store_insert_after (store, &iter, &iter);
        ++i;
      }

    /* inserts after the last node */
    ctk_list_store_append (store, &iter);
    
    i = 0;
    while (i < 100)
      {
        ctk_list_store_insert_after (store, &iter, &iter);
        ++i;
      }

    /* remove everything again */
    while (ctk_tree_model_get_iter_first (model, &iter))
      ctk_list_store_remove (store, &iter);


    /* Prepends */
    ctk_list_store_prepend (store, &iter);
    
    i = 0;
    while (i < 100)
      {
        ctk_list_store_prepend (store, &iter);
        ++i;
      }

    /* remove everything again */
    while (ctk_tree_model_get_iter_first (model, &iter))
      ctk_list_store_remove (store, &iter);
    
    g_object_unref (store);
  }

  {
    /* Make sure tree store mutations don't crash anything */
    CtkTreeStore *store;
    CtkTreeIter root;

    store = ctk_tree_store_new (1, G_TYPE_INT);
    ctk_tree_store_append (CTK_TREE_STORE (store), &root, NULL);
    /* Remove test until it is rewritten to work */
    /*    treestore_torture_recurse (store, &root, 0);*/
    
    g_object_unref (store);
  }

  g_print ("Passed.\n");
}
