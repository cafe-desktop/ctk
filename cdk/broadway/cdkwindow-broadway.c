/* GDK - The GIMP Drawing Kit
 * Copyright (C) 1995-2007 Peter Mattis, Spencer Kimball,
 * Josh MacDonald, Ryan Lortie
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

/*
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/. 
 */

#include "config.h"

#include "cdkwindow-broadway.h"
#include "cdkscreen-broadway.h"

#include "cdkbroadwaydisplay.h"
#include "cdkdisplay.h"
#include "cdkwindow.h"
#include "cdkwindowimpl.h"
#include "cdkdisplay-broadway.h"
#include "cdkprivate-broadway.h"
#include "cdkinternals.h"
#include "cdkdeviceprivate.h"
#include "cdkeventsource.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Forward declarations */
static void     cdk_window_broadway_set_background     (CdkWindow      *window,
                                                   cairo_pattern_t *pattern);

static void        cdk_window_impl_broadway_finalize   (GObject            *object);

static const cairo_user_data_key_t cdk_broadway_cairo_key;

#define WINDOW_IS_TOPLEVEL_OR_FOREIGN(window) \
  (GDK_WINDOW_TYPE (window) != GDK_WINDOW_CHILD &&   \
   GDK_WINDOW_TYPE (window) != GDK_WINDOW_OFFSCREEN)

#define WINDOW_IS_TOPLEVEL(window)		     \
  (GDK_WINDOW_TYPE (window) != GDK_WINDOW_CHILD &&   \
   GDK_WINDOW_TYPE (window) != GDK_WINDOW_FOREIGN && \
   GDK_WINDOW_TYPE (window) != GDK_WINDOW_OFFSCREEN)

struct _CdkBroadwayWindow {
  CdkWindow parent;
};

struct _CdkBroadwayWindowClass {
  CdkWindowClass parent_class;
};

G_DEFINE_TYPE (CdkBroadwayWindow, cdk_broadway_window, GDK_TYPE_WINDOW)

static void
cdk_broadway_window_class_init (CdkBroadwayWindowClass *broadway_window_class)
{
}

static void
cdk_broadway_window_init (CdkBroadwayWindow *broadway_window)
{
}

G_DEFINE_TYPE (CdkWindowImplBroadway,
	       cdk_window_impl_broadway,
	       GDK_TYPE_WINDOW_IMPL)

static CdkDisplay *
find_broadway_display (void)
{
  CdkDisplay *display;
  GSList *list, *l;

  display = NULL;

  list = cdk_display_manager_list_displays (cdk_display_manager_get ());
  for (l = list; l; l = l->next)
    {
      if (GDK_IS_BROADWAY_DISPLAY (l->data))
        {
          display = l->data;
          break; 
        }
    }
  g_slist_free (list);

  return display;
}

static void
update_dirty_windows_and_sync (void)
{
  GList *l;
  CdkBroadwayDisplay *display;
  gboolean updated_surface;

  display = GDK_BROADWAY_DISPLAY (find_broadway_display ());
  g_assert (display != NULL);

  updated_surface = FALSE;
  for (l = display->toplevels; l != NULL; l = l->next)
    {
      CdkWindowImplBroadway *impl = l->data;

      if (impl->dirty)
	{
	  impl->dirty = FALSE;
	  updated_surface = TRUE;
	  _cdk_broadway_server_window_update (display->server,
					      impl->id,
					      impl->surface);
	}
    }

  /* We sync here to ensure all references to the impl->surface memory
     is done, as we may later paint new data in them. */
  if (updated_surface)
    cdk_display_sync (GDK_DISPLAY (display));
  else
    cdk_display_flush (GDK_DISPLAY (display));
}

static guint flush_id = 0;

static gboolean
flush_idle (gpointer data)
{
  flush_id = 0;

  cdk_display_flush (find_broadway_display ());

  return FALSE;
}

/* We need to flush in an idle rather than AFTER_PAINT, as the clock
   is frozen during e.g. window resizes so the paint will not happen
   and the window resize request is never flushed. */
static void
queue_flush (CdkWindow *window)
{
  if (flush_id == 0)
    {
      flush_id = cdk_threads_add_idle (flush_idle, NULL);
      g_source_set_name_by_id (flush_id, "[ctk+] flush_idle");
    }
}

static void
cdk_window_impl_broadway_init (CdkWindowImplBroadway *impl)
{
  impl->toplevel_window_type = -1;
  impl->device_cursor = g_hash_table_new_full (NULL, NULL, NULL,
                                               (GDestroyNotify) g_object_unref);
}

static void
cdk_window_impl_broadway_finalize (GObject *object)
{
  CdkWindow *wrapper;
  CdkWindowImplBroadway *impl;
  CdkBroadwayDisplay *broadway_display;

  g_return_if_fail (GDK_IS_WINDOW_IMPL_BROADWAY (object));

  impl = GDK_WINDOW_IMPL_BROADWAY (object);

  wrapper = impl->wrapper;

  _cdk_broadway_window_grab_check_destroy (wrapper);

  broadway_display = GDK_BROADWAY_DISPLAY (cdk_window_get_display (impl->wrapper));

  g_hash_table_remove (broadway_display->id_ht, GINT_TO_POINTER(impl->id));

  if (impl->cursor)
    g_object_unref (impl->cursor);

  g_hash_table_destroy (impl->device_cursor);

  broadway_display->toplevels = g_list_remove (broadway_display->toplevels, impl);

  G_OBJECT_CLASS (cdk_window_impl_broadway_parent_class)->finalize (object);
}

void
_cdk_broadway_screen_init_root_window (CdkScreen * screen)
{
  CdkWindow *window;
  CdkWindowImplBroadway *impl;
  CdkBroadwayScreen *broadway_screen;

  broadway_screen = GDK_BROADWAY_SCREEN (screen);

  g_assert (broadway_screen->root_window == NULL);

  broadway_screen->root_window = g_object_new (GDK_TYPE_BROADWAY_WINDOW, NULL);

  window = broadway_screen->root_window;
  window->impl = g_object_new (GDK_TYPE_WINDOW_IMPL_BROADWAY, NULL);
  window->impl_window = window;
  window->visual = cdk_screen_get_system_visual (screen);

  impl = GDK_WINDOW_IMPL_BROADWAY (window->impl);

  impl->screen = screen;
  impl->wrapper = window;
  impl->id = 0;

  window->window_type = GDK_WINDOW_ROOT;
  window->depth = 24;

  window->x = 0;
  window->y = 0;
  window->abs_x = 0;
  window->abs_y = 0;
  window->width = cdk_screen_get_width (screen);
  window->height = cdk_screen_get_height (screen);
  window->viewable = TRUE;

  _cdk_window_update_size (broadway_screen->root_window);
}

static void
on_frame_clock_after_paint (CdkFrameClock *clock,
                            CdkWindow     *window)
{
  update_dirty_windows_and_sync ();
}

static void
connect_frame_clock (CdkWindow *window)
{
  if (WINDOW_IS_TOPLEVEL (window))
    {
      CdkFrameClock *frame_clock = cdk_window_get_frame_clock (window);

      g_signal_connect (frame_clock, "after-paint",
                        G_CALLBACK (on_frame_clock_after_paint), window);
    }
}

void
_cdk_broadway_display_create_window_impl (CdkDisplay    *display,
					  CdkWindow     *window,
					  CdkWindow     *real_parent,
					  CdkScreen     *screen,
					  CdkEventMask   event_mask,
					  CdkWindowAttr *attributes,
					  gint           attributes_mask)
{
  CdkWindowImplBroadway *impl;
  CdkBroadwayDisplay *broadway_display;

  broadway_display = GDK_BROADWAY_DISPLAY (display);

  impl = g_object_new (GDK_TYPE_WINDOW_IMPL_BROADWAY, NULL);
  window->impl = (CdkWindowImpl *)impl;
  impl->id = _cdk_broadway_server_new_window (broadway_display->server,
					      window->x,
					      window->y,
					      window->width,
					      window->height,
					      window->window_type == GDK_WINDOW_TEMP);
  g_hash_table_insert (broadway_display->id_ht, GINT_TO_POINTER(impl->id), window);
  impl->wrapper = window;

  impl->screen = screen;

  g_assert (window->window_type == GDK_WINDOW_TOPLEVEL ||
	    window->window_type == GDK_WINDOW_TEMP);
  g_assert (GDK_WINDOW_TYPE (window->parent) == GDK_WINDOW_ROOT);

  broadway_display->toplevels = g_list_prepend (broadway_display->toplevels, impl);

  connect_frame_clock (window);
}

void
_cdk_broadway_window_resize_surface (CdkWindow *window)
{
  CdkWindowImplBroadway *impl = GDK_WINDOW_IMPL_BROADWAY (window->impl);

  if (impl->surface)
    {
      cairo_surface_destroy (impl->surface);

      impl->surface = _cdk_broadway_server_create_surface (cdk_window_get_width (impl->wrapper),
							   cdk_window_get_height (impl->wrapper));
    }

  if (impl->ref_surface)
    {
      cairo_surface_set_user_data (impl->ref_surface, &cdk_broadway_cairo_key,
				   NULL, NULL);
      impl->ref_surface = NULL;
    }

  cdk_window_invalidate_rect (window, NULL, TRUE);
}

static void
ref_surface_destroyed (void *data)
{
  CdkWindowImplBroadway *impl = data;

  impl->ref_surface = NULL;
}

static cairo_surface_t *
cdk_window_broadway_ref_cairo_surface (CdkWindow *window)
{
  CdkWindowImplBroadway *impl = GDK_WINDOW_IMPL_BROADWAY (window->impl);
  int w, h;

  if (GDK_IS_WINDOW_IMPL_BROADWAY (window) &&
      GDK_WINDOW_DESTROYED (impl->wrapper))
    return NULL;

  w = cdk_window_get_width (impl->wrapper);
  h = cdk_window_get_height (impl->wrapper);

  /* Create actual backing store if missing */
  if (!impl->surface)
    impl->surface = _cdk_broadway_server_create_surface (w, h);

  /* Create a destroyable surface referencing the real one */
  if (!impl->ref_surface)
    {
      impl->ref_surface =
	cairo_surface_create_for_rectangle (impl->surface,
					    0, 0,
					    w, h);
      if (impl->ref_surface)
	cairo_surface_set_user_data (impl->ref_surface, &cdk_broadway_cairo_key,
				     impl, ref_surface_destroyed);
    }
  else
    cairo_surface_reference (impl->ref_surface);

  return impl->ref_surface;
}

static void
_cdk_broadway_window_destroy (CdkWindow *window,
			      gboolean   recursing,
			      gboolean   foreign_destroy)
{
  CdkWindowImplBroadway *impl;
  CdkBroadwayDisplay *broadway_display;

  g_return_if_fail (GDK_IS_WINDOW (window));

  impl = GDK_WINDOW_IMPL_BROADWAY (window->impl);

  _cdk_broadway_selection_window_destroyed (window);
  _cdk_broadway_window_grab_check_destroy (window);

  if (impl->ref_surface)
    {
      cairo_surface_finish (impl->ref_surface);
      cairo_surface_set_user_data (impl->ref_surface, &cdk_broadway_cairo_key,
				   NULL, NULL);
    }

  if (impl->surface)
    {
      cairo_surface_destroy (impl->surface);
      impl->surface = NULL;
    }

  broadway_display = GDK_BROADWAY_DISPLAY (cdk_window_get_display (window));
  g_hash_table_remove (broadway_display->id_ht, GINT_TO_POINTER(impl->id));

  _cdk_broadway_server_destroy_window (broadway_display->server,
				       impl->id);
}

static void
cdk_broadway_window_destroy_foreign (CdkWindow *window)
{
}

/* This function is called when the XWindow is really gone.
 */
static void
cdk_broadway_window_destroy_notify (CdkWindow *window)
{
  if (!GDK_WINDOW_DESTROYED (window))
    {
      if (GDK_WINDOW_TYPE(window) != GDK_WINDOW_FOREIGN)
	g_warning ("CdkWindow %p unexpectedly destroyed", window);

      _cdk_window_destroy (window, TRUE);
    }

  g_object_unref (window);
}

static void
cdk_window_broadway_show (CdkWindow *window, gboolean already_mapped)
{
  CdkWindowImplBroadway *impl;
  CdkBroadwayDisplay *broadway_display;

  impl = GDK_WINDOW_IMPL_BROADWAY (window->impl);
  impl->visible = TRUE;

  if (window->event_mask & GDK_STRUCTURE_MASK)
    _cdk_make_event (GDK_WINDOW (window), GDK_MAP, NULL, FALSE);

  if (window->parent && window->parent->event_mask & GDK_SUBSTRUCTURE_MASK)
    _cdk_make_event (GDK_WINDOW (window), GDK_MAP, NULL, FALSE);

  broadway_display = GDK_BROADWAY_DISPLAY (cdk_window_get_display (window));
  if (_cdk_broadway_server_window_show (broadway_display->server, impl->id))
    queue_flush (window);

}

static void
cdk_window_broadway_hide (CdkWindow *window)
{
  CdkWindowImplBroadway *impl;
  CdkBroadwayDisplay *broadway_display;

  impl = GDK_WINDOW_IMPL_BROADWAY (window->impl);
  impl->visible = FALSE;

  if (window->event_mask & GDK_STRUCTURE_MASK)
    _cdk_make_event (GDK_WINDOW (window), GDK_UNMAP, NULL, FALSE);

  if (window->parent && window->parent->event_mask & GDK_SUBSTRUCTURE_MASK)
    _cdk_make_event (GDK_WINDOW (window), GDK_UNMAP, NULL, FALSE);

  broadway_display = GDK_BROADWAY_DISPLAY (cdk_window_get_display (window));

  _cdk_broadway_window_grab_check_unmap (window,
					 _cdk_broadway_server_get_next_serial (broadway_display->server));

  if (_cdk_broadway_server_window_hide (broadway_display->server, impl->id))
    queue_flush (window);

  _cdk_window_clear_update_area (window);
}

static void
cdk_window_broadway_withdraw (CdkWindow *window)
{
  cdk_window_broadway_hide (window);
}

static void
cdk_window_broadway_move_resize (CdkWindow *window,
				 gboolean   with_move,
				 gint       x,
				 gint       y,
				 gint       width,
				 gint       height)
{
  CdkWindowImplBroadway *impl = GDK_WINDOW_IMPL_BROADWAY (window->impl);
  CdkBroadwayDisplay *broadway_display;
  gboolean size_changed;

  size_changed = FALSE;

  broadway_display = GDK_BROADWAY_DISPLAY (cdk_window_get_display (window));

  if (width > 0 || height > 0)
    {
      if (width < 1)
	width = 1;

      if (height < 1)
	height = 1;

      if (width != window->width ||
	  height != window->height)
	{
	  size_changed = TRUE;

	  /* Resize clears the content */
	  impl->dirty = TRUE;
	  impl->last_synced = FALSE;

	  window->width = width;
	  window->height = height;
	  _cdk_broadway_window_resize_surface (window);
	}
    }

  _cdk_broadway_server_window_move_resize (broadway_display->server,
					   impl->id,
					   with_move,
					   x, y,
					   window->width, window->height);
  queue_flush (window);
  if (size_changed)
    window->resize_count++;
}

static gboolean
cdk_window_broadway_reparent (CdkWindow *window,
			      CdkWindow *new_parent,
			      gint       x,
			      gint       y)
{
  return FALSE;
}

static void
cdk_window_broadway_raise (CdkWindow *window)
{
}

static void
cdk_window_broadway_restack_under (CdkWindow *window,
				   GList *native_siblings /* in requested order, first is bottom-most */)
{
}

static void
cdk_window_broadway_restack_toplevel (CdkWindow *window,
				      CdkWindow *sibling,
				      gboolean   above)
{
}

static void
cdk_window_broadway_lower (CdkWindow *window)
{
}


static void
cdk_broadway_window_focus (CdkWindow *window,
			   guint32    timestamp)
{
  CdkWindowImplBroadway *impl;
  CdkBroadwayDisplay *broadway_display;

  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window) ||
      !window->accept_focus)
    return;

  impl = GDK_WINDOW_IMPL_BROADWAY (window->impl);
  broadway_display = GDK_BROADWAY_DISPLAY (cdk_window_get_display (window));
  _cdk_broadway_server_window_focus (broadway_display->server,
				     impl->id);
}

static void
cdk_broadway_window_set_type_hint (CdkWindow        *window,
				   CdkWindowTypeHint hint)
{
}

static CdkWindowTypeHint
cdk_broadway_window_get_type_hint (CdkWindow *window)
{
  return GDK_WINDOW_TYPE_HINT_NORMAL;
}

static void
cdk_broadway_window_set_modal_hint (CdkWindow *window,
				    gboolean   modal)
{
}

static void
cdk_broadway_window_set_skip_taskbar_hint (CdkWindow *window,
					   gboolean   skips_taskbar)
{
}

static void
cdk_broadway_window_set_skip_pager_hint (CdkWindow *window,
					 gboolean   skips_pager)
{
}

static void
cdk_broadway_window_set_urgency_hint (CdkWindow *window,
				      gboolean   urgent)
{
}

static void
cdk_broadway_window_set_geometry_hints (CdkWindow         *window,
					const CdkGeometry *geometry,
					CdkWindowHints     geom_mask)
{
  CdkWindowImplBroadway *impl;

  impl = GDK_WINDOW_IMPL_BROADWAY (window->impl);

  impl->geometry_hints = *geometry;
  impl->geometry_hints_mask = geom_mask;
}

static void
cdk_broadway_window_set_title (CdkWindow   *window,
			       const gchar *title)
{
}

static void
cdk_broadway_window_set_role (CdkWindow   *window,
			      const gchar *role)
{
}

static void
cdk_broadway_window_set_startup_id (CdkWindow   *window,
				    const gchar *startup_id)
{
}

static void
cdk_broadway_window_set_transient_for (CdkWindow *window,
				       CdkWindow *parent)
{
  CdkBroadwayDisplay *display;
  CdkWindowImplBroadway *impl;
  int parent_id;

  impl = GDK_WINDOW_IMPL_BROADWAY (window->impl);

  parent_id = 0;
  if (parent)
    parent_id = GDK_WINDOW_IMPL_BROADWAY (parent->impl)->id;

  impl->transient_for = parent_id;

  display = GDK_BROADWAY_DISPLAY (cdk_window_get_display (impl->wrapper));
  _cdk_broadway_server_window_set_transient_for (display->server, impl->id, impl->transient_for);
}

static void
cdk_window_broadway_set_background (CdkWindow      *window,
				    cairo_pattern_t *pattern)
{
  return;
}

static void
cdk_window_broadway_set_device_cursor (CdkWindow *window,
				       CdkDevice *device,
				       CdkCursor *cursor)
{
  CdkWindowImplBroadway *impl;

  g_return_if_fail (GDK_IS_WINDOW (window));
  g_return_if_fail (GDK_IS_DEVICE (device));

  impl = GDK_WINDOW_IMPL_BROADWAY (window->impl);

  if (!cursor)
    g_hash_table_remove (impl->device_cursor, device);
  else
    {
      _cdk_broadway_cursor_update_theme (cursor);
      g_hash_table_replace (impl->device_cursor,
                            device, g_object_ref (cursor));
    }

  if (!GDK_WINDOW_DESTROYED (window))
    GDK_DEVICE_GET_CLASS (device)->set_window_cursor (device, window, cursor);
}

static void
cdk_window_broadway_get_geometry (CdkWindow *window,
				  gint      *x,
				  gint      *y,
				  gint      *width,
				  gint      *height)
{
  CdkWindowImplBroadway *impl;

  g_return_if_fail (GDK_IS_WINDOW (window));

  impl = GDK_WINDOW_IMPL_BROADWAY (window->impl);

  /* TODO: These should really roundtrip to the client to get the current data */

  if (x)
    *x = impl->wrapper->x;
  if (y)
    *y = impl->wrapper->y;
  if (width)
    *width = impl->wrapper->width;
  if (height)
    *height = impl->wrapper->height;

}

static void
cdk_window_broadway_get_root_coords (CdkWindow *window,
				     gint       x,
				     gint       y,
				     gint      *root_x,
				     gint      *root_y)
{
  CdkWindowImplBroadway *impl;

  impl = GDK_WINDOW_IMPL_BROADWAY (window->impl);

  if (root_x)
    *root_x = x + impl->wrapper->x;
  if (root_y)
    *root_y = y + impl->wrapper->y;
}

static void
cdk_broadway_window_get_frame_extents (CdkWindow    *window,
				       CdkRectangle *rect)
{
  g_return_if_fail (rect != NULL);

  /* TODO: This should take wm frame into account */

  rect->x = window->x;
  rect->y = window->y;
  rect->width = window->width;
  rect->height = window->height;
}

static gboolean
cdk_window_broadway_get_device_state (CdkWindow       *window,
				      CdkDevice       *device,
				      gdouble         *x,
				      gdouble         *y,
				      CdkModifierType *mask)
{
  CdkWindow *child;

  g_return_val_if_fail (window == NULL || GDK_IS_WINDOW (window), FALSE);

  if (GDK_WINDOW_DESTROYED (window))
    return FALSE;

  GDK_DEVICE_GET_CLASS (device)->query_state (device, window,
                                              NULL, &child,
                                              NULL, NULL,
                                              x, y, mask);
  return child != NULL;
}

static CdkEventMask
cdk_window_broadway_get_events (CdkWindow *window)
{
  if (GDK_WINDOW_DESTROYED (window))
    return 0;

  return 0;
}

static void
cdk_window_broadway_set_events (CdkWindow    *window,
				CdkEventMask  event_mask)
{
  if (!GDK_WINDOW_DESTROYED (window))
    {
    }
}

static void
cdk_window_broadway_shape_combine_region (CdkWindow       *window,
					  const cairo_region_t *shape_region,
					  gint             offset_x,
					  gint             offset_y)
{
}

static void
cdk_window_broadway_input_shape_combine_region (CdkWindow       *window,
						const cairo_region_t *shape_region,
						gint             offset_x,
						gint             offset_y)
{
}


static void
cdk_broadway_window_set_override_redirect (CdkWindow *window,
					   gboolean override_redirect)
{
}

static void
cdk_broadway_window_set_accept_focus (CdkWindow *window,
				      gboolean accept_focus)
{
  accept_focus = accept_focus != FALSE;

  if (window->accept_focus != accept_focus)
    {
      window->accept_focus = accept_focus;
    }
}

static void
cdk_broadway_window_set_focus_on_map (CdkWindow *window,
				      gboolean focus_on_map)
{
  focus_on_map = focus_on_map != FALSE;

  if (window->focus_on_map != focus_on_map)
    {
      window->focus_on_map = focus_on_map;
    }
}


static void
cdk_broadway_window_set_icon_list (CdkWindow *window,
				   GList     *pixbufs)
{
}

static void
cdk_broadway_window_set_icon_name (CdkWindow   *window,
				   const gchar *name)
{
  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL_OR_FOREIGN (window))
    return;

  g_object_set_qdata (G_OBJECT (window), g_quark_from_static_string ("cdk-icon-name-set"),
		      GUINT_TO_POINTER (name != NULL));
}

static void
cdk_broadway_window_iconify (CdkWindow *window)
{
  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL_OR_FOREIGN (window))
    return;
}

static void
cdk_broadway_window_deiconify (CdkWindow *window)
{
  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL_OR_FOREIGN (window))
    return;
}

static void
cdk_broadway_window_stick (CdkWindow *window)
{
  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL_OR_FOREIGN (window))
    return;

}

static void
cdk_broadway_window_unstick (CdkWindow *window)
{
  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL_OR_FOREIGN (window))
    return;

}

static void
cdk_broadway_window_maximize (CdkWindow *window)
{
  CdkWindowImplBroadway *impl;
  CdkScreen *screen;

  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL_OR_FOREIGN (window))
    return;

  impl = GDK_WINDOW_IMPL_BROADWAY (window->impl);

  if (impl->maximized)
    return;

  impl->maximized = TRUE;

  cdk_synthesize_window_state (window, 0, GDK_WINDOW_STATE_MAXIMIZED);

  impl->pre_maximize_x = window->x;
  impl->pre_maximize_y = window->y;
  impl->pre_maximize_width = window->width;
  impl->pre_maximize_height = window->height;

  screen = cdk_window_get_screen (window);

  cdk_window_move_resize (window, 0, 0,
			  cdk_screen_get_width (screen),
			  cdk_screen_get_height (screen));
}

static void
cdk_broadway_window_unmaximize (CdkWindow *window)
{
  CdkWindowImplBroadway *impl;

  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL_OR_FOREIGN (window))
    return;

  impl = GDK_WINDOW_IMPL_BROADWAY (window->impl);

  if (!impl->maximized)
    return;

  impl->maximized = FALSE;

  cdk_synthesize_window_state (window, GDK_WINDOW_STATE_MAXIMIZED, 0);

  cdk_window_move_resize (window,
			  impl->pre_maximize_x,
			  impl->pre_maximize_y,
			  impl->pre_maximize_width,
			  impl->pre_maximize_height);
}

static void
cdk_broadway_window_fullscreen (CdkWindow *window)
{
  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL_OR_FOREIGN (window))
    return;

}

static void
cdk_broadway_window_unfullscreen (CdkWindow *window)
{
  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL_OR_FOREIGN (window))
    return;

}

static void
cdk_broadway_window_set_keep_above (CdkWindow *window,
				    gboolean   setting)
{
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL_OR_FOREIGN (window))
    return;

}

static void
cdk_broadway_window_set_keep_below (CdkWindow *window, gboolean setting)
{
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL_OR_FOREIGN (window))
    return;

}

static CdkWindow *
cdk_broadway_window_get_group (CdkWindow *window)
{
  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return NULL;

  return window;
}

static void
cdk_broadway_window_set_group (CdkWindow *window,
			       CdkWindow *leader)
{
}

static void
cdk_broadway_window_set_decorations (CdkWindow      *window,
				     CdkWMDecoration decorations)
{
  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL_OR_FOREIGN (window))
    return;

}

static gboolean
cdk_broadway_window_get_decorations (CdkWindow       *window,
				     CdkWMDecoration *decorations)
{
  gboolean result = FALSE;

  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL_OR_FOREIGN (window))
    return FALSE;

  return result;
}

static void
cdk_broadway_window_set_functions (CdkWindow    *window,
				   CdkWMFunction functions)
{
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL_OR_FOREIGN (window))
    return;
}

static cairo_region_t *
cdk_broadway_window_get_shape (CdkWindow *window)
{
  return NULL;
}

static cairo_region_t *
cdk_broadway_window_get_input_shape (CdkWindow *window)
{
  return NULL;
}

static void
cdk_broadway_window_end_paint (CdkWindow *window)
{
  CdkWindowImplBroadway *impl;
  impl = GDK_WINDOW_IMPL_BROADWAY (window->impl);
  impl->dirty = TRUE;
}

typedef struct _MoveResizeData MoveResizeData;

struct _MoveResizeData
{
  CdkDisplay *display;

  CdkWindow *moveresize_window;
  CdkWindow *moveresize_emulation_window;
  gboolean is_resize;
  CdkWindowEdge resize_edge;
  gint moveresize_button;
  gint moveresize_x;
  gint moveresize_y;
  gint moveresize_orig_x;
  gint moveresize_orig_y;
  gint moveresize_orig_width;
  gint moveresize_orig_height;
  long moveresize_process_time;
  CdkWindowHints moveresize_geom_mask;
  CdkGeometry moveresize_geometry;
  BroadwayInputMsg *moveresize_pending_event;
};

static MoveResizeData *
get_move_resize_data (CdkDisplay *display,
		      gboolean    create)
{
  CdkBroadwayDisplay *broadway_display;
  MoveResizeData *mv_resize;

  broadway_display = GDK_BROADWAY_DISPLAY (display);

  mv_resize = broadway_display->move_resize_data;

  if (!mv_resize && create)
    {
      mv_resize = g_new0 (MoveResizeData, 1);
      mv_resize->display = display;

      broadway_display->move_resize_data = mv_resize;
    }

  return mv_resize;
}

static void
update_pos (MoveResizeData *mv_resize,
	    gint            new_root_x,
	    gint            new_root_y)
{
  gint dx, dy;

  dx = new_root_x - mv_resize->moveresize_x;
  dy = new_root_y - mv_resize->moveresize_y;

  if (mv_resize->is_resize)
    {
      gint x, y, w, h;

      x = mv_resize->moveresize_orig_x;
      y = mv_resize->moveresize_orig_y;

      w = mv_resize->moveresize_orig_width;
      h = mv_resize->moveresize_orig_height;

      switch (mv_resize->resize_edge)
	{
	case GDK_WINDOW_EDGE_NORTH_WEST:
	  x += dx;
	  y += dy;
	  w -= dx;
	  h -= dy;
	  break;
	case GDK_WINDOW_EDGE_NORTH:
	  y += dy;
	  h -= dy;
	  break;
	case GDK_WINDOW_EDGE_NORTH_EAST:
	  y += dy;
	  h -= dy;
	  w += dx;
	  break;
	case GDK_WINDOW_EDGE_SOUTH_WEST:
	  h += dy;
	  x += dx;
	  w -= dx;
	  break;
	case GDK_WINDOW_EDGE_SOUTH_EAST:
	  w += dx;
	  h += dy;
	  break;
	case GDK_WINDOW_EDGE_SOUTH:
	  h += dy;
	  break;
	case GDK_WINDOW_EDGE_EAST:
	  w += dx;
	  break;
	case GDK_WINDOW_EDGE_WEST:
	  x += dx;
	  w -= dx;
	  break;
	}

      x = MAX (x, 0);
      y = MAX (y, 0);
      w = MAX (w, 1);
      h = MAX (h, 1);

      if (mv_resize->moveresize_geom_mask)
	{
	  cdk_window_constrain_size (&mv_resize->moveresize_geometry,
				     mv_resize->moveresize_geom_mask,
				     w, h, &w, &h);
	}

      cdk_window_move_resize (mv_resize->moveresize_window, x, y, w, h);
    }
  else
    {
      gint x, y;

      x = mv_resize->moveresize_orig_x + dx;
      y = mv_resize->moveresize_orig_y + dy;

      cdk_window_move (mv_resize->moveresize_window, x, y);
    }
}

static void
finish_drag (MoveResizeData *mv_resize)
{
  cdk_window_destroy (mv_resize->moveresize_emulation_window);
  mv_resize->moveresize_emulation_window = NULL;
  g_object_unref (mv_resize->moveresize_window);
  mv_resize->moveresize_window = NULL;
  g_clear_pointer (&mv_resize->moveresize_pending_event, g_free);
}

static gboolean
moveresize_lookahead (CdkDisplay *display,
		      MoveResizeData *mv_resize,
		      BroadwayInputMsg *event)
{
  CdkBroadwayDisplay *broadway_display;

  broadway_display = GDK_BROADWAY_DISPLAY (display);

  return !_cdk_broadway_server_lookahead_event (broadway_display->server, "mb");
}

gboolean
_cdk_broadway_moveresize_handle_event (CdkDisplay *display,
				       BroadwayInputMsg *event)
{
  guint button_mask = 0;
  MoveResizeData *mv_resize = get_move_resize_data (display, FALSE);

  if (!mv_resize || !mv_resize->moveresize_window)
    return FALSE;

  button_mask = GDK_BUTTON1_MASK << (mv_resize->moveresize_button - 1);

  switch (event->base.type)
    {
    case BROADWAY_EVENT_TOUCH:
      if (event->touch.touch_type == 2) /* END */
        {
          update_pos (mv_resize,
                      event->touch.root_x,
                      event->touch.root_y);

          finish_drag (mv_resize);
        }
      else if (event->touch.touch_type == 1) /* UPDATE */
        {
          if (mv_resize->moveresize_window->resize_count > 0)
            {
              if (mv_resize->moveresize_pending_event)
                *mv_resize->moveresize_pending_event = *event;
              else
                mv_resize->moveresize_pending_event =
                  g_memdup (event, sizeof (BroadwayInputMsg));

              break;
            }
          update_pos (mv_resize,
                      event->touch.root_x,
                      event->touch.root_y);
        }

      break;

    case BROADWAY_EVENT_POINTER_MOVE:
      if (mv_resize->moveresize_window->resize_count > 0)
	{
	  if (mv_resize->moveresize_pending_event)
	    *mv_resize->moveresize_pending_event = *event;
	  else
	    mv_resize->moveresize_pending_event =
	      g_memdup (event, sizeof (BroadwayInputMsg));

	  break;
	}
      if (!moveresize_lookahead (display, mv_resize, event))
	break;

      update_pos (mv_resize,
		  event->pointer.root_x,
		  event->pointer.root_y);

      /* This should never be triggered in normal cases, but in the
       * case where the drag started without an implicit grab being
       * in effect, we could miss the release if it occurs before
       * we grab the pointer; this ensures that we will never
       * get a permanently stuck grab.
       */
      if ((event->pointer.state & button_mask) == 0)
	finish_drag (mv_resize);
      break;

    case BROADWAY_EVENT_BUTTON_RELEASE:
      update_pos (mv_resize,
		  event->pointer.root_x,
		  event->pointer.root_y);

      if (event->button.button == mv_resize->moveresize_button)
	finish_drag (mv_resize);
      break;
    }
  return TRUE;
}

gboolean
_cdk_broadway_moveresize_configure_done (CdkDisplay *display,
					 CdkWindow  *window)
{
  BroadwayInputMsg *tmp_event;
  MoveResizeData *mv_resize = get_move_resize_data (display, FALSE);

  if (!mv_resize || window != mv_resize->moveresize_window)
    return FALSE;

  if (mv_resize->moveresize_pending_event)
    {
      tmp_event = mv_resize->moveresize_pending_event;
      mv_resize->moveresize_pending_event = NULL;
      _cdk_broadway_moveresize_handle_event (display, tmp_event);
      g_free (tmp_event);
    }

  return TRUE;
}

static void
create_moveresize_window (MoveResizeData *mv_resize,
			  guint32         timestamp)
{
  CdkWindowAttr attributes;
  gint attributes_mask;
  CdkGrabStatus status;

  g_assert (mv_resize->moveresize_emulation_window == NULL);

  attributes.x = -100;
  attributes.y = -100;
  attributes.width = 10;
  attributes.height = 10;
  attributes.window_type = GDK_WINDOW_TEMP;
  attributes.wclass = GDK_INPUT_ONLY;
  attributes.override_redirect = TRUE;
  attributes.event_mask = 0;

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_NOREDIR;

  mv_resize->moveresize_emulation_window =
    cdk_window_new (cdk_screen_get_root_window (cdk_display_get_default_screen (mv_resize->display)),
		    &attributes,
		    attributes_mask);

  cdk_window_show (mv_resize->moveresize_emulation_window);

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  status = cdk_pointer_grab (mv_resize->moveresize_emulation_window,
			     FALSE,
			     GDK_BUTTON_RELEASE_MASK |
			     GDK_POINTER_MOTION_MASK,
			     NULL,
			     NULL,
			     timestamp);
  G_GNUC_END_IGNORE_DEPRECATIONS;

  if (status != GDK_GRAB_SUCCESS)
    {
      /* If this fails, some other client has grabbed the window
       * already.
       */
      finish_drag (mv_resize);
    }

  mv_resize->moveresize_process_time = 0;
}

static void
calculate_unmoving_origin (MoveResizeData *mv_resize)
{
  CdkRectangle rect;
  gint width, height;

  if (mv_resize->moveresize_geom_mask & GDK_HINT_WIN_GRAVITY &&
      mv_resize->moveresize_geometry.win_gravity == GDK_GRAVITY_STATIC)
    {
      cdk_window_get_origin (mv_resize->moveresize_window,
			     &mv_resize->moveresize_orig_x,
			     &mv_resize->moveresize_orig_y);
    }
  else
    {
      cdk_window_get_frame_extents (mv_resize->moveresize_window, &rect);
      cdk_window_get_geometry (mv_resize->moveresize_window,
			       NULL, NULL, &width, &height);

      switch (mv_resize->moveresize_geometry.win_gravity)
	{
	case GDK_GRAVITY_NORTH_WEST:
	  mv_resize->moveresize_orig_x = rect.x;
	  mv_resize->moveresize_orig_y = rect.y;
	  break;
	case GDK_GRAVITY_NORTH:
	  mv_resize->moveresize_orig_x = rect.x + rect.width / 2 - width / 2;
	  mv_resize->moveresize_orig_y = rect.y;
	  break;
	case GDK_GRAVITY_NORTH_EAST:
	  mv_resize->moveresize_orig_x = rect.x + rect.width - width;
	  mv_resize->moveresize_orig_y = rect.y;
	  break;
	case GDK_GRAVITY_WEST:
	  mv_resize->moveresize_orig_x = rect.x;
	  mv_resize->moveresize_orig_y = rect.y + rect.height / 2 - height / 2;
	  break;
	case GDK_GRAVITY_CENTER:
	  mv_resize->moveresize_orig_x = rect.x + rect.width / 2 - width / 2;
	  mv_resize->moveresize_orig_y = rect.y + rect.height / 2 - height / 2;
	  break;
	case GDK_GRAVITY_EAST:
	  mv_resize->moveresize_orig_x = rect.x + rect.width - width;
	  mv_resize->moveresize_orig_y = rect.y + rect.height / 2 - height / 2;
	  break;
	case GDK_GRAVITY_SOUTH_WEST:
	  mv_resize->moveresize_orig_x = rect.x;
	  mv_resize->moveresize_orig_y = rect.y + rect.height - height;
	  break;
	case GDK_GRAVITY_SOUTH:
	  mv_resize->moveresize_orig_x = rect.x + rect.width / 2 - width / 2;
	  mv_resize->moveresize_orig_y = rect.y + rect.height - height;
	  break;
	case GDK_GRAVITY_SOUTH_EAST:
	  mv_resize->moveresize_orig_x = rect.x + rect.width - width;
	  mv_resize->moveresize_orig_y = rect.y + rect.height - height;
	  break;
	default:
	  mv_resize->moveresize_orig_x = rect.x;
	  mv_resize->moveresize_orig_y = rect.y;
	  break;
	}
    }
}

static void
cdk_broadway_window_begin_resize_drag (CdkWindow     *window,
				       CdkWindowEdge  edge,
                                       CdkDevice     *device,
				       gint           button,
				       gint           root_x,
				       gint           root_y,
				       guint32        timestamp)
{
  MoveResizeData *mv_resize;
  CdkWindowImplBroadway *impl;

  impl = GDK_WINDOW_IMPL_BROADWAY (window->impl);

  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL_OR_FOREIGN (window))
    return;

  if (impl->maximized)
    return;

  mv_resize = get_move_resize_data (cdk_window_get_display (window), TRUE);

  mv_resize->is_resize = TRUE;
  mv_resize->moveresize_button = button;
  mv_resize->resize_edge = edge;
  mv_resize->moveresize_x = root_x;
  mv_resize->moveresize_y = root_y;
  mv_resize->moveresize_window = g_object_ref (window);

  mv_resize->moveresize_orig_width = cdk_window_get_width (window);
  mv_resize->moveresize_orig_height = cdk_window_get_height (window);

  mv_resize->moveresize_geom_mask = impl->geometry_hints_mask;
  mv_resize->moveresize_geometry = impl->geometry_hints;

  calculate_unmoving_origin (mv_resize);

  create_moveresize_window (mv_resize, timestamp);
}

static void
cdk_broadway_window_begin_move_drag (CdkWindow *window,
                                     CdkDevice *device,
				     gint       button,
				     gint       root_x,
				     gint       root_y,
				     guint32    timestamp)
{
  MoveResizeData *mv_resize;
  CdkWindowImplBroadway *impl;

  impl = GDK_WINDOW_IMPL_BROADWAY (window->impl);

  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL_OR_FOREIGN (window))
    return;

  if (impl->maximized)
    return;

  mv_resize = get_move_resize_data (cdk_window_get_display (window), TRUE);

  mv_resize->is_resize = FALSE;
  mv_resize->moveresize_button = button;
  mv_resize->moveresize_x = root_x;
  mv_resize->moveresize_y = root_y;
  mv_resize->moveresize_window = g_object_ref (window);

  mv_resize->moveresize_orig_width = cdk_window_get_width (window);
  mv_resize->moveresize_orig_height = cdk_window_get_height (window);

  mv_resize->moveresize_geom_mask = impl->geometry_hints_mask;
  mv_resize->moveresize_geometry = impl->geometry_hints;

  calculate_unmoving_origin (mv_resize);

  create_moveresize_window (mv_resize, timestamp);
}

static gboolean
cdk_broadway_window_beep (CdkWindow *window)
{
  return FALSE;
}

static void
cdk_broadway_window_set_opacity (CdkWindow *window,
				 gdouble    opacity)
{
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  if (opacity < 0)
    opacity = 0;
  else if (opacity > 1)
    opacity = 1;
}

static void
cdk_broadway_window_set_composited (CdkWindow *window,
				    gboolean   composited)
{
}

void
_cdk_broadway_display_before_process_all_updates (CdkDisplay *display)
{
}

void
_cdk_broadway_display_after_process_all_updates (CdkDisplay *display)
{
}

guint32
cdk_broadway_get_last_seen_time (CdkWindow  *window)
{
  CdkDisplay *display;

  display = cdk_window_get_display (window);
  return _cdk_broadway_server_get_last_seen_time (GDK_BROADWAY_DISPLAY (display)->server);
}

static void
cdk_window_impl_broadway_class_init (CdkWindowImplBroadwayClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CdkWindowImplClass *impl_class = GDK_WINDOW_IMPL_CLASS (klass);

  object_class->finalize = cdk_window_impl_broadway_finalize;

  impl_class->ref_cairo_surface = cdk_window_broadway_ref_cairo_surface;
  impl_class->show = cdk_window_broadway_show;
  impl_class->hide = cdk_window_broadway_hide;
  impl_class->withdraw = cdk_window_broadway_withdraw;
  impl_class->set_events = cdk_window_broadway_set_events;
  impl_class->get_events = cdk_window_broadway_get_events;
  impl_class->raise = cdk_window_broadway_raise;
  impl_class->lower = cdk_window_broadway_lower;
  impl_class->restack_under = cdk_window_broadway_restack_under;
  impl_class->restack_toplevel = cdk_window_broadway_restack_toplevel;
  impl_class->move_resize = cdk_window_broadway_move_resize;
  impl_class->set_background = cdk_window_broadway_set_background;
  impl_class->reparent = cdk_window_broadway_reparent;
  impl_class->set_device_cursor = cdk_window_broadway_set_device_cursor;
  impl_class->get_geometry = cdk_window_broadway_get_geometry;
  impl_class->get_root_coords = cdk_window_broadway_get_root_coords;
  impl_class->get_device_state = cdk_window_broadway_get_device_state;
  impl_class->shape_combine_region = cdk_window_broadway_shape_combine_region;
  impl_class->input_shape_combine_region = cdk_window_broadway_input_shape_combine_region;
  impl_class->destroy = _cdk_broadway_window_destroy;
  impl_class->destroy_foreign = cdk_broadway_window_destroy_foreign;
  impl_class->get_shape = cdk_broadway_window_get_shape;
  impl_class->get_input_shape = cdk_broadway_window_get_input_shape;
  impl_class->end_paint = cdk_broadway_window_end_paint;
  impl_class->beep = cdk_broadway_window_beep;

  impl_class->focus = cdk_broadway_window_focus;
  impl_class->set_type_hint = cdk_broadway_window_set_type_hint;
  impl_class->get_type_hint = cdk_broadway_window_get_type_hint;
  impl_class->set_modal_hint = cdk_broadway_window_set_modal_hint;
  impl_class->set_skip_taskbar_hint = cdk_broadway_window_set_skip_taskbar_hint;
  impl_class->set_skip_pager_hint = cdk_broadway_window_set_skip_pager_hint;
  impl_class->set_urgency_hint = cdk_broadway_window_set_urgency_hint;
  impl_class->set_geometry_hints = cdk_broadway_window_set_geometry_hints;
  impl_class->set_title = cdk_broadway_window_set_title;
  impl_class->set_role = cdk_broadway_window_set_role;
  impl_class->set_startup_id = cdk_broadway_window_set_startup_id;
  impl_class->set_transient_for = cdk_broadway_window_set_transient_for;
  impl_class->get_frame_extents = cdk_broadway_window_get_frame_extents;
  impl_class->set_override_redirect = cdk_broadway_window_set_override_redirect;
  impl_class->set_accept_focus = cdk_broadway_window_set_accept_focus;
  impl_class->set_focus_on_map = cdk_broadway_window_set_focus_on_map;
  impl_class->set_icon_list = cdk_broadway_window_set_icon_list;
  impl_class->set_icon_name = cdk_broadway_window_set_icon_name;
  impl_class->iconify = cdk_broadway_window_iconify;
  impl_class->deiconify = cdk_broadway_window_deiconify;
  impl_class->stick = cdk_broadway_window_stick;
  impl_class->unstick = cdk_broadway_window_unstick;
  impl_class->maximize = cdk_broadway_window_maximize;
  impl_class->unmaximize = cdk_broadway_window_unmaximize;
  impl_class->fullscreen = cdk_broadway_window_fullscreen;
  impl_class->unfullscreen = cdk_broadway_window_unfullscreen;
  impl_class->set_keep_above = cdk_broadway_window_set_keep_above;
  impl_class->set_keep_below = cdk_broadway_window_set_keep_below;
  impl_class->get_group = cdk_broadway_window_get_group;
  impl_class->set_group = cdk_broadway_window_set_group;
  impl_class->set_decorations = cdk_broadway_window_set_decorations;
  impl_class->get_decorations = cdk_broadway_window_get_decorations;
  impl_class->set_functions = cdk_broadway_window_set_functions;
  impl_class->begin_resize_drag = cdk_broadway_window_begin_resize_drag;
  impl_class->begin_move_drag = cdk_broadway_window_begin_move_drag;
  impl_class->set_opacity = cdk_broadway_window_set_opacity;
  impl_class->set_composited = cdk_broadway_window_set_composited;
  impl_class->destroy_notify = cdk_broadway_window_destroy_notify;
  impl_class->register_dnd = _cdk_broadway_window_register_dnd;
  impl_class->drag_begin = _cdk_broadway_window_drag_begin;
  impl_class->sync_rendering = _cdk_broadway_window_sync_rendering;
  impl_class->simulate_key = _cdk_broadway_window_simulate_key;
  impl_class->simulate_button = _cdk_broadway_window_simulate_button;
  impl_class->get_property = _cdk_broadway_window_get_property;
  impl_class->change_property = _cdk_broadway_window_change_property;
  impl_class->delete_property = _cdk_broadway_window_delete_property;
  impl_class->get_drag_protocol = _cdk_broadway_window_get_drag_protocol;
}
