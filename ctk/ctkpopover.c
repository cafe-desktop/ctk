/* CTK - The GIMP Toolkit
 * Copyright © 2013 Carlos Garnacho <carlosg@gnome.org>
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

/**
 * SECTION:ctkpopover
 * @Short_description: Context dependent bubbles
 * @Title: CtkPopover
 *
 * CtkPopover is a bubble-like context window, primarily meant to
 * provide context-dependent information or options. Popovers are
 * attached to a widget, passed at construction time on ctk_popover_new(),
 * or updated afterwards through ctk_popover_set_relative_to(), by
 * default they will point to the whole widget area, although this
 * behavior can be changed through ctk_popover_set_pointing_to().
 *
 * The position of a popover relative to the widget it is attached to
 * can also be changed through ctk_popover_set_position().
 *
 * By default, #CtkPopover performs a CTK+ grab, in order to ensure
 * input events get redirected to it while it is shown, and also so
 * the popover is dismissed in the expected situations (clicks outside
 * the popover, or the Esc key being pressed). If no such modal behavior
 * is desired on a popover, ctk_popover_set_modal() may be called on it
 * to tweak its behavior.
 *
 * ## CtkPopover as menu replacement
 *
 * CtkPopover is often used to replace menus. To facilitate this, it
 * supports being populated from a #GMenuModel, using
 * ctk_popover_new_from_model(). In addition to all the regular menu
 * model features, this function supports rendering sections in the
 * model in a more compact form, as a row of icon buttons instead of
 * menu items.
 *
 * To use this rendering, set the ”display-hint” attribute of the
 * section to ”horizontal-buttons” and set the icons of your items
 * with the ”verb-icon” attribute.
 *
 * |[
 * <section>
 *   <attribute name="display-hint">horizontal-buttons</attribute>
 *   <item>
 *     <attribute name="label">Cut</attribute>
 *     <attribute name="action">app.cut</attribute>
 *     <attribute name="verb-icon">edit-cut-symbolic</attribute>
 *   </item>
 *   <item>
 *     <attribute name="label">Copy</attribute>
 *     <attribute name="action">app.copy</attribute>
 *     <attribute name="verb-icon">edit-copy-symbolic</attribute>
 *   </item>
 *   <item>
 *     <attribute name="label">Paste</attribute>
 *     <attribute name="action">app.paste</attribute>
 *     <attribute name="verb-icon">edit-paste-symbolic</attribute>
 *   </item>
 * </section>
 * ]|
 *
 * # CSS nodes
 *
 * CtkPopover has a single css node called popover. It always gets the
 * .background style class and it gets the .menu style class if it is
 * menu-like (e.g. #CtkPopoverMenu or created using ctk_popover_new_from_model().
 *
 * Particular uses of CtkPopover, such as touch selection popups
 * or magnifiers in #CtkEntry or #CtkTextView get style classes
 * like .touch-selection or .magnifier to differentiate from
 * plain popovers.
 *
 * Since: 3.12
 */

#include "config.h"
#include <cdk/cdk.h>
#include "ctkpopover.h"
#include "ctkpopoverprivate.h"
#include "ctktypebuiltins.h"
#include "ctkmain.h"
#include "ctkwindowprivate.h"
#include "ctkscrollable.h"
#include "ctkadjustment.h"
#include "ctkprivate.h"
#include "ctkintl.h"
#include "ctklabel.h"
#include "ctkbox.h"
#include "ctkbutton.h"
#include "ctkseparator.h"
#include "ctkmodelbutton.h"
#include "ctkwidgetprivate.h"
#include "ctkactionmuxer.h"
#include "ctkmenutracker.h"
#include "ctkstack.h"
#include "ctksizegroup.h"
#include "a11y/ctkpopoveraccessible.h"
#include "ctkmenusectionbox.h"
#include "ctkroundedboxprivate.h"
#include "ctkstylecontextprivate.h"
#include "ctkprogresstrackerprivate.h"
#include "ctksettingsprivate.h"

#ifdef CDK_WINDOWING_WAYLAND
#include "wayland/cdkwayland.h"
#endif

#define TAIL_GAP_WIDTH  24
#define TAIL_HEIGHT     12
#define TRANSITION_DIFF 20
#define TRANSITION_DURATION 150 * 1000

#define POS_IS_VERTICAL(p) ((p) == CTK_POS_TOP || (p) == CTK_POS_BOTTOM)

enum {
  PROP_RELATIVE_TO = 1,
  PROP_POINTING_TO,
  PROP_POSITION,
  PROP_MODAL,
  PROP_TRANSITIONS_ENABLED,
  PROP_CONSTRAIN_TO,
  NUM_PROPERTIES
};

enum {
  CLOSED,
  N_SIGNALS
};

enum {
  STATE_SHOWING,
  STATE_SHOWN,
  STATE_HIDING,
  STATE_HIDDEN
};

struct _CtkPopoverPrivate
{
  CtkWidget *widget;
  CtkWindow *window;
  CtkWidget *prev_focus_widget;
  CtkWidget *default_widget;
  CtkWidget *prev_default;
  CtkScrollable *parent_scrollable;
  CtkAdjustment *vadj;
  CtkAdjustment *hadj;
  CdkRectangle pointing_to;
  CtkPopoverConstraint constraint;
  CtkProgressTracker tracker;
  CtkGesture *multipress_gesture;
  guint prev_focus_unmap_id;
  guint hierarchy_changed_id;
  guint size_allocate_id;
  guint unmap_id;
  guint scrollable_notify_id;
  guint grab_notify_id;
  guint state_changed_id;
  guint has_pointing_to    : 1;
  guint preferred_position : 2;
  guint final_position     : 2;
  guint current_position   : 2;
  guint modal              : 1;
  guint button_pressed     : 1;
  guint grab_notify_blocked : 1;
  guint transitions_enabled : 1;
  guint state               : 2;
  guint visible             : 1;
  guint first_frame_skipped : 1;
  gint transition_diff;
  guint tick_id;

  gint tip_x;
  gint tip_y;
};

static GParamSpec *properties[NUM_PROPERTIES];
static GQuark quark_widget_popovers = 0;
static guint signals[N_SIGNALS] = { 0 };

static void ctk_popover_update_relative_to (CtkPopover *popover,
                                            CtkWidget  *relative_to);
static void ctk_popover_set_state          (CtkPopover *popover,
                                            guint       state);
static void ctk_popover_invalidate_borders (CtkPopover *popover);
static void ctk_popover_apply_modality     (CtkPopover *popover,
                                            gboolean    modal);

static void ctk_popover_set_scrollable_full (CtkPopover    *popover,
                                             CtkScrollable *scrollable);

static void ctk_popover_multipress_gesture_pressed (CtkGestureMultiPress *gesture,
                                                    gint                  n_press,
                                                    gdouble               widget_x,
                                                    gdouble               widget_y,
                                                    CtkPopover            *popover);

G_DEFINE_TYPE_WITH_PRIVATE (CtkPopover, ctk_popover, CTK_TYPE_BIN)

static void
ctk_popover_init (CtkPopover *popover)
{
  CtkWidget *widget;
  CtkStyleContext *context;
  CtkPopoverPrivate *priv;

  widget = CTK_WIDGET (popover);
  ctk_widget_set_has_window (widget, TRUE);
  priv = popover->priv = ctk_popover_get_instance_private (popover);
  priv->modal = TRUE;
  priv->tick_id = 0;
  priv->state = STATE_HIDDEN;
  priv->visible = FALSE;
  priv->transitions_enabled = TRUE;
  priv->preferred_position = CTK_POS_TOP;
  priv->constraint = CTK_POPOVER_CONSTRAINT_WINDOW;

  priv->multipress_gesture = ctk_gesture_multi_press_new (widget);
  g_signal_connect (priv->multipress_gesture, "pressed",
                    G_CALLBACK (ctk_popover_multipress_gesture_pressed), popover);
  ctk_gesture_single_set_button (CTK_GESTURE_SINGLE (priv->multipress_gesture), 0);
  ctk_gesture_single_set_exclusive (CTK_GESTURE_SINGLE (priv->multipress_gesture), TRUE);
  ctk_event_controller_set_propagation_phase (CTK_EVENT_CONTROLLER (priv->multipress_gesture),
                                              CTK_PHASE_CAPTURE);

  context = ctk_widget_get_style_context (CTK_WIDGET (popover));
  ctk_style_context_add_class (context, CTK_STYLE_CLASS_BACKGROUND);
}

static void
ctk_popover_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  switch (prop_id)
    {
    case PROP_RELATIVE_TO:
      ctk_popover_set_relative_to (CTK_POPOVER (object),
                                   g_value_get_object (value));
      break;
    case PROP_POINTING_TO:
      ctk_popover_set_pointing_to (CTK_POPOVER (object),
                                   g_value_get_boxed (value));
      break;
    case PROP_POSITION:
      ctk_popover_set_position (CTK_POPOVER (object),
                                g_value_get_enum (value));
      break;
    case PROP_MODAL:
      ctk_popover_set_modal (CTK_POPOVER (object),
                             g_value_get_boolean (value));
      break;
    case PROP_TRANSITIONS_ENABLED:
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      ctk_popover_set_transitions_enabled (CTK_POPOVER (object),
                                           g_value_get_boolean (value));
      G_GNUC_END_IGNORE_DEPRECATIONS;
      break;
    case PROP_CONSTRAIN_TO:
      ctk_popover_set_constrain_to (CTK_POPOVER (object),
                                    g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ctk_popover_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  CtkPopoverPrivate *priv = CTK_POPOVER (object)->priv;

  switch (prop_id)
    {
    case PROP_RELATIVE_TO:
      g_value_set_object (value, priv->widget);
      break;
    case PROP_POINTING_TO:
      g_value_set_boxed (value, &priv->pointing_to);
      break;
    case PROP_POSITION:
      g_value_set_enum (value, priv->preferred_position);
      break;
    case PROP_MODAL:
      g_value_set_boolean (value, priv->modal);
      break;
    case PROP_TRANSITIONS_ENABLED:
      g_value_set_boolean (value, priv->transitions_enabled);
      break;
    case PROP_CONSTRAIN_TO:
      g_value_set_enum (value, priv->constraint);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static gboolean
transitions_enabled (CtkPopover *popover)
{
  CtkPopoverPrivate *priv = popover->priv;

  return ctk_settings_get_enable_animations (ctk_widget_get_settings (CTK_WIDGET (popover))) &&
         priv->transitions_enabled;
}

static void
ctk_popover_hide_internal (CtkPopover *popover)
{
  CtkPopoverPrivate *priv = ctk_popover_get_instance_private (popover);
  CtkWidget *widget = CTK_WIDGET (popover);

  if (!priv->visible)
    return;

  priv->visible = FALSE;
  g_signal_emit (widget, signals[CLOSED], 0);

  if (priv->modal)
    ctk_popover_apply_modality (popover, FALSE);

  if (ctk_widget_get_realized (widget))
    {
      cairo_region_t *region = cairo_region_create ();
      cdk_window_input_shape_combine_region (ctk_widget_get_parent_window (widget),
                                             region, 0, 0);
      cairo_region_destroy (region);
    }
}

static void
ctk_popover_finalize (GObject *object)
{
  CtkPopover *popover = CTK_POPOVER (object);
  CtkPopoverPrivate *priv = popover->priv;

  if (priv->widget)
    ctk_popover_update_relative_to (popover, NULL);

  g_clear_object (&priv->multipress_gesture);

  G_OBJECT_CLASS (ctk_popover_parent_class)->finalize (object);
}

static void
popover_unset_prev_focus (CtkPopover *popover)
{
  CtkPopoverPrivate *priv = popover->priv;

  if (!priv->prev_focus_widget)
    return;

  if (priv->prev_focus_unmap_id)
    {
      g_signal_handler_disconnect (priv->prev_focus_widget,
                                   priv->prev_focus_unmap_id);
      priv->prev_focus_unmap_id = 0;
    }

  g_clear_object (&priv->prev_focus_widget);
}

static void
ctk_popover_dispose (GObject *object)
{
  CtkPopover *popover = CTK_POPOVER (object);
  CtkPopoverPrivate *priv = popover->priv;

  if (priv->modal)
    ctk_popover_apply_modality (popover, FALSE);

  if (priv->window)
    {
      g_signal_handlers_disconnect_by_data (priv->window, popover);
      _ctk_window_remove_popover (priv->window, CTK_WIDGET (object));
    }

  priv->window = NULL;

  if (priv->widget)
    ctk_popover_update_relative_to (popover, NULL);

  popover_unset_prev_focus (popover);

  g_clear_object (&priv->default_widget);

  G_OBJECT_CLASS (ctk_popover_parent_class)->dispose (object);
}

static void
ctk_popover_realize (CtkWidget *widget)
{
  CtkAllocation allocation;
  CdkWindowAttr attributes;
  gint attributes_mask;
  CdkWindow *window;

  ctk_widget_get_allocation (widget, &allocation);

  attributes.x = 0;
  attributes.y = 0;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.window_type = CDK_WINDOW_CHILD;
  attributes.visual = ctk_widget_get_visual (widget);
  attributes.wclass = CDK_INPUT_OUTPUT;
  attributes.event_mask =
    ctk_widget_get_events (widget) |
    CDK_POINTER_MOTION_MASK |
    CDK_BUTTON_MOTION_MASK |
    CDK_BUTTON_PRESS_MASK |
    CDK_BUTTON_RELEASE_MASK |
    CDK_ENTER_NOTIFY_MASK |
    CDK_LEAVE_NOTIFY_MASK;

  attributes_mask = CDK_WA_X | CDK_WA_Y | CDK_WA_VISUAL;
  window = cdk_window_new (ctk_widget_get_parent_window (widget),
                           &attributes, attributes_mask);
  ctk_widget_set_window (widget, window);
  ctk_widget_register_window (widget, window);
  ctk_widget_set_realized (widget, TRUE);
}

static gboolean
window_focus_in (CtkWidget  *widget,
                 CdkEvent   *event,
                 CtkPopover *popover)
{
  CtkPopoverPrivate *priv = ctk_popover_get_instance_private (popover);

  /* Regain the grab when the window is focused */
  if (priv->modal &&
      ctk_widget_is_drawable (CTK_WIDGET (popover)))
    {
      CtkWidget *focus;

      ctk_grab_add (CTK_WIDGET (popover));

      focus = ctk_window_get_focus (CTK_WINDOW (widget));

      if (focus == NULL || !ctk_widget_is_ancestor (focus, CTK_WIDGET (popover)))
        ctk_widget_grab_focus (CTK_WIDGET (popover));

      if (priv->grab_notify_blocked)
        g_signal_handler_unblock (priv->widget, priv->grab_notify_id);

      priv->grab_notify_blocked = FALSE;
    }
  return FALSE;
}

static gboolean
window_focus_out (CtkWidget  *widget,
                  CdkEvent   *event,
                  CtkPopover *popover)
{
  CtkPopoverPrivate *priv = ctk_popover_get_instance_private (popover);

  /* Temporarily remove the grab when unfocused */
  if (priv->modal &&
      ctk_widget_is_drawable (CTK_WIDGET (popover)))
    {
      g_signal_handler_block (priv->widget, priv->grab_notify_id);
      ctk_grab_remove (CTK_WIDGET (popover));
      priv->grab_notify_blocked = TRUE;
    }
  return FALSE;
}

static void
window_set_focus (CtkWindow  *window,
                  CtkWidget  *widget,
                  CtkPopover *popover)
{
  CtkPopoverPrivate *priv = ctk_popover_get_instance_private (popover);

  if (!priv->modal || !widget || !ctk_widget_is_drawable (CTK_WIDGET (popover)))
    return;

  widget = ctk_widget_get_ancestor (widget, CTK_TYPE_POPOVER);
  while (widget != NULL)
    {
      if (widget == CTK_WIDGET (popover))
        return;

      widget = ctk_popover_get_relative_to (CTK_POPOVER (widget));
      if (widget == NULL)
        break;
      widget = ctk_widget_get_ancestor (widget, CTK_TYPE_POPOVER);
    }

  popover_unset_prev_focus (popover);
  ctk_widget_hide (CTK_WIDGET (popover));
}

static void
prev_focus_unmap_cb (CtkWidget  *widget,
                     CtkPopover *popover)
{
  popover_unset_prev_focus (popover);
}

static void
ctk_popover_apply_modality (CtkPopover *popover,
                            gboolean    modal)
{
  CtkPopoverPrivate *priv = popover->priv;

  if (!priv->window)
    return;

  if (modal)
    {
      CtkWidget *prev_focus;

      prev_focus = ctk_window_get_focus (priv->window);
      priv->prev_focus_widget = prev_focus;
      if (priv->prev_focus_widget)
        {
          priv->prev_focus_unmap_id =
            g_signal_connect (prev_focus, "unmap",
                              G_CALLBACK (prev_focus_unmap_cb), popover);
          g_object_ref (prev_focus);
        }
      ctk_grab_add (CTK_WIDGET (popover));
      ctk_window_set_focus (priv->window, NULL);
      ctk_widget_grab_focus (CTK_WIDGET (popover));

      g_signal_connect (priv->window, "focus-in-event",
                        G_CALLBACK (window_focus_in), popover);
      g_signal_connect (priv->window, "focus-out-event",
                        G_CALLBACK (window_focus_out), popover);
      g_signal_connect (priv->window, "set-focus",
                        G_CALLBACK (window_set_focus), popover);
    }
  else
    {
      g_signal_handlers_disconnect_by_data (priv->window, popover);
      ctk_grab_remove (CTK_WIDGET (popover));

      /* Let prev_focus_widget regain focus */
      if (priv->prev_focus_widget &&
          ctk_widget_is_drawable (priv->prev_focus_widget))
        {
           if (CTK_IS_ENTRY (priv->prev_focus_widget))
             ctk_entry_grab_focus_without_selecting (CTK_ENTRY (priv->prev_focus_widget));
           else
             ctk_widget_grab_focus (priv->prev_focus_widget);
        }
      else if (priv->window)
        ctk_widget_grab_focus (CTK_WIDGET (priv->window));

      popover_unset_prev_focus (popover);
    }
}

static gboolean
show_animate_cb (CtkWidget     *widget,
                 CdkFrameClock *frame_clock,
                 gpointer       user_data)
{
  CtkPopover *popover = CTK_POPOVER (widget);
  CtkPopoverPrivate *priv = ctk_popover_get_instance_private (popover);
  gdouble t;

  if (priv->first_frame_skipped)
    ctk_progress_tracker_advance_frame (&priv->tracker,
                                        cdk_frame_clock_get_frame_time (frame_clock));
  else
    priv->first_frame_skipped = TRUE;

  t = ctk_progress_tracker_get_ease_out_cubic (&priv->tracker, FALSE);

  if (priv->state == STATE_SHOWING)
    {
      priv->transition_diff = TRANSITION_DIFF - (TRANSITION_DIFF * t);
      ctk_widget_set_opacity (widget, t);
    }
  else if (priv->state == STATE_HIDING)
    {
      priv->transition_diff = -TRANSITION_DIFF * t;
      ctk_widget_set_opacity (widget, 1.0 - t);
    }

  ctk_popover_update_position (popover);

  if (ctk_progress_tracker_get_state (&priv->tracker) == CTK_PROGRESS_STATE_AFTER)
    {
      if (priv->state == STATE_SHOWING)
        {
          ctk_popover_set_state (popover, STATE_SHOWN);

          if (!priv->visible)
            ctk_popover_set_state (popover, STATE_HIDING);
        }
      else
        {
          ctk_widget_hide (widget);
        }

      priv->tick_id = 0;
      return G_SOURCE_REMOVE;
    }
  else
    return G_SOURCE_CONTINUE;
}

static void
ctk_popover_stop_transition (CtkPopover *popover)
{
  CtkPopoverPrivate *priv = popover->priv;

  if (priv->tick_id != 0)
    {
      ctk_widget_remove_tick_callback (CTK_WIDGET (popover), priv->tick_id);
      priv->tick_id = 0;
    }
}

static void
ctk_popover_start_transition (CtkPopover *popover)
{
  CtkPopoverPrivate *priv = popover->priv;

  if (priv->tick_id != 0)
    return;

  priv->first_frame_skipped = FALSE;
  ctk_progress_tracker_start (&priv->tracker, TRANSITION_DURATION, 0, 1.0);
  priv->tick_id = ctk_widget_add_tick_callback (CTK_WIDGET (popover),
                                                show_animate_cb,
                                                popover, NULL);
}

static void
ctk_popover_set_state (CtkPopover *popover,
                       guint       state)
{
  CtkPopoverPrivate *priv = popover->priv;

  if (!transitions_enabled (popover) ||
      !ctk_widget_get_realized (CTK_WIDGET (popover)))
    {
      if (state == STATE_SHOWING)
        state = STATE_SHOWN;
      else if (state == STATE_HIDING)
        state = STATE_HIDDEN;
    }

  priv->state = state;

  if (state == STATE_SHOWING || state == STATE_HIDING)
    ctk_popover_start_transition (popover);
  else
    {
      ctk_popover_stop_transition (popover);

      ctk_widget_set_visible (CTK_WIDGET (popover), state == STATE_SHOWN);
    }
}

CtkWidget *
ctk_popover_get_prev_default (CtkPopover *popover)
{
  g_return_val_if_fail (CTK_IS_POPOVER (popover), NULL);

  return popover->priv->prev_default;
}


static void
ctk_popover_map (CtkWidget *widget)
{
  CtkPopoverPrivate *priv = CTK_POPOVER (widget)->priv;

  priv->prev_default = ctk_window_get_default_widget (priv->window);
  if (priv->prev_default)
    g_object_ref (priv->prev_default);

  CTK_WIDGET_CLASS (ctk_popover_parent_class)->map (widget);

  cdk_window_show (ctk_widget_get_window (widget));
  ctk_popover_update_position (CTK_POPOVER (widget));

  ctk_window_set_default (priv->window, priv->default_widget);
}

static void
ctk_popover_unmap (CtkWidget *widget)
{
  CtkPopoverPrivate *priv = CTK_POPOVER (widget)->priv;

  priv->button_pressed = FALSE;

  cdk_window_hide (ctk_widget_get_window (widget));
  CTK_WIDGET_CLASS (ctk_popover_parent_class)->unmap (widget);

  if (ctk_window_get_default_widget (priv->window) == priv->default_widget)
    ctk_window_set_default (priv->window, priv->prev_default);
  g_clear_object (&priv->prev_default);
}

static CtkPositionType
get_effective_position (CtkPopover      *popover,
                        CtkPositionType  pos)
{
  if (ctk_widget_get_direction (CTK_WIDGET (popover)) == CTK_TEXT_DIR_RTL)
    {
      if (pos == CTK_POS_LEFT)
        pos = CTK_POS_RIGHT;
      else if (pos == CTK_POS_RIGHT)
        pos = CTK_POS_LEFT;
    }

  return pos;
}

static void
get_margin (CtkWidget *widget,
            CtkBorder *border)
{
  CtkStyleContext *context;

  context = ctk_widget_get_style_context (widget);
  ctk_style_context_get_margin (context,
                                ctk_style_context_get_state (context),
                                border);
}

static void
ctk_popover_get_gap_coords (CtkPopover      *popover,
                            gint            *initial_x_out,
                            gint            *initial_y_out,
                            gint            *tip_x_out,
                            gint            *tip_y_out,
                            gint            *final_x_out,
                            gint            *final_y_out,
                            CtkPositionType *gap_side_out)
{
  CtkWidget *widget = CTK_WIDGET (popover);
  CtkPopoverPrivate *priv = popover->priv;
  CdkRectangle rect;
  gint base, tip, tip_pos;
  gint initial_x, initial_y;
  gint tip_x, tip_y;
  gint final_x, final_y;
  CtkPositionType gap_side, pos;
  CtkAllocation allocation;
  gint border_radius;
  CtkStyleContext *context;
  CtkBorder margin, border, widget_margin;
  CtkStateFlags state;

  ctk_popover_get_pointing_to (popover, &rect);
  ctk_widget_get_allocation (widget, &allocation);

#ifdef CDK_WINDOWING_WAYLAND
  if (CDK_IS_WAYLAND_DISPLAY (ctk_widget_get_display (widget)))
    {
      gint win_x, win_y;

      ctk_widget_translate_coordinates (priv->widget, CTK_WIDGET (priv->window),
                                        rect.x, rect.y, &rect.x, &rect.y);
      cdk_window_get_origin (ctk_widget_get_window (CTK_WIDGET (popover)),
                             &win_x, &win_y);
      rect.x -= win_x;
      rect.y -= win_y;
    }
  else
#endif
    ctk_widget_translate_coordinates (priv->widget, widget,
                                      rect.x, rect.y, &rect.x, &rect.y);

  get_margin (widget, &margin);

  if (ctk_widget_get_direction (widget) == CTK_TEXT_DIR_LTR)
    {
      widget_margin.left = ctk_widget_get_margin_start (widget);
      widget_margin.right = ctk_widget_get_margin_end (widget);
    }
  else
    {
      widget_margin.left = ctk_widget_get_margin_end (widget);
      widget_margin.right = ctk_widget_get_margin_start (widget);
    }

  widget_margin.top = ctk_widget_get_margin_top (widget);
  widget_margin.bottom = ctk_widget_get_margin_bottom (widget);

  context = ctk_widget_get_style_context (widget);
  state = ctk_style_context_get_state (context);

  ctk_style_context_get_border (context, state, &border);
  ctk_style_context_get (context,
                         state,
                         CTK_STYLE_PROPERTY_BORDER_RADIUS, &border_radius,
                         NULL);
  pos = get_effective_position (popover, priv->final_position);

  if (pos == CTK_POS_BOTTOM || pos == CTK_POS_RIGHT)
    {
      tip = ((pos == CTK_POS_BOTTOM) ? border.top + widget_margin.top : border.left + widget_margin.left);
      base = tip + TAIL_HEIGHT;
      gap_side = (priv->final_position == CTK_POS_BOTTOM) ? CTK_POS_TOP : CTK_POS_LEFT;
    }
  else if (pos == CTK_POS_TOP)
    {
      base = allocation.height - TAIL_HEIGHT - border.bottom - widget_margin.bottom;
      tip = base + TAIL_HEIGHT;
      gap_side = CTK_POS_BOTTOM;
    }
  else if (pos == CTK_POS_LEFT)
    {
      base = allocation.width - TAIL_HEIGHT - border.right - widget_margin.right;
      tip = base + TAIL_HEIGHT;
      gap_side = CTK_POS_RIGHT;
    }
  else
    g_assert_not_reached ();

  if (POS_IS_VERTICAL (pos))
    {
      tip_pos = rect.x + (rect.width / 2) + widget_margin.left;
      initial_x = CLAMP (tip_pos - TAIL_GAP_WIDTH / 2,
                         border_radius + margin.left + TAIL_HEIGHT,
                         allocation.width - TAIL_GAP_WIDTH - margin.right - border_radius - TAIL_HEIGHT);
      initial_y = base;

      tip_x = CLAMP (tip_pos, 0, allocation.width);
      tip_y = tip;

      final_x = CLAMP (tip_pos + TAIL_GAP_WIDTH / 2,
                       border_radius + margin.left + TAIL_GAP_WIDTH + TAIL_HEIGHT,
                       allocation.width - margin.right - border_radius - TAIL_HEIGHT);
      final_y = base;
    }
  else
    {
      tip_pos = rect.y + (rect.height / 2) + widget_margin.top;

      initial_x = base;
      initial_y = CLAMP (tip_pos - TAIL_GAP_WIDTH / 2,
                         border_radius + margin.top + TAIL_HEIGHT,
                         allocation.height - TAIL_GAP_WIDTH - margin.bottom - border_radius - TAIL_HEIGHT);

      tip_x = tip;
      tip_y = CLAMP (tip_pos, 0, allocation.height);

      final_x = base;
      final_y = CLAMP (tip_pos + TAIL_GAP_WIDTH / 2,
                       border_radius + margin.top + TAIL_GAP_WIDTH + TAIL_HEIGHT,
                       allocation.height - margin.right - border_radius - TAIL_HEIGHT);
    }

  if (initial_x_out)
    *initial_x_out = initial_x;
  if (initial_y_out)
    *initial_y_out = initial_y;

  if (tip_x_out)
    *tip_x_out = tip_x;
  if (tip_y_out)
    *tip_y_out = tip_y;

  if (final_x_out)
    *final_x_out = final_x;
  if (final_y_out)
    *final_y_out = final_y;

  if (gap_side_out)
    *gap_side_out = gap_side;
}

static void
ctk_popover_get_rect_for_size (CtkPopover   *popover,
                               int           popover_width,
                               int           popover_height,
                               CdkRectangle *rect)
{
  CtkWidget *widget = CTK_WIDGET (popover);
  int x, y, w, h;
  CtkBorder margin;

  get_margin (widget, &margin);

  x = 0;
  y = 0;
  w = popover_width;
  h = popover_height;

  x += MAX (TAIL_HEIGHT, margin.left);
  y += MAX (TAIL_HEIGHT, margin.top);
  w -= x + MAX (TAIL_HEIGHT, margin.right);
  h -= y + MAX (TAIL_HEIGHT, margin.bottom);

  rect->x = x;
  rect->y = y;
  rect->width = w;
  rect->height = h;
}

static void
ctk_popover_get_rect_coords (CtkPopover *popover,
                             int        *x_out,
                             int        *y_out,
                             int        *w_out,
                             int        *h_out)
{
  CtkWidget *widget = CTK_WIDGET (popover);
  CdkRectangle rect;
  CtkAllocation allocation;

  ctk_widget_get_allocation (widget, &allocation);
  ctk_popover_get_rect_for_size (popover, allocation.width, allocation.height, &rect);

  *x_out = rect.x;
  *y_out = rect.y;
  *w_out = rect.width;
  *h_out = rect.height;
}

static void
ctk_popover_apply_tail_path (CtkPopover *popover,
                             cairo_t    *cr)
{
  gint initial_x, initial_y;
  gint tip_x, tip_y;
  gint final_x, final_y;

  if (!popover->priv->widget)
    return;

  cairo_set_line_width (cr, 1);
  ctk_popover_get_gap_coords (popover,
                              &initial_x, &initial_y,
                              &tip_x, &tip_y,
                              &final_x, &final_y,
                              NULL);

  cairo_move_to (cr, initial_x, initial_y);
  cairo_line_to (cr, tip_x, tip_y);
  cairo_line_to (cr, final_x, final_y);
}

static void
ctk_popover_fill_border_path (CtkPopover *popover,
                              cairo_t    *cr)
{
  CtkWidget *widget = CTK_WIDGET (popover);
  CtkAllocation allocation;
  CtkStyleContext *context;
  int x, y, w, h;
  CtkRoundedBox box;

  context = ctk_widget_get_style_context (widget);
  ctk_widget_get_allocation (widget, &allocation);

  cairo_set_source_rgba (cr, 0, 0, 0, 1);

  ctk_popover_apply_tail_path (popover, cr);
  cairo_close_path (cr);
  cairo_fill (cr);

  ctk_popover_get_rect_coords (popover, &x, &y, &w, &h);

  _ctk_rounded_box_init_rect (&box, x, y, w, h);
  _ctk_rounded_box_apply_border_radius_for_style (&box,
                                                  ctk_style_context_lookup_style (context),
                                                  0);
  _ctk_rounded_box_path (&box, cr);
  cairo_fill (cr);
}

static void
ctk_popover_update_shape (CtkPopover *popover)
{
  CtkWidget *widget = CTK_WIDGET (popover);
  cairo_surface_t *surface;
  cairo_region_t *region;
  CdkWindow *win;
  cairo_t *cr;

#ifdef CDK_WINDOWING_WAYLAND
  if (CDK_IS_WAYLAND_DISPLAY (ctk_widget_get_display (widget)))
    return;
#endif

  win = ctk_widget_get_window (widget);
  surface =
    cdk_window_create_similar_surface (win,
                                       CAIRO_CONTENT_COLOR_ALPHA,
                                       cdk_window_get_width (win),
                                       cdk_window_get_height (win));

  cr = cairo_create (surface);
  ctk_popover_fill_border_path (popover, cr);
  cairo_destroy (cr);

  region = cdk_cairo_region_create_from_surface (surface);
  cairo_surface_destroy (surface);

  ctk_widget_shape_combine_region (widget, region);
  cairo_region_destroy (region);

  cdk_window_set_child_shapes (ctk_widget_get_parent_window (widget));
}

static void
_ctk_popover_update_child_visible (CtkPopover *popover)
{
  CtkPopoverPrivate *priv = popover->priv;
  CtkWidget *widget = CTK_WIDGET (popover);
  CdkRectangle rect;
  CtkAllocation allocation;
  CtkWidget *parent;

  if (!priv->parent_scrollable)
    {
      ctk_widget_set_child_visible (widget, TRUE);
      return;
    }

  parent = ctk_widget_get_parent (CTK_WIDGET (priv->parent_scrollable));
  ctk_popover_get_pointing_to (popover, &rect);

  ctk_widget_translate_coordinates (priv->widget, parent,
                                    rect.x, rect.y, &rect.x, &rect.y);

  ctk_widget_get_allocation (CTK_WIDGET (parent), &allocation);

  if (rect.x + rect.width < 0 || rect.x > allocation.width ||
      rect.y + rect.height < 0 || rect.y > allocation.height)
    ctk_widget_set_child_visible (widget, FALSE);
  else
    ctk_widget_set_child_visible (widget, TRUE);
}

static CtkPositionType
opposite_position (CtkPositionType pos)
{
  switch (pos)
    {
    default:
    case CTK_POS_LEFT: return CTK_POS_RIGHT;
    case CTK_POS_RIGHT: return CTK_POS_LEFT;
    case CTK_POS_TOP: return CTK_POS_BOTTOM;
    case CTK_POS_BOTTOM: return CTK_POS_TOP;
    }
}

void
ctk_popover_update_position (CtkPopover *popover)
{
  CtkPopoverPrivate *priv = popover->priv;
  CtkWidget *widget = CTK_WIDGET (popover);
  CtkAllocation window_alloc;
  CtkBorder window_shadow;
  CdkRectangle rect;
  CtkRequisition req;
  CtkPositionType pos;
  gint overshoot[4];
  gint i, j;
  gint best;

  if (!priv->window)
    return;

  ctk_widget_get_preferred_size (widget, NULL, &req);
  ctk_widget_get_allocation (CTK_WIDGET (priv->window), &window_alloc);
  _ctk_window_get_shadow_width (priv->window, &window_shadow);
  priv->final_position = priv->preferred_position;

  ctk_popover_get_pointing_to (popover, &rect);
  ctk_widget_translate_coordinates (priv->widget, CTK_WIDGET (priv->window),
                                    rect.x, rect.y, &rect.x, &rect.y);

  pos = get_effective_position (popover, priv->preferred_position);

  overshoot[CTK_POS_TOP] = req.height - rect.y + window_shadow.top;
  overshoot[CTK_POS_BOTTOM] = rect.y + rect.height + req.height - window_alloc.height
                              + window_shadow.bottom;
  overshoot[CTK_POS_LEFT] = req.width - rect.x + window_shadow.left;
  overshoot[CTK_POS_RIGHT] = rect.x + rect.width + req.width - window_alloc.width
                             + window_shadow.right;

#ifdef CDK_WINDOWING_WAYLAND
  if (CDK_IS_WAYLAND_DISPLAY (ctk_widget_get_display (widget)) &&
      priv->constraint == CTK_POPOVER_CONSTRAINT_NONE)
    {
      priv->final_position = priv->preferred_position;
    }
  else
#endif
  if (overshoot[pos] <= 0)
    {
      priv->final_position = priv->preferred_position;
    }
  else if (overshoot[opposite_position (pos)] <= 0)
    {
      priv->final_position = opposite_position (priv->preferred_position);
    }
  else
    {
      best = G_MAXINT;
      pos = 0;
      for (i = 0; i < 4; i++)
        {
          j = get_effective_position (popover, i);
          if (overshoot[j] < best)
            {
              pos = i;
              best = overshoot[j];
            }
        }
      priv->final_position = pos;
    }

  switch (priv->final_position)
    {
    case CTK_POS_TOP:
      rect.y += priv->transition_diff;
      break;
    case CTK_POS_BOTTOM:
      rect.y -= priv->transition_diff;
      break;
    case CTK_POS_LEFT:
      rect.x += priv->transition_diff;
      break;
    case CTK_POS_RIGHT:
      rect.x -= priv->transition_diff;
      break;
    }

  _ctk_window_set_popover_position (priv->window, widget,
                                    priv->final_position, &rect);

  if (priv->final_position != priv->current_position)
    {
      if (ctk_widget_is_drawable (widget))
        ctk_popover_update_shape (popover);

      priv->current_position = priv->final_position;
      ctk_popover_invalidate_borders (popover);
    }

  _ctk_popover_update_child_visible (popover);
}

static gboolean
ctk_popover_draw (CtkWidget *widget,
                  cairo_t   *cr)
{
  CtkPopover *popover = CTK_POPOVER (widget);
  CtkStyleContext *context;
  CtkAllocation allocation;
  CtkWidget *child;
  CtkBorder border;
  CdkRGBA border_color;
  int rect_x, rect_y, rect_w, rect_h;
  gint initial_x, initial_y, final_x, final_y;
  gint gap_start, gap_end;
  CtkPositionType gap_side;
  CtkStateFlags state;

  context = ctk_widget_get_style_context (widget);

  state = ctk_style_context_get_state (context);
  ctk_widget_get_allocation (widget, &allocation);

  ctk_style_context_get_border (context, state, &border);
  ctk_popover_get_rect_coords (popover,
                               &rect_x, &rect_y,
                               &rect_w, &rect_h);

  /* Render the rect background */
  ctk_render_background (context, cr,
                         rect_x, rect_y,
                         rect_w, rect_h);

  if (popover->priv->widget)
    {
      ctk_popover_get_gap_coords (popover,
                                  &initial_x, &initial_y,
                                  NULL, NULL,
                                  &final_x, &final_y,
                                  &gap_side);

      if (POS_IS_VERTICAL (gap_side))
        {
          gap_start = initial_x - rect_x;
          gap_end = final_x - rect_x;
        }
      else
        {
          gap_start = initial_y - rect_y;
          gap_end = final_y - rect_y;
        }

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      /* Now render the frame, without the gap for the arrow tip */
      ctk_render_frame_gap (context, cr,
                            rect_x, rect_y,
                            rect_w, rect_h,
                            gap_side,
                            gap_start, gap_end);
G_GNUC_END_IGNORE_DEPRECATIONS
    }
  else
    {
      ctk_render_frame (context, cr,
                        rect_x, rect_y,
                        rect_w, rect_h);
    }

  /* Clip to the arrow shape */
  cairo_save (cr);

  ctk_popover_apply_tail_path (popover, cr);
  cairo_clip (cr);

  /* Render the arrow background */
  ctk_render_background (context, cr,
                         0, 0,
                         allocation.width, allocation.height);

  /* Render the border of the arrow tip */
  if (border.bottom > 0)
    {
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      ctk_style_context_get_border_color (context, state, &border_color);
G_GNUC_END_IGNORE_DEPRECATIONS

      ctk_popover_apply_tail_path (popover, cr);
      cdk_cairo_set_source_rgba (cr, &border_color);

      cairo_set_line_width (cr, border.bottom + 1);
      cairo_stroke (cr);
    }

  /* We're done */
  cairo_restore (cr);

  child = ctk_bin_get_child (CTK_BIN (widget));

  if (child)
    ctk_container_propagate_draw (CTK_CONTAINER (widget), child, cr);

  return CDK_EVENT_PROPAGATE;
}

static void
get_padding_and_border (CtkWidget *widget,
                        CtkBorder *border)
{
  CtkStyleContext *context;
  CtkStateFlags state;
  gint border_width;
  CtkBorder tmp;

  context = ctk_widget_get_style_context (widget);
  state = ctk_style_context_get_state (context);

  border_width = ctk_container_get_border_width (CTK_CONTAINER (widget));

  ctk_style_context_get_padding (context, state, border);
  ctk_style_context_get_border (context, state, &tmp);
  border->top += tmp.top + border_width;
  border->right += tmp.right + border_width;
  border->bottom += tmp.bottom + border_width;
  border->left += tmp.left + border_width;
}

static gint
get_border_radius (CtkWidget *widget)
{
  CtkStyleContext *context;
  CtkStateFlags state;
  gint border_radius;

  context = ctk_widget_get_style_context (widget);
  state = ctk_style_context_get_state (context);
  ctk_style_context_get (context, state,
                         CTK_STYLE_PROPERTY_BORDER_RADIUS, &border_radius,
                         NULL);
  return border_radius;
}

static gint
get_minimal_size (CtkPopover     *popover,
                  CtkOrientation  orientation)
{
  CtkPopoverPrivate *priv = popover->priv;
  CtkPositionType pos;
  gint minimal_size;

  minimal_size = 2 * get_border_radius (CTK_WIDGET (popover));
  pos = get_effective_position (popover, priv->preferred_position);

  if ((orientation == CTK_ORIENTATION_HORIZONTAL && POS_IS_VERTICAL (pos)) ||
      (orientation == CTK_ORIENTATION_VERTICAL && !POS_IS_VERTICAL (pos)))
    minimal_size += TAIL_GAP_WIDTH;

  return minimal_size;
}

static void
ctk_popover_get_preferred_width (CtkWidget *widget,
                                 gint      *minimum_width,
                                 gint      *natural_width)
{
  CtkPopover *popover = CTK_POPOVER (widget);
  CtkWidget *child;
  gint min, nat, extra, minimal_size;
  CtkBorder border, margin;

  child = ctk_bin_get_child (CTK_BIN (widget));
  min = nat = 0;

  if (child)
    ctk_widget_get_preferred_width (child, &min, &nat);

  get_padding_and_border (widget, &border);
  get_margin (widget, &margin);
  minimal_size = get_minimal_size (popover, CTK_ORIENTATION_HORIZONTAL);

  min = MAX (min, minimal_size) + border.left + border.right;
  nat = MAX (nat, minimal_size) + border.left + border.right;
  extra = MAX (TAIL_HEIGHT, margin.left) + MAX (TAIL_HEIGHT, margin.right);

  min += extra;
  nat += extra;

  *minimum_width = min;
  *natural_width = nat;
}

static void
ctk_popover_get_preferred_width_for_height (CtkWidget *widget,
                                            gint       height,
                                            gint      *minimum_width,
                                            gint      *natural_width)
{
  CtkPopover *popover = CTK_POPOVER (widget);
  CtkWidget *child;
  CdkRectangle child_rect;
  gint min, nat, extra, minimal_size;
  gint child_height;
  CtkBorder border, margin;

  child = ctk_bin_get_child (CTK_BIN (widget));
  min = nat = 0;

  ctk_popover_get_rect_for_size (popover, 0, height, &child_rect);
  child_height = child_rect.height;


  get_padding_and_border (widget, &border);
  get_margin (widget, &margin);
  child_height -= border.top + border.bottom;
  minimal_size = get_minimal_size (popover, CTK_ORIENTATION_HORIZONTAL);

  if (child)
    ctk_widget_get_preferred_width_for_height (child, child_height, &min, &nat);

  min = MAX (min, minimal_size) + border.left + border.right;
  nat = MAX (nat, minimal_size) + border.left + border.right;
  extra = MAX (TAIL_HEIGHT, margin.left) + MAX (TAIL_HEIGHT, margin.right);

  min += extra;
  nat += extra;

  *minimum_width = min;
  *natural_width = nat;
}

static void
ctk_popover_get_preferred_height (CtkWidget *widget,
                                  gint      *minimum_height,
                                  gint      *natural_height)
{
  CtkPopover *popover = CTK_POPOVER (widget);
  CtkWidget *child;
  gint min, nat, extra, minimal_size;
  CtkBorder border, margin;

  child = ctk_bin_get_child (CTK_BIN (widget));
  min = nat = 0;

  if (child)
    ctk_widget_get_preferred_height (child, &min, &nat);

  get_padding_and_border (widget, &border);
  get_margin (widget, &margin);
  minimal_size = get_minimal_size (popover, CTK_ORIENTATION_VERTICAL);

  min = MAX (min, minimal_size) + border.top + border.bottom;
  nat = MAX (nat, minimal_size) + border.top + border.bottom;
  extra = MAX (TAIL_HEIGHT, margin.top) + MAX (TAIL_HEIGHT, margin.bottom);

  min += extra;
  nat += extra;

  *minimum_height = min;
  *natural_height = nat;
}

static void
ctk_popover_get_preferred_height_for_width (CtkWidget *widget,
                                            gint       width,
                                            gint      *minimum_height,
                                            gint      *natural_height)
{
  CtkPopover *popover = CTK_POPOVER (widget);
  CtkWidget *child;
  CdkRectangle child_rect;
  gint min, nat, extra, minimal_size;
  gint child_width;
  CtkBorder border, margin;

  child = ctk_bin_get_child (CTK_BIN (widget));
  min = nat = 0;

  get_padding_and_border (widget, &border);
  get_margin (widget, &margin);

  ctk_popover_get_rect_for_size (popover, width, 0, &child_rect);
  child_width = child_rect.width;

  child_width -= border.left + border.right;
  minimal_size = get_minimal_size (popover, CTK_ORIENTATION_VERTICAL);
  if (child)
    ctk_widget_get_preferred_height_for_width (child, child_width, &min, &nat);

  min = MAX (min, minimal_size) + border.top + border.bottom;
  nat = MAX (nat, minimal_size) + border.top + border.bottom;
  extra = MAX (TAIL_HEIGHT, margin.top) + MAX (TAIL_HEIGHT, margin.bottom);

  min += extra;
  nat += extra;

  if (minimum_height)
    *minimum_height = min;

  if (natural_height)
    *natural_height = nat;
}

static void
ctk_popover_invalidate_borders (CtkPopover *popover)
{
  CtkAllocation allocation;
  CtkBorder border;

  ctk_widget_get_allocation (CTK_WIDGET (popover), &allocation);
  get_padding_and_border (CTK_WIDGET (popover), &border);

  ctk_widget_queue_draw_area (CTK_WIDGET (popover), 0, 0, border.left + TAIL_HEIGHT, allocation.height);
  ctk_widget_queue_draw_area (CTK_WIDGET (popover), 0, 0, allocation.width, border.top + TAIL_HEIGHT);
  ctk_widget_queue_draw_area (CTK_WIDGET (popover), 0, allocation.height - border.bottom - TAIL_HEIGHT,
                              allocation.width, border.bottom + TAIL_HEIGHT);
  ctk_widget_queue_draw_area (CTK_WIDGET (popover), allocation.width - border.right - TAIL_HEIGHT,
                              0, border.right + TAIL_HEIGHT, allocation.height);
}

static void
ctk_popover_check_invalidate_borders (CtkPopover *popover)
{
  CtkPopoverPrivate *priv = popover->priv;
  CtkPositionType gap_side;
  gint tip_x, tip_y;

  if (!priv->widget)
    return;

  ctk_popover_get_gap_coords (popover, NULL, NULL,
                              &tip_x, &tip_y, NULL, NULL,
                              &gap_side);

  if (tip_x != priv->tip_x || tip_y != priv->tip_y)
    {
      priv->tip_x = tip_x;
      priv->tip_y = tip_y;
      ctk_popover_invalidate_borders (popover);
    }
}

static void
ctk_popover_size_allocate (CtkWidget     *widget,
                           CtkAllocation *allocation)
{
  CtkPopover *popover = CTK_POPOVER (widget);
  CtkWidget *child;

  ctk_widget_set_allocation (widget, allocation);
  child = ctk_bin_get_child (CTK_BIN (widget));
  if (child)
    {
      CtkAllocation child_alloc;
      int x, y, w, h;
      CtkBorder border;

      ctk_popover_get_rect_coords (popover, &x, &y, &w, &h);
      get_padding_and_border (widget, &border);

      child_alloc.x = x + border.left;
      child_alloc.y = y + border.top;
      child_alloc.width = w - border.left - border.right;
      child_alloc.height = h - border.top - border.bottom;
      ctk_widget_size_allocate (child, &child_alloc);
    }

  if (ctk_widget_get_realized (widget))
    {
      cdk_window_move_resize (ctk_widget_get_window (widget),
                              0, 0, allocation->width, allocation->height);
      ctk_popover_update_shape (popover);
    }

  if (ctk_widget_is_drawable (widget))
    ctk_popover_check_invalidate_borders (popover);
}

static gboolean
ctk_popover_button_press (CtkWidget      *widget,
                          CdkEventButton *event)
{
  CtkPopover *popover = CTK_POPOVER (widget);

  if (event->type != CDK_BUTTON_PRESS)
    return CDK_EVENT_PROPAGATE;

  popover->priv->button_pressed = TRUE;

  return CDK_EVENT_PROPAGATE;
}

static gboolean
ctk_popover_button_release (CtkWidget      *widget,
			    CdkEventButton *event)
{
  CtkPopover *popover = CTK_POPOVER (widget);
  CtkWidget *child, *event_widget;

  child = ctk_bin_get_child (CTK_BIN (widget));

  if (!popover->priv->button_pressed)
    return CDK_EVENT_PROPAGATE;

  event_widget = ctk_get_event_widget ((CdkEvent *) event);

  if (child && event->window == ctk_widget_get_window (widget))
    {
      CtkAllocation child_alloc;

      ctk_widget_get_allocation (child, &child_alloc);

      if (event->x < child_alloc.x ||
          event->x > child_alloc.x + child_alloc.width ||
          event->y < child_alloc.y ||
          event->y > child_alloc.y + child_alloc.height)
        ctk_popover_popdown (popover);
    }
  else if (!event_widget || !ctk_widget_is_ancestor (event_widget, widget))
    {
      ctk_popover_popdown (popover);
    }

  return CDK_EVENT_PROPAGATE;
}

static gboolean
ctk_popover_key_press (CtkWidget   *widget,
                       CdkEventKey *event)
{
  CtkWidget *toplevel, *focus;

  if (event->keyval == CDK_KEY_Escape)
    {
      ctk_popover_popdown (CTK_POPOVER (widget));
      return CDK_EVENT_STOP;
    }

  if (!CTK_POPOVER (widget)->priv->modal)
    return CDK_EVENT_PROPAGATE;

  toplevel = ctk_widget_get_toplevel (widget);

  if (CTK_IS_WINDOW (toplevel))
    {
      focus = ctk_window_get_focus (CTK_WINDOW (toplevel));

      if (focus && ctk_widget_is_ancestor (focus, widget))
        return ctk_widget_event (focus, (CdkEvent*) event);
    }

  return CDK_EVENT_PROPAGATE;
}

static void
ctk_popover_grab_focus (CtkWidget *widget)
{
  CtkPopoverPrivate *priv = CTK_POPOVER (widget)->priv;
  CtkWidget *child;

  if (!priv->visible)
    return;

  /* Focus the first natural child */
  child = ctk_bin_get_child (CTK_BIN (widget));

  if (child)
    ctk_widget_child_focus (child, CTK_DIR_TAB_FORWARD);
}

static gboolean
ctk_popover_focus (CtkWidget        *widget,
                   CtkDirectionType  direction)
{
  CtkPopover *popover = CTK_POPOVER (widget);
  CtkPopoverPrivate *priv = popover->priv;

  if (!priv->visible)
    return FALSE;

  if (!CTK_WIDGET_CLASS (ctk_popover_parent_class)->focus (widget, direction))
    {
      CtkWidget *focus;

      focus = ctk_window_get_focus (popover->priv->window);
      focus = ctk_widget_get_parent (focus);

      /* Unset focus child through children, so it is next stepped from
       * scratch.
       */
      while (focus && focus != widget)
        {
          ctk_container_set_focus_child (CTK_CONTAINER (focus), NULL);
          focus = ctk_widget_get_parent (focus);
        }

      return ctk_widget_child_focus (ctk_bin_get_child (CTK_BIN (widget)),
                                     direction);
    }

  return TRUE;
}

static void
ctk_popover_show (CtkWidget *widget)
{
  CtkPopoverPrivate *priv = CTK_POPOVER (widget)->priv;

  if (priv->window)
    _ctk_window_raise_popover (priv->window, widget);

  priv->visible = TRUE;

  CTK_WIDGET_CLASS (ctk_popover_parent_class)->show (widget);

  if (priv->modal)
    ctk_popover_apply_modality (CTK_POPOVER (widget), TRUE);

  priv->state = STATE_SHOWN;

  if (ctk_widget_get_realized (widget))
    cdk_window_input_shape_combine_region (ctk_widget_get_parent_window (widget),
                                           NULL, 0, 0);
}

static void
ctk_popover_hide (CtkWidget *widget)
{
  CtkPopoverPrivate *priv = CTK_POPOVER (widget)->priv;

  ctk_popover_hide_internal (CTK_POPOVER (widget));

  ctk_popover_stop_transition (CTK_POPOVER (widget));
  priv->state = STATE_HIDDEN;
  priv->transition_diff = 0;
  ctk_progress_tracker_finish (&priv->tracker);
  ctk_widget_set_opacity (widget, 1.0);


  CTK_WIDGET_CLASS (ctk_popover_parent_class)->hide (widget);
}

static void
ctk_popover_class_init (CtkPopoverClass *klass)
{
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = ctk_popover_set_property;
  object_class->get_property = ctk_popover_get_property;
  object_class->finalize = ctk_popover_finalize;
  object_class->dispose = ctk_popover_dispose;

  widget_class->realize = ctk_popover_realize;
  widget_class->map = ctk_popover_map;
  widget_class->unmap = ctk_popover_unmap;
  widget_class->get_preferred_width = ctk_popover_get_preferred_width;
  widget_class->get_preferred_height = ctk_popover_get_preferred_height;
  widget_class->get_preferred_width_for_height = ctk_popover_get_preferred_width_for_height;
  widget_class->get_preferred_height_for_width = ctk_popover_get_preferred_height_for_width;
  widget_class->size_allocate = ctk_popover_size_allocate;
  widget_class->draw = ctk_popover_draw;
  widget_class->button_press_event = ctk_popover_button_press;
  widget_class->button_release_event = ctk_popover_button_release;
  widget_class->key_press_event = ctk_popover_key_press;
  widget_class->grab_focus = ctk_popover_grab_focus;
  widget_class->focus = ctk_popover_focus;
  widget_class->show = ctk_popover_show;
  widget_class->hide = ctk_popover_hide;

  /**
   * CtkPopover:relative-to:
   *
   * Sets the attached widget.
   *
   * Since: 3.12
   */
  properties[PROP_RELATIVE_TO] =
      g_param_spec_object ("relative-to",
                           P_("Relative to"),
                           P_("Widget the bubble window points to"),
                           CTK_TYPE_WIDGET,
                           CTK_PARAM_READWRITE);

  /**
   * CtkPopover:pointing-to:
   *
   * Marks a specific rectangle to be pointed.
   *
   * Since: 3.12
   */
  properties[PROP_POINTING_TO] =
      g_param_spec_boxed ("pointing-to",
                          P_("Pointing to"),
                          P_("Rectangle the bubble window points to"),
                          CDK_TYPE_RECTANGLE,
                          CTK_PARAM_READWRITE);

  /**
   * CtkPopover:position
   *
   * Sets the preferred position of the popover.
   *
   * Since: 3.12
   */
  properties[PROP_POSITION] =
      g_param_spec_enum ("position",
                         P_("Position"),
                         P_("Position to place the bubble window"),
                         CTK_TYPE_POSITION_TYPE, CTK_POS_TOP,
                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkPopover:modal
   *
   * Sets whether the popover is modal (so other elements in the window do not
   * receive input while the popover is visible).
   *
   * Since: 3.12
   */
  properties[PROP_MODAL] =
      g_param_spec_boolean ("modal",
                            P_("Modal"),
                            P_("Whether the popover is modal"),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkPopover:transitions-enabled
   *
   * Whether show/hide transitions are enabled for this popover.
   *
   * Since: 3.16
   *
   * Deprecated: 3.22: You can show or hide the popover without transitions
   *   using ctk_widget_show() and ctk_widget_hide() while ctk_popover_popup()
   *   and ctk_popover_popdown() will use transitions.
   */
  properties[PROP_TRANSITIONS_ENABLED] =
      g_param_spec_boolean ("transitions-enabled",
                            P_("Transitions enabled"),
                            P_("Whether show/hide transitions are enabled or not"),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_DEPRECATED);

  /**
   * CtkPopover:constrain-to:
   *
   * Sets a constraint for the popover position.
   *
   * Since: 3.20
   */
  properties[PROP_CONSTRAIN_TO] =
      g_param_spec_enum ("constrain-to",
                         P_("Constraint"),
                         P_("Constraint for the popover position"),
                         CTK_TYPE_POPOVER_CONSTRAINT, CTK_POPOVER_CONSTRAINT_WINDOW,
                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);

  /**
   * CtkPopover::closed:
   *
   * This signal is emitted when the popover is dismissed either through
   * API or user interaction.
   *
   * Since: 3.12
   */
  signals[CLOSED] =
    g_signal_new (I_("closed"),
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkPopoverClass, closed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  quark_widget_popovers = g_quark_from_static_string ("ctk-quark-widget-popovers");
  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_POPOVER_ACCESSIBLE);
  ctk_widget_class_set_css_name (widget_class, "popover");
}

static void
ctk_popover_update_scrollable (CtkPopover *popover)
{
  CtkPopoverPrivate *priv = popover->priv;
  CtkScrollable *scrollable;

  scrollable = CTK_SCROLLABLE (ctk_widget_get_ancestor (priv->widget,
                                                        CTK_TYPE_SCROLLABLE));
  ctk_popover_set_scrollable_full (popover, scrollable);
}

static void
_ctk_popover_parent_hierarchy_changed (CtkWidget  *widget,
                                       CtkWidget  *previous_toplevel,
                                       CtkPopover *popover)
{
  CtkPopoverPrivate *priv = popover->priv;
  CtkWindow *new_window;

  new_window = CTK_WINDOW (ctk_widget_get_ancestor (widget, CTK_TYPE_WINDOW));

  if (priv->window == new_window)
    return;

  g_object_ref (popover);

  if (ctk_widget_has_grab (CTK_WIDGET (popover)))
    ctk_popover_apply_modality (popover, FALSE);

  if (priv->window)
    _ctk_window_remove_popover (priv->window, CTK_WIDGET (popover));

  if (priv->parent_scrollable)
    ctk_popover_set_scrollable_full (popover, NULL);

  priv->window = new_window;

  if (new_window)
    {
      _ctk_window_add_popover (new_window, CTK_WIDGET (popover), priv->widget, TRUE);
      ctk_popover_update_scrollable (popover);
      ctk_popover_update_position (popover);
    }

  if (ctk_widget_is_visible (CTK_WIDGET (popover)))
    ctk_widget_queue_resize (CTK_WIDGET (popover));

  g_object_unref (popover);
}

static void
_popover_propagate_state (CtkPopover    *popover,
                          CtkStateFlags  state,
                          CtkStateFlags  old_state,
                          CtkStateFlags  flag)
{
  if ((state & flag) != (old_state & flag))
    {
      if ((state & flag) == flag)
        ctk_widget_set_state_flags (CTK_WIDGET (popover), flag, FALSE);
      else
        ctk_widget_unset_state_flags (CTK_WIDGET (popover), flag);
    }
}

static void
_ctk_popover_parent_state_changed (CtkWidget     *widget,
                                   CtkStateFlags  old_state,
                                   CtkPopover    *popover)
{
  guint state;

  state = ctk_widget_get_state_flags (widget);
  _popover_propagate_state (popover, state, old_state,
                            CTK_STATE_FLAG_INSENSITIVE);
  _popover_propagate_state (popover, state, old_state,
                            CTK_STATE_FLAG_BACKDROP);
}

static void
_ctk_popover_parent_grab_notify (CtkWidget  *widget,
                                 gboolean    was_shadowed,
                                 CtkPopover *popover)
{
  CtkPopoverPrivate *priv = popover->priv;

  if (priv->modal &&
      ctk_widget_is_visible (CTK_WIDGET (popover)) &&
      !ctk_widget_has_grab (CTK_WIDGET (popover)))
    {
      CtkWidget *grab_widget;

      grab_widget = ctk_grab_get_current ();

      if (!grab_widget || !CTK_IS_POPOVER (grab_widget))
        ctk_popover_popdown (popover);
    }
}

static void
_ctk_popover_parent_unmap (CtkWidget *widget,
                           CtkPopover *popover)
{
  CtkPopoverPrivate *priv = popover->priv;

  if (priv->state == STATE_SHOWING)
    priv->visible = FALSE;
  else if (priv->state == STATE_SHOWN)
    ctk_popover_set_state (popover, STATE_HIDING);
}

static void
_ctk_popover_parent_size_allocate (CtkWidget     *widget,
                                   CtkAllocation *allocation,
                                   CtkPopover    *popover)
{
  ctk_popover_update_position (popover);
}

static void
_unmanage_popover (GObject *object)
{
  ctk_popover_update_relative_to (CTK_POPOVER (object), NULL);
  g_object_unref (object);
}

static void
widget_manage_popover (CtkWidget  *widget,
                       CtkPopover *popover)
{
  GHashTable *popovers;

  popovers = g_object_get_qdata (G_OBJECT (widget), quark_widget_popovers);

  if (G_UNLIKELY (!popovers))
    {
      popovers = g_hash_table_new_full (NULL, NULL,
                                        (GDestroyNotify) _unmanage_popover, NULL);
      g_object_set_qdata_full (G_OBJECT (widget),
                               quark_widget_popovers, popovers,
                               (GDestroyNotify) g_hash_table_unref);
    }

  g_hash_table_add (popovers, g_object_ref_sink (popover));
}

static void
widget_unmanage_popover (CtkWidget  *widget,
                         CtkPopover *popover)
{
  GHashTable *popovers;

  popovers = g_object_get_qdata (G_OBJECT (widget), quark_widget_popovers);

  if (G_UNLIKELY (!popovers))
    return;

  g_hash_table_remove (popovers, popover);
}

static void
adjustment_changed_cb (CtkAdjustment *adjustment,
                       CtkPopover    *popover)
{
  ctk_popover_update_position (popover);
}

static void
_ctk_popover_set_scrollable (CtkPopover    *popover,
                             CtkScrollable *scrollable)
{
  CtkPopoverPrivate *priv = popover->priv;

  if (priv->parent_scrollable)
    {
      if (priv->vadj)
        {
          g_signal_handlers_disconnect_by_data (priv->vadj, popover);
          g_object_unref (priv->vadj);
          priv->vadj = NULL;
        }

      if (priv->hadj)
        {
          g_signal_handlers_disconnect_by_data (priv->hadj, popover);
          g_object_unref (priv->hadj);
          priv->hadj = NULL;
        }

      g_object_unref (priv->parent_scrollable);
    }

  priv->parent_scrollable = scrollable;

  if (scrollable)
    {
      g_object_ref (scrollable);
      priv->vadj = ctk_scrollable_get_vadjustment (scrollable);
      priv->hadj = ctk_scrollable_get_hadjustment (scrollable);

      if (priv->vadj)
        {
          g_object_ref (priv->vadj);
          g_signal_connect (priv->vadj, "changed",
                            G_CALLBACK (adjustment_changed_cb), popover);
          g_signal_connect (priv->vadj, "value-changed",
                            G_CALLBACK (adjustment_changed_cb), popover);
        }

      if (priv->hadj)
        {
          g_object_ref (priv->hadj);
          g_signal_connect (priv->hadj, "changed",
                            G_CALLBACK (adjustment_changed_cb), popover);
          g_signal_connect (priv->hadj, "value-changed",
                            G_CALLBACK (adjustment_changed_cb), popover);
        }
    }
}

static void
scrollable_notify_cb (GObject    *object,
                      GParamSpec *pspec,
                      CtkPopover *popover)
{
  if (pspec->value_type == CTK_TYPE_ADJUSTMENT)
    _ctk_popover_set_scrollable (popover, CTK_SCROLLABLE (object));
}

static void
ctk_popover_set_scrollable_full (CtkPopover    *popover,
                                 CtkScrollable *scrollable)
{
  CtkPopoverPrivate *priv = popover->priv;

  if (priv->scrollable_notify_id != 0 &&
      g_signal_handler_is_connected (priv->parent_scrollable, priv->scrollable_notify_id))
    {
      g_signal_handler_disconnect (priv->parent_scrollable, priv->scrollable_notify_id);
      priv->scrollable_notify_id = 0;
    }

  _ctk_popover_set_scrollable (popover, scrollable);

  if (scrollable)
    {
      priv->scrollable_notify_id =
        g_signal_connect (priv->parent_scrollable, "notify",
                          G_CALLBACK (scrollable_notify_cb), popover);
    }
}

static void
ctk_popover_update_relative_to (CtkPopover *popover,
                                CtkWidget  *relative_to)
{
  CtkPopoverPrivate *priv = popover->priv;
  CtkStateFlags old_state = 0;

  if (priv->widget == relative_to)
    return;

  g_object_ref (popover);

  if (priv->window)
    {
      _ctk_window_remove_popover (priv->window, CTK_WIDGET (popover));
      priv->window = NULL;
    }

  popover_unset_prev_focus (popover);

  if (priv->widget)
    {
      old_state = ctk_widget_get_state_flags (priv->widget);
      if (g_signal_handler_is_connected (priv->widget, priv->hierarchy_changed_id))
        g_signal_handler_disconnect (priv->widget, priv->hierarchy_changed_id);
      if (g_signal_handler_is_connected (priv->widget, priv->size_allocate_id))
        g_signal_handler_disconnect (priv->widget, priv->size_allocate_id);
      if (g_signal_handler_is_connected (priv->widget, priv->unmap_id))
        g_signal_handler_disconnect (priv->widget, priv->unmap_id);
      if (g_signal_handler_is_connected (priv->widget, priv->state_changed_id))
        g_signal_handler_disconnect (priv->widget, priv->state_changed_id);
      if (g_signal_handler_is_connected (priv->widget, priv->grab_notify_id))
        g_signal_handler_disconnect (priv->widget, priv->grab_notify_id);

      widget_unmanage_popover (priv->widget, popover);
    }

  if (priv->parent_scrollable)
    ctk_popover_set_scrollable_full (popover, NULL);

  priv->widget = relative_to;
  g_object_notify_by_pspec (G_OBJECT (popover), properties[PROP_RELATIVE_TO]);

  if (priv->widget)
    {
      priv->window =
        CTK_WINDOW (ctk_widget_get_ancestor (priv->widget, CTK_TYPE_WINDOW));

      priv->hierarchy_changed_id =
        g_signal_connect (priv->widget, "hierarchy-changed",
                          G_CALLBACK (_ctk_popover_parent_hierarchy_changed),
                          popover);
      priv->size_allocate_id =
        g_signal_connect (priv->widget, "size-allocate",
                          G_CALLBACK (_ctk_popover_parent_size_allocate),
                          popover);
      priv->unmap_id =
        g_signal_connect (priv->widget, "unmap",
                          G_CALLBACK (_ctk_popover_parent_unmap),
                          popover);
      priv->state_changed_id =
        g_signal_connect (priv->widget, "state-flags-changed",
                          G_CALLBACK (_ctk_popover_parent_state_changed),
                          popover);
      priv->grab_notify_id =
        g_signal_connect (priv->widget, "grab-notify",
                          G_CALLBACK (_ctk_popover_parent_grab_notify),
                          popover);

      /* Give ownership of the popover to widget */
      widget_manage_popover (priv->widget, popover);
    }

  if (priv->window)
    _ctk_window_add_popover (priv->window, CTK_WIDGET (popover), priv->widget, TRUE);

  if (priv->widget)
    ctk_popover_update_scrollable (popover);

  if (priv->widget)
    _ctk_popover_parent_state_changed (priv->widget, old_state, popover);

  _ctk_widget_update_parent_muxer (CTK_WIDGET (popover));
  g_object_unref (popover);
}

static void
ctk_popover_update_pointing_to (CtkPopover         *popover,
                                const CdkRectangle *pointing_to)
{
  CtkPopoverPrivate *priv = popover->priv;

  if (pointing_to)
    {
      priv->pointing_to = *pointing_to;
      priv->has_pointing_to = TRUE;
    }
  else
    priv->has_pointing_to = FALSE;

  g_object_notify_by_pspec (G_OBJECT (popover), properties[PROP_POINTING_TO]);
}

static void
ctk_popover_update_preferred_position (CtkPopover      *popover,
                                       CtkPositionType  position)
{
  if (popover->priv->preferred_position == position)
    return;

  popover->priv->preferred_position = position;
  g_object_notify_by_pspec (G_OBJECT (popover), properties[PROP_POSITION]);
}

static void
ctk_popover_multipress_gesture_pressed (CtkGestureMultiPress *gesture,
                                        gint                  n_press,
                                        gdouble               widget_x,
                                        gdouble               widget_y,
                                        CtkPopover           *popover)
{
  CtkPopoverPrivate *priv = popover->priv;

  if (!ctk_window_is_active (priv->window) && ctk_widget_is_drawable (CTK_WIDGET (popover)))
    ctk_window_present_with_time (priv->window, ctk_get_current_event_time ());
}

/**
 * ctk_popover_new:
 * @relative_to: (allow-none): #CtkWidget the popover is related to
 *
 * Creates a new popover to point to @relative_to
 *
 * Returns: a new #CtkPopover
 *
 * Since: 3.12
 **/
CtkWidget *
ctk_popover_new (CtkWidget *relative_to)
{
  g_return_val_if_fail (relative_to == NULL || CTK_IS_WIDGET (relative_to), NULL);

  return g_object_new (CTK_TYPE_POPOVER,
                       "relative-to", relative_to,
                       NULL);
}

/**
 * ctk_popover_set_relative_to:
 * @popover: a #CtkPopover
 * @relative_to: (allow-none): a #CtkWidget
 *
 * Sets a new widget to be attached to @popover. If @popover is
 * visible, the position will be updated.
 *
 * Note: the ownership of popovers is always given to their @relative_to
 * widget, so if @relative_to is set to %NULL on an attached @popover, it
 * will be detached from its previous widget, and consequently destroyed
 * unless extra references are kept.
 *
 * Since: 3.12
 **/
void
ctk_popover_set_relative_to (CtkPopover *popover,
                             CtkWidget  *relative_to)
{
  g_return_if_fail (CTK_IS_POPOVER (popover));
  g_return_if_fail (relative_to == NULL || CTK_IS_WIDGET (relative_to));

  ctk_popover_update_relative_to (popover, relative_to);

  if (relative_to)
    ctk_popover_update_position (popover);
}

/**
 * ctk_popover_get_relative_to:
 * @popover: a #CtkPopover
 *
 * Returns the widget @popover is currently attached to
 *
 * Returns: (transfer none): a #CtkWidget
 *
 * Since: 3.12
 **/
CtkWidget *
ctk_popover_get_relative_to (CtkPopover *popover)
{
  g_return_val_if_fail (CTK_IS_POPOVER (popover), NULL);

  return popover->priv->widget;
}

/**
 * ctk_popover_set_pointing_to:
 * @popover: a #CtkPopover
 * @rect: rectangle to point to
 *
 * Sets the rectangle that @popover will point to, in the
 * coordinate space of the widget @popover is attached to,
 * see ctk_popover_set_relative_to().
 *
 * Since: 3.12
 **/
void
ctk_popover_set_pointing_to (CtkPopover         *popover,
                             const CdkRectangle *rect)
{
  g_return_if_fail (CTK_IS_POPOVER (popover));
  g_return_if_fail (rect != NULL);

  ctk_popover_update_pointing_to (popover, rect);
  ctk_popover_update_position (popover);
}

/**
 * ctk_popover_get_pointing_to:
 * @popover: a #CtkPopover
 * @rect: (out): location to store the rectangle
 *
 * If a rectangle to point to has been set, this function will
 * return %TRUE and fill in @rect with such rectangle, otherwise
 * it will return %FALSE and fill in @rect with the attached
 * widget coordinates.
 *
 * Returns: %TRUE if a rectangle to point to was set.
 **/
gboolean
ctk_popover_get_pointing_to (CtkPopover   *popover,
                             CdkRectangle *rect)
{
  CtkPopoverPrivate *priv = ctk_popover_get_instance_private (popover);

  g_return_val_if_fail (CTK_IS_POPOVER (popover), FALSE);

  if (rect)
    {
      if (priv->has_pointing_to)
        *rect = priv->pointing_to;
      else if (priv->widget)
        {
          ctk_widget_get_allocation (priv->widget, rect);
          rect->x = rect->y = 0;
        }
    }

  return priv->has_pointing_to;
}

/**
 * ctk_popover_set_position:
 * @popover: a #CtkPopover
 * @position: preferred popover position
 *
 * Sets the preferred position for @popover to appear. If the @popover
 * is currently visible, it will be immediately updated.
 *
 * This preference will be respected where possible, although
 * on lack of space (eg. if close to the window edges), the
 * #CtkPopover may choose to appear on the opposite side
 *
 * Since: 3.12
 **/
void
ctk_popover_set_position (CtkPopover      *popover,
                          CtkPositionType  position)
{
  g_return_if_fail (CTK_IS_POPOVER (popover));
  g_return_if_fail (position >= CTK_POS_LEFT && position <= CTK_POS_BOTTOM);

  ctk_popover_update_preferred_position (popover, position);
  ctk_popover_update_position (popover);
}

/**
 * ctk_popover_get_position:
 * @popover: a #CtkPopover
 *
 * Returns the preferred position of @popover.
 *
 * Returns: The preferred position.
 **/
CtkPositionType
ctk_popover_get_position (CtkPopover *popover)
{
  g_return_val_if_fail (CTK_IS_POPOVER (popover), CTK_POS_TOP);

  return popover->priv->preferred_position;
}

/**
 * ctk_popover_set_modal:
 * @popover: a #CtkPopover
 * @modal: #TRUE to make popover claim all input within the toplevel
 *
 * Sets whether @popover is modal, a modal popover will grab all input
 * within the toplevel and grab the keyboard focus on it when being
 * displayed. Clicking outside the popover area or pressing Esc will
 * dismiss the popover and ungrab input.
 *
 * Since: 3.12
 **/
void
ctk_popover_set_modal (CtkPopover *popover,
                       gboolean    modal)
{
  CtkPopoverPrivate *priv = ctk_popover_get_instance_private (popover);

  g_return_if_fail (CTK_IS_POPOVER (popover));

  modal = modal != FALSE;

  if (priv->modal == modal)
    return;

  priv->modal = modal;

  if (ctk_widget_is_visible (CTK_WIDGET (popover)))
    ctk_popover_apply_modality (popover, priv->modal);

  g_object_notify_by_pspec (G_OBJECT (popover), properties[PROP_MODAL]);
}

/**
 * ctk_popover_get_modal:
 * @popover: a #CtkPopover
 *
 * Returns whether the popover is modal, see ctk_popover_set_modal to
 * see the implications of this.
 *
 * Returns: #TRUE if @popover is modal
 *
 * Since: 3.12
 **/
gboolean
ctk_popover_get_modal (CtkPopover *popover)
{
  g_return_val_if_fail (CTK_IS_POPOVER (popover), FALSE);

  return popover->priv->modal;
}

/**
 * ctk_popover_set_transitions_enabled:
 * @popover: a #CtkPopover
 * @transitions_enabled: Whether transitions are enabled
 *
 * Sets whether show/hide transitions are enabled on this popover
 *
 * Since: 3.16
 *
 * Deprecated: 3.22: You can show or hide the popover without transitions
 *   using ctk_widget_show() and ctk_widget_hide() while ctk_popover_popup()
 *   and ctk_popover_popdown() will use transitions.
 */
void
ctk_popover_set_transitions_enabled (CtkPopover *popover,
                                     gboolean    transitions_enabled)
{
  CtkPopoverPrivate *priv = ctk_popover_get_instance_private (popover);

  g_return_if_fail (CTK_IS_POPOVER (popover));

  transitions_enabled = !!transitions_enabled;

  if (priv->transitions_enabled == transitions_enabled)
    return;

  priv->transitions_enabled = transitions_enabled;
  g_object_notify_by_pspec (G_OBJECT (popover), properties[PROP_TRANSITIONS_ENABLED]);
}

/**
 * ctk_popover_get_transitions_enabled:
 * @popover: a #CtkPopover
 *
 * Returns whether show/hide transitions are enabled on this popover.
 *
 * Returns: #TRUE if the show and hide transitions of the given
 *          popover are enabled, #FALSE otherwise.
 *
 * Since: 3.16
 *
 * Deprecated: 3.22: You can show or hide the popover without transitions
 *   using ctk_widget_show() and ctk_widget_hide() while ctk_popover_popup()
 *   and ctk_popover_popdown() will use transitions.
 */
gboolean
ctk_popover_get_transitions_enabled (CtkPopover *popover)
{
  g_return_val_if_fail (CTK_IS_POPOVER (popover), FALSE);

  return popover->priv->transitions_enabled;
}


static void
back_to_main (CtkWidget *popover)
{
  CtkWidget *stack;

  stack = ctk_bin_get_child (CTK_BIN (popover));
  ctk_stack_set_visible_child_name (CTK_STACK (stack), "main");
}

/**
 * ctk_popover_bind_model:
 * @popover: a #CtkPopover
 * @model: (allow-none): the #GMenuModel to bind to or %NULL to remove
 *   binding
 * @action_namespace: (allow-none): the namespace for actions in @model
 *
 * Establishes a binding between a #CtkPopover and a #GMenuModel.
 *
 * The contents of @popover are removed and then refilled with menu items
 * according to @model.  When @model changes, @popover is updated.
 * Calling this function twice on @popover with different @model will
 * cause the first binding to be replaced with a binding to the new
 * model. If @model is %NULL then any previous binding is undone and
 * all children are removed.
 *
 * If @action_namespace is non-%NULL then the effect is as if all
 * actions mentioned in the @model have their names prefixed with the
 * namespace, plus a dot.  For example, if the action “quit” is
 * mentioned and @action_namespace is “app” then the effective action
 * name is “app.quit”.
 *
 * This function uses #CtkActionable to define the action name and
 * target values on the created menu items.  If you want to use an
 * action group other than “app” and “win”, or if you want to use a
 * #CtkMenuShell outside of a #CtkApplicationWindow, then you will need
 * to attach your own action group to the widget hierarchy using
 * ctk_widget_insert_action_group().  As an example, if you created a
 * group with a “quit” action and inserted it with the name “mygroup”
 * then you would use the action name “mygroup.quit” in your
 * #GMenuModel.
 *
 * Since: 3.12
 */
void
ctk_popover_bind_model (CtkPopover  *popover,
                        GMenuModel  *model,
                        const gchar *action_namespace)
{
  CtkWidget *child;
  CtkWidget *stack;
  CtkStyleContext *style_context;

  g_return_if_fail (CTK_IS_POPOVER (popover));
  g_return_if_fail (model == NULL || G_IS_MENU_MODEL (model));

  child = ctk_bin_get_child (CTK_BIN (popover));
  if (child)
    ctk_widget_destroy (child);

  style_context = ctk_widget_get_style_context (CTK_WIDGET (popover));

  if (model)
    {
      stack = ctk_stack_new ();
      ctk_stack_set_vhomogeneous (CTK_STACK (stack), FALSE);
      ctk_stack_set_transition_type (CTK_STACK (stack), CTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
      ctk_stack_set_interpolate_size (CTK_STACK (stack), TRUE);
      ctk_widget_show (stack);
      ctk_container_add (CTK_CONTAINER (popover), stack);

      ctk_menu_section_box_new_toplevel (CTK_STACK (stack),
                                         model,
                                         action_namespace,
                                         popover);
      ctk_stack_set_visible_child_name (CTK_STACK (stack), "main");

      g_signal_connect (popover, "unmap", G_CALLBACK (back_to_main), NULL);
      g_signal_connect (popover, "map", G_CALLBACK (back_to_main), NULL);

      ctk_style_context_add_class (style_context, CTK_STYLE_CLASS_MENU);
    }
  else
    {
      ctk_style_context_remove_class (style_context, CTK_STYLE_CLASS_MENU);
    }
}

/**
 * ctk_popover_new_from_model:
 * @relative_to: (allow-none): #CtkWidget the popover is related to
 * @model: a #GMenuModel
 *
 * Creates a #CtkPopover and populates it according to
 * @model. The popover is pointed to the @relative_to widget.
 *
 * The created buttons are connected to actions found in the
 * #CtkApplicationWindow to which the popover belongs - typically
 * by means of being attached to a widget that is contained within
 * the #CtkApplicationWindows widget hierarchy.
 *
 * Actions can also be added using ctk_widget_insert_action_group()
 * on the menus attach widget or on any of its parent widgets.
 *
 * Returns: the new #CtkPopover
 *
 * Since: 3.12
 */
CtkWidget *
ctk_popover_new_from_model (CtkWidget  *relative_to,
                            GMenuModel *model)
{
  CtkWidget *popover;

  g_return_val_if_fail (relative_to == NULL || CTK_IS_WIDGET (relative_to), NULL);
  g_return_val_if_fail (G_IS_MENU_MODEL (model), NULL);

  popover = ctk_popover_new (relative_to);
  ctk_popover_bind_model (CTK_POPOVER (popover), model, NULL);

  return popover;
}

/**
 * ctk_popover_set_default_widget:
 * @popover: a #CtkPopover
 * @widget: (allow-none): the new default widget, or %NULL
 *
 * Sets the widget that should be set as default widget while
 * the popover is shown (see ctk_window_set_default()). #CtkPopover
 * remembers the previous default widget and reestablishes it
 * when the popover is dismissed.
 *
 * Since: 3.18
 */
void
ctk_popover_set_default_widget (CtkPopover *popover,
                                CtkWidget  *widget)
{
  CtkPopoverPrivate *priv = ctk_popover_get_instance_private (popover);

  g_return_if_fail (CTK_IS_POPOVER (popover));
  g_return_if_fail (widget == NULL || ctk_widget_get_can_default (widget));

  if (priv->default_widget == widget)
    return;

  if (priv->default_widget)
    g_object_unref (priv->default_widget);

  priv->default_widget = widget;

  if (priv->default_widget)
    g_object_ref (priv->default_widget);

  if (ctk_widget_get_mapped (CTK_WIDGET (popover)))
    ctk_window_set_default (priv->window, priv->default_widget);
}

/**
 * ctk_popover_get_default_widget:
 * @popover: a #CtkPopover
 *
 * Gets the widget that should be set as the default while
 * the popover is shown.
 *
 * Returns: (nullable) (transfer none): the default widget,
 * or %NULL if there is none
 *
 * Since: 3.18
 */
CtkWidget *
ctk_popover_get_default_widget (CtkPopover *popover)
{
  CtkPopoverPrivate *priv = ctk_popover_get_instance_private (popover);

  g_return_val_if_fail (CTK_IS_POPOVER (popover), NULL);

  return priv->default_widget;
}

/**
 * ctk_popover_set_constrain_to:
 * @popover: a #CtkPopover
 * @constraint: the new constraint
 *
 * Sets a constraint for positioning this popover.
 *
 * Note that not all platforms support placing popovers freely,
 * and may already impose constraints.
 *
 * Since: 3.20
 */
void
ctk_popover_set_constrain_to (CtkPopover           *popover,
                              CtkPopoverConstraint  constraint)
{
  CtkPopoverPrivate *priv = ctk_popover_get_instance_private (popover);

  g_return_if_fail (CTK_IS_POPOVER (popover));

  if (priv->constraint == constraint)
    return;

  priv->constraint = constraint;
  ctk_popover_update_position (popover);

  g_object_notify_by_pspec (G_OBJECT (popover), properties[PROP_CONSTRAIN_TO]);
}

/**
 * ctk_popover_get_constrain_to:
 * @popover: a #CtkPopover
 *
 * Returns the constraint for placing this popover.
 * See ctk_popover_set_constrain_to().
 *
 * Returns: the constraint for placing this popover.
 *
 * Since: 3.20
 */
CtkPopoverConstraint
ctk_popover_get_constrain_to (CtkPopover *popover)
{
  CtkPopoverPrivate *priv = ctk_popover_get_instance_private (popover);

  g_return_val_if_fail (CTK_IS_POPOVER (popover), CTK_POPOVER_CONSTRAINT_WINDOW);

  return priv->constraint;
}

/**
 * ctk_popover_popup:
 * @popover: a #CtkPopover
 *
 * Pops @popover up. This is different than a ctk_widget_show() call
 * in that it shows the popover with a transition. If you want to show
 * the popover without a transition, use ctk_widget_show().
 *
 * Since: 3.22
 */
void
ctk_popover_popup (CtkPopover *popover)
{
  CtkPopoverPrivate *priv = ctk_popover_get_instance_private (popover);

  g_return_if_fail (CTK_IS_POPOVER (popover));

  if (priv->state == STATE_SHOWING ||
      priv->state == STATE_SHOWN)
    return;

  ctk_widget_show (CTK_WIDGET (popover));

  if (transitions_enabled (popover))
    ctk_popover_set_state (popover, STATE_SHOWING);
}

/**
 * ctk_popover_popdown:
 * @popover: a #CtkPopover
 *
 * Pops @popover down.This is different than a ctk_widget_hide() call
 * in that it shows the popover with a transition. If you want to hide
 * the popover without a transition, use ctk_widget_hide().
 *
 * Since: 3.22
 */
void
ctk_popover_popdown (CtkPopover *popover)
{
  CtkPopoverPrivate *priv = ctk_popover_get_instance_private (popover);

  g_return_if_fail (CTK_IS_POPOVER (popover));

  if (priv->state == STATE_HIDING ||
      priv->state == STATE_HIDDEN)
    return;


  if (!transitions_enabled (popover))
    ctk_widget_hide (CTK_WIDGET (popover));
  else
    ctk_popover_set_state (popover, STATE_HIDING);

  ctk_popover_hide_internal (popover);
}
