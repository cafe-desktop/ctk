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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CDK_VISUAL_H__
#define __CDK_VISUAL_H__

#if !defined (__CDK_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdk.h> can be included directly."
#endif

#include <cdk/cdktypes.h>
#include <cdk/cdkversionmacros.h>

G_BEGIN_DECLS

#define CDK_TYPE_VISUAL              (cdk_visual_get_type ())
#define CDK_VISUAL(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), CDK_TYPE_VISUAL, CdkVisual))
#define CDK_IS_VISUAL(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), CDK_TYPE_VISUAL))

/**
 * CdkVisualType:
 * @CDK_VISUAL_STATIC_GRAY: Each pixel value indexes a grayscale value
 *     directly.
 * @CDK_VISUAL_GRAYSCALE: Each pixel is an index into a color map that
 *     maps pixel values into grayscale values. The color map can be
 *     changed by an application.
 * @CDK_VISUAL_STATIC_COLOR: Each pixel value is an index into a predefined,
 *     unmodifiable color map that maps pixel values into RGB values.
 * @CDK_VISUAL_PSEUDO_COLOR: Each pixel is an index into a color map that
 *     maps pixel values into rgb values. The color map can be changed by
 *     an application.
 * @CDK_VISUAL_TRUE_COLOR: Each pixel value directly contains red, green,
 *     and blue components. Use cdk_visual_get_red_pixel_details(), etc,
 *     to obtain information about how the components are assembled into
 *     a pixel value.
 * @CDK_VISUAL_DIRECT_COLOR: Each pixel value contains red, green, and blue
 *     components as for %CDK_VISUAL_TRUE_COLOR, but the components are
 *     mapped via a color table into the final output table instead of
 *     being converted directly.
 *
 * A set of values that describe the manner in which the pixel values
 * for a visual are converted into RGB values for display.
 */
typedef enum
{
  CDK_VISUAL_STATIC_GRAY,
  CDK_VISUAL_GRAYSCALE,
  CDK_VISUAL_STATIC_COLOR,
  CDK_VISUAL_PSEUDO_COLOR,
  CDK_VISUAL_TRUE_COLOR,
  CDK_VISUAL_DIRECT_COLOR
} CdkVisualType;

/**
 * CdkVisual:
 *
 * A #CdkVisual contains information about
 * a particular visual.
 */

CDK_AVAILABLE_IN_ALL
GType         cdk_visual_get_type            (void) G_GNUC_CONST;

CDK_DEPRECATED_IN_3_22
gint          cdk_visual_get_best_depth      (void);
CDK_DEPRECATED_IN_3_22
CdkVisualType cdk_visual_get_best_type       (void);
CDK_DEPRECATED_IN_3_22_FOR(cdk_screen_get_system_visual)
CdkVisual*    cdk_visual_get_system          (void);
CDK_DEPRECATED_IN_3_22
CdkVisual*    cdk_visual_get_best            (void);
CDK_DEPRECATED_IN_3_22
CdkVisual*    cdk_visual_get_best_with_depth (gint           depth);
CDK_DEPRECATED_IN_3_22
CdkVisual*    cdk_visual_get_best_with_type  (CdkVisualType  visual_type);
CDK_DEPRECATED_IN_3_22
CdkVisual*    cdk_visual_get_best_with_both  (gint           depth,
                                              CdkVisualType  visual_type);

CDK_DEPRECATED_IN_3_22
void cdk_query_depths       (gint           **depths,
                             gint            *count);
CDK_DEPRECATED_IN_3_22
void cdk_query_visual_types (CdkVisualType  **visual_types,
                             gint            *count);

CDK_DEPRECATED_IN_3_22_FOR(cdk_screen_list_visuals)
GList* cdk_list_visuals (void);

CDK_AVAILABLE_IN_ALL
CdkScreen    *cdk_visual_get_screen (CdkVisual *visual);

CDK_AVAILABLE_IN_ALL
CdkVisualType cdk_visual_get_visual_type         (CdkVisual *visual);
CDK_AVAILABLE_IN_ALL
gint          cdk_visual_get_depth               (CdkVisual *visual);
CDK_DEPRECATED_IN_3_22
CdkByteOrder  cdk_visual_get_byte_order          (CdkVisual *visual);
CDK_DEPRECATED_IN_3_22
gint          cdk_visual_get_colormap_size       (CdkVisual *visual);
CDK_DEPRECATED_IN_3_22
gint          cdk_visual_get_bits_per_rgb        (CdkVisual *visual);
CDK_AVAILABLE_IN_ALL
void          cdk_visual_get_red_pixel_details   (CdkVisual *visual,
                                                  guint32   *mask,
                                                  gint      *shift,
                                                  gint      *precision);
CDK_AVAILABLE_IN_ALL
void          cdk_visual_get_green_pixel_details (CdkVisual *visual,
                                                  guint32   *mask,
                                                  gint      *shift,
                                                  gint      *precision);
CDK_AVAILABLE_IN_ALL
void          cdk_visual_get_blue_pixel_details  (CdkVisual *visual,
                                                  guint32   *mask,
                                                  gint      *shift,
                                                  gint      *precision);

G_END_DECLS

#endif /* __CDK_VISUAL_H__ */
