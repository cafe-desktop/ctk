/* CTK - The GIMP Toolkit
 * ctkfilechoosernative.h: Native File selector dialog
 * Copyright (C) 2015, Red Hat, Inc.
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

#ifndef __CTK_FILE_CHOOSER_NATIVE_H__
#define __CTK_FILE_CHOOSER_NATIVE_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkfilechooser.h>
#include <ctk/ctknativedialog.h>

G_BEGIN_DECLS

#define CTK_TYPE_FILE_CHOOSER_NATIVE             (ctk_file_chooser_native_get_type ())

CDK_AVAILABLE_IN_3_20
G_DECLARE_FINAL_TYPE (CtkFileChooserNative, ctk_file_chooser_native, CTK, FILE_CHOOSER_NATIVE, CtkNativeDialog)

CDK_AVAILABLE_IN_3_20
CtkFileChooserNative *ctk_file_chooser_native_new (const gchar          *title,
                                                   CtkWindow            *parent,
                                                   CtkFileChooserAction  action,
                                                   const gchar          *accept_label,
                                                   const gchar          *cancel_label);

CDK_AVAILABLE_IN_3_20
const char *ctk_file_chooser_native_get_accept_label (CtkFileChooserNative *self);
CDK_AVAILABLE_IN_3_20
void        ctk_file_chooser_native_set_accept_label (CtkFileChooserNative *self,
                                                      const char           *accept_label);
CDK_AVAILABLE_IN_3_20
const char *ctk_file_chooser_native_get_cancel_label (CtkFileChooserNative *self);
CDK_AVAILABLE_IN_3_20
void        ctk_file_chooser_native_set_cancel_label (CtkFileChooserNative *self,
                                                      const char           *cancel_label);

G_END_DECLS

#endif /* __CTK_FILE_CHOOSER_NATIVE_H__ */
