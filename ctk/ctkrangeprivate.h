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

#ifndef __CTK_RANGE_PRIVATE_H__
#define __CTK_RANGE_PRIVATE_H__


#include <ctk/ctkrange.h>
#include <ctk/ctkcssgadgetprivate.h>
#include <ctk/ctkcssnodeprivate.h>


G_BEGIN_DECLS

gdouble            _ctk_range_get_wheel_delta              (CtkRange       *range,
                                                            GdkEventScroll *event);
void               _ctk_range_set_has_origin               (CtkRange      *range,
                                                            gboolean       has_origin);
gboolean           _ctk_range_get_has_origin               (CtkRange      *range);
void               _ctk_range_set_stop_values              (CtkRange      *range,
                                                            gdouble       *values,
                                                            gint           n_values);
gint               _ctk_range_get_stop_positions           (CtkRange      *range,
                                                            gint         **values);
void               _ctk_range_set_steppers                 (CtkRange      *range,
                                                            gboolean       has_a,
                                                            gboolean       has_b,
                                                            gboolean       has_c,
                                                            gboolean       has_d);

void               ctk_range_set_slider_use_min_size       (CtkRange      *range,
                                                            gboolean       use_min_size);
CtkCssGadget      *ctk_range_get_slider_gadget             (CtkRange *range);
CtkCssGadget      *ctk_range_get_gadget                    (CtkRange *range);

G_END_DECLS


#endif /* __CTK_RANGE_PRIVATE_H__ */
