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

static void
test_parse (void)
{
  CtkPaperSize *p;

  p = ctk_paper_size_new (CTK_PAPER_NAME_A4);
  g_assert (p != NULL);
  g_assert_cmpint (ctk_paper_size_get_width (p, CTK_UNIT_MM), ==, 210);
  g_assert_cmpint (ctk_paper_size_get_height (p, CTK_UNIT_MM), ==, 297);
  g_assert_cmpstr (ctk_paper_size_get_name (p), ==, "iso_a4");
  g_assert_cmpstr (ctk_paper_size_get_display_name (p), ==, "A4");
  g_assert_cmpstr (ctk_paper_size_get_ppd_name (p), ==, "A4");
  g_assert (!ctk_paper_size_is_custom (p));
  ctk_paper_size_free (p);

  p = ctk_paper_size_new (CTK_PAPER_NAME_B5);
  g_assert (p != NULL);
  g_assert_cmpint (ctk_paper_size_get_width (p, CTK_UNIT_MM), ==, 176);
  g_assert_cmpint (ctk_paper_size_get_height (p, CTK_UNIT_MM), ==, 250);
  g_assert_cmpstr (ctk_paper_size_get_name (p), ==, "iso_b5");
  g_assert_cmpstr (ctk_paper_size_get_display_name (p), ==, "B5");
  g_assert_cmpstr (ctk_paper_size_get_ppd_name (p), ==, "ISOB5");
  g_assert (!ctk_paper_size_is_custom (p));
  ctk_paper_size_free (p);

  p = ctk_paper_size_new (CTK_PAPER_NAME_EXECUTIVE);
  g_assert (p != NULL);
  g_assert_cmpint (ctk_paper_size_get_width (p, CTK_UNIT_MM), ==, 184);
  g_assert_cmpint (ctk_paper_size_get_height (p, CTK_UNIT_MM), ==, 266);
  g_assert_cmpstr (ctk_paper_size_get_name (p), ==, "na_executive");
  g_assert_cmpstr (ctk_paper_size_get_display_name (p), ==, "Executive");
  g_assert_cmpstr (ctk_paper_size_get_ppd_name (p), ==, "Executive");
  g_assert (!ctk_paper_size_is_custom (p));
  ctk_paper_size_free (p);

  p = ctk_paper_size_new ("iso_a4_210x297mm");
  g_assert (p != NULL);
  g_assert_cmpint (ctk_paper_size_get_width (p, CTK_UNIT_MM), ==, 210);
  g_assert_cmpint (ctk_paper_size_get_height (p, CTK_UNIT_MM), ==, 297);
  g_assert_cmpstr (ctk_paper_size_get_name (p), ==, "iso_a4");
  g_assert_cmpstr (ctk_paper_size_get_display_name (p), ==, "A4");
  g_assert_cmpstr (ctk_paper_size_get_ppd_name (p), ==, "A4");
  g_assert (!ctk_paper_size_is_custom (p));
  ctk_paper_size_free (p);

  p = ctk_paper_size_new ("custom_w1_20x30in");
  g_assert (p != NULL);
  g_assert_cmpint (ctk_paper_size_get_width (p, CTK_UNIT_INCH), ==, 20);
  g_assert_cmpint (ctk_paper_size_get_height (p, CTK_UNIT_INCH), ==, 30);
  g_assert_cmpstr (ctk_paper_size_get_name (p), ==, "custom_w1");
  g_assert_cmpstr (ctk_paper_size_get_display_name (p), ==, "custom_w1");
  g_assert (ctk_paper_size_is_custom (p));
  ctk_paper_size_free (p);
}

static void
test_compare (void)
{
  CtkPaperSize *a1, *a2, *b, *c;

  a1 = ctk_paper_size_new (CTK_PAPER_NAME_A4);
  a2 = ctk_paper_size_new ("iso_a4_210x297mm");
  b = ctk_paper_size_new (CTK_PAPER_NAME_B5);
  c = ctk_paper_size_new ("custom_w1_20x30in");

  g_assert (ctk_paper_size_is_equal (a1, a2));
  g_assert (!ctk_paper_size_is_equal (a1, b));
  g_assert (!ctk_paper_size_is_equal (a1, c));
  g_assert (!ctk_paper_size_is_equal (b, c));

  ctk_paper_size_free (a1);
  ctk_paper_size_free (a2);
  ctk_paper_size_free (b);
  ctk_paper_size_free (c);
}

static void
test_units (void)
{
  CtkPaperSize *p;

  p = ctk_paper_size_new (CTK_PAPER_NAME_A4);

  g_assert_cmpint (ctk_paper_size_get_width (p, CTK_UNIT_MM), ==, 210);
  g_assert_cmpint (ctk_paper_size_get_height (p, CTK_UNIT_MM), ==, 297);

  /* compare up to 2 decimals */
  g_assert_cmpint (100 * ctk_paper_size_get_width (p, CTK_UNIT_INCH), ==, 100 * 8.26);
  g_assert_cmpint (100 * ctk_paper_size_get_height (p, CTK_UNIT_INCH), ==, 100 * 11.69);

  g_assert_cmpint (ctk_paper_size_get_width (p, CTK_UNIT_POINTS), ==, 595);
  g_assert_cmpint (ctk_paper_size_get_height (p, CTK_UNIT_POINTS), ==, 841);

  ctk_paper_size_free (p);
}

int
main (int argc, char *argv[])
{
  ctk_test_init (&argc, &argv);

  g_test_add_func ("/paper-size/parse", test_parse);
  g_test_add_func ("/paper-size/compare", test_compare);
  g_test_add_func ("/paper-size/units", test_units);

  return g_test_run();
}
