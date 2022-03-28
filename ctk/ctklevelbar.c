/* CTK - The GIMP Toolkit
 * Copyright © 2012 Red Hat, Inc.
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
 *
 * Author: Cosimo Cecchi <cosimoc@gnome.org>
 *
 */

/**
 * SECTION:ctklevelbar
 * @Title: CtkLevelBar
 * @Short_description: A bar that can used as a level indicator
 *
 * The #CtkLevelBar is a bar widget that can be used
 * as a level indicator. Typical use cases are displaying the strength
 * of a password, or showing the charge level of a battery.
 *
 * Use ctk_level_bar_set_value() to set the current value, and
 * ctk_level_bar_add_offset_value() to set the value offsets at which
 * the bar will be considered in a different state. CTK will add a few
 * offsets by default on the level bar: #CTK_LEVEL_BAR_OFFSET_LOW,
 * #CTK_LEVEL_BAR_OFFSET_HIGH and #CTK_LEVEL_BAR_OFFSET_FULL, with
 * values 0.25, 0.75 and 1.0 respectively.
 *
 * Note that it is your responsibility to update preexisting offsets
 * when changing the minimum or maximum value. CTK+ will simply clamp
 * them to the new range.
 *
 * ## Adding a custom offset on the bar
 *
 * |[<!-- language="C" -->
 *
 * static CtkWidget *
 * create_level_bar (void)
 * {
 *   CtkWidget *widget;
 *   CtkLevelBar *bar;
 *
 *   widget = ctk_level_bar_new ();
 *   bar = CTK_LEVEL_BAR (widget);
 *
 *   // This changes the value of the default low offset
 *
 *   ctk_level_bar_add_offset_value (bar,
 *                                   CTK_LEVEL_BAR_OFFSET_LOW,
 *                                   0.10);
 *
 *   // This adds a new offset to the bar; the application will
 *   // be able to change its color CSS like this:
 *   //
 *   // levelbar block.my-offset {
 *   //   background-color: magenta;
 *   //   border-style: solid;
 *   //   border-color: black;
 *   //   border-style: 1px;
 *   // }
 *
 *   ctk_level_bar_add_offset_value (bar, "my-offset", 0.60);
 *
 *   return widget;
 * }
 * ]|
 *
 * The default interval of values is between zero and one, but it’s possible to
 * modify the interval using ctk_level_bar_set_min_value() and
 * ctk_level_bar_set_max_value(). The value will be always drawn in proportion to
 * the admissible interval, i.e. a value of 15 with a specified interval between
 * 10 and 20 is equivalent to a value of 0.5 with an interval between 0 and 1.
 * When #CTK_LEVEL_BAR_MODE_DISCRETE is used, the bar level is rendered
 * as a finite number of separated blocks instead of a single one. The number
 * of blocks that will be rendered is equal to the number of units specified by
 * the admissible interval.
 *
 * For instance, to build a bar rendered with five blocks, it’s sufficient to
 * set the minimum value to 0 and the maximum value to 5 after changing the indicator
 * mode to discrete.
 *
 * CtkLevelBar was introduced in CTK+ 3.6.
 *
 * # CtkLevelBar as CtkBuildable
 *
 * The CtkLevelBar implementation of the CtkBuildable interface supports a
 * custom <offsets> element, which can contain any number of <offset> elements,
 * each of which must have name and value attributes.
 *
 * # CSS nodes
 *
 * |[<!-- language="plain" -->
 * levelbar[.discrete]
 * ╰── trough
 *     ├── block.filled.level-name
 *     ┊
 *     ├── block.empty
 *     ┊
 * ]|
 *
 * CtkLevelBar has a main CSS node with name levelbar and one of the style
 * classes .discrete or .continuous and a subnode with name trough. Below the
 * trough node are a number of nodes with name block and style class .filled
 * or .empty. In continuous mode, there is exactly one node of each, in discrete
 * mode, the number of filled and unfilled nodes corresponds to blocks that are
 * drawn. The block.filled nodes also get a style class .level-name corresponding
 * to the level for the current value.
 *
 * In horizontal orientation, the nodes are always arranged from left to right,
 * regardless of text direction.
 */
#include "config.h"

#include "ctkbuildable.h"
#include "ctkbuilderprivate.h"
#include "ctkcsscustomgadgetprivate.h"
#include "ctkintl.h"
#include "ctkorientableprivate.h"
#include "ctklevelbar.h"
#include "ctkmarshalers.h"
#include "ctkstylecontext.h"
#include "ctktypebuiltins.h"
#include "ctkwidget.h"
#include "ctkwidgetprivate.h"
#include "ctkstylecontextprivate.h"
#include "ctkcssstylepropertyprivate.h"
#include "ctkcssnodeprivate.h"

#include <math.h>
#include <stdlib.h>

#include "a11y/ctklevelbaraccessible.h"

#include "fallback-c89.c"

#define DEFAULT_BLOCK_SIZE 3

enum {
  PROP_VALUE = 1,
  PROP_MIN_VALUE,
  PROP_MAX_VALUE,
  PROP_MODE,
  PROP_INVERTED,
  LAST_PROPERTY,
  PROP_ORIENTATION /* overridden */
};

enum {
  SIGNAL_OFFSET_CHANGED,
  NUM_SIGNALS
};

static GParamSpec *properties[LAST_PROPERTY] = { NULL, };
static guint signals[NUM_SIGNALS] = { 0, };

typedef struct {
  gchar *name;
  gdouble value;
} CtkLevelBarOffset;

struct _CtkLevelBarPrivate {
  CtkOrientation orientation;

  CtkLevelBarMode bar_mode;

  gdouble min_value;
  gdouble max_value;
  gdouble cur_value;

  GList *offsets;

  CtkCssGadget *trough_gadget;
  CtkCssGadget **block_gadget;
  guint n_blocks;

  guint inverted : 1;
};

static void ctk_level_bar_set_value_internal (CtkLevelBar *self,
                                              gdouble      value);

static void ctk_level_bar_buildable_init (CtkBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (CtkLevelBar, ctk_level_bar, CTK_TYPE_WIDGET,
                         G_ADD_PRIVATE (CtkLevelBar)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_ORIENTABLE, NULL)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_BUILDABLE,
                                                ctk_level_bar_buildable_init))

static CtkLevelBarOffset *
ctk_level_bar_offset_new (const gchar *name,
                          gdouble      value)
{
  CtkLevelBarOffset *offset = g_slice_new0 (CtkLevelBarOffset);

  offset->name = g_strdup (name);
  offset->value = value;

  return offset;
}

static void
ctk_level_bar_offset_free (CtkLevelBarOffset *offset)
{
  g_free (offset->name);
  g_slice_free (CtkLevelBarOffset, offset);
}

static gint
offset_find_func (gconstpointer data,
                  gconstpointer user_data)
{
  const CtkLevelBarOffset *offset = data;
  const gchar *name = user_data;

  return g_strcmp0 (name, offset->name);
}

static gint
offset_sort_func (gconstpointer a,
                  gconstpointer b)
{
  const CtkLevelBarOffset *offset_a = a;
  const CtkLevelBarOffset *offset_b = b;

  return (offset_a->value > offset_b->value);
}

static gboolean
ctk_level_bar_ensure_offset (CtkLevelBar *self,
                             const gchar *name,
                             gdouble      value)
{
  GList *existing;
  CtkLevelBarOffset *offset = NULL;
  CtkLevelBarOffset *new_offset;

  existing = g_list_find_custom (self->priv->offsets, name, offset_find_func);
  if (existing)
    offset = existing->data;

  if (offset && (offset->value == value))
    return FALSE;

  new_offset = ctk_level_bar_offset_new (name, value);

  if (offset)
    {
      ctk_level_bar_offset_free (offset);
      self->priv->offsets = g_list_delete_link (self->priv->offsets, existing);
    }

  self->priv->offsets = g_list_insert_sorted (self->priv->offsets, new_offset, offset_sort_func);

  return TRUE;
}

static gboolean
ctk_level_bar_value_in_interval (CtkLevelBar *self,
                                 gdouble      value)
{
  return ((value >= self->priv->min_value) &&
          (value <= self->priv->max_value));
}

static gint
ctk_level_bar_get_num_blocks (CtkLevelBar *self)
{
  if (self->priv->bar_mode == CTK_LEVEL_BAR_MODE_CONTINUOUS)
    return 1;
  else if (self->priv->bar_mode == CTK_LEVEL_BAR_MODE_DISCRETE)
    return MAX (1, (gint) (round (self->priv->max_value) - round (self->priv->min_value)));

  return 0;
}

static gint
ctk_level_bar_get_num_block_nodes (CtkLevelBar *self)
{
  if (self->priv->bar_mode == CTK_LEVEL_BAR_MODE_CONTINUOUS)
    return 2;
  else
    return ctk_level_bar_get_num_blocks (self);
}

static void
ctk_level_bar_get_min_block_size (CtkLevelBar *self,
                                  gint        *block_width,
                                  gint        *block_height)
{
  guint i, n_blocks;
  gint width, height;

  *block_width = *block_height = 0;
  n_blocks = ctk_level_bar_get_num_block_nodes (self);

  for (i = 0; i < n_blocks; i++)
    {
      ctk_css_gadget_get_preferred_size (self->priv->block_gadget[i],
                                         CTK_ORIENTATION_HORIZONTAL,
                                         -1,
                                         &width, NULL,
                                         NULL, NULL);
      ctk_css_gadget_get_preferred_size (self->priv->block_gadget[i],
                                         CTK_ORIENTATION_VERTICAL,
                                         -1,
                                         &height, NULL,
                                         NULL, NULL);

      *block_width = MAX (width, *block_width);
      *block_height = MAX (height, *block_height);
    }
}

static gboolean
ctk_level_bar_get_real_inverted (CtkLevelBar *self)
{
  if (ctk_widget_get_direction (CTK_WIDGET (self)) == CTK_TEXT_DIR_RTL &&
      self->priv->orientation == CTK_ORIENTATION_HORIZONTAL)
    return !self->priv->inverted;

  return self->priv->inverted;
}

static void
ctk_level_bar_draw_fill_continuous (CtkLevelBar *self,
                                    cairo_t     *cr)
{
  gboolean inverted;

  inverted = ctk_level_bar_get_real_inverted (self);

  /* render the empty (unfilled) part */
  ctk_css_gadget_draw (self->priv->block_gadget[inverted ? 0 : 1], cr);

  /* now render the filled part on top of it */
  if (self->priv->cur_value != 0)
    ctk_css_gadget_draw (self->priv->block_gadget[inverted ? 1 : 0], cr);
}

static void
ctk_level_bar_draw_fill_discrete (CtkLevelBar *self,
                                  cairo_t     *cr)
{
  gint num_blocks, i;

  num_blocks = ctk_level_bar_get_num_blocks (self);

  for (i = 0; i < num_blocks; i++)
    ctk_css_gadget_draw (self->priv->block_gadget[i], cr);
}

static gboolean
ctk_level_bar_render_trough (CtkCssGadget *gadget,
                             cairo_t      *cr,
                             int           x,
                             int           y,
                             int           width,
                             int           height,
                             gpointer      data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkLevelBar *self = CTK_LEVEL_BAR (widget);

  if (self->priv->bar_mode == CTK_LEVEL_BAR_MODE_CONTINUOUS)
    ctk_level_bar_draw_fill_continuous (self, cr);
  else
    ctk_level_bar_draw_fill_discrete (self, cr);

  return FALSE;
}

static gboolean
ctk_level_bar_draw (CtkWidget *widget,
                    cairo_t   *cr)
{
  CtkLevelBar *self = CTK_LEVEL_BAR (widget);

  ctk_css_gadget_draw (self->priv->trough_gadget, cr);

  return FALSE;
}

static void
ctk_level_bar_measure_trough (CtkCssGadget   *gadget,
                              CtkOrientation  orientation,
                              int             for_size,
                              int            *minimum,
                              int            *natural,
                              int            *minimum_baseline,
                              int            *natural_baseline,
                              gpointer        data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkLevelBar *self = CTK_LEVEL_BAR (widget);
  gint num_blocks, size;
  gint block_width, block_height;

  num_blocks = ctk_level_bar_get_num_blocks (self);
  ctk_level_bar_get_min_block_size (self, &block_width, &block_height);

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      if (self->priv->orientation == CTK_ORIENTATION_HORIZONTAL)
        size = num_blocks * block_width;
      else
        size = block_width;
    }
  else
    {
      if (self->priv->orientation == CTK_ORIENTATION_VERTICAL)
        size = num_blocks * block_height;
      else
        size = block_height;
    }

  *minimum = size;
  *natural = size;
}

static void
ctk_level_bar_get_preferred_width (CtkWidget *widget,
                                   gint      *minimum,
                                   gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CTK_LEVEL_BAR (widget)->priv->trough_gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_level_bar_get_preferred_height (CtkWidget *widget,
                                    gint      *minimum,
                                    gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CTK_LEVEL_BAR (widget)->priv->trough_gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_level_bar_allocate_trough_continuous (CtkLevelBar *self,
                                          const CtkAllocation *allocation,
                                          int baseline,
                                          CtkAllocation *out_clip)
{
  CtkAllocation block_area, clip;
  gdouble fill_percentage;
  gboolean inverted;
  int block_min;

  inverted = ctk_level_bar_get_real_inverted (self);

  /* allocate the empty (unfilled) part */
  ctk_css_gadget_allocate (self->priv->block_gadget[inverted ? 0 : 1],
                           allocation,
                           baseline,
                           out_clip);

  if (self->priv->cur_value == 0)
    return;

  /* now allocate the filled part */
  block_area = *allocation;
  fill_percentage = (self->priv->cur_value - self->priv->min_value) /
    (self->priv->max_value - self->priv->min_value);

  ctk_css_gadget_get_preferred_size (self->priv->block_gadget[inverted ? 1 : 0],
                                     self->priv->orientation, -1,
                                     &block_min, NULL,
                                     NULL, NULL);

  if (self->priv->orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      block_area.width = (gint) floor (block_area.width * fill_percentage);
      block_area.width = MAX (block_area.width, block_min);

      if (inverted)
        block_area.x += allocation->width - block_area.width;
    }
  else
    {
      block_area.height = (gint) floor (block_area.height * fill_percentage);
      block_area.height = MAX (block_area.height, block_min);

      if (inverted)
        block_area.y += allocation->height - block_area.height;
    }

  ctk_css_gadget_allocate (self->priv->block_gadget[inverted ? 1 : 0],
                           &block_area,
                           baseline,
                           &clip);
  cdk_rectangle_intersect (out_clip, &clip, out_clip);
}

static void
ctk_level_bar_allocate_trough_discrete (CtkLevelBar *self,
                                        const CtkAllocation *allocation,
                                        int baseline,
                                        CtkAllocation *out_clip)
{
  CtkAllocation block_area, clip;
  gint num_blocks, i;
  gint block_width, block_height;

  ctk_level_bar_get_min_block_size (self, &block_width, &block_height);
  num_blocks = ctk_level_bar_get_num_blocks (self);

  if (num_blocks == 0)
    return;

  if (self->priv->orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      block_width = MAX (block_width, (gint) floor (allocation->width / num_blocks));
      block_height = allocation->height;
    }
  else
    {
      block_width = allocation->width;
      block_height = MAX (block_height, (gint) floor (allocation->height / num_blocks));
    }

  block_area.x = allocation->x;
  block_area.y = allocation->y;
  block_area.width = block_width;
  block_area.height = block_height;

  for (i = 0; i < num_blocks; i++)
    {
      ctk_css_gadget_allocate (self->priv->block_gadget[i],
                               &block_area,
                               baseline,
                               &clip);
      cdk_rectangle_intersect (out_clip, &clip, out_clip);

      if (self->priv->orientation == CTK_ORIENTATION_HORIZONTAL)
        block_area.x += block_area.width;
      else
        block_area.y += block_area.height;
    }
}

static void
ctk_level_bar_allocate_trough (CtkCssGadget        *gadget,
                               const CtkAllocation *allocation,
                               int                  baseline,
                               CtkAllocation       *out_clip,
                               gpointer             data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkLevelBar *self = CTK_LEVEL_BAR (widget);

  if (self->priv->bar_mode == CTK_LEVEL_BAR_MODE_CONTINUOUS)
    ctk_level_bar_allocate_trough_continuous (self, allocation, baseline, out_clip);
  else
    ctk_level_bar_allocate_trough_discrete (self, allocation, baseline, out_clip);
}

static void
ctk_level_bar_size_allocate (CtkWidget     *widget,
                             CtkAllocation *allocation)
{
  CtkAllocation clip;

  CTK_WIDGET_CLASS (ctk_level_bar_parent_class)->size_allocate (widget, allocation);

  ctk_css_gadget_allocate (CTK_LEVEL_BAR (widget)->priv->trough_gadget,
                           allocation,
                           ctk_widget_get_allocated_baseline (widget),
                           &clip);

  ctk_widget_set_clip (widget, &clip);
}

static void
update_block_nodes (CtkLevelBar *self)
{
  CtkLevelBarPrivate *priv = self->priv;
  CtkCssNode *trough_node = ctk_css_gadget_get_node (priv->trough_gadget);
  guint n_blocks;
  guint i;

  n_blocks = ctk_level_bar_get_num_block_nodes (self);

  if (priv->n_blocks == n_blocks)
    return;
  else if (n_blocks < priv->n_blocks)
    {
      for (i = n_blocks; i < priv->n_blocks; i++)
        {
          ctk_css_node_set_parent (ctk_css_gadget_get_node (priv->block_gadget[i]), NULL);
          g_clear_object (&priv->block_gadget[i]);
        }
      priv->block_gadget = g_renew (CtkCssGadget*, priv->block_gadget, n_blocks);
      priv->n_blocks = n_blocks;
    }
  else
    {
      priv->block_gadget = g_renew (CtkCssGadget*, priv->block_gadget, n_blocks);
      for (i = priv->n_blocks; i < n_blocks; i++)
        {
          priv->block_gadget[i] = ctk_css_custom_gadget_new ("block",
                                                             CTK_WIDGET (self),
                                                             priv->trough_gadget,
                                                             NULL,
                                                             NULL, NULL, NULL,
                                                             NULL, NULL);
          ctk_css_gadget_set_state (priv->block_gadget[i], ctk_css_node_get_state (trough_node));
        }
      priv->n_blocks = n_blocks;
    }
}

static void
update_mode_style_classes (CtkLevelBar *self)
{
  CtkLevelBarPrivate *priv = self->priv;
  CtkCssNode *widget_node;

  widget_node = ctk_widget_get_css_node (CTK_WIDGET (self));
  if (priv->bar_mode == CTK_LEVEL_BAR_MODE_CONTINUOUS)
    {
      ctk_css_node_remove_class (widget_node, g_quark_from_static_string ("discrete"));
      ctk_css_node_add_class (widget_node, g_quark_from_static_string ("continuous"));
    }
  else if (priv->bar_mode == CTK_LEVEL_BAR_MODE_DISCRETE)
    {
      ctk_css_node_add_class (widget_node, g_quark_from_static_string ("discrete"));
      ctk_css_node_remove_class (widget_node, g_quark_from_static_string ("continuous"));
    }
}

static void
ctk_level_bar_state_flags_changed (CtkWidget     *widget,
                                   CtkStateFlags  previous_state)
{
  CtkLevelBar *self = CTK_LEVEL_BAR (widget);
  CtkLevelBarPrivate *priv = self->priv;
  CtkStateFlags state;
  gint i;

  state = ctk_widget_get_state_flags (widget);

  ctk_css_gadget_set_state (priv->trough_gadget, state);
  for (i = 0; i < priv->n_blocks; i++)
    ctk_css_gadget_set_state (priv->block_gadget[i], state);

  CTK_WIDGET_CLASS (ctk_level_bar_parent_class)->state_flags_changed (widget, previous_state);
}

static void
update_level_style_classes (CtkLevelBar *self)
{
  CtkLevelBarPrivate *priv = self->priv;
  gdouble value;
  const gchar *classes[3] = { NULL, NULL, NULL };
  const gchar *value_class = NULL;
  CtkLevelBarOffset *offset, *prev_offset;
  GList *l;
  gint num_filled, num_blocks, i;
  gboolean inverted;

  value = ctk_level_bar_get_value (self);

  for (l = priv->offsets; l != NULL; l = l->next)
    {
      offset = l->data;

      /* find the right offset for our style class */
      if (value <= offset->value)
        {
          if (l->prev == NULL)
            {
              value_class = offset->name;
            }
          else
            {
              prev_offset = l->prev->data;
              if (prev_offset->value < value)
                value_class = offset->name;
            }
        }

      if (value_class)
        break;
    }

  inverted = ctk_level_bar_get_real_inverted (self);
  num_blocks = ctk_level_bar_get_num_block_nodes (self);

  if (priv->bar_mode == CTK_LEVEL_BAR_MODE_CONTINUOUS)
    num_filled = 1;
  else
    num_filled = MIN (num_blocks, (gint) round (priv->cur_value) - (gint) round (priv->min_value));

  classes[0] = "filled";
  classes[1] = value_class;
  for (i = 0; i < num_filled; i++)
    ctk_css_node_set_classes (ctk_css_gadget_get_node (priv->block_gadget[inverted ? num_blocks - 1 - i : i]), classes);

  classes[0] = "empty";
  classes[1] = NULL;
  for (; i < num_blocks; i++)
    ctk_css_node_set_classes (ctk_css_gadget_get_node (priv->block_gadget[inverted ? num_blocks - 1 - i : i]), classes);
}

static void
ctk_level_bar_direction_changed (CtkWidget        *widget,
                                 CtkTextDirection  previous_dir)
{
  CtkLevelBar *self = CTK_LEVEL_BAR (widget);

  update_level_style_classes (self);

  CTK_WIDGET_CLASS (ctk_level_bar_parent_class)->direction_changed (widget, previous_dir);
}

static void
ctk_level_bar_ensure_offsets_in_range (CtkLevelBar *self)
{
  CtkLevelBarOffset *offset;
  GList *l = self->priv->offsets;

  while (l != NULL)
    {
      offset = l->data;
      l = l->next;

      if (offset->value < self->priv->min_value)
        ctk_level_bar_ensure_offset (self, offset->name, self->priv->min_value);
      else if (offset->value > self->priv->max_value)
        ctk_level_bar_ensure_offset (self, offset->name, self->priv->max_value);
    }
}

typedef struct {
  CtkLevelBar *self;
  CtkBuilder *builder;
  GList *offsets;
} OffsetsParserData;

static void
offset_start_element (GMarkupParseContext  *context,
                      const gchar          *element_name,
                      const gchar         **names,
                      const gchar         **values,
                      gpointer              user_data,
                      GError              **error)
{
  OffsetsParserData *data = user_data;

  if (strcmp (element_name, "offsets") == 0)
    {
      if (!_ctk_builder_check_parent (data->builder, context, "object", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_INVALID, NULL, NULL,
                                        G_MARKUP_COLLECT_INVALID))
        _ctk_builder_prefix_error (data->builder, context, error);
    }
  else if (strcmp (element_name, "offset") == 0)
    {
      const gchar *name;
      const gchar *value;
      GValue gvalue = G_VALUE_INIT;
      CtkLevelBarOffset *offset;

      if (!_ctk_builder_check_parent (data->builder, context, "offsets", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_STRING, "name", &name,
                                        G_MARKUP_COLLECT_STRING, "value", &value,
                                        G_MARKUP_COLLECT_INVALID))
        {
          _ctk_builder_prefix_error (data->builder, context, error);
          return;
        }

      if (!ctk_builder_value_from_string_type (data->builder, G_TYPE_DOUBLE, value, &gvalue, error))
        {
          _ctk_builder_prefix_error (data->builder, context, error);
          return;
        }

      offset = ctk_level_bar_offset_new (name, g_value_get_double (&gvalue));
      data->offsets = g_list_prepend (data->offsets, offset);
    }
  else
    {
      _ctk_builder_error_unhandled_tag (data->builder, context,
                                        "CtkLevelBar", element_name,
                                        error);
    }
}

static const GMarkupParser offset_parser =
{
  .start_element = offset_start_element
};

static gboolean
ctk_level_bar_buildable_custom_tag_start (CtkBuildable  *buildable,
                                          CtkBuilder    *builder,
                                          GObject       *child,
                                          const gchar   *tagname,
                                          GMarkupParser *parser,
                                          gpointer      *parser_data)
{
  OffsetsParserData *data;

  if (child)
    return FALSE;

  if (strcmp (tagname, "offsets") != 0)
    return FALSE;

  data = g_slice_new0 (OffsetsParserData);
  data->self = CTK_LEVEL_BAR (buildable);
  data->builder = builder;
  data->offsets = NULL;

  *parser = offset_parser;
  *parser_data = data;

  return TRUE;
}

static void
ctk_level_bar_buildable_custom_finished (CtkBuildable *buildable,
                                         CtkBuilder   *builder,
                                         GObject      *child,
                                         const gchar  *tagname,
                                         gpointer      user_data)
{
  OffsetsParserData *data = user_data;
  CtkLevelBar *self;
  CtkLevelBarOffset *offset;
  GList *l;

  self = data->self;

  if (strcmp (tagname, "offsets") != 0)
    goto out;

  for (l = data->offsets; l != NULL; l = l->next)
    {
      offset = l->data;
      ctk_level_bar_add_offset_value (self, offset->name, offset->value);
    }

 out:
  g_list_free_full (data->offsets, (GDestroyNotify) ctk_level_bar_offset_free);
  g_slice_free (OffsetsParserData, data);
}

static void
ctk_level_bar_buildable_init (CtkBuildableIface *iface)
{
  iface->custom_tag_start = ctk_level_bar_buildable_custom_tag_start;
  iface->custom_finished = ctk_level_bar_buildable_custom_finished;
}

static void
ctk_level_bar_set_orientation (CtkLevelBar    *self,
                               CtkOrientation  orientation)
{
  if (self->priv->orientation != orientation)
    {
      self->priv->orientation = orientation;
      _ctk_orientable_set_style_classes (CTK_ORIENTABLE (self));
      ctk_widget_queue_resize (CTK_WIDGET (self));
      g_object_notify (G_OBJECT (self), "orientation");
    }
}

static void
ctk_level_bar_get_property (GObject    *obj,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  CtkLevelBar *self = CTK_LEVEL_BAR (obj);

  switch (property_id)
    {
    case PROP_VALUE:
      g_value_set_double (value, ctk_level_bar_get_value (self));
      break;
    case PROP_MIN_VALUE:
      g_value_set_double (value, ctk_level_bar_get_min_value (self));
      break;
    case PROP_MAX_VALUE:
      g_value_set_double (value, ctk_level_bar_get_max_value (self));
      break;
    case PROP_MODE:
      g_value_set_enum (value, ctk_level_bar_get_mode (self));
      break;
    case PROP_INVERTED:
      g_value_set_boolean (value, ctk_level_bar_get_inverted (self));
      break;
    case PROP_ORIENTATION:
      g_value_set_enum (value, self->priv->orientation);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
      break;
    }
}

static void
ctk_level_bar_set_property (GObject      *obj,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  CtkLevelBar *self = CTK_LEVEL_BAR (obj);

  switch (property_id)
    {
    case PROP_VALUE:
      ctk_level_bar_set_value (self, g_value_get_double (value));
      break;
    case PROP_MIN_VALUE:
      ctk_level_bar_set_min_value (self, g_value_get_double (value));
      break;
    case PROP_MAX_VALUE:
      ctk_level_bar_set_max_value (self, g_value_get_double (value));
      break;
    case PROP_MODE:
      ctk_level_bar_set_mode (self, g_value_get_enum (value));
      break;
    case PROP_INVERTED:
      ctk_level_bar_set_inverted (self, g_value_get_boolean (value));
      break;
    case PROP_ORIENTATION:
      ctk_level_bar_set_orientation (self, g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
      break;
    }
}

static void
ctk_level_bar_finalize (GObject *obj)
{
  CtkLevelBar *self = CTK_LEVEL_BAR (obj);
  CtkLevelBarPrivate *priv = self->priv;
  gint i;

  g_list_free_full (priv->offsets, (GDestroyNotify) ctk_level_bar_offset_free);

  for (i = 0; i < priv->n_blocks; i++)
    g_clear_object (&priv->block_gadget[i]);
  g_free (priv->block_gadget);

  g_clear_object (&priv->trough_gadget);

  G_OBJECT_CLASS (ctk_level_bar_parent_class)->finalize (obj);
}

static void
ctk_level_bar_class_init (CtkLevelBarClass *klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);
  CtkWidgetClass *wclass = CTK_WIDGET_CLASS (klass);

  oclass->get_property = ctk_level_bar_get_property;
  oclass->set_property = ctk_level_bar_set_property;
  oclass->finalize = ctk_level_bar_finalize;

  wclass->draw = ctk_level_bar_draw;
  wclass->size_allocate = ctk_level_bar_size_allocate;
  wclass->get_preferred_width = ctk_level_bar_get_preferred_width;
  wclass->get_preferred_height = ctk_level_bar_get_preferred_height;
  wclass->state_flags_changed = ctk_level_bar_state_flags_changed;
  wclass->direction_changed = ctk_level_bar_direction_changed;

  g_object_class_override_property (oclass, PROP_ORIENTATION, "orientation");

  /**
   * CtkLevelBar::offset-changed:
   * @self: a #CtkLevelBar
   * @name: the name of the offset that changed value
   *
   * Emitted when an offset specified on the bar changes value as an
   * effect to ctk_level_bar_add_offset_value() being called.
   *
   * The signal supports detailed connections; you can connect to the
   * detailed signal "changed::x" in order to only receive callbacks when
   * the value of offset "x" changes.
   *
   * Since: 3.6
   */
  signals[SIGNAL_OFFSET_CHANGED] =
    g_signal_new (I_("offset-changed"),
                  CTK_TYPE_LEVEL_BAR,
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_DETAILED,
                  G_STRUCT_OFFSET (CtkLevelBarClass, offset_changed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  1, G_TYPE_STRING);

  /**
   * CtkLevelBar:value:
   *
   * The #CtkLevelBar:value property determines the currently
   * filled value of the level bar.
   *
   * Since: 3.6
   */
  properties[PROP_VALUE] =
    g_param_spec_double ("value",
                         P_("Currently filled value level"),
                         P_("Currently filled value level of the level bar"),
                         0.0, G_MAXDOUBLE, 0.0,
                         G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkLevelBar:min-value:
   *
   * The #CtkLevelBar:min-value property determines the minimum value of
   * the interval that can be displayed by the bar.
   *
   * Since: 3.6
   */
  properties[PROP_MIN_VALUE] =
    g_param_spec_double ("min-value",
                         P_("Minimum value level for the bar"),
                         P_("Minimum value level that can be displayed by the bar"),
                         0.0, G_MAXDOUBLE, 0.0,
                         G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkLevelBar:max-value:
   *
   * The #CtkLevelBar:max-value property determaxes the maximum value of
   * the interval that can be displayed by the bar.
   *
   * Since: 3.6
   */
  properties[PROP_MAX_VALUE] =
    g_param_spec_double ("max-value",
                         P_("Maximum value level for the bar"),
                         P_("Maximum value level that can be displayed by the bar"),
                         0.0, G_MAXDOUBLE, 1.0,
                         G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkLevelBar:mode:
   *
   * The #CtkLevelBar:mode property determines the way #CtkLevelBar
   * interprets the value properties to draw the level fill area.
   * Specifically, when the value is #CTK_LEVEL_BAR_MODE_CONTINUOUS,
   * #CtkLevelBar will draw a single block representing the current value in
   * that area; when the value is #CTK_LEVEL_BAR_MODE_DISCRETE,
   * the widget will draw a succession of separate blocks filling the
   * draw area, with the number of blocks being equal to the units separating
   * the integral roundings of #CtkLevelBar:min-value and #CtkLevelBar:max-value.
   *
   * Since: 3.6
   */
  properties[PROP_MODE] =
    g_param_spec_enum ("mode",
                       P_("The mode of the value indicator"),
                       P_("The mode of the value indicator displayed by the bar"),
                       CTK_TYPE_LEVEL_BAR_MODE,
                       CTK_LEVEL_BAR_MODE_CONTINUOUS,
                       G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkLevelBar:inverted:
   *
   * Level bars normally grow from top to bottom or left to right.
   * Inverted level bars grow in the opposite direction.
   *
   * Since: 3.8
   */
  properties[PROP_INVERTED] =
    g_param_spec_boolean ("inverted",
                          P_("Inverted"),
                          P_("Invert the direction in which the level bar grows"),
                          FALSE,
                          G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkLevelBar:min-block-height:
   *
   * The min-block-height style property determines the minimum
   * height for blocks filling the #CtkLevelBar widget.
   *
   * Since: 3.6
   *
   * Deprecated: 3.20: Use the standard min-width/min-height CSS properties on
   *   the block elements; the value of this style property is ignored.
   */
  ctk_widget_class_install_style_property
    (wclass, g_param_spec_int ("min-block-height",
                               P_("Minimum height for filling blocks"),
                               P_("Minimum height for blocks that fill the bar"),
                               1, G_MAXINT, DEFAULT_BLOCK_SIZE,
                               G_PARAM_READWRITE|G_PARAM_DEPRECATED));
  /**
   * CtkLevelBar:min-block-width:
   *
   * The min-block-width style property determines the minimum
   * width for blocks filling the #CtkLevelBar widget.
   *
   * Since: 3.6
   *
   * Deprecated: 3.20: Use the standard min-width/min-height CSS properties on
   *   the block elements; the value of this style property is ignored.
   */
  ctk_widget_class_install_style_property
    (wclass, g_param_spec_int ("min-block-width",
                               P_("Minimum width for filling blocks"),
                               P_("Minimum width for blocks that fill the bar"),
                               1, G_MAXINT, DEFAULT_BLOCK_SIZE,
                               G_PARAM_READWRITE|G_PARAM_DEPRECATED));

  g_object_class_install_properties (oclass, LAST_PROPERTY, properties);

  ctk_widget_class_set_accessible_type (wclass, CTK_TYPE_LEVEL_BAR_ACCESSIBLE);
  ctk_widget_class_set_css_name (wclass, "levelbar");
}

static void
ctk_level_bar_init (CtkLevelBar *self)
{
  CtkLevelBarPrivate *priv;
  CtkCssNode *widget_node, *trough_node;

  priv = self->priv = ctk_level_bar_get_instance_private (self);

  priv->cur_value = 0.0;
  priv->min_value = 0.0;
  priv->max_value = 1.0;

  /* set initial orientation and style classes */
  priv->orientation = CTK_ORIENTATION_HORIZONTAL;
  _ctk_orientable_set_style_classes (CTK_ORIENTABLE (self));

  priv->inverted = FALSE;

  ctk_widget_set_has_window (CTK_WIDGET (self), FALSE);

  widget_node = ctk_widget_get_css_node (CTK_WIDGET (self));
  priv->trough_gadget = ctk_css_custom_gadget_new ("trough",
                                                   CTK_WIDGET (self),
                                                   NULL, NULL,
                                                   ctk_level_bar_measure_trough,
                                                   ctk_level_bar_allocate_trough,
                                                   ctk_level_bar_render_trough,
                                                   NULL, NULL);
  trough_node = ctk_css_gadget_get_node (priv->trough_gadget);
  ctk_css_node_set_parent (trough_node, widget_node);
  ctk_css_node_set_state (trough_node, ctk_css_node_get_state (widget_node));

  ctk_level_bar_ensure_offset (self, CTK_LEVEL_BAR_OFFSET_LOW, 0.25);
  ctk_level_bar_ensure_offset (self, CTK_LEVEL_BAR_OFFSET_HIGH, 0.75);
  ctk_level_bar_ensure_offset (self, CTK_LEVEL_BAR_OFFSET_FULL, 1.0);

  priv->block_gadget = NULL;
  priv->n_blocks = 0;

  priv->bar_mode = CTK_LEVEL_BAR_MODE_CONTINUOUS;
  update_mode_style_classes (self);
  update_block_nodes (self);
  update_level_style_classes (self);
}

/**
 * ctk_level_bar_new:
 *
 * Creates a new #CtkLevelBar.
 *
 * Returns: a #CtkLevelBar.
 *
 * Since: 3.6
 */
CtkWidget *
ctk_level_bar_new (void)
{
  return g_object_new (CTK_TYPE_LEVEL_BAR, NULL);
}

/**
 * ctk_level_bar_new_for_interval:
 * @min_value: a positive value
 * @max_value: a positive value
 *
 * Utility constructor that creates a new #CtkLevelBar for the specified
 * interval.
 *
 * Returns: a #CtkLevelBar
 *
 * Since: 3.6
 */
CtkWidget *
ctk_level_bar_new_for_interval (gdouble min_value,
                                gdouble max_value)
{
  return g_object_new (CTK_TYPE_LEVEL_BAR,
                       "min-value", min_value,
                       "max-value", max_value,
                       NULL);
}

/**
 * ctk_level_bar_get_min_value:
 * @self: a #CtkLevelBar
 *
 * Returns the value of the #CtkLevelBar:min-value property.
 *
 * Returns: a positive value
 *
 * Since: 3.6
 */
gdouble
ctk_level_bar_get_min_value (CtkLevelBar *self)
{
  g_return_val_if_fail (CTK_IS_LEVEL_BAR (self), 0.0);

  return self->priv->min_value;
}

/**
 * ctk_level_bar_get_max_value:
 * @self: a #CtkLevelBar
 *
 * Returns the value of the #CtkLevelBar:max-value property.
 *
 * Returns: a positive value
 *
 * Since: 3.6
 */
gdouble
ctk_level_bar_get_max_value (CtkLevelBar *self)
{
  g_return_val_if_fail (CTK_IS_LEVEL_BAR (self), 0.0);

  return self->priv->max_value;
}

/**
 * ctk_level_bar_get_value:
 * @self: a #CtkLevelBar
 *
 * Returns the value of the #CtkLevelBar:value property.
 *
 * Returns: a value in the interval between
 *     #CtkLevelBar:min-value and #CtkLevelBar:max-value
 *
 * Since: 3.6
 */
gdouble
ctk_level_bar_get_value (CtkLevelBar *self)
{
  g_return_val_if_fail (CTK_IS_LEVEL_BAR (self), 0.0);

  return self->priv->cur_value;
}

static void
ctk_level_bar_set_value_internal (CtkLevelBar *self,
                                  gdouble      value)
{
  self->priv->cur_value = value;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_VALUE]);
  ctk_widget_queue_allocate (CTK_WIDGET (self));
}

/**
 * ctk_level_bar_set_min_value:
 * @self: a #CtkLevelBar
 * @value: a positive value
 *
 * Sets the value of the #CtkLevelBar:min-value property.
 *
 * You probably want to update preexisting level offsets after calling
 * this function.
 *
 * Since: 3.6
 */
void
ctk_level_bar_set_min_value (CtkLevelBar *self,
                             gdouble      value)
{
  CtkLevelBarPrivate *priv = self->priv;

  g_return_if_fail (CTK_IS_LEVEL_BAR (self));
  g_return_if_fail (value >= 0.0);

  if (value == priv->min_value)
    return;

  priv->min_value = value;

  if (priv->min_value > priv->cur_value)
    ctk_level_bar_set_value_internal (self, priv->min_value);

  update_block_nodes (self);
  update_level_style_classes (self);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MIN_VALUE]);
}

/**
 * ctk_level_bar_set_max_value:
 * @self: a #CtkLevelBar
 * @value: a positive value
 *
 * Sets the value of the #CtkLevelBar:max-value property.
 *
 * You probably want to update preexisting level offsets after calling
 * this function.
 *
 * Since: 3.6
 */
void
ctk_level_bar_set_max_value (CtkLevelBar *self,
                             gdouble      value)
{
  CtkLevelBarPrivate *priv = self->priv;

  g_return_if_fail (CTK_IS_LEVEL_BAR (self));
  g_return_if_fail (value >= 0.0);

  if (value == priv->max_value)
    return;

  priv->max_value = value;

  if (priv->max_value < priv->cur_value)
    ctk_level_bar_set_value_internal (self, priv->max_value);

  ctk_level_bar_ensure_offsets_in_range (self);
  update_block_nodes (self);
  update_level_style_classes (self);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MAX_VALUE]);
}

/**
 * ctk_level_bar_set_value:
 * @self: a #CtkLevelBar
 * @value: a value in the interval between
 *     #CtkLevelBar:min-value and #CtkLevelBar:max-value
 *
 * Sets the value of the #CtkLevelBar:value property.
 *
 * Since: 3.6
 */
void
ctk_level_bar_set_value (CtkLevelBar *self,
                         gdouble      value)
{
  g_return_if_fail (CTK_IS_LEVEL_BAR (self));

  if (value == self->priv->cur_value)
    return;

  ctk_level_bar_set_value_internal (self, value);
  update_level_style_classes (self);
}

/**
 * ctk_level_bar_get_mode:
 * @self: a #CtkLevelBar
 *
 * Returns the value of the #CtkLevelBar:mode property.
 *
 * Returns: a #CtkLevelBarMode
 *
 * Since: 3.6
 */
CtkLevelBarMode
ctk_level_bar_get_mode (CtkLevelBar *self)
{
  g_return_val_if_fail (CTK_IS_LEVEL_BAR (self), 0);

  return self->priv->bar_mode;
}

/**
 * ctk_level_bar_set_mode:
 * @self: a #CtkLevelBar
 * @mode: a #CtkLevelBarMode
 *
 * Sets the value of the #CtkLevelBar:mode property.
 *
 * Since: 3.6
 */
void
ctk_level_bar_set_mode (CtkLevelBar     *self,
                        CtkLevelBarMode  mode)
{
  CtkLevelBarPrivate *priv = self->priv;

  g_return_if_fail (CTK_IS_LEVEL_BAR (self));

  if (priv->bar_mode == mode)
    return;

  priv->bar_mode = mode;

  update_mode_style_classes (self);
  update_block_nodes (self);
  update_level_style_classes (self);
  ctk_widget_queue_resize (CTK_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MODE]);

}

/**
 * ctk_level_bar_get_inverted:
 * @self: a #CtkLevelBar
 *
 * Return the value of the #CtkLevelBar:inverted property.
 *
 * Returns: %TRUE if the level bar is inverted
 *
 * Since: 3.8
 */
gboolean
ctk_level_bar_get_inverted (CtkLevelBar *self)
{
  g_return_val_if_fail (CTK_IS_LEVEL_BAR (self), FALSE);

  return self->priv->inverted;
}

/**
 * ctk_level_bar_set_inverted:
 * @self: a #CtkLevelBar
 * @inverted: %TRUE to invert the level bar
 *
 * Sets the value of the #CtkLevelBar:inverted property.
 *
 * Since: 3.8
 */
void
ctk_level_bar_set_inverted (CtkLevelBar *self,
                            gboolean     inverted)
{
  CtkLevelBarPrivate *priv = self->priv;

  g_return_if_fail (CTK_IS_LEVEL_BAR (self));

  if (priv->inverted == inverted)
    return;

  priv->inverted = inverted;
  ctk_widget_queue_resize (CTK_WIDGET (self));
  update_level_style_classes (self);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_INVERTED]);
}

/**
 * ctk_level_bar_remove_offset_value:
 * @self: a #CtkLevelBar
 * @name: (allow-none): the name of an offset in the bar
 *
 * Removes an offset marker previously added with
 * ctk_level_bar_add_offset_value().
 *
 * Since: 3.6
 */
void
ctk_level_bar_remove_offset_value (CtkLevelBar *self,
                                   const gchar *name)
{
  CtkLevelBarPrivate *priv = self->priv;
  GList *existing;

  g_return_if_fail (CTK_IS_LEVEL_BAR (self));

  existing = g_list_find_custom (priv->offsets, name, offset_find_func);
  if (existing)
    {
      ctk_level_bar_offset_free (existing->data);
      priv->offsets = g_list_delete_link (priv->offsets, existing);

      update_level_style_classes (self);
    }
}

/**
 * ctk_level_bar_add_offset_value:
 * @self: a #CtkLevelBar
 * @name: the name of the new offset
 * @value: the value for the new offset
 *
 * Adds a new offset marker on @self at the position specified by @value.
 * When the bar value is in the interval topped by @value (or between @value
 * and #CtkLevelBar:max-value in case the offset is the last one on the bar)
 * a style class named `level-`@name will be applied
 * when rendering the level bar fill.
 * If another offset marker named @name exists, its value will be
 * replaced by @value.
 *
 * Since: 3.6
 */
void
ctk_level_bar_add_offset_value (CtkLevelBar *self,
                                const gchar *name,
                                gdouble      value)
{
  GQuark name_quark;

  g_return_if_fail (CTK_IS_LEVEL_BAR (self));
  g_return_if_fail (ctk_level_bar_value_in_interval (self, value));

  if (!ctk_level_bar_ensure_offset (self, name, value))
    return;

  update_level_style_classes (self);
  name_quark = g_quark_from_string (name);
  g_signal_emit (self, signals[SIGNAL_OFFSET_CHANGED], name_quark, name);
}

/**
 * ctk_level_bar_get_offset_value:
 * @self: a #CtkLevelBar
 * @name: (allow-none): the name of an offset in the bar
 * @value: (out): location where to store the value
 *
 * Fetches the value specified for the offset marker @name in @self,
 * returning %TRUE in case an offset named @name was found.
 *
 * Returns: %TRUE if the specified offset is found
 *
 * Since: 3.6
 */
gboolean
ctk_level_bar_get_offset_value (CtkLevelBar *self,
                                const gchar *name,
                                gdouble     *value)
{
  GList *existing;
  CtkLevelBarOffset *offset = NULL;

  g_return_val_if_fail (CTK_IS_LEVEL_BAR (self), 0.0);

  existing = g_list_find_custom (self->priv->offsets, name, offset_find_func);
  if (existing)
    offset = existing->data;

  if (!offset)
    return FALSE;

  if (value)
    *value = offset->value;

  return TRUE;
}
