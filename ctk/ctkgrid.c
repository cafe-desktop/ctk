/* CTK - The GIMP Toolkit
 * Copyright (C) 2010 Red Hat, Inc.
 * Author: Matthias Clasen
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

#include "config.h"

#include <string.h>

#include "ctkgrid.h"

#include "ctkorientableprivate.h"
#include "ctkrender.h"
#include "ctksizerequest.h"
#include "ctkwidgetprivate.h"
#include "ctkcontainerprivate.h"
#include "ctkcsscustomgadgetprivate.h"
#include "ctkprivate.h"
#include "ctkintl.h"


/**
 * SECTION:ctkgrid
 * @Short_description: Pack widgets in rows and columns
 * @Title: CtkGrid
 * @See_also: #CtkBox
 *
 * CtkGrid is a container which arranges its child widgets in
 * rows and columns, with arbitrary positions and horizontal/vertical spans.
 *
 * Children are added using ctk_grid_attach(). They can span multiple
 * rows or columns. It is also possible to add a child next to an
 * existing child, using ctk_grid_attach_next_to(). The behaviour of
 * CtkGrid when several children occupy the same grid cell is undefined.
 *
 * CtkGrid can be used like a #CtkBox by just using ctk_container_add(),
 * which will place children next to each other in the direction determined
 * by the #CtkOrientable:orientation property. However, if all you want is a
 * single row or column, then #CtkBox is the preferred widget.
 *
 * # CSS nodes
 *
 * CtkGrid uses a single CSS node with name grid.
 */

typedef struct _CtkGridChild CtkGridChild;
typedef struct _CtkGridChildAttach CtkGridChildAttach;
typedef struct _CtkGridRowProperties CtkGridRowProperties;
typedef struct _CtkGridLine CtkGridLine;
typedef struct _CtkGridLines CtkGridLines;
typedef struct _CtkGridLineData CtkGridLineData;
typedef struct _CtkGridRequest CtkGridRequest;

struct _CtkGridChildAttach
{
  gint pos;
  gint span;
};

struct _CtkGridRowProperties
{
  gint row;
  CtkBaselinePosition baseline_position;
};

static const CtkGridRowProperties ctk_grid_row_properties_default = {
  0,
  CTK_BASELINE_POSITION_CENTER
};

struct _CtkGridChild
{
  CtkWidget *widget;
  CtkGridChildAttach attach[2];
};

#define CHILD_LEFT(child)    ((child)->attach[CTK_ORIENTATION_HORIZONTAL].pos)
#define CHILD_WIDTH(child)   ((child)->attach[CTK_ORIENTATION_HORIZONTAL].span)
#define CHILD_TOP(child)     ((child)->attach[CTK_ORIENTATION_VERTICAL].pos)
#define CHILD_HEIGHT(child)  ((child)->attach[CTK_ORIENTATION_VERTICAL].span)

/* A CtkGridLineData struct contains row/column specific parts
 * of the grid.
 */
struct _CtkGridLineData
{
  gint16 spacing;
  guint homogeneous : 1;
};

struct _CtkGridPrivate
{
  GList *children;
  GList *row_properties;

  CtkCssGadget *gadget;

  CtkOrientation orientation;
  gint baseline_row;

  CtkGridLineData linedata[2];
};

#define ROWS(priv)    (&(priv)->linedata[CTK_ORIENTATION_HORIZONTAL])
#define COLUMNS(priv) (&(priv)->linedata[CTK_ORIENTATION_VERTICAL])

/* A CtkGridLine struct represents a single row or column
 * during size requests
 */
struct _CtkGridLine
{
  gint minimum;
  gint natural;
  gint minimum_above;
  gint minimum_below;
  gint natural_above;
  gint natural_below;

  gint position;
  gint allocation;
  gint allocated_baseline;

  guint need_expand : 1;
  guint expand      : 1;
  guint empty       : 1;
};

struct _CtkGridLines
{
  CtkGridLine *lines;
  gint min, max;
};

struct _CtkGridRequest
{
  CtkGrid *grid;
  CtkGridLines lines[2];
};


enum
{
  PROP_0,
  PROP_ROW_SPACING,
  PROP_COLUMN_SPACING,
  PROP_ROW_HOMOGENEOUS,
  PROP_COLUMN_HOMOGENEOUS,
  PROP_BASELINE_ROW,
  N_PROPERTIES,
  PROP_ORIENTATION
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

enum
{
  CHILD_PROP_0,
  CHILD_PROP_LEFT_ATTACH,
  CHILD_PROP_TOP_ATTACH,
  CHILD_PROP_WIDTH,
  CHILD_PROP_HEIGHT,
  N_CHILD_PROPERTIES
};

static GParamSpec *child_properties[N_CHILD_PROPERTIES] = { NULL, };

G_DEFINE_TYPE_WITH_CODE (CtkGrid, ctk_grid, CTK_TYPE_CONTAINER,
                         G_ADD_PRIVATE (CtkGrid)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_ORIENTABLE, NULL))


static void ctk_grid_row_properties_free (CtkGridRowProperties *props);

static void
ctk_grid_get_property (GObject    *object,
                       guint       prop_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
  CtkGrid *grid = CTK_GRID (object);
  CtkGridPrivate *priv = grid->priv;

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, priv->orientation);
      break;

    case PROP_ROW_SPACING:
      g_value_set_int (value, COLUMNS (priv)->spacing);
      break;

    case PROP_COLUMN_SPACING:
      g_value_set_int (value, ROWS (priv)->spacing);
      break;

    case PROP_ROW_HOMOGENEOUS:
      g_value_set_boolean (value, COLUMNS (priv)->homogeneous);
      break;

    case PROP_COLUMN_HOMOGENEOUS:
      g_value_set_boolean (value, ROWS (priv)->homogeneous);
      break;

    case PROP_BASELINE_ROW:
      g_value_set_int (value, priv->baseline_row);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_grid_set_orientation (CtkGrid        *grid,
                          CtkOrientation  orientation)
{
  CtkGridPrivate *priv = grid->priv;

  if (priv->orientation != orientation)
    {
      priv->orientation = orientation;
      _ctk_orientable_set_style_classes (CTK_ORIENTABLE (grid));

      g_object_notify (G_OBJECT (grid), "orientation");
    }
}

static void
ctk_grid_set_property (GObject      *object,
                       guint         prop_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
  CtkGrid *grid = CTK_GRID (object);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      ctk_grid_set_orientation (grid, g_value_get_enum (value));
      break;

    case PROP_ROW_SPACING:
      ctk_grid_set_row_spacing (grid, g_value_get_int (value));
      break;

    case PROP_COLUMN_SPACING:
      ctk_grid_set_column_spacing (grid, g_value_get_int (value));
      break;

    case PROP_ROW_HOMOGENEOUS:
      ctk_grid_set_row_homogeneous (grid, g_value_get_boolean (value));
      break;

    case PROP_COLUMN_HOMOGENEOUS:
      ctk_grid_set_column_homogeneous (grid, g_value_get_boolean (value));
      break;

    case PROP_BASELINE_ROW:
      ctk_grid_set_baseline_row (grid, g_value_get_int (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static CtkGridChild *
find_grid_child (CtkGrid   *grid,
                 CtkWidget *widget)
{
  CtkGridPrivate *priv = grid->priv;
  CtkGridChild *child;
  GList *list;

  for (list = priv->children; list; list = list->next)
    {
      child = list->data;

      if (child->widget == widget)
        return child;
    }

  return NULL;
}

static void
ctk_grid_get_child_property (CtkContainer *container,
                             CtkWidget    *child,
                             guint         property_id,
                             GValue       *value,
                             GParamSpec   *pspec)
{
  CtkGrid *grid = CTK_GRID (container);
  CtkGridChild *grid_child;

  grid_child = find_grid_child (grid, child);

  if (grid_child == NULL)
    {
      CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      return;
    }

  switch (property_id)
    {
    case CHILD_PROP_LEFT_ATTACH:
      g_value_set_int (value, CHILD_LEFT (grid_child));
      break;

    case CHILD_PROP_TOP_ATTACH:
      g_value_set_int (value, CHILD_TOP (grid_child));
      break;

    case CHILD_PROP_WIDTH:
      g_value_set_int (value, CHILD_WIDTH (grid_child));
      break;

    case CHILD_PROP_HEIGHT:
      g_value_set_int (value, CHILD_HEIGHT (grid_child));
      break;

    default:
      CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
ctk_grid_set_child_property (CtkContainer *container,
                             CtkWidget    *child,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  CtkGrid *grid = CTK_GRID (container);
  CtkGridChild *grid_child;

  grid_child = find_grid_child (grid, child);

  if (grid_child == NULL)
    {
      CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      return;
    }

  switch (property_id)
    {
    case CHILD_PROP_LEFT_ATTACH:
      CHILD_LEFT (grid_child) = g_value_get_int (value);
      break;

    case CHILD_PROP_TOP_ATTACH:
      CHILD_TOP (grid_child) = g_value_get_int (value);
      break;

   case CHILD_PROP_WIDTH:
      CHILD_WIDTH (grid_child) = g_value_get_int (value);
      break;

    case CHILD_PROP_HEIGHT:
      CHILD_HEIGHT (grid_child) = g_value_get_int (value);
      break;

    default:
      CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }

  if (_ctk_widget_get_visible (child) &&
      _ctk_widget_get_visible (CTK_WIDGET (grid)))
    ctk_widget_queue_resize (child);
}

static void
ctk_grid_finalize (GObject *object)
{
  CtkGrid *grid = CTK_GRID (object);
  CtkGridPrivate *priv = grid->priv;

  g_list_free_full (priv->row_properties, (GDestroyNotify)ctk_grid_row_properties_free);

  g_clear_object (&priv->gadget);

  G_OBJECT_CLASS (ctk_grid_parent_class)->finalize (object);
}

static void
grid_attach (CtkGrid   *grid,
             CtkWidget *widget,
             gint       left,
             gint       top,
             gint       width,
             gint       height)
{
  CtkGridPrivate *priv = grid->priv;
  CtkGridChild *child;

  child = g_slice_new (CtkGridChild);
  child->widget = widget;
  CHILD_LEFT (child) = left;
  CHILD_TOP (child) = top;
  CHILD_WIDTH (child) = width;
  CHILD_HEIGHT (child) = height;

  priv->children = g_list_prepend (priv->children, child);

  ctk_widget_set_parent (widget, CTK_WIDGET (grid));
}

/* Find the position 'touching' existing
 * children. @orientation and @max determine
 * from which direction to approach (horizontal
 * + max = right, vertical + !max = top, etc).
 * @op_pos, @op_span determine the rows/columns
 * in which the touching has to happen.
 */
static gint
find_attach_position (CtkGrid         *grid,
                      CtkOrientation   orientation,
                      gint             op_pos,
                      gint             op_span,
                      gboolean         max)
{
  CtkGridPrivate *priv = grid->priv;
  CtkGridChild *grid_child;
  CtkGridChildAttach *attach;
  CtkGridChildAttach *opposite;
  GList *list;
  gint pos;
  gboolean hit;

  if (max)
    pos = -G_MAXINT;
  else
    pos = G_MAXINT;

  hit = FALSE;

  for (list = priv->children; list; list = list->next)
    {
      grid_child = list->data;

      attach = &grid_child->attach[orientation];
      opposite = &grid_child->attach[1 - orientation];

      /* check if the ranges overlap */
      if (opposite->pos <= op_pos + op_span && op_pos <= opposite->pos + opposite->span)
        {
          hit = TRUE;

          if (max)
            pos = MAX (pos, attach->pos + attach->span);
          else
            pos = MIN (pos, attach->pos);
        }
     }

  if (!hit)
    pos = 0;

  return pos;
}

static void
ctk_grid_add (CtkContainer *container,
              CtkWidget    *child)
{
  CtkGrid *grid = CTK_GRID (container);
  CtkGridPrivate *priv = grid->priv;
  gint pos[2] = { 0, 0 };

  pos[priv->orientation] = find_attach_position (grid, priv->orientation, 0, 1, TRUE);
  grid_attach (grid, child, pos[0], pos[1], 1, 1);
}

static void
ctk_grid_remove (CtkContainer *container,
                 CtkWidget    *child)
{
  CtkGrid *grid = CTK_GRID (container);
  CtkGridPrivate *priv = grid->priv;
  CtkGridChild *grid_child;
  GList *list;

  for (list = priv->children; list; list = list->next)
    {
      grid_child = list->data;

      if (grid_child->widget == child)
        {
          gboolean was_visible = _ctk_widget_get_visible (child);

          ctk_widget_unparent (child);

          priv->children = g_list_remove (priv->children, grid_child);

          g_slice_free (CtkGridChild, grid_child);

          if (was_visible && _ctk_widget_get_visible (CTK_WIDGET (grid)))
            ctk_widget_queue_resize (CTK_WIDGET (grid));

          break;
        }
    }
}

static void
ctk_grid_forall (CtkContainer *container,
                 gboolean      include_internals G_GNUC_UNUSED,
                 CtkCallback   callback,
                 gpointer      callback_data)
{
  CtkGrid *grid = CTK_GRID (container);
  CtkGridPrivate *priv = grid->priv;
  CtkGridChild *child;
  GList *list;

  list = priv->children;
  while (list)
    {
      child = list->data;
      list  = list->next;

      (* callback) (child->widget, callback_data);
    }
}

static GType
ctk_grid_child_type (CtkContainer *container G_GNUC_UNUSED)
{
  return CTK_TYPE_WIDGET;
}

/* Calculates the min and max numbers for both orientations.
 */
static void
ctk_grid_request_count_lines (CtkGridRequest *request)
{
  CtkGridPrivate *priv = request->grid->priv;
  CtkGridChild *child;
  CtkGridChildAttach *attach;
  GList *list;
  gint min[2];
  gint max[2];

  min[0] = min[1] = G_MAXINT;
  max[0] = max[1] = G_MININT;

  for (list = priv->children; list; list = list->next)
    {
      child = list->data;
      attach = child->attach;

      min[0] = MIN (min[0], attach[0].pos);
      max[0] = MAX (max[0], attach[0].pos + attach[0].span);
      min[1] = MIN (min[1], attach[1].pos);
      max[1] = MAX (max[1], attach[1].pos + attach[1].span);
    }

  request->lines[0].min = min[0];
  request->lines[0].max = max[0];
  request->lines[1].min = min[1];
  request->lines[1].max = max[1];
}

/* Sets line sizes to 0 and marks lines as expand
 * if they have a non-spanning expanding child.
 */
static void
ctk_grid_request_init (CtkGridRequest *request,
                       CtkOrientation  orientation)
{
  CtkGridPrivate *priv = request->grid->priv;
  CtkGridChild *child;
  CtkGridChildAttach *attach;
  CtkGridLines *lines;
  GList *list;
  gint i;

  lines = &request->lines[orientation];

  for (i = 0; i < lines->max - lines->min; i++)
    {
      lines->lines[i].minimum = 0;
      lines->lines[i].natural = 0;
      lines->lines[i].minimum_above = -1;
      lines->lines[i].minimum_below = -1;
      lines->lines[i].natural_above = -1;
      lines->lines[i].natural_below = -1;
      lines->lines[i].expand = FALSE;
      lines->lines[i].empty = TRUE;
    }

  for (list = priv->children; list; list = list->next)
    {
      child = list->data;

      attach = &child->attach[orientation];
      if (attach->span == 1 && ctk_widget_compute_expand (child->widget, orientation))
        lines->lines[attach->pos - lines->min].expand = TRUE;
    }
}

/* Sums allocations for lines spanned by child and their spacing.
 */
static gint
compute_allocation_for_child (CtkGridRequest *request,
                              CtkGridChild   *child,
                              CtkOrientation  orientation)
{
  CtkGridPrivate *priv = request->grid->priv;
  CtkGridLineData *linedata;
  CtkGridLines *lines;
  CtkGridLine *line;
  CtkGridChildAttach *attach;
  gint size;
  gint i;

  linedata = &priv->linedata[orientation];
  lines = &request->lines[orientation];
  attach = &child->attach[orientation];

  size = (attach->span - 1) * linedata->spacing;
  for (i = 0; i < attach->span; i++)
    {
      line = &lines->lines[attach->pos - lines->min + i];
      size += line->allocation;
    }

  return size;
}

static void
compute_request_for_child (CtkGridRequest *request,
                           CtkGridChild   *child,
                           CtkOrientation  orientation,
                           gboolean        contextual,
                           gint           *minimum,
                           gint           *natural,
			   gint           *minimum_baseline,
                           gint           *natural_baseline)
{
  if (minimum_baseline)
    *minimum_baseline = -1;
  if (natural_baseline)
    *natural_baseline = -1;
  if (contextual)
    {
      gint size;

      size = compute_allocation_for_child (request, child, 1 - orientation);
      if (orientation == CTK_ORIENTATION_HORIZONTAL)
        ctk_widget_get_preferred_width_for_height (child->widget,
                                                   size,
                                                   minimum, natural);
      else
        ctk_widget_get_preferred_height_and_baseline_for_width (child->widget,
								size,
								minimum, natural,
								minimum_baseline, natural_baseline);
    }
  else
    {
      if (orientation == CTK_ORIENTATION_HORIZONTAL)
        ctk_widget_get_preferred_width (child->widget, minimum, natural);
      else
        ctk_widget_get_preferred_height_and_baseline_for_width (child->widget,
								-1,
								minimum, natural,
								minimum_baseline, natural_baseline);
    }
}

/* Sets requisition to max. of non-spanning children.
 * If contextual is TRUE, requires allocations of
 * lines in the opposite orientation to be set.
 */
static void
ctk_grid_request_non_spanning (CtkGridRequest *request,
                               CtkOrientation  orientation,
                               gboolean        contextual)
{
  CtkGridPrivate *priv = request->grid->priv;
  CtkGridChild *child;
  CtkGridChildAttach *attach;
  CtkGridLines *lines;
  CtkGridLine *line;
  GList *list;
  gint i;
  CtkBaselinePosition baseline_pos;
  gint minimum, minimum_baseline;
  gint natural, natural_baseline;

  lines = &request->lines[orientation];

  for (list = priv->children; list; list = list->next)
    {
      child = list->data;

      if (!_ctk_widget_get_visible (child->widget))
        continue;

      attach = &child->attach[orientation];
      if (attach->span != 1)
        continue;

      compute_request_for_child (request, child, orientation, contextual, &minimum, &natural, &minimum_baseline, &natural_baseline);

      line = &lines->lines[attach->pos - lines->min];

      if (minimum_baseline != -1)
	{
	  line->minimum_above = MAX (line->minimum_above, minimum_baseline);
	  line->minimum_below = MAX (line->minimum_below, minimum - minimum_baseline);
	  line->natural_above = MAX (line->natural_above, natural_baseline);
	  line->natural_below = MAX (line->natural_below, natural - natural_baseline);
	}
      else
	{
	  line->minimum = MAX (line->minimum, minimum);
	  line->natural = MAX (line->natural, natural);
	}
    }

  for (i = 0; i < lines->max - lines->min; i++)
    {
      line = &lines->lines[i];

      if (line->minimum_above != -1)
	{
	  line->minimum = MAX (line->minimum, line->minimum_above + line->minimum_below);
	  line->natural = MAX (line->natural, line->natural_above + line->natural_below);

	  baseline_pos = ctk_grid_get_row_baseline_position (request->grid, i + lines->min);

	  switch (baseline_pos)
	    {
	    case CTK_BASELINE_POSITION_TOP:
	      line->minimum_above += 0;
	      line->minimum_below += line->minimum - (line->minimum_above + line->minimum_below);
	      line->natural_above += 0;
	      line->natural_below += line->natural - (line->natural_above + line->natural_below);
	      break;
	    case CTK_BASELINE_POSITION_CENTER:
	      line->minimum_above += (line->minimum - (line->minimum_above + line->minimum_below))/2;
	      line->minimum_below += (line->minimum - (line->minimum_above + line->minimum_below))/2;
	      line->natural_above += (line->natural - (line->natural_above + line->natural_below))/2;
	      line->natural_below += (line->natural - (line->natural_above + line->natural_below))/2;
	      break;
	    case CTK_BASELINE_POSITION_BOTTOM:
	      line->minimum_above += line->minimum - (line->minimum_above + line->minimum_below);
	      line->minimum_below += 0;
	      line->natural_above += line->natural - (line->natural_above + line->natural_below);
	      line->natural_below += 0;
	      break;
	    }
	}
    }
}

/* Enforce homogeneous sizes.
 */
static void
ctk_grid_request_homogeneous (CtkGridRequest *request,
                              CtkOrientation  orientation)
{
  CtkGridPrivate *priv = request->grid->priv;
  CtkGridLineData *linedata;
  CtkGridLines *lines;
  gint minimum, natural;
  gint i;

  linedata = &priv->linedata[orientation];
  lines = &request->lines[orientation];

  if (!linedata->homogeneous)
    return;

  minimum = 0;
  natural = 0;

  for (i = 0; i < lines->max - lines->min; i++)
    {
      minimum = MAX (minimum, lines->lines[i].minimum);
      natural = MAX (natural, lines->lines[i].natural);
    }

  for (i = 0; i < lines->max - lines->min; i++)
    {
      lines->lines[i].minimum = minimum;
      lines->lines[i].natural = natural;
      /* TODO: Do we want to adjust the baseline here too?
       * And if so, also in the homogenous resize.
       */
    }
}

/* Deals with spanning children.
 * Requires expand fields of lines to be set for
 * non-spanning children.
 */
static void
ctk_grid_request_spanning (CtkGridRequest *request,
                           CtkOrientation  orientation,
                           gboolean        contextual)
{
  CtkGridPrivate *priv = request->grid->priv;
  GList *list;
  CtkGridChild *child;
  CtkGridChildAttach *attach;
  CtkGridLineData *linedata;
  CtkGridLines *lines;
  CtkGridLine *line;
  gint minimum;
  gint natural;
  gint span_minimum;
  gint span_natural;
  gint span_expand;
  gboolean force_expand;
  gint extra;
  gint expand;
  gint line_extra;
  gint i;

  linedata = &priv->linedata[orientation];
  lines = &request->lines[orientation];

  for (list = priv->children; list; list = list->next)
    {
      child = list->data;

      if (!_ctk_widget_get_visible (child->widget))
        continue;

      attach = &child->attach[orientation];
      if (attach->span == 1)
        continue;

      /* We ignore baselines for spanning children */
      compute_request_for_child (request, child, orientation, contextual, &minimum, &natural, NULL, NULL);

      span_minimum = (attach->span - 1) * linedata->spacing;
      span_natural = (attach->span - 1) * linedata->spacing;
      span_expand = 0;
      force_expand = FALSE;
      for (i = 0; i < attach->span; i++)
        {
          line = &lines->lines[attach->pos - lines->min + i];
          span_minimum += line->minimum;
          span_natural += line->natural;
          if (line->expand)
            span_expand += 1;
        }
      if (span_expand == 0)
        {
          span_expand = attach->span;
          force_expand = TRUE;
        }

      /* If we need to request more space for this child to fill
       * its requisition, then divide up the needed space amongst the
       * lines it spans, favoring expandable lines if any.
       *
       * When doing homogeneous allocation though, try to keep the
       * line allocations even, since we're going to force them to
       * be the same anyway, and we don't want to introduce unnecessary
       * extra space.
       */
      if (span_minimum < minimum)
        {
          if (linedata->homogeneous)
            {
              gint total, m;

              total = minimum - (attach->span - 1) * linedata->spacing;
              m = total / attach->span + (total % attach->span ? 1 : 0);
              for (i = 0; i < attach->span; i++)
                {
                  line = &lines->lines[attach->pos - lines->min + i];
                  line->minimum = MAX(line->minimum, m);
                }
            }
          else
            {
              extra = minimum - span_minimum;
              expand = span_expand;
              for (i = 0; i < attach->span; i++)
                {
                  line = &lines->lines[attach->pos - lines->min + i];
                  if (force_expand || line->expand)
                    {
                      line_extra = extra / expand;
                      line->minimum += line_extra;
                      extra -= line_extra;
                      expand -= 1;
                    }
                }
            }
        }

      if (span_natural < natural)
        {
          if (linedata->homogeneous)
            {
              gint total, n;

              total = natural - (attach->span - 1) * linedata->spacing;
              n = total / attach->span + (total % attach->span ? 1 : 0);
              for (i = 0; i < attach->span; i++)
                {
                  line = &lines->lines[attach->pos - lines->min + i];
                  line->natural = MAX(line->natural, n);
                }
            }
          else
            {
              extra = natural - span_natural;
              expand = span_expand;
              for (i = 0; i < attach->span; i++)
                {
                  line = &lines->lines[attach->pos - lines->min + i];
                  if (force_expand || line->expand)
                    {
                      line_extra = extra / expand;
                      line->natural += line_extra;
                      extra -= line_extra;
                      expand -= 1;
                    }
                }
            }
        }
    }
}

/* Marks empty and expanding lines and counts them.
 */
static void
ctk_grid_request_compute_expand (CtkGridRequest *request,
                                 CtkOrientation  orientation,
				 gint            min,
				 gint            max,
                                 gint           *nonempty_lines,
                                 gint           *expand_lines)
{
  CtkGridPrivate *priv = request->grid->priv;
  CtkGridChild *child;
  CtkGridChildAttach *attach;
  GList *list;
  gint i;
  CtkGridLines *lines;
  CtkGridLine *line;
  gboolean has_expand;
  gint expand;
  gint empty;

  lines = &request->lines[orientation];

  min = MAX (min, lines->min);
  max = MIN (max, lines->max);

  for (i = min - lines->min; i < max - lines->min; i++)
    {
      lines->lines[i].need_expand = FALSE;
      lines->lines[i].expand = FALSE;
      lines->lines[i].empty = TRUE;
    }

  for (list = priv->children; list; list = list->next)
    {
      child = list->data;

      if (!_ctk_widget_get_visible (child->widget))
        continue;

      attach = &child->attach[orientation];
      if (attach->span != 1)
        continue;

      if (attach->pos >= max || attach->pos < min)
	continue;

      line = &lines->lines[attach->pos - lines->min];
      line->empty = FALSE;
      if (ctk_widget_compute_expand (child->widget, orientation))
        line->expand = TRUE;
    }

  for (list = priv->children; list; list = list->next)
    {
      child = list->data;

      if (!_ctk_widget_get_visible (child->widget))
        continue;

      attach = &child->attach[orientation];
      if (attach->span == 1)
        continue;

      has_expand = FALSE;
      for (i = 0; i < attach->span; i++)
        {
          line = &lines->lines[attach->pos - lines->min + i];

          if (line->expand)
            has_expand = TRUE;

	  if (attach->pos + i >= max || attach->pos + 1 < min)
	    continue;

          line->empty = FALSE;
        }

      if (!has_expand && ctk_widget_compute_expand (child->widget, orientation))
        {
          for (i = 0; i < attach->span; i++)
            {
	      if (attach->pos + i >= max || attach->pos + 1 < min)
		continue;

              line = &lines->lines[attach->pos - lines->min + i];
              line->need_expand = TRUE;
            }
        }
    }

  empty = 0;
  expand = 0;
  for (i = min - lines->min; i < max - lines->min; i++)
    {
      line = &lines->lines[i];

      if (line->need_expand)
        line->expand = TRUE;

      if (line->empty)
        empty += 1;

      if (line->expand)
        expand += 1;
    }

  if (nonempty_lines)
    *nonempty_lines = max - min - empty;

  if (expand_lines)
    *expand_lines = expand;
}

/* Sums the minimum and natural fields of lines and their spacing.
 */
static void
ctk_grid_request_sum (CtkGridRequest *request,
                      CtkOrientation  orientation,
                      gint           *minimum,
                      gint           *natural,
		      gint           *minimum_baseline,
		      gint           *natural_baseline)
{
  CtkGridPrivate *priv = request->grid->priv;
  CtkGridLineData *linedata;
  CtkGridLines *lines;
  gint i;
  gint min, nat;
  gint nonempty;

  ctk_grid_request_compute_expand (request, orientation, G_MININT, G_MAXINT, &nonempty, NULL);

  linedata = &priv->linedata[orientation];
  lines = &request->lines[orientation];

  min = 0;
  nat = 0;
  for (i = 0; i < lines->max - lines->min; i++)
    {
      if (orientation == CTK_ORIENTATION_VERTICAL &&
	  lines->min + i == priv->baseline_row &&
	  lines->lines[i].minimum_above != -1)
	{
	  if (minimum_baseline)
	    *minimum_baseline = min + lines->lines[i].minimum_above;
	  if (natural_baseline)
	    *natural_baseline = nat + lines->lines[i].natural_above;
	}

      min += lines->lines[i].minimum;
      nat += lines->lines[i].natural;

      if (!lines->lines[i].empty)
	{
	  min += linedata->spacing;
	  nat += linedata->spacing;
	}
    }

  /* Remove last spacing, if any was applied */
  if (nonempty > 0)
    {
      min -= linedata->spacing;
      nat -= linedata->spacing;
    }

  *minimum = min;
  *natural = nat;
}

/* Computes minimum and natural fields of lines.
 * When contextual is TRUE, requires allocation of
 * lines in the opposite orientation to be set.
 */
static void
ctk_grid_request_run (CtkGridRequest *request,
                      CtkOrientation  orientation,
                      gboolean        contextual)
{
  ctk_grid_request_init (request, orientation);
  ctk_grid_request_non_spanning (request, orientation, contextual);
  ctk_grid_request_homogeneous (request, orientation);
  ctk_grid_request_spanning (request, orientation, contextual);
  ctk_grid_request_homogeneous (request, orientation);
}

static void
ctk_grid_distribute_non_homogeneous (CtkGridLines *lines,
				     gint nonempty,
				     gint expand,
				     gint size,
				     gint min,
				     gint max)
{
  CtkRequestedSize *sizes;
  CtkGridLine *line;
  gint extra;
  gint rest;
  int i, j;

  if (nonempty == 0)
    return;

  sizes = g_newa (CtkRequestedSize, nonempty);

  j = 0;
  for (i = min - lines->min; i < max - lines->min; i++)
    {
      line = &lines->lines[i];
      if (line->empty)
	continue;

      size -= line->minimum;

      sizes[j].minimum_size = line->minimum;
      sizes[j].natural_size = line->natural;
      sizes[j].data = line;
      j++;
    }

  size = ctk_distribute_natural_allocation (MAX (0, size), nonempty, sizes);

  if (expand > 0)
    {
      extra = size / expand;
      rest = size % expand;
    }
  else
    {
      extra = 0;
      rest = 0;
    }

  j = 0;
  for (i = min - lines->min; i < max - lines->min; i++)
    {
      line = &lines->lines[i];
      if (line->empty)
	continue;

      g_assert (line == sizes[j].data);

      line->allocation = sizes[j].minimum_size;
      if (line->expand)
	{
	  line->allocation += extra;
	  if (rest > 0)
	    {
	      line->allocation += 1;
	      rest -= 1;
	    }
	}

      j++;
    }
}

/* Requires that the minimum and natural fields of lines
 * have been set, computes the allocation field of lines
 * by distributing total_size among lines.
 */
static void
ctk_grid_request_allocate (CtkGridRequest *request,
                           CtkOrientation  orientation,
                           gint            total_size)
{
  CtkGridPrivate *priv = request->grid->priv;
  CtkGridLineData *linedata;
  CtkGridLines *lines;
  CtkGridLine *line;
  gint nonempty1, nonempty2;
  gint expand1, expand2;
  gint i;
  CtkBaselinePosition baseline_pos;
  gint baseline;
  gint extra, extra2;
  gint rest;
  gint size1, size2;
  gint split, split_pos;

  linedata = &priv->linedata[orientation];
  lines = &request->lines[orientation];

  baseline = ctk_widget_get_allocated_baseline (CTK_WIDGET (request->grid));

  if (orientation == CTK_ORIENTATION_VERTICAL && baseline != -1 &&
      priv->baseline_row >= lines->min && priv->baseline_row < lines->max &&
      lines->lines[priv->baseline_row - lines->min].minimum_above != -1)
    {
      split = priv->baseline_row;
      split_pos = baseline - lines->lines[priv->baseline_row - lines->min].minimum_above;
      ctk_grid_request_compute_expand (request, orientation, lines->min, split, &nonempty1, &expand1);
      ctk_grid_request_compute_expand (request, orientation, split, lines->max, &nonempty2, &expand2);

      if (nonempty2 > 0)
	{
	  size1 = split_pos - (nonempty1) * linedata->spacing;
	  size2 = (total_size - split_pos) - (nonempty2 - 1) * linedata->spacing;
	}
      else
	{
	  size1 = total_size - (nonempty1 - 1) * linedata->spacing;
	  size2 = 0;
	}
    }
  else
    {
      ctk_grid_request_compute_expand (request, orientation, lines->min, lines->max, &nonempty1, &expand1);
      nonempty2 = expand2 = 0;
      split = lines->max;

      size1 = total_size - (nonempty1 - 1) * linedata->spacing;
      size2 = 0;
    }

  if (nonempty1 == 0 && nonempty2 == 0)
    return;

  if (linedata->homogeneous)
    {
      if (nonempty1 > 0)
	{
	  extra = size1 / nonempty1;
	  rest = size1 % nonempty1;
	}
      else
	{
	  extra = 0;
	  rest = 0;
	}
      if (nonempty2 > 0)
	{
	  extra2 = size2 / nonempty2;
	  if (extra2 < extra || nonempty1 == 0)
	    {
	      extra = extra2;
	      rest = size2 % nonempty2;
	    }
	}

      for (i = 0; i < lines->max - lines->min; i++)
        {
          line = &lines->lines[i];
          if (line->empty)
            continue;

          line->allocation = extra;
          if (rest > 0)
            {
              line->allocation += 1;
              rest -= 1;
            }
        }
    }
  else
    {
      ctk_grid_distribute_non_homogeneous (lines,
					   nonempty1,
					   expand1,
					   size1,
					   lines->min,
					   split);
      ctk_grid_distribute_non_homogeneous (lines,
					   nonempty2,
					   expand2,
					   size2,
					   split,
					   lines->max);
    }

  for (i = 0; i < lines->max - lines->min; i++)
    {
      line = &lines->lines[i];
      if (line->empty)
	continue;

      if (line->minimum_above != -1)
	{
	  /* Note: This is overridden in ctk_grid_request_position for the allocated baseline */
	  baseline_pos = ctk_grid_get_row_baseline_position (request->grid, i + lines->min);

	  switch (baseline_pos)
	    {
	    case CTK_BASELINE_POSITION_TOP:
	      line->allocated_baseline =
		line->minimum_above;
	      break;
	    case CTK_BASELINE_POSITION_CENTER:
	      line->allocated_baseline =
		line->minimum_above +
		(line->allocation - (line->minimum_above + line->minimum_below)) / 2;
	      break;
	    case CTK_BASELINE_POSITION_BOTTOM:
	      line->allocated_baseline =
		line->allocation - line->minimum_below;
	      break;
	    }
	}
      else
	line->allocated_baseline = -1;
    }
}

/* Computes the position fields from allocation and spacing.
 */
static void
ctk_grid_request_position (CtkGridRequest *request,
                           CtkOrientation  orientation)
{
  CtkGridPrivate *priv = request->grid->priv;
  CtkGridLineData *linedata;
  CtkGridLines *lines;
  CtkGridLine *line;
  gint position, old_position;
  int allocated_baseline;
  gint i, j;

  linedata = &priv->linedata[orientation];
  lines = &request->lines[orientation];

  allocated_baseline = ctk_widget_get_allocated_baseline (CTK_WIDGET(request->grid));

  position = 0;
  for (i = 0; i < lines->max - lines->min; i++)
    {
      line = &lines->lines[i];

      if (orientation == CTK_ORIENTATION_VERTICAL &&
	  i + lines->min == priv->baseline_row &&
	  allocated_baseline != -1 &&
	  lines->lines[i].minimum_above != -1)
	{
	  old_position = position;
	  position = allocated_baseline - line->minimum_above;

	  /* Back-patch previous rows */
	  for (j = 0; j < i; j++)
	    {
	      if (!lines->lines[j].empty)
		lines->lines[j].position += position - old_position;
	    }
	}

      if (!line->empty)
        {
          line->position = position;
          position += line->allocation + linedata->spacing;

	  if (orientation == CTK_ORIENTATION_VERTICAL &&
	      i + lines->min == priv->baseline_row &&
	      allocated_baseline != -1 &&
	      lines->lines[i].minimum_above != -1)
	    line->allocated_baseline = allocated_baseline - line->position;
        }
    }
}

static void
ctk_grid_get_size (CtkGrid        *grid,
                   CtkOrientation  orientation,
                   gint           *minimum,
                   gint           *natural,
		   gint           *minimum_baseline,
		   gint           *natural_baseline)
{
  CtkGridRequest request;
  CtkGridLines *lines;

  *minimum = 0;
  *natural = 0;

  if (minimum_baseline)
    *minimum_baseline = -1;

  if (natural_baseline)
    *natural_baseline = -1;

  if (grid->priv->children == NULL)
    return;

  request.grid = grid;
  ctk_grid_request_count_lines (&request);
  lines = &request.lines[orientation];
  lines->lines = g_newa (CtkGridLine, lines->max - lines->min);
  memset (lines->lines, 0, (lines->max - lines->min) * sizeof (CtkGridLine));

  ctk_grid_request_run (&request, orientation, FALSE);
  ctk_grid_request_sum (&request, orientation, minimum, natural,
			minimum_baseline, natural_baseline);
}

static void
ctk_grid_get_size_for_size (CtkGrid        *grid,
                            CtkOrientation  orientation,
                            gint            size,
                            gint           *minimum,
                            gint           *natural,
			    gint           *minimum_baseline,
                            gint           *natural_baseline)
{
  CtkGridRequest request;
  CtkGridLines *lines;
  gint min_size, nat_size;

  *minimum = 0;
  *natural = 0;

  if (minimum_baseline)
    *minimum_baseline = -1;

  if (natural_baseline)
    *natural_baseline = -1;

  if (grid->priv->children == NULL)
    return;

  request.grid = grid;
  ctk_grid_request_count_lines (&request);
  lines = &request.lines[0];
  lines->lines = g_newa (CtkGridLine, lines->max - lines->min);
  memset (lines->lines, 0, (lines->max - lines->min) * sizeof (CtkGridLine));
  lines = &request.lines[1];
  lines->lines = g_newa (CtkGridLine, lines->max - lines->min);
  memset (lines->lines, 0, (lines->max - lines->min) * sizeof (CtkGridLine));

  ctk_grid_request_run (&request, 1 - orientation, FALSE);
  ctk_grid_request_sum (&request, 1 - orientation, &min_size, &nat_size, NULL, NULL);
  ctk_grid_request_allocate (&request, 1 - orientation, MAX (size, min_size));

  ctk_grid_request_run (&request, orientation, TRUE);
  ctk_grid_request_sum (&request, orientation, minimum, natural, minimum_baseline, natural_baseline);
}

static void
ctk_grid_get_preferred_width (CtkWidget *widget,
                              gint      *minimum,
                              gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CTK_GRID (widget)->priv->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_grid_get_preferred_height (CtkWidget *widget,
                               gint      *minimum,
                               gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CTK_GRID (widget)->priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_grid_get_preferred_width_for_height (CtkWidget *widget,
                                         gint       height,
                                         gint      *minimum,
                                         gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CTK_GRID (widget)->priv->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     height,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_grid_get_preferred_height_for_width (CtkWidget *widget,
                                         gint       width,
                                         gint      *minimum,
                                         gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CTK_GRID (widget)->priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     width,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_grid_get_preferred_height_and_baseline_for_width (CtkWidget *widget,
						      gint       width,
						      gint      *minimum,
						      gint      *natural,
						      gint      *minimum_baseline,
						      gint      *natural_baseline)
{
  ctk_css_gadget_get_preferred_size (CTK_GRID (widget)->priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     width,
                                     minimum, natural,
                                     minimum_baseline, natural_baseline);
}

static void
ctk_grid_measure (CtkCssGadget   *gadget,
                  CtkOrientation  orientation,
                  int             for_size,
                  int            *minimum,
                  int            *natural,
                  int            *minimum_baseline,
                  int            *natural_baseline,
                  gpointer        data G_GNUC_UNUSED)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkGrid *grid = CTK_GRID (widget);

  if ((orientation == CTK_ORIENTATION_HORIZONTAL &&
       ctk_widget_get_request_mode (widget) == CTK_SIZE_REQUEST_WIDTH_FOR_HEIGHT) ||
      (orientation == CTK_ORIENTATION_VERTICAL &&
       ctk_widget_get_request_mode (widget) == CTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH))
    ctk_grid_get_size_for_size (grid, orientation, for_size, minimum, natural, minimum_baseline, natural_baseline);
  else
    ctk_grid_get_size (grid, orientation, minimum, natural, minimum_baseline, natural_baseline);
}

static void
allocate_child (CtkGridRequest *request,
                CtkOrientation  orientation,
                CtkGridChild   *child,
                gint           *position,
                gint           *size,
		gint           *baseline)
{
  CtkGridPrivate *priv = request->grid->priv;
  CtkGridLineData *linedata;
  CtkGridLines *lines;
  CtkGridLine *line;
  CtkGridChildAttach *attach;
  gint i;

  linedata = &priv->linedata[orientation];
  lines = &request->lines[orientation];
  attach = &child->attach[orientation];

  *position = lines->lines[attach->pos - lines->min].position;
  if (attach->span == 1)
    *baseline = lines->lines[attach->pos - lines->min].allocated_baseline;
  else
    *baseline = -1;

  *size = (attach->span - 1) * linedata->spacing;
  for (i = 0; i < attach->span; i++)
    {
      line = &lines->lines[attach->pos - lines->min + i];
      *size += line->allocation;
    }
}

static void
ctk_grid_request_allocate_children (CtkGridRequest      *request,
                                    const CtkAllocation *allocation)
{
  CtkGridPrivate *priv = request->grid->priv;
  GList *list;
  CtkGridChild *child;
  CtkAllocation child_allocation;
  gint x, y, width, height, baseline, ignore;

  for (list = priv->children; list; list = list->next)
    {
      child = list->data;

      if (!_ctk_widget_get_visible (child->widget))
        continue;

      allocate_child (request, CTK_ORIENTATION_HORIZONTAL, child, &x, &width, &ignore);
      allocate_child (request, CTK_ORIENTATION_VERTICAL, child, &y, &height, &baseline);

      child_allocation.x = allocation->x + x;
      child_allocation.y = allocation->y + y;
      child_allocation.width = MAX (1, width);
      child_allocation.height = MAX (1, height);

      if (ctk_widget_get_direction (CTK_WIDGET (request->grid)) == CTK_TEXT_DIR_RTL)
        child_allocation.x = allocation->x + allocation->width
                             - (child_allocation.x - allocation->x) - child_allocation.width;

      ctk_widget_size_allocate_with_baseline (child->widget, &child_allocation, baseline);
    }
}

#define GET_SIZE(allocation, orientation) (orientation == CTK_ORIENTATION_HORIZONTAL ? allocation->width : allocation->height)

static void
ctk_grid_size_allocate (CtkWidget     *widget,
                        CtkAllocation *allocation)
{
  CtkAllocation clip;

  ctk_widget_set_allocation (widget, allocation);

  ctk_css_gadget_allocate (CTK_GRID (widget)->priv->gadget,
                           allocation,
                           ctk_widget_get_allocated_baseline (widget),
                           &clip);

  ctk_widget_set_clip (widget, &clip);
}

static void
ctk_grid_allocate (CtkCssGadget        *gadget,
                   const CtkAllocation *allocation,
                   int                  baseline G_GNUC_UNUSED,
                   CtkAllocation       *out_clip,
                   gpointer             data G_GNUC_UNUSED)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkGrid *grid = CTK_GRID (widget);
  CtkGridPrivate *priv = grid->priv;
  CtkGridRequest request;
  CtkGridLines *lines;
  CtkOrientation orientation;

  if (priv->children == NULL)
    return;

  request.grid = grid;

  ctk_grid_request_count_lines (&request);
  lines = &request.lines[0];
  lines->lines = g_newa (CtkGridLine, lines->max - lines->min);
  memset (lines->lines, 0, (lines->max - lines->min) * sizeof (CtkGridLine));
  lines = &request.lines[1];
  lines->lines = g_newa (CtkGridLine, lines->max - lines->min);
  memset (lines->lines, 0, (lines->max - lines->min) * sizeof (CtkGridLine));

  if (ctk_widget_get_request_mode (widget) == CTK_SIZE_REQUEST_WIDTH_FOR_HEIGHT)
    orientation = CTK_ORIENTATION_HORIZONTAL;
  else
    orientation = CTK_ORIENTATION_VERTICAL;

  ctk_grid_request_run (&request, 1 - orientation, FALSE);
  ctk_grid_request_allocate (&request, 1 - orientation, GET_SIZE (allocation, 1 - orientation));
  ctk_grid_request_run (&request, orientation, TRUE);

  ctk_grid_request_allocate (&request, orientation, GET_SIZE (allocation, orientation));

  ctk_grid_request_position (&request, 0);
  ctk_grid_request_position (&request, 1);

  ctk_grid_request_allocate_children (&request, allocation);

  ctk_container_get_children_clip (CTK_CONTAINER (grid), out_clip);
}

static gboolean
ctk_grid_render (CtkCssGadget *gadget,
                 cairo_t      *cr,
                 int           x G_GNUC_UNUSED,
                 int           y G_GNUC_UNUSED,
                 int           width G_GNUC_UNUSED,
                 int           height G_GNUC_UNUSED,
                 gpointer      data G_GNUC_UNUSED)
{
  CTK_WIDGET_CLASS (ctk_grid_parent_class)->draw (ctk_css_gadget_get_owner (gadget), cr);

  return FALSE;
}

static gboolean
ctk_grid_draw (CtkWidget *widget,
               cairo_t   *cr)
{
  ctk_css_gadget_draw (CTK_GRID (widget)->priv->gadget, cr);

  return FALSE;
}

static void
ctk_grid_class_init (CtkGridClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (class);
  CtkContainerClass *container_class = CTK_CONTAINER_CLASS (class);

  object_class->get_property = ctk_grid_get_property;
  object_class->set_property = ctk_grid_set_property;
  object_class->finalize = ctk_grid_finalize;

  widget_class->size_allocate = ctk_grid_size_allocate;
  widget_class->get_preferred_width = ctk_grid_get_preferred_width;
  widget_class->get_preferred_height = ctk_grid_get_preferred_height;
  widget_class->get_preferred_width_for_height = ctk_grid_get_preferred_width_for_height;
  widget_class->get_preferred_height_for_width = ctk_grid_get_preferred_height_for_width;
  widget_class->get_preferred_height_and_baseline_for_width = ctk_grid_get_preferred_height_and_baseline_for_width;
  widget_class->draw = ctk_grid_draw;

  container_class->add = ctk_grid_add;
  container_class->remove = ctk_grid_remove;
  container_class->forall = ctk_grid_forall;
  container_class->child_type = ctk_grid_child_type;
  container_class->set_child_property = ctk_grid_set_child_property;
  container_class->get_child_property = ctk_grid_get_child_property;
  ctk_container_class_handle_border_width (container_class);

  g_object_class_override_property (object_class, PROP_ORIENTATION, "orientation");

  obj_properties[PROP_ROW_SPACING] =
    g_param_spec_int ("row-spacing",
                      P_("Row spacing"),
                      P_("The amount of space between two consecutive rows"),
                      0, G_MAXINT16, 0,
                      CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  obj_properties[PROP_COLUMN_SPACING] =
    g_param_spec_int ("column-spacing",
                      P_("Column spacing"),
                      P_("The amount of space between two consecutive columns"),
                      0, G_MAXINT16, 0,
                      CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  obj_properties[PROP_ROW_HOMOGENEOUS] =
    g_param_spec_boolean ("row-homogeneous",
                          P_("Row Homogeneous"),
                          P_("If TRUE, the rows are all the same height"),
                          FALSE,
                          CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  obj_properties[PROP_COLUMN_HOMOGENEOUS] =
    g_param_spec_boolean ("column-homogeneous",
                          P_("Column Homogeneous"),
                          P_("If TRUE, the columns are all the same width"),
                          FALSE,
                          CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  obj_properties[PROP_BASELINE_ROW] =
    g_param_spec_int ("baseline-row",
                      P_("Baseline Row"),
                      P_("The row to align the to the baseline when valign is CTK_ALIGN_BASELINE"),
                      0, G_MAXINT, 0,
                      CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class,
                                     N_PROPERTIES,
                                     obj_properties);

  child_properties[CHILD_PROP_LEFT_ATTACH] =
    g_param_spec_int ("left-attach",
                      P_("Left attachment"),
                      P_("The column number to attach the left side of the child to"),
                      G_MININT, G_MAXINT, 0,
                      CTK_PARAM_READWRITE);

  child_properties[CHILD_PROP_TOP_ATTACH] =
    g_param_spec_int ("top-attach",
                      P_("Top attachment"),
                      P_("The row number to attach the top side of a child widget to"),
                      G_MININT, G_MAXINT, 0,
                      CTK_PARAM_READWRITE);

  child_properties[CHILD_PROP_WIDTH] =
    g_param_spec_int ("width",
                      P_("Width"),
                      P_("The number of columns that a child spans"),
                      1, G_MAXINT, 1,
                      CTK_PARAM_READWRITE);

  child_properties[CHILD_PROP_HEIGHT] =
    g_param_spec_int ("height",
                      P_("Height"),
                      P_("The number of rows that a child spans"),
                      1, G_MAXINT, 1,
                      CTK_PARAM_READWRITE);

  ctk_container_class_install_child_properties (container_class, N_CHILD_PROPERTIES, child_properties);
  ctk_widget_class_set_css_name (widget_class, "grid");
}

static void
ctk_grid_init (CtkGrid *grid)
{
  CtkGridPrivate *priv;

  grid->priv = ctk_grid_get_instance_private (grid);
  priv = grid->priv;

  ctk_widget_set_has_window (CTK_WIDGET (grid), FALSE);

  priv->children = NULL;
  priv->orientation = CTK_ORIENTATION_HORIZONTAL;
  priv->baseline_row = 0;

  priv->linedata[0].spacing = 0;
  priv->linedata[1].spacing = 0;

  priv->linedata[0].homogeneous = FALSE;
  priv->linedata[1].homogeneous = FALSE;

  priv->gadget = ctk_css_custom_gadget_new_for_node (ctk_widget_get_css_node (CTK_WIDGET (grid)),
                                                     CTK_WIDGET (grid),
                                                     ctk_grid_measure,
                                                     ctk_grid_allocate,
                                                     ctk_grid_render,
                                                     NULL,
                                                     NULL);


  _ctk_orientable_set_style_classes (CTK_ORIENTABLE (grid));
}

/**
 * ctk_grid_new:
 *
 * Creates a new grid widget.
 *
 * Returns: the new #CtkGrid
 */
CtkWidget *
ctk_grid_new (void)
{
  return g_object_new (CTK_TYPE_GRID, NULL);
}

/**
 * ctk_grid_attach:
 * @grid: a #CtkGrid
 * @child: the widget to add
 * @left: the column number to attach the left side of @child to
 * @top: the row number to attach the top side of @child to
 * @width: the number of columns that @child will span
 * @height: the number of rows that @child will span
 *
 * Adds a widget to the grid.
 *
 * The position of @child is determined by @left and @top. The
 * number of “cells” that @child will occupy is determined by
 * @width and @height.
 */
void
ctk_grid_attach (CtkGrid   *grid,
                 CtkWidget *child,
                 gint       left,
                 gint       top,
                 gint       width,
                 gint       height)
{
  g_return_if_fail (CTK_IS_GRID (grid));
  g_return_if_fail (CTK_IS_WIDGET (child));
  g_return_if_fail (_ctk_widget_get_parent (child) == NULL);
  g_return_if_fail (width > 0);
  g_return_if_fail (height > 0);

  grid_attach (grid, child, left, top, width, height);
}

/**
 * ctk_grid_attach_next_to:
 * @grid: a #CtkGrid
 * @child: the widget to add
 * @sibling: (allow-none): the child of @grid that @child will be placed
 *     next to, or %NULL to place @child at the beginning or end
 * @side: the side of @sibling that @child is positioned next to
 * @width: the number of columns that @child will span
 * @height: the number of rows that @child will span
 *
 * Adds a widget to the grid.
 *
 * The widget is placed next to @sibling, on the side determined by
 * @side. When @sibling is %NULL, the widget is placed in row (for
 * left or right placement) or column 0 (for top or bottom placement),
 * at the end indicated by @side.
 *
 * Attaching widgets labeled [1], [2], [3] with @sibling == %NULL and
 * @side == %CTK_POS_LEFT yields a layout of [3][2][1].
 */
void
ctk_grid_attach_next_to (CtkGrid         *grid,
                         CtkWidget       *child,
                         CtkWidget       *sibling,
                         CtkPositionType  side,
                         gint             width,
                         gint             height)
{
  CtkGridChild *grid_sibling;
  gint left, top;

  g_return_if_fail (CTK_IS_GRID (grid));
  g_return_if_fail (CTK_IS_WIDGET (child));
  g_return_if_fail (_ctk_widget_get_parent (child) == NULL);
  g_return_if_fail (sibling == NULL || _ctk_widget_get_parent (sibling) == (CtkWidget*)grid);
  g_return_if_fail (width > 0);
  g_return_if_fail (height > 0);

  if (sibling)
    {
      grid_sibling = find_grid_child (grid, sibling);

      switch (side)
        {
        case CTK_POS_LEFT:
          left = CHILD_LEFT (grid_sibling) - width;
          top = CHILD_TOP (grid_sibling);
          break;
        case CTK_POS_RIGHT:
          left = CHILD_LEFT (grid_sibling) + CHILD_WIDTH (grid_sibling);
          top = CHILD_TOP (grid_sibling);
          break;
        case CTK_POS_TOP:
          left = CHILD_LEFT (grid_sibling);
          top = CHILD_TOP (grid_sibling) - height;
          break;
        case CTK_POS_BOTTOM:
          left = CHILD_LEFT (grid_sibling);
          top = CHILD_TOP (grid_sibling) + CHILD_HEIGHT (grid_sibling);
          break;
        default:
          g_assert_not_reached ();
        }
    }
  else
    {
      switch (side)
        {
        case CTK_POS_LEFT:
          left = find_attach_position (grid, CTK_ORIENTATION_HORIZONTAL, 0, height, FALSE);
          left -= width;
          top = 0;
          break;
        case CTK_POS_RIGHT:
          left = find_attach_position (grid, CTK_ORIENTATION_HORIZONTAL, 0, height, TRUE);
          top = 0;
          break;
        case CTK_POS_TOP:
          left = 0;
          top = find_attach_position (grid, CTK_ORIENTATION_VERTICAL, 0, width, FALSE);
          top -= height;
          break;
        case CTK_POS_BOTTOM:
          left = 0;
          top = find_attach_position (grid, CTK_ORIENTATION_VERTICAL, 0, width, TRUE);
          break;
        default:
          g_assert_not_reached ();
        }
    }

  grid_attach (grid, child, left, top, width, height);
}

/**
 * ctk_grid_get_child_at:
 * @grid: a #CtkGrid
 * @left: the left edge of the cell
 * @top: the top edge of the cell
 *
 * Gets the child of @grid whose area covers the grid
 * cell whose upper left corner is at @left, @top.
 *
 * Returns: (transfer none) (nullable): the child at the given position, or %NULL
 *
 * Since: 3.2
 */
CtkWidget *
ctk_grid_get_child_at (CtkGrid *grid,
                       gint     left,
                       gint     top)
{
  CtkGridPrivate *priv;
  CtkGridChild *child;
  GList *list;

  g_return_val_if_fail (CTK_IS_GRID (grid), NULL);

  priv = grid->priv;

  for (list = priv->children; list; list = list->next)
    {
      child = list->data;

      if (CHILD_LEFT (child) <= left &&
          CHILD_LEFT (child) + CHILD_WIDTH (child) > left &&
          CHILD_TOP (child) <= top &&
          CHILD_TOP (child) + CHILD_HEIGHT (child) > top)
        return child->widget;
    }

  return NULL;
}

/**
 * ctk_grid_insert_row:
 * @grid: a #CtkGrid
 * @position: the position to insert the row at
 *
 * Inserts a row at the specified position.
 *
 * Children which are attached at or below this position
 * are moved one row down. Children which span across this
 * position are grown to span the new row.
 *
 * Since: 3.2
 */
void
ctk_grid_insert_row (CtkGrid *grid,
                     gint     position)
{
  CtkGridPrivate *priv;
  CtkGridChild *child;
  GList *list;
  gint top, height;

  g_return_if_fail (CTK_IS_GRID (grid));

  priv = grid->priv;

  for (list = priv->children; list; list = list->next)
    {
      child = list->data;

      top = CHILD_TOP (child);
      height = CHILD_HEIGHT (child);

      if (top >= position)
        {
          CHILD_TOP (child) = top + 1;
          ctk_container_child_notify_by_pspec (CTK_CONTAINER (grid),
                                               child->widget,
                                               child_properties[CHILD_PROP_TOP_ATTACH]);
        }
      else if (top + height > position)
        {
          CHILD_HEIGHT (child) = height + 1;
          ctk_container_child_notify_by_pspec (CTK_CONTAINER (grid),
                                               child->widget,
                                               child_properties[CHILD_PROP_HEIGHT]);
        }
    }

  for (list = priv->row_properties; list != NULL; list = list->next)
    {
      CtkGridRowProperties *prop = list->data;

      if (prop->row >= position)
	prop->row += 1;
    }
}

/**
 * ctk_grid_remove_row:
 * @grid: a #CtkGrid
 * @position: the position of the row to remove
 *
 * Removes a row from the grid.
 *
 * Children that are placed in this row are removed,
 * spanning children that overlap this row have their
 * height reduced by one, and children below the row
 * are moved up.
 *
 * Since: 3.10
 */
void
ctk_grid_remove_row (CtkGrid *grid,
                     gint     position)
{
  CtkGridPrivate *priv;
  CtkGridChild *child;
  GList *list;
  gint top, height;

  g_return_if_fail (CTK_IS_GRID (grid));

  priv = grid->priv;

  list = priv->children;
  while (list)
    {
      child = list->data;
      list = list->next;

      top = CHILD_TOP (child);
      height = CHILD_HEIGHT (child);

      if (top <= position && top + height > position)
        height--;
      if (top > position)
        top--;

      if (height <= 0)
        ctk_container_remove (CTK_CONTAINER (grid), child->widget);
      else
        ctk_container_child_set (CTK_CONTAINER (grid), child->widget,
                                 "height", height,
                                 "top-attach", top,
                                 NULL);
    }
}

/**
 * ctk_grid_insert_column:
 * @grid: a #CtkGrid
 * @position: the position to insert the column at
 *
 * Inserts a column at the specified position.
 *
 * Children which are attached at or to the right of this position
 * are moved one column to the right. Children which span across this
 * position are grown to span the new column.
 *
 * Since: 3.2
 */
void
ctk_grid_insert_column (CtkGrid *grid,
                        gint     position)
{
  CtkGridPrivate *priv;
  CtkGridChild *child;
  GList *list;
  gint left, width;

  g_return_if_fail (CTK_IS_GRID (grid));

  priv = grid->priv;

  for (list = priv->children; list; list = list->next)
    {
      child = list->data;

      left = CHILD_LEFT (child);
      width = CHILD_WIDTH (child);

      if (left >= position)
        {
          CHILD_LEFT (child) = left + 1;
          ctk_container_child_notify_by_pspec (CTK_CONTAINER (grid),
                                               child->widget,
                                               child_properties[CHILD_PROP_LEFT_ATTACH]);
        }
      else if (left + width > position)
        {
          CHILD_WIDTH (child) = width + 1;
          ctk_container_child_notify_by_pspec (CTK_CONTAINER (grid),
                                               child->widget,
                                               child_properties[CHILD_PROP_WIDTH]);
        }
    }
}

/**
 * ctk_grid_remove_column:
 * @grid: a #CtkGrid
 * @position: the position of the column to remove
 *
 * Removes a column from the grid.
 *
 * Children that are placed in this column are removed,
 * spanning children that overlap this column have their
 * width reduced by one, and children after the column
 * are moved to the left.
 *
 * Since: 3.10
 */
void
ctk_grid_remove_column (CtkGrid *grid,
                        gint     position)
{
  CtkGridPrivate *priv;
  CtkGridChild *child;
  GList *list;
  gint left, width;

  g_return_if_fail (CTK_IS_GRID (grid));

  priv = grid->priv;

  list = priv->children;
  while (list)
    {
      child = list->data;
      list = list->next;

      left = CHILD_LEFT (child);
      width = CHILD_WIDTH (child);

      if (left <= position && left + width > position)
        width--;
      if (left > position)
        left--;

      if (width <= 0)
        ctk_container_remove (CTK_CONTAINER (grid), child->widget);
      else
        ctk_container_child_set (CTK_CONTAINER (grid), child->widget,
                                 "width", width,
                                 "left-attach", left,
                                 NULL);
    }
}

/**
 * ctk_grid_insert_next_to:
 * @grid: a #CtkGrid
 * @sibling: the child of @grid that the new row or column will be
 *     placed next to
 * @side: the side of @sibling that @child is positioned next to
 *
 * Inserts a row or column at the specified position.
 *
 * The new row or column is placed next to @sibling, on the side
 * determined by @side. If @side is %CTK_POS_TOP or %CTK_POS_BOTTOM,
 * a row is inserted. If @side is %CTK_POS_LEFT of %CTK_POS_RIGHT,
 * a column is inserted.
 *
 * Since: 3.2
 */
void
ctk_grid_insert_next_to (CtkGrid         *grid,
                         CtkWidget       *sibling,
                         CtkPositionType  side)
{
  CtkGridChild *child;

  g_return_if_fail (CTK_IS_GRID (grid));
  g_return_if_fail (CTK_IS_WIDGET (sibling));
  g_return_if_fail (_ctk_widget_get_parent (sibling) == (CtkWidget*)grid);

  child = find_grid_child (grid, sibling);

  switch (side)
    {
    case CTK_POS_LEFT:
      ctk_grid_insert_column (grid, CHILD_LEFT (child));
      break;
    case CTK_POS_RIGHT:
      ctk_grid_insert_column (grid, CHILD_LEFT (child) + CHILD_WIDTH (child));
      break;
    case CTK_POS_TOP:
      ctk_grid_insert_row (grid, CHILD_TOP (child));
      break;
    case CTK_POS_BOTTOM:
      ctk_grid_insert_row (grid, CHILD_TOP (child) + CHILD_HEIGHT (child));
      break;
    default:
      g_assert_not_reached ();
    }
}

/**
 * ctk_grid_set_row_homogeneous:
 * @grid: a #CtkGrid
 * @homogeneous: %TRUE to make rows homogeneous
 *
 * Sets whether all rows of @grid will have the same height.
 */
void
ctk_grid_set_row_homogeneous (CtkGrid  *grid,
                              gboolean  homogeneous)
{
  CtkGridPrivate *priv;
  g_return_if_fail (CTK_IS_GRID (grid));

  priv = grid->priv;

  /* Yes, homogeneous rows means all the columns have the same size */
  if (COLUMNS (priv)->homogeneous != homogeneous)
    {
      COLUMNS (priv)->homogeneous = homogeneous;

      if (_ctk_widget_get_visible (CTK_WIDGET (grid)))
        ctk_widget_queue_resize (CTK_WIDGET (grid));

      g_object_notify_by_pspec (G_OBJECT (grid), obj_properties [PROP_ROW_HOMOGENEOUS]);
    }
}

/**
 * ctk_grid_get_row_homogeneous:
 * @grid: a #CtkGrid
 *
 * Returns whether all rows of @grid have the same height.
 *
 * Returns: whether all rows of @grid have the same height.
 */
gboolean
ctk_grid_get_row_homogeneous (CtkGrid *grid)
{
  CtkGridPrivate *priv;
  g_return_val_if_fail (CTK_IS_GRID (grid), FALSE);

  priv = grid->priv;

  return COLUMNS (priv)->homogeneous;
}

/**
 * ctk_grid_set_column_homogeneous:
 * @grid: a #CtkGrid
 * @homogeneous: %TRUE to make columns homogeneous
 *
 * Sets whether all columns of @grid will have the same width.
 */
void
ctk_grid_set_column_homogeneous (CtkGrid  *grid,
                                 gboolean  homogeneous)
{
  CtkGridPrivate *priv;
  g_return_if_fail (CTK_IS_GRID (grid));

  priv = grid->priv;

  /* Yes, homogeneous columns means all the rows have the same size */
  if (ROWS (priv)->homogeneous != homogeneous)
    {
      ROWS (priv)->homogeneous = homogeneous;

      if (_ctk_widget_get_visible (CTK_WIDGET (grid)))
        ctk_widget_queue_resize (CTK_WIDGET (grid));

      g_object_notify_by_pspec (G_OBJECT (grid), obj_properties [PROP_COLUMN_HOMOGENEOUS]);
    }
}

/**
 * ctk_grid_get_column_homogeneous:
 * @grid: a #CtkGrid
 *
 * Returns whether all columns of @grid have the same width.
 *
 * Returns: whether all columns of @grid have the same width.
 */
gboolean
ctk_grid_get_column_homogeneous (CtkGrid *grid)
{
  CtkGridPrivate *priv;
  g_return_val_if_fail (CTK_IS_GRID (grid), FALSE);

  priv = grid->priv;

  return ROWS (priv)->homogeneous;
}

/**
 * ctk_grid_set_row_spacing:
 * @grid: a #CtkGrid
 * @spacing: the amount of space to insert between rows
 *
 * Sets the amount of space between rows of @grid.
 */
void
ctk_grid_set_row_spacing (CtkGrid *grid,
                          guint    spacing)
{
  CtkGridPrivate *priv;
  g_return_if_fail (CTK_IS_GRID (grid));
  g_return_if_fail (spacing <= G_MAXINT16);

  priv = grid->priv;

  if (COLUMNS (priv)->spacing != spacing)
    {
      COLUMNS (priv)->spacing = spacing;

      if (_ctk_widget_get_visible (CTK_WIDGET (grid)))
        ctk_widget_queue_resize (CTK_WIDGET (grid));

      g_object_notify_by_pspec (G_OBJECT (grid), obj_properties [PROP_ROW_SPACING]);
    }
}

/**
 * ctk_grid_get_row_spacing:
 * @grid: a #CtkGrid
 *
 * Returns the amount of space between the rows of @grid.
 *
 * Returns: the row spacing of @grid
 */
guint
ctk_grid_get_row_spacing (CtkGrid *grid)
{
  CtkGridPrivate *priv;
  g_return_val_if_fail (CTK_IS_GRID (grid), 0);

  priv = grid->priv;

  return COLUMNS (priv)->spacing;
}

/**
 * ctk_grid_set_column_spacing:
 * @grid: a #CtkGrid
 * @spacing: the amount of space to insert between columns
 *
 * Sets the amount of space between columns of @grid.
 */
void
ctk_grid_set_column_spacing (CtkGrid *grid,
                             guint    spacing)
{
  CtkGridPrivate *priv;
  g_return_if_fail (CTK_IS_GRID (grid));
  g_return_if_fail (spacing <= G_MAXINT16);

  priv = grid->priv;

  if (ROWS (priv)->spacing != spacing)
    {
      ROWS (priv)->spacing = spacing;

      if (_ctk_widget_get_visible (CTK_WIDGET (grid)))
        ctk_widget_queue_resize (CTK_WIDGET (grid));

      g_object_notify_by_pspec (G_OBJECT (grid), obj_properties [PROP_COLUMN_SPACING]);
    }
}

/**
 * ctk_grid_get_column_spacing:
 * @grid: a #CtkGrid
 *
 * Returns the amount of space between the columns of @grid.
 *
 * Returns: the column spacing of @grid
 */
guint
ctk_grid_get_column_spacing (CtkGrid *grid)
{
  CtkGridPrivate *priv;

  g_return_val_if_fail (CTK_IS_GRID (grid), 0);

  priv = grid->priv;

  return ROWS (priv)->spacing;
}

static CtkGridRowProperties *
find_row_properties (CtkGrid      *grid,
		     gint          row)
{
  GList *l;

  for (l = grid->priv->row_properties; l != NULL; l = l->next)
    {
      CtkGridRowProperties *prop = l->data;
      if (prop->row == row)
	return prop;
    }

  return NULL;
}

static void
ctk_grid_row_properties_free (CtkGridRowProperties *props)
{
  g_slice_free (CtkGridRowProperties, props);
}

static CtkGridRowProperties *
get_row_properties_or_create (CtkGrid      *grid,
			      gint          row)
{
  CtkGridRowProperties *props;
  CtkGridPrivate *priv = grid->priv;

  props = find_row_properties (grid, row);
  if (props)
    return props;

  props = g_slice_new (CtkGridRowProperties);
  *props = ctk_grid_row_properties_default;
  props->row = row;

  priv->row_properties =
    g_list_prepend (priv->row_properties, props);

  return props;
}

static const CtkGridRowProperties *
get_row_properties_or_default (CtkGrid      *grid,
			       gint          row)
{
  CtkGridRowProperties *props;

  props = find_row_properties (grid, row);
  if (props)
    return props;
  return &ctk_grid_row_properties_default;
}

/**
 * ctk_grid_set_row_baseline_position:
 * @grid: a #CtkGrid
 * @row: a row index
 * @pos: a #CtkBaselinePosition
 *
 * Sets how the baseline should be positioned on @row of the
 * grid, in case that row is assigned more space than is requested.
 *
 * Since: 3.10
 */
void
ctk_grid_set_row_baseline_position (CtkGrid            *grid,
				    gint                row,
				    CtkBaselinePosition pos)
{
  CtkGridRowProperties *props;

  g_return_if_fail (CTK_IS_GRID (grid));

  props = get_row_properties_or_create (grid, row);

  if (props->baseline_position != pos)
    {
      props->baseline_position = pos;

      if (_ctk_widget_get_visible (CTK_WIDGET (grid)))
        ctk_widget_queue_resize (CTK_WIDGET (grid));
    }
}

/**
 * ctk_grid_get_row_baseline_position:
 * @grid: a #CtkGrid
 * @row: a row index
 *
 * Returns the baseline position of @row as set
 * by ctk_grid_set_row_baseline_position() or the default value
 * %CTK_BASELINE_POSITION_CENTER.
 *
 * Returns: the baseline position of @row
 *
 * Since: 3.10
 */
CtkBaselinePosition
ctk_grid_get_row_baseline_position (CtkGrid      *grid,
				    gint          row)
{
  const CtkGridRowProperties *props;

  g_return_val_if_fail (CTK_IS_GRID (grid), CTK_BASELINE_POSITION_CENTER);

  props = get_row_properties_or_default (grid, row);

  return props->baseline_position;
}

/**
 * ctk_grid_set_baseline_row:
 * @grid: a #CtkGrid
 * @row: the row index
 *
 * Sets which row defines the global baseline for the entire grid.
 * Each row in the grid can have its own local baseline, but only
 * one of those is global, meaning it will be the baseline in the
 * parent of the @grid.
 *
 * Since: 3.10
 */
void
ctk_grid_set_baseline_row (CtkGrid *grid,
			   gint     row)
{
  CtkGridPrivate *priv;

  g_return_if_fail (CTK_IS_GRID (grid));

  priv =  grid->priv;

  if (priv->baseline_row != row)
    {
      priv->baseline_row = row;

      if (_ctk_widget_get_visible (CTK_WIDGET (grid)))
	ctk_widget_queue_resize (CTK_WIDGET (grid));
      g_object_notify (G_OBJECT (grid), "baseline-row");
    }
}

/**
 * ctk_grid_get_baseline_row:
 * @grid: a #CtkGrid
 *
 * Returns which row defines the global baseline of @grid.
 *
 * Returns: the row index defining the global baseline
 *
 * Since: 3.10
 */
gint
ctk_grid_get_baseline_row (CtkGrid *grid)
{
  CtkGridPrivate *priv;

  g_return_val_if_fail (CTK_IS_GRID (grid), 0);

  priv = grid->priv;

  return priv->baseline_row;
}
