/* CTK+ - accessibility implementations
 * Copyright 2001, 2002, 2003 Sun Microsystems Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <ctk/ctk.h>
#ifdef CDK_WINDOWING_X11
#include <cdk/x11/cdkx.h>
#endif

#include "ctktreeprivate.h"
#include "ctkwidgetprivate.h"

#include "ctktreeviewaccessibleprivate.h"

#include "ctkrenderercellaccessible.h"
#include "ctkbooleancellaccessible.h"
#include "ctkimagecellaccessible.h"
#include "ctkcontainercellaccessible.h"
#include "ctktextcellaccessible.h"
#include "ctkcellaccessibleparent.h"
#include "ctkcellaccessibleprivate.h"

struct _CtkTreeViewAccessiblePrivate
{
  GHashTable *cell_infos;
};

typedef struct _CtkTreeViewAccessibleCellInfo  CtkTreeViewAccessibleCellInfo;
struct _CtkTreeViewAccessibleCellInfo
{
  CtkCellAccessible *cell;
  CtkRBTree *tree;
  CtkRBNode *node;
  CtkTreeViewColumn *cell_col_ref;
  CtkTreeViewAccessible *view;
};

/* Misc */

static int              cell_info_get_index             (CtkTreeView                     *tree_view,
                                                         CtkTreeViewAccessibleCellInfo   *info);
static gboolean         is_cell_showing                 (CtkTreeView            *tree_view,
                                                         CdkRectangle           *cell_rect);

static void             cell_info_new                   (CtkTreeViewAccessible  *accessible,
                                                         CtkRBTree              *tree,
                                                         CtkRBNode              *node,
                                                         CtkTreeViewColumn      *tv_col,
                                                         CtkCellAccessible      *cell);
static gint             get_column_number               (CtkTreeView            *tree_view,
                                                         CtkTreeViewColumn      *column);

static gboolean         get_rbtree_column_from_index    (CtkTreeView            *tree_view,
                                                         gint                   index,
                                                         CtkRBTree              **tree,
                                                         CtkRBNode              **node,
                                                         CtkTreeViewColumn      **column);

static CtkTreeViewAccessibleCellInfo* find_cell_info    (CtkTreeViewAccessible           *view,
                                                         CtkCellAccessible               *cell);
static AtkObject *       get_header_from_column         (CtkTreeViewColumn      *tv_col);


static void atk_table_interface_init                  (AtkTableIface                *iface);
static void atk_selection_interface_init              (AtkSelectionIface            *iface);
static void atk_component_interface_init              (AtkComponentIface            *iface);
static void ctk_cell_accessible_parent_interface_init (CtkCellAccessibleParentIface *iface);

G_DEFINE_TYPE_WITH_CODE (CtkTreeViewAccessible, ctk_tree_view_accessible, CTK_TYPE_CONTAINER_ACCESSIBLE,
                         G_ADD_PRIVATE (CtkTreeViewAccessible)
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_TABLE, atk_table_interface_init)
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_SELECTION, atk_selection_interface_init)
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_COMPONENT, atk_component_interface_init)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_CELL_ACCESSIBLE_PARENT, ctk_cell_accessible_parent_interface_init))


static GQuark
ctk_tree_view_accessible_get_data_quark (void)
{
  static GQuark quark = 0;

  if (G_UNLIKELY (quark == 0))
    quark = g_quark_from_static_string ("ctk-tree-view-accessible-data");

  return quark;
}

static void
cell_info_free (CtkTreeViewAccessibleCellInfo *cell_info)
{
  ctk_accessible_set_widget (CTK_ACCESSIBLE (cell_info->cell), NULL);
  g_object_unref (cell_info->cell);

  g_free (cell_info);
}

static CtkTreePath *
cell_info_get_path (CtkTreeViewAccessibleCellInfo *cell_info)
{
  return _ctk_tree_path_new_from_rbtree (cell_info->tree, cell_info->node);
}

static guint
cell_info_hash (gconstpointer info)
{
  const CtkTreeViewAccessibleCellInfo *cell_info = info;
  guint node, col;

  node = GPOINTER_TO_UINT (cell_info->node);
  col = GPOINTER_TO_UINT (cell_info->cell_col_ref);

  return ((node << sizeof (guint) / 2) | (node >> sizeof (guint) / 2)) ^ col;
}

static gboolean
cell_info_equal (gconstpointer a, gconstpointer b)
{
  const CtkTreeViewAccessibleCellInfo *cell_info_a = a;
  const CtkTreeViewAccessibleCellInfo *cell_info_b = b;

  return cell_info_a->node == cell_info_b->node &&
         cell_info_a->cell_col_ref == cell_info_b->cell_col_ref;
}

static void
ctk_tree_view_accessible_initialize (AtkObject *obj,
                                     gpointer   data)
{
  CtkTreeViewAccessible *accessible;
  CtkTreeView *tree_view;
  CtkTreeModel *tree_model;
  CtkWidget *widget;

  ATK_OBJECT_CLASS (ctk_tree_view_accessible_parent_class)->initialize (obj, data);

  accessible = CTK_TREE_VIEW_ACCESSIBLE (obj);

  accessible->priv->cell_infos = g_hash_table_new_full (cell_info_hash,
      cell_info_equal, NULL, (GDestroyNotify) cell_info_free);

  widget = CTK_WIDGET (data);
  tree_view = CTK_TREE_VIEW (widget);
  tree_model = ctk_tree_view_get_model (tree_view);

  if (tree_model)
    {
      if (ctk_tree_model_get_flags (tree_model) & CTK_TREE_MODEL_LIST_ONLY)
        obj->role = ATK_ROLE_TABLE;
      else
        obj->role = ATK_ROLE_TREE_TABLE;
    }
}

static void
ctk_tree_view_accessible_finalize (GObject *object)
{
  CtkTreeViewAccessible *accessible = CTK_TREE_VIEW_ACCESSIBLE (object);

  if (accessible->priv->cell_infos)
    g_hash_table_destroy (accessible->priv->cell_infos);

  G_OBJECT_CLASS (ctk_tree_view_accessible_parent_class)->finalize (object);
}

static void
ctk_tree_view_accessible_notify_ctk (GObject    *obj,
                                     GParamSpec *pspec)
{
  CtkWidget *widget;
  CtkTreeView *tree_view;
  CtkTreeViewAccessible *accessible;

  widget = CTK_WIDGET (obj);
  accessible = CTK_TREE_VIEW_ACCESSIBLE (ctk_widget_get_accessible (widget));
  tree_view = CTK_TREE_VIEW (widget);

  if (g_strcmp0 (pspec->name, "model") == 0)
    {
      CtkTreeModel *tree_model;
      AtkRole role;

      tree_model = ctk_tree_view_get_model (tree_view);
      g_hash_table_remove_all (accessible->priv->cell_infos);

      if (tree_model)
        {
          if (ctk_tree_model_get_flags (tree_model) & CTK_TREE_MODEL_LIST_ONLY)
            role = ATK_ROLE_TABLE;
          else
            role = ATK_ROLE_TREE_TABLE;
        }
      else
        {
          role = ATK_ROLE_UNKNOWN;
        }
      atk_object_set_role (ATK_OBJECT (accessible), role);
      g_object_freeze_notify (G_OBJECT (accessible));
      g_signal_emit_by_name (accessible, "model-changed");
      g_signal_emit_by_name (accessible, "visible-data-changed");
      g_object_thaw_notify (G_OBJECT (accessible));
    }
  else
    CTK_WIDGET_ACCESSIBLE_CLASS (ctk_tree_view_accessible_parent_class)->notify_ctk (obj, pspec);
}

static void
ctk_tree_view_accessible_widget_unset (CtkAccessible *ctkaccessible)
{
  CtkTreeViewAccessible *accessible = CTK_TREE_VIEW_ACCESSIBLE (ctkaccessible);

  g_hash_table_remove_all (accessible->priv->cell_infos);

  CTK_ACCESSIBLE_CLASS (ctk_tree_view_accessible_parent_class)->widget_unset (ctkaccessible);
}

static gint
get_n_rows (CtkTreeView *tree_view)
{
  CtkRBTree *tree;

  tree = _ctk_tree_view_get_rbtree (tree_view);

  if (tree == NULL)
    return 0;

  return tree->root->total_count;
}

static gint
get_n_columns (CtkTreeView *tree_view)
{
  guint i, visible_columns;

  visible_columns = 0;

  for (i = 0; i < ctk_tree_view_get_n_columns (tree_view); i++)
    {
      CtkTreeViewColumn *column = ctk_tree_view_get_column (tree_view, i);

      if (ctk_tree_view_column_get_visible (column))
        visible_columns++;
    }

  return visible_columns;
}

static gint
ctk_tree_view_accessible_get_n_children (AtkObject *obj)
{
  CtkWidget *widget;
  CtkTreeView *tree_view;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return 0;

  tree_view = CTK_TREE_VIEW (widget);
  return (get_n_rows (tree_view) + 1) * get_n_columns (tree_view);
}

static CtkTreeViewColumn *
get_visible_column (CtkTreeView *tree_view,
                    guint        id)
{
  guint i;

  for (i = 0; i < ctk_tree_view_get_n_columns (tree_view); i++)
    {
      CtkTreeViewColumn *column = ctk_tree_view_get_column (tree_view, i);

      if (!ctk_tree_view_column_get_visible (column))
        continue;

      if (id == 0)
        return column;

      id--;
    }

  g_return_val_if_reached (NULL);
}

static void
set_cell_data (CtkTreeView           *treeview,
               CtkTreeViewAccessible *accessible,
               CtkCellAccessible     *cell)
{
  CtkTreeViewAccessibleCellInfo *cell_info;
  gboolean is_expander, is_expanded;
  CtkTreeModel *model;
  CtkTreeIter iter;
  CtkTreePath *path;

  cell_info = find_cell_info (accessible, cell);
  if (!cell_info)
    return;

  model = ctk_tree_view_get_model (treeview);

  if (CTK_RBNODE_FLAG_SET (cell_info->node, CTK_RBNODE_IS_PARENT) &&
      cell_info->cell_col_ref == ctk_tree_view_get_expander_column (treeview))
    {
      is_expander = TRUE;
      is_expanded = cell_info->node->children != NULL;
    }
  else
    {
      is_expander = FALSE;
      is_expanded = FALSE;
    }

  path = cell_info_get_path (cell_info);
  if (path == NULL ||
      !ctk_tree_model_get_iter (model, &iter, path))
    {
      /* We only track valid cells, this should never happen */
      g_return_if_reached ();
    }
  ctk_tree_path_free (path);

  ctk_tree_view_column_cell_set_cell_data (cell_info->cell_col_ref,
                                           model,
                                           &iter,
                                           is_expander,
                                           is_expanded);
}

static CtkCellAccessible *
peek_cell (CtkTreeViewAccessible *accessible,
           CtkRBTree             *tree,
           CtkRBNode             *node,
           CtkTreeViewColumn     *column)
{
  CtkTreeViewAccessibleCellInfo lookup, *cell_info;

  lookup.tree = tree;
  lookup.node = node;
  lookup.cell_col_ref = column;

  cell_info = g_hash_table_lookup (accessible->priv->cell_infos, &lookup);
  if (cell_info == NULL)
    return NULL;

  return cell_info->cell;
}

static CtkCellAccessible *
create_cell_accessible_for_renderer (CtkCellRenderer *renderer,
                                     CtkWidget       *widget,
                                     AtkObject       *parent)
{
  CtkCellAccessible *cell;

  cell = CTK_CELL_ACCESSIBLE (ctk_renderer_cell_accessible_new (renderer));
  
  _ctk_cell_accessible_initialize (cell, widget, parent);

  return cell;
}

static CtkCellAccessible *
create_cell_accessible (CtkTreeView           *treeview,
                        CtkTreeViewAccessible *accessible,
                        CtkTreeViewColumn     *column)
{
  GList *renderer_list;
  GList *l;
  CtkCellAccessible *cell;

  renderer_list = ctk_cell_layout_get_cells (CTK_CELL_LAYOUT (column));

  /* If there is exactly one renderer in the list (which is a 
   * common case), shortcut and don't make a container
   */
  if (g_list_length (renderer_list) == 1)
    {
      cell = create_cell_accessible_for_renderer (renderer_list->data, CTK_WIDGET (treeview), ATK_OBJECT (accessible));
    }
  else
    {
      CtkContainerCellAccessible *container;

      container = ctk_container_cell_accessible_new ();
      _ctk_cell_accessible_initialize (CTK_CELL_ACCESSIBLE (container), CTK_WIDGET (treeview), ATK_OBJECT (accessible));

      for (l = renderer_list; l; l = l->next)
        {
          cell = create_cell_accessible_for_renderer (l->data, CTK_WIDGET (treeview), ATK_OBJECT (container));
          ctk_container_cell_accessible_add_child (container, cell);
        }

      cell = CTK_CELL_ACCESSIBLE (container);
    }

  g_list_free (renderer_list);

  return cell;
}
                        
static CtkCellAccessible *
create_cell (CtkTreeView           *treeview,
             CtkTreeViewAccessible *accessible,
             CtkRBTree             *tree,
             CtkRBNode             *node,
             CtkTreeViewColumn     *column)
{
  CtkCellAccessible *cell;

  cell = create_cell_accessible (treeview, accessible, column);
  cell_info_new (accessible, tree, node, column, cell);

  set_cell_data (treeview, accessible, cell);
  _ctk_cell_accessible_update_cache (cell, FALSE);

  return cell;
}

static AtkObject *
ctk_tree_view_accessible_ref_child (AtkObject *obj,
                                    gint       i)
{
  CtkWidget *widget;
  CtkTreeViewAccessible *accessible;
  CtkCellAccessible *cell;
  CtkTreeView *tree_view;
  CtkTreeViewColumn *tv_col;
  CtkRBTree *tree;
  CtkRBNode *node;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget == NULL)
    return NULL;

  if (i >= ctk_tree_view_accessible_get_n_children (obj))
    return NULL;

  accessible = CTK_TREE_VIEW_ACCESSIBLE (obj);
  tree_view = CTK_TREE_VIEW (widget);
  if (i < get_n_columns (tree_view))
    {
      AtkObject *child;

      tv_col = get_visible_column (tree_view, i);
      child = get_header_from_column (tv_col);
      if (child)
        g_object_ref (child);
      return child;
    }

  /* Find the RBTree and CtkTreeViewColumn for the index */
  if (!get_rbtree_column_from_index (tree_view, i, &tree, &node, &tv_col))
    return NULL;

  cell = peek_cell (accessible, tree, node, tv_col);
  if (cell == NULL)
    cell = create_cell (tree_view, accessible, tree, node, tv_col);

  return ATK_OBJECT (g_object_ref (cell));
}

static AtkStateSet*
ctk_tree_view_accessible_ref_state_set (AtkObject *obj)
{
  AtkStateSet *state_set;
  CtkWidget *widget;

  state_set = ATK_OBJECT_CLASS (ctk_tree_view_accessible_parent_class)->ref_state_set (obj);
  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));

  if (widget != NULL)
    atk_state_set_add_state (state_set, ATK_STATE_MANAGES_DESCENDANTS);

  return state_set;
}

static void
ctk_tree_view_accessible_class_init (CtkTreeViewAccessibleClass *klass)
{
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  CtkAccessibleClass *accessible_class = (CtkAccessibleClass*)klass;
  CtkWidgetAccessibleClass *widget_class = (CtkWidgetAccessibleClass*)klass;
  CtkContainerAccessibleClass *container_class = (CtkContainerAccessibleClass*)klass;

  class->get_n_children = ctk_tree_view_accessible_get_n_children;
  class->ref_child = ctk_tree_view_accessible_ref_child;
  class->ref_state_set = ctk_tree_view_accessible_ref_state_set;
  class->initialize = ctk_tree_view_accessible_initialize;

  widget_class->notify_ctk = ctk_tree_view_accessible_notify_ctk;

  accessible_class->widget_unset = ctk_tree_view_accessible_widget_unset;

  /* The children of a CtkTreeView are the buttons at the top of the columns
   * we do not represent these as children so we do not want to report
   * children added or deleted when these changed.
   */
  container_class->add_ctk = NULL;
  container_class->remove_ctk = NULL;

  gobject_class->finalize = ctk_tree_view_accessible_finalize;
}

static void
ctk_tree_view_accessible_init (CtkTreeViewAccessible *view)
{
  view->priv = ctk_tree_view_accessible_get_instance_private (view);
}

/* atkcomponent.h */

static AtkObject *
ctk_tree_view_accessible_ref_accessible_at_point (AtkComponent *component,
                                                  gint          x,
                                                  gint          y,
                                                  AtkCoordType  coord_type)
{
  CtkWidget *widget;
  CtkTreeView *tree_view;
  CtkTreePath *path;
  CtkTreeViewColumn *column;
  gint x_pos, y_pos;
  gint bx, by;
  CtkCellAccessible *cell;
  CtkRBTree *tree;
  CtkRBNode *node;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (component));
  if (widget == NULL)
    return NULL;

  tree_view = CTK_TREE_VIEW (widget);

  atk_component_get_extents (component, &x_pos, &y_pos, NULL, NULL, coord_type);
  ctk_tree_view_convert_widget_to_bin_window_coords (tree_view, x, y, &bx, &by);
  if (!ctk_tree_view_get_path_at_pos (tree_view,
                                      bx - x_pos, by - y_pos,
                                      &path, &column, NULL, NULL))
    return NULL;

  if (_ctk_tree_view_find_node (tree_view, path, &tree, &node))
    {
      ctk_tree_path_free (path);
      return NULL;
    }

  cell = peek_cell (CTK_TREE_VIEW_ACCESSIBLE (component), tree, node, column);
  if (cell == NULL)
    cell = create_cell (tree_view, CTK_TREE_VIEW_ACCESSIBLE (component), tree, node, column);

  return ATK_OBJECT (g_object_ref (cell));
}

static void
atk_component_interface_init (AtkComponentIface *iface)
{
  iface->ref_accessible_at_point = ctk_tree_view_accessible_ref_accessible_at_point;
}

/* atktable.h */

static gint
ctk_tree_view_accessible_get_index_at (AtkTable *table,
                                       gint      row,
                                       gint      column)
{
  CtkWidget *widget;
  gint n_cols, n_rows;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (table));
  if (widget == NULL)
    return -1;

  n_cols = atk_table_get_n_columns (table);
  n_rows = atk_table_get_n_rows (table);

  if (row >= n_rows || column >= n_cols)
    return -1;

  return (row + 1) * n_cols + column;
}

static gint
ctk_tree_view_accessible_get_column_at_index (AtkTable *table,
                                              gint      index)
{
  CtkWidget *widget;
  gint n_columns;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (table));
  if (widget == NULL)
    return -1;

  if (index >= ctk_tree_view_accessible_get_n_children (ATK_OBJECT (table)))
    return -1;

  n_columns = get_n_columns (CTK_TREE_VIEW (widget));

  /* checked by the n_children() check above */
  g_assert (n_columns > 0);

  return index % n_columns;
}

static gint
ctk_tree_view_accessible_get_row_at_index (AtkTable *table,
                                           gint      index)
{
  CtkWidget *widget;
  CtkTreeView *tree_view;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (table));
  if (widget == NULL)
    return -1;

  tree_view = CTK_TREE_VIEW (widget);

  index /= get_n_columns (tree_view);
  index--;
  if (index >= get_n_rows (tree_view))
    return -1;

  return index;
}

static AtkObject *
ctk_tree_view_accessible_table_ref_at (AtkTable *table,
                                       gint      row,
                                       gint      column)
{
  gint index;

  index = ctk_tree_view_accessible_get_index_at (table, row, column);
  if (index == -1)
    return NULL;

  return ctk_tree_view_accessible_ref_child (ATK_OBJECT (table), index);
}

static gint
ctk_tree_view_accessible_get_n_rows (AtkTable *table)
{
  CtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (table));
  if (widget == NULL)
    return 0;

  return get_n_rows (CTK_TREE_VIEW (widget));
}

static gint
ctk_tree_view_accessible_get_n_columns (AtkTable *table)
{
  CtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (table));
  if (widget == NULL)
    return 0;

  return get_n_columns (CTK_TREE_VIEW (widget));
}

static gboolean
ctk_tree_view_accessible_is_row_selected (AtkTable *table,
                                          gint      row)
{
  CtkWidget *widget;
  CtkRBTree *tree;
  CtkRBNode *node;

  if (row < 0)
    return FALSE;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (table));
  if (widget == NULL)
    return FALSE;

  if (!_ctk_rbtree_find_index (_ctk_tree_view_get_rbtree (CTK_TREE_VIEW (widget)),
                               row,
                               &tree,
                               &node))
    return FALSE;

  return CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_IS_SELECTED);
}

static gboolean
ctk_tree_view_accessible_is_selected (AtkTable *table,
                                      gint      row,
                                      gint      column)
{
  return ctk_tree_view_accessible_is_row_selected (table, row);
}

typedef struct {
  GArray *array;
  CtkTreeView *treeview;
} SelectedRowsData;

static void
get_selected_rows (CtkTreeModel *model,
                   CtkTreePath  *path,
                   CtkTreeIter  *iter,
                   gpointer      datap)
{
  SelectedRowsData *data = datap;
  CtkRBTree *tree;
  CtkRBNode *node;
  int id;

  if (_ctk_tree_view_find_node (data->treeview,
                                path,
                                &tree, &node))
    {
      g_assert_not_reached ();
    }

  id = _ctk_rbtree_node_get_index (tree, node);

  g_array_append_val (data->array, id);
}

static gint
ctk_tree_view_accessible_get_selected_rows (AtkTable  *table,
                                            gint     **rows_selected)
{
  SelectedRowsData data;
  CtkWidget *widget;
  gint n_rows;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (table));
  if (widget == NULL)
    {
      if (rows_selected != NULL)
        *rows_selected = NULL;
      return 0;
    }

  data.treeview = CTK_TREE_VIEW (widget);
  data.array = g_array_new (FALSE, FALSE, sizeof (gint));

  ctk_tree_selection_selected_foreach (ctk_tree_view_get_selection (data.treeview),
                                       get_selected_rows,
                                       &data);

  n_rows = data.array->len;
  if (rows_selected)
    *rows_selected = (gint *) g_array_free (data.array, FALSE);
  else
    g_array_free (data.array, TRUE);
  
  return n_rows;
}

static gboolean
ctk_tree_view_accessible_add_row_selection (AtkTable *table,
                                            gint      row)
{
  CtkTreeView *treeview;
  CtkTreePath *path;
  CtkRBTree *tree;
  CtkRBNode *node;

  if (row < 0)
    return FALSE;

  treeview = CTK_TREE_VIEW (ctk_accessible_get_widget (CTK_ACCESSIBLE (table)));
  if (treeview == NULL)
    return FALSE;

  if (!_ctk_rbtree_find_index (_ctk_tree_view_get_rbtree (treeview),
                               row,
                               &tree,
                               &node))
    return FALSE;

  if (CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_IS_SELECTED))
    return FALSE;

  path = _ctk_tree_path_new_from_rbtree (tree, node);
  ctk_tree_selection_select_path (ctk_tree_view_get_selection (treeview), path);
  ctk_tree_path_free (path);

  return TRUE;
}

static gboolean
ctk_tree_view_accessible_remove_row_selection (AtkTable *table,
                                               gint      row)
{
  CtkTreeView *treeview;
  CtkTreePath *path;
  CtkRBTree *tree;
  CtkRBNode *node;

  if (row < 0)
    return FALSE;

  treeview = CTK_TREE_VIEW (ctk_accessible_get_widget (CTK_ACCESSIBLE (table)));
  if (treeview == NULL)
    return FALSE;

  if (!_ctk_rbtree_find_index (_ctk_tree_view_get_rbtree (treeview),
                               row,
                               &tree,
                               &node))
    return FALSE;

  if (! CTK_RBNODE_FLAG_SET (node, CTK_RBNODE_IS_SELECTED))
    return FALSE;

  path = _ctk_tree_path_new_from_rbtree (tree, node);
  ctk_tree_selection_unselect_path (ctk_tree_view_get_selection (treeview), path);
  ctk_tree_path_free (path);

  return TRUE;
}

static AtkObject *
ctk_tree_view_accessible_get_column_header (AtkTable *table,
                                            gint      in_col)
{
  CtkWidget *widget;
  CtkTreeView *tree_view;
  CtkTreeViewColumn *tv_col;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (table));
  if (widget == NULL)
    return NULL;

  tree_view = CTK_TREE_VIEW (widget);
  if (in_col < 0 || in_col >= get_n_columns (tree_view))
    return NULL;

  tv_col = get_visible_column (tree_view, in_col);
  return get_header_from_column (tv_col);
}

static const gchar *
ctk_tree_view_accessible_get_column_description (AtkTable *table,
                                                 gint      in_col)
{
  CtkWidget *widget;
  CtkTreeView *tree_view;
  CtkTreeViewColumn *tv_col;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (table));
  if (widget == NULL)
    return NULL;

  tree_view = CTK_TREE_VIEW (widget);
  if (in_col < 0 || in_col >= get_n_columns (tree_view))
    return NULL;

  tv_col = get_visible_column (tree_view, in_col);
  return ctk_tree_view_column_get_title (tv_col);
}

static void
atk_table_interface_init (AtkTableIface *iface)
{
  iface->ref_at = ctk_tree_view_accessible_table_ref_at;
  iface->get_n_rows = ctk_tree_view_accessible_get_n_rows;
  iface->get_n_columns = ctk_tree_view_accessible_get_n_columns;
  iface->get_index_at = ctk_tree_view_accessible_get_index_at;
  iface->get_column_at_index = ctk_tree_view_accessible_get_column_at_index;
  iface->get_row_at_index = ctk_tree_view_accessible_get_row_at_index;
  iface->is_row_selected = ctk_tree_view_accessible_is_row_selected;
  iface->is_selected = ctk_tree_view_accessible_is_selected;
  iface->get_selected_rows = ctk_tree_view_accessible_get_selected_rows;
  iface->add_row_selection = ctk_tree_view_accessible_add_row_selection;
  iface->remove_row_selection = ctk_tree_view_accessible_remove_row_selection;
  iface->get_column_extent_at = NULL;
  iface->get_row_extent_at = NULL;
  iface->get_column_header = ctk_tree_view_accessible_get_column_header;
  iface->get_column_description = ctk_tree_view_accessible_get_column_description;
}

/* atkselection.h */

static gboolean
ctk_tree_view_accessible_add_selection (AtkSelection *selection,
                                        gint          i)
{
  AtkTable *table;
  gint n_columns;
  gint row;

  table = ATK_TABLE (selection);
  n_columns = ctk_tree_view_accessible_get_n_columns (table);
  if (n_columns != 1)
    return FALSE;

  row = ctk_tree_view_accessible_get_row_at_index (table, i);
  return ctk_tree_view_accessible_add_row_selection (table, row);
}

static gboolean
ctk_tree_view_accessible_clear_selection (AtkSelection *selection)
{
  CtkWidget *widget;
  CtkTreeView *tree_view;
  CtkTreeSelection *tree_selection;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (selection));
  if (widget == NULL)
    return FALSE;

  tree_view = CTK_TREE_VIEW (widget);
  tree_selection = ctk_tree_view_get_selection (tree_view);

  ctk_tree_selection_unselect_all (tree_selection);
  return TRUE;
}

static AtkObject *
ctk_tree_view_accessible_ref_selection (AtkSelection *selection,
                                        gint          i)
{
  AtkTable *table;
  gint row;
  gint n_selected;
  gint n_columns;
  gint *selected;

  table = ATK_TABLE (selection);
  n_columns = ctk_tree_view_accessible_get_n_columns (table);
  n_selected = ctk_tree_view_accessible_get_selected_rows (table, &selected);
  if (n_columns == 0 || i >= n_columns * n_selected)
    return NULL;

  row = selected[i / n_columns];
  g_free (selected);

  return ctk_tree_view_accessible_table_ref_at (table, row, i % n_columns);
}

static gint
ctk_tree_view_accessible_get_selection_count (AtkSelection *selection)
{
  AtkTable *table;
  gint n_selected;

  table = ATK_TABLE (selection);
  n_selected = ctk_tree_view_accessible_get_selected_rows (table, NULL);
  if (n_selected > 0)
    n_selected *= ctk_tree_view_accessible_get_n_columns (table);
  return n_selected;
}

static gboolean
ctk_tree_view_accessible_is_child_selected (AtkSelection *selection,
                                            gint          i)
{
  CtkWidget *widget;
  gint row;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (selection));
  if (widget == NULL)
    return FALSE;

  row = ctk_tree_view_accessible_get_row_at_index (ATK_TABLE (selection), i);

  return ctk_tree_view_accessible_is_row_selected (ATK_TABLE (selection), row);
}

static void atk_selection_interface_init (AtkSelectionIface *iface)
{
  iface->add_selection = ctk_tree_view_accessible_add_selection;
  iface->clear_selection = ctk_tree_view_accessible_clear_selection;
  iface->ref_selection = ctk_tree_view_accessible_ref_selection;
  iface->get_selection_count = ctk_tree_view_accessible_get_selection_count;
  iface->is_child_selected = ctk_tree_view_accessible_is_child_selected;
}

#define EXTRA_EXPANDER_PADDING 4

static void
ctk_tree_view_accessible_get_cell_area (CtkCellAccessibleParent *parent,
                                        CtkCellAccessible       *cell,
                                        CdkRectangle            *cell_rect)
{
  CtkWidget *widget;
  CtkTreeView *tree_view;
  CtkTreeViewColumn *tv_col;
  CtkTreePath *path;
  AtkObject *parent_cell;
  CtkTreeViewAccessibleCellInfo *cell_info;
  CtkCellAccessible *top_cell;

  /* Default value. */
  cell_rect->x = 0;
  cell_rect->y = 0;
  cell_rect->width = 0;
  cell_rect->height = 0;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (parent));
  if (widget == NULL)
    return;

  tree_view = CTK_TREE_VIEW (widget);
  parent_cell = atk_object_get_parent (ATK_OBJECT (cell));
  if (parent_cell != ATK_OBJECT (parent))
    top_cell = CTK_CELL_ACCESSIBLE (parent_cell);
  else
    top_cell = cell;
  cell_info = find_cell_info (CTK_TREE_VIEW_ACCESSIBLE (parent), top_cell);
  if (!cell_info)
    return;
  path = cell_info_get_path (cell_info);
  tv_col = cell_info->cell_col_ref;
  if (path)
    {
      CtkTreeViewColumn *expander_column;

      ctk_tree_view_get_cell_area (tree_view, path, tv_col, cell_rect);
      expander_column = ctk_tree_view_get_expander_column (tree_view);
      if (expander_column == tv_col)
        {
          gint expander_size;
          ctk_widget_style_get (widget,
                                "expander-size", &expander_size,
                                NULL);
          cell_rect->x += expander_size + EXTRA_EXPANDER_PADDING;
          cell_rect->width -= expander_size + EXTRA_EXPANDER_PADDING;
        }

      ctk_tree_path_free (path);

      /* A column has more than one renderer so we find the position
       * and width of each
       */
      if (top_cell != cell)
        {
          gint cell_index;
          gboolean found;
          gint cell_start;
          gint cell_width;
          GList *renderers;
          CtkCellRenderer *renderer;

          cell_index = atk_object_get_index_in_parent (ATK_OBJECT (cell));
          renderers = ctk_cell_layout_get_cells (CTK_CELL_LAYOUT (tv_col));
          renderer = g_list_nth_data (renderers, cell_index);

          found = ctk_tree_view_column_cell_get_position (tv_col, renderer, &cell_start, &cell_width);
          if (found)
            {
              cell_rect->x += cell_start;
              cell_rect->width = cell_width;
            }
          g_list_free (renderers);
        }

    }
}

static void
ctk_tree_view_accessible_get_cell_extents (CtkCellAccessibleParent *parent,
                                           CtkCellAccessible       *cell,
                                           gint                    *x,
                                           gint                    *y,
                                           gint                    *width,
                                           gint                    *height,
                                           AtkCoordType             coord_type)
{
  CtkWidget *widget;
  CtkTreeView *tree_view;
  CdkWindow *bin_window;
  CdkRectangle cell_rect;
  gint w_x, w_y;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (parent));
  if (widget == NULL)
    return;

  tree_view = CTK_TREE_VIEW (widget);
  ctk_tree_view_accessible_get_cell_area (parent, cell, &cell_rect);
  bin_window = ctk_tree_view_get_bin_window (tree_view);
  cdk_window_get_origin (bin_window, &w_x, &w_y);

  if (coord_type == ATK_XY_WINDOW)
    {
      CdkWindow *window;
      gint x_toplevel, y_toplevel;

      window = cdk_window_get_toplevel (bin_window);
      cdk_window_get_origin (window, &x_toplevel, &y_toplevel);

      w_x -= x_toplevel;
      w_y -= y_toplevel;
    }

  *width = cell_rect.width;
  *height = cell_rect.height;
  if (is_cell_showing (tree_view, &cell_rect))
    {
      *x = cell_rect.x + w_x;
      *y = cell_rect.y + w_y;
    }
  else
    {
      *x = G_MININT;
      *y = G_MININT;
    }
}

static gboolean
ctk_tree_view_accessible_grab_cell_focus (CtkCellAccessibleParent *parent,
                                          CtkCellAccessible       *cell)
{
  CtkWidget *widget;
  CtkTreeView *tree_view;
  CtkTreeViewColumn *tv_col;
  CtkTreePath *path;
  AtkObject *parent_cell;
  AtkObject *cell_object;
  CtkTreeViewAccessibleCellInfo *cell_info;
  CtkCellRenderer *renderer = NULL;
  CtkWidget *toplevel;
  gint index;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (parent));
  if (widget == NULL)
    return FALSE;

  tree_view = CTK_TREE_VIEW (widget);

  cell_info = find_cell_info (CTK_TREE_VIEW_ACCESSIBLE (parent), cell);
  if (!cell_info)
    return FALSE;
  cell_object = ATK_OBJECT (cell);
  parent_cell = atk_object_get_parent (cell_object);
  tv_col = cell_info->cell_col_ref;
  if (parent_cell != ATK_OBJECT (parent))
    {
      /* CtkCellAccessible is in a CtkContainerCellAccessible.
       * The CtkTreeViewColumn has multiple renderers;
       * find the corresponding one.
       */
      GList *renderers;

      renderers = ctk_cell_layout_get_cells (CTK_CELL_LAYOUT (tv_col));
      index = atk_object_get_index_in_parent (cell_object);
      renderer = g_list_nth_data (renderers, index);
      g_list_free (renderers);
    }
  path = cell_info_get_path (cell_info);
  if (path)
    {
      if (renderer)
        ctk_tree_view_set_cursor_on_cell (tree_view, path, tv_col, renderer, FALSE);
      else
        ctk_tree_view_set_cursor (tree_view, path, tv_col, FALSE);

      ctk_tree_path_free (path);
      ctk_widget_grab_focus (widget);
      toplevel = ctk_widget_get_toplevel (widget);
      if (ctk_widget_is_toplevel (toplevel))
        {
#ifdef CDK_WINDOWING_X11
          if (CDK_IS_X11_DISPLAY (ctk_widget_get_display (toplevel)))
            ctk_window_present_with_time (CTK_WINDOW (toplevel),
                                          cdk_x11_get_server_time (ctk_widget_get_window (widget)));
          else
#endif
            {
              ctk_window_present (CTK_WINDOW (toplevel));
            }
        }

      return TRUE;
    }
  else
      return FALSE;
}

static int
ctk_tree_view_accessible_get_child_index (CtkCellAccessibleParent *parent,
                                          CtkCellAccessible       *cell)
{
  CtkTreeViewAccessibleCellInfo *cell_info;
  CtkTreeView *tree_view;

  cell_info = find_cell_info (CTK_TREE_VIEW_ACCESSIBLE (parent), cell);
  if (!cell_info)
    return -1;

  tree_view = CTK_TREE_VIEW (ctk_accessible_get_widget (CTK_ACCESSIBLE (parent)));

  return cell_info_get_index (tree_view, cell_info);
}

static CtkCellRendererState
ctk_tree_view_accessible_get_renderer_state (CtkCellAccessibleParent *parent,
                                             CtkCellAccessible       *cell)
{
  CtkTreeViewAccessibleCellInfo *cell_info;
  CtkTreeView *treeview;
  CtkCellRendererState flags;

  cell_info = find_cell_info (CTK_TREE_VIEW_ACCESSIBLE (parent), cell);
  if (!cell_info)
    return 0;

  flags = 0;

  if (CTK_RBNODE_FLAG_SET (cell_info->node, CTK_RBNODE_IS_SELECTED))
    flags |= CTK_CELL_RENDERER_SELECTED;

  if (CTK_RBNODE_FLAG_SET (cell_info->node, CTK_RBNODE_IS_PRELIT))
    flags |= CTK_CELL_RENDERER_PRELIT;

  if (ctk_tree_view_column_get_sort_indicator (cell_info->cell_col_ref))
    flags |= CTK_CELL_RENDERER_SORTED;

  treeview = CTK_TREE_VIEW (ctk_accessible_get_widget (CTK_ACCESSIBLE (parent)));

  if (cell_info->cell_col_ref == ctk_tree_view_get_expander_column (treeview))
    {
      if (CTK_RBNODE_FLAG_SET (cell_info->node, CTK_RBNODE_IS_PARENT))
        flags |= CTK_CELL_RENDERER_EXPANDABLE;

      if (cell_info->node->children)
        flags |= CTK_CELL_RENDERER_EXPANDED;
    }

  if (ctk_widget_has_focus (CTK_WIDGET (treeview)))
    {
      CtkTreeViewColumn *column;
      CtkTreePath *path;
      CtkRBTree *tree;
      CtkRBNode *node = NULL;
      
      ctk_tree_view_get_cursor (treeview, &path, &column);
      if (path)
        {
          _ctk_tree_view_find_node (treeview, path, &tree, &node);
          ctk_tree_path_free (path);
        }
      else
        tree = NULL;

      if (cell_info->cell_col_ref == column
          && cell_info->tree == tree
          && cell_info->node == node)
        flags |= CTK_CELL_RENDERER_FOCUSED;
    }

  return flags;
}

static void
ctk_tree_view_accessible_expand_collapse (CtkCellAccessibleParent *parent,
                                          CtkCellAccessible       *cell)
{
  CtkTreeViewAccessibleCellInfo *cell_info;
  CtkTreeView *treeview;
  CtkTreePath *path;

  treeview = CTK_TREE_VIEW (ctk_accessible_get_widget (CTK_ACCESSIBLE (parent)));

  cell_info = find_cell_info (CTK_TREE_VIEW_ACCESSIBLE (parent), cell);
  if (!cell_info ||
      cell_info->cell_col_ref != ctk_tree_view_get_expander_column (treeview))
    return;

  path = cell_info_get_path (cell_info);

  if (cell_info->node->children)
    ctk_tree_view_collapse_row (treeview, path);
  else
    ctk_tree_view_expand_row (treeview, path, FALSE);

  ctk_tree_path_free (path);
}

static void
ctk_tree_view_accessible_activate (CtkCellAccessibleParent *parent,
                                   CtkCellAccessible       *cell)
{
  CtkTreeViewAccessibleCellInfo *cell_info;
  CtkTreeView *treeview;
  CtkTreePath *path;

  treeview = CTK_TREE_VIEW (ctk_accessible_get_widget (CTK_ACCESSIBLE (parent)));

  cell_info = find_cell_info (CTK_TREE_VIEW_ACCESSIBLE (parent), cell);
  if (!cell_info)
    return;

  path = cell_info_get_path (cell_info);

  ctk_tree_view_row_activated (treeview, path, cell_info->cell_col_ref);

  ctk_tree_path_free (path);
}

static void
ctk_tree_view_accessible_edit (CtkCellAccessibleParent *parent,
                               CtkCellAccessible       *cell)
{
  CtkTreeView *treeview;

  if (!ctk_tree_view_accessible_grab_cell_focus (parent, cell))
    return;

  treeview = CTK_TREE_VIEW (ctk_accessible_get_widget (CTK_ACCESSIBLE (parent)));

  g_signal_emit_by_name (treeview,
                         "real-select-cursor-row",
                         TRUE);
}

static void
ctk_tree_view_accessible_update_relationset (CtkCellAccessibleParent *parent,
                                             CtkCellAccessible       *cell,
                                             AtkRelationSet          *relationset)
{
  CtkTreeViewAccessibleCellInfo *cell_info;
  CtkTreeViewAccessible *accessible;
  CtkTreeViewColumn *column;
  CtkTreeView *treeview;
  AtkRelation *relation;
  CtkRBTree *tree;
  AtkObject *object;

  /* Don't set relations on cells that aren't direct descendants of the treeview.
   * So only set it on the container, not on the renderer accessibles */
  if (atk_object_get_parent (ATK_OBJECT (cell)) != ATK_OBJECT (parent))
    return;

  accessible = CTK_TREE_VIEW_ACCESSIBLE (parent);
  cell_info = find_cell_info (accessible, cell);
  if (!cell_info)
    return;

  /* only set parent/child rows on the expander column */
  treeview = CTK_TREE_VIEW (ctk_accessible_get_widget (CTK_ACCESSIBLE (parent)));
  column = ctk_tree_view_get_expander_column (treeview);
  if (column != cell_info->cell_col_ref)
    return;

  /* Update CHILD_OF relation to parent cell */
  relation = atk_relation_set_get_relation_by_type (relationset, ATK_RELATION_NODE_CHILD_OF);
  if (relation)
    atk_relation_set_remove (relationset, relation);

  if (cell_info->tree->parent_tree)
    {
      object = ATK_OBJECT (peek_cell (accessible, cell_info->tree->parent_tree, cell_info->tree->parent_node, column));
      if (object == NULL)
        object = ATK_OBJECT (create_cell (treeview, accessible, cell_info->tree->parent_tree, cell_info->tree->parent_node, column));
    }
  else
    object = ATK_OBJECT (accessible);

  atk_relation_set_add_relation_by_type (relationset, ATK_RELATION_NODE_CHILD_OF, object);

  /* Update PARENT_OF relation for all child cells */
  relation = atk_relation_set_get_relation_by_type (relationset, ATK_RELATION_NODE_PARENT_OF);
  if (relation)
    atk_relation_set_remove (relationset, relation);

  tree = cell_info->node->children;
  if (tree)
    {
      CtkRBNode *node;

      for (node = _ctk_rbtree_first (tree);
           node != NULL;
           node = _ctk_rbtree_next (tree, node))
        {
          object = ATK_OBJECT (peek_cell (accessible, tree, node, column));
          if (object == NULL)
            object = ATK_OBJECT (create_cell (treeview, accessible, tree, node, column));

          atk_relation_set_add_relation_by_type (relationset, ATK_RELATION_NODE_PARENT_OF, ATK_OBJECT (object));
        }
    }
}

static void
ctk_tree_view_accessible_get_cell_position (CtkCellAccessibleParent *parent,
                                            CtkCellAccessible       *cell,
                                            gint                    *row,
                                            gint                    *column)
{
  CtkWidget *widget;
  CtkTreeView *tree_view;
  CtkTreeViewAccessibleCellInfo *cell_info;
  CtkTreeViewAccessible *accessible;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (parent));
  if (widget == NULL)
    return;

  tree_view = CTK_TREE_VIEW (widget);
  accessible = CTK_TREE_VIEW_ACCESSIBLE (parent);
  cell_info = find_cell_info (accessible, cell);
  if (!cell_info)
    return;

  if (row)
    (*row) = _ctk_rbtree_node_get_index (cell_info->tree, cell_info->node);
  if (column)
    (*column) = get_column_number (tree_view, cell_info->cell_col_ref);
}

static GPtrArray *
ctk_tree_view_accessible_get_column_header_cells (CtkCellAccessibleParent *parent,
                                                  CtkCellAccessible       *cell)
{
  CtkWidget *widget;
  CtkTreeViewAccessibleCellInfo *cell_info;
  CtkTreeViewAccessible *accessible;
  GPtrArray *array;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (parent));
  if (widget == NULL)
    return NULL;

  accessible = CTK_TREE_VIEW_ACCESSIBLE (parent);
  cell_info = find_cell_info (accessible, cell);
  if (!cell_info)
    return NULL;

  array = g_ptr_array_new_full (1, g_object_unref);
  g_ptr_array_add (array, g_object_ref (get_header_from_column ( (cell_info->cell_col_ref))));
  return array;
}

static void
ctk_cell_accessible_parent_interface_init (CtkCellAccessibleParentIface *iface)
{
  iface->get_cell_extents = ctk_tree_view_accessible_get_cell_extents;
  iface->get_cell_area = ctk_tree_view_accessible_get_cell_area;
  iface->grab_focus = ctk_tree_view_accessible_grab_cell_focus;
  iface->get_child_index = ctk_tree_view_accessible_get_child_index;
  iface->get_renderer_state = ctk_tree_view_accessible_get_renderer_state;
  iface->expand_collapse = ctk_tree_view_accessible_expand_collapse;
  iface->activate = ctk_tree_view_accessible_activate;
  iface->edit = ctk_tree_view_accessible_edit;
  iface->update_relationset = ctk_tree_view_accessible_update_relationset;
  iface->get_cell_position = ctk_tree_view_accessible_get_cell_position;
  iface->get_column_header_cells = ctk_tree_view_accessible_get_column_header_cells;
}

void
_ctk_tree_view_accessible_reorder (CtkTreeView *treeview)
{
  CtkTreeViewAccessible *accessible;

  accessible = CTK_TREE_VIEW_ACCESSIBLE (_ctk_widget_peek_accessible (CTK_WIDGET (treeview)));
  if (accessible == NULL)
    return;

  g_signal_emit_by_name (accessible, "row-reordered");
}

static gboolean
is_cell_showing (CtkTreeView  *tree_view,
                 CdkRectangle *cell_rect)
{
  CdkRectangle rect, *visible_rect;
  CdkRectangle rect1, *tree_cell_rect;
  gint bx, by;
  gboolean is_showing;

 /* A cell is considered "SHOWING" if any part of the cell is
  * in the visible area. Other ways we could do this is by a
  * cell's midpoint or if the cell is fully in the visible range.
  * Since we have the cell_rect x, y, width, height of the cell,
  * any of these is easy to compute.
  *
  * It is assumed that cell's rectangle is in widget coordinates
  * so we must transform to tree cordinates.
  */
  visible_rect = &rect;
  tree_cell_rect = &rect1;
  tree_cell_rect->x = cell_rect->x;
  tree_cell_rect->y = cell_rect->y;
  tree_cell_rect->width = cell_rect->width;
  tree_cell_rect->height = cell_rect->height;

  ctk_tree_view_get_visible_rect (tree_view, visible_rect);
  ctk_tree_view_convert_tree_to_bin_window_coords (tree_view, visible_rect->x,
                                                   visible_rect->y, &bx, &by);

  if (((tree_cell_rect->x + tree_cell_rect->width) < bx) ||
     ((tree_cell_rect->y + tree_cell_rect->height) < by) ||
     (tree_cell_rect->x > (bx + visible_rect->width)) ||
     (tree_cell_rect->y > (by + visible_rect->height)))
    is_showing =  FALSE;
  else
    is_showing = TRUE;

  return is_showing;
}

/* Misc Private */

static int
cell_info_get_index (CtkTreeView                     *tree_view,
                     CtkTreeViewAccessibleCellInfo   *info)
{
  int index;

  index = _ctk_rbtree_node_get_index (info->tree, info->node) + 1;
  index *= get_n_columns (tree_view);
  index += get_column_number (tree_view, info->cell_col_ref);

  return index;
}

static void
cell_info_new (CtkTreeViewAccessible *accessible,
               CtkRBTree             *tree,
               CtkRBNode             *node,
               CtkTreeViewColumn     *tv_col,
               CtkCellAccessible     *cell)
{
  CtkTreeViewAccessibleCellInfo *cell_info;

  cell_info = g_new (CtkTreeViewAccessibleCellInfo, 1);

  cell_info->tree = tree;
  cell_info->node = node;
  cell_info->cell_col_ref = tv_col;
  cell_info->cell = cell;
  cell_info->view = accessible;

  g_object_set_qdata (G_OBJECT (cell), 
                      ctk_tree_view_accessible_get_data_quark (),
                      cell_info);

  g_hash_table_replace (accessible->priv->cell_infos, cell_info, cell_info);
}

/* Returns the column number of the specified CtkTreeViewColumn
 * The column must be visible.
 */
static gint
get_column_number (CtkTreeView       *treeview,
                   CtkTreeViewColumn *column)
{
  guint i, number;

  number = 0;

  for (i = 0; i < ctk_tree_view_get_n_columns (treeview); i++)
    {
      CtkTreeViewColumn *cur;

      cur = ctk_tree_view_get_column (treeview, i);
      
      if (!ctk_tree_view_column_get_visible (cur))
        continue;

      if (cur == column)
        break;

      number++;
    }

  g_return_val_if_fail (i < ctk_tree_view_get_n_columns (treeview), 0);

  return number;
}

static gboolean
get_rbtree_column_from_index (CtkTreeView        *tree_view,
                              gint                index,
                              CtkRBTree         **tree,
                              CtkRBNode         **node,
                              CtkTreeViewColumn **column)
{
  guint n_columns = get_n_columns (tree_view);

  if (n_columns == 0)
    return FALSE;
  /* First row is the column headers */
  index -= n_columns;
  if (index < 0)
    return FALSE;

  if (tree)
    {
      g_return_val_if_fail (node != NULL, FALSE);

      if (!_ctk_rbtree_find_index (_ctk_tree_view_get_rbtree (tree_view),
                                   index / n_columns,
                                   tree,
                                   node))
        return FALSE;
    }

  if (column)
    {
      *column = get_visible_column (tree_view, index % n_columns);
      if (*column == NULL)
        return FALSE;
  }
  return TRUE;
}

static CtkTreeViewAccessibleCellInfo *
find_cell_info (CtkTreeViewAccessible *accessible,
                CtkCellAccessible     *cell)
{
  AtkObject *parent;
  
  parent = atk_object_get_parent (ATK_OBJECT (cell));
  while (parent != ATK_OBJECT (accessible))
    {
      cell = CTK_CELL_ACCESSIBLE (parent);
      parent = atk_object_get_parent (ATK_OBJECT (cell));
    }

  return g_object_get_qdata (G_OBJECT (cell),
                             ctk_tree_view_accessible_get_data_quark ());
}

static AtkObject *
get_header_from_column (CtkTreeViewColumn *tv_col)
{
  AtkObject *rc;
  CtkWidget *header_widget;

  if (tv_col == NULL)
    return NULL;

  header_widget = ctk_tree_view_column_get_button (tv_col);

  if (header_widget)
    rc = ctk_widget_get_accessible (header_widget);
  else
    rc = NULL;

  return rc;
}

void
_ctk_tree_view_accessible_add (CtkTreeView *treeview,
                               CtkRBTree   *tree,
                               CtkRBNode   *node)
{
  CtkTreeViewAccessible *accessible;
  guint row, n_rows, n_cols, i;

  accessible = CTK_TREE_VIEW_ACCESSIBLE (_ctk_widget_peek_accessible (CTK_WIDGET (treeview)));
  if (accessible == NULL)
    return;

  if (node == NULL)
    {
      row = tree->parent_tree ? _ctk_rbtree_node_get_index (tree->parent_tree, tree->parent_node) : 0;
      n_rows = tree->root->total_count;
    }
  else
    {
      row = _ctk_rbtree_node_get_index (tree, node);
      n_rows = 1 + (node->children ? node->children->root->total_count : 0);
    }

  g_signal_emit_by_name (accessible, "row-inserted", row, n_rows);

  n_cols = get_n_columns (treeview);
  if (n_cols)
    {
      for (i = (row + 1) * n_cols; i < (row + n_rows + 1) * n_cols; i++)
        {
         /* Pass NULL as the child object, i.e. 4th argument */
          g_signal_emit_by_name (accessible, "children-changed::add", i, NULL, NULL);
        }
    }
}

void
_ctk_tree_view_accessible_remove (CtkTreeView *treeview,
                                  CtkRBTree   *tree,
                                  CtkRBNode   *node)
{
  CtkTreeViewAccessibleCellInfo *cell_info;
  GHashTableIter iter;
  CtkTreeViewAccessible *accessible;
  guint row, n_rows, n_cols, i;

  accessible = CTK_TREE_VIEW_ACCESSIBLE (_ctk_widget_peek_accessible (CTK_WIDGET (treeview)));
  if (accessible == NULL)
    return;

  /* if this shows up in profiles, special-case node->children == NULL */

  if (node == NULL)
    {
      row = tree->parent_tree ? _ctk_rbtree_node_get_index (tree->parent_tree, tree->parent_node) : 0;
      n_rows = tree->root->total_count + 1;
    }
  else
    {
      row = _ctk_rbtree_node_get_index (tree, node);
      n_rows = 1 + (node->children ? node->children->root->total_count : 0);

      tree = node->children;
    }

  g_signal_emit_by_name (accessible, "row-deleted", row, n_rows);

  n_cols = get_n_columns (treeview);
  if (n_cols)
    {
      for (i = (n_rows + row + 1) * n_cols - 1; i >= (row + 1) * n_cols; i--)
        {
         /* Pass NULL as the child object, i.e. 4th argument */
          g_signal_emit_by_name (accessible, "children-changed::remove", i, NULL, NULL);
        }

      g_hash_table_iter_init (&iter, accessible->priv->cell_infos);
      while (g_hash_table_iter_next (&iter, NULL, (gpointer *)&cell_info))
        {
          if (node == cell_info->node ||
              tree == cell_info->tree ||
              (tree && _ctk_rbtree_contains (tree, cell_info->tree)))
            g_hash_table_iter_remove (&iter);
        }
    }
}

void
_ctk_tree_view_accessible_changed (CtkTreeView *treeview,
                                   CtkRBTree   *tree,
                                   CtkRBNode   *node)
{
  CtkTreeViewAccessible *accessible;
  guint i;

  accessible = CTK_TREE_VIEW_ACCESSIBLE (ctk_widget_get_accessible (CTK_WIDGET (treeview)));

  for (i = 0; i < ctk_tree_view_get_n_columns (treeview); i++)
    {
      CtkCellAccessible *cell = peek_cell (accessible,
                                           tree, node,
                                           ctk_tree_view_get_column (treeview, i));

      if (cell == NULL)
        continue;

      set_cell_data (treeview, accessible, cell);
      _ctk_cell_accessible_update_cache (cell, TRUE);
    }

  g_signal_emit_by_name (accessible, "visible-data-changed");
}

/* NB: id is not checked, only columns < id are.
 * This is important so the function works for notification of removal of a column */
static guint
to_visible_column_id (CtkTreeView *treeview,
                      guint        id)
{
  guint i;
  guint invisible;

  invisible = 0;

  for (i = 0; i < id; i++)
    {
      CtkTreeViewColumn *column = ctk_tree_view_get_column (treeview, i);

      if (!ctk_tree_view_column_get_visible (column))
        invisible++;
    }

  return id - invisible;
}

static void
ctk_tree_view_accessible_do_add_column (CtkTreeViewAccessible *accessible,
                                        CtkTreeView           *treeview,
                                        CtkTreeViewColumn     *column,
                                        guint                  id)
{
  guint row, n_rows, n_cols;

  /* Generate column-inserted signal */
  g_signal_emit_by_name (accessible, "column-inserted", id, 1);

  n_rows = get_n_rows (treeview);
  n_cols = get_n_columns (treeview);

  /* Generate children-changed signals */
  for (row = 0; row <= n_rows; row++)
    {
     /* Pass NULL as the child object, i.e. 4th argument */
      g_signal_emit_by_name (accessible, "children-changed::add",
                             (row * n_cols) + id, NULL, NULL);
    }
}

void
_ctk_tree_view_accessible_add_column (CtkTreeView       *treeview,
                                      CtkTreeViewColumn *column,
                                      guint              id)
{
  AtkObject *obj;

  if (!ctk_tree_view_column_get_visible (column))
    return;

  obj = _ctk_widget_peek_accessible (CTK_WIDGET (treeview));
  if (obj == NULL)
    return;

  ctk_tree_view_accessible_do_add_column (CTK_TREE_VIEW_ACCESSIBLE (obj),
                                          treeview,
                                          column,
                                          to_visible_column_id (treeview, id));
}

static void
ctk_tree_view_accessible_do_remove_column (CtkTreeViewAccessible *accessible,
                                           CtkTreeView           *treeview,
                                           CtkTreeViewColumn     *column,
                                           guint                  id)
{
  GHashTableIter iter;
  gpointer value;
  guint row, n_rows, n_cols;

  /* Clean column from cache */
  g_hash_table_iter_init (&iter, accessible->priv->cell_infos);
  while (g_hash_table_iter_next (&iter, NULL, &value))
    {
      CtkTreeViewAccessibleCellInfo *cell_info;

      cell_info = value;
      if (cell_info->cell_col_ref == column)
        g_hash_table_iter_remove (&iter);
    }

  /* Generate column-deleted signal */
  g_signal_emit_by_name (accessible, "column-deleted", id, 1);

  n_rows = get_n_rows (treeview);
  n_cols = get_n_columns (treeview);

  /* Generate children-changed signals */
  for (row = 0; row <= n_rows; row++)
    {
      /* Pass NULL as the child object, 4th argument */
      g_signal_emit_by_name (accessible, "children-changed::remove",
                             (row * n_cols) + id, NULL, NULL);
    }
}

void
_ctk_tree_view_accessible_remove_column (CtkTreeView       *treeview,
                                         CtkTreeViewColumn *column,
                                         guint              id)
{
  AtkObject *obj;

  if (!ctk_tree_view_column_get_visible (column))
    return;

  obj = _ctk_widget_peek_accessible (CTK_WIDGET (treeview));
  if (obj == NULL)
    return;

  ctk_tree_view_accessible_do_remove_column (CTK_TREE_VIEW_ACCESSIBLE (obj),
                                             treeview,
                                             column,
                                             to_visible_column_id (treeview, id));
}

void
_ctk_tree_view_accessible_reorder_column (CtkTreeView       *treeview,
                                          CtkTreeViewColumn *column)
{
  AtkObject *obj;

  obj = _ctk_widget_peek_accessible (CTK_WIDGET (treeview));
  if (obj == NULL)
    return;

  g_signal_emit_by_name (obj, "column-reordered");
}

void
_ctk_tree_view_accessible_toggle_visibility (CtkTreeView       *treeview,
                                             CtkTreeViewColumn *column)
{
  AtkObject *obj;
  guint i, id;

  obj = _ctk_widget_peek_accessible (CTK_WIDGET (treeview));
  if (obj == NULL)
    return;

  if (ctk_tree_view_column_get_visible (column))
    {
      id = get_column_number (treeview, column);

      ctk_tree_view_accessible_do_add_column (CTK_TREE_VIEW_ACCESSIBLE (obj),
                                              treeview,
                                              column,
                                              id);
    }
  else
    {
      id = 0;

      for (i = 0; i < ctk_tree_view_get_n_columns (treeview); i++)
        {
          CtkTreeViewColumn *cur = ctk_tree_view_get_column (treeview, i);
          
          if (ctk_tree_view_column_get_visible (cur))
            id++;

          if (cur == column)
            break;
        }

      ctk_tree_view_accessible_do_remove_column (CTK_TREE_VIEW_ACCESSIBLE (obj),
                                                 treeview,
                                                 column,
                                                 id);
    }
}

static CtkTreeViewColumn *
get_effective_focus_column (CtkTreeView       *treeview,
                            CtkTreeViewColumn *column)
{
  if (column == NULL && get_n_columns (treeview) > 0)
    column = get_visible_column (treeview, 0);

  return column;
}

void
_ctk_tree_view_accessible_update_focus_column (CtkTreeView       *treeview,
                                               CtkTreeViewColumn *old_focus,
                                               CtkTreeViewColumn *new_focus)
{
  CtkTreeViewAccessible *accessible;
  AtkObject *obj;
  CtkRBTree *cursor_tree;
  CtkRBNode *cursor_node;
  CtkCellAccessible *cell;

  old_focus = get_effective_focus_column (treeview, old_focus);
  new_focus = get_effective_focus_column (treeview, new_focus);
  if (old_focus == new_focus)
    return;

  obj = _ctk_widget_peek_accessible (CTK_WIDGET (treeview));
  if (obj == NULL)
    return;

  accessible = CTK_TREE_VIEW_ACCESSIBLE (obj);

  if (!_ctk_tree_view_get_cursor_node (treeview, &cursor_tree, &cursor_node))
    return;

  if (old_focus)
    {
      cell = peek_cell (accessible, cursor_tree, cursor_node, old_focus);
      if (cell != NULL)
        _ctk_cell_accessible_state_changed (cell, CTK_CELL_RENDERER_FOCUSED, 0);
    }

  if (new_focus)
    {
      cell = peek_cell (accessible, cursor_tree, cursor_node, new_focus);
      if (cell != NULL)
        _ctk_cell_accessible_state_changed (cell, 0, CTK_CELL_RENDERER_FOCUSED);
      else
        cell = create_cell (treeview, accessible, cursor_tree, cursor_node, new_focus);

      g_signal_emit_by_name (accessible, "active-descendant-changed", cell);
    }
}

void
_ctk_tree_view_accessible_add_state (CtkTreeView          *treeview,
                                     CtkRBTree            *tree,
                                     CtkRBNode            *node,
                                     CtkCellRendererState  state)
{
  CtkTreeViewAccessible *accessible;
  CtkTreeViewColumn *single_column;
  AtkObject *obj;
  guint i;

  obj = _ctk_widget_peek_accessible (CTK_WIDGET (treeview));
  if (obj == NULL)
    return;

  accessible = CTK_TREE_VIEW_ACCESSIBLE (obj);

  if (state == CTK_CELL_RENDERER_FOCUSED)
    {
      single_column = get_effective_focus_column (treeview, _ctk_tree_view_get_focus_column (treeview));
    }
  else if (state == CTK_CELL_RENDERER_EXPANDED ||
           state == CTK_CELL_RENDERER_EXPANDABLE)
    {
      single_column = ctk_tree_view_get_expander_column (treeview);
    }
  else
    single_column = NULL;

  if (single_column)
    {
      CtkCellAccessible *cell = peek_cell (accessible,
                                           tree, node,
                                           single_column);

      if (cell != NULL)
        _ctk_cell_accessible_state_changed (cell, state, 0);

      if (state == CTK_CELL_RENDERER_FOCUSED)
        {
          if (cell == NULL)
            cell = create_cell (treeview, accessible, tree, node, single_column);
          
          g_signal_emit_by_name (accessible, "active-descendant-changed", cell);
        }
    }
  else
    {
      for (i = 0; i < ctk_tree_view_get_n_columns (treeview); i++)
        {
          CtkCellAccessible *cell = peek_cell (accessible,
                                               tree, node,
                                               ctk_tree_view_get_column (treeview, i));

          if (cell == NULL)
            continue;

          _ctk_cell_accessible_state_changed (cell, state, 0);
        }
    }

  if (state == CTK_CELL_RENDERER_SELECTED)
    g_signal_emit_by_name (accessible, "selection-changed");
}

void
_ctk_tree_view_accessible_remove_state (CtkTreeView          *treeview,
                                        CtkRBTree            *tree,
                                        CtkRBNode            *node,
                                        CtkCellRendererState  state)
{
  CtkTreeViewAccessible *accessible;
  CtkTreeViewColumn *single_column;
  AtkObject *obj;
  guint i;

  obj = _ctk_widget_peek_accessible (CTK_WIDGET (treeview));
  if (obj == NULL)
    return;

  accessible = CTK_TREE_VIEW_ACCESSIBLE (obj);

  if (state == CTK_CELL_RENDERER_FOCUSED)
    {
      single_column = get_effective_focus_column (treeview, _ctk_tree_view_get_focus_column (treeview));
    }
  else if (state == CTK_CELL_RENDERER_EXPANDED ||
           state == CTK_CELL_RENDERER_EXPANDABLE)
    {
      single_column = ctk_tree_view_get_expander_column (treeview);
    }
  else
    single_column = NULL;

  if (single_column)
    {
      CtkCellAccessible *cell = peek_cell (accessible,
                                           tree, node,
                                           single_column);

      if (cell != NULL)
        _ctk_cell_accessible_state_changed (cell, 0, state);
    }
  else
    {
      for (i = 0; i < ctk_tree_view_get_n_columns (treeview); i++)
        {
          CtkCellAccessible *cell = peek_cell (accessible,
                                               tree, node,
                                               ctk_tree_view_get_column (treeview, i));

          if (cell == NULL)
            continue;

          _ctk_cell_accessible_state_changed (cell, 0, state);
        }
    }

  if (state == CTK_CELL_RENDERER_SELECTED)
    g_signal_emit_by_name (accessible, "selection-changed");
}
