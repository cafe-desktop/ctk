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

#ifndef __GDK_SEAT_PRIVATE_H__
#define __GDK_SEAT_PRIVATE_H__

typedef struct _CdkSeatClass CdkSeatClass;

#include "cdkseat.h"

#define GDK_SEAT_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c), GDK_TYPE_SEAT, CdkSeatClass))
#define GDK_IS_SEAT_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c), GDK_TYPE_SEAT))
#define GDK_SEAT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDK_TYPE_SEAT, CdkSeatClass))

struct _CdkSeatClass
{
  GObjectClass parent_class;

  void (* device_added)   (CdkSeat   *seat,
                           CdkDevice *device);
  void (* device_removed) (CdkSeat   *seat,
                           CdkDevice *device);
  void (* device_changed) (CdkSeat   *seat,
                           CdkDevice *device);

  CdkSeatCapabilities (*get_capabilities) (CdkSeat *seat);

  CdkGrabStatus (* grab)   (CdkSeat                *seat,
                            CdkWindow              *window,
                            CdkSeatCapabilities     capabilities,
                            gboolean                owner_events,
                            CdkCursor              *cursor,
                            const CdkEvent         *event,
                            CdkSeatGrabPrepareFunc  prepare_func,
                            gpointer                prepare_func_data);
  void          (* ungrab) (CdkSeat                *seat);

  CdkDevice * (* get_master) (CdkSeat             *seat,
                              CdkSeatCapabilities  capability);
  GList *     (* get_slaves) (CdkSeat             *seat,
                              CdkSeatCapabilities  capabilities);

  CdkDeviceTool * (* get_tool) (CdkSeat *seat,
                                guint64  serial,
                                guint64  tool_id);
};

void cdk_seat_device_added   (CdkSeat   *seat,
                              CdkDevice *device);
void cdk_seat_device_removed (CdkSeat   *seat,
                              CdkDevice *device);

void cdk_seat_tool_added     (CdkSeat       *seat,
                              CdkDeviceTool *tool);
void cdk_seat_tool_removed   (CdkSeat       *seat,
                              CdkDeviceTool *tool);

CdkDeviceTool *
     cdk_seat_get_tool       (CdkSeat   *seat,
                              guint64    serial,
                              guint64    hw_id);

#endif /* __GDK_SEAT_PRIVATE_H__ */
