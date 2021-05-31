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

#ifndef __CTK_STYLE_ANIMATION_PRIVATE_H__
#define __CTK_STYLE_ANIMATION_PRIVATE_H__

#include "gtkcssanimatedstyleprivate.h"

G_BEGIN_DECLS

#define CTK_TYPE_STYLE_ANIMATION           (_ctk_style_animation_get_type ())
#define CTK_STYLE_ANIMATION(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, CTK_TYPE_STYLE_ANIMATION, GtkStyleAnimation))
#define CTK_STYLE_ANIMATION_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, CTK_TYPE_STYLE_ANIMATION, GtkStyleAnimationClass))
#define CTK_IS_STYLE_ANIMATION(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, CTK_TYPE_STYLE_ANIMATION))
#define CTK_IS_STYLE_ANIMATION_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, CTK_TYPE_STYLE_ANIMATION))
#define CTK_STYLE_ANIMATION_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_STYLE_ANIMATION, GtkStyleAnimationClass))

typedef struct _GtkStyleAnimation           GtkStyleAnimation;
typedef struct _GtkStyleAnimationClass      GtkStyleAnimationClass;

struct _GtkStyleAnimation
{
  GObject parent;
};

struct _GtkStyleAnimationClass
{
  GObjectClass parent_class;

  gboolean      (* is_finished)                         (GtkStyleAnimation      *animation);
  gboolean      (* is_static)                           (GtkStyleAnimation      *animation);
  void          (* apply_values)                        (GtkStyleAnimation      *animation,
                                                         GtkCssAnimatedStyle    *style);
  GtkStyleAnimation *  (* advance)                      (GtkStyleAnimation      *animation,
                                                         gint64                  timestamp);
};

GType           _ctk_style_animation_get_type           (void) G_GNUC_CONST;

GtkStyleAnimation * _ctk_style_animation_advance        (GtkStyleAnimation      *animation,
                                                         gint64                  timestamp);
void            _ctk_style_animation_apply_values       (GtkStyleAnimation      *animation,
                                                         GtkCssAnimatedStyle    *style);
gboolean        _ctk_style_animation_is_finished        (GtkStyleAnimation      *animation);
gboolean        _ctk_style_animation_is_static          (GtkStyleAnimation      *animation);


G_END_DECLS

#endif /* __CTK_STYLE_ANIMATION_PRIVATE_H__ */
