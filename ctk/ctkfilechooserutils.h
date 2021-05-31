/* GTK - The GIMP Toolkit
 * ctkfilechooserutils.h: Private utility functions useful for
 *                        implementing a CtkFileChooser interface
 * Copyright (C) 2003, Red Hat, Inc.
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

#ifndef __CTK_FILE_CHOOSER_UTILS_H__
#define __CTK_FILE_CHOOSER_UTILS_H__

#include "ctkfilechooserprivate.h"

G_BEGIN_DECLS

#define CTK_FILE_CHOOSER_DELEGATE_QUARK	  (_ctk_file_chooser_delegate_get_quark ())

typedef enum {
  CTK_FILE_CHOOSER_PROP_FIRST                  = 0x1000,
  CTK_FILE_CHOOSER_PROP_ACTION                 = CTK_FILE_CHOOSER_PROP_FIRST,
  CTK_FILE_CHOOSER_PROP_FILTER,
  CTK_FILE_CHOOSER_PROP_LOCAL_ONLY,
  CTK_FILE_CHOOSER_PROP_PREVIEW_WIDGET,
  CTK_FILE_CHOOSER_PROP_PREVIEW_WIDGET_ACTIVE,
  CTK_FILE_CHOOSER_PROP_USE_PREVIEW_LABEL,
  CTK_FILE_CHOOSER_PROP_EXTRA_WIDGET,
  CTK_FILE_CHOOSER_PROP_SELECT_MULTIPLE,
  CTK_FILE_CHOOSER_PROP_SHOW_HIDDEN,
  CTK_FILE_CHOOSER_PROP_DO_OVERWRITE_CONFIRMATION,
  CTK_FILE_CHOOSER_PROP_CREATE_FOLDERS,
  CTK_FILE_CHOOSER_PROP_LAST                   = CTK_FILE_CHOOSER_PROP_CREATE_FOLDERS
} CtkFileChooserProp;

void _ctk_file_chooser_install_properties (GObjectClass *klass);

void _ctk_file_chooser_delegate_iface_init (CtkFileChooserIface *iface);
void _ctk_file_chooser_set_delegate        (CtkFileChooser *receiver,
					    CtkFileChooser *delegate);

GQuark _ctk_file_chooser_delegate_get_quark (void) G_GNUC_CONST;

GList *_ctk_file_chooser_extract_recent_folders (GList *infos);

GSettings *_ctk_file_chooser_get_settings_for_widget (CtkWidget *widget);

gchar * _ctk_file_chooser_label_for_file (GFile *file);

G_END_DECLS

#endif /* __CTK_FILE_CHOOSER_UTILS_H__ */
