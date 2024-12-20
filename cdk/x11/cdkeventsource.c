/* CDK - The GIMP Drawing Kit
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

#include "cdkeventsource.h"

#include "cdkinternals.h"
#include "cdkwindow-x11.h"
#include "cdkprivate-x11.h"


static gboolean cdk_event_source_prepare  (GSource     *source,
                                           gint        *timeout);
static gboolean cdk_event_source_check    (GSource     *source);
static gboolean cdk_event_source_dispatch (GSource     *source,
                                           GSourceFunc  callback,
                                           gpointer     user_data);
static void     cdk_event_source_finalize (GSource     *source);

#define HAS_FOCUS(toplevel)                           \
  ((toplevel)->has_focus || (toplevel)->has_pointer_focus)

struct _CdkEventSource
{
  GSource source;

  CdkDisplay *display;
  GPollFD event_poll_fd;
  GList *translators;
};

static GSourceFuncs event_funcs = {
  .prepare = cdk_event_source_prepare,
  .check = cdk_event_source_check,
  .dispatch = cdk_event_source_dispatch,
  .finalize = cdk_event_source_finalize
};

static gint
cdk_event_apply_filters (XEvent    *xevent,
			 CdkEvent  *event,
			 CdkWindow *window)
{
  GList *tmp_list;
  CdkFilterReturn result;

  if (window == NULL)
    tmp_list = _cdk_default_filters;
  else
    tmp_list = window->filters;

  while (tmp_list)
    {
      CdkEventFilter *filter = (CdkEventFilter*) tmp_list->data;
      GList *node;

      if ((filter->flags & CDK_EVENT_FILTER_REMOVED) != 0)
        {
          tmp_list = tmp_list->next;
          continue;
        }

      filter->ref_count++;
      result = filter->function (xevent, event, filter->data);

      /* Protect against unreffing the filter mutating the list */
      node = tmp_list->next;

      _cdk_event_filter_unref (window, filter);

      tmp_list = node;

      if (result != CDK_FILTER_CONTINUE)
        return result;
    }

  return CDK_FILTER_CONTINUE;
}

static CdkWindow *
cdk_event_source_get_filter_window (CdkEventSource      *event_source,
                                    XEvent              *xevent,
                                    CdkEventTranslator **event_translator)
{
  GList *list = event_source->translators;
  CdkWindow *window;

  *event_translator = NULL;

  while (list)
    {
      CdkEventTranslator *translator = list->data;

      list = list->next;
      window = _cdk_x11_event_translator_get_window (translator,
                                                     event_source->display,
                                                     xevent);
      if (window)
        {
          *event_translator = translator;
          return window;
        }
    }

  window = cdk_x11_window_lookup_for_display (event_source->display,
                                              xevent->xany.window);

  if (window && !CDK_IS_WINDOW (window))
    window = NULL;

  return window;
}

static void
handle_focus_change (CdkEventCrossing *event)
{
  CdkToplevelX11 *toplevel;
  CdkX11Screen *x11_screen;
  gboolean focus_in, had_focus;

  toplevel = _cdk_x11_window_get_toplevel (event->window);
  x11_screen = CDK_X11_SCREEN (cdk_window_get_screen (event->window));
  focus_in = (event->type == CDK_ENTER_NOTIFY);

  if (x11_screen->wmspec_check_window)
    return;

  if (!toplevel || event->detail == CDK_NOTIFY_INFERIOR)
    return;

  toplevel->has_pointer = focus_in;

  if (!event->focus || toplevel->has_focus_window)
    return;

  had_focus = HAS_FOCUS (toplevel);
  toplevel->has_pointer_focus = focus_in;

  if (HAS_FOCUS (toplevel) != had_focus)
    {
      CdkEvent *focus_event;

      focus_event = cdk_event_new (CDK_FOCUS_CHANGE);
      focus_event->focus_change.window = g_object_ref (event->window);
      focus_event->focus_change.send_event = FALSE;
      focus_event->focus_change.in = focus_in;
      cdk_event_set_device (focus_event, cdk_event_get_device ((CdkEvent *) event));

      cdk_event_put (focus_event);
      cdk_event_free (focus_event);
    }
}

static CdkEvent *
cdk_event_source_translate_event (CdkEventSource *event_source,
                                  XEvent         *xevent)
{
  CdkEvent *event = cdk_event_new (CDK_NOTHING);
  CdkFilterReturn result = CDK_FILTER_CONTINUE;
  CdkEventTranslator *event_translator;
  CdkWindow *filter_window;
  Display *dpy;

  dpy = CDK_DISPLAY_XDISPLAY (event_source->display);

#ifdef HAVE_XGENERICEVENTS
  /* Get cookie data here so it's available
   * to every event translator and event filter.
   */
  if (xevent->type == GenericEvent)
    XGetEventData (dpy, &xevent->xcookie);
#endif

  filter_window = cdk_event_source_get_filter_window (event_source, xevent,
                                                      &event_translator);
  if (filter_window)
    event->any.window = g_object_ref (filter_window);

  /* Run default filters */
  if (_cdk_default_filters)
    {
      /* Apply global filters */
      result = cdk_event_apply_filters (xevent, event, NULL);
    }

  if (result == CDK_FILTER_CONTINUE &&
      filter_window && filter_window->filters)
    {
      /* Apply per-window filters */
      result = cdk_event_apply_filters (xevent, event, filter_window);
    }

  if (result != CDK_FILTER_CONTINUE)
    {
#ifdef HAVE_XGENERICEVENTS
      if (xevent->type == GenericEvent)
        XFreeEventData (dpy, &xevent->xcookie);
#endif

      if (result == CDK_FILTER_REMOVE)
        {
          cdk_event_free (event);
          return NULL;
        }
      else /* CDK_FILTER_TRANSLATE */
        return event;
    }

  cdk_event_free (event);
  event = NULL;

  if (event_translator)
    {
      /* Event translator was gotten before in get_filter_window() */
      event = _cdk_x11_event_translator_translate (event_translator,
                                                   event_source->display,
                                                   xevent);
    }
  else
    {
      GList *list = event_source->translators;

      while (list && !event)
        {
          CdkEventTranslator *translator = list->data;

          list = list->next;
          event = _cdk_x11_event_translator_translate (translator,
                                                       event_source->display,
                                                       xevent);
        }
    }

  if (event &&
      (event->type == CDK_ENTER_NOTIFY ||
       event->type == CDK_LEAVE_NOTIFY) &&
      event->crossing.window != NULL)
    {
      /* Handle focusing (in the case where no window manager is running */
      handle_focus_change (&event->crossing);
    }

#ifdef HAVE_XGENERICEVENTS
  if (xevent->type == GenericEvent)
    XFreeEventData (dpy, &xevent->xcookie);
#endif

  return event;
}

static gboolean
cdk_check_xpending (CdkDisplay *display)
{
  return XPending (CDK_DISPLAY_XDISPLAY (display));
}

static gboolean
cdk_event_source_prepare (GSource *source,
                          gint    *timeout)
{
  CdkDisplay *display = ((CdkEventSource*) source)->display;
  gboolean retval;

  cdk_threads_enter ();

  *timeout = -1;

  if (display->event_pause_count > 0)
    retval = _cdk_event_queue_find_first (display) != NULL;
  else
    retval = (_cdk_event_queue_find_first (display) != NULL ||
              cdk_check_xpending (display));

  cdk_threads_leave ();

  return retval;
}

static gboolean
cdk_event_source_check (GSource *source)
{
  CdkEventSource *event_source = (CdkEventSource*) source;
  gboolean retval;

  cdk_threads_enter ();

  if (event_source->display->event_pause_count > 0)
    retval = _cdk_event_queue_find_first (event_source->display) != NULL;
  else if (event_source->event_poll_fd.revents & G_IO_IN)
    retval = (_cdk_event_queue_find_first (event_source->display) != NULL ||
              cdk_check_xpending (event_source->display));
  else
    retval = FALSE;

  cdk_threads_leave ();

  return retval;
}

void
_cdk_x11_display_queue_events (CdkDisplay *display)
{
  CdkEvent *event;
  XEvent xevent;
  Display *xdisplay = CDK_DISPLAY_XDISPLAY (display);
  CdkEventSource *event_source;
  CdkX11Display *display_x11;

  display_x11 = CDK_X11_DISPLAY (display);
  event_source = (CdkEventSource *) display_x11->event_source;

  while (!_cdk_event_queue_find_first (display) && XPending (xdisplay))
    {
      XNextEvent (xdisplay, &xevent);

      switch (xevent.type)
        {
        case KeyPress:
        case KeyRelease:
          break;
        default:
          if (XFilterEvent (&xevent, None))
            continue;
        }

      event = cdk_event_source_translate_event (event_source, &xevent);

      if (event)
        {
          GList *node;

          node = _cdk_event_queue_append (display, event);
          _cdk_windowing_got_event (display, node, event, xevent.xany.serial);
        }
    }
}

static gboolean
cdk_event_source_dispatch (GSource     *source,
                           GSourceFunc  callback G_GNUC_UNUSED,
                           gpointer     user_data G_GNUC_UNUSED)
{
  CdkDisplay *display = ((CdkEventSource*) source)->display;
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
cdk_event_source_finalize (GSource *source)
{
  CdkEventSource *event_source = (CdkEventSource *)source;

  g_list_free (event_source->translators);
  event_source->translators = NULL;
}

GSource *
cdk_x11_event_source_new (CdkDisplay *display)
{
  GSource *source;
  CdkEventSource *event_source;
  CdkX11Display *display_x11;
  int connection_number;
  char *name;

  source = g_source_new (&event_funcs, sizeof (CdkEventSource));
  name = g_strdup_printf ("CDK X11 Event source (%s)",
                          cdk_display_get_name (display));
  g_source_set_name (source, name);
  g_free (name);
  event_source = (CdkEventSource *) source;
  event_source->display = display;

  display_x11 = CDK_X11_DISPLAY (display);
  connection_number = ConnectionNumber (display_x11->xdisplay);

  event_source->event_poll_fd.fd = connection_number;
  event_source->event_poll_fd.events = G_IO_IN;
  g_source_add_poll (source, &event_source->event_poll_fd);

  g_source_set_priority (source, CDK_PRIORITY_EVENTS);
  g_source_set_can_recurse (source, TRUE);
  g_source_attach (source, NULL);

  return source;
}

void
cdk_x11_event_source_add_translator (CdkEventSource     *source,
                                     CdkEventTranslator *translator)
{
  g_return_if_fail (CDK_IS_EVENT_TRANSLATOR (translator));

  source->translators = g_list_append (source->translators, translator);
}

void
cdk_x11_event_source_select_events (CdkEventSource *source,
                                    Window          window,
                                    CdkEventMask    event_mask,
                                    unsigned int    extra_x_mask)
{
  unsigned int xmask = extra_x_mask;
  GList *list;
  gint i;

  list = source->translators;

  while (list)
    {
      CdkEventTranslator *translator = list->data;
      CdkEventMask translator_mask, mask;

      translator_mask = _cdk_x11_event_translator_get_handled_events (translator);
      mask = event_mask & translator_mask;

      if (mask != 0)
        {
          _cdk_x11_event_translator_select_window_events (translator, window, mask);
          event_mask &= ~mask;
        }

      list = list->next;
    }

  for (i = 0; i < _cdk_x11_event_mask_table_size; i++)
    {
      if (event_mask & (1 << (i + 1)))
        xmask |= _cdk_x11_event_mask_table[i];
    }

  XSelectInput (CDK_DISPLAY_XDISPLAY (source->display), window, xmask);
}
