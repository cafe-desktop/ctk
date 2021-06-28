/* ctkiconcache.h
 * Copyright (C) 2004  Anders Carlsson <andersca@gnome.org>
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
#ifndef __CTK_ICON_CACHE_H__
#define __CTK_ICON_CACHE_H__

#include <cdk-pixbuf/cdk-pixbuf.h>
#include <cdk/cdk.h>

typedef struct _CtkIconCache CtkIconCache;

CtkIconCache *_ctk_icon_cache_new            (const gchar  *data);
CtkIconCache *_ctk_icon_cache_new_for_path   (const gchar  *path);
gint          _ctk_icon_cache_get_directory_index  (CtkIconCache *cache,
					            const gchar  *directory);
gboolean      _ctk_icon_cache_has_icon       (CtkIconCache *cache,
					      const gchar  *icon_name);
gboolean      _ctk_icon_cache_has_icon_in_directory (CtkIconCache *cache,
					             const gchar  *icon_name,
					             const gchar  *directory);
gboolean      _ctk_icon_cache_has_icons      (CtkIconCache *cache,
                                              const gchar  *directory);
void	      _ctk_icon_cache_add_icons      (CtkIconCache *cache,
					      const gchar  *directory,
					      GHashTable   *hash_table);

gint          _ctk_icon_cache_get_icon_flags (CtkIconCache *cache,
					      const gchar  *icon_name,
					      gint          directory_index);
CdkPixbuf    *_ctk_icon_cache_get_icon       (CtkIconCache *cache,
					      const gchar  *icon_name,
					      gint          directory_index);

CtkIconCache *_ctk_icon_cache_ref            (CtkIconCache *cache);
void          _ctk_icon_cache_unref          (CtkIconCache *cache);


#endif /* __CTK_ICON_CACHE_H__ */
