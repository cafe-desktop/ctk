/* CDK - The GIMP Drawing Kit
 * cdkdisplaymanager-x11.c
 *
 * Copyright 2010 Red Hat, Inc.
 *
 * Author: Matthias clasen
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

#include "cdkx11displaymanager.h"
#include "cdkdisplaymanagerprivate.h"
#include "cdkdisplay-x11.h"
#include "cdkprivate-x11.h"

#include "cdkinternals.h"

struct _CdkX11DisplayManager
{
  CdkDisplayManager parent;
};

struct _CdkX11DisplayManagerClass
{
  CdkDisplayManagerClass parent_class;
};

G_DEFINE_TYPE (CdkX11DisplayManager, cdk_x11_display_manager, CDK_TYPE_DISPLAY_MANAGER)

static void
cdk_x11_display_manager_init (CdkX11DisplayManager *manager G_GNUC_UNUSED)
{
}

static void
cdk_x11_display_manager_finalize (GObject *object)
{
  g_error ("A CdkX11DisplayManager object was finalized. This should not happen");
  G_OBJECT_CLASS (cdk_x11_display_manager_parent_class)->finalize (object);
}

static void
cdk_x11_display_manager_class_init (CdkX11DisplayManagerClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = cdk_x11_display_manager_finalize;
}
