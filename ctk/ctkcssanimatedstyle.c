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

#include "gtkcssanimatedstyleprivate.h"

#include "gtkcssanimationprivate.h"
#include "gtkcssarrayvalueprivate.h"
#include "gtkcssenumvalueprivate.h"
#include "gtkcssinheritvalueprivate.h"
#include "gtkcssinitialvalueprivate.h"
#include "gtkcssnumbervalueprivate.h"
#include "gtkcsssectionprivate.h"
#include "gtkcssshorthandpropertyprivate.h"
#include "gtkcssstaticstyleprivate.h"
#include "gtkcssstringvalueprivate.h"
#include "gtkcssstylepropertyprivate.h"
#include "gtkcsstransitionprivate.h"
#include "gtkprivate.h"
#include "gtkstyleanimationprivate.h"
#include "gtkstylepropertyprivate.h"
#include "gtkstyleproviderprivate.h"

G_DEFINE_TYPE (GtkCssAnimatedStyle, ctk_css_animated_style, CTK_TYPE_CSS_STYLE)

static GtkCssValue *
ctk_css_animated_style_get_value (GtkCssStyle *style,
                                  guint        id)
{
  GtkCssAnimatedStyle *animated = CTK_CSS_ANIMATED_STYLE (style);

  if (animated->animated_values &&
      id < animated->animated_values->len &&
      g_ptr_array_index (animated->animated_values, id))
    return g_ptr_array_index (animated->animated_values, id);

  return ctk_css_animated_style_get_intrinsic_value (animated, id);
}

static GtkCssSection *
ctk_css_animated_style_get_section (GtkCssStyle *style,
                                    guint        id)
{
  GtkCssAnimatedStyle *animated = CTK_CSS_ANIMATED_STYLE (style);

  return ctk_css_style_get_section (animated->style, id);
}

static gboolean
ctk_css_animated_style_is_static (GtkCssStyle *style)
{
  GtkCssAnimatedStyle *animated = CTK_CSS_ANIMATED_STYLE (style);
  GSList *list;

  for (list = animated->animations; list; list = list->next)
    {
      if (!_ctk_style_animation_is_static (list->data))
        return FALSE;
    }

  return TRUE;
}

static void
ctk_css_animated_style_dispose (GObject *object)
{
  GtkCssAnimatedStyle *style = CTK_CSS_ANIMATED_STYLE (object);

  if (style->animated_values)
    {
      g_ptr_array_unref (style->animated_values);
      style->animated_values = NULL;
    }

  g_slist_free_full (style->animations, g_object_unref);
  style->animations = NULL;

  G_OBJECT_CLASS (ctk_css_animated_style_parent_class)->dispose (object);
}

static void
ctk_css_animated_style_finalize (GObject *object)
{
  GtkCssAnimatedStyle *style = CTK_CSS_ANIMATED_STYLE (object);

  g_object_unref (style->style);

  G_OBJECT_CLASS (ctk_css_animated_style_parent_class)->finalize (object);
}

static void
ctk_css_animated_style_class_init (GtkCssAnimatedStyleClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkCssStyleClass *style_class = CTK_CSS_STYLE_CLASS (klass);

  object_class->dispose = ctk_css_animated_style_dispose;
  object_class->finalize = ctk_css_animated_style_finalize;

  style_class->get_value = ctk_css_animated_style_get_value;
  style_class->get_section = ctk_css_animated_style_get_section;
  style_class->is_static = ctk_css_animated_style_is_static;
}

static void
ctk_css_animated_style_init (GtkCssAnimatedStyle *style)
{
}

void
ctk_css_animated_style_set_animated_value (GtkCssAnimatedStyle *style,
                                           guint                id,
                                           GtkCssValue         *value)
{
  ctk_internal_return_if_fail (CTK_IS_CSS_ANIMATED_STYLE (style));
  ctk_internal_return_if_fail (value != NULL);

  if (style->animated_values == NULL)
    style->animated_values = g_ptr_array_new_with_free_func ((GDestroyNotify)_ctk_css_value_unref);
  if (id >= style->animated_values->len)
   g_ptr_array_set_size (style->animated_values, id + 1);

  if (g_ptr_array_index (style->animated_values, id))
    _ctk_css_value_unref (g_ptr_array_index (style->animated_values, id));
  g_ptr_array_index (style->animated_values, id) = _ctk_css_value_ref (value);

}

GtkCssValue *
ctk_css_animated_style_get_intrinsic_value (GtkCssAnimatedStyle *style,
                                            guint                id)
{
  ctk_internal_return_val_if_fail (CTK_IS_CSS_ANIMATED_STYLE (style), NULL);

  return ctk_css_style_get_value (style->style, id);
}

/* TRANSITIONS */

typedef struct _TransitionInfo TransitionInfo;
struct _TransitionInfo {
  guint index;                  /* index into value arrays */
  gboolean pending;             /* TRUE if we still need to handle it */
};

static void
transition_info_add (TransitionInfo    infos[CTK_CSS_PROPERTY_N_PROPERTIES],
                     GtkStyleProperty *property,
                     guint             index)
{
  if (property == NULL)
    {
      guint i;

      for (i = 0; i < _ctk_css_style_property_get_n_properties (); i++)
        {
          GtkCssStyleProperty *prop = _ctk_css_style_property_lookup_by_id (i);

          transition_info_add (infos, CTK_STYLE_PROPERTY (prop), index);
        }
    }
  else if (CTK_IS_CSS_SHORTHAND_PROPERTY (property))
    {
      GtkCssShorthandProperty *shorthand = CTK_CSS_SHORTHAND_PROPERTY (property);
      guint i;

      for (i = 0; i < _ctk_css_shorthand_property_get_n_subproperties (shorthand); i++)
        {
          GtkCssStyleProperty *prop = _ctk_css_shorthand_property_get_subproperty (shorthand, i);

          transition_info_add (infos, CTK_STYLE_PROPERTY (prop), index);
        }
    }
  else if (CTK_IS_CSS_STYLE_PROPERTY (property))
    {
      guint id;
      
      if (!_ctk_css_style_property_is_animated (CTK_CSS_STYLE_PROPERTY (property)))
        return;

      id = _ctk_css_style_property_get_id (CTK_CSS_STYLE_PROPERTY (property));
      g_assert (id < CTK_CSS_PROPERTY_N_PROPERTIES);
      infos[id].index = index;
      infos[id].pending = TRUE;
    }
  else
    {
      g_assert_not_reached ();
    }
}

static void
transition_infos_set (TransitionInfo  infos[CTK_CSS_PROPERTY_N_PROPERTIES],
                      GtkCssValue    *transitions)
{
  guint i;

  for (i = 0; i < _ctk_css_array_value_get_n_values (transitions); i++)
    {
      GtkStyleProperty *property;
      GtkCssValue *prop_value;

      prop_value = _ctk_css_array_value_get_nth (transitions, i);
      if (g_ascii_strcasecmp (_ctk_css_ident_value_get (prop_value), "all") == 0)
        property = NULL;
      else
        {
          property = _ctk_style_property_lookup (_ctk_css_ident_value_get (prop_value));
          if (property == NULL)
            continue;
        }
      
      transition_info_add (infos, property, i);
    }
}

static GtkStyleAnimation *
ctk_css_animated_style_find_transition (GtkCssAnimatedStyle *style,
                                        guint                property_id)
{
  GSList *list;

  for (list = style->animations; list; list = list->next)
    {
      if (!CTK_IS_CSS_TRANSITION (list->data))
        continue;

      if (_ctk_css_transition_get_property (list->data) == property_id)
        return list->data;
    }

  return NULL;
}

static GSList *
ctk_css_animated_style_create_css_transitions (GSList              *animations,
                                               GtkCssStyle         *base_style,
                                               gint64               timestamp,
                                               GtkCssStyle         *source)
{
  TransitionInfo transitions[CTK_CSS_PROPERTY_N_PROPERTIES] = { { 0, } };
  GtkCssValue *durations, *delays, *timing_functions;
  guint i;

  transition_infos_set (transitions, ctk_css_style_get_value (base_style, CTK_CSS_PROPERTY_TRANSITION_PROPERTY));

  durations = ctk_css_style_get_value (base_style, CTK_CSS_PROPERTY_TRANSITION_DURATION);
  delays = ctk_css_style_get_value (base_style, CTK_CSS_PROPERTY_TRANSITION_DELAY);
  timing_functions = ctk_css_style_get_value (base_style, CTK_CSS_PROPERTY_TRANSITION_TIMING_FUNCTION);

  for (i = 0; i < CTK_CSS_PROPERTY_N_PROPERTIES; i++)
    {
      GtkStyleAnimation *animation;
      GtkCssValue *start, *end;
      double duration, delay;

      if (!transitions[i].pending)
        continue;

      duration = _ctk_css_number_value_get (_ctk_css_array_value_get_nth (durations, transitions[i].index), 100);
      delay = _ctk_css_number_value_get (_ctk_css_array_value_get_nth (delays, transitions[i].index), 100);
      if (duration + delay == 0.0)
        continue;

      if (CTK_IS_CSS_ANIMATED_STYLE (source))
        {
          start = ctk_css_animated_style_get_intrinsic_value (CTK_CSS_ANIMATED_STYLE (source), i);
          end = ctk_css_style_get_value (base_style, i);

          if (_ctk_css_value_equal (start, end))
            {
              animation = ctk_css_animated_style_find_transition (CTK_CSS_ANIMATED_STYLE (source), i);
              if (animation)
                {
                  animation = _ctk_style_animation_advance (animation, timestamp);
                  animations = g_slist_prepend (animations, animation);
                }

              continue;
            }
        }

      if (_ctk_css_value_equal (ctk_css_style_get_value (source, i),
                                ctk_css_style_get_value (base_style, i)))
        continue;

      animation = _ctk_css_transition_new (i,
                                           ctk_css_style_get_value (source, i),
                                           _ctk_css_array_value_get_nth (timing_functions, i),
                                           timestamp,
                                           duration * G_USEC_PER_SEC,
                                           delay * G_USEC_PER_SEC);
      animations = g_slist_prepend (animations, animation);
    }

  return animations;
}

static GtkStyleAnimation *
ctk_css_animated_style_find_animation (GSList     *animations,
                                       const char *name)
{
  GSList *list;

  for (list = animations; list; list = list->next)
    {
      if (!CTK_IS_CSS_ANIMATION (list->data))
        continue;

      if (g_str_equal (_ctk_css_animation_get_name (list->data), name))
        return list->data;
    }

  return NULL;
}

static GSList *
ctk_css_animated_style_create_css_animations (GSList                  *animations,
                                              GtkCssStyle             *base_style,
                                              GtkCssStyle             *parent_style,
                                              gint64                   timestamp,
                                              GtkStyleProviderPrivate *provider,
                                              GtkCssStyle             *source)
{
  GtkCssValue *durations, *delays, *timing_functions, *animation_names;
  GtkCssValue *iteration_counts, *directions, *play_states, *fill_modes;
  guint i;

  animation_names = ctk_css_style_get_value (base_style, CTK_CSS_PROPERTY_ANIMATION_NAME);
  durations = ctk_css_style_get_value (base_style, CTK_CSS_PROPERTY_ANIMATION_DURATION);
  delays = ctk_css_style_get_value (base_style, CTK_CSS_PROPERTY_ANIMATION_DELAY);
  timing_functions = ctk_css_style_get_value (base_style, CTK_CSS_PROPERTY_ANIMATION_TIMING_FUNCTION);
  iteration_counts = ctk_css_style_get_value (base_style, CTK_CSS_PROPERTY_ANIMATION_ITERATION_COUNT);
  directions = ctk_css_style_get_value (base_style, CTK_CSS_PROPERTY_ANIMATION_DIRECTION);
  play_states = ctk_css_style_get_value (base_style, CTK_CSS_PROPERTY_ANIMATION_PLAY_STATE);
  fill_modes = ctk_css_style_get_value (base_style, CTK_CSS_PROPERTY_ANIMATION_FILL_MODE);

  for (i = 0; i < _ctk_css_array_value_get_n_values (animation_names); i++)
    {
      GtkStyleAnimation *animation;
      GtkCssKeyframes *keyframes;
      const char *name;
      
      name = _ctk_css_ident_value_get (_ctk_css_array_value_get_nth (animation_names, i));
      if (g_ascii_strcasecmp (name, "none") == 0)
        continue;

      animation = ctk_css_animated_style_find_animation (animations, name);
      if (animation)
        continue;

      if (CTK_IS_CSS_ANIMATED_STYLE (source))
        animation = ctk_css_animated_style_find_animation (CTK_CSS_ANIMATED_STYLE (source)->animations, name);

      if (animation)
        {
          animation = _ctk_css_animation_advance_with_play_state (CTK_CSS_ANIMATION (animation),
                                                                  timestamp,
                                                                  _ctk_css_play_state_value_get (_ctk_css_array_value_get_nth (play_states, i)));
        }
      else
        {
          keyframes = _ctk_style_provider_private_get_keyframes (provider, name);
          if (keyframes == NULL)
            continue;

          keyframes = _ctk_css_keyframes_compute (keyframes, provider, base_style, parent_style);

          animation = _ctk_css_animation_new (name,
                                              keyframes,
                                              timestamp,
                                              _ctk_css_number_value_get (_ctk_css_array_value_get_nth (delays, i), 100) * G_USEC_PER_SEC,
                                              _ctk_css_number_value_get (_ctk_css_array_value_get_nth (durations, i), 100) * G_USEC_PER_SEC,
                                              _ctk_css_array_value_get_nth (timing_functions, i),
                                              _ctk_css_direction_value_get (_ctk_css_array_value_get_nth (directions, i)),
                                              _ctk_css_play_state_value_get (_ctk_css_array_value_get_nth (play_states, i)),
                                              _ctk_css_fill_mode_value_get (_ctk_css_array_value_get_nth (fill_modes, i)),
                                              _ctk_css_number_value_get (_ctk_css_array_value_get_nth (iteration_counts, i), 100));
          _ctk_css_keyframes_unref (keyframes);
        }
      animations = g_slist_prepend (animations, animation);
    }

  return animations;
}

/* PUBLIC API */

static void
ctk_css_animated_style_apply_animations (GtkCssAnimatedStyle *style)
{
  GSList *l;

  for (l = style->animations; l; l = l->next)
    {
      GtkStyleAnimation *animation = l->data;
      
      _ctk_style_animation_apply_values (animation,
                                         CTK_CSS_ANIMATED_STYLE (style));
    }
}

GtkCssStyle *
ctk_css_animated_style_new (GtkCssStyle             *base_style,
                            GtkCssStyle             *parent_style,
                            gint64                   timestamp,
                            GtkStyleProviderPrivate *provider,
                            GtkCssStyle             *previous_style)
{
  GtkCssAnimatedStyle *result;
  GSList *animations;
  
  ctk_internal_return_val_if_fail (CTK_IS_CSS_STYLE (base_style), NULL);
  ctk_internal_return_val_if_fail (parent_style == NULL || CTK_IS_CSS_STYLE (parent_style), NULL);
  ctk_internal_return_val_if_fail (CTK_IS_STYLE_PROVIDER (provider), NULL);
  ctk_internal_return_val_if_fail (previous_style == NULL || CTK_IS_CSS_STYLE (previous_style), NULL);

  if (timestamp == 0)
    return g_object_ref (base_style);

  animations = NULL;

  if (previous_style != NULL)
    animations = ctk_css_animated_style_create_css_transitions (animations, base_style, timestamp, previous_style);
  animations = ctk_css_animated_style_create_css_animations (animations, base_style, parent_style, timestamp, provider, previous_style);

  if (animations == NULL)
    return g_object_ref (base_style);

  result = g_object_new (CTK_TYPE_CSS_ANIMATED_STYLE, NULL);

  result->style = g_object_ref (base_style);
  result->current_time = timestamp;
  result->animations = animations;

  ctk_css_animated_style_apply_animations (result);

  return CTK_CSS_STYLE (result);
}

GtkCssStyle *
ctk_css_animated_style_new_advance (GtkCssAnimatedStyle *source,
                                    GtkCssStyle         *base,
                                    gint64               timestamp)
{
  GtkCssAnimatedStyle *result;
  GSList *l, *animations;

  ctk_internal_return_val_if_fail (CTK_IS_CSS_ANIMATED_STYLE (source), NULL);
  ctk_internal_return_val_if_fail (CTK_IS_CSS_STYLE (base), NULL);
  
  if (timestamp == 0 || timestamp == source->current_time)
    return g_object_ref (source->style);

  ctk_internal_return_val_if_fail (timestamp > source->current_time, NULL);

  animations = NULL;
  for (l = source->animations; l; l = l->next)
    {
      GtkStyleAnimation *animation = l->data;
      
      if (_ctk_style_animation_is_finished (animation))
        continue;

      animation = _ctk_style_animation_advance (animation, timestamp);
      animations = g_slist_prepend (animations, animation);
    }
  animations = g_slist_reverse (animations);

  if (animations == NULL)
    return g_object_ref (source->style);

  result = g_object_new (CTK_TYPE_CSS_ANIMATED_STYLE, NULL);

  result->style = g_object_ref (base);
  result->current_time = timestamp;
  result->animations = animations;

  ctk_css_animated_style_apply_animations (result);

  return CTK_CSS_STYLE (result);
}
