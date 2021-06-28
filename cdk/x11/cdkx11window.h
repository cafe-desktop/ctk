/* GDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/. 
 */

#ifndef __GDK_X11_WINDOW_H__
#define __GDK_X11_WINDOW_H__

#if !defined (__GDKX_H_INSIDE__) && !defined (GDK_COMPILATION)
#error "Only <cdk/cdkx.h> can be included directly."
#endif

#include <cdk/cdk.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

G_BEGIN_DECLS

#define GDK_TYPE_X11_WINDOW              (cdk_x11_window_get_type ())
#define GDK_X11_WINDOW(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_X11_WINDOW, CdkX11Window))
#define GDK_X11_WINDOW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_X11_WINDOW, CdkX11WindowClass))
#define GDK_IS_X11_WINDOW(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_X11_WINDOW))
#define GDK_IS_X11_WINDOW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_X11_WINDOW))
#define GDK_X11_WINDOW_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_X11_WINDOW, CdkX11WindowClass))

#ifdef GDK_COMPILATION
typedef struct _CdkX11Window CdkX11Window;
#else
typedef CdkWindow CdkX11Window;
#endif
typedef struct _CdkX11WindowClass CdkX11WindowClass;

GDK_AVAILABLE_IN_ALL
GType    cdk_x11_window_get_type          (void);

GDK_AVAILABLE_IN_ALL
Window   cdk_x11_window_get_xid           (CdkWindow   *window);
GDK_AVAILABLE_IN_ALL
void     cdk_x11_window_set_user_time     (CdkWindow   *window,
                                           guint32      timestamp);
GDK_AVAILABLE_IN_3_4
void     cdk_x11_window_set_utf8_property    (CdkWindow *window,
					      const gchar *name,
					      const gchar *value);
GDK_AVAILABLE_IN_3_2
void     cdk_x11_window_set_theme_variant (CdkWindow   *window,
                                           char        *variant);
GDK_DEPRECATED_IN_3_12_FOR(cdk_window_set_shadow_width)
void     cdk_x11_window_set_frame_extents (CdkWindow *window,
                                           int        left,
                                           int        right,
                                           int        top,
                                           int        bottom);
GDK_AVAILABLE_IN_3_4
void     cdk_x11_window_set_hide_titlebar_when_maximized (CdkWindow *window,
                                                          gboolean   hide_titlebar_when_maximized);
GDK_AVAILABLE_IN_ALL
void     cdk_x11_window_move_to_current_desktop (CdkWindow   *window);

GDK_AVAILABLE_IN_3_10
guint32  cdk_x11_window_get_desktop             (CdkWindow   *window);
GDK_AVAILABLE_IN_3_10
void     cdk_x11_window_move_to_desktop         (CdkWindow   *window,
                                                 guint32      desktop);

GDK_AVAILABLE_IN_3_8
void     cdk_x11_window_set_frame_sync_enabled (CdkWindow *window,
                                                gboolean   frame_sync_enabled);

/**
 * GDK_WINDOW_XDISPLAY:
 * @win: a #CdkWindow.
 *
 * Returns the display of a #CdkWindow.
 *
 * Returns: an Xlib Display*.
 */
#define GDK_WINDOW_XDISPLAY(win)      (GDK_DISPLAY_XDISPLAY (cdk_window_get_display (win)))

/**
 * GDK_WINDOW_XID:
 * @win: a #CdkWindow.
 *
 * Returns the X window belonging to a #CdkWindow.
 *
 * Returns: the Xlib Window of @win.
 */
#define GDK_WINDOW_XID(win)           (cdk_x11_window_get_xid (win))

GDK_AVAILABLE_IN_ALL
guint32       cdk_x11_get_server_time  (CdkWindow       *window);

GDK_AVAILABLE_IN_ALL
CdkWindow  *cdk_x11_window_foreign_new_for_display (CdkDisplay *display,
                                                    Window      window);
GDK_AVAILABLE_IN_ALL
CdkWindow  *cdk_x11_window_lookup_for_display      (CdkDisplay *display,
                                                    Window      window);

G_END_DECLS

#endif /* __GDK_X11_WINDOW_H__ */
