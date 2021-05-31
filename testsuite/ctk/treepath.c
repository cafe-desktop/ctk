/* CtkTrePath tests.
 *
 * Copyright (C) 2011, Red Hat, Inc.
 * Authors: Matthias Clasen <mclasen@redhat.com>
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
test_append (void)
{
  CtkTreePath *p;
  gint i;
  gint *indices;

  p = ctk_tree_path_new ();
  for (i = 0; i < 100; i++)
    {
      g_assert_cmpint (ctk_tree_path_get_depth (p), ==, i);
      ctk_tree_path_append_index (p, i);
    }

  indices = ctk_tree_path_get_indices (p);
  for (i = 0; i < 100; i++)
    g_assert_cmpint (indices[i], ==, i);

  ctk_tree_path_free (p);
}

static void
test_prepend (void)
{
  CtkTreePath *p;
  gint i;
  gint *indices;

  p = ctk_tree_path_new ();
  for (i = 0; i < 100; i++)
    {
      g_assert_cmpint (ctk_tree_path_get_depth (p), ==, i);
      ctk_tree_path_prepend_index (p, i);
    }

  indices = ctk_tree_path_get_indices (p);
  for (i = 0; i < 100; i++)
    g_assert_cmpint (indices[i], ==, 99 - i);

  ctk_tree_path_free (p);
}

static void
test_to_string (void)
{
  const gchar *str = "0:1:2:3:4:5:6:7:8:9:10";
  CtkTreePath *p;
  gint *indices;
  gchar *s;
  gint i;

  p = ctk_tree_path_new_from_string (str);
  indices = ctk_tree_path_get_indices (p);
  for (i = 0; i < 10; i++)
    g_assert_cmpint (indices[i], ==, i);
  s = ctk_tree_path_to_string (p);
  g_assert_cmpstr (s, ==, str);

  ctk_tree_path_free (p);
  g_free (s);
}

static void
test_from_indices (void)
{
  CtkTreePath *p;
  gint *indices;
  gint i;

  p = ctk_tree_path_new_from_indices (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1);
  g_assert_cmpint (ctk_tree_path_get_depth (p), ==, 10);
  indices = ctk_tree_path_get_indices (p);
  for (i = 0; i < 10; i++)
    g_assert_cmpint (indices[i], ==, i);
  ctk_tree_path_free (p);
}

static void
test_first (void)
{
  CtkTreePath *p;
  p = ctk_tree_path_new_first ();
  g_assert_cmpint (ctk_tree_path_get_depth (p), ==, 1);
  g_assert_cmpint (ctk_tree_path_get_indices (p)[0], ==, 0);
  ctk_tree_path_free (p);
}

static void
test_navigation (void)
{
  CtkTreePath *p;
  CtkTreePath *q;
  gint *pi;
  gint *qi;
  gint i;
  gboolean res;

  p = ctk_tree_path_new_from_indices (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1);
  q = ctk_tree_path_copy (p);
  g_assert (ctk_tree_path_compare (p, q) == 0);
  ctk_tree_path_next (q);
  pi = ctk_tree_path_get_indices (p);
  qi = ctk_tree_path_get_indices (q);
  for (i = 0; i < 9; i++)
    g_assert_cmpint (pi[i], ==, qi[i]);
  g_assert_cmpint (qi[9], ==, pi[9] + 1);

  g_assert (!ctk_tree_path_is_ancestor (p, q));
  g_assert (!ctk_tree_path_is_ancestor (q, p));
  g_assert (!ctk_tree_path_is_descendant (p, q));
  g_assert (!ctk_tree_path_is_descendant (q, p));

  res = ctk_tree_path_prev (q);
  g_assert (res);
  g_assert (ctk_tree_path_compare (p, q) == 0);

  g_assert (!ctk_tree_path_is_ancestor (p, q));
  g_assert (!ctk_tree_path_is_ancestor (q, p));
  g_assert (!ctk_tree_path_is_descendant (p, q));
  g_assert (!ctk_tree_path_is_descendant (q, p));

  ctk_tree_path_down (q);

  g_assert (ctk_tree_path_compare (p, q) < 0);

  g_assert (ctk_tree_path_is_ancestor (p, q));
  g_assert (!ctk_tree_path_is_ancestor (q, p));
  g_assert (!ctk_tree_path_is_descendant (p, q));
  g_assert (ctk_tree_path_is_descendant (q, p));

  res = ctk_tree_path_prev (q);
  g_assert (!res);

  res = ctk_tree_path_up (q);
  g_assert (res);
  g_assert (ctk_tree_path_compare (p, q) == 0);

  g_assert_cmpint (ctk_tree_path_get_depth (q), ==, 10);
  res = ctk_tree_path_up (q);
  g_assert (res);
  g_assert_cmpint (ctk_tree_path_get_depth (q), ==, 9);

  ctk_tree_path_free (p);
  ctk_tree_path_free (q);
}

int
main (int argc, char *argv[])
{
  ctk_test_init (&argc, &argv, NULL);

  g_test_add_func ("/tree-path/append", test_append);
  g_test_add_func ("/tree-path/prepend", test_prepend);
  g_test_add_func ("/tree-path/to-string", test_to_string);
  g_test_add_func ("/tree-path/from-indices", test_from_indices);
  g_test_add_func ("/tree-path/first", test_first);
  g_test_add_func ("/tree-path/navigation", test_navigation);

  return g_test_run ();
}
