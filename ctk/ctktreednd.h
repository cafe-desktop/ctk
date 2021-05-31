/* ctktreednd.h
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

#ifndef __CTK_TREE_DND_H__
#define __CTK_TREE_DND_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctktreemodel.h>
#include <ctk/ctkdnd.h>

G_BEGIN_DECLS

#define CTK_TYPE_TREE_DRAG_SOURCE            (ctk_tree_drag_source_get_type ())
#define CTK_TREE_DRAG_SOURCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_TREE_DRAG_SOURCE, CtkTreeDragSource))
#define CTK_IS_TREE_DRAG_SOURCE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_TREE_DRAG_SOURCE))
#define CTK_TREE_DRAG_SOURCE_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), CTK_TYPE_TREE_DRAG_SOURCE, CtkTreeDragSourceIface))

typedef struct _CtkTreeDragSource      CtkTreeDragSource; /* Dummy typedef */
typedef struct _CtkTreeDragSourceIface CtkTreeDragSourceIface;

/**
 * CtkTreeDragSourceIface:
 * @row_draggable: Asks the #CtkTreeDragSource whether a particular
 *    row can be used as the source of a DND operation.
 * @drag_data_get: Asks the #CtkTreeDragSource to fill in
 *    selection_data with a representation of the row at path.
 * @drag_data_delete: Asks the #CtkTreeDragSource to delete the row at
 *    path, because it was moved somewhere else via drag-and-drop.
 */
struct _CtkTreeDragSourceIface
{
  /*< private >*/
  GTypeInterface g_iface;

  /*< public >*/

  /* VTable - not signals */

  gboolean     (* row_draggable)        (CtkTreeDragSource   *drag_source,
                                         CtkTreePath         *path);

  gboolean     (* drag_data_get)        (CtkTreeDragSource   *drag_source,
                                         CtkTreePath         *path,
                                         CtkSelectionData    *selection_data);

  gboolean     (* drag_data_delete)     (CtkTreeDragSource *drag_source,
                                         CtkTreePath       *path);
};

GDK_AVAILABLE_IN_ALL
GType           ctk_tree_drag_source_get_type   (void) G_GNUC_CONST;

/* Returns whether the given row can be dragged */
GDK_AVAILABLE_IN_ALL
gboolean ctk_tree_drag_source_row_draggable    (CtkTreeDragSource *drag_source,
                                                CtkTreePath       *path);

/* Deletes the given row, or returns FALSE if it can't */
GDK_AVAILABLE_IN_ALL
gboolean ctk_tree_drag_source_drag_data_delete (CtkTreeDragSource *drag_source,
                                                CtkTreePath       *path);

/* Fills in selection_data with type selection_data->target based on
 * the row denoted by path, returns TRUE if it does anything
 */
GDK_AVAILABLE_IN_ALL
gboolean ctk_tree_drag_source_drag_data_get    (CtkTreeDragSource *drag_source,
                                                CtkTreePath       *path,
                                                CtkSelectionData  *selection_data);

#define CTK_TYPE_TREE_DRAG_DEST            (ctk_tree_drag_dest_get_type ())
#define CTK_TREE_DRAG_DEST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_TREE_DRAG_DEST, CtkTreeDragDest))
#define CTK_IS_TREE_DRAG_DEST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_TREE_DRAG_DEST))
#define CTK_TREE_DRAG_DEST_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), CTK_TYPE_TREE_DRAG_DEST, CtkTreeDragDestIface))

typedef struct _CtkTreeDragDest      CtkTreeDragDest; /* Dummy typedef */
typedef struct _CtkTreeDragDestIface CtkTreeDragDestIface;

/**
 * CtkTreeDragDestIface:
 * @drag_data_received: Asks the #CtkTreeDragDest to insert a row
 *    before the path dest, deriving the contents of the row from
 *    selection_data.
 * @row_drop_possible: Determines whether a drop is possible before
 *    the given dest_path, at the same depth as dest_path.
 */
struct _CtkTreeDragDestIface
{
  /*< private >*/
  GTypeInterface g_iface;

  /*< public >*/

  /* VTable - not signals */

  gboolean     (* drag_data_received) (CtkTreeDragDest   *drag_dest,
                                       CtkTreePath       *dest,
                                       CtkSelectionData  *selection_data);

  gboolean     (* row_drop_possible)  (CtkTreeDragDest   *drag_dest,
                                       CtkTreePath       *dest_path,
				       CtkSelectionData  *selection_data);
};

GDK_AVAILABLE_IN_ALL
GType           ctk_tree_drag_dest_get_type   (void) G_GNUC_CONST;

/* Inserts a row before dest which contains data in selection_data,
 * or returns FALSE if it can't
 */
GDK_AVAILABLE_IN_ALL
gboolean ctk_tree_drag_dest_drag_data_received (CtkTreeDragDest   *drag_dest,
						CtkTreePath       *dest,
						CtkSelectionData  *selection_data);


/* Returns TRUE if we can drop before path; path may not exist. */
GDK_AVAILABLE_IN_ALL
gboolean ctk_tree_drag_dest_row_drop_possible  (CtkTreeDragDest   *drag_dest,
						CtkTreePath       *dest_path,
						CtkSelectionData  *selection_data);


/* The selection data would normally have target type CTK_TREE_MODEL_ROW in this
 * case. If the target is wrong these functions return FALSE.
 */
GDK_AVAILABLE_IN_ALL
gboolean ctk_tree_set_row_drag_data            (CtkSelectionData  *selection_data,
						CtkTreeModel      *tree_model,
						CtkTreePath       *path);
GDK_AVAILABLE_IN_ALL
gboolean ctk_tree_get_row_drag_data            (CtkSelectionData  *selection_data,
						CtkTreeModel     **tree_model,
						CtkTreePath      **path);

G_END_DECLS

#endif /* __CTK_TREE_DND_H__ */
