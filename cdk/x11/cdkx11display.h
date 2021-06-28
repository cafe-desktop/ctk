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

#ifndef __CDK_X11_DISPLAY_H__
#define __CDK_X11_DISPLAY_H__

#if !defined (__CDKX_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdkx.h> can be included directly."
#endif

#include <cdk/cdk.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

G_BEGIN_DECLS

#ifdef CDK_COMPILATION
typedef struct _CdkX11Display CdkX11Display;
#else
typedef CdkDisplay CdkX11Display;
#endif
typedef struct _CdkX11DisplayClass CdkX11DisplayClass;

#define CDK_TYPE_X11_DISPLAY              (cdk_x11_display_get_type())
#define CDK_X11_DISPLAY(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), CDK_TYPE_X11_DISPLAY, CdkX11Display))
#define CDK_X11_DISPLAY_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CDK_TYPE_X11_DISPLAY, CdkX11DisplayClass))
#define CDK_IS_X11_DISPLAY(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), CDK_TYPE_X11_DISPLAY))
#define CDK_IS_X11_DISPLAY_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CDK_TYPE_X11_DISPLAY))
#define CDK_X11_DISPLAY_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CDK_TYPE_X11_DISPLAY, CdkX11DisplayClass))

CDK_AVAILABLE_IN_ALL
GType      cdk_x11_display_get_type            (void);

CDK_AVAILABLE_IN_ALL
Display *cdk_x11_display_get_xdisplay     (CdkDisplay  *display);

#define CDK_DISPLAY_XDISPLAY(display) (cdk_x11_display_get_xdisplay (display))

CDK_AVAILABLE_IN_ALL
guint32       cdk_x11_display_get_user_time (CdkDisplay *display);

CDK_AVAILABLE_IN_ALL
const gchar * cdk_x11_display_get_startup_notification_id         (CdkDisplay *display);
CDK_AVAILABLE_IN_ALL
void          cdk_x11_display_set_startup_notification_id         (CdkDisplay  *display,
                                                                   const gchar *startup_id);

CDK_AVAILABLE_IN_ALL
void          cdk_x11_display_set_cursor_theme (CdkDisplay  *display,
                                                const gchar *theme,
                                                const gint   size);

CDK_AVAILABLE_IN_ALL
void cdk_x11_display_broadcast_startup_message (CdkDisplay *display,
                                                const char *message_type,
                                                ...) G_GNUC_NULL_TERMINATED;

CDK_AVAILABLE_IN_ALL
CdkDisplay   *cdk_x11_lookup_xdisplay (Display *xdisplay);

CDK_AVAILABLE_IN_ALL
void        cdk_x11_display_grab              (CdkDisplay *display);
CDK_AVAILABLE_IN_ALL
void        cdk_x11_display_ungrab            (CdkDisplay *display);

CDK_AVAILABLE_IN_3_10
void        cdk_x11_display_set_window_scale (CdkDisplay *display,
                                              gint scale);

CDK_AVAILABLE_IN_ALL
void                           cdk_x11_display_error_trap_push        (CdkDisplay *display);
/* warn unused because you could use pop_ignored otherwise */
CDK_AVAILABLE_IN_ALL
G_GNUC_WARN_UNUSED_RESULT gint cdk_x11_display_error_trap_pop         (CdkDisplay *display);
CDK_AVAILABLE_IN_ALL
void                           cdk_x11_display_error_trap_pop_ignored (CdkDisplay *display);

CDK_AVAILABLE_IN_ALL
void        cdk_x11_register_standard_event_type (CdkDisplay *display,
                                                  gint        event_base,
                                                  gint        n_events);

CDK_AVAILABLE_IN_ALL
void        cdk_x11_set_sm_client_id (const gchar *sm_client_id);


G_END_DECLS

#endif /* __CDK_X11_DISPLAY_H__ */
