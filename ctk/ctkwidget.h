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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

#ifndef __CTK_WIDGET_H__
#define __CTK_WIDGET_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <cdk/cdk.h>
#include <ctk/ctkaccelgroup.h>
#include <ctk/ctkborder.h>
#include <ctk/ctktypes.h>
#include <atk/atk.h>

G_BEGIN_DECLS

/* Kinds of widget-specific help */
/**
 * CtkWidgetHelpType:
 * @CTK_WIDGET_HELP_TOOLTIP: Tooltip.
 * @CTK_WIDGET_HELP_WHATS_THIS: What’s this.
 *
 * Kinds of widget-specific help. Used by the ::show-help signal.
 */
typedef enum
{
  CTK_WIDGET_HELP_TOOLTIP,
  CTK_WIDGET_HELP_WHATS_THIS
} CtkWidgetHelpType;

/* Macro for casting a pointer to a CtkWidget or CtkWidgetClass pointer.
 * Macros for testing whether widget or klass are of type CTK_TYPE_WIDGET.
 */
#define CTK_TYPE_WIDGET			  (ctk_widget_get_type ())
#define CTK_WIDGET(widget)		  (G_TYPE_CHECK_INSTANCE_CAST ((widget), CTK_TYPE_WIDGET, CtkWidget))
#define CTK_WIDGET_CLASS(klass)		  (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_WIDGET, CtkWidgetClass))
#define CTK_IS_WIDGET(widget)		  (G_TYPE_CHECK_INSTANCE_TYPE ((widget), CTK_TYPE_WIDGET))
#define CTK_IS_WIDGET_CLASS(klass)	  (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_WIDGET))
#define CTK_WIDGET_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_WIDGET, CtkWidgetClass))

#define CTK_TYPE_REQUISITION              (ctk_requisition_get_type ())

typedef struct _CtkWidgetPrivate       CtkWidgetPrivate;
typedef struct _CtkWidgetClass	       CtkWidgetClass;
typedef struct _CtkWidgetClassPrivate  CtkWidgetClassPrivate;

/**
 * CtkAllocation:
 * @x: the X position of the widget’s area relative to its parents allocation.
 * @y: the Y position of the widget’s area relative to its parents allocation.
 * @width: the width of the widget’s allocated area.
 * @height: the height of the widget’s allocated area.
 *
 * A #CtkAllocation-struct of a widget represents region
 * which has been allocated to the widget by its parent. It is a subregion
 * of its parents allocation. See
 * [CtkWidget’s geometry management section][geometry-management] for
 * more information.
 */
typedef 	CdkRectangle	   CtkAllocation;

/**
 * CtkCallback:
 * @widget: the widget to operate on
 * @data: (closure): user-supplied data
 *
 * The type of the callback functions used for e.g. iterating over
 * the children of a container, see ctk_container_foreach().
 */
typedef void    (*CtkCallback)     (CtkWidget        *widget,
				    gpointer          data);

/**
 * CtkTickCallback:
 * @widget: the widget
 * @frame_clock: the frame clock for the widget (same as calling ctk_widget_get_frame_clock())
 * @user_data: user data passed to ctk_widget_add_tick_callback().
 *
 * Callback type for adding a function to update animations. See ctk_widget_add_tick_callback().
 *
 * Returns: %G_SOURCE_CONTINUE if the tick callback should continue to be called,
 *  %G_SOURCE_REMOVE if the tick callback should be removed.
 *
 * Since: 3.8
 */
typedef gboolean (*CtkTickCallback) (CtkWidget     *widget,
                                     CdkFrameClock *frame_clock,
                                     gpointer       user_data);

/**
 * CtkRequisition:
 * @width: the widget’s desired width
 * @height: the widget’s desired height
 *
 * A #CtkRequisition-struct represents the desired size of a widget. See
 * [CtkWidget’s geometry management section][geometry-management] for
 * more information.
 */
struct _CtkRequisition
{
  gint width;
  gint height;
};

/* The widget is the base of the tree for displayable objects.
 *  (A displayable object is one which takes up some amount
 *  of screen real estate). It provides a common base and interface
 *  which actual widgets must adhere to.
 */
struct _CtkWidget
{
  GInitiallyUnowned parent_instance;

  /*< private >*/

  CtkWidgetPrivate *priv;
};

/**
 * CtkWidgetClass:
 * @parent_class: The object class structure needs to be the first
 *   element in the widget class structure in order for the class mechanism
 *   to work correctly. This allows a CtkWidgetClass pointer to be cast to
 *   a GObjectClass pointer.
 * @activate_signal: The signal to emit when a widget of this class is
 *   activated, ctk_widget_activate() handles the emission.
 *   Implementation of this signal is optional.
 * @dispatch_child_properties_changed: Seldomly overidden.
 * @destroy: Signals that all holders of a reference to the widget
 *   should release the reference that they hold.
 * @show: Signal emitted when widget is shown
 * @show_all: Recursively shows a widget, and any child widgets (if the widget is
 * a container).
 * @hide: Signal emitted when widget is hidden.
 * @map: Signal emitted when widget is going to be mapped, that is
 *   when the widget is visible (which is controlled with
 *   ctk_widget_set_visible()) and all its parents up to the toplevel
 *   widget are also visible.
 * @unmap: Signal emitted when widget is going to be unmapped, which
 *   means that either it or any of its parents up to the toplevel
 *   widget have been set as hidden.
 * @realize: Signal emitted when widget is associated with a
 *   #CdkWindow, which means that ctk_widget_realize() has been called or
 *   the widget has been mapped (that is, it is going to be drawn).
 * @unrealize: Signal emitted when the CdkWindow associated with
 *   widget is destroyed, which means that ctk_widget_unrealize() has
 *   been called or the widget has been unmapped (that is, it is going
 *   to be hidden).
 * @size_allocate: Signal emitted to get the widget allocation.
 * @state_changed: Signal emitted when the widget state
 *   changes. Deprecated: 3.0
 * @state_flags_changed: Signal emitted when the widget state changes,
 *   see ctk_widget_get_state_flags().
 * @parent_set: Signal emitted when a new parent has been set on a
 *   widget.
 * @hierarchy_changed: Signal emitted when the anchored state of a
 *   widget changes.
 * @style_set: Signal emitted when a new style has been set on a
 * widget. Deprecated: 3.0
 * @direction_changed: Signal emitted when the text direction of a
 *   widget changes.
 * @grab_notify: Signal emitted when a widget becomes shadowed by a
 *   CTK+ grab (not a pointer or keyboard grab) on another widget, or
 *   when it becomes unshadowed due to a grab being removed.
 * @child_notify: Signal emitted for each child property that has
 *   changed on an object.
 * @draw: Signal emitted when a widget is supposed to render itself.
 * @get_request_mode: This allows a widget to tell its parent container whether
 *   it prefers to be allocated in %CTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH or
 *   %CTK_SIZE_REQUEST_WIDTH_FOR_HEIGHT mode.
 *   %CTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH means the widget prefers to have
 *   #CtkWidgetClass.get_preferred_width() called and then
 *   #CtkWidgetClass.get_preferred_height_for_width().
 *   %CTK_SIZE_REQUEST_CONSTANT_SIZE disables any height-for-width or
 *   width-for-height geometry management for a said widget and is the
 *   default return.
 *   It’s important to note (as described below) that any widget
 *   which trades height-for-width or width-for-height must respond properly 
 *   to both of the virtual methods #CtkWidgetClass.get_preferred_height_for_width()
 *   and #CtkWidgetClass.get_preferred_width_for_height() since it might be 
 *   queried in either #CtkSizeRequestMode by its parent container.
 * @get_preferred_height: This is called by containers to obtain the minimum
 *   and natural height of a widget. A widget that does not actually trade
 *   any height for width or width for height only has to implement these
 *   two virtual methods (#CtkWidgetClass.get_preferred_width() and
 *   #CtkWidgetClass.get_preferred_height()).
 * @get_preferred_width_for_height: This is analogous to
 *   #CtkWidgetClass.get_preferred_height_for_width() except that it
 *   operates in the oposite orientation. It’s rare that a widget actually
 *   does %CTK_SIZE_REQUEST_WIDTH_FOR_HEIGHT requests but this can happen
 *   when, for example, a widget or container gets additional columns to
 *   compensate for a smaller allocated height.
 * @get_preferred_width: This is called by containers to obtain the minimum
 *   and natural width of a widget. A widget will never be allocated a width
 *   less than its minimum and will only ever be allocated a width greater
 *   than the natural width once all of the said widget’s siblings have
 *   received their natural widths.
 *   Furthermore, a widget will only ever be allocated a width greater than
 *   its natural width if it was configured to receive extra expand space
 *   from its parent container.
 * @get_preferred_height_for_width: This is similar to
 *   #CtkWidgetClass.get_preferred_height() except that it is passed a
 *   contextual width to request height for. By implementing this virtual
 *   method it is possible for a #CtkLabel to tell its parent how much height
 *   would be required if the label were to be allocated a said width.
 * @mnemonic_activate: Activates the @widget if @group_cycling is
 *   %FALSE, and just grabs the focus if @group_cycling is %TRUE.
 * @grab_focus: Causes @widget to have the keyboard focus for the
 *   #CtkWindow it’s inside.
 * @focus:
 * @move_focus: Signal emitted when a change of focus is requested
 * @keynav_failed: Signal emitted if keyboard navigation fails.
 * @event: The CTK+ main loop will emit three signals for each CDK
 *   event delivered to a widget: one generic ::event signal, another,
 *   more specific, signal that matches the type of event delivered
 *   (e.g. "key-press-event") and finally a generic "event-after"
 *   signal.
 * @button_press_event: Signal will be emitted when a button
 *   (typically from a mouse) is pressed.
 * @button_release_event: Signal will be emitted when a button
 *   (typically from a mouse) is released.
 * @scroll_event: Signal emitted when a button in the 4 to 7 range is
 *   pressed.
 * @motion_notify_event: Signal emitted when the pointer moves over
 *   the widget’s #CdkWindow.
 * @delete_event: Signal emitted if a user requests that a toplevel
 *   window is closed.
 * @destroy_event: Signal is emitted when a #CdkWindow is destroyed.
 * @key_press_event: Signal emitted when a key is pressed.
 * @key_release_event: Signal is emitted when a key is released.
 * @enter_notify_event: Signal event will be emitted when the pointer
 *   enters the widget’s window.
 * @leave_notify_event: Will be emitted when the pointer leaves the
 *   widget’s window.
 * @configure_event: Signal will be emitted when the size, position or
 *   stacking of the widget’s window has changed.
 * @focus_in_event: Signal emitted when the keyboard focus enters the
 * widget’s window.
 * @focus_out_event: Signal emitted when the keyboard focus leaves the
 * widget’s window.
 * @map_event: Signal emitted when the widget’s window is mapped.
 * @unmap_event: Signal will be emitted when the widget’s window is
 *   unmapped.
 * @property_notify_event: Signal will be emitted when a property on
 *   the widget’s window has been changed or deleted.
 * @selection_clear_event: Signal will be emitted when the the
 *   widget’s window has lost ownership of a selection.
 * @selection_request_event: Signal will be emitted when another
 *   client requests ownership of the selection owned by the widget's
 *   window.
 * @selection_notify_event:
 * @proximity_in_event:
 * @proximity_out_event:
 * @visibility_notify_event: Signal emitted when the widget’s window is
 *   obscured or unobscured.
 * @window_state_event: Signal emitted when the state of the toplevel
 *   window associated to the widget changes.
 * @damage_event: Signal emitted when a redirected window belonging to
 *   widget gets drawn into.
 * @grab_broken_event: Signal emitted when a pointer or keyboard grab
 *   on a window belonging to widget gets broken.
 * @selection_get:
 * @selection_received:
 * @drag_begin: Signal emitted on the drag source when a drag is
 *   started.
 * @drag_end: Signal emitted on the drag source when a drag is
 *   finished.
 * @drag_data_get: Signal emitted on the drag source when the drop
 *   site requests the data which is dragged.
 * @drag_data_delete: Signal emitted on the drag source when a drag
 *   with the action %CDK_ACTION_MOVE is successfully completed.
 * @drag_leave: Signal emitted on the drop site when the cursor leaves
 *   the widget.
 * @drag_motion: signal emitted on the drop site when the user moves
 *   the cursor over the widget during a drag.
 * @drag_drop: Signal emitted on the drop site when the user drops the
 *   data onto the widget.
 * @drag_data_received: Signal emitted on the drop site when the
 *   dragged data has been received.
 * @drag_failed: Signal emitted on the drag source when a drag has
 *   failed.
 * @popup_menu: Signal emitted whenever a widget should pop up a
 *   context menu.
 * @show_help:
 * @get_accessible: Returns the accessible object that describes the
 *   widget to an assistive technology.
 * @screen_changed: Signal emitted when the screen of a widget has
 *   changed.
 * @can_activate_accel: Signal allows applications and derived widgets
 *   to override the default CtkWidget handling for determining whether
 *   an accelerator can be activated.
 * @composited_changed: Signal emitted when the composited status of
 *   widgets screen changes. See cdk_screen_is_composited().
 * @query_tooltip: Signal emitted when “has-tooltip” is %TRUE and the
 *   hover timeout has expired with the cursor hovering “above”
 *   widget; or emitted when widget got focus in keyboard mode.
 * @compute_expand: Computes whether a container should give this
 *   widget extra space when possible.
 * @adjust_size_request: Convert an initial size request from a widget's
 *   #CtkSizeRequestMode virtual method implementations into a size request to
 *   be used by parent containers in laying out the widget.
 *   adjust_size_request adjusts from a child widget's
 *   original request to what a parent container should
 *   use for layout. The @for_size argument will be -1 if the request should
 *   not be for a particular size in the opposing orientation, i.e. if the
 *   request is not height-for-width or width-for-height. If @for_size is
 *   greater than -1, it is the proposed allocation in the opposing
 *   orientation that we need the request for. Implementations of
 *   adjust_size_request should chain up to the default implementation,
 *   which applies #CtkWidget’s margin properties and imposes any values
 *   from ctk_widget_set_size_request(). Chaining up should be last,
 *   after your subclass adjusts the request, so
 *   #CtkWidget can apply constraints and add the margin properly.
 * @adjust_size_allocation: Convert an initial size allocation assigned
 *   by a #CtkContainer using ctk_widget_size_allocate(), into an actual
 *   size allocation to be used by the widget. adjust_size_allocation
 *   adjusts to a child widget’s actual allocation
 *   from what a parent container computed for the
 *   child. The adjusted allocation must be entirely within the original
 *   allocation. In any custom implementation, chain up to the default
 *   #CtkWidget implementation of this method, which applies the margin
 *   and alignment properties of #CtkWidget. Chain up
 *   before performing your own adjustments so your
 *   own adjustments remove more allocation after the #CtkWidget base
 *   class has already removed margin and alignment. The natural size
 *   passed in should be adjusted in the same way as the allocated size,
 *   which allows adjustments to perform alignments or other changes
 *   based on natural size.
 * @style_updated: Signal emitted when the CtkStyleContext of a widget
 *   is changed.
 * @touch_event: Signal emitted when a touch event happens
 * @get_preferred_height_and_baseline_for_width:
 * @adjust_baseline_request:
 * @adjust_baseline_allocation:
 * @queue_draw_region: Invalidates the area of widget defined by
 *   region by calling cdk_window_invalidate_region() on the widget's
 *   window and all its child windows.
 */
struct _CtkWidgetClass
{
  GInitiallyUnownedClass parent_class;

  /*< public >*/

  guint activate_signal;

  /* seldomly overidden */
  void (*dispatch_child_properties_changed) (CtkWidget   *widget,
					     guint        n_pspecs,
					     GParamSpec **pspecs);

  /* basics */
  void (* destroy)             (CtkWidget        *widget);
  void (* show)		       (CtkWidget        *widget);
  void (* show_all)            (CtkWidget        *widget);
  void (* hide)		       (CtkWidget        *widget);
  void (* map)		       (CtkWidget        *widget);
  void (* unmap)	       (CtkWidget        *widget);
  void (* realize)	       (CtkWidget        *widget);
  void (* unrealize)	       (CtkWidget        *widget);
  void (* size_allocate)       (CtkWidget        *widget,
				CtkAllocation    *allocation);
  void (* state_changed)       (CtkWidget        *widget,
				CtkStateType   	  previous_state);
  void (* state_flags_changed) (CtkWidget        *widget,
				CtkStateFlags  	  previous_state_flags);
  void (* parent_set)	       (CtkWidget        *widget,
				CtkWidget        *previous_parent);
  void (* hierarchy_changed)   (CtkWidget        *widget,
				CtkWidget        *previous_toplevel);
  void (* style_set)	       (CtkWidget        *widget,
				CtkStyle         *previous_style);
  void (* direction_changed)   (CtkWidget        *widget,
				CtkTextDirection  previous_direction);
  void (* grab_notify)         (CtkWidget        *widget,
				gboolean          was_grabbed);
  void (* child_notify)        (CtkWidget	 *widget,
				GParamSpec       *child_property);
  gboolean (* draw)	       (CtkWidget	 *widget,
                                cairo_t          *cr);

  /* size requests */
  CtkSizeRequestMode (* get_request_mode)               (CtkWidget      *widget);

  void               (* get_preferred_height)           (CtkWidget       *widget,
                                                         gint            *minimum_height,
                                                         gint            *natural_height);
  void               (* get_preferred_width_for_height) (CtkWidget       *widget,
                                                         gint             height,
                                                         gint            *minimum_width,
                                                         gint            *natural_width);
  void               (* get_preferred_width)            (CtkWidget       *widget,
                                                         gint            *minimum_width,
                                                         gint            *natural_width);
  void               (* get_preferred_height_for_width) (CtkWidget       *widget,
                                                         gint             width,
                                                         gint            *minimum_height,
                                                         gint            *natural_height);

  /* Mnemonics */
  gboolean (* mnemonic_activate)        (CtkWidget           *widget,
                                         gboolean             group_cycling);

  /* explicit focus */
  void     (* grab_focus)               (CtkWidget           *widget);
  gboolean (* focus)                    (CtkWidget           *widget,
                                         CtkDirectionType     direction);

  /* keyboard navigation */
  void     (* move_focus)               (CtkWidget           *widget,
                                         CtkDirectionType     direction);
  gboolean (* keynav_failed)            (CtkWidget           *widget,
                                         CtkDirectionType     direction);

  /* events */
  gboolean (* event)			(CtkWidget	     *widget,
					 CdkEvent	     *event);
  gboolean (* button_press_event)	(CtkWidget	     *widget,
					 CdkEventButton      *event);
  gboolean (* button_release_event)	(CtkWidget	     *widget,
					 CdkEventButton      *event);
  gboolean (* scroll_event)		(CtkWidget           *widget,
					 CdkEventScroll      *event);
  gboolean (* motion_notify_event)	(CtkWidget	     *widget,
					 CdkEventMotion      *event);
  gboolean (* delete_event)		(CtkWidget	     *widget,
					 CdkEventAny	     *event);
  gboolean (* destroy_event)		(CtkWidget	     *widget,
					 CdkEventAny	     *event);
  gboolean (* key_press_event)		(CtkWidget	     *widget,
					 CdkEventKey	     *event);
  gboolean (* key_release_event)	(CtkWidget	     *widget,
					 CdkEventKey	     *event);
  gboolean (* enter_notify_event)	(CtkWidget	     *widget,
					 CdkEventCrossing    *event);
  gboolean (* leave_notify_event)	(CtkWidget	     *widget,
					 CdkEventCrossing    *event);
  gboolean (* configure_event)		(CtkWidget	     *widget,
					 CdkEventConfigure   *event);
  gboolean (* focus_in_event)		(CtkWidget	     *widget,
					 CdkEventFocus       *event);
  gboolean (* focus_out_event)		(CtkWidget	     *widget,
					 CdkEventFocus       *event);
  gboolean (* map_event)		(CtkWidget	     *widget,
					 CdkEventAny	     *event);
  gboolean (* unmap_event)		(CtkWidget	     *widget,
					 CdkEventAny	     *event);
  gboolean (* property_notify_event)	(CtkWidget	     *widget,
					 CdkEventProperty    *event);
  gboolean (* selection_clear_event)	(CtkWidget	     *widget,
					 CdkEventSelection   *event);
  gboolean (* selection_request_event)	(CtkWidget	     *widget,
					 CdkEventSelection   *event);
  gboolean (* selection_notify_event)	(CtkWidget	     *widget,
					 CdkEventSelection   *event);
  gboolean (* proximity_in_event)	(CtkWidget	     *widget,
					 CdkEventProximity   *event);
  gboolean (* proximity_out_event)	(CtkWidget	     *widget,
					 CdkEventProximity   *event);
  gboolean (* visibility_notify_event)	(CtkWidget	     *widget,
					 CdkEventVisibility  *event);
  gboolean (* window_state_event)	(CtkWidget	     *widget,
					 CdkEventWindowState *event);
  gboolean (* damage_event)             (CtkWidget           *widget,
                                         CdkEventExpose      *event);
  gboolean (* grab_broken_event)        (CtkWidget           *widget,
                                         CdkEventGrabBroken  *event);

  /* selection */
  void     (* selection_get)       (CtkWidget          *widget,
				    CtkSelectionData   *selection_data,
				    guint               info,
				    guint               time_);
  void     (* selection_received)  (CtkWidget          *widget,
				    CtkSelectionData   *selection_data,
				    guint               time_);

  /* Source side drag signals */
  void     (* drag_begin)          (CtkWidget         *widget,
				    CdkDragContext     *context);
  void     (* drag_end)	           (CtkWidget	       *widget,
				    CdkDragContext     *context);
  void     (* drag_data_get)       (CtkWidget          *widget,
				    CdkDragContext     *context,
				    CtkSelectionData   *selection_data,
				    guint               info,
				    guint               time_);
  void     (* drag_data_delete)    (CtkWidget          *widget,
				    CdkDragContext     *context);

  /* Target side drag signals */
  void     (* drag_leave)          (CtkWidget          *widget,
				    CdkDragContext     *context,
				    guint               time_);
  gboolean (* drag_motion)         (CtkWidget	       *widget,
				    CdkDragContext     *context,
				    gint                x,
				    gint                y,
				    guint               time_);
  gboolean (* drag_drop)           (CtkWidget	       *widget,
				    CdkDragContext     *context,
				    gint                x,
				    gint                y,
				    guint               time_);
  void     (* drag_data_received)  (CtkWidget          *widget,
				    CdkDragContext     *context,
				    gint                x,
				    gint                y,
				    CtkSelectionData   *selection_data,
				    guint               info,
				    guint               time_);
  gboolean (* drag_failed)         (CtkWidget          *widget,
                                    CdkDragContext     *context,
                                    CtkDragResult       result);

  /* Signals used only for keybindings */
  gboolean (* popup_menu)          (CtkWidget          *widget);

  /* If a widget has multiple tooltips/whatsthis, it should show the
   * one for the current focus location, or if that doesn't make
   * sense, should cycle through them showing each tip alongside
   * whatever piece of the widget it applies to.
   */
  gboolean (* show_help)           (CtkWidget          *widget,
                                    CtkWidgetHelpType   help_type);

  /* accessibility support
   */
  AtkObject *  (* get_accessible)     (CtkWidget *widget);

  void         (* screen_changed)     (CtkWidget *widget,
                                       CdkScreen *previous_screen);
  gboolean     (* can_activate_accel) (CtkWidget *widget,
                                       guint      signal_id);


  void         (* composited_changed) (CtkWidget *widget);

  gboolean     (* query_tooltip)      (CtkWidget  *widget,
				       gint        x,
				       gint        y,
				       gboolean    keyboard_tooltip,
				       CtkTooltip *tooltip);

  void         (* compute_expand)     (CtkWidget  *widget,
                                       gboolean   *hexpand_p,
                                       gboolean   *vexpand_p);

  void         (* adjust_size_request)    (CtkWidget         *widget,
                                           CtkOrientation     orientation,
                                           gint              *minimum_size,
                                           gint              *natural_size);
  void         (* adjust_size_allocation) (CtkWidget         *widget,
                                           CtkOrientation     orientation,
                                           gint              *minimum_size,
                                           gint              *natural_size,
                                           gint              *allocated_pos,
                                           gint              *allocated_size);

  void         (* style_updated)          (CtkWidget *widget);

  gboolean     (* touch_event)            (CtkWidget     *widget,
                                           CdkEventTouch *event);

  void         (* get_preferred_height_and_baseline_for_width)  (CtkWidget     *widget,
								 gint           width,
								 gint          *minimum_height,
								 gint          *natural_height,
								 gint          *minimum_baseline,
								 gint          *natural_baseline);
  void         (* adjust_baseline_request)(CtkWidget         *widget,
                                           gint              *minimum_baseline,
                                           gint              *natural_baseline);
  void         (* adjust_baseline_allocation) (CtkWidget         *widget,
					       gint              *baseline);
  void         (*queue_draw_region)           (CtkWidget         *widget,
					       const cairo_region_t *region);

  /*< private >*/

  CtkWidgetClassPrivate *priv;

  /* Padding for future expansion */
  void (*_ctk_reserved6) (void);
  void (*_ctk_reserved7) (void);
};


CDK_AVAILABLE_IN_ALL
GType	   ctk_widget_get_type		  (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkWidget* ctk_widget_new		  (GType		type,
					   const gchar	       *first_property_name,
					   ...);
CDK_AVAILABLE_IN_ALL
void	   ctk_widget_destroy		  (CtkWidget	       *widget);
CDK_AVAILABLE_IN_ALL
void	   ctk_widget_destroyed		  (CtkWidget	       *widget,
					   CtkWidget	      **widget_pointer);
CDK_AVAILABLE_IN_ALL
void	   ctk_widget_unparent		  (CtkWidget	       *widget);
CDK_AVAILABLE_IN_ALL
void       ctk_widget_show                (CtkWidget           *widget);
CDK_AVAILABLE_IN_ALL
void       ctk_widget_hide                (CtkWidget           *widget);
CDK_AVAILABLE_IN_ALL
void       ctk_widget_show_now            (CtkWidget           *widget);
CDK_AVAILABLE_IN_ALL
void       ctk_widget_show_all            (CtkWidget           *widget);
CDK_AVAILABLE_IN_ALL
void       ctk_widget_set_no_show_all     (CtkWidget           *widget,
                                           gboolean             no_show_all);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_widget_get_no_show_all     (CtkWidget           *widget);
CDK_AVAILABLE_IN_ALL
void	   ctk_widget_map		  (CtkWidget	       *widget);
CDK_AVAILABLE_IN_ALL
void	   ctk_widget_unmap		  (CtkWidget	       *widget);
CDK_AVAILABLE_IN_ALL
void	   ctk_widget_realize		  (CtkWidget	       *widget);
CDK_AVAILABLE_IN_ALL
void	   ctk_widget_unrealize		  (CtkWidget	       *widget);

CDK_AVAILABLE_IN_ALL
void       ctk_widget_draw                (CtkWidget           *widget,
                                           cairo_t             *cr);
/* Queuing draws */
CDK_AVAILABLE_IN_ALL
void	   ctk_widget_queue_draw	  (CtkWidget	       *widget);
CDK_AVAILABLE_IN_ALL
void	   ctk_widget_queue_draw_area	  (CtkWidget	       *widget,
					   gint                 x,
					   gint                 y,
					   gint                 width,
					   gint                 height);
CDK_AVAILABLE_IN_ALL
void	   ctk_widget_queue_draw_region   (CtkWidget	       *widget,
                                           const cairo_region_t*region);
CDK_AVAILABLE_IN_ALL
void	   ctk_widget_queue_resize	  (CtkWidget	       *widget);
CDK_AVAILABLE_IN_ALL
void	   ctk_widget_queue_resize_no_redraw (CtkWidget *widget);
CDK_AVAILABLE_IN_3_20
void       ctk_widget_queue_allocate      (CtkWidget           *widget);
CDK_AVAILABLE_IN_3_8
CdkFrameClock* ctk_widget_get_frame_clock (CtkWidget           *widget);

CDK_DEPRECATED_IN_3_0_FOR(ctk_widget_get_preferred_size)
void       ctk_widget_size_request        (CtkWidget           *widget,
                                           CtkRequisition      *requisition);
CDK_AVAILABLE_IN_ALL
void	   ctk_widget_size_allocate	  (CtkWidget	       *widget,
					   CtkAllocation       *allocation);
CDK_AVAILABLE_IN_3_10
void	   ctk_widget_size_allocate_with_baseline	  (CtkWidget	       *widget,
							   CtkAllocation       *allocation,
							   gint                 baseline);

CDK_AVAILABLE_IN_ALL
CtkSizeRequestMode  ctk_widget_get_request_mode               (CtkWidget      *widget);
CDK_AVAILABLE_IN_ALL
void                ctk_widget_get_preferred_width            (CtkWidget      *widget,
                                                               gint           *minimum_width,
                                                               gint           *natural_width);
CDK_AVAILABLE_IN_ALL
void                ctk_widget_get_preferred_height_for_width (CtkWidget      *widget,
                                                               gint            width,
                                                               gint           *minimum_height,
                                                               gint           *natural_height);
CDK_AVAILABLE_IN_ALL
void                ctk_widget_get_preferred_height           (CtkWidget      *widget,
                                                               gint           *minimum_height,
                                                               gint           *natural_height);
CDK_AVAILABLE_IN_ALL
void                ctk_widget_get_preferred_width_for_height (CtkWidget      *widget,
                                                               gint            height,
                                                               gint           *minimum_width,
                                                               gint           *natural_width);
CDK_AVAILABLE_IN_3_10
void   ctk_widget_get_preferred_height_and_baseline_for_width (CtkWidget     *widget,
							       gint           width,
							       gint          *minimum_height,
							       gint          *natural_height,
							       gint          *minimum_baseline,
							       gint          *natural_baseline);
CDK_AVAILABLE_IN_ALL
void                ctk_widget_get_preferred_size             (CtkWidget      *widget,
                                                               CtkRequisition *minimum_size,
                                                               CtkRequisition *natural_size);

CDK_DEPRECATED_IN_3_0_FOR(ctk_widget_get_preferred_size)
void       ctk_widget_get_child_requisition (CtkWidget         *widget,
                                             CtkRequisition    *requisition);
CDK_AVAILABLE_IN_ALL
void	   ctk_widget_add_accelerator	  (CtkWidget           *widget,
					   const gchar         *accel_signal,
					   CtkAccelGroup       *accel_group,
					   guint                accel_key,
					   CdkModifierType      accel_mods,
					   CtkAccelFlags        accel_flags);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_widget_remove_accelerator  (CtkWidget           *widget,
					   CtkAccelGroup       *accel_group,
					   guint                accel_key,
					   CdkModifierType      accel_mods);
CDK_AVAILABLE_IN_ALL
void       ctk_widget_set_accel_path      (CtkWidget           *widget,
					   const gchar         *accel_path,
					   CtkAccelGroup       *accel_group);
CDK_AVAILABLE_IN_ALL
GList*     ctk_widget_list_accel_closures (CtkWidget	       *widget);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_widget_can_activate_accel  (CtkWidget           *widget,
                                           guint                signal_id);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_widget_mnemonic_activate   (CtkWidget           *widget,
					   gboolean             group_cycling);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_widget_event		  (CtkWidget	       *widget,
					   CdkEvent	       *event);
CDK_DEPRECATED_IN_3_22
gint       ctk_widget_send_expose         (CtkWidget           *widget,
					   CdkEvent            *event);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_widget_send_focus_change   (CtkWidget           *widget,
                                           CdkEvent            *event);

CDK_AVAILABLE_IN_ALL
gboolean   ctk_widget_activate		     (CtkWidget	       *widget);
     
CDK_DEPRECATED_IN_3_14
void	   ctk_widget_reparent		  (CtkWidget	       *widget,
					   CtkWidget	       *new_parent);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_widget_intersect		  (CtkWidget	       *widget,
					   const CdkRectangle  *area,
					   CdkRectangle	       *intersection);
CDK_DEPRECATED_IN_3_14
cairo_region_t *ctk_widget_region_intersect	  (CtkWidget	       *widget,
					   const cairo_region_t     *region);

CDK_AVAILABLE_IN_ALL
void	ctk_widget_freeze_child_notify	  (CtkWidget	       *widget);
CDK_AVAILABLE_IN_ALL
void	ctk_widget_child_notify		  (CtkWidget	       *widget,
					   const gchar	       *child_property);
CDK_AVAILABLE_IN_ALL
void	ctk_widget_thaw_child_notify	  (CtkWidget	       *widget);

CDK_AVAILABLE_IN_ALL
void       ctk_widget_set_can_focus       (CtkWidget           *widget,
                                           gboolean             can_focus);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_widget_get_can_focus       (CtkWidget           *widget);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_widget_has_focus           (CtkWidget           *widget);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_widget_is_focus            (CtkWidget           *widget);
CDK_AVAILABLE_IN_3_2
gboolean   ctk_widget_has_visible_focus   (CtkWidget           *widget);
CDK_AVAILABLE_IN_ALL
void       ctk_widget_grab_focus          (CtkWidget           *widget);
CDK_AVAILABLE_IN_3_20
void       ctk_widget_set_focus_on_click  (CtkWidget           *widget,
                                           gboolean             focus_on_click);
CDK_AVAILABLE_IN_3_20
gboolean   ctk_widget_get_focus_on_click  (CtkWidget           *widget);

CDK_AVAILABLE_IN_ALL
void       ctk_widget_set_can_default     (CtkWidget           *widget,
                                           gboolean             can_default);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_widget_get_can_default     (CtkWidget           *widget);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_widget_has_default         (CtkWidget           *widget);
CDK_AVAILABLE_IN_ALL
void       ctk_widget_grab_default        (CtkWidget           *widget);

CDK_AVAILABLE_IN_ALL
void      ctk_widget_set_receives_default (CtkWidget           *widget,
                                           gboolean             receives_default);
CDK_AVAILABLE_IN_ALL
gboolean  ctk_widget_get_receives_default (CtkWidget           *widget);

CDK_AVAILABLE_IN_ALL
gboolean   ctk_widget_has_grab            (CtkWidget           *widget);

CDK_AVAILABLE_IN_ALL
gboolean   ctk_widget_device_is_shadowed  (CtkWidget           *widget,
                                           CdkDevice           *device);


CDK_AVAILABLE_IN_ALL
void                  ctk_widget_set_name               (CtkWidget    *widget,
							 const gchar  *name);
CDK_AVAILABLE_IN_ALL
const gchar *         ctk_widget_get_name               (CtkWidget    *widget);

CDK_DEPRECATED_IN_3_0_FOR(ctk_widget_set_state_flags)
void                  ctk_widget_set_state              (CtkWidget    *widget,
							 CtkStateType  state);

CDK_DEPRECATED_IN_3_0_FOR(ctk_widget_get_state_flags)
CtkStateType          ctk_widget_get_state              (CtkWidget    *widget);

CDK_AVAILABLE_IN_ALL
void                  ctk_widget_set_state_flags        (CtkWidget     *widget,
                                                         CtkStateFlags  flags,
                                                         gboolean       clear);
CDK_AVAILABLE_IN_ALL
void                  ctk_widget_unset_state_flags      (CtkWidget     *widget,
                                                         CtkStateFlags  flags);
CDK_AVAILABLE_IN_ALL
CtkStateFlags         ctk_widget_get_state_flags        (CtkWidget     *widget);

CDK_AVAILABLE_IN_ALL
void                  ctk_widget_set_sensitive          (CtkWidget    *widget,
							 gboolean      sensitive);
CDK_AVAILABLE_IN_ALL
gboolean              ctk_widget_get_sensitive          (CtkWidget    *widget);
CDK_AVAILABLE_IN_ALL
gboolean              ctk_widget_is_sensitive           (CtkWidget    *widget);

CDK_AVAILABLE_IN_ALL
void                  ctk_widget_set_visible            (CtkWidget    *widget,
                                                         gboolean      visible);
CDK_AVAILABLE_IN_ALL
gboolean              ctk_widget_get_visible            (CtkWidget    *widget);
CDK_AVAILABLE_IN_ALL
gboolean              ctk_widget_is_visible             (CtkWidget    *widget);

CDK_AVAILABLE_IN_ALL
void                  ctk_widget_set_has_window         (CtkWidget    *widget,
                                                         gboolean      has_window);
CDK_AVAILABLE_IN_ALL
gboolean              ctk_widget_get_has_window         (CtkWidget    *widget);

CDK_AVAILABLE_IN_ALL
gboolean              ctk_widget_is_toplevel            (CtkWidget    *widget);
CDK_AVAILABLE_IN_ALL
gboolean              ctk_widget_is_drawable            (CtkWidget    *widget);
CDK_AVAILABLE_IN_ALL
void                  ctk_widget_set_realized           (CtkWidget    *widget,
                                                         gboolean      realized);
CDK_AVAILABLE_IN_ALL
gboolean              ctk_widget_get_realized           (CtkWidget    *widget);
CDK_AVAILABLE_IN_ALL
void                  ctk_widget_set_mapped             (CtkWidget    *widget,
                                                         gboolean      mapped);
CDK_AVAILABLE_IN_ALL
gboolean              ctk_widget_get_mapped             (CtkWidget    *widget);

CDK_AVAILABLE_IN_ALL
void                  ctk_widget_set_app_paintable      (CtkWidget    *widget,
							 gboolean      app_paintable);
CDK_AVAILABLE_IN_ALL
gboolean              ctk_widget_get_app_paintable      (CtkWidget    *widget);

CDK_DEPRECATED_IN_3_14
void                  ctk_widget_set_double_buffered    (CtkWidget    *widget,
							 gboolean      double_buffered);
CDK_DEPRECATED_IN_3_14
gboolean              ctk_widget_get_double_buffered    (CtkWidget    *widget);

CDK_AVAILABLE_IN_ALL
void                  ctk_widget_set_redraw_on_allocate (CtkWidget    *widget,
							 gboolean      redraw_on_allocate);

CDK_AVAILABLE_IN_ALL
void                  ctk_widget_set_parent             (CtkWidget    *widget,
							 CtkWidget    *parent);
CDK_AVAILABLE_IN_ALL
CtkWidget           * ctk_widget_get_parent             (CtkWidget    *widget);

CDK_AVAILABLE_IN_ALL
void                  ctk_widget_set_parent_window      (CtkWidget    *widget,
							 CdkWindow    *parent_window);
CDK_AVAILABLE_IN_ALL
CdkWindow           * ctk_widget_get_parent_window      (CtkWidget    *widget);

CDK_AVAILABLE_IN_ALL
void                  ctk_widget_set_child_visible      (CtkWidget    *widget,
							 gboolean      is_visible);
CDK_AVAILABLE_IN_ALL
gboolean              ctk_widget_get_child_visible      (CtkWidget    *widget);

CDK_AVAILABLE_IN_ALL
void                  ctk_widget_set_window             (CtkWidget    *widget,
                                                         CdkWindow    *window);
CDK_AVAILABLE_IN_ALL
CdkWindow           * ctk_widget_get_window             (CtkWidget    *widget);
CDK_AVAILABLE_IN_3_8
void                  ctk_widget_register_window        (CtkWidget    *widget,
                                                         CdkWindow    *window);
CDK_AVAILABLE_IN_3_8
void                  ctk_widget_unregister_window      (CtkWidget    *widget,
                                                         CdkWindow    *window);

CDK_AVAILABLE_IN_ALL
int                   ctk_widget_get_allocated_width    (CtkWidget     *widget);
CDK_AVAILABLE_IN_ALL
int                   ctk_widget_get_allocated_height   (CtkWidget     *widget);
CDK_AVAILABLE_IN_3_10
int                   ctk_widget_get_allocated_baseline (CtkWidget     *widget);
CDK_AVAILABLE_IN_3_20
void                  ctk_widget_get_allocated_size     (CtkWidget     *widget,
                                                         CtkAllocation *allocation,
                                                         int           *baseline);

CDK_AVAILABLE_IN_ALL
void                  ctk_widget_get_allocation         (CtkWidget     *widget,
                                                         CtkAllocation *allocation);
CDK_AVAILABLE_IN_ALL
void                  ctk_widget_set_allocation         (CtkWidget     *widget,
                                                         const CtkAllocation *allocation);
CDK_AVAILABLE_IN_3_14
void                  ctk_widget_set_clip               (CtkWidget     *widget,
                                                         const CtkAllocation *clip);
CDK_AVAILABLE_IN_3_14
void                  ctk_widget_get_clip               (CtkWidget     *widget,
                                                         CtkAllocation *clip);

CDK_DEPRECATED_IN_3_0_FOR(ctk_widget_get_preferred_width & ctk_widget_get_preferred_height)

void                  ctk_widget_get_requisition        (CtkWidget     *widget,
                                                         CtkRequisition *requisition);

CDK_AVAILABLE_IN_ALL
gboolean   ctk_widget_child_focus         (CtkWidget           *widget,
                                           CtkDirectionType     direction);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_widget_keynav_failed       (CtkWidget           *widget,
                                           CtkDirectionType     direction);
CDK_AVAILABLE_IN_ALL
void       ctk_widget_error_bell          (CtkWidget           *widget);

CDK_AVAILABLE_IN_ALL
void       ctk_widget_set_size_request    (CtkWidget           *widget,
                                           gint                 width,
                                           gint                 height);
CDK_AVAILABLE_IN_ALL
void       ctk_widget_get_size_request    (CtkWidget           *widget,
                                           gint                *width,
                                           gint                *height);
CDK_AVAILABLE_IN_ALL
void	   ctk_widget_set_events	  (CtkWidget	       *widget,
					   gint			events);
CDK_AVAILABLE_IN_ALL
void       ctk_widget_add_events          (CtkWidget           *widget,
					   gint	                events);
CDK_AVAILABLE_IN_ALL
void	   ctk_widget_set_device_events	  (CtkWidget	       *widget,
                                           CdkDevice           *device,
					   CdkEventMask		events);
CDK_AVAILABLE_IN_ALL
void       ctk_widget_add_device_events   (CtkWidget           *widget,
                                           CdkDevice           *device,
					   CdkEventMask         events);
CDK_AVAILABLE_IN_3_8
void	   ctk_widget_set_opacity	  (CtkWidget	       *widget,
					   double		opacity);
CDK_AVAILABLE_IN_3_8
double	   ctk_widget_get_opacity	  (CtkWidget	       *widget);

CDK_AVAILABLE_IN_ALL
void       ctk_widget_set_device_enabled  (CtkWidget    *widget,
                                           CdkDevice    *device,
                                           gboolean      enabled);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_widget_get_device_enabled  (CtkWidget    *widget,
                                           CdkDevice    *device);

CDK_AVAILABLE_IN_ALL
CtkWidget*   ctk_widget_get_toplevel	(CtkWidget	*widget);
CDK_AVAILABLE_IN_ALL
CtkWidget*   ctk_widget_get_ancestor	(CtkWidget	*widget,
					 GType		 widget_type);
CDK_AVAILABLE_IN_ALL
CdkVisual*   ctk_widget_get_visual	(CtkWidget	*widget);
CDK_AVAILABLE_IN_ALL
void         ctk_widget_set_visual	(CtkWidget	*widget,
                                         CdkVisual      *visual);

CDK_AVAILABLE_IN_ALL
CdkScreen *   ctk_widget_get_screen      (CtkWidget *widget);
CDK_AVAILABLE_IN_ALL
gboolean      ctk_widget_has_screen      (CtkWidget *widget);
CDK_AVAILABLE_IN_3_10
gint          ctk_widget_get_scale_factor (CtkWidget *widget);
CDK_AVAILABLE_IN_ALL
CdkDisplay *  ctk_widget_get_display     (CtkWidget *widget);
CDK_DEPRECATED_IN_3_12
CdkWindow *   ctk_widget_get_root_window (CtkWidget *widget);
CDK_AVAILABLE_IN_ALL
CtkSettings*  ctk_widget_get_settings    (CtkWidget *widget);
CDK_AVAILABLE_IN_ALL
CtkClipboard *ctk_widget_get_clipboard   (CtkWidget *widget,
					  CdkAtom    selection);


/* Expand flags and related support */
CDK_AVAILABLE_IN_ALL
gboolean ctk_widget_get_hexpand          (CtkWidget      *widget);
CDK_AVAILABLE_IN_ALL
void     ctk_widget_set_hexpand          (CtkWidget      *widget,
                                          gboolean        expand);
CDK_AVAILABLE_IN_ALL
gboolean ctk_widget_get_hexpand_set      (CtkWidget      *widget);
CDK_AVAILABLE_IN_ALL
void     ctk_widget_set_hexpand_set      (CtkWidget      *widget,
                                          gboolean        set);
CDK_AVAILABLE_IN_ALL
gboolean ctk_widget_get_vexpand          (CtkWidget      *widget);
CDK_AVAILABLE_IN_ALL
void     ctk_widget_set_vexpand          (CtkWidget      *widget,
                                          gboolean        expand);
CDK_AVAILABLE_IN_ALL
gboolean ctk_widget_get_vexpand_set      (CtkWidget      *widget);
CDK_AVAILABLE_IN_ALL
void     ctk_widget_set_vexpand_set      (CtkWidget      *widget,
                                          gboolean        set);
CDK_AVAILABLE_IN_ALL
void     ctk_widget_queue_compute_expand (CtkWidget      *widget);
CDK_AVAILABLE_IN_ALL
gboolean ctk_widget_compute_expand       (CtkWidget      *widget,
                                          CtkOrientation  orientation);


/* Multidevice support */
CDK_AVAILABLE_IN_ALL
gboolean         ctk_widget_get_support_multidevice (CtkWidget      *widget);
CDK_AVAILABLE_IN_ALL
void             ctk_widget_set_support_multidevice (CtkWidget      *widget,
                                                     gboolean        support_multidevice);

/* Accessibility support */
CDK_AVAILABLE_IN_3_2
void             ctk_widget_class_set_accessible_type    (CtkWidgetClass     *widget_class,
                                                          GType               type);
CDK_AVAILABLE_IN_3_2
void             ctk_widget_class_set_accessible_role    (CtkWidgetClass     *widget_class,
                                                          AtkRole             role);
CDK_AVAILABLE_IN_ALL
AtkObject*       ctk_widget_get_accessible               (CtkWidget          *widget);


/* Margin and alignment */
CDK_AVAILABLE_IN_ALL
CtkAlign ctk_widget_get_halign        (CtkWidget *widget);
CDK_AVAILABLE_IN_ALL
void     ctk_widget_set_halign        (CtkWidget *widget,
                                       CtkAlign   align);
CDK_AVAILABLE_IN_ALL
CtkAlign ctk_widget_get_valign        (CtkWidget *widget);
CDK_AVAILABLE_IN_3_10
CtkAlign ctk_widget_get_valign_with_baseline (CtkWidget *widget);
CDK_AVAILABLE_IN_ALL
void     ctk_widget_set_valign        (CtkWidget *widget,
                                       CtkAlign   align);
CDK_DEPRECATED_IN_3_12_FOR(ctk_widget_get_margin_start)
gint     ctk_widget_get_margin_left   (CtkWidget *widget);
CDK_DEPRECATED_IN_3_12_FOR(ctk_widget_set_margin_start)
void     ctk_widget_set_margin_left   (CtkWidget *widget,
                                       gint       margin);
CDK_DEPRECATED_IN_3_12_FOR(ctk_widget_get_margin_end)
gint     ctk_widget_get_margin_right  (CtkWidget *widget);
CDK_DEPRECATED_IN_3_12_FOR(ctk_widget_set_margin_end)
void     ctk_widget_set_margin_right  (CtkWidget *widget,
                                       gint       margin);
CDK_AVAILABLE_IN_3_12
gint     ctk_widget_get_margin_start  (CtkWidget *widget);
CDK_AVAILABLE_IN_3_12
void     ctk_widget_set_margin_start  (CtkWidget *widget,
                                       gint       margin);
CDK_AVAILABLE_IN_3_12
gint     ctk_widget_get_margin_end    (CtkWidget *widget);
CDK_AVAILABLE_IN_3_12
void     ctk_widget_set_margin_end    (CtkWidget *widget,
                                       gint       margin);
CDK_AVAILABLE_IN_ALL
gint     ctk_widget_get_margin_top    (CtkWidget *widget);
CDK_AVAILABLE_IN_ALL
void     ctk_widget_set_margin_top    (CtkWidget *widget,
                                       gint       margin);
CDK_AVAILABLE_IN_ALL
gint     ctk_widget_get_margin_bottom (CtkWidget *widget);
CDK_AVAILABLE_IN_ALL
void     ctk_widget_set_margin_bottom (CtkWidget *widget,
                                       gint       margin);


CDK_AVAILABLE_IN_ALL
gint	     ctk_widget_get_events	(CtkWidget	*widget);
CDK_AVAILABLE_IN_ALL
CdkEventMask ctk_widget_get_device_events (CtkWidget	*widget,
                                           CdkDevice    *device);
CDK_DEPRECATED_IN_3_4_FOR(cdk_window_get_device_position)
void	     ctk_widget_get_pointer	(CtkWidget	*widget,
					 gint		*x,
					 gint		*y);

CDK_AVAILABLE_IN_ALL
gboolean     ctk_widget_is_ancestor	(CtkWidget	*widget,
					 CtkWidget	*ancestor);

CDK_AVAILABLE_IN_ALL
gboolean     ctk_widget_translate_coordinates (CtkWidget  *src_widget,
					       CtkWidget  *dest_widget,
					       gint        src_x,
					       gint        src_y,
					       gint       *dest_x,
					       gint       *dest_y);

/* Hide widget and return TRUE.
 */
CDK_AVAILABLE_IN_ALL
gboolean     ctk_widget_hide_on_delete	(CtkWidget	*widget);

/* Functions to override widget styling */
CDK_DEPRECATED_IN_3_16
void         ctk_widget_override_color            (CtkWidget     *widget,
                                                   CtkStateFlags  state,
                                                   const CdkRGBA *color);
CDK_DEPRECATED_IN_3_16
void         ctk_widget_override_background_color (CtkWidget     *widget,
                                                   CtkStateFlags  state,
                                                   const CdkRGBA *color);

CDK_DEPRECATED_IN_3_16
void         ctk_widget_override_font             (CtkWidget                  *widget,
                                                   const PangoFontDescription *font_desc);

CDK_DEPRECATED_IN_3_16
void         ctk_widget_override_symbolic_color   (CtkWidget     *widget,
                                                   const gchar   *name,
                                                   const CdkRGBA *color);
CDK_DEPRECATED_IN_3_16
void         ctk_widget_override_cursor           (CtkWidget       *widget,
                                                   const CdkRGBA   *cursor,
                                                   const CdkRGBA   *secondary_cursor);

CDK_AVAILABLE_IN_ALL
void       ctk_widget_reset_style       (CtkWidget      *widget);

CDK_AVAILABLE_IN_ALL
PangoContext *ctk_widget_create_pango_context (CtkWidget   *widget);
CDK_AVAILABLE_IN_ALL
PangoContext *ctk_widget_get_pango_context    (CtkWidget   *widget);
CDK_AVAILABLE_IN_3_18
void ctk_widget_set_font_options (CtkWidget                  *widget,
                                  const cairo_font_options_t *options);
CDK_AVAILABLE_IN_3_18
const cairo_font_options_t *ctk_widget_get_font_options (CtkWidget *widget);
CDK_AVAILABLE_IN_ALL
PangoLayout  *ctk_widget_create_pango_layout  (CtkWidget   *widget,
					       const gchar *text);

CDK_DEPRECATED_IN_3_10_FOR(ctk_icon_theme_load_icon)
GdkPixbuf    *ctk_widget_render_icon_pixbuf   (CtkWidget   *widget,
                                               const gchar *stock_id,
                                               CtkIconSize  size);

/* handle composite names for CTK_COMPOSITE_CHILD widgets,
 * the returned name is newly allocated.
 */
CDK_DEPRECATED_IN_3_10_FOR(ctk_widget_class_set_template)
void   ctk_widget_set_composite_name	(CtkWidget	*widget,
					 const gchar   	*name);
CDK_DEPRECATED_IN_3_10_FOR(ctk_widget_class_set_template)
gchar* ctk_widget_get_composite_name	(CtkWidget	*widget);
     
/* Push/pop pairs, to change default values upon a widget's creation.
 * This will override the values that got set by the
 * ctk_widget_set_default_* () functions.
 */
CDK_DEPRECATED_IN_3_10_FOR(ctk_widget_class_set_template)
void	     ctk_widget_push_composite_child (void);
CDK_DEPRECATED_IN_3_10_FOR(ctk_widget_class_set_template)
void	     ctk_widget_pop_composite_child  (void);

/* widget style properties
 */
CDK_AVAILABLE_IN_ALL
void ctk_widget_class_install_style_property        (CtkWidgetClass     *klass,
						     GParamSpec         *pspec);
CDK_AVAILABLE_IN_ALL
void ctk_widget_class_install_style_property_parser (CtkWidgetClass     *klass,
						     GParamSpec         *pspec,
						     CtkRcPropertyParser parser);
CDK_AVAILABLE_IN_ALL
GParamSpec*  ctk_widget_class_find_style_property   (CtkWidgetClass     *klass,
						     const gchar        *property_name);
CDK_AVAILABLE_IN_ALL
GParamSpec** ctk_widget_class_list_style_properties (CtkWidgetClass     *klass,
						     guint              *n_properties);
CDK_AVAILABLE_IN_ALL
void ctk_widget_style_get_property (CtkWidget	     *widget,
				    const gchar    *property_name,
				    GValue	     *value);
CDK_AVAILABLE_IN_ALL
void ctk_widget_style_get_valist   (CtkWidget	     *widget,
				    const gchar    *first_property_name,
				    va_list         var_args);
CDK_AVAILABLE_IN_ALL
void ctk_widget_style_get          (CtkWidget	     *widget,
				    const gchar    *first_property_name,
				    ...) G_GNUC_NULL_TERMINATED;

/* Functions for setting directionality for widgets */

CDK_AVAILABLE_IN_ALL
void             ctk_widget_set_direction         (CtkWidget        *widget,
						   CtkTextDirection  dir);
CDK_AVAILABLE_IN_ALL
CtkTextDirection ctk_widget_get_direction         (CtkWidget        *widget);

CDK_AVAILABLE_IN_ALL
void             ctk_widget_set_default_direction (CtkTextDirection  dir);
CDK_AVAILABLE_IN_ALL
CtkTextDirection ctk_widget_get_default_direction (void);

/* Compositing manager functionality */
CDK_DEPRECATED_IN_3_22_FOR(cdk_screen_is_composited)
gboolean ctk_widget_is_composited (CtkWidget *widget);

/* Counterpart to cdk_window_shape_combine_region.
 */
CDK_AVAILABLE_IN_ALL
void	     ctk_widget_shape_combine_region (CtkWidget *widget,
                                              cairo_region_t *region);
CDK_AVAILABLE_IN_ALL
void	     ctk_widget_input_shape_combine_region (CtkWidget *widget,
                                                    cairo_region_t *region);

CDK_AVAILABLE_IN_ALL
GList* ctk_widget_list_mnemonic_labels  (CtkWidget *widget);
CDK_AVAILABLE_IN_ALL
void   ctk_widget_add_mnemonic_label    (CtkWidget *widget,
					 CtkWidget *label);
CDK_AVAILABLE_IN_ALL
void   ctk_widget_remove_mnemonic_label (CtkWidget *widget,
					 CtkWidget *label);

CDK_AVAILABLE_IN_ALL
void                  ctk_widget_set_tooltip_window    (CtkWidget   *widget,
                                                        CtkWindow   *custom_window);
CDK_AVAILABLE_IN_ALL
CtkWindow *ctk_widget_get_tooltip_window    (CtkWidget   *widget);
CDK_AVAILABLE_IN_ALL
void       ctk_widget_trigger_tooltip_query (CtkWidget   *widget);
CDK_AVAILABLE_IN_ALL
void       ctk_widget_set_tooltip_text      (CtkWidget   *widget,
                                             const gchar *text);
CDK_AVAILABLE_IN_ALL
gchar *    ctk_widget_get_tooltip_text      (CtkWidget   *widget);
CDK_AVAILABLE_IN_ALL
void       ctk_widget_set_tooltip_markup    (CtkWidget   *widget,
                                             const gchar *markup);
CDK_AVAILABLE_IN_ALL
gchar *    ctk_widget_get_tooltip_markup    (CtkWidget   *widget);
CDK_AVAILABLE_IN_ALL
void       ctk_widget_set_has_tooltip       (CtkWidget   *widget,
					     gboolean     has_tooltip);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_widget_get_has_tooltip       (CtkWidget   *widget);

CDK_AVAILABLE_IN_ALL
gboolean   ctk_cairo_should_draw_window     (cairo_t     *cr,
                                             CdkWindow   *window);
CDK_AVAILABLE_IN_ALL
void       ctk_cairo_transform_to_window    (cairo_t     *cr,
                                             CtkWidget   *widget,
                                             CdkWindow   *window);

CDK_AVAILABLE_IN_ALL
GType           ctk_requisition_get_type (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkRequisition *ctk_requisition_new      (void) G_GNUC_MALLOC;
CDK_AVAILABLE_IN_ALL
CtkRequisition *ctk_requisition_copy     (const CtkRequisition *requisition);
CDK_AVAILABLE_IN_ALL
void            ctk_requisition_free     (CtkRequisition       *requisition);

CDK_AVAILABLE_IN_ALL
gboolean     ctk_widget_in_destruction (CtkWidget *widget);

CDK_AVAILABLE_IN_ALL
CtkStyleContext * ctk_widget_get_style_context (CtkWidget *widget);

CDK_AVAILABLE_IN_ALL
CtkWidgetPath *   ctk_widget_get_path (CtkWidget *widget);

CDK_AVAILABLE_IN_3_20
void              ctk_widget_class_set_css_name (CtkWidgetClass *widget_class,
                                                 const char     *name);
CDK_AVAILABLE_IN_3_20
const char *      ctk_widget_class_get_css_name (CtkWidgetClass *widget_class);

CDK_AVAILABLE_IN_3_4
CdkModifierType   ctk_widget_get_modifier_mask (CtkWidget         *widget,
                                                CdkModifierIntent  intent);

CDK_AVAILABLE_IN_3_6
void                    ctk_widget_insert_action_group                  (CtkWidget    *widget,
                                                                         const gchar  *name,
                                                                         GActionGroup *group);



CDK_AVAILABLE_IN_3_8
guint ctk_widget_add_tick_callback (CtkWidget       *widget,
                                    CtkTickCallback  callback,
                                    gpointer         user_data,
                                    GDestroyNotify   notify);

CDK_AVAILABLE_IN_3_8
void ctk_widget_remove_tick_callback (CtkWidget       *widget,
                                      guint            id);

/**
 * ctk_widget_class_bind_template_callback:
 * @widget_class: a #CtkWidgetClass
 * @callback: the callback symbol
 *
 * Binds a callback function defined in a template to the @widget_class.
 *
 * This macro is a convenience wrapper around the
 * ctk_widget_class_bind_template_callback_full() function.
 *
 * Since: 3.10
 */
#define ctk_widget_class_bind_template_callback(widget_class, callback) \
  ctk_widget_class_bind_template_callback_full (CTK_WIDGET_CLASS (widget_class), \
                                                #callback, \
                                                G_CALLBACK (callback))

/**
 * ctk_widget_class_bind_template_child:
 * @widget_class: a #CtkWidgetClass
 * @TypeName: the type name of this widget
 * @member_name: name of the instance member in the instance struct for @data_type
 *
 * Binds a child widget defined in a template to the @widget_class.
 *
 * This macro is a convenience wrapper around the
 * ctk_widget_class_bind_template_child_full() function.
 *
 * This macro will use the offset of the @member_name inside the @TypeName
 * instance structure.
 *
 * Since: 3.10
 */
#define ctk_widget_class_bind_template_child(widget_class, TypeName, member_name) \
  ctk_widget_class_bind_template_child_full (widget_class, \
                                             #member_name, \
                                             FALSE, \
                                             G_STRUCT_OFFSET (TypeName, member_name))

/**
 * ctk_widget_class_bind_template_child_internal:
 * @widget_class: a #CtkWidgetClass
 * @TypeName: the type name, in CamelCase
 * @member_name: name of the instance member in the instance struct for @data_type
 *
 * Binds a child widget defined in a template to the @widget_class, and
 * also makes it available as an internal child in CtkBuilder, under the
 * name @member_name.
 *
 * This macro is a convenience wrapper around the
 * ctk_widget_class_bind_template_child_full() function.
 *
 * This macro will use the offset of the @member_name inside the @TypeName
 * instance structure.
 *
 * Since: 3.10
 */
#define ctk_widget_class_bind_template_child_internal(widget_class, TypeName, member_name) \
  ctk_widget_class_bind_template_child_full (widget_class, \
                                             #member_name, \
                                             TRUE, \
                                             G_STRUCT_OFFSET (TypeName, member_name))

/**
 * ctk_widget_class_bind_template_child_private:
 * @widget_class: a #CtkWidgetClass
 * @TypeName: the type name of this widget
 * @member_name: name of the instance private member in the private struct for @data_type
 *
 * Binds a child widget defined in a template to the @widget_class.
 *
 * This macro is a convenience wrapper around the
 * ctk_widget_class_bind_template_child_full() function.
 *
 * This macro will use the offset of the @member_name inside the @TypeName
 * private data structure (it uses G_PRIVATE_OFFSET(), so the private struct
 * must be added with G_ADD_PRIVATE()).
 *
 * Since: 3.10
 */
#define ctk_widget_class_bind_template_child_private(widget_class, TypeName, member_name) \
  ctk_widget_class_bind_template_child_full (widget_class, \
                                             #member_name, \
                                             FALSE, \
                                             G_PRIVATE_OFFSET (TypeName, member_name))

/**
 * ctk_widget_class_bind_template_child_internal_private:
 * @widget_class: a #CtkWidgetClass
 * @TypeName: the type name, in CamelCase
 * @member_name: name of the instance private member on the private struct for @data_type
 *
 * Binds a child widget defined in a template to the @widget_class, and
 * also makes it available as an internal child in CtkBuilder, under the
 * name @member_name.
 *
 * This macro is a convenience wrapper around the
 * ctk_widget_class_bind_template_child_full() function.
 *
 * This macro will use the offset of the @member_name inside the @TypeName
 * private data structure.
 *
 * Since: 3.10
 */
#define ctk_widget_class_bind_template_child_internal_private(widget_class, TypeName, member_name) \
  ctk_widget_class_bind_template_child_full (widget_class, \
                                             #member_name, \
                                             TRUE, \
                                             G_PRIVATE_OFFSET (TypeName, member_name))

CDK_AVAILABLE_IN_3_10
void    ctk_widget_init_template                        (CtkWidget             *widget);
CDK_AVAILABLE_IN_3_10
GObject *ctk_widget_get_template_child                  (CtkWidget             *widget,
						         GType                  widget_type,
						         const gchar           *name);
CDK_AVAILABLE_IN_3_10
void    ctk_widget_class_set_template                   (CtkWidgetClass        *widget_class,
						         GBytes                *template_bytes);
CDK_AVAILABLE_IN_3_10
void    ctk_widget_class_set_template_from_resource     (CtkWidgetClass        *widget_class,
						         const gchar           *resource_name);
CDK_AVAILABLE_IN_3_10
void    ctk_widget_class_bind_template_callback_full    (CtkWidgetClass        *widget_class,
						         const gchar           *callback_name,
						         GCallback              callback_symbol);
CDK_AVAILABLE_IN_3_10
void    ctk_widget_class_set_connect_func               (CtkWidgetClass        *widget_class,
						         CtkBuilderConnectFunc  connect_func,
						         gpointer               connect_data,
						         GDestroyNotify         connect_data_destroy);
CDK_AVAILABLE_IN_3_10
void    ctk_widget_class_bind_template_child_full       (CtkWidgetClass        *widget_class,
						         const gchar           *name,
						         gboolean               internal_child,
						         gssize                 struct_offset);

CDK_AVAILABLE_IN_3_16
GActionGroup           *ctk_widget_get_action_group     (CtkWidget             *widget,
                                                         const gchar           *prefix);

CDK_AVAILABLE_IN_3_16
const gchar **          ctk_widget_list_action_prefixes (CtkWidget             *widget);

CDK_AVAILABLE_IN_3_18
void                    ctk_widget_set_font_map         (CtkWidget             *widget,
                                                         PangoFontMap          *font_map);
CDK_AVAILABLE_IN_3_18
PangoFontMap *          ctk_widget_get_font_map         (CtkWidget             *widget);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkWidget, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkRequisition, ctk_requisition_free)

G_END_DECLS

#endif /* __CTK_WIDGET_H__ */
