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

#include "config.h"

#include <glib.h>
#include <gio/gio.h>

#include "cdkmonitor-broadway.h"
#include "cdkscreen-broadway.h"


G_DEFINE_TYPE (CdkBroadwayMonitor, cdk_broadway_monitor, CDK_TYPE_MONITOR)

static void
cdk_broadway_monitor_init (CdkBroadwayMonitor *monitor G_GNUC_UNUSED)
{
}

static void
cdk_broadway_monitor_class_init (CdkBroadwayMonitorClass *class G_GNUC_UNUSED)
{
}
