/*
 * Copyright (c) 2013 - 2014 Red Hat, Inc.
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

#include "ctkactionbar.h"
#include "ctkintl.h"
#include "ctkaccessible.h"
#include "ctkbuildable.h"
#include "ctktypebuiltins.h"
#include "ctkbox.h"
#include "ctkrevealer.h"
#include "ctkcsscustomgadgetprivate.h"
#include "ctkwidgetprivate.h"
#include "ctkcontainerprivate.h"

#include <string.h>

/**
 * SECTION:ctkactionbar
 * @Short_description: A full width bar for presenting contextual actions
 * @Title: CtkActionBar
 * @See_also: #CtkBox
 *
 * CtkActionBar is designed to present contextual actions. It is
 * expected to be displayed below the content and expand horizontally
 * to fill the area.
 *
 * It allows placing children at the start or the end. In addition, it
 * contains an internal centered box which is centered with respect to
 * the full width of the box, even if the children at either side take
 * up different amounts of space.
 *
 * # CSS nodes
 *
 * CtkActionBar has a single CSS node with name actionbar.
 */

struct _CtkActionBarPrivate
{
  CtkWidget *box;
  CtkWidget *revealer;
  CtkCssGadget *gadget;
};

enum {
  CHILD_PROP_0,
  CHILD_PROP_PACK_TYPE,
  CHILD_PROP_POSITION
};

static void ctk_action_bar_finalize (GObject *object);
static void ctk_action_bar_buildable_interface_init (CtkBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (CtkActionBar, ctk_action_bar, CTK_TYPE_BIN,
                         G_ADD_PRIVATE (CtkActionBar)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_BUILDABLE,
                                                ctk_action_bar_buildable_interface_init))

static void
ctk_action_bar_show (CtkWidget *widget)
{
  CtkActionBarPrivate *priv = ctk_action_bar_get_instance_private (CTK_ACTION_BAR (widget));

  CTK_WIDGET_CLASS (ctk_action_bar_parent_class)->show (widget);

  ctk_revealer_set_reveal_child (CTK_REVEALER (priv->revealer), TRUE);
}

static void
child_revealed (GObject    *object,
                GParamSpec *pspec G_GNUC_UNUSED,
                CtkWidget  *widget)
{
  CTK_WIDGET_CLASS (ctk_action_bar_parent_class)->hide (widget);
  g_signal_handlers_disconnect_by_func (object, child_revealed, widget);
  g_object_notify (G_OBJECT (widget), "visible");
}

static void
ctk_action_bar_hide (CtkWidget *widget)
{
  CtkActionBarPrivate *priv = ctk_action_bar_get_instance_private (CTK_ACTION_BAR (widget));

  g_signal_connect_object (priv->revealer, "notify::child-revealed",
                            G_CALLBACK (child_revealed), widget, 0);
  ctk_revealer_set_reveal_child (CTK_REVEALER (priv->revealer), FALSE);
}

static void
ctk_action_bar_add (CtkContainer *container,
                    CtkWidget    *child)
{
  CtkActionBarPrivate *priv = ctk_action_bar_get_instance_private (CTK_ACTION_BAR (container));

  /* When constructing the widget, we want the revealer to be added
   * as the first child of the bar, as an implementation detail.
   * After that, the child added by the application should be added
   * to box.
   */

  if (priv->box == NULL)
    CTK_CONTAINER_CLASS (ctk_action_bar_parent_class)->add (container, child);
  else
    ctk_container_add (CTK_CONTAINER (priv->box), child);
}

static void
ctk_action_bar_remove (CtkContainer *container,
                       CtkWidget    *child)
{
  CtkActionBarPrivate *priv = ctk_action_bar_get_instance_private (CTK_ACTION_BAR (container));

  if (child == priv->revealer)
    CTK_CONTAINER_CLASS (ctk_action_bar_parent_class)->remove (container, child);
  else
    ctk_container_remove (CTK_CONTAINER (priv->box), child);
}

static void
ctk_action_bar_forall (CtkContainer *container,
                       gboolean      include_internals,
                       CtkCallback   callback,
                       gpointer      callback_data)
{
  CtkActionBarPrivate *priv = ctk_action_bar_get_instance_private (CTK_ACTION_BAR (container));

  if (include_internals)
    (* callback) (priv->revealer, callback_data);
  else if (priv->box)
    ctk_container_forall (CTK_CONTAINER (priv->box), callback, callback_data);
}

static void
ctk_action_bar_destroy (CtkWidget *widget)
{
  CtkActionBarPrivate *priv = ctk_action_bar_get_instance_private (CTK_ACTION_BAR (widget));

  if (priv->revealer)
    {
      ctk_widget_destroy (priv->revealer);
      priv->revealer = NULL;
    }

  CTK_WIDGET_CLASS (ctk_action_bar_parent_class)->destroy (widget);
}

static GType
ctk_action_bar_child_type (CtkContainer *container G_GNUC_UNUSED)
{
  return CTK_TYPE_WIDGET;
}

static void
ctk_action_bar_get_child_property (CtkContainer *container,
                                   CtkWidget    *child,
                                   guint         property_id G_GNUC_UNUSED,
                                   GValue       *value,
                                   GParamSpec   *pspec)
{
  CtkActionBarPrivate *priv = ctk_action_bar_get_instance_private (CTK_ACTION_BAR (container));

  if (child == priv->revealer)
    g_param_value_set_default (pspec, value);
  else
    ctk_container_child_get_property (CTK_CONTAINER (priv->box),
                                      child,
                                      pspec->name,
                                      value);
}

static void
ctk_action_bar_set_child_property (CtkContainer *container,
                                   CtkWidget    *child,
                                   guint         property_id G_GNUC_UNUSED,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  CtkActionBarPrivate *priv = ctk_action_bar_get_instance_private (CTK_ACTION_BAR (container));

  if (child != priv->revealer)
    ctk_container_child_set_property (CTK_CONTAINER (priv->box),
                                      child,
                                      pspec->name,
                                      value);
}

static gboolean
ctk_action_bar_render (CtkCssGadget *gadget,
                       cairo_t      *cr,
                       int           x G_GNUC_UNUSED,
                       int           y G_GNUC_UNUSED,
                       int           width G_GNUC_UNUSED,
                       int           height G_GNUC_UNUSED,
                       gpointer      data G_GNUC_UNUSED)
{
  CTK_WIDGET_CLASS (ctk_action_bar_parent_class)->draw (ctk_css_gadget_get_owner (gadget), cr);

  return FALSE;
}

static gboolean
ctk_action_bar_draw (CtkWidget *widget,
                     cairo_t   *cr)
{
  CtkActionBarPrivate *priv = ctk_action_bar_get_instance_private (CTK_ACTION_BAR (widget));

  ctk_css_gadget_draw (priv->gadget, cr);

  return FALSE;
}

static void
ctk_action_bar_allocate (CtkCssGadget        *gadget,
                         const CtkAllocation *allocation,
                         int                  baseline G_GNUC_UNUSED,
                         CtkAllocation       *out_clip,
                         gpointer             data G_GNUC_UNUSED)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkActionBarPrivate *priv = ctk_action_bar_get_instance_private (CTK_ACTION_BAR (widget));

  ctk_widget_size_allocate (priv->revealer, (CtkAllocation *)allocation);

  ctk_container_get_children_clip (CTK_CONTAINER (widget), out_clip);
}

static void
ctk_action_bar_size_allocate (CtkWidget     *widget,
                              CtkAllocation *allocation)
{
  CtkActionBarPrivate *priv = ctk_action_bar_get_instance_private (CTK_ACTION_BAR (widget));
  CtkAllocation clip;

  ctk_widget_set_allocation (widget, allocation);

  ctk_css_gadget_allocate (priv->gadget, allocation, ctk_widget_get_allocated_baseline (widget), &clip);

  ctk_widget_set_clip (widget, &clip);
}

static void
ctk_action_bar_measure (CtkCssGadget   *gadget,
                        CtkOrientation  orientation,
                        int             for_size,
                        int            *minimum,
                        int            *natural,
                        int            *minimum_baseline,
                        int            *natural_baseline,
                        gpointer        data G_GNUC_UNUSED)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkActionBarPrivate *priv = ctk_action_bar_get_instance_private (CTK_ACTION_BAR (widget));

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    ctk_widget_get_preferred_width_for_height (priv->revealer, for_size, minimum, natural);
  else
    ctk_widget_get_preferred_height_and_baseline_for_width (priv->revealer, for_size, minimum, natural, minimum_baseline, natural_baseline);
}

static void
ctk_action_bar_get_preferred_width_for_height (CtkWidget *widget,
                                               gint       height,
                                               gint      *minimum,
                                               gint      *natural)
{
  CtkActionBarPrivate *priv = ctk_action_bar_get_instance_private (CTK_ACTION_BAR (widget));

  ctk_css_gadget_get_preferred_size (priv->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     height,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_action_bar_get_preferred_height_and_baseline_for_width (CtkWidget *widget,
                                                            gint       width,
                                                            gint      *minimum,
                                                            gint      *natural,
                                                            gint      *minimum_baseline,
                                                            gint      *natural_baseline)
{
  CtkActionBarPrivate *priv = ctk_action_bar_get_instance_private (CTK_ACTION_BAR (widget));

  ctk_css_gadget_get_preferred_size (priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     width,
                                     minimum, natural,
                                     minimum_baseline, natural_baseline);
}

static void
ctk_action_bar_class_init (CtkActionBarClass *klass)
{
  GObjectClass *object_class;
  CtkWidgetClass *widget_class;
  CtkContainerClass *container_class;

  object_class = G_OBJECT_CLASS (klass);
  widget_class = CTK_WIDGET_CLASS (klass);
  container_class = CTK_CONTAINER_CLASS (klass);

  object_class->finalize = ctk_action_bar_finalize;

  widget_class->show = ctk_action_bar_show;
  widget_class->hide = ctk_action_bar_hide;
  widget_class->destroy = ctk_action_bar_destroy;
  widget_class->draw = ctk_action_bar_draw;
  widget_class->size_allocate = ctk_action_bar_size_allocate;
  widget_class->get_preferred_width_for_height = ctk_action_bar_get_preferred_width_for_height;
  widget_class->get_preferred_height_and_baseline_for_width = ctk_action_bar_get_preferred_height_and_baseline_for_width;

  container_class->add = ctk_action_bar_add;
  container_class->remove = ctk_action_bar_remove;
  container_class->forall = ctk_action_bar_forall;
  container_class->child_type = ctk_action_bar_child_type;
  container_class->set_child_property = ctk_action_bar_set_child_property;
  container_class->get_child_property = ctk_action_bar_get_child_property;

  ctk_container_class_install_child_property (container_class,
                                              CHILD_PROP_PACK_TYPE,
                                              g_param_spec_enum ("pack-type",
                                                                 P_("Pack type"),
                                                                 P_("A CtkPackType indicating whether the child is packed with reference to the start or end of the parent"),
                                                                 CTK_TYPE_PACK_TYPE, CTK_PACK_START,
                                                                 G_PARAM_READWRITE));
  ctk_container_class_install_child_property (container_class,
                                              CHILD_PROP_POSITION,
                                              g_param_spec_int ("position",
                                                                P_("Position"),
                                                                P_("The index of the child in the parent"),
                                                                -1, G_MAXINT, 0,
                                                                G_PARAM_READWRITE));

  ctk_widget_class_set_template_from_resource (widget_class, "/org/ctk/libctk/ui/ctkactionbar.ui");
  ctk_widget_class_bind_template_child_private (widget_class, CtkActionBar, box);
  ctk_widget_class_bind_template_child_private (widget_class, CtkActionBar, revealer);

  ctk_widget_class_set_accessible_role (widget_class, ATK_ROLE_PANEL);
  ctk_widget_class_set_css_name (widget_class, "actionbar");
}

static void
ctk_action_bar_init (CtkActionBar *action_bar)
{
  CtkActionBarPrivate *priv = ctk_action_bar_get_instance_private (action_bar);
  CtkCssNode *widget_node;

  ctk_widget_init_template (CTK_WIDGET (action_bar));

  ctk_revealer_set_transition_type (CTK_REVEALER (priv->revealer), CTK_REVEALER_TRANSITION_TYPE_SLIDE_UP);

  widget_node = ctk_widget_get_css_node (CTK_WIDGET (action_bar));
  priv->gadget = ctk_css_custom_gadget_new_for_node (widget_node,
                                                     CTK_WIDGET (action_bar),
                                                     ctk_action_bar_measure,
                                                     ctk_action_bar_allocate,
                                                     ctk_action_bar_render,
                                                     NULL,
                                                     NULL);
}

static void
ctk_action_bar_finalize (GObject *object)
{
  CtkActionBar *action_bar = CTK_ACTION_BAR (object);
  CtkActionBarPrivate *priv = ctk_action_bar_get_instance_private (action_bar);

  g_clear_object (&priv->gadget);

  G_OBJECT_CLASS (ctk_action_bar_parent_class)->finalize (object);
}

static void
ctk_action_bar_buildable_add_child (CtkBuildable *buildable,
                                    CtkBuilder   *builder G_GNUC_UNUSED,
                                    GObject      *child,
                                    const gchar  *type)
{
  CtkActionBar *action_bar = CTK_ACTION_BAR (buildable);
  CtkActionBarPrivate *priv = ctk_action_bar_get_instance_private (action_bar);

  if (type && strcmp (type, "center") == 0)
    ctk_box_set_center_widget (CTK_BOX (priv->box), CTK_WIDGET (child));
  else if (!type)
    ctk_container_add (CTK_CONTAINER (buildable), CTK_WIDGET (child));
  else
    CTK_BUILDER_WARN_INVALID_CHILD_TYPE (action_bar, type);
}

static CtkBuildableIface *parent_buildable_iface;

static void
ctk_action_bar_buildable_interface_init (CtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->add_child = ctk_action_bar_buildable_add_child;
}

/**
 * ctk_action_bar_pack_start:
 * @action_bar: A #CtkActionBar
 * @child: the #CtkWidget to be added to @action_bar
 *
 * Adds @child to @action_bar, packed with reference to the
 * start of the @action_bar.
 *
 * Since: 3.12
 */
void
ctk_action_bar_pack_start (CtkActionBar *action_bar,
                           CtkWidget    *child)
{
  CtkActionBarPrivate *priv = ctk_action_bar_get_instance_private (action_bar);

  ctk_box_pack_start (CTK_BOX (priv->box), child, FALSE, TRUE, 0);
}

/**
 * ctk_action_bar_pack_end:
 * @action_bar: A #CtkActionBar
 * @child: the #CtkWidget to be added to @action_bar
 *
 * Adds @child to @action_bar, packed with reference to the
 * end of the @action_bar.
 *
 * Since: 3.12
 */
void
ctk_action_bar_pack_end (CtkActionBar *action_bar,
                         CtkWidget    *child)
{
  CtkActionBarPrivate *priv = ctk_action_bar_get_instance_private (action_bar);

  ctk_box_pack_end (CTK_BOX (priv->box), child, FALSE, TRUE, 0);
}

/**
 * ctk_action_bar_set_center_widget:
 * @action_bar: a #CtkActionBar
 * @center_widget: (allow-none): a widget to use for the center
 *
 * Sets the center widget for the #CtkActionBar.
 *
 * Since: 3.12
 */
void
ctk_action_bar_set_center_widget (CtkActionBar *action_bar,
                                  CtkWidget    *center_widget)
{
  CtkActionBarPrivate *priv = ctk_action_bar_get_instance_private (action_bar);

  ctk_box_set_center_widget (CTK_BOX (priv->box), center_widget);
}

/**
 * ctk_action_bar_get_center_widget:
 * @action_bar: a #CtkActionBar
 *
 * Retrieves the center bar widget of the bar.
 *
 * Returns: (transfer none) (nullable): the center #CtkWidget or %NULL.
 *
 * Since: 3.12
 */
CtkWidget *
ctk_action_bar_get_center_widget (CtkActionBar *action_bar)
{
  CtkActionBarPrivate *priv = ctk_action_bar_get_instance_private (action_bar);

  g_return_val_if_fail (CTK_IS_ACTION_BAR (action_bar), NULL);

  return ctk_box_get_center_widget (CTK_BOX (priv->box));
}

/**
 * ctk_action_bar_new:
 *
 * Creates a new #CtkActionBar widget.
 *
 * Returns: a new #CtkActionBar
 *
 * Since: 3.12
 */
CtkWidget *
ctk_action_bar_new (void)
{
  return CTK_WIDGET (g_object_new (CTK_TYPE_ACTION_BAR, NULL));
}
