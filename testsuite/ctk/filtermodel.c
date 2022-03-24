/* Extensive CtkTreeModelFilter tests.
 * Copyright (C) 2009,2011  Kristian Rietveld  <kris@gtk.org>
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
#include <string.h>

#include "treemodel.h"
#include "ctktreemodelrefcount.h"

/* Left to do:
 *   - Proper coverage checking to see if the unit tests cover
 *     all possible cases.
 *   - Check if the iterator stamp is incremented at the correct times.
 *
 * For more thorough testing:
 *   - Test with randomized models.
 *   - Extensively test a filter model wrapping a sort model,
 *     or a sort model wrapping a filter model by:
 *       # Checking structure.
 *       # Checking for correct signals emissions.
 *       # Checking correct reference counting.
 *       # Tests should be done with the sort and filter model
 *         in various filtering and sorting states.
 */


/*
 * Model creation
 */

#define LEVEL_LENGTH 5

static void
create_tree_store_set_values (CtkTreeStore *store,
                              CtkTreeIter  *iter,
                              gboolean      visible)
{
  CtkTreePath *path;
  gchar *path_string;

  path = ctk_tree_model_get_path (CTK_TREE_MODEL (store), iter);
  path_string = ctk_tree_path_to_string (path);

  ctk_tree_store_set (store, iter,
                      0, path_string,
                      1, visible,
                      -1);

  ctk_tree_path_free (path);
  g_free (path_string);
}

static void
create_tree_store_recurse (int           depth,
                           CtkTreeStore *store,
                           CtkTreeIter  *parent,
                           gboolean      visible)
{
  int i;

  for (i = 0; i < LEVEL_LENGTH; i++)
    {
      CtkTreeIter iter;

      ctk_tree_store_insert (store, &iter, parent, i);
      create_tree_store_set_values (store, &iter, visible);

      if (depth > 0)
        create_tree_store_recurse (depth - 1, store, &iter, visible);
    }
}

static CtkTreeStore *
create_tree_store (int      depth,
                   gboolean visible)
{
  CtkTreeStore *store;

  store = ctk_tree_store_new (2, G_TYPE_STRING, G_TYPE_BOOLEAN);

  create_tree_store_recurse (depth, store, NULL, visible);

  return store;
}

/*
 * Fixture
 */

typedef struct
{
  CtkWidget *tree_view;

  CtkTreeStore *store;
  CtkTreeModelFilter *filter;

  SignalMonitor *monitor;

  guint block_signals : 1;
} FilterTest;


static void
filter_test_store_signal (FilterTest *fixture)
{
  if (fixture->block_signals)
    g_signal_stop_emission_by_name (fixture->store, "row-changed");
}


static void
filter_test_setup_generic (FilterTest    *fixture,
                           gconstpointer  test_data,
                           int            depth,
                           gboolean       empty,
                           gboolean       unfiltered)
{
  const CtkTreePath *vroot = test_data;
  CtkTreeModel *filter;

  fixture->store = create_tree_store (depth, !empty);

  g_signal_connect_swapped (fixture->store, "row-changed",
                            G_CALLBACK (filter_test_store_signal), fixture);

  /* Please forgive me for casting const away. */
  filter = ctk_tree_model_filter_new (CTK_TREE_MODEL (fixture->store),
                                      (CtkTreePath *)vroot);
  fixture->filter = CTK_TREE_MODEL_FILTER (filter);

  if (!unfiltered)
    ctk_tree_model_filter_set_visible_column (fixture->filter, 1);

  /* We need a tree view that's listening to get ref counting from that
   * side.
   */
  fixture->tree_view = ctk_tree_view_new_with_model (filter);

  fixture->monitor = signal_monitor_new (filter);
}

static void
filter_test_setup_expand_root (FilterTest *fixture)
{
  int i;
  CtkTreePath *path;

  path = ctk_tree_path_new_from_indices (0, -1);

  for (i = 0; i < LEVEL_LENGTH; i++)
    {
      ctk_tree_view_expand_row (CTK_TREE_VIEW (fixture->tree_view),
                                path, FALSE);
      ctk_tree_path_next (path);
    }
  ctk_tree_path_free (path);
}

static void
filter_test_setup (FilterTest    *fixture,
                   gconstpointer  test_data)
{
  filter_test_setup_generic (fixture, test_data, 3, FALSE, FALSE);
}

static void
filter_test_setup_empty (FilterTest    *fixture,
                         gconstpointer  test_data)
{
  filter_test_setup_generic (fixture, test_data, 3, TRUE, FALSE);
}

static void
filter_test_setup_unfiltered (FilterTest    *fixture,
                              gconstpointer  test_data)
{
  filter_test_setup_generic (fixture, test_data, 3, FALSE, TRUE);
}

static void
filter_test_setup_unfiltered_root_expanded (FilterTest    *fixture,
                                            gconstpointer  test_data)
{
  filter_test_setup_unfiltered (fixture, test_data);
  filter_test_setup_expand_root (fixture);
}

static void
filter_test_setup_empty_unfiltered (FilterTest    *fixture,
                                    gconstpointer  test_data)
{
  filter_test_setup_generic (fixture, test_data, 3, TRUE, TRUE);
}

static void
filter_test_setup_empty_unfiltered_root_expanded (FilterTest    *fixture,
                                                  gconstpointer  test_data)
{
  filter_test_setup_empty_unfiltered (fixture, test_data);
  filter_test_setup_expand_root (fixture);
}

static CtkTreePath *
strip_virtual_root (CtkTreePath *path,
                    CtkTreePath *root_path)
{
  CtkTreePath *real_path;

  if (root_path)
    {
      int j;
      int depth = ctk_tree_path_get_depth (path);
      int root_depth = ctk_tree_path_get_depth (root_path);

      real_path = ctk_tree_path_new ();

      for (j = 0; j < depth - root_depth; j++)
        ctk_tree_path_append_index (real_path,
                                    ctk_tree_path_get_indices (path)[root_depth + j]);
    }
  else
    real_path = ctk_tree_path_copy (path);

  return real_path;
}

static int
count_visible (FilterTest  *fixture,
               CtkTreePath *store_path)
{
  int i;
  int n_visible = 0;
  CtkTreeIter iter;

  ctk_tree_model_get_iter (CTK_TREE_MODEL (fixture->store),
                           &iter, store_path);

  for (i = 0; i < LEVEL_LENGTH; i++)
    {
      gboolean visible;

      ctk_tree_model_get (CTK_TREE_MODEL (fixture->store), &iter,
                          1, &visible,
                          -1);

      if (visible)
        n_visible++;
    }

  return n_visible;
}

static void
filter_test_append_refilter_signals_recurse (FilterTest  *fixture,
                                             CtkTreePath *store_path,
                                             CtkTreePath *filter_path,
                                             int          depth,
                                             CtkTreePath *root_path)
{
  int i;
  int rows_deleted = 0;
  CtkTreeIter iter;

  ctk_tree_path_down (store_path);
  ctk_tree_path_down (filter_path);

  ctk_tree_model_get_iter (CTK_TREE_MODEL (fixture->store),
                           &iter, store_path);

  for (i = 0; i < LEVEL_LENGTH; i++)
    {
      gboolean visible;
      CtkTreePath *real_path;

      ctk_tree_model_get (CTK_TREE_MODEL (fixture->store), &iter,
                          1, &visible,
                          -1);

      if (root_path &&
          (!ctk_tree_path_is_descendant (store_path, root_path)
           || !ctk_tree_path_compare (store_path, root_path)))
        {
          if (!ctk_tree_path_compare (store_path, root_path))
            {
              if (depth > 1
                  && ctk_tree_model_iter_has_child (CTK_TREE_MODEL (fixture->store),
                                                    &iter))
                {
                  CtkTreePath *store_copy;
                  CtkTreePath *filter_copy;

                  store_copy = ctk_tree_path_copy (store_path);
                  filter_copy = ctk_tree_path_copy (filter_path);
                  filter_test_append_refilter_signals_recurse (fixture,
                                                               store_copy,
                                                               filter_copy,
                                                               depth - 1,
                                                               root_path);
                  ctk_tree_path_free (store_copy);
                  ctk_tree_path_free (filter_copy);
                }
            }

          ctk_tree_path_next (store_path);
          ctk_tree_model_iter_next (CTK_TREE_MODEL (fixture->store), &iter);

          if (visible)
            ctk_tree_path_next (filter_path);

          continue;
        }

      real_path = strip_virtual_root (filter_path, root_path);

      if (visible)
        {
          /* This row will be inserted */
          signal_monitor_append_signal_path (fixture->monitor, ROW_CHANGED,
                                             real_path);

          if (ctk_tree_model_iter_has_child (CTK_TREE_MODEL (fixture->store),
                                             &iter))
            {
              signal_monitor_append_signal_path (fixture->monitor,
                                                 ROW_HAS_CHILD_TOGGLED,
                                                 real_path);

              if (depth > 1)
                {
                  CtkTreePath *store_copy;
                  CtkTreePath *filter_copy;

                  store_copy = ctk_tree_path_copy (store_path);
                  filter_copy = ctk_tree_path_copy (filter_path);
                  filter_test_append_refilter_signals_recurse (fixture,
                                                               store_copy,
                                                               filter_copy,
                                                               depth - 1,
                                                               root_path);
                  ctk_tree_path_free (store_copy);
                  ctk_tree_path_free (filter_copy);
                }
              else if (depth == 1)
                {
                  CtkTreePath *tmp_path;

                  /* If all child rows are invisible, then the last row to
                   * become invisible will emit row-has-child-toggled on the
                   * parent.
                   */

                  tmp_path = ctk_tree_path_copy (store_path);
                  ctk_tree_path_append_index (tmp_path, 0);

                  if (count_visible (fixture, tmp_path) == 0)
                    signal_monitor_append_signal_path (fixture->monitor,
                                                       ROW_HAS_CHILD_TOGGLED,
                                                       real_path);

                  ctk_tree_path_free (tmp_path);
                }
            }

          ctk_tree_path_next (filter_path);
        }
      else
        {
          /* This row will be deleted */
          rows_deleted++;
          signal_monitor_append_signal_path (fixture->monitor, ROW_DELETED,
                                             real_path);
        }

      ctk_tree_path_free (real_path);

      ctk_tree_path_next (store_path);
      ctk_tree_model_iter_next (CTK_TREE_MODEL (fixture->store), &iter);
    }

  if (rows_deleted == LEVEL_LENGTH
      && ctk_tree_path_get_depth (filter_path) > 1)
    {
      CtkTreePath *real_path;

      ctk_tree_path_up (store_path);
      ctk_tree_path_up (filter_path);

      /* A row-has-child-toggled will be emitted on the parent */
      if (!root_path
          || (root_path
              && ctk_tree_path_is_descendant (store_path, root_path)
              && ctk_tree_path_compare (store_path, root_path)))
        {
          real_path = strip_virtual_root (filter_path, root_path);
          signal_monitor_append_signal_path (fixture->monitor,
                                             ROW_HAS_CHILD_TOGGLED,
                                             real_path);

          ctk_tree_path_free (real_path);
        }
    }
}

static void
filter_test_append_refilter_signals (FilterTest *fixture,
                                     int         depth)
{
  /* A special function that walks the tree store like the
   * model validation functions below.
   */
  CtkTreePath *path;
  CtkTreePath *filter_path;

  path = ctk_tree_path_new ();
  filter_path = ctk_tree_path_new ();
  filter_test_append_refilter_signals_recurse (fixture,
                                               path,
                                               filter_path,
                                               depth,
                                               NULL);
  ctk_tree_path_free (path);
  ctk_tree_path_free (filter_path);
}

static void
filter_test_append_refilter_signals_with_vroot (FilterTest  *fixture,
                                                int          depth,
                                                CtkTreePath *root_path)
{
  /* A special function that walks the tree store like the
   * model validation functions below.
   */
  CtkTreePath *path;
  CtkTreePath *filter_path;

  path = ctk_tree_path_new ();
  filter_path = ctk_tree_path_new ();
  filter_test_append_refilter_signals_recurse (fixture,
                                               path,
                                               filter_path,
                                               depth,
                                               root_path);
  ctk_tree_path_free (path);
  ctk_tree_path_free (filter_path);
}

static void
filter_test_enable_filter (FilterTest *fixture)
{
  ctk_tree_model_filter_set_visible_column (fixture->filter, 1);
  ctk_tree_model_filter_refilter (fixture->filter);
}

static void
filter_test_block_signals (FilterTest *fixture)
{
  fixture->block_signals = TRUE;
}

static void
filter_test_unblock_signals (FilterTest *fixture)
{
  fixture->block_signals = FALSE;
}

static void
filter_test_teardown (FilterTest    *fixture,
                      gconstpointer  test_data)
{
  signal_monitor_free (fixture->monitor);

  ctk_widget_destroy (fixture->tree_view);

  g_object_unref (fixture->filter);
  g_object_unref (fixture->store);
}

/*
 * Model structure validation
 */

static void
check_filter_model_recurse (FilterTest  *fixture,
                            CtkTreePath *store_parent_path,
                            CtkTreePath *filter_parent_path)
{
  int i;
  CtkTreeIter store_iter;
  CtkTreeIter filter_iter;
  gboolean store_has_next, filter_has_next;

  ctk_tree_path_down (store_parent_path);
  ctk_tree_path_down (filter_parent_path);

  store_has_next = ctk_tree_model_get_iter (CTK_TREE_MODEL (fixture->store),
                                            &store_iter, store_parent_path);
  filter_has_next = ctk_tree_model_get_iter (CTK_TREE_MODEL (fixture->filter),
                                             &filter_iter, filter_parent_path);

  for (i = 0; i < LEVEL_LENGTH; i++)
    {
      gboolean visible;

      g_return_if_fail (store_has_next == TRUE);

      ctk_tree_model_get (CTK_TREE_MODEL (fixture->store),
                          &store_iter,
                          1, &visible,
                          -1);

      if (visible)
        {
          CtkTreePath *tmp;
          gchar *filter_str, *store_str;

          g_return_if_fail (filter_has_next == TRUE);

          /* Verify path */
          tmp = ctk_tree_model_get_path (CTK_TREE_MODEL (fixture->filter),
                                         &filter_iter);
          g_return_if_fail (ctk_tree_path_compare (tmp, filter_parent_path) == 0);

          /* Verify model content */
          ctk_tree_model_get (CTK_TREE_MODEL (fixture->store),
                              &store_iter,
                              0, &store_str,
                              -1);
          ctk_tree_model_get (CTK_TREE_MODEL (fixture->filter),
                              &filter_iter,
                              0, &filter_str,
                              -1);

          g_return_if_fail (g_strcmp0 (store_str, filter_str) == 0);

          g_free (store_str);
          g_free (filter_str);

          if (ctk_tree_model_iter_has_child (CTK_TREE_MODEL (fixture->filter),
                                             &filter_iter))
            {
              g_return_if_fail (ctk_tree_model_iter_has_child (CTK_TREE_MODEL (fixture->store), &store_iter));

              check_filter_model_recurse (fixture,
                                          ctk_tree_path_copy (store_parent_path),
                                          tmp);
            }
          else
            /* Only when we do not recurse we need to free tmp */
            ctk_tree_path_free (tmp);

          ctk_tree_path_next (filter_parent_path);
          filter_has_next = ctk_tree_model_iter_next (CTK_TREE_MODEL (fixture->filter), &filter_iter);
        }

      ctk_tree_path_next (store_parent_path);
      store_has_next = ctk_tree_model_iter_next (CTK_TREE_MODEL (fixture->store), &store_iter);
    }

  /* Both models should have no more content! */
  g_return_if_fail (store_has_next == FALSE);
  g_return_if_fail (filter_has_next == FALSE);

  ctk_tree_path_free (store_parent_path);
  ctk_tree_path_free (filter_parent_path);
}

static void
check_filter_model (FilterTest *fixture)
{
  CtkTreePath *path;

  if (fixture->monitor)
    signal_monitor_assert_is_empty (fixture->monitor);

  path = ctk_tree_path_new ();

  check_filter_model_recurse (fixture, path, ctk_tree_path_copy (path));
}

static void
check_filter_model_with_root (FilterTest  *fixture,
                              CtkTreePath *path)
{
  if (fixture->monitor)
    signal_monitor_assert_is_empty (fixture->monitor);

  check_filter_model_recurse (fixture,
                              ctk_tree_path_copy (path),
                              ctk_tree_path_new ());
}

/* Helpers */

static void
check_level_length (CtkTreeModelFilter *filter,
                    const gchar        *level,
                    const int           expected_length)
{
  if (!level)
    {
      int model_length;

      model_length = ctk_tree_model_iter_n_children (CTK_TREE_MODEL (filter), NULL);
      g_assert_cmpint (model_length, ==, expected_length);
    }
  else
    {
      int model_length;
      gboolean retrieved_iter = FALSE;
      CtkTreeIter iter;

      retrieved_iter = ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (filter),
                                                            &iter, level);
      g_return_if_fail (retrieved_iter);
      model_length = ctk_tree_model_iter_n_children (CTK_TREE_MODEL (filter), &iter);
      g_assert_cmpint (model_length, ==, expected_length);
    }
}

static void
set_path_visibility (FilterTest  *fixture,
                     const gchar *path,
                     gboolean     visible)
{
  CtkTreeIter store_iter;

  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store),
                                       &store_iter, path);
  ctk_tree_store_set (fixture->store, &store_iter,
                      1, visible,
                      -1);
}

#if 0
static void
insert_path_with_visibility (FilterTest  *fixture,
                             const gchar *path_string,
                             gboolean     visible)
{
  int position;
  CtkTreePath *path;
  CtkTreeIter parent, iter;

  path = ctk_tree_path_new_from_string (path_string);
  position = ctk_tree_path_get_indices (path)[ctk_tree_path_get_depth (path)];
  ctk_tree_path_up (path);

  if (ctk_tree_model_get_iter (CTK_TREE_MODEL (fixture->store), &parent, path))
    {
      ctk_tree_store_insert (fixture->store, &iter, &parent, position);
      create_tree_store_set_values (fixture->store, &iter, visible);
    }
  ctk_tree_path_free (path);
}
#endif

/*
 * The actual tests.
 */

static void
verify_test_suite (FilterTest    *fixture,
                   gconstpointer  user_data)
{
  check_filter_model (fixture);
}

static void
verify_test_suite_vroot (FilterTest    *fixture,
                         gconstpointer  user_data)
{
  check_filter_model_with_root (fixture, (CtkTreePath *)user_data);
}


static void
filled_hide_root_level (FilterTest    *fixture,
                        gconstpointer  user_data)
{
  signal_monitor_append_signal (fixture->monitor, ROW_DELETED, "2");
  set_path_visibility (fixture, "2", FALSE);
  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH - 1);

  signal_monitor_append_signal (fixture->monitor, ROW_DELETED, "0");
  set_path_visibility (fixture, "0", FALSE);
  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH - 2);

  signal_monitor_append_signal (fixture->monitor, ROW_DELETED, "2");
  set_path_visibility (fixture, "4", FALSE);
  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH - 3);


  /* Hide remaining */
  signal_monitor_append_signal (fixture->monitor, ROW_DELETED, "0");
  signal_monitor_append_signal (fixture->monitor, ROW_DELETED, "0");

  set_path_visibility (fixture, "1", FALSE);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH - 4);

  set_path_visibility (fixture, "3", FALSE);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH - 5);

  check_filter_model (fixture);

  /* Show some */
  signal_monitor_append_signal (fixture->monitor, ROW_INSERTED, "0");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "0");
  signal_monitor_append_signal (fixture->monitor, ROW_INSERTED, "1");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "1");

  set_path_visibility (fixture, "1", TRUE);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH - 4);

  set_path_visibility (fixture, "3", TRUE);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH - 3);

  check_filter_model (fixture);
}

static void
filled_hide_child_levels (FilterTest    *fixture,
                          gconstpointer  user_data)
{
  set_path_visibility (fixture, "0:2", FALSE);
  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "0", LEVEL_LENGTH - 1);

  set_path_visibility (fixture, "0:4", FALSE);
  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "0", LEVEL_LENGTH - 2);

  set_path_visibility (fixture, "0:4:3", FALSE);
  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "0", LEVEL_LENGTH - 2);

  set_path_visibility (fixture, "0:4:0", FALSE);
  set_path_visibility (fixture, "0:4:1", FALSE);
  set_path_visibility (fixture, "0:4:2", FALSE);
  set_path_visibility (fixture, "0:4:4", FALSE);
  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "0", LEVEL_LENGTH - 2);

  /* Since "0:2" is hidden, "0:4" must be "0:3" in the filter model */
  set_path_visibility (fixture, "0:4", TRUE);
  check_filter_model (fixture);
  check_level_length (fixture->filter, "0:3", 0);

  set_path_visibility (fixture, "0:2", TRUE);
  check_filter_model (fixture);
  check_level_length (fixture->filter, "0:2", LEVEL_LENGTH);
  check_level_length (fixture->filter, "0:3", LEVEL_LENGTH);
  check_level_length (fixture->filter, "0:4", 0);

  /* Once 0:4:0 got inserted, 0:4 became a parent.  Because 0:4 is
   * not visible, not signals are emitted.
   */
  set_path_visibility (fixture, "0:4:2", TRUE);
  set_path_visibility (fixture, "0:4:4", TRUE);
  signal_monitor_assert_is_empty (fixture->monitor);
  check_level_length (fixture->filter, "0:4", 2);
}

static void
filled_hide_child_levels_root_expanded (FilterTest    *fixture,
                                        gconstpointer  user_data)
{
  CtkTreePath *path;

  path = ctk_tree_path_new_from_indices (0, -1);
  ctk_tree_view_expand_row (CTK_TREE_VIEW (fixture->tree_view), path, FALSE);
  ctk_tree_path_free (path);

  signal_monitor_append_signal (fixture->monitor, ROW_DELETED, "0:2");
  set_path_visibility (fixture, "0:2", FALSE);
  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "0", LEVEL_LENGTH - 1);

  signal_monitor_append_signal (fixture->monitor, ROW_DELETED, "0:3");
  set_path_visibility (fixture, "0:4", FALSE);
  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "0", LEVEL_LENGTH - 2);

  set_path_visibility (fixture, "0:4:3", FALSE);
  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "0", LEVEL_LENGTH - 2);

  set_path_visibility (fixture, "0:4:0", FALSE);
  set_path_visibility (fixture, "0:4:1", FALSE);
  set_path_visibility (fixture, "0:4:2", FALSE);
  set_path_visibility (fixture, "0:4:4", FALSE);
  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "0", LEVEL_LENGTH - 2);

  /* Since "0:2" is hidden, "0:4" must be "0:3" in the filter model */
  signal_monitor_append_signal (fixture->monitor, ROW_INSERTED, "0:3");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "0:3");
  set_path_visibility (fixture, "0:4", TRUE);
  check_filter_model (fixture);
  check_level_length (fixture->filter, "0:3", 0);

  signal_monitor_append_signal (fixture->monitor, ROW_INSERTED, "0:2");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "0:2");
  set_path_visibility (fixture, "0:2", TRUE);
  check_filter_model (fixture);
  check_level_length (fixture->filter, "0:2", LEVEL_LENGTH);
  check_level_length (fixture->filter, "0:3", LEVEL_LENGTH);
  check_level_length (fixture->filter, "0:4", 0);

  /* has-child-toggled for 0:4 is required.  */
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "0:4");
  set_path_visibility (fixture, "0:4:2", TRUE);
  set_path_visibility (fixture, "0:4:4", TRUE);
  signal_monitor_assert_is_empty (fixture->monitor);
  check_level_length (fixture->filter, "0:4", 2);
}


static void
filled_vroot_hide_root_level (FilterTest    *fixture,
                              gconstpointer  user_data)
{
  CtkTreePath *path = (CtkTreePath *)user_data;

  /* These changes do not affect the filter's root level */
  set_path_visibility (fixture, "0", FALSE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "0", LEVEL_LENGTH);

  set_path_visibility (fixture, "4", FALSE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "0", LEVEL_LENGTH);

  /* Even though we set the virtual root parent node to FALSE,
   * the virtual root contents remain.
   */
  set_path_visibility (fixture, "2", FALSE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "0", LEVEL_LENGTH);

  /* No change */
  set_path_visibility (fixture, "1", FALSE);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "0", LEVEL_LENGTH);

  set_path_visibility (fixture, "3", FALSE);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "0", LEVEL_LENGTH);

  check_filter_model_with_root (fixture, path);

  /* Show some */
  set_path_visibility (fixture, "2", TRUE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "0", LEVEL_LENGTH);

  set_path_visibility (fixture, "1", TRUE);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "0", LEVEL_LENGTH);

  set_path_visibility (fixture, "3", TRUE);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "0", LEVEL_LENGTH);

  check_filter_model_with_root (fixture, path);

  /* Now test changes in the virtual root level */
  signal_monitor_append_signal (fixture->monitor, ROW_DELETED, "2");
  set_path_visibility (fixture, "2:2", FALSE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH - 1);

  signal_monitor_append_signal (fixture->monitor, ROW_DELETED, "3");
  set_path_visibility (fixture, "2:4", FALSE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH - 2);

  set_path_visibility (fixture, "1:4", FALSE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH - 2);

  signal_monitor_append_signal (fixture->monitor, ROW_INSERTED, "3");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "3");
  set_path_visibility (fixture, "2:4", TRUE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH - 1);

  set_path_visibility (fixture, "2", FALSE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH - 1);

  signal_monitor_append_signal (fixture->monitor, ROW_DELETED, "0");
  signal_monitor_append_signal (fixture->monitor, ROW_DELETED, "0");
  signal_monitor_append_signal (fixture->monitor, ROW_DELETED, "0");
  signal_monitor_append_signal (fixture->monitor, ROW_DELETED, "0");
  set_path_visibility (fixture, "2:0", FALSE);
  set_path_visibility (fixture, "2:1", FALSE);
  set_path_visibility (fixture, "2:2", FALSE);
  set_path_visibility (fixture, "2:3", FALSE);
  set_path_visibility (fixture, "2:4", FALSE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, 0);

  set_path_visibility (fixture, "2", TRUE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, 0);

  set_path_visibility (fixture, "1:4", FALSE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, 0);

  signal_monitor_append_signal (fixture->monitor, ROW_INSERTED, "0");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "0");
  set_path_visibility (fixture, "2:4", TRUE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH - 4);

  signal_monitor_append_signal (fixture->monitor, ROW_DELETED, "0");
  set_path_visibility (fixture, "2:4", FALSE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, 0);

  set_path_visibility (fixture, "2", FALSE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, 0);

  signal_monitor_append_signal (fixture->monitor, ROW_INSERTED, "0");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "0");
  signal_monitor_append_signal (fixture->monitor, ROW_INSERTED, "1");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "1");
  signal_monitor_append_signal (fixture->monitor, ROW_INSERTED, "2");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "2");
  set_path_visibility (fixture, "2:0", TRUE);
  set_path_visibility (fixture, "2:1", TRUE);
  set_path_visibility (fixture, "2:2", TRUE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH - 2);

  set_path_visibility (fixture, "2", TRUE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH - 2);
}

static void
filled_vroot_hide_child_levels (FilterTest    *fixture,
                                gconstpointer  user_data)
{
  CtkTreePath *path = (CtkTreePath *)user_data;

  set_path_visibility (fixture, "2:0:2", FALSE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "0", LEVEL_LENGTH - 1);

  set_path_visibility (fixture, "2:0:4", FALSE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "0", LEVEL_LENGTH - 2);

  set_path_visibility (fixture, "2:0:4:3", FALSE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "0", LEVEL_LENGTH - 2);

  set_path_visibility (fixture, "2:0:4:0", FALSE);
  set_path_visibility (fixture, "2:0:4:1", FALSE);
  set_path_visibility (fixture, "2:0:4:2", FALSE);
  set_path_visibility (fixture, "2:0:4:4", FALSE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "0", LEVEL_LENGTH - 2);

  /* Since "0:2" is hidden, "0:4" must be "0:3" in the filter model */
  set_path_visibility (fixture, "2:0:4", TRUE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, "0:3", 0);

  set_path_visibility (fixture, "2:0:2", TRUE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, "0:2", LEVEL_LENGTH);
  check_level_length (fixture->filter, "0:3", LEVEL_LENGTH);
  check_level_length (fixture->filter, "0:4", 0);

  /* Once 0:4:0 got inserted, 0:4 became a parent. However, 0:4 is not
   * visible, so no signal should be emitted.
   */
  set_path_visibility (fixture, "2:0:4:2", TRUE);
  set_path_visibility (fixture, "2:0:4:4", TRUE);
  check_level_length (fixture->filter, "0:4", 2);
  signal_monitor_assert_is_empty (fixture->monitor);
}

static void
filled_vroot_hide_child_levels_root_expanded (FilterTest    *fixture,
                                              gconstpointer  user_data)
{
  CtkTreePath *path = (CtkTreePath *)user_data;
  CtkTreePath *tmp_path;

  tmp_path = ctk_tree_path_new_from_indices (0, -1);
  ctk_tree_view_expand_row (CTK_TREE_VIEW (fixture->tree_view), tmp_path, FALSE);
  ctk_tree_path_free (tmp_path);

  signal_monitor_append_signal (fixture->monitor, ROW_DELETED, "0:2");
  set_path_visibility (fixture, "2:0:2", FALSE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "0", LEVEL_LENGTH - 1);

  signal_monitor_append_signal (fixture->monitor, ROW_DELETED, "0:3");
  set_path_visibility (fixture, "2:0:4", FALSE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "0", LEVEL_LENGTH - 2);

  set_path_visibility (fixture, "2:0:4:3", FALSE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "0", LEVEL_LENGTH - 2);

  set_path_visibility (fixture, "2:0:4:0", FALSE);
  set_path_visibility (fixture, "2:0:4:1", FALSE);
  set_path_visibility (fixture, "2:0:4:2", FALSE);
  set_path_visibility (fixture, "2:0:4:4", FALSE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "0", LEVEL_LENGTH - 2);

  /* Since "0:2" is hidden, "0:4" must be "0:3" in the filter model */
  signal_monitor_append_signal (fixture->monitor, ROW_INSERTED, "0:3");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "0:3");
  set_path_visibility (fixture, "2:0:4", TRUE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, "0:3", 0);

  signal_monitor_append_signal (fixture->monitor, ROW_INSERTED, "0:2");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "0:2");
  set_path_visibility (fixture, "2:0:2", TRUE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, "0:2", LEVEL_LENGTH);
  check_level_length (fixture->filter, "0:3", LEVEL_LENGTH);
  check_level_length (fixture->filter, "0:4", 0);

  /* Once 0:4:0 got inserted, 0:4 became a parent */
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "0:4");
  set_path_visibility (fixture, "2:0:4:2", TRUE);
  set_path_visibility (fixture, "2:0:4:4", TRUE);
  check_level_length (fixture->filter, "0:4", 2);
  signal_monitor_assert_is_empty (fixture->monitor);
}

static void
empty_show_nodes (FilterTest    *fixture,
                  gconstpointer  user_data)
{
  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, 0);

  signal_monitor_append_signal (fixture->monitor, ROW_INSERTED, "0");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "0");
  set_path_visibility (fixture, "3", TRUE);
  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, 1);
  check_level_length (fixture->filter, "0", 0);

  set_path_visibility (fixture, "3:2:2", TRUE);
  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, 1);
  check_level_length (fixture->filter, "0", 0);

  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "0");
  set_path_visibility (fixture, "3:2", TRUE);
  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, 1);
  check_level_length (fixture->filter, "0", 1);
  check_level_length (fixture->filter, "0:0", 1);
  check_level_length (fixture->filter, "0:0:0", 0);

  signal_monitor_append_signal (fixture->monitor, ROW_DELETED, "0");
  set_path_visibility (fixture, "3", FALSE);
  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, 0);

  signal_monitor_append_signal (fixture->monitor, ROW_INSERTED, "0");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "0");
  set_path_visibility (fixture, "3:2:1", TRUE);
  set_path_visibility (fixture, "3", TRUE);
  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, 1);
  check_level_length (fixture->filter, "0", 1);
  check_level_length (fixture->filter, "0:0", 2);
  check_level_length (fixture->filter, "0:0:0", 0);
}

static void
empty_show_multiple_nodes (FilterTest    *fixture,
                           gconstpointer  user_data)
{
  CtkTreeIter iter;
  CtkTreePath *changed_path;

  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, 0);

  signal_monitor_append_signal (fixture->monitor, ROW_INSERTED, "0");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "0");
  signal_monitor_append_signal (fixture->monitor, ROW_INSERTED, "1");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "1");

  /* We simulate a change in visible func condition with this.  The
   * visibility state of multiple nodes changes at once, we emit row-changed
   * for these nodes (and others) after that.
   */
  filter_test_block_signals (fixture);
  set_path_visibility (fixture, "3", TRUE);
  set_path_visibility (fixture, "4", TRUE);
  filter_test_unblock_signals (fixture);

  changed_path = ctk_tree_path_new ();
  ctk_tree_path_append_index (changed_path, 2);
  ctk_tree_model_get_iter (CTK_TREE_MODEL (fixture->store),
                           &iter, changed_path);
  /* Invisible node - so no signals expected */
  ctk_tree_model_row_changed (CTK_TREE_MODEL (fixture->store),
                              changed_path, &iter);

  ctk_tree_path_next (changed_path);
  ctk_tree_model_iter_next (CTK_TREE_MODEL (fixture->store), &iter);
  ctk_tree_model_row_changed (CTK_TREE_MODEL (fixture->store),
                              changed_path, &iter);

  ctk_tree_path_next (changed_path);
  ctk_tree_model_iter_next (CTK_TREE_MODEL (fixture->store), &iter);
  ctk_tree_model_row_changed (CTK_TREE_MODEL (fixture->store),
                              changed_path, &iter);

  ctk_tree_path_free (changed_path);

  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, 2);
  check_level_length (fixture->filter, "0", 0);

  set_path_visibility (fixture, "3:2:2", TRUE);
  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, 2);
  check_level_length (fixture->filter, "0", 0);

  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "0");
  set_path_visibility (fixture, "3:2", TRUE);
  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, 2);
  check_level_length (fixture->filter, "0", 1);
  check_level_length (fixture->filter, "0:0", 1);
  check_level_length (fixture->filter, "0:0:0", 0);

  signal_monitor_append_signal (fixture->monitor, ROW_DELETED, "0");
  set_path_visibility (fixture, "3", FALSE);
  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, 1);

  signal_monitor_append_signal (fixture->monitor, ROW_INSERTED, "0");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "0");
  set_path_visibility (fixture, "3:2:1", TRUE);
  set_path_visibility (fixture, "3", TRUE);
  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, 2);
  check_level_length (fixture->filter, "0", 1);
  check_level_length (fixture->filter, "0:0", 2);
  check_level_length (fixture->filter, "0:0:0", 0);
}

static void
empty_vroot_show_nodes (FilterTest    *fixture,
                        gconstpointer  user_data)
{
  CtkTreePath *path = (CtkTreePath *)user_data;

  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, 0);

  set_path_visibility (fixture, "2", TRUE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, 0);

  set_path_visibility (fixture, "2:2:2", TRUE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, 0);

  signal_monitor_append_signal (fixture->monitor, ROW_INSERTED, "0");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "0");
  set_path_visibility (fixture, "2:2", TRUE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, 1);
  check_level_length (fixture->filter, "0", 1);
  check_level_length (fixture->filter, "0:0", 0);

  set_path_visibility (fixture, "3", TRUE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, 1);

  signal_monitor_append_signal (fixture->monitor, ROW_DELETED, "0");
  set_path_visibility (fixture, "2:2", FALSE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, 0);

  signal_monitor_append_signal (fixture->monitor, ROW_INSERTED, "0");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "0");
  set_path_visibility (fixture, "2:2:1", TRUE);
  set_path_visibility (fixture, "2:2", TRUE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, 1);
  check_level_length (fixture->filter, "0", 2);
  check_level_length (fixture->filter, "0:1", 0);
}

static void
empty_vroot_show_multiple_nodes (FilterTest    *fixture,
                                 gconstpointer  user_data)
{
  CtkTreeIter iter;
  CtkTreePath *changed_path;
  CtkTreePath *path = (CtkTreePath *)user_data;

  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, 0);

  /* We simulate a change in visible func condition with this.  The
   * visibility state of multiple nodes changes at once, we emit row-changed
   * for these nodes (and others) after that.
   */
  filter_test_block_signals (fixture);
  set_path_visibility (fixture, "2", TRUE);
  set_path_visibility (fixture, "3", TRUE);
  filter_test_unblock_signals (fixture);

  changed_path = ctk_tree_path_new ();
  ctk_tree_path_append_index (changed_path, 1);
  ctk_tree_model_get_iter (CTK_TREE_MODEL (fixture->store),
                           &iter, changed_path);
  ctk_tree_model_row_changed (CTK_TREE_MODEL (fixture->store),
                              changed_path, &iter);

  ctk_tree_path_next (changed_path);
  ctk_tree_model_iter_next (CTK_TREE_MODEL (fixture->store), &iter);
  ctk_tree_model_row_changed (CTK_TREE_MODEL (fixture->store),
                              changed_path, &iter);

  ctk_tree_path_next (changed_path);
  ctk_tree_model_iter_next (CTK_TREE_MODEL (fixture->store), &iter);
  ctk_tree_model_row_changed (CTK_TREE_MODEL (fixture->store),
                              changed_path, &iter);

  ctk_tree_path_next (changed_path);
  ctk_tree_model_iter_next (CTK_TREE_MODEL (fixture->store), &iter);
  ctk_tree_model_row_changed (CTK_TREE_MODEL (fixture->store),
                              changed_path, &iter);

  ctk_tree_path_free (changed_path);

  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, 0);

  set_path_visibility (fixture, "2:2:2", TRUE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, 0);

  signal_monitor_append_signal (fixture->monitor, ROW_INSERTED, "0");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "0");
  signal_monitor_append_signal (fixture->monitor, ROW_INSERTED, "1");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "1");

  /* Again, we simulate a call to refilter */
  filter_test_block_signals (fixture);
  set_path_visibility (fixture, "2:2", TRUE);
  set_path_visibility (fixture, "2:3", TRUE);
  filter_test_unblock_signals (fixture);

  changed_path = ctk_tree_path_new ();
  ctk_tree_path_append_index (changed_path, 2);
  ctk_tree_path_append_index (changed_path, 1);
  ctk_tree_model_get_iter (CTK_TREE_MODEL (fixture->store),
                           &iter, changed_path);
  ctk_tree_model_row_changed (CTK_TREE_MODEL (fixture->store),
                              changed_path, &iter);

  ctk_tree_path_next (changed_path);
  ctk_tree_model_iter_next (CTK_TREE_MODEL (fixture->store), &iter);
  ctk_tree_model_row_changed (CTK_TREE_MODEL (fixture->store),
                              changed_path, &iter);

  ctk_tree_path_next (changed_path);
  ctk_tree_model_iter_next (CTK_TREE_MODEL (fixture->store), &iter);
  ctk_tree_model_row_changed (CTK_TREE_MODEL (fixture->store),
                              changed_path, &iter);

  ctk_tree_path_next (changed_path);
  ctk_tree_model_iter_next (CTK_TREE_MODEL (fixture->store), &iter);
  ctk_tree_model_row_changed (CTK_TREE_MODEL (fixture->store),
                              changed_path, &iter);

  ctk_tree_path_free (changed_path);

  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, 2);
  check_level_length (fixture->filter, "0", 1);
  check_level_length (fixture->filter, "0:0", 0);

  set_path_visibility (fixture, "3", TRUE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, 2);

  signal_monitor_append_signal (fixture->monitor, ROW_DELETED, "0");
  set_path_visibility (fixture, "2:2", FALSE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, 1);

  signal_monitor_append_signal (fixture->monitor, ROW_INSERTED, "0");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "0");
  set_path_visibility (fixture, "2:2:1", TRUE);
  set_path_visibility (fixture, "2:2", TRUE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, 2);
  check_level_length (fixture->filter, "0", 2);
  check_level_length (fixture->filter, "0:1", 0);
}


static void
unfiltered_hide_single (FilterTest    *fixture,
                        gconstpointer  user_data)

{
  signal_monitor_append_signal (fixture->monitor, ROW_CHANGED, "2");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "2");
  set_path_visibility (fixture, "2", FALSE);

  signal_monitor_assert_is_empty (fixture->monitor);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);

  /* The view only shows the root level, so we only expect signals
   * for the root level.
   */
  filter_test_append_refilter_signals (fixture, 1);
  filter_test_enable_filter (fixture);

  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH - 1);
}

static void
unfiltered_hide_single_root_expanded (FilterTest    *fixture,
                                      gconstpointer  user_data)

{
  signal_monitor_append_signal (fixture->monitor, ROW_CHANGED, "2");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "2");
  set_path_visibility (fixture, "2", FALSE);

  signal_monitor_assert_is_empty (fixture->monitor);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);

  filter_test_append_refilter_signals (fixture, 2);
  filter_test_enable_filter (fixture);

  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH - 1);
}

static void
unfiltered_hide_single_child (FilterTest    *fixture,
                              gconstpointer  user_data)

{
  /* This row is not shown, so its signal is not propagated */
  set_path_visibility (fixture, "2:2", FALSE);

  signal_monitor_assert_is_empty (fixture->monitor);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "2", LEVEL_LENGTH);

  /* The view only shows the root level, so we only expect signals
   * for the root level.
   */
  filter_test_append_refilter_signals (fixture, 0);
  filter_test_enable_filter (fixture);

  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "2", LEVEL_LENGTH - 1);
}

static void
unfiltered_hide_single_child_root_expanded (FilterTest    *fixture,
                                            gconstpointer  user_data)

{
  signal_monitor_append_signal (fixture->monitor, ROW_CHANGED, "2:2");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "2:2");
  set_path_visibility (fixture, "2:2", FALSE);

  signal_monitor_assert_is_empty (fixture->monitor);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "2", LEVEL_LENGTH);

  filter_test_append_refilter_signals (fixture, 2);
  filter_test_enable_filter (fixture);

  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "2", LEVEL_LENGTH - 1);
}

static void
unfiltered_hide_single_multi_level (FilterTest    *fixture,
                                    gconstpointer  user_data)

{
  /* This row is not shown, so its signal is not propagated */
  set_path_visibility (fixture, "2:2:2", FALSE);

  /* This row is not shown, so its signal is not propagated */
  set_path_visibility (fixture, "2:2", FALSE);

  signal_monitor_assert_is_empty (fixture->monitor);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "2", LEVEL_LENGTH);
  check_level_length (fixture->filter, "2:2", LEVEL_LENGTH);

  /* The view only shows the root level, so we only expect signals
   * for the root level.
   */
  filter_test_append_refilter_signals (fixture, 1);
  filter_test_enable_filter (fixture);

  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "2", LEVEL_LENGTH - 1);

  set_path_visibility (fixture, "2:2", TRUE);

  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "2", LEVEL_LENGTH);
  check_level_length (fixture->filter, "2:2", LEVEL_LENGTH - 1);
}

static void
unfiltered_hide_single_multi_level_root_expanded (FilterTest    *fixture,
                                                  gconstpointer  user_data)

{
  /* This row is not shown, so its signal is not propagated */
  set_path_visibility (fixture, "2:2:2", FALSE);

  signal_monitor_append_signal (fixture->monitor, ROW_CHANGED, "2:2");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "2:2");
  set_path_visibility (fixture, "2:2", FALSE);

  signal_monitor_assert_is_empty (fixture->monitor);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "2", LEVEL_LENGTH);
  check_level_length (fixture->filter, "2:2", LEVEL_LENGTH);

  filter_test_append_refilter_signals (fixture, 2);
  filter_test_enable_filter (fixture);

  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "2", LEVEL_LENGTH - 1);

  signal_monitor_append_signal (fixture->monitor, ROW_INSERTED, "2:2");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "2:2");
  set_path_visibility (fixture, "2:2", TRUE);

  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "2", LEVEL_LENGTH);
  check_level_length (fixture->filter, "2:2", LEVEL_LENGTH - 1);
}



static void
unfiltered_vroot_hide_single (FilterTest    *fixture,
                              gconstpointer  user_data)

{
  CtkTreePath *path = (CtkTreePath *)user_data;

  signal_monitor_append_signal (fixture->monitor, ROW_CHANGED, "2");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "2");
  set_path_visibility (fixture, "2:2", FALSE);

  signal_monitor_assert_is_empty (fixture->monitor);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);

  /* The view only shows the root level, so we only expect signals
   * for the root level.  (Though for the depth argument, we have to
   * take the virtual root into account).
   */
  filter_test_append_refilter_signals_with_vroot (fixture, 2, path);
  filter_test_enable_filter (fixture);

  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH - 1);
}

static void
unfiltered_vroot_hide_single_child (FilterTest    *fixture,
                                    gconstpointer  user_data)

{
  CtkTreePath *path = (CtkTreePath *)user_data;

  /* Not visible, so no signal will be received. */
  set_path_visibility (fixture, "2:2:2", FALSE);

  signal_monitor_assert_is_empty (fixture->monitor);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "2", LEVEL_LENGTH);

  /* The view only shows the root level, so we only expect signals
   * for the root level.  (Though for the depth argument, we have to
   * take the virtual root into account).
   */
  filter_test_append_refilter_signals_with_vroot (fixture, 2, path);
  filter_test_enable_filter (fixture);

  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "2", LEVEL_LENGTH - 1);
}

static void
unfiltered_vroot_hide_single_child_root_expanded (FilterTest    *fixture,
                                                  gconstpointer  user_data)

{
  CtkTreePath *path = (CtkTreePath *)user_data;

  signal_monitor_append_signal (fixture->monitor, ROW_CHANGED, "2:2");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "2:2");
  set_path_visibility (fixture, "2:2:2", FALSE);

  signal_monitor_assert_is_empty (fixture->monitor);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "2", LEVEL_LENGTH);

  filter_test_append_refilter_signals_with_vroot (fixture, 3, path);
  filter_test_enable_filter (fixture);

  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "2", LEVEL_LENGTH - 1);
}

static void
unfiltered_vroot_hide_single_multi_level (FilterTest    *fixture,
                                          gconstpointer  user_data)

{
  CtkTreePath *path = (CtkTreePath *)user_data;

  /* This row is not shown, so its signal is not propagated */
  set_path_visibility (fixture, "2:2:2:2", FALSE);

  /* Not shown, so no signal */
  set_path_visibility (fixture, "2:2:2", FALSE);

  signal_monitor_assert_is_empty (fixture->monitor);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "2", LEVEL_LENGTH);
  check_level_length (fixture->filter, "2:2", LEVEL_LENGTH);

  /* We only expect signals for the root level.  The depth is 2
   * because we have to take the virtual root into account.
   */
  filter_test_append_refilter_signals_with_vroot (fixture, 2, path);
  filter_test_enable_filter (fixture);

  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "2", LEVEL_LENGTH - 1);

  /* Not shown, so no signal */
  set_path_visibility (fixture, "2:2:2", TRUE);

  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "2", LEVEL_LENGTH);
  check_level_length (fixture->filter, "2:2", LEVEL_LENGTH - 1);
}

static void
unfiltered_vroot_hide_single_multi_level_root_expanded (FilterTest    *fixture,
                                                        gconstpointer  user_data)

{
  CtkTreePath *path = (CtkTreePath *)user_data;

  /* This row is not shown, so its signal is not propagated */
  set_path_visibility (fixture, "2:2:2:2", FALSE);

  signal_monitor_append_signal (fixture->monitor, ROW_CHANGED, "2:2");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "2:2");
  set_path_visibility (fixture, "2:2:2", FALSE);

  signal_monitor_assert_is_empty (fixture->monitor);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "2", LEVEL_LENGTH);
  check_level_length (fixture->filter, "2:2", LEVEL_LENGTH);

  filter_test_append_refilter_signals_with_vroot (fixture, 3, path);
  filter_test_enable_filter (fixture);

  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "2", LEVEL_LENGTH - 1);

  signal_monitor_append_signal (fixture->monitor, ROW_INSERTED, "2:2");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "2:2");
  set_path_visibility (fixture, "2:2:2", TRUE);

  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "2", LEVEL_LENGTH);
  check_level_length (fixture->filter, "2:2", LEVEL_LENGTH - 1);
}

static void
unfiltered_show_single (FilterTest    *fixture,
                        gconstpointer  user_data)

{
  signal_monitor_append_signal (fixture->monitor, ROW_CHANGED, "2");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "2");
  set_path_visibility (fixture, "2", TRUE);

  signal_monitor_assert_is_empty (fixture->monitor);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);

  /* We only expect signals for the root level */
  filter_test_append_refilter_signals (fixture, 1);
  filter_test_enable_filter (fixture);

  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, 1);
}

static void
unfiltered_show_single_child (FilterTest    *fixture,
                              gconstpointer  user_data)

{
  set_path_visibility (fixture, "2:2", TRUE);

  signal_monitor_assert_is_empty (fixture->monitor);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "2", LEVEL_LENGTH);

  /* We only expect signals for the root level */
  filter_test_append_refilter_signals (fixture, 1);
  filter_test_enable_filter (fixture);

  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, 0);

  /* From here we are filtered, "2" in the real model is "0" in the filter
   * model.
   */
  signal_monitor_append_signal (fixture->monitor, ROW_INSERTED, "0");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "0");
  set_path_visibility (fixture, "2", TRUE);
  signal_monitor_assert_is_empty (fixture->monitor);
  check_level_length (fixture->filter, NULL, 1);
  check_level_length (fixture->filter, "0", 1);
}

static void
unfiltered_show_single_child_root_expanded (FilterTest    *fixture,
                                            gconstpointer  user_data)

{
  signal_monitor_append_signal (fixture->monitor, ROW_CHANGED, "2:2");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "2:2");
  set_path_visibility (fixture, "2:2", TRUE);

  signal_monitor_assert_is_empty (fixture->monitor);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "2", LEVEL_LENGTH);

  filter_test_append_refilter_signals (fixture, 2);
  filter_test_enable_filter (fixture);

  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, 0);

  /* From here we are filtered, "2" in the real model is "0" in the filter
   * model.
   */
  signal_monitor_append_signal (fixture->monitor, ROW_INSERTED, "0");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "0");
  set_path_visibility (fixture, "2", TRUE);
  signal_monitor_assert_is_empty (fixture->monitor);
  check_level_length (fixture->filter, NULL, 1);
  check_level_length (fixture->filter, "0", 1);
}

static void
unfiltered_show_single_multi_level (FilterTest    *fixture,
                                    gconstpointer  user_data)

{
  /* The view is not showing these rows (collapsed state), so it is not
   * referenced.  The signal should not go through.
   */
  set_path_visibility (fixture, "2:2:2", TRUE);
  set_path_visibility (fixture, "2:2", TRUE);

  signal_monitor_assert_is_empty (fixture->monitor);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "2", LEVEL_LENGTH);
  check_level_length (fixture->filter, "2:2", LEVEL_LENGTH);

  /* We only expect signals for the first level */
  filter_test_append_refilter_signals (fixture, 1);
  filter_test_enable_filter (fixture);

  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, 0);

  /* From here we are filtered, "2" in the real model is "0" in the filter
   * model.
   */
  signal_monitor_append_signal (fixture->monitor, ROW_INSERTED, "0");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "0");
  set_path_visibility (fixture, "2", TRUE);
  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, 1);
  check_level_length (fixture->filter, "0", 1);
  check_level_length (fixture->filter, "0:0", 1);
}

static void
unfiltered_show_single_multi_level_root_expanded (FilterTest    *fixture,
                                                  gconstpointer  user_data)

{
  /* The view is not showing this row (collapsed state), so it is not
   * referenced.  The signal should not go through.
   */
  set_path_visibility (fixture, "2:2:2", TRUE);

  signal_monitor_append_signal (fixture->monitor, ROW_CHANGED, "2:2");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "2:2");
  set_path_visibility (fixture, "2:2", TRUE);

  signal_monitor_assert_is_empty (fixture->monitor);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "2", LEVEL_LENGTH);
  check_level_length (fixture->filter, "2:2", LEVEL_LENGTH);

  filter_test_append_refilter_signals (fixture, 2);
  filter_test_enable_filter (fixture);

  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, 0);

  /* From here we are filtered, "2" in the real model is "0" in the filter
   * model.
   */
  signal_monitor_append_signal (fixture->monitor, ROW_INSERTED, "0");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "0");
  set_path_visibility (fixture, "2", TRUE);
  check_filter_model (fixture);
  check_level_length (fixture->filter, NULL, 1);
  check_level_length (fixture->filter, "0", 1);
  check_level_length (fixture->filter, "0:0", 1);
}

static void
unfiltered_vroot_show_single (FilterTest    *fixture,
                              gconstpointer  user_data)

{
  CtkTreePath *path = (CtkTreePath *)user_data;

  signal_monitor_append_signal (fixture->monitor, ROW_CHANGED, "2");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "2");
  set_path_visibility (fixture, "2:2", TRUE);

  signal_monitor_assert_is_empty (fixture->monitor);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);

  /* The view only shows the root level, so the filter model only has
   * the first two levels cached.
   */
  filter_test_append_refilter_signals_with_vroot (fixture, 2, path);
  filter_test_enable_filter (fixture);

  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, 1);
}

static void
unfiltered_vroot_show_single_child (FilterTest    *fixture,
                                    gconstpointer  user_data)

{
  CtkTreePath *path = (CtkTreePath *)user_data;

  set_path_visibility (fixture, "2:2:2", TRUE);

  signal_monitor_assert_is_empty (fixture->monitor);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "2", LEVEL_LENGTH);

  /* The view only shows the root level, so the filter model only has
   * the first two levels cached.
   */
  filter_test_append_refilter_signals_with_vroot (fixture, 2, path);
  filter_test_enable_filter (fixture);

  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, 0);

  /* From here we are filtered, "2" in the real model is "0" in the filter
   * model.
   */
  signal_monitor_append_signal (fixture->monitor, ROW_INSERTED, "0");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "0");
  set_path_visibility (fixture, "2:2", TRUE);
  signal_monitor_assert_is_empty (fixture->monitor);
  check_level_length (fixture->filter, NULL, 1);
  check_level_length (fixture->filter, "0", 1);
}

static void
unfiltered_vroot_show_single_child_root_expanded (FilterTest    *fixture,
                                                  gconstpointer  user_data)

{
  CtkTreePath *path = (CtkTreePath *)user_data;

  signal_monitor_append_signal (fixture->monitor, ROW_CHANGED, "2:2");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "2:2");
  set_path_visibility (fixture, "2:2:2", TRUE);

  signal_monitor_assert_is_empty (fixture->monitor);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "2", LEVEL_LENGTH);

  filter_test_append_refilter_signals_with_vroot (fixture, 3, path);
  filter_test_enable_filter (fixture);

  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, 0);

  /* From here we are filtered, "2" in the real model is "0" in the filter
   * model.
   */
  signal_monitor_append_signal (fixture->monitor, ROW_INSERTED, "0");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "0");
  set_path_visibility (fixture, "2:2", TRUE);
  signal_monitor_assert_is_empty (fixture->monitor);
  check_level_length (fixture->filter, NULL, 1);
  check_level_length (fixture->filter, "0", 1);
}


static void
unfiltered_vroot_show_single_multi_level (FilterTest    *fixture,
                                          gconstpointer  user_data)

{
  CtkTreePath *path = (CtkTreePath *)user_data;

  /* The view is not showing this row (collapsed state), so it is not
   * referenced.  The signal should not go through.
   */
  set_path_visibility (fixture, "2:2:2:2", TRUE);

  set_path_visibility (fixture, "2:2:2", TRUE);

  signal_monitor_assert_is_empty (fixture->monitor);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "2", LEVEL_LENGTH);
  check_level_length (fixture->filter, "2:2", LEVEL_LENGTH);

  /* We only expect signals for the root level */
  filter_test_append_refilter_signals_with_vroot (fixture, 2, path);
  filter_test_enable_filter (fixture);

  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, 0);

  /* From here we are filtered, "2" in the real model is "0" in the filter
   * model.
   */
  signal_monitor_append_signal (fixture->monitor, ROW_INSERTED, "0");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "0");
  set_path_visibility (fixture, "2:2", TRUE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, 1);
  check_level_length (fixture->filter, "0", 1);
  check_level_length (fixture->filter, "0:0", 1);
}

static void
unfiltered_vroot_show_single_multi_level_root_expanded (FilterTest    *fixture,
                                                        gconstpointer  user_data)

{
  CtkTreePath *path = (CtkTreePath *)user_data;

  /* The view is not showing this row (collapsed state), so it is not
   * referenced.  The signal should not go through.
   */
  set_path_visibility (fixture, "2:2:2:2", TRUE);

  signal_monitor_append_signal (fixture->monitor, ROW_CHANGED, "2:2");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "2:2");
  set_path_visibility (fixture, "2:2:2", TRUE);

  signal_monitor_assert_is_empty (fixture->monitor);
  check_level_length (fixture->filter, NULL, LEVEL_LENGTH);
  check_level_length (fixture->filter, "2", LEVEL_LENGTH);
  check_level_length (fixture->filter, "2:2", LEVEL_LENGTH);

  filter_test_append_refilter_signals_with_vroot (fixture, 3, path);
  filter_test_enable_filter (fixture);

  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, 0);

  /* From here we are filtered, "2" in the real model is "0" in the filter
   * model.
   */
  signal_monitor_append_signal (fixture->monitor, ROW_INSERTED, "0");
  signal_monitor_append_signal (fixture->monitor, ROW_HAS_CHILD_TOGGLED, "0");
  set_path_visibility (fixture, "2:2", TRUE);
  check_filter_model_with_root (fixture, path);
  check_level_length (fixture->filter, NULL, 1);
  check_level_length (fixture->filter, "0", 1);
  check_level_length (fixture->filter, "0:0", 1);
}

static void
unfiltered_rows_reordered_root_level (FilterTest    *fixture,
                                      gconstpointer  user_data)
{
  int order0[] = { 1, 2, 3, 4, 0 };
  int order1[] = { 0, 2, 1, 3, 4 };
  int order2[] = { 4, 0, 1, 2, 3 };
  CtkTreeIter iter0, iter1, iter2, iter3, iter4;
  CtkTreePath *path;

  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store),
                                       &iter0, "0");
  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store),
                                       &iter1, "1");
  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store),
                                       &iter2, "2");
  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store),
                                       &iter3, "3");
  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store),
                                       &iter4, "4");

  path = ctk_tree_path_new ();
  signal_monitor_append_signal_reordered (fixture->monitor,
                                          ROWS_REORDERED,
                                          path, order0, 5);
  ctk_tree_store_move_after (fixture->store, &iter0, &iter4);
  signal_monitor_assert_is_empty (fixture->monitor);

  signal_monitor_append_signal_reordered (fixture->monitor,
                                          ROWS_REORDERED,
                                          path, order1, 5);
  ctk_tree_store_move_after (fixture->store, &iter2, &iter3);
  signal_monitor_assert_is_empty (fixture->monitor);

  signal_monitor_append_signal_reordered (fixture->monitor,
                                          ROWS_REORDERED,
                                          path, order2, 5);
  ctk_tree_store_move_before (fixture->store, &iter0, &iter1);
  signal_monitor_assert_is_empty (fixture->monitor);

  ctk_tree_path_free (path);
}

static void
unfiltered_rows_reordered_child_level (FilterTest    *fixture,
                                       gconstpointer  user_data)
{
  int order0[] = { 1, 2, 3, 4, 0 };
  int order1[] = { 0, 2, 1, 3, 4 };
  int order2[] = { 4, 0, 1, 2, 3 };
  CtkTreeIter iter0, iter1, iter2, iter3, iter4;
  CtkTreePath *path;

  /* Expand row 0 */
  path = ctk_tree_path_new_from_indices (0, -1);
  ctk_tree_view_expand_row (CTK_TREE_VIEW (fixture->tree_view), path, FALSE);

  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store),
                                       &iter0, "0:0");
  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store),
                                       &iter1, "0:1");
  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store),
                                       &iter2, "0:2");
  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store),
                                       &iter3, "0:3");
  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store),
                                       &iter4, "0:4");

  signal_monitor_append_signal_reordered (fixture->monitor,
                                          ROWS_REORDERED,
                                          path, order0, 5);
  ctk_tree_store_move_after (fixture->store, &iter0, &iter4);
  signal_monitor_assert_is_empty (fixture->monitor);

  signal_monitor_append_signal_reordered (fixture->monitor,
                                          ROWS_REORDERED,
                                          path, order1, 5);
  ctk_tree_store_move_after (fixture->store, &iter2, &iter3);
  signal_monitor_assert_is_empty (fixture->monitor);

  signal_monitor_append_signal_reordered (fixture->monitor,
                                          ROWS_REORDERED,
                                          path, order2, 5);
  ctk_tree_store_move_before (fixture->store, &iter0, &iter1);
  signal_monitor_assert_is_empty (fixture->monitor);

  ctk_tree_path_free (path);
}

static void
filtered_rows_reordered_root_level_first_hidden (FilterTest    *fixture,
                                                 gconstpointer  user_data)
{
  int order0[] = { 1, 2, 3, 0 };
  int order1[] = { 0, 2, 1, 3 };
  int order2[] = { 3, 0, 1, 2 };
  CtkTreeIter iter1, iter2, iter3, iter4;
  CtkTreePath *path;

  /* Hide middle path */
  signal_monitor_append_signal (fixture->monitor,
                                ROW_DELETED, "0");
  set_path_visibility (fixture, "0", FALSE);
  signal_monitor_assert_is_empty (fixture->monitor);

  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store),
                                       &iter1, "1");
  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store),
                                       &iter2, "2");
  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store),
                                       &iter3, "3");
  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store),
                                       &iter4, "4");

  path = ctk_tree_path_new ();
  signal_monitor_append_signal_reordered (fixture->monitor,
                                          ROWS_REORDERED,
                                          path, order0, 4);
  ctk_tree_store_move_after (fixture->store, &iter1, &iter4);
  signal_monitor_assert_is_empty (fixture->monitor);

  signal_monitor_append_signal_reordered (fixture->monitor,
                                          ROWS_REORDERED,
                                          path, order1, 4);
  ctk_tree_store_move_after (fixture->store, &iter3, &iter4);
  signal_monitor_assert_is_empty (fixture->monitor);

  signal_monitor_append_signal_reordered (fixture->monitor,
                                          ROWS_REORDERED,
                                          path, order2, 4);
  ctk_tree_store_move_before (fixture->store, &iter1, &iter2);
  signal_monitor_assert_is_empty (fixture->monitor);

  ctk_tree_path_free (path);
}

static void
filtered_rows_reordered_root_level_middle_hidden (FilterTest    *fixture,
                                                  gconstpointer  user_data)
{
  int order0[] = { 1, 2, 3, 0 };
  int order1[] = { 0, 2, 1, 3 };
  int order2[] = { 3, 0, 1, 2 };
  CtkTreeIter iter0, iter1, iter3, iter4;
  CtkTreePath *path;

  /* Hide middle path */
  signal_monitor_append_signal (fixture->monitor,
                                ROW_DELETED, "2");
  set_path_visibility (fixture, "2", FALSE);
  signal_monitor_assert_is_empty (fixture->monitor);

  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store),
                                       &iter0, "0");
  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store),
                                       &iter1, "1");
  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store),
                                       &iter3, "3");
  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store),
                                       &iter4, "4");

  path = ctk_tree_path_new ();
  signal_monitor_append_signal_reordered (fixture->monitor,
                                          ROWS_REORDERED,
                                          path, order0, 4);
  ctk_tree_store_move_after (fixture->store, &iter0, &iter4);
  signal_monitor_assert_is_empty (fixture->monitor);

  signal_monitor_append_signal_reordered (fixture->monitor,
                                          ROWS_REORDERED,
                                          path, order1, 4);
  ctk_tree_store_move_after (fixture->store, &iter3, &iter4);
  signal_monitor_assert_is_empty (fixture->monitor);

  signal_monitor_append_signal_reordered (fixture->monitor,
                                          ROWS_REORDERED,
                                          path, order2, 4);
  ctk_tree_store_move_before (fixture->store, &iter0, &iter1);
  signal_monitor_assert_is_empty (fixture->monitor);

  ctk_tree_path_free (path);
}

static void
filtered_rows_reordered_child_level_first_hidden (FilterTest    *fixture,
                                                  gconstpointer  user_data)
{
  int order0[] = { 1, 2, 3, 0 };
  int order1[] = { 0, 2, 1, 3 };
  int order2[] = { 3, 0, 1, 2 };
  CtkTreeIter iter1, iter2, iter3, iter4;
  CtkTreePath *path;

  /* Expand row 0 */
  path = ctk_tree_path_new_from_indices (0, -1);
  ctk_tree_view_expand_row (CTK_TREE_VIEW (fixture->tree_view), path, TRUE);

  /* Hide middle path */
  signal_monitor_append_signal (fixture->monitor,
                                ROW_DELETED, "0:0");
  set_path_visibility (fixture, "0:0", FALSE);
  signal_monitor_assert_is_empty (fixture->monitor);

  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store),
                                       &iter1, "0:1");
  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store),
                                       &iter2, "0:2");
  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store),
                                       &iter3, "0:3");
  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store),
                                       &iter4, "0:4");

  signal_monitor_append_signal_reordered (fixture->monitor,
                                          ROWS_REORDERED,
                                          path, order0, 4);
  ctk_tree_store_move_after (fixture->store, &iter1, &iter4);
  signal_monitor_assert_is_empty (fixture->monitor);

  signal_monitor_append_signal_reordered (fixture->monitor,
                                          ROWS_REORDERED,
                                          path, order1, 4);
  ctk_tree_store_move_after (fixture->store, &iter3, &iter4);
  signal_monitor_assert_is_empty (fixture->monitor);

  signal_monitor_append_signal_reordered (fixture->monitor,
                                          ROWS_REORDERED,
                                          path, order2, 4);
  ctk_tree_store_move_before (fixture->store, &iter1, &iter2);
  signal_monitor_assert_is_empty (fixture->monitor);

  ctk_tree_path_free (path);
}

static void
filtered_rows_reordered_child_level_middle_hidden (FilterTest    *fixture,
                                                   gconstpointer  user_data)
{
  int order0[] = { 1, 2, 3, 0 };
  int order1[] = { 0, 2, 1, 3 };
  int order2[] = { 3, 0, 1, 2 };
  CtkTreeIter iter0, iter1, iter3, iter4;
  CtkTreePath *path;

  /* Expand row 0 */
  path = ctk_tree_path_new_from_indices (0, -1);
  ctk_tree_view_expand_row (CTK_TREE_VIEW (fixture->tree_view), path, FALSE);

  /* Hide middle path */
  signal_monitor_append_signal (fixture->monitor,
                                ROW_DELETED, "0:2");
  set_path_visibility (fixture, "0:2", FALSE);
  signal_monitor_assert_is_empty (fixture->monitor);

  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store),
                                       &iter0, "0:0");
  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store),
                                       &iter1, "0:1");
  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store),
                                       &iter3, "0:3");
  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store),
                                       &iter4, "0:4");

  signal_monitor_append_signal_reordered (fixture->monitor,
                                          ROWS_REORDERED,
                                          path, order0, 4);
  ctk_tree_store_move_after (fixture->store, &iter0, &iter4);
  signal_monitor_assert_is_empty (fixture->monitor);

  signal_monitor_append_signal_reordered (fixture->monitor,
                                          ROWS_REORDERED,
                                          path, order1, 4);
  ctk_tree_store_move_after (fixture->store, &iter3, &iter4);
  signal_monitor_assert_is_empty (fixture->monitor);

  signal_monitor_append_signal_reordered (fixture->monitor,
                                          ROWS_REORDERED,
                                          path, order2, 4);
  ctk_tree_store_move_before (fixture->store, &iter0, &iter1);
  signal_monitor_assert_is_empty (fixture->monitor);

  ctk_tree_path_free (path);
}

static void
filtered_rows_reordered_child_level_4_hidden (FilterTest    *fixture,
                                              gconstpointer  user_data)
{
  int order0[] = { 0 };
  CtkTreeIter iter1, iter4;
  CtkTreePath *path;

  /* Expand row 0 */
  path = ctk_tree_path_new_from_indices (0, -1);
  ctk_tree_view_expand_row (CTK_TREE_VIEW (fixture->tree_view), path, FALSE);

  /* Hide last 4 paths */
  signal_monitor_append_signal (fixture->monitor,
                                ROW_DELETED, "0:4");
  signal_monitor_append_signal (fixture->monitor,
                                ROW_DELETED, "0:3");
  signal_monitor_append_signal (fixture->monitor,
                                ROW_DELETED, "0:2");
  signal_monitor_append_signal (fixture->monitor,
                                ROW_DELETED, "0:0");
  set_path_visibility (fixture, "0:4", FALSE);
  set_path_visibility (fixture, "0:3", FALSE);
  set_path_visibility (fixture, "0:2", FALSE);
  set_path_visibility (fixture, "0:0", FALSE);
  signal_monitor_assert_is_empty (fixture->monitor);

  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store),
                                       &iter1, "0:1");
  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store),
                                       &iter4, "0:4");

  signal_monitor_append_signal_reordered (fixture->monitor,
                                          ROWS_REORDERED,
                                          path, order0, 1);
  ctk_tree_store_move_after (fixture->store, &iter1, &iter4);
  signal_monitor_assert_is_empty (fixture->monitor);

  ctk_tree_path_free (path);
}

static void
filtered_rows_reordered_child_level_all_hidden (FilterTest    *fixture,
                                                gconstpointer  user_data)
{
  CtkTreeIter iter1, iter4;
  CtkTreePath *path;

  /* Expand row 0 */
  path = ctk_tree_path_new_from_indices (0, -1);
  ctk_tree_view_expand_row (CTK_TREE_VIEW (fixture->tree_view), path, FALSE);
  ctk_tree_path_free (path);

  /* Hide last 4 paths */
  signal_monitor_append_signal (fixture->monitor,
                                ROW_DELETED, "0:4");
  signal_monitor_append_signal (fixture->monitor,
                                ROW_DELETED, "0:3");
  signal_monitor_append_signal (fixture->monitor,
                                ROW_DELETED, "0:2");
  signal_monitor_append_signal (fixture->monitor,
                                ROW_DELETED, "0:1");
  signal_monitor_append_signal (fixture->monitor,
                                ROW_DELETED, "0:0");
  signal_monitor_append_signal (fixture->monitor,
                                ROW_HAS_CHILD_TOGGLED, "0");
  set_path_visibility (fixture, "0:4", FALSE);
  set_path_visibility (fixture, "0:3", FALSE);
  set_path_visibility (fixture, "0:2", FALSE);
  set_path_visibility (fixture, "0:1", FALSE);
  set_path_visibility (fixture, "0:0", FALSE);
  signal_monitor_assert_is_empty (fixture->monitor);

  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store),
                                       &iter1, "0:1");
  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store),
                                       &iter4, "0:4");

  ctk_tree_store_move_after (fixture->store, &iter1, &iter4);
  signal_monitor_assert_is_empty (fixture->monitor);
}

static void
insert_before (void)
{
  CtkTreeStore *store;
  CtkTreeModel *filter;
  CtkWidget *tree_view;
  SignalMonitor *monitor;
  CtkTreeIter iter;
  CtkTreeIter last_iter;
  CtkTreePath *path;

  /* This tests two aspects of the row-inserted handling:
   *   1) If the newly inserted node was already handled by building
   *      the root level, don't handle it a second time.
   *   2) Offsets of existing nodes must be updated when a new
   *      node is inserted.
   */

  store = ctk_tree_store_new (2, G_TYPE_STRING, G_TYPE_BOOLEAN);
  filter = ctk_tree_model_filter_new (CTK_TREE_MODEL (store), NULL);
  ctk_tree_model_filter_set_visible_column (CTK_TREE_MODEL_FILTER (filter),
                                            1);

  tree_view = ctk_tree_view_new_with_model (filter);
  monitor = signal_monitor_new (filter);

  check_level_length (CTK_TREE_MODEL_FILTER (filter), NULL, 0);

  /* Insert 0 */
  path = ctk_tree_path_new_from_indices (0, -1);
  signal_monitor_append_signal_path (monitor, ROW_INSERTED, path);
  ctk_tree_path_free (path);

  ctk_tree_store_insert_with_values (store, &iter, NULL, 0,
                                     0, "Foo", 1, TRUE, -1);

  signal_monitor_assert_is_empty (monitor);
  check_level_length (CTK_TREE_MODEL_FILTER (filter), NULL, 1);

  /* Insert 1 */
  path = ctk_tree_path_new_from_indices (1, -1);
  signal_monitor_append_signal_path (monitor, ROW_INSERTED, path);
  ctk_tree_path_free (path);

  ctk_tree_store_insert_with_values (store, &iter, NULL, 1,
                                     0, "Foo", 1, TRUE, -1);
  last_iter = iter;

  signal_monitor_assert_is_empty (monitor);
  check_level_length (CTK_TREE_MODEL_FILTER (filter), NULL, 2);

  /* Insert on 1 again -- invisible */
  ctk_tree_store_insert_with_values (store, &iter, NULL, 1,
                                     0, "Foo", 1, FALSE, -1);

  signal_monitor_assert_is_empty (monitor);
  check_level_length (CTK_TREE_MODEL_FILTER (filter), NULL, 2);

  /* Insert on 1 again -- visible */
  path = ctk_tree_path_new_from_indices (1, -1);
  signal_monitor_append_signal_path (monitor, ROW_INSERTED, path);
  ctk_tree_path_free (path);

  ctk_tree_store_insert_with_values (store, &iter, NULL, 1,
                                     0, "Foo", 1, TRUE, -1);

  signal_monitor_assert_is_empty (monitor);
  check_level_length (CTK_TREE_MODEL_FILTER (filter), NULL, 3);

  /* Modify the iter that should be at the last position and check the
   * signal we get.
   */
  path = ctk_tree_path_new_from_indices (2, -1);
  signal_monitor_append_signal_path (monitor, ROW_CHANGED, path);
  ctk_tree_path_free (path);

  ctk_tree_store_set (store, &last_iter, 0, "Foo changed", -1);

  signal_monitor_assert_is_empty (monitor);
  check_level_length (CTK_TREE_MODEL_FILTER (filter), NULL, 3);

  g_object_unref (filter);
  g_object_unref (store);
  ctk_widget_destroy (tree_view);
}

static void
insert_child (void)
{
  CtkTreeStore *store;
  CtkTreeModel *filter;
  CtkWidget *tree_view;
  SignalMonitor *monitor;
  CtkTreeIter parent, iter;
  CtkTreePath *path;

  store = ctk_tree_store_new (2, G_TYPE_STRING, G_TYPE_BOOLEAN);

  ctk_tree_store_insert_with_values (store, &parent, NULL, 0,
                                     0, "Parent", 1, TRUE, -1);


  filter = ctk_tree_model_filter_new (CTK_TREE_MODEL (store), NULL);
  ctk_tree_model_filter_set_visible_column (CTK_TREE_MODEL_FILTER (filter),
                                            1);

  tree_view = ctk_tree_view_new_with_model (filter);
  monitor = signal_monitor_new (filter);

  /* Insert child -- invisible */
  path = ctk_tree_path_new_from_indices (0, -1);
  signal_monitor_append_signal_path (monitor, ROW_HAS_CHILD_TOGGLED, path);
  /* The signal is received twice, once a pass through from CtkTreeStore
   * and one generated by CtkTreeModelFilter.  Not accurate, but cannot
   * hurt.
   */
  signal_monitor_append_signal_path (monitor, ROW_HAS_CHILD_TOGGLED, path);
  ctk_tree_path_free (path);

  ctk_tree_store_insert_with_values (store, &iter, &parent, 1,
                                     0, "Child", 1, FALSE, -1);

  signal_monitor_assert_is_empty (monitor);
  check_level_length (CTK_TREE_MODEL_FILTER (filter), NULL, 1);

  /* Insert child */
  path = ctk_tree_path_new_from_indices (0, 0, -1);
  ctk_tree_path_up (path); /* 0 */
  signal_monitor_append_signal_path (monitor, ROW_HAS_CHILD_TOGGLED, path);
  ctk_tree_path_free (path);

  ctk_tree_store_insert_with_values (store, &iter, &parent, 0,
                                     0, "Child", 1, TRUE, -1);

  signal_monitor_assert_is_empty (monitor);
  check_level_length (CTK_TREE_MODEL_FILTER (filter), NULL, 1);

  /* Insert child -- invisible */
  ctk_tree_store_insert_with_values (store, &iter, &parent, 1,
                                     0, "Child", 1, FALSE, -1);

  signal_monitor_assert_is_empty (monitor);
  check_level_length (CTK_TREE_MODEL_FILTER (filter), NULL, 1);

  g_object_unref (filter);
  g_object_unref (store);
  ctk_widget_destroy (tree_view);
}



static void
remove_node (void)
{
  CtkTreeIter iter, iter1, iter2, iter3;
  CtkListStore *list;
  CtkTreeModel *filter;
  CtkWidget *view G_GNUC_UNUSED;

  list = ctk_list_store_new (1, G_TYPE_INT);
  ctk_list_store_insert_with_values (list, &iter1, 0, 0, 1, -1);
  ctk_list_store_insert_with_values (list, &iter, 1, 0, 2, -1);
  ctk_list_store_insert_with_values (list, &iter, 2, 0, 3, -1);
  ctk_list_store_insert_with_values (list, &iter, 3, 0, 4, -1);
  ctk_list_store_insert_with_values (list, &iter, 4, 0, 5, -1);
  ctk_list_store_insert_with_values (list, &iter, 5, 0, 6, -1);
  ctk_list_store_insert_with_values (list, &iter2, 6, 0, 7, -1);
  ctk_list_store_insert_with_values (list, &iter3, 7, 0, 8, -1);

  filter = ctk_tree_model_filter_new (CTK_TREE_MODEL (list), NULL);
  view = ctk_tree_view_new_with_model (filter);

  ctk_list_store_remove (list, &iter1);
  ctk_list_store_remove (list, &iter3);
  ctk_list_store_remove (list, &iter2);

  ctk_widget_destroy (view);
  g_object_unref (filter);
  g_object_unref (list);
}

static void
remove_node_vroot (void)
{
  CtkTreeIter parent, root;
  CtkTreeIter iter, iter1, iter2, iter3;
  CtkTreeStore *tree;
  CtkTreeModel *filter;
  CtkTreePath *path;
  CtkWidget *view G_GNUC_UNUSED;

  tree = ctk_tree_store_new (1, G_TYPE_INT);
  ctk_tree_store_insert_with_values (tree, &parent, NULL, 0, 0, 0, -1);
  ctk_tree_store_insert_with_values (tree, &root, &parent, 0, 0, 0, -1);

  ctk_tree_store_insert_with_values (tree, &iter1, &root, 0, 0, 1, -1);
  ctk_tree_store_insert_with_values (tree, &iter, &root, 1, 0, 2, -1);
  ctk_tree_store_insert_with_values (tree, &iter, &root, 2, 0, 3, -1);
  ctk_tree_store_insert_with_values (tree, &iter, &root, 3, 0, 4, -1);
  ctk_tree_store_insert_with_values (tree, &iter, &root, 4, 0, 5, -1);
  ctk_tree_store_insert_with_values (tree, &iter, &root, 5, 0, 6, -1);
  ctk_tree_store_insert_with_values (tree, &iter2, &root, 6, 0, 7, -1);
  ctk_tree_store_insert_with_values (tree, &iter3, &root, 7, 0, 8, -1);

  path = ctk_tree_path_new_from_indices (0, 0, -1);
  filter = ctk_tree_model_filter_new (CTK_TREE_MODEL (tree), path);
  ctk_tree_path_free (path);

  view = ctk_tree_view_new_with_model (filter);

  ctk_tree_store_remove (tree, &iter1);
  ctk_tree_store_remove (tree, &iter3);
  ctk_tree_store_remove (tree, &iter2);

  ctk_widget_destroy (view);
  g_object_unref (filter);
  g_object_unref (tree);
}

static void
remove_vroot_ancestor (void)
{
  CtkTreeIter parent, root;
  CtkTreeIter iter, iter1, iter2, iter3;
  CtkTreeStore *tree;
  CtkTreeModel *filter;
  CtkTreePath *path;
  CtkWidget *view G_GNUC_UNUSED;

  tree = ctk_tree_store_new (1, G_TYPE_INT);
  ctk_tree_store_insert_with_values (tree, &parent, NULL, 0, 0, 0, -1);
  ctk_tree_store_insert_with_values (tree, &root, &parent, 0, 0, 0, -1);

  ctk_tree_store_insert_with_values (tree, &iter1, &root, 0, 0, 1, -1);
  ctk_tree_store_insert_with_values (tree, &iter, &root, 1, 0, 2, -1);
  ctk_tree_store_insert_with_values (tree, &iter, &root, 2, 0, 3, -1);
  ctk_tree_store_insert_with_values (tree, &iter, &root, 3, 0, 4, -1);
  ctk_tree_store_insert_with_values (tree, &iter, &root, 4, 0, 5, -1);
  ctk_tree_store_insert_with_values (tree, &iter, &root, 5, 0, 6, -1);
  ctk_tree_store_insert_with_values (tree, &iter2, &root, 6, 0, 7, -1);
  ctk_tree_store_insert_with_values (tree, &iter3, &root, 7, 0, 8, -1);

  path = ctk_tree_path_new_from_indices (0, 0, -1);
  filter = ctk_tree_model_filter_new (CTK_TREE_MODEL (tree), path);
  ctk_tree_path_free (path);

  view = ctk_tree_view_new_with_model (filter);

  ctk_tree_store_remove (tree, &parent);

  ctk_widget_destroy (view);
  g_object_unref (filter);
  g_object_unref (tree);
}

static void
ref_count_single_level (void)
{
  CtkTreeIter iter[5];
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkTreeModel *filter_model;
  CtkWidget *tree_view;

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  ctk_tree_store_append (CTK_TREE_STORE (model), &iter[0], NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter[1], NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter[2], NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter[3], NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter[4], NULL);

  assert_root_level_unreferenced (ref_model);

  filter_model = ctk_tree_model_filter_new (model, NULL);
  tree_view = ctk_tree_view_new_with_model (filter_model);

  assert_node_ref_count (ref_model, &iter[0], 2);
  assert_node_ref_count (ref_model, &iter[1], 1);
  assert_node_ref_count (ref_model, &iter[2], 1);
  assert_node_ref_count (ref_model, &iter[3], 1);
  assert_node_ref_count (ref_model, &iter[4], 1);

  ctk_widget_destroy (tree_view);

  assert_node_ref_count (ref_model, &iter[0], 1);
  assert_node_ref_count (ref_model, &iter[1], 0);
  assert_node_ref_count (ref_model, &iter[2], 0);
  assert_node_ref_count (ref_model, &iter[3], 0);
  assert_node_ref_count (ref_model, &iter[4], 0);

  g_object_unref (filter_model);

  assert_node_ref_count (ref_model, &iter[0], 0);

  g_object_unref (ref_model);
}

static void
ref_count_two_levels (void)
{
  CtkTreeIter parent1, parent2, iter, iter_first;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkTreeModel *filter_model;
  CtkWidget *tree_view;

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  ctk_tree_store_append (CTK_TREE_STORE (model), &parent1, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &parent2, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter_first, &parent2);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, &parent2);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter, &parent2);

  assert_entire_model_unreferenced (ref_model);

  filter_model = ctk_tree_model_filter_new (model, NULL);
  tree_view = ctk_tree_view_new_with_model (filter_model);

  /* This is quite confusing:
   *  - node 0 has a ref count of 2 because it is referenced as the
   *    first node in a level and by the tree view.
   *  - node 1 has a ref count of 2 because it is referenced by its
   *    child level and by the tree view.
   */
  assert_root_level_referenced (ref_model, 2);
  assert_node_ref_count (ref_model, &iter_first, 1);
  assert_node_ref_count (ref_model, &iter, 0);

  ctk_tree_view_expand_all (CTK_TREE_VIEW (tree_view));

  assert_node_ref_count (ref_model, &parent1, 2);
  assert_node_ref_count (ref_model, &parent2, 2);
  assert_node_ref_count (ref_model, &iter_first, 2);
  assert_node_ref_count (ref_model, &iter, 1);

  ctk_tree_view_collapse_all (CTK_TREE_VIEW (tree_view));

  /* The child level is not destroyed because its parent is visible */
  assert_node_ref_count (ref_model, &parent1, 2);
  assert_node_ref_count (ref_model, &parent2, 2);
  assert_node_ref_count (ref_model, &iter_first, 1);
  assert_node_ref_count (ref_model, &iter, 0);

  ctk_tree_model_filter_clear_cache (CTK_TREE_MODEL_FILTER (filter_model));

  assert_node_ref_count (ref_model, &parent1, 2);
  assert_node_ref_count (ref_model, &parent2, 2);
  assert_node_ref_count (ref_model, &iter_first, 1);
  assert_node_ref_count (ref_model, &iter, 0);

  ctk_widget_destroy (tree_view);

  assert_root_level_referenced (ref_model, 1);
  assert_node_ref_count (ref_model, &iter_first, 1);
  assert_node_ref_count (ref_model, &iter, 0);

  ctk_tree_model_filter_clear_cache (CTK_TREE_MODEL_FILTER (filter_model));

  /* The root level and first level remain cached, only the references on the
   * first nodes of these levels are kept.
   */
  assert_node_ref_count (ref_model, &parent1, 1);
  assert_node_ref_count (ref_model, &parent2, 1);
  assert_node_ref_count (ref_model, &iter_first, 1);
  assert_node_ref_count (ref_model, &iter, 0);

  g_object_unref (filter_model);
  g_object_unref (ref_model);
}

static void
ref_count_three_levels (void)
{
  CtkTreeIter grandparent1, grandparent2, parent1, parent2;
  CtkTreeIter iter_parent1, iter_parent2, iter_parent2_first;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkTreeModel *filter_model;
  CtkTreePath *path;
  CtkWidget *tree_view;

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  /* + grandparent1
   * + grandparent2
   *   + parent1
   *     + iter_parent1
   *   + parent2
   *     + iter_parent2_first
   *     + iter_parent2
   */

  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent1, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent2, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &parent1, &grandparent2);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter_parent1, &parent1);
  ctk_tree_store_append (CTK_TREE_STORE (model), &parent2, &grandparent2);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter_parent2_first, &parent2);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter_parent2, &parent2);

  assert_entire_model_unreferenced (ref_model);

  filter_model = ctk_tree_model_filter_new (model, NULL);
  tree_view = ctk_tree_view_new_with_model (filter_model);

  /* This is quite confusing:
   *  - node 0 has a ref count of 2 because it is referenced as the
   *    first node in a level and by the tree view.
   *  - node 1 has a ref count of 2 because it is referenced by its
   *    child level and by the tree view.
   */
  assert_root_level_referenced (ref_model, 2);
  assert_node_ref_count (ref_model, &parent1, 1);
  assert_node_ref_count (ref_model, &parent2, 0);
  assert_level_unreferenced (ref_model, &parent1);
  assert_level_unreferenced (ref_model, &parent2);

  path = ctk_tree_path_new_from_indices (1, -1);
  ctk_tree_view_expand_row (CTK_TREE_VIEW (tree_view), path, FALSE);

  assert_node_ref_count (ref_model, &grandparent1, 2);
  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &parent1, 3);
  assert_node_ref_count (ref_model, &parent2, 2);
  assert_node_ref_count (ref_model, &iter_parent1, 1);
  assert_node_ref_count (ref_model, &iter_parent2_first, 1);
  assert_node_ref_count (ref_model, &iter_parent2, 0);

  ctk_tree_view_expand_row (CTK_TREE_VIEW (tree_view), path, TRUE);

  assert_node_ref_count (ref_model, &grandparent1, 2);
  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &parent1, 3);
  assert_node_ref_count (ref_model, &parent2, 2);
  assert_node_ref_count (ref_model, &iter_parent1, 2);
  assert_node_ref_count (ref_model, &iter_parent2_first, 2);
  assert_node_ref_count (ref_model, &iter_parent2, 1);

  ctk_tree_view_collapse_all (CTK_TREE_VIEW (tree_view));

  assert_node_ref_count (ref_model, &grandparent1, 2);
  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &parent1, 2);
  assert_node_ref_count (ref_model, &parent2, 1);
  assert_node_ref_count (ref_model, &iter_parent1, 1);
  assert_node_ref_count (ref_model, &iter_parent2_first, 1);
  assert_node_ref_count (ref_model, &iter_parent2, 0);

  ctk_tree_model_filter_clear_cache (CTK_TREE_MODEL_FILTER (filter_model));

  assert_node_ref_count (ref_model, &grandparent1, 2);
  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &parent1, 1);
  assert_node_ref_count (ref_model, &parent2, 0);
  assert_node_ref_count (ref_model, &iter_parent1, 0);
  assert_node_ref_count (ref_model, &iter_parent2_first, 0);
  assert_node_ref_count (ref_model, &iter_parent2, 0);

  ctk_tree_view_expand_row (CTK_TREE_VIEW (tree_view), path, FALSE);

  assert_node_ref_count (ref_model, &grandparent1, 2);
  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &parent1, 3);
  assert_node_ref_count (ref_model, &parent2, 2);
  assert_node_ref_count (ref_model, &iter_parent1, 1);
  assert_node_ref_count (ref_model, &iter_parent2_first, 1);
  assert_node_ref_count (ref_model, &iter_parent2, 0);

  ctk_tree_path_append_index (path, 1);
  ctk_tree_view_expand_row (CTK_TREE_VIEW (tree_view), path, FALSE);

  assert_node_ref_count (ref_model, &grandparent1, 2);
  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &parent1, 3);
  assert_node_ref_count (ref_model, &parent2, 2);
  assert_node_ref_count (ref_model, &iter_parent1, 1);
  assert_node_ref_count (ref_model, &iter_parent2_first, 2);
  assert_node_ref_count (ref_model, &iter_parent2, 1);

  ctk_tree_view_collapse_row (CTK_TREE_VIEW (tree_view), path);

  assert_node_ref_count (ref_model, &grandparent1, 2);
  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &parent1, 3);
  assert_node_ref_count (ref_model, &parent2, 2);
  assert_node_ref_count (ref_model, &iter_parent1, 1);
  assert_node_ref_count (ref_model, &iter_parent2_first, 1);
  assert_node_ref_count (ref_model, &iter_parent2, 0);

  ctk_tree_model_filter_clear_cache (CTK_TREE_MODEL_FILTER (filter_model));

  assert_node_ref_count (ref_model, &grandparent1, 2);
  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &parent1, 3);
  assert_node_ref_count (ref_model, &parent2, 2);
  assert_node_ref_count (ref_model, &iter_parent1, 1);
  assert_node_ref_count (ref_model, &iter_parent2_first, 1);
  assert_node_ref_count (ref_model, &iter_parent2, 0);

  ctk_tree_path_up (path);
  ctk_tree_view_collapse_row (CTK_TREE_VIEW (tree_view), path);
  ctk_tree_path_free (path);

  assert_node_ref_count (ref_model, &grandparent1, 2);
  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &parent1, 2);
  assert_node_ref_count (ref_model, &parent2, 1);
  assert_node_ref_count (ref_model, &iter_parent1, 1);
  assert_node_ref_count (ref_model, &iter_parent2_first, 1);
  assert_node_ref_count (ref_model, &iter_parent2, 0);

  ctk_tree_model_filter_clear_cache (CTK_TREE_MODEL_FILTER (filter_model));

  assert_node_ref_count (ref_model, &grandparent1, 2);
  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &parent1, 1);
  assert_node_ref_count (ref_model, &parent2, 0);
  assert_node_ref_count (ref_model, &iter_parent1, 0);
  assert_node_ref_count (ref_model, &iter_parent2_first, 0);
  assert_node_ref_count (ref_model, &iter_parent2, 0);

  ctk_widget_destroy (tree_view);

  ctk_tree_model_filter_clear_cache (CTK_TREE_MODEL_FILTER (filter_model));

  /* The root level and first level remain cached, only the references on the
   * first nodes of these levels are kept.  Grandparent2 is the parent
   * of the first level with parent1, so grandparent2 keeps a reference
   * as well.
   */
  assert_node_ref_count (ref_model, &grandparent1, 1);
  assert_node_ref_count (ref_model, &grandparent2, 1);
  assert_node_ref_count (ref_model, &parent1, 1);
  assert_node_ref_count (ref_model, &parent2, 0);
  assert_node_ref_count (ref_model, &iter_parent1, 0);
  assert_node_ref_count (ref_model, &iter_parent2_first, 0);
  assert_node_ref_count (ref_model, &iter_parent2, 0);

  g_object_unref (filter_model);
  g_object_unref (ref_model);
}

static void
ref_count_delete_row (void)
{
  CtkTreeIter grandparent1, grandparent2, parent1, parent2;
  CtkTreeIter iter_parent1, iter_parent2, iter_parent2_first;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkTreeModel *filter_model;
  CtkTreePath *path;
  CtkWidget *tree_view;

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  /* + grandparent1
   * + grandparent2
   *   + parent1
   *     + iter_parent1
   *   + parent2
   *     + iter_parent2_first
   *     + iter_parent2
   */

  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent1, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent2, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &parent1, &grandparent2);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter_parent1, &parent1);
  ctk_tree_store_append (CTK_TREE_STORE (model), &parent2, &grandparent2);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter_parent2_first, &parent2);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter_parent2, &parent2);

  assert_entire_model_unreferenced (ref_model);

  filter_model = ctk_tree_model_filter_new (model, NULL);
  tree_view = ctk_tree_view_new_with_model (filter_model);

  assert_root_level_referenced (ref_model, 2);
  assert_node_ref_count (ref_model, &parent1, 1);
  assert_node_ref_count (ref_model, &parent2, 0);
  assert_level_unreferenced (ref_model, &parent1);
  assert_level_unreferenced (ref_model, &parent2);

  path = ctk_tree_path_new_from_indices (1, -1);
  ctk_tree_view_expand_row (CTK_TREE_VIEW (tree_view), path, TRUE);
  ctk_tree_path_free (path);

  assert_node_ref_count (ref_model, &grandparent1, 2);
  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &parent1, 3);
  assert_node_ref_count (ref_model, &parent2, 2);
  assert_node_ref_count (ref_model, &iter_parent1, 2);
  assert_node_ref_count (ref_model, &iter_parent2_first, 2);
  assert_node_ref_count (ref_model, &iter_parent2, 1);

  ctk_tree_store_remove (CTK_TREE_STORE (model), &iter_parent2);

  assert_node_ref_count (ref_model, &grandparent1, 2);
  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &parent1, 3);
  assert_node_ref_count (ref_model, &parent2, 2);
  assert_node_ref_count (ref_model, &iter_parent1, 2);
  assert_node_ref_count (ref_model, &iter_parent2_first, 2);

  ctk_tree_store_remove (CTK_TREE_STORE (model), &parent1);

  assert_node_ref_count (ref_model, &grandparent1, 2);
  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &parent2, 3);
  assert_level_referenced (ref_model, 2, &parent2);

  ctk_tree_store_remove (CTK_TREE_STORE (model), &grandparent2);

  assert_node_ref_count (ref_model, &grandparent1, 2);

  ctk_tree_model_filter_clear_cache (CTK_TREE_MODEL_FILTER (filter_model));

  assert_node_ref_count (ref_model, &grandparent1, 2);

  ctk_widget_destroy (tree_view);
  ctk_tree_model_filter_clear_cache (CTK_TREE_MODEL_FILTER (filter_model));

  assert_node_ref_count (ref_model, &grandparent1, 1);

  g_object_unref (filter_model);

  assert_node_ref_count (ref_model, &grandparent1, 0);

  g_object_unref (ref_model);
}

static void
ref_count_filter_row_length_1 (void)
{
  CtkTreeIter level1_1;
  CtkTreeIter level2_1;
  CtkTreeIter level3_1;
  CtkTreeIter level4_1;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkTreeModel *filter_model;
  CtkTreePath *path;
  CtkWidget *tree_view;
  GType column_types[] = { G_TYPE_BOOLEAN };

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  ctk_tree_store_set_column_types (CTK_TREE_STORE (model), 1,
                                   column_types);


  /* + level1_1
   *   + level2_1
   *     + level3_1
   *       + level4_1
   *
   * Node level1_1 is expanded.  This makes that levels 1 and 2 are
   * visible.  Level 3 is cached because its parent is visible.  Level 4
   * is not cached.
   */

  ctk_tree_store_append (CTK_TREE_STORE (model), &level1_1, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &level2_1, &level1_1);
  ctk_tree_store_append (CTK_TREE_STORE (model), &level3_1, &level2_1);
  ctk_tree_store_append (CTK_TREE_STORE (model), &level4_1, &level3_1);

  ctk_tree_store_set (CTK_TREE_STORE (model), &level1_1, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &level2_1, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &level3_1, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &level4_1, 0, TRUE, -1);

  assert_entire_model_unreferenced (ref_model);

  filter_model = ctk_tree_model_filter_new (model, NULL);
  ctk_tree_model_filter_set_visible_column (CTK_TREE_MODEL_FILTER (filter_model), 0);
  tree_view = ctk_tree_view_new_with_model (filter_model);

  assert_node_ref_count (ref_model, &level1_1, 3);
  assert_node_ref_count (ref_model, &level2_1, 1);
  assert_node_ref_count (ref_model, &level3_1, 0);
  assert_node_ref_count (ref_model, &level4_1, 0);

  path = ctk_tree_path_new_from_indices (0, -1);
  ctk_tree_view_expand_row (CTK_TREE_VIEW (tree_view), path, FALSE);
  ctk_tree_path_free (path);

  assert_node_ref_count (ref_model, &level1_1, 3);
  assert_node_ref_count (ref_model, &level2_1, 3);
  assert_node_ref_count (ref_model, &level3_1, 1);
  assert_node_ref_count (ref_model, &level4_1, 0);

  ctk_tree_store_set (CTK_TREE_STORE (model), &level4_1, 0, FALSE, -1);

  assert_node_ref_count (ref_model, &level1_1, 3);
  assert_node_ref_count (ref_model, &level2_1, 3);
  assert_node_ref_count (ref_model, &level3_1, 1);
  assert_node_ref_count (ref_model, &level4_1, 0);

  /* level3_1 has a visible parent, so the node is kept in the cache. */
  ctk_tree_store_set (CTK_TREE_STORE (model), &level3_1, 0, FALSE, -1);

  assert_node_ref_count (ref_model, &level1_1, 3);
  assert_node_ref_count (ref_model, &level2_1, 3);
  assert_node_ref_count (ref_model, &level3_1, 1);
  assert_node_ref_count (ref_model, &level4_1, 0);

  /* level2_1 has a visible parent, so is kept in the cache.  However,
   * the external reference should be released.
   */
  ctk_tree_store_set (CTK_TREE_STORE (model), &level2_1, 0, FALSE, -1);

  assert_node_ref_count (ref_model, &level1_1, 3);
  assert_node_ref_count (ref_model, &level2_1, 1);
  assert_node_ref_count (ref_model, &level3_1, 0);
  assert_node_ref_count (ref_model, &level4_1, 0);

  ctk_tree_store_set (CTK_TREE_STORE (model), &level1_1, 0, FALSE, -1);

  assert_node_ref_count (ref_model, &level1_1, 2);
  assert_node_ref_count (ref_model, &level2_1, 1);
  assert_node_ref_count (ref_model, &level3_1, 0);
  assert_node_ref_count (ref_model, &level4_1, 0);

  ctk_widget_destroy (tree_view);
  ctk_tree_model_filter_clear_cache (CTK_TREE_MODEL_FILTER (filter_model));

  assert_node_ref_count (ref_model, &level1_1, 2);
  assert_node_ref_count (ref_model, &level2_1, 1);

  g_object_unref (filter_model);

  assert_node_ref_count (ref_model, &level1_1, 0);

  g_object_unref (ref_model);
}

static void
ref_count_filter_row_length_1_remove_in_root_level (void)
{
  CtkTreeIter level1_1;
  CtkTreeIter level2_1;
  CtkTreeIter level3_1;
  CtkTreeIter level4_1;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkTreeModel *filter_model;
  CtkTreePath *path;
  CtkWidget *tree_view;
  GType column_types[] = { G_TYPE_BOOLEAN };

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  ctk_tree_store_set_column_types (CTK_TREE_STORE (model), 1,
                                   column_types);


  /* + level1_1
   *   + level2_1
   *     + level3_1
   *       + level4_1
   */

  ctk_tree_store_append (CTK_TREE_STORE (model), &level1_1, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &level2_1, &level1_1);
  ctk_tree_store_append (CTK_TREE_STORE (model), &level3_1, &level2_1);
  ctk_tree_store_append (CTK_TREE_STORE (model), &level4_1, &level3_1);

  ctk_tree_store_set (CTK_TREE_STORE (model), &level1_1, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &level2_1, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &level3_1, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &level4_1, 0, TRUE, -1);

  assert_entire_model_unreferenced (ref_model);

  filter_model = ctk_tree_model_filter_new (model, NULL);
  ctk_tree_model_filter_set_visible_column (CTK_TREE_MODEL_FILTER (filter_model), 0);
  tree_view = ctk_tree_view_new_with_model (filter_model);

  assert_node_ref_count (ref_model, &level1_1, 3);
  assert_node_ref_count (ref_model, &level2_1, 1);
  assert_node_ref_count (ref_model, &level3_1, 0);
  assert_node_ref_count (ref_model, &level4_1, 0);

  path = ctk_tree_path_new_from_indices (0, -1);
  ctk_tree_view_expand_row (CTK_TREE_VIEW (tree_view), path, TRUE);
  ctk_tree_path_free (path);

  assert_node_ref_count (ref_model, &level1_1, 3);
  assert_node_ref_count (ref_model, &level2_1, 3);
  assert_node_ref_count (ref_model, &level3_1, 3);
  assert_node_ref_count (ref_model, &level4_1, 2);

  ctk_tree_store_set (CTK_TREE_STORE (model), &level1_1, 0, FALSE, -1);

  assert_node_ref_count (ref_model, &level1_1, 2);
  assert_node_ref_count (ref_model, &level2_1, 1);
  assert_node_ref_count (ref_model, &level3_1, 0);
  assert_node_ref_count (ref_model, &level4_1, 0);

  ctk_widget_destroy (tree_view);
  ctk_tree_model_filter_clear_cache (CTK_TREE_MODEL_FILTER (filter_model));

  assert_node_ref_count (ref_model, &level1_1, 2);
  assert_node_ref_count (ref_model, &level2_1, 1);
  assert_node_ref_count (ref_model, &level3_1, 0);
  assert_node_ref_count (ref_model, &level4_1, 0);

  g_object_unref (filter_model);

  assert_node_ref_count (ref_model, &level1_1, 0);
  assert_node_ref_count (ref_model, &level2_1, 0);
  assert_node_ref_count (ref_model, &level3_1, 0);
  assert_node_ref_count (ref_model, &level4_1, 0);

  g_object_unref (ref_model);
}

static void
ref_count_filter_row_length_1_remove_in_child_level (void)
{
  CtkTreeIter level1_1;
  CtkTreeIter level2_1;
  CtkTreeIter level3_1;
  CtkTreeIter level4_1;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkTreeModel *filter_model;
  CtkTreePath *path;
  CtkWidget *tree_view;
  GType column_types[] = { G_TYPE_BOOLEAN };

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  ctk_tree_store_set_column_types (CTK_TREE_STORE (model), 1,
                                   column_types);


  /* + level1_1
   *   + level2_1
   *     + level3_1
   *       + level4_1
   */

  ctk_tree_store_append (CTK_TREE_STORE (model), &level1_1, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &level2_1, &level1_1);
  ctk_tree_store_append (CTK_TREE_STORE (model), &level3_1, &level2_1);
  ctk_tree_store_append (CTK_TREE_STORE (model), &level4_1, &level3_1);

  ctk_tree_store_set (CTK_TREE_STORE (model), &level1_1, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &level2_1, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &level3_1, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &level4_1, 0, TRUE, -1);

  assert_entire_model_unreferenced (ref_model);

  filter_model = ctk_tree_model_filter_new (model, NULL);
  ctk_tree_model_filter_set_visible_column (CTK_TREE_MODEL_FILTER (filter_model), 0);
  tree_view = ctk_tree_view_new_with_model (filter_model);

  assert_node_ref_count (ref_model, &level1_1, 3);
  assert_node_ref_count (ref_model, &level2_1, 1);
  assert_node_ref_count (ref_model, &level3_1, 0);
  assert_node_ref_count (ref_model, &level4_1, 0);

  path = ctk_tree_path_new_from_indices (0, -1);
  ctk_tree_view_expand_row (CTK_TREE_VIEW (tree_view), path, TRUE);
  ctk_tree_path_free (path);

  assert_node_ref_count (ref_model, &level1_1, 3);
  assert_node_ref_count (ref_model, &level2_1, 3);
  assert_node_ref_count (ref_model, &level3_1, 3);
  assert_node_ref_count (ref_model, &level4_1, 2);

  ctk_tree_store_set (CTK_TREE_STORE (model), &level2_1, 0, FALSE, -1);

  assert_node_ref_count (ref_model, &level1_1, 3);
  assert_node_ref_count (ref_model, &level2_1, 1);
  assert_node_ref_count (ref_model, &level3_1, 0);
  assert_node_ref_count (ref_model, &level4_1, 0);

  ctk_widget_destroy (tree_view);
  ctk_tree_model_filter_clear_cache (CTK_TREE_MODEL_FILTER (filter_model));

  assert_node_ref_count (ref_model, &level1_1, 2);
  assert_node_ref_count (ref_model, &level2_1, 1);
  assert_node_ref_count (ref_model, &level3_1, 0);
  assert_node_ref_count (ref_model, &level4_1, 0);

  g_object_unref (filter_model);

  assert_node_ref_count (ref_model, &level1_1, 0);
  assert_node_ref_count (ref_model, &level2_1, 0);
  assert_node_ref_count (ref_model, &level3_1, 0);
  assert_node_ref_count (ref_model, &level4_1, 0);

  g_object_unref (ref_model);
}

static void
ref_count_filter_row_length_gt_1 (void)
{
  CtkTreeIter level1_1, level1_2;
  CtkTreeIter level2_1, level2_2;
  CtkTreeIter level3_1, level3_2;
  CtkTreeIter level4_1, level4_2;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkTreeModel *filter_model;
  CtkTreePath *path;
  CtkWidget *tree_view;
  GType column_types[] = { G_TYPE_BOOLEAN };

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  ctk_tree_store_set_column_types (CTK_TREE_STORE (model), 1,
                                   column_types);


  /* + level1_1
   * + level1_2
   *   + level2_1
   *   + level2_2
   *     + level3_1
   *     + level3_2
   *       + level4_1
   *       + level4_2
   *
   * Node level1_2 is expanded.  This makes that levels 1 and 2 are
   * visible.  Level 3 is cached because its parent is visible.  Level 4
   * is not cached.
   */

  ctk_tree_store_append (CTK_TREE_STORE (model), &level1_1, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &level1_2, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &level2_1, &level1_2);
  ctk_tree_store_append (CTK_TREE_STORE (model), &level2_2, &level1_2);
  ctk_tree_store_append (CTK_TREE_STORE (model), &level3_1, &level2_2);
  ctk_tree_store_append (CTK_TREE_STORE (model), &level3_2, &level2_2);
  ctk_tree_store_append (CTK_TREE_STORE (model), &level4_1, &level3_2);
  ctk_tree_store_append (CTK_TREE_STORE (model), &level4_2, &level3_2);

  ctk_tree_store_set (CTK_TREE_STORE (model), &level1_1, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &level1_2, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &level2_1, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &level2_2, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &level3_1, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &level3_2, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &level4_1, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &level4_2, 0, TRUE, -1);

  assert_entire_model_unreferenced (ref_model);

  filter_model = ctk_tree_model_filter_new (model, NULL);
  ctk_tree_model_filter_set_visible_column (CTK_TREE_MODEL_FILTER (filter_model), 0);
  tree_view = ctk_tree_view_new_with_model (filter_model);

  assert_node_ref_count (ref_model, &level1_1, 2);
  assert_node_ref_count (ref_model, &level1_2, 2);
  assert_node_ref_count (ref_model, &level2_1, 1);
  assert_node_ref_count (ref_model, &level2_2, 0);
  assert_node_ref_count (ref_model, &level3_1, 0);
  assert_node_ref_count (ref_model, &level3_2, 0);
  assert_node_ref_count (ref_model, &level4_1, 0);
  assert_node_ref_count (ref_model, &level4_2, 0);

  path = ctk_tree_path_new_from_indices (1, -1);
  ctk_tree_view_expand_row (CTK_TREE_VIEW (tree_view), path, FALSE);
  ctk_tree_path_free (path);

  assert_node_ref_count (ref_model, &level1_1, 2);
  assert_node_ref_count (ref_model, &level1_2, 2);
  assert_node_ref_count (ref_model, &level2_1, 2);
  assert_node_ref_count (ref_model, &level2_2, 2);
  assert_node_ref_count (ref_model, &level3_1, 1);
  assert_node_ref_count (ref_model, &level3_2, 0);
  assert_node_ref_count (ref_model, &level4_1, 0);
  assert_node_ref_count (ref_model, &level4_2, 0);

  ctk_tree_store_set (CTK_TREE_STORE (model), &level4_1, 0, FALSE, -1);

  assert_node_ref_count (ref_model, &level1_1, 2);
  assert_node_ref_count (ref_model, &level1_2, 2);
  assert_node_ref_count (ref_model, &level2_1, 2);
  assert_node_ref_count (ref_model, &level2_2, 2);
  assert_node_ref_count (ref_model, &level3_1, 1);
  assert_node_ref_count (ref_model, &level3_2, 0);
  assert_node_ref_count (ref_model, &level4_1, 0);
  assert_node_ref_count (ref_model, &level4_2, 0);

  ctk_tree_store_set (CTK_TREE_STORE (model), &level3_1, 0, FALSE, -1);

  assert_node_ref_count (ref_model, &level1_1, 2);
  assert_node_ref_count (ref_model, &level1_2, 2);
  assert_node_ref_count (ref_model, &level2_1, 2);
  assert_node_ref_count (ref_model, &level2_2, 2);
  assert_node_ref_count (ref_model, &level3_1, 0);
  assert_node_ref_count (ref_model, &level3_2, 1);
  assert_node_ref_count (ref_model, &level4_1, 0);
  assert_node_ref_count (ref_model, &level4_2, 0);

  ctk_tree_store_set (CTK_TREE_STORE (model), &level2_2, 0, FALSE, -1);

  assert_node_ref_count (ref_model, &level1_1, 2);
  assert_node_ref_count (ref_model, &level1_2, 2);
  assert_node_ref_count (ref_model, &level2_1, 2);
  assert_node_ref_count (ref_model, &level2_2, 0);
  assert_node_ref_count (ref_model, &level3_1, 0);
  assert_node_ref_count (ref_model, &level3_2, 0);
  assert_node_ref_count (ref_model, &level4_1, 0);
  assert_node_ref_count (ref_model, &level4_2, 0);

  ctk_tree_store_set (CTK_TREE_STORE (model), &level1_2, 0, FALSE, -1);

  assert_node_ref_count (ref_model, &level1_1, 2);
  assert_node_ref_count (ref_model, &level1_2, 0);
  assert_node_ref_count (ref_model, &level2_1, 0);
  assert_node_ref_count (ref_model, &level2_2, 0);
  assert_node_ref_count (ref_model, &level3_1, 0);
  assert_node_ref_count (ref_model, &level3_2, 0);
  assert_node_ref_count (ref_model, &level4_1, 0);
  assert_node_ref_count (ref_model, &level4_2, 0);

  ctk_widget_destroy (tree_view);
  ctk_tree_model_filter_clear_cache (CTK_TREE_MODEL_FILTER (filter_model));

  assert_node_ref_count (ref_model, &level1_1, 1);

  g_object_unref (filter_model);

  assert_node_ref_count (ref_model, &level1_1, 0);

  g_object_unref (ref_model);
}

static void
ref_count_filter_row_length_gt_1_visible_children (void)
{
  CtkTreeIter level1_1, level1_2;
  CtkTreeIter level2_1, level2_2;
  CtkTreeIter level3_1, level3_2;
  CtkTreeIter level4_1, level4_2;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkTreeModel *filter_model;
  CtkTreePath *path;
  CtkWidget *tree_view;
  GType column_types[] = { G_TYPE_BOOLEAN };

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  ctk_tree_store_set_column_types (CTK_TREE_STORE (model), 1,
                                   column_types);


  /* + level1_1
   * + level1_2
   *   + level2_1
   *   + level2_2
   *     + level3_1
   *     + level3_2
   *       + level4_1
   *       + level4_2
   */

  ctk_tree_store_append (CTK_TREE_STORE (model), &level1_1, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &level1_2, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &level2_1, &level1_2);
  ctk_tree_store_append (CTK_TREE_STORE (model), &level2_2, &level1_2);
  ctk_tree_store_append (CTK_TREE_STORE (model), &level3_1, &level2_2);
  ctk_tree_store_append (CTK_TREE_STORE (model), &level3_2, &level2_2);
  ctk_tree_store_append (CTK_TREE_STORE (model), &level4_1, &level3_2);
  ctk_tree_store_append (CTK_TREE_STORE (model), &level4_2, &level3_2);

  ctk_tree_store_set (CTK_TREE_STORE (model), &level1_1, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &level1_2, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &level2_1, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &level2_2, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &level3_1, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &level3_2, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &level4_1, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &level4_2, 0, TRUE, -1);

  assert_entire_model_unreferenced (ref_model);

  filter_model = ctk_tree_model_filter_new (model, NULL);
  ctk_tree_model_filter_set_visible_column (CTK_TREE_MODEL_FILTER (filter_model), 0);
  tree_view = ctk_tree_view_new_with_model (filter_model);

  assert_node_ref_count (ref_model, &level1_1, 2);
  assert_node_ref_count (ref_model, &level1_2, 2);
  assert_node_ref_count (ref_model, &level2_1, 1);
  assert_node_ref_count (ref_model, &level2_2, 0);
  assert_node_ref_count (ref_model, &level3_1, 0);
  assert_node_ref_count (ref_model, &level3_2, 0);
  assert_node_ref_count (ref_model, &level4_1, 0);
  assert_node_ref_count (ref_model, &level4_2, 0);

  path = ctk_tree_path_new_from_indices (1, -1);
  ctk_tree_view_expand_row (CTK_TREE_VIEW (tree_view), path, TRUE);
  ctk_tree_path_free (path);

  assert_node_ref_count (ref_model, &level1_1, 2);
  assert_node_ref_count (ref_model, &level1_2, 2);
  assert_node_ref_count (ref_model, &level2_1, 2);
  assert_node_ref_count (ref_model, &level2_2, 2);
  assert_node_ref_count (ref_model, &level3_1, 2);
  assert_node_ref_count (ref_model, &level3_2, 2);
  assert_node_ref_count (ref_model, &level4_1, 2);
  assert_node_ref_count (ref_model, &level4_2, 1);

  ctk_tree_store_set (CTK_TREE_STORE (model), &level2_2, 0, FALSE, -1);

  assert_node_ref_count (ref_model, &level1_1, 2);
  assert_node_ref_count (ref_model, &level1_2, 2);
  assert_node_ref_count (ref_model, &level2_1, 2);
  assert_node_ref_count (ref_model, &level2_2, 0);
  assert_node_ref_count (ref_model, &level3_1, 0);
  assert_node_ref_count (ref_model, &level3_2, 0);
  assert_node_ref_count (ref_model, &level4_1, 0);
  assert_node_ref_count (ref_model, &level4_2, 0);

  ctk_widget_destroy (tree_view);
  ctk_tree_model_filter_clear_cache (CTK_TREE_MODEL_FILTER (filter_model));

  assert_node_ref_count (ref_model, &level1_1, 1);
  assert_node_ref_count (ref_model, &level1_2, 1);
  assert_node_ref_count (ref_model, &level2_1, 1);
  assert_node_ref_count (ref_model, &level2_2, 0);
  assert_node_ref_count (ref_model, &level3_1, 0);
  assert_node_ref_count (ref_model, &level3_2, 0);
  assert_node_ref_count (ref_model, &level4_1, 0);
  assert_node_ref_count (ref_model, &level4_2, 0);

  g_object_unref (filter_model);

  assert_node_ref_count (ref_model, &level1_1, 0);
  assert_node_ref_count (ref_model, &level1_2, 0);
  assert_node_ref_count (ref_model, &level2_1, 0);
  assert_node_ref_count (ref_model, &level2_2, 0);
  assert_node_ref_count (ref_model, &level3_1, 0);
  assert_node_ref_count (ref_model, &level3_2, 0);
  assert_node_ref_count (ref_model, &level4_1, 0);
  assert_node_ref_count (ref_model, &level4_2, 0);

  g_object_unref (ref_model);
}

static void
ref_count_cleanup (void)
{
  CtkTreeIter grandparent1, grandparent2, parent1, parent2;
  CtkTreeIter iter_parent1, iter_parent2, iter_parent2_first;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkTreeModel *filter_model;
  CtkWidget *tree_view;

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  /* + grandparent1
   * + grandparent2
   *   + parent1
   *     + iter_parent1
   *   + parent2
   *     + iter_parent2_first
   *     + iter_parent2
   */

  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent1, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent2, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &parent1, &grandparent2);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter_parent1, &parent1);
  ctk_tree_store_append (CTK_TREE_STORE (model), &parent2, &grandparent2);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter_parent2_first, &parent2);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter_parent2, &parent2);

  filter_model = ctk_tree_model_filter_new (model, NULL);
  tree_view = ctk_tree_view_new_with_model (filter_model);

  ctk_tree_view_expand_all (CTK_TREE_VIEW (tree_view));

  assert_node_ref_count (ref_model, &grandparent1, 2);
  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &parent1, 3);
  assert_node_ref_count (ref_model, &parent2, 2);
  assert_node_ref_count (ref_model, &iter_parent1, 2);
  assert_node_ref_count (ref_model, &iter_parent2_first, 2);
  assert_node_ref_count (ref_model, &iter_parent2, 1);

  ctk_widget_destroy (tree_view);

  assert_node_ref_count (ref_model, &grandparent1, 1);
  assert_node_ref_count (ref_model, &grandparent2, 1);
  assert_node_ref_count (ref_model, &parent1, 2);
  assert_node_ref_count (ref_model, &parent2, 1);
  assert_node_ref_count (ref_model, &iter_parent1, 1);
  assert_node_ref_count (ref_model, &iter_parent2_first, 1);
  assert_node_ref_count (ref_model, &iter_parent2, 0);

  ctk_tree_model_filter_clear_cache (CTK_TREE_MODEL_FILTER (filter_model));

  /* The root level and first level remain cached, only the references on the
   * first nodes of these levels are kept.  Grandparent2 is the parent
   * of the first level with parent1, so grandparent2 keeps a reference
   * as well.
   */
  assert_node_ref_count (ref_model, &grandparent1, 1);
  assert_node_ref_count (ref_model, &grandparent2, 1);
  assert_node_ref_count (ref_model, &parent1, 1);
  assert_node_ref_count (ref_model, &parent2, 0);
  assert_node_ref_count (ref_model, &iter_parent1, 0);
  assert_node_ref_count (ref_model, &iter_parent2_first, 0);
  assert_node_ref_count (ref_model, &iter_parent2, 0);

  g_object_unref (filter_model);
  g_object_unref (ref_model);
}

static void
ref_count_row_ref (void)
{
  CtkTreeIter grandparent1, grandparent2, parent1, parent2;
  CtkTreeIter iter_parent1, iter_parent2, iter_parent2_first;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkTreeModel *filter_model;
  CtkWidget *tree_view;
  CtkTreePath *path;
  CtkTreeRowReference *row_ref;

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  /* + grandparent1
   * + grandparent2
   *   + parent1
   *     + iter_parent1
   *   + parent2
   *     + iter_parent2
   *     + iter_parent2
   */

  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent1, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent2, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &parent1, &grandparent2);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter_parent1, &parent1);
  ctk_tree_store_append (CTK_TREE_STORE (model), &parent2, &grandparent2);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter_parent2_first, &parent2);
  ctk_tree_store_append (CTK_TREE_STORE (model), &iter_parent2, &parent2);

  filter_model = ctk_tree_model_filter_new (model, NULL);
  tree_view = ctk_tree_view_new_with_model (filter_model);

  path = ctk_tree_path_new_from_indices (1, 1, 1, -1);
  row_ref = ctk_tree_row_reference_new (filter_model, path);
  ctk_tree_path_free (path);

  assert_node_ref_count (ref_model, &grandparent1, 2);
  assert_node_ref_count (ref_model, &grandparent2, 3);
  assert_node_ref_count (ref_model, &parent1, 1);
  assert_node_ref_count (ref_model, &parent2, 2);
  assert_node_ref_count (ref_model, &iter_parent1, 0);
  assert_node_ref_count (ref_model, &iter_parent2_first, 1);
  assert_node_ref_count (ref_model, &iter_parent2, 1);

  ctk_tree_row_reference_free (row_ref);

  assert_node_ref_count (ref_model, &grandparent1, 2);
  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &parent1, 1);
  assert_node_ref_count (ref_model, &parent2, 1);
  assert_node_ref_count (ref_model, &iter_parent1, 0);
  assert_node_ref_count (ref_model, &iter_parent2_first, 1);
  assert_node_ref_count (ref_model, &iter_parent2, 0);

  path = ctk_tree_path_new_from_indices (1, 1, 1, -1);
  row_ref = ctk_tree_row_reference_new (filter_model, path);
  ctk_tree_path_free (path);

  assert_node_ref_count (ref_model, &grandparent1, 2);
  assert_node_ref_count (ref_model, &grandparent2, 3);
  assert_node_ref_count (ref_model, &parent1, 1);
  assert_node_ref_count (ref_model, &parent2, 2);
  assert_node_ref_count (ref_model, &iter_parent1, 0);
  assert_node_ref_count (ref_model, &iter_parent2_first, 1);
  assert_node_ref_count (ref_model, &iter_parent2, 1);

  ctk_tree_store_remove (CTK_TREE_STORE (model), &parent2);

  assert_node_ref_count (ref_model, &grandparent1, 2);
  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &parent1, 1);
  assert_node_ref_count (ref_model, &iter_parent1, 0);

  ctk_tree_row_reference_free (row_ref);

  assert_node_ref_count (ref_model, &grandparent1, 2);
  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &parent1, 1);
  assert_node_ref_count (ref_model, &iter_parent1, 0);

  ctk_widget_destroy (tree_view);

  ctk_tree_model_filter_clear_cache (CTK_TREE_MODEL_FILTER (filter_model));

  /* The root level and first level remain cached, only the references on the
   * first nodes of these levels are kept.  Grandparent2 is the parent
   * of the first level with parent1, so grandparent2 keeps a reference
   * as well.
   */
  assert_node_ref_count (ref_model, &grandparent1, 1);
  assert_node_ref_count (ref_model, &grandparent2, 1);
  assert_node_ref_count (ref_model, &parent1, 1);

  g_object_unref (filter_model);
  g_object_unref (ref_model);
}

static void
ref_count_transfer_root_level_insert (void)
{
  CtkTreeIter grandparent1, grandparent2, grandparent3;
  CtkTreeIter new_node;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkTreeModel *filter_model;
  CtkWidget *tree_view;

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  /* + grandparent1
   * + grandparent2
   * + grandparent3
   */

  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent1, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent2, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent3, NULL);

  filter_model = ctk_tree_model_filter_new (model, NULL);
  tree_view = ctk_tree_view_new_with_model (filter_model);

  assert_node_ref_count (ref_model, &grandparent1, 2);
  assert_node_ref_count (ref_model, &grandparent2, 1);
  assert_node_ref_count (ref_model, &grandparent3, 1);

  ctk_tree_store_prepend (CTK_TREE_STORE (model), &new_node, NULL);

  assert_node_ref_count (ref_model, &new_node, 2);
  assert_node_ref_count (ref_model, &grandparent1, 1);
  assert_node_ref_count (ref_model, &grandparent2, 1);
  assert_node_ref_count (ref_model, &grandparent3, 1);

  ctk_widget_destroy (tree_view);
  g_object_unref (filter_model);
  g_object_unref (ref_model);
}

static void
ref_count_transfer_root_level_remove (void)
{
  CtkTreeIter grandparent1, grandparent2, grandparent3;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkTreeModel *filter_model;
  CtkWidget *tree_view;

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  /* + grandparent1
   * + grandparent2
   * + grandparent3
   */

  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent1, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent2, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent3, NULL);

  filter_model = ctk_tree_model_filter_new (model, NULL);
  tree_view = ctk_tree_view_new_with_model (filter_model);

  assert_node_ref_count (ref_model, &grandparent1, 2);
  assert_node_ref_count (ref_model, &grandparent2, 1);
  assert_node_ref_count (ref_model, &grandparent3, 1);

  ctk_tree_store_remove (CTK_TREE_STORE (model), &grandparent1);

  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &grandparent3, 1);

  ctk_tree_store_remove (CTK_TREE_STORE (model), &grandparent2);

  assert_node_ref_count (ref_model, &grandparent3, 2);

  ctk_widget_destroy (tree_view);
  g_object_unref (filter_model);
  g_object_unref (ref_model);
}

static void
ref_count_transfer_root_level_remove_filtered (void)
{
  CtkTreeIter grandparent1, grandparent2, grandparent3, grandparent4;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkTreeModel *filter_model;
  CtkWidget *tree_view;
  GType column_types[] = { G_TYPE_BOOLEAN };

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  ctk_tree_store_set_column_types (CTK_TREE_STORE (model), 1,
                                   column_types);

  /* + grandparent1
   * + grandparent2
   * + grandparent3
   * + grandparent4
   */

  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent1, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent2, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent3, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent4, NULL);

  /* Filter first node */
  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent1, 0, FALSE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent2, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent3, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent4, 0, TRUE, -1);

  filter_model = ctk_tree_model_filter_new (model, NULL);
  ctk_tree_model_filter_set_visible_column (CTK_TREE_MODEL_FILTER (filter_model), 0);
  tree_view = ctk_tree_view_new_with_model (filter_model);

  assert_node_ref_count (ref_model, &grandparent1, 0);
  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &grandparent3, 1);
  assert_node_ref_count (ref_model, &grandparent4, 1);

  ctk_tree_store_remove (CTK_TREE_STORE (model), &grandparent2);

  assert_node_ref_count (ref_model, &grandparent1, 0);
  assert_node_ref_count (ref_model, &grandparent3, 2);
  assert_node_ref_count (ref_model, &grandparent4, 1);

  ctk_tree_store_remove (CTK_TREE_STORE (model), &grandparent3);

  assert_node_ref_count (ref_model, &grandparent1, 0);
  assert_node_ref_count (ref_model, &grandparent4, 2);

  ctk_tree_store_remove (CTK_TREE_STORE (model), &grandparent4);

  /* Check level length to get root level cached again */
  check_level_length (CTK_TREE_MODEL_FILTER (filter_model), NULL, 0);

  assert_node_ref_count (ref_model, &grandparent1, 1);

  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent2, NULL);

  assert_node_ref_count (ref_model, &grandparent1, 1);
  assert_node_ref_count (ref_model, &grandparent2, 0);

  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent2, 0, TRUE, -1);

  assert_node_ref_count (ref_model, &grandparent1, 1);
  assert_node_ref_count (ref_model, &grandparent2, 1);

  check_level_length (CTK_TREE_MODEL_FILTER (filter_model), NULL, 1);

  ctk_widget_destroy (tree_view);
  g_object_unref (filter_model);
  g_object_unref (ref_model);
}

static void
ref_count_transfer_root_level_reordered (void)
{
  CtkTreeIter grandparent1, grandparent2, grandparent3;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkTreeModel *filter_model;
  CtkWidget *tree_view;

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  /* + grandparent1
   * + grandparent2
   * + grandparent3
   */

  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent1, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent2, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent3, NULL);

  filter_model = ctk_tree_model_filter_new (model, NULL);
  tree_view = ctk_tree_view_new_with_model (filter_model);

  assert_node_ref_count (ref_model, &grandparent1, 2);
  assert_node_ref_count (ref_model, &grandparent2, 1);
  assert_node_ref_count (ref_model, &grandparent3, 1);

  /* ctk_tree_store_move() will emit rows-reordered */
  ctk_tree_store_move_after (CTK_TREE_STORE (model),
                             &grandparent1, &grandparent3);

  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &grandparent3, 1);
  assert_node_ref_count (ref_model, &grandparent1, 1);

  ctk_widget_destroy (tree_view);
  g_object_unref (filter_model);
  g_object_unref (ref_model);
}

static void
ref_count_transfer_root_level_reordered_filtered (void)
{
  CtkTreeIter grandparent1, grandparent2, grandparent3;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkTreeModel *filter_model;
  CtkWidget *tree_view;
  GType column_types[] = { G_TYPE_BOOLEAN };

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  ctk_tree_store_set_column_types (CTK_TREE_STORE (model), 1,
                                   column_types);

  /* + grandparent1
   * + grandparent2
   * + grandparent3
   */

  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent1, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent2, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent3, NULL);

  /* Test with 1 node filtered */
  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent2, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent3, 0, TRUE, -1);

  filter_model = ctk_tree_model_filter_new (model, NULL);
  ctk_tree_model_filter_set_visible_column (CTK_TREE_MODEL_FILTER (filter_model), 0);
  tree_view = ctk_tree_view_new_with_model (filter_model);

  assert_node_ref_count (ref_model, &grandparent1, 0);
  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &grandparent3, 1);

  /* Move the invisible node grandparent1 */

  /* ctk_tree_store_move() will emit rows-reordered */
  ctk_tree_store_move_after (CTK_TREE_STORE (model),
                             &grandparent1, &grandparent3);

  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &grandparent3, 1);
  assert_node_ref_count (ref_model, &grandparent1, 0);

  /* Move the invisible node grandparent1 */

  /* ctk_tree_store_move() will emit rows-reordered */
  ctk_tree_store_move_before (CTK_TREE_STORE (model),
                              &grandparent1, &grandparent2);

  assert_node_ref_count (ref_model, &grandparent1, 0);
  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &grandparent3, 1);

  /* Now swap grandparent2 and grandparent3, first reference must transfer */
  /* ctk_tree_store_swap() will emit rows-reordered */
  ctk_tree_store_swap (CTK_TREE_STORE (model),
                       &grandparent2, &grandparent3);

  assert_node_ref_count (ref_model, &grandparent1, 0);
  assert_node_ref_count (ref_model, &grandparent3, 2);
  assert_node_ref_count (ref_model, &grandparent2, 1);

  /* Swap back */
  ctk_tree_store_swap (CTK_TREE_STORE (model),
                       &grandparent2, &grandparent3);

  assert_node_ref_count (ref_model, &grandparent1, 0);
  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &grandparent3, 1);

  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent1, 0, TRUE, -1);

  assert_node_ref_count (ref_model, &grandparent1, 2);
  assert_node_ref_count (ref_model, &grandparent2, 1);
  assert_node_ref_count (ref_model, &grandparent3, 1);

  /* Test with two nodes filtered */
  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent1, 0, FALSE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent2, 0, FALSE, -1);

  assert_node_ref_count (ref_model, &grandparent1, 0);
  assert_node_ref_count (ref_model, &grandparent2, 0);
  assert_node_ref_count (ref_model, &grandparent3, 2);

  /* ctk_tree_store_move() will emit rows-reordered */
  ctk_tree_store_move_before (CTK_TREE_STORE (model),
                             &grandparent3, &grandparent1);

  assert_node_ref_count (ref_model, &grandparent3, 2);
  assert_node_ref_count (ref_model, &grandparent2, 0);
  assert_node_ref_count (ref_model, &grandparent1, 0);

  ctk_widget_destroy (tree_view);
  g_object_unref (filter_model);
  g_object_unref (ref_model);
}

static void
ref_count_transfer_root_level_filter (void)
{
  CtkTreeIter grandparent1, grandparent2, grandparent3, grandparent4;
  CtkTreeIter new_node;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkTreeModel *filter_model;
  CtkWidget *tree_view;
  GType column_types[] = { G_TYPE_BOOLEAN };

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  ctk_tree_store_set_column_types (CTK_TREE_STORE (model), 1,
                                   column_types);

  /* + grandparent1
   * + grandparent2
   * + grandparent3
   * + grandparent4
   */

  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent1, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent2, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent3, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent4, NULL);

  /* Filter first node */
  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent1, 0, FALSE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent2, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent3, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent4, 0, TRUE, -1);

  filter_model = ctk_tree_model_filter_new (model, NULL);
  ctk_tree_model_filter_set_visible_column (CTK_TREE_MODEL_FILTER (filter_model), 0);
  tree_view = ctk_tree_view_new_with_model (filter_model);

  assert_node_ref_count (ref_model, &grandparent1, 0);
  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &grandparent3, 1);
  assert_node_ref_count (ref_model, &grandparent4, 1);

  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent2, 0, FALSE, -1);

  assert_node_ref_count (ref_model, &grandparent1, 0);
  assert_node_ref_count (ref_model, &grandparent2, 0);
  assert_node_ref_count (ref_model, &grandparent3, 2);
  assert_node_ref_count (ref_model, &grandparent4, 1);

  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent3, 0, FALSE, -1);

  assert_node_ref_count (ref_model, &grandparent1, 0);
  assert_node_ref_count (ref_model, &grandparent2, 0);
  assert_node_ref_count (ref_model, &grandparent3, 0);
  assert_node_ref_count (ref_model, &grandparent4, 2);

  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent4, 0, FALSE, -1);

  assert_node_ref_count (ref_model, &grandparent1, 0);
  assert_node_ref_count (ref_model, &grandparent2, 0);
  assert_node_ref_count (ref_model, &grandparent3, 0);
  assert_node_ref_count (ref_model, &grandparent4, 1);

  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent2, 0, TRUE, -1);

  assert_node_ref_count (ref_model, &grandparent1, 0);
  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &grandparent3, 0);
  assert_node_ref_count (ref_model, &grandparent4, 0);

  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent2, 0, FALSE, -1);

  assert_node_ref_count (ref_model, &grandparent1, 0);
  assert_node_ref_count (ref_model, &grandparent2, 0);
  assert_node_ref_count (ref_model, &grandparent3, 0);
  assert_node_ref_count (ref_model, &grandparent4, 1);

  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent1, 0, TRUE, -1);

  assert_node_ref_count (ref_model, &grandparent1, 2);
  assert_node_ref_count (ref_model, &grandparent2, 0);
  assert_node_ref_count (ref_model, &grandparent3, 0);
  assert_node_ref_count (ref_model, &grandparent4, 0);

  ctk_tree_store_prepend (CTK_TREE_STORE (model), &new_node, NULL);

  assert_node_ref_count (ref_model, &new_node, 0);
  assert_node_ref_count (ref_model, &grandparent1, 2);
  assert_node_ref_count (ref_model, &grandparent2, 0);
  assert_node_ref_count (ref_model, &grandparent3, 0);
  assert_node_ref_count (ref_model, &grandparent4, 0);

  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent1, 0, FALSE, -1);

  assert_node_ref_count (ref_model, &new_node, 0);
  assert_node_ref_count (ref_model, &grandparent1, 0);
  assert_node_ref_count (ref_model, &grandparent2, 0);
  assert_node_ref_count (ref_model, &grandparent3, 0);
  assert_node_ref_count (ref_model, &grandparent4, 1);

  ctk_tree_store_remove (CTK_TREE_STORE (model), &new_node);
  ctk_tree_store_prepend (CTK_TREE_STORE (model), &new_node, NULL);

  assert_node_ref_count (ref_model, &new_node, 0);
  assert_node_ref_count (ref_model, &grandparent1, 0);
  assert_node_ref_count (ref_model, &grandparent2, 0);
  assert_node_ref_count (ref_model, &grandparent3, 0);
  assert_node_ref_count (ref_model, &grandparent4, 1);

  ctk_tree_store_set (CTK_TREE_STORE (model), &new_node, 0, TRUE, -1);

  assert_node_ref_count (ref_model, &new_node, 2);
  assert_node_ref_count (ref_model, &grandparent1, 0);
  assert_node_ref_count (ref_model, &grandparent2, 0);
  assert_node_ref_count (ref_model, &grandparent3, 0);
  assert_node_ref_count (ref_model, &grandparent4, 0);

  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent2, 0, TRUE, -1);
  ctk_tree_store_remove (CTK_TREE_STORE (model), &new_node);

  assert_node_ref_count (ref_model, &grandparent1, 0);
  assert_node_ref_count (ref_model, &grandparent2, 2);
  assert_node_ref_count (ref_model, &grandparent3, 0);
  assert_node_ref_count (ref_model, &grandparent4, 0);

  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent4, 0, TRUE, -1);
  ctk_tree_store_remove (CTK_TREE_STORE (model), &grandparent2);

  ctk_widget_destroy (tree_view);
  g_object_unref (filter_model);
  g_object_unref (ref_model);
}

static void
ref_count_transfer_child_level_insert (void)
{
  CtkTreeIter grandparent1;
  CtkTreeIter parent1, parent2, parent3;
  CtkTreeIter new_node;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkTreeModel *filter_model;
  CtkWidget *tree_view;

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  /* + grandparent1
   *   + parent1
   *   + parent2
   *   + parent3
   */

  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent1, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &parent1, &grandparent1);
  ctk_tree_store_append (CTK_TREE_STORE (model), &parent2, &grandparent1);
  ctk_tree_store_append (CTK_TREE_STORE (model), &parent3, &grandparent1);

  filter_model = ctk_tree_model_filter_new (model, NULL);
  tree_view = ctk_tree_view_new_with_model (filter_model);

  assert_node_ref_count (ref_model, &grandparent1, 3);
  assert_node_ref_count (ref_model, &parent1, 1);
  assert_node_ref_count (ref_model, &parent2, 0);
  assert_node_ref_count (ref_model, &parent3, 0);

  ctk_tree_store_prepend (CTK_TREE_STORE (model), &new_node, &grandparent1);

  assert_node_ref_count (ref_model, &grandparent1, 3);
  assert_node_ref_count (ref_model, &new_node, 1);
  assert_node_ref_count (ref_model, &parent1, 0);
  assert_node_ref_count (ref_model, &parent2, 0);
  assert_node_ref_count (ref_model, &parent3, 0);

  ctk_widget_destroy (tree_view);
  g_object_unref (filter_model);
  g_object_unref (ref_model);
}

static void
ref_count_transfer_child_level_remove (void)
{
  CtkTreeIter grandparent1;
  CtkTreeIter parent1, parent2, parent3;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkTreeModel *filter_model;
  CtkWidget *tree_view;

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  /* + grandparent1
   *   + parent1
   *   + parent2
   *   + parent3
   */

  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent1, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &parent1, &grandparent1);
  ctk_tree_store_append (CTK_TREE_STORE (model), &parent2, &grandparent1);
  ctk_tree_store_append (CTK_TREE_STORE (model), &parent3, &grandparent1);

  filter_model = ctk_tree_model_filter_new (model, NULL);
  tree_view = ctk_tree_view_new_with_model (filter_model);

  assert_node_ref_count (ref_model, &grandparent1, 3);
  assert_node_ref_count (ref_model, &parent1, 1);
  assert_node_ref_count (ref_model, &parent2, 0);
  assert_node_ref_count (ref_model, &parent3, 0);

  ctk_tree_store_remove (CTK_TREE_STORE (model), &parent1);

  assert_node_ref_count (ref_model, &grandparent1, 3);
  assert_node_ref_count (ref_model, &parent2, 1);
  assert_node_ref_count (ref_model, &parent3, 0);

  ctk_tree_store_remove (CTK_TREE_STORE (model), &parent2);

  assert_node_ref_count (ref_model, &grandparent1, 3);
  assert_node_ref_count (ref_model, &parent3, 1);

  ctk_widget_destroy (tree_view);
  g_object_unref (filter_model);
  g_object_unref (ref_model);
}

static void
ref_count_transfer_child_level_remove_filtered (void)
{
  CtkTreeIter grandparent1;
  CtkTreeIter parent1, parent2, parent3, parent4;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkTreeModel *filter_model;
  CtkWidget *tree_view;
  GType column_types[] = { G_TYPE_BOOLEAN };

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  ctk_tree_store_set_column_types (CTK_TREE_STORE (model), 1,
                                   column_types);

  /* + grandparent1
   *   + parent1
   *   + parent2
   *   + parent3
   *   + parent4
   */

  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent1, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &parent1, &grandparent1);
  ctk_tree_store_append (CTK_TREE_STORE (model), &parent2, &grandparent1);
  ctk_tree_store_append (CTK_TREE_STORE (model), &parent3, &grandparent1);
  ctk_tree_store_append (CTK_TREE_STORE (model), &parent4, &grandparent1);

  /* Filter first node */
  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent1, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &parent1, 0, FALSE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &parent2, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &parent3, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &parent4, 0, TRUE, -1);

  filter_model = ctk_tree_model_filter_new (model, NULL);
  ctk_tree_model_filter_set_visible_column (CTK_TREE_MODEL_FILTER (filter_model), 0);
  tree_view = ctk_tree_view_new_with_model (filter_model);

  assert_node_ref_count (ref_model, &grandparent1, 3);
  assert_node_ref_count (ref_model, &parent1, 0);
  assert_node_ref_count (ref_model, &parent2, 1);
  assert_node_ref_count (ref_model, &parent3, 0);
  assert_node_ref_count (ref_model, &parent4, 0);

  ctk_tree_store_remove (CTK_TREE_STORE (model), &parent2);

  assert_node_ref_count (ref_model, &grandparent1, 3);
  assert_node_ref_count (ref_model, &parent1, 0);
  assert_node_ref_count (ref_model, &parent3, 1);
  assert_node_ref_count (ref_model, &parent4, 0);

  ctk_tree_store_remove (CTK_TREE_STORE (model), &parent3);

  assert_node_ref_count (ref_model, &grandparent1, 3);
  assert_node_ref_count (ref_model, &parent1, 0);
  assert_node_ref_count (ref_model, &parent4, 1);

  ctk_tree_store_remove (CTK_TREE_STORE (model), &parent4);

  /* Check level length to get level cached again */
  check_level_length (CTK_TREE_MODEL_FILTER (filter_model), "0", 0);

  assert_node_ref_count (ref_model, &grandparent1, 3);
  assert_node_ref_count (ref_model, &parent1, 1);

  ctk_tree_store_append (CTK_TREE_STORE (model), &parent2, &grandparent1);

  assert_node_ref_count (ref_model, &grandparent1, 3);
  assert_node_ref_count (ref_model, &parent1, 1);
  assert_node_ref_count (ref_model, &parent2, 0);

  ctk_tree_store_set (CTK_TREE_STORE (model), &parent2, 0, TRUE, -1);

  assert_node_ref_count (ref_model, &grandparent1, 3);
  assert_node_ref_count (ref_model, &parent1, 1);
  assert_node_ref_count (ref_model, &parent2, 0);

  check_level_length (CTK_TREE_MODEL_FILTER (filter_model), "0", 1);

  ctk_widget_destroy (tree_view);
  g_object_unref (filter_model);
  g_object_unref (ref_model);
}

static void
ref_count_transfer_child_level_reordered (void)
{
  CtkTreeIter grandparent1;
  CtkTreeIter parent1, parent2, parent3;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkTreeModel *filter_model;
  CtkWidget *tree_view;

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  /* + grandparent1
   *   + parent1
   *   + parent2
   *   + parent3
   */

  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent1, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &parent1, &grandparent1);
  ctk_tree_store_append (CTK_TREE_STORE (model), &parent2, &grandparent1);
  ctk_tree_store_append (CTK_TREE_STORE (model), &parent3, &grandparent1);

  filter_model = ctk_tree_model_filter_new (model, NULL);
  tree_view = ctk_tree_view_new_with_model (filter_model);

  assert_node_ref_count (ref_model, &grandparent1, 3);
  assert_node_ref_count (ref_model, &parent1, 1);
  assert_node_ref_count (ref_model, &parent2, 0);
  assert_node_ref_count (ref_model, &parent3, 0);

  /* ctk_tree_store_move() will emit rows-reordered */
  ctk_tree_store_move_after (CTK_TREE_STORE (model),
                             &parent1, &parent3);

  assert_node_ref_count (ref_model, &grandparent1, 3);
  assert_node_ref_count (ref_model, &parent2, 1);
  assert_node_ref_count (ref_model, &parent3, 0);
  assert_node_ref_count (ref_model, &parent1, 0);

  ctk_widget_destroy (tree_view);
  g_object_unref (filter_model);
  g_object_unref (ref_model);
}

static void
ref_count_transfer_child_level_reordered_filtered (void)
{
  CtkTreeIter grandparent1;
  CtkTreeIter parent1, parent2, parent3;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkTreeModel *filter_model;
  CtkWidget *tree_view;
  GType column_types[] = { G_TYPE_BOOLEAN };

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  ctk_tree_store_set_column_types (CTK_TREE_STORE (model), 1,
                                   column_types);

  /* + grandparent1
   *   + parent1
   *   + parent2
   *   + parent3
   */

  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent1, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &parent1, &grandparent1);
  ctk_tree_store_append (CTK_TREE_STORE (model), &parent2, &grandparent1);
  ctk_tree_store_append (CTK_TREE_STORE (model), &parent3, &grandparent1);

  /* Test with 1 node filtered (parent1) */
  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent1, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &parent2, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &parent3, 0, TRUE, -1);

  filter_model = ctk_tree_model_filter_new (model, NULL);
  ctk_tree_model_filter_set_visible_column (CTK_TREE_MODEL_FILTER (filter_model), 0);
  tree_view = ctk_tree_view_new_with_model (filter_model);

  assert_node_ref_count (ref_model, &grandparent1, 3);
  assert_node_ref_count (ref_model, &parent1, 0);
  assert_node_ref_count (ref_model, &parent2, 1);
  assert_node_ref_count (ref_model, &parent3, 0);

  /* Move invisible node parent 1 */

  /* ctk_tree_store_move() will emit rows-reordered */
  ctk_tree_store_move_after (CTK_TREE_STORE (model),
                             &parent1, &parent3);

  assert_node_ref_count (ref_model, &grandparent1, 3);
  assert_node_ref_count (ref_model, &parent2, 1);
  assert_node_ref_count (ref_model, &parent3, 0);
  assert_node_ref_count (ref_model, &parent1, 0);

  /* Move invisible node parent 1 */

  /* ctk_tree_store_move() will emit rows-reordered */
  ctk_tree_store_move_before (CTK_TREE_STORE (model),
                              &parent1, &parent2);

  assert_node_ref_count (ref_model, &grandparent1, 3);
  assert_node_ref_count (ref_model, &parent1, 0);
  assert_node_ref_count (ref_model, &parent2, 1);
  assert_node_ref_count (ref_model, &parent3, 0);

  /* Now swap parent2 and parent2, first reference must transfer */
  /* ctk_tree_store_swap() will emit rows-reordered */
  ctk_tree_store_swap (CTK_TREE_STORE (model),
                       &parent2, &parent3);

  assert_node_ref_count (ref_model, &grandparent1, 3);
  assert_node_ref_count (ref_model, &parent1, 0);
  assert_node_ref_count (ref_model, &parent3, 1);
  assert_node_ref_count (ref_model, &parent2, 0);

  /* Swap back */
  ctk_tree_store_swap (CTK_TREE_STORE (model),
                       &parent2, &parent3);

  assert_node_ref_count (ref_model, &grandparent1, 3);
  assert_node_ref_count (ref_model, &parent1, 0);
  assert_node_ref_count (ref_model, &parent2, 1);
  assert_node_ref_count (ref_model, &parent3, 0);

  ctk_tree_store_set (CTK_TREE_STORE (model), &parent1, 0, TRUE, -1);

  assert_node_ref_count (ref_model, &parent1, 1);
  assert_node_ref_count (ref_model, &parent2, 0);
  assert_node_ref_count (ref_model, &parent3, 0);

  /* Test with two nodes filtered */
  ctk_tree_store_set (CTK_TREE_STORE (model), &parent1, 0, FALSE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &parent2, 0, FALSE, -1);

  assert_node_ref_count (ref_model, &parent1, 0);
  assert_node_ref_count (ref_model, &parent2, 0);
  assert_node_ref_count (ref_model, &parent3, 1);

  /* ctk_tree_store_move() will emit rows-reordered */
  ctk_tree_store_move_before (CTK_TREE_STORE (model),
                             &parent3, &parent1);

  assert_node_ref_count (ref_model, &parent3, 1);
  assert_node_ref_count (ref_model, &parent2, 0);
  assert_node_ref_count (ref_model, &parent1, 0);

  ctk_widget_destroy (tree_view);
  g_object_unref (filter_model);
  g_object_unref (ref_model);
}

static void
ref_count_transfer_child_level_filter (void)
{
  CtkTreeIter root;
  CtkTreeIter grandparent1, grandparent2, grandparent3, grandparent4;
  CtkTreeIter new_node;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkTreeModel *filter_model;
  CtkWidget *tree_view;
  GType column_types[] = { G_TYPE_BOOLEAN };

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  ctk_tree_store_set_column_types (CTK_TREE_STORE (model), 1,
                                   column_types);

  /* + root
   *    + grandparent1
   *    + grandparent2
   *    + grandparent3
   *    + grandparent4
   */

  ctk_tree_store_append (CTK_TREE_STORE (model), &root, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent1, &root);
  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent2, &root);
  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent3, &root);
  ctk_tree_store_append (CTK_TREE_STORE (model), &grandparent4, &root);

  /* Filter first node */
  ctk_tree_store_set (CTK_TREE_STORE (model), &root, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent1, 0, FALSE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent2, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent3, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent4, 0, TRUE, -1);

  filter_model = ctk_tree_model_filter_new (model, NULL);
  ctk_tree_model_filter_set_visible_column (CTK_TREE_MODEL_FILTER (filter_model), 0);
  tree_view = ctk_tree_view_new_with_model (filter_model);

  assert_node_ref_count (ref_model, &grandparent1, 0);
  assert_node_ref_count (ref_model, &grandparent2, 1);
  assert_node_ref_count (ref_model, &grandparent3, 0);
  assert_node_ref_count (ref_model, &grandparent4, 0);

  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent2, 0, FALSE, -1);

  assert_node_ref_count (ref_model, &grandparent1, 0);
  assert_node_ref_count (ref_model, &grandparent2, 0);
  assert_node_ref_count (ref_model, &grandparent3, 1);
  assert_node_ref_count (ref_model, &grandparent4, 0);

  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent3, 0, FALSE, -1);

  assert_node_ref_count (ref_model, &grandparent1, 0);
  assert_node_ref_count (ref_model, &grandparent2, 0);
  assert_node_ref_count (ref_model, &grandparent3, 0);
  assert_node_ref_count (ref_model, &grandparent4, 1);

  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent4, 0, FALSE, -1);

  assert_node_ref_count (ref_model, &grandparent1, 0);
  assert_node_ref_count (ref_model, &grandparent2, 0);
  assert_node_ref_count (ref_model, &grandparent3, 0);
  assert_node_ref_count (ref_model, &grandparent4, 1);

  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent2, 0, TRUE, -1);

  assert_node_ref_count (ref_model, &grandparent1, 0);
  assert_node_ref_count (ref_model, &grandparent2, 1);
  assert_node_ref_count (ref_model, &grandparent3, 0);
  assert_node_ref_count (ref_model, &grandparent4, 0);

  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent2, 0, FALSE, -1);

  assert_node_ref_count (ref_model, &grandparent1, 0);
  assert_node_ref_count (ref_model, &grandparent2, 0);
  assert_node_ref_count (ref_model, &grandparent3, 0);
  assert_node_ref_count (ref_model, &grandparent4, 1);

  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent1, 0, TRUE, -1);

  assert_node_ref_count (ref_model, &grandparent1, 1);
  assert_node_ref_count (ref_model, &grandparent2, 0);
  assert_node_ref_count (ref_model, &grandparent3, 0);
  assert_node_ref_count (ref_model, &grandparent4, 0);

  ctk_tree_store_prepend (CTK_TREE_STORE (model), &new_node, &root);

  assert_node_ref_count (ref_model, &new_node, 0);
  assert_node_ref_count (ref_model, &grandparent1, 1);
  assert_node_ref_count (ref_model, &grandparent2, 0);
  assert_node_ref_count (ref_model, &grandparent3, 0);
  assert_node_ref_count (ref_model, &grandparent4, 0);

  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent1, 0, FALSE, -1);

  assert_node_ref_count (ref_model, &new_node, 0);
  assert_node_ref_count (ref_model, &grandparent1, 0);
  assert_node_ref_count (ref_model, &grandparent2, 0);
  assert_node_ref_count (ref_model, &grandparent3, 0);
  assert_node_ref_count (ref_model, &grandparent4, 1);

  ctk_tree_store_remove (CTK_TREE_STORE (model), &new_node);
  ctk_tree_store_prepend (CTK_TREE_STORE (model), &new_node, &root);

  assert_node_ref_count (ref_model, &new_node, 0);
  assert_node_ref_count (ref_model, &grandparent1, 0);
  assert_node_ref_count (ref_model, &grandparent2, 0);
  assert_node_ref_count (ref_model, &grandparent3, 0);
  assert_node_ref_count (ref_model, &grandparent4, 1);

  ctk_tree_store_set (CTK_TREE_STORE (model), &new_node, 0, TRUE, -1);

  assert_node_ref_count (ref_model, &new_node, 1);
  assert_node_ref_count (ref_model, &grandparent1, 0);
  assert_node_ref_count (ref_model, &grandparent2, 0);
  assert_node_ref_count (ref_model, &grandparent3, 0);
  assert_node_ref_count (ref_model, &grandparent4, 0);

  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent2, 0, TRUE, -1);
  ctk_tree_store_remove (CTK_TREE_STORE (model), &new_node);

  assert_node_ref_count (ref_model, &grandparent1, 0);
  assert_node_ref_count (ref_model, &grandparent2, 1);
  assert_node_ref_count (ref_model, &grandparent3, 0);
  assert_node_ref_count (ref_model, &grandparent4, 0);

  ctk_tree_store_set (CTK_TREE_STORE (model), &grandparent4, 0, TRUE, -1);
  ctk_tree_store_remove (CTK_TREE_STORE (model), &grandparent2);

  ctk_widget_destroy (tree_view);
  g_object_unref (filter_model);
  g_object_unref (ref_model);
}


static gboolean
specific_path_dependent_filter_func (CtkTreeModel *model,
                                     CtkTreeIter  *iter,
                                     gpointer      data)
{
  CtkTreePath *path;

  path = ctk_tree_model_get_path (model, iter);
  if (ctk_tree_path_get_indices (path)[0] < 4)
    return FALSE;

  return TRUE;
}

static void
specific_path_dependent_filter (void)
{
  int i;
  CtkTreeIter iter;
  CtkListStore *list;
  CtkTreeModel *sort;
  CtkTreeModel *filter;

  list = ctk_list_store_new (1, G_TYPE_INT);
  ctk_list_store_insert_with_values (list, &iter, 0, 0, 1, -1);
  ctk_list_store_insert_with_values (list, &iter, 1, 0, 2, -1);
  ctk_list_store_insert_with_values (list, &iter, 2, 0, 3, -1);
  ctk_list_store_insert_with_values (list, &iter, 3, 0, 4, -1);
  ctk_list_store_insert_with_values (list, &iter, 4, 0, 5, -1);
  ctk_list_store_insert_with_values (list, &iter, 5, 0, 6, -1);
  ctk_list_store_insert_with_values (list, &iter, 6, 0, 7, -1);
  ctk_list_store_insert_with_values (list, &iter, 7, 0, 8, -1);

  sort = ctk_tree_model_sort_new_with_model (CTK_TREE_MODEL (list));
  filter = ctk_tree_model_filter_new (CTK_TREE_MODEL (sort), NULL);
  ctk_tree_model_filter_set_visible_func (CTK_TREE_MODEL_FILTER (filter),
                                          specific_path_dependent_filter_func,
                                          NULL, NULL);

  ctk_tree_sortable_set_sort_column_id (CTK_TREE_SORTABLE (sort), 0,
                                        CTK_SORT_DESCENDING);

  for (i = 0; i < 4; i++)
    {
      if (ctk_tree_model_iter_nth_child (CTK_TREE_MODEL (list), &iter,
                                         NULL, 1))
        ctk_list_store_remove (list, &iter);

      if (ctk_tree_model_iter_nth_child (CTK_TREE_MODEL (list), &iter,
                                         NULL, 2))
        ctk_list_store_remove (list, &iter);
    }

  g_object_unref (filter);
  g_object_unref (sort);
  g_object_unref (list);
}


static gboolean
specific_append_after_collapse_visible_func (CtkTreeModel *model,
                                             CtkTreeIter  *iter,
                                             gpointer      data)
{
  gint number;
  gboolean hide_negative_numbers;

  ctk_tree_model_get (model, iter, 1, &number, -1);
  hide_negative_numbers = GPOINTER_TO_INT (g_object_get_data (data, "private-hide-negative-numbers"));

  return (number >= 0 || !hide_negative_numbers);
}

static void
specific_append_after_collapse (void)
{
  /* This test is based on one of the test cases I found in my
   * old test cases directory.  I unfortunately do not have a record
   * from who this test case originated.  -Kris.
   *
   * General idea:
   * - Construct tree.
   * - Show tree, expand, collapse.
   * - Add a row.
   */

  CtkTreeIter iter;
  CtkTreeIter child_iter;
  CtkTreeIter child_iter2;
  CtkTreePath *append_path;
  CtkTreeStore *store;
  CtkTreeModel *filter;
  CtkTreeModel *sort;

  CtkWidget *window;
  CtkWidget *tree_view;

  store = ctk_tree_store_new (2, G_TYPE_STRING, G_TYPE_INT);

  filter = ctk_tree_model_filter_new (CTK_TREE_MODEL (store), NULL);
  g_object_set_data (G_OBJECT (filter), "private-hide-negative-numbers",
                     GINT_TO_POINTER (FALSE));
  ctk_tree_model_filter_set_visible_func (CTK_TREE_MODEL_FILTER (filter),
                                          specific_append_after_collapse_visible_func,
                                          filter, NULL);

  sort = ctk_tree_model_sort_new_with_model (filter);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  tree_view = ctk_tree_view_new_with_model (sort);
  ctk_container_add (CTK_CONTAINER (window), tree_view);
  ctk_widget_realize (tree_view);

  while (ctk_events_pending ())
    ctk_main_iteration ();

  ctk_tree_store_prepend (store, &iter, NULL);
  ctk_tree_store_set (store, &iter,
                      0, "hallo", 1, 1, -1);

  ctk_tree_store_append (store, &child_iter, &iter);
  ctk_tree_store_set (store, &child_iter,
                      0, "toemaar", 1, 1, -1);

  ctk_tree_store_append (store, &child_iter2, &child_iter);
  ctk_tree_store_set (store, &child_iter2,
                      0, "very deep", 1, 1, -1);

  append_path = ctk_tree_model_get_path (CTK_TREE_MODEL (store), &child_iter2);

  ctk_tree_store_append (store, &child_iter, &iter);
  ctk_tree_store_set (store, &child_iter,
                      0, "sja", 1, 1, -1);

  ctk_tree_store_append (store, &child_iter, &iter);
  ctk_tree_store_set (store, &child_iter,
                      0, "some word", 1, -1, -1);

  /* Expand and collapse the tree */
  ctk_tree_view_expand_all (CTK_TREE_VIEW (tree_view));
  while (ctk_events_pending ())
    ctk_main_iteration ();

  ctk_tree_view_collapse_all (CTK_TREE_VIEW (tree_view));
  while (ctk_events_pending ())
    ctk_main_iteration ();

  /* Add another it */
  g_object_set_data (G_OBJECT (filter), "private-hide-negative-numbers",
                     GINT_TO_POINTER (TRUE));

  if (ctk_tree_model_get_iter (CTK_TREE_MODEL (store), &iter, append_path))
    {
      ctk_tree_store_append (store, &child_iter, &iter);
      ctk_tree_store_set (store, &child_iter,
                          0, "new new new !!", 1, 1, -1);
    }
  ctk_tree_path_free (append_path);

  /* Expand */
  ctk_tree_view_expand_all (CTK_TREE_VIEW (tree_view));
  while (ctk_events_pending ())
    ctk_main_iteration ();
}


static gint
specific_sort_filter_remove_node_compare_func (CtkTreeModel  *model,
                                               CtkTreeIter   *iter1,
                                               CtkTreeIter   *iter2,
                                               gpointer       data)
{
  return -1;
}

static gboolean
specific_sort_filter_remove_node_visible_func (CtkTreeModel  *model,
                                               CtkTreeIter   *iter,
                                               gpointer       data)
{
  char *item = NULL;

  /* Do reference the model */
  ctk_tree_model_get (model, iter, 0, &item, -1);
  g_free (item);

  return FALSE;
}

static void
specific_sort_filter_remove_node (void)
{
  /* This test is based on one of the test cases I found in my
   * old test cases directory.  I unfortunately do not have a record
   * from who this test case originated.  -Kris.
   *
   * General idea:
   *  - Create tree store, sort, filter models.  The sort model has
   *    a default sort func that is enabled, filter model a visible func
   *    that defaults to returning FALSE.
   *  - Remove a node from the tree store.
   */

  CtkTreeIter iter;
  CtkTreeStore *store;
  CtkTreeModel *filter;
  CtkTreeModel *sort;

  CtkWidget *window;
  CtkWidget *tree_view;

  store = ctk_tree_store_new (1, G_TYPE_STRING);
  ctk_tree_store_append (store, &iter, NULL);
  ctk_tree_store_set (store, &iter, 0, "Hello1", -1);

  ctk_tree_store_append (store, &iter, NULL);
  ctk_tree_store_set (store, &iter, 0, "Hello2", -1);

  sort = ctk_tree_model_sort_new_with_model (CTK_TREE_MODEL (store));
  ctk_tree_sortable_set_default_sort_func (CTK_TREE_SORTABLE (sort),
                                           specific_sort_filter_remove_node_compare_func, NULL, NULL);

  filter = ctk_tree_model_filter_new (sort, NULL);
  ctk_tree_model_filter_set_visible_func (CTK_TREE_MODEL_FILTER (filter),
                                          specific_sort_filter_remove_node_visible_func,
                                          filter, NULL);


  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  tree_view = ctk_tree_view_new_with_model (filter);
  ctk_container_add (CTK_CONTAINER (window), tree_view);
  ctk_widget_realize (tree_view);

  while (ctk_events_pending ())
    ctk_main_iteration ();

  /* Remove a node */
  ctk_tree_model_get_iter_first (CTK_TREE_MODEL (store), &iter);
  ctk_tree_model_iter_next (CTK_TREE_MODEL (store), &iter);
  ctk_tree_store_remove (store, &iter);

  while (ctk_events_pending ())
    ctk_main_iteration ();
}


static void
specific_sort_filter_remove_root (void)
{
  /* This test is based on one of the test cases I found in my
   * old test cases directory.  I unfortunately do not have a record
   * from who this test case originated.  -Kris.
   */

  CtkTreeModel *model, *sort, *filter;
  CtkTreeIter root, mid, leaf;
  CtkTreePath *path;

  model = CTK_TREE_MODEL (ctk_tree_store_new (1, G_TYPE_INT));
  ctk_tree_store_append (CTK_TREE_STORE (model), &root, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &mid, &root);
  ctk_tree_store_append (CTK_TREE_STORE (model), &leaf, &mid);

  path = ctk_tree_model_get_path (model, &mid);

  sort = ctk_tree_model_sort_new_with_model (model);
  filter = ctk_tree_model_filter_new (sort, path);

  ctk_tree_path_free (path);

  ctk_tree_store_remove (CTK_TREE_STORE (model), &root);

  g_object_unref (filter);
  g_object_unref (sort);
  g_object_unref (model);
}


static void
specific_root_mixed_visibility (void)
{
  int i;
  CtkTreeModel *filter;
  /* A bit nasty, apologies */
  FilterTest fixture;

  fixture.store = ctk_tree_store_new (2, G_TYPE_STRING, G_TYPE_BOOLEAN);

  for (i = 0; i < LEVEL_LENGTH; i++)
    {
      CtkTreeIter iter;

      ctk_tree_store_insert (fixture.store, &iter, NULL, i);
      if (i % 2 == 0)
        create_tree_store_set_values (fixture.store, &iter, TRUE);
      else
        create_tree_store_set_values (fixture.store, &iter, FALSE);
    }

  filter = ctk_tree_model_filter_new (CTK_TREE_MODEL (fixture.store), NULL);
  fixture.filter = CTK_TREE_MODEL_FILTER (filter);
  fixture.monitor = NULL;

  ctk_tree_model_filter_set_visible_column (fixture.filter, 1);

  /* In order to trigger the potential bug, we should not access
   * the filter model here (so don't call the check functions).
   */

  /* Change visibility of an odd row to TRUE */
  set_path_visibility (&fixture, "3", TRUE);
  check_filter_model (&fixture);
  check_level_length (fixture.filter, NULL, 4);
}



static gboolean
specific_has_child_filter_filter_func (CtkTreeModel *model,
                                       CtkTreeIter  *iter,
                                       gpointer      data)
{
  return ctk_tree_model_iter_has_child (model, iter);
}

static void
specific_has_child_filter (void)
{
  CtkTreeModel *filter;
  CtkTreeIter iter, root;
  FilterTest fixture; /* This is not how it should be done */
  CtkWidget *tree_view;

  fixture.store = ctk_tree_store_new (2, G_TYPE_STRING, G_TYPE_BOOLEAN);
  filter = ctk_tree_model_filter_new (CTK_TREE_MODEL (fixture.store), NULL);
  fixture.filter = CTK_TREE_MODEL_FILTER (filter);
  fixture.monitor = signal_monitor_new (filter);

  tree_view = ctk_tree_view_new_with_model (filter);

  /* We will filter on parent state using a filter function.  We will
   * manually keep the boolean column in sync, so that we can use
   * check_filter_model() to check the consistency of the model.
   */
  /* FIXME: We need a check_filter_model() that is not tied to LEVEL_LENGTH
   * to be able to check the structure here.  We keep the calls to
   * check_filter_model() commented out until then.
   */
  ctk_tree_model_filter_set_visible_func (fixture.filter,
                                          specific_has_child_filter_filter_func,
                                          NULL, NULL);

  /* The first node will be initially invisible: no signals */
  ctk_tree_store_append (fixture.store, &root, NULL);
  create_tree_store_set_values (fixture.store, &root, FALSE);

  /* check_filter_model (&fixture); */
  check_level_length (fixture.filter, NULL, 0);
  signal_monitor_assert_is_empty (fixture.monitor);

  /* Insert a child node. This will cause the parent to become visible
   * since there is a child now.
   */
  signal_monitor_append_signal (fixture.monitor, ROW_INSERTED, "0");
  signal_monitor_append_signal (fixture.monitor, ROW_HAS_CHILD_TOGGLED, "0");
  signal_monitor_append_signal (fixture.monitor, ROW_HAS_CHILD_TOGGLED, "0");

  ctk_tree_store_append (fixture.store, &iter, &root);
  create_tree_store_set_values (fixture.store, &iter, TRUE);

  /* Parent must now be visible.  Do the level length check first,
   * to avoid modifying the child model triggering a row-changed to
   * the filter model.
   */
  check_level_length (fixture.filter, NULL, 1);
  check_level_length (fixture.filter, "0", 0);
  signal_monitor_assert_is_empty (fixture.monitor);

  /* This should propagate row-changed */
  signal_monitor_append_signal (fixture.monitor, ROW_CHANGED, "0");
  signal_monitor_append_signal (fixture.monitor, ROW_HAS_CHILD_TOGGLED, "0");

  set_path_visibility (&fixture, "0", TRUE);
  /* check_filter_model (&fixture); */
  signal_monitor_assert_is_empty (fixture.monitor);

  /* New root node, no child, so no signal */
  ctk_tree_store_append (fixture.store, &root, NULL);
  check_level_length (fixture.filter, NULL, 1);
  signal_monitor_assert_is_empty (fixture.monitor);

  /* When the child comes in, this node will become visible */
  signal_monitor_append_signal (fixture.monitor, ROW_INSERTED, "1");
  signal_monitor_append_signal (fixture.monitor, ROW_HAS_CHILD_TOGGLED, "1");
  signal_monitor_append_signal (fixture.monitor, ROW_HAS_CHILD_TOGGLED, "1");
  signal_monitor_append_signal (fixture.monitor, ROW_CHANGED, "1");
  signal_monitor_append_signal (fixture.monitor, ROW_HAS_CHILD_TOGGLED, "1");

  ctk_tree_store_append (fixture.store, &iter, &root);
  check_level_length (fixture.filter, NULL, 2);
  check_level_length (fixture.filter, "1", 0);

  create_tree_store_set_values (fixture.store, &root, TRUE);
  create_tree_store_set_values (fixture.store, &iter, TRUE);

  /* check_filter_model (&fixture); */
  signal_monitor_assert_is_empty (fixture.monitor);

  /* Add another child for 1 */
  ctk_tree_store_append (fixture.store, &iter, &root);
  create_tree_store_set_values (fixture.store, &iter, TRUE);
  check_level_length (fixture.filter, NULL, 2);
  check_level_length (fixture.filter, "0", 0);
  check_level_length (fixture.filter, "1", 0);
  signal_monitor_assert_is_empty (fixture.monitor);

  /* Now remove one of the remaining child rows */
  signal_monitor_append_signal (fixture.monitor, ROW_DELETED, "0");

  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture.store),
                                       &iter, "0:0");
  ctk_tree_store_remove (fixture.store, &iter);

  check_level_length (fixture.filter, NULL, 1);
  check_level_length (fixture.filter, "0", 0);

  set_path_visibility (&fixture, "0", FALSE);
  /* check_filter_model (&fixture); */
  signal_monitor_assert_is_empty (fixture.monitor);

  g_object_unref (fixture.filter);
  g_object_unref (fixture.store);
  ctk_widget_destroy (tree_view);
}


static gboolean
specific_root_has_child_filter_filter_func (CtkTreeModel *model,
                                            CtkTreeIter  *iter,
                                            gpointer      data)
{
  int depth;
  CtkTreePath *path;

  path = ctk_tree_model_get_path (model, iter);
  depth = ctk_tree_path_get_depth (path);
  ctk_tree_path_free (path);

  if (depth > 1)
    return TRUE;
  /* else */
  return ctk_tree_model_iter_has_child (model, iter);
}

static void
specific_root_has_child_filter (void)
{
  CtkTreeModel *filter;
  CtkTreeIter iter, root;
  FilterTest fixture; /* This is not how it should be done ... */
  CtkWidget *tree_view;

  /* This is a variation on the above test case, specific has-child-filter,
   * herein the has-child check for visibility only applies to root level
   * nodes.  In this test, children are always visible because we
   * only filter based on the "has child" criterion.
   */

  fixture.store = ctk_tree_store_new (2, G_TYPE_STRING, G_TYPE_BOOLEAN);
  filter = ctk_tree_model_filter_new (CTK_TREE_MODEL (fixture.store), NULL);
  fixture.filter = CTK_TREE_MODEL_FILTER (filter);
  fixture.monitor = signal_monitor_new (filter);

  tree_view = ctk_tree_view_new_with_model (filter);

  /* We will filter on parent state using a filter function.  We will
   * manually keep the boolean column in sync, so that we can use
   * check_filter_model() to check the consistency of the model.
   */
  /* FIXME: We need a check_filter_model() that is not tied to LEVEL_LENGTH
   * to be able to check the structure here.  We keep the calls to
   * check_filter_model() commented out until then.
   */
  ctk_tree_model_filter_set_visible_func (fixture.filter,
                                          specific_root_has_child_filter_filter_func,
                                          NULL, NULL);

  /* Add a first node, this will be invisible initially, so no signal
   * should be emitted.
   */
  ctk_tree_store_append (fixture.store, &root, NULL);
  create_tree_store_set_values (fixture.store, &root, FALSE);

  signal_monitor_assert_is_empty (fixture.monitor);
  /* check_filter_model (&fixture); */
  check_level_length (fixture.filter, NULL, 0);

  /* Add a child node.  This will cause the parent to become visible,
   * so we expect row-inserted signals for both.
   */
  signal_monitor_append_signal (fixture.monitor, ROW_INSERTED, "0");
  signal_monitor_append_signal (fixture.monitor, ROW_HAS_CHILD_TOGGLED, "0");
  signal_monitor_append_signal (fixture.monitor, ROW_HAS_CHILD_TOGGLED, "0");

  ctk_tree_store_append (fixture.store, &iter, &root);
  signal_monitor_assert_is_empty (fixture.monitor);

  check_level_length (fixture.filter, NULL, 1);
  check_level_length (fixture.filter, "0", 1);

  /* Modify the content of iter, no signals because the parent is not
   * expanded.
   */
  create_tree_store_set_values (fixture.store, &iter, TRUE);
  signal_monitor_assert_is_empty (fixture.monitor);

  /* Parent must now be visible.  Do the level length check first,
   * to avoid modifying the child model triggering a row-changed to
   * the filter model.
   */
  check_level_length (fixture.filter, NULL, 1);
  check_level_length (fixture.filter, "0", 1);

  /* Modify path 0 */
  signal_monitor_append_signal (fixture.monitor, ROW_CHANGED, "0");
  signal_monitor_append_signal (fixture.monitor, ROW_HAS_CHILD_TOGGLED, "0");

  set_path_visibility (&fixture, "0", TRUE);
  /* check_filter_model (&fixture); */

  signal_monitor_assert_is_empty (fixture.monitor);

  /* Insert another node in the root level.  Initially invisible, so
   * not expecting any signal.
   */
  ctk_tree_store_append (fixture.store, &root, NULL);
  check_level_length (fixture.filter, NULL, 1);

  signal_monitor_assert_is_empty (fixture.monitor);

  /* Adding a child node which also makes parent at path 1 visible. */
  signal_monitor_append_signal (fixture.monitor, ROW_INSERTED, "1");
  signal_monitor_append_signal (fixture.monitor, ROW_HAS_CHILD_TOGGLED, "1");
  signal_monitor_append_signal (fixture.monitor, ROW_HAS_CHILD_TOGGLED, "1");

  ctk_tree_store_append (fixture.store, &iter, &root);
  check_level_length (fixture.filter, NULL, 2);
  check_level_length (fixture.filter, "1", 1);

  signal_monitor_assert_is_empty (fixture.monitor);

  /* Check if row-changed is propagated */
  signal_monitor_append_signal (fixture.monitor, ROW_CHANGED, "1");
  signal_monitor_append_signal (fixture.monitor, ROW_HAS_CHILD_TOGGLED, "1");

  create_tree_store_set_values (fixture.store, &root, TRUE);
  create_tree_store_set_values (fixture.store, &iter, TRUE);
  /* check_filter_model (&fixture); */
  signal_monitor_assert_is_empty (fixture.monitor);

  /* Insert another child under node 1 */
  ctk_tree_store_append (fixture.store, &iter, &root);
  create_tree_store_set_values (fixture.store, &iter, TRUE);
  check_level_length (fixture.filter, NULL, 2);
  check_level_length (fixture.filter, "0", 1);
  check_level_length (fixture.filter, "1", 2);
  signal_monitor_assert_is_empty (fixture.monitor);

  /* Set a child node to invisible.  This should not yield any
   * change, because filtering is only done on whether the root
   * node has a child, which it still has.
   */
  set_path_visibility (&fixture, "0:0", FALSE);
  signal_monitor_assert_is_empty (fixture.monitor);

  /* Now remove one of the remaining child rows */
  signal_monitor_append_signal (fixture.monitor, ROW_HAS_CHILD_TOGGLED, "0");
  signal_monitor_append_signal (fixture.monitor, ROW_DELETED, "0");

  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture.store),
                                       &iter, "0:0");
  ctk_tree_store_remove (fixture.store, &iter);

  check_level_length (fixture.filter, NULL, 1);
  check_level_length (fixture.filter, "0", 2);
  signal_monitor_assert_is_empty (fixture.monitor);

  /* Set visibility of 0 to FALSE, no-op for filter model since
   * the child 0:0 is already gone
   */
  set_path_visibility (&fixture, "0", FALSE);
  /* check_filter_model (&fixture); */
  signal_monitor_assert_is_empty (fixture.monitor);

  g_object_unref (fixture.filter);
  g_object_unref (fixture.store);
  ctk_widget_destroy (tree_view);
}

static void
specific_has_child_filter_on_sort_model (void)
{
  CtkTreeModel *filter;
  CtkTreeModel *sort_model;
  CtkTreeIter iter, root;
  FilterTest fixture; /* This is not how it should be done */
  CtkWidget *tree_view;

  fixture.store = ctk_tree_store_new (2, G_TYPE_STRING, G_TYPE_BOOLEAN);
  sort_model = ctk_tree_model_sort_new_with_model (CTK_TREE_MODEL (fixture.store));
  filter = ctk_tree_model_filter_new (sort_model, NULL);
  fixture.filter = CTK_TREE_MODEL_FILTER (filter);
  fixture.monitor = signal_monitor_new (filter);

  tree_view = ctk_tree_view_new_with_model (filter);

  /* We will filter on parent state using a filter function.  We will
   * manually keep the boolean column in sync, so that we can use
   * check_filter_model() to check the consistency of the model.
   */
  /* FIXME: We need a check_filter_model() that is not tied to LEVEL_LENGTH
   * to be able to check the structure here.  We keep the calls to
   * check_filter_model() commented out until then.
   */
  ctk_tree_model_filter_set_visible_func (fixture.filter,
                                          specific_has_child_filter_filter_func,
                                          NULL, NULL);

  /* The first node will be initially invisible: no signals */
  ctk_tree_store_append (fixture.store, &root, NULL);
  create_tree_store_set_values (fixture.store, &root, FALSE);

  /* check_filter_model (&fixture); */
  check_level_length (fixture.filter, NULL, 0);
  signal_monitor_assert_is_empty (fixture.monitor);

  /* Insert a child node. This will cause the parent to become visible
   * since there is a child now.
   */
  signal_monitor_append_signal (fixture.monitor, ROW_INSERTED, "0");
  signal_monitor_append_signal (fixture.monitor, ROW_HAS_CHILD_TOGGLED, "0");

  ctk_tree_store_append (fixture.store, &iter, &root);
  create_tree_store_set_values (fixture.store, &iter, TRUE);

  /* Parent must now be visible.  Do the level length check first,
   * to avoid modifying the child model triggering a row-changed to
   * the filter model.
   */
  check_level_length (fixture.filter, NULL, 1);
  check_level_length (fixture.filter, "0", 0);
  signal_monitor_assert_is_empty (fixture.monitor);

  /* This should propagate row-changed */
  signal_monitor_append_signal (fixture.monitor, ROW_CHANGED, "0");
  signal_monitor_append_signal (fixture.monitor, ROW_HAS_CHILD_TOGGLED, "0");

  set_path_visibility (&fixture, "0", TRUE);
  /* check_filter_model (&fixture); */
  signal_monitor_assert_is_empty (fixture.monitor);

  /* New root node, no child, so no signal */
  ctk_tree_store_append (fixture.store, &root, NULL);
  check_level_length (fixture.filter, NULL, 1);
  signal_monitor_assert_is_empty (fixture.monitor);

  /* When the child comes in, this node will become visible */
  signal_monitor_append_signal (fixture.monitor, ROW_INSERTED, "1");
  signal_monitor_append_signal (fixture.monitor, ROW_HAS_CHILD_TOGGLED, "1");
  signal_monitor_append_signal (fixture.monitor, ROW_CHANGED, "1");
  signal_monitor_append_signal (fixture.monitor, ROW_HAS_CHILD_TOGGLED, "1");

  ctk_tree_store_append (fixture.store, &iter, &root);
  check_level_length (fixture.filter, NULL, 2);
  check_level_length (fixture.filter, "1", 0);

  create_tree_store_set_values (fixture.store, &root, TRUE);
  create_tree_store_set_values (fixture.store, &iter, TRUE);

  /* check_filter_model (&fixture); */
  signal_monitor_assert_is_empty (fixture.monitor);

  /* Add another child for 1 */
  ctk_tree_store_append (fixture.store, &iter, &root);
  create_tree_store_set_values (fixture.store, &iter, TRUE);
  check_level_length (fixture.filter, NULL, 2);
  check_level_length (fixture.filter, "0", 0);
  check_level_length (fixture.filter, "1", 0);
  signal_monitor_assert_is_empty (fixture.monitor);

  /* Now remove one of the remaining child rows */
  signal_monitor_append_signal (fixture.monitor, ROW_DELETED, "0");

  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture.store),
                                       &iter, "0:0");
  ctk_tree_store_remove (fixture.store, &iter);

  check_level_length (fixture.filter, NULL, 1);
  check_level_length (fixture.filter, "0", 0);

  set_path_visibility (&fixture, "0", FALSE);
  /* check_filter_model (&fixture); */
  signal_monitor_assert_is_empty (fixture.monitor);

  g_object_unref (fixture.filter);
  g_object_unref (fixture.store);
  ctk_widget_destroy (tree_view);
}

static gboolean
specific_at_least_2_children_filter_filter_func (CtkTreeModel *model,
                                                 CtkTreeIter  *iter,
                                                 gpointer      data)
{
  return ctk_tree_model_iter_n_children (model, iter) >= 2;
}

static void
specific_at_least_2_children_filter (void)
{
  CtkTreeModel *filter;
  CtkTreeIter iter, root;
  FilterTest fixture; /* This is not how it should be done */
  CtkWidget *tree_view;

  fixture.store = ctk_tree_store_new (2, G_TYPE_STRING, G_TYPE_BOOLEAN);
  filter = ctk_tree_model_filter_new (CTK_TREE_MODEL (fixture.store), NULL);
  fixture.filter = CTK_TREE_MODEL_FILTER (filter);
  fixture.monitor = signal_monitor_new (filter);

  tree_view = ctk_tree_view_new_with_model (filter);

  ctk_tree_model_filter_set_visible_func (fixture.filter,
                                          specific_at_least_2_children_filter_filter_func,
                                          NULL, NULL);

  /* The first node will be initially invisible: no signals */
  ctk_tree_store_append (fixture.store, &root, NULL);
  create_tree_store_set_values (fixture.store, &root, FALSE);

  /* check_filter_model (&fixture); */
  check_level_length (fixture.filter, NULL, 0);
  signal_monitor_assert_is_empty (fixture.monitor);

  /* Insert a child node.  Nothing should happen.
   */
  ctk_tree_store_append (fixture.store, &iter, &root);
  create_tree_store_set_values (fixture.store, &iter, TRUE);

  check_level_length (fixture.filter, NULL, 0);
  signal_monitor_assert_is_empty (fixture.monitor);

  /* Insert a second child node.  This will cause the parent to become
   * visible.
   */
  signal_monitor_append_signal (fixture.monitor, ROW_INSERTED, "0");
  signal_monitor_append_signal (fixture.monitor, ROW_HAS_CHILD_TOGGLED, "0");

  ctk_tree_store_append (fixture.store, &iter, &root);
  create_tree_store_set_values (fixture.store, &iter, TRUE);

  /* Parent must now be visible.  Do the level length check first,
   * to avoid modifying the child model triggering a row-changed to
   * the filter model.
   */
  check_level_length (fixture.filter, NULL, 1);
  check_level_length (fixture.filter, "0", 0);
  signal_monitor_assert_is_empty (fixture.monitor);

  /* This should propagate row-changed */
  signal_monitor_append_signal (fixture.monitor, ROW_CHANGED, "0");
  signal_monitor_append_signal (fixture.monitor, ROW_HAS_CHILD_TOGGLED, "0");

  set_path_visibility (&fixture, "0", TRUE);
  /* check_filter_model (&fixture); */
  signal_monitor_assert_is_empty (fixture.monitor);

  /* New root node, no child, so no signal */
  ctk_tree_store_append (fixture.store, &root, NULL);
  check_level_length (fixture.filter, NULL, 1);
  signal_monitor_assert_is_empty (fixture.monitor);

  /* First child, no signal, no change */
  ctk_tree_store_append (fixture.store, &iter, &root);
  check_level_length (fixture.filter, NULL, 1);
  signal_monitor_assert_is_empty (fixture.monitor);

  /* When the second child comes in, this node will become visible */
  signal_monitor_append_signal (fixture.monitor, ROW_INSERTED, "1");
  signal_monitor_append_signal (fixture.monitor, ROW_HAS_CHILD_TOGGLED, "1");
  signal_monitor_append_signal (fixture.monitor, ROW_CHANGED, "1");
  signal_monitor_append_signal (fixture.monitor, ROW_HAS_CHILD_TOGGLED, "1");

  ctk_tree_store_append (fixture.store, &iter, &root);
  check_level_length (fixture.filter, NULL, 2);
  check_level_length (fixture.filter, "1", 0);

  create_tree_store_set_values (fixture.store, &root, TRUE);
  create_tree_store_set_values (fixture.store, &iter, TRUE);

  /* check_filter_model (&fixture); */
  signal_monitor_assert_is_empty (fixture.monitor);

  /* Add another child for 1 */
  ctk_tree_store_append (fixture.store, &iter, &root);
  create_tree_store_set_values (fixture.store, &iter, TRUE);
  check_level_length (fixture.filter, NULL, 2);
  check_level_length (fixture.filter, "0", 0);
  check_level_length (fixture.filter, "1", 0);
  signal_monitor_assert_is_empty (fixture.monitor);

  /* Now remove one of the remaining child rows */
  signal_monitor_append_signal (fixture.monitor, ROW_DELETED, "0");

  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture.store),
                                       &iter, "0:0");
  ctk_tree_store_remove (fixture.store, &iter);

  check_level_length (fixture.filter, NULL, 1);
  check_level_length (fixture.filter, "0", 0);

  set_path_visibility (&fixture, "0", FALSE);
  /* check_filter_model (&fixture); */
  signal_monitor_assert_is_empty (fixture.monitor);

  g_object_unref (fixture.filter);
  g_object_unref (fixture.store);
  ctk_widget_destroy (tree_view);
}

static void
specific_at_least_2_children_filter_on_sort_model (void)
{
  CtkTreeRowReference *ref;
  CtkTreeModel *filter;
  CtkTreeModel *sort_model;
  CtkTreeIter iter, root;
  FilterTest fixture; /* This is not how it should be done */
  CtkWidget *tree_view;

  fixture.store = ctk_tree_store_new (2, G_TYPE_STRING, G_TYPE_BOOLEAN);
  sort_model = ctk_tree_model_sort_new_with_model (CTK_TREE_MODEL (fixture.store));
  filter = ctk_tree_model_filter_new (sort_model, NULL);
  fixture.filter = CTK_TREE_MODEL_FILTER (filter);
  fixture.monitor = signal_monitor_new (filter);

  tree_view = ctk_tree_view_new_with_model (filter);

  ctk_tree_model_filter_set_visible_func (fixture.filter,
                                          specific_at_least_2_children_filter_filter_func,
                                          NULL, NULL);

  /* The first node will be initially invisible: no signals */
  ctk_tree_store_append (fixture.store, &root, NULL);
  create_tree_store_set_values (fixture.store, &root, FALSE);

  /* check_filter_model (&fixture); */
  check_level_length (fixture.filter, NULL, 0);
  signal_monitor_assert_is_empty (fixture.monitor);

  /* Insert a child node.  Nothing should happen.
   */
  ctk_tree_store_append (fixture.store, &iter, &root);
  create_tree_store_set_values (fixture.store, &iter, TRUE);

  check_level_length (fixture.filter, NULL, 0);
  signal_monitor_assert_is_empty (fixture.monitor);

    {
      CtkTreePath *path = ctk_tree_path_new_from_indices (0, 0, -1);

      ref = ctk_tree_row_reference_new (sort_model, path);
      ctk_tree_path_free (path);
    }

  /* Insert a second child node.  This will cause the parent to become
   * visible.
   */
  signal_monitor_append_signal (fixture.monitor, ROW_INSERTED, "0");
  signal_monitor_append_signal (fixture.monitor, ROW_HAS_CHILD_TOGGLED, "0");

  ctk_tree_store_append (fixture.store, &iter, &root);
  create_tree_store_set_values (fixture.store, &iter, TRUE);

  /* Parent must now be visible.  Do the level length check first,
   * to avoid modifying the child model triggering a row-changed to
   * the filter model.
   */
  check_level_length (fixture.filter, NULL, 1);
  check_level_length (fixture.filter, "0", 0);
  signal_monitor_assert_is_empty (fixture.monitor);

  /* This should propagate row-changed */
  signal_monitor_append_signal (fixture.monitor, ROW_CHANGED, "0");
  signal_monitor_append_signal (fixture.monitor, ROW_HAS_CHILD_TOGGLED, "0");

  set_path_visibility (&fixture, "0", TRUE);
  /* check_filter_model (&fixture); */
  signal_monitor_assert_is_empty (fixture.monitor);

  /* New root node, no child, so no signal */
  ctk_tree_store_append (fixture.store, &root, NULL);
  check_level_length (fixture.filter, NULL, 1);
  signal_monitor_assert_is_empty (fixture.monitor);

  ctk_tree_row_reference_free (ref);
  g_object_unref (fixture.filter);
  g_object_unref (fixture.store);
  ctk_widget_destroy (tree_view);
}


static void
specific_filter_add_child (void)
{
  /* This test is based on one of the test cases I found in my
   * old test cases directory.  I unfortunately do not have a record
   * from who this test case originated.  -Kris.
   */

  CtkTreeIter iter;
  CtkTreeIter iter_first;
  CtkTreeIter child;
  CtkTreeStore *store;
  CtkTreeModel *filter G_GNUC_UNUSED;

  store = ctk_tree_store_new (1, G_TYPE_STRING);

  ctk_tree_store_append (store, &iter_first, NULL);
  ctk_tree_store_set (store, &iter_first, 0, "Hello", -1);

  ctk_tree_store_append (store, &iter, NULL);
  ctk_tree_store_set (store, &iter, 0, "Hello", -1);

  ctk_tree_store_append (store, &iter, NULL);
  ctk_tree_store_set (store, &iter, 0, "Hello", -1);

  ctk_tree_store_append (store, &iter, NULL);
  ctk_tree_store_set (store, &iter, 0, "Hello", -1);

  filter = ctk_tree_model_filter_new (CTK_TREE_MODEL (store), NULL);

  ctk_tree_store_set (store, &iter, 0, "Hello", -1);
  ctk_tree_store_append (store, &child, &iter_first);
  ctk_tree_store_set (store, &child, 0, "Hello", -1);
}

static void
specific_list_store_clear (void)
{
  CtkTreeIter iter;
  CtkListStore *list;
  CtkTreeModel *filter;
  CtkWidget *view G_GNUC_UNUSED;

  list = ctk_list_store_new (1, G_TYPE_INT);
  ctk_list_store_insert_with_values (list, &iter, 0, 0, 1, -1);
  ctk_list_store_insert_with_values (list, &iter, 1, 0, 2, -1);
  ctk_list_store_insert_with_values (list, &iter, 2, 0, 3, -1);
  ctk_list_store_insert_with_values (list, &iter, 3, 0, 4, -1);
  ctk_list_store_insert_with_values (list, &iter, 4, 0, 5, -1);
  ctk_list_store_insert_with_values (list, &iter, 5, 0, 6, -1);
  ctk_list_store_insert_with_values (list, &iter, 6, 0, 7, -1);
  ctk_list_store_insert_with_values (list, &iter, 7, 0, 8, -1);

  filter = ctk_tree_model_filter_new (CTK_TREE_MODEL (list), NULL);
  view = ctk_tree_view_new_with_model (filter);

  ctk_list_store_clear (list);
}

static void
specific_sort_ref_leaf_and_remove_ancestor (void)
{
  CtkTreeIter iter, child, child2, child3;
  CtkTreeStore *tree;
  CtkTreeModel *sort;
  CtkTreePath *path;
  CtkTreeRowReference *rowref;
  CtkWidget *view G_GNUC_UNUSED;

  tree = ctk_tree_store_new (1, G_TYPE_INT);
  ctk_tree_store_insert_with_values (tree, &iter, NULL, 0, 0, 1, -1);
  ctk_tree_store_insert_with_values (tree, &iter, NULL, 1, 0, 2, -1);
  ctk_tree_store_insert_with_values (tree, &iter, NULL, 2, 0, 3, -1);
  ctk_tree_store_insert_with_values (tree, &iter, NULL, 3, 0, 4, -1);

  ctk_tree_store_insert_with_values (tree, &child, &iter, 0, 0, 50, -1);
  ctk_tree_store_insert_with_values (tree, &child2, &child, 0, 0, 6, -1);
  ctk_tree_store_insert_with_values (tree, &child3, &child2, 0, 0, 7, -1);

  sort = ctk_tree_model_sort_new_with_model (CTK_TREE_MODEL (tree));
  view = ctk_tree_view_new_with_model (sort);
  ctk_tree_view_expand_all (CTK_TREE_VIEW (view));

  path = ctk_tree_path_new_from_indices (3, 0, 0, 0, -1);
  rowref = ctk_tree_row_reference_new (sort, path);
  ctk_tree_path_free (path);

  path = ctk_tree_path_new_from_indices (3, 0, 0, 0, -1);
  rowref = ctk_tree_row_reference_new (sort, path);
  ctk_tree_path_free (path);

  path = ctk_tree_path_new_from_indices (3, 0, -1);
  rowref = ctk_tree_row_reference_new (sort, path);
  ctk_tree_path_free (path);

  path = ctk_tree_path_new_from_indices (3, -1);
  rowref = ctk_tree_row_reference_new (sort, path);
  ctk_tree_path_free (path);

  /* Deleting a parent */
  path = ctk_tree_path_new_from_indices (3, 0, -1);
  ctk_tree_model_get_iter (CTK_TREE_MODEL (tree), &iter, path);
  ctk_tree_store_remove (tree, &iter);
  ctk_tree_path_free (path);

  ctk_tree_row_reference_free (rowref);
}

static void
specific_ref_leaf_and_remove_ancestor (void)
{
  CtkTreeIter iter, child, child2, child3;
  CtkTreeStore *tree;
  CtkTreeModel *filter;
  CtkTreePath *path;
  CtkTreeRowReference *rowref;
  CtkWidget *view G_GNUC_UNUSED;

  tree = ctk_tree_store_new (1, G_TYPE_INT);
  ctk_tree_store_insert_with_values (tree, &iter, NULL, 0, 0, 1, -1);
  ctk_tree_store_insert_with_values (tree, &iter, NULL, 1, 0, 2, -1);
  ctk_tree_store_insert_with_values (tree, &iter, NULL, 2, 0, 3, -1);
  ctk_tree_store_insert_with_values (tree, &iter, NULL, 3, 0, 4, -1);

  ctk_tree_store_insert_with_values (tree, &child, &iter, 0, 0, 50, -1);
  ctk_tree_store_insert_with_values (tree, &child2, &child, 0, 0, 6, -1);
  ctk_tree_store_insert_with_values (tree, &child3, &child2, 0, 0, 7, -1);

  filter = ctk_tree_model_filter_new (CTK_TREE_MODEL (tree), NULL);
  view = ctk_tree_view_new_with_model (filter);
  ctk_tree_view_expand_all (CTK_TREE_VIEW (view));

  path = ctk_tree_path_new_from_indices (3, 0, 0, 0, -1);
  rowref = ctk_tree_row_reference_new (filter, path);
  ctk_tree_path_free (path);

  path = ctk_tree_path_new_from_indices (3, 0, 0, 0, -1);
  rowref = ctk_tree_row_reference_new (filter, path);
  ctk_tree_path_free (path);

  path = ctk_tree_path_new_from_indices (3, 0, -1);
  rowref = ctk_tree_row_reference_new (filter, path);
  ctk_tree_path_free (path);

  path = ctk_tree_path_new_from_indices (3, -1);
  rowref = ctk_tree_row_reference_new (filter, path);
  ctk_tree_path_free (path);

  /* Deleting a parent */
  path = ctk_tree_path_new_from_indices (3, 0, -1);
  ctk_tree_model_get_iter (CTK_TREE_MODEL (tree), &iter, path);
  ctk_tree_store_remove (tree, &iter);
  ctk_tree_path_free (path);

  ctk_tree_row_reference_free (rowref);
}

static void
specific_virtual_ref_leaf_and_remove_ancestor (void)
{
  CtkTreeIter iter, child, child2, child3;
  CtkTreeStore *tree;
  CtkTreeModel *filter;
  CtkTreePath *path;
  CtkTreeRowReference *rowref;
  CtkWidget *view G_GNUC_UNUSED;

  tree = ctk_tree_store_new (1, G_TYPE_INT);
  ctk_tree_store_insert_with_values (tree, &iter, NULL, 0, 0, 1, -1);
  ctk_tree_store_insert_with_values (tree, &iter, NULL, 1, 0, 2, -1);
  ctk_tree_store_insert_with_values (tree, &iter, NULL, 2, 0, 3, -1);
  ctk_tree_store_insert_with_values (tree, &iter, NULL, 3, 0, 4, -1);

  ctk_tree_store_insert_with_values (tree, &child, &iter, 0, 0, 50, -1);
  ctk_tree_store_insert_with_values (tree, &child2, &child, 0, 0, 6, -1);
  ctk_tree_store_insert_with_values (tree, &child3, &child2, 0, 0, 7, -1);

  /* Set a virtual root of 3:0 */
  path = ctk_tree_path_new_from_indices (3, 0, -1);
  filter = ctk_tree_model_filter_new (CTK_TREE_MODEL (tree), path);
  ctk_tree_path_free (path);

  view = ctk_tree_view_new_with_model (filter);
  ctk_tree_view_expand_all (CTK_TREE_VIEW (view));

  path = ctk_tree_path_new_from_indices (0, 0, -1);
  rowref = ctk_tree_row_reference_new (filter, path);
  ctk_tree_path_free (path);

  path = ctk_tree_path_new_from_indices (0, 0, -1);
  rowref = ctk_tree_row_reference_new (filter, path);
  ctk_tree_path_free (path);

  path = ctk_tree_path_new_from_indices (0, -1);
  rowref = ctk_tree_row_reference_new (filter, path);
  ctk_tree_path_free (path);

  /* Deleting the virtual root */
  path = ctk_tree_path_new_from_indices (3, 0, -1);
  ctk_tree_model_get_iter (CTK_TREE_MODEL (tree), &iter, path);
  ctk_tree_store_remove (tree, &iter);
  ctk_tree_path_free (path);

  ctk_tree_row_reference_free (rowref);
}


static int
specific_bug_301558_sort_func (CtkTreeModel *model,
                               CtkTreeIter  *a,
                               CtkTreeIter  *b,
                               gpointer      data)
{
  int i, j;

  ctk_tree_model_get (model, a, 0, &i, -1);
  ctk_tree_model_get (model, b, 0, &j, -1);

  return j - i;
}

static void
specific_bug_301558 (void)
{
  /* Test case for GNOME Bugzilla bug 301558 provided by
   * Markku Vire.
   */
  CtkTreeStore *tree;
  CtkTreeModel *filter;
  CtkTreeModel *sort;
  CtkTreeIter root, iter, iter2;
  CtkWidget *view G_GNUC_UNUSED;
  int i;
  gboolean add;

  g_test_bug ("301558");

  tree = ctk_tree_store_new (2, G_TYPE_INT, G_TYPE_BOOLEAN);
  ctk_tree_store_append (tree, &iter, NULL);
  ctk_tree_store_set (tree, &iter, 0, 123, 1, TRUE, -1);
  ctk_tree_store_append (tree, &iter2, &iter);
  ctk_tree_store_set (tree, &iter2, 0, 73, 1, TRUE, -1);

  sort = ctk_tree_model_sort_new_with_model (CTK_TREE_MODEL (tree));
  ctk_tree_sortable_set_default_sort_func (CTK_TREE_SORTABLE (sort),
                                           specific_bug_301558_sort_func,
                                           NULL, NULL);

  filter = ctk_tree_model_filter_new (sort, NULL);
  ctk_tree_model_filter_set_visible_column (CTK_TREE_MODEL_FILTER (filter), 1);

  view = ctk_tree_view_new_with_model (filter);

  while (ctk_events_pending ())
    ctk_main_iteration ();

  add = TRUE;

  for (i = 0; i < 10; i++)
    {
      if (!ctk_tree_model_get_iter_first (CTK_TREE_MODEL (tree), &root))
        g_assert_not_reached ();

      if (add)
        {
          ctk_tree_store_append (tree, &iter, &root);
          ctk_tree_store_set (tree, &iter, 0, 456, 1, TRUE, -1);
        }
      else
        {
          int n;
          n = ctk_tree_model_iter_n_children (CTK_TREE_MODEL (tree), &root);
          ctk_tree_model_iter_nth_child (CTK_TREE_MODEL (tree), &iter,
                                         &root, n - 1);
          ctk_tree_store_remove (tree, &iter);
        }

      add = !add;
    }
}


static gboolean
specific_bug_311955_filter_func (CtkTreeModel *model,
                                 CtkTreeIter  *iter,
                                 gpointer      data)
{
  int value;

  ctk_tree_model_get (model, iter, 0, &value, -1);

  return (value != 0);
}

static void
specific_bug_311955 (void)
{
  /* This is a test case for GNOME Bugzilla bug 311955.  It was written
   * by Markku Vire.
   */
  CtkTreeIter iter, child, root;
  CtkTreeStore *store;
  CtkTreeModel *sort;
  CtkTreeModel *filter;

  CtkWidget *window G_GNUC_UNUSED;
  CtkWidget *tree_view;
  int i;
  int n;
  CtkTreePath *path;

  g_test_bug ("311955");

  store = ctk_tree_store_new (1, G_TYPE_INT);

  ctk_tree_store_append (store, &root, NULL);
  ctk_tree_store_set (store, &root, 0, 33, -1);

  ctk_tree_store_append (store, &iter, &root);
  ctk_tree_store_set (store, &iter, 0, 50, -1);

  ctk_tree_store_append (store, &iter, NULL);
  ctk_tree_store_set (store, &iter, 0, 22, -1);

  sort = ctk_tree_model_sort_new_with_model (CTK_TREE_MODEL (store));
  filter = ctk_tree_model_filter_new (sort, NULL);

  ctk_tree_model_filter_set_visible_func (CTK_TREE_MODEL_FILTER (filter),
                                          specific_bug_311955_filter_func,
                                          NULL, NULL);

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  tree_view = ctk_tree_view_new_with_model (filter);
  g_object_unref (store);

  ctk_tree_view_expand_all (CTK_TREE_VIEW (tree_view));

  while (ctk_events_pending ())
    ctk_main_iteration ();

  check_level_length (CTK_TREE_MODEL_FILTER (filter), NULL, 2);
  check_level_length (CTK_TREE_MODEL_FILTER (filter), "0", 1);

  /* Fill model */
  for (i = 0; i < 4; i++)
    {
      ctk_tree_model_get_iter_first (CTK_TREE_MODEL (store), &root);

      ctk_tree_store_append (store, &iter, &root);

      if (i < 3)
        ctk_tree_store_set (store, &iter, 0, i, -1);

      if (i % 2 == 0)
        {
          ctk_tree_store_append (store, &child, &iter);
          ctk_tree_store_set (store, &child, 0, 10, -1);
        }
    }

  while (ctk_events_pending ())
    ctk_main_iteration ();

  check_level_length (CTK_TREE_MODEL_FILTER (filter), "0", 3);
  check_level_length (CTK_TREE_MODEL_FILTER (filter), "0:2", 1);

  /* Remove bottommost child from the tree. */
  ctk_tree_model_get_iter_first (CTK_TREE_MODEL (store), &root);
  n = ctk_tree_model_iter_n_children (CTK_TREE_MODEL (store), &root);

  if (ctk_tree_model_iter_nth_child (CTK_TREE_MODEL (store), &iter, &root, n - 2))
    {
      if (ctk_tree_model_iter_children (CTK_TREE_MODEL (store), &child, &iter))
        ctk_tree_store_remove (store, &child);
    }
  else
    g_assert_not_reached ();

  path = ctk_tree_path_new_from_indices (0, 2, -1);
  ctk_tree_view_expand_row (CTK_TREE_VIEW (tree_view), path, FALSE);
  ctk_tree_path_free (path);

  check_level_length (CTK_TREE_MODEL_FILTER (filter), "0", 3);
  check_level_length (CTK_TREE_MODEL_FILTER (filter), "0:2", 0);
}

static void
specific_bug_311955_clean (void)
{
  /* Cleaned up version of the test case for GNOME Bugzilla bug 311955,
   * which is easier to understand.
   */
  CtkTreeIter iter, child, grandchild;
  CtkTreeStore *store;
  CtkTreeModel *sort;
  CtkTreeModel *filter;

  CtkWidget *tree_view;
  CtkTreePath *path;

  store = ctk_tree_store_new (1, G_TYPE_INT);

  ctk_tree_store_append (store, &iter, NULL);
  ctk_tree_store_set (store, &iter, 0, 1, -1);

  ctk_tree_store_append (store, &child, &iter);
  ctk_tree_store_set (store, &child, 0, 1, -1);

  sort = ctk_tree_model_sort_new_with_model (CTK_TREE_MODEL (store));
  filter = ctk_tree_model_filter_new (sort, NULL);

  ctk_tree_model_filter_set_visible_func (CTK_TREE_MODEL_FILTER (filter),
                                          specific_bug_311955_filter_func,
                                          NULL, NULL);

  tree_view = ctk_tree_view_new_with_model (filter);
  g_object_unref (store);

  ctk_tree_view_expand_all (CTK_TREE_VIEW (tree_view));

  while (ctk_events_pending ())
    ctk_main_iteration ();

  check_level_length (CTK_TREE_MODEL_FILTER (filter), NULL, 1);
  check_level_length (CTK_TREE_MODEL_FILTER (filter), "0", 1);

  ctk_tree_model_get_iter_first (CTK_TREE_MODEL (store), &iter);

  ctk_tree_store_append (store, &child, &iter);
  ctk_tree_store_set (store, &child, 0, 0, -1);

  ctk_tree_store_append (store, &child, &iter);
  ctk_tree_store_set (store, &child, 0, 1, -1);

  ctk_tree_store_append (store, &child, &iter);
  ctk_tree_store_set (store, &child, 0, 1, -1);

  ctk_tree_store_append (store, &grandchild, &child);
  ctk_tree_store_set (store, &grandchild, 0, 1, -1);

  ctk_tree_store_append (store, &child, &iter);
  /* Don't set a value: assume 0 */

  /* Remove leaf node, check trigger row-has-child-toggled */
  path = ctk_tree_path_new_from_indices (0, 3, 0, -1);
  ctk_tree_model_get_iter (CTK_TREE_MODEL (store), &iter, path);
  ctk_tree_path_free (path);
  ctk_tree_store_remove (store, &iter);

  path = ctk_tree_path_new_from_indices (0, 2, -1);
  ctk_tree_view_expand_row (CTK_TREE_VIEW (tree_view), path, FALSE);
  ctk_tree_path_free (path);

  check_level_length (CTK_TREE_MODEL_FILTER (filter), "0", 3);
  check_level_length (CTK_TREE_MODEL_FILTER (filter), "0:2", 0);

  ctk_widget_destroy (tree_view);
}

static void
specific_bug_346800 (void)
{
  /* This is a test case for GNOME Bugzilla bug 346800.  It was written
   * by Jonathan Matthew.
   */

  CtkTreeIter node_iters[50];
  CtkTreeIter child_iters[50];
  CtkTreeModel *model;
  CtkTreeModelFilter *filter;
  CtkTreeStore *store;
  GType *columns;
  int i;
  int items = 50;
  columns = g_new (GType, 2);
  columns[0] = G_TYPE_STRING;
  columns[1] = G_TYPE_BOOLEAN;
  store = ctk_tree_store_newv (2, columns);
  model = CTK_TREE_MODEL (store);

  g_test_bug ("346800");

  filter = CTK_TREE_MODEL_FILTER (ctk_tree_model_filter_new (model, NULL));
  ctk_tree_model_filter_set_visible_column (filter, 1);

  for (i=0; i<items; i++)
    {
      /* allocate random amounts of junk, otherwise the filter model's arrays can expand without moving */

      g_malloc (138);
      ctk_tree_store_append (store, &node_iters[i], NULL);
      ctk_tree_store_set (store, &node_iters[i],
                          0, "something",
                          1, ((i%6) == 0) ? FALSE : TRUE,
                          -1);

      g_malloc (47);
      ctk_tree_store_append (store, &child_iters[i], &node_iters[i]);
      ctk_tree_store_set (store, &child_iters[i],
                          0, "something else",
                          1, FALSE,
                          -1);
      ctk_tree_model_filter_refilter (filter);

      if (i > 6)
        {
          ctk_tree_store_set (CTK_TREE_STORE (model), &child_iters[i-1], 1,
                              (i & 1) ? TRUE : FALSE, -1);
          ctk_tree_model_filter_refilter (filter);

          ctk_tree_store_set (CTK_TREE_STORE (model), &child_iters[i-2], 1,
                              (i & 1) ? FALSE: TRUE, -1);
          ctk_tree_model_filter_refilter (filter);
        }
    }
}

static gboolean
specific_bug_464173_visible_func (CtkTreeModel *model,
                                  CtkTreeIter  *iter,
                                  gpointer      data)
{
  gboolean *visible = (gboolean *)data;

  return *visible;
}

static void
specific_bug_464173 (void)
{
  /* Test case for GNOME Bugzilla bug 464173, test case written
   * by Andreas Koehler.
   */
  CtkTreeStore *model;
  CtkTreeModelFilter *f_model;
  CtkTreeIter iter1, iter2;
  CtkWidget *view G_GNUC_UNUSED;
  gboolean visible = TRUE;

  g_test_bug ("464173");

  model = ctk_tree_store_new (1, G_TYPE_STRING);
  ctk_tree_store_append (model, &iter1, NULL);
  ctk_tree_store_set (model, &iter1, 0, "Foo", -1);
  ctk_tree_store_append (model, &iter2, &iter1);
  ctk_tree_store_set (model, &iter2, 0, "Bar", -1);

  f_model = CTK_TREE_MODEL_FILTER (ctk_tree_model_filter_new (CTK_TREE_MODEL(model), NULL));
  ctk_tree_model_filter_set_visible_func (f_model,
                                          specific_bug_464173_visible_func,
                                          &visible, NULL);

  view = ctk_tree_view_new_with_model (CTK_TREE_MODEL (f_model));

  visible = FALSE;
  ctk_tree_model_filter_refilter (f_model);
}


static gboolean
specific_bug_540201_filter_func (CtkTreeModel *model,
                                 CtkTreeIter  *iter,
                                 gpointer      data)
{
  gboolean has_children;

  has_children = ctk_tree_model_iter_has_child (model, iter);

  return has_children;
}

static void
specific_bug_540201 (void)
{
  /* Test case for GNOME Bugzilla bug 540201, steps provided by
   * Charles Day.
   */
  CtkTreeIter iter, root;
  CtkTreeStore *store;
  CtkTreeModel *filter;

  CtkWidget *tree_view G_GNUC_UNUSED;

  g_test_bug ("540201");

  store = ctk_tree_store_new (1, G_TYPE_INT);

  ctk_tree_store_append (store, &root, NULL);
  ctk_tree_store_set (store, &root, 0, 33, -1);

  filter = ctk_tree_model_filter_new (CTK_TREE_MODEL (store), NULL);
  tree_view = ctk_tree_view_new_with_model (filter);

  ctk_tree_model_filter_set_visible_func (CTK_TREE_MODEL_FILTER (filter),
                                          specific_bug_540201_filter_func,
                                          NULL, NULL);

  ctk_tree_store_append (store, &iter, &root);
  ctk_tree_store_set (store, &iter, 0, 50, -1);

  ctk_tree_store_append (store, &iter, &root);
  ctk_tree_store_set (store, &iter, 0, 22, -1);


  ctk_tree_store_append (store, &root, NULL);
  ctk_tree_store_set (store, &root, 0, 33, -1);

  ctk_tree_store_append (store, &iter, &root);
  ctk_tree_store_set (store, &iter, 0, 22, -1);
}


static gboolean
specific_bug_549287_visible_func (CtkTreeModel *model,
                                  CtkTreeIter  *iter,
                                  gpointer      data)
{
  gboolean result = FALSE;

  result = ctk_tree_model_iter_has_child (model, iter);

  return result;
}

static void
specific_bug_549287 (void)
{
  /* Test case for GNOME Bugzilla bug 529287, provided by Julient Puydt */

  int i;
  CtkTreeStore *store;
  CtkTreeModel *filtered;
  CtkWidget *view G_GNUC_UNUSED;
  CtkTreeIter iter;
  CtkTreeIter *swap, *parent, *child;

  g_test_bug ("529287");

  store = ctk_tree_store_new (1, G_TYPE_STRING);
  filtered = ctk_tree_model_filter_new (CTK_TREE_MODEL (store), NULL);
  ctk_tree_model_filter_set_visible_func (CTK_TREE_MODEL_FILTER (filtered),
                                          specific_bug_549287_visible_func,
                                          NULL, NULL);

  view = ctk_tree_view_new_with_model (filtered);

  for (i = 0; i < 4; i++)
    {
      if (ctk_tree_model_get_iter_first (CTK_TREE_MODEL (store), &iter))
        {
          parent = ctk_tree_iter_copy (&iter);
          child = ctk_tree_iter_copy (&iter);

          while (ctk_tree_model_iter_nth_child (CTK_TREE_MODEL (store),
                                                child, parent, 0))
            {

              swap = parent;
              parent = child;
              child = swap;
            }

          ctk_tree_store_append (store, child, parent);
          ctk_tree_store_set (store, child,
                              0, "Something",
                              -1);

          ctk_tree_iter_free (parent);
          ctk_tree_iter_free (child);
        }
      else
        {
          ctk_tree_store_append (store, &iter, NULL);
          ctk_tree_store_set (store, &iter,
                              0, "Something",
                              -1);
        }

      /* since we inserted something, we changed the visibility conditions: */
      ctk_tree_model_filter_refilter (CTK_TREE_MODEL_FILTER (filtered));
    }
}

static gboolean
specific_bug_621076_visible_func (CtkTreeModel *model,
                                  CtkTreeIter  *iter,
                                  gpointer      data)
{
  gboolean visible = FALSE;
  gchar *str = NULL;

  ctk_tree_model_get (model, iter, 0, &str, -1);
  if (str != NULL && g_str_has_prefix (str, "visible"))
    {
      visible = TRUE;
    }
  else
    {
      CtkTreeIter child_iter;
      gboolean valid;

      /* Recursively check if we have a visible child */
      for (valid = ctk_tree_model_iter_children (model, &child_iter, iter);
           valid; valid = ctk_tree_model_iter_next (model, &child_iter))
        {
          if (specific_bug_621076_visible_func (model, &child_iter, data))
            {
              visible = TRUE;
              break;
            }
        }
    }

  g_free (str);

  return visible;
}

static void
specific_bug_621076 (void)
{
  /* Test case for GNOME Bugzilla bug 621076, provided by Xavier Claessens */

  /* This test case differs from has-child-filter and root-has-child-filter
   * in that the visible function both filters on content and model
   * structure.  Also, it is recursive.
   */

  CtkTreeStore *store;
  CtkTreeModel *filter;
  CtkWidget *view;
  CtkTreeIter group_iter;
  CtkTreeIter item_iter;
  SignalMonitor *monitor;

  g_test_bug ("621076");

  store = ctk_tree_store_new (1, G_TYPE_STRING);
  filter = ctk_tree_model_filter_new (CTK_TREE_MODEL (store), NULL);
  ctk_tree_model_filter_set_visible_func (CTK_TREE_MODEL_FILTER (filter),
                                          specific_bug_621076_visible_func,
                                          NULL, NULL);

  view = ctk_tree_view_new_with_model (filter);
  g_object_ref_sink (view);

  monitor = signal_monitor_new (filter);

  signal_monitor_append_signal (monitor, ROW_INSERTED, "0");
  ctk_tree_store_insert_with_values (store, &item_iter, NULL, -1,
                                     0, "visible-group-0",
                                     -1);
  signal_monitor_assert_is_empty (monitor);

  /* visible-group-0 is not expanded, so ROW_INSERTED should not be emitted
   * for its children. However, ROW_HAS_CHILD_TOGGLED should be emitted on
   * visible-group-0 to tell the view that row can be expanded. */
  signal_monitor_append_signal (monitor, ROW_HAS_CHILD_TOGGLED, "0");
  signal_monitor_append_signal (monitor, ROW_HAS_CHILD_TOGGLED, "0");
  group_iter = item_iter;
  ctk_tree_store_insert_with_values (store, &item_iter, &group_iter, -1,
                                     0, "visible-0:0",
                                     -1);
  signal_monitor_assert_is_empty (monitor);

  signal_monitor_append_signal (monitor, ROW_INSERTED, "1");
  ctk_tree_store_insert_with_values (store, &item_iter, NULL, -1,
                                     0, "visible-group-1",
                                     -1);
  signal_monitor_assert_is_empty (monitor);

  /* We are adding an hidden item inside visible-group-1, so
   * ROW_HAS_CHILD_TOGGLED should not be emitted.  It is emitted though,
   * because the signal originating at TreeStore will be propagated,
   * as well a generated signal because the state of the parent *could*
   * change by a change in the model.
   */
  signal_monitor_append_signal (monitor, ROW_HAS_CHILD_TOGGLED, "1");
  signal_monitor_append_signal (monitor, ROW_HAS_CHILD_TOGGLED, "1");
  group_iter = item_iter;
  ctk_tree_store_insert_with_values (store, &item_iter, &group_iter, -1,
                                     0, "group-1:0",
                                     -1);
  signal_monitor_assert_is_empty (monitor);

  /* This group is invisible and its parent too. Nothing should be emitted */
  group_iter = item_iter;
  ctk_tree_store_insert_with_values (store, &item_iter, &group_iter, -1,
                                     0, "group-1:0:0",
                                     -1);
  signal_monitor_assert_is_empty (monitor);

  /* Adding a visible item in this group hierarchy will make all nodes
   * in this path visible.  The first level should simply tell the view
   * that it now has a child, and the view will load the tree if needed
   * (depends on the expanded state).
   */
  signal_monitor_append_signal (monitor, ROW_HAS_CHILD_TOGGLED, "1");
  group_iter = item_iter;
  ctk_tree_store_insert_with_values (store, &item_iter, &group_iter, -1,
                                     0, "visible-1:0:0:0",
                                     -1);
  signal_monitor_assert_is_empty (monitor);

  check_level_length (CTK_TREE_MODEL_FILTER (filter), "1", 1);

  ctk_tree_store_insert_with_values (store, &item_iter, NULL, -1,
                                     0, "group-2",
                                     -1);
  signal_monitor_assert_is_empty (monitor);

  /* Parent is invisible, and adding this invisible item won't change that,
   * so no signal should be emitted. */
  group_iter = item_iter;
  ctk_tree_store_insert_with_values (store, NULL, &group_iter, -1,
                                     0, "invisible-2:0",
                                     -1);
  signal_monitor_assert_is_empty (monitor);

  /* This makes group-2 visible, so it gets inserted and tells it has
   * children.
   */
  signal_monitor_append_signal (monitor, ROW_INSERTED, "2");
  signal_monitor_append_signal (monitor, ROW_HAS_CHILD_TOGGLED, "2");
  ctk_tree_store_insert_with_values (store, NULL, &group_iter, -1,
                                     0, "visible-2:1",
                                     -1);
  signal_monitor_assert_is_empty (monitor);

  /* group-2 is already visible, so this time it is a normal insertion */
  ctk_tree_store_insert_with_values (store, NULL, &group_iter, -1,
                                     0, "visible-2:2",
                                     -1);
  signal_monitor_assert_is_empty (monitor);


  ctk_tree_store_insert_with_values (store, &item_iter, NULL, -1,
                                     0, "group-3",
                                     -1);
  signal_monitor_assert_is_empty (monitor);

  /* Parent is invisible, and adding this invisible item won't change that,
   * so no signal should be emitted. */
  group_iter = item_iter;
  ctk_tree_store_insert_with_values (store, NULL, &group_iter, -1,
                                     0, "invisible-3:0",
                                     -1);
  signal_monitor_assert_is_empty (monitor);

  ctk_tree_store_insert_with_values (store, &item_iter, &group_iter, -1,
                                     0, "invisible-3:1",
                                     -1);
  signal_monitor_assert_is_empty (monitor);

  /* This will make group 3 visible. */
  signal_monitor_append_signal (monitor, ROW_INSERTED, "3");
  signal_monitor_append_signal (monitor, ROW_HAS_CHILD_TOGGLED, "3");
  signal_monitor_append_signal (monitor, ROW_HAS_CHILD_TOGGLED, "3");
  ctk_tree_store_set (store, &item_iter, 0, "visible-3:1", -1);
  signal_monitor_assert_is_empty (monitor);

  /* Make sure all groups are expanded, so the filter has the tree cached */
  ctk_tree_view_expand_all (CTK_TREE_VIEW (view));
  while (ctk_events_pending ())
    ctk_main_iteration ();

  /* Should only yield a row-changed */
  signal_monitor_append_signal (monitor, ROW_CHANGED, "3:0");
  ctk_tree_store_set (store, &item_iter, 0, "visible-3:1", -1);
  signal_monitor_assert_is_empty (monitor);

  /* Now remove/hide some items. If a group loses its last item, the group
   * should be deleted instead of the item.
   */

  signal_monitor_append_signal (monitor, ROW_DELETED, "2:1");
  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (store), &item_iter, "2:2");
  ctk_tree_store_remove (store, &item_iter);
  signal_monitor_assert_is_empty (monitor);

  signal_monitor_append_signal (monitor, ROW_DELETED, "2:0");
  signal_monitor_append_signal (monitor, ROW_HAS_CHILD_TOGGLED, "2");
  signal_monitor_append_signal (monitor, ROW_DELETED, "2");
  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (store), &item_iter, "2:1");
  ctk_tree_store_set (store, &item_iter, 0, "invisible-2:1", -1);
  signal_monitor_assert_is_empty (monitor);

  signal_monitor_append_signal (monitor, ROW_DELETED, "1:0:0:0");
  signal_monitor_append_signal (monitor, ROW_HAS_CHILD_TOGGLED, "1:0:0");
  signal_monitor_append_signal (monitor, ROW_DELETED, "1:0");
  signal_monitor_append_signal (monitor, ROW_HAS_CHILD_TOGGLED, "1");
  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (store), &item_iter, "1:0:0:0");
  ctk_tree_store_remove (store, &item_iter);
  signal_monitor_assert_is_empty (monitor);

  /* Hide a group using row-changed instead of row-deleted */
  /* Caution: group 2 is gone, so offsets of the signals have moved. */
  signal_monitor_append_signal (monitor, ROW_DELETED, "2:0");
  signal_monitor_append_signal (monitor, ROW_HAS_CHILD_TOGGLED, "2");
  signal_monitor_append_signal (monitor, ROW_DELETED, "2");
  ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (store), &item_iter,
                                       "3:1");
  ctk_tree_store_set (store, &item_iter, 0, "invisible-3:1", -1);
  signal_monitor_assert_is_empty (monitor);

#if 0
  {
    CtkWidget *window;
    CtkTreeViewColumn *col;

    ctk_tree_view_expand_all (CTK_TREE_VIEW (view));

    col = ctk_tree_view_column_new_with_attributes ("foo",
        ctk_cell_renderer_text_new (),
        "text", 0, NULL);
    ctk_tree_view_append_column (CTK_TREE_VIEW (view), col);

    window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
    g_signal_connect (window, "delete-event",
        G_CALLBACK (ctk_widget_destroy), NULL);
    g_signal_connect (window, "destroy",
        G_CALLBACK (ctk_main_quit), NULL);

    ctk_container_add (CTK_CONTAINER (window), view);

    ctk_widget_show (view);
    ctk_widget_show (window);

    ctk_main ();
  }
#endif

  /* Cleanup */
  signal_monitor_free (monitor);
  g_object_unref (view);
  g_object_unref (store);
  g_object_unref (filter);
}

static void
specific_bug_657353_related (void)
{
  CtkTreeIter node1, node2, node3, node4;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkTreeModel *filter_model;
  CtkWidget *tree_view;
  GType column_types[] = { G_TYPE_BOOLEAN };

  /* ctk_tree_model_filter_rows_reordered() used to have a problem to
   * not properly transfer the first ref count when the first node in
   * the level does not have elt->offset == 0.  This test checks for
   * that.  This bug could cause the faulty condition
   *   elt->ext_ref_count > elt->ref_count
   * to raise.
   */

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  ctk_tree_store_set_column_types (CTK_TREE_STORE (model), 1,
                                   column_types);

  ctk_tree_store_append (CTK_TREE_STORE (model), &node1, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &node2, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &node3, NULL);
  ctk_tree_store_append (CTK_TREE_STORE (model), &node4, NULL);

  /* Hide the first node */
  ctk_tree_store_set (CTK_TREE_STORE (model), &node1, 0, FALSE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &node2, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &node3, 0, TRUE, -1);
  ctk_tree_store_set (CTK_TREE_STORE (model), &node4, 0, TRUE, -1);

  filter_model = ctk_tree_model_filter_new (model, NULL);
  ctk_tree_model_filter_set_visible_column (CTK_TREE_MODEL_FILTER (filter_model), 0);
  tree_view = ctk_tree_view_new_with_model (filter_model);

  assert_node_ref_count (ref_model, &node1, 0);
  assert_node_ref_count (ref_model, &node2, 2);
  assert_node_ref_count (ref_model, &node3, 1);
  assert_node_ref_count (ref_model, &node4, 1);

  /* Swap nodes 2 and 3 */

  /* ctk_tree_store_swap() will emit rows-reordered */
  ctk_tree_store_swap (CTK_TREE_STORE (model),
                       &node2, &node3);

  assert_node_ref_count (ref_model, &node1, 0);
  assert_node_ref_count (ref_model, &node3, 2);
  assert_node_ref_count (ref_model, &node2, 1);
  assert_node_ref_count (ref_model, &node4, 1);

  /* Hide node 3 */
  ctk_tree_store_set (CTK_TREE_STORE (model), &node3, 0, FALSE, -1);

  assert_node_ref_count (ref_model, &node1, 0);
  assert_node_ref_count (ref_model, &node3, 0);
  assert_node_ref_count (ref_model, &node2, 2);
  assert_node_ref_count (ref_model, &node4, 1);

  ctk_widget_destroy (tree_view);
  g_object_unref (filter_model);
  g_object_unref (ref_model);
}

static gboolean
specific_bug_657353_visible_func (CtkTreeModel *model,
                                  CtkTreeIter  *iter,
                                  gpointer      data)
{
  gchar *str;
  gboolean ret = FALSE;

  ctk_tree_model_get (model, iter, 0, &str, -1);
  ret = strstr (str, "hidden") ? FALSE : TRUE;
  g_free (str);

  return ret;
}

static void
specific_bug_657353 (void)
{
  CtkListStore *store;
  CtkTreeModel *sort_model;
  CtkTreeModel *filter_model;
  CtkTreeIter iter, iter_a, iter_b, iter_c;
  CtkWidget *tree_view;

  /* This is a very carefully crafted test case that is triggering the
   * situation described in bug 657353.
   *
   *   CtkListStore acts like EphyCompletionModel
   *   CtkTreeModelSort acts like the sort model added in
   *                      ephy_location_entry_set_completion.
   *   CtkTreeModelFilter acts like the filter model in
   *                      CtkEntryCompletion.
   */

  /* Set up a model that's wrapped in a CtkTreeModelSort.  The first item
   * will be hidden.
   */
  store = ctk_list_store_new (1, G_TYPE_STRING);
  ctk_list_store_insert_with_values (store, &iter_b, 0, 0, "BBB hidden", -1);
  ctk_list_store_insert_with_values (store, &iter, 1, 0, "EEE", -1);
  ctk_list_store_insert_with_values (store, &iter, 2, 0, "DDD", -1);
  ctk_list_store_insert_with_values (store, &iter_c, 3, 0, "CCC", -1);

  sort_model = ctk_tree_model_sort_new_with_model (CTK_TREE_MODEL (store));

  filter_model = ctk_tree_model_filter_new (sort_model, NULL);
  ctk_tree_model_filter_set_visible_func (CTK_TREE_MODEL_FILTER (filter_model),
                                          specific_bug_657353_visible_func,
                                          filter_model, NULL);

  tree_view = ctk_tree_view_new_with_model (filter_model);

  /* This triggers emission of rows-reordered.  The elt with offset == 0
   * is hidden, which used to cause misbehavior.  (The first reference should
   * have moved to CCC, which did not happen).
   */
  ctk_tree_sortable_set_sort_column_id (CTK_TREE_SORTABLE (sort_model),
                                        0, CTK_SORT_ASCENDING);

  /* By inserting another item that will appear at the first position, a
   * reference transfer is done from CCC (which failed to get this reference
   * earlier) to AAA.  At this point, the rule
   * elt->ref_count >= elt->ext_ref_count is broken for CCC.
   */
  ctk_list_store_insert_with_values (store, &iter_a, 6, 0, "AAA", -1);

  /* When we hide CCC, the references cannot be correctly released, because
   * CCC failed to get a reference during rows-reordered.  The faulty condition
   * only manifests itself here with MODEL_FILTER_DEBUG disabled (as is usual
   * in production).
   */
  ctk_list_store_set (store, &iter_c, 0, "CCC hidden", -1);

  ctk_widget_destroy (tree_view);
  g_object_unref (filter_model);
  g_object_unref (sort_model);
  g_object_unref (store);
}

static void
specific_bug_658696 (void)
{
  CtkTreeStore *store;
  CtkTreeModel *filter;
  CtkTreePath *vroot;
  CtkTreeIter iter;

  store = create_tree_store (4, TRUE);

  vroot = ctk_tree_path_new_from_indices (0, 0, -1);
  filter = ctk_tree_model_filter_new (CTK_TREE_MODEL (store), vroot);
  ctk_tree_path_free (vroot);

  /* This used to cause a crash in ctk_tree_model_filter_check_ancestors() */
  ctk_tree_store_append (store, &iter, NULL);

  g_object_unref (store);
  g_object_unref (filter);
}

static gboolean
specific_bug_659022_visible_func (CtkTreeModel *model,
                                  CtkTreeIter  *iter,
                                  gpointer      data)
{
  CtkTreeIter tmp;

  if (!ctk_tree_model_iter_parent (model, &tmp, iter))
    {
      if (ctk_tree_model_iter_n_children (model, iter) >= 2)
        return TRUE;
      else
        return FALSE;
    }

  return TRUE;
}

static void
specific_bug_659022_row_changed_emission (void)
{
  CtkTreeModel *filter;
  CtkTreeModel *model;
  CtkTreeIter parent, child, child2;
  CtkTreePath *path;
  CtkWidget *tree_view;

  model = ctk_tree_model_ref_count_new ();

  filter = ctk_tree_model_filter_new (model, NULL);
  ctk_tree_model_filter_set_visible_func (CTK_TREE_MODEL_FILTER (filter),
                                          specific_bug_659022_visible_func,
                                          NULL, NULL);

  tree_view = ctk_tree_view_new_with_model (CTK_TREE_MODEL (filter));

  ctk_tree_store_insert (CTK_TREE_STORE (model), &parent, NULL, 0);
  ctk_tree_store_insert (CTK_TREE_STORE (model), &child, &parent, 0);
  ctk_tree_store_insert (CTK_TREE_STORE (model), &child2, &parent, 0);

  ctk_tree_view_expand_all (CTK_TREE_VIEW (tree_view));

  ctk_tree_model_filter_refilter (CTK_TREE_MODEL_FILTER (filter));

  ctk_tree_store_remove (CTK_TREE_STORE (model), &child2);

  ctk_tree_model_filter_refilter (CTK_TREE_MODEL_FILTER (filter));

  path = ctk_tree_model_get_path (model, &child);
  ctk_tree_model_row_changed (model, path, &child);
  ctk_tree_path_free (path);

  ctk_widget_destroy (tree_view);
  g_object_unref (filter);
  g_object_unref (model);
}

static void
specific_bug_659022_row_deleted_node_invisible (void)
{
  CtkTreeModel *filter;
  CtkTreeModel *model;
  CtkTreeIter parent, child;
  CtkTreeIter parent2, child2, child3;
  CtkWidget *tree_view;

  model = ctk_tree_model_ref_count_new ();

  filter = ctk_tree_model_filter_new (model, NULL);
  ctk_tree_model_filter_set_visible_func (CTK_TREE_MODEL_FILTER (filter),
                                          specific_bug_659022_visible_func,
                                          NULL, NULL);

  tree_view = ctk_tree_view_new_with_model (CTK_TREE_MODEL (filter));

  ctk_tree_store_insert (CTK_TREE_STORE (model), &parent, NULL, 0);
  ctk_tree_store_insert (CTK_TREE_STORE (model), &child, &parent, 0);

  ctk_tree_store_insert (CTK_TREE_STORE (model), &parent2, NULL, 0);
  ctk_tree_store_insert (CTK_TREE_STORE (model), &child2, &parent2, 0);
  ctk_tree_store_insert (CTK_TREE_STORE (model), &child3, &parent2, 0);

  ctk_tree_view_expand_all (CTK_TREE_VIEW (tree_view));

  ctk_tree_model_filter_refilter (CTK_TREE_MODEL_FILTER (filter));

  ctk_tree_store_remove (CTK_TREE_STORE (model), &parent);

  ctk_widget_destroy (tree_view);
  g_object_unref (filter);
  g_object_unref (model);
}

static void
specific_bug_659022_row_deleted_free_level (void)
{
  CtkTreeModel *filter;
  CtkTreeModel *model;
  CtkTreeModelRefCount *ref_model;
  CtkTreeIter parent, child;
  CtkTreeIter parent2, child2, child3;
  CtkWidget *tree_view;

  model = ctk_tree_model_ref_count_new ();
  ref_model = CTK_TREE_MODEL_REF_COUNT (model);

  filter = ctk_tree_model_filter_new (model, NULL);
  ctk_tree_model_filter_set_visible_func (CTK_TREE_MODEL_FILTER (filter),
                                          specific_bug_659022_visible_func,
                                          NULL, NULL);

  tree_view = ctk_tree_view_new_with_model (CTK_TREE_MODEL (filter));

  /* Carefully construct a model */
  ctk_tree_store_insert (CTK_TREE_STORE (model), &parent, NULL, 0);
  ctk_tree_store_insert (CTK_TREE_STORE (model), &child, &parent, 0);

  ctk_tree_store_insert (CTK_TREE_STORE (model), &parent2, NULL, 0);
  ctk_tree_store_insert (CTK_TREE_STORE (model), &child2, &parent2, 0);
  ctk_tree_store_insert (CTK_TREE_STORE (model), &child3, &parent2, 0);

  /* Only parent2 is visible, child3 holds first ref count for that level
   * (Note that above, both child2 as child3 are inserted at position 0).
   */
  assert_node_ref_count (ref_model, &parent, 0);
  assert_node_ref_count (ref_model, &child, 0);
  assert_node_ref_count (ref_model, &parent2, 3);
  assert_node_ref_count (ref_model, &child3, 1);
  assert_node_ref_count (ref_model, &child2, 0);

  /* Make sure child level is cached */
  ctk_tree_view_expand_all (CTK_TREE_VIEW (tree_view));

  assert_node_ref_count (ref_model, &parent, 0);
  assert_node_ref_count (ref_model, &child, 0);
  assert_node_ref_count (ref_model, &parent2, 3);
  assert_node_ref_count (ref_model, &child3, 2);
  assert_node_ref_count (ref_model, &child2, 1);

  ctk_tree_view_collapse_all (CTK_TREE_VIEW (tree_view));

  assert_node_ref_count (ref_model, &parent, 0);
  assert_node_ref_count (ref_model, &child, 0);
  assert_node_ref_count (ref_model, &parent2, 3);
  assert_node_ref_count (ref_model, &child3, 1);
  assert_node_ref_count (ref_model, &child2, 0);

  /* Remove node with longer child level first */
  ctk_tree_store_remove (CTK_TREE_STORE (model), &parent2);
  ctk_tree_store_remove (CTK_TREE_STORE (model), &parent);

  ctk_widget_destroy (tree_view);
  g_object_unref (filter);
  g_object_unref (model);
}

static void
specific_bug_679910 (void)
{
  CtkTreeModel *filter;
  CtkListStore *store;
  CtkTreeIter iter, nil_iter;
  CtkTreeIter filter_iter;

  store = ctk_list_store_new (1, G_TYPE_POINTER);
  filter = ctk_tree_model_filter_new (CTK_TREE_MODEL (store), NULL);

  ctk_list_store_append (store, &nil_iter);
  ctk_list_store_append (store, &iter);
  ctk_list_store_append (store, &nil_iter);

  ctk_tree_model_filter_convert_child_iter_to_iter (CTK_TREE_MODEL_FILTER (filter),
                                                    &filter_iter,
                                                    &iter);
  iter = filter_iter;
  g_return_if_fail (ctk_tree_model_iter_next (filter, &iter));
  iter = filter_iter;
  g_return_if_fail (ctk_tree_model_iter_previous (filter, &iter));

  g_object_unref (filter);
  g_object_unref (store);
}

static int row_changed_count;
static int filter_row_changed_count;

static void
row_changed (CtkTreeModel *model,
             CtkTreePath  *path,
             CtkTreeIter  *iter,
             gpointer data)
{
  int *count = data;

  (*count)++;
}

static void
test_row_changed (void)
{
  CtkTreeModel *filter;
  CtkListStore *store;
  CtkTreeIter iter1, iter2, iter3;
  CtkTreeIter fiter1, fiter2, fiter3;

  store = ctk_list_store_new (1, G_TYPE_INT);
  filter = ctk_tree_model_filter_new (CTK_TREE_MODEL (store), NULL);

  ctk_list_store_append (store, &iter1);
  ctk_list_store_append (store, &iter2);
  ctk_list_store_append (store, &iter3);

  ctk_tree_model_filter_convert_child_iter_to_iter (CTK_TREE_MODEL_FILTER (filter), &fiter1, &iter1);
  ctk_tree_model_filter_convert_child_iter_to_iter (CTK_TREE_MODEL_FILTER (filter), &fiter2, &iter2);
  ctk_tree_model_filter_convert_child_iter_to_iter (CTK_TREE_MODEL_FILTER (filter), &fiter3, &iter3);

  g_signal_connect (store, "row-changed", G_CALLBACK (row_changed), &row_changed_count);
  g_signal_connect (filter, "row-changed", G_CALLBACK (row_changed), &filter_row_changed_count);

  row_changed_count = 0;
  filter_row_changed_count = 0;

  ctk_list_store_set (store, &iter1, 0, 1, -1);
  ctk_list_store_set (store, &iter2, 0, 1, -1);
  ctk_list_store_set (store, &iter3, 0, 1, -1);

  g_assert (row_changed_count == 3);
  g_assert (filter_row_changed_count == 0);

  row_changed_count = 0;
  filter_row_changed_count = 0;

  ctk_tree_model_ref_node (filter, &fiter1);
  ctk_tree_model_ref_node (filter, &fiter2);
  ctk_tree_model_ref_node (filter, &fiter3);

  ctk_list_store_set (store, &iter1, 0, 2, -1);
  ctk_list_store_set (store, &iter2, 0, 2, -1);
  ctk_list_store_set (store, &iter3, 0, 2, -1);

  g_assert (row_changed_count == 3);
  g_assert (filter_row_changed_count == 3);

  ctk_tree_model_unref_node (filter, &fiter1);
  ctk_tree_model_unref_node (filter, &fiter2);
  ctk_tree_model_unref_node (filter, &fiter3);

  g_object_unref (filter);
  g_object_unref (store);
}


/* main */

void
register_filter_model_tests (void)
{
  g_test_add ("/TreeModelFilter/self/verify-test-suite",
              FilterTest, NULL,
              filter_test_setup,
              verify_test_suite,
              filter_test_teardown);

  g_test_add ("/TreeModelFilter/self/verify-test-suite/vroot/depth-1",
              FilterTest, ctk_tree_path_new_from_indices (2, -1),
              filter_test_setup,
              verify_test_suite_vroot,
              filter_test_teardown);
  g_test_add ("/TreeModelFilter/self/verify-test-suite/vroot/depth-2",
              FilterTest, ctk_tree_path_new_from_indices (2, 3, -1),
              filter_test_setup,
              verify_test_suite_vroot,
              filter_test_teardown);


  g_test_add ("/TreeModelFilter/filled/hide-root-level",
              FilterTest, NULL,
              filter_test_setup,
              filled_hide_root_level,
              filter_test_teardown);
  g_test_add ("/TreeModelFilter/filled/hide-child-levels",
              FilterTest, NULL,
              filter_test_setup,
              filled_hide_child_levels,
              filter_test_teardown);
  g_test_add ("/TreeModelFilter/filled/hide-child-levels/root-expanded",
              FilterTest, NULL,
              filter_test_setup,
              filled_hide_child_levels_root_expanded,
              filter_test_teardown);

  g_test_add ("/TreeModelFilter/filled/hide-root-level/vroot",
              FilterTest, ctk_tree_path_new_from_indices (2, -1),
              filter_test_setup,
              filled_vroot_hide_root_level,
              filter_test_teardown);
  g_test_add ("/TreeModelFilter/filled/hide-child-levels/vroot",
              FilterTest, ctk_tree_path_new_from_indices (2, -1),
              filter_test_setup,
              filled_vroot_hide_child_levels,
              filter_test_teardown);
  g_test_add ("/TreeModelFilter/filled/hide-child-levels/vroot-root-expanded",
              FilterTest, ctk_tree_path_new_from_indices (2, -1),
              filter_test_setup,
              filled_vroot_hide_child_levels_root_expanded,
              filter_test_teardown);


  g_test_add ("/TreeModelFilter/empty/show-nodes",
              FilterTest, NULL,
              filter_test_setup_empty,
              empty_show_nodes,
              filter_test_teardown);
  g_test_add ("/TreeModelFilter/empty/show-multiple-nodes",
              FilterTest, NULL,
              filter_test_setup_empty,
              empty_show_multiple_nodes,
              filter_test_teardown);

  g_test_add ("/TreeModelFilter/empty/show-nodes/vroot",
              FilterTest, ctk_tree_path_new_from_indices (2, -1),
              filter_test_setup_empty,
              empty_vroot_show_nodes,
              filter_test_teardown);
  g_test_add ("/TreeModelFilter/empty/show-multiple-nodes/vroot",
              FilterTest, ctk_tree_path_new_from_indices (2, -1),
              filter_test_setup_empty,
              empty_vroot_show_multiple_nodes,
              filter_test_teardown);


  g_test_add ("/TreeModelFilter/unfiltered/hide-single",
              FilterTest, NULL,
              filter_test_setup_unfiltered,
              unfiltered_hide_single,
              filter_test_teardown);
  g_test_add ("/TreeModelFilter/unfiltered/hide-single/root-expanded",
              FilterTest, NULL,
              filter_test_setup_unfiltered_root_expanded,
              unfiltered_hide_single_root_expanded,
              filter_test_teardown);
  g_test_add ("/TreeModelFilter/unfiltered/hide-single-child",
              FilterTest, NULL,
              filter_test_setup_unfiltered,
              unfiltered_hide_single_child,
              filter_test_teardown);
  g_test_add ("/TreeModelFilter/unfiltered/hide-single-child/root-expanded",
              FilterTest, NULL,
              filter_test_setup_unfiltered_root_expanded,
              unfiltered_hide_single_child_root_expanded,
              filter_test_teardown);
  g_test_add ("/TreeModelFilter/unfiltered/hide-single-multi-level",
              FilterTest, NULL,
              filter_test_setup_unfiltered,
              unfiltered_hide_single_multi_level,
              filter_test_teardown);
  g_test_add ("/TreeModelFilter/unfiltered/hide-single-multi-level/root-expanded",
              FilterTest, NULL,
              filter_test_setup_unfiltered_root_expanded,
              unfiltered_hide_single_multi_level_root_expanded,
              filter_test_teardown);

  g_test_add ("/TreeModelFilter/unfiltered/hide-single/vroot",
              FilterTest, ctk_tree_path_new_from_indices (2, -1),
              filter_test_setup_unfiltered,
              unfiltered_vroot_hide_single,
              filter_test_teardown);
  g_test_add ("/TreeModelFilter/unfiltered/hide-single-child/vroot",
              FilterTest, ctk_tree_path_new_from_indices (2, -1),
              filter_test_setup_unfiltered,
              unfiltered_vroot_hide_single_child,
              filter_test_teardown);
  g_test_add ("/TreeModelFilter/unfiltered/hide-single-child/vroot/root-expanded",
              FilterTest, ctk_tree_path_new_from_indices (2, -1),
              filter_test_setup_unfiltered_root_expanded,
              unfiltered_vroot_hide_single_child_root_expanded,
              filter_test_teardown);
  g_test_add ("/TreeModelFilter/unfiltered/hide-single-multi-level/vroot",
              FilterTest, ctk_tree_path_new_from_indices (2, -1),
              filter_test_setup_unfiltered,
              unfiltered_vroot_hide_single_multi_level,
              filter_test_teardown);
  g_test_add ("/TreeModelFilter/unfiltered/hide-single-multi-level/vroot/root-expanded",
              FilterTest, ctk_tree_path_new_from_indices (2, -1),
              filter_test_setup_unfiltered_root_expanded,
              unfiltered_vroot_hide_single_multi_level_root_expanded,
              filter_test_teardown);



  g_test_add ("/TreeModelFilter/unfiltered/show-single",
              FilterTest, NULL,
              filter_test_setup_empty_unfiltered,
              unfiltered_show_single,
              filter_test_teardown);
  g_test_add ("/TreeModelFilter/unfiltered/show-single-child",
              FilterTest, NULL,
              filter_test_setup_empty_unfiltered,
              unfiltered_show_single_child,
              filter_test_teardown);
  g_test_add ("/TreeModelFilter/unfiltered/show-single-child/root-expanded",
              FilterTest, NULL,
              filter_test_setup_empty_unfiltered_root_expanded,
              unfiltered_show_single_child_root_expanded,
              filter_test_teardown);
  g_test_add ("/TreeModelFilter/unfiltered/show-single-multi-level",
              FilterTest, NULL,
              filter_test_setup_empty_unfiltered,
              unfiltered_show_single_multi_level,
              filter_test_teardown);
  g_test_add ("/TreeModelFilter/unfiltered/show-single-multi-level/root-expanded",
              FilterTest, NULL,
              filter_test_setup_empty_unfiltered_root_expanded,
              unfiltered_show_single_multi_level_root_expanded,
              filter_test_teardown);

  g_test_add ("/TreeModelFilter/unfiltered/show-single/vroot",
              FilterTest, ctk_tree_path_new_from_indices (2, -1),
              filter_test_setup_empty_unfiltered,
              unfiltered_vroot_show_single,
              filter_test_teardown);
  g_test_add ("/TreeModelFilter/unfiltered/show-single-child/vroot",
              FilterTest, ctk_tree_path_new_from_indices (2, -1),
              filter_test_setup_empty_unfiltered,
              unfiltered_vroot_show_single_child,
              filter_test_teardown);
  g_test_add ("/TreeModelFilter/unfiltered/show-single-child/vroot/root-expanded",
              FilterTest, ctk_tree_path_new_from_indices (2, -1),
              filter_test_setup_empty_unfiltered_root_expanded,
              unfiltered_vroot_show_single_child_root_expanded,
              filter_test_teardown);
  g_test_add ("/TreeModelFilter/unfiltered/show-single-multi-level/vroot",
              FilterTest, ctk_tree_path_new_from_indices (2, -1),
              filter_test_setup_empty_unfiltered,
              unfiltered_vroot_show_single_multi_level,
              filter_test_teardown);
  g_test_add ("/TreeModelFilter/unfiltered/show-single-multi-level/vroot/root-expanded",
              FilterTest, ctk_tree_path_new_from_indices (2, -1),
              filter_test_setup_empty_unfiltered_root_expanded,
              unfiltered_vroot_show_single_multi_level_root_expanded,
              filter_test_teardown);


  g_test_add ("/TreeModelFilter/unfiltered/rows-reordered/root-level",
              FilterTest, NULL,
              filter_test_setup_unfiltered,
              unfiltered_rows_reordered_root_level,
              filter_test_teardown);
  g_test_add ("/TreeModelFilter/unfiltered/rows-reordered/child-level",
              FilterTest, NULL,
              filter_test_setup_unfiltered,
              unfiltered_rows_reordered_child_level,
              filter_test_teardown);

  g_test_add ("/TreeModelFilter/filtered/rows-reordered/root-level/first-hidden",
              FilterTest, NULL,
              filter_test_setup,
              filtered_rows_reordered_root_level_first_hidden,
              filter_test_teardown);
  g_test_add ("/TreeModelFilter/filtered/rows-reordered/root-level/middle-hidden",
              FilterTest, NULL,
              filter_test_setup,
              filtered_rows_reordered_root_level_middle_hidden,
              filter_test_teardown);
  g_test_add ("/TreeModelFilter/filtered/rows-reordered/child-level/first-hidden",
              FilterTest, NULL,
              filter_test_setup,
              filtered_rows_reordered_child_level_first_hidden,
              filter_test_teardown);
  g_test_add ("/TreeModelFilter/filtered/rows-reordered/child-level/middle-hidden",
              FilterTest, NULL,
              filter_test_setup,
              filtered_rows_reordered_child_level_middle_hidden,
              filter_test_teardown);
  g_test_add ("/TreeModelFilter/filtered/rows-reordered/child-level/4-hidden",
              FilterTest, NULL,
              filter_test_setup,
              filtered_rows_reordered_child_level_4_hidden,
              filter_test_teardown);
  g_test_add ("/TreeModelFilter/filtered/rows-reordered/child-level/all-hidden",
              FilterTest, NULL,
              filter_test_setup,
              filtered_rows_reordered_child_level_all_hidden,
              filter_test_teardown);

  /* Inserts in child models after creation of filter model */
  g_test_add_func ("/TreeModelFilter/insert/before",
                   insert_before);
  g_test_add_func ("/TreeModelFilter/insert/child",
                   insert_child);

  /* Removals from child model after creating of filter model */
  g_test_add_func ("/TreeModelFilter/remove/node",
                   remove_node);
  g_test_add_func ("/TreeModelFilter/remove/node-vroot",
                   remove_node_vroot);
  g_test_add_func ("/TreeModelFilter/remove/vroot-ancestor",
                   remove_vroot_ancestor);

  /* Reference counting */
  g_test_add_func ("/TreeModelFilter/ref-count/single-level",
                   ref_count_single_level);
  g_test_add_func ("/TreeModelFilter/ref-count/two-levels",
                   ref_count_two_levels);
  g_test_add_func ("/TreeModelFilter/ref-count/three-levels",
                   ref_count_three_levels);
  g_test_add_func ("/TreeModelFilter/ref-count/delete-row",
                   ref_count_delete_row);
  g_test_add_func ("/TreeModelFilter/ref-count/filter-row/length-1",
                   ref_count_filter_row_length_1);
  g_test_add_func ("/TreeModelFilter/ref-count/filter-row/length-1-remove-in-root-level",
                   ref_count_filter_row_length_1_remove_in_root_level);
  g_test_add_func ("/TreeModelFilter/ref-count/filter-row/length-1-remove-in-child-level",
                   ref_count_filter_row_length_1_remove_in_child_level);
  g_test_add_func ("/TreeModelFilter/ref-count/filter-row/length-gt-1",
                   ref_count_filter_row_length_gt_1);
  g_test_add_func ("/TreeModelFilter/ref-count/filter-row/length-gt-1-visible-children",
                   ref_count_filter_row_length_gt_1_visible_children);
  g_test_add_func ("/TreeModelFilter/ref-count/cleanup",
                   ref_count_cleanup);
  g_test_add_func ("/TreeModelFilter/ref-count/row-ref",
                   ref_count_row_ref);

  /* Reference counting, transfer of first reference on
   * first node in level.  This is a CtkTreeModelFilter-specific
   * feature.
   */
  g_test_add_func ("/TreeModelFilter/ref-count/transfer/root-level/insert",
                   ref_count_transfer_root_level_insert);
  g_test_add_func ("/TreeModelFilter/ref-count/transfer/root-level/remove",
                   ref_count_transfer_root_level_remove);
  g_test_add_func ("/TreeModelFilter/ref-count/transfer/root-level/remove/filtered",
                   ref_count_transfer_root_level_remove_filtered);
  g_test_add_func ("/TreeModelFilter/ref-count/transfer/root-level/reordered",
                   ref_count_transfer_root_level_reordered);
  g_test_add_func ("/TreeModelFilter/ref-count/transfer/root-level/reordered/filtered",
                   ref_count_transfer_root_level_reordered_filtered);
  g_test_add_func ("/TreeModelFilter/ref-count/transfer/root-level/filter",
                   ref_count_transfer_root_level_filter);
  g_test_add_func ("/TreeModelFilter/ref-count/transfer/child-level/insert",
                   ref_count_transfer_child_level_insert);
  g_test_add_func ("/TreeModelFilter/ref-count/transfer/child-level/remove",
                   ref_count_transfer_child_level_remove);
  g_test_add_func ("/TreeModelFilter/ref-count/transfer/child-level/remove/filtered",
                   ref_count_transfer_child_level_remove_filtered);
  g_test_add_func ("/TreeModelFilter/ref-count/transfer/child-level/reordered",
                   ref_count_transfer_child_level_reordered);
  g_test_add_func ("/TreeModelFilter/ref-count/transfer/child-level/reordered/filtered",
                   ref_count_transfer_child_level_reordered_filtered);
  g_test_add_func ("/TreeModelFilter/ref-count/transfer/child-level/filter",
                   ref_count_transfer_child_level_filter);

  g_test_add_func ("/TreeModelFilter/specific/path-dependent-filter",
                   specific_path_dependent_filter);
  g_test_add_func ("/TreeModelFilter/specific/append-after-collapse",
                   specific_append_after_collapse);
  g_test_add_func ("/TreeModelFilter/specific/sort-filter-remove-node",
                   specific_sort_filter_remove_node);
  g_test_add_func ("/TreeModelFilter/specific/sort-filter-remove-root",
                   specific_sort_filter_remove_root);
  g_test_add_func ("/TreeModelFilter/specific/root-mixed-visibility",
                   specific_root_mixed_visibility);
  g_test_add_func ("/TreeModelFilter/specific/has-child-filter",
                   specific_has_child_filter);
  g_test_add_func ("/TreeModelFilter/specific/has-child-filter-on-sort-model",
                   specific_has_child_filter_on_sort_model);
  g_test_add_func ("/TreeModelFilter/specific/at-least-2-children-filter",
                   specific_at_least_2_children_filter);
  g_test_add_func ("/TreeModelFilter/specific/at-least-2-children-filter-on-sort-model",
                   specific_at_least_2_children_filter_on_sort_model);
  g_test_add_func ("/TreeModelFilter/specific/root-has-child-filter",
                   specific_root_has_child_filter);
  g_test_add_func ("/TreeModelFilter/specific/filter-add-child",
                   specific_filter_add_child);
  g_test_add_func ("/TreeModelFilter/specific/list-store-clear",
                   specific_list_store_clear);
  g_test_add_func ("/TreeModelFilter/specific/sort-ref-leaf-and-remove-ancestor",
                   specific_sort_ref_leaf_and_remove_ancestor);
  g_test_add_func ("/TreeModelFilter/specific/ref-leaf-and-remove-ancestor",
                   specific_ref_leaf_and_remove_ancestor);
  g_test_add_func ("/TreeModelFilter/specific/virtual-ref-leaf-and-remove-ancestor",
                   specific_virtual_ref_leaf_and_remove_ancestor);

  g_test_add_func ("/TreeModelFilter/specific/bug-301558",
                   specific_bug_301558);
  g_test_add_func ("/TreeModelFilter/specific/bug-311955",
                   specific_bug_311955);
  g_test_add_func ("/TreeModelFilter/specific/bug-311955-clean",
                   specific_bug_311955_clean);
  g_test_add_func ("/TreeModelFilter/specific/bug-346800",
                   specific_bug_346800);
  g_test_add_func ("/TreeModelFilter/specific/bug-464173",
                   specific_bug_464173);
  g_test_add_func ("/TreeModelFilter/specific/bug-540201",
                   specific_bug_540201);
  g_test_add_func ("/TreeModelFilter/specific/bug-549287",
                   specific_bug_549287);
  g_test_add_func ("/TreeModelFilter/specific/bug-621076",
                   specific_bug_621076);
  g_test_add_func ("/TreeModelFilter/specific/bug-657353-related",
                   specific_bug_657353_related);
  g_test_add_func ("/TreeModelFilter/specific/bug-657353",
                   specific_bug_657353);
  g_test_add_func ("/TreeModelFilter/specific/bug-658696",
                   specific_bug_658696);
  g_test_add_func ("/TreeModelFilter/specific/bug-659022/row-changed-emission",
                   specific_bug_659022_row_changed_emission);
  g_test_add_func ("/TreeModelFilter/specific/bug-659022/row-deleted-node-invisible",
                   specific_bug_659022_row_deleted_node_invisible);
  g_test_add_func ("/TreeModelFilter/specific/bug-659022/row-deleted-free-level",
                   specific_bug_659022_row_deleted_free_level);
  g_test_add_func ("/TreeModelFilter/specific/bug-679910",
                   specific_bug_679910);

  g_test_add_func ("/TreeModelFilter/signal/row-changed", test_row_changed);
}
