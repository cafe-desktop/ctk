 /*
 * cdkscreen-broadway.c
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

#include "config.h"

#include "cdkscreen-broadway.h"

#include "cdkscreen.h"
#include "cdkdisplay.h"
#include "cdkdisplay-broadway.h"

#include <glib.h>

#include <stdlib.h>
#include <string.h>

static void   cdk_broadway_screen_dispose     (GObject *object);
static void   cdk_broadway_screen_finalize    (GObject *object);

G_DEFINE_TYPE (CdkBroadwayScreen, cdk_broadway_screen, GDK_TYPE_SCREEN)

static void
cdk_broadway_screen_init (CdkBroadwayScreen *screen)
{
  screen->width = 1024;
  screen->height = 768;
}

static CdkDisplay *
cdk_broadway_screen_get_display (CdkScreen *screen)
{
  return GDK_BROADWAY_SCREEN (screen)->display;
}

static gint
cdk_broadway_screen_get_width (CdkScreen *screen)
{
  return GDK_BROADWAY_SCREEN (screen)->width;
}

static gint
cdk_broadway_screen_get_height (CdkScreen *screen)
{
  return GDK_BROADWAY_SCREEN (screen)->height;
}

static gint
cdk_broadway_screen_get_width_mm (CdkScreen *screen)
{
  return cdk_screen_get_width (screen) * 25.4 / 96;
}

static gint
cdk_broadway_screen_get_height_mm (CdkScreen *screen)
{
  return cdk_screen_get_height (screen) * 25.4 / 96;
}

static gint
cdk_broadway_screen_get_number (CdkScreen *screen)
{
  return 0;
}

static CdkWindow *
cdk_broadway_screen_get_root_window (CdkScreen *screen)
{
  return GDK_BROADWAY_SCREEN (screen)->root_window;
}

void
_cdk_broadway_screen_size_changed (CdkScreen                       *screen,
                                   BroadwayInputScreenResizeNotify *msg)
{
  CdkBroadwayScreen *broadway_screen = GDK_BROADWAY_SCREEN (screen);
  CdkMonitor *monitor;
  gint width, height;
  GList *toplevels, *l;

  width = cdk_screen_get_width (screen);
  height = cdk_screen_get_height (screen);

  broadway_screen->width   = msg->width;
  broadway_screen->height  = msg->height;

  if (width == cdk_screen_get_width (screen) &&
      height == cdk_screen_get_height (screen))
    return;

  monitor = GDK_BROADWAY_DISPLAY (broadway_screen->display)->monitor;

  cdk_monitor_set_size (monitor, msg->width, msg->height);
  cdk_monitor_set_physical_size (monitor, msg->width * 25.4 / 96, msg->height * 25.4 / 96);

  g_signal_emit_by_name (screen, "size-changed");
  toplevels = cdk_screen_get_toplevel_windows (screen);
  for (l = toplevels; l != NULL; l = l->next)
    {
      CdkWindow *toplevel = l->data;
      CdkWindowImplBroadway *toplevel_impl = GDK_WINDOW_IMPL_BROADWAY (toplevel->impl);

      if (toplevel_impl->maximized)
	cdk_window_move_resize (toplevel, 0, 0,
				cdk_screen_get_width (screen),
				cdk_screen_get_height (screen));
    }
}

static void
cdk_broadway_screen_dispose (GObject *object)
{
  CdkBroadwayScreen *broadway_screen = GDK_BROADWAY_SCREEN (object);

  if (broadway_screen->root_window)
    _cdk_window_destroy (broadway_screen->root_window, TRUE);

  G_OBJECT_CLASS (cdk_broadway_screen_parent_class)->dispose (object);
}

static void
cdk_broadway_screen_finalize (GObject *object)
{
  CdkBroadwayScreen *broadway_screen = GDK_BROADWAY_SCREEN (object);
  gint          i;

  if (broadway_screen->root_window)
    g_object_unref (broadway_screen->root_window);

  /* Visual Part */
  for (i = 0; i < broadway_screen->nvisuals; i++)
    g_object_unref (broadway_screen->visuals[i]);
  g_free (broadway_screen->visuals);

  G_OBJECT_CLASS (cdk_broadway_screen_parent_class)->finalize (object);
}

static CdkVisual *
cdk_broadway_screen_get_rgba_visual (CdkScreen *screen)
{
  CdkBroadwayScreen *broadway_screen = GDK_BROADWAY_SCREEN (screen);

  return broadway_screen->rgba_visual;
}

CdkScreen *
_cdk_broadway_screen_new (CdkDisplay *display,
			  gint	 screen_number)
{
  CdkScreen *screen;
  CdkBroadwayScreen *broadway_screen;

  screen = g_object_new (GDK_TYPE_BROADWAY_SCREEN, NULL);

  broadway_screen = GDK_BROADWAY_SCREEN (screen);
  broadway_screen->display = display;
  _cdk_broadway_screen_init_visuals (screen);
  _cdk_broadway_screen_init_root_window (screen);

  return screen;
}

/*
 * It is important that we first request the selection
 * notification, and then setup the initial state of
 * is_composited to avoid a race condition here.
 */
void
_cdk_broadway_screen_setup (CdkScreen *screen)
{
}

static gboolean
cdk_broadway_screen_is_composited (CdkScreen *screen)
{
  return TRUE;
}


static gchar *
cdk_broadway_screen_make_display_name (CdkScreen *screen)
{
  return g_strdup ("browser");
}

static CdkWindow *
cdk_broadway_screen_get_active_window (CdkScreen *screen)
{
  return NULL;
}

static GList *
cdk_broadway_screen_get_window_stack (CdkScreen *screen)
{
  return NULL;
}

static void
cdk_broadway_screen_broadcast_client_message (CdkScreen *screen,
					      CdkEvent  *event)
{
}

static gboolean
cdk_broadway_screen_get_setting (CdkScreen   *screen,
				 const gchar *name,
				 GValue      *value)
{
  return FALSE;
}

void
_cdk_broadway_screen_events_init (CdkScreen *screen)
{
}

static void
cdk_broadway_screen_class_init (CdkBroadwayScreenClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CdkScreenClass *screen_class = GDK_SCREEN_CLASS (klass);

  object_class->dispose = cdk_broadway_screen_dispose;
  object_class->finalize = cdk_broadway_screen_finalize;

  screen_class->get_display = cdk_broadway_screen_get_display;
  screen_class->get_width = cdk_broadway_screen_get_width;
  screen_class->get_height = cdk_broadway_screen_get_height;
  screen_class->get_width_mm = cdk_broadway_screen_get_width_mm;
  screen_class->get_height_mm = cdk_broadway_screen_get_height_mm;
  screen_class->get_number = cdk_broadway_screen_get_number;
  screen_class->get_root_window = cdk_broadway_screen_get_root_window;
  screen_class->is_composited = cdk_broadway_screen_is_composited;
  screen_class->make_display_name = cdk_broadway_screen_make_display_name;
  screen_class->get_active_window = cdk_broadway_screen_get_active_window;
  screen_class->get_window_stack = cdk_broadway_screen_get_window_stack;
  screen_class->broadcast_client_message = cdk_broadway_screen_broadcast_client_message;
  screen_class->get_setting = cdk_broadway_screen_get_setting;
  screen_class->get_rgba_visual = cdk_broadway_screen_get_rgba_visual;
  screen_class->get_system_visual = _cdk_broadway_screen_get_system_visual;
  screen_class->visual_get_best_depth = _cdk_broadway_screen_visual_get_best_depth;
  screen_class->visual_get_best_type = _cdk_broadway_screen_visual_get_best_type;
  screen_class->visual_get_best = _cdk_broadway_screen_visual_get_best;
  screen_class->visual_get_best_with_depth = _cdk_broadway_screen_visual_get_best_with_depth;
  screen_class->visual_get_best_with_type = _cdk_broadway_screen_visual_get_best_with_type;
  screen_class->visual_get_best_with_both = _cdk_broadway_screen_visual_get_best_with_both;
  screen_class->query_depths = _cdk_broadway_screen_query_depths;
  screen_class->query_visual_types = _cdk_broadway_screen_query_visual_types;
  screen_class->list_visuals = _cdk_broadway_screen_list_visuals;
}

