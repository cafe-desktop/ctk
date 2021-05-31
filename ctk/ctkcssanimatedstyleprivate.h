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

#ifndef __CTK_CSS_ANIMATED_STYLE_PRIVATE_H__
#define __CTK_CSS_ANIMATED_STYLE_PRIVATE_H__

#include "ctk/ctkcssstyleprivate.h"

G_BEGIN_DECLS

#define CTK_TYPE_CSS_ANIMATED_STYLE           (ctk_css_animated_style_get_type ())
#define CTK_CSS_ANIMATED_STYLE(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, CTK_TYPE_CSS_ANIMATED_STYLE, CtkCssAnimatedStyle))
#define CTK_CSS_ANIMATED_STYLE_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, CTK_TYPE_CSS_ANIMATED_STYLE, CtkCssAnimatedStyleClass))
#define CTK_IS_CSS_ANIMATED_STYLE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, CTK_TYPE_CSS_ANIMATED_STYLE))
#define CTK_IS_CSS_ANIMATED_STYLE_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, CTK_TYPE_CSS_ANIMATED_STYLE))
#define CTK_CSS_ANIMATED_STYLE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CSS_ANIMATED_STYLE, CtkCssAnimatedStyleClass))

typedef struct _CtkCssAnimatedStyle           CtkCssAnimatedStyle;
typedef struct _CtkCssAnimatedStyleClass      CtkCssAnimatedStyleClass;

struct _CtkCssAnimatedStyle
{
  CtkCssStyle parent;

  CtkCssStyle           *style;                /* the style if we weren't animating */

  GPtrArray             *animated_values;      /* NULL or array of animated values/NULL if not animated */
  gint64                 current_time;         /* the current time in our world */
  GSList                *animations;           /* the running animations, least important one first */
};

struct _CtkCssAnimatedStyleClass
{
  CtkCssStyleClass parent_class;
};

GType                   ctk_css_animated_style_get_type         (void) G_GNUC_CONST;

CtkCssStyle *           ctk_css_animated_style_new              (CtkCssStyle            *base_style,
                                                                 CtkCssStyle            *parent_style,
                                                                 gint64                  timestamp,
                                                                 CtkStyleProviderPrivate *provider,
                                                                 CtkCssStyle            *previous_style);
CtkCssStyle *           ctk_css_animated_style_new_advance      (CtkCssAnimatedStyle    *source,
                                                                 CtkCssStyle            *base,
                                                                 gint64                  timestamp);

void                    ctk_css_animated_style_set_animated_value(CtkCssAnimatedStyle   *style,
                                                                 guint                   id,
                                                                 CtkCssValue            *value);
                                                                        
CtkCssValue *           ctk_css_animated_style_get_intrinsic_value (CtkCssAnimatedStyle *style,
                                                                 guint                   id);

G_END_DECLS

#endif /* __CTK_CSS_ANIMATED_STYLE_PRIVATE_H__ */
