/* CTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

#ifndef __CTK_BORDER_H__
#define __CTK_BORDER_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <glib-object.h>
#include <cdk/cdk.h>

G_BEGIN_DECLS

typedef struct _CtkBorder CtkBorder;

#define CTK_TYPE_BORDER (ctk_border_get_type ())

/**
 * CtkBorder:
 * @left: The width of the left border
 * @right: The width of the right border
 * @top: The width of the top border
 * @bottom: The width of the bottom border
 *
 * A struct that specifies a border around a rectangular area
 * that can be of different width on each side.
 */
struct _CtkBorder
{
  gint16 left;
  gint16 right;
  gint16 top;
  gint16 bottom;
};

CDK_AVAILABLE_IN_ALL
GType      ctk_border_get_type (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkBorder *ctk_border_new      (void) G_GNUC_MALLOC;
CDK_AVAILABLE_IN_ALL
CtkBorder *ctk_border_copy     (const CtkBorder *border_);
CDK_AVAILABLE_IN_ALL
void       ctk_border_free     (CtkBorder       *border_);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkBorder, ctk_border_free)

G_END_DECLS

#endif /* __CTK_BORDER_H__ */
