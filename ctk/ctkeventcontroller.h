/* GTK - The GIMP Toolkit
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

typedef struct _GtkEventController GtkEventController;
typedef struct _GtkEventControllerClass GtkEventControllerClass;

#include <gdk/gdk.h>
#include <ctk/ctktypes.h>
#include <ctk/ctkenums.h>

G_BEGIN_DECLS

#define CTK_TYPE_EVENT_CONTROLLER         (ctk_event_controller_get_type ())
#define CTK_EVENT_CONTROLLER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_EVENT_CONTROLLER, GtkEventController))
#define CTK_EVENT_CONTROLLER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), CTK_TYPE_EVENT_CONTROLLER, GtkEventControllerClass))
#define CTK_IS_EVENT_CONTROLLER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_EVENT_CONTROLLER))
#define CTK_IS_EVENT_CONTROLLER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), CTK_TYPE_EVENT_CONTROLLER))
#define CTK_EVENT_CONTROLLER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CTK_TYPE_EVENT_CONTROLLER, GtkEventControllerClass))


GDK_AVAILABLE_IN_3_14
GType        ctk_event_controller_get_type       (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_3_14
GtkWidget  * ctk_event_controller_get_widget     (GtkEventController *controller);

GDK_AVAILABLE_IN_3_14
gboolean     ctk_event_controller_handle_event   (GtkEventController *controller,
                                                  const GdkEvent     *event);
GDK_AVAILABLE_IN_3_14
void         ctk_event_controller_reset          (GtkEventController *controller);

GDK_AVAILABLE_IN_3_14
GtkPropagationPhase ctk_event_controller_get_propagation_phase (GtkEventController *controller);

GDK_AVAILABLE_IN_3_14
void                ctk_event_controller_set_propagation_phase (GtkEventController  *controller,
                                                                GtkPropagationPhase  phase);

G_END_DECLS

#endif /* __CTK_EVENT_CONTROLLER_H__ */
