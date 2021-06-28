
/* cdkquartz.h
 *
 * Copyright (C) 2005-2007 Imendio AB
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

#ifndef __GDK_QUARTZ_H__
#define __GDK_QUARTZ_H__

#include <cdk/cdk.h>
#include <cdk/cdkprivate.h>

G_BEGIN_DECLS

typedef enum
{
  GDK_OSX_UNSUPPORTED = 0,
  GDK_OSX_MIN = 4,
  GDK_OSX_TIGER = 4,
  GDK_OSX_LEOPARD = 5,
  GDK_OSX_SNOW_LEOPARD = 6,
  GDK_OSX_LION = 7,
  GDK_OSX_MOUNTAIN_LION = 8,
  GDK_OSX_MAVERICKS = 9,
  GDK_OSX_YOSEMITE = 10,
  GDK_OSX_EL_CAPITAN = 11,
  GDK_OSX_SIERRA = 12,
  GDK_OSX_HIGH_SIERRA = 13,
  GDK_OSX_MOJAVE = 14,
  GDK_OSX_CATALINA = 15,
  GDK_OSX_BIGSUR = 16,
  GDK_OSX_CURRENT = 15,
  GDK_OSX_NEW = 99
} GdkOSXVersion;

GDK_AVAILABLE_IN_ALL
GdkOSXVersion cdk_quartz_osx_version (void);

G_END_DECLS

#define __GDKQUARTZ_H_INSIDE__

#include <cdk/quartz/cdkquartzcursor.h>
#include <cdk/quartz/cdkquartzdevice-core.h>
#include <cdk/quartz/cdkquartzdevicemanager-core.h>
#include <cdk/quartz/cdkquartzdisplay.h>
#include <cdk/quartz/cdkquartzdisplaymanager.h>
#include <cdk/quartz/cdkquartzkeys.h>
#include <cdk/quartz/cdkquartzmonitor.h>
#include <cdk/quartz/cdkquartzscreen.h>
#include <cdk/quartz/cdkquartzutils.h>
#include <cdk/quartz/cdkquartzvisual.h>
#include <cdk/quartz/cdkquartzwindow.h>

#undef __GDKQUARTZ_H_INSIDE__

#endif /* __GDK_QUARTZ_H__ */
