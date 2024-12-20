/* CTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 1998 Elliot Lee
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

#include <stdlib.h>

#define CDK_DISABLE_DEPRECATION_WARNINGS

#include "ctkhandlebox.h"
#include "ctkinvisible.h"
#include "ctkmain.h"
#include "ctkmarshalers.h"
#include "ctkrender.h"
#include "ctkwindow.h"
#include "ctktypebuiltins.h"
#include "ctkprivate.h"
#include "ctkintl.h"


/**
 * SECTION:ctkhandlebox
 * @Short_description: a widget for detachable window portions
 * @Title: CtkHandleBox
 *
 * The #CtkHandleBox widget allows a portion of a window to be "torn
 * off". It is a bin widget which displays its child and a handle that
 * the user can drag to tear off a separate window (the “float
 * window”) containing the child widget. A thin
 * “ghost” is drawn in the original location of the
 * handlebox. By dragging the separate window back to its original
 * location, it can be reattached.
 *
 * When reattaching, the ghost and float window, must be aligned
 * along one of the edges, the “snap edge”.
 * This either can be specified by the application programmer
 * explicitly, or CTK+ will pick a reasonable default based
 * on the handle position.
 *
 * To make detaching and reattaching the handlebox as minimally confusing
 * as possible to the user, it is important to set the snap edge so that
 * the snap edge does not move when the handlebox is deattached. For
 * instance, if the handlebox is packed at the bottom of a VBox, then
 * when the handlebox is detached, the bottom edge of the handlebox's
 * allocation will remain fixed as the height of the handlebox shrinks,
 * so the snap edge should be set to %CTK_POS_BOTTOM.
 *
 * > #CtkHandleBox has been deprecated. It is very specialized, lacks features
 * > to make it useful and most importantly does not fit well into modern
 * > application design. Do not use it. There is no replacement.
 */


struct _CtkHandleBoxPrivate
{
  /* Properties */
  CtkPositionType handle_position;
  gint snap_edge;
  CtkShadowType   shadow_type;
  gboolean        child_detached;
  /* Properties */

  CtkAllocation   attach_allocation;
  CtkAllocation   float_allocation;

  CdkDevice      *grab_device;

  CdkWindow      *bin_window;     /* parent window for children */
  CdkWindow      *float_window;

  /* Variables used during a drag
   */
  gint            orig_x;
  gint            orig_y;

  guint           float_window_mapped : 1;
  guint           in_drag : 1;
  guint           shrink_on_detach : 1;
};

enum {
  PROP_0,
  PROP_SHADOW_TYPE,
  PROP_HANDLE_POSITION,
  PROP_SNAP_EDGE,
  PROP_SNAP_EDGE_SET,
  PROP_CHILD_DETACHED
};

#define DRAG_HANDLE_SIZE 10
#define CHILDLESS_SIZE	25
#define GHOST_HEIGHT 3
#define TOLERANCE 5

enum {
  SIGNAL_CHILD_ATTACHED,
  SIGNAL_CHILD_DETACHED,
  SIGNAL_LAST
};

/* The algorithm for docking and redocking implemented here
 * has a couple of nice properties:
 *
 * 1) During a single drag, docking always occurs at the
 *    the same cursor position. This means that the users
 *    motions are reversible, and that you won't
 *    undock/dock oscillations.
 *
 * 2) Docking generally occurs at user-visible features.
 *    The user, once they figure out to redock, will
 *    have useful information about doing it again in
 *    the future.
 *
 * Please try to preserve these properties if you
 * change the algorithm. (And the current algorithm
 * is far from ideal). Briefly, the current algorithm
 * for deciding whether the handlebox is docked or not:
 *
 * 1) The decision is done by comparing two rectangles - the
 *    allocation if the widget at the start of the drag,
 *    and the boundary of hb->bin_window at the start of
 *    of the drag offset by the distance that the cursor
 *    has moved.
 *
 * 2) These rectangles must have one edge, the “snap_edge”
 *    of the handlebox, aligned within TOLERANCE.
 * 
 * 3) On the other dimension, the extents of one rectangle
 *    must be contained in the extents of the other,
 *    extended by tolerance. That is, either we can have:
 *
 * <-TOLERANCE-|--------bin_window--------------|-TOLERANCE->
 *         <--------float_window-------------------->
 *
 * or we can have:
 *
 * <-TOLERANCE-|------float_window--------------|-TOLERANCE->
 *          <--------bin_window-------------------->
 */

static void     ctk_handle_box_set_property  (GObject        *object,
                                              guint           param_id,
                                              const GValue   *value,
                                              GParamSpec     *pspec);
static void     ctk_handle_box_get_property  (GObject        *object,
                                              guint           param_id,
                                              GValue         *value,
                                              GParamSpec     *pspec);
static void     ctk_handle_box_map           (CtkWidget      *widget);
static void     ctk_handle_box_unmap         (CtkWidget      *widget);
static void     ctk_handle_box_realize       (CtkWidget      *widget);
static void     ctk_handle_box_unrealize     (CtkWidget      *widget);
static void     ctk_handle_box_style_updated (CtkWidget      *widget);
static void     ctk_handle_box_size_request  (CtkWidget      *widget,
                                              CtkRequisition *requisition);
static void     ctk_handle_box_get_preferred_width (CtkWidget *widget,
						    gint      *minimum,
						    gint      *natural);
static void     ctk_handle_box_get_preferred_height (CtkWidget *widget,
						    gint      *minimum,
						    gint      *natural);
static void     ctk_handle_box_size_allocate (CtkWidget      *widget,
                                              CtkAllocation  *real_allocation);
static void     ctk_handle_box_add           (CtkContainer   *container,
                                              CtkWidget      *widget);
static void     ctk_handle_box_remove        (CtkContainer   *container,
                                              CtkWidget      *widget);
static gboolean ctk_handle_box_draw          (CtkWidget      *widget,
                                              cairo_t        *cr);
static gboolean ctk_handle_box_button_press  (CtkWidget      *widget,
                                              CdkEventButton *event);
static gboolean ctk_handle_box_motion        (CtkWidget      *widget,
                                              CdkEventMotion *event);
static gboolean ctk_handle_box_delete_event  (CtkWidget      *widget,
                                              CdkEventAny    *event);
static void     ctk_handle_box_reattach      (CtkHandleBox   *hb);
static void     ctk_handle_box_end_drag      (CtkHandleBox   *hb,
                                              guint32         time);

static guint handle_box_signals[SIGNAL_LAST] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (CtkHandleBox, ctk_handle_box, CTK_TYPE_BIN)

static void
ctk_handle_box_class_init (CtkHandleBoxClass *class)
{
  GObjectClass *gobject_class;
  CtkWidgetClass *widget_class;
  CtkContainerClass *container_class;

  gobject_class = (GObjectClass *) class;
  widget_class = (CtkWidgetClass *) class;
  container_class = (CtkContainerClass *) class;

  gobject_class->set_property = ctk_handle_box_set_property;
  gobject_class->get_property = ctk_handle_box_get_property;
  
  g_object_class_install_property (gobject_class,
                                   PROP_SHADOW_TYPE,
                                   g_param_spec_enum ("shadow-type",
                                                      P_("Shadow type"),
                                                      P_("Appearance of the shadow that surrounds the container"),
						      CTK_TYPE_SHADOW_TYPE,
						      CTK_SHADOW_OUT,
                                                      CTK_PARAM_READWRITE));
  
  g_object_class_install_property (gobject_class,
                                   PROP_HANDLE_POSITION,
                                   g_param_spec_enum ("handle-position",
                                                      P_("Handle position"),
                                                      P_("Position of the handle relative to the child widget"),
						      CTK_TYPE_POSITION_TYPE,
						      CTK_POS_LEFT,
                                                      CTK_PARAM_READWRITE));
  
  g_object_class_install_property (gobject_class,
                                   PROP_SNAP_EDGE,
                                   g_param_spec_enum ("snap-edge",
                                                      P_("Snap edge"),
                                                      P_("Side of the handlebox that's lined up with the docking point to dock the handlebox"),
						      CTK_TYPE_POSITION_TYPE,
						      CTK_POS_TOP,
                                                      CTK_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_SNAP_EDGE_SET,
                                   g_param_spec_boolean ("snap-edge-set",
							 P_("Snap edge set"),
							 P_("Whether to use the value from the snap_edge property or a value derived from handle_position"),
							 FALSE,
							 CTK_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_CHILD_DETACHED,
                                   g_param_spec_boolean ("child-detached",
							 P_("Child Detached"),
							 P_("A boolean value indicating whether the handlebox's child is attached or detached."),
							 FALSE,
							 CTK_PARAM_READABLE));

  widget_class->map = ctk_handle_box_map;
  widget_class->unmap = ctk_handle_box_unmap;
  widget_class->realize = ctk_handle_box_realize;
  widget_class->unrealize = ctk_handle_box_unrealize;
  widget_class->style_updated = ctk_handle_box_style_updated;
  widget_class->get_preferred_width = ctk_handle_box_get_preferred_width;
  widget_class->get_preferred_height = ctk_handle_box_get_preferred_height;
  widget_class->size_allocate = ctk_handle_box_size_allocate;
  widget_class->draw = ctk_handle_box_draw;
  widget_class->button_press_event = ctk_handle_box_button_press;
  widget_class->delete_event = ctk_handle_box_delete_event;

  container_class->add = ctk_handle_box_add;
  container_class->remove = ctk_handle_box_remove;

  class->child_attached = NULL;
  class->child_detached = NULL;

  /**
   * CtkHandleBox::child-attached:
   * @handlebox: the object which received the signal.
   * @widget: the child widget of the handlebox.
   *   (this argument provides no extra information
   *   and is here only for backwards-compatibility)
   *
   * This signal is emitted when the contents of the
   * handlebox are reattached to the main window.
   *
   * Deprecated: 3.4: #CtkHandleBox has been deprecated.
   */
  handle_box_signals[SIGNAL_CHILD_ATTACHED] =
    g_signal_new (I_("child-attached"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (CtkHandleBoxClass, child_attached),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1,
		  CTK_TYPE_WIDGET);

  /**
   * CtkHandleBox::child-detached:
   * @handlebox: the object which received the signal.
   * @widget: the child widget of the handlebox.
   *   (this argument provides no extra information
   *   and is here only for backwards-compatibility)
   *
   * This signal is emitted when the contents of the
   * handlebox are detached from the main window.
   *
   * Deprecated: 3.4: #CtkHandleBox has been deprecated.
   */
  handle_box_signals[SIGNAL_CHILD_DETACHED] =
    g_signal_new (I_("child-detached"),
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (CtkHandleBoxClass, child_detached),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1,
		  CTK_TYPE_WIDGET);
}

static void
ctk_handle_box_init (CtkHandleBox *handle_box)
{
  CtkHandleBoxPrivate *priv;
  CtkStyleContext *context;

  handle_box->priv = ctk_handle_box_get_instance_private (handle_box);
  priv = handle_box->priv;

  ctk_widget_set_has_window (CTK_WIDGET (handle_box), TRUE);

  priv->bin_window = NULL;
  priv->float_window = NULL;
  priv->shadow_type = CTK_SHADOW_OUT;
  priv->handle_position = CTK_POS_LEFT;
  priv->float_window_mapped = FALSE;
  priv->child_detached = FALSE;
  priv->in_drag = FALSE;
  priv->shrink_on_detach = TRUE;
  priv->snap_edge = -1;

  context = ctk_widget_get_style_context (CTK_WIDGET (handle_box));
  ctk_style_context_add_class (context, CTK_STYLE_CLASS_DOCK);
}

static void 
ctk_handle_box_set_property (GObject         *object,
			     guint            prop_id,
			     const GValue    *value,
			     GParamSpec      *pspec)
{
  CtkHandleBox *handle_box = CTK_HANDLE_BOX (object);

  switch (prop_id)
    {
    case PROP_SHADOW_TYPE:
      ctk_handle_box_set_shadow_type (handle_box, g_value_get_enum (value));
      break;
    case PROP_HANDLE_POSITION:
      ctk_handle_box_set_handle_position (handle_box, g_value_get_enum (value));
      break;
    case PROP_SNAP_EDGE:
      ctk_handle_box_set_snap_edge (handle_box, g_value_get_enum (value));
      break;
    case PROP_SNAP_EDGE_SET:
      if (!g_value_get_boolean (value))
	ctk_handle_box_set_snap_edge (handle_box, (CtkPositionType)-1);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void 
ctk_handle_box_get_property (GObject         *object,
			     guint            prop_id,
			     GValue          *value,
			     GParamSpec      *pspec)
{
  CtkHandleBox *handle_box = CTK_HANDLE_BOX (object);
  CtkHandleBoxPrivate *priv = handle_box->priv;

  switch (prop_id)
    {
    case PROP_SHADOW_TYPE:
      g_value_set_enum (value, priv->shadow_type);
      break;
    case PROP_HANDLE_POSITION:
      g_value_set_enum (value, priv->handle_position);
      break;
    case PROP_SNAP_EDGE:
      g_value_set_enum (value,
			(priv->snap_edge == -1 ?
			 CTK_POS_TOP : priv->snap_edge));
      break;
    case PROP_SNAP_EDGE_SET:
      g_value_set_boolean (value, priv->snap_edge != -1);
      break;
    case PROP_CHILD_DETACHED:
      g_value_set_boolean (value, priv->child_detached);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * ctk_handle_box_new:
 *
 * Create a new handle box.
 *
 * Returns: a new #CtkHandleBox.
 *
 * Deprecated: 3.4: #CtkHandleBox has been deprecated.
 */
CtkWidget*
ctk_handle_box_new (void)
{
  return g_object_new (CTK_TYPE_HANDLE_BOX, NULL);
}

static void
ctk_handle_box_map (CtkWidget *widget)
{
  CtkHandleBox *hb = CTK_HANDLE_BOX (widget);
  CtkHandleBoxPrivate *priv = hb->priv;
  CtkBin *bin = CTK_BIN (widget);
  CtkWidget *child;

  ctk_widget_set_mapped (widget, TRUE);

  child = ctk_bin_get_child (bin);
  if (child != NULL &&
      ctk_widget_get_visible (child) &&
      !ctk_widget_get_mapped (child))
    ctk_widget_map (child);

  if (priv->child_detached && !priv->float_window_mapped)
    {
      cdk_window_show (priv->float_window);
      priv->float_window_mapped = TRUE;
    }

  cdk_window_show (priv->bin_window);
  cdk_window_show (ctk_widget_get_window (widget));
}

static void
ctk_handle_box_unmap (CtkWidget *widget)
{
  CtkHandleBox *hb = CTK_HANDLE_BOX (widget);
  CtkHandleBoxPrivate *priv = hb->priv;

  ctk_widget_set_mapped (widget, FALSE);

  cdk_window_hide (ctk_widget_get_window (widget));
  if (priv->float_window_mapped)
    {
      cdk_window_hide (priv->float_window);
      priv->float_window_mapped = FALSE;
    }

  CTK_WIDGET_CLASS (ctk_handle_box_parent_class)->unmap (widget);
}

static void
ctk_handle_box_realize (CtkWidget *widget)
{
  CtkHandleBox *hb = CTK_HANDLE_BOX (widget);
  CtkHandleBoxPrivate *priv = hb->priv;
  CtkAllocation allocation;
  CtkRequisition requisition;
  CtkStyleContext *context;
  CtkWidget *child;
  CdkWindow *window;
  CdkWindowAttr attributes;
  gint attributes_mask;

  ctk_widget_set_realized (widget, TRUE);

  ctk_widget_get_allocation (widget, &allocation);

  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.window_type = CDK_WINDOW_CHILD;
  attributes.wclass = CDK_INPUT_OUTPUT;
  attributes.visual = ctk_widget_get_visual (widget);
  attributes.event_mask = ctk_widget_get_events (widget);
  attributes_mask = CDK_WA_X | CDK_WA_Y | CDK_WA_VISUAL;

  window = cdk_window_new (ctk_widget_get_parent_window (widget),
                           &attributes, attributes_mask);
  ctk_widget_set_window (widget, window);
  cdk_window_set_user_data (window, widget);

  attributes.x = 0;
  attributes.y = 0;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.window_type = CDK_WINDOW_CHILD;
  attributes.event_mask = (ctk_widget_get_events (widget)
                           | CDK_BUTTON1_MOTION_MASK
                           | CDK_POINTER_MOTION_HINT_MASK
                           | CDK_BUTTON_PRESS_MASK
                           | CDK_BUTTON_RELEASE_MASK);
  attributes_mask = CDK_WA_X | CDK_WA_Y | CDK_WA_VISUAL;

  priv->bin_window = cdk_window_new (window,
                                     &attributes, attributes_mask);
  cdk_window_set_user_data (priv->bin_window, widget);

  child = ctk_bin_get_child (CTK_BIN (hb));
  if (child)
    ctk_widget_set_parent_window (child, priv->bin_window);

  ctk_widget_get_preferred_size (widget, &requisition, NULL);

  attributes.x = 0;
  attributes.y = 0;
  attributes.width = requisition.width;
  attributes.height = requisition.height;
  attributes.window_type = CDK_WINDOW_TOPLEVEL;
  attributes.wclass = CDK_INPUT_OUTPUT;
  attributes.visual = ctk_widget_get_visual (widget);
  attributes.event_mask = (ctk_widget_get_events (widget)
                           | CDK_KEY_PRESS_MASK
                           | CDK_ENTER_NOTIFY_MASK
                           | CDK_LEAVE_NOTIFY_MASK
                           | CDK_FOCUS_CHANGE_MASK
                           | CDK_STRUCTURE_MASK);
  attributes.type_hint = CDK_WINDOW_TYPE_HINT_TOOLBAR;
  attributes_mask = CDK_WA_X | CDK_WA_Y | CDK_WA_VISUAL | CDK_WA_TYPE_HINT;
  priv->float_window = cdk_window_new (cdk_screen_get_root_window (ctk_widget_get_screen (widget)),
                                       &attributes, attributes_mask);
  cdk_window_set_user_data (priv->float_window, widget);
  cdk_window_set_decorations (priv->float_window, 0);
  cdk_window_set_type_hint (priv->float_window, CDK_WINDOW_TYPE_HINT_TOOLBAR);

  context = ctk_widget_get_style_context (widget);
  ctk_style_context_set_background (context, window);
  ctk_style_context_set_background (context, priv->bin_window);
  ctk_style_context_set_background (context, priv->float_window);
}

static void
ctk_handle_box_unrealize (CtkWidget *widget)
{
  CtkHandleBox *hb = CTK_HANDLE_BOX (widget);
  CtkHandleBoxPrivate *priv = hb->priv;

  cdk_window_set_user_data (priv->bin_window, NULL);
  cdk_window_destroy (priv->bin_window);
  priv->bin_window = NULL;
  cdk_window_set_user_data (priv->float_window, NULL);
  cdk_window_destroy (priv->float_window);
  priv->float_window = NULL;

  CTK_WIDGET_CLASS (ctk_handle_box_parent_class)->unrealize (widget);
}

static void
ctk_handle_box_style_updated (CtkWidget *widget)
{
  CtkHandleBox *hb = CTK_HANDLE_BOX (widget);
  CtkHandleBoxPrivate *priv = hb->priv;

  CTK_WIDGET_CLASS (ctk_handle_box_parent_class)->style_updated (widget);

  if (ctk_widget_get_realized (widget) &&
      ctk_widget_get_has_window (widget))
    {
      CtkStateFlags state;
      CtkStyleContext *context;

      context = ctk_widget_get_style_context (widget);
      state = ctk_widget_get_state_flags (widget);

      ctk_style_context_save (context);
      ctk_style_context_set_state (context, state);

      ctk_style_context_set_background (context, ctk_widget_get_window (widget));
      ctk_style_context_set_background (context, priv->bin_window);
      ctk_style_context_set_background (context, priv->float_window);

      ctk_style_context_restore (context);
    }
}

static int
effective_handle_position (CtkHandleBox *hb)
{
  CtkHandleBoxPrivate *priv = hb->priv;
  int handle_position;

  if (ctk_widget_get_direction (CTK_WIDGET (hb)) == CTK_TEXT_DIR_LTR)
    handle_position = priv->handle_position;
  else
    {
      switch (priv->handle_position)
	{
	case CTK_POS_LEFT:
	  handle_position = CTK_POS_RIGHT;
	  break;
	case CTK_POS_RIGHT:
	  handle_position = CTK_POS_LEFT;
	  break;
	default:
	  handle_position = priv->handle_position;
	  break;
	}
    }

  return handle_position;
}

static void
ctk_handle_box_size_request (CtkWidget      *widget,
			     CtkRequisition *requisition)
{
  CtkBin *bin = CTK_BIN (widget);
  CtkHandleBox *hb = CTK_HANDLE_BOX (widget);
  CtkHandleBoxPrivate *priv = hb->priv;
  CtkRequisition child_requisition;
  CtkWidget *child;
  gint handle_position;

  handle_position = effective_handle_position (hb);

  if (handle_position == CTK_POS_LEFT ||
      handle_position == CTK_POS_RIGHT)
    {
      requisition->width = DRAG_HANDLE_SIZE;
      requisition->height = 0;
    }
  else
    {
      requisition->width = 0;
      requisition->height = DRAG_HANDLE_SIZE;
    }

  child = ctk_bin_get_child (bin);
  /* if our child is not visible, we still request its size, since we
   * won't have any useful hint for our size otherwise.
   */
  if (child)
    {
      ctk_widget_get_preferred_size (child, &child_requisition, NULL);
    }
  else
    {
      child_requisition.width = 0;
      child_requisition.height = 0;
    }      

  if (priv->child_detached)
    {
      /* FIXME: This doesn't work currently */
      if (!priv->shrink_on_detach)
	{
	  if (handle_position == CTK_POS_LEFT ||
	      handle_position == CTK_POS_RIGHT)
	    requisition->height += child_requisition.height;
	  else
	    requisition->width += child_requisition.width;
	}
      else
	{
          CtkStyleContext *context;
          CtkStateFlags state;
          CtkBorder padding;

          context = ctk_widget_get_style_context (widget);
          state = ctk_widget_get_state_flags (widget);
          ctk_style_context_get_padding (context, state, &padding);

	  if (handle_position == CTK_POS_LEFT ||
	      handle_position == CTK_POS_RIGHT)
	    requisition->height += padding.top;
	  else
	    requisition->width += padding.left;
	}
    }
  else
    {
      guint border_width;

      border_width = ctk_container_get_border_width (CTK_CONTAINER (widget));
      requisition->width += border_width * 2;
      requisition->height += border_width * 2;
      
      if (child)
	{
	  requisition->width += child_requisition.width;
	  requisition->height += child_requisition.height;
	}
      else
	{
	  requisition->width += CHILDLESS_SIZE;
	  requisition->height += CHILDLESS_SIZE;
	}
    }
}

static void
ctk_handle_box_get_preferred_width (CtkWidget *widget,
				     gint      *minimum,
				     gint      *natural)
{
  CtkRequisition requisition;

  ctk_handle_box_size_request (widget, &requisition);

  *minimum = *natural = requisition.width;
}

static void
ctk_handle_box_get_preferred_height (CtkWidget *widget,
				     gint      *minimum,
				     gint      *natural)
{
  CtkRequisition requisition;

  ctk_handle_box_size_request (widget, &requisition);

  *minimum = *natural = requisition.height;
}


static void
ctk_handle_box_size_allocate (CtkWidget     *widget,
			      CtkAllocation *allocation)
{
  CtkBin *bin = CTK_BIN (widget);
  CtkHandleBox *hb = CTK_HANDLE_BOX (widget);
  CtkHandleBoxPrivate *priv = hb->priv;
  CtkRequisition child_requisition;
  CtkWidget *child;
  gint handle_position;

  handle_position = effective_handle_position (hb);

  child = ctk_bin_get_child (bin);

  if (child)
    {
      ctk_widget_get_preferred_size (child, &child_requisition, NULL);
    }
  else
    {
      child_requisition.width = 0;
      child_requisition.height = 0;
    }

  ctk_widget_set_allocation (widget, allocation);

  if (ctk_widget_get_realized (widget))
    cdk_window_move_resize (ctk_widget_get_window (widget),
                            allocation->x, allocation->y,
                            allocation->width, allocation->height);

  if (child != NULL && ctk_widget_get_visible (child))
    {
      CtkAllocation child_allocation;
      guint border_width;

      border_width = ctk_container_get_border_width (CTK_CONTAINER (widget));

      child_allocation.x = border_width;
      child_allocation.y = border_width;
      if (handle_position == CTK_POS_LEFT)
	child_allocation.x += DRAG_HANDLE_SIZE;
      else if (handle_position == CTK_POS_TOP)
	child_allocation.y += DRAG_HANDLE_SIZE;

      if (priv->child_detached)
	{
	  guint float_width;
	  guint float_height;
	  
	  child_allocation.width = child_requisition.width;
	  child_allocation.height = child_requisition.height;
	  
	  float_width = child_allocation.width + 2 * border_width;
	  float_height = child_allocation.height + 2 * border_width;
	  
	  if (handle_position == CTK_POS_LEFT ||
	      handle_position == CTK_POS_RIGHT)
	    float_width += DRAG_HANDLE_SIZE;
	  else
	    float_height += DRAG_HANDLE_SIZE;

	  if (ctk_widget_get_realized (widget))
	    {
	      cdk_window_resize (priv->float_window,
				 float_width,
				 float_height);
	      cdk_window_move_resize (priv->bin_window,
				      0,
				      0,
				      float_width,
				      float_height);
	    }
	}
      else
	{
	  child_allocation.width = MAX (1, (gint) allocation->width - 2 * border_width);
	  child_allocation.height = MAX (1, (gint) allocation->height - 2 * border_width);

	  if (handle_position == CTK_POS_LEFT ||
	      handle_position == CTK_POS_RIGHT)
	    child_allocation.width -= DRAG_HANDLE_SIZE;
	  else
	    child_allocation.height -= DRAG_HANDLE_SIZE;
	  
	  if (ctk_widget_get_realized (widget))
	    cdk_window_move_resize (priv->bin_window,
				    0,
				    0,
				    allocation->width,
				    allocation->height);
	}

      ctk_widget_size_allocate (child, &child_allocation);
    }
}

static void
ctk_handle_box_draw_ghost (CtkHandleBox *hb,
                           cairo_t      *cr)
{
  CtkWidget *widget = CTK_WIDGET (hb);
  CtkStateFlags state;
  CtkStyleContext *context;
  guint x;
  guint y;
  guint width;
  guint height;
  gint allocation_width;
  gint allocation_height;
  gint handle_position;

  handle_position = effective_handle_position (hb);
  allocation_width = ctk_widget_get_allocated_width (widget);
  allocation_height = ctk_widget_get_allocated_height (widget);

  if (handle_position == CTK_POS_LEFT ||
      handle_position == CTK_POS_RIGHT)
    {
      x = handle_position == CTK_POS_LEFT ? 0 : allocation_width - DRAG_HANDLE_SIZE;
      y = 0;
      width = DRAG_HANDLE_SIZE;
      height = allocation_height;
    }
  else
    {
      x = 0;
      y = handle_position == CTK_POS_TOP ? 0 : allocation_height - DRAG_HANDLE_SIZE;
      width = allocation_width;
      height = DRAG_HANDLE_SIZE;
    }

  context = ctk_widget_get_style_context (widget);
  state = ctk_widget_get_state_flags (widget);

  ctk_style_context_save (context);
  ctk_style_context_set_state (context, state);

  ctk_render_background (context, cr, x, y, width, height);
  ctk_render_frame (context, cr, x, y, width, height);

  if (handle_position == CTK_POS_LEFT ||
      handle_position == CTK_POS_RIGHT)
    ctk_render_line (context, cr,
                     handle_position == CTK_POS_LEFT ? DRAG_HANDLE_SIZE : 0,
                     allocation_height / 2,
                     handle_position == CTK_POS_LEFT ? allocation_width : allocation_width - DRAG_HANDLE_SIZE,
                     allocation_height / 2);
  else
    ctk_render_line (context, cr,
                     allocation_width / 2,
                     handle_position == CTK_POS_TOP ? DRAG_HANDLE_SIZE : 0,
                     allocation_width / 2,
                     handle_position == CTK_POS_TOP ? allocation_height : allocation_height - DRAG_HANDLE_SIZE);

  ctk_style_context_restore (context);
}

/**
 * ctk_handle_box_set_shadow_type:
 * @handle_box: a #CtkHandleBox
 * @type: the shadow type.
 *
 * Sets the type of shadow to be drawn around the border
 * of the handle box.
 *
 * Deprecated: 3.4: #CtkHandleBox has been deprecated.
 */
void
ctk_handle_box_set_shadow_type (CtkHandleBox  *handle_box,
				CtkShadowType  type)
{
  CtkHandleBoxPrivate *priv;

  g_return_if_fail (CTK_IS_HANDLE_BOX (handle_box));

  priv = handle_box->priv;

  if ((CtkShadowType) priv->shadow_type != type)
    {
      priv->shadow_type = type;
      g_object_notify (G_OBJECT (handle_box), "shadow-type");
      ctk_widget_queue_resize (CTK_WIDGET (handle_box));
    }
}

/**
 * ctk_handle_box_get_shadow_type:
 * @handle_box: a #CtkHandleBox
 *
 * Gets the type of shadow drawn around the handle box. See
 * ctk_handle_box_set_shadow_type().
 *
 * Returns: the type of shadow currently drawn around the handle box.
 *
 * Deprecated: 3.4: #CtkHandleBox has been deprecated.
 **/
CtkShadowType
ctk_handle_box_get_shadow_type (CtkHandleBox *handle_box)
{
  g_return_val_if_fail (CTK_IS_HANDLE_BOX (handle_box), CTK_SHADOW_ETCHED_OUT);

  return handle_box->priv->shadow_type;
}

/**
 * ctk_handle_box_set_handle_position:
 * @handle_box: a #CtkHandleBox
 * @position: the side of the handlebox where the handle should be drawn.
 *
 * Sets the side of the handlebox where the handle is drawn.
 *
 * Deprecated: 3.4: #CtkHandleBox has been deprecated.
 */
void        
ctk_handle_box_set_handle_position  (CtkHandleBox    *handle_box,
				     CtkPositionType  position)
{
  CtkHandleBoxPrivate *priv;

  g_return_if_fail (CTK_IS_HANDLE_BOX (handle_box));

  priv = handle_box->priv;

  if ((CtkPositionType) priv->handle_position != position)
    {
      priv->handle_position = position;
      g_object_notify (G_OBJECT (handle_box), "handle-position");
      ctk_widget_queue_resize (CTK_WIDGET (handle_box));
    }
}

/**
 * ctk_handle_box_get_handle_position:
 * @handle_box: a #CtkHandleBox
 *
 * Gets the handle position of the handle box. See
 * ctk_handle_box_set_handle_position().
 *
 * Returns: the current handle position.
 *
 * Deprecated: 3.4: #CtkHandleBox has been deprecated.
 **/
CtkPositionType
ctk_handle_box_get_handle_position (CtkHandleBox *handle_box)
{
  g_return_val_if_fail (CTK_IS_HANDLE_BOX (handle_box), CTK_POS_LEFT);

  return handle_box->priv->handle_position;
}

/**
 * ctk_handle_box_set_snap_edge:
 * @handle_box: a #CtkHandleBox
 * @edge: the snap edge, or -1 to unset the value; in which
 *   case CTK+ will try to guess an appropriate value
 *   in the future.
 *
 * Sets the snap edge of a handlebox. The snap edge is
 * the edge of the detached child that must be aligned
 * with the corresponding edge of the “ghost” left
 * behind when the child was detached to reattach
 * the torn-off window. Usually, the snap edge should
 * be chosen so that it stays in the same place on
 * the screen when the handlebox is torn off.
 *
 * If the snap edge is not set, then an appropriate value
 * will be guessed from the handle position. If the
 * handle position is %CTK_POS_RIGHT or %CTK_POS_LEFT,
 * then the snap edge will be %CTK_POS_TOP, otherwise
 * it will be %CTK_POS_LEFT.
 *
 * Deprecated: 3.4: #CtkHandleBox has been deprecated.
 */
void
ctk_handle_box_set_snap_edge        (CtkHandleBox    *handle_box,
				     CtkPositionType  edge)
{
  CtkHandleBoxPrivate *priv;

  g_return_if_fail (CTK_IS_HANDLE_BOX (handle_box));

  priv = handle_box->priv;

  if (priv->snap_edge != edge)
    {
      priv->snap_edge = edge;

      g_object_freeze_notify (G_OBJECT (handle_box));
      g_object_notify (G_OBJECT (handle_box), "snap-edge");
      g_object_notify (G_OBJECT (handle_box), "snap-edge-set");
      g_object_thaw_notify (G_OBJECT (handle_box));
    }
}

/**
 * ctk_handle_box_get_snap_edge:
 * @handle_box: a #CtkHandleBox
 *
 * Gets the edge used for determining reattachment of the handle box.
 * See ctk_handle_box_set_snap_edge().
 *
 * Returns: the edge used for determining reattachment, or
 *   (CtkPositionType)-1 if this is determined (as per default)
 *   from the handle position.
 *
 * Deprecated: 3.4: #CtkHandleBox has been deprecated.
 **/
CtkPositionType
ctk_handle_box_get_snap_edge (CtkHandleBox *handle_box)
{
  g_return_val_if_fail (CTK_IS_HANDLE_BOX (handle_box), (CtkPositionType)-1);

  return (CtkPositionType)handle_box->priv->snap_edge;
}

/**
 * ctk_handle_box_get_child_detached:
 * @handle_box: a #CtkHandleBox
 *
 * Whether the handlebox’s child is currently detached.
 *
 * Returns: %TRUE if the child is currently detached, otherwise %FALSE
 *
 * Since: 2.14
 *
 * Deprecated: 3.4: #CtkHandleBox has been deprecated.
 **/
gboolean
ctk_handle_box_get_child_detached (CtkHandleBox *handle_box)
{
  g_return_val_if_fail (CTK_IS_HANDLE_BOX (handle_box), FALSE);

  return handle_box->priv->child_detached;
}

static void
ctk_handle_box_paint (CtkWidget *widget,
                      cairo_t   *cr)
{
  CtkHandleBox *hb = CTK_HANDLE_BOX (widget);
  CtkHandleBoxPrivate *priv = hb->priv;
  CtkBin *bin = CTK_BIN (widget);
  CtkStyleContext *context;
  CtkStateFlags state;
  CtkWidget *child;
  gint width, height;
  CdkRectangle rect;
  gint handle_position;

  handle_position = effective_handle_position (hb);

  width = cdk_window_get_width (priv->bin_window);
  height = cdk_window_get_height (priv->bin_window);

  context = ctk_widget_get_style_context (widget);
  state = ctk_widget_get_state_flags (widget);

  ctk_style_context_save (context);
  ctk_style_context_set_state (context, state);

  ctk_render_background (context, cr, 0, 0, width, height);
  ctk_render_frame (context, cr, 0, 0, width, height);

  switch (handle_position)
    {
    case CTK_POS_LEFT:
      rect.x = 0;
      rect.y = 0;
      rect.width = DRAG_HANDLE_SIZE;
      rect.height = height;
      break;
    case CTK_POS_RIGHT:
      rect.x = width - DRAG_HANDLE_SIZE;
      rect.y = 0;
      rect.width = DRAG_HANDLE_SIZE;
      rect.height = height;
      break;
    case CTK_POS_TOP:
      rect.x = 0;
      rect.y = 0;
      rect.width = width;
      rect.height = DRAG_HANDLE_SIZE;
      break;
    case CTK_POS_BOTTOM:
      rect.x = 0;
      rect.y = height - DRAG_HANDLE_SIZE;
      rect.width = width;
      rect.height = DRAG_HANDLE_SIZE;
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  ctk_render_handle (context, cr,
                     rect.x, rect.y, rect.width, rect.height);

  child = ctk_bin_get_child (bin);
  if (child != NULL && ctk_widget_get_visible (child))
    CTK_WIDGET_CLASS (ctk_handle_box_parent_class)->draw (widget, cr);

  ctk_style_context_restore (context);
}

static gboolean
ctk_handle_box_draw (CtkWidget *widget,
		     cairo_t   *cr)
{
  CtkHandleBox *hb = CTK_HANDLE_BOX (widget);
  CtkHandleBoxPrivate *priv = hb->priv;

  if (ctk_cairo_should_draw_window (cr, ctk_widget_get_window (widget)))
    {
      if (priv->child_detached)
        ctk_handle_box_draw_ghost (hb, cr);
    }
  else if (ctk_cairo_should_draw_window (cr, priv->bin_window))
    ctk_handle_box_paint (widget, cr);
  
  return FALSE;
}

static CtkWidget *
ctk_handle_box_get_invisible (void)
{
  static CtkWidget *handle_box_invisible = NULL;

  if (!handle_box_invisible)
    {
      handle_box_invisible = ctk_invisible_new ();
      ctk_widget_show (handle_box_invisible);
    }
  
  return handle_box_invisible;
}

static gboolean
ctk_handle_box_grab_event (CtkWidget    *widget G_GNUC_UNUSED,
			   CdkEvent     *event,
			   CtkHandleBox *hb)
{
  CtkHandleBoxPrivate *priv = hb->priv;

  switch (event->type)
    {
    case CDK_BUTTON_RELEASE:
      if (priv->in_drag)		/* sanity check */
	{
	  ctk_handle_box_end_drag (hb, event->button.time);
	  return TRUE;
	}
      break;

    case CDK_MOTION_NOTIFY:
      return ctk_handle_box_motion (CTK_WIDGET (hb), (CdkEventMotion *)event);
      break;

    default:
      break;
    }

  return FALSE;
}

static gboolean
ctk_handle_box_button_press (CtkWidget      *widget,
                             CdkEventButton *event)
{
  CtkHandleBox *hb = CTK_HANDLE_BOX (widget);
  CtkHandleBoxPrivate *priv = hb->priv;
  gboolean event_handled;
  gint handle_position;

  handle_position = effective_handle_position (hb);

  event_handled = FALSE;
  if ((event->button == CDK_BUTTON_PRIMARY) &&
      (event->type == CDK_BUTTON_PRESS || event->type == CDK_2BUTTON_PRESS))
    {
      CtkWidget *child;
      gboolean in_handle;
      
      if (event->window != priv->bin_window)
	return FALSE;

      child = ctk_bin_get_child (CTK_BIN (hb));

      if (child)
	{
          CtkAllocation child_allocation;
          guint border_width;

          ctk_widget_get_allocation (child, &child_allocation);
          border_width = ctk_container_get_border_width (CTK_CONTAINER (hb));

	  switch (handle_position)
	    {
	    case CTK_POS_LEFT:
	      in_handle = event->x < DRAG_HANDLE_SIZE;
	      break;
	    case CTK_POS_TOP:
	      in_handle = event->y < DRAG_HANDLE_SIZE;
	      break;
	    case CTK_POS_RIGHT:
	      in_handle = event->x > 2 * border_width + child_allocation.width;
	      break;
	    case CTK_POS_BOTTOM:
	      in_handle = event->y > 2 * border_width + child_allocation.height;
	      break;
	    default:
	      in_handle = FALSE;
	      break;
	    }
	}
      else
	{
	  in_handle = FALSE;
	  event_handled = TRUE;
	}
      
      if (in_handle)
	{
	  if (event->type == CDK_BUTTON_PRESS) /* Start a drag */
	    {
	      CdkCursor *fleur;

	      CtkWidget *invisible = ctk_handle_box_get_invisible ();
              CdkWindow *window;
	      gint root_x, root_y;

              ctk_invisible_set_screen (CTK_INVISIBLE (invisible),
                                        ctk_widget_get_screen (CTK_WIDGET (hb)));
	      cdk_window_get_origin (priv->bin_window, &root_x, &root_y);

	      priv->orig_x = event->x_root;
	      priv->orig_y = event->y_root;

	      priv->float_allocation.x = root_x - event->x_root;
	      priv->float_allocation.y = root_y - event->y_root;
	      priv->float_allocation.width = cdk_window_get_width (priv->bin_window);
	      priv->float_allocation.height = cdk_window_get_height (priv->bin_window);

              window = ctk_widget_get_window (widget);
	      if (cdk_window_is_viewable (window))
		{
		  cdk_window_get_origin (window, &root_x, &root_y);

		  priv->attach_allocation.x = root_x;
		  priv->attach_allocation.y = root_y;
		  priv->attach_allocation.width = cdk_window_get_width (window);
		  priv->attach_allocation.height = cdk_window_get_height (window);
		}
	      else
		{
		  priv->attach_allocation.x = -1;
		  priv->attach_allocation.y = -1;
		  priv->attach_allocation.width = 0;
		  priv->attach_allocation.height = 0;
		}
	      priv->in_drag = TRUE;
              priv->grab_device = event->device;
	      fleur = cdk_cursor_new_for_display (ctk_widget_get_display (widget),
						  CDK_FLEUR);
	      if (cdk_device_grab (event->device,
                                   ctk_widget_get_window (invisible),
                                   CDK_OWNERSHIP_WINDOW,
                                   FALSE,
                                   (CDK_BUTTON1_MOTION_MASK |
                                    CDK_POINTER_MOTION_HINT_MASK |
                                    CDK_BUTTON_RELEASE_MASK),
                                   fleur,
                                   event->time) != CDK_GRAB_SUCCESS)
		{
		  priv->in_drag = FALSE;
                  priv->grab_device = NULL;
		}
	      else
		{
                  ctk_device_grab_add (invisible, priv->grab_device, TRUE);
		  g_signal_connect (invisible, "event",
				    G_CALLBACK (ctk_handle_box_grab_event), hb);
		}

	      g_object_unref (fleur);
	      event_handled = TRUE;
	    }
	  else if (priv->child_detached) /* Double click */
	    {
	      ctk_handle_box_reattach (hb);
	    }
	}
    }
  
  return event_handled;
}

static gboolean
ctk_handle_box_motion (CtkWidget      *widget,
		       CdkEventMotion *event)
{
  CtkHandleBox *hb = CTK_HANDLE_BOX (widget);
  CtkHandleBoxPrivate *priv = hb->priv;
  CtkWidget *child;
  gint new_x, new_y;
  gint snap_edge;
  gboolean is_snapped = FALSE;
  gint handle_position;
  CdkGeometry geometry;
  CdkScreen *screen, *pointer_screen;

  if (!priv->in_drag)
    return FALSE;
  handle_position = effective_handle_position (hb);

  /* Calculate the attachment point on the float, if the float
   * were detached
   */
  new_x = 0;
  new_y = 0;
  screen = ctk_widget_get_screen (widget);
  cdk_device_get_position (event->device,
                           &pointer_screen,
                           &new_x, &new_y);
  if (pointer_screen != screen)
    {
      new_x = priv->orig_x;
      new_y = priv->orig_y;
    }

  new_x += priv->float_allocation.x;
  new_y += priv->float_allocation.y;

  snap_edge = priv->snap_edge;
  if (snap_edge == -1)
    snap_edge = (handle_position == CTK_POS_LEFT ||
		 handle_position == CTK_POS_RIGHT) ?
      CTK_POS_TOP : CTK_POS_LEFT;

  if (ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL) 
    switch (snap_edge) 
      {
      case CTK_POS_LEFT:
	snap_edge = CTK_POS_RIGHT;
	break;
      case CTK_POS_RIGHT:
	snap_edge = CTK_POS_LEFT;
	break;
      default:
	break;
      }

  /* First, check if the snapped edge is aligned
   */
  switch (snap_edge)
    {
    case CTK_POS_TOP:
      is_snapped = abs (priv->attach_allocation.y - new_y) < TOLERANCE;
      break;
    case CTK_POS_BOTTOM:
      is_snapped = abs (priv->attach_allocation.y + (gint)priv->attach_allocation.height -
			new_y - (gint)priv->float_allocation.height) < TOLERANCE;
      break;
    case CTK_POS_LEFT:
      is_snapped = abs (priv->attach_allocation.x - new_x) < TOLERANCE;
      break;
    case CTK_POS_RIGHT:
      is_snapped = abs (priv->attach_allocation.x + (gint)priv->attach_allocation.width -
			new_x - (gint)priv->float_allocation.width) < TOLERANCE;
      break;
    }

  /* Next, check if coordinates in the other direction are sufficiently
   * aligned
   */
  if (is_snapped)
    {
      gint float_pos1 = 0;	/* Initialize to suppress warnings */
      gint float_pos2 = 0;
      gint attach_pos1 = 0;
      gint attach_pos2 = 0;
      
      switch (snap_edge)
	{
	case CTK_POS_TOP:
	case CTK_POS_BOTTOM:
	  attach_pos1 = priv->attach_allocation.x;
	  attach_pos2 = priv->attach_allocation.x + priv->attach_allocation.width;
	  float_pos1 = new_x;
	  float_pos2 = new_x + priv->float_allocation.width;
	  break;
	case CTK_POS_LEFT:
	case CTK_POS_RIGHT:
	  attach_pos1 = priv->attach_allocation.y;
	  attach_pos2 = priv->attach_allocation.y + priv->attach_allocation.height;
	  float_pos1 = new_y;
	  float_pos2 = new_y + priv->float_allocation.height;
	  break;
	}

      is_snapped = ((attach_pos1 - TOLERANCE < float_pos1) && 
		    (attach_pos2 + TOLERANCE > float_pos2)) ||
	           ((float_pos1 - TOLERANCE < attach_pos1) &&
		    (float_pos2 + TOLERANCE > attach_pos2));
    }

  child = ctk_bin_get_child (CTK_BIN (hb));

  if (is_snapped)
    {
      if (priv->child_detached)
	{
	  priv->child_detached = FALSE;
	  cdk_window_hide (priv->float_window);
	  cdk_window_reparent (priv->bin_window, ctk_widget_get_window (widget), 0, 0);
	  priv->float_window_mapped = FALSE;
	  g_signal_emit (hb,
			 handle_box_signals[SIGNAL_CHILD_ATTACHED],
			 0,
			 child);
	  
	  ctk_widget_queue_resize (widget);
	}
    }
  else
    {
      gint width, height;

      width = cdk_window_get_width (priv->float_window);
      height = cdk_window_get_height (priv->float_window);

      switch (handle_position)
	{
	case CTK_POS_LEFT:
	  new_y += ((gint)priv->float_allocation.height - height) / 2;
	  break;
	case CTK_POS_RIGHT:
	  new_x += (gint)priv->float_allocation.width - width;
	  new_y += ((gint)priv->float_allocation.height - height) / 2;
	  break;
	case CTK_POS_TOP:
	  new_x += ((gint)priv->float_allocation.width - width) / 2;
	  break;
	case CTK_POS_BOTTOM:
	  new_x += ((gint)priv->float_allocation.width - width) / 2;
	  new_y += (gint)priv->float_allocation.height - height;
	  break;
	}

      if (priv->child_detached)
	{
	  cdk_window_move (priv->float_window, new_x, new_y);
	  cdk_window_raise (priv->float_window);
	}
      else
	{
          guint border_width;
	  CtkRequisition child_requisition;

	  priv->child_detached = TRUE;

	  if (child)
            {
              ctk_widget_get_preferred_size (child, &child_requisition, NULL);
            }
	  else
	    {
	      child_requisition.width = 0;
	      child_requisition.height = 0;
	    }      

          border_width = ctk_container_get_border_width (CTK_CONTAINER (hb));
	  width = child_requisition.width + 2 * border_width;
	  height = child_requisition.height + 2 * border_width;

	  if (handle_position == CTK_POS_LEFT || handle_position == CTK_POS_RIGHT)
	    width += DRAG_HANDLE_SIZE;
	  else
	    height += DRAG_HANDLE_SIZE;
	  
	  cdk_window_move_resize (priv->float_window, new_x, new_y, width, height);
	  cdk_window_reparent (priv->bin_window, priv->float_window, 0, 0);
	  cdk_window_set_geometry_hints (priv->float_window, &geometry, CDK_HINT_POS);
	  cdk_window_show (priv->float_window);
	  priv->float_window_mapped = TRUE;
#if	0
	  /* this extra move is necessary if we use decorations, or our
	   * window manager insists on decorations.
	   */
	  cdk_display_sync (ctk_widget_get_display (widget));
	  cdk_window_move (priv->float_window, new_x, new_y);
	  cdk_display_sync (ctk_widget_get_display (widget));
#endif	/* 0 */
	  g_signal_emit (hb,
			 handle_box_signals[SIGNAL_CHILD_DETACHED],
			 0,
			 child);
	  
	  ctk_widget_queue_resize (widget);
	}
    }

  return TRUE;
}

static void
ctk_handle_box_add (CtkContainer *container,
		    CtkWidget    *widget)
{
  CtkHandleBoxPrivate *priv = CTK_HANDLE_BOX (container)->priv;

  ctk_widget_set_parent_window (widget, priv->bin_window);
  CTK_CONTAINER_CLASS (ctk_handle_box_parent_class)->add (container, widget);
}

static void
ctk_handle_box_remove (CtkContainer *container,
		       CtkWidget    *widget)
{
  CTK_CONTAINER_CLASS (ctk_handle_box_parent_class)->remove (container, widget);

  ctk_handle_box_reattach (CTK_HANDLE_BOX (container));
}

static gint
ctk_handle_box_delete_event (CtkWidget *widget,
			     CdkEventAny  *event)
{
  CtkHandleBox *hb = CTK_HANDLE_BOX (widget);
  CtkHandleBoxPrivate *priv = hb->priv;

  if (event->window == priv->float_window)
    {
      ctk_handle_box_reattach (hb);
      
      return TRUE;
    }

  return FALSE;
}

static void
ctk_handle_box_reattach (CtkHandleBox *hb)
{
  CtkHandleBoxPrivate *priv = hb->priv;
  CtkWidget *widget = CTK_WIDGET (hb);
  
  if (priv->child_detached)
    {
      priv->child_detached = FALSE;
      if (ctk_widget_get_realized (widget))
        {
          CtkWidget *child;

          cdk_window_hide (priv->float_window);
          cdk_window_reparent (priv->bin_window, ctk_widget_get_window (widget),
                               0, 0);

          child = ctk_bin_get_child (CTK_BIN (hb));
	  if (child)
	    g_signal_emit (hb,
			   handle_box_signals[SIGNAL_CHILD_ATTACHED],
			   0,
			   child);

	}
      priv->float_window_mapped = FALSE;
    }
  if (priv->in_drag)
    ctk_handle_box_end_drag (hb, CDK_CURRENT_TIME);

  ctk_widget_queue_resize (CTK_WIDGET (hb));
}

static void
ctk_handle_box_end_drag (CtkHandleBox *hb,
			 guint32       time)
{
  CtkHandleBoxPrivate *priv = hb->priv;
  CtkWidget *invisible = ctk_handle_box_get_invisible ();

  priv->in_drag = FALSE;

  ctk_device_grab_remove (invisible, priv->grab_device);
  cdk_device_ungrab (priv->grab_device, time);
  g_signal_handlers_disconnect_by_func (invisible,
					G_CALLBACK (ctk_handle_box_grab_event),
					hb);

  priv->grab_device = NULL;
}
