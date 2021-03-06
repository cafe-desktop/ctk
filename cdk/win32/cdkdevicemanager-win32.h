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

#ifndef __CDK_DEVICE_MANAGER_WIN32_H__
#define __CDK_DEVICE_MANAGER_WIN32_H__

#include <cdk/cdkdevicemanagerprivate.h>

G_BEGIN_DECLS

#define CDK_TYPE_DEVICE_MANAGER_WIN32         (cdk_device_manager_win32_get_type ())
#define CDK_DEVICE_MANAGER_WIN32(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CDK_TYPE_DEVICE_MANAGER_WIN32, CdkDeviceManagerWin32))
#define CDK_DEVICE_MANAGER_WIN32_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c), CDK_TYPE_DEVICE_MANAGER_WIN32, CdkDeviceManagerWin32Class))
#define CDK_IS_DEVICE_MANAGER_WIN32(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CDK_TYPE_DEVICE_MANAGER_WIN32))
#define CDK_IS_DEVICE_MANAGER_WIN32_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c), CDK_TYPE_DEVICE_MANAGER_WIN32))
#define CDK_DEVICE_MANAGER_WIN32_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CDK_TYPE_DEVICE_MANAGER_WIN32, CdkDeviceManagerWin32Class))

typedef struct _CdkDeviceManagerWin32 CdkDeviceManagerWin32;
typedef struct _CdkDeviceManagerWin32Class CdkDeviceManagerWin32Class;

struct _CdkDeviceManagerWin32
{
  CdkDeviceManager parent_object;
  /* Master Devices */
  CdkDevice *core_pointer;
  CdkDevice *core_keyboard;
  /* Fake slave devices */
  CdkDevice *system_pointer;
  CdkDevice *system_keyboard;
  GList *wintab_devices;

  /* Bumped up every time a wintab device enters the proximity
   * of our context (WT_PROXIMITY). Bumped down when we either
   * receive a WT_PACKET, or a WT_CSRCHANGE.
   */
  gint dev_entered_proximity;
};

struct _CdkDeviceManagerWin32Class
{
  CdkDeviceManagerClass parent_class;
};

GType cdk_device_manager_win32_get_type (void) G_GNUC_CONST;

void     _cdk_input_set_tablet_active (void);
gboolean cdk_input_other_event        (CdkDisplay *display,
                                       CdkEvent   *event,
                                       MSG        *msg,
                                       CdkWindow  *window);

G_END_DECLS

#endif /* __CDK_DEVICE_MANAGER_WIN32_H__ */
