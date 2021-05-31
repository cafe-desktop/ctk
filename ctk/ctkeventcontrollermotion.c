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

/**
 * SECTION:ctkeventcontrollermotion
 * @Short_description: Event controller for motion events
 * @Title: CtkEventControllerMotion
 * @See_also: #CtkEventController
 *
 * #CtkEventControllerMotion is an event controller meant for situations
 * where you need to track the position of the pointer.
 *
 * This object was added in 3.24.
 **/
#include "config.h"

#include "ctkintl.h"
#include "ctkmarshalers.h"
#include "ctkwidget.h"
#include "ctkeventcontrollerprivate.h"
#include "ctkeventcontrollermotion.h"
#include "ctktypebuiltins.h"
#include "ctkmarshalers.h"

struct _CtkEventControllerMotion
{
  CtkEventController parent_instance;
};

struct _CtkEventControllerMotionClass
{
  CtkEventControllerClass parent_class;
};

enum {
  ENTER,
  LEAVE,
  MOTION,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE (CtkEventControllerMotion, ctk_event_controller_motion, CTK_TYPE_EVENT_CONTROLLER)

static void
get_coords (CtkWidget      *widget,
            const GdkEvent *event,
            double         *x,
            double         *y)
{
  GdkWindow *window, *ancestor;
  CtkAllocation alloc;

  ctk_widget_get_allocation (widget, &alloc);
  gdk_event_get_coords (event, x, y);

  ancestor = ctk_widget_get_window (widget);
  window = gdk_event_get_window (event);

  while (window && ancestor && (window != ancestor))
    {
      gdk_window_coords_to_parent (window, *x, *y, x, y);
      window = gdk_window_get_parent (window);
    }

  if (!ctk_widget_get_has_window (widget))
    {
      *x -= alloc.x;
      *y -= alloc.y;
    }
}

static gboolean
ctk_event_controller_motion_handle_event (CtkEventController *controller,
                                          const GdkEvent     *event)
{
  CtkEventControllerClass *parent_class;
  CtkWidget *widget;
  GdkEventType type;

  widget = ctk_event_controller_get_widget (controller);

  type = gdk_event_get_event_type (event);
  if (type == GDK_ENTER_NOTIFY)
    {
      double x, y;
      get_coords (widget, event, &x, &y);
      g_signal_emit (controller, signals[ENTER], 0, x, y);
    }
  else if (type == GDK_LEAVE_NOTIFY)
    {
      g_signal_emit (controller, signals[LEAVE], 0);
    }
  else if (type == GDK_MOTION_NOTIFY)
    {
      double x, y;
      get_coords (widget, event, &x, &y);
      g_signal_emit (controller, signals[MOTION], 0, x, y);
    }

  parent_class = CTK_EVENT_CONTROLLER_CLASS (ctk_event_controller_motion_parent_class);

  return parent_class->handle_event (controller, event);
}

static void
ctk_event_controller_motion_class_init (CtkEventControllerMotionClass *klass)
{
  CtkEventControllerClass *controller_class = CTK_EVENT_CONTROLLER_CLASS (klass);

  controller_class->handle_event = ctk_event_controller_motion_handle_event;

  /**
   * CtkEventControllerMotion::enter:
   * @controller: The object that received the signal
   * @x: the x coordinate
   * @y: the y coordinate
   *
   * Signals that the pointer has entered the widget.
   */
  signals[ENTER] =
    g_signal_new (I_("enter"),
                  CTK_TYPE_EVENT_CONTROLLER_MOTION,
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL,
                  _ctk_marshal_VOID__DOUBLE_DOUBLE,
                  G_TYPE_NONE, 2, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
  g_signal_set_va_marshaller (signals[ENTER],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_VOID__DOUBLE_DOUBLEv);

  /**
   * CtkEventControllerMotion::leave:
   * @controller: The object that received the signal
   *
   * Signals that pointer has left the widget.
   */
  signals[LEAVE] =
    g_signal_new (I_("leave"),
                  CTK_TYPE_EVENT_CONTROLLER_MOTION,
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * CtkEventControllerMotion::motion:
   * @controller: The object that received the signal
   * @x: the x coordinate
   * @y: the y coordinate
   *
   * Emitted when the pointer moves inside the widget.
   */
  signals[MOTION] =
    g_signal_new (I_("motion"),
                  CTK_TYPE_EVENT_CONTROLLER_MOTION,
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL,
                  _ctk_marshal_VOID__DOUBLE_DOUBLE,
                  G_TYPE_NONE, 2, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
  g_signal_set_va_marshaller (signals[MOTION],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_VOID__DOUBLE_DOUBLEv);
}

static void
ctk_event_controller_motion_init (CtkEventControllerMotion *motion)
{
}

/**
 * ctk_event_controller_motion_new:
 * @widget: a #CtkWidget
 *
 * Creates a new event controller that will handle motion events
 * for the given @widget.
 *
 * Returns: a new #CtkEventControllerMotion
 *
 * Since: 3.24
 **/
CtkEventController *
ctk_event_controller_motion_new (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  return g_object_new (CTK_TYPE_EVENT_CONTROLLER_MOTION,
                       "widget", widget,
                       NULL);
}
