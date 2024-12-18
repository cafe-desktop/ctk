/*
 * Copyright (c) 2014 Intel Corporation
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
 * Author:
 *      Ikey Doherty <michael.i.doherty@intel.com>
 */

#include "config.h"

#include "ctkstacksidebar.h"

#include "ctklabel.h"
#include "ctklistbox.h"
#include "ctkscrolledwindow.h"
#include "ctkseparator.h"
#include "ctkstylecontext.h"
#include "ctkprivate.h"
#include "ctkintl.h"

/**
 * SECTION:ctkstacksidebar
 * @Title: CtkStackSidebar
 * @Short_description: An automatic sidebar widget
 *
 * A CtkStackSidebar enables you to quickly and easily provide a
 * consistent "sidebar" object for your user interface.
 *
 * In order to use a CtkStackSidebar, you simply use a CtkStack to
 * organize your UI flow, and add the sidebar to your sidebar area. You
 * can use ctk_stack_sidebar_set_stack() to connect the #CtkStackSidebar
 * to the #CtkStack.
 *
 * # CSS nodes
 *
 * CtkStackSidebar has a single CSS node with name stacksidebar and
 * style class .sidebar.
 *
 * When circumstances require it, CtkStackSidebar adds the
 * .needs-attention style class to the widgets representing the stack
 * pages.
 *
 * Since: 3.16
 */

struct _CtkStackSidebarPrivate
{
  CtkListBox *list;
  CtkStack *stack;
  GHashTable *rows;
  gboolean in_child_changed;
};

G_DEFINE_TYPE_WITH_PRIVATE (CtkStackSidebar, ctk_stack_sidebar, CTK_TYPE_BIN)

enum
{
  PROP_0,
  PROP_STACK,
  N_PROPERTIES
};
static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void
ctk_stack_sidebar_set_property (GObject    *object,
                                guint       prop_id,
                                const       GValue *value,
                                GParamSpec *pspec)
{
  switch (prop_id)
    {
    case PROP_STACK:
      ctk_stack_sidebar_set_stack (CTK_STACK_SIDEBAR (object), g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_stack_sidebar_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  CtkStackSidebarPrivate *priv = ctk_stack_sidebar_get_instance_private (CTK_STACK_SIDEBAR (object));

  switch (prop_id)
    {
    case PROP_STACK:
      g_value_set_object (value, priv->stack);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
update_header (CtkListBoxRow *row,
               CtkListBoxRow *before,
               gpointer       userdata G_GNUC_UNUSED)
{
  CtkWidget *ret = NULL;

  if (before && !ctk_list_box_row_get_header (row))
    {
      ret = ctk_separator_new (CTK_ORIENTATION_HORIZONTAL);
      ctk_list_box_row_set_header (row, ret);
    }
}

static gint
sort_list (CtkListBoxRow *row1,
           CtkListBoxRow *row2,
           gpointer       userdata)
{
  CtkStackSidebar *sidebar = CTK_STACK_SIDEBAR (userdata);
  CtkStackSidebarPrivate *priv = ctk_stack_sidebar_get_instance_private (sidebar);
  CtkWidget *item;
  CtkWidget *widget;
  gint left = 0; gint right = 0;


  if (row1)
    {
      item = ctk_bin_get_child (CTK_BIN (row1));
      widget = g_object_get_data (G_OBJECT (item), "stack-child");
      ctk_container_child_get (CTK_CONTAINER (priv->stack), widget,
                               "position", &left,
                               NULL);
    }

  if (row2)
    {
      item = ctk_bin_get_child (CTK_BIN (row2));
      widget = g_object_get_data (G_OBJECT (item), "stack-child");
      ctk_container_child_get (CTK_CONTAINER (priv->stack), widget,
                               "position", &right,
                               NULL);
    }

  if (left < right)
    return  -1;

  if (left == right)
    return 0;

  return 1;
}

static void
ctk_stack_sidebar_row_selected (CtkListBox    *box G_GNUC_UNUSED,
                                CtkListBoxRow *row,
                                gpointer       userdata)
{
  CtkStackSidebar *sidebar = CTK_STACK_SIDEBAR (userdata);
  CtkStackSidebarPrivate *priv = ctk_stack_sidebar_get_instance_private (sidebar);
  CtkWidget *item;
  CtkWidget *widget;

  if (priv->in_child_changed)
    return;

  if (!row)
    return;

  item = ctk_bin_get_child (CTK_BIN (row));
  widget = g_object_get_data (G_OBJECT (item), "stack-child");
  ctk_stack_set_visible_child (priv->stack, widget);
}

static void
ctk_stack_sidebar_init (CtkStackSidebar *sidebar)
{
  CtkStyleContext *style;
  CtkStackSidebarPrivate *priv;
  CtkWidget *sw;

  priv = ctk_stack_sidebar_get_instance_private (sidebar);

  sw = ctk_scrolled_window_new (NULL, NULL);
  ctk_widget_show (sw);
  ctk_widget_set_no_show_all (sw, TRUE);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (sw),
                                  CTK_POLICY_NEVER,
                                  CTK_POLICY_AUTOMATIC);

  ctk_container_add (CTK_CONTAINER (sidebar), sw);

  priv->list = CTK_LIST_BOX (ctk_list_box_new ());
  ctk_widget_show (CTK_WIDGET (priv->list));

  ctk_container_add (CTK_CONTAINER (sw), CTK_WIDGET (priv->list));

  ctk_list_box_set_header_func (priv->list, update_header, sidebar, NULL);
  ctk_list_box_set_sort_func (priv->list, sort_list, sidebar, NULL);

  g_signal_connect (priv->list, "row-selected",
                    G_CALLBACK (ctk_stack_sidebar_row_selected), sidebar);

  style = ctk_widget_get_style_context (CTK_WIDGET (sidebar));
  ctk_style_context_add_class (style, "sidebar");

  priv->rows = g_hash_table_new (NULL, NULL);
}

static void
update_row (CtkStackSidebar *sidebar,
            CtkWidget       *widget,
            CtkWidget       *row)
{
  CtkStackSidebarPrivate *priv = ctk_stack_sidebar_get_instance_private (sidebar);
  CtkWidget *item;
  gchar *title;
  gboolean needs_attention;
  CtkStyleContext *context;

  ctk_container_child_get (CTK_CONTAINER (priv->stack), widget,
                           "title", &title,
                           "needs-attention", &needs_attention,
                           NULL);

  item = ctk_bin_get_child (CTK_BIN (row));
  ctk_label_set_text (CTK_LABEL (item), title);

  ctk_widget_set_visible (row, ctk_widget_get_visible (widget) && title != NULL);

  context = ctk_widget_get_style_context (row);
  if (needs_attention)
     ctk_style_context_add_class (context, CTK_STYLE_CLASS_NEEDS_ATTENTION);
  else
    ctk_style_context_remove_class (context, CTK_STYLE_CLASS_NEEDS_ATTENTION);

  g_free (title);
}

static void
on_position_updated (CtkWidget       *widget G_GNUC_UNUSED,
                     GParamSpec      *pspec G_GNUC_UNUSED,
                     CtkStackSidebar *sidebar)
{
  CtkStackSidebarPrivate *priv = ctk_stack_sidebar_get_instance_private (sidebar);

  ctk_list_box_invalidate_sort (priv->list);
}

static void
on_child_updated (CtkWidget       *widget,
                  GParamSpec      *pspec G_GNUC_UNUSED,
                  CtkStackSidebar *sidebar)
{
  CtkStackSidebarPrivate *priv = ctk_stack_sidebar_get_instance_private (sidebar);
  CtkWidget *row;

  row = g_hash_table_lookup (priv->rows, widget);
  update_row (sidebar, widget, row);
}

static void
add_child (CtkWidget       *widget,
           CtkStackSidebar *sidebar)
{
  CtkStackSidebarPrivate *priv = ctk_stack_sidebar_get_instance_private (sidebar);
  CtkWidget *item;
  CtkWidget *row;

  /* Check we don't actually already know about this widget */
  if (g_hash_table_lookup (priv->rows, widget))
    return;

  /* Make a pretty item when we add kids */
  item = ctk_label_new ("");
  ctk_widget_set_halign (item, CTK_ALIGN_START);
  ctk_widget_set_valign (item, CTK_ALIGN_CENTER);
  row = ctk_list_box_row_new ();
  ctk_container_add (CTK_CONTAINER (row), item);
  ctk_widget_show (item);

  update_row (sidebar, widget, row);

  /* Hook up for events */
  g_signal_connect (widget, "child-notify::title",
                    G_CALLBACK (on_child_updated), sidebar);
  g_signal_connect (widget, "child-notify::needs-attention",
                    G_CALLBACK (on_child_updated), sidebar);
  g_signal_connect (widget, "notify::visible",
                    G_CALLBACK (on_child_updated), sidebar);
  g_signal_connect (widget, "child-notify::position",
                    G_CALLBACK (on_position_updated), sidebar);

  g_object_set_data (G_OBJECT (item), "stack-child", widget);
  g_hash_table_insert (priv->rows, widget, row);
  ctk_container_add (CTK_CONTAINER (priv->list), row);
}

static void
remove_child (CtkWidget       *widget,
              CtkStackSidebar *sidebar)
{
  CtkStackSidebarPrivate *priv = ctk_stack_sidebar_get_instance_private (sidebar);
  CtkWidget *row;

  row = g_hash_table_lookup (priv->rows, widget);
  if (!row)
    return;

  g_signal_handlers_disconnect_by_func (widget, on_child_updated, sidebar);
  g_signal_handlers_disconnect_by_func (widget, on_position_updated, sidebar);

  ctk_container_remove (CTK_CONTAINER (priv->list), row);
  g_hash_table_remove (priv->rows, widget);
}

static void
populate_sidebar (CtkStackSidebar *sidebar)
{
  CtkStackSidebarPrivate *priv = ctk_stack_sidebar_get_instance_private (sidebar);
  CtkWidget *widget, *row;

  ctk_container_foreach (CTK_CONTAINER (priv->stack), (CtkCallback)add_child, sidebar);

  widget = ctk_stack_get_visible_child (priv->stack);
  if (widget)
    {
      row = g_hash_table_lookup (priv->rows, widget);
      ctk_list_box_select_row (priv->list, CTK_LIST_BOX_ROW (row));
    }
}

static void
clear_sidebar (CtkStackSidebar *sidebar)
{
  CtkStackSidebarPrivate *priv = ctk_stack_sidebar_get_instance_private (sidebar);

  ctk_container_foreach (CTK_CONTAINER (priv->stack), (CtkCallback)remove_child, sidebar);
}

static void
on_child_changed (CtkWidget       *widget,
                  GParamSpec      *pspec G_GNUC_UNUSED,
                  CtkStackSidebar *sidebar)
{
  CtkStackSidebarPrivate *priv = ctk_stack_sidebar_get_instance_private (sidebar);
  CtkWidget *child;
  CtkWidget *row;

  child = ctk_stack_get_visible_child (CTK_STACK (widget));
  row = g_hash_table_lookup (priv->rows, child);
  if (row != NULL)
    {
      priv->in_child_changed = TRUE;
      ctk_list_box_select_row (priv->list, CTK_LIST_BOX_ROW (row));
      priv->in_child_changed = FALSE;
    }
}

static void
on_stack_child_added (CtkContainer    *container G_GNUC_UNUSED,
                      CtkWidget       *widget,
                      CtkStackSidebar *sidebar)
{
  add_child (widget, sidebar);
}

static void
on_stack_child_removed (CtkContainer    *container G_GNUC_UNUSED,
                        CtkWidget       *widget,
                        CtkStackSidebar *sidebar)
{
  remove_child (widget, sidebar);
}

static void
disconnect_stack_signals (CtkStackSidebar *sidebar)
{
  CtkStackSidebarPrivate *priv = ctk_stack_sidebar_get_instance_private (sidebar);

  g_signal_handlers_disconnect_by_func (priv->stack, on_stack_child_added, sidebar);
  g_signal_handlers_disconnect_by_func (priv->stack, on_stack_child_removed, sidebar);
  g_signal_handlers_disconnect_by_func (priv->stack, on_child_changed, sidebar);
  g_signal_handlers_disconnect_by_func (priv->stack, disconnect_stack_signals, sidebar);
}

static void
connect_stack_signals (CtkStackSidebar *sidebar)
{
  CtkStackSidebarPrivate *priv = ctk_stack_sidebar_get_instance_private (sidebar);

  g_signal_connect_after (priv->stack, "add",
                          G_CALLBACK (on_stack_child_added), sidebar);
  g_signal_connect_after (priv->stack, "remove",
                          G_CALLBACK (on_stack_child_removed), sidebar);
  g_signal_connect (priv->stack, "notify::visible-child",
                    G_CALLBACK (on_child_changed), sidebar);
  g_signal_connect_swapped (priv->stack, "destroy",
                            G_CALLBACK (disconnect_stack_signals), sidebar);
}

static void
ctk_stack_sidebar_dispose (GObject *object)
{
  CtkStackSidebar *sidebar = CTK_STACK_SIDEBAR (object);

  ctk_stack_sidebar_set_stack (sidebar, NULL);

  G_OBJECT_CLASS (ctk_stack_sidebar_parent_class)->dispose (object);
}

static void
ctk_stack_sidebar_finalize (GObject *object)
{
  CtkStackSidebar *sidebar = CTK_STACK_SIDEBAR (object);
  CtkStackSidebarPrivate *priv = ctk_stack_sidebar_get_instance_private (sidebar);

  g_hash_table_destroy (priv->rows);

  G_OBJECT_CLASS (ctk_stack_sidebar_parent_class)->finalize (object);
}

static void
ctk_stack_sidebar_class_init (CtkStackSidebarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);

  object_class->dispose = ctk_stack_sidebar_dispose;
  object_class->finalize = ctk_stack_sidebar_finalize;
  object_class->set_property = ctk_stack_sidebar_set_property;
  object_class->get_property = ctk_stack_sidebar_get_property;

  obj_properties[PROP_STACK] =
      g_param_spec_object (I_("stack"), P_("Stack"),
                           P_("Associated stack for this CtkStackSidebar"),
                           CTK_TYPE_STACK,
                           G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, N_PROPERTIES, obj_properties);

  ctk_widget_class_set_css_name (widget_class, "stacksidebar");
}

/**
 * ctk_stack_sidebar_new:
 *
 * Creates a new sidebar.
 *
 * Returns: the new #CtkStackSidebar
 *
 * Since: 3.16
 */
CtkWidget *
ctk_stack_sidebar_new (void)
{
  return CTK_WIDGET (g_object_new (CTK_TYPE_STACK_SIDEBAR, NULL));
}

/**
 * ctk_stack_sidebar_set_stack:
 * @sidebar: a #CtkStackSidebar
 * @stack: a #CtkStack
 *
 * Set the #CtkStack associated with this #CtkStackSidebar.
 *
 * The sidebar widget will automatically update according to the order
 * (packing) and items within the given #CtkStack.
 *
 * Since: 3.16
 */
void
ctk_stack_sidebar_set_stack (CtkStackSidebar *sidebar,
                             CtkStack        *stack)
{
  CtkStackSidebarPrivate *priv;

  g_return_if_fail (CTK_IS_STACK_SIDEBAR (sidebar));
  g_return_if_fail (CTK_IS_STACK (stack) || stack == NULL);

  priv = ctk_stack_sidebar_get_instance_private (sidebar);

  if (priv->stack == stack)
    return;

  if (priv->stack)
    {
      disconnect_stack_signals (sidebar);
      clear_sidebar (sidebar);
      g_clear_object (&priv->stack);
    }
  if (stack)
    {
      priv->stack = g_object_ref (stack);
      populate_sidebar (sidebar);
      connect_stack_signals (sidebar);
    }

  ctk_widget_queue_resize (CTK_WIDGET (sidebar));

  g_object_notify (G_OBJECT (sidebar), "stack");
}

/**
 * ctk_stack_sidebar_get_stack:
 * @sidebar: a #CtkStackSidebar
 *
 * Retrieves the stack.
 * See ctk_stack_sidebar_set_stack().
 *
 * Returns: (nullable) (transfer none): the associated #CtkStack or
 *     %NULL if none has been set explicitly
 *
 * Since: 3.16
 */
CtkStack *
ctk_stack_sidebar_get_stack (CtkStackSidebar *sidebar)
{
  CtkStackSidebarPrivate *priv;

  g_return_val_if_fail (CTK_IS_STACK_SIDEBAR (sidebar), NULL);

  priv = ctk_stack_sidebar_get_instance_private (sidebar);

  return CTK_STACK (priv->stack);
}
