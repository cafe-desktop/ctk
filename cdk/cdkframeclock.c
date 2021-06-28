/* CDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the CTK+ Team and others 1997-2010.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#include "config.h"

#include "cdkframeclockprivate.h"
#include "cdkinternals.h"
#include "cdkprofilerprivate.h"

/**
 * SECTION:cdkframeclock
 * @Short_description: Frame clock syncs painting to a window or display
 * @Title: Frame clock
 *
 * A #CdkFrameClock tells the application when to update and repaint a
 * window. This may be synced to the vertical refresh rate of the
 * monitor, for example. Even when the frame clock uses a simple timer
 * rather than a hardware-based vertical sync, the frame clock helps
 * because it ensures everything paints at the same time (reducing the
 * total number of frames). The frame clock can also automatically
 * stop painting when it knows the frames will not be visible, or
 * scale back animation framerates.
 *
 * #CdkFrameClock is designed to be compatible with an OpenGL-based
 * implementation or with mozRequestAnimationFrame in Firefox,
 * for example.
 *
 * A frame clock is idle until someone requests a frame with
 * cdk_frame_clock_request_phase(). At some later point that makes
 * sense for the synchronization being implemented, the clock will
 * process a frame and emit signals for each phase that has been
 * requested. (See the signals of the #CdkFrameClock class for
 * documentation of the phases. %CDK_FRAME_CLOCK_PHASE_UPDATE and the
 * #CdkFrameClock::update signal are most interesting for application
 * writers, and are used to update the animations, using the frame time
 * given by cdk_frame_clock_get_frame_time().
 *
 * The frame time is reported in microseconds and generally in the same
 * timescale as g_get_monotonic_time(), however, it is not the same
 * as g_get_monotonic_time(). The frame time does not advance during
 * the time a frame is being painted, and outside of a frame, an attempt
 * is made so that all calls to cdk_frame_clock_get_frame_time() that
 * are called at a “similar” time get the same value. This means that
 * if different animations are timed by looking at the difference in
 * time between an initial value from cdk_frame_clock_get_frame_time()
 * and the value inside the #CdkFrameClock::update signal of the clock,
 * they will stay exactly synchronized.
 */

enum {
  FLUSH_EVENTS,
  BEFORE_PAINT,
  UPDATE,
  LAYOUT,
  PAINT,
  AFTER_PAINT,
  RESUME_EVENTS,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

#ifdef G_ENABLE_DEBUG
static guint fps_counter;
#endif

#define FRAME_HISTORY_MAX_LENGTH 16

struct _CdkFrameClockPrivate
{
  gint64 frame_counter;
  gint n_timings;
  gint current;
  CdkFrameTimings *timings[FRAME_HISTORY_MAX_LENGTH];
};

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (CdkFrameClock, cdk_frame_clock, G_TYPE_OBJECT)

static void
cdk_frame_clock_finalize (GObject *object)
{
  CdkFrameClockPrivate *priv = CDK_FRAME_CLOCK (object)->priv;
  int i;

  for (i = 0; i < FRAME_HISTORY_MAX_LENGTH; i++)
    if (priv->timings[i] != 0)
      cdk_frame_timings_unref (priv->timings[i]);

  G_OBJECT_CLASS (cdk_frame_clock_parent_class)->finalize (object);
}

static void
cdk_frame_clock_class_init (CdkFrameClockClass *klass)
{
  GObjectClass *gobject_class = (GObjectClass*) klass;

  gobject_class->finalize     = cdk_frame_clock_finalize;

  /**
   * CdkFrameClock::flush-events:
   * @clock: the frame clock emitting the signal
   *
   * This signal is used to flush pending motion events that
   * are being batched up and compressed together. Applications
   * should not handle this signal.
   */
  signals[FLUSH_EVENTS] =
    g_signal_new (g_intern_static_string ("flush-events"),
                  CDK_TYPE_FRAME_CLOCK,
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  /**
   * CdkFrameClock::before-paint:
   * @clock: the frame clock emitting the signal
   *
   * This signal begins processing of the frame. Applications
   * should generally not handle this signal.
   */
  signals[BEFORE_PAINT] =
    g_signal_new (g_intern_static_string ("before-paint"),
                  CDK_TYPE_FRAME_CLOCK,
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  /**
   * CdkFrameClock::update:
   * @clock: the frame clock emitting the signal
   *
   * This signal is emitted as the first step of toolkit and
   * application processing of the frame. Animations should
   * be updated using cdk_frame_clock_get_frame_time().
   * Applications can connect directly to this signal, or
   * use ctk_widget_add_tick_callback() as a more convenient
   * interface.
   */
  signals[UPDATE] =
    g_signal_new (g_intern_static_string ("update"),
                  CDK_TYPE_FRAME_CLOCK,
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  /**
   * CdkFrameClock::layout:
   * @clock: the frame clock emitting the signal
   *
   * This signal is emitted as the second step of toolkit and
   * application processing of the frame. Any work to update
   * sizes and positions of application elements should be
   * performed. CTK+ normally handles this internally.
   */
  signals[LAYOUT] =
    g_signal_new (g_intern_static_string ("layout"),
                  CDK_TYPE_FRAME_CLOCK,
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  /**
   * CdkFrameClock::paint:
   * @clock: the frame clock emitting the signal
   *
   * This signal is emitted as the third step of toolkit and
   * application processing of the frame. The frame is
   * repainted. CDK normally handles this internally and
   * produces expose events, which are turned into CTK+
   * #CtkWidget::draw signals.
   */
  signals[PAINT] =
    g_signal_new (g_intern_static_string ("paint"),
                  CDK_TYPE_FRAME_CLOCK,
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  /**
   * CdkFrameClock::after-paint:
   * @clock: the frame clock emitting the signal
   *
   * This signal ends processing of the frame. Applications
   * should generally not handle this signal.
   */
  signals[AFTER_PAINT] =
    g_signal_new (g_intern_static_string ("after-paint"),
                  CDK_TYPE_FRAME_CLOCK,
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  /**
   * CdkFrameClock::resume-events:
   * @clock: the frame clock emitting the signal
   *
   * This signal is emitted after processing of the frame is
   * finished, and is handled internally by CTK+ to resume normal
   * event processing. Applications should not handle this signal.
   */
  signals[RESUME_EVENTS] =
    g_signal_new (g_intern_static_string ("resume-events"),
                  CDK_TYPE_FRAME_CLOCK,
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);
}

static void
cdk_frame_clock_init (CdkFrameClock *clock)
{
  CdkFrameClockPrivate *priv;

  clock->priv = priv = cdk_frame_clock_get_instance_private (clock);

  priv->frame_counter = -1;
  priv->current = FRAME_HISTORY_MAX_LENGTH - 1;

#ifdef G_ENABLE_DEBUG
  if (fps_counter == 0)
    fps_counter = cdk_profiler_define_counter ("fps", "Frames per Second");
#endif
}

/**
 * cdk_frame_clock_get_frame_time:
 * @frame_clock: a #CdkFrameClock
 *
 * Gets the time that should currently be used for animations.  Inside
 * the processing of a frame, it’s the time used to compute the
 * animation position of everything in a frame. Outside of a frame, it's
 * the time of the conceptual “previous frame,” which may be either
 * the actual previous frame time, or if that’s too old, an updated
 * time.
 *
 * Since: 3.8
 * Returns: a timestamp in microseconds, in the timescale of
 *  of g_get_monotonic_time().
 */
gint64
cdk_frame_clock_get_frame_time (CdkFrameClock *frame_clock)
{
  g_return_val_if_fail (CDK_IS_FRAME_CLOCK (frame_clock), 0);

  return CDK_FRAME_CLOCK_GET_CLASS (frame_clock)->get_frame_time (frame_clock);
}

/**
 * cdk_frame_clock_request_phase:
 * @frame_clock: a #CdkFrameClock
 * @phase: the phase that is requested
 *
 * Asks the frame clock to run a particular phase. The signal
 * corresponding the requested phase will be emitted the next
 * time the frame clock processes. Multiple calls to
 * cdk_frame_clock_request_phase() will be combined together
 * and only one frame processed. If you are displaying animated
 * content and want to continually request the
 * %CDK_FRAME_CLOCK_PHASE_UPDATE phase for a period of time,
 * you should use cdk_frame_clock_begin_updating() instead, since
 * this allows CTK+ to adjust system parameters to get maximally
 * smooth animations.
 *
 * Since: 3.8
 */
void
cdk_frame_clock_request_phase (CdkFrameClock      *frame_clock,
                               CdkFrameClockPhase  phase)
{
  g_return_if_fail (CDK_IS_FRAME_CLOCK (frame_clock));

  CDK_FRAME_CLOCK_GET_CLASS (frame_clock)->request_phase (frame_clock, phase);
}

/**
 * cdk_frame_clock_begin_updating:
 * @frame_clock: a #CdkFrameClock
 *
 * Starts updates for an animation. Until a matching call to
 * cdk_frame_clock_end_updating() is made, the frame clock will continually
 * request a new frame with the %CDK_FRAME_CLOCK_PHASE_UPDATE phase.
 * This function may be called multiple times and frames will be
 * requested until cdk_frame_clock_end_updating() is called the same
 * number of times.
 *
 * Since: 3.8
 */
void
cdk_frame_clock_begin_updating (CdkFrameClock *frame_clock)
{
  g_return_if_fail (CDK_IS_FRAME_CLOCK (frame_clock));

  CDK_FRAME_CLOCK_GET_CLASS (frame_clock)->begin_updating (frame_clock);
}

/**
 * cdk_frame_clock_end_updating:
 * @frame_clock: a #CdkFrameClock
 *
 * Stops updates for an animation. See the documentation for
 * cdk_frame_clock_begin_updating().
 *
 * Since: 3.8
 */
void
cdk_frame_clock_end_updating (CdkFrameClock *frame_clock)
{
  g_return_if_fail (CDK_IS_FRAME_CLOCK (frame_clock));

  CDK_FRAME_CLOCK_GET_CLASS (frame_clock)->end_updating (frame_clock);
}

void
_cdk_frame_clock_freeze (CdkFrameClock *clock)
{
  g_return_if_fail (CDK_IS_FRAME_CLOCK (clock));

  CDK_FRAME_CLOCK_GET_CLASS (clock)->freeze (clock);
}


void
_cdk_frame_clock_thaw (CdkFrameClock *clock)
{
  g_return_if_fail (CDK_IS_FRAME_CLOCK (clock));

  CDK_FRAME_CLOCK_GET_CLASS (clock)->thaw (clock);
}

/**
 * cdk_frame_clock_get_frame_counter:
 * @frame_clock: a #CdkFrameClock
 *
 * A #CdkFrameClock maintains a 64-bit counter that increments for
 * each frame drawn.
 *
 * Returns: inside frame processing, the value of the frame counter
 *  for the current frame. Outside of frame processing, the frame
 *   counter for the last frame.
 * Since: 3.8
 */
gint64
cdk_frame_clock_get_frame_counter (CdkFrameClock *frame_clock)
{
  CdkFrameClockPrivate *priv;

  g_return_val_if_fail (CDK_IS_FRAME_CLOCK (frame_clock), 0);

  priv = frame_clock->priv;

  return priv->frame_counter;
}

/**
 * cdk_frame_clock_get_history_start:
 * @frame_clock: a #CdkFrameClock
 *
 * #CdkFrameClock internally keeps a history of #CdkFrameTimings
 * objects for recent frames that can be retrieved with
 * cdk_frame_clock_get_timings(). The set of stored frames
 * is the set from the counter values given by
 * cdk_frame_clock_get_history_start() and
 * cdk_frame_clock_get_frame_counter(), inclusive.
 *
 * Returns: the frame counter value for the oldest frame
 *  that is available in the internal frame history of the
 *  #CdkFrameClock.
 * Since: 3.8
 */
gint64
cdk_frame_clock_get_history_start (CdkFrameClock *frame_clock)
{
  CdkFrameClockPrivate *priv;

  g_return_val_if_fail (CDK_IS_FRAME_CLOCK (frame_clock), 0);

  priv = frame_clock->priv;

  return priv->frame_counter + 1 - priv->n_timings;
}

void
_cdk_frame_clock_begin_frame (CdkFrameClock *frame_clock)
{
  CdkFrameClockPrivate *priv;

  g_return_if_fail (CDK_IS_FRAME_CLOCK (frame_clock));

  priv = frame_clock->priv;

  priv->frame_counter++;
  priv->current = (priv->current + 1) % FRAME_HISTORY_MAX_LENGTH;

  /* Try to steal the previous frame timing instead of discarding
   * and allocating a new one.
   */
  if G_LIKELY (priv->n_timings == FRAME_HISTORY_MAX_LENGTH &&
               _cdk_frame_timings_steal (priv->timings[priv->current],
                                         priv->frame_counter))
    return;

  if (priv->n_timings < FRAME_HISTORY_MAX_LENGTH)
    priv->n_timings++;
  else
    cdk_frame_timings_unref (priv->timings[priv->current]);

  priv->timings[priv->current] = _cdk_frame_timings_new (priv->frame_counter);
}

/**
 * cdk_frame_clock_get_timings:
 * @frame_clock: a #CdkFrameClock
 * @frame_counter: the frame counter value identifying the frame to
 *  be received.
 *
 * Retrieves a #CdkFrameTimings object holding timing information
 * for the current frame or a recent frame. The #CdkFrameTimings
 * object may not yet be complete: see cdk_frame_timings_get_complete().
 *
 * Returns: (nullable) (transfer none): the #CdkFrameTimings object for
 *  the specified frame, or %NULL if it is not available. See
 *  cdk_frame_clock_get_history_start().
 * Since: 3.8
 */
CdkFrameTimings *
cdk_frame_clock_get_timings (CdkFrameClock *frame_clock,
                             gint64         frame_counter)
{
  CdkFrameClockPrivate *priv;
  gint pos;

  g_return_val_if_fail (CDK_IS_FRAME_CLOCK (frame_clock), NULL);

  priv = frame_clock->priv;

  if (frame_counter > priv->frame_counter)
    return NULL;

  if (frame_counter <= priv->frame_counter - priv->n_timings)
    return NULL;

  pos = (priv->current - (priv->frame_counter - frame_counter) + FRAME_HISTORY_MAX_LENGTH) % FRAME_HISTORY_MAX_LENGTH;

  return priv->timings[pos];
}

/**
 * cdk_frame_clock_get_current_timings:
 * @frame_clock: a #CdkFrameClock
 *
 * Gets the frame timings for the current frame.
 *
 * Returns: (nullable) (transfer none): the #CdkFrameTimings for the
 *  frame currently being processed, or even no frame is being
 *  processed, for the previous frame. Before any frames have been
 *  processed, returns %NULL.
 * Since: 3.8
 */
CdkFrameTimings *
cdk_frame_clock_get_current_timings (CdkFrameClock *frame_clock)
{
  CdkFrameClockPrivate *priv;

  g_return_val_if_fail (CDK_IS_FRAME_CLOCK (frame_clock), 0);

  priv = frame_clock->priv;

  return cdk_frame_clock_get_timings (frame_clock, priv->frame_counter);
}


#ifdef G_ENABLE_DEBUG
void
_cdk_frame_clock_debug_print_timings (CdkFrameClock   *clock,
                                      CdkFrameTimings *timings)
{
  GString *str;

  gint64 previous_frame_time = 0;
  gint64 previous_smoothed_frame_time = 0;
  CdkFrameTimings *previous_timings = cdk_frame_clock_get_timings (clock,
                                                                   timings->frame_counter - 1);

  if (previous_timings != NULL)
    {
      previous_frame_time = previous_timings->frame_time;
      previous_smoothed_frame_time = previous_timings->smoothed_frame_time;
    }

  str = g_string_new ("");

  g_string_append_printf (str, "%5" G_GINT64_FORMAT ":", timings->frame_counter);
  if (previous_frame_time != 0)
    {
      g_string_append_printf (str, " interval=%-4.1f", (timings->frame_time - previous_frame_time) / 1000.);
      g_string_append_printf (str, timings->slept_before ?  " (sleep)" : "        ");
      g_string_append_printf (str, " smoothed=%4.1f / %-4.1f",
                              (timings->smoothed_frame_time - timings->frame_time) / 1000.,
                              (timings->smoothed_frame_time - previous_smoothed_frame_time) / 1000.);
    }
  if (timings->layout_start_time != 0)
    g_string_append_printf (str, " layout_start=%-4.1f", (timings->layout_start_time - timings->frame_time) / 1000.);
  if (timings->paint_start_time != 0)
    g_string_append_printf (str, " paint_start=%-4.1f", (timings->paint_start_time - timings->frame_time) / 1000.);
  if (timings->frame_end_time != 0)
    g_string_append_printf (str, " frame_end=%-4.1f", (timings->frame_end_time - timings->frame_time) / 1000.);
  if (timings->drawn_time != 0)
    g_string_append_printf (str, " drawn=%-4.1f", (timings->drawn_time - timings->frame_time) / 1000.);
  if (timings->presentation_time != 0)
    g_string_append_printf (str, " present=%-4.1f", (timings->presentation_time - timings->frame_time) / 1000.);
  if (timings->predicted_presentation_time != 0)
    g_string_append_printf (str, " predicted=%-4.1f", (timings->predicted_presentation_time - timings->frame_time) / 1000.);
  if (timings->refresh_interval != 0)
    g_string_append_printf (str, " refresh_interval=%-4.1f", timings->refresh_interval / 1000.);

  g_message ("%s", str->str);
  g_string_free (str, TRUE);
}
#endif /* G_ENABLE_DEBUG */

#define DEFAULT_REFRESH_INTERVAL 16667 /* 16.7ms (1/60th second) */
#define MAX_HISTORY_AGE 150000         /* 150ms */

/**
 * cdk_frame_clock_get_refresh_info:
 * @frame_clock: a #CdkFrameClock
 * @base_time: base time for determining a presentaton time
 * @refresh_interval_return: (out) (optional): a location to store the
 * determined refresh interval, or %NULL. A default refresh interval of
 * 1/60th of a second will be stored if no history is present.
 * @presentation_time_return: (out): a location to store the next
 *  candidate presentation time after the given base time.
 *  0 will be will be stored if no history is present.
 *
 * Using the frame history stored in the frame clock, finds the last
 * known presentation time and refresh interval, and assuming that
 * presentation times are separated by the refresh interval,
 * predicts a presentation time that is a multiple of the refresh
 * interval after the last presentation time, and later than @base_time.
 *
 * Since: 3.8
 */
void
cdk_frame_clock_get_refresh_info (CdkFrameClock *frame_clock,
                                  gint64         base_time,
                                  gint64        *refresh_interval_return,
                                  gint64        *presentation_time_return)
{
  gint64 frame_counter;
  gint64 default_refresh_interval = DEFAULT_REFRESH_INTERVAL;

  g_return_if_fail (CDK_IS_FRAME_CLOCK (frame_clock));

  frame_counter = cdk_frame_clock_get_frame_counter (frame_clock);

  while (TRUE)
    {
      CdkFrameTimings *timings = cdk_frame_clock_get_timings (frame_clock, frame_counter);
      gint64 presentation_time;
      gint64 refresh_interval;

      if (timings == NULL)
        break;

      refresh_interval = timings->refresh_interval;
      presentation_time = timings->presentation_time;

      if (refresh_interval == 0)
        refresh_interval = default_refresh_interval;
      else
        default_refresh_interval = refresh_interval;

      if (presentation_time != 0)
        {
          if (presentation_time > base_time - MAX_HISTORY_AGE &&
              presentation_time_return)
            {
              if (refresh_interval_return)
                *refresh_interval_return = refresh_interval;

              while (presentation_time < base_time)
                presentation_time += refresh_interval;

              if (presentation_time_return)
                *presentation_time_return = presentation_time;

              return;
            }

          break;
        }

      frame_counter--;
    }

  if (presentation_time_return)
    *presentation_time_return = 0;
  if (refresh_interval_return)
    *refresh_interval_return = default_refresh_interval;
}

void
_cdk_frame_clock_emit_flush_events (CdkFrameClock *frame_clock)
{
  g_signal_emit (frame_clock, signals[FLUSH_EVENTS], 0);
}

void
_cdk_frame_clock_emit_before_paint (CdkFrameClock *frame_clock)
{
  g_signal_emit (frame_clock, signals[BEFORE_PAINT], 0);
}

void
_cdk_frame_clock_emit_update (CdkFrameClock *frame_clock)
{
  g_signal_emit (frame_clock, signals[UPDATE], 0);
}

void
_cdk_frame_clock_emit_layout (CdkFrameClock *frame_clock)
{
  g_signal_emit (frame_clock, signals[LAYOUT], 0);
}

void
_cdk_frame_clock_emit_paint (CdkFrameClock *frame_clock)
{
  g_signal_emit (frame_clock, signals[PAINT], 0);
}

void
_cdk_frame_clock_emit_after_paint (CdkFrameClock *frame_clock)
{
  g_signal_emit (frame_clock, signals[AFTER_PAINT], 0);
}

void
_cdk_frame_clock_emit_resume_events (CdkFrameClock *frame_clock)
{
  g_signal_emit (frame_clock, signals[RESUME_EVENTS], 0);
}

#ifdef G_ENABLE_DEBUG
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

static double
frame_clock_get_fps (CdkFrameClock *frame_clock)
{
  CdkFrameTimings *start, *end;
  gint64 start_counter, end_counter;
  gint64 start_timestamp, end_timestamp;
  gint64 interval;

  start_counter = cdk_frame_clock_get_history_start (frame_clock);
  end_counter = cdk_frame_clock_get_frame_counter (frame_clock);
  start = cdk_frame_clock_get_timings (frame_clock, start_counter);
  for (end = cdk_frame_clock_get_timings (frame_clock, end_counter);
       end_counter > start_counter && end != NULL && !cdk_frame_timings_get_complete (end);
       end = cdk_frame_clock_get_timings (frame_clock, end_counter))
    end_counter--;
  if (end_counter - start_counter < 4)
    return 0.0;

  start_timestamp = cdk_frame_timings_get_presentation_time (start);
  end_timestamp = cdk_frame_timings_get_presentation_time (end);
  if (start_timestamp == 0 || end_timestamp == 0)
    {
      start_timestamp = cdk_frame_timings_get_frame_time (start);
      end_timestamp = cdk_frame_timings_get_frame_time (end);
    }
  interval = cdk_frame_timings_get_refresh_interval (end);
  if (interval == 0)
    {
      interval = guess_refresh_interval (frame_clock);
      if (interval == 0)
        return 0.0;
    }   
    
  return ((double) end_counter - start_counter) * G_USEC_PER_SEC / (end_timestamp - start_timestamp);
}
#endif

void
_cdk_frame_clock_add_timings_to_profiler (CdkFrameClock   *clock,
                                          CdkFrameTimings *timings)
{
#ifdef G_ENABLE_DEBUG
  cdk_profiler_add_mark (timings->frame_time * 1000,
                         (timings->frame_end_time - timings->frame_time) * 1000,
                         "frame", "");

  if (timings->layout_start_time != 0)
    cdk_profiler_add_mark (timings->layout_start_time * 1000,
                           (timings->paint_start_time - timings->layout_start_time) * 1000,
                            "layout", "");

  if (timings->paint_start_time != 0)
    cdk_profiler_add_mark (timings->paint_start_time * 1000,
                           (timings->frame_end_time - timings->paint_start_time) * 1000,
                            "paint", "");

  if (timings->presentation_time != 0)
    cdk_profiler_add_mark (timings->presentation_time * 1000,
                           0,
                           "presentation", "");

  cdk_profiler_set_counter (fps_counter,
                            timings->frame_end_time * 1000,
                            frame_clock_get_fps (clock));
#endif
}
