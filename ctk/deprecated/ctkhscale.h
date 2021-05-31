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

#ifndef __CTK_HSCALE_H__
#define __CTK_HSCALE_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkscale.h>

G_BEGIN_DECLS

#define CTK_TYPE_HSCALE            (ctk_hscale_get_type ())
#define CTK_HSCALE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_HSCALE, CtkHScale))
#define CTK_HSCALE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_HSCALE, CtkHScaleClass))
#define CTK_IS_HSCALE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_HSCALE))
#define CTK_IS_HSCALE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_HSCALE))
#define CTK_HSCALE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_HSCALE, CtkHScaleClass))


typedef struct _CtkHScale       CtkHScale;
typedef struct _CtkHScaleClass  CtkHScaleClass;

struct _CtkHScale
{
  CtkScale scale;
};

struct _CtkHScaleClass
{
  CtkScaleClass parent_class;
};


GDK_DEPRECATED_IN_3_2
GType      ctk_hscale_get_type       (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_3_2_FOR(ctk_scale_new)
CtkWidget* ctk_hscale_new            (CtkAdjustment *adjustment);
GDK_DEPRECATED_IN_3_2_FOR(ctk_scale_new_with_range)
CtkWidget* ctk_hscale_new_with_range (gdouble        min,
                                      gdouble        max,
                                      gdouble        step);

G_END_DECLS

#endif /* __CTK_HSCALE_H__ */
