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
 */

#include "config.h"
#include "ctkstackswitcher.h"
#include "ctkradiobutton.h"
#include "ctklabel.h"
#include "ctkdnd.h"
#include "ctkdragdest.h"
#include "ctkorientable.h"
#include "ctkprivate.h"
#include "ctkintl.h"

/**
 * SECTION:ctkstackswitcher
 * @Short_description: A controller for CtkStack
 * @Title: CtkStackSwitcher
 * @See_also: #CtkStack
 *
 * The CtkStackSwitcher widget acts as a controller for a
 * #CtkStack; it shows a row of buttons to switch between
 * the various pages of the associated stack widget.
 *
 * All the content for the buttons comes from the child properties
 * of the #CtkStack; the button visibility in a #CtkStackSwitcher
 * widget is controlled by the visibility of the child in the
 * #CtkStack.
 *
 * It is possible to associate multiple #CtkStackSwitcher widgets
 * with the same #CtkStack widget.
 *
 * The CtkStackSwitcher widget was added in 3.10.
 *
 * # CSS nodes
 *
 * CtkStackSwitcher has a single CSS node named stackswitcher and
 * style class .stack-switcher.
 *
 * When circumstances require it, CtkStackSwitcher adds the
 * .needs-attention style class to the widgets representing the
 * stack pages.
 */

#define TIMEOUT_EXPAND 500

typedef struct _CtkStackSwitcherPrivate CtkStackSwitcherPrivate;
struct _CtkStackSwitcherPrivate
{
  CtkStack *stack;
  GHashTable *buttons;
  gint icon_size;
  gboolean in_child_changed;
  CtkWidget *switch_button;
  guint switch_timer;
};

enum {
  PROP_0,
  PROP_ICON_SIZE,
  PROP_STACK
};

G_DEFINE_TYPE_WITH_PRIVATE (CtkStackSwitcher, ctk_stack_switcher, CTK_TYPE_BOX)

static void
ctk_stack_switcher_init (CtkStackSwitcher *switcher)
{
  CtkStyleContext *context;
  CtkStackSwitcherPrivate *priv;

  ctk_widget_set_has_window (CTK_WIDGET (switcher), FALSE);

  priv = ctk_stack_switcher_get_instance_private (switcher);

  priv->icon_size = CTK_ICON_SIZE_MENU;
  priv->stack = NULL;
  priv->buttons = g_hash_table_new (g_direct_hash, g_direct_equal);

  context = ctk_widget_get_style_context (CTK_WIDGET (switcher));
  ctk_style_context_add_class (context, "stack-switcher");
  ctk_style_context_add_class (context, CTK_STYLE_CLASS_LINKED);

  ctk_orientable_set_orientation (CTK_ORIENTABLE (switcher), CTK_ORIENTATION_HORIZONTAL);

  ctk_drag_dest_set (CTK_WIDGET (switcher), 0, NULL, 0, 0);
  ctk_drag_dest_set_track_motion (CTK_WIDGET (switcher), TRUE);
}

static void
on_button_clicked (CtkWidget        *widget,
                   CtkStackSwitcher *self)
{
  CtkWidget *child;
  CtkStackSwitcherPrivate *priv;

  priv = ctk_stack_switcher_get_instance_private (self);

  if (!priv->in_child_changed)
    {
      child = g_object_get_data (G_OBJECT (widget), "stack-child");
      ctk_stack_set_visible_child (priv->stack, child);
    }
}

static void
rebuild_child (CtkWidget   *self,
               const gchar *icon_name,
               const gchar *title,
               gint         icon_size)
{
  CtkStyleContext *context;
  CtkWidget *button_child;

  button_child = ctk_bin_get_child (CTK_BIN (self));
  if (button_child != NULL)
    ctk_widget_destroy (button_child);

  button_child = NULL;
  context = ctk_widget_get_style_context (CTK_WIDGET (self));

  if (icon_name != NULL)
    {
      button_child = ctk_image_new_from_icon_name (icon_name, icon_size);
      if (title != NULL)
        ctk_widget_set_tooltip_text (CTK_WIDGET (self), title);

      ctk_style_context_remove_class (context, "text-button");
      ctk_style_context_add_class (context, "image-button");
    }
  else if (title != NULL)
    {
      button_child = ctk_label_new (title);

      ctk_widget_set_tooltip_text (CTK_WIDGET (self), NULL);

      ctk_style_context_remove_class (context, "image-button");
      ctk_style_context_add_class (context, "text-button");
    }

  if (button_child)
    {
      ctk_widget_set_halign (CTK_WIDGET (button_child), CTK_ALIGN_CENTER);
      ctk_widget_show_all (button_child);
      ctk_container_add (CTK_CONTAINER (self), button_child);
    }
}

static void
update_needs_attention (CtkWidget *widget, CtkWidget *button, gpointer data)
{
  CtkContainer *container;
  gboolean needs_attention;
  CtkStyleContext *context;

  container = CTK_CONTAINER (data);
  ctk_container_child_get (container, widget,
                           "needs-attention", &needs_attention,
                           NULL);

  context = ctk_widget_get_style_context (button);
  if (needs_attention)
    ctk_style_context_add_class (context, CTK_STYLE_CLASS_NEEDS_ATTENTION);
  else
    ctk_style_context_remove_class (context, CTK_STYLE_CLASS_NEEDS_ATTENTION);
}

static void
update_button (CtkStackSwitcher *self,
               CtkWidget        *widget,
               CtkWidget        *button)
{
  gchar *title;
  gchar *icon_name;
  CtkStackSwitcherPrivate *priv;

  priv = ctk_stack_switcher_get_instance_private (self);

  ctk_container_child_get (CTK_CONTAINER (priv->stack), widget,
                           "title", &title,
                           "icon-name", &icon_name,
                           NULL);

  rebuild_child (button, icon_name, title, priv->icon_size);

  ctk_widget_set_visible (button, ctk_widget_get_visible (widget) && (title != NULL || icon_name != NULL));

  g_free (title);
  g_free (icon_name);

  update_needs_attention (widget, button, priv->stack);
}

static void
on_title_icon_visible_updated (CtkWidget        *widget,
                               GParamSpec       *pspec,
                               CtkStackSwitcher *self)
{
  CtkWidget *button;
  CtkStackSwitcherPrivate *priv;

  priv = ctk_stack_switcher_get_instance_private (self);

  button = g_hash_table_lookup (priv->buttons, widget);
  update_button (self, widget, button);
}

static void
on_position_updated (CtkWidget        *widget,
                     GParamSpec       *pspec,
                     CtkStackSwitcher *self)
{
  CtkWidget *button;
  gint position;
  CtkStackSwitcherPrivate *priv;

  priv = ctk_stack_switcher_get_instance_private (self);

  button = g_hash_table_lookup (priv->buttons, widget);

  ctk_container_child_get (CTK_CONTAINER (priv->stack), widget,
                           "position", &position,
                           NULL);

  ctk_box_reorder_child (CTK_BOX (self), button, position);
}

static void
on_needs_attention_updated (CtkWidget        *widget,
                            GParamSpec       *pspec,
                            CtkStackSwitcher *self)
{
  CtkWidget *button;
  CtkStackSwitcherPrivate *priv;

  priv = ctk_stack_switcher_get_instance_private (self);

  button = g_hash_table_lookup (priv->buttons, widget);
  update_button (self, widget, button);
}

static void
remove_switch_timer (CtkStackSwitcher *self)
{
  CtkStackSwitcherPrivate *priv;

  priv = ctk_stack_switcher_get_instance_private (self);

  if (priv->switch_timer)
    {
      g_source_remove (priv->switch_timer);
      priv->switch_timer = 0;
    }
}

static gboolean
ctk_stack_switcher_switch_timeout (gpointer data)
{
  CtkStackSwitcher *self = data;
  CtkStackSwitcherPrivate *priv;
  CtkWidget *button;

  priv = ctk_stack_switcher_get_instance_private (self);

  priv->switch_timer = 0;

  button = priv->switch_button;
  priv->switch_button = NULL;

  if (button)
    ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (button), TRUE);

  return G_SOURCE_REMOVE;
}

static gboolean
ctk_stack_switcher_drag_motion (CtkWidget      *widget,
                                CdkDragContext *context,
                                gint            x,
                                gint            y,
                                guint           time)
{
  CtkStackSwitcher *self = CTK_STACK_SWITCHER (widget);
  CtkStackSwitcherPrivate *priv;
  CtkAllocation allocation;
  CtkWidget *button;
  GHashTableIter iter;
  gpointer value;
  gboolean retval = FALSE;

  ctk_widget_get_allocation (widget, &allocation);

  priv = ctk_stack_switcher_get_instance_private (self);

  x += allocation.x;
  y += allocation.y;

  button = NULL;
  g_hash_table_iter_init (&iter, priv->buttons);
  while (g_hash_table_iter_next (&iter, NULL, &value))
    {
      ctk_widget_get_allocation (CTK_WIDGET (value), &allocation);
      if (x >= allocation.x && x <= allocation.x + allocation.width &&
          y >= allocation.y && y <= allocation.y + allocation.height)
        {
          button = CTK_WIDGET (value);
          retval = TRUE;
          break;
        }
    }

  if (button != priv->switch_button)
    remove_switch_timer (self);

  priv->switch_button = button;

  if (button && !priv->switch_timer)
    {
      priv->switch_timer = cdk_threads_add_timeout (TIMEOUT_EXPAND,
                                                    ctk_stack_switcher_switch_timeout,
                                                    self);
      g_source_set_name_by_id (priv->switch_timer, "[ctk+] ctk_stack_switcher_switch_timeout");
    }

  return retval;
}

static void
ctk_stack_switcher_drag_leave (CtkWidget      *widget,
                               CdkDragContext *context,
                               guint           time)
{
  CtkStackSwitcher *self = CTK_STACK_SWITCHER (widget);

  remove_switch_timer (self);
}

static void
add_child (CtkWidget        *widget,
           CtkStackSwitcher *self)
{
  CtkWidget *button;
  GList *group;
  CtkStackSwitcherPrivate *priv;

  priv = ctk_stack_switcher_get_instance_private (self);

  button = ctk_radio_button_new (NULL);

  ctk_widget_set_focus_on_click (button, FALSE);
  ctk_toggle_button_set_mode (CTK_TOGGLE_BUTTON (button), FALSE);

  update_button (self, widget, button);

  group = ctk_container_get_children (CTK_CONTAINER (self));
  if (group != NULL)
    {
      ctk_radio_button_join_group (CTK_RADIO_BUTTON (button), CTK_RADIO_BUTTON (group->data));
      g_list_free (group);
    }

  ctk_container_add (CTK_CONTAINER (self), button);

  g_object_set_data (G_OBJECT (button), "stack-child", widget);
  g_signal_connect (button, "clicked", G_CALLBACK (on_button_clicked), self);
  g_signal_connect (widget, "notify::visible", G_CALLBACK (on_title_icon_visible_updated), self);
  g_signal_connect (widget, "child-notify::title", G_CALLBACK (on_title_icon_visible_updated), self);
  g_signal_connect (widget, "child-notify::icon-name", G_CALLBACK (on_title_icon_visible_updated), self);
  g_signal_connect (widget, "child-notify::position", G_CALLBACK (on_position_updated), self);
  g_signal_connect (widget, "child-notify::needs-attention", G_CALLBACK (on_needs_attention_updated), self);

  g_hash_table_insert (priv->buttons, widget, button);
}

static void
remove_child (CtkWidget        *widget,
              CtkStackSwitcher *self)
{
  CtkWidget *button;
  CtkStackSwitcherPrivate *priv;

  priv = ctk_stack_switcher_get_instance_private (self);

  g_signal_handlers_disconnect_by_func (widget, on_title_icon_visible_updated, self);
  g_signal_handlers_disconnect_by_func (widget, on_position_updated, self);
  g_signal_handlers_disconnect_by_func (widget, on_needs_attention_updated, self);

  button = g_hash_table_lookup (priv->buttons, widget);
  ctk_container_remove (CTK_CONTAINER (self), button);
  g_hash_table_remove (priv->buttons, widget);
}

static void
populate_switcher (CtkStackSwitcher *self)
{
  CtkStackSwitcherPrivate *priv;
  CtkWidget *widget, *button;

  priv = ctk_stack_switcher_get_instance_private (self);
  ctk_container_foreach (CTK_CONTAINER (priv->stack), (CtkCallback)add_child, self);

  widget = ctk_stack_get_visible_child (priv->stack);
  if (widget)
    {
      button = g_hash_table_lookup (priv->buttons, widget);
      priv->in_child_changed = TRUE;
      ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (button), TRUE);
      priv->in_child_changed = FALSE;
    }
}

static void
clear_switcher (CtkStackSwitcher *self)
{
  CtkStackSwitcherPrivate *priv;

  priv = ctk_stack_switcher_get_instance_private (self);
  ctk_container_foreach (CTK_CONTAINER (priv->stack), (CtkCallback)remove_child, self);
}

static void
on_child_changed (CtkWidget        *widget,
                  GParamSpec       *pspec,
                  CtkStackSwitcher *self)
{
  CtkWidget *child;
  CtkWidget *button;
  CtkStackSwitcherPrivate *priv;

  priv = ctk_stack_switcher_get_instance_private (self);

  child = ctk_stack_get_visible_child (CTK_STACK (widget));
  button = g_hash_table_lookup (priv->buttons, child);
  if (button != NULL)
    {
      priv->in_child_changed = TRUE;
      ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (button), TRUE);
      priv->in_child_changed = FALSE;
    }
}

static void
on_stack_child_added (CtkContainer     *container,
                      CtkWidget        *widget,
                      CtkStackSwitcher *self)
{
  add_child (widget, self);
}

static void
on_stack_child_removed (CtkContainer     *container,
                        CtkWidget        *widget,
                        CtkStackSwitcher *self)
{
  remove_child (widget, self);
}

static void
disconnect_stack_signals (CtkStackSwitcher *switcher)
{
  CtkStackSwitcherPrivate *priv;

  priv = ctk_stack_switcher_get_instance_private (switcher);
  g_signal_handlers_disconnect_by_func (priv->stack, on_stack_child_added, switcher);
  g_signal_handlers_disconnect_by_func (priv->stack, on_stack_child_removed, switcher);
  g_signal_handlers_disconnect_by_func (priv->stack, on_child_changed, switcher);
  g_signal_handlers_disconnect_by_func (priv->stack, disconnect_stack_signals, switcher);
}

static void
connect_stack_signals (CtkStackSwitcher *switcher)
{
  CtkStackSwitcherPrivate *priv;

  priv = ctk_stack_switcher_get_instance_private (switcher);
  g_signal_connect_after (priv->stack, "add",
                          G_CALLBACK (on_stack_child_added), switcher);
  g_signal_connect_after (priv->stack, "remove",
                          G_CALLBACK (on_stack_child_removed), switcher);
  g_signal_connect (priv->stack, "notify::visible-child",
                    G_CALLBACK (on_child_changed), switcher);
  g_signal_connect_swapped (priv->stack, "destroy",
                            G_CALLBACK (disconnect_stack_signals), switcher);
}

/**
 * ctk_stack_switcher_set_stack:
 * @switcher: a #CtkStackSwitcher
 * @stack: (allow-none): a #CtkStack
 *
 * Sets the stack to control.
 *
 * Since: 3.10
 */
void
ctk_stack_switcher_set_stack (CtkStackSwitcher *switcher,
                              CtkStack         *stack)
{
  CtkStackSwitcherPrivate *priv;

  g_return_if_fail (CTK_IS_STACK_SWITCHER (switcher));
  g_return_if_fail (CTK_IS_STACK (stack) || stack == NULL);

  priv = ctk_stack_switcher_get_instance_private (switcher);

  if (priv->stack == stack)
    return;

  if (priv->stack)
    {
      disconnect_stack_signals (switcher);
      clear_switcher (switcher);
      g_clear_object (&priv->stack);
    }
  if (stack)
    {
      priv->stack = g_object_ref (stack);
      populate_switcher (switcher);
      connect_stack_signals (switcher);
    }

  ctk_widget_queue_resize (CTK_WIDGET (switcher));

  g_object_notify (G_OBJECT (switcher), "stack");
}

/**
 * ctk_stack_switcher_get_stack:
 * @switcher: a #CtkStackSwitcher
 *
 * Retrieves the stack.
 * See ctk_stack_switcher_set_stack().
 *
 * Returns: (nullable) (transfer none): the stack, or %NULL if
 *    none has been set explicitly.
 *
 * Since: 3.10
 */
CtkStack *
ctk_stack_switcher_get_stack (CtkStackSwitcher *switcher)
{
  CtkStackSwitcherPrivate *priv;
  g_return_val_if_fail (CTK_IS_STACK_SWITCHER (switcher), NULL);

  priv = ctk_stack_switcher_get_instance_private (switcher);
  return priv->stack;
}

static void
ctk_stack_switcher_set_icon_size (CtkStackSwitcher *switcher,
                                  gint              icon_size)
{
  CtkStackSwitcherPrivate *priv;

  g_return_if_fail (CTK_IS_STACK_SWITCHER (switcher));

  priv = ctk_stack_switcher_get_instance_private (switcher);

  if (icon_size != priv->icon_size)
    {
      priv->icon_size = icon_size;

      if (priv->stack != NULL)
        {
          clear_switcher (switcher);
          populate_switcher (switcher);
        }

      g_object_notify (G_OBJECT (switcher), "icon-size");
    }
}

static void
ctk_stack_switcher_get_property (GObject      *object,
                                 guint         prop_id,
                                 GValue       *value,
                                 GParamSpec   *pspec)
{
  CtkStackSwitcher *switcher = CTK_STACK_SWITCHER (object);
  CtkStackSwitcherPrivate *priv;

  priv = ctk_stack_switcher_get_instance_private (switcher);
  switch (prop_id)
    {
    case PROP_ICON_SIZE:
      g_value_set_int (value, priv->icon_size);
      break;

    case PROP_STACK:
      g_value_set_object (value, priv->stack);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_stack_switcher_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  CtkStackSwitcher *switcher = CTK_STACK_SWITCHER (object);

  switch (prop_id)
    {
    case PROP_ICON_SIZE:
      ctk_stack_switcher_set_icon_size (switcher, g_value_get_int (value));
      break;

    case PROP_STACK:
      ctk_stack_switcher_set_stack (switcher, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_stack_switcher_dispose (GObject *object)
{
  CtkStackSwitcher *switcher = CTK_STACK_SWITCHER (object);

  remove_switch_timer (switcher);
  ctk_stack_switcher_set_stack (switcher, NULL);

  G_OBJECT_CLASS (ctk_stack_switcher_parent_class)->dispose (object);
}

static void
ctk_stack_switcher_finalize (GObject *object)
{
  CtkStackSwitcher *switcher = CTK_STACK_SWITCHER (object);
  CtkStackSwitcherPrivate *priv;

  priv = ctk_stack_switcher_get_instance_private (switcher);

  g_hash_table_destroy (priv->buttons);

  G_OBJECT_CLASS (ctk_stack_switcher_parent_class)->finalize (object);
}

static void
ctk_stack_switcher_class_init (CtkStackSwitcherClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (class);

  object_class->get_property = ctk_stack_switcher_get_property;
  object_class->set_property = ctk_stack_switcher_set_property;
  object_class->dispose = ctk_stack_switcher_dispose;
  object_class->finalize = ctk_stack_switcher_finalize;

  widget_class->drag_motion = ctk_stack_switcher_drag_motion;
  widget_class->drag_leave = ctk_stack_switcher_drag_leave;
  /**
   * CtkStackSwitcher:icon-size:
   *
   * Use the "icon-size" property to change the size of the image displayed
   * when a #CtkStackSwitcher is displaying icons.
   *
   * Since: 3.20
   */
  g_object_class_install_property (object_class,
                                   PROP_ICON_SIZE,
                                   g_param_spec_int ("icon-size",
                                                     P_("Icon Size"),
                                                     P_("Symbolic size to use for named icon"),
                                                     0, G_MAXINT,
                                                     CTK_ICON_SIZE_MENU,
                                                     G_PARAM_EXPLICIT_NOTIFY | CTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_STACK,
                                   g_param_spec_object ("stack",
                                                        P_("Stack"),
                                                        P_("Stack"),
                                                        CTK_TYPE_STACK,
                                                        CTK_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  ctk_widget_class_set_css_name (widget_class, "stackswitcher");
}

/**
 * ctk_stack_switcher_new:
 *
 * Create a new #CtkStackSwitcher.
 *
 * Returns: a new #CtkStackSwitcher.
 *
 * Since: 3.10
 */
CtkWidget *
ctk_stack_switcher_new (void)
{
  return g_object_new (CTK_TYPE_STACK_SWITCHER, NULL);
}
