/* CTK - The GIMP Toolkit
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

#ifndef __CTK_BUILDABLE_H__
#define __CTK_BUILDABLE_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkbuilder.h>

G_BEGIN_DECLS

#define CTK_TYPE_BUILDABLE            (ctk_buildable_get_type ())
#define CTK_BUILDABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_BUILDABLE, CtkBuildable))
#define CTK_BUILDABLE_CLASS(obj)      (G_TYPE_CHECK_CLASS_CAST ((obj), CTK_TYPE_BUILDABLE, CtkBuildableIface))
#define CTK_IS_BUILDABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_BUILDABLE))
#define CTK_BUILDABLE_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), CTK_TYPE_BUILDABLE, CtkBuildableIface))


typedef struct _CtkBuildable      CtkBuildable; /* Dummy typedef */
typedef struct _CtkBuildableIface CtkBuildableIface;

/**
 * CtkBuildableIface:
 * @g_iface: the parent class
 * @set_name: Stores the name attribute given in the CtkBuilder UI definition.
 *  #CtkWidget stores the name as object data. Implement this method if your
 *  object has some notion of “name” and it makes sense to map the XML name
 *  attribute to it.
 * @get_name: The getter corresponding to @set_name. Implement this
 *  if you implement @set_name.
 * @add_child: Adds a child. The @type parameter can be used to
 *  differentiate the kind of child. #CtkContainer implements this
 *  to add add a child widget to the container, #CtkNotebook uses
 *  the @type to distinguish between page labels (of type "page-label")
 *  and normal children.
 * @set_buildable_property: Sets a property of a buildable object.
 *  It is normally not necessary to implement this, g_object_set_property()
 *  is used by default. #CtkWindow implements this to delay showing itself
 *  (i.e. setting the #CtkWidget:visible property) until the whole interface
 *  is created.
 * @construct_child: Constructs a child of a buildable that has been
 *  specified as “constructor” in the UI definition. #CtkUIManager implements
 *  this to reference to a widget created in a <ui> tag which is outside
 *  of the normal CtkBuilder UI definition hierarchy.  A reference to the
 *  constructed object is returned and becomes owned by the caller.
 * @custom_tag_start: Implement this if the buildable needs to parse
 *  content below <child>. To handle an element, the implementation
 *  must fill in the @parser and @user_data and return %TRUE.
 *  #CtkWidget implements this to parse keyboard accelerators specified
 *  in <accelerator> elements. #CtkContainer implements it to map
 *  properties defined via <packing> elements to child properties.
 *  Note that @user_data must be freed in @custom_tag_end or @custom_finished.
 * @custom_tag_end: Called for the end tag of each custom element that is
 *  handled by the buildable (see @custom_tag_start).
 * @custom_finished: Called for each custom tag handled by the buildable
 *  when the builder finishes parsing (see @custom_tag_start)
 * @parser_finished: Called when a builder finishes the parsing
 *  of a UI definition. It is normally not necessary to implement this,
 *  unless you need to perform special cleanup actions. #CtkWindow sets
 *  the #CtkWidget:visible property here.
 * @get_internal_child: Returns an internal child of a buildable.
 *  #CtkDialog implements this to give access to its @vbox, making
 *  it possible to add children to the vbox in a UI definition.
 *  Implement this if the buildable has internal children that may
 *  need to be accessed from a UI definition.
 *
 * The #CtkBuildableIface interface contains method that are
 * necessary to allow #CtkBuilder to construct an object from
 * a #CtkBuilder UI definition.
 */
struct _CtkBuildableIface
{
  GTypeInterface g_iface;

  /* virtual table */
  void          (* set_name)               (CtkBuildable  *buildable,
                                            const gchar   *name);
  const gchar * (* get_name)               (CtkBuildable  *buildable);
  void          (* add_child)              (CtkBuildable  *buildable,
					    CtkBuilder    *builder,
					    GObject       *child,
					    const gchar   *type);
  void          (* set_buildable_property) (CtkBuildable  *buildable,
					    CtkBuilder    *builder,
					    const gchar   *name,
					    const GValue  *value);
  GObject *     (* construct_child)        (CtkBuildable  *buildable,
					    CtkBuilder    *builder,
					    const gchar   *name);
  gboolean      (* custom_tag_start)       (CtkBuildable  *buildable,
					    CtkBuilder    *builder,
					    GObject       *child,
					    const gchar   *tagname,
					    GMarkupParser *parser,
					    gpointer      *data);
  void          (* custom_tag_end)         (CtkBuildable  *buildable,
					    CtkBuilder    *builder,
					    GObject       *child,
					    const gchar   *tagname,
					    gpointer      *data);
  void          (* custom_finished)        (CtkBuildable  *buildable,
					    CtkBuilder    *builder,
					    GObject       *child,
					    const gchar   *tagname,
					    gpointer       data);
  void          (* parser_finished)        (CtkBuildable  *buildable,
					    CtkBuilder    *builder);

  GObject *     (* get_internal_child)     (CtkBuildable  *buildable,
					    CtkBuilder    *builder,
					    const gchar   *childname);
};


CDK_AVAILABLE_IN_ALL
GType     ctk_buildable_get_type               (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_ALL
void      ctk_buildable_set_name               (CtkBuildable        *buildable,
						const gchar         *name);
CDK_AVAILABLE_IN_ALL
const gchar * ctk_buildable_get_name           (CtkBuildable        *buildable);
CDK_AVAILABLE_IN_ALL
void      ctk_buildable_add_child              (CtkBuildable        *buildable,
						CtkBuilder          *builder,
						GObject             *child,
						const gchar         *type);
CDK_AVAILABLE_IN_ALL
void      ctk_buildable_set_buildable_property (CtkBuildable        *buildable,
						CtkBuilder          *builder,
						const gchar         *name,
						const GValue        *value);
CDK_AVAILABLE_IN_ALL
GObject * ctk_buildable_construct_child        (CtkBuildable        *buildable,
						CtkBuilder          *builder,
						const gchar         *name);
CDK_AVAILABLE_IN_ALL
gboolean  ctk_buildable_custom_tag_start       (CtkBuildable        *buildable,
						CtkBuilder          *builder,
						GObject             *child,
						const gchar         *tagname,
						GMarkupParser       *parser,
						gpointer            *data);
CDK_AVAILABLE_IN_ALL
void      ctk_buildable_custom_tag_end         (CtkBuildable        *buildable,
						CtkBuilder          *builder,
						GObject             *child,
						const gchar         *tagname,
						gpointer            *data);
CDK_AVAILABLE_IN_ALL
void      ctk_buildable_custom_finished        (CtkBuildable        *buildable,
						CtkBuilder          *builder,
						GObject             *child,
						const gchar         *tagname,
						gpointer             data);
CDK_AVAILABLE_IN_ALL
void      ctk_buildable_parser_finished        (CtkBuildable        *buildable,
						CtkBuilder          *builder);
CDK_AVAILABLE_IN_ALL
GObject * ctk_buildable_get_internal_child     (CtkBuildable        *buildable,
						CtkBuilder          *builder,
						const gchar         *childname);

G_END_DECLS

#endif /* __CTK_BUILDABLE_H__ */
