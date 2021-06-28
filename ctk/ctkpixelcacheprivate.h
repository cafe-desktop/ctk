/*
 * Copyright © 2013 Red Hat Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Alexander Larsson <alexl@gnome.org>
 */

#ifndef __CTK_PIXEL_CACHE_PRIVATE_H__
#define __CTK_PIXEL_CACHE_PRIVATE_H__

#include <glib-object.h>
#include <ctkwidget.h>

G_BEGIN_DECLS

typedef struct _CtkPixelCache           CtkPixelCache;

typedef void (*CtkPixelCacheDrawFunc) (cairo_t *cr,
				       gpointer user_data);

CtkPixelCache *_ctk_pixel_cache_new              (void);
void           _ctk_pixel_cache_free             (CtkPixelCache         *cache);
void           _ctk_pixel_cache_map              (CtkPixelCache         *cache);
void           _ctk_pixel_cache_unmap            (CtkPixelCache         *cache);
void           _ctk_pixel_cache_invalidate       (CtkPixelCache         *cache,
                                                  cairo_region_t        *region);
void           _ctk_pixel_cache_draw             (CtkPixelCache         *cache,
                                                  cairo_t               *cr,
                                                  CdkWindow             *window,
                                                  cairo_rectangle_int_t *view_rect,
                                                  cairo_rectangle_int_t *canvas_rect,
                                                  CtkPixelCacheDrawFunc  draw,
                                                  gpointer               user_data);
void           _ctk_pixel_cache_get_extra_size   (CtkPixelCache         *cache,
                                                  guint                 *extra_width,
                                                  guint                 *extra_height);
void           _ctk_pixel_cache_set_extra_size   (CtkPixelCache         *cache,
                                                  guint                  extra_width,
                                                  guint                  extra_height);
void           _ctk_pixel_cache_set_content      (CtkPixelCache         *cache,
                                                  cairo_content_t        content);
gboolean       _ctk_pixel_cache_get_always_cache (CtkPixelCache         *cache);
void           _ctk_pixel_cache_set_always_cache (CtkPixelCache         *cache,
                                                  gboolean               always_cache);
void           ctk_pixel_cache_set_is_opaque     (CtkPixelCache         *cache,
                                                  gboolean               is_opaque);


G_END_DECLS

#endif /* __CTK_PIXEL_CACHE_PRIVATE_H__ */
