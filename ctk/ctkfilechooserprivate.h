/* GTK - The GIMP Toolkit
 * ctkfilechooserprivate.h: Interface definition for file selector GUIs
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

#ifndef __CTK_FILE_CHOOSER_PRIVATE_H__
#define __CTK_FILE_CHOOSER_PRIVATE_H__

#include "ctkbookmarksmanager.h"
#include "ctkfilechooser.h"
#include "ctkfilesystem.h"
#include "ctkfilesystemmodel.h"
#include "ctkliststore.h"
#include "ctkrecentmanager.h"
#include "ctksearchengine.h"
#include "ctkquery.h"
#include "ctksizegroup.h"
#include "ctktreemodelsort.h"
#include "ctktreestore.h"
#include "ctktreeview.h"
#include "ctkbox.h"

G_BEGIN_DECLS

#define SETTINGS_KEY_LOCATION_MODE          "location-mode"
#define SETTINGS_KEY_SHOW_HIDDEN            "show-hidden"
#define SETTINGS_KEY_SHOW_SIZE_COLUMN       "show-size-column"
#define SETTINGS_KEY_SHOW_TYPE_COLUMN       "show-type-column"
#define SETTINGS_KEY_SORT_COLUMN            "sort-column"
#define SETTINGS_KEY_SORT_ORDER             "sort-order"
#define SETTINGS_KEY_WINDOW_POSITION        "window-position"
#define SETTINGS_KEY_WINDOW_SIZE            "window-size"
#define SETTINGS_KEY_SIDEBAR_WIDTH          "sidebar-width"
#define SETTINGS_KEY_STARTUP_MODE           "startup-mode"
#define SETTINGS_KEY_SORT_DIRECTORIES_FIRST "sort-directories-first"
#define SETTINGS_KEY_CLOCK_FORMAT           "clock-format"
#define SETTINGS_KEY_DATE_FORMAT            "date-format"
#define SETTINGS_KEY_TYPE_FORMAT            "type-format"

#define CTK_FILE_CHOOSER_GET_IFACE(inst)  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), CTK_TYPE_FILE_CHOOSER, CtkFileChooserIface))

typedef struct _CtkFileChooserIface CtkFileChooserIface;

struct _CtkFileChooserIface
{
  GTypeInterface base_iface;

  /* Methods
   */
  gboolean       (*set_current_folder) 	   (CtkFileChooser    *chooser,
					    GFile             *file,
					    GError           **error);
  GFile *        (*get_current_folder) 	   (CtkFileChooser    *chooser);
  void           (*set_current_name)   	   (CtkFileChooser    *chooser,
					    const gchar       *name);
  gchar *        (*get_current_name)       (CtkFileChooser    *chooser);
  gboolean       (*select_file)        	   (CtkFileChooser    *chooser,
					    GFile             *file,
					    GError           **error);
  void           (*unselect_file)      	   (CtkFileChooser    *chooser,
					    GFile             *file);
  void           (*select_all)         	   (CtkFileChooser    *chooser);
  void           (*unselect_all)       	   (CtkFileChooser    *chooser);
  GSList *       (*get_files)          	   (CtkFileChooser    *chooser);
  GFile *        (*get_preview_file)   	   (CtkFileChooser    *chooser);
  CtkFileSystem *(*get_file_system)    	   (CtkFileChooser    *chooser);
  void           (*add_filter)         	   (CtkFileChooser    *chooser,
					    CtkFileFilter     *filter);
  void           (*remove_filter)      	   (CtkFileChooser    *chooser,
					    CtkFileFilter     *filter);
  GSList *       (*list_filters)       	   (CtkFileChooser    *chooser);
  gboolean       (*add_shortcut_folder)    (CtkFileChooser    *chooser,
					    GFile             *file,
					    GError           **error);
  gboolean       (*remove_shortcut_folder) (CtkFileChooser    *chooser,
					    GFile             *file,
					    GError           **error);
  GSList *       (*list_shortcut_folders)  (CtkFileChooser    *chooser);

  /* Signals
   */
  void (*current_folder_changed) (CtkFileChooser *chooser);
  void (*selection_changed)      (CtkFileChooser *chooser);
  void (*update_preview)         (CtkFileChooser *chooser);
  void (*file_activated)         (CtkFileChooser *chooser);
  CtkFileChooserConfirmation (*confirm_overwrite) (CtkFileChooser *chooser);

  /* 3.22 additions */
  void           (*add_choice)    (CtkFileChooser *chooser,
                                   const char      *id,
                                   const char      *label,
                                   const char     **options,
                                   const char     **option_labels);
  void           (*remove_choice) (CtkFileChooser  *chooser,
                                   const char      *id);
  void           (*set_choice)    (CtkFileChooser  *chooser,
                                   const char      *id,
                                   const char      *option);
  const char *   (*get_choice)    (CtkFileChooser  *chooser,
                                   const char      *id);
};

CtkFileSystem *_ctk_file_chooser_get_file_system         (CtkFileChooser    *chooser);
gboolean       _ctk_file_chooser_add_shortcut_folder     (CtkFileChooser    *chooser,
							  GFile             *folder,
							  GError           **error);
gboolean       _ctk_file_chooser_remove_shortcut_folder  (CtkFileChooser    *chooser,
							  GFile             *folder,
							  GError           **error);
GSList *       _ctk_file_chooser_list_shortcut_folder_files (CtkFileChooser *chooser);


G_END_DECLS

#endif /* __CTK_FILE_CHOOSER_PRIVATE_H__ */
