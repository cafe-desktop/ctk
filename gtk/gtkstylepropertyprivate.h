/* GTK - The GIMP Toolkit
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

#ifndef __CTK_STYLEPROPERTY_PRIVATE_H__
#define __CTK_STYLEPROPERTY_PRIVATE_H__

#include "gtkcssparserprivate.h"
#include "gtkstylecontextprivate.h"
#include "gtkcssvalueprivate.h"

G_BEGIN_DECLS

#define CTK_TYPE_STYLE_PROPERTY           (_ctk_style_property_get_type ())
#define CTK_STYLE_PROPERTY(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, CTK_TYPE_STYLE_PROPERTY, GtkStyleProperty))
#define CTK_STYLE_PROPERTY_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, CTK_TYPE_STYLE_PROPERTY, GtkStylePropertyClass))
#define CTK_IS_STYLE_PROPERTY(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, CTK_TYPE_STYLE_PROPERTY))
#define CTK_IS_STYLE_PROPERTY_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, CTK_TYPE_STYLE_PROPERTY))
#define CTK_STYLE_PROPERTY_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_STYLE_PROPERTY, GtkStylePropertyClass))

typedef struct _GtkStyleProperty           GtkStyleProperty;
typedef struct _GtkStylePropertyClass      GtkStylePropertyClass;

typedef GtkCssValue *   (* GtkStyleQueryFunc)        (guint                   id,
						      gpointer                data);

struct _GtkStyleProperty
{
  GObject parent;

  char *name;
  GType value_type;
};

struct _GtkStylePropertyClass
{
  GObjectClass  parent_class;
  
  void              (* assign)                             (GtkStyleProperty       *property,
                                                            GtkStyleProperties     *props,
                                                            GtkStateFlags           state,
                                                            const GValue           *value);
  void              (* query)                              (GtkStyleProperty       *property,
                                                            GValue                 *value,
                                                            GtkStyleQueryFunc       query_func,
                                                            gpointer                query_data);
  GtkCssValue *     (* parse_value)                        (GtkStyleProperty *      property,
                                                            GtkCssParser           *parser);

  GHashTable   *properties;
};

GType               _ctk_style_property_get_type             (void) G_GNUC_CONST;

void                _ctk_style_property_init_properties      (void);

void                _ctk_style_property_add_alias (const gchar *name,
                                                   const gchar *alias);

GtkStyleProperty *       _ctk_style_property_lookup        (const char             *name);

const char *             _ctk_style_property_get_name      (GtkStyleProperty       *property);

GtkCssValue *            _ctk_style_property_parse_value   (GtkStyleProperty *      property,
                                                            GtkCssParser           *parser);

GType                    _ctk_style_property_get_value_type(GtkStyleProperty *      property);
void                     _ctk_style_property_query         (GtkStyleProperty *      property,
                                                            GValue                 *value,
                                                            GtkStyleQueryFunc       query_func,
                                                            gpointer                query_data);
void                     _ctk_style_property_assign        (GtkStyleProperty       *property,
                                                            GtkStyleProperties     *props,
                                                            GtkStateFlags           state,
                                                            const GValue           *value);

G_END_DECLS

#endif /* __CTK_CSS_STYLEPROPERTY_PRIVATE_H__ */
