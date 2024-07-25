/* ctkbuildable.c
 * Copyright (C) 2006-2007 Async Open Source,
 *                         Johan Dahlin <jdahlin@async.com.br>
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

/**
 * SECTION:ctkbuildable
 * @Short_description: Interface for objects that can be built by CtkBuilder
 * @Title: CtkBuildable
 *
 * CtkBuildable allows objects to extend and customize their deserialization
 * from [CtkBuilder UI descriptions][BUILDER-UI].
 * The interface includes methods for setting names and properties of objects, 
 * parsing custom tags and constructing child objects.
 *
 * The CtkBuildable interface is implemented by all widgets and
 * many of the non-widget objects that are provided by CTK+. The
 * main user of this interface is #CtkBuilder. There should be
 * very little need for applications to call any of these functions directly.
 *
 * An object only needs to implement this interface if it needs to extend the
 * #CtkBuilder format or run any extra routines at deserialization time.
 */

#include "config.h"
#include "ctkbuildable.h"
#include "ctkintl.h"


typedef CtkBuildableIface CtkBuildableInterface;
G_DEFINE_INTERFACE (CtkBuildable, ctk_buildable, G_TYPE_OBJECT)

static void
ctk_buildable_default_init (CtkBuildableInterface *iface G_GNUC_UNUSED)
{
}

/**
 * ctk_buildable_set_name:
 * @buildable: a #CtkBuildable
 * @name: name to set
 *
 * Sets the name of the @buildable object.
 *
 * Since: 2.12
 **/
void
ctk_buildable_set_name (CtkBuildable *buildable,
                        const gchar  *name)
{
  CtkBuildableIface *iface;

  g_return_if_fail (CTK_IS_BUILDABLE (buildable));
  g_return_if_fail (name != NULL);

  iface = CTK_BUILDABLE_GET_IFACE (buildable);

  if (iface->set_name)
    (* iface->set_name) (buildable, name);
  else
    g_object_set_data_full (G_OBJECT (buildable),
			    "ctk-builder-name",
			    g_strdup (name),
			    g_free);
}

/**
 * ctk_buildable_get_name:
 * @buildable: a #CtkBuildable
 *
 * Gets the name of the @buildable object. 
 * 
 * #CtkBuilder sets the name based on the
 * [CtkBuilder UI definition][BUILDER-UI] 
 * used to construct the @buildable.
 *
 * Returns: the name set with ctk_buildable_set_name()
 *
 * Since: 2.12
 **/
const gchar *
ctk_buildable_get_name (CtkBuildable *buildable)
{
  CtkBuildableIface *iface;

  g_return_val_if_fail (CTK_IS_BUILDABLE (buildable), NULL);

  iface = CTK_BUILDABLE_GET_IFACE (buildable);

  if (iface->get_name)
    return (* iface->get_name) (buildable);
  else
    return (const gchar*)g_object_get_data (G_OBJECT (buildable),
					    "ctk-builder-name");
}

/**
 * ctk_buildable_add_child:
 * @buildable: a #CtkBuildable
 * @builder: a #CtkBuilder
 * @child: child to add
 * @type: (allow-none): kind of child or %NULL
 *
 * Adds a child to @buildable. @type is an optional string
 * describing how the child should be added.
 *
 * Since: 2.12
 **/
void
ctk_buildable_add_child (CtkBuildable *buildable,
			 CtkBuilder   *builder,
			 GObject      *child,
			 const gchar  *type)
{
  CtkBuildableIface *iface;

  g_return_if_fail (CTK_IS_BUILDABLE (buildable));
  g_return_if_fail (CTK_IS_BUILDER (builder));

  iface = CTK_BUILDABLE_GET_IFACE (buildable);
  g_return_if_fail (iface->add_child != NULL);

  (* iface->add_child) (buildable, builder, child, type);
}

/**
 * ctk_buildable_set_buildable_property:
 * @buildable: a #CtkBuildable
 * @builder: a #CtkBuilder
 * @name: name of property
 * @value: value of property
 *
 * Sets the property name @name to @value on the @buildable object.
 *
 * Since: 2.12
 **/
void
ctk_buildable_set_buildable_property (CtkBuildable *buildable,
				      CtkBuilder   *builder,
				      const gchar  *name,
				      const GValue *value)
{
  CtkBuildableIface *iface;

  g_return_if_fail (CTK_IS_BUILDABLE (buildable));
  g_return_if_fail (CTK_IS_BUILDER (builder));
  g_return_if_fail (name != NULL);
  g_return_if_fail (value != NULL);

  iface = CTK_BUILDABLE_GET_IFACE (buildable);
  if (iface->set_buildable_property)
    (* iface->set_buildable_property) (buildable, builder, name, value);
  else
    g_object_set_property (G_OBJECT (buildable), name, value);
}

/**
 * ctk_buildable_parser_finished:
 * @buildable: a #CtkBuildable
 * @builder: a #CtkBuilder
 *
 * Called when the builder finishes the parsing of a 
 * [CtkBuilder UI definition][BUILDER-UI]. 
 * Note that this will be called once for each time 
 * ctk_builder_add_from_file() or ctk_builder_add_from_string() 
 * is called on a builder.
 *
 * Since: 2.12
 **/
void
ctk_buildable_parser_finished (CtkBuildable *buildable,
			       CtkBuilder   *builder)
{
  CtkBuildableIface *iface;

  g_return_if_fail (CTK_IS_BUILDABLE (buildable));
  g_return_if_fail (CTK_IS_BUILDER (builder));

  iface = CTK_BUILDABLE_GET_IFACE (buildable);
  if (iface->parser_finished)
    (* iface->parser_finished) (buildable, builder);
}

/**
 * ctk_buildable_construct_child:
 * @buildable: A #CtkBuildable
 * @builder: #CtkBuilder used to construct this object
 * @name: name of child to construct
 *
 * Constructs a child of @buildable with the name @name.
 *
 * #CtkBuilder calls this function if a “constructor” has been
 * specified in the UI definition.
 *
 * Returns: (transfer full): the constructed child
 *
 * Since: 2.12
 **/
GObject *
ctk_buildable_construct_child (CtkBuildable *buildable,
                               CtkBuilder   *builder,
                               const gchar  *name)
{
  CtkBuildableIface *iface;

  g_return_val_if_fail (CTK_IS_BUILDABLE (buildable), NULL);
  g_return_val_if_fail (CTK_IS_BUILDER (builder), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  iface = CTK_BUILDABLE_GET_IFACE (buildable);
  g_return_val_if_fail (iface->construct_child != NULL, NULL);

  return (* iface->construct_child) (buildable, builder, name);
}

/**
 * ctk_buildable_custom_tag_start:
 * @buildable: a #CtkBuildable
 * @builder: a #CtkBuilder used to construct this object
 * @child: (allow-none): child object or %NULL for non-child tags
 * @tagname: name of tag
 * @parser: (out): a #GMarkupParser to fill in
 * @data: (out): return location for user data that will be passed in 
 *   to parser functions
 *
 * This is called for each unknown element under <child>.
 * 
 * Returns: %TRUE if a object has a custom implementation, %FALSE
 *          if it doesn't.
 *
 * Since: 2.12
 **/
gboolean
ctk_buildable_custom_tag_start (CtkBuildable  *buildable,
                                CtkBuilder    *builder,
                                GObject       *child,
                                const gchar   *tagname,
                                GMarkupParser *parser,
                                gpointer      *data)
{
  CtkBuildableIface *iface;

  g_return_val_if_fail (CTK_IS_BUILDABLE (buildable), FALSE);
  g_return_val_if_fail (CTK_IS_BUILDER (builder), FALSE);
  g_return_val_if_fail (tagname != NULL, FALSE);

  iface = CTK_BUILDABLE_GET_IFACE (buildable);
  g_return_val_if_fail (iface->custom_tag_start != NULL, FALSE);

  return (* iface->custom_tag_start) (buildable, builder, child,
                                      tagname, parser, data);
}

/**
 * ctk_buildable_custom_tag_end:
 * @buildable: A #CtkBuildable
 * @builder: #CtkBuilder used to construct this object
 * @child: (allow-none): child object or %NULL for non-child tags
 * @tagname: name of tag
 * @data: (type gpointer): user data that will be passed in to parser functions
 *
 * This is called at the end of each custom element handled by 
 * the buildable.
 *
 * Since: 2.12
 **/
void
ctk_buildable_custom_tag_end (CtkBuildable  *buildable,
                              CtkBuilder    *builder,
                              GObject       *child,
                              const gchar   *tagname,
                              gpointer      *data)
{
  CtkBuildableIface *iface;

  g_return_if_fail (CTK_IS_BUILDABLE (buildable));
  g_return_if_fail (CTK_IS_BUILDER (builder));
  g_return_if_fail (tagname != NULL);

  iface = CTK_BUILDABLE_GET_IFACE (buildable);
  if (iface->custom_tag_end)
    (* iface->custom_tag_end) (buildable, builder, child, tagname, data);
}

/**
 * ctk_buildable_custom_finished:
 * @buildable: a #CtkBuildable
 * @builder: a #CtkBuilder
 * @child: (allow-none): child object or %NULL for non-child tags
 * @tagname: the name of the tag
 * @data: user data created in custom_tag_start
 *
 * This is similar to ctk_buildable_parser_finished() but is
 * called once for each custom tag handled by the @buildable.
 * 
 * Since: 2.12
 **/
void
ctk_buildable_custom_finished (CtkBuildable  *buildable,
			       CtkBuilder    *builder,
			       GObject       *child,
			       const gchar   *tagname,
			       gpointer       data)
{
  CtkBuildableIface *iface;

  g_return_if_fail (CTK_IS_BUILDABLE (buildable));
  g_return_if_fail (CTK_IS_BUILDER (builder));

  iface = CTK_BUILDABLE_GET_IFACE (buildable);
  if (iface->custom_finished)
    (* iface->custom_finished) (buildable, builder, child, tagname, data);
}

/**
 * ctk_buildable_get_internal_child:
 * @buildable: a #CtkBuildable
 * @builder: a #CtkBuilder
 * @childname: name of child
 *
 * Get the internal child called @childname of the @buildable object.
 *
 * Returns: (transfer none): the internal child of the buildable object
 *
 * Since: 2.12
 **/
GObject *
ctk_buildable_get_internal_child (CtkBuildable *buildable,
                                  CtkBuilder   *builder,
                                  const gchar  *childname)
{
  CtkBuildableIface *iface;

  g_return_val_if_fail (CTK_IS_BUILDABLE (buildable), NULL);
  g_return_val_if_fail (CTK_IS_BUILDER (builder), NULL);
  g_return_val_if_fail (childname != NULL, NULL);

  iface = CTK_BUILDABLE_GET_IFACE (buildable);
  if (!iface->get_internal_child)
    return NULL;

  return (* iface->get_internal_child) (buildable, builder, childname);
}
