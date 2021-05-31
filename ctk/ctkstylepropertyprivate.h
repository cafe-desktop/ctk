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

#include "ctkcssparserprivate.h"
#include "ctkstylecontextprivate.h"
#include "ctkcssvalueprivate.h"

G_BEGIN_DECLS

#define CTK_TYPE_STYLE_PROPERTY           (_ctk_style_property_get_type ())
#define CTK_STYLE_PROPERTY(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, CTK_TYPE_STYLE_PROPERTY, CtkStyleProperty))
#define CTK_STYLE_PROPERTY_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, CTK_TYPE_STYLE_PROPERTY, CtkStylePropertyClass))
#define CTK_IS_STYLE_PROPERTY(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, CTK_TYPE_STYLE_PROPERTY))
#define CTK_IS_STYLE_PROPERTY_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, CTK_TYPE_STYLE_PROPERTY))
#define CTK_STYLE_PROPERTY_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_STYLE_PROPERTY, CtkStylePropertyClass))

typedef struct _CtkStyleProperty           CtkStyleProperty;
typedef struct _CtkStylePropertyClass      CtkStylePropertyClass;

typedef CtkCssValue *   (* CtkStyleQueryFunc)        (guint                   id,
						      gpointer                data);

struct _CtkStyleProperty
{
  GObject parent;

  char *name;
  GType value_type;
};

struct _CtkStylePropertyClass
{
  GObjectClass  parent_class;
  
  void              (* assign)                             (CtkStyleProperty       *property,
                                                            CtkStyleProperties     *props,
                                                            CtkStateFlags           state,
                                                            const GValue           *value);
  void              (* query)                              (CtkStyleProperty       *property,
                                                            GValue                 *value,
                                                            CtkStyleQueryFunc       query_func,
                                                            gpointer                query_data);
  CtkCssValue *     (* parse_value)                        (CtkStyleProperty *      property,
                                                            CtkCssParser           *parser);

  GHashTable   *properties;
};

GType               _ctk_style_property_get_type             (void) G_GNUC_CONST;

void                _ctk_style_property_init_properties      (void);

void                _ctk_style_property_add_alias (const gchar *name,
                                                   const gchar *alias);

CtkStyleProperty *       _ctk_style_property_lookup        (const char             *name);

const char *             _ctk_style_property_get_name      (CtkStyleProperty       *property);

CtkCssValue *            _ctk_style_property_parse_value   (CtkStyleProperty *      property,
                                                            CtkCssParser           *parser);

GType                    _ctk_style_property_get_value_type(CtkStyleProperty *      property);
void                     _ctk_style_property_query         (CtkStyleProperty *      property,
                                                            GValue                 *value,
                                                            CtkStyleQueryFunc       query_func,
                                                            gpointer                query_data);
void                     _ctk_style_property_assign        (CtkStyleProperty       *property,
                                                            CtkStyleProperties     *props,
                                                            CtkStateFlags           state,
                                                            const GValue           *value);

G_END_DECLS

#endif /* __CTK_CSS_STYLEPROPERTY_PRIVATE_H__ */
