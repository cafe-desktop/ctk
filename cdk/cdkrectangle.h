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

#ifndef __CDK_RECTANGLE_H__
#define __CDK_RECTANGLE_H__

#if !defined (__CDK_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdk.h> can be included directly."
#endif

#include <cdk/cdktypes.h>
#include <cdk/cdkversionmacros.h>

G_BEGIN_DECLS

/* Rectangle utilities
 */
CDK_AVAILABLE_IN_ALL
gboolean cdk_rectangle_intersect (const CdkRectangle *src1,
                                  const CdkRectangle *src2,
                                  CdkRectangle       *dest);
CDK_AVAILABLE_IN_ALL
void     cdk_rectangle_union     (const CdkRectangle *src1,
                                  const CdkRectangle *src2,
                                  CdkRectangle       *dest);

CDK_AVAILABLE_IN_3_20
gboolean cdk_rectangle_equal     (const CdkRectangle *rect1,
                                  const CdkRectangle *rect2);

CDK_AVAILABLE_IN_ALL
GType cdk_rectangle_get_type (void) G_GNUC_CONST;

#define CDK_TYPE_RECTANGLE (cdk_rectangle_get_type ())

G_END_DECLS

#endif /* __CDK__RECTANGLE_H__ */
