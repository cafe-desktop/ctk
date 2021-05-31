/* GTK - The GIMP Toolkit
 * ctkfilesystem.h: Filesystem abstraction functions.
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

#ifndef __CTK_FILE_SYSTEM_H__
#define __CTK_FILE_SYSTEM_H__

#include <gio/gio.h>
#include <ctk/ctkwidget.h>	/* For icon handling */

G_BEGIN_DECLS

#define CTK_TYPE_FILE_SYSTEM         (_ctk_file_system_get_type ())
#define CTK_FILE_SYSTEM(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_FILE_SYSTEM, CtkFileSystem))
#define CTK_FILE_SYSTEM_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST    ((c), CTK_TYPE_FILE_SYSTEM, CtkFileSystemClass))
#define CTK_IS_FILE_SYSTEM(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_FILE_SYSTEM))
#define CTK_IS_FILE_SYSTEM_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE    ((c), CTK_TYPE_FILE_SYSTEM))
#define CTK_FILE_SYSTEM_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS  ((o), CTK_TYPE_FILE_SYSTEM, CtkFileSystemClass))

typedef struct CtkFileSystem          CtkFileSystem;
typedef struct CtkFileSystemPrivate      CtkFileSystemPrivate;
typedef struct CtkFileSystemClass     CtkFileSystemClass;


typedef struct CtkFileSystemVolume CtkFileSystemVolume; /* opaque struct */


struct CtkFileSystem
{
  GObject parent_object;

  CtkFileSystemPrivate *priv;
};

struct CtkFileSystemClass
{
  GObjectClass parent_class;

  void (*volumes_changed)   (CtkFileSystem *file_system);
};


typedef void (* CtkFileSystemGetInfoCallback)      (GCancellable        *cancellable,
						    GFileInfo           *file_info,
						    const GError        *error,
						    gpointer             data);
typedef void (* CtkFileSystemVolumeMountCallback)  (GCancellable        *cancellable,
						    CtkFileSystemVolume *volume,
						    const GError        *error,
						    gpointer             data);

/* CtkFileSystem methods */
GType           _ctk_file_system_get_type     (void) G_GNUC_CONST;

CtkFileSystem * _ctk_file_system_new          (void);

GSList *        _ctk_file_system_list_volumes   (CtkFileSystem *file_system);

GCancellable *  _ctk_file_system_get_info               (CtkFileSystem                     *file_system,
							 GFile                             *file,
							 const gchar                       *attributes,
							 CtkFileSystemGetInfoCallback       callback,
							 gpointer                           data);
GCancellable *  _ctk_file_system_mount_volume           (CtkFileSystem                     *file_system,
							 CtkFileSystemVolume               *volume,
							 GMountOperation                   *mount_operation,
							 CtkFileSystemVolumeMountCallback   callback,
							 gpointer                           data);
GCancellable *  _ctk_file_system_mount_enclosing_volume (CtkFileSystem                     *file_system,
							 GFile                             *file,
							 GMountOperation                   *mount_operation,
							 CtkFileSystemVolumeMountCallback   callback,
							 gpointer                           data);

CtkFileSystemVolume * _ctk_file_system_get_volume_for_file (CtkFileSystem       *file_system,
							    GFile               *file);

/* CtkFileSystemVolume methods */
gchar *               _ctk_file_system_volume_get_display_name (CtkFileSystemVolume *volume);
gboolean              _ctk_file_system_volume_is_mounted       (CtkFileSystemVolume *volume);
GFile *               _ctk_file_system_volume_get_root         (CtkFileSystemVolume *volume);
GIcon *               _ctk_file_system_volume_get_symbolic_icon (CtkFileSystemVolume *volume);
cairo_surface_t *     _ctk_file_system_volume_render_icon      (CtkFileSystemVolume  *volume,
							        CtkWidget            *widget,
							        gint                  icon_size,
							        GError              **error);

CtkFileSystemVolume  *_ctk_file_system_volume_ref              (CtkFileSystemVolume *volume);
void                  _ctk_file_system_volume_unref            (CtkFileSystemVolume *volume);

/* GFileInfo helper functions */
cairo_surface_t *     _ctk_file_info_render_icon (GFileInfo *info,
						  CtkWidget *widget,
						  gint       icon_size);

gboolean	_ctk_file_info_consider_as_directory (GFileInfo *info);

/* GFile helper functions */
gboolean	_ctk_file_has_native_path (GFile *file);

gboolean        _ctk_file_consider_as_remote (GFile *file);

G_END_DECLS

#endif /* __CTK_FILE_SYSTEM_H__ */
