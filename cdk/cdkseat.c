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

#include "config.h"

#include <glib-object.h>
#include "cdkdisplay.h"
#include "cdkdevice.h"
#include "cdkseatprivate.h"
#include "cdkdeviceprivate.h"
#include "cdkintl.h"

/**
 * SECTION:cdkseat
 * @Short_description: Object representing an user seat
 * @Title: CdkSeat
 * @See_also: #CdkDisplay, #CdkDevice
 *
 * The #CdkSeat object represents a collection of input devices
 * that belong to a user.
 */

typedef struct _CdkSeatPrivate CdkSeatPrivate;

struct _CdkSeatPrivate
{
  CdkDisplay *display;
};

enum {
  DEVICE_ADDED,
  DEVICE_REMOVED,
  TOOL_ADDED,
  TOOL_REMOVED,
  N_SIGNALS
};

enum {
  PROP_0,
  PROP_DISPLAY,
  N_PROPS
};

static guint signals[N_SIGNALS] = { 0 };
static GParamSpec *props[N_PROPS] = { NULL };

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (CdkSeat, cdk_seat, G_TYPE_OBJECT)

static void
cdk_seat_set_property (GObject      *object,
                       guint         prop_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
  CdkSeatPrivate *priv = cdk_seat_get_instance_private (CDK_SEAT (object));

  switch (prop_id)
    {
    case PROP_DISPLAY:
      priv->display = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
cdk_seat_get_property (GObject    *object,
                       guint       prop_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
  CdkSeatPrivate *priv = cdk_seat_get_instance_private (CDK_SEAT (object));

  switch (prop_id)
    {
    case PROP_DISPLAY:
      g_value_set_object (value, priv->display);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
cdk_seat_class_init (CdkSeatClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = cdk_seat_set_property;
  object_class->get_property = cdk_seat_get_property;

  /**
   * CdkSeat::device-added:
   * @seat: the object on which the signal is emitted
   * @device: the newly added #CdkDevice.
   *
   * The ::device-added signal is emitted when a new input
   * device is related to this seat.
   *
   * Since: 3.20
   */
  signals [DEVICE_ADDED] =
    g_signal_new (g_intern_static_string ("device-added"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CdkSeatClass, device_added),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  CDK_TYPE_DEVICE);

  /**
   * CdkSeat::device-removed:
   * @seat: the object on which the signal is emitted
   * @device: the just removed #CdkDevice.
   *
   * The ::device-removed signal is emitted when an
   * input device is removed (e.g. unplugged).
   *
   * Since: 3.20
   */
  signals [DEVICE_REMOVED] =
    g_signal_new (g_intern_static_string ("device-removed"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CdkSeatClass, device_removed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  CDK_TYPE_DEVICE);

  /**
   * CdkSeat::tool-added:
   * @seat: the object on which the signal is emitted
   * @tool: the new #CdkDeviceTool known to the seat
   *
   * The ::tool-added signal is emitted whenever a new tool
   * is made known to the seat. The tool may later be assigned
   * to a device (i.e. on proximity with a tablet). The device
   * will emit the #CdkDevice::tool-changed signal accordingly.
   *
   * A same tool may be used by several devices.
   *
   * Since: 3.22
   */
  signals [TOOL_ADDED] =
    g_signal_new (g_intern_static_string ("tool-added"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  CDK_TYPE_DEVICE_TOOL);

  /**
   * CdkSeat::tool-removed:
   * @seat: the object on which the signal is emitted
   * @tool: the just removed #CdkDeviceTool
   *
   * This signal is emitted whenever a tool is no longer known
   * to this @seat.
   *
   * Since: 3.22
   */
  signals [TOOL_REMOVED] =
    g_signal_new (g_intern_static_string ("tool-removed"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  CDK_TYPE_DEVICE_TOOL);

  /**
   * CdkSeat:display:
   *
   * #CdkDisplay of this seat.
   *
   * Since: 3.20
   */
  props[PROP_DISPLAY] =
    g_param_spec_object ("display",
                         P_("Display"),
                         P_("Display"),
                         CDK_TYPE_DISPLAY,
                         G_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY |
                         G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, props);
}

static void
cdk_seat_init (CdkSeat *seat)
{
}

/**
 * cdk_seat_get_capabilities:
 * @seat: a #CdkSeat
 *
 * Returns the capabilities this #CdkSeat currently has.
 *
 * Returns: the seat capabilities
 *
 * Since: 3.20
 **/
CdkSeatCapabilities
cdk_seat_get_capabilities (CdkSeat *seat)
{
  CdkSeatClass *seat_class;

  g_return_val_if_fail (CDK_IS_SEAT (seat), CDK_SEAT_CAPABILITY_NONE);

  seat_class = CDK_SEAT_GET_CLASS (seat);
  return seat_class->get_capabilities (seat);
}

/**
 * cdk_seat_grab:
 * @seat: a #CdkSeat
 * @window: the #CdkWindow which will own the grab
 * @capabilities: capabilities that will be grabbed
 * @owner_events: if %FALSE then all device events are reported with respect to
 *                @window and are only reported if selected by @event_mask. If
 *                %TRUE then pointer events for this application are reported
 *                as normal, but pointer events outside this application are
 *                reported with respect to @window and only if selected by
 *                @event_mask. In either mode, unreported events are discarded.
 * @cursor: (nullable): the cursor to display while the grab is active. If
 *          this is %NULL then the normal cursors are used for
 *          @window and its descendants, and the cursor for @window is used
 *          elsewhere.
 * @event: (nullable): the event that is triggering the grab, or %NULL if none
 *         is available.
 * @prepare_func: (nullable) (scope call) (closure prepare_func_data): function to
 *                prepare the window to be grabbed, it can be %NULL if @window is
 *                visible before this call.
 * @prepare_func_data: user data to pass to @prepare_func
 *
 * Grabs the seat so that all events corresponding to the given @capabilities
 * are passed to this application until the seat is ungrabbed with cdk_seat_ungrab(),
 * or the window becomes hidden. This overrides any previous grab on the
 * seat by this client.
 *
 * As a rule of thumb, if a grab is desired over %CDK_SEAT_CAPABILITY_POINTER,
 * all other "pointing" capabilities (eg. %CDK_SEAT_CAPABILITY_TOUCH) should
 * be grabbed too, so the user is able to interact with all of those while
 * the grab holds, you should thus use %CDK_SEAT_CAPABILITY_ALL_POINTING most
 * commonly.
 *
 * Grabs are used for operations which need complete control over the
 * events corresponding to the given capabilities. For example in CTK+ this
 * is used for Drag and Drop operations, popup menus and such.
 *
 * Note that if the event mask of a #CdkWindow has selected both button press
 * and button release events, or touch begin and touch end, then a press event
 * will cause an automatic grab until the button is released, equivalent to a
 * grab on the window with @owner_events set to %TRUE. This is done because most
 * applications expect to receive paired press and release events.
 *
 * If you set up anything at the time you take the grab that needs to be
 * cleaned up when the grab ends, you should handle the #CdkEventGrabBroken
 * events that are emitted when the grab ends unvoluntarily.
 *
 * Returns: %CDK_GRAB_SUCCESS if the grab was successful.
 *
 * Since: 3.20
 **/
CdkGrabStatus
cdk_seat_grab (CdkSeat                *seat,
               CdkWindow              *window,
               CdkSeatCapabilities     capabilities,
               gboolean                owner_events,
               CdkCursor              *cursor,
               const CdkEvent         *event,
               CdkSeatGrabPrepareFunc  prepare_func,
               gpointer                prepare_func_data)
{
  CdkSeatClass *seat_class;

  g_return_val_if_fail (CDK_IS_SEAT (seat), CDK_GRAB_FAILED);
  g_return_val_if_fail (CDK_IS_WINDOW (window), CDK_GRAB_FAILED);

  capabilities &= CDK_SEAT_CAPABILITY_ALL;
  g_return_val_if_fail (capabilities != CDK_SEAT_CAPABILITY_NONE, CDK_GRAB_FAILED);

  seat_class = CDK_SEAT_GET_CLASS (seat);

  return seat_class->grab (seat, window, capabilities, owner_events, cursor,
                           event, prepare_func, prepare_func_data);
}

/**
 * cdk_seat_ungrab:
 * @seat: a #CdkSeat
 *
 * Releases a grab added through cdk_seat_grab().
 *
 * Since: 3.20
 **/
void
cdk_seat_ungrab (CdkSeat *seat)
{
  CdkSeatClass *seat_class;

  g_return_if_fail (CDK_IS_SEAT (seat));

  seat_class = CDK_SEAT_GET_CLASS (seat);
  seat_class->ungrab (seat);
}

/**
 * cdk_seat_get_slaves:
 * @seat: a #CdkSeat
 * @capabilities: capabilities to get devices for
 *
 * Returns the slave devices that match the given capabilities.
 *
 * Returns: (transfer container) (element-type CdkDevice): A list of #CdkDevices.
 *          The list must be freed with g_list_free(), the elements are owned
 *          by CDK and must not be freed.
 *
 * Since: 3.20
 **/
GList *
cdk_seat_get_slaves (CdkSeat             *seat,
                     CdkSeatCapabilities  capabilities)
{
  CdkSeatClass *seat_class;

  g_return_val_if_fail (CDK_IS_SEAT (seat), NULL);

  seat_class = CDK_SEAT_GET_CLASS (seat);
  return seat_class->get_slaves (seat, capabilities);
}

/**
 * cdk_seat_get_pointer:
 * @seat: a #CdkSeat
 *
 * Returns the master device that routes pointer events.
 *
 * Returns: (transfer none) (nullable): a master #CdkDevice with pointer
 *          capabilities. This object is owned by CTK+ and must not be freed.
 *
 * Since: 3.20
 **/
CdkDevice *
cdk_seat_get_pointer (CdkSeat *seat)
{
  CdkSeatClass *seat_class;

  g_return_val_if_fail (CDK_IS_SEAT (seat), NULL);

  seat_class = CDK_SEAT_GET_CLASS (seat);
  return seat_class->get_master (seat, CDK_SEAT_CAPABILITY_POINTER);
}

/**
 * cdk_seat_get_keyboard:
 * @seat: a #CdkSeat
 *
 * Returns the master device that routes keyboard events.
 *
 * Returns: (transfer none) (nullable): a master #CdkDevice with keyboard
 *          capabilities. This object is owned by CTK+ and must not be freed.
 *
 * Since: 3.20
 **/
CdkDevice *
cdk_seat_get_keyboard (CdkSeat *seat)
{
  CdkSeatClass *seat_class;

  g_return_val_if_fail (CDK_IS_SEAT (seat), NULL);

  seat_class = CDK_SEAT_GET_CLASS (seat);
  return seat_class->get_master (seat, CDK_SEAT_CAPABILITY_KEYBOARD);
}

void
cdk_seat_device_added (CdkSeat   *seat,
                       CdkDevice *device)
{
  cdk_device_set_seat (device, seat);
  g_signal_emit (seat, signals[DEVICE_ADDED], 0, device);
}

void
cdk_seat_device_removed (CdkSeat   *seat,
                         CdkDevice *device)
{
  cdk_device_set_seat (device, NULL);
  g_signal_emit (seat, signals[DEVICE_REMOVED], 0, device);
}

/**
 * cdk_seat_get_display:
 * @seat: a #CdkSeat
 *
 * Returns the #CdkDisplay this seat belongs to.
 *
 * Returns: (transfer none): a #CdkDisplay. This object is owned by CTK+
 *          and must not be freed.
 **/
CdkDisplay *
cdk_seat_get_display (CdkSeat *seat)
{
  CdkSeatPrivate *priv = cdk_seat_get_instance_private (seat);

  g_return_val_if_fail (CDK_IS_SEAT (seat), NULL);

  return priv->display;
}

void
cdk_seat_tool_added (CdkSeat       *seat,
                     CdkDeviceTool *tool)
{
  g_signal_emit (seat, signals[TOOL_ADDED], 0, tool);
}

void
cdk_seat_tool_removed (CdkSeat       *seat,
                       CdkDeviceTool *tool)
{
  g_signal_emit (seat, signals[TOOL_REMOVED], 0, tool);
}

CdkDeviceTool *
cdk_seat_get_tool (CdkSeat *seat,
                   guint64  serial,
                   guint64  hw_id)
{
  CdkSeatClass *seat_class;

  g_return_val_if_fail (CDK_IS_SEAT (seat), NULL);

  seat_class = CDK_SEAT_GET_CLASS (seat);
  return seat_class->get_tool (seat, serial, hw_id);
}
