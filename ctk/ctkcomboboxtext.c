/* CTK - The GIMP Toolkit
 *
 * Copyright (C) 2010 Christian Dywan
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
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "ctkcomboboxtext.h"
#include "ctkcombobox.h"
#include "ctkcellrenderertext.h"
#include "ctkcelllayout.h"
#include "ctkbuildable.h"
#include "ctkbuilderprivate.h"

#include <string.h>

/**
 * SECTION:ctkcomboboxtext
 * @Short_description: A simple, text-only combo box
 * @Title: CtkComboBoxText
 * @See_also: #CtkComboBox
 *
 * A CtkComboBoxText is a simple variant of #CtkComboBox that hides
 * the model-view complexity for simple text-only use cases.
 *
 * To create a CtkComboBoxText, use ctk_combo_box_text_new() or
 * ctk_combo_box_text_new_with_entry().
 *
 * You can add items to a CtkComboBoxText with
 * ctk_combo_box_text_append_text(), ctk_combo_box_text_insert_text()
 * or ctk_combo_box_text_prepend_text() and remove options with
 * ctk_combo_box_text_remove().
 *
 * If the CtkComboBoxText contains an entry (via the “has-entry” property),
 * its contents can be retrieved using ctk_combo_box_text_get_active_text().
 * The entry itself can be accessed by calling ctk_bin_get_child() on the
 * combo box.
 *
 * You should not call ctk_combo_box_set_model() or attempt to pack more cells
 * into this combo box via its CtkCellLayout interface.
 *
 * # CtkComboBoxText as CtkBuildable
 *
 * The CtkComboBoxText implementation of the CtkBuildable interface supports
 * adding items directly using the <items> element and specifying <item>
 * elements for each item. Each <item> element can specify the “id”
 * corresponding to the appended text and also supports the regular
 * translation attributes “translatable”, “context” and “comments”.
 *
 * Here is a UI definition fragment specifying CtkComboBoxText items:
 * |[
 * <object class="CtkComboBoxText">
 *   <items>
 *     <item translatable="yes" id="factory">Factory</item>
 *     <item translatable="yes" id="home">Home</item>
 *     <item translatable="yes" id="subway">Subway</item>
 *   </items>
 * </object>
 * ]|
 *
 * # CSS nodes
 *
 * |[<!-- language="plain" -->
 * combobox
 * ╰── box.linked
 *     ├── entry.combo
 *     ├── button.combo
 *     ╰── window.popup
 * ]|
 *
 * CtkComboBoxText has a single CSS node with name combobox. It adds
 * the style class .combo to the main CSS nodes of its entry and button
 * children, and the .linked class to the node of its internal box.
 */

static void     ctk_combo_box_text_buildable_interface_init     (CtkBuildableIface *iface);
static gboolean ctk_combo_box_text_buildable_custom_tag_start   (CtkBuildable     *buildable,
								 CtkBuilder       *builder,
								 GObject          *child,
								 const gchar      *tagname,
								 GMarkupParser    *parser,
								 gpointer         *data);

static void     ctk_combo_box_text_buildable_custom_finished    (CtkBuildable     *buildable,
								 CtkBuilder       *builder,
								 GObject          *child,
								 const gchar      *tagname,
								 gpointer          user_data);

static CtkBuildableIface *buildable_parent_iface = NULL;

G_DEFINE_TYPE_WITH_CODE (CtkComboBoxText, ctk_combo_box_text, CTK_TYPE_COMBO_BOX,
			 G_IMPLEMENT_INTERFACE (CTK_TYPE_BUILDABLE,
						ctk_combo_box_text_buildable_interface_init));

static void
ctk_combo_box_text_constructed (GObject *object)
{
  const gint text_column = 0;

  G_OBJECT_CLASS (ctk_combo_box_text_parent_class)->constructed (object);

  ctk_combo_box_set_entry_text_column (CTK_COMBO_BOX (object), text_column);
  ctk_combo_box_set_id_column (CTK_COMBO_BOX (object), 1);

  if (!ctk_combo_box_get_has_entry (CTK_COMBO_BOX (object)))
    {
      CtkCellRenderer *cell;

      cell = ctk_cell_renderer_text_new ();
      ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (object), cell, TRUE);
      ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (object), cell,
                                      "text", text_column,
                                      NULL);
    }
}

static void
ctk_combo_box_text_init (CtkComboBoxText *combo_box)
{
  CtkListStore *store;

  store = ctk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
  ctk_combo_box_set_model (CTK_COMBO_BOX (combo_box), CTK_TREE_MODEL (store));
  g_object_unref (store);
}

static void
ctk_combo_box_text_class_init (CtkComboBoxTextClass *klass)
{
  GObjectClass *object_class;

  object_class = (GObjectClass *)klass;
  object_class->constructed = ctk_combo_box_text_constructed;
}

static void
ctk_combo_box_text_buildable_interface_init (CtkBuildableIface *iface)
{
  buildable_parent_iface = g_type_interface_peek_parent (iface);

  iface->custom_tag_start = ctk_combo_box_text_buildable_custom_tag_start;
  iface->custom_finished = ctk_combo_box_text_buildable_custom_finished;
}

typedef struct {
  CtkBuilder    *builder;
  GObject       *object;
  const gchar   *domain;
  gchar         *id;

  GString       *string;

  gchar         *context;
  guint          translatable : 1;

  guint          is_text : 1;
} ItemParserData;

static void
item_start_element (GMarkupParseContext  *context,
                    const gchar          *element_name,
                    const gchar         **names,
                    const gchar         **values,
                    gpointer              user_data,
                    GError              **error)
{
  ItemParserData *data = (ItemParserData*)user_data;

  if (strcmp (element_name, "items") == 0)
    {
      if (!_ctk_builder_check_parent (data->builder, context, "object", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_INVALID, NULL, NULL,
                                        G_MARKUP_COLLECT_INVALID))
        _ctk_builder_prefix_error (data->builder, context, error);
    }
  else if (strcmp (element_name, "item") == 0)
    {
      const gchar *id = NULL;
      gboolean translatable = FALSE;
      const gchar *msg_context = NULL;

      if (!_ctk_builder_check_parent (data->builder, context, "items", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, "id", &id,
                                        G_MARKUP_COLLECT_BOOLEAN|G_MARKUP_COLLECT_OPTIONAL, "translatable", &translatable,
                                        G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, "comments", NULL,
                                        G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, "context", &msg_context,
                                        G_MARKUP_COLLECT_INVALID))
        {
          _ctk_builder_prefix_error (data->builder, context, error);
          return;
        }

      data->is_text = TRUE;
      data->translatable = translatable;
      data->context = g_strdup (msg_context);
      data->id = g_strdup (id);
    }
  else
    {
      _ctk_builder_error_unhandled_tag (data->builder, context,
                                        "CtkComboBoxText", element_name,
                                        error);
    }
}

static void
item_text (GMarkupParseContext  *context,
           const gchar          *text,
           gsize                 text_len,
           gpointer              user_data,
           GError              **error)
{
  ItemParserData *data = (ItemParserData*)user_data;

  if (data->is_text)
    g_string_append_len (data->string, text, text_len);
}

static void
item_end_element (GMarkupParseContext  *context,
                  const gchar          *element_name,
                  gpointer              user_data,
                  GError              **error)
{
  ItemParserData *data = (ItemParserData*)user_data;

  /* Append the translated strings */
  if (data->string->len)
    {
      if (data->translatable)
	{
	  const gchar *translated;

	  translated = _ctk_builder_parser_translate (data->domain,
						      data->context,
						      data->string->str);
	  g_string_assign (data->string, translated);
	}

      ctk_combo_box_text_append (CTK_COMBO_BOX_TEXT (data->object), data->id, data->string->str);
    }

  data->translatable = FALSE;
  g_string_set_size (data->string, 0);
  g_clear_pointer (&data->context, g_free);
  g_clear_pointer (&data->id, g_free);
  data->is_text = FALSE;
}

static const GMarkupParser item_parser =
  {
    .start_element = item_start_element,
    .end_element = item_end_element,
    .text = item_text
  };

static gboolean
ctk_combo_box_text_buildable_custom_tag_start (CtkBuildable  *buildable,
                                               CtkBuilder    *builder,
                                               GObject       *child,
                                               const gchar   *tagname,
                                               GMarkupParser *parser,
                                               gpointer      *parser_data)
{
  if (buildable_parent_iface->custom_tag_start (buildable, builder, child,
						tagname, parser, parser_data))
    return TRUE;

  if (strcmp (tagname, "items") == 0)
    {
      ItemParserData *data;

      data = g_slice_new0 (ItemParserData);
      data->builder = g_object_ref (builder);
      data->object = G_OBJECT (g_object_ref (buildable));
      data->domain = ctk_builder_get_translation_domain (builder);
      data->string = g_string_new ("");

      *parser = item_parser;
      *parser_data = data;

      return TRUE;
    }

  return FALSE;
}

static void
ctk_combo_box_text_buildable_custom_finished (CtkBuildable *buildable,
                                              CtkBuilder   *builder,
                                              GObject      *child,
                                              const gchar  *tagname,
                                              gpointer      user_data)
{
  ItemParserData *data;

  buildable_parent_iface->custom_finished (buildable, builder, child,
					   tagname, user_data);

  if (strcmp (tagname, "items") == 0)
    {
      data = (ItemParserData*)user_data;

      g_object_unref (data->object);
      g_object_unref (data->builder);
      g_string_free (data->string, TRUE);
      g_slice_free (ItemParserData, data);
    }
}

/**
 * ctk_combo_box_text_new:
 *
 * Creates a new #CtkComboBoxText, which is a #CtkComboBox just displaying
 * strings.
 *
 * Returns: A new #CtkComboBoxText
 *
 * Since: 2.24
 */
CtkWidget *
ctk_combo_box_text_new (void)
{
  return g_object_new (CTK_TYPE_COMBO_BOX_TEXT,
                       NULL);
}

/**
 * ctk_combo_box_text_new_with_entry:
 *
 * Creates a new #CtkComboBoxText, which is a #CtkComboBox just displaying
 * strings. The combo box created by this function has an entry.
 *
 * Returns: a new #CtkComboBoxText
 *
 * Since: 2.24
 */
CtkWidget *
ctk_combo_box_text_new_with_entry (void)
{
  return g_object_new (CTK_TYPE_COMBO_BOX_TEXT,
                       "has-entry", TRUE,
                       NULL);
}

/**
 * ctk_combo_box_text_append_text:
 * @combo_box: A #CtkComboBoxText
 * @text: A string
 *
 * Appends @text to the list of strings stored in @combo_box.
 *
 * This is the same as calling ctk_combo_box_text_insert_text() with a
 * position of -1.
 *
 * Since: 2.24
 */
void
ctk_combo_box_text_append_text (CtkComboBoxText *combo_box,
                                const gchar     *text)
{
  ctk_combo_box_text_insert (combo_box, -1, NULL, text);
}

/**
 * ctk_combo_box_text_prepend_text:
 * @combo_box: A #CtkComboBox
 * @text: A string
 *
 * Prepends @text to the list of strings stored in @combo_box.
 *
 * This is the same as calling ctk_combo_box_text_insert_text() with a
 * position of 0.
 *
 * Since: 2.24
 */
void
ctk_combo_box_text_prepend_text (CtkComboBoxText *combo_box,
                                 const gchar     *text)
{
  ctk_combo_box_text_insert (combo_box, 0, NULL, text);
}

/**
 * ctk_combo_box_text_insert_text:
 * @combo_box: A #CtkComboBoxText
 * @position: An index to insert @text
 * @text: A string
 *
 * Inserts @text at @position in the list of strings stored in @combo_box.
 *
 * If @position is negative then @text is appended.
 *
 * This is the same as calling ctk_combo_box_text_insert() with a %NULL
 * ID string.
 *
 * Since: 2.24
 */
void
ctk_combo_box_text_insert_text (CtkComboBoxText *combo_box,
                                gint             position,
                                const gchar     *text)
{
  ctk_combo_box_text_insert (combo_box, position, NULL, text);
}

/**
 * ctk_combo_box_text_append:
 * @combo_box: A #CtkComboBoxText
 * @id: (allow-none): a string ID for this value, or %NULL
 * @text: A string
 *
 * Appends @text to the list of strings stored in @combo_box.
 * If @id is non-%NULL then it is used as the ID of the row.
 *
 * This is the same as calling ctk_combo_box_text_insert() with a
 * position of -1.
 *
 * Since: 2.24
 */
void
ctk_combo_box_text_append (CtkComboBoxText *combo_box,
                           const gchar     *id,
                           const gchar     *text)
{
  ctk_combo_box_text_insert (combo_box, -1, id, text);
}

/**
 * ctk_combo_box_text_prepend:
 * @combo_box: A #CtkComboBox
 * @id: (allow-none): a string ID for this value, or %NULL
 * @text: a string
 *
 * Prepends @text to the list of strings stored in @combo_box.
 * If @id is non-%NULL then it is used as the ID of the row.
 *
 * This is the same as calling ctk_combo_box_text_insert() with a
 * position of 0.
 *
 * Since: 2.24
 */
void
ctk_combo_box_text_prepend (CtkComboBoxText *combo_box,
                            const gchar     *id,
                            const gchar     *text)
{
  ctk_combo_box_text_insert (combo_box, 0, id, text);
}


/**
 * ctk_combo_box_text_insert:
 * @combo_box: A #CtkComboBoxText
 * @position: An index to insert @text
 * @id: (allow-none): a string ID for this value, or %NULL
 * @text: A string to display
 *
 * Inserts @text at @position in the list of strings stored in @combo_box.
 * If @id is non-%NULL then it is used as the ID of the row.  See
 * #CtkComboBox:id-column.
 *
 * If @position is negative then @text is appended.
 *
 * Since: 3.0
 */
void
ctk_combo_box_text_insert (CtkComboBoxText *combo_box,
                           gint             position,
                           const gchar     *id,
                           const gchar     *text)
{
  CtkListStore *store;
  CtkTreeIter iter;
  gint text_column;
  gint column_type;

  g_return_if_fail (CTK_IS_COMBO_BOX_TEXT (combo_box));
  g_return_if_fail (text != NULL);

  store = CTK_LIST_STORE (ctk_combo_box_get_model (CTK_COMBO_BOX (combo_box)));
  g_return_if_fail (CTK_IS_LIST_STORE (store));

  text_column = ctk_combo_box_get_entry_text_column (CTK_COMBO_BOX (combo_box));

  if (ctk_combo_box_get_has_entry (CTK_COMBO_BOX (combo_box)))
    g_return_if_fail (text_column >= 0);
  else if (text_column < 0)
    text_column = 0;

  column_type = ctk_tree_model_get_column_type (CTK_TREE_MODEL (store), text_column);
  g_return_if_fail (column_type == G_TYPE_STRING);

  if (position < 0)
    ctk_list_store_append (store, &iter);
  else
    ctk_list_store_insert (store, &iter, position);

  ctk_list_store_set (store, &iter, text_column, text, -1);

  if (id != NULL)
    {
      gint id_column;

      id_column = ctk_combo_box_get_id_column (CTK_COMBO_BOX (combo_box));
      g_return_if_fail (id_column >= 0);
      column_type = ctk_tree_model_get_column_type (CTK_TREE_MODEL (store), id_column);
      g_return_if_fail (column_type == G_TYPE_STRING);

      ctk_list_store_set (store, &iter, id_column, id, -1);
    }
}

/**
 * ctk_combo_box_text_remove:
 * @combo_box: A #CtkComboBox
 * @position: Index of the item to remove
 *
 * Removes the string at @position from @combo_box.
 *
 * Since: 2.24
 */
void
ctk_combo_box_text_remove (CtkComboBoxText *combo_box,
                           gint             position)
{
  CtkTreeModel *model;
  CtkListStore *store;
  CtkTreeIter iter;

  g_return_if_fail (CTK_IS_COMBO_BOX_TEXT (combo_box));
  g_return_if_fail (position >= 0);

  model = ctk_combo_box_get_model (CTK_COMBO_BOX (combo_box));
  store = CTK_LIST_STORE (model);
  g_return_if_fail (CTK_IS_LIST_STORE (store));

  if (ctk_tree_model_iter_nth_child (model, &iter, NULL, position))
    ctk_list_store_remove (store, &iter);
}

/**
 * ctk_combo_box_text_remove_all:
 * @combo_box: A #CtkComboBoxText
 *
 * Removes all the text entries from the combo box.
 *
 * Since: 3.0
 */
void
ctk_combo_box_text_remove_all (CtkComboBoxText *combo_box)
{
  CtkListStore *store;

  g_return_if_fail (CTK_IS_COMBO_BOX_TEXT (combo_box));

  store = CTK_LIST_STORE (ctk_combo_box_get_model (CTK_COMBO_BOX (combo_box)));
  ctk_list_store_clear (store);
}

/**
 * ctk_combo_box_text_get_active_text:
 * @combo_box: A #CtkComboBoxText
 *
 * Returns the currently active string in @combo_box, or %NULL
 * if none is selected. If @combo_box contains an entry, this
 * function will return its contents (which will not necessarily
 * be an item from the list).
 *
 * Returns: (transfer full): a newly allocated string containing the
 *     currently active text. Must be freed with g_free().
 *
 * Since: 2.24
 */
gchar *
ctk_combo_box_text_get_active_text (CtkComboBoxText *combo_box)
{
  CtkTreeIter iter;
  gchar *text = NULL;

  g_return_val_if_fail (CTK_IS_COMBO_BOX_TEXT (combo_box), NULL);

 if (ctk_combo_box_get_has_entry (CTK_COMBO_BOX (combo_box)))
   {
     CtkWidget *entry;

     entry = ctk_bin_get_child (CTK_BIN (combo_box));
     text = g_strdup (ctk_entry_get_text (CTK_ENTRY (entry)));
   }
  else if (ctk_combo_box_get_active_iter (CTK_COMBO_BOX (combo_box), &iter))
    {
      CtkTreeModel *model;
      gint text_column;
      gint column_type;

      model = ctk_combo_box_get_model (CTK_COMBO_BOX (combo_box));
      g_return_val_if_fail (CTK_IS_LIST_STORE (model), NULL);
      text_column = ctk_combo_box_get_entry_text_column (CTK_COMBO_BOX (combo_box));
      g_return_val_if_fail (text_column >= 0, NULL);
      column_type = ctk_tree_model_get_column_type (model, text_column);
      g_return_val_if_fail (column_type == G_TYPE_STRING, NULL);
      ctk_tree_model_get (model, &iter, text_column, &text, -1);
    }

  return text;
}
