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
#define CTK_FILE_SYSTEM(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_FILE_SYSTEM, GtkFileSystem))
#define CTK_FILE_SYSTEM_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST    ((c), CTK_TYPE_FILE_SYSTEM, GtkFileSystemClass))
#define CTK_IS_FILE_SYSTEM(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_FILE_SYSTEM))
#define CTK_IS_FILE_SYSTEM_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE    ((c), CTK_TYPE_FILE_SYSTEM))
#define CTK_FILE_SYSTEM_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS  ((o), CTK_TYPE_FILE_SYSTEM, GtkFileSystemClass))

typedef struct GtkFileSystem          GtkFileSystem;
typedef struct GtkFileSystemPrivate      GtkFileSystemPrivate;
typedef struct GtkFileSystemClass     GtkFileSystemClass;


typedef struct GtkFileSystemVolume GtkFileSystemVolume; /* opaque struct */


struct GtkFileSystem
{
  GObject parent_object;

  GtkFileSystemPrivate *priv;
};

struct GtkFileSystemClass
{
  GObjectClass parent_class;

  void (*volumes_changed)   (GtkFileSystem *file_system);
};


typedef void (* GtkFileSystemGetInfoCallback)      (GCancellable        *cancellable,
						    GFileInfo           *file_info,
						    const GError        *error,
						    gpointer             data);
typedef void (* GtkFileSystemVolumeMountCallback)  (GCancellable        *cancellable,
						    GtkFileSystemVolume *volume,
						    const GError        *error,
						    gpointer             data);

/* GtkFileSystem methods */
GType           _ctk_file_system_get_type     (void) G_GNUC_CONST;

GtkFileSystem * _ctk_file_system_new          (void);

GSList *        _ctk_file_system_list_volumes   (GtkFileSystem *file_system);

GCancellable *  _ctk_file_system_get_info               (GtkFileSystem                     *file_system,
							 GFile                             *file,
							 const gchar                       *attributes,
							 GtkFileSystemGetInfoCallback       callback,
							 gpointer                           data);
GCancellable *  _ctk_file_system_mount_volume           (GtkFileSystem                     *file_system,
							 GtkFileSystemVolume               *volume,
							 GMountOperation                   *mount_operation,
							 GtkFileSystemVolumeMountCallback   callback,
							 gpointer                           data);
GCancellable *  _ctk_file_system_mount_enclosing_volume (GtkFileSystem                     *file_system,
							 GFile                             *file,
							 GMountOperation                   *mount_operation,
							 GtkFileSystemVolumeMountCallback   callback,
							 gpointer                           data);

GtkFileSystemVolume * _ctk_file_system_get_volume_for_file (GtkFileSystem       *file_system,
							    GFile               *file);

/* GtkFileSystemVolume methods */
gchar *               _ctk_file_system_volume_get_display_name (GtkFileSystemVolume *volume);
gboolean              _ctk_file_system_volume_is_mounted       (GtkFileSystemVolume *volume);
GFile *               _ctk_file_system_volume_get_root         (GtkFileSystemVolume *volume);
GIcon *               _ctk_file_system_volume_get_symbolic_icon (GtkFileSystemVolume *volume);
cairo_surface_t *     _ctk_file_system_volume_render_icon      (GtkFileSystemVolume  *volume,
							        GtkWidget            *widget,
							        gint                  icon_size,
							        GError              **error);

GtkFileSystemVolume  *_ctk_file_system_volume_ref              (GtkFileSystemVolume *volume);
void                  _ctk_file_system_volume_unref            (GtkFileSystemVolume *volume);

/* GFileInfo helper functions */
cairo_surface_t *     _ctk_file_info_render_icon (GFileInfo *info,
						  GtkWidget *widget,
						  gint       icon_size);

gboolean	_ctk_file_info_consider_as_directory (GFileInfo *info);

/* GFile helper functions */
gboolean	_ctk_file_has_native_path (GFile *file);

gboolean        _ctk_file_consider_as_remote (GFile *file);

G_END_DECLS

#endif /* __CTK_FILE_SYSTEM_H__ */
