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

#include "gtk/gtkstylepropertyprivate.h"

G_BEGIN_DECLS

#define CTK_TYPE_CSS_STYLE_PROPERTY           (_ctk_css_style_property_get_type ())
#define CTK_CSS_STYLE_PROPERTY(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, CTK_TYPE_CSS_STYLE_PROPERTY, GtkCssStyleProperty))
#define CTK_CSS_STYLE_PROPERTY_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, CTK_TYPE_CSS_STYLE_PROPERTY, GtkCssStylePropertyClass))
#define CTK_IS_CSS_STYLE_PROPERTY(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, CTK_TYPE_CSS_STYLE_PROPERTY))
#define CTK_IS_CSS_STYLE_PROPERTY_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, CTK_TYPE_CSS_STYLE_PROPERTY))
#define CTK_CSS_STYLE_PROPERTY_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CSS_STYLE_PROPERTY, GtkCssStylePropertyClass))

typedef struct _GtkCssStyleProperty           GtkCssStyleProperty;
typedef struct _GtkCssStylePropertyClass      GtkCssStylePropertyClass;

typedef GtkCssValue *    (* GtkCssStylePropertyParseFunc)  (GtkCssStyleProperty    *property,
                                                            GtkCssParser           *parser);
typedef void             (* GtkCssStylePropertyQueryFunc)  (GtkCssStyleProperty    *property,
                                                            const GtkCssValue      *cssvalue,
                                                            GValue                 *value);
typedef GtkCssValue *    (* GtkCssStylePropertyAssignFunc) (GtkCssStyleProperty    *property,
                                                            const GValue           *value);
struct _GtkCssStyleProperty
{
  GtkStyleProperty parent;

  GtkCssValue *initial_value;
  guint id;
  GtkCssAffects affects;
  guint inherit :1;
  guint animated :1;

  GtkCssStylePropertyParseFunc parse_value;
  GtkCssStylePropertyQueryFunc query_value;
  GtkCssStylePropertyAssignFunc assign_value;
};

struct _GtkCssStylePropertyClass
{
  GtkStylePropertyClass parent_class;

  GPtrArray *style_properties;
};

GType                   _ctk_css_style_property_get_type        (void) G_GNUC_CONST;

void                    _ctk_css_style_property_init_properties (void);

guint                   _ctk_css_style_property_get_n_properties(void);
GtkCssStyleProperty *   _ctk_css_style_property_lookup_by_id    (guint                   id);

gboolean                _ctk_css_style_property_is_inherit      (GtkCssStyleProperty    *property);
gboolean                _ctk_css_style_property_is_animated     (GtkCssStyleProperty    *property);
GtkCssAffects           _ctk_css_style_property_get_affects     (GtkCssStyleProperty    *property);
gboolean                _ctk_css_style_property_affects_size    (GtkCssStyleProperty    *property);
gboolean                _ctk_css_style_property_affects_font    (GtkCssStyleProperty    *property);
guint                   _ctk_css_style_property_get_id          (GtkCssStyleProperty    *property);
GtkCssValue  *          _ctk_css_style_property_get_initial_value
                                                                (GtkCssStyleProperty    *property);

void                    _ctk_css_style_property_print_value     (GtkCssStyleProperty    *property,
                                                                 GtkCssValue            *value,
                                                                 GString                *string);

GtkBitmask *            _ctk_css_style_property_get_mask_affecting
                                                                (GtkCssAffects           affects);

/* XXX - find a better place for these */
GtkCssValue * ctk_css_font_family_value_parse (GtkCssParser *parser);
GtkCssValue * ctk_css_font_size_value_parse   (GtkCssParser *parser);

G_END_DECLS

#endif /* __CTK_CSS_STYLE_PROPERTY_PRIVATE_H__ */