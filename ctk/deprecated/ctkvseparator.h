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

#ifndef __CTK_VSEPARATOR_H__
#define __CTK_VSEPARATOR_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkseparator.h>

G_BEGIN_DECLS

#define CTK_TYPE_VSEPARATOR                  (ctk_vseparator_get_type ())
#define CTK_VSEPARATOR(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_VSEPARATOR, CtkVSeparator))
#define CTK_VSEPARATOR_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_VSEPARATOR, CtkVSeparatorClass))
#define CTK_IS_VSEPARATOR(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_VSEPARATOR))
#define CTK_IS_VSEPARATOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_VSEPARATOR))
#define CTK_VSEPARATOR_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_VSEPARATOR, CtkVSeparatorClass))


typedef struct _CtkVSeparator       CtkVSeparator;
typedef struct _CtkVSeparatorClass  CtkVSeparatorClass;

/**
 * CtkVSeparator:
 *
 * The #CtkVSeparator struct contains private data only, and
 * should be accessed using the functions below.
 */
struct _CtkVSeparator
{
  CtkSeparator separator;
};

struct _CtkVSeparatorClass
{
  CtkSeparatorClass parent_class;
};


CDK_DEPRECATED_IN_3_2
GType      ctk_vseparator_get_type (void) G_GNUC_CONST;
CDK_DEPRECATED_IN_3_2_FOR(ctk_separator_new)
CtkWidget* ctk_vseparator_new      (void);

G_END_DECLS

#endif /* __CTK_VSEPARATOR_H__ */
