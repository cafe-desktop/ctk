/* CTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

#include "ctktexttagtable.h"

#include "ctkbuildable.h"
#include "ctktexttagprivate.h"
#include "ctkmarshalers.h"
#include "ctktextbufferprivate.h" /* just for the lame notify_will_remove_tag hack */
#include "ctkintl.h"

#include <stdlib.h>


/**
 * SECTION:ctktexttagtable
 * @Short_description: Collection of tags that can be used together
 * @Title: CtkTextTagTable
 *
 * You may wish to begin by reading the
 * [text widget conceptual overview][TextWidget]
 * which gives an overview of all the objects and
 * data types related to the text widget and how they work together.
 *
 * # CtkTextTagTables as CtkBuildable
 *
 * The CtkTextTagTable implementation of the CtkBuildable interface
 * supports adding tags by specifying “tag” as the “type” attribute
 * of a <child> element.
 *
 * An example of a UI definition fragment specifying tags:
 * |[
 * <object class="CtkTextTagTable">
 *  <child type="tag">
 *    <object class="CtkTextTag"/>
 *  </child>
 * </object>
 * ]|
 */

struct _CtkTextTagTablePrivate
{
  GHashTable *hash;
  GSList     *anonymous;
  GSList     *buffers;

  gint anon_count;

  guint seen_invisible : 1;
};

enum {
  TAG_CHANGED,
  TAG_ADDED,
  TAG_REMOVED,
  LAST_SIGNAL
};

static void ctk_text_tag_table_finalize                 (GObject             *object);

static void ctk_text_tag_table_buildable_interface_init (CtkBuildableIface   *iface);
static void ctk_text_tag_table_buildable_add_child      (CtkBuildable        *buildable,
							 CtkBuilder          *builder,
							 GObject             *child,
							 const gchar         *type);

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_CODE (CtkTextTagTable, ctk_text_tag_table, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (CtkTextTagTable)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_BUILDABLE,
                                                ctk_text_tag_table_buildable_interface_init))

static void
ctk_text_tag_table_class_init (CtkTextTagTableClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = ctk_text_tag_table_finalize;

  /**
   * CtkTextTagTable::tag-changed:
   * @texttagtable: the object which received the signal.
   * @tag: the changed tag.
   * @size_changed: whether the change affects the #CtkTextView layout.
   */
  signals[TAG_CHANGED] =
    g_signal_new (I_("tag-changed"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkTextTagTableClass, tag_changed),
                  NULL, NULL,
                  _ctk_marshal_VOID__OBJECT_BOOLEAN,
                  G_TYPE_NONE,
                  2,
                  CTK_TYPE_TEXT_TAG,
                  G_TYPE_BOOLEAN);  
  g_signal_set_va_marshaller (signals[TAG_CHANGED],
                              G_OBJECT_CLASS_TYPE (object_class),
                              _ctk_marshal_VOID__OBJECT_BOOLEANv);

  /**
   * CtkTextTagTable::tag-added:
   * @texttagtable: the object which received the signal.
   * @tag: the added tag.
   */
  signals[TAG_ADDED] =
    g_signal_new (I_("tag-added"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkTextTagTableClass, tag_added),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  1,
                  CTK_TYPE_TEXT_TAG);

  /**
   * CtkTextTagTable::tag-removed:
   * @texttagtable: the object which received the signal.
   * @tag: the removed tag.
   */
  signals[TAG_REMOVED] =
    g_signal_new (I_("tag-removed"),  
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkTextTagTableClass, tag_removed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  1,
                  CTK_TYPE_TEXT_TAG);
}

static void
ctk_text_tag_table_init (CtkTextTagTable *table)
{
  table->priv = ctk_text_tag_table_get_instance_private (table);
  table->priv->hash = g_hash_table_new (g_str_hash, g_str_equal);
}

static void
check_visible (CtkTextTagTable *table,
               CtkTextTag      *tag)
{
  if (table->priv->seen_invisible)
    return;

  if (tag->priv->invisible_set)
    {
      gboolean invisible;

      g_object_get (tag, "invisible", &invisible, NULL);
      table->priv->seen_invisible = invisible;
    }
}

/**
 * ctk_text_tag_table_new:
 * 
 * Creates a new #CtkTextTagTable. The table contains no tags by
 * default.
 * 
 * Returns: a new #CtkTextTagTable
 **/
CtkTextTagTable*
ctk_text_tag_table_new (void)
{
  CtkTextTagTable *table;

  table = g_object_new (CTK_TYPE_TEXT_TAG_TABLE, NULL);

  return table;
}

static void
foreach_unref (CtkTextTag *tag,
               gpointer    data G_GNUC_UNUSED)
{
  CtkTextTagTable *table = CTK_TEXT_TAG_TABLE (tag->priv->table);
  CtkTextTagTablePrivate *priv = table->priv;
  GSList *l;

  /* We don't want to emit the remove signal here; so we just unparent
   * and unref the tag.
   */

  for (l = priv->buffers; l != NULL; l = l->next)
    _ctk_text_buffer_notify_will_remove_tag (CTK_TEXT_BUFFER (l->data),
                                             tag);

  tag->priv->table = NULL;
  g_object_unref (tag);
}

static void
ctk_text_tag_table_finalize (GObject *object)
{
  CtkTextTagTable *table = CTK_TEXT_TAG_TABLE (object);
  CtkTextTagTablePrivate *priv = table->priv;

  ctk_text_tag_table_foreach (table, foreach_unref, NULL);

  g_hash_table_destroy (priv->hash);
  g_slist_free (priv->anonymous);
  g_slist_free (priv->buffers);

  G_OBJECT_CLASS (ctk_text_tag_table_parent_class)->finalize (object);
}

static void
ctk_text_tag_table_buildable_interface_init (CtkBuildableIface   *iface)
{
  iface->add_child = ctk_text_tag_table_buildable_add_child;
}

static void
ctk_text_tag_table_buildable_add_child (CtkBuildable *buildable,
					CtkBuilder   *builder G_GNUC_UNUSED,
					GObject      *child,
					const gchar  *type)
{
  if (type && strcmp (type, "tag") == 0)
    ctk_text_tag_table_add (CTK_TEXT_TAG_TABLE (buildable),
			    CTK_TEXT_TAG (child));
}

/**
 * ctk_text_tag_table_add:
 * @table: a #CtkTextTagTable
 * @tag: a #CtkTextTag
 *
 * Add a tag to the table. The tag is assigned the highest priority
 * in the table.
 *
 * @tag must not be in a tag table already, and may not have
 * the same name as an already-added tag.
 *
 * Returns: %TRUE on success.
 **/
gboolean
ctk_text_tag_table_add (CtkTextTagTable *table,
                        CtkTextTag      *tag)
{
  CtkTextTagTablePrivate *priv;
  guint size;

  g_return_val_if_fail (CTK_IS_TEXT_TAG_TABLE (table), FALSE);
  g_return_val_if_fail (CTK_IS_TEXT_TAG (tag), FALSE);
  g_return_val_if_fail (tag->priv->table == NULL, FALSE);

  priv = table->priv;

  if (tag->priv->name && g_hash_table_lookup (priv->hash, tag->priv->name))
    {
      g_warning ("A tag named '%s' is already in the tag table.",
                 tag->priv->name);
      return FALSE;
    }
  
  g_object_ref (tag);

  if (tag->priv->name)
    g_hash_table_insert (priv->hash, tag->priv->name, tag);
  else
    {
      priv->anonymous = g_slist_prepend (priv->anonymous, tag);
      priv->anon_count++;
    }

  tag->priv->table = table;

  /* We get the highest tag priority, as the most-recently-added
     tag. Note that we do NOT use ctk_text_tag_set_priority,
     as it assumes the tag is already in the table. */
  size = ctk_text_tag_table_get_size (table);
  g_assert (size > 0);
  tag->priv->priority = size - 1;

  check_visible (table, tag);

  g_signal_emit (table, signals[TAG_ADDED], 0, tag);
  return TRUE;
}

/**
 * ctk_text_tag_table_lookup:
 * @table: a #CtkTextTagTable 
 * @name: name of a tag
 * 
 * Look up a named tag.
 * 
 * Returns: (nullable) (transfer none): The tag, or %NULL if none by that
 * name is in the table.
 **/
CtkTextTag*
ctk_text_tag_table_lookup (CtkTextTagTable *table,
                           const gchar     *name)
{
  CtkTextTagTablePrivate *priv;

  g_return_val_if_fail (CTK_IS_TEXT_TAG_TABLE (table), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  priv = table->priv;

  return g_hash_table_lookup (priv->hash, name);
}

/**
 * ctk_text_tag_table_remove:
 * @table: a #CtkTextTagTable
 * @tag: a #CtkTextTag
 *
 * Remove a tag from the table. If a #CtkTextBuffer has @table as its tag table,
 * the tag is removed from the buffer. The table’s reference to the tag is
 * removed, so the tag will end up destroyed if you don’t have a reference to
 * it.
 **/
void
ctk_text_tag_table_remove (CtkTextTagTable *table,
                           CtkTextTag      *tag)
{
  CtkTextTagTablePrivate *priv;
  GSList *l;

  g_return_if_fail (CTK_IS_TEXT_TAG_TABLE (table));
  g_return_if_fail (CTK_IS_TEXT_TAG (tag));
  g_return_if_fail (tag->priv->table == table);

  priv = table->priv;

  /* Our little bad hack to be sure buffers don't still have the tag
   * applied to text in the buffer
   */
  for (l = priv->buffers; l != NULL; l = l->next)
    _ctk_text_buffer_notify_will_remove_tag (CTK_TEXT_BUFFER (l->data),
                                             tag);

  /* Set ourselves to the highest priority; this means
     when we're removed, there won't be any gaps in the
     priorities of the tags in the table. */
  ctk_text_tag_set_priority (tag, ctk_text_tag_table_get_size (table) - 1);

  tag->priv->table = NULL;

  if (tag->priv->name)
    g_hash_table_remove (priv->hash, tag->priv->name);
  else
    {
      priv->anonymous = g_slist_remove (priv->anonymous, tag);
      priv->anon_count--;
    }

  g_signal_emit (table, signals[TAG_REMOVED], 0, tag);

  g_object_unref (tag);
}

struct ForeachData
{
  CtkTextTagTableForeach func;
  gpointer data;
};

static void
hash_foreach (gpointer key G_GNUC_UNUSED,
              gpointer value,
              gpointer data)
{
  struct ForeachData *fd = data;

  g_return_if_fail (CTK_IS_TEXT_TAG (value));

  (* fd->func) (value, fd->data);
}

static void
list_foreach (gpointer data, gpointer user_data)
{
  struct ForeachData *fd = user_data;

  g_return_if_fail (CTK_IS_TEXT_TAG (data));

  (* fd->func) (data, fd->data);
}

/**
 * ctk_text_tag_table_foreach:
 * @table: a #CtkTextTagTable
 * @func: (scope call): a function to call on each tag
 * @data: user data
 *
 * Calls @func on each tag in @table, with user data @data.
 * Note that the table may not be modified while iterating 
 * over it (you can’t add/remove tags).
 **/
void
ctk_text_tag_table_foreach (CtkTextTagTable       *table,
                            CtkTextTagTableForeach func,
                            gpointer               data)
{
  CtkTextTagTablePrivate *priv;
  struct ForeachData d;

  g_return_if_fail (CTK_IS_TEXT_TAG_TABLE (table));
  g_return_if_fail (func != NULL);

  priv = table->priv;

  d.func = func;
  d.data = data;

  g_hash_table_foreach (priv->hash, hash_foreach, &d);
  g_slist_foreach (priv->anonymous, list_foreach, &d);
}

/**
 * ctk_text_tag_table_get_size:
 * @table: a #CtkTextTagTable
 * 
 * Returns the size of the table (number of tags)
 * 
 * Returns: number of tags in @table
 **/
gint
ctk_text_tag_table_get_size (CtkTextTagTable *table)
{
  CtkTextTagTablePrivate *priv;

  g_return_val_if_fail (CTK_IS_TEXT_TAG_TABLE (table), 0);

  priv = table->priv;

  return g_hash_table_size (priv->hash) + priv->anon_count;
}

void
_ctk_text_tag_table_add_buffer (CtkTextTagTable *table,
                                gpointer         buffer)
{
  CtkTextTagTablePrivate *priv = table->priv;

  priv->buffers = g_slist_prepend (priv->buffers, buffer);
}

static void
foreach_remove_tag (CtkTextTag *tag, gpointer data)
{
  CtkTextBuffer *buffer;

  buffer = CTK_TEXT_BUFFER (data);

  _ctk_text_buffer_notify_will_remove_tag (buffer, tag);
}

void
_ctk_text_tag_table_remove_buffer (CtkTextTagTable *table,
                                   gpointer         buffer)
{
  CtkTextTagTablePrivate *priv = table->priv;

  ctk_text_tag_table_foreach (table, foreach_remove_tag, buffer);

  priv->buffers = g_slist_remove (priv->buffers, buffer);
}

void
_ctk_text_tag_table_tag_changed (CtkTextTagTable *table,
                                 CtkTextTag      *tag,
                                 gboolean         size_changed)
{
  check_visible (table, tag);
  g_signal_emit (table, signals[TAG_CHANGED], 0, tag, size_changed);
}

gboolean
_ctk_text_tag_table_affects_visibility (CtkTextTagTable *table)
{
  return table->priv->seen_invisible;
}
