/* CDK - The GIMP Drawing Kit
 * cdkdisplaymanager-quartz.c
 *
 * Copyright (C) 2005 Imendio AB
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

#include <ApplicationServices/ApplicationServices.h>

#include "cdkquartzdisplay.h"
#include "cdkquartzdisplaymanager.h"
#include "cdkprivate-quartz.h"

#include "cdkdisplaymanagerprivate.h"
#include "cdkinternals.h"

struct _CdkQuartzDisplayManager
{
  CdkDisplayManager parent;
};


G_DEFINE_TYPE (CdkQuartzDisplayManager, cdk_quartz_display_manager, CDK_TYPE_DISPLAY_MANAGER)

static void
cdk_quartz_display_manager_init (CdkQuartzDisplayManager *manager)
{
}

static void
cdk_quartz_display_manager_finalize (GObject *object)
{
  g_error ("A CdkQuartzDisplayManager object was finalized. This should not happen");
  G_OBJECT_CLASS (cdk_quartz_display_manager_parent_class)->finalize (object);
}

static void
cdk_quartz_display_manager_class_init (CdkQuartzDisplayManagerClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = cdk_quartz_display_manager_finalize;
}
