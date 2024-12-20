/* CTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * CtkBindingSet: Keybinding manager for GObjects.
 * Copyright (C) 1998 Tim Janik
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

/*
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#include "config.h"
#include <string.h>
#include <stdarg.h>

#include "ctkbindingsprivate.h"
#include "ctkkeyhash.h"
#include "ctkstylecontext.h"
#include "ctkwidget.h"
#include "ctkintl.h"

/**
 * SECTION:ctkbindings
 * @Title: Bindings
 * @Short_description: Key bindings for individual widgets
 * @See_also: Keyboard Accelerators, Mnemonics, #CtkCssProvider
 *
 * #CtkBindingSet provides a mechanism for configuring CTK+ key bindings
 * through CSS files. This eases key binding adjustments for application
 * developers as well as users and provides CTK+ users or administrators
 * with high key  binding configurability which requires no application
 * or toolkit side changes.
 *
 * In order for bindings to work in a custom widget implementation, the
 * widget’s #CtkWidget:can-focus and #CtkWidget:has-focus properties
 * must both be true. For example, by calling ctk_widget_set_can_focus()
 * in the widget’s initialisation function; and by calling
 * ctk_widget_grab_focus() when the widget is clicked.
 *
 * # Installing a key binding
 *
 * A CSS file binding consists of a “binding-set” definition and a match
 * statement to apply the binding set to specific widget types. Details
 * on the matching mechanism are described under
 * [Selectors][ctkcssprovider-selectors]
 * in the #CtkCssProvider documentation. Inside the binding set
 * definition, key combinations are bound to one or more specific
 * signal emissions on the target widget. Key combinations are strings
 * consisting of an optional #CdkModifierType name and
 * [key names][cdk3-Keyboard-Handling]
 * such as those defined in `cdk/cdkkeysyms.h`
 * or returned from cdk_keyval_name(), they have to be parsable by
 * ctk_accelerator_parse(). Specifications of signal emissions consist
 * of a string identifying the signal name, and a list of signal specific
 * arguments in parenthesis.
 *
 * For example for binding Control and the left or right cursor keys
 * of a #CtkEntry widget to the #CtkEntry::move-cursor signal (so
 * movement occurs in 3-character steps), the following binding can be
 * used:
 *
 * |[ <!-- language="CSS" -->
 * @binding-set MoveCursor3
 * {
 *   bind "<Control>Right" { "move-cursor" (visual-positions, 3, 0) };
 *   bind "<Control>Left" { "move-cursor" (visual-positions, -3, 0) };
 * }
 *
 * entry
 * {
 *   -ctk-key-bindings: MoveCursor3;
 * }
 * ]|
 *
 * # Unbinding existing key bindings
 *
 * CTK+ already defines a number of useful bindings for the widgets
 * it provides. Because custom bindings set up in CSS files take
 * precedence over the default bindings shipped with CTK+, overriding
 * existing bindings as demonstrated in
 * [Installing a key binding][ctk-bindings-install]
 * works as expected. The same mechanism can not be used to “unbind”
 * existing bindings, however.
 *
 * |[ <!-- language="CSS" -->
 * @binding-set MoveCursor3
 * {
 *   bind "<Control>Right" {  };
 *   bind "<Control>Left" {  };
 * }
 *
 * entry
 * {
 *   -ctk-key-bindings: MoveCursor3;
 * }
 * ]|
 *
 * The above example will not have the desired effect of causing
 * “<Control>Right” and “<Control>Left” key presses to be ignored by CTK+.
 * Instead, it just causes any existing bindings from the bindings set
 * “MoveCursor3” to be deleted, so when “<Control>Right” or
 * “<Control>Left” are pressed, no binding for these keys is found in
 * binding set “MoveCursor3”. CTK+ will thus continue to search for
 * matching key bindings, and will eventually lookup and find the default
 * CTK+ bindings for entries which implement word movement. To keep CTK+
 * from activating its default bindings, the “unbind” keyword can be used
 * like this:
 *
 * |[ <!-- language="CSS" -->
 * @binding-set MoveCursor3
 * {
 *   unbind "<Control>Right";
 *   unbind "<Control>Left";
 * }
 *
 * entry
 * {
 *   -ctk-key-bindings: MoveCursor3;
 * }
 * ]|
 *
 * Now, CTK+ will find a match when looking up “<Control>Right” and
 * “<Control>Left” key presses before it resorts to its default bindings,
 * and the match instructs it to abort (“unbind”) the search, so the key
 * presses are not consumed by this widget. As usual, further processing
 * of the key presses, e.g. by an entry’s parent widget, is now possible.
 */

/* --- defines --- */
#define BINDING_MOD_MASK() (ctk_accelerator_get_default_mod_mask () | CDK_RELEASE_MASK)


#define CTK_TYPE_IDENTIFIER (ctk_identifier_get_type ())
_CDK_EXTERN
GType ctk_identifier_get_type (void) G_GNUC_CONST;


/* --- structures --- */
typedef enum {
  CTK_BINDING_TOKEN_BIND,
  CTK_BINDING_TOKEN_UNBIND
} CtkBindingTokens;

/* --- variables --- */
static GHashTable       *binding_entry_hash_table = NULL;
static GSList           *binding_key_hashes = NULL;
static GSList           *binding_set_list = NULL;
static const gchar       key_class_binding_set[] = "ctk-class-binding-set";
static GQuark            key_id_class_binding_set = 0;


/* --- functions --- */
GType
ctk_identifier_get_type (void)
{
  static GType our_type = 0;

  if (our_type == 0)
    {
      GTypeInfo tinfo = { 0, };
      our_type = g_type_register_static (G_TYPE_STRING, I_("CtkIdentifier"), &tinfo, 0);
    }

  return our_type;
}

static CtkBindingSignal*
binding_signal_new (const gchar *signal_name,
                    guint        n_args)
{
  CtkBindingSignal *signal;

  signal = (CtkBindingSignal *) g_slice_alloc0 (sizeof (CtkBindingSignal) + n_args * sizeof (CtkBindingArg));
  signal->next = NULL;
  signal->signal_name = (gchar *)g_intern_string (signal_name);
  signal->n_args = n_args;
  signal->args = (CtkBindingArg *)(signal + 1);

  return signal;
}

static void
binding_signal_free (CtkBindingSignal *sig)
{
  guint i;

  for (i = 0; i < sig->n_args; i++)
    {
      if (G_TYPE_FUNDAMENTAL (sig->args[i].arg_type) == G_TYPE_STRING)
        g_free (sig->args[i].d.string_data);
    }
  g_slice_free1 (sizeof (CtkBindingSignal) + sig->n_args * sizeof (CtkBindingArg), sig);
}

static guint
binding_entry_hash (gconstpointer  key)
{
  register const CtkBindingEntry *e = key;
  register guint h;

  h = e->keyval;
  h ^= e->modifiers;

  return h;
}

static gint
binding_entries_compare (gconstpointer  a,
                         gconstpointer  b)
{
  register const CtkBindingEntry *ea = a;
  register const CtkBindingEntry *eb = b;

  return (ea->keyval == eb->keyval && ea->modifiers == eb->modifiers);
}

static void
binding_key_hash_insert_entry (CtkKeyHash      *key_hash,
                               CtkBindingEntry *entry)
{
  guint keyval = entry->keyval;

  /* We store lowercased accelerators. To deal with this, if <Shift>
   * was specified, uppercase.
   */
  if (entry->modifiers & CDK_SHIFT_MASK)
    {
      if (keyval == CDK_KEY_Tab)
        keyval = CDK_KEY_ISO_Left_Tab;
      else
        keyval = cdk_keyval_to_upper (keyval);
    }

  _ctk_key_hash_add_entry (key_hash, keyval, entry->modifiers & ~CDK_RELEASE_MASK, entry);
}

static void
binding_key_hash_destroy (gpointer data)
{
  CtkKeyHash *key_hash = data;

  binding_key_hashes = g_slist_remove (binding_key_hashes, key_hash);
  _ctk_key_hash_free (key_hash);
}

static void
insert_entries_into_key_hash (gpointer key G_GNUC_UNUSED,
                              gpointer value,
                              gpointer data)
{
  CtkKeyHash *key_hash = data;
  CtkBindingEntry *entry = value;

  for (; entry; entry = entry->hash_next)
    binding_key_hash_insert_entry (key_hash, entry);
}

static CtkKeyHash *
binding_key_hash_for_keymap (CdkKeymap *keymap)
{
  static GQuark key_hash_quark = 0;
  CtkKeyHash *key_hash;

  if (!key_hash_quark)
    key_hash_quark = g_quark_from_static_string ("ctk-binding-key-hash");

  key_hash = g_object_get_qdata (G_OBJECT (keymap), key_hash_quark);

  if (!key_hash)
    {
      key_hash = _ctk_key_hash_new (keymap, NULL);
      g_object_set_qdata_full (G_OBJECT (keymap), key_hash_quark, key_hash, binding_key_hash_destroy);

      if (binding_entry_hash_table)
        g_hash_table_foreach (binding_entry_hash_table,
                              insert_entries_into_key_hash,
                              key_hash);

      binding_key_hashes = g_slist_prepend (binding_key_hashes, key_hash);
    }

  return key_hash;
}


static CtkBindingEntry*
binding_entry_new (CtkBindingSet  *binding_set,
                   guint           keyval,
                   CdkModifierType modifiers)
{
  GSList *tmp_list;
  CtkBindingEntry *entry;

  if (!binding_entry_hash_table)
    binding_entry_hash_table = g_hash_table_new (binding_entry_hash, binding_entries_compare);

  entry = g_new (CtkBindingEntry, 1);
  entry->keyval = keyval;
  entry->modifiers = modifiers;
  entry->binding_set = binding_set,
  entry->destroyed = FALSE;
  entry->in_emission = FALSE;
  entry->marks_unbound = FALSE;
  entry->signals = NULL;

  entry->set_next = binding_set->entries;
  binding_set->entries = entry;

  entry->hash_next = g_hash_table_lookup (binding_entry_hash_table, entry);
  if (entry->hash_next)
    g_hash_table_remove (binding_entry_hash_table, entry->hash_next);
  g_hash_table_insert (binding_entry_hash_table, entry, entry);

  for (tmp_list = binding_key_hashes; tmp_list; tmp_list = tmp_list->next)
    {
      CtkKeyHash *key_hash = tmp_list->data;
      binding_key_hash_insert_entry (key_hash, entry);
    }

  return entry;
}

static void
binding_entry_free (CtkBindingEntry *entry)
{
  CtkBindingSignal *sig;

  g_assert (entry->set_next == NULL &&
            entry->hash_next == NULL &&
            entry->in_emission == FALSE &&
            entry->destroyed == TRUE);

  entry->destroyed = FALSE;

  sig = entry->signals;
  while (sig)
    {
      CtkBindingSignal *prev;

      prev = sig;
      sig = prev->next;
      binding_signal_free (prev);
    }
  g_free (entry);
}

static void
binding_entry_destroy (CtkBindingEntry *entry)
{
  CtkBindingEntry *o_entry;
  register CtkBindingEntry *tmp;
  CtkBindingEntry *begin;
  register CtkBindingEntry *last;
  GSList *tmp_list;

  /* unlink from binding set
   */
  last = NULL;
  tmp = entry->binding_set->entries;
  while (tmp)
    {
      if (tmp == entry)
        {
          if (last)
            last->set_next = entry->set_next;
          else
            entry->binding_set->entries = entry->set_next;
          break;
        }
      last = tmp;
      tmp = last->set_next;
    }
  entry->set_next = NULL;

  o_entry = g_hash_table_lookup (binding_entry_hash_table, entry);
  begin = o_entry;
  last = NULL;
  tmp = begin;
  while (tmp)
    {
      if (tmp == entry)
        {
          if (last)
            last->hash_next = entry->hash_next;
          else
            begin = entry->hash_next;
          break;
        }
      last = tmp;
      tmp = last->hash_next;
    }
  entry->hash_next = NULL;

  if (!begin)
    g_hash_table_remove (binding_entry_hash_table, entry);
  else if (begin != o_entry)
    {
      g_hash_table_remove (binding_entry_hash_table, entry);
      g_hash_table_insert (binding_entry_hash_table, begin, begin);
    }

  for (tmp_list = binding_key_hashes; tmp_list; tmp_list = tmp_list->next)
    {
      CtkKeyHash *key_hash = tmp_list->data;
      _ctk_key_hash_remove_entry (key_hash, entry);
    }

  entry->destroyed = TRUE;

  if (!entry->in_emission)
    binding_entry_free (entry);
}

static CtkBindingEntry*
binding_ht_lookup_entry (CtkBindingSet  *set,
                         guint           keyval,
                         CdkModifierType modifiers)
{
  CtkBindingEntry lookup_entry = { 0 };
  CtkBindingEntry *entry;

  if (!binding_entry_hash_table)
    return NULL;

  lookup_entry.keyval = keyval;
  lookup_entry.modifiers = modifiers;

  entry = g_hash_table_lookup (binding_entry_hash_table, &lookup_entry);
  for (; entry; entry = entry->hash_next)
    if (entry->binding_set == set)
      return entry;

  return NULL;
}

static gboolean
binding_compose_params (GObject         *object,
                        CtkBindingArg   *args,
                        GSignalQuery    *query,
                        GValue         **params_p)
{
  GValue *params;
  const GType *types;
  guint i;
  gboolean valid;

  params = g_new0 (GValue, query->n_params + 1);
  *params_p = params;

  /* The instance we emit on is the first object in the array
   */
  g_value_init (params, G_TYPE_OBJECT);
  g_value_set_object (params, G_OBJECT (object));
  params++;

  types = query->param_types;
  valid = TRUE;
  for (i = 1; i < query->n_params + 1 && valid; i++)
    {
      GValue tmp_value = G_VALUE_INIT;

      g_value_init (params, *types);

      switch (G_TYPE_FUNDAMENTAL (args->arg_type))
        {
        case G_TYPE_DOUBLE:
          g_value_init (&tmp_value, G_TYPE_DOUBLE);
          g_value_set_double (&tmp_value, args->d.double_data);
          break;
        case G_TYPE_LONG:
          g_value_init (&tmp_value, G_TYPE_LONG);
          g_value_set_long (&tmp_value, args->d.long_data);
          break;
        case G_TYPE_STRING:
          /* ctk_rc_parse_flags/enum() has fancier parsing for this; we can't call
           * that since we don't have a GParamSpec, so just do something simple
           */
          if (G_TYPE_FUNDAMENTAL (*types) == G_TYPE_ENUM)
            {
              GEnumClass *class = G_ENUM_CLASS (g_type_class_ref (*types));

              valid = FALSE;

              if (args->arg_type == CTK_TYPE_IDENTIFIER)
                {
                  GEnumValue *enum_value = NULL;
                  enum_value = g_enum_get_value_by_name (class, args->d.string_data);
                  if (!enum_value)
                    enum_value = g_enum_get_value_by_nick (class, args->d.string_data);
                  if (enum_value)
                    {
                      g_value_init (&tmp_value, *types);
                      g_value_set_enum (&tmp_value, enum_value->value);
                      valid = TRUE;
                    }
                }

              g_type_class_unref (class);
            }
          /* This is just a hack for compatibility with CTK+-1.2 where a string
           * could be used for a single flag value / without the support for multiple
           * values in ctk_rc_parse_flags(), this isn't very useful.
           */
          else if (G_TYPE_FUNDAMENTAL (*types) == G_TYPE_FLAGS)
            {
              GFlagsClass *class = G_FLAGS_CLASS (g_type_class_ref (*types));

              valid = FALSE;

              if (args->arg_type == CTK_TYPE_IDENTIFIER)
                {
                  GFlagsValue *flags_value = NULL;
                  flags_value = g_flags_get_value_by_name (class, args->d.string_data);
                  if (!flags_value)
                    flags_value = g_flags_get_value_by_nick (class, args->d.string_data);
                  if (flags_value)
                    {
                      g_value_init (&tmp_value, *types);
                      g_value_set_flags (&tmp_value, flags_value->value);
                      valid = TRUE;
                    }
                }

              g_type_class_unref (class);
            }
          else
            {
              g_value_init (&tmp_value, G_TYPE_STRING);
              g_value_set_static_string (&tmp_value, args->d.string_data);
            }
          break;
        default:
          valid = FALSE;
          break;
        }

      if (valid)
        {
          if (!g_value_transform (&tmp_value, params))
            valid = FALSE;

          g_value_unset (&tmp_value);
        }

      types++;
      params++;
      args++;
    }

  if (!valid)
    {
      guint j;

      for (j = 0; j < i; j++)
        g_value_unset (&(*params_p)[j]);

      g_free (*params_p);
      *params_p = NULL;
    }

  return valid;
}

static gboolean
ctk_binding_entry_activate (CtkBindingEntry *entry,
                            GObject         *object)
{
  CtkBindingSignal *sig;
  gboolean old_emission;
  gboolean handled = FALSE;
  gint i;

  old_emission = entry->in_emission;
  entry->in_emission = TRUE;

  g_object_ref (object);

  for (sig = entry->signals; sig; sig = sig->next)
    {
      GSignalQuery query;
      guint signal_id;
      GValue *params = NULL;
      GValue return_val = G_VALUE_INIT;
      gchar *accelerator = NULL;

      signal_id = g_signal_lookup (sig->signal_name, G_OBJECT_TYPE (object));
      if (!signal_id)
        {
          accelerator = ctk_accelerator_name (entry->keyval, entry->modifiers);
          g_warning ("ctk_binding_entry_activate(): binding \"%s::%s\": "
                     "could not find signal \"%s\" in the '%s' class ancestry",
                     entry->binding_set->set_name,
                     accelerator,
                     sig->signal_name,
                     g_type_name (G_OBJECT_TYPE (object)));
          g_free (accelerator);
          continue;
        }

      g_signal_query (signal_id, &query);
      if (query.n_params != sig->n_args ||
          (query.return_type != G_TYPE_NONE && query.return_type != G_TYPE_BOOLEAN) ||
          !binding_compose_params (object, sig->args, &query, &params))
        {
          accelerator = ctk_accelerator_name (entry->keyval, entry->modifiers);
          g_warning ("ctk_binding_entry_activate(): binding \"%s::%s\": "
                     "signature mismatch for signal \"%s\" in the '%s' class ancestry",
                     entry->binding_set->set_name,
                     accelerator,
                     sig->signal_name,
                     g_type_name (G_OBJECT_TYPE (object)));
        }
      else if (!(query.signal_flags & G_SIGNAL_ACTION))
        {
          accelerator = ctk_accelerator_name (entry->keyval, entry->modifiers);
          g_warning ("ctk_binding_entry_activate(): binding \"%s::%s\": "
                     "signal \"%s\" in the '%s' class ancestry cannot be used for action emissions",
                     entry->binding_set->set_name,
                     accelerator,
                     sig->signal_name,
                     g_type_name (G_OBJECT_TYPE (object)));
        }
      g_free (accelerator);
      if (accelerator)
        continue;

      if (query.return_type == G_TYPE_BOOLEAN)
        g_value_init (&return_val, G_TYPE_BOOLEAN);

      g_signal_emitv (params, signal_id, 0, &return_val);

      if (query.return_type == G_TYPE_BOOLEAN)
        {
          if (g_value_get_boolean (&return_val))
            handled = TRUE;
          g_value_unset (&return_val);
        }
      else
        handled = TRUE;

      if (params != NULL)
        {
          for (i = 0; i < query.n_params + 1; i++)
            g_value_unset (&params[i]);

          g_free (params);
        }

      if (entry->destroyed)
        break;
    }

  g_object_unref (object);

  entry->in_emission = old_emission;
  if (entry->destroyed && !entry->in_emission)
    binding_entry_free (entry);

  return handled;
}

/**
 * ctk_binding_set_new: (skip)
 * @set_name: unique name of this binding set
 *
 * CTK+ maintains a global list of binding sets. Each binding set has
 * a unique name which needs to be specified upon creation.
 *
 * Returns: (transfer none): new binding set
 */
CtkBindingSet*
ctk_binding_set_new (const gchar *set_name)
{
  CtkBindingSet *binding_set;

  g_return_val_if_fail (set_name != NULL, NULL);

  binding_set = g_new (CtkBindingSet, 1);
  binding_set->set_name = (gchar *) g_intern_string (set_name);
  binding_set->widget_path_pspecs = NULL;
  binding_set->widget_class_pspecs = NULL;
  binding_set->class_branch_pspecs = NULL;
  binding_set->entries = NULL;
  binding_set->current = NULL;
  binding_set->parsed = FALSE;

  binding_set_list = g_slist_prepend (binding_set_list, binding_set);

  return binding_set;
}

/**
 * ctk_binding_set_by_class: (skip)
 * @object_class: a valid #GObject class
 *
 * This function returns the binding set named after the type name of
 * the passed in class structure. New binding sets are created on
 * demand by this function.
 *
 * Returns: (transfer none): the binding set corresponding to
 *     @object_class
 */
CtkBindingSet*
ctk_binding_set_by_class (gpointer object_class)
{
  GObjectClass *class = object_class;
  CtkBindingSet* binding_set;

  g_return_val_if_fail (G_IS_OBJECT_CLASS (class), NULL);

  if (!key_id_class_binding_set)
    key_id_class_binding_set = g_quark_from_static_string (key_class_binding_set);

  binding_set = g_dataset_id_get_data (class, key_id_class_binding_set);

  if (binding_set)
    return binding_set;

  binding_set = ctk_binding_set_new (g_type_name (G_OBJECT_CLASS_TYPE (class)));
  g_dataset_id_set_data (class, key_id_class_binding_set, binding_set);

  return binding_set;
}

static CtkBindingSet*
ctk_binding_set_find_interned (const gchar *set_name)
{
  GSList *slist;

  for (slist = binding_set_list; slist; slist = slist->next)
    {
      CtkBindingSet *binding_set;

      binding_set = slist->data;
      if (binding_set->set_name == set_name)
        return binding_set;
    }

  return NULL;
}

/**
 * ctk_binding_set_find:
 * @set_name: unique binding set name
 *
 * Find a binding set by its globally unique name.
 *
 * The @set_name can either be a name used for ctk_binding_set_new()
 * or the type name of a class used in ctk_binding_set_by_class().
 *
 * Returns: (nullable) (transfer none): %NULL or the specified binding set
 */
CtkBindingSet*
ctk_binding_set_find (const gchar *set_name)
{
  g_return_val_if_fail (set_name != NULL, NULL);

  return ctk_binding_set_find_interned (g_intern_string (set_name));
}

/**
 * ctk_binding_set_activate:
 * @binding_set: a #CtkBindingSet set to activate
 * @keyval:      key value of the binding
 * @modifiers:   key modifier of the binding
 * @object:      object to activate when binding found
 *
 * Find a key binding matching @keyval and @modifiers within
 * @binding_set and activate the binding on @object.
 *
 * Returns: %TRUE if a binding was found and activated
 */
gboolean
ctk_binding_set_activate (CtkBindingSet  *binding_set,
                          guint           keyval,
                          CdkModifierType modifiers,
                          GObject        *object)
{
  CtkBindingEntry *entry;

  g_return_val_if_fail (binding_set != NULL, FALSE);
  g_return_val_if_fail (G_IS_OBJECT (object), FALSE);

  keyval = cdk_keyval_to_lower (keyval);
  modifiers = modifiers & BINDING_MOD_MASK ();

  entry = binding_ht_lookup_entry (binding_set, keyval, modifiers);
  if (entry)
    return ctk_binding_entry_activate (entry, object);

  return FALSE;
}

static void
ctk_binding_entry_clear_internal (CtkBindingSet  *binding_set,
                                  guint           keyval,
                                  CdkModifierType modifiers)
{
  CtkBindingEntry *entry;

  keyval = cdk_keyval_to_lower (keyval);
  modifiers = modifiers & BINDING_MOD_MASK ();

  entry = binding_ht_lookup_entry (binding_set, keyval, modifiers);
  if (entry)
    binding_entry_destroy (entry);

  entry = binding_entry_new (binding_set, keyval, modifiers);
}

/**
 * ctk_binding_entry_skip:
 * @binding_set: a #CtkBindingSet to skip an entry of
 * @keyval:      key value of binding to skip
 * @modifiers:   key modifier of binding to skip
 *
 * Install a binding on @binding_set which causes key lookups
 * to be aborted, to prevent bindings from lower priority sets
 * to be activated.
 *
 * Since: 2.12
 */
void
ctk_binding_entry_skip (CtkBindingSet  *binding_set,
                        guint           keyval,
                        CdkModifierType modifiers)
{
  CtkBindingEntry *entry;

  g_return_if_fail (binding_set != NULL);

  keyval = cdk_keyval_to_lower (keyval);
  modifiers = modifiers & BINDING_MOD_MASK ();

  entry = binding_ht_lookup_entry (binding_set, keyval, modifiers);
  if (entry)
    binding_entry_destroy (entry);

  entry = binding_entry_new (binding_set, keyval, modifiers);
  entry->marks_unbound = TRUE;
}

/**
 * ctk_binding_entry_remove:
 * @binding_set: a #CtkBindingSet to remove an entry of
 * @keyval:      key value of binding to remove
 * @modifiers:   key modifier of binding to remove
 *
 * Remove a binding previously installed via
 * ctk_binding_entry_add_signal() on @binding_set.
 */
void
ctk_binding_entry_remove (CtkBindingSet  *binding_set,
                          guint           keyval,
                          CdkModifierType modifiers)
{
  CtkBindingEntry *entry;

  g_return_if_fail (binding_set != NULL);

  keyval = cdk_keyval_to_lower (keyval);
  modifiers = modifiers & BINDING_MOD_MASK ();

  entry = binding_ht_lookup_entry (binding_set, keyval, modifiers);
  if (entry)
    binding_entry_destroy (entry);
}

/**
 * ctk_binding_entry_add_signall:
 * @binding_set:  a #CtkBindingSet to add a signal to
 * @keyval:       key value
 * @modifiers:    key modifier
 * @signal_name:  signal name to be bound
 * @binding_args: (transfer none) (element-type CtkBindingArg):
 *     list of #CtkBindingArg signal arguments
 *
 * Override or install a new key binding for @keyval with @modifiers on
 * @binding_set.
 */
void
ctk_binding_entry_add_signall (CtkBindingSet  *binding_set,
                               guint           keyval,
                               CdkModifierType modifiers,
                               const gchar    *signal_name,
                               GSList         *binding_args)
{
  _ctk_binding_entry_add_signall (binding_set,
                                  keyval, modifiers,
                                  signal_name, binding_args);
}

void
_ctk_binding_entry_add_signall (CtkBindingSet  *binding_set,
                                guint          keyval,
                                CdkModifierType modifiers,
                                const gchar    *signal_name,
                                GSList        *binding_args)
{
  CtkBindingEntry *entry;
  CtkBindingSignal *signal, **signal_p;
  GSList *slist;
  guint n = 0;
  CtkBindingArg *arg;

  g_return_if_fail (binding_set != NULL);
  g_return_if_fail (signal_name != NULL);

  keyval = cdk_keyval_to_lower (keyval);
  modifiers = modifiers & BINDING_MOD_MASK ();

  signal = binding_signal_new (signal_name, g_slist_length (binding_args));

  arg = signal->args;
  for (slist = binding_args; slist; slist = slist->next)
    {
      CtkBindingArg *tmp_arg;

      tmp_arg = slist->data;
      if (!tmp_arg)
        {
          g_warning ("ctk_binding_entry_add_signall(): arg[%u] is 'NULL'", n);
          binding_signal_free (signal);
          return;
        }
      switch (G_TYPE_FUNDAMENTAL (tmp_arg->arg_type))
        {
        case  G_TYPE_LONG:
          arg->arg_type = G_TYPE_LONG;
          arg->d.long_data = tmp_arg->d.long_data;
          break;
        case  G_TYPE_DOUBLE:
          arg->arg_type = G_TYPE_DOUBLE;
          arg->d.double_data = tmp_arg->d.double_data;
          break;
        case  G_TYPE_STRING:
          if (tmp_arg->arg_type != CTK_TYPE_IDENTIFIER)
            arg->arg_type = G_TYPE_STRING;
          else
            arg->arg_type = CTK_TYPE_IDENTIFIER;
          arg->d.string_data = g_strdup (tmp_arg->d.string_data);
          if (!arg->d.string_data)
            {
              g_warning ("ctk_binding_entry_add_signall(): value of 'string' arg[%u] is 'NULL'", n);
              binding_signal_free (signal);
              return;
            }
          break;
        default:
          g_warning ("ctk_binding_entry_add_signall(): unsupported type '%s' for arg[%u]",
                     g_type_name (arg->arg_type), n);
          binding_signal_free (signal);
          return;
        }
      arg++;
      n++;
    }

  entry = binding_ht_lookup_entry (binding_set, keyval, modifiers);
  if (!entry)
    {
      ctk_binding_entry_clear_internal (binding_set, keyval, modifiers);
      entry = binding_ht_lookup_entry (binding_set, keyval, modifiers);
    }
  signal_p = &entry->signals;
  while (*signal_p)
    signal_p = &(*signal_p)->next;
  *signal_p = signal;
}

/**
 * ctk_binding_entry_add_signal:
 * @binding_set: a #CtkBindingSet to install an entry for
 * @keyval:      key value of binding to install
 * @modifiers:   key modifier of binding to install
 * @signal_name: signal to execute upon activation
 * @n_args:      number of arguments to @signal_name
 * @...:         arguments to @signal_name
 *
 * Override or install a new key binding for @keyval with @modifiers on
 * @binding_set. When the binding is activated, @signal_name will be
 * emitted on the target widget, with @n_args @Varargs used as
 * arguments.
 *
 * Each argument to the signal must be passed as a pair of varargs: the
 * #GType of the argument, followed by the argument value (which must
 * be of the given type). There must be @n_args pairs in total.
 *
 * ## Adding a Key Binding
 *
 * |[<!-- language="C" -->
 * CtkBindingSet *binding_set;
 * CdkModifierType modmask = CDK_CONTROL_MASK;
 * int count = 1;
 * ctk_binding_entry_add_signal (binding_set,
 *                               CDK_KEY_space,
 *                               modmask,
 *                               "move-cursor", 2,
 *                               CTK_TYPE_MOVEMENT_STEP, CTK_MOVEMENT_PAGES,
 *                               G_TYPE_INT, count,
 *                               G_TYPE_BOOLEAN, FALSE);
 * ]|
 */
void
ctk_binding_entry_add_signal (CtkBindingSet  *binding_set,
                              guint           keyval,
                              CdkModifierType modifiers,
                              const gchar    *signal_name,
                              guint           n_args,
                              ...)
{
  GSList *slist, *free_slist;
  va_list args;
  guint i;

  g_return_if_fail (binding_set != NULL);
  g_return_if_fail (signal_name != NULL);

  va_start (args, n_args);
  slist = NULL;
  for (i = 0; i < n_args; i++)
    {
      CtkBindingArg *arg;

      arg = g_slice_new0 (CtkBindingArg);
      slist = g_slist_prepend (slist, arg);

      arg->arg_type = va_arg (args, GType);
      switch (G_TYPE_FUNDAMENTAL (arg->arg_type))
        {
        case G_TYPE_CHAR:
        case G_TYPE_UCHAR:
        case G_TYPE_INT:
        case G_TYPE_UINT:
        case G_TYPE_BOOLEAN:
        case G_TYPE_ENUM:
        case G_TYPE_FLAGS:
          arg->arg_type = G_TYPE_LONG;
          arg->d.long_data = va_arg (args, gint);
          break;
        case G_TYPE_LONG:
        case G_TYPE_ULONG:
          arg->arg_type = G_TYPE_LONG;
          arg->d.long_data = va_arg (args, glong);
          break;
        case G_TYPE_FLOAT:
        case G_TYPE_DOUBLE:
          arg->arg_type = G_TYPE_DOUBLE;
          arg->d.double_data = va_arg (args, gdouble);
          break;
        case G_TYPE_STRING:
          if (arg->arg_type != CTK_TYPE_IDENTIFIER)
            arg->arg_type = G_TYPE_STRING;
          arg->d.string_data = va_arg (args, gchar*);
          if (!arg->d.string_data)
            {
              g_warning ("ctk_binding_entry_add_signal(): type '%s' arg[%u] is 'NULL'",
                         g_type_name (arg->arg_type),
                         i);
              i += n_args + 1;
            }
          break;
        default:
          g_warning ("ctk_binding_entry_add_signal(): unsupported type '%s' for arg[%u]",
                     g_type_name (arg->arg_type), i);
          i += n_args + 1;
          break;
        }
    }
  va_end (args);

  if (i == n_args || i == 0)
    {
      slist = g_slist_reverse (slist);
      _ctk_binding_entry_add_signall (binding_set, keyval, modifiers, signal_name, slist);
    }

  free_slist = slist;
  while (slist)
    {
      g_slice_free (CtkBindingArg, slist->data);
      slist = slist->next;
    }
  g_slist_free (free_slist);
}

static guint
ctk_binding_parse_signal (GScanner       *scanner,
                          CtkBindingSet  *binding_set,
                          guint           keyval,
                          CdkModifierType modifiers)
{
  gchar *signal;
  guint expected_token = 0;
  GSList *args;
  GSList *slist;
  gboolean done;
  gboolean negate;
  gboolean need_arg;
  gboolean seen_comma;

  g_return_val_if_fail (scanner != NULL, G_TOKEN_ERROR);

  g_scanner_get_next_token (scanner);

  if (scanner->token != G_TOKEN_STRING)
    return G_TOKEN_STRING;

  g_scanner_peek_next_token (scanner);

  if (scanner->next_token != '(')
    {
      g_scanner_get_next_token (scanner);
      return '(';
    }

  signal = g_strdup (scanner->value.v_string);
  g_scanner_get_next_token (scanner);

  negate = FALSE;
  args = NULL;
  done = FALSE;
  need_arg = TRUE;
  seen_comma = FALSE;
  scanner->config->scan_symbols = FALSE;

  do
    {
      CtkBindingArg *arg;

      if (need_arg)
        expected_token = G_TOKEN_INT;
      else
        expected_token = ')';

      g_scanner_get_next_token (scanner);

      switch ((guint) scanner->token)
        {
        case G_TOKEN_FLOAT:
          if (need_arg)
            {
              need_arg = FALSE;
              arg = g_new (CtkBindingArg, 1);
              arg->arg_type = G_TYPE_DOUBLE;
              arg->d.double_data = scanner->value.v_float;

              if (negate)
                {
                  arg->d.double_data = - arg->d.double_data;
                  negate = FALSE;
                }
              args = g_slist_prepend (args, arg);
            }
          else
            done = TRUE;

          break;
        case G_TOKEN_INT:
          if (need_arg)
            {
              need_arg = FALSE;
              arg = g_new (CtkBindingArg, 1);
              arg->arg_type = G_TYPE_LONG;
              arg->d.long_data = scanner->value.v_int;

              if (negate)
                {
                  arg->d.long_data = - arg->d.long_data;
                  negate = FALSE;
                }
              args = g_slist_prepend (args, arg);
            }
          else
            done = TRUE;
          break;
        case G_TOKEN_STRING:
          if (need_arg && !negate)
            {
              need_arg = FALSE;
              arg = g_new (CtkBindingArg, 1);
              arg->arg_type = G_TYPE_STRING;
              arg->d.string_data = g_strdup (scanner->value.v_string);
              args = g_slist_prepend (args, arg);
            }
          else
            done = TRUE;

          break;
        case G_TOKEN_IDENTIFIER:
          if (need_arg && !negate)
            {
              need_arg = FALSE;
              arg = g_new (CtkBindingArg, 1);
              arg->arg_type = CTK_TYPE_IDENTIFIER;
              arg->d.string_data = g_strdup (scanner->value.v_identifier);
              args = g_slist_prepend (args, arg);
            }
          else
            done = TRUE;

          break;
        case '-':
          if (!need_arg)
            done = TRUE;
          else if (negate)
            {
              expected_token = G_TOKEN_INT;
              done = TRUE;
            }
          else
            negate = TRUE;

          break;
        case ',':
          seen_comma = TRUE;
          if (need_arg)
            done = TRUE;
          else
            need_arg = TRUE;

          break;
        case ')':
          if (!(need_arg && seen_comma) && !negate)
            {
              args = g_slist_reverse (args);
              _ctk_binding_entry_add_signall (binding_set,
                                              keyval,
                                              modifiers,
                                              signal,
                                              args);
              expected_token = G_TOKEN_NONE;
            }

          done = TRUE;
          break;
        default:
          done = TRUE;
          break;
        }
    }
  while (!done);

  scanner->config->scan_symbols = TRUE;

  for (slist = args; slist; slist = slist->next)
    {
      CtkBindingArg *arg;

      arg = slist->data;

      if (G_TYPE_FUNDAMENTAL (arg->arg_type) == G_TYPE_STRING)
        g_free (arg->d.string_data);
      g_free (arg);
    }

  g_slist_free (args);
  g_free (signal);

  return expected_token;
}

static inline guint
ctk_binding_parse_bind (GScanner       *scanner,
                        CtkBindingSet  *binding_set)
{
  guint keyval = 0;
  CdkModifierType modifiers = 0;
  gboolean unbind = FALSE;

  g_return_val_if_fail (scanner != NULL, G_TOKEN_ERROR);

  g_scanner_get_next_token (scanner);

  if (scanner->token != G_TOKEN_SYMBOL)
    return G_TOKEN_SYMBOL;

  if (scanner->value.v_symbol != GUINT_TO_POINTER (CTK_BINDING_TOKEN_BIND) &&
      scanner->value.v_symbol != GUINT_TO_POINTER (CTK_BINDING_TOKEN_UNBIND))
    return G_TOKEN_SYMBOL;

  unbind = (scanner->value.v_symbol == GUINT_TO_POINTER (CTK_BINDING_TOKEN_UNBIND));
  g_scanner_get_next_token (scanner);

  if (scanner->token != (guint) G_TOKEN_STRING)
    return G_TOKEN_STRING;

  ctk_accelerator_parse (scanner->value.v_string, &keyval, &modifiers);
  modifiers &= BINDING_MOD_MASK ();

  if (keyval == 0)
    return G_TOKEN_STRING;

  if (unbind)
    {
      ctk_binding_entry_skip (binding_set, keyval, modifiers);
      return G_TOKEN_NONE;
    }

  g_scanner_get_next_token (scanner);

  if (scanner->token != '{')
    return '{';

  ctk_binding_entry_clear_internal (binding_set, keyval, modifiers);
  g_scanner_peek_next_token (scanner);

  while (scanner->next_token != '}')
    {
      guint expected_token;

      switch (scanner->next_token)
        {
        case G_TOKEN_STRING:
          expected_token = ctk_binding_parse_signal (scanner,
                                                     binding_set,
                                                     keyval,
                                                     modifiers);
          if (expected_token != G_TOKEN_NONE)
            return expected_token;
          break;
        default:
          g_scanner_get_next_token (scanner);
          return '}';
        }

      g_scanner_peek_next_token (scanner);
    }

  g_scanner_get_next_token (scanner);

  return G_TOKEN_NONE;
}

static GScanner *
create_signal_scanner (void)
{
  GScanner *scanner;

  scanner = g_scanner_new (NULL);
  scanner->config->cset_identifier_nth = G_CSET_a_2_z G_CSET_A_2_Z G_CSET_DIGITS "-_";

  g_scanner_scope_add_symbol (scanner, 0, "bind", GUINT_TO_POINTER (CTK_BINDING_TOKEN_BIND));
  g_scanner_scope_add_symbol (scanner, 0, "unbind", GUINT_TO_POINTER (CTK_BINDING_TOKEN_UNBIND));

  g_scanner_set_scope (scanner, 0);

  return scanner;
}

/**
 * ctk_binding_entry_add_signal_from_string:
 * @binding_set: a #CtkBindingSet
 * @signal_desc: a signal description
 *
 * Parses a signal description from @signal_desc and incorporates
 * it into @binding_set.
 *
 * Signal descriptions may either bind a key combination to
 * one or more signals:
 * |[
 *   bind "key" {
 *     "signalname" (param, ...)
 *     ...
 *   }
 * ]|
 *
 * Or they may also unbind a key combination:
 * |[
 *   unbind "key"
 * ]|
 *
 * Key combinations must be in a format that can be parsed by
 * ctk_accelerator_parse().
 *
 * Returns: %G_TOKEN_NONE if the signal was successfully parsed and added,
 *     the expected token otherwise
 *
 * Since: 3.0
 */
GTokenType
ctk_binding_entry_add_signal_from_string (CtkBindingSet *binding_set,
                                          const gchar   *signal_desc)
{
  static GScanner *scanner = NULL;
  GTokenType ret;

  g_return_val_if_fail (binding_set != NULL, G_TOKEN_NONE);
  g_return_val_if_fail (signal_desc != NULL, G_TOKEN_NONE);

  if (G_UNLIKELY (!scanner))
    scanner = create_signal_scanner ();

  g_scanner_input_text (scanner, signal_desc,
                        (guint) strlen (signal_desc));

  ret = ctk_binding_parse_bind (scanner, binding_set);

  /* Reset for next use */
  g_scanner_set_scope (scanner, 0);

  return ret;
}

static gint
find_entry_with_binding (CtkBindingEntry *entry,
                         CtkBindingSet   *binding_set)
{
  return (entry->binding_set == binding_set) ? 0 : 1;
}

static gboolean
binding_activate (CtkBindingSet *binding_set,
                  GSList        *entries,
                  GObject       *object,
                  gboolean       is_release,
                  gboolean      *unbound)
{
  CtkBindingEntry *entry;
  GSList *elem;

  elem = g_slist_find_custom (entries, binding_set,
                              (GCompareFunc) find_entry_with_binding);

  if (!elem)
    return FALSE;

  entry = elem->data;

  if (is_release != ((entry->modifiers & CDK_RELEASE_MASK) != 0))
    return FALSE;

  if (entry->marks_unbound)
    {
      *unbound = TRUE;
      return FALSE;
    }

  if (ctk_binding_entry_activate (entry, object))
    return TRUE;

  return FALSE;
}

static gboolean
ctk_bindings_activate_list (GObject  *object,
                            GSList   *entries,
                            gboolean  is_release)
{
  CtkStyleContext *context;
  CtkBindingSet *binding_set;
  gboolean handled = FALSE;
  gboolean unbound = FALSE;
  GPtrArray *array;

  if (!entries)
    return FALSE;

  context = ctk_widget_get_style_context (CTK_WIDGET (object));

  ctk_style_context_get (context, ctk_style_context_get_state (context),
                         "-ctk-key-bindings", &array,
                         NULL);
  if (array)
    {
      gint i;

      for (i = 0; i < array->len; i++)
        {
          binding_set = g_ptr_array_index (array, i);
          handled = binding_activate (binding_set, entries,
                                      object, is_release,
                                      &unbound);
          if (handled || unbound)
            break;
        }

      g_ptr_array_unref (array);

      if (unbound)
        return FALSE;
    }

  if (!handled)
    {
      GType class_type;

      class_type = G_TYPE_FROM_INSTANCE (object);

      while (class_type && !handled)
        {
          binding_set = ctk_binding_set_find_interned (g_type_name (class_type));
          class_type = g_type_parent (class_type);

          if (!binding_set)
            continue;

          handled = binding_activate (binding_set, entries,
                                      object, is_release,
                                      &unbound);
          if (unbound)
            break;
        }

      if (unbound)
        return FALSE;
    }

  return handled;
}

/**
 * ctk_bindings_activate:
 * @object: object to activate when binding found
 * @keyval: key value of the binding
 * @modifiers: key modifier of the binding
 *
 * Find a key binding matching @keyval and @modifiers and activate the
 * binding on @object.
 *
 * Returns: %TRUE if a binding was found and activated
 */
gboolean
ctk_bindings_activate (GObject         *object,
                       guint            keyval,
                       CdkModifierType  modifiers)
{
  GSList *entries = NULL;
  CdkDisplay *display;
  CtkKeyHash *key_hash;
  gboolean handled = FALSE;
  gboolean is_release;

  if (!CTK_IS_WIDGET (object))
    return FALSE;

  is_release = (modifiers & CDK_RELEASE_MASK) != 0;
  modifiers = modifiers & BINDING_MOD_MASK () & ~CDK_RELEASE_MASK;

  display = ctk_widget_get_display (CTK_WIDGET (object));
  key_hash = binding_key_hash_for_keymap (cdk_keymap_get_for_display (display));

  entries = _ctk_key_hash_lookup_keyval (key_hash, keyval, modifiers);

  handled = ctk_bindings_activate_list (object, entries, is_release);

  g_slist_free (entries);

  return handled;
}

/**
 * ctk_bindings_activate_event:
 * @object: a #GObject (generally must be a widget)
 * @event: a #CdkEventKey
 *
 * Looks up key bindings for @object to find one matching
 * @event, and if one was found, activate it.
 *
 * Returns: %TRUE if a matching key binding was found
 *
 * Since: 2.4
 */
gboolean
ctk_bindings_activate_event (GObject     *object,
                             CdkEventKey *event)
{
  GSList *entries = NULL;
  CdkDisplay *display;
  CtkKeyHash *key_hash;
  gboolean handled = FALSE;

  if (!CTK_IS_WIDGET (object))
    return FALSE;

  display = ctk_widget_get_display (CTK_WIDGET (object));
  key_hash = binding_key_hash_for_keymap (cdk_keymap_get_for_display (display));

  entries = _ctk_key_hash_lookup (key_hash,
                                  event->hardware_keycode,
                                  event->state,
                                  BINDING_MOD_MASK () & ~CDK_RELEASE_MASK,
                                  event->group);

  handled = ctk_bindings_activate_list (object, entries,
                                        event->type == CDK_KEY_RELEASE);

  g_slist_free (entries);

  return handled;
}
