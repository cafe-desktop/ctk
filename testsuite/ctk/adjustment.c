/* CTK - The GIMP Toolkit
 * Copyright (C) 2014 Red Hat, Inc.
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
test_basic (void)
{
  CtkAdjustment *a;

  a = ctk_adjustment_new (2.0, 0.0, 100.0, 1.0, 5.0, 10.0);
  
  g_assert_cmpfloat (ctk_adjustment_get_value (a), ==, 2.0);
  g_assert_cmpfloat (ctk_adjustment_get_lower (a), ==, 0.0);
  g_assert_cmpfloat (ctk_adjustment_get_upper (a), ==, 100.0);
  g_assert_cmpfloat (ctk_adjustment_get_step_increment (a), ==, 1.0);
  g_assert_cmpfloat (ctk_adjustment_get_page_increment (a), ==, 5.0);
  g_assert_cmpfloat (ctk_adjustment_get_page_size (a), ==, 10.0);
  g_assert_cmpfloat (ctk_adjustment_get_minimum_increment (a), ==, 1.0);

  ctk_adjustment_set_value (a, 50.0);
  ctk_adjustment_set_lower (a, 20.0);
  ctk_adjustment_set_upper (a, 75.5);
  ctk_adjustment_set_step_increment (a, 2.2);
  ctk_adjustment_set_page_increment (a, 1.5);
  ctk_adjustment_set_page_size (a, 10.0);

  g_assert_cmpfloat (ctk_adjustment_get_value (a), ==, 50.0);
  g_assert_cmpfloat (ctk_adjustment_get_lower (a), ==, 20.0);
  g_assert_cmpfloat (ctk_adjustment_get_upper (a), ==, 75.5);
  g_assert_cmpfloat (ctk_adjustment_get_step_increment (a), ==, 2.2);
  g_assert_cmpfloat (ctk_adjustment_get_page_increment (a), ==, 1.5);
  g_assert_cmpfloat (ctk_adjustment_get_page_size (a), ==, 10.0);
  g_assert_cmpfloat (ctk_adjustment_get_minimum_increment (a), ==, 1.5);

  g_object_unref (a);
}

static gint changed_count;
static gint value_changed_count;

static void
changed_cb (CtkAdjustment *a G_GNUC_UNUSED)
{
  changed_count++;
}

static void
value_changed_cb (CtkAdjustment *a G_GNUC_UNUSED)
{
  value_changed_count++;
}

static void
test_signals (void)
{
  CtkAdjustment *a;

  a = ctk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

  g_signal_connect (a, "changed", G_CALLBACK (changed_cb), NULL);
  g_signal_connect (a, "value-changed", G_CALLBACK (value_changed_cb), NULL);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  changed_count = value_changed_count = 0;
  ctk_adjustment_changed (a);
  g_assert_cmpint (changed_count, ==, 1);  
  g_assert_cmpint (value_changed_count, ==, 0);  

  changed_count = value_changed_count = 0;
  ctk_adjustment_value_changed (a);
  g_assert_cmpint (changed_count, ==, 0);  
  g_assert_cmpint (value_changed_count, ==, 1);  
G_GNUC_END_IGNORE_DEPRECATIONS

  changed_count = value_changed_count = 0;
  ctk_adjustment_configure (a, 0.0, 0.0, 100.0, 1.0, 5.0, 0.0);
  g_assert_cmpint (changed_count, ==, 1);  
  g_assert_cmpint (value_changed_count, ==, 0);  

  changed_count = value_changed_count = 0;
  ctk_adjustment_set_value (a, 50.0);
  ctk_adjustment_set_lower (a, 20.0);
  ctk_adjustment_set_upper (a, 75.5);
  ctk_adjustment_set_step_increment (a, 2.2);
  ctk_adjustment_set_page_increment (a, 1.5);
  ctk_adjustment_set_page_size (a, 10.0);
  g_assert_cmpint (changed_count, ==, 5);  
  g_assert_cmpint (value_changed_count, ==, 1);  

  g_object_unref (a); 
}

static void
test_clamp (void)
{
  CtkAdjustment *a;

  a = ctk_adjustment_new (2.0, 0.0, 100.0, 1.0, 5.0, 10.0);

  ctk_adjustment_set_value (a, -10.0);
  g_assert_cmpfloat (ctk_adjustment_get_value (a), ==, 0.0);

  ctk_adjustment_set_value (a, 200.0);
  g_assert_cmpfloat (ctk_adjustment_get_value (a), ==, 90.0);

  ctk_adjustment_set_value (a, 99.0);
  g_assert_cmpfloat (ctk_adjustment_get_value (a), ==, 90.0);

  ctk_adjustment_configure (a, 0.0, 0.0, 10.0, 1.0, 5.0, 20.0);
  
  ctk_adjustment_set_value (a, 5.0);
  g_assert_cmpfloat (ctk_adjustment_get_value (a), ==, 0.0);

  g_object_unref (a);
}

static void
test_clamp_page (void)
{
  CtkAdjustment *a;

  a = ctk_adjustment_new (20.0, 0.0, 100.0, 1.0, 5.0, 10.0);

  ctk_adjustment_clamp_page (a, 50.0, 55.0);
  g_assert_cmpfloat (ctk_adjustment_get_value (a), ==, 45.0);

  ctk_adjustment_clamp_page (a, 52.0, 58.0);
  g_assert_cmpfloat (ctk_adjustment_get_value (a), ==, 48.0);

  ctk_adjustment_clamp_page (a, 48.0, 50.0);
  g_assert_cmpfloat (ctk_adjustment_get_value (a), ==, 48.0);

  ctk_adjustment_clamp_page (a, 30.0, 50.0);
  g_assert_cmpfloat (ctk_adjustment_get_value (a), ==, 30.0);

  g_object_unref (a);
}

int
main (int   argc,
      char *argv[])
{
  ctk_test_init (&argc, &argv);

  g_test_add_func ("/adjustment/basic", test_basic);
  g_test_add_func ("/adjustment/signals", test_signals);
  g_test_add_func ("/adjustment/clamp", test_clamp);
  g_test_add_func ("/adjustment/clamp_page", test_clamp_page);

  return g_test_run();
}
