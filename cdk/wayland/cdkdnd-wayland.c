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

#include "cdkdndprivate.h"

#include "cdkmain.h"
#include "cdkinternals.h"
#include "cdkproperty.h"
#include "cdkprivate-wayland.h"
#include "cdkdisplay-wayland.h"
#include "cdkseat-wayland.h"

#include "cdkdeviceprivate.h"

#include <string.h>

#define CDK_TYPE_WAYLAND_DRAG_CONTEXT              (cdk_wayland_drag_context_get_type ())
#define CDK_WAYLAND_DRAG_CONTEXT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), CDK_TYPE_WAYLAND_DRAG_CONTEXT, CdkWaylandDragContext))
#define CDK_WAYLAND_DRAG_CONTEXT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CDK_TYPE_WAYLAND_DRAG_CONTEXT, CdkWaylandDragContextClass))
#define CDK_IS_WAYLAND_DRAG_CONTEXT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), CDK_TYPE_WAYLAND_DRAG_CONTEXT))
#define CDK_IS_WAYLAND_DRAG_CONTEXT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CDK_TYPE_WAYLAND_DRAG_CONTEXT))
#define CDK_WAYLAND_DRAG_CONTEXT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CDK_TYPE_WAYLAND_DRAG_CONTEXT, CdkWaylandDragContextClass))

typedef struct _CdkWaylandDragContext CdkWaylandDragContext;
typedef struct _CdkWaylandDragContextClass CdkWaylandDragContextClass;

struct _CdkWaylandDragContext
{
  CdkDragContext context;
  CdkWindow *dnd_window;
  struct wl_surface *dnd_surface;
  struct wl_data_source *data_source;
  CdkDragAction selected_action;
  uint32_t serial;
  gdouble x;
  gdouble y;
  gint hot_x;
  gint hot_y;
};

struct _CdkWaylandDragContextClass
{
  CdkDragContextClass parent_class;
};

static GList *contexts;

GType cdk_wayland_drag_context_get_type (void);

G_DEFINE_TYPE (CdkWaylandDragContext, cdk_wayland_drag_context, CDK_TYPE_DRAG_CONTEXT)

static void
cdk_wayland_drag_context_finalize (GObject *object)
{
  CdkWaylandDragContext *wayland_context = CDK_WAYLAND_DRAG_CONTEXT (object);
  CdkDragContext *context = CDK_DRAG_CONTEXT (object);
  CdkWindow *dnd_window;

  contexts = g_list_remove (contexts, context);

  if (context->is_source)
    {
      CdkDisplay *display = cdk_window_get_display (context->source_window);
      CdkAtom selection;
      CdkWindow *selection_owner;

      selection = cdk_drag_get_selection (context);
      selection_owner = cdk_selection_owner_get_for_display (display, selection);
      if (selection_owner == context->source_window)
        cdk_wayland_selection_unset_data_source (display, selection);

      cdk_drag_context_set_cursor (context, NULL);
    }

  if (wayland_context->data_source)
    wl_data_source_destroy (wayland_context->data_source);

  dnd_window = wayland_context->dnd_window;

  G_OBJECT_CLASS (cdk_wayland_drag_context_parent_class)->finalize (object);

  if (dnd_window)
    cdk_window_destroy (dnd_window);
}

void
_cdk_wayland_drag_context_emit_event (CdkDragContext *context,
                                      CdkEventType    type,
                                      guint32         time_)
{
  CdkWindow *window;
  CdkEvent *event;

  switch (type)
    {
    case CDK_DRAG_ENTER:
    case CDK_DRAG_LEAVE:
    case CDK_DRAG_MOTION:
    case CDK_DRAG_STATUS:
    case CDK_DROP_START:
    case CDK_DROP_FINISHED:
      break;
    default:
      return;
    }

  if (context->is_source)
    window = cdk_drag_context_get_source_window (context);
  else
    window = cdk_drag_context_get_dest_window (context);

  event = cdk_event_new (type);
  event->dnd.window = g_object_ref (window);
  event->dnd.context = g_object_ref (context);
  event->dnd.time = time_;
  event->dnd.x_root = CDK_WAYLAND_DRAG_CONTEXT (context)->x;
  event->dnd.y_root = CDK_WAYLAND_DRAG_CONTEXT (context)->y;
  cdk_event_set_device (event, cdk_drag_context_get_device (context));

  cdk_event_put (event);
  cdk_event_free (event);
}

static CdkWindow *
cdk_wayland_drag_context_find_window (CdkDragContext  *context,
				      CdkWindow       *drag_window,
				      CdkScreen       *screen,
				      gint             x_root,
				      gint             y_root,
				      CdkDragProtocol *protocol)
{
  CdkDevice *device;
  CdkWindow *window;

  device = cdk_drag_context_get_device (context);
  window = cdk_device_get_window_at_position (device, NULL, NULL);

  if (window)
    {
      window = cdk_window_get_toplevel (window);
      *protocol = CDK_DRAG_PROTO_WAYLAND;
      return g_object_ref (window);
    }

  return NULL;
}

static inline uint32_t
cdk_to_wl_actions (CdkDragAction action)
{
  uint32_t dnd_actions = 0;

  if (action & (CDK_ACTION_COPY | CDK_ACTION_LINK | CDK_ACTION_PRIVATE))
    dnd_actions |= WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY;
  if (action & CDK_ACTION_MOVE)
    dnd_actions |= WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE;
  if (action & CDK_ACTION_ASK)
    dnd_actions |= WL_DATA_DEVICE_MANAGER_DND_ACTION_ASK;

  return dnd_actions;
}

void
cdk_wayland_drag_context_set_action (CdkDragContext *context,
                                     CdkDragAction   action)
{
  context->suggested_action = context->action = action;
}

static gboolean
cdk_wayland_drag_context_drag_motion (CdkDragContext *context,
				      CdkWindow      *dest_window,
				      CdkDragProtocol protocol,
				      gint            x_root,
				      gint            y_root,
				      CdkDragAction   suggested_action,
				      CdkDragAction   possible_actions,
				      guint32         time)
{
  if (context->dest_window != dest_window)
    {
      context->dest_window = dest_window ? g_object_ref (dest_window) : NULL;
      _cdk_wayland_drag_context_set_coords (context, x_root, y_root);
      _cdk_wayland_drag_context_emit_event (context, CDK_DRAG_STATUS, time);
    }

  cdk_wayland_drag_context_set_action (context, suggested_action);

  return context->dest_window != NULL;
}

static void
cdk_wayland_drag_context_drag_abort (CdkDragContext *context,
				     guint32         time)
{
}

static void
cdk_wayland_drag_context_drag_drop (CdkDragContext *context,
				    guint32         time)
{
}

/* Destination side */

static void
cdk_wayland_drop_context_set_status (CdkDragContext *context,
                                     gboolean        accepted)
{
  CdkWaylandDragContext *context_wayland;
  CdkDisplay *display;
  struct wl_data_offer *wl_offer;

  if (!context->dest_window)
    return;

  context_wayland = CDK_WAYLAND_DRAG_CONTEXT (context);

  display = cdk_device_get_display (cdk_drag_context_get_device (context));
  wl_offer = cdk_wayland_selection_get_offer (display,
                                              cdk_drag_get_selection (context));

  if (!wl_offer)
    return;

  if (accepted)
    {
      GList *l;

      for (l = context->targets; l; l = l->next)
        {
          if (l->data != cdk_atom_intern_static_string ("DELETE"))
            break;
        }

      if (l)
        {
          gchar *mimetype = cdk_atom_name (l->data);

          wl_data_offer_accept (wl_offer, context_wayland->serial, mimetype);
          g_free (mimetype);
          return;
        }
    }

  wl_data_offer_accept (wl_offer, context_wayland->serial, NULL);
}

static void
cdk_wayland_drag_context_commit_status (CdkDragContext *context)
{
  CdkWaylandDragContext *wayland_context;
  CdkDisplay *display;
  uint32_t dnd_actions;

  wayland_context = CDK_WAYLAND_DRAG_CONTEXT (context);
  display = cdk_device_get_display (cdk_drag_context_get_device (context));

  dnd_actions = cdk_to_wl_actions (wayland_context->selected_action);
  cdk_wayland_selection_set_current_offer_actions (display, dnd_actions);

  cdk_wayland_drop_context_set_status (context, wayland_context->selected_action != 0);
}

static void
cdk_wayland_drag_context_drag_status (CdkDragContext *context,
				      CdkDragAction   action,
				      guint32         time_)
{
  CdkWaylandDragContext *wayland_context;

  wayland_context = CDK_WAYLAND_DRAG_CONTEXT (context);
  wayland_context->selected_action = action;
}

static void
cdk_wayland_drag_context_drop_reply (CdkDragContext *context,
				     gboolean        accepted,
				     guint32         time_)
{
  if (!accepted)
    cdk_wayland_drop_context_set_status (context, accepted);
}

static void
cdk_wayland_drag_context_drop_finish (CdkDragContext *context,
				      gboolean        success,
				      guint32         time)
{
  CdkDisplay *display = cdk_device_get_display (cdk_drag_context_get_device (context));
  CdkWaylandDisplay *display_wayland = CDK_WAYLAND_DISPLAY (display);
  CdkWaylandDragContext *wayland_context;
  struct wl_data_offer *wl_offer;
  CdkAtom selection;

  wayland_context = CDK_WAYLAND_DRAG_CONTEXT (context);
  selection = cdk_drag_get_selection (context);
  wl_offer = cdk_wayland_selection_get_offer (display, selection);

  if (wl_offer && success && wayland_context->selected_action &&
      wayland_context->selected_action != CDK_ACTION_ASK)
    {
      cdk_wayland_drag_context_commit_status (context);

      if (display_wayland->data_device_manager_version >=
          WL_DATA_OFFER_FINISH_SINCE_VERSION)
        wl_data_offer_finish (wl_offer);
    }

  cdk_wayland_selection_set_offer (display, selection, NULL);
}

static gboolean
cdk_wayland_drag_context_drop_status (CdkDragContext *context)
{
  return FALSE;
}

static CdkAtom
cdk_wayland_drag_context_get_selection (CdkDragContext *context)
{
  return cdk_atom_intern_static_string ("CdkWaylandSelection");
}

static void
cdk_wayland_drag_context_init (CdkWaylandDragContext *context_wayland)
{
  CdkDragContext *context;

  context = CDK_DRAG_CONTEXT (context_wayland);
  contexts = g_list_prepend (contexts, context);

  context->action = CDK_ACTION_COPY;
  context->suggested_action = CDK_ACTION_COPY;
  context->actions = CDK_ACTION_COPY | CDK_ACTION_MOVE;
}

static CdkWindow *
cdk_wayland_drag_context_get_drag_window (CdkDragContext *context)
{
  return CDK_WAYLAND_DRAG_CONTEXT (context)->dnd_window;
}

static void
cdk_wayland_drag_context_set_hotspot (CdkDragContext *context,
                                      gint            hot_x,
                                      gint            hot_y)
{
  CdkWaylandDragContext *context_wayland = CDK_WAYLAND_DRAG_CONTEXT (context);
  gint prev_hot_x = context_wayland->hot_x;
  gint prev_hot_y = context_wayland->hot_y;
  const CdkRectangle damage_rect = { .width = 1, .height = 1 };

  context_wayland->hot_x = hot_x;
  context_wayland->hot_y = hot_y;

  if (prev_hot_x == hot_x && prev_hot_y == hot_y)
    return;

  _cdk_wayland_window_offset_next_wl_buffer (context_wayland->dnd_window,
                                             prev_hot_x - hot_x, prev_hot_y - hot_y);
  cdk_window_invalidate_rect (context_wayland->dnd_window, &damage_rect, FALSE);
}

static gboolean
cdk_wayland_drag_context_manage_dnd (CdkDragContext *context,
                                     CdkWindow      *ipc_window,
                                     CdkDragAction   actions)
{
  CdkWaylandDragContext *context_wayland;
  CdkWaylandDisplay *display_wayland;
  CdkDevice *device;
  CdkWindow *toplevel;

  device = cdk_drag_context_get_device (context);
  display_wayland = CDK_WAYLAND_DISPLAY (cdk_device_get_display (device));
  toplevel = _cdk_device_window_at_position (device, NULL, NULL, NULL, TRUE);

  context_wayland = CDK_WAYLAND_DRAG_CONTEXT (context);

  if (display_wayland->data_device_manager_version >=
      WL_DATA_SOURCE_SET_ACTIONS_SINCE_VERSION)
    {
      wl_data_source_set_actions (context_wayland->data_source,
                                  cdk_to_wl_actions (actions));
    }

  wl_data_device_start_drag (cdk_wayland_device_get_data_device (device),
                             context_wayland->data_source,
                             cdk_wayland_window_get_wl_surface (toplevel),
			     context_wayland->dnd_surface,
                             _cdk_wayland_display_get_serial (display_wayland));

  cdk_seat_ungrab (cdk_device_get_seat (device));

  return TRUE;
}

static void
cdk_wayland_drag_context_set_cursor (CdkDragContext *context,
                                     CdkCursor      *cursor)
{
  CdkDevice *device = cdk_drag_context_get_device (context);

  cdk_wayland_seat_set_global_cursor (cdk_device_get_seat (device), cursor);
}

static void
cdk_wayland_drag_context_action_changed (CdkDragContext *context,
                                         CdkDragAction   action)
{
  CdkCursor *cursor;

  cursor = cdk_drag_get_cursor (context, action);
  cdk_drag_context_set_cursor (context, cursor);
}

static void
cdk_wayland_drag_context_drop_performed (CdkDragContext *context,
                                         guint32         time_)
{
  cdk_drag_context_set_cursor (context, NULL);
}

static void
cdk_wayland_drag_context_cancel (CdkDragContext      *context,
                                 CdkDragCancelReason  reason)
{
  cdk_drag_context_set_cursor (context, NULL);
}

static void
cdk_wayland_drag_context_drop_done (CdkDragContext *context,
                                    gboolean        success)
{
  CdkWaylandDragContext *context_wayland = CDK_WAYLAND_DRAG_CONTEXT (context);

  if (success)
    {
      if (context_wayland->dnd_window)
        cdk_window_hide (context_wayland->dnd_window);
    }
}

static void
cdk_wayland_drag_context_class_init (CdkWaylandDragContextClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CdkDragContextClass *context_class = CDK_DRAG_CONTEXT_CLASS (klass);

  object_class->finalize = cdk_wayland_drag_context_finalize;

  context_class->find_window = cdk_wayland_drag_context_find_window;
  context_class->drag_status = cdk_wayland_drag_context_drag_status;
  context_class->drag_motion = cdk_wayland_drag_context_drag_motion;
  context_class->drag_abort = cdk_wayland_drag_context_drag_abort;
  context_class->drag_drop = cdk_wayland_drag_context_drag_drop;
  context_class->drop_reply = cdk_wayland_drag_context_drop_reply;
  context_class->drop_finish = cdk_wayland_drag_context_drop_finish;
  context_class->drop_status = cdk_wayland_drag_context_drop_status;
  context_class->get_selection = cdk_wayland_drag_context_get_selection;
  context_class->get_drag_window = cdk_wayland_drag_context_get_drag_window;
  context_class->set_hotspot = cdk_wayland_drag_context_set_hotspot;
  context_class->drop_done = cdk_wayland_drag_context_drop_done;
  context_class->manage_dnd = cdk_wayland_drag_context_manage_dnd;
  context_class->set_cursor = cdk_wayland_drag_context_set_cursor;
  context_class->action_changed = cdk_wayland_drag_context_action_changed;
  context_class->drop_performed = cdk_wayland_drag_context_drop_performed;
  context_class->cancel = cdk_wayland_drag_context_cancel;
  context_class->commit_drag_status = cdk_wayland_drag_context_commit_status;
}

CdkDragProtocol
_cdk_wayland_window_get_drag_protocol (CdkWindow *window, CdkWindow **target)
{
  return CDK_DRAG_PROTO_WAYLAND;
}

void
_cdk_wayland_window_register_dnd (CdkWindow *window)
{
}

static CdkWindow *
create_dnd_window (CdkScreen *screen)
{
  CdkWindowAttr attrs;
  guint mask;

  attrs.x = attrs.y = 0;
  attrs.width = attrs.height = 100;
  attrs.wclass = CDK_INPUT_OUTPUT;
  attrs.window_type = CDK_WINDOW_TEMP;
  attrs.type_hint = CDK_WINDOW_TYPE_HINT_DND;
  attrs.visual = cdk_screen_get_system_visual (screen);

  mask = CDK_WA_X | CDK_WA_Y | CDK_WA_VISUAL | CDK_WA_TYPE_HINT;

  return cdk_window_new (cdk_screen_get_root_window (screen), &attrs, mask);
}

CdkDragContext *
_cdk_wayland_window_drag_begin (CdkWindow *window,
				CdkDevice *device,
				GList     *targets,
                                gint       x_root,
                                gint       y_root)
{
  CdkWaylandDragContext *context_wayland;
  CdkDragContext *context;
  GList *l;

  context_wayland = g_object_new (CDK_TYPE_WAYLAND_DRAG_CONTEXT, NULL);
  context = CDK_DRAG_CONTEXT (context_wayland);
  context->display = cdk_window_get_display (window);
  context->source_window = g_object_ref (window);
  context->is_source = TRUE;
  context->targets = g_list_copy (targets);

  cdk_drag_context_set_device (context, device);

  context_wayland->dnd_window = create_dnd_window (cdk_window_get_screen (window));
  context_wayland->dnd_surface = cdk_wayland_window_get_wl_surface (context_wayland->dnd_window);
  context_wayland->data_source =
    cdk_wayland_selection_get_data_source (window,
                                           cdk_wayland_drag_context_get_selection (context));

  for (l = context->targets; l; l = l->next)
    {
      gchar *mimetype = cdk_atom_name (l->data);

      wl_data_source_offer (context_wayland->data_source, mimetype);
      g_free (mimetype);
    }

  /* If there's no targets this is local DnD, ensure we create a target for it */
  if (!context->targets)
    {
      gchar *local_dnd_mime;
      local_dnd_mime = g_strdup_printf ("application/ctk+-local-dnd-%x", getpid());
      wl_data_source_offer (context_wayland->data_source, local_dnd_mime);
      g_free (local_dnd_mime);
    }

  return context;
}

CdkDragContext *
_cdk_wayland_drop_context_new (CdkDisplay            *display,
                               struct wl_data_device *data_device)
{
  CdkWaylandDragContext *context_wayland;
  CdkDragContext *context;

  context_wayland = g_object_new (CDK_TYPE_WAYLAND_DRAG_CONTEXT, NULL);
  context = CDK_DRAG_CONTEXT (context_wayland);
  context->display = display;
  context->is_source = FALSE;

  return context;
}

void
cdk_wayland_drop_context_update_targets (CdkDragContext *context)
{
  CdkDisplay *display;
  CdkDevice *device;

  device = cdk_drag_context_get_device (context);
  display = cdk_device_get_display (device);
  g_list_free (context->targets);
  context->targets = g_list_copy (cdk_wayland_selection_get_targets (display,
                                                                     cdk_drag_get_selection (context)));
}

void
_cdk_wayland_drag_context_set_coords (CdkDragContext *context,
                                      gdouble         x,
                                      gdouble         y)
{
  CdkWaylandDragContext *context_wayland;

  context_wayland = CDK_WAYLAND_DRAG_CONTEXT (context);
  context_wayland->x = x;
  context_wayland->y = y;
}

void
_cdk_wayland_drag_context_set_source_window (CdkDragContext *context,
                                             CdkWindow      *window)
{
  if (context->source_window)
    g_object_unref (context->source_window);

  context->source_window = window ? g_object_ref (window) : NULL;
}

void
_cdk_wayland_drag_context_set_dest_window (CdkDragContext *context,
                                           CdkWindow      *dest_window,
                                           uint32_t        serial)
{
  if (context->dest_window)
    g_object_unref (context->dest_window);

  context->dest_window = dest_window ? g_object_ref (dest_window) : NULL;
  CDK_WAYLAND_DRAG_CONTEXT (context)->serial = serial;
  cdk_wayland_drop_context_update_targets (context);
}

CdkDragContext *
cdk_wayland_drag_context_lookup_by_data_source (struct wl_data_source *source)
{
  GList *l;

  for (l = contexts; l; l = l->next)
    {
      CdkWaylandDragContext *wayland_context = l->data;

      if (wayland_context->data_source == source)
        return l->data;
    }

  return NULL;
}

CdkDragContext *
cdk_wayland_drag_context_lookup_by_source_window (CdkWindow *window)
{
  GList *l;

  for (l = contexts; l; l = l->next)
    {
      if (window == cdk_drag_context_get_source_window (l->data))
        return l->data;
    }

  return NULL;
}

struct wl_data_source *
cdk_wayland_drag_context_get_data_source (CdkDragContext *context)
{
  return CDK_WAYLAND_DRAG_CONTEXT (context)->data_source;
}
