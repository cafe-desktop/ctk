/* CTK+ - accessibility implementations
 * Copyright 2004 Sun Microsystems Inc.
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

#include <ctk/ctk.h>
#include "ctkscaleaccessible.h"

G_DEFINE_TYPE (CtkScaleAccessible, ctk_scale_accessible, CTK_TYPE_RANGE_ACCESSIBLE)

static const gchar *
ctk_scale_accessible_get_description (AtkObject *object)
{
  CtkWidget *widget;
  PangoLayout *layout;

  widget = ctk_accessible_get_widget (CTK_ACCESSIBLE (object));
  if (widget == NULL)
    return NULL;

  layout = ctk_scale_get_layout (CTK_SCALE (widget));
  if (layout)
    return pango_layout_get_text (layout);

  return ATK_OBJECT_CLASS (ctk_scale_accessible_parent_class)->get_description (object);
}

static void
ctk_scale_accessible_class_init (CtkScaleAccessibleClass *klass)
{
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  class->get_description = ctk_scale_accessible_get_description;
}

static void
ctk_scale_accessible_init (CtkScaleAccessible *scale)
{
}
