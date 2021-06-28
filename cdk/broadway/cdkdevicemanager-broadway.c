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

#include "cdkdevicemanager-broadway.h"

#include "cdktypes.h"
#include "cdkdevicemanager.h"
#include "cdkdevice-broadway.h"
#include "cdkkeysyms.h"
#include "cdkprivate-broadway.h"
#include "cdkseatdefaultprivate.h"

#define HAS_FOCUS(toplevel)                           \
  ((toplevel)->has_focus || (toplevel)->has_pointer_focus)

static void    cdk_broadway_device_manager_finalize    (GObject *object);
static void    cdk_broadway_device_manager_constructed (GObject *object);

static GList * cdk_broadway_device_manager_list_devices (CdkDeviceManager *device_manager,
							 CdkDeviceType     type);
static CdkDevice * cdk_broadway_device_manager_get_client_pointer (CdkDeviceManager *device_manager);

G_DEFINE_TYPE (CdkBroadwayDeviceManager, cdk_broadway_device_manager, CDK_TYPE_DEVICE_MANAGER)

static void
cdk_broadway_device_manager_class_init (CdkBroadwayDeviceManagerClass *klass)
{
  CdkDeviceManagerClass *device_manager_class = CDK_DEVICE_MANAGER_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = cdk_broadway_device_manager_finalize;
  object_class->constructed = cdk_broadway_device_manager_constructed;
  device_manager_class->list_devices = cdk_broadway_device_manager_list_devices;
  device_manager_class->get_client_pointer = cdk_broadway_device_manager_get_client_pointer;
}

static CdkDevice *
create_core_pointer (CdkDeviceManager *device_manager,
                     CdkDisplay       *display)
{
  return g_object_new (CDK_TYPE_BROADWAY_DEVICE,
                       "name", "Core Pointer",
                       "type", CDK_DEVICE_TYPE_MASTER,
                       "input-source", CDK_SOURCE_MOUSE,
                       "input-mode", CDK_MODE_SCREEN,
                       "has-cursor", TRUE,
                       "display", display,
                       "device-manager", device_manager,
                       NULL);
}

static CdkDevice *
create_core_keyboard (CdkDeviceManager *device_manager,
                      CdkDisplay       *display)
{
  return g_object_new (CDK_TYPE_BROADWAY_DEVICE,
                       "name", "Core Keyboard",
                       "type", CDK_DEVICE_TYPE_MASTER,
                       "input-source", CDK_SOURCE_KEYBOARD,
                       "input-mode", CDK_MODE_SCREEN,
                       "has-cursor", FALSE,
                       "display", display,
                       "device-manager", device_manager,
                       NULL);
}

static CdkDevice *
create_touchscreen (CdkDeviceManager *device_manager,
                    CdkDisplay       *display)
{
  return g_object_new (CDK_TYPE_BROADWAY_DEVICE,
                       "name", "Touchscreen",
                       "type", CDK_DEVICE_TYPE_SLAVE,
                       "input-source", CDK_SOURCE_TOUCHSCREEN,
                       "input-mode", CDK_MODE_SCREEN,
                       "has-cursor", FALSE,
                       "display", display,
                       "device-manager", device_manager,
                       NULL);
}

static void
cdk_broadway_device_manager_init (CdkBroadwayDeviceManager *device_manager)
{
}

static void
cdk_broadway_device_manager_finalize (GObject *object)
{
  CdkBroadwayDeviceManager *device_manager;

  device_manager = CDK_BROADWAY_DEVICE_MANAGER (object);

  g_object_unref (device_manager->core_pointer);
  g_object_unref (device_manager->core_keyboard);
  g_object_unref (device_manager->touchscreen);

  G_OBJECT_CLASS (cdk_broadway_device_manager_parent_class)->finalize (object);
}

static void
cdk_broadway_device_manager_constructed (GObject *object)
{
  CdkBroadwayDeviceManager *device_manager;
  CdkDisplay *display;
  CdkSeat *seat;

  device_manager = CDK_BROADWAY_DEVICE_MANAGER (object);
  display = cdk_device_manager_get_display (CDK_DEVICE_MANAGER (object));
  device_manager->core_pointer = create_core_pointer (CDK_DEVICE_MANAGER (device_manager), display);
  device_manager->core_keyboard = create_core_keyboard (CDK_DEVICE_MANAGER (device_manager), display);
  device_manager->touchscreen = create_touchscreen (CDK_DEVICE_MANAGER (device_manager), display);

  _cdk_device_set_associated_device (device_manager->core_pointer, device_manager->core_keyboard);
  _cdk_device_set_associated_device (device_manager->core_keyboard, device_manager->core_pointer);
  _cdk_device_set_associated_device (device_manager->touchscreen, device_manager->core_pointer);
  _cdk_device_add_slave (device_manager->core_pointer, device_manager->touchscreen);

  seat = cdk_seat_default_new_for_master_pair (device_manager->core_pointer,
                                               device_manager->core_keyboard);
  cdk_display_add_seat (display, seat);
  cdk_seat_default_add_slave (CDK_SEAT_DEFAULT (seat), device_manager->touchscreen);
  g_object_unref (seat);
}


static GList *
cdk_broadway_device_manager_list_devices (CdkDeviceManager *device_manager,
					  CdkDeviceType     type)
{
  CdkBroadwayDeviceManager *broadway_device_manager = (CdkBroadwayDeviceManager *) device_manager;
  GList *devices = NULL;

  if (type == CDK_DEVICE_TYPE_MASTER)
    {
      devices = g_list_prepend (devices, broadway_device_manager->core_keyboard);
      devices = g_list_prepend (devices, broadway_device_manager->core_pointer);
    }

  if (type == CDK_DEVICE_TYPE_SLAVE)
    {
      devices = g_list_prepend (devices, broadway_device_manager->touchscreen);
    }

  return devices;
}

static CdkDevice *
cdk_broadway_device_manager_get_client_pointer (CdkDeviceManager *device_manager)
{
  CdkBroadwayDeviceManager *broadway_device_manager = (CdkBroadwayDeviceManager *) device_manager;

  return broadway_device_manager->core_pointer;
}

CdkDeviceManager *
_cdk_broadway_device_manager_new (CdkDisplay *display)
{
  return g_object_new (CDK_TYPE_BROADWAY_DEVICE_MANAGER,
		       "display", display,
		       NULL);
}
