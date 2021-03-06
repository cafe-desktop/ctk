/* CTK - The GIMP Toolkit
 * Copyright (C) 2010 Carlos Garnacho <carlosg@gnome.org>
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

#include "ctkwidgetpath.h"

#include <string.h>

#include "ctkcssnodedeclarationprivate.h"
#include "ctkprivate.h"
#include "ctkstylecontextprivate.h"
#include "ctktypebuiltins.h"
#include "ctkwidget.h"
#include "ctkwidgetpathprivate.h"

/**
 * SECTION:ctkwidgetpath
 * @Short_description: Widget path abstraction
 * @Title: CtkWidgetPath
 * @See_also: #CtkStyleContext
 *
 * CtkWidgetPath is a boxed type that represents a widget hierarchy from
 * the topmost widget, typically a toplevel, to any child. This widget
 * path abstraction is used in #CtkStyleContext on behalf of the real
 * widget in order to query style information.
 *
 * If you are using CTK+ widgets, you probably will not need to use
 * this API directly, as there is ctk_widget_get_path(), and the style
 * context returned by ctk_widget_get_style_context() will be automatically
 * updated on widget hierarchy changes.
 *
 * The widget path generation is generally simple:
 *
 * ## Defining a button within a window
 *
 * |[<!-- language="C" -->
 * {
 *   CtkWidgetPath *path;
 *
 *   path = ctk_widget_path_new ();
 *   ctk_widget_path_append_type (path, CTK_TYPE_WINDOW);
 *   ctk_widget_path_append_type (path, CTK_TYPE_BUTTON);
 * }
 * ]|
 *
 * Although more complex information, such as widget names, or
 * different classes (property that may be used by other widget
 * types) and intermediate regions may be included:
 *
 * ## Defining the first tab widget in a notebook
 *
 * |[<!-- language="C" -->
 * {
 *   CtkWidgetPath *path;
 *   guint pos;
 *
 *   path = ctk_widget_path_new ();
 *
 *   pos = ctk_widget_path_append_type (path, CTK_TYPE_NOTEBOOK);
 *   ctk_widget_path_iter_add_region (path, pos, "tab", CTK_REGION_EVEN | CTK_REGION_FIRST);
 *
 *   pos = ctk_widget_path_append_type (path, CTK_TYPE_LABEL);
 *   ctk_widget_path_iter_set_name (path, pos, "first tab label");
 * }
 * ]|
 *
 * All this information will be used to match the style information
 * that applies to the described widget.
 **/

G_DEFINE_BOXED_TYPE (CtkWidgetPath, ctk_widget_path,
		     ctk_widget_path_ref, ctk_widget_path_unref)


typedef struct CtkPathElement CtkPathElement;

struct CtkPathElement
{
  CtkCssNodeDeclaration *decl;
  guint sibling_index;
  CtkWidgetPath *siblings;
};

struct _CtkWidgetPath
{
  guint ref_count;

  GArray *elems; /* First element contains the described widget */
};

/**
 * ctk_widget_path_new:
 *
 * Returns an empty widget path.
 *
 * Returns: (transfer full): A newly created, empty, #CtkWidgetPath
 *
 * Since: 3.0
 **/
CtkWidgetPath *
ctk_widget_path_new (void)
{
  CtkWidgetPath *path;

  path = g_slice_new0 (CtkWidgetPath);
  path->elems = g_array_new (FALSE, TRUE, sizeof (CtkPathElement));
  path->ref_count = 1;

  return path;
}

static void
ctk_path_element_copy (CtkPathElement       *dest,
                       const CtkPathElement *src)
{
  memset (dest, 0, sizeof (CtkPathElement));

  dest->decl = ctk_css_node_declaration_ref (src->decl);
  if (src->siblings)
    dest->siblings = ctk_widget_path_ref (src->siblings);
  dest->sibling_index = src->sibling_index;
}

/**
 * ctk_widget_path_copy:
 * @path: a #CtkWidgetPath
 *
 * Returns a copy of @path
 *
 * Returns: (transfer full): a copy of @path
 *
 * Since: 3.0
 **/
CtkWidgetPath *
ctk_widget_path_copy (const CtkWidgetPath *path)
{
  CtkWidgetPath *new_path;
  guint i;

  ctk_internal_return_val_if_fail (path != NULL, NULL);

  new_path = ctk_widget_path_new ();

  g_array_set_size (new_path->elems, path->elems->len);

  for (i = 0; i < path->elems->len; i++)
    {
      CtkPathElement *elem, *dest;

      elem = &g_array_index (path->elems, CtkPathElement, i);
      dest = &g_array_index (new_path->elems, CtkPathElement, i);

      ctk_path_element_copy (dest, elem);
    }

  return new_path;
}

/**
 * ctk_widget_path_ref:
 * @path: a #CtkWidgetPath
 *
 * Increments the reference count on @path.
 *
 * Returns: @path itself.
 *
 * Since: 3.2
 **/
CtkWidgetPath *
ctk_widget_path_ref (CtkWidgetPath *path)
{
  ctk_internal_return_val_if_fail (path != NULL, path);

  path->ref_count += 1;

  return path;
}

/**
 * ctk_widget_path_unref:
 * @path: a #CtkWidgetPath
 *
 * Decrements the reference count on @path, freeing the structure
 * if the reference count reaches 0.
 *
 * Since: 3.2
 **/
void
ctk_widget_path_unref (CtkWidgetPath *path)
{
  guint i;

  ctk_internal_return_if_fail (path != NULL);

  path->ref_count -= 1;
  if (path->ref_count > 0)
    return;

  for (i = 0; i < path->elems->len; i++)
    {
      CtkPathElement *elem;

      elem = &g_array_index (path->elems, CtkPathElement, i);

      ctk_css_node_declaration_unref (elem->decl);
      if (elem->siblings)
        ctk_widget_path_unref (elem->siblings);
    }

  g_array_free (path->elems, TRUE);
  g_slice_free (CtkWidgetPath, path);
}

/**
 * ctk_widget_path_free:
 * @path: a #CtkWidgetPath
 *
 * Decrements the reference count on @path, freeing the structure
 * if the reference count reaches 0.
 *
 * Since: 3.0
 **/
void
ctk_widget_path_free (CtkWidgetPath *path)
{
  ctk_internal_return_if_fail (path != NULL);

  ctk_widget_path_unref (path);
}

/**
 * ctk_widget_path_length:
 * @path: a #CtkWidgetPath
 *
 * Returns the number of #CtkWidget #GTypes between the represented
 * widget and its topmost container.
 *
 * Returns: the number of elements in the path
 *
 * Since: 3.0
 **/
gint
ctk_widget_path_length (const CtkWidgetPath *path)
{
  ctk_internal_return_val_if_fail (path != NULL, 0);

  return path->elems->len;
}

/**
 * ctk_widget_path_to_string:
 * @path: the path
 *
 * Dumps the widget path into a string representation. It tries to match
 * the CSS style as closely as possible (Note that there might be paths
 * that cannot be represented in CSS).
 *
 * The main use of this code is for debugging purposes, so that you can
 * g_print() the path or dump it in a gdb session.
 *
 * Returns: A new string describing @path.
 *
 * Since: 3.2
 **/
char *
ctk_widget_path_to_string (const CtkWidgetPath *path)
{
  GString *string;
  guint i, j, n;

  ctk_internal_return_val_if_fail (path != NULL, NULL);

  string = g_string_new ("");

  for (i = 0; i < path->elems->len; i++)
    {
      CtkPathElement *elem;
      CtkStateFlags state;
      const GQuark *classes;
      GList *list, *regions;

      elem = &g_array_index (path->elems, CtkPathElement, i);

      if (i > 0)
        g_string_append_c (string, ' ');

      if (ctk_css_node_declaration_get_name (elem->decl))
        g_string_append (string, ctk_css_node_declaration_get_name (elem->decl));
      else
        g_string_append (string, g_type_name (ctk_css_node_declaration_get_type (elem->decl)));

      if (ctk_css_node_declaration_get_id (elem->decl))
        {
          g_string_append_c (string, '(');
          g_string_append (string, ctk_css_node_declaration_get_id (elem->decl));
          g_string_append_c (string, ')');
        }

      state = ctk_css_node_declaration_get_state (elem->decl);
      if (state)
        {
          GFlagsClass *fclass;

          fclass = g_type_class_ref (CTK_TYPE_STATE_FLAGS);
          for (j = 0; j < fclass->n_values; j++)
            {
              if (state & fclass->values[j].value)
                {
                  g_string_append_c (string, ':');
                  g_string_append (string, fclass->values[j].value_nick);
                }
            }
          g_type_class_unref (fclass);
        }

      if (elem->siblings)
        g_string_append_printf (string, "[%d/%d]",
                                elem->sibling_index + 1,
                                ctk_widget_path_length (elem->siblings));

      classes = ctk_css_node_declaration_get_classes (elem->decl, &n);
      for (j = 0; j < n; j++)
        {
          g_string_append_c (string, '.');
          g_string_append (string, g_quark_to_string (classes[j]));
        }

      regions = ctk_css_node_declaration_list_regions (elem->decl);
      for (list = regions; list; list = list->next)
        {
          static const char *flag_names[] = {
            "even",
            "odd",
            "first",
            "last",
            "only",
            "sorted"
          };
          CtkRegionFlags flags;
          GQuark region = GPOINTER_TO_UINT (regions->data);

          ctk_css_node_declaration_has_region (elem->decl, region, &flags);
          g_string_append_c (string, ' ');
          g_string_append (string, g_quark_to_string (region));
          for (j = 0; j < G_N_ELEMENTS(flag_names); j++)
            {
              if (flags & (1 << j))
                {
                  g_string_append_c (string, ':');
                  g_string_append (string, flag_names[j]);
                }
            }
        }
      g_list_free (regions);
    }

  return g_string_free (string, FALSE);
}

/**
 * ctk_widget_path_prepend_type:
 * @path: a #CtkWidgetPath
 * @type: widget type to prepend
 *
 * Prepends a widget type to the widget hierachy represented by @path.
 *
 * Since: 3.0
 **/
void
ctk_widget_path_prepend_type (CtkWidgetPath *path,
                              GType          type)
{
  CtkPathElement new = { NULL };

  ctk_internal_return_if_fail (path != NULL);

  new.decl = ctk_css_node_declaration_new ();
  ctk_css_node_declaration_set_type (&new.decl, type);

  g_array_prepend_val (path->elems, new);
}

/**
 * ctk_widget_path_append_type:
 * @path: a #CtkWidgetPath
 * @type: widget type to append
 *
 * Appends a widget type to the widget hierarchy represented by @path.
 *
 * Returns: the position where the element was inserted
 *
 * Since: 3.0
 **/
gint
ctk_widget_path_append_type (CtkWidgetPath *path,
                             GType          type)
{
  CtkPathElement new = { NULL };

  ctk_internal_return_val_if_fail (path != NULL, 0);

  new.decl = ctk_css_node_declaration_new ();
  ctk_css_node_declaration_set_type (&new.decl, type);
  g_array_append_val (path->elems, new);

  return path->elems->len - 1;
}

/**
 * ctk_widget_path_append_with_siblings:
 * @path: the widget path to append to
 * @siblings: a widget path describing a list of siblings. This path
 *   may not contain any siblings itself and it must not be modified
 *   afterwards.
 * @sibling_index: index into @siblings for where the added element is
 *   positioned.
 *
 * Appends a widget type with all its siblings to the widget hierarchy
 * represented by @path. Using this function instead of
 * ctk_widget_path_append_type() will allow the CSS theming to use
 * sibling matches in selectors and apply :nth-child() pseudo classes.
 * In turn, it requires a lot more care in widget implementations as
 * widgets need to make sure to call ctk_widget_reset_style() on all
 * involved widgets when the @siblings path changes.
 *
 * Returns: the position where the element was inserted.
 *
 * Since: 3.2
 **/
gint
ctk_widget_path_append_with_siblings (CtkWidgetPath *path,
                                      CtkWidgetPath *siblings,
                                      guint          sibling_index)
{
  CtkPathElement new;

  ctk_internal_return_val_if_fail (path != NULL, 0);
  ctk_internal_return_val_if_fail (siblings != NULL, 0);
  ctk_internal_return_val_if_fail (sibling_index < ctk_widget_path_length (siblings), 0);

  ctk_path_element_copy (&new, &g_array_index (siblings->elems, CtkPathElement, sibling_index));
  new.siblings = ctk_widget_path_ref (siblings);
  new.sibling_index = sibling_index;
  g_array_append_val (path->elems, new);

  return path->elems->len - 1;
}

/**
 * ctk_widget_path_iter_get_siblings:
 * @path: a #CtkWidgetPath
 * @pos: position to get the siblings for, -1 for the path head
 *
 * Returns the list of siblings for the element at @pos. If the element
 * was not added with siblings, %NULL is returned.
 *
 * Returns: %NULL or the list of siblings for the element at @pos.
 **/
const CtkWidgetPath *
ctk_widget_path_iter_get_siblings (const CtkWidgetPath *path,
                                   gint                 pos)
{
  CtkPathElement *elem;

  ctk_internal_return_val_if_fail (path != NULL, NULL);
  ctk_internal_return_val_if_fail (path->elems->len != 0, NULL);

  if (pos < 0 || pos >= path->elems->len)
    pos = path->elems->len - 1;

  elem = &g_array_index (path->elems, CtkPathElement, pos);
  return elem->siblings;
}

/**
 * ctk_widget_path_iter_get_sibling_index:
 * @path: a #CtkWidgetPath
 * @pos: position to get the sibling index for, -1 for the path head
 *
 * Returns the index into the list of siblings for the element at @pos as
 * returned by ctk_widget_path_iter_get_siblings(). If that function would
 * return %NULL because the element at @pos has no siblings, this function
 * will return 0.
 *
 * Returns: 0 or the index into the list of siblings for the element at @pos.
 **/
guint
ctk_widget_path_iter_get_sibling_index (const CtkWidgetPath *path,
                                        gint                 pos)
{
  CtkPathElement *elem;

  ctk_internal_return_val_if_fail (path != NULL, G_TYPE_INVALID);
  ctk_internal_return_val_if_fail (path->elems->len != 0, G_TYPE_INVALID);

  if (pos < 0 || pos >= path->elems->len)
    pos = path->elems->len - 1;

  elem = &g_array_index (path->elems, CtkPathElement, pos);
  return elem->sibling_index;
}

/**
 * ctk_widget_path_iter_get_object_name:
 * @path: a #CtkWidgetPath
 * @pos: position to get the object name for, -1 for the path head
 *
 * Returns the object name that is at position @pos in the widget
 * hierarchy defined in @path.
 *
 * Returns: (nullable): the name or %NULL
 *
 * Since: 3.20
 **/
const char *
ctk_widget_path_iter_get_object_name (const CtkWidgetPath *path,
                                      gint                 pos)
{
  CtkPathElement *elem;

  ctk_internal_return_val_if_fail (path != NULL, NULL);
  ctk_internal_return_val_if_fail (path->elems->len != 0, NULL);

  if (pos < 0 || pos >= path->elems->len)
    pos = path->elems->len - 1;

  elem = &g_array_index (path->elems, CtkPathElement, pos);
  return ctk_css_node_declaration_get_name (elem->decl);
}

/**
 * ctk_widget_path_iter_set_object_name:
 * @path: a #CtkWidgetPath
 * @pos: position to modify, -1 for the path head
 * @name: (allow-none): object name to set or %NULL to unset
 *
 * Sets the object name for a given position in the widget hierarchy
 * defined by @path.
 *
 * When set, the object name overrides the object type when matching
 * CSS.
 *
 * Since: 3.20
 **/
void
ctk_widget_path_iter_set_object_name (CtkWidgetPath *path,
                                      gint           pos,
                                      const char    *name)
{
  CtkPathElement *elem;

  ctk_internal_return_if_fail (path != NULL);
  ctk_internal_return_if_fail (path->elems->len != 0);

  if (pos < 0 || pos >= path->elems->len)
    pos = path->elems->len - 1;

  elem = &g_array_index (path->elems, CtkPathElement, pos);
  ctk_css_node_declaration_set_name (&elem->decl, g_intern_string (name));
}

/**
 * ctk_widget_path_iter_get_object_type:
 * @path: a #CtkWidgetPath
 * @pos: position to get the object type for, -1 for the path head
 *
 * Returns the object #GType that is at position @pos in the widget
 * hierarchy defined in @path.
 *
 * Returns: a widget type
 *
 * Since: 3.0
 **/
GType
ctk_widget_path_iter_get_object_type (const CtkWidgetPath *path,
                                      gint                 pos)
{
  CtkPathElement *elem;

  ctk_internal_return_val_if_fail (path != NULL, G_TYPE_INVALID);
  ctk_internal_return_val_if_fail (path->elems->len != 0, G_TYPE_INVALID);

  if (pos < 0 || pos >= path->elems->len)
    pos = path->elems->len - 1;

  elem = &g_array_index (path->elems, CtkPathElement, pos);
  return ctk_css_node_declaration_get_type (elem->decl);
}

/**
 * ctk_widget_path_iter_set_object_type:
 * @path: a #CtkWidgetPath
 * @pos: position to modify, -1 for the path head
 * @type: object type to set
 *
 * Sets the object type for a given position in the widget hierarchy
 * defined by @path.
 *
 * Since: 3.0
 **/
void
ctk_widget_path_iter_set_object_type (CtkWidgetPath *path,
                                      gint           pos,
                                      GType          type)
{
  CtkPathElement *elem;

  ctk_internal_return_if_fail (path != NULL);
  ctk_internal_return_if_fail (path->elems->len != 0);

  if (pos < 0 || pos >= path->elems->len)
    pos = path->elems->len - 1;

  elem = &g_array_index (path->elems, CtkPathElement, pos);
  ctk_css_node_declaration_set_type (&elem->decl, type);
}

/**
 * ctk_widget_path_iter_get_state:
 * @path: a #CtkWidgetPath
 * @pos: position to get the state for, -1 for the path head
 *
 * Returns the state flags corresponding to the widget found at
 * the position @pos in the widget hierarchy defined by
 * @path
 *
 * Returns: The state flags
 *
 * Since: 3.14
 **/
CtkStateFlags
ctk_widget_path_iter_get_state (const CtkWidgetPath *path,
                                gint                 pos)
{
  CtkPathElement *elem;

  ctk_internal_return_val_if_fail (path != NULL, 0);
  ctk_internal_return_val_if_fail (path->elems->len != 0, 0);

  if (pos < 0 || pos >= path->elems->len)
    pos = path->elems->len - 1;

  elem = &g_array_index (path->elems, CtkPathElement, pos);
  return ctk_css_node_declaration_get_state (elem->decl);
}

/**
 * ctk_widget_path_iter_set_state:
 * @path: a #CtkWidgetPath
 * @pos: position to modify, -1 for the path head
 * @state: state flags
 *
 * Sets the widget name for the widget found at position @pos
 * in the widget hierarchy defined by @path.
 *
 * If you want to update just a single state flag, you need to do
 * this manually, as this function updates all state flags.
 *
 * ## Setting a flag
 *
 * |[<!-- language="C" -->
 * ctk_widget_path_iter_set_state (path, pos, ctk_widget_path_iter_get_state (path, pos) | flag);
 * ]|
 *
 * ## Unsetting a flag
 *
 * |[<!-- language="C" -->
 * ctk_widget_path_iter_set_state (path, pos, ctk_widget_path_iter_get_state (path, pos) & ~flag);
 * ]|
 *
 *
 * Since: 3.14
 **/
void
ctk_widget_path_iter_set_state (CtkWidgetPath *path,
                                gint           pos,
                                CtkStateFlags  state)
{
  CtkPathElement *elem;

  ctk_internal_return_if_fail (path != NULL);
  ctk_internal_return_if_fail (path->elems->len != 0);

  if (pos < 0 || pos >= path->elems->len)
    pos = path->elems->len - 1;

  elem = &g_array_index (path->elems, CtkPathElement, pos);

  ctk_css_node_declaration_set_state (&elem->decl, state);
}

/**
 * ctk_widget_path_iter_get_name:
 * @path: a #CtkWidgetPath
 * @pos: position to get the widget name for, -1 for the path head
 *
 * Returns the name corresponding to the widget found at
 * the position @pos in the widget hierarchy defined by
 * @path
 *
 * Returns: (nullable): The widget name, or %NULL if none was set.
 **/
const gchar *
ctk_widget_path_iter_get_name (const CtkWidgetPath *path,
                               gint                 pos)
{
  CtkPathElement *elem;

  ctk_internal_return_val_if_fail (path != NULL, NULL);
  ctk_internal_return_val_if_fail (path->elems->len != 0, NULL);

  if (pos < 0 || pos >= path->elems->len)
    pos = path->elems->len - 1;

  elem = &g_array_index (path->elems, CtkPathElement, pos);
  return ctk_css_node_declaration_get_id (elem->decl);
}

/**
 * ctk_widget_path_iter_set_name:
 * @path: a #CtkWidgetPath
 * @pos: position to modify, -1 for the path head
 * @name: widget name
 *
 * Sets the widget name for the widget found at position @pos
 * in the widget hierarchy defined by @path.
 *
 * Since: 3.0
 **/
void
ctk_widget_path_iter_set_name (CtkWidgetPath *path,
                               gint           pos,
                               const gchar   *name)
{
  CtkPathElement *elem;

  ctk_internal_return_if_fail (path != NULL);
  ctk_internal_return_if_fail (path->elems->len != 0);
  ctk_internal_return_if_fail (name != NULL);

  if (pos < 0 || pos >= path->elems->len)
    pos = path->elems->len - 1;

  elem = &g_array_index (path->elems, CtkPathElement, pos);

  ctk_css_node_declaration_set_id (&elem->decl, g_intern_string (name));
}

/**
 * ctk_widget_path_iter_has_qname:
 * @path: a #CtkWidgetPath
 * @pos: position to query, -1 for the path head
 * @qname: widget name as a #GQuark
 *
 * See ctk_widget_path_iter_has_name(). This is a version
 * that operates on #GQuarks.
 *
 * Returns: %TRUE if the widget at @pos has this name
 *
 * Since: 3.0
 **/
gboolean
ctk_widget_path_iter_has_qname (const CtkWidgetPath *path,
                                gint                 pos,
                                GQuark               qname)
{
  ctk_internal_return_val_if_fail (path != NULL, FALSE);
  ctk_internal_return_val_if_fail (path->elems->len != 0, FALSE);
  ctk_internal_return_val_if_fail (qname != 0, FALSE);

  return ctk_widget_path_iter_has_name (path, pos, g_quark_to_string (qname));
}

/**
 * ctk_widget_path_iter_has_name:
 * @path: a #CtkWidgetPath
 * @pos: position to query, -1 for the path head
 * @name: a widget name
 *
 * Returns %TRUE if the widget at position @pos has the name @name,
 * %FALSE otherwise.
 *
 * Returns: %TRUE if the widget at @pos has this name
 *
 * Since: 3.0
 **/
gboolean
ctk_widget_path_iter_has_name (const CtkWidgetPath *path,
                               gint                 pos,
                               const gchar         *name)
{
  CtkPathElement *elem;

  ctk_internal_return_val_if_fail (path != NULL, FALSE);
  ctk_internal_return_val_if_fail (path->elems->len != 0, FALSE);

  if (pos < 0 || pos >= path->elems->len)
    pos = path->elems->len - 1;

  name = g_intern_string (name);
  elem = &g_array_index (path->elems, CtkPathElement, pos);

  return ctk_css_node_declaration_get_id (elem->decl) == name;
}

/**
 * ctk_widget_path_iter_add_class:
 * @path: a #CtkWidget
 * @pos: position to modify, -1 for the path head
 * @name: a class name
 *
 * Adds the class @name to the widget at position @pos in
 * the hierarchy defined in @path. See
 * ctk_style_context_add_class().
 *
 * Since: 3.0
 **/
void
ctk_widget_path_iter_add_class (CtkWidgetPath *path,
                                gint           pos,
                                const gchar   *name)
{
  ctk_internal_return_if_fail (path != NULL);
  ctk_internal_return_if_fail (path->elems->len != 0);
  ctk_internal_return_if_fail (name != NULL);

  if (pos < 0 || pos >= path->elems->len)
    pos = path->elems->len - 1;

  ctk_widget_path_iter_add_qclass (path, pos, g_quark_from_string (name));
}

void
ctk_widget_path_iter_add_qclass (CtkWidgetPath *path,
                                 gint           pos,
                                 GQuark         qname)
{
  CtkPathElement *elem;

  elem = &g_array_index (path->elems, CtkPathElement, pos);

  ctk_css_node_declaration_add_class (&elem->decl, qname);
}

/**
 * ctk_widget_path_iter_remove_class:
 * @path: a #CtkWidgetPath
 * @pos: position to modify, -1 for the path head
 * @name: class name
 *
 * Removes the class @name from the widget at position @pos in
 * the hierarchy defined in @path.
 *
 * Since: 3.0
 **/
void
ctk_widget_path_iter_remove_class (CtkWidgetPath *path,
                                   gint           pos,
                                   const gchar   *name)
{
  CtkPathElement *elem;
  GQuark qname;

  ctk_internal_return_if_fail (path != NULL);
  ctk_internal_return_if_fail (path->elems->len != 0);
  ctk_internal_return_if_fail (name != NULL);

  if (pos < 0 || pos >= path->elems->len)
    pos = path->elems->len - 1;

  elem = &g_array_index (path->elems, CtkPathElement, pos);
  qname = g_quark_try_string (name);
  if (qname == 0)
    return;

  ctk_css_node_declaration_remove_class (&elem->decl, qname);
}

/**
 * ctk_widget_path_iter_clear_classes:
 * @path: a #CtkWidget
 * @pos: position to modify, -1 for the path head
 *
 * Removes all classes from the widget at position @pos in the
 * hierarchy defined in @path.
 *
 * Since: 3.0
 **/
void
ctk_widget_path_iter_clear_classes (CtkWidgetPath *path,
                                    gint           pos)
{
  CtkPathElement *elem;

  ctk_internal_return_if_fail (path != NULL);
  ctk_internal_return_if_fail (path->elems->len != 0);

  if (pos < 0 || pos >= path->elems->len)
    pos = path->elems->len - 1;

  elem = &g_array_index (path->elems, CtkPathElement, pos);

  ctk_css_node_declaration_clear_classes (&elem->decl);
}

/**
 * ctk_widget_path_iter_list_classes:
 * @path: a #CtkWidgetPath
 * @pos: position to query, -1 for the path head
 *
 * Returns a list with all the class names defined for the widget
 * at position @pos in the hierarchy defined in @path.
 *
 * Returns: (transfer container) (element-type utf8): The list of
 *          classes, This is a list of strings, the #GSList contents
 *          are owned by CTK+, but you should use g_slist_free() to
 *          free the list itself.
 *
 * Since: 3.0
 **/
GSList *
ctk_widget_path_iter_list_classes (const CtkWidgetPath *path,
                                   gint                 pos)
{
  CtkPathElement *elem;
  GSList *list = NULL;
  const GQuark *classes;
  guint i, n;

  ctk_internal_return_val_if_fail (path != NULL, NULL);
  ctk_internal_return_val_if_fail (path->elems->len != 0, NULL);

  if (pos < 0 || pos >= path->elems->len)
    pos = path->elems->len - 1;

  elem = &g_array_index (path->elems, CtkPathElement, pos);
  classes = ctk_css_node_declaration_get_classes (elem->decl, &n);

  for (i = 0; i < n; i++)
    {
      list = g_slist_prepend (list, (gchar *) g_quark_to_string (classes[i]));
    }

  return g_slist_reverse (list);
}

/**
 * ctk_widget_path_iter_has_qclass:
 * @path: a #CtkWidgetPath
 * @pos: position to query, -1 for the path head
 * @qname: class name as a #GQuark
 *
 * See ctk_widget_path_iter_has_class(). This is a version that operates
 * with GQuarks.
 *
 * Returns: %TRUE if the widget at @pos has the class defined.
 *
 * Since: 3.0
 **/
gboolean
ctk_widget_path_iter_has_qclass (const CtkWidgetPath *path,
                                 gint                 pos,
                                 GQuark               qname)
{
  CtkPathElement *elem;

  ctk_internal_return_val_if_fail (path != NULL, FALSE);
  ctk_internal_return_val_if_fail (path->elems->len != 0, FALSE);
  ctk_internal_return_val_if_fail (qname != 0, FALSE);

  if (pos < 0 || pos >= path->elems->len)
    pos = path->elems->len - 1;

  elem = &g_array_index (path->elems, CtkPathElement, pos);

  return ctk_css_node_declaration_has_class (elem->decl, qname);
}

/**
 * ctk_widget_path_iter_has_class:
 * @path: a #CtkWidgetPath
 * @pos: position to query, -1 for the path head
 * @name: class name
 *
 * Returns %TRUE if the widget at position @pos has the class @name
 * defined, %FALSE otherwise.
 *
 * Returns: %TRUE if the class @name is defined for the widget at @pos
 *
 * Since: 3.0
 **/
gboolean
ctk_widget_path_iter_has_class (const CtkWidgetPath *path,
                                gint                 pos,
                                const gchar         *name)
{
  GQuark qname;

  ctk_internal_return_val_if_fail (path != NULL, FALSE);
  ctk_internal_return_val_if_fail (path->elems->len != 0, FALSE);
  ctk_internal_return_val_if_fail (name != NULL, FALSE);

  if (pos < 0 || pos >= path->elems->len)
    pos = path->elems->len - 1;

  qname = g_quark_try_string (name);

  if (qname == 0)
    return FALSE;

  return ctk_widget_path_iter_has_qclass (path, pos, qname);
}

/**
 * ctk_widget_path_iter_add_region:
 * @path: a #CtkWidgetPath
 * @pos: position to modify, -1 for the path head
 * @name: region name
 * @flags: flags affecting the region
 *
 * Adds the region @name to the widget at position @pos in
 * the hierarchy defined in @path. See
 * ctk_style_context_add_region().
 *
 * Region names must only contain lowercase letters
 * and “-”, starting always with a lowercase letter.
 *
 * Since: 3.0
 *
 * Deprecated: 3.14: The use of regions is deprecated.
 **/
void
ctk_widget_path_iter_add_region (CtkWidgetPath  *path,
                                 gint            pos,
                                 const gchar    *name,
                                 CtkRegionFlags  flags)
{
  CtkPathElement *elem;
  GQuark qname;

  ctk_internal_return_if_fail (path != NULL);
  ctk_internal_return_if_fail (path->elems->len != 0);
  ctk_internal_return_if_fail (name != NULL);
  ctk_internal_return_if_fail (_ctk_style_context_check_region_name (name));

  if (pos < 0 || pos >= path->elems->len)
    pos = path->elems->len - 1;

  elem = &g_array_index (path->elems, CtkPathElement, pos);
  qname = g_quark_from_string (name);

  ctk_css_node_declaration_add_region (&elem->decl, qname, flags);
}

/**
 * ctk_widget_path_iter_remove_region:
 * @path: a #CtkWidgetPath
 * @pos: position to modify, -1 for the path head
 * @name: region name
 *
 * Removes the region @name from the widget at position @pos in
 * the hierarchy defined in @path.
 *
 * Since: 3.0
 *
 * Deprecated: 3.14: The use of regions is deprecated.
 **/
void
ctk_widget_path_iter_remove_region (CtkWidgetPath *path,
                                    gint           pos,
                                    const gchar   *name)
{
  CtkPathElement *elem;
  GQuark qname;

  ctk_internal_return_if_fail (path != NULL);
  ctk_internal_return_if_fail (path->elems->len != 0);
  ctk_internal_return_if_fail (name != NULL);

  if (pos < 0 || pos >= path->elems->len)
    pos = path->elems->len - 1;

  elem = &g_array_index (path->elems, CtkPathElement, pos);
  qname = g_quark_try_string (name);
  if (qname == 0)
    return;

  ctk_css_node_declaration_remove_region (&elem->decl, qname);
}

/**
 * ctk_widget_path_iter_clear_regions:
 * @path: a #CtkWidgetPath
 * @pos: position to modify, -1 for the path head
 *
 * Removes all regions from the widget at position @pos in the
 * hierarchy defined in @path.
 *
 * Since: 3.0
 *
 * Deprecated: 3.14: The use of regions is deprecated.
 **/
void
ctk_widget_path_iter_clear_regions (CtkWidgetPath *path,
                                    gint           pos)
{
  CtkPathElement *elem;

  ctk_internal_return_if_fail (path != NULL);
  ctk_internal_return_if_fail (path->elems->len != 0);

  if (pos < 0 || pos >= path->elems->len)
    pos = path->elems->len - 1;

  elem = &g_array_index (path->elems, CtkPathElement, pos);

  ctk_css_node_declaration_clear_regions (&elem->decl);
}

/**
 * ctk_widget_path_iter_list_regions:
 * @path: a #CtkWidgetPath
 * @pos: position to query, -1 for the path head
 *
 * Returns a list with all the region names defined for the widget
 * at position @pos in the hierarchy defined in @path.
 *
 * Returns: (transfer container) (element-type utf8): The list of
 *          regions, This is a list of strings, the #GSList contents
 *          are owned by CTK+, but you should use g_slist_free() to
 *          free the list itself.
 *
 * Since: 3.0
 *
 * Deprecated: 3.14: The use of regions is deprecated.
 **/
GSList *
ctk_widget_path_iter_list_regions (const CtkWidgetPath *path,
                                   gint                 pos)
{
  CtkPathElement *elem;
  GSList *list = NULL;
  GList *l, *wrong_list;

  ctk_internal_return_val_if_fail (path != NULL, NULL);
  ctk_internal_return_val_if_fail (path->elems->len != 0, NULL);

  if (pos < 0 || pos >= path->elems->len)
    pos = path->elems->len - 1;

  elem = &g_array_index (path->elems, CtkPathElement, pos);

  wrong_list = ctk_css_node_declaration_list_regions (elem->decl);
  for (l = wrong_list; l; l = l->next)
    {
      list = g_slist_prepend (list, (char *) g_quark_to_string (GPOINTER_TO_UINT (l->data)));
    }
  g_list_free (wrong_list);

  return list;
}

/**
 * ctk_widget_path_iter_has_qregion:
 * @path: a #CtkWidgetPath
 * @pos: position to query, -1 for the path head
 * @qname: region name as a #GQuark
 * @flags: (out): return location for the region flags
 *
 * See ctk_widget_path_iter_has_region(). This is a version that operates
 * with GQuarks.
 *
 * Returns: %TRUE if the widget at @pos has the region defined.
 *
 * Since: 3.0
 *
 * Deprecated: 3.14: The use of regions is deprecated.
 **/
gboolean
ctk_widget_path_iter_has_qregion (const CtkWidgetPath *path,
                                  gint                 pos,
                                  GQuark               qname,
                                  CtkRegionFlags      *flags)
{
  CtkPathElement *elem;

  ctk_internal_return_val_if_fail (path != NULL, FALSE);
  ctk_internal_return_val_if_fail (path->elems->len != 0, FALSE);
  ctk_internal_return_val_if_fail (qname != 0, FALSE);

  if (pos < 0 || pos >= path->elems->len)
    pos = path->elems->len - 1;

  elem = &g_array_index (path->elems, CtkPathElement, pos);

  return ctk_css_node_declaration_has_region (elem->decl, qname, flags);
}

/**
 * ctk_widget_path_iter_has_region:
 * @path: a #CtkWidgetPath
 * @pos: position to query, -1 for the path head
 * @name: region name
 * @flags: (out): return location for the region flags
 *
 * Returns %TRUE if the widget at position @pos has the class @name
 * defined, %FALSE otherwise.
 *
 * Returns: %TRUE if the class @name is defined for the widget at @pos
 *
 * Since: 3.0
 *
 * Deprecated: 3.14: The use of regions is deprecated.
 **/
gboolean
ctk_widget_path_iter_has_region (const CtkWidgetPath *path,
                                 gint                 pos,
                                 const gchar         *name,
                                 CtkRegionFlags      *flags)
{
  GQuark qname;

  ctk_internal_return_val_if_fail (path != NULL, FALSE);
  ctk_internal_return_val_if_fail (path->elems->len != 0, FALSE);
  ctk_internal_return_val_if_fail (name != NULL, FALSE);

  if (pos < 0 || pos >= path->elems->len)
    pos = path->elems->len - 1;

  qname = g_quark_try_string (name);

  if (qname == 0)
    return FALSE;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  return ctk_widget_path_iter_has_qregion (path, pos, qname, flags);
G_GNUC_END_IGNORE_DEPRECATIONS
}

/**
 * ctk_widget_path_get_object_type:
 * @path: a #CtkWidget
 *
 * Returns the topmost object type, that is, the object type this path
 * is representing.
 *
 * Returns: The object type
 *
 * Since: 3.0
 **/
GType
ctk_widget_path_get_object_type (const CtkWidgetPath *path)
{
  CtkPathElement *elem;

  ctk_internal_return_val_if_fail (path != NULL, G_TYPE_INVALID);

  elem = &g_array_index (path->elems, CtkPathElement,
                         path->elems->len - 1);
  return ctk_css_node_declaration_get_type (elem->decl);
}

/**
 * ctk_widget_path_is_type:
 * @path: a #CtkWidgetPath
 * @type: widget type to match
 *
 * Returns %TRUE if the widget type represented by this path
 * is @type, or a subtype of it.
 *
 * Returns: %TRUE if the widget represented by @path is of type @type
 *
 * Since: 3.0
 **/
gboolean
ctk_widget_path_is_type (const CtkWidgetPath *path,
                         GType                type)
{
  CtkPathElement *elem;

  ctk_internal_return_val_if_fail (path != NULL, FALSE);

  elem = &g_array_index (path->elems, CtkPathElement,
                         path->elems->len - 1);

  return g_type_is_a (ctk_css_node_declaration_get_type (elem->decl), type);
}

/**
 * ctk_widget_path_has_parent:
 * @path: a #CtkWidgetPath
 * @type: widget type to check in parents
 *
 * Returns %TRUE if any of the parents of the widget represented
 * in @path is of type @type, or any subtype of it.
 *
 * Returns: %TRUE if any parent is of type @type
 *
 * Since: 3.0
 **/
gboolean
ctk_widget_path_has_parent (const CtkWidgetPath *path,
                            GType                type)
{
  guint i;

  ctk_internal_return_val_if_fail (path != NULL, FALSE);

  for (i = 0; i < path->elems->len - 1; i++)
    {
      CtkPathElement *elem;

      elem = &g_array_index (path->elems, CtkPathElement, i);

      if (g_type_is_a (ctk_css_node_declaration_get_type (elem->decl), type))
        return TRUE;
    }

  return FALSE;
}
