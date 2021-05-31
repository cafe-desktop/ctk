/*
 * Copyright (C) 2007-2010 Openismus GmbH
 * Copyright (C) 2013 Red Hat, Inc.
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *      Tristan Van Berkom <tristanvb@openismus.com>
 *      Matthias Clasen <mclasen@redhat.com>
 *      William Jon McCann <jmccann@redhat.com>
 */

/* Preamble {{{1 */

/**
 * SECTION:ctkflowbox
 * @Short_Description: A container that allows reflowing its children
 * @Title: CtkFlowBox
 *
 * A CtkFlowBox positions child widgets in sequence according to its
 * orientation.
 *
 * For instance, with the horizontal orientation, the widgets will be
 * arranged from left to right, starting a new row under the previous
 * row when necessary. Reducing the width in this case will require more
 * rows, so a larger height will be requested.
 *
 * Likewise, with the vertical orientation, the widgets will be arranged
 * from top to bottom, starting a new column to the right when necessary.
 * Reducing the height will require more columns, so a larger width will
 * be requested.
 *
 * The size request of a CtkFlowBox alone may not be what you expect; if you
 * need to be able to shrink it along both axes and dynamically reflow its
 * children, you may have to wrap it in a #CtkScrolledWindow to enable that.
 *
 * The children of a CtkFlowBox can be dynamically sorted and filtered.
 *
 * Although a CtkFlowBox must have only #CtkFlowBoxChild children,
 * you can add any kind of widget to it via ctk_container_add(), and
 * a CtkFlowBoxChild widget will automatically be inserted between
 * the box and the widget.
 *
 * Also see #CtkListBox.
 *
 * CtkFlowBox was added in CTK+ 3.12.
 *
 * # CSS nodes
 *
 * |[<!-- language="plain" -->
 * flowbox
 * ├── flowboxchild
 * │   ╰── <child>
 * ├── flowboxchild
 * │   ╰── <child>
 * ┊
 * ╰── [rubberband]
 * ]|
 *
 * CtkFlowBox uses a single CSS node with name flowbox. CtkFlowBoxChild
 * uses a single CSS node with name flowboxchild.
 * For rubberband selection, a subnode with name rubberband is used.
 */

#include <config.h>

#include "ctkflowbox.h"
#include "ctkmarshalers.h"
#include "ctkprivate.h"
#include "ctkorientableprivate.h"
#include "ctkintl.h"
#include "ctkcssnodeprivate.h"
#include "ctkwidgetprivate.h"
#include "ctkstylecontextprivate.h"
#include "ctkcsscustomgadgetprivate.h"
#include "ctkcontainerprivate.h"

#include "a11y/ctkflowboxaccessibleprivate.h"
#include "a11y/ctkflowboxchildaccessible.h"

/* Forward declarations and utilities {{{1 */

static void ctk_flow_box_update_cursor       (CtkFlowBox      *box,
                                              CtkFlowBoxChild *child);
static void ctk_flow_box_select_and_activate (CtkFlowBox      *box,
                                              CtkFlowBoxChild *child);
static void ctk_flow_box_update_selection    (CtkFlowBox      *box,
                                              CtkFlowBoxChild *child,
                                              gboolean         modify,
                                              gboolean         extend);
static void ctk_flow_box_apply_filter        (CtkFlowBox      *box,
                                              CtkFlowBoxChild *child);
static void ctk_flow_box_apply_sort          (CtkFlowBox      *box,
                                              CtkFlowBoxChild *child);
static gint ctk_flow_box_sort                (CtkFlowBoxChild *a,
                                              CtkFlowBoxChild *b,
                                              CtkFlowBox      *box);

static void ctk_flow_box_bound_model_changed (GListModel *list,
                                              guint       position,
                                              guint       removed,
                                              guint       added,
                                              gpointer    user_data);

static void ctk_flow_box_check_model_compat  (CtkFlowBox *box);

static void
get_current_selection_modifiers (CtkWidget *widget,
                                 gboolean  *modify,
                                 gboolean  *extend)
{
  GdkModifierType state = 0;
  GdkModifierType mask;

  *modify = FALSE;
  *extend = FALSE;

  if (ctk_get_current_event_state (&state))
    {
      mask = ctk_widget_get_modifier_mask (widget, GDK_MODIFIER_INTENT_MODIFY_SELECTION);
      if ((state & mask) == mask)
        *modify = TRUE;
      mask = ctk_widget_get_modifier_mask (widget, GDK_MODIFIER_INTENT_EXTEND_SELECTION);
      if ((state & mask) == mask)
        *extend = TRUE;
    }
}

static void
path_from_horizontal_line_rects (cairo_t      *cr,
                                 GdkRectangle *lines,
                                 gint          n_lines)
{
  gint start_line, end_line;
  GdkRectangle *r;
  gint i;

  /* Join rows vertically by extending to the middle */
  for (i = 0; i < n_lines - 1; i++)
    {
      GdkRectangle *r1 = &lines[i];
      GdkRectangle *r2 = &lines[i+1];
      gint gap, old;

      gap = r2->y - (r1->y + r1->height);
      r1->height += gap / 2;
      old = r2->y;
      r2->y = r1->y + r1->height;
      r2->height += old - r2->y;
    }

  cairo_new_path (cr);
  start_line = 0;

  do
    {
      for (i = start_line; i < n_lines; i++)
        {
          r = &lines[i];
          if (i == start_line)
            cairo_move_to (cr, r->x + r->width, r->y);
          else
            cairo_line_to (cr, r->x + r->width, r->y);
          cairo_line_to (cr, r->x + r->width, r->y + r->height);

          if (i < n_lines - 1 &&
              (r->x + r->width < lines[i+1].x ||
              r->x > lines[i+1].x + lines[i+1].width))
            {
              i++;
              break;
            }
        }
      end_line = i;
      for (i = end_line - 1; i >= start_line; i--)
        {
          r = &lines[i];
          cairo_line_to (cr, r->x, r->y + r->height);
          cairo_line_to (cr, r->x, r->y);
        }
      cairo_close_path (cr);
      start_line = end_line;
    }
  while (end_line < n_lines);
}

static void
path_from_vertical_line_rects (cairo_t      *cr,
                               GdkRectangle *lines,
                               gint          n_lines)
{
  gint start_line, end_line;
  GdkRectangle *r;
  gint i;

  /* Join rows horizontally by extending to the middle */
  for (i = 0; i < n_lines - 1; i++)
    {
      GdkRectangle *r1 = &lines[i];
      GdkRectangle *r2 = &lines[i+1];
      gint gap, old;

      gap = r2->x - (r1->x + r1->width);
      r1->width += gap / 2;
      old = r2->x;
      r2->x = r1->x + r1->width;
      r2->width += old - r2->x;
    }

  cairo_new_path (cr);
  start_line = 0;

  do
    {
      for (i = start_line; i < n_lines; i++)
        {
          r = &lines[i];
          if (i == start_line)
            cairo_move_to (cr, r->x, r->y + r->height);
          else
            cairo_line_to (cr, r->x, r->y + r->height);
          cairo_line_to (cr, r->x + r->width, r->y + r->height);

          if (i < n_lines - 1 &&
              (r->y + r->height < lines[i+1].y ||
              r->y > lines[i+1].y + lines[i+1].height))
            {
              i++;
              break;
            }
        }
      end_line = i;
      for (i = end_line - 1; i >= start_line; i--)
        {
          r = &lines[i];
          cairo_line_to (cr, r->x + r->width, r->y);
          cairo_line_to (cr, r->x, r->y);
        }
      cairo_close_path (cr);
      start_line = end_line;
    }
  while (end_line < n_lines);
}

/* CtkFlowBoxChild {{{1 */

/* GObject boilerplate {{{2 */

enum {
  CHILD_ACTIVATE,
  CHILD_LAST_SIGNAL
};

static guint child_signals[CHILD_LAST_SIGNAL] = { 0 };

typedef struct _CtkFlowBoxChildPrivate CtkFlowBoxChildPrivate;
struct _CtkFlowBoxChildPrivate
{
  GSequenceIter *iter;
  CtkCssGadget  *gadget;
  gboolean       selected;
};

#define CHILD_PRIV(child) ((CtkFlowBoxChildPrivate*)ctk_flow_box_child_get_instance_private ((CtkFlowBoxChild*)(child)))

G_DEFINE_TYPE_WITH_PRIVATE (CtkFlowBoxChild, ctk_flow_box_child, CTK_TYPE_BIN)

/* Internal API {{{2 */

static CtkFlowBox *
ctk_flow_box_child_get_box (CtkFlowBoxChild *child)
{
  CtkWidget *parent;

  parent = ctk_widget_get_parent (CTK_WIDGET (child));
  if (parent && CTK_IS_FLOW_BOX (parent))
    return CTK_FLOW_BOX (parent);

  return NULL;
}

static void
ctk_flow_box_child_set_focus (CtkFlowBoxChild *child)
{
  CtkFlowBox *box = ctk_flow_box_child_get_box (child);
  gboolean modify;
  gboolean extend;

  if (box == NULL)
    return;

  get_current_selection_modifiers (CTK_WIDGET (box), &modify, &extend);

  if (modify)
    ctk_flow_box_update_cursor (box, child);
  else
    ctk_flow_box_update_selection (box, child, FALSE, FALSE);
}

/* CtkWidget implementation {{{2 */

static gboolean
ctk_flow_box_child_focus (CtkWidget        *widget,
                          CtkDirectionType  direction)
{
  gboolean had_focus = FALSE;
  CtkWidget *child;

  child = ctk_bin_get_child (CTK_BIN (widget));

  /* Without "can-focus" flag try to pass the focus to the child immediately */
  if (!ctk_widget_get_can_focus (widget))
    {
      if (child)
        {
          if (ctk_widget_child_focus (child, direction))
            {
              CtkFlowBox *box;
              box = ctk_flow_box_child_get_box (CTK_FLOW_BOX_CHILD (widget));
              if (box)
                ctk_flow_box_update_cursor (box, CTK_FLOW_BOX_CHILD (widget));
              return TRUE;
            }
        }
      return FALSE;
    }

  g_object_get (widget, "has-focus", &had_focus, NULL);
  if (had_focus)
    {
      /* If on row, going right, enter into possible container */
      if (child &&
          (direction == CTK_DIR_RIGHT || direction == CTK_DIR_TAB_FORWARD))
        {
          if (ctk_widget_child_focus (CTK_WIDGET (child), direction))
            return TRUE;
        }

      return FALSE;
    }
  else if (ctk_container_get_focus_child (CTK_CONTAINER (widget)) != NULL)
    {
      /* Child has focus, always navigate inside it first */
      if (ctk_widget_child_focus (child, direction))
        return TRUE;

      /* If exiting child container to the left, select child  */
      if (direction == CTK_DIR_LEFT || direction == CTK_DIR_TAB_BACKWARD)
        {
          ctk_flow_box_child_set_focus (CTK_FLOW_BOX_CHILD (widget));
          return TRUE;
        }

      return FALSE;
    }
  else
    {
      /* If coming from the left, enter into possible container */
      if (child &&
          (direction == CTK_DIR_LEFT || direction == CTK_DIR_TAB_BACKWARD))
        {
          if (ctk_widget_child_focus (child, direction))
            return TRUE;
        }

      ctk_flow_box_child_set_focus (CTK_FLOW_BOX_CHILD (widget));
      return TRUE;
    }
}

static void
ctk_flow_box_child_activate (CtkFlowBoxChild *child)
{
  CtkFlowBox *box;

  box = ctk_flow_box_child_get_box (child);
  if (box)
    ctk_flow_box_select_and_activate (box, child);
}

static gboolean
ctk_flow_box_child_draw (CtkWidget *widget,
                         cairo_t   *cr)
{
  ctk_css_gadget_draw (CHILD_PRIV (CTK_FLOW_BOX_CHILD (widget))->gadget, cr);

  return FALSE;
}

static gboolean
ctk_flow_box_child_render (CtkCssGadget *gadget,
                           cairo_t      *cr,
                           int           x,
                           int           y,
                           int           width,
                           int           height,
                           gpointer      data)
{
  CtkWidget *widget;

  widget = ctk_css_gadget_get_owner (gadget);

  CTK_WIDGET_CLASS (ctk_flow_box_child_parent_class)->draw (widget, cr);

  return ctk_widget_has_visible_focus (widget);
}

/* Size allocation {{{3 */

static CtkSizeRequestMode
ctk_flow_box_child_get_request_mode (CtkWidget *widget)
{
  CtkFlowBox *box;

  box = ctk_flow_box_child_get_box (CTK_FLOW_BOX_CHILD (widget));
  if (box)
    return ctk_widget_get_request_mode (CTK_WIDGET (box));
  else
    return CTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}

static void
ctk_flow_box_child_get_preferred_height (CtkWidget *widget,
                                         gint      *minimum,
                                         gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CHILD_PRIV (CTK_FLOW_BOX_CHILD (widget))->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_flow_box_child_get_preferred_height_for_width (CtkWidget *widget,
                                                   gint       width,
                                                   gint      *minimum,
                                                   gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CHILD_PRIV (CTK_FLOW_BOX_CHILD (widget))->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     width,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_flow_box_child_get_preferred_width (CtkWidget *widget,
                                        gint      *minimum,
                                        gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CHILD_PRIV (CTK_FLOW_BOX_CHILD (widget))->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_flow_box_child_get_preferred_width_for_height (CtkWidget *widget,
                                                   gint       height,
                                                   gint      *minimum,
                                                   gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CHILD_PRIV (CTK_FLOW_BOX_CHILD (widget))->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     height,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_flow_box_child_measure (CtkCssGadget   *gadget,
                            CtkOrientation  orientation,
                            int             for_size,
                            int            *minimum,
                            int            *natural,
                            int            *minimum_baseline,
                            int            *natural_baseline,
                            gpointer        data)
{
  CtkWidget *widget;
  CtkWidget *child;

  widget = ctk_css_gadget_get_owner (gadget);
  child = ctk_bin_get_child (CTK_BIN (widget));
  if (!child || ! ctk_widget_get_visible (child))
    {
      *minimum = *natural = 0;
      return;
    }

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      if (for_size < 0)
        {
          if (ctk_flow_box_child_get_request_mode (widget) == CTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH)
            {
              ctk_widget_get_preferred_width (child, minimum, natural);
            }
          else
            {
              gint height;
              ctk_widget_get_preferred_height (child, NULL, &height);
              ctk_widget_get_preferred_width_for_height (child, height, minimum, natural);
            }
        }
      else
        {
          if (ctk_flow_box_child_get_request_mode (widget) == CTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH)
            ctk_widget_get_preferred_width (child, minimum, natural);
          else
            ctk_widget_get_preferred_width_for_height (child, for_size, minimum, natural);
        }
    }
  else
    {
      if (for_size < 0)
        {
          if (ctk_flow_box_child_get_request_mode (widget) == CTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH)
            {
              gint width;
              ctk_widget_get_preferred_width (child, NULL, &width);
              ctk_widget_get_preferred_height_for_width (child, width, minimum, natural);
            }
          else
            {
              ctk_widget_get_preferred_height (child, minimum, natural);
            }
        }
      else
        {
          if (ctk_flow_box_child_get_request_mode (widget) == CTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH)
            ctk_widget_get_preferred_height_for_width (child, for_size, minimum, natural);
          else
            ctk_widget_get_preferred_height (child, minimum, natural);
        }
    }
}

static void
ctk_flow_box_child_size_allocate (CtkWidget     *widget,
                                  CtkAllocation *allocation)
{
  CtkAllocation clip;

  ctk_widget_set_allocation (widget, allocation);

  ctk_css_gadget_allocate (CHILD_PRIV (CTK_FLOW_BOX_CHILD (widget))->gadget,
                           allocation,
                           ctk_widget_get_allocated_baseline (widget),
                           &clip);

  ctk_widget_set_clip (widget, &clip);
}

static void
ctk_flow_box_child_allocate (CtkCssGadget        *gadget,
                             const CtkAllocation *allocation,
                             int                  baseline,
                             CtkAllocation       *out_clip,
                             gpointer             data)
{
  CtkWidget *widget;
  CtkWidget *child;

  widget = ctk_css_gadget_get_owner (gadget);

  child = ctk_bin_get_child (CTK_BIN (widget));
  if (child && ctk_widget_get_visible (child))
    ctk_widget_size_allocate (child, (CtkAllocation *)allocation);

  ctk_container_get_children_clip (CTK_CONTAINER (widget), out_clip);
}

/* GObject implementation {{{2 */

static void
ctk_flow_box_child_finalize (GObject *object)
{
  g_clear_object (&CHILD_PRIV (CTK_FLOW_BOX_CHILD (object))->gadget);

  G_OBJECT_CLASS (ctk_flow_box_child_parent_class)->finalize (object);
}

static void
ctk_flow_box_child_class_init (CtkFlowBoxChildClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (class);

  object_class->finalize = ctk_flow_box_child_finalize;

  widget_class->draw = ctk_flow_box_child_draw;
  widget_class->get_request_mode = ctk_flow_box_child_get_request_mode;
  widget_class->get_preferred_height = ctk_flow_box_child_get_preferred_height;
  widget_class->get_preferred_height_for_width = ctk_flow_box_child_get_preferred_height_for_width;
  widget_class->get_preferred_width = ctk_flow_box_child_get_preferred_width;
  widget_class->get_preferred_width_for_height = ctk_flow_box_child_get_preferred_width_for_height;
  widget_class->size_allocate = ctk_flow_box_child_size_allocate;
  widget_class->focus = ctk_flow_box_child_focus;

  class->activate = ctk_flow_box_child_activate;

  /**
   * CtkFlowBoxChild::activate:
   * @child: The child on which the signal is emitted
   *
   * The ::activate signal is emitted when the user activates
   * a child widget in a #CtkFlowBox, either by clicking or
   * double-clicking, or by using the Space or Enter key.
   *
   * While this signal is used as a
   * [keybinding signal][CtkBindingSignal],
   * it can be used by applications for their own purposes.
   */
  child_signals[CHILD_ACTIVATE] =
    g_signal_new (I_("activate"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (CtkFlowBoxChildClass, activate),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);
  widget_class->activate_signal = child_signals[CHILD_ACTIVATE];

  ctk_widget_class_set_accessible_role (widget_class, ATK_ROLE_LIST_ITEM);
  ctk_widget_class_set_css_name (widget_class, "flowboxchild");
}

static void
ctk_flow_box_child_init (CtkFlowBoxChild *child)
{
  ctk_widget_set_can_focus (CTK_WIDGET (child), TRUE);

  CHILD_PRIV (child)->gadget = ctk_css_custom_gadget_new_for_node (ctk_widget_get_css_node (CTK_WIDGET (child)),
                                                     CTK_WIDGET (child),
                                                     ctk_flow_box_child_measure,
                                                     ctk_flow_box_child_allocate,
                                                     ctk_flow_box_child_render,
                                                     NULL,
                                                     NULL);
}

/* Public API {{{2 */

/**
 * ctk_flow_box_child_new:
 *
 * Creates a new #CtkFlowBoxChild, to be used as a child
 * of a #CtkFlowBox.
 *
 * Returns: a new #CtkFlowBoxChild
 *
 * Since: 3.12
 */
CtkWidget *
ctk_flow_box_child_new (void)
{
  return g_object_new (CTK_TYPE_FLOW_BOX_CHILD, NULL);
}

/**
 * ctk_flow_box_child_get_index:
 * @child: a #CtkFlowBoxChild
 *
 * Gets the current index of the @child in its #CtkFlowBox container.
 *
 * Returns: the index of the @child, or -1 if the @child is not
 *     in a flow box.
 *
 * Since: 3.12
 */
gint
ctk_flow_box_child_get_index (CtkFlowBoxChild *child)
{
  CtkFlowBoxChildPrivate *priv;

  g_return_val_if_fail (CTK_IS_FLOW_BOX_CHILD (child), -1);

  priv = CHILD_PRIV (child);

  if (priv->iter != NULL)
    return g_sequence_iter_get_position (priv->iter);

  return -1;
}

/**
 * ctk_flow_box_child_is_selected:
 * @child: a #CtkFlowBoxChild
 *
 * Returns whether the @child is currently selected in its
 * #CtkFlowBox container.
 *
 * Returns: %TRUE if @child is selected
 *
 * Since: 3.12
 */
gboolean
ctk_flow_box_child_is_selected (CtkFlowBoxChild *child)
{
  g_return_val_if_fail (CTK_IS_FLOW_BOX_CHILD (child), FALSE);

  return CHILD_PRIV (child)->selected;
}

/**
 * ctk_flow_box_child_changed:
 * @child: a #CtkFlowBoxChild
 *
 * Marks @child as changed, causing any state that depends on this
 * to be updated. This affects sorting and filtering.
 *
 * Note that calls to this method must be in sync with the data
 * used for the sorting and filtering functions. For instance, if
 * the list is mirroring some external data set, and *two* children
 * changed in the external data set when you call
 * ctk_flow_box_child_changed() on the first child, the sort function
 * must only read the new data for the first of the two changed
 * children, otherwise the resorting of the children will be wrong.
 *
 * This generally means that if you don’t fully control the data
 * model, you have to duplicate the data that affects the sorting
 * and filtering functions into the widgets themselves. Another
 * alternative is to call ctk_flow_box_invalidate_sort() on any
 * model change, but that is more expensive.
 *
 * Since: 3.12
 */
void
ctk_flow_box_child_changed (CtkFlowBoxChild *child)
{
  CtkFlowBox *box;

  g_return_if_fail (CTK_IS_FLOW_BOX_CHILD (child));

  box = ctk_flow_box_child_get_box (child);

  if (box == NULL)
    return;

  ctk_flow_box_apply_sort (box, child);
  ctk_flow_box_apply_filter (box, child);
}

/* CtkFlowBox  {{{1 */

/* Constants {{{2 */

#define DEFAULT_MAX_CHILDREN_PER_LINE 7
#define RUBBERBAND_START_DISTANCE 32
#define AUTOSCROLL_FAST_DISTANCE 32
#define AUTOSCROLL_FACTOR 20
#define AUTOSCROLL_FACTOR_FAST 10

/* GObject boilerplate {{{2 */

enum {
  CHILD_ACTIVATED,
  SELECTED_CHILDREN_CHANGED,
  ACTIVATE_CURSOR_CHILD,
  TOGGLE_CURSOR_CHILD,
  MOVE_CURSOR,
  SELECT_ALL,
  UNSELECT_ALL,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

enum {
  PROP_0,
  PROP_HOMOGENEOUS,
  PROP_COLUMN_SPACING,
  PROP_ROW_SPACING,
  PROP_MIN_CHILDREN_PER_LINE,
  PROP_MAX_CHILDREN_PER_LINE,
  PROP_SELECTION_MODE,
  PROP_ACTIVATE_ON_SINGLE_CLICK,

  /* orientable */
  PROP_ORIENTATION,
  LAST_PROP = PROP_ORIENTATION
};

static GParamSpec *props[LAST_PROP] = { NULL, };

typedef struct _CtkFlowBoxPrivate CtkFlowBoxPrivate;
struct _CtkFlowBoxPrivate {
  CtkOrientation    orientation;
  gboolean          homogeneous;

  guint             row_spacing;
  guint             column_spacing;

  CtkFlowBoxChild  *cursor_child;
  CtkFlowBoxChild  *selected_child;

  gboolean          active_child_active;
  CtkFlowBoxChild  *active_child;

  CtkSelectionMode  selection_mode;

  CtkAdjustment    *hadjustment;
  CtkAdjustment    *vadjustment;
  gboolean          activate_on_single_click;

  guint16           min_children_per_line;
  guint16           max_children_per_line;
  guint16           cur_children_per_line;

  GSequence        *children;

  CtkCssGadget     *gadget;
  CtkFlowBoxFilterFunc filter_func;
  gpointer             filter_data;
  GDestroyNotify       filter_destroy;

  CtkFlowBoxSortFunc sort_func;
  gpointer           sort_data;
  GDestroyNotify     sort_destroy;

  CtkGesture        *multipress_gesture;
  CtkGesture        *drag_gesture;

  CtkFlowBoxChild   *rubberband_first;
  CtkFlowBoxChild   *rubberband_last;
  CtkCssNode        *rubberband_node;
  gboolean           rubberband_select;
  gboolean           rubberband_modify;
  gboolean           rubberband_extend;

  CtkScrollType      autoscroll_mode;
  guint              autoscroll_id;

  GListModel                 *bound_model;
  CtkFlowBoxCreateWidgetFunc  create_widget_func;
  gpointer                    create_widget_func_data;
  GDestroyNotify              create_widget_func_data_destroy;
};

#define BOX_PRIV(box) ((CtkFlowBoxPrivate*)ctk_flow_box_get_instance_private ((CtkFlowBox*)(box)))

G_DEFINE_TYPE_WITH_CODE (CtkFlowBox, ctk_flow_box, CTK_TYPE_CONTAINER,
                         G_ADD_PRIVATE (CtkFlowBox)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_ORIENTABLE, NULL))

/*  Internal API, utilities {{{2 */

#define ORIENTATION_ALIGN(box)                              \
  (BOX_PRIV(box)->orientation == CTK_ORIENTATION_HORIZONTAL \
   ? ctk_widget_get_halign (CTK_WIDGET (box))               \
   : ctk_widget_get_valign (CTK_WIDGET (box)))

#define OPPOSING_ORIENTATION_ALIGN(box)                     \
  (BOX_PRIV(box)->orientation == CTK_ORIENTATION_HORIZONTAL \
   ? ctk_widget_get_valign (CTK_WIDGET (box))               \
   : ctk_widget_get_halign (CTK_WIDGET (box)))

/* Children are visible if they are shown by the app (visible)
 * and not filtered out (child_visible) by the box
 */
static inline gboolean
child_is_visible (CtkWidget *child)
{
  return ctk_widget_get_visible (child) &&
         ctk_widget_get_child_visible (child);
}

static gint
get_visible_children (CtkFlowBox *box)
{
  GSequenceIter *iter;
  gint i = 0;

  for (iter = g_sequence_get_begin_iter (BOX_PRIV (box)->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      CtkWidget *child;

      child = g_sequence_get (iter);
      if (child_is_visible (child))
        i++;
    }

  return i;
}

static void
ctk_flow_box_update_active (CtkFlowBox      *box,
                            CtkFlowBoxChild *child)
{
  CtkFlowBoxPrivate *priv = BOX_PRIV (box);
  gboolean val;

  val = priv->active_child == child;
  if (priv->active_child != NULL &&
      val != priv->active_child_active)
    {
      priv->active_child_active = val;
      ctk_widget_queue_draw (CTK_WIDGET (box));
    }
}

static void
ctk_flow_box_apply_filter (CtkFlowBox      *box,
                           CtkFlowBoxChild *child)
{
  CtkFlowBoxPrivate *priv = BOX_PRIV (box);
  gboolean do_show;

  do_show = TRUE;
  if (priv->filter_func != NULL)
    do_show = priv->filter_func (child, priv->filter_data);

  ctk_widget_set_child_visible (CTK_WIDGET (child), do_show);
}

static void
ctk_flow_box_apply_filter_all (CtkFlowBox *box)
{
  GSequenceIter *iter;

  for (iter = g_sequence_get_begin_iter (BOX_PRIV (box)->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      CtkFlowBoxChild *child;

      child = g_sequence_get (iter);
      ctk_flow_box_apply_filter (box, child);
    }
  ctk_widget_queue_resize (CTK_WIDGET (box));
}

static void
ctk_flow_box_apply_sort (CtkFlowBox      *box,
                         CtkFlowBoxChild *child)
{
  if (BOX_PRIV (box)->sort_func != NULL)
    {
      g_sequence_sort_changed (CHILD_PRIV (child)->iter,
                               (GCompareDataFunc)ctk_flow_box_sort, box);
      ctk_widget_queue_resize (CTK_WIDGET (box));
    }
}

/* Selection utilities {{{3 */

static gboolean
ctk_flow_box_child_set_selected (CtkFlowBoxChild *child,
                                 gboolean         selected)
{
  if (CHILD_PRIV (child)->selected != selected)
    {
      CHILD_PRIV (child)->selected = selected;
      if (selected)
        ctk_widget_set_state_flags (CTK_WIDGET (child),
                                    CTK_STATE_FLAG_SELECTED, FALSE);
      else
        ctk_widget_unset_state_flags (CTK_WIDGET (child),
                                      CTK_STATE_FLAG_SELECTED);

      return TRUE;
    }

  return FALSE;
}

static gboolean
ctk_flow_box_unselect_all_internal (CtkFlowBox *box)
{
  CtkFlowBoxChild *child;
  GSequenceIter *iter;
  gboolean dirty = FALSE;

  if (BOX_PRIV (box)->selection_mode == CTK_SELECTION_NONE)
    return FALSE;

  for (iter = g_sequence_get_begin_iter (BOX_PRIV (box)->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      child = g_sequence_get (iter);
      dirty |= ctk_flow_box_child_set_selected (child, FALSE);
    }

  return dirty;
}

static void
ctk_flow_box_unselect_child_internal (CtkFlowBox      *box,
                                      CtkFlowBoxChild *child)
{
  if (!CHILD_PRIV (child)->selected)
    return;

  if (BOX_PRIV (box)->selection_mode == CTK_SELECTION_NONE)
    return;
  else if (BOX_PRIV (box)->selection_mode != CTK_SELECTION_MULTIPLE)
    ctk_flow_box_unselect_all_internal (box);
  else
    ctk_flow_box_child_set_selected (child, FALSE);

  g_signal_emit (box, signals[SELECTED_CHILDREN_CHANGED], 0);
}

static void
ctk_flow_box_update_cursor (CtkFlowBox      *box,
                            CtkFlowBoxChild *child)
{
  BOX_PRIV (box)->cursor_child = child;
  ctk_widget_grab_focus (CTK_WIDGET (child));
  ctk_widget_queue_draw (CTK_WIDGET (child));
  _ctk_flow_box_accessible_update_cursor (CTK_WIDGET (box), CTK_WIDGET (child));
}

static void
ctk_flow_box_select_child_internal (CtkFlowBox      *box,
                                    CtkFlowBoxChild *child)
{
  if (CHILD_PRIV (child)->selected)
    return;

  if (BOX_PRIV (box)->selection_mode == CTK_SELECTION_NONE)
    return;
  if (BOX_PRIV (box)->selection_mode != CTK_SELECTION_MULTIPLE)
    ctk_flow_box_unselect_all_internal (box);

  ctk_flow_box_child_set_selected (child, TRUE);
  BOX_PRIV (box)->selected_child = child;

  g_signal_emit (box, signals[SELECTED_CHILDREN_CHANGED], 0);
}

static void
ctk_flow_box_select_all_between (CtkFlowBox      *box,
                                 CtkFlowBoxChild *child1,
                                 CtkFlowBoxChild *child2,
				 gboolean         modify)
{
  GSequenceIter *iter, *iter1, *iter2;

  if (child1)
    iter1 = CHILD_PRIV (child1)->iter;
  else
    iter1 = g_sequence_get_begin_iter (BOX_PRIV (box)->children);

  if (child2)
    iter2 = CHILD_PRIV (child2)->iter;
  else
    iter2 = g_sequence_get_end_iter (BOX_PRIV (box)->children);

  if (g_sequence_iter_compare (iter2, iter1) < 0)
    {
      iter = iter1;
      iter1 = iter2;
      iter2 = iter;
    }

  for (iter = iter1;
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      CtkWidget *child;

      child = g_sequence_get (iter);
      if (child_is_visible (child))
        {
          if (modify)
            ctk_flow_box_child_set_selected (CTK_FLOW_BOX_CHILD (child), !CHILD_PRIV (child)->selected);
          else
            ctk_flow_box_child_set_selected (CTK_FLOW_BOX_CHILD (child), TRUE);
	}

      if (g_sequence_iter_compare (iter, iter2) == 0)
        break;
    }
}

static void
ctk_flow_box_update_selection (CtkFlowBox      *box,
                               CtkFlowBoxChild *child,
                               gboolean         modify,
                               gboolean         extend)
{
  CtkFlowBoxPrivate *priv = BOX_PRIV (box);

  ctk_flow_box_update_cursor (box, child);

  if (priv->selection_mode == CTK_SELECTION_NONE)
    return;

  if (priv->selection_mode == CTK_SELECTION_BROWSE)
    {
      ctk_flow_box_unselect_all_internal (box);
      ctk_flow_box_child_set_selected (child, TRUE);
      priv->selected_child = child;
    }
  else if (priv->selection_mode == CTK_SELECTION_SINGLE)
    {
      gboolean was_selected;

      was_selected = CHILD_PRIV (child)->selected;
      ctk_flow_box_unselect_all_internal (box);
      ctk_flow_box_child_set_selected (child, modify ? !was_selected : TRUE);
      priv->selected_child = CHILD_PRIV (child)->selected ? child : NULL;
    }
  else /* CTK_SELECTION_MULTIPLE */
    {
      if (extend)
        {
          ctk_flow_box_unselect_all_internal (box);
          if (priv->selected_child == NULL)
            {
              ctk_flow_box_child_set_selected (child, TRUE);
              priv->selected_child = child;
            }
          else
            ctk_flow_box_select_all_between (box, priv->selected_child, child, FALSE);
        }
      else
        {
          if (modify)
            {
              ctk_flow_box_child_set_selected (child, !CHILD_PRIV (child)->selected);
            }
          else
            {
              ctk_flow_box_unselect_all_internal (box);
              ctk_flow_box_child_set_selected (child, !CHILD_PRIV (child)->selected);
              priv->selected_child = child;
            }
        }
    }

  g_signal_emit (box, signals[SELECTED_CHILDREN_CHANGED], 0);
}

static void
ctk_flow_box_select_and_activate (CtkFlowBox      *box,
                                  CtkFlowBoxChild *child)
{
  if (child != NULL)
    {
      ctk_flow_box_select_child_internal (box, child);
      ctk_flow_box_update_cursor (box, child);
      g_signal_emit (box, signals[CHILD_ACTIVATED], 0, child);
    }
}

/* Focus utilities {{{3 */

static GSequenceIter *
ctk_flow_box_get_previous_focusable (CtkFlowBox    *box,
                                     GSequenceIter *iter)
{
  CtkFlowBoxChild *child;

  while (!g_sequence_iter_is_begin (iter))
    {
      iter = g_sequence_iter_prev (iter);
      child = g_sequence_get (iter);
      if (child_is_visible (CTK_WIDGET (child)) &&
          ctk_widget_is_sensitive (CTK_WIDGET (child)))
        return iter;
    }

  return NULL;
}

static GSequenceIter *
ctk_flow_box_get_next_focusable (CtkFlowBox    *box,
                                 GSequenceIter *iter)
{
  CtkFlowBoxChild *child;

  while (TRUE)
    {
      iter = g_sequence_iter_next (iter);
      if (g_sequence_iter_is_end (iter))
        return NULL;
      child = g_sequence_get (iter);
      if (child_is_visible (CTK_WIDGET (child)) &&
          ctk_widget_is_sensitive (CTK_WIDGET (child)))
        return iter;
    }

  return NULL;
}

static GSequenceIter *
ctk_flow_box_get_first_focusable (CtkFlowBox *box)
{
  GSequenceIter *iter;
  CtkFlowBoxChild *child;

  iter = g_sequence_get_begin_iter (BOX_PRIV (box)->children);
  if (g_sequence_iter_is_end (iter))
    return NULL;

  child = g_sequence_get (iter);
  if (child_is_visible (CTK_WIDGET (child)) &&
      ctk_widget_is_sensitive (CTK_WIDGET (child)))
    return iter;

  return ctk_flow_box_get_next_focusable (box, iter);
}

static GSequenceIter *
ctk_flow_box_get_last_focusable (CtkFlowBox *box)
{
  GSequenceIter *iter;

  iter = g_sequence_get_end_iter (BOX_PRIV (box)->children);
  return ctk_flow_box_get_previous_focusable (box, iter);
}


static GSequenceIter *
ctk_flow_box_get_above_focusable (CtkFlowBox    *box,
                                  GSequenceIter *iter)
{
  CtkFlowBoxChild *child = NULL;
  gint i;

  while (TRUE)
    {
      i = 0;
      while (i < BOX_PRIV (box)->cur_children_per_line)
        {
          if (g_sequence_iter_is_begin (iter))
            return NULL;
          iter = g_sequence_iter_prev (iter);
          child = g_sequence_get (iter);
          if (child_is_visible (CTK_WIDGET (child)))
            i++;
        }
      if (child && ctk_widget_get_sensitive (CTK_WIDGET (child)))
        return iter;
    }

  return NULL;
}

static GSequenceIter *
ctk_flow_box_get_below_focusable (CtkFlowBox    *box,
                                  GSequenceIter *iter)
{
  CtkFlowBoxChild *child = NULL;
  gint i;

  while (TRUE)
    {
      i = 0;
      while (i < BOX_PRIV (box)->cur_children_per_line)
        {
          iter = g_sequence_iter_next (iter);
          if (g_sequence_iter_is_end (iter))
            return NULL;
          child = g_sequence_get (iter);
          if (child_is_visible (CTK_WIDGET (child)))
            i++;
        }
      if (child && ctk_widget_get_sensitive (CTK_WIDGET (child)))
        return iter;
    }

  return NULL;
}

/* CtkWidget implementation {{{2 */

/* Size allocation {{{3 */

/* Used in columned modes where all items share at least their
 * equal widths or heights
 */
static void
get_max_item_size (CtkFlowBox     *box,
                   CtkOrientation  orientation,
                   gint           *min_size,
                   gint           *nat_size)
{
  GSequenceIter *iter;
  gint max_min_size = 0;
  gint max_nat_size = 0;

  for (iter = g_sequence_get_begin_iter (BOX_PRIV (box)->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      CtkWidget *child;
      gint child_min, child_nat;

      child = g_sequence_get (iter);

      if (!child_is_visible (child))
        continue;

      if (orientation == CTK_ORIENTATION_HORIZONTAL)
        ctk_widget_get_preferred_width (child, &child_min, &child_nat);
      else
        ctk_widget_get_preferred_height (child, &child_min, &child_nat);

      max_min_size = MAX (max_min_size, child_min);
      max_nat_size = MAX (max_nat_size, child_nat);
    }

  if (min_size)
    *min_size = max_min_size;

  if (nat_size)
    *nat_size = max_nat_size;
}


/* Gets the largest minimum/natural size for a given size (used to get
 * the largest item heights for a fixed item width and the opposite)
 */
static void
get_largest_size_for_opposing_orientation (CtkFlowBox     *box,
                                           CtkOrientation  orientation,
                                           gint            item_size,
                                           gint           *min_item_size,
                                           gint           *nat_item_size)
{
  GSequenceIter *iter;
  gint max_min_size = 0;
  gint max_nat_size = 0;

  for (iter = g_sequence_get_begin_iter (BOX_PRIV (box)->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      CtkWidget *child;
      gint       child_min, child_nat;

      child = g_sequence_get (iter);

      if (!child_is_visible (child))
        continue;

      if (orientation == CTK_ORIENTATION_HORIZONTAL)
        ctk_widget_get_preferred_height_for_width (child,
                                                   item_size,
                                                   &child_min, &child_nat);
      else
        ctk_widget_get_preferred_width_for_height (child,
                                                   item_size,
                                                   &child_min, &child_nat);

      max_min_size = MAX (max_min_size, child_min);
      max_nat_size = MAX (max_nat_size, child_nat);
    }

  if (min_item_size)
    *min_item_size = max_min_size;

  if (nat_item_size)
    *nat_item_size = max_nat_size;
}

/* Gets the largest minimum/natural size on a single line for a given size
 * (used to get the largest line heights for a fixed item width and the opposite
 * while iterating over a list of children, note the new index is returned)
 */
static GSequenceIter *
get_largest_size_for_line_in_opposing_orientation (CtkFlowBox       *box,
                                                   CtkOrientation    orientation,
                                                   GSequenceIter    *cursor,
                                                   gint              line_length,
                                                   CtkRequestedSize *item_sizes,
                                                   gint              extra_pixels,
                                                   gint             *min_item_size,
                                                   gint             *nat_item_size)
{
  GSequenceIter *iter;
  gint max_min_size = 0;
  gint max_nat_size = 0;
  gint i;

  i = 0;
  for (iter = cursor;
       !g_sequence_iter_is_end (iter) && i < line_length;
       iter = g_sequence_iter_next (iter))
    {
      CtkWidget *child;
      gint child_min, child_nat, this_item_size;

      child = g_sequence_get (iter);

      if (!child_is_visible (child))
        continue;

      /* Distribute the extra pixels to the first children in the line
       * (could be fancier and spread them out more evenly) */
      this_item_size = item_sizes[i].minimum_size;
      if (extra_pixels > 0 && ORIENTATION_ALIGN (box) == CTK_ALIGN_FILL)
        {
          this_item_size++;
          extra_pixels--;
        }

      if (orientation == CTK_ORIENTATION_HORIZONTAL)
        ctk_widget_get_preferred_height_for_width (child,
                                                   this_item_size,
                                                   &child_min, &child_nat);
      else
        ctk_widget_get_preferred_width_for_height (child,
                                                   this_item_size,
                                                   &child_min, &child_nat);

      max_min_size = MAX (max_min_size, child_min);
      max_nat_size = MAX (max_nat_size, child_nat);

      i++;
    }

  if (min_item_size)
    *min_item_size = max_min_size;

  if (nat_item_size)
    *nat_item_size = max_nat_size;

  /* Return next item in the list */
  return iter;
}

/* fit_aligned_item_requests() helper */
static gint
gather_aligned_item_requests (CtkFlowBox       *box,
                              CtkOrientation    orientation,
                              gint              line_length,
                              gint              item_spacing,
                              gint              n_children,
                              CtkRequestedSize *item_sizes)
{
  GSequenceIter *iter;
  gint i;
  gint extra_items, natural_line_size = 0;

  extra_items = n_children % line_length;

  i = 0;
  for (iter = g_sequence_get_begin_iter (BOX_PRIV (box)->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      CtkWidget *child;
      CtkAlign item_align;
      gint child_min, child_nat;
      gint position;

      child = g_sequence_get (iter);

      if (!child_is_visible (child))
        continue;

      if (orientation == CTK_ORIENTATION_HORIZONTAL)
        ctk_widget_get_preferred_width (child,
                                        &child_min, &child_nat);
      else
        ctk_widget_get_preferred_height (child,
                                         &child_min, &child_nat);

      /* Get the index and push it over for the last line when spreading to the end */
      position = i % line_length;

      item_align = ORIENTATION_ALIGN (box);
      if (item_align == CTK_ALIGN_END && i >= n_children - extra_items)
        position += line_length - extra_items;

      /* Round up the size of every column/row */
      item_sizes[position].minimum_size = MAX (item_sizes[position].minimum_size, child_min);
      item_sizes[position].natural_size = MAX (item_sizes[position].natural_size, child_nat);

      i++;
    }

  for (i = 0; i < line_length; i++)
    natural_line_size += item_sizes[i].natural_size;

  natural_line_size += (line_length - 1) * item_spacing;

  return natural_line_size;
}

static CtkRequestedSize *
fit_aligned_item_requests (CtkFlowBox     *box,
                           CtkOrientation  orientation,
                           gint            avail_size,
                           gint            item_spacing,
                           gint           *line_length, /* in-out */
                           gint            items_per_line,
                           gint            n_children)
{
  CtkRequestedSize *sizes, *try_sizes;
  gint try_line_size, try_length;

  sizes = g_new0 (CtkRequestedSize, *line_length);

  /* get the sizes for the initial guess */
  try_line_size = gather_aligned_item_requests (box,
                                                orientation,
                                                *line_length,
                                                item_spacing,
                                                n_children,
                                                sizes);

  /* Try columnizing the whole thing and adding an item to the end of
   * the line; try to fit as many columns into the available size as
   * possible
   */
  for (try_length = *line_length + 1; try_line_size < avail_size; try_length++)
    {
      try_sizes = g_new0 (CtkRequestedSize, try_length);
      try_line_size = gather_aligned_item_requests (box,
                                                    orientation,
                                                    try_length,
                                                    item_spacing,
                                                    n_children,
                                                    try_sizes);

      if (try_line_size <= avail_size &&
          items_per_line >= try_length)
        {
          *line_length = try_length;

          g_free (sizes);
          sizes = try_sizes;
        }
      else
        {
          /* oops, this one failed; stick to the last size that fit and then return */
          g_free (try_sizes);
          break;
        }
    }

  return sizes;
}

typedef struct {
  GArray *requested;
  gint    extra_pixels;
} AllocatedLine;

static gint
get_offset_pixels (CtkAlign align,
                   gint     pixels)
{
  gint offset;

  switch (align) {
  case CTK_ALIGN_START:
  case CTK_ALIGN_FILL:
    offset = 0;
    break;
  case CTK_ALIGN_CENTER:
    offset = pixels / 2;
    break;
  case CTK_ALIGN_END:
    offset = pixels;
    break;
  default:
    g_assert_not_reached ();
    break;
  }

  return offset;
}

static void
ctk_flow_box_size_allocate (CtkWidget     *widget,
                            CtkAllocation *allocation)
{
  CtkAllocation clip;
  GdkWindow *window;
  CtkAllocation child_allocation;

  ctk_widget_set_allocation (widget, allocation);

  window = ctk_widget_get_window (widget);
  if (window != NULL)
    gdk_window_move_resize (window,
                            allocation->x, allocation->y,
                            allocation->width, allocation->height);

  child_allocation.x = 0;
  child_allocation.y = 0;
  child_allocation.width = allocation->width;
  child_allocation.height = allocation->height;

  ctk_css_gadget_allocate (BOX_PRIV (widget)->gadget,
                           &child_allocation,
                           ctk_widget_get_allocated_baseline (widget),
                           &clip);

  _ctk_widget_set_simple_clip (widget, &clip);
}

static void
ctk_flow_box_allocate (CtkCssGadget        *gadget,
                       const CtkAllocation *allocation,
                       int                  baseline,
                       CtkAllocation       *out_clip,
                       gpointer             unused)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkFlowBox *box = CTK_FLOW_BOX (widget);
  CtkFlowBoxPrivate  *priv = BOX_PRIV (box);
  CtkAllocation child_allocation;
  gint avail_size, avail_other_size, min_items, item_spacing, line_spacing;
  CtkAlign item_align;
  CtkAlign line_align;
  CtkRequestedSize *line_sizes = NULL;
  CtkRequestedSize *item_sizes = NULL;
  gint min_item_size, nat_item_size;
  gint line_length;
  gint item_size = 0;
  gint line_size = 0, min_fixed_line_size = 0, nat_fixed_line_size = 0;
  gint line_offset, item_offset, n_children, n_lines, line_count;
  gint extra_pixels = 0, extra_per_item = 0, extra_extra = 0;
  gint extra_line_pixels = 0, extra_per_line = 0, extra_line_extra = 0;
  gint i, this_line_size;
  GSequenceIter *iter;

  min_items = MAX (1, priv->min_children_per_line);

  if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      avail_size = allocation->width;
      avail_other_size = allocation->height;
      item_spacing = priv->column_spacing; line_spacing = priv->row_spacing;
    }
  else /* CTK_ORIENTATION_VERTICAL */
    {
      avail_size = allocation->height;
      avail_other_size = allocation->width;
      item_spacing = priv->row_spacing;
      line_spacing = priv->column_spacing;
    }

  item_align = ORIENTATION_ALIGN (box);
  line_align = OPPOSING_ORIENTATION_ALIGN (box);

  /* Get how many lines we'll be needing to flow */
  n_children = get_visible_children (box);
  if (n_children <= 0)
    return;

  /* Deal with ALIGNED/HOMOGENEOUS modes first, start with
   * initial guesses at item/line sizes
   */
  get_max_item_size (box, priv->orientation, &min_item_size, &nat_item_size);
  if (nat_item_size <= 0)
    return;

  /* By default flow at the natural item width */
  line_length = avail_size / (nat_item_size + item_spacing);

  /* After the above aproximation, check if we cant fit one more on the line */
  if (line_length * item_spacing + (line_length + 1) * nat_item_size <= avail_size)
    line_length++;

  /* Its possible we were allocated just less than the natural width of the
   * minimum item flow length
   */
  line_length = MAX (min_items, line_length);
  line_length = MIN (line_length, priv->max_children_per_line);

  /* Here we just use the largest height-for-width and use that for the height
   * of all lines
   */
  if (priv->homogeneous)
    {
      n_lines = n_children / line_length;
      if ((n_children % line_length) > 0)
        n_lines++;

      n_lines = MAX (n_lines, 1);

      /* Now we need the real item allocation size */
      item_size = (avail_size - (line_length - 1) * item_spacing) / line_length;

      /* Cut out the expand space if we're not distributing any */
      if (item_align != CTK_ALIGN_FILL)
        item_size = MIN (item_size, nat_item_size);

      get_largest_size_for_opposing_orientation (box,
                                                 priv->orientation,
                                                 item_size,
                                                 &min_fixed_line_size,
                                                 &nat_fixed_line_size);

      /* resolve a fixed 'line_size' */
      line_size = (avail_other_size - (n_lines - 1) * line_spacing) / n_lines;

      if (line_align != CTK_ALIGN_FILL)
        line_size = MIN (line_size, nat_fixed_line_size);

      /* Get the real extra pixels incase of CTK_ALIGN_START lines */
      extra_pixels = avail_size - (line_length - 1) * item_spacing - item_size * line_length;
      extra_line_pixels = avail_other_size - (n_lines - 1) * line_spacing - line_size * n_lines;
    }
  else
    {
      gboolean first_line = TRUE;

      /* Find the amount of columns that can fit aligned into the available space
       * and collect their requests.
       */
      item_sizes = fit_aligned_item_requests (box,
                                              priv->orientation,
                                              avail_size,
                                              item_spacing,
                                              &line_length,
                                              priv->max_children_per_line,
                                              n_children);

      /* Calculate the number of lines after determining the final line_length */
      n_lines = n_children / line_length;
      if ((n_children % line_length) > 0)
        n_lines++;

      n_lines = MAX (n_lines, 1);
      line_sizes = g_new0 (CtkRequestedSize, n_lines);

      /* Get the available remaining size */
      avail_size -= (line_length - 1) * item_spacing;
      for (i = 0; i < line_length; i++)
        avail_size -= item_sizes[i].minimum_size;

      /* Perform a natural allocation on the columnized items and get the remaining pixels */
      if (avail_size > 0)
        extra_pixels = ctk_distribute_natural_allocation (avail_size, line_length, item_sizes);

      /* Now that we have the size of each column of items find the size of each individual
       * line based on the aligned item sizes.
       */

      for (i = 0, iter = g_sequence_get_begin_iter (priv->children);
           !g_sequence_iter_is_end (iter) && i < n_lines;
           i++)
        {
          iter = get_largest_size_for_line_in_opposing_orientation (box,
                                                                    priv->orientation,
                                                                    iter,
                                                                    line_length,
                                                                    item_sizes,
                                                                    extra_pixels,
                                                                    &line_sizes[i].minimum_size,
                                                                    &line_sizes[i].natural_size);


          /* Its possible a line is made of completely invisible children */
          if (line_sizes[i].natural_size > 0)
            {
              if (first_line)
                first_line = FALSE;
              else
                avail_other_size -= line_spacing;

              avail_other_size -= line_sizes[i].minimum_size;

              line_sizes[i].data = GINT_TO_POINTER (i);
            }
        }

      /* Distribute space among lines naturally */
      if (avail_other_size > 0)
        extra_line_pixels = ctk_distribute_natural_allocation (avail_other_size, n_lines, line_sizes);
    }

  /*
   * Initial sizes of items/lines guessed at this point,
   * go on to distribute expand space if needed.
   */

  priv->cur_children_per_line = line_length;

  /* FIXME: This portion needs to consider which columns
   * and rows asked for expand space and distribute those
   * accordingly for the case of ALIGNED allocation.
   *
   * If at least one child in a column/row asked for expand;
   * we should make that row/column expand entirely.
   */

  /* Calculate expand space per item */
  if (item_align == CTK_ALIGN_FILL)
    {
      extra_per_item = extra_pixels / line_length;
      extra_extra    = extra_pixels % line_length;
    }

  /* Calculate expand space per line */
  if (line_align == CTK_ALIGN_FILL)
    {
      extra_per_line   = extra_line_pixels / n_lines;
      extra_line_extra = extra_line_pixels % n_lines;
    }

  /*
   * Prepare item/line initial offsets and jump into the
   * real allocation loop.
   */
  line_offset = allocation->y;
  item_offset = allocation->x;

  /* prepend extra space to item_offset/line_offset for SPREAD_END */
  item_offset += get_offset_pixels (item_align, extra_pixels);
  line_offset += get_offset_pixels (line_align, extra_line_pixels);

  /* Get the allocation size for the first line */
  if (priv->homogeneous)
    this_line_size = line_size;
  else
    {
      this_line_size = line_sizes[0].minimum_size;

      if (line_align == CTK_ALIGN_FILL)
        {
          this_line_size += extra_per_line;

          if (extra_line_extra > 0)
            this_line_size++;
        }
    }

  i = 0;
  line_count = 0;
  for (iter = g_sequence_get_begin_iter (priv->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      CtkWidget *child;
      gint position;
      gint this_item_size;

      child = g_sequence_get (iter);

      if (!child_is_visible (child))
        continue;

      /* Get item position */
      position = i % line_length;

      /* adjust the line_offset/count at the beginning of each new line */
      if (i > 0 && position == 0)
        {
          /* Push the line_offset */
          line_offset += this_line_size + line_spacing;

          line_count++;

          /* Get the new line size */
          if (priv->homogeneous)
            this_line_size = line_size;
          else
            {
              this_line_size = line_sizes[line_count].minimum_size;

              if (line_align == CTK_ALIGN_FILL)
                {
                  this_line_size += extra_per_line;

                  if (line_count < extra_line_extra)
                    this_line_size++;
                }
            }

          item_offset = allocation->x;

          if (item_align == CTK_ALIGN_CENTER)
            {
              item_offset += get_offset_pixels (item_align, extra_pixels);
            }
          else if (item_align == CTK_ALIGN_END)
            {
              item_offset += get_offset_pixels (item_align, extra_pixels);

              /* If we're on the last line, prepend the space for
               * any leading items */
              if (line_count == n_lines -1)
                {
                  gint extra_items = n_children % line_length;

                  if (priv->homogeneous)
                    {
                      item_offset += item_size * (line_length - extra_items);
                      item_offset += item_spacing * (line_length - extra_items);
                    }
                  else
                    {
                      gint j;

                      for (j = 0; j < (line_length - extra_items); j++)
                        {
                          item_offset += item_sizes[j].minimum_size;
                          item_offset += item_spacing;
                        }
                    }
                }
            }
        }

      /* Push the index along for the last line when spreading to the end */
      if (item_align == CTK_ALIGN_END && line_count == n_lines -1)
        {
          gint extra_items = n_children % line_length;

          position += line_length - extra_items;
        }

      if (priv->homogeneous)
        this_item_size = item_size;
      else
        this_item_size = item_sizes[position].minimum_size;

      if (item_align == CTK_ALIGN_FILL)
        {
          this_item_size += extra_per_item;

          if (position < extra_extra)
            this_item_size++;
        }

      /* Do the actual allocation */
      if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
        {
          child_allocation.x = item_offset;
          child_allocation.y = line_offset;
          child_allocation.width = this_item_size;
          child_allocation.height = this_line_size;
        }
      else /* CTK_ORIENTATION_VERTICAL */
        {
          child_allocation.x = line_offset;
          child_allocation.y = item_offset;
          child_allocation.width = this_line_size;
          child_allocation.height = this_item_size;
        }

      if (ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL)
        child_allocation.x = allocation->width - child_allocation.x - child_allocation.width;
      ctk_widget_size_allocate (child, &child_allocation);

      item_offset += this_item_size;
      item_offset += item_spacing;

      i++;
    }

  g_free (item_sizes);
  g_free (line_sizes);

  ctk_container_get_children_clip (CTK_CONTAINER (widget), out_clip);
}

static CtkSizeRequestMode
ctk_flow_box_get_request_mode (CtkWidget *widget)
{
  CtkFlowBox *box = CTK_FLOW_BOX (widget);

  return (BOX_PRIV (box)->orientation == CTK_ORIENTATION_HORIZONTAL) ?
    CTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH : CTK_SIZE_REQUEST_WIDTH_FOR_HEIGHT;
}

/* Gets the largest minimum and natural length of
 * 'line_length' consecutive items when aligned into rows/columns */
static void
get_largest_aligned_line_length (CtkFlowBox     *box,
                                 CtkOrientation  orientation,
                                 gint            line_length,
                                 gint           *min_size,
                                 gint           *nat_size)
{
  GSequenceIter *iter;
  gint max_min_size = 0;
  gint max_nat_size = 0;
  gint spacing, i;
  CtkRequestedSize *aligned_item_sizes;

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    spacing = BOX_PRIV (box)->column_spacing;
  else
    spacing = BOX_PRIV (box)->row_spacing;

  aligned_item_sizes = g_new0 (CtkRequestedSize, line_length);

  /* Get the largest sizes of each index in the line.
   */
  i = 0;
  for (iter = g_sequence_get_begin_iter (BOX_PRIV (box)->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      CtkWidget *child;
      gint child_min, child_nat;

      child = g_sequence_get (iter);
      if (!child_is_visible (child))
        continue;

      if (orientation == CTK_ORIENTATION_HORIZONTAL)
        ctk_widget_get_preferred_width (child,
                                        &child_min, &child_nat);
      else /* CTK_ORIENTATION_VERTICAL */
        ctk_widget_get_preferred_height (child,
                                         &child_min, &child_nat);

      aligned_item_sizes[i % line_length].minimum_size =
        MAX (aligned_item_sizes[i % line_length].minimum_size, child_min);

      aligned_item_sizes[i % line_length].natural_size =
        MAX (aligned_item_sizes[i % line_length].natural_size, child_nat);

      i++;
    }

  /* Add up the largest indexes */
  for (i = 0; i < line_length; i++)
    {
      max_min_size += aligned_item_sizes[i].minimum_size;
      max_nat_size += aligned_item_sizes[i].natural_size;
    }

  g_free (aligned_item_sizes);

  max_min_size += (line_length - 1) * spacing;
  max_nat_size += (line_length - 1) * spacing;

  if (min_size)
    *min_size = max_min_size;

  if (nat_size)
    *nat_size = max_nat_size;
}

static void
ctk_flow_box_get_preferred_width (CtkWidget *widget,
                                  gint      *minimum,
                                  gint      *natural)
{
  ctk_css_gadget_get_preferred_size (BOX_PRIV (widget)->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_flow_box_get_preferred_height (CtkWidget *widget,
                                   gint      *minimum,
                                   gint      *natural)
{
  ctk_css_gadget_get_preferred_size (BOX_PRIV (widget)->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_flow_box_get_preferred_height_for_width (CtkWidget *widget,
                                             gint       width,
                                             gint      *minimum,
                                             gint      *natural)
{
  ctk_css_gadget_get_preferred_size (BOX_PRIV (widget)->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     width,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_flow_box_get_preferred_width_for_height (CtkWidget *widget,
                                             gint       height,
                                             gint      *minimum,
                                             gint      *natural)
{
  ctk_css_gadget_get_preferred_size (BOX_PRIV (widget)->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     height,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_flow_box_measure (CtkCssGadget   *gadget,
                      CtkOrientation  orientation,
                      int             for_size,
                      int            *minimum,
                      int            *natural,
                      int            *minimum_baseline,
                      int            *natural_baseline,
                      gpointer        data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkFlowBox *box = CTK_FLOW_BOX (widget);
  CtkFlowBoxPrivate *priv = BOX_PRIV (box);

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      if (for_size < 0)
        {
          gint min_item_width, nat_item_width;
          gint min_items, nat_items;
          gint min_width, nat_width;

          min_items = MAX (1, priv->min_children_per_line);
          nat_items = MAX (min_items, priv->max_children_per_line);

          if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
            {
              min_width = nat_width = 0;

              if (!priv->homogeneous)
                {
                  /* When not homogeneous; horizontally oriented boxes
                   * need enough width for the widest row
                   */
                  if (min_items == 1)
                    {
                      get_max_item_size (box,
                                         CTK_ORIENTATION_HORIZONTAL,
                                         &min_item_width,
                                         &nat_item_width);

                      min_width += min_item_width;
                      nat_width += nat_item_width;
                    }
                  else
                    {
                      gint min_line_length, nat_line_length;

                      get_largest_aligned_line_length (box,
                                                       CTK_ORIENTATION_HORIZONTAL,
                                                       min_items,
                                                       &min_line_length,
                                                       &nat_line_length);

                      if (nat_items > min_items)
                        get_largest_aligned_line_length (box,
                                                         CTK_ORIENTATION_HORIZONTAL,
                                                         nat_items,
                                                         NULL,
                                                         &nat_line_length);

                      min_width += min_line_length;
                      nat_width += nat_line_length;
                    }
                }
              else /* In homogeneous mode; horizontally oriented boxs
                    * give the same width to all children */
                {
                  get_max_item_size (box, CTK_ORIENTATION_HORIZONTAL,
                                     &min_item_width, &nat_item_width);

                  min_width += min_item_width * min_items;
                  min_width += (min_items -1) * priv->column_spacing;

                  nat_width += nat_item_width * nat_items;
                  nat_width += (nat_items -1) * priv->column_spacing;
                }
            }
          else /* CTK_ORIENTATION_VERTICAL */
            {
              /* Return the width for the minimum height */
              gint min_height;

              ctk_css_gadget_get_preferred_size (gadget,
                                                 CTK_ORIENTATION_VERTICAL,
                                                 -1,
                                                 &min_height, NULL,
                                                 NULL, NULL);
              ctk_css_gadget_get_preferred_size (gadget,
                                                 CTK_ORIENTATION_HORIZONTAL,
                                                 min_height,
                                                 &min_width, &nat_width,
                                                 NULL, NULL);
            }

          *minimum = min_width;
          *natural = nat_width;
        }
      else
        {
          gint min_item_height, nat_item_height;
          gint min_items;
          gint min_width, nat_width;
          gint avail_size, n_children;

          min_items = MAX (1, priv->min_children_per_line);

          min_width = 0;
          nat_width = 0;

          if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
            {
              /* Return the minimum width */
              ctk_css_gadget_get_preferred_size (gadget,
                                                 CTK_ORIENTATION_HORIZONTAL,
                                                 -1,
                                                 &min_width, &nat_width,
                                                 NULL, NULL);
            }
          else /* CTK_ORIENTATION_VERTICAL */
            {
              gint min_height;
              gint line_length;
              gint item_size, extra_pixels;

              n_children = get_visible_children (box);
              if (n_children <= 0)
                goto out_width;

              /* Make sure its no smaller than the minimum */
              ctk_css_gadget_get_preferred_size (gadget,
                                                 CTK_ORIENTATION_VERTICAL,
                                                 -1,
                                                 &min_height, NULL,
                                                 NULL, NULL);

              avail_size = MAX (for_size, min_height);
              if (avail_size <= 0)
                goto out_width;

              get_max_item_size (box, CTK_ORIENTATION_VERTICAL, &min_item_height, &nat_item_height);

              /* By default flow at the natural item width */
              line_length = avail_size / (nat_item_height + priv->row_spacing);

              /* After the above aproximation, check if we cant fit one more on the line */
              if (line_length * priv->row_spacing + (line_length + 1) * nat_item_height <= avail_size)
                line_length++;

              /* Its possible we were allocated just less than the natural width of the
               * minimum item flow length
               */
              line_length = MAX (min_items, line_length);
              line_length = MIN (line_length, priv->max_children_per_line);

              /* Now we need the real item allocation size */
              item_size = (avail_size - (line_length - 1) * priv->row_spacing) / line_length;

              /* Cut out the expand space if we're not distributing any */
              if (ctk_widget_get_valign (widget) != CTK_ALIGN_FILL)
                {
                  item_size = MIN (item_size, nat_item_height);
                  extra_pixels = 0;
                }
              else
                /* Collect the extra pixels for expand children */
                extra_pixels = (avail_size - (line_length - 1) * priv->row_spacing) % line_length;

              if (priv->homogeneous)
                {
                  gint min_item_width, nat_item_width;
                  gint lines;

                  /* Here we just use the largest height-for-width and
                   * add up the size accordingly
                   */
                  get_largest_size_for_opposing_orientation (box,
                                                             CTK_ORIENTATION_VERTICAL,
                                                             item_size,
                                                             &min_item_width,
                                                             &nat_item_width);

                  /* Round up how many lines we need to allocate for */
                  n_children = get_visible_children (box);
                  lines = n_children / line_length;
                  if ((n_children % line_length) > 0)
                    lines++;

                  min_width = min_item_width * lines;
                  nat_width = nat_item_width * lines;

                  min_width += (lines - 1) * priv->column_spacing;
                  nat_width += (lines - 1) * priv->column_spacing;
                }
              else
                {
                  gint min_line_width, nat_line_width, i;
                  gboolean first_line = TRUE;
                  CtkRequestedSize *item_sizes;
                  GSequenceIter *iter;

                  /* First get the size each set of items take to span the line
                   * when aligning the items above and below after flowping.
                   */
                  item_sizes = fit_aligned_item_requests (box,
                                                          priv->orientation,
                                                          avail_size,
                                                          priv->row_spacing,
                                                          &line_length,
                                                          priv->max_children_per_line,
                                                          n_children);

                  /* Get the available remaining size */
                  avail_size -= (line_length - 1) * priv->column_spacing;
                  for (i = 0; i < line_length; i++)
                    avail_size -= item_sizes[i].minimum_size;

                  if (avail_size > 0)
                    extra_pixels = ctk_distribute_natural_allocation (avail_size, line_length, item_sizes);

                  for (iter = g_sequence_get_begin_iter (priv->children);
                       !g_sequence_iter_is_end (iter);)
                    {
                      iter = get_largest_size_for_line_in_opposing_orientation (box,
                                                                                CTK_ORIENTATION_VERTICAL,
                                                                                iter,
                                                                                line_length,
                                                                                item_sizes,
                                                                                extra_pixels,
                                                                                &min_line_width,
                                                                                &nat_line_width);

                      /* Its possible the last line only had invisible widgets */
                      if (nat_line_width > 0)
                        {
                          if (first_line)
                            first_line = FALSE;
                          else
                            {
                              min_width += priv->column_spacing;
                              nat_width += priv->column_spacing;
                            }

                          min_width += min_line_width;
                          nat_width += nat_line_width;
                        }
                    }
                  g_free (item_sizes);
                }
            }

         out_width:
          *minimum = min_width;
          *natural = nat_width;
        }
    }
  else
    {
      if (for_size < 0)
        {
          gint min_item_height, nat_item_height;
          gint min_items, nat_items;
          gint min_height, nat_height;

          min_items = MAX (1, priv->min_children_per_line);
          nat_items = MAX (min_items, priv->max_children_per_line);

          if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
            {
              /* Return the height for the minimum width */
              gint min_width;

              ctk_css_gadget_get_preferred_size (gadget,
                                                 CTK_ORIENTATION_HORIZONTAL,
                                                 -1,
                                                 &min_width, NULL,
                                                 NULL, NULL);
              ctk_css_gadget_get_preferred_size (gadget,
                                                 CTK_ORIENTATION_VERTICAL,
                                                 min_width,
                                                 &min_height, &nat_height,
                                                 NULL, NULL);
            }
          else /* CTK_ORIENTATION_VERTICAL */
            {
              min_height = nat_height = 0;

              if (! priv->homogeneous)
                {
                  /* When not homogeneous; vertically oriented boxes
                   * need enough height for the tallest column
                   */
                  if (min_items == 1)
                    {
                      get_max_item_size (box, CTK_ORIENTATION_VERTICAL,
                                         &min_item_height, &nat_item_height);

                      min_height += min_item_height;
                      nat_height += nat_item_height;
                    }
                  else
                    {
                      gint min_line_length, nat_line_length;

                      get_largest_aligned_line_length (box,
                                                       CTK_ORIENTATION_VERTICAL,
                                                       min_items,
                                                       &min_line_length,
                                                       &nat_line_length);

                      if (nat_items > min_items)
                        get_largest_aligned_line_length (box,
                                                         CTK_ORIENTATION_VERTICAL,
                                                         nat_items,
                                                         NULL,
                                                         &nat_line_length);

                      min_height += min_line_length;
                      nat_height += nat_line_length;
                    }

                }
              else
                {
                  /* In homogeneous mode; vertically oriented boxes
                   * give the same height to all children
                   */
                  get_max_item_size (box,
                                     CTK_ORIENTATION_VERTICAL,
                                     &min_item_height,
                                     &nat_item_height);

                  min_height += min_item_height * min_items;
                  min_height += (min_items -1) * priv->row_spacing;

                  nat_height += nat_item_height * nat_items;
                  nat_height += (nat_items -1) * priv->row_spacing;
                }
            }

          *minimum = min_height;
          *natural = nat_height;
        }
      else
        {
          gint min_item_width, nat_item_width;
          gint min_items;
          gint min_height, nat_height;
          gint avail_size, n_children;

          min_items = MAX (1, priv->min_children_per_line);

          min_height = 0;
          nat_height = 0;

          if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
            {
              gint min_width;
              gint line_length;
              gint item_size, extra_pixels;

              n_children = get_visible_children (box);
              if (n_children <= 0)
                goto out_height;

              /* Make sure its no smaller than the minimum */
              ctk_css_gadget_get_preferred_size (gadget,
                                                 CTK_ORIENTATION_HORIZONTAL,
                                                 -1,
                                                 &min_width, NULL,
                                                 NULL, NULL);

              avail_size = MAX (for_size, min_width);
              if (avail_size <= 0)
                goto out_height;

              get_max_item_size (box, CTK_ORIENTATION_HORIZONTAL, &min_item_width, &nat_item_width);
              if (nat_item_width <= 0)
                goto out_height;

              /* By default flow at the natural item width */
              line_length = avail_size / (nat_item_width + priv->column_spacing);

              /* After the above aproximation, check if we cant fit one more on the line */
              if (line_length * priv->column_spacing + (line_length + 1) * nat_item_width <= avail_size)
                line_length++;

              /* Its possible we were allocated just less than the natural width of the
               * minimum item flow length
               */
              line_length = MAX (min_items, line_length);
              line_length = MIN (line_length, priv->max_children_per_line);

              /* Now we need the real item allocation size */
              item_size = (avail_size - (line_length - 1) * priv->column_spacing) / line_length;

              /* Cut out the expand space if we're not distributing any */
              if (ctk_widget_get_halign (widget) != CTK_ALIGN_FILL)
                {
                  item_size    = MIN (item_size, nat_item_width);
                  extra_pixels = 0;
                }
              else
                /* Collect the extra pixels for expand children */
                extra_pixels = (avail_size - (line_length - 1) * priv->column_spacing) % line_length;

              if (priv->homogeneous)
                {
                  gint min_item_height, nat_item_height;
                  gint lines;

                  /* Here we just use the largest height-for-width and
                   * add up the size accordingly
                   */
                  get_largest_size_for_opposing_orientation (box,
                                                             CTK_ORIENTATION_HORIZONTAL,
                                                             item_size,
                                                             &min_item_height,
                                                             &nat_item_height);

                  /* Round up how many lines we need to allocate for */
                  lines = n_children / line_length;
                  if ((n_children % line_length) > 0)
                    lines++;

                  min_height = min_item_height * lines;
                  nat_height = nat_item_height * lines;

                  min_height += (lines - 1) * priv->row_spacing;
                  nat_height += (lines - 1) * priv->row_spacing;
                }
              else
                {
                  gint min_line_height, nat_line_height, i;
                  gboolean first_line = TRUE;
                  CtkRequestedSize *item_sizes;
                  GSequenceIter *iter;

                  /* First get the size each set of items take to span the line
                   * when aligning the items above and below after flowping.
                   */
                  item_sizes = fit_aligned_item_requests (box,
                                                          priv->orientation,
                                                          avail_size,
                                                          priv->column_spacing,
                                                          &line_length,
                                                          priv->max_children_per_line,
                                                          n_children);

                  /* Get the available remaining size */
                  avail_size -= (line_length - 1) * priv->column_spacing;
                  for (i = 0; i < line_length; i++)
                    avail_size -= item_sizes[i].minimum_size;

                  if (avail_size > 0)
                    extra_pixels = ctk_distribute_natural_allocation (avail_size, line_length, item_sizes);

                  for (iter = g_sequence_get_begin_iter (priv->children);
                       !g_sequence_iter_is_end (iter);)
                    {
                      iter = get_largest_size_for_line_in_opposing_orientation (box,
                                                                                CTK_ORIENTATION_HORIZONTAL,
                                                                                iter,
                                                                                line_length,
                                                                                item_sizes,
                                                                                extra_pixels,
                                                                                &min_line_height,
                                                                                &nat_line_height);
                      /* Its possible the line only had invisible widgets */
                      if (nat_line_height > 0)
                        {
                          if (first_line)
                            first_line = FALSE;
                          else
                            {
                              min_height += priv->row_spacing;
                              nat_height += priv->row_spacing;
                            }

                          min_height += min_line_height;
                          nat_height += nat_line_height;
                        }
                    }

                  g_free (item_sizes);
                }
            }
          else /* CTK_ORIENTATION_VERTICAL */
            {
              /* Return the minimum height */
              ctk_css_gadget_get_preferred_size (gadget,
                                                 CTK_ORIENTATION_VERTICAL,
                                                 -1,
                                                 &min_height, &nat_height,
                                                 NULL, NULL);
            }

         out_height:
          *minimum = min_height;
          *natural = nat_height;
        }
    }
}

/* Drawing {{{3 */

static gboolean
ctk_flow_box_draw (CtkWidget *widget,
                   cairo_t   *cr)
{
  ctk_css_gadget_draw (BOX_PRIV (widget)->gadget, cr);

  return FALSE;
}

static gboolean
ctk_flow_box_render (CtkCssGadget *gadget,
                     cairo_t      *cr,
                     int           x,
                     int           y,
                     int           width,
                     int           height,
                     gpointer      data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkFlowBox *box = CTK_FLOW_BOX (widget);
  CtkFlowBoxPrivate *priv = BOX_PRIV (box);

  CTK_WIDGET_CLASS (ctk_flow_box_parent_class)->draw (widget, cr);

  if (priv->rubberband_first && priv->rubberband_last)
    {
      CtkStyleContext *context;
      GSequenceIter *iter, *iter1, *iter2;
      GdkRectangle line_rect, rect;
      GArray *lines;
      gboolean vertical;

      context = ctk_widget_get_style_context (CTK_WIDGET (box));

      vertical = priv->orientation == CTK_ORIENTATION_VERTICAL;

      cairo_save (cr);

      context = ctk_widget_get_style_context (widget);
      ctk_style_context_save_to_node (context, priv->rubberband_node);

      iter1 = CHILD_PRIV (priv->rubberband_first)->iter;
      iter2 = CHILD_PRIV (priv->rubberband_last)->iter;

      if (g_sequence_iter_compare (iter2, iter1) < 0)
        {
          iter = iter1;
          iter1 = iter2;
          iter2 = iter;
        }

      line_rect.width = 0;
      lines = g_array_new (FALSE, FALSE, sizeof (GdkRectangle));

      for (iter = iter1;
           !g_sequence_iter_is_end (iter);
           iter = g_sequence_iter_next (iter))
        {
          CtkWidget *child;

          child = g_sequence_get (iter);
          ctk_widget_get_allocation (CTK_WIDGET (child), &rect);
          if (line_rect.width == 0)
            line_rect = rect;
          else
            {
              if ((vertical && rect.x == line_rect.x) ||
                  (!vertical && rect.y == line_rect.y))
                gdk_rectangle_union (&rect, &line_rect, &line_rect);
              else
                {
                  g_array_append_val (lines, line_rect);
                  line_rect = rect;
                }
            }

          if (g_sequence_iter_compare (iter, iter2) == 0)
            break;
        }

      if (line_rect.width != 0)
        g_array_append_val (lines, line_rect);

      if (lines->len > 0)
        {
          CtkStateFlags state;
          cairo_path_t *path;
          CtkBorder border;
          GdkRGBA border_color;

          if (vertical)
            path_from_vertical_line_rects (cr, (GdkRectangle *)lines->data, lines->len);
          else
            path_from_horizontal_line_rects (cr, (GdkRectangle *)lines->data, lines->len);

          /* For some reason we need to copy and reapply the path,
           * or it gets eaten by ctk_render_background()
           */
          path = cairo_copy_path (cr);

          cairo_save (cr);
          cairo_clip (cr);
          ctk_render_background (context, cr, x, y, width, height);
          cairo_restore (cr);

          cairo_append_path (cr, path);
          cairo_path_destroy (path);

          state = ctk_style_context_get_state (context);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
          ctk_style_context_get_border_color (context, state, &border_color);
G_GNUC_END_IGNORE_DEPRECATIONS
          ctk_style_context_get_border (context, state, &border);

          cairo_set_line_width (cr, border.left);
          gdk_cairo_set_source_rgba (cr, &border_color);
          cairo_stroke (cr);
        }
      g_array_free (lines, TRUE);

      ctk_style_context_restore (context);
      cairo_restore (cr);
    }

  return ctk_widget_has_visible_focus (widget);
}

/* Autoscrolling {{{3 */

static void
remove_autoscroll (CtkFlowBox *box)
{
  CtkFlowBoxPrivate *priv = BOX_PRIV (box);

  if (priv->autoscroll_id)
    {
      ctk_widget_remove_tick_callback (CTK_WIDGET (box), priv->autoscroll_id);
      priv->autoscroll_id = 0;
    }

  priv->autoscroll_mode = CTK_SCROLL_NONE;
}

static gboolean
autoscroll_cb (CtkWidget     *widget,
               GdkFrameClock *frame_clock,
               gpointer       data)
{
  CtkFlowBox *box = CTK_FLOW_BOX (data);
  CtkFlowBoxPrivate *priv = BOX_PRIV (box);
  CtkAdjustment *adjustment;
  gdouble factor;
  gdouble increment;
  gdouble value;

  if (priv->orientation == CTK_ORIENTATION_HORIZONTAL)
    adjustment = priv->vadjustment;
  else
    adjustment = priv->hadjustment;

  switch (priv->autoscroll_mode)
    {
    case CTK_SCROLL_STEP_FORWARD:
      factor = AUTOSCROLL_FACTOR;
      break;
    case CTK_SCROLL_STEP_BACKWARD:
      factor = - AUTOSCROLL_FACTOR;
      break;
    case CTK_SCROLL_PAGE_FORWARD:
      factor = AUTOSCROLL_FACTOR_FAST;
      break;
    case CTK_SCROLL_PAGE_BACKWARD:
      factor = - AUTOSCROLL_FACTOR_FAST;
      break;
    default:
      g_assert_not_reached ();
    }

  increment = ctk_adjustment_get_step_increment (adjustment) / factor;

  value = ctk_adjustment_get_value (adjustment);
  value += increment;
  ctk_adjustment_set_value (adjustment, value);

  if (priv->rubberband_select)
    {
      GdkEventSequence *sequence;
      gdouble x, y;
      CtkFlowBoxChild *child;

      sequence = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (priv->drag_gesture));
      ctk_gesture_get_point (priv->drag_gesture, sequence, &x, &y);

      child = ctk_flow_box_get_child_at_pos (box, x, y);

      ctk_flow_box_update_active (box, child);

      if (child != NULL)
        priv->rubberband_last = child;
    }

  return G_SOURCE_CONTINUE;
}

static void
add_autoscroll (CtkFlowBox *box)
{
  CtkFlowBoxPrivate *priv = BOX_PRIV (box);

  if (priv->autoscroll_id != 0 ||
      priv->autoscroll_mode == CTK_SCROLL_NONE)
    return;

  priv->autoscroll_id = ctk_widget_add_tick_callback (CTK_WIDGET (box),
                                                      autoscroll_cb,
                                                      box,
                                                      NULL);
}

static gboolean
get_view_rect (CtkFlowBox   *box,
               GdkRectangle *rect)
{
  CtkFlowBoxPrivate *priv = BOX_PRIV (box);
  CtkWidget *parent;
  GdkWindow *view;

  parent = ctk_widget_get_parent (CTK_WIDGET (box));
  if (CTK_IS_VIEWPORT (parent))
    {
      view = ctk_viewport_get_view_window (CTK_VIEWPORT (parent));
      rect->x = rect->y = 0;

      rect->x = ctk_adjustment_get_value (priv->hadjustment);
      rect->y = ctk_adjustment_get_value (priv->vadjustment);
      rect->width = gdk_window_get_width (view);
      rect->height = gdk_window_get_height (view);
      return TRUE;
    }

  return FALSE;
}

static void
update_autoscroll_mode (CtkFlowBox *box,
                        gint        x,
                        gint        y)
{
  CtkFlowBoxPrivate *priv = BOX_PRIV (box);
  CtkScrollType mode = CTK_SCROLL_NONE;
  GdkRectangle rect;
  gint size, pos;

  if (priv->rubberband_select && get_view_rect (box, &rect))
    {
      if (priv->orientation == CTK_ORIENTATION_VERTICAL)
        {
          size = rect.width;
          pos = x - rect.x;
        }
      else
        {
          size = rect.height;
          pos = y - rect.y;
        }

      if (pos < 0 - AUTOSCROLL_FAST_DISTANCE)
        mode = CTK_SCROLL_PAGE_BACKWARD;
      else if (pos > size + AUTOSCROLL_FAST_DISTANCE)
        mode = CTK_SCROLL_PAGE_FORWARD;
      else if (pos < 0)
        mode = CTK_SCROLL_STEP_BACKWARD;
      else if (pos > size)
        mode = CTK_SCROLL_STEP_FORWARD;
    }

  if (mode != priv->autoscroll_mode)
    {
      remove_autoscroll (box);
      priv->autoscroll_mode = mode;
      add_autoscroll (box);
    }
}

/* Event handling {{{3 */

static gboolean
ctk_flow_box_enter_notify_event (CtkWidget        *widget,
                                 GdkEventCrossing *event)
{
  CtkFlowBox *box = CTK_FLOW_BOX (widget);
  CtkFlowBoxChild *child;

  if (event->window != ctk_widget_get_window (CTK_WIDGET (box)))
    return FALSE;

  child = ctk_flow_box_get_child_at_pos (box, event->x, event->y);
  ctk_flow_box_update_active (box, child);

  return FALSE;
}

static gboolean
ctk_flow_box_leave_notify_event (CtkWidget        *widget,
                                 GdkEventCrossing *event)
{
  CtkFlowBox *box = CTK_FLOW_BOX (widget);
  CtkFlowBoxChild *child = NULL;

  if (event->window != ctk_widget_get_window (CTK_WIDGET (box)))
    return FALSE;

  if (event->detail != GDK_NOTIFY_INFERIOR)
    child = NULL;
  else
    child = ctk_flow_box_get_child_at_pos (box, event->x, event->y);

  ctk_flow_box_update_active (box, child);

  return FALSE;
}

static void
ctk_flow_box_drag_gesture_update (CtkGestureDrag *gesture,
                                  gdouble         offset_x,
                                  gdouble         offset_y,
                                  CtkFlowBox     *box)
{
  CtkFlowBoxPrivate *priv = BOX_PRIV (box);
  gdouble start_x, start_y;
  CtkFlowBoxChild *child;
  CtkCssNode *widget_node;

  ctk_gesture_drag_get_start_point (gesture, &start_x, &start_y);

  if (!priv->rubberband_select &&
      (offset_x * offset_x) + (offset_y * offset_y) > RUBBERBAND_START_DISTANCE * RUBBERBAND_START_DISTANCE)
    {
      priv->rubberband_select = TRUE;
      priv->rubberband_first = ctk_flow_box_get_child_at_pos (box, start_x, start_y);
  
      widget_node = ctk_widget_get_css_node (CTK_WIDGET (box));
      priv->rubberband_node = ctk_css_node_new ();
      ctk_css_node_set_name (priv->rubberband_node, I_("rubberband"));
      ctk_css_node_set_parent (priv->rubberband_node, widget_node);
      ctk_css_node_set_state (priv->rubberband_node, ctk_css_node_get_state (widget_node));
      g_object_unref (priv->rubberband_node);

      /* Grab focus here, so Escape-to-stop-rubberband  works */
      if (priv->rubberband_first)
        ctk_flow_box_update_cursor (box, priv->rubberband_first);
      ctk_gesture_set_state (CTK_GESTURE (gesture), CTK_EVENT_SEQUENCE_CLAIMED);
    }

  if (priv->rubberband_select)
    {
      child = ctk_flow_box_get_child_at_pos (box, start_x + offset_x,
                                              start_y + offset_y);

      if (priv->rubberband_first == NULL)
        {
          priv->rubberband_first = child;
          if (priv->rubberband_first)
            ctk_flow_box_update_cursor (box, priv->rubberband_first);
        }
      if (child != NULL)
        priv->rubberband_last = child;

      update_autoscroll_mode (box, start_x + offset_x, start_y + offset_y);
      ctk_widget_queue_draw (CTK_WIDGET (box));
    }
}

static gboolean
ctk_flow_box_motion_notify_event (CtkWidget      *widget,
                                  GdkEventMotion *event)
{
  CtkFlowBox *box = CTK_FLOW_BOX (widget);
  CtkFlowBoxChild *child;
  GdkWindow *window;
  GdkWindow *event_window;
  gint relative_x;
  gint relative_y;
  gdouble parent_x;
  gdouble parent_y;

  window = ctk_widget_get_window (CTK_WIDGET (box));
  event_window = event->window;
  relative_x = event->x;
  relative_y = event->y;

  while ((event_window != NULL) && (event_window != window))
    {
      gdk_window_coords_to_parent (event_window,
                                   relative_x, relative_y,
                                   &parent_x, &parent_y);
      relative_x = parent_x;
      relative_y = parent_y;
      event_window = gdk_window_get_effective_parent (event_window);
    }

  child = ctk_flow_box_get_child_at_pos (box, relative_x, relative_y);
  ctk_flow_box_update_active (box, child);

  return CTK_WIDGET_CLASS (ctk_flow_box_parent_class)->motion_notify_event (widget, event);
}

static void
ctk_flow_box_multipress_gesture_pressed (CtkGestureMultiPress *gesture,
                                         guint                 n_press,
                                         gdouble               x,
                                         gdouble               y,
                                         CtkFlowBox           *box)
{
  CtkFlowBoxPrivate *priv = BOX_PRIV (box);
  CtkFlowBoxChild *child;

  child = ctk_flow_box_get_child_at_pos (box, x, y);

  if (child == NULL)
    return;

  /* The drag gesture is only triggered by first press */
  if (n_press != 1)
    ctk_gesture_set_state (priv->drag_gesture, CTK_EVENT_SEQUENCE_DENIED);

  priv->active_child = child;
  priv->active_child_active = TRUE;
  ctk_widget_queue_draw (CTK_WIDGET (box));

  if (n_press == 2 && !priv->activate_on_single_click)
    {
      ctk_gesture_set_state (CTK_GESTURE (gesture),
                             CTK_EVENT_SEQUENCE_CLAIMED);
      g_signal_emit (box, signals[CHILD_ACTIVATED], 0, child);
    }
}

static void
ctk_flow_box_multipress_gesture_released (CtkGestureMultiPress *gesture,
                                          guint                 n_press,
                                          gdouble               x,
                                          gdouble               y,
                                          CtkFlowBox           *box)
{
  CtkFlowBoxPrivate *priv = BOX_PRIV (box);

  if (priv->active_child != NULL && priv->active_child_active)
    {
      ctk_gesture_set_state (CTK_GESTURE (gesture),
                             CTK_EVENT_SEQUENCE_CLAIMED);

      if (priv->activate_on_single_click)
        ctk_flow_box_select_and_activate (box, priv->active_child);
      else
        {
          GdkEventSequence *sequence;
          GdkInputSource source;
          const GdkEvent *event;
          gboolean modify;
          gboolean extend;

          get_current_selection_modifiers (CTK_WIDGET (box), &modify, &extend);

          /* With touch, we default to modifying the selection.
           * You can still clear the selection and start over
           * by holding Ctrl.
           */

          sequence = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));
          event = ctk_gesture_get_last_event (CTK_GESTURE (gesture), sequence);
          source = gdk_device_get_source (gdk_event_get_source_device (event));

          if (source == GDK_SOURCE_TOUCHSCREEN)
            modify = !modify;

          ctk_flow_box_update_selection (box, priv->active_child, modify, extend);
        }
    }
}

static void
ctk_flow_box_multipress_gesture_stopped (CtkGestureMultiPress *gesture,
                                         CtkFlowBox           *box)
{
  CtkFlowBoxPrivate *priv = BOX_PRIV (box);

  priv->active_child = NULL;
  priv->active_child_active = FALSE;
  ctk_widget_queue_draw (CTK_WIDGET (box));
}

static void
ctk_flow_box_drag_gesture_begin (CtkGestureDrag *gesture,
                                 gdouble         start_x,
                                 gdouble         start_y,
                                 CtkWidget      *widget)
{
  CtkFlowBoxPrivate *priv = BOX_PRIV (widget);

  if (priv->selection_mode != CTK_SELECTION_MULTIPLE)
    {
      ctk_gesture_set_state (CTK_GESTURE (gesture), CTK_EVENT_SEQUENCE_DENIED);
      return;
    }

  priv->rubberband_select = FALSE;
  priv->rubberband_first = NULL;
  priv->rubberband_last = NULL;
  get_current_selection_modifiers (widget, &priv->rubberband_modify, &priv->rubberband_extend);
}

static void
ctk_flow_box_stop_rubberband (CtkFlowBox *box)
{
  CtkFlowBoxPrivate *priv = BOX_PRIV (box);

  priv->rubberband_select = FALSE;
  priv->rubberband_first = NULL;
  priv->rubberband_last = NULL;

  ctk_css_node_set_parent (priv->rubberband_node, NULL);
  priv->rubberband_node = NULL;

  remove_autoscroll (box);

  ctk_widget_queue_draw (CTK_WIDGET (box));
}

static void
ctk_flow_box_drag_gesture_end (CtkGestureDrag *gesture,
                               gdouble         offset_x,
                               gdouble         offset_y,
                               CtkFlowBox     *box)
{
  CtkFlowBoxPrivate *priv = BOX_PRIV (box);
  GdkEventSequence *sequence;

  if (!priv->rubberband_select)
    return;

  sequence = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture));

  if (ctk_gesture_handles_sequence (CTK_GESTURE (gesture), sequence))
    {
      if (!priv->rubberband_extend && !priv->rubberband_modify)
        ctk_flow_box_unselect_all_internal (box);

      if (priv->rubberband_first && priv->rubberband_last)
        ctk_flow_box_select_all_between (box, priv->rubberband_first, priv->rubberband_last, priv->rubberband_modify);

      ctk_flow_box_stop_rubberband (box);

      g_signal_emit (box, signals[SELECTED_CHILDREN_CHANGED], 0);
    }
  else
    ctk_flow_box_stop_rubberband (box);

  ctk_widget_queue_draw (CTK_WIDGET (box));
}

static gboolean
ctk_flow_box_key_press_event (CtkWidget   *widget,
                              GdkEventKey *event)
{
  CtkFlowBox *box = CTK_FLOW_BOX (widget);
  CtkFlowBoxPrivate *priv = BOX_PRIV (box);

  if (priv->rubberband_select)
    {
      if (event->keyval == GDK_KEY_Escape)
        {
          ctk_flow_box_stop_rubberband (box);
          return TRUE;
        }
    }

  return CTK_WIDGET_CLASS (ctk_flow_box_parent_class)->key_press_event (widget, event);
}

/* Realize and map {{{3 */

static void
ctk_flow_box_realize (CtkWidget *widget)
{
  CtkFlowBox *box = CTK_FLOW_BOX (widget);
  CtkAllocation allocation;
  GdkWindowAttr attributes = {0};
  GdkWindow *window;

  ctk_widget_get_allocation (CTK_WIDGET (box), &allocation);
  ctk_widget_set_realized (CTK_WIDGET (box), TRUE);

  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.event_mask = ctk_widget_get_events (CTK_WIDGET (box))
                                | GDK_ENTER_NOTIFY_MASK
                                | GDK_LEAVE_NOTIFY_MASK
                                | GDK_POINTER_MOTION_MASK
                                | GDK_KEY_PRESS_MASK
                                | GDK_BUTTON_PRESS_MASK
                                | GDK_BUTTON_RELEASE_MASK;
  attributes.wclass = GDK_INPUT_OUTPUT;

  window = gdk_window_new (ctk_widget_get_parent_window (CTK_WIDGET (box)),
                           &attributes, GDK_WA_X | GDK_WA_Y);
  ctk_widget_register_window (CTK_WIDGET (box), window);
  ctk_widget_set_window (CTK_WIDGET (box), window);
}

static void
ctk_flow_box_unmap (CtkWidget *widget)
{
  CtkFlowBox *box = CTK_FLOW_BOX (widget);

  remove_autoscroll (box);

  CTK_WIDGET_CLASS (ctk_flow_box_parent_class)->unmap (widget);
}

/* CtkContainer implementation {{{2 */

static void
ctk_flow_box_add (CtkContainer *container,
                  CtkWidget    *child)
{
  ctk_flow_box_insert (CTK_FLOW_BOX (container), child, -1);
}

static void
ctk_flow_box_remove (CtkContainer *container,
                     CtkWidget    *widget)
{
  CtkFlowBox *box = CTK_FLOW_BOX (container);
  CtkFlowBoxPrivate *priv = BOX_PRIV (box);
  gboolean was_visible;
  gboolean was_selected;
  CtkFlowBoxChild *child;

  if (CTK_IS_FLOW_BOX_CHILD (widget))
    child = CTK_FLOW_BOX_CHILD (widget);
  else
    {
      child = (CtkFlowBoxChild*)ctk_widget_get_parent (widget);
      if (!CTK_IS_FLOW_BOX_CHILD (child))
        {
          g_warning ("Tried to remove non-child %p", widget);
          return;
        }
    }

  was_visible = child_is_visible (CTK_WIDGET (child));
  was_selected = CHILD_PRIV (child)->selected;

  if (child == priv->active_child)
    priv->active_child = NULL;
  if (child == priv->selected_child)
    priv->selected_child = NULL;

  g_sequence_remove (CHILD_PRIV (child)->iter);
  ctk_widget_unparent (CTK_WIDGET (child));

  if (was_visible && ctk_widget_get_visible (CTK_WIDGET (box)))
    ctk_widget_queue_resize (CTK_WIDGET (box));

  if (was_selected && !ctk_widget_in_destruction (CTK_WIDGET (box)))
    g_signal_emit (box, signals[SELECTED_CHILDREN_CHANGED], 0);
}

static void
ctk_flow_box_forall (CtkContainer *container,
                     gboolean      include_internals,
                     CtkCallback   callback,
                     gpointer      callback_target)
{
  GSequenceIter *iter;
  CtkWidget *child;

  iter = g_sequence_get_begin_iter (BOX_PRIV (container)->children);
  while (!g_sequence_iter_is_end (iter))
    {
      child = g_sequence_get (iter);
      iter = g_sequence_iter_next (iter);
      callback (child, callback_target);
    }
}

static GType
ctk_flow_box_child_type (CtkContainer *container)
{
  return CTK_TYPE_FLOW_BOX_CHILD;
}

/* Keynav {{{2 */

static gboolean
ctk_flow_box_focus (CtkWidget        *widget,
                    CtkDirectionType  direction)
{
  CtkFlowBox *box = CTK_FLOW_BOX (widget);
  CtkWidget *focus_child;
  GSequenceIter *iter;
  CtkFlowBoxChild *next_focus_child;

  /* Without "can-focus" flag fall back to the default behavior immediately */
  if (!ctk_widget_get_can_focus (widget))
    {
      return CTK_WIDGET_CLASS (ctk_flow_box_parent_class)->focus (widget, direction);
    }

  focus_child = ctk_container_get_focus_child (CTK_CONTAINER (box));
  next_focus_child = NULL;

  if (focus_child != NULL)
    {
      if (ctk_widget_child_focus (focus_child, direction))
        return TRUE;

      iter = CHILD_PRIV (focus_child)->iter;

      if (direction == CTK_DIR_LEFT || direction == CTK_DIR_TAB_BACKWARD)
        iter = ctk_flow_box_get_previous_focusable (box, iter);
      else if (direction == CTK_DIR_RIGHT || direction == CTK_DIR_TAB_FORWARD)
        iter = ctk_flow_box_get_next_focusable (box, iter);
      else if (direction == CTK_DIR_UP)
        iter = ctk_flow_box_get_above_focusable (box, iter);
      else if (direction == CTK_DIR_DOWN)
        iter = ctk_flow_box_get_below_focusable (box, iter);

      if (iter != NULL)
        next_focus_child = g_sequence_get (iter);
    }
  else
    {
      if (BOX_PRIV (box)->selected_child)
        next_focus_child = BOX_PRIV (box)->selected_child;
      else
        {
          if (direction == CTK_DIR_UP || direction == CTK_DIR_TAB_BACKWARD)
            iter = ctk_flow_box_get_last_focusable (box);
          else
            iter = ctk_flow_box_get_first_focusable (box);

          if (iter != NULL)
            next_focus_child = g_sequence_get (iter);
        }
    }

  if (next_focus_child == NULL)
    {
      if (direction == CTK_DIR_UP || direction == CTK_DIR_DOWN ||
          direction == CTK_DIR_LEFT || direction == CTK_DIR_RIGHT)
        {
          if (ctk_widget_keynav_failed (CTK_WIDGET (box), direction))
            return TRUE;
        }

      return FALSE;
    }

  if (ctk_widget_child_focus (CTK_WIDGET (next_focus_child), direction))
    return TRUE;

  return TRUE;
}

static void
ctk_flow_box_add_move_binding (CtkBindingSet   *binding_set,
                               guint            keyval,
                               GdkModifierType  modmask,
                               CtkMovementStep  step,
                               gint             count)
{
  GdkDisplay *display;
  GdkModifierType extend_mod_mask = GDK_SHIFT_MASK;
  GdkModifierType modify_mod_mask = GDK_CONTROL_MASK;

  display = gdk_display_get_default ();
  if (display)
    {
      extend_mod_mask = gdk_keymap_get_modifier_mask (gdk_keymap_get_for_display (display),
                                                      GDK_MODIFIER_INTENT_EXTEND_SELECTION);
      modify_mod_mask = gdk_keymap_get_modifier_mask (gdk_keymap_get_for_display (display),
                                                      GDK_MODIFIER_INTENT_MODIFY_SELECTION);
    }

  ctk_binding_entry_add_signal (binding_set, keyval, modmask,
                                "move-cursor", 2,
                                CTK_TYPE_MOVEMENT_STEP, step,
                                G_TYPE_INT, count,
                                NULL);
  ctk_binding_entry_add_signal (binding_set, keyval, modmask | extend_mod_mask,
                                "move-cursor", 2,
                                CTK_TYPE_MOVEMENT_STEP, step,
                                G_TYPE_INT, count,
                                NULL);
  ctk_binding_entry_add_signal (binding_set, keyval, modmask | modify_mod_mask,
                                "move-cursor", 2,
                                CTK_TYPE_MOVEMENT_STEP, step,
                                G_TYPE_INT, count,
                                NULL);
  ctk_binding_entry_add_signal (binding_set, keyval, modmask | extend_mod_mask | modify_mod_mask,
                                "move-cursor", 2,
                                CTK_TYPE_MOVEMENT_STEP, step,
                                G_TYPE_INT, count,
                                NULL);
}

static void
ctk_flow_box_activate_cursor_child (CtkFlowBox *box)
{
  ctk_flow_box_select_and_activate (box, BOX_PRIV (box)->cursor_child);
}

static void
ctk_flow_box_toggle_cursor_child (CtkFlowBox *box)
{
  CtkFlowBoxPrivate *priv = BOX_PRIV (box);

  if (priv->cursor_child == NULL)
    return;

  if ((priv->selection_mode == CTK_SELECTION_SINGLE ||
       priv->selection_mode == CTK_SELECTION_MULTIPLE) &&
      CHILD_PRIV (priv->cursor_child)->selected)
    ctk_flow_box_unselect_child_internal (box, priv->cursor_child);
  else
    ctk_flow_box_select_and_activate (box, priv->cursor_child);
}

static gboolean
ctk_flow_box_move_cursor (CtkFlowBox      *box,
                          CtkMovementStep  step,
                          gint             count)
{
  CtkFlowBoxPrivate *priv = BOX_PRIV (box);
  gboolean modify;
  gboolean extend;
  CtkFlowBoxChild *child;
  CtkFlowBoxChild *prev;
  CtkFlowBoxChild *next;
  CtkAllocation allocation;
  gint page_size;
  GSequenceIter *iter;
  gint start;
  CtkAdjustment *adjustment;
  gboolean vertical;

  /* Without "can-focus" flag fall back to the default behavior immediately */
  if (!ctk_widget_get_can_focus (CTK_WIDGET (box)))
    return FALSE;

  vertical = priv->orientation == CTK_ORIENTATION_VERTICAL;

  if (vertical)
    {
       switch (step)
         {
         case CTK_MOVEMENT_VISUAL_POSITIONS:
           step = CTK_MOVEMENT_DISPLAY_LINES;
           break;
         case CTK_MOVEMENT_DISPLAY_LINES:
           step = CTK_MOVEMENT_VISUAL_POSITIONS;
           break;
         default: ;
         }
    }

  child = NULL;
  switch (step)
    {
    case CTK_MOVEMENT_VISUAL_POSITIONS:
      if (priv->cursor_child != NULL)
        {
          iter = CHILD_PRIV (priv->cursor_child)->iter;
          if (ctk_widget_get_direction (CTK_WIDGET (box)) == CTK_TEXT_DIR_RTL)
            count = - count;

          while (count < 0 && iter != NULL)
            {
              iter = ctk_flow_box_get_previous_focusable (box, iter);
              count = count + 1;
            }
          while (count > 0 && iter != NULL)
            {
              iter = ctk_flow_box_get_next_focusable (box, iter);
              count = count - 1;
            }

          if (iter != NULL && !g_sequence_iter_is_end (iter))
            child = g_sequence_get (iter);
        }
      break;

    case CTK_MOVEMENT_BUFFER_ENDS:
      if (count < 0)
        iter = ctk_flow_box_get_first_focusable (box);
      else
        iter = ctk_flow_box_get_last_focusable (box);
      if (iter != NULL)
        child = g_sequence_get (iter);
      break;

    case CTK_MOVEMENT_DISPLAY_LINES:
      if (priv->cursor_child != NULL)
        {
          iter = CHILD_PRIV (priv->cursor_child)->iter;

          while (count < 0 && iter != NULL)
            {
              iter = ctk_flow_box_get_above_focusable (box, iter);
              count = count + 1;
            }
          while (count > 0 && iter != NULL)
            {
              iter = ctk_flow_box_get_below_focusable (box, iter);
              count = count - 1;
            }

          if (iter != NULL)
            child = g_sequence_get (iter);
        }
      break;

    case CTK_MOVEMENT_PAGES:
      page_size = 100;
      adjustment = vertical ? priv->hadjustment : priv->vadjustment;
      if (adjustment)
        page_size = ctk_adjustment_get_page_increment (adjustment);

      if (priv->cursor_child != NULL)
        {
          child = priv->cursor_child;
          iter = CHILD_PRIV (child)->iter;
          ctk_widget_get_allocation (CTK_WIDGET (child), &allocation);
          start = vertical ? allocation.x : allocation.y;

          if (count < 0)
            {
              gint i = 0;

              /* Up */
              while (iter != NULL)
                {
                  iter = ctk_flow_box_get_previous_focusable (box, iter);
                  if (iter == NULL)
                    break;

                  prev = g_sequence_get (iter);

                  /* go up an even number of rows */
                  if (i % priv->cur_children_per_line == 0)
                    {
                      ctk_widget_get_allocation (CTK_WIDGET (prev), &allocation);
                      if ((vertical ? allocation.x : allocation.y) < start - page_size)
                        break;
                    }

                  child = prev;
                  i++;
                }
            }
          else
            {
              gint i = 0;

              /* Down */
              while (!g_sequence_iter_is_end (iter))
                {
                  iter = ctk_flow_box_get_next_focusable (box, iter);
                  if (iter == NULL || g_sequence_iter_is_end (iter))
                    break;

                  next = g_sequence_get (iter);

                  if (i % priv->cur_children_per_line == 0)
                    {
                      ctk_widget_get_allocation (CTK_WIDGET (next), &allocation);
                      if ((vertical ? allocation.x : allocation.y) > start + page_size)
                        break;
                    }

                  child = next;
                  i++;
                }
            }
          ctk_widget_get_allocation (CTK_WIDGET (child), &allocation);
        }
      break;

    default:
      g_assert_not_reached ();
    }

  if (child == NULL || child == priv->cursor_child)
    {
      CtkDirectionType direction = count < 0 ? CTK_DIR_UP : CTK_DIR_DOWN;

      if (!ctk_widget_keynav_failed (CTK_WIDGET (box), direction))
        {
          return FALSE;
        }

      return TRUE;
    }

  /* If the child has its "can-focus" property set to FALSE then it will
   * not grab the focus. We must pass the focus to its child directly.
   */
  if (!ctk_widget_get_can_focus (CTK_WIDGET (child)))
    {
      CtkWidget *subchild;

      subchild = ctk_bin_get_child (CTK_BIN (child));
      if (subchild)
        {
          CtkDirectionType direction = count < 0 ? CTK_DIR_TAB_BACKWARD : CTK_DIR_TAB_FORWARD;
          ctk_widget_child_focus (subchild, direction);
        }
    }

  get_current_selection_modifiers (CTK_WIDGET (box), &modify, &extend);

  ctk_flow_box_update_cursor (box, child);
  if (!modify)
    ctk_flow_box_update_selection (box, child, FALSE, extend);
  return TRUE;
}

/* Selection {{{2 */

static void
ctk_flow_box_selected_children_changed (CtkFlowBox *box)
{
  _ctk_flow_box_accessible_selection_changed (CTK_WIDGET (box));
}

/* GObject implementation {{{2 */

static void
ctk_flow_box_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  CtkFlowBox *box = CTK_FLOW_BOX (object);
  CtkFlowBoxPrivate *priv = BOX_PRIV (box);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, priv->orientation);
      break;
    case PROP_HOMOGENEOUS:
      g_value_set_boolean (value, priv->homogeneous);
      break;
    case PROP_COLUMN_SPACING:
      g_value_set_uint (value, priv->column_spacing);
      break;
    case PROP_ROW_SPACING:
      g_value_set_uint (value, priv->row_spacing);
      break;
    case PROP_MIN_CHILDREN_PER_LINE:
      g_value_set_uint (value, priv->min_children_per_line);
      break;
    case PROP_MAX_CHILDREN_PER_LINE:
      g_value_set_uint (value, priv->max_children_per_line);
      break;
    case PROP_SELECTION_MODE:
      g_value_set_enum (value, priv->selection_mode);
      break;
    case PROP_ACTIVATE_ON_SINGLE_CLICK:
      g_value_set_boolean (value, priv->activate_on_single_click);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_flow_box_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  CtkFlowBox *box = CTK_FLOW_BOX (object);
  CtkFlowBoxPrivate *priv = BOX_PRIV (box);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      if (priv->orientation != g_value_get_enum (value))
        {
          priv->orientation = g_value_get_enum (value);
          _ctk_orientable_set_style_classes (CTK_ORIENTABLE (box));
          /* Re-box the children in the new orientation */
          ctk_widget_queue_resize (CTK_WIDGET (box));
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_HOMOGENEOUS:
      ctk_flow_box_set_homogeneous (box, g_value_get_boolean (value));
      break;
    case PROP_COLUMN_SPACING:
      ctk_flow_box_set_column_spacing (box, g_value_get_uint (value));
      break;
    case PROP_ROW_SPACING:
      ctk_flow_box_set_row_spacing (box, g_value_get_uint (value));
      break;
    case PROP_MIN_CHILDREN_PER_LINE:
      ctk_flow_box_set_min_children_per_line (box, g_value_get_uint (value));
      break;
    case PROP_MAX_CHILDREN_PER_LINE:
      ctk_flow_box_set_max_children_per_line (box, g_value_get_uint (value));
      break;
    case PROP_SELECTION_MODE:
      ctk_flow_box_set_selection_mode (box, g_value_get_enum (value));
      break;
    case PROP_ACTIVATE_ON_SINGLE_CLICK:
      ctk_flow_box_set_activate_on_single_click (box, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_flow_box_finalize (GObject *obj)
{
  CtkFlowBoxPrivate *priv = BOX_PRIV (obj);

  if (priv->filter_destroy != NULL)
    priv->filter_destroy (priv->filter_data);
  if (priv->sort_destroy != NULL)
    priv->sort_destroy (priv->sort_data);

  g_sequence_free (priv->children);
  g_clear_object (&priv->hadjustment);
  g_clear_object (&priv->vadjustment);

  g_object_unref (priv->drag_gesture);
  g_object_unref (priv->multipress_gesture);

  if (priv->bound_model)
    {
      if (priv->create_widget_func_data_destroy)
        priv->create_widget_func_data_destroy (priv->create_widget_func_data);

      g_signal_handlers_disconnect_by_func (priv->bound_model, ctk_flow_box_bound_model_changed, obj);
      g_clear_object (&priv->bound_model);
    }

  g_clear_object (&priv->gadget);

  G_OBJECT_CLASS (ctk_flow_box_parent_class)->finalize (obj);
}

static void
ctk_flow_box_class_init (CtkFlowBoxClass *class)
{
  GObjectClass      *object_class = G_OBJECT_CLASS (class);
  CtkWidgetClass    *widget_class = CTK_WIDGET_CLASS (class);
  CtkContainerClass *container_class = CTK_CONTAINER_CLASS (class);
  CtkBindingSet     *binding_set;

  object_class->finalize = ctk_flow_box_finalize;
  object_class->get_property = ctk_flow_box_get_property;
  object_class->set_property = ctk_flow_box_set_property;

  widget_class->enter_notify_event = ctk_flow_box_enter_notify_event;
  widget_class->leave_notify_event = ctk_flow_box_leave_notify_event;
  widget_class->motion_notify_event = ctk_flow_box_motion_notify_event;
  widget_class->size_allocate = ctk_flow_box_size_allocate;
  widget_class->realize = ctk_flow_box_realize;
  widget_class->unmap = ctk_flow_box_unmap;
  widget_class->focus = ctk_flow_box_focus;
  widget_class->draw = ctk_flow_box_draw;
  widget_class->key_press_event = ctk_flow_box_key_press_event;
  widget_class->get_request_mode = ctk_flow_box_get_request_mode;
  widget_class->get_preferred_width = ctk_flow_box_get_preferred_width;
  widget_class->get_preferred_height = ctk_flow_box_get_preferred_height;
  widget_class->get_preferred_height_for_width = ctk_flow_box_get_preferred_height_for_width;
  widget_class->get_preferred_width_for_height = ctk_flow_box_get_preferred_width_for_height;

  container_class->add = ctk_flow_box_add;
  container_class->remove = ctk_flow_box_remove;
  container_class->forall = ctk_flow_box_forall;
  container_class->child_type = ctk_flow_box_child_type;
  ctk_container_class_handle_border_width (container_class);

  class->activate_cursor_child = ctk_flow_box_activate_cursor_child;
  class->toggle_cursor_child = ctk_flow_box_toggle_cursor_child;
  class->move_cursor = ctk_flow_box_move_cursor;
  class->select_all = ctk_flow_box_select_all;
  class->unselect_all = ctk_flow_box_unselect_all;
  class->selected_children_changed = ctk_flow_box_selected_children_changed;

  g_object_class_override_property (object_class, PROP_ORIENTATION, "orientation");

  /**
   * CtkFlowBox:selection-mode:
   *
   * The selection mode used by the flow  box.
   */
  props[PROP_SELECTION_MODE] = 
    g_param_spec_enum ("selection-mode",
                       P_("Selection mode"),
                       P_("The selection mode"),
                       CTK_TYPE_SELECTION_MODE,
                       CTK_SELECTION_SINGLE,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkFlowBox:activate-on-single-click:
   *
   * Determines whether children can be activated with a single
   * click, or require a double-click.
   */
  props[PROP_ACTIVATE_ON_SINGLE_CLICK] =
    g_param_spec_boolean ("activate-on-single-click",
                          P_("Activate on Single Click"),
                          P_("Activate row on a single click"),
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkFlowBox:homogeneous:
   *
   * Determines whether all children should be allocated the
   * same size.
   */
  props[PROP_HOMOGENEOUS] = 
    g_param_spec_boolean ("homogeneous",
                          P_("Homogeneous"),
                          P_("Whether the children should all be the same size"),
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkFlowBox:min-children-per-line:
   *
   * The minimum number of children to allocate consecutively
   * in the given orientation.
   *
   * Setting the minimum children per line ensures
   * that a reasonably small height will be requested
   * for the overall minimum width of the box.
   */
  props[PROP_MIN_CHILDREN_PER_LINE] =
    g_param_spec_uint ("min-children-per-line",
                       P_("Minimum Children Per Line"),
                       P_("The minimum number of children to allocate "
                       "consecutively in the given orientation."),
                       0, G_MAXUINT, 0,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkFlowBox:max-children-per-line:
   *
   * The maximum amount of children to request space for consecutively
   * in the given orientation.
   */
  props[PROP_MAX_CHILDREN_PER_LINE] =
    g_param_spec_uint ("max-children-per-line",
                       P_("Maximum Children Per Line"),
                       P_("The maximum amount of children to request space for "
                          "consecutively in the given orientation."),
                       1, G_MAXUINT, DEFAULT_MAX_CHILDREN_PER_LINE,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkFlowBox:row-spacing:
   *
   * The amount of vertical space between two children.
   */
  props[PROP_ROW_SPACING] =
    g_param_spec_uint ("row-spacing",
                       P_("Vertical spacing"),
                       P_("The amount of vertical space between two children"),
                       0, G_MAXUINT, 0,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkFlowBox:column-spacing:
   *
   * The amount of horizontal space between two children.
   */
  props[PROP_COLUMN_SPACING] =
    g_param_spec_uint ("column-spacing",
                       P_("Horizontal spacing"),
                       P_("The amount of horizontal space between two children"),
                       0, G_MAXUINT, 0,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  /**
   * CtkFlowBox::child-activated:
   * @box: the #CtkFlowBox on which the signal is emitted
   * @child: the child that is activated
   *
   * The ::child-activated signal is emitted when a child has been
   * activated by the user.
   */
  signals[CHILD_ACTIVATED] = g_signal_new (I_("child-activated"),
                                           CTK_TYPE_FLOW_BOX,
                                           G_SIGNAL_RUN_LAST,
                                           G_STRUCT_OFFSET (CtkFlowBoxClass, child_activated),
                                           NULL, NULL,
                                           NULL,
                                           G_TYPE_NONE, 1,
                                           CTK_TYPE_FLOW_BOX_CHILD);

  /**
   * CtkFlowBox::selected-children-changed:
   * @box: the #CtkFlowBox on wich the signal is emitted
   *
   * The ::selected-children-changed signal is emitted when the
   * set of selected children changes.
   *
   * Use ctk_flow_box_selected_foreach() or
   * ctk_flow_box_get_selected_children() to obtain the
   * selected children.
   */
  signals[SELECTED_CHILDREN_CHANGED] = g_signal_new (I_("selected-children-changed"),
                                                     CTK_TYPE_FLOW_BOX,
                                                     G_SIGNAL_RUN_FIRST,
                                                     G_STRUCT_OFFSET (CtkFlowBoxClass, selected_children_changed),
                                                     NULL, NULL,
                                                     NULL,
                                                     G_TYPE_NONE, 0);

  /**
   * CtkFlowBox::activate-cursor-child:
   * @box: the #CtkFlowBox on which the signal is emitted
   *
   * The ::activate-cursor-child signal is a
   * [keybinding signal][CtkBindingSignal]
   * which gets emitted when the user activates the @box.
   */
  signals[ACTIVATE_CURSOR_CHILD] = g_signal_new (I_("activate-cursor-child"),
                                                 CTK_TYPE_FLOW_BOX,
                                                 G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                                 G_STRUCT_OFFSET (CtkFlowBoxClass, activate_cursor_child),
                                                 NULL, NULL,
                                                 NULL,
                                                 G_TYPE_NONE, 0);

  /**
   * CtkFlowBox::toggle-cursor-child:
   * @box: the #CtkFlowBox on which the signal is emitted
   *
   * The ::toggle-cursor-child signal is a
   * [keybinding signal][CtkBindingSignal]
   * which toggles the selection of the child that has the focus.
   *
   * The default binding for this signal is Ctrl-Space.
   */
  signals[TOGGLE_CURSOR_CHILD] = g_signal_new (I_("toggle-cursor-child"),
                                               CTK_TYPE_FLOW_BOX,
                                               G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                               G_STRUCT_OFFSET (CtkFlowBoxClass, toggle_cursor_child),
                                               NULL, NULL,
                                               NULL,
                                               G_TYPE_NONE, 0);

  /**
   * CtkFlowBox::move-cursor:
   * @box: the #CtkFlowBox on which the signal is emitted
   * @step: the granularity fo the move, as a #CtkMovementStep
   * @count: the number of @step units to move
   *
   * The ::move-cursor signal is a
   * [keybinding signal][CtkBindingSignal]
   * which gets emitted when the user initiates a cursor movement.
   *
   * Applications should not connect to it, but may emit it with
   * g_signal_emit_by_name() if they need to control the cursor
   * programmatically.
   *
   * The default bindings for this signal come in two variants,
   * the variant with the Shift modifier extends the selection,
   * the variant without the Shift modifer does not.
   * There are too many key combinations to list them all here.
   * - Arrow keys move by individual children
   * - Home/End keys move to the ends of the box
   * - PageUp/PageDown keys move vertically by pages
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event.
   * %FALSE to propagate the event further.
   */
  signals[MOVE_CURSOR] = g_signal_new (I_("move-cursor"),
                                       CTK_TYPE_FLOW_BOX,
                                       G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                       G_STRUCT_OFFSET (CtkFlowBoxClass, move_cursor),
                                       NULL, NULL,
                                       _ctk_marshal_BOOLEAN__ENUM_INT,
                                       G_TYPE_BOOLEAN, 2,
                                       CTK_TYPE_MOVEMENT_STEP, G_TYPE_INT);
  g_signal_set_va_marshaller (signals[MOVE_CURSOR],
                              G_TYPE_FROM_CLASS (class),
                              _ctk_marshal_BOOLEAN__ENUM_INTv);
  /**
   * CtkFlowBox::select-all:
   * @box: the #CtkFlowBox on which the signal is emitted
   *
   * The ::select-all signal is a
   * [keybinding signal][CtkBindingSignal]
   * which gets emitted to select all children of the box, if
   * the selection mode permits it.
   *
   * The default bindings for this signal is Ctrl-a.
   */
  signals[SELECT_ALL] = g_signal_new (I_("select-all"),
                                      CTK_TYPE_FLOW_BOX,
                                      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                      G_STRUCT_OFFSET (CtkFlowBoxClass, select_all),
                                      NULL, NULL,
                                      NULL,
                                      G_TYPE_NONE, 0);

  /**
   * CtkFlowBox::unselect-all:
   * @box: the #CtkFlowBox on which the signal is emitted
   *
   * The ::unselect-all signal is a
   * [keybinding signal][CtkBindingSignal]
   * which gets emitted to unselect all children of the box, if
   * the selection mode permits it.
   *
   * The default bindings for this signal is Ctrl-Shift-a.
   */
  signals[UNSELECT_ALL] = g_signal_new (I_("unselect-all"),
                                        CTK_TYPE_FLOW_BOX,
                                        G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                        G_STRUCT_OFFSET (CtkFlowBoxClass, unselect_all),
                                        NULL, NULL,
                                        NULL,
                                        G_TYPE_NONE, 0);

  widget_class->activate_signal = signals[ACTIVATE_CURSOR_CHILD];

  binding_set = ctk_binding_set_by_class (class);
  ctk_flow_box_add_move_binding (binding_set, GDK_KEY_Home, 0,
                                 CTK_MOVEMENT_BUFFER_ENDS, -1);
  ctk_flow_box_add_move_binding (binding_set, GDK_KEY_KP_Home, 0,
                                 CTK_MOVEMENT_BUFFER_ENDS, -1);
  ctk_flow_box_add_move_binding (binding_set, GDK_KEY_End, 0,
                                 CTK_MOVEMENT_BUFFER_ENDS, 1);
  ctk_flow_box_add_move_binding (binding_set, GDK_KEY_KP_End, 0,
                                 CTK_MOVEMENT_BUFFER_ENDS, 1);
  ctk_flow_box_add_move_binding (binding_set, GDK_KEY_Up, 0,
                                 CTK_MOVEMENT_DISPLAY_LINES, -1);
  ctk_flow_box_add_move_binding (binding_set, GDK_KEY_KP_Up, 0,
                                 CTK_MOVEMENT_DISPLAY_LINES, -1);
  ctk_flow_box_add_move_binding (binding_set, GDK_KEY_Down, 0,
                                 CTK_MOVEMENT_DISPLAY_LINES, 1);
  ctk_flow_box_add_move_binding (binding_set, GDK_KEY_KP_Down, 0,
                                 CTK_MOVEMENT_DISPLAY_LINES, 1);
  ctk_flow_box_add_move_binding (binding_set, GDK_KEY_Page_Up, 0,
                                 CTK_MOVEMENT_PAGES, -1);
  ctk_flow_box_add_move_binding (binding_set, GDK_KEY_KP_Page_Up, 0,
                                 CTK_MOVEMENT_PAGES, -1);
  ctk_flow_box_add_move_binding (binding_set, GDK_KEY_Page_Down, 0,
                                 CTK_MOVEMENT_PAGES, 1);
  ctk_flow_box_add_move_binding (binding_set, GDK_KEY_KP_Page_Down, 0,
                                 CTK_MOVEMENT_PAGES, 1);

  ctk_flow_box_add_move_binding (binding_set, GDK_KEY_Right, 0,
                                 CTK_MOVEMENT_VISUAL_POSITIONS, 1);
  ctk_flow_box_add_move_binding (binding_set, GDK_KEY_KP_Right, 0,
                                 CTK_MOVEMENT_VISUAL_POSITIONS, 1);
  ctk_flow_box_add_move_binding (binding_set, GDK_KEY_Left, 0,
                                 CTK_MOVEMENT_VISUAL_POSITIONS, -1);
  ctk_flow_box_add_move_binding (binding_set, GDK_KEY_KP_Left, 0,
                                 CTK_MOVEMENT_VISUAL_POSITIONS, -1);

  ctk_binding_entry_add_signal (binding_set, GDK_KEY_space, GDK_CONTROL_MASK,
                                "toggle-cursor-child", 0, NULL);
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Space, GDK_CONTROL_MASK,
                                "toggle-cursor-child", 0, NULL);

  ctk_binding_entry_add_signal (binding_set, GDK_KEY_a, GDK_CONTROL_MASK,
                                "select-all", 0);
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_a, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
                                "unselect-all", 0);

  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_FLOW_BOX_ACCESSIBLE);
  ctk_widget_class_set_css_name (widget_class, "flowbox");
}

static void
ctk_flow_box_init (CtkFlowBox *box)
{
  CtkFlowBoxPrivate *priv = BOX_PRIV (box);
  CtkCssNode *widget_node;

  ctk_widget_set_has_window (CTK_WIDGET (box), TRUE);

  priv->orientation = CTK_ORIENTATION_HORIZONTAL;
  priv->selection_mode = CTK_SELECTION_SINGLE;
  priv->max_children_per_line = DEFAULT_MAX_CHILDREN_PER_LINE;
  priv->column_spacing = 0;
  priv->row_spacing = 0;
  priv->activate_on_single_click = TRUE;

  _ctk_orientable_set_style_classes (CTK_ORIENTABLE (box));

  priv->children = g_sequence_new (NULL);

  priv->multipress_gesture = ctk_gesture_multi_press_new (CTK_WIDGET (box));
  ctk_gesture_single_set_touch_only (CTK_GESTURE_SINGLE (priv->multipress_gesture),
                                     FALSE);
  ctk_gesture_single_set_button (CTK_GESTURE_SINGLE (priv->multipress_gesture),
                                 GDK_BUTTON_PRIMARY);
  ctk_event_controller_set_propagation_phase (CTK_EVENT_CONTROLLER (priv->multipress_gesture),
                                              CTK_PHASE_BUBBLE);
  g_signal_connect (priv->multipress_gesture, "pressed",
                    G_CALLBACK (ctk_flow_box_multipress_gesture_pressed), box);
  g_signal_connect (priv->multipress_gesture, "released",
                    G_CALLBACK (ctk_flow_box_multipress_gesture_released), box);
  g_signal_connect (priv->multipress_gesture, "stopped",
                    G_CALLBACK (ctk_flow_box_multipress_gesture_stopped), box);

  priv->drag_gesture = ctk_gesture_drag_new (CTK_WIDGET (box));
  ctk_gesture_single_set_touch_only (CTK_GESTURE_SINGLE (priv->drag_gesture),
                                     FALSE);
  ctk_gesture_single_set_button (CTK_GESTURE_SINGLE (priv->drag_gesture),
                                 GDK_BUTTON_PRIMARY);
  ctk_event_controller_set_propagation_phase (CTK_EVENT_CONTROLLER (priv->drag_gesture),
                                              CTK_PHASE_CAPTURE);
  g_signal_connect (priv->drag_gesture, "drag-begin",
                    G_CALLBACK (ctk_flow_box_drag_gesture_begin), box);
  g_signal_connect (priv->drag_gesture, "drag-update",
                    G_CALLBACK (ctk_flow_box_drag_gesture_update), box);
  g_signal_connect (priv->drag_gesture, "drag-end",
                    G_CALLBACK (ctk_flow_box_drag_gesture_end), box);

  widget_node = ctk_widget_get_css_node (CTK_WIDGET (box));
  priv->gadget = ctk_css_custom_gadget_new_for_node (widget_node,
                                                     CTK_WIDGET (box),
                                                     ctk_flow_box_measure,
                                                     ctk_flow_box_allocate,
                                                     ctk_flow_box_render,
                                                     NULL,
                                                     NULL);
}

static void
ctk_flow_box_bound_model_changed (GListModel *list,
                                  guint       position,
                                  guint       removed,
                                  guint       added,
                                  gpointer    user_data)
{
  CtkFlowBox *box = user_data;
  CtkFlowBoxPrivate *priv = BOX_PRIV (box);
  gint i;

  while (removed--)
    {
      CtkFlowBoxChild *child;

      child = ctk_flow_box_get_child_at_index (box, position);
      ctk_widget_destroy (CTK_WIDGET (child));
    }

  for (i = 0; i < added; i++)
    {
      GObject *item;
      CtkWidget *widget;

      item = g_list_model_get_item (list, position + i);
      widget = priv->create_widget_func (item, priv->create_widget_func_data);

      /* We need to sink the floating reference here, so that we can accept
       * both instances created with a floating reference (e.g. C functions
       * that just return the result of g_object_new()) and without (e.g.
       * from language bindings which will automatically sink the floating
       * reference).
       *
       * See the similar code in ctklistbox.c:ctk_list_box_bound_model_changed.
       */
      if (g_object_is_floating (widget))
        g_object_ref_sink (widget);

      ctk_widget_show (widget);
      ctk_flow_box_insert (box, widget, position + i);

      g_object_unref (widget);
      g_object_unref (item);
    }
}

 /* Public API {{{2 */

/**
 * ctk_flow_box_new:
 *
 * Creates a CtkFlowBox.
 *
 * Returns: a new #CtkFlowBox container
 *
 * Since: 3.12
 */
CtkWidget *
ctk_flow_box_new (void)
{
  return (CtkWidget *)g_object_new (CTK_TYPE_FLOW_BOX, NULL);
}

static void
ctk_flow_box_insert_css_node (CtkFlowBox    *box,
                              CtkWidget     *child,
                              GSequenceIter *iter)
{
  GSequenceIter *prev_iter;
  CtkWidget *sibling;

  prev_iter = g_sequence_iter_prev (iter);

  if (prev_iter != iter)
    {
      sibling = g_sequence_get (prev_iter);
      ctk_css_node_insert_after (ctk_widget_get_css_node (CTK_WIDGET (box)),
                                 ctk_widget_get_css_node (child),
                                 ctk_widget_get_css_node (sibling));
    }
}

/**
 * ctk_flow_box_insert:
 * @box: a #CtkFlowBox
 * @widget: the #CtkWidget to add
 * @position: the position to insert @child in
 *
 * Inserts the @widget into @box at @position.
 *
 * If a sort function is set, the widget will actually be inserted
 * at the calculated position and this function has the same effect
 * as ctk_container_add().
 *
 * If @position is -1, or larger than the total number of children
 * in the @box, then the @widget will be appended to the end.
 *
 * Since: 3.12
 */
void
ctk_flow_box_insert (CtkFlowBox *box,
                     CtkWidget  *widget,
                     gint        position)
{
  CtkFlowBoxPrivate *priv;
  CtkFlowBoxChild *child;
  GSequenceIter *iter;

  g_return_if_fail (CTK_IS_FLOW_BOX (box));
  g_return_if_fail (CTK_IS_WIDGET (widget));

  priv = BOX_PRIV (box);

  if (CTK_IS_FLOW_BOX_CHILD (widget))
    child = CTK_FLOW_BOX_CHILD (widget);
  else
    {
      child = CTK_FLOW_BOX_CHILD (ctk_flow_box_child_new ());
      ctk_widget_show (CTK_WIDGET (child));
      ctk_container_add (CTK_CONTAINER (child), widget);
    }

  if (priv->sort_func != NULL)
    iter = g_sequence_insert_sorted (priv->children, child,
                                     (GCompareDataFunc)ctk_flow_box_sort, box);
  else if (position == 0)
    iter = g_sequence_prepend (priv->children, child);
  else if (position == -1)
    iter = g_sequence_append (priv->children, child);
  else
    {
      GSequenceIter *pos;
      pos = g_sequence_get_iter_at_pos (priv->children, position);
      iter = g_sequence_insert_before (pos, child);
    }

  ctk_flow_box_insert_css_node (box, CTK_WIDGET (child), iter);

  CHILD_PRIV (child)->iter = iter;
  ctk_widget_set_parent (CTK_WIDGET (child), CTK_WIDGET (box));
  ctk_flow_box_apply_filter (box, child);
}

/**
 * ctk_flow_box_get_child_at_index:
 * @box: a #CtkFlowBox
 * @idx: the position of the child
 *
 * Gets the nth child in the @box.
 *
 * Returns: (transfer none) (nullable): the child widget, which will
 *     always be a #CtkFlowBoxChild or %NULL in case no child widget
 *     with the given index exists.
 *
 * Since: 3.12
 */
CtkFlowBoxChild *
ctk_flow_box_get_child_at_index (CtkFlowBox *box,
                                 gint        idx)
{
  GSequenceIter *iter;

  g_return_val_if_fail (CTK_IS_FLOW_BOX (box), NULL);

  iter = g_sequence_get_iter_at_pos (BOX_PRIV (box)->children, idx);
  if (!g_sequence_iter_is_end (iter))
    return g_sequence_get (iter);

  return NULL;
}

/**
 * ctk_flow_box_get_child_at_pos:
 * @box: a #CtkFlowBox
 * @x: the x coordinate of the child
 * @y: the y coordinate of the child
 *
 * Gets the child in the (@x, @y) position.
 *
 * Returns: (transfer none) (nullable): the child widget, which will
 *     always be a #CtkFlowBoxChild or %NULL in case no child widget
 *     exists for the given x and y coordinates.
 *
 * Since: 3.22.6
 */
CtkFlowBoxChild *
ctk_flow_box_get_child_at_pos (CtkFlowBox *box,
                               gint        x,
                               gint        y)
{
  CtkWidget *child;
  GSequenceIter *iter;
  CtkAllocation allocation;

  for (iter = g_sequence_get_begin_iter (BOX_PRIV (box)->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      child = g_sequence_get (iter);
      if (!child_is_visible (child))
        continue;
      ctk_widget_get_allocation (child, &allocation);
      if (x >= allocation.x && x < (allocation.x + allocation.width) &&
          y >= allocation.y && y < (allocation.y + allocation.height))
        return CTK_FLOW_BOX_CHILD (child);
    }

  return NULL;
}

/**
 * ctk_flow_box_set_hadjustment:
 * @box: a #CtkFlowBox
 * @adjustment: an adjustment which should be adjusted
 *    when the focus is moved among the descendents of @container
 *
 * Hooks up an adjustment to focus handling in @box.
 * The adjustment is also used for autoscrolling during
 * rubberband selection. See ctk_scrolled_window_get_hadjustment()
 * for a typical way of obtaining the adjustment, and
 * ctk_flow_box_set_vadjustment()for setting the vertical
 * adjustment.
 *
 * The adjustments have to be in pixel units and in the same
 * coordinate system as the allocation for immediate children
 * of the box.
 *
 * Since: 3.12
 */
void
ctk_flow_box_set_hadjustment (CtkFlowBox    *box,
                              CtkAdjustment *adjustment)
{
  CtkFlowBoxPrivate *priv;

  g_return_if_fail (CTK_IS_FLOW_BOX (box));
  g_return_if_fail (CTK_IS_ADJUSTMENT (adjustment));

  priv = BOX_PRIV (box);

  g_object_ref (adjustment);
  if (priv->hadjustment)
    g_object_unref (priv->hadjustment);
  priv->hadjustment = adjustment;
  ctk_container_set_focus_hadjustment (CTK_CONTAINER (box), adjustment);
}

/**
 * ctk_flow_box_set_vadjustment:
 * @box: a #CtkFlowBox
 * @adjustment: an adjustment which should be adjusted
 *    when the focus is moved among the descendents of @container
 *
 * Hooks up an adjustment to focus handling in @box.
 * The adjustment is also used for autoscrolling during
 * rubberband selection. See ctk_scrolled_window_get_vadjustment()
 * for a typical way of obtaining the adjustment, and
 * ctk_flow_box_set_hadjustment()for setting the horizontal
 * adjustment.
 *
 * The adjustments have to be in pixel units and in the same
 * coordinate system as the allocation for immediate children
 * of the box.
 *
 * Since: 3.12
 */
void
ctk_flow_box_set_vadjustment (CtkFlowBox    *box,
                              CtkAdjustment *adjustment)
{
  CtkFlowBoxPrivate *priv;

  g_return_if_fail (CTK_IS_FLOW_BOX (box));
  g_return_if_fail (CTK_IS_ADJUSTMENT (adjustment));

  priv = BOX_PRIV (box);

  g_object_ref (adjustment);
  if (priv->vadjustment)
    g_object_unref (priv->vadjustment);
  priv->vadjustment = adjustment;
  ctk_container_set_focus_vadjustment (CTK_CONTAINER (box), adjustment);
}

static void
ctk_flow_box_check_model_compat (CtkFlowBox *box)
{
  CtkFlowBoxPrivate *priv = BOX_PRIV (box);

  if (priv->bound_model &&
      (priv->sort_func || priv->filter_func))
    g_warning ("CtkFlowBox with a model will ignore sort and filter functions");
}

/**
 * ctk_flow_box_bind_model:
 * @box: a #CtkFlowBox
 * @model: (allow-none): the #GListModel to be bound to @box
 * @create_widget_func: a function that creates widgets for items
 * @user_data: user data passed to @create_widget_func
 * @user_data_free_func: function for freeing @user_data
 *
 * Binds @model to @box.
 *
 * If @box was already bound to a model, that previous binding is
 * destroyed.
 *
 * The contents of @box are cleared and then filled with widgets that
 * represent items from @model. @box is updated whenever @model changes.
 * If @model is %NULL, @box is left empty.
 *
 * It is undefined to add or remove widgets directly (for example, with
 * ctk_flow_box_insert() or ctk_container_add()) while @box is bound to a
 * model.
 *
 * Note that using a model is incompatible with the filtering and sorting
 * functionality in CtkFlowBox. When using a model, filtering and sorting
 * should be implemented by the model.
 *
 * Since: 3.18
 */
void
ctk_flow_box_bind_model (CtkFlowBox                 *box,
                         GListModel                 *model,
                         CtkFlowBoxCreateWidgetFunc  create_widget_func,
                         gpointer                    user_data,
                         GDestroyNotify              user_data_free_func)
{
  CtkFlowBoxPrivate *priv = BOX_PRIV (box);

  g_return_if_fail (CTK_IS_FLOW_BOX (box));
  g_return_if_fail (model == NULL || G_IS_LIST_MODEL (model));
  g_return_if_fail (model == NULL || create_widget_func != NULL);

  if (priv->bound_model)
    {
      if (priv->create_widget_func_data_destroy)
        priv->create_widget_func_data_destroy (priv->create_widget_func_data);

      g_signal_handlers_disconnect_by_func (priv->bound_model, ctk_flow_box_bound_model_changed, box);
      g_clear_object (&priv->bound_model);
    }

  ctk_flow_box_forall (CTK_CONTAINER (box), FALSE, (CtkCallback) ctk_widget_destroy, NULL);

  if (model == NULL)
    return;

  priv->bound_model = g_object_ref (model);
  priv->create_widget_func = create_widget_func;
  priv->create_widget_func_data = user_data;
  priv->create_widget_func_data_destroy = user_data_free_func;

  ctk_flow_box_check_model_compat (box);

  g_signal_connect (priv->bound_model, "items-changed", G_CALLBACK (ctk_flow_box_bound_model_changed), box);
  ctk_flow_box_bound_model_changed (model, 0, 0, g_list_model_get_n_items (model), box);
}

/* Setters and getters {{{2 */

/**
 * ctk_flow_box_get_homogeneous:
 * @box: a #CtkFlowBox
 *
 * Returns whether the box is homogeneous (all children are the
 * same size). See ctk_box_set_homogeneous().
 *
 * Returns: %TRUE if the box is homogeneous.
 *
 * Since: 3.12
 */
gboolean
ctk_flow_box_get_homogeneous (CtkFlowBox *box)
{
  g_return_val_if_fail (CTK_IS_FLOW_BOX (box), FALSE);

  return BOX_PRIV (box)->homogeneous;
}

/**
 * ctk_flow_box_set_homogeneous:
 * @box: a #CtkFlowBox
 * @homogeneous: %TRUE to create equal allotments,
 *   %FALSE for variable allotments
 *
 * Sets the #CtkFlowBox:homogeneous property of @box, controlling
 * whether or not all children of @box are given equal space
 * in the box.
 *
 * Since: 3.12
 */
void
ctk_flow_box_set_homogeneous (CtkFlowBox *box,
                              gboolean    homogeneous)
{
  g_return_if_fail (CTK_IS_FLOW_BOX (box));

  homogeneous = homogeneous != FALSE;

  if (BOX_PRIV (box)->homogeneous != homogeneous)
    {
      BOX_PRIV (box)->homogeneous = homogeneous;

      g_object_notify_by_pspec (G_OBJECT (box), props[PROP_HOMOGENEOUS]);
      ctk_widget_queue_resize (CTK_WIDGET (box));
    }
}

/**
 * ctk_flow_box_set_row_spacing:
 * @box: a #CtkFlowBox
 * @spacing: the spacing to use
 *
 * Sets the vertical space to add between children.
 * See the #CtkFlowBox:row-spacing property.
 *
 * Since: 3.12
 */
void
ctk_flow_box_set_row_spacing (CtkFlowBox *box,
                              guint       spacing)
{
  g_return_if_fail (CTK_IS_FLOW_BOX (box));

  if (BOX_PRIV (box)->row_spacing != spacing)
    {
      BOX_PRIV (box)->row_spacing = spacing;

      ctk_widget_queue_resize (CTK_WIDGET (box));
      g_object_notify_by_pspec (G_OBJECT (box), props[PROP_ROW_SPACING]);
    }
}

/**
 * ctk_flow_box_get_row_spacing:
 * @box: a #CtkFlowBox
 *
 * Gets the vertical spacing.
 *
 * Returns: the vertical spacing
 *
 * Since: 3.12
 */
guint
ctk_flow_box_get_row_spacing (CtkFlowBox *box)
{
  g_return_val_if_fail (CTK_IS_FLOW_BOX (box), FALSE);

  return BOX_PRIV (box)->row_spacing;
}

/**
 * ctk_flow_box_set_column_spacing:
 * @box: a #CtkFlowBox
 * @spacing: the spacing to use
 *
 * Sets the horizontal space to add between children.
 * See the #CtkFlowBox:column-spacing property.
 *
 * Since: 3.12
 */
void
ctk_flow_box_set_column_spacing (CtkFlowBox *box,
                                 guint       spacing)
{
  g_return_if_fail (CTK_IS_FLOW_BOX (box));

  if (BOX_PRIV (box)->column_spacing != spacing)
    {
      BOX_PRIV (box)->column_spacing = spacing;

      ctk_widget_queue_resize (CTK_WIDGET (box));
      g_object_notify_by_pspec (G_OBJECT (box), props[PROP_COLUMN_SPACING]);
    }
}

/**
 * ctk_flow_box_get_column_spacing:
 * @box: a #CtkFlowBox
 *
 * Gets the horizontal spacing.
 *
 * Returns: the horizontal spacing
 *
 * Since: 3.12
 */
guint
ctk_flow_box_get_column_spacing (CtkFlowBox *box)
{
  g_return_val_if_fail (CTK_IS_FLOW_BOX (box), FALSE);

  return BOX_PRIV (box)->column_spacing;
}

/**
 * ctk_flow_box_set_min_children_per_line:
 * @box: a #CtkFlowBox
 * @n_children: the minimum number of children per line
 *
 * Sets the minimum number of children to line up
 * in @box’s orientation before flowing.
 *
 * Since: 3.12
 */
void
ctk_flow_box_set_min_children_per_line (CtkFlowBox *box,
                                        guint       n_children)
{
  g_return_if_fail (CTK_IS_FLOW_BOX (box));

  if (BOX_PRIV (box)->min_children_per_line != n_children)
    {
      BOX_PRIV (box)->min_children_per_line = n_children;

      ctk_widget_queue_resize (CTK_WIDGET (box));
      g_object_notify_by_pspec (G_OBJECT (box), props[PROP_MIN_CHILDREN_PER_LINE]);
    }
}

/**
 * ctk_flow_box_get_min_children_per_line:
 * @box: a #CtkFlowBox
 *
 * Gets the minimum number of children per line.
 *
 * Returns: the minimum number of children per line
 *
 * Since: 3.12
 */
guint
ctk_flow_box_get_min_children_per_line (CtkFlowBox *box)
{
  g_return_val_if_fail (CTK_IS_FLOW_BOX (box), FALSE);

  return BOX_PRIV (box)->min_children_per_line;
}

/**
 * ctk_flow_box_set_max_children_per_line:
 * @box: a #CtkFlowBox
 * @n_children: the maximum number of children per line
 *
 * Sets the maximum number of children to request and
 * allocate space for in @box’s orientation.
 *
 * Setting the maximum number of children per line
 * limits the overall natural size request to be no more
 * than @n_children children long in the given orientation.
 *
 * Since: 3.12
 */
void
ctk_flow_box_set_max_children_per_line (CtkFlowBox *box,
                                        guint       n_children)
{
  g_return_if_fail (CTK_IS_FLOW_BOX (box));
  g_return_if_fail (n_children > 0);

  if (BOX_PRIV (box)->max_children_per_line != n_children)
    {
      BOX_PRIV (box)->max_children_per_line = n_children;

      ctk_widget_queue_resize (CTK_WIDGET (box));
      g_object_notify_by_pspec (G_OBJECT (box), props[PROP_MAX_CHILDREN_PER_LINE]);
    }
}

/**
 * ctk_flow_box_get_max_children_per_line:
 * @box: a #CtkFlowBox
 *
 * Gets the maximum number of children per line.
 *
 * Returns: the maximum number of children per line
 *
 * Since: 3.12
 */
guint
ctk_flow_box_get_max_children_per_line (CtkFlowBox *box)
{
  g_return_val_if_fail (CTK_IS_FLOW_BOX (box), FALSE);

  return BOX_PRIV (box)->max_children_per_line;
}

/**
 * ctk_flow_box_set_activate_on_single_click:
 * @box: a #CtkFlowBox
 * @single: %TRUE to emit child-activated on a single click
 *
 * If @single is %TRUE, children will be activated when you click
 * on them, otherwise you need to double-click.
 *
 * Since: 3.12
 */
void
ctk_flow_box_set_activate_on_single_click (CtkFlowBox *box,
                                           gboolean    single)
{
  g_return_if_fail (CTK_IS_FLOW_BOX (box));

  single = single != FALSE;

  if (BOX_PRIV (box)->activate_on_single_click != single)
    {
      BOX_PRIV (box)->activate_on_single_click = single;
      g_object_notify_by_pspec (G_OBJECT (box), props[PROP_ACTIVATE_ON_SINGLE_CLICK]);
    }
}

/**
 * ctk_flow_box_get_activate_on_single_click:
 * @box: a #CtkFlowBox
 *
 * Returns whether children activate on single clicks.
 *
 * Returns: %TRUE if children are activated on single click,
 *     %FALSE otherwise
 *
 * Since: 3.12
 */
gboolean
ctk_flow_box_get_activate_on_single_click (CtkFlowBox *box)
{
  g_return_val_if_fail (CTK_IS_FLOW_BOX (box), FALSE);

  return BOX_PRIV (box)->activate_on_single_click;
}
 
 /* Selection handling {{{2 */

/**
 * ctk_flow_box_get_selected_children:
 * @box: a #CtkFlowBox
 *
 * Creates a list of all selected children.
 *
 * Returns: (element-type CtkFlowBoxChild) (transfer container):
 *     A #GList containing the #CtkWidget for each selected child.
 *     Free with g_list_free() when done.
 *
 * Since: 3.12
 */
GList *
ctk_flow_box_get_selected_children (CtkFlowBox *box)
{
  CtkFlowBoxChild *child;
  GSequenceIter *iter;
  GList *selected = NULL;

  g_return_val_if_fail (CTK_IS_FLOW_BOX (box), NULL);

  for (iter = g_sequence_get_begin_iter (BOX_PRIV (box)->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      child = g_sequence_get (iter);
      if (CHILD_PRIV (child)->selected)
        selected = g_list_prepend (selected, child);
    }

  return g_list_reverse (selected);
}

/**
 * ctk_flow_box_select_child:
 * @box: a #CtkFlowBox
 * @child: a child of @box
 *
 * Selects a single child of @box, if the selection
 * mode allows it.
 *
 * Since: 3.12
 */
void
ctk_flow_box_select_child (CtkFlowBox      *box,
                           CtkFlowBoxChild *child)
{
  g_return_if_fail (CTK_IS_FLOW_BOX (box));
  g_return_if_fail (CTK_IS_FLOW_BOX_CHILD (child));

  ctk_flow_box_select_child_internal (box, child);
}

/**
 * ctk_flow_box_unselect_child:
 * @box: a #CtkFlowBox
 * @child: a child of @box
 *
 * Unselects a single child of @box, if the selection
 * mode allows it.
 *
 * Since: 3.12
 */
void
ctk_flow_box_unselect_child (CtkFlowBox      *box,
                             CtkFlowBoxChild *child)
{
  g_return_if_fail (CTK_IS_FLOW_BOX (box));
  g_return_if_fail (CTK_IS_FLOW_BOX_CHILD (child));

  ctk_flow_box_unselect_child_internal (box, child);
}

/**
 * ctk_flow_box_select_all:
 * @box: a #CtkFlowBox
 *
 * Select all children of @box, if the selection
 * mode allows it.
 *
 * Since: 3.12
 */
void
ctk_flow_box_select_all (CtkFlowBox *box)
{
  g_return_if_fail (CTK_IS_FLOW_BOX (box));

  if (BOX_PRIV (box)->selection_mode != CTK_SELECTION_MULTIPLE)
    return;

  if (g_sequence_get_length (BOX_PRIV (box)->children) > 0)
    {
      ctk_flow_box_select_all_between (box, NULL, NULL, FALSE);
      g_signal_emit (box, signals[SELECTED_CHILDREN_CHANGED], 0);
    }
}

/**
 * ctk_flow_box_unselect_all:
 * @box: a #CtkFlowBox
 *
 * Unselect all children of @box, if the selection
 * mode allows it.
 *
 * Since: 3.12
 */
void
ctk_flow_box_unselect_all (CtkFlowBox *box)
{
  gboolean dirty = FALSE;

  g_return_if_fail (CTK_IS_FLOW_BOX (box));

  if (BOX_PRIV (box)->selection_mode == CTK_SELECTION_BROWSE)
    return;

  dirty = ctk_flow_box_unselect_all_internal (box);

  if (dirty)
    g_signal_emit (box, signals[SELECTED_CHILDREN_CHANGED], 0);
}

/**
 * CtkFlowBoxForeachFunc:
 * @box: a #CtkFlowBox
 * @child: a #CtkFlowBoxChild
 * @user_data: (closure): user data
 *
 * A function used by ctk_flow_box_selected_foreach().
 * It will be called on every selected child of the @box.
 *
 * Since: 3.12
 */

/**
 * ctk_flow_box_selected_foreach:
 * @box: a #CtkFlowBox
 * @func: (scope call): the function to call for each selected child
 * @data: user data to pass to the function
 *
 * Calls a function for each selected child.
 *
 * Note that the selection cannot be modified from within
 * this function.
 *
 * Since: 3.12
 */
void
ctk_flow_box_selected_foreach (CtkFlowBox            *box,
                               CtkFlowBoxForeachFunc  func,
                               gpointer               data)
{
  CtkFlowBoxChild *child;
  GSequenceIter *iter;

  g_return_if_fail (CTK_IS_FLOW_BOX (box));

  for (iter = g_sequence_get_begin_iter (BOX_PRIV (box)->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      child = g_sequence_get (iter);
      if (CHILD_PRIV (child)->selected)
        (*func) (box, child, data);
    }
}

/**
 * ctk_flow_box_set_selection_mode:
 * @box: a #CtkFlowBox
 * @mode: the new selection mode
 *
 * Sets how selection works in @box.
 * See #CtkSelectionMode for details.
 *
 * Since: 3.12
 */
void
ctk_flow_box_set_selection_mode (CtkFlowBox       *box,
                                 CtkSelectionMode  mode)
{
  gboolean dirty = FALSE;

  g_return_if_fail (CTK_IS_FLOW_BOX (box));

  if (mode == BOX_PRIV (box)->selection_mode)
    return;

  if (mode == CTK_SELECTION_NONE ||
      BOX_PRIV (box)->selection_mode == CTK_SELECTION_MULTIPLE)
    {
      dirty = ctk_flow_box_unselect_all_internal (box);
      BOX_PRIV (box)->selected_child = NULL;
    }

  BOX_PRIV (box)->selection_mode = mode;

  g_object_notify_by_pspec (G_OBJECT (box), props[PROP_SELECTION_MODE]);

  if (dirty)
    g_signal_emit (box, signals[SELECTED_CHILDREN_CHANGED], 0);
}

/**
 * ctk_flow_box_get_selection_mode:
 * @box: a #CtkFlowBox
 *
 * Gets the selection mode of @box.
 *
 * Returns: the #CtkSelectionMode
 *
 * Since: 3.12
 */
CtkSelectionMode
ctk_flow_box_get_selection_mode (CtkFlowBox *box)
{
  g_return_val_if_fail (CTK_IS_FLOW_BOX (box), CTK_SELECTION_SINGLE);

  return BOX_PRIV (box)->selection_mode;
}

/* Filtering {{{2 */

/**
 * CtkFlowBoxFilterFunc:
 * @child: a #CtkFlowBoxChild that may be filtered
 * @user_data: (closure): user data
 *
 * A function that will be called whenrever a child changes
 * or is added. It lets you control if the child should be
 * visible or not.
 *
 * Returns: %TRUE if the row should be visible, %FALSE otherwise
 *
 * Since: 3.12
 */

/**
 * ctk_flow_box_set_filter_func:
 * @box: a #CtkFlowBox
 * @filter_func: (closure user_data) (allow-none): callback that
 *     lets you filter which children to show
 * @user_data: user data passed to @filter_func
 * @destroy: destroy notifier for @user_data
 *
 * By setting a filter function on the @box one can decide dynamically
 * which of the children to show. For instance, to implement a search
 * function that only shows the children matching the search terms.
 *
 * The @filter_func will be called for each child after the call, and
 * it will continue to be called each time a child changes (via
 * ctk_flow_box_child_changed()) or when ctk_flow_box_invalidate_filter()
 * is called.
 *
 * Note that using a filter function is incompatible with using a model
 * (see ctk_flow_box_bind_model()).
 *
 * Since: 3.12
 */
void
ctk_flow_box_set_filter_func (CtkFlowBox           *box,
                              CtkFlowBoxFilterFunc  filter_func,
                              gpointer              user_data,
                              GDestroyNotify        destroy)
{
  CtkFlowBoxPrivate *priv;

  g_return_if_fail (CTK_IS_FLOW_BOX (box));

  priv = BOX_PRIV (box);

  if (priv->filter_destroy != NULL)
    priv->filter_destroy (priv->filter_data);

  priv->filter_func = filter_func;
  priv->filter_data = user_data;
  priv->filter_destroy = destroy;

  ctk_flow_box_check_model_compat (box);

  ctk_flow_box_apply_filter_all (box);
}

/**
 * ctk_flow_box_invalidate_filter:
 * @box: a #CtkFlowBox
 *
 * Updates the filtering for all children.
 *
 * Call this function when the result of the filter
 * function on the @box is changed due ot an external
 * factor. For instance, this would be used if the
 * filter function just looked for a specific search
 * term, and the entry with the string has changed.
 *
 * Since: 3.12
 */
void
ctk_flow_box_invalidate_filter (CtkFlowBox *box)
{
  g_return_if_fail (CTK_IS_FLOW_BOX (box));

  if (BOX_PRIV (box)->filter_func != NULL)
    ctk_flow_box_apply_filter_all (box);
}

/* Sorting {{{2 */

/**
 * CtkFlowBoxSortFunc:
 * @child1: the first child
 * @child2: the second child
 * @user_data: (closure): user data
 *
 * A function to compare two children to determine which
 * should come first.
 *
 * Returns: < 0 if @child1 should be before @child2, 0 if
 *     the are equal, and > 0 otherwise
 *
 * Since: 3.12
 */

/**
 * ctk_flow_box_set_sort_func:
 * @box: a #CtkFlowBox
 * @sort_func: (closure user_data) (allow-none): the sort function
 * @user_data: user data passed to @sort_func
 * @destroy: destroy notifier for @user_data
 *
 * By setting a sort function on the @box, one can dynamically
 * reorder the children of the box, based on the contents of
 * the children.
 *
 * The @sort_func will be called for each child after the call,
 * and will continue to be called each time a child changes (via
 * ctk_flow_box_child_changed()) and when ctk_flow_box_invalidate_sort()
 * is called.
 *
 * Note that using a sort function is incompatible with using a model
 * (see ctk_flow_box_bind_model()).
 *
 * Since: 3.12
 */
void
ctk_flow_box_set_sort_func (CtkFlowBox         *box,
                            CtkFlowBoxSortFunc  sort_func,
                            gpointer            user_data,
                            GDestroyNotify      destroy)
{
  CtkFlowBoxPrivate *priv;

  g_return_if_fail (CTK_IS_FLOW_BOX (box));

  priv = BOX_PRIV (box);

  if (priv->sort_destroy != NULL)
    priv->sort_destroy (priv->sort_data);

  priv->sort_func = sort_func;
  priv->sort_data = user_data;
  priv->sort_destroy = destroy;

  ctk_flow_box_check_model_compat (box);

  ctk_flow_box_invalidate_sort (box);
}

static gint
ctk_flow_box_sort (CtkFlowBoxChild *a,
                   CtkFlowBoxChild *b,
                   CtkFlowBox      *box)
{
  CtkFlowBoxPrivate *priv = BOX_PRIV (box);

  return priv->sort_func (a, b, priv->sort_data);
}

static void
ctk_flow_box_css_node_foreach (gpointer data,
                               gpointer user_data)
{
  CtkWidget **previous = user_data;
  CtkWidget *row = data;
  CtkCssNode *row_node;
  CtkCssNode *prev_node;

  if (*previous)
    {
      prev_node = ctk_widget_get_css_node (*previous);
      row_node = ctk_widget_get_css_node (row);
      ctk_css_node_insert_after (ctk_css_node_get_parent (row_node),
                                 row_node,
                                 prev_node);
    }

  *previous = row;
}

/**
 * ctk_flow_box_invalidate_sort:
 * @box: a #CtkFlowBox
 *
 * Updates the sorting for all children.
 *
 * Call this when the result of the sort function on
 * @box is changed due to an external factor.
 *
 * Since: 3.12
 */
void
ctk_flow_box_invalidate_sort (CtkFlowBox *box)
{
  CtkFlowBoxPrivate *priv;
  CtkWidget *previous = NULL;

  g_return_if_fail (CTK_IS_FLOW_BOX (box));

  priv = BOX_PRIV (box);

  if (priv->sort_func != NULL)
    {
      g_sequence_sort (priv->children, (GCompareDataFunc)ctk_flow_box_sort, box);
      g_sequence_foreach (priv->children, ctk_flow_box_css_node_foreach, &previous);
      ctk_widget_queue_resize (CTK_WIDGET (box));
    }
}

/* vim:set foldmethod=marker expandtab: */
