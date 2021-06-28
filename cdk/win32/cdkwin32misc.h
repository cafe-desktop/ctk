/* CDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 2013 Chun-wei Fan
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

#ifndef __CDK_WIN32_MISC_H__
#define __CDK_WIN32_MISC_H__

#if !defined (__CDKWIN32_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdkwin32.h> can be included directly."
#endif

#include <cdk/cdk.h>

#ifndef STRICT
#define STRICT			/* We want strict type checks */
#endif
#include <windows.h>
#include <commctrl.h>

G_BEGIN_DECLS

#ifdef INSIDE_CDK_WIN32

#include "cdkprivate-win32.h"

#define CDK_WINDOW_HWND(win)          (CDK_WINDOW_IMPL_WIN32(win->impl)->handle)
#else
/* definition for exported 'internals' go here */
#define CDK_WINDOW_HWND(d) (cdk_win32_window_get_handle (d))

#endif /* INSIDE_CDK_WIN32 */

/* These need to be here so ctkstatusicon.c can pick them up if needed. */
#ifndef WM_XBUTTONDOWN
#define WM_XBUTTONDOWN 0x020B
#endif
#ifndef WM_XBUTTONUP
#define WM_XBUTTONUP 0x020C
#endif
#ifndef GET_XBUTTON_WPARAM
#define GET_XBUTTON_WPARAM(w) (HIWORD(w))
#endif
#ifndef XBUTTON1
#define XBUTTON1 1
#endif
#ifndef XBUTTON2
#define XBUTTON2 2
#endif

/* Return true if the CdkWindow is a win32 implemented window */
CDK_AVAILABLE_IN_ALL
gboolean      cdk_win32_window_is_win32 (CdkWindow *window);
CDK_AVAILABLE_IN_ALL
HWND          cdk_win32_window_get_impl_hwnd (CdkWindow *window);

/* Return the Cdk* for a particular HANDLE */
CDK_AVAILABLE_IN_ALL
gpointer      cdk_win32_handle_table_lookup (HWND handle);
/* Translate from window to Windows handle */
CDK_AVAILABLE_IN_ALL
HGDIOBJ       cdk_win32_window_get_handle (CdkWindow *window);

CDK_AVAILABLE_IN_ALL
void          cdk_win32_selection_add_targets (CdkWindow  *owner,
					       CdkAtom     selection,
					       gint	   n_targets,
					       CdkAtom    *targets);

#if defined (CTK_COMPILATION) || defined (CDK_COMPILATION)
#define cdk_win32_selection_clear_targets cdk_win32_selection_clear_targets_libctk_only
CDK_AVAILABLE_IN_ALL
void          cdk_win32_selection_clear_targets (CdkDisplay *display,
                                                 CdkAtom     selection);
#endif

CDK_AVAILABLE_IN_ALL
CdkWindow *   cdk_win32_window_foreign_new_for_display (CdkDisplay *display,
                                                        HWND        anid);
CDK_AVAILABLE_IN_ALL
CdkWindow *   cdk_win32_window_lookup_for_display (CdkDisplay *display,
                                                   HWND        anid);

#if defined (INSIDE_CDK_WIN32) || defined (CDK_COMPILATION) || defined (CTK_COMPILATION)

/* For internal CTK use only */
CDK_AVAILABLE_IN_ALL
GdkPixbuf    *cdk_win32_icon_to_pixbuf_libctk_only (HICON hicon,
                                                    gdouble *x_hot,
                                                    gdouble *y_hot);
CDK_AVAILABLE_IN_ALL
HICON         cdk_win32_pixbuf_to_hicon_libctk_only (GdkPixbuf *pixbuf);
CDK_AVAILABLE_IN_ALL
void          cdk_win32_set_modal_dialog_libctk_only (HWND window);

#endif

G_END_DECLS

#endif /* __CDK_WIN32_MISC_H__ */
