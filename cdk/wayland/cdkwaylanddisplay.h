/* GDK - The GIMP Drawing Kit
 * Copyright (C) 2013 Jan Arne Petersen
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

#ifndef __GDK_WAYLAND_DISPLAY_H__
#define __GDK_WAYLAND_DISPLAY_H__

#if !defined (__GDKWAYLAND_H_INSIDE__) && !defined (GDK_COMPILATION)
#error "Only <cdk/cdkwayland.h> can be included directly."
#endif

#include <cdk/cdk.h>

#include <wayland-client.h>

G_BEGIN_DECLS

#ifdef GDK_COMPILATION
typedef struct _CdkWaylandDisplay CdkWaylandDisplay;
#else
typedef CdkDisplay CdkWaylandDisplay;
#endif
typedef struct _CdkWaylandDisplayClass CdkWaylandDisplayClass;

#define GDK_TYPE_WAYLAND_DISPLAY              (cdk_wayland_display_get_type())
#define GDK_WAYLAND_DISPLAY(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_WAYLAND_DISPLAY, CdkWaylandDisplay))
#define GDK_WAYLAND_DISPLAY_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_WAYLAND_DISPLAY, CdkWaylandDisplayClass))
#define GDK_IS_WAYLAND_DISPLAY(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_WAYLAND_DISPLAY))
#define GDK_IS_WAYLAND_DISPLAY_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_WAYLAND_DISPLAY))
#define GDK_WAYLAND_DISPLAY_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_WAYLAND_DISPLAY, CdkWaylandDisplayClass))

GDK_AVAILABLE_IN_ALL
GType                   cdk_wayland_display_get_type            (void);

GDK_AVAILABLE_IN_ALL
struct wl_display      *cdk_wayland_display_get_wl_display      (CdkDisplay *display);
GDK_AVAILABLE_IN_ALL
struct wl_compositor   *cdk_wayland_display_get_wl_compositor   (CdkDisplay *display);
GDK_AVAILABLE_IN_3_10
void                    cdk_wayland_display_set_cursor_theme    (CdkDisplay  *display,
                                                                 const gchar *theme,
                                                                 gint         size);
GDK_AVAILABLE_IN_3_22
void                    cdk_wayland_display_set_startup_notification_id (CdkDisplay *display,
                                                                         const char *startup_id);

GDK_AVAILABLE_IN_3_22
gboolean                cdk_wayland_display_prefers_ssd         (CdkDisplay *display);

GDK_AVAILABLE_IN_3_22
gboolean                cdk_wayland_display_query_registry      (CdkDisplay  *display,
								 const gchar *global);

G_END_DECLS

#endif /* __GDK_WAYLAND_DISPLAY_H__ */
