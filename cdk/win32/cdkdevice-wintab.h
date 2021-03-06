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

#ifndef __CDK_DEVICE_WINTAB_H__
#define __CDK_DEVICE_WINTAB_H__

#include <cdk/cdkdeviceprivate.h>

#include <windows.h>
#include <wintab.h>

G_BEGIN_DECLS

#define CDK_TYPE_DEVICE_WINTAB         (cdk_device_wintab_get_type ())
#define CDK_DEVICE_WINTAB(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CDK_TYPE_DEVICE_WINTAB, CdkDeviceWintab))
#define CDK_DEVICE_WINTAB_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c), CDK_TYPE_DEVICE_WINTAB, CdkDeviceWintabClass))
#define CDK_IS_DEVICE_WINTAB(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CDK_TYPE_DEVICE_WINTAB))
#define CDK_IS_DEVICE_WINTAB_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c), CDK_TYPE_DEVICE_WINTAB))
#define CDK_DEVICE_WINTAB_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CDK_TYPE_DEVICE_WINTAB, CdkDeviceWintabClass))

typedef struct _CdkDeviceWintab CdkDeviceWintab;
typedef struct _CdkDeviceWintabClass CdkDeviceWintabClass;

struct _CdkDeviceWintab
{
  CdkDevice parent_instance;

  gboolean sends_core;
  gint *last_axis_data;
  gint button_state;

  /* WINTAB stuff: */
  HCTX hctx;
  /* Cursor number */
  UINT cursor;
  /* The cursor's CSR_PKTDATA */
  WTPKT pktdata;
  /* Azimuth and altitude axis */
  AXIS orientation_axes[2];
};

struct _CdkDeviceWintabClass
{
  CdkDeviceClass parent_class;
};

GType cdk_device_wintab_get_type (void) G_GNUC_CONST;

void         _cdk_device_wintab_translate_axes (CdkDeviceWintab *device,
                                                CdkWindow       *window,
                                                gdouble         *axes,
                                                gdouble         *x,
                                                gdouble         *y);

G_END_DECLS

#endif /* __CDK_DEVICE_WINTAB_H__ */
