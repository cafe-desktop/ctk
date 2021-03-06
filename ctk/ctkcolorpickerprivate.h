/*
 * CTK - The GIMP Toolkit
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

#ifndef __CTK_COLOR_PICKER_H__
#define __CTK_COLOR_PICKER_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkwindow.h>

G_BEGIN_DECLS


#define CTK_TYPE_COLOR_PICKER             (ctk_color_picker_get_type ())
#define CTK_COLOR_PICKER(o)               (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_COLOR_PICKER, CtkColorPicker))
#define CTK_IS_COLOR_PICKER(o)            (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_COLOR_PICKER))
#define CTK_COLOR_PICKER_GET_INTERFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE ((o), CTK_TYPE_COLOR_PICKER, CtkColorPickerInterface))


typedef struct _CtkColorPicker            CtkColorPicker;
typedef struct _CtkColorPickerInterface   CtkColorPickerInterface;

struct _CtkColorPickerInterface {
  GTypeInterface g_iface;

  void (* pick)             (CtkColorPicker      *picker,
                             GAsyncReadyCallback  callback,
                             gpointer             user_data);

  CdkRGBA * (* pick_finish) (CtkColorPicker      *picker,
                             GAsyncResult        *res,
                             GError             **error);
};

CDK_AVAILABLE_IN_ALL
GType            ctk_color_picker_get_type    (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkColorPicker * ctk_color_picker_new         (void);
CDK_AVAILABLE_IN_ALL
void             ctk_color_picker_pick        (CtkColorPicker       *picker,
                                               GAsyncReadyCallback   callback,
                                               gpointer              user_data);
CDK_AVAILABLE_IN_ALL
CdkRGBA *        ctk_color_picker_pick_finish (CtkColorPicker       *picker,
                                               GAsyncResult         *res,
                                               GError              **error);

G_END_DECLS

#endif  /* __CTK_COLOR_PICKER_H__ */
