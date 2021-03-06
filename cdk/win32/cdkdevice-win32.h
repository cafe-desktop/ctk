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

#ifndef __CDK_DEVICE_WIN32_H__
#define __CDK_DEVICE_WIN32_H__

#include <cdk/cdkdeviceprivate.h>

G_BEGIN_DECLS

#define CDK_TYPE_DEVICE_WIN32         (cdk_device_win32_get_type ())
#define CDK_DEVICE_WIN32(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CDK_TYPE_DEVICE_WIN32, CdkDeviceWin32))
#define CDK_DEVICE_WIN32_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c), CDK_TYPE_DEVICE_WIN32, CdkDeviceWin32Class))
#define CDK_IS_DEVICE_WIN32(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CDK_TYPE_DEVICE_WIN32))
#define CDK_IS_DEVICE_WIN32_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c), CDK_TYPE_DEVICE_WIN32))
#define CDK_DEVICE_WIN32_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CDK_TYPE_DEVICE_WIN32, CdkDeviceWin32Class))

typedef struct _CdkDeviceWin32 CdkDeviceWin32;
typedef struct _CdkDeviceWin32Class CdkDeviceWin32Class;

struct _CdkDeviceWin32
{
  CdkDevice parent_instance;
};

struct _CdkDeviceWin32Class
{
  CdkDeviceClass parent_class;
};

GType cdk_device_win32_get_type (void) G_GNUC_CONST;

CdkWindow *_cdk_device_win32_window_at_position (CdkDevice       *device,
                                                 gdouble         *win_x,
                                                 gdouble         *win_y,
                                                 CdkModifierType *mask,
                                                 gboolean         get_toplevel);

G_END_DECLS

#endif /* __CDK_DEVICE_WIN32_H__ */
