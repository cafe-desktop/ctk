/* CTK - The GIMP Toolkit
 * Copyright (C) 2012, One Laptop Per Child.
 * Copyright (C) 2014, Red Hat, Inc.
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
 * Author(s): Carlos Garnacho <carlosg@gnome.org>
 */
#ifndef __CTK_EVENT_CONTROLLER_H__
#define __CTK_EVENT_CONTROLLER_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

typedef struct _CtkEventController CtkEventController;
typedef struct _CtkEventControllerClass CtkEventControllerClass;

#include <cdk/cdk.h>
#include <ctk/ctktypes.h>
#include <ctk/ctkenums.h>

G_BEGIN_DECLS

#define CTK_TYPE_EVENT_CONTROLLER         (ctk_event_controller_get_type ())
#define CTK_EVENT_CONTROLLER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_EVENT_CONTROLLER, CtkEventController))
#define CTK_EVENT_CONTROLLER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), CTK_TYPE_EVENT_CONTROLLER, CtkEventControllerClass))
#define CTK_IS_EVENT_CONTROLLER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_EVENT_CONTROLLER))
#define CTK_IS_EVENT_CONTROLLER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), CTK_TYPE_EVENT_CONTROLLER))
#define CTK_EVENT_CONTROLLER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CTK_TYPE_EVENT_CONTROLLER, CtkEventControllerClass))


CDK_AVAILABLE_IN_3_14
GType        ctk_event_controller_get_type       (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_3_14
CtkWidget  * ctk_event_controller_get_widget     (CtkEventController *controller);

CDK_AVAILABLE_IN_3_14
gboolean     ctk_event_controller_handle_event   (CtkEventController *controller,
                                                  const CdkEvent     *event);
CDK_AVAILABLE_IN_3_14
void         ctk_event_controller_reset          (CtkEventController *controller);

CDK_AVAILABLE_IN_3_14
CtkPropagationPhase ctk_event_controller_get_propagation_phase (CtkEventController *controller);

CDK_AVAILABLE_IN_3_14
void                ctk_event_controller_set_propagation_phase (CtkEventController  *controller,
                                                                CtkPropagationPhase  phase);

G_END_DECLS

#endif /* __CTK_EVENT_CONTROLLER_H__ */
