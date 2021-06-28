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

#ifndef __GDK_SEAT_H__
#define __GDK_SEAT_H__

#if !defined (__GDK_H_INSIDE__) && !defined (GDK_COMPILATION)
#error "Only <cdk/cdk.h> can be included directly."
#endif

#include <glib-object.h>
#include <cdk/cdkwindow.h>
#include <cdk/cdkevents.h>
#include <cdk/cdktypes.h>

G_BEGIN_DECLS

#define GDK_TYPE_SEAT  (cdk_seat_get_type ())
#define GDK_SEAT(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GDK_TYPE_SEAT, CdkSeat))
#define GDK_IS_SEAT(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDK_TYPE_SEAT))

/**
 * CdkSeatCapabilities:
 * @GDK_SEAT_CAPABILITY_NONE: No input capabilities
 * @GDK_SEAT_CAPABILITY_POINTER: The seat has a pointer (e.g. mouse)
 * @GDK_SEAT_CAPABILITY_TOUCH: The seat has touchscreen(s) attached
 * @GDK_SEAT_CAPABILITY_TABLET_STYLUS: The seat has drawing tablet(s) attached
 * @GDK_SEAT_CAPABILITY_KEYBOARD: The seat has keyboard(s) attached
 * @GDK_SEAT_CAPABILITY_ALL_POINTING: The union of all pointing capabilities
 * @GDK_SEAT_CAPABILITY_ALL: The union of all capabilities
 *
 * Flags describing the seat capabilities.
 *
 * Since: 3.20
 */
typedef enum {
  GDK_SEAT_CAPABILITY_NONE          = 0,
  GDK_SEAT_CAPABILITY_POINTER       = 1 << 0,
  GDK_SEAT_CAPABILITY_TOUCH         = 1 << 1,
  GDK_SEAT_CAPABILITY_TABLET_STYLUS = 1 << 2,
  GDK_SEAT_CAPABILITY_KEYBOARD      = 1 << 3,
  GDK_SEAT_CAPABILITY_ALL_POINTING  = (GDK_SEAT_CAPABILITY_POINTER | GDK_SEAT_CAPABILITY_TOUCH | GDK_SEAT_CAPABILITY_TABLET_STYLUS),
  GDK_SEAT_CAPABILITY_ALL           = (GDK_SEAT_CAPABILITY_ALL_POINTING | GDK_SEAT_CAPABILITY_KEYBOARD)
} CdkSeatCapabilities;

/**
 * CdkSeatGrabPrepareFunc:
 * @seat: the #CdkSeat being grabbed
 * @window: the #CdkWindow being grabbed
 * @user_data: user data passed in cdk_seat_grab()
 *
 * Type of the callback used to set up @window so it can be
 * grabbed. A typical action would be ensuring the window is
 * visible, although there's room for other initialization
 * actions.
 *
 * Since: 3.20
 */
typedef void (* CdkSeatGrabPrepareFunc) (CdkSeat   *seat,
                                         CdkWindow *window,
                                         gpointer   user_data);

struct _CdkSeat
{
  GObject parent_instance;
};

GDK_AVAILABLE_IN_3_20
GType          cdk_seat_get_type         (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_3_20
CdkGrabStatus  cdk_seat_grab             (CdkSeat                *seat,
                                          CdkWindow              *window,
                                          CdkSeatCapabilities     capabilities,
                                          gboolean                owner_events,
                                          CdkCursor              *cursor,
                                          const CdkEvent         *event,
                                          CdkSeatGrabPrepareFunc  prepare_func,
                                          gpointer                prepare_func_data);
GDK_AVAILABLE_IN_3_20
void           cdk_seat_ungrab           (CdkSeat                *seat);

GDK_AVAILABLE_IN_3_20
CdkDisplay *   cdk_seat_get_display      (CdkSeat             *seat);

GDK_AVAILABLE_IN_3_20
CdkSeatCapabilities
               cdk_seat_get_capabilities (CdkSeat             *seat);

GDK_AVAILABLE_IN_3_20
GList *        cdk_seat_get_slaves       (CdkSeat             *seat,
                                          CdkSeatCapabilities  capabilities);

GDK_AVAILABLE_IN_3_20
CdkDevice *    cdk_seat_get_pointer      (CdkSeat             *seat);
GDK_AVAILABLE_IN_3_20
CdkDevice *    cdk_seat_get_keyboard     (CdkSeat             *seat);

G_END_DECLS

#endif /* __GDK_SEAT_H__ */
