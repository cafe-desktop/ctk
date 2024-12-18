/* ctkkeyhash.c: Keymap aware matching of key bindings
 *
 * CTK - The GIMP Toolkit
 * Copyright (C) 2002, Red Hat Inc.
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

#include "config.h"

#include "ctkdebug.h"
#include "ctkkeyhash.h"
#include "ctkprivate.h"

typedef struct _CtkKeyHashEntry CtkKeyHashEntry;

struct _CtkKeyHashEntry
{
  guint keyval;
  CdkModifierType modifiers;
  gpointer value;

  /* Set as a side effect of generating key_hash->keycode_hash
   */
  CdkKeymapKey *keys;		
  gint n_keys;
};

struct _CtkKeyHash
{
  CdkKeymap *keymap;
  GHashTable *keycode_hash;
  GHashTable *reverse_hash;
  GList *entries_list;
  GDestroyNotify destroy_notify;
};

static void
key_hash_clear_keycode (gpointer key G_GNUC_UNUSED,
			gpointer value,
			gpointer data G_GNUC_UNUSED)
{
  GSList *keys = value;
  g_slist_free (keys);
}

static void
key_hash_insert_entry (CtkKeyHash      *key_hash,
		       CtkKeyHashEntry *entry)
{
  gint i;

  g_free (entry->keys);
  cdk_keymap_get_entries_for_keyval (key_hash->keymap,
				     entry->keyval,
				     &entry->keys, &entry->n_keys);
  
  for (i = 0; i < entry->n_keys; i++)
    {
      GSList *old_keys = g_hash_table_lookup (key_hash->keycode_hash,
					      GUINT_TO_POINTER (entry->keys[i].keycode));
      old_keys = g_slist_prepend (old_keys, entry);
      g_hash_table_insert (key_hash->keycode_hash,
			   GUINT_TO_POINTER (entry->keys[i].keycode),
			   old_keys);
    }
}

static GHashTable *
key_hash_get_keycode_hash (CtkKeyHash *key_hash)
{
  if (!key_hash->keycode_hash)
    {
      GList *tmp_list;
  
      key_hash->keycode_hash = g_hash_table_new (g_direct_hash, NULL);
      
      /* Preserve the original insertion order
       */
      for (tmp_list = g_list_last (key_hash->entries_list);
	   tmp_list;
	   tmp_list = tmp_list->prev)
	key_hash_insert_entry (key_hash, tmp_list->data);
    }

  return key_hash->keycode_hash;
}

static void
key_hash_keys_changed (CdkKeymap  *keymap G_GNUC_UNUSED,
		       CtkKeyHash *key_hash)
{
  /* The keymap changed, so we have to regenerate the keycode hash
   */
  if (key_hash->keycode_hash)
    {
      g_hash_table_foreach (key_hash->keycode_hash, key_hash_clear_keycode, NULL);
      g_hash_table_destroy (key_hash->keycode_hash);
      key_hash->keycode_hash = NULL;
    }
}

/**
 * _ctk_key_hash_new:
 * @keymap: a #CdkKeymap
 * @item_destroy_notify: function to be called when items are removed
 *   from the hash or %NULL.
 * 
 * Create a new key hash object for doing binding resolution. 
 * 
 * Returns: the newly created object. Free with _ctk_key_hash_free().
 **/
CtkKeyHash *
_ctk_key_hash_new (CdkKeymap      *keymap,
		   GDestroyNotify  item_destroy_notify)
{
  CtkKeyHash *key_hash = g_new (CtkKeyHash, 1);

  key_hash->keymap = keymap;
  g_signal_connect (keymap, "keys-changed",
		    G_CALLBACK (key_hash_keys_changed), key_hash);

  key_hash->entries_list = NULL;
  key_hash->keycode_hash = NULL;
  key_hash->reverse_hash = g_hash_table_new (g_direct_hash, NULL);
  key_hash->destroy_notify = item_destroy_notify;

  return key_hash;
}

static void
key_hash_free_entry (CtkKeyHash      *key_hash,
		     CtkKeyHashEntry *entry)
{
  if (key_hash->destroy_notify)
    (*key_hash->destroy_notify) (entry->value);
  
  g_free (entry->keys);
  g_slice_free (CtkKeyHashEntry, entry);
}

static void
key_hash_free_entry_foreach (gpointer value,
			     gpointer data)
{
  CtkKeyHashEntry *entry = value;
  CtkKeyHash *key_hash = data;

  key_hash_free_entry (key_hash, entry);
}

/**
 * ctk_key_hash_free:
 * @key_hash: a #CtkKeyHash
 * 
 * Destroys a key hash created with ctk_key_hash_new()
 **/
void
_ctk_key_hash_free (CtkKeyHash *key_hash)
{
  g_signal_handlers_disconnect_by_func (key_hash->keymap,
					key_hash_keys_changed,
					key_hash);

  if (key_hash->keycode_hash)
    {
      g_hash_table_foreach (key_hash->keycode_hash, key_hash_clear_keycode, NULL);
      g_hash_table_destroy (key_hash->keycode_hash);
    }
  
  g_hash_table_destroy (key_hash->reverse_hash);

  g_list_foreach (key_hash->entries_list, key_hash_free_entry_foreach, key_hash);
  g_list_free (key_hash->entries_list);
  
  g_free (key_hash);
}

/**
 * _ctk_key_hash_add_entry:
 * @key_hash: a #CtkKeyHash
 * @keyval: key symbol for this binding
 * @modifiers: modifiers for this binding
 * @value: value to insert in the key hash
 * 
 * Inserts a pair of key symbol and modifier mask into the key hash. 
 **/
void
_ctk_key_hash_add_entry (CtkKeyHash      *key_hash,
			 guint            keyval,
			 CdkModifierType  modifiers,
			 gpointer         value)
{
  CtkKeyHashEntry *entry = g_slice_new (CtkKeyHashEntry);

  entry->value = value;
  entry->keyval = keyval;
  entry->modifiers = modifiers;
  entry->keys = NULL;

  key_hash->entries_list = g_list_prepend (key_hash->entries_list, entry);
  g_hash_table_insert (key_hash->reverse_hash, value, key_hash->entries_list);

  if (key_hash->keycode_hash)
    key_hash_insert_entry (key_hash, entry);
}

/**
 * _ctk_key_hash_remove_entry:
 * @key_hash: a #CtkKeyHash
 * @value: value previously added with _ctk_key_hash_add_entry()
 * 
 * Removes a value previously added to the key hash with
 * _ctk_key_hash_add_entry().
 **/
void
_ctk_key_hash_remove_entry (CtkKeyHash *key_hash,
			    gpointer    value)
{
  GList *entry_node = g_hash_table_lookup (key_hash->reverse_hash, value);
  
  if (entry_node)
    {
      CtkKeyHashEntry *entry = entry_node->data;

      if (key_hash->keycode_hash)
	{
	  gint i;
	  
	  for (i = 0; i < entry->n_keys; i++)
	    {
	      GSList *old_keys = g_hash_table_lookup (key_hash->keycode_hash,
						      GUINT_TO_POINTER (entry->keys[i].keycode));
	      
	      GSList *new_keys = g_slist_remove (old_keys, entry);
	      if (new_keys != old_keys)
		{
		  if (new_keys)
		    g_hash_table_insert (key_hash->keycode_hash,
					 GUINT_TO_POINTER (entry->keys[i].keycode),
					 new_keys);
		  else
		    g_hash_table_remove (key_hash->keycode_hash,
					 GUINT_TO_POINTER (entry->keys[i].keycode));
		}
	    }
	}
	  
      g_hash_table_remove (key_hash->reverse_hash, entry_node);
      key_hash->entries_list = g_list_delete_link (key_hash->entries_list, entry_node);

      key_hash_free_entry (key_hash, entry);
    }
}

static gint
lookup_result_compare (gconstpointer a,
		       gconstpointer b)
{
  const CtkKeyHashEntry *entry_a = a;
  const CtkKeyHashEntry *entry_b = b;
  guint modifiers;

  gint n_bits_a = 0;
  gint n_bits_b = 0;

  modifiers = entry_a->modifiers;
  while (modifiers)
    {
      if (modifiers & 1)
	n_bits_a++;
      modifiers >>= 1;
    }

  modifiers = entry_b->modifiers;
  while (modifiers)
    {
      if (modifiers & 1)
	n_bits_b++;
      modifiers >>= 1;
    }

  return n_bits_a < n_bits_b ? -1 : (n_bits_a == n_bits_b ? 0 : 1);
  
}

/* Sort a list of results so that matches with less modifiers come
 * before matches with more modifiers
 */
static GSList *
sort_lookup_results (GSList *slist)
{
  return g_slist_sort (slist, lookup_result_compare);
}

static gint
lookup_result_compare_by_keyval (gconstpointer a,
		                 gconstpointer b)
{
  const CtkKeyHashEntry *entry_a = a;
  const CtkKeyHashEntry *entry_b = b;

  if (entry_a->keyval < entry_b->keyval)
	return -1;
  else if (entry_a->keyval > entry_b->keyval)
	return 1;
  else
	return 0;
}

static GSList *
sort_lookup_results_by_keyval (GSList *slist)
{
  return g_slist_sort (slist, lookup_result_compare_by_keyval);
}

/* Return true if keyval is defined in keyboard group
 */
static gboolean 
keyval_in_group (CdkKeymap  *keymap,
                 guint      keyval,
                 gint       group)
{                 
  CtkKeyHashEntry entry;
  gint i;

  cdk_keymap_get_entries_for_keyval (keymap,
				     keyval,
				     &entry.keys, &entry.n_keys);

  for (i = 0; i < entry.n_keys; i++)
    {
      if (entry.keys[i].group == group)
        {
          g_free (entry.keys);
          return TRUE;
        }
    }

  g_free (entry.keys);
  return FALSE;
}

/**
 * _ctk_key_hash_lookup:
 * @key_hash: a #CtkKeyHash
 * @hardware_keycode: hardware keycode field from a #CdkEventKey
 * @state: state field from a #CdkEventKey
 * @mask: mask of modifiers to consider when matching against the
 *        modifiers in entries.
 * @group: group field from a #CdkEventKey
 * 
 * Looks up the best matching entry or entries in the hash table for
 * a given event. The results are sorted so that entries with less
 * modifiers come before entries with more modifiers.
 * 
 * The matches returned by this function can be exact (i.e. keycode, level
 * and group all match) or fuzzy (i.e. keycode and level match, but group
 * does not). As long there are any exact matches, only exact matches
 * are returned. If there are no exact matches, fuzzy matches will be
 * returned, as long as they are not shadowing a possible exact match.
 * This means that fuzzy matches won’t be considered if their keyval is 
 * present in the current group.
 * 
 * Returns: A newly-allocated #GSList of matching entries.
 *     Free with g_slist_free() when no longer needed.
 */
GSList *
_ctk_key_hash_lookup (CtkKeyHash      *key_hash,
		      guint16          hardware_keycode,
		      CdkModifierType  state,
		      CdkModifierType  mask,
		      gint             group)
{
  GHashTable *keycode_hash = key_hash_get_keycode_hash (key_hash);
  GSList *keys = g_hash_table_lookup (keycode_hash, GUINT_TO_POINTER ((guint)hardware_keycode));
  GSList *results = NULL;
  GSList *l;
  gboolean have_exact = FALSE;
  guint keyval;
  gint effective_group;
  gint level;
  CdkModifierType modifiers;
  CdkModifierType consumed_modifiers;
  CdkModifierType shift_group_mask;
  gboolean group_mod_is_accel_mod = FALSE;
  const CdkModifierType xmods = CDK_MOD2_MASK|CDK_MOD3_MASK|CDK_MOD4_MASK|CDK_MOD5_MASK;
  const CdkModifierType vmods = CDK_SUPER_MASK|CDK_HYPER_MASK|CDK_META_MASK;

  /* We don't want Caps_Lock to affect keybinding lookups.
   */
  state &= ~CDK_LOCK_MASK;

  _ctk_translate_keyboard_accel_state (key_hash->keymap,
                                       hardware_keycode, state, mask, group,
                                       &keyval,
                                       &effective_group, &level, &consumed_modifiers);

  /* if the group-toggling modifier is part of the default accel mod
   * mask, and it is active, disable it for matching
   */
  shift_group_mask = cdk_keymap_get_modifier_mask (key_hash->keymap,
                                                   CDK_MODIFIER_INTENT_SHIFT_GROUP);
  if (mask & shift_group_mask)
    group_mod_is_accel_mod = TRUE;

  cdk_keymap_map_virtual_modifiers (key_hash->keymap, &mask);
  cdk_keymap_add_virtual_modifiers (key_hash->keymap, &state);

  CTK_NOTE (KEYBINDINGS,
	    g_message ("Looking up keycode = %u, modifiers = 0x%04x,\n"
		       "    keyval = %u, group = %d, level = %d, consumed_modifiers = 0x%04x",
		       hardware_keycode, state, keyval, effective_group, level, consumed_modifiers));

  if (keys)
    {
      GSList *tmp_list = keys;
      while (tmp_list)
	{
	  CtkKeyHashEntry *entry = tmp_list->data;

	  /* If the virtual Super, Hyper or Meta modifiers are present,
	   * they will also be mapped to some of the Mod2 - Mod5 modifiers,
	   * so we compare them twice, ignoring either set.
	   * We accept combinations involving virtual modifiers only if they
	   * are mapped to separate modifiers; i.e. if Super and Hyper are
	   * both mapped to Mod4, then pressing a key that is mapped to Mod4
	   * will not match a Super+Hyper entry.
	   */
          modifiers = entry->modifiers;
          if (cdk_keymap_map_virtual_modifiers (key_hash->keymap, &modifiers) &&
	      ((modifiers & ~consumed_modifiers & mask & ~vmods) == (state & ~consumed_modifiers & mask & ~vmods) ||
	       (modifiers & ~consumed_modifiers & mask & ~xmods) == (state & ~consumed_modifiers & mask & ~xmods)))
	    {
	      gint i;

	      if (keyval == entry->keyval && /* Exact match */
                  /* but also match for group if it is an accel mod, because
                   * otherwise we can get multiple exact matches, some being
                   * bogus */
                  (!group_mod_is_accel_mod ||
                   (state & shift_group_mask) == (entry->modifiers & shift_group_mask)))

		{
		  CTK_NOTE (KEYBINDINGS,
			    g_message ("  found exact match, keyval = %u, modifiers = 0x%04x",
				       entry->keyval, entry->modifiers));

		  if (!have_exact)
		    {
		      g_slist_free (results);
		      results = NULL;
		    }

		  have_exact = TRUE;
		  results = g_slist_prepend (results, entry);
		}

	      if (!have_exact)
		{
		  for (i = 0; i < entry->n_keys; i++)
		    {
                      if (entry->keys[i].keycode == hardware_keycode &&
                          entry->keys[i].level == level &&
                           /* Only match for group if it's an accel mod */
                          (!group_mod_is_accel_mod ||
                           entry->keys[i].group == effective_group))
			{
			  CTK_NOTE (KEYBINDINGS,
				    g_message ("  found group = %d, level = %d",
					       entry->keys[i].group, entry->keys[i].level));
			  results = g_slist_prepend (results, entry);
			  break;
			}
		    }
		}
	    }

	  tmp_list = tmp_list->next;
	}
    }

  if (!have_exact && results) 
    {
      /* If there are fuzzy matches, check that the current group doesn't also 
       * define these keyvals; if yes, discard results because a widget up in 
       * the stack may have an exact match and we don't want to 'steal' it.
       */
      guint oldkeyval = 0;
      CtkKeyHashEntry *keyhashentry;

      results = sort_lookup_results_by_keyval (results);
      for (l = results; l; l = l->next)
        {
          keyhashentry = l->data;
          if (l == results || oldkeyval != keyhashentry->keyval)
            {
              oldkeyval = keyhashentry->keyval;
              if (keyval_in_group (key_hash->keymap, oldkeyval, group))
                {
       	          g_slist_free (results);
       	          return NULL;
                }
            }
        }
    }
    
  results = sort_lookup_results (results);
  for (l = results; l; l = l->next)
    l->data = ((CtkKeyHashEntry *)l->data)->value;

  return results;
}

/**
 * _ctk_key_hash_lookup_keyval:
 * @key_hash: a #CtkKeyHash
 * @event: a #CtkEvent
 * 
 * Looks up the best matching entry or entries in the hash table for a
 * given keyval/modifiers pair. It’s better to use
 * _ctk_key_hash_lookup() if you have the original #CdkEventKey
 * available.  The results are sorted so that entries with less
 * modifiers come before entries with more modifiers.
 * 
 * Returns: A #GSList of all matching entries.
 **/
GSList *
_ctk_key_hash_lookup_keyval (CtkKeyHash     *key_hash,
			     guint           keyval,
			     CdkModifierType modifiers)
{
  CdkKeymapKey *keys;
  gint n_keys;
  GSList *results = NULL;
  GSList *l;

  if (!keyval)			/* Key without symbol */
    return NULL;

  /* Find some random keycode for this keyval
   */
  cdk_keymap_get_entries_for_keyval (key_hash->keymap, keyval,
				     &keys, &n_keys);

  if (n_keys)
    {
      GHashTable *keycode_hash = key_hash_get_keycode_hash (key_hash);
      GSList *entries = g_hash_table_lookup (keycode_hash, GUINT_TO_POINTER (keys[0].keycode));

      while (entries)
	{
	  CtkKeyHashEntry *entry = entries->data;

	  if (entry->keyval == keyval && entry->modifiers == modifiers)
	    results = g_slist_prepend (results, entry);

	  entries = entries->next;
	}
    }

  g_free (keys);
	  
  results = sort_lookup_results (results);
  for (l = results; l; l = l->next)
    l->data = ((CtkKeyHashEntry *)l->data)->value;

  return results;
}
