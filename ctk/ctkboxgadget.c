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

#include "ctkboxgadgetprivate.h"

#include "ctkcssnodeprivate.h"
#include "ctkmain.h"
#include "ctkprivate.h"
#include "ctksizerequest.h"
#include "ctkwidgetprivate.h"

/* CtkBoxGadget is a container gadget implementation that arranges its
 * children in a row, either horizontally or vertically. Children can
 * be either widgets or gadgets, and can be set to expand horizontally
 * or vertically, or both.
 */

typedef struct _CtkBoxGadgetPrivate CtkBoxGadgetPrivate;
struct _CtkBoxGadgetPrivate {
  CtkOrientation orientation;
  GArray *children;

  guint draw_focus       : 1;
  guint draw_reverse     : 1;
  guint allocate_reverse : 1;
  guint align_reverse    : 1;
};

typedef gboolean (* ComputeExpandFunc) (GObject *object, CtkOrientation orientation);

typedef struct _CtkBoxGadgetChild CtkBoxGadgetChild;
struct _CtkBoxGadgetChild {
  GObject *object;
  gboolean expand;
  CtkAlign align;
};

G_DEFINE_TYPE_WITH_CODE (CtkBoxGadget, ctk_box_gadget, CTK_TYPE_CSS_GADGET,
                         G_ADD_PRIVATE (CtkBoxGadget))

static gboolean
ctk_box_gadget_child_is_visible (GObject *child)
{
  if (CTK_IS_WIDGET (child))
    return ctk_widget_get_visible (CTK_WIDGET (child));
  else
    return ctk_css_gadget_get_visible (CTK_CSS_GADGET (child));
}

static gboolean
ctk_box_gadget_child_compute_expand (CtkBoxGadget      *gadget,
                                     CtkBoxGadgetChild *child)
{
  CtkBoxGadgetPrivate *priv = ctk_box_gadget_get_instance_private (CTK_BOX_GADGET (gadget));

  if (child->expand)
    return TRUE;

  if (CTK_IS_WIDGET (child->object))
    return ctk_widget_compute_expand (CTK_WIDGET (child->object), priv->orientation);

  return FALSE;
}

static CtkAlign
ctk_box_gadget_child_get_align (CtkBoxGadget      *gadget,
                                CtkBoxGadgetChild *child)
{
  CtkBoxGadgetPrivate *priv = ctk_box_gadget_get_instance_private (CTK_BOX_GADGET (gadget));
  CtkAlign align;

  if (CTK_IS_WIDGET (child->object))
    {
      if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
        g_object_get (child->object, "valign", &align, NULL);
      else
        g_object_get (child->object, "halign", &align, NULL);
    }
  else
    align = child->align;

  return align;
}

static CtkAlign
effective_align (CtkAlign align,
                 gboolean reverse)
{
  switch (align)
    {
    case CTK_ALIGN_START:
      return reverse ? CTK_ALIGN_END : CTK_ALIGN_START;
    case CTK_ALIGN_END:
      return reverse ? CTK_ALIGN_START : CTK_ALIGN_END;
    default:
      return align;
    }
}

static void
ctk_box_gadget_measure_child (GObject        *child,
                              CtkOrientation  orientation,
                              gint            for_size,
                              gint           *minimum,
                              gint           *natural,
                              gint           *minimum_baseline,
                              gint           *natural_baseline)
{
  if (CTK_IS_WIDGET (child))
    {
      _ctk_widget_get_preferred_size_for_size (CTK_WIDGET (child),
                                               orientation,
                                               for_size,
                                               minimum, natural,
                                               minimum_baseline, natural_baseline);
    }
  else
    {
      ctk_css_gadget_get_preferred_size (CTK_CSS_GADGET (child),
                                         orientation,
                                         for_size,
                                         minimum, natural,
                                         minimum_baseline, natural_baseline);
    }
}

static void
ctk_box_gadget_distribute (CtkBoxGadget     *gadget,
                           gint              for_size,
                           gint              size,
                           CtkRequestedSize *sizes)
{
  CtkBoxGadgetPrivate *priv = ctk_box_gadget_get_instance_private (CTK_BOX_GADGET (gadget));
  guint i, n_expand;

  n_expand = 0;

  for (i = 0 ; i < priv->children->len; i++)
    {
      CtkBoxGadgetChild *child = &g_array_index (priv->children, CtkBoxGadgetChild, i);

      ctk_box_gadget_measure_child (child->object,
                                    priv->orientation,
                                    for_size,
                                    &sizes[i].minimum_size, &sizes[i].natural_size,
                                    NULL, NULL);
      if (ctk_box_gadget_child_is_visible (child->object) &&
          ctk_box_gadget_child_compute_expand (gadget, child))
        n_expand++;
      size -= sizes[i].minimum_size;
    }

  if G_UNLIKELY (size < 0)
    {
      g_critical ("%s: assertion 'size >= 0' failed in %s", G_STRFUNC, G_OBJECT_TYPE_NAME (ctk_css_gadget_get_owner (CTK_CSS_GADGET (gadget))));
      return;
    }

  size = ctk_distribute_natural_allocation (size, priv->children->len, sizes);

  if (size <= 0 || n_expand == 0)
    return;

  for (i = 0 ; i < priv->children->len; i++)
    {
      CtkBoxGadgetChild *child = &g_array_index (priv->children, CtkBoxGadgetChild, i);

      if (!ctk_box_gadget_child_is_visible (child->object) ||
          !ctk_box_gadget_child_compute_expand (gadget, child))
        continue;

      sizes[i].minimum_size += size / n_expand;
      /* distribute all pixels, even if there's a remainder */
      size -= size / n_expand;
      n_expand--;
    }

}

static void
ctk_box_gadget_measure_orientation (CtkCssGadget   *gadget,
                                    CtkOrientation  orientation,
                                    gint            for_size,
                                    gint           *minimum,
                                    gint           *natural,
                                    gint           *minimum_baseline,
                                    gint           *natural_baseline)
{
  CtkBoxGadgetPrivate *priv = ctk_box_gadget_get_instance_private (CTK_BOX_GADGET (gadget));
  gint child_min, child_nat;
  guint i;

  *minimum = 0;
  *natural = 0;

  for (i = 0 ; i < priv->children->len; i++)
    {
      CtkBoxGadgetChild *child = &g_array_index (priv->children, CtkBoxGadgetChild, i);

      ctk_box_gadget_measure_child (child->object,
                                    orientation,
                                    for_size,
                                    &child_min, &child_nat,
                                    NULL, NULL);

      *minimum += child_min;
      *natural += child_nat;
    }
}

static void
ctk_box_gadget_measure_opposite (CtkCssGadget   *gadget,
                                 CtkOrientation  orientation,
                                 gint            for_size,
                                 gint           *minimum,
                                 gint           *natural,
                                 gint           *minimum_baseline,
                                 gint           *natural_baseline)
{
  CtkBoxGadgetPrivate *priv = ctk_box_gadget_get_instance_private (CTK_BOX_GADGET (gadget));
  int child_min, child_nat, child_min_baseline, child_nat_baseline;
  int total_min, above_min, below_min, total_nat, above_nat, below_nat;
  CtkRequestedSize *sizes;
  guint i;

  if (for_size >= 0)
    {
      sizes = g_newa (CtkRequestedSize, priv->children->len);
      ctk_box_gadget_distribute (CTK_BOX_GADGET (gadget), -1, for_size, sizes);
    }

  above_min = below_min = above_nat = below_nat = -1;
  total_min = total_nat = 0;

  for (i = 0 ; i < priv->children->len; i++)
    {
      CtkBoxGadgetChild *child = &g_array_index (priv->children, CtkBoxGadgetChild, i);

      ctk_box_gadget_measure_child (child->object,
                                    orientation,
                                    for_size >= 0 ? sizes[i].minimum_size : -1,
                                    &child_min, &child_nat,
                                    &child_min_baseline, &child_nat_baseline);

      if (child_min_baseline >= 0)
        {
          below_min = MAX (below_min, child_min - child_min_baseline);
          above_min = MAX (above_min, child_min_baseline);
          below_nat = MAX (below_nat, child_nat - child_nat_baseline);
          above_nat = MAX (above_nat, child_nat_baseline);
        }
      else
        {
          total_min = MAX (total_min, child_min);
          total_nat = MAX (total_nat, child_nat);
        }
    }

  if (above_min >= 0)
    {
      total_min = MAX (total_min, above_min + below_min);
      total_nat = MAX (total_nat, above_nat + below_nat);
      /* assume CTK_BASELINE_POSITION_CENTER for now */
      if (minimum_baseline)
        *minimum_baseline = above_min + (total_min - (above_min + below_min)) / 2;
      if (natural_baseline)
        *natural_baseline = above_nat + (total_nat - (above_nat + below_nat)) / 2;
    }

  *minimum = total_min;
  *natural = total_nat;
}

static void
ctk_box_gadget_get_preferred_size (CtkCssGadget   *gadget,
                                   CtkOrientation  orientation,
                                   gint            for_size,
                                   gint           *minimum,
                                   gint           *natural,
                                   gint           *minimum_baseline,
                                   gint           *natural_baseline)
{
  CtkBoxGadgetPrivate *priv = ctk_box_gadget_get_instance_private (CTK_BOX_GADGET (gadget));

  if (priv->orientation == orientation)
    ctk_box_gadget_measure_orientation (gadget, orientation, for_size, minimum, natural, minimum_baseline, natural_baseline);
  else
    ctk_box_gadget_measure_opposite (gadget, orientation, for_size, minimum, natural, minimum_baseline, natural_baseline);
}

static void
ctk_box_gadget_allocate_child (GObject        *child,
                               CtkOrientation  box_orientation,
                               CtkAlign        child_align,
                               CtkAllocation  *allocation,
                               int             baseline,
                               CtkAllocation  *out_clip)
{
  if (CTK_IS_WIDGET (child))
    {
      ctk_widget_size_allocate_with_baseline (CTK_WIDGET (child), allocation, baseline);
      ctk_widget_get_clip (CTK_WIDGET (child), out_clip);
    }
  else
    {
      CtkAllocation child_allocation;
      int minimum, natural;
      int minimum_baseline, natural_baseline;

      if (box_orientation == CTK_ORIENTATION_HORIZONTAL)
        {
          child_allocation.width = allocation->width;
          child_allocation.x = allocation->x;

          ctk_css_gadget_get_preferred_size (CTK_CSS_GADGET (child),
                                             CTK_ORIENTATION_VERTICAL,
                                             allocation->width,
                                             &minimum, &natural,
                                             &minimum_baseline, &natural_baseline);

          switch (child_align)
            {
            case CTK_ALIGN_FILL:
              child_allocation.height = allocation->height;
              child_allocation.y = allocation->y;
              break;
            case CTK_ALIGN_START:
              child_allocation.height = MIN(natural, allocation->height);
              child_allocation.y = allocation->y;
              break;
            case CTK_ALIGN_END:
              child_allocation.height = MIN(natural, allocation->height);
              child_allocation.y = allocation->y + allocation->height - child_allocation.height;
              break;
            case CTK_ALIGN_BASELINE:
              if (minimum_baseline >= 0 && baseline >= 0)
                {
                  child_allocation.height = MIN(natural, allocation->height);
                  child_allocation.y = allocation->y + MAX(0, baseline - minimum_baseline);
                  break;
                }
            case CTK_ALIGN_CENTER:
              child_allocation.height = MIN(natural, allocation->height);
              child_allocation.y = allocation->y + (allocation->height - child_allocation.height) / 2;
              break;
            default:
              g_assert_not_reached ();
            }
        }
      else
        {
          child_allocation.height = allocation->height;
          child_allocation.y = allocation->y;

          ctk_css_gadget_get_preferred_size (CTK_CSS_GADGET (child),
                                             CTK_ORIENTATION_HORIZONTAL,
                                             allocation->height,
                                             &minimum, &natural,
                                             NULL, NULL);

          switch (child_align)
            {
            case CTK_ALIGN_FILL:
              child_allocation.width = allocation->width;
              child_allocation.x = allocation->x;
              break;
            case CTK_ALIGN_START:
              child_allocation.width = MIN(natural, allocation->width);
              child_allocation.x = allocation->x;
              break;
            case CTK_ALIGN_END:
              child_allocation.width = MIN(natural, allocation->width);
              child_allocation.x = allocation->x + allocation->width - child_allocation.width;
              break;
            case CTK_ALIGN_BASELINE:
            case CTK_ALIGN_CENTER:
              child_allocation.width = MIN(natural, allocation->width);
              child_allocation.x = allocation->x + (allocation->width - child_allocation.width) / 2;
              break;
            default:
              g_assert_not_reached ();
            }
        }

      ctk_css_gadget_allocate (CTK_CSS_GADGET (child), &child_allocation, baseline, out_clip);
    }
}

static void
ctk_box_gadget_allocate (CtkCssGadget        *gadget,
                         const CtkAllocation *allocation,
                         int                  baseline,
                         CtkAllocation       *out_clip)
{
  CtkBoxGadgetPrivate *priv = ctk_box_gadget_get_instance_private (CTK_BOX_GADGET (gadget));
  CtkRequestedSize *sizes;
  CtkAllocation child_allocation, child_clip;
  CtkAlign child_align;
  guint i;

  child_allocation = *allocation;
  sizes = g_newa (CtkRequestedSize, priv->children->len);

  if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      ctk_box_gadget_distribute (CTK_BOX_GADGET (gadget), allocation->height, allocation->width, sizes);

      if (priv->allocate_reverse)
        child_allocation.x = allocation->x + allocation->width;

      for (i = 0; i < priv->children->len; i++)
        {
          guint idx = priv->allocate_reverse ? priv->children->len - 1 - i : i;
          CtkBoxGadgetChild *child = &g_array_index (priv->children, CtkBoxGadgetChild, idx);
          child_allocation.width = sizes[idx].minimum_size;
          child_allocation.height = allocation->height;
          child_allocation.y = allocation->y;
          if (priv->allocate_reverse)
            child_allocation.x -= child_allocation.width;

          child_align = ctk_box_gadget_child_get_align (CTK_BOX_GADGET (gadget), child);
          ctk_box_gadget_allocate_child (child->object,
                                         priv->orientation,
                                         effective_align (child_align, priv->align_reverse),
                                         &child_allocation,
                                         baseline,
                                         &child_clip);

          if (i == 0)
            *out_clip = child_clip;
          else
            gdk_rectangle_union (out_clip, &child_clip, out_clip);

          if (!priv->allocate_reverse)
            child_allocation.x += sizes[idx].minimum_size;
        }
    }
  else
    {
      ctk_box_gadget_distribute (CTK_BOX_GADGET (gadget), allocation->width, allocation->height, sizes);

      if (priv->allocate_reverse)
        child_allocation.y = allocation->y + allocation->height;

      for (i = 0 ; i < priv->children->len; i++)
        {
          guint idx = priv->allocate_reverse ? priv->children->len - 1 - i : i;
          CtkBoxGadgetChild *child = &g_array_index (priv->children, CtkBoxGadgetChild, idx);
          child_allocation.height = sizes[idx].minimum_size;
          child_allocation.width = allocation->width;
          child_allocation.x = allocation->x;
          if (priv->allocate_reverse)
            child_allocation.y -= child_allocation.height;

          child_align = ctk_box_gadget_child_get_align (CTK_BOX_GADGET (gadget), child);
          ctk_box_gadget_allocate_child (child->object,
                                         priv->orientation,
                                         effective_align (child_align, priv->align_reverse),
                                         &child_allocation,
                                         -1,
                                         &child_clip);

          if (i == 0)
            *out_clip = child_clip;
          else
            gdk_rectangle_union (out_clip, &child_clip, out_clip);

          if (!priv->allocate_reverse)
            child_allocation.y += sizes[idx].minimum_size;
        }
    }
}

static gboolean
ctk_box_gadget_draw (CtkCssGadget *gadget,
                     cairo_t      *cr,
                     int           x,
                     int           y,
                     int           width,
                     int           height)
{
  CtkBoxGadgetPrivate *priv = ctk_box_gadget_get_instance_private (CTK_BOX_GADGET (gadget));
  CtkWidget *owner = ctk_css_gadget_get_owner (gadget);
  guint i;

  for (i = 0; i < priv->children->len; i++)
    {
      guint draw_index = priv->draw_reverse ? priv->children->len - 1 - i : i;
      CtkBoxGadgetChild *child = &g_array_index (priv->children, CtkBoxGadgetChild, draw_index);

      if (CTK_IS_WIDGET (child->object))
        ctk_container_propagate_draw (CTK_CONTAINER (owner), CTK_WIDGET (child->object), cr);
      else
        ctk_css_gadget_draw (CTK_CSS_GADGET (child->object), cr);
    }

  if (priv->draw_focus && ctk_widget_has_visible_focus (owner))
    return TRUE;

  return FALSE;
}

static void
ctk_box_gadget_finalize (GObject *object)
{
  CtkBoxGadgetPrivate *priv = ctk_box_gadget_get_instance_private (CTK_BOX_GADGET (object));

  g_array_free (priv->children, TRUE);

  G_OBJECT_CLASS (ctk_box_gadget_parent_class)->finalize (object);
}

static void
ctk_box_gadget_class_init (CtkBoxGadgetClass *klass)
{
  CtkCssGadgetClass *gadget_class = CTK_CSS_GADGET_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = ctk_box_gadget_finalize;

  gadget_class->get_preferred_size = ctk_box_gadget_get_preferred_size;
  gadget_class->allocate = ctk_box_gadget_allocate;
  gadget_class->draw = ctk_box_gadget_draw;
}

static void
ctk_box_gadget_clear_child (gpointer data)
{
  CtkBoxGadgetChild *child = data;

  g_object_unref (child->object);
}

static void
ctk_box_gadget_init (CtkBoxGadget *gadget)
{
  CtkBoxGadgetPrivate *priv = ctk_box_gadget_get_instance_private (gadget);

  priv->children = g_array_new (FALSE, FALSE, sizeof (CtkBoxGadgetChild));
  g_array_set_clear_func (priv->children, ctk_box_gadget_clear_child);
}

CtkCssGadget *
ctk_box_gadget_new_for_node (CtkCssNode *node,
                               CtkWidget  *owner)
{
  return g_object_new (CTK_TYPE_BOX_GADGET,
                       "node", node,
                       "owner", owner,
                       NULL);
}

CtkCssGadget *
ctk_box_gadget_new (const char   *name,
                    CtkWidget    *owner,
                    CtkCssGadget *parent,
                    CtkCssGadget *next_sibling)
{
  CtkCssNode *node;
  CtkCssGadget *result;

  node = ctk_css_node_new ();
  ctk_css_node_set_name (node, g_intern_string (name));
  if (parent)
    ctk_css_node_insert_before (ctk_css_gadget_get_node (parent),
                                node,
                                next_sibling ? ctk_css_gadget_get_node (next_sibling) : NULL);

  result = ctk_box_gadget_new_for_node (node, owner);

  g_object_unref (node);

  return result;
}

void
ctk_box_gadget_set_orientation (CtkBoxGadget   *gadget,
                                CtkOrientation  orientation)
{
  CtkBoxGadgetPrivate *priv = ctk_box_gadget_get_instance_private (gadget);

  priv->orientation = orientation;
}

void
ctk_box_gadget_set_draw_focus (CtkBoxGadget *gadget,
                               gboolean      draw_focus)
{
  CtkBoxGadgetPrivate *priv = ctk_box_gadget_get_instance_private (gadget);

  priv->draw_focus = draw_focus;
}

void
ctk_box_gadget_set_draw_reverse (CtkBoxGadget *gadget,
                                 gboolean      draw_reverse)
{
  CtkBoxGadgetPrivate *priv = ctk_box_gadget_get_instance_private (gadget);

  priv->draw_reverse = draw_reverse;
}

void
ctk_box_gadget_set_allocate_reverse (CtkBoxGadget *gadget,
                                     gboolean      allocate_reverse)
{
  CtkBoxGadgetPrivate *priv = ctk_box_gadget_get_instance_private (gadget);

  priv->allocate_reverse = allocate_reverse;
}

void
ctk_box_gadget_set_align_reverse (CtkBoxGadget *gadget,
                                  gboolean      align_reverse)
{
  CtkBoxGadgetPrivate *priv = ctk_box_gadget_get_instance_private (gadget);

  priv->align_reverse = align_reverse;
}

static CtkCssNode *
get_css_node (GObject *child)
{
  if (CTK_IS_WIDGET (child))
    return ctk_widget_get_css_node (CTK_WIDGET (child));
  else
    return ctk_css_gadget_get_node (CTK_CSS_GADGET (child));
}

static void
ctk_box_gadget_insert_object (CtkBoxGadget *gadget,
                              int           pos,
                              GObject      *object,
                              gboolean      expand,
                              CtkAlign      align)
{
  CtkBoxGadgetPrivate *priv = ctk_box_gadget_get_instance_private (gadget);
  CtkBoxGadgetChild child;

  child.object = g_object_ref (object);
  child.expand = expand;
  child.align = align;

  if (pos < 0 || pos >= priv->children->len)
    {
      g_array_append_val (priv->children, child);
      ctk_css_node_insert_before (ctk_css_gadget_get_node (CTK_CSS_GADGET (gadget)),
                                  get_css_node (object),
                                  NULL);
    }
  else
    {
      g_array_insert_val (priv->children, pos, child);
      ctk_css_node_insert_before (ctk_css_gadget_get_node (CTK_CSS_GADGET (gadget)),
                                  get_css_node (object),
                                  get_css_node (g_array_index (priv->children, CtkBoxGadgetChild, pos + 1).object));
    }
}

void
ctk_box_gadget_insert_widget (CtkBoxGadget *gadget,
                              int           pos,
                              CtkWidget    *widget)
{
  ctk_box_gadget_insert_object (gadget, pos, G_OBJECT (widget), FALSE, CTK_ALIGN_FILL);
}

static CtkBoxGadgetChild *
ctk_box_gadget_find_object (CtkBoxGadget *gadget,
                            GObject      *object,
                            int          *position)
{
  CtkBoxGadgetPrivate *priv = ctk_box_gadget_get_instance_private (gadget);
  guint i;

  for (i = 0; i < priv->children->len; i++)
    {
      CtkBoxGadgetChild *child = &g_array_index (priv->children, CtkBoxGadgetChild, i);

      if (child->object == object)
        {
          if (position)
            *position = i;
          return child;
        }
    }

  return NULL;
}

static void
ctk_box_gadget_remove_object (CtkBoxGadget *gadget,
                              GObject      *object)
{
  CtkBoxGadgetPrivate *priv = ctk_box_gadget_get_instance_private (gadget);
  CtkBoxGadgetChild *child;
  int position;

  child = ctk_box_gadget_find_object (gadget, object, &position);
  if (child)
    {
      ctk_css_node_set_parent (get_css_node (child->object), NULL);
      g_array_remove_index (priv->children, position);
    }
}

void
ctk_box_gadget_remove_widget (CtkBoxGadget *gadget,
                              CtkWidget    *widget)
{
  ctk_box_gadget_remove_object (gadget, G_OBJECT (widget));
}

void
ctk_box_gadget_insert_gadget_before (CtkBoxGadget *gadget,
                                     CtkCssGadget *sibling,
                                     CtkCssGadget *cssgadget,
                                     gboolean      expand,
                                     CtkAlign      align)
{
  /* Insert at the end if no sibling specified */
  int pos = -1;

  if (sibling)
    ctk_box_gadget_find_object (gadget, G_OBJECT (sibling), &pos);

  ctk_box_gadget_insert_gadget (gadget, pos, cssgadget, expand, align);
}

void
ctk_box_gadget_insert_gadget_after (CtkBoxGadget *gadget,
                                    CtkCssGadget *sibling,
                                    CtkCssGadget *cssgadget,
                                    gboolean      expand,
                                    CtkAlign      align)
{
  /* Insert at the beginning if no sibling specified */
  int pos = 0;

  if (sibling && ctk_box_gadget_find_object (gadget, G_OBJECT (sibling), &pos))
    pos++;

  ctk_box_gadget_insert_gadget (gadget, pos, cssgadget, expand, align);
}

void
ctk_box_gadget_insert_gadget (CtkBoxGadget *gadget,
                              int           pos,
                              CtkCssGadget *cssgadget,
                              gboolean      expand,
                              CtkAlign      align)
{
  ctk_box_gadget_insert_object (gadget, pos, G_OBJECT (cssgadget), expand, align);
}

void
ctk_box_gadget_remove_gadget (CtkBoxGadget *gadget,
                              CtkCssGadget *cssgadget)
{
  ctk_box_gadget_remove_object (gadget, G_OBJECT (cssgadget));
}

void
ctk_box_gadget_reverse_children (CtkBoxGadget *gadget)
{
  CtkBoxGadgetPrivate *priv = ctk_box_gadget_get_instance_private (CTK_BOX_GADGET (gadget));
  int i, j;

  ctk_css_node_reverse_children (ctk_css_gadget_get_node (CTK_CSS_GADGET (gadget)));

  for (i = 0, j = priv->children->len - 1; i < j; i++, j--)
    {
      CtkBoxGadgetChild *child1 = &g_array_index (priv->children, CtkBoxGadgetChild, i);
      CtkBoxGadgetChild *child2 = &g_array_index (priv->children, CtkBoxGadgetChild, j);
      CtkBoxGadgetChild tmp;

      tmp = *child1;
      *child1 = *child2;
      *child2 = tmp;
    }
}

void
ctk_box_gadget_set_gadget_expand (CtkBoxGadget *gadget,
                                  GObject      *object,
                                  gboolean      expand)
{
  CtkBoxGadgetChild *child;

  child = ctk_box_gadget_find_object (gadget, object, NULL);

  if (!child)
    return;

  if (child->expand == expand)
    return;

  child->expand = expand;
  ctk_css_gadget_queue_resize (CTK_CSS_GADGET (gadget));
}

void
ctk_box_gadget_set_gadget_align (CtkBoxGadget *gadget,
                                 GObject      *object,
                                 CtkAlign      align)
{
  CtkBoxGadgetChild *child;

  child = ctk_box_gadget_find_object (gadget, object, NULL);

  if (!child)
    return;

  if (child->align == align)
    return;

  child->align = align;
  ctk_css_gadget_queue_resize (CTK_CSS_GADGET (gadget));
}
