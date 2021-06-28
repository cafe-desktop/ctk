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

#ifndef __CTK_TREE_SELECTION_H__
#define __CTK_TREE_SELECTION_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctktreeview.h>

G_BEGIN_DECLS


#define CTK_TYPE_TREE_SELECTION			(ctk_tree_selection_get_type ())
#define CTK_TREE_SELECTION(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_TREE_SELECTION, CtkTreeSelection))
#define CTK_TREE_SELECTION_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_TREE_SELECTION, CtkTreeSelectionClass))
#define CTK_IS_TREE_SELECTION(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_TREE_SELECTION))
#define CTK_IS_TREE_SELECTION_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_TREE_SELECTION))
#define CTK_TREE_SELECTION_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_TREE_SELECTION, CtkTreeSelectionClass))

typedef struct _CtkTreeSelectionPrivate      CtkTreeSelectionPrivate;

/**
 * CtkTreeSelectionFunc:
 * @selection: A #CtkTreeSelection
 * @model: A #CtkTreeModel being viewed
 * @path: The #CtkTreePath of the row in question
 * @path_currently_selected: %TRUE, if the path is currently selected
 * @data: (closure): user data
 *
 * A function used by ctk_tree_selection_set_select_function() to filter
 * whether or not a row may be selected.  It is called whenever a row's
 * state might change.  A return value of %TRUE indicates to @selection
 * that it is okay to change the selection.
 *
 * Returns: %TRUE, if the selection state of the row can be toggled
 */
typedef gboolean (* CtkTreeSelectionFunc)    (CtkTreeSelection  *selection,
					      CtkTreeModel      *model,
					      CtkTreePath       *path,
                                              gboolean           path_currently_selected,
					      gpointer           data);

/**
 * CtkTreeSelectionForeachFunc:
 * @model: The #CtkTreeModel being viewed
 * @path: The #CtkTreePath of a selected row
 * @iter: A #CtkTreeIter pointing to a selected row
 * @data: (closure): user data
 *
 * A function used by ctk_tree_selection_selected_foreach() to map all
 * selected rows.  It will be called on every selected row in the view.
 */
typedef void (* CtkTreeSelectionForeachFunc) (CtkTreeModel      *model,
					      CtkTreePath       *path,
					      CtkTreeIter       *iter,
					      gpointer           data);

struct _CtkTreeSelection
{
  /*< private >*/
  GObject parent;

  CtkTreeSelectionPrivate *priv;
};

/**
 * CtkTreeSelectionClass:
 * @parent_class: The parent class.
 * @changed: Signal emitted whenever the selection has (possibly) changed.
 */
struct _CtkTreeSelectionClass
{
  GObjectClass parent_class;

  /*< public >*/

  void (* changed) (CtkTreeSelection *selection);

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


CDK_AVAILABLE_IN_ALL
GType            ctk_tree_selection_get_type            (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_ALL
void             ctk_tree_selection_set_mode            (CtkTreeSelection            *selection,
							 CtkSelectionMode             type);
CDK_AVAILABLE_IN_ALL
CtkSelectionMode ctk_tree_selection_get_mode        (CtkTreeSelection            *selection);
CDK_AVAILABLE_IN_ALL
void             ctk_tree_selection_set_select_function (CtkTreeSelection            *selection,
							 CtkTreeSelectionFunc         func,
							 gpointer                     data,
							 GDestroyNotify               destroy);
CDK_AVAILABLE_IN_ALL
gpointer         ctk_tree_selection_get_user_data       (CtkTreeSelection            *selection);
CDK_AVAILABLE_IN_ALL
CtkTreeView*     ctk_tree_selection_get_tree_view       (CtkTreeSelection            *selection);

CDK_AVAILABLE_IN_ALL
CtkTreeSelectionFunc ctk_tree_selection_get_select_function (CtkTreeSelection        *selection);

/* Only meaningful if CTK_SELECTION_SINGLE or CTK_SELECTION_BROWSE is set */
/* Use selected_foreach or get_selected_rows for CTK_SELECTION_MULTIPLE */
CDK_AVAILABLE_IN_ALL
gboolean         ctk_tree_selection_get_selected        (CtkTreeSelection            *selection,
							 CtkTreeModel               **model,
							 CtkTreeIter                 *iter);
CDK_AVAILABLE_IN_ALL
GList *          ctk_tree_selection_get_selected_rows   (CtkTreeSelection            *selection,
                                                         CtkTreeModel               **model);
CDK_AVAILABLE_IN_ALL
gint             ctk_tree_selection_count_selected_rows (CtkTreeSelection            *selection);
CDK_AVAILABLE_IN_ALL
void             ctk_tree_selection_selected_foreach    (CtkTreeSelection            *selection,
							 CtkTreeSelectionForeachFunc  func,
							 gpointer                     data);
CDK_AVAILABLE_IN_ALL
void             ctk_tree_selection_select_path         (CtkTreeSelection            *selection,
							 CtkTreePath                 *path);
CDK_AVAILABLE_IN_ALL
void             ctk_tree_selection_unselect_path       (CtkTreeSelection            *selection,
							 CtkTreePath                 *path);
CDK_AVAILABLE_IN_ALL
void             ctk_tree_selection_select_iter         (CtkTreeSelection            *selection,
							 CtkTreeIter                 *iter);
CDK_AVAILABLE_IN_ALL
void             ctk_tree_selection_unselect_iter       (CtkTreeSelection            *selection,
							 CtkTreeIter                 *iter);
CDK_AVAILABLE_IN_ALL
gboolean         ctk_tree_selection_path_is_selected    (CtkTreeSelection            *selection,
							 CtkTreePath                 *path);
CDK_AVAILABLE_IN_ALL
gboolean         ctk_tree_selection_iter_is_selected    (CtkTreeSelection            *selection,
							 CtkTreeIter                 *iter);
CDK_AVAILABLE_IN_ALL
void             ctk_tree_selection_select_all          (CtkTreeSelection            *selection);
CDK_AVAILABLE_IN_ALL
void             ctk_tree_selection_unselect_all        (CtkTreeSelection            *selection);
CDK_AVAILABLE_IN_ALL
void             ctk_tree_selection_select_range        (CtkTreeSelection            *selection,
							 CtkTreePath                 *start_path,
							 CtkTreePath                 *end_path);
CDK_AVAILABLE_IN_ALL
void             ctk_tree_selection_unselect_range      (CtkTreeSelection            *selection,
                                                         CtkTreePath                 *start_path,
							 CtkTreePath                 *end_path);


G_END_DECLS

#endif /* __CTK_TREE_SELECTION_H__ */
