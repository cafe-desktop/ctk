/* CTK - The GIMP Toolkit
 * Copyright (C) 2017 Benjamin Otte <otte@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "ctkfishbowl.h"

#include "ctk/fallback-c89.c"

typedef struct _CtkFishbowlPrivate       CtkFishbowlPrivate;
typedef struct _CtkFishbowlChild         CtkFishbowlChild;

struct _CtkFishbowlPrivate
{
  CtkFishCreationFunc creation_func;
  GList *children;
  guint count;

  gint64 last_frame_time;
  gint64 update_delay;
  guint tick_id;

  double framerate;
  int last_benchmark_change;

  guint benchmark : 1;
};

struct _CtkFishbowlChild
{
  CtkWidget *widget;
  double x;
  double y;
  double dx;
  double dy;
};

enum {
   PROP_0,
   PROP_ANIMATING,
   PROP_BENCHMARK,
   PROP_COUNT,
   PROP_FRAMERATE,
   PROP_UPDATE_DELAY,
   NUM_PROPERTIES
};

static GParamSpec *props[NUM_PROPERTIES] = { NULL, };

G_DEFINE_TYPE_WITH_PRIVATE (CtkFishbowl, ctk_fishbowl, CTK_TYPE_CONTAINER)

static void
ctk_fishbowl_init (CtkFishbowl *fishbowl)
{
  CtkFishbowlPrivate *priv = ctk_fishbowl_get_instance_private (fishbowl);

  ctk_widget_set_has_window (CTK_WIDGET (fishbowl), FALSE);

  priv->update_delay = G_USEC_PER_SEC;
}

/**
 * ctk_fishbowl_new:
 *
 * Creates a new #CtkFishbowl.
 *
 * Returns: a new #CtkFishbowl.
 */
CtkWidget*
ctk_fishbowl_new (void)
{
  return g_object_new (CTK_TYPE_FISHBOWL, NULL);
}

static void
ctk_fishbowl_get_preferred_width (CtkWidget *widget,
                                  int       *minimum,
                                  int       *natural)
{
  CtkFishbowl *fishbowl = CTK_FISHBOWL (widget);
  CtkFishbowlPrivate *priv = ctk_fishbowl_get_instance_private (fishbowl);
  GList *children;
  gint child_min, child_nat;

  *minimum = 0;
  *natural = 0;

  for (children = priv->children; children; children = children->next)
    {
      CtkFishbowlChild *child;

      child = children->data;

      if (!ctk_widget_get_visible (child->widget))
        continue;

      ctk_widget_get_preferred_width (child->widget, &child_min, &child_nat);

      *minimum = MAX (*minimum, child_min);
      *natural = MAX (*natural, child_nat);
    }
}

static void
ctk_fishbowl_get_preferred_height (CtkWidget *widget,
                                   int       *minimum,
                                   int       *natural)
{
  CtkFishbowl *fishbowl = CTK_FISHBOWL (widget);
  CtkFishbowlPrivate *priv = ctk_fishbowl_get_instance_private (fishbowl);
  GList *children;
  gint child_min, child_nat;

  *minimum = 0;
  *natural = 0;

  for (children = priv->children; children; children = children->next)
    {
      int min_width;
      CtkFishbowlChild *child;

      child = children->data;

      if (!ctk_widget_get_visible (child->widget))
        continue;

      ctk_widget_get_preferred_width (child->widget, &min_width, NULL);
      ctk_widget_get_preferred_height_for_width (child->widget, min_width, &child_min, &child_nat);

      *minimum = MAX (*minimum, child_min);
      *natural = MAX (*natural, child_nat);
    }
}

static void
ctk_fishbowl_size_allocate (CtkWidget     *widget,
                            CtkAllocation *allocation)
{
  CtkFishbowl *fishbowl = CTK_FISHBOWL (widget);
  CtkFishbowlPrivate *priv = ctk_fishbowl_get_instance_private (fishbowl);
  CtkAllocation child_allocation;
  CtkRequisition child_requisition;
  GList *children;

  for (children = priv->children; children; children = children->next)
    {
      CtkFishbowlChild *child;

      child = children->data;

      if (!ctk_widget_get_visible (child->widget))
        continue;

      ctk_widget_get_preferred_size (child->widget, &child_requisition, NULL);
      child_allocation.x = allocation->x + round (child->x * (allocation->width - child_requisition.width));
      child_allocation.y = allocation->y + round (child->y * (allocation->height - child_requisition.height));
      child_allocation.width = child_requisition.width;
      child_allocation.height = child_requisition.height;

      ctk_widget_size_allocate (child->widget, &child_allocation);
    }
}

static double
new_speed (void)
{
  /* 5s to 50s to cross screen seems fair */
  return g_random_double_range (0.02, 0.2);
}

static void
ctk_fishbowl_add (CtkContainer *container,
                  CtkWidget    *widget)
{
  CtkFishbowl *fishbowl = CTK_FISHBOWL (container);
  CtkFishbowlPrivate *priv = ctk_fishbowl_get_instance_private (fishbowl);
  CtkFishbowlChild *child_info;

  g_return_if_fail (CTK_IS_FISHBOWL (fishbowl));
  g_return_if_fail (CTK_IS_WIDGET (widget));

  child_info = g_new0 (CtkFishbowlChild, 1);
  child_info->widget = widget;
  child_info->x = 0;
  child_info->y = 0;
  child_info->dx = new_speed ();
  child_info->dy = new_speed ();

  ctk_widget_set_parent (widget, CTK_WIDGET (fishbowl));

  priv->children = g_list_prepend (priv->children, child_info);
  priv->count++;
  g_object_notify_by_pspec (G_OBJECT (fishbowl), props[PROP_COUNT]);
}

static void
ctk_fishbowl_remove (CtkContainer *container,
                     CtkWidget    *widget)
{
  CtkFishbowl *fishbowl = CTK_FISHBOWL (container);
  CtkFishbowlPrivate *priv = ctk_fishbowl_get_instance_private (fishbowl);
  CtkWidget *widget_bowl = CTK_WIDGET (fishbowl);
  GList *children;

  for (children = priv->children; children; children = children->next)
    {
      CtkFishbowlChild *child;

      child = children->data;

      if (child->widget == widget)
        {
          gboolean was_visible = ctk_widget_get_visible (widget);

          ctk_widget_unparent (widget);

          priv->children = g_list_remove_link (priv->children, children);
          g_list_free (children);
          g_free (child);

          if (was_visible && ctk_widget_get_visible (widget_bowl))
            ctk_widget_queue_resize (widget_bowl);

          priv->count--;
          g_object_notify_by_pspec (G_OBJECT (fishbowl), props[PROP_COUNT]);
          break;
        }
    }
}


static void
ctk_fishbowl_forall (CtkContainer *container,
                     gboolean      include_internals,
                     CtkCallback   callback,
                     gpointer      callback_data)
{
  CtkFishbowl *fishbowl = CTK_FISHBOWL (container);
  CtkFishbowlPrivate *priv = ctk_fishbowl_get_instance_private (fishbowl);
  GList *children;

  if (!include_internals)
    return;

  children = priv->children;
  while (children)
    {
      CtkFishbowlChild *child;

      child = children->data;
      children = children->next;

      (* callback) (child->widget, callback_data);
    }
}

static void
ctk_fishbowl_dispose (GObject *object)
{
  CtkFishbowl *fishbowl = CTK_FISHBOWL (object);

  ctk_fishbowl_set_animating (fishbowl, FALSE);
  ctk_fishbowl_set_count (fishbowl, 0);

  G_OBJECT_CLASS (ctk_fishbowl_parent_class)->dispose (object);
}

static void
ctk_fishbowl_set_property (GObject         *object,
                           guint            prop_id,
                           const GValue    *value,
                           GParamSpec      *pspec)
{
  CtkFishbowl *fishbowl = CTK_FISHBOWL (object);

  switch (prop_id)
    {
    case PROP_ANIMATING:
      ctk_fishbowl_set_animating (fishbowl, g_value_get_boolean (value));
      break;

    case PROP_BENCHMARK:
      ctk_fishbowl_set_benchmark (fishbowl, g_value_get_boolean (value));
      break;

    case PROP_COUNT:
      ctk_fishbowl_set_count (fishbowl, g_value_get_uint (value));
      break;

    case PROP_UPDATE_DELAY:
      ctk_fishbowl_set_update_delay (fishbowl, g_value_get_int64 (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_fishbowl_get_property (GObject         *object,
                           guint            prop_id,
                           GValue          *value,
                           GParamSpec      *pspec)
{
  CtkFishbowl *fishbowl = CTK_FISHBOWL (object);

  switch (prop_id)
    {
    case PROP_ANIMATING:
      g_value_set_boolean (value, ctk_fishbowl_get_animating (fishbowl));
      break;

    case PROP_BENCHMARK:
      g_value_set_boolean (value, ctk_fishbowl_get_benchmark (fishbowl));
      break;

    case PROP_COUNT:
      g_value_set_uint (value, ctk_fishbowl_get_count (fishbowl));
      break;

    case PROP_FRAMERATE:
      g_value_set_double (value, ctk_fishbowl_get_framerate (fishbowl));
      break;

    case PROP_UPDATE_DELAY:
      g_value_set_int64 (value, ctk_fishbowl_get_update_delay (fishbowl));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_fishbowl_class_init (CtkFishbowlClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);
  CtkContainerClass *container_class = CTK_CONTAINER_CLASS (klass);

  object_class->dispose = ctk_fishbowl_dispose;
  object_class->set_property = ctk_fishbowl_set_property;
  object_class->get_property = ctk_fishbowl_get_property;

  widget_class->get_preferred_width = ctk_fishbowl_get_preferred_width;
  widget_class->get_preferred_height = ctk_fishbowl_get_preferred_height;
  widget_class->size_allocate = ctk_fishbowl_size_allocate;

  container_class->add = ctk_fishbowl_add;
  container_class->remove = ctk_fishbowl_remove;
  container_class->forall = ctk_fishbowl_forall;

  props[PROP_ANIMATING] =
      g_param_spec_boolean ("animating",
                            "animating",
                            "Whether children are moving around",
                            FALSE,
                            G_PARAM_READWRITE);

  props[PROP_BENCHMARK] =
      g_param_spec_boolean ("benchmark",
                            "Benchmark",
                            "Adapt the count property to hit the maximum framerate",
                            FALSE,
                            G_PARAM_READWRITE);

  props[PROP_COUNT] =
      g_param_spec_uint ("count",
                         "Count",
                         "Number of widgets",
                         0, G_MAXUINT,
                         0,
                         G_PARAM_READWRITE);

  props[PROP_FRAMERATE] =
      g_param_spec_double ("framerate",
                           "Framerate",
                           "Framerate of this widget in frames per second",
                           0, G_MAXDOUBLE,
                           0,
                           G_PARAM_READABLE);

  props[PROP_UPDATE_DELAY] =
      g_param_spec_int64 ("update-delay",
                          "Update delay",
                          "Number of usecs between updates",
                          0, G_MAXINT64,
                          G_USEC_PER_SEC,
                          G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, props);
}

guint
ctk_fishbowl_get_count (CtkFishbowl *fishbowl)
{
  CtkFishbowlPrivate *priv = ctk_fishbowl_get_instance_private (fishbowl);

  return priv->count;
}

void
ctk_fishbowl_set_count (CtkFishbowl *fishbowl,
                        guint        count)
{
  CtkFishbowlPrivate *priv = ctk_fishbowl_get_instance_private (fishbowl);

  if (priv->count == count)
    return;

  g_object_freeze_notify (G_OBJECT (fishbowl));

  while (priv->count > count)
    {
      ctk_fishbowl_remove (CTK_CONTAINER (fishbowl), ((CtkFishbowlChild *) priv->children->data)->widget);
    }

  while (priv->count < count)
    {
      CtkWidget *new_widget;

      new_widget = priv->creation_func ();

      ctk_widget_show (new_widget);

      ctk_fishbowl_add (CTK_CONTAINER (fishbowl), new_widget);
    }

  g_object_thaw_notify (G_OBJECT (fishbowl));
}

gboolean
ctk_fishbowl_get_benchmark (CtkFishbowl *fishbowl)
{
  CtkFishbowlPrivate *priv = ctk_fishbowl_get_instance_private (fishbowl);

  return priv->benchmark;
}

void
ctk_fishbowl_set_benchmark (CtkFishbowl *fishbowl,
                            gboolean     benchmark)
{
  CtkFishbowlPrivate *priv = ctk_fishbowl_get_instance_private (fishbowl);

  if (priv->benchmark == benchmark)
    return;

  priv->benchmark = benchmark;
  if (!benchmark)
    priv->last_benchmark_change = 0;

  g_object_notify_by_pspec (G_OBJECT (fishbowl), props[PROP_BENCHMARK]);
}

gboolean
ctk_fishbowl_get_animating (CtkFishbowl *fishbowl)
{
  CtkFishbowlPrivate *priv = ctk_fishbowl_get_instance_private (fishbowl);

  return priv->tick_id != 0;
}

static gint64
guess_refresh_interval (CdkFrameClock *frame_clock)
{
  gint64 interval;
  gint64 i;

  interval = G_MAXINT64;

  for (i = cdk_frame_clock_get_history_start (frame_clock);
       i < cdk_frame_clock_get_frame_counter (frame_clock);
       i++)
    {
      CdkFrameTimings *t, *before;
      gint64 ts, before_ts;

      t = cdk_frame_clock_get_timings (frame_clock, i);
      before = cdk_frame_clock_get_timings (frame_clock, i - 1);
      if (t == NULL || before == NULL)
        continue;

      ts = cdk_frame_timings_get_frame_time (t);
      before_ts = cdk_frame_timings_get_frame_time (before);
      if (ts == 0 || before_ts == 0)
        continue;

      interval = MIN (interval, ts - before_ts);
    }

  if (interval == G_MAXINT64)
    return 0;

  return interval;
}

static void
ctk_fishbowl_do_update (CtkFishbowl *fishbowl)
{
  CtkFishbowlPrivate *priv = ctk_fishbowl_get_instance_private (fishbowl);
  CdkFrameClock *frame_clock;
  CdkFrameTimings *start, *end;
  gint64 start_counter, end_counter;
  gint64 n_frames, expected_frames;
  gint64 start_timestamp, end_timestamp;
  gint64 interval;

  frame_clock = ctk_widget_get_frame_clock (CTK_WIDGET (fishbowl));
  if (frame_clock == NULL)
    return;

  start_counter = cdk_frame_clock_get_history_start (frame_clock);
  end_counter = cdk_frame_clock_get_frame_counter (frame_clock);
  start = cdk_frame_clock_get_timings (frame_clock, start_counter);
  for (end = cdk_frame_clock_get_timings (frame_clock, end_counter);
       end_counter > start_counter && end != NULL && !cdk_frame_timings_get_complete (end);
       end = cdk_frame_clock_get_timings (frame_clock, end_counter))
    end_counter--;
  if (end_counter - start_counter < 4)
    return;

  start_timestamp = cdk_frame_timings_get_presentation_time (start);
  end_timestamp = cdk_frame_timings_get_presentation_time (end);
  if (start_timestamp == 0 || end_timestamp == 0)
    {
      start_timestamp = cdk_frame_timings_get_frame_time (start);
      end_timestamp = cdk_frame_timings_get_frame_time (end);
    }

  n_frames = end_counter - start_counter;
  priv->framerate = ((double) n_frames) * G_USEC_PER_SEC / (end_timestamp - start_timestamp);
  g_object_notify_by_pspec (G_OBJECT (fishbowl), props[PROP_FRAMERATE]);

  if (!priv->benchmark)
    return;

  interval = cdk_frame_timings_get_refresh_interval (end);
  if (interval == 0)
    {
      interval = guess_refresh_interval (frame_clock);
      if (interval == 0)
        return;
    }
  expected_frames = round ((double) (end_timestamp - start_timestamp) / interval);

  if (n_frames >= expected_frames)
    {
      if (priv->last_benchmark_change > 0)
        priv->last_benchmark_change *= 2;
      else
        priv->last_benchmark_change = 1;
    }
  else if (n_frames + 1 < expected_frames)
    {
      if (priv->last_benchmark_change < 0)
        priv->last_benchmark_change--;
      else
        priv->last_benchmark_change = -1;
    }
  else
    {
      priv->last_benchmark_change = 0;
    }

  ctk_fishbowl_set_count (fishbowl, MAX (1, (int) priv->count + priv->last_benchmark_change));
}

static gboolean
ctk_fishbowl_tick (CtkWidget     *widget,
                   CdkFrameClock *frame_clock G_GNUC_UNUSED,
                   gpointer       unused G_GNUC_UNUSED)
{
  CtkFishbowl *fishbowl = CTK_FISHBOWL (widget);
  CtkFishbowlPrivate *priv = ctk_fishbowl_get_instance_private (fishbowl);
  GList *l;
  gint64 frame_time, elapsed;
  gboolean do_update;

  frame_time = cdk_frame_clock_get_frame_time (ctk_widget_get_frame_clock (widget));
  elapsed = frame_time - priv->last_frame_time;
  do_update = frame_time / priv->update_delay != priv->last_frame_time / priv->update_delay;
  priv->last_frame_time = frame_time;

  /* last frame was 0, so we're just starting to animate */
  if (elapsed == frame_time)
    return G_SOURCE_CONTINUE;

  for (l = priv->children; l; l = l->next)
    {
      CtkFishbowlChild *child;

      child = l->data;

      child->x += child->dx * ((double) elapsed / G_USEC_PER_SEC);
      child->y += child->dy * ((double) elapsed / G_USEC_PER_SEC);

      if (child->x <= 0)
        {
          child->x = 0;
          child->dx = new_speed ();
        }
      else if (child->x >= 1)
        {
          child->x = 1;
          child->dx =  - new_speed ();
        }

      if (child->y <= 0)
        {
          child->y = 0;
          child->dy = new_speed ();
        }
      else if (child->y >= 1)
        {
          child->y = 1;
          child->dy =  - new_speed ();
        }
    }

  ctk_widget_queue_allocate (widget);

  if (do_update)
    ctk_fishbowl_do_update (fishbowl);

  return G_SOURCE_CONTINUE;
}

void
ctk_fishbowl_set_animating (CtkFishbowl *fishbowl,
                            gboolean     animating)
{
  CtkFishbowlPrivate *priv = ctk_fishbowl_get_instance_private (fishbowl);

  if (ctk_fishbowl_get_animating (fishbowl) == animating)
    return;

  if (animating)
    {
      priv->tick_id = ctk_widget_add_tick_callback (CTK_WIDGET (fishbowl),
                                                    ctk_fishbowl_tick,
                                                    NULL,
                                                    NULL);
    }
  else
    {
      priv->last_frame_time = 0;
      ctk_widget_remove_tick_callback (CTK_WIDGET (fishbowl), priv->tick_id);
      priv->tick_id = 0;
      priv->framerate = 0;
      g_object_notify_by_pspec (G_OBJECT (fishbowl), props[PROP_FRAMERATE]);
    }

  g_object_notify_by_pspec (G_OBJECT (fishbowl), props[PROP_ANIMATING]);
}

double
ctk_fishbowl_get_framerate (CtkFishbowl *fishbowl)
{
  CtkFishbowlPrivate *priv = ctk_fishbowl_get_instance_private (fishbowl);

  return priv->framerate;
}

gint64
ctk_fishbowl_get_update_delay (CtkFishbowl *fishbowl)
{
  CtkFishbowlPrivate *priv = ctk_fishbowl_get_instance_private (fishbowl);

  return priv->update_delay;
}

void
ctk_fishbowl_set_update_delay (CtkFishbowl *fishbowl,
                               gint64       update_delay)
{
  CtkFishbowlPrivate *priv = ctk_fishbowl_get_instance_private (fishbowl);

  if (priv->update_delay == update_delay)
    return;

  priv->update_delay = update_delay;

  g_object_notify_by_pspec (G_OBJECT (fishbowl), props[PROP_UPDATE_DELAY]);
}

void
ctk_fishbowl_set_creation_func (CtkFishbowl         *fishbowl,
                                CtkFishCreationFunc  creation_func)
{
  CtkFishbowlPrivate *priv = ctk_fishbowl_get_instance_private (fishbowl);

  g_object_freeze_notify (G_OBJECT (fishbowl));

  ctk_fishbowl_set_count (fishbowl, 0);
  priv->last_benchmark_change = 0;

  priv->creation_func = creation_func;

  ctk_fishbowl_set_count (fishbowl, 1);

  g_object_thaw_notify (G_OBJECT (fishbowl));
}
