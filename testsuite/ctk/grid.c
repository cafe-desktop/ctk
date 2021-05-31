/* CTK - The GIMP Toolkit
 * Copyright (C) 2011 Red Hat, Inc.
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

/* test that attach_next_to picks the places
 * we expect it to pick, when there is any choice
 */
static void
test_attach (void)
{
  CtkGrid *g;
  CtkWidget *child, *sibling, *z, *A, *B;
  gint left, top, width, height;

  g = (CtkGrid *)ctk_grid_new ();

  child = ctk_label_new ("a");
  ctk_grid_attach_next_to (g, child, NULL, CTK_POS_LEFT, 1, 1);
  ctk_container_child_get (CTK_CONTAINER (g), child,
                           "left-attach", &left,
                           "top-attach", &top,
                           "width", &width,
                           "height", &height,
                           NULL);
  g_assert_cmpint (left,   ==, -1);
  g_assert_cmpint (top,    ==, 0);
  g_assert_cmpint (width,  ==, 1);
  g_assert_cmpint (height, ==, 1);

  sibling = child;
  child = ctk_label_new ("b");
  ctk_grid_attach_next_to (g, child, sibling, CTK_POS_RIGHT, 2, 2);
  ctk_container_child_get (CTK_CONTAINER (g), child,
                           "left-attach", &left,
                           "top-attach", &top,
                           "width", &width,
                           "height", &height,
                           NULL);
  g_assert_cmpint (left,   ==, 0);
  g_assert_cmpint (top,    ==, 0);
  g_assert_cmpint (width,  ==, 2);
  g_assert_cmpint (height, ==, 2);

  /* this one should just be ignored */
  z = ctk_label_new ("z");
  ctk_grid_attach (g, z, 4, 4, 1, 1);

  child = ctk_label_new ("c");
  ctk_grid_attach_next_to (g, child, sibling, CTK_POS_BOTTOM, 3, 1);
  ctk_container_child_get (CTK_CONTAINER (g), child,
                           "left-attach", &left,
                           "top-attach", &top,
                           "width", &width,
                           "height", &height,
                           NULL);
  g_assert_cmpint (left,   ==, -1);
  g_assert_cmpint (top,    ==, 1);
  g_assert_cmpint (width,  ==, 3);
  g_assert_cmpint (height, ==, 1);

  child = ctk_label_new ("u");
  ctk_grid_attach_next_to (g, child, z, CTK_POS_LEFT, 2, 1);
  ctk_container_child_get (CTK_CONTAINER (g), child,
                           "left-attach", &left,
                           "top-attach", &top,
                           "width", &width,
                           "height", &height,
                           NULL);
  g_assert_cmpint (left,   ==, 2);
  g_assert_cmpint (top,    ==, 4);
  g_assert_cmpint (width,  ==, 2);
  g_assert_cmpint (height, ==, 1);

  child = ctk_label_new ("v");
  ctk_grid_attach_next_to (g, child, z, CTK_POS_RIGHT, 2, 1);
  ctk_container_child_get (CTK_CONTAINER (g), child,
                           "left-attach", &left,
                           "top-attach", &top,
                           "width", &width,
                           "height", &height,
                           NULL);
  g_assert_cmpint (left,   ==, 5);
  g_assert_cmpint (top,    ==, 4);
  g_assert_cmpint (width,  ==, 2);
  g_assert_cmpint (height, ==, 1);

  child = ctk_label_new ("x");
  ctk_grid_attach_next_to (g, child, z, CTK_POS_TOP, 1, 2);
  ctk_container_child_get (CTK_CONTAINER (g), child,
                           "left-attach", &left,
                           "top-attach", &top,
                           "width", &width,
                           "height", &height,
                           NULL);
  g_assert_cmpint (left,   ==, 4);
  g_assert_cmpint (top,    ==, 2);
  g_assert_cmpint (width,  ==, 1);
  g_assert_cmpint (height, ==, 2);

  child = ctk_label_new ("x");
  ctk_grid_attach_next_to (g, child, z, CTK_POS_TOP, 1, 2);
  ctk_container_child_get (CTK_CONTAINER (g), child,
                           "left-attach", &left,
                           "top-attach", &top,
                           "width", &width,
                           "height", &height,
                           NULL);
  g_assert_cmpint (left,   ==, 4);
  g_assert_cmpint (top,    ==, 2);
  g_assert_cmpint (width,  ==, 1);
  g_assert_cmpint (height, ==, 2);

  child = ctk_label_new ("y");
  ctk_grid_attach_next_to (g, child, z, CTK_POS_BOTTOM, 1, 2);
  ctk_container_child_get (CTK_CONTAINER (g), child,
                           "left-attach", &left,
                           "top-attach", &top,
                           "width", &width,
                           "height", &height,
                           NULL);
  g_assert_cmpint (left,   ==, 4);
  g_assert_cmpint (top,    ==, 5);
  g_assert_cmpint (width,  ==, 1);
  g_assert_cmpint (height, ==, 2);

  A = ctk_label_new ("A");
  ctk_grid_attach (g, A, 10, 10, 1, 1);
  B = ctk_label_new ("B");
  ctk_grid_attach (g, B, 10, 12, 1, 1);

  child  = ctk_label_new ("D");
  ctk_grid_attach_next_to (g, child, A, CTK_POS_RIGHT, 1, 3);
  ctk_container_child_get (CTK_CONTAINER (g), child,
                           "left-attach", &left,
                           "top-attach", &top,
                           "width", &width,
                           "height", &height,
                           NULL);
  g_assert_cmpint (left,   ==, 11);
  g_assert_cmpint (top,    ==, 10);
  g_assert_cmpint (width,  ==,  1);
  g_assert_cmpint (height, ==,  3);
}

static void
test_add (void)
{
  CtkGrid *g;
  CtkWidget *child;
  gint left, top, width, height;

  g = (CtkGrid *)ctk_grid_new ();

  ctk_orientable_set_orientation (CTK_ORIENTABLE (g), CTK_ORIENTATION_HORIZONTAL);

  child = ctk_label_new ("a");
  ctk_container_add (CTK_CONTAINER (g), child);
  ctk_container_child_get (CTK_CONTAINER (g), child,
                           "left-attach", &left,
                           "top-attach", &top,
                           "width", &width,
                           "height", &height,
                           NULL);
  g_assert_cmpint (left,   ==, 0);
  g_assert_cmpint (top,    ==, 0);
  g_assert_cmpint (width,  ==, 1);
  g_assert_cmpint (height, ==, 1);

  child = ctk_label_new ("b");
  ctk_container_add (CTK_CONTAINER (g), child);
  ctk_container_child_get (CTK_CONTAINER (g), child,
                           "left-attach", &left,
                           "top-attach", &top,
                           "width", &width,
                           "height", &height,
                           NULL);
  g_assert_cmpint (left,   ==, 1);
  g_assert_cmpint (top,    ==, 0);
  g_assert_cmpint (width,  ==, 1);
  g_assert_cmpint (height, ==, 1);

  child = ctk_label_new ("c");
  ctk_container_add (CTK_CONTAINER (g), child);
  ctk_container_child_get (CTK_CONTAINER (g), child,
                           "left-attach", &left,
                           "top-attach", &top,
                           "width", &width,
                           "height", &height,
                           NULL);
  g_assert_cmpint (left,   ==, 2);
  g_assert_cmpint (top,    ==, 0);
  g_assert_cmpint (width,  ==, 1);
  g_assert_cmpint (height, ==, 1);

  ctk_orientable_set_orientation (CTK_ORIENTABLE (g), CTK_ORIENTATION_VERTICAL);

  child = ctk_label_new ("d");
  ctk_container_add (CTK_CONTAINER (g), child);
  ctk_container_child_get (CTK_CONTAINER (g), child,
                           "left-attach", &left,
                           "top-attach", &top,
                           "width", &width,
                           "height", &height,
                           NULL);
  g_assert_cmpint (left,   ==, 0);
  g_assert_cmpint (top,    ==, 1);
  g_assert_cmpint (width,  ==, 1);
  g_assert_cmpint (height, ==, 1);
}

int
main (int   argc,
      char *argv[])
{
  ctk_test_init (&argc, &argv);

  g_test_add_func ("/grid/attach", test_attach);
  g_test_add_func ("/grid/add", test_add);

  return g_test_run();
}
