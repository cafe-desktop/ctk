/* GDK - The GIMP Drawing Kit
 * Copyright (C) 2009 Carlos Garnacho <carlosg@gnome.org>
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

#include "config.h"

#include "cdkinternals.h"
#include "cdkprivate-wayland.h"

#include <unistd.h>
#include <errno.h>

typedef struct _CdkWaylandEventSource {
  GSource source;
  GPollFD pfd;
  uint32_t mask;
  CdkDisplay *display;
  gboolean reading;
} CdkWaylandEventSource;

static gboolean
cdk_event_source_prepare (GSource *base,
                          gint    *timeout)
{
  CdkWaylandEventSource *source = (CdkWaylandEventSource *) base;
  CdkWaylandDisplay *display = (CdkWaylandDisplay *) source->display;

  *timeout = -1;

  if (source->display->event_pause_count > 0)
    return _cdk_event_queue_find_first (source->display) != NULL;

  /* We have to add/remove the GPollFD if we want to update our
   * poll event mask dynamically.  Instead, let's just flush all
   * write on idle instead, which is what this amounts to.
   */

  if (_cdk_event_queue_find_first (source->display) != NULL)
    return TRUE;

  /* wl_display_prepare_read() needs to be balanced with either
   * wl_display_read_events() or wl_display_cancel_read()
   * (in cdk_event_source_check() */
  if (source->reading)
    return FALSE;

  /* if prepare_read() returns non-zero, there are events to be dispatched */
  if (wl_display_prepare_read (display->wl_display) != 0)
    return TRUE;
  source->reading = TRUE;

  if (wl_display_flush (display->wl_display) < 0)
    {
      g_message ("Error flushing display: %s", g_strerror (errno));
      _exit (1);
    }

  return FALSE;
}

static gboolean
cdk_event_source_check (GSource *base)
{
  CdkWaylandEventSource *source = (CdkWaylandEventSource *) base;
  CdkWaylandDisplay *display_wayland = (CdkWaylandDisplay *) source->display;

  if (source->display->event_pause_count > 0)
    {
      if (source->reading)
        wl_display_cancel_read (display_wayland->wl_display);
      source->reading = FALSE;

      return _cdk_event_queue_find_first (source->display) != NULL;
    }

  /* read the events from the wayland fd into their respective queues if we have data */
  if (source->reading)
    {
      if (source->pfd.revents & G_IO_IN)
        {
          if (wl_display_read_events (display_wayland->wl_display) < 0)
            {
              g_message ("Error reading events from display: %s", g_strerror (errno));
              _exit (1);
            }
        }
      else
        wl_display_cancel_read (display_wayland->wl_display);
      source->reading = FALSE;
    }

  return _cdk_event_queue_find_first (source->display) != NULL ||
    source->pfd.revents;
}

static gboolean
cdk_event_source_dispatch (GSource     *base,
			   GSourceFunc  callback,
			   gpointer     data)
{
  CdkWaylandEventSource *source = (CdkWaylandEventSource *) base;
  CdkDisplay *display = source->display;
  CdkEvent *event;

  cdk_threads_enter ();

  event = cdk_display_get_event (display);

  if (event)
    {
      _cdk_event_emit (event);

      cdk_event_free (event);
    }

  cdk_threads_leave ();

  return TRUE;
}

static void
cdk_event_source_finalize (GSource *base)
{
  CdkWaylandEventSource *source = (CdkWaylandEventSource *) base;
  CdkWaylandDisplay *display = (CdkWaylandDisplay *) source->display;

  if (source->reading)
    wl_display_cancel_read (display->wl_display);
  source->reading = FALSE;
}

static GSourceFuncs wl_glib_source_funcs = {
  cdk_event_source_prepare,
  cdk_event_source_check,
  cdk_event_source_dispatch,
  cdk_event_source_finalize
};

void
_cdk_wayland_display_deliver_event (CdkDisplay *display,
                                    CdkEvent   *event)
{
  GList *node;

  node = _cdk_event_queue_append (display, event);
  _cdk_windowing_got_event (display, node, event,
                            _cdk_display_get_next_serial (display));
}

GSource *
_cdk_wayland_display_event_source_new (CdkDisplay *display)
{
  GSource *source;
  CdkWaylandEventSource *wl_source;
  CdkWaylandDisplay *display_wayland;
  char *name;

  source = g_source_new (&wl_glib_source_funcs,
			 sizeof (CdkWaylandEventSource));
  name = g_strdup_printf ("GDK Wayland Event source (%s)",
                          cdk_display_get_name (display));
  g_source_set_name (source, name);
  g_free (name);
  wl_source = (CdkWaylandEventSource *) source;

  display_wayland = GDK_WAYLAND_DISPLAY (display);
  wl_source->display = display;
  wl_source->pfd.fd = wl_display_get_fd (display_wayland->wl_display);
  wl_source->pfd.events = G_IO_IN | G_IO_ERR | G_IO_HUP;
  g_source_add_poll (source, &wl_source->pfd);

  g_source_set_priority (source, GDK_PRIORITY_EVENTS);
  g_source_set_can_recurse (source, TRUE);
  g_source_attach (source, NULL);

  return source;
}

void
_cdk_wayland_display_queue_events (CdkDisplay *display)
{
  CdkWaylandDisplay *display_wayland;
  CdkWaylandEventSource *source;

  display_wayland = GDK_WAYLAND_DISPLAY (display);
  source = (CdkWaylandEventSource *) display_wayland->event_source;

  if (wl_display_dispatch_pending (display_wayland->wl_display) < 0)
    {
      g_message ("Error %d (%s) dispatching to Wayland display.",
                 errno, g_strerror (errno));
      _exit (1);
    }

  if (source->pfd.revents & (G_IO_ERR | G_IO_HUP))
    {
      g_message ("Lost connection to Wayland compositor.");
      _exit (1);
    }
  source->pfd.revents = 0;
}
