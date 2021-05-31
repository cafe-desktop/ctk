/* typename.c: Test for CtkBuilder's type-name-mangling heuristics
 * Copyright (C) 2014 Red Hat, Inc.
 * Authors: Matthias Clasen
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

#include <glib.h>

/* Keep in sync with ctkbuilder.c ! */
static gchar *
type_name_mangle (const gchar *name)
{
  GString *symbol_name = g_string_new ("");
  gint i;

  for (i = 0; name[i] != '\0'; i++)
    {
      /* skip if uppercase, first or previous is uppercase */
      if ((name[i] == g_ascii_toupper (name[i]) &&
           i > 0 && name[i-1] != g_ascii_toupper (name[i-1])) ||
           (i > 2 && name[i]   == g_ascii_toupper (name[i]) &&
           name[i-1] == g_ascii_toupper (name[i-1]) &&
           name[i-2] == g_ascii_toupper (name[i-2])))
        g_string_append_c (symbol_name, '_');
      g_string_append_c (symbol_name, g_ascii_tolower (name[i]));
    }
  g_string_append (symbol_name, "_get_type");

  return g_string_free (symbol_name, FALSE);
}

static void
check (const gchar *TN, const gchar *gtf)
{
  gchar *symbol;

  symbol = type_name_mangle (TN);
  g_assert_cmpstr (symbol, ==, gtf);
  g_free (symbol);
}

static void test_CtkWindow (void)    { check ("CtkWindow", "ctk_window_get_type"); }
static void test_CtkHBox (void)      { check ("CtkHBox", "ctk_hbox_get_type"); }
static void test_CtkUIManager (void) { check ("CtkUIManager", "ctk_ui_manager_get_type"); }
static void test_CtkCList (void)     { check ("CtkCList", "ctk_clist_get_type"); }
static void test_CtkIMContext (void) { check ("CtkIMContext", "ctk_im_context_get_type"); }
static void test_Me2Shell (void)     { check ("Me2Shell", "me_2shell_get_type"); }
static void test_GWeather (void)     { check ("GWeatherLocation", "gweather_location_get_type"); }
 
int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);
  
  g_test_add_func ("/builder/get-type/CtkWindow",    test_CtkWindow);
  g_test_add_func ("/builder/get-type/CtkHBox",      test_CtkHBox);
  g_test_add_func ("/builder/get-type/CtkUIManager", test_CtkUIManager);
  g_test_add_func ("/builder/get-type/CtkCList",     test_CtkCList);
  g_test_add_func ("/builder/get-type/CtkIMContext", test_CtkIMContext);
  g_test_add_func ("/builder/get-type/Me2Shell",     test_Me2Shell);
  g_test_add_func ("/builder/get-type/GWeather",     test_GWeather);

  return g_test_run ();
}
