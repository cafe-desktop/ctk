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

#ifndef __CTK_BUILDER_H__
#define __CTK_BUILDER_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkapplication.h>
#include <ctk/ctkwidget.h>

G_BEGIN_DECLS

#define CTK_TYPE_BUILDER                 (ctk_builder_get_type ())
#define CTK_BUILDER(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_BUILDER, CtkBuilder))
#define CTK_BUILDER_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_BUILDER, CtkBuilderClass))
#define CTK_IS_BUILDER(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_BUILDER))
#define CTK_IS_BUILDER_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_BUILDER))
#define CTK_BUILDER_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_BUILDER, CtkBuilderClass))

#define CTK_BUILDER_ERROR                (ctk_builder_error_quark ())

typedef struct _CtkBuilderClass   CtkBuilderClass;
typedef struct _CtkBuilderPrivate CtkBuilderPrivate;

/**
 * CtkBuilderError:
 * @CTK_BUILDER_ERROR_INVALID_TYPE_FUNCTION: A type-func attribute didn’t name
 *  a function that returns a #GType.
 * @CTK_BUILDER_ERROR_UNHANDLED_TAG: The input contained a tag that #CtkBuilder
 *  can’t handle.
 * @CTK_BUILDER_ERROR_MISSING_ATTRIBUTE: An attribute that is required by
 *  #CtkBuilder was missing.
 * @CTK_BUILDER_ERROR_INVALID_ATTRIBUTE: #CtkBuilder found an attribute that
 *  it doesn’t understand.
 * @CTK_BUILDER_ERROR_INVALID_TAG: #CtkBuilder found a tag that
 *  it doesn’t understand.
 * @CTK_BUILDER_ERROR_MISSING_PROPERTY_VALUE: A required property value was
 *  missing.
 * @CTK_BUILDER_ERROR_INVALID_VALUE: #CtkBuilder couldn’t parse
 *  some attribute value.
 * @CTK_BUILDER_ERROR_VERSION_MISMATCH: The input file requires a newer version
 *  of CTK+.
 * @CTK_BUILDER_ERROR_DUPLICATE_ID: An object id occurred twice.
 * @CTK_BUILDER_ERROR_OBJECT_TYPE_REFUSED: A specified object type is of the same type or
 *  derived from the type of the composite class being extended with builder XML.
 * @CTK_BUILDER_ERROR_TEMPLATE_MISMATCH: The wrong type was specified in a composite class’s template XML
 * @CTK_BUILDER_ERROR_INVALID_PROPERTY: The specified property is unknown for the object class.
 * @CTK_BUILDER_ERROR_INVALID_SIGNAL: The specified signal is unknown for the object class.
 * @CTK_BUILDER_ERROR_INVALID_ID: An object id is unknown
 *
 * Error codes that identify various errors that can occur while using
 * #CtkBuilder.
 */
typedef enum
{
  CTK_BUILDER_ERROR_INVALID_TYPE_FUNCTION,
  CTK_BUILDER_ERROR_UNHANDLED_TAG,
  CTK_BUILDER_ERROR_MISSING_ATTRIBUTE,
  CTK_BUILDER_ERROR_INVALID_ATTRIBUTE,
  CTK_BUILDER_ERROR_INVALID_TAG,
  CTK_BUILDER_ERROR_MISSING_PROPERTY_VALUE,
  CTK_BUILDER_ERROR_INVALID_VALUE,
  CTK_BUILDER_ERROR_VERSION_MISMATCH,
  CTK_BUILDER_ERROR_DUPLICATE_ID,
  CTK_BUILDER_ERROR_OBJECT_TYPE_REFUSED,
  CTK_BUILDER_ERROR_TEMPLATE_MISMATCH,
  CTK_BUILDER_ERROR_INVALID_PROPERTY,
  CTK_BUILDER_ERROR_INVALID_SIGNAL,
  CTK_BUILDER_ERROR_INVALID_ID
} CtkBuilderError;

GDK_AVAILABLE_IN_ALL
GQuark ctk_builder_error_quark (void);

struct _CtkBuilder
{
  GObject parent_instance;

  CtkBuilderPrivate *priv;
};

struct _CtkBuilderClass
{
  GObjectClass parent_class;
  
  GType (* get_type_from_name) (CtkBuilder *builder,
                                const char *type_name);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
  void (*_ctk_reserved5) (void);
  void (*_ctk_reserved6) (void);
  void (*_ctk_reserved7) (void);
  void (*_ctk_reserved8) (void);
};

GDK_AVAILABLE_IN_ALL
GType        ctk_builder_get_type                (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
CtkBuilder*  ctk_builder_new                     (void);

GDK_AVAILABLE_IN_ALL
guint        ctk_builder_add_from_file           (CtkBuilder    *builder,
                                                  const gchar   *filename,
                                                  GError       **error);
GDK_AVAILABLE_IN_ALL
guint        ctk_builder_add_from_resource       (CtkBuilder    *builder,
                                                  const gchar   *resource_path,
                                                  GError       **error);
GDK_AVAILABLE_IN_ALL
guint        ctk_builder_add_from_string         (CtkBuilder    *builder,
                                                  const gchar   *buffer,
                                                  gsize          length,
                                                  GError       **error);
GDK_AVAILABLE_IN_ALL
guint        ctk_builder_add_objects_from_file   (CtkBuilder    *builder,
                                                  const gchar   *filename,
                                                  gchar        **object_ids,
                                                  GError       **error);
GDK_AVAILABLE_IN_3_4
guint        ctk_builder_add_objects_from_resource(CtkBuilder    *builder,
                                                  const gchar   *resource_path,
                                                  gchar        **object_ids,
                                                  GError       **error);
GDK_AVAILABLE_IN_ALL
guint        ctk_builder_add_objects_from_string (CtkBuilder    *builder,
                                                  const gchar   *buffer,
                                                  gsize          length,
                                                  gchar        **object_ids,
                                                  GError       **error);
GDK_AVAILABLE_IN_ALL
GObject*     ctk_builder_get_object              (CtkBuilder    *builder,
                                                  const gchar   *name);
GDK_AVAILABLE_IN_ALL
GSList*      ctk_builder_get_objects             (CtkBuilder    *builder);
GDK_AVAILABLE_IN_3_8
void         ctk_builder_expose_object           (CtkBuilder    *builder,
                                                  const gchar   *name,
                                                  GObject       *object);
GDK_AVAILABLE_IN_ALL
void         ctk_builder_connect_signals         (CtkBuilder    *builder,
						  gpointer       user_data);
GDK_AVAILABLE_IN_ALL
void         ctk_builder_connect_signals_full    (CtkBuilder    *builder,
                                                  CtkBuilderConnectFunc func,
						  gpointer       user_data);
GDK_AVAILABLE_IN_ALL
void         ctk_builder_set_translation_domain  (CtkBuilder   	*builder,
                                                  const gchar  	*domain);
GDK_AVAILABLE_IN_ALL
const gchar* ctk_builder_get_translation_domain  (CtkBuilder   	*builder);
GDK_AVAILABLE_IN_ALL
GType        ctk_builder_get_type_from_name      (CtkBuilder   	*builder,
                                                  const char   	*type_name);

GDK_AVAILABLE_IN_ALL
gboolean     ctk_builder_value_from_string       (CtkBuilder    *builder,
						  GParamSpec   	*pspec,
                                                  const gchar  	*string,
                                                  GValue       	*value,
						  GError       **error);
GDK_AVAILABLE_IN_ALL
gboolean     ctk_builder_value_from_string_type  (CtkBuilder    *builder,
						  GType        	 type,
                                                  const gchar  	*string,
                                                  GValue       	*value,
						  GError       **error);
GDK_AVAILABLE_IN_3_10
CtkBuilder * ctk_builder_new_from_file           (const gchar   *filename);
GDK_AVAILABLE_IN_3_10
CtkBuilder * ctk_builder_new_from_resource       (const gchar   *resource_path);
GDK_AVAILABLE_IN_3_10
CtkBuilder * ctk_builder_new_from_string         (const gchar   *string,
                                                  gssize         length);

GDK_AVAILABLE_IN_3_10
void         ctk_builder_add_callback_symbol     (CtkBuilder    *builder,
						  const gchar   *callback_name,
						  GCallback      callback_symbol);
GDK_AVAILABLE_IN_3_10
void         ctk_builder_add_callback_symbols    (CtkBuilder    *builder,
						  const gchar   *first_callback_name,
						  GCallback      first_callback_symbol,
						  ...) G_GNUC_NULL_TERMINATED;
GDK_AVAILABLE_IN_3_10
GCallback    ctk_builder_lookup_callback_symbol  (CtkBuilder    *builder,
						  const gchar   *callback_name);

GDK_AVAILABLE_IN_3_12
void         ctk_builder_set_application         (CtkBuilder     *builder,
                                                  CtkApplication *application);

GDK_AVAILABLE_IN_3_12
CtkApplication * ctk_builder_get_application     (CtkBuilder     *builder);


/**
 * CTK_BUILDER_WARN_INVALID_CHILD_TYPE:
 * @object: the #CtkBuildable on which the warning ocurred
 * @type: the unexpected type value
 *
 * This macro should be used to emit a warning about and unexpected @type value
 * in a #CtkBuildable add_child implementation.
 */
#define CTK_BUILDER_WARN_INVALID_CHILD_TYPE(object, type) \
  g_warning ("'%s' is not a valid child type of '%s'", type, g_type_name (G_OBJECT_TYPE (object)))

GDK_AVAILABLE_IN_3_18
guint     ctk_builder_extend_with_template  (CtkBuilder    *builder,
                                             CtkWidget     *widget,
                                             GType          template_type,                                                          const gchar   *buffer,
                                             gsize          length,
                                             GError       **error);

G_END_DECLS

#endif /* __CTK_BUILDER_H__ */
