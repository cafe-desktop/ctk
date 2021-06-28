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

#ifndef __GDK_FRAME_CLOCK_H__
#define __GDK_FRAME_CLOCK_H__

#if !defined (__GDK_H_INSIDE__) && !defined (GDK_COMPILATION)
#error "Only <cdk/cdk.h> can be included directly."
#endif

#include <cdk/cdkframetimings.h>

G_BEGIN_DECLS

#define GDK_TYPE_FRAME_CLOCK            (cdk_frame_clock_get_type ())
#define GDK_FRAME_CLOCK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GDK_TYPE_FRAME_CLOCK, CdkFrameClock))
#define GDK_FRAME_CLOCK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_FRAME_CLOCK, CdkFrameClockClass))
#define GDK_IS_FRAME_CLOCK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDK_TYPE_FRAME_CLOCK))
#define GDK_IS_FRAME_CLOCK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_FRAME_CLOCK))
#define GDK_FRAME_CLOCK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_FRAME_CLOCK, CdkFrameClockClass))

typedef struct _CdkFrameClock              CdkFrameClock;
typedef struct _CdkFrameClockPrivate       CdkFrameClockPrivate;
typedef struct _CdkFrameClockClass         CdkFrameClockClass;

/**
 * CdkFrameClockPhase:
 * @GDK_FRAME_CLOCK_PHASE_NONE: no phase
 * @GDK_FRAME_CLOCK_PHASE_FLUSH_EVENTS: corresponds to CdkFrameClock::flush-events. Should not be handled by applications.
 * @GDK_FRAME_CLOCK_PHASE_BEFORE_PAINT: corresponds to CdkFrameClock::before-paint. Should not be handled by applications.
 * @GDK_FRAME_CLOCK_PHASE_UPDATE: corresponds to CdkFrameClock::update.
 * @GDK_FRAME_CLOCK_PHASE_LAYOUT: corresponds to CdkFrameClock::layout.
 * @GDK_FRAME_CLOCK_PHASE_PAINT: corresponds to CdkFrameClock::paint.
 * @GDK_FRAME_CLOCK_PHASE_RESUME_EVENTS: corresponds to CdkFrameClock::resume-events. Should not be handled by applications.
 * @GDK_FRAME_CLOCK_PHASE_AFTER_PAINT: corresponds to CdkFrameClock::after-paint. Should not be handled by applications.
 *
 * #CdkFrameClockPhase is used to represent the different paint clock
 * phases that can be requested. The elements of the enumeration
 * correspond to the signals of #CdkFrameClock.
 *
 * Since: 3.8
 **/
typedef enum {
  GDK_FRAME_CLOCK_PHASE_NONE          = 0,
  GDK_FRAME_CLOCK_PHASE_FLUSH_EVENTS  = 1 << 0,
  GDK_FRAME_CLOCK_PHASE_BEFORE_PAINT  = 1 << 1,
  GDK_FRAME_CLOCK_PHASE_UPDATE        = 1 << 2,
  GDK_FRAME_CLOCK_PHASE_LAYOUT        = 1 << 3,
  GDK_FRAME_CLOCK_PHASE_PAINT         = 1 << 4,
  GDK_FRAME_CLOCK_PHASE_RESUME_EVENTS = 1 << 5,
  GDK_FRAME_CLOCK_PHASE_AFTER_PAINT   = 1 << 6
} CdkFrameClockPhase;

GDK_AVAILABLE_IN_3_8
GType    cdk_frame_clock_get_type             (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_3_8
gint64   cdk_frame_clock_get_frame_time            (CdkFrameClock *frame_clock);

GDK_AVAILABLE_IN_3_8
void               cdk_frame_clock_request_phase (CdkFrameClock      *frame_clock,
                                                  CdkFrameClockPhase  phase);

GDK_AVAILABLE_IN_3_8
void               cdk_frame_clock_begin_updating (CdkFrameClock      *frame_clock);
GDK_AVAILABLE_IN_3_8
void               cdk_frame_clock_end_updating   (CdkFrameClock      *frame_clock);

/* Frame history */
GDK_AVAILABLE_IN_3_8
gint64           cdk_frame_clock_get_frame_counter (CdkFrameClock *frame_clock);
GDK_AVAILABLE_IN_3_8
gint64           cdk_frame_clock_get_history_start (CdkFrameClock *frame_clock);
GDK_AVAILABLE_IN_3_8
CdkFrameTimings *cdk_frame_clock_get_timings       (CdkFrameClock *frame_clock,
                                                    gint64         frame_counter);

GDK_AVAILABLE_IN_3_8
CdkFrameTimings *cdk_frame_clock_get_current_timings (CdkFrameClock *frame_clock);

GDK_AVAILABLE_IN_3_8
void cdk_frame_clock_get_refresh_info (CdkFrameClock *frame_clock,
                                       gint64         base_time,
                                       gint64        *refresh_interval_return,
                                       gint64        *presentation_time_return);

G_END_DECLS

#endif /* __GDK_FRAME_CLOCK_H__ */
