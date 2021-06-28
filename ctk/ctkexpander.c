/* CTK - The GIMP Toolkit
 *
 * Copyright (C) 2003 Sun Microsystems, Inc.
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
 *
 * Authors:
 *      Mark McLoughlin <mark@skynet.ie>
 */

/**
 * SECTION:ctkexpander
 * @Short_description: A container which can hide its child
 * @Title: CtkExpander
 *
 * A #CtkExpander allows the user to hide or show its child by clicking
 * on an expander triangle similar to the triangles used in a #CtkTreeView.
 *
 * Normally you use an expander as you would use any other descendant
 * of #CtkBin; you create the child widget and use ctk_container_add()
 * to add it to the expander. When the expander is toggled, it will take
 * care of showing and hiding the child automatically.
 *
 * # Special Usage
 *
 * There are situations in which you may prefer to show and hide the
 * expanded widget yourself, such as when you want to actually create
 * the widget at expansion time. In this case, create a #CtkExpander
 * but do not add a child to it. The expander widget has an
 * #CtkExpander:expanded property which can be used to monitor
 * its expansion state. You should watch this property with a signal
 * connection as follows:
 *
 * |[<!-- language="C" -->
 * static void
 * expander_callback (GObject    *object,
 *                    GParamSpec *param_spec,
 *                    gpointer    user_data)
 * {
 *   CtkExpander *expander;
 *
 *   expander = CTK_EXPANDER (object);
 *
 *   if (ctk_expander_get_expanded (expander))
 *     {
 *       // Show or create widgets
 *     }
 *   else
 *     {
 *       // Hide or destroy widgets
 *     }
 * }
 *
 * static void
 * create_expander (void)
 * {
 *   CtkWidget *expander = ctk_expander_new_with_mnemonic ("_More Options");
 *   g_signal_connect (expander, "notify::expanded",
 *                     G_CALLBACK (expander_callback), NULL);
 *
 *   // ...
 * }
 * ]|
 *
 * # CtkExpander as CtkBuildable
 *
 * The CtkExpander implementation of the CtkBuildable interface supports
 * placing a child in the label position by specifying “label” as the
 * “type” attribute of a <child> element. A normal content child can be
 * specified without specifying a <child> type attribute.
 *
 * An example of a UI definition fragment with CtkExpander:
 * |[
 * <object class="CtkExpander">
 *   <child type="label">
 *     <object class="CtkLabel" id="expander-label"/>
 *   </child>
 *   <child>
 *     <object class="CtkEntry" id="expander-content"/>
 *   </child>
 * </object>
 * ]|
 *
 * # CSS nodes
 *
 * |[<!-- language="plain" -->
 * expander
 * ├── title
 * │   ├── arrow
 * │   ╰── <label widget>
 * ╰── <child>
 * ]|
 *
 * CtkExpander has three CSS nodes, the main node with the name expander,
 * a subnode with name title and node below it with name arrow. The arrow of an
 * expander that is showing its child gets the :checked pseudoclass added to it.
 */

#include "config.h"

#include <string.h>

#include "ctkexpander.h"

#include "ctklabel.h"
#include "ctkbuildable.h"
#include "ctkcontainer.h"
#include "ctkmarshalers.h"
#include "ctkmain.h"
#include "ctkintl.h"
#include "ctkprivate.h"
#include "ctkdnd.h"
#include "a11y/ctkexpanderaccessible.h"
#include "ctkstylecontextprivate.h"
#include "ctkcsscustomgadgetprivate.h"
#include "ctkboxgadgetprivate.h"
#include "ctkbuiltiniconprivate.h"
#include "ctkwidgetprivate.h"
#include "ctkcontainerprivate.h"


#define DEFAULT_EXPANDER_SIZE 10
#define DEFAULT_EXPANDER_SPACING 2
#define TIMEOUT_EXPAND 500

enum
{
  PROP_0,
  PROP_EXPANDED,
  PROP_LABEL,
  PROP_USE_UNDERLINE,
  PROP_USE_MARKUP,
  PROP_SPACING,
  PROP_LABEL_WIDGET,
  PROP_LABEL_FILL,
  PROP_RESIZE_TOPLEVEL
};

struct _CtkExpanderPrivate
{
  CtkWidget        *label_widget;
  GdkWindow        *event_window;

  CtkCssGadget     *gadget;
  CtkCssGadget     *title_gadget;
  CtkCssGadget     *arrow_gadget;

  CtkGesture       *multipress_gesture;
  gint              spacing;

  guint             expand_timer;

  guint             expanded        : 1;
  guint             use_underline   : 1;
  guint             use_markup      : 1;
  guint             prelight        : 1;
  guint             label_fill      : 1;
  guint             resize_toplevel : 1;
};

static void ctk_expander_set_property (GObject          *object,
                                       guint             prop_id,
                                       const GValue     *value,
                                       GParamSpec       *pspec);
static void ctk_expander_get_property (GObject          *object,
                                       guint             prop_id,
                                       GValue           *value,
                                       GParamSpec       *pspec);

static void     ctk_expander_destroy        (CtkWidget        *widget);
static void     ctk_expander_realize        (CtkWidget        *widget);
static void     ctk_expander_unrealize      (CtkWidget        *widget);
static void     ctk_expander_size_allocate  (CtkWidget        *widget,
                                             CtkAllocation    *allocation);
static void     ctk_expander_map            (CtkWidget        *widget);
static void     ctk_expander_unmap          (CtkWidget        *widget);
static gboolean ctk_expander_draw           (CtkWidget        *widget,
                                             cairo_t          *cr);

static gboolean ctk_expander_enter_notify   (CtkWidget        *widget,
                                             GdkEventCrossing *event);
static gboolean ctk_expander_leave_notify   (CtkWidget        *widget,
                                             GdkEventCrossing *event);
static gboolean ctk_expander_focus          (CtkWidget        *widget,
                                             CtkDirectionType  direction);
static gboolean ctk_expander_drag_motion    (CtkWidget        *widget,
                                             GdkDragContext   *context,
                                             gint              x,
                                             gint              y,
                                             guint             time);
static void     ctk_expander_drag_leave     (CtkWidget        *widget,
                                             GdkDragContext   *context,
                                             guint             time);

static void ctk_expander_add    (CtkContainer *container,
                                 CtkWidget    *widget);
static void ctk_expander_remove (CtkContainer *container,
                                 CtkWidget    *widget);
static void ctk_expander_forall (CtkContainer *container,
                                 gboolean        include_internals,
                                 CtkCallback     callback,
                                 gpointer        callback_data);

static void ctk_expander_activate (CtkExpander *expander);


/* CtkBuildable */
static void ctk_expander_buildable_init           (CtkBuildableIface *iface);
static void ctk_expander_buildable_add_child      (CtkBuildable *buildable,
                                                   CtkBuilder   *builder,
                                                   GObject      *child,
                                                   const gchar  *type);


/* CtkWidget      */
static void  ctk_expander_get_preferred_width             (CtkWidget           *widget,
                                                           gint                *minimum_size,
                                                           gint                *natural_size);
static void  ctk_expander_get_preferred_height            (CtkWidget           *widget,
                                                           gint                *minimum_size,
                                                           gint                *natural_size);
static void  ctk_expander_get_preferred_height_for_width  (CtkWidget           *layout,
                                                           gint                 width,
                                                           gint                *minimum_height,
                                                           gint                *natural_height);
static void  ctk_expander_get_preferred_width_for_height  (CtkWidget           *layout,
                                                           gint                 width,
                                                           gint                *minimum_height,
                                                           gint                *natural_height);
static void ctk_expander_state_flags_changed (CtkWidget        *widget,
                                              CtkStateFlags     previous_state);
static void ctk_expander_direction_changed   (CtkWidget        *widget,
                                              CtkTextDirection  previous_direction);

/* Gestures */
static void     gesture_multipress_released_cb (CtkGestureMultiPress *gesture,
                                                gint                  n_press,
                                                gdouble               x,
                                                gdouble               y,
                                                CtkExpander          *expander);

G_DEFINE_TYPE_WITH_CODE (CtkExpander, ctk_expander, CTK_TYPE_BIN,
                         G_ADD_PRIVATE (CtkExpander)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_BUILDABLE,
                                                ctk_expander_buildable_init))

static void
ctk_expander_class_init (CtkExpanderClass *klass)
{
  GObjectClass *gobject_class;
  CtkWidgetClass *widget_class;
  CtkContainerClass *container_class;

  gobject_class   = (GObjectClass *) klass;
  widget_class    = (CtkWidgetClass *) klass;
  container_class = (CtkContainerClass *) klass;

  gobject_class->set_property = ctk_expander_set_property;
  gobject_class->get_property = ctk_expander_get_property;

  widget_class->destroy              = ctk_expander_destroy;
  widget_class->realize              = ctk_expander_realize;
  widget_class->unrealize            = ctk_expander_unrealize;
  widget_class->size_allocate        = ctk_expander_size_allocate;
  widget_class->map                  = ctk_expander_map;
  widget_class->unmap                = ctk_expander_unmap;
  widget_class->draw                 = ctk_expander_draw;
  widget_class->enter_notify_event   = ctk_expander_enter_notify;
  widget_class->leave_notify_event   = ctk_expander_leave_notify;
  widget_class->focus                = ctk_expander_focus;
  widget_class->drag_motion          = ctk_expander_drag_motion;
  widget_class->drag_leave           = ctk_expander_drag_leave;
  widget_class->get_preferred_width            = ctk_expander_get_preferred_width;
  widget_class->get_preferred_height           = ctk_expander_get_preferred_height;
  widget_class->get_preferred_height_for_width = ctk_expander_get_preferred_height_for_width;
  widget_class->get_preferred_width_for_height = ctk_expander_get_preferred_width_for_height;
  widget_class->state_flags_changed  = ctk_expander_state_flags_changed;
  widget_class->direction_changed  = ctk_expander_direction_changed;

  container_class->add    = ctk_expander_add;
  container_class->remove = ctk_expander_remove;
  container_class->forall = ctk_expander_forall;

  klass->activate = ctk_expander_activate;

  g_object_class_install_property (gobject_class,
                                   PROP_EXPANDED,
                                   g_param_spec_boolean ("expanded",
                                                         P_("Expanded"),
                                                         P_("Whether the expander has been opened to reveal the child widget"),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (gobject_class,
                                   PROP_LABEL,
                                   g_param_spec_string ("label",
                                                        P_("Label"),
                                                        P_("Text of the expander's label"),
                                                        NULL,
                                                        CTK_PARAM_READWRITE|G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class,
                                   PROP_USE_UNDERLINE,
                                   g_param_spec_boolean ("use-underline",
                                                         P_("Use underline"),
                                                         P_("If set, an underline in the text indicates the next character should be used for the mnemonic accelerator key"),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (gobject_class,
                                   PROP_USE_MARKUP,
                                   g_param_spec_boolean ("use-markup",
                                                         P_("Use markup"),
                                                         P_("The text of the label includes XML markup. See pango_parse_markup()"),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkExpander:spacing:
   *
   * Space to put between the label and the child when the
   * expander is expanded.
   *
   * Deprecated: 3.20: This property is deprecated and ignored.
   *     Use margins on the child instead.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_SPACING,
                                   g_param_spec_int ("spacing",
                                                     P_("Spacing"),
                                                     P_("Space to put between the label and the child"),
                                                     0,
                                                     G_MAXINT,
                                                     0,
                                                     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_DEPRECATED));

  g_object_class_install_property (gobject_class,
                                   PROP_LABEL_WIDGET,
                                   g_param_spec_object ("label-widget",
                                                        P_("Label widget"),
                                                        P_("A widget to display in place of the usual expander label"),
                                                        CTK_TYPE_WIDGET,
                                                        CTK_PARAM_READWRITE));

  /**
   * CtkExpander:label-fill:
   *
   * Whether the label widget should fill all available horizontal space.
   *
   * Note that this property is ignored since 3.20.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_LABEL_FILL,
                                   g_param_spec_boolean ("label-fill",
                                                         P_("Label fill"),
                                                         P_("Whether the label widget should fill all available horizontal space"),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkExpander:resize-toplevel:
   *
   * When this property is %TRUE, the expander will resize the toplevel
   * widget containing the expander upon expanding and collapsing.
   *
   * Since: 3.2
   */
  g_object_class_install_property (gobject_class,
                                   PROP_RESIZE_TOPLEVEL,
                                   g_param_spec_boolean ("resize-toplevel",
                                                         P_("Resize toplevel"),
                                                         P_("Whether the expander will resize the toplevel window upon expanding and collapsing"),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkExpander:expander-size:
   *
   * The size of the expander arrow.
   *
   * Deprecated: 3.20: Use CSS min-width and min-height instead.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("expander-size",
                                                             P_("Expander Size"),
                                                             P_("Size of the expander arrow"),
                                                             0,
                                                             G_MAXINT,
                                                             DEFAULT_EXPANDER_SIZE,
                                                             CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  /**
   * CtkExpander:expander-spacing:
   *
   * Spaing around the expander arrow.
   *
   * Deprecated: 3.20: Use CSS margins instead, the value of this
   *    style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("expander-spacing",
                                                             P_("Indicator Spacing"),
                                                             P_("Spacing around expander arrow"),
                                                             0,
                                                             G_MAXINT,
                                                             DEFAULT_EXPANDER_SPACING,
                                                             CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  widget_class->activate_signal =
    g_signal_new (I_("activate"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (CtkExpanderClass, activate),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_EXPANDER_ACCESSIBLE);
  ctk_widget_class_set_css_name (widget_class, "expander");
}

static void
ctk_expander_init (CtkExpander *expander)
{
  CtkExpanderPrivate *priv;
  CtkCssNode *widget_node;

  expander->priv = priv = ctk_expander_get_instance_private (expander);

  ctk_widget_set_can_focus (CTK_WIDGET (expander), TRUE);
  ctk_widget_set_has_window (CTK_WIDGET (expander), FALSE);

  priv->label_widget = NULL;
  priv->event_window = NULL;
  priv->spacing = 0;

  priv->expanded = FALSE;
  priv->use_underline = FALSE;
  priv->use_markup = FALSE;
  priv->prelight = FALSE;
  priv->label_fill = FALSE;
  priv->expand_timer = 0;
  priv->resize_toplevel = 0;

  widget_node = ctk_widget_get_css_node (CTK_WIDGET (expander));
  priv->gadget = ctk_box_gadget_new_for_node (widget_node, CTK_WIDGET (expander));
  ctk_box_gadget_set_orientation (CTK_BOX_GADGET (priv->gadget), CTK_ORIENTATION_VERTICAL);
  priv->title_gadget = ctk_box_gadget_new ("title",
                                           CTK_WIDGET (expander),
                                           priv->gadget,
                                           NULL);
  ctk_box_gadget_set_orientation (CTK_BOX_GADGET (priv->title_gadget), CTK_ORIENTATION_HORIZONTAL);
  ctk_box_gadget_set_draw_focus (CTK_BOX_GADGET (priv->title_gadget), TRUE);
  ctk_box_gadget_insert_gadget (CTK_BOX_GADGET (priv->gadget), -1, priv->title_gadget, FALSE, CTK_ALIGN_START);

  priv->arrow_gadget = ctk_builtin_icon_new ("arrow",
                                             CTK_WIDGET (expander),
                                             priv->title_gadget,
                                             NULL);
  ctk_css_gadget_add_class (priv->arrow_gadget, CTK_STYLE_CLASS_HORIZONTAL);
  ctk_builtin_icon_set_default_size_property (CTK_BUILTIN_ICON (priv->arrow_gadget), "expander-size");

  ctk_box_gadget_insert_gadget (CTK_BOX_GADGET (priv->title_gadget), -1, priv->arrow_gadget, FALSE, CTK_ALIGN_CENTER);

  ctk_drag_dest_set (CTK_WIDGET (expander), 0, NULL, 0, 0);
  ctk_drag_dest_set_track_motion (CTK_WIDGET (expander), TRUE);

  priv->multipress_gesture = ctk_gesture_multi_press_new (CTK_WIDGET (expander));
  ctk_gesture_single_set_button (CTK_GESTURE_SINGLE (priv->multipress_gesture),
                                 GDK_BUTTON_PRIMARY);
  ctk_gesture_single_set_touch_only (CTK_GESTURE_SINGLE (priv->multipress_gesture),
                                     FALSE);
  g_signal_connect (priv->multipress_gesture, "released",
                    G_CALLBACK (gesture_multipress_released_cb), expander);
  ctk_event_controller_set_propagation_phase (CTK_EVENT_CONTROLLER (priv->multipress_gesture),
                                              CTK_PHASE_BUBBLE);
}

static void
ctk_expander_buildable_add_child (CtkBuildable  *buildable,
                                  CtkBuilder    *builder,
                                  GObject       *child,
                                  const gchar   *type)
{
  if (!type)
    ctk_container_add (CTK_CONTAINER (buildable), CTK_WIDGET (child));
  else if (strcmp (type, "label") == 0)
    ctk_expander_set_label_widget (CTK_EXPANDER (buildable), CTK_WIDGET (child));
  else
    CTK_BUILDER_WARN_INVALID_CHILD_TYPE (CTK_EXPANDER (buildable), type);
}

static void
ctk_expander_buildable_init (CtkBuildableIface *iface)
{
  iface->add_child = ctk_expander_buildable_add_child;
}

static void
ctk_expander_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  CtkExpander *expander = CTK_EXPANDER (object);

  switch (prop_id)
    {
    case PROP_EXPANDED:
      ctk_expander_set_expanded (expander, g_value_get_boolean (value));
      break;
    case PROP_LABEL:
      ctk_expander_set_label (expander, g_value_get_string (value));
      break;
    case PROP_USE_UNDERLINE:
      ctk_expander_set_use_underline (expander, g_value_get_boolean (value));
      break;
    case PROP_USE_MARKUP:
      ctk_expander_set_use_markup (expander, g_value_get_boolean (value));
      break;
    case PROP_SPACING:
      ctk_expander_set_spacing (expander, g_value_get_int (value));
      break;
    case PROP_LABEL_WIDGET:
      ctk_expander_set_label_widget (expander, g_value_get_object (value));
      break;
    case PROP_LABEL_FILL:
      ctk_expander_set_label_fill (expander, g_value_get_boolean (value));
      break;
    case PROP_RESIZE_TOPLEVEL:
      ctk_expander_set_resize_toplevel (expander, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_expander_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  CtkExpander *expander = CTK_EXPANDER (object);
  CtkExpanderPrivate *priv = expander->priv;

  switch (prop_id)
    {
    case PROP_EXPANDED:
      g_value_set_boolean (value, priv->expanded);
      break;
    case PROP_LABEL:
      g_value_set_string (value, ctk_expander_get_label (expander));
      break;
    case PROP_USE_UNDERLINE:
      g_value_set_boolean (value, priv->use_underline);
      break;
    case PROP_USE_MARKUP:
      g_value_set_boolean (value, priv->use_markup);
      break;
    case PROP_SPACING:
      g_value_set_int (value, priv->spacing);
      break;
    case PROP_LABEL_WIDGET:
      g_value_set_object (value,
                          priv->label_widget ?
                          G_OBJECT (priv->label_widget) : NULL);
      break;
    case PROP_LABEL_FILL:
      g_value_set_boolean (value, priv->label_fill);
      break;
    case PROP_RESIZE_TOPLEVEL:
      g_value_set_boolean (value, ctk_expander_get_resize_toplevel (expander));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_expander_destroy (CtkWidget *widget)
{
  CtkExpanderPrivate *priv = CTK_EXPANDER (widget)->priv;

  if (priv->expand_timer)
    {
      g_source_remove (priv->expand_timer);
      priv->expand_timer = 0;
    }

  g_clear_object (&priv->multipress_gesture);

  CTK_WIDGET_CLASS (ctk_expander_parent_class)->destroy (widget);

  g_clear_object (&priv->arrow_gadget);
  g_clear_object (&priv->title_gadget);
  g_clear_object (&priv->gadget);
}

static void
ctk_expander_realize (CtkWidget *widget)
{
  CtkAllocation title_allocation;
  CtkExpanderPrivate *priv;
  GdkWindow *window;
  GdkWindowAttr attributes;
  gint attributes_mask;

  priv = CTK_EXPANDER (widget)->priv;

  ctk_css_gadget_get_border_allocation (priv->title_gadget, &title_allocation, NULL);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = title_allocation.x;
  attributes.y = title_allocation.y;
  attributes.width = title_allocation.width;
  attributes.height = title_allocation.height;
  attributes.wclass = GDK_INPUT_ONLY;
  attributes.event_mask = ctk_widget_get_events (widget)
                          | GDK_BUTTON_PRESS_MASK
                          | GDK_BUTTON_RELEASE_MASK
                          | GDK_ENTER_NOTIFY_MASK
                          | GDK_LEAVE_NOTIFY_MASK;

  attributes_mask = GDK_WA_X | GDK_WA_Y;

  window = ctk_widget_get_parent_window (widget);
  ctk_widget_set_window (widget, window);
  g_object_ref (window);

  priv->event_window = cdk_window_new (ctk_widget_get_parent_window (widget),
                                       &attributes, attributes_mask);
  ctk_widget_register_window (widget, priv->event_window);

  ctk_gesture_set_window (priv->multipress_gesture, priv->event_window);
  ctk_widget_set_realized (widget, TRUE);
}

static void
ctk_expander_unrealize (CtkWidget *widget)
{
  CtkExpanderPrivate *priv = CTK_EXPANDER (widget)->priv;

  if (priv->event_window)
    {
      ctk_gesture_set_window (priv->multipress_gesture, NULL);
      ctk_widget_unregister_window (widget, priv->event_window);
      cdk_window_destroy (priv->event_window);
      priv->event_window = NULL;
    }

  CTK_WIDGET_CLASS (ctk_expander_parent_class)->unrealize (widget);
}

static void
ctk_expander_size_allocate (CtkWidget     *widget,
                            CtkAllocation *allocation)
{
  CtkExpanderPrivate *priv = CTK_EXPANDER (widget)->priv;
  CtkAllocation clip;

  ctk_widget_set_allocation (widget, allocation);

  ctk_css_gadget_allocate (priv->gadget,
                           allocation,
                           ctk_widget_get_allocated_baseline (widget),
                           &clip);

  ctk_widget_set_clip (widget, &clip);

  if (ctk_widget_get_realized (widget))
    {
      CtkAllocation title_allocation;

      ctk_css_gadget_get_border_allocation (priv->title_gadget, &title_allocation, NULL);
      cdk_window_move_resize (priv->event_window,
                              title_allocation.x, title_allocation.y,
                              title_allocation.width, title_allocation.height);
    }
}

static void
ctk_expander_map (CtkWidget *widget)
{
  CtkExpanderPrivate *priv = CTK_EXPANDER (widget)->priv;

  if (priv->label_widget)
    ctk_widget_map (priv->label_widget);

  CTK_WIDGET_CLASS (ctk_expander_parent_class)->map (widget);

  if (priv->event_window)
    cdk_window_show (priv->event_window);
}

static void
ctk_expander_unmap (CtkWidget *widget)
{
  CtkExpanderPrivate *priv = CTK_EXPANDER (widget)->priv;

  if (priv->event_window)
    cdk_window_hide (priv->event_window);

  CTK_WIDGET_CLASS (ctk_expander_parent_class)->unmap (widget);

  if (priv->label_widget)
    ctk_widget_unmap (priv->label_widget);
}

static gboolean
ctk_expander_draw (CtkWidget *widget,
                   cairo_t   *cr)
{
  ctk_css_gadget_draw (CTK_EXPANDER (widget)->priv->gadget, cr);

  return FALSE;
}

static void
gesture_multipress_released_cb (CtkGestureMultiPress *gesture,
                                gint                  n_press,
                                gdouble               x,
                                gdouble               y,
                                CtkExpander          *expander)
{
  if (expander->priv->prelight)
    ctk_widget_activate (CTK_WIDGET (expander));
}

static void
ctk_expander_redraw_expander (CtkExpander *expander)
{
  CtkAllocation allocation;
  CtkWidget *widget = CTK_WIDGET (expander);

  if (ctk_widget_get_realized (widget))
    {
      ctk_widget_get_allocation (widget, &allocation);
      cdk_window_invalidate_rect (ctk_widget_get_window (widget), &allocation, FALSE);
    }
}

static void
update_node_state (CtkExpander *expander)
{
  CtkExpanderPrivate *priv = expander->priv;
  CtkStateFlags state;

  state = ctk_widget_get_state_flags (CTK_WIDGET (expander));

  if (priv->prelight)
    state |= CTK_STATE_FLAG_PRELIGHT;
  else
    state &= ~CTK_STATE_FLAG_PRELIGHT;

  ctk_css_gadget_set_state (priv->title_gadget, state);

  if (priv->expanded)
    state |= CTK_STATE_FLAG_CHECKED;
  else
    state &= ~CTK_STATE_FLAG_CHECKED;

  ctk_css_gadget_set_state (priv->arrow_gadget, state);
}

static void
ctk_expander_state_flags_changed (CtkWidget     *widget,
                                  CtkStateFlags  previous_state)
{
  update_node_state (CTK_EXPANDER (widget));

  CTK_WIDGET_CLASS (ctk_expander_parent_class)->state_flags_changed (widget, previous_state);
}

static void
ctk_expander_direction_changed (CtkWidget        *widget,
                                CtkTextDirection  previous_direction)
{
  CtkExpanderPrivate *priv = CTK_EXPANDER (widget)->priv;
  CtkAlign align;

  ctk_box_gadget_reverse_children (CTK_BOX_GADGET (priv->title_gadget));

  align = ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL ? CTK_ALIGN_END : CTK_ALIGN_START;
  ctk_box_gadget_remove_gadget (CTK_BOX_GADGET (priv->gadget), priv->title_gadget);
  ctk_box_gadget_insert_gadget (CTK_BOX_GADGET (priv->gadget), 0, priv->title_gadget, FALSE, align);

  ctk_box_gadget_set_allocate_reverse (CTK_BOX_GADGET (priv->title_gadget),
                                       ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL);
  ctk_box_gadget_set_align_reverse (CTK_BOX_GADGET (priv->title_gadget),
                                    ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL);

  CTK_WIDGET_CLASS (ctk_expander_parent_class)->direction_changed (widget, previous_direction);
}

static gboolean
ctk_expander_enter_notify (CtkWidget        *widget,
                           GdkEventCrossing *event)
{
  CtkExpander *expander = CTK_EXPANDER (widget);

  if (event->window == expander->priv->event_window &&
      event->detail != GDK_NOTIFY_INFERIOR)
    {
      expander->priv->prelight = TRUE;

      update_node_state (expander);

      if (expander->priv->label_widget)
        ctk_widget_set_state_flags (expander->priv->label_widget,
                                    CTK_STATE_FLAG_PRELIGHT,
                                    FALSE);

      ctk_expander_redraw_expander (expander);
    }

  return FALSE;
}

static gboolean
ctk_expander_leave_notify (CtkWidget        *widget,
                           GdkEventCrossing *event)
{
  CtkExpander *expander = CTK_EXPANDER (widget);

  if (event->window == expander->priv->event_window &&
      event->detail != GDK_NOTIFY_INFERIOR)
    {
      expander->priv->prelight = FALSE;

      update_node_state (expander);

      if (expander->priv->label_widget)
        ctk_widget_unset_state_flags (expander->priv->label_widget,
                                      CTK_STATE_FLAG_PRELIGHT);

      ctk_expander_redraw_expander (expander);
    }

  return FALSE;
}

static gboolean
expand_timeout (gpointer data)
{
  CtkExpander *expander = CTK_EXPANDER (data);
  CtkExpanderPrivate *priv = expander->priv;

  priv->expand_timer = 0;
  ctk_expander_set_expanded (expander, TRUE);

  return FALSE;
}

static gboolean
ctk_expander_drag_motion (CtkWidget        *widget,
                          GdkDragContext   *context,
                          gint              x,
                          gint              y,
                          guint             time)
{
  CtkExpander *expander = CTK_EXPANDER (widget);
  CtkExpanderPrivate *priv = expander->priv;

  if (!priv->expanded && !priv->expand_timer)
    {
      priv->expand_timer = cdk_threads_add_timeout (TIMEOUT_EXPAND, (GSourceFunc) expand_timeout, expander);
      g_source_set_name_by_id (priv->expand_timer, "[ctk+] expand_timeout");
    }

  return TRUE;
}

static void
ctk_expander_drag_leave (CtkWidget      *widget,
                         GdkDragContext *context,
                         guint           time)
{
  CtkExpander *expander = CTK_EXPANDER (widget);
  CtkExpanderPrivate *priv = expander->priv;

  if (priv->expand_timer)
    {
      g_source_remove (priv->expand_timer);
      priv->expand_timer = 0;
    }
}

typedef enum
{
  FOCUS_NONE,
  FOCUS_WIDGET,
  FOCUS_LABEL,
  FOCUS_CHILD
} FocusSite;

static gboolean
focus_current_site (CtkExpander      *expander,
                    CtkDirectionType  direction)
{
  CtkWidget *current_focus;

  current_focus = ctk_container_get_focus_child (CTK_CONTAINER (expander));

  if (!current_focus)
    return FALSE;

  return ctk_widget_child_focus (current_focus, direction);
}

static gboolean
focus_in_site (CtkExpander      *expander,
               FocusSite         site,
               CtkDirectionType  direction)
{
  switch (site)
    {
    case FOCUS_WIDGET:
      ctk_widget_grab_focus (CTK_WIDGET (expander));
      return TRUE;
    case FOCUS_LABEL:
      if (expander->priv->label_widget)
        return ctk_widget_child_focus (expander->priv->label_widget, direction);
      else
        return FALSE;
    case FOCUS_CHILD:
      {
        CtkWidget *child = ctk_bin_get_child (CTK_BIN (expander));

        if (child && ctk_widget_get_child_visible (child))
          return ctk_widget_child_focus (child, direction);
        else
          return FALSE;
      }
    case FOCUS_NONE:
      break;
    }

  g_assert_not_reached ();
  return FALSE;
}

static FocusSite
get_next_site (CtkExpander      *expander,
               FocusSite         site,
               CtkDirectionType  direction)
{
  gboolean ltr;

  ltr = ctk_widget_get_direction (CTK_WIDGET (expander)) != CTK_TEXT_DIR_RTL;

  switch (site)
    {
    case FOCUS_NONE:
      switch (direction)
        {
        case CTK_DIR_TAB_BACKWARD:
        case CTK_DIR_LEFT:
        case CTK_DIR_UP:
          return FOCUS_CHILD;
        case CTK_DIR_TAB_FORWARD:
        case CTK_DIR_DOWN:
        case CTK_DIR_RIGHT:
          return FOCUS_WIDGET;
        }
      break;
    case FOCUS_WIDGET:
      switch (direction)
        {
        case CTK_DIR_TAB_BACKWARD:
        case CTK_DIR_UP:
          return FOCUS_NONE;
        case CTK_DIR_LEFT:
          return ltr ? FOCUS_NONE : FOCUS_LABEL;
        case CTK_DIR_TAB_FORWARD:
        case CTK_DIR_DOWN:
          return FOCUS_LABEL;
        case CTK_DIR_RIGHT:
          return ltr ? FOCUS_LABEL : FOCUS_NONE;
        }
      break;
    case FOCUS_LABEL:
      switch (direction)
        {
        case CTK_DIR_TAB_BACKWARD:
        case CTK_DIR_UP:
          return FOCUS_WIDGET;
        case CTK_DIR_LEFT:
          return ltr ? FOCUS_WIDGET : FOCUS_CHILD;
        case CTK_DIR_TAB_FORWARD:
        case CTK_DIR_DOWN:
          return FOCUS_CHILD;
        case CTK_DIR_RIGHT:
          return ltr ? FOCUS_CHILD : FOCUS_WIDGET;
        }
      break;
    case FOCUS_CHILD:
      switch (direction)
        {
        case CTK_DIR_TAB_BACKWARD:
        case CTK_DIR_LEFT:
        case CTK_DIR_UP:
          return FOCUS_LABEL;
        case CTK_DIR_TAB_FORWARD:
        case CTK_DIR_DOWN:
        case CTK_DIR_RIGHT:
          return FOCUS_NONE;
        }
      break;
    }

  g_assert_not_reached ();
  return FOCUS_NONE;
}

static void
ctk_expander_resize_toplevel (CtkExpander *expander)
{
  CtkExpanderPrivate *priv = expander->priv;
  CtkWidget *child = ctk_bin_get_child (CTK_BIN (expander));

  if (child && priv->resize_toplevel &&
      ctk_widget_get_realized (CTK_WIDGET (expander)))
    {
      CtkWidget *toplevel = ctk_widget_get_toplevel (CTK_WIDGET (expander));

      if (toplevel && CTK_IS_WINDOW (toplevel) &&
          ctk_widget_get_realized (toplevel))
        {
          int toplevel_width, toplevel_height;
          int child_height;

          ctk_widget_get_preferred_height (child, &child_height, NULL);
          ctk_window_get_size (CTK_WINDOW (toplevel), &toplevel_width, &toplevel_height);

          if (priv->expanded)
            toplevel_height += child_height;
          else
            toplevel_height -= child_height;

          ctk_window_resize (CTK_WINDOW (toplevel),
                             toplevel_width,
                             toplevel_height);
        }
    }
}

static gboolean
ctk_expander_focus (CtkWidget        *widget,
                    CtkDirectionType  direction)
{
  CtkExpander *expander = CTK_EXPANDER (widget);

  if (!focus_current_site (expander, direction))
    {
      CtkWidget *old_focus_child;
      gboolean widget_is_focus;
      FocusSite site = FOCUS_NONE;

      widget_is_focus = ctk_widget_is_focus (widget);
      old_focus_child = ctk_container_get_focus_child (CTK_CONTAINER (widget));

      if (old_focus_child && old_focus_child == expander->priv->label_widget)
        site = FOCUS_LABEL;
      else if (old_focus_child)
        site = FOCUS_CHILD;
      else if (widget_is_focus)
        site = FOCUS_WIDGET;

      while ((site = get_next_site (expander, site, direction)) != FOCUS_NONE)
        {
          if (focus_in_site (expander, site, direction))
            return TRUE;
        }

      return FALSE;
    }

  return TRUE;
}

static void
ctk_expander_update_child_mapped (CtkExpander *expander,
                                  CtkWidget   *child)
{
  /* If collapsing, we must unmap the child, as ctk_box_gadget_remove() does
   * not, so otherwise the child is not drawn but still consumes input in-place.
   */

  if (expander->priv->expanded &&
      ctk_widget_get_realized (child) &&
      ctk_widget_get_visible (child))
    {
      ctk_widget_map (child);
    }
  else
    {
      ctk_widget_unmap (child);
    }
}

static void
ctk_expander_add (CtkContainer *container,
                  CtkWidget    *widget)
{
  CtkExpander *expander = CTK_EXPANDER (container);

  CTK_CONTAINER_CLASS (ctk_expander_parent_class)->add (container, widget);

  ctk_widget_queue_resize (CTK_WIDGET (container));

  if (expander->priv->expanded)
    ctk_box_gadget_insert_widget (CTK_BOX_GADGET (expander->priv->gadget), -1, widget);

  ctk_expander_update_child_mapped (expander, widget);
}

static void
ctk_expander_remove (CtkContainer *container,
                     CtkWidget    *widget)
{
  CtkExpander *expander = CTK_EXPANDER (container);

  if (CTK_EXPANDER (expander)->priv->label_widget == widget)
    ctk_expander_set_label_widget (expander, NULL);
  else
    {
      ctk_box_gadget_remove_widget (CTK_BOX_GADGET (expander->priv->gadget), widget);
      CTK_CONTAINER_CLASS (ctk_expander_parent_class)->remove (container, widget);
    }
}

static void
ctk_expander_forall (CtkContainer *container,
                     gboolean      include_internals,
                     CtkCallback   callback,
                     gpointer      callback_data)
{
  CtkBin *bin = CTK_BIN (container);
  CtkExpanderPrivate *priv = CTK_EXPANDER (container)->priv;
  CtkWidget *child;

  child = ctk_bin_get_child (bin);
  if (child)
    (* callback) (child, callback_data);

  if (priv->label_widget)
    (* callback) (priv->label_widget, callback_data);
}

static void
ctk_expander_activate (CtkExpander *expander)
{
  ctk_expander_set_expanded (expander, !expander->priv->expanded);
}

static void
ctk_expander_get_preferred_width (CtkWidget *widget,
                                  gint      *minimum_size,
                                  gint      *natural_size)
{
  ctk_css_gadget_get_preferred_size (CTK_EXPANDER (widget)->priv->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     minimum_size, natural_size,
                                     NULL, NULL);
}

static void
ctk_expander_get_preferred_height (CtkWidget *widget,
                                   gint      *minimum_size,
                                   gint      *natural_size)
{
  ctk_css_gadget_get_preferred_size (CTK_EXPANDER (widget)->priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     -1,
                                     minimum_size, natural_size,
                                     NULL, NULL);
}

static void
ctk_expander_get_preferred_width_for_height (CtkWidget *widget,
                                             gint       height,
                                             gint      *minimum_size,
                                             gint      *natural_size)
{
  ctk_css_gadget_get_preferred_size (CTK_EXPANDER (widget)->priv->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     height,
                                     minimum_size, natural_size,
                                     NULL, NULL);
}

static void
ctk_expander_get_preferred_height_for_width (CtkWidget *widget,
                                             gint       width,
                                             gint      *minimum_size,
                                             gint      *natural_size)
{
  ctk_css_gadget_get_preferred_size (CTK_EXPANDER (widget)->priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     width,
                                     minimum_size, natural_size,
                                     NULL, NULL);
}

/**
 * ctk_expander_new:
 * @label: (nullable): the text of the label
 *
 * Creates a new expander using @label as the text of the label.
 *
 * Returns: a new #CtkExpander widget.
 *
 * Since: 2.4
 */
CtkWidget *
ctk_expander_new (const gchar *label)
{
  return g_object_new (CTK_TYPE_EXPANDER, "label", label, NULL);
}

/**
 * ctk_expander_new_with_mnemonic:
 * @label: (nullable): the text of the label with an underscore
 *     in front of the mnemonic character
 *
 * Creates a new expander using @label as the text of the label.
 * If characters in @label are preceded by an underscore, they are underlined.
 * If you need a literal underscore character in a label, use “__” (two
 * underscores). The first underlined character represents a keyboard
 * accelerator called a mnemonic.
 * Pressing Alt and that key activates the button.
 *
 * Returns: a new #CtkExpander widget.
 *
 * Since: 2.4
 */
CtkWidget *
ctk_expander_new_with_mnemonic (const gchar *label)
{
  return g_object_new (CTK_TYPE_EXPANDER,
                       "label", label,
                       "use-underline", TRUE,
                       NULL);
}

/**
 * ctk_expander_set_expanded:
 * @expander: a #CtkExpander
 * @expanded: whether the child widget is revealed
 *
 * Sets the state of the expander. Set to %TRUE, if you want
 * the child widget to be revealed, and %FALSE if you want the
 * child widget to be hidden.
 *
 * Since: 2.4
 */
void
ctk_expander_set_expanded (CtkExpander *expander,
                           gboolean     expanded)
{
  CtkExpanderPrivate *priv;
  CtkWidget *child;

  g_return_if_fail (CTK_IS_EXPANDER (expander));

  priv = expander->priv;

  expanded = expanded != FALSE;

  if (priv->expanded == expanded)
    return;

  priv->expanded = expanded;

  update_node_state (expander);

  child = ctk_bin_get_child (CTK_BIN (expander));

  if (child)
    {
      if (priv->expanded)
        ctk_box_gadget_insert_widget (CTK_BOX_GADGET (priv->gadget), 1, child);
      else
        ctk_box_gadget_remove_widget (CTK_BOX_GADGET (priv->gadget), child);

      ctk_widget_queue_resize (CTK_WIDGET (expander));
      ctk_expander_resize_toplevel (expander);

      ctk_expander_update_child_mapped (expander, child);
    }

  g_object_notify (G_OBJECT (expander), "expanded");
}

/**
 * ctk_expander_get_expanded:
 * @expander:a #CtkExpander
 *
 * Queries a #CtkExpander and returns its current state. Returns %TRUE
 * if the child widget is revealed.
 *
 * See ctk_expander_set_expanded().
 *
 * Returns: the current state of the expander
 *
 * Since: 2.4
 */
gboolean
ctk_expander_get_expanded (CtkExpander *expander)
{
  g_return_val_if_fail (CTK_IS_EXPANDER (expander), FALSE);

  return expander->priv->expanded;
}

/**
 * ctk_expander_set_spacing:
 * @expander: a #CtkExpander
 * @spacing: distance between the expander and child in pixels
 *
 * Sets the spacing field of @expander, which is the number of
 * pixels to place between expander and the child.
 *
 * Since: 2.4
 *
 * Deprecated: 3.20: Use margins on the child instead.
 */
void
ctk_expander_set_spacing (CtkExpander *expander,
                          gint         spacing)
{
  g_return_if_fail (CTK_IS_EXPANDER (expander));
  g_return_if_fail (spacing >= 0);

  if (expander->priv->spacing != spacing)
    {
      expander->priv->spacing = spacing;

      ctk_widget_queue_resize (CTK_WIDGET (expander));

      g_object_notify (G_OBJECT (expander), "spacing");
    }
}

/**
 * ctk_expander_get_spacing:
 * @expander: a #CtkExpander
 *
 * Gets the value set by ctk_expander_set_spacing().
 *
 * Returns: spacing between the expander and child
 *
 * Since: 2.4
 *
 * Deprecated: 3.20: Use margins on the child instead.
 */
gint
ctk_expander_get_spacing (CtkExpander *expander)
{
  g_return_val_if_fail (CTK_IS_EXPANDER (expander), 0);

  return expander->priv->spacing;
}

/**
 * ctk_expander_set_label:
 * @expander: a #CtkExpander
 * @label: (nullable): a string
 *
 * Sets the text of the label of the expander to @label.
 *
 * This will also clear any previously set labels.
 *
 * Since: 2.4
 */
void
ctk_expander_set_label (CtkExpander *expander,
                        const gchar *label)
{
  g_return_if_fail (CTK_IS_EXPANDER (expander));

  if (!label)
    {
      ctk_expander_set_label_widget (expander, NULL);
    }
  else
    {
      CtkWidget *child;

      child = ctk_label_new (label);
      ctk_label_set_use_underline (CTK_LABEL (child), expander->priv->use_underline);
      ctk_label_set_use_markup (CTK_LABEL (child), expander->priv->use_markup);
      ctk_widget_show (child);

      ctk_expander_set_label_widget (expander, child);
    }

  g_object_notify (G_OBJECT (expander), "label");
}

/**
 * ctk_expander_get_label:
 * @expander: a #CtkExpander
 *
 * Fetches the text from a label widget including any embedded
 * underlines indicating mnemonics and Pango markup, as set by
 * ctk_expander_set_label(). If the label text has not been set the
 * return value will be %NULL. This will be the case if you create an
 * empty button with ctk_button_new() to use as a container.
 *
 * Note that this function behaved differently in versions prior to
 * 2.14 and used to return the label text stripped of embedded
 * underlines indicating mnemonics and Pango markup. This problem can
 * be avoided by fetching the label text directly from the label
 * widget.
 *
 * Returns: (nullable): The text of the label widget. This string is owned
 *     by the widget and must not be modified or freed.
 *
 * Since: 2.4
 */
const gchar *
ctk_expander_get_label (CtkExpander *expander)
{
  CtkExpanderPrivate *priv;

  g_return_val_if_fail (CTK_IS_EXPANDER (expander), NULL);

  priv = expander->priv;

  if (CTK_IS_LABEL (priv->label_widget))
    return ctk_label_get_label (CTK_LABEL (priv->label_widget));
  else
    return NULL;
}

/**
 * ctk_expander_set_use_underline:
 * @expander: a #CtkExpander
 * @use_underline: %TRUE if underlines in the text indicate mnemonics
 *
 * If true, an underline in the text of the expander label indicates
 * the next character should be used for the mnemonic accelerator key.
 *
 * Since: 2.4
 */
void
ctk_expander_set_use_underline (CtkExpander *expander,
                                gboolean     use_underline)
{
  CtkExpanderPrivate *priv;

  g_return_if_fail (CTK_IS_EXPANDER (expander));

  priv = expander->priv;

  use_underline = use_underline != FALSE;

  if (priv->use_underline != use_underline)
    {
      priv->use_underline = use_underline;

      if (CTK_IS_LABEL (priv->label_widget))
        ctk_label_set_use_underline (CTK_LABEL (priv->label_widget), use_underline);

      g_object_notify (G_OBJECT (expander), "use-underline");
    }
}

/**
 * ctk_expander_get_use_underline:
 * @expander: a #CtkExpander
 *
 * Returns whether an embedded underline in the expander label
 * indicates a mnemonic. See ctk_expander_set_use_underline().
 *
 * Returns: %TRUE if an embedded underline in the expander
 *     label indicates the mnemonic accelerator keys
 *
 * Since: 2.4
 */
gboolean
ctk_expander_get_use_underline (CtkExpander *expander)
{
  g_return_val_if_fail (CTK_IS_EXPANDER (expander), FALSE);

  return expander->priv->use_underline;
}

/**
 * ctk_expander_set_use_markup:
 * @expander: a #CtkExpander
 * @use_markup: %TRUE if the label’s text should be parsed for markup
 *
 * Sets whether the text of the label contains markup in
 * [Pango’s text markup language][PangoMarkupFormat].
 * See ctk_label_set_markup().
 *
 * Since: 2.4
 */
void
ctk_expander_set_use_markup (CtkExpander *expander,
                             gboolean     use_markup)
{
  CtkExpanderPrivate *priv;

  g_return_if_fail (CTK_IS_EXPANDER (expander));

  priv = expander->priv;

  use_markup = use_markup != FALSE;

  if (priv->use_markup != use_markup)
    {
      priv->use_markup = use_markup;

      if (CTK_IS_LABEL (priv->label_widget))
        ctk_label_set_use_markup (CTK_LABEL (priv->label_widget), use_markup);

      g_object_notify (G_OBJECT (expander), "use-markup");
    }
}

/**
 * ctk_expander_get_use_markup:
 * @expander: a #CtkExpander
 *
 * Returns whether the label’s text is interpreted as marked up with
 * the [Pango text markup language][PangoMarkupFormat].
 * See ctk_expander_set_use_markup().
 *
 * Returns: %TRUE if the label’s text will be parsed for markup
 *
 * Since: 2.4
 */
gboolean
ctk_expander_get_use_markup (CtkExpander *expander)
{
  g_return_val_if_fail (CTK_IS_EXPANDER (expander), FALSE);

  return expander->priv->use_markup;
}

/**
 * ctk_expander_set_label_widget:
 * @expander: a #CtkExpander
 * @label_widget: (nullable): the new label widget
 *
 * Set the label widget for the expander. This is the widget
 * that will appear embedded alongside the expander arrow.
 *
 * Since: 2.4
 */
void
ctk_expander_set_label_widget (CtkExpander *expander,
                               CtkWidget   *label_widget)
{
  CtkExpanderPrivate *priv;
  CtkWidget *widget;
  int pos;

  g_return_if_fail (CTK_IS_EXPANDER (expander));
  g_return_if_fail (label_widget == NULL || CTK_IS_WIDGET (label_widget));
  g_return_if_fail (label_widget == NULL || ctk_widget_get_parent (label_widget) == NULL);

  priv = expander->priv;

  if (priv->label_widget == label_widget)
    return;

  if (priv->label_widget)
    {
      ctk_box_gadget_remove_widget (CTK_BOX_GADGET (priv->title_gadget), priv->label_widget);
      ctk_widget_set_state_flags (priv->label_widget, 0, TRUE);
      ctk_widget_unparent (priv->label_widget);
    }

  priv->label_widget = label_widget;
  widget = CTK_WIDGET (expander);

  if (label_widget)
    {
      priv->label_widget = label_widget;
      ctk_widget_set_parent (label_widget, widget);

      if (priv->prelight)
        ctk_widget_set_state_flags (label_widget, CTK_STATE_FLAG_PRELIGHT, FALSE);

      pos = ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL ? 0 : 1;
      ctk_box_gadget_insert_widget (CTK_BOX_GADGET (priv->title_gadget), pos, label_widget);
    }

  if (ctk_widget_get_visible (widget))
    ctk_widget_queue_resize (widget);

  g_object_freeze_notify (G_OBJECT (expander));
  g_object_notify (G_OBJECT (expander), "label-widget");
  g_object_notify (G_OBJECT (expander), "label");
  g_object_thaw_notify (G_OBJECT (expander));
}

/**
 * ctk_expander_get_label_widget:
 * @expander: a #CtkExpander
 *
 * Retrieves the label widget for the frame. See
 * ctk_expander_set_label_widget().
 *
 * Returns: (nullable) (transfer none): the label widget,
 *     or %NULL if there is none
 *
 * Since: 2.4
 */
CtkWidget *
ctk_expander_get_label_widget (CtkExpander *expander)
{
  g_return_val_if_fail (CTK_IS_EXPANDER (expander), NULL);

  return expander->priv->label_widget;
}

/**
 * ctk_expander_set_label_fill:
 * @expander: a #CtkExpander
 * @label_fill: %TRUE if the label should should fill
 *     all available horizontal space
 *
 * Sets whether the label widget should fill all available
 * horizontal space allocated to @expander.
 *
 * Note that this function has no effect since 3.20.
 *
 * Since: 2.22
 */
void
ctk_expander_set_label_fill (CtkExpander *expander,
                             gboolean     label_fill)
{
  CtkExpanderPrivate *priv;

  g_return_if_fail (CTK_IS_EXPANDER (expander));

  priv = expander->priv;

  label_fill = label_fill != FALSE;

  if (priv->label_fill != label_fill)
    {
      priv->label_fill = label_fill;

      if (priv->label_widget != NULL)
        ctk_widget_queue_resize (CTK_WIDGET (expander));

      g_object_notify (G_OBJECT (expander), "label-fill");
    }
}

/**
 * ctk_expander_get_label_fill:
 * @expander: a #CtkExpander
 *
 * Returns whether the label widget will fill all available
 * horizontal space allocated to @expander.
 *
 * Returns: %TRUE if the label widget will fill all
 *     available horizontal space
 *
 * Since: 2.22
 */
gboolean
ctk_expander_get_label_fill (CtkExpander *expander)
{
  g_return_val_if_fail (CTK_IS_EXPANDER (expander), FALSE);

  return expander->priv->label_fill;
}

/**
 * ctk_expander_set_resize_toplevel:
 * @expander: a #CtkExpander
 * @resize_toplevel: whether to resize the toplevel
 *
 * Sets whether the expander will resize the toplevel widget
 * containing the expander upon resizing and collpasing.
 *
 * Since: 3.2
 */
void
ctk_expander_set_resize_toplevel (CtkExpander *expander,
                                  gboolean     resize_toplevel)
{
  g_return_if_fail (CTK_IS_EXPANDER (expander));

  if (expander->priv->resize_toplevel != resize_toplevel)
    {
      expander->priv->resize_toplevel = resize_toplevel ? TRUE : FALSE;
      g_object_notify (G_OBJECT (expander), "resize-toplevel");
    }
}

/**
 * ctk_expander_get_resize_toplevel:
 * @expander: a #CtkExpander
 *
 * Returns whether the expander will resize the toplevel widget
 * containing the expander upon resizing and collpasing.
 *
 * Returns: the “resize toplevel” setting.
 *
 * Since: 3.2
 */
gboolean
ctk_expander_get_resize_toplevel (CtkExpander *expander)
{
  g_return_val_if_fail (CTK_IS_EXPANDER (expander), FALSE);

  return expander->priv->resize_toplevel;
}
