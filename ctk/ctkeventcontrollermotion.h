/* CTK - The GIMP Toolkit
 * Copyright (C) 2017, Red Hat, Inc.
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
 * Author(s): Matthias Clasen <mclasen@redhat.com>
 */

#ifndef __CTK_EVENT_CONTROLLER_MOTION_H__
#define __CTK_EVENT_CONTROLLER_MOTION_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <cdk/cdk.h>
#include <ctk/ctkeventcontroller.h>

G_BEGIN_DECLS

#define CTK_TYPE_EVENT_CONTROLLER_MOTION         (ctk_event_controller_motion_get_type ())
#define CTK_EVENT_CONTROLLER_MOTION(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_EVENT_CONTROLLER_MOTION, CtkEventControllerMotion))
#define CTK_EVENT_CONTROLLER_MOTION_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), CTK_TYPE_EVENT_CONTROLLER_MOTION, CtkEventControllerMotionClass))
#define CTK_IS_EVENT_CONTROLLER_MOTION(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_EVENT_CONTROLLER_MOTION))
#define CTK_IS_EVENT_CONTROLLER_MOTION_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), CTK_TYPE_EVENT_CONTROLLER_MOTION))
#define CTK_EVENT_CONTROLLER_MOTION_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CTK_TYPE_EVENT_CONTROLLER_MOTION, CtkEventControllerMotionClass))

typedef struct _CtkEventControllerMotion CtkEventControllerMotion;
typedef struct _CtkEventControllerMotionClass CtkEventControllerMotionClass;

CDK_AVAILABLE_IN_3_24
GType               ctk_event_controller_motion_get_type (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_3_24
CtkEventController *ctk_event_controller_motion_new      (CtkWidget *widget);

G_END_DECLS

#endif /* __CTK_EVENT_CONTROLLER_MOTION_H__ */
