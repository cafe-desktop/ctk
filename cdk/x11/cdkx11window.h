/* CDK - The GIMP Drawing Kit
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

#ifndef __CDK_X11_WINDOW_H__
#define __CDK_X11_WINDOW_H__

#if !defined (__CDKX_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdkx.h> can be included directly."
#endif

#include <cdk/cdk.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

G_BEGIN_DECLS

#define CDK_TYPE_X11_WINDOW              (cdk_x11_window_get_type ())
#define CDK_X11_WINDOW(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), CDK_TYPE_X11_WINDOW, CdkX11Window))
#define CDK_X11_WINDOW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CDK_TYPE_X11_WINDOW, CdkX11WindowClass))
#define CDK_IS_X11_WINDOW(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), CDK_TYPE_X11_WINDOW))
#define CDK_IS_X11_WINDOW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CDK_TYPE_X11_WINDOW))
#define CDK_X11_WINDOW_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CDK_TYPE_X11_WINDOW, CdkX11WindowClass))

#ifdef CDK_COMPILATION
typedef struct _CdkX11Window CdkX11Window;
#else
typedef CdkWindow CdkX11Window;
#endif
typedef struct _CdkX11WindowClass CdkX11WindowClass;

CDK_AVAILABLE_IN_ALL
GType    cdk_x11_window_get_type          (void);

CDK_AVAILABLE_IN_ALL
Window   cdk_x11_window_get_xid           (CdkWindow   *window);
CDK_AVAILABLE_IN_ALL
void     cdk_x11_window_set_user_time     (CdkWindow   *window,
                                           guint32      timestamp);
CDK_AVAILABLE_IN_3_4
void     cdk_x11_window_set_utf8_property    (CdkWindow *window,
					      const gchar *name,
					      const gchar *value);
CDK_AVAILABLE_IN_3_2
void     cdk_x11_window_set_theme_variant (CdkWindow   *window,
                                           char        *variant);
CDK_DEPRECATED_IN_3_12_FOR(cdk_window_set_shadow_width)
void     cdk_x11_window_set_frame_extents (CdkWindow *window,
                                           int        left,
                                           int        right,
                                           int        top,
                                           int        bottom);
CDK_AVAILABLE_IN_3_4
void     cdk_x11_window_set_hide_titlebar_when_maximized (CdkWindow *window,
                                                          gboolean   hide_titlebar_when_maximized);
CDK_AVAILABLE_IN_ALL
void     cdk_x11_window_move_to_current_desktop (CdkWindow   *window);

CDK_AVAILABLE_IN_3_10
guint32  cdk_x11_window_get_desktop             (CdkWindow   *window);
CDK_AVAILABLE_IN_3_10
void     cdk_x11_window_move_to_desktop         (CdkWindow   *window,
                                                 guint32      desktop);

CDK_AVAILABLE_IN_3_8
void     cdk_x11_window_set_frame_sync_enabled (CdkWindow *window,
                                                gboolean   frame_sync_enabled);

/**
 * CDK_WINDOW_XDISPLAY:
 * @win: a #CdkWindow.
 *
 * Returns the display of a #CdkWindow.
 *
 * Returns: an Xlib Display*.
 */
#define CDK_WINDOW_XDISPLAY(win)      (CDK_DISPLAY_XDISPLAY (cdk_window_get_display (win)))

/**
 * CDK_WINDOW_XID:
 * @win: a #CdkWindow.
 *
 * Returns the X window belonging to a #CdkWindow.
 *
 * Returns: the Xlib Window of @win.
 */
#define CDK_WINDOW_XID(win)           (cdk_x11_window_get_xid (win))

CDK_AVAILABLE_IN_ALL
guint32       cdk_x11_get_server_time  (CdkWindow       *window);

CDK_AVAILABLE_IN_ALL
CdkWindow  *cdk_x11_window_foreign_new_for_display (CdkDisplay *display,
                                                    Window      window);
CDK_AVAILABLE_IN_ALL
CdkWindow  *cdk_x11_window_lookup_for_display      (CdkDisplay *display,
                                                    Window      window);

G_END_DECLS

#endif /* __CDK_X11_WINDOW_H__ */
