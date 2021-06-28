/* CTK - The GIMP Toolkit
 * ctkfilechooser.h: Abstract interface for file selector GUIs
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

#ifndef __CTK_FILE_CHOOSER_H__
#define __CTK_FILE_CHOOSER_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkfilefilter.h>
#include <ctk/ctkwidget.h>

G_BEGIN_DECLS

#define CTK_TYPE_FILE_CHOOSER             (ctk_file_chooser_get_type ())
#define CTK_FILE_CHOOSER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_FILE_CHOOSER, CtkFileChooser))
#define CTK_IS_FILE_CHOOSER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_FILE_CHOOSER))

typedef struct _CtkFileChooser      CtkFileChooser;

/**
 * CtkFileChooserAction:
 * @CTK_FILE_CHOOSER_ACTION_OPEN: Indicates open mode.  The file chooser
 *  will only let the user pick an existing file.
 * @CTK_FILE_CHOOSER_ACTION_SAVE: Indicates save mode.  The file chooser
 *  will let the user pick an existing file, or type in a new
 *  filename.
 * @CTK_FILE_CHOOSER_ACTION_SELECT_FOLDER: Indicates an Open mode for
 *  selecting folders.  The file chooser will let the user pick an
 *  existing folder.
 * @CTK_FILE_CHOOSER_ACTION_CREATE_FOLDER: Indicates a mode for creating a
 *  new folder.  The file chooser will let the user name an existing or
 *  new folder.
 *
 * Describes whether a #CtkFileChooser is being used to open existing files
 * or to save to a possibly new file.
 */
typedef enum
{
  CTK_FILE_CHOOSER_ACTION_OPEN,
  CTK_FILE_CHOOSER_ACTION_SAVE,
  CTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
  CTK_FILE_CHOOSER_ACTION_CREATE_FOLDER
} CtkFileChooserAction;

/**
 * CtkFileChooserConfirmation:
 * @CTK_FILE_CHOOSER_CONFIRMATION_CONFIRM: The file chooser will present
 *  its stock dialog to confirm about overwriting an existing file.
 * @CTK_FILE_CHOOSER_CONFIRMATION_ACCEPT_FILENAME: The file chooser will
 *  terminate and accept the user’s choice of a file name.
 * @CTK_FILE_CHOOSER_CONFIRMATION_SELECT_AGAIN: The file chooser will
 *  continue running, so as to let the user select another file name.
 *
 * Used as a return value of handlers for the
 * #CtkFileChooser::confirm-overwrite signal of a #CtkFileChooser. This
 * value determines whether the file chooser will present the stock
 * confirmation dialog, accept the user’s choice of a filename, or
 * let the user choose another filename.
 *
 * Since: 2.8
 */
typedef enum
{
  CTK_FILE_CHOOSER_CONFIRMATION_CONFIRM,
  CTK_FILE_CHOOSER_CONFIRMATION_ACCEPT_FILENAME,
  CTK_FILE_CHOOSER_CONFIRMATION_SELECT_AGAIN
} CtkFileChooserConfirmation;

CDK_AVAILABLE_IN_ALL
GType ctk_file_chooser_get_type (void) G_GNUC_CONST;

/* GError enumeration for CtkFileChooser */
/**
 * CTK_FILE_CHOOSER_ERROR:
 *
 * Used to get the #GError quark for #CtkFileChooser errors.
 */
#define CTK_FILE_CHOOSER_ERROR (ctk_file_chooser_error_quark ())

/**
 * CtkFileChooserError:
 * @CTK_FILE_CHOOSER_ERROR_NONEXISTENT: Indicates that a file does not exist.
 * @CTK_FILE_CHOOSER_ERROR_BAD_FILENAME: Indicates a malformed filename.
 * @CTK_FILE_CHOOSER_ERROR_ALREADY_EXISTS: Indicates a duplicate path (e.g. when
 *  adding a bookmark).
 * @CTK_FILE_CHOOSER_ERROR_INCOMPLETE_HOSTNAME: Indicates an incomplete hostname (e.g. "http://foo" without a slash after that).
 *
 * These identify the various errors that can occur while calling
 * #CtkFileChooser functions.
 */
typedef enum {
  CTK_FILE_CHOOSER_ERROR_NONEXISTENT,
  CTK_FILE_CHOOSER_ERROR_BAD_FILENAME,
  CTK_FILE_CHOOSER_ERROR_ALREADY_EXISTS,
  CTK_FILE_CHOOSER_ERROR_INCOMPLETE_HOSTNAME
} CtkFileChooserError;

CDK_AVAILABLE_IN_ALL
GQuark ctk_file_chooser_error_quark (void);

/* Configuration
 */
CDK_AVAILABLE_IN_ALL
void                 ctk_file_chooser_set_action          (CtkFileChooser       *chooser,
							   CtkFileChooserAction  action);
CDK_AVAILABLE_IN_ALL
CtkFileChooserAction ctk_file_chooser_get_action          (CtkFileChooser       *chooser);
CDK_AVAILABLE_IN_ALL
void                 ctk_file_chooser_set_local_only      (CtkFileChooser       *chooser,
							   gboolean              local_only);
CDK_AVAILABLE_IN_ALL
gboolean             ctk_file_chooser_get_local_only      (CtkFileChooser       *chooser);
CDK_AVAILABLE_IN_ALL
void                 ctk_file_chooser_set_select_multiple (CtkFileChooser       *chooser,
							   gboolean              select_multiple);
CDK_AVAILABLE_IN_ALL
gboolean             ctk_file_chooser_get_select_multiple (CtkFileChooser       *chooser);
CDK_AVAILABLE_IN_ALL
void                 ctk_file_chooser_set_show_hidden     (CtkFileChooser       *chooser,
							   gboolean              show_hidden);
CDK_AVAILABLE_IN_ALL
gboolean             ctk_file_chooser_get_show_hidden     (CtkFileChooser       *chooser);

CDK_AVAILABLE_IN_ALL
void                 ctk_file_chooser_set_do_overwrite_confirmation (CtkFileChooser *chooser,
								     gboolean        do_overwrite_confirmation);
CDK_AVAILABLE_IN_ALL
gboolean             ctk_file_chooser_get_do_overwrite_confirmation (CtkFileChooser *chooser);

CDK_AVAILABLE_IN_ALL
void                 ctk_file_chooser_set_create_folders  (CtkFileChooser       *chooser,
							  gboolean               create_folders);
CDK_AVAILABLE_IN_ALL
gboolean             ctk_file_chooser_get_create_folders (CtkFileChooser *chooser);

/* Suggested name for the Save-type actions
 */
CDK_AVAILABLE_IN_ALL
void        ctk_file_chooser_set_current_name  (CtkFileChooser *chooser,
					        const gchar    *name);
CDK_AVAILABLE_IN_3_10
gchar *ctk_file_chooser_get_current_name (CtkFileChooser *chooser);

/* Filename manipulation
 */
CDK_AVAILABLE_IN_ALL
gchar *  ctk_file_chooser_get_filename       (CtkFileChooser *chooser);
CDK_AVAILABLE_IN_ALL
gboolean ctk_file_chooser_set_filename       (CtkFileChooser *chooser,
					      const char     *filename);
CDK_AVAILABLE_IN_ALL
gboolean ctk_file_chooser_select_filename    (CtkFileChooser *chooser,
					      const char     *filename);
CDK_AVAILABLE_IN_ALL
void     ctk_file_chooser_unselect_filename  (CtkFileChooser *chooser,
					      const char     *filename);
CDK_AVAILABLE_IN_ALL
void     ctk_file_chooser_select_all         (CtkFileChooser *chooser);
CDK_AVAILABLE_IN_ALL
void     ctk_file_chooser_unselect_all       (CtkFileChooser *chooser);
CDK_AVAILABLE_IN_ALL
GSList * ctk_file_chooser_get_filenames      (CtkFileChooser *chooser);
CDK_AVAILABLE_IN_ALL
gboolean ctk_file_chooser_set_current_folder (CtkFileChooser *chooser,
					      const gchar    *filename);
CDK_AVAILABLE_IN_ALL
gchar *  ctk_file_chooser_get_current_folder (CtkFileChooser *chooser);


/* URI manipulation
 */
CDK_AVAILABLE_IN_ALL
gchar *  ctk_file_chooser_get_uri                (CtkFileChooser *chooser);
CDK_AVAILABLE_IN_ALL
gboolean ctk_file_chooser_set_uri                (CtkFileChooser *chooser,
						  const char     *uri);
CDK_AVAILABLE_IN_ALL
gboolean ctk_file_chooser_select_uri             (CtkFileChooser *chooser,
						  const char     *uri);
CDK_AVAILABLE_IN_ALL
void     ctk_file_chooser_unselect_uri           (CtkFileChooser *chooser,
						  const char     *uri);
CDK_AVAILABLE_IN_ALL
GSList * ctk_file_chooser_get_uris               (CtkFileChooser *chooser);
CDK_AVAILABLE_IN_ALL
gboolean ctk_file_chooser_set_current_folder_uri (CtkFileChooser *chooser,
						  const gchar    *uri);
CDK_AVAILABLE_IN_ALL
gchar *  ctk_file_chooser_get_current_folder_uri (CtkFileChooser *chooser);

/* GFile manipulation */
CDK_AVAILABLE_IN_ALL
GFile *  ctk_file_chooser_get_file                (CtkFileChooser  *chooser);
CDK_AVAILABLE_IN_ALL
gboolean ctk_file_chooser_set_file                (CtkFileChooser  *chooser,
                                                   GFile           *file,
                                                   GError         **error);
CDK_AVAILABLE_IN_ALL
gboolean ctk_file_chooser_select_file             (CtkFileChooser  *chooser,
                                                   GFile           *file,
                                                   GError         **error);
CDK_AVAILABLE_IN_ALL
void     ctk_file_chooser_unselect_file           (CtkFileChooser  *chooser,
                                                   GFile           *file);
CDK_AVAILABLE_IN_ALL
GSList * ctk_file_chooser_get_files               (CtkFileChooser  *chooser);
CDK_AVAILABLE_IN_ALL
gboolean ctk_file_chooser_set_current_folder_file (CtkFileChooser  *chooser,
                                                   GFile           *file,
                                                   GError         **error);
CDK_AVAILABLE_IN_ALL
GFile *  ctk_file_chooser_get_current_folder_file (CtkFileChooser  *chooser);

/* Preview widget
 */
CDK_AVAILABLE_IN_ALL
void       ctk_file_chooser_set_preview_widget        (CtkFileChooser *chooser,
						       CtkWidget      *preview_widget);
CDK_AVAILABLE_IN_ALL
CtkWidget *ctk_file_chooser_get_preview_widget        (CtkFileChooser *chooser);
CDK_AVAILABLE_IN_ALL
void       ctk_file_chooser_set_preview_widget_active (CtkFileChooser *chooser,
						       gboolean        active);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_file_chooser_get_preview_widget_active (CtkFileChooser *chooser);
CDK_AVAILABLE_IN_ALL
void       ctk_file_chooser_set_use_preview_label     (CtkFileChooser *chooser,
						       gboolean        use_label);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_file_chooser_get_use_preview_label     (CtkFileChooser *chooser);

CDK_AVAILABLE_IN_ALL
char  *ctk_file_chooser_get_preview_filename (CtkFileChooser *chooser);
CDK_AVAILABLE_IN_ALL
char  *ctk_file_chooser_get_preview_uri      (CtkFileChooser *chooser);
CDK_AVAILABLE_IN_ALL
GFile *ctk_file_chooser_get_preview_file     (CtkFileChooser *chooser);

/* Extra widget
 */
CDK_AVAILABLE_IN_ALL
void       ctk_file_chooser_set_extra_widget (CtkFileChooser *chooser,
					      CtkWidget      *extra_widget);
CDK_AVAILABLE_IN_ALL
CtkWidget *ctk_file_chooser_get_extra_widget (CtkFileChooser *chooser);

/* List of user selectable filters
 */
CDK_AVAILABLE_IN_ALL
void    ctk_file_chooser_add_filter    (CtkFileChooser *chooser,
					CtkFileFilter  *filter);
CDK_AVAILABLE_IN_ALL
void    ctk_file_chooser_remove_filter (CtkFileChooser *chooser,
					CtkFileFilter  *filter);
CDK_AVAILABLE_IN_ALL
GSList *ctk_file_chooser_list_filters  (CtkFileChooser *chooser);

/* Current filter
 */
CDK_AVAILABLE_IN_ALL
void           ctk_file_chooser_set_filter (CtkFileChooser *chooser,
					   CtkFileFilter  *filter);
CDK_AVAILABLE_IN_ALL
CtkFileFilter *ctk_file_chooser_get_filter (CtkFileChooser *chooser);

/* Per-application shortcut folders */

CDK_AVAILABLE_IN_ALL
gboolean ctk_file_chooser_add_shortcut_folder    (CtkFileChooser *chooser,
						  const char     *folder,
						  GError        **error);
CDK_AVAILABLE_IN_ALL
gboolean ctk_file_chooser_remove_shortcut_folder (CtkFileChooser *chooser,
						  const char     *folder,
						  GError        **error);
CDK_AVAILABLE_IN_ALL
GSList *ctk_file_chooser_list_shortcut_folders   (CtkFileChooser *chooser);

CDK_AVAILABLE_IN_ALL
gboolean ctk_file_chooser_add_shortcut_folder_uri    (CtkFileChooser *chooser,
						      const char     *uri,
						      GError        **error);
CDK_AVAILABLE_IN_ALL
gboolean ctk_file_chooser_remove_shortcut_folder_uri (CtkFileChooser *chooser,
						      const char     *uri,
						      GError        **error);
CDK_AVAILABLE_IN_ALL
GSList *ctk_file_chooser_list_shortcut_folder_uris   (CtkFileChooser *chooser);

CDK_AVAILABLE_IN_3_22
void        ctk_file_chooser_add_choice              (CtkFileChooser  *chooser,
                                                      const char      *id,
                                                      const char      *label,
                                                      const char     **options,
                                                      const char     **option_labels);
CDK_AVAILABLE_IN_3_22
void        ctk_file_chooser_remove_choice           (CtkFileChooser  *chooser,
                                                      const char      *id);
CDK_AVAILABLE_IN_3_22
void        ctk_file_chooser_set_choice              (CtkFileChooser  *chooser,
                                                      const char      *id,
                                                      const char      *option);
CDK_AVAILABLE_IN_3_22
const char *ctk_file_chooser_get_choice              (CtkFileChooser  *chooser,
                                                      const char      *id);

G_END_DECLS

#endif /* __CTK_FILE_CHOOSER_H__ */
