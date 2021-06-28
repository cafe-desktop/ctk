/*
 * cdkscreen-x11.h
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

#ifndef __GDK_X11_SCREEN__
#define __GDK_X11_SCREEN__

#include "cdkscreenprivate.h"
#include "cdkx11screen.h"
#include "cdkvisual.h"
#include <X11/X.h>
#include <X11/Xlib.h>

G_BEGIN_DECLS
  
typedef struct _CdkX11Monitor CdkX11Monitor;

struct _CdkX11Screen
{
  CdkScreen parent_instance;

  CdkDisplay *display;
  Display *xdisplay;
  Screen *xscreen;
  Window xroot_window;
  CdkWindow *root_window;
  gint screen_num;

  gint width;
  gint height;

  gint window_scale;
  gboolean fixed_window_scale;

  /* Xft resources for the display, used for default values for
   * the Xft/ XSETTINGS
   */
  gint xft_hintstyle;
  gint xft_rgba;
  gint xft_dpi;

  /* Window manager */
  long last_wmspec_check_time;
  Window wmspec_check_window;
  char *window_manager_name;

  /* X Settings */
  CdkWindow *xsettings_manager_window;
  Atom xsettings_selection_atom;
  GHashTable *xsettings; /* string of GDK settings name => GValue */

  /* TRUE if wmspec_check_window has changed since last
   * fetch of _NET_SUPPORTED
   */
  guint need_refetch_net_supported : 1;
  /* TRUE if wmspec_check_window has changed since last
   * fetch of window manager name
   */
  guint need_refetch_wm_name : 1;
  guint is_composited : 1;
  guint xft_init : 1; /* Whether we've intialized these values yet */
  guint xft_antialias : 1;
  guint xft_hinting : 1;

  /* Visual Part */
  gint nvisuals;
  CdkVisual **visuals;
  CdkVisual *system_visual;
  gint available_depths[7];
  CdkVisualType available_types[6];
  gint16 navailable_depths;
  gint16 navailable_types;
  GHashTable *visual_hash;
  CdkVisual *rgba_visual;

  /* cache for window->translate vfunc */
  GC subwindow_gcs[32];
};

struct _CdkX11ScreenClass
{
  CdkScreenClass parent_class;

  void (* window_manager_changed) (CdkX11Screen *x11_screen);
};

GType       _cdk_x11_screen_get_type (void);
CdkScreen * _cdk_x11_screen_new      (CdkDisplay *display,
				      gint	  screen_number);

void _cdk_x11_screen_setup                  (CdkScreen *screen);
void _cdk_x11_screen_update_visuals_for_gl  (CdkScreen *screen);
void _cdk_x11_screen_window_manager_changed (CdkScreen *screen);
void _cdk_x11_screen_size_changed           (CdkScreen *screen,
					     XEvent    *event);
void _cdk_x11_screen_process_owner_change   (CdkScreen *screen,
					     XEvent    *event);
void _cdk_x11_screen_get_edge_monitors      (CdkScreen *screen,
					     gint      *top,
					     gint      *bottom,
					     gint      *left,
					     gint      *right);
void _cdk_x11_screen_set_window_scale       (CdkX11Screen *x11_screen,
					     int        scale);
gboolean _cdk_x11_screen_get_monitor_work_area (CdkScreen    *screen,
                                                CdkMonitor   *monitor,
                                                CdkRectangle *area);
void cdk_x11_screen_get_work_area           (CdkScreen    *screen,
                                             CdkRectangle *area);
gint cdk_x11_screen_get_width               (CdkScreen *screen);
gint cdk_x11_screen_get_height              (CdkScreen *screen);
gint cdk_x11_screen_get_number              (CdkScreen *screen);

G_END_DECLS

#endif /* __GDK_X11_SCREEN__ */
