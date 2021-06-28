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

#ifndef __CDK_SEAT_H__
#define __CDK_SEAT_H__

#if !defined (__CDK_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdk.h> can be included directly."
#endif

#include <glib-object.h>
#include <cdk/cdkwindow.h>
#include <cdk/cdkevents.h>
#include <cdk/cdktypes.h>

G_BEGIN_DECLS

#define CDK_TYPE_SEAT  (cdk_seat_get_type ())
#define CDK_SEAT(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), CDK_TYPE_SEAT, CdkSeat))
#define CDK_IS_SEAT(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), CDK_TYPE_SEAT))

/**
 * CdkSeatCapabilities:
 * @CDK_SEAT_CAPABILITY_NONE: No input capabilities
 * @CDK_SEAT_CAPABILITY_POINTER: The seat has a pointer (e.g. mouse)
 * @CDK_SEAT_CAPABILITY_TOUCH: The seat has touchscreen(s) attached
 * @CDK_SEAT_CAPABILITY_TABLET_STYLUS: The seat has drawing tablet(s) attached
 * @CDK_SEAT_CAPABILITY_KEYBOARD: The seat has keyboard(s) attached
 * @CDK_SEAT_CAPABILITY_ALL_POINTING: The union of all pointing capabilities
 * @CDK_SEAT_CAPABILITY_ALL: The union of all capabilities
 *
 * Flags describing the seat capabilities.
 *
 * Since: 3.20
 */
typedef enum {
  CDK_SEAT_CAPABILITY_NONE          = 0,
  CDK_SEAT_CAPABILITY_POINTER       = 1 << 0,
  CDK_SEAT_CAPABILITY_TOUCH         = 1 << 1,
  CDK_SEAT_CAPABILITY_TABLET_STYLUS = 1 << 2,
  CDK_SEAT_CAPABILITY_KEYBOARD      = 1 << 3,
  CDK_SEAT_CAPABILITY_ALL_POINTING  = (CDK_SEAT_CAPABILITY_POINTER | CDK_SEAT_CAPABILITY_TOUCH | CDK_SEAT_CAPABILITY_TABLET_STYLUS),
  CDK_SEAT_CAPABILITY_ALL           = (CDK_SEAT_CAPABILITY_ALL_POINTING | CDK_SEAT_CAPABILITY_KEYBOARD)
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

CDK_AVAILABLE_IN_3_20
GType          cdk_seat_get_type         (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_3_20
CdkGrabStatus  cdk_seat_grab             (CdkSeat                *seat,
                                          CdkWindow              *window,
                                          CdkSeatCapabilities     capabilities,
                                          gboolean                owner_events,
                                          CdkCursor              *cursor,
                                          const CdkEvent         *event,
                                          CdkSeatGrabPrepareFunc  prepare_func,
                                          gpointer                prepare_func_data);
CDK_AVAILABLE_IN_3_20
void           cdk_seat_ungrab           (CdkSeat                *seat);

CDK_AVAILABLE_IN_3_20
CdkDisplay *   cdk_seat_get_display      (CdkSeat             *seat);

CDK_AVAILABLE_IN_3_20
CdkSeatCapabilities
               cdk_seat_get_capabilities (CdkSeat             *seat);

CDK_AVAILABLE_IN_3_20
GList *        cdk_seat_get_slaves       (CdkSeat             *seat,
                                          CdkSeatCapabilities  capabilities);

CDK_AVAILABLE_IN_3_20
CdkDevice *    cdk_seat_get_pointer      (CdkSeat             *seat);
CDK_AVAILABLE_IN_3_20
CdkDevice *    cdk_seat_get_keyboard     (CdkSeat             *seat);

G_END_DECLS

#endif /* __CDK_SEAT_H__ */
