/* GTK - The GIMP Toolkit
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
 * Author(s): Carlos Garnacho <carlosg@gnome.org>
 */

#ifndef __CTK_EVENT_CONTROLLER_KEY_H__
#define __CTK_EVENT_CONTROLLER_KEY_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gdk/gdk.h>
#include <gtk/gtkeventcontroller.h>
#include <gtk/gtkimcontext.h>

G_BEGIN_DECLS

#define CTK_TYPE_EVENT_CONTROLLER_KEY         (ctk_event_controller_key_get_type ())
#define CTK_EVENT_CONTROLLER_KEY(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_EVENT_CONTROLLER_KEY, GtkEventControllerKey))
#define CTK_EVENT_CONTROLLER_KEY_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), CTK_TYPE_EVENT_CONTROLLER_KEY, GtkEventControllerKeyClass))
#define CTK_IS_EVENT_CONTROLLER_KEY(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_EVENT_CONTROLLER_KEY))
#define CTK_IS_EVENT_CONTROLLER_KEY_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), CTK_TYPE_EVENT_CONTROLLER_KEY))
#define CTK_EVENT_CONTROLLER_KEY_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CTK_TYPE_EVENT_CONTROLLER_KEY, GtkEventControllerKeyClass))

typedef struct _GtkEventControllerKey GtkEventControllerKey;
typedef struct _GtkEventControllerKeyClass GtkEventControllerKeyClass;

GDK_AVAILABLE_IN_3_24
GType               ctk_event_controller_key_get_type  (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_3_24
GtkEventController *ctk_event_controller_key_new (GtkWidget *widget);

GDK_AVAILABLE_IN_3_24
void                ctk_event_controller_key_set_im_context (GtkEventControllerKey *controller,
                                                             GtkIMContext          *im_context);
GDK_AVAILABLE_IN_3_24
GtkIMContext *      ctk_event_controller_key_get_im_context (GtkEventControllerKey *controller);

GDK_AVAILABLE_IN_3_24
gboolean            ctk_event_controller_key_forward        (GtkEventControllerKey *controller,
                                                             GtkWidget             *widget);
GDK_AVAILABLE_IN_3_24
guint               ctk_event_controller_key_get_group      (GtkEventControllerKey *controller);

G_END_DECLS

#endif /* __CTK_EVENT_CONTROLLER_KEY_H__ */
