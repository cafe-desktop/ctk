/* cdkscreen-quartz.c
 *
 * Copyright (C) 2005 Imendio AB
 * Copyright (C) 2009,2010  Kristian Rietveld  <kris@ctk.org>
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
/* CdkScreen is deprecated, but we need to support it still so silence the warnings.*/
#define GDK_DISABLE_DEPRECATION_WARNINGS 1
#include <cdk/cdk.h>

#include "cdkprivate-quartz.h"
#include "cdkdisplay-quartz.h"
#include "cdkmonitor-quartz.h"
 

/* A couple of notes about this file are in order.  In GDK, a
 * CdkScreen can contain multiple monitors.  A CdkScreen has an
 * associated root window, in which the monitors are placed.  The
 * root window "spans" all monitors.  The origin is at the top-left
 * corner of the root window.
 *
 * Cocoa works differently.  The system has a "screen" (NSScreen) for
 * each monitor that is connected (note the conflicting definitions
 * of screen).  The screen containing the menu bar is screen 0 and the
 * bottom-left corner of this screen is the origin of the "monitor
 * coordinate space".  All other screens are positioned according to this
 * origin.  If the menu bar is on a secondary screen (for example on
 * a monitor hooked up to a laptop), then this screen is screen 0 and
 * other monitors will be positioned according to the "secondary screen".
 * The main screen is the monitor that shows the window that is currently
 * active (has focus), the position of the menu bar does not have influence
 * on this!
 *
 * Upon start up and changes in the layout of screens, we calculate the
 * size of the CdkScreen root window that is needed to be able to place
 * all monitors in the root window.  Once that size is known, we iterate
 * over the monitors and translate their Cocoa position to a position
 * in the root window of the CdkScreen.  This happens below in the
 * function cdk_quartz_screen_calculate_layout().
 *
 * A Cocoa coordinate is always relative to the origin of the monitor
 * coordinate space.  Such coordinates are mapped to their respective
 * position in the CdkScreen root window (_cdk_quartz_window_xy_to_cdk_xy)
 * and vice versa (_cdk_quartz_window_cdk_xy_to_xy).  Both functions can
 * be found in cdkwindow-quartz.c.  Note that Cocoa coordinates can have
 * negative values (in case a monitor is located left or below of screen 0),
 * but GDK coordinates can *not*!
 */

static void  cdk_quartz_screen_dispose          (GObject          *object);
static void  cdk_quartz_screen_finalize         (GObject          *object);
static void  cdk_quartz_screen_calculate_layout (CdkQuartzScreen  *screen,
                                                 CdkQuartzDisplay *display);
static void  cdk_quartz_screen_reconfigure      (CdkQuartzDisplay *display,
                                                 CdkQuartzScreen  *screen);

static const double dpi = 72.0;

G_DEFINE_TYPE (CdkQuartzScreen, cdk_quartz_screen, GDK_TYPE_SCREEN);

static void
cdk_quartz_screen_init (CdkQuartzScreen *quartz_screen)
{
  CdkScreen *screen = GDK_SCREEN (quartz_screen);
  /* Screen resolution is used exclusively to pass to Pango for font
   * scaling. There's a long discussion in
   * https://bugzilla.gnome.org/show_bug.cgi?id=787867 exploring how
   * screen resolution and pangocairo-coretext interact. The summary
   * is that MacOS takes care of scaling fonts for Retina screens and
   * that while the Apple Documentation goes on about "points" they're
   * CSS points (96/in), not typeography points (72/in) and
   * pangocairo-coretext needs to default to that scaling factor.
   */

  g_signal_connect (_cdk_display, "monitors-changed",
                    G_CALLBACK (cdk_quartz_screen_reconfigure), quartz_screen);
  /* The first monitors-changed should have fired already. */
  _cdk_screen_set_resolution (screen, dpi);
  cdk_quartz_screen_calculate_layout (quartz_screen, NULL);
}

static void
cdk_quartz_screen_dispose (GObject *object)
{
  CdkQuartzScreen *screen = GDK_QUARTZ_SCREEN (object);

  if (screen->screen_changed_id)
    {
      g_source_remove (screen->screen_changed_id);
      screen->screen_changed_id = 0;
    }

  G_OBJECT_CLASS (cdk_quartz_screen_parent_class)->dispose (object);
}

static void
cdk_quartz_screen_finalize (GObject *object)
{
  G_OBJECT_CLASS (cdk_quartz_screen_parent_class)->finalize (object);
}

/* Protocol to build cleanly for OSX < 10.7 */
@protocol ScaleFactor
- (CGFloat) backingScaleFactor;
@end

static void
cdk_quartz_screen_calculate_layout (CdkQuartzScreen *screen,
                                    CdkQuartzDisplay *display)
{
  if (!display)
    display = GDK_QUARTZ_DISPLAY (cdk_screen_get_display (GDK_SCREEN (screen)));

/* Display geometry is the origin and size in AppKit coordinates. AppKit computes */
  screen->width = (int)trunc (display->geometry.size.width);
  screen->height = (int)trunc (display->geometry.size.height);
  screen->orig_x = -(int)trunc (display->geometry.origin.x);
  screen->orig_y = (int)trunc (display->geometry.origin.y);
  screen->mm_width = (int)trunc (display->size.width);
  screen->mm_height = (int)trunc (display->size.height);

 }

void
_cdk_quartz_screen_update_window_sizes (CdkScreen *screen)
{
  GList *windows, *list;

  /* The size of the root window is so that it can contain all
   * monitors attached to this machine.  The monitors are laid out
   * within this root window.  We calculate the size of the root window
   * and the positions of the different monitors in cdkscreen-quartz.c.
   *
   * This data is updated when the monitor configuration is changed.
   */

  /* FIXME: At some point, fetch the root window from CdkScreen.  But
   * on OS X will we only have a single root window anyway.
   */
  _cdk_root->x = 0;
  _cdk_root->y = 0;
  _cdk_root->abs_x = 0;
  _cdk_root->abs_y = 0;
  _cdk_root->width = cdk_screen_get_width (screen);
  _cdk_root->height = cdk_screen_get_height (screen);

  windows = cdk_screen_get_toplevel_windows (screen);

  for (list = windows; list; list = list->next)
    {
      if (GDK_WINDOW_TYPE(list->data) == GDK_WINDOW_OFFSCREEN)
        continue;
      _cdk_quartz_window_update_position (list->data);
    }

  g_list_free (windows);
}

static void
cdk_quartz_screen_reconfigure (CdkQuartzDisplay *display, CdkQuartzScreen *screen)
{
  int width, height;

  width = cdk_screen_get_width (GDK_SCREEN (screen));
  height = cdk_screen_get_height (GDK_SCREEN (screen));

  cdk_quartz_screen_calculate_layout (screen, display);

  _cdk_quartz_screen_update_window_sizes (GDK_SCREEN (screen));

  g_signal_emit_by_name (screen, "monitors-changed");

  if (width != cdk_screen_get_width (GDK_SCREEN (screen))
      || height != cdk_screen_get_height (GDK_SCREEN (screen)))
    g_signal_emit_by_name (screen, "size-changed");
}

static CdkDisplay *
cdk_quartz_screen_get_display (CdkScreen *screen)
{
  return _cdk_display;
}

static CdkWindow *
cdk_quartz_screen_get_root_window (CdkScreen *screen)
{
  return _cdk_root;
}

static gint
cdk_quartz_screen_get_number (CdkScreen *screen)
{
  return 0;
}

static gint
cdk_quartz_screen_get_width (CdkScreen *screen)
{
  return GDK_QUARTZ_SCREEN (screen)->width;
}

static gint
cdk_quartz_screen_get_height (CdkScreen *screen)
{
  return GDK_QUARTZ_SCREEN (screen)->height;
}

static gchar *
cdk_quartz_screen_make_display_name (CdkScreen *screen)
{
  return g_strdup (cdk_display_get_name (_cdk_display));
}

static CdkWindow *
cdk_quartz_screen_get_active_window (CdkScreen *screen)
{
  return NULL;
}

static GList *
cdk_quartz_screen_get_window_stack (CdkScreen *screen)
{
  return NULL;
}

static gboolean
cdk_quartz_screen_is_composited (CdkScreen *screen)
{
  return TRUE;
}

static gint
cdk_quartz_screen_get_width_mm (CdkScreen *screen)
{
  return GDK_QUARTZ_SCREEN (screen)->mm_width;
}

static gint
cdk_quartz_screen_get_height_mm (CdkScreen *screen)
{
  return GDK_QUARTZ_SCREEN (screen)->mm_height;
}

static void
cdk_quartz_screen_class_init (CdkQuartzScreenClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CdkScreenClass *screen_class = GDK_SCREEN_CLASS (klass);

  object_class->dispose = cdk_quartz_screen_dispose;
  object_class->finalize = cdk_quartz_screen_finalize;

  screen_class->get_display = cdk_quartz_screen_get_display;
  screen_class->get_width = cdk_quartz_screen_get_width;
  screen_class->get_height = cdk_quartz_screen_get_height;
  screen_class->get_width_mm = cdk_quartz_screen_get_width_mm;
  screen_class->get_height_mm = cdk_quartz_screen_get_height_mm;
  screen_class->get_number = cdk_quartz_screen_get_number;
  screen_class->get_root_window = cdk_quartz_screen_get_root_window;
  screen_class->is_composited = cdk_quartz_screen_is_composited;
  screen_class->make_display_name = cdk_quartz_screen_make_display_name;
  screen_class->get_active_window = cdk_quartz_screen_get_active_window;
  screen_class->get_window_stack = cdk_quartz_screen_get_window_stack;
  screen_class->broadcast_client_message = _cdk_quartz_screen_broadcast_client_message;
  screen_class->get_setting = _cdk_quartz_screen_get_setting;
  screen_class->get_rgba_visual = _cdk_quartz_screen_get_rgba_visual;
  screen_class->get_system_visual = _cdk_quartz_screen_get_system_visual;
  screen_class->visual_get_best_depth = _cdk_quartz_screen_visual_get_best_depth;
  screen_class->visual_get_best_type = _cdk_quartz_screen_visual_get_best_type;
  screen_class->visual_get_best = _cdk_quartz_screen_visual_get_best;
  screen_class->visual_get_best_with_depth = _cdk_quartz_screen_visual_get_best_with_depth;
  screen_class->visual_get_best_with_type = _cdk_quartz_screen_visual_get_best_with_type;
  screen_class->visual_get_best_with_both = _cdk_quartz_screen_visual_get_best_with_both;
  screen_class->query_depths = _cdk_quartz_screen_query_depths;
  screen_class->query_visual_types = _cdk_quartz_screen_query_visual_types;
  screen_class->list_visuals = _cdk_quartz_screen_list_visuals;
}
