/* GDK - The GIMP Drawing Kit
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

#ifndef __GDK_SEAT_DEFAULT_PRIVATE_H__
#define __GDK_SEAT_DEFAULT_PRIVATE_H__

#include "cdkseat.h"
#include "cdkseatprivate.h"

#define GDK_TYPE_SEAT_DEFAULT         (cdk_seat_default_get_type ())
#define GDK_SEAT_DEFAULT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDK_TYPE_SEAT_DEFAULT, CdkSeatDefault))
#define GDK_IS_SEAT_DEFAULT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDK_TYPE_SEAT_DEFAULT))
#define GDK_SEAT_DEFAULT_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c), GDK_TYPE_SEAT_DEFAULT, CdkSeatDefaultClass))
#define GDK_IS_SEAT_DEFAULT_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c), GDK_TYPE_SEAT_DEFAULT))
#define GDK_SEAT_DEFAULT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDK_TYPE_SEAT_DEFAULT, CdkSeatDefaultClass))

typedef struct _CdkSeatDefault CdkSeatDefault;
typedef struct _CdkSeatDefaultClass CdkSeatDefaultClass;

struct _CdkSeatDefault
{
  CdkSeat parent_instance;
};

struct _CdkSeatDefaultClass
{
  CdkSeatClass parent_class;
};

GType     cdk_seat_default_get_type     (void) G_GNUC_CONST;

CdkSeat * cdk_seat_default_new_for_master_pair (CdkDevice *pointer,
                                                CdkDevice *keyboard);

void      cdk_seat_default_add_slave    (CdkSeatDefault *seat,
                                         CdkDevice      *device);
void      cdk_seat_default_remove_slave (CdkSeatDefault *seat,
                                         CdkDevice      *device);
void      cdk_seat_default_add_tool     (CdkSeatDefault *seat,
                                         CdkDeviceTool  *tool);
void      cdk_seat_default_remove_tool  (CdkSeatDefault *seat,
                                         CdkDeviceTool  *tool);

#endif /* __GDK_SEAT_DEFAULT_PRIVATE_H__ */
