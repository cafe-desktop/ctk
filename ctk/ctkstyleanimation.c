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

#include "config.h"

#include "ctkstyleanimationprivate.h"

G_DEFINE_ABSTRACT_TYPE (CtkStyleAnimation, _ctk_style_animation, G_TYPE_OBJECT)

static CtkStyleAnimation *
ctk_style_animation_real_advance (CtkStyleAnimation *animation G_GNUC_UNUSED,
                                  gint64             timestamp G_GNUC_UNUSED)
{
  return NULL;
}

static void
ctk_style_animation_real_apply_values (CtkStyleAnimation   *animation G_GNUC_UNUSED,
                                       CtkCssAnimatedStyle *style G_GNUC_UNUSED)
{
}

static gboolean
ctk_style_animation_real_is_finished (CtkStyleAnimation *animation G_GNUC_UNUSED)
{
  return TRUE;
}

static gboolean
ctk_style_animation_real_is_static (CtkStyleAnimation *animation G_GNUC_UNUSED)
{
  return FALSE;
}

static void
_ctk_style_animation_class_init (CtkStyleAnimationClass *klass)
{
  klass->advance = ctk_style_animation_real_advance;
  klass->apply_values = ctk_style_animation_real_apply_values;
  klass->is_finished = ctk_style_animation_real_is_finished;
  klass->is_static = ctk_style_animation_real_is_static;
}

static void
_ctk_style_animation_init (CtkStyleAnimation *animation G_GNUC_UNUSED)
{
}

CtkStyleAnimation *
_ctk_style_animation_advance (CtkStyleAnimation    *animation,
                              gint64                timestamp)
{
  CtkStyleAnimationClass *klass;

  g_return_val_if_fail (CTK_IS_STYLE_ANIMATION (animation), NULL);

  klass = CTK_STYLE_ANIMATION_GET_CLASS (animation);

  return klass->advance (animation, timestamp);
}

void
_ctk_style_animation_apply_values (CtkStyleAnimation    *animation,
                                   CtkCssAnimatedStyle  *style)
{
  CtkStyleAnimationClass *klass;

  g_return_if_fail (CTK_IS_STYLE_ANIMATION (animation));
  g_return_if_fail (CTK_IS_CSS_ANIMATED_STYLE (style));

  klass = CTK_STYLE_ANIMATION_GET_CLASS (animation);

  klass->apply_values (animation, style);
}

gboolean
_ctk_style_animation_is_finished (CtkStyleAnimation *animation)
{
  CtkStyleAnimationClass *klass;

  g_return_val_if_fail (CTK_IS_STYLE_ANIMATION (animation), TRUE);

  klass = CTK_STYLE_ANIMATION_GET_CLASS (animation);

  return klass->is_finished (animation);
}

/**
 * _ctk_style_animation_is_static:
 * @animation: The animation to query
 * @at_time_us: The timestamp to query for
 *
 * Checks if @animation will not change its values anymore after
 * @at_time_us. This happens for example when the animation has reached its
 * final value or when it has been paused.
 *
 * Returns: %TRUE if @animation will not change anymore after @at_time_us
 **/
gboolean
_ctk_style_animation_is_static (CtkStyleAnimation *animation)
{
  CtkStyleAnimationClass *klass;

  g_return_val_if_fail (CTK_IS_STYLE_ANIMATION (animation), TRUE);

  klass = CTK_STYLE_ANIMATION_GET_CLASS (animation);

  return klass->is_static (animation);
}
