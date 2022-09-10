/* Extensive CtkTreeStore tests.
 * Copyright (C) 2007  Imendio AB
 * Authors: Kristian Rietveld  <kris@imendio.com>
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

/* To do:
 *  - All the to do items from liststore.c, plus:
 *  - Finish up the insertion tests; things aren't as nicely refactored
 *    here as in CtkListStore, so we need to check for corner cases on
 *    all insertion functions separately.
 *  - We only test in the root level, we also need all tests "duplicated"
 *    for child levels.
 *  - And we also need tests for creating these child levels, etc.
 */

#include "treemodel.h"

#include <ctk/ctk.h>

static inline gboolean
iters_equal (CtkTreeIter *a,
	     CtkTreeIter *b)
{
  if (a->stamp != b->stamp)
    return FALSE;

  if (a->user_data != b->user_data)
    return FALSE;

  /* user_data2 and user_data3 are not used in CtkTreeStore */

  return TRUE;
}

static gboolean
iter_position (CtkTreeStore *store,
	       CtkTreeIter  *iter,
	       int           n)
{
  gboolean ret = TRUE;
  CtkTreePath *path;

  path = ctk_tree_model_get_path (CTK_TREE_MODEL (store), iter);
  if (!path)
    return FALSE;

  if (ctk_tree_path_get_indices (path)[0] != n)
    ret = FALSE;

  ctk_tree_path_free (path);

  return ret;
}

/*
 * Fixture
 */
typedef struct
{
  CtkTreeIter iter[5];
  CtkTreeStore *store;
} TreeStore;

static void
tree_store_setup (TreeStore     *fixture,
		  gconstpointer  test_data)
{
  int i;

  fixture->store = ctk_tree_store_new (1, G_TYPE_INT);

  for (i = 0; i < 5; i++)
    {
      ctk_tree_store_insert (fixture->store, &fixture->iter[i], NULL, i);
      ctk_tree_store_set (fixture->store, &fixture->iter[i], 0, i, -1);
    }
}

static void
tree_store_teardown (TreeStore     *fixture,
		     gconstpointer  test_data)
{
  g_object_unref (fixture->store);
}

/*
 * The actual tests.
 */

static void
check_model (TreeStore *fixture,
	     gint      *new_order,
	     gint       skip)
{
  int i;
  CtkTreePath *path;

  path = ctk_tree_path_new ();
  ctk_tree_path_down (path);

  /* Check validity of the model and validity of the iters-persistent
   * claim.
   */
  for (i = 0; i < 5; i++)
    {
      CtkTreeIter iter;

      if (i == skip)
	continue;

      /* The saved iterator at new_order[i] should match the iterator
       * at i.
       */

      ctk_tree_model_get_iter (CTK_TREE_MODEL (fixture->store),
			       &iter, path);

      g_assert (ctk_tree_store_iter_is_valid (fixture->store, &iter));
      g_assert (iters_equal (&iter, &fixture->iter[new_order[i]]));

      ctk_tree_path_next (path);
    }

  ctk_tree_path_free (path);
}

/* insertion */
static void
tree_store_test_insert_high_values (void)
{
  CtkTreeIter iter, iter2;
  CtkTreeIter iter_copy;
  CtkTreeStore *store;

  store = ctk_tree_store_new (1, G_TYPE_INT);

  ctk_tree_store_insert (store, &iter, NULL, 1234);
  g_assert (ctk_tree_store_iter_is_valid (store, &iter));
  g_assert (ctk_tree_model_iter_n_children (CTK_TREE_MODEL (store), NULL) == 1);
  g_assert (ctk_tree_model_get_iter_first (CTK_TREE_MODEL (store), &iter_copy));
  g_assert (iters_equal (&iter, &iter_copy));
  g_assert (iter_position (store, &iter, 0));

  ctk_tree_store_insert (store, &iter2, NULL, 765);
  g_assert (ctk_tree_store_iter_is_valid (store, &iter2));
  g_assert (ctk_tree_model_iter_n_children (CTK_TREE_MODEL (store), NULL) == 2);

  /* Walk over the model */
  g_assert (ctk_tree_model_get_iter_first (CTK_TREE_MODEL (store), &iter_copy));
  g_assert (iters_equal (&iter, &iter_copy));
  g_assert (iter_position (store, &iter, 0));

  g_assert (ctk_tree_model_iter_next (CTK_TREE_MODEL (store), &iter_copy));
  g_assert (iters_equal (&iter2, &iter_copy));
  g_assert (iter_position (store, &iter2, 1));

  g_assert (!ctk_tree_model_iter_next (CTK_TREE_MODEL (store), &iter_copy));

  g_assert (ctk_tree_model_iter_nth_child (CTK_TREE_MODEL (store), &iter_copy, NULL, 1));
  g_assert (iters_equal (&iter2, &iter_copy));
  g_assert (iter_position (store, &iter2, 1));

  g_assert (ctk_tree_model_iter_previous (CTK_TREE_MODEL (store), &iter_copy));
  g_assert (iters_equal (&iter, &iter_copy));
  g_assert (iter_position (store, &iter, 0));

  g_assert (!ctk_tree_model_iter_previous (CTK_TREE_MODEL (store), &iter_copy));

  g_object_unref (store);
}

static void
tree_store_test_append (void)
{
  CtkTreeIter iter, iter2;
  CtkTreeIter iter_copy;
  CtkTreeStore *store;

  store = ctk_tree_store_new (1, G_TYPE_INT);

  ctk_tree_store_append (store, &iter, NULL);
  g_assert (ctk_tree_store_iter_is_valid (store, &iter));
  g_assert (ctk_tree_model_iter_n_children (CTK_TREE_MODEL (store), NULL) == 1);
  g_assert (ctk_tree_model_get_iter_first (CTK_TREE_MODEL (store), &iter_copy));
  g_assert (iters_equal (&iter, &iter_copy));
  g_assert (iter_position (store, &iter, 0));

  ctk_tree_store_append (store, &iter2, NULL);
  g_assert (ctk_tree_store_iter_is_valid (store, &iter2));
  g_assert (ctk_tree_model_iter_n_children (CTK_TREE_MODEL (store), NULL) == 2);

  /* Walk over the model */
  g_assert (ctk_tree_model_get_iter_first (CTK_TREE_MODEL (store), &iter_copy));
  g_assert (iters_equal (&iter, &iter_copy));
  g_assert (iter_position (store, &iter, 0));

  g_assert (ctk_tree_model_iter_next (CTK_TREE_MODEL (store), &iter_copy));
  g_assert (iters_equal (&iter2, &iter_copy));
  g_assert (iter_position (store, &iter2, 1));

  g_assert (!ctk_tree_model_iter_next (CTK_TREE_MODEL (store), &iter_copy));

  g_assert (ctk_tree_model_iter_nth_child (CTK_TREE_MODEL (store), &iter_copy, NULL, 1));
  g_assert (iters_equal (&iter2, &iter_copy));
  g_assert (iter_position (store, &iter2, 1));

  g_assert (ctk_tree_model_iter_previous (CTK_TREE_MODEL (store), &iter_copy));
  g_assert (iters_equal (&iter, &iter_copy));
  g_assert (iter_position (store, &iter, 0));

  g_assert (!ctk_tree_model_iter_previous (CTK_TREE_MODEL (store), &iter_copy));

  g_object_unref (store);
}

static void
tree_store_test_prepend (void)
{
  CtkTreeIter iter, iter2;
  CtkTreeIter iter_copy;
  CtkTreeStore *store;

  store = ctk_tree_store_new (1, G_TYPE_INT);

  ctk_tree_store_prepend (store, &iter, NULL);
  g_assert (ctk_tree_store_iter_is_valid (store, &iter));
  g_assert (ctk_tree_model_iter_n_children (CTK_TREE_MODEL (store), NULL) == 1);
  g_assert (ctk_tree_model_get_iter_first (CTK_TREE_MODEL (store), &iter_copy));
  g_assert (iters_equal (&iter, &iter_copy));
  g_assert (iter_position (store, &iter, 0));

  ctk_tree_store_prepend (store, &iter2, NULL);
  g_assert (ctk_tree_store_iter_is_valid (store, &iter2));
  g_assert (ctk_tree_model_iter_n_children (CTK_TREE_MODEL (store), NULL) == 2);

  /* Walk over the model */
  g_assert (ctk_tree_model_get_iter_first (CTK_TREE_MODEL (store), &iter_copy));
  g_assert (iters_equal (&iter2, &iter_copy));
  g_assert (iter_position (store, &iter2, 0));

  g_assert (ctk_tree_model_iter_next (CTK_TREE_MODEL (store), &iter_copy));
  g_assert (iters_equal (&iter, &iter_copy));
  g_assert (iter_position (store, &iter, 1));

  g_assert (!ctk_tree_model_iter_next (CTK_TREE_MODEL (store), &iter_copy));

  g_assert (ctk_tree_model_iter_nth_child (CTK_TREE_MODEL (store), &iter_copy, NULL, 1));
  g_assert (iters_equal (&iter, &iter_copy));
  g_assert (iter_position (store, &iter, 1));

  g_assert (ctk_tree_model_iter_previous (CTK_TREE_MODEL (store), &iter_copy));
  g_assert (iters_equal (&iter2, &iter_copy));
  g_assert (iter_position (store, &iter2, 0));

  g_assert (!ctk_tree_model_iter_previous (CTK_TREE_MODEL (store), &iter_copy));

  g_object_unref (store);
}

static void
tree_store_test_insert_after (void)
{
  CtkTreeIter iter, iter2, iter3;
  CtkTreeIter iter_copy;
  CtkTreeStore *store;

  store = ctk_tree_store_new (1, G_TYPE_INT);

  ctk_tree_store_append (store, &iter, NULL);
  ctk_tree_store_append (store, &iter2, NULL);

  ctk_tree_store_insert_after (store, &iter3, NULL, &iter);
  g_assert (ctk_tree_store_iter_is_valid (store, &iter3));
  g_assert (ctk_tree_model_iter_n_children (CTK_TREE_MODEL (store), NULL) == 3);
  g_assert (ctk_tree_model_iter_nth_child (CTK_TREE_MODEL (store), &iter_copy, NULL, 1));
  g_assert (iters_equal (&iter3, &iter_copy));
  g_assert (iter_position (store, &iter3, 1));

  /* Walk over the model */
  g_assert (ctk_tree_model_get_iter_first (CTK_TREE_MODEL (store), &iter_copy));
  g_assert (iters_equal (&iter, &iter_copy));
  g_assert (iter_position (store, &iter_copy, 0));

  g_assert (ctk_tree_model_iter_next (CTK_TREE_MODEL (store), &iter_copy));
  g_assert (iters_equal (&iter3, &iter_copy));
  g_assert (iter_position (store, &iter_copy, 1));

  g_assert (ctk_tree_model_iter_next (CTK_TREE_MODEL (store), &iter_copy));
  g_assert (iters_equal (&iter2, &iter_copy));
  g_assert (iter_position (store, &iter_copy, 2));

  g_assert (!ctk_tree_model_iter_next (CTK_TREE_MODEL (store), &iter_copy));

  g_assert (ctk_tree_model_iter_nth_child (CTK_TREE_MODEL (store), &iter_copy, NULL, 2));
  g_assert (iters_equal (&iter2, &iter_copy));
  g_assert (iter_position (store, &iter2, 2));

  g_assert (ctk_tree_model_iter_previous (CTK_TREE_MODEL (store), &iter_copy));
  g_assert (iters_equal (&iter3, &iter_copy));
  g_assert (iter_position (store, &iter3, 1));

  g_assert (ctk_tree_model_iter_previous (CTK_TREE_MODEL (store), &iter_copy));
  g_assert (iters_equal (&iter, &iter_copy));
  g_assert (iter_position (store, &iter, 0));

  g_assert (!ctk_tree_model_iter_previous (CTK_TREE_MODEL (store), &iter_copy));

  g_object_unref (store);
}

static void
tree_store_test_insert_after_NULL (void)
{
  CtkTreeIter iter, iter2;
  CtkTreeIter iter_copy;
  CtkTreeStore *store;

  store = ctk_tree_store_new (1, G_TYPE_INT);

  ctk_tree_store_append (store, &iter, NULL);

  /* move_after NULL is basically a prepend */
  ctk_tree_store_insert_after (store, &iter2, NULL, NULL);
  g_assert (ctk_tree_store_iter_is_valid (store, &iter2));
  g_assert (ctk_tree_model_iter_n_children (CTK_TREE_MODEL (store), NULL) == 2);

  /* Walk over the model */
  g_assert (ctk_tree_model_get_iter_first (CTK_TREE_MODEL (store), &iter_copy));
  g_assert (iters_equal (&iter2, &iter_copy));
  g_assert (iter_position (store, &iter2, 0));

  g_assert (ctk_tree_model_iter_next (CTK_TREE_MODEL (store), &iter_copy));
  g_assert (iters_equal (&iter, &iter_copy));
  g_assert (iter_position (store, &iter, 1));

  g_assert (!ctk_tree_model_iter_next (CTK_TREE_MODEL (store), &iter_copy));

  g_assert (ctk_tree_model_iter_nth_child (CTK_TREE_MODEL (store), &iter_copy, NULL, 0));
  g_assert (iters_equal (&iter2, &iter_copy));

  g_assert (ctk_tree_model_iter_nth_child (CTK_TREE_MODEL (store), &iter_copy, NULL, 1));
  g_assert (iters_equal (&iter, &iter_copy));
  g_assert (iter_position (store, &iter, 1));

  g_assert (ctk_tree_model_iter_previous (CTK_TREE_MODEL (store), &iter_copy));
  g_assert (iters_equal (&iter2, &iter_copy));
  g_assert (iter_position (store, &iter2, 0));

  g_assert (!ctk_tree_model_iter_previous (CTK_TREE_MODEL (store), &iter_copy));

  g_object_unref (store);
}

static void
tree_store_test_insert_before (void)
{
  CtkTreeIter iter, iter2, iter3;
  CtkTreeIter iter_copy;
  CtkTreeStore *store;

  store = ctk_tree_store_new (1, G_TYPE_INT);

  ctk_tree_store_append (store, &iter, NULL);
  ctk_tree_store_append (store, &iter2, NULL);

  ctk_tree_store_insert_before (store, &iter3, NULL, &iter2);
  g_assert (ctk_tree_store_iter_is_valid (store, &iter3));
  g_assert (ctk_tree_model_iter_n_children (CTK_TREE_MODEL (store), NULL) == 3);
  g_assert (ctk_tree_model_iter_nth_child (CTK_TREE_MODEL (store), &iter_copy, NULL, 1));
  g_assert (iters_equal (&iter3, &iter_copy));
  g_assert (iter_position (store, &iter3, 1));

  /* Walk over the model */
  g_assert (ctk_tree_model_get_iter_first (CTK_TREE_MODEL (store), &iter_copy));
  g_assert (iters_equal (&iter, &iter_copy));
  g_assert (iter_position (store, &iter_copy, 0));

  g_assert (ctk_tree_model_iter_next (CTK_TREE_MODEL (store), &iter_copy));
  g_assert (iters_equal (&iter3, &iter_copy));
  g_assert (iter_position (store, &iter_copy, 1));

  g_assert (ctk_tree_model_iter_next (CTK_TREE_MODEL (store), &iter_copy));
  g_assert (iters_equal (&iter2, &iter_copy));
  g_assert (iter_position (store, &iter_copy, 2));

  g_assert (!ctk_tree_model_iter_next (CTK_TREE_MODEL (store), &iter_copy));

  g_assert (ctk_tree_model_iter_nth_child (CTK_TREE_MODEL (store), &iter_copy, NULL, 1));
  g_assert (iters_equal (&iter3, &iter_copy));

  g_assert (ctk_tree_model_iter_nth_child (CTK_TREE_MODEL (store), &iter_copy, NULL, 2));
  g_assert (iters_equal (&iter2, &iter_copy));
  g_assert (iter_position (store, &iter2, 2));

  g_assert (ctk_tree_model_iter_previous (CTK_TREE_MODEL (store), &iter_copy));
  g_assert (iters_equal (&iter3, &iter_copy));
  g_assert (iter_position (store, &iter3, 1));

  g_assert (ctk_tree_model_iter_previous (CTK_TREE_MODEL (store), &iter_copy));
  g_assert (iters_equal (&iter, &iter_copy));
  g_assert (iter_position (store, &iter, 0));

  g_assert (!ctk_tree_model_iter_previous (CTK_TREE_MODEL (store), &iter_copy));

  g_object_unref (store);
}

static void
tree_store_test_insert_before_NULL (void)
{
  CtkTreeIter iter, iter2;
  CtkTreeIter iter_copy;
  CtkTreeStore *store;

  store = ctk_tree_store_new (1, G_TYPE_INT);

  ctk_tree_store_append (store, &iter, NULL);

  /* move_before NULL is basically an append */
  ctk_tree_store_insert_before (store, &iter2, NULL, NULL);
  g_assert (ctk_tree_store_iter_is_valid (store, &iter2));
  g_assert (ctk_tree_model_iter_n_children (CTK_TREE_MODEL (store), NULL) == 2);

  /* Walk over the model */
  g_assert (ctk_tree_model_get_iter_first (CTK_TREE_MODEL (store), &iter_copy));
  g_assert (iters_equal (&iter, &iter_copy));
  g_assert (iter_position (store, &iter, 0));

  g_assert (ctk_tree_model_iter_next (CTK_TREE_MODEL (store), &iter_copy));
  g_assert (iters_equal (&iter2, &iter_copy));
  g_assert (iter_position (store, &iter2, 1));

  g_assert (!ctk_tree_model_iter_next (CTK_TREE_MODEL (store), &iter_copy));

  g_assert (ctk_tree_model_iter_nth_child (CTK_TREE_MODEL (store), &iter_copy, NULL, 1));
  g_assert (iters_equal (&iter2, &iter_copy));
  g_assert (iter_position (store, &iter2, 1));

  g_assert (ctk_tree_model_iter_previous (CTK_TREE_MODEL (store), &iter_copy));
  g_assert (iters_equal (&iter, &iter_copy));
  g_assert (iter_position (store, &iter, 0));

  g_assert (!ctk_tree_model_iter_previous (CTK_TREE_MODEL (store), &iter_copy));

  g_object_unref (store);
}

/* setting values */
static void
tree_store_set_gvalue_to_transform (void)
{
  CtkTreeStore *store;
  CtkTreeIter iter;
  GValue value = G_VALUE_INIT;

  /* https://bugzilla.gnome.org/show_bug.cgi?id=677649 */
  store = ctk_tree_store_new (1, G_TYPE_LONG);
  ctk_tree_store_append (store, &iter, NULL);

  g_value_init (&value, G_TYPE_INT);
  g_value_set_int (&value, 42);
  ctk_tree_store_set_value (store, &iter, 0, &value);
}

/* removal */
static void
tree_store_test_remove_begin (TreeStore     *fixture,
			      gconstpointer  user_data)
{
  int new_order[5] = { -1, 1, 2, 3, 4 };
  CtkTreePath *path;
  CtkTreeIter iter;

  /* Remove node at 0 */
  path = ctk_tree_path_new_from_indices (0, -1);
  ctk_tree_model_get_iter (CTK_TREE_MODEL (fixture->store), &iter, path);
  ctk_tree_path_free (path);

  g_assert (ctk_tree_store_remove (fixture->store, &iter) == TRUE);
  g_assert (!ctk_tree_store_iter_is_valid (fixture->store, &fixture->iter[0]));
  g_assert (iters_equal (&iter, &fixture->iter[1]));

  check_model (fixture, new_order, 0);
}

static void
tree_store_test_remove_middle (TreeStore     *fixture,
			       gconstpointer  user_data)
{
  int new_order[5] = { 0, 1, -1, 3, 4 };
  CtkTreePath *path;
  CtkTreeIter iter;

  /* Remove node at 2 */
  path = ctk_tree_path_new_from_indices (2, -1);
  ctk_tree_model_get_iter (CTK_TREE_MODEL (fixture->store), &iter, path);
  ctk_tree_path_free (path);

  g_assert (ctk_tree_store_remove (fixture->store, &iter) == TRUE);
  g_assert (!ctk_tree_store_iter_is_valid (fixture->store, &fixture->iter[2]));
  g_assert (iters_equal (&iter, &fixture->iter[3]));

  check_model (fixture, new_order, 2);
}

static void
tree_store_test_remove_end (TreeStore     *fixture,
			    gconstpointer  user_data)
{
  int new_order[5] = { 0, 1, 2, 3, -1 };
  CtkTreePath *path;
  CtkTreeIter iter;

  /* Remove node at 4 */
  path = ctk_tree_path_new_from_indices (4, -1);
  ctk_tree_model_get_iter (CTK_TREE_MODEL (fixture->store), &iter, path);
  ctk_tree_path_free (path);

  g_assert (ctk_tree_store_remove (fixture->store, &iter) == FALSE);
  g_assert (!ctk_tree_store_iter_is_valid (fixture->store, &fixture->iter[4]));

  check_model (fixture, new_order, 4);
}

static void
tree_store_test_clear (TreeStore     *fixture,
		       gconstpointer  user_data)
{
  int i;

  ctk_tree_store_clear (fixture->store);

  g_assert (ctk_tree_model_iter_n_children (CTK_TREE_MODEL (fixture->store), NULL) == 0);

  for (i = 0; i < 5; i++)
    g_assert (!ctk_tree_store_iter_is_valid (fixture->store, &fixture->iter[i]));
}

/* reorder */

static void
tree_store_test_reorder (TreeStore     *fixture,
			 gconstpointer  user_data)
{
  int new_order[5] = { 4, 1, 0, 2, 3 };

  ctk_tree_store_reorder (fixture->store, NULL, new_order);
  check_model (fixture, new_order, -1);
}

/* swapping */

static void
tree_store_test_swap_begin (TreeStore     *fixture,
		            gconstpointer  user_data)
{
  /* We swap nodes 0 and 1 at the beginning */
  int new_order[5] = { 1, 0, 2, 3, 4 };

  CtkTreeIter iter_a;
  CtkTreeIter iter_b;

  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter_a, "0"));
  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter_b, "1"));

  ctk_tree_store_swap (fixture->store, &iter_a, &iter_b);
  check_model (fixture, new_order, -1);
}

static void
tree_store_test_swap_middle_next (TreeStore     *fixture,
		                  gconstpointer  user_data)
{
  /* We swap nodes 2 and 3 in the middle that are next to each other */
  int new_order[5] = { 0, 1, 3, 2, 4 };

  CtkTreeIter iter_a;
  CtkTreeIter iter_b;

  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter_a, "2"));
  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter_b, "3"));

  ctk_tree_store_swap (fixture->store, &iter_a, &iter_b);
  check_model (fixture, new_order, -1);
}

static void
tree_store_test_swap_middle_apart (TreeStore     *fixture,
		                   gconstpointer  user_data)
{
  /* We swap nodes 1 and 3 in the middle that are apart from each other */
  int new_order[5] = { 0, 3, 2, 1, 4 };

  CtkTreeIter iter_a;
  CtkTreeIter iter_b;

  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter_a, "1"));
  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter_b, "3"));

  ctk_tree_store_swap (fixture->store, &iter_a, &iter_b);
  check_model (fixture, new_order, -1);
}

static void
tree_store_test_swap_end (TreeStore     *fixture,
		          gconstpointer  user_data)
{
  /* We swap nodes 3 and 4 at the end */
  int new_order[5] = { 0, 1, 2, 4, 3 };

  CtkTreeIter iter_a;
  CtkTreeIter iter_b;

  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter_a, "3"));
  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter_b, "4"));

  ctk_tree_store_swap (fixture->store, &iter_a, &iter_b);
  check_model (fixture, new_order, -1);
}

static void
tree_store_test_swap_single (void)
{
  CtkTreeIter iter;
  CtkTreeIter iter_copy;
  CtkTreeStore *store;

  store = ctk_tree_store_new (1, G_TYPE_INT);

  /* Check if swap on a store with a single node does not corrupt
   * the store.
   */

  ctk_tree_store_append (store, &iter, NULL);
  iter_copy = iter;

  ctk_tree_store_swap (store, &iter, &iter);
  g_assert (iters_equal (&iter, &iter_copy));
  g_assert (ctk_tree_model_get_iter_first (CTK_TREE_MODEL (store), &iter));
  g_assert (iters_equal (&iter, &iter_copy));

  g_object_unref (store);
}

/* move after */

static void
tree_store_test_move_after_from_start (TreeStore     *fixture,
				       gconstpointer  user_data)
{
  /* We move node 0 after 2 */
  int new_order[5] = { 1, 2, 0, 3, 4 };

  CtkTreeIter iter;
  CtkTreeIter position;

  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter, "0"));
  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &position, "2"));

  ctk_tree_store_move_after (fixture->store, &iter, &position);
  check_model (fixture, new_order, -1);
}

static void
tree_store_test_move_after_next (TreeStore     *fixture,
			         gconstpointer  user_data)
{
  /* We move node 2 after 3 */
  int new_order[5] = { 0, 1, 3, 2, 4 };

  CtkTreeIter iter;
  CtkTreeIter position;

  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter, "2"));
  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &position, "3"));

  ctk_tree_store_move_after (fixture->store, &iter, &position);
  check_model (fixture, new_order, -1);
}

static void
tree_store_test_move_after_apart (TreeStore     *fixture,
			          gconstpointer  user_data)
{
  /* We move node 1 after 3 */
  int new_order[5] = { 0, 2, 3, 1, 4 };

  CtkTreeIter iter;
  CtkTreeIter position;

  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter, "1"));
  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &position, "3"));

  ctk_tree_store_move_after (fixture->store, &iter, &position);
  check_model (fixture, new_order, -1);
}

static void
tree_store_test_move_after_end (TreeStore     *fixture,
			        gconstpointer  user_data)
{
  /* We move node 2 after 4 */
  int new_order[5] = { 0, 1, 3, 4, 2 };

  CtkTreeIter iter;
  CtkTreeIter position;

  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter, "2"));
  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &position, "4"));

  ctk_tree_store_move_after (fixture->store, &iter, &position);
  check_model (fixture, new_order, -1);
}

static void
tree_store_test_move_after_from_end (TreeStore     *fixture,
			             gconstpointer  user_data)
{
  /* We move node 4 after 1 */
  int new_order[5] = { 0, 1, 4, 2, 3 };

  CtkTreeIter iter;
  CtkTreeIter position;

  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter, "4"));
  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &position, "1"));

  ctk_tree_store_move_after (fixture->store, &iter, &position);
  check_model (fixture, new_order, -1);
}

static void
tree_store_test_move_after_change_ends (TreeStore     *fixture,
			                gconstpointer  user_data)
{
  /* We move 0 after 4, this will cause both the head and tail ends to
   * change.
   */
  int new_order[5] = { 1, 2, 3, 4, 0 };

  CtkTreeIter iter;
  CtkTreeIter position;

  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter, "0"));
  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &position, "4"));

  ctk_tree_store_move_after (fixture->store, &iter, &position);
  check_model (fixture, new_order, -1);
}

static void
tree_store_test_move_after_NULL (TreeStore     *fixture,
			         gconstpointer  user_data)
{
  /* We move node 2, NULL should prepend */
  int new_order[5] = { 2, 0, 1, 3, 4 };

  CtkTreeIter iter;

  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter, "2"));

  ctk_tree_store_move_after (fixture->store, &iter, NULL);
  check_model (fixture, new_order, -1);
}

static void
tree_store_test_move_after_single (void)
{
  CtkTreeIter iter;
  CtkTreeIter iter_copy;
  CtkTreeStore *store;

  store = ctk_tree_store_new (1, G_TYPE_INT);

  /* Check if move-after on a store with a single node does not corrupt
   * the store.
   */

  ctk_tree_store_append (store, &iter, NULL);
  iter_copy = iter;

  ctk_tree_store_move_after (store, &iter, NULL);
  g_assert (iters_equal (&iter, &iter_copy));
  g_assert (ctk_tree_model_get_iter_first (CTK_TREE_MODEL (store), &iter));
  g_assert (iters_equal (&iter, &iter_copy));

  ctk_tree_store_move_after (store, &iter, &iter);
  g_assert (iters_equal (&iter, &iter_copy));
  g_assert (ctk_tree_model_get_iter_first (CTK_TREE_MODEL (store), &iter));
  g_assert (iters_equal (&iter, &iter_copy));

  g_object_unref (store);
}

/* move before */

static void
tree_store_test_move_before_next (TreeStore     *fixture,
		                  gconstpointer  user_data)
{
  /* We move node 3 before 2 */
  int new_order[5] = { 0, 1, 3, 2, 4 };

  CtkTreeIter iter;
  CtkTreeIter position;

  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter, "3"));
  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &position, "2"));

  ctk_tree_store_move_before (fixture->store, &iter, &position);
  check_model (fixture, new_order, -1);
}

static void
tree_store_test_move_before_apart (TreeStore     *fixture,
				   gconstpointer  user_data)
{
  /* We move node 1 before 3 */
  int new_order[5] = { 0, 2, 1, 3, 4 };

  CtkTreeIter iter;
  CtkTreeIter position;

  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter, "1"));
  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &position, "3"));

  ctk_tree_store_move_before (fixture->store, &iter, &position);
  check_model (fixture, new_order, -1);
}

static void
tree_store_test_move_before_to_start (TreeStore     *fixture,
				      gconstpointer  user_data)
{
  /* We move node 2 before 0 */
  int new_order[5] = { 2, 0, 1, 3, 4 };

  CtkTreeIter iter;
  CtkTreeIter position;

  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter, "2"));
  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &position, "0"));

  ctk_tree_store_move_before (fixture->store, &iter, &position);
  check_model (fixture, new_order, -1);
}

static void
tree_store_test_move_before_from_end (TreeStore     *fixture,
			              gconstpointer  user_data)
{
  /* We move node 4 before 2 (replace end) */
  int new_order[5] = { 0, 1, 4, 2, 3 };

  CtkTreeIter iter;
  CtkTreeIter position;

  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter, "4"));
  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &position, "2"));

  ctk_tree_store_move_before (fixture->store, &iter, &position);
  check_model (fixture, new_order, -1);
}

static void
tree_store_test_move_before_change_ends (TreeStore     *fixture,
				         gconstpointer  user_data)
{
  /* We move node 4 before 0 */
  int new_order[5] = { 4, 0, 1, 2, 3 };

  CtkTreeIter iter;
  CtkTreeIter position;

  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter, "4"));
  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &position, "0"));

  ctk_tree_store_move_before (fixture->store, &iter, &position);
  check_model (fixture, new_order, -1);
}

static void
tree_store_test_move_before_NULL (TreeStore     *fixture,
			          gconstpointer  user_data)
{
  /* We move node 2, NULL should append */
  int new_order[5] = { 0, 1, 3, 4, 2 };

  CtkTreeIter iter;

  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter, "2"));

  ctk_tree_store_move_before (fixture->store, &iter, NULL);
  check_model (fixture, new_order, -1);
}

static void
tree_store_test_move_before_single (void)
{
  CtkTreeIter iter;
  CtkTreeIter iter_copy;
  CtkTreeStore *store;

  store = ctk_tree_store_new (1, G_TYPE_INT);

  /* Check if move-after on a store with a single node does not corrupt
   * the store.
   */

  ctk_tree_store_append (store, &iter, NULL);
  iter_copy = iter;

  ctk_tree_store_move_before (store, &iter, NULL);
  g_assert (iters_equal (&iter, &iter_copy));
  g_assert (ctk_tree_model_get_iter_first (CTK_TREE_MODEL (store), &iter));
  g_assert (iters_equal (&iter, &iter_copy));

  ctk_tree_store_move_before (store, &iter, &iter);
  g_assert (iters_equal (&iter, &iter_copy));
  g_assert (ctk_tree_model_get_iter_first (CTK_TREE_MODEL (store), &iter));
  g_assert (iters_equal (&iter, &iter_copy));

  g_object_unref (store);
}


/* iter invalidation */

static void
tree_store_test_iter_previous_invalid (TreeStore     *fixture,
                                       gconstpointer  user_data)
{
  CtkTreeIter iter;

  ctk_tree_model_get_iter_first (CTK_TREE_MODEL (fixture->store), &iter);

  g_assert (ctk_tree_model_iter_previous (CTK_TREE_MODEL (fixture->store),
                                          &iter) == FALSE);
  g_assert (ctk_tree_store_iter_is_valid (fixture->store, &iter) == FALSE);
  g_assert (iter.stamp == 0);
}

static void
tree_store_test_iter_next_invalid (TreeStore     *fixture,
                                   gconstpointer  user_data)
{
  CtkTreePath *path;
  CtkTreeIter iter;

  path = ctk_tree_path_new_from_indices (4, -1);
  ctk_tree_model_get_iter (CTK_TREE_MODEL (fixture->store), &iter, path);
  ctk_tree_path_free (path);

  g_assert (ctk_tree_model_iter_next (CTK_TREE_MODEL (fixture->store),
                                      &iter) == FALSE);
  g_assert (ctk_tree_store_iter_is_valid (fixture->store, &iter) == FALSE);
  g_assert (iter.stamp == 0);
}

static void
tree_store_test_iter_children_invalid (TreeStore     *fixture,
                                       gconstpointer  user_data)
{
  CtkTreeIter iter, child;

  ctk_tree_model_get_iter_first (CTK_TREE_MODEL (fixture->store), &iter);
  g_assert (ctk_tree_store_iter_is_valid (fixture->store, &iter) == TRUE);

  g_assert (ctk_tree_model_iter_children (CTK_TREE_MODEL (fixture->store),
                                          &child, &iter) == FALSE);
  g_assert (ctk_tree_store_iter_is_valid (fixture->store, &child) == FALSE);
  g_assert (child.stamp == 0);
}

static void
tree_store_test_iter_nth_child_invalid (TreeStore     *fixture,
                                        gconstpointer  user_data)
{
  CtkTreeIter iter, child;

  ctk_tree_model_get_iter_first (CTK_TREE_MODEL (fixture->store), &iter);
  g_assert (ctk_tree_store_iter_is_valid (fixture->store, &iter) == TRUE);

  g_assert (ctk_tree_model_iter_nth_child (CTK_TREE_MODEL (fixture->store),
                                           &child, &iter, 0) == FALSE);
  g_assert (ctk_tree_store_iter_is_valid (fixture->store, &child) == FALSE);
  g_assert (child.stamp == 0);
}

static void
tree_store_test_iter_parent_invalid (TreeStore     *fixture,
                                     gconstpointer  user_data)
{
  CtkTreeIter iter, child;

  ctk_tree_model_get_iter_first (CTK_TREE_MODEL (fixture->store), &child);
  g_assert (ctk_tree_store_iter_is_valid (fixture->store, &child) == TRUE);

  g_assert (ctk_tree_model_iter_parent (CTK_TREE_MODEL (fixture->store),
                                        &iter, &child) == FALSE);
  g_assert (ctk_tree_store_iter_is_valid (fixture->store, &iter) == FALSE);
  g_assert (iter.stamp == 0);
}

/* specific bugs */
static void
specific_bug_77977 (void)
{
  CtkTreeStore *tree_store;
  CtkTreeIter iter1, iter2, iter3;
  CtkTreePath *path;
  CtkTreeRowReference *row_ref;

  /* Stripped down version of test case for bug 77977 by Damon Chaplin */

  g_test_bug ("77977");

  tree_store = ctk_tree_store_new (1, G_TYPE_STRING);

  ctk_tree_store_append (tree_store, &iter1, NULL);
  ctk_tree_store_set (tree_store, &iter1, 0, "Window1", -1);

  ctk_tree_store_append (tree_store, &iter2, &iter1);
  ctk_tree_store_set (tree_store, &iter2, 0, "Table1", -1);

  ctk_tree_store_append (tree_store, &iter3, &iter2);
  ctk_tree_store_set (tree_store, &iter3, 0, "Button1", -1);

  path = ctk_tree_path_new_from_indices (0, 0, 0, -1);
  row_ref = ctk_tree_row_reference_new (CTK_TREE_MODEL (tree_store), path);
  ctk_tree_path_free (path);

  ctk_tree_store_remove (tree_store, &iter1);

  ctk_tree_row_reference_free (row_ref);
  g_object_unref (tree_store);
}

static void
specific_bug_698396 (void)
{
  g_test_bug ("698396");

  if (g_test_subprocess ())
    {
      CtkTreeStore *tree_store;
      int new_order[1] = { 0 };

      tree_store = ctk_tree_store_new (1, G_TYPE_STRING);
      ctk_tree_store_reorder (tree_store, NULL, new_order);
      g_object_unref (tree_store);

      return;
    }

  g_test_trap_subprocess (NULL, 0, 0);
  g_test_trap_assert_stderr ("*Cannot reorder, parent has no children*");
  g_test_trap_assert_failed ();
}

/* main */

void
register_tree_store_tests (void)
{
  /* insertion */
  g_test_add_func ("/TreeStore/insert-high-values",
	           tree_store_test_insert_high_values);
  g_test_add_func ("/TreeStore/append",
		   tree_store_test_append);
  g_test_add_func ("/TreeStore/prepend",
		   tree_store_test_prepend);
  g_test_add_func ("/TreeStore/insert-after",
		   tree_store_test_insert_after);
  g_test_add_func ("/TreeStore/insert-after-NULL",
		   tree_store_test_insert_after_NULL);
  g_test_add_func ("/TreeStore/insert-before",
		   tree_store_test_insert_before);
  g_test_add_func ("/TreeStore/insert-before-NULL",
		   tree_store_test_insert_before_NULL);

  /* setting values (FIXME) */
  g_test_add_func ("/TreeStore/set-gvalue-to-transform",
                   tree_store_set_gvalue_to_transform);

  /* removal */
  g_test_add ("/TreeStore/remove-begin", TreeStore, NULL,
	      tree_store_setup, tree_store_test_remove_begin,
	      tree_store_teardown);
  g_test_add ("/TreeStore/remove-middle", TreeStore, NULL,
	      tree_store_setup, tree_store_test_remove_middle,
	      tree_store_teardown);
  g_test_add ("/TreeStore/remove-end", TreeStore, NULL,
	      tree_store_setup, tree_store_test_remove_end,
	      tree_store_teardown);

  g_test_add ("/TreeStore/clear", TreeStore, NULL,
	      tree_store_setup, tree_store_test_clear,
	      tree_store_teardown);

  /* reordering */
  g_test_add ("/TreeStore/reorder", TreeStore, NULL,
	      tree_store_setup, tree_store_test_reorder,
	      tree_store_teardown);

  /* swapping */
  g_test_add ("/TreeStore/swap-begin", TreeStore, NULL,
	      tree_store_setup, tree_store_test_swap_begin,
	      tree_store_teardown);
  g_test_add ("/TreeStore/swap-middle-next", TreeStore, NULL,
	      tree_store_setup, tree_store_test_swap_middle_next,
	      tree_store_teardown);
  g_test_add ("/TreeStore/swap-middle-apart", TreeStore, NULL,
	      tree_store_setup, tree_store_test_swap_middle_apart,
	      tree_store_teardown);
  g_test_add ("/TreeStore/swap-end", TreeStore, NULL,
	      tree_store_setup, tree_store_test_swap_end,
	      tree_store_teardown);
  g_test_add_func ("/TreeStore/swap-single",
		   tree_store_test_swap_single);

  /* moving */
  g_test_add ("/TreeStore/move-after-from-start", TreeStore, NULL,
	      tree_store_setup, tree_store_test_move_after_from_start,
	      tree_store_teardown);
  g_test_add ("/TreeStore/move-after-next", TreeStore, NULL,
	      tree_store_setup, tree_store_test_move_after_next,
	      tree_store_teardown);
  g_test_add ("/TreeStore/move-after-apart", TreeStore, NULL,
	      tree_store_setup, tree_store_test_move_after_apart,
	      tree_store_teardown);
  g_test_add ("/TreeStore/move-after-end", TreeStore, NULL,
	      tree_store_setup, tree_store_test_move_after_end,
	      tree_store_teardown);
  g_test_add ("/TreeStore/move-after-from-end", TreeStore, NULL,
	      tree_store_setup, tree_store_test_move_after_from_end,
	      tree_store_teardown);
  g_test_add ("/TreeStore/move-after-change-ends", TreeStore, NULL,
	      tree_store_setup, tree_store_test_move_after_change_ends,
	      tree_store_teardown);
  g_test_add ("/TreeStore/move-after-NULL", TreeStore, NULL,
	      tree_store_setup, tree_store_test_move_after_NULL,
	      tree_store_teardown);
  g_test_add_func ("/TreeStore/move-after-single",
		   tree_store_test_move_after_single);

  g_test_add ("/TreeStore/move-before-next", TreeStore, NULL,
	      tree_store_setup, tree_store_test_move_before_next,
	      tree_store_teardown);
  g_test_add ("/TreeStore/move-before-apart", TreeStore, NULL,
	      tree_store_setup, tree_store_test_move_before_apart,
	      tree_store_teardown);
  g_test_add ("/TreeStore/move-before-to-start", TreeStore, NULL,
	      tree_store_setup, tree_store_test_move_before_to_start,
	      tree_store_teardown);
  g_test_add ("/TreeStore/move-before-from-end", TreeStore, NULL,
	      tree_store_setup, tree_store_test_move_before_from_end,
	      tree_store_teardown);
  g_test_add ("/TreeStore/move-before-change-ends", TreeStore, NULL,
	      tree_store_setup, tree_store_test_move_before_change_ends,
	      tree_store_teardown);
  g_test_add ("/TreeStore/move-before-NULL", TreeStore, NULL,
	      tree_store_setup, tree_store_test_move_before_NULL,
	      tree_store_teardown);
  g_test_add_func ("/TreeStore/move-before-single",
		   tree_store_test_move_before_single);

  /* iter invalidation */
  g_test_add ("/TreeStore/iter-prev-invalid", TreeStore, NULL,
              tree_store_setup, tree_store_test_iter_previous_invalid,
              tree_store_teardown);
  g_test_add ("/TreeStore/iter-next-invalid", TreeStore, NULL,
              tree_store_setup, tree_store_test_iter_next_invalid,
              tree_store_teardown);
  g_test_add ("/TreeStore/iter-children-invalid", TreeStore, NULL,
              tree_store_setup, tree_store_test_iter_children_invalid,
              tree_store_teardown);
  g_test_add ("/TreeStore/iter-nth-child-invalid", TreeStore, NULL,
              tree_store_setup, tree_store_test_iter_nth_child_invalid,
              tree_store_teardown);
  g_test_add ("/TreeStore/iter-parent-invalid", TreeStore, NULL,
              tree_store_setup, tree_store_test_iter_parent_invalid,
              tree_store_teardown);

  /* specific bugs */
  g_test_add_func ("/TreeStore/bug-77977", specific_bug_77977);
  g_test_add_func ("/TreeStore/bug-698396", specific_bug_698396);
}
