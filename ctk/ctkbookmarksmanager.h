/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* CTK - The GIMP Toolkit
 * ctkbookmarksmanager.h: Utilities to manage and monitor ~/.ctk-bookmarks
 * Copyright (C) 2003, Red Hat, Inc.
 * Copyright (C) 2007-2008 Carlos Garnacho
 * Copyright (C) 2011 Suse
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Federico Mena Quintero <federico@gnome.org>
 */

#ifndef __CTK_BOOKMARKS_MANAGER_H__
#define __CTK_BOOKMARKS_MANAGER_H__

#include <gio/gio.h>

typedef void (* CtkBookmarksChangedFunc) (gpointer data);

typedef struct
{
  /* This list contains CtkBookmark structs */
  GSList *bookmarks;

  GFileMonitor *bookmarks_monitor;
  gulong bookmarks_monitor_changed_id;

  gpointer changed_func_data;
  CtkBookmarksChangedFunc changed_func;
} CtkBookmarksManager;

typedef struct
{
  GFile *file;
  gchar *label;
} CtkBookmark;

CtkBookmarksManager *_ctk_bookmarks_manager_new (CtkBookmarksChangedFunc changed_func,
						 gpointer                changed_func_data);


void _ctk_bookmarks_manager_free (CtkBookmarksManager *manager);

GSList *_ctk_bookmarks_manager_list_bookmarks (CtkBookmarksManager *manager);

gboolean _ctk_bookmarks_manager_insert_bookmark (CtkBookmarksManager *manager,
						 GFile               *file,
						 gint                 position,
						 GError             **error);

gboolean _ctk_bookmarks_manager_remove_bookmark (CtkBookmarksManager *manager,
						 GFile               *file,
						 GError             **error);

gboolean _ctk_bookmarks_manager_reorder_bookmark (CtkBookmarksManager *manager,
						  GFile               *file,
						  gint                 new_position,
						  GError             **error);

gboolean _ctk_bookmarks_manager_has_bookmark (CtkBookmarksManager *manager,
                                              GFile               *file);

gchar * _ctk_bookmarks_manager_get_bookmark_label (CtkBookmarksManager *manager,
						   GFile               *file);

gboolean _ctk_bookmarks_manager_set_bookmark_label (CtkBookmarksManager *manager,
						    GFile               *file,
						    const gchar         *label,
						    GError             **error);

gboolean _ctk_bookmarks_manager_get_xdg_type (CtkBookmarksManager *manager,
                                              GFile               *file,
                                              GUserDirectory      *directory);
gboolean _ctk_bookmarks_manager_get_is_builtin (CtkBookmarksManager *manager,
                                                GFile               *file);

gboolean _ctk_bookmarks_manager_get_is_xdg_dir_builtin (GUserDirectory xdg_type);

#endif /* __CTK_BOOKMARKS_MANAGER_H__ */
