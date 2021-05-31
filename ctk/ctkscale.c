/* CTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 2001 Red Hat, Inc.
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

#include <math.h>
#include <stdlib.h>

#include "ctkrangeprivate.h"

#include "ctkadjustment.h"
#include "ctkbindings.h"
#include "ctkbuildable.h"
#include "ctkbuilderprivate.h"
#include "ctkcsscustomgadgetprivate.h"
#include "ctkicontheme.h"
#include "ctkintl.h"
#include "ctkmarshalers.h"
#include "ctkorientable.h"
#include "ctkprivate.h"
#include "ctktypebuiltins.h"
#include "ctkstylecontextprivate.h"
#include "ctkstylepropertyprivate.h"
#include "ctkwidgetprivate.h"
#include "ctkcsswidgetnodeprivate.h"

#include "a11y/ctkscaleaccessible.h"


/**
 * SECTION:ctkscale
 * @Short_description: A slider widget for selecting a value from a range
 * @Title: CtkScale
 *
 * A CtkScale is a slider control used to select a numeric value. 
 * To use it, you’ll probably want to investigate the methods on
 * its base class, #CtkRange, in addition to the methods for CtkScale itself.
 * To set the value of a scale, you would normally use ctk_range_set_value().
 * To detect changes to the value, you would normally use the
 * #CtkRange::value-changed signal.
 *
 * Note that using the same upper and lower bounds for the #CtkScale (through
 * the #CtkRange methods) will hide the slider itself. This is useful for
 * applications that want to show an undeterminate value on the scale, without
 * changing the layout of the application (such as movie or music players).
 *
 * # CtkScale as CtkBuildable
 *
 * CtkScale supports a custom <marks> element, which can contain multiple
 * <mark> elements. The “value” and “position” attributes have the same
 * meaning as ctk_scale_add_mark() parameters of the same name. If the
 * element is not empty, its content is taken as the markup to show at
 * the mark. It can be translated with the usual ”translatable” and
 * “context” attributes.
 *
 * # CSS nodes
 *
 * |[<!-- language="plain" -->
 * scale[.fine-tune][.marks-before][.marks-after]
 * ├── marks.top
 * │   ├── mark
 * │   ┊    ├── [label]
 * │   ┊    ╰── indicator
 * ┊   ┊
 * │   ╰── mark
 * ├── [value]
 * ├── contents
 * │   ╰── trough
 * │       ├── slider
 * │       ├── [highlight]
 * │       ╰── [fill]
 * ╰── marks.bottom
 *     ├── mark
 *     ┊    ├── indicator
 *     ┊    ╰── [label]
 *     ╰── mark
 * ]|
 *
 * CtkScale has a main CSS node with name scale and a subnode for its contents,
 * with subnodes named trough and slider.
 *
 * The main node gets the style class .fine-tune added when the scale is in
 * 'fine-tuning' mode.
 *
 * If the scale has an origin (see ctk_scale_set_has_origin()), there is a
 * subnode with name highlight below the trough node that is used for rendering
 * the highlighted part of the trough.
 *
 * If the scale is showing a fill level (see ctk_range_set_show_fill_level()),
 * there is a subnode with name fill below the trough node that is used for
 * rendering the filled in part of the trough.
 *
 * If marks are present, there is a marks subnode before or after the contents
 * node, below which each mark gets a node with name mark. The marks nodes get
 * either the .top or .bottom style class.
 *
 * The mark node has a subnode named indicator. If the mark has text, it also
 * has a subnode named label. When the mark is either above or left of the
 * scale, the label subnode is the first when present. Otherwise, the indicator
 * subnode is the first.
 *
 * The main CSS node gets the 'marks-before' and/or 'marks-after' style classes
 * added depending on what marks are present.
 *
 * If the scale is displaying the value (see #CtkScale:draw-value), there is
 * subnode with name value.
 */


#define	MAX_DIGITS	(64)	/* don't change this,
				 * a) you don't need to and
				 * b) you might cause buffer owerflows in
				 *    unrelated code portions otherwise
				 */

typedef struct _CtkScaleMark CtkScaleMark;

struct _CtkScalePrivate
{
  PangoLayout  *layout;

  GSList       *marks;

  CtkCssGadget *top_marks_gadget;
  CtkCssGadget *bottom_marks_gadget;
  CtkCssGadget *value_gadget;

  gint          digits;

  guint         draw_value : 1;
  guint         value_pos  : 2;
};

struct _CtkScaleMark
{
  gdouble          value;
  int              stop_position;
  gchar           *markup;
  PangoLayout     *layout;
  CtkCssGadget    *gadget;
  CtkCssGadget    *indicator_gadget;
  CtkCssGadget    *label_gadget;
  CtkPositionType  position; /* always CTK_POS_TOP or CTK_POS_BOTTOM */
};

enum {
  PROP_0,
  PROP_DIGITS,
  PROP_DRAW_VALUE,
  PROP_HAS_ORIGIN,
  PROP_VALUE_POS,
  LAST_PROP
};

enum {
  FORMAT_VALUE,
  LAST_SIGNAL
};

static GParamSpec *properties[LAST_PROP];
static guint signals[LAST_SIGNAL];

static void     ctk_scale_set_property            (GObject        *object,
                                                   guint           prop_id,
                                                   const GValue   *value,
                                                   GParamSpec     *pspec);
static void     ctk_scale_get_property            (GObject        *object,
                                                   guint           prop_id,
                                                   GValue         *value,
                                                   GParamSpec     *pspec);
static void     ctk_scale_get_preferred_width     (CtkWidget      *widget,
                                                   gint           *minimum,
                                                   gint           *natural);
static void     ctk_scale_get_preferred_height    (CtkWidget      *widget,
                                                   gint           *minimum,
                                                   gint           *natural);
static void     ctk_scale_get_range_border        (CtkRange       *range,
                                                   CtkBorder      *border);
static void     ctk_scale_get_range_size_request  (CtkRange       *range,
                                                   CtkOrientation  orientation,
                                                   gint           *minimum,
                                                   gint           *natural);
static void     ctk_scale_finalize                (GObject        *object);
static void     ctk_scale_value_style_changed     (CtkCssNode        *node,
                                                   CtkCssStyleChange *change,
                                                   CtkScale          *scale);
static void     ctk_scale_screen_changed          (CtkWidget      *widget,
                                                   GdkScreen      *old_screen);
static gboolean ctk_scale_draw                    (CtkWidget      *widget,
                                                   cairo_t        *cr);
static void     ctk_scale_real_get_layout_offsets (CtkScale       *scale,
                                                   gint           *x,
                                                   gint           *y);
static void     ctk_scale_buildable_interface_init   (CtkBuildableIface *iface);
static gboolean ctk_scale_buildable_custom_tag_start (CtkBuildable  *buildable,
                                                      CtkBuilder    *builder,
                                                      GObject       *child,
                                                      const gchar   *tagname,
                                                      GMarkupParser *parser,
                                                      gpointer      *data);
static void     ctk_scale_buildable_custom_finished  (CtkBuildable  *buildable,
                                                      CtkBuilder    *builder,
                                                      GObject       *child,
                                                      const gchar   *tagname,
                                                      gpointer       user_data);
static void     ctk_scale_clear_value_layout         (CtkScale      *scale);
static void     ctk_scale_clear_mark_layouts         (CtkScale      *scale);
static gchar  * ctk_scale_format_value               (CtkScale      *scale,
                                                      gdouble        value);


G_DEFINE_TYPE_WITH_CODE (CtkScale, ctk_scale, CTK_TYPE_RANGE,
                         G_ADD_PRIVATE (CtkScale)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_BUILDABLE,
                                                ctk_scale_buildable_interface_init))

static gint
compare_marks (gconstpointer a, gconstpointer b, gpointer data)
{
  gboolean inverted = GPOINTER_TO_INT (data);
  gint val;
  const CtkScaleMark *ma, *mb;

  val = inverted ? -1 : 1;

  ma = a; mb = b;

  return (ma->value > mb->value) ? val : ((ma->value < mb->value) ? -val : 0);
}

static void
ctk_scale_notify (GObject    *object,
                  GParamSpec *pspec)
{
  if (strcmp (pspec->name, "inverted") == 0)
    {
      CtkScale *scale = CTK_SCALE (object);
      CtkScaleMark *mark;
      GSList *m;
      gint i, n;
      gdouble *values;

      scale->priv->marks = g_slist_sort_with_data (scale->priv->marks,
                                                   compare_marks,
                                                   GINT_TO_POINTER (ctk_range_get_inverted (CTK_RANGE (scale))));

      n = g_slist_length (scale->priv->marks);
      values = g_new (gdouble, n);
      for (m = scale->priv->marks, i = 0; m; m = m->next, i++)
        {
          mark = m->data;
          values[i] = mark->value;
        }

      _ctk_range_set_stop_values (CTK_RANGE (scale), values, n);

      g_free (values);
    }

  if (G_OBJECT_CLASS (ctk_scale_parent_class)->notify)
    G_OBJECT_CLASS (ctk_scale_parent_class)->notify (object, pspec);
}

static void
ctk_scale_allocate_value (CtkScale      *scale,
                          CtkAllocation *out_clip)
{
  CtkScalePrivate *priv = scale->priv;
  CtkWidget *widget = CTK_WIDGET (scale);
  CtkRange *range = CTK_RANGE (widget);
  CtkCssGadget *range_gadget, *slider_gadget;
  CtkAllocation range_alloc, slider_alloc, value_alloc;

  range_gadget = ctk_range_get_gadget (range);
  ctk_css_gadget_get_margin_allocation (range_gadget, &range_alloc, NULL);

  slider_gadget = ctk_range_get_slider_gadget (range);
  ctk_css_gadget_get_border_allocation (slider_gadget, &slider_alloc, NULL);

  ctk_css_gadget_get_preferred_size (priv->value_gadget,
                                     CTK_ORIENTATION_HORIZONTAL, -1,
                                     &value_alloc.width, NULL,
                                     NULL, NULL);
  ctk_css_gadget_get_preferred_size (priv->value_gadget,
                                     CTK_ORIENTATION_VERTICAL, -1,
                                     &value_alloc.height, NULL,
                                     NULL, NULL);

  if (ctk_orientable_get_orientation (CTK_ORIENTABLE (range)) == CTK_ORIENTATION_HORIZONTAL)
    {
      switch (priv->value_pos)
        {
        case CTK_POS_LEFT:
          value_alloc.x = range_alloc.x;
          value_alloc.y = range_alloc.y + (range_alloc.height - value_alloc.height) / 2;
          break;

        case CTK_POS_RIGHT:
          value_alloc.x = range_alloc.x + range_alloc.width - value_alloc.width;
          value_alloc.y = range_alloc.y + (range_alloc.height - value_alloc.height) / 2;
          break;

        case CTK_POS_TOP:
          value_alloc.x = slider_alloc.x + (slider_alloc.width - value_alloc.width) / 2;
          value_alloc.x = CLAMP (value_alloc.x, range_alloc.x, range_alloc.x + range_alloc.width - value_alloc.width);
          value_alloc.y = range_alloc.y;
          break;

        case CTK_POS_BOTTOM:
          value_alloc.x = slider_alloc.x + (slider_alloc.width - value_alloc.width) / 2;
          value_alloc.x = CLAMP (value_alloc.x, range_alloc.x, range_alloc.x + range_alloc.width - value_alloc.width);
          value_alloc.y = range_alloc.y + range_alloc.height - value_alloc.height;
          break;

        default:
          g_return_if_reached ();
          break;
        }
    }
  else
    {
      switch (priv->value_pos)
        {
        case CTK_POS_LEFT:
          value_alloc.x = range_alloc.x;
          value_alloc.y = slider_alloc.y + (slider_alloc.height - value_alloc.height) / 2;
          value_alloc.y = CLAMP (value_alloc.y, range_alloc.y, range_alloc.y + range_alloc.height - value_alloc.height);
          break;

        case CTK_POS_RIGHT:
          value_alloc.x = range_alloc.x + range_alloc.width - value_alloc.width;
          value_alloc.y = slider_alloc.y + (slider_alloc.height - value_alloc.height) / 2;
          value_alloc.y = CLAMP (value_alloc.y, range_alloc.y, range_alloc.y + range_alloc.height - value_alloc.height);
          break;

        case CTK_POS_TOP:
          value_alloc.x = range_alloc.x + (range_alloc.width - value_alloc.width) / 2;
          value_alloc.y = range_alloc.y;
          break;

        case CTK_POS_BOTTOM:
          value_alloc.x = range_alloc.x + (range_alloc.width - value_alloc.width) / 2;
          value_alloc.y = range_alloc.y + range_alloc.height - value_alloc.height;
          break;

        default:
          g_return_if_reached ();
        }
    }

  ctk_css_gadget_allocate (priv->value_gadget,
                           &value_alloc, -1,
                           out_clip);
}

static void
ctk_scale_allocate_mark (CtkCssGadget        *gadget,
                         const CtkAllocation *allocation,
                         int                  baseline,
                         CtkAllocation       *out_clip,
                         gpointer             user_data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkScaleMark *mark = user_data;
  CtkAllocation indicator_alloc, widget_alloc;
  int indicator_width, indicator_height;
  CtkOrientation orientation;

  orientation = ctk_orientable_get_orientation (CTK_ORIENTABLE (widget));
  ctk_widget_get_allocation (widget, &widget_alloc);
  ctk_css_gadget_get_preferred_size (mark->indicator_gadget,
                                     CTK_ORIENTATION_HORIZONTAL, -1,
                                     &indicator_width, NULL,
                                     NULL, NULL);
  ctk_css_gadget_get_preferred_size (mark->indicator_gadget,
                                     CTK_ORIENTATION_VERTICAL, -1,
                                     &indicator_height, NULL,
                                     NULL, NULL);

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      indicator_alloc.x = mark->stop_position + widget_alloc.x - indicator_width / 2;
      if (mark->position == CTK_POS_TOP)
        indicator_alloc.y = allocation->y + allocation->height - indicator_height;
      else
        indicator_alloc.y = allocation->y;
      indicator_alloc.width = indicator_width;
      indicator_alloc.height = indicator_height;
    }
  else
    {
      if (mark->position == CTK_POS_TOP)
        indicator_alloc.x = allocation->x + allocation->width - indicator_width;
      else
        indicator_alloc.x = allocation->x;
      indicator_alloc.y = mark->stop_position + widget_alloc.y - indicator_height / 2;
      indicator_alloc.width = indicator_width;
      indicator_alloc.height = indicator_height;
    }

  ctk_css_gadget_allocate (mark->indicator_gadget,
                           &indicator_alloc, baseline,
                           out_clip);

  if (mark->label_gadget)
    {
      CtkAllocation label_alloc, label_clip;

      label_alloc = *allocation;

      if (orientation == CTK_ORIENTATION_HORIZONTAL)
        {
          label_alloc.height = allocation->height - indicator_alloc.height;
          if (mark->position == CTK_POS_BOTTOM)
            label_alloc.y = indicator_alloc.y + indicator_alloc.height;
        }
      else
        {
          label_alloc.width = allocation->width - indicator_alloc.width;
          if (mark->position == CTK_POS_BOTTOM)
            label_alloc.x = indicator_alloc.x + indicator_alloc.width;
        }

      ctk_css_gadget_allocate (mark->label_gadget,
                               &label_alloc, baseline,
                               &label_clip);
      gdk_rectangle_union (out_clip, &label_clip, out_clip);
    }
}

static gint
find_next_pos (CtkWidget       *widget,
               GSList          *list,
               gint            *marks,
               CtkPositionType  pos)
{
  CtkAllocation allocation;
  GSList *m;
  gint i;

  for (m = list->next, i = 1; m; m = m->next, i++)
    {
      CtkScaleMark *mark = m->data;

      if (mark->position == pos)
        return marks[i];
    }

  ctk_widget_get_allocation (widget, &allocation);
  if (ctk_orientable_get_orientation (CTK_ORIENTABLE (widget)) == CTK_ORIENTATION_HORIZONTAL)
    return allocation.width;
  else
    return allocation.height;
}

static void
ctk_scale_allocate_marks (CtkCssGadget        *gadget,
                          const CtkAllocation *allocation,
                          int                  baseline,
                          CtkAllocation       *out_clip,
                          gpointer             data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkScale *scale = CTK_SCALE (widget);
  CtkScalePrivate *priv = scale->priv;
  CtkOrientation orientation;
  int *marks;
  int min_pos_before, min_pos_after;
  int min_sep = 4;
  int i;
  int min_pos, max_pos;
  GSList *m;
  CtkAllocation widget_alloc;

  orientation = ctk_orientable_get_orientation (CTK_ORIENTABLE (scale));
  _ctk_range_get_stop_positions (CTK_RANGE (scale), &marks);
  ctk_widget_get_allocation (widget, &widget_alloc);

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    min_pos_before = min_pos_after = widget_alloc.x;
  else
    min_pos_before = min_pos_after = widget_alloc.y;

  for (m = priv->marks, i = 0; m; m = m->next, i++)
    {
      CtkScaleMark *mark = m->data;
      CtkAllocation mark_alloc, mark_clip;
      int mark_size;

      if ((mark->position == CTK_POS_TOP && gadget == priv->bottom_marks_gadget) ||
          (mark->position == CTK_POS_BOTTOM && gadget == priv->top_marks_gadget))
        continue;

      ctk_css_gadget_get_preferred_size (mark->gadget,
                                         orientation, -1,
                                         &mark_size, NULL,
                                         NULL, NULL);
      mark->stop_position = marks[i];

      if (orientation == CTK_ORIENTATION_HORIZONTAL)
        {
          mark_alloc.x = mark->stop_position + widget_alloc.x;
          mark_alloc.y = allocation->y;
          mark_alloc.width = mark_size;
          mark_alloc.height = allocation->height;

          if (mark->position == CTK_POS_TOP)
            {
              min_pos = min_pos_before;
              max_pos = find_next_pos (widget, m, marks + i, CTK_POS_TOP) - min_sep + widget_alloc.x;
            }
          else
            {
              min_pos = min_pos_after;
              max_pos = find_next_pos (widget, m, marks + i, CTK_POS_BOTTOM) - min_sep + widget_alloc.x;
            }

          mark_alloc.x -= mark_size / 2;

          if (mark_alloc.x < min_pos)
            mark_alloc.x = min_pos;
          if (mark_alloc.x + mark_size > max_pos)
            mark_alloc.x = max_pos - mark_size;
          if (mark_alloc.x < 0)
            mark_alloc.x = 0;

          if (mark->position == CTK_POS_TOP)
            min_pos_before = mark_alloc.x + mark_size + min_sep;
          else
            min_pos_after = mark_alloc.x + mark_size + min_sep;
        }
      else
        {
          mark_alloc.x = allocation->x;
          mark_alloc.y = mark->stop_position + widget_alloc.y;
          mark_alloc.width = allocation->width;
          mark_alloc.height = mark_size;

          if (mark->position == CTK_POS_TOP)
            {
              min_pos = min_pos_before;
              max_pos = find_next_pos (widget, m, marks + i, CTK_POS_TOP) - min_sep + widget_alloc.y;
            }
          else
            {
              min_pos = min_pos_after;
              max_pos = find_next_pos (widget, m, marks + i, CTK_POS_BOTTOM) - min_sep + widget_alloc.y;
            }

          mark_alloc.y -= mark_size / 2;

          if (mark_alloc.y < min_pos)
            mark_alloc.y = min_pos;
          if (mark_alloc.y + mark_size > max_pos)
            mark_alloc.y = max_pos - mark_size;
          if (mark_alloc.y < 0)
            mark_alloc.y = 0;

          if (mark->position == CTK_POS_TOP)
            min_pos_before = mark_alloc.y + mark_size + min_sep;
          else
            min_pos_after = mark_alloc.y + mark_size + min_sep;
        }

      ctk_css_gadget_allocate (mark->gadget, &mark_alloc, baseline, &mark_clip);
      gdk_rectangle_union (out_clip, &mark_clip, out_clip);
    }

  g_free (marks);
}

static void
ctk_scale_size_allocate (CtkWidget     *widget,
                         CtkAllocation *allocation)
{
  CtkScale *scale = CTK_SCALE (widget);
  CtkScalePrivate *priv = scale->priv;
  CtkAllocation clip, marks_clip, range_rect, marks_rect;
  CtkOrientation orientation;

  CTK_WIDGET_CLASS (ctk_scale_parent_class)->size_allocate (widget, allocation);

  ctk_widget_get_clip (widget, &clip);
  orientation = ctk_orientable_get_orientation (CTK_ORIENTABLE (widget));
  ctk_range_get_range_rect (CTK_RANGE (scale), &range_rect);

  range_rect.x += allocation->x;
  range_rect.y += allocation->y;

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      int marks_height = 0;

      if (priv->top_marks_gadget)
        {
          ctk_css_gadget_get_preferred_size (priv->top_marks_gadget,
                                             CTK_ORIENTATION_VERTICAL, -1,
                                             &marks_height, NULL,
                                             NULL, NULL);
          marks_rect = range_rect;
          marks_rect.y -= marks_height;
          marks_rect.height = marks_height;
          ctk_css_gadget_allocate (priv->top_marks_gadget,
                                   &marks_rect,
                                   -1,
                                   &marks_clip);
          gdk_rectangle_union (&clip, &marks_clip, &clip);
        }

      if (priv->bottom_marks_gadget)
        {
          ctk_css_gadget_get_preferred_size (priv->bottom_marks_gadget,
                                             CTK_ORIENTATION_VERTICAL, -1,
                                             &marks_height, NULL,
                                             NULL, NULL);
          marks_rect = range_rect;
          marks_rect.y += range_rect.height;
          marks_rect.height = marks_height;
          ctk_css_gadget_allocate (priv->bottom_marks_gadget,
                                   &marks_rect,
                                   -1,
                                   &marks_clip);
          gdk_rectangle_union (&clip, &marks_clip, &clip);
        }
    }
  else
    {
      int marks_width = 0;

      if (priv->top_marks_gadget)
        {
          ctk_css_gadget_get_preferred_size (priv->top_marks_gadget,
                                             CTK_ORIENTATION_HORIZONTAL, -1,
                                             &marks_width, NULL,
                                             NULL, NULL);
          marks_rect = range_rect;
          marks_rect.x -= marks_width;
          marks_rect.width = marks_width;
          ctk_css_gadget_allocate (priv->top_marks_gadget,
                                   &marks_rect,
                                   -1,
                                   &marks_clip);
          gdk_rectangle_union (&clip, &marks_clip, &clip);
        }

      if (priv->bottom_marks_gadget)
        {
          ctk_css_gadget_get_preferred_size (priv->bottom_marks_gadget,
                                             CTK_ORIENTATION_HORIZONTAL, -1,
                                             &marks_width, NULL,
                                             NULL, NULL);
          marks_rect = range_rect;
          marks_rect.x += range_rect.width;
          marks_rect.width = marks_width;
          ctk_css_gadget_allocate (priv->bottom_marks_gadget,
                                   &marks_rect,
                                   -1,
                                   &marks_clip);
          gdk_rectangle_union (&clip, &marks_clip, &clip);
        }
    }

  if (priv->value_gadget)
    {
      CtkAllocation value_clip;

      ctk_scale_allocate_value (scale, &value_clip);
      gdk_rectangle_union (&clip, &value_clip, &clip);
    }

  ctk_widget_set_clip (widget, &clip);
}

#define add_slider_binding(binding_set, keyval, mask, scroll)              \
  ctk_binding_entry_add_signal (binding_set, keyval, mask,                 \
                                I_("move-slider"), 1, \
                                CTK_TYPE_SCROLL_TYPE, scroll)

static void
ctk_scale_class_init (CtkScaleClass *class)
{
  GObjectClass   *gobject_class;
  CtkWidgetClass *widget_class;
  CtkRangeClass  *range_class;
  CtkBindingSet  *binding_set;
  
  gobject_class = G_OBJECT_CLASS (class);
  range_class = (CtkRangeClass*) class;
  widget_class = (CtkWidgetClass*) class;
  
  gobject_class->set_property = ctk_scale_set_property;
  gobject_class->get_property = ctk_scale_get_property;
  gobject_class->notify = ctk_scale_notify;
  gobject_class->finalize = ctk_scale_finalize;

  widget_class->screen_changed = ctk_scale_screen_changed;
  widget_class->draw = ctk_scale_draw;
  widget_class->size_allocate = ctk_scale_size_allocate;
  widget_class->get_preferred_width = ctk_scale_get_preferred_width;
  widget_class->get_preferred_height = ctk_scale_get_preferred_height;

  range_class->get_range_border = ctk_scale_get_range_border;
  range_class->get_range_size_request = ctk_scale_get_range_size_request;

  class->get_layout_offsets = ctk_scale_real_get_layout_offsets;

  /**
   * CtkScale::format-value:
   * @scale: the object which received the signal
   * @value: the value to format
   *
   * Signal which allows you to change how the scale value is displayed.
   * Connect a signal handler which returns an allocated string representing 
   * @value. That string will then be used to display the scale's value.
   *
   * If no user-provided handlers are installed, the value will be displayed on
   * its own, rounded according to the value of the #CtkScale:digits property.
   *
   * Here's an example signal handler which displays a value 1.0 as
   * with "-->1.0<--".
   * |[<!-- language="C" -->
   * static gchar*
   * format_value_callback (CtkScale *scale,
   *                        gdouble   value)
   * {
   *   return g_strdup_printf ("-->\%0.*g<--",
   *                           ctk_scale_get_digits (scale), value);
   *  }
   * ]|
   *
   * Returns: allocated string representing @value
   */
  signals[FORMAT_VALUE] =
    g_signal_new (I_("format-value"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkScaleClass, format_value),
                  _ctk_single_string_accumulator, NULL,
                  _ctk_marshal_STRING__DOUBLE,
                  G_TYPE_STRING, 1,
                  G_TYPE_DOUBLE);

  properties[PROP_DIGITS] =
      g_param_spec_int ("digits",
                        P_("Digits"),
                        P_("The number of decimal places that are displayed in the value"),
                        -1, MAX_DIGITS,
                        1,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  properties[PROP_DRAW_VALUE] =
      g_param_spec_boolean ("draw-value",
                            P_("Draw Value"),
                            P_("Whether the current value is displayed as a string next to the slider"),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  properties[PROP_HAS_ORIGIN] =
      g_param_spec_boolean ("has-origin",
                            P_("Has Origin"),
                            P_("Whether the scale has an origin"),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  properties[PROP_VALUE_POS] =
      g_param_spec_enum ("value-pos",
                         P_("Value Position"),
                         P_("The position in which the current value is displayed"),
                         CTK_TYPE_POSITION_TYPE,
                         CTK_POS_TOP,
                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class, LAST_PROP, properties);

  /**
   * CtkScale:slider-length:
   *
   * Length of scale's slider.
   *
   * Deprecated: 3.20: Use min-height/min-width CSS properties on the slider
   *   element instead. The value of this style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("slider-length",
                                                             P_("Slider Length"),
                                                             P_("Length of scale's slider"),
                                                             0, G_MAXINT, 31,
                                                             CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  /**
   * CtkScale:value-spacing:
   *
   * Space between value text and the slider/trough area.
   *
   * Deprecated: 3.20: Use min-height/min-width CSS properties on the value
   *   element instead. The value of this style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("value-spacing",
							     P_("Value spacing"),
							     P_("Space between value text and the slider/trough area"),
							     0,
							     G_MAXINT,
							     2,
							     CTK_PARAM_READABLE|G_PARAM_DEPRECATED));
  
  /* All bindings (even arrow keys) are on both h/v scale, because
   * blind users etc. don't care about scale orientation.
   */
  
  binding_set = ctk_binding_set_by_class (class);

  add_slider_binding (binding_set, GDK_KEY_Left, 0,
                      CTK_SCROLL_STEP_LEFT);

  add_slider_binding (binding_set, GDK_KEY_Left, GDK_CONTROL_MASK,
                      CTK_SCROLL_PAGE_LEFT);

  add_slider_binding (binding_set, GDK_KEY_KP_Left, 0,
                      CTK_SCROLL_STEP_LEFT);

  add_slider_binding (binding_set, GDK_KEY_KP_Left, GDK_CONTROL_MASK,
                      CTK_SCROLL_PAGE_LEFT);

  add_slider_binding (binding_set, GDK_KEY_Right, 0,
                      CTK_SCROLL_STEP_RIGHT);

  add_slider_binding (binding_set, GDK_KEY_Right, GDK_CONTROL_MASK,
                      CTK_SCROLL_PAGE_RIGHT);

  add_slider_binding (binding_set, GDK_KEY_KP_Right, 0,
                      CTK_SCROLL_STEP_RIGHT);

  add_slider_binding (binding_set, GDK_KEY_KP_Right, GDK_CONTROL_MASK,
                      CTK_SCROLL_PAGE_RIGHT);

  add_slider_binding (binding_set, GDK_KEY_Up, 0,
                      CTK_SCROLL_STEP_UP);

  add_slider_binding (binding_set, GDK_KEY_Up, GDK_CONTROL_MASK,
                      CTK_SCROLL_PAGE_UP);

  add_slider_binding (binding_set, GDK_KEY_KP_Up, 0,
                      CTK_SCROLL_STEP_UP);

  add_slider_binding (binding_set, GDK_KEY_KP_Up, GDK_CONTROL_MASK,
                      CTK_SCROLL_PAGE_UP);

  add_slider_binding (binding_set, GDK_KEY_Down, 0,
                      CTK_SCROLL_STEP_DOWN);

  add_slider_binding (binding_set, GDK_KEY_Down, GDK_CONTROL_MASK,
                      CTK_SCROLL_PAGE_DOWN);

  add_slider_binding (binding_set, GDK_KEY_KP_Down, 0,
                      CTK_SCROLL_STEP_DOWN);

  add_slider_binding (binding_set, GDK_KEY_KP_Down, GDK_CONTROL_MASK,
                      CTK_SCROLL_PAGE_DOWN);
   
  add_slider_binding (binding_set, GDK_KEY_Page_Up, GDK_CONTROL_MASK,
                      CTK_SCROLL_PAGE_LEFT);

  add_slider_binding (binding_set, GDK_KEY_KP_Page_Up, GDK_CONTROL_MASK,
                      CTK_SCROLL_PAGE_LEFT);  

  add_slider_binding (binding_set, GDK_KEY_Page_Up, 0,
                      CTK_SCROLL_PAGE_UP);

  add_slider_binding (binding_set, GDK_KEY_KP_Page_Up, 0,
                      CTK_SCROLL_PAGE_UP);
  
  add_slider_binding (binding_set, GDK_KEY_Page_Down, GDK_CONTROL_MASK,
                      CTK_SCROLL_PAGE_RIGHT);

  add_slider_binding (binding_set, GDK_KEY_KP_Page_Down, GDK_CONTROL_MASK,
                      CTK_SCROLL_PAGE_RIGHT);

  add_slider_binding (binding_set, GDK_KEY_Page_Down, 0,
                      CTK_SCROLL_PAGE_DOWN);

  add_slider_binding (binding_set, GDK_KEY_KP_Page_Down, 0,
                      CTK_SCROLL_PAGE_DOWN);

  /* Logical bindings (vs. visual bindings above) */

  add_slider_binding (binding_set, GDK_KEY_plus, 0,
                      CTK_SCROLL_STEP_FORWARD);  

  add_slider_binding (binding_set, GDK_KEY_minus, 0,
                      CTK_SCROLL_STEP_BACKWARD);  

  add_slider_binding (binding_set, GDK_KEY_plus, GDK_CONTROL_MASK,
                      CTK_SCROLL_PAGE_FORWARD);  

  add_slider_binding (binding_set, GDK_KEY_minus, GDK_CONTROL_MASK,
                      CTK_SCROLL_PAGE_BACKWARD);


  add_slider_binding (binding_set, GDK_KEY_KP_Add, 0,
                      CTK_SCROLL_STEP_FORWARD);  

  add_slider_binding (binding_set, GDK_KEY_KP_Subtract, 0,
                      CTK_SCROLL_STEP_BACKWARD);  

  add_slider_binding (binding_set, GDK_KEY_KP_Add, GDK_CONTROL_MASK,
                      CTK_SCROLL_PAGE_FORWARD);  

  add_slider_binding (binding_set, GDK_KEY_KP_Subtract, GDK_CONTROL_MASK,
                      CTK_SCROLL_PAGE_BACKWARD);
  
  
  add_slider_binding (binding_set, GDK_KEY_Home, 0,
                      CTK_SCROLL_START);

  add_slider_binding (binding_set, GDK_KEY_KP_Home, 0,
                      CTK_SCROLL_START);

  add_slider_binding (binding_set, GDK_KEY_End, 0,
                      CTK_SCROLL_END);

  add_slider_binding (binding_set, GDK_KEY_KP_End, 0,
                      CTK_SCROLL_END);

  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_SCALE_ACCESSIBLE);
  ctk_widget_class_set_css_name (widget_class, "scale");
}

static void
ctk_scale_init (CtkScale *scale)
{
  CtkScalePrivate *priv;
  CtkRange *range = CTK_RANGE (scale);

  scale->priv = ctk_scale_get_instance_private (scale);
  priv = scale->priv;

  priv->value_pos = CTK_POS_TOP;
  priv->digits = 1;

  ctk_widget_set_can_focus (CTK_WIDGET (scale), TRUE);

  ctk_range_set_slider_size_fixed (range, TRUE);
  ctk_range_set_slider_use_min_size (range, TRUE);

  _ctk_range_set_has_origin (range, TRUE);

  ctk_scale_set_draw_value (scale, TRUE);
  ctk_range_set_round_digits (range, priv->digits);

  ctk_range_set_flippable (range, TRUE);
}

static void
ctk_scale_set_property (GObject      *object,
			guint         prop_id,
			const GValue *value,
			GParamSpec   *pspec)
{
  CtkScale *scale;

  scale = CTK_SCALE (object);

  switch (prop_id)
    {
    case PROP_DIGITS:
      ctk_scale_set_digits (scale, g_value_get_int (value));
      break;
    case PROP_DRAW_VALUE:
      ctk_scale_set_draw_value (scale, g_value_get_boolean (value));
      break;
    case PROP_HAS_ORIGIN:
      ctk_scale_set_has_origin (scale, g_value_get_boolean (value));
      break;
    case PROP_VALUE_POS:
      ctk_scale_set_value_pos (scale, g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_scale_get_property (GObject      *object,
			guint         prop_id,
			GValue       *value,
			GParamSpec   *pspec)
{
  CtkScale *scale = CTK_SCALE (object);
  CtkScalePrivate *priv = scale->priv;

  switch (prop_id)
    {
    case PROP_DIGITS:
      g_value_set_int (value, priv->digits);
      break;
    case PROP_DRAW_VALUE:
      g_value_set_boolean (value, priv->draw_value);
      break;
    case PROP_HAS_ORIGIN:
      g_value_set_boolean (value, ctk_scale_get_has_origin (scale));
      break;
    case PROP_VALUE_POS:
      g_value_set_enum (value, priv->value_pos);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * ctk_scale_new:
 * @orientation: the scale’s orientation.
 * @adjustment: (nullable): the #CtkAdjustment which sets the range
 *              of the scale, or %NULL to create a new adjustment.
 *
 * Creates a new #CtkScale.
 *
 * Returns: a new #CtkScale
 *
 * Since: 3.0
 **/
CtkWidget *
ctk_scale_new (CtkOrientation  orientation,
               CtkAdjustment  *adjustment)
{
  g_return_val_if_fail (adjustment == NULL || CTK_IS_ADJUSTMENT (adjustment),
                        NULL);

  return g_object_new (CTK_TYPE_SCALE,
                       "orientation", orientation,
                       "adjustment",  adjustment,
                       NULL);
}

/**
 * ctk_scale_new_with_range:
 * @orientation: the scale’s orientation.
 * @min: minimum value
 * @max: maximum value
 * @step: step increment (tick size) used with keyboard shortcuts
 *
 * Creates a new scale widget with the given orientation that lets the
 * user input a number between @min and @max (including @min and @max)
 * with the increment @step.  @step must be nonzero; it’s the distance
 * the slider moves when using the arrow keys to adjust the scale
 * value.
 *
 * Note that the way in which the precision is derived works best if @step
 * is a power of ten. If the resulting precision is not suitable for your
 * needs, use ctk_scale_set_digits() to correct it.
 *
 * Returns: a new #CtkScale
 *
 * Since: 3.0
 */
CtkWidget *
ctk_scale_new_with_range (CtkOrientation orientation,
                          gdouble        min,
                          gdouble        max,
                          gdouble        step)
{
  CtkAdjustment *adj;
  gint digits;

  g_return_val_if_fail (min < max, NULL);
  g_return_val_if_fail (step != 0.0, NULL);

  adj = ctk_adjustment_new (min, min, max, step, 10 * step, 0);

  if (fabs (step) >= 1.0 || step == 0.0)
    {
      digits = 0;
    }
  else
    {
      digits = abs ((gint) floor (log10 (fabs (step))));
      if (digits > 5)
        digits = 5;
    }

  return g_object_new (CTK_TYPE_SCALE,
                       "orientation", orientation,
                       "adjustment",  adj,
                       "digits",      digits,
                       NULL);
}

/**
 * ctk_scale_set_digits:
 * @scale: a #CtkScale
 * @digits: the number of decimal places to display,
 *     e.g. use 1 to display 1.0, 2 to display 1.00, etc
 *
 * Sets the number of decimal places that are displayed in the value. Also
 * causes the value of the adjustment to be rounded to this number of digits,
 * so the retrieved value matches the displayed one, if #CtkScale:draw-value is
 * %TRUE when the value changes. If you want to enforce rounding the value when
 * #CtkScale:draw-value is %FALSE, you can set #CtkRange:round-digits instead.
 *
 * Note that rounding to a small number of digits can interfere with
 * the smooth autoscrolling that is built into #CtkScale. As an alternative,
 * you can use the #CtkScale::format-value signal to format the displayed
 * value yourself.
 */
void
ctk_scale_set_digits (CtkScale *scale,
		      gint      digits)
{
  CtkScalePrivate *priv;
  CtkRange *range;

  g_return_if_fail (CTK_IS_SCALE (scale));

  priv = scale->priv;
  range = CTK_RANGE (scale);
  
  digits = CLAMP (digits, -1, MAX_DIGITS);

  if (priv->digits != digits)
    {
      priv->digits = digits;
      if (priv->draw_value)
        ctk_range_set_round_digits (range, digits);

      ctk_scale_clear_value_layout (scale);
      ctk_widget_queue_resize (CTK_WIDGET (scale));

      g_object_notify_by_pspec (G_OBJECT (scale), properties[PROP_DIGITS]);
    }
}

/**
 * ctk_scale_get_digits:
 * @scale: a #CtkScale
 *
 * Gets the number of decimal places that are displayed in the value.
 *
 * Returns: the number of decimal places that are displayed
 */
gint
ctk_scale_get_digits (CtkScale *scale)
{
  g_return_val_if_fail (CTK_IS_SCALE (scale), -1);

  return scale->priv->digits;
}

static gboolean
ctk_scale_render_value (CtkCssGadget *gadget,
                        cairo_t      *cr,
                        int           x,
                        int           y,
                        int           width,
                        int           height,
                        gpointer      user_data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkScale *scale = CTK_SCALE (widget);
  CtkStyleContext *context;
  PangoLayout *layout;

  context = ctk_widget_get_style_context (widget);
  ctk_style_context_save_to_node (context, ctk_css_gadget_get_node (gadget));

  layout = ctk_scale_get_layout (scale);
  ctk_render_layout (context, cr, x, y, layout);

  ctk_style_context_restore (context);

  return FALSE;
}

static void
ctk_css_node_update_layout_attributes (CtkCssNode  *node,
                                       PangoLayout *layout)
{
  CtkCssStyle *style;
  PangoAttrList *attrs;
  PangoFontDescription *desc;

  style = ctk_css_node_get_style (node);

  attrs = ctk_css_style_get_pango_attributes (style);
  desc = ctk_css_style_get_pango_font (style);

  pango_layout_set_attributes (layout, attrs);
  pango_layout_set_font_description (layout, desc);

  if (attrs)
    pango_attr_list_unref (attrs);
  pango_font_description_free (desc);
}

static void
ctk_scale_measure_value (CtkCssGadget   *gadget,
                         CtkOrientation  orientation,
                         gint            for_size,
                         gint           *minimum,
                         gint           *natural,
                         gint           *minimum_baseline,
                         gint           *natural_baseline,
                         gpointer        user_data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkScale *scale = CTK_SCALE (widget);
  CtkScalePrivate *priv = scale->priv;
  int width, height;

  width = height = 0;

  if (priv->draw_value)
    {
      CtkAdjustment *adjustment;
      PangoLayout *layout;
      PangoRectangle logical_rect;
      gchar *txt;

      layout = ctk_widget_create_pango_layout (widget, NULL);
      ctk_css_node_update_layout_attributes (ctk_css_gadget_get_node (priv->value_gadget), layout);

      adjustment = ctk_range_get_adjustment (CTK_RANGE (scale));

      txt = ctk_scale_format_value (scale, ctk_adjustment_get_lower (adjustment));
      pango_layout_set_text (layout, txt, -1);
      g_free (txt);

      pango_layout_get_pixel_extents (layout, NULL, &logical_rect);

      width = logical_rect.width;
      height = logical_rect.height;

      txt = ctk_scale_format_value (scale, ctk_adjustment_get_upper (adjustment));
      pango_layout_set_text (layout, txt, -1);
      g_free (txt);

      pango_layout_get_pixel_extents (layout, NULL, &logical_rect);

      width = MAX (width, logical_rect.width);
      height = MAX (height, logical_rect.height);

      g_object_unref (layout);
    }

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    *minimum = *natural = width;
  else
    *minimum = *natural = height;
}

static void
update_value_position (CtkScale *scale)
{
  CtkScalePrivate *priv = scale->priv;

  if (!priv->value_gadget)
    return;

  if (priv->value_pos == CTK_POS_TOP || priv->value_pos == CTK_POS_LEFT)
    {
      ctk_css_gadget_remove_class (priv->value_gadget, CTK_STYLE_CLASS_BOTTOM);
      ctk_css_gadget_add_class (priv->value_gadget, CTK_STYLE_CLASS_TOP);
    }
  else
    {
      ctk_css_gadget_remove_class (priv->value_gadget, CTK_STYLE_CLASS_TOP);
      ctk_css_gadget_add_class (priv->value_gadget, CTK_STYLE_CLASS_BOTTOM);
    }
}

/**
 * ctk_scale_set_draw_value:
 * @scale: a #CtkScale
 * @draw_value: %TRUE to draw the value
 * 
 * Specifies whether the current value is displayed as a string next 
 * to the slider.
 */
void
ctk_scale_set_draw_value (CtkScale *scale,
			  gboolean  draw_value)
{
  CtkScalePrivate *priv;
  CtkWidget *widget;

  g_return_if_fail (CTK_IS_SCALE (scale));

  priv = scale->priv;
  widget = CTK_WIDGET (scale);

  draw_value = draw_value != FALSE;

  if (priv->draw_value != draw_value)
    {
      priv->draw_value = draw_value;
      if (draw_value)
        {
          CtkCssNode *widget_node;

          widget_node = ctk_widget_get_css_node (widget);
          priv->value_gadget = ctk_css_custom_gadget_new ("value",
                                                          widget, NULL, NULL,
                                                          ctk_scale_measure_value,
                                                          NULL,
                                                          ctk_scale_render_value,
                                                          NULL, NULL);
          g_signal_connect (ctk_css_gadget_get_node (priv->value_gadget), "style-changed",
                            G_CALLBACK (ctk_scale_value_style_changed), scale);

          if (priv->value_pos == CTK_POS_TOP || priv->value_pos == CTK_POS_LEFT)
            ctk_css_node_insert_after (widget_node, ctk_css_gadget_get_node (priv->value_gadget), NULL);
          else
            ctk_css_node_insert_before (widget_node, ctk_css_gadget_get_node (priv->value_gadget), NULL);

          ctk_range_set_round_digits (CTK_RANGE (scale), priv->digits);
          update_value_position (scale);
        }
      else
        {
          if (priv->value_gadget)
            ctk_css_node_set_parent (ctk_css_gadget_get_node (priv->value_gadget), NULL);
          g_clear_object (&priv->value_gadget);

          ctk_range_set_round_digits (CTK_RANGE (scale), -1);
        }

      ctk_scale_clear_value_layout (scale);

      ctk_widget_queue_resize (widget);

      g_object_notify_by_pspec (G_OBJECT (scale), properties[PROP_DRAW_VALUE]);
    }
}

/**
 * ctk_scale_get_draw_value:
 * @scale: a #CtkScale
 *
 * Returns whether the current value is displayed as a string 
 * next to the slider.
 *
 * Returns: whether the current value is displayed as a string
 */
gboolean
ctk_scale_get_draw_value (CtkScale *scale)
{
  g_return_val_if_fail (CTK_IS_SCALE (scale), FALSE);

  return scale->priv->draw_value;
}

/**
 * ctk_scale_set_has_origin:
 * @scale: a #CtkScale
 * @has_origin: %TRUE if the scale has an origin
 * 
 * If #CtkScale:has-origin is set to %TRUE (the default), the scale will
 * highlight the part of the trough between the origin (bottom or left side)
 * and the current value.
 *
 * Since: 3.4
 */
void
ctk_scale_set_has_origin (CtkScale *scale,
                          gboolean  has_origin)
{
  g_return_if_fail (CTK_IS_SCALE (scale));

  has_origin = has_origin != FALSE;

  if (_ctk_range_get_has_origin (CTK_RANGE (scale)) != has_origin)
    {
      _ctk_range_set_has_origin (CTK_RANGE (scale), has_origin);

      ctk_widget_queue_draw (CTK_WIDGET (scale));

      g_object_notify_by_pspec (G_OBJECT (scale), properties[PROP_HAS_ORIGIN]);
    }
}

/**
 * ctk_scale_get_has_origin:
 * @scale: a #CtkScale
 *
 * Returns whether the scale has an origin.
 *
 * Returns: %TRUE if the scale has an origin.
 * 
 * Since: 3.4
 */
gboolean
ctk_scale_get_has_origin (CtkScale *scale)
{
  g_return_val_if_fail (CTK_IS_SCALE (scale), FALSE);

  return _ctk_range_get_has_origin (CTK_RANGE (scale));
}

/**
 * ctk_scale_set_value_pos:
 * @scale: a #CtkScale
 * @pos: the position in which the current value is displayed
 * 
 * Sets the position in which the current value is displayed.
 */
void
ctk_scale_set_value_pos (CtkScale        *scale,
			 CtkPositionType  pos)
{
  CtkScalePrivate *priv;
  CtkWidget *widget;

  g_return_if_fail (CTK_IS_SCALE (scale));

  priv = scale->priv;

  if (priv->value_pos != pos)
    {
      priv->value_pos = pos;
      widget = CTK_WIDGET (scale);

      ctk_scale_clear_value_layout (scale);
      update_value_position (scale);

      if (ctk_widget_get_visible (widget) && ctk_widget_get_mapped (widget))
	ctk_widget_queue_resize (widget);

      g_object_notify_by_pspec (G_OBJECT (scale), properties[PROP_VALUE_POS]);
    }
}

/**
 * ctk_scale_get_value_pos:
 * @scale: a #CtkScale
 *
 * Gets the position in which the current value is displayed.
 *
 * Returns: the position in which the current value is displayed
 */
CtkPositionType
ctk_scale_get_value_pos (CtkScale *scale)
{
  g_return_val_if_fail (CTK_IS_SCALE (scale), 0);

  return scale->priv->value_pos;
}

static void
ctk_scale_get_range_border (CtkRange  *range,
                            CtkBorder *border)
{
  CtkScalePrivate *priv;
  CtkScale *scale;

  scale = CTK_SCALE (range);
  priv = scale->priv;

  border->left = 0;
  border->right = 0;
  border->top = 0;
  border->bottom = 0;

  if (priv->value_gadget)
    {
      int value_size;
      CtkOrientation value_orientation;

      if (priv->value_pos == CTK_POS_LEFT || priv->value_pos == CTK_POS_RIGHT)
        value_orientation = CTK_ORIENTATION_HORIZONTAL;
      else
        value_orientation = CTK_ORIENTATION_VERTICAL;

      ctk_css_gadget_get_preferred_size (priv->value_gadget,
                                         value_orientation, -1,
                                         &value_size, NULL,
                                         NULL, NULL);

      switch (priv->value_pos)
        {
        case CTK_POS_LEFT:
          border->left += value_size;
          break;
        case CTK_POS_RIGHT:
          border->right += value_size;
          break;
        case CTK_POS_TOP:
          border->top += value_size;
          break;
        case CTK_POS_BOTTOM:
          border->bottom += value_size;
          break;
        }
    }

  if (ctk_orientable_get_orientation (CTK_ORIENTABLE (range)) == CTK_ORIENTATION_HORIZONTAL)
    {
      int height;

      if (priv->top_marks_gadget)
        {
          ctk_css_gadget_get_preferred_size (priv->top_marks_gadget,
                                             CTK_ORIENTATION_VERTICAL, -1,
                                             &height, NULL,
                                             NULL, NULL);
          if (height > 0)
            border->top += height;
        }

      if (priv->bottom_marks_gadget)
        {
          ctk_css_gadget_get_preferred_size (priv->bottom_marks_gadget,
                                             CTK_ORIENTATION_VERTICAL, -1,
                                             &height, NULL,
                                             NULL, NULL);
          if (height > 0)
            border->bottom += height;
        }
    }
  else
    {
      int width;

      if (priv->top_marks_gadget)
        {
          ctk_css_gadget_get_preferred_size (priv->top_marks_gadget,
                                             CTK_ORIENTATION_HORIZONTAL, -1,
                                             &width, NULL,
                                             NULL, NULL);
          if (width > 0)
            border->left += width;
        }

      if (priv->bottom_marks_gadget)
        {
          ctk_css_gadget_get_preferred_size (priv->bottom_marks_gadget,
                                             CTK_ORIENTATION_HORIZONTAL, -1,
                                             &width, NULL,
                                             NULL, NULL);
          if (width > 0)
            border->right += width;
        }
    }
}

static void
ctk_scale_get_range_size_request (CtkRange       *range,
                                  CtkOrientation  orientation,
                                  gint           *minimum,
                                  gint           *natural)
{
  CtkScalePrivate *priv = CTK_SCALE (range)->priv;

  /* Ensure the range requests enough size for our value */
  if (priv->value_gadget)
    ctk_css_gadget_get_preferred_size (priv->value_gadget,
                                       orientation, -1,
                                       minimum, natural,
                                       NULL, NULL);
  else
    {
      *minimum = 0;
      *natural = 0;
    }
}

static void
ctk_scale_value_style_changed (CtkCssNode        *node,
                               CtkCssStyleChange *change,
                               CtkScale          *scale)
{
  if (change == NULL ||
      ctk_css_style_change_affects (change, CTK_CSS_AFFECTS_TEXT_ATTRS) ||
      ctk_css_style_change_affects (change, CTK_CSS_AFFECTS_FONT))
    {
      ctk_scale_clear_value_layout (scale);
      ctk_widget_queue_resize (CTK_WIDGET (scale));
    }
}

static void
ctk_scale_mark_style_changed (CtkCssNode        *node,
                              CtkCssStyleChange *change,
                              CtkScaleMark      *mark)
{
  if (change == NULL ||
      ctk_css_style_change_affects (change, CTK_CSS_AFFECTS_TEXT_ATTRS) ||
      ctk_css_style_change_affects (change, CTK_CSS_AFFECTS_FONT))
    {
      CtkCssNode *widget_node;
      CtkWidget *scale;

      g_clear_object (&mark->layout);

      widget_node = ctk_css_node_get_parent (ctk_css_node_get_parent (ctk_css_node_get_parent (node)));
      scale = ctk_css_widget_node_get_widget (CTK_CSS_WIDGET_NODE (widget_node));
      ctk_widget_queue_resize (CTK_WIDGET (scale));
    }
}

static void
ctk_scale_screen_changed (CtkWidget *widget,
                          GdkScreen *old_screen)
{
  ctk_scale_clear_value_layout (CTK_SCALE (widget));
  ctk_scale_clear_mark_layouts (CTK_SCALE (widget));
}

static void
ctk_scale_measure_mark_label (CtkCssGadget   *gadget,
                              CtkOrientation  orientation,
                              gint            for_size,
                              gint           *minimum,
                              gint           *natural,
                              gint           *minimum_baseline,
                              gint           *natural_baseline,
                              gpointer        user_data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkScaleMark *mark = user_data;
  PangoRectangle logical_rect;

  *minimum = *natural = 0;

  if (!mark->layout)
    {
      mark->layout = ctk_widget_create_pango_layout (widget, NULL);
      pango_layout_set_markup (mark->layout, mark->markup, -1);
      ctk_css_node_update_layout_attributes (ctk_css_gadget_get_node (gadget), mark->layout);
    }

  pango_layout_get_pixel_extents (mark->layout, NULL, &logical_rect);

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    *minimum = *natural = logical_rect.width;
  else
    *minimum = *natural = logical_rect.height;
}

static void
ctk_scale_measure_mark (CtkCssGadget   *gadget,
                        CtkOrientation  orientation,
                        gint            for_size,
                        gint           *minimum,
                        gint           *natural,
                        gint           *minimum_baseline,
                        gint           *natural_baseline,
                        gpointer        user_data)
{
  CtkScaleMark *mark = user_data;

  ctk_css_gadget_get_preferred_size (mark->indicator_gadget,
                                     orientation, -1,
                                     minimum, natural,
                                     NULL, NULL);

  if (mark->label_gadget)
    {
      int label_min, label_nat;

      ctk_css_gadget_get_preferred_size (mark->label_gadget,
                                         orientation, -1,
                                         &label_min, &label_nat,
                                         NULL, NULL);
      *minimum += label_min;
      *natural += label_nat;
    }
}

static void
ctk_scale_measure_marks (CtkCssGadget   *gadget,
                         CtkOrientation  orientation,
                         gint            for_size,
                         gint           *minimum,
                         gint           *natural,
                         gint           *minimum_baseline,
                         gint           *natural_baseline,
                         gpointer        user_data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkScale *scale = CTK_SCALE (widget);
  CtkScalePrivate *priv = scale->priv;
  CtkOrientation scale_orientation = ctk_orientable_get_orientation (CTK_ORIENTABLE (scale));
  GSList *m;

  *minimum = *natural = 0;

  for (m = priv->marks; m; m = m->next)
    {
      CtkScaleMark *mark = m->data;
      int mark_size;

      if ((mark->position == CTK_POS_TOP && gadget == priv->bottom_marks_gadget) ||
          (mark->position == CTK_POS_BOTTOM && gadget == priv->top_marks_gadget))
        continue;

      ctk_css_gadget_get_preferred_size (mark->gadget,
                                         orientation, -1,
                                         &mark_size, NULL,
                                         NULL, NULL);

      if (scale_orientation == orientation)
        {
          *minimum += mark_size;
          *natural += mark_size;
        }
      else
        {
          *minimum = MAX (*minimum, mark_size);
          *natural = MAX (*natural, mark_size);
        }
    }
}

static void
ctk_scale_get_preferred_width (CtkWidget *widget,
                               gint      *minimum,
                               gint      *natural)
{
  CtkScale *scale = CTK_SCALE (widget);
  CtkScalePrivate *priv = scale->priv;

  CTK_WIDGET_CLASS (ctk_scale_parent_class)->get_preferred_width (widget, minimum, natural);
  
  if (ctk_orientable_get_orientation (CTK_ORIENTABLE (widget)) == CTK_ORIENTATION_HORIZONTAL)
    {
      int top_marks_width = 0, bottom_marks_width = 0, marks_width;

      if (priv->top_marks_gadget)
        ctk_css_gadget_get_preferred_size (priv->top_marks_gadget,
                                           CTK_ORIENTATION_HORIZONTAL, -1,
                                           &top_marks_width, NULL,
                                           NULL, NULL);
      if (priv->bottom_marks_gadget)
        ctk_css_gadget_get_preferred_size (priv->bottom_marks_gadget,
                                           CTK_ORIENTATION_HORIZONTAL, -1,
                                           &bottom_marks_width, NULL,
                                           NULL, NULL);

      marks_width = MAX (top_marks_width, bottom_marks_width);

      *minimum = MAX (*minimum, marks_width);
      *natural = MAX (*natural, marks_width);
    }
}

static void
ctk_scale_get_preferred_height (CtkWidget *widget,
                                gint      *minimum,
                                gint      *natural)
{
  CtkScale *scale = CTK_SCALE (widget);
  CtkScalePrivate *priv = scale->priv;

  CTK_WIDGET_CLASS (ctk_scale_parent_class)->get_preferred_height (widget, minimum, natural);

  if (ctk_orientable_get_orientation (CTK_ORIENTABLE (widget)) == CTK_ORIENTATION_VERTICAL)
    {
      int top_marks_height = 0, bottom_marks_height = 0, marks_height;

      if (priv->top_marks_gadget)
        ctk_css_gadget_get_preferred_size (priv->top_marks_gadget,
                                           CTK_ORIENTATION_VERTICAL, -1,
                                           &top_marks_height, NULL,
                                           NULL, NULL);
      if (priv->bottom_marks_gadget)
        ctk_css_gadget_get_preferred_size (priv->bottom_marks_gadget,
                                           CTK_ORIENTATION_VERTICAL, -1,
                                           &bottom_marks_height, NULL,
                                           NULL, NULL);

      marks_height = MAX (top_marks_height, bottom_marks_height);

      *minimum = MAX (*minimum, marks_height);
      *natural = MAX (*natural, marks_height);
    }
}

static gboolean
ctk_scale_render_mark_indicator (CtkCssGadget *gadget,
                                 cairo_t      *cr,
                                 int           x,
                                 int           y,
                                 int           width,
                                 int           height,
                                 gpointer      user_data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkStyleContext *context;
  CtkOrientation orientation;

  orientation = ctk_orientable_get_orientation (CTK_ORIENTABLE (widget));
  context = ctk_widget_get_style_context (widget);
  ctk_style_context_save_to_node (context, ctk_css_gadget_get_node (gadget));

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    ctk_render_line (context, cr,
                     x + width / 2, y,
                     x + width / 2, y + height);
  else
    ctk_render_line (context, cr,
                     x, y + height / 2,
                     x + width, y + height / 2);

  ctk_style_context_restore (context);

  return FALSE;
}

static gboolean
ctk_scale_render_mark_label (CtkCssGadget *gadget,
                             cairo_t      *cr,
                             int           x,
                             int           y,
                             int           width,
                             int           height,
                             gpointer      user_data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkScaleMark *mark = user_data;
  CtkStyleContext *context;

  context = ctk_widget_get_style_context (widget);
  ctk_style_context_save_to_node (context, ctk_css_gadget_get_node (gadget));
  ctk_css_node_update_layout_attributes (ctk_css_gadget_get_node (gadget), mark->layout);

  ctk_render_layout (context, cr, x, y, mark->layout);

  ctk_style_context_restore (context);

  return FALSE;
}

static gboolean
ctk_scale_render_mark (CtkCssGadget *gadget,
                       cairo_t      *cr,
                       int           x,
                       int           y,
                       int           width,
                       int           height,
                       gpointer      user_data)
{
  CtkScaleMark *mark = user_data;

  ctk_css_gadget_draw (mark->indicator_gadget, cr);
  if (mark->label_gadget)
    ctk_css_gadget_draw (mark->label_gadget, cr);

  return FALSE;
}

static gboolean
ctk_scale_render_marks (CtkCssGadget *gadget,
                        cairo_t      *cr,
                        int           x,
                        int           y,
                        int           width,
                        int           height,
                        gpointer      user_data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkScale *scale = CTK_SCALE (widget);
  CtkScalePrivate *priv = scale->priv;
  GSList *m;

  for (m = priv->marks; m; m = m->next)
    {
      CtkScaleMark *mark = m->data;

      if ((mark->position == CTK_POS_TOP && gadget == priv->bottom_marks_gadget) ||
          (mark->position == CTK_POS_BOTTOM && gadget == priv->top_marks_gadget))
        continue;

      ctk_css_gadget_draw (mark->gadget, cr);
    }

  return FALSE;
}

static gboolean
ctk_scale_draw (CtkWidget *widget,
                cairo_t   *cr)
{
  CtkScale *scale = CTK_SCALE (widget);
  CtkScalePrivate *priv = scale->priv;

  if (priv->top_marks_gadget)
    ctk_css_gadget_draw (priv->top_marks_gadget, cr);
  if (priv->bottom_marks_gadget)
    ctk_css_gadget_draw (priv->bottom_marks_gadget, cr);

  CTK_WIDGET_CLASS (ctk_scale_parent_class)->draw (widget, cr);

  if (priv->value_gadget)
    ctk_css_gadget_draw (priv->value_gadget, cr);

  return FALSE;
}

static void
ctk_scale_real_get_layout_offsets (CtkScale *scale,
                                   gint     *x,
                                   gint     *y)
{
  CtkScalePrivate *priv = scale->priv;
  CtkAllocation value_alloc;

  if (!priv->value_gadget)
    {
      *x = 0;
      *y = 0;

      return;
    }

  ctk_css_gadget_get_content_allocation (priv->value_gadget, &value_alloc, NULL);

  *x = value_alloc.x;
  *y = value_alloc.y;
}

static gchar *
weed_out_neg_zero (gchar *str,
                   gint   digits)
{
  if (str[0] == '-')
    {
      gchar neg_zero[8];
      g_snprintf (neg_zero, 8, "%0.*f", digits, -0.0);
      if (strcmp (neg_zero, str) == 0)
        memmove (str, str + 1, strlen (str));
    }
  return str;
}

/*
 * Emits #CtkScale:format-value signal to format the value;
 * if no user signal handlers, falls back to a default format.
 *
 * Returns: formatted value
 */
static gchar *
ctk_scale_format_value (CtkScale *scale,
                        gdouble   value)
{
  gchar *fmt = NULL;

  g_signal_emit (scale, signals[FORMAT_VALUE], 0, value, &fmt);

  if (fmt)
    return fmt;
  else
    {
      fmt = g_strdup_printf ("%0.*f", scale->priv->digits, value);
      return weed_out_neg_zero (fmt, scale->priv->digits);
    }
}

static void
ctk_scale_finalize (GObject *object)
{
  CtkScale *scale = CTK_SCALE (object);
  CtkScalePrivate *priv = scale->priv;

  ctk_scale_clear_value_layout (scale);
  ctk_scale_clear_marks (scale);

  if (priv->value_gadget)
    ctk_css_node_set_parent (ctk_css_gadget_get_node (priv->value_gadget), NULL);
  g_clear_object (&priv->value_gadget);

  G_OBJECT_CLASS (ctk_scale_parent_class)->finalize (object);
}

/**
 * ctk_scale_get_layout:
 * @scale: A #CtkScale
 *
 * Gets the #PangoLayout used to display the scale. The returned
 * object is owned by the scale so does not need to be freed by
 * the caller.
 *
 * Returns: (transfer none) (nullable): the #PangoLayout for this scale,
 *     or %NULL if the #CtkScale:draw-value property is %FALSE.
 *
 * Since: 2.4
 */
PangoLayout *
ctk_scale_get_layout (CtkScale *scale)
{
  CtkScalePrivate *priv;
  gchar *txt;

  g_return_val_if_fail (CTK_IS_SCALE (scale), NULL);

  priv = scale->priv;

  if (!priv->layout && priv->draw_value)
    {
      int min_layout_width;

      priv->layout = ctk_widget_create_pango_layout (CTK_WIDGET (scale), NULL);
      ctk_css_node_update_layout_attributes (ctk_css_gadget_get_node (priv->value_gadget), priv->layout);

      ctk_css_gadget_get_preferred_size (priv->value_gadget,
                                         CTK_ORIENTATION_HORIZONTAL, -1,
                                         &min_layout_width, NULL,
                                         NULL, NULL);
      pango_layout_set_width (priv->layout, min_layout_width * PANGO_SCALE);

      if (priv->value_pos == CTK_POS_LEFT)
        pango_layout_set_alignment (priv->layout, PANGO_ALIGN_RIGHT);
      else if (priv->value_pos == CTK_POS_RIGHT)
        pango_layout_set_alignment (priv->layout, PANGO_ALIGN_LEFT);
      else
        pango_layout_set_alignment (priv->layout, PANGO_ALIGN_CENTER);
    }

  if (priv->draw_value)
    {
      txt = ctk_scale_format_value (scale,
                                    ctk_adjustment_get_value (ctk_range_get_adjustment (CTK_RANGE (scale))));
      pango_layout_set_text (priv->layout, txt, -1);
      g_free (txt);
    }

  return priv->layout;
}

/**
 * ctk_scale_get_layout_offsets:
 * @scale: a #CtkScale
 * @x: (out) (allow-none): location to store X offset of layout, or %NULL
 * @y: (out) (allow-none): location to store Y offset of layout, or %NULL
 *
 * Obtains the coordinates where the scale will draw the 
 * #PangoLayout representing the text in the scale. Remember
 * when using the #PangoLayout function you need to convert to
 * and from pixels using PANGO_PIXELS() or #PANGO_SCALE. 
 *
 * If the #CtkScale:draw-value property is %FALSE, the return 
 * values are undefined.
 *
 * Since: 2.4
 */
void 
ctk_scale_get_layout_offsets (CtkScale *scale,
                              gint     *x,
                              gint     *y)
{
  gint local_x = 0; 
  gint local_y = 0;

  g_return_if_fail (CTK_IS_SCALE (scale));

  if (CTK_SCALE_GET_CLASS (scale)->get_layout_offsets)
    (CTK_SCALE_GET_CLASS (scale)->get_layout_offsets) (scale, &local_x, &local_y);

  if (x)
    *x = local_x;
  
  if (y)
    *y = local_y;
}

static void
ctk_scale_clear_value_layout (CtkScale *scale)
{
  g_set_object (&scale->priv->layout, NULL);
}

static void
ctk_scale_mark_free (gpointer data)
{
  CtkScaleMark *mark = data;

  if (mark->label_gadget)
    ctk_css_node_set_parent (ctk_css_gadget_get_node (mark->label_gadget), NULL);
  g_clear_object (&mark->label_gadget);
  ctk_css_node_set_parent (ctk_css_gadget_get_node (mark->indicator_gadget), NULL);
  g_object_unref (mark->indicator_gadget);
  ctk_css_node_set_parent (ctk_css_gadget_get_node (mark->gadget), NULL);
  g_object_unref (mark->gadget);
  g_clear_object (&mark->layout);
  g_free (mark->markup);
  g_free (mark);
}

static void
ctk_scale_clear_mark_layouts (CtkScale *scale)
{
  GSList *m;

  for (m = scale->priv->marks; m; m = m->next)
    {
      CtkScaleMark *mark = m->data;
      g_clear_object (&mark->layout);
    }
}

/**
 * ctk_scale_clear_marks:
 * @scale: a #CtkScale
 * 
 * Removes any marks that have been added with ctk_scale_add_mark().
 *
 * Since: 2.16
 */
void
ctk_scale_clear_marks (CtkScale *scale)
{
  CtkScalePrivate *priv;
  CtkStyleContext *context;

  g_return_if_fail (CTK_IS_SCALE (scale));

  priv = scale->priv;

  g_slist_free_full (priv->marks, ctk_scale_mark_free);
  priv->marks = NULL;

  if (priv->top_marks_gadget)
    ctk_css_node_set_parent (ctk_css_gadget_get_node (priv->top_marks_gadget), NULL);
  g_clear_object (&priv->top_marks_gadget);
  if (priv->bottom_marks_gadget)
    ctk_css_node_set_parent (ctk_css_gadget_get_node (priv->bottom_marks_gadget), NULL);
  g_clear_object (&priv->bottom_marks_gadget);

  context = ctk_widget_get_style_context (CTK_WIDGET (scale));
  ctk_style_context_remove_class (context, "marks-before");
  ctk_style_context_remove_class (context, "marks-after");

  _ctk_range_set_stop_values (CTK_RANGE (scale), NULL, 0);

  ctk_widget_queue_resize (CTK_WIDGET (scale));
}

/**
 * ctk_scale_add_mark:
 * @scale: a #CtkScale
 * @value: the value at which the mark is placed, must be between
 *   the lower and upper limits of the scales’ adjustment
 * @position: where to draw the mark. For a horizontal scale, #CTK_POS_TOP
 *   and %CTK_POS_LEFT are drawn above the scale, anything else below.
 *   For a vertical scale, #CTK_POS_LEFT and %CTK_POS_TOP are drawn to
 *   the left of the scale, anything else to the right.
 * @markup: (allow-none): Text to be shown at the mark, using [Pango markup][PangoMarkupFormat], or %NULL
 *
 *
 * Adds a mark at @value.
 *
 * A mark is indicated visually by drawing a tick mark next to the scale,
 * and CTK+ makes it easy for the user to position the scale exactly at the
 * marks value.
 *
 * If @markup is not %NULL, text is shown next to the tick mark.
 *
 * To remove marks from a scale, use ctk_scale_clear_marks().
 *
 * Since: 2.16
 */
void
ctk_scale_add_mark (CtkScale        *scale,
                    gdouble          value,
                    CtkPositionType  position,
                    const gchar     *markup)
{
  CtkWidget *widget;
  CtkScalePrivate *priv;
  CtkScaleMark *mark;
  GSList *m;
  gdouble *values;
  gint n, i;
  CtkCssNode *widget_node, *marks_node;
  CtkStyleContext *context;

  g_return_if_fail (CTK_IS_SCALE (scale));

  priv = scale->priv;
  widget = CTK_WIDGET (scale);
  widget_node = ctk_widget_get_css_node (widget);

  mark = g_new0 (CtkScaleMark, 1);
  mark->value = value;
  mark->markup = g_strdup (markup);
  if (position == CTK_POS_LEFT ||
      position == CTK_POS_TOP)
    mark->position = CTK_POS_TOP;
  else
    mark->position = CTK_POS_BOTTOM;

  priv->marks = g_slist_insert_sorted_with_data (priv->marks, mark,
                                                 compare_marks,
                                                 GINT_TO_POINTER (ctk_range_get_inverted (CTK_RANGE (scale))));

  if (mark->position == CTK_POS_TOP)
    {
      if (!priv->top_marks_gadget)
        {
          priv->top_marks_gadget =
            ctk_css_custom_gadget_new ("marks",
                                       widget, NULL, NULL,
                                       ctk_scale_measure_marks,
                                       ctk_scale_allocate_marks,
                                       ctk_scale_render_marks,
                                       NULL, NULL);
          ctk_css_node_insert_after (widget_node,
                                     ctk_css_gadget_get_node (priv->top_marks_gadget),
                                     (priv->value_gadget &&
                                      (priv->value_pos == CTK_POS_TOP || priv->value_pos == CTK_POS_LEFT)) ?
                                     ctk_css_gadget_get_node (priv->value_gadget) : NULL);
          ctk_css_gadget_add_class (priv->top_marks_gadget, CTK_STYLE_CLASS_TOP);
          ctk_css_gadget_set_state (priv->top_marks_gadget, ctk_css_node_get_state (widget_node));
        }
      marks_node = ctk_css_gadget_get_node (priv->top_marks_gadget);
    }
  else
    {
      if (!priv->bottom_marks_gadget)
        {
          priv->bottom_marks_gadget =
            ctk_css_custom_gadget_new ("marks",
                                       widget, NULL, NULL,
                                       ctk_scale_measure_marks,
                                       ctk_scale_allocate_marks,
                                       ctk_scale_render_marks,
                                       NULL, NULL);
          ctk_css_node_insert_before (widget_node,
                                      ctk_css_gadget_get_node (priv->bottom_marks_gadget),
                                      (priv->value_gadget &&
                                       (priv->value_pos == CTK_POS_BOTTOM || priv->value_pos == CTK_POS_RIGHT)) ?
                                      ctk_css_gadget_get_node (priv->value_gadget) : NULL);
          ctk_css_gadget_add_class (priv->bottom_marks_gadget, CTK_STYLE_CLASS_BOTTOM);
          ctk_css_gadget_set_state (priv->bottom_marks_gadget, ctk_css_node_get_state (widget_node));
        }
      marks_node = ctk_css_gadget_get_node (priv->bottom_marks_gadget);
    }

  mark->gadget =
    ctk_css_custom_gadget_new ("mark",
                               widget, NULL, NULL,
                               ctk_scale_measure_mark,
                               ctk_scale_allocate_mark,
                               ctk_scale_render_mark,
                               mark, NULL);
  ctk_css_gadget_set_state (mark->gadget, ctk_css_node_get_state (marks_node));

  mark->indicator_gadget =
    ctk_css_custom_gadget_new ("indicator",
                               widget, mark->gadget, NULL,
                               NULL,
                               NULL,
                               ctk_scale_render_mark_indicator,
                               mark, NULL);
  if (mark->markup && *mark->markup)
    {
      mark->label_gadget =
        ctk_css_custom_gadget_new ("label",
                                   widget, mark->gadget,
                                   mark->position == CTK_POS_TOP ?
                                   NULL : mark->indicator_gadget,
                                   ctk_scale_measure_mark_label,
                                   NULL,
                                   ctk_scale_render_mark_label,
                                   mark, NULL);
      g_signal_connect (ctk_css_gadget_get_node (mark->label_gadget), "style-changed",
                        G_CALLBACK (ctk_scale_mark_style_changed), mark);
    }

  m = g_slist_find (priv->marks, mark);
  m = m->next;
  while (m)
    {
      CtkScaleMark *next = m->data;
      if (next->position == mark->position)
        break;
      m = m->next;
    }

  if (m)
    {
      CtkScaleMark *next = m->data;
      ctk_css_node_insert_before (marks_node,
                                  ctk_css_gadget_get_node (mark->gadget),
                                  ctk_css_gadget_get_node (next->gadget));
    }
  else
    {
      ctk_css_node_set_parent (ctk_css_gadget_get_node (mark->gadget), marks_node);
    }

  n = g_slist_length (priv->marks);
  values = g_new (gdouble, n);
  for (m = priv->marks, i = 0; m; m = m->next, i++)
    {
      mark = m->data;
      values[i] = mark->value;
    }

  _ctk_range_set_stop_values (CTK_RANGE (scale), values, n);

  g_free (values);

  context = ctk_widget_get_style_context (CTK_WIDGET (scale));
  if (priv->top_marks_gadget)
    ctk_style_context_add_class (context, "marks-before");
  if (priv->bottom_marks_gadget)
    ctk_style_context_add_class (context, "marks-after");

  ctk_widget_queue_resize (widget);
}

static CtkBuildableIface *parent_buildable_iface;

static void
ctk_scale_buildable_interface_init (CtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->custom_tag_start = ctk_scale_buildable_custom_tag_start;
  iface->custom_finished = ctk_scale_buildable_custom_finished;
}

typedef struct
{
  CtkScale *scale;
  CtkBuilder *builder;
  GSList *marks;
} MarksSubparserData;

typedef struct
{
  gdouble value;
  CtkPositionType position;
  GString *markup;
  gchar *context;
  gboolean translatable;
} MarkData;

static void
mark_data_free (MarkData *data)
{
  g_string_free (data->markup, TRUE);
  g_free (data->context);
  g_slice_free (MarkData, data);
}

static void
marks_start_element (GMarkupParseContext *context,
                     const gchar         *element_name,
                     const gchar        **names,
                     const gchar        **values,
                     gpointer             user_data,
                     GError             **error)
{
  MarksSubparserData *data = (MarksSubparserData*)user_data;

  if (strcmp (element_name, "marks") == 0)
    {
      if (!_ctk_builder_check_parent (data->builder, context, "object", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_INVALID, NULL, NULL,
                                        G_MARKUP_COLLECT_INVALID))
        _ctk_builder_prefix_error (data->builder, context, error);
    }
  else if (strcmp (element_name, "mark") == 0)
    {
      const gchar *value_str;
      gdouble value = 0;
      const gchar *position_str = NULL;
      CtkPositionType position = CTK_POS_BOTTOM;
      const gchar *msg_context = NULL;
      gboolean translatable = FALSE;
      MarkData *mark;

      if (!_ctk_builder_check_parent (data->builder, context, "marks", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_STRING, "value", &value_str,
                                        G_MARKUP_COLLECT_BOOLEAN|G_MARKUP_COLLECT_OPTIONAL, "translatable", &translatable,
                                        G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, "comments", NULL,
                                        G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, "context", &msg_context,
                                        G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, "position", &position_str,
                                        G_MARKUP_COLLECT_INVALID))
        {
          _ctk_builder_prefix_error (data->builder, context, error);
          return;
        }

      if (value_str != NULL)
        {
          GValue gvalue = G_VALUE_INIT;

          if (!ctk_builder_value_from_string_type (data->builder, G_TYPE_DOUBLE, value_str, &gvalue, error))
            {
              _ctk_builder_prefix_error (data->builder, context, error);
              return;
            }

          value = g_value_get_double (&gvalue);
        }

      if (position_str != NULL)
        {
          GValue gvalue = G_VALUE_INIT;

          if (!ctk_builder_value_from_string_type (data->builder, CTK_TYPE_POSITION_TYPE, position_str, &gvalue, error))
            {
              _ctk_builder_prefix_error (data->builder, context, error);
              return;
            }

          position = g_value_get_enum (&gvalue);
        }

      mark = g_slice_new (MarkData);
      mark->value = value;
      if (position == CTK_POS_LEFT || position == CTK_POS_TOP)
        mark->position = CTK_POS_TOP;
      else
        mark->position = CTK_POS_BOTTOM;
      mark->markup = g_string_new ("");
      mark->context = g_strdup (msg_context);
      mark->translatable = translatable;

      data->marks = g_slist_prepend (data->marks, mark);
    }
  else
    {
      _ctk_builder_error_unhandled_tag (data->builder, context,
                                        "CtkScale", element_name,
                                        error);
    }
}

static void
marks_text (GMarkupParseContext  *context,
            const gchar          *text,
            gsize                 text_len,
            gpointer              user_data,
            GError              **error)
{
  MarksSubparserData *data = (MarksSubparserData*)user_data;

  if (strcmp (g_markup_parse_context_get_element (context), "mark") == 0)
    {
      MarkData *mark = data->marks->data;

      g_string_append_len (mark->markup, text, text_len);
    }
}

static const GMarkupParser marks_parser =
  {
    marks_start_element,
    NULL,
    marks_text,
  };


static gboolean
ctk_scale_buildable_custom_tag_start (CtkBuildable  *buildable,
                                      CtkBuilder    *builder,
                                      GObject       *child,
                                      const gchar   *tagname,
                                      GMarkupParser *parser,
                                      gpointer      *parser_data)
{
  MarksSubparserData *data;

  if (child)
    return FALSE;

  if (strcmp (tagname, "marks") == 0)
    {
      data = g_slice_new0 (MarksSubparserData);
      data->scale = CTK_SCALE (buildable);
      data->builder = builder;
      data->marks = NULL;

      *parser = marks_parser;
      *parser_data = data;

      return TRUE;
    }

  return parent_buildable_iface->custom_tag_start (buildable, builder, child,
                                                   tagname, parser, parser_data);
}

static void
ctk_scale_buildable_custom_finished (CtkBuildable *buildable,
                                     CtkBuilder   *builder,
                                     GObject      *child,
                                     const gchar  *tagname,
                                     gpointer      user_data)
{
  CtkScale *scale = CTK_SCALE (buildable);
  MarksSubparserData *marks_data;

  if (strcmp (tagname, "marks") == 0)
    {
      GSList *m;
      const gchar *markup;

      marks_data = (MarksSubparserData *)user_data;

      for (m = marks_data->marks; m; m = m->next)
        {
          MarkData *mdata = m->data;

          if (mdata->translatable && mdata->markup->len)
            markup = _ctk_builder_parser_translate (ctk_builder_get_translation_domain (builder),
                                                    mdata->context,
                                                    mdata->markup->str);
          else
            markup = mdata->markup->str;

          ctk_scale_add_mark (scale, mdata->value, mdata->position, markup);

          mark_data_free (mdata);
        }

      g_slist_free (marks_data->marks);
      g_slice_free (MarksSubparserData, marks_data);
    }
  else
    {
      parent_buildable_iface->custom_finished (buildable, builder, child,
					       tagname, user_data);
    }

}
