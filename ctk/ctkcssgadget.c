/*
 * Copyright © 2015 Red Hat Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#include "config.h"

#include "ctkcssgadgetprivate.h"

#include <math.h>

#include "ctkcssnumbervalueprivate.h"
#include "ctkcssshadowsvalueprivate.h"
#include "ctkcssstyleprivate.h"
#include "ctkcssstylepropertyprivate.h"
#include "ctkcsswidgetnodeprivate.h"
#include "ctkrenderbackgroundprivate.h"
#include "ctkrenderborderprivate.h"
#include "ctkdebug.h"
#include "ctkprivate.h"

/*
 * Gadgets are 'next-generation widgets' - they combine a CSS node
 * for style matching with geometry management and drawing. Each gadget
 * corresponds to 'CSS box'. Compared to traditional widgets, they are more
 * like building blocks - a typical CTK+ widget will have multiple gadgets,
 * for example a check button has its main gadget, and sub-gadgets for
 * the checkmark and the text.
 *
 * Gadgets are not themselves hierarchically organized, but it is common
 * to have a 'main' gadget, which gets used by the widgets size_allocate,
 * get_preferred_width, etc. and draw callbacks, and which in turn calls out
 * to the sub-gadgets. This call tree might extend further if there are
 * sub-sub-gadgets that a allocated relative to sub-gadgets. In typical
 * situations, the callback chain will reflect the tree structure of the
 * gadgets CSS nodes.
 *
 * Geometry management - Gadgets implement much of the CSS box model for you:
 * margins, border, padding, shadows, min-width/height are all applied automatically.
 *
 * Drawing - Gadgets implement standardized CSS drawing for you: background,
 * shadows and border are drawn before any custom drawing, and the focus outline
 * is (optionally) drawn afterwards.
 *
 * Invalidation - Gadgets sit 'between' widgets and CSS nodes, and connect
 * to the nodes ::style-changed signal and trigger appropriate invalidations
 * on the widget side.
 */

typedef struct _CtkCssGadgetPrivate CtkCssGadgetPrivate;
struct _CtkCssGadgetPrivate {
  CtkCssNode    *node;
  CtkWidget     *owner;
  CtkAllocation  allocated_size;
  gint           allocated_baseline;
};

enum {
  PROP_0,
  PROP_NODE,
  PROP_OWNER,
  /* add more */
  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES];

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (CtkCssGadget, ctk_css_gadget, G_TYPE_OBJECT,
                                  G_ADD_PRIVATE (CtkCssGadget))

static void
ctk_css_gadget_real_get_preferred_size (CtkCssGadget   *gadget G_GNUC_UNUSED,
                                        CtkOrientation  orientation G_GNUC_UNUSED,
                                        gint            for_size G_GNUC_UNUSED,
                                        gint           *minimum,
                                        gint           *natural,
                                        gint           *minimum_baseline,
                                        gint           *natural_baseline)
{
  *minimum = 0;
  *natural = 0;

  if (minimum_baseline)
    *minimum_baseline = 0;
  if (natural_baseline)
    *natural_baseline = 0;
}

static void
ctk_css_gadget_real_allocate (CtkCssGadget        *gadget G_GNUC_UNUSED,
                              const CtkAllocation *allocation,
                              int                  baseline G_GNUC_UNUSED,
                              CtkAllocation       *out_clip)
{
  *out_clip = *allocation;
}

static gboolean
ctk_css_gadget_real_draw (CtkCssGadget *gadget G_GNUC_UNUSED,
                          cairo_t      *cr G_GNUC_UNUSED,
                          int           x G_GNUC_UNUSED,
                          int           y G_GNUC_UNUSED,
                          int           width G_GNUC_UNUSED,
                          int           height G_GNUC_UNUSED)
{
  return FALSE;
}

static void
ctk_css_gadget_real_style_changed (CtkCssGadget      *gadget,
                                   CtkCssStyleChange *change)
{
  if (ctk_css_style_change_affects (change, CTK_CSS_AFFECTS_SIZE))
    ctk_css_gadget_queue_resize (gadget);
  else if (ctk_css_style_change_affects (change, CTK_CSS_AFFECTS_CLIP))
    ctk_css_gadget_queue_allocate (gadget);
  else if (ctk_css_style_change_affects (change, CTK_CSS_AFFECTS_REDRAW))
    ctk_css_gadget_queue_draw (gadget);
}

static void
ctk_css_gadget_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  CtkCssGadgetPrivate *priv = ctk_css_gadget_get_instance_private (CTK_CSS_GADGET (object));

  switch (property_id)
  {
    case PROP_NODE:
      g_value_set_object (value, priv->node);
      break;

    case PROP_OWNER:
      g_value_set_object (value, priv->owner);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
ctk_css_gadget_node_style_changed_cb (CtkCssNode        *node G_GNUC_UNUSED,
                                      CtkCssStyleChange *change,
                                      CtkCssGadget      *gadget)
{
  CtkCssGadgetClass *klass = CTK_CSS_GADGET_GET_CLASS (gadget);

  klass->style_changed (gadget, change);
}

static gboolean
ctk_css_gadget_should_connect_style_changed (CtkCssNode *node)
{
  /* Delegate to WidgetClass->style_changed */
  if (CTK_IS_CSS_WIDGET_NODE (node))
    return FALSE;

  return TRUE;
}

static void
ctk_css_gadget_unset_node (CtkCssGadget *gadget)
{
  CtkCssGadgetPrivate *priv = ctk_css_gadget_get_instance_private (gadget);

  if (priv->node)
    {
      if (ctk_css_gadget_should_connect_style_changed (priv->node))
        {
        if (g_signal_handlers_disconnect_by_func (priv->node, ctk_css_gadget_node_style_changed_cb, gadget) != 1)
          {
            g_assert_not_reached ();
          }
        }
      g_object_unref (priv->node);
      priv->node = NULL;
    }
}

void
ctk_css_gadget_set_node (CtkCssGadget *gadget,
                         CtkCssNode   *node)
{
  CtkCssGadgetPrivate *priv = ctk_css_gadget_get_instance_private (gadget);

  ctk_css_gadget_unset_node (gadget);

  if (node != NULL)
    priv->node = g_object_ref (node);
  else
    priv->node = ctk_css_node_new ();

  if (ctk_css_gadget_should_connect_style_changed (priv->node))
    {
      g_signal_connect_after (priv->node,
                              "style-changed",
                              G_CALLBACK (ctk_css_gadget_node_style_changed_cb),
                              gadget);
    }
}

static void
ctk_css_gadget_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  CtkCssGadget *gadget = CTK_CSS_GADGET (object);
  CtkCssGadgetPrivate *priv = ctk_css_gadget_get_instance_private (gadget);

  switch (property_id)
  {
    case PROP_NODE:
      ctk_css_gadget_set_node (gadget, g_value_get_object (value));
      break;

    case PROP_OWNER:
      priv->owner = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
ctk_css_gadget_finalize (GObject *object)
{
  CtkCssGadget *gadget = CTK_CSS_GADGET (object);

  ctk_css_gadget_unset_node (gadget);

  G_OBJECT_CLASS (ctk_css_gadget_parent_class)->finalize (object);
}

static void
ctk_css_gadget_class_init (CtkCssGadgetClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = ctk_css_gadget_get_property;
  object_class->set_property = ctk_css_gadget_set_property;
  object_class->finalize = ctk_css_gadget_finalize;

  klass->get_preferred_size = ctk_css_gadget_real_get_preferred_size;
  klass->allocate = ctk_css_gadget_real_allocate;
  klass->draw = ctk_css_gadget_real_draw;
  klass->style_changed = ctk_css_gadget_real_style_changed;

  properties[PROP_NODE] = g_param_spec_object ("node", "Node",
                                               "CSS node",
                                               CTK_TYPE_CSS_NODE,
                                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                                               G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  properties[PROP_OWNER] = g_param_spec_object ("owner", "Owner",
                                                "Widget that created and owns this gadget",
                                                CTK_TYPE_WIDGET,
                                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                                                G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);


  g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);
}

static void
ctk_css_gadget_init (CtkCssGadget *gadget)
{
  CtkCssGadgetPrivate *priv = ctk_css_gadget_get_instance_private (gadget);

  priv->allocated_size.width = -1;
  priv->allocated_size.height = -1;
  priv->allocated_baseline = -1;
}

/**
 * ctk_css_gadget_get_node:
 * @gadget: a #CtkCssGadget
 *
 * Get the CSS node for this gadget.
 *
  * Returns: (transfer none):  the CSS node
 */
CtkCssNode *
ctk_css_gadget_get_node (CtkCssGadget *gadget)
{
  CtkCssGadgetPrivate *priv = ctk_css_gadget_get_instance_private (gadget);

  return priv->node;
}

/**
 * ctk_css_gadget_get_style:
 * @gadget: a #CtkCssGadget
 *
 * Get the CSS style for this gadget.
 *
 * Returns: (transfer none):  the CSS style
 */
CtkCssStyle *
ctk_css_gadget_get_style (CtkCssGadget *gadget)
{
  CtkCssGadgetPrivate *priv = ctk_css_gadget_get_instance_private (gadget);

  return ctk_css_node_get_style (priv->node);
}

/**
 * ctk_css_gadget_get_owner:
 * @gadget: a #CtkCssGadget
 *
 * Get the widget to which this gadget belongs.
 *
 * Returns: (transfer none):  the widget to which @gadget belongs
 */
CtkWidget *
ctk_css_gadget_get_owner (CtkCssGadget *gadget)
{
  CtkCssGadgetPrivate *priv = ctk_css_gadget_get_instance_private (gadget);

  return priv->owner;
}

void
ctk_css_gadget_set_visible (CtkCssGadget *gadget,
                            gboolean      visible)
{
  CtkCssGadgetPrivate *priv = ctk_css_gadget_get_instance_private (gadget);

  ctk_css_node_set_visible (priv->node, visible);
}

gboolean
ctk_css_gadget_get_visible (CtkCssGadget *gadget)
{
  CtkCssGadgetPrivate *priv = ctk_css_gadget_get_instance_private (gadget);

  return ctk_css_node_get_visible (priv->node);
}

/**
 * ctk_css_gadget_add_class:
 * @gadget: a #CtkCssGadget
 * @name: class name to use in CSS matching
 *
 * Adds a style class to the gadgets CSS node.
 */
void
ctk_css_gadget_add_class (CtkCssGadget *gadget,
                          const char   *name)
{
  CtkCssGadgetPrivate *priv = ctk_css_gadget_get_instance_private (gadget);
  GQuark quark;

  quark = g_quark_from_string (name);

  ctk_css_node_add_class (priv->node, quark);
}

/**
 * ctk_css_gadget_remove_class:
 * @gadget: a #CtkCssGadget
 * @name: class name
 *
 * Removes a style class from the gadgets CSS node.
 */
void
ctk_css_gadget_remove_class (CtkCssGadget *gadget,
                             const char   *name)
{
  CtkCssGadgetPrivate *priv = ctk_css_gadget_get_instance_private (gadget);
  GQuark quark;

  quark = g_quark_try_string (name);
  if (quark == 0)
    return;

  ctk_css_node_remove_class (priv->node, quark);
}

/**
 * ctk_css_gadget_set_state:
 * @gadget: a #CtkCssGadget
 * @state: The new state
 *
 * Sets the state of the gadget's CSS node.
 */
void
ctk_css_gadget_set_state (CtkCssGadget  *gadget,
                          CtkStateFlags  state)
{
  CtkCssGadgetPrivate *priv = ctk_css_gadget_get_instance_private (gadget);

  ctk_css_node_set_state (priv->node, state);
}

/**
 * ctk_css_gadget_add_state:
 * @gadget: a #CtkCssGadget
 * @state: The state to add
 *
 * Adds the given states to the states of gadget's CSS node. Other states
 * will be kept as they are.
 */
void
ctk_css_gadget_add_state (CtkCssGadget  *gadget,
                          CtkStateFlags  state)
{
  CtkCssGadgetPrivate *priv = ctk_css_gadget_get_instance_private (gadget);

  ctk_css_node_set_state (priv->node, ctk_css_node_get_state (priv->node) | state);
}

/**
 * ctk_css_gadget_remove_state:
 * @gadget: a #CtkCssGadget
 * @state: The state to remove
 *
 * Adds the given states to the states of gadget's CSS node. Other states
 * will be kept as they are.
 */
void
ctk_css_gadget_remove_state (CtkCssGadget  *gadget,
                             CtkStateFlags  state)
{
  CtkCssGadgetPrivate *priv = ctk_css_gadget_get_instance_private (gadget);

  ctk_css_node_set_state (priv->node, ctk_css_node_get_state (priv->node) & ~state);
}

static gint
get_number (CtkCssStyle *style,
            guint        property)
{
  double d = _ctk_css_number_value_get (ctk_css_style_get_value (style, property), 100);

  if (d < 1)
    return ceil (d);
  else
    return floor (d);
}

/* Special-case min-width|height to round upwards, to avoid underalloc by 1px */
static int
get_number_ceil (CtkCssStyle *style,
                 guint        property)
{
  return ceil (_ctk_css_number_value_get (ctk_css_style_get_value (style, property), 100));
}

static void
get_box_margin (CtkCssStyle *style,
                CtkBorder   *margin)
{
  margin->top = get_number (style, CTK_CSS_PROPERTY_MARGIN_TOP);
  margin->left = get_number (style, CTK_CSS_PROPERTY_MARGIN_LEFT);
  margin->bottom = get_number (style, CTK_CSS_PROPERTY_MARGIN_BOTTOM);
  margin->right = get_number (style, CTK_CSS_PROPERTY_MARGIN_RIGHT);
}

static void
get_box_border (CtkCssStyle *style,
                CtkBorder   *border)
{
  border->top = get_number (style, CTK_CSS_PROPERTY_BORDER_TOP_WIDTH);
  border->left = get_number (style, CTK_CSS_PROPERTY_BORDER_LEFT_WIDTH);
  border->bottom = get_number (style, CTK_CSS_PROPERTY_BORDER_BOTTOM_WIDTH);
  border->right = get_number (style, CTK_CSS_PROPERTY_BORDER_RIGHT_WIDTH);
}

static void
get_box_padding (CtkCssStyle *style,
                 CtkBorder   *border)
{
  border->top = get_number (style, CTK_CSS_PROPERTY_PADDING_TOP);
  border->left = get_number (style, CTK_CSS_PROPERTY_PADDING_LEFT);
  border->bottom = get_number (style, CTK_CSS_PROPERTY_PADDING_BOTTOM);
  border->right = get_number (style, CTK_CSS_PROPERTY_PADDING_RIGHT);
}

static gboolean
allocation_contains_point (CtkAllocation *allocation,
                           gint           x,
                           gint           y)
{
  return (x >= allocation->x) &&
    (x < allocation->x + allocation->width) &&
    (y >= allocation->y) &&
    (y < allocation->y + allocation->height);
}

static void
shift_allocation (CtkCssGadget  *gadget,
                  CtkAllocation *allocation)
{
  CtkCssGadgetPrivate *priv = ctk_css_gadget_get_instance_private (gadget);

  if (priv->owner && !ctk_widget_get_has_window (priv->owner))
    {
      CtkAllocation widget_alloc;
      ctk_widget_get_allocation (priv->owner, &widget_alloc);
      allocation->x -= widget_alloc.x;
      allocation->y -= widget_alloc.y;
    }
}

/**
 * ctk_css_gadget_margin_box_contains_point:
 * @gadget: the #CtkCssGadget being tested
 * @x: X coordinate of the testing point
 * @y: Y coordinate of the testing point
 *
 * Checks whether the point at the provided coordinates is contained within the
 *   margin box of the gadget. The (X, Y) are relative to the gadget
 *   origin.
 *
 * Returns: %TRUE if the point at (X, Y) is contained within the margin
 *   box of the @gadget.
 */
gboolean
ctk_css_gadget_margin_box_contains_point (CtkCssGadget *gadget,
                                          int           x,
                                          int           y)
{
  CtkAllocation margin_box = { 0, };
  ctk_css_gadget_get_margin_box (gadget, &margin_box);
  return allocation_contains_point (&margin_box, x, y);
}

/**
 * ctk_css_gadget_border_box_contains_point:
 * @gadget: the #CtkCssGadget being tested
 * @x: X coordinate of the testing point
 * @y: Y coordinate of the testing point
 *
 * Checks whether the point at the provided coordinates is contained within the
 *   border box of the gadget. The (X, Y) are relative to the gadget
 *   origin.
 *
 * Returns: %TRUE if the point at (X, Y) is contained within the border
 *   box of the @gadget.
 */
gboolean
ctk_css_gadget_border_box_contains_point (CtkCssGadget *gadget,
                                          int           x,
                                          int           y)
{
  CtkAllocation border_box;
  ctk_css_gadget_get_border_box (gadget, &border_box);
  return allocation_contains_point (&border_box, x, y);
}

/**
 * ctk_css_gadget_content_box_contains_point:
 * @gadget: the #CtkCssGadget being tested
 * @x: X coordinate of the testing point
 * @y: Y coordinate of the testing point
 *
 * Checks whether the point at the provided coordinates is contained within the
 *   content box of the gadget. The (X, Y) are relative to the gadget
 *   origin.
 *
 * Returns: %TRUE if the point at (X, Y) is contained within the content
 *   box of the @gadget.
 */
gboolean
ctk_css_gadget_content_box_contains_point (CtkCssGadget *gadget,
                                           int           x,
                                           int           y)
{
  CtkAllocation content_box;
  ctk_css_gadget_get_content_box (gadget, &content_box);
  return allocation_contains_point (&content_box, x, y);
}

/**
 * ctk_css_gadget_get_preferred_size:
 * @gadget: the #CtkCssGadget whose size is requested
 * @orientation: whether a width (ie horizontal) or height (ie vertical) size is requested
 * @for_size: the available size in the opposite direction, or -1
 * @minimum: (nullable): return location for the minimum size
 * @natural: (nullable): return location for the natural size
 * @minimum_baseline: (nullable): return location for the baseline at minimum size
 * @natural_baseline: (nullable): return location for the baseline at natural size
 *
 * Gets the gadgets minimum and natural size (and, optionally, baseline)
 * in the given orientation for the specified size in the opposite direction.
 *
 * The returned values include CSS padding, border and margin in addition to the
 * gadgets content size, and respect the CSS min-with or min-height properties.
 *
 * The @for_size is assumed to include CSS padding, border and margins as well.
 */
void
ctk_css_gadget_get_preferred_size (CtkCssGadget   *gadget,
                                   CtkOrientation  orientation,
                                   gint            for_size,
                                   gint           *minimum,
                                   gint           *natural,
                                   gint           *minimum_baseline,
                                   gint           *natural_baseline)
{
  CtkCssStyle *style;
  CtkBorder margin, border, padding;
  int min_size, extra_size, extra_opposite, extra_baseline;
  int unused_minimum, unused_natural;
  int forced_minimum, forced_natural;
  int min_for_size;

  if (minimum == NULL)
    minimum = &unused_minimum;
  if (natural == NULL)
    natural = &unused_natural;

  if (!ctk_css_gadget_get_visible (gadget))
    {
      *minimum = 0;
      *natural = 0;
      if (minimum_baseline)
        *minimum_baseline = -1;
      if (natural_baseline)
        *natural_baseline = -1;
      return;
    }

  style = ctk_css_gadget_get_style (gadget);
  get_box_margin (style, &margin);
  get_box_border (style, &border);
  get_box_padding (style, &padding);
  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      extra_size = margin.left + margin.right + border.left + border.right + padding.left + padding.right;
      extra_opposite = margin.top + margin.bottom + border.top + border.bottom + padding.top + padding.bottom;
      extra_baseline = margin.left + border.left + padding.left;
      min_size = get_number_ceil (style, CTK_CSS_PROPERTY_MIN_WIDTH);
      min_for_size = get_number_ceil (style, CTK_CSS_PROPERTY_MIN_HEIGHT);
    }
  else
    {
      extra_size = margin.top + margin.bottom + border.top + border.bottom + padding.top + padding.bottom;
      extra_opposite = margin.left + margin.right + border.left + border.right + padding.left + padding.right;
      extra_baseline = margin.top + border.top + padding.top;
      min_size = get_number_ceil (style, CTK_CSS_PROPERTY_MIN_HEIGHT);
      min_for_size = get_number_ceil (style, CTK_CSS_PROPERTY_MIN_WIDTH);
    }

  if (for_size > -1)
    {
      if (for_size < min_for_size)
        g_warning ("for_size smaller than min-size (%d < %d) "
                   "while measuring gadget (node %s, owner %s)",
                   for_size, min_for_size,
                   ctk_css_node_get_name (ctk_css_gadget_get_node (gadget)),
                   G_OBJECT_TYPE_NAME (ctk_css_gadget_get_owner (gadget)));

      for_size = MAX (0, for_size - extra_opposite);
    }

  if (minimum_baseline)
    *minimum_baseline = -1;
  if (natural_baseline)
    *natural_baseline = -1;

  CTK_CSS_GADGET_GET_CLASS (gadget)->get_preferred_size (gadget,
                                                         orientation,
                                                         for_size,
                                                         minimum, natural,
                                                         minimum_baseline, natural_baseline);

  g_warn_if_fail (*minimum <= *natural);

  forced_minimum = MAX (*minimum, min_size);
  forced_natural = MAX (*natural, min_size);

  if (minimum_baseline && *minimum_baseline > -1)
    {
      *minimum_baseline += 0.5 * (forced_minimum - *minimum);
      *minimum_baseline = MAX (0, *minimum_baseline + extra_baseline);
    }
  if (natural_baseline && *natural_baseline > -1)
    {
      *natural_baseline += 0.5 * (forced_natural - *natural);
      *natural_baseline = MAX (0, *natural_baseline + extra_baseline);
    }

  *minimum = MAX (0, forced_minimum + extra_size);
  *natural = MAX (0, forced_natural + extra_size);

}

/**
 * ctk_css_gadget_allocate:
 * @gadget: the #CtkCssGadget to allocate
 * @allocation: the allocation
 * @baseline: the baseline for the allocation
 * @out_clip: (out): return location for the gadgets clip region
 *
 * Allocates the gadget.
 *
 * The @allocation is assumed to include CSS padding, border and margin.
 * The gadget content will be allocated a smaller area that excludes these.
 * The @out_clip includes the shadow extents of the gadget in addition to
 * any content clip.
 */
void
ctk_css_gadget_allocate (CtkCssGadget        *gadget,
                         const CtkAllocation *allocation,
                         int                  baseline,
                         CtkAllocation       *out_clip)
{
  CtkCssGadgetPrivate *priv = ctk_css_gadget_get_instance_private (gadget);
  CtkAllocation content_allocation;
  CtkAllocation tmp_clip;
  CtkAllocation content_clip = { 0, 0, 0, 0 };
  CtkBorder margin, border, padding, shadow, extents;
  CtkCssStyle *style;

  g_return_if_fail (out_clip != NULL);

  if (!ctk_css_gadget_get_visible (gadget))
    {
      out_clip->x = 0;
      out_clip->y = 0;
      out_clip->width = 0;
      out_clip->height = 0;
      return;
    }

  priv->allocated_size = *allocation;
  priv->allocated_baseline = baseline;

  style = ctk_css_gadget_get_style (gadget);
  get_box_margin (style, &margin);
  get_box_border (style, &border);
  get_box_padding (style, &padding);
  extents.top = margin.top + border.top + padding.top;
  extents.right = margin.right + border.right + padding.right;
  extents.bottom = margin.bottom + border.bottom + padding.bottom;
  extents.left = margin.left + border.left + padding.left;

  content_allocation.x = allocation->x + extents.left;
  content_allocation.y = allocation->y + extents.top;
  content_allocation.width = allocation->width - extents.left - extents.right;
  content_allocation.height = allocation->height - extents.top - extents.bottom;

  if (baseline >= 0)
    baseline -= extents.top;

  if (content_allocation.width < 0)
    {
      g_warning ("Negative content width %d (allocation %d, extents %dx%d) "
                 "while allocating gadget (node %s, owner %s)",
                 content_allocation.width, allocation->width,
                 extents.left, extents.right,
                 ctk_css_node_get_name (ctk_css_gadget_get_node (gadget)),
                 G_OBJECT_TYPE_NAME (ctk_css_gadget_get_owner (gadget)));
      content_allocation.width = 0;
    }

  if (content_allocation.height < 0)
    {
      g_warning ("Negative content height %d (allocation %d, extents %dx%d) "
                 "while allocating gadget (node %s, owner %s)",
                 content_allocation.height, allocation->height,
                 extents.top, extents.bottom,
                 ctk_css_node_get_name (ctk_css_gadget_get_node (gadget)),
                 G_OBJECT_TYPE_NAME (ctk_css_gadget_get_owner (gadget)));
      content_allocation.height = 0;
    }

  CTK_CSS_GADGET_GET_CLASS (gadget)->allocate (gadget, &content_allocation, baseline, &content_clip);

  _ctk_css_shadows_value_get_extents (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_BOX_SHADOW), &shadow);

  out_clip->x = allocation->x + margin.left - shadow.left;
  out_clip->y = allocation->y + margin.top - shadow.top;
  out_clip->width = MAX (0, allocation->width - margin.left - margin.right + shadow.left + shadow.right);
  out_clip->height = MAX (0, allocation->height - margin.top - margin.bottom + shadow.top + shadow.bottom);

  if (content_clip.width > 0 && content_clip.height > 0)
    cdk_rectangle_union (&content_clip, out_clip, out_clip);

  if (ctk_css_style_render_outline_get_clip (style,
                                             allocation->x + margin.left,
                                             allocation->y + margin.top,
                                             allocation->width - margin.left - margin.right,
                                             allocation->height - margin.top - margin.bottom,
                                             &tmp_clip))
    cdk_rectangle_union (&tmp_clip, out_clip, out_clip);
}

/**
 * ctk_css_gadget_draw:
 * @gadget: The gadget to draw
 * @cr: The cairo context to draw to
 *
 * Will draw the gadget at the position allocated via
 * ctk_css_gadget_allocate(). It is your responsibility to make
 * sure that those 2 coordinate systems match.
 *
 * The drawing virtual function will be passed an untransformed @cr.
 * This is important because functions like
 * ctk_container_propagate_draw() depend on that.
 */
void
ctk_css_gadget_draw (CtkCssGadget *gadget,
                     cairo_t      *cr)
{
  CtkCssGadgetPrivate *priv = ctk_css_gadget_get_instance_private (gadget);
  CtkBorder margin, border, padding;
  gboolean draw_focus = FALSE;
  CtkCssStyle *style;
  int x, y, width, height;
  int contents_x, contents_y, contents_width, contents_height;
  CtkAllocation margin_box;

  if (!ctk_css_gadget_get_visible (gadget))
    return;

  ctk_css_gadget_get_margin_box (gadget, &margin_box);

  x = margin_box.x;
  y = margin_box.y;
  width = margin_box.width;
  height = margin_box.height;

  if (width < 0 || height < 0)
    {
      g_warning ("Drawing a gadget with negative dimensions. "
                 "Did you forget to allocate a size? (node %s owner %s)",
                 ctk_css_node_get_name (ctk_css_gadget_get_node (gadget)),
                 G_OBJECT_TYPE_NAME (ctk_css_gadget_get_owner (gadget)));
      x = 0;
      y = 0;
      width = ctk_widget_get_allocated_width (priv->owner);
      height = ctk_widget_get_allocated_height (priv->owner);
    }

  style = ctk_css_gadget_get_style (gadget);
  get_box_margin (style, &margin);
  get_box_border (style, &border);
  get_box_padding (style, &padding);

  ctk_css_style_render_background (style,
                                   cr,
                                   x + margin.left,
                                   y + margin.top,
                                   width - margin.left - margin.right,
                                   height - margin.top - margin.bottom,
                                   ctk_css_node_get_junction_sides (priv->node));
  ctk_css_style_render_border (style,
                               cr,
                               x + margin.left,
                               y + margin.top,
                               width - margin.left - margin.right,
                               height - margin.top - margin.bottom,
                               0,
                               ctk_css_node_get_junction_sides (priv->node));

  contents_x = x + margin.left + border.left + padding.left;
  contents_y = y + margin.top + border.top + padding.top;
  contents_width = width - margin.left - margin.right - border.left - border.right - padding.left - padding.right;
  contents_height = height - margin.top - margin.bottom - border.top - border.bottom - padding.top - padding.bottom;

  if (contents_width > 0 && contents_height > 0)
    draw_focus = CTK_CSS_GADGET_GET_CLASS (gadget)->draw (gadget,
                                                          cr,
                                                          contents_x, contents_y,
                                                          contents_width, contents_height);

  if (draw_focus)
    ctk_css_style_render_outline (style,
                                  cr,
                                  x + margin.left,
                                  y + margin.top,
                                  width - margin.left - margin.right,
                                  height - margin.top - margin.bottom);

#ifdef G_ENABLE_DEBUG
  {
    CdkDisplay *display = ctk_widget_get_display (ctk_css_gadget_get_owner (gadget));
    CtkDebugFlag flags = ctk_get_display_debug_flags (display);
    if G_UNLIKELY (flags & CTK_DEBUG_LAYOUT)
      {
        cairo_save (cr);
        cairo_new_path (cr);
        cairo_rectangle (cr,
                         x + margin.left,
                         y + margin.top,
                         width - margin.left - margin.right,
                         height - margin.top - margin.bottom);
        cairo_set_line_width (cr, 1.0);
        cairo_set_source_rgba (cr, 0, 0, 1.0, 0.33);
        cairo_stroke (cr);
        cairo_rectangle (cr,
                         contents_x,
                         contents_y,
                         contents_width,
                         contents_height);
        cairo_set_line_width (cr, 1.0);
        cairo_set_source_rgba (cr, 1.0, 0, 1.0, 0.33);
        cairo_stroke (cr);
        cairo_restore (cr);
      }
    if G_UNLIKELY (flags & CTK_DEBUG_BASELINES)
      {
        int baseline = priv->allocated_baseline;

        if (baseline != -1)
          {
            if (priv->owner && !ctk_widget_get_has_window (priv->owner))
              {
                CtkAllocation widget_alloc;
                ctk_widget_get_allocation (priv->owner, &widget_alloc);
                baseline -= widget_alloc.y;
              }
            cairo_save (cr);
            cairo_new_path (cr);
            cairo_move_to (cr, x + margin.left, baseline + 0.5);
            cairo_rel_line_to (cr, width - margin.left - margin.right, 0);
            cairo_set_line_width (cr, 1.0);
            cairo_set_source_rgba (cr, 1.0, 0, 0.25, 0.25);
            cairo_stroke (cr);
            cairo_restore (cr);
          }
      }
  }
#endif
}

void
ctk_css_gadget_queue_resize (CtkCssGadget *gadget)
{
  g_return_if_fail (CTK_IS_CSS_GADGET (gadget));

  ctk_widget_queue_resize (ctk_css_gadget_get_owner (gadget));
}

void
ctk_css_gadget_queue_allocate (CtkCssGadget *gadget)
{
  g_return_if_fail (CTK_IS_CSS_GADGET (gadget));

  ctk_widget_queue_allocate (ctk_css_gadget_get_owner (gadget));
}

void
ctk_css_gadget_queue_draw (CtkCssGadget *gadget)
{
  g_return_if_fail (CTK_IS_CSS_GADGET (gadget));
  
  /* XXX: Only invalidate clip here */
  ctk_widget_queue_draw (ctk_css_gadget_get_owner (gadget));
}

/**
 * ctk_css_gadget_get_margin_box:
 * @gadget: a #CtkCssGadget
 * @box: (out): Return location for gadget's the margin box
 *
 * Returns the margin box of the gadget. The box coordinates are relative to
 *   the gadget origin. Compare with ctk_css_gadget_get_margin_allocation(),
 *   which returns the margin box in the widget allocation coordinates.
 */
void
ctk_css_gadget_get_margin_box (CtkCssGadget  *gadget,
                               CtkAllocation *box)
{
  ctk_css_gadget_get_margin_allocation (gadget, box, NULL);
  shift_allocation (gadget, box);
}

/**
 * ctk_css_gadget_get_border_box:
 * @gadget: a #CtkCssGadget
 * @box: (out): Return location for gadget's the border box
 *
 * Returns the border box of the gadget. The box coordinates are relative to
 *   the gadget origin. Compare with ctk_css_gadget_get_border_allocation(),
 *   which returns the border box in the widget allocation coordinates.
 */
void
ctk_css_gadget_get_border_box (CtkCssGadget  *gadget,
                               CtkAllocation *box)
{
  ctk_css_gadget_get_border_allocation (gadget, box, NULL);
  shift_allocation (gadget, box);
}

/**
 * ctk_css_gadget_get_content_box:
 * @gadget: a #CtkCssGadget
 * @box: (out): Return location for gadget's the content box
 *
 * Returns the content box of the gadget. The box coordinates are relative to
 *   the gadget origin. Compare with ctk_css_gadget_get_content_allocation(),
 *   which returns the content box in the widget allocation coordinates.
 */
void
ctk_css_gadget_get_content_box (CtkCssGadget  *gadget,
                                CtkAllocation *box)
{
  ctk_css_gadget_get_content_allocation (gadget, box, NULL);
  shift_allocation (gadget, box);
}

void
ctk_css_gadget_get_margin_allocation (CtkCssGadget  *gadget,
                                      CtkAllocation *allocation,
                                      int           *baseline)
{
  CtkCssGadgetPrivate *priv = ctk_css_gadget_get_instance_private (gadget);

  g_return_if_fail (CTK_IS_CSS_GADGET (gadget));

  if (!ctk_css_gadget_get_visible (gadget))
    {
      if (allocation)
        allocation->x = allocation->y = allocation->width = allocation->height = 0;
      if (baseline)
        *baseline = -1;
      return;
    }

  if (allocation)
    *allocation = priv->allocated_size;
  if (baseline)
    *baseline = priv->allocated_baseline;
}

void
ctk_css_gadget_get_border_allocation (CtkCssGadget  *gadget,
                                      CtkAllocation *allocation,
                                      int           *baseline)
{
  CtkCssGadgetPrivate *priv = ctk_css_gadget_get_instance_private (gadget);
  CtkBorder margin;

  g_return_if_fail (CTK_IS_CSS_GADGET (gadget));

  if (!ctk_css_gadget_get_visible (gadget))
    {
      if (allocation)
        allocation->x = allocation->y = allocation->width = allocation->height = 0;
      if (baseline)
        *baseline = -1;
      return;
    }

  get_box_margin (ctk_css_gadget_get_style (gadget), &margin);

  if (allocation)
    {
      allocation->x = priv->allocated_size.x + margin.left;
      allocation->y = priv->allocated_size.y + margin.top;
      allocation->width = MAX (0, priv->allocated_size.width - margin.left - margin.right);
      allocation->height = MAX (0, priv->allocated_size.height - margin.top - margin.bottom);
    }
  if (baseline)
    {
      if (priv->allocated_baseline >= 0)
        *baseline = priv->allocated_baseline - margin.top;
      else
        *baseline = -1;
    }
}

void
ctk_css_gadget_get_content_allocation (CtkCssGadget  *gadget,
                                       CtkAllocation *allocation,
                                       int           *baseline)
{
  CtkCssGadgetPrivate *priv = ctk_css_gadget_get_instance_private (gadget);
  CtkBorder margin, border, padding, extents;
  CtkCssStyle *style;

  g_return_if_fail (CTK_IS_CSS_GADGET (gadget));

  if (!ctk_css_gadget_get_visible (gadget))
    {
      if (allocation)
        allocation->x = allocation->y = allocation->width = allocation->height = 0;
      if (baseline)
        *baseline = -1;
      return;
    }

  style = ctk_css_gadget_get_style (gadget);
  get_box_margin (style, &margin);
  get_box_border (style, &border);
  get_box_padding (style, &padding);
  extents.top = margin.top + border.top + padding.top;
  extents.right = margin.right + border.right + padding.right;
  extents.bottom = margin.bottom + border.bottom + padding.bottom;
  extents.left = margin.left + border.left + padding.left;

  if (allocation)
    {
      allocation->x = priv->allocated_size.x + extents.left;
      allocation->y = priv->allocated_size.y + extents.top;
      allocation->width = MAX (0, priv->allocated_size.width - extents.left - extents.right);
      allocation->height = MAX (0, priv->allocated_size.height - extents.top - extents.bottom);
    }

  if (baseline)
    {
      if (priv->allocated_baseline >= 0)
        *baseline = priv->allocated_baseline - extents.top;
      else
        *baseline = -1;
    }
}
