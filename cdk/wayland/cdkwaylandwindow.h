/* CDK - The GIMP Drawing Kit
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

#ifndef __CDK_WAYLAND_WINDOW_H__
#define __CDK_WAYLAND_WINDOW_H__

#if !defined (__CDKWAYLAND_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdkwayland.h> can be included directly."
#endif

#include <cdk/cdk.h>

#include <wayland-client.h>

G_BEGIN_DECLS

#ifdef CDK_COMPILATION
typedef struct _CdkWaylandWindow CdkWaylandWindow;
#else
typedef CdkWindow CdkWaylandWindow;
#endif
typedef struct _CdkWaylandWindowClass CdkWaylandWindowClass;

#define CDK_TYPE_WAYLAND_WINDOW              (cdk_wayland_window_get_type())
#define CDK_WAYLAND_WINDOW(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), CDK_TYPE_WAYLAND_WINDOW, CdkWaylandWindow))
#define CDK_WAYLAND_WINDOW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CDK_TYPE_WAYLAND_WINDOW, CdkWaylandWindowClass))
#define CDK_IS_WAYLAND_WINDOW(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), CDK_TYPE_WAYLAND_WINDOW))
#define CDK_IS_WAYLAND_WINDOW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CDK_TYPE_WAYLAND_WINDOW))
#define CDK_WAYLAND_WINDOW_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CDK_TYPE_WAYLAND_WINDOW, CdkWaylandWindowClass))

CDK_AVAILABLE_IN_ALL
GType                    cdk_wayland_window_get_type             (void);

CDK_AVAILABLE_IN_ALL
struct wl_surface       *cdk_wayland_window_get_wl_surface       (CdkWindow *window);

CDK_AVAILABLE_IN_ALL
void                     cdk_wayland_window_set_use_custom_surface (CdkWindow *window);

CDK_AVAILABLE_IN_3_10
void                     cdk_wayland_window_set_dbus_properties_libctk_only (CdkWindow  *window,
									     const char *application_id,
									     const char *app_menu_path,
									     const char *menubar_path,
									     const char *window_object_path,
									     const char *application_object_path,
									     const char *unique_bus_name);

typedef void (*CdkWaylandWindowExported) (CdkWindow  *window,
                                          const char *handle,
                                          gpointer    user_data);

CDK_AVAILABLE_IN_3_22
gboolean                 cdk_wayland_window_export_handle (CdkWindow               *window,
                                                           CdkWaylandWindowExported callback,
                                                           gpointer                 user_data,
                                                           GDestroyNotify           destroy_func);

CDK_AVAILABLE_IN_3_22
void                     cdk_wayland_window_unexport_handle (CdkWindow *window);

CDK_AVAILABLE_IN_3_22
gboolean                 cdk_wayland_window_set_transient_for_exported (CdkWindow *window,
                                                                        char      *parent_handle_str);

CDK_AVAILABLE_IN_3_24
void                     cdk_wayland_window_set_application_id (CdkWindow *window,
                                                                const char *application_id);

CDK_AVAILABLE_IN_3_22
void cdk_wayland_window_announce_csd                        (CdkWindow *window);

CDK_AVAILABLE_IN_3_24
void cdk_wayland_window_announce_ssd                        (CdkWindow *window);

G_END_DECLS

#endif /* __CDK_WAYLAND_WINDOW_H__ */
