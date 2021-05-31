/* GTK - The GIMP Toolkit
 * gtkfilesystemmodel.h: GtkTreeModel wrapping a GtkFileSystem
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

#ifndef __CTK_FILE_SYSTEM_MODEL_H__
#define __CTK_FILE_SYSTEM_MODEL_H__

#include <gio/gio.h>
#include <gtk/gtkfilefilter.h>
#include <gtk/gtktreemodel.h>

G_BEGIN_DECLS

#define CTK_TYPE_FILE_SYSTEM_MODEL             (_ctk_file_system_model_get_type ())
#define CTK_FILE_SYSTEM_MODEL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_FILE_SYSTEM_MODEL, GtkFileSystemModel))
#define CTK_IS_FILE_SYSTEM_MODEL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_FILE_SYSTEM_MODEL))

typedef struct _GtkFileSystemModel      GtkFileSystemModel;

GType _ctk_file_system_model_get_type (void) G_GNUC_CONST;

typedef gboolean (*GtkFileSystemModelGetValue)   (GtkFileSystemModel *model,
                                                  GFile              *file,
                                                  GFileInfo          *info,
                                                  int                 column,
                                                  GValue             *value,
                                                  gpointer            user_data);

GtkFileSystemModel *_ctk_file_system_model_new              (GtkFileSystemModelGetValue get_func,
                                                             gpointer            get_data,
                                                             guint               n_columns,
                                                             ...);
GtkFileSystemModel *_ctk_file_system_model_new_for_directory(GFile *             dir,
                                                             const gchar *       attributes,
                                                             GtkFileSystemModelGetValue get_func,
                                                             gpointer            get_data,
                                                             guint               n_columns,
                                                             ...);
GFile *             _ctk_file_system_model_get_directory    (GtkFileSystemModel *model);
GCancellable *      _ctk_file_system_model_get_cancellable  (GtkFileSystemModel *model);
gboolean            _ctk_file_system_model_iter_is_visible  (GtkFileSystemModel *model,
							     GtkTreeIter        *iter);
gboolean            _ctk_file_system_model_iter_is_filtered_out (GtkFileSystemModel *model,
								 GtkTreeIter        *iter);
GFileInfo *         _ctk_file_system_model_get_info         (GtkFileSystemModel *model,
							     GtkTreeIter        *iter);
gboolean            _ctk_file_system_model_get_iter_for_file(GtkFileSystemModel *model,
							     GtkTreeIter        *iter,
							     GFile              *file);
GFile *             _ctk_file_system_model_get_file         (GtkFileSystemModel *model,
							     GtkTreeIter        *iter);
const GValue *      _ctk_file_system_model_get_value        (GtkFileSystemModel *model,
                                                             GtkTreeIter *       iter,
                                                             int                 column);

void                _ctk_file_system_model_add_and_query_file  (GtkFileSystemModel *model,
                                                                GFile              *file,
                                                                const char         *attributes);
void                _ctk_file_system_model_add_and_query_files (GtkFileSystemModel *model,
                                                                GList              *files,
                                                                const char         *attributes);
void                _ctk_file_system_model_update_file      (GtkFileSystemModel *model,
                                                             GFile              *file,
                                                             GFileInfo          *info);
void                _ctk_file_system_model_update_files     (GtkFileSystemModel *model,
                                                             GList              *files,
                                                             GList              *infos);

void                _ctk_file_system_model_set_show_hidden  (GtkFileSystemModel *model,
							     gboolean            show_hidden);
void                _ctk_file_system_model_set_show_folders (GtkFileSystemModel *model,
							     gboolean            show_folders);
void                _ctk_file_system_model_set_show_files   (GtkFileSystemModel *model,
							     gboolean            show_files);
void                _ctk_file_system_model_set_filter_folders (GtkFileSystemModel *model,
							     gboolean            show_folders);
void                _ctk_file_system_model_clear_cache      (GtkFileSystemModel *model,
                                                             int                 column);

void                _ctk_file_system_model_set_filter       (GtkFileSystemModel *model,
                                                             GtkFileFilter      *filter);

G_END_DECLS

#endif /* __CTK_FILE_SYSTEM_MODEL_H__ */