/* GDK - The GIMP Drawing Kit
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

#include <cdk/cdktypes.h>
#include <cdk/cdkdevicemanager.h>
#include <cdk/cdkdeviceprivate.h>
#include <cdk/cdkseatdefaultprivate.h>
#include <cdk/cdkdevicemanagerprivate.h>
#include <cdk/cdkdisplayprivate.h>
#include "cdkdevicemanager-core-quartz.h"
#include "cdkquartzdevice-core.h"
#include "cdkkeysyms.h"
#include "cdkprivate-quartz.h"
#include "cdkinternal-quartz.h"

typedef enum
{
#if MAC_OS_X_VERSION_MIN_REQUIRED < 101200
  GDK_QUARTZ_POINTER_DEVICE_TYPE_CURSOR = NSCursorPointingDevice,
  GDK_QUARTZ_POINTER_DEVICE_TYPE_ERASER = NSEraserPointingDevice,
  GDK_QUARTZ_POINTER_DEVICE_TYPE_PEN = NSPenPointingDevice,
#else
  GDK_QUARTZ_POINTER_DEVICE_TYPE_CURSOR = NSPointingDeviceTypeCursor,
  GDK_QUARTZ_POINTER_DEVICE_TYPE_ERASER = NSPointingDeviceTypeEraser,
  GDK_QUARTZ_POINTER_DEVICE_TYPE_PEN = NSPointingDeviceTypePen,
#endif
} CdkQuartzPointerDeviceType;

#define HAS_FOCUS(toplevel)                           \
  ((toplevel)->has_focus || (toplevel)->has_pointer_focus)

static void    cdk_quartz_device_manager_core_finalize    (GObject *object);
static void    cdk_quartz_device_manager_core_constructed (GObject *object);

static GList * cdk_quartz_device_manager_core_list_devices (CdkDeviceManager *device_manager,
                                                            CdkDeviceType     type);
static CdkDevice * cdk_quartz_device_manager_core_get_client_pointer (CdkDeviceManager *device_manager);


G_DEFINE_TYPE (CdkQuartzDeviceManagerCore, cdk_quartz_device_manager_core, GDK_TYPE_DEVICE_MANAGER)

static void
cdk_quartz_device_manager_core_class_init (CdkQuartzDeviceManagerCoreClass *klass)
{
  CdkDeviceManagerClass *device_manager_class = GDK_DEVICE_MANAGER_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = cdk_quartz_device_manager_core_finalize;
  object_class->constructed = cdk_quartz_device_manager_core_constructed;
  device_manager_class->list_devices = cdk_quartz_device_manager_core_list_devices;
  device_manager_class->get_client_pointer = cdk_quartz_device_manager_core_get_client_pointer;
}

static CdkDevice *
create_core_pointer (CdkDeviceManager *device_manager,
                     CdkDisplay       *display)
{
  return g_object_new (GDK_TYPE_QUARTZ_DEVICE_CORE,
                       "name", "Core Pointer",
                       "type", GDK_DEVICE_TYPE_MASTER,
                       "input-source", GDK_SOURCE_MOUSE,
                       "input-mode", GDK_MODE_SCREEN,
                       "has-cursor", TRUE,
                       "display", display,
                       "device-manager", device_manager,
                       NULL);
}

static CdkDevice *
create_core_keyboard (CdkDeviceManager *device_manager,
                      CdkDisplay       *display)
{
  return g_object_new (GDK_TYPE_QUARTZ_DEVICE_CORE,
                       "name", "Core Keyboard",
                       "type", GDK_DEVICE_TYPE_MASTER,
                       "input-source", GDK_SOURCE_KEYBOARD,
                       "input-mode", GDK_MODE_SCREEN,
                       "has-cursor", FALSE,
                       "display", display,
                       "device-manager", device_manager,
                       NULL);
}

static void
cdk_quartz_device_manager_core_init (CdkQuartzDeviceManagerCore *device_manager)
{
  device_manager->known_tablet_devices = NULL;
}

static void
cdk_quartz_device_manager_core_finalize (GObject *object)
{
  CdkQuartzDeviceManagerCore *quartz_device_manager_core;

  quartz_device_manager_core = GDK_QUARTZ_DEVICE_MANAGER_CORE (object);

  g_object_unref (quartz_device_manager_core->core_pointer);
  g_object_unref (quartz_device_manager_core->core_keyboard);

  g_list_free_full (quartz_device_manager_core->known_tablet_devices, g_object_unref);

  G_OBJECT_CLASS (cdk_quartz_device_manager_core_parent_class)->finalize (object);
}

static void
cdk_quartz_device_manager_core_constructed (GObject *object)
{
  CdkQuartzDeviceManagerCore *device_manager;
  CdkDisplay *display;
  CdkSeat *seat;

  device_manager = GDK_QUARTZ_DEVICE_MANAGER_CORE (object);
  display = cdk_device_manager_get_display (GDK_DEVICE_MANAGER (object));
  device_manager->core_pointer = create_core_pointer (GDK_DEVICE_MANAGER (device_manager), display);
  device_manager->core_keyboard = create_core_keyboard (GDK_DEVICE_MANAGER (device_manager), display);

  _cdk_device_set_associated_device (device_manager->core_pointer, device_manager->core_keyboard);
  _cdk_device_set_associated_device (device_manager->core_keyboard, device_manager->core_pointer);

  seat = cdk_seat_default_new_for_master_pair (device_manager->core_pointer,
                                               device_manager->core_keyboard);
  cdk_display_add_seat (display, seat);
  g_object_unref (seat);
}

static GList *
cdk_quartz_device_manager_core_list_devices (CdkDeviceManager *device_manager,
                                             CdkDeviceType     type)
{
  CdkQuartzDeviceManagerCore *self;
  GList *devices = NULL;
  GList *l;

  self = GDK_QUARTZ_DEVICE_MANAGER_CORE (device_manager);

  if (type == GDK_DEVICE_TYPE_MASTER)
    {
      devices = g_list_prepend (devices, self->core_keyboard);
      devices = g_list_prepend (devices, self->core_pointer);
    }

  for (l = self->known_tablet_devices; l; l = g_list_next (l))
    {
      devices = g_list_prepend (devices, GDK_DEVICE (l->data));
    }

  devices = g_list_reverse (devices);

  return devices;
}

static CdkDevice *
cdk_quartz_device_manager_core_get_client_pointer (CdkDeviceManager *device_manager)
{
  CdkQuartzDeviceManagerCore *quartz_device_manager_core;

  quartz_device_manager_core = (CdkQuartzDeviceManagerCore *) device_manager;
  return quartz_device_manager_core->core_pointer;
}

static CdkDevice *
create_core_device (CdkDeviceManager *device_manager,
                    const gchar      *device_name,
                    CdkInputSource    source)
{
  CdkDisplay *display = cdk_device_manager_get_display (device_manager);
  CdkDevice *device = g_object_new (GDK_TYPE_QUARTZ_DEVICE_CORE,
                                    "name", device_name,
                                    "type", GDK_DEVICE_TYPE_SLAVE,
                                    "input-source", source,
                                    "input-mode", GDK_MODE_DISABLED,
                                    "has-cursor", FALSE,
                                    "display", display,
                                    "device-manager", device_manager,
                                    NULL);

  _cdk_device_add_axis (device, GDK_NONE, GDK_AXIS_PRESSURE, 0.0, 1.0, 0.001);
  _cdk_device_add_axis (device, GDK_NONE, GDK_AXIS_XTILT, -1.0, 1.0, 0.001);
  _cdk_device_add_axis (device, GDK_NONE, GDK_AXIS_YTILT, -1.0, 1.0, 0.001);

  return device;
}

static void
mimic_device_axes (CdkDevice *logical,
                   CdkDevice *physical)
{
  double axis_min, axis_max, axis_resolution;
  CdkAtom axis_label;
  CdkAxisUse axis_use;
  int axis_count;
  int i;

  axis_count = cdk_device_get_n_axes (physical);

  for (i = 0; i < axis_count; i++)
    {
      _cdk_device_get_axis_info (physical, i, &axis_label, &axis_use, &axis_min,
                                 &axis_max, &axis_resolution);
      _cdk_device_add_axis (logical, axis_label, axis_use, axis_min,
                            axis_max, axis_resolution);
    }
}

static void
translate_device_axes (CdkDevice *source_device,
                       gboolean   active)
{
  CdkSeat *seat = cdk_display_get_default_seat (_cdk_display);
  CdkDevice *core_pointer = cdk_seat_get_pointer (seat);

  g_object_freeze_notify (G_OBJECT (core_pointer));

  _cdk_device_reset_axes (core_pointer);
  if (active && source_device)
    {
      mimic_device_axes (core_pointer, source_device);
    }
  else
    {
      _cdk_device_add_axis (core_pointer, GDK_NONE, GDK_AXIS_X, 0, 0, 1);
      _cdk_device_add_axis (core_pointer, GDK_NONE, GDK_AXIS_Y, 0, 0, 1);
    }

  g_object_thaw_notify (G_OBJECT (core_pointer));
}

void
_cdk_quartz_device_manager_register_device_for_ns_event (CdkDeviceManager *device_manager,
                                                         NSEvent          *nsevent)
{
  CdkQuartzDeviceManagerCore *self = GDK_QUARTZ_DEVICE_MANAGER_CORE (device_manager);
  GList *l = NULL;
  CdkInputSource input_source = GDK_SOURCE_MOUSE;
  CdkDevice *device = NULL;

  /* Only handle device updates for proximity events */
  if ([nsevent type] != GDK_QUARTZ_EVENT_TABLET_PROXIMITY &&
      [nsevent subtype] != GDK_QUARTZ_EVENT_SUBTYPE_TABLET_PROXIMITY)
    return;

  if ([nsevent pointingDeviceType] == GDK_QUARTZ_POINTER_DEVICE_TYPE_PEN)
    input_source = GDK_SOURCE_PEN;
  else if ([nsevent pointingDeviceType] == GDK_QUARTZ_POINTER_DEVICE_TYPE_CURSOR)
    input_source = GDK_SOURCE_CURSOR;
  else if ([nsevent pointingDeviceType] == GDK_QUARTZ_POINTER_DEVICE_TYPE_ERASER)
    input_source = GDK_SOURCE_ERASER;

  for (l = self->known_tablet_devices; l; l = g_list_next (l))
    {
      CdkDevice *device_to_check = GDK_DEVICE (l->data);

      if (input_source == cdk_device_get_source (device_to_check) &&
          [nsevent uniqueID] == _cdk_quartz_device_core_get_unique (device_to_check))
        {
          device = device_to_check;
          if ([nsevent isEnteringProximity])
            {
              if (!_cdk_quartz_device_core_is_active (device, [nsevent deviceID]))
                self->num_active_devices++;

              _cdk_quartz_device_core_set_active (device, TRUE, [nsevent deviceID]);
            }
          else
            {
              if (_cdk_quartz_device_core_is_active (device, [nsevent deviceID]))
                self->num_active_devices--;

              _cdk_quartz_device_core_set_active (device, FALSE, [nsevent deviceID]);
            }
        }
    }

  /* If we haven't seen this device before, add it */
  if (!device)
    {
      CdkSeat *seat;

      switch (input_source)
        {
        case GDK_SOURCE_PEN:
          device = create_core_device (device_manager,
                                       "Quartz Pen",
                                       GDK_SOURCE_PEN);
          break;
        case GDK_SOURCE_CURSOR:
          device = create_core_device (device_manager,
                                       "Quartz Cursor",
                                       GDK_SOURCE_CURSOR);
          break;
        case GDK_SOURCE_ERASER:
          device = create_core_device (device_manager,
                                       "Quartz Eraser",
                                       GDK_SOURCE_ERASER);
          break;
        default:
          g_warning ("GDK Quarz unknown input source: %i", input_source);
          break;
        }

      _cdk_device_set_associated_device (GDK_DEVICE (device), self->core_pointer);
      _cdk_device_add_slave (self->core_pointer, GDK_DEVICE (device));

      seat = cdk_device_get_seat (self->core_pointer);
      cdk_seat_default_add_slave (GDK_SEAT_DEFAULT (seat), device);

      _cdk_quartz_device_core_set_unique (device, [nsevent uniqueID]);
      _cdk_quartz_device_core_set_active (device, TRUE, [nsevent deviceID]);

      self->known_tablet_devices = g_list_append (self->known_tablet_devices,
                                                  device);

      if ([nsevent isEnteringProximity])
        {
          if (!_cdk_quartz_device_core_is_active (device, [nsevent deviceID]))
            self->num_active_devices++;
          _cdk_quartz_device_core_set_active (device, TRUE, [nsevent deviceID]);
        }
    }

  translate_device_axes (device, [nsevent isEnteringProximity]);

  if (self->num_active_devices)
    [NSEvent setMouseCoalescingEnabled: FALSE];
  else
    [NSEvent setMouseCoalescingEnabled: TRUE];
}

CdkDevice *
_cdk_quartz_device_manager_core_device_for_ns_event (CdkDeviceManager *device_manager,
                                                     NSEvent          *nsevent)
{
  CdkQuartzDeviceManagerCore *self = GDK_QUARTZ_DEVICE_MANAGER_CORE (device_manager);
  CdkDevice *device = NULL;

  if ([nsevent type] == GDK_QUARTZ_EVENT_TABLET_PROXIMITY ||
      [nsevent subtype] == GDK_QUARTZ_EVENT_SUBTYPE_TABLET_PROXIMITY ||
      [nsevent subtype] == GDK_QUARTZ_EVENT_SUBTYPE_TABLET_POINT)
    {
      /* Find the device based on deviceID */
      GList *l = NULL;

      for (l = self->known_tablet_devices; l && !device; l = g_list_next (l))
        {
          CdkDevice *device_to_check = GDK_DEVICE (l->data);

          if (_cdk_quartz_device_core_is_active (device_to_check, [nsevent deviceID]))
            device = device_to_check;
        }
    }

  if (!device)
    device = self->core_pointer;

  return device;
}
