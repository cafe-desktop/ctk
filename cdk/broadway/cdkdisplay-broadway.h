/*
 * cdkdisplay-broadway.h
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

#ifndef __GDK_BROADWAY_DISPLAY__
#define __GDK_BROADWAY_DISPLAY__

#include "cdkbroadwaydisplay.h"

#include "cdkdisplayprivate.h"
#include "cdkkeys.h"
#include "cdkwindow.h"
#include "cdkinternals.h"
#include "cdkmain.h"
#include "cdkbroadway-server.h"
#include "cdkmonitorprivate.h"

G_BEGIN_DECLS

struct _CdkBroadwayDisplay
{
  CdkDisplay parent_instance;
  CdkScreen *default_screen;
  CdkScreen **screens;

  GHashTable *id_ht;
  GList *toplevels;

  GSource *event_source;

  /* Keyboard related information */
  CdkKeymap *keymap;

  /* drag and drop information */
  CdkDragContext *current_dest_drag;

  /* The offscreen window that has the pointer in it (if any) */
  CdkWindow *active_offscreen_window;

  CdkBroadwayServer *server;

  gpointer move_resize_data;

  CdkMonitor *monitor;
};

struct _CdkBroadwayDisplayClass
{
  CdkDisplayClass parent_class;
};

G_END_DECLS

#endif				/* __GDK_BROADWAY_DISPLAY__ */
