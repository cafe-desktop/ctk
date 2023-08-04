/*
 * Copyright © 2010 Intel Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <netinet/in.h>
#include <unistd.h>

#include "cdk.h"
#include "cdkwayland.h"

#include "cdkwindow.h"
#include "cdkwindowimpl.h"
#include "cdkdisplay-wayland.h"
#include "cdkglcontext-wayland.h"
#include "cdkframeclockprivate.h"
#include "cdkprivate-wayland.h"
#include "cdkprofilerprivate.h"
#include "cdkinternals.h"
#include "cdkdeviceprivate.h"
#include "cdkprivate-wayland.h"
#include "xdg-shell-unstable-v6-client-protocol.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

enum {
  COMMITTED,

  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

#define WINDOW_IS_TOPLEVEL_OR_FOREIGN(window) \
  (CDK_WINDOW_TYPE (window) != CDK_WINDOW_CHILD &&   \
   CDK_WINDOW_TYPE (window) != CDK_WINDOW_OFFSCREEN)

#define WINDOW_IS_TOPLEVEL(window)                   \
  (CDK_WINDOW_TYPE (window) != CDK_WINDOW_CHILD &&   \
   CDK_WINDOW_TYPE (window) != CDK_WINDOW_FOREIGN && \
   CDK_WINDOW_TYPE (window) != CDK_WINDOW_OFFSCREEN)

#define MAX_WL_BUFFER_SIZE (4083) /* 4096 minus header, string argument length and NUL byte */

typedef struct _CdkWaylandWindow CdkWaylandWindow;
typedef struct _CdkWaylandWindowClass CdkWaylandWindowClass;

struct _CdkWaylandWindow
{
  CdkWindow parent;
};

struct _CdkWaylandWindowClass
{
  CdkWindowClass parent_class;
};

G_DEFINE_TYPE (CdkWaylandWindow, cdk_wayland_window, CDK_TYPE_WINDOW)

static void
cdk_wayland_window_class_init (CdkWaylandWindowClass *wayland_window_class)
{
}

static void
cdk_wayland_window_init (CdkWaylandWindow *wayland_window)
{
}

#define CDK_TYPE_WINDOW_IMPL_WAYLAND              (_cdk_window_impl_wayland_get_type ())
#define CDK_WINDOW_IMPL_WAYLAND(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), CDK_TYPE_WINDOW_IMPL_WAYLAND, CdkWindowImplWayland))
#define CDK_WINDOW_IMPL_WAYLAND_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CDK_TYPE_WINDOW_IMPL_WAYLAND, CdkWindowImplWaylandClass))
#define CDK_IS_WINDOW_IMPL_WAYLAND(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), CDK_TYPE_WINDOW_IMPL_WAYLAND))
#define CDK_IS_WINDOW_IMPL_WAYLAND_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CDK_TYPE_WINDOW_IMPL_WAYLAND))
#define CDK_WINDOW_IMPL_WAYLAND_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CDK_TYPE_WINDOW_IMPL_WAYLAND, CdkWindowImplWaylandClass))

typedef struct _CdkWindowImplWayland CdkWindowImplWayland;
typedef struct _CdkWindowImplWaylandClass CdkWindowImplWaylandClass;

typedef enum _PositionMethod
{
  POSITION_METHOD_NONE,
  POSITION_METHOD_MOVE_RESIZE,
  POSITION_METHOD_MOVE_TO_RECT
} PositionMethod;

typedef struct _ExportedClosure
{
  CdkWaylandWindowExported callback;
  gpointer user_data;
  GDestroyNotify destroy_func;
} ExportedClosure;

struct _CdkWindowImplWayland
{
  CdkWindowImpl parent_instance;

  CdkWindow *wrapper;

  struct {
    /* The wl_outputs that this window currently touches */
    GSList               *outputs;

    struct wl_surface    *wl_surface;

    struct xdg_surface *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;
    struct xdg_popup *xdg_popup;

    /* Legacy xdg-shell unstable v6 fallback support */
    struct zxdg_surface_v6 *zxdg_surface_v6;
    struct zxdg_toplevel_v6 *zxdg_toplevel_v6;
    struct zxdg_popup_v6 *zxdg_popup_v6;

    struct ctk_surface1  *ctk_surface;
    struct wl_subsurface *wl_subsurface;
    struct wl_egl_window *egl_window;
    struct wl_egl_window *dummy_egl_window;
    struct zxdg_exported_v1 *xdg_exported;
    struct org_kde_kwin_server_decoration *server_decoration;
  } display_server;

  EGLSurface egl_surface;
  EGLSurface dummy_egl_surface;

  unsigned int initial_configure_received : 1;
  unsigned int configuring_popup : 1;
  unsigned int mapped : 1;
  unsigned int use_custom_surface : 1;
  unsigned int pending_buffer_attached : 1;
  unsigned int pending_commit : 1;
  unsigned int awaiting_frame : 1;
  unsigned int using_csd : 1;
  CdkWindowTypeHint hint;
  CdkWindow *transient_for;
  CdkWindow *popup_parent;
  PositionMethod position_method;

  cairo_surface_t *staging_cairo_surface;
  cairo_surface_t *committed_cairo_surface;
  cairo_surface_t *backfill_cairo_surface;

  int pending_buffer_offset_x;
  int pending_buffer_offset_y;

  int subsurface_x;
  int subsurface_y;

  gchar *title;

  struct {
    gboolean was_set;

    gchar *application_id;
    gchar *app_menu_path;
    gchar *menubar_path;
    gchar *window_object_path;
    gchar *application_object_path;
    gchar *unique_bus_name;
  } application;

  CdkGeometry geometry_hints;
  CdkWindowHints geometry_mask;

  CdkSeat *grab_input_seat;

  gint64 pending_frame_counter;
  guint32 scale;

  int margin_left;
  int margin_right;
  int margin_top;
  int margin_bottom;
  gboolean margin_dirty;

  int initial_fullscreen_monitor;

  cairo_region_t *opaque_region;
  gboolean opaque_region_dirty;

  cairo_region_t *input_region;
  gboolean input_region_dirty;

  cairo_region_t *staged_updates_region;

  int saved_width;
  int saved_height;
  gboolean saved_size_changed;

  int unconfigured_width;
  int unconfigured_height;

  int fixed_size_width;
  int fixed_size_height;

  gulong parent_surface_committed_handler;

  struct {
    CdkRectangle rect;
    CdkGravity rect_anchor;
    CdkGravity window_anchor;
    CdkAnchorHints anchor_hints;
    gint rect_anchor_dx;
    gint rect_anchor_dy;
  } pending_move_to_rect;

  struct {
    int width;
    int height;
    CdkWindowState state;
  } pending;

  struct {
    char *handle;
    int export_count;
    GList *closures;
    guint idle_source_id;
  } exported;

  struct zxdg_imported_v1 *imported_transient_for;
  GHashTable *shortcuts_inhibitors;
};

struct _CdkWindowImplWaylandClass
{
  CdkWindowImplClass parent_class;
};

static void cdk_wayland_window_maybe_configure (CdkWindow *window,
                                                int        width,
                                                int        height,
                                                int        scale);

static void maybe_set_ctk_surface_dbus_properties (CdkWindow *window);
static void maybe_set_ctk_surface_modal (CdkWindow *window);

static void cdk_window_request_transient_parent_commit (CdkWindow *window);

static void cdk_wayland_window_sync_margin (CdkWindow *window);
static void cdk_wayland_window_sync_input_region (CdkWindow *window);
static void cdk_wayland_window_sync_opaque_region (CdkWindow *window);

static void unset_transient_for_exported (CdkWindow *window);

static void calculate_moved_to_rect_result (CdkWindow    *window,
                                            int           x,
                                            int           y,
                                            int           width,
                                            int           height,
                                            CdkRectangle *flipped_rect,
                                            CdkRectangle *final_rect,
                                            gboolean     *flipped_x,
                                            gboolean     *flipped_y);

static gboolean cdk_wayland_window_is_exported (CdkWindow *window);
static void cdk_wayland_window_unexport (CdkWindow *window);
static void cdk_wayland_window_announce_decoration_mode (CdkWindow *window);

static gboolean should_map_as_subsurface (CdkWindow *window);
static gboolean should_map_as_popup (CdkWindow *window);

GType _cdk_window_impl_wayland_get_type (void);

G_DEFINE_TYPE (CdkWindowImplWayland, _cdk_window_impl_wayland, CDK_TYPE_WINDOW_IMPL)

static void
_cdk_window_impl_wayland_init (CdkWindowImplWayland *impl)
{
  impl->scale = 1;
  impl->initial_fullscreen_monitor = -1;
  impl->saved_width = -1;
  impl->saved_height = -1;
}

static void
_cdk_wayland_screen_add_orphan_dialog (CdkWindow *window)
{
  CdkWaylandDisplay *display_wayland =
    CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));

  if (!g_list_find (display_wayland->orphan_dialogs, window))
    display_wayland->orphan_dialogs =
      g_list_prepend (display_wayland->orphan_dialogs, window);
}

static void
drop_cairo_surfaces (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  g_clear_pointer (&impl->staging_cairo_surface, cairo_surface_destroy);
  g_clear_pointer (&impl->backfill_cairo_surface, cairo_surface_destroy);

  /* We nullify this so if a buffer release comes in later, we won't
   * try to reuse that buffer since it's no longer suitable
   */
  impl->committed_cairo_surface = NULL;
}

static int
calculate_width_without_margin (CdkWindow *window,
                                int        width)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  return width - (impl->margin_left + impl->margin_right);
}

static int
calculate_height_without_margin (CdkWindow *window,
                                 int        height)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  return height - (impl->margin_top + impl->margin_bottom);
}

static int
calculate_width_with_margin (CdkWindow *window,
                             int        width)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  return width + impl->margin_left + impl->margin_right;
}

static int
calculate_height_with_margin (CdkWindow *window,
                              int        height)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  return height + impl->margin_top + impl->margin_bottom;
}

static void
_cdk_wayland_window_save_size (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  if (window->state & (CDK_WINDOW_STATE_FULLSCREEN |
                       CDK_WINDOW_STATE_MAXIMIZED |
                       CDK_WINDOW_STATE_TILED))
    return;

  impl->saved_width = calculate_width_without_margin (window, window->width);
  impl->saved_height = calculate_height_without_margin (window, window->height);
}

static void
_cdk_wayland_window_clear_saved_size (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  if (window->state & (CDK_WINDOW_STATE_FULLSCREEN |
                       CDK_WINDOW_STATE_MAXIMIZED |
                       CDK_WINDOW_STATE_TILED))
    return;

  impl->saved_width = -1;
  impl->saved_height = -1;
}

/*
 * cdk_wayland_window_update_size:
 * @drawable: a #CdkDrawableImplWayland.
 *
 * Updates the state of the drawable (in particular the drawable's
 * cairo surface) when its size has changed.
 */
static void
cdk_wayland_window_update_size (CdkWindow *window,
                                int32_t    width,
                                int32_t    height,
                                int        scale)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  CdkRectangle area;
  cairo_region_t *region;

  if ((window->width == width) &&
      (window->height == height) &&
      (impl->scale == scale))
    return;

  drop_cairo_surfaces (window);

  window->width = width;
  window->height = height;
  impl->scale = scale;

  if (impl->display_server.egl_window)
    wl_egl_window_resize (impl->display_server.egl_window, width * scale, height * scale, 0, 0);
  if (impl->display_server.wl_surface)
    wl_surface_set_buffer_scale (impl->display_server.wl_surface, scale);

  area.x = 0;
  area.y = 0;
  area.width = window->width;
  area.height = window->height;

  region = cairo_region_create_rectangle (&area);
  _cdk_window_invalidate_for_expose (window, region);
  cairo_region_destroy (region);
}

CdkWindow *
_cdk_wayland_screen_create_root_window (CdkScreen *screen,
                                        int        width,
                                        int        height)
{
  CdkWaylandDisplay *display_wayland =
    CDK_WAYLAND_DISPLAY (cdk_screen_get_display (screen));
  CdkWindow *window;
  CdkWindowImplWayland *impl;

  window = _cdk_display_create_window (CDK_DISPLAY (display_wayland));
  window->impl = g_object_new (CDK_TYPE_WINDOW_IMPL_WAYLAND, NULL);
  window->impl_window = window;
  window->visual = cdk_screen_get_system_visual (screen);

  impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  impl->wrapper = CDK_WINDOW (window);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  if (display_wayland->compositor_version >= WL_SURFACE_HAS_BUFFER_SCALE &&
      cdk_screen_get_n_monitors (screen) > 0)
    impl->scale = cdk_screen_get_monitor_scale_factor (screen, 0);
G_GNUC_END_IGNORE_DEPRECATIONS

  impl->using_csd = TRUE;

  /* logical 1x1 fake buffer */
  impl->staging_cairo_surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                                            impl->scale,
                                                            impl->scale);

  cairo_surface_set_device_scale (impl->staging_cairo_surface, impl->scale, impl->scale);

  window->window_type = CDK_WINDOW_ROOT;
  window->depth = 32;

  window->x = 0;
  window->y = 0;
  window->abs_x = 0;
  window->abs_y = 0;
  window->width = width;
  window->height = height;
  window->viewable = TRUE;

  /* see init_randr_support() in cdkscreen-wayland.c */
  window->event_mask = CDK_STRUCTURE_MASK;

  return window;
}

static const gchar *
get_default_title (void)
{
  const char *title;

  title = g_get_application_name ();
  if (!title)
    title = g_get_prgname ();
  if (!title)
    title = "";

  return title;
}

static void
fill_presentation_time_from_frame_time (CdkFrameTimings *timings,
                                        guint32          frame_time)
{
  /* The timestamp in a wayland frame is a msec time value that in some
   * way reflects the time at which the server started drawing the frame.
   * This is not useful from our perspective.
   *
   * However, for the DRM backend of Weston, on reasonably recent
   * Linux, we know that the time is the
   * clock_gettime (CLOCK_MONOTONIC) value at the vblank, and that
   * backend starts drawing immediately after receiving the vblank
   * notification. If we detect this, and make the assumption that the
   * compositor will finish drawing before the next vblank, we can
   * then determine the presentation time as the frame time we
   * received plus one refresh interval.
   *
   * If a backend is using clock_gettime(CLOCK_MONOTONIC), but not
   * picking values right at the vblank, then the presentation times
   * we compute won't be accurate, but not really worse than then
   * the alternative of not providing presentation times at all.
   *
   * The complexity here is dealing with the fact that we receive
   * only the low 32 bits of the CLOCK_MONOTONIC value in milliseconds.
   */
  gint64 now_monotonic = g_get_monotonic_time ();
  gint64 now_monotonic_msec = now_monotonic / 1000;
  uint32_t now_monotonic_low = (uint32_t)now_monotonic_msec;

  if (frame_time - now_monotonic_low < 1000 ||
      frame_time - now_monotonic_low > (uint32_t)-1000)
    {
      /* Timestamp we received is within one second of the current time.
       */
      gint64 last_frame_time = now_monotonic + (gint64)1000 * (gint32)(frame_time - now_monotonic_low);
      if ((gint32)now_monotonic_low < 0 && (gint32)frame_time > 0)
        last_frame_time += (gint64)1000 * G_GINT64_CONSTANT(0x100000000);
      else if ((gint32)now_monotonic_low > 0 && (gint32)frame_time < 0)
        last_frame_time -= (gint64)1000 * G_GINT64_CONSTANT(0x100000000);

      timings->presentation_time = last_frame_time + timings->refresh_interval;
    }
}

static void
read_back_cairo_surface (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  cairo_t *cr;
  cairo_region_t *paint_region = NULL;

  if (!impl->backfill_cairo_surface)
    goto out;

  paint_region = cairo_region_copy (window->clip_region);
  cairo_region_subtract (paint_region, impl->staged_updates_region);

  if (cairo_region_is_empty (paint_region))
    goto out;

  cr = cairo_create (impl->staging_cairo_surface);
  cairo_set_source_surface (cr, impl->backfill_cairo_surface, 0, 0);
  cdk_cairo_region (cr, paint_region);
  cairo_clip (cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_paint (cr);
  cairo_destroy (cr);
  cairo_surface_flush (impl->staging_cairo_surface);

out:
  g_clear_pointer (&paint_region, cairo_region_destroy);
  g_clear_pointer (&impl->staged_updates_region, cairo_region_destroy);
  g_clear_pointer (&impl->backfill_cairo_surface, cairo_surface_destroy);
}

static void
frame_callback (void               *data,
                struct wl_callback *callback,
                uint32_t            time)
{
  CdkWindow *window = data;
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  CdkWaylandDisplay *display_wayland =
    CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));
  CdkFrameClock *clock = cdk_window_get_frame_clock (window);
  CdkFrameTimings *timings;

  CDK_NOTE (EVENTS,
            g_message ("frame %p", window));

  wl_callback_destroy (callback);

  if (CDK_WINDOW_DESTROYED (window))
    return;

  if (!impl->awaiting_frame)
    return;

  impl->awaiting_frame = FALSE;
  _cdk_frame_clock_thaw (clock);

  timings = cdk_frame_clock_get_timings (clock, impl->pending_frame_counter);
  impl->pending_frame_counter = 0;

  if (timings == NULL)
    return;

  timings->refresh_interval = 16667; /* default to 1/60th of a second */
  if (impl->display_server.outputs)
    {
      /* We pick a random output out of the outputs that the window touches
       * The rate here is in milli-hertz */
      int refresh_rate =
        _cdk_wayland_screen_get_output_refresh_rate (display_wayland->screen,
                                                     impl->display_server.outputs->data);
      if (refresh_rate != 0)
        timings->refresh_interval = G_GINT64_CONSTANT(1000000000) / refresh_rate;
    }

  fill_presentation_time_from_frame_time (timings, time);

  timings->complete = TRUE;

#ifdef G_ENABLE_DEBUG
  if ((_cdk_debug_flags & CDK_DEBUG_FRAMES) != 0)
    _cdk_frame_clock_debug_print_timings (clock, timings);

  if (cdk_profiler_is_running ())
    _cdk_frame_clock_add_timings_to_profiler (clock, timings);
#endif
}

static const struct wl_callback_listener frame_listener = {
  frame_callback
};

static void
on_frame_clock_before_paint (CdkFrameClock *clock,
                             CdkWindow     *window)
{
  CdkFrameTimings *timings = cdk_frame_clock_get_current_timings (clock);
  gint64 presentation_time;
  gint64 refresh_interval;

  if (window->update_freeze_count > 0)
    return;

  cdk_frame_clock_get_refresh_info (clock,
                                    timings->frame_time,
                                    &refresh_interval, &presentation_time);

  if (presentation_time != 0)
    {
      /* Assume the algorithm used by the DRM backend of Weston - it
       * starts drawing at the next vblank after receiving the commit
       * for this frame, and presentation occurs at the vblank
       * after that.
       */
      timings->predicted_presentation_time = presentation_time + refresh_interval;
    }
  else
    {
      /* As above, but we don't actually know the phase of the vblank,
       * so just assume that we're half way through a refresh cycle.
       */
      timings->predicted_presentation_time = timings->frame_time + refresh_interval / 2 + refresh_interval;
    }
}

static void
on_frame_clock_after_paint (CdkFrameClock *clock,
                            CdkWindow     *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  struct wl_callback *callback;

  if (!impl->pending_commit)
    return;

  if (window->update_freeze_count > 0)
    return;

  callback = wl_surface_frame (impl->display_server.wl_surface);
  wl_callback_add_listener (callback, &frame_listener, window);
  _cdk_frame_clock_freeze (clock);

  /* Before we commit a new buffer, make sure we've backfilled
   * undrawn parts from any old committed buffer
   */
  if (impl->pending_buffer_attached)
    read_back_cairo_surface (window);

  /* From this commit forward, we can't write to the buffer,
   * it's "live".  In the future, if we need to stage more changes
   * we have to allocate a new staging buffer and draw to it instead.
   *
   * Our one saving grace is if the compositor releases the buffer
   * before we need to stage any changes, then we can take it back and
   * use it again.
   */
  wl_surface_commit (impl->display_server.wl_surface);

  if (impl->pending_buffer_attached)
    impl->committed_cairo_surface = g_steal_pointer (&impl->staging_cairo_surface);

  impl->pending_buffer_attached = FALSE;
  impl->pending_commit = FALSE;
  impl->pending_frame_counter = cdk_frame_clock_get_frame_counter (clock);
  impl->awaiting_frame = TRUE;

  g_signal_emit (impl, signals[COMMITTED], 0);
}

static void
window_update_scale (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  CdkWaylandDisplay *display_wayland =
    CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));
  guint32 scale;
  GSList *l;

  if (display_wayland->compositor_version < WL_SURFACE_HAS_BUFFER_SCALE)
    {
      /* We can't set the scale on this surface */
      return;
    }

  scale = 1;
  for (l = impl->display_server.outputs; l != NULL; l = l->next)
    {
      guint32 output_scale =
        _cdk_wayland_screen_get_output_scale (display_wayland->screen, l->data);
      scale = MAX (scale, output_scale);
    }

  /* Notify app that scale changed */
  cdk_wayland_window_maybe_configure (window, window->width, window->height, scale);
}

static void
on_monitors_changed (CdkScreen *screen,
                     CdkWindow *window)
{
  window_update_scale (window);
}


static void cdk_wayland_window_create_surface (CdkWindow *window);

void
_cdk_wayland_display_create_window_impl (CdkDisplay    *display,
                                         CdkWindow     *window,
                                         CdkWindow     *real_parent,
                                         CdkScreen     *screen,
                                         CdkEventMask   event_mask,
                                         CdkWindowAttr *attributes,
                                         gint           attributes_mask)
{
  CdkWaylandDisplay *display_wayland = CDK_WAYLAND_DISPLAY (display);
  CdkWindowImplWayland *impl;
  CdkFrameClock *frame_clock;
  const char *title;

  impl = g_object_new (CDK_TYPE_WINDOW_IMPL_WAYLAND, NULL);
  window->impl = CDK_WINDOW_IMPL (impl);
  impl->unconfigured_width = window->width;
  impl->unconfigured_height = window->height;
  impl->wrapper = CDK_WINDOW (window);
  impl->shortcuts_inhibitors = g_hash_table_new (NULL, NULL);
  impl->using_csd = TRUE;

  if (window->width > 65535)
    {
      g_warning ("Native Windows wider than 65535 pixels are not supported");
      window->width = 65535;
    }
  if (window->height > 65535)
    {
      g_warning ("Native Windows taller than 65535 pixels are not supported");
      window->height = 65535;
    }

  g_object_ref (window);

  /* More likely to be right than just assuming 1 */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  if (display_wayland->compositor_version >= WL_SURFACE_HAS_BUFFER_SCALE &&
      cdk_screen_get_n_monitors (screen) > 0)
    impl->scale = cdk_screen_get_monitor_scale_factor (screen, 0);
G_GNUC_END_IGNORE_DEPRECATIONS

  impl->title = NULL;

  switch (CDK_WINDOW_TYPE (window))
    {
    case CDK_WINDOW_TOPLEVEL:
    case CDK_WINDOW_TEMP:
      if (attributes_mask & CDK_WA_TITLE)
        title = attributes->title;
      else
        title = get_default_title ();

      cdk_window_set_title (window, title);
      break;

    case CDK_WINDOW_CHILD:
    default:
      break;
    }

  cdk_wayland_window_create_surface (window);

  if (attributes_mask & CDK_WA_TYPE_HINT)
    cdk_window_set_type_hint (window, attributes->type_hint);

  frame_clock = cdk_window_get_frame_clock (window);

  g_signal_connect (frame_clock, "before-paint",
                    G_CALLBACK (on_frame_clock_before_paint), window);
  g_signal_connect (frame_clock, "after-paint",
                    G_CALLBACK (on_frame_clock_after_paint), window);

  g_signal_connect (screen, "monitors-changed",
                    G_CALLBACK (on_monitors_changed), window);
}

static void
cdk_wayland_window_attach_image (CdkWindow *window)
{
  CdkWaylandDisplay *display;
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  if (CDK_WINDOW_DESTROYED (window))
    return;

  g_assert (_cdk_wayland_is_shm_surface (impl->staging_cairo_surface));

  /* Attach this new buffer to the surface */
  wl_surface_attach (impl->display_server.wl_surface,
                     _cdk_wayland_shm_surface_get_wl_buffer (impl->staging_cairo_surface),
                     impl->pending_buffer_offset_x,
                     impl->pending_buffer_offset_y);
  impl->pending_buffer_offset_x = 0;
  impl->pending_buffer_offset_y = 0;

  /* Only set the buffer scale if supported by the compositor */
  display = CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));
  if (display->compositor_version >= WL_SURFACE_HAS_BUFFER_SCALE)
    wl_surface_set_buffer_scale (impl->display_server.wl_surface, impl->scale);

  impl->pending_buffer_attached = TRUE;
  impl->pending_commit = TRUE;
}

static const cairo_user_data_key_t cdk_wayland_window_cairo_key;

static void
buffer_release_callback (void             *_data,
                         struct wl_buffer *wl_buffer)
{
  cairo_surface_t *cairo_surface = _data;
  CdkWindowImplWayland *impl = cairo_surface_get_user_data (cairo_surface, &cdk_wayland_window_cairo_key);

  g_return_if_fail (CDK_IS_WINDOW_IMPL_WAYLAND (impl));

  /* The released buffer isn't the latest committed one, we have no further
   * use for it, so clean it up.
   */
  if (impl->committed_cairo_surface != cairo_surface)
    {
      /* If this fails, then the surface buffer got reused before it was
       * released from the compositor
       */
      g_warn_if_fail (impl->staging_cairo_surface != cairo_surface);

      cairo_surface_destroy (cairo_surface);
      return;
    }

  if (impl->staged_updates_region != NULL)
    {
      /* If this fails, then we're tracking staged updates on a staging surface
       * that doesn't exist.
       */
      g_warn_if_fail (impl->staging_cairo_surface != NULL);

      /* If we've staged updates into a new buffer before the release for this
       * buffer came in, then we can't reuse this buffer, so unref it. It may still
       * be alive as a readback buffer though (via impl->backfill_cairo_surface).
       *
       * It's possible a staging surface was allocated but no updates were staged.
       * If that happened, clean up that staging surface now, since the old commit
       * buffer is available again, and reusing the old commit buffer for future
       * updates will save having to do a read back later.
       */
      if (!cairo_region_is_empty (impl->staged_updates_region))
        {
          g_clear_pointer (&impl->committed_cairo_surface, cairo_surface_destroy);
          return;
        }
      else
        {
          g_clear_pointer (&impl->staged_updates_region, cairo_region_destroy);
          g_clear_pointer (&impl->staging_cairo_surface, cairo_surface_destroy);
        }
    }

  /* Release came in, we haven't done any interim updates, so we can just use
   * the old committed buffer again.
   */
  impl->staging_cairo_surface = g_steal_pointer (&impl->committed_cairo_surface);
}

static const struct wl_buffer_listener buffer_listener = {
  buffer_release_callback
};

static void
cdk_wayland_window_ensure_cairo_surface (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  /* If we are drawing using OpenGL then we only need a logical 1x1 surface. */
  if (impl->display_server.egl_window)
    {
      if (impl->staging_cairo_surface &&
          _cdk_wayland_is_shm_surface (impl->staging_cairo_surface))
        g_clear_pointer (&impl->staging_cairo_surface, cairo_surface_destroy);

      if (!impl->staging_cairo_surface)
        {
          impl->staging_cairo_surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                                                    impl->scale,
                                                                    impl->scale);
          cairo_surface_set_device_scale (impl->staging_cairo_surface,
                                          impl->scale, impl->scale);
        }
    }
  else if (!impl->staging_cairo_surface)
    {
      CdkWaylandDisplay *display_wayland = CDK_WAYLAND_DISPLAY (cdk_window_get_display (impl->wrapper));
      struct wl_buffer *buffer;

      impl->staging_cairo_surface = _cdk_wayland_display_create_shm_surface (display_wayland,
                                                                             impl->wrapper->width,
                                                                             impl->wrapper->height,
                                                                             impl->scale);
      cairo_surface_set_user_data (impl->staging_cairo_surface,
                                   &cdk_wayland_window_cairo_key,
                                   g_object_ref (impl),
                                   (cairo_destroy_func_t)
                                   g_object_unref);
      buffer = _cdk_wayland_shm_surface_get_wl_buffer (impl->staging_cairo_surface);
      wl_buffer_add_listener (buffer, &buffer_listener, impl->staging_cairo_surface);
    }
}

/* The cairo surface returned here uses a memory segment that's shared
 * with the display server.  This is not a temporary buffer that gets
 * copied to the display server, but the actual buffer the display server
 * will ultimately end up sending to the GPU. At the time this happens
 * impl->committed_cairo_surface gets set to impl->staging_cairo_surface, and
 * impl->staging_cairo_surface gets nullified.
 */
static cairo_surface_t *
cdk_wayland_window_ref_cairo_surface (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  if (CDK_WINDOW_DESTROYED (impl->wrapper))
    return NULL;

  cdk_wayland_window_ensure_cairo_surface (window);

  cairo_surface_reference (impl->staging_cairo_surface);

  return impl->staging_cairo_surface;
}

static cairo_surface_t *
cdk_wayland_window_create_similar_image_surface (CdkWindow *     window,
                                                 cairo_format_t  format,
                                                 int             width,
                                                 int             height)
{
  return cairo_image_surface_create (format, width, height);
}

static gboolean
cdk_window_impl_wayland_begin_paint (CdkWindow *window)
{
  cdk_wayland_window_ensure_cairo_surface (window);

  return FALSE;
}

static void
cdk_window_impl_wayland_end_paint (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  cairo_rectangle_int_t rect;

  if (!CDK_WINDOW_IS_MAPPED (window))
    return;

  if (impl->staging_cairo_surface &&
      _cdk_wayland_is_shm_surface (impl->staging_cairo_surface) &&
      !window->current_paint.use_gl &&
      !cairo_region_is_empty (window->current_paint.region))
    {
      int i, n;

      cdk_wayland_window_attach_image (window);

      /* If there's a committed buffer pending, then track which
       * updates are staged until the next frame, so we can back
       * fill the unstaged parts of the staging buffer with the
       * last frame.
       */
      if (impl->committed_cairo_surface != NULL)
        {
          if (impl->staged_updates_region == NULL)
            {
              impl->staged_updates_region = cairo_region_copy (window->current_paint.region);
              impl->backfill_cairo_surface = cairo_surface_reference (impl->committed_cairo_surface);
            }
          else
            {
              cairo_region_union (impl->staged_updates_region, window->current_paint.region);
            }
        }

      n = cairo_region_num_rectangles (window->current_paint.region);
      for (i = 0; i < n; i++)
        {
          cairo_region_get_rectangle (window->current_paint.region, i, &rect);
          wl_surface_damage (impl->display_server.wl_surface, rect.x, rect.y, rect.width, rect.height);
        }

      impl->pending_commit = TRUE;
    }

  cdk_wayland_window_sync_margin (window);
  cdk_wayland_window_sync_opaque_region (window);
  cdk_wayland_window_sync_input_region (window);
}

static gboolean
cdk_window_impl_wayland_beep (CdkWindow *window)
{
  cdk_wayland_display_system_bell (cdk_window_get_display (window),
                                   window);

  return TRUE;
}

static void
cdk_window_impl_wayland_finalize (GObject *object)
{
  CdkWindowImplWayland *impl;

  g_return_if_fail (CDK_IS_WINDOW_IMPL_WAYLAND (object));

  impl = CDK_WINDOW_IMPL_WAYLAND (object);

  if (cdk_wayland_window_is_exported (impl->wrapper))
    cdk_wayland_window_unexport_handle (impl->wrapper);

  g_free (impl->title);

  g_free (impl->application.application_id);
  g_free (impl->application.app_menu_path);
  g_free (impl->application.menubar_path);
  g_free (impl->application.window_object_path);
  g_free (impl->application.application_object_path);
  g_free (impl->application.unique_bus_name);

  g_clear_pointer (&impl->opaque_region, cairo_region_destroy);
  g_clear_pointer (&impl->input_region, cairo_region_destroy);
  g_clear_pointer (&impl->staged_updates_region, cairo_region_destroy);

  g_clear_pointer (&impl->shortcuts_inhibitors, g_hash_table_unref);

  G_OBJECT_CLASS (_cdk_window_impl_wayland_parent_class)->finalize (object);
}

static void
cdk_wayland_window_configure (CdkWindow *window,
                              int        width,
                              int        height,
                              int        scale)
{
  CdkDisplay *display;
  CdkEvent *event;

  g_return_if_fail (width > 0);
  g_return_if_fail (height > 0);

  event = cdk_event_new (CDK_CONFIGURE);
  event->configure.window = g_object_ref (window);
  event->configure.send_event = FALSE;
  event->configure.width = width;
  event->configure.height = height;

  cdk_wayland_window_update_size (window, width, height, scale);
  _cdk_window_update_size (window);

  display = cdk_window_get_display (window);
  _cdk_wayland_display_deliver_event (display, event);
}

static gboolean
is_realized_shell_surface (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  return (impl->display_server.xdg_surface ||
          impl->display_server.zxdg_surface_v6);
}

static gboolean
is_realized_toplevel (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  return (impl->display_server.xdg_toplevel ||
          impl->display_server.zxdg_toplevel_v6);
}

static gboolean
is_realized_popup (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  return (impl->display_server.xdg_popup ||
          impl->display_server.zxdg_popup_v6);
}

static gboolean
should_inhibit_resize (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  if (impl->display_server.wl_subsurface)
    return FALSE;
  else if (impl->use_custom_surface)
    return FALSE;
  else if (impl->hint == CDK_WINDOW_TYPE_HINT_DND)
    return FALSE;
  else if (is_realized_popup (window))
    return FALSE;
  else if (should_map_as_popup (window))
    return FALSE;
  else if (should_map_as_subsurface (window))
    return FALSE;

  /* This should now either be, or eventually be, a toplevel window,
   * and we should wait for the initial configure to really configure it.
   */
  return !impl->initial_configure_received;
}

static void
cdk_wayland_window_maybe_configure (CdkWindow *window,
                                    int        width,
                                    int        height,
                                    int        scale)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  gboolean is_xdg_popup;
  gboolean is_visible;

  impl->unconfigured_width = calculate_width_without_margin (window, width);
  impl->unconfigured_height = calculate_height_without_margin (window, height);

  if (should_inhibit_resize (window))
    return;

  if (window->width == width &&
      window->height == height &&
      impl->scale == scale)
    return;

  /* For xdg_popup using an xdg_positioner, there is a race condition if
   * the application tries to change the size after it's mapped, but before
   * the initial configure is received, so hide and show the surface again
   * force the new size onto the compositor. See bug #772505.
   */

  is_xdg_popup = is_realized_popup (window);
  is_visible = cdk_window_is_visible (window);

  if (is_xdg_popup &&
      is_visible &&
      !impl->initial_configure_received &&
      !impl->configuring_popup)
    cdk_window_hide (window);

  cdk_wayland_window_configure (window, width, height, scale);

  if (is_xdg_popup &&
      is_visible &&
      !impl->initial_configure_received &&
      !impl->configuring_popup)
    cdk_window_show (window);
}

static void
cdk_wayland_window_sync_parent (CdkWindow *window,
                                CdkWindow *parent)
{
  CdkWaylandDisplay *display_wayland =
    CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  CdkWindowImplWayland *impl_parent = NULL;

  g_assert (parent == NULL ||
            cdk_window_get_display (window) == cdk_window_get_display (parent));

  if (!is_realized_toplevel (window))
    return;

  if (impl->transient_for)
    impl_parent = CDK_WINDOW_IMPL_WAYLAND (impl->transient_for->impl);
  else if (parent)
    impl_parent = CDK_WINDOW_IMPL_WAYLAND (parent->impl);

  /* XXX: Is this correct? */
  if (impl_parent && !impl_parent->display_server.wl_surface)
    return;

  switch (display_wayland->shell_variant)
    {
    case CDK_WAYLAND_SHELL_VARIANT_XDG_SHELL:
      {
        struct xdg_toplevel *parent_toplevel;

        if (impl_parent)
          parent_toplevel = impl_parent->display_server.xdg_toplevel;
        else
          parent_toplevel = NULL;

        xdg_toplevel_set_parent (impl->display_server.xdg_toplevel,
                                 parent_toplevel);
        break;
      }
      break;
    case CDK_WAYLAND_SHELL_VARIANT_ZXDG_SHELL_V6:
      {
        struct zxdg_toplevel_v6 *parent_toplevel;

        if (impl_parent)
          parent_toplevel = impl_parent->display_server.zxdg_toplevel_v6;
        else
          parent_toplevel = NULL;

        zxdg_toplevel_v6_set_parent (impl->display_server.zxdg_toplevel_v6,
                                     parent_toplevel);
        break;
      }
    }
}

static void
cdk_wayland_window_sync_parent_of_imported (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  if (!impl->display_server.wl_surface)
    return;

  if (!impl->imported_transient_for)
    return;

  if (!is_realized_toplevel (window))
    return;

  zxdg_imported_v1_set_parent_of (impl->imported_transient_for,
                                  impl->display_server.wl_surface);
}

static void
cdk_wayland_window_update_dialogs (CdkWindow *window)
{
  CdkWaylandDisplay *display_wayland =
    CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));
  GList *l;

  if (!display_wayland->orphan_dialogs)
    return;

  for (l = display_wayland->orphan_dialogs; l; l = l->next)
    {
      CdkWindow *w = l->data;
      CdkWindowImplWayland *impl;

      if (!CDK_IS_WINDOW_IMPL_WAYLAND(w->impl))
        continue;

      impl = CDK_WINDOW_IMPL_WAYLAND (w->impl);
      if (w == window)
	continue;
      if (impl->hint != CDK_WINDOW_TYPE_HINT_DIALOG)
        continue;
      if (impl->transient_for)
        continue;

      /* Update the parent relationship only for dialogs without transients */
      cdk_wayland_window_sync_parent (w, window);
    }
}

static void
cdk_wayland_window_sync_title (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  CdkWaylandDisplay *display_wayland =
    CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));

  if (!is_realized_toplevel (window))
    return;

  if (!impl->title)
    return;

  switch (display_wayland->shell_variant)
    {
    case CDK_WAYLAND_SHELL_VARIANT_XDG_SHELL:
      xdg_toplevel_set_title (impl->display_server.xdg_toplevel,
                              impl->title);
      break;
    case CDK_WAYLAND_SHELL_VARIANT_ZXDG_SHELL_V6:
      zxdg_toplevel_v6_set_title (impl->display_server.zxdg_toplevel_v6,
                                  impl->title);
      break;
    }
}

static void
cdk_wayland_window_get_window_geometry (CdkWindow    *window,
                                        CdkRectangle *geometry)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  *geometry = (CdkRectangle) {
    .x = impl->margin_left,
    .y = impl->margin_top,
    .width = calculate_width_without_margin (window, window->width),
    .height = calculate_height_without_margin (window, window->height)
  };
}

static void
cdk_wayland_window_sync_margin (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  CdkWaylandDisplay *display_wayland =
    CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));
  CdkRectangle geometry;

  if (!is_realized_shell_surface (window))
    return;

  cdk_wayland_window_get_window_geometry (window, &geometry);

  g_return_if_fail (geometry.width > 0 && geometry.height > 0);

  cdk_window_set_geometry_hints (window,
                                 &impl->geometry_hints,
                                 impl->geometry_mask);

  switch (display_wayland->shell_variant)
    {
    case CDK_WAYLAND_SHELL_VARIANT_XDG_SHELL:
      xdg_surface_set_window_geometry (impl->display_server.xdg_surface,
                                       geometry.x,
                                       geometry.y,
                                       geometry.width,
                                       geometry.height);
      break;
    case CDK_WAYLAND_SHELL_VARIANT_ZXDG_SHELL_V6:
      zxdg_surface_v6_set_window_geometry (impl->display_server.zxdg_surface_v6,
                                           geometry.x,
                                           geometry.y,
                                           geometry.width,
                                           geometry.height);
      break;
    }
}

static struct wl_region *
wl_region_from_cairo_region (CdkWaylandDisplay *display,
                             cairo_region_t    *region)
{
  struct wl_region *wl_region;
  int i, n_rects;

  wl_region = wl_compositor_create_region (display->compositor);
  if (wl_region == NULL)
    return NULL;

  n_rects = cairo_region_num_rectangles (region);
  for (i = 0; i < n_rects; i++)
    {
      cairo_rectangle_int_t rect;
      cairo_region_get_rectangle (region, i, &rect);
      wl_region_add (wl_region, rect.x, rect.y, rect.width, rect.height);
    }

  return wl_region;
}

static void
cdk_wayland_window_sync_opaque_region (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  struct wl_region *wl_region = NULL;

  if (!impl->display_server.wl_surface)
    return;

  if (!impl->opaque_region_dirty)
    return;

  if (impl->opaque_region != NULL)
    wl_region = wl_region_from_cairo_region (CDK_WAYLAND_DISPLAY (cdk_window_get_display (window)),
                                             impl->opaque_region);

  wl_surface_set_opaque_region (impl->display_server.wl_surface, wl_region);

  if (wl_region != NULL)
    wl_region_destroy (wl_region);

  impl->opaque_region_dirty = FALSE;
}

static void
cdk_wayland_window_sync_input_region (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  struct wl_region *wl_region = NULL;

  if (!impl->display_server.wl_surface)
    return;

  if (!impl->input_region_dirty)
    return;

  if (impl->input_region != NULL)
    wl_region = wl_region_from_cairo_region (CDK_WAYLAND_DISPLAY (cdk_window_get_display (window)),
                                             impl->input_region);

  wl_surface_set_input_region (impl->display_server.wl_surface, wl_region);

  if (wl_region != NULL)
    wl_region_destroy (wl_region);

  impl->input_region_dirty = FALSE;
}

static void
cdk_wayland_set_input_region_if_empty (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  CdkWaylandDisplay *display;
  struct wl_region *empty;

  if (!impl->input_region_dirty)
    return;

  if (impl->input_region == NULL)
    return;

  if (!cairo_region_is_empty (impl->input_region))
    return;

  display = CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));
  empty = wl_compositor_create_region (display->compositor);

  wl_surface_set_input_region (impl->display_server.wl_surface, empty);
  wl_region_destroy (empty);

  impl->input_region_dirty = FALSE;
}

static void
surface_enter (void              *data,
               struct wl_surface *wl_surface,
               struct wl_output  *output)
{
  CdkWindow *window = CDK_WINDOW (data);
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  CDK_NOTE (EVENTS,
            g_message ("surface enter, window %p output %p", window, output));

  impl->display_server.outputs = g_slist_prepend (impl->display_server.outputs, output);

  window_update_scale (window);
}

static void
surface_leave (void              *data,
               struct wl_surface *wl_surface,
               struct wl_output  *output)
{
  CdkWindow *window = CDK_WINDOW (data);
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  CDK_NOTE (EVENTS,
            g_message ("surface leave, window %p output %p", window, output));

  impl->display_server.outputs = g_slist_remove (impl->display_server.outputs, output);

  if (impl->display_server.outputs)
    window_update_scale (window);
}

static const struct wl_surface_listener surface_listener = {
  .enter = surface_enter,
  .leave = surface_leave
};

static void
on_parent_surface_committed (CdkWindowImplWayland *parent_impl,
                             CdkWindow            *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  g_signal_handler_disconnect (parent_impl,
                               impl->parent_surface_committed_handler);
  impl->parent_surface_committed_handler = 0;

  wl_subsurface_set_desync (impl->display_server.wl_subsurface);

  /* Special case if the input region is empty, it won't change on resize */
  cdk_wayland_set_input_region_if_empty (window);
}

static void
cdk_wayland_window_set_subsurface_position (CdkWindow *window,
                                            int        x,
                                            int        y)
{
  CdkWindowImplWayland *impl;

  impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  wl_subsurface_set_position (impl->display_server.wl_subsurface, x, y);
  impl->subsurface_x = x;
  impl->subsurface_y = y;

  cdk_window_request_transient_parent_commit (window);
}

static void
cdk_wayland_window_create_subsurface (CdkWindow *window)
{
  CdkWindowImplWayland *impl, *parent_impl = NULL;

  impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  if (!impl->display_server.wl_surface)
    return; /* Bail out, surface and subsurface will be created later when shown */

  if (impl->display_server.wl_subsurface)
    return;

  if (impl->transient_for)
    parent_impl = CDK_WINDOW_IMPL_WAYLAND (impl->transient_for->impl);

  if (parent_impl && parent_impl->display_server.wl_surface)
    {
      CdkWaylandDisplay *display_wayland;

      display_wayland = CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));
      impl->display_server.wl_subsurface =
        wl_subcompositor_get_subsurface (display_wayland->subcompositor,
                                         impl->display_server.wl_surface, parent_impl->display_server.wl_surface);

      /* In order to synchronize the initial position with the initial frame
       * content, wait with making the subsurface desynchronized until after
       * the parent was committed.
       */
      impl->parent_surface_committed_handler =
        g_signal_connect_object (parent_impl, "committed",
                                 G_CALLBACK (on_parent_surface_committed),
                                 window, 0);

      cdk_wayland_window_set_subsurface_position (window,
                                                  window->x + window->abs_x,
                                                  window->y + window->abs_y);
    }
}

static void
cdk_wayland_window_create_surface (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  CdkWaylandDisplay *display_wayland = CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));

  impl->display_server.wl_surface = wl_compositor_create_surface (display_wayland->compositor);
  wl_surface_add_listener (impl->display_server.wl_surface, &surface_listener, window);
}

static gboolean
should_use_fixed_size (CdkWindowState state)
{
  return state & (CDK_WINDOW_STATE_MAXIMIZED |
                  CDK_WINDOW_STATE_FULLSCREEN |
                  CDK_WINDOW_STATE_TILED);
}

static void
cdk_wayland_window_handle_configure (CdkWindow *window,
                                     uint32_t   serial)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  CdkWaylandDisplay *display_wayland =
    CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));
  CdkWindowState new_state;
  int width = impl->pending.width;
  int height = impl->pending.height;
  gboolean fixed_size;
  gboolean saved_size;

  if (!impl->initial_configure_received)
    {
      cdk_window_thaw_updates (window);
      impl->initial_configure_received = TRUE;
    }

  if (impl->display_server.xdg_popup)
    {
      xdg_surface_ack_configure (impl->display_server.xdg_surface, serial);
      return;
    }
  else if (impl->display_server.zxdg_popup_v6)
    {
      zxdg_surface_v6_ack_configure (impl->display_server.zxdg_surface_v6,
                                     serial);
      return;
    }

  new_state = impl->pending.state;
  impl->pending.state = 0;

  fixed_size = should_use_fixed_size (new_state);

  saved_size = (width == 0 && height == 0);
  /* According to xdg_shell, an xdg_surface.configure with size 0x0
   * should be interpreted as that it is up to the client to set a
   * size.
   *
   * When transitioning from maximize or fullscreen state, this means
   * the client should configure its size back to what it was before
   * being maximize or fullscreen.
   * Additionally, if we receive a manual resize request, we must prefer this
   * new size instead of the compositor's size hints.
   * In such a scenario, and without letting the compositor know about the new
   * size, the client has to manage all dimensions and ignore any server hints.
   */
  if (!fixed_size && (saved_size || impl->saved_size_changed))
    {
      width = impl->saved_width;
      height = impl->saved_height;
      impl->saved_size_changed = FALSE;
    }

  if (width > 0 && height > 0)
    {
      CdkWindowHints geometry_mask = impl->geometry_mask;
      int configure_width;
      int configure_height;

      /* Ignore size increments for maximized/fullscreen windows */
      if (fixed_size)
        geometry_mask &= ~CDK_HINT_RESIZE_INC;
      if (!saved_size)
        {
          /* Do not reapply contrains if we are restoring original size */
          cdk_window_constrain_size (&impl->geometry_hints,
                                     geometry_mask,
                                     calculate_width_with_margin (window, width),
                                     calculate_height_with_margin (window, height),
                                     &width,
                                     &height);

          /* Save size for next time we get 0x0 */
          _cdk_wayland_window_save_size (window);
        }

      if (saved_size)
        {
          configure_width = calculate_width_with_margin (window, width);
          configure_height = calculate_height_with_margin (window, height);
        }
      else
        {
          configure_width = width;
          configure_height = height;
        }
      cdk_wayland_window_configure (window,
                                    configure_width,
                                    configure_height,
                                    impl->scale);
    }
  else
    {
      int unconfigured_width;
      int unconfigured_height;

      unconfigured_width =
        calculate_width_with_margin (window, impl->unconfigured_width);
      unconfigured_height =
        calculate_height_with_margin (window, impl->unconfigured_height);
      cdk_wayland_window_configure (window,
                                    unconfigured_width,
                                    unconfigured_height,
                                    impl->scale);
    }

  if (fixed_size)
    {
      impl->fixed_size_width = width;
      impl->fixed_size_height = height;
    }

  CDK_NOTE (EVENTS,
            g_message ("configure, window %p %dx%d,%s%s%s%s",
                       window, width, height,
                       (new_state & CDK_WINDOW_STATE_FULLSCREEN) ? " fullscreen" : "",
                       (new_state & CDK_WINDOW_STATE_MAXIMIZED) ? " maximized" : "",
                       (new_state & CDK_WINDOW_STATE_FOCUSED) ? " focused" : "",
                       (new_state & CDK_WINDOW_STATE_TILED) ? " tiled" : ""));

  _cdk_set_window_state (window, new_state);

  switch (display_wayland->shell_variant)
    {
    case CDK_WAYLAND_SHELL_VARIANT_XDG_SHELL:
      xdg_surface_ack_configure (impl->display_server.xdg_surface, serial);
      break;
    case CDK_WAYLAND_SHELL_VARIANT_ZXDG_SHELL_V6:
      zxdg_surface_v6_ack_configure (impl->display_server.zxdg_surface_v6,
                                     serial);
      break;
    }

  if (impl->hint != CDK_WINDOW_TYPE_HINT_DIALOG &&
      new_state & CDK_WINDOW_STATE_FOCUSED)
    cdk_wayland_window_update_dialogs (window);
}

static void
cdk_wayland_window_handle_configure_toplevel (CdkWindow     *window,
                                              int32_t        width,
                                              int32_t        height,
                                              CdkWindowState state)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  impl->pending.state |= state;
  impl->pending.width = width;
  impl->pending.height = height;
}

static void
cdk_wayland_window_handle_close (CdkWindow *window)
{
  CdkDisplay *display;
  CdkEvent *event;

  CDK_NOTE (EVENTS,
            g_message ("close %p", window));

  event = cdk_event_new (CDK_DELETE);
  event->any.window = g_object_ref (window);
  event->any.send_event = TRUE;

  display = cdk_window_get_display (window);
  _cdk_wayland_display_deliver_event (display, event);
}

static void
xdg_surface_configure (void               *data,
                       struct xdg_surface *xdg_surface,
                       uint32_t            serial)
{
  CdkWindow *window = CDK_WINDOW (data);

  cdk_wayland_window_handle_configure (window, serial);
}

static const struct xdg_surface_listener xdg_surface_listener = {
  xdg_surface_configure,
};

static void
xdg_toplevel_configure (void                *data,
                        struct xdg_toplevel *xdg_toplevel,
                        int32_t              width,
                        int32_t              height,
                        struct wl_array     *states)
{
  CdkWindow *window = CDK_WINDOW (data);
  uint32_t *p;
  CdkWindowState pending_state = 0;

  wl_array_for_each (p, states)
    {
      uint32_t state = *p;

      switch (state)
        {
        case XDG_TOPLEVEL_STATE_FULLSCREEN:
          pending_state |= CDK_WINDOW_STATE_FULLSCREEN;
          break;
        case XDG_TOPLEVEL_STATE_MAXIMIZED:
          pending_state |= CDK_WINDOW_STATE_MAXIMIZED;
          break;
        case XDG_TOPLEVEL_STATE_ACTIVATED:
          pending_state |= CDK_WINDOW_STATE_FOCUSED;
          break;
        case XDG_TOPLEVEL_STATE_RESIZING:
          break;
        default:
          /* Unknown state */
          break;
        }
    }

  cdk_wayland_window_handle_configure_toplevel (window, width, height,
                                                pending_state);
}

static void
xdg_toplevel_close (void                *data,
                    struct xdg_toplevel *xdg_toplevel)
{
  CdkWindow *window = CDK_WINDOW (data);

  cdk_wayland_window_handle_close (window);
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
  .configure = xdg_toplevel_configure,
  .close = xdg_toplevel_close,
};

static void
create_xdg_toplevel_resources (CdkWindow *window)
{
  CdkWaylandDisplay *display_wayland =
    CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  impl->display_server.xdg_surface =
    xdg_wm_base_get_xdg_surface (display_wayland->xdg_wm_base,
                                 impl->display_server.wl_surface);
  xdg_surface_add_listener (impl->display_server.xdg_surface,
                            &xdg_surface_listener,
                            window);

  impl->display_server.xdg_toplevel =
    xdg_surface_get_toplevel (impl->display_server.xdg_surface);
  xdg_toplevel_add_listener (impl->display_server.xdg_toplevel,
                             &xdg_toplevel_listener,
                             window);
}

static void
zxdg_surface_v6_configure (void                   *data,
                           struct zxdg_surface_v6 *xdg_surface,
                           uint32_t                serial)
{
  CdkWindow *window = CDK_WINDOW (data);

  cdk_wayland_window_handle_configure (window, serial);
}

static const struct zxdg_surface_v6_listener zxdg_surface_v6_listener = {
  zxdg_surface_v6_configure,
};

static void
zxdg_toplevel_v6_configure (void                    *data,
                            struct zxdg_toplevel_v6 *xdg_toplevel,
                            int32_t                  width,
                            int32_t                  height,
                            struct wl_array         *states)
{
  CdkWindow *window = CDK_WINDOW (data);
  uint32_t *p;
  CdkWindowState pending_state = 0;

  wl_array_for_each (p, states)
    {
      uint32_t state = *p;

      switch (state)
        {
        case ZXDG_TOPLEVEL_V6_STATE_FULLSCREEN:
          pending_state |= CDK_WINDOW_STATE_FULLSCREEN;
          break;
        case ZXDG_TOPLEVEL_V6_STATE_MAXIMIZED:
          pending_state |= CDK_WINDOW_STATE_MAXIMIZED;
          break;
        case ZXDG_TOPLEVEL_V6_STATE_ACTIVATED:
          pending_state |= CDK_WINDOW_STATE_FOCUSED;
          break;
        case ZXDG_TOPLEVEL_V6_STATE_RESIZING:
          break;
        default:
          /* Unknown state */
          break;
        }
    }

  cdk_wayland_window_handle_configure_toplevel (window, width, height,
                                                pending_state);
}

static void
zxdg_toplevel_v6_close (void                    *data,
                        struct zxdg_toplevel_v6 *xdg_toplevel)
{
  CdkWindow *window = CDK_WINDOW (data);

  cdk_wayland_window_handle_close (window);
}

static const struct zxdg_toplevel_v6_listener zxdg_toplevel_v6_listener = {
  zxdg_toplevel_v6_configure,
  zxdg_toplevel_v6_close,
};

static void
create_zxdg_toplevel_v6_resources (CdkWindow *window)
{
  CdkWaylandDisplay *display_wayland =
    CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  impl->display_server.zxdg_surface_v6 =
    zxdg_shell_v6_get_xdg_surface (display_wayland->zxdg_shell_v6,
                                   impl->display_server.wl_surface);
  zxdg_surface_v6_add_listener (impl->display_server.zxdg_surface_v6,
                                &zxdg_surface_v6_listener,
                                window);

  impl->display_server.zxdg_toplevel_v6 =
    zxdg_surface_v6_get_toplevel (impl->display_server.zxdg_surface_v6);
  zxdg_toplevel_v6_add_listener (impl->display_server.zxdg_toplevel_v6,
                                 &zxdg_toplevel_v6_listener,
                                 window);
}

void
cdk_wayland_window_set_application_id (CdkWindow *window, const char* application_id)
{
  CdkWindowImplWayland *impl;
  CdkWaylandDisplay *display_wayland;

  g_return_if_fail (application_id != NULL);

  if (CDK_WINDOW_DESTROYED (window))
    return;

  if (!is_realized_toplevel (window))
    return;

  display_wayland = CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));
  impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  switch (display_wayland->shell_variant)
    {
    case CDK_WAYLAND_SHELL_VARIANT_XDG_SHELL:
      xdg_toplevel_set_app_id (impl->display_server.xdg_toplevel,
                               application_id);
      break;
    case CDK_WAYLAND_SHELL_VARIANT_ZXDG_SHELL_V6:
      zxdg_toplevel_v6_set_app_id (impl->display_server.zxdg_toplevel_v6,
                                   application_id);
      break;
    }
}

static void
cdk_wayland_window_create_xdg_toplevel (CdkWindow *window)
{
  CdkWaylandDisplay *display_wayland = CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  const gchar *app_id;
  CdkScreen *screen = cdk_window_get_screen (window);
  struct wl_output *fullscreen_output = NULL;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  if (impl->initial_fullscreen_monitor >= 0 &&
      impl->initial_fullscreen_monitor < cdk_screen_get_n_monitors (screen))
    fullscreen_output =
      _cdk_wayland_screen_get_wl_output (screen,
                                         impl->initial_fullscreen_monitor);
G_GNUC_END_IGNORE_DEPRECATIONS

  cdk_window_freeze_updates (window);

  switch (display_wayland->shell_variant)
    {
    case CDK_WAYLAND_SHELL_VARIANT_XDG_SHELL:
      create_xdg_toplevel_resources (window);
      break;
    case CDK_WAYLAND_SHELL_VARIANT_ZXDG_SHELL_V6:
      create_zxdg_toplevel_v6_resources (window);
      break;
    }

  cdk_wayland_window_sync_parent (window, NULL);
  cdk_wayland_window_sync_parent_of_imported (window);
  cdk_wayland_window_sync_title (window);

  switch (display_wayland->shell_variant)
    {
    case CDK_WAYLAND_SHELL_VARIANT_XDG_SHELL:
      if (window->state & CDK_WINDOW_STATE_MAXIMIZED)
        xdg_toplevel_set_maximized (impl->display_server.xdg_toplevel);
      if (window->state & CDK_WINDOW_STATE_FULLSCREEN)
        xdg_toplevel_set_fullscreen (impl->display_server.xdg_toplevel,
                                     fullscreen_output);
      break;
    case CDK_WAYLAND_SHELL_VARIANT_ZXDG_SHELL_V6:
      if (window->state & CDK_WINDOW_STATE_MAXIMIZED)
        zxdg_toplevel_v6_set_maximized (impl->display_server.zxdg_toplevel_v6);
      if (window->state & CDK_WINDOW_STATE_FULLSCREEN)
        zxdg_toplevel_v6_set_fullscreen (impl->display_server.zxdg_toplevel_v6,
                                         fullscreen_output);
      break;
    }

  app_id = g_get_prgname ();
  if (app_id == NULL)
    app_id = cdk_get_program_class ();

  cdk_wayland_window_set_application_id (window, app_id);

  maybe_set_ctk_surface_dbus_properties (window);
  maybe_set_ctk_surface_modal (window);

  if (impl->hint == CDK_WINDOW_TYPE_HINT_DIALOG)
    _cdk_wayland_screen_add_orphan_dialog (window);

  wl_surface_commit (impl->display_server.wl_surface);
}

static void
cdk_wayland_window_handle_configure_popup (CdkWindow *window,
                                           int32_t    x,
                                           int32_t    y,
                                           int32_t    width,
                                           int32_t    height)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  CdkRectangle flipped_rect;
  CdkRectangle final_rect;
  gboolean flipped_x;
  gboolean flipped_y;

  g_return_if_fail (impl->transient_for);

  if (impl->position_method != POSITION_METHOD_MOVE_TO_RECT)
    return;

  calculate_moved_to_rect_result (window, x, y, width, height,
                                  &flipped_rect,
                                  &final_rect,
                                  &flipped_x,
                                  &flipped_y);

  impl->position_method = POSITION_METHOD_MOVE_TO_RECT;

  g_signal_emit_by_name (window,
                         "moved-to-rect",
                         &flipped_rect,
                         &final_rect,
                         flipped_x,
                         flipped_y);
}

static void
xdg_popup_configure (void             *data,
                     struct xdg_popup *xdg_popup,
                     int32_t           x,
                     int32_t           y,
                     int32_t           width,
                     int32_t           height)
{
  CdkWindow *window = CDK_WINDOW (data);

  cdk_wayland_window_handle_configure_popup (window, x, y, width, height);
}

static void
xdg_popup_done (void             *data,
                struct xdg_popup *xdg_popup)
{
  CdkWindow *window = CDK_WINDOW (data);

  CDK_NOTE (EVENTS,
            g_message ("done %p", window));

  cdk_window_hide (window);
}

static const struct xdg_popup_listener xdg_popup_listener = {
  .configure = xdg_popup_configure,
  .popup_done = xdg_popup_done,
};

static void
zxdg_popup_v6_configure (void                 *data,
                         struct zxdg_popup_v6 *xdg_popup,
                         int32_t               x,
                         int32_t               y,
                         int32_t               width,
                         int32_t               height)
{
  CdkWindow *window = CDK_WINDOW (data);

  cdk_wayland_window_handle_configure_popup (window, x, y, width, height);
}

static void
zxdg_popup_v6_done (void                 *data,
                    struct zxdg_popup_v6 *xdg_popup)
{
  CdkWindow *window = CDK_WINDOW (data);

  CDK_NOTE (EVENTS,
            g_message ("done %p", window));

  cdk_window_hide (window);
}

static const struct zxdg_popup_v6_listener zxdg_popup_v6_listener = {
  zxdg_popup_v6_configure,
  zxdg_popup_v6_done,
};

static enum xdg_positioner_anchor
rect_anchor_to_anchor (CdkGravity rect_anchor)
{
  switch (rect_anchor)
    {
    case CDK_GRAVITY_NORTH_WEST:
    case CDK_GRAVITY_STATIC:
      return XDG_POSITIONER_ANCHOR_TOP_LEFT;
    case CDK_GRAVITY_NORTH:
      return XDG_POSITIONER_ANCHOR_TOP;
    case CDK_GRAVITY_NORTH_EAST:
      return XDG_POSITIONER_ANCHOR_TOP_RIGHT;
    case CDK_GRAVITY_WEST:
      return XDG_POSITIONER_ANCHOR_LEFT;
    case CDK_GRAVITY_CENTER:
      return XDG_POSITIONER_ANCHOR_NONE;
    case CDK_GRAVITY_EAST:
      return XDG_POSITIONER_ANCHOR_RIGHT;
    case CDK_GRAVITY_SOUTH_WEST:
      return XDG_POSITIONER_ANCHOR_BOTTOM_LEFT;
    case CDK_GRAVITY_SOUTH:
      return XDG_POSITIONER_ANCHOR_BOTTOM;
    case CDK_GRAVITY_SOUTH_EAST:
      return XDG_POSITIONER_ANCHOR_BOTTOM_RIGHT;
    default:
      g_assert_not_reached ();
    }
}

static enum xdg_positioner_gravity
window_anchor_to_gravity (CdkGravity rect_anchor)
{
  switch (rect_anchor)
    {
    case CDK_GRAVITY_NORTH_WEST:
    case CDK_GRAVITY_STATIC:
      return XDG_POSITIONER_GRAVITY_BOTTOM_RIGHT;
    case CDK_GRAVITY_NORTH:
      return XDG_POSITIONER_GRAVITY_BOTTOM;
    case CDK_GRAVITY_NORTH_EAST:
      return XDG_POSITIONER_GRAVITY_BOTTOM_LEFT;
    case CDK_GRAVITY_WEST:
      return XDG_POSITIONER_GRAVITY_RIGHT;
    case CDK_GRAVITY_CENTER:
      return XDG_POSITIONER_GRAVITY_NONE;
    case CDK_GRAVITY_EAST:
      return XDG_POSITIONER_GRAVITY_LEFT;
    case CDK_GRAVITY_SOUTH_WEST:
      return XDG_POSITIONER_GRAVITY_TOP_RIGHT;
    case CDK_GRAVITY_SOUTH:
      return XDG_POSITIONER_GRAVITY_TOP;
    case CDK_GRAVITY_SOUTH_EAST:
      return XDG_POSITIONER_GRAVITY_TOP_LEFT;
    default:
      g_assert_not_reached ();
    }
}

static enum zxdg_positioner_v6_anchor
rect_anchor_to_anchor_legacy (CdkGravity rect_anchor)
{
  switch (rect_anchor)
    {
    case CDK_GRAVITY_NORTH_WEST:
    case CDK_GRAVITY_STATIC:
      return (ZXDG_POSITIONER_V6_ANCHOR_TOP |
              ZXDG_POSITIONER_V6_ANCHOR_LEFT);
    case CDK_GRAVITY_NORTH:
      return ZXDG_POSITIONER_V6_ANCHOR_TOP;
    case CDK_GRAVITY_NORTH_EAST:
      return (ZXDG_POSITIONER_V6_ANCHOR_TOP |
              ZXDG_POSITIONER_V6_ANCHOR_RIGHT);
    case CDK_GRAVITY_WEST:
      return ZXDG_POSITIONER_V6_ANCHOR_LEFT;
    case CDK_GRAVITY_CENTER:
      return ZXDG_POSITIONER_V6_ANCHOR_NONE;
    case CDK_GRAVITY_EAST:
      return ZXDG_POSITIONER_V6_ANCHOR_RIGHT;
    case CDK_GRAVITY_SOUTH_WEST:
      return (ZXDG_POSITIONER_V6_ANCHOR_BOTTOM |
              ZXDG_POSITIONER_V6_ANCHOR_LEFT);
    case CDK_GRAVITY_SOUTH:
      return ZXDG_POSITIONER_V6_ANCHOR_BOTTOM;
    case CDK_GRAVITY_SOUTH_EAST:
      return (ZXDG_POSITIONER_V6_ANCHOR_BOTTOM |
              ZXDG_POSITIONER_V6_ANCHOR_RIGHT);
    default:
      g_assert_not_reached ();
    }
}

static enum zxdg_positioner_v6_gravity
window_anchor_to_gravity_legacy (CdkGravity rect_anchor)
{
  switch (rect_anchor)
    {
    case CDK_GRAVITY_NORTH_WEST:
    case CDK_GRAVITY_STATIC:
      return (ZXDG_POSITIONER_V6_GRAVITY_BOTTOM |
              ZXDG_POSITIONER_V6_GRAVITY_RIGHT);
    case CDK_GRAVITY_NORTH:
      return ZXDG_POSITIONER_V6_GRAVITY_BOTTOM;
    case CDK_GRAVITY_NORTH_EAST:
      return (ZXDG_POSITIONER_V6_GRAVITY_BOTTOM |
              ZXDG_POSITIONER_V6_GRAVITY_LEFT);
    case CDK_GRAVITY_WEST:
      return ZXDG_POSITIONER_V6_GRAVITY_RIGHT;
    case CDK_GRAVITY_CENTER:
      return ZXDG_POSITIONER_V6_GRAVITY_NONE;
    case CDK_GRAVITY_EAST:
      return ZXDG_POSITIONER_V6_GRAVITY_LEFT;
    case CDK_GRAVITY_SOUTH_WEST:
      return (ZXDG_POSITIONER_V6_GRAVITY_TOP |
              ZXDG_POSITIONER_V6_GRAVITY_RIGHT);
    case CDK_GRAVITY_SOUTH:
      return ZXDG_POSITIONER_V6_GRAVITY_TOP;
    case CDK_GRAVITY_SOUTH_EAST:
      return (ZXDG_POSITIONER_V6_GRAVITY_TOP |
              ZXDG_POSITIONER_V6_GRAVITY_LEFT);
    default:
      g_assert_not_reached ();
    }
}

static void
kwin_server_decoration_mode_set (void *data, struct org_kde_kwin_server_decoration *org_kde_kwin_server_decoration, uint32_t mode)
{
  CdkWindow *window = CDK_WINDOW (data);
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  if ((mode == ORG_KDE_KWIN_SERVER_DECORATION_MODE_SERVER && impl->using_csd) ||
        (mode == ORG_KDE_KWIN_SERVER_DECORATION_MODE_CLIENT && !impl->using_csd))
    cdk_wayland_window_announce_decoration_mode (window);
}

static const struct org_kde_kwin_server_decoration_listener kwin_server_decoration_listener = {
  kwin_server_decoration_mode_set
};

static void
cdk_wayland_window_announce_decoration_mode (CdkWindow *window)
{
  CdkWaylandDisplay *display_wayland = CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  if (!display_wayland->server_decoration_manager)
    return;
  if (!impl->display_server.server_decoration)
    {
      impl->display_server.server_decoration =
        org_kde_kwin_server_decoration_manager_create (display_wayland->server_decoration_manager,
                                                                                         impl->display_server.wl_surface);
      org_kde_kwin_server_decoration_add_listener (impl->display_server.server_decoration,
                                                                                &kwin_server_decoration_listener,
                                                                                window);
  }

  if (impl->display_server.server_decoration)
    {
      if (impl->using_csd)
        org_kde_kwin_server_decoration_request_mode (impl->display_server.server_decoration,
                                                                                     ORG_KDE_KWIN_SERVER_DECORATION_MODE_CLIENT);
      else
        org_kde_kwin_server_decoration_request_mode (impl->display_server.server_decoration,
                                                                                     ORG_KDE_KWIN_SERVER_DECORATION_MODE_SERVER);
    }
}

void
cdk_wayland_window_announce_csd (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  impl->using_csd = TRUE;
  if (impl->mapped)
    cdk_wayland_window_announce_decoration_mode (window);
}

void
cdk_wayland_window_announce_ssd (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  impl->using_csd = FALSE;
  if (impl->mapped)
    cdk_wayland_window_announce_decoration_mode (window);
}

static CdkWindow *
get_real_parent_and_translate (CdkWindow *window,
                               gint      *x,
                               gint      *y)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  CdkWindow *parent = impl->transient_for;

  while (parent)
    {
      CdkWindowImplWayland *parent_impl =
        CDK_WINDOW_IMPL_WAYLAND (parent->impl);
      CdkWindow *effective_parent = cdk_window_get_effective_parent (parent);

      if (parent == NULL || (cdk_window_has_native (parent) &&
           !parent_impl->display_server.wl_subsurface) ||
          !effective_parent)
        break;

      *x += parent->x;
      *y += parent->y;

      if (cdk_window_has_native (parent) &&
          parent_impl->display_server.wl_subsurface)
        parent = parent->transient_for;
      else
        parent = effective_parent;
    }

  return parent;
}

static void
translate_to_real_parent_window_geometry (CdkWindow  *window,
                                          gint       *x,
                                          gint       *y)
{
  CdkWindow *parent;

  parent = get_real_parent_and_translate (window, x, y);

  if (parent != NULL) {
    *x -= parent->shadow_left;
    *y -= parent->shadow_top;
  }
}

static CdkWindow *
translate_from_real_parent_window_geometry (CdkWindow *window,
                                            gint      *x,
                                            gint      *y)
{
  CdkWindow *parent;
  gint dx = 0;
  gint dy = 0;

  parent = get_real_parent_and_translate (window, &dx, &dy);

  *x -= dx;
  *y -= dy;

  if (parent != NULL) {
    *x += parent->shadow_left;
    *y += parent->shadow_top;
  }

  return parent;
}

static void
calculate_popup_rect (CdkWindow    *window,
                      CdkGravity    rect_anchor,
                      CdkGravity    window_anchor,
                      CdkRectangle *out_rect)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  CdkRectangle geometry;
  CdkRectangle anchor_rect;
  int x = 0, y = 0;

  cdk_wayland_window_get_window_geometry (window, &geometry);

  anchor_rect = (CdkRectangle) {
    .x = (impl->pending_move_to_rect.rect.x +
          impl->pending_move_to_rect.rect_anchor_dx),
    .y = (impl->pending_move_to_rect.rect.y +
          impl->pending_move_to_rect.rect_anchor_dy),
    .width = impl->pending_move_to_rect.rect.width,
    .height = impl->pending_move_to_rect.rect.height
  };

  switch (rect_anchor)
    {
    case CDK_GRAVITY_STATIC:
    case CDK_GRAVITY_NORTH_WEST:
      x = anchor_rect.x;
      y = anchor_rect.y;
      break;
    case CDK_GRAVITY_NORTH:
      x = anchor_rect.x + (anchor_rect.width / 2);
      y = anchor_rect.y;
      break;
    case CDK_GRAVITY_NORTH_EAST:
      x = anchor_rect.x + anchor_rect.width;
      y = anchor_rect.y;
      break;
    case CDK_GRAVITY_WEST:
      x = anchor_rect.x;
      y = anchor_rect.y + (anchor_rect.height / 2);
      break;
    case CDK_GRAVITY_CENTER:
      x = anchor_rect.x + (anchor_rect.width / 2);
      y = anchor_rect.y + (anchor_rect.height / 2);
      break;
    case CDK_GRAVITY_EAST:
      x = anchor_rect.x + anchor_rect.width;
      y = anchor_rect.y + (anchor_rect.height / 2);
      break;
    case CDK_GRAVITY_SOUTH_WEST:
      x = anchor_rect.x;
      y = anchor_rect.y + anchor_rect.height;
      break;
    case CDK_GRAVITY_SOUTH:
      x = anchor_rect.x + (anchor_rect.width / 2);
      y = anchor_rect.y + anchor_rect.height;
      break;
    case CDK_GRAVITY_SOUTH_EAST:
      x = anchor_rect.x + anchor_rect.width;
      y = anchor_rect.y + anchor_rect.height;
      break;
    }

  switch (window_anchor)
    {
    case CDK_GRAVITY_STATIC:
    case CDK_GRAVITY_NORTH_WEST:
      break;
    case CDK_GRAVITY_NORTH:
      x -= geometry.width / 2;
      break;
    case CDK_GRAVITY_NORTH_EAST:
      x -= geometry.width;
      break;
    case CDK_GRAVITY_WEST:
      y -= geometry.height / 2;
      break;
    case CDK_GRAVITY_CENTER:
      x -= geometry.width / 2;
      y -= geometry.height / 2;
      break;
    case CDK_GRAVITY_EAST:
      x -= geometry.width;
      y -= geometry.height / 2;
      break;
    case CDK_GRAVITY_SOUTH_WEST:
      y -= geometry.height;
      break;
    case CDK_GRAVITY_SOUTH:
      x -= geometry.width / 2;
      y -= geometry.height;
      break;
    case CDK_GRAVITY_SOUTH_EAST:
      x -= geometry.width;
      y -= geometry.height;
      break;
    }

  *out_rect = (CdkRectangle) {
    .x = x,
    .y = y,
    .width = geometry.width,
    .height = geometry.height
  };
}

static CdkGravity
flip_anchor_horizontally (CdkGravity anchor)
{
  switch (anchor)
    {
    case CDK_GRAVITY_STATIC:
    case CDK_GRAVITY_NORTH_WEST:
      return CDK_GRAVITY_NORTH_EAST;
    case CDK_GRAVITY_NORTH:
      return CDK_GRAVITY_NORTH;
    case CDK_GRAVITY_NORTH_EAST:
      return CDK_GRAVITY_NORTH_WEST;
    case CDK_GRAVITY_WEST:
      return CDK_GRAVITY_EAST;
    case CDK_GRAVITY_CENTER:
      return CDK_GRAVITY_CENTER;
    case CDK_GRAVITY_EAST:
      return CDK_GRAVITY_WEST;
    case CDK_GRAVITY_SOUTH_WEST:
      return CDK_GRAVITY_SOUTH_EAST;
    case CDK_GRAVITY_SOUTH:
      return CDK_GRAVITY_SOUTH;
    case CDK_GRAVITY_SOUTH_EAST:
      return CDK_GRAVITY_SOUTH_WEST;
    }

  g_assert_not_reached ();
}

static CdkGravity
flip_anchor_vertically (CdkGravity anchor)
{
  switch (anchor)
    {
    case CDK_GRAVITY_STATIC:
    case CDK_GRAVITY_NORTH_WEST:
      return CDK_GRAVITY_SOUTH_WEST;
    case CDK_GRAVITY_NORTH:
      return CDK_GRAVITY_SOUTH;
    case CDK_GRAVITY_NORTH_EAST:
      return CDK_GRAVITY_SOUTH_EAST;
    case CDK_GRAVITY_WEST:
      return CDK_GRAVITY_WEST;
    case CDK_GRAVITY_CENTER:
      return CDK_GRAVITY_CENTER;
    case CDK_GRAVITY_EAST:
      return CDK_GRAVITY_EAST;
    case CDK_GRAVITY_SOUTH_WEST:
      return CDK_GRAVITY_NORTH_WEST;
    case CDK_GRAVITY_SOUTH:
      return CDK_GRAVITY_NORTH;
    case CDK_GRAVITY_SOUTH_EAST:
      return CDK_GRAVITY_NORTH_EAST;
    }

  g_assert_not_reached ();
}

static void
calculate_moved_to_rect_result (CdkWindow    *window,
                                int           x,
                                int           y,
                                int           width,
                                int           height,
                                CdkRectangle *flipped_rect,
                                CdkRectangle *final_rect,
                                gboolean     *flipped_x,
                                gboolean     *flipped_y)
{
  CdkWindowImplWayland *impl;
  CdkWindow *parent;
  gint window_x, window_y;
  gint window_width, window_height;
  CdkRectangle best_rect;

  g_return_if_fail (CDK_IS_WAYLAND_WINDOW (window));
  g_return_if_fail (CDK_IS_WINDOW_IMPL_WAYLAND (window->impl));

  impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  parent = translate_from_real_parent_window_geometry (window, &x, &y);
  *final_rect = (CdkRectangle) {
    .x = x,
    .y = y,
    .width = width,
    .height = height,
  };

  window_x = parent->x + x;
  window_y = parent->y + y;
  window_width = width + window->shadow_left + window->shadow_right;
  window_height = height + window->shadow_top + window->shadow_bottom;

  impl->configuring_popup = TRUE;
  cdk_window_move_resize (window,
                          window_x, window_y,
                          window_width, window_height);
  impl->configuring_popup = FALSE;

  calculate_popup_rect (window,
                        impl->pending_move_to_rect.rect_anchor,
                        impl->pending_move_to_rect.window_anchor,
                        &best_rect);

  *flipped_rect = best_rect;

  if (x != best_rect.x &&
      impl->pending_move_to_rect.anchor_hints & CDK_ANCHOR_FLIP_X)
    {
      CdkRectangle flipped_x_rect;
      CdkGravity flipped_rect_anchor;
      CdkGravity flipped_window_anchor;

      flipped_rect_anchor =
        flip_anchor_horizontally (impl->pending_move_to_rect.rect_anchor);
      flipped_window_anchor =
        flip_anchor_horizontally (impl->pending_move_to_rect.window_anchor),
      calculate_popup_rect (window,
                            flipped_rect_anchor,
                            flipped_window_anchor,
                            &flipped_x_rect);

      if (flipped_x_rect.x == x)
        flipped_rect->x = x;
    }
  if (y != best_rect.y &&
      impl->pending_move_to_rect.anchor_hints & CDK_ANCHOR_FLIP_Y)
    {
      CdkRectangle flipped_y_rect;
      CdkGravity flipped_rect_anchor;
      CdkGravity flipped_window_anchor;

      flipped_rect_anchor =
        flip_anchor_vertically (impl->pending_move_to_rect.rect_anchor);
      flipped_window_anchor =
        flip_anchor_vertically (impl->pending_move_to_rect.window_anchor),
      calculate_popup_rect (window,
                            flipped_rect_anchor,
                            flipped_window_anchor,
                            &flipped_y_rect);

      if (flipped_y_rect.y == y)
        flipped_rect->y = y;
    }

  *flipped_x = flipped_rect->x != best_rect.x;
  *flipped_y = flipped_rect->y != best_rect.y;
}

static gpointer
create_dynamic_positioner (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  CdkWaylandDisplay *display =
    CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));
  CdkRectangle geometry;
  uint32_t constraint_adjustment = ZXDG_POSITIONER_V6_CONSTRAINT_ADJUSTMENT_NONE;
  gint real_anchor_rect_x, real_anchor_rect_y;
  gint anchor_rect_width, anchor_rect_height;

  cdk_wayland_window_get_window_geometry (window, &geometry);

  real_anchor_rect_x = impl->pending_move_to_rect.rect.x;
  real_anchor_rect_y = impl->pending_move_to_rect.rect.y;
  translate_to_real_parent_window_geometry (window,
                                            &real_anchor_rect_x,
                                            &real_anchor_rect_y);

  anchor_rect_width = impl->pending_move_to_rect.rect.width;
  anchor_rect_height = impl->pending_move_to_rect.rect.height;

  switch (display->shell_variant)
    {
    case CDK_WAYLAND_SHELL_VARIANT_XDG_SHELL:
      {
        struct xdg_positioner *positioner;
        enum xdg_positioner_anchor anchor;
        enum xdg_positioner_gravity gravity;

        positioner = xdg_wm_base_create_positioner (display->xdg_wm_base);

        xdg_positioner_set_size (positioner, geometry.width, geometry.height);
        xdg_positioner_set_anchor_rect (positioner,
                                        real_anchor_rect_x,
                                        real_anchor_rect_y,
                                        anchor_rect_width,
                                        anchor_rect_height);
        xdg_positioner_set_offset (positioner,
                                   impl->pending_move_to_rect.rect_anchor_dx,
                                   impl->pending_move_to_rect.rect_anchor_dy);

        anchor = rect_anchor_to_anchor (impl->pending_move_to_rect.rect_anchor);
        xdg_positioner_set_anchor (positioner, anchor);

        gravity = window_anchor_to_gravity (impl->pending_move_to_rect.window_anchor);
        xdg_positioner_set_gravity (positioner, gravity);

        if (impl->pending_move_to_rect.anchor_hints & CDK_ANCHOR_FLIP_X)
          constraint_adjustment |= XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_FLIP_X;
        if (impl->pending_move_to_rect.anchor_hints & CDK_ANCHOR_FLIP_Y)
          constraint_adjustment |= XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_FLIP_Y;
        if (impl->pending_move_to_rect.anchor_hints & CDK_ANCHOR_SLIDE_X)
          constraint_adjustment |= XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_X;
        if (impl->pending_move_to_rect.anchor_hints & CDK_ANCHOR_SLIDE_Y)
          constraint_adjustment |= XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_Y;
        if (impl->pending_move_to_rect.anchor_hints & CDK_ANCHOR_RESIZE_X)
          constraint_adjustment |= XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_RESIZE_X;
        if (impl->pending_move_to_rect.anchor_hints & CDK_ANCHOR_RESIZE_Y)
          constraint_adjustment |= XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_RESIZE_Y;
        xdg_positioner_set_constraint_adjustment (positioner,
                                                  constraint_adjustment);

        return positioner;
      }
    case CDK_WAYLAND_SHELL_VARIANT_ZXDG_SHELL_V6:
      {
        struct zxdg_positioner_v6 *positioner;
        enum zxdg_positioner_v6_anchor anchor;
        enum zxdg_positioner_v6_gravity gravity;

        positioner = zxdg_shell_v6_create_positioner (display->zxdg_shell_v6);

        zxdg_positioner_v6_set_size (positioner, geometry.width, geometry.height);
        zxdg_positioner_v6_set_anchor_rect (positioner,
                                            real_anchor_rect_x,
                                            real_anchor_rect_y,
                                            anchor_rect_width,
                                            anchor_rect_height);
        zxdg_positioner_v6_set_offset (positioner,
                                       impl->pending_move_to_rect.rect_anchor_dx,
                                       impl->pending_move_to_rect.rect_anchor_dy);

        anchor = rect_anchor_to_anchor_legacy (impl->pending_move_to_rect.rect_anchor);
        zxdg_positioner_v6_set_anchor (positioner, anchor);

        gravity = window_anchor_to_gravity_legacy (impl->pending_move_to_rect.window_anchor);
        zxdg_positioner_v6_set_gravity (positioner, gravity);

        if (impl->pending_move_to_rect.anchor_hints & CDK_ANCHOR_FLIP_X)
          constraint_adjustment |= ZXDG_POSITIONER_V6_CONSTRAINT_ADJUSTMENT_FLIP_X;
        if (impl->pending_move_to_rect.anchor_hints & CDK_ANCHOR_FLIP_Y)
          constraint_adjustment |= ZXDG_POSITIONER_V6_CONSTRAINT_ADJUSTMENT_FLIP_Y;
        if (impl->pending_move_to_rect.anchor_hints & CDK_ANCHOR_SLIDE_X)
          constraint_adjustment |= ZXDG_POSITIONER_V6_CONSTRAINT_ADJUSTMENT_SLIDE_X;
        if (impl->pending_move_to_rect.anchor_hints & CDK_ANCHOR_SLIDE_Y)
          constraint_adjustment |= ZXDG_POSITIONER_V6_CONSTRAINT_ADJUSTMENT_SLIDE_Y;
        if (impl->pending_move_to_rect.anchor_hints & CDK_ANCHOR_RESIZE_X)
          constraint_adjustment |= ZXDG_POSITIONER_V6_CONSTRAINT_ADJUSTMENT_RESIZE_X;
        if (impl->pending_move_to_rect.anchor_hints & CDK_ANCHOR_RESIZE_Y)
          constraint_adjustment |= ZXDG_POSITIONER_V6_CONSTRAINT_ADJUSTMENT_RESIZE_Y;
        zxdg_positioner_v6_set_constraint_adjustment (positioner,
                                                      constraint_adjustment);

        return positioner;
      }
    }

  g_assert_not_reached ();
}

static gpointer
create_simple_positioner (CdkWindow *window,
                          CdkWindow *parent)
{
  CdkWaylandDisplay *display =
    CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));
  CdkRectangle geometry;
  CdkRectangle parent_geometry;
  int parent_x, parent_y;

  cdk_wayland_window_get_window_geometry (window, &geometry);

  parent_x = parent->x;
  parent_y = parent->y;

  cdk_wayland_window_get_window_geometry (parent, &parent_geometry);
  parent_x += parent_geometry.x;
  parent_y += parent_geometry.y;

  switch (display->shell_variant)
    {
    case CDK_WAYLAND_SHELL_VARIANT_XDG_SHELL:
      {
        struct xdg_positioner *positioner;

        positioner = xdg_wm_base_create_positioner (display->xdg_wm_base);
        xdg_positioner_set_size (positioner, geometry.width, geometry.height);
        xdg_positioner_set_anchor_rect (positioner,
                                        (window->x + geometry.x) - parent_x,
                                        (window->y + geometry.y) - parent_y,
                                        1, 1);
        xdg_positioner_set_anchor (positioner,
                                   XDG_POSITIONER_ANCHOR_TOP_LEFT);
        xdg_positioner_set_gravity (positioner,
                                    XDG_POSITIONER_GRAVITY_BOTTOM_RIGHT);

        return positioner;
      }
    case CDK_WAYLAND_SHELL_VARIANT_ZXDG_SHELL_V6:
      {
        struct zxdg_positioner_v6 *positioner;

        positioner = zxdg_shell_v6_create_positioner (display->zxdg_shell_v6);
        zxdg_positioner_v6_set_size (positioner, geometry.width, geometry.height);
        zxdg_positioner_v6_set_anchor_rect (positioner,
                                            (window->x + geometry.x) - parent_x,
                                            (window->y + geometry.y) - parent_y,
                                            1, 1);
        zxdg_positioner_v6_set_anchor (positioner,
                                       (ZXDG_POSITIONER_V6_ANCHOR_TOP |
                                        ZXDG_POSITIONER_V6_ANCHOR_LEFT));
        zxdg_positioner_v6_set_gravity (positioner,
                                        (ZXDG_POSITIONER_V6_GRAVITY_BOTTOM |
                                         ZXDG_POSITIONER_V6_GRAVITY_RIGHT));

        return positioner;
      }
    }

  g_assert_not_reached ();
}

static void
cdk_wayland_window_create_xdg_popup (CdkWindow      *window,
                                     CdkWindow      *parent,
                                     struct wl_seat *seat)
{
  CdkWaylandDisplay *display = CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  CdkWindowImplWayland *parent_impl = CDK_WINDOW_IMPL_WAYLAND (parent->impl);
  gpointer positioner;
  guint32 serial;

  if (!impl->display_server.wl_surface)
    return;

  if (!is_realized_shell_surface (parent))
    return;

  if (is_realized_toplevel (window))
    {
      g_warning ("Can't map popup, already mapped as toplevel");
      return;
    }
  if (is_realized_popup (window))
    {
      g_warning ("Can't map popup, already mapped");
      return;
    }
  if ((display->current_popups &&
       g_list_last (display->current_popups)->data != parent) ||
      (!display->current_popups &&
       !is_realized_toplevel (parent)))
    {
      g_warning ("Tried to map a popup with a non-top most parent");
      return;
    }

  cdk_window_freeze_updates (window);

  if (impl->position_method == POSITION_METHOD_MOVE_TO_RECT)
    positioner = create_dynamic_positioner (window);
  else
    positioner = create_simple_positioner (window, parent);

  switch (display->shell_variant)
    {
    case CDK_WAYLAND_SHELL_VARIANT_XDG_SHELL:
      impl->display_server.xdg_surface =
        xdg_wm_base_get_xdg_surface (display->xdg_wm_base,
                                     impl->display_server.wl_surface);
      xdg_surface_add_listener (impl->display_server.xdg_surface,
                                &xdg_surface_listener,
                                window);
      impl->display_server.xdg_popup =
        xdg_surface_get_popup (impl->display_server.xdg_surface,
                               parent_impl->display_server.xdg_surface,
                               positioner);
      xdg_popup_add_listener (impl->display_server.xdg_popup,
                              &xdg_popup_listener,
                              window);
      xdg_positioner_destroy (positioner);
      break;
    case CDK_WAYLAND_SHELL_VARIANT_ZXDG_SHELL_V6:
      impl->display_server.zxdg_surface_v6 =
        zxdg_shell_v6_get_xdg_surface (display->zxdg_shell_v6,
                                       impl->display_server.wl_surface);
      zxdg_surface_v6_add_listener (impl->display_server.zxdg_surface_v6,
                                    &zxdg_surface_v6_listener,
                                    window);
      impl->display_server.zxdg_popup_v6 =
        zxdg_surface_v6_get_popup (impl->display_server.zxdg_surface_v6,
                                   parent_impl->display_server.zxdg_surface_v6,
                                   positioner);
      zxdg_popup_v6_add_listener (impl->display_server.zxdg_popup_v6,
                                  &zxdg_popup_v6_listener,
                                  window);
      zxdg_positioner_v6_destroy (positioner);
      break;
    }

  if (seat)
    {
      CdkSeat *cdk_seat;

      cdk_seat = cdk_display_get_default_seat (CDK_DISPLAY (display));
      serial = _cdk_wayland_seat_get_last_implicit_grab_serial (cdk_seat, NULL);

      switch (display->shell_variant)
        {
        case CDK_WAYLAND_SHELL_VARIANT_XDG_SHELL:
          xdg_popup_grab (impl->display_server.xdg_popup, seat, serial);
          break;
        case CDK_WAYLAND_SHELL_VARIANT_ZXDG_SHELL_V6:
          zxdg_popup_v6_grab (impl->display_server.zxdg_popup_v6, seat, serial);
          break;
        }
    }

  wl_surface_commit (impl->display_server.wl_surface);

  impl->popup_parent = parent;
  display->current_popups = g_list_append (display->current_popups, window);
}

static struct wl_seat *
find_grab_input_seat (CdkWindow *window, CdkWindow *transient_for)
{
  CdkWindow *attached_grab_window;
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  CdkWindowImplWayland *tmp_impl;

  /* Use the device that was used for the grab as the device for
   * the popup window setup - so this relies on CTK+ taking the
   * grab before showing the popup window.
   */
  if (impl->grab_input_seat)
    return cdk_wayland_seat_get_wl_seat (impl->grab_input_seat);

  /* HACK: CtkMenu grabs a special window known as the "grab transfer window"
   * and then transfers the grab over to the correct window later. Look for
   * this window when taking the grab to know it's correct.
   *
   * See: associate_menu_grab_transfer_window in ctkmenu.c
   */
  attached_grab_window = g_object_get_data (G_OBJECT (window), "cdk-attached-grab-window");
  if (attached_grab_window)
    {
      tmp_impl = CDK_WINDOW_IMPL_WAYLAND (attached_grab_window->impl);
      if (tmp_impl->grab_input_seat)
        return cdk_wayland_seat_get_wl_seat (tmp_impl->grab_input_seat);
    }

  while (transient_for)
    {
      tmp_impl = CDK_WINDOW_IMPL_WAYLAND (transient_for->impl);

      if (tmp_impl->grab_input_seat)
        return cdk_wayland_seat_get_wl_seat (tmp_impl->grab_input_seat);

      transient_for = tmp_impl->transient_for;
    }

  return NULL;
}

static gboolean
should_be_mapped (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  /* Don't map crazy temp that CTK+ uses for internal X11 shenanigans. */
  if (window->window_type == CDK_WINDOW_TEMP && window->x < 0 && window->y < 0)
    return FALSE;

  if (impl->hint == CDK_WINDOW_TYPE_HINT_DND)
    return FALSE;

  return TRUE;
}

static gboolean
should_map_as_popup (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  /* Ideally, popup would be temp windows with a parent and grab */
  if (CDK_WINDOW_TYPE (window) == CDK_WINDOW_TEMP)
    {
      /* If a temp window has a parent and a grab, we can use a popup */
      if (impl->transient_for)
        {
          if (impl->grab_input_seat)
            return TRUE;
        }
    }

  /* Yet we need to keep the window type hint tests for compatibility */
  switch (impl->hint)
    {
    case CDK_WINDOW_TYPE_HINT_POPUP_MENU:
    case CDK_WINDOW_TYPE_HINT_DROPDOWN_MENU:
    case CDK_WINDOW_TYPE_HINT_COMBO:
      return TRUE;

    default:
      break;
    }

  if (impl->position_method == POSITION_METHOD_MOVE_TO_RECT)
    return TRUE;

  return FALSE;
}

static gboolean
should_map_as_subsurface (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  if (CDK_WINDOW_TYPE (window) == CDK_WINDOW_SUBSURFACE)
    return TRUE;

  if (CDK_WINDOW_TYPE (window) != CDK_WINDOW_TEMP)
    return FALSE;

  /* if we want a popup, we do not want a subsurface */
  if (should_map_as_popup (window))
    return FALSE;

  if (impl->transient_for)
    {
      CdkWindowImplWayland *impl_parent;

      impl_parent = CDK_WINDOW_IMPL_WAYLAND (impl->transient_for->impl);
      /* subsurface require that the parent is mapped */
      if (impl_parent->mapped)
        return TRUE;
      else
        g_warning ("Couldn't map window %p as subsurface because its parent is not mapped.",
                   window);

    }

  return FALSE;
}

/* Get the window that can be used as a parent for a popup, i.e. a xdg_toplevel
 * or xdg_popup. If the window is not, traverse up the transiency parents until
 * we find one.
 */
static CdkWindow *
get_popup_parent (CdkWindow *window)
{
  while (window)
    {
      CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

      if (is_realized_popup (window) || is_realized_toplevel (window))
        return window;

      window = impl->transient_for;
    }

  return NULL;
}

static void
cdk_wayland_window_map (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  CdkWindow *transient_for = NULL;

  if (!should_be_mapped (window))
    return;

  if (impl->mapped || impl->use_custom_surface)
    return;

  if (CDK_WINDOW_TYPE (window) == CDK_WINDOW_TEMP)
    {
      if (!impl->transient_for)
        {
          g_message ("Window %p is a temporary window without parent, "
                     "application will not be able to position it on screen.",
                     window);
        }
    }

  if (should_map_as_subsurface (window))
    {
      if (impl->transient_for)
        cdk_wayland_window_create_subsurface (window);
      else
        g_warning ("Couldn't map window %p as susburface yet because it doesn't have a parent",
                   window);
    }
  else if (should_map_as_popup (window))
    {
      gboolean create_fallback = FALSE;
      struct wl_seat *grab_input_seat;

      /* Popup menus can appear without a transient parent, which means they
       * cannot be positioned properly on Wayland. This attempts to guess the
       * surface they should be positioned with by finding the surface beneath
       * the device that created the grab for the popup window.
       */
      if (!impl->transient_for && impl->hint == CDK_WINDOW_TYPE_HINT_POPUP_MENU)
        {
          CdkDevice *grab_device = NULL;

          /* The popup menu window is not the grabbed window. This may mean
           * that a "transfer window" (see ctkmenu.c) is used, and we need
           * to find that window to get the grab device. If so is the case
           * the "transfer window" can be retrieved via the
           * "cdk-attached-grab-window" associated data field.
           */
          if (!impl->grab_input_seat)
            {
              CdkWindow *attached_grab_window =
                g_object_get_data (G_OBJECT (window),
                                   "cdk-attached-grab-window");
              if (attached_grab_window)
                {
                  CdkWindowImplWayland *attached_impl =
                    CDK_WINDOW_IMPL_WAYLAND (attached_grab_window->impl);
                  grab_device = cdk_seat_get_pointer (attached_impl->grab_input_seat);
                  transient_for =
                    cdk_device_get_window_at_position (grab_device,
                                                       NULL, NULL);
                }
            }
          else
            {
              grab_device = cdk_seat_get_pointer (impl->grab_input_seat);
              transient_for =
                cdk_device_get_window_at_position (grab_device, NULL, NULL);
            }

          if (transient_for)
            transient_for = get_popup_parent (cdk_window_get_effective_toplevel (transient_for));

          /* If the position was not explicitly set, start the popup at the
           * position of the device that holds the grab.
           */
          if (impl->position_method == POSITION_METHOD_NONE && grab_device)
            cdk_window_get_device_position (transient_for, grab_device,
                                            &window->x, &window->y, NULL);
        }
      else
        {
          transient_for = cdk_window_get_effective_toplevel (impl->transient_for);
          transient_for = get_popup_parent (transient_for);
        }

      if (!transient_for)
        {
          g_warning ("Couldn't map as window %p as popup because it doesn't have a parent",
                     window);

          create_fallback = TRUE;
        }
      else
        {
          grab_input_seat = find_grab_input_seat (window, transient_for);
        }

      if (!create_fallback)
        {
          cdk_wayland_window_create_xdg_popup (window,
                                               transient_for,
                                               grab_input_seat);
        }
      else
        {
          cdk_wayland_window_create_xdg_toplevel (window);
          cdk_wayland_window_announce_decoration_mode (window);
        }
    }
  else
    {
      cdk_wayland_window_create_xdg_toplevel (window);
      cdk_wayland_window_announce_decoration_mode (window);
    }

  impl->mapped = TRUE;
}

static void
cdk_wayland_window_show (CdkWindow *window,
                         gboolean   already_mapped)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  if (!impl->display_server.wl_surface)
    cdk_wayland_window_create_surface (window);

  cdk_wayland_window_map (window);

  _cdk_make_event (window, CDK_MAP, NULL, FALSE);

  if (impl->staging_cairo_surface &&
      _cdk_wayland_is_shm_surface (impl->staging_cairo_surface))
    cdk_wayland_window_attach_image (window);
}

static void
unmap_subsurface (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  CdkWindowImplWayland *parent_impl;

  g_return_if_fail (impl->display_server.wl_subsurface);
  g_return_if_fail (impl->transient_for);

  parent_impl = CDK_WINDOW_IMPL_WAYLAND (impl->transient_for->impl);
  wl_subsurface_destroy (impl->display_server.wl_subsurface);
  if (impl->parent_surface_committed_handler)
    {
      g_signal_handler_disconnect (parent_impl,
                                   impl->parent_surface_committed_handler);
      impl->parent_surface_committed_handler = 0;
    }
  impl->display_server.wl_subsurface = NULL;
}

static void
unmap_popups_for_window (CdkWindow *window)
{
  CdkWaylandDisplay *display_wayland;
  GList *l;

  display_wayland = CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));
  for (l = display_wayland->current_popups; l; l = l->next)
    {
       CdkWindow *popup = l->data;
       CdkWindowImplWayland *popup_impl = CDK_WINDOW_IMPL_WAYLAND (popup->impl);

       if (popup_impl->popup_parent == window)
         {
           g_warning ("Tried to unmap the parent of a popup");
           cdk_window_hide (popup);

           return;
         }
    }
}

static void
cdk_wayland_window_hide_surface (CdkWindow *window)
{
  CdkWaylandDisplay *display_wayland = CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  unmap_popups_for_window (window);

  if (impl->display_server.wl_surface)
    {
      if (impl->dummy_egl_surface)
        {
          eglDestroySurface (display_wayland->egl_display, impl->dummy_egl_surface);
          impl->dummy_egl_surface = NULL;
        }

      if (impl->display_server.dummy_egl_window)
        {
          wl_egl_window_destroy (impl->display_server.dummy_egl_window);
          impl->display_server.dummy_egl_window = NULL;
        }

      if (impl->egl_surface)
        {
          eglDestroySurface (display_wayland->egl_display, impl->egl_surface);
          impl->egl_surface = NULL;
        }

      if (impl->display_server.egl_window)
        {
          wl_egl_window_destroy (impl->display_server.egl_window);
          impl->display_server.egl_window = NULL;
        }

      if (impl->display_server.xdg_toplevel)
        {
          xdg_toplevel_destroy (impl->display_server.xdg_toplevel);
          impl->display_server.xdg_toplevel = NULL;
        }
      else if (impl->display_server.xdg_popup)
        {
          xdg_popup_destroy (impl->display_server.xdg_popup);
          impl->display_server.xdg_popup = NULL;
          display_wayland->current_popups =
            g_list_remove (display_wayland->current_popups, window);
        }
      if (impl->display_server.xdg_surface)
        {
          xdg_surface_destroy (impl->display_server.xdg_surface);
          impl->display_server.xdg_surface = NULL;
          if (!impl->initial_configure_received)
            cdk_window_thaw_updates (window);
          else
            impl->initial_configure_received = FALSE;
        }

      if (impl->display_server.zxdg_toplevel_v6)
        {
          zxdg_toplevel_v6_destroy (impl->display_server.zxdg_toplevel_v6);
          impl->display_server.zxdg_toplevel_v6 = NULL;
        }
      else if (impl->display_server.zxdg_popup_v6)
        {
          zxdg_popup_v6_destroy (impl->display_server.zxdg_popup_v6);
          impl->display_server.zxdg_popup_v6 = NULL;
          display_wayland->current_popups =
            g_list_remove (display_wayland->current_popups, window);
        }
      if (impl->display_server.zxdg_surface_v6)
        {
          zxdg_surface_v6_destroy (impl->display_server.zxdg_surface_v6);
          impl->display_server.zxdg_surface_v6 = NULL;
          if (!impl->initial_configure_received)
            cdk_window_thaw_updates (window);
          else
            impl->initial_configure_received = FALSE;
        }

      if (impl->display_server.wl_subsurface)
        unmap_subsurface (window);

      if (impl->awaiting_frame)
        {
          CdkFrameClock *frame_clock;

          impl->awaiting_frame = FALSE;
          frame_clock = cdk_window_get_frame_clock (window);
          if (frame_clock)
            _cdk_frame_clock_thaw (frame_clock);
        }

      if (impl->display_server.ctk_surface)
        {
          ctk_surface1_destroy (impl->display_server.ctk_surface);
          impl->display_server.ctk_surface = NULL;
          impl->application.was_set = FALSE;
        }

      if (impl->display_server.server_decoration)
        {
          org_kde_kwin_server_decoration_release (impl->display_server.server_decoration);
          impl->display_server.server_decoration = NULL;
        }

      wl_surface_destroy (impl->display_server.wl_surface);
      impl->display_server.wl_surface = NULL;

      g_slist_free (impl->display_server.outputs);
      impl->display_server.outputs = NULL;

      if (impl->hint == CDK_WINDOW_TYPE_HINT_DIALOG && !impl->transient_for)
        display_wayland->orphan_dialogs =
          g_list_remove (display_wayland->orphan_dialogs, window);
    }

  unset_transient_for_exported (window);

  _cdk_wayland_window_clear_saved_size (window);
  drop_cairo_surfaces (window);
  impl->pending_commit = FALSE;
  impl->mapped = FALSE;
}

static void
cdk_wayland_window_hide (CdkWindow *window)
{
  cdk_wayland_window_hide_surface (window);
  _cdk_window_clear_update_area (window);
}

static void
cdk_window_wayland_withdraw (CdkWindow *window)
{
  if (!window->destroyed)
    {
      if (CDK_WINDOW_IS_MAPPED (window))
        cdk_synthesize_window_state (window, 0, CDK_WINDOW_STATE_WITHDRAWN);

      g_assert (!CDK_WINDOW_IS_MAPPED (window));

      cdk_wayland_window_hide_surface (window);
    }
}

static void
cdk_window_wayland_set_events (CdkWindow    *window,
                               CdkEventMask  event_mask)
{
  CDK_WINDOW (window)->event_mask = event_mask;
}

static CdkEventMask
cdk_window_wayland_get_events (CdkWindow *window)
{
  if (CDK_WINDOW_DESTROYED (window))
    return 0;
  else
    return CDK_WINDOW (window)->event_mask;
}

static void
cdk_window_wayland_raise (CdkWindow *window)
{
}

static void
cdk_window_wayland_lower (CdkWindow *window)
{
}

static void
cdk_window_wayland_restack_under (CdkWindow *window,
                                  GList     *native_siblings)
{
}

static void
cdk_window_wayland_restack_toplevel (CdkWindow *window,
                                     CdkWindow *sibling,
                                     gboolean   above)
{
}

static void
cdk_window_request_transient_parent_commit (CdkWindow *window)
{
  CdkWindowImplWayland *window_impl, *impl;
  CdkFrameClock *frame_clock;

  window_impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  if (!window_impl->transient_for)
    return;

  impl = CDK_WINDOW_IMPL_WAYLAND (window_impl->transient_for->impl);

  if (!impl->display_server.wl_surface || impl->pending_commit)
    return;

  frame_clock = cdk_window_get_frame_clock (window_impl->transient_for);

  if (!frame_clock)
    return;

  impl->pending_commit = TRUE;
  cdk_frame_clock_request_phase (frame_clock,
                                 CDK_FRAME_CLOCK_PHASE_AFTER_PAINT);
}

static void
cdk_window_wayland_move_resize (CdkWindow *window,
                                gboolean   with_move,
                                gint       x,
                                gint       y,
                                gint       width,
                                gint       height)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  if (with_move)
    {
      /* Each toplevel has in its own "root" coordinate system */
      if (CDK_WINDOW_TYPE (window) != CDK_WINDOW_TOPLEVEL)
        {
          window->x = x;
          window->y = y;
          impl->position_method = POSITION_METHOD_MOVE_RESIZE;

          if (impl->display_server.wl_subsurface &&
	      (x + window->abs_x != impl->subsurface_x ||
	       y + window->abs_y != impl->subsurface_y))
            {
              cdk_wayland_window_set_subsurface_position (window,
                                                          x + window->abs_x,
                                                          y + window->abs_y);
            }
        }
    }

  if (window->state & (CDK_WINDOW_STATE_FULLSCREEN |
                       CDK_WINDOW_STATE_MAXIMIZED |
                       CDK_WINDOW_STATE_TILED))
    {
      impl->saved_width = width;
      impl->saved_height = height;
      impl->saved_size_changed = (width > 0 && height > 0);
    }

  /* If this function is called with width and height = -1 then that means
   * just move the window - don't update its size
   */
  if (width > 0 && height > 0)
    {
      if (!should_use_fixed_size (window->state) ||
          (width == impl->fixed_size_width &&
           height == impl->fixed_size_height))
        {
          cdk_wayland_window_maybe_configure (window,
                                              width,
                                              height,
                                              impl->scale);
        }
      else if (!should_inhibit_resize (window))
        {
          cdk_wayland_window_configure (window,
                                        window->width,
                                        window->height,
                                        impl->scale);
        }
    }
}

/* Avoid zero width/height as this is a protocol error */
static void
sanitize_anchor_rect (CdkWindow    *window,
                      CdkRectangle *rect)
{
  gint original_width = rect->width;
  gint original_height = rect->height;

  rect->width  = MAX (1, rect->width);
  rect->height = MAX (1, rect->height);
  rect->x = MAX (rect->x + original_width - rect->width, 0);
  rect->y = MAX (rect->y + original_height - rect->height, 0);
}

static void
cdk_window_wayland_move_to_rect (CdkWindow          *window,
                                 const CdkRectangle *rect,
                                 CdkGravity          rect_anchor,
                                 CdkGravity          window_anchor,
                                 CdkAnchorHints      anchor_hints,
                                 gint                rect_anchor_dx,
                                 gint                rect_anchor_dy)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  impl->pending_move_to_rect.rect = *rect;
  sanitize_anchor_rect (window, &impl->pending_move_to_rect.rect);

  impl->pending_move_to_rect.rect_anchor = rect_anchor;
  impl->pending_move_to_rect.window_anchor = window_anchor;
  impl->pending_move_to_rect.anchor_hints = anchor_hints;
  impl->pending_move_to_rect.rect_anchor_dx = rect_anchor_dx;
  impl->pending_move_to_rect.rect_anchor_dy = rect_anchor_dy;

  impl->position_method = POSITION_METHOD_MOVE_TO_RECT;
}

static void
cdk_window_wayland_set_background (CdkWindow       *window,
                                   cairo_pattern_t *pattern)
{
}

static gboolean
cdk_window_wayland_reparent (CdkWindow *window,
                             CdkWindow *new_parent,
                             gint       x,
                             gint       y)
{
  return FALSE;
}

static void
cdk_window_wayland_set_device_cursor (CdkWindow *window,
                                      CdkDevice *device,
                                      CdkCursor *cursor)
{
  g_return_if_fail (CDK_IS_WINDOW (window));
  g_return_if_fail (CDK_IS_DEVICE (device));

  if (!CDK_WINDOW_DESTROYED (window))
    CDK_DEVICE_GET_CLASS (device)->set_window_cursor (device, window, cursor);
}

static void
cdk_window_wayland_get_geometry (CdkWindow *window,
                                 gint      *x,
                                 gint      *y,
                                 gint      *width,
                                 gint      *height)
{
  if (!CDK_WINDOW_DESTROYED (window))
    {
      if (x)
        *x = window->x;
      if (y)
        *y = window->y;
      if (width)
        *width = window->width;
      if (height)
        *height = window->height;
    }
}

static void
cdk_window_wayland_get_root_coords (CdkWindow *window,
                                    gint       x,
                                    gint       y,
                                    gint      *root_x,
                                    gint      *root_y)
{
  /*
   * Wayland does not have a global coordinate space shared between surfaces. In
   * fact, for regular toplevels, we have no idea where our surfaces are
   * positioned, relatively.
   *
   * However, there are some cases like popups and subsurfaces where we do have
   * some amount of control over the placement of our window, and we can
   * semi-accurately control the x/y position of these windows, if they are
   * relative to another surface.
   *
   * To pretend we have something called a root coordinate space, assume all
   * parent-less windows are positioned in (0, 0), and all relative positioned
   * popups and subsurfaces are placed within this fake root coordinate space.
   *
   * For example a 200x200 large toplevel window will have the position (0, 0).
   * If a popup positioned in the middle of the toplevel will have the fake
   * position (100,100). Furthermore, if a positioned is placed in the middle
   * that popup, will have the fake position (150,150), even though it has the
   * relative position (50,50). These three windows would make up one single
   * fake root coordinate space.
   */

  if (root_x)
    *root_x = window->x + x;

  if (root_y)
    *root_y = window->y + y;
}

static gboolean
cdk_window_wayland_get_device_state (CdkWindow       *window,
                                     CdkDevice       *device,
                                     gdouble         *x,
                                     gdouble         *y,
                                     CdkModifierType *mask)
{
  gboolean return_val;

  g_return_val_if_fail (window == NULL || CDK_IS_WINDOW (window), FALSE);

  return_val = TRUE;

  if (!CDK_WINDOW_DESTROYED (window))
    {
      CdkWindow *child;

      CDK_DEVICE_GET_CLASS (device)->query_state (device, window,
                                                  NULL, &child,
                                                  NULL, NULL,
                                                  x, y, mask);
      return_val = (child != NULL);
    }

  return return_val;
}

static void
cdk_window_wayland_shape_combine_region (CdkWindow            *window,
                                         const cairo_region_t *shape_region,
                                         gint                  offset_x,
                                         gint                  offset_y)
{
}

static void
cdk_window_wayland_input_shape_combine_region (CdkWindow            *window,
                                               const cairo_region_t *shape_region,
                                               gint                  offset_x,
                                               gint                  offset_y)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  if (CDK_WINDOW_DESTROYED (window))
    return;

  g_clear_pointer (&impl->input_region, cairo_region_destroy);

  if (shape_region)
    {
      impl->input_region = cairo_region_copy (shape_region);
      cairo_region_translate (impl->input_region, offset_x, offset_y);
    }

  impl->input_region_dirty = TRUE;
}

static void
cdk_wayland_window_destroy (CdkWindow *window,
                            gboolean   recursing,
                            gboolean   foreign_destroy)
{
  g_return_if_fail (CDK_IS_WINDOW (window));

  /* Wayland windows can't be externally destroyed; we may possibly
   * eventually want to use this path at display close-down
   */
  g_return_if_fail (!foreign_destroy);

  cdk_wayland_window_hide_surface (window);
}

static void
cdk_window_wayland_destroy_foreign (CdkWindow *window)
{
}

static cairo_region_t *
cdk_wayland_window_get_shape (CdkWindow *window)
{
  return NULL;
}

static cairo_region_t *
cdk_wayland_window_get_input_shape (CdkWindow *window)
{
  return NULL;
}

static void
cdk_wayland_window_focus (CdkWindow *window,
                          guint32    timestamp)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  if (!impl->display_server.ctk_surface)
    return;

  if (timestamp == CDK_CURRENT_TIME)
    {
      CdkWaylandDisplay *display_wayland =
        CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));

      if (display_wayland->ctk_shell_version >= 3)
        {
          ctk_surface1_request_focus (impl->display_server.ctk_surface,
                                      display_wayland->startup_notification_id);
          g_clear_pointer (&display_wayland->startup_notification_id, g_free);
        }
    }
  else
    ctk_surface1_present (impl->display_server.ctk_surface, timestamp);
}

static void
cdk_wayland_window_set_type_hint (CdkWindow         *window,
                                  CdkWindowTypeHint  hint)
{
  CdkWindowImplWayland *impl;

  impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  if (CDK_WINDOW_DESTROYED (window))
    return;

  impl->hint = hint;
}

static CdkWindowTypeHint
cdk_wayland_window_get_type_hint (CdkWindow *window)
{
  CdkWindowImplWayland *impl;

  if (CDK_WINDOW_DESTROYED (window))
    return CDK_WINDOW_TYPE_HINT_NORMAL;

  impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  return impl->hint;
}

static void
ctk_surface_configure (void                *data,
                       struct ctk_surface1 *ctk_surface,
                       struct wl_array     *states)
{
  CdkWindow *window = CDK_WINDOW (data);
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  CdkWindowState new_state = 0;
  uint32_t *p;

  wl_array_for_each (p, states)
    {
      uint32_t state = *p;

      switch (state)
        {
        case CTK_SURFACE1_STATE_TILED:
          new_state |= CDK_WINDOW_STATE_TILED;
          break;

        /* Since v2 */
        case CTK_SURFACE1_STATE_TILED_TOP:
          new_state |= (CDK_WINDOW_STATE_TILED | CDK_WINDOW_STATE_TOP_TILED);
          break;
        case CTK_SURFACE1_STATE_TILED_RIGHT:
          new_state |= (CDK_WINDOW_STATE_TILED | CDK_WINDOW_STATE_RIGHT_TILED);
          break;
        case CTK_SURFACE1_STATE_TILED_BOTTOM:
          new_state |= (CDK_WINDOW_STATE_TILED | CDK_WINDOW_STATE_BOTTOM_TILED);
          break;
        case CTK_SURFACE1_STATE_TILED_LEFT:
          new_state |= (CDK_WINDOW_STATE_TILED | CDK_WINDOW_STATE_LEFT_TILED);
          break;
        default:
          /* Unknown state */
          break;
        }
    }

  impl->pending.state |= new_state;
}

static void
ctk_surface_configure_edges (void                *data,
                             struct ctk_surface1 *ctk_surface,
                             struct wl_array     *edge_constraints)
{
  CdkWindow *window = CDK_WINDOW (data);
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  CdkWindowState new_state = 0;
  uint32_t *p;

  wl_array_for_each (p, edge_constraints)
    {
      uint32_t constraint = *p;

      switch (constraint)
        {
        case CTK_SURFACE1_EDGE_CONSTRAINT_RESIZABLE_TOP:
          new_state |= CDK_WINDOW_STATE_TOP_RESIZABLE;
          break;
        case CTK_SURFACE1_EDGE_CONSTRAINT_RESIZABLE_RIGHT:
          new_state |= CDK_WINDOW_STATE_RIGHT_RESIZABLE;
          break;
        case CTK_SURFACE1_EDGE_CONSTRAINT_RESIZABLE_BOTTOM:
          new_state |= CDK_WINDOW_STATE_BOTTOM_RESIZABLE;
          break;
        case CTK_SURFACE1_EDGE_CONSTRAINT_RESIZABLE_LEFT:
          new_state |= CDK_WINDOW_STATE_LEFT_RESIZABLE;
          break;
        default:
          /* Unknown state */
          break;
        }
    }

  impl->pending.state |= new_state;
}

static const struct ctk_surface1_listener ctk_surface_listener = {
  ctk_surface_configure,
  ctk_surface_configure_edges
};

static void
cdk_wayland_window_init_ctk_surface (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  CdkWaylandDisplay *display =
    CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));

  if (impl->display_server.ctk_surface != NULL)
    return;
  if (!is_realized_toplevel (window))
    return;
  if (display->ctk_shell == NULL)
    return;

  impl->display_server.ctk_surface =
    ctk_shell1_get_ctk_surface (display->ctk_shell,
                                impl->display_server.wl_surface);
  cdk_window_set_geometry_hints (window,
                                 &impl->geometry_hints,
                                 impl->geometry_mask);
  ctk_surface1_add_listener (impl->display_server.ctk_surface,
                             &ctk_surface_listener,
                             window);
}

static void
maybe_set_ctk_surface_modal (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  cdk_wayland_window_init_ctk_surface (window);
  if (impl->display_server.ctk_surface == NULL)
    return;

  if (window->modal_hint)
    ctk_surface1_set_modal (impl->display_server.ctk_surface);
  else
    ctk_surface1_unset_modal (impl->display_server.ctk_surface);

}

static void
cdk_wayland_window_set_modal_hint (CdkWindow *window,
                                   gboolean   modal)
{
  window->modal_hint = modal;
  maybe_set_ctk_surface_modal (window);
}

static void
cdk_wayland_window_set_skip_taskbar_hint (CdkWindow *window,
                                          gboolean   skips_taskbar)
{
}

static void
cdk_wayland_window_set_skip_pager_hint (CdkWindow *window,
                                        gboolean   skips_pager)
{
}

static void
cdk_wayland_window_set_urgency_hint (CdkWindow *window,
                                     gboolean   urgent)
{
}

static void
cdk_wayland_window_set_geometry_hints (CdkWindow         *window,
                                       const CdkGeometry *geometry,
                                       CdkWindowHints     geom_mask)
{
  CdkWaylandDisplay *display_wayland;
  CdkWindowImplWayland *impl;
  int min_width = 0, min_height = 0, max_width = 0, max_height = 0;

  if (CDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL_OR_FOREIGN (window))
    return;

  impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  display_wayland = CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));

  impl->geometry_hints = *geometry;
  impl->geometry_mask = geom_mask;

  if (!is_realized_toplevel (window))
    return;

  if (geom_mask & CDK_HINT_MIN_SIZE)
    {
      min_width =
        MAX (0, calculate_width_without_margin (window, geometry->min_width));
      min_height =
        MAX (0, calculate_height_without_margin (window, geometry->min_height));
    }

  if (geom_mask & CDK_HINT_MAX_SIZE)
    {
      max_width =
        MAX (0, calculate_width_without_margin (window, geometry->max_width));
      max_height =
        MAX (0, calculate_height_without_margin (window, geometry->max_height));
    }

  switch (display_wayland->shell_variant)
    {
    case CDK_WAYLAND_SHELL_VARIANT_XDG_SHELL:
      xdg_toplevel_set_min_size (impl->display_server.xdg_toplevel,
                                 min_width, min_height);
      xdg_toplevel_set_max_size (impl->display_server.xdg_toplevel,
                                 max_width, max_height);
      break;
    case CDK_WAYLAND_SHELL_VARIANT_ZXDG_SHELL_V6:
      zxdg_toplevel_v6_set_min_size (impl->display_server.zxdg_toplevel_v6,
                                     min_width, min_height);
      zxdg_toplevel_v6_set_max_size (impl->display_server.zxdg_toplevel_v6,
                                     max_width, max_height);
      break;
    }
}

static void
cdk_wayland_window_set_title (CdkWindow   *window,
                              const gchar *title)
{
  CdkWindowImplWayland *impl;
  const char *end;
  g_return_if_fail (title != NULL);

  if (CDK_WINDOW_DESTROYED (window))
    return;

  impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  if (g_strcmp0 (impl->title, title) == 0)
    return;

  g_free (impl->title);

  g_utf8_validate (title, MAX_WL_BUFFER_SIZE, &end);
  impl->title = g_malloc (end - title + 1);
  memcpy (impl->title, title, end - title);
  impl->title[end - title] = '\0';

  cdk_wayland_window_sync_title (window);
}

static void
cdk_wayland_window_set_role (CdkWindow   *window,
                             const gchar *role)
{
}

static void
cdk_wayland_window_set_startup_id (CdkWindow   *window,
                                   const gchar *startup_id)
{
}

static gboolean
check_transient_for_loop (CdkWindow *window,
                          CdkWindow *parent)
{
  while (parent)
    {
      CdkWindowImplWayland *impl;

      if (!CDK_IS_WINDOW_IMPL_WAYLAND(parent->impl))
        return FALSE;

      impl = CDK_WINDOW_IMPL_WAYLAND (parent->impl);
      if (impl->transient_for == window)
        return TRUE;
      parent = impl->transient_for;
    }
  return FALSE;
}

static void
cdk_wayland_window_set_transient_for (CdkWindow *window,
                                      CdkWindow *parent)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  CdkWaylandDisplay *display_wayland =
    CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));
  CdkWindow *previous_parent;
  gboolean was_subsurface = FALSE;

  g_assert (parent == NULL ||
            cdk_window_get_display (window) == cdk_window_get_display (parent));

  if (check_transient_for_loop (window, parent))
    {
      g_warning ("Setting %p transient for %p would create a loop", window, parent);
      return;
    }

  unset_transient_for_exported (window);

  if (impl->display_server.wl_subsurface)
    {
      was_subsurface = TRUE;
      unmap_subsurface (window);
    }

  previous_parent = impl->transient_for;
  impl->transient_for = parent;

  if (impl->hint == CDK_WINDOW_TYPE_HINT_DIALOG)
    {
      if (!parent)
        _cdk_wayland_screen_add_orphan_dialog (window);
      else if (!previous_parent)
        display_wayland->orphan_dialogs =
          g_list_remove (display_wayland->orphan_dialogs, window);
    }

  cdk_wayland_window_sync_parent (window, NULL);

  if (was_subsurface && parent)
    cdk_wayland_window_create_subsurface (window);
}

static void
cdk_wayland_window_get_frame_extents (CdkWindow    *window,
                                      CdkRectangle *rect)
{
  *rect = (CdkRectangle) {
    .x = window->x,
    .y = window->y,
    .width = window->width,
    .height = window->height
  };
}

static void
cdk_wayland_window_set_override_redirect (CdkWindow *window,
                                          gboolean   override_redirect)
{
}

static void
cdk_wayland_window_set_accept_focus (CdkWindow *window,
                                     gboolean   accept_focus)
{
}

static void
cdk_wayland_window_set_focus_on_map (CdkWindow *window,
                                     gboolean focus_on_map)
{
}

static void
cdk_wayland_window_set_icon_list (CdkWindow *window,
                                  GList     *pixbufs)
{
}

static void
cdk_wayland_window_set_icon_name (CdkWindow   *window,
                                  const gchar *name)
{
  if (CDK_WINDOW_DESTROYED (window))
    return;
}

static void
cdk_wayland_window_iconify (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  CdkWaylandDisplay *display_wayland;

  if (CDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL_OR_FOREIGN (window))
    return;

  if (!is_realized_toplevel (window))
    return;

  display_wayland = CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));
  switch (display_wayland->shell_variant)
    {
    case CDK_WAYLAND_SHELL_VARIANT_XDG_SHELL:
      xdg_toplevel_set_minimized (impl->display_server.xdg_toplevel);
      break;
    case CDK_WAYLAND_SHELL_VARIANT_ZXDG_SHELL_V6:
      zxdg_toplevel_v6_set_minimized (impl->display_server.zxdg_toplevel_v6);
      break;
    }
}

static void
cdk_wayland_window_deiconify (CdkWindow *window)
{
  if (CDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL_OR_FOREIGN (window))
    return;

  if (CDK_WINDOW_IS_MAPPED (window))
    cdk_window_show (window);
  else
    /* Flip our client side flag, the real work happens on map. */
    cdk_synthesize_window_state (window, CDK_WINDOW_STATE_ICONIFIED, 0);
}

static void
cdk_wayland_window_stick (CdkWindow *window)
{
}

static void
cdk_wayland_window_unstick (CdkWindow *window)
{
}

static void
cdk_wayland_window_maximize (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  if (CDK_WINDOW_DESTROYED (window))
    return;

  _cdk_wayland_window_save_size (window);
  if (is_realized_toplevel (window))
    {
      CdkWaylandDisplay *display_wayland =
        CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));

      switch (display_wayland->shell_variant)
        {
        case CDK_WAYLAND_SHELL_VARIANT_XDG_SHELL:
          xdg_toplevel_set_maximized (impl->display_server.xdg_toplevel);
          break;
        case CDK_WAYLAND_SHELL_VARIANT_ZXDG_SHELL_V6:
          zxdg_toplevel_v6_set_maximized (impl->display_server.zxdg_toplevel_v6);
          break;
        }
    }
  else
    {
      cdk_synthesize_window_state (window, 0, CDK_WINDOW_STATE_MAXIMIZED);
    }
}

static void
cdk_wayland_window_unmaximize (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  if (CDK_WINDOW_DESTROYED (window))
    return;

  if (is_realized_toplevel (window))
    {
      CdkWaylandDisplay *display_wayland =
        CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));

      switch (display_wayland->shell_variant)
        {
        case CDK_WAYLAND_SHELL_VARIANT_XDG_SHELL:
          xdg_toplevel_unset_maximized (impl->display_server.xdg_toplevel);
          break;
        case CDK_WAYLAND_SHELL_VARIANT_ZXDG_SHELL_V6:
          zxdg_toplevel_v6_unset_maximized (impl->display_server.zxdg_toplevel_v6);
          break;
        }
    }
  else
    {
      cdk_synthesize_window_state (window, CDK_WINDOW_STATE_MAXIMIZED, 0);
    }
}

static void
cdk_wayland_window_fullscreen_on_monitor (CdkWindow *window, gint monitor)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  CdkScreen *screen = cdk_window_get_screen (window);
  struct wl_output *fullscreen_output = 
    _cdk_wayland_screen_get_wl_output (screen, monitor);
  
  if (CDK_WINDOW_DESTROYED (window))
    return;

  _cdk_wayland_window_save_size (window);
  if (is_realized_toplevel (window))
    {
      CdkWaylandDisplay *display_wayland =
        CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));

      switch (display_wayland->shell_variant)
        {
        case CDK_WAYLAND_SHELL_VARIANT_XDG_SHELL:
          xdg_toplevel_set_fullscreen (impl->display_server.xdg_toplevel,
                                       fullscreen_output);
          break;
        case CDK_WAYLAND_SHELL_VARIANT_ZXDG_SHELL_V6:
          zxdg_toplevel_v6_set_fullscreen (impl->display_server.zxdg_toplevel_v6,
                                           fullscreen_output);
          break;
        }
    }
  else
    {
      cdk_synthesize_window_state (window, 0, CDK_WINDOW_STATE_FULLSCREEN);
      impl->initial_fullscreen_monitor = monitor;
    }
}

static void
cdk_wayland_window_fullscreen (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  if (CDK_WINDOW_DESTROYED (window))
    return;

  impl->initial_fullscreen_monitor = -1;

  _cdk_wayland_window_save_size (window);
  if (is_realized_toplevel (window))
    {
      CdkWaylandDisplay *display_wayland =
        CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));

      switch (display_wayland->shell_variant)
        {
        case CDK_WAYLAND_SHELL_VARIANT_XDG_SHELL:
          xdg_toplevel_set_fullscreen (impl->display_server.xdg_toplevel,
                                       NULL);
          break;
        case CDK_WAYLAND_SHELL_VARIANT_ZXDG_SHELL_V6:
          zxdg_toplevel_v6_set_fullscreen (impl->display_server.zxdg_toplevel_v6,
                                           NULL);
          break;
        }
    }
  else
    {
      cdk_synthesize_window_state (window, 0, CDK_WINDOW_STATE_FULLSCREEN);
    }
}

static void
cdk_wayland_window_unfullscreen (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  
  if (CDK_WINDOW_DESTROYED (window))
    return;

  impl->initial_fullscreen_monitor = -1;

  if (is_realized_toplevel (window))
    {
      CdkWaylandDisplay *display_wayland =
        CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));

      switch (display_wayland->shell_variant)
        {
        case CDK_WAYLAND_SHELL_VARIANT_XDG_SHELL:
          xdg_toplevel_unset_fullscreen (impl->display_server.xdg_toplevel);
          break;
        case CDK_WAYLAND_SHELL_VARIANT_ZXDG_SHELL_V6:
          zxdg_toplevel_v6_unset_fullscreen (impl->display_server.zxdg_toplevel_v6);
          break;
        }
    }
  else
    {
      cdk_synthesize_window_state (window, CDK_WINDOW_STATE_FULLSCREEN, 0);
    }
}

static void
cdk_wayland_window_set_keep_above (CdkWindow *window, gboolean setting)
{
}

static void
cdk_wayland_window_set_keep_below (CdkWindow *window, gboolean setting)
{
}

static CdkWindow *
cdk_wayland_window_get_group (CdkWindow *window)
{
  return NULL;
}

static void
cdk_wayland_window_set_group (CdkWindow *window,
                              CdkWindow *leader)
{
}

static void
cdk_wayland_window_set_decorations (CdkWindow       *window,
                                    CdkWMDecoration  decorations)
{
}

static gboolean
cdk_wayland_window_get_decorations (CdkWindow       *window,
                                    CdkWMDecoration *decorations)
{
  return FALSE;
}

static void
cdk_wayland_window_set_functions (CdkWindow     *window,
                                  CdkWMFunction  functions)
{
}

static void
cdk_wayland_window_begin_resize_drag (CdkWindow     *window,
                                      CdkWindowEdge  edge,
                                      CdkDevice     *device,
                                      gint           button,
                                      gint           root_x,
                                      gint           root_y,
                                      guint32        timestamp)
{
  CdkWindowImplWayland *impl;
  CdkWaylandDisplay *display_wayland;
  CdkEventSequence *sequence;
  uint32_t resize_edges, serial;

  if (CDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL_OR_FOREIGN (window))
    return;

  switch (edge)
    {
    case CDK_WINDOW_EDGE_NORTH_WEST:
      resize_edges = ZXDG_TOPLEVEL_V6_RESIZE_EDGE_TOP_LEFT;
      break;

    case CDK_WINDOW_EDGE_NORTH:
      resize_edges = ZXDG_TOPLEVEL_V6_RESIZE_EDGE_TOP;
      break;

    case CDK_WINDOW_EDGE_NORTH_EAST:
      resize_edges = ZXDG_TOPLEVEL_V6_RESIZE_EDGE_TOP_RIGHT;
      break;

    case CDK_WINDOW_EDGE_WEST:
      resize_edges = ZXDG_TOPLEVEL_V6_RESIZE_EDGE_LEFT;
      break;

    case CDK_WINDOW_EDGE_EAST:
      resize_edges = ZXDG_TOPLEVEL_V6_RESIZE_EDGE_RIGHT;
      break;

    case CDK_WINDOW_EDGE_SOUTH_WEST:
      resize_edges = ZXDG_TOPLEVEL_V6_RESIZE_EDGE_BOTTOM_LEFT;
      break;

    case CDK_WINDOW_EDGE_SOUTH:
      resize_edges = ZXDG_TOPLEVEL_V6_RESIZE_EDGE_BOTTOM;
      break;

    case CDK_WINDOW_EDGE_SOUTH_EAST:
      resize_edges = ZXDG_TOPLEVEL_V6_RESIZE_EDGE_BOTTOM_RIGHT;
      break;

    default:
      g_warning ("cdk_window_begin_resize_drag: bad resize edge %d!", edge);
      return;
    }

  impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  display_wayland = CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));

  if (!is_realized_toplevel (window))
    return;

  serial = _cdk_wayland_seat_get_last_implicit_grab_serial (cdk_device_get_seat (device),
                                                            &sequence);

  switch (display_wayland->shell_variant)
    {
    case CDK_WAYLAND_SHELL_VARIANT_XDG_SHELL:
      xdg_toplevel_resize (impl->display_server.xdg_toplevel,
                           cdk_wayland_device_get_wl_seat (device),
                           serial, resize_edges);
      break;
    case CDK_WAYLAND_SHELL_VARIANT_ZXDG_SHELL_V6:
      zxdg_toplevel_v6_resize (impl->display_server.zxdg_toplevel_v6,
                               cdk_wayland_device_get_wl_seat (device),
                               serial, resize_edges);
      break;
    }

  if (sequence)
    cdk_wayland_device_unset_touch_grab (device, sequence);

  /* This is needed since Wayland will absorb all the pointer events after the
   * above function - FIXME: Is this always safe..?
   */
  cdk_seat_ungrab (cdk_device_get_seat (device));
}

static void
cdk_wayland_window_begin_move_drag (CdkWindow *window,
                                    CdkDevice *device,
                                    gint       button,
                                    gint       root_x,
                                    gint       root_y,
                                    guint32    timestamp)
{
  CdkWindowImplWayland *impl;
  CdkWaylandDisplay *display_wayland;
  CdkEventSequence *sequence;
  uint32_t serial;

  if (CDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  display_wayland = CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));

  if (!is_realized_toplevel (window))
    return;

  serial = _cdk_wayland_seat_get_last_implicit_grab_serial (cdk_device_get_seat (device),
                                                            &sequence);
  switch (display_wayland->shell_variant)
    {
    case CDK_WAYLAND_SHELL_VARIANT_XDG_SHELL:
      xdg_toplevel_move (impl->display_server.xdg_toplevel,
                         cdk_wayland_device_get_wl_seat (device),
                         serial);
      break;
    case CDK_WAYLAND_SHELL_VARIANT_ZXDG_SHELL_V6:
      zxdg_toplevel_v6_move (impl->display_server.zxdg_toplevel_v6,
                             cdk_wayland_device_get_wl_seat (device),
                             serial);
      break;
    }

  if (sequence)
    cdk_wayland_device_unset_touch_grab (device, sequence);

  /* This is needed since Wayland will absorb all the pointer events after the
   * above function - FIXME: Is this always safe..?
   */
  cdk_seat_ungrab (cdk_device_get_seat (device));
}

static void
cdk_wayland_window_set_opacity (CdkWindow *window,
                                gdouble    opacity)
{
}

static void
cdk_wayland_window_set_composited (CdkWindow *window,
                                   gboolean   composited)
{
}

static void
cdk_wayland_window_destroy_notify (CdkWindow *window)
{
  if (!CDK_WINDOW_DESTROYED (window))
    {
      if (CDK_WINDOW_TYPE (window) != CDK_WINDOW_FOREIGN)
        g_warning ("CdkWindow %p unexpectedly destroyed", window);

      _cdk_window_destroy (window, TRUE);
    }

  g_object_unref (window);
}

static void
cdk_wayland_window_sync_rendering (CdkWindow *window)
{
}

static gboolean
cdk_wayland_window_simulate_key (CdkWindow       *window,
                                 gint             x,
                                 gint             y,
                                 guint            keyval,
                                 CdkModifierType  modifiers,
                                 CdkEventType     key_pressrelease)
{
  return FALSE;
}

static gboolean
cdk_wayland_window_simulate_button (CdkWindow       *window,
                                    gint             x,
                                    gint             y,
                                    guint            button,
                                    CdkModifierType  modifiers,
                                    CdkEventType     button_pressrelease)
{
  return FALSE;
}

static gboolean
cdk_wayland_window_get_property (CdkWindow   *window,
                                 CdkAtom      property,
                                 CdkAtom      type,
                                 gulong       offset,
                                 gulong       length,
                                 gint         pdelete,
                                 CdkAtom     *actual_property_type,
                                 gint        *actual_format_type,
                                 gint        *actual_length,
                                 guchar     **data)
{
  return FALSE;
}

static void
cdk_wayland_window_change_property (CdkWindow    *window,
                                    CdkAtom       property,
                                    CdkAtom       type,
                                    gint          format,
                                    CdkPropMode   mode,
                                    const guchar *data,
                                    gint          nelements)
{
  if (property == cdk_atom_intern_static_string ("CDK_SELECTION"))
    cdk_wayland_selection_store (window, type, mode, data, nelements * (format / 8));
}

static void
cdk_wayland_window_delete_property (CdkWindow *window,
                                    CdkAtom    property)
{
}

static gint
cdk_wayland_window_get_scale_factor (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  if (CDK_WINDOW_DESTROYED (window))
    return 1;

  return impl->scale;
}

static void
cdk_wayland_window_set_opaque_region (CdkWindow      *window,
                                      cairo_region_t *region)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  if (CDK_WINDOW_DESTROYED (window))
    return;

  g_clear_pointer (&impl->opaque_region, cairo_region_destroy);
  impl->opaque_region = cairo_region_reference (region);
  impl->opaque_region_dirty = TRUE;
}

static void
cdk_wayland_window_set_shadow_width (CdkWindow *window,
                                     int        left,
                                     int        right,
                                     int        top,
                                     int        bottom)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  gint new_width, new_height;

  if (CDK_WINDOW_DESTROYED (window))
    return;

  /* Reconfigure window to keep the same window geometry */
  new_width = (calculate_width_without_margin (window, window->width) +
               (left + right));
  new_height = (calculate_height_without_margin (window, window->height) +
                (top + bottom));
  cdk_wayland_window_maybe_configure (window, new_width, new_height, impl->scale);

  impl->margin_left = left;
  impl->margin_right = right;
  impl->margin_top = top;
  impl->margin_bottom = bottom;
}

static gboolean
cdk_wayland_window_show_window_menu (CdkWindow *window,
                                     CdkEvent  *event)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  CdkWaylandDisplay *display_wayland =
    CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));
  struct wl_seat *seat;
  CdkWaylandDevice *device;
  CdkWindow *event_window;
  double x, y;
  uint32_t serial;

  switch (event->type)
    {
    case CDK_BUTTON_PRESS:
    case CDK_BUTTON_RELEASE:
    case CDK_TOUCH_BEGIN:
    case CDK_TOUCH_END:
      break;
    default:
      return FALSE;
    }

  if (!is_realized_toplevel (window))
    return FALSE;

  device = CDK_WAYLAND_DEVICE (cdk_event_get_device (event));
  seat = cdk_wayland_device_get_wl_seat (CDK_DEVICE (device));

  cdk_event_get_coords (event, &x, &y);
  event_window = cdk_event_get_window (event);
  while (cdk_window_get_window_type (event_window) != CDK_WINDOW_TOPLEVEL)
    {
      cdk_window_coords_to_parent (event_window, x, y, &x, &y);
      event_window = cdk_window_get_effective_parent (event_window);
    }

  serial = _cdk_wayland_device_get_implicit_grab_serial (device, event);

  switch (display_wayland->shell_variant)
    {
    case CDK_WAYLAND_SHELL_VARIANT_XDG_SHELL:
      xdg_toplevel_show_window_menu (impl->display_server.xdg_toplevel,
                                     seat, serial, x, y);
      break;
    case CDK_WAYLAND_SHELL_VARIANT_ZXDG_SHELL_V6:
      zxdg_toplevel_v6_show_window_menu (impl->display_server.zxdg_toplevel_v6,
                                         seat, serial, x, y);
      break;
    }

  return TRUE;
}

static void
_cdk_window_impl_wayland_class_init (CdkWindowImplWaylandClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CdkWindowImplClass *impl_class = CDK_WINDOW_IMPL_CLASS (klass);

  object_class->finalize = cdk_window_impl_wayland_finalize;

  impl_class->ref_cairo_surface = cdk_wayland_window_ref_cairo_surface;
  impl_class->create_similar_image_surface = cdk_wayland_window_create_similar_image_surface;
  impl_class->show = cdk_wayland_window_show;
  impl_class->hide = cdk_wayland_window_hide;
  impl_class->withdraw = cdk_window_wayland_withdraw;
  impl_class->set_events = cdk_window_wayland_set_events;
  impl_class->get_events = cdk_window_wayland_get_events;
  impl_class->raise = cdk_window_wayland_raise;
  impl_class->lower = cdk_window_wayland_lower;
  impl_class->restack_under = cdk_window_wayland_restack_under;
  impl_class->restack_toplevel = cdk_window_wayland_restack_toplevel;
  impl_class->move_resize = cdk_window_wayland_move_resize;
  impl_class->move_to_rect = cdk_window_wayland_move_to_rect;
  impl_class->set_background = cdk_window_wayland_set_background;
  impl_class->reparent = cdk_window_wayland_reparent;
  impl_class->set_device_cursor = cdk_window_wayland_set_device_cursor;
  impl_class->get_geometry = cdk_window_wayland_get_geometry;
  impl_class->get_root_coords = cdk_window_wayland_get_root_coords;
  impl_class->get_device_state = cdk_window_wayland_get_device_state;
  impl_class->shape_combine_region = cdk_window_wayland_shape_combine_region;
  impl_class->input_shape_combine_region = cdk_window_wayland_input_shape_combine_region;
  impl_class->destroy = cdk_wayland_window_destroy;
  impl_class->destroy_foreign = cdk_window_wayland_destroy_foreign;
  impl_class->get_shape = cdk_wayland_window_get_shape;
  impl_class->get_input_shape = cdk_wayland_window_get_input_shape;
  impl_class->begin_paint = cdk_window_impl_wayland_begin_paint;
  impl_class->end_paint = cdk_window_impl_wayland_end_paint;
  impl_class->beep = cdk_window_impl_wayland_beep;

  impl_class->focus = cdk_wayland_window_focus;
  impl_class->set_type_hint = cdk_wayland_window_set_type_hint;
  impl_class->get_type_hint = cdk_wayland_window_get_type_hint;
  impl_class->set_modal_hint = cdk_wayland_window_set_modal_hint;
  impl_class->set_skip_taskbar_hint = cdk_wayland_window_set_skip_taskbar_hint;
  impl_class->set_skip_pager_hint = cdk_wayland_window_set_skip_pager_hint;
  impl_class->set_urgency_hint = cdk_wayland_window_set_urgency_hint;
  impl_class->set_geometry_hints = cdk_wayland_window_set_geometry_hints;
  impl_class->set_title = cdk_wayland_window_set_title;
  impl_class->set_role = cdk_wayland_window_set_role;
  impl_class->set_startup_id = cdk_wayland_window_set_startup_id;
  impl_class->set_transient_for = cdk_wayland_window_set_transient_for;
  impl_class->get_frame_extents = cdk_wayland_window_get_frame_extents;
  impl_class->set_override_redirect = cdk_wayland_window_set_override_redirect;
  impl_class->set_accept_focus = cdk_wayland_window_set_accept_focus;
  impl_class->set_focus_on_map = cdk_wayland_window_set_focus_on_map;
  impl_class->set_icon_list = cdk_wayland_window_set_icon_list;
  impl_class->set_icon_name = cdk_wayland_window_set_icon_name;
  impl_class->iconify = cdk_wayland_window_iconify;
  impl_class->deiconify = cdk_wayland_window_deiconify;
  impl_class->stick = cdk_wayland_window_stick;
  impl_class->unstick = cdk_wayland_window_unstick;
  impl_class->maximize = cdk_wayland_window_maximize;
  impl_class->unmaximize = cdk_wayland_window_unmaximize;
  impl_class->fullscreen = cdk_wayland_window_fullscreen;
  impl_class->fullscreen_on_monitor = cdk_wayland_window_fullscreen_on_monitor;
  impl_class->unfullscreen = cdk_wayland_window_unfullscreen;
  impl_class->set_keep_above = cdk_wayland_window_set_keep_above;
  impl_class->set_keep_below = cdk_wayland_window_set_keep_below;
  impl_class->get_group = cdk_wayland_window_get_group;
  impl_class->set_group = cdk_wayland_window_set_group;
  impl_class->set_decorations = cdk_wayland_window_set_decorations;
  impl_class->get_decorations = cdk_wayland_window_get_decorations;
  impl_class->set_functions = cdk_wayland_window_set_functions;
  impl_class->begin_resize_drag = cdk_wayland_window_begin_resize_drag;
  impl_class->begin_move_drag = cdk_wayland_window_begin_move_drag;
  impl_class->set_opacity = cdk_wayland_window_set_opacity;
  impl_class->set_composited = cdk_wayland_window_set_composited;
  impl_class->destroy_notify = cdk_wayland_window_destroy_notify;
  impl_class->get_drag_protocol = _cdk_wayland_window_get_drag_protocol;
  impl_class->register_dnd = _cdk_wayland_window_register_dnd;
  impl_class->drag_begin = _cdk_wayland_window_drag_begin;
  impl_class->sync_rendering = cdk_wayland_window_sync_rendering;
  impl_class->simulate_key = cdk_wayland_window_simulate_key;
  impl_class->simulate_button = cdk_wayland_window_simulate_button;
  impl_class->get_property = cdk_wayland_window_get_property;
  impl_class->change_property = cdk_wayland_window_change_property;
  impl_class->delete_property = cdk_wayland_window_delete_property;
  impl_class->get_scale_factor = cdk_wayland_window_get_scale_factor;
  impl_class->set_opaque_region = cdk_wayland_window_set_opaque_region;
  impl_class->set_shadow_width = cdk_wayland_window_set_shadow_width;
  impl_class->show_window_menu = cdk_wayland_window_show_window_menu;
  impl_class->create_gl_context = cdk_wayland_window_create_gl_context;
  impl_class->invalidate_for_new_frame = cdk_wayland_window_invalidate_for_new_frame;

  signals[COMMITTED] = g_signal_new ("committed",
                                     G_TYPE_FROM_CLASS (object_class),
                                     G_SIGNAL_RUN_LAST,
                                     0,
                                     NULL, NULL, NULL,
                                     G_TYPE_NONE, 0);
}

void
_cdk_wayland_window_set_grab_seat (CdkWindow *window,
                                   CdkSeat   *seat)
{
  CdkWindowImplWayland *impl;

  g_return_if_fail (window != NULL);

  impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  impl->grab_input_seat = seat;
}

/**
 * cdk_wayland_window_get_wl_surface:
 * @window: (type CdkWaylandWindow): a #CdkWindow
 *
 * Returns the Wayland surface of a #CdkWindow.
 *
 * Returns: (transfer none): a Wayland wl_surface
 *
 * Since: 3.8
 */
struct wl_surface *
cdk_wayland_window_get_wl_surface (CdkWindow *window)
{
  g_return_val_if_fail (CDK_IS_WAYLAND_WINDOW (window), NULL);

  return CDK_WINDOW_IMPL_WAYLAND (window->impl)->display_server.wl_surface;
}

struct wl_output *
cdk_wayland_window_get_wl_output (CdkWindow *window)
{
  CdkWindowImplWayland *impl;

  g_return_val_if_fail (CDK_IS_WAYLAND_WINDOW (window), NULL);

  impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  /* We pick the head of the list as this is the last entered output */
  if (impl->display_server.outputs)
    return (struct wl_output *) impl->display_server.outputs->data;

  return NULL;
}

static struct wl_egl_window *
cdk_wayland_window_get_wl_egl_window (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  if (impl->display_server.egl_window == NULL)
    {
      impl->display_server.egl_window =
        wl_egl_window_create (impl->display_server.wl_surface,
                              impl->wrapper->width * impl->scale,
                              impl->wrapper->height * impl->scale);
      wl_surface_set_buffer_scale (impl->display_server.wl_surface, impl->scale);
    }

  return impl->display_server.egl_window;
}

EGLSurface
cdk_wayland_window_get_egl_surface (CdkWindow *window,
                                    EGLConfig  config)
{
  CdkWaylandDisplay *display = CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));
  CdkWindowImplWayland *impl;

  g_return_val_if_fail (CDK_IS_WAYLAND_WINDOW (window), NULL);

  impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  if (impl->egl_surface == NULL)
    {
      struct wl_egl_window *egl_window;

      egl_window = cdk_wayland_window_get_wl_egl_window (window);

      impl->egl_surface =
        eglCreateWindowSurface (display->egl_display, config, egl_window, NULL);
    }

  return impl->egl_surface;
}

EGLSurface
cdk_wayland_window_get_dummy_egl_surface (CdkWindow *window,
                                          EGLConfig  config)
{
  CdkWaylandDisplay *display = CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));
  CdkWindowImplWayland *impl;

  g_return_val_if_fail (CDK_IS_WAYLAND_WINDOW (window), NULL);

  impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  if (impl->dummy_egl_surface == NULL)
    {
      impl->display_server.dummy_egl_window =
        wl_egl_window_create (impl->display_server.wl_surface, 1, 1);

      impl->dummy_egl_surface =
        eglCreateWindowSurface (display->egl_display, config, impl->display_server.dummy_egl_window, NULL);
    }

  return impl->dummy_egl_surface;
}

struct ctk_surface1 *
cdk_wayland_window_get_ctk_surface (CdkWindow *window)
{
  g_return_val_if_fail (CDK_IS_WAYLAND_WINDOW (window), NULL);

  return CDK_WINDOW_IMPL_WAYLAND (window->impl)->display_server.ctk_surface;
}

/**
 * cdk_wayland_window_set_use_custom_surface:
 * @window: (type CdkWaylandWindow): a #CdkWindow
 *
 * Marks a #CdkWindow as a custom Wayland surface. The application is
 * expected to register the surface as some type of surface using
 * some Wayland interface.
 *
 * A good example would be writing a panel or on-screen-keyboard as an
 * out-of-process helper - as opposed to having those in the compositor
 * process. In this case the underlying surface isn’t an xdg_shell
 * surface and the panel or OSK client need to identify the wl_surface
 * as a panel or OSK to the compositor. The assumption is that the
 * compositor will expose a private interface to the special client
 * that lets the client identify the wl_surface as a panel or such.
 *
 * This function should be called before a #CdkWindow is shown. This is
 * best done by connecting to the #CtkWidget::realize signal:
 *
 * |[<!-- language="C" -->
 *   static void
 *   widget_realize_cb (CtkWidget *widget)
 *   {
 *     CdkWindow *window;
 *     struct wl_surface *surface;
 *     struct input_panel_surface *ip_surface;
 *
 *     window = ctk_widget_get_window (widget);
 *     cdk_wayland_window_set_custom_surface (window);
 *
 *     surface = cdk_wayland_window_get_wl_surface (window);
 *     ip_surface = input_panel_get_input_panel_surface (input_panel, surface);
 *     input_panel_surface_set_panel (ip_surface);
 *   }
 *
 *   static void
 *   setup_window (CtkWindow *window)
 *   {
 *     g_signal_connect (window, "realize", G_CALLBACK (widget_realize_cb), NULL);
 *   }
 * ]|
 *
 * Since: 3.10
 */
void
cdk_wayland_window_set_use_custom_surface (CdkWindow *window)
{
  CdkWindowImplWayland *impl;

  g_return_if_fail (CDK_IS_WAYLAND_WINDOW (window));

  impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  if (!impl->display_server.wl_surface)
    cdk_wayland_window_create_surface (window);

  impl->use_custom_surface = TRUE;
}

static void
maybe_set_ctk_surface_dbus_properties (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  if (impl->application.was_set)
    return;

  if (impl->application.application_id == NULL &&
      impl->application.app_menu_path == NULL &&
      impl->application.menubar_path == NULL &&
      impl->application.window_object_path == NULL &&
      impl->application.application_object_path == NULL &&
      impl->application.unique_bus_name == NULL)
    return;

  cdk_wayland_window_init_ctk_surface (window);
  if (impl->display_server.ctk_surface == NULL)
    return;

  ctk_surface1_set_dbus_properties (impl->display_server.ctk_surface,
                                    impl->application.application_id,
                                    impl->application.app_menu_path,
                                    impl->application.menubar_path,
                                    impl->application.window_object_path,
                                    impl->application.application_object_path,
                                    impl->application.unique_bus_name);
  impl->application.was_set = TRUE;
}

void
cdk_wayland_window_set_dbus_properties_libctk_only (CdkWindow  *window,
                                                    const char *application_id,
                                                    const char *app_menu_path,
                                                    const char *menubar_path,
                                                    const char *window_object_path,
                                                    const char *application_object_path,
                                                    const char *unique_bus_name)
{
  CdkWindowImplWayland *impl;

  g_return_if_fail (CDK_IS_WAYLAND_WINDOW (window));

  impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  impl->application.application_id = g_strdup (application_id);
  impl->application.app_menu_path = g_strdup (app_menu_path);
  impl->application.menubar_path = g_strdup (menubar_path);
  impl->application.window_object_path = g_strdup (window_object_path);
  impl->application.application_object_path =
    g_strdup (application_object_path);
  impl->application.unique_bus_name = g_strdup (unique_bus_name);

  maybe_set_ctk_surface_dbus_properties (window);
}

void
_cdk_wayland_window_offset_next_wl_buffer (CdkWindow *window,
                                           int        x,
                                           int        y)
{
  CdkWindowImplWayland *impl;

  g_return_if_fail (CDK_IS_WAYLAND_WINDOW (window));

  impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  impl->pending_buffer_offset_x = x;
  impl->pending_buffer_offset_y = y;
}

static void
invoke_exported_closures (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  GList *l;

  for (l = impl->exported.closures; l; l = l->next)
    {
      ExportedClosure *closure = l->data;

      closure->callback (window, impl->exported.handle, closure->user_data);

      if (closure->destroy_func)
        closure->destroy_func (closure->user_data);
    }

  g_list_free_full (impl->exported.closures, g_free);
  impl->exported.closures = NULL;
}

static void
xdg_exported_handle (void                    *data,
                     struct zxdg_exported_v1 *zxdg_exported_v1,
                     const char              *handle)
{
  CdkWindow *window = data;
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  impl->exported.handle = g_strdup (handle);

  invoke_exported_closures (window);
}

static const struct zxdg_exported_v1_listener xdg_exported_listener = {
  xdg_exported_handle
};

/**
 * CdkWaylandWindowExported:
 * @window: the #CdkWindow that is exported
 * @handle: the handle
 * @user_data: user data that was passed to cdk_wayland_window_export_handle()
 *
 * Callback that gets called when the handle for a window has been
 * obtained from the Wayland compositor. The handle can be passed
 * to other processes, for the purpose of marking windows as transient
 * for out-of-process surfaces.
 *
 * Since: 3.22
 */

static gboolean
cdk_wayland_window_is_exported (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  return !!impl->display_server.xdg_exported;
}

static gboolean
exported_idle (gpointer user_data)
{
  CdkWindow *window = user_data;
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  invoke_exported_closures (window);

  impl->exported.idle_source_id = 0;

  return G_SOURCE_REMOVE;
}

/**
 * cdk_wayland_window_export_handle:
 * @window: the #CdkWindow to obtain a handle for
 * @callback: callback to call with the handle
 * @user_data: user data for @callback
 * @destroy_func: destroy notify for @user_data
 *
 * Asynchronously obtains a handle for a surface that can be passed
 * to other processes. When the handle has been obtained, @callback
 * will be called.
 *
 * Up until 3.22.15 it was an error to call this function on a window that is
 * already exported. When the handle is no longer needed,
 * cdk_wayland_window_unexport_handle() should be called to clean up resources.
 *
 * Starting with 3.22.16, calling this function on an already exported window
 * will cause the callback to be invoked with the same handle as was already
 * invoked, from an idle callback. To unexport the window,
 * cdk_wayland_window_unexport_handle() must be called the same number of times
 * as cdk_wayland_window_export_handle() was called. Any 'exported' callback
 * may still be invoked until the window is unexported or destroyed.
 *
 * The main purpose for obtaining a handle is to mark a surface
 * from another window as transient for this one, see
 * cdk_wayland_window_set_transient_for_exported().
 *
 * Note that this API depends on an unstable Wayland protocol,
 * and thus may require changes in the future.
 *
 * Return value: %TRUE if the handle has been requested, %FALSE if
 *     an error occurred.
 *
 * Since: 3.22
 */
gboolean
cdk_wayland_window_export_handle (CdkWindow                *window,
                                  CdkWaylandWindowExported  callback,
                                  gpointer                  user_data,
                                  GDestroyNotify            destroy_func)
{
  CdkWindowImplWayland *impl;
  CdkWaylandDisplay *display_wayland;
  CdkDisplay *display = cdk_window_get_display (window);
  ExportedClosure *closure;

  g_return_val_if_fail (CDK_IS_WAYLAND_WINDOW (window), FALSE);
  g_return_val_if_fail (CDK_IS_WAYLAND_DISPLAY (display), FALSE);

  impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  display_wayland = CDK_WAYLAND_DISPLAY (display);

  if (!display_wayland->xdg_exporter)
    {
      g_warning ("Server is missing xdg_foreign support");
      return FALSE;
    }

  if (!impl->display_server.xdg_exported)
    {
      struct zxdg_exported_v1 *xdg_exported;

      xdg_exported = zxdg_exporter_v1_export (display_wayland->xdg_exporter,
                                              impl->display_server.wl_surface);
      zxdg_exported_v1_add_listener (xdg_exported,
                                     &xdg_exported_listener,
                                     window);

      impl->display_server.xdg_exported = xdg_exported;
    }

  closure = g_new0 (ExportedClosure, 1);
  closure->callback = callback;
  closure->user_data = user_data;
  closure->destroy_func = destroy_func;

  impl->exported.closures = g_list_append (impl->exported.closures, closure);
  impl->exported.export_count++;

  if (impl->exported.handle && !impl->exported.idle_source_id)
    impl->exported.idle_source_id = g_idle_add (exported_idle, window);

  return TRUE;
}

static void
cdk_wayland_window_unexport (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  GList *l;

  g_clear_pointer (&impl->display_server.xdg_exported,
                   zxdg_exported_v1_destroy);

  for (l = impl->exported.closures; l; l = l->next)
    {
      ExportedClosure *closure = l->data;

      if (closure->destroy_func)
        closure->destroy_func (closure->user_data);
    }

  g_list_free_full (impl->exported.closures, g_free);
  impl->exported.closures = NULL;
  g_clear_pointer (&impl->exported.handle, g_free);

  if (impl->exported.idle_source_id)
    {
      g_source_remove (impl->exported.idle_source_id);
      impl->exported.idle_source_id = 0;
    }
}

/**
 * cdk_wayland_window_unexport_handle:
 * @window: the #CdkWindow to unexport
 *
 * Destroys the handle that was obtained with
 * cdk_wayland_window_export_handle().
 *
 * It is an error to call this function on a window that
 * does not have a handle.
 *
 * Note that this API depends on an unstable Wayland protocol,
 * and thus may require changes in the future.
 *
 * Since: 3.22
 */
void
cdk_wayland_window_unexport_handle (CdkWindow *window)
{
  CdkWindowImplWayland *impl;

  g_return_if_fail (CDK_IS_WAYLAND_WINDOW (window));

  impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  g_return_if_fail (impl->display_server.xdg_exported);

  impl->exported.export_count--;
  if (impl->exported.export_count == 0)
    cdk_wayland_window_unexport (window);
}

static void
unset_transient_for_exported (CdkWindow *window)
{
  CdkWindowImplWayland *impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);

  g_clear_pointer (&impl->imported_transient_for, zxdg_imported_v1_destroy);
}

static void
xdg_imported_destroyed (void                    *data,
                        struct zxdg_imported_v1 *zxdg_imported_v1)
{
  CdkWindow *window = data;

  unset_transient_for_exported (window);
}

static const struct zxdg_imported_v1_listener xdg_imported_listener = {
  xdg_imported_destroyed,
};

/**
 * cdk_wayland_window_set_transient_for_exported:
 * @window: the #CdkWindow to make as transient
 * @parent_handle_str: an exported handle for a surface
 *
 * Marks @window as transient for the surface to which the given
 * @parent_handle_str refers. Typically, the handle will originate
 * from a cdk_wayland_window_export_handle() call in another process.
 *
 * Note that this API depends on an unstable Wayland protocol,
 * and thus may require changes in the future.
 *
 * Return value: %TRUE if the window has been marked as transient,
 *     %FALSE if an error occurred.
 *
 * Since: 3.22
 */
gboolean
cdk_wayland_window_set_transient_for_exported (CdkWindow *window,
                                               char      *parent_handle_str)
{
  CdkWindowImplWayland *impl;
  CdkWaylandDisplay *display_wayland;
  CdkDisplay *display = cdk_window_get_display (window);

  g_return_val_if_fail (CDK_IS_WAYLAND_WINDOW (window), FALSE);
  g_return_val_if_fail (CDK_IS_WAYLAND_DISPLAY (display), FALSE);
  g_return_val_if_fail (!should_map_as_subsurface (window) &&
                        !should_map_as_popup (window), FALSE);

  impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  display_wayland = CDK_WAYLAND_DISPLAY (display);

  if (!display_wayland->xdg_importer)
    {
      g_warning ("Server is missing xdg_foreign support");
      return FALSE;
    }

  cdk_window_set_transient_for (window, NULL);

  impl->imported_transient_for =
    zxdg_importer_v1_import (display_wayland->xdg_importer, parent_handle_str);
  zxdg_imported_v1_add_listener (impl->imported_transient_for,
                                 &xdg_imported_listener,
                                 window);

  cdk_wayland_window_sync_parent_of_imported (window);

  return TRUE;
}

static struct zwp_keyboard_shortcuts_inhibitor_v1 *
cdk_wayland_window_get_inhibitor (CdkWindowImplWayland *impl,
                                  struct wl_seat *seat)
{
  return g_hash_table_lookup (impl->shortcuts_inhibitors, seat);
}

void
cdk_wayland_window_inhibit_shortcuts (CdkWindow *window,
                                      CdkSeat   *cdk_seat)
{
  CdkWindowImplWayland *impl= CDK_WINDOW_IMPL_WAYLAND (window->impl);
  CdkWaylandDisplay *display = CDK_WAYLAND_DISPLAY (cdk_window_get_display (window));
  struct wl_surface *surface = impl->display_server.wl_surface;
  struct wl_seat *seat = cdk_wayland_seat_get_wl_seat (cdk_seat);
  struct zwp_keyboard_shortcuts_inhibitor_v1 *inhibitor;

  if (display->keyboard_shortcuts_inhibit == NULL)
    return;

  if (cdk_wayland_window_get_inhibitor (impl, seat))
    return; /* Already inhibitted */

  inhibitor =
      zwp_keyboard_shortcuts_inhibit_manager_v1_inhibit_shortcuts (
          display->keyboard_shortcuts_inhibit, surface, seat);

  g_hash_table_insert (impl->shortcuts_inhibitors, seat, inhibitor);
}

void
cdk_wayland_window_restore_shortcuts (CdkWindow *window,
                                      CdkSeat   *cdk_seat)
{
  CdkWindowImplWayland *impl;
  struct wl_seat *seat;
  struct zwp_keyboard_shortcuts_inhibitor_v1 *inhibitor;

  g_return_if_fail (CDK_IS_WAYLAND_WINDOW (window));
  g_return_if_fail (CDK_IS_WINDOW_IMPL_WAYLAND (window->impl));

  impl = CDK_WINDOW_IMPL_WAYLAND (window->impl);
  seat = cdk_wayland_seat_get_wl_seat (cdk_seat);

  inhibitor = cdk_wayland_window_get_inhibitor (impl, seat);
  if (inhibitor == NULL)
    return; /* Not inhibitted */

  zwp_keyboard_shortcuts_inhibitor_v1_destroy (inhibitor);
  g_hash_table_remove (impl->shortcuts_inhibitors, seat);
}

