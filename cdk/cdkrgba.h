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

#ifndef __CDK_RGBA_H__
#define __CDK_RGBA_H__

#if !defined (__CDK_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdk.h> can be included directly."
#endif

#include <cdk/cdktypes.h>
#include <cdk/cdkversionmacros.h>

G_BEGIN_DECLS

struct _CdkRGBA
{
  gdouble red;
  gdouble green;
  gdouble blue;
  gdouble alpha;
};

#define CDK_TYPE_RGBA (cdk_rgba_get_type ())

CDK_AVAILABLE_IN_ALL
GType     cdk_rgba_get_type  (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_ALL
CdkRGBA * cdk_rgba_copy      (const CdkRGBA *rgba);
CDK_AVAILABLE_IN_ALL
void      cdk_rgba_free      (CdkRGBA       *rgba);

CDK_AVAILABLE_IN_ALL
guint     cdk_rgba_hash      (gconstpointer  p);
CDK_AVAILABLE_IN_ALL
gboolean  cdk_rgba_equal     (gconstpointer  p1,
                              gconstpointer  p2);

CDK_AVAILABLE_IN_ALL
gboolean  cdk_rgba_parse     (CdkRGBA       *rgba,
                              const gchar   *spec);
CDK_AVAILABLE_IN_ALL
gchar *   cdk_rgba_to_string (const CdkRGBA *rgba);


G_END_DECLS

#endif /* __CDK_RGBA_H__ */
