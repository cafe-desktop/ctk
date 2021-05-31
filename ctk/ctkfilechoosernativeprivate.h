/* CTK - The GIMP Toolkit
 * ctkfilechoosernativeprivate.h: Native File selector dialog
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

#ifndef __CTK_FILE_CHOOSER_NATIVE_PRIVATE_H__
#define __CTK_FILE_CHOOSER_NATIVE_PRIVATE_H__

#include <ctk/ctkfilechoosernative.h>
#ifdef GDK_WINDOWING_QUARTZ
#include <AvailabilityMacros.h>
#endif

G_BEGIN_DECLS

typedef struct {
  char *id;
  char *label;
  char **options;
  char **option_labels;
  char *selected;
} CtkFileChooserNativeChoice;

struct _CtkFileChooserNative
{
  CtkNativeDialog parent_instance;

  char *accept_label;
  char *cancel_label;

  int mode;
  GSList *custom_files;

  GFile *current_folder;
  GFile *current_file;
  char *current_name;
  CtkFileFilter *current_filter;
  GSList *choices;

  /* Fallback mode */
  CtkWidget *dialog;
  CtkWidget *accept_button;
  CtkWidget *cancel_button;

  gpointer mode_data;
};

gboolean ctk_file_chooser_native_win32_show (CtkFileChooserNative *self);
void ctk_file_chooser_native_win32_hide (CtkFileChooserNative *self);

#if defined GDK_WINDOWING_QUARTZ && MAC_OS_X_VERSION_MAX_ALLOWED >= 1060
gboolean ctk_file_chooser_native_quartz_show (CtkFileChooserNative *self);
void ctk_file_chooser_native_quartz_hide (CtkFileChooserNative *self);
#endif

gboolean ctk_file_chooser_native_portal_show (CtkFileChooserNative *self);
void ctk_file_chooser_native_portal_hide (CtkFileChooserNative *self);

G_END_DECLS

#endif /* __CTK_FILE_CHOOSER_NATIVE_PRIVATE_H__ */
