/*
 * Copyright © 2010 Intel Corporation
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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef HAVE_LINUX_MEMFD_H
#include <linux/memfd.h>
#endif

#include <sys/mman.h>
#include <sys/syscall.h>

#include <glib.h>
#include "cdkwayland.h"
#include "cdkdisplay.h"
#include "cdkdisplay-wayland.h"
#include "cdkscreen.h"
#include "cdkinternals.h"
#include "cdkdeviceprivate.h"
#include "cdkdevicemanager.h"
#include "cdkkeysprivate.h"
#include "cdkprivate-wayland.h"
#include "cdkglcontext-wayland.h"
#include "cdkwaylandmonitor.h"
#include "cdk-private.h"
#include "pointer-gestures-unstable-v1-client-protocol.h"
#include "tablet-unstable-v2-client-protocol.h"
#include "xdg-shell-unstable-v6-client-protocol.h"
#include "xdg-foreign-unstable-v1-client-protocol.h"
#include "server-decoration-client-protocol.h"

/**
 * SECTION:wayland_interaction
 * @Short_description: Wayland backend-specific functions
 * @Title: Wayland Interaction
 *
 * The functions in this section are specific to the CDK Wayland backend.
 * To use them, you need to include the `<cdk/cdkwayland.h>` header and use
 * the Wayland-specific pkg-config files to build your application (either
 * `cdk-wayland-3.0` or `ctk+-wayland-3.0`).
 *
 * To make your code compile with other CDK backends, guard backend-specific
 * calls by an ifdef as follows. Since CDK may be built with multiple
 * backends, you should also check for the backend that is in use (e.g. by
 * using the CDK_IS_WAYLAND_DISPLAY() macro).
 * |[<!-- language="C" -->
 * #ifdef CDK_WINDOWING_WAYLAND
 *   if (CDK_IS_WAYLAND_DISPLAY (display))
 *     {
 *       // make Wayland-specific calls here
 *     }
 *   else
 * #endif
 * #ifdef CDK_WINDOWING_X11
 *   if (CDK_IS_X11_DISPLAY (display))
 *     {
 *       // make X11-specific calls here
 *     }
 *   else
 * #endif
 *   g_error ("Unsupported CDK backend");
 * ]|
 */

#define MIN_SYSTEM_BELL_DELAY_MS 20

#define CTK_SHELL1_VERSION       3

static void _cdk_wayland_display_load_cursor_theme (CdkWaylandDisplay *display_wayland);

G_DEFINE_TYPE (CdkWaylandDisplay, cdk_wayland_display, CDK_TYPE_DISPLAY)

static void
async_roundtrip_callback (void               *data,
                          struct wl_callback *callback,
                          uint32_t            time G_GNUC_UNUSED)
{
  CdkWaylandDisplay *display_wayland = data;

  display_wayland->async_roundtrips =
    g_list_remove (display_wayland->async_roundtrips, callback);
  wl_callback_destroy (callback);
}

static const struct wl_callback_listener async_roundtrip_listener = {
  async_roundtrip_callback
};

static void
_cdk_wayland_display_async_roundtrip (CdkWaylandDisplay *display_wayland)
{
  struct wl_callback *callback;

  callback = wl_display_sync (display_wayland->wl_display);
  wl_callback_add_listener (callback,
                            &async_roundtrip_listener,
                            display_wayland);
  display_wayland->async_roundtrips =
    g_list_append (display_wayland->async_roundtrips, callback);
}

static void
xdg_wm_base_ping (void               *data,
                  struct xdg_wm_base *xdg_wm_base,
                  uint32_t            serial)
{
  CdkWaylandDisplay *display_wayland = data;

  _cdk_wayland_display_update_serial (display_wayland, serial);

  CDK_NOTE (EVENTS,
            g_message ("ping, shell %p, serial %u\n", xdg_wm_base, serial));

  xdg_wm_base_pong (xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
  xdg_wm_base_ping,
};

static void
zxdg_shell_v6_ping (void                 *data,
                    struct zxdg_shell_v6 *xdg_shell,
                    uint32_t              serial)
{
  CdkWaylandDisplay *display_wayland = data;

  _cdk_wayland_display_update_serial (display_wayland, serial);

  CDK_NOTE (EVENTS,
            g_message ("ping, shell %p, serial %u\n", xdg_shell, serial));

  zxdg_shell_v6_pong (xdg_shell, serial);
}

static const struct zxdg_shell_v6_listener zxdg_shell_v6_listener = {
  zxdg_shell_v6_ping,
};

static gboolean
is_known_global (gpointer key G_GNUC_UNUSED,
		 gpointer value,
		 gpointer user_data)
{
  const char *required_global = user_data;
  const char *known_global = value;

  return g_strcmp0 (required_global, known_global) == 0;
}

static gboolean
has_required_globals (CdkWaylandDisplay *display_wayland,
                      const char *required_globals[])
{
  int i = 0;

  while (required_globals[i])
    {
      if (g_hash_table_find (display_wayland->known_globals,
                             is_known_global,
                             (gpointer)required_globals[i]) == NULL)
        return FALSE;

      i++;
    }

  return TRUE;
}

typedef struct _OnHasGlobalsClosure OnHasGlobalsClosure;

typedef void (*HasGlobalsCallback) (CdkWaylandDisplay *display_wayland,
                                    OnHasGlobalsClosure *closure);

struct _OnHasGlobalsClosure
{
  HasGlobalsCallback handler;
  const char **required_globals;
};

static void
process_on_globals_closures (CdkWaylandDisplay *display_wayland)
{
  GList *iter;

  iter = display_wayland->on_has_globals_closures;
  while (iter != NULL)
    {
      GList *next = iter->next;
      OnHasGlobalsClosure *closure = iter->data;

      if (has_required_globals (display_wayland,
                                closure->required_globals))
        {
          closure->handler (display_wayland, closure);
          g_free (closure);
          display_wayland->on_has_globals_closures =
            g_list_delete_link (display_wayland->on_has_globals_closures, iter);
        }

      iter = next;
    }
}

typedef struct
{
  OnHasGlobalsClosure base;
  uint32_t id;
  uint32_t version;
} SeatAddedClosure;

static void
_cdk_wayland_display_add_seat (CdkWaylandDisplay *display_wayland,
                               uint32_t id,
                               uint32_t version)
{
  CdkDisplay *cdk_display = CDK_DISPLAY_OBJECT (display_wayland);
  struct wl_seat *seat;

  display_wayland->seat_version = MIN (version, 5);
  seat = wl_registry_bind (display_wayland->wl_registry,
                           id, &wl_seat_interface,
                           display_wayland->seat_version);
  _cdk_wayland_device_manager_add_seat (cdk_display->device_manager,
                                        id, seat);
  _cdk_wayland_display_async_roundtrip (display_wayland);
}

static void
seat_added_closure_run (CdkWaylandDisplay *display_wayland,
                        OnHasGlobalsClosure *closure)
{
  SeatAddedClosure *seat_added_closure = (SeatAddedClosure*)closure;

  _cdk_wayland_display_add_seat (display_wayland,
                                 seat_added_closure->id,
                                 seat_added_closure->version);
}

static void
postpone_on_globals_closure (CdkWaylandDisplay *display_wayland,
                             OnHasGlobalsClosure *closure)
{
  display_wayland->on_has_globals_closures =
    g_list_append (display_wayland->on_has_globals_closures, closure);
}

#ifdef G_ENABLE_DEBUG

static const char *
get_format_name (enum wl_shm_format format)
{
  int i;
#define FORMAT(s) { WL_SHM_FORMAT_ ## s, #s }
  struct { int format; const char *name; } formats[] = {
    FORMAT(ARGB8888),
    FORMAT(XRGB8888),
    FORMAT(C8),
    FORMAT(RGB332),
    FORMAT(BGR233),
    FORMAT(XRGB4444),
    FORMAT(XBGR4444),
    FORMAT(RGBX4444),
    FORMAT(BGRX4444),
    FORMAT(ARGB4444),
    FORMAT(ABGR4444),
    FORMAT(RGBA4444),
    FORMAT(BGRA4444),
    FORMAT(XRGB1555),
    FORMAT(XBGR1555),
    FORMAT(RGBX5551),
    FORMAT(BGRX5551),
    FORMAT(ARGB1555),
    FORMAT(ABGR1555),
    FORMAT(RGBA5551),
    FORMAT(BGRA5551),
    FORMAT(RGB565),
    FORMAT(BGR565),
    FORMAT(RGB888),
    FORMAT(BGR888),
    FORMAT(XBGR8888),
    FORMAT(RGBX8888),
    FORMAT(BGRX8888),
    FORMAT(ABGR8888),
    FORMAT(RGBA8888),
    FORMAT(BGRA8888),
    FORMAT(XRGB2101010),
    FORMAT(XBGR2101010),
    FORMAT(RGBX1010102),
    FORMAT(BGRX1010102),
    FORMAT(ARGB2101010),
    FORMAT(ABGR2101010),
    FORMAT(RGBA1010102),
    FORMAT(BGRA1010102),
    FORMAT(YUYV),
    FORMAT(YVYU),
    FORMAT(UYVY),
    FORMAT(VYUY),
    FORMAT(AYUV),
    FORMAT(NV12),
    FORMAT(NV21),
    FORMAT(NV16),
    FORMAT(NV61),
    FORMAT(YUV410),
    FORMAT(YVU410),
    FORMAT(YUV411),
    FORMAT(YVU411),
    FORMAT(YUV420),
    FORMAT(YVU420),
    FORMAT(YUV422),
    FORMAT(YVU422),
    FORMAT(YUV444),
    FORMAT(YVU444),
    { 0xffffffff, NULL }
  };
#undef FORMAT

  for (i = 0; formats[i].name; i++)
    {
      if (formats[i].format == format)
        return formats[i].name;
    }
  return NULL;
}
#endif

static void
wl_shm_format (void          *data G_GNUC_UNUSED,
               struct wl_shm *wl_shm G_GNUC_UNUSED,
               uint32_t       format)
{
  CDK_NOTE (MISC, g_message ("supported pixel format %s", get_format_name (format)));
}

static const struct wl_shm_listener wl_shm_listener = {
  wl_shm_format
};

static void
server_decoration_manager_default_mode (void                                          *data,
                                        struct org_kde_kwin_server_decoration_manager *manager G_GNUC_UNUSED,
                                        uint32_t                                       mode)
{
  g_assert (mode <= ORG_KDE_KWIN_SERVER_DECORATION_MANAGER_MODE_SERVER);
  const char *modes[] = {
    [ORG_KDE_KWIN_SERVER_DECORATION_MANAGER_MODE_NONE]   = "none",
    [ORG_KDE_KWIN_SERVER_DECORATION_MANAGER_MODE_CLIENT] = "client",
    [ORG_KDE_KWIN_SERVER_DECORATION_MANAGER_MODE_SERVER] = "server",
  };
  CdkWaylandDisplay *display_wayland = data;
  g_debug ("Compositor prefers decoration mode '%s'", modes[mode]);
  display_wayland->server_decoration_mode = mode;
}

static const struct org_kde_kwin_server_decoration_manager_listener server_decoration_listener = {
  .default_mode = server_decoration_manager_default_mode
};

gboolean
cdk_wayland_display_prefers_ssd (CdkDisplay *display)
{
  CdkWaylandDisplay *display_wayland = CDK_WAYLAND_DISPLAY (display);
  if (display_wayland->server_decoration_manager)
    return display_wayland->server_decoration_mode == ORG_KDE_KWIN_SERVER_DECORATION_MANAGER_MODE_SERVER;
  return FALSE;
}

static void
cdk_registry_handle_global (void               *data,
                            struct wl_registry *registry G_GNUC_UNUSED,
                            uint32_t            id,
                            const char         *interface,
                            uint32_t            version)
{
  CdkWaylandDisplay *display_wayland = data;
  struct wl_output *output;

  CDK_NOTE (MISC,
            g_message ("add global %u, interface %s, version %u", id, interface, version));

  if (strcmp (interface, "wl_compositor") == 0)
    {
      display_wayland->compositor =
        wl_registry_bind (display_wayland->wl_registry, id, &wl_compositor_interface, MIN (version, 3));
      display_wayland->compositor_version = MIN (version, 3);
    }
  else if (strcmp (interface, "wl_shm") == 0)
    {
      display_wayland->shm =
        wl_registry_bind (display_wayland->wl_registry, id, &wl_shm_interface, 1);
      wl_shm_add_listener (display_wayland->shm, &wl_shm_listener, display_wayland);
    }
  else if (strcmp (interface, "xdg_wm_base") == 0)
    {
      display_wayland->xdg_wm_base_id = id;
    }
  else if (strcmp (interface, "zxdg_shell_v6") == 0)
    {
      display_wayland->zxdg_shell_v6_id = id;
    }
  else if (strcmp (interface, "ctk_shell1") == 0)
    {
      display_wayland->ctk_shell =
        wl_registry_bind(display_wayland->wl_registry, id,
                         &ctk_shell1_interface,
                         MIN (version, CTK_SHELL1_VERSION));
      _cdk_wayland_screen_set_has_ctk_shell (display_wayland->screen);
      display_wayland->ctk_shell_version = version;
    }
  else if (strcmp (interface, "wl_output") == 0)
    {
      output =
        wl_registry_bind (display_wayland->wl_registry, id, &wl_output_interface, MIN (version, 2));
      _cdk_wayland_screen_add_output (display_wayland->screen, id, output, MIN (version, 2));
      _cdk_wayland_display_async_roundtrip (display_wayland);
    }
  else if (strcmp (interface, "wl_seat") == 0)
    {
      static const char *required_device_manager_globals[] = {
        "wl_compositor",
        "wl_data_device_manager",
        NULL
      };

      if (has_required_globals (display_wayland,
                                required_device_manager_globals))
        _cdk_wayland_display_add_seat (display_wayland, id, version);
      else
        {
          SeatAddedClosure *closure;

          closure = g_new0 (SeatAddedClosure, 1);
          closure->base.handler = seat_added_closure_run;
          closure->base.required_globals = required_device_manager_globals;
          closure->id = id;
          closure->version = version;
          postpone_on_globals_closure (display_wayland, &closure->base);
        }
    }
  else if (strcmp (interface, "wl_data_device_manager") == 0)
    {
      display_wayland->data_device_manager_version = MIN (version, 3);
      display_wayland->data_device_manager =
        wl_registry_bind (display_wayland->wl_registry, id, &wl_data_device_manager_interface,
                          display_wayland->data_device_manager_version);
    }
  else if (strcmp (interface, "wl_subcompositor") == 0)
    {
      display_wayland->subcompositor =
        wl_registry_bind (display_wayland->wl_registry, id, &wl_subcompositor_interface, 1);
    }
  else if (strcmp (interface, "zwp_pointer_gestures_v1") == 0 &&
           version == CDK_ZWP_POINTER_GESTURES_V1_VERSION)
    {
      display_wayland->pointer_gestures =
        wl_registry_bind (display_wayland->wl_registry,
                          id, &zwp_pointer_gestures_v1_interface, version);
    }
  else if (strcmp (interface, "ctk_primary_selection_device_manager") == 0)
    {
      display_wayland->ctk_primary_selection_manager =
        wl_registry_bind(display_wayland->wl_registry, id,
                         &ctk_primary_selection_device_manager_interface, 1);
    }
  else if (strcmp (interface, "zwp_primary_selection_device_manager_v1") == 0)
    {
      display_wayland->zwp_primary_selection_manager_v1 =
        wl_registry_bind(display_wayland->wl_registry, id,
                         &zwp_primary_selection_device_manager_v1_interface, 1);
    }
  else if (strcmp (interface, "zwp_tablet_manager_v2") == 0)
    {
      display_wayland->tablet_manager =
        wl_registry_bind(display_wayland->wl_registry, id,
                         &zwp_tablet_manager_v2_interface, 1);
    }
  else if (strcmp (interface, "zxdg_exporter_v1") == 0)
    {
      display_wayland->xdg_exporter =
        wl_registry_bind (display_wayland->wl_registry, id,
                          &zxdg_exporter_v1_interface, 1);
    }
  else if (strcmp (interface, "zxdg_importer_v1") == 0)
    {
      display_wayland->xdg_importer =
        wl_registry_bind (display_wayland->wl_registry, id,
                          &zxdg_importer_v1_interface, 1);
    }
  else if (strcmp (interface, "zwp_keyboard_shortcuts_inhibit_manager_v1") == 0)
    {
      display_wayland->keyboard_shortcuts_inhibit =
        wl_registry_bind (display_wayland->wl_registry, id,
                          &zwp_keyboard_shortcuts_inhibit_manager_v1_interface, 1);
    }
  else if (strcmp (interface, "org_kde_kwin_server_decoration_manager") == 0)
    {
      display_wayland->server_decoration_manager =
        wl_registry_bind (display_wayland->wl_registry, id,
                          &org_kde_kwin_server_decoration_manager_interface, 1);
      org_kde_kwin_server_decoration_manager_add_listener (display_wayland->server_decoration_manager,
                                                           &server_decoration_listener,
                                                           display_wayland);
    }
  else if (strcmp(interface, "zxdg_output_manager_v1") == 0)
    {
      display_wayland->xdg_output_manager_version = MIN (version, 3);
      display_wayland->xdg_output_manager =
        wl_registry_bind (display_wayland->wl_registry, id,
                          &zxdg_output_manager_v1_interface,
                          display_wayland->xdg_output_manager_version);
      display_wayland->xdg_output_version = version;
      _cdk_wayland_screen_init_xdg_output (display_wayland->screen);
      _cdk_wayland_display_async_roundtrip (display_wayland);
    }

  g_hash_table_insert (display_wayland->known_globals,
                       GUINT_TO_POINTER (id), g_strdup (interface));

  process_on_globals_closures (display_wayland);
}

static void
cdk_registry_handle_global_remove (void               *data,
                                   struct wl_registry *registry G_GNUC_UNUSED,
                                   uint32_t            id)
{
  CdkWaylandDisplay *display_wayland = data;
  CdkDisplay *display = CDK_DISPLAY (display_wayland);

  CDK_NOTE (MISC, g_message ("remove global %u", id));
  _cdk_wayland_device_manager_remove_seat (display->device_manager, id);
  _cdk_wayland_screen_remove_output (display_wayland->screen, id);

  g_hash_table_remove (display_wayland->known_globals, GUINT_TO_POINTER (id));

  /* FIXME: the object needs to be destroyed here, we're leaking */
}

static const struct wl_registry_listener registry_listener = {
    cdk_registry_handle_global,
    cdk_registry_handle_global_remove
};

static void
log_handler (const char *format, va_list args)
{
  g_logv (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, format, args);
}

static void
load_cursor_theme_closure_run (CdkWaylandDisplay   *display_wayland,
                               OnHasGlobalsClosure *closure G_GNUC_UNUSED)
{
  _cdk_wayland_display_load_cursor_theme (display_wayland);
}

static void
_cdk_wayland_display_prepare_cursor_themes (CdkWaylandDisplay *display_wayland)
{
  OnHasGlobalsClosure *closure;
  static const char *required_cursor_theme_globals[] = {
      "wl_shm",
      NULL
  };

  closure = g_new0 (OnHasGlobalsClosure, 1);
  closure->handler = load_cursor_theme_closure_run;
  closure->required_globals = required_cursor_theme_globals;
  postpone_on_globals_closure (display_wayland, closure);
}

CdkDisplay *
_cdk_wayland_display_open (const gchar *display_name)
{
  struct wl_display *wl_display;
  CdkDisplay *display;
  CdkWaylandDisplay *display_wayland;

  CDK_NOTE (MISC, g_message ("opening display %s", display_name ? display_name : ""));

  /* If this variable is unset then wayland initialisation will surely
   * fail, logging a fatal error in the process.  Save ourselves from
   * that.
   */
  if (g_getenv ("XDG_RUNTIME_DIR") == NULL)
    return NULL;

  wl_log_set_handler_client (log_handler);

  wl_display = wl_display_connect (display_name);
  if (!wl_display)
    return NULL;

  display = g_object_new (CDK_TYPE_WAYLAND_DISPLAY, NULL);
  display->device_manager = _cdk_wayland_device_manager_new (display);

  display_wayland = CDK_WAYLAND_DISPLAY (display);
  display_wayland->wl_display = wl_display;
  display_wayland->screen = _cdk_wayland_screen_new (display);
  display_wayland->event_source = _cdk_wayland_display_event_source_new (display);

  display_wayland->known_globals =
    g_hash_table_new_full (NULL, NULL, NULL, g_free);

  _cdk_wayland_display_init_cursors (display_wayland);
  _cdk_wayland_display_prepare_cursor_themes (display_wayland);

  display_wayland->wl_registry = wl_display_get_registry (display_wayland->wl_display);
  wl_registry_add_listener (display_wayland->wl_registry, &registry_listener, display_wayland);

  _cdk_wayland_display_async_roundtrip (display_wayland);

  /* Wait for initializing to complete. This means waiting for all
   * asynchrounous roundtrips that were triggered during initial roundtrip. */
  while (g_list_length (display_wayland->async_roundtrips) > 0)
    {
      if (wl_display_dispatch (display_wayland->wl_display) < 0)
        {
          g_object_unref (display);
          return NULL;
        }
    }

  if (display_wayland->xdg_wm_base_id)
    {
      display_wayland->shell_variant = CDK_WAYLAND_SHELL_VARIANT_XDG_SHELL;
      display_wayland->xdg_wm_base =
        wl_registry_bind (display_wayland->wl_registry,
                          display_wayland->xdg_wm_base_id,
                          &xdg_wm_base_interface, 1);
      xdg_wm_base_add_listener (display_wayland->xdg_wm_base,
                                &xdg_wm_base_listener,
                                display_wayland);
    }
  else if (display_wayland->zxdg_shell_v6_id)
    {
      display_wayland->shell_variant = CDK_WAYLAND_SHELL_VARIANT_ZXDG_SHELL_V6;
      display_wayland->zxdg_shell_v6 =
        wl_registry_bind (display_wayland->wl_registry,
                          display_wayland->zxdg_shell_v6_id,
                          &zxdg_shell_v6_interface, 1);
      zxdg_shell_v6_add_listener (display_wayland->zxdg_shell_v6,
                                  &zxdg_shell_v6_listener,
                                  display_wayland);
    }
  else
    {
      g_warning ("The Wayland compositor does not provide any supported shell interface, "
                 "not using Wayland display");
      g_object_unref (display);

      return NULL;
    }

  display_wayland->selection = cdk_wayland_selection_new ();

  g_signal_emit_by_name (display, "opened");

  return display;
}

static void
cdk_wayland_display_dispose (GObject *object)
{
  CdkWaylandDisplay *display_wayland = CDK_WAYLAND_DISPLAY (object);

  _cdk_screen_close (display_wayland->screen);

  if (display_wayland->event_source)
    {
      g_source_destroy (display_wayland->event_source);
      g_source_unref (display_wayland->event_source);
      display_wayland->event_source = NULL;
    }

  if (display_wayland->selection)
    {
      cdk_wayland_selection_free (display_wayland->selection);
      display_wayland->selection = NULL;
    }

  g_list_free_full (display_wayland->async_roundtrips, (GDestroyNotify) wl_callback_destroy);

  if (display_wayland->known_globals)
    {
      g_hash_table_destroy (display_wayland->known_globals);
      display_wayland->known_globals = NULL;
    }

  g_list_free_full (display_wayland->on_has_globals_closures, g_free);

  G_OBJECT_CLASS (cdk_wayland_display_parent_class)->dispose (object);
}

static void
cdk_wayland_display_finalize (GObject *object)
{
  CdkWaylandDisplay *display_wayland = CDK_WAYLAND_DISPLAY (object);
  guint i;

  _cdk_wayland_display_finalize_cursors (display_wayland);

  g_object_unref (display_wayland->screen);

  g_free (display_wayland->startup_notification_id);
  g_free (display_wayland->cursor_theme_name);
  xkb_context_unref (display_wayland->xkb_context);

  for (i = 0; i < CDK_WAYLAND_THEME_SCALES_COUNT; i++)
    {
      if (display_wayland->scaled_cursor_themes[i])
        {
          wl_cursor_theme_destroy (display_wayland->scaled_cursor_themes[i]);
          display_wayland->scaled_cursor_themes[i] = NULL;
        }
    }

  g_ptr_array_free (display_wayland->monitors, TRUE);

  wl_display_disconnect(display_wayland->wl_display);

  G_OBJECT_CLASS (cdk_wayland_display_parent_class)->finalize (object);
}

static const gchar *
cdk_wayland_display_get_name (CdkDisplay *display G_GNUC_UNUSED)
{
  const gchar *name;

  name = g_getenv ("WAYLAND_DISPLAY");
  if (name == NULL)
    name = "wayland-0";

  return name;
}

static CdkScreen *
cdk_wayland_display_get_default_screen (CdkDisplay *display)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), NULL);

  return CDK_WAYLAND_DISPLAY (display)->screen;
}

void
cdk_wayland_display_system_bell (CdkDisplay *display,
                                 CdkWindow  *window)
{
  CdkWaylandDisplay *display_wayland;
  struct ctk_surface1 *ctk_surface;
  gint64 now_ms;

  g_return_if_fail (CDK_IS_DISPLAY (display));

  display_wayland = CDK_WAYLAND_DISPLAY (display);

  if (!display_wayland->ctk_shell)
    return;

  if (window)
    ctk_surface = cdk_wayland_window_get_ctk_surface (window);
  else
    ctk_surface = NULL;

  now_ms = g_get_monotonic_time () / 1000;
  if (now_ms - display_wayland->last_bell_time_ms < MIN_SYSTEM_BELL_DELAY_MS)
    return;

  display_wayland->last_bell_time_ms = now_ms;

  ctk_shell1_system_bell (display_wayland->ctk_shell, ctk_surface);
}

static void
cdk_wayland_display_beep (CdkDisplay *display)
{
  cdk_wayland_display_system_bell (display, NULL);
}

static void
cdk_wayland_display_sync (CdkDisplay *display)
{
  CdkWaylandDisplay *display_wayland;

  g_return_if_fail (CDK_IS_DISPLAY (display));

  display_wayland = CDK_WAYLAND_DISPLAY (display);

  wl_display_roundtrip (display_wayland->wl_display);
}

static void
cdk_wayland_display_flush (CdkDisplay *display)
{
  g_return_if_fail (CDK_IS_DISPLAY (display));

  if (!display->closed)
    wl_display_flush (CDK_WAYLAND_DISPLAY (display)->wl_display);
}

static void
cdk_wayland_display_make_default (CdkDisplay *display)
{
  CdkWaylandDisplay *display_wayland = CDK_WAYLAND_DISPLAY (display);
  const gchar *startup_id;

  g_free (display_wayland->startup_notification_id);
  display_wayland->startup_notification_id = NULL;

  startup_id = cdk_get_desktop_startup_id ();
  if (startup_id)
    display_wayland->startup_notification_id = g_strdup (startup_id);
}

static gboolean
cdk_wayland_display_has_pending (CdkDisplay *display G_GNUC_UNUSED)
{
  return FALSE;
}

static CdkWindow *
cdk_wayland_display_get_default_group (CdkDisplay *display)
{
  g_return_val_if_fail (CDK_IS_DISPLAY (display), NULL);

  return NULL;
}


static gboolean
cdk_wayland_display_supports_selection_notification (CdkDisplay *display G_GNUC_UNUSED)
{
  return FALSE;
}

static gboolean
cdk_wayland_display_request_selection_notification (CdkDisplay *display G_GNUC_UNUSED,
						    CdkAtom     selection G_GNUC_UNUSED)

{
    return FALSE;
}

static gboolean
cdk_wayland_display_supports_clipboard_persistence (CdkDisplay *display G_GNUC_UNUSED)
{
  return FALSE;
}

static void
cdk_wayland_display_store_clipboard (CdkDisplay    *display G_GNUC_UNUSED,
				     CdkWindow     *clipboard_window G_GNUC_UNUSED,
				     guint32        time_ G_GNUC_UNUSED,
				     const CdkAtom *targets G_GNUC_UNUSED,
				     gint           n_targets G_GNUC_UNUSED)
{
}

static gboolean
cdk_wayland_display_supports_shapes (CdkDisplay *display G_GNUC_UNUSED)
{
  return FALSE;
}

static gboolean
cdk_wayland_display_supports_input_shapes (CdkDisplay *display G_GNUC_UNUSED)
{
  return TRUE;
}

static gboolean
cdk_wayland_display_supports_composite (CdkDisplay *display G_GNUC_UNUSED)
{
  return FALSE;
}

static void
cdk_wayland_display_before_process_all_updates (CdkDisplay *display G_GNUC_UNUSED)
{
}

static void
cdk_wayland_display_after_process_all_updates (CdkDisplay *display G_GNUC_UNUSED)
{
  /* Post the damage here instead? */
}

static gulong
cdk_wayland_display_get_next_serial (CdkDisplay *display G_GNUC_UNUSED)
{
  static gulong serial = 0;
  return ++serial;
}

/**
 * cdk_wayland_display_set_startup_notification_id:
 * @display: (type CdkWaylandDisplay): a #CdkDisplay
 * @startup_id: the startup notification ID (must be valid utf8)
 *
 * Sets the startup notification ID for a display.
 *
 * This is usually taken from the value of the DESKTOP_STARTUP_ID
 * environment variable, but in some cases (such as the application not
 * being launched using exec()) it can come from other sources.
 *
 * The startup ID is also what is used to signal that the startup is
 * complete (for example, when opening a window or when calling
 * cdk_notify_startup_complete()).
 *
 * Since: 3.22
 **/
void
cdk_wayland_display_set_startup_notification_id (CdkDisplay *display,
                                                 const char *startup_id)
{
  CdkWaylandDisplay *display_wayland = CDK_WAYLAND_DISPLAY (display);

  g_free (display_wayland->startup_notification_id);
  display_wayland->startup_notification_id = g_strdup (startup_id);
}

static void
cdk_wayland_display_notify_startup_complete (CdkDisplay  *display,
					     const gchar *startup_id)
{
  CdkWaylandDisplay *display_wayland = CDK_WAYLAND_DISPLAY (display);

  if (startup_id == NULL)
    {
      startup_id = display_wayland->startup_notification_id;

      if (startup_id == NULL)
        return;
    }

  if (display_wayland->ctk_shell)
    ctk_shell1_set_startup_id (display_wayland->ctk_shell, startup_id);
}

static CdkKeymap *
_cdk_wayland_display_get_keymap (CdkDisplay *display)
{
  CdkDevice *core_keyboard = NULL;
  static CdkKeymap *tmp_keymap = NULL;

  core_keyboard = cdk_seat_get_keyboard (cdk_display_get_default_seat (display));

  if (core_keyboard && tmp_keymap)
    {
      g_object_unref (tmp_keymap);
      tmp_keymap = NULL;
    }

  if (core_keyboard)
    return _cdk_wayland_device_get_keymap (core_keyboard);

  if (!tmp_keymap)
    tmp_keymap = _cdk_wayland_keymap_new ();

  return tmp_keymap;
}

static void
cdk_wayland_display_push_error_trap (CdkDisplay *display G_GNUC_UNUSED)
{
}

static gint
cdk_wayland_display_pop_error_trap (CdkDisplay *display G_GNUC_UNUSED,
				    gboolean    ignored G_GNUC_UNUSED)
{
  return 0;
}

static int
cdk_wayland_display_get_n_monitors (CdkDisplay *display)
{
  CdkWaylandDisplay *display_wayland = CDK_WAYLAND_DISPLAY (display);

  return display_wayland->monitors->len;
}

static CdkMonitor *
cdk_wayland_display_get_monitor (CdkDisplay *display,
                                 int         monitor_num)
{
  CdkWaylandDisplay *display_wayland = CDK_WAYLAND_DISPLAY (display);

  if (monitor_num < 0 || monitor_num >= display_wayland->monitors->len)
    return NULL;

  return (CdkMonitor *)display_wayland->monitors->pdata[monitor_num];
}

static CdkMonitor *
cdk_wayland_display_get_monitor_at_window (CdkDisplay *display,
                                           CdkWindow  *window)
{
  CdkWaylandDisplay *display_wayland = CDK_WAYLAND_DISPLAY (display);
  struct wl_output *output;
  int i;

  g_return_val_if_fail (CDK_IS_WAYLAND_WINDOW (window), NULL);

  output = cdk_wayland_window_get_wl_output (window);
  if (output == NULL)
    return NULL;

  for (i = 0; i < display_wayland->monitors->len; i++)
    {
      CdkMonitor *monitor = display_wayland->monitors->pdata[i];

      if (cdk_wayland_monitor_get_wl_output (monitor) == output)
        return monitor;
    }

  return NULL;
}

static void
cdk_wayland_display_class_init (CdkWaylandDisplayClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  CdkDisplayClass *display_class = CDK_DISPLAY_CLASS (class);

  object_class->dispose = cdk_wayland_display_dispose;
  object_class->finalize = cdk_wayland_display_finalize;

  display_class->window_type = cdk_wayland_window_get_type ();
  display_class->get_name = cdk_wayland_display_get_name;
  display_class->get_default_screen = cdk_wayland_display_get_default_screen;
  display_class->beep = cdk_wayland_display_beep;
  display_class->sync = cdk_wayland_display_sync;
  display_class->flush = cdk_wayland_display_flush;
  display_class->make_default = cdk_wayland_display_make_default;
  display_class->has_pending = cdk_wayland_display_has_pending;
  display_class->queue_events = _cdk_wayland_display_queue_events;
  display_class->get_default_group = cdk_wayland_display_get_default_group;
  display_class->supports_selection_notification = cdk_wayland_display_supports_selection_notification;
  display_class->request_selection_notification = cdk_wayland_display_request_selection_notification;
  display_class->supports_clipboard_persistence = cdk_wayland_display_supports_clipboard_persistence;
  display_class->store_clipboard = cdk_wayland_display_store_clipboard;
  display_class->supports_shapes = cdk_wayland_display_supports_shapes;
  display_class->supports_input_shapes = cdk_wayland_display_supports_input_shapes;
  display_class->supports_composite = cdk_wayland_display_supports_composite;
  display_class->get_app_launch_context = _cdk_wayland_display_get_app_launch_context;
  display_class->get_default_cursor_size = _cdk_wayland_display_get_default_cursor_size;
  display_class->get_maximal_cursor_size = _cdk_wayland_display_get_maximal_cursor_size;
  display_class->get_cursor_for_type = _cdk_wayland_display_get_cursor_for_type;
  display_class->get_cursor_for_name = _cdk_wayland_display_get_cursor_for_name;
  display_class->get_cursor_for_surface = _cdk_wayland_display_get_cursor_for_surface;
  display_class->supports_cursor_alpha = _cdk_wayland_display_supports_cursor_alpha;
  display_class->supports_cursor_color = _cdk_wayland_display_supports_cursor_color;
  display_class->before_process_all_updates = cdk_wayland_display_before_process_all_updates;
  display_class->after_process_all_updates = cdk_wayland_display_after_process_all_updates;
  display_class->get_next_serial = cdk_wayland_display_get_next_serial;
  display_class->notify_startup_complete = cdk_wayland_display_notify_startup_complete;
  display_class->create_window_impl = _cdk_wayland_display_create_window_impl;
  display_class->get_keymap = _cdk_wayland_display_get_keymap;
  display_class->push_error_trap = cdk_wayland_display_push_error_trap;
  display_class->pop_error_trap = cdk_wayland_display_pop_error_trap;
  display_class->get_selection_owner = _cdk_wayland_display_get_selection_owner;
  display_class->set_selection_owner = _cdk_wayland_display_set_selection_owner;
  display_class->send_selection_notify = _cdk_wayland_display_send_selection_notify;
  display_class->get_selection_property = _cdk_wayland_display_get_selection_property;
  display_class->convert_selection = _cdk_wayland_display_convert_selection;
  display_class->text_property_to_utf8_list = _cdk_wayland_display_text_property_to_utf8_list;
  display_class->utf8_to_string_target = _cdk_wayland_display_utf8_to_string_target;

  display_class->make_gl_context_current = cdk_wayland_display_make_gl_context_current;

  display_class->get_n_monitors = cdk_wayland_display_get_n_monitors;
  display_class->get_monitor = cdk_wayland_display_get_monitor;
  display_class->get_monitor_at_window = cdk_wayland_display_get_monitor_at_window;
}

static void
cdk_wayland_display_init (CdkWaylandDisplay *display)
{
  display->xkb_context = xkb_context_new (0);

  display->monitors = g_ptr_array_new_with_free_func (g_object_unref);
}

void
cdk_wayland_display_set_cursor_theme (CdkDisplay  *display,
                                      const gchar *name,
                                      gint         size)
{
  CdkWaylandDisplay *display_wayland = CDK_WAYLAND_DISPLAY(display);
  struct wl_cursor_theme *theme;
  int i;

  g_assert (display_wayland);
  g_assert (display_wayland->shm);

  if (g_strcmp0 (name, display_wayland->cursor_theme_name) == 0 &&
      display_wayland->cursor_theme_size == size)
    return;

  theme = wl_cursor_theme_load (name, size, display_wayland->shm);
  if (theme == NULL)
    {
      g_warning ("Failed to load cursor theme %s", name);
      return;
    }

  for (i = 0; i < CDK_WAYLAND_THEME_SCALES_COUNT; i++)
    {
      if (display_wayland->scaled_cursor_themes[i])
        {
          wl_cursor_theme_destroy (display_wayland->scaled_cursor_themes[i]);
          display_wayland->scaled_cursor_themes[i] = NULL;
        }
    }
  display_wayland->scaled_cursor_themes[0] = theme;
  if (display_wayland->cursor_theme_name != NULL)
    g_free (display_wayland->cursor_theme_name);
  display_wayland->cursor_theme_name = g_strdup (name);
  display_wayland->cursor_theme_size = size;

  _cdk_wayland_display_update_cursors (display_wayland);
}

struct wl_cursor_theme *
_cdk_wayland_display_get_scaled_cursor_theme (CdkWaylandDisplay *display_wayland,
                                              guint              scale)
{
  struct wl_cursor_theme *theme;

  g_assert (display_wayland->cursor_theme_name);
  g_assert (scale <= CDK_WAYLAND_MAX_THEME_SCALE);
  g_assert (scale >= 1);

  theme = display_wayland->scaled_cursor_themes[scale - 1];
  if (!theme)
    {
      theme = wl_cursor_theme_load (display_wayland->cursor_theme_name,
                                    display_wayland->cursor_theme_size * scale,
                                    display_wayland->shm);
      if (theme == NULL)
        {
          g_warning ("Failed to load cursor theme %s with scale %u",
                     display_wayland->cursor_theme_name, scale);
          return NULL;
        }
      display_wayland->scaled_cursor_themes[scale - 1] = theme;
    }

  return theme;
}

static void
_cdk_wayland_display_load_cursor_theme (CdkWaylandDisplay *display_wayland)
{
  guint size;
  const gchar *name;
  GValue v = G_VALUE_INIT;

  g_assert (display_wayland);
  g_assert (display_wayland->shm);

  g_value_init (&v, G_TYPE_INT);
  if (cdk_screen_get_setting (display_wayland->screen, "ctk-cursor-theme-size", &v))
    size = g_value_get_int (&v);
  else
    size = 32;
  g_value_unset (&v);

  g_value_init (&v, G_TYPE_STRING);
  if (cdk_screen_get_setting (display_wayland->screen, "ctk-cursor-theme-name", &v))
    name = g_value_get_string (&v);
  else
    name = "default";

  cdk_wayland_display_set_cursor_theme (CDK_DISPLAY (display_wayland), name, size);
  g_value_unset (&v);
}

guint32
_cdk_wayland_display_get_serial (CdkWaylandDisplay *display_wayland)
{
  return display_wayland->serial;
}

void
_cdk_wayland_display_update_serial (CdkWaylandDisplay *display_wayland,
                                    guint32            serial)
{
  display_wayland->serial = serial;
}

/**
 * cdk_wayland_display_get_wl_display:
 * @display: (type CdkWaylandDisplay): a #CdkDisplay
 *
 * Returns the Wayland wl_display of a #CdkDisplay.
 *
 * Returns: (transfer none): a Wayland wl_display
 *
 * Since: 3.8
 */
struct wl_display *
cdk_wayland_display_get_wl_display (CdkDisplay *display)
{
  g_return_val_if_fail (CDK_IS_WAYLAND_DISPLAY (display), NULL);

  return CDK_WAYLAND_DISPLAY (display)->wl_display;
}

/**
 * cdk_wayland_display_get_wl_compositor:
 * @display: (type CdkWaylandDisplay): a #CdkDisplay
 *
 * Returns the Wayland global singleton compositor of a #CdkDisplay.
 *
 * Returns: (transfer none): a Wayland wl_compositor
 *
 * Since: 3.8
 */
struct wl_compositor *
cdk_wayland_display_get_wl_compositor (CdkDisplay *display)
{
  g_return_val_if_fail (CDK_IS_WAYLAND_DISPLAY (display), NULL);

  return CDK_WAYLAND_DISPLAY (display)->compositor;
}

static const cairo_user_data_key_t cdk_wayland_shm_surface_cairo_key;

typedef struct _CdkWaylandCairoSurfaceData {
  gpointer buf;
  size_t buf_length;
  struct wl_shm_pool *pool;
  struct wl_buffer *buffer;
  CdkWaylandDisplay *display;
  uint32_t scale;
} CdkWaylandCairoSurfaceData;

static int
open_shared_memory (void)
{
  static gboolean force_shm_open = FALSE;
  int ret = -1;

#if !defined (__NR_memfd_create)
  force_shm_open = TRUE;
#endif

  do
    {
#if defined (__NR_memfd_create)
      if (!force_shm_open)
        {
          ret = syscall (__NR_memfd_create, "cdk-wayland", MFD_CLOEXEC);

          /* fall back to shm_open until debian stops shipping 3.16 kernel
           * See bug 766341
           */
          if (ret < 0 && errno == ENOSYS)
            force_shm_open = TRUE;
        }
#endif

      if (force_shm_open)
        {
#if defined (__FreeBSD__)
          ret = shm_open (SHM_ANON, O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC, 0600);
#else
          char name[NAME_MAX - 1] = "";

          sprintf (name, "/cdk-wayland-%x", g_random_int ());

          ret = shm_open (name, O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC, 0600);

          if (ret >= 0)
            shm_unlink (name);
          else if (errno == EEXIST)
            continue;
#endif
        }
    }
  while (ret < 0 && errno == EINTR);

  if (ret < 0)
    g_critical (G_STRLOC ": creating shared memory file (using %s) failed: %m",
                force_shm_open? "shm_open" : "memfd_create");

  return ret;
}

static struct wl_shm_pool *
create_shm_pool (struct wl_shm  *shm,
                 int             size,
                 size_t         *buf_length,
                 void          **data_out)
{
  struct wl_shm_pool *pool;
  int fd;
  void *data;

  fd = open_shared_memory ();

  if (fd < 0)
    return NULL;

  if (ftruncate (fd, size) < 0)
    {
      g_critical (G_STRLOC ": Truncating shared memory file failed: %m");
      close (fd);
      return NULL;
    }

  data = mmap (NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  if (data == MAP_FAILED)
    {
      g_critical (G_STRLOC ": mmap'ping shared memory file failed: %m");
      close (fd);
      return NULL;
    }

  pool = wl_shm_create_pool (shm, fd, size);

  close (fd);

  *data_out = data;
  *buf_length = size;

  return pool;
}

static void
cdk_wayland_cairo_surface_destroy (void *p)
{
  CdkWaylandCairoSurfaceData *data = p;

  if (data->buffer)
    wl_buffer_destroy (data->buffer);

  if (data->pool)
    wl_shm_pool_destroy (data->pool);

  munmap (data->buf, data->buf_length);
  g_free (data);
}

cairo_surface_t *
_cdk_wayland_display_create_shm_surface (CdkWaylandDisplay *display,
                                         int                width,
                                         int                height,
                                         guint              scale)
{
  CdkWaylandCairoSurfaceData *data;
  cairo_surface_t *surface = NULL;
  cairo_status_t status;
  int stride;

  data = g_new (CdkWaylandCairoSurfaceData, 1);
  data->display = display;
  data->buffer = NULL;
  data->scale = scale;

  stride = cairo_format_stride_for_width (CAIRO_FORMAT_ARGB32, width*scale);

  data->pool = create_shm_pool (display->shm,
                                height*scale*stride,
                                &data->buf_length,
                                &data->buf);

  surface = cairo_image_surface_create_for_data (data->buf,
                                                 CAIRO_FORMAT_ARGB32,
                                                 width*scale,
                                                 height*scale,
                                                 stride);

  data->buffer = wl_shm_pool_create_buffer (data->pool, 0,
                                            width*scale, height*scale,
                                            stride, WL_SHM_FORMAT_ARGB8888);

  cairo_surface_set_user_data (surface, &cdk_wayland_shm_surface_cairo_key,
                               data, cdk_wayland_cairo_surface_destroy);

  cairo_surface_set_device_scale (surface, scale, scale);

  status = cairo_surface_status (surface);
  if (status != CAIRO_STATUS_SUCCESS)
    {
      g_critical (G_STRLOC ": Unable to create Cairo image surface: %s",
                  cairo_status_to_string (status));
    }

  return surface;
}

struct wl_buffer *
_cdk_wayland_shm_surface_get_wl_buffer (cairo_surface_t *surface)
{
  CdkWaylandCairoSurfaceData *data = cairo_surface_get_user_data (surface, &cdk_wayland_shm_surface_cairo_key);
  return data->buffer;
}

gboolean
_cdk_wayland_is_shm_surface (cairo_surface_t *surface)
{
  return cairo_surface_get_user_data (surface, &cdk_wayland_shm_surface_cairo_key) != NULL;
}

CdkWaylandSelection *
cdk_wayland_display_get_selection (CdkDisplay *display)
{
  CdkWaylandDisplay *display_wayland = CDK_WAYLAND_DISPLAY (display);

  return display_wayland->selection;
}

/**
 * cdk_wayland_display_query_registry:
 * @display: a wayland #CdkDisplay
 * @interface: global interface to query in the registry
 *
 * Returns %TRUE if the the interface was found in the display
 * wl_registry.global handler.
 *
 * Returns: %TRUE if the global is offered by the compositor
 *
 * Since: 3.22.27
 **/
gboolean
cdk_wayland_display_query_registry (CdkDisplay  *display,
				    const gchar *global)
{
  CdkWaylandDisplay *display_wayland = CDK_WAYLAND_DISPLAY (display);
  GHashTableIter iter;
  gchar *value;

  g_return_val_if_fail (CDK_IS_WAYLAND_DISPLAY (display), FALSE);
  g_return_val_if_fail (global != NULL, FALSE);

  g_hash_table_iter_init (&iter, display_wayland->known_globals);

  while (g_hash_table_iter_next (&iter, NULL, (gpointer*) &value))
    {
      if (strcmp (value, global) == 0)
        return TRUE;
    }

  return FALSE;
}
