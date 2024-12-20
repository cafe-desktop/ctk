/* CDK - The GIMP Drawing Kit
 * Copyright (C) 2009 Carlos Garnacho <carlosg@gnome.org>
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
 */

#include "config.h"

#include "cdkdevicemanagerprivate.h"
#include "cdkdisplay.h"
#include "cdkintl.h"


/**
 * SECTION:cdkdevicemanager
 * @Short_description: Functions for handling input devices
 * @Title: CdkDeviceManager
 * @See_also: #CdkDevice, #CdkEvent
 *
 * In addition to a single pointer and keyboard for user interface input,
 * CDK contains support for a variety of input devices, including graphics
 * tablets, touchscreens and multiple pointers/keyboards interacting
 * simultaneously with the user interface. Such input devices often have
 * additional features, such as sub-pixel positioning information and
 * additional device-dependent information.
 *
 * In order to query the device hierarchy and be aware of changes in the
 * device hierarchy (such as virtual devices being created or removed, or
 * physical devices being plugged or unplugged), CDK provides
 * #CdkDeviceManager.
 *
 * By default, and if the platform supports it, CDK is aware of multiple
 * keyboard/pointer pairs and multitouch devices. This behavior can be
 * changed by calling cdk_disable_multidevice() before cdk_display_open().
 * There should rarely be a need to do that though, since CDK defaults
 * to a compatibility mode in which it will emit just one enter/leave
 * event pair for all devices on a window. To enable per-device
 * enter/leave events and other multi-pointer interaction features,
 * cdk_window_set_support_multidevice() must be called on
 * #CdkWindows (or ctk_widget_set_support_multidevice() on widgets).
 * window. See the cdk_window_set_support_multidevice() documentation
 * for more information.
 *
 * On X11, multi-device support is implemented through XInput 2.
 * Unless cdk_disable_multidevice() is called, the XInput 2
 * #CdkDeviceManager implementation will be used as the input source.
 * Otherwise either the core or XInput 1 implementations will be used.
 *
 * For simple applications that don’t have any special interest in
 * input devices, the so-called “client pointer”
 * provides a reasonable approximation to a simple setup with a single
 * pointer and keyboard. The device that has been set as the client
 * pointer can be accessed via cdk_device_manager_get_client_pointer().
 *
 * Conceptually, in multidevice mode there are 2 device types. Virtual
 * devices (or master devices) are represented by the pointer cursors
 * and keyboard foci that are seen on the screen. Physical devices (or
 * slave devices) represent the hardware that is controlling the virtual
 * devices, and thus have no visible cursor on the screen.
 *
 * Virtual devices are always paired, so there is a keyboard device for every
 * pointer device. Associations between devices may be inspected through
 * cdk_device_get_associated_device().
 *
 * There may be several virtual devices, and several physical devices could
 * be controlling each of these virtual devices. Physical devices may also
 * be “floating”, which means they are not attached to any virtual device.
 *
 * # Master and slave devices
 *
 * |[
 * carlos@sacarino:~$ xinput list
 * ⎡ Virtual core pointer                          id=2    [master pointer  (3)]
 * ⎜   ↳ Virtual core XTEST pointer                id=4    [slave  pointer  (2)]
 * ⎜   ↳ Wacom ISDv4 E6 Pen stylus                 id=10   [slave  pointer  (2)]
 * ⎜   ↳ Wacom ISDv4 E6 Finger touch               id=11   [slave  pointer  (2)]
 * ⎜   ↳ SynPS/2 Synaptics TouchPad                id=13   [slave  pointer  (2)]
 * ⎜   ↳ TPPS/2 IBM TrackPoint                     id=14   [slave  pointer  (2)]
 * ⎜   ↳ Wacom ISDv4 E6 Pen eraser                 id=16   [slave  pointer  (2)]
 * ⎣ Virtual core keyboard                         id=3    [master keyboard (2)]
 *     ↳ Virtual core XTEST keyboard               id=5    [slave  keyboard (3)]
 *     ↳ Power Button                              id=6    [slave  keyboard (3)]
 *     ↳ Video Bus                                 id=7    [slave  keyboard (3)]
 *     ↳ Sleep Button                              id=8    [slave  keyboard (3)]
 *     ↳ Integrated Camera                         id=9    [slave  keyboard (3)]
 *     ↳ AT Translated Set 2 keyboard              id=12   [slave  keyboard (3)]
 *     ↳ ThinkPad Extra Buttons                    id=15   [slave  keyboard (3)]
 * ]|
 *
 * By default, CDK will automatically listen for events coming from all
 * master devices, setting the #CdkDevice for all events coming from input
 * devices. Events containing device information are #CDK_MOTION_NOTIFY,
 * #CDK_BUTTON_PRESS, #CDK_2BUTTON_PRESS, #CDK_3BUTTON_PRESS,
 * #CDK_BUTTON_RELEASE, #CDK_SCROLL, #CDK_KEY_PRESS, #CDK_KEY_RELEASE,
 * #CDK_ENTER_NOTIFY, #CDK_LEAVE_NOTIFY, #CDK_FOCUS_CHANGE,
 * #CDK_PROXIMITY_IN, #CDK_PROXIMITY_OUT, #CDK_DRAG_ENTER, #CDK_DRAG_LEAVE,
 * #CDK_DRAG_MOTION, #CDK_DRAG_STATUS, #CDK_DROP_START, #CDK_DROP_FINISHED
 * and #CDK_GRAB_BROKEN. When dealing with an event on a master device,
 * it is possible to get the source (slave) device that the event originated
 * from via cdk_event_get_source_device().
 *
 * On a standard session, all physical devices are connected by default to
 * the "Virtual Core Pointer/Keyboard" master devices, hence routing all events
 * through these. This behavior is only modified by device grabs, where the
 * slave device is temporarily detached for as long as the grab is held, and
 * more permanently by user modifications to the device hierarchy.
 *
 * On certain application specific setups, it may make sense
 * to detach a physical device from its master pointer, and mapping it to
 * an specific window. This can be achieved by the combination of
 * cdk_device_grab() and cdk_device_set_mode().
 *
 * In order to listen for events coming from devices
 * other than a virtual device, cdk_window_set_device_events() must be
 * called. Generally, this function can be used to modify the event mask
 * for any given device.
 *
 * Input devices may also provide additional information besides X/Y.
 * For example, graphics tablets may also provide pressure and X/Y tilt
 * information. This information is device-dependent, and may be
 * queried through cdk_device_get_axis(). In multidevice mode, virtual
 * devices will change axes in order to always represent the physical
 * device that is routing events through it. Whenever the physical device
 * changes, the #CdkDevice:n-axes property will be notified, and
 * cdk_device_list_axes() will return the new device axes.
 *
 * Devices may also have associated “keys” or
 * macro buttons. Such keys can be globally set to map into normal X
 * keyboard events. The mapping is set using cdk_device_set_key().
 *
 * In CTK+ 3.20, a new #CdkSeat object has been introduced that
 * supersedes #CdkDeviceManager and should be preferred in newly
 * written code.
 */

static void cdk_device_manager_set_property (GObject      *object,
                                             guint         prop_id,
                                             const GValue *value,
                                             GParamSpec   *pspec);
static void cdk_device_manager_get_property (GObject      *object,
                                             guint         prop_id,
                                             GValue       *value,
                                             GParamSpec   *pspec);


G_DEFINE_ABSTRACT_TYPE (CdkDeviceManager, cdk_device_manager, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_DISPLAY
};

enum {
  DEVICE_ADDED,
  DEVICE_REMOVED,
  DEVICE_CHANGED,
  LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };


static void
cdk_device_manager_class_init (CdkDeviceManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = cdk_device_manager_set_property;
  object_class->get_property = cdk_device_manager_get_property;

  g_object_class_install_property (object_class,
                                   PROP_DISPLAY,
                                   g_param_spec_object ("display",
                                                        P_("Display"),
                                                        P_("Display for the device manager"),
                                                        CDK_TYPE_DISPLAY,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * CdkDeviceManager::device-added:
   * @device_manager: the object on which the signal is emitted
   * @device: the newly added #CdkDevice.
   *
   * The ::device-added signal is emitted either when a new master
   * pointer is created, or when a slave (Hardware) input device
   * is plugged in.
   */
  signals [DEVICE_ADDED] =
    g_signal_new (g_intern_static_string ("device-added"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CdkDeviceManagerClass, device_added),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  CDK_TYPE_DEVICE);

  /**
   * CdkDeviceManager::device-removed:
   * @device_manager: the object on which the signal is emitted
   * @device: the just removed #CdkDevice.
   *
   * The ::device-removed signal is emitted either when a master
   * pointer is removed, or when a slave (Hardware) input device
   * is unplugged.
   */
  signals [DEVICE_REMOVED] =
    g_signal_new (g_intern_static_string ("device-removed"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CdkDeviceManagerClass, device_removed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  CDK_TYPE_DEVICE);

  /**
   * CdkDeviceManager::device-changed:
   * @device_manager: the object on which the signal is emitted
   * @device: the #CdkDevice that changed.
   *
   * The ::device-changed signal is emitted whenever a device
   * has changed in the hierarchy, either slave devices being
   * disconnected from their master device or connected to
   * another one, or master devices being added or removed
   * a slave device.
   *
   * If a slave device is detached from all master devices
   * (cdk_device_get_associated_device() returns %NULL), its
   * #CdkDeviceType will change to %CDK_DEVICE_TYPE_FLOATING,
   * if it's attached, it will change to %CDK_DEVICE_TYPE_SLAVE.
   */
  signals [DEVICE_CHANGED] =
    g_signal_new (g_intern_static_string ("device-changed"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CdkDeviceManagerClass, device_changed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  CDK_TYPE_DEVICE);
}

static void
cdk_device_manager_init (CdkDeviceManager *device_manager G_GNUC_UNUSED)
{
}

static void
cdk_device_manager_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  switch (prop_id)
    {
    case PROP_DISPLAY:
      CDK_DEVICE_MANAGER (object)->display = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
cdk_device_manager_get_property (GObject      *object,
                                 guint         prop_id,
                                 GValue       *value,
                                 GParamSpec   *pspec)
{

  switch (prop_id)
    {
    case PROP_DISPLAY:
      g_value_set_object (value, CDK_DEVICE_MANAGER (object)->display);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * cdk_device_manager_get_display:
 * @device_manager: a #CdkDeviceManager
 *
 * Gets the #CdkDisplay associated to @device_manager.
 *
 * Returns: (nullable) (transfer none): the #CdkDisplay to which
 *          @device_manager is associated to, or %NULL. This memory is
 *          owned by CDK and must not be freed or unreferenced.
 *
 * Since: 3.0
 **/
CdkDisplay *
cdk_device_manager_get_display (CdkDeviceManager *device_manager)
{
  g_return_val_if_fail (CDK_IS_DEVICE_MANAGER (device_manager), NULL);

  return device_manager->display;
}

/**
 * cdk_device_manager_list_devices:
 * @device_manager: a #CdkDeviceManager
 * @type: device type to get.
 *
 * Returns the list of devices of type @type currently attached to
 * @device_manager.
 *
 * Returns: (transfer container) (element-type Cdk.Device): a list of 
 *          #CdkDevices. The returned list must be
 *          freed with g_list_free (). The list elements are owned by
 *          CTK+ and must not be freed or unreffed.
 *
 * Since: 3.0
 *
 * Deprecated: 3.20, use cdk_seat_get_pointer(), cdk_seat_get_keyboard()
 *             and cdk_seat_get_slaves() instead.
 **/
GList *
cdk_device_manager_list_devices (CdkDeviceManager *device_manager,
                                 CdkDeviceType     type)
{
  g_return_val_if_fail (CDK_IS_DEVICE_MANAGER (device_manager), NULL);

  return CDK_DEVICE_MANAGER_GET_CLASS (device_manager)->list_devices (device_manager, type);
}

/**
 * cdk_device_manager_get_client_pointer:
 * @device_manager: a #CdkDeviceManager
 *
 * Returns the client pointer, that is, the master pointer that acts as the core pointer
 * for this application. In X11, window managers may change this depending on the interaction
 * pattern under the presence of several pointers.
 *
 * You should use this function seldomly, only in code that isn’t triggered by a #CdkEvent
 * and there aren’t other means to get a meaningful #CdkDevice to operate on.
 *
 * Returns: (transfer none): The client pointer. This memory is
 *          owned by CDK and must not be freed or unreferenced.
 *
 * Since: 3.0
 *
 * Deprecated: 3.20: Use cdk_seat_get_pointer() instead.
 **/
CdkDevice *
cdk_device_manager_get_client_pointer (CdkDeviceManager *device_manager)
{
  g_return_val_if_fail (CDK_IS_DEVICE_MANAGER (device_manager), NULL);

  return CDK_DEVICE_MANAGER_GET_CLASS (device_manager)->get_client_pointer (device_manager);
}
