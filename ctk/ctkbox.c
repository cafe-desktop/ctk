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
 * SECTION:ctkbox
 * @Short_description: A container for packing widgets in a single row or column
 * @Title: CtkBox
 * @See_also: #CtkGrid
 *
 * The CtkBox widget arranges child widgets into a single row or column,
 * depending upon the value of its #CtkOrientable:orientation property. Within
 * the other dimension, all children are allocated the same size. Of course,
 * the #CtkWidget:halign and #CtkWidget:valign properties can be used on
 * the children to influence their allocation.
 *
 * CtkBox uses a notion of packing. Packing refers
 * to adding widgets with reference to a particular position in a
 * #CtkContainer. For a CtkBox, there are two reference positions: the
 * start and the end of the box.
 * For a vertical #CtkBox, the start is defined as the top of the box and
 * the end is defined as the bottom. For a horizontal #CtkBox the start
 * is defined as the left side and the end is defined as the right side.
 *
 * Use repeated calls to ctk_box_pack_start() to pack widgets into a
 * CtkBox from start to end. Use ctk_box_pack_end() to add widgets from
 * end to start. You may intersperse these calls and add widgets from
 * both ends of the same CtkBox.
 *
 * Because CtkBox is a #CtkContainer, you may also use ctk_container_add()
 * to insert widgets into the box, and they will be packed with the default
 * values for expand and fill child properties. Use ctk_container_remove()
 * to remove widgets from the CtkBox.
 *
 * Use ctk_box_set_homogeneous() to specify whether or not all children
 * of the CtkBox are forced to get the same amount of space.
 *
 * Use ctk_box_set_spacing() to determine how much space will be
 * minimally placed between all children in the CtkBox. Note that
 * spacing is added between the children, while
 * padding added by ctk_box_pack_start() or ctk_box_pack_end() is added
 * on either side of the widget it belongs to.
 *
 * Use ctk_box_reorder_child() to move a CtkBox child to a different
 * place in the box.
 *
 * Use ctk_box_set_child_packing() to reset the expand,
 * fill and padding child properties.
 * Use ctk_box_query_child_packing() to query these fields.
 *
 * # CSS nodes
 *
 * CtkBox uses a single CSS node with name box.
 *
 * In horizontal orientation, the nodes of the children are always arranged
 * from left to right. So :first-child will always select the leftmost child,
 * regardless of text direction.
 */

#include "config.h"

#include "ctkbox.h"
#include "ctkboxprivate.h"
#include "ctkcontainerprivate.h"
#include "ctkcsscustomgadgetprivate.h"
#include "ctkcssnodeprivate.h"
#include "ctkintl.h"
#include "ctkorientable.h"
#include "ctkorientableprivate.h"
#include "ctkprivate.h"
#include "ctktypebuiltins.h"
#include "ctksizerequest.h"
#include "ctkwidgetpath.h"
#include "ctkwidgetprivate.h"
#include "a11y/ctkcontaineraccessible.h"


enum {
  PROP_0,
  PROP_SPACING,
  PROP_HOMOGENEOUS,
  PROP_BASELINE_POSITION,

  /* orientable */
  PROP_ORIENTATION,
  LAST_PROP = PROP_ORIENTATION
};

enum {
  CHILD_PROP_0,
  CHILD_PROP_EXPAND,
  CHILD_PROP_FILL,
  CHILD_PROP_PADDING,
  CHILD_PROP_PACK_TYPE,
  CHILD_PROP_POSITION,
  LAST_CHILD_PROP
};

typedef struct _CtkBoxChild        CtkBoxChild;

struct _CtkBoxPrivate
{
  GList          *children;
  CtkBoxChild    *center;
  CtkCssGadget   *gadget;

  CtkOrientation  orientation;
  gint16          spacing;

  guint           default_expand : 1;
  guint           homogeneous    : 1;
  guint           spacing_set    : 1;
  guint           baseline_pos   : 2;
};

static GParamSpec *props[LAST_PROP] = { NULL, };
static GParamSpec *child_props[LAST_CHILD_PROP] = { NULL, };

/*
 * CtkBoxChild:
 * @widget: the child widget, packed into the CtkBox.
 * @padding: the number of extra pixels to put between this child and its
 *  neighbors, set when packed, zero by default.
 * @expand: flag indicates whether extra space should be given to this child.
 *  Any extra space given to the parent CtkBox is divided up among all children
 *  with this attribute set to %TRUE; set when packed, %TRUE by default.
 * @fill: flag indicates whether any extra space given to this child due to its
 *  @expand attribute being set is actually allocated to the child, rather than
 *  being used as padding around the widget; set when packed, %TRUE by default.
 * @pack: one of #CtkPackType indicating whether the child is packed with
 *  reference to the start (top/left) or end (bottom/right) of the CtkBox.
 */
struct _CtkBoxChild
{
  CtkWidget *widget;

  guint16    padding;

  guint      expand : 1;
  guint      fill   : 1;
  guint      pack   : 1;
};

static void ctk_box_size_allocate         (CtkWidget              *widget,
                                           CtkAllocation          *allocation);
static gboolean ctk_box_draw           (CtkWidget        *widget,
                                        cairo_t          *cr);

static void ctk_box_direction_changed  (CtkWidget        *widget,
                                        CtkTextDirection  previous_direction);

static void ctk_box_set_property       (GObject        *object,
                                        guint           prop_id,
                                        const GValue   *value,
                                        GParamSpec     *pspec);
static void ctk_box_get_property       (GObject        *object,
                                        guint           prop_id,
                                        GValue         *value,
                                        GParamSpec     *pspec);
static void ctk_box_add                (CtkContainer   *container,
                                        CtkWidget      *widget);
static void ctk_box_remove             (CtkContainer   *container,
                                        CtkWidget      *widget);
static void ctk_box_forall             (CtkContainer   *container,
                                        gboolean        include_internals,
                                        CtkCallback     callback,
                                        gpointer        callback_data);
static void ctk_box_set_child_property (CtkContainer   *container,
                                        CtkWidget      *child,
                                        guint           property_id,
                                        const GValue   *value,
                                        GParamSpec     *pspec);
static void ctk_box_get_child_property (CtkContainer   *container,
                                        CtkWidget      *child,
                                        guint           property_id,
                                        GValue         *value,
                                        GParamSpec     *pspec);
static GType ctk_box_child_type        (CtkContainer   *container);
static CtkWidgetPath * ctk_box_get_path_for_child
                                       (CtkContainer   *container,
                                        CtkWidget      *child);


static void               ctk_box_get_preferred_width            (CtkWidget           *widget,
                                                                  gint                *minimum_size,
                                                                  gint                *natural_size);
static void               ctk_box_get_preferred_height           (CtkWidget           *widget,
                                                                  gint                *minimum_size,
                                                                  gint                *natural_size);
static void               ctk_box_get_preferred_width_for_height (CtkWidget           *widget,
                                                                  gint                 height,
                                                                  gint                *minimum_width,
                                                                  gint                *natural_width);
static void               ctk_box_get_preferred_height_for_width (CtkWidget           *widget,
                                                                  gint                 width,
                                                                  gint                *minimum_height,
                                                                  gint                *natural_height);
static void  ctk_box_get_preferred_height_and_baseline_for_width (CtkWidget           *widget,
								  gint                 width,
								  gint                *minimum_height,
								  gint                *natural_height,
								  gint                *minimum_baseline,
								  gint                *natural_baseline);

static void               ctk_box_buildable_init                 (CtkBuildableIface  *iface);

G_DEFINE_TYPE_WITH_CODE (CtkBox, ctk_box, CTK_TYPE_CONTAINER,
                         G_ADD_PRIVATE (CtkBox)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_ORIENTABLE, NULL)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_BUILDABLE, ctk_box_buildable_init))

static void
ctk_box_dispose (GObject *object)
{
  CtkBox *box = CTK_BOX (object);
  CtkBoxPrivate *priv = box->priv;

  g_clear_object (&priv->gadget);

  G_OBJECT_CLASS (ctk_box_parent_class)->dispose (object);
}

static void
ctk_box_class_init (CtkBoxClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (class);
  CtkContainerClass *container_class = CTK_CONTAINER_CLASS (class);

  object_class->set_property = ctk_box_set_property;
  object_class->get_property = ctk_box_get_property;
  object_class->dispose = ctk_box_dispose;

  widget_class->draw                           = ctk_box_draw;
  widget_class->size_allocate                  = ctk_box_size_allocate;
  widget_class->get_preferred_width            = ctk_box_get_preferred_width;
  widget_class->get_preferred_height           = ctk_box_get_preferred_height;
  widget_class->get_preferred_height_for_width = ctk_box_get_preferred_height_for_width;
  widget_class->get_preferred_height_and_baseline_for_width = ctk_box_get_preferred_height_and_baseline_for_width;
  widget_class->get_preferred_width_for_height = ctk_box_get_preferred_width_for_height;
  widget_class->direction_changed              = ctk_box_direction_changed;

  container_class->add = ctk_box_add;
  container_class->remove = ctk_box_remove;
  container_class->forall = ctk_box_forall;
  container_class->child_type = ctk_box_child_type;
  container_class->set_child_property = ctk_box_set_child_property;
  container_class->get_child_property = ctk_box_get_child_property;
  container_class->get_path_for_child = ctk_box_get_path_for_child;
  ctk_container_class_handle_border_width (container_class);

  g_object_class_override_property (object_class,
                                    PROP_ORIENTATION,
                                    "orientation");

  props[PROP_SPACING] =
    g_param_spec_int ("spacing",
                      P_("Spacing"),
                      P_("The amount of space between children"),
                      0, G_MAXINT, 0,
                      CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_HOMOGENEOUS] =
    g_param_spec_boolean ("homogeneous",
                          P_("Homogeneous"),
                          P_("Whether the children should all be the same size"),
                          FALSE,
                          CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_BASELINE_POSITION] =
    g_param_spec_enum ("baseline-position",
                       P_("Baseline position"),
                       P_("The position of the baseline aligned widgets if extra space is available"),
                       CTK_TYPE_BASELINE_POSITION,
                       CTK_BASELINE_POSITION_CENTER,
                       CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  /**
   * CtkBox:expand:
   *
   * Whether the child should receive extra space when the parent grows.
   *
   * Note that the default value for this property is %FALSE for CtkBox,
   * but #CtkHBox, #CtkVBox and other subclasses use the old default
   * of %TRUE.
   *
   * Note: The #CtkWidget:hexpand or #CtkWidget:vexpand properties are the
   * preferred way to influence whether the child receives extra space, by
   * setting the child’s expand property corresponding to the box’s orientation.
   *
   * In contrast to #CtkWidget:hexpand, the expand child property does
   * not cause the box to expand itself.
   */
  child_props[CHILD_PROP_EXPAND] =
      g_param_spec_boolean ("expand",
                            P_("Expand"),
                            P_("Whether the child should receive extra space when the parent grows"),
                            FALSE,
                            CTK_PARAM_READWRITE);

  /**
   * CtkBox:fill:
   *
   * Whether the child should fill extra space or use it as padding.
   *
   * Note: The #CtkWidget:halign or #CtkWidget:valign properties are the
   * preferred way to influence whether the child fills available space, by
   * setting the child’s align property corresponding to the box’s orientation
   * to %CTK_ALIGN_FILL to fill, or to something else to refrain from filling.
   */
  child_props[CHILD_PROP_FILL] =
      g_param_spec_boolean ("fill",
                            P_("Fill"),
                            P_("Whether extra space given to the child should be allocated to the child or used as padding"),
                            TRUE,
                            CTK_PARAM_READWRITE);

  /**
   * CtkBox:padding:
   *
   * Extra space to put between the child and its neighbors, in pixels.
   *
   * Note: The CSS padding properties are the preferred way to add space among
   * widgets, by setting the paddings corresponding to the box’s orientation.
   */
  child_props[CHILD_PROP_PADDING] =
      g_param_spec_uint ("padding",
                         P_("Padding"),
                         P_("Extra space to put between the child and its neighbors, in pixels"),
                         0, G_MAXINT,
                         0,
                         CTK_PARAM_READWRITE);

  child_props[CHILD_PROP_PACK_TYPE] =
      g_param_spec_enum ("pack-type",
                         P_("Pack type"),
                         P_("A CtkPackType indicating whether the child is packed with reference to the start or end of the parent"),
                         CTK_TYPE_PACK_TYPE, CTK_PACK_START,
                         CTK_PARAM_READWRITE);

  child_props[CHILD_PROP_POSITION] =
      g_param_spec_int ("position",
                        P_("Position"),
                        P_("The index of the child in the parent"),
                        -1, G_MAXINT,
                        0,
                        CTK_PARAM_READWRITE);

  ctk_container_class_install_child_properties (container_class, LAST_CHILD_PROP, child_props);

  ctk_widget_class_set_accessible_role (widget_class, ATK_ROLE_FILLER);
  ctk_widget_class_set_css_name (widget_class, "box");
}

static void
ctk_box_set_property (GObject      *object,
                      guint         prop_id,
                      const GValue *value,
                      GParamSpec   *pspec)
{
  CtkBox *box = CTK_BOX (object);
  CtkBoxPrivate *private = box->priv;

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      {
        CtkOrientation orientation = g_value_get_enum (value);
        if (private->orientation != orientation)
          {
            private->orientation = orientation;
            _ctk_orientable_set_style_classes (CTK_ORIENTABLE (box));
            ctk_widget_queue_resize (CTK_WIDGET (box));
            g_object_notify (object, "orientation");
          }
      }
      break;
    case PROP_SPACING:
      ctk_box_set_spacing (box, g_value_get_int (value));
      break;
    case PROP_BASELINE_POSITION:
      ctk_box_set_baseline_position (box, g_value_get_enum (value));
      break;
    case PROP_HOMOGENEOUS:
      ctk_box_set_homogeneous (box, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_box_get_property (GObject    *object,
                      guint       prop_id,
                      GValue     *value,
                      GParamSpec *pspec)
{
  CtkBox *box = CTK_BOX (object);
  CtkBoxPrivate *private = box->priv;

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, private->orientation);
      break;
    case PROP_SPACING:
      g_value_set_int (value, private->spacing);
      break;
    case PROP_BASELINE_POSITION:
      g_value_set_enum (value, private->baseline_pos);
      break;
    case PROP_HOMOGENEOUS:
      g_value_set_boolean (value, private->homogeneous);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static gboolean
ctk_box_draw_contents (CtkCssGadget *gadget,
                       cairo_t      *cr,
                       int           x G_GNUC_UNUSED,
                       int           y G_GNUC_UNUSED,
                       int           width G_GNUC_UNUSED,
                       int           height G_GNUC_UNUSED,
                       gpointer      unused G_GNUC_UNUSED)
{
  CTK_WIDGET_CLASS (ctk_box_parent_class)->draw (ctk_css_gadget_get_owner (gadget), cr);

  return FALSE;
}

static gboolean
ctk_box_draw (CtkWidget *widget,
              cairo_t   *cr)
{
  ctk_css_gadget_draw (CTK_BOX (widget)->priv->gadget, cr);

  return FALSE;
}


static void
count_expand_children (CtkBox *box,
                       gint *visible_children,
                       gint *expand_children)
{
  CtkBoxPrivate  *private = box->priv;
  GList       *children;
  CtkBoxChild *child;

  *visible_children = *expand_children = 0;

  for (children = private->children; children; children = children->next)
    {
      child = children->data;

      if (_ctk_widget_get_visible (child->widget))
	{
	  *visible_children += 1;
	  if (child->expand || ctk_widget_compute_expand (child->widget, private->orientation))
	    *expand_children += 1;
	}
    }
}

static void
ctk_box_size_allocate_no_center (CtkWidget           *widget,
                                 const CtkAllocation *allocation)
{
  CtkBox *box = CTK_BOX (widget);
  CtkBoxPrivate *private = box->priv;
  CtkBoxChild *child;
  GList *children;
  gint nvis_children;
  gint nexpand_children;

  CtkTextDirection direction;
  CtkAllocation child_allocation;
  CtkRequestedSize *sizes;
  gint child_minimum_baseline, child_natural_baseline;
  gint minimum_above, natural_above;
  gint minimum_below, natural_below;
  gboolean have_baseline;
  gint baseline;

  CtkPackType packing;

  gint size;
  gint extra;
  gint n_extra_widgets = 0; /* Number of widgets that receive 1 extra px */
  gint x = 0, y = 0, i;
  gint child_size;


  count_expand_children (box, &nvis_children, &nexpand_children);

  /* If there is no visible child, simply return. */
  if (nvis_children <= 0)
    return;

  direction = ctk_widget_get_direction (widget);
  sizes = g_newa (CtkRequestedSize, nvis_children);
  memset (sizes, 0, nvis_children * sizeof (CtkRequestedSize));

  if (private->orientation == CTK_ORIENTATION_HORIZONTAL)
    size = allocation->width - (nvis_children - 1) * private->spacing;
  else
    size = allocation->height - (nvis_children - 1) * private->spacing;

  have_baseline = FALSE;
  minimum_above = natural_above = 0;
  minimum_below = natural_below = 0;

  /* Retrieve desired size for visible children. */
  for (i = 0, children = private->children; children; children = children->next)
    {
      child = children->data;

      if (!_ctk_widget_get_visible (child->widget))
	continue;

      if (private->orientation == CTK_ORIENTATION_HORIZONTAL)
	ctk_widget_get_preferred_width_for_height (child->widget,
                                                   allocation->height,
                                                   &sizes[i].minimum_size,
                                                   &sizes[i].natural_size);
      else
	ctk_widget_get_preferred_height_and_baseline_for_width (child->widget,
								allocation->width,
								&sizes[i].minimum_size,
								&sizes[i].natural_size,
								NULL, NULL);

      /* Assert the api is working properly */
      if (sizes[i].minimum_size < 0)
	g_error ("CtkBox child %s minimum %s: %d < 0 for %s %d",
		 ctk_widget_get_name (CTK_WIDGET (child->widget)),
		 (private->orientation == CTK_ORIENTATION_HORIZONTAL) ? "width" : "height",
		 sizes[i].minimum_size,
		 (private->orientation == CTK_ORIENTATION_HORIZONTAL) ? "height" : "width",
		 (private->orientation == CTK_ORIENTATION_HORIZONTAL) ? allocation->height : allocation->width);

      if (sizes[i].natural_size < sizes[i].minimum_size)
	g_error ("CtkBox child %s natural %s: %d < minimum %d for %s %d",
		 ctk_widget_get_name (CTK_WIDGET (child->widget)),
		 (private->orientation == CTK_ORIENTATION_HORIZONTAL) ? "width" : "height",
		 sizes[i].natural_size,
		 sizes[i].minimum_size,
		 (private->orientation == CTK_ORIENTATION_HORIZONTAL) ? "height" : "width",
		 (private->orientation == CTK_ORIENTATION_HORIZONTAL) ? allocation->height : allocation->width);

      size -= sizes[i].minimum_size;
      size -= child->padding * 2;

      sizes[i].data = child;

      i++;
    }

  if (private->homogeneous)
    {
      /* If were homogenous we still need to run the above loop to get the
       * minimum sizes for children that are not going to fill
       */
      if (private->orientation == CTK_ORIENTATION_HORIZONTAL)
	size = allocation->width - (nvis_children - 1) * private->spacing;
      else
	size = allocation->height - (nvis_children - 1) * private->spacing;

      extra = size / nvis_children;
      n_extra_widgets = size % nvis_children;
    }
  else
    {
      /* Bring children up to size first */
      size = ctk_distribute_natural_allocation (MAX (0, size), nvis_children, sizes);

      /* Calculate space which hasn't distributed yet,
       * and is available for expanding children.
       */
      if (nexpand_children > 0)
	{
	  extra = size / nexpand_children;
	  n_extra_widgets = size % nexpand_children;
	}
      else
	extra = 0;
    }

  /* Allocate child sizes. */
  for (packing = CTK_PACK_START; packing <= CTK_PACK_END; ++packing)
    {
      for (i = 0, children = private->children;
	   children;
	   children = children->next)
	{
	  child = children->data;

	  /* If widget is not visible, skip it. */
	  if (!_ctk_widget_get_visible (child->widget))
	    continue;

	  /* If widget is packed differently skip it, but still increment i,
	   * since widget is visible and will be handled in next loop iteration.
	   */
	  if (child->pack != packing)
	    {
	      i++;
	      continue;
	    }

	  /* Assign the child's size. */
	  if (private->homogeneous)
	    {
	      child_size = extra;

	      if (n_extra_widgets > 0)
		{
		  child_size++;
		  n_extra_widgets--;
		}
	    }
	  else
	    {
	      child_size = sizes[i].minimum_size + child->padding * 2;

	      if (child->expand || ctk_widget_compute_expand (child->widget, private->orientation))
		{
		  child_size += extra;

		  if (n_extra_widgets > 0)
		    {
		      child_size++;
		      n_extra_widgets--;
		    }
		}
	    }

	  sizes[i].natural_size = child_size;

	  if (private->orientation == CTK_ORIENTATION_HORIZONTAL &&
	      ctk_widget_get_valign_with_baseline (child->widget) == CTK_ALIGN_BASELINE)
	    {
	      int child_allocation_width;
	      int child_minimum_height, child_natural_height;

	      if (child->fill)
		child_allocation_width = MAX (1, child_size - child->padding * 2);
	      else
		child_allocation_width = sizes[i].minimum_size;

	      child_minimum_baseline = -1;
	      child_natural_baseline = -1;
	      ctk_widget_get_preferred_height_and_baseline_for_width (child->widget,
								      child_allocation_width,
								      &child_minimum_height, &child_natural_height,
								      &child_minimum_baseline, &child_natural_baseline);

	      if (child_minimum_baseline >= 0)
		{
		  have_baseline = TRUE;
		  minimum_below = MAX (minimum_below, child_minimum_height - child_minimum_baseline);
		  natural_below = MAX (natural_below, child_natural_height - child_natural_baseline);
		  minimum_above = MAX (minimum_above, child_minimum_baseline);
		  natural_above = MAX (natural_above, child_natural_baseline);
		}
	    }

	  i++;
	}
    }

  baseline = ctk_widget_get_allocated_baseline (widget);
  if (baseline == -1 && have_baseline)
    {
      gint height = MAX (1, allocation->height);

      /* TODO: This is purely based on the minimum baseline, when things fit we should
	 use the natural one? */

      switch (private->baseline_pos)
	{
	case CTK_BASELINE_POSITION_TOP:
	  baseline = minimum_above;
	  break;
	case CTK_BASELINE_POSITION_CENTER:
	  baseline = minimum_above + (height - (minimum_above + minimum_below)) / 2;
	  break;
	case CTK_BASELINE_POSITION_BOTTOM:
	  baseline = height - minimum_below;
	  break;
	}
    }

  /* Allocate child positions. */
  for (packing = CTK_PACK_START; packing <= CTK_PACK_END; ++packing)
    {
      if (private->orientation == CTK_ORIENTATION_HORIZONTAL)
	{
	  child_allocation.y = allocation->y;
	  child_allocation.height = MAX (1, allocation->height);
	  if (packing == CTK_PACK_START)
	    x = allocation->x;
	  else
	    x = allocation->x + allocation->width;
	}
      else
	{
	  child_allocation.x = allocation->x;
	  child_allocation.width = MAX (1, allocation->width);
	  if (packing == CTK_PACK_START)
	    y = allocation->y;
	  else
	    y = allocation->y + allocation->height;
	}

      for (i = 0, children = private->children;
	   children;
	   children = children->next)
	{
	  child = children->data;

	  /* If widget is not visible, skip it. */
	  if (!_ctk_widget_get_visible (child->widget))
	    continue;

	  /* If widget is packed differently skip it, but still increment i,
	   * since widget is visible and will be handled in next loop iteration.
	   */
	  if (child->pack != packing)
	    {
	      i++;
	      continue;
	    }

	  child_size = sizes[i].natural_size;

	  /* Assign the child's position. */
	  if (private->orientation == CTK_ORIENTATION_HORIZONTAL)
	    {
	      if (child->fill)
		{
		  child_allocation.width = MAX (1, child_size - child->padding * 2);
		  child_allocation.x = x + child->padding;
		}
	      else
		{
		  child_allocation.width = sizes[i].minimum_size;
		  child_allocation.x = x + (child_size - child_allocation.width) / 2;
		}

	      if (packing == CTK_PACK_START)
		{
		  x += child_size + private->spacing;
		}
	      else
		{
		  x -= child_size + private->spacing;

		  child_allocation.x -= child_size;
		}

	      if (direction == CTK_TEXT_DIR_RTL)
		child_allocation.x = allocation->x + allocation->width - (child_allocation.x - allocation->x) - child_allocation.width;

	    }
	  else /* (private->orientation == CTK_ORIENTATION_VERTICAL) */
	    {
	      if (child->fill)
		{
		  child_allocation.height = MAX (1, child_size - child->padding * 2);
		  child_allocation.y = y + child->padding;
		}
	      else
		{
		  child_allocation.height = sizes[i].minimum_size;
		  child_allocation.y = y + (child_size - child_allocation.height) / 2;
		}

	      if (packing == CTK_PACK_START)
		{
		  y += child_size + private->spacing;
		}
	      else
		{
		  y -= child_size + private->spacing;

		  child_allocation.y -= child_size;
		}
	    }
	  ctk_widget_size_allocate_with_baseline (child->widget, &child_allocation, baseline);

	  i++;
	}
    }

  _ctk_widget_set_simple_clip (widget, NULL);
}

static void
ctk_box_size_allocate_with_center (CtkWidget           *widget,
                                   const CtkAllocation *allocation)
{
  CtkBox *box = CTK_BOX (widget);
  CtkBoxPrivate *priv = box->priv;
  CtkBoxChild *child;
  GList *children;
  gint nvis[2];
  gint nexp[2];
  CtkTextDirection direction;
  CtkAllocation child_allocation;
  CtkRequestedSize *sizes[2];
  CtkRequestedSize center_req = {.minimum_size = 0, .natural_size = 0};
  gint child_minimum_baseline, child_natural_baseline;
  gint minimum_above, natural_above;
  gint minimum_below, natural_below;
  gboolean have_baseline;
  gint baseline;
  gint idx[2];
  gint center_pos;
  gint center_size;
  gint box_size;
  gint side[2];
  CtkPackType packing;
  gint min_size[2];
  gint nat_size[2];
  gint extra[2];
  gint n_extra_widgets[2];
  gint x = 0, y = 0, i;
  gint child_size;

  nvis[0] = nvis[1] = 0;
  nexp[0] = nexp[1] = 0;
  for (children = priv->children; children; children = children->next)
    {
      child = children->data;

      if (child != priv->center &&
          _ctk_widget_get_visible (child->widget))
        {
          nvis[child->pack] += 1;
          if (child->expand || ctk_widget_compute_expand (child->widget, priv->orientation))
            nexp[child->pack] += 1;
        }
    }

  direction = ctk_widget_get_direction (widget);
  sizes[0] = g_newa (CtkRequestedSize, nvis[0]);
  sizes[1] = g_newa (CtkRequestedSize, nvis[1]);

  if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
    box_size = allocation->width;
  else
    box_size = allocation->height;

  have_baseline = FALSE;
  minimum_above = natural_above = 0;
  minimum_below = natural_below = 0;

  min_size[0] = nat_size[0] = nvis[0] * priv->spacing;
  min_size[1] = nat_size[1] = nvis[1] * priv->spacing;

  /* Retrieve desired size for visible children. */
  idx[0] = idx[1] = 0;
  for (children = priv->children; children; children = children->next)
    {
      CtkRequestedSize *req;

      child = children->data;

      if (!_ctk_widget_get_visible (child->widget))
	continue;

      if (child == priv->center)
        req = &center_req;
      else
        req = &(sizes[child->pack][idx[child->pack]]);

      if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
        ctk_widget_get_preferred_width_for_height (child->widget,
                                                   allocation->height,
                                                   &req->minimum_size,
                                                   &req->natural_size);
      else
        ctk_widget_get_preferred_height_and_baseline_for_width (child->widget,
                                                                allocation->width,
                                                                &req->minimum_size,
                                                                &req->natural_size,
                                                                NULL, NULL);

      if (child != priv->center)
        {
          min_size[child->pack] += req->minimum_size + 2 * child->padding;
          nat_size[child->pack] += req->natural_size + 2 * child->padding;

          idx[child->pack] += 1;
        }

      req->data = child;
    }

  /* Determine size of center */
  if (priv->center->expand)
    center_size = MAX (box_size - 2 * MAX (nat_size[0], nat_size[1]), center_req.minimum_size);
  else
    center_size = MAX (MIN (center_req.natural_size, box_size - min_size[0] - min_size[1]), center_req.minimum_size);

  if (priv->homogeneous)
    {
      extra[0] = nvis[0] ? ((box_size - center_size) / 2 - nvis[0] * priv->spacing) / nvis[0] : 0;
      extra[1] = nvis[1] ? ((box_size - center_size) / 2 - nvis[1] * priv->spacing) / nvis[1] : 0;
      extra[0] = MIN (extra[0], extra[1]);
      n_extra_widgets[0] = 0;
    }
  else
    {
      for (packing = CTK_PACK_START; packing <= CTK_PACK_END; packing++)
        {
          gint s;
          /* Distribute the remainder naturally on each side */
          s = MIN ((box_size - center_size) / 2 - min_size[packing], box_size - center_size - min_size[0] - min_size[1]);
          s = ctk_distribute_natural_allocation (MAX (0, s), nvis[packing], sizes[packing]);

          /* Calculate space which hasn't distributed yet,
           * and is available for expanding children.
           */
          if (nexp[packing] > 0)
            {
              extra[packing] = s / nexp[packing];
              n_extra_widgets[packing] = s % nexp[packing];
            }
           else
	     extra[packing] = 0;
        }
    }

  /* Allocate child sizes. */
  for (packing = CTK_PACK_START; packing <= CTK_PACK_END; ++packing)
    {
      for (i = 0, children = priv->children; children; children = children->next)
        {
          child = children->data;

          /* If widget is not visible, skip it. */
          if (!_ctk_widget_get_visible (child->widget))
            continue;

          /* Skip the center widget */
          if (child == priv->center)
            continue;

          /* If widget is packed differently, skip it. */
          if (child->pack != packing)
            continue;

          /* Assign the child's size. */
          if (priv->homogeneous)
            {
              child_size = extra[0];

              if (n_extra_widgets[0] > 0)
                {
                  child_size++;
                  n_extra_widgets[0]--;
                }
            }
          else
            {
              child_size = sizes[packing][i].minimum_size + child->padding * 2;

              if (child->expand || ctk_widget_compute_expand (child->widget, priv->orientation))
                {
                  child_size += extra[packing];

                  if (n_extra_widgets[packing] > 0)
                    {
                      child_size++;
                      n_extra_widgets[packing]--;
                    }
                }
            }

          sizes[packing][i].natural_size = child_size;

          if (priv->orientation == CTK_ORIENTATION_HORIZONTAL &&
              ctk_widget_get_valign_with_baseline (child->widget) == CTK_ALIGN_BASELINE)
            {
              gint child_allocation_width;
	      gint child_minimum_height, child_natural_height;

              if (child->fill)
                child_allocation_width = MAX (1, child_size - child->padding * 2);
              else
                child_allocation_width = sizes[packing][i].minimum_size;

              child_minimum_baseline = -1;
              child_natural_baseline = -1;
              ctk_widget_get_preferred_height_and_baseline_for_width (child->widget,
                                                                      child_allocation_width,
                                                                      &child_minimum_height, &child_natural_height,
                                                                      &child_minimum_baseline, &child_natural_baseline);

              if (child_minimum_baseline >= 0)
                {
                  have_baseline = TRUE;
                  minimum_below = MAX (minimum_below, child_minimum_height - child_minimum_baseline);
                  natural_below = MAX (natural_below, child_natural_height - child_natural_baseline);
                  minimum_above = MAX (minimum_above, child_minimum_baseline);
                  natural_above = MAX (natural_above, child_natural_baseline);
                }
            }

          i++;
        }
    }

  baseline = ctk_widget_get_allocated_baseline (widget);
  if (baseline == -1 && have_baseline)
    {
      gint height = MAX (1, allocation->height);

     /* TODO: This is purely based on the minimum baseline, when things fit we should
      * use the natural one?
      */
      switch (priv->baseline_pos)
        {
        case CTK_BASELINE_POSITION_TOP:
          baseline = minimum_above;
          break;
        case CTK_BASELINE_POSITION_CENTER:
          baseline = minimum_above + (height - (minimum_above + minimum_below)) / 2;
          break;
        case CTK_BASELINE_POSITION_BOTTOM:
          baseline = height - minimum_below;
          break;
        }
    }

  /* Allocate child positions. */
  for (packing = CTK_PACK_START; packing <= CTK_PACK_END; ++packing)
    {
      if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
        {
          child_allocation.y = allocation->y;
          child_allocation.height = MAX (1, allocation->height);
          if ((packing == CTK_PACK_START && direction == CTK_TEXT_DIR_LTR) ||
              (packing == CTK_PACK_END && direction == CTK_TEXT_DIR_RTL))
            x = allocation->x;
          else
            x = allocation->x + allocation->width;
        }
      else
        {
          child_allocation.x = allocation->x;
          child_allocation.width = MAX (1, allocation->width);
          if (packing == CTK_PACK_START)
            y = allocation->y;
          else
            y = allocation->y + allocation->height;
        }

      for (i = 0, children = priv->children; children; children = children->next)
        {
          child = children->data;

          /* If widget is not visible, skip it. */
          if (!_ctk_widget_get_visible (child->widget))
            continue;

          /* Skip the center widget */
          if (child == priv->center)
            continue;

          /* If widget is packed differently, skip it. */
          if (child->pack != packing)
            continue;

          child_size = sizes[packing][i].natural_size;

          /* Assign the child's position. */
          if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
            {
              if (child->fill)
                {
                  child_allocation.width = MAX (1, child_size - child->padding * 2);
                  child_allocation.x = x + child->padding;
                }
              else
                {
                  child_allocation.width = sizes[packing][i].minimum_size;
                  child_allocation.x = x + (child_size - child_allocation.width) / 2;
                }

              if ((packing == CTK_PACK_START && direction == CTK_TEXT_DIR_LTR) ||
                  (packing == CTK_PACK_END && direction == CTK_TEXT_DIR_RTL))
                {
                  x += child_size + priv->spacing;
                }
              else
                {
                  x -= child_size + priv->spacing;
                  child_allocation.x -= child_size;
                }
            }
          else /* (private->orientation == CTK_ORIENTATION_VERTICAL) */
            {
              if (child->fill)
                {
                  child_allocation.height = MAX (1, child_size - child->padding * 2);
                  child_allocation.y = y + child->padding;
                }
              else
                {
                  child_allocation.height = sizes[packing][i].minimum_size;
                  child_allocation.y = y + (child_size - child_allocation.height) / 2;
                }

              if (packing == CTK_PACK_START)
                {
                  y += child_size + priv->spacing;
                }
              else
                {
                  y -= child_size + priv->spacing;
                  child_allocation.y -= child_size;
                }
            }
          ctk_widget_size_allocate_with_baseline (child->widget, &child_allocation, baseline);

          i++;
        }

      if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
        side[packing] = x;
      else
        side[packing] = y;
    }

  /* Allocate the center widget */
  if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
    center_pos = allocation->x + (box_size - center_size) / 2;
  else
    center_pos = allocation->y + (box_size - center_size) / 2;

  if (priv->orientation == CTK_ORIENTATION_HORIZONTAL &&
      direction == CTK_TEXT_DIR_RTL)
    packing = CTK_PACK_END;
  else
    packing = CTK_PACK_START;

  if (center_pos < side[packing])
    center_pos = side[packing];
  else if (center_pos + center_size > side[1 - packing])
    center_pos = side[1 - packing] - center_size;

  if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      child_allocation.x = center_pos;
      child_allocation.width = center_size;
    }
  else
    {
      child_allocation.y = center_pos;
      child_allocation.height = center_size;
    }
  ctk_widget_size_allocate_with_baseline (priv->center->widget, &child_allocation, baseline);

  _ctk_widget_set_simple_clip (widget, NULL);
}

static void
ctk_box_allocate_contents (CtkCssGadget        *gadget,
                           const CtkAllocation *allocation,
                           int                  baseline G_GNUC_UNUSED,
                           CtkAllocation       *out_clip,
                           gpointer             unused G_GNUC_UNUSED)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkBox *box = CTK_BOX (widget);

  if (box->priv->center &&
      _ctk_widget_get_visible (box->priv->center->widget))
    ctk_box_size_allocate_with_center (widget, allocation);
  else
    ctk_box_size_allocate_no_center (widget, allocation);

  ctk_container_get_children_clip (CTK_CONTAINER (box), out_clip);
}

static void
ctk_box_size_allocate (CtkWidget     *widget,
                       CtkAllocation *allocation)
{
  CtkBoxPrivate *priv = CTK_BOX (widget)->priv;
  CtkAllocation clip;

  ctk_widget_set_allocation (widget, allocation);

  ctk_css_gadget_allocate (priv->gadget,
                           allocation,
                           ctk_widget_get_allocated_baseline (widget),
                           &clip);

  ctk_widget_set_clip (widget, &clip);
}

static GType
ctk_box_child_type (CtkContainer   *container G_GNUC_UNUSED)
{
  return CTK_TYPE_WIDGET;
}

static void
ctk_box_set_child_property (CtkContainer *container,
                            CtkWidget    *child,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  gboolean expand = 0;
  gboolean fill = 0;
  guint padding = 0;
  CtkPackType pack_type = 0;

  if (property_id != CHILD_PROP_POSITION)
    ctk_box_query_child_packing (CTK_BOX (container),
				 child,
				 &expand,
				 &fill,
				 &padding,
				 &pack_type);
  switch (property_id)
    {
    case CHILD_PROP_EXPAND:
      ctk_box_set_child_packing (CTK_BOX (container),
				 child,
				 g_value_get_boolean (value),
				 fill,
				 padding,
				 pack_type);
      break;
    case CHILD_PROP_FILL:
      ctk_box_set_child_packing (CTK_BOX (container),
				 child,
				 expand,
				 g_value_get_boolean (value),
				 padding,
				 pack_type);
      break;
    case CHILD_PROP_PADDING:
      ctk_box_set_child_packing (CTK_BOX (container),
				 child,
				 expand,
				 fill,
				 g_value_get_uint (value),
				 pack_type);
      break;
    case CHILD_PROP_PACK_TYPE:
      ctk_box_set_child_packing (CTK_BOX (container),
				 child,
				 expand,
				 fill,
				 padding,
				 g_value_get_enum (value));
      break;
    case CHILD_PROP_POSITION:
      ctk_box_reorder_child (CTK_BOX (container),
			     child,
			     g_value_get_int (value));
      break;
    default:
      CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
ctk_box_get_child_property (CtkContainer *container,
			    CtkWidget    *child,
			    guint         property_id,
			    GValue       *value,
			    GParamSpec   *pspec)
{
  gboolean expand = FALSE;
  gboolean fill = FALSE;
  guint padding = 0;
  CtkPackType pack_type = 0;
  GList *list;

  if (property_id != CHILD_PROP_POSITION)
    ctk_box_query_child_packing (CTK_BOX (container),
				 child,
				 &expand,
				 &fill,
				 &padding,
				 &pack_type);
  switch (property_id)
    {
      guint i;
    case CHILD_PROP_EXPAND:
      g_value_set_boolean (value, expand);
      break;
    case CHILD_PROP_FILL:
      g_value_set_boolean (value, fill);
      break;
    case CHILD_PROP_PADDING:
      g_value_set_uint (value, padding);
      break;
    case CHILD_PROP_PACK_TYPE:
      g_value_set_enum (value, pack_type);
      break;
    case CHILD_PROP_POSITION:
      i = 0;
      for (list = CTK_BOX (container)->priv->children; list; list = list->next)
	{
	  CtkBoxChild *child_entry;

	  child_entry = list->data;
	  if (child_entry->widget == child)
	    break;
	  i++;
	}
      g_value_set_int (value, list ? i : -1);
      break;
    default:
      CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

typedef struct _CountingData CountingData;
struct _CountingData {
  CtkWidget *widget;
  gboolean found;
  guint before;
  guint after;
};

static void
count_widget_position (CtkWidget *widget,
                       gpointer   data)
{
  CountingData *count = data;

  if (!_ctk_widget_get_visible (widget))
    return;

  if (count->widget == widget)
    count->found = TRUE;
  else if (count->found)
    count->after++;
  else
    count->before++;
}

static gint
ctk_box_get_visible_position (CtkBox    *box,
                              CtkWidget *child)
{
  CountingData count = { child, FALSE, 0, 0 };

  /* foreach iterates in visible order */
  ctk_container_foreach (CTK_CONTAINER (box),
                         count_widget_position,
                         &count);

  /* the child wasn't found, it's likely an internal child of some
   * subclass, return -1 to indicate that there is no sibling relation
   * to the regular box children
   */
  if (!count.found)
    return -1;

  if (box->priv->orientation == CTK_ORIENTATION_HORIZONTAL &&
      ctk_widget_get_direction (CTK_WIDGET (box)) == CTK_TEXT_DIR_RTL)
    return count.after;
  else
    return count.before;
}

static CtkWidgetPath *
ctk_box_get_path_for_child (CtkContainer *container,
                            CtkWidget    *child)
{
  CtkWidgetPath *path, *sibling_path;
  CtkBox *box;
  CtkBoxPrivate *private;
  GList *list, *children;

  box = CTK_BOX (container);
  private = box->priv;

  path = _ctk_widget_create_path (CTK_WIDGET (container));

  if (_ctk_widget_get_visible (child))
    {
      gint position;

      sibling_path = ctk_widget_path_new ();

      /* get_children works in visible order */
      children = ctk_container_get_children (container);
      if (private->orientation == CTK_ORIENTATION_HORIZONTAL &&
          ctk_widget_get_direction (CTK_WIDGET (box)) == CTK_TEXT_DIR_RTL)
        children = g_list_reverse (children);

      for (list = children; list; list = list->next)
        {
          if (!_ctk_widget_get_visible (list->data))
            continue;

          ctk_widget_path_append_for_widget (sibling_path, list->data);
        }

      g_list_free (children);

      position = ctk_box_get_visible_position (box, child);

      if (position >= 0)
        ctk_widget_path_append_with_siblings (path, sibling_path, position);
      else
        ctk_widget_path_append_for_widget (path, child);

      ctk_widget_path_unref (sibling_path);
    }
  else
    ctk_widget_path_append_for_widget (path, child);

  return path;
}

static void
ctk_box_buildable_add_child (CtkBuildable *buildable,
                             CtkBuilder   *builder G_GNUC_UNUSED,
                             GObject      *child,
                             const gchar  *type)
{
  if (type && strcmp (type, "center") == 0)
    ctk_box_set_center_widget (CTK_BOX (buildable), CTK_WIDGET (child));
  else if (!type)
    ctk_container_add (CTK_CONTAINER (buildable), CTK_WIDGET (child));
  else
    CTK_BUILDER_WARN_INVALID_CHILD_TYPE (CTK_BOX (buildable), type);
}

static void
ctk_box_buildable_init (CtkBuildableIface *iface)
{
  iface->add_child = ctk_box_buildable_add_child;
}

static void
ctk_box_update_child_css_position (CtkBox      *box,
                                   CtkBoxChild *child_info)
{
  CtkBoxPrivate *priv = box->priv;
  CtkBoxChild *prev;
  gboolean reverse;
  GList *l;

  prev = NULL;
  for (l = priv->children; l->data != child_info; l = l->next)
    {
      CtkBoxChild *cur = l->data;

      if (cur->pack == child_info->pack)
        prev = cur;
    }

  reverse = child_info->pack == CTK_PACK_END;
  if (box->priv->orientation == CTK_ORIENTATION_HORIZONTAL &&
      ctk_widget_get_direction (CTK_WIDGET (box)) == CTK_TEXT_DIR_RTL)
    reverse = !reverse;

  if (reverse)
    ctk_css_node_insert_before (ctk_widget_get_css_node (CTK_WIDGET (box)),
                                ctk_widget_get_css_node (child_info->widget),
                                prev ? ctk_widget_get_css_node (prev->widget) : NULL);
  else
    ctk_css_node_insert_after (ctk_widget_get_css_node (CTK_WIDGET (box)),
                               ctk_widget_get_css_node (child_info->widget),
                               prev ? ctk_widget_get_css_node (prev->widget) : NULL);
}

static void
ctk_box_direction_changed (CtkWidget        *widget,
                           CtkTextDirection  previous_direction G_GNUC_UNUSED)
{
  CtkBox *box = CTK_BOX (widget);

  if (box->priv->orientation == CTK_ORIENTATION_HORIZONTAL)
    ctk_css_node_reverse_children (ctk_widget_get_css_node (widget));
}

static CtkBoxChild *
ctk_box_pack (CtkBox      *box,
              CtkWidget   *child,
              gboolean     expand,
              gboolean     fill,
              guint        padding,
              CtkPackType  pack_type)
{
  CtkContainer *container = CTK_CONTAINER (box);
  CtkBoxPrivate *private = box->priv;
  CtkBoxChild *child_info;

  g_return_val_if_fail (CTK_IS_BOX (box), NULL);
  g_return_val_if_fail (CTK_IS_WIDGET (child), NULL);
  g_return_val_if_fail (_ctk_widget_get_parent (child) == NULL, NULL);

  child_info = g_new (CtkBoxChild, 1);
  child_info->widget = child;
  child_info->padding = padding;
  child_info->expand = expand ? TRUE : FALSE;
  child_info->fill = fill ? TRUE : FALSE;
  child_info->pack = pack_type;

  private->children = g_list_append (private->children, child_info);
  ctk_box_update_child_css_position (box, child_info);

  ctk_widget_freeze_child_notify (child);

  ctk_widget_set_parent (child, CTK_WIDGET (box));

  if (expand)
    ctk_container_child_notify_by_pspec (container, child, child_props[CHILD_PROP_EXPAND]);
  if (!fill)
    ctk_container_child_notify_by_pspec (container, child, child_props[CHILD_PROP_FILL]);
  if (padding != 0)
    ctk_container_child_notify_by_pspec (container, child, child_props[CHILD_PROP_PADDING]);
  if (pack_type != CTK_PACK_START)
    ctk_container_child_notify_by_pspec (container, child, child_props[CHILD_PROP_PACK_TYPE]);
  ctk_container_child_notify_by_pspec (container, child, child_props[CHILD_PROP_POSITION]);

  ctk_widget_thaw_child_notify (child);

  return child_info;
}

static void
ctk_box_get_size (CtkWidget      *widget,
		  CtkOrientation  orientation,
		  gint           *minimum_size,
		  gint           *natural_size,
		  gint           *minimum_baseline,
		  gint           *natural_baseline)
{
  CtkBox *box;
  CtkBoxPrivate *private;
  GList *children;
  gint nvis_children;
  gint minimum, natural;
  gint minimum_above, natural_above;
  gint minimum_below, natural_below;
  gboolean have_baseline;
  gint min_baseline, nat_baseline;
  gint center_min, center_nat;

  box = CTK_BOX (widget);
  private = box->priv;

  have_baseline = FALSE;
  minimum = natural = 0;
  minimum_above = natural_above = 0;
  minimum_below = natural_below = 0;
  min_baseline = nat_baseline = -1;

  nvis_children = 0;

  center_min = center_nat = 0;

  for (children = private->children; children; children = children->next)
    {
      CtkBoxChild *child = children->data;

      if (_ctk_widget_get_visible (child->widget))
        {
          gint child_minimum, child_natural;
          gint child_minimum_baseline = -1, child_natural_baseline = -1;

	  if (orientation == CTK_ORIENTATION_HORIZONTAL)
	    ctk_widget_get_preferred_width (child->widget,
                                            &child_minimum, &child_natural);
	  else
	    ctk_widget_get_preferred_height_and_baseline_for_width (child->widget, -1,
								    &child_minimum, &child_natural,
								    &child_minimum_baseline, &child_natural_baseline);

          if (private->orientation == orientation)
	    {
              if (private->homogeneous)
                {
                  if (child == private->center)
                    {
                      center_min = child_minimum + child->padding * 2;
                      center_nat = child_natural + child->padding * 2;
                    }
                  else
                    {
                      gint largest;

                      largest = child_minimum + child->padding * 2;
                      minimum = MAX (minimum, largest);

                      largest = child_natural + child->padding * 2;
                      natural = MAX (natural, largest);
                    }
                }
              else
                {
                  minimum += child_minimum + child->padding * 2;
                  natural += child_natural + child->padding * 2;
                }
	    }
	  else
	    {
	      if (child_minimum_baseline >= 0)
		{
		  have_baseline = TRUE;
		  minimum_below = MAX (minimum_below, child_minimum - child_minimum_baseline);
		  natural_below = MAX (natural_below, child_natural - child_natural_baseline);
		  minimum_above = MAX (minimum_above, child_minimum_baseline);
		  natural_above = MAX (natural_above, child_natural_baseline);
		}
	      else
		{
		  /* The biggest mins and naturals in the opposing orientation */
		  minimum = MAX (minimum, child_minimum);
		  natural = MAX (natural, child_natural);
		}
	    }

          nvis_children += 1;
        }
    }

  if (nvis_children > 0 && private->orientation == orientation)
    {
      if (private->homogeneous)
	{
          if (center_min > 0)
            {
              minimum = minimum * (nvis_children - 1) + center_min;
              natural = natural * (nvis_children - 1) + center_nat;
            }
          else
            {
	      minimum *= nvis_children;
	      natural *= nvis_children;
            }
	}
      minimum += (nvis_children - 1) * private->spacing;
      natural += (nvis_children - 1) * private->spacing;
    }

  minimum = MAX (minimum, minimum_below + minimum_above);
  natural = MAX (natural, natural_below + natural_above);

  if (have_baseline)
    {
      switch (private->baseline_pos)
	{
	case CTK_BASELINE_POSITION_TOP:
	  min_baseline = minimum_above;
	  nat_baseline = natural_above;
	  break;
	case CTK_BASELINE_POSITION_CENTER:
	  min_baseline = minimum_above + (minimum - (minimum_above + minimum_below)) / 2;
	  nat_baseline = natural_above + (natural - (natural_above + natural_below)) / 2;
	  break;
	case CTK_BASELINE_POSITION_BOTTOM:
	  min_baseline = minimum - minimum_below;
	  nat_baseline = natural - natural_below;
	  break;
	}
    }

  if (minimum_size)
    *minimum_size = minimum;

  if (natural_size)
    *natural_size = natural;

  if (minimum_baseline)
    *minimum_baseline = min_baseline;

  if (natural_baseline)
    *natural_baseline = nat_baseline;
}

static void
ctk_box_get_preferred_width (CtkWidget *widget,
                             gint      *minimum,
                             gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CTK_BOX (widget)->priv->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_box_get_preferred_height (CtkWidget *widget,
                              gint      *minimum,
                              gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CTK_BOX (widget)->priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_box_compute_size_for_opposing_orientation (CtkBox *box,
					       gint    avail_size,
					       gint   *minimum_size,
					       gint   *natural_size,
					       gint   *minimum_baseline,
					       gint   *natural_baseline)
{
  CtkBoxPrivate       *private = box->priv;
  CtkBoxChild      *child;
  GList            *children;
  gint              nvis_children;
  gint              nexpand_children;
  gint              computed_minimum = 0, computed_natural = 0;
  gint              computed_minimum_above = 0, computed_natural_above = 0;
  gint              computed_minimum_below = 0, computed_natural_below = 0;
  gint              computed_minimum_baseline = -1, computed_natural_baseline = -1;
  CtkRequestedSize *sizes;
  CtkPackType       packing;
  gint              size, extra, i;
  gint              child_size, child_minimum, child_natural;
  gint              child_minimum_baseline, child_natural_baseline;
  gint              n_extra_widgets = 0;
  gboolean          have_baseline;

  count_expand_children (box, &nvis_children, &nexpand_children);

  if (nvis_children <= 0)
    return;

  sizes = g_newa (CtkRequestedSize, nvis_children);
  memset (sizes, 0, nvis_children * sizeof (CtkRequestedSize));
  size = avail_size - (nvis_children - 1) * private->spacing;

  /* Retrieve desired size for visible children */
  for (i = 0, children = private->children; children; children = children->next)
    {
      child = children->data;

      if (_ctk_widget_get_visible (child->widget))
	{
	  if (private->orientation == CTK_ORIENTATION_HORIZONTAL)
	    ctk_widget_get_preferred_width (child->widget,
                                            &sizes[i].minimum_size,
                                            &sizes[i].natural_size);
	  else
	    ctk_widget_get_preferred_height (child->widget,
                                             &sizes[i].minimum_size,
                                             &sizes[i].natural_size);

	  /* Assert the api is working properly */
	  if (sizes[i].minimum_size < 0)
	    g_error ("CtkBox child %s minimum %s: %d < 0",
		     ctk_widget_get_name (CTK_WIDGET (child->widget)),
		     (private->orientation == CTK_ORIENTATION_HORIZONTAL) ? "width" : "height",
		     sizes[i].minimum_size);

	  if (sizes[i].natural_size < sizes[i].minimum_size)
	    g_error ("CtkBox child %s natural %s: %d < minimum %d",
		     ctk_widget_get_name (CTK_WIDGET (child->widget)),
		     (private->orientation == CTK_ORIENTATION_HORIZONTAL) ? "width" : "height",
		     sizes[i].natural_size,
		     sizes[i].minimum_size);

	  size -= sizes[i].minimum_size;
	  size -= child->padding * 2;

	  sizes[i].data = child;

	  i += 1;
	}
    }

  if (private->homogeneous)
    {
      /* If were homogenous we still need to run the above loop to get the
       * minimum sizes for children that are not going to fill
       */
      size = avail_size - (nvis_children - 1) * private->spacing;
      extra = size / nvis_children;
      n_extra_widgets = size % nvis_children;
    }
  else
    {
      /* Bring children up to size first */
      size = ctk_distribute_natural_allocation (MAX (0, size), nvis_children, sizes);

      /* Calculate space which hasn't distributed yet,
       * and is available for expanding children.
       */
      if (nexpand_children > 0)
	{
	  extra = size / nexpand_children;
	  n_extra_widgets = size % nexpand_children;
	}
      else
	extra = 0;
    }

  have_baseline = FALSE;
  /* Allocate child positions. */
  for (packing = CTK_PACK_START; packing <= CTK_PACK_END; ++packing)
    {
      for (i = 0, children = private->children;
	   children;
	   children = children->next)
	{
	  child = children->data;

	  /* If widget is not visible, skip it. */
	  if (!_ctk_widget_get_visible (child->widget))
	    continue;

	  /* If widget is packed differently skip it, but still increment i,
	   * since widget is visible and will be handled in next loop iteration.
	   */
	  if (child->pack != packing)
	    {
	      i++;
	      continue;
	    }

	  if (child->pack == packing)
	    {
	      /* Assign the child's size. */
	      if (private->homogeneous)
		{
		  child_size = extra;

		  if (n_extra_widgets > 0)
		    {
		      child_size++;
		      n_extra_widgets--;
		    }
		}
	      else
		{
		  child_size = sizes[i].minimum_size + child->padding * 2;

		  if (child->expand || ctk_widget_compute_expand (child->widget, private->orientation))
		    {
		      child_size += extra;

		      if (n_extra_widgets > 0)
			{
			  child_size++;
			  n_extra_widgets--;
			}
		    }
		}

	      if (child->fill)
		{
		  child_size = MAX (1, child_size - child->padding * 2);
		}
	      else
		{
		  child_size = sizes[i].minimum_size;
		}


	      child_minimum_baseline = child_natural_baseline = -1;
	      /* Assign the child's position. */
	      if (private->orientation == CTK_ORIENTATION_HORIZONTAL)
		ctk_widget_get_preferred_height_and_baseline_for_width (child->widget, child_size,
									&child_minimum, &child_natural,
									&child_minimum_baseline, &child_natural_baseline);
	      else /* (private->orientation == CTK_ORIENTATION_VERTICAL) */
		ctk_widget_get_preferred_width_for_height (child->widget,
                                                           child_size, &child_minimum, &child_natural);

	      if (child_minimum_baseline >= 0)
		{
		  have_baseline = TRUE;
		  computed_minimum_below = MAX (computed_minimum_below, child_minimum - child_minimum_baseline);
		  computed_natural_below = MAX (computed_natural_below, child_natural - child_natural_baseline);
		  computed_minimum_above = MAX (computed_minimum_above, child_minimum_baseline);
		  computed_natural_above = MAX (computed_natural_above, child_natural_baseline);
		}
	      else
		{
		  computed_minimum = MAX (computed_minimum, child_minimum);
		  computed_natural = MAX (computed_natural, child_natural);
		}
	    }
	  i += 1;
	}
    }

  if (have_baseline)
    {
      computed_minimum = MAX (computed_minimum, computed_minimum_below + computed_minimum_above);
      computed_natural = MAX (computed_natural, computed_natural_below + computed_natural_above);
      switch (private->baseline_pos)
	{
	case CTK_BASELINE_POSITION_TOP:
	  computed_minimum_baseline = computed_minimum_above;
	  computed_natural_baseline = computed_natural_above;
	  break;
	case CTK_BASELINE_POSITION_CENTER:
	  computed_minimum_baseline = computed_minimum_above + MAX((computed_minimum - (computed_minimum_above + computed_minimum_below)) / 2, 0);
	  computed_natural_baseline = computed_natural_above + MAX((computed_natural - (computed_natural_above + computed_natural_below)) / 2, 0);
	  break;
	case CTK_BASELINE_POSITION_BOTTOM:
	  computed_minimum_baseline = computed_minimum - computed_minimum_below;
	  computed_natural_baseline = computed_natural - computed_natural_below;
	  break;
	}
    }

  if (minimum_baseline)
    *minimum_baseline = computed_minimum_baseline;
  if (natural_baseline)
    *natural_baseline = computed_natural_baseline;

  if (minimum_size)
    *minimum_size = computed_minimum;
  if (natural_size)
    *natural_size = MAX (computed_natural, computed_natural_below + computed_natural_above);
}

static void
ctk_box_compute_size_for_orientation (CtkBox *box,
				      gint    avail_size,
				      gint   *minimum_size,
				      gint   *natural_size)
{
  CtkBoxPrivate    *private = box->priv;
  GList         *children;
  gint           nvis_children = 0;
  gint           required_size = 0, required_natural = 0, child_size, child_natural;
  gint           largest_child = 0, largest_natural = 0;

  for (children = private->children; children != NULL;
       children = children->next)
    {
      CtkBoxChild *child = children->data;

      if (_ctk_widget_get_visible (child->widget))
        {

          if (private->orientation == CTK_ORIENTATION_HORIZONTAL)
	    ctk_widget_get_preferred_width_for_height (child->widget,
                                                       avail_size, &child_size, &child_natural);
	  else
	    ctk_widget_get_preferred_height_for_width (child->widget,
						       avail_size, &child_size, &child_natural);


	  child_size    += child->padding * 2;
	  child_natural += child->padding * 2;

	  if (child_size > largest_child)
	    largest_child = child_size;

	  if (child_natural > largest_natural)
	    largest_natural = child_natural;

	  required_size    += child_size;
	  required_natural += child_natural;

          nvis_children += 1;
        }
    }

  if (nvis_children > 0)
    {
      if (private->homogeneous)
	{
	  required_size    = largest_child   * nvis_children;
	  required_natural = largest_natural * nvis_children;
	}

      required_size     += (nvis_children - 1) * private->spacing;
      required_natural  += (nvis_children - 1) * private->spacing;
    }

  if (minimum_size)
    *minimum_size = required_size;

  if (natural_size)
    *natural_size = required_natural;
}

static void
ctk_box_get_preferred_width_for_height (CtkWidget *widget,
                                        gint       height,
                                        gint      *minimum,
                                        gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CTK_BOX (widget)->priv->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     height,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_box_get_preferred_height_and_baseline_for_width (CtkWidget *widget,
						     gint       width,
						     gint      *minimum,
						     gint      *natural,
						     gint      *minimum_baseline,
						     gint      *natural_baseline)
{
  ctk_css_gadget_get_preferred_size (CTK_BOX (widget)->priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     width,
                                     minimum, natural,
                                     minimum_baseline, natural_baseline);
}

static void
ctk_box_get_content_size (CtkCssGadget   *gadget,
                          CtkOrientation  orientation,
                          gint            for_size,
                          gint           *minimum,
                          gint           *natural,
                          gint           *minimum_baseline,
                          gint           *natural_baseline,
                          gpointer        unused G_GNUC_UNUSED)
{
  CtkWidget     *widget  = ctk_css_gadget_get_owner (gadget);
  CtkBox        *box     = CTK_BOX (widget);
  CtkBoxPrivate *private = box->priv;

  if (for_size < 0)
    ctk_box_get_size (widget, orientation, minimum, natural, minimum_baseline, natural_baseline);
  else
    {
      if (private->orientation != orientation)
	ctk_box_compute_size_for_opposing_orientation (box, for_size, minimum, natural, minimum_baseline, natural_baseline);
      else
	{
	  if (minimum_baseline)
	    *minimum_baseline = -1;
	  if (natural_baseline)
	    *natural_baseline = -1;
	  ctk_box_compute_size_for_orientation (box, for_size, minimum, natural);
	}
    }
}

static void
ctk_box_get_preferred_height_for_width (CtkWidget *widget,
                                        gint       width,
                                        gint      *minimum_height,
                                        gint      *natural_height)
{
  ctk_box_get_preferred_height_and_baseline_for_width (widget, width, minimum_height, natural_height, NULL, NULL);
}

static void
ctk_box_init (CtkBox *box)
{
  CtkBoxPrivate *private;

  box->priv = ctk_box_get_instance_private (box);
  private = box->priv;

  ctk_widget_set_has_window (CTK_WIDGET (box), FALSE);

  private->orientation = CTK_ORIENTATION_HORIZONTAL;
  private->children = NULL;

  private->default_expand = FALSE;
  private->homogeneous = FALSE;
  private->spacing = 0;
  private->spacing_set = FALSE;
  private->baseline_pos = CTK_BASELINE_POSITION_CENTER;

  private->gadget = ctk_css_custom_gadget_new_for_node (ctk_widget_get_css_node (CTK_WIDGET (box)),
                                                        CTK_WIDGET (box),
                                                        ctk_box_get_content_size,
                                                        ctk_box_allocate_contents,
                                                        ctk_box_draw_contents,
                                                        NULL,
                                                        NULL);

  _ctk_orientable_set_style_classes (CTK_ORIENTABLE (box));
}

CtkCssGadget *
ctk_box_get_gadget (CtkBox *box)
{
  return box->priv->gadget;
}

/**
 * ctk_box_new:
 * @orientation: the box’s orientation.
 * @spacing: the number of pixels to place by default between children.
 *
 * Creates a new #CtkBox.
 *
 * Returns: a new #CtkBox.
 *
 * Since: 3.0
 **/
CtkWidget*
ctk_box_new (CtkOrientation orientation,
             gint           spacing)
{
  return g_object_new (CTK_TYPE_BOX,
                       "orientation", orientation,
                       "spacing",     spacing,
                       NULL);
}

/**
 * ctk_box_pack_start:
 * @box: a #CtkBox
 * @child: the #CtkWidget to be added to @box
 * @expand: %TRUE if the new child is to be given extra space allocated
 *     to @box. The extra space will be divided evenly between all children
 *     that use this option
 * @fill: %TRUE if space given to @child by the @expand option is
 *     actually allocated to @child, rather than just padding it.  This
 *     parameter has no effect if @expand is set to %FALSE.  A child is
 *     always allocated the full height of a horizontal #CtkBox and the full width
 *     of a vertical #CtkBox. This option affects the other dimension
 * @padding: extra space in pixels to put between this child and its
 *   neighbors, over and above the global amount specified by
 *   #CtkBox:spacing property.  If @child is a widget at one of the
 *   reference ends of @box, then @padding pixels are also put between
 *   @child and the reference edge of @box
 *
 * Adds @child to @box, packed with reference to the start of @box.
 * The @child is packed after any other child packed with reference
 * to the start of @box.
 */
void
ctk_box_pack_start (CtkBox    *box,
		    CtkWidget *child,
		    gboolean   expand,
		    gboolean   fill,
		    guint      padding)
{
  ctk_box_pack (box, child, expand, fill, padding, CTK_PACK_START);
}

/**
 * ctk_box_pack_end:
 * @box: a #CtkBox
 * @child: the #CtkWidget to be added to @box
 * @expand: %TRUE if the new child is to be given extra space allocated
 *   to @box. The extra space will be divided evenly between all children
 *   of @box that use this option
 * @fill: %TRUE if space given to @child by the @expand option is
 *   actually allocated to @child, rather than just padding it.  This
 *   parameter has no effect if @expand is set to %FALSE.  A child is
 *   always allocated the full height of a horizontal #CtkBox and the full width
 *   of a vertical #CtkBox.  This option affects the other dimension
 * @padding: extra space in pixels to put between this child and its
 *   neighbors, over and above the global amount specified by
 *   #CtkBox:spacing property.  If @child is a widget at one of the
 *   reference ends of @box, then @padding pixels are also put between
 *   @child and the reference edge of @box
 *
 * Adds @child to @box, packed with reference to the end of @box.
 * The @child is packed after (away from end of) any other child
 * packed with reference to the end of @box.
 */
void
ctk_box_pack_end (CtkBox    *box,
		  CtkWidget *child,
		  gboolean   expand,
		  gboolean   fill,
		  guint      padding)
{
  ctk_box_pack (box, child, expand, fill, padding, CTK_PACK_END);
}

/**
 * ctk_box_set_homogeneous:
 * @box: a #CtkBox
 * @homogeneous: a boolean value, %TRUE to create equal allotments,
 *   %FALSE for variable allotments
 *
 * Sets the #CtkBox:homogeneous property of @box, controlling
 * whether or not all children of @box are given equal space
 * in the box.
 */
void
ctk_box_set_homogeneous (CtkBox  *box,
			 gboolean homogeneous)
{
  CtkBoxPrivate *private;

  g_return_if_fail (CTK_IS_BOX (box));

  private = box->priv;

  homogeneous = homogeneous != FALSE;

  if (private->homogeneous != homogeneous)
    {
      private->homogeneous = homogeneous;
      g_object_notify_by_pspec (G_OBJECT (box), props[PROP_HOMOGENEOUS]);
      ctk_widget_queue_resize (CTK_WIDGET (box));
    }
}

/**
 * ctk_box_get_homogeneous:
 * @box: a #CtkBox
 *
 * Returns whether the box is homogeneous (all children are the
 * same size). See ctk_box_set_homogeneous().
 *
 * Returns: %TRUE if the box is homogeneous.
 **/
gboolean
ctk_box_get_homogeneous (CtkBox *box)
{
  g_return_val_if_fail (CTK_IS_BOX (box), FALSE);

  return box->priv->homogeneous;
}

/**
 * ctk_box_set_spacing:
 * @box: a #CtkBox
 * @spacing: the number of pixels to put between children
 *
 * Sets the #CtkBox:spacing property of @box, which is the
 * number of pixels to place between children of @box.
 */
void
ctk_box_set_spacing (CtkBox *box,
		     gint    spacing)
{
  CtkBoxPrivate *private;

  g_return_if_fail (CTK_IS_BOX (box));

  private = box->priv;

  if (private->spacing != spacing)
    {
      private->spacing = spacing;
      _ctk_box_set_spacing_set (box, TRUE);

      g_object_notify_by_pspec (G_OBJECT (box), props[PROP_SPACING]);

      ctk_widget_queue_resize (CTK_WIDGET (box));
    }
}

/**
 * ctk_box_get_spacing:
 * @box: a #CtkBox
 *
 * Gets the value set by ctk_box_set_spacing().
 *
 * Returns: spacing between children
 **/
gint
ctk_box_get_spacing (CtkBox *box)
{
  g_return_val_if_fail (CTK_IS_BOX (box), 0);

  return box->priv->spacing;
}

/**
 * ctk_box_set_baseline_position:
 * @box: a #CtkBox
 * @position: a #CtkBaselinePosition
 *
 * Sets the baseline position of a box. This affects
 * only horizontal boxes with at least one baseline aligned
 * child. If there is more vertical space available than requested,
 * and the baseline is not allocated by the parent then
 * @position is used to allocate the baseline wrt the
 * extra space available.
 *
 * Since: 3.10
 */
void
ctk_box_set_baseline_position (CtkBox             *box,
			       CtkBaselinePosition position)
{
  CtkBoxPrivate *private;

  g_return_if_fail (CTK_IS_BOX (box));

  private = box->priv;

  if (private->baseline_pos != position)
    {
      private->baseline_pos = position;

      g_object_notify_by_pspec (G_OBJECT (box), props[PROP_BASELINE_POSITION]);

      ctk_widget_queue_resize (CTK_WIDGET (box));
    }
}

/**
 * ctk_box_get_baseline_position:
 * @box: a #CtkBox
 *
 * Gets the value set by ctk_box_set_baseline_position().
 *
 * Returns: the baseline position
 *
 * Since: 3.10
 **/
CtkBaselinePosition
ctk_box_get_baseline_position (CtkBox *box)
{
  g_return_val_if_fail (CTK_IS_BOX (box), CTK_BASELINE_POSITION_CENTER);

  return box->priv->baseline_pos;
}


void
_ctk_box_set_spacing_set (CtkBox  *box,
                          gboolean spacing_set)
{
  CtkBoxPrivate *private;

  g_return_if_fail (CTK_IS_BOX (box));

  private = box->priv;

  private->spacing_set = spacing_set ? TRUE : FALSE;
}

gboolean
_ctk_box_get_spacing_set (CtkBox *box)
{
  CtkBoxPrivate *private;

  g_return_val_if_fail (CTK_IS_BOX (box), FALSE);

  private = box->priv;

  return private->spacing_set;
}

/**
 * ctk_box_reorder_child:
 * @box: a #CtkBox
 * @child: the #CtkWidget to move
 * @position: the new position for @child in the list of children
 *   of @box, starting from 0. If negative, indicates the end of
 *   the list
 *
 * Moves @child to a new @position in the list of @box children.
 * The list contains widgets packed #CTK_PACK_START
 * as well as widgets packed #CTK_PACK_END, in the order that these
 * widgets were added to @box.
 *
 * A widget’s position in the @box children list determines where
 * the widget is packed into @box.  A child widget at some position
 * in the list will be packed just after all other widgets of the
 * same packing type that appear earlier in the list.
 */
void
ctk_box_reorder_child (CtkBox    *box,
		       CtkWidget *child,
		       gint       position)
{
  CtkBoxPrivate *priv;
  GList *old_link;
  GList *new_link;
  CtkBoxChild *child_info = NULL;
  gint old_position;

  g_return_if_fail (CTK_IS_BOX (box));
  g_return_if_fail (CTK_IS_WIDGET (child));

  priv = box->priv;

  old_link = priv->children;
  old_position = 0;
  while (old_link)
    {
      child_info = old_link->data;
      if (child_info->widget == child)
	break;

      old_link = old_link->next;
      old_position++;
    }

  g_return_if_fail (old_link != NULL);

  if (position == old_position)
    return;

  priv->children = g_list_delete_link (priv->children, old_link);

  if (position < 0)
    new_link = NULL;
  else
    new_link = g_list_nth (priv->children, position);

  priv->children = g_list_insert_before (priv->children, new_link, child_info);
  ctk_box_update_child_css_position (box, child_info);

  ctk_container_child_notify_by_pspec (CTK_CONTAINER (box), child, child_props[CHILD_PROP_POSITION]);
  if (_ctk_widget_get_visible (child) &&
      _ctk_widget_get_visible (CTK_WIDGET (box)))
    {
      ctk_widget_queue_resize (child);
    }
}

/**
 * ctk_box_query_child_packing:
 * @box: a #CtkBox
 * @child: the #CtkWidget of the child to query
 * @expand: (out): pointer to return location for expand child
 *     property
 * @fill: (out): pointer to return location for fill child
 *     property
 * @padding: (out): pointer to return location for padding
 *     child property
 * @pack_type: (out): pointer to return location for pack-type
 *     child property
 *
 * Obtains information about how @child is packed into @box.
 */
void
ctk_box_query_child_packing (CtkBox      *box,
			     CtkWidget   *child,
			     gboolean    *expand,
			     gboolean    *fill,
			     guint       *padding,
			     CtkPackType *pack_type)
{
  CtkBoxPrivate *private;
  GList *list;
  CtkBoxChild *child_info = NULL;

  g_return_if_fail (CTK_IS_BOX (box));
  g_return_if_fail (CTK_IS_WIDGET (child));

  private = box->priv;

  list = private->children;
  while (list)
    {
      child_info = list->data;
      if (child_info->widget == child)
	break;

      list = list->next;
    }

  if (list)
    {
      if (expand)
	*expand = child_info->expand;
      if (fill)
	*fill = child_info->fill;
      if (padding)
	*padding = child_info->padding;
      if (pack_type)
	*pack_type = child_info->pack;
    }
}

/**
 * ctk_box_set_child_packing:
 * @box: a #CtkBox
 * @child: the #CtkWidget of the child to set
 * @expand: the new value of the expand child property
 * @fill: the new value of the fill child property
 * @padding: the new value of the padding child property
 * @pack_type: the new value of the pack-type child property
 *
 * Sets the way @child is packed into @box.
 */
void
ctk_box_set_child_packing (CtkBox      *box,
			   CtkWidget   *child,
			   gboolean     expand,
			   gboolean     fill,
			   guint        padding,
			   CtkPackType  pack_type)
{
  CtkBoxPrivate *private;
  GList *list;
  CtkBoxChild *child_info = NULL;

  g_return_if_fail (CTK_IS_BOX (box));
  g_return_if_fail (CTK_IS_WIDGET (child));

  private = box->priv;

  list = private->children;
  while (list)
    {
      child_info = list->data;
      if (child_info->widget == child)
	break;

      list = list->next;
    }

  ctk_widget_freeze_child_notify (child);
  if (list)
    {
      expand = expand != FALSE;

      if (child_info->expand != expand)
        {
          child_info->expand = expand;
          ctk_container_child_notify_by_pspec (CTK_CONTAINER (box), child, child_props[CHILD_PROP_EXPAND]);
        }

      fill = fill != FALSE;

      if (child_info->fill != fill)
        {
          child_info->fill = fill;
          ctk_container_child_notify_by_pspec (CTK_CONTAINER (box), child, child_props[CHILD_PROP_FILL]);
        }

      if (child_info->padding != padding)
        {
          child_info->padding = padding;
          ctk_container_child_notify_by_pspec (CTK_CONTAINER (box), child, child_props[CHILD_PROP_PADDING]);
        }

      if (pack_type != CTK_PACK_END)
        pack_type = CTK_PACK_START;
      if (child_info->pack != pack_type)
        {
	  child_info->pack = pack_type;
          ctk_box_update_child_css_position (box, child_info);
          ctk_container_child_notify_by_pspec (CTK_CONTAINER (box), child, child_props[CHILD_PROP_PACK_TYPE]);
        }

      if (_ctk_widget_get_visible (child) &&
          _ctk_widget_get_visible (CTK_WIDGET (box)))
	ctk_widget_queue_resize (child);
    }
  ctk_widget_thaw_child_notify (child);
}

void
_ctk_box_set_old_defaults (CtkBox *box)
{
  CtkBoxPrivate *private;

  g_return_if_fail (CTK_IS_BOX (box));

  private = box->priv;

  private->default_expand = TRUE;
}

static void
ctk_box_add (CtkContainer *container,
	     CtkWidget    *widget)
{
  CtkBoxPrivate *priv = CTK_BOX (container)->priv;

  ctk_box_pack_start (CTK_BOX (container), widget,
                      priv->default_expand,
                      TRUE,
                      0);
}

static void
ctk_box_remove (CtkContainer *container,
		CtkWidget    *widget)
{
  CtkBox *box = CTK_BOX (container);
  CtkBoxPrivate *priv = box->priv;
  CtkBoxChild *child;
  GList *children;

  children = priv->children;
  while (children)
    {
      child = children->data;

      if (child->widget == widget)
	{
	  gboolean was_visible;

          if (priv->center == child)
            priv->center = NULL;

	  was_visible = _ctk_widget_get_visible (widget);
	  ctk_widget_unparent (widget);

	  priv->children = g_list_remove_link (priv->children, children);
	  g_list_free (children);
	  g_free (child);

	  /* queue resize regardless of ctk_widget_get_visible (container),
	   * since that's what is needed by toplevels.
	   */
	  if (was_visible)
            {
	      ctk_widget_queue_resize (CTK_WIDGET (container));
            }

	  break;
	}

      children = children->next;
    }
}

static void
ctk_box_forall (CtkContainer *container,
		gboolean      include_internals G_GNUC_UNUSED,
		CtkCallback   callback,
		gpointer      callback_data)
{
  CtkBox *box = CTK_BOX (container);
  CtkBoxPrivate *priv = box->priv;
  CtkBoxChild *child;
  GList *children;

  children = priv->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      if (child == priv->center)
        continue;

      if (child->pack == CTK_PACK_START)
	(* callback) (child->widget, callback_data);
    }

  if (priv->center)
    (* callback) (priv->center->widget, callback_data);

  children = g_list_last (priv->children);
  while (children)
    {
      child = children->data;
      children = children->prev;

      if (child == priv->center)
        continue;

      if (child->pack == CTK_PACK_END)
	(* callback) (child->widget, callback_data);
    }
}

GList *
_ctk_box_get_children (CtkBox *box)
{
  CtkBoxPrivate *priv;
  CtkBoxChild *child;
  GList *children;
  GList *retval = NULL;

  g_return_val_if_fail (CTK_IS_BOX (box), NULL);

  priv = box->priv;

  children = priv->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      retval = g_list_prepend (retval, child->widget);
    }

  return g_list_reverse (retval);
}

/**
 * ctk_box_set_center_widget:
 * @box: a #CtkBox
 * @widget: (allow-none): the widget to center
 *
 * Sets a center widget; that is a child widget that will be
 * centered with respect to the full width of the box, even
 * if the children at either side take up different amounts
 * of space.
 *
 * Since: 3.12
 */
void
ctk_box_set_center_widget (CtkBox    *box,
                           CtkWidget *widget)
{
  CtkBoxPrivate *priv = box->priv;
  CtkWidget *old_center = NULL;

  g_return_if_fail (CTK_IS_BOX (box));

  if (priv->center)
    {
      old_center = g_object_ref (priv->center->widget);
      ctk_box_remove (CTK_CONTAINER (box), priv->center->widget);
      priv->center = NULL;
    }

  if (widget)
    priv->center = ctk_box_pack (box, widget, FALSE, TRUE, 0, CTK_PACK_START);

  if (old_center)
    g_object_unref (old_center);
}

/**
 * ctk_box_get_center_widget:
 * @box: a #CtkBox
 *
 * Retrieves the center widget of the box.
 *
 * Returns: (transfer none) (nullable): the center widget
 *   or %NULL in case no center widget is set.
 *
 * Since: 3.12
 */
CtkWidget *
ctk_box_get_center_widget (CtkBox *box)
{
  CtkBoxPrivate *priv = box->priv;

  g_return_val_if_fail (CTK_IS_BOX (box), NULL);

  if (priv->center)
    return priv->center->widget;

  return NULL;
}
