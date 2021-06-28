/*
 * cdkdisplay.h
 *
 * Copyright 2001 Sun Microsystems Inc.
 *
 * Erwann Chenede <erwann.chenede@sun.com>
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

#ifndef __GDK_DISPLAY_H__
#define __GDK_DISPLAY_H__

#if !defined (__GDK_H_INSIDE__) && !defined (GDK_COMPILATION)
#error "Only <cdk/cdk.h> can be included directly."
#endif

#include <cdk/cdkversionmacros.h>
#include <cdk/cdktypes.h>
#include <cdk/cdkevents.h>
#include <cdk/cdkdevicemanager.h>
#include <cdk/cdkseat.h>
#include <cdk/cdkmonitor.h>

G_BEGIN_DECLS

#define GDK_TYPE_DISPLAY              (cdk_display_get_type ())
#define GDK_DISPLAY(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_DISPLAY, CdkDisplay))
#define GDK_IS_DISPLAY(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_DISPLAY))
#ifndef GDK_DISABLE_DEPRECATED
#define GDK_DISPLAY_OBJECT(object)    GDK_DISPLAY(object)
#endif

GDK_AVAILABLE_IN_ALL
GType       cdk_display_get_type (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
CdkDisplay *cdk_display_open                (const gchar *display_name);

GDK_AVAILABLE_IN_ALL
const gchar * cdk_display_get_name         (CdkDisplay *display);

GDK_DEPRECATED_IN_3_10
gint        cdk_display_get_n_screens      (CdkDisplay  *display);
GDK_DEPRECATED_IN_3_20
CdkScreen * cdk_display_get_screen         (CdkDisplay  *display,
                                            gint         screen_num);
GDK_AVAILABLE_IN_ALL
CdkScreen * cdk_display_get_default_screen (CdkDisplay  *display);

#ifndef GDK_MULTIDEVICE_SAFE
GDK_DEPRECATED_IN_3_0_FOR(cdk_device_ungrab)
void        cdk_display_pointer_ungrab     (CdkDisplay  *display,
                                            guint32      time_);
GDK_DEPRECATED_IN_3_0_FOR(cdk_device_ungrab)
void        cdk_display_keyboard_ungrab    (CdkDisplay  *display,
                                            guint32      time_);
GDK_DEPRECATED_IN_3_0_FOR(cdk_display_device_is_grabbed)
gboolean    cdk_display_pointer_is_grabbed (CdkDisplay  *display);
#endif /* GDK_MULTIDEVICE_SAFE */

GDK_AVAILABLE_IN_ALL
gboolean    cdk_display_device_is_grabbed  (CdkDisplay  *display,
                                            CdkDevice   *device);
GDK_AVAILABLE_IN_ALL
void        cdk_display_beep               (CdkDisplay  *display);
GDK_AVAILABLE_IN_ALL
void        cdk_display_sync               (CdkDisplay  *display);
GDK_AVAILABLE_IN_ALL
void        cdk_display_flush              (CdkDisplay  *display);

GDK_AVAILABLE_IN_ALL
void        cdk_display_close                  (CdkDisplay  *display);
GDK_AVAILABLE_IN_ALL
gboolean    cdk_display_is_closed          (CdkDisplay  *display);

GDK_DEPRECATED_IN_3_0_FOR(cdk_device_manager_list_devices)
GList *     cdk_display_list_devices       (CdkDisplay  *display);

GDK_AVAILABLE_IN_ALL
CdkEvent* cdk_display_get_event  (CdkDisplay     *display);
GDK_AVAILABLE_IN_ALL
CdkEvent* cdk_display_peek_event (CdkDisplay     *display);
GDK_AVAILABLE_IN_ALL
void      cdk_display_put_event  (CdkDisplay     *display,
                                  const CdkEvent *event);
GDK_AVAILABLE_IN_ALL
gboolean  cdk_display_has_pending (CdkDisplay  *display);

GDK_AVAILABLE_IN_ALL
void cdk_display_set_double_click_time     (CdkDisplay   *display,
                                            guint         msec);
GDK_AVAILABLE_IN_ALL
void cdk_display_set_double_click_distance (CdkDisplay   *display,
                                            guint         distance);

GDK_AVAILABLE_IN_ALL
CdkDisplay *cdk_display_get_default (void);

#ifndef GDK_MULTIDEVICE_SAFE
GDK_DEPRECATED_IN_3_0_FOR(cdk_device_get_position)
void             cdk_display_get_pointer           (CdkDisplay             *display,
                                                    CdkScreen             **screen,
                                                    gint                   *x,
                                                    gint                   *y,
                                                    CdkModifierType        *mask);
GDK_DEPRECATED_IN_3_0_FOR(cdk_device_get_window_at_position)
CdkWindow *      cdk_display_get_window_at_pointer (CdkDisplay             *display,
                                                    gint                   *win_x,
                                                    gint                   *win_y);
GDK_DEPRECATED_IN_3_0_FOR(cdk_device_warp)
void             cdk_display_warp_pointer          (CdkDisplay             *display,
                                                    CdkScreen              *screen,
                                                    gint                   x,
                                                    gint                   y);
#endif /* GDK_MULTIDEVICE_SAFE */

GDK_DEPRECATED_IN_3_16
CdkDisplay *cdk_display_open_default_libctk_only (void);

GDK_AVAILABLE_IN_ALL
gboolean cdk_display_supports_cursor_alpha     (CdkDisplay    *display);
GDK_AVAILABLE_IN_ALL
gboolean cdk_display_supports_cursor_color     (CdkDisplay    *display);
GDK_AVAILABLE_IN_ALL
guint    cdk_display_get_default_cursor_size   (CdkDisplay    *display);
GDK_AVAILABLE_IN_ALL
void     cdk_display_get_maximal_cursor_size   (CdkDisplay    *display,
                                                guint         *width,
                                                guint         *height);

GDK_AVAILABLE_IN_ALL
CdkWindow *cdk_display_get_default_group       (CdkDisplay *display); 

GDK_AVAILABLE_IN_ALL
gboolean cdk_display_supports_selection_notification (CdkDisplay *display);
GDK_AVAILABLE_IN_ALL
gboolean cdk_display_request_selection_notification  (CdkDisplay *display,
                                                      CdkAtom     selection);

GDK_AVAILABLE_IN_ALL
gboolean cdk_display_supports_clipboard_persistence (CdkDisplay    *display);
GDK_AVAILABLE_IN_ALL
void     cdk_display_store_clipboard                (CdkDisplay    *display,
                                                     CdkWindow     *clipboard_window,
                                                     guint32        time_,
                                                     const CdkAtom *targets,
                                                     gint           n_targets);

GDK_AVAILABLE_IN_ALL
gboolean cdk_display_supports_shapes           (CdkDisplay    *display);
GDK_AVAILABLE_IN_ALL
gboolean cdk_display_supports_input_shapes     (CdkDisplay    *display);
GDK_DEPRECATED_IN_3_16
gboolean cdk_display_supports_composite        (CdkDisplay    *display);
GDK_AVAILABLE_IN_ALL
void     cdk_display_notify_startup_complete   (CdkDisplay    *display,
                                                const gchar   *startup_id);

GDK_DEPRECATED_IN_3_20_FOR(cdk_display_get_default_seat)
CdkDeviceManager * cdk_display_get_device_manager (CdkDisplay *display);

GDK_AVAILABLE_IN_ALL
CdkAppLaunchContext *cdk_display_get_app_launch_context (CdkDisplay *display);

GDK_AVAILABLE_IN_3_20
CdkSeat * cdk_display_get_default_seat (CdkDisplay *display);

GDK_AVAILABLE_IN_3_20
GList   * cdk_display_list_seats       (CdkDisplay *display);

GDK_AVAILABLE_IN_3_22
int          cdk_display_get_n_monitors        (CdkDisplay *display);
GDK_AVAILABLE_IN_3_22
CdkMonitor * cdk_display_get_monitor           (CdkDisplay *display,
                                                int         monitor_num);
GDK_AVAILABLE_IN_3_22
CdkMonitor * cdk_display_get_primary_monitor   (CdkDisplay *display);
GDK_AVAILABLE_IN_3_22
CdkMonitor * cdk_display_get_monitor_at_point  (CdkDisplay *display,
                                                int         x,
                                                int         y);
GDK_AVAILABLE_IN_3_22
CdkMonitor * cdk_display_get_monitor_at_window (CdkDisplay *display,
                                                CdkWindow  *window);


G_END_DECLS

#endif  /* __GDK_DISPLAY_H__ */
