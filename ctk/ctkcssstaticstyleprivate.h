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

#include "gtk/gtkcssmatcherprivate.h"
#include "gtk/gtkcssstyleprivate.h"

G_BEGIN_DECLS

#define CTK_TYPE_CSS_STATIC_STYLE           (ctk_css_static_style_get_type ())
#define CTK_CSS_STATIC_STYLE(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, CTK_TYPE_CSS_STATIC_STYLE, GtkCssStaticStyle))
#define CTK_CSS_STATIC_STYLE_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, CTK_TYPE_CSS_STATIC_STYLE, GtkCssStaticStyleClass))
#define CTK_IS_CSS_STATIC_STYLE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, CTK_TYPE_CSS_STATIC_STYLE))
#define CTK_IS_CSS_STATIC_STYLE_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, CTK_TYPE_CSS_STATIC_STYLE))
#define CTK_CSS_STATIC_STYLE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CSS_STATIC_STYLE, GtkCssStaticStyleClass))

typedef struct _GtkCssStaticStyle           GtkCssStaticStyle;
typedef struct _GtkCssStaticStyleClass      GtkCssStaticStyleClass;

struct _GtkCssStaticStyle
{
  GtkCssStyle parent;

  GtkCssValue           *values[CTK_CSS_PROPERTY_N_PROPERTIES]; /* the values */
  GPtrArray             *sections;             /* sections the values are defined in */

  GtkCssChange           change;               /* change as returned by value lookup */
};

struct _GtkCssStaticStyleClass
{
  GtkCssStyleClass parent_class;
};

GType                   ctk_css_static_style_get_type           (void) G_GNUC_CONST;

GtkCssStyle *           ctk_css_static_style_get_default        (void);
GtkCssStyle *           ctk_css_static_style_new_compute        (GtkStyleProviderPrivate *provider,
                                                                 const GtkCssMatcher    *matcher,
                                                                 GtkCssStyle            *parent);

void                    ctk_css_static_style_compute_value      (GtkCssStaticStyle      *style,
                                                                 GtkStyleProviderPrivate*provider,
                                                                 GtkCssStyle            *parent_style,
                                                                 guint                   id,
                                                                 GtkCssValue            *specified,
                                                                 GtkCssSection          *section);

GtkCssChange            ctk_css_static_style_get_change         (GtkCssStaticStyle      *style);

G_END_DECLS

#endif /* __CTK_CSS_STATIC_STYLE_PRIVATE_H__ */