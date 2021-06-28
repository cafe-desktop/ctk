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

#ifndef __GDK_DEVICE_MANAGER_BROADWAY_H__
#define __GDK_DEVICE_MANAGER_BROADWAY_H__

#include <cdk/cdkdevicemanagerprivate.h>

G_BEGIN_DECLS

#define GDK_TYPE_BROADWAY_DEVICE_MANAGER         (cdk_broadway_device_manager_get_type ())
#define GDK_BROADWAY_DEVICE_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDK_TYPE_BROADWAY_DEVICE_MANAGER, CdkBroadwayDeviceManager))
#define GDK_BROADWAY_DEVICE_MANAGER_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c), GDK_TYPE_BROADWAY_DEVICE_MANAGER, CdkBroadwayDeviceManagerClass))
#define GDK_IS_BROADWAY_DEVICE_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDK_TYPE_BROADWAY_DEVICE_MANAGER))
#define GDK_IS_BROADWAY_DEVICE_MANAGER_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c), GDK_TYPE_BROADWAY_DEVICE_MANAGER))
#define GDK_BROADWAY_DEVICE_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDK_TYPE_BROADWAY_DEVICE_MANAGER, CdkBroadwayDeviceManagerClass))

typedef struct _CdkBroadwayDeviceManager CdkBroadwayDeviceManager;
typedef struct _CdkBroadwayDeviceManagerClass CdkBroadwayDeviceManagerClass;

struct _CdkBroadwayDeviceManager
{
  CdkDeviceManager parent_object;
  CdkDevice *core_pointer;
  CdkDevice *core_keyboard;
  CdkDevice *touchscreen;
};

struct _CdkBroadwayDeviceManagerClass
{
  CdkDeviceManagerClass parent_class;
};

GType cdk_broadway_device_manager_get_type (void) G_GNUC_CONST;
CdkDeviceManager *_cdk_broadway_device_manager_new (CdkDisplay *display);

G_END_DECLS

#endif /* __GDK_DEVICE_MANAGER_BROADWAY_H__ */
