/* GTK - The GIMP Toolkit
 * Copyright (C) 2014 Red Hat, Inc
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

#ifndef __CTK_ADJUSTMENT_PRIVATE_H__
#define __CTK_ADJUSTMENT_PRIVATE_H__


#include <ctk/ctkadjustment.h>


G_BEGIN_DECLS

void ctk_adjustment_enable_animation (CtkAdjustment *adjustment,
                                      GdkFrameClock *clock,
                                      guint          duration);
guint ctk_adjustment_get_animation_duration (CtkAdjustment *adjustment);
void ctk_adjustment_animate_to_value (CtkAdjustment *adjustment,
                                      gdouble        value);
gdouble ctk_adjustment_get_target_value (CtkAdjustment *adjustment);

gboolean ctk_adjustment_is_animating (CtkAdjustment *adjustment);

G_END_DECLS


#endif /* __CTK_ADJUSTMENT_PRIVATE_H__ */
