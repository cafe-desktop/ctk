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

#ifndef __CDK_COLOR_H__
#define __CDK_COLOR_H__

#if !defined (__CDK_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdk.h> can be included directly."
#endif

#include <cairo.h>
#include <cdk/cdktypes.h>
#include <cdk/cdkversionmacros.h>

G_BEGIN_DECLS


/**
 * CdkColor:
 * @pixel: For allocated colors, the pixel value used to
 *     draw this color on the screen. Not used anymore.
 * @red: The red component of the color. This is
 *     a value between 0 and 65535, with 65535 indicating
 *     full intensity
 * @green: The green component of the color
 * @blue: The blue component of the color
 *
 * A #CdkColor is used to describe a color,
 * similar to the XColor struct used in the X11 drawing API.
 */
struct _CdkColor
{
  guint32 pixel;
  guint16 red;
  guint16 green;
  guint16 blue;
};

#define CDK_TYPE_COLOR (cdk_color_get_type ())

CDK_AVAILABLE_IN_ALL
GType     cdk_color_get_type (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_ALL
CdkColor *cdk_color_copy      (const CdkColor *color);
CDK_AVAILABLE_IN_ALL
void      cdk_color_free      (CdkColor       *color);

CDK_AVAILABLE_IN_ALL
guint     cdk_color_hash      (const CdkColor *color);
CDK_AVAILABLE_IN_ALL
gboolean  cdk_color_equal     (const CdkColor *colora,
                               const CdkColor *colorb);

CDK_AVAILABLE_IN_ALL
gboolean  cdk_color_parse     (const gchar    *spec,
                               CdkColor       *color);
CDK_AVAILABLE_IN_ALL
gchar *   cdk_color_to_string (const CdkColor *color);


G_END_DECLS

#endif /* __CDK_COLOR_H__ */
