/*
 * Copyright © 2016 Red Hat, Inc
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CDK_X11_MONITOR_PRIVATE_H__
#define __CDK_X11_MONITOR_PRIVATE_H__

#include <glib.h>
#include <gio/gio.h>
#include <X11/Xlib.h>

#include "cdkmonitorprivate.h"

#include "cdkx11monitor.h"


struct _CdkX11Monitor
{
  CdkMonitor parent;

  XID output;
  guint add     : 1;
  guint remove  : 1;
};

struct _CdkX11MonitorClass {
  CdkMonitorClass parent_class;
};

#endif
