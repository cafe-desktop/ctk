/* ctktreednd.c
 * Copyright (C) 2001  Red Hat, Inc.
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
#include "ctktreednd.h"
#include "ctkintl.h"


/**
 * SECTION:ctktreednd
 * @Short_description: Interfaces for drag-and-drop support in CtkTreeView
 * @Title: CtkTreeView drag-and-drop
 *
 * CTK+ supports Drag-and-Drop in tree views with a high-level and a low-level
 * API.
 *
 * The low-level API consists of the CTK+ DND API, augmented by some treeview
 * utility functions: ctk_tree_view_set_drag_dest_row(),
 * ctk_tree_view_get_drag_dest_row(), ctk_tree_view_get_dest_row_at_pos(),
 * ctk_tree_view_create_row_drag_icon(), ctk_tree_set_row_drag_data() and
 * ctk_tree_get_row_drag_data(). This API leaves a lot of flexibility, but
 * nothing is done automatically, and implementing advanced features like
 * hover-to-open-rows or autoscrolling on top of this API is a lot of work.
 *
 * On the other hand, if you write to the high-level API, then all the
 * bookkeeping of rows is done for you, as well as things like hover-to-open
 * and auto-scroll, but your models have to implement the
 * #CtkTreeDragSource and #CtkTreeDragDest interfaces.
 */


GType
ctk_tree_drag_source_get_type (void)
{
  static GType our_type = 0;

  if (!our_type)
    {
      const GTypeInfo our_info =
      {
        .class_size = sizeof (CtkTreeDragSourceIface),
        .instance_size = 0,
        .n_preallocs = 0
      };

      our_type = g_type_register_static (G_TYPE_INTERFACE, 
					 I_("CtkTreeDragSource"),
					 &our_info, 0);
    }
  
  return our_type;
}


GType
ctk_tree_drag_dest_get_type (void)
{
  static GType our_type = 0;

  if (!our_type)
    {
      const GTypeInfo our_info =
      {
        .class_size = sizeof (CtkTreeDragDestIface),
        .instance_size = 0,
        .n_preallocs = 0
      };

      our_type = g_type_register_static (G_TYPE_INTERFACE, I_("CtkTreeDragDest"), &our_info, 0);
    }
  
  return our_type;
}

/**
 * ctk_tree_drag_source_row_draggable:
 * @drag_source: a #CtkTreeDragSource
 * @path: row on which user is initiating a drag
 * 
 * Asks the #CtkTreeDragSource whether a particular row can be used as
 * the source of a DND operation. If the source doesn’t implement
 * this interface, the row is assumed draggable.
 *
 * Returns: %TRUE if the row can be dragged
 **/
gboolean
ctk_tree_drag_source_row_draggable (CtkTreeDragSource *drag_source,
                                    CtkTreePath       *path)
{
  CtkTreeDragSourceIface *iface = CTK_TREE_DRAG_SOURCE_GET_IFACE (drag_source);

  g_return_val_if_fail (path != NULL, FALSE);

  if (iface->row_draggable)
    return (* iface->row_draggable) (drag_source, path);
  else
    return TRUE;
    /* Returning TRUE if row_draggable is not implemented is a fallback.
       Interface implementations such as CtkTreeStore and CtkListStore really should
       implement row_draggable. */
}


/**
 * ctk_tree_drag_source_drag_data_delete:
 * @drag_source: a #CtkTreeDragSource
 * @path: row that was being dragged
 * 
 * Asks the #CtkTreeDragSource to delete the row at @path, because
 * it was moved somewhere else via drag-and-drop. Returns %FALSE
 * if the deletion fails because @path no longer exists, or for
 * some model-specific reason. Should robustly handle a @path no
 * longer found in the model!
 * 
 * Returns: %TRUE if the row was successfully deleted
 **/
gboolean
ctk_tree_drag_source_drag_data_delete (CtkTreeDragSource *drag_source,
                                       CtkTreePath       *path)
{
  CtkTreeDragSourceIface *iface = CTK_TREE_DRAG_SOURCE_GET_IFACE (drag_source);

  g_return_val_if_fail (iface->drag_data_delete != NULL, FALSE);
  g_return_val_if_fail (path != NULL, FALSE);

  return (* iface->drag_data_delete) (drag_source, path);
}

/**
 * ctk_tree_drag_source_drag_data_get:
 * @drag_source: a #CtkTreeDragSource
 * @path: row that was dragged
 * @selection_data: a #CtkSelectionData to fill with data
 *                  from the dragged row
 * 
 * Asks the #CtkTreeDragSource to fill in @selection_data with a
 * representation of the row at @path. @selection_data->target gives
 * the required type of the data.  Should robustly handle a @path no
 * longer found in the model!
 * 
 * Returns: %TRUE if data of the required type was provided 
 **/
gboolean
ctk_tree_drag_source_drag_data_get    (CtkTreeDragSource *drag_source,
                                       CtkTreePath       *path,
                                       CtkSelectionData  *selection_data)
{
  CtkTreeDragSourceIface *iface = CTK_TREE_DRAG_SOURCE_GET_IFACE (drag_source);

  g_return_val_if_fail (iface->drag_data_get != NULL, FALSE);
  g_return_val_if_fail (path != NULL, FALSE);
  g_return_val_if_fail (selection_data != NULL, FALSE);

  return (* iface->drag_data_get) (drag_source, path, selection_data);
}

/**
 * ctk_tree_drag_dest_drag_data_received:
 * @drag_dest: a #CtkTreeDragDest
 * @dest: row to drop in front of
 * @selection_data: data to drop
 * 
 * Asks the #CtkTreeDragDest to insert a row before the path @dest,
 * deriving the contents of the row from @selection_data. If @dest is
 * outside the tree so that inserting before it is impossible, %FALSE
 * will be returned. Also, %FALSE may be returned if the new row is
 * not created for some model-specific reason.  Should robustly handle
 * a @dest no longer found in the model!
 * 
 * Returns: whether a new row was created before position @dest
 **/
gboolean
ctk_tree_drag_dest_drag_data_received (CtkTreeDragDest  *drag_dest,
                                       CtkTreePath      *dest,
                                       CtkSelectionData *selection_data)
{
  CtkTreeDragDestIface *iface = CTK_TREE_DRAG_DEST_GET_IFACE (drag_dest);

  g_return_val_if_fail (iface->drag_data_received != NULL, FALSE);
  g_return_val_if_fail (dest != NULL, FALSE);
  g_return_val_if_fail (selection_data != NULL, FALSE);

  return (* iface->drag_data_received) (drag_dest, dest, selection_data);
}


/**
 * ctk_tree_drag_dest_row_drop_possible:
 * @drag_dest: a #CtkTreeDragDest
 * @dest_path: destination row
 * @selection_data: the data being dragged
 * 
 * Determines whether a drop is possible before the given @dest_path,
 * at the same depth as @dest_path. i.e., can we drop the data in
 * @selection_data at that location. @dest_path does not have to
 * exist; the return value will almost certainly be %FALSE if the
 * parent of @dest_path doesn’t exist, though.
 * 
 * Returns: %TRUE if a drop is possible before @dest_path
 **/
gboolean
ctk_tree_drag_dest_row_drop_possible (CtkTreeDragDest   *drag_dest,
                                      CtkTreePath       *dest_path,
				      CtkSelectionData  *selection_data)
{
  CtkTreeDragDestIface *iface = CTK_TREE_DRAG_DEST_GET_IFACE (drag_dest);

  g_return_val_if_fail (iface->row_drop_possible != NULL, FALSE);
  g_return_val_if_fail (selection_data != NULL, FALSE);
  g_return_val_if_fail (dest_path != NULL, FALSE);

  return (* iface->row_drop_possible) (drag_dest, dest_path, selection_data);
}

typedef struct _TreeRowData TreeRowData;

struct _TreeRowData
{
  CtkTreeModel *model;
  gchar path[4];
};

/**
 * ctk_tree_set_row_drag_data:
 * @selection_data: some #CtkSelectionData
 * @tree_model: a #CtkTreeModel
 * @path: a row in @tree_model
 * 
 * Sets selection data of target type %CTK_TREE_MODEL_ROW. Normally used
 * in a drag_data_get handler.
 * 
 * Returns: %TRUE if the #CtkSelectionData had the proper target type to allow us to set a tree row
 **/
gboolean
ctk_tree_set_row_drag_data (CtkSelectionData *selection_data,
			    CtkTreeModel     *tree_model,
			    CtkTreePath      *path)
{
  TreeRowData *trd;
  gchar *path_str;
  gint len;
  gint struct_size;
  
  g_return_val_if_fail (selection_data != NULL, FALSE);
  g_return_val_if_fail (CTK_IS_TREE_MODEL (tree_model), FALSE);
  g_return_val_if_fail (path != NULL, FALSE);

  if (ctk_selection_data_get_target (selection_data) != cdk_atom_intern_static_string ("CTK_TREE_MODEL_ROW"))
    return FALSE;
  
  path_str = ctk_tree_path_to_string (path);

  len = strlen (path_str);

  /* the old allocate-end-of-struct-to-hold-string trick */
  struct_size = sizeof (TreeRowData) + len + 1 -
    (sizeof (TreeRowData) - G_STRUCT_OFFSET (TreeRowData, path));

  trd = g_malloc (struct_size); 

  strcpy (trd->path, path_str);

  g_free (path_str);
  
  trd->model = tree_model;
  
  ctk_selection_data_set (selection_data,
                          cdk_atom_intern_static_string ("CTK_TREE_MODEL_ROW"),
                          8, /* bytes */
                          (void*)trd,
                          struct_size);

  g_free (trd);
  
  return TRUE;
}

/**
 * ctk_tree_get_row_drag_data:
 * @selection_data: a #CtkSelectionData
 * @tree_model: (nullable) (optional) (transfer none) (out): a #CtkTreeModel
 * @path: (nullable) (optional) (out): row in @tree_model
 * 
 * Obtains a @tree_model and @path from selection data of target type
 * %CTK_TREE_MODEL_ROW. Normally called from a drag_data_received handler.
 * This function can only be used if @selection_data originates from the same
 * process that’s calling this function, because a pointer to the tree model
 * is being passed around. If you aren’t in the same process, then you'll
 * get memory corruption. In the #CtkTreeDragDest drag_data_received handler,
 * you can assume that selection data of type %CTK_TREE_MODEL_ROW is
 * in from the current process. The returned path must be freed with
 * ctk_tree_path_free().
 * 
 * Returns: %TRUE if @selection_data had target type %CTK_TREE_MODEL_ROW and
 *  is otherwise valid
 **/
gboolean
ctk_tree_get_row_drag_data (CtkSelectionData  *selection_data,
			    CtkTreeModel     **tree_model,
			    CtkTreePath      **path)
{
  TreeRowData *trd;
  
  g_return_val_if_fail (selection_data != NULL, FALSE);  

  if (tree_model)
    *tree_model = NULL;

  if (path)
    *path = NULL;

  if (ctk_selection_data_get_target (selection_data) != cdk_atom_intern_static_string ("CTK_TREE_MODEL_ROW"))
    return FALSE;

  if (ctk_selection_data_get_length (selection_data) < 0)
    return FALSE;

  trd = (void*) ctk_selection_data_get_data (selection_data);

  if (tree_model)
    *tree_model = trd->model;

  if (path)
    *path = ctk_tree_path_new_from_string (trd->path);
  
  return TRUE;
}
