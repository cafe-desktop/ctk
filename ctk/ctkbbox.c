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

/**
 * SECTION:ctkbbox
 * @Short_description: A container for arranging buttons
 * @Title: CtkButtonBox
 *
 * A button box should be used to provide a consistent layout of buttons
 * throughout your application. The layout/spacing can be altered by the
 * programmer, or if desired, by the user to alter the “feel” of a
 * program to a small degree.
 *
 * ctk_button_box_get_layout() and ctk_button_box_set_layout() retrieve and
 * alter the method used to spread the buttons in a button box across the
 * container, respectively.
 *
 * The main purpose of CtkButtonBox is to make sure the children have all the
 * same size. CtkButtonBox gives all children the same size, but it does allow
 * 'outliers' to keep their own larger size.
 *
 * To exempt individual children from homogeneous sizing regardless of their
 * 'outlier' status, you can set the non-homogeneous child
 * property.
 *
 * # CSS nodes
 *
 * CtkButtonBox uses a single CSS node with name buttonbox.
 */

#include "config.h"

#include "ctkbbox.h"

#include "ctkboxprivate.h"
#include "ctkorientable.h"
#include "ctktypebuiltins.h"
#include "ctkprivate.h"
#include "ctksizerequest.h"
#include "ctkwidgetprivate.h"
#include "ctkcontainerprivate.h"
#include "ctkcsscustomgadgetprivate.h"
#include "ctkintl.h"


struct _CtkButtonBoxPrivate
{
  CtkButtonBoxStyle layout_style;
  CtkCssGadget *gadget;
};

enum {
  PROP_0,
  PROP_LAYOUT_STYLE
};

enum {
  CHILD_PROP_0,
  CHILD_PROP_SECONDARY,
  CHILD_PROP_NONHOMOGENEOUS
};

#define CTK_BOX_SECONDARY_CHILD "ctk-box-secondary-child"
#define CTK_BOX_NON_HOMOGENEOUS "ctk-box-non-homogeneous"

static void ctk_button_box_set_property       (GObject           *object,
                                               guint              prop_id,
                                               const GValue      *value,
                                               GParamSpec        *pspec);
static void ctk_button_box_get_property       (GObject           *object,
                                               guint              prop_id,
                                               GValue            *value,
                                               GParamSpec        *pspec);
static gboolean ctk_button_box_draw           (CtkWidget         *widget,
                                               cairo_t           *cr);
static void ctk_button_box_get_preferred_width            (CtkWidget *widget,
                                                           gint      *minimum,
                                                           gint      *natural);
static void ctk_button_box_get_preferred_height           (CtkWidget *widget,
                                                           gint      *minimum,
                                                           gint      *natural);
static void ctk_button_box_get_preferred_width_for_height (CtkWidget *widget,
                                                           gint       height,
                                                           gint      *minimum,
                                                           gint      *natural);
static void ctk_button_box_get_preferred_height_for_width (CtkWidget *widget,
                                                           gint       width,
                                                           gint      *minimum,
                                                           gint      *natural);
static void ctk_button_box_get_preferred_height_and_baseline_for_width (CtkWidget *widget,
									gint       width,
									gint      *minimum,
									gint      *natural,
									gint      *minimum_baseline,
									gint      *natural_baseline);

static void ctk_button_box_size_allocate      (CtkWidget         *widget,
                                               CtkAllocation     *allocation);
static void ctk_button_box_remove             (CtkContainer      *container,
                                               CtkWidget         *widget);
static void ctk_button_box_set_child_property (CtkContainer      *container,
                                               CtkWidget         *child,
                                               guint              property_id,
                                               const GValue      *value,
                                               GParamSpec        *pspec);
static void ctk_button_box_get_child_property (CtkContainer      *container,
                                               CtkWidget         *child,
                                               guint              property_id,
                                               GValue            *value,
                                               GParamSpec        *pspec);

static void     ctk_button_box_measure         (CtkCssGadget        *gadget,
                                                CtkOrientation       orientation,
                                                int                  for_size,
                                                int                 *minimum,
                                                int                 *natural,
                                                int                 *minimum_baseline,
                                                int                 *natural_baseline,
                                                gpointer             unused);
static void     ctk_button_box_allocate        (CtkCssGadget        *gadget,
                                                const CtkAllocation *allocation,
                                                int                  baseline,
                                                CtkAllocation       *out_clip,
                                                gpointer             unused);
static gboolean ctk_button_box_render          (CtkCssGadget        *gadget,
                                                cairo_t             *cr,
                                                int                  x,
                                                int                  y,
                                                int                  width,
                                                int                  height,
                                                gpointer             data);

#define DEFAULT_CHILD_MIN_WIDTH 85
#define DEFAULT_CHILD_MIN_HEIGHT 27
#define DEFAULT_CHILD_IPAD_X 4
#define DEFAULT_CHILD_IPAD_Y 0
#define DEFAULT_LAYOUT_STYLE CTK_BUTTONBOX_EDGE

G_DEFINE_TYPE_WITH_PRIVATE (CtkButtonBox, ctk_button_box, CTK_TYPE_BOX)

static void
ctk_button_box_add (CtkContainer *container,
                    CtkWidget    *widget)
{
  ctk_box_pack_start (CTK_BOX (container), widget, TRUE, TRUE, 0);
}

static void
ctk_button_box_finalize (GObject *object)
{
  CtkButtonBox *button_box = CTK_BUTTON_BOX (object);

  g_clear_object (&button_box->priv->gadget);

  G_OBJECT_CLASS (ctk_button_box_parent_class)->finalize (object);
}

static void
ctk_button_box_class_init (CtkButtonBoxClass *class)
{
  CtkWidgetClass *widget_class;
  GObjectClass *gobject_class;
  CtkContainerClass *container_class;

  gobject_class = G_OBJECT_CLASS (class);
  widget_class = (CtkWidgetClass*) class;
  container_class = (CtkContainerClass*) class;

  gobject_class->set_property = ctk_button_box_set_property;
  gobject_class->get_property = ctk_button_box_get_property;
  gobject_class->finalize = ctk_button_box_finalize;

  widget_class->get_preferred_width = ctk_button_box_get_preferred_width;
  widget_class->get_preferred_height = ctk_button_box_get_preferred_height;
  widget_class->get_preferred_width_for_height = ctk_button_box_get_preferred_width_for_height;
  widget_class->get_preferred_height_for_width = ctk_button_box_get_preferred_height_for_width;
  widget_class->get_preferred_height_and_baseline_for_width = ctk_button_box_get_preferred_height_and_baseline_for_width;
  widget_class->size_allocate = ctk_button_box_size_allocate;
  widget_class->draw = ctk_button_box_draw;

  container_class->remove = ctk_button_box_remove;
  container_class->add = ctk_button_box_add;
  container_class->set_child_property = ctk_button_box_set_child_property;
  container_class->get_child_property = ctk_button_box_get_child_property;
  ctk_container_class_handle_border_width (container_class);

  /**
   * CtkButtonBox:child-min-width:
   *
   * The minimum width of buttons inside the box.
   *
   * Deprecated: 3.20: Use CSS min-width instead.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("child-min-width",
                                                             P_("Minimum child width"),
                                                             P_("Minimum width of buttons inside the box"),
                                                             0,
                                                             G_MAXINT,
                                                             DEFAULT_CHILD_MIN_WIDTH,
                                                             CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  /**
   * CtkButtonBox:child-min-height:
   *
   * The minimum height of buttons inside the box.
   *
   * Deprecated: 3.20: Use CSS min-height instead.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("child-min-height",
                                                             P_("Minimum child height"),
                                                             P_("Minimum height of buttons inside the box"),
                                                             0,
                                                             G_MAXINT,
                                                             DEFAULT_CHILD_MIN_HEIGHT,
                                                             CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  /**
   * CtkButtonBox:child-internal-pad-x:
   *
   * The amount to increase a child's size on either side.
   *
   * Deprecated: 3.20: Use CSS padding instead.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("child-internal-pad-x",
                                                             P_("Child internal width padding"),
                                                             P_("Amount to increase child's size on either side"),
                                                             0,
                                                             G_MAXINT,
                                                             DEFAULT_CHILD_IPAD_X,
                                                             CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  /**
   * CtkButtonBox:child-internal-pad-y:
   *
   * The amount to increase a child's size on the top and bottom.
   *
   * Deprecated: 3.20: Use CSS padding instead.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("child-internal-pad-y",
                                                             P_("Child internal height padding"),
                                                             P_("Amount to increase child's size on the top and bottom"),
                                                             0,
                                                             G_MAXINT,
                                                             DEFAULT_CHILD_IPAD_Y,
                                                             CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  g_object_class_install_property (gobject_class,
                                   PROP_LAYOUT_STYLE,
                                   g_param_spec_enum ("layout-style",
                                                      P_("Layout style"),
                                                      P_("How to lay out the buttons in the box. Possible values are: spread, edge, start and end"),
                                                      CTK_TYPE_BUTTON_BOX_STYLE,
                                                      DEFAULT_LAYOUT_STYLE,
                                                      CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  ctk_container_class_install_child_property (container_class,
                                              CHILD_PROP_SECONDARY,
                                              g_param_spec_boolean ("secondary", 
                                                                    P_("Secondary"),
                                                                    P_("If TRUE, the child appears in a secondary group of children, suitable for, e.g., help buttons"),
                                                                    FALSE,
                                                                    CTK_PARAM_READWRITE));

  ctk_container_class_install_child_property (container_class,
                                              CHILD_PROP_NONHOMOGENEOUS,
                                              g_param_spec_boolean ("non-homogeneous",
                                                                    P_("Non-Homogeneous"),
                                                                    P_("If TRUE, the child will not be subject to homogeneous sizing"),
                                                                    FALSE,
                                                                    CTK_PARAM_READWRITE));

  ctk_widget_class_set_css_name (widget_class, "buttonbox");
}

static gboolean
ctk_button_box_render (CtkCssGadget *gadget,
                       cairo_t      *cr,
                       int           x,
                       int           y,
                       int           width,
                       int           height,
                       gpointer      unused)
{
  CtkWidget *widget;
  GList *children, *l;

  widget = ctk_css_gadget_get_owner (gadget);
  children = ctk_container_get_children (CTK_CONTAINER (widget));

  for (l = children; l; l = l->next)
    {
      CtkWidget *child = l->data;

      ctk_container_propagate_draw (CTK_CONTAINER (widget), child, cr);
    }

  g_list_free (children);

  return FALSE;
}


static gboolean
ctk_button_box_draw (CtkWidget *widget,
                     cairo_t   *cr)
{
  CtkButtonBoxPrivate *priv = CTK_BUTTON_BOX (widget)->priv;
  CtkCssGadget *gadget;

  if (priv->layout_style == CTK_BUTTONBOX_EXPAND)
    gadget = ctk_box_get_gadget (CTK_BOX (widget));
  else
    gadget = priv->gadget;

  ctk_css_gadget_draw (gadget, cr);

  return FALSE;
}

static void
ctk_button_box_init (CtkButtonBox *button_box)
{
  button_box->priv = ctk_button_box_get_instance_private (button_box);
  button_box->priv->layout_style = DEFAULT_LAYOUT_STYLE;

  ctk_box_set_spacing (CTK_BOX (button_box), 0);

  button_box->priv->gadget = ctk_css_custom_gadget_new_for_node (ctk_widget_get_css_node (CTK_WIDGET (button_box)),
                                                         CTK_WIDGET (button_box),
                                                         ctk_button_box_measure,
                                                         ctk_button_box_allocate,
                                                         ctk_button_box_render,
                                                         NULL,
                                                         NULL);
}

static void
ctk_button_box_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  switch (prop_id)
    {
    case PROP_LAYOUT_STYLE:
      ctk_button_box_set_layout (CTK_BUTTON_BOX (object),
                                 g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_button_box_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  CtkButtonBoxPrivate *priv = CTK_BUTTON_BOX (object)->priv;

  switch (prop_id)
    {
    case PROP_LAYOUT_STYLE:
      g_value_set_enum (value, priv->layout_style);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_button_box_set_child_property (CtkContainer *container,
                                   CtkWidget    *child,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  switch (property_id)
    {
    case CHILD_PROP_SECONDARY:
      ctk_button_box_set_child_secondary (CTK_BUTTON_BOX (container), child,
                                          g_value_get_boolean (value));
      break;
    case CHILD_PROP_NONHOMOGENEOUS:
      ctk_button_box_set_child_non_homogeneous (CTK_BUTTON_BOX (container), child,
                                                g_value_get_boolean (value));
      break;
    default:
      CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
ctk_button_box_get_child_property (CtkContainer *container,
                                   CtkWidget    *child,
                                   guint         property_id,
                                   GValue       *value,
                                   GParamSpec   *pspec)
{
  switch (property_id)
    {
    case CHILD_PROP_SECONDARY:
      g_value_set_boolean (value,
                           ctk_button_box_get_child_secondary (CTK_BUTTON_BOX (container),
                                                               child));
      break;
    case CHILD_PROP_NONHOMOGENEOUS:
      g_value_set_boolean (value,
                           ctk_button_box_get_child_non_homogeneous (CTK_BUTTON_BOX (container),
                                                                     child));
      break;
    default:
      CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
ctk_button_box_remove (CtkContainer *container,
                       CtkWidget    *widget)
{
  /* clear is_secondary and nonhomogeneous flag in case the widget
   * is added to another container
   */
  ctk_button_box_set_child_secondary (CTK_BUTTON_BOX (container), widget, FALSE);
  ctk_button_box_set_child_non_homogeneous (CTK_BUTTON_BOX (container), widget, FALSE);

  CTK_CONTAINER_CLASS (ctk_button_box_parent_class)->remove (container, widget);
}

/**
 * ctk_button_box_set_layout:
 * @widget: a #CtkButtonBox
 * @layout_style: the new layout style
 *
 * Changes the way buttons are arranged in their container.
 */
void
ctk_button_box_set_layout (CtkButtonBox      *widget,
                           CtkButtonBoxStyle  layout_style)
{
  CtkButtonBoxPrivate *priv;

  g_return_if_fail (CTK_IS_BUTTON_BOX (widget));

  priv = widget->priv;

  if (priv->layout_style != layout_style)
    {
      priv->layout_style = layout_style;

      if (priv->layout_style == CTK_BUTTONBOX_EXPAND)
        {
          ctk_style_context_add_class (ctk_widget_get_style_context (CTK_WIDGET (widget)), "linked");
          ctk_box_set_spacing (CTK_BOX (widget), 0);
          ctk_box_set_homogeneous (CTK_BOX (widget), TRUE);
        }
      else
        {
          ctk_style_context_remove_class (ctk_widget_get_style_context (CTK_WIDGET (widget)), "linked");
          ctk_box_set_homogeneous (CTK_BOX (widget), FALSE);
        }

      g_object_notify (G_OBJECT (widget), "layout-style");
      ctk_widget_queue_resize (CTK_WIDGET (widget));
    }
}

/**
 * ctk_button_box_get_layout:
 * @widget: a #CtkButtonBox
 *
 * Retrieves the method being used to arrange the buttons in a button box.
 *
 * Returns: the method used to lay out buttons in @widget.
 */
CtkButtonBoxStyle
ctk_button_box_get_layout (CtkButtonBox *widget)
{
  g_return_val_if_fail (CTK_IS_BUTTON_BOX (widget), DEFAULT_LAYOUT_STYLE);

  return widget->priv->layout_style;
}

/**
 * ctk_button_box_get_child_secondary:
 * @widget: a #CtkButtonBox
 * @child: a child of @widget
 *
 * Returns whether @child should appear in a secondary group of children.
 *
 * Returns: whether @child should appear in a secondary group of children.
 *
 * Since: 2.4
 **/
gboolean
ctk_button_box_get_child_secondary (CtkButtonBox *widget,
                                    CtkWidget    *child)
{
  g_return_val_if_fail (CTK_IS_BUTTON_BOX (widget), FALSE);
  g_return_val_if_fail (CTK_IS_WIDGET (child), FALSE);

  return (g_object_get_data (G_OBJECT (child), CTK_BOX_SECONDARY_CHILD) != NULL);
}

/**
 * ctk_button_box_set_child_secondary:
 * @widget: a #CtkButtonBox
 * @child: a child of @widget
 * @is_secondary: if %TRUE, the @child appears in a secondary group of the
 *                button box.
 *
 * Sets whether @child should appear in a secondary group of children.
 * A typical use of a secondary child is the help button in a dialog.
 *
 * This group appears after the other children if the style
 * is %CTK_BUTTONBOX_START, %CTK_BUTTONBOX_SPREAD or
 * %CTK_BUTTONBOX_EDGE, and before the other children if the style
 * is %CTK_BUTTONBOX_END. For horizontal button boxes, the definition
 * of before/after depends on direction of the widget (see
 * ctk_widget_set_direction()). If the style is %CTK_BUTTONBOX_START
 * or %CTK_BUTTONBOX_END, then the secondary children are aligned at
 * the other end of the button box from the main children. For the
 * other styles, they appear immediately next to the main children.
 **/
void
ctk_button_box_set_child_secondary (CtkButtonBox *widget,
                                    CtkWidget    *child,
                                    gboolean      is_secondary)
{
  CtkButtonBox *bbox;

  g_return_if_fail (CTK_IS_BUTTON_BOX (widget));
  g_return_if_fail (CTK_IS_WIDGET (child));
  g_return_if_fail (ctk_widget_get_parent (child) == CTK_WIDGET (widget));

  bbox = CTK_BUTTON_BOX (widget);

  g_object_set_data (G_OBJECT (child),
                     CTK_BOX_SECONDARY_CHILD,
                     is_secondary ? GINT_TO_POINTER (1) : NULL);
  ctk_widget_child_notify (child, "secondary");

  if (bbox->priv->layout_style == CTK_BUTTONBOX_EXPAND)
    {
      ctk_box_reorder_child (CTK_BOX (bbox), child, is_secondary ? 0 : -1);
    }

  if (ctk_widget_get_visible (CTK_WIDGET (widget)) &&
      ctk_widget_get_visible (child))
    ctk_widget_queue_resize (child);
}

/* Ask children how much space they require and round up
 * to match minimum size and internal padding.
 * Returns the size each single child should have.
 */
static void
ctk_button_box_child_requisition (CtkWidget  *widget,
                                  gint       *nvis_children,
                                  gint       *nvis_secondaries,
                                  gint      **widths,
                                  gint      **heights,
                                  gint      **baselines,
				  gint       *baseline,
				  gint       *baseline_height)
{
  CtkButtonBox *bbox;
  GList *children, *list;
  gint nchildren;
  gint nsecondaries;
  gint needed_width;
  gint needed_height;
  gint needed_above, needed_below;
  gint avg_w, avg_h;
  CtkRequisition child_requisition;
  gint ipad_w;
  gint ipad_h;
  gint child_min_width;
  gint child_min_height;
  gint ipad_x;
  gint ipad_y;
  gboolean homogeneous;
  gint i;
  gint max_above, max_below, child_baseline;
  CtkOrientation orientation;
  gboolean have_baseline;

  g_return_if_fail (CTK_IS_BUTTON_BOX (widget));

  bbox = CTK_BUTTON_BOX (widget);

  orientation = ctk_orientable_get_orientation (CTK_ORIENTABLE (widget));
  homogeneous = ctk_box_get_homogeneous (CTK_BOX (widget));

  ctk_widget_style_get (widget,
                        "child-min-width", &child_min_width,
                        "child-min-height", &child_min_height,
                        "child-internal-pad-x", &ipad_x,
                        "child-internal-pad-y", &ipad_y,
                        NULL);

  nchildren = 0;
  nsecondaries = 0;
  list = children = _ctk_box_get_children (CTK_BOX (bbox));
  needed_width = child_min_width;
  needed_height = child_min_height;
  needed_above = 0;
  needed_below = 0;
  ipad_w = ipad_x * 2;
  ipad_h = ipad_y * 2;

  have_baseline = FALSE;
  max_above = max_below = 0;
  avg_w = avg_h = 0;
  for (children = list; children != NULL; children = children->next)
    {
      CtkWidget *child;

      child = children->data;

      if (ctk_widget_get_visible (child))
        {
          nchildren += 1;
          _ctk_widget_get_preferred_size_and_baseline (child,
                                                       &child_requisition, NULL, &child_baseline, NULL);
	  if (orientation == CTK_ORIENTATION_HORIZONTAL &&
	      ctk_widget_get_valign_with_baseline (child) == CTK_ALIGN_BASELINE &&
	      child_baseline != -1)
	    {
	      have_baseline = TRUE;
	      max_above = MAX (max_above, child_baseline + ipad_y);
	      max_below = MAX (max_below , child_requisition.height + ipad_h - (child_baseline + ipad_y));
	    }
          avg_w += child_requisition.width + ipad_w;
          avg_h += child_requisition.height + ipad_h;
        }
    }
  avg_w /= MAX (nchildren, 1);
  avg_h /= MAX (nchildren, 1);

  if (baseline)
    *baseline = have_baseline ? max_above : -1;
  if (baseline_height)
    *baseline_height = max_above + max_below;

  *widths = g_new (gint, nchildren);
  *heights = g_new (gint, nchildren);
  *baselines = g_new (gint, nchildren);

  i = 0;
  children = list;
  while (children)
    {
      CtkWidget *child;
      gboolean is_secondary;
      gboolean non_homogeneous;

      child = children->data;
      children = children->next;

      if (ctk_widget_get_visible (child))
        {
          is_secondary = ctk_button_box_get_child_secondary (bbox, child);
          non_homogeneous = ctk_button_box_get_child_non_homogeneous (bbox, child);

          if (is_secondary)
            nsecondaries++;

          _ctk_widget_get_preferred_size_and_baseline (child,
                                                       &child_requisition, NULL, &child_baseline, NULL);

          if (homogeneous ||
              (!non_homogeneous && (child_requisition.width + ipad_w < avg_w * 1.5)))
            {
              (*widths)[i] = -1;
              if (child_requisition.width + ipad_w > needed_width)
                needed_width = child_requisition.width + ipad_w;
            }
          else
            {
              (*widths)[i] = child_requisition.width + ipad_w;
            }

	  (*baselines)[i] = -1;

          if (homogeneous ||
              (!non_homogeneous && (child_requisition.height + ipad_h < avg_h * 1.5)))
            {
              (*heights)[i] = -1;

	      if (orientation == CTK_ORIENTATION_HORIZONTAL &&
		  ctk_widget_get_valign_with_baseline (child) == CTK_ALIGN_BASELINE &&
		  child_baseline != -1)
		{
		  (*baselines)[i] = child_baseline + ipad_y;

		  if (child_baseline + ipad_y > needed_above)
		    needed_above = child_baseline + ipad_y;
		  if (child_requisition.height - child_baseline + ipad_y > needed_below)
		    needed_below = child_requisition.height - child_baseline + ipad_y;
		}
	      else
		{
		  if (child_requisition.height + ipad_h > needed_height)
		    needed_height = child_requisition.height + ipad_h;
		}
            }
          else
            {
              (*heights)[i] = child_requisition.height + ipad_h;

	      if (orientation == CTK_ORIENTATION_HORIZONTAL &&
		  ctk_widget_get_valign_with_baseline (child) == CTK_ALIGN_BASELINE &&
		  child_baseline != -1)
		(*baselines)[i] = child_baseline + ipad_y;
            }

          i++;
        }
    }

  g_list_free (list);

  needed_height = MAX (needed_height, needed_above + needed_below);

  for (i = 0; i < nchildren; i++)
    {
      if ((*widths)[i] == -1)
        (*widths)[i] = needed_width;
      if ((*heights)[i] == -1)
	{
	  (*heights)[i] = needed_height;
	  if ((*baselines)[i] != -1)
	    (*baselines)[i] = needed_above;
	}
    }

  if (nvis_children)
    *nvis_children = nchildren;

  if (nvis_secondaries)
    *nvis_secondaries = nsecondaries;
}

static void
ctk_button_box_size_request (CtkWidget      *widget,
                             CtkRequisition *requisition,
			     gint           *baseline)
{
  CtkButtonBoxPrivate *priv;
  CtkButtonBox *bbox;
  gint nvis_children;
  gint max_size, max_above, max_below;
  gint total_size;
  gint spacing;
  CtkOrientation orientation;
  gint *widths;
  gint *heights;
  gint *baselines;
  gint i;

  if (baseline)
    *baseline = -1;

  bbox = CTK_BUTTON_BOX (widget);
  priv = bbox->priv;

  orientation = ctk_orientable_get_orientation (CTK_ORIENTABLE (widget));
  spacing = ctk_box_get_spacing (CTK_BOX (widget));

  ctk_button_box_child_requisition (widget,
                                    &nvis_children,
                                    NULL,
                                    &widths, &heights, &baselines, baseline, NULL);

  max_size = max_above = max_below = 0;
  total_size = 0;
  for (i = 0; i < nvis_children; i++)
    {
      if (orientation == CTK_ORIENTATION_HORIZONTAL)
        {
          total_size += widths[i];
	  if (baselines[i] == -1)
	    max_size = MAX (max_size, heights[i]);
	  else
	    {
	      max_above = MAX (max_above, baselines[i]);
	      max_below = MAX (max_below, heights[i] - baselines[i]);
	    }
        }
      else
        {
          total_size += heights[i];
          max_size = MAX (max_size, widths[i]);
        }
    }
  g_free (widths);
  g_free (heights);
  g_free (baselines);

  max_size = MAX (max_size, max_above + max_below);

  switch (ctk_box_get_baseline_position (CTK_BOX (widget)))
    {
    case CTK_BASELINE_POSITION_TOP:
      break;
    case CTK_BASELINE_POSITION_CENTER:
      if (baseline != NULL && *baseline != -1)
	*baseline += (max_size - (max_above + max_below)) / 2;
      break;
    case CTK_BASELINE_POSITION_BOTTOM:
      if (baseline != NULL && *baseline != -1)
	*baseline += max_size - (max_above + max_below);
      break;
    }

  if (nvis_children == 0)
    {
      requisition->width = 0;
      requisition->height = 0;
    }
  else
    {
      switch (priv->layout_style)
        {
          case CTK_BUTTONBOX_SPREAD:
            if (orientation == CTK_ORIENTATION_HORIZONTAL)
              requisition->width = total_size + ((nvis_children + 1)*spacing);
            else
              requisition->height = total_size + ((nvis_children + 1)*spacing);

            break;
          case CTK_BUTTONBOX_EDGE:
          case CTK_BUTTONBOX_START:
          case CTK_BUTTONBOX_END:
          case CTK_BUTTONBOX_CENTER:
          case CTK_BUTTONBOX_EXPAND:
            if (orientation == CTK_ORIENTATION_HORIZONTAL)
              requisition->width = total_size + ((nvis_children - 1)*spacing);
            else
              requisition->height = total_size + ((nvis_children - 1)*spacing);

            break;
          default:
            g_assert_not_reached ();
            break;
        }

      if (orientation == CTK_ORIENTATION_HORIZONTAL)
        requisition->height = max_size;
      else
        requisition->width = max_size;
    }
}

static void
ctk_button_box_measure (CtkCssGadget   *gadget,
                        CtkOrientation  orientation,
                        int             for_size,
                        int            *minimum,
                        int            *natural,
                        int            *minimum_baseline,
                        int            *natural_baseline,
                        gpointer        unused)
{
  CtkWidget *widget;
  CtkRequisition requisition;
  int baseline;
  int *pb;

  if (minimum_baseline || natural_baseline)
    pb = &baseline;
  else
    pb = NULL;

  widget = ctk_css_gadget_get_owner (gadget);
  ctk_button_box_size_request (widget, &requisition, pb);

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    *minimum = *natural = requisition.width;
  else
    *minimum = *natural = requisition.height;

  if (minimum_baseline)
    *minimum_baseline = baseline;
  if (natural_baseline)
    *natural_baseline = baseline;
}

static void
ctk_button_box_get_preferred_width (CtkWidget *widget,
                                    gint      *minimum,
                                    gint      *natural)
{
  CtkButtonBoxPrivate *priv = CTK_BUTTON_BOX (widget)->priv;
  CtkCssGadget *gadget;

  if (priv->layout_style == CTK_BUTTONBOX_EXPAND)
    gadget = ctk_box_get_gadget (CTK_BOX (widget));
  else
    gadget = priv->gadget;

  ctk_css_gadget_get_preferred_size (gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_button_box_get_preferred_height (CtkWidget *widget,
                                     gint      *minimum,
                                     gint      *natural)
{
  CtkButtonBoxPrivate *priv = CTK_BUTTON_BOX (widget)->priv;
  CtkCssGadget *gadget;

  if (priv->layout_style == CTK_BUTTONBOX_EXPAND)
    gadget = ctk_box_get_gadget (CTK_BOX (widget));
  else
    gadget = priv->gadget;

  ctk_css_gadget_get_preferred_size (gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_button_box_get_preferred_width_for_height (CtkWidget *widget,
                                               gint       height,
                                               gint      *minimum,
                                               gint      *natural)
{
  CtkButtonBoxPrivate *priv = CTK_BUTTON_BOX (widget)->priv;
  CtkCssGadget *gadget;

  if (priv->layout_style == CTK_BUTTONBOX_EXPAND)
    gadget = ctk_box_get_gadget (CTK_BOX (widget));
  else
    gadget = priv->gadget;

  ctk_css_gadget_get_preferred_size (gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     height,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_button_box_get_preferred_height_for_width (CtkWidget *widget,
                                               gint       width,
                                               gint      *minimum,
                                               gint      *natural)
{
  CtkButtonBoxPrivate *priv = CTK_BUTTON_BOX (widget)->priv;
  CtkCssGadget *gadget;

  if (priv->layout_style == CTK_BUTTONBOX_EXPAND)
    gadget = ctk_box_get_gadget (CTK_BOX (widget));
  else
    gadget = priv->gadget;

  ctk_css_gadget_get_preferred_size (gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     width,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_button_box_get_preferred_height_and_baseline_for_width (CtkWidget *widget,
							    gint       width,
							    gint      *minimum,
							    gint      *natural,
							    gint      *minimum_baseline,
							    gint      *natural_baseline)
{
  CtkButtonBoxPrivate *priv = CTK_BUTTON_BOX (widget)->priv;
  CtkCssGadget *gadget;

  if (priv->layout_style == CTK_BUTTONBOX_EXPAND)
    gadget = ctk_box_get_gadget (CTK_BOX (widget));
  else
    gadget = priv->gadget;

  ctk_css_gadget_get_preferred_size (gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     width,
                                     minimum, natural,
                                     minimum_baseline, natural_baseline);
}

static void
ctk_button_box_size_allocate (CtkWidget     *widget,
                              CtkAllocation *allocation)
{
  CtkButtonBoxPrivate *priv = CTK_BUTTON_BOX (widget)->priv;
  CtkCssGadget *gadget;
  GdkRectangle clip;

  if (priv->layout_style == CTK_BUTTONBOX_EXPAND)
    gadget = ctk_box_get_gadget (CTK_BOX (widget));
  else
    gadget = priv->gadget;

  ctk_widget_set_allocation (widget, allocation);

  ctk_css_gadget_allocate (gadget,
                           allocation,
                           ctk_widget_get_allocated_baseline (widget),
                           &clip);

  ctk_widget_set_clip (widget, &clip);
}

static void
ctk_button_box_allocate (CtkCssGadget        *gadget,
                         const CtkAllocation *allocation,
                         int                  baseline,
                         CtkAllocation       *out_clip,
                         gpointer             unused)
{
  CtkWidget *widget;
  CtkButtonBoxPrivate *priv;
  CtkButtonBox *bbox;
  GList *children, *list;
  CtkAllocation child_allocation;
  gint nvis_children;
  gint n_primaries;
  gint n_secondaries;
  gint x = 0;
  gint y = 0;
  gint secondary_x = 0;
  gint secondary_y = 0;
  gint width = 0;
  gint height = 0;
  gint childspacing = 0;
  gint spacing;
  CtkOrientation orientation;
  gint ipad_x, ipad_y;
  gint *widths;
  gint *heights;
  gint *baselines;
  gint *sizes;
  gint primary_size;
  gint secondary_size;
  gint total_size;
  gint baseline_height;
  gint child_baseline;
  gint i;

  widget = ctk_css_gadget_get_owner (gadget);
  bbox = CTK_BUTTON_BOX (widget);
  priv = bbox->priv;

  orientation = ctk_orientable_get_orientation (CTK_ORIENTABLE (widget));
  spacing = ctk_box_get_spacing (CTK_BOX (widget));

  ctk_widget_style_get (widget,
                        "child-internal-pad-x", &ipad_x,
                        "child-internal-pad-y", &ipad_y,
                        NULL);
  ctk_button_box_child_requisition (widget,
                                    &nvis_children,
                                    &n_secondaries,
                                    &widths, &heights, &baselines, &baseline, &baseline_height);

  if (baseline != -1)
    {
      /* TODO: modify baseline based on baseline_pos && allocated_baseline*/
      switch (ctk_box_get_baseline_position (CTK_BOX (widget)))
	{
	case CTK_BASELINE_POSITION_TOP:
          /* keep baseline as is */
	  break;
	case CTK_BASELINE_POSITION_CENTER:
	  baseline = baseline + (allocation->height - baseline_height) / 2;
	  break;
	case CTK_BASELINE_POSITION_BOTTOM:
	  baseline = allocation->height - (baseline_height - baseline);
	  break;
	}
    }

  n_primaries = nvis_children - n_secondaries;
  primary_size = 0;
  secondary_size = 0;
  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    sizes = widths;
  else
    sizes = heights;

  i = 0;
  list = children = _ctk_box_get_children (CTK_BOX (widget));
  while (children)
    {
      CtkWidget *child;

      child = children->data;
      children = children->next;

      if (ctk_widget_get_visible (child))
        {
          if (ctk_button_box_get_child_secondary (bbox, child))
            secondary_size += sizes[i];
          else
            primary_size += sizes[i];
          i++;
        }
    }
  total_size = primary_size + secondary_size;

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    width = allocation->width;
  else
    height = allocation->height;

  switch (priv->layout_style)
    {
      case CTK_BUTTONBOX_SPREAD:

        if (orientation == CTK_ORIENTATION_HORIZONTAL)
          {
            childspacing = (width - total_size) / (nvis_children + 1);
            x = allocation->x + childspacing;
            secondary_x = x + primary_size + n_primaries * childspacing;
          }
        else
          {
            childspacing = (height - total_size) / (nvis_children + 1);
            y = allocation->y + childspacing;
            secondary_y = y + primary_size + n_primaries * childspacing;
          }

        break;

      case CTK_BUTTONBOX_EDGE:

        if (orientation == CTK_ORIENTATION_HORIZONTAL)
          {
            if (nvis_children >= 2)
              {
                childspacing = (width - total_size) / (nvis_children - 1);
                x = allocation->x;
                secondary_x = x + primary_size + n_primaries * childspacing;
              }
            else if (nvis_children == 1)
              {
                /* one child, just center */
                childspacing = width;
                x = secondary_x = allocation->x
                                  + (allocation->width - widths[0]) / 2;
              }
            else
              {
                /* zero children, meh */
                childspacing = width;
                x = secondary_x = allocation->x + allocation->width / 2;
              }
          }
        else
          {
            if (nvis_children >= 2)
              {
                childspacing = (height - total_size) / (nvis_children - 1);
                y = allocation->y;
                secondary_y = y + primary_size + n_primaries * childspacing;
              }
            else if (nvis_children == 1)
              {
                /* one child, just center */
                childspacing = height;
                y = secondary_y = allocation->y
                                     + (allocation->height - heights[0]) / 2;
              }
            else
              {
                /* zero children, meh */
                childspacing = height;
                y = secondary_y = allocation->y + allocation->height / 2;
              }
          }

        break;

      case CTK_BUTTONBOX_START:

        if (orientation == CTK_ORIENTATION_HORIZONTAL)
          {
            childspacing = spacing;
            x = allocation->x;
            secondary_x = allocation->x + allocation->width
              - secondary_size - spacing * (n_secondaries - 1);
          }
        else
          {
            childspacing = spacing;
            y = allocation->y;
            secondary_y = allocation->y + allocation->height
              - secondary_size - spacing * (n_secondaries - 1);
          }

        break;

      case CTK_BUTTONBOX_END:

        if (orientation == CTK_ORIENTATION_HORIZONTAL)
          {
            childspacing = spacing;
            x = allocation->x + allocation->width
              - primary_size - spacing * (n_primaries - 1);
            secondary_x = allocation->x;
          }
        else
          {
            childspacing = spacing;
            y = allocation->y + allocation->height
              - primary_size - spacing * (n_primaries - 1);
            secondary_y = allocation->y;
          }

        break;

      case CTK_BUTTONBOX_CENTER:

        if (orientation == CTK_ORIENTATION_HORIZONTAL)
          {
            childspacing = spacing;
            x = allocation->x +
              (allocation->width
               - (primary_size + spacing * (n_primaries - 1))) / 2
              + (secondary_size + n_secondaries * spacing) / 2;
            secondary_x = allocation->x;
          }
        else
          {
            childspacing = spacing;
            y = allocation->y +
              (allocation->height
               - (primary_size + spacing * (n_primaries - 1))) / 2
              + (secondary_size + n_secondaries * spacing) / 2;
            secondary_y = allocation->y;
          }

        break;

      default:
        g_assert_not_reached ();
        break;
    }

  children = list;
  i = 0;
  while (children)
    {
      CtkWidget *child;

      child = children->data;
      children = children->next;

      if (ctk_widget_get_visible (child))
        {
          child_allocation.width = widths[i];
          child_allocation.height = heights[i];
	  child_baseline = -1;

          if (orientation == CTK_ORIENTATION_HORIZONTAL)
            {
	      if (baselines[i] != -1)
		{
		  child_allocation.y = allocation->y + baseline - baselines[i];
		  child_baseline = baselines[i];
		}
	      else
		child_allocation.y = allocation->y + (allocation->height - child_allocation.height) / 2;

              if (ctk_button_box_get_child_secondary (bbox, child))
                {
                  child_allocation.x = secondary_x;
                  secondary_x += child_allocation.width + childspacing;
                }
              else
                {
                  child_allocation.x = x;
                  x += child_allocation.width + childspacing;
                }

              if (ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL)
                  child_allocation.x = (allocation->x + allocation->width)
                          - (child_allocation.x + child_allocation.width - allocation->x);
            }
          else
            {
              child_allocation.x = allocation->x + (allocation->width - child_allocation.width) / 2;

              if (ctk_button_box_get_child_secondary (bbox, child))
                {
                  child_allocation.y = secondary_y;
                  secondary_y += child_allocation.height + childspacing;
                }
              else
                {
                  child_allocation.y = y;
                  y += child_allocation.height + childspacing;
                }
            }

          ctk_widget_size_allocate_with_baseline (child, &child_allocation, child_baseline);
          i++;
        }
    }

  g_list_free (list);
  g_free (widths);
  g_free (heights);
  g_free (baselines);

  ctk_container_get_children_clip (CTK_CONTAINER (widget), out_clip);
}

/**
 * ctk_button_box_new:
 * @orientation: the box's orientation.
 *
 * Creates a new #CtkButtonBox.
 *
 * Returns: a new #CtkButtonBox.
 *
 * Since: 3.0
 */
CtkWidget *
ctk_button_box_new (CtkOrientation orientation)
{
  return g_object_new (CTK_TYPE_BUTTON_BOX,
                       "orientation", orientation,
                       NULL);
}

/**
 * ctk_button_box_get_child_non_homogeneous:
 * @widget: a #CtkButtonBox
 * @child: a child of @widget
 *
 * Returns whether the child is exempted from homogenous
 * sizing.
 *
 * Returns: %TRUE if the child is not subject to homogenous sizing
 *
 * Since: 3.2
 */
gboolean
ctk_button_box_get_child_non_homogeneous (CtkButtonBox *widget,
                                          CtkWidget    *child)
{
  g_return_val_if_fail (CTK_IS_BUTTON_BOX (widget), FALSE);
  g_return_val_if_fail (CTK_IS_WIDGET (child), FALSE);

  return (g_object_get_data (G_OBJECT (child), CTK_BOX_NON_HOMOGENEOUS) != NULL);
}

/**
 * ctk_button_box_set_child_non_homogeneous:
 * @widget: a #CtkButtonBox
 * @child: a child of @widget
 * @non_homogeneous: the new value
 *
 * Sets whether the child is exempted from homogeous sizing.
 *
 * Since: 3.2
 */
void
ctk_button_box_set_child_non_homogeneous (CtkButtonBox *widget,
                                          CtkWidget    *child,
                                          gboolean      non_homogeneous)
{
  g_return_if_fail (CTK_IS_BUTTON_BOX (widget));
  g_return_if_fail (CTK_IS_WIDGET (child));
  g_return_if_fail (ctk_widget_get_parent (child) == CTK_WIDGET (widget));

  g_object_set_data (G_OBJECT (child),
                     CTK_BOX_NON_HOMOGENEOUS,
                     non_homogeneous ? GINT_TO_POINTER (1) : NULL);
  ctk_widget_child_notify (child, "non-homogeneous");

  if (ctk_widget_get_visible (CTK_WIDGET (widget)) &&
      ctk_widget_get_visible (child))
    ctk_widget_queue_resize (child);
}
