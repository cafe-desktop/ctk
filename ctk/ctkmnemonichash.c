/* ctkmnemonichash.c: Sets of mnemonics with cycling
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

#include "ctkmnemonichash.h"

struct _CtkMnemnonicHash
{
  GHashTable *hash;
};


CtkMnemonicHash *
_ctk_mnemonic_hash_new (void)
{
  CtkMnemonicHash *mnemonic_hash = g_new (CtkMnemonicHash, 1);

  mnemonic_hash->hash = g_hash_table_new (g_direct_hash, NULL);

  return mnemonic_hash;
}

static void
mnemonic_hash_free_foreach (gpointer	key,
			    gpointer	value,
			    gpointer	user)
{
  guint keyval = GPOINTER_TO_UINT (key);
  GSList *targets = value;

  gchar *name = ctk_accelerator_name (keyval, 0);
      
  g_warning ("mnemonic \"%s\" wasn't removed for widget (%p)",
	     name, targets->data);
  g_free (name);
  
  g_slist_free (targets);
}

void
_ctk_mnemonic_hash_free (CtkMnemonicHash *mnemonic_hash)
{
  g_hash_table_foreach (mnemonic_hash->hash,
			mnemonic_hash_free_foreach,
			NULL);

  g_hash_table_destroy (mnemonic_hash->hash);
  g_free (mnemonic_hash);
}

void
_ctk_mnemonic_hash_add (CtkMnemonicHash *mnemonic_hash,
			guint            keyval,
			CtkWidget       *target)
{
  gpointer key = GUINT_TO_POINTER (keyval);
  GSList *targets, *new_targets;
  
  g_return_if_fail (CTK_IS_WIDGET (target));
  
  targets = g_hash_table_lookup (mnemonic_hash->hash, key);
  g_return_if_fail (g_slist_find (targets, target) == NULL);

  new_targets = g_slist_append (targets, target);
  if (new_targets != targets)
    g_hash_table_insert (mnemonic_hash->hash, key, new_targets);
}

void
_ctk_mnemonic_hash_remove (CtkMnemonicHash *mnemonic_hash,
			   guint           keyval,
			   CtkWidget      *target)
{
  gpointer key = GUINT_TO_POINTER (keyval);
  GSList *targets, *new_targets;
  
  g_return_if_fail (CTK_IS_WIDGET (target));
  
  targets = g_hash_table_lookup (mnemonic_hash->hash, key);

  g_return_if_fail (targets && g_slist_find (targets, target) != NULL);

  new_targets = g_slist_remove (targets, target);
  if (new_targets != targets)
    {
      if (new_targets == NULL)
	g_hash_table_remove (mnemonic_hash->hash, key);
      else
	g_hash_table_insert (mnemonic_hash->hash, key, new_targets);
    }
}

gboolean
_ctk_mnemonic_hash_activate (CtkMnemonicHash *mnemonic_hash,
			     guint            keyval)
{
  GSList *list, *targets;
  CtkWidget *widget, *chosen_widget;
  gboolean overloaded;

  targets = g_hash_table_lookup (mnemonic_hash->hash,
				 GUINT_TO_POINTER (keyval));
  if (!targets)
    return FALSE;
  
  overloaded = FALSE;
  chosen_widget = NULL;
  for (list = targets; list; list = list->next)
    {
      CdkWindow *window;

      widget = CTK_WIDGET (list->data);
      window = ctk_widget_get_window (widget);

      if (ctk_widget_is_sensitive (widget) &&
	  ctk_widget_get_mapped (widget) &&
          window && cdk_window_is_viewable (window))
	{
	  if (chosen_widget)
	    {
	      overloaded = TRUE;
	      break;
	    }
	  else
	    chosen_widget = widget;
	}
    }

  if (chosen_widget)
    {
      /* For round robin we put the activated entry on
       * the end of the list after activation
       */
      targets = g_slist_remove (targets, chosen_widget);
      targets = g_slist_append (targets, chosen_widget);
      g_hash_table_insert (mnemonic_hash->hash,
			   GUINT_TO_POINTER (keyval),
			   targets);

      return ctk_widget_mnemonic_activate (chosen_widget, overloaded);
    }
  return FALSE;
}

GSList *
_ctk_mnemonic_hash_lookup (CtkMnemonicHash *mnemonic_hash,
			   guint            keyval)
{
  return g_hash_table_lookup (mnemonic_hash->hash, GUINT_TO_POINTER (keyval));
}

static void
mnemonic_hash_foreach_func (gpointer key,
			    gpointer value,
			    gpointer data)
{
  struct {
    CtkMnemonicHashForeach func;
    gpointer func_data;
  } *info = data;

  guint keyval = GPOINTER_TO_UINT (key);
  GSList *targets = value;
  
  (*info->func) (keyval, targets, info->func_data);
}

void
_ctk_mnemonic_hash_foreach (CtkMnemonicHash       *mnemonic_hash,
			    CtkMnemonicHashForeach func,
			    gpointer               func_data)
{
  struct {
    CtkMnemonicHashForeach func;
    gpointer func_data;
  } info;
  
  info.func = func;
  info.func_data = func_data;

  g_hash_table_foreach (mnemonic_hash->hash,
			mnemonic_hash_foreach_func,
			&info);
}
