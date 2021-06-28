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

#ifndef __CDK_X11_SCREEN_H__
#define __CDK_X11_SCREEN_H__

#if !defined (__CDKX_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdkx.h> can be included directly."
#endif

#include <cdk/cdk.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

G_BEGIN_DECLS

#define CDK_TYPE_X11_SCREEN              (cdk_x11_screen_get_type ())
#define CDK_X11_SCREEN(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), CDK_TYPE_X11_SCREEN, CdkX11Screen))
#define CDK_X11_SCREEN_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CDK_TYPE_X11_SCREEN, CdkX11ScreenClass))
#define CDK_IS_X11_SCREEN(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), CDK_TYPE_X11_SCREEN))
#define CDK_IS_X11_SCREEN_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CDK_TYPE_X11_SCREEN))
#define CDK_X11_SCREEN_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CDK_TYPE_X11_SCREEN, CdkX11ScreenClass))

#ifdef CDK_COMPILATION
typedef struct _CdkX11Screen CdkX11Screen;
#else
typedef CdkScreen CdkX11Screen;
#endif
typedef struct _CdkX11ScreenClass CdkX11ScreenClass;

CDK_AVAILABLE_IN_ALL
GType    cdk_x11_screen_get_type          (void);

CDK_AVAILABLE_IN_ALL
Screen * cdk_x11_screen_get_xscreen       (CdkScreen   *screen);
CDK_AVAILABLE_IN_ALL
int      cdk_x11_screen_get_screen_number (CdkScreen   *screen);

CDK_AVAILABLE_IN_ALL
const char* cdk_x11_screen_get_window_manager_name (CdkScreen *screen);

CDK_AVAILABLE_IN_ALL
gint     cdk_x11_get_default_screen       (void);

/**
 * CDK_SCREEN_XDISPLAY:
 * @screen: a #CdkScreen
 *
 * Returns the display of a X11 #CdkScreen.
 *
 * Returns: an Xlib Display*.
 */
#define CDK_SCREEN_XDISPLAY(screen) (cdk_x11_display_get_xdisplay (cdk_screen_get_display (screen)))

/**
 * CDK_SCREEN_XSCREEN:
 * @screen: a #CdkScreen
 *
 * Returns the screen of a X11 #CdkScreen.
 *
 * Returns: an Xlib Screen*
 */
#define CDK_SCREEN_XSCREEN(screen) (cdk_x11_screen_get_xscreen (screen))

/**
 * CDK_SCREEN_XNUMBER:
 * @screen: a #CdkScreen
 *
 * Returns the index of a X11 #CdkScreen.
 *
 * Returns: the position of @screen among the screens of its display
 */
#define CDK_SCREEN_XNUMBER(screen) (cdk_x11_screen_get_screen_number (screen))

CDK_AVAILABLE_IN_ALL
gboolean cdk_x11_screen_supports_net_wm_hint (CdkScreen *screen,
                                              CdkAtom    property);

CDK_AVAILABLE_IN_ALL
XID      cdk_x11_screen_get_monitor_output   (CdkScreen *screen,
                                              gint       monitor_num);

CDK_AVAILABLE_IN_3_10
guint32  cdk_x11_screen_get_number_of_desktops (CdkScreen *screen);
CDK_AVAILABLE_IN_3_10
guint32  cdk_x11_screen_get_current_desktop    (CdkScreen *screen);

G_END_DECLS

#endif /* __CDK_X11_SCREEN_H__ */
