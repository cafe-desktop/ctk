/* CTK - The GIMP Toolkit
 * Copyright © 2012 Carlos Garnacho <carlosg@gnome.org>
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
#include "ctkprivatetypebuiltins.h"
#include "ctktexthandleprivate.h"
#include "ctkmarshalers.h"
#include "ctkprivate.h"
#include "ctkwindowprivate.h"
#include "ctkcssnodeprivate.h"
#include "ctkwidgetprivate.h"
#include "ctkintl.h"

#include <ctk/ctk.h>

typedef struct _CtkTextHandlePrivate CtkTextHandlePrivate;
typedef struct _HandleWindow HandleWindow;

enum {
  DRAG_STARTED,
  HANDLE_DRAGGED,
  DRAG_FINISHED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_PARENT
};

struct _HandleWindow
{
  CtkWidget *widget;
  CdkRectangle pointing_to;
  CtkBorder border;
  gint dx;
  gint dy;
  CtkTextDirection dir;
  guint dragged : 1;
  guint mode_visible : 1;
  guint user_visible : 1;
  guint has_point : 1;
};

struct _CtkTextHandlePrivate
{
  HandleWindow windows[2];
  CtkWidget *parent;
  CtkScrollable *parent_scrollable;
  CtkAdjustment *vadj;
  CtkAdjustment *hadj;
  guint hierarchy_changed_id;
  guint scrollable_notify_id;
  guint mode : 2;
};

G_DEFINE_TYPE_WITH_PRIVATE (CtkTextHandle, _ctk_text_handle, G_TYPE_OBJECT)

static guint signals[LAST_SIGNAL] = { 0 };

static void _ctk_text_handle_update (CtkTextHandle         *handle,
                                     CtkTextHandlePosition  pos);

static void
_ctk_text_handle_get_size (CtkTextHandle *handle,
                           gint          *width,
                           gint          *height)
{
  CtkTextHandlePrivate *priv;
  gint w, h;

  priv = handle->priv;

  ctk_widget_style_get (priv->parent,
                        "text-handle-width", &w,
                        "text-handle-height", &h,
                        NULL);
  if (width)
    *width = w;

  if (height)
    *height = h;
}

static void
_ctk_text_handle_draw (CtkTextHandle         *handle,
                       cairo_t               *cr,
                       CtkTextHandlePosition  pos)
{
  CtkTextHandlePrivate *priv;
  HandleWindow *handle_window;
  CtkStyleContext *context;
  gint width, height;

  priv = handle->priv;
  handle_window = &priv->windows[pos];

  context = ctk_widget_get_style_context (handle_window->widget);
  _ctk_text_handle_get_size (handle, &width, &height);

  cairo_save (cr);
  cairo_translate (cr, handle_window->border.left, handle_window->border.top);

  ctk_render_handle (context, cr, 0, 0, width, height);

  cairo_restore (cr);
}

static gint
_text_handle_pos_from_widget (CtkTextHandle *handle,
                              CtkWidget     *widget)
{
  CtkTextHandlePrivate *priv = handle->priv;

  if (widget == priv->windows[CTK_TEXT_HANDLE_POSITION_SELECTION_START].widget)
    return CTK_TEXT_HANDLE_POSITION_SELECTION_START;
  else if (widget == priv->windows[CTK_TEXT_HANDLE_POSITION_SELECTION_END].widget)
    return CTK_TEXT_HANDLE_POSITION_SELECTION_END;
  else
    return -1;
}

static gboolean
ctk_text_handle_widget_draw (CtkWidget     *widget,
                             cairo_t       *cr,
                             CtkTextHandle *handle)
{
  gint pos;

  pos = _text_handle_pos_from_widget (handle, widget);

  if (pos < 0)
    return FALSE;

#if 0
  /* Show the invisible border */
  cairo_set_source_rgba (cr, 1, 0, 0, 0.5);
  cairo_paint (cr);
#endif

  _ctk_text_handle_draw (handle, cr, pos);
  return TRUE;
}

static void
ctk_text_handle_set_state (CtkTextHandle *handle,
                           gint           pos,
                           CtkStateFlags  state)
{
  CtkTextHandlePrivate *priv = handle->priv;

  if (!priv->windows[pos].widget)
    return;

  ctk_widget_set_state_flags (priv->windows[pos].widget, state, FALSE);
  ctk_widget_queue_draw (priv->windows[pos].widget);
}

static void
ctk_text_handle_unset_state (CtkTextHandle *handle,
                             gint           pos,
                             CtkStateFlags  state)
{
  CtkTextHandlePrivate *priv = handle->priv;

  if (!priv->windows[pos].widget)
    return;

  ctk_widget_unset_state_flags (priv->windows[pos].widget, state);
  ctk_widget_queue_draw (priv->windows[pos].widget);
}

static gboolean
ctk_text_handle_widget_event (CtkWidget     *widget,
                              CdkEvent      *event,
                              CtkTextHandle *handle)
{
  CtkTextHandlePrivate *priv;
  gint pos;

  priv = handle->priv;
  pos = _text_handle_pos_from_widget (handle, widget);

  if (pos < 0)
    return FALSE;

  if (event->type == CDK_BUTTON_PRESS)
    {
      priv->windows[pos].dx = event->button.x;
      priv->windows[pos].dy = event->button.y;
      priv->windows[pos].dragged = TRUE;
      ctk_text_handle_set_state (handle, pos, CTK_STATE_FLAG_ACTIVE);
      g_signal_emit (handle, signals[DRAG_STARTED], 0, pos);
    }
  else if (event->type == CDK_BUTTON_RELEASE)
    {
      g_signal_emit (handle, signals[DRAG_FINISHED], 0, pos);
      priv->windows[pos].dragged = FALSE;
      ctk_text_handle_unset_state (handle, pos, CTK_STATE_FLAG_ACTIVE);
    }
  else if (event->type == CDK_ENTER_NOTIFY)
    ctk_text_handle_set_state (handle, pos, CTK_STATE_FLAG_PRELIGHT);
  else if (event->type == CDK_LEAVE_NOTIFY)
    {
      if (!priv->windows[pos].dragged &&
          (event->crossing.mode == CDK_CROSSING_NORMAL ||
           event->crossing.mode == CDK_CROSSING_UNGRAB))
        ctk_text_handle_unset_state (handle, pos, CTK_STATE_FLAG_PRELIGHT);
    }
  else if (event->type == CDK_MOTION_NOTIFY &&
           event->motion.state & CDK_BUTTON1_MASK &&
           priv->windows[pos].dragged)
    {
      gint x, y, handle_width, handle_height;
      cairo_rectangle_int_t rect;
      CtkAllocation allocation;
      CtkWidget *window;

      window = ctk_widget_get_parent (priv->windows[pos].widget);
      ctk_widget_get_allocation (priv->windows[pos].widget, &allocation);
      _ctk_text_handle_get_size (handle, &handle_width, &handle_height);

      _ctk_window_get_popover_position (CTK_WINDOW (window),
                                        priv->windows[pos].widget,
                                        NULL, &rect);

      x = rect.x + event->motion.x - priv->windows[pos].dx;
      y = rect.y + event->motion.y - priv->windows[pos].dy +
        priv->windows[pos].border.top / 2;

      if (pos == CTK_TEXT_HANDLE_POSITION_CURSOR &&
          priv->mode == CTK_TEXT_HANDLE_MODE_CURSOR)
        x += handle_width / 2;
      else if ((pos == CTK_TEXT_HANDLE_POSITION_CURSOR &&
                priv->windows[pos].dir == CTK_TEXT_DIR_RTL) ||
               (pos == CTK_TEXT_HANDLE_POSITION_SELECTION_START &&
                priv->windows[pos].dir != CTK_TEXT_DIR_RTL))
        x += handle_width;

      ctk_widget_translate_coordinates (window, priv->parent, x, y, &x, &y);
      g_signal_emit (handle, signals[HANDLE_DRAGGED], 0, pos, x, y);
    }

  return TRUE;
}

static void
ctk_text_handle_widget_style_updated (CtkWidget     *widget,
                                      CtkTextHandle *handle)
{
  CtkTextHandlePrivate *priv;

  priv = handle->priv;
  ctk_style_context_set_parent (ctk_widget_get_style_context (widget),
                                ctk_widget_get_style_context (priv->parent));

  _ctk_text_handle_update (handle, CTK_TEXT_HANDLE_POSITION_SELECTION_START);
  _ctk_text_handle_update (handle, CTK_TEXT_HANDLE_POSITION_SELECTION_END);
}

static CtkWidget *
_ctk_text_handle_ensure_widget (CtkTextHandle         *handle,
                                CtkTextHandlePosition  pos)
{
  CtkTextHandlePrivate *priv;

  priv = handle->priv;

  if (!priv->windows[pos].widget)
    {
      CtkWidget *widget, *window;
      CtkStyleContext *context;

      widget = ctk_event_box_new ();
      ctk_event_box_set_visible_window (CTK_EVENT_BOX (widget), TRUE);
      ctk_widget_set_events (widget,
                             CDK_BUTTON_PRESS_MASK |
                             CDK_BUTTON_RELEASE_MASK |
                             CDK_ENTER_NOTIFY_MASK |
                             CDK_LEAVE_NOTIFY_MASK |
                             CDK_POINTER_MOTION_MASK);

      ctk_widget_set_direction (widget, priv->windows[pos].dir);

      g_signal_connect (widget, "draw",
                        G_CALLBACK (ctk_text_handle_widget_draw), handle);
      g_signal_connect (widget, "event",
                        G_CALLBACK (ctk_text_handle_widget_event), handle);
      g_signal_connect (widget, "style-updated",
                        G_CALLBACK (ctk_text_handle_widget_style_updated),
                        handle);

      priv->windows[pos].widget = g_object_ref_sink (widget);
      window = ctk_widget_get_ancestor (priv->parent, CTK_TYPE_WINDOW);
      _ctk_window_add_popover (CTK_WINDOW (window), widget, priv->parent, FALSE);

      context = ctk_widget_get_style_context (widget);
      ctk_style_context_set_parent (context, ctk_widget_get_style_context (priv->parent));
      ctk_css_node_set_name (ctk_widget_get_css_node (widget), I_("cursor-handle"));
      if (pos == CTK_TEXT_HANDLE_POSITION_SELECTION_END)
        {
          ctk_style_context_add_class (context, CTK_STYLE_CLASS_BOTTOM);
          if (priv->mode == CTK_TEXT_HANDLE_MODE_CURSOR)
            ctk_style_context_add_class (context, CTK_STYLE_CLASS_INSERTION_CURSOR);
        }
      else
        ctk_style_context_add_class (context, CTK_STYLE_CLASS_TOP);
    }

  return priv->windows[pos].widget;
}

static void
_handle_update_child_visible (CtkTextHandle         *handle,
                              CtkTextHandlePosition  pos)
{
  HandleWindow *handle_window;
  CtkTextHandlePrivate *priv;
  cairo_rectangle_int_t rect;
  CtkAllocation allocation;
  CtkWidget *parent;

  priv = handle->priv;
  handle_window = &priv->windows[pos];

  if (!priv->parent_scrollable)
    {
      ctk_widget_set_child_visible (handle_window->widget, TRUE);
      return;
    }

  parent = ctk_widget_get_parent (CTK_WIDGET (priv->parent_scrollable));
  rect = handle_window->pointing_to;

  ctk_widget_translate_coordinates (priv->parent, parent,
                                    rect.x, rect.y, &rect.x, &rect.y);

  ctk_widget_get_allocation (CTK_WIDGET (parent), &allocation);

  if (rect.x < 0 || rect.x + rect.width > allocation.width ||
      rect.y < 0 || rect.y + rect.height > allocation.height)
    ctk_widget_set_child_visible (handle_window->widget, FALSE);
  else
    ctk_widget_set_child_visible (handle_window->widget, TRUE);
}

static void
_ctk_text_handle_update (CtkTextHandle         *handle,
                         CtkTextHandlePosition  pos)
{
  CtkTextHandlePrivate *priv;
  HandleWindow *handle_window;
  CtkBorder *border;

  priv = handle->priv;
  handle_window = &priv->windows[pos];
  border = &handle_window->border;

  if (!priv->parent || !ctk_widget_is_drawable (priv->parent))
    return;

  if (handle_window->has_point &&
      handle_window->mode_visible && handle_window->user_visible)
    {
      cairo_rectangle_int_t rect;
      gint width, height;
      CtkWidget *window;
      CtkAllocation alloc;
      gint w, h;

      _ctk_text_handle_ensure_widget (handle, pos);
      _ctk_text_handle_get_size (handle, &width, &height);

      border->top = height;
      border->bottom = height;
      border->left = width;
      border->right = width;

      rect.x = handle_window->pointing_to.x;
      rect.y = handle_window->pointing_to.y + handle_window->pointing_to.height - handle_window->border.top;
      rect.width = width;
      rect.height = 0;

      _handle_update_child_visible (handle, pos);

      window = ctk_widget_get_parent (handle_window->widget);
      ctk_widget_translate_coordinates (priv->parent, window,
                                        rect.x, rect.y, &rect.x, &rect.y);

      if (pos == CTK_TEXT_HANDLE_POSITION_CURSOR &&
          priv->mode == CTK_TEXT_HANDLE_MODE_CURSOR)
        rect.x -= rect.width / 2;
      else if ((pos == CTK_TEXT_HANDLE_POSITION_CURSOR &&
                handle_window->dir == CTK_TEXT_DIR_RTL) ||
               (pos == CTK_TEXT_HANDLE_POSITION_SELECTION_START &&
                handle_window->dir != CTK_TEXT_DIR_RTL))
        rect.x -= rect.width;

      /* The goal is to make the window 3 times as wide and high. The handle
       * will be rendered in the center, making the rest an invisible border.
       * If we hit the edge of the toplevel, we shrink the border to avoid
       * mispositioning the handle, if at all possible. This calculation uses
       * knowledge about how popover_get_rect() works.
       */

      ctk_widget_get_allocation (window, &alloc);

      w = width + border->left + border->right;
      h = height + border->top + border->bottom;

      if (rect.x + rect.width/2 - w/2 < alloc.x)
        border->left = MAX (0, border->left - (alloc.x - (rect.x + rect.width/2 - w/2)));
      if (rect.y + rect.height/2 - h/2 < alloc.y)
        border->top = MAX (0, border->top - (alloc.y - (rect.y + rect.height/2 - h/2)));
      if (rect.x + rect.width/2 + w/2 > alloc.x + alloc.width)
        border->right = MAX (0, border->right - (rect.x + rect.width/2 + w/2 - (alloc.x + alloc.width)));
      if (rect.y + rect.height/2 + h/2 > alloc.y + alloc.height)
        border->bottom = MAX (0, border->bottom - (rect.y + rect.height/2 + h/2 - (alloc.y + alloc.height)));

      width += border->left + border->right;
      height += border->top + border->bottom;

      ctk_widget_set_size_request (handle_window->widget, width, height);
      ctk_widget_show (handle_window->widget);
      _ctk_window_raise_popover (CTK_WINDOW (window), handle_window->widget);
      _ctk_window_set_popover_position (CTK_WINDOW (window),
                                        handle_window->widget,
                                        CTK_POS_BOTTOM, &rect);
    }
  else if (handle_window->widget)
    ctk_widget_hide (handle_window->widget);
}

static void
adjustment_changed_cb (CtkAdjustment *adjustment G_GNUC_UNUSED,
                       CtkTextHandle *handle)
{
  _ctk_text_handle_update (handle, CTK_TEXT_HANDLE_POSITION_SELECTION_START);
  _ctk_text_handle_update (handle, CTK_TEXT_HANDLE_POSITION_SELECTION_END);
}

static void
_ctk_text_handle_set_scrollable (CtkTextHandle *handle,
                                 CtkScrollable *scrollable)
{
  CtkTextHandlePrivate *priv;

  priv = handle->priv;

  if (priv->vadj)
    {
      g_signal_handlers_disconnect_by_data (priv->vadj, handle);
      g_clear_object (&priv->vadj);
    }

  if (priv->hadj)
    {
      g_signal_handlers_disconnect_by_data (priv->hadj, handle);
      g_clear_object (&priv->hadj);
    }

  if (priv->parent_scrollable)
    g_object_remove_weak_pointer (G_OBJECT (priv->parent_scrollable), (gpointer *) &priv->parent_scrollable);

  priv->parent_scrollable = scrollable;

  if (scrollable)
    {
      g_object_add_weak_pointer (G_OBJECT (priv->parent_scrollable), (gpointer *) &priv->parent_scrollable);

      priv->vadj = ctk_scrollable_get_vadjustment (scrollable);
      priv->hadj = ctk_scrollable_get_hadjustment (scrollable);

      if (priv->vadj)
        {
          g_object_ref (priv->vadj);
          g_signal_connect (priv->vadj, "changed",
                            G_CALLBACK (adjustment_changed_cb), handle);
          g_signal_connect (priv->vadj, "value-changed",
                            G_CALLBACK (adjustment_changed_cb), handle);
        }

      if (priv->hadj)
        {
          g_object_ref (priv->hadj);
          g_signal_connect (priv->hadj, "changed",
                            G_CALLBACK (adjustment_changed_cb), handle);
          g_signal_connect (priv->hadj, "value-changed",
                            G_CALLBACK (adjustment_changed_cb), handle);
        }
    }
}

static void
_ctk_text_handle_scrollable_notify (GObject       *object,
                                    GParamSpec    *pspec,
                                    CtkTextHandle *handle)
{
  if (pspec->value_type == CTK_TYPE_ADJUSTMENT)
    _ctk_text_handle_set_scrollable (handle, CTK_SCROLLABLE (object));
}

static void
_ctk_text_handle_update_scrollable (CtkTextHandle *handle,
                                    CtkScrollable *scrollable)
{
  CtkTextHandlePrivate *priv;

  priv = handle->priv;

  if (priv->parent_scrollable == scrollable)
    return;

  if (priv->parent_scrollable && priv->scrollable_notify_id &&
      g_signal_handler_is_connected (priv->parent_scrollable,
                                     priv->scrollable_notify_id))
    g_signal_handler_disconnect (priv->parent_scrollable,
                                 priv->scrollable_notify_id);

  _ctk_text_handle_set_scrollable (handle, scrollable);

  if (priv->parent_scrollable)
    priv->scrollable_notify_id =
      g_signal_connect (priv->parent_scrollable, "notify",
                        G_CALLBACK (_ctk_text_handle_scrollable_notify),
                        handle);
}

static CtkWidget *
ctk_text_handle_lookup_scrollable (CtkTextHandle *handle)
{
  CtkTextHandlePrivate *priv;
  CtkWidget *scrolled_window;

  priv = handle->priv;
  scrolled_window = ctk_widget_get_ancestor (priv->parent,
                                             CTK_TYPE_SCROLLED_WINDOW);
  if (!scrolled_window)
    return NULL;

  return ctk_bin_get_child (CTK_BIN (scrolled_window));
}

static void
_ctk_text_handle_parent_hierarchy_changed (CtkWidget     *widget,
                                           CtkWindow     *previous_toplevel,
                                           CtkTextHandle *handle)
{
  CtkWidget *toplevel, *scrollable;
  CtkTextHandlePrivate *priv;

  priv = handle->priv;
  toplevel = ctk_widget_get_ancestor (widget, CTK_TYPE_WINDOW);

  if (previous_toplevel && !toplevel)
    {
      if (priv->windows[CTK_TEXT_HANDLE_POSITION_SELECTION_START].widget)
        {
          _ctk_window_remove_popover (CTK_WINDOW (previous_toplevel),
                                      priv->windows[CTK_TEXT_HANDLE_POSITION_SELECTION_START].widget);
          g_object_unref (priv->windows[CTK_TEXT_HANDLE_POSITION_SELECTION_START].widget);
          priv->windows[CTK_TEXT_HANDLE_POSITION_SELECTION_START].widget = NULL;
        }

      if (priv->windows[CTK_TEXT_HANDLE_POSITION_SELECTION_END].widget)
        {
          _ctk_window_remove_popover (CTK_WINDOW (previous_toplevel),
                                      priv->windows[CTK_TEXT_HANDLE_POSITION_SELECTION_END].widget);
          g_object_unref (priv->windows[CTK_TEXT_HANDLE_POSITION_SELECTION_END].widget);
          priv->windows[CTK_TEXT_HANDLE_POSITION_SELECTION_END].widget = NULL;
        }
    }

  scrollable = ctk_text_handle_lookup_scrollable (handle);
  _ctk_text_handle_update_scrollable (handle, CTK_SCROLLABLE (scrollable));
}

static void
_ctk_text_handle_set_parent (CtkTextHandle *handle,
                             CtkWidget     *parent)
{
  CtkTextHandlePrivate *priv;
  CtkWidget *scrollable = NULL;

  priv = handle->priv;

  if (priv->parent == parent)
    return;

  if (priv->parent && priv->hierarchy_changed_id &&
      g_signal_handler_is_connected (priv->parent, priv->hierarchy_changed_id))
    g_signal_handler_disconnect (priv->parent, priv->hierarchy_changed_id);

  priv->parent = parent;

  if (parent)
    {
      priv->hierarchy_changed_id =
        g_signal_connect (parent, "hierarchy-changed",
                          G_CALLBACK (_ctk_text_handle_parent_hierarchy_changed),
                          handle);

      scrollable = ctk_text_handle_lookup_scrollable (handle);
    }

  _ctk_text_handle_update_scrollable (handle, CTK_SCROLLABLE (scrollable));
}

static void
ctk_text_handle_finalize (GObject *object)
{
  CtkTextHandlePrivate *priv;

  priv = CTK_TEXT_HANDLE (object)->priv;

  _ctk_text_handle_set_parent (CTK_TEXT_HANDLE (object), NULL);

  /* We sank the references, unref here */
  if (priv->windows[CTK_TEXT_HANDLE_POSITION_SELECTION_START].widget)
    g_object_unref (priv->windows[CTK_TEXT_HANDLE_POSITION_SELECTION_START].widget);

  if (priv->windows[CTK_TEXT_HANDLE_POSITION_SELECTION_END].widget)
    g_object_unref (priv->windows[CTK_TEXT_HANDLE_POSITION_SELECTION_END].widget);

  G_OBJECT_CLASS (_ctk_text_handle_parent_class)->finalize (object);
}

static void
ctk_text_handle_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  CtkTextHandle *handle;

  handle = CTK_TEXT_HANDLE (object);

  switch (prop_id)
    {
    case PROP_PARENT:
      _ctk_text_handle_set_parent (handle, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ctk_text_handle_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  CtkTextHandlePrivate *priv;

  priv = CTK_TEXT_HANDLE (object)->priv;

  switch (prop_id)
    {
    case PROP_PARENT:
      g_value_set_object (value, priv->parent);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
_ctk_text_handle_class_init (CtkTextHandleClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = ctk_text_handle_finalize;
  object_class->set_property = ctk_text_handle_set_property;
  object_class->get_property = ctk_text_handle_get_property;

  signals[HANDLE_DRAGGED] =
    g_signal_new (I_("handle-dragged"),
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkTextHandleClass, handle_dragged),
		  NULL, NULL,
		  _ctk_marshal_VOID__ENUM_INT_INT,
		  G_TYPE_NONE, 3,
                  CTK_TYPE_TEXT_HANDLE_POSITION,
                  G_TYPE_INT, G_TYPE_INT);
  signals[DRAG_STARTED] =
    g_signal_new (I_("drag-started"),
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_LAST, 0,
		  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  CTK_TYPE_TEXT_HANDLE_POSITION);
  signals[DRAG_FINISHED] =
    g_signal_new (I_("drag-finished"),
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_LAST, 0,
		  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  CTK_TYPE_TEXT_HANDLE_POSITION);

  g_object_class_install_property (object_class,
                                   PROP_PARENT,
                                   g_param_spec_object ("parent",
                                                        P_("Parent widget"),
                                                        P_("Parent widget"),
                                                        CTK_TYPE_WIDGET,
                                                        CTK_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
_ctk_text_handle_init (CtkTextHandle *handle)
{
  handle->priv = _ctk_text_handle_get_instance_private (handle);
}

CtkTextHandle *
_ctk_text_handle_new (CtkWidget *parent)
{
  return g_object_new (CTK_TYPE_TEXT_HANDLE,
                       "parent", parent,
                       NULL);
}

void
_ctk_text_handle_set_mode (CtkTextHandle     *handle,
                           CtkTextHandleMode  mode)
{
  CtkTextHandlePrivate *priv;
  HandleWindow *start, *end;

  g_return_if_fail (CTK_IS_TEXT_HANDLE (handle));

  priv = handle->priv;

  if (priv->mode == mode)
    return;

  priv->mode = mode;
  start = &priv->windows[CTK_TEXT_HANDLE_POSITION_SELECTION_START];
  end = &priv->windows[CTK_TEXT_HANDLE_POSITION_SELECTION_END];

  switch (mode)
    {
    case CTK_TEXT_HANDLE_MODE_CURSOR:
      start->mode_visible = FALSE;
      /* end = cursor */
      end->mode_visible = TRUE;
      break;
    case CTK_TEXT_HANDLE_MODE_SELECTION:
      start->mode_visible = TRUE;
      end->mode_visible = TRUE;
      break;
    case CTK_TEXT_HANDLE_MODE_NONE:
    default:
      start->mode_visible = FALSE;
      end->mode_visible = FALSE;
      break;
    }

  if (end->widget)
    {
      if (mode == CTK_TEXT_HANDLE_MODE_CURSOR)
        ctk_style_context_add_class (ctk_widget_get_style_context (end->widget), CTK_STYLE_CLASS_INSERTION_CURSOR);
      else
        ctk_style_context_remove_class (ctk_widget_get_style_context (end->widget), CTK_STYLE_CLASS_INSERTION_CURSOR);
    }

  _ctk_text_handle_update (handle, CTK_TEXT_HANDLE_POSITION_SELECTION_START);
  _ctk_text_handle_update (handle, CTK_TEXT_HANDLE_POSITION_SELECTION_END);

  if (start->widget && start->mode_visible)
    ctk_widget_queue_draw (start->widget);
  if (end->widget && end->mode_visible)
    ctk_widget_queue_draw (end->widget);
}

CtkTextHandleMode
_ctk_text_handle_get_mode (CtkTextHandle *handle)
{
  CtkTextHandlePrivate *priv;

  g_return_val_if_fail (CTK_IS_TEXT_HANDLE (handle), CTK_TEXT_HANDLE_MODE_NONE);

  priv = handle->priv;
  return priv->mode;
}

void
_ctk_text_handle_set_position (CtkTextHandle         *handle,
                               CtkTextHandlePosition  pos,
                               CdkRectangle          *rect)
{
  CtkTextHandlePrivate *priv;
  HandleWindow *handle_window;

  g_return_if_fail (CTK_IS_TEXT_HANDLE (handle));

  priv = handle->priv;
  pos = CLAMP (pos, CTK_TEXT_HANDLE_POSITION_CURSOR,
               CTK_TEXT_HANDLE_POSITION_SELECTION_START);
  handle_window = &priv->windows[pos];

  if (priv->mode == CTK_TEXT_HANDLE_MODE_NONE ||
      (priv->mode == CTK_TEXT_HANDLE_MODE_CURSOR &&
       pos != CTK_TEXT_HANDLE_POSITION_CURSOR))
    return;

  handle_window->pointing_to = *rect;
  handle_window->has_point = TRUE;

  if (ctk_widget_is_visible (priv->parent))
    _ctk_text_handle_update (handle, pos);
}

void
_ctk_text_handle_set_visible (CtkTextHandle         *handle,
                              CtkTextHandlePosition  pos,
                              gboolean               visible)
{
  CtkTextHandlePrivate *priv;

  g_return_if_fail (CTK_IS_TEXT_HANDLE (handle));

  priv = handle->priv;
  pos = CLAMP (pos, CTK_TEXT_HANDLE_POSITION_CURSOR,
               CTK_TEXT_HANDLE_POSITION_SELECTION_START);

  priv->windows[pos].user_visible = visible;

  if (ctk_widget_is_visible (priv->parent))
    _ctk_text_handle_update (handle, pos);
}

gboolean
_ctk_text_handle_get_is_dragged (CtkTextHandle         *handle,
                                 CtkTextHandlePosition  pos)
{
  CtkTextHandlePrivate *priv;

  g_return_val_if_fail (CTK_IS_TEXT_HANDLE (handle), FALSE);

  priv = handle->priv;
  pos = CLAMP (pos, CTK_TEXT_HANDLE_POSITION_CURSOR,
               CTK_TEXT_HANDLE_POSITION_SELECTION_START);

  return priv->windows[pos].dragged;
}

void
_ctk_text_handle_set_direction (CtkTextHandle         *handle,
                                CtkTextHandlePosition  pos,
                                CtkTextDirection       dir)
{
  CtkTextHandlePrivate *priv;

  g_return_if_fail (CTK_IS_TEXT_HANDLE (handle));

  priv = handle->priv;
  pos = CLAMP (pos, CTK_TEXT_HANDLE_POSITION_CURSOR,
               CTK_TEXT_HANDLE_POSITION_SELECTION_START);
  priv->windows[pos].dir = dir;

  if (priv->windows[pos].widget)
    {
      ctk_widget_set_direction (priv->windows[pos].widget, dir);
      _ctk_text_handle_update (handle, pos);
    }
}
