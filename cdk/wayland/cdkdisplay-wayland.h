/*
 * cdkdisplay-wayland.h
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

#ifndef __CDK_WAYLAND_DISPLAY__
#define __CDK_WAYLAND_DISPLAY__

#include <config.h>
#include <stdint.h>
#include <wayland-client.h>
#include <wayland-cursor.h>
#include <wayland-egl.h>
#include <cdk/wayland/tablet-unstable-v2-client-protocol.h>
#include <cdk/wayland/ctk-shell-client-protocol.h>
#include <cdk/wayland/xdg-shell-client-protocol.h>
#include <cdk/wayland/xdg-shell-unstable-v6-client-protocol.h>
#include <cdk/wayland/xdg-foreign-unstable-v1-client-protocol.h>
#include <cdk/wayland/keyboard-shortcuts-inhibit-unstable-v1-client-protocol.h>
#include <cdk/wayland/server-decoration-client-protocol.h>
#include <cdk/wayland/xdg-output-unstable-v1-client-protocol.h>
#include <cdk/wayland/primary-selection-unstable-v1-client-protocol.h>

#include <glib.h>
#include <cdk/cdkkeys.h>
#include <cdk/cdkwindow.h>
#include <cdk/cdkinternals.h>
#include <cdk/cdk.h>		/* For cdk_get_program_class() */

#include "cdkdisplayprivate.h"

#include <epoxy/egl.h>

G_BEGIN_DECLS

#define CDK_WAYLAND_MAX_THEME_SCALE 4
#define CDK_WAYLAND_THEME_SCALES_COUNT CDK_WAYLAND_MAX_THEME_SCALE

#define CDK_ZWP_POINTER_GESTURES_V1_VERSION 1

typedef struct _CdkWaylandSelection CdkWaylandSelection;

typedef enum _CdkWaylandShellVariant
{
  CDK_WAYLAND_SHELL_VARIANT_XDG_SHELL,
  CDK_WAYLAND_SHELL_VARIANT_ZXDG_SHELL_V6,
} CdkWaylandShellVariant;

struct _CdkWaylandDisplay
{
  CdkDisplay parent_instance;
  CdkScreen *screen;

  /* Startup notification */
  gchar *startup_notification_id;

  /* Most recent serial */
  guint32 serial;

  uint32_t xdg_wm_base_id;
  uint32_t zxdg_shell_v6_id;
  CdkWaylandShellVariant shell_variant;

  /* Wayland fields below */
  struct wl_display *wl_display;
  struct wl_registry *wl_registry;
  struct wl_compositor *compositor;
  struct wl_shm *shm;
  struct xdg_wm_base *xdg_wm_base;
  struct zxdg_shell_v6 *zxdg_shell_v6;
  struct ctk_shell1 *ctk_shell;
  struct wl_input_device *input_device;
  struct wl_data_device_manager *data_device_manager;
  struct wl_subcompositor *subcompositor;
  struct zwp_pointer_gestures_v1 *pointer_gestures;
  struct ctk_primary_selection_device_manager *ctk_primary_selection_manager;
  struct zwp_primary_selection_device_manager_v1 *zwp_primary_selection_manager_v1;
  struct zwp_tablet_manager_v2 *tablet_manager;
  struct zxdg_exporter_v1 *xdg_exporter;
  struct zxdg_importer_v1 *xdg_importer;
  struct zwp_keyboard_shortcuts_inhibit_manager_v1 *keyboard_shortcuts_inhibit;
  struct org_kde_kwin_server_decoration_manager *server_decoration_manager;
  struct zxdg_output_manager_v1 *xdg_output_manager;
  uint32_t xdg_output_version;

  GList *async_roundtrips;

  /* Keep track of the ID's of the known globals and their corresponding
   * names. This way we can check whether an interface is known, and
   * remove globals given its ID. This table is not expected to be very
   * large, meaning the lookup by interface name time is insignificant. */
  GHashTable *known_globals;
  GList *on_has_globals_closures;

  /* Keep a list of orphaned dialogs (i.e. without parent) */
  GList *orphan_dialogs;

  GList *current_popups;

  struct wl_cursor_theme *scaled_cursor_themes[CDK_WAYLAND_THEME_SCALES_COUNT];
  gchar *cursor_theme_name;
  int cursor_theme_size;
  GHashTable *cursor_cache;

  GSource *event_source;

  int compositor_version;
  int seat_version;
  int data_device_manager_version;
  int ctk_shell_version;
  int xdg_output_manager_version;

  uint32_t server_decoration_mode;

  struct xkb_context *xkb_context;

  CdkWaylandSelection *selection;

  GPtrArray *monitors;

  gint64 last_bell_time_ms;

  /* egl info */
  EGLDisplay egl_display;
  int egl_major_version;
  int egl_minor_version;

  guint have_egl : 1;
  guint have_egl_khr_create_context : 1;
  guint have_egl_buffer_age : 1;
  guint have_egl_swap_buffers_with_damage : 1;
  guint have_egl_surfaceless_context : 1;
  EGLint egl_min_swap_interval;
};

struct _CdkWaylandDisplayClass
{
  CdkDisplayClass parent_class;
};

G_END_DECLS

#endif  /* __CDK_WAYLAND_DISPLAY__ */
