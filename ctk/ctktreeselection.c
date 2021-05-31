/* ctktreeselection.h
 * Copyright (C) 2000  Red Hat, Inc.,  Jonathan Blandford <jrb@redhat.com>
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
#include <string.h>
#include "ctktreeselection.h"
#include "ctktreeprivate.h"
#include "ctkrbtree.h"
#include "ctkmarshalers.h"
#include "ctkintl.h"
#include "ctkprivate.h"
#include "ctktypebuiltins.h"
#include "a11y/ctktreeviewaccessibleprivate.h"


/**
 * SECTION:ctktreeselection
 * @Short_description: The selection object for GtkTreeView
 * @Title: GtkTreeSelection
 * @See_also: #GtkTreeView, #GtkTreeViewColumn, #GtkTreeModel,
 *   #GtkTreeSortable, #GtkTreeModelSort, #GtkListStore, #GtkTreeStore,
 *   #GtkCellRenderer, #GtkCellEditable, #GtkCellRendererPixbuf,
 *   #GtkCellRendererText, #GtkCellRendererToggle, [GtkTreeView drag-and-drop][ctk3-GtkTreeView-drag-and-drop]
 *
 * The #GtkTreeSelection object is a helper object to manage the selection
 * for a #GtkTreeView widget.  The #GtkTreeSelection object is
 * automatically created when a new #GtkTreeView widget is created, and
 * cannot exist independently of this widget.  The primary reason the
 * #GtkTreeSelection objects exists is for cleanliness of code and API.
 * That is, there is no conceptual reason all these functions could not be
 * methods on the #GtkTreeView widget instead of a separate function.
 *
 * The #GtkTreeSelection object is gotten from a #GtkTreeView by calling
 * ctk_tree_view_get_selection().  It can be manipulated to check the
 * selection status of the tree, as well as select and deselect individual
 * rows.  Selection is done completely view side.  As a result, multiple
 * views of the same model can have completely different selections.
 * Additionally, you cannot change the selection of a row on the model that
 * is not currently displayed by the view without expanding its parents
 * first.
 *
 * One of the important things to remember when monitoring the selection of
 * a view is that the #GtkTreeSelection::changed signal is mostly a hint.
 * That is, it may only emit one signal when a range of rows is selected.
 * Additionally, it may on occasion emit a #GtkTreeSelection::changed signal
 * when nothing has happened (mostly as a result of programmers calling
 * select_row on an already selected row).
 */

struct _GtkTreeSelectionPrivate
{
  GtkTreeView *tree_view;
  GtkSelectionMode type;
  GtkTreeSelectionFunc user_func;
  gpointer user_data;
  GDestroyNotify destroy;
};

static void ctk_tree_selection_finalize          (GObject               *object);
static gint ctk_tree_selection_real_select_all   (GtkTreeSelection      *selection);
static gint ctk_tree_selection_real_unselect_all (GtkTreeSelection      *selection);
static gint ctk_tree_selection_real_select_node  (GtkTreeSelection      *selection,
						  GtkRBTree             *tree,
						  GtkRBNode             *node,
						  gboolean               select);
static void ctk_tree_selection_set_property      (GObject               *object,
                                                  guint                  prop_id,
                                                  const GValue          *value,
                                                  GParamSpec            *pspec);
static void ctk_tree_selection_get_property      (GObject               *object,
                                                  guint                  prop_id,
                                                  GValue                *value,
                                                  GParamSpec            *pspec);

enum
{
  PROP_0,
  PROP_MODE,
  N_PROPERTIES
};

enum
{
  CHANGED,
  LAST_SIGNAL
};

static GParamSpec *properties[N_PROPERTIES];
static guint tree_selection_signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (GtkTreeSelection, ctk_tree_selection, G_TYPE_OBJECT)

static void
ctk_tree_selection_class_init (GtkTreeSelectionClass *class)
{
  GObjectClass *object_class;

  object_class = (GObjectClass*) class;

  object_class->finalize = ctk_tree_selection_finalize;
  object_class->set_property = ctk_tree_selection_set_property;
  object_class->get_property = ctk_tree_selection_get_property;
  class->changed = NULL;

  /* Properties */
  
  /**
   * GtkTreeSelection:mode:
   *
   * Selection mode.
   * See ctk_tree_selection_set_mode() for more information on this property.
   *
   * Since: 3.2
   */
  properties[PROP_MODE] = g_param_spec_enum ("mode",
                                             P_("Mode"),
                                             P_("Selection mode"),
                                             CTK_TYPE_SELECTION_MODE,
                                             CTK_SELECTION_SINGLE,
                                             CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /* Install all properties */
  g_object_class_install_properties (object_class, N_PROPERTIES, properties);
  
  /* Signals */
  
  /**
   * GtkTreeSelection::changed:
   * @treeselection: the object which received the signal.
   *
   * Emitted whenever the selection has (possibly) changed. Please note that
   * this signal is mostly a hint.  It may only be emitted once when a range
   * of rows are selected, and it may occasionally be emitted when nothing
   * has happened.
   */
  tree_selection_signals[CHANGED] =
    g_signal_new (I_("changed"),
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GtkTreeSelectionClass, changed),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);
}

static void
ctk_tree_selection_init (GtkTreeSelection *selection)
{
  selection->priv = ctk_tree_selection_get_instance_private (selection);
  selection->priv->type = CTK_SELECTION_SINGLE;
}

static void
ctk_tree_selection_finalize (GObject *object)
{
  GtkTreeSelection *selection = CTK_TREE_SELECTION (object);
  GtkTreeSelectionPrivate *priv = selection->priv;

  if (priv->destroy)
    priv->destroy (priv->user_data);

  /* chain parent_class' handler */
  G_OBJECT_CLASS (ctk_tree_selection_parent_class)->finalize (object);
}

static void
ctk_tree_selection_set_property (GObject *object,
                                 guint prop_id,
                                 const GValue *value,
                                 GParamSpec *pspec)
{
  g_return_if_fail (CTK_IS_TREE_SELECTION (object));

  switch (prop_id)
    {
      case PROP_MODE:
        ctk_tree_selection_set_mode (CTK_TREE_SELECTION (object), g_value_get_enum (value));
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
ctk_tree_selection_get_property (GObject *object,
                                 guint prop_id,
                                 GValue *value,
                                 GParamSpec *pspec)
{
  g_return_if_fail (CTK_IS_TREE_SELECTION (object));

  switch (prop_id)
    {
      case PROP_MODE:
        g_value_set_enum (value, ctk_tree_selection_get_mode (CTK_TREE_SELECTION (object)));
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

/**
 * _ctk_tree_selection_new:
 *
 * Creates a new #GtkTreeSelection object.  This function should not be invoked,
 * as each #GtkTreeView will create its own #GtkTreeSelection.
 *
 * Returns: A newly created #GtkTreeSelection object.
 **/
GtkTreeSelection*
_ctk_tree_selection_new (void)
{
  GtkTreeSelection *selection;

  selection = g_object_new (CTK_TYPE_TREE_SELECTION, NULL);

  return selection;
}

/**
 * _ctk_tree_selection_new_with_tree_view:
 * @tree_view: The #GtkTreeView.
 *
 * Creates a new #GtkTreeSelection object.  This function should not be invoked,
 * as each #GtkTreeView will create its own #GtkTreeSelection.
 *
 * Returns: A newly created #GtkTreeSelection object.
 **/
GtkTreeSelection*
_ctk_tree_selection_new_with_tree_view (GtkTreeView *tree_view)
{
  GtkTreeSelection *selection;

  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), NULL);

  selection = _ctk_tree_selection_new ();
  _ctk_tree_selection_set_tree_view (selection, tree_view);

  return selection;
}

/**
 * _ctk_tree_selection_set_tree_view:
 * @selection: A #GtkTreeSelection.
 * @tree_view: The #GtkTreeView.
 *
 * Sets the #GtkTreeView of @selection.  This function should not be invoked, as
 * it is used internally by #GtkTreeView.
 **/
void
_ctk_tree_selection_set_tree_view (GtkTreeSelection *selection,
                                   GtkTreeView      *tree_view)
{
  GtkTreeSelectionPrivate *priv;

  g_return_if_fail (CTK_IS_TREE_SELECTION (selection));
  if (tree_view != NULL)
    g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  priv = selection->priv;

  priv->tree_view = tree_view;
}

/**
 * ctk_tree_selection_set_mode:
 * @selection: A #GtkTreeSelection.
 * @type: The selection mode
 *
 * Sets the selection mode of the @selection.  If the previous type was
 * #CTK_SELECTION_MULTIPLE, then the anchor is kept selected, if it was
 * previously selected.
 **/
void
ctk_tree_selection_set_mode (GtkTreeSelection *selection,
			     GtkSelectionMode  type)
{
  GtkTreeSelectionPrivate *priv;
  GtkTreeSelectionFunc tmp_func;

  g_return_if_fail (CTK_IS_TREE_SELECTION (selection));

  priv = selection->priv;

  if (priv->type == type)
    return;

  if (type == CTK_SELECTION_NONE)
    {
      /* We do this so that we unconditionally unset all rows
       */
      tmp_func = priv->user_func;
      priv->user_func = NULL;
      ctk_tree_selection_unselect_all (selection);
      priv->user_func = tmp_func;

      _ctk_tree_view_set_anchor_path (priv->tree_view, NULL);
    }
  else if (type == CTK_SELECTION_SINGLE ||
	   type == CTK_SELECTION_BROWSE)
    {
      GtkRBTree *tree = NULL;
      GtkRBNode *node = NULL;
      gint selected = FALSE;
      GtkTreePath *anchor_path = NULL;

      anchor_path = _ctk_tree_view_get_anchor_path (priv->tree_view);

      if (anchor_path)
	{
	  _ctk_tree_view_find_node (priv->tree_view,
				    anchor_path,
				    &tree,
				    &node);
	  
	  if (node && CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_IS_SELECTED))
	    selected = TRUE;
	}

      /* We do this so that we unconditionally unset all rows
       */
      tmp_func = priv->user_func;
      priv->user_func = NULL;
      ctk_tree_selection_unselect_all (selection);
      priv->user_func = tmp_func;

      if (node && selected)
	_ctk_tree_selection_internal_select_node (selection,
						  node,
						  tree,
						  anchor_path,
                                                  0,
						  FALSE);
      if (anchor_path)
	ctk_tree_path_free (anchor_path);
    }

  priv->type = type;
  
  g_object_notify_by_pspec (G_OBJECT (selection), properties[PROP_MODE]);
}

/**
 * ctk_tree_selection_get_mode:
 * @selection: a #GtkTreeSelection
 *
 * Gets the selection mode for @selection. See
 * ctk_tree_selection_set_mode().
 *
 * Returns: the current selection mode
 **/
GtkSelectionMode
ctk_tree_selection_get_mode (GtkTreeSelection *selection)
{
  g_return_val_if_fail (CTK_IS_TREE_SELECTION (selection), CTK_SELECTION_SINGLE);

  return selection->priv->type;
}

/**
 * ctk_tree_selection_set_select_function:
 * @selection: A #GtkTreeSelection.
 * @func: (nullable): The selection function. May be %NULL
 * @data: The selection function’s data. May be %NULL
 * @destroy: The destroy function for user data.  May be %NULL
 *
 * Sets the selection function.
 *
 * If set, this function is called before any node is selected or unselected,
 * giving some control over which nodes are selected. The select function
 * should return %TRUE if the state of the node may be toggled, and %FALSE
 * if the state of the node should be left unchanged.
 */
void
ctk_tree_selection_set_select_function (GtkTreeSelection     *selection,
					GtkTreeSelectionFunc  func,
					gpointer              data,
					GDestroyNotify        destroy)
{
  GtkTreeSelectionPrivate *priv;

  g_return_if_fail (CTK_IS_TREE_SELECTION (selection));

  priv = selection->priv;

  if (priv->destroy)
    priv->destroy (priv->user_data);

  priv->user_func = func;
  priv->user_data = data;
  priv->destroy = destroy;
}

/**
 * ctk_tree_selection_get_select_function: (skip)
 * @selection: A #GtkTreeSelection.
 *
 * Returns the current selection function.
 *
 * Returns: The function.
 *
 * Since: 2.14
 **/
GtkTreeSelectionFunc
ctk_tree_selection_get_select_function (GtkTreeSelection *selection)
{
  g_return_val_if_fail (CTK_IS_TREE_SELECTION (selection), NULL);

  return selection->priv->user_func;
}

/**
 * ctk_tree_selection_get_user_data: (skip)
 * @selection: A #GtkTreeSelection.
 *
 * Returns the user data for the selection function.
 *
 * Returns: The user data.
 **/
gpointer
ctk_tree_selection_get_user_data (GtkTreeSelection *selection)
{
  g_return_val_if_fail (CTK_IS_TREE_SELECTION (selection), NULL);

  return selection->priv->user_data;
}

/**
 * ctk_tree_selection_get_tree_view:
 * @selection: A #GtkTreeSelection
 * 
 * Returns the tree view associated with @selection.
 * 
 * Returns: (transfer none): A #GtkTreeView
 **/
GtkTreeView *
ctk_tree_selection_get_tree_view (GtkTreeSelection *selection)
{
  g_return_val_if_fail (CTK_IS_TREE_SELECTION (selection), NULL);

  return selection->priv->tree_view;
}

/**
 * ctk_tree_selection_get_selected:
 * @selection: A #GtkTreeSelection.
 * @model: (out) (allow-none) (transfer none): A pointer to set to the #GtkTreeModel, or NULL.
 * @iter: (out) (allow-none): The #GtkTreeIter, or NULL.
 *
 * Sets @iter to the currently selected node if @selection is set to
 * #CTK_SELECTION_SINGLE or #CTK_SELECTION_BROWSE.  @iter may be NULL if you
 * just want to test if @selection has any selected nodes.  @model is filled
 * with the current model as a convenience.  This function will not work if you
 * use @selection is #CTK_SELECTION_MULTIPLE.
 *
 * Returns: TRUE, if there is a selected node.
 **/
gboolean
ctk_tree_selection_get_selected (GtkTreeSelection  *selection,
				 GtkTreeModel     **model,
				 GtkTreeIter       *iter)
{
  GtkTreeSelectionPrivate *priv;
  GtkRBTree *tree;
  GtkRBNode *node;
  GtkTreePath *anchor_path;
  gboolean retval = FALSE;
  gboolean found_node;

  g_return_val_if_fail (CTK_IS_TREE_SELECTION (selection), FALSE);

  priv = selection->priv;

  g_return_val_if_fail (priv->type != CTK_SELECTION_MULTIPLE, FALSE);
  g_return_val_if_fail (priv->tree_view != NULL, FALSE);

  /* Clear the iter */
  if (iter)
    memset (iter, 0, sizeof (GtkTreeIter));

  if (model)
    *model = ctk_tree_view_get_model (priv->tree_view);

  anchor_path = _ctk_tree_view_get_anchor_path (priv->tree_view);

  if (anchor_path == NULL)
    return FALSE;

  found_node = !_ctk_tree_view_find_node (priv->tree_view,
                                          anchor_path,
                                          &tree,
                                          &node);

  if (found_node && CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_IS_SELECTED))
    {
      /* we only want to return the anchor if it exists in the rbtree and
       * is selected.
       */
      if (iter == NULL)
	retval = TRUE;
      else
        retval = ctk_tree_model_get_iter (ctk_tree_view_get_model (priv->tree_view),
                                          iter,
                                          anchor_path);
    }
  else
    {
      /* We don't want to return the anchor if it isn't actually selected.
       */
      retval = FALSE;
    }

  ctk_tree_path_free (anchor_path);

  return retval;
}

/**
 * ctk_tree_selection_get_selected_rows:
 * @selection: A #GtkTreeSelection.
 * @model: (out) (allow-none) (transfer none): A pointer to set to the #GtkTreeModel, or %NULL.
 *
 * Creates a list of path of all selected rows. Additionally, if you are
 * planning on modifying the model after calling this function, you may
 * want to convert the returned list into a list of #GtkTreeRowReferences.
 * To do this, you can use ctk_tree_row_reference_new().
 *
 * To free the return value, use:
 * |[<!-- language="C" -->
 * g_list_free_full (list, (GDestroyNotify) ctk_tree_path_free);
 * ]|
 *
 * Returns: (element-type GtkTreePath) (transfer full): A #GList containing a #GtkTreePath for each selected row.
 *
 * Since: 2.2
 **/
GList *
ctk_tree_selection_get_selected_rows (GtkTreeSelection   *selection,
                                      GtkTreeModel      **model)
{
  GtkTreeSelectionPrivate *priv;
  GList *list = NULL;
  GtkRBTree *tree = NULL;
  GtkRBNode *node = NULL;
  GtkTreePath *path;

  g_return_val_if_fail (CTK_IS_TREE_SELECTION (selection), NULL);

  priv = selection->priv;

  g_return_val_if_fail (priv->tree_view != NULL, NULL);

  if (model)
    *model = ctk_tree_view_get_model (priv->tree_view);

  tree = _ctk_tree_view_get_rbtree (priv->tree_view);

  if (tree == NULL || tree->root == NULL)
    return NULL;

  if (priv->type == CTK_SELECTION_NONE)
    return NULL;
  else if (priv->type != CTK_SELECTION_MULTIPLE)
    {
      GtkTreeIter iter;

      if (ctk_tree_selection_get_selected (selection, NULL, &iter))
        {
	  path = ctk_tree_model_get_path (ctk_tree_view_get_model (priv->tree_view), &iter);
	  list = g_list_append (list, path);

	  return list;
	}

      return NULL;
    }

  node = _ctk_rbtree_first (tree);
  path = ctk_tree_path_new_first ();

  while (node != NULL)
    {
      if (CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_IS_SELECTED))
	list = g_list_prepend (list, ctk_tree_path_copy (path));

      if (node->children)
        {
	  tree = node->children;
          node = _ctk_rbtree_first (tree);

	  ctk_tree_path_append_index (path, 0);
	}
      else
        {
	  gboolean done = FALSE;

	  do
	    {
	      node = _ctk_rbtree_next (tree, node);
	      if (node != NULL)
	        {
		  done = TRUE;
		  ctk_tree_path_next (path);
		}
	      else
	        {
		  node = tree->parent_node;
		  tree = tree->parent_tree;

		  if (!tree)
		    {
		      ctk_tree_path_free (path);

		      goto done; 
		    }

		  ctk_tree_path_up (path);
		}
	    }
	  while (!done);
	}
    }

  ctk_tree_path_free (path);

 done:
  return g_list_reverse (list);
}

static void
ctk_tree_selection_count_selected_rows_helper (GtkRBTree *tree,
					       GtkRBNode *node,
					       gpointer   data)
{
  gint *count = (gint *)data;

  g_return_if_fail (node != NULL);

  if (CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_IS_SELECTED))
    (*count)++;

  if (node->children)
    _ctk_rbtree_traverse (node->children, node->children->root,
			  G_PRE_ORDER,
			  ctk_tree_selection_count_selected_rows_helper, data);
}

/**
 * ctk_tree_selection_count_selected_rows:
 * @selection: A #GtkTreeSelection.
 *
 * Returns the number of rows that have been selected in @tree.
 *
 * Returns: The number of rows selected.
 * 
 * Since: 2.2
 **/
gint
ctk_tree_selection_count_selected_rows (GtkTreeSelection *selection)
{
  GtkTreeSelectionPrivate *priv;
  gint count = 0;
  GtkRBTree *tree;

  g_return_val_if_fail (CTK_IS_TREE_SELECTION (selection), 0);

  priv = selection->priv;

  g_return_val_if_fail (priv->tree_view != NULL, 0);

  tree = _ctk_tree_view_get_rbtree (priv->tree_view);

  if (tree == NULL || tree->root == NULL)
    return 0;

  if (priv->type == CTK_SELECTION_SINGLE ||
      priv->type == CTK_SELECTION_BROWSE)
    {
      if (ctk_tree_selection_get_selected (selection, NULL, NULL))
	return 1;
      else
	return 0;
    }

  _ctk_rbtree_traverse (tree, tree->root,
			G_PRE_ORDER,
			ctk_tree_selection_count_selected_rows_helper,
			&count);

  return count;
}

/* ctk_tree_selection_selected_foreach helper */
static void
model_changed (gpointer data)
{
  gboolean *stop = (gboolean *)data;

  *stop = TRUE;
}

/**
 * ctk_tree_selection_selected_foreach:
 * @selection: A #GtkTreeSelection.
 * @func: (scope call): The function to call for each selected node.
 * @data: user data to pass to the function.
 *
 * Calls a function for each selected node. Note that you cannot modify
 * the tree or selection from within this function. As a result,
 * ctk_tree_selection_get_selected_rows() might be more useful.
 **/
void
ctk_tree_selection_selected_foreach (GtkTreeSelection            *selection,
				     GtkTreeSelectionForeachFunc  func,
				     gpointer                     data)
{
  GtkTreeSelectionPrivate *priv;
  GtkTreePath *path;
  GtkRBTree *tree;
  GtkRBNode *node;
  GtkTreeIter iter;
  GtkTreeModel *model;

  gulong inserted_id, deleted_id, reordered_id, changed_id;
  gboolean stop = FALSE;

  g_return_if_fail (CTK_IS_TREE_SELECTION (selection));

  priv = selection->priv;

  g_return_if_fail (priv->tree_view != NULL);

  tree = _ctk_tree_view_get_rbtree (priv->tree_view);

  if (func == NULL || tree == NULL || tree->root == NULL)
    return;

  model = ctk_tree_view_get_model (priv->tree_view);

  if (priv->type == CTK_SELECTION_SINGLE ||
      priv->type == CTK_SELECTION_BROWSE)
    {
      path = _ctk_tree_view_get_anchor_path (priv->tree_view);

      if (path)
	{
	  ctk_tree_model_get_iter (model, &iter, path);
	  (* func) (model, path, &iter, data);
	  ctk_tree_path_free (path);
	}
      return;
    }

  node = _ctk_rbtree_first (tree);

  g_object_ref (model);

  /* connect to signals to monitor changes in treemodel */
  inserted_id = g_signal_connect_swapped (model, "row-inserted",
					  G_CALLBACK (model_changed),
				          &stop);
  deleted_id = g_signal_connect_swapped (model, "row-deleted",
					 G_CALLBACK (model_changed),
				         &stop);
  reordered_id = g_signal_connect_swapped (model, "rows-reordered",
					   G_CALLBACK (model_changed),
				           &stop);
  changed_id = g_signal_connect_swapped (priv->tree_view, "notify::model",
					 G_CALLBACK (model_changed), 
					 &stop);

  /* find the node internally */
  path = ctk_tree_path_new_first ();

  while (node != NULL)
    {
      if (CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_IS_SELECTED))
        {
          ctk_tree_model_get_iter (model, &iter, path);
	  (* func) (model, path, &iter, data);
        }

      if (stop)
	goto out;

      if (node->children)
	{
	  tree = node->children;
          node = _ctk_rbtree_first (tree);

	  ctk_tree_path_append_index (path, 0);
	}
      else
	{
	  gboolean done = FALSE;

	  do
	    {
	      node = _ctk_rbtree_next (tree, node);
	      if (node != NULL)
		{
		  done = TRUE;
		  ctk_tree_path_next (path);
		}
	      else
		{
		  node = tree->parent_node;
		  tree = tree->parent_tree;

		  if (tree == NULL)
		    {
		      /* we've run out of tree */
		      /* We're done with this function */

		      goto out;
		    }

		  ctk_tree_path_up (path);
		}
	    }
	  while (!done);
	}
    }

out:
  if (path)
    ctk_tree_path_free (path);

  g_signal_handler_disconnect (model, inserted_id);
  g_signal_handler_disconnect (model, deleted_id);
  g_signal_handler_disconnect (model, reordered_id);
  g_signal_handler_disconnect (priv->tree_view, changed_id);
  g_object_unref (model);

  /* check if we have to spew a scary message */
  if (stop)
    g_warning ("The model has been modified from within ctk_tree_selection_selected_foreach.\n"
	       "This function is for observing the selections of the tree only.  If\n"
	       "you are trying to get all selected items from the tree, try using\n"
	       "ctk_tree_selection_get_selected_rows instead.");
}

/**
 * ctk_tree_selection_select_path:
 * @selection: A #GtkTreeSelection.
 * @path: The #GtkTreePath to be selected.
 *
 * Select the row at @path.
 **/
void
ctk_tree_selection_select_path (GtkTreeSelection *selection,
				GtkTreePath      *path)
{
  GtkTreeSelectionPrivate *priv;
  GtkRBNode *node;
  GtkRBTree *tree;
  gboolean ret;
  GtkTreeSelectMode mode = 0;

  g_return_if_fail (CTK_IS_TREE_SELECTION (selection));

  priv = selection->priv;

  g_return_if_fail (priv->tree_view != NULL);
  g_return_if_fail (path != NULL);

  ret = _ctk_tree_view_find_node (priv->tree_view,
				  path,
				  &tree,
				  &node);

  if (node == NULL || CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_IS_SELECTED) ||
      ret == TRUE)
    return;

  if (priv->type == CTK_SELECTION_MULTIPLE)
    mode = CTK_TREE_SELECT_MODE_TOGGLE;

  _ctk_tree_selection_internal_select_node (selection,
					    node,
					    tree,
					    path,
                                            mode,
					    FALSE);
}

/**
 * ctk_tree_selection_unselect_path:
 * @selection: A #GtkTreeSelection.
 * @path: The #GtkTreePath to be unselected.
 *
 * Unselects the row at @path.
 **/
void
ctk_tree_selection_unselect_path (GtkTreeSelection *selection,
				  GtkTreePath      *path)
{
  GtkTreeSelectionPrivate *priv;
  GtkRBNode *node;
  GtkRBTree *tree;
  gboolean ret;

  g_return_if_fail (CTK_IS_TREE_SELECTION (selection));

  priv = selection->priv;

  g_return_if_fail (priv->tree_view != NULL);
  g_return_if_fail (path != NULL);

  ret = _ctk_tree_view_find_node (priv->tree_view,
				  path,
				  &tree,
				  &node);

  if (node == NULL || !CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_IS_SELECTED) ||
      ret == TRUE)
    return;

  _ctk_tree_selection_internal_select_node (selection,
					    node,
					    tree,
					    path,
                                            CTK_TREE_SELECT_MODE_TOGGLE,
					    TRUE);
}

/**
 * ctk_tree_selection_select_iter:
 * @selection: A #GtkTreeSelection.
 * @iter: The #GtkTreeIter to be selected.
 *
 * Selects the specified iterator.
 **/
void
ctk_tree_selection_select_iter (GtkTreeSelection *selection,
				GtkTreeIter      *iter)
{
  GtkTreeSelectionPrivate *priv;
  GtkTreePath *path;
  GtkTreeModel *model;

  g_return_if_fail (CTK_IS_TREE_SELECTION (selection));

  priv = selection->priv;

  g_return_if_fail (priv->tree_view != NULL);

  model = ctk_tree_view_get_model (priv->tree_view);
  g_return_if_fail (model != NULL);
  g_return_if_fail (iter != NULL);

  path = ctk_tree_model_get_path (model, iter);

  if (path == NULL)
    return;

  ctk_tree_selection_select_path (selection, path);
  ctk_tree_path_free (path);
}


/**
 * ctk_tree_selection_unselect_iter:
 * @selection: A #GtkTreeSelection.
 * @iter: The #GtkTreeIter to be unselected.
 *
 * Unselects the specified iterator.
 **/
void
ctk_tree_selection_unselect_iter (GtkTreeSelection *selection,
				  GtkTreeIter      *iter)
{
  GtkTreeSelectionPrivate *priv;
  GtkTreePath *path;
  GtkTreeModel *model;

  g_return_if_fail (CTK_IS_TREE_SELECTION (selection));

  priv = selection->priv;

  g_return_if_fail (priv->tree_view != NULL);

  model = ctk_tree_view_get_model (priv->tree_view);
  g_return_if_fail (model != NULL);
  g_return_if_fail (iter != NULL);

  path = ctk_tree_model_get_path (model, iter);

  if (path == NULL)
    return;

  ctk_tree_selection_unselect_path (selection, path);
  ctk_tree_path_free (path);
}

/**
 * ctk_tree_selection_path_is_selected:
 * @selection: A #GtkTreeSelection.
 * @path: A #GtkTreePath to check selection on.
 * 
 * Returns %TRUE if the row pointed to by @path is currently selected.  If @path
 * does not point to a valid location, %FALSE is returned
 * 
 * Returns: %TRUE if @path is selected.
 **/
gboolean
ctk_tree_selection_path_is_selected (GtkTreeSelection *selection,
				     GtkTreePath      *path)
{
  GtkTreeSelectionPrivate *priv;
  GtkRBNode *node;
  GtkRBTree *tree;
  gboolean ret;

  g_return_val_if_fail (CTK_IS_TREE_SELECTION (selection), FALSE);

  priv = selection->priv;

  g_return_val_if_fail (path != NULL, FALSE);
  g_return_val_if_fail (priv->tree_view != NULL, FALSE);

  if (ctk_tree_view_get_model (priv->tree_view) == NULL)
    return FALSE;

  ret = _ctk_tree_view_find_node (priv->tree_view,
				  path,
				  &tree,
				  &node);

  if ((node == NULL) || !CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_IS_SELECTED) ||
      ret == TRUE)
    return FALSE;

  return TRUE;
}

/**
 * ctk_tree_selection_iter_is_selected:
 * @selection: A #GtkTreeSelection
 * @iter: A valid #GtkTreeIter
 * 
 * Returns %TRUE if the row at @iter is currently selected.
 * 
 * Returns: %TRUE, if @iter is selected
 **/
gboolean
ctk_tree_selection_iter_is_selected (GtkTreeSelection *selection,
				     GtkTreeIter      *iter)
{
  GtkTreeSelectionPrivate *priv;
  GtkTreePath *path;
  GtkTreeModel *model;
  gboolean retval;

  g_return_val_if_fail (CTK_IS_TREE_SELECTION (selection), FALSE);

  priv = selection->priv;

  g_return_val_if_fail (iter != NULL, FALSE);
  g_return_val_if_fail (priv->tree_view != NULL, FALSE);

  model = ctk_tree_view_get_model (priv->tree_view);
  g_return_val_if_fail (model != NULL, FALSE);

  path = ctk_tree_model_get_path (model, iter);
  if (path == NULL)
    return FALSE;

  retval = ctk_tree_selection_path_is_selected (selection, path);
  ctk_tree_path_free (path);

  return retval;
}


/* Wish I was in python, right now... */
struct _TempTuple {
  GtkTreeSelection *selection;
  gint dirty;
};

static void
select_all_helper (GtkRBTree  *tree,
		   GtkRBNode  *node,
		   gpointer    data)
{
  struct _TempTuple *tuple = data;

  if (node->children)
    _ctk_rbtree_traverse (node->children,
			  node->children->root,
			  G_PRE_ORDER,
			  select_all_helper,
			  data);
  if (!CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_IS_SELECTED))
    {
      tuple->dirty = ctk_tree_selection_real_select_node (tuple->selection, tree, node, TRUE) || tuple->dirty;
    }
}


/* We have a real_{un,}select_all function that doesn't emit the signal, so we
 * can use it in other places without fear of the signal being emitted.
 */
static gint
ctk_tree_selection_real_select_all (GtkTreeSelection *selection)
{
  GtkTreeSelectionPrivate *priv = selection->priv;
  struct _TempTuple *tuple;
  GtkRBTree *tree;

  tree = _ctk_tree_view_get_rbtree (priv->tree_view);

  if (tree == NULL)
    return FALSE;

  /* Mark all nodes selected */
  tuple = g_new (struct _TempTuple, 1);
  tuple->selection = selection;
  tuple->dirty = FALSE;

  _ctk_rbtree_traverse (tree, tree->root,
			G_PRE_ORDER,
			select_all_helper,
			tuple);
  if (tuple->dirty)
    {
      g_free (tuple);
      return TRUE;
    }
  g_free (tuple);
  return FALSE;
}

/**
 * ctk_tree_selection_select_all:
 * @selection: A #GtkTreeSelection.
 *
 * Selects all the nodes. @selection must be set to #CTK_SELECTION_MULTIPLE
 * mode.
 **/
void
ctk_tree_selection_select_all (GtkTreeSelection *selection)
{
  GtkTreeSelectionPrivate *priv;

  g_return_if_fail (CTK_IS_TREE_SELECTION (selection));

  priv = selection->priv;

  g_return_if_fail (priv->tree_view != NULL);

  if (_ctk_tree_view_get_rbtree (priv->tree_view) == NULL ||
      ctk_tree_view_get_model (priv->tree_view) == NULL)
    return;

  g_return_if_fail (priv->type == CTK_SELECTION_MULTIPLE);

  if (ctk_tree_selection_real_select_all (selection))
    g_signal_emit (selection, tree_selection_signals[CHANGED], 0);
}

static void
unselect_all_helper (GtkRBTree  *tree,
		     GtkRBNode  *node,
		     gpointer    data)
{
  struct _TempTuple *tuple = data;

  if (node->children)
    _ctk_rbtree_traverse (node->children,
			  node->children->root,
			  G_PRE_ORDER,
			  unselect_all_helper,
			  data);
  if (CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_IS_SELECTED))
    {
      tuple->dirty = ctk_tree_selection_real_select_node (tuple->selection, tree, node, FALSE) || tuple->dirty;
    }
}

static gboolean
ctk_tree_selection_real_unselect_all (GtkTreeSelection *selection)
{
  GtkTreeSelectionPrivate *priv = selection->priv;
  struct _TempTuple *tuple;

  if (priv->type == CTK_SELECTION_SINGLE ||
      priv->type == CTK_SELECTION_BROWSE)
    {
      GtkRBTree *tree = NULL;
      GtkRBNode *node = NULL;
      GtkTreePath *anchor_path;

      anchor_path = _ctk_tree_view_get_anchor_path (priv->tree_view);

      if (anchor_path == NULL)
        return FALSE;

      _ctk_tree_view_find_node (priv->tree_view,
                                anchor_path,
				&tree,
				&node);

      ctk_tree_path_free (anchor_path);

      if (tree == NULL)
        return FALSE;

      if (CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_IS_SELECTED))
	{
	  if (ctk_tree_selection_real_select_node (selection, tree, node, FALSE))
	    {
	      _ctk_tree_view_set_anchor_path (priv->tree_view, NULL);
	      return TRUE;
	    }
	}
      return FALSE;
    }
  else
    {
      GtkRBTree *tree;

      tuple = g_new (struct _TempTuple, 1);
      tuple->selection = selection;
      tuple->dirty = FALSE;

      tree = _ctk_tree_view_get_rbtree (priv->tree_view);
      _ctk_rbtree_traverse (tree, tree->root,
                            G_PRE_ORDER,
                            unselect_all_helper,
                            tuple);

      if (tuple->dirty)
        {
          g_free (tuple);
          return TRUE;
        }
      g_free (tuple);
      return FALSE;
    }
}

/**
 * ctk_tree_selection_unselect_all:
 * @selection: A #GtkTreeSelection.
 *
 * Unselects all the nodes.
 **/
void
ctk_tree_selection_unselect_all (GtkTreeSelection *selection)
{
  GtkTreeSelectionPrivate *priv;

  g_return_if_fail (CTK_IS_TREE_SELECTION (selection));

  priv = selection->priv;

  g_return_if_fail (priv->tree_view != NULL);

  if (_ctk_tree_view_get_rbtree (priv->tree_view) == NULL ||
      ctk_tree_view_get_model (priv->tree_view) == NULL)
    return;
  
  if (ctk_tree_selection_real_unselect_all (selection))
    g_signal_emit (selection, tree_selection_signals[CHANGED], 0);
}

enum
{
  RANGE_SELECT,
  RANGE_UNSELECT
};

static gint
ctk_tree_selection_real_modify_range (GtkTreeSelection *selection,
                                      gint              mode,
				      GtkTreePath      *start_path,
				      GtkTreePath      *end_path)
{
  GtkTreeSelectionPrivate *priv = selection->priv;
  GtkRBNode *start_node = NULL, *end_node = NULL;
  GtkRBTree *start_tree, *end_tree;
  GtkTreePath *anchor_path = NULL;
  gboolean dirty = FALSE;

  switch (ctk_tree_path_compare (start_path, end_path))
    {
    case 1:
      _ctk_tree_view_find_node (priv->tree_view,
				end_path,
				&start_tree,
				&start_node);
      _ctk_tree_view_find_node (priv->tree_view,
				start_path,
				&end_tree,
				&end_node);
      anchor_path = start_path;
      break;
    case 0:
      _ctk_tree_view_find_node (priv->tree_view,
				start_path,
				&start_tree,
				&start_node);
      end_tree = start_tree;
      end_node = start_node;
      anchor_path = start_path;
      break;
    case -1:
      _ctk_tree_view_find_node (priv->tree_view,
				start_path,
				&start_tree,
				&start_node);
      _ctk_tree_view_find_node (priv->tree_view,
				end_path,
				&end_tree,
				&end_node);
      anchor_path = start_path;
      break;
    }

  /* Invalid start or end node? */
  if (start_node == NULL || end_node == NULL)
    return dirty;

  if (anchor_path)
    _ctk_tree_view_set_anchor_path (priv->tree_view, anchor_path);

  do
    {
      dirty |= ctk_tree_selection_real_select_node (selection, start_tree, start_node, (mode == RANGE_SELECT)?TRUE:FALSE);

      if (start_node == end_node)
	break;

      if (start_node->children)
	{
	  start_tree = start_node->children;
          start_node = _ctk_rbtree_first (start_tree);
	}
      else
	{
	  _ctk_rbtree_next_full (start_tree, start_node, &start_tree, &start_node);
	  if (start_tree == NULL)
	    {
	      /* we just ran out of tree.  That means someone passed in bogus values.
	       */
	      return dirty;
	    }
	}
    }
  while (TRUE);

  return dirty;
}

/**
 * ctk_tree_selection_select_range:
 * @selection: A #GtkTreeSelection.
 * @start_path: The initial node of the range.
 * @end_path: The final node of the range.
 *
 * Selects a range of nodes, determined by @start_path and @end_path inclusive.
 * @selection must be set to #CTK_SELECTION_MULTIPLE mode. 
 **/
void
ctk_tree_selection_select_range (GtkTreeSelection *selection,
				 GtkTreePath      *start_path,
				 GtkTreePath      *end_path)
{
  GtkTreeSelectionPrivate *priv;

  g_return_if_fail (CTK_IS_TREE_SELECTION (selection));

  priv = selection->priv;

  g_return_if_fail (priv->tree_view != NULL);
  g_return_if_fail (priv->type == CTK_SELECTION_MULTIPLE);
  g_return_if_fail (ctk_tree_view_get_model (priv->tree_view) != NULL);

  if (ctk_tree_selection_real_modify_range (selection, RANGE_SELECT, start_path, end_path))
    g_signal_emit (selection, tree_selection_signals[CHANGED], 0);
}

/**
 * ctk_tree_selection_unselect_range:
 * @selection: A #GtkTreeSelection.
 * @start_path: The initial node of the range.
 * @end_path: The initial node of the range.
 *
 * Unselects a range of nodes, determined by @start_path and @end_path
 * inclusive.
 *
 * Since: 2.2
 **/
void
ctk_tree_selection_unselect_range (GtkTreeSelection *selection,
                                   GtkTreePath      *start_path,
				   GtkTreePath      *end_path)
{
  GtkTreeSelectionPrivate *priv;

  g_return_if_fail (CTK_IS_TREE_SELECTION (selection));

  priv = selection->priv;

  g_return_if_fail (priv->tree_view != NULL);
  g_return_if_fail (ctk_tree_view_get_model (priv->tree_view) != NULL);

  if (ctk_tree_selection_real_modify_range (selection, RANGE_UNSELECT, start_path, end_path))
    g_signal_emit (selection, tree_selection_signals[CHANGED], 0);
}

gboolean
_ctk_tree_selection_row_is_selectable (GtkTreeSelection *selection,
				       GtkRBNode        *node,
				       GtkTreePath      *path)
{
  GtkTreeSelectionPrivate *priv = selection->priv;
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkTreeViewRowSeparatorFunc separator_func;
  gpointer separator_data;
  gboolean sensitive = FALSE;

  model = ctk_tree_view_get_model (priv->tree_view);

  _ctk_tree_view_get_row_separator_func (priv->tree_view,
					 &separator_func, &separator_data);

  if (!ctk_tree_model_get_iter (model, &iter, path))
    sensitive = TRUE;

  if (!sensitive && separator_func)
    {
      /* never allow separators to be selected */
      if ((* separator_func) (model, &iter, separator_data))
	return FALSE;
    }

  if (priv->user_func)
    return (*priv->user_func) (selection, model, path,
				    CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_IS_SELECTED),
				    priv->user_data);
  else
    return TRUE;
}


/* Called internally by ctktreeview.c It handles actually selecting the tree.
 */

/*
 * docs about the “override_browse_mode”, we set this flag when we want to
 * unset select the node and override the select browse mode behaviour (that is
 * 'one node should *always* be selected').
 */
void
_ctk_tree_selection_internal_select_node (GtkTreeSelection *selection,
					  GtkRBNode        *node,
					  GtkRBTree        *tree,
					  GtkTreePath      *path,
                                          GtkTreeSelectMode mode,
					  gboolean          override_browse_mode)
{
  GtkTreeSelectionPrivate *priv = selection->priv;
  gint flags;
  gint dirty = FALSE;
  GtkTreePath *anchor_path = NULL;

  if (priv->type == CTK_SELECTION_NONE)
    return;

  anchor_path = _ctk_tree_view_get_anchor_path (priv->tree_view);

  if (priv->type == CTK_SELECTION_SINGLE ||
      priv->type == CTK_SELECTION_BROWSE)
    {
      /* just unselect */
      if (priv->type == CTK_SELECTION_BROWSE && override_browse_mode)
        {
	  dirty = ctk_tree_selection_real_unselect_all (selection);
	}
      /* Did we try to select the same node again? */
      else if (priv->type == CTK_SELECTION_SINGLE &&
	       anchor_path && ctk_tree_path_compare (path, anchor_path) == 0)
	{
	  if ((mode & CTK_TREE_SELECT_MODE_TOGGLE) == CTK_TREE_SELECT_MODE_TOGGLE)
	    {
	      dirty = ctk_tree_selection_real_unselect_all (selection);
	    }
	}
      else
	{
	  if (anchor_path)
	    {
	      /* We only want to select the new node if we can unselect the old one,
	       * and we can select the new one. */
	      dirty = _ctk_tree_selection_row_is_selectable (selection, node, path);

	      /* if dirty is FALSE, we weren't able to select the new one, otherwise, we try to
	       * unselect the new one
	       */
	      if (dirty)
		dirty = ctk_tree_selection_real_unselect_all (selection);

	      /* if dirty is TRUE at this point, we successfully unselected the
	       * old one, and can then select the new one */
	      if (dirty)
		{

		  _ctk_tree_view_set_anchor_path (priv->tree_view, NULL);

		  if (ctk_tree_selection_real_select_node (selection, tree, node, TRUE))
		    _ctk_tree_view_set_anchor_path (priv->tree_view, path);
		}
	    }
	  else
	    {
	      if (ctk_tree_selection_real_select_node (selection, tree, node, TRUE))
		{
		  dirty = TRUE;

		  _ctk_tree_view_set_anchor_path (priv->tree_view, path);
		}
	    }
	}
    }
  else if (priv->type == CTK_SELECTION_MULTIPLE)
    {
      if ((mode & CTK_TREE_SELECT_MODE_EXTEND) == CTK_TREE_SELECT_MODE_EXTEND
          && (anchor_path == NULL))
	{
	  _ctk_tree_view_set_anchor_path (priv->tree_view, path);

	  dirty = ctk_tree_selection_real_select_node (selection, tree, node, TRUE);
	}
      else if ((mode & (CTK_TREE_SELECT_MODE_EXTEND | CTK_TREE_SELECT_MODE_TOGGLE)) == (CTK_TREE_SELECT_MODE_EXTEND | CTK_TREE_SELECT_MODE_TOGGLE))
	{
	  ctk_tree_selection_select_range (selection,
					   anchor_path,
					   path);
	}
      else if ((mode & CTK_TREE_SELECT_MODE_TOGGLE) == CTK_TREE_SELECT_MODE_TOGGLE)
	{
	  flags = node->flags;

	  _ctk_tree_view_set_anchor_path (priv->tree_view, path);

	  if ((flags & CTK_RBNODE_IS_SELECTED) == CTK_RBNODE_IS_SELECTED)
	    dirty |= ctk_tree_selection_real_select_node (selection, tree, node, FALSE);
	  else
	    dirty |= ctk_tree_selection_real_select_node (selection, tree, node, TRUE);
	}
      else if ((mode & CTK_TREE_SELECT_MODE_EXTEND) == CTK_TREE_SELECT_MODE_EXTEND)
	{
	  dirty = ctk_tree_selection_real_unselect_all (selection);
	  dirty |= ctk_tree_selection_real_modify_range (selection,
                                                         RANGE_SELECT,
							 anchor_path,
							 path);
	}
      else
	{
	  dirty = ctk_tree_selection_real_unselect_all (selection);

	  _ctk_tree_view_set_anchor_path (priv->tree_view, path);

	  dirty |= ctk_tree_selection_real_select_node (selection, tree, node, TRUE);
	}
    }

  if (anchor_path)
    ctk_tree_path_free (anchor_path);

  if (dirty)
    g_signal_emit (selection, tree_selection_signals[CHANGED], 0);  
}


void 
_ctk_tree_selection_emit_changed (GtkTreeSelection *selection)
{
  g_signal_emit (selection, tree_selection_signals[CHANGED], 0);  
}

/* NOTE: Any {un,}selection ever done _MUST_ be done through this function!
 */

static gint
ctk_tree_selection_real_select_node (GtkTreeSelection *selection,
				     GtkRBTree        *tree,
				     GtkRBNode        *node,
				     gboolean          select)
{
  GtkTreeSelectionPrivate *priv = selection->priv;
  gboolean toggle = FALSE;
  GtkTreePath *path = NULL;

  g_return_val_if_fail (node != NULL, FALSE);

  select = !! select;

  if (CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_IS_SELECTED) != select)
    {
      path = _ctk_tree_path_new_from_rbtree (tree, node);
      toggle = _ctk_tree_selection_row_is_selectable (selection, node, path);
      ctk_tree_path_free (path);
    }

  if (toggle)
    {
      if (!CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_IS_SELECTED))
        {
          CTK_RBNODE_SET_FLAG (node, CTK_RBNODE_IS_SELECTED);
          _ctk_tree_view_accessible_add_state (priv->tree_view, tree, node, CTK_CELL_RENDERER_SELECTED);
        }
      else
        {
          CTK_RBNODE_UNSET_FLAG (node, CTK_RBNODE_IS_SELECTED);
          _ctk_tree_view_accessible_remove_state (priv->tree_view, tree, node, CTK_CELL_RENDERER_SELECTED);
        }

      _ctk_tree_view_queue_draw_node (priv->tree_view, tree, node, NULL);

      return TRUE;
    }

  return FALSE;
}
