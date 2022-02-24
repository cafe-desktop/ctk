/* CDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 1998-2002 Tor Lillqvist
 * Copyright (C) 2001,2009 Hans Breuer
 * Copyright (C) 2007-2009 Cody Russell
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

/* Cannot use TrackMouseEvent, as the stupid WM_MOUSELEAVE message
 * doesn’t tell us where the mouse has gone. Thus we cannot use it to
 * generate a correct CdkNotifyType. Pity, as using TrackMouseEvent
 * otherwise would make it possible to reliably generate
 * CDK_LEAVE_NOTIFY events, which would help get rid of those pesky
 * tooltips sometimes popping up in the wrong place.
 *
 * Update: a combination of TrackMouseEvent, GetCursorPos and
 * GetWindowPos can and is actually used to get rid of those
 * pesky tooltips. It should be possible to use this for the
 * whole ENTER/LEAVE NOTIFY handling but some platforms may
 * not have TrackMouseEvent at all (?) --hb
 */

#include "config.h"

#include "cdkprivate-win32.h"

#include <glib/gprintf.h>
#include <cairo-win32.h>

#include "cdk.h"
#include "cdkdisplayprivate.h"
#include "cdkmonitorprivate.h"
#include "cdkwin32.h"
#include "cdkkeysyms.h"
#include "cdkglcontext-win32.h"
#include "cdkdevicemanager-win32.h"
#include "cdkdeviceprivate.h"
#include "cdkdevice-wintab.h"
#include "cdkwin32dnd.h"
#include "cdkdisplay-win32.h"
#include "cdkselection-win32.h"
#include "cdkdndprivate.h"

#include <windowsx.h>

#ifdef G_WITH_CYGWIN
#include <fcntl.h>
#include <errno.h>
#endif

#include <objbase.h>

#include <imm.h>

#ifndef XBUTTON1
#define XBUTTON1 1
#define XBUTTON2 2
#endif

#ifndef VK_XBUTTON1
#define VK_XBUTTON1 5
#define VK_XBUTTON2 6
#endif

#ifndef MK_XBUTTON1
#define MK_XBUTTON1 32
#define MK_XBUTTON2 64
#endif

/* Undefined flags: */
#define SWP_NOCLIENTSIZE 0x0800
#define SWP_NOCLIENTMOVE 0x1000
#define SWP_STATECHANGED 0x8000
/*
 * Private function declarations
 */

#define SYNAPSIS_ICON_WINDOW_CLASS "SynTrackCursorWindowClass"

static gboolean cdk_event_translate (MSG        *msg,
				     gint       *ret_valp);
static gboolean cdk_event_prepare  (GSource     *source,
				    gint        *timeout);
static gboolean cdk_event_check    (GSource     *source);
static gboolean cdk_event_dispatch (GSource     *source,
				    GSourceFunc  callback,
				    gpointer     user_data);

/* Private variable declarations
 */

static GList *client_filters;	/* Filters for client messages */
extern gint       _cdk_input_ignore_core;

CdkCursor *_cdk_win32_grab_cursor;

typedef struct
{
  GSource source;

  CdkDisplay *display;
  GPollFD event_poll_fd;
} CdkWin32EventSource;

static GSourceFuncs event_funcs = {
  cdk_event_prepare,
  cdk_event_check,
  cdk_event_dispatch,
  NULL
};

static CdkWindow *mouse_window = NULL;
static CdkWindow *mouse_window_ignored_leave = NULL;
static gint current_x, current_y;
static gint current_root_x, current_root_y;
static UINT client_message;

static UINT got_cdk_events_message;
static HWND modal_win32_dialog = NULL;

#if 0
static HKL latin_locale = NULL;
#endif

static gboolean in_ime_composition = FALSE;
static UINT     modal_timer;
static UINT     sync_timer = 0;

static int debug_indent = 0;

static int both_shift_pressed[2]; /* to store keycodes for shift keys */

/* low-level keyboard hook handle */
static HHOOK keyboard_hook = NULL;
static UINT aerosnap_message;

static void
track_mouse_event (DWORD dwFlags,
		   HWND  hwnd)
{
  TRACKMOUSEEVENT tme;

  tme.cbSize = sizeof(TRACKMOUSEEVENT);
  tme.dwFlags = dwFlags;
  tme.hwndTrack = hwnd;
  tme.dwHoverTime = HOVER_DEFAULT; /* not used */

  if (!TrackMouseEvent (&tme))
    WIN32_API_FAILED ("TrackMouseEvent");
  else if (dwFlags == TME_LEAVE)
    CDK_NOTE (EVENTS, g_print(" (TrackMouseEvent %p)", hwnd));
  else if (dwFlags == TME_CANCEL)
    CDK_NOTE (EVENTS, g_print(" (cancel TrackMouseEvent %p)", hwnd));
}

gulong
_cdk_win32_get_next_tick (gulong suggested_tick)
{
  static gulong cur_tick = 0;

  if (suggested_tick == 0)
    suggested_tick = GetTickCount ();
  /* Ticks eventually wrap around.
   * This works as long as the interval between ticks is < 2147483648ms */
  if (suggested_tick <= cur_tick && ((cur_tick - suggested_tick) < 0x7FFFFFFF))
    return cur_tick;
  else
    return cur_tick = suggested_tick;
}

static void
generate_focus_event (CdkDeviceManager *device_manager,
                      CdkWindow        *window,
                      gboolean          in)
{
  CdkDevice *device;
  CdkDevice *source_device;
  CdkEvent *event;

  device = CDK_DEVICE_MANAGER_WIN32 (device_manager)->core_keyboard;
  source_device = CDK_DEVICE_MANAGER_WIN32 (device_manager)->system_keyboard;

  event = cdk_event_new (CDK_FOCUS_CHANGE);
  event->focus_change.window = window;
  event->focus_change.in = in;
  cdk_event_set_device (event, device);
  cdk_event_set_source_device (event, source_device);
  cdk_event_set_seat (event, cdk_device_get_seat (device));

  _cdk_win32_append_event (event);
}

static void
generate_grab_broken_event (CdkDeviceManager *device_manager,
                            CdkWindow        *window,
                            gboolean          keyboard,
                            CdkWindow        *grab_window)
{
  CdkEvent *event = cdk_event_new (CDK_GRAB_BROKEN);
  CdkDevice *device;
  CdkDevice *source_device;

  if (keyboard)
    {
      device = CDK_DEVICE_MANAGER_WIN32 (device_manager)->core_keyboard;
      source_device = CDK_DEVICE_MANAGER_WIN32 (device_manager)->system_keyboard;
    }
  else
    {
      device = CDK_DEVICE_MANAGER_WIN32 (device_manager)->core_pointer;
      source_device = CDK_DEVICE_MANAGER_WIN32 (device_manager)->system_pointer;
    }

  event->grab_broken.window = window;
  event->grab_broken.send_event = 0;
  event->grab_broken.keyboard = keyboard;
  event->grab_broken.implicit = FALSE;
  event->grab_broken.grab_window = grab_window;
  cdk_event_set_device (event, device);
  cdk_event_set_source_device (event, source_device);
  cdk_event_set_seat (event, cdk_device_get_seat (device));

  _cdk_win32_append_event (event);
}

static LRESULT
inner_window_procedure (HWND   hwnd,
			UINT   message,
			WPARAM wparam,
			LPARAM lparam)
{
  MSG msg;
  DWORD pos;
  gint ret_val = 0;

  msg.hwnd = hwnd;
  msg.message = message;
  msg.wParam = wparam;
  msg.lParam = lparam;
  msg.time = _cdk_win32_get_next_tick (0);
  pos = GetMessagePos ();
  msg.pt.x = GET_X_LPARAM (pos);
  msg.pt.y = GET_Y_LPARAM (pos);

  if (cdk_event_translate (&msg, &ret_val))
    {
      /* If cdk_event_translate() returns TRUE, we return ret_val from
       * the window procedure.
       */
      if (modal_win32_dialog)
	PostMessageW (modal_win32_dialog, got_cdk_events_message,
		      (WPARAM) 1, 0);
      return ret_val;
    }
  else
    {
      /* Otherwise call DefWindowProcW(). */
      CDK_NOTE (EVENTS, g_print (" DefWindowProcW"));
      return DefWindowProcW (hwnd, message, wparam, lparam);
    }
}

LRESULT CALLBACK
_cdk_win32_window_procedure (HWND   hwnd,
                             UINT   message,
                             WPARAM wparam,
                             LPARAM lparam)
{
  LRESULT retval;

  CDK_NOTE (EVENTS, g_print ("%s%*s%s %p %#x %#lx",
			     (debug_indent > 0 ? "\n" : ""),
			     debug_indent, "",
			     _cdk_win32_message_to_string (message), hwnd,
			     wparam, lparam));
  debug_indent += 2;
  retval = inner_window_procedure (hwnd, message, wparam, lparam);
  debug_indent -= 2;

  CDK_NOTE (EVENTS, g_print (" => %" G_GINT64_FORMAT "%s", (gint64) retval, (debug_indent == 0 ? "\n" : "")));

  return retval;
}

static LRESULT
low_level_keystroke_handler (WPARAM message,
                                       KBDLLHOOKSTRUCT *kbdhook,
                                       CdkWindow *window)
{
  CdkWindow *toplevel = cdk_window_get_toplevel (window);
  static DWORD last_keydown = 0;

  if (message == WM_KEYDOWN &&
      !CDK_WINDOW_DESTROYED (toplevel) &&
      _cdk_win32_window_lacks_wm_decorations (toplevel) && /* For CSD only */
      last_keydown != kbdhook->vkCode &&
      ((GetKeyState (VK_LWIN) & 0x8000) ||
      (GetKeyState (VK_RWIN) & 0x8000)))
	{
	  CdkWin32AeroSnapCombo combo = CDK_WIN32_AEROSNAP_COMBO_NOTHING;
	  gboolean lshiftdown = GetKeyState (VK_LSHIFT) & 0x8000;
          gboolean rshiftdown = GetKeyState (VK_RSHIFT) & 0x8000;
          gboolean oneshiftdown = (lshiftdown || rshiftdown) && !(lshiftdown && rshiftdown);
          gboolean maximized = cdk_window_get_state (toplevel) & CDK_WINDOW_STATE_MAXIMIZED;

	  switch (kbdhook->vkCode)
	    {
	    case VK_UP:
	      combo = CDK_WIN32_AEROSNAP_COMBO_UP;
	      break;
	    case VK_DOWN:
	      combo = CDK_WIN32_AEROSNAP_COMBO_DOWN;
	      break;
	    case VK_LEFT:
	      combo = CDK_WIN32_AEROSNAP_COMBO_LEFT;
	      break;
	    case VK_RIGHT:
	      combo = CDK_WIN32_AEROSNAP_COMBO_RIGHT;
	      break;
	    }

	  if (oneshiftdown && combo != CDK_WIN32_AEROSNAP_COMBO_NOTHING)
	    combo += 4;

	  /* These are the only combos that Windows WM does handle for us */
	  if (combo == CDK_WIN32_AEROSNAP_COMBO_SHIFTLEFT ||
              combo == CDK_WIN32_AEROSNAP_COMBO_SHIFTRIGHT)
            combo = CDK_WIN32_AEROSNAP_COMBO_NOTHING;

          /* On Windows 10 the WM will handle this specific combo */
          if (combo == CDK_WIN32_AEROSNAP_COMBO_DOWN && maximized &&
              g_win32_check_windows_version (6, 4, 0, G_WIN32_OS_ANY))
            combo = CDK_WIN32_AEROSNAP_COMBO_NOTHING;

	  if (combo != CDK_WIN32_AEROSNAP_COMBO_NOTHING)
            PostMessage (CDK_WINDOW_HWND (toplevel), aerosnap_message, (WPARAM) combo, 0);
	}

  if (message == WM_KEYDOWN)
    last_keydown = kbdhook->vkCode;
  else if (message = WM_KEYUP && last_keydown == kbdhook->vkCode)
    last_keydown = 0;

  return 0;
}

static LRESULT CALLBACK
low_level_keyboard_proc (int    code,
                         WPARAM wParam,
                         LPARAM lParam)
{
  HWND kbd_focus_owner;
  LRESULT chain;

  do
  {
    KBDLLHOOKSTRUCT *kbdhook;
    CdkWindow *cdk_kbd_focus_owner;

    if (code < 0)
      break;

    kbd_focus_owner = GetFocus ();

    if (kbd_focus_owner == NULL)
      break;

    cdk_kbd_focus_owner = cdk_win32_handle_table_lookup (kbd_focus_owner);

    if (cdk_kbd_focus_owner == NULL)
      break;

    kbdhook = (KBDLLHOOKSTRUCT *) lParam;
    chain = low_level_keystroke_handler (wParam, kbdhook, cdk_kbd_focus_owner);

    if (chain != 0)
      return chain;
  } while (FALSE);

  return CallNextHookEx (0, code, wParam, lParam);
}

static void
set_up_low_level_keyboard_hook (void)
{
  HHOOK hook_handle;

  if (keyboard_hook != NULL)
    return;

  hook_handle = SetWindowsHookEx (WH_KEYBOARD_LL,
                                  (HOOKPROC) low_level_keyboard_proc,
                                  _cdk_dll_hinstance,
                                  0);

  if (hook_handle != NULL)
    keyboard_hook = hook_handle;
  else
    WIN32_API_FAILED ("SetWindowsHookEx");

  aerosnap_message = RegisterWindowMessage ("CDK_WIN32_AEROSNAP_MESSAGE");
}

void
_cdk_events_init (CdkDisplay *display)
{
  GSource *source;
  CdkWin32EventSource *event_source;

#if 0
  int i, j, n;

  /* List of languages that use a latin keyboard. Somewhat sorted in
   * "order of least surprise", in case we have to load one of them if
   * the user only has arabic loaded, for instance.
   */
  static int latin_languages[] = {
    LANG_ENGLISH,
    LANG_SPANISH,
    LANG_PORTUGUESE,
    LANG_FRENCH,
    LANG_GERMAN,
    /* Rest in numeric order */
    LANG_CZECH,
    LANG_DANISH,
    LANG_FINNISH,
    LANG_HUNGARIAN,
    LANG_ICELANDIC,
    LANG_ITALIAN,
    LANG_DUTCH,
    LANG_NORWEGIAN,
    LANG_POLISH,
    LANG_ROMANIAN,
    LANG_SLOVAK,
    LANG_ALBANIAN,
    LANG_SWEDISH,
    LANG_TURKISH,
    LANG_INDONESIAN,
    LANG_SLOVENIAN,
    LANG_ESTONIAN,
    LANG_LATVIAN,
    LANG_LITHUANIAN,
    LANG_VIETNAMESE,
    LANG_AFRIKAANS,
    LANG_FAEROESE
#ifdef LANG_SWAHILI
   ,LANG_SWAHILI
#endif
  };
#endif

  client_message = RegisterWindowMessage ("CDK_WIN32_CLIENT_MESSAGE");
  got_cdk_events_message = RegisterWindowMessage ("CDK_WIN32_GOT_EVENTS");

#if 0
  /* Check if we have some input locale identifier loaded that uses a
   * latin keyboard, to be able to get the virtual-key code for the
   * latin characters corresponding to ASCII control characters.
   */
  if ((n = GetKeyboardLayoutList (0, NULL)) == 0)
    WIN32_API_FAILED ("GetKeyboardLayoutList");
  else
    {
      HKL *hkl_list = g_new (HKL, n);
      if (GetKeyboardLayoutList (n, hkl_list) == 0)
	WIN32_API_FAILED ("GetKeyboardLayoutList");
      else
	{
	  for (i = 0; latin_locale == NULL && i < n; i++)
	    for (j = 0; j < G_N_ELEMENTS (latin_languages); j++)
	      if (PRIMARYLANGID (LOWORD (hkl_list[i])) == latin_languages[j])
		{
		  latin_locale = hkl_list [i];
		  break;
		}
	}
      g_free (hkl_list);
    }

  if (latin_locale == NULL)
    {
      /* Try to load a keyboard layout with latin characters then.
       */
      i = 0;
      while (latin_locale == NULL && i < G_N_ELEMENTS (latin_languages))
	{
	  char id[9];
	  g_sprintf (id, "%08x", MAKELANGID (latin_languages[i++], SUBLANG_DEFAULT));
	  latin_locale = LoadKeyboardLayout (id, KLF_NOTELLSHELL|KLF_SUBSTITUTE_OK);
	}
    }

  CDK_NOTE (EVENTS, g_print ("latin_locale = %08x\n", (guint) latin_locale));
#endif

  source = g_source_new (&event_funcs, sizeof (CdkWin32EventSource));
  g_source_set_name (source, "CDK Win32 event source");
  g_source_set_priority (source, CDK_PRIORITY_EVENTS);

  event_source = (CdkWin32EventSource *)source;
  event_source->display = display;
#ifdef G_WITH_CYGWIN
  event_source->event_poll_fd.fd = open ("/dev/windows", O_RDONLY);
  if (event_source->event_poll_fd.fd == -1)
    g_error ("can't open \"/dev/windows\": %s", g_strerror (errno));
#else
  event_source->event_poll_fd.fd = G_WIN32_MSG_HANDLE;
#endif
  event_source->event_poll_fd.events = G_IO_IN;

  g_source_add_poll (source, &event_source->event_poll_fd);
  g_source_set_can_recurse (source, TRUE);
  g_source_attach (source, NULL);

  set_up_low_level_keyboard_hook ();
}

gboolean
_cdk_win32_display_has_pending (CdkDisplay *display)
{
  return (_cdk_event_queue_find_first (display) ||
	  (modal_win32_dialog == NULL &&
	   GetQueueStatus (QS_ALLINPUT) != 0));
}

#if 0 /* Unused, but might be useful to re-introduce in some debugging output? */

static char *
event_mask_string (CdkEventMask mask)
{
  static char bfr[500];
  char *p = bfr;

  *p = '\0';
#define BIT(x) \
  if (mask & CDK_##x##_MASK) \
    p += g_sprintf (p, "%s" #x, (p > bfr ? " " : ""))
  BIT (EXPOSURE);
  BIT (POINTER_MOTION);
  BIT (POINTER_MOTION_HINT);
  BIT (BUTTON_MOTION);
  BIT (BUTTON1_MOTION);
  BIT (BUTTON2_MOTION);
  BIT (BUTTON3_MOTION);
  BIT (BUTTON_PRESS);
  BIT (BUTTON_RELEASE);
  BIT (KEY_PRESS);
  BIT (KEY_RELEASE);
  BIT (ENTER_NOTIFY);
  BIT (LEAVE_NOTIFY);
  BIT (FOCUS_CHANGE);
  BIT (STRUCTURE);
  BIT (PROPERTY_CHANGE);
  BIT (VISIBILITY_NOTIFY);
  BIT (PROXIMITY_IN);
  BIT (PROXIMITY_OUT);
  BIT (SUBSTRUCTURE);
  BIT (SCROLL);
#undef BIT

  return bfr;
}

#endif

static CdkWindow *
find_window_for_mouse_event (CdkWindow* reported_window,
			     MSG*       msg)
{
  POINT pt;
  CdkDisplay *display;
  CdkDeviceManagerWin32 *device_manager;
  CdkWindow *event_window;
  HWND hwnd;
  RECT rect;
  CdkDeviceGrabInfo *grab;

  display = cdk_display_get_default ();
  device_manager = CDK_DEVICE_MANAGER_WIN32 (cdk_display_get_device_manager (display));

  grab = _cdk_display_get_last_device_grab (display, device_manager->core_pointer);
  if (grab == NULL)
    return reported_window;

  pt = msg->pt;

  if (!grab->owner_events)
    event_window = grab->native_window;
  else
    {
      event_window = NULL;
      hwnd = WindowFromPoint (pt);
      if (hwnd != NULL)
	{
	  POINT client_pt = pt;

	  ScreenToClient (hwnd, &client_pt);
	  GetClientRect (hwnd, &rect);
	  if (PtInRect (&rect, client_pt))
	    event_window = cdk_win32_handle_table_lookup (hwnd);
	}
      if (event_window == NULL)
	event_window = grab->native_window;
    }

  /* need to also adjust the coordinates to the new window */
  ScreenToClient (CDK_WINDOW_HWND (event_window), &pt);

  /* ATTENTION: need to update client coords */
  msg->lParam = MAKELPARAM (pt.x, pt.y);

  return event_window;
}

static void
build_key_event_state (CdkEvent *event,
		       BYTE     *key_state)
{
  CdkWin32Keymap *keymap;

  event->key.state = 0;

  if (key_state[VK_SHIFT] & 0x80)
    event->key.state |= CDK_SHIFT_MASK;

  if (key_state[VK_CAPITAL] & 0x01)
    event->key.state |= CDK_LOCK_MASK;

  if (key_state[VK_LBUTTON] & 0x80)
    event->key.state |= CDK_BUTTON1_MASK;
  if (key_state[VK_MBUTTON] & 0x80)
    event->key.state |= CDK_BUTTON2_MASK;
  if (key_state[VK_RBUTTON] & 0x80)
    event->key.state |= CDK_BUTTON3_MASK;
  if (key_state[VK_XBUTTON1] & 0x80)
    event->key.state |= CDK_BUTTON4_MASK;
  if (key_state[VK_XBUTTON2] & 0x80)
    event->key.state |= CDK_BUTTON5_MASK;

  keymap = CDK_WIN32_KEYMAP (_cdk_win32_display_get_keymap (_cdk_display));
  event->key.group = _cdk_win32_keymap_get_active_group (keymap);

  if (_cdk_win32_keymap_has_altgr (keymap) &&
      (key_state[VK_LCONTROL] & 0x80) &&
      (key_state[VK_RMENU] & 0x80))
    {
      event->key.state |= CDK_MOD2_MASK;
      if (key_state[VK_RCONTROL] & 0x80)
	event->key.state |= CDK_CONTROL_MASK;
      if (key_state[VK_LMENU] & 0x80)
	event->key.state |= CDK_MOD1_MASK;
    }
  else
    {
      if (key_state[VK_CONTROL] & 0x80)
	event->key.state |= CDK_CONTROL_MASK;
      if (key_state[VK_MENU] & 0x80)
	event->key.state |= CDK_MOD1_MASK;
    }
}

static gint
build_pointer_event_state (MSG *msg)
{
  gint state;

  state = 0;

  if (msg->wParam & MK_CONTROL)
    state |= CDK_CONTROL_MASK;

  if ((msg->message != WM_LBUTTONDOWN &&
       (msg->wParam & MK_LBUTTON)) ||
      msg->message == WM_LBUTTONUP)
    state |= CDK_BUTTON1_MASK;

  if ((msg->message != WM_MBUTTONDOWN &&
       (msg->wParam & MK_MBUTTON)) ||
      msg->message == WM_MBUTTONUP)
    state |= CDK_BUTTON2_MASK;

  if ((msg->message != WM_RBUTTONDOWN &&
       (msg->wParam & MK_RBUTTON)) ||
      msg->message == WM_RBUTTONUP)
    state |= CDK_BUTTON3_MASK;

  if (((msg->message != WM_XBUTTONDOWN || HIWORD (msg->wParam) != XBUTTON1) &&
       (msg->wParam & MK_XBUTTON1)) ||
      (msg->message == WM_XBUTTONUP && HIWORD (msg->wParam) == XBUTTON1))
    state |= CDK_BUTTON4_MASK;

  if (((msg->message != WM_XBUTTONDOWN || HIWORD (msg->wParam) != XBUTTON2) &&
       (msg->wParam & MK_XBUTTON2)) ||
      (msg->message == WM_XBUTTONUP && HIWORD (msg->wParam) == XBUTTON2))
    state |= CDK_BUTTON5_MASK;

  if (msg->wParam & MK_SHIFT)
    state |= CDK_SHIFT_MASK;

  if (GetKeyState (VK_MENU) < 0)
    state |= CDK_MOD1_MASK;

  if (GetKeyState (VK_CAPITAL) & 0x1)
    state |= CDK_LOCK_MASK;

  return state;
}

static void
build_wm_ime_composition_event (CdkEvent *event,
				MSG      *msg,
				wchar_t   wc,
				BYTE     *key_state)
{
  event->key.time = _cdk_win32_get_next_tick (msg->time);

  build_key_event_state (event, key_state);

  event->key.hardware_keycode = 0; /* FIXME: What should it be? */
  event->key.string = NULL;
  event->key.length = 0;
  event->key.keyval = cdk_unicode_to_keyval (wc);
}

#ifdef G_ENABLE_DEBUG

static void
print_event_state (guint state)
{
#define CASE(bit) if (state & CDK_ ## bit ## _MASK) g_print (#bit " ");
  CASE (SHIFT);
  CASE (LOCK);
  CASE (CONTROL);
  CASE (MOD1);
  CASE (MOD2);
  CASE (MOD3);
  CASE (MOD4);
  CASE (MOD5);
  CASE (BUTTON1);
  CASE (BUTTON2);
  CASE (BUTTON3);
  CASE (BUTTON4);
  CASE (BUTTON5);
#undef CASE
}

void
_cdk_win32_print_event (const CdkEvent *event)
{
  gchar *escaped, *kvname;
  gchar *selection_name, *target_name, *property_name;

  g_print ("%s%*s===> ", (debug_indent > 0 ? "\n" : ""), debug_indent, "");
  switch (event->any.type)
    {
#define CASE(x) case x: g_print (#x); break;
    CASE (CDK_NOTHING);
    CASE (CDK_DELETE);
    CASE (CDK_DESTROY);
    CASE (CDK_EXPOSE);
    CASE (CDK_MOTION_NOTIFY);
    CASE (CDK_BUTTON_PRESS);
    CASE (CDK_2BUTTON_PRESS);
    CASE (CDK_3BUTTON_PRESS);
    CASE (CDK_BUTTON_RELEASE);
    CASE (CDK_KEY_PRESS);
    CASE (CDK_KEY_RELEASE);
    CASE (CDK_ENTER_NOTIFY);
    CASE (CDK_LEAVE_NOTIFY);
    CASE (CDK_FOCUS_CHANGE);
    CASE (CDK_CONFIGURE);
    CASE (CDK_MAP);
    CASE (CDK_UNMAP);
    CASE (CDK_PROPERTY_NOTIFY);
    CASE (CDK_SELECTION_CLEAR);
    CASE (CDK_SELECTION_REQUEST);
    CASE (CDK_SELECTION_NOTIFY);
    CASE (CDK_PROXIMITY_IN);
    CASE (CDK_PROXIMITY_OUT);
    CASE (CDK_DRAG_ENTER);
    CASE (CDK_DRAG_LEAVE);
    CASE (CDK_DRAG_MOTION);
    CASE (CDK_DRAG_STATUS);
    CASE (CDK_DROP_START);
    CASE (CDK_DROP_FINISHED);
    CASE (CDK_CLIENT_EVENT);
    CASE (CDK_VISIBILITY_NOTIFY);
    CASE (CDK_SCROLL);
    CASE (CDK_WINDOW_STATE);
    CASE (CDK_SETTING);
    CASE (CDK_OWNER_CHANGE);
    CASE (CDK_GRAB_BROKEN);
#undef CASE
    default: g_assert_not_reached ();
    }

  g_print (" %p @ %ums ",
           event->any.window ? CDK_WINDOW_HWND (event->any.window) : NULL,
           cdk_event_get_time (event));

  switch (event->any.type)
    {
    case CDK_EXPOSE:
      g_print ("%s %d",
	       _cdk_win32_cdkrectangle_to_string (&event->expose.area),
	       event->expose.count);
      break;
    case CDK_MOTION_NOTIFY:
      g_print ("(%.4g,%.4g) (%.4g,%.4g) %s",
	       event->motion.x, event->motion.y,
	       event->motion.x_root, event->motion.y_root,
	       event->motion.is_hint ? "HINT " : "");
      print_event_state (event->motion.state);
      break;
    case CDK_BUTTON_PRESS:
    case CDK_2BUTTON_PRESS:
    case CDK_3BUTTON_PRESS:
    case CDK_BUTTON_RELEASE:
      g_print ("%d (%.4g,%.4g) (%.4g,%.4g) ",
	       event->button.button,
	       event->button.x, event->button.y,
	       event->button.x_root, event->button.y_root);
      print_event_state (event->button.state);
      break;
    case CDK_KEY_PRESS:
    case CDK_KEY_RELEASE:
      if (event->key.length == 0)
	escaped = g_strdup ("");
      else
	escaped = g_strescape (event->key.string, NULL);
      kvname = cdk_keyval_name (event->key.keyval);
      g_print ("%#.02x group:%d %s %d:\"%s\" ",
	       event->key.hardware_keycode, event->key.group,
	       (kvname ? kvname : "??"),
	       event->key.length,
	       escaped);
      g_free (escaped);
      print_event_state (event->key.state);
      break;
    case CDK_ENTER_NOTIFY:
    case CDK_LEAVE_NOTIFY:
      g_print ("%p (%.4g,%.4g) (%.4g,%.4g) %s %s%s",
	       event->crossing.subwindow == NULL ? NULL : CDK_WINDOW_HWND (event->crossing.subwindow),
	       event->crossing.x, event->crossing.y,
	       event->crossing.x_root, event->crossing.y_root,
	       (event->crossing.mode == CDK_CROSSING_NORMAL ? "NORMAL" :
		(event->crossing.mode == CDK_CROSSING_GRAB ? "GRAB" :
		 (event->crossing.mode == CDK_CROSSING_UNGRAB ? "UNGRAB" :
		  "???"))),
	       (event->crossing.detail == CDK_NOTIFY_ANCESTOR ? "ANCESTOR" :
		(event->crossing.detail == CDK_NOTIFY_VIRTUAL ? "VIRTUAL" :
		 (event->crossing.detail == CDK_NOTIFY_INFERIOR ? "INFERIOR" :
		  (event->crossing.detail == CDK_NOTIFY_NONLINEAR ? "NONLINEAR" :
		   (event->crossing.detail == CDK_NOTIFY_NONLINEAR_VIRTUAL ? "NONLINEAR_VIRTUAL" :
		    (event->crossing.detail == CDK_NOTIFY_UNKNOWN ? "UNKNOWN" :
		     "???")))))),
	       event->crossing.focus ? " FOCUS" : "");
      print_event_state (event->crossing.state);
      break;
    case CDK_FOCUS_CHANGE:
      g_print ("%s", (event->focus_change.in ? "IN" : "OUT"));
      break;
    case CDK_CONFIGURE:
      g_print ("x:%d y:%d w:%d h:%d",
	       event->configure.x, event->configure.y,
	       event->configure.width, event->configure.height);
      break;
    case CDK_SELECTION_CLEAR:
    case CDK_SELECTION_REQUEST:
    case CDK_SELECTION_NOTIFY:
      selection_name = cdk_atom_name (event->selection.selection);
      target_name = cdk_atom_name (event->selection.target);
      property_name = cdk_atom_name (event->selection.property);
      g_print ("sel:%s tgt:%s prop:%s",
	       selection_name, target_name, property_name);
      g_free (selection_name);
      g_free (target_name);
      g_free (property_name);
      break;
    case CDK_DRAG_ENTER:
    case CDK_DRAG_LEAVE:
    case CDK_DRAG_MOTION:
    case CDK_DRAG_STATUS:
    case CDK_DROP_START:
    case CDK_DROP_FINISHED:
      if (event->dnd.context != NULL)
	g_print ("ctx:%p: %s %s src:%p dest:%p",
		 event->dnd.context,
		 _cdk_win32_drag_protocol_to_string (event->dnd.context->protocol),
		 event->dnd.context->is_source ? "SOURCE" : "DEST",
		 event->dnd.context->source_window == NULL ? NULL : CDK_WINDOW_HWND (event->dnd.context->source_window),
		 event->dnd.context->dest_window == NULL ? NULL : CDK_WINDOW_HWND (event->dnd.context->dest_window));
      break;
    case CDK_CLIENT_EVENT:
      /* no more CdkEventClient */
      break;
    case CDK_SCROLL:
      g_print ("(%.4g,%.4g) (%.4g,%.4g) %s ",
	       event->scroll.x, event->scroll.y,
	       event->scroll.x_root, event->scroll.y_root,
	       (event->scroll.direction == CDK_SCROLL_UP ? "UP" :
		(event->scroll.direction == CDK_SCROLL_DOWN ? "DOWN" :
		 (event->scroll.direction == CDK_SCROLL_LEFT ? "LEFT" :
		  (event->scroll.direction == CDK_SCROLL_RIGHT ? "RIGHT" :
		   "???")))));
      print_event_state (event->scroll.state);
      break;
    case CDK_WINDOW_STATE:
      g_print ("%s: %s",
	       _cdk_win32_window_state_to_string (event->window_state.changed_mask),
	       _cdk_win32_window_state_to_string (event->window_state.new_window_state));
    case CDK_SETTING:
      g_print ("%s: %s",
	       (event->setting.action == CDK_SETTING_ACTION_NEW ? "NEW" :
		(event->setting.action == CDK_SETTING_ACTION_CHANGED ? "CHANGED" :
		 (event->setting.action == CDK_SETTING_ACTION_DELETED ? "DELETED" :
		  "???"))),
	       (event->setting.name ? event->setting.name : "NULL"));
    case CDK_GRAB_BROKEN:
      g_print ("%s %s %p",
	       (event->grab_broken.keyboard ? "KEYBOARD" : "POINTER"),
	       (event->grab_broken.implicit ? "IMPLICIT" : "EXPLICIT"),
	       (event->grab_broken.grab_window ? CDK_WINDOW_HWND (event->grab_broken.grab_window) : 0));
    default:
      /* Nothing */
      break;
    }
  g_print ("%s", (debug_indent == 0 ? "\n" : ""));
}

static char *
decode_key_lparam (LPARAM lParam)
{
  static char buf[100];
  char *p = buf;

  if (HIWORD (lParam) & KF_UP)
    p += g_sprintf (p, "KF_UP ");
  if (HIWORD (lParam) & KF_REPEAT)
    p += g_sprintf (p, "KF_REPEAT ");
  if (HIWORD (lParam) & KF_ALTDOWN)
    p += g_sprintf (p, "KF_ALTDOWN ");
  if (HIWORD (lParam) & KF_EXTENDED)
    p += g_sprintf (p, "KF_EXTENDED ");
  p += g_sprintf (p, "sc:%d rep:%d", LOBYTE (HIWORD (lParam)), LOWORD (lParam));

  return buf;
}

#endif

static void
fixup_event (CdkEvent *event)
{
  if (event->any.window)
    g_object_ref (event->any.window);
  if (((event->any.type == CDK_ENTER_NOTIFY) ||
       (event->any.type == CDK_LEAVE_NOTIFY)) &&
      (event->crossing.subwindow != NULL))
    g_object_ref (event->crossing.subwindow);
  if (((event->any.type == CDK_SELECTION_CLEAR) ||
       (event->any.type == CDK_SELECTION_NOTIFY) ||
       (event->any.type == CDK_SELECTION_REQUEST)) &&
      (event->selection.requestor != NULL))
    g_object_ref (event->selection.requestor);
  if ((event->any.type == CDK_OWNER_CHANGE) &&
      (event->owner_change.owner != NULL))
    g_object_ref (event->owner_change.owner);
  event->any.send_event = InSendMessage ();
}

void
_cdk_win32_append_event (CdkEvent *event)
{
  CdkDisplay *display;
  GList *link;

  display = cdk_display_get_default ();

  fixup_event (event);
#if 1
  link = _cdk_event_queue_append (display, event);
  CDK_NOTE (EVENTS, _cdk_win32_print_event (event));
  /* event morphing, the passed in may not be valid afterwards */
  _cdk_windowing_got_event (display, link, event, 0);
#else
  _cdk_event_queue_append (display, event);
  CDK_NOTE (EVENTS, _cdk_win32_print_event (event));
#endif
}

static void
fill_key_event_string (CdkEvent *event)
{
  gunichar c;
  gchar buf[256];

  /* Fill in event->string crudely, since various programs
   * depend on it.
   */

  c = 0;
  if (event->key.keyval != CDK_KEY_VoidSymbol)
    c = cdk_keyval_to_unicode (event->key.keyval);

  if (c)
    {
      gsize bytes_written;
      gint len;

      /* Apply the control key - Taken from Xlib
       */
      if (event->key.state & CDK_CONTROL_MASK)
	{
	  if ((c >= '@' && c < '\177') || c == ' ')
	    c &= 0x1F;
	  else if (c == '2')
	    {
	      event->key.string = g_memdup2 ("\0\0", 2);
	      event->key.length = 1;
	      return;
	    }
	  else if (c >= '3' && c <= '7')
	    c -= ('3' - '\033');
	  else if (c == '8')
	    c = '\177';
	  else if (c == '/')
	    c = '_' & 0x1F;
	}

      len = g_unichar_to_utf8 (c, buf);
      buf[len] = '\0';

      event->key.string = g_locale_from_utf8 (buf, len,
					      NULL, &bytes_written,
					      NULL);
      if (event->key.string)
	event->key.length = bytes_written;
    }
  else if (event->key.keyval == CDK_KEY_Escape)
    {
      event->key.length = 1;
      event->key.string = g_strdup ("\033");
    }
  else if (event->key.keyval == CDK_KEY_Return ||
	   event->key.keyval == CDK_KEY_KP_Enter)
    {
      event->key.length = 1;
      event->key.string = g_strdup ("\r");
    }

  if (!event->key.string)
    {
      event->key.length = 0;
      event->key.string = g_strdup ("");
    }
}

static CdkFilterReturn
apply_event_filters (CdkWindow  *window,
		     MSG        *msg,
		     GList     **filters)
{
  CdkFilterReturn result = CDK_FILTER_CONTINUE;
  CdkEvent *event;
  CdkDisplay *display;
  GList *node;
  GList *tmp_list;

  event = cdk_event_new (CDK_NOTHING);
  event->any.window = g_object_ref (window);
  ((CdkEventPrivate *)event)->flags |= CDK_EVENT_PENDING;

  display = cdk_display_get_default ();

  /* I think CdkFilterFunc semantics require the passed-in event
   * to already be in the queue. The filter func can generate
   * more events and append them after it if it likes.
   */
  node = _cdk_event_queue_append (display, event);

  tmp_list = *filters;
  while (tmp_list)
    {
      CdkEventFilter *filter = (CdkEventFilter *) tmp_list->data;
      GList *node;

      if ((filter->flags & CDK_EVENT_FILTER_REMOVED) != 0)
        {
          tmp_list = tmp_list->next;
          continue;
        }

      filter->ref_count++;
      result = filter->function (msg, event, filter->data);

      /* get the next node after running the function since the
         function may add or remove a next node */
      node = tmp_list;
      tmp_list = tmp_list->next;

      filter->ref_count--;
      if (filter->ref_count == 0)
        {
          *filters = g_list_remove_link (*filters, node);
          g_list_free_1 (node);
          g_free (filter);
        }

      if (result !=  CDK_FILTER_CONTINUE)
	break;
    }

  if (result == CDK_FILTER_CONTINUE || result == CDK_FILTER_REMOVE)
    {
      _cdk_event_queue_remove_link (display, node);
      g_list_free_1 (node);
      cdk_event_free (event);
    }
  else /* CDK_FILTER_TRANSLATE */
    {
      ((CdkEventPrivate *)event)->flags &= ~CDK_EVENT_PENDING;
      fixup_event (event);
      CDK_NOTE (EVENTS, _cdk_win32_print_event (event));
    }

  return result;
}

/*
 * On Windows, transient windows will not have their own taskbar entries.
 * Because of this, we must hide and restore groups of transients in both
 * directions.  That is, all transient children must be hidden or restored
 * with this window, but if this window’s transient owner also has a
 * transient owner then this window’s transient owner must be hidden/restored
 * with this one.  And etc, up the chain until we hit an ancestor that has no
 * transient owner.
 *
 * It would be a good idea if applications don’t chain transient windows
 * together.  There’s a limit to how much evil CTK can try to shield you
 * from.
 */
static void
show_window_recurse (CdkWindow *window, gboolean hide_window)
{
  CdkWindowImplWin32 *impl = CDK_WINDOW_IMPL_WIN32 (window->impl);
  GSList *children = impl->transient_children;
  CdkWindow *child = NULL;

  if (!impl->changing_state)
    {
      impl->changing_state = TRUE;

      if (children != NULL)
	{
	  while (children != NULL)
	    {
	      child = children->data;
	      show_window_recurse (child, hide_window);

	      children = children->next;
	    }
	}

      if (CDK_WINDOW_IS_MAPPED (window))
	{
	  if (!hide_window)
	    {
	      if (cdk_window_get_state (window) & CDK_WINDOW_STATE_ICONIFIED)
		{
		  if (cdk_window_get_state (window) & CDK_WINDOW_STATE_MAXIMIZED)
		    {
		      CtkShowWindow (window, SW_SHOWMAXIMIZED);
		    }
		  else
		    {
		      CtkShowWindow (window, SW_RESTORE);
		    }
		}
	    }
	  else
	    {
	      CtkShowWindow (window, SW_MINIMIZE);
	    }
	}

      impl->changing_state = FALSE;
    }
}

static void
do_show_window (CdkWindow *window, gboolean hide_window)
{
  CdkWindow *tmp_window = NULL;
  CdkWindowImplWin32 *tmp_impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  if (!tmp_impl->changing_state)
    {
      /* Find the top-level window in our transient chain. */
      while (tmp_impl->transient_owner != NULL)
	{
	  tmp_window = tmp_impl->transient_owner;
	  tmp_impl = CDK_WINDOW_IMPL_WIN32 (tmp_window->impl);
	}

      /* If we couldn't find one, use the window provided. */
      if (tmp_window == NULL)
	{
	  tmp_window = window;
	}

      /* Recursively show/hide every window in the chain. */
      if (tmp_window != window)
	{
	  show_window_recurse (tmp_window, hide_window);
	}
    }
}

static void
send_crossing_event (CdkDisplay                 *display,
		     CdkWindow                  *window,
		     CdkEventType                type,
		     CdkCrossingMode             mode,
		     CdkNotifyType               notify_type,
		     CdkWindow                  *subwindow,
		     POINT                      *screen_pt,
		     CdkModifierType             mask,
		     guint32                     time_)
{
  CdkEvent *event;
  CdkDeviceGrabInfo *grab;
  CdkDeviceManagerWin32 *device_manager;
  POINT pt;
  CdkWindowImplWin32 *impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  device_manager = CDK_DEVICE_MANAGER_WIN32 (cdk_display_get_device_manager (display));

  grab = _cdk_display_has_device_grab (display, device_manager->core_pointer, 0);

  if (grab != NULL &&
      !grab->owner_events &&
      mode != CDK_CROSSING_UNGRAB)
    {
      /* !owner_event => only report events wrt grab window, ignore rest */
      if ((CdkWindow *)window != grab->native_window)
	return;
    }

  pt = *screen_pt;
  ScreenToClient (CDK_WINDOW_HWND (window), &pt);

  event = cdk_event_new (type);
  event->crossing.window = window;
  event->crossing.subwindow = subwindow;
  event->crossing.time = _cdk_win32_get_next_tick (time_);
  event->crossing.x = pt.x / impl->window_scale;
  event->crossing.y = pt.y / impl->window_scale;
  event->crossing.x_root = (screen_pt->x + _cdk_offset_x) / impl->window_scale;
  event->crossing.y_root = (screen_pt->y + _cdk_offset_y) / impl->window_scale;
  event->crossing.mode = mode;
  event->crossing.detail = notify_type;
  event->crossing.mode = mode;
  event->crossing.detail = notify_type;
  event->crossing.focus = FALSE;
  event->crossing.state = mask;
  cdk_event_set_device (event, device_manager->core_pointer);
  cdk_event_set_source_device (event, device_manager->system_pointer);
  cdk_event_set_seat (event, cdk_device_get_seat (device_manager->core_pointer));

  _cdk_win32_append_event (event);
}

static CdkWindow *
get_native_parent (CdkWindow *window)
{
  if (window->parent != NULL)
    return window->parent->impl_window;
  return NULL;
}

static CdkWindow *
find_common_ancestor (CdkWindow *win1,
		      CdkWindow *win2)
{
  CdkWindow *tmp;
  GList *path1 = NULL, *path2 = NULL;
  GList *list1, *list2;

  tmp = win1;
  while (tmp != NULL && tmp->window_type != CDK_WINDOW_ROOT)
    {
      path1 = g_list_prepend (path1, tmp);
      tmp = get_native_parent (tmp);
    }

  tmp = win2;
  while (tmp != NULL && tmp->window_type != CDK_WINDOW_ROOT)
    {
      path2 = g_list_prepend (path2, tmp);
      tmp = get_native_parent (tmp);
    }

  list1 = path1;
  list2 = path2;
  tmp = NULL;
  while (list1 && list2 && (list1->data == list2->data))
    {
      tmp = (CdkWindow *)list1->data;
      list1 = list1->next;
      list2 = list2->next;
    }
  g_list_free (path1);
  g_list_free (path2);

  return tmp;
}

void
synthesize_crossing_events (CdkDisplay                 *display,
			    CdkWindow                  *src,
			    CdkWindow                  *dest,
			    CdkCrossingMode             mode,
			    POINT                      *screen_pt,
			    CdkModifierType             mask,
			    guint32                     time_,
			    gboolean                    non_linear)
{
  CdkWindow *c;
  CdkWindow *win;
  GList *path, *list;
  CdkWindow *a;
  CdkWindow *b;
  CdkNotifyType notify_type;

  a = src;
  b = dest;
  if (a == b)
    return; /* No crossings generated between src and dest */

  c = find_common_ancestor (a, b);

  non_linear |= (c != a) && (c != b);

  if (a) /* There might not be a source (i.e. if no previous pointer_in_window) */
    {
      /* Traverse up from a to (excluding) c sending leave events */
      if (non_linear)
	notify_type = CDK_NOTIFY_NONLINEAR;
      else if (c == a)
	notify_type = CDK_NOTIFY_INFERIOR;
      else
	notify_type = CDK_NOTIFY_ANCESTOR;
      send_crossing_event (display,
			   a, CDK_LEAVE_NOTIFY,
			   mode,
			   notify_type,
			   NULL,
			   screen_pt,
			   mask, time_);

      if (c != a)
	{
	  CdkWindow *last;

	  if (non_linear)
	    notify_type = CDK_NOTIFY_NONLINEAR_VIRTUAL;
	  else
	    notify_type = CDK_NOTIFY_VIRTUAL;

	  last = a;
	  win = get_native_parent (a);
	  while (win != c && win->window_type != CDK_WINDOW_ROOT)
	    {
	      send_crossing_event (display,
				   win, CDK_LEAVE_NOTIFY,
				   mode,
				   notify_type,
				   (CdkWindow *)last,
				   screen_pt,
				   mask, time_);

	      last = win;
	      win = get_native_parent (win);
	    }
	}
    }

  if (b) /* Might not be a dest, e.g. if we're moving out of the window */
    {
      /* Traverse down from c to b */
      if (c != b)
	{
	  path = NULL;
	  win = get_native_parent (b);
	  while (win != c && win->window_type != CDK_WINDOW_ROOT)
	    {
	      path = g_list_prepend (path, win);
	      win = get_native_parent (win);
	    }

	  if (non_linear)
	    notify_type = CDK_NOTIFY_NONLINEAR_VIRTUAL;
	  else
	    notify_type = CDK_NOTIFY_VIRTUAL;

	  list = path;
	  while (list)
	    {
	      CdkWindow *next;

	      win = (CdkWindow *)list->data;
	      list = list->next;
	      if (list)
		next = (CdkWindow *)list->data;
	      else
		next = b;

	      send_crossing_event (display,
				   win, CDK_ENTER_NOTIFY,
				   mode,
				   notify_type,
				   next,
				   screen_pt,
				   mask, time_);
	    }
	  g_list_free (path);
	}


      if (non_linear)
	notify_type = CDK_NOTIFY_NONLINEAR;
      else if (c == a)
	notify_type = CDK_NOTIFY_ANCESTOR;
      else
	notify_type = CDK_NOTIFY_INFERIOR;

      send_crossing_event (display,
			   b, CDK_ENTER_NOTIFY,
			   mode,
			   notify_type,
			   NULL,
			   screen_pt,
			   mask, time_);
    }
}

/* The check_extended flag controls whether to check if the windows want
 * events from extended input devices and if the message should be skipped
 * because an extended input device is active
 */
static gboolean
propagate (CdkWindow  **window,
	   MSG         *msg,
	   CdkWindow   *grab_window,
	   gboolean     grab_owner_events,
	   gint	        grab_mask,
	   gboolean   (*doesnt_want_it) (gint mask,
					 MSG *msg))
{
  if (grab_window != NULL && !grab_owner_events)
    {
      /* Event source is grabbed with owner_events FALSE */

      if ((*doesnt_want_it) (grab_mask, msg))
	{
	  CDK_NOTE (EVENTS, g_print (" (grabber doesn't want it)"));
	  return FALSE;
	}
      else
	{
	  CDK_NOTE (EVENTS, g_print (" (to grabber)"));
	  g_set_object (window, grab_window);
	  return TRUE;
	}
    }

  /* If we come here, we know that if grab_window != NULL then
   * grab_owner_events is TRUE
   */
  while (TRUE)
    {
      if ((*doesnt_want_it) ((*window)->event_mask, msg))
	{
	  /* Owner doesn't want it, propagate to parent. */
	  CdkWindow *parent = cdk_window_get_parent (*window);
	  if (parent == cdk_get_default_root_window () || parent == NULL)
	    {
	      /* No parent; check if grabbed */
	      if (grab_window != NULL)
		{
		  /* Event source is grabbed with owner_events TRUE */

		  if ((*doesnt_want_it) (grab_mask, msg))
		    {
		      /* Grabber doesn't want it either */
		      CDK_NOTE (EVENTS, g_print (" (grabber doesn't want it)"));
		      return FALSE;
		    }
		  else
		    {
		      /* Grabbed! */
		      CDK_NOTE (EVENTS, g_print (" (to grabber)"));
		      g_set_object (window, grab_window);
		      return TRUE;
		    }
		}
	      else
		{
		  CDK_NOTE (EVENTS, g_print (" (undelivered)"));
		  return FALSE;
		}
	    }
	  else
	    {
	      g_set_object (window, parent);
	      /* The only branch where we actually continue the loop */
	    }
	}
      else
	return TRUE;
    }
}

static gboolean
doesnt_want_key (gint mask,
		 MSG *msg)
{
  return (((msg->message == WM_KEYUP || msg->message == WM_SYSKEYUP) &&
	   !(mask & CDK_KEY_RELEASE_MASK)) ||
	  ((msg->message == WM_KEYDOWN || msg->message == WM_SYSKEYDOWN) &&
	   !(mask & CDK_KEY_PRESS_MASK)));
}

static gboolean
doesnt_want_char (gint mask,
		  MSG *msg)
{
  return !(mask & (CDK_KEY_PRESS_MASK | CDK_KEY_RELEASE_MASK));
}

/* Acquires actual client area size of the underlying native window.
 * Rectangle is in CDK screen coordinates (_cdk_offset_* is added).
 * Returns FALSE if configure events should be inhibited,
 * TRUE otherwise.
 */
gboolean
_cdk_win32_get_window_rect (CdkWindow *window,
                            RECT      *rect)
{
  CdkWindowImplWin32 *window_impl;
  RECT client_rect;
  POINT point;
  HWND hwnd;

  window_impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  hwnd = CDK_WINDOW_HWND (window);

  GetClientRect (hwnd, &client_rect);
  point.x = client_rect.left; /* always 0 */
  point.y = client_rect.top;

  /* top level windows need screen coords */
  if (cdk_window_get_parent (window) == cdk_get_default_root_window ())
    {
      ClientToScreen (hwnd, &point);
      point.x += _cdk_offset_x * window_impl->window_scale;
      point.y += _cdk_offset_y * window_impl->window_scale;
    }

  rect->left = point.x;
  rect->top = point.y;
  rect->right = point.x + client_rect.right - client_rect.left;
  rect->bottom = point.y + client_rect.bottom - client_rect.top;

  return !window_impl->inhibit_configure;
}

void
_cdk_win32_do_emit_configure_event (CdkWindow *window,
                                    RECT       rect)
{
  CdkWindowImplWin32 *impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  impl->unscaled_width = rect.right - rect.left;
  impl->unscaled_height = rect.bottom - rect.top;
  window->width = (impl->unscaled_width + impl->window_scale - 1) / impl->window_scale;
  window->height = (impl->unscaled_height + impl->window_scale - 1) / impl->window_scale;
  window->x = rect.left / impl->window_scale;
  window->y = rect.top / impl->window_scale;

  _cdk_window_update_size (window);

  if (window->event_mask & CDK_STRUCTURE_MASK)
    {
      CdkEvent *event = cdk_event_new (CDK_CONFIGURE);

      event->configure.window = window;

      event->configure.width = window->width;
      event->configure.height = window->height;

      event->configure.x = window->x;
      event->configure.y = window->y;

      _cdk_win32_append_event (event);
    }
}

void
_cdk_win32_emit_configure_event (CdkWindow *window)
{
  RECT rect;

  if (!_cdk_win32_get_window_rect (window, &rect))
    return;

  _cdk_win32_do_emit_configure_event (window, rect);
}

cairo_region_t *
_cdk_win32_hrgn_to_region (HRGN  hrgn,
                           guint scale)
{
  RGNDATA *rgndata;
  RECT *rects;
  cairo_region_t *result;
  gint nbytes;
  guint i;

  if ((nbytes = GetRegionData (hrgn, 0, NULL)) == 0)
    {
      WIN32_GDI_FAILED ("GetRegionData");
      return NULL;
    }

  rgndata = (RGNDATA *) g_malloc (nbytes);

  if (GetRegionData (hrgn, nbytes, rgndata) == 0)
    {
      WIN32_GDI_FAILED ("GetRegionData");
      g_free (rgndata);
      return NULL;
    }

  result = cairo_region_create ();
  rects = (RECT *) rgndata->Buffer;
  for (i = 0; i < rgndata->rdh.nCount; i++)
    {
      CdkRectangle r;

      r.x = rects[i].left;
      r.y = rects[i].top;
      r.width = (rects[i].right - r.x) / scale;
      r.height = (rects[i].bottom - r.y) / scale;

      cairo_region_union_rectangle (result, &r);
    }

  g_free (rgndata);

  return result;
}

static void
adjust_drag (LONG *drag,
	     LONG  curr,
	     gint  inc)
{
  if (*drag > curr)
    *drag = curr + ((*drag + inc/2 - curr) / inc) * inc;
  else
    *drag = curr - ((curr - *drag + inc/2) / inc) * inc;
}

static void
handle_wm_paint (MSG        *msg,
		 CdkWindow  *window)
{
  HRGN hrgn = CreateRectRgn (0, 0, 0, 0);
  HDC hdc;
  PAINTSTRUCT paintstruct;
  cairo_region_t *update_region;
  CdkWindowImplWin32 *impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  if (GetUpdateRgn (msg->hwnd, hrgn, FALSE) == ERROR)
    {
      WIN32_GDI_FAILED ("GetUpdateRgn");
      DeleteObject (hrgn);
      return;
    }

  hdc = BeginPaint (msg->hwnd, &paintstruct);

  CDK_NOTE (EVENTS, g_print (" %s %s dc %p",
			     _cdk_win32_rect_to_string (&paintstruct.rcPaint),
			     (paintstruct.fErase ? "erase" : ""),
			     hdc));

  EndPaint (msg->hwnd, &paintstruct);

  if ((paintstruct.rcPaint.right == paintstruct.rcPaint.left) ||
      (paintstruct.rcPaint.bottom == paintstruct.rcPaint.top))
    {
      CDK_NOTE (EVENTS, g_print (" (empty paintstruct, ignored)"));
      DeleteObject (hrgn);
      return;
    }

  update_region = _cdk_win32_hrgn_to_region (hrgn, impl->window_scale);
  if (!cairo_region_is_empty (update_region))
    _cdk_window_invalidate_for_expose (window, update_region);
  cairo_region_destroy (update_region);

  DeleteObject (hrgn);
}

static VOID CALLBACK
modal_timer_proc (HWND     hwnd,
		  UINT     msg,
		  UINT_PTR id,
		  DWORD    time)
{
  int arbitrary_limit = 10;

  while (_modal_operation_in_progress != CDK_WIN32_MODAL_OP_NONE &&
	 g_main_context_pending (NULL) &&
	 arbitrary_limit--)
    g_main_context_iteration (NULL, FALSE);
}

void
_cdk_win32_begin_modal_call (CdkWin32ModalOpKind kind)
{
  CdkWin32ModalOpKind was = _modal_operation_in_progress;
  g_assert (!(_modal_operation_in_progress & kind));

  _modal_operation_in_progress |= kind;

  if (was == CDK_WIN32_MODAL_OP_NONE)
    {
      modal_timer = SetTimer (NULL, 0, 10, modal_timer_proc);

      if (modal_timer == 0)
	WIN32_API_FAILED ("SetTimer");
    }
}

void
_cdk_win32_end_modal_call (CdkWin32ModalOpKind kind)
{
  g_assert (_modal_operation_in_progress & kind);

  _modal_operation_in_progress &= ~kind;

  if (_modal_operation_in_progress == CDK_WIN32_MODAL_OP_NONE &&
      modal_timer != 0)
    {
      API_CALL (KillTimer, (NULL, modal_timer));
      modal_timer = 0;
    }
}

static VOID CALLBACK
sync_timer_proc (HWND     hwnd,
		 UINT     msg,
		 UINT_PTR id,
		 DWORD    time)
{
  MSG message;
  if (PeekMessageW (&message, hwnd, WM_PAINT, WM_PAINT, PM_REMOVE))
    {
      return;
    }

  RedrawWindow (hwnd, NULL, NULL, RDW_INVALIDATE|RDW_UPDATENOW|RDW_ALLCHILDREN);

  KillTimer (hwnd, sync_timer);
}

static gboolean
handle_nchittest (HWND hwnd,
                  CdkWindow *window,
                  gint16 screen_x,
                  gint16 screen_y,
                  gint *ret_valp)
{
  RECT rect;
  CdkWindowImplWin32 *impl;

  if (window == NULL || window->input_shape == NULL)
    return FALSE;

  /* If the window has decorations, DefWindowProc() will take
   * care of NCHITTEST.
   */
  if (!_cdk_win32_window_lacks_wm_decorations (window))
    return FALSE;

  if (!GetWindowRect (hwnd, &rect))
    return FALSE;

  impl = CDK_WINDOW_IMPL_WIN32 (window->impl);
  rect.left = screen_x - rect.left;
  rect.top = screen_y - rect.top;

  /* If it's inside the rect, return FALSE and let DefWindowProc() handle it */
  if (cairo_region_contains_point (window->input_shape,
                                   rect.left / impl->window_scale,
                                   rect.top / impl->window_scale))
    return FALSE;

  /* Otherwise override DefWindowProc() and tell WM that the point is not
   * within the window
   */
  *ret_valp = HTNOWHERE;
  return TRUE;
}

static void
generate_button_event (CdkEventType      type,
                       gint              button,
                       CdkWindow        *window,
                       MSG              *msg)
{
  CdkEvent *event = cdk_event_new (type);
  CdkDeviceManagerWin32 *device_manager;
  CdkWindowImplWin32 *impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  if (_cdk_input_ignore_core > 0)
    return;

  device_manager = CDK_DEVICE_MANAGER_WIN32 (cdk_display_get_device_manager (cdk_display_get_default ()));

  event->button.window = window;
  event->button.time = _cdk_win32_get_next_tick (msg->time);
  event->button.x = current_x = (gint16) GET_X_LPARAM (msg->lParam) / impl->window_scale;
  event->button.y = current_y = (gint16) GET_Y_LPARAM (msg->lParam) / impl->window_scale;
  event->button.x_root = (msg->pt.x + _cdk_offset_x) / impl->window_scale;
  event->button.y_root = (msg->pt.y + _cdk_offset_y) / impl->window_scale;
  event->button.axes = NULL;
  event->button.state = build_pointer_event_state (msg);
  event->button.button = button;
  cdk_event_set_device (event, device_manager->core_pointer);
  cdk_event_set_source_device (event, device_manager->system_pointer);
  cdk_event_set_seat (event, cdk_device_get_seat (device_manager->core_pointer));

  _cdk_win32_append_event (event);
}

static gboolean
handle_wm_sysmenu (CdkWindow *window, MSG *msg, gint *ret_valp)
{
  CdkWindowImplWin32 *impl;
  LONG_PTR style, tmp_style;
  gboolean maximized, minimized;
  LONG_PTR additional_styles;

  impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  style = GetWindowLongPtr (msg->hwnd, GWL_STYLE);

  maximized = IsZoomed (msg->hwnd) || (style & WS_MAXIMIZE);
  minimized = IsIconic (msg->hwnd) || (style & WS_MINIMIZE);
  additional_styles = 0;

  if (!(style & WS_SYSMENU))
    additional_styles |= WS_SYSMENU;

  if (!(style & WS_MAXIMIZEBOX))
    additional_styles |= WS_MAXIMIZEBOX;

  if (!(style & WS_MINIMIZEBOX))
    additional_styles |= WS_MINIMIZEBOX;

  if (!(style & WS_SIZEBOX))
    additional_styles |= WS_SIZEBOX;

  if (!(style & WS_DLGFRAME))
    additional_styles |= WS_DLGFRAME;

  if (!(style & WS_BORDER))
    additional_styles |= WS_BORDER;

  if (additional_styles == 0)
    /* The caller will eventually pass this to DefWindowProc (),
     * only without the style dance, which isn't needed, as it turns out.
     */
    return FALSE;

  /* Note: This code will enable resizing, maximizing and minimizing windows
   * via window menu even if these are non-CSD windows that were explicitly
   * forbidden from doing this by removing the appropriate styles,
   * or if these are CSD windows that were explicitly forbidden from doing
   * this by removing appropriate decorations from the headerbar and/or
   * changing hints or properties.
   *
   * If doing this for non-CSD windows is not desired,
   * do a _cdk_win32_window_lacks_wm_decorations() check and return FALSE
   * if it doesn't pass.
   *
   * If doing this for CSD windows with disabled decorations is not desired,
   * tough luck - CDK can't know which CSD decorations are enabled, and which
   * are not.
   *
   * If doing this for CSD windows with particular hints is not desired,
   * check window hints here and return FALSE (DefWindowProc() will return
   * FALSE later) or set *ret_valp to 0 and return TRUE.
   */
  tmp_style = style | additional_styles;
  CDK_NOTE (EVENTS, g_print (" Handling WM_SYSMENU: style 0x%lx -> 0x%lx\n", style, tmp_style));
  impl->have_temp_styles = TRUE;
  impl->temp_styles = additional_styles;
  SetWindowLongPtr (msg->hwnd, GWL_STYLE, tmp_style);

  *ret_valp = DefWindowProc (msg->hwnd, msg->message, msg->wParam, msg->lParam);

  tmp_style = GetWindowLongPtr (msg->hwnd, GWL_STYLE);
  style = tmp_style & ~additional_styles;

  CDK_NOTE (EVENTS, g_print (" Handling WM_SYSMENU: style 0x%lx <- 0x%lx\n", style, tmp_style));
  SetWindowLongPtr (msg->hwnd, GWL_STYLE, style);
  impl->have_temp_styles = FALSE;

  return TRUE;
}

gboolean
_cdk_win32_window_fill_min_max_info (CdkWindow  *window,
                                     MINMAXINFO *mmi)
{
  CdkWindowImplWin32 *impl;
  RECT rect;

  if (CDK_WINDOW_DESTROYED (window))
    return FALSE;

  impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  if (impl->hint_flags & CDK_HINT_MIN_SIZE)
    {
      rect.left = rect.top = 0;
      rect.right = impl->hints.min_width * impl->window_scale;
      rect.bottom = impl->hints.min_height * impl->window_scale;

      _cdk_win32_adjust_client_rect (window, &rect);

      mmi->ptMinTrackSize.x = rect.right - rect.left;
      mmi->ptMinTrackSize.y = rect.bottom - rect.top;
    }

  if (impl->hint_flags & CDK_HINT_MAX_SIZE)
    {
      int maxw, maxh;

      rect.left = rect.top = 0;
      rect.right = impl->hints.max_width * impl->window_scale;
      rect.bottom = impl->hints.max_height * impl->window_scale;

      _cdk_win32_adjust_client_rect (window, &rect);

      /* at least on win9x we have the 16 bit trouble */
      maxw = rect.right - rect.left;
      maxh = rect.bottom - rect.top;
      mmi->ptMaxTrackSize.x = maxw > 0 && maxw < G_MAXSHORT ? maxw : G_MAXSHORT;
      mmi->ptMaxTrackSize.y = maxh > 0 && maxh < G_MAXSHORT ? maxh : G_MAXSHORT;
    }
  else
    {
      /* According to "How does the window manager adjust ptMaxSize and
       * ptMaxPosition for multiple monitors?" article
       * https://blogs.msdn.microsoft.com/oldnewthing/20150501-00/?p=44964
       * if ptMaxSize >= primary_monitor_size, then it will be adjusted by
       * WM to account for the monitor size differences if the window gets
       * maximized on a non-primary monitor, by simply adding the size
       * difference (i.e. if non-primary monitor is larger by 100px, then
       * window will be made larger exactly by 100px).
       * If ptMaxSize < primary_monitor_size at least in one direction,
       * nothing is adjusted.
       * Therefore, if primary monitor is smaller than the actual monitor,
       * then it is not possible to give window a size that is larger than
       * the primary monitor and smaller than the non-primary monitor,
       * because WM will always enlarge the window.
       * Therefore, it is impossible to account for taskbar size.
       * So we don't try at all. Instead we just remember that we're trying
       * to maximize the window, catch WM_WINDOWPOSCHANGING and
       * adjust the size then.
       */
      HMONITOR nearest_monitor;
      MONITORINFO nearest_info;

      nearest_monitor = MonitorFromWindow (CDK_WINDOW_HWND (window), MONITOR_DEFAULTTONEAREST);
      nearest_info.cbSize = sizeof (nearest_info);

      if (GetMonitorInfoA (nearest_monitor, &nearest_info))
        {
          /* MSDN says that we must specify maximized window
           * size as if it was located on the primary monitor.
           * However, we still need to account for a taskbar
           * that might or might not be on the nearest monitor where
           * window will actually end up.
           * "0" here is the top-left corner of the primary monitor.
           */
          /* An investigation into bug 765161 turned up a weird Windows WM behaviour
           * where it would interpret "0:0" as "top-left of the workea" (accounting for a taskbar
           * possibly being along the left/top edge of the screen) when window has styles
           * (i.e. not CSD), and interpret the same "0:0" as "top-left of the screen" (not
           * accounting for a taskbar) when window has no styles (i.e. a CSD window).
           * This doesn't seem to be documented anywhere.
           * The following code uses a simple CSD/non-CSD test, but it could be that
           * this behaviour hinges on just one particular window style.
           * Finding exactly which style that could be is not very useful for CTK, however.
           */
          mmi->ptMaxPosition.x = 0;
          mmi->ptMaxPosition.y = 0;

          if (_cdk_win32_window_lacks_wm_decorations (window))
            {
              mmi->ptMaxPosition.x += (nearest_info.rcWork.left - nearest_info.rcMonitor.left);
              mmi->ptMaxPosition.y += (nearest_info.rcWork.top - nearest_info.rcMonitor.top);
            }

          mmi->ptMaxSize.x = nearest_info.rcWork.right - nearest_info.rcWork.left;
          mmi->ptMaxSize.y = nearest_info.rcWork.bottom - nearest_info.rcWork.top;
        }

      mmi->ptMaxTrackSize.x = GetSystemMetrics (SM_CXVIRTUALSCREEN) + impl->margins_x * impl->window_scale;
      mmi->ptMaxTrackSize.y = GetSystemMetrics (SM_CYVIRTUALSCREEN) + impl->margins_y * impl->window_scale;
    }

  return TRUE;
}

static void
cdk_settings_notify (CdkWindow        *window,
                     const char       *name,
                     CdkSettingAction  action)
{
  CdkEvent *new_event;

  if (!g_str_has_prefix (name, "ctk-"))
    return;

  new_event = cdk_event_new (CDK_SETTING);
  new_event->setting.window = window;
  new_event->setting.send_event = FALSE;
  new_event->setting.action = action;
  new_event->setting.name = g_strdup (name);

  _cdk_win32_append_event (new_event);
}

#define CDK_ANY_BUTTON_MASK (CDK_BUTTON1_MASK | \
			     CDK_BUTTON2_MASK | \
			     CDK_BUTTON3_MASK | \
			     CDK_BUTTON4_MASK | \
			     CDK_BUTTON5_MASK)

static gboolean
cdk_event_translate (MSG  *msg,
		     gint *ret_valp)
{
  RECT rect, *drag, orig_drag;
  POINT point;
  MINMAXINFO *mmi;
  HWND hwnd;
  CdkCursor *cursor;
  BYTE key_state[256];
  HIMC himc;
  WINDOWPOS *windowpos;
  gboolean ignore_leave;

  CdkEvent *event;

  wchar_t wbuf[100];
  gint ccount;

  CdkDisplay *display;
  CdkWindow *window = NULL;
  CdkWindowImplWin32 *impl;

  CdkWindow *new_window = NULL;

  CdkDeviceManager *device_manager = NULL;
  CdkDeviceManagerWin32 *device_manager_win32 = NULL;

  CdkDeviceGrabInfo *keyboard_grab = NULL;
  CdkDeviceGrabInfo *pointer_grab = NULL;
  CdkWindow *grab_window = NULL;

  gint button;
  CdkAtom target;

  gchar buf[256];
  gboolean return_val = FALSE;

  int i;

  CdkWin32Selection *win32_sel = NULL;

  STGMEDIUM *property_change_data;

  display = cdk_display_get_default ();
  window = cdk_win32_handle_table_lookup (msg->hwnd);

  if (_cdk_default_filters)
    {
      /* Apply global filters */

      CdkFilterReturn result = apply_event_filters (window ? window : cdk_screen_get_root_window (cdk_display_get_default_screen (display)),
                                                    msg,
                                                    &_cdk_default_filters);

      /* If result is CDK_FILTER_CONTINUE, we continue as if nothing
       * happened. If it is CDK_FILTER_REMOVE or CDK_FILTER_TRANSLATE,
       * we return TRUE, and DefWindowProcW() will not be called.
       */
      if (result == CDK_FILTER_REMOVE || result == CDK_FILTER_TRANSLATE)
	return TRUE;
    }

  if (window == NULL)
    {
      /* XXX Handle WM_QUIT here ? */
      if (msg->message == WM_QUIT)
	{
	  CDK_NOTE (EVENTS, g_print (" %d", (int) msg->wParam));
	  exit (msg->wParam);
	}
      else if (msg->message == WM_CREATE)
	{
	  window = (UNALIGNED CdkWindow*) (((LPCREATESTRUCTW) msg->lParam)->lpCreateParams);
	  CDK_WINDOW_HWND (window) = msg->hwnd;
	}
      else
	{
	  CDK_NOTE (EVENTS, g_print (" (no CdkWindow)"));
	}
      return FALSE;
    }

  /* cdk_event_translate() can be called during initialization, if something
   * sends MSGs. In this case, the default display or its device manager will
   * be NULL, so avoid trying to read the active grabs.
   * https://bugzilla.gnome.org/show_bug.cgi?id=774379
   */
  if (display != NULL)
    {
      device_manager = cdk_display_get_device_manager (display);
    }
  else
    {
      CDK_NOTE (EVENTS, g_print (" (no CdkDisplay)"));
    }

  if (device_manager != NULL)
    {
      device_manager_win32 = CDK_DEVICE_MANAGER_WIN32 (device_manager);
    }
  else
    {
      CDK_NOTE (EVENTS, g_print (" (no CdkDeviceManager)"));
    }

  if (device_manager_win32 != NULL)
    {
      keyboard_grab = _cdk_display_get_last_device_grab (display, device_manager_win32->core_keyboard);
      pointer_grab = _cdk_display_get_last_device_grab (display, device_manager_win32->core_pointer);
    }

  g_object_ref (window);

  /* window's refcount has now been increased, so code below should
   * not just return from this function, but instead goto done (or
   * break out of the big switch). To protect against forgetting this,
   * #define return to a syntax error...
   */
#define return GOTO_DONE_INSTEAD

  if (!CDK_WINDOW_DESTROYED (window) && window->filters)
    {
      /* Apply per-window filters */

      CdkFilterReturn result = apply_event_filters (window, msg, &window->filters);

      if (result == CDK_FILTER_REMOVE || result == CDK_FILTER_TRANSLATE)
	{
	  return_val = TRUE;
	  goto done;
	}
    }

  if (msg->message == client_message)
    {
      GList *tmp_list;
      CdkFilterReturn result = CDK_FILTER_CONTINUE;
      GList *node;

      CDK_NOTE (EVENTS, g_print (" client_message"));

      event = cdk_event_new (CDK_NOTHING);
      ((CdkEventPrivate *)event)->flags |= CDK_EVENT_PENDING;

      node = _cdk_event_queue_append (display, event);

      tmp_list = client_filters;
      while (tmp_list)
	{
	  CdkClientFilter *filter = tmp_list->data;

	  tmp_list = tmp_list->next;

	  if (filter->type == CDK_POINTER_TO_ATOM (msg->wParam))
	    {
	      CDK_NOTE (EVENTS, g_print (" (match)"));

	      result = (*filter->function) (msg, event, filter->data);

	      if (result != CDK_FILTER_CONTINUE)
		break;
	    }
	}

      switch (result)
	{
	case CDK_FILTER_REMOVE:
	  _cdk_event_queue_remove_link (display, node);
	  g_list_free_1 (node);
	  cdk_event_free (event);
	  return_val = TRUE;
	  goto done;

	case CDK_FILTER_TRANSLATE:
	  ((CdkEventPrivate *)event)->flags &= ~CDK_EVENT_PENDING;
	  CDK_NOTE (EVENTS, _cdk_win32_print_event (event));
	  return_val = TRUE;
	  goto done;

	case CDK_FILTER_CONTINUE:
	  /* No more: Send unknown client messages on to Ctk for it to use */
	  CDK_NOTE (EVENTS, _cdk_win32_print_event (event));
	  return_val = TRUE;
	  goto done;
	}
    }

  if (msg->message == aerosnap_message)
    _cdk_win32_window_handle_aerosnap (cdk_window_get_toplevel (window),
                                       (CdkWin32AeroSnapCombo) msg->wParam);

  switch (msg->message)
    {
    case WM_INPUTLANGCHANGE:
      _cdk_input_locale = (HKL) msg->lParam;
      _cdk_win32_keymap_set_active_layout (CDK_WIN32_KEYMAP (_cdk_win32_display_get_keymap (_cdk_display)), _cdk_input_locale);
      GetLocaleInfo (MAKELCID (LOWORD (_cdk_input_locale), SORT_DEFAULT),
		     LOCALE_IDEFAULTANSICODEPAGE,
		     buf, sizeof (buf));
      _cdk_input_codepage = atoi (buf);
      _cdk_keymap_serial++;
      CDK_NOTE (EVENTS,
		g_print (" cs:%lu hkl:%p%s cp:%d",
			 (gulong) msg->wParam,
			 (gpointer) msg->lParam, _cdk_input_locale_is_ime ? " (IME)" : "",
			 _cdk_input_codepage));
      cdk_settings_notify (window, "ctk-im-module", CDK_SETTING_ACTION_CHANGED);

      /* Generate a dummy key event to "nudge" IMContext */
      event = cdk_event_new (CDK_KEY_PRESS);
      event->key.window = window;
      event->key.time = _cdk_win32_get_next_tick (msg->time);
      event->key.keyval = CDK_KEY_VoidSymbol;
      event->key.string = NULL;
      event->key.length = 0;
      event->key.hardware_keycode = 0;
      cdk_event_set_scancode (event, 0);
      cdk_event_set_device (event, device_manager_win32->core_keyboard);
      cdk_event_set_source_device (event, device_manager_win32->system_keyboard);
      cdk_event_set_seat (event, cdk_device_get_seat (device_manager_win32->core_keyboard));
      event->key.is_modifier = FALSE;
      event->key.state = 0;
      _cdk_win32_append_event (event);
      break;

    case WM_SYSKEYUP:
    case WM_SYSKEYDOWN:
      CDK_NOTE (EVENTS,
		g_print (" %s ch:%.02x %s",
			 _cdk_win32_key_to_string (msg->lParam),
			 (int) msg->wParam,
			 decode_key_lparam (msg->lParam)));

      /* If posted without us having keyboard focus, ignore */
      if ((msg->wParam != VK_F10 && msg->wParam != VK_MENU) &&
	  !(HIWORD (msg->lParam) & KF_ALTDOWN))
	break;

      /* Let the system handle Alt-Tab, Alt-Space and Alt-F4 unless
       * the keyboard is grabbed.
       */
      if (!keyboard_grab &&
	  (msg->wParam == VK_TAB ||
	   msg->wParam == VK_SPACE ||
	   msg->wParam == VK_F4))
	break;

      /* Jump to code in common with WM_KEYUP and WM_KEYDOWN */
      goto keyup_or_down;

    case WM_KEYUP:
    case WM_KEYDOWN:
      CDK_NOTE (EVENTS,
		g_print (" %s ch:%.02x %s",
			 _cdk_win32_key_to_string (msg->lParam),
			 (int) msg->wParam,
			 decode_key_lparam (msg->lParam)));

    keyup_or_down:

      /* Ignore key messages intended for the IME */
      if (msg->wParam == VK_PROCESSKEY ||
	  in_ime_composition)
	break;

      /* Ignore autorepeats on modifiers */
      if (msg->message == WM_KEYDOWN &&
          (msg->wParam == VK_MENU ||
           msg->wParam == VK_CONTROL ||
           msg->wParam == VK_SHIFT) &&
           ((HIWORD(msg->lParam) & KF_REPEAT) >= 1))
        break;

      if (keyboard_grab &&
          !propagate (&window, msg,
		      keyboard_grab->window,
		      keyboard_grab->owner_events,
		      CDK_ALL_EVENTS_MASK,
		      doesnt_want_key))
	break;

      if (CDK_WINDOW_DESTROYED (window))
	break;

      impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

      API_CALL (GetKeyboardState, (key_state));

      event = cdk_event_new ((msg->message == WM_KEYDOWN ||
			      msg->message == WM_SYSKEYDOWN) ?
			     CDK_KEY_PRESS : CDK_KEY_RELEASE);
      event->key.window = window;
      event->key.time = _cdk_win32_get_next_tick (msg->time);
      event->key.keyval = CDK_KEY_VoidSymbol;
      event->key.string = NULL;
      event->key.length = 0;
      event->key.hardware_keycode = msg->wParam;
      /* save original scancode */
      cdk_event_set_scancode (event, msg->lParam >> 16);
      cdk_event_set_device (event, device_manager_win32->core_keyboard);
      cdk_event_set_source_device (event, device_manager_win32->system_keyboard);
      cdk_event_set_seat (event, cdk_device_get_seat (device_manager_win32->core_keyboard));

      /* Get the WinAPI translation of the WM_KEY messages to characters.

         The WM_CHAR messages are generated by a previous call to TranslateMessage() and always
	 follow directly after the corresponding WM_KEY* messages.
	 There could be 0 or more WM_CHAR messages following (for example dead keys don't generate
	 WM_CHAR messages - they generate WM_DEAD_CHAR instead, but we are not interested in those
	 messages). */

      if (cdk_event_is_allocated (event)) /* Should always be true */
	{
	  CdkEventPrivate *event_priv = (CdkEventPrivate*) event;

	  MSG msg2;
	  while (PeekMessageW (&msg2, msg->hwnd, 0, 0, 0) && (msg2.message == WM_CHAR || msg2.message == WM_SYSCHAR))
	    {
	      /* The character is encoded in WPARAM as UTF-16. */
	      gunichar2 c = msg2.wParam;

	      /* Ignore control sequences like Backspace */
	      if (!g_unichar_iscntrl(c))
		{
		  /* Append character to translation string. */
		  event_priv->translation_len ++;
		  event_priv->translation = g_realloc (event_priv->translation, event_priv->translation_len * sizeof (event_priv->translation[0]));
		  event_priv->translation[event_priv->translation_len - 1] = c;
		}

	      /* Remove message from queue */
	      GetMessageW (&msg2, msg->hwnd, 0, 0);
	    }
	}

      if (HIWORD (msg->lParam) & KF_EXTENDED)
	{
	  switch (msg->wParam)
	    {
	    case VK_CONTROL:
	      event->key.hardware_keycode = VK_RCONTROL;
	      break;
	    case VK_SHIFT:	/* Actually, KF_EXTENDED is not set
				 * for the right shift key.
				 */
	      event->key.hardware_keycode = VK_RSHIFT;
	      break;
	    case VK_MENU:
	      event->key.hardware_keycode = VK_RMENU;
	      break;
	    }
	}
      else if (msg->wParam == VK_SHIFT &&
	       LOBYTE (HIWORD (msg->lParam)) == _cdk_win32_keymap_get_rshift_scancode (CDK_WIN32_KEYMAP (_cdk_win32_display_get_keymap (_cdk_display))))
	event->key.hardware_keycode = VK_RSHIFT;

      event->key.is_modifier = (msg->wParam == VK_CONTROL ||
                                msg->wParam == VK_SHIFT ||
                                msg->wParam == VK_MENU);
      /* g_print ("ctrl:%02x lctrl:%02x rctrl:%02x alt:%02x lalt:%02x ralt:%02x\n", key_state[VK_CONTROL], key_state[VK_LCONTROL], key_state[VK_RCONTROL], key_state[VK_MENU], key_state[VK_LMENU], key_state[VK_RMENU]); */

      build_key_event_state (event, key_state);

      cdk_keymap_translate_keyboard_state (_cdk_win32_display_get_keymap (display),
					   event->key.hardware_keycode,
					   event->key.state,
					   event->key.group,
					   &event->key.keyval,
					   NULL, NULL, NULL);

      fill_key_event_string (event);

  /* Only one release key event is fired when both shift keys are pressed together
     and then released. In order to send the missing event, press events for shift
     keys are recorded and sent together when the release event occurs.
     Other modifiers (e.g. ctrl, alt) don't have this problem. */
  if (msg->message == WM_KEYDOWN && msg->wParam == VK_SHIFT)
    {
      int pressed_shift = msg->lParam & 0xffffff; /* mask shift modifier */
      if (both_shift_pressed[0] == 0)
        both_shift_pressed[0] = pressed_shift;
      else if (both_shift_pressed[0] != pressed_shift)
        both_shift_pressed[1] = pressed_shift;
    }

  if (msg->message == WM_KEYUP && msg->wParam == VK_SHIFT)
    {
      if (both_shift_pressed[0] != 0 && both_shift_pressed[1] != 0)
        {
          gint tmp_retval;
          MSG fake_release = *msg;
          int pressed_shift = msg->lParam & 0xffffff;

          if (both_shift_pressed[0] == pressed_shift)
            fake_release.lParam = both_shift_pressed[1];
          else
            fake_release.lParam = both_shift_pressed[0];

          both_shift_pressed[0] = both_shift_pressed[1] = 0;
          cdk_event_translate (&fake_release, &tmp_retval);
        }
      both_shift_pressed[0] = both_shift_pressed[1] = 0;
    }

      /* Reset MOD1_MASK if it is the Alt key itself */
      if (msg->wParam == VK_MENU)
	event->key.state &= ~CDK_MOD1_MASK;

      _cdk_win32_append_event (event);

      return_val = TRUE;
      break;

    case WM_SYSCHAR:
      if (msg->wParam != VK_SPACE)
	{
	  /* To prevent beeps, don't let DefWindowProcW() be called */
	  return_val = TRUE;
	  goto done;
	}
      break;

    case WM_IME_STARTCOMPOSITION:
      in_ime_composition = TRUE;
      break;

    case WM_IME_ENDCOMPOSITION:
      in_ime_composition = FALSE;
      break;

    case WM_IME_COMPOSITION:
      /* On Win2k WM_IME_CHAR doesn't work correctly for non-Unicode
       * applications. Thus, handle WM_IME_COMPOSITION with
       * GCS_RESULTSTR instead, fetch the Unicode chars from the IME
       * with ImmGetCompositionStringW().
       *
       * See for instance
       * http://groups.google.com/groups?selm=natX5.57%24g77.19788%40nntp2.onemain.com
       * and
       * http://groups.google.com/groups?selm=u2XfrXw5BHA.1628%40tkmsftngp02
       * for comments by other people that seems to have the same
       * experience. WM_IME_CHAR just gives question marks, apparently
       * because of going through some conversion to the current code
       * page.
       *
       * WM_IME_CHAR might work on NT4 or Win9x with ActiveIMM, but
       * use WM_IME_COMPOSITION there, too, to simplify the code.
       */
      CDK_NOTE (EVENTS, g_print (" %#lx", (long) msg->lParam));

      if (!(msg->lParam & GCS_RESULTSTR))
	break;

      if (keyboard_grab &&
          !propagate (&window, msg,
		      keyboard_grab->window,
		      keyboard_grab->owner_events,
		      CDK_ALL_EVENTS_MASK,
		      doesnt_want_char))
	break;

      if (CDK_WINDOW_DESTROYED (window))
	break;

      himc = ImmGetContext (msg->hwnd);
      ccount = ImmGetCompositionStringW (himc, GCS_RESULTSTR,
					 wbuf, sizeof (wbuf));
      ImmReleaseContext (msg->hwnd, himc);

      ccount /= 2;

      API_CALL (GetKeyboardState, (key_state));

      for (i = 0; i < ccount; i++)
	{
	  if (window->event_mask & CDK_KEY_PRESS_MASK)
	    {
	      /* Build a key press event */
	      event = cdk_event_new (CDK_KEY_PRESS);
	      event->key.window = window;
	      cdk_event_set_device (event, device_manager_win32->core_keyboard);
	      cdk_event_set_source_device (event, device_manager_win32->system_keyboard);
              cdk_event_set_seat (event, cdk_device_get_seat (device_manager_win32->core_keyboard));
	      build_wm_ime_composition_event (event, msg, wbuf[i], key_state);

	      _cdk_win32_append_event (event);
	    }

	  if (window->event_mask & CDK_KEY_RELEASE_MASK)
	    {
	      /* Build a key release event.  */
	      event = cdk_event_new (CDK_KEY_RELEASE);
	      event->key.window = window;
	      cdk_event_set_device (event, device_manager_win32->core_keyboard);
	      cdk_event_set_source_device (event, device_manager_win32->system_keyboard);
              cdk_event_set_seat (event, cdk_device_get_seat (device_manager_win32->core_keyboard));
	      build_wm_ime_composition_event (event, msg, wbuf[i], key_state);

	      _cdk_win32_append_event (event);
	    }
	}
      return_val = TRUE;
      break;

    case WM_LBUTTONDOWN:
      button = 1;
      goto buttondown0;

    case WM_MBUTTONDOWN:
      button = 2;
      goto buttondown0;

    case WM_RBUTTONDOWN:
      button = 3;
      goto buttondown0;

    case WM_XBUTTONDOWN:
      if (HIWORD (msg->wParam) == XBUTTON1)
	button = 4;
      else
	button = 5;

    buttondown0:
      CDK_NOTE (EVENTS,
		g_print (" (%d,%d)",
			 GET_X_LPARAM (msg->lParam), GET_Y_LPARAM (msg->lParam)));

      g_set_object (&window, find_window_for_mouse_event (window, msg));
      /* TODO_CSW?: there used to some synthesize and propagate */
      if (CDK_WINDOW_DESTROYED (window))
	break;

      if (pointer_grab == NULL)
	{
	  SetCapture (CDK_WINDOW_HWND (window));
	}

      generate_button_event (CDK_BUTTON_PRESS, button,
			     window, msg);

      return_val = TRUE;
      break;

    case WM_LBUTTONUP:
      button = 1;
      goto buttonup0;

    case WM_MBUTTONUP:
      button = 2;
      goto buttonup0;

    case WM_RBUTTONUP:
      button = 3;
      goto buttonup0;

    case WM_XBUTTONUP:
      if (HIWORD (msg->wParam) == XBUTTON1)
	button = 4;
      else
	button = 5;

    buttonup0:
      CDK_NOTE (EVENTS,
		g_print (" (%d,%d)",
			 GET_X_LPARAM (msg->lParam), GET_Y_LPARAM (msg->lParam)));

      g_set_object (&window, find_window_for_mouse_event (window, msg));

      if (pointer_grab != NULL && pointer_grab->implicit)
	{
	  gint state = build_pointer_event_state (msg);

	  /* We keep the implicit grab until no buttons at all are held down */
	  if ((state & CDK_ANY_BUTTON_MASK & ~(CDK_BUTTON1_MASK << (button - 1))) == 0)
	    {
	      CdkWindow *native_window = pointer_grab->native_window;

	      ReleaseCapture ();

	      new_window = NULL;
	      hwnd = WindowFromPoint (msg->pt);
	      if (hwnd != NULL)
		{
		  POINT client_pt = msg->pt;

		  ScreenToClient (hwnd, &client_pt);
		  GetClientRect (hwnd, &rect);
		  if (PtInRect (&rect, client_pt))
		    new_window = cdk_win32_handle_table_lookup (hwnd);
		}

	      synthesize_crossing_events (display,
					  native_window, new_window,
					  CDK_CROSSING_UNGRAB,
					  &msg->pt,
					  0, /* TODO: Set right mask */
					  msg->time,
					  FALSE);
	      g_set_object (&mouse_window, new_window);
	      mouse_window_ignored_leave = NULL;
	    }
	}

      generate_button_event (CDK_BUTTON_RELEASE, button,
			     window, msg);

      impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

      /* End a drag op when the same button that started it is released */
      if (impl->drag_move_resize_context.op != CDK_WIN32_DRAGOP_NONE &&
          impl->drag_move_resize_context.button == button)
        cdk_win32_window_end_move_resize_drag (window);

      return_val = TRUE;
      break;

    case WM_MOUSEMOVE:
      CDK_NOTE (EVENTS,
		g_print (" %p (%d,%d)",
			 (gpointer) msg->wParam,
			 GET_X_LPARAM (msg->lParam), GET_Y_LPARAM (msg->lParam)));

      new_window = window;

      if (pointer_grab != NULL)
	{
	  POINT pt;
	  pt = msg->pt;

	  new_window = NULL;
	  hwnd = WindowFromPoint (pt);
	  if (hwnd != NULL)
	    {
	      POINT client_pt = pt;

	      ScreenToClient (hwnd, &client_pt);
	      GetClientRect (hwnd, &rect);
	      if (PtInRect (&rect, client_pt))
		new_window = cdk_win32_handle_table_lookup (hwnd);
	    }

	  if (!pointer_grab->owner_events &&
	      new_window != NULL &&
	      new_window != pointer_grab->native_window)
	    new_window = NULL;
	}

      if (mouse_window != new_window)
	{
	  CDK_NOTE (EVENTS, g_print (" mouse_sinwod %p -> %p",
				     mouse_window ? CDK_WINDOW_HWND (mouse_window) : NULL,
				     new_window ? CDK_WINDOW_HWND (new_window) : NULL));
	  synthesize_crossing_events (display,
				      mouse_window, new_window,
				      CDK_CROSSING_NORMAL,
				      &msg->pt,
				      0, /* TODO: Set right mask */
				      msg->time,
				      FALSE);
	  g_set_object (&mouse_window, new_window);
	  mouse_window_ignored_leave = NULL;
	  if (new_window != NULL)
	    track_mouse_event (TME_LEAVE, CDK_WINDOW_HWND (new_window));
	}
      else if (new_window != NULL &&
	       new_window == mouse_window_ignored_leave)
	{
	  /* If we ignored a leave event for this window and we're now getting
	     input again we need to re-arm the mouse tracking, as that was
	     cancelled by the mouseleave. */
	  mouse_window_ignored_leave = NULL;
	  track_mouse_event (TME_LEAVE, CDK_WINDOW_HWND (new_window));
	}

      g_set_object (&window, find_window_for_mouse_event (window, msg));
      impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

      /* If we haven't moved, don't create any CDK event. Windows
       * sends WM_MOUSEMOVE messages after a new window is shows under
       * the mouse, even if the mouse hasn't moved. This disturbs ctk.
       */
      if ((msg->pt.x + _cdk_offset_x) / impl->window_scale == current_root_x &&
	  (msg->pt.y + _cdk_offset_y) / impl->window_scale == current_root_y)
	break;

      current_root_x = (msg->pt.x + _cdk_offset_x) / impl->window_scale;
      current_root_y = (msg->pt.y + _cdk_offset_y) / impl->window_scale;


      if (impl->drag_move_resize_context.op != CDK_WIN32_DRAGOP_NONE)
        {
          cdk_win32_window_do_move_resize_drag (window, current_root_x, current_root_y);
        }
      else if (_cdk_input_ignore_core == 0)
	{
	  event = cdk_event_new (CDK_MOTION_NOTIFY);
	  event->motion.window = window;
	  event->motion.time = _cdk_win32_get_next_tick (msg->time);
	  event->motion.x = current_x = (gint16) GET_X_LPARAM (msg->lParam) / impl->window_scale;
	  event->motion.y = current_y = (gint16) GET_Y_LPARAM (msg->lParam) / impl->window_scale;
	  event->motion.x_root = current_root_x;
	  event->motion.y_root = current_root_y;
	  event->motion.axes = NULL;
	  event->motion.state = build_pointer_event_state (msg);
	  event->motion.is_hint = FALSE;
	  cdk_event_set_device (event, device_manager_win32->core_pointer);
	  cdk_event_set_source_device (event, device_manager_win32->system_pointer);
          cdk_event_set_seat (event, cdk_device_get_seat (device_manager_win32->core_pointer));

	  _cdk_win32_append_event (event);
	}

      return_val = TRUE;
      break;

    case WM_NCMOUSEMOVE:
      CDK_NOTE (EVENTS,
		g_print (" (%d,%d)",
			 GET_X_LPARAM (msg->lParam), GET_Y_LPARAM (msg->lParam)));
      break;

    case WM_MOUSELEAVE:
      CDK_NOTE (EVENTS, g_print (" %d (%ld,%ld)",
				 HIWORD (msg->wParam), msg->pt.x, msg->pt.y));

      new_window = NULL;
      hwnd = WindowFromPoint (msg->pt);
      ignore_leave = FALSE;
      if (hwnd != NULL)
	{
	  char classname[64];

	  POINT client_pt = msg->pt;

	  /* The synapitics trackpad drivers have this irritating
	     feature where it pops up a window right under the pointer
	     when you scroll. We ignore the leave and enter events for
	     this window */
	  if (GetClassNameA (hwnd, classname, sizeof(classname)) &&
	      strcmp (classname, SYNAPSIS_ICON_WINDOW_CLASS) == 0)
	    ignore_leave = TRUE;

	  ScreenToClient (hwnd, &client_pt);
	  GetClientRect (hwnd, &rect);
	  if (PtInRect (&rect, client_pt))
	    new_window = cdk_win32_handle_table_lookup (hwnd);
	}

      if (!ignore_leave)
	synthesize_crossing_events (display,
				    mouse_window, new_window,
				    CDK_CROSSING_NORMAL,
				    &msg->pt,
				    0, /* TODO: Set right mask */
				    msg->time,
				    FALSE);
      g_set_object (&mouse_window, new_window);
      mouse_window_ignored_leave = ignore_leave ? new_window : NULL;


      return_val = TRUE;
      break;

    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
      CDK_NOTE (EVENTS, g_print (" %d", (short) HIWORD (msg->wParam)));

      /* WM_MOUSEWHEEL is delivered to the focus window. Work around
       * that. Also, the position is in screen coordinates, not client
       * coordinates as with the button messages. I love the
       * consistency of Windows.
       */
      point.x = GET_X_LPARAM (msg->lParam);
      point.y = GET_Y_LPARAM (msg->lParam);

      if ((hwnd = WindowFromPoint (point)) == NULL)
	break;

      {
	char classname[64];

	/* The synapitics trackpad drivers have this irritating
	   feature where it pops up a window right under the pointer
	   when you scroll. We backtrack and to the toplevel and
	   find the innermost child instead. */
	if (GetClassNameA (hwnd, classname, sizeof(classname)) &&
	    strcmp (classname, SYNAPSIS_ICON_WINDOW_CLASS) == 0)
	  {
	    HWND hwndc;

	    /* Find our toplevel window */
	    hwnd = GetAncestor (msg->hwnd, GA_ROOT);

	    /* Walk back up to the outermost child at the desired point */
	    do {
	      ScreenToClient (hwnd, &point);
	      hwndc = ChildWindowFromPoint (hwnd, point);
	      ClientToScreen (hwnd, &point);
	    } while (hwndc != hwnd && (hwnd = hwndc, 1));
	  }
      }

      msg->hwnd = hwnd;
      if ((new_window = cdk_win32_handle_table_lookup (msg->hwnd)) == NULL)
	break;

      if (new_window != window)
	{
	  g_set_object (&window, new_window);
	}

      impl = CDK_WINDOW_IMPL_WIN32 (window->impl);
      ScreenToClient (msg->hwnd, &point);

      event = cdk_event_new (CDK_SCROLL);
      event->scroll.window = window;
      event->scroll.direction = CDK_SCROLL_SMOOTH;

      if (msg->message == WM_MOUSEWHEEL)
        {
          event->scroll.delta_y = (gdouble) GET_WHEEL_DELTA_WPARAM (msg->wParam) / (gdouble) WHEEL_DELTA;
        }
      else if (msg->message == WM_MOUSEHWHEEL)
        {
          event->scroll.delta_x = (gdouble) GET_WHEEL_DELTA_WPARAM (msg->wParam) / (gdouble) WHEEL_DELTA;
        }
      /* Positive delta scrolls up, not down,
         see API documentation for WM_MOUSEWHEEL message.
       */
      event->scroll.delta_y *= -1.0;
      event->scroll.time = _cdk_win32_get_next_tick (msg->time);
      event->scroll.x = (gint16) point.x / impl->window_scale;
      event->scroll.y = (gint16) point.y / impl->window_scale;
      event->scroll.x_root = ((gint16) GET_X_LPARAM (msg->lParam) + _cdk_offset_x) / impl->window_scale;
      event->scroll.y_root = ((gint16) GET_Y_LPARAM (msg->lParam) + _cdk_offset_y) / impl->window_scale;
      event->scroll.state = build_pointer_event_state (msg);
      cdk_event_set_device (event, device_manager_win32->core_pointer);
      cdk_event_set_source_device (event, device_manager_win32->system_pointer);
      cdk_event_set_seat (event, cdk_device_get_seat (device_manager_win32->core_pointer));
      cdk_event_set_pointer_emulated (event, FALSE);

      _cdk_win32_append_event (cdk_event_copy (event));

      /* Append the discrete version too */
      if (msg->message == WM_MOUSEWHEEL)
	event->scroll.direction = (((short) HIWORD (msg->wParam)) > 0) ?
	  CDK_SCROLL_UP : CDK_SCROLL_DOWN;
      else if (msg->message == WM_MOUSEHWHEEL)
	event->scroll.direction = (((short) HIWORD (msg->wParam)) > 0) ?
	  CDK_SCROLL_RIGHT : CDK_SCROLL_LEFT;
      event->scroll.delta_x = 0;
      event->scroll.delta_y = 0;
      cdk_event_set_pointer_emulated (event, TRUE);

      _cdk_win32_append_event (event);

      return_val = TRUE;
      break;

    case WM_HSCROLL:
      /* Just print more debugging information, don't actually handle it. */
      CDK_NOTE (EVENTS,
		(g_print (" %s",
			  (LOWORD (msg->wParam) == SB_ENDSCROLL ? "ENDSCROLL" :
			   (LOWORD (msg->wParam) == SB_LEFT ? "LEFT" :
			    (LOWORD (msg->wParam) == SB_RIGHT ? "RIGHT" :
			     (LOWORD (msg->wParam) == SB_LINELEFT ? "LINELEFT" :
			      (LOWORD (msg->wParam) == SB_LINERIGHT ? "LINERIGHT" :
			       (LOWORD (msg->wParam) == SB_PAGELEFT ? "PAGELEFT" :
				(LOWORD (msg->wParam) == SB_PAGERIGHT ? "PAGERIGHT" :
				 (LOWORD (msg->wParam) == SB_THUMBPOSITION ? "THUMBPOSITION" :
				  (LOWORD (msg->wParam) == SB_THUMBTRACK ? "THUMBTRACK" :
				   "???")))))))))),
		 (LOWORD (msg->wParam) == SB_THUMBPOSITION ||
		  LOWORD (msg->wParam) == SB_THUMBTRACK) ?
		 (g_print (" %d", HIWORD (msg->wParam)), 0) : 0));
      break;

    case WM_VSCROLL:
      /* Just print more debugging information, don't actually handle it. */
      CDK_NOTE (EVENTS,
		(g_print (" %s",
			  (LOWORD (msg->wParam) == SB_ENDSCROLL ? "ENDSCROLL" :
			   (LOWORD (msg->wParam) == SB_BOTTOM ? "BOTTOM" :
			    (LOWORD (msg->wParam) == SB_TOP ? "TOP" :
			     (LOWORD (msg->wParam) == SB_LINEDOWN ? "LINDOWN" :
			      (LOWORD (msg->wParam) == SB_LINEUP ? "LINEUP" :
			       (LOWORD (msg->wParam) == SB_PAGEDOWN ? "PAGEDOWN" :
				(LOWORD (msg->wParam) == SB_PAGEUP ? "PAGEUP" :
				 (LOWORD (msg->wParam) == SB_THUMBPOSITION ? "THUMBPOSITION" :
				  (LOWORD (msg->wParam) == SB_THUMBTRACK ? "THUMBTRACK" :
				   "???")))))))))),
		 (LOWORD (msg->wParam) == SB_THUMBPOSITION ||
		  LOWORD (msg->wParam) == SB_THUMBTRACK) ?
		 (g_print (" %d", HIWORD (msg->wParam)), 0) : 0));
      break;

     case WM_MOUSEACTIVATE:
       {
	 if (cdk_window_get_window_type (window) == CDK_WINDOW_TEMP
	     || !window->accept_focus)
	   {
	     *ret_valp = MA_NOACTIVATE;
	     return_val = TRUE;
	   }

	 if (_cdk_modal_blocked (cdk_window_get_toplevel (window)))
	   {
	     *ret_valp = MA_NOACTIVATEANDEAT;
	     return_val = TRUE;
	   }
       }

       break;

    case WM_KILLFOCUS:
      if (keyboard_grab != NULL &&
	  !CDK_WINDOW_DESTROYED (keyboard_grab->window) &&
	  (_modal_operation_in_progress & CDK_WIN32_MODAL_OP_DND) == 0)
	{
	  generate_grab_broken_event (device_manager, keyboard_grab->window, TRUE, NULL);
	}

      /* fallthrough */
    case WM_SETFOCUS:
      if (keyboard_grab != NULL &&
	  !keyboard_grab->owner_events)
	break;

      if (!(window->event_mask & CDK_FOCUS_CHANGE_MASK))
	break;

      if (CDK_WINDOW_DESTROYED (window))
	break;

      generate_focus_event (device_manager, window, (msg->message == WM_SETFOCUS));
      return_val = TRUE;
      break;

    case WM_ERASEBKGND:
      CDK_NOTE (EVENTS, g_print (" %p", (HANDLE) msg->wParam));

      if (CDK_WINDOW_DESTROYED (window))
	break;

      return_val = TRUE;
      *ret_valp = 1;
      break;

    case WM_SYNCPAINT:
      sync_timer = SetTimer (CDK_WINDOW_HWND (window),
			     1,
			     200, sync_timer_proc);
      break;

    case WM_PAINT:
      handle_wm_paint (msg, window);
      break;

    case WM_SETCURSOR:
      CDK_NOTE (EVENTS, g_print (" %#x %#x",
				 LOWORD (msg->lParam), HIWORD (msg->lParam)));

      if (pointer_grab != NULL)
        grab_window = pointer_grab->window;

      if (grab_window == NULL && LOWORD (msg->lParam) != HTCLIENT)
	break;

      if (grab_window != NULL && _cdk_win32_grab_cursor != NULL)
	cursor = _cdk_win32_grab_cursor;
      else if (!CDK_WINDOW_DESTROYED (window) && CDK_WINDOW_IMPL_WIN32 (window->impl)->cursor != NULL)
	cursor = CDK_WINDOW_IMPL_WIN32 (window->impl)->cursor;
      else
	cursor = NULL;

      if (cursor != NULL)
        {
	  CDK_NOTE (EVENTS, g_print (" (SetCursor(%p)", cursor));
	  SetCursor (CDK_WIN32_CURSOR (cursor)->hcursor);
          return_val = TRUE;
          *ret_valp = TRUE;
        }
      break;

    case WM_SYSMENU:
      return_val = handle_wm_sysmenu (window, msg, ret_valp);
      break;

    case WM_INITMENU:
      impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

      if (impl->have_temp_styles)
        {
          LONG_PTR window_style;

          window_style = GetWindowLongPtr (CDK_WINDOW_HWND (window),
                                           GWL_STYLE);
          /* Handling WM_SYSMENU added extra styles to this window,
           * remove them now.
           */
          window_style &= ~impl->temp_styles;
          SetWindowLongPtr (CDK_WINDOW_HWND (window),
                            GWL_STYLE,
                            window_style);
        }

      break;

    case WM_SYSCOMMAND:
      switch (msg->wParam)
	{
	case SC_MINIMIZE:
	case SC_RESTORE:
	  do_show_window (window, msg->wParam == SC_MINIMIZE ? TRUE : FALSE);

    if (msg->wParam == SC_RESTORE)
      _cdk_win32_window_invalidate_egl_framebuffer (window);
	  break;
        case SC_MAXIMIZE:
          impl = CDK_WINDOW_IMPL_WIN32 (window->impl);
          impl->maximizing = TRUE;
	  break;
	}

      break;

    case WM_ENTERSIZEMOVE:
      _modal_move_resize_window = msg->hwnd;
      _cdk_win32_begin_modal_call (CDK_WIN32_MODAL_OP_SIZEMOVE_MASK);
      break;

    case WM_EXITSIZEMOVE:
      if (_modal_operation_in_progress & CDK_WIN32_MODAL_OP_SIZEMOVE_MASK)
	{
	  _modal_move_resize_window = NULL;
	  _cdk_win32_end_modal_call (CDK_WIN32_MODAL_OP_SIZEMOVE_MASK);
	}
      break;

    case WM_ENTERMENULOOP:
      _cdk_win32_begin_modal_call (CDK_WIN32_MODAL_OP_MENU);
      break;

    case WM_EXITMENULOOP:
      if (_modal_operation_in_progress & CDK_WIN32_MODAL_OP_MENU)
	_cdk_win32_end_modal_call (CDK_WIN32_MODAL_OP_MENU);
      break;

      break;

    /*
     * Handle WM_CANCELMODE and do nothing in response to it when DnD is
     * active. Otherwise pass it to DefWindowProc, which will call ReleaseCapture()
     * on our behalf.
     * This prevents us from losing mouse capture when alt-tabbing during DnD
     * (this includes the feature of Windows Explorer where dragging stuff over
     * a window button in the taskbar causes that window to receive focus, i.e.
     * keyboardless alt-tabbing).
     */
    case WM_CANCELMODE:
      if (_modal_operation_in_progress & CDK_WIN32_MODAL_OP_DND)
        {
          return_val = TRUE;
          *ret_valp = 0;
        }
      break;

    case WM_CAPTURECHANGED:
      /* Sometimes we don't get WM_EXITSIZEMOVE, for instance when you
	 select move/size in the menu and then click somewhere without
	 moving/resizing. We work around this using WM_CAPTURECHANGED. */
      if (_modal_operation_in_progress & CDK_WIN32_MODAL_OP_SIZEMOVE_MASK)
	{
	  _modal_move_resize_window = NULL;
	  _cdk_win32_end_modal_call (CDK_WIN32_MODAL_OP_SIZEMOVE_MASK);
	}

      impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

      if (impl->drag_move_resize_context.op != CDK_WIN32_DRAGOP_NONE)
        cdk_win32_window_end_move_resize_drag (window);
      break;

    case WM_WINDOWPOSCHANGING:
      CDK_NOTE (EVENTS, (windowpos = (WINDOWPOS *) msg->lParam,
			 g_print (" %s %s %dx%d@%+d%+d now below %p",
				  _cdk_win32_window_pos_bits_to_string (windowpos->flags),
				  (windowpos->hwndInsertAfter == HWND_BOTTOM ? "BOTTOM" :
				   (windowpos->hwndInsertAfter == HWND_NOTOPMOST ? "NOTOPMOST" :
				    (windowpos->hwndInsertAfter == HWND_TOP ? "TOP" :
				     (windowpos->hwndInsertAfter == HWND_TOPMOST ? "TOPMOST" :
				      (sprintf (buf, "%p", windowpos->hwndInsertAfter),
				       buf))))),
				  windowpos->cx, windowpos->cy, windowpos->x, windowpos->y,
				  GetNextWindow (msg->hwnd, GW_HWNDPREV))));

      if (CDK_WINDOW_IS_MAPPED (window))
        {
          impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

          if (impl->maximizing)
            {
              MINMAXINFO our_mmi;

              _cdk_win32_window_invalidate_egl_framebuffer (window);

              if (_cdk_win32_window_fill_min_max_info (window, &our_mmi))
                {
                  windowpos = (WINDOWPOS *) msg->lParam;
                  windowpos->cx = our_mmi.ptMaxSize.x;
                  windowpos->cy = our_mmi.ptMaxSize.y;
                }

              impl->maximizing = FALSE;
            }
        }

      break;

    case WM_WINDOWPOSCHANGED:
      windowpos = (WINDOWPOS *) msg->lParam;
      CDK_NOTE (EVENTS, g_print (" %s %s %dx%d@%+d%+d",
				 _cdk_win32_window_pos_bits_to_string (windowpos->flags),
				 (windowpos->hwndInsertAfter == HWND_BOTTOM ? "BOTTOM" :
				  (windowpos->hwndInsertAfter == HWND_NOTOPMOST ? "NOTOPMOST" :
				   (windowpos->hwndInsertAfter == HWND_TOP ? "TOP" :
				    (windowpos->hwndInsertAfter == HWND_TOPMOST ? "TOPMOST" :
				     (sprintf (buf, "%p", windowpos->hwndInsertAfter),
				      buf))))),
				 windowpos->cx, windowpos->cy, windowpos->x, windowpos->y));

      /* Break grabs on unmap or minimize */
      if (windowpos->flags & SWP_HIDEWINDOW ||
	  ((windowpos->flags & SWP_STATECHANGED) && IsIconic (msg->hwnd)))
      {
        CdkDevice *device = cdk_device_manager_get_client_pointer (device_manager);

        if ((pointer_grab != NULL && pointer_grab->window == window) ||
            (keyboard_grab != NULL && keyboard_grab->window == window))
          cdk_device_ungrab (device, msg -> time);
    }

      /* Send MAP events  */
      if ((windowpos->flags & SWP_SHOWWINDOW) &&
	  !CDK_WINDOW_DESTROYED (window))
	{
	  event = cdk_event_new (CDK_MAP);
	  event->any.window = window;
	  _cdk_win32_append_event (event);
	}

      /* Update window state */
      if (windowpos->flags & (SWP_STATECHANGED | SWP_SHOWWINDOW | SWP_HIDEWINDOW))
	{
	  CdkWindowState set_bits, unset_bits, old_state, new_state;

	  old_state = window->state;

	  set_bits = 0;
	  unset_bits = 0;

	  if (IsWindowVisible (msg->hwnd))
	    unset_bits |= CDK_WINDOW_STATE_WITHDRAWN;
	  else
	    set_bits |= CDK_WINDOW_STATE_WITHDRAWN;

	  if (IsIconic (msg->hwnd))
	    set_bits |= CDK_WINDOW_STATE_ICONIFIED;
	  else
	    unset_bits |= CDK_WINDOW_STATE_ICONIFIED;

	  if (IsZoomed (msg->hwnd))
	    set_bits |= CDK_WINDOW_STATE_MAXIMIZED;
	  else
	    unset_bits |= CDK_WINDOW_STATE_MAXIMIZED;

	  cdk_synthesize_window_state (window, unset_bits, set_bits);

	  new_state = window->state;

	  /* Whenever one window changes iconified state we need to also
	   * change the iconified state in all transient related windows,
	   * as windows doesn't give icons for transient childrens.
	   */
	  if ((old_state & CDK_WINDOW_STATE_ICONIFIED) !=
	      (new_state & CDK_WINDOW_STATE_ICONIFIED))
	    do_show_window (window, (new_state & CDK_WINDOW_STATE_ICONIFIED));
	}

      /* Show, New size or position => configure event */
      if (!(windowpos->flags & SWP_NOCLIENTMOVE) ||
	  !(windowpos->flags & SWP_NOCLIENTSIZE) ||
	  (windowpos->flags & SWP_SHOWWINDOW))
	{
	  if (CDK_WINDOW_TYPE (window) != CDK_WINDOW_CHILD &&
	      !IsIconic (msg->hwnd) &&
	      !CDK_WINDOW_DESTROYED (window))
	    _cdk_win32_emit_configure_event (window);
	}

      if ((windowpos->flags & SWP_HIDEWINDOW) &&
	  !CDK_WINDOW_DESTROYED (window))
	{
	  /* Send UNMAP events  */
	  event = cdk_event_new (CDK_UNMAP);
	  event->any.window = window;
	  _cdk_win32_append_event (event);

	  /* Make transient parent the forground window when window unmaps */
	  impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

	  if (impl->transient_owner &&
	      GetForegroundWindow () == CDK_WINDOW_HWND (window))
	    SetForegroundWindow (CDK_WINDOW_HWND (impl->transient_owner));
	}

      if (!(windowpos->flags & SWP_NOCLIENTSIZE))
	{
	  if (window->resize_count > 1)
	    window->resize_count -= 1;
	}

      /* Call modal timer immediate so that we repaint faster after a resize. */
      if (_modal_operation_in_progress & CDK_WIN32_MODAL_OP_SIZEMOVE_MASK)
	modal_timer_proc (0,0,0,0);

      /* Claim as handled, so that WM_SIZE and WM_MOVE are avoided */
      return_val = TRUE;
      *ret_valp = 0;
      break;

    case WM_SIZING:
      GetWindowRect (CDK_WINDOW_HWND (window), &rect);
      drag = (RECT *) msg->lParam;
      CDK_NOTE (EVENTS, g_print (" %s curr:%s drag:%s",
				 (msg->wParam == WMSZ_BOTTOM ? "BOTTOM" :
				  (msg->wParam == WMSZ_BOTTOMLEFT ? "BOTTOMLEFT" :
				   (msg->wParam == WMSZ_LEFT ? "LEFT" :
				    (msg->wParam == WMSZ_TOPLEFT ? "TOPLEFT" :
				     (msg->wParam == WMSZ_TOP ? "TOP" :
				      (msg->wParam == WMSZ_TOPRIGHT ? "TOPRIGHT" :
				       (msg->wParam == WMSZ_RIGHT ? "RIGHT" :

					(msg->wParam == WMSZ_BOTTOMRIGHT ? "BOTTOMRIGHT" :
					 "???")))))))),
				 _cdk_win32_rect_to_string (&rect),
				 _cdk_win32_rect_to_string (drag)));

      impl = CDK_WINDOW_IMPL_WIN32 (window->impl);
      orig_drag = *drag;
      if (impl->hint_flags & CDK_HINT_RESIZE_INC)
	{
	  CDK_NOTE (EVENTS, g_print (" (RESIZE_INC)"));
	  if (impl->hint_flags & CDK_HINT_BASE_SIZE)
	    {
	      /* Resize in increments relative to the base size */
	      rect.left = rect.top = 0;
	      rect.right = impl->hints.base_width * impl->window_scale;
	      rect.bottom = impl->hints.base_height * impl->window_scale;
	      _cdk_win32_adjust_client_rect (window, &rect);
	      point.x = rect.left;
	      point.y = rect.top;
	      ClientToScreen (CDK_WINDOW_HWND (window), &point);
	      rect.left = point.x;
	      rect.top = point.y;
	      point.x = rect.right;
	      point.y = rect.bottom;
	      ClientToScreen (CDK_WINDOW_HWND (window), &point);
	      rect.right = point.x;
	      rect.bottom = point.y;

	      CDK_NOTE (EVENTS, g_print (" (also BASE_SIZE, using %s)",
					 _cdk_win32_rect_to_string (&rect)));
	    }

	  switch (msg->wParam)
	    {
	    case WMSZ_BOTTOM:
	      if (drag->bottom == rect.bottom)
		break;
        adjust_drag (&drag->bottom, rect.bottom, impl->hints.height_inc * impl->window_scale);
	      break;

	    case WMSZ_BOTTOMLEFT:
	      if (drag->bottom == rect.bottom && drag->left == rect.left)
		break;
	      adjust_drag (&drag->bottom, rect.bottom, impl->hints.height_inc * impl->window_scale);
	      adjust_drag (&drag->left, rect.left, impl->hints.width_inc * impl->window_scale);
	      break;

	    case WMSZ_LEFT:
	      if (drag->left == rect.left)
		break;
	      adjust_drag (&drag->left, rect.left, impl->hints.width_inc * impl->window_scale);
	      break;

	    case WMSZ_TOPLEFT:
	      if (drag->top == rect.top && drag->left == rect.left)
		break;
	      adjust_drag (&drag->top, rect.top, impl->hints.height_inc * impl->window_scale);
	      adjust_drag (&drag->left, rect.left, impl->hints.width_inc * impl->window_scale);
	      break;

	    case WMSZ_TOP:
	      if (drag->top == rect.top)
		break;
	      adjust_drag (&drag->top, rect.top, impl->hints.height_inc * impl->window_scale);
	      break;

	    case WMSZ_TOPRIGHT:
	      if (drag->top == rect.top && drag->right == rect.right)
		break;
	      adjust_drag (&drag->top, rect.top, impl->hints.height_inc * impl->window_scale);
	      adjust_drag (&drag->right, rect.right, impl->hints.width_inc * impl->window_scale);
	      break;

	    case WMSZ_RIGHT:
	      if (drag->right == rect.right)
		break;
	      adjust_drag (&drag->right, rect.right, impl->hints.width_inc * impl->window_scale);
	      break;

	    case WMSZ_BOTTOMRIGHT:
	      if (drag->bottom == rect.bottom && drag->right == rect.right)
		break;
	      adjust_drag (&drag->bottom, rect.bottom, impl->hints.height_inc * impl->window_scale);
	      adjust_drag (&drag->right, rect.right, impl->hints.width_inc * impl->window_scale);
	      break;
	    }

	  if (drag->bottom != orig_drag.bottom || drag->left != orig_drag.left ||
	      drag->top != orig_drag.top || drag->right != orig_drag.right)
	    {
	      *ret_valp = TRUE;
	      return_val = TRUE;
	      CDK_NOTE (EVENTS, g_print (" (handled RESIZE_INC: %s)",
					 _cdk_win32_rect_to_string (drag)));
	    }
	}

      /* WM_GETMINMAXINFO handles min_size and max_size hints? */

      if (impl->hint_flags & CDK_HINT_ASPECT)
	{
	  RECT decorated_rect;
	  RECT undecorated_drag;
	  int decoration_width, decoration_height;
	  gdouble drag_aspect;
	  int drag_width, drag_height, new_width, new_height;

	  GetClientRect (CDK_WINDOW_HWND (window), &rect);
	  decorated_rect = rect;
	  _cdk_win32_adjust_client_rect (window, &decorated_rect);

	  /* Set undecorated_drag to the client area being dragged
	   * out, in screen coordinates.
	   */
	  undecorated_drag = *drag;
	  undecorated_drag.left -= decorated_rect.left - rect.left;
	  undecorated_drag.right -= decorated_rect.right - rect.right;
	  undecorated_drag.top -= decorated_rect.top - rect.top;
	  undecorated_drag.bottom -= decorated_rect.bottom - rect.bottom;

	  decoration_width = (decorated_rect.right - decorated_rect.left) - (rect.right - rect.left);
	  decoration_height = (decorated_rect.bottom - decorated_rect.top) - (rect.bottom - rect.top);

	  drag_width = undecorated_drag.right - undecorated_drag.left;
	  drag_height = undecorated_drag.bottom - undecorated_drag.top;

	  drag_aspect = (gdouble) drag_width / drag_height;

	  CDK_NOTE (EVENTS, g_print (" (ASPECT:%g--%g curr: %g)",
				     impl->hints.min_aspect, impl->hints.max_aspect, drag_aspect));

	  if (drag_aspect < impl->hints.min_aspect)
	    {
	      /* Aspect is getting too narrow */
	      switch (msg->wParam)
		{
		case WMSZ_BOTTOM:
		case WMSZ_TOP:
		  /* User drags top or bottom edge outward. Keep height, increase width. */
		  new_width = impl->hints.min_aspect * drag_height;
		  drag->left -= (new_width - drag_width) / 2;
		  drag->right = drag->left + new_width + decoration_width;
		  break;
		case WMSZ_BOTTOMLEFT:
		case WMSZ_BOTTOMRIGHT:
		  /* User drags bottom-left or bottom-right corner down. Adjust height. */
		  new_height = drag_width / impl->hints.min_aspect;
		  drag->bottom = drag->top + new_height + decoration_height;
		  break;
		case WMSZ_LEFT:
		case WMSZ_RIGHT:
		  /* User drags left or right edge inward. Decrease height */
		  new_height = drag_width / impl->hints.min_aspect;
		  drag->top += (drag_height - new_height) / 2;
		  drag->bottom = drag->top + new_height + decoration_height;
		  break;
		case WMSZ_TOPLEFT:
		case WMSZ_TOPRIGHT:
		  /* User drags top-left or top-right corner up. Adjust height. */
		  new_height = drag_width / impl->hints.min_aspect;
		  drag->top = drag->bottom - new_height - decoration_height;
		}
	    }
	  else if (drag_aspect > impl->hints.max_aspect)
	    {
	      /* Aspect is getting too wide */
	      switch (msg->wParam)
		{
		case WMSZ_BOTTOM:
		case WMSZ_TOP:
		  /* User drags top or bottom edge inward. Decrease width. */
		  new_width = impl->hints.max_aspect * drag_height;
		  drag->left += (drag_width - new_width) / 2;
		  drag->right = drag->left + new_width + decoration_width;
		  break;
		case WMSZ_BOTTOMLEFT:
		case WMSZ_TOPLEFT:
		  /* User drags bottom-left or top-left corner left. Adjust width. */
		  new_width = impl->hints.max_aspect * drag_height;
		  drag->left = drag->right - new_width - decoration_width;
		  break;
		case WMSZ_BOTTOMRIGHT:
		case WMSZ_TOPRIGHT:
		  /* User drags bottom-right or top-right corner right. Adjust width. */
		  new_width = impl->hints.max_aspect * drag_height;
		  drag->right = drag->left + new_width + decoration_width;
		  break;
		case WMSZ_LEFT:
		case WMSZ_RIGHT:
		  /* User drags left or right edge outward. Increase height. */
		  new_height = drag_width / impl->hints.max_aspect;
		  drag->top -= (new_height - drag_height) / 2;
		  drag->bottom = drag->top + new_height + decoration_height;
		  break;
		}
	    }

	  *ret_valp = TRUE;
	  return_val = TRUE;
	  CDK_NOTE (EVENTS, g_print (" (handled ASPECT: %s)",
				     _cdk_win32_rect_to_string (drag)));
	}
      break;

    case WM_GETMINMAXINFO:
      mmi = (MINMAXINFO*) msg->lParam;

      CDK_NOTE (EVENTS, g_print (" (mintrack:%ldx%ld maxtrack:%ldx%ld "
				 "maxpos:%+ld%+ld maxsize:%ldx%ld)",
				 mmi->ptMinTrackSize.x, mmi->ptMinTrackSize.y,
				 mmi->ptMaxTrackSize.x, mmi->ptMaxTrackSize.y,
				 mmi->ptMaxPosition.x, mmi->ptMaxPosition.y,
				 mmi->ptMaxSize.x, mmi->ptMaxSize.y));

      if (_cdk_win32_window_fill_min_max_info (window, mmi))
        {
          /* Don't call DefWindowProcW() */
          CDK_NOTE (EVENTS,
                    g_print (" (handled, mintrack:%ldx%ld maxtrack:%ldx%ld "
                             "maxpos:%+ld%+ld maxsize:%ldx%ld)",
                             mmi->ptMinTrackSize.x, mmi->ptMinTrackSize.y,
                             mmi->ptMaxTrackSize.x, mmi->ptMaxTrackSize.y,
                             mmi->ptMaxPosition.x, mmi->ptMaxPosition.y,
                             mmi->ptMaxSize.x, mmi->ptMaxSize.y));

          return_val = TRUE;
        }

      break;

    case WM_CLOSE:
      if (CDK_WINDOW_DESTROYED (window))
	break;

      event = cdk_event_new (CDK_DELETE);
      event->any.window = window;

      _cdk_win32_append_event (event);

      impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

      if (impl->transient_owner && GetForegroundWindow() == CDK_WINDOW_HWND (window))
	{
	  SetForegroundWindow (CDK_WINDOW_HWND (impl->transient_owner));
	}

      return_val = TRUE;
      break;

    case WM_NCDESTROY:
      if ((pointer_grab != NULL && pointer_grab -> window == window) ||
          (keyboard_grab && keyboard_grab -> window == window))
      {
        CdkDevice *device = cdk_device_manager_get_client_pointer (device_manager);
        cdk_device_ungrab (device, msg -> time);
      }

      if ((window != NULL) && (msg->hwnd != GetDesktopWindow ()))
	cdk_window_destroy_notify (window);

      if (window == NULL || CDK_WINDOW_DESTROYED (window))
	break;

      event = cdk_event_new (CDK_DESTROY);
      event->any.window = window;

      _cdk_win32_append_event (event);

      return_val = TRUE;
      break;

    case WM_DWMCOMPOSITIONCHANGED:
      _cdk_win32_window_enable_transparency (window);
      break;

    case WM_DESTROYCLIPBOARD:
      win32_sel = _cdk_win32_selection_get ();

      if (!win32_sel->ignore_destroy_clipboard)
	{
	  event = cdk_event_new (CDK_SELECTION_CLEAR);
	  event->selection.window = window;
	  event->selection.selection = CDK_SELECTION_CLIPBOARD;
	  event->selection.time = _cdk_win32_get_next_tick (msg->time);
          _cdk_win32_append_event (event);
	}
      else
	{
	  return_val = TRUE;
	}

      break;

    case WM_RENDERFORMAT:
      CDK_NOTE (EVENTS, g_print (" %s", _cdk_win32_cf_to_string (msg->wParam)));

      *ret_valp = 0;
      return_val = TRUE;

      win32_sel = _cdk_win32_selection_get ();

      for (target = NULL, i = 0;
           i < win32_sel->clipboard_selection_targets->len;
           i++)
        {
          CdkSelTargetFormat target_format = g_array_index (win32_sel->clipboard_selection_targets, CdkSelTargetFormat, i);

          if (target_format.format == msg->wParam)
            {
              target = target_format.target;
              win32_sel->property_change_transmute = target_format.transmute;
            }
        }

      if (target == NULL)
        {
          CDK_NOTE (EVENTS, g_print (" (target not found)"));
          break;
        }

      /* We need to render to clipboard immediately, don't call
       * _cdk_win32_append_event()
       */
      event = cdk_event_new (CDK_SELECTION_REQUEST);
      event->selection.window = window;
      event->selection.send_event = FALSE;
      event->selection.selection = CDK_SELECTION_CLIPBOARD;
      event->selection.target = target;
      event->selection.property = _cdk_win32_selection_atom (CDK_WIN32_ATOM_INDEX_CDK_SELECTION);
      event->selection.requestor = cdk_win32_handle_table_lookup (msg->hwnd);
      event->selection.time = msg->time;
      property_change_data = g_new0 (STGMEDIUM, 1);
      win32_sel->property_change_data = property_change_data;
      win32_sel->property_change_format = msg->wParam;
      win32_sel->property_change_target_atom = target;

      fixup_event (event);
      CDK_NOTE (EVENTS, g_print (" (calling _cdk_event_emit)"));
      CDK_NOTE (EVENTS, _cdk_win32_print_event (event));
      _cdk_event_emit (event);
      cdk_event_free (event);
      win32_sel->property_change_format = 0;

      /* Now the clipboard owner should have rendered */
      if (!property_change_data->hGlobal)
        {
          CDK_NOTE (EVENTS, g_print (" (no _delayed_rendering_data?)"));
        }
      else
        {
          /* The requestor is holding the clipboard, no
           * OpenClipboard() is required/possible
           */
          CDK_NOTE (DND,
                    g_print (" SetClipboardData(%s,%p)",
                             _cdk_win32_cf_to_string (msg->wParam),
                             property_change_data->hGlobal));

          API_CALL (SetClipboardData, (msg->wParam, property_change_data->hGlobal));
        }

        g_clear_pointer (&property_change_data, g_free);
        *ret_valp = 0;
        return_val = TRUE;
      break;

    case WM_RENDERALLFORMATS:
      *ret_valp = 0;
      return_val = TRUE;

      win32_sel = _cdk_win32_selection_get ();

      if (API_CALL (OpenClipboard, (msg->hwnd)))
        {
          for (target = NULL, i = 0;
               i < win32_sel->clipboard_selection_targets->len;
               i++)
            {
              CdkSelTargetFormat target_format = g_array_index (win32_sel->clipboard_selection_targets, CdkSelTargetFormat, i);
              if (target_format.format != 0)
                SendMessage (msg->hwnd, WM_RENDERFORMAT, target_format.format, 0);
            }

          API_CALL (CloseClipboard, ());
        }
      break;

    case WM_ACTIVATE:
      CDK_NOTE (EVENTS, g_print (" %s%s %p",
				 (LOWORD (msg->wParam) == WA_ACTIVE ? "ACTIVE" :
				  (LOWORD (msg->wParam) == WA_CLICKACTIVE ? "CLICKACTIVE" :
				   (LOWORD (msg->wParam) == WA_INACTIVE ? "INACTIVE" : "???"))),
				 HIWORD (msg->wParam) ? " minimized" : "",
				 (HWND) msg->lParam));
      /* We handle mouse clicks for modally-blocked windows under WM_MOUSEACTIVATE,
       * but we still need to deal with alt-tab, or with SetActiveWindow() type
       * situations.
       */
      if (_cdk_modal_blocked (window) && LOWORD (msg->wParam) == WA_ACTIVE)
	{
	  CdkWindow *modal_current = _cdk_modal_current ();
	  SetActiveWindow (CDK_WINDOW_HWND (modal_current));
	  *ret_valp = 0;
	  return_val = TRUE;
	  break;
	}

      if (LOWORD (msg->wParam) == WA_INACTIVE)
	cdk_synthesize_window_state (window, CDK_WINDOW_STATE_FOCUSED, 0);
      else
	cdk_synthesize_window_state (window, 0, CDK_WINDOW_STATE_FOCUSED);

      /* Bring any tablet contexts to the top of the overlap order when
       * one of our windows is activated.
       * NOTE: It doesn't seem to work well if it is done in WM_ACTIVATEAPP
       * instead
       */
      if (LOWORD(msg->wParam) != WA_INACTIVE)
	_cdk_input_set_tablet_active ();
      break;

    case WM_ACTIVATEAPP:
      CDK_NOTE (EVENTS, g_print (" %s thread: %" G_GINT64_FORMAT,
				 msg->wParam ? "YES" : "NO",
				 (gint64) msg->lParam));
      break;
    case WM_NCHITTEST:
      /* TODO: pass all messages to DwmDefWindowProc() first! */
      return_val = handle_nchittest (msg->hwnd, window,
                                     GET_X_LPARAM (msg->lParam),
                                     GET_Y_LPARAM (msg->lParam), ret_valp);
      break;

      /* Handle WINTAB events here, as we know that the device manager will
       * use the fixed WT_DEFBASE as lcMsgBase, and we thus can use the
       * constants as case labels.
       */
    case WT_PACKET:
      CDK_NOTE (EVENTS, g_print (" %d %p",
				 (int) msg->wParam, (gpointer) msg->lParam));
      goto wintab;

    case WT_CSRCHANGE:
      CDK_NOTE (EVENTS, g_print (" %d %p",
				 (int) msg->wParam, (gpointer) msg->lParam));
      goto wintab;

    case WT_PROXIMITY:
      CDK_NOTE (EVENTS, g_print (" %p %d %d",
				 (gpointer) msg->wParam,
				 LOWORD (msg->lParam),
				 HIWORD (msg->lParam)));
      /* Fall through */
    wintab:

      event = cdk_event_new (CDK_NOTHING);
      event->any.window = window;
      g_object_ref (window);

      if (cdk_input_other_event (display, event, msg, window))
	_cdk_win32_append_event (event);
      else
	cdk_event_free (event);

      break;
    }

done:

  if (window)
    g_object_unref (window);

#undef return
  return return_val;
}

void
_cdk_win32_display_queue_events (CdkDisplay *display)
{
  MSG msg;

  if (modal_win32_dialog != NULL)
    return;

  while (PeekMessageW (&msg, NULL, 0, 0, PM_REMOVE))
    {
      TranslateMessage (&msg);
      DispatchMessageW (&msg);
    }
}

static gboolean
cdk_event_prepare (GSource *source,
		   gint    *timeout)
{
  CdkWin32EventSource *event_source = (CdkWin32EventSource *)source;
  gboolean retval;

  cdk_threads_enter ();

  *timeout = -1;

  if (event_source->display->event_pause_count > 0)
    retval =_cdk_event_queue_find_first (event_source->display) != NULL;
  else
    retval = (_cdk_event_queue_find_first (event_source->display) != NULL ||
              (modal_win32_dialog == NULL &&
               GetQueueStatus (QS_ALLINPUT) != 0));

  cdk_threads_leave ();

  return retval;
}

static gboolean
cdk_event_check (GSource *source)
{
  CdkWin32EventSource *event_source = (CdkWin32EventSource *)source;
  gboolean retval;

  cdk_threads_enter ();

  if (event_source->display->event_pause_count > 0)
    retval = _cdk_event_queue_find_first (event_source->display) != NULL;
  else if (event_source->event_poll_fd.revents & G_IO_IN)
    retval = (_cdk_event_queue_find_first (event_source->display) != NULL ||
              (modal_win32_dialog == NULL &&
               GetQueueStatus (QS_ALLINPUT) != 0));
  else
    retval = FALSE;

  cdk_threads_leave ();

  return retval;
}

static gboolean
cdk_event_dispatch (GSource     *source,
                    GSourceFunc  callback,
                    gpointer     user_data)
{
  CdkWin32EventSource *event_source = (CdkWin32EventSource *)source;
  CdkEvent *event;

  cdk_threads_enter ();

  _cdk_win32_display_queue_events (event_source->display);
  event = _cdk_event_unqueue (event_source->display);

  if (event)
    {
      CdkWin32Selection *sel_win32 = _cdk_win32_selection_get ();

      _cdk_event_emit (event);

      cdk_event_free (event);

      /* Do drag & drop if it is still pending */
      if (sel_win32->dnd_source_state == CDK_WIN32_DND_PENDING)
        {
          sel_win32->dnd_source_state = CDK_WIN32_DND_DRAGGING;
          _cdk_win32_dnd_do_dragdrop ();
          sel_win32->dnd_source_state = CDK_WIN32_DND_NONE;
        }
    }

  cdk_threads_leave ();

  return TRUE;
}

void
cdk_win32_set_modal_dialog_libctk_only (HWND window)
{
  modal_win32_dialog = window;
}
