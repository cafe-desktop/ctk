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

#ifndef __CTK_CSS_ANIMATION_PRIVATE_H__
#define __CTK_CSS_ANIMATION_PRIVATE_H__

#include "ctkstyleanimationprivate.h"

#include "ctkcsskeyframesprivate.h"
#include "ctkprogresstrackerprivate.h"

G_BEGIN_DECLS

#define CTK_TYPE_CSS_ANIMATION           (_ctk_css_animation_get_type ())
#define CTK_CSS_ANIMATION(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, CTK_TYPE_CSS_ANIMATION, CtkCssAnimation))
#define CTK_CSS_ANIMATION_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, CTK_TYPE_CSS_ANIMATION, CtkCssAnimationClass))
#define CTK_IS_CSS_ANIMATION(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, CTK_TYPE_CSS_ANIMATION))
#define CTK_IS_CSS_ANIMATION_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, CTK_TYPE_CSS_ANIMATION))
#define CTK_CSS_ANIMATION_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CSS_ANIMATION, CtkCssAnimationClass))

typedef struct _CtkCssAnimation           CtkCssAnimation;
typedef struct _CtkCssAnimationClass      CtkCssAnimationClass;

struct _CtkCssAnimation
{
  CtkStyleAnimation parent;

  char            *name;
  CtkCssKeyframes *keyframes;
  CtkCssValue     *ease;
  CtkCssDirection  direction;
  CtkCssPlayState  play_state;
  CtkCssFillMode   fill_mode;
  CtkProgressTracker tracker;
};

struct _CtkCssAnimationClass
{
  CtkStyleAnimationClass parent_class;
};

GType                   _ctk_css_animation_get_type        (void) G_GNUC_CONST;

CtkStyleAnimation *     _ctk_css_animation_new             (const char         *name,
                                                            CtkCssKeyframes    *keyframes,
                                                            gint64              timestamp,
                                                            gint64              delay_us,
                                                            gint64              duration_us,
                                                            CtkCssValue        *ease,
                                                            CtkCssDirection     direction,
                                                            CtkCssPlayState     play_state,
                                                            CtkCssFillMode      fill_mode,
                                                            double              iteration_count);

CtkStyleAnimation *     _ctk_css_animation_advance_with_play_state (CtkCssAnimation   *animation,
                                                                    gint64             timestamp,
                                                                    CtkCssPlayState    play_state);

const char *            _ctk_css_animation_get_name        (CtkCssAnimation   *animation);

G_END_DECLS

#endif /* __CTK_CSS_ANIMATION_PRIVATE_H__ */
