/* keyhash.c
 * Copyright (C) 2012 Red Hat, Inc12 Red Hat, Inc
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

#include <ctk/ctk.h>
#include <cdk/cdkkeysyms.h>
#include "../../ctk/ctkkeyhash.h"
#include "../../ctk/ctkprivate.h"

static gint count;

static void
counting_destroy (gpointer data)
{
  count++;
}

static void
test_basic (void)
{
  CtkKeyHash *hash;
  GSList *keys;

  count = 0;
  hash = _ctk_key_hash_new (cdk_keymap_get_default (), counting_destroy);

  keys = _ctk_key_hash_lookup (hash, 0, 0, 0, 0);
  g_assert (keys == NULL);

  _ctk_key_hash_add_entry (hash, 1, 0, NULL);
  _ctk_key_hash_add_entry (hash, 1, 1, NULL);
  _ctk_key_hash_add_entry (hash, 2, 0, NULL);
  _ctk_key_hash_add_entry (hash, 3, 0, NULL);
  _ctk_key_hash_add_entry (hash, 4, 0, NULL);

  _ctk_key_hash_free (hash);

  g_assert_cmpint (count, ==, 5);
}


#if 0
typedef struct
{
  guint           keyval;
  CdkModifierType modifiers;
} Entry;

static void
test_lookup (CtkKeyHash      *hash,
             guint            keyval,
             CdkModifierType  modifiers,
             CdkModifierType  mask,
             gint             n_results,
             ...)
{
  va_list ap;
  gint d;
  GSList *res, *l;
  gint i;
  CdkKeymapKey *keys;
  gint n_keys;

  cdk_keymap_get_entries_for_keyval (cdk_keymap_get_default (), keyval, &keys, &n_keys);
  if (n_keys == 0)
    return;

  res = _ctk_key_hash_lookup (hash, keys[0].keycode, modifiers, mask, keys[0].group);
  g_free (keys);

  g_assert_cmpint (g_slist_length (res), ==, n_results);

  va_start (ap, n_results);
  for (i = 0, l = res; i < n_results; i++, l = l->next)
    {
      d = va_arg (ap, int);
      g_assert_cmpint (d, ==, GPOINTER_TO_INT (l->data));
    }
  va_end (ap);

  g_slist_free (res);
}

static void
add_entries (CtkKeyHash *hash,
             Entry      *entries)
{
  gint i;

  for (i = 0; entries[i].keyval; i++)
    _ctk_key_hash_add_entry (hash, entries[i].keyval, entries[i].modifiers, GINT_TO_POINTER (i+1));
}

#define DEFAULT_MASK (CDK_CONTROL_MASK \
                      | CDK_SHIFT_MASK \
                      | CDK_MOD1_MASK  \
                      | CDK_SUPER_MASK \
                      | CDK_HYPER_MASK \
                      | CDK_META_MASK)

static void
test_match (void)
{
  CtkKeyHash *hash;
  static Entry entries[] = {
    { CDK_KEY_a, CDK_CONTROL_MASK },
    { CDK_KEY_a, CDK_CONTROL_MASK|CDK_SHIFT_MASK } ,
    { CDK_KEY_b, CDK_MOD1_MASK|CDK_CONTROL_MASK },
    { CDK_KEY_F10, 0 },
    {  0, 0 }
  };

  hash = _ctk_key_hash_new (cdk_keymap_get_default (), NULL);
  add_entries (hash, entries);

  test_lookup (hash, CDK_KEY_a, CDK_CONTROL_MASK, DEFAULT_MASK, 4, 1, 1, 2, 2);
  test_lookup (hash, CDK_KEY_A, CDK_CONTROL_MASK, DEFAULT_MASK, 4, 1, 1, 2, 2);
  test_lookup (hash, CDK_KEY_a, CDK_MOD1_MASK, DEFAULT_MASK, 0);
  test_lookup (hash, CDK_KEY_F10, 0, DEFAULT_MASK, 4, 4, 4, 4, 4);
  test_lookup (hash, CDK_KEY_F10, CDK_SHIFT_MASK, DEFAULT_MASK, 4, 4, 4, 4, 4);

  _ctk_key_hash_free (hash);
}

static gboolean
hyper_equals_super (void)
{
  CdkModifierType mods1, mods2;

  mods1 = CDK_HYPER_MASK;
  cdk_keymap_map_virtual_modifiers (cdk_keymap_get_default (), &mods1);
  mods1 = mods1 & ~CDK_HYPER_MASK;
  mods2 = CDK_SUPER_MASK;
  cdk_keymap_map_virtual_modifiers (cdk_keymap_get_default (), &mods2);
  mods2 = mods2 & ~CDK_SUPER_MASK;

  return mods1 == mods2;
}

static void
test_virtual (void)
{
  CtkKeyHash *hash;
  static Entry entries[] = {
    { CDK_KEY_a, CDK_SUPER_MASK },
    { CDK_KEY_b, CDK_HYPER_MASK } ,
    { CDK_KEY_c, CDK_META_MASK },
    { CDK_KEY_d, CDK_SUPER_MASK|CDK_HYPER_MASK },
    {  0, 0 }
  };

  hash = _ctk_key_hash_new (cdk_keymap_get_default (), NULL);
  add_entries (hash, entries);

  test_lookup (hash, CDK_KEY_a, CDK_SUPER_MASK, DEFAULT_MASK, 2, 1, 1);
  test_lookup (hash, CDK_KEY_a, CDK_HYPER_MASK, DEFAULT_MASK, 0);
  test_lookup (hash, CDK_KEY_b, CDK_HYPER_MASK, DEFAULT_MASK, 2, 2, 2);
  test_lookup (hash, CDK_KEY_c, CDK_META_MASK,  DEFAULT_MASK, 2, 3, 3);
  if (hyper_equals_super ())
    {
      CdkModifierType mods;

      /* test that colocated virtual modifiers don't count twice */
      test_lookup (hash, CDK_KEY_d, CDK_SUPER_MASK, DEFAULT_MASK, 0);
      test_lookup (hash, CDK_KEY_d, CDK_HYPER_MASK, DEFAULT_MASK, 0);

      mods = CDK_HYPER_MASK;
      cdk_keymap_map_virtual_modifiers (cdk_keymap_get_default (), &mods);
      test_lookup (hash, CDK_KEY_d, mods, DEFAULT_MASK, 0);
    }

  _ctk_key_hash_free (hash);
}
#endif

int
main (int argc, char **argv)
{
  /* initialize test program */
  ctk_test_init (&argc, &argv);

  g_test_add_func ("/keyhash/basic", test_basic);
#if 0
  /* FIXME: need to make these independent of xkb configuration */
  g_test_add_func ("/keyhash/match", test_match);
  g_test_add_func ("/keyhash/virtual", test_virtual);
#endif
  return g_test_run();
}
