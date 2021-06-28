/* GDK - The GIMP Drawing Kit
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

/* Uninstalled header, internal to GDK */

#ifndef __GDK_FRAME_CLOCK_PRIVATE_H__
#define __GDK_FRAME_CLOCK_PRIVATE_H__

#include <cdk/cdkframeclock.h>

G_BEGIN_DECLS

/**
 * CdkFrameClock:
 * @parent_instance: The parent instance.
 */
struct _CdkFrameClock
{
  GObject parent_instance;

  /*< private >*/
  CdkFrameClockPrivate *priv;
};

/**
 * CdkFrameClockClass:
 * @parent_class: The parent class.

 * @get_frame_time: Gets the time that should currently be used for
 *    animations.
 * @request_phase: Asks the frame clock to run a particular phase.
 * @begin_updating: Starts updates for an animation.
 * @end_updating: Stops updates for an animation.
 * @freeze: 
 * @thaw: 
 */
struct _CdkFrameClockClass
{
  GObjectClass parent_class;

  /*< public >*/

  gint64   (* get_frame_time) (CdkFrameClock *clock);

  void     (* request_phase)  (CdkFrameClock      *clock,
                               CdkFrameClockPhase  phase);
  void     (* begin_updating) (CdkFrameClock      *clock);
  void     (* end_updating)   (CdkFrameClock      *clock);

  void     (* freeze)         (CdkFrameClock *clock);
  void     (* thaw)           (CdkFrameClock *clock);

  /* signals */
  /* void (* flush_events)       (CdkFrameClock *clock); */
  /* void (* before_paint)       (CdkFrameClock *clock); */
  /* void (* update)             (CdkFrameClock *clock); */
  /* void (* layout)             (CdkFrameClock *clock); */
  /* void (* paint)              (CdkFrameClock *clock); */
  /* void (* after_paint)        (CdkFrameClock *clock); */
  /* void (* resume_events)      (CdkFrameClock *clock); */
};

struct _CdkFrameTimings
{
  /*< private >*/
  guint ref_count;

  gint64 frame_counter;
  guint64 cookie;
  gint64 frame_time;
  gint64 smoothed_frame_time;
  gint64 drawn_time;
  gint64 presentation_time;
  gint64 refresh_interval;
  gint64 predicted_presentation_time;

#ifdef G_ENABLE_DEBUG
  gint64 layout_start_time;
  gint64 paint_start_time;
  gint64 frame_end_time;
#endif /* G_ENABLE_DEBUG */

  guint complete : 1;
  guint slept_before : 1;
};

void _cdk_frame_clock_freeze (CdkFrameClock *clock);
void _cdk_frame_clock_thaw   (CdkFrameClock *clock);

void _cdk_frame_clock_begin_frame         (CdkFrameClock   *clock);
void _cdk_frame_clock_debug_print_timings (CdkFrameClock   *clock,
                                           CdkFrameTimings *timings);
void _cdk_frame_clock_add_timings_to_profiler (CdkFrameClock *frame_clock,
                                               CdkFrameTimings *timings);

CdkFrameTimings *_cdk_frame_timings_new   (gint64           frame_counter);
gboolean         _cdk_frame_timings_steal (CdkFrameTimings *timings,
                                           gint64           frame_counter);

void _cdk_frame_clock_emit_flush_events  (CdkFrameClock *frame_clock);
void _cdk_frame_clock_emit_before_paint  (CdkFrameClock *frame_clock);
void _cdk_frame_clock_emit_update        (CdkFrameClock *frame_clock);
void _cdk_frame_clock_emit_layout        (CdkFrameClock *frame_clock);
void _cdk_frame_clock_emit_paint         (CdkFrameClock *frame_clock);
void _cdk_frame_clock_emit_after_paint   (CdkFrameClock *frame_clock);
void _cdk_frame_clock_emit_resume_events (CdkFrameClock *frame_clock);

G_END_DECLS

#endif /* __GDK_FRAME_CLOCK_PRIVATE_H__ */
