/* GDK - The GIMP Drawing Kit
 * cdkdisplaymanager-win32.c
 *
 * Copyright 2010 Hans Breuer
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

#include "cdkwin32display.h"
#include "cdkwin32displaymanager.h"
#include "cdkprivate-win32.h"

#include "cdkdisplaymanagerprivate.h"
#include "cdkinternals.h"

struct _GdkWin32DisplayManager
{
  GdkDisplayManager parent_instance;
};

struct _GdkWin32DisplayManagerClass
{
  GdkDisplayManagerClass parent_instance;
};

G_DEFINE_TYPE (GdkWin32DisplayManager, cdk_win32_display_manager, GDK_TYPE_DISPLAY_MANAGER)

static void
cdk_win32_display_manager_init (GdkWin32DisplayManager *manager)
{
}

static void
cdk_win32_display_manager_finalize (GObject *object)
{
  g_error ("A GdkWin32DisplayManager object was finalized. This should not happen");
  G_OBJECT_CLASS (cdk_win32_display_manager_parent_class)->finalize (object);
}

static void
cdk_win32_display_manager_class_init (GdkWin32DisplayManagerClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = cdk_win32_display_manager_finalize;

#if 0
  manager_class->atom_intern = _cdk_win32_display_manager_atom_intern;
  manager_class->get_atom_name = _cdk_win32_display_manager_get_atom_name;
#endif
}
