/*
 * Copyright © 2011 Red Hat Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#ifndef __CTK_CSS_SHORTHAND_PROPERTY_PRIVATE_H__
#define __CTK_CSS_SHORTHAND_PROPERTY_PRIVATE_H__

#include <glib-object.h>

#include "ctk/ctkcssparserprivate.h"
#include "ctk/ctkcssstylepropertyprivate.h"
#include "ctk/ctkstylepropertyprivate.h"

G_BEGIN_DECLS

#define CTK_TYPE_CSS_SHORTHAND_PROPERTY           (_ctk_css_shorthand_property_get_type ())
#define CTK_CSS_SHORTHAND_PROPERTY(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, CTK_TYPE_CSS_SHORTHAND_PROPERTY, CtkCssShorthandProperty))
#define CTK_CSS_SHORTHAND_PROPERTY_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, CTK_TYPE_CSS_SHORTHAND_PROPERTY, CtkCssShorthandPropertyClass))
#define CTK_IS_CSS_SHORTHAND_PROPERTY(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, CTK_TYPE_CSS_SHORTHAND_PROPERTY))
#define CTK_IS_CSS_SHORTHAND_PROPERTY_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, CTK_TYPE_CSS_SHORTHAND_PROPERTY))
#define CTK_CSS_SHORTHAND_PROPERTY_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CSS_SHORTHAND_PROPERTY, CtkCssShorthandPropertyClass))

typedef struct _CtkCssShorthandProperty           CtkCssShorthandProperty;
typedef struct _CtkCssShorthandPropertyClass      CtkCssShorthandPropertyClass;

typedef gboolean              (* CtkCssShorthandPropertyParseFunc)      (CtkCssShorthandProperty *shorthand,
                                                                         CtkCssValue            **values,
                                                                         CtkCssParser            *parser);
typedef void                  (* CtkCssShorthandPropertyAssignFunc)     (CtkCssShorthandProperty *shorthand,
                                                                         CtkStyleProperties      *props,
                                                                         CtkStateFlags            state,
                                                                         const GValue            *value);
typedef void                  (* CtkCssShorthandPropertyQueryFunc)      (CtkCssShorthandProperty *shorthand,
                                                                         GValue                  *value,
                                                                         CtkStyleQueryFunc        query_func,
                                                                         gpointer                 query_data);

struct _CtkCssShorthandProperty
{
  CtkStyleProperty parent;

  GPtrArray *subproperties;

  CtkCssShorthandPropertyParseFunc parse;
  CtkCssShorthandPropertyAssignFunc assign;
  CtkCssShorthandPropertyQueryFunc query;
};

struct _CtkCssShorthandPropertyClass
{
  CtkStylePropertyClass parent_class;
};

void                    _ctk_css_shorthand_property_init_properties     (void);

GType                   _ctk_css_shorthand_property_get_type            (void) G_GNUC_CONST;

CtkCssStyleProperty *   _ctk_css_shorthand_property_get_subproperty     (CtkCssShorthandProperty *shorthand,
                                                                         guint                    property);
guint                   _ctk_css_shorthand_property_get_n_subproperties (CtkCssShorthandProperty *shorthand);


G_END_DECLS

#endif /* __CTK_CSS_SHORTHAND_PROPERTY_PRIVATE_H__ */
