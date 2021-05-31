/*
 * Copyright © 2012 Red Hat Inc.
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

#ifndef __CTK_CSS_STATIC_STYLE_PRIVATE_H__
#define __CTK_CSS_STATIC_STYLE_PRIVATE_H__

#include "ctk/ctkcssmatcherprivate.h"
#include "ctk/ctkcssstyleprivate.h"

G_BEGIN_DECLS

#define CTK_TYPE_CSS_STATIC_STYLE           (ctk_css_static_style_get_type ())
#define CTK_CSS_STATIC_STYLE(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, CTK_TYPE_CSS_STATIC_STYLE, CtkCssStaticStyle))
#define CTK_CSS_STATIC_STYLE_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, CTK_TYPE_CSS_STATIC_STYLE, CtkCssStaticStyleClass))
#define CTK_IS_CSS_STATIC_STYLE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, CTK_TYPE_CSS_STATIC_STYLE))
#define CTK_IS_CSS_STATIC_STYLE_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, CTK_TYPE_CSS_STATIC_STYLE))
#define CTK_CSS_STATIC_STYLE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CSS_STATIC_STYLE, CtkCssStaticStyleClass))

typedef struct _CtkCssStaticStyle           CtkCssStaticStyle;
typedef struct _CtkCssStaticStyleClass      CtkCssStaticStyleClass;

struct _CtkCssStaticStyle
{
  CtkCssStyle parent;

  CtkCssValue           *values[CTK_CSS_PROPERTY_N_PROPERTIES]; /* the values */
  GPtrArray             *sections;             /* sections the values are defined in */

  CtkCssChange           change;               /* change as returned by value lookup */
};

struct _CtkCssStaticStyleClass
{
  CtkCssStyleClass parent_class;
};

GType                   ctk_css_static_style_get_type           (void) G_GNUC_CONST;

CtkCssStyle *           ctk_css_static_style_get_default        (void);
CtkCssStyle *           ctk_css_static_style_new_compute        (CtkStyleProviderPrivate *provider,
                                                                 const CtkCssMatcher    *matcher,
                                                                 CtkCssStyle            *parent);

void                    ctk_css_static_style_compute_value      (CtkCssStaticStyle      *style,
                                                                 CtkStyleProviderPrivate*provider,
                                                                 CtkCssStyle            *parent_style,
                                                                 guint                   id,
                                                                 CtkCssValue            *specified,
                                                                 CtkCssSection          *section);

CtkCssChange            ctk_css_static_style_get_change         (CtkCssStaticStyle      *style);

G_END_DECLS

#endif /* __CTK_CSS_STATIC_STYLE_PRIVATE_H__ */
