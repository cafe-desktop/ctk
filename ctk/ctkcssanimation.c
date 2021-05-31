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

#include "ctkcssanimationprivate.h"

#include "ctkcsseasevalueprivate.h"
#include "ctkprogresstrackerprivate.h"

#include <math.h>

G_DEFINE_TYPE (GtkCssAnimation, _ctk_css_animation, CTK_TYPE_STYLE_ANIMATION)

static gboolean
ctk_css_animation_is_executing (GtkCssAnimation *animation)
{
  GtkProgressState state = ctk_progress_tracker_get_state (&animation->tracker);

  switch (animation->fill_mode)
    {
    case CTK_CSS_FILL_NONE:
      return state == CTK_PROGRESS_STATE_DURING;
    case CTK_CSS_FILL_FORWARDS:
      return state != CTK_PROGRESS_STATE_BEFORE;
    case CTK_CSS_FILL_BACKWARDS:
      return state != CTK_PROGRESS_STATE_AFTER;
    case CTK_CSS_FILL_BOTH:
      return TRUE;
    default:
      g_return_val_if_reached (FALSE);
    }
}

static double
ctk_css_animation_get_progress (GtkCssAnimation *animation)
{
  gboolean reverse, odd_iteration;
  gint cycle = ctk_progress_tracker_get_iteration_cycle (&animation->tracker);
  odd_iteration = cycle % 2 > 0;

  switch (animation->direction)
    {
    case CTK_CSS_DIRECTION_NORMAL:
      reverse = FALSE;
      break;
    case CTK_CSS_DIRECTION_REVERSE:
      reverse = TRUE;
      break;
    case CTK_CSS_DIRECTION_ALTERNATE:
      reverse = odd_iteration;
      break;
    case CTK_CSS_DIRECTION_ALTERNATE_REVERSE:
      reverse = !odd_iteration;
      break;
    default:
      g_return_val_if_reached (0.0);
    }

  return ctk_progress_tracker_get_progress (&animation->tracker, reverse);
}

GtkStyleAnimation *
ctk_css_animation_advance (GtkStyleAnimation    *style_animation,
                           gint64                timestamp)
{
  GtkCssAnimation *animation = CTK_CSS_ANIMATION (style_animation);

  return _ctk_css_animation_advance_with_play_state (animation,
                                                     timestamp,
                                                     animation->play_state);
}

static void
ctk_css_animation_apply_values (GtkStyleAnimation    *style_animation,
                                GtkCssAnimatedStyle  *style)
{
  GtkCssAnimation *animation = CTK_CSS_ANIMATION (style_animation);
  double progress;
  guint i;

  if (!ctk_css_animation_is_executing (animation))
    return;

  progress = ctk_css_animation_get_progress (animation);
  progress = _ctk_css_ease_value_transform (animation->ease, progress);

  for (i = 0; i < _ctk_css_keyframes_get_n_properties (animation->keyframes); i++)
    {
      GtkCssValue *value;
      guint property_id;
      
      property_id = _ctk_css_keyframes_get_property_id (animation->keyframes, i);

      value = _ctk_css_keyframes_get_value (animation->keyframes,
                                            i,
                                            progress,
                                            ctk_css_animated_style_get_intrinsic_value (style, property_id));
      ctk_css_animated_style_set_animated_value (style, property_id, value);
      _ctk_css_value_unref (value);
    }
}

static gboolean
ctk_css_animation_is_finished (GtkStyleAnimation *style_animation)
{
  return FALSE;
}

static gboolean
ctk_css_animation_is_static (GtkStyleAnimation *style_animation)
{
  GtkCssAnimation *animation = CTK_CSS_ANIMATION (style_animation);

  if (animation->play_state == CTK_CSS_PLAY_STATE_PAUSED)
    return TRUE;

  return ctk_progress_tracker_get_state (&animation->tracker) == CTK_PROGRESS_STATE_AFTER;
}

static void
ctk_css_animation_finalize (GObject *object)
{
  GtkCssAnimation *animation = CTK_CSS_ANIMATION (object);

  g_free (animation->name);
  _ctk_css_keyframes_unref (animation->keyframes);
  _ctk_css_value_unref (animation->ease);

  G_OBJECT_CLASS (_ctk_css_animation_parent_class)->finalize (object);
}

static void
_ctk_css_animation_class_init (GtkCssAnimationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkStyleAnimationClass *animation_class = CTK_STYLE_ANIMATION_CLASS (klass);

  object_class->finalize = ctk_css_animation_finalize;

  animation_class->advance = ctk_css_animation_advance;
  animation_class->apply_values = ctk_css_animation_apply_values;
  animation_class->is_finished = ctk_css_animation_is_finished;
  animation_class->is_static = ctk_css_animation_is_static;
}

static void
_ctk_css_animation_init (GtkCssAnimation *animation)
{
}

GtkStyleAnimation *
_ctk_css_animation_new (const char      *name,
                        GtkCssKeyframes *keyframes,
                        gint64           timestamp,
                        gint64           delay_us,
                        gint64           duration_us,
                        GtkCssValue     *ease,
                        GtkCssDirection  direction,
                        GtkCssPlayState  play_state,
                        GtkCssFillMode   fill_mode,
                        double           iteration_count)
{
  GtkCssAnimation *animation;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (keyframes != NULL, NULL);
  g_return_val_if_fail (ease != NULL, NULL);
  g_return_val_if_fail (iteration_count >= 0, NULL);

  animation = g_object_new (CTK_TYPE_CSS_ANIMATION, NULL);

  animation->name = g_strdup (name);
  animation->keyframes = _ctk_css_keyframes_ref (keyframes);
  animation->ease = _ctk_css_value_ref (ease);
  animation->direction = direction;
  animation->play_state = play_state;
  animation->fill_mode = fill_mode;

  ctk_progress_tracker_start (&animation->tracker, duration_us, delay_us, iteration_count);
  if (animation->play_state == CTK_CSS_PLAY_STATE_PAUSED)
    ctk_progress_tracker_skip_frame (&animation->tracker, timestamp);
  else
    ctk_progress_tracker_advance_frame (&animation->tracker, timestamp);

  return CTK_STYLE_ANIMATION (animation);
}

const char *
_ctk_css_animation_get_name (GtkCssAnimation *animation)
{
  g_return_val_if_fail (CTK_IS_CSS_ANIMATION (animation), NULL);

  return animation->name;
}

GtkStyleAnimation *
_ctk_css_animation_advance_with_play_state (GtkCssAnimation *source,
                                            gint64           timestamp,
                                            GtkCssPlayState  play_state)
{
  GtkCssAnimation *animation;

  g_return_val_if_fail (CTK_IS_CSS_ANIMATION (source), NULL);

  animation = g_object_new (CTK_TYPE_CSS_ANIMATION, NULL);

  animation->name = g_strdup (source->name);
  animation->keyframes = _ctk_css_keyframes_ref (source->keyframes);
  animation->ease = _ctk_css_value_ref (source->ease);
  animation->direction = source->direction;
  animation->play_state = play_state;
  animation->fill_mode = source->fill_mode;

  ctk_progress_tracker_init_copy (&source->tracker, &animation->tracker);
  if (animation->play_state == CTK_CSS_PLAY_STATE_PAUSED)
    ctk_progress_tracker_skip_frame (&animation->tracker, timestamp);
  else
    ctk_progress_tracker_advance_frame (&animation->tracker, timestamp);

  return CTK_STYLE_ANIMATION (animation);
}
