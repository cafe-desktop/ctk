/* CDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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
 * Modified by the CTK+ Team and others 1997-2005.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#include "config.h"

#include "cdkwindow.h"
#include "cdkinternals.h"
#include "cdkwindowimpl.h"

#include <math.h>

#include "fallback-c89.c"

/* LIMITATIONS:
 *
 * Offscreen windows can’t be the child of a foreign window,
 *   nor contain foreign windows
 * CDK_POINTER_MOTION_HINT_MASK isn't effective
 */

typedef struct _CdkOffscreenWindow      CdkOffscreenWindow;
typedef struct _CdkOffscreenWindowClass CdkOffscreenWindowClass;

struct _CdkOffscreenWindow
{
  CdkWindowImpl parent_instance;

  CdkWindow *wrapper;

  cairo_surface_t *surface;
  CdkWindow *embedder;
};

struct _CdkOffscreenWindowClass
{
  CdkWindowImplClass parent_class;
};

#define CDK_TYPE_OFFSCREEN_WINDOW            (cdk_offscreen_window_get_type())
#define CDK_OFFSCREEN_WINDOW(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), CDK_TYPE_OFFSCREEN_WINDOW, CdkOffscreenWindow))
#define CDK_IS_OFFSCREEN_WINDOW(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), CDK_TYPE_OFFSCREEN_WINDOW))
#define CDK_OFFSCREEN_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CDK_TYPE_OFFSCREEN_WINDOW, CdkOffscreenWindowClass))
#define CDK_IS_OFFSCREEN_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CDK_TYPE_OFFSCREEN_WINDOW))
#define CDK_OFFSCREEN_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CDK_TYPE_OFFSCREEN_WINDOW, CdkOffscreenWindowClass))

static void       cdk_offscreen_window_hide               (CdkWindow                  *window);

G_DEFINE_TYPE (CdkOffscreenWindow, cdk_offscreen_window, CDK_TYPE_WINDOW_IMPL)


static void
cdk_offscreen_window_finalize (GObject *object)
{
  CdkOffscreenWindow *offscreen = CDK_OFFSCREEN_WINDOW (object);

  if (offscreen->surface)
    cairo_surface_destroy (offscreen->surface);

  G_OBJECT_CLASS (cdk_offscreen_window_parent_class)->finalize (object);
}

static void
cdk_offscreen_window_init (CdkOffscreenWindow *window G_GNUC_UNUSED)
{
}

static void
cdk_offscreen_window_destroy (CdkWindow *window,
                              gboolean   recursing,
                              gboolean   foreign_destroy G_GNUC_UNUSED)
{
  cdk_offscreen_window_set_embedder (window, NULL);

  if (!recursing)
    cdk_offscreen_window_hide (window);
}

static cairo_surface_t *
get_surface (CdkOffscreenWindow *offscreen)
{
  if (! offscreen->surface)
    {
      CdkWindow *window = offscreen->wrapper;

      g_signal_emit_by_name (window, "create-surface",
                             window->width,
                             window->height,
                             &offscreen->surface);
    }

  return offscreen->surface;
}

static gboolean
is_parent_of (CdkWindow *parent,
	      CdkWindow *child)
{
  CdkWindow *w;

  w = child;
  while (w != NULL)
    {
      if (w == parent)
	return TRUE;

      w = cdk_window_get_parent (w);
    }

  return FALSE;
}

static cairo_surface_t *
cdk_offscreen_window_ref_cairo_surface (CdkWindow *window)
{
  CdkOffscreenWindow *offscreen = CDK_OFFSCREEN_WINDOW (window->impl);

  return cairo_surface_reference (get_surface (offscreen));
}

cairo_surface_t *
_cdk_offscreen_window_create_surface (CdkWindow *offscreen,
                                      gint       width,
                                      gint       height)
{
  CdkOffscreenWindow *impl;
  CdkWindow *derived;

  g_return_val_if_fail (CDK_IS_OFFSCREEN_WINDOW (offscreen->impl), NULL);

  impl = CDK_OFFSCREEN_WINDOW (offscreen->impl);
  derived = impl->embedder ? impl->embedder : offscreen->parent;

  return cdk_window_create_similar_surface (derived,
					    CAIRO_CONTENT_COLOR_ALPHA,
					    width, height);
}

void
_cdk_offscreen_window_new (CdkWindow     *window,
			   CdkWindowAttr *attributes,
			   gint           attributes_mask G_GNUC_UNUSED)
{
  CdkOffscreenWindow *offscreen;

  g_return_if_fail (attributes != NULL);

  if (attributes->wclass != CDK_INPUT_OUTPUT)
    return; /* Can't support input only offscreens */

  if (window->parent != NULL && CDK_WINDOW_DESTROYED (window->parent))
    return;

  window->impl = g_object_new (CDK_TYPE_OFFSCREEN_WINDOW, NULL);
  offscreen = CDK_OFFSCREEN_WINDOW (window->impl);
  offscreen->wrapper = window;
}

static gboolean
cdk_offscreen_window_reparent (CdkWindow *window,
			       CdkWindow *new_parent,
			       gint       x,
			       gint       y)
{
  CdkWindow *old_parent;
  gboolean was_mapped;

  if (new_parent)
    {
      /* No input-output children of input-only windows */
      if (new_parent->input_only && !window->input_only)
	return FALSE;

      /* Don't create loops in hierarchy */
      if (is_parent_of (window, new_parent))
	return FALSE;
    }

  was_mapped = CDK_WINDOW_IS_MAPPED (window);

  cdk_window_hide (window);

  if (window->parent)
    window->parent->children = g_list_remove_link (window->parent->children, &window->children_list_node);

  old_parent = window->parent;
  window->parent = new_parent;
  window->x = x;
  window->y = y;

  if (new_parent)
    window->parent->children = g_list_concat (&window->children_list_node, window->parent->children);

  _cdk_synthesize_crossing_events_for_geometry_change (window);
  if (old_parent)
    _cdk_synthesize_crossing_events_for_geometry_change (old_parent);

  return was_mapped;
}

static void
cdk_offscreen_window_set_device_cursor (CdkWindow *window G_GNUC_UNUSED,
					CdkDevice *device G_GNUC_UNUSED,
					CdkCursor *cursor G_GNUC_UNUSED)
{
}

static void
from_embedder (CdkWindow *window,
	       double embedder_x, double embedder_y,
	       double *offscreen_x, double *offscreen_y)
{
  g_signal_emit_by_name (window->impl_window,
			 "from-embedder",
			 embedder_x, embedder_y,
			 offscreen_x, offscreen_y,
			 NULL);
}

static void
to_embedder (CdkWindow *window,
	     double offscreen_x, double offscreen_y,
	     double *embedder_x, double *embedder_y)
{
  g_signal_emit_by_name (window->impl_window,
			 "to-embedder",
			 offscreen_x, offscreen_y,
			 embedder_x, embedder_y,
			 NULL);
}

static void
cdk_offscreen_window_get_root_coords (CdkWindow *window,
				      gint       x,
				      gint       y,
				      gint      *root_x,
				      gint      *root_y)
{
  CdkOffscreenWindow *offscreen;
  int tmpx, tmpy;

  tmpx = x;
  tmpy = y;

  offscreen = CDK_OFFSCREEN_WINDOW (window->impl);
  if (offscreen->embedder)
    {
      double dx, dy;
      to_embedder (window,
		   x, y,
		   &dx, &dy);
      tmpx = floor (dx + 0.5);
      tmpy = floor (dy + 0.5);
      cdk_window_get_root_coords (offscreen->embedder,
				  tmpx, tmpy,
				  &tmpx, &tmpy);

    }

  if (root_x)
    *root_x = tmpx;
  if (root_y)
    *root_y = tmpy;
}

static gboolean
cdk_offscreen_window_get_device_state (CdkWindow       *window,
                                       CdkDevice       *device,
                                       gdouble         *x,
                                       gdouble         *y,
                                       CdkModifierType *mask)
{
  CdkOffscreenWindow *offscreen;
  double tmpx, tmpy;
  double dtmpx, dtmpy;
  CdkModifierType tmpmask;

  tmpx = 0;
  tmpy = 0;
  tmpmask = 0;

  offscreen = CDK_OFFSCREEN_WINDOW (window->impl);
  if (offscreen->embedder != NULL)
    {
      cdk_window_get_device_position_double (offscreen->embedder, device, &tmpx, &tmpy, &tmpmask);
      from_embedder (window,
		     tmpx, tmpy,
		     &dtmpx, &dtmpy);
      tmpx = dtmpx;
      tmpy = dtmpy;
    }

  if (x)
    *x = round (tmpx);
  if (y)
    *y = round (tmpy);
  if (mask)
    *mask = tmpmask;
  return TRUE;
}

/**
 * cdk_offscreen_window_get_surface:
 * @window: a #CdkWindow
 *
 * Gets the offscreen surface that an offscreen window renders into.
 * If you need to keep this around over window resizes, you need to
 * add a reference to it.
 *
 * Returns: (nullable) (transfer none): The offscreen surface, or
 *   %NULL if not offscreen
 */
cairo_surface_t *
cdk_offscreen_window_get_surface (CdkWindow *window)
{
  CdkOffscreenWindow *offscreen;

  g_return_val_if_fail (CDK_IS_WINDOW (window), FALSE);

  if (!CDK_IS_OFFSCREEN_WINDOW (window->impl))
    return NULL;

  offscreen = CDK_OFFSCREEN_WINDOW (window->impl);

  return get_surface (offscreen);
}

static void
cdk_offscreen_window_raise (CdkWindow *window)
{
  /* cdk_window_raise already changed the stacking order */
  _cdk_synthesize_crossing_events_for_geometry_change (window);
}

static void
cdk_offscreen_window_lower (CdkWindow *window)
{
  /* cdk_window_lower already changed the stacking order */
  _cdk_synthesize_crossing_events_for_geometry_change (window);
}

static void
cdk_offscreen_window_move_resize_internal (CdkWindow *window,
                                           gint       x,
                                           gint       y,
                                           gint       width,
                                           gint       height,
                                           gboolean   send_expose_events G_GNUC_UNUSED)
{
  CdkOffscreenWindow *offscreen;

  offscreen = CDK_OFFSCREEN_WINDOW (window->impl);

  if (width < 1)
    width = 1;
  if (height < 1)
    height = 1;

  if (window->destroyed)
    return;

  window->x = x;
  window->y = y;

  if (window->width != width ||
      window->height != height)
    {
      window->width = width;
      window->height = height;

      if (offscreen->surface)
        {
          cairo_surface_t *old_surface;
          cairo_t *cr;

          old_surface = offscreen->surface;
          offscreen->surface = NULL;

          offscreen->surface = get_surface (offscreen);

          cr = cairo_create (offscreen->surface);
          cairo_set_source_surface (cr, old_surface, 0, 0);
          cairo_paint (cr);
          cairo_destroy (cr);

          cairo_surface_destroy (old_surface);
        }
    }

  if (CDK_WINDOW_IS_MAPPED (window))
    {
      /* TODO: Only invalidate new area, i.e. for larger windows */
      cdk_window_invalidate_rect (window, NULL, TRUE);
      _cdk_synthesize_crossing_events_for_geometry_change (window);
    }
}

static void
cdk_offscreen_window_move_resize (CdkWindow *window,
                                  gboolean   with_move,
                                  gint       x,
                                  gint       y,
                                  gint       width,
                                  gint       height)
{
  if (!with_move)
    {
      x = window->x;
      y = window->y;
    }

  if (width < 0)
    width = window->width;

  if (height < 0)
    height = window->height;

  cdk_offscreen_window_move_resize_internal (window,
                                             x, y, width, height,
                                             TRUE);
}

static void
cdk_offscreen_window_show (CdkWindow *window,
			   gboolean   already_mapped G_GNUC_UNUSED)
{
  CdkRectangle area = { 0, 0, window->width, window->height };

  cdk_window_invalidate_rect (window, &area, FALSE);
}


static void
cdk_offscreen_window_hide (CdkWindow *window G_GNUC_UNUSED)
{
  /* TODO: This needs updating to the new grab world */
#if 0
  CdkOffscreenWindow *offscreen;
  CdkDisplay *display;

  g_return_if_fail (window != NULL);

  offscreen = CDK_OFFSCREEN_WINDOW (window->impl);

  /* May need to break grabs on children */
  display = cdk_window_get_display (window);

  if (display->pointer_grab.window != NULL)
    {
      if (is_parent_of (window, display->pointer_grab.window))
	{
	  /* Call this ourselves, even though cdk_display_pointer_ungrab
	     does so too, since we want to pass implicit == TRUE so the
	     broken grab event is generated */
	  _cdk_display_unset_has_pointer_grab (display,
					       TRUE,
					       FALSE,
					       CDK_CURRENT_TIME);
	  cdk_display_pointer_ungrab (display, CDK_CURRENT_TIME);
	}
    }
#endif
}

static void
cdk_offscreen_window_withdraw (CdkWindow *window G_GNUC_UNUSED)
{
}

static CdkEventMask
cdk_offscreen_window_get_events (CdkWindow *window G_GNUC_UNUSED)
{
  return 0;
}

static void
cdk_offscreen_window_set_events (CdkWindow    *window G_GNUC_UNUSED,
				 CdkEventMask  event_mask G_GNUC_UNUSED)
{
}

static void
cdk_offscreen_window_set_background (CdkWindow       *window G_GNUC_UNUSED,
				     cairo_pattern_t *pattern G_GNUC_UNUSED)
{
}

static void
cdk_offscreen_window_shape_combine_region (CdkWindow            *window G_GNUC_UNUSED,
					   const cairo_region_t *shape_region G_GNUC_UNUSED,
					   gint                  offset_x G_GNUC_UNUSED,
					   gint                  offset_y G_GNUC_UNUSED)
{
}

static void
cdk_offscreen_window_input_shape_combine_region (CdkWindow            *window G_GNUC_UNUSED,
						 const cairo_region_t *shape_region G_GNUC_UNUSED,
						 gint                  offset_x G_GNUC_UNUSED,
						 gint                  offset_y G_GNUC_UNUSED)
{
}

static void
cdk_offscreen_window_get_geometry (CdkWindow *window,
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
cdk_offscreen_window_queue_antiexpose (CdkWindow      *window G_GNUC_UNUSED,
				       cairo_region_t *area G_GNUC_UNUSED)
{
}

/**
 * cdk_offscreen_window_set_embedder:
 * @window: a #CdkWindow
 * @embedder: the #CdkWindow that @window gets embedded in
 *
 * Sets @window to be embedded in @embedder.
 *
 * To fully embed an offscreen window, in addition to calling this
 * function, it is also necessary to handle the #CdkWindow::pick-embedded-child
 * signal on the @embedder and the #CdkWindow::to-embedder and
 * #CdkWindow::from-embedder signals on @window.
 *
 * Since: 2.18
 */
void
cdk_offscreen_window_set_embedder (CdkWindow     *window,
				   CdkWindow     *embedder)
{
  CdkOffscreenWindow *offscreen;

  g_return_if_fail (CDK_IS_WINDOW (window));

  if (!CDK_IS_OFFSCREEN_WINDOW (window->impl))
    return;

  offscreen = CDK_OFFSCREEN_WINDOW (window->impl);

  if (embedder)
    {
      g_object_ref (embedder);
      embedder->num_offscreen_children++;
    }

  if (offscreen->embedder)
    {
      g_object_unref (offscreen->embedder);
      offscreen->embedder->num_offscreen_children--;
    }

  offscreen->embedder = embedder;
}

/**
 * cdk_offscreen_window_get_embedder:
 * @window: a #CdkWindow
 *
 * Gets the window that @window is embedded in.
 *
 * Returns: (nullable) (transfer none): the embedding #CdkWindow, or
 *     %NULL if @window is not an mbedded offscreen window
 *
 * Since: 2.18
 */
CdkWindow *
cdk_offscreen_window_get_embedder (CdkWindow *window)
{
  CdkOffscreenWindow *offscreen;

  g_return_val_if_fail (CDK_IS_WINDOW (window), NULL);

  if (!CDK_IS_OFFSCREEN_WINDOW (window->impl))
    return NULL;

  offscreen = CDK_OFFSCREEN_WINDOW (window->impl);

  return offscreen->embedder;
}

static void
cdk_offscreen_window_do_nothing (CdkWindow *window G_GNUC_UNUSED)
{
}

static void
cdk_offscreen_window_set_boolean (CdkWindow *window G_GNUC_UNUSED,
				  gboolean   setting G_GNUC_UNUSED)
{
}

static void
cdk_offscreen_window_set_string (CdkWindow   *window G_GNUC_UNUSED,
				 const gchar *setting G_GNUC_UNUSED)
{
}

static void
cdk_offscreen_window_set_list (CdkWindow *window G_GNUC_UNUSED,
			       GList     *list G_GNUC_UNUSED)
{
}

static void
cdk_offscreen_window_set_wmfunctions (CdkWindow	    *window G_GNUC_UNUSED,
				      CdkWMFunction  functions G_GNUC_UNUSED)
{
}

static void
cdk_offscreen_window_begin_move_drag (CdkWindow *window G_GNUC_UNUSED,
                                      CdkDevice *device G_GNUC_UNUSED,
                                      gint       button G_GNUC_UNUSED,
                                      gint       root_x G_GNUC_UNUSED,
                                      gint       root_y G_GNUC_UNUSED,
                                      guint32    timestamp G_GNUC_UNUSED)
{
}

static void
cdk_offscreen_window_set_transient_for (CdkWindow *window G_GNUC_UNUSED,
					CdkWindow *another G_GNUC_UNUSED)
{
}

static void
cdk_offscreen_window_get_frame_extents (CdkWindow    *window,
					CdkRectangle *rect)
{
  rect->x = window->x;
  rect->y = window->y;
  rect->width = window->width;
  rect->height = window->height;
}

static gint
cdk_offscreen_window_get_scale_factor (CdkWindow *window)
{
  CdkOffscreenWindow *offscreen;

  if (CDK_WINDOW_DESTROYED (window))
    return 1;

  offscreen = CDK_OFFSCREEN_WINDOW (window->impl);
  if (offscreen->embedder)
    return cdk_window_get_scale_factor (offscreen->embedder);

  return cdk_window_get_scale_factor (window->parent);
}

static void
cdk_offscreen_window_set_opacity (CdkWindow *window G_GNUC_UNUSED,
				  gdouble    opacity G_GNUC_UNUSED)
{
}

static gboolean
cdk_offscreen_window_beep (CdkWindow *window G_GNUC_UNUSED)
{
  return FALSE;
}

static void
cdk_offscreen_window_class_init (CdkOffscreenWindowClass *klass)
{
  CdkWindowImplClass *impl_class = CDK_WINDOW_IMPL_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = cdk_offscreen_window_finalize;

  impl_class->ref_cairo_surface = cdk_offscreen_window_ref_cairo_surface;
  impl_class->show = cdk_offscreen_window_show;
  impl_class->hide = cdk_offscreen_window_hide;
  impl_class->withdraw = cdk_offscreen_window_withdraw;
  impl_class->set_events = cdk_offscreen_window_set_events;
  impl_class->get_events = cdk_offscreen_window_get_events;
  impl_class->raise = cdk_offscreen_window_raise;
  impl_class->lower = cdk_offscreen_window_lower;
  impl_class->restack_under = NULL;
  impl_class->restack_toplevel = NULL;
  impl_class->move_resize = cdk_offscreen_window_move_resize;
  impl_class->set_background = cdk_offscreen_window_set_background;
  impl_class->reparent = cdk_offscreen_window_reparent;
  impl_class->set_device_cursor = cdk_offscreen_window_set_device_cursor;
  impl_class->get_geometry = cdk_offscreen_window_get_geometry;
  impl_class->get_root_coords = cdk_offscreen_window_get_root_coords;
  impl_class->get_device_state = cdk_offscreen_window_get_device_state;
  impl_class->shape_combine_region = cdk_offscreen_window_shape_combine_region;
  impl_class->input_shape_combine_region = cdk_offscreen_window_input_shape_combine_region;
  impl_class->queue_antiexpose = cdk_offscreen_window_queue_antiexpose;
  impl_class->destroy = cdk_offscreen_window_destroy;
  impl_class->destroy_foreign = NULL;
  impl_class->get_shape = NULL;
  impl_class->get_input_shape = NULL;
  impl_class->beep = cdk_offscreen_window_beep;

  impl_class->focus = NULL;
  impl_class->set_type_hint = NULL;
  impl_class->get_type_hint = NULL;
  impl_class->set_modal_hint = cdk_offscreen_window_set_boolean;
  impl_class->set_skip_taskbar_hint = cdk_offscreen_window_set_boolean;
  impl_class->set_skip_pager_hint = cdk_offscreen_window_set_boolean;
  impl_class->set_urgency_hint = cdk_offscreen_window_set_boolean;
  impl_class->set_geometry_hints = NULL;
  impl_class->set_title = cdk_offscreen_window_set_string;
  impl_class->set_role = cdk_offscreen_window_set_string;
  impl_class->set_startup_id = cdk_offscreen_window_set_string;
  impl_class->set_transient_for = cdk_offscreen_window_set_transient_for;
  impl_class->get_frame_extents = cdk_offscreen_window_get_frame_extents;
  impl_class->set_override_redirect = NULL;
  impl_class->set_accept_focus = cdk_offscreen_window_set_boolean;
  impl_class->set_focus_on_map = cdk_offscreen_window_set_boolean;
  impl_class->set_icon_list = cdk_offscreen_window_set_list;
  impl_class->set_icon_name = cdk_offscreen_window_set_string;
  impl_class->iconify = cdk_offscreen_window_do_nothing;
  impl_class->deiconify = cdk_offscreen_window_do_nothing;
  impl_class->stick = cdk_offscreen_window_do_nothing;
  impl_class->unstick = cdk_offscreen_window_do_nothing;
  impl_class->maximize = cdk_offscreen_window_do_nothing;
  impl_class->unmaximize = cdk_offscreen_window_do_nothing;
  impl_class->fullscreen = cdk_offscreen_window_do_nothing;
  impl_class->unfullscreen = cdk_offscreen_window_do_nothing;
  impl_class->set_keep_above = cdk_offscreen_window_set_boolean;
  impl_class->set_keep_below = cdk_offscreen_window_set_boolean;
  impl_class->get_group = NULL;
  impl_class->set_group = NULL;
  impl_class->set_decorations = NULL;
  impl_class->get_decorations = NULL;
  impl_class->set_functions = cdk_offscreen_window_set_wmfunctions;
  impl_class->begin_resize_drag = NULL;
  impl_class->begin_move_drag = cdk_offscreen_window_begin_move_drag;
  impl_class->enable_synchronized_configure = cdk_offscreen_window_do_nothing;
  impl_class->configure_finished = NULL;
  impl_class->set_opacity = cdk_offscreen_window_set_opacity;
  impl_class->set_composited = NULL;
  impl_class->destroy_notify = NULL;
  impl_class->register_dnd = cdk_offscreen_window_do_nothing;
  impl_class->drag_begin = NULL;
  impl_class->sync_rendering = NULL;
  impl_class->simulate_key = NULL;
  impl_class->simulate_button = NULL;
  impl_class->get_property = NULL;
  impl_class->change_property = NULL;
  impl_class->delete_property = NULL;
  impl_class->get_scale_factor = cdk_offscreen_window_get_scale_factor;
}
