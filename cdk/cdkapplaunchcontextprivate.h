/* CDK - The GIMP Drawing Kit
 * Copyright (C) 2010 Red Hat, Inc.
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

#ifndef __CDK_APP_LAUNCH_CONTEXT_PRIVATE_H__
#define __CDK_APP_LAUNCH_CONTEXT_PRIVATE_H__

#include <gio/gio.h>
#include "cdkapplaunchcontext.h"
#include "cdktypes.h"

G_BEGIN_DECLS

#define CDK_APP_LAUNCH_CONTEXT_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), CDK_TYPE_APP_LAUNCH_CONTEXT, CdkAppLaunchContextClass))
#define CDK_IS_APP_LAUNCH_CONTEXT_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), CDK_TYPE_APP_LAUNCH_CONTEXT))
#define CDK_APP_LAUNCH_CONTEXT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CDK_TYPE_APP_LAUNCH_CONTEXT, CdkAppLaunchContextClass))


typedef GAppLaunchContextClass CdkAppLaunchContextClass;

struct _CdkAppLaunchContext
{
  GAppLaunchContext parent_instance;

  CdkDisplay *display;
  CdkScreen *screen;
  gint workspace;
  guint32 timestamp;
  GIcon *icon;
  char *icon_name;
};

G_END_DECLS

#endif
