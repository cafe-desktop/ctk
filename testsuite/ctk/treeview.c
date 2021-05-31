/* Basic GtkTreeView unit tests.
 * Copyright (C) 2009  Kristian Rietveld  <kris@ctk.org>
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

#include <ctk/ctk.h>

static void
test_bug_546005 (void)
{
  GtkTreeIter iter;
  GtkTreePath *path;
  GtkTreePath *cursor_path;
  GtkListStore *list_store;
  GtkWidget *view;

  g_test_bug ("546005");

  /* Tests provided by Bjorn Lindqvist, Paul Pogonyshev */
  view = ctk_tree_view_new ();

  /* Invalid path on tree view without model */
  path = ctk_tree_path_new_from_indices (1, -1);
  ctk_tree_view_set_cursor (CTK_TREE_VIEW (view), path,
                            NULL, FALSE);
  ctk_tree_path_free (path);

  list_store = ctk_list_store_new (1, G_TYPE_STRING);
  ctk_tree_view_set_model (CTK_TREE_VIEW (view),
                           CTK_TREE_MODEL (list_store));

  /* Invalid path on tree view with empty model */
  path = ctk_tree_path_new_from_indices (1, -1);
  ctk_tree_view_set_cursor (CTK_TREE_VIEW (view), path,
                            NULL, FALSE);
  ctk_tree_path_free (path);

  /* Valid path */
  ctk_list_store_insert_with_values (list_store, &iter, 0,
                                     0, "hi",
                                     -1);

  path = ctk_tree_path_new_from_indices (0, -1);
  ctk_tree_view_set_cursor (CTK_TREE_VIEW (view), path,
                            NULL, FALSE);

  ctk_tree_view_get_cursor (CTK_TREE_VIEW (view), &cursor_path, NULL);
  //ctk_assert_cmptreepath (cursor_path, ==, path);

  ctk_tree_path_free (path);
  ctk_tree_path_free (cursor_path);

  /* Invalid path on tree view with model */
  path = ctk_tree_path_new_from_indices (1, -1);
  ctk_tree_view_set_cursor (CTK_TREE_VIEW (view), path,
                            NULL, FALSE);
  ctk_tree_path_free (path);

  ctk_widget_destroy (view);
}

static void
test_bug_539377 (void)
{
  GtkWidget *view;
  GtkTreePath *path;
  GtkListStore *list_store;

  g_test_bug ("539377");

  /* Test provided by Bjorn Lindqvist */

  /* Non-realized view, no model */
  view = ctk_tree_view_new ();
  g_assert (ctk_tree_view_get_path_at_pos (CTK_TREE_VIEW (view), 10, 10, &path,
                                           NULL, NULL, NULL) == FALSE);
  g_assert (ctk_tree_view_get_dest_row_at_pos (CTK_TREE_VIEW (view), 10, 10,
                                               &path, NULL) == FALSE);

  /* Non-realized view, with model */
  list_store = ctk_list_store_new (1, G_TYPE_STRING);
  ctk_tree_view_set_model (CTK_TREE_VIEW (view),
                           CTK_TREE_MODEL (list_store));

  g_assert (ctk_tree_view_get_path_at_pos (CTK_TREE_VIEW (view), 10, 10, &path,
                                           NULL, NULL, NULL) == FALSE);
  g_assert (ctk_tree_view_get_dest_row_at_pos (CTK_TREE_VIEW (view), 10, 10,
                                               &path, NULL) == FALSE);

  ctk_widget_destroy (view);
}

static void
test_select_collapsed_row (void)
{
  GtkTreeIter child, parent;
  GtkTreePath *path;
  GtkTreeStore *tree_store;
  GtkTreeSelection *selection;
  GtkWidget *view;

  /* Reported by Michael Natterer */
  tree_store = ctk_tree_store_new (1, G_TYPE_STRING);
  view = ctk_tree_view_new_with_model (CTK_TREE_MODEL (tree_store));

  ctk_tree_store_insert_with_values (tree_store, &parent, NULL, 0,
                                     0, "Parent",
                                     -1);

  ctk_tree_store_insert_with_values (tree_store, &child, &parent, 0,
                                     0, "Child",
                                     -1);
  ctk_tree_store_insert_with_values (tree_store, &child, &parent, 0,
                                     0, "Child",
                                     -1);


  /* Try to select a child path. */
  path = ctk_tree_path_new_from_indices (0, 1, -1);
  ctk_tree_view_set_cursor (CTK_TREE_VIEW (view), path, NULL, FALSE);

  selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (view));

  /* Check that the parent is not selected. */
  ctk_tree_path_up (path);
  g_return_if_fail (ctk_tree_selection_path_is_selected (selection, path) == FALSE);

  /* Nothing should be selected at this point. */
  g_return_if_fail (ctk_tree_selection_count_selected_rows (selection) == 0);

  /* Check that selection really still works. */
  ctk_tree_view_set_cursor (CTK_TREE_VIEW (view), path, NULL, FALSE);
  g_return_if_fail (ctk_tree_selection_path_is_selected (selection, path) == TRUE);
  g_return_if_fail (ctk_tree_selection_count_selected_rows (selection) == 1);

  /* Expand and select child node now. */
  ctk_tree_path_append_index (path, 1);
  ctk_tree_view_expand_all (CTK_TREE_VIEW (view));

  ctk_tree_view_set_cursor (CTK_TREE_VIEW (view), path, NULL, FALSE);
  g_return_if_fail (ctk_tree_selection_path_is_selected (selection, path) == TRUE);
  g_return_if_fail (ctk_tree_selection_count_selected_rows (selection) == 1);

  ctk_tree_path_free (path);

  ctk_widget_destroy (view);
}

static gboolean
test_row_separator_height_func (GtkTreeModel *model,
                                GtkTreeIter  *iter,
                                gpointer      data)
{
  gboolean ret = FALSE;
  GtkTreePath *path;

  path = ctk_tree_model_get_path (model, iter);
  if (ctk_tree_path_get_indices (path)[0] == 2)
    ret = TRUE;
  ctk_tree_path_free (path);

  return ret;
}

static void
test_row_separator_height (void)
{
  int focus_pad, separator_height, height;
  gboolean wide_separators;
  GtkTreeIter iter;
  GtkTreePath *path;
  GtkListStore *store;
  GtkWidget *window;
  GtkWidget *tree_view;
  GdkRectangle rect = { 0, }, cell_rect = { 0, };

  store = ctk_list_store_new (1, G_TYPE_STRING);
  ctk_list_store_insert_with_values (store, &iter, 0, 0, "Row content", -1);
  ctk_list_store_insert_with_values (store, &iter, 1, 0, "Row content", -1);
  ctk_list_store_insert_with_values (store, &iter, 2, 0, "Row content", -1);
  ctk_list_store_insert_with_values (store, &iter, 3, 0, "Row content", -1);
  ctk_list_store_insert_with_values (store, &iter, 4, 0, "Row content", -1);

  window = ctk_offscreen_window_new ();

  tree_view = ctk_tree_view_new_with_model (CTK_TREE_MODEL (store));
  ctk_tree_view_set_row_separator_func (CTK_TREE_VIEW (tree_view),
                                        test_row_separator_height_func,
                                        NULL,
                                        NULL);

  ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (tree_view),
                                               0,
                                               "Test",
                                               ctk_cell_renderer_text_new (),
                                               "text", 0,
                                               NULL);

  ctk_container_add (CTK_CONTAINER (window), tree_view);
  ctk_widget_show_all (window);

  ctk_test_widget_wait_for_draw (window);

  path = ctk_tree_path_new_from_indices (2, -1);
  ctk_tree_view_get_background_area (CTK_TREE_VIEW (tree_view),
                                     path, NULL, &rect);
  ctk_tree_view_get_cell_area (CTK_TREE_VIEW (tree_view),
                               path, NULL, &cell_rect);
  ctk_tree_path_free (path);

  ctk_widget_style_get (tree_view,
                        "focus-padding", &focus_pad,
                        "wide-separators", &wide_separators,
                        "separator-height", &separator_height,
                        NULL);

  if (wide_separators)
    height = separator_height;
  else
    height = 2;

  g_assert_cmpint (rect.height, ==, height);
  g_assert_cmpint (cell_rect.height, ==, height);

  ctk_widget_destroy (tree_view);
}

static void
test_selection_count (void)
{
  GtkTreePath *path;
  GtkListStore *list_store;
  GtkTreeSelection *selection;
  GtkWidget *view;

  g_test_bug ("702957");

  list_store = ctk_list_store_new (1, G_TYPE_STRING);
  view = ctk_tree_view_new_with_model (CTK_TREE_MODEL (list_store));

  ctk_list_store_insert_with_values (list_store, NULL, 0, 0, "One", -1);
  ctk_list_store_insert_with_values (list_store, NULL, 1, 0, "Two", -1);
  ctk_list_store_insert_with_values (list_store, NULL, 2, 0, "Tree", -1);

  selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (view));
  ctk_tree_selection_set_mode (selection, CTK_SELECTION_MULTIPLE);

  g_assert_cmpint (ctk_tree_selection_count_selected_rows (selection), ==, 0);

  path = ctk_tree_path_new_from_indices (0, -1);
  ctk_tree_selection_select_path (selection, path);
  ctk_tree_path_free (path);

  g_assert_cmpint (ctk_tree_selection_count_selected_rows (selection), ==, 1);

  path = ctk_tree_path_new_from_indices (2, -1);
  ctk_tree_selection_select_path (selection, path);
  ctk_tree_path_free (path);

  g_assert_cmpint (ctk_tree_selection_count_selected_rows (selection), ==, 2);

  path = ctk_tree_path_new_from_indices (2, -1);
  ctk_tree_selection_select_path (selection, path);
  ctk_tree_path_free (path);

  g_assert_cmpint (ctk_tree_selection_count_selected_rows (selection), ==, 2);

  path = ctk_tree_path_new_from_indices (1, -1);
  ctk_tree_selection_select_path (selection, path);
  ctk_tree_path_free (path);

  g_assert_cmpint (ctk_tree_selection_count_selected_rows (selection), ==, 3);

  ctk_tree_selection_unselect_all (selection);

  g_assert_cmpint (ctk_tree_selection_count_selected_rows (selection), ==, 0);

  ctk_widget_destroy (view);
}

static void
abort_cb (GtkTreeModel *model,
          GtkTreePath  *path,
          GtkTreeIter  *iter,
          gpointer      data)
{
  g_assert_not_reached ();
}

static void
test_selection_empty (void)
{
  GtkTreePath *path;
  GtkListStore *list_store;
  GtkTreeSelection *selection;
  GtkWidget *view;
  GtkTreeIter iter;

  g_test_bug ("712760");

  list_store = ctk_list_store_new (1, G_TYPE_STRING);
  view = ctk_tree_view_new_with_model (CTK_TREE_MODEL (list_store));
  selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (view));

  g_assert_false (ctk_tree_selection_get_selected (selection, NULL, &iter));
  ctk_tree_selection_selected_foreach (selection, abort_cb, NULL);
  g_assert_null (ctk_tree_selection_get_selected_rows (selection, NULL));
  g_assert_cmpint (ctk_tree_selection_count_selected_rows (selection), ==, 0);

  path = ctk_tree_path_new_from_indices (0, -1);

  ctk_tree_selection_select_path (selection, path);
  ctk_tree_selection_unselect_path (selection, path);
  g_assert_false (ctk_tree_selection_path_is_selected (selection, path));

  ctk_tree_selection_set_mode (selection, CTK_SELECTION_MULTIPLE);

  ctk_tree_selection_select_all (selection);
  g_assert_cmpint (ctk_tree_selection_count_selected_rows (selection), ==, 0);

  ctk_tree_selection_unselect_all (selection);
  g_assert_cmpint (ctk_tree_selection_count_selected_rows (selection), ==, 0);

  ctk_tree_selection_select_range (selection, path, path);
  g_assert_cmpint (ctk_tree_selection_count_selected_rows (selection), ==, 0);

  ctk_tree_selection_unselect_range (selection, path, path);
  g_assert_cmpint (ctk_tree_selection_count_selected_rows (selection), ==, 0);

  ctk_tree_path_free (path);

  ctk_widget_destroy (view);
}

int
main (int    argc,
      char **argv)
{
  ctk_test_init (&argc, &argv, NULL);
  g_test_bug_base ("http://bugzilla.gnome.org/");

  g_test_add_func ("/TreeView/cursor/bug-546005", test_bug_546005);
  g_test_add_func ("/TreeView/cursor/bug-539377", test_bug_539377);
  g_test_add_func ("/TreeView/cursor/select-collapsed_row",
                   test_select_collapsed_row);
  g_test_add_func ("/TreeView/sizing/row-separator-height",
                   test_row_separator_height);
  g_test_add_func ("/TreeView/selection/count", test_selection_count);
  g_test_add_func ("/TreeView/selection/empty", test_selection_empty);

  return g_test_run ();
}
