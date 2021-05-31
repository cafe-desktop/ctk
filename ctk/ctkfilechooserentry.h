/* CTK - The GIMP Toolkit
 * ctkfilechooserentry.h: Entry with filename completion
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

#ifndef __CTK_FILE_CHOOSER_ENTRY_H__
#define __CTK_FILE_CHOOSER_ENTRY_H__

#include "ctkfilesystem.h"
#include "ctkfilechooser.h"

G_BEGIN_DECLS

#define CTK_TYPE_FILE_CHOOSER_ENTRY    (_ctk_file_chooser_entry_get_type ())
#define CTK_FILE_CHOOSER_ENTRY(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_FILE_CHOOSER_ENTRY, CtkFileChooserEntry))
#define CTK_IS_FILE_CHOOSER_ENTRY(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_FILE_CHOOSER_ENTRY))

typedef struct _CtkFileChooserEntry      CtkFileChooserEntry;

GType              _ctk_file_chooser_entry_get_type           (void) G_GNUC_CONST;
CtkWidget *        _ctk_file_chooser_entry_new                (gboolean             eat_tab,
                                                               gboolean             eat_escape);
void               _ctk_file_chooser_entry_set_action         (CtkFileChooserEntry *chooser_entry,
							       CtkFileChooserAction action);
CtkFileChooserAction _ctk_file_chooser_entry_get_action       (CtkFileChooserEntry *chooser_entry);
void               _ctk_file_chooser_entry_set_base_folder    (CtkFileChooserEntry *chooser_entry,
							       GFile               *folder);
GFile *            _ctk_file_chooser_entry_get_current_folder (CtkFileChooserEntry *chooser_entry);
const gchar *      _ctk_file_chooser_entry_get_file_part      (CtkFileChooserEntry *chooser_entry);
gboolean           _ctk_file_chooser_entry_get_is_folder      (CtkFileChooserEntry *chooser_entry,
							       GFile               *file);
void               _ctk_file_chooser_entry_select_filename    (CtkFileChooserEntry *chooser_entry);
void               _ctk_file_chooser_entry_set_local_only     (CtkFileChooserEntry *chooser_entry,
                                                               gboolean             local_only);
gboolean           _ctk_file_chooser_entry_get_local_only     (CtkFileChooserEntry *chooser_entry);
void               _ctk_file_chooser_entry_set_file_filter    (CtkFileChooserEntry *chooser_entry,
                                                               CtkFileFilter       *filter);

G_END_DECLS

#endif /* __CTK_FILE_CHOOSER_ENTRY_H__ */
