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
 * SECTION:ctkfixed
 * @Short_description: A container which allows you to position
 * widgets at fixed coordinates
 * @Title: CtkFixed
 * @See_also: #CtkLayout
 *
 * The #CtkFixed widget is a container which can place child widgets
 * at fixed positions and with fixed sizes, given in pixels. #CtkFixed
 * performs no automatic layout management.
 *
 * For most applications, you should not use this container! It keeps
 * you from having to learn about the other CTK+ containers, but it
 * results in broken applications.  With #CtkFixed, the following
 * things will result in truncated text, overlapping widgets, and
 * other display bugs:
 *
 * - Themes, which may change widget sizes.
 *
 * - Fonts other than the one you used to write the app will of course
 *   change the size of widgets containing text; keep in mind that
 *   users may use a larger font because of difficulty reading the
 *   default, or they may be using a different OS that provides different fonts.
 *
 * - Translation of text into other languages changes its size. Also,
 *   display of non-English text will use a different font in many
 *   cases.
 *
 * In addition, #CtkFixed does not pay attention to text direction and thus may
 * produce unwanted results if your app is run under right-to-left languages
 * such as Hebrew or Arabic. That is: normally CTK+ will order containers
 * appropriately for the text direction, e.g. to put labels to the right of the
 * thing they label when using an RTL language, but it can’t do that with
 * #CtkFixed. So if you need to reorder widgets depending on the text direction,
 * you would need to manually detect it and adjust child positions accordingly.
 *
 * Finally, fixed positioning makes it kind of annoying to add/remove
 * GUI elements, since you have to reposition all the other
 * elements. This is a long-term maintenance problem for your
 * application.
 *
 * If you know none of these things are an issue for your application,
 * and prefer the simplicity of #CtkFixed, by all means use the
 * widget. But you should be aware of the tradeoffs.
 *
 * See also #CtkLayout, which shares the ability to perform fixed positioning
 * of child widgets and additionally adds custom drawing and scrollability.
 */

#include "config.h"

#include "ctkfixed.h"

#include "ctkwidgetprivate.h"
#include "ctkprivate.h"
#include "ctkintl.h"


struct _CtkFixedPrivate
{
  GList *children;
};

enum {
  CHILD_PROP_0,
  CHILD_PROP_X,
  CHILD_PROP_Y
};

static void ctk_fixed_realize       (CtkWidget        *widget);
static void ctk_fixed_get_preferred_width  (CtkWidget *widget,
                                            gint      *minimum,
                                            gint      *natural);
static void ctk_fixed_get_preferred_height (CtkWidget *widget,
                                            gint      *minimum,
                                            gint      *natural);
static void ctk_fixed_size_allocate (CtkWidget        *widget,
                                     CtkAllocation    *allocation);
static void ctk_fixed_style_updated (CtkWidget        *widget);
static gboolean ctk_fixed_draw      (CtkWidget        *widget,
                                     cairo_t          *cr);
static void ctk_fixed_add           (CtkContainer     *container,
                                     CtkWidget        *widget);
static void ctk_fixed_remove        (CtkContainer     *container,
                                     CtkWidget        *widget);
static void ctk_fixed_forall        (CtkContainer     *container,
                                     gboolean          include_internals,
                                     CtkCallback       callback,
                                     gpointer          callback_data);
static GType ctk_fixed_child_type   (CtkContainer     *container);

static void ctk_fixed_set_child_property (CtkContainer *container,
                                          CtkWidget    *child,
                                          guint         property_id,
                                          const GValue *value,
                                          GParamSpec   *pspec);
static void ctk_fixed_get_child_property (CtkContainer *container,
                                          CtkWidget    *child,
                                          guint         property_id,
                                          GValue       *value,
                                          GParamSpec   *pspec);

G_DEFINE_TYPE_WITH_PRIVATE (CtkFixed, ctk_fixed, CTK_TYPE_CONTAINER)

static void
ctk_fixed_class_init (CtkFixedClass *class)
{
  CtkWidgetClass *widget_class;
  CtkContainerClass *container_class;

  widget_class = (CtkWidgetClass*) class;
  container_class = (CtkContainerClass*) class;

  widget_class->realize = ctk_fixed_realize;
  widget_class->get_preferred_width = ctk_fixed_get_preferred_width;
  widget_class->get_preferred_height = ctk_fixed_get_preferred_height;
  widget_class->size_allocate = ctk_fixed_size_allocate;
  widget_class->draw = ctk_fixed_draw;
  widget_class->style_updated = ctk_fixed_style_updated;

  container_class->add = ctk_fixed_add;
  container_class->remove = ctk_fixed_remove;
  container_class->forall = ctk_fixed_forall;
  container_class->child_type = ctk_fixed_child_type;
  container_class->set_child_property = ctk_fixed_set_child_property;
  container_class->get_child_property = ctk_fixed_get_child_property;
  ctk_container_class_handle_border_width (container_class);

  ctk_container_class_install_child_property (container_class,
                                              CHILD_PROP_X,
                                              g_param_spec_int ("x",
                                                                P_("X position"),
                                                                P_("X position of child widget"),
                                                                G_MININT, G_MAXINT, 0,
                                                                CTK_PARAM_READWRITE));

  ctk_container_class_install_child_property (container_class,
                                              CHILD_PROP_Y,
                                              g_param_spec_int ("y",
                                                                P_("Y position"),
                                                                P_("Y position of child widget"),
                                                                G_MININT, G_MAXINT, 0,
                                                                CTK_PARAM_READWRITE));
}

static GType
ctk_fixed_child_type (CtkContainer *container G_GNUC_UNUSED)
{
  return CTK_TYPE_WIDGET;
}

static void
ctk_fixed_init (CtkFixed *fixed)
{
  fixed->priv = ctk_fixed_get_instance_private (fixed);

  ctk_widget_set_has_window (CTK_WIDGET (fixed), FALSE);

  fixed->priv->children = NULL;
}

/**
 * ctk_fixed_new:
 *
 * Creates a new #CtkFixed.
 *
 * Returns: a new #CtkFixed.
 */
CtkWidget*
ctk_fixed_new (void)
{
  return g_object_new (CTK_TYPE_FIXED, NULL);
}

static CtkFixedChild*
get_child (CtkFixed  *fixed,
           CtkWidget *widget)
{
  CtkFixedPrivate *priv = fixed->priv;
  GList *children;

  for (children = priv->children; children; children = children->next)
    {
      CtkFixedChild *child;

      child = children->data;

      if (child->widget == widget)
        return child;
    }

  return NULL;
}

/**
 * ctk_fixed_put:
 * @fixed: a #CtkFixed.
 * @widget: the widget to add.
 * @x: the horizontal position to place the widget at.
 * @y: the vertical position to place the widget at.
 *
 * Adds a widget to a #CtkFixed container at the given position.
 */
void
ctk_fixed_put (CtkFixed  *fixed,
               CtkWidget *widget,
               gint       x,
               gint       y)
{
  CtkFixedPrivate *priv;
  CtkFixedChild *child_info;

  g_return_if_fail (CTK_IS_FIXED (fixed));
  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (_ctk_widget_get_parent (widget) == NULL);

  priv = fixed->priv;

  child_info = g_new (CtkFixedChild, 1);
  child_info->widget = widget;
  child_info->x = x;
  child_info->y = y;

  ctk_widget_set_parent (widget, CTK_WIDGET (fixed));

  priv->children = g_list_append (priv->children, child_info);
}

static void
ctk_fixed_move_internal (CtkFixed      *fixed,
                         CtkFixedChild *child,
                         gint           x,
                         gint           y)
{
  g_return_if_fail (CTK_IS_FIXED (fixed));
  g_return_if_fail (ctk_widget_get_parent (child->widget) == CTK_WIDGET (fixed));

  ctk_widget_freeze_child_notify (child->widget);

  if (child->x != x)
    {
      child->x = x;
      ctk_widget_child_notify (child->widget, "x");
    }

  if (child->y != y)
    {
      child->y = y;
      ctk_widget_child_notify (child->widget, "y");
    }

  ctk_widget_thaw_child_notify (child->widget);

  if (ctk_widget_get_visible (child->widget) &&
      ctk_widget_get_visible (CTK_WIDGET (fixed)))
    ctk_widget_queue_resize (CTK_WIDGET (fixed));
}

/**
 * ctk_fixed_move:
 * @fixed: a #CtkFixed.
 * @widget: the child widget.
 * @x: the horizontal position to move the widget to.
 * @y: the vertical position to move the widget to.
 *
 * Moves a child of a #CtkFixed container to the given position.
 */
void
ctk_fixed_move (CtkFixed  *fixed,
                CtkWidget *widget,
                gint       x,
                gint       y)
{
  ctk_fixed_move_internal (fixed, get_child (fixed, widget), x, y);
}

static void
ctk_fixed_set_child_property (CtkContainer *container,
                              CtkWidget    *child,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  CtkFixed *fixed = CTK_FIXED (container);
  CtkFixedChild *fixed_child;

  fixed_child = get_child (fixed, child);

  switch (property_id)
    {
    case CHILD_PROP_X:
      ctk_fixed_move_internal (fixed,
                               fixed_child,
                               g_value_get_int (value),
                               fixed_child->y);
      break;
    case CHILD_PROP_Y:
      ctk_fixed_move_internal (fixed,
                               fixed_child,
                               fixed_child->x,
                               g_value_get_int (value));
      break;
    default:
      CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
ctk_fixed_get_child_property (CtkContainer *container,
                              CtkWidget    *child,
                              guint         property_id,
                              GValue       *value,
                              GParamSpec   *pspec)
{
  CtkFixedChild *fixed_child;

  fixed_child = get_child (CTK_FIXED (container), child);
  
  switch (property_id)
    {
    case CHILD_PROP_X:
      g_value_set_int (value, fixed_child->x);
      break;
    case CHILD_PROP_Y:
      g_value_set_int (value, fixed_child->y);
      break;
    default:
      CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
set_background (CtkWidget *widget)
{
  if (ctk_widget_get_realized (widget))
    {
      /* We still need to call ctk_style_context_set_background() here for
       * CtkFixed, since subclasses like EmacsFixed depend on the X window
       * background to be set.
       * This should be revisited next time we have a major API break.
       */
      ctk_style_context_set_background (ctk_widget_get_style_context (widget),
                                        ctk_widget_get_window (widget));
    }
}

static void
ctk_fixed_style_updated (CtkWidget *widget)
{
  CTK_WIDGET_CLASS (ctk_fixed_parent_class)->style_updated (widget);

  set_background (widget);
}

static void
ctk_fixed_realize (CtkWidget *widget)
{
  CtkAllocation allocation;
  CdkWindow *window;
  CdkWindowAttr attributes;
  gint attributes_mask;

  if (!ctk_widget_get_has_window (widget))
    CTK_WIDGET_CLASS (ctk_fixed_parent_class)->realize (widget);
  else
    {
      ctk_widget_set_realized (widget, TRUE);

      ctk_widget_get_allocation (widget, &allocation);

      attributes.window_type = CDK_WINDOW_CHILD;
      attributes.x = allocation.x;
      attributes.y = allocation.y;
      attributes.width = allocation.width;
      attributes.height = allocation.height;
      attributes.wclass = CDK_INPUT_OUTPUT;
      attributes.visual = ctk_widget_get_visual (widget);
      attributes.event_mask = ctk_widget_get_events (widget);
      attributes.event_mask |= CDK_EXPOSURE_MASK | CDK_BUTTON_PRESS_MASK;

      attributes_mask = CDK_WA_X | CDK_WA_Y | CDK_WA_VISUAL;

      window = cdk_window_new (ctk_widget_get_parent_window (widget),
                               &attributes, attributes_mask);
      ctk_widget_set_window (widget, window);
      ctk_widget_register_window (widget, window);

      set_background (widget);
    }
}

static void
ctk_fixed_get_preferred_width (CtkWidget *widget,
                               gint      *minimum,
                               gint      *natural)
{
  CtkFixed *fixed = CTK_FIXED (widget);
  CtkFixedPrivate *priv = fixed->priv;
  CtkFixedChild *child;
  GList *children;
  gint child_min, child_nat;

  *minimum = 0;
  *natural = 0;

  for (children = priv->children; children; children = children->next)
    {
      child = children->data;

      if (!ctk_widget_get_visible (child->widget))
        continue;

      ctk_widget_get_preferred_width (child->widget, &child_min, &child_nat);

      *minimum = MAX (*minimum, child->x + child_min);
      *natural = MAX (*natural, child->x + child_nat);
    }
}

static void
ctk_fixed_get_preferred_height (CtkWidget *widget,
                                gint      *minimum,
                                gint      *natural)
{
  CtkFixed *fixed = CTK_FIXED (widget);
  CtkFixedPrivate *priv = fixed->priv;
  CtkFixedChild *child;
  GList *children;
  gint child_min, child_nat;

  *minimum = 0;
  *natural = 0;

  for (children = priv->children; children; children = children->next)
    {
      child = children->data;

      if (!ctk_widget_get_visible (child->widget))
        continue;

      ctk_widget_get_preferred_height (child->widget, &child_min, &child_nat);

      *minimum = MAX (*minimum, child->y + child_min);
      *natural = MAX (*natural, child->y + child_nat);
    }
}

static void
ctk_fixed_size_allocate (CtkWidget     *widget,
                         CtkAllocation *allocation)
{
  CtkFixed *fixed = CTK_FIXED (widget);
  CtkFixedPrivate *priv = fixed->priv;
  CtkFixedChild *child;
  CtkAllocation child_allocation;
  CtkRequisition child_requisition;
  GList *children;

  ctk_widget_set_allocation (widget, allocation);

  if (ctk_widget_get_has_window (widget))
    {
      if (ctk_widget_get_realized (widget))
        cdk_window_move_resize (ctk_widget_get_window (widget),
                                allocation->x,
                                allocation->y,
                                allocation->width,
                                allocation->height);
    }

  for (children = priv->children; children; children = children->next)
    {
      child = children->data;

      if (!ctk_widget_get_visible (child->widget))
        continue;

      ctk_widget_get_preferred_size (child->widget, &child_requisition, NULL);
      child_allocation.x = child->x;
      child_allocation.y = child->y;

      if (!ctk_widget_get_has_window (widget))
        {
          child_allocation.x += allocation->x;
          child_allocation.y += allocation->y;
        }

      child_allocation.width = child_requisition.width;
      child_allocation.height = child_requisition.height;
      ctk_widget_size_allocate (child->widget, &child_allocation);
    }
}

static void
ctk_fixed_add (CtkContainer *container,
               CtkWidget    *widget)
{
  ctk_fixed_put (CTK_FIXED (container), widget, 0, 0);
}

static void
ctk_fixed_remove (CtkContainer *container,
                  CtkWidget    *widget)
{
  CtkFixed *fixed = CTK_FIXED (container);
  CtkFixedPrivate *priv = fixed->priv;
  CtkFixedChild *child;
  CtkWidget *widget_container = CTK_WIDGET (container);
  GList *children;

  for (children = priv->children; children; children = children->next)
    {
      child = children->data;

      if (child->widget == widget)
        {
          gboolean was_visible = ctk_widget_get_visible (widget);

          ctk_widget_unparent (widget);

          priv->children = g_list_remove_link (priv->children, children);
          g_list_free (children);
          g_free (child);

          if (was_visible && ctk_widget_get_visible (widget_container))
            ctk_widget_queue_resize (widget_container);

          break;
        }
    }
}

static void
ctk_fixed_forall (CtkContainer *container,
                  gboolean      include_internals G_GNUC_UNUSED,
                  CtkCallback   callback,
                  gpointer      callback_data)
{
  CtkFixed *fixed = CTK_FIXED (container);
  CtkFixedPrivate *priv = fixed->priv;
  CtkFixedChild *child;
  GList *children;

  children = priv->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      (* callback) (child->widget, callback_data);
    }
}

static gboolean
ctk_fixed_draw (CtkWidget *widget,
                cairo_t   *cr)
{
  CtkFixed *fixed = CTK_FIXED (widget);
  CtkFixedPrivate *priv = fixed->priv;
  CtkFixedChild *child;
  GList *list;

  for (list = priv->children;
       list;
       list = list->next)
    {
      child = list->data;

      ctk_container_propagate_draw (CTK_CONTAINER (fixed),
                                    child->widget,
                                    cr);
    }
  
  return FALSE;
}

