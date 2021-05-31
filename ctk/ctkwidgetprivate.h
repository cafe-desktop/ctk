/* CTK - The GIMP Toolkit
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
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_WIDGET_PRIVATE_H__
#define __CTK_WIDGET_PRIVATE_H__

#include "ctkcsstypesprivate.h"
#include "ctkwidget.h"
#include "ctkcontainer.h"
#include "ctkeventcontroller.h"
#include "ctkactionmuxer.h"
#include "ctksizerequestcacheprivate.h"

G_BEGIN_DECLS

#define CTK_STATE_FLAGS_BITS 13

struct _CtkWidgetPrivate
{
  /* The state of the widget. Needs to be able to hold all CtkStateFlags bits
   * (defined in "ctkenums.h").
   */
  guint state_flags : CTK_STATE_FLAGS_BITS;

  guint direction             : 2;

#ifdef G_ENABLE_DEBUG
  guint highlight_resize      : 1;
#endif

  guint in_destruction        : 1;
  guint toplevel              : 1;
  guint anchored              : 1;
  guint composite_child       : 1;
  guint no_window             : 1;
  guint realized              : 1;
  guint mapped                : 1;
  guint visible               : 1;
  guint sensitive             : 1;
  guint can_focus             : 1;
  guint has_focus             : 1;
  guint focus_on_click        : 1;
  guint can_default           : 1;
  guint has_default           : 1;
  guint receives_default      : 1;
  guint has_grab              : 1;
  guint shadowed              : 1;
  guint app_paintable         : 1;
  guint double_buffered       : 1;
  guint redraw_on_alloc       : 1;
  guint no_show_all           : 1;
  guint child_visible         : 1;
  guint multidevice           : 1;
  guint has_shape_mask        : 1;
  guint in_reparent           : 1;

  /* Queue-resize related flags */
  guint resize_needed         : 1; /* queue_resize() has been called but no get_preferred_size() yet */
  guint alloc_needed          : 1; /* this widget needs a size_allocate() call */
  guint alloc_needed_on_child : 1; /* 0 or more children - or this widget - need a size_allocate() call */

  /* Expand-related flags */
  guint need_compute_expand   : 1; /* Need to recompute computed_[hv]_expand */
  guint computed_hexpand      : 1; /* computed results (composite of child flags) */
  guint computed_vexpand      : 1;
  guint hexpand               : 1; /* application-forced expand */
  guint vexpand               : 1;
  guint hexpand_set           : 1; /* whether to use application-forced  */
  guint vexpand_set           : 1; /* instead of computing from children */
  guint has_tooltip           : 1;
  guint frameclock_connected  : 1;

  /* SizeGroup related flags */
  guint have_size_groups      : 1;

  /* Alignment */
  guint   halign              : 4;
  guint   valign              : 4;

  guint8 alpha;
  guint8 user_alpha;

#ifdef G_ENABLE_CONSISTENCY_CHECKS
  /* Number of ctk_widget_push_verify_invariants () */
  guint8 verifying_invariants_count;
#endif

  gint width;
  gint height;
  CtkBorder margin;

  /* Animations and other things to update on clock ticks */
  guint clock_tick_id;
  GList *tick_callbacks;

  /* The widget's name. If the widget does not have a name
   * (the name is NULL), then its name (as returned by
   * "ctk_widget_get_name") is its class's name.
   * Among other things, the widget name is used to determine
   * the style to use for a widget.
   */
  gchar *name;

  /* The list of attached windows to this widget.
   * We keep a list in order to call reset_style to all of them,
   * recursively.
   */
  GList *attached_windows;

  /* The style for the widget. The style contains the
   * colors the widget should be drawn in for each state
   * along with graphics contexts used to draw with and
   * the font to use for text.
   */
  CtkStyle *style;
  CtkCssNode *cssnode;
  CtkStyleContext *context;

  /* The widget's allocated size */
  CtkAllocation allocated_size;
  gint allocated_size_baseline;
  CtkAllocation allocation;
  CtkAllocation clip;
  gint allocated_baseline;

  /* The widget's requested sizes */
  SizeRequestCache requests;

  /* The widget's window or its parent window if it does
   * not have a window. (Which will be indicated by the
   * no_window field being set).
   */
  GdkWindow *window;
  GList *registered_windows;

  /* The widget's parent */
  CtkWidget *parent;

  GList *event_controllers;

  AtkObject *accessible;
};

CtkCssNode *  ctk_widget_get_css_node       (CtkWidget *widget);
void         _ctk_widget_set_visible_flag   (CtkWidget *widget,
                                             gboolean   visible);
gboolean     _ctk_widget_get_in_reparent    (CtkWidget *widget);
void         _ctk_widget_set_in_reparent    (CtkWidget *widget,
                                             gboolean   in_reparent);
gboolean     _ctk_widget_get_anchored       (CtkWidget *widget);
void         _ctk_widget_set_anchored       (CtkWidget *widget,
                                             gboolean   anchored);
gboolean     _ctk_widget_get_shadowed       (CtkWidget *widget);
void         _ctk_widget_set_shadowed       (CtkWidget *widget,
                                             gboolean   shadowed);
gboolean     _ctk_widget_get_alloc_needed   (CtkWidget *widget);
gboolean     ctk_widget_needs_allocate      (CtkWidget *widget);
void         ctk_widget_queue_resize_on_widget (CtkWidget *widget);
void         ctk_widget_ensure_resize       (CtkWidget *widget);
void         ctk_widget_ensure_allocate     (CtkWidget *widget);
void         ctk_widget_draw_internal       (CtkWidget *widget,
					     cairo_t   *cr,
                                             gboolean   do_clip);
void          _ctk_widget_scale_changed     (CtkWidget *widget);


void         _ctk_widget_add_sizegroup         (CtkWidget    *widget,
						gpointer      group);
void         _ctk_widget_remove_sizegroup      (CtkWidget    *widget,
						gpointer      group);
GSList      *_ctk_widget_get_sizegroups        (CtkWidget    *widget);

void         _ctk_widget_add_attached_window    (CtkWidget    *widget,
                                                 CtkWindow    *window);
void         _ctk_widget_remove_attached_window (CtkWidget    *widget,
                                                 CtkWindow    *window);

void _ctk_widget_get_preferred_size_for_size   (CtkWidget         *widget,
                                                CtkOrientation     orientation,
                                                gint               size,
                                                gint              *minimum,
                                                gint              *natural,
                                                gint              *minimum_baseline,
                                                gint              *natural_baseline);
void _ctk_widget_get_preferred_size_and_baseline(CtkWidget        *widget,
                                                CtkRequisition    *minimum_size,
                                                CtkRequisition    *natural_size,
                                                gint              *minimum_baseline,
                                                gint              *natural_baseline);
gboolean _ctk_widget_has_baseline_support (CtkWidget *widget);

const gchar*      _ctk_widget_get_accel_path               (CtkWidget *widget,
                                                            gboolean  *locked);

AtkObject *       _ctk_widget_peek_accessible              (CtkWidget *widget);

void              _ctk_widget_set_has_default              (CtkWidget *widget,
                                                            gboolean   has_default);
void              _ctk_widget_set_has_grab                 (CtkWidget *widget,
                                                            gboolean   has_grab);
void              _ctk_widget_set_is_toplevel              (CtkWidget *widget,
                                                            gboolean   is_toplevel);

void              _ctk_widget_grab_notify                  (CtkWidget *widget,
                                                            gboolean   was_grabbed);

void              _ctk_widget_propagate_hierarchy_changed  (CtkWidget *widget,
                                                            CtkWidget *previous_toplevel);
void              _ctk_widget_propagate_screen_changed     (CtkWidget *widget,
                                                            GdkScreen *previous_screen);
void              _ctk_widget_propagate_composited_changed (CtkWidget *widget);

void              _ctk_widget_set_device_window            (CtkWidget *widget,
                                                            GdkDevice *device,
                                                            GdkWindow *pointer_window);
GdkWindow *       _ctk_widget_get_device_window            (CtkWidget *widget,
                                                            GdkDevice *device);
GList *           _ctk_widget_list_devices                 (CtkWidget *widget);

void              _ctk_widget_synthesize_crossing          (CtkWidget       *from,
                                                            CtkWidget       *to,
                                                            GdkDevice       *device,
                                                            GdkCrossingMode  mode);

static inline gpointer _ctk_widget_peek_request_cache           (CtkWidget *widget);

void              _ctk_widget_buildable_finish_accelerator (CtkWidget *widget,
                                                            CtkWidget *toplevel,
                                                            gpointer   user_data);
CtkStyleContext * _ctk_widget_peek_style_context           (CtkWidget *widget);
CtkStyle *        _ctk_widget_get_style                    (CtkWidget *widget);
void              _ctk_widget_set_style                    (CtkWidget *widget,
                                                            CtkStyle  *style);
gboolean          _ctk_widget_supports_clip                (CtkWidget *widget);
void              _ctk_widget_set_simple_clip              (CtkWidget *widget,
                                                            CtkAllocation *content_clip);

typedef gboolean (*CtkCapturedEventHandler) (CtkWidget *widget, GdkEvent *event);

void              _ctk_widget_set_captured_event_handler (CtkWidget               *widget,
                                                          CtkCapturedEventHandler  handler);

gboolean          _ctk_widget_captured_event               (CtkWidget *widget,
                                                            GdkEvent  *event);

CtkWidgetPath *   _ctk_widget_create_path                  (CtkWidget    *widget);
void              ctk_widget_clear_path                    (CtkWidget    *widget);
void              _ctk_widget_invalidate_style_context     (CtkWidget    *widget,
                                                            CtkCssChange  change);
void              _ctk_widget_style_context_invalidated    (CtkWidget    *widget);

void              _ctk_widget_update_parent_muxer          (CtkWidget    *widget);
CtkActionMuxer *  _ctk_widget_get_action_muxer             (CtkWidget    *widget,
                                                            gboolean      create);

void              _ctk_widget_add_controller               (CtkWidget           *widget,
                                                            CtkEventController  *controller);
void              _ctk_widget_remove_controller            (CtkWidget           *widget,
                                                            CtkEventController  *controller);
GList *           _ctk_widget_list_controllers             (CtkWidget           *widget,
                                                            CtkPropagationPhase  phase);
gboolean          _ctk_widget_consumes_motion              (CtkWidget           *widget,
                                                            GdkEventSequence    *sequence);

gboolean          ctk_widget_has_tick_callback             (CtkWidget *widget);

void              ctk_widget_set_csd_input_shape           (CtkWidget            *widget,
                                                            const cairo_region_t *region);

gboolean          ctk_widget_has_size_request              (CtkWidget *widget);

void              ctk_widget_reset_controllers             (CtkWidget *widget);

gboolean          ctk_widget_query_tooltip                 (CtkWidget  *widget,
                                                            gint        x,
                                                            gint        y,
                                                            gboolean    keyboard_mode,
                                                            CtkTooltip *tooltip);

void              ctk_widget_render                        (CtkWidget            *widget,
                                                            GdkWindow            *window,
                                                            const cairo_region_t *region);


/* inline getters */

static inline gboolean
ctk_widget_get_resize_needed (CtkWidget *widget)
{
  return widget->priv->resize_needed;
}

static inline CtkWidget *
_ctk_widget_get_parent (CtkWidget *widget)
{
  return widget->priv->parent;
}

static inline gboolean
_ctk_widget_get_visible (CtkWidget *widget)
{
  return widget->priv->visible;
}

static inline gboolean
_ctk_widget_get_child_visible (CtkWidget *widget)
{
  return widget->priv->child_visible;
}

static inline gboolean
_ctk_widget_get_mapped (CtkWidget *widget)
{
  return widget->priv->mapped;
}

static inline gboolean
_ctk_widget_is_drawable (CtkWidget *widget)
{
  return widget->priv->visible && widget->priv->mapped;
}

static inline gboolean
_ctk_widget_get_has_window (CtkWidget *widget)
{
  return !widget->priv->no_window;
}

static inline gboolean
_ctk_widget_get_realized (CtkWidget *widget)
{
  return widget->priv->realized;
}

static inline gboolean
_ctk_widget_is_toplevel (CtkWidget *widget)
{
  return widget->priv->toplevel;
}

static inline CtkStateFlags
_ctk_widget_get_state_flags (CtkWidget *widget)
{
  return widget->priv->state_flags;
}

extern CtkTextDirection ctk_default_direction;

static inline CtkTextDirection
_ctk_widget_get_direction (CtkWidget *widget)
{
  if (widget->priv->direction == CTK_TEXT_DIR_NONE)
    return ctk_default_direction;
  else
    return widget->priv->direction;
}

static inline CtkWidget *
_ctk_widget_get_toplevel (CtkWidget *widget)
{
  while (widget->priv->parent)
    widget = widget->priv->parent;

  return widget;
}

static inline CtkStyleContext *
_ctk_widget_get_style_context (CtkWidget *widget)
{
  if (G_LIKELY (widget->priv->context))
    return widget->priv->context;

  return ctk_widget_get_style_context (widget);
}

static inline gpointer
_ctk_widget_peek_request_cache (CtkWidget *widget)
{
  return &widget->priv->requests;
}

static inline GdkWindow *
_ctk_widget_get_window (CtkWidget *widget)
{
  return widget->priv->window;
}

static inline void
_ctk_widget_get_allocation (CtkWidget     *widget,
                            CtkAllocation *allocation)
{
  *allocation = widget->priv->allocation;
}

G_END_DECLS

#endif /* __CTK_WIDGET_PRIVATE_H__ */
