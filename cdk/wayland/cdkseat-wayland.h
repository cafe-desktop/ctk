/* CDK - The GIMP Drawing Kit
 * Copyright (C) 2015 Red Hat
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
 *
 * Author: Carlos Garnacho <carlosg@gnome.org>
 */

#ifndef __CDK_WAYLAND_SEAT_H__
#define __CDK_WAYLAND_SEAT_H__

#include "config.h"

#include <cdk/cdkseatprivate.h>

#define CDK_TYPE_WAYLAND_SEAT         (cdk_wayland_seat_get_type ())
#define CDK_WAYLAND_SEAT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CDK_TYPE_WAYLAND_SEAT, CdkWaylandSeat))
#define CDK_WAYLAND_SEAT_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c), CDK_TYPE_WAYLAND_SEAT, CdkWaylandSeatClass))
#define CDK_IS_WAYLAND_SEAT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CDK_TYPE_WAYLAND_SEAT))
#define CDK_IS_WAYLAND_SEAT_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c), CDK_TYPE_WAYLAND_SEAT))
#define CDK_WAYLAND_SEAT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CDK_TYPE_WAYLAND_SEAT, CdkWaylandSeatClass))

typedef struct _CdkWaylandSeat CdkWaylandSeat;
typedef struct _CdkWaylandSeatClass CdkWaylandSeatClass;

struct _CdkWaylandSeatClass
{
  CdkSeatClass parent_class;
};

GType cdk_wayland_seat_get_type (void) G_GNUC_CONST;

#endif /* __CDK_WAYLAND_SEAT_H__ */
