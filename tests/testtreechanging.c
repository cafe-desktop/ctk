/* testtreeview.c
 * Copyright (C) 2011 Red Hat, Inc
 * Author: Benjamin Otte <otte@gnome.org>
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

#include <ctk/ctk.h>

#define MIN_ROWS 50
#define MAX_ROWS 150

typedef void (* DoStuffFunc) (GtkTreeView *treeview);

static guint
count_children (GtkTreeModel *model,
                GtkTreeIter  *parent)
{
  GtkTreeIter iter;
  guint count = 0;
  gboolean valid;

  for (valid = ctk_tree_model_iter_children (model, &iter, parent);
       valid;
       valid = ctk_tree_model_iter_next (model, &iter))
    {
      count += count_children (model, &iter) + 1;
    }

  return count;
}

static void
set_rows (GtkTreeView *treeview, guint i)
{
  g_assert (i == count_children (ctk_tree_view_get_model (treeview), NULL));
  g_object_set_data (G_OBJECT (treeview), "rows", GUINT_TO_POINTER (i));
}

static guint
get_rows (GtkTreeView *treeview)
{
  return GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (treeview), "rows"));
}

static void
log_operation_for_path (GtkTreePath *path,
                        const char  *operation_name)
{
  char *path_string;

  path_string = path ? ctk_tree_path_to_string (path) : g_strdup ("");

  g_printerr ("%10s %s\n", operation_name, path_string);

  g_free (path_string);
}

static void
log_operation (GtkTreeModel *model,
               GtkTreeIter  *iter,
               const char   *operation_name)
{
  GtkTreePath *path;

  path = ctk_tree_model_get_path (model, iter);

  log_operation_for_path (path, operation_name);

  ctk_tree_path_free (path);
}

/* moves iter to the next iter in the model in the display order
 * inside a treeview. Returns FALSE if no more rows exist.
 */
static gboolean
tree_model_iter_step (GtkTreeModel *model,
                      GtkTreeIter *iter)
{
  GtkTreeIter tmp;
  
  if (ctk_tree_model_iter_children (model, &tmp, iter))
    {
      *iter = tmp;
      return TRUE;
    }

  do {
    tmp = *iter;

    if (ctk_tree_model_iter_next (model, iter))
      return TRUE;
    }
  while (ctk_tree_model_iter_parent (model, iter, &tmp));

  return FALSE;
}

/* NB: may include invisible iters (because they are collapsed) */
static void
tree_view_random_iter (GtkTreeView *treeview,
                       GtkTreeIter *iter)
{
  guint n_rows = get_rows (treeview);
  guint i = g_random_int_range (0, n_rows);
  GtkTreeModel *model;

  model = ctk_tree_view_get_model (treeview);
  
  if (!ctk_tree_model_get_iter_first (model, iter))
    return;

  while (i-- > 0)
    {
      if (!tree_model_iter_step (model, iter))
        {
          g_assert_not_reached ();
          return;
        }
    }

  return;
}

static void
delete (GtkTreeView *treeview)
{
  guint n_rows = get_rows (treeview);
  GtkTreeModel *model;
  GtkTreeIter iter;

  model = ctk_tree_view_get_model (treeview);
  
  tree_view_random_iter (treeview, &iter);

  n_rows -= count_children (model, &iter) + 1;
  log_operation (model, &iter, "remove");
  ctk_tree_store_remove (CTK_TREE_STORE (model), &iter);
  set_rows (treeview, n_rows);
}

static void
add_one (GtkTreeModel *model,
         GtkTreeIter *iter)
{
  guint n = ctk_tree_model_iter_n_children (model, iter);
  GtkTreeIter new_iter;
  static guint counter = 0;
  
  if (n > 0 && g_random_boolean ())
    {
      GtkTreeIter child;
      ctk_tree_model_iter_nth_child (model, &child, iter, g_random_int_range (0, n));
      add_one (model, &child);
      return;
    }

  ctk_tree_store_insert_with_values (CTK_TREE_STORE (model),
                                     &new_iter,
                                     iter,
                                     g_random_int_range (-1, n),
                                     0, ++counter,
                                     -1);
  log_operation (model, &new_iter, "add");
}

static void
add (GtkTreeView *treeview)
{
  GtkTreeModel *model;

  model = ctk_tree_view_get_model (treeview);
  add_one (model, NULL);

  set_rows (treeview, get_rows (treeview) + 1);
}

static void
add_or_delete (GtkTreeView *treeview)
{
  guint n_rows = get_rows (treeview);

  if (g_random_int_range (MIN_ROWS, MAX_ROWS) >= n_rows)
    add (treeview);
  else
    delete (treeview);
}

/* XXX: We only expand/collapse from the top and not randomly */
static void
expand (GtkTreeView *treeview)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  GtkTreePath *path;
  gboolean valid;

  model = ctk_tree_view_get_model (treeview);
  
  for (valid = ctk_tree_model_get_iter_first (model, &iter);
       valid;
       valid = tree_model_iter_step (model, &iter))
    {
      if (ctk_tree_model_iter_has_child (model, &iter))
        {
          path = ctk_tree_model_get_path (model, &iter);
          if (!ctk_tree_view_row_expanded (treeview, path))
            {
              log_operation (model, &iter, "expand");
              ctk_tree_view_expand_row (treeview, path, FALSE);
              ctk_tree_path_free (path);
              return;
            }
          ctk_tree_path_free (path);
        }
    }
}

static void
collapse (GtkTreeView *treeview)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  GtkTreePath *last, *path;
  gboolean valid;

  model = ctk_tree_view_get_model (treeview);
  last = NULL;
  
  for (valid = ctk_tree_model_get_iter_first (model, &iter);
       valid;
       valid = tree_model_iter_step (model, &iter))
    {
      path = ctk_tree_model_get_path (model, &iter);
      if (ctk_tree_view_row_expanded (treeview, path))
        {
          if (last)
            ctk_tree_path_free (last);
          last = path;
        }
      else
        ctk_tree_path_free (path);
    }

  if (last)
    {
      log_operation_for_path (last, "collapse");
      ctk_tree_view_collapse_row (treeview, last);
      ctk_tree_path_free (last);
    }
}

static void
select_ (GtkTreeView *treeview)
{
  GtkTreeIter iter;

  tree_view_random_iter (treeview, &iter);

  log_operation (ctk_tree_view_get_model (treeview), &iter, "select");
  ctk_tree_selection_select_iter (ctk_tree_view_get_selection (treeview),
                                  &iter);
}

static void
unselect (GtkTreeView *treeview)
{
  GtkTreeIter iter;

  tree_view_random_iter (treeview, &iter);

  log_operation (ctk_tree_view_get_model (treeview), &iter, "unselect");
  ctk_tree_selection_unselect_iter (ctk_tree_view_get_selection (treeview),
                                    &iter);
}

static void
reset_model (GtkTreeView *treeview)
{
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GList *list, *selected;
  GtkTreePath *cursor;
  
  selection = ctk_tree_view_get_selection (treeview);
  model = g_object_ref (ctk_tree_view_get_model (treeview));

  log_operation_for_path (NULL, "reset");

  selected = ctk_tree_selection_get_selected_rows (selection, NULL);
  ctk_tree_view_get_cursor (treeview, &cursor, NULL);

  ctk_tree_view_set_model (treeview, NULL);
  ctk_tree_view_set_model (treeview, model);

  if (cursor)
    {
      ctk_tree_view_set_cursor (treeview, cursor, NULL, FALSE);
      ctk_tree_path_free (cursor);
    }
  for (list = selected; list; list = list->next)
    {
      ctk_tree_selection_select_path (selection, list->data);
    }
  g_list_free_full (selected, (GDestroyNotify) ctk_tree_path_free);

  g_object_unref (model);
}

/* sanity checks */

static void
assert_row_reference_is_path (GtkTreeRowReference *ref,
                              GtkTreePath *path)
{
  GtkTreePath *expected;

  if (ref == NULL)
    {
      g_assert (path == NULL);
      return;
    }

  g_assert (path != NULL);
  g_assert (ctk_tree_row_reference_valid (ref));

  expected = ctk_tree_row_reference_get_path (ref);
  g_assert (expected != NULL);
  g_assert (ctk_tree_path_compare (expected, path) == 0);
  ctk_tree_path_free (expected);
}

static void
check_cursor (GtkTreeView *treeview)
{
  GtkTreeRowReference *ref = g_object_get_data (G_OBJECT (treeview), "cursor");
  GtkTreePath *cursor;

  ctk_tree_view_get_cursor (treeview, &cursor, NULL);
  assert_row_reference_is_path (ref, cursor);

  if (cursor)
    ctk_tree_path_free (cursor);
}

static void
check_selection_item (GtkTreeModel *model,
                      GtkTreePath  *path,
                      GtkTreeIter  *iter,
                      gpointer      listp)
{
  GList **list = listp;

  g_assert (*list);
  assert_row_reference_is_path ((*list)->data, path);
  *list = (*list)->next;
}

static void
check_selection (GtkTreeView *treeview)
{
  GList *selection = g_object_get_data (G_OBJECT (treeview), "selection");

  ctk_tree_selection_selected_foreach (ctk_tree_view_get_selection (treeview),
                                       check_selection_item,
                                       &selection);
}

static void
check_sanity (GtkTreeView *treeview)
{
  check_cursor (treeview);
  check_selection (treeview);
}

static gboolean
dance (gpointer treeview)
{
  static const DoStuffFunc funcs[] = {
    add_or_delete,
    add_or_delete,
    expand,
    collapse,
    select_,
    unselect,
    reset_model
  };
  guint i;

  i = g_random_int_range (0, G_N_ELEMENTS(funcs));

  funcs[i] (treeview);

  check_sanity (treeview);

  return G_SOURCE_CONTINUE;
}

static void
cursor_changed_cb (GtkTreeView *treeview,
                   gpointer     unused)
{
  GtkTreePath *path;
  GtkTreeRowReference *ref;

  ctk_tree_view_get_cursor (treeview, &path, NULL);
  if (path)
    {
      ref = ctk_tree_row_reference_new (ctk_tree_view_get_model (treeview),
                                        path);
      ctk_tree_path_free (path);
    }
  else
    ref = NULL;
  g_object_set_data_full (G_OBJECT (treeview), "cursor", ref, (GDestroyNotify) ctk_tree_row_reference_free);
}

static void
selection_list_free (gpointer list)
{
  g_list_free_full (list, (GDestroyNotify) ctk_tree_row_reference_free);
}

static void
selection_changed_cb (GtkTreeSelection *tree_selection,
                      gpointer          unused)
{
  GList *selected, *list;
  GtkTreeModel *model;

  selected = ctk_tree_selection_get_selected_rows (tree_selection, &model);

  for (list = selected; list; list = list->next)
    {
      GtkTreePath *path = list->data;

      list->data = ctk_tree_row_reference_new (model, path);
      ctk_tree_path_free (path);
    }

  g_object_set_data_full (G_OBJECT (ctk_tree_selection_get_tree_view (tree_selection)),
                          "selection",
                          selected,
                          selection_list_free);
}

static void
setup_sanity_checks (GtkTreeView *treeview)
{
  g_signal_connect (treeview, "cursor-changed", G_CALLBACK (cursor_changed_cb), NULL);
  cursor_changed_cb (treeview, NULL);
  g_signal_connect (ctk_tree_view_get_selection (treeview), "changed", G_CALLBACK (selection_changed_cb), NULL);
  selection_changed_cb (ctk_tree_view_get_selection (treeview), NULL);
}

int
main (int    argc,
      char **argv)
{
  GtkWidget *window;
  GtkWidget *sw;
  GtkWidget *treeview;
  GtkTreeModel *model;
  guint i;
  
  ctk_init (&argc, &argv);

  if (g_getenv ("RTL"))
    ctk_widget_set_default_direction (CTK_TEXT_DIR_RTL);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "destroy", G_CALLBACK (ctk_main_quit), NULL);
  ctk_window_set_default_size (CTK_WINDOW (window), 430, 400);

  sw = ctk_scrolled_window_new (NULL, NULL);
  ctk_widget_set_hexpand (sw, TRUE);
  ctk_widget_set_vexpand (sw, TRUE);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (sw),
                                  CTK_POLICY_AUTOMATIC,
                                  CTK_POLICY_AUTOMATIC);
  ctk_container_add (CTK_CONTAINER (window), sw);

  model = CTK_TREE_MODEL (ctk_tree_store_new (1, G_TYPE_UINT));
  treeview = ctk_tree_view_new_with_model (model);
  g_object_unref (model);
  setup_sanity_checks (CTK_TREE_VIEW (treeview));
  ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (treeview),
                                               0,
                                               "Counter",
                                               ctk_cell_renderer_text_new (),
                                               "text", 0,
                                               NULL);
  for (i = 0; i < (MIN_ROWS + MAX_ROWS) / 2; i++)
    add (CTK_TREE_VIEW (treeview));
  ctk_container_add (CTK_CONTAINER (sw), treeview);

  ctk_widget_show_all (window);

  g_idle_add (dance, treeview);
  
  ctk_main ();

  return 0;
}

