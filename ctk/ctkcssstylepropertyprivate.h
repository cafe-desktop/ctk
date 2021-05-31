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

#ifndef __CTK_CSS_STYLE_PROPERTY_PRIVATE_H__
#define __CTK_CSS_STYLE_PROPERTY_PRIVATE_H__

#include "ctk/ctkstylepropertyprivate.h"

G_BEGIN_DECLS

#define CTK_TYPE_CSS_STYLE_PROPERTY           (_ctk_css_style_property_get_type ())
#define CTK_CSS_STYLE_PROPERTY(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, CTK_TYPE_CSS_STYLE_PROPERTY, CtkCssStyleProperty))
#define CTK_CSS_STYLE_PROPERTY_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, CTK_TYPE_CSS_STYLE_PROPERTY, CtkCssStylePropertyClass))
#define CTK_IS_CSS_STYLE_PROPERTY(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, CTK_TYPE_CSS_STYLE_PROPERTY))
#define CTK_IS_CSS_STYLE_PROPERTY_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, CTK_TYPE_CSS_STYLE_PROPERTY))
#define CTK_CSS_STYLE_PROPERTY_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CSS_STYLE_PROPERTY, CtkCssStylePropertyClass))

typedef struct _CtkCssStyleProperty           CtkCssStyleProperty;
typedef struct _CtkCssStylePropertyClass      CtkCssStylePropertyClass;

typedef CtkCssValue *    (* CtkCssStylePropertyParseFunc)  (CtkCssStyleProperty    *property,
                                                            CtkCssParser           *parser);
typedef void             (* CtkCssStylePropertyQueryFunc)  (CtkCssStyleProperty    *property,
                                                            const CtkCssValue      *cssvalue,
                                                            GValue                 *value);
typedef CtkCssValue *    (* CtkCssStylePropertyAssignFunc) (CtkCssStyleProperty    *property,
                                                            const GValue           *value);
struct _CtkCssStyleProperty
{
  CtkStyleProperty parent;

  CtkCssValue *initial_value;
  guint id;
  CtkCssAffects affects;
  guint inherit :1;
  guint animated :1;

  CtkCssStylePropertyParseFunc parse_value;
  CtkCssStylePropertyQueryFunc query_value;
  CtkCssStylePropertyAssignFunc assign_value;
};

struct _CtkCssStylePropertyClass
{
  CtkStylePropertyClass parent_class;

  GPtrArray *style_properties;
};

GType                   _ctk_css_style_property_get_type        (void) G_GNUC_CONST;

void                    _ctk_css_style_property_init_properties (void);

guint                   _ctk_css_style_property_get_n_properties(void);
CtkCssStyleProperty *   _ctk_css_style_property_lookup_by_id    (guint                   id);

gboolean                _ctk_css_style_property_is_inherit      (CtkCssStyleProperty    *property);
gboolean                _ctk_css_style_property_is_animated     (CtkCssStyleProperty    *property);
CtkCssAffects           _ctk_css_style_property_get_affects     (CtkCssStyleProperty    *property);
gboolean                _ctk_css_style_property_affects_size    (CtkCssStyleProperty    *property);
gboolean                _ctk_css_style_property_affects_font    (CtkCssStyleProperty    *property);
guint                   _ctk_css_style_property_get_id          (CtkCssStyleProperty    *property);
CtkCssValue  *          _ctk_css_style_property_get_initial_value
                                                                (CtkCssStyleProperty    *property);

void                    _ctk_css_style_property_print_value     (CtkCssStyleProperty    *property,
                                                                 CtkCssValue            *value,
                                                                 GString                *string);

CtkBitmask *            _ctk_css_style_property_get_mask_affecting
                                                                (CtkCssAffects           affects);

/* XXX - find a better place for these */
CtkCssValue * ctk_css_font_family_value_parse (CtkCssParser *parser);
CtkCssValue * ctk_css_font_size_value_parse   (CtkCssParser *parser);

G_END_DECLS

#endif /* __CTK_CSS_STYLE_PROPERTY_PRIVATE_H__ */
