/* CTK+ - accessibility implementations
 * Copyright 2001 Sun Microsystems Inc.
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

#include "config.h"

#include <string.h>
#include <ctk/ctk.h>
#include "ctkframeaccessible.h"


G_DEFINE_TYPE (CtkFrameAccessible, ctk_frame_accessible, CTK_TYPE_CONTAINER_ACCESSIBLE)

static void
ctk_frame_accessible_initialize (AtkObject *accessible,
                                 gpointer   data)
{
  ATK_OBJECT_CLASS (ctk_frame_accessible_parent_class)->initialize (accessible, data);

  accessible->role = ATK_ROLE_PANEL;
}

static const gchar *
ctk_frame_accessible_get_name (AtkObject *obj)
{
  const gchar *name;
  CtkWidget *widget;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (obj));
  if (widget == NULL)
      return NULL;

  name = ATK_OBJECT_CLASS (ctk_frame_accessible_parent_class)->get_name (obj);
  if (name != NULL)
    return name;

  return ctk_frame_get_label (CTK_FRAME (widget));
}

static void
ctk_frame_accessible_class_init (CtkFrameAccessibleClass *klass)
{
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  class->initialize = ctk_frame_accessible_initialize;
  class->get_name = ctk_frame_accessible_get_name;
}

static void
ctk_frame_accessible_init (CtkFrameAccessible *frame)
{
}
