/* testtreecolumns.c
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

#include "config.h"
#include <ctk/ctk.h>

/*
 * README README README README README README README README README README
 * README README README README README README README README README README
 * README README README README README README README README README README
 * README README README README README README README README README README
 * README README README README README README README README README README
 * README README README README README README README README README README
 * README README README README README README README README README README
 * README README README README README README README README README README
 * README README README README README README README README README README
 * README README README README README README README README README README
 * README README README README README README README README README README
 * README README README README README README README README README README
 * README README README README README README README README README README
 *
 * DO NOT!!! I REPEAT DO NOT!  EVER LOOK AT THIS CODE AS AN EXAMPLE OF WHAT YOUR
 * CODE SHOULD LOOK LIKE.
 *
 * IT IS VERY CONFUSING, AND IS MEANT TO TEST A LOT OF CODE IN THE TREE.  WHILE
 * IT IS ACTUALLY CORRECT CODE, IT IS NOT USEFUL.
 */

CtkWidget *left_tree_view;
CtkWidget *top_right_tree_view;
CtkWidget *bottom_right_tree_view;
CtkTreeModel *left_tree_model;
CtkTreeModel *top_right_tree_model;
CtkTreeModel *bottom_right_tree_model;
CtkWidget *sample_tree_view_top;
CtkWidget *sample_tree_view_bottom;

#define column_data "my_column_data"

static void move_row  (CtkTreeModel *src,
		       CtkTreeIter  *src_iter,
		       CtkTreeModel *dest,
		       CtkTreeIter  *dest_iter);

/* Kids, don't try this at home.  */

/* Small CtkTreeModel to model columns */
typedef struct _ViewColumnModel ViewColumnModel;
typedef struct _ViewColumnModelClass ViewColumnModelClass;

struct _ViewColumnModel
{
  CtkListStore parent;
  CtkTreeView *view;
  GList *columns;
  gint stamp;
};

struct _ViewColumnModelClass
{
  CtkListStoreClass parent_class;
};

static void view_column_model_init (ViewColumnModel *model)
{
  model->stamp = g_random_int ();
}

static gint
view_column_model_get_n_columns (CtkTreeModel *tree_model)
{
  return 2;
}

static GType
view_column_model_get_column_type (CtkTreeModel *tree_model,
				   gint          index)
{
  switch (index)
    {
    case 0:
      return G_TYPE_STRING;
    case 1:
      return CTK_TYPE_TREE_VIEW_COLUMN;
    default:
      return G_TYPE_INVALID;
    }
}

static gboolean
view_column_model_get_iter (CtkTreeModel *tree_model,
			    CtkTreeIter  *iter,
			    CtkTreePath  *path)

{
  ViewColumnModel *view_model = (ViewColumnModel *)tree_model;
  GList *list;
  gint i;

  g_return_val_if_fail (ctk_tree_path_get_depth (path) > 0, FALSE);

  i = ctk_tree_path_get_indices (path)[0];
  list = g_list_nth (view_model->columns, i);

  if (list == NULL)
    return FALSE;

  iter->stamp = view_model->stamp;
  iter->user_data = list;

  return TRUE;
}

static CtkTreePath *
view_column_model_get_path (CtkTreeModel *tree_model,
			    CtkTreeIter  *iter)
{
  ViewColumnModel *view_model = (ViewColumnModel *)tree_model;
  CtkTreePath *retval;
  GList *list;
  gint i = 0;

  g_return_val_if_fail (iter->stamp == view_model->stamp, NULL);

  for (list = view_model->columns; list; list = list->next)
    {
      if (list == (GList *)iter->user_data)
	break;
      i++;
    }
  if (list == NULL)
    return NULL;

  retval = ctk_tree_path_new ();
  ctk_tree_path_append_index (retval, i);
  return retval;
}

static void
view_column_model_get_value (CtkTreeModel *tree_model,
			     CtkTreeIter  *iter,
			     gint          column,
			     GValue       *value)
{
  ViewColumnModel *view_model = (ViewColumnModel *)tree_model;

  g_return_if_fail (column < 2);
  g_return_if_fail (view_model->stamp == iter->stamp);
  g_return_if_fail (iter->user_data != NULL);

  if (column == 0)
    {
      g_value_init (value, G_TYPE_STRING);
      g_value_set_string (value, ctk_tree_view_column_get_title (CTK_TREE_VIEW_COLUMN (((GList *)iter->user_data)->data)));
    }
  else
    {
      g_value_init (value, CTK_TYPE_TREE_VIEW_COLUMN);
      g_value_set_object (value, ((GList *)iter->user_data)->data);
    }
}

static gboolean
view_column_model_iter_next (CtkTreeModel  *tree_model,
			     CtkTreeIter   *iter)
{
  ViewColumnModel *view_model = (ViewColumnModel *)tree_model;

  g_return_val_if_fail (view_model->stamp == iter->stamp, FALSE);
  g_return_val_if_fail (iter->user_data != NULL, FALSE);

  iter->user_data = ((GList *)iter->user_data)->next;
  return iter->user_data != NULL;
}

static gboolean
view_column_model_iter_children (CtkTreeModel *tree_model,
				 CtkTreeIter  *iter,
				 CtkTreeIter  *parent)
{
  ViewColumnModel *view_model = (ViewColumnModel *)tree_model;

  /* this is a list, nodes have no children */
  if (parent)
    return FALSE;

  /* but if parent == NULL we return the list itself as children of the
   * "root"
   */

  if (view_model->columns)
    {
      iter->stamp = view_model->stamp;
      iter->user_data = view_model->columns;
      return TRUE;
    }
  else
    return FALSE;
}

static gboolean
view_column_model_iter_has_child (CtkTreeModel *tree_model,
				  CtkTreeIter  *iter)
{
  return FALSE;
}

static gint
view_column_model_iter_n_children (CtkTreeModel *tree_model,
				   CtkTreeIter  *iter)
{
  return g_list_length (((ViewColumnModel *)tree_model)->columns);
}

static gint
view_column_model_iter_nth_child (CtkTreeModel *tree_model,
 				  CtkTreeIter  *iter,
				  CtkTreeIter  *parent,
				  gint          n)
{
  ViewColumnModel *view_model = (ViewColumnModel *)tree_model;

  if (parent)
    return FALSE;

  iter->stamp = view_model->stamp;
  iter->user_data = g_list_nth ((GList *)view_model->columns, n);

  return (iter->user_data != NULL);
}

static gboolean
view_column_model_iter_parent (CtkTreeModel *tree_model,
			       CtkTreeIter  *iter,
			       CtkTreeIter  *child)
{
  return FALSE;
}

static void
view_column_model_tree_model_init (CtkTreeModelIface *iface)
{
  iface->get_n_columns = view_column_model_get_n_columns;
  iface->get_column_type = view_column_model_get_column_type;
  iface->get_iter = view_column_model_get_iter;
  iface->get_path = view_column_model_get_path;
  iface->get_value = view_column_model_get_value;
  iface->iter_next = view_column_model_iter_next;
  iface->iter_children = view_column_model_iter_children;
  iface->iter_has_child = view_column_model_iter_has_child;
  iface->iter_n_children = view_column_model_iter_n_children;
  iface->iter_nth_child = view_column_model_iter_nth_child;
  iface->iter_parent = view_column_model_iter_parent;
}

static gboolean
view_column_model_drag_data_get (CtkTreeDragSource   *drag_source,
				 CtkTreePath         *path,
				 CtkSelectionData    *selection_data)
{
  if (ctk_tree_set_row_drag_data (selection_data,
				  CTK_TREE_MODEL (drag_source),
				  path))
    return TRUE;
  else
    return FALSE;
}

static gboolean
view_column_model_drag_data_delete (CtkTreeDragSource *drag_source,
				    CtkTreePath       *path)
{
  /* Nothing -- we handle moves on the dest side */
  
  return TRUE;
}

static gboolean
view_column_model_row_drop_possible (CtkTreeDragDest   *drag_dest,
				     CtkTreePath       *dest_path,
				     CtkSelectionData  *selection_data)
{
  CtkTreeModel *src_model;
  
  if (ctk_tree_get_row_drag_data (selection_data,
				  &src_model,
				  NULL))
    {
      if (src_model == left_tree_model ||
	  src_model == top_right_tree_model ||
	  src_model == bottom_right_tree_model)
	return TRUE;
    }

  return FALSE;
}

static gboolean
view_column_model_drag_data_received (CtkTreeDragDest   *drag_dest,
				      CtkTreePath       *dest,
				      CtkSelectionData  *selection_data)
{
  CtkTreeModel *src_model;
  CtkTreePath *src_path = NULL;
  gboolean retval = FALSE;
  
  if (ctk_tree_get_row_drag_data (selection_data,
				  &src_model,
				  &src_path))
    {
      CtkTreeIter src_iter;
      CtkTreeIter dest_iter;
      gboolean have_dest;

      /* We are a little lazy here, and assume if we can't convert dest
       * to an iter, we need to append. See ctkliststore.c for a more
       * careful handling of this.
       */
      have_dest = ctk_tree_model_get_iter (CTK_TREE_MODEL (drag_dest), &dest_iter, dest);

      if (ctk_tree_model_get_iter (src_model, &src_iter, src_path))
	{
	  if (src_model == left_tree_model ||
	      src_model == top_right_tree_model ||
	      src_model == bottom_right_tree_model)
	    {
	      move_row (src_model, &src_iter, CTK_TREE_MODEL (drag_dest),
			have_dest ? &dest_iter : NULL);
	      retval = TRUE;
	    }
	}

      ctk_tree_path_free (src_path);
    }
  
  return retval;
}

static void
view_column_model_drag_source_init (CtkTreeDragSourceIface *iface)
{
  iface->drag_data_get = view_column_model_drag_data_get;
  iface->drag_data_delete = view_column_model_drag_data_delete;
}

static void
view_column_model_drag_dest_init (CtkTreeDragDestIface *iface)
{
  iface->drag_data_received = view_column_model_drag_data_received;
  iface->row_drop_possible = view_column_model_row_drop_possible;
}

static void
view_column_model_class_init (ViewColumnModelClass *klass)
{
}

G_DEFINE_TYPE_WITH_CODE (ViewColumnModel, view_column_model, CTK_TYPE_LIST_STORE,
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_TREE_MODEL, view_column_model_tree_model_init)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_TREE_DRAG_SOURCE, view_column_model_drag_source_init)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_TREE_DRAG_DEST, view_column_model_drag_dest_init))

static void
update_columns (CtkTreeView *view, ViewColumnModel *view_model)
{
  GList *old_columns = view_model->columns;
  gint old_length, length;
  GList *a, *b;

  view_model->columns = ctk_tree_view_get_columns (view_model->view);

  /* As the view tells us one change at a time, we can do this hack. */
  length = g_list_length (view_model->columns);
  old_length = g_list_length (old_columns);
  if (length != old_length)
    {
      CtkTreePath *path;
      gint i = 0;

      /* where are they different */
      for (a = old_columns, b = view_model->columns; a && b; a = a->next, b = b->next)
	{
	  if (a->data != b->data)
	    break;
	  i++;
	}
      path = ctk_tree_path_new ();
      ctk_tree_path_append_index (path, i);
      if (length < old_length)
	{
	  view_model->stamp++;
	  ctk_tree_model_row_deleted (CTK_TREE_MODEL (view_model), path);
	}
      else
	{
	  CtkTreeIter iter;
	  iter.stamp = view_model->stamp;
	  iter.user_data = b;
	  ctk_tree_model_row_inserted (CTK_TREE_MODEL (view_model), path, &iter);
	}
      ctk_tree_path_free (path);
    }
  else
    {
      gint i;
      gint m = 0, n = 1;
      gint *new_order;
      CtkTreePath *path;

      new_order = g_new (int, length);
      a = old_columns; b = view_model->columns;

      while (a->data == b->data)
	{
	  a = a->next;
	  b = b->next;
	  if (a == NULL)
	    return;
	  m++;
	}

      if (a->next->data == b->data)
	{
	  b = b->next;
	  while (b->data != a->data)
	    {
	      b = b->next;
	      n++;
	    }
	  for (i = 0; i < m; i++)
	    new_order[i] = i;
	  for (i = m; i < m+n; i++)
	    new_order[i] = i+1;
	  new_order[i] = m;
	  for (i = m + n +1; i < length; i++)
	    new_order[i] = i;
	}
      else
	{
	  a = a->next;
	  while (a->data != b->data)
	    {
	      a = a->next;
	      n++;
	    }
	  for (i = 0; i < m; i++)
	    new_order[i] = i;
	  new_order[m] = m+n;
	  for (i = m+1; i < m + n+ 1; i++)
	    new_order[i] = i - 1;
	  for (i = m + n + 1; i < length; i++)
	    new_order[i] = i;
	}

      path = ctk_tree_path_new ();
      ctk_tree_model_rows_reordered (CTK_TREE_MODEL (view_model),
				     path,
				     NULL,
				     new_order);
      ctk_tree_path_free (path);
      g_free (new_order);
    }
  if (old_columns)
    g_list_free (old_columns);
}

static CtkTreeModel *
view_column_model_new (CtkTreeView *view)
{
  CtkTreeModel *retval;

  retval = g_object_new (view_column_model_get_type (), NULL);
  ((ViewColumnModel *)retval)->view = view;
  ((ViewColumnModel *)retval)->columns = ctk_tree_view_get_columns (view);

  g_signal_connect (view, "columns_changed", G_CALLBACK (update_columns), retval);

  return retval;
}

/* Back to sanity.
 */

static void
add_clicked (CtkWidget *button, gpointer data)
{
  static gint i = 0;

  CtkTreeIter iter;
  CtkTreeViewColumn *column;
  CtkTreeSelection *selection;
  CtkCellRenderer *cell;
  gchar *label = g_strdup_printf ("Column %d", i);

  cell = ctk_cell_renderer_text_new ();
  column = ctk_tree_view_column_new_with_attributes (label, cell, "text", 0, NULL);
  g_object_set_data_full (G_OBJECT (column), column_data, label, g_free);
  ctk_tree_view_column_set_reorderable (column, TRUE);
  ctk_tree_view_column_set_sizing (column, CTK_TREE_VIEW_COLUMN_GROW_ONLY);
  ctk_tree_view_column_set_resizable (column, TRUE);
  ctk_list_store_append (CTK_LIST_STORE (left_tree_model), &iter);
  ctk_list_store_set (CTK_LIST_STORE (left_tree_model), &iter, 0, label, 1, column, -1);
  i++;

  selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (left_tree_view));
  ctk_tree_selection_select_iter (selection, &iter);
}

static void
get_visible (CtkTreeViewColumn *tree_column,
	     CtkCellRenderer   *cell,
	     CtkTreeModel      *tree_model,
	     CtkTreeIter       *iter,
	     gpointer           data)
{
  CtkTreeViewColumn *column;

  ctk_tree_model_get (tree_model, iter, 1, &column, -1);
  if (column)
    {
      ctk_cell_renderer_toggle_set_active (CTK_CELL_RENDERER_TOGGLE (cell),
					   ctk_tree_view_column_get_visible (column));
    }
}

static void
set_visible (CtkCellRendererToggle *cell,
	     gchar                 *path_str,
	     gpointer               data)
{
  CtkTreeView *tree_view = (CtkTreeView *) data;
  CtkTreeViewColumn *column;
  CtkTreeModel *model;
  CtkTreeIter iter;
  CtkTreePath *path = ctk_tree_path_new_from_string (path_str);

  model = ctk_tree_view_get_model (tree_view);

  ctk_tree_model_get_iter (model, &iter, path);
  ctk_tree_model_get (model, &iter, 1, &column, -1);

  if (column)
    {
      ctk_tree_view_column_set_visible (column, ! ctk_tree_view_column_get_visible (column));
      ctk_tree_model_row_changed (model, path, &iter);
    }
  ctk_tree_path_free (path);
}

static void
move_to_left (CtkTreeModel *src,
	      CtkTreeIter  *src_iter,
	      CtkTreeIter  *dest_iter)
{
  CtkTreeIter iter;
  CtkTreeViewColumn *column;
  CtkTreeSelection *selection;
  gchar *label;

  ctk_tree_model_get (src, src_iter, 0, &label, 1, &column, -1);

  if (src == top_right_tree_model)
    ctk_tree_view_remove_column (CTK_TREE_VIEW (sample_tree_view_top), column);
  else
    ctk_tree_view_remove_column (CTK_TREE_VIEW (sample_tree_view_bottom), column);

  /*  ctk_list_store_remove (CTK_LIST_STORE (ctk_tree_view_get_model (CTK_TREE_VIEW (data))), &iter);*/

  /* Put it back on the left */
  if (dest_iter)
    ctk_list_store_insert_before (CTK_LIST_STORE (left_tree_model),
				  &iter, dest_iter);
  else
    ctk_list_store_append (CTK_LIST_STORE (left_tree_model), &iter);
  
  ctk_list_store_set (CTK_LIST_STORE (left_tree_model), &iter, 0, label, 1, column, -1);
  selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (left_tree_view));
  ctk_tree_selection_select_iter (selection, &iter);

  g_free (label);
}

static void
move_to_right (CtkTreeIter  *src_iter,
	       CtkTreeModel *dest,
	       CtkTreeIter  *dest_iter)
{
  gchar *label;
  CtkTreeViewColumn *column;
  gint before = -1;

  ctk_tree_model_get (CTK_TREE_MODEL (left_tree_model),
		      src_iter, 0, &label, 1, &column, -1);
  ctk_list_store_remove (CTK_LIST_STORE (left_tree_model), src_iter);

  if (dest_iter)
    {
      CtkTreePath *path = ctk_tree_model_get_path (dest, dest_iter);
      before = (ctk_tree_path_get_indices (path))[0];
      ctk_tree_path_free (path);
    }
  
  if (dest == top_right_tree_model)
    ctk_tree_view_insert_column (CTK_TREE_VIEW (sample_tree_view_top), column, before);
  else
    ctk_tree_view_insert_column (CTK_TREE_VIEW (sample_tree_view_bottom), column, before);

  g_free (label);
}

static void
move_up_or_down (CtkTreeModel *src,
		 CtkTreeIter  *src_iter,
		 CtkTreeModel *dest,
		 CtkTreeIter  *dest_iter)
{
  CtkTreeViewColumn *column;
  gchar *label;
  gint before = -1;
  
  ctk_tree_model_get (src, src_iter, 0, &label, 1, &column, -1);

  if (dest_iter)
    {
      CtkTreePath *path = ctk_tree_model_get_path (dest, dest_iter);
      before = (ctk_tree_path_get_indices (path))[0];
      ctk_tree_path_free (path);
    }
  
  if (src == top_right_tree_model)
    ctk_tree_view_remove_column (CTK_TREE_VIEW (sample_tree_view_top), column);
  else
    ctk_tree_view_remove_column (CTK_TREE_VIEW (sample_tree_view_bottom), column);

  if (dest == top_right_tree_model)
    ctk_tree_view_insert_column (CTK_TREE_VIEW (sample_tree_view_top), column, before);
  else
    ctk_tree_view_insert_column (CTK_TREE_VIEW (sample_tree_view_bottom), column, before);

  g_free (label);
}

static void
move_row  (CtkTreeModel *src,
	   CtkTreeIter  *src_iter,
	   CtkTreeModel *dest,
	   CtkTreeIter  *dest_iter)
{
  if (src == left_tree_model)
    move_to_right (src_iter, dest, dest_iter);
  else if (dest == left_tree_model)
    move_to_left (src, src_iter, dest_iter);
  else 
    move_up_or_down (src, src_iter, dest, dest_iter);
}

static void
add_left_clicked (CtkWidget *button,
		  gpointer data)
{
  CtkTreeIter iter;

  CtkTreeSelection *selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (data));

  ctk_tree_selection_get_selected (selection, NULL, &iter);

  move_to_left (ctk_tree_view_get_model (CTK_TREE_VIEW (data)), &iter, NULL);
}

static void
add_right_clicked (CtkWidget *button, gpointer data)
{
  CtkTreeIter iter;

  CtkTreeSelection *selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (left_tree_view));

  ctk_tree_selection_get_selected (selection, NULL, &iter);

  move_to_right (&iter, ctk_tree_view_get_model (CTK_TREE_VIEW (data)), NULL);
}

static void
selection_changed (CtkTreeSelection *selection, CtkWidget *button)
{
  if (ctk_tree_selection_get_selected (selection, NULL, NULL))
    ctk_widget_set_sensitive (button, TRUE);
  else
    ctk_widget_set_sensitive (button, FALSE);
}

static CtkTargetEntry row_targets[] = {
  { "CTK_TREE_MODEL_ROW", CTK_TARGET_SAME_APP, 0}
};

int
main (int argc, char *argv[])
{
  CtkWidget *window;
  CtkWidget *hbox, *vbox;
  CtkWidget *vbox2, *bbox;
  CtkWidget *button;
  CtkTreeViewColumn *column;
  CtkCellRenderer *cell;
  CtkWidget *swindow;
  CtkTreeModel *sample_model;
  gint i;

  ctk_init (&argc, &argv);

  /* First initialize all the models for signal purposes */
  left_tree_model = (CtkTreeModel *) ctk_list_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);
  sample_model = (CtkTreeModel *) ctk_list_store_new (1, G_TYPE_STRING);
  sample_tree_view_top = ctk_tree_view_new_with_model (sample_model);
  sample_tree_view_bottom = ctk_tree_view_new_with_model (sample_model);
  top_right_tree_model = (CtkTreeModel *) view_column_model_new (CTK_TREE_VIEW (sample_tree_view_top));
  bottom_right_tree_model = (CtkTreeModel *) view_column_model_new (CTK_TREE_VIEW (sample_tree_view_bottom));
  top_right_tree_view = ctk_tree_view_new_with_model (top_right_tree_model);
  bottom_right_tree_view = ctk_tree_view_new_with_model (bottom_right_tree_model);

  for (i = 0; i < 10; i++)
    {
      CtkTreeIter iter;
      gchar *string = g_strdup_printf ("%d", i);
      ctk_list_store_append (CTK_LIST_STORE (sample_model), &iter);
      ctk_list_store_set (CTK_LIST_STORE (sample_model), &iter, 0, string, -1);
      g_free (string);
    }

  /* Set up the test windows. */
  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "destroy", G_CALLBACK (ctk_main_quit), NULL); 
  ctk_window_set_default_size (CTK_WINDOW (window), 300, 300);
  ctk_window_set_title (CTK_WINDOW (window), "Top Window");
  swindow = ctk_scrolled_window_new (NULL, NULL);
  ctk_container_add (CTK_CONTAINER (window), swindow);
  ctk_container_add (CTK_CONTAINER (swindow), sample_tree_view_top);
  ctk_widget_show_all (window);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "destroy", G_CALLBACK (ctk_main_quit), NULL); 
  ctk_window_set_default_size (CTK_WINDOW (window), 300, 300);
  ctk_window_set_title (CTK_WINDOW (window), "Bottom Window");
  swindow = ctk_scrolled_window_new (NULL, NULL);
  ctk_container_add (CTK_CONTAINER (window), swindow);
  ctk_container_add (CTK_CONTAINER (swindow), sample_tree_view_bottom);
  ctk_widget_show_all (window);

  /* Set up the main window */
  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "destroy", G_CALLBACK (ctk_main_quit), NULL); 
  ctk_window_set_default_size (CTK_WINDOW (window), 500, 300);
  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 8);
  ctk_container_set_border_width (CTK_CONTAINER (vbox), 8);
  ctk_container_add (CTK_CONTAINER (window), vbox);

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 8);
  ctk_box_pack_start (CTK_BOX (vbox), hbox, TRUE, TRUE, 0);

  /* Left Pane */
  cell = ctk_cell_renderer_text_new ();

  swindow = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (swindow), CTK_POLICY_AUTOMATIC, CTK_POLICY_AUTOMATIC);
  left_tree_view = ctk_tree_view_new_with_model (left_tree_model);
  ctk_container_add (CTK_CONTAINER (swindow), left_tree_view);
  ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (left_tree_view), -1,
					       "Unattached Columns", cell, "text", 0, NULL);
  cell = ctk_cell_renderer_toggle_new ();
  g_signal_connect (cell, "toggled", G_CALLBACK (set_visible), left_tree_view);
  column = ctk_tree_view_column_new_with_attributes ("Visible", cell, NULL);
  ctk_tree_view_append_column (CTK_TREE_VIEW (left_tree_view), column);

  ctk_tree_view_column_set_cell_data_func (column, cell, get_visible, NULL, NULL);
  ctk_box_pack_start (CTK_BOX (hbox), swindow, TRUE, TRUE, 0);

  /* Middle Pane */
  vbox2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 8);
  ctk_box_pack_start (CTK_BOX (hbox), vbox2, FALSE, FALSE, 0);
  
  bbox = ctk_button_box_new (CTK_ORIENTATION_VERTICAL);
  ctk_button_box_set_layout (CTK_BUTTON_BOX (bbox), CTK_BUTTONBOX_SPREAD);
  ctk_box_pack_start (CTK_BOX (vbox2), bbox, TRUE, TRUE, 0);

  button = ctk_button_new_with_mnemonic ("<< (_Q)");
  ctk_widget_set_sensitive (button, FALSE);
  g_signal_connect (button, "clicked", G_CALLBACK (add_left_clicked), top_right_tree_view);
  g_signal_connect (ctk_tree_view_get_selection (CTK_TREE_VIEW (top_right_tree_view)),
                    "changed", G_CALLBACK (selection_changed), button);
  ctk_box_pack_start (CTK_BOX (bbox), button, FALSE, FALSE, 0);

  button = ctk_button_new_with_mnemonic (">> (_W)");
  ctk_widget_set_sensitive (button, FALSE);
  g_signal_connect (button, "clicked", G_CALLBACK (add_right_clicked), top_right_tree_view);
  g_signal_connect (ctk_tree_view_get_selection (CTK_TREE_VIEW (left_tree_view)),
                    "changed", G_CALLBACK (selection_changed), button);
  ctk_box_pack_start (CTK_BOX (bbox), button, FALSE, FALSE, 0);

  bbox = ctk_button_box_new (CTK_ORIENTATION_VERTICAL);
  ctk_button_box_set_layout (CTK_BUTTON_BOX (bbox), CTK_BUTTONBOX_SPREAD);
  ctk_box_pack_start (CTK_BOX (vbox2), bbox, TRUE, TRUE, 0);

  button = ctk_button_new_with_mnemonic ("<< (_E)");
  ctk_widget_set_sensitive (button, FALSE);
  g_signal_connect (button, "clicked", G_CALLBACK (add_left_clicked), bottom_right_tree_view);
  g_signal_connect (ctk_tree_view_get_selection (CTK_TREE_VIEW (bottom_right_tree_view)),
                    "changed", G_CALLBACK (selection_changed), button);
  ctk_box_pack_start (CTK_BOX (bbox), button, FALSE, FALSE, 0);

  button = ctk_button_new_with_mnemonic (">> (_R)");
  ctk_widget_set_sensitive (button, FALSE);
  g_signal_connect (button, "clicked", G_CALLBACK (add_right_clicked), bottom_right_tree_view);
  g_signal_connect (ctk_tree_view_get_selection (CTK_TREE_VIEW (left_tree_view)),
                    "changed", G_CALLBACK (selection_changed), button);
  ctk_box_pack_start (CTK_BOX (bbox), button, FALSE, FALSE, 0);

  
  /* Right Pane */
  vbox2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 8);
  ctk_box_pack_start (CTK_BOX (hbox), vbox2, TRUE, TRUE, 0);

  swindow = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (swindow), CTK_POLICY_AUTOMATIC, CTK_POLICY_AUTOMATIC);
  ctk_tree_view_set_headers_visible (CTK_TREE_VIEW (top_right_tree_view), FALSE);
  cell = ctk_cell_renderer_text_new ();
  ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (top_right_tree_view), -1,
					       NULL, cell, "text", 0, NULL);
  cell = ctk_cell_renderer_toggle_new ();
  g_signal_connect (cell, "toggled", G_CALLBACK (set_visible), top_right_tree_view);
  column = ctk_tree_view_column_new_with_attributes (NULL, cell, NULL);
  ctk_tree_view_column_set_cell_data_func (column, cell, get_visible, NULL, NULL);
  ctk_tree_view_append_column (CTK_TREE_VIEW (top_right_tree_view), column);

  ctk_container_add (CTK_CONTAINER (swindow), top_right_tree_view);
  ctk_box_pack_start (CTK_BOX (vbox2), swindow, TRUE, TRUE, 0);

  swindow = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (swindow), CTK_POLICY_AUTOMATIC, CTK_POLICY_AUTOMATIC);
  ctk_tree_view_set_headers_visible (CTK_TREE_VIEW (bottom_right_tree_view), FALSE);
  cell = ctk_cell_renderer_text_new ();
  ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (bottom_right_tree_view), -1,
					       NULL, cell, "text", 0, NULL);
  cell = ctk_cell_renderer_toggle_new ();
  g_signal_connect (cell, "toggled", G_CALLBACK (set_visible), bottom_right_tree_view);
  column = ctk_tree_view_column_new_with_attributes (NULL, cell, NULL);
  ctk_tree_view_column_set_cell_data_func (column, cell, get_visible, NULL, NULL);
  ctk_tree_view_append_column (CTK_TREE_VIEW (bottom_right_tree_view), column);
  ctk_container_add (CTK_CONTAINER (swindow), bottom_right_tree_view);
  ctk_box_pack_start (CTK_BOX (vbox2), swindow, TRUE, TRUE, 0);

  
  /* Drag and Drop */
  ctk_tree_view_enable_model_drag_source (CTK_TREE_VIEW (left_tree_view),
					  CDK_BUTTON1_MASK,
					  row_targets,
					  G_N_ELEMENTS (row_targets),
					  CDK_ACTION_MOVE);
  ctk_tree_view_enable_model_drag_dest (CTK_TREE_VIEW (left_tree_view),
					row_targets,
					G_N_ELEMENTS (row_targets),
					CDK_ACTION_MOVE);

  ctk_tree_view_enable_model_drag_source (CTK_TREE_VIEW (top_right_tree_view),
					  CDK_BUTTON1_MASK,
					  row_targets,
					  G_N_ELEMENTS (row_targets),
					  CDK_ACTION_MOVE);
  ctk_tree_view_enable_model_drag_dest (CTK_TREE_VIEW (top_right_tree_view),
					row_targets,
					G_N_ELEMENTS (row_targets),
					CDK_ACTION_MOVE);

  ctk_tree_view_enable_model_drag_source (CTK_TREE_VIEW (bottom_right_tree_view),
					  CDK_BUTTON1_MASK,
					  row_targets,
					  G_N_ELEMENTS (row_targets),
					  CDK_ACTION_MOVE);
  ctk_tree_view_enable_model_drag_dest (CTK_TREE_VIEW (bottom_right_tree_view),
					row_targets,
					G_N_ELEMENTS (row_targets),
					CDK_ACTION_MOVE);


  ctk_box_pack_start (CTK_BOX (vbox), ctk_separator_new (CTK_ORIENTATION_HORIZONTAL),
                      FALSE, FALSE, 0);

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 8);
  ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  button = ctk_button_new_with_mnemonic ("_Add new Column");
  g_signal_connect (button, "clicked", G_CALLBACK (add_clicked), left_tree_model);
  ctk_box_pack_start (CTK_BOX (hbox), button, FALSE, FALSE, 0);

  ctk_widget_show_all (window);
  ctk_main ();

  return 0;
}
