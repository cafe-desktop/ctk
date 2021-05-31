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

#include "ctkcsstransitionprivate.h"

#include "ctkcsseasevalueprivate.h"
#include "ctkprogresstrackerprivate.h"

G_DEFINE_TYPE (CtkCssTransition, _ctk_css_transition, CTK_TYPE_STYLE_ANIMATION)

static CtkStyleAnimation *
ctk_css_transition_advance (CtkStyleAnimation    *style_animation,
                           gint64                timestamp)
{
  CtkCssTransition *source = CTK_CSS_TRANSITION (style_animation);

  CtkCssTransition *transition;

  transition = g_object_new (CTK_TYPE_CSS_TRANSITION, NULL);

  transition->property = source->property;
  transition->start = _ctk_css_value_ref (source->start);
  transition->ease = _ctk_css_value_ref (source->ease);

  ctk_progress_tracker_init_copy (&source->tracker, &transition->tracker);
  ctk_progress_tracker_advance_frame (&transition->tracker, timestamp);

  return CTK_STYLE_ANIMATION (transition);
}

static void
ctk_css_transition_apply_values (CtkStyleAnimation   *style_animation,
                                 CtkCssAnimatedStyle *style)
{
  CtkCssTransition *transition = CTK_CSS_TRANSITION (style_animation);
  CtkCssValue *value, *end;
  double progress;
  CtkProgressState state;

  end = ctk_css_animated_style_get_intrinsic_value (style, transition->property);

  state = ctk_progress_tracker_get_state (&transition->tracker);

  if (state == CTK_PROGRESS_STATE_BEFORE)
    value = _ctk_css_value_ref (transition->start);
  else if (state == CTK_PROGRESS_STATE_DURING)
    {
      progress = ctk_progress_tracker_get_progress (&transition->tracker, FALSE);
      progress = _ctk_css_ease_value_transform (transition->ease, progress);

      value = _ctk_css_value_transition (transition->start,
                                         end,
                                         transition->property,
                                         progress);
    }
  else
    return;

  if (value == NULL)
    value = _ctk_css_value_ref (end);

  ctk_css_animated_style_set_animated_value (style, transition->property, value);
  _ctk_css_value_unref (value);
}

static gboolean
ctk_css_transition_is_finished (CtkStyleAnimation *animation)
{
  CtkCssTransition *transition = CTK_CSS_TRANSITION (animation);

  return ctk_progress_tracker_get_state (&transition->tracker) == CTK_PROGRESS_STATE_AFTER;
}

static gboolean
ctk_css_transition_is_static (CtkStyleAnimation *animation)
{
  CtkCssTransition *transition = CTK_CSS_TRANSITION (animation);

  return ctk_progress_tracker_get_state (&transition->tracker) == CTK_PROGRESS_STATE_AFTER;
}

static void
ctk_css_transition_finalize (GObject *object)
{
  CtkCssTransition *transition = CTK_CSS_TRANSITION (object);

  _ctk_css_value_unref (transition->start);
  _ctk_css_value_unref (transition->ease);

  G_OBJECT_CLASS (_ctk_css_transition_parent_class)->finalize (object);
}

static void
_ctk_css_transition_class_init (CtkCssTransitionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CtkStyleAnimationClass *animation_class = CTK_STYLE_ANIMATION_CLASS (klass);

  object_class->finalize = ctk_css_transition_finalize;

  animation_class->advance = ctk_css_transition_advance;
  animation_class->apply_values = ctk_css_transition_apply_values;
  animation_class->is_finished = ctk_css_transition_is_finished;
  animation_class->is_static = ctk_css_transition_is_static;
}

static void
_ctk_css_transition_init (CtkCssTransition *transition)
{
}

CtkStyleAnimation *
_ctk_css_transition_new (guint        property,
                         CtkCssValue *start,
                         CtkCssValue *ease,
                         gint64       timestamp,
                         gint64       duration_us,
                         gint64       delay_us)
{
  CtkCssTransition *transition;

  g_return_val_if_fail (start != NULL, NULL);
  g_return_val_if_fail (ease != NULL, NULL);

  transition = g_object_new (CTK_TYPE_CSS_TRANSITION, NULL);

  transition->property = property;
  transition->start = _ctk_css_value_ref (start);
  transition->ease = _ctk_css_value_ref (ease);
  ctk_progress_tracker_start (&transition->tracker, duration_us, delay_us, 1.0);
  ctk_progress_tracker_advance_frame (&transition->tracker, timestamp);

  return CTK_STYLE_ANIMATION (transition);
}

guint
_ctk_css_transition_get_property (CtkCssTransition *transition)
{
  g_return_val_if_fail (CTK_IS_CSS_TRANSITION (transition), 0);

  return transition->property;
}
