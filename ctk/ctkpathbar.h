/* ctkpathbar.h
 * Copyright (C) 2004  Red Hat, Inc.,  Jonathan Blandford <jrb@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CTK_PATH_BAR_H__
#define __CTK_PATH_BAR_H__

#include "ctkcontainer.h"
#include "ctkfilesystem.h"

G_BEGIN_DECLS

typedef struct _GtkPathBar        GtkPathBar;
typedef struct _GtkPathBarClass   GtkPathBarClass;
typedef struct _GtkPathBarPrivate GtkPathBarPrivate;


#define CTK_TYPE_PATH_BAR                 (ctk_path_bar_get_type ())
#define CTK_PATH_BAR(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_PATH_BAR, GtkPathBar))
#define CTK_PATH_BAR_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_PATH_BAR, GtkPathBarClass))
#define CTK_IS_PATH_BAR(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_PATH_BAR))
#define CTK_IS_PATH_BAR_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_PATH_BAR))
#define CTK_PATH_BAR_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_PATH_BAR, GtkPathBarClass))

struct _GtkPathBar
{
  GtkContainer parent;

  GtkPathBarPrivate *priv;
};

struct _GtkPathBarClass
{
  GtkContainerClass parent_class;

  void (* path_clicked) (GtkPathBar  *path_bar,
			 GFile       *file,
			 GFile       *child_file,
			 gboolean     child_is_hidden);
};

GDK_AVAILABLE_IN_ALL
GType    ctk_path_bar_get_type (void) G_GNUC_CONST;
void     _ctk_path_bar_set_file_system (GtkPathBar         *path_bar,
					GtkFileSystem      *file_system);
void     _ctk_path_bar_set_file        (GtkPathBar         *path_bar,
					GFile              *file,
					gboolean            keep_trail);
void     _ctk_path_bar_up              (GtkPathBar *path_bar);
void     _ctk_path_bar_down            (GtkPathBar *path_bar);

G_END_DECLS

#endif /* __CTK_PATH_BAR_H__ */
