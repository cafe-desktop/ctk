/*
 * Copyright © 2016 Endless Mobile Inc.
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
 * Authors: Matthew Watson <mattdangerw@gmail.com>
 */

#ifndef __CTK_PROGRESS_TRACKER_PRIVATE_H__
#define __CTK_PROGRESS_TRACKER_PRIVATE_H__

#include <glib-object.h>
#include "ctkcsseasevalueprivate.h"

G_BEGIN_DECLS

typedef enum {
  CTK_PROGRESS_STATE_BEFORE,
  CTK_PROGRESS_STATE_DURING,
  CTK_PROGRESS_STATE_AFTER,
} CtkProgressState;

typedef struct _CtkProgressTracker CtkProgressTracker;

struct _CtkProgressTracker
{
  gboolean is_running;
  guint64 last_frame_time;
  guint64 duration;
  gdouble iteration;
  gdouble iteration_count;
};

void                 ctk_progress_tracker_init_copy           (CtkProgressTracker *source,
                                                               CtkProgressTracker *dest);

void                 ctk_progress_tracker_start               (CtkProgressTracker *tracker,
                                                               guint64 duration,
                                                               gint64 delay,
                                                               gdouble iteration_count);

void                 ctk_progress_tracker_finish              (CtkProgressTracker *tracker);

void                 ctk_progress_tracker_advance_frame       (CtkProgressTracker *tracker,
                                                               guint64 frame_time);

void                 ctk_progress_tracker_skip_frame          (CtkProgressTracker *tracker,
                                                               guint64 frame_time);

CtkProgressState     ctk_progress_tracker_get_state           (CtkProgressTracker *tracker);

gdouble              ctk_progress_tracker_get_iteration       (CtkProgressTracker *tracker);

guint64              ctk_progress_tracker_get_iteration_cycle (CtkProgressTracker *tracker);

gdouble              ctk_progress_tracker_get_progress        (CtkProgressTracker *tracker,
                                                               gboolean reverse);

gdouble              ctk_progress_tracker_get_ease_out_cubic  (CtkProgressTracker *tracker,
                                                               gboolean reverse);

G_END_DECLS

#endif /* __CTK_PROGRESS_TRACKER_PRIVATE_H__ */
