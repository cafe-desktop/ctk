/* CDK - The GIMP Drawing Kit
 * Copyright (C) 2012 Red Hat, Inc.
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

#include <string.h>

#include "cdkframeclockprivate.h"

/**
 * SECTION:cdkframetimings
 * @Short_description: Object holding timing information for a single frame
 * @Title: Frame timings
 *
 * A #CdkFrameTimings object holds timing information for a single frame
 * of the application’s displays. To retrieve #CdkFrameTimings objects,
 * use cdk_frame_clock_get_timings() or cdk_frame_clock_get_current_timings().
 * The information in #CdkFrameTimings is useful for precise synchronization
 * of video with the event or audio streams, and for measuring
 * quality metrics for the application’s display, such as latency and jitter.
 */

G_DEFINE_BOXED_TYPE (CdkFrameTimings, cdk_frame_timings,
                     cdk_frame_timings_ref,
                     cdk_frame_timings_unref)

CdkFrameTimings *
_cdk_frame_timings_new (gint64 frame_counter)
{
  CdkFrameTimings *timings;

  timings = g_slice_new0 (CdkFrameTimings);
  timings->ref_count = 1;
  timings->frame_counter = frame_counter;

  return timings;
}

gboolean
_cdk_frame_timings_steal (CdkFrameTimings *timings,
                          gint64           frame_counter)
{
  if (timings->ref_count == 1)
    {
      memset (timings, 0, sizeof *timings);
      timings->ref_count = 1;
      timings->frame_counter = frame_counter;
      return TRUE;
    }

  return FALSE;
}

/**
 * cdk_frame_timings_ref:
 * @timings: a #CdkFrameTimings
 *
 * Increases the reference count of @timings.
 *
 * Returns: @timings
 * Since: 3.8
 */
CdkFrameTimings *
cdk_frame_timings_ref (CdkFrameTimings *timings)
{
  g_return_val_if_fail (timings != NULL, NULL);

  timings->ref_count++;

  return timings;
}

/**
 * cdk_frame_timings_unref:
 * @timings: a #CdkFrameTimings
 *
 * Decreases the reference count of @timings. If @timings
 * is no longer referenced, it will be freed.
 *
 * Since: 3.8
 */
void
cdk_frame_timings_unref (CdkFrameTimings *timings)
{
  g_return_if_fail (timings != NULL);
  g_return_if_fail (timings->ref_count > 0);

  timings->ref_count--;
  if (timings->ref_count == 0)
    {
      g_slice_free (CdkFrameTimings, timings);
    }
}

/**
 * cdk_frame_timings_get_frame_counter:
 * @timings: a #CdkFrameTimings
 *
 * Gets the frame counter value of the #CdkFrameClock when this
 * this frame was drawn.
 *
 * Returns: the frame counter value for this frame
 * Since: 3.8
 */
gint64
cdk_frame_timings_get_frame_counter (CdkFrameTimings *timings)
{
  return timings->frame_counter;
}

/**
 * cdk_frame_timings_get_complete:
 * @timings: a #CdkFrameTimings
 *
 * The timing information in a #CdkFrameTimings is filled in
 * incrementally as the frame as drawn and passed off to the
 * window system for processing and display to the user. The
 * accessor functions for #CdkFrameTimings can return 0 to
 * indicate an unavailable value for two reasons: either because
 * the information is not yet available, or because it isn't
 * available at all. Once cdk_frame_timings_get_complete() returns
 * %TRUE for a frame, you can be certain that no further values
 * will become available and be stored in the #CdkFrameTimings.
 *
 * Returns: %TRUE if all information that will be available
 *  for the frame has been filled in.
 * Since: 3.8
 */
gboolean
cdk_frame_timings_get_complete (CdkFrameTimings *timings)
{
  g_return_val_if_fail (timings != NULL, FALSE);

  return timings->complete;
}

/**
 * cdk_frame_timings_get_frame_time:
 * @timings: A #CdkFrameTimings
 *
 * Returns the frame time for the frame. This is the time value
 * that is typically used to time animations for the frame. See
 * cdk_frame_clock_get_frame_time().
 *
 * Returns: the frame time for the frame, in the timescale
 *  of g_get_monotonic_time()
 */
gint64
cdk_frame_timings_get_frame_time (CdkFrameTimings *timings)
{
  g_return_val_if_fail (timings != NULL, 0);

  return timings->frame_time;
}

/**
 * cdk_frame_timings_get_presentation_time:
 * @timings: a #CdkFrameTimings
 *
 * Reurns the presentation time. This is the time at which the frame
 * became visible to the user.
 *
 * Returns: the time the frame was displayed to the user, in the
 *  timescale of g_get_monotonic_time(), or 0 if no presentation
 *  time is available. See cdk_frame_timings_get_complete()
 * Since: 3.8
 */
gint64
cdk_frame_timings_get_presentation_time (CdkFrameTimings *timings)
{
  g_return_val_if_fail (timings != NULL, 0);

  return timings->presentation_time;
}

/**
 * cdk_frame_timings_get_predicted_presentation_time:
 * @timings: a #CdkFrameTimings
 *
 * Gets the predicted time at which this frame will be displayed. Although
 * no predicted time may be available, if one is available, it will
 * be available while the frame is being generated, in contrast to
 * cdk_frame_timings_get_presentation_time(), which is only available
 * after the frame has been presented. In general, if you are simply
 * animating, you should use cdk_frame_clock_get_frame_time() rather
 * than this function, but this function is useful for applications
 * that want exact control over latency. For example, a movie player
 * may want this information for Audio/Video synchronization.
 *
 * Returns: The predicted time at which the frame will be presented,
 *  in the timescale of g_get_monotonic_time(), or 0 if no predicted
 *  presentation time is available.
 * Since: 3.8
 */
gint64
cdk_frame_timings_get_predicted_presentation_time (CdkFrameTimings *timings)
{
  g_return_val_if_fail (timings != NULL, 0);

  return timings->predicted_presentation_time;
}

/**
 * cdk_frame_timings_get_refresh_interval:
 * @timings: a #CdkFrameTimings
 *
 * Gets the natural interval between presentation times for
 * the display that this frame was displayed on. Frame presentation
 * usually happens during the “vertical blanking interval”.
 *
 * Returns: the refresh interval of the display, in microseconds,
 *  or 0 if the refresh interval is not available.
 *  See cdk_frame_timings_get_complete().
 * Since: 3.8
 */
gint64
cdk_frame_timings_get_refresh_interval (CdkFrameTimings *timings)
{
  g_return_val_if_fail (timings != NULL, 0);

  return timings->refresh_interval;
}
