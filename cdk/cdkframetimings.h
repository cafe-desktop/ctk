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

#ifndef __CDK_FRAME_TIMINGS_H__
#define __CDK_FRAME_TIMINGS_H__

#if !defined (__CDK_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdk.h> can be included directly."
#endif

#include <glib-object.h>
#include <cdk/cdkversionmacros.h>

G_BEGIN_DECLS

typedef struct _CdkFrameTimings CdkFrameTimings;

CDK_AVAILABLE_IN_3_8
GType            cdk_frame_timings_get_type (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_3_8
CdkFrameTimings *cdk_frame_timings_ref   (CdkFrameTimings *timings);
CDK_AVAILABLE_IN_3_8
void             cdk_frame_timings_unref (CdkFrameTimings *timings);

CDK_AVAILABLE_IN_3_8
gint64           cdk_frame_timings_get_frame_counter     (CdkFrameTimings *timings);
CDK_AVAILABLE_IN_3_8
gboolean         cdk_frame_timings_get_complete          (CdkFrameTimings *timings);
CDK_AVAILABLE_IN_3_8
gint64           cdk_frame_timings_get_frame_time        (CdkFrameTimings *timings);
CDK_AVAILABLE_IN_3_8
gint64           cdk_frame_timings_get_presentation_time (CdkFrameTimings *timings);
CDK_AVAILABLE_IN_3_8
gint64           cdk_frame_timings_get_refresh_interval  (CdkFrameTimings *timings);

CDK_AVAILABLE_IN_3_8
gint64           cdk_frame_timings_get_predicted_presentation_time (CdkFrameTimings *timings);

G_END_DECLS

#endif /* __CDK_FRAME_TIMINGS_H__ */
