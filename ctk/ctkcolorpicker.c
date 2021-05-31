/* GTK - The GIMP Toolkit
 * Copyright (C) 2018, Red Hat, Inc
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

#include "ctkcolorpickerprivate.h"
#include "ctkcolorpickerportalprivate.h"
#include "ctkcolorpickershellprivate.h"
#include "ctkcolorpickerkwinprivate.h"
#include <gio/gio.h>


G_DEFINE_INTERFACE_WITH_CODE (CtkColorPicker, ctk_color_picker, G_TYPE_OBJECT,
                              g_type_interface_add_prerequisite (g_define_type_id, G_TYPE_INITABLE);)

static void
ctk_color_picker_default_init (CtkColorPickerInterface *iface)
{
}

void
ctk_color_picker_pick (CtkColorPicker      *picker,
                       GAsyncReadyCallback  callback,
                       gpointer             user_data)
{
  CTK_COLOR_PICKER_GET_INTERFACE (picker)->pick (picker, callback, user_data);
}

GdkRGBA *
ctk_color_picker_pick_finish (CtkColorPicker  *picker,
                              GAsyncResult    *res,
                              GError         **error)
{
  return CTK_COLOR_PICKER_GET_INTERFACE (picker)->pick_finish (picker, res, error);
}

CtkColorPicker *
ctk_color_picker_new (void)
{
  CtkColorPicker *picker;

  picker = ctk_color_picker_portal_new ();
  if (!picker)
    picker = ctk_color_picker_shell_new ();
  if (!picker)
    picker = ctk_color_picker_kwin_new ();

  if (!picker)
    g_debug ("No suitable CtkColorPicker implementation");
  else
    g_debug ("Using %s for picking colors", G_OBJECT_TYPE_NAME (picker));

  return picker;
}

