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

#ifndef __CDK_WINDOW_X11_H__
#define __CDK_WINDOW_X11_H__

#include "cdk/x11/cdkprivate-x11.h"
#include "cdk/cdkwindowimpl.h"

#include <X11/Xlib.h>

#ifdef HAVE_XDAMAGE
#include <X11/extensions/Xdamage.h>
#endif

#ifdef HAVE_XSYNC
#include <X11/Xlib.h>
#include <X11/extensions/sync.h>
#endif

G_BEGIN_DECLS

typedef struct _CdkToplevelX11 CdkToplevelX11;
typedef struct _CdkWindowImplX11 CdkWindowImplX11;
typedef struct _CdkWindowImplX11Class CdkWindowImplX11Class;
typedef struct _CdkXPositionInfo CdkXPositionInfo;

/* Window implementation for X11
 */

#define CDK_TYPE_WINDOW_IMPL_X11              (cdk_window_impl_x11_get_type ())
#define CDK_WINDOW_IMPL_X11(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), CDK_TYPE_WINDOW_IMPL_X11, CdkWindowImplX11))
#define CDK_WINDOW_IMPL_X11_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CDK_TYPE_WINDOW_IMPL_X11, CdkWindowImplX11Class))
#define CDK_IS_WINDOW_IMPL_X11(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), CDK_TYPE_WINDOW_IMPL_X11))
#define CDK_IS_WINDOW_IMPL_X11_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CDK_TYPE_WINDOW_IMPL_X11))
#define CDK_WINDOW_IMPL_X11_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CDK_TYPE_WINDOW_IMPL_X11, CdkWindowImplX11Class))

struct _CdkWindowImplX11
{
  CdkWindowImpl parent_instance;

  CdkWindow *wrapper;

  Window xid;

  CdkToplevelX11 *toplevel;	/* Toplevel-specific information */
  CdkCursor *cursor;
  GHashTable *device_cursor;

  guint no_bg : 1;        /* Set when the window background is temporarily
                           * unset during resizing and scaling */
  guint override_redirect : 1;
  guint frame_clock_connected : 1;
  guint frame_sync_enabled : 1;
  guint tracking_damage: 1;

  gint window_scale;

  /* Width and height not divided by window_scale - this matters in the
   * corner-case where the window manager assigns us a size that isn't
   * a multiple of window_scale - for example for a maximized window
   * with an odd-sized title-bar.
   */
  gint unscaled_width;
  gint unscaled_height;

  cairo_surface_t *cairo_surface;

#if defined (HAVE_XCOMPOSITE) && defined(HAVE_XDAMAGE) && defined (HAVE_XFIXES)
  Damage damage;
#endif
};
 
struct _CdkWindowImplX11Class 
{
  CdkWindowImplClass parent_class;
};

struct _CdkToplevelX11
{

  /* Set if the window, or any descendent of it, is the server's focus window
   */
  guint has_focus_window : 1;

  /* Set if window->has_focus_window and the focus isn't grabbed elsewhere.
   */
  guint has_focus : 1;

  /* Set if the pointer is inside this window. (This is needed for
   * for focus tracking)
   */
  guint has_pointer : 1;
  
  /* Set if the window is a descendent of the focus window and the pointer is
   * inside it. (This is the case where the window will receive keystroke
   * events even window->has_focus_window is FALSE)
   */
  guint has_pointer_focus : 1;

  /* Set if we are requesting these hints */
  guint skip_taskbar_hint : 1;
  guint skip_pager_hint : 1;
  guint urgency_hint : 1;

  guint on_all_desktops : 1;   /* _NET_WM_STICKY == 0xFFFFFFFF */

  guint have_sticky : 1;	/* _NET_WM_STATE_STICKY */
  guint have_maxvert : 1;       /* _NET_WM_STATE_MAXIMIZED_VERT */
  guint have_maxhorz : 1;       /* _NET_WM_STATE_MAXIMIZED_HORZ */
  guint have_fullscreen : 1;    /* _NET_WM_STATE_FULLSCREEN */
  guint have_hidden : 1;	/* _NET_WM_STATE_HIDDEN */

  guint is_leader : 1;

  /* Set if the WM is presenting us as focused, i.e. with active decorations
   */
  guint have_focused : 1;

  guint in_frame : 1;

  /* If we're expecting a response from the compositor after painting a frame */
  guint frame_pending : 1;

  /* Whether pending_counter_value/configure_counter_value are updates
   * to the extended update counter */
  guint pending_counter_value_is_extended : 1;
  guint configure_counter_value_is_extended : 1;

  gulong map_serial;	/* Serial of last transition from unmapped */
  
  cairo_surface_t *icon_pixmap;
  cairo_surface_t *icon_mask;
  CdkWindow *group_leader;

  /* Time of most recent user interaction. */
  gulong user_time;

  /* We use an extra X window for toplevel windows that we XSetInputFocus()
   * to in order to avoid getting keyboard events redirected to subwindows
   * that might not even be part of this app
   */
  Window focus_window;

  CdkWindowHints last_geometry_hints_mask;
  CdkGeometry last_geometry_hints;
  
  /* Constrained edge information */
  guint edge_constraints;

#ifdef HAVE_XSYNC
  XID update_counter;
  XID extended_update_counter;
  gint64 pending_counter_value; /* latest _NET_WM_SYNC_REQUEST value received */
  gint64 configure_counter_value; /* Latest _NET_WM_SYNC_REQUEST value received
				 * where we have also seen the corresponding
				 * ConfigureNotify
				 */
  gint64 current_counter_value;

  /* After a _NET_WM_FRAME_DRAWN message, this is the soonest that we think
   * frame after will be presented */
  gint64 throttled_presentation_time;
#endif
};

GType cdk_window_impl_x11_get_type (void);

void            cdk_x11_window_set_user_time        (CdkWindow *window,
						     guint32    timestamp);
void            cdk_x11_window_set_frame_sync_enabled (CdkWindow *window,
                                                       gboolean   frame_sync_enabled);

CdkToplevelX11 *_cdk_x11_window_get_toplevel        (CdkWindow *window);
void            _cdk_x11_window_tmp_unset_bg        (CdkWindow *window,
						     gboolean   recurse);
void            _cdk_x11_window_tmp_reset_bg        (CdkWindow *window,
						     gboolean   recurse);
void            _cdk_x11_window_tmp_unset_parent_bg (CdkWindow *window);
void            _cdk_x11_window_tmp_reset_parent_bg (CdkWindow *window);

CdkCursor      *_cdk_x11_window_get_cursor          (CdkWindow *window);

void            _cdk_x11_window_update_size         (CdkWindowImplX11 *impl);
void            _cdk_x11_window_set_window_scale    (CdkWindow *window,
						     int        scale);

G_END_DECLS

#endif /* __CDK_WINDOW_X11_H__ */
