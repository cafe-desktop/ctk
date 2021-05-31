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
 * @Short_description: The selection object for CtkTreeView
 * @Title: CtkTreeSelection
 * @See_also: #CtkTreeView, #CtkTreeViewColumn, #CtkTreeModel,
 *   #CtkTreeSortable, #CtkTreeModelSort, #CtkListStore, #CtkTreeStore,
 *   #CtkCellRenderer, #CtkCellEditable, #CtkCellRendererPixbuf,
 *   #CtkCellRendererText, #CtkCellRendererToggle, [CtkTreeView drag-and-drop][ctk3-CtkTreeView-drag-and-drop]
 *
 * The #CtkTreeSelection object is a helper object to manage the selection
 * for a #CtkTreeView widget.  The #CtkTreeSelection object is
 * automatically created when a new #CtkTreeView widget is created, and
 * cannot exist independently of this widget.  The primary reason the
 * #CtkTreeSelection objects exists is for cleanliness of code and API.
 * That is, there is no conceptual reason all these functions could not be
 * methods on the #CtkTreeView widget instead of a separate function.
 *
 * The #CtkTreeSelection object is gotten from a #CtkTreeView by calling
 * ctk_tree_view_get_selection().  It can be manipulated to check the
 * selection status of the tree, as well as select and deselect individual
 * rows.  Selection is done completely view side.  As a result, multiple
 * views of the same model can have completely different selections.
 * Additionally, you cannot change the selection of a row on the model that
 * is not currently displayed by the view without expanding its parents
 * first.
 *
 * One of the important things to remember when monitoring the selection of
 * a view is that the #CtkTreeSelection::changed signal is mostly a hint.
 * That is, it may only emit one signal when a range of rows is selected.
 * Additionally, it may on occasion emit a #CtkTreeSelection::changed signal
 * when nothing has happened (mostly as a result of programmers calling
 * select_row on an already selected row).
 */

struct _CtkTreeSelectionPrivate
{
  CtkTreeView *tree_view;
  CtkSelectionMode type;
  CtkTreeSelectionFunc user_func;
  gpointer user_data;
  GDestroyNotify destroy;
};

static void ctk_tree_selection_finalize          (GObject               *object);
static gint ctk_tree_selection_real_select_all   (CtkTreeSelection      *selection);
static gint ctk_tree_selection_real_unselect_all (CtkTreeSelection      *selection);
static gint ctk_tree_selection_real_select_node  (CtkTreeSelection      *selection,
						  CtkRBTree             *tree,
						  CtkRBNode             *node,
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

G_DEFINE_TYPE_WITH_PRIVATE (CtkTreeSelection, ctk_tree_selection, G_TYPE_OBJECT)

static void
ctk_tree_selection_class_init (CtkTreeSelectionClass *class)
{
  GObjectClass *object_class;

  object_class = (GObjectClass*) class;

  object_class->finalize = ctk_tree_selection_finalize;
  object_class->set_property = ctk_tree_selection_set_property;
  object_class->get_property = ctk_tree_selection_get_property;
  class->changed = NULL;

  /* Properties */
  
  /**
   * CtkTreeSelection:mode:
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
   * CtkTreeSelection::changed:
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
		  G_STRUCT_OFFSET (CtkTreeSelectionClass, changed),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);
}

static void
ctk_tree_selection_init (CtkTreeSelection *selection)
{
  selection->priv = ctk_tree_selection_get_instance_private (selection);
  selection->priv->type = CTK_SELECTION_SINGLE;
}

static void
ctk_tree_selection_finalize (GObject *object)
{
  CtkTreeSelection *selection = CTK_TREE_SELECTION (object);
  CtkTreeSelectionPrivate *priv = selection->priv;

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
 * Creates a new #CtkTreeSelection object.  This function should not be invoked,
 * as each #CtkTreeView will create its own #CtkTreeSelection.
 *
 * Returns: A newly created #CtkTreeSelection object.
 **/
CtkTreeSelection*
_ctk_tree_selection_new (void)
{
  CtkTreeSelection *selection;

  selection = g_object_new (CTK_TYPE_TREE_SELECTION, NULL);

  return selection;
}

/**
 * _ctk_tree_selection_new_with_tree_view:
 * @tree_view: The #CtkTreeView.
 *
 * Creates a new #CtkTreeSelection object.  This function should not be invoked,
 * as each #CtkTreeView will create its own #CtkTreeSelection.
 *
 * Returns: A newly created #CtkTreeSelection object.
 **/
CtkTreeSelection*
_ctk_tree_selection_new_with_tree_view (CtkTreeView *tree_view)
{
  CtkTreeSelection *selection;

  g_return_val_if_fail (CTK_IS_TREE_VIEW (tree_view), NULL);

  selection = _ctk_tree_selection_new ();
  _ctk_tree_selection_set_tree_view (selection, tree_view);

  return selection;
}

/**
 * _ctk_tree_selection_set_tree_view:
 * @selection: A #CtkTreeSelection.
 * @tree_view: The #CtkTreeView.
 *
 * Sets the #CtkTreeView of @selection.  This function should not be invoked, as
 * it is used internally by #CtkTreeView.
 **/
void
_ctk_tree_selection_set_tree_view (CtkTreeSelection *selection,
                                   CtkTreeView      *tree_view)
{
  CtkTreeSelectionPrivate *priv;

  g_return_if_fail (CTK_IS_TREE_SELECTION (selection));
  if (tree_view != NULL)
    g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));

  priv = selection->priv;

  priv->tree_view = tree_view;
}

/**
 * ctk_tree_selection_set_mode:
 * @selection: A #CtkTreeSelection.
 * @type: The selection mode
 *
 * Sets the selection mode of the @selection.  If the previous type was
 * #CTK_SELECTION_MULTIPLE, then the anchor is kept selected, if it was
 * previously selected.
 **/
void
ctk_tree_selection_set_mode (CtkTreeSelection *selection,
			     CtkSelectionMode  type)
{
  CtkTreeSelectionPrivate *priv;
  CtkTreeSelectionFunc tmp_func;

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
      CtkRBTree *tree = NULL;
      CtkRBNode *node = NULL;
      gint selected = FALSE;
      CtkTreePath *anchor_path = NULL;

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
 * @selection: a #CtkTreeSelection
 *
 * Gets the selection mode for @selection. See
 * ctk_tree_selection_set_mode().
 *
 * Returns: the current selection mode
 **/
CtkSelectionMode
ctk_tree_selection_get_mode (CtkTreeSelection *selection)
{
  g_return_val_if_fail (CTK_IS_TREE_SELECTION (selection), CTK_SELECTION_SINGLE);

  return selection->priv->type;
}

/**
 * ctk_tree_selection_set_select_function:
 * @selection: A #CtkTreeSelection.
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
ctk_tree_selection_set_select_function (CtkTreeSelection     *selection,
					CtkTreeSelectionFunc  func,
					gpointer              data,
					GDestroyNotify        destroy)
{
  CtkTreeSelectionPrivate *priv;

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
 * @selection: A #CtkTreeSelection.
 *
 * Returns the current selection function.
 *
 * Returns: The function.
 *
 * Since: 2.14
 **/
CtkTreeSelectionFunc
ctk_tree_selection_get_select_function (CtkTreeSelection *selection)
{
  g_return_val_if_fail (CTK_IS_TREE_SELECTION (selection), NULL);

  return selection->priv->user_func;
}

/**
 * ctk_tree_selection_get_user_data: (skip)
 * @selection: A #CtkTreeSelection.
 *
 * Returns the user data for the selection function.
 *
 * Returns: The user data.
 **/
gpointer
ctk_tree_selection_get_user_data (CtkTreeSelection *selection)
{
  g_return_val_if_fail (CTK_IS_TREE_SELECTION (selection), NULL);

  return selection->priv->user_data;
}

/**
 * ctk_tree_selection_get_tree_view:
 * @selection: A #CtkTreeSelection
 * 
 * Returns the tree view associated with @selection.
 * 
 * Returns: (transfer none): A #CtkTreeView
 **/
CtkTreeView *
ctk_tree_selection_get_tree_view (CtkTreeSelection *selection)
{
  g_return_val_if_fail (CTK_IS_TREE_SELECTION (selection), NULL);

  return selection->priv->tree_view;
}

/**
 * ctk_tree_selection_get_selected:
 * @selection: A #CtkTreeSelection.
 * @model: (out) (allow-none) (transfer none): A pointer to set to the #CtkTreeModel, or NULL.
 * @iter: (out) (allow-none): The #CtkTreeIter, or NULL.
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
ctk_tree_selection_get_selected (CtkTreeSelection  *selection,
				 CtkTreeModel     **model,
				 CtkTreeIter       *iter)
{
  CtkTreeSelectionPrivate *priv;
  CtkRBTree *tree;
  CtkRBNode *node;
  CtkTreePath *anchor_path;
  gboolean retval = FALSE;
  gboolean found_node;

  g_return_val_if_fail (CTK_IS_TREE_SELECTION (selection), FALSE);

  priv = selection->priv;

  g_return_val_if_fail (priv->type != CTK_SELECTION_MULTIPLE, FALSE);
  g_return_val_if_fail (priv->tree_view != NULL, FALSE);

  /* Clear the iter */
  if (iter)
    memset (iter, 0, sizeof (CtkTreeIter));

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
 * @selection: A #CtkTreeSelection.
 * @model: (out) (allow-none) (transfer none): A pointer to set to the #CtkTreeModel, or %NULL.
 *
 * Creates a list of path of all selected rows. Additionally, if you are
 * planning on modifying the model after calling this function, you may
 * want to convert the returned list into a list of #CtkTreeRowReferences.
 * To do this, you can use ctk_tree_row_reference_new().
 *
 * To free the return value, use:
 * |[<!-- language="C" -->
 * g_list_free_full (list, (GDestroyNotify) ctk_tree_path_free);
 * ]|
 *
 * Returns: (element-type CtkTreePath) (transfer full): A #GList containing a #CtkTreePath for each selected row.
 *
 * Since: 2.2
 **/
GList *
ctk_tree_selection_get_selected_rows (CtkTreeSelection   *selection,
                                      CtkTreeModel      **model)
{
  CtkTreeSelectionPrivate *priv;
  GList *list = NULL;
  CtkRBTree *tree = NULL;
  CtkRBNode *node = NULL;
  CtkTreePath *path;

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
      CtkTreeIter iter;

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
ctk_tree_selection_count_selected_rows_helper (CtkRBTree *tree,
					       CtkRBNode *node,
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
 * @selection: A #CtkTreeSelection.
 *
 * Returns the number of rows that have been selected in @tree.
 *
 * Returns: The number of rows selected.
 * 
 * Since: 2.2
 **/
gint
ctk_tree_selection_count_selected_rows (CtkTreeSelection *selection)
{
  CtkTreeSelectionPrivate *priv;
  gint count = 0;
  CtkRBTree *tree;

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
 * @selection: A #CtkTreeSelection.
 * @func: (scope call): The function to call for each selected node.
 * @data: user data to pass to the function.
 *
 * Calls a function for each selected node. Note that you cannot modify
 * the tree or selection from within this function. As a result,
 * ctk_tree_selection_get_selected_rows() might be more useful.
 **/
void
ctk_tree_selection_selected_foreach (CtkTreeSelection            *selection,
				     CtkTreeSelectionForeachFunc  func,
				     gpointer                     data)
{
  CtkTreeSelectionPrivate *priv;
  CtkTreePath *path;
  CtkRBTree *tree;
  CtkRBNode *node;
  CtkTreeIter iter;
  CtkTreeModel *model;

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
 * @selection: A #CtkTreeSelection.
 * @path: The #CtkTreePath to be selected.
 *
 * Select the row at @path.
 **/
void
ctk_tree_selection_select_path (CtkTreeSelection *selection,
				CtkTreePath      *path)
{
  CtkTreeSelectionPrivate *priv;
  CtkRBNode *node;
  CtkRBTree *tree;
  gboolean ret;
  CtkTreeSelectMode mode = 0;

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
 * @selection: A #CtkTreeSelection.
 * @path: The #CtkTreePath to be unselected.
 *
 * Unselects the row at @path.
 **/
void
ctk_tree_selection_unselect_path (CtkTreeSelection *selection,
				  CtkTreePath      *path)
{
  CtkTreeSelectionPrivate *priv;
  CtkRBNode *node;
  CtkRBTree *tree;
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
 * @selection: A #CtkTreeSelection.
 * @iter: The #CtkTreeIter to be selected.
 *
 * Selects the specified iterator.
 **/
void
ctk_tree_selection_select_iter (CtkTreeSelection *selection,
				CtkTreeIter      *iter)
{
  CtkTreeSelectionPrivate *priv;
  CtkTreePath *path;
  CtkTreeModel *model;

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
 * @selection: A #CtkTreeSelection.
 * @iter: The #CtkTreeIter to be unselected.
 *
 * Unselects the specified iterator.
 **/
void
ctk_tree_selection_unselect_iter (CtkTreeSelection *selection,
				  CtkTreeIter      *iter)
{
  CtkTreeSelectionPrivate *priv;
  CtkTreePath *path;
  CtkTreeModel *model;

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
 * @selection: A #CtkTreeSelection.
 * @path: A #CtkTreePath to check selection on.
 * 
 * Returns %TRUE if the row pointed to by @path is currently selected.  If @path
 * does not point to a valid location, %FALSE is returned
 * 
 * Returns: %TRUE if @path is selected.
 **/
gboolean
ctk_tree_selection_path_is_selected (CtkTreeSelection *selection,
				     CtkTreePath      *path)
{
  CtkTreeSelectionPrivate *priv;
  CtkRBNode *node;
  CtkRBTree *tree;
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
 * @selection: A #CtkTreeSelection
 * @iter: A valid #CtkTreeIter
 * 
 * Returns %TRUE if the row at @iter is currently selected.
 * 
 * Returns: %TRUE, if @iter is selected
 **/
gboolean
ctk_tree_selection_iter_is_selected (CtkTreeSelection *selection,
				     CtkTreeIter      *iter)
{
  CtkTreeSelectionPrivate *priv;
  CtkTreePath *path;
  CtkTreeModel *model;
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
  CtkTreeSelection *selection;
  gint dirty;
};

static void
select_all_helper (CtkRBTree  *tree,
		   CtkRBNode  *node,
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
ctk_tree_selection_real_select_all (CtkTreeSelection *selection)
{
  CtkTreeSelectionPrivate *priv = selection->priv;
  struct _TempTuple *tuple;
  CtkRBTree *tree;

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
 * @selection: A #CtkTreeSelection.
 *
 * Selects all the nodes. @selection must be set to #CTK_SELECTION_MULTIPLE
 * mode.
 **/
void
ctk_tree_selection_select_all (CtkTreeSelection *selection)
{
  CtkTreeSelectionPrivate *priv;

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
unselect_all_helper (CtkRBTree  *tree,
		     CtkRBNode  *node,
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
ctk_tree_selection_real_unselect_all (CtkTreeSelection *selection)
{
  CtkTreeSelectionPrivate *priv = selection->priv;
  struct _TempTuple *tuple;

  if (priv->type == CTK_SELECTION_SINGLE ||
      priv->type == CTK_SELECTION_BROWSE)
    {
      CtkRBTree *tree = NULL;
      CtkRBNode *node = NULL;
      CtkTreePath *anchor_path;

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
      CtkRBTree *tree;

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
 * @selection: A #CtkTreeSelection.
 *
 * Unselects all the nodes.
 **/
void
ctk_tree_selection_unselect_all (CtkTreeSelection *selection)
{
  CtkTreeSelectionPrivate *priv;

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
ctk_tree_selection_real_modify_range (CtkTreeSelection *selection,
                                      gint              mode,
				      CtkTreePath      *start_path,
				      CtkTreePath      *end_path)
{
  CtkTreeSelectionPrivate *priv = selection->priv;
  CtkRBNode *start_node = NULL, *end_node = NULL;
  CtkRBTree *start_tree, *end_tree;
  CtkTreePath *anchor_path = NULL;
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
 * @selection: A #CtkTreeSelection.
 * @start_path: The initial node of the range.
 * @end_path: The final node of the range.
 *
 * Selects a range of nodes, determined by @start_path and @end_path inclusive.
 * @selection must be set to #CTK_SELECTION_MULTIPLE mode. 
 **/
void
ctk_tree_selection_select_range (CtkTreeSelection *selection,
				 CtkTreePath      *start_path,
				 CtkTreePath      *end_path)
{
  CtkTreeSelectionPrivate *priv;

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
 * @selection: A #CtkTreeSelection.
 * @start_path: The initial node of the range.
 * @end_path: The initial node of the range.
 *
 * Unselects a range of nodes, determined by @start_path and @end_path
 * inclusive.
 *
 * Since: 2.2
 **/
void
ctk_tree_selection_unselect_range (CtkTreeSelection *selection,
                                   CtkTreePath      *start_path,
				   CtkTreePath      *end_path)
{
  CtkTreeSelectionPrivate *priv;

  g_return_if_fail (CTK_IS_TREE_SELECTION (selection));

  priv = selection->priv;

  g_return_if_fail (priv->tree_view != NULL);
  g_return_if_fail (ctk_tree_view_get_model (priv->tree_view) != NULL);

  if (ctk_tree_selection_real_modify_range (selection, RANGE_UNSELECT, start_path, end_path))
    g_signal_emit (selection, tree_selection_signals[CHANGED], 0);
}

gboolean
_ctk_tree_selection_row_is_selectable (CtkTreeSelection *selection,
				       CtkRBNode        *node,
				       CtkTreePath      *path)
{
  CtkTreeSelectionPrivate *priv = selection->priv;
  CtkTreeIter iter;
  CtkTreeModel *model;
  CtkTreeViewRowSeparatorFunc separator_func;
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
_ctk_tree_selection_internal_select_node (CtkTreeSelection *selection,
					  CtkRBNode        *node,
					  CtkRBTree        *tree,
					  CtkTreePath      *path,
                                          CtkTreeSelectMode mode,
					  gboolean          override_browse_mode)
{
  CtkTreeSelectionPrivate *priv = selection->priv;
  gint flags;
  gint dirty = FALSE;
  CtkTreePath *anchor_path = NULL;

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
_ctk_tree_selection_emit_changed (CtkTreeSelection *selection)
{
  g_signal_emit (selection, tree_selection_signals[CHANGED], 0);  
}

/* NOTE: Any {un,}selection ever done _MUST_ be done through this function!
 */

static gint
ctk_tree_selection_real_select_node (CtkTreeSelection *selection,
				     CtkRBTree        *tree,
				     CtkRBNode        *node,
				     gboolean          select)
{
  CtkTreeSelectionPrivate *priv = selection->priv;
  gboolean toggle = FALSE;
  CtkTreePath *path = NULL;

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
