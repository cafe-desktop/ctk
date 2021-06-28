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

#ifndef __CDK_BROADWAY_CURSOR_H__
#define __CDK_BROADWAY_CURSOR_H__

#if !defined (__CDKBROADWAY_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdkbroadway.h> can be included directly."
#endif

#include <cdk/cdk.h>

G_BEGIN_DECLS

#define CDK_TYPE_BROADWAY_CURSOR              (cdk_broadway_cursor_get_type ())
#define CDK_BROADWAY_CURSOR(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), CDK_TYPE_BROADWAY_CURSOR, CdkBroadwayCursor))
#define CDK_BROADWAY_CURSOR_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CDK_TYPE_BROADWAY_CURSOR, CdkBroadwayCursorClass))
#define CDK_IS_BROADWAY_CURSOR(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), CDK_TYPE_BROADWAY_CURSOR))
#define CDK_IS_BROADWAY_CURSOR_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CDK_TYPE_BROADWAY_CURSOR))
#define CDK_BROADWAY_CURSOR_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CDK_TYPE_BROADWAY_CURSOR, CdkBroadwayCursorClass))

#ifdef CDK_COMPILATION
typedef struct _CdkBroadwayCursor CdkBroadwayCursor;
#else
typedef CdkCursor CdkBroadwayCursor;
#endif
typedef struct _CdkBroadwayCursorClass CdkBroadwayCursorClass;

CDK_AVAILABLE_IN_ALL
GType    cdk_broadway_cursor_get_type          (void);

G_END_DECLS

#endif /* __CDK_BROADWAY_CURSOR_H__ */
