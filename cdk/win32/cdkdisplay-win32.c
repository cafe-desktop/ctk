/* CDK - The GIMP Drawing Kit
 * Copyright (C) 2002,2005 Hans Breuer
 * Copyright (C) 2003 Tor Lillqvist
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

#define _WIN32_WINNT 0x0600

#include "cdk.h"
#include "cdkprivate-win32.h"
#include "cdkdisplay-win32.h"
#include "cdkdevicemanager-win32.h"
#include "cdkglcontext-win32.h"
#include "cdkwin32display.h"
#include "cdkwin32screen.h"
#include "cdkwin32window.h"
#include "cdkmonitor-win32.h"
#include "cdkwin32.h"

#ifdef CDK_WIN32_ENABLE_EGL
# include <epoxy/egl.h>
#endif

#include "cdkwin32langnotification.h"

#ifndef IMAGE_FILE_MACHINE_ARM64
# define IMAGE_FILE_MACHINE_ARM64 0xAA64
#endif

static int debug_indent = 0;

static CdkMonitor *
_cdk_win32_display_find_matching_monitor (CdkWin32Display *win32_display,
                                          CdkMonitor      *needle)
{
  int i;

  for (i = 0; i < win32_display->monitors->len; i++)
    {
      CdkWin32Monitor *m;

      m = CDK_WIN32_MONITOR (g_ptr_array_index (win32_display->monitors, i));

      if (_cdk_win32_monitor_compare (m, CDK_WIN32_MONITOR (needle)) == 0)
        return CDK_MONITOR (m);
    }

  return NULL;
}

gboolean
_cdk_win32_display_init_monitors (CdkWin32Display *win32_display)
{
  CdkDisplay *display = CDK_DISPLAY (win32_display);
  GPtrArray *new_monitors;
  gint i;
  gboolean changed = FALSE;
  CdkWin32Monitor *primary_to_move = NULL;

  for (i = 0; i < win32_display->monitors->len; i++)
    CDK_WIN32_MONITOR (g_ptr_array_index (win32_display->monitors, i))->remove = TRUE;

  new_monitors = _cdk_win32_display_get_monitor_list (win32_display);

  for (i = 0; i < new_monitors->len; i++)
    {
      CdkWin32Monitor *w32_m;
      CdkMonitor *m;
      CdkWin32Monitor *w32_ex_monitor;
      CdkMonitor *ex_monitor;
      CdkRectangle geometry, ex_geometry;
      CdkRectangle workarea, ex_workarea;

      w32_m = CDK_WIN32_MONITOR (g_ptr_array_index (new_monitors, i));
      m = CDK_MONITOR (w32_m);
      ex_monitor = _cdk_win32_display_find_matching_monitor (win32_display, m);
      w32_ex_monitor = CDK_WIN32_MONITOR (ex_monitor);

      if (ex_monitor == NULL)
        {
          w32_m->add = TRUE;
          changed = TRUE;
          continue;
        }

      w32_ex_monitor->remove = FALSE;

      if (i == 0)
        primary_to_move = w32_ex_monitor;

      cdk_monitor_get_geometry (m, &geometry);
      cdk_monitor_get_geometry (ex_monitor, &ex_geometry);
      cdk_monitor_get_workarea (m, &workarea);
      cdk_monitor_get_workarea (ex_monitor, &ex_workarea);

      if (memcmp (&workarea, &ex_workarea, sizeof (CdkRectangle)) != 0)
        {
          w32_ex_monitor->work_rect = workarea;
          changed = TRUE;
        }

      if (memcmp (&geometry, &ex_geometry, sizeof (CdkRectangle)) != 0)
        {
          cdk_monitor_set_size (ex_monitor, geometry.width, geometry.height);
          cdk_monitor_set_position (ex_monitor, geometry.x, geometry.y);
          changed = TRUE;
        }

      if (cdk_monitor_get_width_mm (m) != cdk_monitor_get_width_mm (ex_monitor) ||
          cdk_monitor_get_height_mm (m) != cdk_monitor_get_height_mm (ex_monitor))
        {
          cdk_monitor_set_physical_size (ex_monitor,
                                         cdk_monitor_get_width_mm (m),
                                         cdk_monitor_get_height_mm (m));
          changed = TRUE;
        }

      if (g_strcmp0 (cdk_monitor_get_model (m), cdk_monitor_get_model (ex_monitor)) != 0)
        {
          cdk_monitor_set_model (ex_monitor,
                                 cdk_monitor_get_model (m));
          changed = TRUE;
        }

      if (g_strcmp0 (cdk_monitor_get_manufacturer (m), cdk_monitor_get_manufacturer (ex_monitor)) != 0)
        {
          cdk_monitor_set_manufacturer (ex_monitor,
                                        cdk_monitor_get_manufacturer (m));
          changed = TRUE;
        }

      if (cdk_monitor_get_refresh_rate (m) != cdk_monitor_get_refresh_rate (ex_monitor))
        {
          cdk_monitor_set_refresh_rate (ex_monitor, cdk_monitor_get_refresh_rate (m));
          changed = TRUE;
        }

      if (cdk_monitor_get_scale_factor (m) != cdk_monitor_get_scale_factor (ex_monitor))
        {
          cdk_monitor_set_scale_factor (ex_monitor, cdk_monitor_get_scale_factor (m));
          changed = TRUE;
        }

      if (cdk_monitor_get_subpixel_layout (m) != cdk_monitor_get_subpixel_layout (ex_monitor))
        {
          cdk_monitor_set_subpixel_layout (ex_monitor, cdk_monitor_get_subpixel_layout (m));
          changed = TRUE;
        }
    }

  for (i = win32_display->monitors->len - 1; i >= 0; i--)
    {
      CdkWin32Monitor *w32_ex_monitor;
      CdkMonitor *ex_monitor;

      w32_ex_monitor = CDK_WIN32_MONITOR (g_ptr_array_index (win32_display->monitors, i));
      ex_monitor = CDK_MONITOR (w32_ex_monitor);

      if (!w32_ex_monitor->remove)
        continue;

      changed = TRUE;
      cdk_display_monitor_removed (display, ex_monitor);
      g_ptr_array_remove_index (win32_display->monitors, i);
    }

  for (i = 0; i < new_monitors->len; i++)
    {
      CdkWin32Monitor *w32_m;
      CdkMonitor *m;

      w32_m = CDK_WIN32_MONITOR (g_ptr_array_index (new_monitors, i));
      m = CDK_MONITOR (w32_m);

      if (!w32_m->add)
        continue;

      cdk_display_monitor_added (display, m);

      if (i == 0)
        g_ptr_array_insert (win32_display->monitors, 0, g_object_ref (w32_m));
      else
        g_ptr_array_add (win32_display->monitors, g_object_ref (w32_m));
    }

  g_ptr_array_free (new_monitors, TRUE);

  if (primary_to_move)
    {
      g_ptr_array_remove (win32_display->monitors, g_object_ref (primary_to_move));
      g_ptr_array_insert (win32_display->monitors, 0, primary_to_move);
      changed = TRUE;
    }
  return changed;
}


/**
 * cdk_win32_display_set_cursor_theme:
 * @display: (type CdkWin32Display): a #CdkDisplay
 * @name: (allow-none): the name of the cursor theme to use, or %NULL to unset
 *         a previously set value
 * @size: the cursor size to use, or 0 to keep the previous size
 *
 * Sets the cursor theme from which the images for cursor
 * should be taken.
 *
 * If the windowing system supports it, existing cursors created
 * with cdk_cursor_new(), cdk_cursor_new_for_display() and
 * cdk_cursor_new_from_name() are updated to reflect the theme
 * change. Custom cursors constructed with
 * cdk_cursor_new_from_pixbuf() will have to be handled
 * by the application (CTK+ applications can learn about
 * cursor theme changes by listening for change notification
 * for the corresponding #CtkSetting).
 *
 * Since: 3.18
 */
void
cdk_win32_display_set_cursor_theme (CdkDisplay  *display,
                                    const gchar *name,
                                    gint         size)
{
  gint cursor_size;
  gint w, h;
  Win32CursorTheme *theme;
  CdkWin32Display *win32_display = CDK_WIN32_DISPLAY (display);

  g_assert (win32_display);

  if (name == NULL)
    name = "system";

  w = GetSystemMetrics (SM_CXCURSOR);
  h = GetSystemMetrics (SM_CYCURSOR);

  /* We can load cursors of any size, but SetCursor() will scale them back
   * to this value. It's possible to break that restrictions with SetSystemCursor(),
   * but that will override cursors for the whole desktop session.
   */
  cursor_size = (w == h) ? w : size;

  if (win32_display->cursor_theme_name != NULL &&
      g_strcmp0 (name, win32_display->cursor_theme_name) == 0 &&
      win32_display->cursor_theme_size == cursor_size)
    return;

  theme = win32_cursor_theme_load (name, cursor_size);
  if (theme == NULL)
    {
      g_warning ("Failed to load cursor theme %s", name);
      return;
    }

  if (win32_display->cursor_theme)
    {
      win32_cursor_theme_destroy (win32_display->cursor_theme);
      win32_display->cursor_theme = NULL;
    }

  win32_display->cursor_theme = theme;
  g_free (win32_display->cursor_theme_name);
  win32_display->cursor_theme_name = g_strdup (name);
  win32_display->cursor_theme_size = cursor_size;

  _cdk_win32_display_update_cursors (win32_display);
}

Win32CursorTheme *
_cdk_win32_display_get_cursor_theme (CdkWin32Display *win32_display)
{
  Win32CursorTheme *theme;

  g_assert (win32_display->cursor_theme_name);

  theme = win32_display->cursor_theme;
  if (!theme)
    {
      theme = win32_cursor_theme_load (win32_display->cursor_theme_name,
                                       win32_display->cursor_theme_size);
      if (theme == NULL)
        {
          g_warning ("Failed to load cursor theme %s",
                     win32_display->cursor_theme_name);
          return NULL;
        }
      win32_display->cursor_theme = theme;
    }

  return theme;
}

static gulong
cdk_win32_display_get_next_serial (CdkDisplay *display)
{
	return 0;
}

static LRESULT CALLBACK
inner_display_change_window_procedure (HWND   hwnd,
                                       UINT   message,
                                       WPARAM wparam,
                                       LPARAM lparam)
{
  switch (message)
    {
    case WM_DESTROY:
      {
        PostQuitMessage (0);
        return 0;
      }
    case WM_DISPLAYCHANGE:
      {
        CdkWin32Display *win32_display = CDK_WIN32_DISPLAY (_cdk_display);

        _cdk_win32_screen_on_displaychange_event (CDK_WIN32_SCREEN (win32_display->screen));
        return 0;
      }
    default:
      /* Otherwise call DefWindowProcW(). */
      CDK_NOTE (EVENTS, g_print (" DefWindowProcW"));
      return DefWindowProc (hwnd, message, wparam, lparam);
    }
}

static LRESULT CALLBACK
display_change_window_procedure (HWND   hwnd,
                                 UINT   message,
                                 WPARAM wparam,
                                 LPARAM lparam)
{
  LRESULT retval;

  CDK_NOTE (EVENTS, g_print ("%s%*s%s %p",
			     (debug_indent > 0 ? "\n" : ""),
			     debug_indent, "",
			     _cdk_win32_message_to_string (message), hwnd));
  debug_indent += 2;
  retval = inner_display_change_window_procedure (hwnd, message, wparam, lparam);
  debug_indent -= 2;

  CDK_NOTE (EVENTS, g_print (" => %" G_GINT64_FORMAT "%s", (gint64) retval, (debug_indent == 0 ? "\n" : "")));

  return retval;
}

/* Use a hidden window to be notified about display changes */
static void
register_display_change_notification (CdkDisplay *display)
{
  CdkWin32Display *display_win32 = CDK_WIN32_DISPLAY (display);
  WNDCLASS wclass = { 0, };
  ATOM klass;

  wclass.lpszClassName = "CdkDisplayChange";
  wclass.lpfnWndProc = display_change_window_procedure;
  wclass.hInstance = _cdk_app_hmodule;

  klass = RegisterClass (&wclass);
  if (klass)
    {
      display_win32->hwnd = CreateWindow (MAKEINTRESOURCE (klass),
                                          NULL, WS_POPUP,
                                          0, 0, 0, 0, NULL, NULL,
                                          _cdk_app_hmodule, NULL);
      if (!display_win32->hwnd)
        {
          UnregisterClass (MAKEINTRESOURCE (klass), _cdk_app_hmodule);
        }
    }
}

CdkDisplay *
_cdk_win32_display_open (const gchar *display_name)
{
  CdkWin32Display *win32_display;

  CDK_NOTE (MISC, g_print ("cdk_display_open: %s\n", (display_name ? display_name : "NULL")));

  if (display_name == NULL ||
      g_ascii_strcasecmp (display_name,
			  cdk_display_get_name (_cdk_display)) == 0)
    {
      if (_cdk_display != NULL)
	{
	  CDK_NOTE (MISC, g_print ("... return _cdk_display\n"));
	  return _cdk_display;
	}
    }
  else
    {
      CDK_NOTE (MISC, g_print ("... return NULL\n"));
      return NULL;
    }

  _cdk_display = g_object_new (CDK_TYPE_WIN32_DISPLAY, NULL);
  win32_display = CDK_WIN32_DISPLAY (_cdk_display);

  win32_display->screen = g_object_new (CDK_TYPE_WIN32_SCREEN, NULL);

  _cdk_events_init (_cdk_display);

  _cdk_input_ignore_core = 0;

  _cdk_display->device_manager = g_object_new (CDK_TYPE_DEVICE_MANAGER_WIN32,
                                               "display", _cdk_display,
                                               NULL);

  _cdk_win32_lang_notification_init ();
  _cdk_dnd_init ();

  /* Precalculate display name */
  (void) cdk_display_get_name (_cdk_display);

  register_display_change_notification (_cdk_display);

  g_signal_emit_by_name (_cdk_display, "opened");

  CDK_NOTE (MISC, g_print ("... _cdk_display now set up\n"));

  return _cdk_display;
}

G_DEFINE_TYPE (CdkWin32Display, cdk_win32_display, CDK_TYPE_DISPLAY)

static const gchar *
cdk_win32_display_get_name (CdkDisplay *display)
{
  HDESK hdesk = GetThreadDesktop (GetCurrentThreadId ());
  char dummy;
  char *desktop_name;
  HWINSTA hwinsta = GetProcessWindowStation ();
  char *window_station_name;
  DWORD n;
  DWORD session_id;
  char *display_name;
  static const char *display_name_cache = NULL;
  typedef BOOL (WINAPI *PFN_ProcessIdToSessionId) (DWORD, DWORD *);
  PFN_ProcessIdToSessionId processIdToSessionId;

  g_return_val_if_fail (CDK_IS_DISPLAY (display), NULL);

  if (display_name_cache != NULL)
    return display_name_cache;

  n = 0;
  GetUserObjectInformation (hdesk, UOI_NAME, &dummy, 0, &n);
  if (n == 0)
    desktop_name = "Default";
  else
    {
      n++;
      desktop_name = g_alloca (n + 1);
      memset (desktop_name, 0, n + 1);

      if (!GetUserObjectInformation (hdesk, UOI_NAME, desktop_name, n, &n))
	desktop_name = "Default";
    }

  n = 0;
  GetUserObjectInformation (hwinsta, UOI_NAME, &dummy, 0, &n);
  if (n == 0)
    window_station_name = "WinSta0";
  else
    {
      n++;
      window_station_name = g_alloca (n + 1);
      memset (window_station_name, 0, n + 1);

      if (!GetUserObjectInformation (hwinsta, UOI_NAME, window_station_name, n, &n))
	window_station_name = "WinSta0";
    }

  processIdToSessionId = (PFN_ProcessIdToSessionId) GetProcAddress (GetModuleHandle ("kernel32.dll"), "ProcessIdToSessionId");
  if (!processIdToSessionId || !processIdToSessionId (GetCurrentProcessId (), &session_id))
    session_id = 0;

  display_name = g_strdup_printf ("%ld\\%s\\%s",
				  session_id,
				  window_station_name,
				  desktop_name);

  CDK_NOTE (MISC, g_print ("cdk_win32_display_get_name: %s\n", display_name));

  display_name_cache = display_name;

  return display_name_cache;
}

static CdkScreen *
cdk_win32_display_get_default_screen (CdkDisplay *display)
{
  g_return_val_if_fail (CDK_IS_WIN32_DISPLAY (display), NULL);

  return CDK_WIN32_DISPLAY (display)->screen;
}

static CdkWindow *
cdk_win32_display_get_default_group (CdkDisplay *display)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), NULL);

  g_warning ("cdk_display_get_default_group not yet implemented");

  return NULL;
}

static gboolean
cdk_win32_display_supports_selection_notification (CdkDisplay *display)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), FALSE);

  return TRUE;
}

/*
 * maybe this should be integrated with the default message loop - or maybe not ;-)
 */
static LRESULT CALLBACK
inner_clipboard_window_procedure (HWND   hwnd,
                                  UINT   message,
                                  WPARAM wparam,
                                  LPARAM lparam)
{
  switch (message)
    {
    case WM_DESTROY: /* remove us from chain */
      {
        RemoveClipboardFormatListener (hwnd);
        PostQuitMessage (0);
        return 0;
      }
    case WM_CLIPBOARDUPDATE:
      {
        HWND hwnd_owner;
        HWND stored_hwnd_owner;
        HWND hwnd_opener;
        CdkEvent *event;
        CdkWindow *owner;
        CdkWindow *stored_owner;
        CdkWin32Selection *win32_sel = _cdk_win32_selection_get ();

        hwnd_owner = GetClipboardOwner ();

        if ((hwnd_owner == NULL) &&
            (GetLastError () != ERROR_SUCCESS))
            WIN32_API_FAILED ("GetClipboardOwner");

        hwnd_opener = GetOpenClipboardWindow ();

        CDK_NOTE (DND, g_print (" drawclipboard owner: %p; opener %p ", hwnd_owner, hwnd_opener));

#ifdef G_ENABLE_DEBUG
        if (_cdk_debug_flags & CDK_DEBUG_DND)
          {
            if (win32_sel->clipboard_opened_for != INVALID_HANDLE_VALUE ||
                OpenClipboard (hwnd))
              {
                UINT nFormat = 0;

                while ((nFormat = EnumClipboardFormats (nFormat)) != 0)
                  g_print ("%s ", _cdk_win32_cf_to_string (nFormat));

                if (win32_sel->clipboard_opened_for == INVALID_HANDLE_VALUE)
                  CloseClipboard ();
              }
            else
              {
                WIN32_API_FAILED ("OpenClipboard");
              }
          }
#endif

        CDK_NOTE (DND, g_print (" \n"));

        owner = cdk_win32_window_lookup_for_display (_cdk_display, hwnd_owner);
        if (owner == NULL)
          owner = cdk_win32_window_foreign_new_for_display (_cdk_display, hwnd_owner);

        stored_owner = _cdk_win32_display_get_selection_owner (cdk_display_get_default (),
                                                               CDK_SELECTION_CLIPBOARD);

        if (stored_owner)
          stored_hwnd_owner = CDK_WINDOW_HWND (stored_owner);
        else
          stored_hwnd_owner = NULL;

        if (stored_hwnd_owner != hwnd_owner)
          {
            if (win32_sel->clipboard_opened_for != INVALID_HANDLE_VALUE)
              {
                CloseClipboard ();
                CDK_NOTE (DND, g_print ("Closed clipboard @ %s:%d\n", __FILE__, __LINE__));
              }

            win32_sel->clipboard_opened_for = INVALID_HANDLE_VALUE;

            _cdk_win32_clear_clipboard_queue ();
          }

        event = cdk_event_new (CDK_OWNER_CHANGE);
        event->owner_change.window = cdk_get_default_root_window ();
        event->owner_change.owner = owner;
        event->owner_change.reason = CDK_OWNER_CHANGE_NEW_OWNER;
        event->owner_change.selection = CDK_SELECTION_CLIPBOARD;
        event->owner_change.time = _cdk_win32_get_next_tick (0);
        event->owner_change.selection_time = CDK_CURRENT_TIME;
        _cdk_win32_append_event (event);

        /* clear error to avoid confusing SetClipboardViewer() return */
        SetLastError (0);
        return 0;
      }
    default:
      /* Otherwise call DefWindowProcW(). */
      CDK_NOTE (EVENTS, g_print (" DefWindowProcW"));
      return DefWindowProc (hwnd, message, wparam, lparam);
    }
}

static LRESULT CALLBACK
_clipboard_window_procedure (HWND   hwnd,
                             UINT   message,
                             WPARAM wparam,
                             LPARAM lparam)
{
  LRESULT retval;

  CDK_NOTE (EVENTS, g_print ("%s%*s%s %p",
			     (debug_indent > 0 ? "\n" : ""),
			     debug_indent, "",
			     _cdk_win32_message_to_string (message), hwnd));
  debug_indent += 2;
  retval = inner_clipboard_window_procedure (hwnd, message, wparam, lparam);
  debug_indent -= 2;

  CDK_NOTE (EVENTS, g_print (" => %" G_GINT64_FORMAT "%s", (gint64) retval, (debug_indent == 0 ? "\n" : "")));

  return retval;
}

/*
 * Creates a hidden window and adds it to the clipboard chain
 */
static gboolean
register_clipboard_notification (CdkDisplay *display)
{
  CdkWin32Display *display_win32 = CDK_WIN32_DISPLAY (display);
  WNDCLASS wclass = { 0, };
  ATOM klass;

  wclass.lpszClassName = "CdkClipboardNotification";
  wclass.lpfnWndProc = _clipboard_window_procedure;
  wclass.hInstance = _cdk_app_hmodule;

  klass = RegisterClass (&wclass);
  if (!klass)
    return FALSE;

  display_win32->clipboard_hwnd = CreateWindow (MAKEINTRESOURCE (klass),
                                                NULL, WS_POPUP,
                                                0, 0, 0, 0, NULL, NULL,
                                                _cdk_app_hmodule, NULL);

  if (display_win32->clipboard_hwnd == NULL)
    goto failed;

  SetLastError (0);

  if (AddClipboardFormatListener (display_win32->clipboard_hwnd) == FALSE)
    goto failed;

  return TRUE;

failed:
  g_critical ("Failed to install clipboard viewer");
  UnregisterClass (MAKEINTRESOURCE (klass), _cdk_app_hmodule);
  return FALSE;
}

static gboolean
cdk_win32_display_request_selection_notification (CdkDisplay *display,
                                                  CdkAtom     selection)

{
  CdkWin32Display *display_win32 = CDK_WIN32_DISPLAY (display);
  gboolean ret = FALSE;
  gchar *selection_name = cdk_atom_name (selection);

  CDK_NOTE (DND,
            g_print ("cdk_display_request_selection_notification (..., %s)",
                     selection_name));

  if (selection == CDK_SELECTION_CLIPBOARD ||
      selection == CDK_SELECTION_PRIMARY)
    {
      if (display_win32->clipboard_hwnd == NULL)
        {
          if (register_clipboard_notification (display))
            CDK_NOTE (DND, g_print (" registered"));
          else
            CDK_NOTE (DND, g_print (" failed to register"));
        }
      ret = (display_win32->clipboard_hwnd != NULL);
    }
  else
    {
      CDK_NOTE (DND, g_print (" unsupported"));
      ret = FALSE;
    }

  g_free (selection_name);

  CDK_NOTE (DND, g_print (" -> %s\n", ret ? "TRUE" : "FALSE"));
  return ret;
}

static gboolean
cdk_win32_display_supports_clipboard_persistence (CdkDisplay *display)
{
  return TRUE;
}

static void
cdk_win32_display_store_clipboard (CdkDisplay    *display,
                                   CdkWindow     *clipboard_window,
                                   guint32        time_,
                                   const CdkAtom *targets,
                                   gint           n_targets)
{
  CdkEvent tmp_event;
  SendMessage (CDK_WINDOW_HWND (clipboard_window), WM_RENDERALLFORMATS, 0, 0);

  memset (&tmp_event, 0, sizeof (tmp_event));
  tmp_event.selection.type = CDK_SELECTION_NOTIFY;
  tmp_event.selection.window = clipboard_window;
  tmp_event.selection.send_event = FALSE;
  tmp_event.selection.selection = _cdk_win32_selection_atom (CDK_WIN32_ATOM_INDEX_CLIPBOARD_MANAGER);
  tmp_event.selection.target = 0;
  tmp_event.selection.property = CDK_NONE;
  tmp_event.selection.requestor = 0;
  tmp_event.selection.time = CDK_CURRENT_TIME;

  cdk_event_put (&tmp_event);
}

static gboolean
cdk_win32_display_supports_shapes (CdkDisplay *display)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), FALSE);

  return TRUE;
}

static gboolean
cdk_win32_display_supports_input_shapes (CdkDisplay *display)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), FALSE);

  /* Partially supported, see WM_NCHITTEST handler. */
  return TRUE;
}

static gboolean
cdk_win32_display_supports_composite (CdkDisplay *display)
{
  return FALSE;
}

static void
cdk_win32_display_beep (CdkDisplay *display)
{
  g_return_if_fail (display == cdk_display_get_default());
  if (!MessageBeep (-1))
    Beep(1000, 50);
}

static void
cdk_win32_display_flush (CdkDisplay * display)
{
  g_return_if_fail (display == _cdk_display);

  GdiFlush ();
}

static void
cdk_win32_display_sync (CdkDisplay * display)
{
  g_return_if_fail (display == _cdk_display);

  GdiFlush ();
}

static void
cdk_win32_display_dispose (GObject *object)
{
  CdkWin32Display *display_win32 = CDK_WIN32_DISPLAY (object);

  _cdk_screen_close (display_win32->screen);

#ifdef CDK_WIN32_ENABLE_EGL
  if (display_win32->egl_disp != EGL_NO_DISPLAY)
    {
      eglTerminate (display_win32->egl_disp);
      display_win32->egl_disp = EGL_NO_DISPLAY;
    }
#endif

  if (display_win32->hwnd != NULL)
    {
      DestroyWindow (display_win32->hwnd);
      display_win32->hwnd = NULL;
    }

  if (display_win32->clipboard_hwnd != NULL)
    {
      DestroyWindow (display_win32->clipboard_hwnd);
      display_win32->clipboard_hwnd = NULL;
    }

  if (display_win32->have_at_least_win81)
    {
      if (display_win32->shcore_funcs.hshcore != NULL)
        {
          FreeLibrary (display_win32->shcore_funcs.hshcore);
          display_win32->shcore_funcs.hshcore = NULL;
        }
    }

  G_OBJECT_CLASS (cdk_win32_display_parent_class)->dispose (object);
}

static void
cdk_win32_display_finalize (GObject *object)
{
  CdkWin32Display *display_win32 = CDK_WIN32_DISPLAY (object);

  _cdk_win32_display_finalize_cursors (display_win32);
  _cdk_win32_dnd_exit ();
  _cdk_win32_lang_notification_exit ();

  g_ptr_array_free (display_win32->monitors, TRUE);

  G_OBJECT_CLASS (cdk_win32_display_parent_class)->finalize (object);
}

static void
_cdk_win32_enable_hidpi (CdkWin32Display *display)
{
  gboolean check_for_dpi_awareness = FALSE;
  gboolean have_hpi_disable_envvar = FALSE;

  enum dpi_aware_status {
    DPI_STATUS_PENDING,
    DPI_STATUS_SUCCESS,
    DPI_STATUS_DISABLED,
    DPI_STATUS_FAILED
  } status = DPI_STATUS_PENDING;

  if (g_win32_check_windows_version (6, 3, 0, G_WIN32_OS_ANY))
    {
      /* If we are on Windows 8.1 or later, cache up functions from shcore.dll, by all means */
      display->have_at_least_win81 = TRUE;
      display->shcore_funcs.hshcore = LoadLibraryW (L"shcore.dll");

      if (display->shcore_funcs.hshcore != NULL)
        {
          display->shcore_funcs.setDpiAwareFunc =
            (funcSetProcessDpiAwareness) GetProcAddress (display->shcore_funcs.hshcore,
                                                         "SetProcessDpiAwareness");
          display->shcore_funcs.getDpiAwareFunc =
            (funcGetProcessDpiAwareness) GetProcAddress (display->shcore_funcs.hshcore,
                                                         "GetProcessDpiAwareness");

          display->shcore_funcs.getDpiForMonitorFunc =
            (funcGetDpiForMonitor) GetProcAddress (display->shcore_funcs.hshcore,
                                                   "GetDpiForMonitor");
        }
    }
  else
    {
      /* Windows Vista through 8: use functions from user32.dll directly */
      HMODULE user32;

      display->have_at_least_win81 = FALSE;
      user32 = GetModuleHandleW (L"user32.dll");

      if (user32 != NULL)
        {
          display->user32_dpi_funcs.setDpiAwareFunc =
            (funcSetProcessDPIAware) GetProcAddress (user32, "SetProcessDPIAware");
          display->user32_dpi_funcs.isDpiAwareFunc =
            (funcIsProcessDPIAware) GetProcAddress (user32, "IsProcessDPIAware");
        }
    }

  if (g_getenv ("CDK_WIN32_DISABLE_HIDPI") == NULL)
    {
      /* For Windows 8.1 and later, use SetProcessDPIAwareness() */
      if (display->have_at_least_win81)
        {
          /* then make the CDK-using app DPI-aware */
          if (display->shcore_funcs.setDpiAwareFunc != NULL)
            {
              switch (display->shcore_funcs.setDpiAwareFunc (PROCESS_SYSTEM_DPI_AWARE))
                {
                  case S_OK:
                    display->dpi_aware_type = PROCESS_SYSTEM_DPI_AWARE;
                    status = DPI_STATUS_SUCCESS;
                    break;
                  case E_ACCESSDENIED:
                    /* This means the app used a manifest to set DPI awareness, or a
                       DPI compatibility setting is used.
                       The manifest is the trump card in this game of bridge here.  The
                       same applies if one uses the control panel or program properties to
                       force system DPI awareness */
                    check_for_dpi_awareness = TRUE;
                    break;
                  default:
                    display->dpi_aware_type = PROCESS_DPI_UNAWARE;
                    status = DPI_STATUS_FAILED;
                    break;
                }
            }
          else
            {
              check_for_dpi_awareness = TRUE;
            }
        }
      else
        {
          /* For Windows Vista through 8, use SetProcessDPIAware() */
          display->have_at_least_win81 = FALSE;
          if (display->user32_dpi_funcs.setDpiAwareFunc != NULL)
            {
              if (display->user32_dpi_funcs.setDpiAwareFunc () != 0)
                {
                  display->dpi_aware_type = PROCESS_SYSTEM_DPI_AWARE;
                  status = DPI_STATUS_SUCCESS;
                }
              else
                {
                  check_for_dpi_awareness = TRUE;
                }
            }
          else
            {
              display->dpi_aware_type = PROCESS_DPI_UNAWARE;
              status = DPI_STATUS_FAILED;
            }
        }
    }
  else
    {
      /* if CDK_WIN32_DISABLE_HIDPI is set, check for any DPI
       * awareness settings done via manifests or user settings
       */
      check_for_dpi_awareness = TRUE;
      have_hpi_disable_envvar = TRUE;
    }

  if (check_for_dpi_awareness)
    {
      if (display->have_at_least_win81)
        {
          if (display->shcore_funcs.getDpiAwareFunc != NULL)
            {
              display->shcore_funcs.getDpiAwareFunc (NULL, &display->dpi_aware_type);

              if (display->dpi_aware_type != PROCESS_DPI_UNAWARE)
                status = DPI_STATUS_SUCCESS;
              else
                /* This means the DPI awareness setting was forcefully disabled */
                status = DPI_STATUS_DISABLED;
            }
          else
            {
              display->dpi_aware_type = PROCESS_DPI_UNAWARE;
              status = DPI_STATUS_FAILED;
            }
        }
      else
        {
          if (display->user32_dpi_funcs.isDpiAwareFunc != NULL)
            {
              /* This most probably means DPI awareness is set through
                 the manifest, or a DPI compatibility setting is used. */
              display->dpi_aware_type = display->user32_dpi_funcs.isDpiAwareFunc () ?
                                        PROCESS_SYSTEM_DPI_AWARE :
                                        PROCESS_DPI_UNAWARE;

              if (display->dpi_aware_type == PROCESS_SYSTEM_DPI_AWARE)
                status = DPI_STATUS_SUCCESS;
              else
                status = DPI_STATUS_DISABLED;
            }
          else
            {
              display->dpi_aware_type = PROCESS_DPI_UNAWARE;
              status = DPI_STATUS_FAILED;
            }
        }
      if (have_hpi_disable_envvar &&
          status == DPI_STATUS_SUCCESS)
        {
          /* The user setting or application manifest trumps over CDK_WIN32_DISABLE_HIDPI */
          g_print ("Note: CDK_WIN32_DISABLE_HIDPI is ignored due to preset\n"
                   "      DPI awareness settings in user settings or application\n"
                   "      manifest, DPI awareness is still enabled.");
        }
    }

  switch (status)
    {
    case DPI_STATUS_SUCCESS:
      CDK_NOTE (MISC, g_message ("HiDPI support enabled, type: %s",
                                 display->dpi_aware_type == PROCESS_PER_MONITOR_DPI_AWARE ? "per-monitor" : "system"));
      break;
    case DPI_STATUS_DISABLED:
      CDK_NOTE (MISC, g_message ("HiDPI support disabled via manifest"));
      break;
    case DPI_STATUS_FAILED:
      g_warning ("Failed to enable HiDPI support.");
    }
}

static void
_cdk_win32_check_on_arm64 (CdkWin32Display *display)
{
  static gsize checked = 0;

  if (g_once_init_enter (&checked))
    {
      HMODULE kernel32 = LoadLibraryW (L"kernel32.dll");

      if (kernel32 != NULL)
        {
          display->cpu_funcs.isWow64Process2 =
            (funcIsWow64Process2) GetProcAddress (kernel32, "IsWow64Process2");

          if (display->cpu_funcs.isWow64Process2 != NULL)
            {
              USHORT proc_cpu = 0;
              USHORT native_cpu = 0;

              display->cpu_funcs.isWow64Process2 (GetCurrentProcess (),
                                                  &proc_cpu,
                                                  &native_cpu);

              if (native_cpu == IMAGE_FILE_MACHINE_ARM64)
                display->running_on_arm64 = TRUE;
            }

          FreeLibrary (kernel32);
        }

      g_once_init_leave (&checked, 1);
    }
}

static void
cdk_win32_display_init (CdkWin32Display *display)
{
  const gchar *scale_str = g_getenv ("CDK_SCALE");

  display->monitors = g_ptr_array_new_with_free_func (g_object_unref);

  _cdk_win32_enable_hidpi (display);
  _cdk_win32_check_on_arm64 (display);

  /* if we have DPI awareness, set up fixed scale if set */
  if (display->dpi_aware_type != PROCESS_DPI_UNAWARE &&
      scale_str != NULL)
    {
      display->window_scale = atol (scale_str);

      if (display->window_scale == 0)
        display->window_scale = 1;

      display->has_fixed_scale = TRUE;
    }
  else
    display->window_scale = 1;

  _cdk_win32_display_init_cursors (display);
}

static void
cdk_win32_display_before_process_all_updates (CdkDisplay  *display)
{
  /* nothing */
}
static void
cdk_win32_display_after_process_all_updates (CdkDisplay  *display)
{
  /* nothing */
}

static void
cdk_win32_display_notify_startup_complete (CdkDisplay  *display,
                                           const gchar *startup_id)
{
  /* nothing */
}

static void
cdk_win32_display_push_error_trap (CdkDisplay *display)
{
  /* nothing */
}

static gint
cdk_win32_display_pop_error_trap (CdkDisplay *display,
				  gboolean    ignored)
{
  return 0;
}

static int
cdk_win32_display_get_n_monitors (CdkDisplay *display)
{
  CdkWin32Display *win32_display = CDK_WIN32_DISPLAY (display);

  return win32_display->monitors->len;
}


static CdkMonitor *
cdk_win32_display_get_monitor (CdkDisplay *display,
                               int         monitor_num)
{
  CdkWin32Display *win32_display = CDK_WIN32_DISPLAY (display);

  if (monitor_num < 0 || monitor_num >= win32_display->monitors->len)
    return NULL;

  return (CdkMonitor *) g_ptr_array_index (win32_display->monitors, monitor_num);
}

static CdkMonitor *
cdk_win32_display_get_primary_monitor (CdkDisplay *display)
{
  CdkWin32Display *win32_display = CDK_WIN32_DISPLAY (display);

  /* We arrange for the first monitor in the array to also be the primiary monitor */
  if (win32_display->monitors->len > 0)
    return (CdkMonitor *) g_ptr_array_index (win32_display->monitors, 0);

  return NULL;
}

guint
_cdk_win32_display_get_monitor_scale_factor (CdkWin32Display *win32_display,
                                             HMONITOR         hmonitor,
                                             HWND             hwnd,
                                             gint            *dpi)
{
  gboolean is_scale_acquired = FALSE;
  gboolean use_dpi_for_monitor = FALSE;
  guint dpix, dpiy;

  if (win32_display->have_at_least_win81)
    {
      if (hmonitor != NULL)
        use_dpi_for_monitor = TRUE;

      else
        {
          if (hwnd != NULL)
            {
              hmonitor = MonitorFromWindow (hwnd, MONITOR_DEFAULTTONEAREST);
              use_dpi_for_monitor = TRUE;
            }
        }
    }

  if (use_dpi_for_monitor)
    {
      /* Use GetDpiForMonitor() for Windows 8.1+, when we have a HMONITOR */
      if (win32_display->shcore_funcs.hshcore != NULL &&
          win32_display->shcore_funcs.getDpiForMonitorFunc != NULL)
        {
          if (win32_display->shcore_funcs.getDpiForMonitorFunc (hmonitor,
                                                                MDT_EFFECTIVE_DPI,
                                                                &dpix,
                                                                &dpiy) == S_OK)
            {
              is_scale_acquired = TRUE;
            }
        }
    }
  else
    {
      /* Go back to GetDeviceCaps() for Windows 8 and earler, or when we don't
       * have a HMONITOR nor a HWND
       */
      HDC hdc = GetDC (hwnd);

      /* in case we can't get the DC for the window, return 1 for the scale */
      if (hdc == NULL)
        {
          if (dpi != NULL)
            *dpi = USER_DEFAULT_SCREEN_DPI;

          return 1;
        }

      dpix = GetDeviceCaps (hdc, LOGPIXELSX);
      dpiy = GetDeviceCaps (hdc, LOGPIXELSY);
      ReleaseDC (hwnd, hdc);

      is_scale_acquired = TRUE;
    }

  if (is_scale_acquired)
    /* USER_DEFAULT_SCREEN_DPI = 96, in winuser.h */
    {
      if (dpi != NULL)
        *dpi = dpix;

      if (win32_display->has_fixed_scale)
        return win32_display->window_scale;
      else
        return dpix / USER_DEFAULT_SCREEN_DPI > 1 ? dpix / USER_DEFAULT_SCREEN_DPI : 1;
    }
  else
    {
      if (dpi != NULL)
        *dpi = USER_DEFAULT_SCREEN_DPI;

      return 1;
  }
}

static void
cdk_win32_display_class_init (CdkWin32DisplayClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CdkDisplayClass *display_class = CDK_DISPLAY_CLASS (klass);

  object_class->dispose = cdk_win32_display_dispose;
  object_class->finalize = cdk_win32_display_finalize;

  display_class->window_type = CDK_TYPE_WIN32_WINDOW;

  display_class->get_name = cdk_win32_display_get_name;
  display_class->get_default_screen = cdk_win32_display_get_default_screen;
  display_class->beep = cdk_win32_display_beep;
  display_class->sync = cdk_win32_display_sync;
  display_class->flush = cdk_win32_display_flush;
  display_class->has_pending = _cdk_win32_display_has_pending;
  display_class->queue_events = _cdk_win32_display_queue_events;
  display_class->get_default_group = cdk_win32_display_get_default_group;

  display_class->supports_selection_notification = cdk_win32_display_supports_selection_notification;
  display_class->request_selection_notification = cdk_win32_display_request_selection_notification;
  display_class->supports_clipboard_persistence = cdk_win32_display_supports_clipboard_persistence;
  display_class->store_clipboard = cdk_win32_display_store_clipboard;
  display_class->supports_shapes = cdk_win32_display_supports_shapes;
  display_class->supports_input_shapes = cdk_win32_display_supports_input_shapes;
  display_class->supports_composite = cdk_win32_display_supports_composite;

  //? display_class->get_app_launch_context = _cdk_win32_display_get_app_launch_context;
  display_class->get_cursor_for_type = _cdk_win32_display_get_cursor_for_type;
  display_class->get_cursor_for_name = _cdk_win32_display_get_cursor_for_name;
  display_class->get_cursor_for_surface = _cdk_win32_display_get_cursor_for_surface;
  display_class->get_default_cursor_size = _cdk_win32_display_get_default_cursor_size;
  display_class->get_maximal_cursor_size = _cdk_win32_display_get_maximal_cursor_size;
  display_class->supports_cursor_alpha = _cdk_win32_display_supports_cursor_alpha;
  display_class->supports_cursor_color = _cdk_win32_display_supports_cursor_color;

  display_class->before_process_all_updates = cdk_win32_display_before_process_all_updates;
  display_class->after_process_all_updates = cdk_win32_display_after_process_all_updates;
  display_class->get_next_serial = cdk_win32_display_get_next_serial;
  display_class->notify_startup_complete = cdk_win32_display_notify_startup_complete;
  display_class->create_window_impl = _cdk_win32_display_create_window_impl;

  display_class->get_keymap = _cdk_win32_display_get_keymap;
  display_class->push_error_trap = cdk_win32_display_push_error_trap;
  display_class->pop_error_trap = cdk_win32_display_pop_error_trap;
  display_class->get_selection_owner = _cdk_win32_display_get_selection_owner;
  display_class->set_selection_owner = _cdk_win32_display_set_selection_owner;
  display_class->send_selection_notify = _cdk_win32_display_send_selection_notify;
  display_class->get_selection_property = _cdk_win32_display_get_selection_property;
  display_class->convert_selection = _cdk_win32_display_convert_selection;
  display_class->text_property_to_utf8_list = _cdk_win32_display_text_property_to_utf8_list;
  display_class->utf8_to_string_target = _cdk_win32_display_utf8_to_string_target;
  display_class->make_gl_context_current = _cdk_win32_display_make_gl_context_current;

  display_class->get_n_monitors = cdk_win32_display_get_n_monitors;
  display_class->get_monitor = cdk_win32_display_get_monitor;
  display_class->get_primary_monitor = cdk_win32_display_get_primary_monitor;

  _cdk_win32_windowing_init ();
}
