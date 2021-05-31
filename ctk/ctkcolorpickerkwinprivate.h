/*
 * GTK - The GIMP Toolkit
 * Copyright (C) 2018 Red Hat, Inc.
 * All rights reserved.
 *
 * This Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CTK_COLOR_PICKER_KWIN_H__
#define __CTK_COLOR_PICKER_KWIN_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkcolorpickerprivate.h>

G_BEGIN_DECLS


#define CTK_TYPE_COLOR_PICKER_KWIN ctk_color_picker_kwin_get_type ()
G_DECLARE_FINAL_TYPE (CtkColorPickerKwin, ctk_color_picker_kwin, GTK, COLOR_PICKER_KWIN, GObject)

GDK_AVAILABLE_IN_ALL
CtkColorPicker * ctk_color_picker_kwin_new (void);

G_END_DECLS

#endif  /* __CTK_COLOR_PICKER_KWIN_H__ */
