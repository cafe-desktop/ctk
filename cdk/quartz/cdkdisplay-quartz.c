/* cdkdisplay-quartz.c
 *
 * Copyright (C) 2005 Imendio AB
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

#include <cdk/cdk.h>
#include <cdk/cdkdisplayprivate.h>
#include <cdk/cdkmonitorprivate.h>

#include "cdkprivate-quartz.h"
#include "cdkquartzscreen.h"
#include "cdkquartzwindow.h"
#include "cdkquartzdisplay.h"
#include "cdkquartzdevicemanager-core.h"
#include "cdkscreen.h"
#include "cdkmonitorprivate.h"
#include "cdkdisplay-quartz.h"
#include "cdkmonitor-quartz.h"
#include "cdkglcontext-quartz.h"

/* Note about coordinates: There are three coordinate systems at play:
 *
 * 1. Core Graphics starts at the origin at the upper right of the
 * main window (the one with the menu bar when you look at arrangement
 * in System Preferences>Displays) and increases down and to the
 * right; up and to the left are negative values of y and x
 * respectively.
 *
 * 2. AppKit (functions beginning with "NS" for NextStep) coordinates
 * also have their origin at the main window, but it's the *lower*
 * left corner and coordinates increase up and to the
 * right. Coordinates below or left of the origin are negative.
 *
 * 3. Cdk coordinates origin is at the upper left corner of the
 * imaginary rectangle enclosing all monitors and like Core Graphics
 * increase down and to the right. There are no negative coordinates.
 *
 * We need to deal with all three because AppKit's NSScreen array is
 * recomputed with new pointers whenever the monitor arrangement
 * changes so we can't cache the references it provides. CoreGraphics
 * screen IDs are constant between reboots so those are what we use to
 * map CdkMonitors and screens, but the sizes and origins must be
 * converted to Cdk coordinates to make sense to Cdk and we must
 * frequently convert between Cdk and AppKit coordinates when
 * determining the drawable area of a monitor and placing windows and
 * views (the latter containing our cairo surfaces for drawing on).
 */

static gint MONITORS_CHANGED = 0;

static void display_reconfiguration_callback (CGDirectDisplayID            display,
                                              CGDisplayChangeSummaryFlags  flags,
                                              void                        *data);

static CdkWindow *
cdk_quartz_display_get_default_group (CdkDisplay *display)
{
/* X11-only. */
  return NULL;
}

CdkDeviceManager *
_cdk_device_manager_new (CdkDisplay *display)
{
  return g_object_new (GDK_TYPE_QUARTZ_DEVICE_MANAGER_CORE,
                       "display", display,
                       NULL);
}

CdkDisplay *
_cdk_quartz_display_open (const gchar *display_name)
{
  if (_cdk_display != NULL)
    return NULL;

  _cdk_display = g_object_new (cdk_quartz_display_get_type (), NULL);
  _cdk_display->device_manager = _cdk_device_manager_new (_cdk_display);

  _cdk_screen = g_object_new (cdk_quartz_screen_get_type (), NULL);
  _cdk_quartz_screen_init_visuals (_cdk_screen);

  _cdk_quartz_window_init_windowing (_cdk_display, _cdk_screen);

  _cdk_quartz_events_init ();

  /* Initialize application */
  [NSApplication sharedApplication];
#if 0
  /* FIXME: Remove the #if 0 when we have these functions */
  _cdk_quartz_dnd_init ();
#endif

  g_signal_emit_by_name (_cdk_display, "opened");

  return _cdk_display;
}

static const gchar *
cdk_quartz_display_get_name (CdkDisplay *display)
{
  static gchar *display_name = NULL;

  if (!display_name)
    {
      GDK_QUARTZ_ALLOC_POOL;
      display_name = g_strdup ([[[NSHost currentHost] name] UTF8String]);
      GDK_QUARTZ_RELEASE_POOL;
    }

  return display_name;
}

static CdkScreen *
cdk_quartz_display_get_default_screen (CdkDisplay *display)
{
  return _cdk_screen;
}

static void
cdk_quartz_display_beep (CdkDisplay *display)
{
  g_return_if_fail (GDK_IS_DISPLAY (display));

  NSBeep();
}

static void
cdk_quartz_display_sync (CdkDisplay *display)
{
  /* Not needed. */
}

static void
cdk_quartz_display_flush (CdkDisplay *display)
{
  /* Not needed. */
}

static gboolean
cdk_quartz_display_supports_selection_notification (CdkDisplay *display)
{
  g_return_val_if_fail (GDK_IS_DISPLAY (display), FALSE);
  /* X11-only. */
  return FALSE;
}

static gboolean
cdk_quartz_display_request_selection_notification (CdkDisplay *display,
                                                   CdkAtom     selection)
{
  /* X11-only. */
  return FALSE;
}

static gboolean
cdk_quartz_display_supports_clipboard_persistence (CdkDisplay *display)
{
  /* X11-only */
  return FALSE;
}

static gboolean
cdk_quartz_display_supports_shapes (CdkDisplay *display)
{
  /* Not needed, nothing ever calls this.*/
  return FALSE;
}

static gboolean
cdk_quartz_display_supports_input_shapes (CdkDisplay *display)
{
  /* Not needed, nothign ever calls this. */
  return FALSE;
}

static void
cdk_quartz_display_store_clipboard (CdkDisplay    *display,
                                    CdkWindow     *clipboard_window,
                                    guint32        time_,
                                    const CdkAtom *targets,
                                    gint           n_targets)
{
  /* MacOS persists pasteboard items automatically, no application
   * action is required.
   */
}


static gboolean
cdk_quartz_display_supports_composite (CdkDisplay *display)
{
  /* X11-only. */
  return FALSE;
}

static gulong
cdk_quartz_display_get_next_serial (CdkDisplay *display)
{
  /* X11-only. */
  return 0;
}

static void
cdk_quartz_display_notify_startup_complete (CdkDisplay  *display,
                                            const gchar *startup_id)
{
  /* This should call finishLaunching, but doing so causes Quartz to throw
   * "_createMenuRef called with existing principal MenuRef already"
   * " associated with menu".
  [NSApp finishLaunching];
  */
}

static void
cdk_quartz_display_push_error_trap (CdkDisplay *display)
{
  /* X11-only. */
}

static gint
cdk_quartz_display_pop_error_trap (CdkDisplay *display, gboolean ignore)
{
  /* X11 only. */
  return 0;
}

/* The display monitor list comprises all of the CGDisplays connected
   to the system, some of which may not be drawable either because
   they're asleep or are mirroring another monitor. The NSScreens
   array contains only the monitors that are currently drawable and we
   use the index of the screens array placing CdkNSViews, so we'll use
   the same for determining the number of monitors and indexing them.
 */

int
get_active_displays (CGDirectDisplayID **displays)
{
  unsigned int n_displays = 0;

  CGGetActiveDisplayList (0, NULL, &n_displays);
  if (displays)
    {
      *displays = g_new0 (CGDirectDisplayID, n_displays);
      CGGetActiveDisplayList (n_displays, *displays, &n_displays);
    }

  return n_displays;
}

static inline CdkRectangle
cgrect_to_cdkrect (CGRect cgrect)
{
  CdkRectangle cdkrect = {(int)trunc (cgrect.origin.x),
                          (int)trunc (cgrect.origin.y),
                          (int)trunc (cgrect.size.width),
                          (int)trunc (cgrect.size.height)};
  return cdkrect;
}

static void
configure_monitor (CdkMonitor       *monitor,
                   CdkQuartzDisplay *display)
{
  CdkQuartzMonitor *quartz_monitor = GDK_QUARTZ_MONITOR (monitor);
  CGSize disp_size = CGDisplayScreenSize (quartz_monitor->id);
  gint width = (int)trunc (disp_size.width);
  gint height = (int)trunc (disp_size.height);
  CGRect disp_bounds = CGDisplayBounds (quartz_monitor->id);
  CGRect main_bounds = CGDisplayBounds (CGMainDisplayID());
  /* Change origin to Cdk coordinates. */
  disp_bounds.origin.x = disp_bounds.origin.x + display->geometry.origin.x;
  disp_bounds.origin.y =
    display->geometry.origin.y - main_bounds.size.height + disp_bounds.origin.y;
  CdkRectangle disp_geometry = cgrect_to_cdkrect (disp_bounds);
  CGDisplayModeRef mode = CGDisplayCopyDisplayMode (quartz_monitor->id);
  gint refresh_rate = (int)trunc (CGDisplayModeGetRefreshRate (mode));

  monitor->width_mm = width;
  monitor->height_mm = height;
  monitor->geometry = disp_geometry;
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1080
  if (mode && cdk_quartz_osx_version () >= GDK_OSX_MOUNTAIN_LION)
  {
    monitor->scale_factor = CGDisplayModeGetPixelWidth (mode) / CGDisplayModeGetWidth (mode);
    CGDisplayModeRelease (mode);
  }
  else
#endif
    monitor->scale_factor = 1;
  monitor->refresh_rate = refresh_rate;
  monitor->subpixel_layout = GDK_SUBPIXEL_LAYOUT_UNKNOWN;
}

static void
display_rect (CdkQuartzDisplay *display)
{
  uint32_t disp, n_displays = 0;
  float min_x = 0.0, max_x = 0.0, min_y = 0.0, max_y = 0.0;
  float min_x_mm = 0.0, max_x_mm = 0.0, min_y_mm = 0.0, max_y_mm = 0.0;
  float main_height;
  CGDirectDisplayID *displays;

  n_displays = get_active_displays (&displays);
  for (disp = 0; disp < n_displays; ++disp)
    {
      CGRect bounds = CGDisplayBounds (displays[disp]);
      CGSize disp_size = CGDisplayScreenSize (displays[disp]);
      float x_scale = disp_size.width / bounds.size.width;
      float y_scale = disp_size.height / bounds.size.height;
      if (disp == 0)
        main_height = bounds.size.height;
      min_x = MIN (min_x, bounds.origin.x);
      min_y = MIN (min_y, bounds.origin.y);

      max_x = MAX (max_x, bounds.origin.x + bounds.size.width);
      max_y = MAX (max_y, bounds.origin.y + bounds.size.height);
      min_x_mm = MIN (min_x_mm, bounds.origin.x / x_scale);
      min_y_mm = MIN (min_y_mm, main_height - (bounds.size.height + bounds.origin.y) / y_scale);
      max_x_mm = MAX (max_x_mm, (bounds.origin.x + bounds.size.width) / x_scale);
      max_y_mm = MAX (max_y_mm, (bounds.origin.y + bounds.size.height) / y_scale);

    }
  g_free (displays);
  /* Adjusts the origin to AppKit coordinates. */
  display->geometry = NSMakeRect (-min_x, main_height - min_y,
                                  max_x - min_x, max_y - min_y);
  display->size = NSMakeSize (max_x_mm - min_x_mm, max_y_mm - min_y_mm);
}

static gboolean
same_monitor (gconstpointer a, gconstpointer b)
{
  CdkQuartzMonitor *mon_a = GDK_QUARTZ_MONITOR (a);
  CGDirectDisplayID disp_id = (CGDirectDisplayID)GPOINTER_TO_INT (b);
  if (!mon_a)
    return FALSE;
  return mon_a->id == disp_id;
}

static void
display_reconfiguration_callback (CGDirectDisplayID            cg_display,
                                  CGDisplayChangeSummaryFlags  flags,
                                  void                        *data)
{
  CdkQuartzDisplay *display = data;

  /* Ignore the begin configuration signal. */
  if (flags & kCGDisplayBeginConfigurationFlag)
      return;

  if (flags & (kCGDisplayMovedFlag | kCGDisplayAddFlag | kCGDisplayEnabledFlag |
               kCGDisplaySetMainFlag | kCGDisplayMirrorFlag |
               kCGDisplayUnMirrorFlag))
    {
      CdkQuartzMonitor *monitor = NULL;
      guint index;

      if (!g_ptr_array_find_with_equal_func (display->monitors,
                                             GINT_TO_POINTER (cg_display),
                                             same_monitor,
                                             &index))
        {
          monitor = g_object_new (GDK_TYPE_QUARTZ_MONITOR,
                                  "display", display, NULL);
          monitor->id = cg_display;
          g_ptr_array_add (display->monitors, monitor);
          display_rect (display);
          configure_monitor (GDK_MONITOR (monitor), display);
          cdk_display_monitor_added (GDK_DISPLAY (display),
                                     GDK_MONITOR (monitor));
        }
      else
        {
          monitor = g_ptr_array_index (display->monitors, index);
          display_rect (display);
          configure_monitor (GDK_MONITOR (monitor), display);
        }
    }
  else if (flags & (kCGDisplayRemoveFlag |  kCGDisplayDisabledFlag))
    {
      guint index;

      if (g_ptr_array_find_with_equal_func (display->monitors,
                                            GINT_TO_POINTER (cg_display),
                                            same_monitor,
                                            &index))
        {
          CdkQuartzMonitor *monitor = g_ptr_array_index (display->monitors,
                                                         index);
          cdk_display_monitor_removed (GDK_DISPLAY (display),
                                       GDK_MONITOR (monitor));
          g_ptr_array_remove_fast (display->monitors, monitor);
        }
    }

  g_signal_emit (display, MONITORS_CHANGED, 0);
}


static int
cdk_quartz_display_get_n_monitors (CdkDisplay *display)
{
  CdkQuartzDisplay *quartz_display = GDK_QUARTZ_DISPLAY (display);
  return quartz_display->monitors->len;
}

static CdkMonitor *
cdk_quartz_display_get_monitor (CdkDisplay *display,
                                int         monitor_num)
{
  CdkQuartzDisplay *quartz_display = GDK_QUARTZ_DISPLAY (display);
  int n_displays = cdk_quartz_display_get_n_monitors (display);

  if (monitor_num >= 0 && monitor_num < n_displays)
    return g_ptr_array_index (quartz_display->monitors, monitor_num);

  return NULL;
}

static CdkMonitor *
cdk_quartz_display_get_primary_monitor (CdkDisplay *display)
{
  CdkQuartzDisplay *quartz_display = GDK_QUARTZ_DISPLAY (display);
  CGDirectDisplayID primary_id = CGMainDisplayID ();
  CdkMonitor *monitor = NULL;
  guint index;

  if (g_ptr_array_find_with_equal_func (quartz_display->monitors,
                                        GINT_TO_POINTER (primary_id),
                                        same_monitor, &index))
    monitor = g_ptr_array_index (quartz_display->monitors, index);

  return monitor;
}

static CdkMonitor *
cdk_quartz_display_get_monitor_at_window (CdkDisplay *display,
                                          CdkWindow *window)
{
  CdkWindowImplQuartz *impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);
  NSWindow *nswindow = impl->toplevel;
  NSScreen *screen = [nswindow screen];
  CdkMonitor *monitor = NULL;
  if (screen)
  {
    CdkQuartzDisplay *quartz_display = GDK_QUARTZ_DISPLAY (display);
    guint index;
    CGDirectDisplayID disp_id =
      [[[screen deviceDescription]
        objectForKey: @"NSScreenNumber"] unsignedIntValue];
    if (g_ptr_array_find_with_equal_func (quartz_display->monitors,
                                          GINT_TO_POINTER (disp_id),
                                          same_monitor, &index))
      monitor = g_ptr_array_index (quartz_display->monitors, index);
  }
  if (!monitor)
    {
      CdkRectangle rect = cgrect_to_cdkrect (NSRectToCGRect ([nswindow frame]));
      monitor = cdk_display_get_monitor_at_point (display,
                                                 rect.x + rect.width/2,
                                                 rect.y + rect.height /2);
    }
  return monitor;
}

G_DEFINE_TYPE (CdkQuartzDisplay, cdk_quartz_display, GDK_TYPE_DISPLAY)

static void
cdk_quartz_display_init (CdkQuartzDisplay *display)
{
  uint32_t n_displays = 0, disp;
  CGDirectDisplayID *displays;

  display_rect(display); /* Initialize the overall display coordinates. */
  n_displays = get_active_displays (&displays);
  display->monitors = g_ptr_array_new_full (n_displays, g_object_unref);
  for (disp = 0; disp < n_displays; ++disp)
    {
      CdkQuartzMonitor *monitor = g_object_new (GDK_TYPE_QUARTZ_MONITOR,
                                                       "display", display, NULL);
      monitor->id = displays[disp];
      g_ptr_array_add (display->monitors, monitor);
      configure_monitor (GDK_MONITOR (monitor), display);
    }
  g_free (displays);
  CGDisplayRegisterReconfigurationCallback (display_reconfiguration_callback,
                                            display);
  /* So that monitors changed will keep display->geometry syncronized. */
  g_signal_emit (display, MONITORS_CHANGED, 0);
}

static void
cdk_quartz_display_dispose (GObject *object)
{
  CdkQuartzDisplay *quartz_display = GDK_QUARTZ_DISPLAY (object);

  g_ptr_array_free (quartz_display->monitors, TRUE);
  CGDisplayRemoveReconfigurationCallback (display_reconfiguration_callback,
                                          quartz_display);

  G_OBJECT_CLASS (cdk_quartz_display_parent_class)->dispose (object);
}

static void
cdk_quartz_display_finalize (GObject *object)
{
  G_OBJECT_CLASS (cdk_quartz_display_parent_class)->finalize (object);
}

static void
cdk_quartz_display_class_init (CdkQuartzDisplayClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  CdkDisplayClass *display_class = GDK_DISPLAY_CLASS (class);

  object_class->finalize = cdk_quartz_display_finalize;
  object_class->dispose = cdk_quartz_display_dispose;

  display_class->window_type = GDK_TYPE_QUARTZ_WINDOW;

  display_class->get_name = cdk_quartz_display_get_name;
  display_class->get_default_screen = cdk_quartz_display_get_default_screen;
  display_class->beep = cdk_quartz_display_beep;
  display_class->sync = cdk_quartz_display_sync;
  display_class->flush = cdk_quartz_display_flush;
  display_class->has_pending = _cdk_quartz_display_has_pending;
  display_class->queue_events = _cdk_quartz_display_queue_events;
  display_class->get_default_group = cdk_quartz_display_get_default_group;
  display_class->supports_selection_notification = cdk_quartz_display_supports_selection_notification;
  display_class->request_selection_notification = cdk_quartz_display_request_selection_notification;

  display_class->supports_shapes = cdk_quartz_display_supports_shapes;
  display_class->supports_input_shapes = cdk_quartz_display_supports_input_shapes;
  display_class->supports_composite = cdk_quartz_display_supports_composite;
  display_class->supports_cursor_alpha = _cdk_quartz_display_supports_cursor_alpha;
  display_class->supports_cursor_color = _cdk_quartz_display_supports_cursor_color;

  display_class->supports_clipboard_persistence = cdk_quartz_display_supports_clipboard_persistence;
  display_class->store_clipboard = cdk_quartz_display_store_clipboard;

  display_class->get_default_cursor_size = _cdk_quartz_display_get_default_cursor_size;
  display_class->get_maximal_cursor_size = _cdk_quartz_display_get_maximal_cursor_size;
  display_class->get_cursor_for_type = _cdk_quartz_display_get_cursor_for_type;
  display_class->get_cursor_for_name = _cdk_quartz_display_get_cursor_for_name;
  display_class->get_cursor_for_surface = _cdk_quartz_display_get_cursor_for_surface;

  /* display_class->get_app_launch_context = NULL; Has default. */
  display_class->before_process_all_updates = _cdk_quartz_display_before_process_all_updates;
  display_class->after_process_all_updates = _cdk_quartz_display_after_process_all_updates;
  display_class->get_next_serial = cdk_quartz_display_get_next_serial;
  display_class->notify_startup_complete = cdk_quartz_display_notify_startup_complete;
  display_class->event_data_copy = _cdk_quartz_display_event_data_copy;
  display_class->event_data_free = _cdk_quartz_display_event_data_free;
  display_class->create_window_impl = _cdk_quartz_display_create_window_impl;
  display_class->get_keymap = _cdk_quartz_display_get_keymap;
  display_class->push_error_trap = cdk_quartz_display_push_error_trap;
  display_class->pop_error_trap = cdk_quartz_display_pop_error_trap;

  display_class->get_selection_owner = _cdk_quartz_display_get_selection_owner;
  display_class->set_selection_owner = _cdk_quartz_display_set_selection_owner;
  display_class->send_selection_notify = NULL; /* Ignore. X11 stuff removed in master.  */
  display_class->get_selection_property = _cdk_quartz_display_get_selection_property;
  display_class->convert_selection = _cdk_quartz_display_convert_selection;
  display_class->text_property_to_utf8_list = _cdk_quartz_display_text_property_to_utf8_list;
  display_class->utf8_to_string_target = _cdk_quartz_display_utf8_to_string_target;

/* display_class->get_default_seat; The parent class default works fine. */

  display_class->get_n_monitors = cdk_quartz_display_get_n_monitors;
  display_class->get_monitor = cdk_quartz_display_get_monitor;
  display_class->get_primary_monitor = cdk_quartz_display_get_primary_monitor;
  display_class->get_monitor_at_window = cdk_quartz_display_get_monitor_at_window;
  display_class->make_gl_context_current = cdk_quartz_display_make_gl_context_current;

  /**
   * CdkQuartzDisplay::monitors-changed:
   * @display: The object on which the signal is emitted
   *
   * The ::monitors-changed signal is emitted whenever the arrangement
   * of the monitors changes, either because of the addition or
   * removal of a monitor or because of some other configuration
   * change in System Preferences>Displays including a resolution
   * change or a position change. Note that enabling or disabling
   * mirroring will result in the addition or removal of the mirror
   * monitor(s).
   */
  MONITORS_CHANGED =
    g_signal_new (g_intern_static_string ("monitors-changed"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                  0, NULL, NULL, NULL,
                  G_TYPE_NONE, 0, NULL);

  ProcessSerialNumber psn = { 0, kCurrentProcess };

  /* Make the current process a foreground application, i.e. an app
   * with a user interface, in case we're not running from a .app bundle
   */
  TransformProcessType (&psn, kProcessTransformToForegroundApplication);
}
