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

#ifndef __CTK_EVENT_CONTROLLER_SCROLL_H__
#define __CTK_EVENT_CONTROLLER_SCROLL_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <gdk/gdk.h>
#include <ctk/ctkeventcontroller.h>

G_BEGIN_DECLS

#define CTK_TYPE_EVENT_CONTROLLER_SCROLL         (ctk_event_controller_scroll_get_type ())
#define CTK_EVENT_CONTROLLER_SCROLL(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_EVENT_CONTROLLER_SCROLL, CtkEventControllerScroll))
#define CTK_EVENT_CONTROLLER_SCROLL_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), CTK_TYPE_EVENT_CONTROLLER_SCROLL, CtkEventControllerScrollClass))
#define CTK_IS_EVENT_CONTROLLER_SCROLL(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_EVENT_CONTROLLER_SCROLL))
#define CTK_IS_EVENT_CONTROLLER_SCROLL_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), CTK_TYPE_EVENT_CONTROLLER_SCROLL))
#define CTK_EVENT_CONTROLLER_SCROLL_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CTK_TYPE_EVENT_CONTROLLER_SCROLL, CtkEventControllerScrollClass))

typedef struct _CtkEventControllerScroll CtkEventControllerScroll;
typedef struct _CtkEventControllerScrollClass CtkEventControllerScrollClass;

/**
 * CtkEventControllerScrollFlags:
 * @CTK_EVENT_CONTROLLER_SCROLL_NONE: Don't emit scroll.
 * @CTK_EVENT_CONTROLLER_SCROLL_VERTICAL: Emit scroll with vertical deltas.
 * @CTK_EVENT_CONTROLLER_SCROLL_HORIZONTAL: Emit scroll with horizontal deltas.
 * @CTK_EVENT_CONTROLLER_SCROLL_DISCRETE: Only emit deltas that are multiples of 1.
 * @CTK_EVENT_CONTROLLER_SCROLL_KINETIC: Emit #CtkEventControllerScroll::decelerate
 *   after continuous scroll finishes.
 * @CTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES: Emit scroll on both axes.
 *
 * Describes the behavior of a #CtkEventControllerScroll.
 *
 * Since: 3.24
 **/
typedef enum {
  CTK_EVENT_CONTROLLER_SCROLL_NONE       = 0,
  CTK_EVENT_CONTROLLER_SCROLL_VERTICAL   = 1 << 0,
  CTK_EVENT_CONTROLLER_SCROLL_HORIZONTAL = 1 << 1,
  CTK_EVENT_CONTROLLER_SCROLL_DISCRETE   = 1 << 2,
  CTK_EVENT_CONTROLLER_SCROLL_KINETIC    = 1 << 3,
  CTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES  = (CTK_EVENT_CONTROLLER_SCROLL_VERTICAL | CTK_EVENT_CONTROLLER_SCROLL_HORIZONTAL),
} CtkEventControllerScrollFlags;

GDK_AVAILABLE_IN_3_24
GType               ctk_event_controller_scroll_get_type  (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_3_24
CtkEventController *ctk_event_controller_scroll_new (CtkWidget                     *widget,
                                                     CtkEventControllerScrollFlags  flags);
GDK_AVAILABLE_IN_3_24
void                ctk_event_controller_scroll_set_flags (CtkEventControllerScroll      *controller,
                                                           CtkEventControllerScrollFlags  flags);
GDK_AVAILABLE_IN_3_24
CtkEventControllerScrollFlags
                    ctk_event_controller_scroll_get_flags (CtkEventControllerScroll      *controller);

G_END_DECLS

#endif /* __CTK_EVENT_CONTROLLER_SCROLL_H__ */
