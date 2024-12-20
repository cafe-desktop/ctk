/* Extensive CtkListStore tests.
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
 *  - Test implementations of the interfaces: DnD, sortable, buildable
 *    and the tree model interface itself?
 *  - Need to check if the emitted signals are right.
 *  - Needs analysis with the code coverage tool once it is there.
 */

#include <ctk/ctk.h>

#include "treemodel.h"

static inline gboolean
iters_equal (CtkTreeIter *a,
	     CtkTreeIter *b)
{
  if (a->stamp != b->stamp)
    return FALSE;

  if (a->user_data != b->user_data)
    return FALSE;

  /* user_data2 and user_data3 are not used in CtkListStore */

  return TRUE;
}

static gboolean
iter_position (CtkListStore *store,
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
  CtkListStore *store;
} ListStore;

static void
list_store_setup (ListStore     *fixture,
		  gconstpointer  test_data G_GNUC_UNUSED)
{
  int i;

  fixture->store = ctk_list_store_new (1, G_TYPE_INT);

  for (i = 0; i < 5; i++)
    {
      ctk_list_store_insert (fixture->store, &fixture->iter[i], i);
      ctk_list_store_set (fixture->store, &fixture->iter[i], 0, i, -1);
    }
}

static void
list_store_teardown (ListStore     *fixture,
		     gconstpointer  test_data G_GNUC_UNUSED)
{
  g_object_unref (fixture->store);
}

/*
 * The actual tests.
 */

static void
check_model (ListStore *fixture,
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

      g_assert (ctk_list_store_iter_is_valid (fixture->store, &iter));
      g_assert (iters_equal (&iter, &fixture->iter[new_order[i]]));

      ctk_tree_path_next (path);
    }

  ctk_tree_path_free (path);
}

/* insertion */
static void
list_store_test_insert_high_values (void)
{
  CtkTreeIter iter, iter2;
  CtkTreeIter iter_copy;
  CtkListStore *store;

  store = ctk_list_store_new (1, G_TYPE_INT);

  ctk_list_store_insert (store, &iter, 1234);
  g_assert (ctk_list_store_iter_is_valid (store, &iter));
  g_assert (ctk_tree_model_iter_n_children (CTK_TREE_MODEL (store), NULL) == 1);
  g_assert (ctk_tree_model_get_iter_first (CTK_TREE_MODEL (store), &iter_copy));
  g_assert (iters_equal (&iter, &iter_copy));
  g_assert (iter_position (store, &iter, 0));

  ctk_list_store_insert (store, &iter2, 765);
  g_assert (ctk_list_store_iter_is_valid (store, &iter2));
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
list_store_test_append (void)
{
  CtkTreeIter iter, iter2;
  CtkTreeIter iter_copy;
  CtkListStore *store;

  store = ctk_list_store_new (1, G_TYPE_INT);

  ctk_list_store_append (store, &iter);
  g_assert (ctk_list_store_iter_is_valid (store, &iter));
  g_assert (ctk_tree_model_iter_n_children (CTK_TREE_MODEL (store), NULL) == 1);
  g_assert (ctk_tree_model_get_iter_first (CTK_TREE_MODEL (store), &iter_copy));
  g_assert (iters_equal (&iter, &iter_copy));
  g_assert (iter_position (store, &iter, 0));

  ctk_list_store_append (store, &iter2);
  g_assert (ctk_list_store_iter_is_valid (store, &iter2));
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
list_store_test_prepend (void)
{
  CtkTreeIter iter, iter2;
  CtkTreeIter iter_copy;
  CtkListStore *store;

  store = ctk_list_store_new (1, G_TYPE_INT);

  ctk_list_store_prepend (store, &iter);
  g_assert (ctk_list_store_iter_is_valid (store, &iter));
  g_assert (ctk_tree_model_iter_n_children (CTK_TREE_MODEL (store), NULL) == 1);
  g_assert (ctk_tree_model_get_iter_first (CTK_TREE_MODEL (store), &iter_copy));
  g_assert (iters_equal (&iter, &iter_copy));
  g_assert (iter_position (store, &iter, 0));

  ctk_list_store_prepend (store, &iter2);
  g_assert (ctk_list_store_iter_is_valid (store, &iter2));
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
list_store_test_insert_after (void)
{
  CtkTreeIter iter, iter2, iter3;
  CtkTreeIter iter_copy;
  CtkListStore *store;

  store = ctk_list_store_new (1, G_TYPE_INT);

  ctk_list_store_append (store, &iter);
  ctk_list_store_append (store, &iter2);

  ctk_list_store_insert_after (store, &iter3, &iter);
  g_assert (ctk_list_store_iter_is_valid (store, &iter3));
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
list_store_test_insert_after_NULL (void)
{
  CtkTreeIter iter, iter2;
  CtkTreeIter iter_copy;
  CtkListStore *store;

  store = ctk_list_store_new (1, G_TYPE_INT);

  ctk_list_store_append (store, &iter);

  /* move_after NULL is basically a prepend */
  ctk_list_store_insert_after (store, &iter2, NULL);
  g_assert (ctk_list_store_iter_is_valid (store, &iter2));
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
list_store_test_insert_before (void)
{
  CtkTreeIter iter, iter2, iter3;
  CtkTreeIter iter_copy;
  CtkListStore *store;

  store = ctk_list_store_new (1, G_TYPE_INT);

  ctk_list_store_append (store, &iter);
  ctk_list_store_append (store, &iter2);

  ctk_list_store_insert_before (store, &iter3, &iter2);
  g_assert (ctk_list_store_iter_is_valid (store, &iter3));
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
list_store_test_insert_before_NULL (void)
{
  CtkTreeIter iter, iter2;
  CtkTreeIter iter_copy;
  CtkListStore *store;

  store = ctk_list_store_new (1, G_TYPE_INT);

  ctk_list_store_append (store, &iter);

  /* move_before NULL is basically an append */
  ctk_list_store_insert_before (store, &iter2, NULL);
  g_assert (ctk_list_store_iter_is_valid (store, &iter2));
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
list_store_set_gvalue_to_transform (void)
{
  CtkListStore *store;
  CtkTreeIter iter;
  GValue value = G_VALUE_INIT;

  /* https://bugzilla.gnome.org/show_bug.cgi?id=677649 */
  store = ctk_list_store_new (1, G_TYPE_LONG);
  ctk_list_store_append (store, &iter);

  g_value_init (&value, G_TYPE_INT);
  g_value_set_int (&value, 42);
  ctk_list_store_set_value (store, &iter, 0, &value);
}

/* removal */
static void
list_store_test_remove_begin (ListStore     *fixture,
			      gconstpointer  user_data G_GNUC_UNUSED)
{
  int new_order[5] = { -1, 1, 2, 3, 4 };
  CtkTreePath *path;
  CtkTreeIter iter;

  /* Remove node at 0 */
  path = ctk_tree_path_new_from_indices (0, -1);
  ctk_tree_model_get_iter (CTK_TREE_MODEL (fixture->store), &iter, path);
  ctk_tree_path_free (path);

  g_assert (ctk_list_store_remove (fixture->store, &iter) == TRUE);
  g_assert (!ctk_list_store_iter_is_valid (fixture->store, &fixture->iter[0]));
  g_assert (iters_equal (&iter, &fixture->iter[1]));

  check_model (fixture, new_order, 0);
}

static void
list_store_test_remove_middle (ListStore     *fixture,
			       gconstpointer  user_data G_GNUC_UNUSED)
{
  int new_order[5] = { 0, 1, -1, 3, 4 };
  CtkTreePath *path;
  CtkTreeIter iter;

  /* Remove node at 2 */
  path = ctk_tree_path_new_from_indices (2, -1);
  ctk_tree_model_get_iter (CTK_TREE_MODEL (fixture->store), &iter, path);
  ctk_tree_path_free (path);

  g_assert (ctk_list_store_remove (fixture->store, &iter) == TRUE);
  g_assert (!ctk_list_store_iter_is_valid (fixture->store, &fixture->iter[2]));
  g_assert (iters_equal (&iter, &fixture->iter[3]));

  check_model (fixture, new_order, 2);
}

static void
list_store_test_remove_end (ListStore     *fixture,
			    gconstpointer  user_data G_GNUC_UNUSED)
{
  int new_order[5] = { 0, 1, 2, 3, -1 };
  CtkTreePath *path;
  CtkTreeIter iter;

  /* Remove node at 4 */
  path = ctk_tree_path_new_from_indices (4, -1);
  ctk_tree_model_get_iter (CTK_TREE_MODEL (fixture->store), &iter, path);
  ctk_tree_path_free (path);

  g_assert (ctk_list_store_remove (fixture->store, &iter) == FALSE);
  g_assert (!ctk_list_store_iter_is_valid (fixture->store, &fixture->iter[4]));

  check_model (fixture, new_order, 4);
}

static void
list_store_test_clear (ListStore     *fixture,
		       gconstpointer  user_data G_GNUC_UNUSED)
{
  int i;

  ctk_list_store_clear (fixture->store);

  g_assert (ctk_tree_model_iter_n_children (CTK_TREE_MODEL (fixture->store), NULL) == 0);

  for (i = 0; i < 5; i++)
    g_assert (!ctk_list_store_iter_is_valid (fixture->store, &fixture->iter[i]));
}

/* reorder */

static void
list_store_test_reorder (ListStore     *fixture,
			 gconstpointer  user_data G_GNUC_UNUSED)
{
  int new_order[5] = { 4, 1, 0, 2, 3 };

  ctk_list_store_reorder (fixture->store, new_order);
  check_model (fixture, new_order, -1);
}

/* swapping */

static void
list_store_test_swap_begin (ListStore     *fixture,
			    gconstpointer  user_data G_GNUC_UNUSED)
{
  /* We swap nodes 0 and 1 at the beginning */
  int new_order[5] = { 1, 0, 2, 3, 4 };

  CtkTreeIter iter_a;
  CtkTreeIter iter_b;

  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter_a, "0"));
  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter_b, "1"));

  ctk_list_store_swap (fixture->store, &iter_a, &iter_b);
  check_model (fixture, new_order, -1);
}

static void
list_store_test_swap_middle_next (ListStore     *fixture,
				  gconstpointer  user_data G_GNUC_UNUSED)
{
  /* We swap nodes 2 and 3 in the middle that are next to each other */
  int new_order[5] = { 0, 1, 3, 2, 4 };

  CtkTreeIter iter_a;
  CtkTreeIter iter_b;

  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter_a, "2"));
  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter_b, "3"));

  ctk_list_store_swap (fixture->store, &iter_a, &iter_b);
  check_model (fixture, new_order, -1);
}

static void
list_store_test_swap_middle_apart (ListStore     *fixture,
				   gconstpointer  user_data G_GNUC_UNUSED)
{
  /* We swap nodes 1 and 3 in the middle that are apart from each other */
  int new_order[5] = { 0, 3, 2, 1, 4 };

  CtkTreeIter iter_a;
  CtkTreeIter iter_b;

  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter_a, "1"));
  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter_b, "3"));

  ctk_list_store_swap (fixture->store, &iter_a, &iter_b);
  check_model (fixture, new_order, -1);
}

static void
list_store_test_swap_end (ListStore     *fixture,
			  gconstpointer  user_data G_GNUC_UNUSED)
{
  /* We swap nodes 3 and 4 at the end */
  int new_order[5] = { 0, 1, 2, 4, 3 };

  CtkTreeIter iter_a;
  CtkTreeIter iter_b;

  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter_a, "3"));
  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter_b, "4"));

  ctk_list_store_swap (fixture->store, &iter_a, &iter_b);
  check_model (fixture, new_order, -1);
}

static void
list_store_test_swap_single (void)
{
  CtkTreeIter iter;
  CtkTreeIter iter_copy;
  CtkListStore *store;

  store = ctk_list_store_new (1, G_TYPE_INT);

  /* Check if swap on a store with a single node does not corrupt
   * the store.
   */

  ctk_list_store_append (store, &iter);
  iter_copy = iter;

  ctk_list_store_swap (store, &iter, &iter);
  g_assert (iters_equal (&iter, &iter_copy));
  g_assert (ctk_tree_model_get_iter_first (CTK_TREE_MODEL (store), &iter));
  g_assert (iters_equal (&iter, &iter_copy));

  g_object_unref (store);
}

/* move after */

static void
list_store_test_move_after_from_start (ListStore     *fixture,
				       gconstpointer  user_data G_GNUC_UNUSED)
{
  /* We move node 0 after 2 */
  int new_order[5] = { 1, 2, 0, 3, 4 };

  CtkTreeIter iter;
  CtkTreeIter position;

  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter, "0"));
  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &position, "2"));

  ctk_list_store_move_after (fixture->store, &iter, &position);
  check_model (fixture, new_order, -1);
}

static void
list_store_test_move_after_next (ListStore     *fixture,
				 gconstpointer  user_data G_GNUC_UNUSED)
{
  /* We move node 2 after 3 */
  int new_order[5] = { 0, 1, 3, 2, 4 };

  CtkTreeIter iter;
  CtkTreeIter position;

  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter, "2"));
  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &position, "3"));

  ctk_list_store_move_after (fixture->store, &iter, &position);
  check_model (fixture, new_order, -1);
}

static void
list_store_test_move_after_apart (ListStore     *fixture,
				  gconstpointer  user_data G_GNUC_UNUSED)
{
  /* We move node 1 after 3 */
  int new_order[5] = { 0, 2, 3, 1, 4 };

  CtkTreeIter iter;
  CtkTreeIter position;

  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter, "1"));
  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &position, "3"));

  ctk_list_store_move_after (fixture->store, &iter, &position);
  check_model (fixture, new_order, -1);
}

static void
list_store_test_move_after_end (ListStore     *fixture,
				gconstpointer  user_data G_GNUC_UNUSED)
{
  /* We move node 2 after 4 */
  int new_order[5] = { 0, 1, 3, 4, 2 };

  CtkTreeIter iter;
  CtkTreeIter position;

  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter, "2"));
  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &position, "4"));

  ctk_list_store_move_after (fixture->store, &iter, &position);
  check_model (fixture, new_order, -1);
}

static void
list_store_test_move_after_from_end (ListStore     *fixture,
				     gconstpointer  user_data G_GNUC_UNUSED)
{
  /* We move node 4 after 1 */
  int new_order[5] = { 0, 1, 4, 2, 3 };

  CtkTreeIter iter;
  CtkTreeIter position;

  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter, "4"));
  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &position, "1"));

  ctk_list_store_move_after (fixture->store, &iter, &position);
  check_model (fixture, new_order, -1);
}

static void
list_store_test_move_after_change_ends (ListStore     *fixture,
					gconstpointer  user_data G_GNUC_UNUSED)
{
  /* We move 0 after 4, this will cause both the head and tail ends to
   * change.
   */
  int new_order[5] = { 1, 2, 3, 4, 0 };

  CtkTreeIter iter;
  CtkTreeIter position;

  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter, "0"));
  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &position, "4"));

  ctk_list_store_move_after (fixture->store, &iter, &position);
  check_model (fixture, new_order, -1);
}

static void
list_store_test_move_after_NULL (ListStore     *fixture,
				 gconstpointer  user_data G_GNUC_UNUSED)
{
  /* We move node 2, NULL should prepend */
  int new_order[5] = { 2, 0, 1, 3, 4 };

  CtkTreeIter iter;

  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter, "2"));

  ctk_list_store_move_after (fixture->store, &iter, NULL);
  check_model (fixture, new_order, -1);
}

static void
list_store_test_move_after_single (void)
{
  CtkTreeIter iter;
  CtkTreeIter iter_copy;
  CtkListStore *store;

  store = ctk_list_store_new (1, G_TYPE_INT);

  /* Check if move-after on a store with a single node does not corrupt
   * the store.
   */

  ctk_list_store_append (store, &iter);
  iter_copy = iter;

  ctk_list_store_move_after (store, &iter, NULL);
  g_assert (iters_equal (&iter, &iter_copy));
  g_assert (ctk_tree_model_get_iter_first (CTK_TREE_MODEL (store), &iter));
  g_assert (iters_equal (&iter, &iter_copy));

  ctk_list_store_move_after (store, &iter, &iter);
  g_assert (iters_equal (&iter, &iter_copy));
  g_assert (ctk_tree_model_get_iter_first (CTK_TREE_MODEL (store), &iter));
  g_assert (iters_equal (&iter, &iter_copy));

  g_object_unref (store);
}

/* move before */

static void
list_store_test_move_before_next (ListStore     *fixture,
				  gconstpointer  user_data G_GNUC_UNUSED)
{
  /* We move node 3 before 2 */
  int new_order[5] = { 0, 1, 3, 2, 4 };

  CtkTreeIter iter;
  CtkTreeIter position;

  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter, "3"));
  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &position, "2"));

  ctk_list_store_move_before (fixture->store, &iter, &position);
  check_model (fixture, new_order, -1);
}

static void
list_store_test_move_before_apart (ListStore     *fixture,
				   gconstpointer  user_data G_GNUC_UNUSED)
{
  /* We move node 1 before 3 */
  int new_order[5] = { 0, 2, 1, 3, 4 };

  CtkTreeIter iter;
  CtkTreeIter position;

  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter, "1"));
  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &position, "3"));

  ctk_list_store_move_before (fixture->store, &iter, &position);
  check_model (fixture, new_order, -1);
}

static void
list_store_test_move_before_to_start (ListStore     *fixture,
				      gconstpointer  user_data G_GNUC_UNUSED)
{
  /* We move node 2 before 0 */
  int new_order[5] = { 2, 0, 1, 3, 4 };

  CtkTreeIter iter;
  CtkTreeIter position;

  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter, "2"));
  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &position, "0"));

  ctk_list_store_move_before (fixture->store, &iter, &position);
  check_model (fixture, new_order, -1);
}

static void
list_store_test_move_before_from_end (ListStore     *fixture,
			              gconstpointer  user_data G_GNUC_UNUSED)
{
  /* We move node 4 before 2 (replace end) */
  int new_order[5] = { 0, 1, 4, 2, 3 };

  CtkTreeIter iter;
  CtkTreeIter position;

  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter, "4"));
  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &position, "2"));

  ctk_list_store_move_before (fixture->store, &iter, &position);
  check_model (fixture, new_order, -1);
}

static void
list_store_test_move_before_change_ends (ListStore     *fixture,
				         gconstpointer  user_data G_GNUC_UNUSED)
{
  /* We move node 4 before 0 */
  int new_order[5] = { 4, 0, 1, 2, 3 };

  CtkTreeIter iter;
  CtkTreeIter position;

  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter, "4"));
  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &position, "0"));

  ctk_list_store_move_before (fixture->store, &iter, &position);
  check_model (fixture, new_order, -1);
}

static void
list_store_test_move_before_NULL (ListStore     *fixture,
			          gconstpointer  user_data G_GNUC_UNUSED)
{
  /* We move node 2, NULL should append */
  int new_order[5] = { 0, 1, 3, 4, 2 };

  CtkTreeIter iter;

  g_assert (ctk_tree_model_get_iter_from_string (CTK_TREE_MODEL (fixture->store), &iter, "2"));

  ctk_list_store_move_before (fixture->store, &iter, NULL);
  check_model (fixture, new_order, -1);
}

static void
list_store_test_move_before_single (void)
{
  CtkTreeIter iter;
  CtkTreeIter iter_copy;
  CtkListStore *store;

  store = ctk_list_store_new (1, G_TYPE_INT);

  /* Check if move-before on a store with a single node does not corrupt
   * the store.
   */

  ctk_list_store_append (store, &iter);
  iter_copy = iter;

  ctk_list_store_move_before (store, &iter, NULL);
  g_assert (iters_equal (&iter, &iter_copy));
  g_assert (ctk_tree_model_get_iter_first (CTK_TREE_MODEL (store), &iter));
  g_assert (iters_equal (&iter, &iter_copy));

  ctk_list_store_move_before (store, &iter, &iter);
  g_assert (iters_equal (&iter, &iter_copy));
  g_assert (ctk_tree_model_get_iter_first (CTK_TREE_MODEL (store), &iter));
  g_assert (iters_equal (&iter, &iter_copy));

  g_object_unref (store);
}


/* iter invalidation */

static void
list_store_test_iter_previous_invalid (ListStore     *fixture,
                                       gconstpointer  user_data G_GNUC_UNUSED)
{
  CtkTreeIter iter;

  ctk_tree_model_get_iter_first (CTK_TREE_MODEL (fixture->store), &iter);

  g_assert (ctk_tree_model_iter_previous (CTK_TREE_MODEL (fixture->store),
                                          &iter) == FALSE);
  g_assert (ctk_list_store_iter_is_valid (fixture->store, &iter) == FALSE);
  g_assert (iter.stamp == 0);
}

static void
list_store_test_iter_next_invalid (ListStore     *fixture,
                                   gconstpointer  user_data G_GNUC_UNUSED)
{
  CtkTreePath *path;
  CtkTreeIter iter;

  path = ctk_tree_path_new_from_indices (4, -1);
  ctk_tree_model_get_iter (CTK_TREE_MODEL (fixture->store), &iter, path);
  ctk_tree_path_free (path);

  g_assert (ctk_tree_model_iter_next (CTK_TREE_MODEL (fixture->store),
                                      &iter) == FALSE);
  g_assert (ctk_list_store_iter_is_valid (fixture->store, &iter) == FALSE);
  g_assert (iter.stamp == 0);
}

static void
list_store_test_iter_children_invalid (ListStore     *fixture,
                                       gconstpointer  user_data G_GNUC_UNUSED)
{
  CtkTreeIter iter, child;

  ctk_tree_model_get_iter_first (CTK_TREE_MODEL (fixture->store), &iter);
  g_assert (ctk_list_store_iter_is_valid (fixture->store, &iter) == TRUE);

  g_assert (ctk_tree_model_iter_children (CTK_TREE_MODEL (fixture->store),
                                          &child, &iter) == FALSE);
  g_assert (ctk_list_store_iter_is_valid (fixture->store, &child) == FALSE);
  g_assert (child.stamp == 0);
}

static void
list_store_test_iter_nth_child_invalid (ListStore     *fixture,
                                        gconstpointer  user_data G_GNUC_UNUSED)
{
  CtkTreeIter iter, child;

  ctk_tree_model_get_iter_first (CTK_TREE_MODEL (fixture->store), &iter);
  g_assert (ctk_list_store_iter_is_valid (fixture->store, &iter) == TRUE);

  g_assert (ctk_tree_model_iter_nth_child (CTK_TREE_MODEL (fixture->store),
                                           &child, &iter, 0) == FALSE);
  g_assert (ctk_list_store_iter_is_valid (fixture->store, &child) == FALSE);
  g_assert (child.stamp == 0);
}

static void
list_store_test_iter_parent_invalid (ListStore     *fixture,
                                     gconstpointer  user_data G_GNUC_UNUSED)
{
  CtkTreeIter iter, child;

  ctk_tree_model_get_iter_first (CTK_TREE_MODEL (fixture->store), &child);
  g_assert (ctk_list_store_iter_is_valid (fixture->store, &child) == TRUE);

  g_assert (ctk_tree_model_iter_parent (CTK_TREE_MODEL (fixture->store),
                                        &iter, &child) == FALSE);
  g_assert (ctk_list_store_iter_is_valid (fixture->store, &iter) == FALSE);
  g_assert (iter.stamp == 0);
}


/* main */

void
register_list_store_tests (void)
{
  /* insertion */
  g_test_add_func ("/ListStore/insert-high-values",
	           list_store_test_insert_high_values);
  g_test_add_func ("/ListStore/append",
		   list_store_test_append);
  g_test_add_func ("/ListStore/prepend",
		   list_store_test_prepend);
  g_test_add_func ("/ListStore/insert-after",
		   list_store_test_insert_after);
  g_test_add_func ("/ListStore/insert-after-NULL",
		   list_store_test_insert_after_NULL);
  g_test_add_func ("/ListStore/insert-before",
		   list_store_test_insert_before);
  g_test_add_func ("/ListStore/insert-before-NULL",
		   list_store_test_insert_before_NULL);

  /* setting values (FIXME) */
  g_test_add_func ("/ListStore/set-gvalue-to-transform",
                   list_store_set_gvalue_to_transform);

  /* removal */
  g_test_add ("/ListStore/remove-begin", ListStore, NULL,
	      list_store_setup, list_store_test_remove_begin,
	      list_store_teardown);
  g_test_add ("/ListStore/remove-middle", ListStore, NULL,
	      list_store_setup, list_store_test_remove_middle,
	      list_store_teardown);
  g_test_add ("/ListStore/remove-end", ListStore, NULL,
	      list_store_setup, list_store_test_remove_end,
	      list_store_teardown);

  g_test_add ("/ListStore/clear", ListStore, NULL,
	      list_store_setup, list_store_test_clear,
	      list_store_teardown);

  /* reordering */
  g_test_add ("/ListStore/reorder", ListStore, NULL,
	      list_store_setup, list_store_test_reorder,
	      list_store_teardown);

  /* swapping */
  g_test_add ("/ListStore/swap-begin", ListStore, NULL,
	      list_store_setup, list_store_test_swap_begin,
	      list_store_teardown);
  g_test_add ("/ListStore/swap-middle-next", ListStore, NULL,
	      list_store_setup, list_store_test_swap_middle_next,
	      list_store_teardown);
  g_test_add ("/ListStore/swap-middle-apart", ListStore, NULL,
	      list_store_setup, list_store_test_swap_middle_apart,
	      list_store_teardown);
  g_test_add ("/ListStore/swap-end", ListStore, NULL,
	      list_store_setup, list_store_test_swap_end,
	      list_store_teardown);
  g_test_add_func ("/ListStore/swap-single",
		   list_store_test_swap_single);

  /* moving */
  g_test_add ("/ListStore/move-after-from-start", ListStore, NULL,
	      list_store_setup, list_store_test_move_after_from_start,
	      list_store_teardown);
  g_test_add ("/ListStore/move-after-next", ListStore, NULL,
	      list_store_setup, list_store_test_move_after_next,
	      list_store_teardown);
  g_test_add ("/ListStore/move-after-apart", ListStore, NULL,
	      list_store_setup, list_store_test_move_after_apart,
	      list_store_teardown);
  g_test_add ("/ListStore/move-after-end", ListStore, NULL,
	      list_store_setup, list_store_test_move_after_end,
	      list_store_teardown);
  g_test_add ("/ListStore/move-after-from-end", ListStore, NULL,
	      list_store_setup, list_store_test_move_after_from_end,
	      list_store_teardown);
  g_test_add ("/ListStore/move-after-change-ends", ListStore, NULL,
	      list_store_setup, list_store_test_move_after_change_ends,
	      list_store_teardown);
  g_test_add ("/ListStore/move-after-NULL", ListStore, NULL,
	      list_store_setup, list_store_test_move_after_NULL,
	      list_store_teardown);
  g_test_add_func ("/ListStore/move-after-single",
		   list_store_test_move_after_single);

  g_test_add ("/ListStore/move-before-next", ListStore, NULL,
	      list_store_setup, list_store_test_move_before_next,
	      list_store_teardown);
  g_test_add ("/ListStore/move-before-apart", ListStore, NULL,
	      list_store_setup, list_store_test_move_before_apart,
	      list_store_teardown);
  g_test_add ("/ListStore/move-before-to-start", ListStore, NULL,
	      list_store_setup, list_store_test_move_before_to_start,
	      list_store_teardown);
  g_test_add ("/ListStore/move-before-from-end", ListStore, NULL,
	      list_store_setup, list_store_test_move_before_from_end,
	      list_store_teardown);
  g_test_add ("/ListStore/move-before-change-ends", ListStore, NULL,
	      list_store_setup, list_store_test_move_before_change_ends,
	      list_store_teardown);
  g_test_add ("/ListStore/move-before-NULL", ListStore, NULL,
	      list_store_setup, list_store_test_move_before_NULL,
	      list_store_teardown);
  g_test_add_func ("/ListStore/move-before-single",
		   list_store_test_move_before_single);

  /* iter invalidation */
  g_test_add ("/ListStore/iter-prev-invalid", ListStore, NULL,
              list_store_setup, list_store_test_iter_previous_invalid,
              list_store_teardown);
  g_test_add ("/ListStore/iter-next-invalid", ListStore, NULL,
              list_store_setup, list_store_test_iter_next_invalid,
              list_store_teardown);
  g_test_add ("/ListStore/iter-children-invalid", ListStore, NULL,
              list_store_setup, list_store_test_iter_children_invalid,
              list_store_teardown);
  g_test_add ("/ListStore/iter-nth-child-invalid", ListStore, NULL,
              list_store_setup, list_store_test_iter_nth_child_invalid,
              list_store_teardown);
  g_test_add ("/ListStore/iter-parent-invalid", ListStore, NULL,
              list_store_setup, list_store_test_iter_parent_invalid,
              list_store_teardown);
}
