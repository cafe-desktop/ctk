/* CDK - The GIMP Drawing Kit
 * cdkdisplay-x11.c
 * 
 * Copyright 2001 Sun Microsystems Inc.
 * Copyright (C) 2004 Nokia Corporation
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

#include "cdkasync.h"
#include "cdkdisplay.h"
#include "cdkeventsource.h"
#include "cdkeventtranslator.h"
#include "cdkframeclockprivate.h"
#include "cdkinternals.h"
#include "cdkscreen.h"
#include "cdkinternals.h"
#include "cdkdeviceprivate.h"
#include "cdkkeysprivate.h"
#include "cdkdevicemanager.h"
#include "xsettings-client.h"
#include "cdkdisplay-x11.h"
#include "cdkprivate-x11.h"
#include "cdkscreen-x11.h"
#include "cdkglcontext-x11.h"
#include "cdk-private.h"
#include "cdkprofilerprivate.h"

#include <glib.h>
#include <glib/gprintf.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <X11/Xatom.h>
#include <X11/Xlibint.h>

#ifdef HAVE_XKB
#include <X11/XKBlib.h>
#endif

#ifdef HAVE_XFIXES
#include <X11/extensions/Xfixes.h>
#endif

#include <X11/extensions/shape.h>

#ifdef HAVE_XCOMPOSITE
#include <X11/extensions/Xcomposite.h>
#endif

#ifdef HAVE_XDAMAGE
#include <X11/extensions/Xdamage.h>
#endif

#ifdef HAVE_RANDR
#include <X11/extensions/Xrandr.h>
#endif

typedef struct _CdkErrorTrap  CdkErrorTrap;

struct _CdkErrorTrap
{
  /* Next sequence when trap was pushed, i.e. first sequence to
   * ignore
   */
  gulong start_sequence;

  /* Next sequence when trap was popped, i.e. first sequence
   * to not ignore. 0 if trap is still active.
   */
  gulong end_sequence;

  /* Most recent error code within the sequence */
  int error_code;
};

static void   cdk_x11_display_dispose            (GObject            *object);
static void   cdk_x11_display_finalize           (GObject            *object);

static void     cdk_x11_display_event_translator_init (CdkEventTranslatorIface *iface);

static gboolean cdk_x11_display_translate_event (CdkEventTranslator *translator,
                                                 CdkDisplay         *display,
                                                 CdkEvent           *event,
                                                 XEvent             *xevent);

static void cdk_internal_connection_watch (Display  *display,
					   XPointer  arg,
					   gint      fd,
					   gboolean  opening,
					   XPointer *watch_data);

typedef struct _CdkEventTypeX11 CdkEventTypeX11;

struct _CdkEventTypeX11
{
  gint base;
  gint n_events;
};

/* Note that we never *directly* use WM_LOCALE_NAME, WM_PROTOCOLS,
 * but including them here has the side-effect of getting them
 * into the internal Xlib cache
 */
static const char *const precache_atoms[] = {
  "UTF8_STRING",
  "WM_CLIENT_LEADER",
  "WM_DELETE_WINDOW",
  "WM_ICON_NAME",
  "WM_LOCALE_NAME",
  "WM_NAME",
  "WM_PROTOCOLS",
  "WM_TAKE_FOCUS",
  "WM_WINDOW_ROLE",
  "_NET_ACTIVE_WINDOW",
  "_NET_CURRENT_DESKTOP",
  "_NET_FRAME_EXTENTS",
  "_NET_STARTUP_ID",
  "_NET_WM_CM_S0",
  "_NET_WM_DESKTOP",
  "_NET_WM_ICON",
  "_NET_WM_ICON_NAME",
  "_NET_WM_NAME",
  "_NET_WM_PID",
  "_NET_WM_PING",
  "_NET_WM_STATE",
  "_NET_WM_STATE_ABOVE",
  "_NET_WM_STATE_BELOW",
  "_NET_WM_STATE_FULLSCREEN",
  "_NET_WM_STATE_HIDDEN",
  "_NET_WM_STATE_MODAL",
  "_NET_WM_STATE_MAXIMIZED_VERT",
  "_NET_WM_STATE_MAXIMIZED_HORZ",
  "_NET_WM_STATE_SKIP_TASKBAR",
  "_NET_WM_STATE_SKIP_PAGER",
  "_NET_WM_STATE_STICKY",
  "_NET_WM_SYNC_REQUEST",
  "_NET_WM_SYNC_REQUEST_COUNTER",
  "_NET_WM_WINDOW_TYPE",
  "_NET_WM_WINDOW_TYPE_COMBO",
  "_NET_WM_WINDOW_TYPE_DIALOG",
  "_NET_WM_WINDOW_TYPE_DND",
  "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU",
  "_NET_WM_WINDOW_TYPE_MENU",
  "_NET_WM_WINDOW_TYPE_NORMAL",
  "_NET_WM_WINDOW_TYPE_POPUP_MENU",
  "_NET_WM_WINDOW_TYPE_TOOLTIP",
  "_NET_WM_WINDOW_TYPE_UTILITY",
  "_NET_WM_USER_TIME",
  "_NET_WM_USER_TIME_WINDOW",
  "_NET_VIRTUAL_ROOTS",
  "CDK_SELECTION",
  "_NET_WM_STATE_FOCUSED",
  "CDK_VISUALS"
};

static char *cdk_sm_client_id;

G_DEFINE_TYPE_WITH_CODE (CdkX11Display, cdk_x11_display, CDK_TYPE_DISPLAY,
                         G_IMPLEMENT_INTERFACE (CDK_TYPE_EVENT_TRANSLATOR,
                                                cdk_x11_display_event_translator_init))


static void
cdk_x11_display_init (CdkX11Display *display)
{
  display->monitors = g_ptr_array_new_with_free_func (g_object_unref);
}

static void
cdk_x11_display_event_translator_init (CdkEventTranslatorIface *iface)
{
  iface->translate_event = cdk_x11_display_translate_event;
}

#define ANY_EDGE_TILED (CDK_WINDOW_STATE_LEFT_TILED | \
                        CDK_WINDOW_STATE_RIGHT_TILED | \
                        CDK_WINDOW_STATE_TOP_TILED | \
                        CDK_WINDOW_STATE_BOTTOM_TILED)

static void
do_edge_constraint_state_check (CdkWindow      *window,
                                CdkWindowState  old_state,
                                CdkWindowState *set,
                                CdkWindowState *unset)
{
  CdkToplevelX11 *toplevel = _cdk_x11_window_get_toplevel (window);
  CdkWindowState local_set, local_unset;
  CdkScreen *screen = CDK_WINDOW_SCREEN (window);
  guint edge_constraints;

  local_set = *set;
  local_unset = *unset;
  edge_constraints = toplevel->edge_constraints;

  /* If the WM doesn't support _CTK_EDGE_CONSTRAINTS, rely on the fallback
   * implementation. If it supports _CTK_EDGE_CONSTRAINTS, arrange for
   * CDK_WINDOW_STATE_TILED to be set if any edge is tiled, and cleared
   * if no edge is tiled.
   */
  if (!cdk_x11_screen_supports_net_wm_hint (screen,
                                            cdk_atom_intern_static_string ("_CTK_EDGE_CONSTRAINTS")))
    {
      /* FIXME: we rely on implementation details of mutter here:
       * mutter only tiles horizontally, and sets maxvert when it does
       * and if it tiles, it always affects all edges
       */
      if (old_state & CDK_WINDOW_STATE_TILED)
        {
          if (!toplevel->have_maxvert)
            local_unset |= CDK_WINDOW_STATE_TILED;
        }
      else
        {
          if (toplevel->have_maxvert && !toplevel->have_maxhorz)
            local_set |= CDK_WINDOW_STATE_TILED;
        }
    }
  else
    {
      if (old_state & CDK_WINDOW_STATE_TILED)
        {
          if (!(edge_constraints & ANY_EDGE_TILED))
            local_unset |= CDK_WINDOW_STATE_TILED;
        }
      else
        {
          if (edge_constraints & ANY_EDGE_TILED)
            local_set |= CDK_WINDOW_STATE_TILED;
        }
    }

  /* Top edge */
  if (old_state & CDK_WINDOW_STATE_TOP_TILED)
    {
      if ((edge_constraints & CDK_WINDOW_STATE_TOP_TILED) == 0)
        local_unset |= CDK_WINDOW_STATE_TOP_TILED;
    }
  else
    {
      if (edge_constraints & CDK_WINDOW_STATE_TOP_TILED)
        local_set |= CDK_WINDOW_STATE_TOP_TILED;
    }

  if (old_state & CDK_WINDOW_STATE_TOP_RESIZABLE)
    {
      if ((edge_constraints & CDK_WINDOW_STATE_TOP_RESIZABLE) == 0)
        local_unset |= CDK_WINDOW_STATE_TOP_RESIZABLE;
    }
  else
    {
      if (edge_constraints & CDK_WINDOW_STATE_TOP_RESIZABLE)
        local_set |= CDK_WINDOW_STATE_TOP_RESIZABLE;
    }

  /* Right edge */
  if (old_state & CDK_WINDOW_STATE_RIGHT_TILED)
    {
      if ((edge_constraints & CDK_WINDOW_STATE_RIGHT_TILED) == 0)
        local_unset |= CDK_WINDOW_STATE_RIGHT_TILED;
    }
  else
    {
      if (edge_constraints & CDK_WINDOW_STATE_RIGHT_TILED)
        local_set |= CDK_WINDOW_STATE_RIGHT_TILED;
    }

  if (old_state & CDK_WINDOW_STATE_RIGHT_RESIZABLE)
    {
      if ((edge_constraints & CDK_WINDOW_STATE_RIGHT_RESIZABLE) == 0)
        local_unset |= CDK_WINDOW_STATE_RIGHT_RESIZABLE;
    }
  else
    {
      if (edge_constraints & CDK_WINDOW_STATE_RIGHT_RESIZABLE)
        local_set |= CDK_WINDOW_STATE_RIGHT_RESIZABLE;
    }

  /* Bottom edge */
  if (old_state & CDK_WINDOW_STATE_BOTTOM_TILED)
    {
      if ((edge_constraints & CDK_WINDOW_STATE_BOTTOM_TILED) == 0)
        local_unset |= CDK_WINDOW_STATE_BOTTOM_TILED;
    }
  else
    {
      if (edge_constraints & CDK_WINDOW_STATE_BOTTOM_TILED)
        local_set |= CDK_WINDOW_STATE_BOTTOM_TILED;
    }

  if (old_state & CDK_WINDOW_STATE_BOTTOM_RESIZABLE)
    {
      if ((edge_constraints & CDK_WINDOW_STATE_BOTTOM_RESIZABLE) == 0)
        local_unset |= CDK_WINDOW_STATE_BOTTOM_RESIZABLE;
    }
  else
    {
      if (edge_constraints & CDK_WINDOW_STATE_BOTTOM_RESIZABLE)
        local_set |= CDK_WINDOW_STATE_BOTTOM_RESIZABLE;
    }

  /* Left edge */
  if (old_state & CDK_WINDOW_STATE_LEFT_TILED)
    {
      if ((edge_constraints & CDK_WINDOW_STATE_LEFT_TILED) == 0)
        local_unset |= CDK_WINDOW_STATE_LEFT_TILED;
    }
  else
    {
      if (edge_constraints & CDK_WINDOW_STATE_LEFT_TILED)
        local_set |= CDK_WINDOW_STATE_LEFT_TILED;
    }

  if (old_state & CDK_WINDOW_STATE_LEFT_RESIZABLE)
    {
      if ((edge_constraints & CDK_WINDOW_STATE_LEFT_RESIZABLE) == 0)
        local_unset |= CDK_WINDOW_STATE_LEFT_RESIZABLE;
    }
  else
    {
      if (edge_constraints & CDK_WINDOW_STATE_LEFT_RESIZABLE)
        local_set |= CDK_WINDOW_STATE_LEFT_RESIZABLE;
    }

  *set = local_set;
  *unset = local_unset;
}

static void
do_net_wm_state_changes (CdkWindow *window)
{
  CdkToplevelX11 *toplevel = _cdk_x11_window_get_toplevel (window);
  CdkWindowState old_state, set, unset;

  if (CDK_WINDOW_DESTROYED (window) ||
      cdk_window_get_window_type (window) != CDK_WINDOW_TOPLEVEL)
    return;

  old_state = cdk_window_get_state (window);

  set = unset = 0;

  /* For found_sticky to remain TRUE, we have to also be on desktop
   * 0xFFFFFFFF
   */
  if (old_state & CDK_WINDOW_STATE_STICKY)
    {
      if (!(toplevel->have_sticky && toplevel->on_all_desktops))
        unset |= CDK_WINDOW_STATE_STICKY;
    }
  else
    {
      if (toplevel->have_sticky && toplevel->on_all_desktops)
        set |= CDK_WINDOW_STATE_STICKY;
    }

  if (old_state & CDK_WINDOW_STATE_FULLSCREEN)
    {
      if (!toplevel->have_fullscreen)
        unset |= CDK_WINDOW_STATE_FULLSCREEN;
    }
  else
    {
      if (toplevel->have_fullscreen)
        set |= CDK_WINDOW_STATE_FULLSCREEN;
    }

  /* Our "maximized" means both vertical and horizontal; if only one,
   * we don't expose that via CDK
   */
  if (old_state & CDK_WINDOW_STATE_MAXIMIZED)
    {
      if (!(toplevel->have_maxvert && toplevel->have_maxhorz))
        unset |= CDK_WINDOW_STATE_MAXIMIZED;
    }
  else
    {
      if (toplevel->have_maxvert && toplevel->have_maxhorz)
        set |= CDK_WINDOW_STATE_MAXIMIZED;
    }

  if (old_state & CDK_WINDOW_STATE_FOCUSED)
    {
      if (!toplevel->have_focused)
        unset |= CDK_WINDOW_STATE_FOCUSED;
    }
  else
    {
      if (toplevel->have_focused)
        set |= CDK_WINDOW_STATE_FOCUSED;
    }

  if (old_state & CDK_WINDOW_STATE_ICONIFIED)
    {
      if (!toplevel->have_hidden)
        unset |= CDK_WINDOW_STATE_ICONIFIED;
    }
  else
    {
      if (toplevel->have_hidden)
        set |= CDK_WINDOW_STATE_ICONIFIED;
    }

  /* Update edge constraints and tiling */
  do_edge_constraint_state_check (window, old_state, &set, &unset);

  cdk_synthesize_window_state (window, unset, set);
}

static void
cdk_check_wm_desktop_changed (CdkWindow *window)
{
  CdkToplevelX11 *toplevel = _cdk_x11_window_get_toplevel (window);
  CdkDisplay *display = CDK_WINDOW_DISPLAY (window);

  Atom type;
  gint format;
  gulong nitems;
  gulong bytes_after;
  guchar *data;
  gulong *desktop;

  type = None;
  cdk_x11_display_error_trap_push (display);
  XGetWindowProperty (CDK_DISPLAY_XDISPLAY (display),
                      CDK_WINDOW_XID (window),
                      cdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_DESKTOP"),
                      0, G_MAXLONG, False, XA_CARDINAL, &type,
                      &format, &nitems,
                      &bytes_after, &data);
  cdk_x11_display_error_trap_pop_ignored (display);

  if (type != None)
    {
      desktop = (gulong *)data;
      toplevel->on_all_desktops = ((*desktop & 0xFFFFFFFF) == 0xFFFFFFFF);
      XFree (desktop);
    }
  else
    toplevel->on_all_desktops = FALSE;

  do_net_wm_state_changes (window);
}

static void
cdk_check_wm_state_changed (CdkWindow *window)
{
  CdkToplevelX11 *toplevel = _cdk_x11_window_get_toplevel (window);
  CdkDisplay *display = CDK_WINDOW_DISPLAY (window);
  CdkScreen *screen = CDK_WINDOW_SCREEN (window);

  Atom type;
  gint format;
  gulong nitems;
  gulong bytes_after;
  guchar *data;
  Atom *atoms = NULL;
  gulong i;

  gboolean had_sticky = toplevel->have_sticky;

  toplevel->have_sticky = FALSE;
  toplevel->have_maxvert = FALSE;
  toplevel->have_maxhorz = FALSE;
  toplevel->have_fullscreen = FALSE;
  toplevel->have_focused = FALSE;
  toplevel->have_hidden = FALSE;

  type = None;
  cdk_x11_display_error_trap_push (display);
  XGetWindowProperty (CDK_DISPLAY_XDISPLAY (display), CDK_WINDOW_XID (window),
		      cdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_STATE"),
		      0, G_MAXLONG, False, XA_ATOM, &type, &format, &nitems,
		      &bytes_after, &data);
  cdk_x11_display_error_trap_pop_ignored (display);

  if (type != None)
    {
      Atom sticky_atom = cdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_STATE_STICKY");
      Atom maxvert_atom = cdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_STATE_MAXIMIZED_VERT");
      Atom maxhorz_atom	= cdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_STATE_MAXIMIZED_HORZ");
      Atom fullscreen_atom = cdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_STATE_FULLSCREEN");
      Atom focused_atom = cdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_STATE_FOCUSED");
      Atom hidden_atom = cdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_STATE_HIDDEN");

      atoms = (Atom *)data;

      i = 0;
      while (i < nitems)
        {
          if (atoms[i] == sticky_atom)
            toplevel->have_sticky = TRUE;
          else if (atoms[i] == maxvert_atom)
            toplevel->have_maxvert = TRUE;
          else if (atoms[i] == maxhorz_atom)
            toplevel->have_maxhorz = TRUE;
          else if (atoms[i] == fullscreen_atom)
            toplevel->have_fullscreen = TRUE;
          else if (atoms[i] == focused_atom)
            toplevel->have_focused = TRUE;
          else if (atoms[i] == hidden_atom)
            toplevel->have_hidden = TRUE;

          ++i;
        }

      XFree (atoms);
    }

  if (!cdk_x11_screen_supports_net_wm_hint (screen,
                                            cdk_atom_intern_static_string ("_NET_WM_STATE_FOCUSED")))
    toplevel->have_focused = TRUE;

  /* When have_sticky is turned on, we have to check the DESKTOP property
   * as well.
   */
  if (toplevel->have_sticky && !had_sticky)
    cdk_check_wm_desktop_changed (window);
  else
    do_net_wm_state_changes (window);
}

static void
cdk_check_edge_constraints_changed (CdkWindow *window)
{
  CdkToplevelX11 *toplevel = _cdk_x11_window_get_toplevel (window);
  CdkDisplay *display = CDK_WINDOW_DISPLAY (window);

  Atom type;
  gint format;
  gulong nitems;
  gulong bytes_after;
  guchar *data;
  gulong *constraints;

  type = None;
  cdk_x11_display_error_trap_push (display);
  XGetWindowProperty (CDK_DISPLAY_XDISPLAY (display),
                      CDK_WINDOW_XID (window),
                      cdk_x11_get_xatom_by_name_for_display (display, "_CTK_EDGE_CONSTRAINTS"),
                      0, G_MAXLONG, False, XA_CARDINAL, &type,
                      &format, &nitems,
                      &bytes_after, &data);
  cdk_x11_display_error_trap_pop_ignored (display);

  if (type != None)
    {
      constraints = (gulong *)data;

      /* The CDK enum for these states does not begin at zero so, to avoid
       * messing around with shifts, just make the passed value and CDK's
       * enum values match by shifting to the first tiled state.
       */
      toplevel->edge_constraints = constraints[0] << 9;

      XFree (constraints);
    }
  else
    {
      toplevel->edge_constraints = 0;
    }

  do_net_wm_state_changes (window);
}

static Window
get_event_xwindow (XEvent             *xevent)
{
  Window xwindow;

  switch (xevent->type)
    {
    case DestroyNotify:
      xwindow = xevent->xdestroywindow.window;
      break;
    case UnmapNotify:
      xwindow = xevent->xunmap.window;
      break;
    case MapNotify:
      xwindow = xevent->xmap.window;
      break;
    case ConfigureNotify:
      xwindow = xevent->xconfigure.window;
      break;
    case ReparentNotify:
      xwindow = xevent->xreparent.window;
      break;
    case GravityNotify:
      xwindow = xevent->xgravity.window;
      break;
    case CirculateNotify:
      xwindow = xevent->xcirculate.window;
      break;
    default:
      xwindow = xevent->xany.window;
    }

  return xwindow;
}

static gboolean
cdk_x11_display_translate_event (CdkEventTranslator *translator,
                                 CdkDisplay         *display,
                                 CdkEvent           *event,
                                 XEvent             *xevent)
{
  Window xwindow;
  CdkWindow *window;
  gboolean is_substructure;
  CdkWindowImplX11 *window_impl = NULL;
  CdkScreen *screen = NULL;
  CdkX11Screen *x11_screen = NULL;
  CdkToplevelX11 *toplevel = NULL;
  CdkX11Display *display_x11 = CDK_X11_DISPLAY (display);
  gboolean return_val;

  /* Find the CdkWindow that this event relates to. If that's
   * not the same as the window that the event was sent to,
   * we are getting an event from SubstructureNotifyMask.
   * We ignore such events for internal operation, but we
   * need to report them to the application because of
   * CDK_SUBSTRUCTURE_MASK (which should be removed at next
   * opportunity.) The most likely reason for getting these
   * events is when we are used in the Metacity or Mutter
   * window managers.
   */
  xwindow = get_event_xwindow (xevent);
  is_substructure = xwindow != xevent->xany.window;

  window = cdk_x11_window_lookup_for_display (display, xwindow);
  if (window)
    {
      /* We may receive events such as NoExpose/GraphicsExpose
       * and ShmCompletion for pixmaps
       */
      if (!CDK_IS_WINDOW (window))
        return FALSE;

      screen = CDK_WINDOW_SCREEN (window);
      x11_screen = CDK_X11_SCREEN (screen);
      toplevel = _cdk_x11_window_get_toplevel (window);
      window_impl = CDK_WINDOW_IMPL_X11 (window->impl);

      g_object_ref (window);
    }

  event->any.window = window;
  event->any.send_event = xevent->xany.send_event ? TRUE : FALSE;

  if (window && CDK_WINDOW_DESTROYED (window))
    {
      if (xevent->type != DestroyNotify)
	{
	  return_val = FALSE;
	  goto done;
	}
    }

  if (xevent->type == DestroyNotify && !is_substructure)
    {
      screen = CDK_X11_DISPLAY (display)->screen;
      x11_screen = CDK_X11_SCREEN (screen);

      if (x11_screen->wmspec_check_window == xevent->xdestroywindow.window)
        {
          x11_screen->wmspec_check_window = None;
          x11_screen->last_wmspec_check_time = 0;
          g_free (x11_screen->window_manager_name);
          x11_screen->window_manager_name = g_strdup ("unknown");

          /* careful, reentrancy */
          _cdk_x11_screen_window_manager_changed (screen);

          return_val = FALSE;
          goto done;
        }
    }

  /* We do a "manual" conversion of the XEvent to a
   *  CdkEvent. The structures are mostly the same so
   *  the conversion is fairly straightforward. We also
   *  optionally print debugging info regarding events
   *  received.
   */

  return_val = TRUE;

  switch (xevent->type)
    {
    case KeymapNotify:
      CDK_NOTE (EVENTS,
		g_message ("keymap notify"));

      /* Not currently handled */
      return_val = FALSE;
      break;

    case Expose:
      CDK_NOTE (EVENTS,
		g_message ("expose:\t\twindow: %ld  %d	x,y: %d %d  w,h: %d %d%s",
			   xevent->xexpose.window, xevent->xexpose.count,
			   xevent->xexpose.x, xevent->xexpose.y,
			   xevent->xexpose.width, xevent->xexpose.height,
			   event->any.send_event ? " (send)" : ""));

      if (window == NULL)
        {
          return_val = FALSE;
          break;
        }

      {
	CdkRectangle expose_rect;
        int x2, y2;

        expose_rect.x = xevent->xexpose.x / window_impl->window_scale;
        expose_rect.y = xevent->xexpose.y / window_impl->window_scale;

        x2 = (xevent->xexpose.x + xevent->xexpose.width + window_impl->window_scale -1) / window_impl->window_scale;
        expose_rect.width = x2 - expose_rect.x;

        y2 = (xevent->xexpose.y + xevent->xexpose.height + window_impl->window_scale -1) / window_impl->window_scale;
        expose_rect.height = y2 - expose_rect.y;

        _cdk_x11_window_process_expose (window, xevent->xexpose.serial, &expose_rect);
        return_val = FALSE;
      }

      break;

    case GraphicsExpose:
      {
	CdkRectangle expose_rect;
        int x2, y2;

        CDK_NOTE (EVENTS,
		  g_message ("graphics expose:\tdrawable: %ld",
			     xevent->xgraphicsexpose.drawable));

        if (window == NULL)
          {
            return_val = FALSE;
            break;
          }

        expose_rect.x = xevent->xgraphicsexpose.x / window_impl->window_scale;
        expose_rect.y = xevent->xgraphicsexpose.y / window_impl->window_scale;

        x2 = (xevent->xgraphicsexpose.x + xevent->xgraphicsexpose.width + window_impl->window_scale -1) / window_impl->window_scale;
        expose_rect.width = x2 - expose_rect.x;

        y2 = (xevent->xgraphicsexpose.y + xevent->xgraphicsexpose.height + window_impl->window_scale -1) / window_impl->window_scale;
        expose_rect.height = y2 - expose_rect.y;

        _cdk_x11_window_process_expose (window, xevent->xgraphicsexpose.serial, &expose_rect);
        return_val = FALSE;
      }
      break;

    case VisibilityNotify:
#ifdef G_ENABLE_DEBUG
      if (CDK_DEBUG_CHECK (EVENTS))
	switch (xevent->xvisibility.state)
	  {
	  case VisibilityFullyObscured:
	    g_message ("visibility notify:\twindow: %ld	 none",
		       xevent->xvisibility.window);
	    break;
	  case VisibilityPartiallyObscured:
	    g_message ("visibility notify:\twindow: %ld	 partial",
		       xevent->xvisibility.window);
	    break;
	  case VisibilityUnobscured:
	    g_message ("visibility notify:\twindow: %ld	 full",
		       xevent->xvisibility.window);
	    break;
	  }
#endif /* G_ENABLE_DEBUG */

      if (window == NULL)
        {
          return_val = FALSE;
          break;
        }

      event->visibility.type = CDK_VISIBILITY_NOTIFY;
      event->visibility.window = window;

      switch (xevent->xvisibility.state)
	{
	case VisibilityFullyObscured:
	  event->visibility.state = CDK_VISIBILITY_FULLY_OBSCURED;
	  break;

	case VisibilityPartiallyObscured:
	  event->visibility.state = CDK_VISIBILITY_PARTIAL;
	  break;

	case VisibilityUnobscured:
	  event->visibility.state = CDK_VISIBILITY_UNOBSCURED;
	  break;
	}

      break;

    case CreateNotify:
      CDK_NOTE (EVENTS,
		g_message ("create notify:\twindow: %ld  x,y: %d %d	w,h: %d %d  b-w: %d  parent: %ld	 ovr: %d",
			   xevent->xcreatewindow.window,
			   xevent->xcreatewindow.x,
			   xevent->xcreatewindow.y,
			   xevent->xcreatewindow.width,
			   xevent->xcreatewindow.height,
			   xevent->xcreatewindow.border_width,
			   xevent->xcreatewindow.parent,
			   xevent->xcreatewindow.override_redirect));
      /* not really handled */
      break;

    case DestroyNotify:
      CDK_NOTE (EVENTS,
		g_message ("destroy notify:\twindow: %ld",
			   xevent->xdestroywindow.window));

      if (!is_substructure)
	{
	  event->any.type = CDK_DESTROY;
	  event->any.window = window;

	  return_val = window && !CDK_WINDOW_DESTROYED (window);

	  if (window && CDK_WINDOW_XID (window) != x11_screen->xroot_window)
	    cdk_window_destroy_notify (window);
	}
      else
	return_val = FALSE;

      break;

    case UnmapNotify:
      CDK_NOTE (EVENTS,
		g_message ("unmap notify:\t\twindow: %ld",
			   xevent->xmap.window));

      event->any.type = CDK_UNMAP;
      event->any.window = window;

      if (window && !is_substructure)
	{
          /* If the WM supports the _NET_WM_STATE_HIDDEN hint, we do not want to
           * interpret UnmapNotify events as implying iconic state.
           * http://bugzilla.gnome.org/show_bug.cgi?id=590726.
           */
          if (screen &&
              !cdk_x11_screen_supports_net_wm_hint (screen,
                                                    cdk_atom_intern_static_string ("_NET_WM_STATE_HIDDEN")))
            {
              /* If we are shown (not withdrawn) and get an unmap, it means we were
               * iconified in the X sense. If we are withdrawn, and get an unmap, it
               * means we hid the window ourselves, so we will have already flipped
               * the iconified bit off.
               */
              if (CDK_WINDOW_IS_MAPPED (window))
                cdk_synthesize_window_state (window,
                                             0,
                                             CDK_WINDOW_STATE_ICONIFIED);
            }

          if (window_impl->toplevel &&
              window_impl->toplevel->frame_pending)
            {
              window_impl->toplevel->frame_pending = FALSE;
              _cdk_frame_clock_thaw (cdk_window_get_frame_clock (event->any.window));
            }

	  if (toplevel)
            cdk_window_freeze_toplevel_updates (window);

          _cdk_x11_window_grab_check_unmap (window, xevent->xany.serial);
        }

      break;

    case MapNotify:
      CDK_NOTE (EVENTS,
		g_message ("map notify:\t\twindow: %ld",
			   xevent->xmap.window));

      event->any.type = CDK_MAP;
      event->any.window = window;

      if (window && !is_substructure)
	{
	  /* Unset iconified if it was set */
	  if (window->state & CDK_WINDOW_STATE_ICONIFIED)
	    cdk_synthesize_window_state (window,
					 CDK_WINDOW_STATE_ICONIFIED,
					 0);

	  if (toplevel)
	    cdk_window_thaw_toplevel_updates (window);
	}

      break;

    case ReparentNotify:
      CDK_NOTE (EVENTS,
		g_message ("reparent notify:\twindow: %ld  x,y: %d %d  parent: %ld	ovr: %d",
			   xevent->xreparent.window,
			   xevent->xreparent.x,
			   xevent->xreparent.y,
			   xevent->xreparent.parent,
			   xevent->xreparent.override_redirect));

      /* Not currently handled */
      return_val = FALSE;
      break;

    case ConfigureNotify:
      CDK_NOTE (EVENTS,
		g_message ("configure notify:\twindow: %ld  x,y: %d %d	w,h: %d %d  b-w: %d  above: %ld	 ovr: %d%s",
			   xevent->xconfigure.window,
			   xevent->xconfigure.x,
			   xevent->xconfigure.y,
			   xevent->xconfigure.width,
			   xevent->xconfigure.height,
			   xevent->xconfigure.border_width,
			   xevent->xconfigure.above,
			   xevent->xconfigure.override_redirect,
			   !window
			   ? " (discarding)"
			   : window->window_type == CDK_WINDOW_CHILD
			   ? " (discarding child)"
			   : xevent->xconfigure.event != xevent->xconfigure.window
			   ? " (discarding substructure)"
			   : ""));
      if (window && CDK_WINDOW_TYPE (window) == CDK_WINDOW_ROOT)
        {
          window_impl->unscaled_width = xevent->xconfigure.width;
          window_impl->unscaled_height = xevent->xconfigure.height;
	  window->width = (xevent->xconfigure.width + window_impl->window_scale - 1) / window_impl->window_scale;
	  window->height = (xevent->xconfigure.height + window_impl->window_scale - 1) / window_impl->window_scale;

	  _cdk_window_update_size (window);
	  _cdk_x11_window_update_size (window_impl);
	  _cdk_x11_screen_size_changed (screen, xevent);
        }

#ifdef HAVE_XSYNC
      if (!is_substructure && toplevel && display_x11->use_sync && toplevel->pending_counter_value != 0)
	{
	  toplevel->configure_counter_value = toplevel->pending_counter_value;
	  toplevel->configure_counter_value_is_extended = toplevel->pending_counter_value_is_extended;
	  toplevel->pending_counter_value = 0;
	}
#endif

    if (!window ||
	  xevent->xconfigure.event != xevent->xconfigure.window ||
          CDK_WINDOW_TYPE (window) == CDK_WINDOW_CHILD ||
          CDK_WINDOW_TYPE (window) == CDK_WINDOW_ROOT)
	return_val = FALSE;
      else
	{
	  event->configure.type = CDK_CONFIGURE;
	  event->configure.window = window;
	  event->configure.width = (xevent->xconfigure.width + window_impl->window_scale - 1) / window_impl->window_scale;
	  event->configure.height = (xevent->xconfigure.height + window_impl->window_scale - 1) / window_impl->window_scale;

	  if (!xevent->xconfigure.send_event &&
	      !xevent->xconfigure.override_redirect &&
	      !CDK_WINDOW_DESTROYED (window))
	    {
	      gint tx = 0;
	      gint ty = 0;
	      Window child_window = 0;

	      cdk_x11_display_error_trap_push (display);
	      if (XTranslateCoordinates (CDK_WINDOW_XDISPLAY (window),
					 CDK_WINDOW_XID (window),
					 x11_screen->xroot_window,
					 0, 0,
					 &tx, &ty,
					 &child_window))
		{
		  event->configure.x = tx / window_impl->window_scale;
		  event->configure.y = ty / window_impl->window_scale;
		}
	      cdk_x11_display_error_trap_pop_ignored (display);
	    }
	  else
	    {
	      event->configure.x = xevent->xconfigure.x / window_impl->window_scale;
	      event->configure.y = xevent->xconfigure.y / window_impl->window_scale;
	    }
	  if (!is_substructure)
	    {
	      window->x = event->configure.x;
	      window->y = event->configure.y;

              if (window_impl->unscaled_width != xevent->xconfigure.width ||
                  window_impl->unscaled_height != xevent->xconfigure.height)
                {
                  window_impl->unscaled_width = xevent->xconfigure.width;
                  window_impl->unscaled_height = xevent->xconfigure.height;
                  window->width = event->configure.width;
                  window->height = event->configure.height;

                  _cdk_window_update_size (window);
                  _cdk_x11_window_update_size (window_impl);
                }

	      if (window->resize_count >= 1)
		{
		  window->resize_count -= 1;

		  if (window->resize_count == 0)
		    _cdk_x11_moveresize_configure_done (display, window);
		}
	    }
        }
      break;

    case PropertyNotify:
      CDK_NOTE (EVENTS,
		g_message ("property notify:\twindow: %ld, atom(%ld): %s%s%s",
			   xevent->xproperty.window,
			   xevent->xproperty.atom,
			   "\"",
			   cdk_x11_get_xatom_name_for_display (display, xevent->xproperty.atom),
			   "\""));

      if (window == NULL)
        {
	  return_val = FALSE;
          break;
        }

      /* We compare with the serial of the last time we mapped the
       * window to avoid refetching properties that we set ourselves
       */
      if (toplevel &&
	  xevent->xproperty.serial >= toplevel->map_serial)
	{
	  if (xevent->xproperty.atom == cdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_STATE"))
	    cdk_check_wm_state_changed (window);

	  if (xevent->xproperty.atom == cdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_DESKTOP"))
	    cdk_check_wm_desktop_changed (window);

	  if (xevent->xproperty.atom == cdk_x11_get_xatom_by_name_for_display (display, "_CTK_EDGE_CONSTRAINTS"))
	    cdk_check_edge_constraints_changed (window);
	}

      if (window->event_mask & CDK_PROPERTY_CHANGE_MASK)
	{
	  event->property.type = CDK_PROPERTY_NOTIFY;
	  event->property.window = window;
	  event->property.atom = cdk_x11_xatom_to_atom_for_display (display, xevent->xproperty.atom);
	  event->property.time = xevent->xproperty.time;
	  event->property.state = xevent->xproperty.state;
	}
      else
	return_val = FALSE;

      break;

    case SelectionClear:
      CDK_NOTE (EVENTS,
		g_message ("selection clear:\twindow: %ld",
			   xevent->xproperty.window));

      if (_cdk_x11_selection_filter_clear_event (&xevent->xselectionclear))
	{
	  event->selection.type = CDK_SELECTION_CLEAR;
	  event->selection.window = window;
	  event->selection.selection = cdk_x11_xatom_to_atom_for_display (display, xevent->xselectionclear.selection);
	  event->selection.time = xevent->xselectionclear.time;
	}
      else
	return_val = FALSE;

      break;

    case SelectionRequest:
      CDK_NOTE (EVENTS,
		g_message ("selection request:\twindow: %ld",
			   xevent->xproperty.window));

      event->selection.type = CDK_SELECTION_REQUEST;
      event->selection.window = window;
      event->selection.selection = cdk_x11_xatom_to_atom_for_display (display, xevent->xselectionrequest.selection);
      event->selection.target = cdk_x11_xatom_to_atom_for_display (display, xevent->xselectionrequest.target);
      if (xevent->xselectionrequest.property == None)
        event->selection.property = event->selection.target;
      else
        event->selection.property = cdk_x11_xatom_to_atom_for_display (display, xevent->xselectionrequest.property);
      if (xevent->xselectionrequest.requestor != None)
        event->selection.requestor = cdk_x11_window_foreign_new_for_display (display,
                                                                             xevent->xselectionrequest.requestor);
      else
        event->selection.requestor = NULL;
      event->selection.time = xevent->xselectionrequest.time;

      break;

    case SelectionNotify:
      CDK_NOTE (EVENTS,
		g_message ("selection notify:\twindow: %ld",
			   xevent->xproperty.window));

      event->selection.type = CDK_SELECTION_NOTIFY;
      event->selection.window = window;
      event->selection.selection = cdk_x11_xatom_to_atom_for_display (display, xevent->xselection.selection);
      event->selection.target = cdk_x11_xatom_to_atom_for_display (display, xevent->xselection.target);
      if (xevent->xselection.property == None)
        event->selection.property = event->selection.target;
      else
        event->selection.property = cdk_x11_xatom_to_atom_for_display (display, xevent->xselection.property);
      event->selection.time = xevent->xselection.time;

      break;

    case ColormapNotify:
      CDK_NOTE (EVENTS,
		g_message ("colormap notify:\twindow: %ld",
			   xevent->xcolormap.window));

      /* Not currently handled */
      return_val = FALSE;
      break;

    case ClientMessage:
      CDK_NOTE (EVENTS,
                g_message ("client message:\twindow: %ld",
                           xevent->xclient.window));

      /* Not currently handled */
      return_val = FALSE;
      break;

    case MappingNotify:
      CDK_NOTE (EVENTS,
		g_message ("mapping notify"));

      /* Let XLib know that there is a new keyboard mapping.
       */
      XRefreshKeyboardMapping (&xevent->xmapping);
      _cdk_x11_keymap_keys_changed (display);
      return_val = FALSE;
      break;

    default:
#ifdef HAVE_XFIXES
      if (xevent->type - display_x11->xfixes_event_base == XFixesSelectionNotify)
	{
	  XFixesSelectionNotifyEvent *selection_notify = (XFixesSelectionNotifyEvent *)xevent;

	  _cdk_x11_screen_process_owner_change (screen, xevent);
	  
	  event->owner_change.type = CDK_OWNER_CHANGE;
	  event->owner_change.window = window;
          if (selection_notify->owner != None)
            event->owner_change.owner = cdk_x11_window_foreign_new_for_display (display,
                                                                                selection_notify->owner);
          else
            event->owner_change.owner = NULL;
	  event->owner_change.reason = selection_notify->subtype;
	  event->owner_change.selection = 
	    cdk_x11_xatom_to_atom_for_display (display, 
					       selection_notify->selection);
	  event->owner_change.time = selection_notify->timestamp;
	  event->owner_change.selection_time = selection_notify->selection_timestamp;
	  
	  return_val = TRUE;
	}
      else
#endif
#ifdef HAVE_RANDR
      if (xevent->type - display_x11->xrandr_event_base == RRScreenChangeNotify ||
          xevent->type - display_x11->xrandr_event_base == RRNotify)
	{
          if (screen)
            _cdk_x11_screen_size_changed (screen, xevent);
	}
      else
#endif
#if defined(HAVE_XCOMPOSITE) && defined (HAVE_XDAMAGE) && defined (HAVE_XFIXES)
      if (display_x11->have_xdamage && window && window->composited &&
	  xevent->type == display_x11->xdamage_event_base + XDamageNotify &&
	  ((XDamageNotifyEvent *) xevent)->damage == window_impl->damage)
	{
	  XDamageNotifyEvent *damage_event = (XDamageNotifyEvent *) xevent;
	  XserverRegion repair;
	  CdkRectangle rect;
          int x2, y2;

	  rect.x = window->x + damage_event->area.x / window_impl->window_scale;
	  rect.y = window->y + damage_event->area.y / window_impl->window_scale;

          x2 = (rect.x * window_impl->window_scale + damage_event->area.width + window_impl->window_scale -1) / window_impl->window_scale;
          y2 = (rect.y * window_impl->window_scale + damage_event->area.height + window_impl->window_scale -1) / window_impl->window_scale;
	  rect.width = x2 - rect.x;
	  rect.height = y2 - rect.y;

	  repair = XFixesCreateRegion (display_x11->xdisplay,
				       &damage_event->area, 1);
	  XDamageSubtract (display_x11->xdisplay,
			   window_impl->damage,
			   repair, None);
	  XFixesDestroyRegion (display_x11->xdisplay, repair);

          if (window->parent != NULL)
           _cdk_x11_window_process_expose (window->parent,
                                           damage_event->serial, &rect);

	  return_val = TRUE;
	}
      else
#endif
#ifdef HAVE_XKB
      if (xevent->type == display_x11->xkb_event_type)
	{
	  XkbEvent *xkb_event = (XkbEvent *) xevent;

	  switch (xkb_event->any.xkb_type)
	    {
	    case XkbNewKeyboardNotify:
	    case XkbMapNotify:
	      _cdk_x11_keymap_keys_changed (display);

	      return_val = FALSE;
	      break;

	    case XkbStateNotify:
	      _cdk_x11_keymap_state_changed (display, xevent);
	      break;
	    }
	}
      else
#endif
        return_val = FALSE;
    }

 done:
  if (return_val)
    {
      if (event->any.window)
	g_object_ref (event->any.window);
    }
  else
    {
      /* Mark this event as having no resources to be freed */
      event->any.window = NULL;
      event->any.type = CDK_NOTHING;
    }

  if (window)
    g_object_unref (window);

  return return_val;
}

static CdkFrameTimings *
find_frame_timings (CdkFrameClock *clock,
                    guint64        serial)
{
  gint64 start_frame, end_frame, i;

  start_frame = cdk_frame_clock_get_history_start (clock);
  end_frame = cdk_frame_clock_get_frame_counter (clock);
  for (i = end_frame; i >= start_frame; i--)
    {
      CdkFrameTimings *timings = cdk_frame_clock_get_timings (clock, i);

      if (timings->cookie == serial)
        return timings;
    }

  return NULL;
}

/* _NET_WM_FRAME_DRAWN and _NET_WM_FRAME_TIMINGS messages represent time
 * as a "high resolution server time" - this is the server time interpolated
 * to microsecond resolution. The advantage of this time representation
 * is that if  X server is running on the same computer as a client, and
 * the Xserver uses 'clock_gettime(CLOCK_MONOTONIC, ...)' for the server
 * time, the client can detect this, and all such clients will share a
 * a time representation with high accuracy. If there is not a common
 * time source, then the time synchronization will be less accurate.
 */
gint64
server_time_to_monotonic_time (CdkX11Display *display_x11,
                               gint64         server_time)
{
  if (display_x11->server_time_query_time == 0 ||
      (!display_x11->server_time_is_monotonic_time &&
       server_time > display_x11->server_time_query_time + 10*1000*1000)) /* 10 seconds */
    {
      gint64 current_server_time = cdk_x11_get_server_time (display_x11->leader_cdk_window);
      gint64 current_server_time_usec = (gint64)current_server_time * 1000;
      gint64 current_monotonic_time = g_get_monotonic_time ();
      display_x11->server_time_query_time = current_monotonic_time;

      /* If the server time is within a second of the monotonic time,
       * we assume that they are identical. This seems like a big margin,
       * but we want to be as robust as possible even if the system
       * is under load and our processing of the server response is
       * delayed.
       */
      if (current_server_time_usec > current_monotonic_time - 1000*1000 &&
          current_server_time_usec < current_monotonic_time + 1000*1000)
        display_x11->server_time_is_monotonic_time = TRUE;

      display_x11->server_time_offset = current_server_time_usec - current_monotonic_time;
    }

  if (display_x11->server_time_is_monotonic_time)
    return server_time;
  else
    return server_time - display_x11->server_time_offset;
}

CdkFilterReturn
_cdk_wm_protocols_filter (CdkXEvent *xev,
			  CdkEvent  *event,
			  gpointer   data)
{
  XEvent *xevent = (XEvent *)xev;
  CdkWindow *win = event->any.window;
  CdkDisplay *display;
  Atom atom;

  if (!CDK_IS_X11_WINDOW (win) || CDK_WINDOW_DESTROYED (win))
    return CDK_FILTER_CONTINUE;

  if (xevent->type != ClientMessage)
    return CDK_FILTER_CONTINUE;

  display = CDK_WINDOW_DISPLAY (win);

  /* This isn't actually WM_PROTOCOLS because that wouldn't leave enough space
   * in the message for everything that gets stuffed in */
  if (xevent->xclient.message_type == cdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_FRAME_DRAWN"))
    {
      CdkWindowImplX11 *window_impl;
      window_impl = CDK_WINDOW_IMPL_X11 (win->impl);
      if (window_impl->toplevel)
        {
          guint32 d0 = xevent->xclient.data.l[0];
          guint32 d1 = xevent->xclient.data.l[1];
          guint32 d2 = xevent->xclient.data.l[2];
          guint32 d3 = xevent->xclient.data.l[3];

          guint64 serial = ((guint64)d1 << 32) | d0;
          gint64 frame_drawn_time = server_time_to_monotonic_time (CDK_X11_DISPLAY (display), ((guint64)d3 << 32) | d2);
          gint64 refresh_interval, presentation_time;

          CdkFrameClock *clock = cdk_window_get_frame_clock (win);
          CdkFrameTimings *timings = find_frame_timings (clock, serial);

          if (timings)
            timings->drawn_time = frame_drawn_time;

          if (window_impl->toplevel->frame_pending)
            {
              window_impl->toplevel->frame_pending = FALSE;
              _cdk_frame_clock_thaw (clock);
            }

          cdk_frame_clock_get_refresh_info (clock,
                                            frame_drawn_time,
                                            &refresh_interval,
                                            &presentation_time);
          if (presentation_time != 0)
            window_impl->toplevel->throttled_presentation_time = presentation_time + refresh_interval;
        }

      return CDK_FILTER_REMOVE;
    }

  if (xevent->xclient.message_type == cdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_FRAME_TIMINGS"))
    {
      CdkWindowImplX11 *window_impl;
      window_impl = CDK_WINDOW_IMPL_X11 (win->impl);
      if (window_impl->toplevel)
        {
          guint32 d0 = xevent->xclient.data.l[0];
          guint32 d1 = xevent->xclient.data.l[1];
          guint32 d2 = xevent->xclient.data.l[2];
          guint32 d3 = xevent->xclient.data.l[3];

          guint64 serial = ((guint64)d1 << 32) | d0;

          CdkFrameClock *clock = cdk_window_get_frame_clock (win);
          CdkFrameTimings *timings = find_frame_timings (clock, serial);

          if (timings)
            {
              gint32 presentation_time_offset = (gint32)d2;
              gint32 refresh_interval = d3;

              if (timings->drawn_time && presentation_time_offset)
                timings->presentation_time = timings->drawn_time + presentation_time_offset;

              if (refresh_interval)
                timings->refresh_interval = refresh_interval;

              timings->complete = TRUE;
#ifdef G_ENABLE_DEBUG
              if (CDK_DEBUG_CHECK (FRAMES))
                _cdk_frame_clock_debug_print_timings (clock, timings);

              if (cdk_profiler_is_running ())
                _cdk_frame_clock_add_timings_to_profiler (clock, timings);
#endif /* G_ENABLE_DEBUG */
            }
        }
    }

  if (xevent->xclient.message_type != cdk_x11_get_xatom_by_name_for_display (display, "WM_PROTOCOLS"))
    return CDK_FILTER_CONTINUE;

  atom = (Atom) xevent->xclient.data.l[0];

  if (atom == cdk_x11_get_xatom_by_name_for_display (display, "WM_DELETE_WINDOW"))
    {
  /* The delete window request specifies a window
   *  to delete. We don't actually destroy the
   *  window because "it is only a request". (The
   *  window might contain vital data that the
   *  program does not want destroyed). Instead
   *  the event is passed along to the program,
   *  which should then destroy the window.
   */
      CDK_NOTE (EVENTS,
		g_message ("delete window:\t\twindow: %ld",
			   xevent->xclient.window));

      event->any.type = CDK_DELETE;

      cdk_x11_window_set_user_time (win, xevent->xclient.data.l[1]);

      return CDK_FILTER_TRANSLATE;
    }
  else if (atom == cdk_x11_get_xatom_by_name_for_display (display, "WM_TAKE_FOCUS"))
    {
      CdkToplevelX11 *toplevel = _cdk_x11_window_get_toplevel (win);

      /* There is no way of knowing reliably whether we are viewable;
       * so trap errors asynchronously around the XSetInputFocus call
       */
      if (toplevel && win->accept_focus)
        {
          cdk_x11_display_error_trap_push (display);
          XSetInputFocus (CDK_DISPLAY_XDISPLAY (display),
                          toplevel->focus_window,
                          RevertToParent,
                          xevent->xclient.data.l[1]);
          cdk_x11_display_error_trap_pop_ignored (display);
       }

      return CDK_FILTER_REMOVE;
    }
  else if (atom == cdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_PING") &&
	   !_cdk_x11_display_is_root_window (display,
					     xevent->xclient.window))
    {
      XClientMessageEvent xclient = xevent->xclient;

      xclient.window = CDK_WINDOW_XROOTWIN (win);
      XSendEvent (CDK_WINDOW_XDISPLAY (win),
		  xclient.window,
		  False,
		  SubstructureRedirectMask | SubstructureNotifyMask, (XEvent *)&xclient);

      return CDK_FILTER_REMOVE;
    }
  else if (atom == cdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_SYNC_REQUEST") &&
	   CDK_X11_DISPLAY (display)->use_sync)
    {
      CdkToplevelX11 *toplevel = _cdk_x11_window_get_toplevel (win);
      if (toplevel)
	{
#ifdef HAVE_XSYNC
	  toplevel->pending_counter_value = xevent->xclient.data.l[2] + ((gint64)xevent->xclient.data.l[3] << 32);
	  toplevel->pending_counter_value_is_extended = xevent->xclient.data.l[4] != 0;
#endif
	}
      return CDK_FILTER_REMOVE;
    }

  return CDK_FILTER_CONTINUE;
}

static void
cdk_event_init (CdkDisplay *display)
{
  CdkX11Display *display_x11;
  CdkDeviceManager *device_manager;

  display_x11 = CDK_X11_DISPLAY (display);
  display_x11->event_source = cdk_x11_event_source_new (display);

  cdk_x11_event_source_add_translator ((CdkEventSource *) display_x11->event_source,
                                       CDK_EVENT_TRANSLATOR (display));

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  device_manager = cdk_display_get_device_manager (display);
  cdk_x11_event_source_add_translator ((CdkEventSource *) display_x11->event_source,
                                        CDK_EVENT_TRANSLATOR (device_manager));
  G_GNUC_END_IGNORE_DEPRECATIONS;
}

static void
set_sm_client_id (CdkDisplay  *display,
                  const gchar *sm_client_id)
{
  CdkX11Display *display_x11 = CDK_X11_DISPLAY (display);

  if (cdk_display_is_closed (display))
    return;

  if (sm_client_id && strcmp (sm_client_id, ""))
    XChangeProperty (display_x11->xdisplay, display_x11->leader_window,
                     cdk_x11_get_xatom_by_name_for_display (display, "SM_CLIENT_ID"),
                     XA_STRING, 8, PropModeReplace, (guchar *)sm_client_id,
                     strlen (sm_client_id));
  else
    XDeleteProperty (display_x11->xdisplay, display_x11->leader_window,
                     cdk_x11_get_xatom_by_name_for_display (display, "SM_CLIENT_ID"));
}

CdkDisplay *
_cdk_x11_display_open (const gchar *display_name)
{
  Display *xdisplay;
  CdkDisplay *display;
  CdkX11Display *display_x11;
  CdkWindowAttr attr;
  gint argc;
  gchar *argv[1];

  XClassHint *class_hint;
  gint ignore;
  gint maj, min;

  xdisplay = XOpenDisplay (display_name);
  if (!xdisplay)
    return NULL;

  display = g_object_new (CDK_TYPE_X11_DISPLAY, NULL);
  display_x11 = CDK_X11_DISPLAY (display);

  display_x11->xdisplay = xdisplay;

  /* Set up handlers for Xlib internal connections */
  XAddConnectionWatch (xdisplay, cdk_internal_connection_watch, NULL);

  _cdk_x11_precache_atoms (display, precache_atoms, G_N_ELEMENTS (precache_atoms));

  /* RandR must be initialized before we initialize the screens */
  display_x11->have_randr12 = FALSE;
  display_x11->have_randr13 = FALSE;
  display_x11->have_randr15 = FALSE;
#ifdef HAVE_RANDR
  if (XRRQueryExtension (display_x11->xdisplay,
			 &display_x11->xrandr_event_base, &ignore))
  {
      int major, minor;

      XRRQueryVersion (display_x11->xdisplay, &major, &minor);

      if ((major == 1 && minor >= 2) || major > 1) {
	  display_x11->have_randr12 = TRUE;
	  if (minor >= 3 || major > 1)
	      display_x11->have_randr13 = TRUE;
#ifdef HAVE_RANDR15
	  if (minor >= 5 || major > 1)
	      display_x11->have_randr15 = TRUE;
#endif
      }

       cdk_x11_register_standard_event_type (display, display_x11->xrandr_event_base, RRNumberEvents);
  }
#endif

  /* initialize the display's screens */ 
  display_x11->screen = _cdk_x11_screen_new (display, DefaultScreen (display_x11->xdisplay));

  /* We need to initialize events after we have the screen
   * structures in places
   */
  _cdk_x11_xsettings_init (CDK_X11_SCREEN (display_x11->screen));

  display->device_manager = _cdk_x11_device_manager_new (display);

  cdk_event_init (display);

  attr.window_type = CDK_WINDOW_TOPLEVEL;
  attr.wclass = CDK_INPUT_ONLY;
  attr.x = 10;
  attr.y = 10;
  attr.width = 10;
  attr.height = 10;
  attr.event_mask = 0;

  display_x11->leader_cdk_window = cdk_window_new (CDK_X11_SCREEN (display_x11->screen)->root_window, 
						   &attr, CDK_WA_X | CDK_WA_Y);
  (_cdk_x11_window_get_toplevel (display_x11->leader_cdk_window))->is_leader = TRUE;

  display_x11->leader_window = CDK_WINDOW_XID (display_x11->leader_cdk_window);

  display_x11->leader_window_title_set = FALSE;

#ifdef HAVE_XFIXES
  if (XFixesQueryExtension (display_x11->xdisplay, 
			    &display_x11->xfixes_event_base, 
			    &ignore))
    {
      display_x11->have_xfixes = TRUE;

      cdk_x11_register_standard_event_type (display,
					    display_x11->xfixes_event_base, 
					    XFixesNumberEvents);
    }
  else
#endif
    display_x11->have_xfixes = FALSE;

#ifdef HAVE_XCOMPOSITE
  if (XCompositeQueryExtension (display_x11->xdisplay,
				&ignore, &ignore))
    {
      int major, minor;

      XCompositeQueryVersion (display_x11->xdisplay, &major, &minor);

      /* Prior to Composite version 0.4, composited windows clipped their
       * parents, so you had to use IncludeInferiors to draw to the parent
       * This isn't useful for our purposes, so require 0.4
       */
      display_x11->have_xcomposite = major > 0 || (major == 0 && minor >= 4);
    }
  else
#endif
    display_x11->have_xcomposite = FALSE;

#ifdef HAVE_XDAMAGE
  if (XDamageQueryExtension (display_x11->xdisplay,
			     &display_x11->xdamage_event_base,
			     &ignore))
    {
      display_x11->have_xdamage = TRUE;

      cdk_x11_register_standard_event_type (display,
					    display_x11->xdamage_event_base,
					    XDamageNumberEvents);
    }
  else
#endif
    display_x11->have_xdamage = FALSE;

  display_x11->have_shapes = FALSE;
  display_x11->have_input_shapes = FALSE;

  if (XShapeQueryExtension (CDK_DISPLAY_XDISPLAY (display), &display_x11->shape_event_base, &ignore))
    {
      display_x11->have_shapes = TRUE;
#ifdef ShapeInput
      if (XShapeQueryVersion (CDK_DISPLAY_XDISPLAY (display), &maj, &min))
	display_x11->have_input_shapes = (maj == 1 && min >= 1);
#endif
    }

  display_x11->trusted_client = TRUE;
  {
    Window root, child;
    int rootx, rooty, winx, winy;
    unsigned int xmask;

    cdk_x11_display_error_trap_push (display);
    XQueryPointer (display_x11->xdisplay,
		   CDK_X11_SCREEN (display_x11->screen)->xroot_window,
		   &root, &child, &rootx, &rooty, &winx, &winy, &xmask);
    if (G_UNLIKELY (cdk_x11_display_error_trap_pop (display) == BadWindow))
      {
	g_warning ("Connection to display %s appears to be untrusted. Pointer and keyboard grabs and inter-client communication may not work as expected.", cdk_display_get_name (display));
	display_x11->trusted_client = FALSE;
      }
  }

  if (g_getenv ("CDK_SYNCHRONIZE"))
    XSynchronize (display_x11->xdisplay, True);

  class_hint = XAllocClassHint();
  class_hint->res_name = (char *) g_get_prgname ();
  class_hint->res_class = (char *)cdk_get_program_class ();

  /* XmbSetWMProperties sets the RESOURCE_NAME environment variable
   * from argv[0], so we just synthesize an argument array here.
   */
  argc = 1;
  argv[0] = (char *) g_get_prgname ();

  XmbSetWMProperties (display_x11->xdisplay,
		      display_x11->leader_window,
		      NULL, NULL, argv, argc, NULL, NULL,
		      class_hint);
  XFree (class_hint);

  if (cdk_sm_client_id)
    set_sm_client_id (display, cdk_sm_client_id);

  if (!cdk_running_in_sandbox ())
    {
      /* if sandboxed, we're likely in a pid namespace and would only confuse the wm with this */
      long pid = getpid ();
      XChangeProperty (display_x11->xdisplay,
                       display_x11->leader_window,
                       cdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_PID"),
                       XA_CARDINAL, 32, PropModeReplace, (guchar *) & pid, 1);
    }

  /* We don't yet know a valid time. */
  display_x11->user_time = 0;
  
#ifdef HAVE_XKB
  {
    gint xkb_major = XkbMajorVersion;
    gint xkb_minor = XkbMinorVersion;
    if (XkbLibraryVersion (&xkb_major, &xkb_minor))
      {
        xkb_major = XkbMajorVersion;
        xkb_minor = XkbMinorVersion;
	    
        if (XkbQueryExtension (display_x11->xdisplay, 
			       NULL, &display_x11->xkb_event_type, NULL,
                               &xkb_major, &xkb_minor))
          {
	    Bool detectable_autorepeat_supported;
	    
	    display_x11->use_xkb = TRUE;

            XkbSelectEvents (display_x11->xdisplay,
                             XkbUseCoreKbd,
                             XkbNewKeyboardNotifyMask | XkbMapNotifyMask | XkbStateNotifyMask,
                             XkbNewKeyboardNotifyMask | XkbMapNotifyMask | XkbStateNotifyMask);

	    /* keep this in sync with _cdk_x11_keymap_state_changed() */
	    XkbSelectEventDetails (display_x11->xdisplay,
				   XkbUseCoreKbd, XkbStateNotify,
				   XkbAllStateComponentsMask,
                                   XkbModifierStateMask|XkbGroupStateMask);

	    XkbSetDetectableAutoRepeat (display_x11->xdisplay,
					True,
					&detectable_autorepeat_supported);

	    CDK_NOTE (MISC, g_message ("Detectable autorepeat %s.",
				       detectable_autorepeat_supported ?
				       "supported" : "not supported"));
	    
	    display_x11->have_xkb_autorepeat = detectable_autorepeat_supported;
          }
      }
  }
#endif

  display_x11->use_sync = FALSE;
#ifdef HAVE_XSYNC
  {
    int major, minor;
    int error_base, event_base;
    
    if (XSyncQueryExtension (display_x11->xdisplay,
			     &event_base, &error_base) &&
        XSyncInitialize (display_x11->xdisplay,
                         &major, &minor))
      display_x11->use_sync = TRUE;
  }
#endif

  _cdk_x11_screen_setup (display_x11->screen);

  g_signal_emit_by_name (display, "opened");

  return display;
}

/*
 * XLib internal connection handling
 */
typedef struct _CdkInternalConnection CdkInternalConnection;

struct _CdkInternalConnection
{
  gint	         fd;
  GSource	*source;
  Display	*display;
};

static gboolean
process_internal_connection (GIOChannel  *gioc,
			     GIOCondition cond,
			     gpointer     data)
{
  CdkInternalConnection *connection = (CdkInternalConnection *)data;

  cdk_threads_enter ();

  XProcessInternalConnection ((Display*)connection->display, connection->fd);

  cdk_threads_leave ();

  return TRUE;
}

static gulong
cdk_x11_display_get_next_serial (CdkDisplay *display)
{
  return NextRequest (CDK_DISPLAY_XDISPLAY (display));
}


static CdkInternalConnection *
cdk_add_connection_handler (Display *display,
			    guint    fd)
{
  GIOChannel *io_channel;
  CdkInternalConnection *connection;

  connection = g_new (CdkInternalConnection, 1);

  connection->fd = fd;
  connection->display = display;
  
  io_channel = g_io_channel_unix_new (fd);
  
  connection->source = g_io_create_watch (io_channel, G_IO_IN);
  g_source_set_callback (connection->source,
			 (GSourceFunc)process_internal_connection, connection, NULL);
  g_source_attach (connection->source, NULL);
  
  g_io_channel_unref (io_channel);
  
  return connection;
}

static void
cdk_remove_connection_handler (CdkInternalConnection *connection)
{
  g_source_destroy (connection->source);
  g_free (connection);
}

static void
cdk_internal_connection_watch (Display  *display,
			       XPointer  arg,
			       gint      fd,
			       gboolean  opening,
			       XPointer *watch_data)
{
  if (opening)
    *watch_data = (XPointer)cdk_add_connection_handler (display, fd);
  else
    cdk_remove_connection_handler ((CdkInternalConnection *)*watch_data);
}

static const gchar *
cdk_x11_display_get_name (CdkDisplay *display)
{
  return (gchar *) DisplayString (CDK_X11_DISPLAY (display)->xdisplay);
}

static CdkScreen *
cdk_x11_display_get_default_screen (CdkDisplay *display)
{
  return CDK_X11_DISPLAY (display)->screen;
}

gboolean
_cdk_x11_display_is_root_window (CdkDisplay *display,
				 Window      xroot_window)
{
  CdkX11Display *display_x11;

  display_x11 = CDK_X11_DISPLAY (display);

  return CDK_SCREEN_XROOTWIN (display_x11->screen) == xroot_window;
}

struct XPointerUngrabInfo {
  CdkDisplay *display;
  guint32 time;
};

static void
device_grab_update_callback (CdkDisplay *display,
                             gpointer    data,
                             gulong      serial)
{
  CdkPointerWindowInfo *pointer_info;
  CdkDevice *device = data;

  pointer_info = _cdk_display_get_pointer_info (display, device);
  _cdk_display_device_grab_update (display, device,
                                   pointer_info->last_slave ? pointer_info->last_slave : device,
                                   serial);
}

#define XSERVER_TIME_IS_LATER(time1, time2)                        \
  ( (( time1 > time2 ) && ( time1 - time2 < ((guint32)-1)/2 )) ||  \
    (( time1 < time2 ) && ( time2 - time1 > ((guint32)-1)/2 ))     \
  )

void
_cdk_x11_display_update_grab_info (CdkDisplay *display,
                                   CdkDevice  *device,
                                   gint        status)
{
  if (status == GrabSuccess)
    _cdk_x11_roundtrip_async (display, device_grab_update_callback, device);
}

void
_cdk_x11_display_update_grab_info_ungrab (CdkDisplay *display,
                                          CdkDevice  *device,
                                          guint32     time,
                                          gulong      serial)
{
  CdkDeviceGrabInfo *grab;

  XFlush (CDK_DISPLAY_XDISPLAY (display));

  grab = _cdk_display_get_last_device_grab (display, device);
  if (grab &&
      (time == CDK_CURRENT_TIME ||
       grab->time == CDK_CURRENT_TIME ||
       !XSERVER_TIME_IS_LATER (grab->time, time)))
    {
      grab->serial_end = serial;
      _cdk_x11_roundtrip_async (display, device_grab_update_callback, device);
    }
}

static void
cdk_x11_display_beep (CdkDisplay *display)
{
#ifdef HAVE_XKB
  XkbBell (CDK_DISPLAY_XDISPLAY (display), None, 0, None);
#else
  XBell (CDK_DISPLAY_XDISPLAY (display), 0);
#endif
}

static void
cdk_x11_display_sync (CdkDisplay *display)
{
  XSync (CDK_DISPLAY_XDISPLAY (display), False);
}

static void
cdk_x11_display_flush (CdkDisplay *display)
{
  if (!display->closed)
    XFlush (CDK_DISPLAY_XDISPLAY (display));
}

static gboolean
cdk_x11_display_has_pending (CdkDisplay *display)
{
  return XPending (CDK_DISPLAY_XDISPLAY (display));
}

static CdkWindow *
cdk_x11_display_get_default_group (CdkDisplay *display)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), NULL);

  return CDK_X11_DISPLAY (display)->leader_cdk_window;
}

/**
 * cdk_x11_display_grab:
 * @display: (type CdkX11Display): a #CdkDisplay 
 * 
 * Call XGrabServer() on @display. 
 * To ungrab the display again, use cdk_x11_display_ungrab(). 
 *
 * cdk_x11_display_grab()/cdk_x11_display_ungrab() calls can be nested.
 *
 * Since: 2.2
 **/
void
cdk_x11_display_grab (CdkDisplay *display)
{
  CdkX11Display *display_x11;
  
  g_return_if_fail (CDK_IS_DISPLAY (display));
  
  display_x11 = CDK_X11_DISPLAY (display);
  
  if (display_x11->grab_count == 0)
    XGrabServer (display_x11->xdisplay);
  display_x11->grab_count++;
}

/**
 * cdk_x11_display_ungrab:
 * @display: (type CdkX11Display): a #CdkDisplay
 * 
 * Ungrab @display after it has been grabbed with 
 * cdk_x11_display_grab(). 
 *
 * Since: 2.2
 **/
void
cdk_x11_display_ungrab (CdkDisplay *display)
{
  CdkX11Display *display_x11;
  
  g_return_if_fail (CDK_IS_DISPLAY (display));
  
  display_x11 = CDK_X11_DISPLAY (display);;
  g_return_if_fail (display_x11->grab_count > 0);
  
  display_x11->grab_count--;
  if (display_x11->grab_count == 0)
    {
      XUngrabServer (display_x11->xdisplay);
      XFlush (display_x11->xdisplay);
    }
}

static void
cdk_x11_display_dispose (GObject *object)
{
  CdkX11Display *display_x11 = CDK_X11_DISPLAY (object);

  _cdk_screen_close (display_x11->screen);

  if (display_x11->event_source)
    {
      g_source_destroy (display_x11->event_source);
      g_source_unref (display_x11->event_source);
      display_x11->event_source = NULL;
    }

  G_OBJECT_CLASS (cdk_x11_display_parent_class)->dispose (object);
}

static void
cdk_x11_display_finalize (GObject *object)
{
  CdkX11Display *display_x11 = CDK_X11_DISPLAY (object);

  /* Keymap */
  if (display_x11->keymap)
    g_object_unref (display_x11->keymap);

  _cdk_x11_cursor_display_finalize (CDK_DISPLAY (display_x11));

  /* Empty the event queue */
  _cdk_x11_display_free_translate_queue (CDK_DISPLAY (display_x11));

  /* Atom Hashtable */
  g_hash_table_destroy (display_x11->atom_from_virtual);
  g_hash_table_destroy (display_x11->atom_to_virtual);

  /* Leader Window */
  XDestroyWindow (display_x11->xdisplay, display_x11->leader_window);

  /* List of event window extraction functions */
  g_slist_free_full (display_x11->event_types, g_free);

  /* input CdkWindow list */
  g_list_free_full (display_x11->input_windows, g_free);

  /* Free all CdkScreens */
  g_object_unref (display_x11->screen);
  g_list_free_full (display_x11->screens, g_object_unref);

  g_ptr_array_free (display_x11->monitors, TRUE);

  g_free (display_x11->startup_notification_id);

  /* X ID hashtable */
  g_hash_table_destroy (display_x11->xid_ht);

  XCloseDisplay (display_x11->xdisplay);

  /* error traps */
  while (display_x11->error_traps != NULL)
    {
      CdkErrorTrap *trap = display_x11->error_traps->data;

      display_x11->error_traps =
        g_slist_delete_link (display_x11->error_traps,
                             display_x11->error_traps);

      if (trap->end_sequence == 0)
        g_warning ("Display finalized with an unpopped error trap");

      g_slice_free (CdkErrorTrap, trap);
    }

  G_OBJECT_CLASS (cdk_x11_display_parent_class)->finalize (object);
}

/**
 * cdk_x11_lookup_xdisplay:
 * @xdisplay: a pointer to an X Display
 * 
 * Find the #CdkDisplay corresponding to @xdisplay, if any exists.
 * 
 * Returns: (transfer none) (type CdkX11Display): the #CdkDisplay, if found, otherwise %NULL.
 *
 * Since: 2.2
 **/
CdkDisplay *
cdk_x11_lookup_xdisplay (Display *xdisplay)
{
  GSList *list, *l;
  CdkDisplay *display;

  display = NULL;

  list = cdk_display_manager_list_displays (cdk_display_manager_get ());

  for (l = list; l; l = l->next)
    {
      if (CDK_IS_X11_DISPLAY (l->data) &&
          CDK_DISPLAY_XDISPLAY (l->data) == xdisplay)
        {
          display = l->data;
          break;
        }
    }

  g_slist_free (list);

  return display;
}

/**
 * _cdk_x11_display_screen_for_xrootwin:
 * @display: a #CdkDisplay
 * @xrootwin: window ID for one of of the screen’s of the display.
 * 
 * Given the root window ID of one of the screen’s of a #CdkDisplay,
 * finds the screen.
 * 
 * Returns: (transfer none): the #CdkScreen corresponding to
 *     @xrootwin, or %NULL.
 **/
CdkScreen *
_cdk_x11_display_screen_for_xrootwin (CdkDisplay *display,
				      Window      xrootwin)
{
  CdkScreen *screen;
  XWindowAttributes attrs;
  gboolean result;
  CdkX11Display *display_x11;
  GList *l;

  screen = cdk_display_get_default_screen (display);

  if (CDK_SCREEN_XROOTWIN (screen) == xrootwin)
    return screen;

  display_x11 = CDK_X11_DISPLAY (display);

  for (l = display_x11->screens; l; l = l->next)
    {
      screen = l->data;
      if (CDK_SCREEN_XROOTWIN (screen) == xrootwin)
        return screen;
    }

  cdk_x11_display_error_trap_push (display);
  result = XGetWindowAttributes (display_x11->xdisplay, xrootwin, &attrs);
  if (cdk_x11_display_error_trap_pop (display) || !result)
    return NULL;

  screen = _cdk_x11_screen_new (display, XScreenNumberOfScreen (attrs.screen));

  display_x11->screens = g_list_prepend (display_x11->screens, screen);

  return screen;
}

/**
 * cdk_x11_display_get_xdisplay:
 * @display: (type CdkX11Display): a #CdkDisplay
 *
 * Returns the X display of a #CdkDisplay.
 *
 * Returns: (transfer none): an X display
 *
 * Since: 2.2
 */
Display *
cdk_x11_display_get_xdisplay (CdkDisplay *display)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), NULL);
  return CDK_X11_DISPLAY (display)->xdisplay;
}

static void
cdk_x11_display_make_default (CdkDisplay *display)
{
  CdkX11Display *display_x11 = CDK_X11_DISPLAY (display);
  const gchar *startup_id;

  g_free (display_x11->startup_notification_id);
  display_x11->startup_notification_id = NULL;

  startup_id = cdk_get_desktop_startup_id ();
  if (startup_id)
    cdk_x11_display_set_startup_notification_id (display, startup_id);
}

static void
broadcast_xmessage (CdkDisplay *display,
		    const char *message_type,
		    const char *message_type_begin,
		    const char *message)
{
  Display *xdisplay = CDK_DISPLAY_XDISPLAY (display);
  CdkScreen *screen = cdk_display_get_default_screen (display);
  CdkWindow *root_window = cdk_screen_get_root_window (screen);
  Window xroot_window = CDK_WINDOW_XID (root_window);
  
  Atom type_atom;
  Atom type_atom_begin;
  Window xwindow;

  if (!G_LIKELY (CDK_X11_DISPLAY (display)->trusted_client))
    return;

  {
    XSetWindowAttributes attrs;

    attrs.override_redirect = True;
    attrs.event_mask = PropertyChangeMask | StructureNotifyMask;

    xwindow =
      XCreateWindow (xdisplay,
                     xroot_window,
                     -100, -100, 1, 1,
                     0,
                     CopyFromParent,
                     CopyFromParent,
                     (Visual *)CopyFromParent,
                     CWOverrideRedirect | CWEventMask,
                     &attrs);
  }

  type_atom = cdk_x11_get_xatom_by_name_for_display (display,
                                                     message_type);
  type_atom_begin = cdk_x11_get_xatom_by_name_for_display (display,
                                                           message_type_begin);
  
  {
    XClientMessageEvent xclient;
    const char *src;
    const char *src_end;
    char *dest;
    char *dest_end;
    
		memset(&xclient, 0, sizeof (xclient));
    xclient.type = ClientMessage;
    xclient.message_type = type_atom_begin;
    xclient.display =xdisplay;
    xclient.window = xwindow;
    xclient.format = 8;

    src = message;
    src_end = message + strlen (message) + 1; /* +1 to include nul byte */
    
    while (src != src_end)
      {
        dest = &xclient.data.b[0];
        dest_end = dest + 20;        
        
        while (dest != dest_end &&
               src != src_end)
          {
            *dest = *src;
            ++dest;
            ++src;
          }

	while (dest != dest_end)
	  {
	    *dest = 0;
	    ++dest;
	  }
        
        XSendEvent (xdisplay,
                    xroot_window,
                    False,
                    PropertyChangeMask,
                    (XEvent *)&xclient);

        xclient.message_type = type_atom;
      }
  }

  XDestroyWindow (xdisplay, xwindow);
  XFlush (xdisplay);
}

/**
 * cdk_x11_display_broadcast_startup_message:
 * @display: (type CdkX11Display): a #CdkDisplay
 * @message_type: startup notification message type ("new", "change",
 * or "remove")
 * @...: a list of key/value pairs (as strings), terminated by a
 * %NULL key. (A %NULL value for a key will cause that key to be
 * skipped in the output.)
 *
 * Sends a startup notification message of type @message_type to
 * @display. 
 *
 * This is a convenience function for use by code that implements the
 * freedesktop startup notification specification. Applications should
 * not normally need to call it directly. See the
 * [Startup Notification Protocol specification](http://standards.freedesktop.org/startup-notification-spec/startup-notification-latest.txt)
 * for definitions of the message types and keys that can be used.
 *
 * Since: 2.12
 **/
void
cdk_x11_display_broadcast_startup_message (CdkDisplay *display,
					   const char *message_type,
					   ...)
{
  GString *message;
  va_list ap;
  const char *key, *value, *p;

  message = g_string_new (message_type);
  g_string_append_c (message, ':');

  va_start (ap, message_type);
  while ((key = va_arg (ap, const char *)))
    {
      value = va_arg (ap, const char *);
      if (!value)
	continue;

      g_string_append_printf (message, " %s=\"", key);
      for (p = value; *p; p++)
	{
	  switch (*p)
	    {
	    case ' ':
	    case '"':
	    case '\\':
	      g_string_append_c (message, '\\');
	      break;
	    }

	  g_string_append_c (message, *p);
	}
      g_string_append_c (message, '\"');
    }
  va_end (ap);

  broadcast_xmessage (display,
                      "_NET_STARTUP_INFO",
                      "_NET_STARTUP_INFO_BEGIN",
                      message->str);

  g_string_free (message, TRUE);
}

static void
cdk_x11_display_notify_startup_complete (CdkDisplay  *display,
                                         const gchar *startup_id)
{
  gchar *free_this = NULL;

  if (startup_id == NULL)
    {
      CdkX11Display *display_x11 = CDK_X11_DISPLAY (display);

      startup_id = free_this = display_x11->startup_notification_id;
      display_x11->startup_notification_id = NULL;

      if (startup_id == NULL)
        return;
    }

  cdk_x11_display_broadcast_startup_message (display, "remove",
                                             "ID", startup_id,
                                             NULL);

  g_free (free_this);
}

static gboolean
cdk_x11_display_supports_selection_notification (CdkDisplay *display)
{
  CdkX11Display *display_x11 = CDK_X11_DISPLAY (display);

  return display_x11->have_xfixes;
}

static gboolean
cdk_x11_display_request_selection_notification (CdkDisplay *display,
						CdkAtom     selection)

{
#ifdef HAVE_XFIXES
  CdkX11Display *display_x11 = CDK_X11_DISPLAY (display);
  Atom atom;

  if (display_x11->have_xfixes)
    {
      atom = cdk_x11_atom_to_xatom_for_display (display, 
						selection);
      XFixesSelectSelectionInput (display_x11->xdisplay, 
				  display_x11->leader_window,
				  atom,
				  XFixesSetSelectionOwnerNotifyMask |
				  XFixesSelectionWindowDestroyNotifyMask |
				  XFixesSelectionClientCloseNotifyMask);
      return TRUE;
    }
  else
#endif
    return FALSE;
}

static gboolean
cdk_x11_display_supports_clipboard_persistence (CdkDisplay *display)
{
  Atom clipboard_manager;

  /* It might make sense to cache this */
  clipboard_manager = cdk_x11_get_xatom_by_name_for_display (display, "CLIPBOARD_MANAGER");
  return XGetSelectionOwner (CDK_X11_DISPLAY (display)->xdisplay, clipboard_manager) != None;
}

static void
cdk_x11_display_store_clipboard (CdkDisplay    *display,
				 CdkWindow     *clipboard_window,
				 guint32        time_,
				 const CdkAtom *targets,
				 gint           n_targets)
{
  CdkX11Display *display_x11 = CDK_X11_DISPLAY (display);
  Atom clipboard_manager, save_targets;

  g_return_if_fail (CDK_WINDOW_IS_X11 (clipboard_window));

  clipboard_manager = cdk_x11_get_xatom_by_name_for_display (display, "CLIPBOARD_MANAGER");
  save_targets = cdk_x11_get_xatom_by_name_for_display (display, "SAVE_TARGETS");

  cdk_x11_display_error_trap_push (display);

  if (XGetSelectionOwner (display_x11->xdisplay, clipboard_manager) != None)
    {
      Atom property_name = None;
      Atom *xatoms;
      int i;

      if (n_targets > 0)
        {
          property_name = cdk_x11_get_xatom_by_name_for_display (display, "CDK_SELECTION");

	  xatoms = g_new (Atom, n_targets);
	  for (i = 0; i < n_targets; i++)
	    xatoms[i] = cdk_x11_atom_to_xatom_for_display (display, targets[i]);

	  XChangeProperty (display_x11->xdisplay, CDK_WINDOW_XID (clipboard_window),
			   property_name, XA_ATOM,
			   32, PropModeReplace, (guchar *)xatoms, n_targets);
	  g_free (xatoms);

	}

      XConvertSelection (display_x11->xdisplay,
                         clipboard_manager, save_targets, property_name,
                         CDK_WINDOW_XID (clipboard_window), time_);

    }
  cdk_x11_display_error_trap_pop_ignored (display);

}

/**
 * cdk_x11_display_get_user_time:
 * @display: (type CdkX11Display): a #CdkDisplay
 *
 * Returns the timestamp of the last user interaction on 
 * @display. The timestamp is taken from events caused
 * by user interaction such as key presses or pointer 
 * movements. See cdk_x11_window_set_user_time().
 *
 * Returns: the timestamp of the last user interaction 
 *
 * Since: 2.8
 */
guint32
cdk_x11_display_get_user_time (CdkDisplay *display)
{
  return CDK_X11_DISPLAY (display)->user_time;
}

static gboolean
cdk_x11_display_supports_shapes (CdkDisplay *display)
{
  return CDK_X11_DISPLAY (display)->have_shapes;
}

static gboolean
cdk_x11_display_supports_input_shapes (CdkDisplay *display)
{
  return CDK_X11_DISPLAY (display)->have_input_shapes;
}


/**
 * cdk_x11_display_get_startup_notification_id:
 * @display: (type CdkX11Display): a #CdkDisplay
 *
 * Gets the startup notification ID for a display.
 * 
 * Returns: the startup notification ID for @display
 *
 * Since: 2.12
 */
const gchar *
cdk_x11_display_get_startup_notification_id (CdkDisplay *display)
{
  return CDK_X11_DISPLAY (display)->startup_notification_id;
}

/**
 * cdk_x11_display_set_startup_notification_id:
 * @display: (type CdkX11Display): a #CdkDisplay
 * @startup_id: the startup notification ID (must be valid utf8)
 *
 * Sets the startup notification ID for a display.
 *
 * This is usually taken from the value of the DESKTOP_STARTUP_ID
 * environment variable, but in some cases (such as the application not
 * being launched using exec()) it can come from other sources.
 *
 * If the ID contains the string "_TIME" then the portion following that
 * string is taken to be the X11 timestamp of the event that triggered
 * the application to be launched and the CDK current event time is set
 * accordingly.
 *
 * The startup ID is also what is used to signal that the startup is
 * complete (for example, when opening a window or when calling
 * cdk_notify_startup_complete()).
 *
 * Since: 3.0
 **/
void
cdk_x11_display_set_startup_notification_id (CdkDisplay  *display,
                                             const gchar *startup_id)
{
  CdkX11Display *display_x11;
  gchar *time_str;

  display_x11 = CDK_X11_DISPLAY (display);

  g_free (display_x11->startup_notification_id);
  display_x11->startup_notification_id = g_strdup (startup_id);

  if (startup_id != NULL)
    {
      /* Find the launch time from the startup_id, if it's there.  Newer spec
       * states that the startup_id is of the form <unique>_TIME<timestamp>
       */
      time_str = g_strrstr (startup_id, "_TIME");
      if (time_str != NULL)
        {
          gulong retval;
          gchar *end;
          errno = 0;

          /* Skip past the "_TIME" part */
          time_str += 5;

          retval = strtoul (time_str, &end, 0);
          if (end != time_str && errno == 0)
            display_x11->user_time = retval;
        }
      else
        display_x11->user_time = 0;

      /* Set the startup id on the leader window so it
       * applies to all windows we create on this display
       */
      XChangeProperty (display_x11->xdisplay,
                       display_x11->leader_window,
                       cdk_x11_get_xatom_by_name_for_display (display, "_NET_STARTUP_ID"),
                       cdk_x11_get_xatom_by_name_for_display (display, "UTF8_STRING"), 8,
                       PropModeReplace,
                       (guchar *)startup_id, strlen (startup_id));
    }
  else
    {
      XDeleteProperty (display_x11->xdisplay, display_x11->leader_window,
                       cdk_x11_get_xatom_by_name_for_display (display, "_NET_STARTUP_ID"));
      display_x11->user_time = 0;
    }
}

static gboolean
cdk_x11_display_supports_composite (CdkDisplay *display)
{
  CdkX11Display *x11_display = CDK_X11_DISPLAY (display);

  return x11_display->have_xcomposite &&
	 x11_display->have_xdamage &&
	 x11_display->have_xfixes;
}

/**
 * cdk_x11_register_standard_event_type:
 * @display: (type CdkX11Display): a #CdkDisplay
 * @event_base: first event type code to register
 * @n_events: number of event type codes to register
 *
 * Registers interest in receiving extension events with type codes
 * between @event_base and `event_base + n_events - 1`.
 * The registered events must have the window field in the same place
 * as core X events (this is not the case for e.g. XKB extension events).
 *
 * If an event type is registered, events of this type will go through
 * global and window-specific filters (see cdk_window_add_filter()).
 * Unregistered events will only go through global filters.
 * CDK may register the events of some X extensions on its own.
 *
 * This function should only be needed in unusual circumstances, e.g.
 * when filtering XInput extension events on the root window.
 *
 * Since: 2.4
 **/
void
cdk_x11_register_standard_event_type (CdkDisplay *display,
				      gint        event_base,
				      gint        n_events)
{
  CdkEventTypeX11 *event_type;
  CdkX11Display *display_x11;

  display_x11 = CDK_X11_DISPLAY (display);
  event_type = g_new (CdkEventTypeX11, 1);

  event_type->base = event_base;
  event_type->n_events = n_events;

  display_x11->event_types = g_slist_prepend (display_x11->event_types, event_type);
}

/* look up the extension name for a given major opcode.  grubs around in
 * xlib to do it since a) it’s already cached there b) XQueryExtension
 * emits protocol so we can’t use it in an error handler.
 */
static const char *
_cdk_x11_decode_request_code(Display *dpy, int code)
{
  _XExtension *ext;

  if (code < 128)
    return "core protocol";

  for (ext = dpy->ext_procs; ext; ext = ext->next)
    {
      if (ext->codes.major_opcode == code)
        return ext->name;
    }

  return "unknown";
}

/* compare X sequence numbers handling wraparound */
#define SEQUENCE_COMPARE(a,op,b) (((long) (a) - (long) (b)) op 0)

/* delivers an error event from the error handler in cdkmain-x11.c */
void
_cdk_x11_display_error_event (CdkDisplay  *display,
                              XErrorEvent *error)
{
  CdkX11Display *display_x11;
  GSList *tmp_list;
  gboolean ignore;

  display_x11 = CDK_X11_DISPLAY (display);

  ignore = FALSE;
  for (tmp_list = display_x11->error_traps;
       tmp_list != NULL;
       tmp_list = tmp_list->next)
    {
      CdkErrorTrap *trap;

      trap = tmp_list->data;

      if (SEQUENCE_COMPARE (trap->start_sequence, <=, error->serial) &&
          (trap->end_sequence == 0 ||
           SEQUENCE_COMPARE (trap->end_sequence, >, error->serial)))
        {
          ignore = TRUE;
          trap->error_code = error->error_code;
          break; /* only innermost trap gets the error code */
        }
    }

  if (!ignore)
    {
      gchar buf[64];
      gchar *msg;

      XGetErrorText (display_x11->xdisplay, error->error_code, buf, 63);

      msg =
        g_strdup_printf ("The program '%s' received an X Window System error.\n"
                         "This probably reflects a bug in the program.\n"
                         "The error was '%s'.\n"
                         "  (Details: serial %ld error_code %d request_code %d (%s) minor_code %d)\n"
                         "  (Note to programmers: normally, X errors are reported asynchronously;\n"
                         "   that is, you will receive the error a while after causing it.\n"
                         "   To debug your program, run it with the CDK_SYNCHRONIZE environment\n"
                         "   variable to change this behavior. You can then get a meaningful\n"
                         "   backtrace from your debugger if you break on the cdk_x_error() function.)",
                         g_get_prgname (),
                         buf,
                         error->serial,
                         error->error_code,
                         error->request_code,
                         _cdk_x11_decode_request_code(display_x11->xdisplay,
                                                      error->request_code),
                         error->minor_code);

#ifdef G_ENABLE_DEBUG
      g_error ("%s", msg);
#else /* !G_ENABLE_DEBUG */
      g_warning ("%s", msg);

      _exit (1);
#endif /* G_ENABLE_DEBUG */
    }
}

static void
delete_outdated_error_traps (CdkX11Display *display_x11)
{
  GSList *tmp_list;
  gulong processed_sequence;

  processed_sequence = XLastKnownRequestProcessed (display_x11->xdisplay);

  tmp_list = display_x11->error_traps;
  while (tmp_list != NULL)
    {
      CdkErrorTrap *trap = tmp_list->data;

      if (trap->end_sequence != 0 &&
          SEQUENCE_COMPARE (trap->end_sequence, <=, processed_sequence))
        {
          GSList *free_me = tmp_list;

          tmp_list = tmp_list->next;
          display_x11->error_traps =
            g_slist_delete_link (display_x11->error_traps, free_me);
          g_slice_free (CdkErrorTrap, trap);
        }
      else
        {
          tmp_list = tmp_list->next;
        }
    }
}

/**
 * cdk_x11_display_error_trap_push:
 * @display: (type CdkX11Display): a #CdkDisplay
 *
 * Begins a range of X requests on @display for which X error events
 * will be ignored. Unignored errors (when no trap is pushed) will abort
 * the application. Use cdk_x11_display_error_trap_pop() or
 * cdk_x11_display_error_trap_pop_ignored()to lift a trap pushed
 * with this function.
 *
 * See also cdk_error_trap_push() to push a trap on all displays.
 *
 * Since: 3.0
 */
void
cdk_x11_display_error_trap_push (CdkDisplay *display)
{
  CdkX11Display *display_x11;
  CdkErrorTrap *trap;

  display_x11 = CDK_X11_DISPLAY (display);

  delete_outdated_error_traps (display_x11);

  /* set up the Xlib callback to tell us about errors */
  _cdk_x11_error_handler_push ();

  trap = g_slice_new0 (CdkErrorTrap);

  trap->start_sequence = XNextRequest (display_x11->xdisplay);
  trap->error_code = Success;

  display_x11->error_traps =
    g_slist_prepend (display_x11->error_traps, trap);
}

static gint
cdk_x11_display_error_trap_pop_internal (CdkDisplay *display,
                                         gboolean    need_code)
{
  CdkX11Display *display_x11;
  CdkErrorTrap *trap;
  GSList *tmp_list;
  int result;

  display_x11 = CDK_X11_DISPLAY (display);

  g_return_val_if_fail (display_x11->error_traps != NULL, Success);

  /* Find the first trap that hasn't been popped already */
  trap = NULL; /* quiet gcc */
  for (tmp_list = display_x11->error_traps;
       tmp_list != NULL;
       tmp_list = tmp_list->next)
    {
      trap = tmp_list->data;

      if (trap->end_sequence == 0)
        break;
    }

  g_return_val_if_fail (trap != NULL, Success);
  g_assert (trap->end_sequence == 0);

  /* May need to sync to fill in trap->error_code if we care about
   * getting an error code.
   */
  if (need_code)
    {
      gulong processed_sequence;
      gulong next_sequence;

      next_sequence = XNextRequest (display_x11->xdisplay);
      processed_sequence = XLastKnownRequestProcessed (display_x11->xdisplay);

      /* If our last request was already processed, there is no point
       * in syncing. i.e. if last request was a round trip (or even if
       * we got an event with the serial of a non-round-trip)
       */
      if ((next_sequence - 1) != processed_sequence)
        {
          XSync (display_x11->xdisplay, False);
        }

      result = trap->error_code;
    }
  else
    {
      result = Success;
    }

  /* record end of trap, giving us a range of
   * error sequences we'll ignore.
   */
  trap->end_sequence = XNextRequest (display_x11->xdisplay);

  /* remove the Xlib callback */
  _cdk_x11_error_handler_pop ();

  /* we may already be outdated */
  delete_outdated_error_traps (display_x11);

  return result;
}

/**
 * cdk_x11_display_set_window_scale:
 * @display: (type CdkX11Display): the display
 * @scale: The new scale value
 *
 * Forces a specific window scale for all windows on this display,
 * instead of using the default or user configured scale. This
 * is can be used to disable scaling support by setting @scale to
 * 1, or to programmatically set the window scale.
 *
 * Once the scale is set by this call it will not change in response
 * to later user configuration changes.
 *
 * Since: 3.10
 */
void
cdk_x11_display_set_window_scale (CdkDisplay *display,
                                  gint        scale)
{
  CdkX11Screen *x11_screen;
  gboolean need_reread_settings = FALSE;

  g_return_if_fail (CDK_IS_X11_DISPLAY (display));

  scale = MAX (scale, 1);

  x11_screen = CDK_X11_SCREEN (CDK_X11_DISPLAY (display)->screen);

  if (!x11_screen->fixed_window_scale)
    {
      x11_screen->fixed_window_scale = TRUE;

      /* We treat screens with a window scale set differently when
       * reading xsettings, so we need to reread
       */
      need_reread_settings = TRUE;
    }

  _cdk_x11_screen_set_window_scale (x11_screen, scale);

  if (need_reread_settings)
    _cdk_x11_settings_force_reread (x11_screen);
}


/**
 * cdk_x11_display_error_trap_pop:
 * @display: (type CdkX11Display): the display
 *
 * Pops the error trap pushed by cdk_x11_display_error_trap_push().
 * Will XSync() if necessary and will always block until
 * the error is known to have occurred or not occurred,
 * so the error code can be returned.
 *
 * If you don’t need to use the return value,
 * cdk_x11_display_error_trap_pop_ignored() would be more efficient.
 *
 * See cdk_error_trap_pop() for the all-displays-at-once
 * equivalent.
 *
 * Since: 3.0
 *
 * Returns: X error code or 0 on success
 */
gint
cdk_x11_display_error_trap_pop (CdkDisplay *display)
{
  g_return_val_if_fail (CDK_IS_X11_DISPLAY (display), Success);

  return cdk_x11_display_error_trap_pop_internal (display, TRUE);
}

/**
 * cdk_x11_display_error_trap_pop_ignored:
 * @display: (type CdkX11Display): the display
 *
 * Pops the error trap pushed by cdk_x11_display_error_trap_push().
 * Does not block to see if an error occurred; merely records the
 * range of requests to ignore errors for, and ignores those errors
 * if they arrive asynchronously.
 *
 * See cdk_error_trap_pop_ignored() for the all-displays-at-once
 * equivalent.
 *
 * Since: 3.0
 */
void
cdk_x11_display_error_trap_pop_ignored (CdkDisplay *display)
{
  g_return_if_fail (CDK_IS_X11_DISPLAY (display));

  cdk_x11_display_error_trap_pop_internal (display, FALSE);
}

/**
 * cdk_x11_set_sm_client_id:
 * @sm_client_id: (nullable): the client id assigned by the session manager
 *    when the connection was opened, or %NULL to remove the property.
 *
 * Sets the `SM_CLIENT_ID` property on the application’s leader window so that
 * the window manager can save the application’s state using the X11R6 ICCCM
 * session management protocol.
 *
 * See the X Session Management Library documentation for more information on
 * session management and the Inter-Client Communication Conventions Manual
 *
 * Since: 2.24
 */
void
cdk_x11_set_sm_client_id (const gchar *sm_client_id)
{
 GSList *displays, *l;

  g_free (cdk_sm_client_id);
  cdk_sm_client_id = g_strdup (sm_client_id);

  displays = cdk_display_manager_list_displays (cdk_display_manager_get ());
  for (l = displays; l; l = l->next)
    {
      if (CDK_IS_X11_DISPLAY (l->data))
        set_sm_client_id (l->data, sm_client_id);
    }

  g_slist_free (displays);
}

static gint
pop_error_trap (CdkDisplay *display,
                gboolean    ignored)
{
  if (ignored)
    {
      cdk_x11_display_error_trap_pop_ignored (display);
      return Success;
    }
  else
    {
      return cdk_x11_display_error_trap_pop (display);
    }
}

static CdkKeymap *
cdk_x11_display_get_keymap (CdkDisplay *display)
{
  CdkX11Display *display_x11;
  g_return_val_if_fail (CDK_IS_DISPLAY (display), NULL);
  display_x11 = CDK_X11_DISPLAY (display);

  if (!display_x11->keymap)
    display_x11->keymap = g_object_new (CDK_TYPE_X11_KEYMAP, NULL);

  display_x11->keymap->display = display;

  return display_x11->keymap;
}

static CdkSeat *
cdk_x11_display_get_default_seat (CdkDisplay *display)
{
  GList *seats, *l;
  int device_id;
  gboolean result = FALSE;

  seats = cdk_display_list_seats (display);

  /* Shortcut only one seat being available.
   * This path always triggers for core events, so we can freely use XInput below. */
  if (g_list_length (seats) == 1)
    {
      CdkSeat *seat = seats->data;

      g_list_free (seats);
      return seat;
    }

  cdk_x11_display_error_trap_push (display);
  result = XIGetClientPointer (CDK_DISPLAY_XDISPLAY (display),
                               None, &device_id);
  cdk_x11_display_error_trap_pop_ignored (display);

  for (l = seats; l; l = l->next)
    {
      CdkDevice *pointer;

      pointer = cdk_seat_get_pointer (l->data);

      if (cdk_x11_device_get_id (pointer) == device_id || !result)
        {
          CdkSeat *seat = l->data;
          g_list_free (seats);

          return seat;
        }
    }

  g_list_free (seats);

  return NULL;
}

static int
cdk_x11_display_get_n_monitors (CdkDisplay *display)
{
  CdkX11Display *x11_display = CDK_X11_DISPLAY (display);

  return x11_display->monitors->len;
}


static CdkMonitor *
cdk_x11_display_get_monitor (CdkDisplay *display,
                             int         monitor_num)
{
  CdkX11Display *x11_display = CDK_X11_DISPLAY (display);

  if (0 <= monitor_num && monitor_num < x11_display->monitors->len)
    return (CdkMonitor *)x11_display->monitors->pdata[monitor_num];

  return NULL;
}

static CdkMonitor *
cdk_x11_display_get_primary_monitor (CdkDisplay *display)
{
  CdkX11Display *x11_display = CDK_X11_DISPLAY (display);

  if (0 <= x11_display->primary_monitor && x11_display->primary_monitor < x11_display->monitors->len)
    return x11_display->monitors->pdata[x11_display->primary_monitor];

  return NULL;
}

static void
cdk_x11_display_class_init (CdkX11DisplayClass * class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  CdkDisplayClass *display_class = CDK_DISPLAY_CLASS (class);

  object_class->dispose = cdk_x11_display_dispose;
  object_class->finalize = cdk_x11_display_finalize;

  display_class->window_type = CDK_TYPE_X11_WINDOW;

  display_class->get_name = cdk_x11_display_get_name;
  display_class->get_default_screen = cdk_x11_display_get_default_screen;
  display_class->beep = cdk_x11_display_beep;
  display_class->sync = cdk_x11_display_sync;
  display_class->flush = cdk_x11_display_flush;
  display_class->make_default = cdk_x11_display_make_default;
  display_class->has_pending = cdk_x11_display_has_pending;
  display_class->queue_events = _cdk_x11_display_queue_events;
  display_class->get_default_group = cdk_x11_display_get_default_group;
  display_class->supports_selection_notification = cdk_x11_display_supports_selection_notification;
  display_class->request_selection_notification = cdk_x11_display_request_selection_notification;
  display_class->supports_clipboard_persistence = cdk_x11_display_supports_clipboard_persistence;
  display_class->store_clipboard = cdk_x11_display_store_clipboard;
  display_class->supports_shapes = cdk_x11_display_supports_shapes;
  display_class->supports_input_shapes = cdk_x11_display_supports_input_shapes;
  display_class->supports_composite = cdk_x11_display_supports_composite;
  display_class->get_app_launch_context = _cdk_x11_display_get_app_launch_context;
  display_class->get_cursor_for_type = _cdk_x11_display_get_cursor_for_type;
  display_class->get_cursor_for_name = _cdk_x11_display_get_cursor_for_name;
  display_class->get_cursor_for_surface = _cdk_x11_display_get_cursor_for_surface;
  display_class->get_default_cursor_size = _cdk_x11_display_get_default_cursor_size;
  display_class->get_maximal_cursor_size = _cdk_x11_display_get_maximal_cursor_size;
  display_class->supports_cursor_alpha = _cdk_x11_display_supports_cursor_alpha;
  display_class->supports_cursor_color = _cdk_x11_display_supports_cursor_color;

  display_class->before_process_all_updates = _cdk_x11_display_before_process_all_updates;
  display_class->after_process_all_updates = _cdk_x11_display_after_process_all_updates;
  display_class->get_next_serial = cdk_x11_display_get_next_serial;
  display_class->notify_startup_complete = cdk_x11_display_notify_startup_complete;
  display_class->create_window_impl = _cdk_x11_display_create_window_impl;
  display_class->get_keymap = cdk_x11_display_get_keymap;
  display_class->push_error_trap = cdk_x11_display_error_trap_push;
  display_class->pop_error_trap = pop_error_trap;
  display_class->get_selection_owner = _cdk_x11_display_get_selection_owner;
  display_class->set_selection_owner = _cdk_x11_display_set_selection_owner;
  display_class->send_selection_notify = _cdk_x11_display_send_selection_notify;
  display_class->get_selection_property = _cdk_x11_display_get_selection_property;
  display_class->convert_selection = _cdk_x11_display_convert_selection;
  display_class->text_property_to_utf8_list = _cdk_x11_display_text_property_to_utf8_list;
  display_class->utf8_to_string_target = _cdk_x11_display_utf8_to_string_target;

  display_class->make_gl_context_current = cdk_x11_display_make_gl_context_current;

  display_class->get_default_seat = cdk_x11_display_get_default_seat;

  display_class->get_n_monitors = cdk_x11_display_get_n_monitors;
  display_class->get_monitor = cdk_x11_display_get_monitor;
  display_class->get_primary_monitor = cdk_x11_display_get_primary_monitor;

  _cdk_x11_windowing_init ();
}
