/*
 * Copyright (c) 2013 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Alexander Larsson <alexl@redhat.com>
 *
 */

#include "config.h"

#include <ctk/ctk.h>
#include "ctkstack.h"
#include "ctkprivate.h"
#include "ctkintl.h"
#include "ctkcsscustomgadgetprivate.h"
#include "ctkcontainerprivate.h"
#include "ctkprogresstrackerprivate.h"
#include "ctksettingsprivate.h"
#include "ctkwidgetprivate.h"
#include "a11y/ctkstackaccessible.h"
#include "a11y/ctkstackaccessibleprivate.h"
#include <math.h>
#include <string.h>

/**
 * SECTION:ctkstack
 * @Short_description: A stacking container
 * @Title: CtkStack
 * @See_also: #CtkNotebook, #CtkStackSwitcher
 *
 * The CtkStack widget is a container which only shows
 * one of its children at a time. In contrast to CtkNotebook,
 * CtkStack does not provide a means for users to change the
 * visible child. Instead, the #CtkStackSwitcher widget can be
 * used with CtkStack to provide this functionality.
 *
 * Transitions between pages can be animated as slides or
 * fades. This can be controlled with ctk_stack_set_transition_type().
 * These animations respect the #CtkSettings:ctk-enable-animations
 * setting.
 *
 * The CtkStack widget was added in CTK+ 3.10.
 *
 * # CSS nodes
 *
 * CtkStack has a single CSS node named stack.
 */

/**
 * CtkStackTransitionType:
 * @CTK_STACK_TRANSITION_TYPE_NONE: No transition
 * @CTK_STACK_TRANSITION_TYPE_CROSSFADE: A cross-fade
 * @CTK_STACK_TRANSITION_TYPE_SLIDE_RIGHT: Slide from left to right
 * @CTK_STACK_TRANSITION_TYPE_SLIDE_LEFT: Slide from right to left
 * @CTK_STACK_TRANSITION_TYPE_SLIDE_UP: Slide from bottom up
 * @CTK_STACK_TRANSITION_TYPE_SLIDE_DOWN: Slide from top down
 * @CTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT: Slide from left or right according to the children order
 * @CTK_STACK_TRANSITION_TYPE_SLIDE_UP_DOWN: Slide from top down or bottom up according to the order
 * @CTK_STACK_TRANSITION_TYPE_OVER_UP: Cover the old page by sliding up. Since 3.12
 * @CTK_STACK_TRANSITION_TYPE_OVER_DOWN: Cover the old page by sliding down. Since: 3.12
 * @CTK_STACK_TRANSITION_TYPE_OVER_LEFT: Cover the old page by sliding to the left. Since: 3.12
 * @CTK_STACK_TRANSITION_TYPE_OVER_RIGHT: Cover the old page by sliding to the right. Since: 3.12
 * @CTK_STACK_TRANSITION_TYPE_UNDER_UP: Uncover the new page by sliding up. Since 3.12
 * @CTK_STACK_TRANSITION_TYPE_UNDER_DOWN: Uncover the new page by sliding down. Since: 3.12
 * @CTK_STACK_TRANSITION_TYPE_UNDER_LEFT: Uncover the new page by sliding to the left. Since: 3.12
 * @CTK_STACK_TRANSITION_TYPE_UNDER_RIGHT: Uncover the new page by sliding to the right. Since: 3.12
 * @CTK_STACK_TRANSITION_TYPE_OVER_UP_DOWN: Cover the old page sliding up or uncover the new page sliding down, according to order. Since: 3.12
 * @CTK_STACK_TRANSITION_TYPE_OVER_DOWN_UP: Cover the old page sliding down or uncover the new page sliding up, according to order. Since: 3.14
 * @CTK_STACK_TRANSITION_TYPE_OVER_LEFT_RIGHT: Cover the old page sliding left or uncover the new page sliding right, according to order. Since: 3.14
 * @CTK_STACK_TRANSITION_TYPE_OVER_RIGHT_LEFT: Cover the old page sliding right or uncover the new page sliding left, according to order. Since: 3.14
 *
 * These enumeration values describe the possible transitions
 * between pages in a #CtkStack widget.
 *
 * New values may be added to this enumeration over time.
 */

/* TODO:
 *  filter events out events to the last_child widget during transitions
 */

enum  {
  PROP_0,
  PROP_HOMOGENEOUS,
  PROP_HHOMOGENEOUS,
  PROP_VHOMOGENEOUS,
  PROP_VISIBLE_CHILD,
  PROP_VISIBLE_CHILD_NAME,
  PROP_TRANSITION_DURATION,
  PROP_TRANSITION_TYPE,
  PROP_TRANSITION_RUNNING,
  PROP_INTERPOLATE_SIZE,
  LAST_PROP
};

enum
{
  CHILD_PROP_0,
  CHILD_PROP_NAME,
  CHILD_PROP_TITLE,
  CHILD_PROP_ICON_NAME,
  CHILD_PROP_POSITION,
  CHILD_PROP_NEEDS_ATTENTION,
  LAST_CHILD_PROP
};

typedef struct _CtkStackChildInfo CtkStackChildInfo;

struct _CtkStackChildInfo {
  CtkWidget *widget;
  gchar *name;
  gchar *title;
  gchar *icon_name;
  gboolean needs_attention;
  CtkWidget *last_focus;
};

typedef struct {
  GList *children;

  CdkWindow* bin_window;
  CdkWindow* view_window;

  CtkStackChildInfo *visible_child;

  CtkCssGadget *gadget;

  gboolean hhomogeneous;
  gboolean vhomogeneous;

  CtkStackTransitionType transition_type;
  guint transition_duration;

  CtkStackChildInfo *last_visible_child;
  cairo_surface_t *last_visible_surface;
  CtkAllocation last_visible_surface_allocation;
  guint tick_id;
  CtkProgressTracker tracker;
  gboolean first_frame_skipped;

  gint last_visible_widget_width;
  gint last_visible_widget_height;

  gboolean interpolate_size;

  CtkStackTransitionType active_transition_type;

} CtkStackPrivate;

static GParamSpec *stack_props[LAST_PROP] = { NULL, };
static GParamSpec *stack_child_props[LAST_CHILD_PROP] = { NULL, };

static void     ctk_stack_add                            (CtkContainer  *widget,
                                                          CtkWidget     *child);
static void     ctk_stack_remove                         (CtkContainer  *widget,
                                                          CtkWidget     *child);
static void     ctk_stack_forall                         (CtkContainer  *container,
                                                          gboolean       include_internals,
                                                          CtkCallback    callback,
                                                          gpointer       callback_data);
static void     ctk_stack_compute_expand                 (CtkWidget     *widget,
                                                          gboolean      *hexpand,
                                                          gboolean      *vexpand);
static void     ctk_stack_size_allocate                  (CtkWidget     *widget,
                                                          CtkAllocation *allocation);
static gboolean ctk_stack_draw                           (CtkWidget     *widget,
                                                          cairo_t       *cr);
static void     ctk_stack_get_preferred_height           (CtkWidget     *widget,
                                                          gint          *minimum_height,
                                                          gint          *natural_height);
static void     ctk_stack_get_preferred_height_for_width (CtkWidget     *widget,
                                                          gint           width,
                                                          gint          *minimum_height,
                                                          gint          *natural_height);
static void     ctk_stack_get_preferred_width            (CtkWidget     *widget,
                                                          gint          *minimum_width,
                                                          gint          *natural_width);
static void     ctk_stack_get_preferred_width_for_height (CtkWidget     *widget,
                                                          gint           height,
                                                          gint          *minimum_width,
                                                          gint          *natural_width);
static void     ctk_stack_finalize                       (GObject       *obj);
static void     ctk_stack_get_property                   (GObject       *object,
                                                          guint          property_id,
                                                          GValue        *value,
                                                          GParamSpec    *pspec);
static void     ctk_stack_set_property                   (GObject       *object,
                                                          guint          property_id,
                                                          const GValue  *value,
                                                          GParamSpec    *pspec);
static void     ctk_stack_get_child_property             (CtkContainer  *container,
                                                          CtkWidget     *child,
                                                          guint          property_id,
                                                          GValue        *value,
                                                          GParamSpec    *pspec);
static void     ctk_stack_set_child_property             (CtkContainer  *container,
                                                          CtkWidget     *child,
                                                          guint          property_id,
                                                          const GValue  *value,
                                                          GParamSpec    *pspec);
static void     ctk_stack_unschedule_ticks               (CtkStack      *stack);
static gint     get_bin_window_x                         (CtkStack            *stack,
                                                          const CtkAllocation *allocation);
static gint     get_bin_window_y                         (CtkStack            *stack,
                                                          const CtkAllocation *allocation);

G_DEFINE_TYPE_WITH_PRIVATE (CtkStack, ctk_stack, CTK_TYPE_CONTAINER)

static void
ctk_stack_dispose (GObject *obj)
{
  CtkStack *stack = CTK_STACK (obj);
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);

  priv->visible_child = NULL;

  G_OBJECT_CLASS (ctk_stack_parent_class)->dispose (obj);
}

static void
ctk_stack_finalize (GObject *obj)
{
  CtkStack *stack = CTK_STACK (obj);
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);

  ctk_stack_unschedule_ticks (stack);

  if (priv->last_visible_surface != NULL)
    cairo_surface_destroy (priv->last_visible_surface);

  g_clear_object (&priv->gadget);

  G_OBJECT_CLASS (ctk_stack_parent_class)->finalize (obj);
}

static void
ctk_stack_get_property (GObject   *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  CtkStack *stack = CTK_STACK (object);

  switch (property_id)
    {
    case PROP_HOMOGENEOUS:
      g_value_set_boolean (value, ctk_stack_get_homogeneous (stack));
      break;
    case PROP_HHOMOGENEOUS:
      g_value_set_boolean (value, ctk_stack_get_hhomogeneous (stack));
      break;
    case PROP_VHOMOGENEOUS:
      g_value_set_boolean (value, ctk_stack_get_vhomogeneous (stack));
      break;
    case PROP_VISIBLE_CHILD:
      g_value_set_object (value, ctk_stack_get_visible_child (stack));
      break;
    case PROP_VISIBLE_CHILD_NAME:
      g_value_set_string (value, ctk_stack_get_visible_child_name (stack));
      break;
    case PROP_TRANSITION_DURATION:
      g_value_set_uint (value, ctk_stack_get_transition_duration (stack));
      break;
    case PROP_TRANSITION_TYPE:
      g_value_set_enum (value, ctk_stack_get_transition_type (stack));
      break;
    case PROP_TRANSITION_RUNNING:
      g_value_set_boolean (value, ctk_stack_get_transition_running (stack));
      break;
    case PROP_INTERPOLATE_SIZE:
      g_value_set_boolean (value, ctk_stack_get_interpolate_size (stack));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ctk_stack_set_property (GObject     *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  CtkStack *stack = CTK_STACK (object);

  switch (property_id)
    {
    case PROP_HOMOGENEOUS:
      ctk_stack_set_homogeneous (stack, g_value_get_boolean (value));
      break;
    case PROP_HHOMOGENEOUS:
      ctk_stack_set_hhomogeneous (stack, g_value_get_boolean (value));
      break;
    case PROP_VHOMOGENEOUS:
      ctk_stack_set_vhomogeneous (stack, g_value_get_boolean (value));
      break;
    case PROP_VISIBLE_CHILD:
      ctk_stack_set_visible_child (stack, g_value_get_object (value));
      break;
    case PROP_VISIBLE_CHILD_NAME:
      ctk_stack_set_visible_child_name (stack, g_value_get_string (value));
      break;
    case PROP_TRANSITION_DURATION:
      ctk_stack_set_transition_duration (stack, g_value_get_uint (value));
      break;
    case PROP_TRANSITION_TYPE:
      ctk_stack_set_transition_type (stack, g_value_get_enum (value));
      break;
    case PROP_INTERPOLATE_SIZE:
      ctk_stack_set_interpolate_size (stack, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ctk_stack_realize (CtkWidget *widget)
{
  CtkStack *stack = CTK_STACK (widget);
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);
  CtkAllocation allocation;
  CdkWindowAttr attributes = { 0 };
  CdkWindowAttributesType attributes_mask;
  CtkStackChildInfo *info;
  GList *l;

  ctk_widget_set_realized (widget, TRUE);
  ctk_widget_set_window (widget, g_object_ref (ctk_widget_get_parent_window (widget)));

  ctk_css_gadget_get_content_allocation (priv->gadget, &allocation, NULL);

  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.window_type = CDK_WINDOW_CHILD;
  attributes.wclass = CDK_INPUT_OUTPUT;
  attributes.visual = ctk_widget_get_visual (widget);
  attributes.event_mask =
    ctk_widget_get_events (widget);
  attributes_mask = (CDK_WA_X | CDK_WA_Y) | CDK_WA_VISUAL;

  priv->view_window =
    cdk_window_new (ctk_widget_get_window (CTK_WIDGET (stack)),
                    &attributes, attributes_mask);
  ctk_widget_register_window (widget, priv->view_window);

  attributes.x = get_bin_window_x (stack, &allocation);
  attributes.y = get_bin_window_y (stack, &allocation);
  attributes.width = allocation.width;
  attributes.height = allocation.height;

  for (l = priv->children; l != NULL; l = l->next)
    {
      info = l->data;
      attributes.event_mask |= ctk_widget_get_events (info->widget);
    }

  priv->bin_window =
    cdk_window_new (priv->view_window, &attributes, attributes_mask);
  ctk_widget_register_window (widget, priv->bin_window);

  for (l = priv->children; l != NULL; l = l->next)
    {
      info = l->data;

      ctk_widget_set_parent_window (info->widget, priv->bin_window);
    }

  cdk_window_show (priv->bin_window);
}

static void
ctk_stack_unrealize (CtkWidget *widget)
{
  CtkStack *stack = CTK_STACK (widget);
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);

  ctk_widget_unregister_window (widget, priv->bin_window);
  cdk_window_destroy (priv->bin_window);
  priv->bin_window = NULL;
  ctk_widget_unregister_window (widget, priv->view_window);
  cdk_window_destroy (priv->view_window);
  priv->view_window = NULL;

  CTK_WIDGET_CLASS (ctk_stack_parent_class)->unrealize (widget);
}

static void
ctk_stack_map (CtkWidget *widget)
{
  CtkStack *stack = CTK_STACK (widget);
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);

  CTK_WIDGET_CLASS (ctk_stack_parent_class)->map (widget);

  cdk_window_show (priv->view_window);
}

static void
ctk_stack_unmap (CtkWidget *widget)
{
  CtkStack *stack = CTK_STACK (widget);
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);

  cdk_window_hide (priv->view_window);

  CTK_WIDGET_CLASS (ctk_stack_parent_class)->unmap (widget);
}

static void
ctk_stack_class_init (CtkStackClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);
  CtkContainerClass *container_class = CTK_CONTAINER_CLASS (klass);

  object_class->get_property = ctk_stack_get_property;
  object_class->set_property = ctk_stack_set_property;
  object_class->dispose = ctk_stack_dispose;
  object_class->finalize = ctk_stack_finalize;

  widget_class->size_allocate = ctk_stack_size_allocate;
  widget_class->draw = ctk_stack_draw;
  widget_class->realize = ctk_stack_realize;
  widget_class->unrealize = ctk_stack_unrealize;
  widget_class->map = ctk_stack_map;
  widget_class->unmap = ctk_stack_unmap;
  widget_class->get_preferred_height = ctk_stack_get_preferred_height;
  widget_class->get_preferred_height_for_width = ctk_stack_get_preferred_height_for_width;
  widget_class->get_preferred_width = ctk_stack_get_preferred_width;
  widget_class->get_preferred_width_for_height = ctk_stack_get_preferred_width_for_height;
  widget_class->compute_expand = ctk_stack_compute_expand;

  container_class->add = ctk_stack_add;
  container_class->remove = ctk_stack_remove;
  container_class->forall = ctk_stack_forall;
  container_class->set_child_property = ctk_stack_set_child_property;
  container_class->get_child_property = ctk_stack_get_child_property;
  ctk_container_class_handle_border_width (container_class);

  stack_props[PROP_HOMOGENEOUS] =
      g_param_spec_boolean ("homogeneous", P_("Homogeneous"), P_("Homogeneous sizing"),
                            TRUE,
                            CTK_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkStack:hhomogeneous:
   *
   * %TRUE if the stack allocates the same width for all children.
   *
   * Since: 3.16
   */
  stack_props[PROP_HHOMOGENEOUS] =
      g_param_spec_boolean ("hhomogeneous", P_("Horizontally homogeneous"), P_("Horizontally homogeneous sizing"),
                            TRUE,
                            CTK_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkStack:vhomogeneous:
   *
   * %TRUE if the stack allocates the same height for all children.
   *
   * Since: 3.16
   */
  stack_props[PROP_VHOMOGENEOUS] =
      g_param_spec_boolean ("vhomogeneous", P_("Vertically homogeneous"), P_("Vertically homogeneous sizing"),
                            TRUE,
                            CTK_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);
  stack_props[PROP_VISIBLE_CHILD] =
      g_param_spec_object ("visible-child", P_("Visible child"), P_("The widget currently visible in the stack"),
                           CTK_TYPE_WIDGET,
                           CTK_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);
  stack_props[PROP_VISIBLE_CHILD_NAME] =
      g_param_spec_string ("visible-child-name", P_("Name of visible child"), P_("The name of the widget currently visible in the stack"),
                           NULL,
                           CTK_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);
  stack_props[PROP_TRANSITION_DURATION] =
      g_param_spec_uint ("transition-duration", P_("Transition duration"), P_("The animation duration, in milliseconds"),
                         0, G_MAXUINT, 200,
                         CTK_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);
  stack_props[PROP_TRANSITION_TYPE] =
      g_param_spec_enum ("transition-type", P_("Transition type"), P_("The type of animation used to transition"),
                         CTK_TYPE_STACK_TRANSITION_TYPE, CTK_STACK_TRANSITION_TYPE_NONE,
                         CTK_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);
  stack_props[PROP_TRANSITION_RUNNING] =
      g_param_spec_boolean ("transition-running", P_("Transition running"), P_("Whether or not the transition is currently running"),
                            FALSE,
                            CTK_PARAM_READABLE);
  stack_props[PROP_INTERPOLATE_SIZE] =
      g_param_spec_boolean ("interpolate-size", P_("Interpolate size"), P_("Whether or not the size should smoothly change when changing between differently sized children"),
                            FALSE,
                            CTK_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);


  g_object_class_install_properties (object_class, LAST_PROP, stack_props);

  stack_child_props[CHILD_PROP_NAME] =
    g_param_spec_string ("name",
                         P_("Name"),
                         P_("The name of the child page"),
                         NULL,
                         CTK_PARAM_READWRITE);

  stack_child_props[CHILD_PROP_TITLE] =
    g_param_spec_string ("title",
                         P_("Title"),
                         P_("The title of the child page"),
                         NULL,
                         CTK_PARAM_READWRITE);

  stack_child_props[CHILD_PROP_ICON_NAME] =
    g_param_spec_string ("icon-name",
                         P_("Icon name"),
                         P_("The icon name of the child page"),
                         NULL,
                         CTK_PARAM_READWRITE);

  stack_child_props[CHILD_PROP_POSITION] =
    g_param_spec_int ("position",
                      P_("Position"),
                      P_("The index of the child in the parent"),
                      -1, G_MAXINT,
                      0,
                      CTK_PARAM_READWRITE);

  /**
   * CtkStack:needs-attention:
   *
   * Sets a flag specifying whether the child requires the user attention.
   * This is used by the #CtkStackSwitcher to change the appearance of the
   * corresponding button when a page needs attention and it is not the
   * current one.
   *
   * Since: 3.12
   */
  stack_child_props[CHILD_PROP_NEEDS_ATTENTION] =
    g_param_spec_boolean ("needs-attention",
                         P_("Needs Attention"),
                         P_("Whether this page needs attention"),
                         FALSE,
                         CTK_PARAM_READWRITE);

  ctk_container_class_install_child_properties (container_class, LAST_CHILD_PROP, stack_child_props);

  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_STACK_ACCESSIBLE);
  ctk_widget_class_set_css_name (widget_class, "stack");
}

/**
 * ctk_stack_new:
 *
 * Creates a new #CtkStack container.
 *
 * Returns: a new #CtkStack
 *
 * Since: 3.10
 */
CtkWidget *
ctk_stack_new (void)
{
  return g_object_new (CTK_TYPE_STACK, NULL);
}

static CtkStackChildInfo *
find_child_info_for_widget (CtkStack  *stack,
                            CtkWidget *child)
{
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);
  CtkStackChildInfo *info;
  GList *l;

  for (l = priv->children; l != NULL; l = l->next)
    {
      info = l->data;
      if (info->widget == child)
        return info;
    }

  return NULL;
}

static void
reorder_child (CtkStack  *stack,
               CtkWidget *child,
               gint       position)
{
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);
  GList *l;
  GList *old_link = NULL;
  GList *new_link = NULL;
  CtkStackChildInfo *child_info = NULL;
  gint num = 0;

  l = priv->children;

  /* Loop to find the old position and link of child, new link of child and
   * total number of children. new_link will be NULL if the child should be
   * moved to the end (in case of position being < 0 || >= num)
   */
  while (l && (new_link == NULL || old_link == NULL))
    {
      /* Record the new position if found */
      if (position == num)
        new_link = l;

      if (old_link == NULL)
        {
          CtkStackChildInfo *info;
          info = l->data;

          /* Keep trying to find the current position and link location of the child */
          if (info->widget == child)
            {
              old_link = l;
              child_info = info;
            }
        }

      l = l->next;
      num++;
    }

  g_return_if_fail (old_link != NULL);

  if (old_link == new_link || (old_link->next == NULL && new_link == NULL))
    return;

  priv->children = g_list_delete_link (priv->children, old_link);
  priv->children = g_list_insert_before (priv->children, new_link, child_info);

  ctk_container_child_notify_by_pspec (CTK_CONTAINER (stack), child, stack_child_props[CHILD_PROP_POSITION]);
}

static void
ctk_stack_get_child_property (CtkContainer *container,
                              CtkWidget    *child,
                              guint         property_id,
                              GValue       *value,
                              GParamSpec   *pspec)
{
  CtkStack *stack = CTK_STACK (container);
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);
  CtkStackChildInfo *info;

  info = find_child_info_for_widget (stack, child);
  if (info == NULL)
    {
      CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      return;
    }

  switch (property_id)
    {
    case CHILD_PROP_NAME:
      g_value_set_string (value, info->name);
      break;

    case CHILD_PROP_TITLE:
      g_value_set_string (value, info->title);
      break;

    case CHILD_PROP_ICON_NAME:
      g_value_set_string (value, info->icon_name);
      break;

    case CHILD_PROP_POSITION:
      g_value_set_int (value, g_list_index (priv->children, info));
      break;

    case CHILD_PROP_NEEDS_ATTENTION:
      g_value_set_boolean (value, info->needs_attention);
      break;

    default:
      CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
ctk_stack_set_child_property (CtkContainer *container,
                              CtkWidget    *child,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  CtkStack *stack = CTK_STACK (container);
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);
  CtkStackChildInfo *info;
  CtkStackChildInfo *info2;
  gchar *name;
  GList *l;

  info = find_child_info_for_widget (stack, child);
  if (info == NULL)
    {
      CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      return;
    }

  switch (property_id)
    {
    case CHILD_PROP_NAME:
      name = g_value_dup_string (value);
      for (l = priv->children; l != NULL; l = l->next)
        {
          info2 = l->data;
          if (info == info2)
            continue;
          if (g_strcmp0 (info2->name, name) == 0)
            {
              g_warning ("Duplicate child name in CtkStack: %s", name);
              break;
            }
        }

      g_free (info->name);
      info->name = name;

      ctk_container_child_notify_by_pspec (container, child, pspec);

      if (priv->visible_child == info)
        g_object_notify_by_pspec (G_OBJECT (stack),
                                  stack_props[PROP_VISIBLE_CHILD_NAME]);

      break;

    case CHILD_PROP_TITLE:
      g_free (info->title);
      info->title = g_value_dup_string (value);
      ctk_container_child_notify_by_pspec (container, child, pspec);
      break;

    case CHILD_PROP_ICON_NAME:
      g_free (info->icon_name);
      info->icon_name = g_value_dup_string (value);
      ctk_container_child_notify_by_pspec (container, child, pspec);
      break;

    case CHILD_PROP_POSITION:
      reorder_child (stack, child, g_value_get_int (value));
      break;

    case CHILD_PROP_NEEDS_ATTENTION:
      info->needs_attention = g_value_get_boolean (value);
      ctk_container_child_notify_by_pspec (container, child, pspec);
      break;

    default:
      CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static inline gboolean
is_left_transition (CtkStackTransitionType transition_type)
{
  return (transition_type == CTK_STACK_TRANSITION_TYPE_SLIDE_LEFT ||
          transition_type == CTK_STACK_TRANSITION_TYPE_OVER_LEFT);
}

static inline gboolean
is_right_transition (CtkStackTransitionType transition_type)
{
  return (transition_type == CTK_STACK_TRANSITION_TYPE_SLIDE_RIGHT ||
          transition_type == CTK_STACK_TRANSITION_TYPE_OVER_RIGHT);
}

static inline gboolean
is_up_transition (CtkStackTransitionType transition_type)
{
  return (transition_type == CTK_STACK_TRANSITION_TYPE_SLIDE_UP ||
          transition_type == CTK_STACK_TRANSITION_TYPE_OVER_UP);
}

static inline gboolean
is_down_transition (CtkStackTransitionType transition_type)
{
  return (transition_type == CTK_STACK_TRANSITION_TYPE_SLIDE_DOWN ||
          transition_type == CTK_STACK_TRANSITION_TYPE_OVER_DOWN);
}

/* Transitions that cause the bin window to move */
static inline gboolean
is_window_moving_transition (CtkStackTransitionType transition_type)
{
  return (transition_type == CTK_STACK_TRANSITION_TYPE_SLIDE_LEFT ||
          transition_type == CTK_STACK_TRANSITION_TYPE_SLIDE_RIGHT ||
          transition_type == CTK_STACK_TRANSITION_TYPE_SLIDE_UP ||
          transition_type == CTK_STACK_TRANSITION_TYPE_SLIDE_DOWN ||
          transition_type == CTK_STACK_TRANSITION_TYPE_OVER_UP ||
          transition_type == CTK_STACK_TRANSITION_TYPE_OVER_DOWN ||
          transition_type == CTK_STACK_TRANSITION_TYPE_OVER_LEFT ||
          transition_type == CTK_STACK_TRANSITION_TYPE_OVER_RIGHT);
}

/* Transitions that change direction depending on the relative order of the
old and new child */
static inline gboolean
is_direction_dependent_transition (CtkStackTransitionType transition_type)
{
  return (transition_type == CTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT ||
          transition_type == CTK_STACK_TRANSITION_TYPE_SLIDE_UP_DOWN ||
          transition_type == CTK_STACK_TRANSITION_TYPE_OVER_UP_DOWN ||
          transition_type == CTK_STACK_TRANSITION_TYPE_OVER_DOWN_UP ||
          transition_type == CTK_STACK_TRANSITION_TYPE_OVER_LEFT_RIGHT ||
          transition_type == CTK_STACK_TRANSITION_TYPE_OVER_RIGHT_LEFT);
}

/* Returns simple transition type for a direction dependent transition, given
whether the new child (the one being switched to) is first in the stacking order
(added earlier). */
static inline CtkStackTransitionType
get_simple_transition_type (gboolean               new_child_first,
                            CtkStackTransitionType transition_type)
{
  switch (transition_type)
    {
    case CTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT:
      return new_child_first ? CTK_STACK_TRANSITION_TYPE_SLIDE_RIGHT : CTK_STACK_TRANSITION_TYPE_SLIDE_LEFT;
    case CTK_STACK_TRANSITION_TYPE_SLIDE_UP_DOWN:
      return new_child_first ? CTK_STACK_TRANSITION_TYPE_SLIDE_DOWN : CTK_STACK_TRANSITION_TYPE_SLIDE_UP;
    case CTK_STACK_TRANSITION_TYPE_OVER_UP_DOWN:
      return new_child_first ? CTK_STACK_TRANSITION_TYPE_UNDER_DOWN : CTK_STACK_TRANSITION_TYPE_OVER_UP;
    case CTK_STACK_TRANSITION_TYPE_OVER_DOWN_UP:
      return new_child_first ? CTK_STACK_TRANSITION_TYPE_UNDER_UP : CTK_STACK_TRANSITION_TYPE_OVER_DOWN;
    case CTK_STACK_TRANSITION_TYPE_OVER_LEFT_RIGHT:
      return new_child_first ? CTK_STACK_TRANSITION_TYPE_UNDER_RIGHT : CTK_STACK_TRANSITION_TYPE_OVER_LEFT;
    case CTK_STACK_TRANSITION_TYPE_OVER_RIGHT_LEFT:
      return new_child_first ? CTK_STACK_TRANSITION_TYPE_UNDER_LEFT : CTK_STACK_TRANSITION_TYPE_OVER_RIGHT;
    default: ;
    }
  return transition_type;
}

static gint
get_bin_window_x (CtkStack            *stack,
                  const CtkAllocation *allocation)
{
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);
  int x = 0;

  if (ctk_progress_tracker_get_state (&priv->tracker) != CTK_PROGRESS_STATE_AFTER)
    {
      if (is_left_transition (priv->active_transition_type))
        x = allocation->width * (1 - ctk_progress_tracker_get_ease_out_cubic (&priv->tracker, FALSE));
      if (is_right_transition (priv->active_transition_type))
        x = -allocation->width * (1 - ctk_progress_tracker_get_ease_out_cubic (&priv->tracker, FALSE));
    }

  return x;
}

static gint
get_bin_window_y (CtkStack            *stack,
                  const CtkAllocation *allocation)
{
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);
  int y = 0;

  if (ctk_progress_tracker_get_state (&priv->tracker) != CTK_PROGRESS_STATE_AFTER)
    {
      if (is_up_transition (priv->active_transition_type))
        y = allocation->height * (1 - ctk_progress_tracker_get_ease_out_cubic (&priv->tracker, FALSE));
      if (is_down_transition(priv->active_transition_type))
        y = -allocation->height * (1 - ctk_progress_tracker_get_ease_out_cubic (&priv->tracker, FALSE));
    }

  return y;
}

static void
ctk_stack_progress_updated (CtkStack *stack)
{
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);

  ctk_widget_queue_draw (CTK_WIDGET (stack));

  if (!priv->vhomogeneous || !priv->hhomogeneous)
    ctk_widget_queue_resize (CTK_WIDGET (stack));

  if (priv->bin_window != NULL &&
      is_window_moving_transition (priv->active_transition_type))
    {
      CtkAllocation allocation;
      ctk_widget_get_allocation (CTK_WIDGET (stack), &allocation);
      cdk_window_move (priv->bin_window,
                       get_bin_window_x (stack, &allocation), get_bin_window_y (stack, &allocation));
    }

  if (ctk_progress_tracker_get_state (&priv->tracker) == CTK_PROGRESS_STATE_AFTER)
    {
      if (priv->last_visible_surface != NULL)
        {
          cairo_surface_destroy (priv->last_visible_surface);
          priv->last_visible_surface = NULL;
        }

      if (priv->last_visible_child != NULL)
        {
          ctk_widget_set_child_visible (priv->last_visible_child->widget, FALSE);
          priv->last_visible_child = NULL;
        }
    }
}

static gboolean
ctk_stack_transition_cb (CtkWidget     *widget,
                         CdkFrameClock *frame_clock,
                         gpointer       user_data G_GNUC_UNUSED)
{
  CtkStack *stack = CTK_STACK (widget);
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);

  if (priv->first_frame_skipped)
    ctk_progress_tracker_advance_frame (&priv->tracker,
                                        cdk_frame_clock_get_frame_time (frame_clock));
  else
    priv->first_frame_skipped = TRUE;

  /* Finish animation early if not mapped anymore */
  if (!ctk_widget_get_mapped (widget))
    ctk_progress_tracker_finish (&priv->tracker);

  ctk_stack_progress_updated (CTK_STACK (widget));

  if (ctk_progress_tracker_get_state (&priv->tracker) == CTK_PROGRESS_STATE_AFTER)
    {
      priv->tick_id = 0;
      g_object_notify_by_pspec (G_OBJECT (stack), stack_props[PROP_TRANSITION_RUNNING]);

      return FALSE;
    }

  return TRUE;
}

static void
ctk_stack_schedule_ticks (CtkStack *stack)
{
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);

  if (priv->tick_id == 0)
    {
      priv->tick_id =
        ctk_widget_add_tick_callback (CTK_WIDGET (stack), ctk_stack_transition_cb, stack, NULL);
      g_object_notify_by_pspec (G_OBJECT (stack), stack_props[PROP_TRANSITION_RUNNING]);
    }
}

static void
ctk_stack_unschedule_ticks (CtkStack *stack)
{
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);

  if (priv->tick_id != 0)
    {
      ctk_widget_remove_tick_callback (CTK_WIDGET (stack), priv->tick_id);
      priv->tick_id = 0;
      g_object_notify_by_pspec (G_OBJECT (stack), stack_props[PROP_TRANSITION_RUNNING]);
    }
}

static CtkStackTransitionType
effective_transition_type (CtkStack               *stack,
                           CtkStackTransitionType  transition_type)
{
  if (ctk_widget_get_direction (CTK_WIDGET (stack)) == CTK_TEXT_DIR_RTL)
    {
      switch (transition_type)
        {
        case CTK_STACK_TRANSITION_TYPE_SLIDE_LEFT:
          return CTK_STACK_TRANSITION_TYPE_SLIDE_RIGHT;
        case CTK_STACK_TRANSITION_TYPE_SLIDE_RIGHT:
          return CTK_STACK_TRANSITION_TYPE_SLIDE_LEFT;
        case CTK_STACK_TRANSITION_TYPE_OVER_LEFT:
          return CTK_STACK_TRANSITION_TYPE_OVER_RIGHT;
        case CTK_STACK_TRANSITION_TYPE_OVER_RIGHT:
          return CTK_STACK_TRANSITION_TYPE_OVER_LEFT;
        case CTK_STACK_TRANSITION_TYPE_UNDER_LEFT:
          return CTK_STACK_TRANSITION_TYPE_UNDER_RIGHT;
        case CTK_STACK_TRANSITION_TYPE_UNDER_RIGHT:
          return CTK_STACK_TRANSITION_TYPE_UNDER_LEFT;
        default: ;
        }
    }

  return transition_type;
}

static void
ctk_stack_start_transition (CtkStack               *stack,
                            CtkStackTransitionType  transition_type,
                            guint                   transition_duration)
{
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);
  CtkWidget *widget = CTK_WIDGET (stack);

  if (ctk_widget_get_mapped (widget) &&
      ctk_settings_get_enable_animations (ctk_widget_get_settings (widget)) &&
      transition_type != CTK_STACK_TRANSITION_TYPE_NONE &&
      transition_duration != 0 &&
      priv->last_visible_child != NULL)
    {
      priv->active_transition_type = effective_transition_type (stack, transition_type);
      priv->first_frame_skipped = FALSE;
      ctk_stack_schedule_ticks (stack);
      ctk_progress_tracker_start (&priv->tracker,
                                  priv->transition_duration * 1000,
                                  0,
                                  1.0);
    }
  else
    {
      ctk_stack_unschedule_ticks (stack);
      priv->active_transition_type = CTK_STACK_TRANSITION_TYPE_NONE;
      ctk_progress_tracker_finish (&priv->tracker);
    }

  ctk_stack_progress_updated (CTK_STACK (widget));
}

static void
set_visible_child (CtkStack               *stack,
                   CtkStackChildInfo      *child_info,
                   CtkStackTransitionType  transition_type,
                   guint                   transition_duration)
{
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);
  CtkStackChildInfo *info;
  CtkWidget *widget = CTK_WIDGET (stack);
  GList *l;
  CtkWidget *toplevel;
  CtkWidget *focus;
  gboolean contains_focus = FALSE;

  /* if we are being destroyed, do not bother with transitions
   * and notifications
   */
  if (ctk_widget_in_destruction (widget))
    return;

  /* If none, pick first visible */
  if (child_info == NULL)
    {
      for (l = priv->children; l != NULL; l = l->next)
        {
          info = l->data;
          if (ctk_widget_get_visible (info->widget))
            {
              child_info = info;
              break;
            }
        }
    }

  if (child_info == priv->visible_child)
    return;

  toplevel = ctk_widget_get_toplevel (widget);
  if (CTK_IS_WINDOW (toplevel))
    {
      focus = ctk_window_get_focus (CTK_WINDOW (toplevel));
      if (focus &&
          priv->visible_child &&
          priv->visible_child->widget &&
          ctk_widget_is_ancestor (focus, priv->visible_child->widget))
        {
          contains_focus = TRUE;

          if (priv->visible_child->last_focus)
            g_object_remove_weak_pointer (G_OBJECT (priv->visible_child->last_focus),
                                          (gpointer *)&priv->visible_child->last_focus);
          priv->visible_child->last_focus = focus;
          g_object_add_weak_pointer (G_OBJECT (priv->visible_child->last_focus),
                                     (gpointer *)&priv->visible_child->last_focus);
        }
    }

  if (priv->last_visible_child)
    ctk_widget_set_child_visible (priv->last_visible_child->widget, FALSE);
  priv->last_visible_child = NULL;

  if (priv->last_visible_surface != NULL)
    cairo_surface_destroy (priv->last_visible_surface);
  priv->last_visible_surface = NULL;

  if (priv->visible_child && priv->visible_child->widget)
    {
      if (ctk_widget_is_visible (widget))
        {
          CtkAllocation allocation;

          priv->last_visible_child = priv->visible_child;
          ctk_widget_get_allocated_size (priv->last_visible_child->widget, &allocation, NULL);
          priv->last_visible_widget_width = allocation.width;
          priv->last_visible_widget_height = allocation.height;
        }
      else
        {
          ctk_widget_set_child_visible (priv->visible_child->widget, FALSE);
        }
    }

  ctk_stack_accessible_update_visible_child (stack,
                                             priv->visible_child ? priv->visible_child->widget : NULL,
                                             child_info ? child_info->widget : NULL);

  priv->visible_child = child_info;

  if (child_info)
    {
      ctk_widget_set_child_visible (child_info->widget, TRUE);

      if (contains_focus)
        {
          if (child_info->last_focus)
            ctk_widget_grab_focus (child_info->last_focus);
          else
            ctk_widget_child_focus (child_info->widget, CTK_DIR_TAB_FORWARD);
        }
    }

  if ((child_info == NULL || priv->last_visible_child == NULL) &&
      is_direction_dependent_transition (transition_type))
    {
      transition_type = CTK_STACK_TRANSITION_TYPE_NONE;
    }
  else if (is_direction_dependent_transition (transition_type))
    {
      gboolean i_first = FALSE;
      for (l = priv->children; l != NULL; l = l->next)
        {
	  if (child_info == l->data)
	    {
	      i_first = TRUE;
	      break;
	    }
	  if (priv->last_visible_child == l->data)
	    break;
        }

      transition_type = get_simple_transition_type (i_first, transition_type);
    }

  if (priv->hhomogeneous && priv->vhomogeneous)
    ctk_widget_queue_allocate (widget);
  else
    ctk_widget_queue_resize (widget);

  g_object_notify_by_pspec (G_OBJECT (stack), stack_props[PROP_VISIBLE_CHILD]);
  g_object_notify_by_pspec (G_OBJECT (stack),
                            stack_props[PROP_VISIBLE_CHILD_NAME]);

  ctk_stack_start_transition (stack, transition_type, transition_duration);
}

static void
stack_child_visibility_notify_cb (GObject    *obj,
                                  GParamSpec *pspec G_GNUC_UNUSED,
                                  gpointer    user_data)
{
  CtkStack *stack = CTK_STACK (user_data);
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);
  CtkWidget *child = CTK_WIDGET (obj);
  CtkStackChildInfo *child_info;

  child_info = find_child_info_for_widget (stack, child);

  if (priv->visible_child == NULL &&
      ctk_widget_get_visible (child))
    set_visible_child (stack, child_info, priv->transition_type, priv->transition_duration);
  else if (priv->visible_child == child_info &&
           !ctk_widget_get_visible (child))
    set_visible_child (stack, NULL, priv->transition_type, priv->transition_duration);

  if (child_info == priv->last_visible_child)
    {
      ctk_widget_set_child_visible (priv->last_visible_child->widget, FALSE);
      priv->last_visible_child = NULL;
    }
}

/**
 * ctk_stack_add_titled:
 * @stack: a #CtkStack
 * @child: the widget to add
 * @name: the name for @child
 * @title: a human-readable title for @child
 *
 * Adds a child to @stack.
 * The child is identified by the @name. The @title
 * will be used by #CtkStackSwitcher to represent
 * @child in a tab bar, so it should be short.
 *
 * Since: 3.10
 */
void
ctk_stack_add_titled (CtkStack   *stack,
                     CtkWidget   *child,
                     const gchar *name,
                     const gchar *title)
{
  g_return_if_fail (CTK_IS_STACK (stack));
  g_return_if_fail (CTK_IS_WIDGET (child));

  ctk_container_add_with_properties (CTK_CONTAINER (stack),
                                     child,
                                     "name", name,
                                     "title", title,
                                     NULL);
}

/**
 * ctk_stack_add_named:
 * @stack: a #CtkStack
 * @child: the widget to add
 * @name: the name for @child
 *
 * Adds a child to @stack.
 * The child is identified by the @name.
 *
 * Since: 3.10
 */
void
ctk_stack_add_named (CtkStack   *stack,
                    CtkWidget   *child,
                    const gchar *name)
{
  g_return_if_fail (CTK_IS_STACK (stack));
  g_return_if_fail (CTK_IS_WIDGET (child));

  ctk_container_add_with_properties (CTK_CONTAINER (stack),
                                     child,
                                     "name", name,
                                     NULL);
}

static void
ctk_stack_add (CtkContainer *container,
               CtkWidget    *child)
{
  CtkStack *stack = CTK_STACK (container);
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);
  CtkStackChildInfo *child_info;

  g_return_if_fail (child != NULL);

  child_info = g_slice_new (CtkStackChildInfo);
  child_info->widget = child;
  child_info->name = NULL;
  child_info->title = NULL;
  child_info->icon_name = NULL;
  child_info->needs_attention = FALSE;
  child_info->last_focus = NULL;

  priv->children = g_list_append (priv->children, child_info);

  ctk_widget_set_child_visible (child, FALSE);
  ctk_widget_set_parent_window (child, priv->bin_window);
  ctk_widget_set_parent (child, CTK_WIDGET (stack));

  if (priv->bin_window)
    cdk_window_set_events (priv->bin_window,
                           cdk_window_get_events (priv->bin_window) |
                           ctk_widget_get_events (child));

  g_signal_connect (child, "notify::visible",
                    G_CALLBACK (stack_child_visibility_notify_cb), stack);

  ctk_container_child_notify_by_pspec (container, child, stack_child_props[CHILD_PROP_POSITION]);

  if (priv->visible_child == NULL &&
      ctk_widget_get_visible (child))
    set_visible_child (stack, child_info, priv->transition_type, priv->transition_duration);

  if (priv->hhomogeneous || priv->vhomogeneous || priv->visible_child == child_info)
    ctk_widget_queue_resize (CTK_WIDGET (stack));
}

static void
ctk_stack_remove (CtkContainer *container,
                  CtkWidget    *child)
{
  CtkStack *stack = CTK_STACK (container);
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);
  CtkStackChildInfo *child_info;
  gboolean was_visible;

  child_info = find_child_info_for_widget (stack, child);
  if (child_info == NULL)
    return;

  priv->children = g_list_remove (priv->children, child_info);

  g_signal_handlers_disconnect_by_func (child,
                                        stack_child_visibility_notify_cb,
                                        stack);

  was_visible = ctk_widget_get_visible (child);

  child_info->widget = NULL;

  if (priv->visible_child == child_info)
    set_visible_child (stack, NULL, priv->transition_type, priv->transition_duration);

  if (priv->last_visible_child == child_info)
    priv->last_visible_child = NULL;

  ctk_widget_unparent (child);

  g_free (child_info->name);
  g_free (child_info->title);
  g_free (child_info->icon_name);

  if (child_info->last_focus)
    g_object_remove_weak_pointer (G_OBJECT (child_info->last_focus),
                                  (gpointer *)&child_info->last_focus);

  g_slice_free (CtkStackChildInfo, child_info);

  if ((priv->hhomogeneous || priv->vhomogeneous) && was_visible)
    ctk_widget_queue_resize (CTK_WIDGET (stack));
}

/**
 * ctk_stack_get_child_by_name:
 * @stack: a #CtkStack
 * @name: the name of the child to find
 *
 * Finds the child of the #CtkStack with the name given as
 * the argument. Returns %NULL if there is no child with this
 * name.
 *
 * Returns: (transfer none) (nullable): the requested child of the #CtkStack
 *
 * Since: 3.12
 */
CtkWidget *
ctk_stack_get_child_by_name (CtkStack    *stack,
                             const gchar *name)
{
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);
  CtkStackChildInfo *info;
  GList *l;

  g_return_val_if_fail (CTK_IS_STACK (stack), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  for (l = priv->children; l != NULL; l = l->next)
    {
      info = l->data;
      if (info->name && strcmp (info->name, name) == 0)
        return info->widget;
    }

  return NULL;
}

/**
 * ctk_stack_set_homogeneous:
 * @stack: a #CtkStack
 * @homogeneous: %TRUE to make @stack homogeneous
 *
 * Sets the #CtkStack to be homogeneous or not. If it
 * is homogeneous, the #CtkStack will request the same
 * size for all its children. If it isn't, the stack
 * may change size when a different child becomes visible.
 *
 * Since 3.16, homogeneity can be controlled separately
 * for horizontal and vertical size, with the
 * #CtkStack:hhomogeneous and #CtkStack:vhomogeneous.
 *
 * Since: 3.10
 */
void
ctk_stack_set_homogeneous (CtkStack *stack,
                           gboolean  homogeneous)
{
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);

  g_return_if_fail (CTK_IS_STACK (stack));

  homogeneous = !!homogeneous;

  if ((priv->hhomogeneous && priv->vhomogeneous) == homogeneous)
    return;

  g_object_freeze_notify (G_OBJECT (stack));

  if (priv->hhomogeneous != homogeneous)
    {
      priv->hhomogeneous = homogeneous;
      g_object_notify_by_pspec (G_OBJECT (stack), stack_props[PROP_HHOMOGENEOUS]);
    }

  if (priv->vhomogeneous != homogeneous)
    {
      priv->vhomogeneous = homogeneous;
      g_object_notify_by_pspec (G_OBJECT (stack), stack_props[PROP_VHOMOGENEOUS]);
    }

  if (ctk_widget_get_visible (CTK_WIDGET(stack)))
    ctk_widget_queue_resize (CTK_WIDGET (stack));

  g_object_notify_by_pspec (G_OBJECT (stack), stack_props[PROP_HOMOGENEOUS]);
  g_object_thaw_notify (G_OBJECT (stack));
}

/**
 * ctk_stack_get_homogeneous:
 * @stack: a #CtkStack
 *
 * Gets whether @stack is homogeneous.
 * See ctk_stack_set_homogeneous().
 *
 * Returns: whether @stack is homogeneous.
 *
 * Since: 3.10
 */
gboolean
ctk_stack_get_homogeneous (CtkStack *stack)
{
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);

  g_return_val_if_fail (CTK_IS_STACK (stack), FALSE);

  return priv->hhomogeneous && priv->vhomogeneous;
}

/**
 * ctk_stack_set_hhomogeneous:
 * @stack: a #CtkStack
 * @hhomogeneous: %TRUE to make @stack horizontally homogeneous
 *
 * Sets the #CtkStack to be horizontally homogeneous or not.
 * If it is homogeneous, the #CtkStack will request the same
 * width for all its children. If it isn't, the stack
 * may change width when a different child becomes visible.
 *
 * Since: 3.16
 */
void
ctk_stack_set_hhomogeneous (CtkStack *stack,
                            gboolean  hhomogeneous)
{
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);

  g_return_if_fail (CTK_IS_STACK (stack));

  hhomogeneous = !!hhomogeneous;

  if (priv->hhomogeneous == hhomogeneous)
    return;

  priv->hhomogeneous = hhomogeneous;

  if (ctk_widget_get_visible (CTK_WIDGET(stack)))
    ctk_widget_queue_resize (CTK_WIDGET (stack));

  g_object_notify_by_pspec (G_OBJECT (stack), stack_props[PROP_HHOMOGENEOUS]);
}

/**
 * ctk_stack_get_hhomogeneous:
 * @stack: a #CtkStack
 *
 * Gets whether @stack is horizontally homogeneous.
 * See ctk_stack_set_hhomogeneous().
 *
 * Returns: whether @stack is horizontally homogeneous.
 *
 * Since: 3.16
 */
gboolean
ctk_stack_get_hhomogeneous (CtkStack *stack)
{
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);

  g_return_val_if_fail (CTK_IS_STACK (stack), FALSE);

  return priv->hhomogeneous;
}

/**
 * ctk_stack_set_vhomogeneous:
 * @stack: a #CtkStack
 * @vhomogeneous: %TRUE to make @stack vertically homogeneous
 *
 * Sets the #CtkStack to be vertically homogeneous or not.
 * If it is homogeneous, the #CtkStack will request the same
 * height for all its children. If it isn't, the stack
 * may change height when a different child becomes visible.
 *
 * Since: 3.16
 */
void
ctk_stack_set_vhomogeneous (CtkStack *stack,
                            gboolean  vhomogeneous)
{
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);

  g_return_if_fail (CTK_IS_STACK (stack));

  vhomogeneous = !!vhomogeneous;

  if (priv->vhomogeneous == vhomogeneous)
    return;

  priv->vhomogeneous = vhomogeneous;

  if (ctk_widget_get_visible (CTK_WIDGET(stack)))
    ctk_widget_queue_resize (CTK_WIDGET (stack));

  g_object_notify_by_pspec (G_OBJECT (stack), stack_props[PROP_VHOMOGENEOUS]);
}

/**
 * ctk_stack_get_vhomogeneous:
 * @stack: a #CtkStack
 *
 * Gets whether @stack is vertically homogeneous.
 * See ctk_stack_set_vhomogeneous().
 *
 * Returns: whether @stack is vertically homogeneous.
 *
 * Since: 3.16
 */
gboolean
ctk_stack_get_vhomogeneous (CtkStack *stack)
{
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);

  g_return_val_if_fail (CTK_IS_STACK (stack), FALSE);

  return priv->vhomogeneous;
}

/**
 * ctk_stack_get_transition_duration:
 * @stack: a #CtkStack
 *
 * Returns the amount of time (in milliseconds) that
 * transitions between pages in @stack will take.
 *
 * Returns: the transition duration
 *
 * Since: 3.10
 */
guint
ctk_stack_get_transition_duration (CtkStack *stack)
{
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);

  g_return_val_if_fail (CTK_IS_STACK (stack), 0);

  return priv->transition_duration;
}

/**
 * ctk_stack_set_transition_duration:
 * @stack: a #CtkStack
 * @duration: the new duration, in milliseconds
 *
 * Sets the duration that transitions between pages in @stack
 * will take.
 *
 * Since: 3.10
 */
void
ctk_stack_set_transition_duration (CtkStack *stack,
                                   guint     duration)
{
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);

  g_return_if_fail (CTK_IS_STACK (stack));

  if (priv->transition_duration == duration)
    return;

  priv->transition_duration = duration;
  g_object_notify_by_pspec (G_OBJECT (stack),
                            stack_props[PROP_TRANSITION_DURATION]);
}

/**
 * ctk_stack_get_transition_type:
 * @stack: a #CtkStack
 *
 * Gets the type of animation that will be used
 * for transitions between pages in @stack.
 *
 * Returns: the current transition type of @stack
 *
 * Since: 3.10
 */
CtkStackTransitionType
ctk_stack_get_transition_type (CtkStack *stack)
{
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);

  g_return_val_if_fail (CTK_IS_STACK (stack), CTK_STACK_TRANSITION_TYPE_NONE);

  return priv->transition_type;
}

/**
 * ctk_stack_set_transition_type:
 * @stack: a #CtkStack
 * @transition: the new transition type
 *
 * Sets the type of animation that will be used for
 * transitions between pages in @stack. Available
 * types include various kinds of fades and slides.
 *
 * The transition type can be changed without problems
 * at runtime, so it is possible to change the animation
 * based on the page that is about to become current.
 *
 * Since: 3.10
 */
void
ctk_stack_set_transition_type (CtkStack              *stack,
                              CtkStackTransitionType  transition)
{
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);

  g_return_if_fail (CTK_IS_STACK (stack));

  if (priv->transition_type == transition)
    return;

  priv->transition_type = transition;
  g_object_notify_by_pspec (G_OBJECT (stack),
                            stack_props[PROP_TRANSITION_TYPE]);
}

/**
 * ctk_stack_get_transition_running:
 * @stack: a #CtkStack
 *
 * Returns whether the @stack is currently in a transition from one page to
 * another.
 *
 * Returns: %TRUE if the transition is currently running, %FALSE otherwise.
 *
 * Since: 3.12
 */
gboolean
ctk_stack_get_transition_running (CtkStack *stack)
{
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);

  g_return_val_if_fail (CTK_IS_STACK (stack), FALSE);

  return (priv->tick_id != 0);
}

/**
 * ctk_stack_set_interpolate_size:
 * @stack: A #CtkStack
 * @interpolate_size: the new value
 *
 * Sets whether or not @stack will interpolate its size when
 * changing the visible child. If the #CtkStack:interpolate-size
 * property is set to %TRUE, @stack will interpolate its size between
 * the current one and the one it'll take after changing the
 * visible child, according to the set transition duration.
 *
 * Since: 3.18
 */
void
ctk_stack_set_interpolate_size (CtkStack *stack,
                                gboolean  interpolate_size)
{
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);
  g_return_if_fail (CTK_IS_STACK (stack));

  interpolate_size = !!interpolate_size;

  if (priv->interpolate_size == interpolate_size)
    return;

  priv->interpolate_size = interpolate_size;
  g_object_notify_by_pspec (G_OBJECT (stack),
                            stack_props[PROP_INTERPOLATE_SIZE]);
}

/**
 * ctk_stack_get_interpolate_size:
 * @stack: A #CtkStack
 *
 * Returns wether the #CtkStack is set up to interpolate between
 * the sizes of children on page switch.
 *
 * Returns: %TRUE if child sizes are interpolated
 *
 * Since: 3.18
 */
gboolean
ctk_stack_get_interpolate_size (CtkStack *stack)
{
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);
  g_return_val_if_fail (CTK_IS_STACK (stack), FALSE);

  return priv->interpolate_size;
}



/**
 * ctk_stack_get_visible_child:
 * @stack: a #CtkStack
 *
 * Gets the currently visible child of @stack, or %NULL if
 * there are no visible children.
 *
 * Returns: (transfer none) (nullable): the visible child of the #CtkStack
 *
 * Since: 3.10
 */
CtkWidget *
ctk_stack_get_visible_child (CtkStack *stack)
{
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);

  g_return_val_if_fail (CTK_IS_STACK (stack), NULL);

  return priv->visible_child ? priv->visible_child->widget : NULL;
}

/**
 * ctk_stack_get_visible_child_name:
 * @stack: a #CtkStack
 *
 * Returns the name of the currently visible child of @stack, or
 * %NULL if there is no visible child.
 *
 * Returns: (transfer none) (nullable): the name of the visible child of the #CtkStack
 *
 * Since: 3.10
 */
const gchar *
ctk_stack_get_visible_child_name (CtkStack *stack)
{
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);

  g_return_val_if_fail (CTK_IS_STACK (stack), NULL);

  if (priv->visible_child)
    return priv->visible_child->name;

  return NULL;
}

/**
 * ctk_stack_set_visible_child:
 * @stack: a #CtkStack
 * @child: a child of @stack
 *
 * Makes @child the visible child of @stack.
 *
 * If @child is different from the currently
 * visible child, the transition between the
 * two will be animated with the current
 * transition type of @stack.
 *
 * Note that the @child widget has to be visible itself
 * (see ctk_widget_show()) in order to become the visible
 * child of @stack.
 *
 * Since: 3.10
 */
void
ctk_stack_set_visible_child (CtkStack  *stack,
                             CtkWidget *child)
{
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);
  CtkStackChildInfo *child_info;

  g_return_if_fail (CTK_IS_STACK (stack));
  g_return_if_fail (CTK_IS_WIDGET (child));

  child_info = find_child_info_for_widget (stack, child);
  if (child_info == NULL)
    {
      g_warning ("Given child of type '%s' not found in CtkStack",
                 G_OBJECT_TYPE_NAME (child));
      return;
    }

  if (ctk_widget_get_visible (child_info->widget))
    set_visible_child (stack, child_info,
                       priv->transition_type,
                       priv->transition_duration);
}

/**
 * ctk_stack_set_visible_child_name:
 * @stack: a #CtkStack
 * @name: the name of the child to make visible
 *
 * Makes the child with the given name visible.
 *
 * If @child is different from the currently
 * visible child, the transition between the
 * two will be animated with the current
 * transition type of @stack.
 *
 * Note that the child widget has to be visible itself
 * (see ctk_widget_show()) in order to become the visible
 * child of @stack.
 *
 * Since: 3.10
 */
void
ctk_stack_set_visible_child_name (CtkStack   *stack,
                                 const gchar *name)
{
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);

  g_return_if_fail (CTK_IS_STACK (stack));

  ctk_stack_set_visible_child_full (stack, name, priv->transition_type);
}

/**
 * ctk_stack_set_visible_child_full:
 * @stack: a #CtkStack
 * @name: the name of the child to make visible
 * @transition: the transition type to use
 *
 * Makes the child with the given name visible.
 *
 * Note that the child widget has to be visible itself
 * (see ctk_widget_show()) in order to become the visible
 * child of @stack.
 *
 * Since: 3.10
 */
void
ctk_stack_set_visible_child_full (CtkStack               *stack,
                                  const gchar            *name,
                                  CtkStackTransitionType  transition)
{
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);
  CtkStackChildInfo *child_info, *info;
  GList *l;

  g_return_if_fail (CTK_IS_STACK (stack));

  if (name == NULL)
    return;

  child_info = NULL;
  for (l = priv->children; l != NULL; l = l->next)
    {
      info = l->data;
      if (info->name != NULL &&
          strcmp (info->name, name) == 0)
        {
          child_info = info;
          break;
        }
    }

  if (child_info == NULL)
    {
      g_warning ("Child name '%s' not found in CtkStack", name);
      return;
    }

  if (ctk_widget_get_visible (child_info->widget))
    set_visible_child (stack, child_info, transition, priv->transition_duration);
}

static void
ctk_stack_forall (CtkContainer *container,
                  gboolean      include_internals G_GNUC_UNUSED,
                  CtkCallback   callback,
                  gpointer      callback_data)
{
  CtkStack *stack = CTK_STACK (container);
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);
  CtkStackChildInfo *child_info;
  GList *l;

  l = priv->children;
  while (l)
    {
      child_info = l->data;
      l = l->next;

      (* callback) (child_info->widget, callback_data);
    }
}

static void
ctk_stack_compute_expand (CtkWidget *widget,
                          gboolean  *hexpand_p,
                          gboolean  *vexpand_p)
{
  CtkStack *stack = CTK_STACK (widget);
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);
  gboolean hexpand, vexpand;
  CtkStackChildInfo *child_info;
  CtkWidget *child;
  GList *l;

  hexpand = FALSE;
  vexpand = FALSE;
  for (l = priv->children; l != NULL; l = l->next)
    {
      child_info = l->data;
      child = child_info->widget;

      if (!hexpand &&
          ctk_widget_compute_expand (child, CTK_ORIENTATION_HORIZONTAL))
        hexpand = TRUE;

      if (!vexpand &&
          ctk_widget_compute_expand (child, CTK_ORIENTATION_VERTICAL))
        vexpand = TRUE;

      if (hexpand && vexpand)
        break;
    }

  *hexpand_p = hexpand;
  *vexpand_p = vexpand;
}

static void
ctk_stack_draw_crossfade (CtkWidget *widget,
                          cairo_t   *cr)
{
  CtkStack *stack = CTK_STACK (widget);
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);
  gdouble progress = ctk_progress_tracker_get_progress (&priv->tracker, FALSE);

  cairo_push_group (cr);
  ctk_container_propagate_draw (CTK_CONTAINER (stack),
                                priv->visible_child->widget,
                                cr);
  cairo_save (cr);

  /* Multiply alpha by progress */
  cairo_set_source_rgba (cr, 1, 1, 1, progress);
  cairo_set_operator (cr, CAIRO_OPERATOR_DEST_IN);
  cairo_paint (cr);

  if (priv->last_visible_surface)
    {
      cairo_set_source_surface (cr, priv->last_visible_surface,
                                priv->last_visible_surface_allocation.x,
                                priv->last_visible_surface_allocation.y);
      cairo_set_operator (cr, CAIRO_OPERATOR_ADD);
      cairo_paint_with_alpha (cr, MAX (1.0 - progress, 0));
    }

  cairo_restore (cr);

  cairo_pop_group_to_source (cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_paint (cr);
}

static void
ctk_stack_draw_under (CtkWidget *widget,
                      cairo_t   *cr)
{
  CtkStack *stack = CTK_STACK (widget);
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);
  CtkAllocation allocation;
  gint x, y, width, height, pos_x, pos_y;

  ctk_widget_get_allocation (widget, &allocation);
  x = y = 0;
  width = allocation.width;
  height = allocation.height;
  pos_x = pos_y = 0;

  switch (priv->active_transition_type)
    {
    case CTK_STACK_TRANSITION_TYPE_UNDER_DOWN:
      y = 0;
      height = allocation.height * (ctk_progress_tracker_get_ease_out_cubic (&priv->tracker, FALSE));
      pos_y = height;
      break;
    case CTK_STACK_TRANSITION_TYPE_UNDER_UP:
      y = allocation.height * (1 - ctk_progress_tracker_get_ease_out_cubic (&priv->tracker, FALSE));
      height = allocation.height - y;
      pos_y = y - allocation.height;
      break;
    case CTK_STACK_TRANSITION_TYPE_UNDER_LEFT:
      x = allocation.width * (1 - ctk_progress_tracker_get_ease_out_cubic (&priv->tracker, FALSE));
      width = allocation.width - x;
      pos_x = x - allocation.width;
      break;
    case CTK_STACK_TRANSITION_TYPE_UNDER_RIGHT:
      x = 0;
      width = allocation.width * (ctk_progress_tracker_get_ease_out_cubic (&priv->tracker, FALSE));
      pos_x = width;
      break;
    default:
      g_assert_not_reached ();
    }

  cairo_save (cr);
  cairo_rectangle (cr, x, y, width, height);
  cairo_clip (cr);

  ctk_container_propagate_draw (CTK_CONTAINER (stack),
                                priv->visible_child->widget,
                                cr);

  cairo_restore (cr);

  if (priv->last_visible_surface)
    {
      cairo_set_source_surface (cr, priv->last_visible_surface, pos_x, pos_y);
      cairo_paint (cr);
    }
}

static void
ctk_stack_draw_slide (CtkWidget *widget,
                      cairo_t   *cr)
{
  CtkStack *stack = CTK_STACK (widget);
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);

  if (priv->last_visible_surface &&
      ctk_cairo_should_draw_window (cr, priv->view_window))
    {
      CtkAllocation allocation;
      int x, y;

      ctk_widget_get_allocation (widget, &allocation);

      x = get_bin_window_x (stack, &allocation);
      y = get_bin_window_y (stack, &allocation);

      switch (priv->active_transition_type)
        {
        case CTK_STACK_TRANSITION_TYPE_SLIDE_LEFT:
          x -= allocation.width;
          break;
        case CTK_STACK_TRANSITION_TYPE_SLIDE_RIGHT:
          x += allocation.width;
          break;
        case CTK_STACK_TRANSITION_TYPE_SLIDE_UP:
          y -= allocation.height;
          break;
        case CTK_STACK_TRANSITION_TYPE_SLIDE_DOWN:
          y += allocation.height;
          break;
        case CTK_STACK_TRANSITION_TYPE_OVER_UP:
        case CTK_STACK_TRANSITION_TYPE_OVER_DOWN:
          y = 0;
          break;
        case CTK_STACK_TRANSITION_TYPE_OVER_LEFT:
        case CTK_STACK_TRANSITION_TYPE_OVER_RIGHT:
          x = 0;
          break;
        default:
          g_assert_not_reached ();
          break;
        }

      x += priv->last_visible_surface_allocation.x;
      y += priv->last_visible_surface_allocation.y;

      if (priv->last_visible_child != NULL)
        {
          if (ctk_widget_get_valign (priv->last_visible_child->widget) == CTK_ALIGN_END &&
              priv->last_visible_widget_height > allocation.height)
            y -= priv->last_visible_widget_height - allocation.height;
          else if (ctk_widget_get_valign (priv->last_visible_child->widget) == CTK_ALIGN_CENTER)
            y -= (priv->last_visible_widget_height - allocation.height) / 2;
        }

      cairo_save (cr);
      cairo_set_source_surface (cr, priv->last_visible_surface, x, y);
      cairo_paint (cr);
      cairo_restore (cr);
     }

  if (ctk_cairo_should_draw_window (cr, priv->bin_window))
    ctk_container_propagate_draw (CTK_CONTAINER (stack),
                                  priv->visible_child->widget,
                                  cr);
}

static gboolean
ctk_stack_draw (CtkWidget *widget,
                cairo_t   *cr)
{
  CtkStack *stack = CTK_STACK (widget);
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);

  ctk_css_gadget_draw (priv->gadget, cr);

  return FALSE;
}

static gboolean
ctk_stack_render (CtkCssGadget *gadget,
                  cairo_t      *cr,
                  int           x G_GNUC_UNUSED,
                  int           y G_GNUC_UNUSED,
                  int           width G_GNUC_UNUSED,
                  int           height G_GNUC_UNUSED,
                  gpointer      data G_GNUC_UNUSED)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkStack *stack = CTK_STACK (widget);
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);
  cairo_t *pattern_cr;

  if (ctk_cairo_should_draw_window (cr, priv->view_window))
    {
      CtkStyleContext *context;

      context = ctk_widget_get_style_context (widget);
      ctk_render_background (context,
                             cr,
                             0, 0,
                             ctk_widget_get_allocated_width (widget),
                             ctk_widget_get_allocated_height (widget));
    }

  if (priv->visible_child)
    {
      if (ctk_progress_tracker_get_state (&priv->tracker) != CTK_PROGRESS_STATE_AFTER)
        {
          if (priv->last_visible_surface == NULL &&
              priv->last_visible_child != NULL)
            {
              ctk_widget_get_allocation (priv->last_visible_child->widget,
                                         &priv->last_visible_surface_allocation);
              priv->last_visible_surface =
                cdk_window_create_similar_surface (ctk_widget_get_window (widget),
                                                   CAIRO_CONTENT_COLOR_ALPHA,
                                                   priv->last_visible_surface_allocation.width,
                                                   priv->last_visible_surface_allocation.height);
              pattern_cr = cairo_create (priv->last_visible_surface);
              /* We don't use propagate_draw here, because we don't want to apply
               * the bin_window offset
               */
              ctk_widget_draw (priv->last_visible_child->widget, pattern_cr);
              cairo_destroy (pattern_cr);
            }

          cairo_rectangle (cr,
                           0, 0,
                           ctk_widget_get_allocated_width (widget),
                           ctk_widget_get_allocated_height (widget));
          cairo_clip (cr);

          switch (priv->active_transition_type)
            {
            case CTK_STACK_TRANSITION_TYPE_CROSSFADE:
	      if (ctk_cairo_should_draw_window (cr, priv->bin_window))
		ctk_stack_draw_crossfade (widget, cr);
              break;
            case CTK_STACK_TRANSITION_TYPE_SLIDE_LEFT:
            case CTK_STACK_TRANSITION_TYPE_SLIDE_RIGHT:
            case CTK_STACK_TRANSITION_TYPE_SLIDE_UP:
            case CTK_STACK_TRANSITION_TYPE_SLIDE_DOWN:
            case CTK_STACK_TRANSITION_TYPE_OVER_UP:
            case CTK_STACK_TRANSITION_TYPE_OVER_DOWN:
            case CTK_STACK_TRANSITION_TYPE_OVER_LEFT:
            case CTK_STACK_TRANSITION_TYPE_OVER_RIGHT:
              ctk_stack_draw_slide (widget, cr);
              break;
            case CTK_STACK_TRANSITION_TYPE_UNDER_UP:
            case CTK_STACK_TRANSITION_TYPE_UNDER_DOWN:
            case CTK_STACK_TRANSITION_TYPE_UNDER_LEFT:
            case CTK_STACK_TRANSITION_TYPE_UNDER_RIGHT:
	      if (ctk_cairo_should_draw_window (cr, priv->bin_window))
		ctk_stack_draw_under (widget, cr);
              break;
            default:
              g_assert_not_reached ();
            }

        }
      else if (ctk_cairo_should_draw_window (cr, priv->bin_window))
        ctk_container_propagate_draw (CTK_CONTAINER (stack),
                                      priv->visible_child->widget,
                                      cr);
    }

  return FALSE;
}

static void
ctk_stack_size_allocate (CtkWidget     *widget,
                         CtkAllocation *allocation)
{
  CtkStack *stack = CTK_STACK (widget);
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);
  CtkAllocation clip;

  ctk_widget_set_allocation (widget, allocation);

  ctk_css_gadget_allocate (priv->gadget,
                           allocation,
                           ctk_widget_get_allocated_baseline (widget),
                           &clip);

  ctk_widget_set_clip (widget, &clip);
}

static void
ctk_stack_allocate (CtkCssGadget        *gadget,
                    const CtkAllocation *allocation,
                    int                  baseline G_GNUC_UNUSED,
                    CtkAllocation       *out_clip,
                    gpointer             data G_GNUC_UNUSED)
{
  CtkWidget *widget;
  CtkStack *stack;
  CtkStackPrivate *priv;
  CtkAllocation child_allocation;

  widget = ctk_css_gadget_get_owner (gadget);
  stack = CTK_STACK (widget);
  priv = ctk_stack_get_instance_private (stack);

  child_allocation.x = 0;
  child_allocation.y = 0;

  if (ctk_widget_get_realized (widget))
    {
      cdk_window_move_resize (priv->view_window,
                              allocation->x, allocation->y,
                              allocation->width, allocation->height);
      cdk_window_move_resize (priv->bin_window,
                              get_bin_window_x (stack, allocation), get_bin_window_y (stack, allocation),
                              allocation->width, allocation->height);
    }

  if (priv->last_visible_child)
    {
      int min, nat;
      ctk_widget_get_preferred_width (priv->last_visible_child->widget, &min, &nat);
      child_allocation.width = MAX (min, allocation->width);
      ctk_widget_get_preferred_height_for_width (priv->last_visible_child->widget,
                                                 child_allocation.width,
                                                 &min, &nat);
      child_allocation.height = MAX (min, allocation->height);

      ctk_widget_size_allocate (priv->last_visible_child->widget, &child_allocation);
    }

  child_allocation.width = allocation->width;
  child_allocation.height = allocation->height;

  if (priv->visible_child)
    {
      int min, nat;
      CtkAlign valign;

      ctk_widget_get_preferred_height_for_width (priv->visible_child->widget,
                                                 allocation->width,
                                                 &min, &nat);
      if (priv->interpolate_size)
        {
          valign = ctk_widget_get_valign (priv->visible_child->widget);
          child_allocation.height = MAX (nat, allocation->height);
          if (valign == CTK_ALIGN_END &&
              child_allocation.height > allocation->height)
            child_allocation.y -= nat - allocation->height;
          else if (valign == CTK_ALIGN_CENTER &&
                   child_allocation.height > allocation->height)
            child_allocation.y -= (nat - allocation->height) / 2;
        }

      ctk_widget_size_allocate (priv->visible_child->widget, &child_allocation);
    }
  ctk_container_get_children_clip (CTK_CONTAINER (widget), out_clip);
}

static void
ctk_stack_get_preferred_width (CtkWidget *widget,
                               gint      *minimum,
                               gint      *natural)
{
  CtkStack *stack = CTK_STACK (widget);
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);

  ctk_css_gadget_get_preferred_size (priv->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_stack_get_preferred_width_for_height (CtkWidget *widget,
                                          gint       height,
                                          gint      *minimum,
                                          gint      *natural)
{
  CtkStack *stack = CTK_STACK (widget);
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);

  ctk_css_gadget_get_preferred_size (priv->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     height,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_stack_get_preferred_height (CtkWidget *widget,
                                gint      *minimum,
                                gint      *natural)
{
  CtkStack *stack = CTK_STACK (widget);
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);

  ctk_css_gadget_get_preferred_size (priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_stack_get_preferred_height_for_width (CtkWidget *widget,
                                          gint       width,
                                          gint      *minimum,
                                          gint      *natural)
{
  CtkStack *stack = CTK_STACK (widget);
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);

  ctk_css_gadget_get_preferred_size (priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     width,
                                     minimum, natural,
                                     NULL, NULL);
}

#define LERP(a, b, t) ((a) + (((b) - (a)) * (1.0 - (t))))

static void
ctk_stack_measure (CtkCssGadget   *gadget,
                   CtkOrientation  orientation,
                   int             for_size,
                   int            *minimum,
                   int            *natural,
                   int            *minimum_baseline G_GNUC_UNUSED,
                   int            *natural_baseline G_GNUC_UNUSED,
                   gpointer        data G_GNUC_UNUSED)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkStack *stack = CTK_STACK (widget);
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);
  CtkStackChildInfo *child_info;
  CtkWidget *child;
  gint child_min, child_nat;
  GList *l;

  *minimum = 0;
  *natural = 0;

  for (l = priv->children; l != NULL; l = l->next)
    {
      child_info = l->data;
      child = child_info->widget;

      if (((orientation == CTK_ORIENTATION_VERTICAL && !priv->vhomogeneous) ||
           (orientation == CTK_ORIENTATION_HORIZONTAL && !priv->hhomogeneous)) &&
           priv->visible_child != child_info)
        continue;

      if (ctk_widget_get_visible (child))
        {
          if (orientation == CTK_ORIENTATION_VERTICAL)
            {
              if (for_size < 0)
                ctk_widget_get_preferred_height (child, &child_min, &child_nat);
              else
                ctk_widget_get_preferred_height_for_width (child, for_size, &child_min, &child_nat);
            }
          else
            {
              if (for_size < 0)
                ctk_widget_get_preferred_width (child, &child_min, &child_nat);
              else
                ctk_widget_get_preferred_width_for_height (child, for_size, &child_min, &child_nat);
            }

          *minimum = MAX (*minimum, child_min);
          *natural = MAX (*natural, child_nat);
        }
    }

  if (priv->last_visible_child != NULL)
    {
      if (orientation == CTK_ORIENTATION_VERTICAL && !priv->vhomogeneous)
        {
          gdouble t = priv->interpolate_size ? ctk_progress_tracker_get_ease_out_cubic (&priv->tracker, FALSE) : 1.0;
          *minimum = LERP (*minimum, priv->last_visible_widget_height, t);
          *natural = LERP (*natural, priv->last_visible_widget_height, t);
        }
      if (orientation == CTK_ORIENTATION_HORIZONTAL && !priv->hhomogeneous)
        {
          gdouble t = priv->interpolate_size ? ctk_progress_tracker_get_ease_out_cubic (&priv->tracker, FALSE) : 1.0;
          *minimum = LERP (*minimum, priv->last_visible_widget_width, t);
          *natural = LERP (*natural, priv->last_visible_widget_width, t);
        }
    }
}

static void
ctk_stack_init (CtkStack *stack)
{
  CtkStackPrivate *priv = ctk_stack_get_instance_private (stack);

  ctk_widget_set_has_window (CTK_WIDGET (stack), FALSE);

  priv->vhomogeneous = TRUE;
  priv->hhomogeneous = TRUE;
  priv->transition_duration = 200;
  priv->transition_type = CTK_STACK_TRANSITION_TYPE_NONE;

  priv->gadget = ctk_css_custom_gadget_new_for_node (ctk_widget_get_css_node (CTK_WIDGET (stack)),
                                                     CTK_WIDGET (stack),
                                                     ctk_stack_measure,
                                                     ctk_stack_allocate,
                                                     ctk_stack_render,
                                                     NULL,
                                                     NULL);

}
