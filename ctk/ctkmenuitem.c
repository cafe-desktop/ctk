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

#include "config.h"

#include <string.h>

#include "ctkaccellabel.h"
#include "ctkbuiltiniconprivate.h"
#include "ctkcontainerprivate.h"
#include "ctkcsscustomgadgetprivate.h"
#include "ctkmain.h"
#include "ctkmarshalers.h"
#include "ctkmenuprivate.h"
#include "ctkmenushellprivate.h"
#include "ctkmenuitemprivate.h"
#include "ctkmenubar.h"
#include "ctkmenuprivate.h"
#include "ctkseparatormenuitem.h"
#include "ctkprivate.h"
#include "ctkbuildable.h"
#include "ctkactivatable.h"
#include "ctkwidgetprivate.h"
#include "ctkintl.h"
#include "ctksettings.h"
#include "ctktypebuiltins.h"
#include "a11y/ctkmenuitemaccessible.h"
#include "ctktearoffmenuitem.h"
#include "ctkstylecontextprivate.h"
#include "ctkcssstylepropertyprivate.h"

#define MENU_POPUP_DELAY     225

/**
 * SECTION:ctkmenuitem
 * @Short_description: The widget used for item in menus
 * @Title: CtkMenuItem
 * @See_also: #CtkBin, #CtkMenuShell
 *
 * The #CtkMenuItem widget and the derived widgets are the only valid
 * children for menus. Their function is to correctly handle highlighting,
 * alignment, events and submenus.
 *
 * As a CtkMenuItem derives from #CtkBin it can hold any valid child widget,
 * although only a few are really useful.
 *
 * By default, a CtkMenuItem sets a #CtkAccelLabel as its child.
 * CtkMenuItem has direct functions to set the label and its mnemonic.
 * For more advanced label settings, you can fetch the child widget from the CtkBin.
 *
 * An example for setting markup and accelerator on a MenuItem:
 * |[<!-- language="C" -->
 * CtkWidget *menu_item = ctk_menu_item_new_with_label ("Example Menu Item");
 *
 * CtkWidget *child = ctk_bin_get_child (CTK_BIN (menu_item));
 * ctk_label_set_markup (CTK_LABEL (child), "<i>new label</i> with <b>markup</b>");
 * ctk_accel_label_set_accel (CTK_ACCEL_LABEL (child), CDK_KEY_1, 0);
 * ]|
 *
 * # CtkMenuItem as CtkBuildable
 *
 * The CtkMenuItem implementation of the #CtkBuildable interface supports
 * adding a submenu by specifying “submenu” as the “type” attribute of
 * a <child> element.
 *
 * An example of UI definition fragment with submenus:
 * |[
 * <object class="CtkMenuItem">
 *   <child type="submenu">
 *     <object class="CtkMenu"/>
 *   </child>
 * </object>
 * ]|
 *
 * # CSS nodes
 *
 * |[<!-- language="plain" -->
 * menuitem
 * ├── <child>
 * ╰── [arrow.right]
 * ]|
 *
 * CtkMenuItem has a single CSS node with name menuitem. If the menuitem
 * has a submenu, it gets another CSS node with name arrow, which has
 * the .left or .right style class.
 */


enum {
  ACTIVATE,
  ACTIVATE_ITEM,
  TOGGLE_SIZE_REQUEST,
  TOGGLE_SIZE_ALLOCATE,
  SELECT,
  DESELECT,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_RIGHT_JUSTIFIED,
  PROP_SUBMENU,
  PROP_ACCEL_PATH,
  PROP_LABEL,
  PROP_USE_UNDERLINE,

  LAST_PROP,

  /* activatable properties */
  PROP_ACTIVATABLE_RELATED_ACTION = LAST_PROP,
  PROP_ACTIVATABLE_USE_ACTION_APPEARANCE,

  PROP_ACTION_NAME,
  PROP_ACTION_TARGET
};


static void ctk_menu_item_dispose        (GObject          *object);
static void ctk_menu_item_set_property   (GObject          *object,
                                          guint             prop_id,
                                          const GValue     *value,
                                          GParamSpec       *pspec);
static void ctk_menu_item_get_property   (GObject          *object,
                                          guint             prop_id,
                                          GValue           *value,
                                          GParamSpec       *pspec);
static void ctk_menu_item_destroy        (CtkWidget        *widget);
static void ctk_menu_item_realize        (CtkWidget        *widget);
static void ctk_menu_item_unrealize      (CtkWidget        *widget);
static void ctk_menu_item_map            (CtkWidget        *widget);
static void ctk_menu_item_unmap          (CtkWidget        *widget);
static gboolean ctk_menu_item_enter      (CtkWidget        *widget,
                                          CdkEventCrossing *event);
static gboolean ctk_menu_item_leave      (CtkWidget        *widget,
                                          CdkEventCrossing *event);
static void ctk_menu_item_parent_set     (CtkWidget        *widget,
                                          CtkWidget        *previous_parent);
static void ctk_menu_item_direction_changed (CtkWidget        *widget,
                                             CtkTextDirection  previous_dir);


static void ctk_real_menu_item_select               (CtkMenuItem *item);
static void ctk_real_menu_item_deselect             (CtkMenuItem *item);
static void ctk_real_menu_item_activate             (CtkMenuItem *item);
static void ctk_real_menu_item_activate_item        (CtkMenuItem *item);
static void ctk_real_menu_item_toggle_size_request  (CtkMenuItem *menu_item,
                                                     gint        *requisition);
static void ctk_real_menu_item_toggle_size_allocate (CtkMenuItem *menu_item,
                                                     gint         allocation);
static gboolean ctk_menu_item_mnemonic_activate     (CtkWidget   *widget,
                                                     gboolean     group_cycling);

static void ctk_menu_item_ensure_label   (CtkMenuItem      *menu_item);
static gint ctk_menu_item_popup_timeout  (gpointer          data);
static void ctk_menu_item_show_all       (CtkWidget        *widget);

static void ctk_menu_item_forall         (CtkContainer    *container,
                                          gboolean         include_internals,
                                          CtkCallback      callback,
                                          gpointer         callback_data);

static gboolean ctk_menu_item_can_activate_accel (CtkWidget *widget,
                                                  guint      signal_id);

static void ctk_real_menu_item_set_label (CtkMenuItem     *menu_item,
                                          const gchar     *label);
static const gchar * ctk_real_menu_item_get_label (CtkMenuItem *menu_item);

static void ctk_menu_item_buildable_interface_init (CtkBuildableIface   *iface);
static void ctk_menu_item_buildable_add_child      (CtkBuildable        *buildable,
                                                    CtkBuilder          *builder,
                                                    GObject             *child,
                                                    const gchar         *type);
static void ctk_menu_item_buildable_custom_finished(CtkBuildable        *buildable,
                                                    CtkBuilder          *builder,
                                                    GObject             *child,
                                                    const gchar         *tagname,
                                                    gpointer             user_data);

static void ctk_menu_item_actionable_interface_init  (CtkActionableInterface *iface);
static void ctk_menu_item_activatable_interface_init (CtkActivatableIface  *iface);
static void ctk_menu_item_update                     (CtkActivatable       *activatable,
                                                      CtkAction            *action,
                                                      const gchar          *property_name);
static void ctk_menu_item_sync_action_properties     (CtkActivatable       *activatable,
                                                      CtkAction            *action);
static void ctk_menu_item_set_related_action         (CtkMenuItem          *menu_item, 
                                                      CtkAction            *action);
static void ctk_menu_item_set_use_action_appearance  (CtkMenuItem          *menu_item, 
                                                      gboolean              use_appearance);

static guint menu_item_signals[LAST_SIGNAL] = { 0 };
static GParamSpec *menu_item_props[LAST_PROP];

static CtkBuildableIface *parent_buildable_iface;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
G_DEFINE_TYPE_WITH_CODE (CtkMenuItem, ctk_menu_item, CTK_TYPE_BIN,
                         G_ADD_PRIVATE (CtkMenuItem)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_BUILDABLE,
                                                ctk_menu_item_buildable_interface_init)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_ACTIVATABLE,
                                                ctk_menu_item_activatable_interface_init)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_ACTIONABLE,
                                                ctk_menu_item_actionable_interface_init))
G_GNUC_END_IGNORE_DEPRECATIONS;

static void
ctk_menu_item_set_action_name (CtkActionable *actionable,
                               const gchar   *action_name)
{
  CtkMenuItem *menu_item = CTK_MENU_ITEM (actionable);

  if (!menu_item->priv->action_helper)
    menu_item->priv->action_helper = ctk_action_helper_new (actionable);

  ctk_action_helper_set_action_name (menu_item->priv->action_helper, action_name);
}

static void
ctk_menu_item_set_action_target_value (CtkActionable *actionable,
                                       GVariant      *action_target)
{
  CtkMenuItem *menu_item = CTK_MENU_ITEM (actionable);

  if (!menu_item->priv->action_helper)
    menu_item->priv->action_helper = ctk_action_helper_new (actionable);

  ctk_action_helper_set_action_target_value (menu_item->priv->action_helper, action_target);
}

static const gchar *
ctk_menu_item_get_action_name (CtkActionable *actionable)
{
  CtkMenuItem *menu_item = CTK_MENU_ITEM (actionable);

  return ctk_action_helper_get_action_name (menu_item->priv->action_helper);
}

static GVariant *
ctk_menu_item_get_action_target_value (CtkActionable *actionable)
{
  CtkMenuItem *menu_item = CTK_MENU_ITEM (actionable);

  return ctk_action_helper_get_action_target_value (menu_item->priv->action_helper);
}

static void
ctk_menu_item_actionable_interface_init (CtkActionableInterface *iface)
{
  iface->set_action_name = ctk_menu_item_set_action_name;
  iface->get_action_name = ctk_menu_item_get_action_name;
  iface->set_action_target_value = ctk_menu_item_set_action_target_value;
  iface->get_action_target_value = ctk_menu_item_get_action_target_value;
}

static gboolean
ctk_menu_item_render (CtkCssGadget *gadget,
                      cairo_t      *cr,
                      int           x,
                      int           y,
                      int           width,
                      int           height,
                      gpointer      data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkMenuItem *menu_item = CTK_MENU_ITEM (widget);
  CtkMenuItemPrivate *priv = menu_item->priv;
  CtkWidget *parent;

  parent = ctk_widget_get_parent (widget);

  if (priv->submenu && !CTK_IS_MENU_BAR (parent))
    ctk_css_gadget_draw (priv->arrow_gadget, cr);

  CTK_WIDGET_CLASS (ctk_menu_item_parent_class)->draw (widget, cr);

  return FALSE;
}

static gboolean
ctk_menu_item_draw (CtkWidget *widget,
                    cairo_t   *cr)
{
  ctk_css_gadget_draw (CTK_MENU_ITEM (widget)->priv->gadget, cr);

  return FALSE;
}

static void
ctk_menu_item_allocate (CtkCssGadget        *gadget,
                        const CtkAllocation *allocation,
                        int                  baseline,
                        CtkAllocation       *out_clip,
                        gpointer             data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkMenuItem *menu_item = CTK_MENU_ITEM (widget);
  CtkMenuItemPrivate *priv = menu_item->priv;
  CtkAllocation child_allocation;
  CtkAllocation arrow_clip = { 0 };
  CtkTextDirection direction;
  CtkPackDirection child_pack_dir;
  CtkWidget *child;
  CtkWidget *parent;

  g_return_if_fail (CTK_IS_MENU_ITEM (widget));
  g_return_if_fail (allocation != NULL);

  direction = ctk_widget_get_direction (widget);

  parent = ctk_widget_get_parent (widget);
  if (CTK_IS_MENU_BAR (parent))
    {
      child_pack_dir = ctk_menu_bar_get_child_pack_direction (CTK_MENU_BAR (parent));
    }
  else
    {
      child_pack_dir = CTK_PACK_DIRECTION_LTR;
    }

  child = ctk_bin_get_child (CTK_BIN (widget));
  if (child)
    {
      child_allocation = *allocation;

      if (child_pack_dir == CTK_PACK_DIRECTION_LTR ||
          child_pack_dir == CTK_PACK_DIRECTION_RTL)
        {
          if ((direction == CTK_TEXT_DIR_LTR) == (child_pack_dir != CTK_PACK_DIRECTION_RTL))
            child_allocation.x += priv->toggle_size;
          child_allocation.width -= priv->toggle_size;
        }
      else
        {
          if ((direction == CTK_TEXT_DIR_LTR) == (child_pack_dir != CTK_PACK_DIRECTION_BTT))
            child_allocation.y += priv->toggle_size;
          child_allocation.height -= priv->toggle_size;
        }

      if ((priv->submenu && !CTK_IS_MENU_BAR (parent)) || priv->reserve_indicator)
	{
          CtkAllocation arrow_alloc;

          ctk_css_gadget_get_preferred_size (priv->arrow_gadget,
                                             CTK_ORIENTATION_HORIZONTAL,
                                             -1,
                                             &arrow_alloc.width, NULL,
                                             NULL, NULL);
          ctk_css_gadget_get_preferred_size (priv->arrow_gadget,
                                             CTK_ORIENTATION_VERTICAL,
                                             -1,
                                             &arrow_alloc.height, NULL,
                                             NULL, NULL);

          if (direction == CTK_TEXT_DIR_LTR)
            {
              arrow_alloc.x = child_allocation.x +
                child_allocation.width - arrow_alloc.width;
            }
          else
            {
              arrow_alloc.x = 0;
              child_allocation.x += arrow_alloc.width;
            }

          child_allocation.width -= arrow_alloc.width;
          arrow_alloc.y = child_allocation.y +
            (child_allocation.height - arrow_alloc.height) / 2;

          ctk_css_gadget_allocate (priv->arrow_gadget,
                                   &arrow_alloc,
                                   baseline,
                                   &arrow_clip);
	}

      child_allocation.width = MAX (1, child_allocation.width);

      ctk_widget_size_allocate (child, &child_allocation);

      ctk_container_get_children_clip (CTK_CONTAINER (widget), out_clip);
      cdk_rectangle_union (out_clip, &arrow_clip, out_clip);
    }

  if (priv->submenu)
    ctk_menu_reposition (CTK_MENU (priv->submenu));
}

static void
ctk_menu_item_size_allocate (CtkWidget     *widget,
                             CtkAllocation *allocation)
{
  CtkMenuItem *menu_item = CTK_MENU_ITEM (widget);
  CtkMenuItemPrivate *priv = menu_item->priv;
  CtkAllocation clip;
  
  ctk_widget_set_allocation (widget, allocation);

  if (ctk_widget_get_realized (widget))
    cdk_window_move_resize (priv->event_window,
                            allocation->x, allocation->y,
                            allocation->width, allocation->height);

  ctk_css_gadget_allocate (priv->gadget,
                           allocation,
                           ctk_widget_get_allocated_baseline (widget),
                           &clip);

  ctk_widget_set_clip (widget, &clip);
}

static void
ctk_menu_item_accel_width_foreach (CtkWidget *widget,
                                   gpointer   data)
{
  guint *width = data;

  if (CTK_IS_ACCEL_LABEL (widget))
    {
      guint w;

      w = ctk_accel_label_get_accel_width (CTK_ACCEL_LABEL (widget));
      *width = MAX (*width, w);
    }
  else if (CTK_IS_CONTAINER (widget))
    ctk_container_foreach (CTK_CONTAINER (widget),
                           ctk_menu_item_accel_width_foreach,
                           data);
}

static void
ctk_menu_item_real_get_width (CtkWidget *widget,
                              gint      *minimum_size,
                              gint      *natural_size)
{
  CtkMenuItem *menu_item = CTK_MENU_ITEM (widget);
  CtkMenuItemPrivate *priv = menu_item->priv;
  CtkWidget *child;
  CtkWidget *parent;
  guint accel_width;
  gint  min_width, nat_width;

  min_width = nat_width = 0;

  parent = ctk_widget_get_parent (widget);
  child = ctk_bin_get_child (CTK_BIN (widget));

  if (child != NULL && ctk_widget_get_visible (child))
    {
      gint child_min, child_nat;

      ctk_widget_get_preferred_width (child, &child_min, &child_nat);

      if ((priv->submenu && !CTK_IS_MENU_BAR (parent)) || priv->reserve_indicator)
	{
          gint arrow_size;

          ctk_css_gadget_get_preferred_size (priv->arrow_gadget,
                                             CTK_ORIENTATION_HORIZONTAL,
                                             -1,
                                             &arrow_size, NULL,
                                             NULL, NULL);

          min_width += arrow_size;
          nat_width = min_width;
        }

      min_width += child_min;
      nat_width += child_nat;
    }

  accel_width = 0;
  ctk_container_foreach (CTK_CONTAINER (menu_item),
                         ctk_menu_item_accel_width_foreach,
                         &accel_width);
  priv->accelerator_width = accel_width;

  *minimum_size = min_width;
  *natural_size = nat_width;
}

static void
ctk_menu_item_real_get_height (CtkWidget *widget,
                               gint       for_size,
                               gint      *minimum_size,
                               gint      *natural_size)
{
  CtkMenuItem *menu_item = CTK_MENU_ITEM (widget);
  CtkMenuItemPrivate *priv = menu_item->priv;
  CtkWidget *child;
  CtkWidget *parent;
  guint accel_width;
  gint min_height, nat_height;
  gint avail_size = 0;

  min_height = nat_height = 0;

  if (for_size != -1)
    avail_size = for_size;

  parent = ctk_widget_get_parent (widget);
  child = ctk_bin_get_child (CTK_BIN (widget));

  if (child != NULL && ctk_widget_get_visible (child))
    {
      gint child_min, child_nat;
      gint arrow_size = 0;

      if ((priv->submenu && !CTK_IS_MENU_BAR (parent)) || priv->reserve_indicator)
        ctk_css_gadget_get_preferred_size (priv->arrow_gadget,
                                           CTK_ORIENTATION_VERTICAL,
                                           -1,
                                           &arrow_size, NULL,
                                           NULL, NULL);

      if (for_size != -1)
        {
          avail_size -= arrow_size;
          ctk_widget_get_preferred_height_for_width (child,
                                                     avail_size,
                                                     &child_min,
                                                     &child_nat);
        }
      else
        {
          ctk_widget_get_preferred_height (child, &child_min, &child_nat);
        }

      min_height += child_min;
      nat_height += child_nat;

      min_height = MAX (min_height, arrow_size);
      nat_height = MAX (nat_height, arrow_size);
    }

  accel_width = 0;
  ctk_container_foreach (CTK_CONTAINER (menu_item),
                         ctk_menu_item_accel_width_foreach,
                         &accel_width);
  priv->accelerator_width = accel_width;

  *minimum_size = min_height;
  *natural_size = nat_height;
}

static void
ctk_menu_item_measure (CtkCssGadget   *gadget,
                       CtkOrientation  orientation,
                       int             size,
                       int            *minimum,
                       int            *natural,
                       int            *minimum_baseline,
                       int            *natural_baseline,
                       gpointer        data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    ctk_menu_item_real_get_width (widget, minimum, natural);
  else
    ctk_menu_item_real_get_height (widget, size, minimum, natural);
}

static void
ctk_menu_item_get_preferred_width (CtkWidget *widget,
                                   gint      *minimum_size,
                                   gint      *natural_size)
{
  ctk_css_gadget_get_preferred_size (CTK_MENU_ITEM (widget)->priv->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     minimum_size, natural_size,
                                     NULL, NULL);
}

static void
ctk_menu_item_get_preferred_height (CtkWidget *widget,
                                    gint      *minimum_size,
                                    gint      *natural_size)
{
  ctk_css_gadget_get_preferred_size (CTK_MENU_ITEM (widget)->priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     -1,
                                     minimum_size, natural_size,
                                     NULL, NULL);
}

static void
ctk_menu_item_get_preferred_height_for_width (CtkWidget *widget,
                                              gint       for_size,
                                              gint      *minimum_size,
                                              gint      *natural_size)
{
  ctk_css_gadget_get_preferred_size (CTK_MENU_ITEM (widget)->priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     for_size,
                                     minimum_size, natural_size,
                                     NULL, NULL);
}

static void
ctk_menu_item_class_init (CtkMenuItemClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);
  CtkContainerClass *container_class = CTK_CONTAINER_CLASS (klass);

  gobject_class->dispose = ctk_menu_item_dispose;
  gobject_class->set_property = ctk_menu_item_set_property;
  gobject_class->get_property = ctk_menu_item_get_property;

  widget_class->destroy = ctk_menu_item_destroy;
  widget_class->size_allocate = ctk_menu_item_size_allocate;
  widget_class->draw = ctk_menu_item_draw;
  widget_class->realize = ctk_menu_item_realize;
  widget_class->unrealize = ctk_menu_item_unrealize;
  widget_class->map = ctk_menu_item_map;
  widget_class->unmap = ctk_menu_item_unmap;
  widget_class->enter_notify_event = ctk_menu_item_enter;
  widget_class->leave_notify_event = ctk_menu_item_leave;
  widget_class->show_all = ctk_menu_item_show_all;
  widget_class->mnemonic_activate = ctk_menu_item_mnemonic_activate;
  widget_class->parent_set = ctk_menu_item_parent_set;
  widget_class->can_activate_accel = ctk_menu_item_can_activate_accel;
  widget_class->get_preferred_width = ctk_menu_item_get_preferred_width;
  widget_class->get_preferred_height = ctk_menu_item_get_preferred_height;
  widget_class->get_preferred_height_for_width = ctk_menu_item_get_preferred_height_for_width;
  widget_class->direction_changed = ctk_menu_item_direction_changed;

  container_class->forall = ctk_menu_item_forall;

  klass->activate = ctk_real_menu_item_activate;
  klass->activate_item = ctk_real_menu_item_activate_item;
  klass->toggle_size_request = ctk_real_menu_item_toggle_size_request;
  klass->toggle_size_allocate = ctk_real_menu_item_toggle_size_allocate;
  klass->set_label = ctk_real_menu_item_set_label;
  klass->get_label = ctk_real_menu_item_get_label;
  klass->select = ctk_real_menu_item_select;
  klass->deselect = ctk_real_menu_item_deselect;

  klass->hide_on_activate = TRUE;

  /**
   * CtkMenuItem::activate:
   * @menuitem: the object which received the signal.
   *
   * Emitted when the item is activated.
   */
  menu_item_signals[ACTIVATE] =
    g_signal_new (I_("activate"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (CtkMenuItemClass, activate),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);
  widget_class->activate_signal = menu_item_signals[ACTIVATE];

  /**
   * CtkMenuItem::activate-item:
   * @menuitem: the object which received the signal.
   *
   * Emitted when the item is activated, but also if the menu item has a
   * submenu. For normal applications, the relevant signal is
   * #CtkMenuItem::activate.
   */
  menu_item_signals[ACTIVATE_ITEM] =
    g_signal_new (I_("activate-item"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (CtkMenuItemClass, activate_item),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  menu_item_signals[TOGGLE_SIZE_REQUEST] =
    g_signal_new (I_("toggle-size-request"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (CtkMenuItemClass, toggle_size_request),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  menu_item_signals[TOGGLE_SIZE_ALLOCATE] =
    g_signal_new (I_("toggle-size-allocate"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (CtkMenuItemClass, toggle_size_allocate),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_INT);

  menu_item_signals[SELECT] =
    g_signal_new (I_("select"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (CtkMenuItemClass, select),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  menu_item_signals[DESELECT] =
    g_signal_new (I_("deselect"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (CtkMenuItemClass, deselect),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * CtkMenuItem:right-justified:
   *
   * Sets whether the menu item appears justified
   * at the right side of a menu bar.
   *
   * Since: 2.14
   */
  menu_item_props[PROP_RIGHT_JUSTIFIED] =
      g_param_spec_boolean ("right-justified",
                            P_("Right Justified"),
                            P_("Sets whether the menu item appears justified at the right side of a menu bar"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_DEPRECATED);

  /**
   * CtkMenuItem:submenu:
   *
   * The submenu attached to the menu item, or %NULL if it has none.
   *
   * Since: 2.12
   */
  menu_item_props[PROP_SUBMENU] =
      g_param_spec_object ("submenu",
                           P_("Submenu"),
                           P_("The submenu attached to the menu item, or NULL if it has none"),
                           CTK_TYPE_MENU,
                           CTK_PARAM_READWRITE);

  /**
   * CtkMenuItem:accel-path:
   *
   * Sets the accelerator path of the menu item, through which runtime
   * changes of the menu item's accelerator caused by the user can be
   * identified and saved to persistant storage.
   *
   * Since: 2.14
   */
  menu_item_props[PROP_ACCEL_PATH] =
      g_param_spec_string ("accel-path",
                           P_("Accel Path"),
                           P_("Sets the accelerator path of the menu item"),
                           NULL,
                           CTK_PARAM_READWRITE);

  /**
   * CtkMenuItem:label:
   *
   * The text for the child label.
   *
   * Since: 2.16
   */
  menu_item_props[PROP_LABEL] =
      g_param_spec_string ("label",
                           P_("Label"),
                           P_("The text for the child label"),
                           "",
                           CTK_PARAM_READWRITE);

  /**
   * CtkMenuItem:use-underline:
   *
   * %TRUE if underlines in the text indicate mnemonics.
   *
   * Since: 2.16
   */
  menu_item_props[PROP_USE_UNDERLINE] =
      g_param_spec_boolean ("use-underline",
                            P_("Use underline"),
                            P_("If set, an underline in the text indicates "
                               "the next character should be used for the "
                               "mnemonic accelerator key"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class, LAST_PROP, menu_item_props);

  g_object_class_override_property (gobject_class, PROP_ACTIVATABLE_RELATED_ACTION, "related-action");
  g_object_class_override_property (gobject_class, PROP_ACTIVATABLE_USE_ACTION_APPEARANCE, "use-action-appearance");

  g_object_class_override_property (gobject_class, PROP_ACTION_NAME, "action-name");
  g_object_class_override_property (gobject_class, PROP_ACTION_TARGET, "action-target");

  /**
   * CtkMenuItem:selected-shadow-type:
   *
   * The shadow type when the item is selected.
   *
   * Deprecated: 3.20: Use CSS to determine the shadow; the value of this
   *     style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_enum ("selected-shadow-type",
                                                              "Selected Shadow Type",
                                                              "Shadow type when item is selected",
                                                              CTK_TYPE_SHADOW_TYPE,
                                                              CTK_SHADOW_NONE,
                                                              CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  /**
   * CtkMenuItem:horizontal-padding:
   *
   * Padding to left and right of the menu item.
   *
   * Deprecated: 3.8: use the standard padding CSS property (through objects
   *   like #CtkStyleContext and #CtkCssProvider); the value of this style
   *   property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("horizontal-padding",
                                                             "Horizontal Padding",
                                                             "Padding to left and right of the menu item",
                                                             0,
                                                             G_MAXINT,
                                                             0,
                                                             CTK_PARAM_READABLE |
                                                             G_PARAM_DEPRECATED));

  /**
   * CtkMenuItem:toggle-spacing:
   *
   * Spacing between menu icon and label.
   *
   * Deprecated: 3.20: use the standard margin CSS property on the check or
   *   radio nodes; the value of this style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("toggle-spacing",
                                                             "Icon Spacing",
                                                             "Space between icon and label",
                                                             0,
                                                             G_MAXINT,
                                                             5,
                                                             CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  /**
   * CtkMenuItem:arrow-spacing:
   *
   * Spacing between menu item label and submenu arrow.
   *
   * Deprecated: 3.20: use the standard margin CSS property on the arrow node;
   *   the value of this style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("arrow-spacing",
                                                             "Arrow Spacing",
                                                             "Space between label and arrow",
                                                             0,
                                                             G_MAXINT,
                                                             10,
                                                             CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  /**
   * CtkMenuItem:arrow-scaling:
   *
   * Amount of space used up by the arrow, relative to the menu item's font
   * size.
   *
   * Deprecated: 3.20: use the standard min-width/min-height CSS properties on
   *   the arrow node; the value of this style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_float ("arrow-scaling",
                                                               P_("Arrow Scaling"),
                                                               P_("Amount of space used up by arrow, relative to the menu item's font size"),
                                                               0.0, 2.0, 0.8,
                                                               CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  /**
   * CtkMenuItem:width-chars:
   *
   * The minimum desired width of the menu item in characters.
   *
   * Since: 2.14
   *
   * Deprecated: 3.20: Use the standard CSS property min-width; the value of
   *     this style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("width-chars",
                                                             P_("Width in Characters"),
                                                             P_("The minimum desired width of the menu item in characters"),
                                                             0, G_MAXINT, 12,
                                                             CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_MENU_ITEM_ACCESSIBLE);
  ctk_widget_class_set_css_name (widget_class, "menuitem");

  ctk_container_class_handle_border_width (container_class);
}

static void
ctk_menu_item_init (CtkMenuItem *menu_item)
{
  CtkMenuItemPrivate *priv;
  CtkCssNode *widget_node;

  priv = ctk_menu_item_get_instance_private (menu_item);
  menu_item->priv = priv;

  ctk_widget_set_has_window (CTK_WIDGET (menu_item), FALSE);

  priv->action = NULL;
  priv->use_action_appearance = TRUE;
  
  priv->submenu = NULL;
  priv->toggle_size = 0;
  priv->accelerator_width = 0;
  if (ctk_widget_get_direction (CTK_WIDGET (menu_item)) == CTK_TEXT_DIR_RTL)
    priv->submenu_direction = CTK_DIRECTION_LEFT;
  else
    priv->submenu_direction = CTK_DIRECTION_RIGHT;
  priv->submenu_placement = CTK_TOP_BOTTOM;
  priv->right_justify = FALSE;
  priv->use_action_appearance = TRUE;
  priv->timer = 0;
  priv->action = NULL;

  widget_node = ctk_widget_get_css_node (CTK_WIDGET (menu_item));
  priv->gadget = ctk_css_custom_gadget_new_for_node (widget_node,
                                                     CTK_WIDGET (menu_item),
                                                     ctk_menu_item_measure,
                                                     ctk_menu_item_allocate,
                                                     ctk_menu_item_render,
                                                     NULL, NULL);
}

CtkCssGadget *
_ctk_menu_item_get_gadget (CtkMenuItem *menu_item)
{
  return menu_item->priv->gadget;
}

/**
 * ctk_menu_item_new:
 *
 * Creates a new #CtkMenuItem.
 *
 * Returns: the newly created #CtkMenuItem
 */
CtkWidget*
ctk_menu_item_new (void)
{
  return g_object_new (CTK_TYPE_MENU_ITEM, NULL);
}

/**
 * ctk_menu_item_new_with_label:
 * @label: the text for the label
 *
 * Creates a new #CtkMenuItem whose child is a #CtkLabel.
 *
 * Returns: the newly created #CtkMenuItem
 */
CtkWidget*
ctk_menu_item_new_with_label (const gchar *label)
{
  return g_object_new (CTK_TYPE_MENU_ITEM,
                       "label", label,
                       NULL);
}


/**
 * ctk_menu_item_new_with_mnemonic:
 * @label: The text of the button, with an underscore in front of the
 *     mnemonic character
 *
 * Creates a new #CtkMenuItem containing a label.
 *
 * The label will be created using ctk_label_new_with_mnemonic(),
 * so underscores in @label indicate the mnemonic for the menu item.
 *
 * Returns: a new #CtkMenuItem
 */
CtkWidget*
ctk_menu_item_new_with_mnemonic (const gchar *label)
{
  return g_object_new (CTK_TYPE_MENU_ITEM,
                       "use-underline", TRUE,
                       "label", label,
                       NULL);
}

static void
ctk_menu_item_dispose (GObject *object)
{
  CtkMenuItem *menu_item = CTK_MENU_ITEM (object);
  CtkMenuItemPrivate *priv = menu_item->priv;

  g_clear_object (&priv->action_helper);

  if (priv->action)
    {
      ctk_action_disconnect_accelerator (priv->action);
      ctk_activatable_do_set_related_action (CTK_ACTIVATABLE (menu_item), NULL);
      priv->action = NULL;
    }

  g_clear_object (&priv->arrow_gadget);
  g_clear_object (&priv->gadget);

  G_OBJECT_CLASS (ctk_menu_item_parent_class)->dispose (object);
}

static void
ctk_menu_item_do_set_right_justified (CtkMenuItem *menu_item,
                                      gboolean     right_justified)
{
  CtkMenuItemPrivate *priv = menu_item->priv;

  right_justified = right_justified != FALSE;

  if (priv->right_justify != right_justified)
    {
      priv->right_justify = right_justified;
      ctk_widget_queue_resize (CTK_WIDGET (menu_item));
      g_object_notify_by_pspec (G_OBJECT (menu_item), menu_item_props[PROP_RIGHT_JUSTIFIED]);
    }
}

static void
ctk_menu_item_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  CtkMenuItem *menu_item = CTK_MENU_ITEM (object);

  switch (prop_id)
    {
    case PROP_RIGHT_JUSTIFIED:
      ctk_menu_item_do_set_right_justified (menu_item, g_value_get_boolean (value));
      break;
    case PROP_SUBMENU:
      ctk_menu_item_set_submenu (menu_item, g_value_get_object (value));
      break;
    case PROP_ACCEL_PATH:
      ctk_menu_item_set_accel_path (menu_item, g_value_get_string (value));
      break;
    case PROP_LABEL:
      ctk_menu_item_set_label (menu_item, g_value_get_string (value));
      break;
    case PROP_USE_UNDERLINE:
      ctk_menu_item_set_use_underline (menu_item, g_value_get_boolean (value));
      break;
    case PROP_ACTIVATABLE_RELATED_ACTION:
      ctk_menu_item_set_related_action (menu_item, g_value_get_object (value));
      break;
    case PROP_ACTIVATABLE_USE_ACTION_APPEARANCE:
      ctk_menu_item_set_use_action_appearance (menu_item, g_value_get_boolean (value));
      break;
    case PROP_ACTION_NAME:
      ctk_menu_item_set_action_name (CTK_ACTIONABLE (menu_item), g_value_get_string (value));
      break;
    case PROP_ACTION_TARGET:
      ctk_menu_item_set_action_target_value (CTK_ACTIONABLE (menu_item), g_value_get_variant (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_menu_item_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  CtkMenuItem *menu_item = CTK_MENU_ITEM (object);
  CtkMenuItemPrivate *priv = menu_item->priv;

  switch (prop_id)
    {
    case PROP_RIGHT_JUSTIFIED:
      g_value_set_boolean (value, priv->right_justify);
      break;
    case PROP_SUBMENU:
      g_value_set_object (value, ctk_menu_item_get_submenu (menu_item));
      break;
    case PROP_ACCEL_PATH:
      g_value_set_string (value, ctk_menu_item_get_accel_path (menu_item));
      break;
    case PROP_LABEL:
      g_value_set_string (value, ctk_menu_item_get_label (menu_item));
      break;
    case PROP_USE_UNDERLINE:
      g_value_set_boolean (value, ctk_menu_item_get_use_underline (menu_item));
      break;
    case PROP_ACTIVATABLE_RELATED_ACTION:
      g_value_set_object (value, priv->action);
      break;
    case PROP_ACTIVATABLE_USE_ACTION_APPEARANCE:
      g_value_set_boolean (value, priv->use_action_appearance);
      break;
    case PROP_ACTION_NAME:
      g_value_set_string (value, ctk_action_helper_get_action_name (priv->action_helper));
      break;
    case PROP_ACTION_TARGET:
      g_value_set_variant (value, ctk_action_helper_get_action_target_value (priv->action_helper));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_menu_item_destroy (CtkWidget *widget)
{
  CtkMenuItem *menu_item = CTK_MENU_ITEM (widget);
  CtkMenuItemPrivate *priv = menu_item->priv;

  if (priv->submenu)
    ctk_widget_destroy (priv->submenu);

  CTK_WIDGET_CLASS (ctk_menu_item_parent_class)->destroy (widget);
}

static void
ctk_menu_item_detacher (CtkWidget *widget,
                        CtkMenu   *menu)
{
  CtkMenuItem *menu_item = CTK_MENU_ITEM (widget);
  CtkMenuItemPrivate *priv = menu_item->priv;

  g_return_if_fail (priv->submenu == (CtkWidget*) menu);

  priv->submenu = NULL;
  g_clear_object (&priv->arrow_gadget);
}

static void
ctk_menu_item_buildable_interface_init (CtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->add_child = ctk_menu_item_buildable_add_child;
  iface->custom_finished = ctk_menu_item_buildable_custom_finished;
}

static void
ctk_menu_item_buildable_add_child (CtkBuildable *buildable,
                                   CtkBuilder   *builder,
                                   GObject      *child,
                                   const gchar  *type)
{
  if (type && strcmp (type, "submenu") == 0)
        ctk_menu_item_set_submenu (CTK_MENU_ITEM (buildable),
                                   CTK_WIDGET (child));
  else
    parent_buildable_iface->add_child (buildable, builder, child, type);
}


static void
ctk_menu_item_buildable_custom_finished (CtkBuildable *buildable,
                                         CtkBuilder   *builder,
                                         GObject      *child,
                                         const gchar  *tagname,
                                         gpointer      user_data)
{
  CtkWidget *toplevel;

  if (strcmp (tagname, "accelerator") == 0)
    {
      CtkMenuShell *menu_shell;
      CtkWidget *attach;

      menu_shell = CTK_MENU_SHELL (ctk_widget_get_parent (CTK_WIDGET (buildable)));
      if (menu_shell)
        {
          while (CTK_IS_MENU (menu_shell) &&
                 (attach = ctk_menu_get_attach_widget (CTK_MENU (menu_shell))) != NULL)
            menu_shell = CTK_MENU_SHELL (ctk_widget_get_parent (attach));

          toplevel = ctk_widget_get_toplevel (CTK_WIDGET (menu_shell));
        }
      else
        {
          /* Fall back to something ... */
          toplevel = ctk_widget_get_toplevel (CTK_WIDGET (buildable));

          g_warning ("found a CtkMenuItem '%s' without a parent CtkMenuShell, assigned accelerators wont work.",
                     ctk_buildable_get_name (buildable));
        }

      /* Feed the correct toplevel to the CtkWidget accelerator parsing code */
      _ctk_widget_buildable_finish_accelerator (CTK_WIDGET (buildable), toplevel, user_data);
    }
  else
    parent_buildable_iface->custom_finished (buildable, builder, child, tagname, user_data);
}


static void
ctk_menu_item_activatable_interface_init (CtkActivatableIface *iface)
{
  iface->update = ctk_menu_item_update;
  iface->sync_action_properties = ctk_menu_item_sync_action_properties;
}

static void
activatable_update_label (CtkMenuItem *menu_item, CtkAction *action)
{
  CtkWidget *child;

  child = ctk_bin_get_child (CTK_BIN (menu_item));

  if (CTK_IS_LABEL (child))
    {
      const gchar *label;

      label = ctk_action_get_label (action);
      ctk_menu_item_set_label (menu_item, label);
    }
}

/*
 * ctk_menu_is_empty:
 * @menu: (allow-none): a #CtkMenu or %NULL
 * 
 * Determines whether @menu is empty. A menu is considered empty if it
 * the only visible children are tearoff menu items or “filler” menu 
 * items which were inserted to mark the menu as empty.
 * 
 * This function is used by #CtkAction.
 *
 * Returns: whether @menu is empty.
 **/
static gboolean
ctk_menu_is_empty (CtkWidget *menu)
{
  GList *children, *cur;
  gboolean result = TRUE;

  g_return_val_if_fail (menu == NULL || CTK_IS_MENU (menu), TRUE);

  if (!menu)
    return FALSE;

  children = ctk_container_get_children (CTK_CONTAINER (menu));

  cur = children;
  while (cur) 
    {
      if (ctk_widget_get_visible (cur->data))
	{
	  if (!CTK_IS_TEAROFF_MENU_ITEM (cur->data) &&
	      !g_object_get_data (cur->data, "ctk-empty-menu-item"))
            {
	      result = FALSE;
              break;
            }
	}
      cur = cur->next;
    }
  g_list_free (children);

  return result;
}


static void
ctk_menu_item_update (CtkActivatable *activatable,
                      CtkAction      *action,
                      const gchar    *property_name)
{
  CtkMenuItem *menu_item = CTK_MENU_ITEM (activatable);
  CtkMenuItemPrivate *priv = menu_item->priv;

  if (strcmp (property_name, "visible") == 0)
    {
      _ctk_action_sync_menu_visible (action, CTK_WIDGET (menu_item),
                                     ctk_menu_is_empty (ctk_menu_item_get_submenu (menu_item)));
    }
  else if (strcmp (property_name, "sensitive") == 0)
    {
      ctk_widget_set_sensitive (CTK_WIDGET (menu_item), ctk_action_is_sensitive (action));
    }
  else if (priv->use_action_appearance)
    {
      if (strcmp (property_name, "label") == 0)
        activatable_update_label (menu_item, action);
    }
}

static void
ctk_menu_item_sync_action_properties (CtkActivatable *activatable,
                                      CtkAction      *action)
{
  CtkMenuItem *menu_item = CTK_MENU_ITEM (activatable);
  CtkMenuItemPrivate *priv = menu_item->priv;
  CtkWidget *label;

  if (!priv->use_action_appearance || !action)
    {
      label = ctk_bin_get_child (CTK_BIN (menu_item));

      if (CTK_IS_ACCEL_LABEL (label))
        ctk_accel_label_set_accel_widget (CTK_ACCEL_LABEL (label), CTK_WIDGET (menu_item));
    }

  if (!action)
    return;

  _ctk_action_sync_menu_visible (action, CTK_WIDGET (menu_item),
                                 ctk_menu_is_empty (ctk_menu_item_get_submenu (menu_item)));

  ctk_widget_set_sensitive (CTK_WIDGET (menu_item), ctk_action_is_sensitive (action));

  if (priv->use_action_appearance)
    {
      label = ctk_bin_get_child (CTK_BIN (menu_item));

      /* make sure label is a label, deleting it otherwise */
      if (label && !CTK_IS_LABEL (label))
        {
          ctk_container_remove (CTK_CONTAINER (menu_item), label);
          label = NULL;
        }
      /* Make sure that menu_item has a label and that any
       * accelerators are set */
      ctk_menu_item_ensure_label (menu_item);
      ctk_menu_item_set_use_underline (menu_item, TRUE);
      /* Make label point to the menu_item's label */
      label = ctk_bin_get_child (CTK_BIN (menu_item));

      if (CTK_IS_ACCEL_LABEL (label) && ctk_action_get_accel_path (action))
        {
          ctk_accel_label_set_accel_widget (CTK_ACCEL_LABEL (label), NULL);
          ctk_accel_label_set_accel_closure (CTK_ACCEL_LABEL (label),
                                             ctk_action_get_accel_closure (action));
        }

      activatable_update_label (menu_item, action);
    }
}

static void
ctk_menu_item_set_related_action (CtkMenuItem *menu_item,
                                  CtkAction   *action)
{
    CtkMenuItemPrivate *priv = menu_item->priv;

    if (priv->action == action)
      return;

    G_GNUC_BEGIN_IGNORE_DEPRECATIONS;

    if (priv->action)
      {
        ctk_action_disconnect_accelerator (priv->action);
      }

    if (action)
      {
        const gchar *accel_path;

        accel_path = ctk_action_get_accel_path (action);
        if (accel_path)
          {
            ctk_action_connect_accelerator (action);
            ctk_menu_item_set_accel_path (menu_item, accel_path);
          }
      }

    ctk_activatable_do_set_related_action (CTK_ACTIVATABLE (menu_item), action);

    G_GNUC_END_IGNORE_DEPRECATIONS;

    priv->action = action;
}

static void
ctk_menu_item_set_use_action_appearance (CtkMenuItem *menu_item,
                                         gboolean     use_appearance)
{
    CtkMenuItemPrivate *priv = menu_item->priv;

    if (priv->use_action_appearance != use_appearance)
      {
        priv->use_action_appearance = use_appearance;

        G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
        ctk_activatable_sync_action_properties (CTK_ACTIVATABLE (menu_item), priv->action);
        G_GNUC_END_IGNORE_DEPRECATIONS;
      }
}

static void
update_node_classes (CtkMenuItem *menu_item)
{
  CtkMenuItemPrivate *priv = menu_item->priv;
  CtkCssNode *arrow_node, *widget_node, *node;

  if (!priv->arrow_gadget)
    return;

  arrow_node = ctk_css_gadget_get_node (priv->arrow_gadget);
  widget_node = ctk_widget_get_css_node (CTK_WIDGET (menu_item));

  ctk_css_node_set_state (arrow_node, ctk_css_node_get_state (widget_node));

  if (ctk_widget_get_direction (CTK_WIDGET (menu_item)) == CTK_TEXT_DIR_RTL)
    {
      ctk_css_node_add_class (arrow_node, g_quark_from_static_string (CTK_STYLE_CLASS_LEFT));
      ctk_css_node_remove_class (arrow_node, g_quark_from_static_string (CTK_STYLE_CLASS_RIGHT));

      node = ctk_css_node_get_first_child (widget_node);
      if (node != arrow_node)
        ctk_css_node_insert_before (widget_node, arrow_node, node);
    }
  else
    {
      ctk_css_node_remove_class (arrow_node, g_quark_from_static_string (CTK_STYLE_CLASS_LEFT));
      ctk_css_node_add_class (arrow_node, g_quark_from_static_string (CTK_STYLE_CLASS_RIGHT));

      node = ctk_css_node_get_last_child (widget_node);
      if (node != arrow_node)
        ctk_css_node_insert_after (widget_node, arrow_node, node);
    }
}

static void
update_arrow_gadget (CtkMenuItem *menu_item)
{
  CtkMenuItemPrivate *priv = menu_item->priv;
  CtkWidget *widget = CTK_WIDGET (menu_item);
  gboolean should_have_gadget;

  should_have_gadget = priv->reserve_indicator ||
    (priv->submenu && !CTK_IS_MENU_BAR (ctk_widget_get_parent (widget)));

  if (should_have_gadget)
    {
      if (!priv->arrow_gadget)
        {
          priv->arrow_gadget = ctk_builtin_icon_new ("arrow",
                                                     widget,
                                                     priv->gadget,
                                                     NULL);
          update_node_classes (menu_item);
        }
    }
  else
    {
      g_clear_object (&priv->arrow_gadget);
    }
}

/**
 * ctk_menu_item_set_submenu:
 * @menu_item: a #CtkMenuItem
 * @submenu: (allow-none) (type Ctk.Menu): the submenu, or %NULL
 *
 * Sets or replaces the menu item’s submenu, or removes it when a %NULL
 * submenu is passed.
 */
void
ctk_menu_item_set_submenu (CtkMenuItem *menu_item,
                           CtkWidget   *submenu)
{
  CtkWidget *widget;
  CtkMenuItemPrivate *priv;

  g_return_if_fail (CTK_IS_MENU_ITEM (menu_item));
  g_return_if_fail (submenu == NULL || CTK_IS_MENU (submenu));

  widget = CTK_WIDGET (menu_item);
  priv = menu_item->priv;

  if (priv->submenu != submenu)
    {
      if (priv->submenu)
        {
          ctk_menu_detach (CTK_MENU (priv->submenu));
          priv->submenu = NULL;
        }

      if (submenu)
        {
          priv->submenu = submenu;
          ctk_menu_attach_to_widget (CTK_MENU (submenu),
                                     widget,
                                     ctk_menu_item_detacher);
        }

      update_arrow_gadget (menu_item);

      if (ctk_widget_get_parent (widget))
        ctk_widget_queue_resize (widget);

      g_object_notify_by_pspec (G_OBJECT (menu_item), menu_item_props[PROP_SUBMENU]);
    }
}

/**
 * ctk_menu_item_get_submenu:
 * @menu_item: a #CtkMenuItem
 *
 * Gets the submenu underneath this menu item, if any.
 * See ctk_menu_item_set_submenu().
 *
 * Returns: (nullable) (transfer none): submenu for this menu item, or %NULL if none
 */
CtkWidget *
ctk_menu_item_get_submenu (CtkMenuItem *menu_item)
{
  g_return_val_if_fail (CTK_IS_MENU_ITEM (menu_item), NULL);

  return menu_item->priv->submenu;
}

void _ctk_menu_item_set_placement (CtkMenuItem         *menu_item,
                                   CtkSubmenuPlacement  placement);

void
_ctk_menu_item_set_placement (CtkMenuItem         *menu_item,
                             CtkSubmenuPlacement  placement)
{
  g_return_if_fail (CTK_IS_MENU_ITEM (menu_item));

  menu_item->priv->submenu_placement = placement;
}

/**
 * ctk_menu_item_select:
 * @menu_item: the menu item
 *
 * Emits the #CtkMenuItem::select signal on the given item.
 */
void
ctk_menu_item_select (CtkMenuItem *menu_item)
{
  g_return_if_fail (CTK_IS_MENU_ITEM (menu_item));

  g_signal_emit (menu_item, menu_item_signals[SELECT], 0);
}

/**
 * ctk_menu_item_deselect:
 * @menu_item: the menu item
 *
 * Emits the #CtkMenuItem::deselect signal on the given item.
 */
void
ctk_menu_item_deselect (CtkMenuItem *menu_item)
{
  g_return_if_fail (CTK_IS_MENU_ITEM (menu_item));

  g_signal_emit (menu_item, menu_item_signals[DESELECT], 0);
}

/**
 * ctk_menu_item_activate:
 * @menu_item: the menu item
 *
 * Emits the #CtkMenuItem::activate signal on the given item
 */
void
ctk_menu_item_activate (CtkMenuItem *menu_item)
{
  g_return_if_fail (CTK_IS_MENU_ITEM (menu_item));

  g_signal_emit (menu_item, menu_item_signals[ACTIVATE], 0);
}

/**
 * ctk_menu_item_toggle_size_request:
 * @menu_item: the menu item
 * @requisition: (inout): the requisition to use as signal data.
 *
 * Emits the #CtkMenuItem::toggle-size-request signal on the given item.
 */
void
ctk_menu_item_toggle_size_request (CtkMenuItem *menu_item,
                                   gint        *requisition)
{
  g_return_if_fail (CTK_IS_MENU_ITEM (menu_item));

  g_signal_emit (menu_item, menu_item_signals[TOGGLE_SIZE_REQUEST], 0, requisition);
}

/**
 * ctk_menu_item_toggle_size_allocate:
 * @menu_item: the menu item.
 * @allocation: the allocation to use as signal data.
 *
 * Emits the #CtkMenuItem::toggle-size-allocate signal on the given item.
 */
void
ctk_menu_item_toggle_size_allocate (CtkMenuItem *menu_item,
                                    gint         allocation)
{
  g_return_if_fail (CTK_IS_MENU_ITEM (menu_item));

  g_signal_emit (menu_item, menu_item_signals[TOGGLE_SIZE_ALLOCATE], 0, allocation);
}

static void
ctk_menu_item_realize (CtkWidget *widget)
{
  CtkMenuItem *menu_item = CTK_MENU_ITEM (widget);
  CtkMenuItemPrivate *priv = menu_item->priv;
  CtkAllocation allocation;
  CdkWindow *window;
  CdkWindowAttr attributes;
  gint attributes_mask;

  ctk_widget_set_realized (widget, TRUE);

  window = ctk_widget_get_parent_window (widget);
  ctk_widget_set_window (widget, window);
  g_object_ref (window);

  ctk_widget_get_allocation (widget, &allocation);

  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.window_type = CDK_WINDOW_CHILD;
  attributes.wclass = CDK_INPUT_ONLY;
  attributes.event_mask = (ctk_widget_get_events (widget) |
                           CDK_BUTTON_PRESS_MASK |
                           CDK_BUTTON_RELEASE_MASK |
                           CDK_ENTER_NOTIFY_MASK |
                           CDK_LEAVE_NOTIFY_MASK |
                           CDK_POINTER_MOTION_MASK);

  attributes_mask = CDK_WA_X | CDK_WA_Y;

  priv->event_window = cdk_window_new (ctk_widget_get_parent_window (widget),
                                       &attributes, attributes_mask);
  ctk_widget_register_window (widget, priv->event_window);
}

static void
ctk_menu_item_unrealize (CtkWidget *widget)
{
  CtkMenuItem *menu_item = CTK_MENU_ITEM (widget);
  CtkMenuItemPrivate *priv = menu_item->priv;

  ctk_widget_unregister_window (widget, priv->event_window);
  cdk_window_destroy (priv->event_window);
  priv->event_window = NULL;

  CTK_WIDGET_CLASS (ctk_menu_item_parent_class)->unrealize (widget);
}

static void
ctk_menu_item_map (CtkWidget *widget)
{
  CtkMenuItem *menu_item = CTK_MENU_ITEM (widget);
  CtkMenuItemPrivate *priv = menu_item->priv;

  CTK_WIDGET_CLASS (ctk_menu_item_parent_class)->map (widget);

  cdk_window_show (priv->event_window);
}

static void
ctk_menu_item_unmap (CtkWidget *widget)
{
  CtkMenuItem *menu_item = CTK_MENU_ITEM (widget);
  CtkMenuItemPrivate *priv = menu_item->priv;

  cdk_window_hide (priv->event_window);

  CTK_WIDGET_CLASS (ctk_menu_item_parent_class)->unmap (widget);
}

static gboolean
ctk_menu_item_enter (CtkWidget        *widget,
                     CdkEventCrossing *event)
{
  g_return_val_if_fail (event != NULL, FALSE);

  return ctk_widget_event (ctk_widget_get_parent (widget), (CdkEvent *) event);
}

static gboolean
ctk_menu_item_leave (CtkWidget        *widget,
                     CdkEventCrossing *event)
{
  g_return_val_if_fail (event != NULL, FALSE);

  return ctk_widget_event (ctk_widget_get_parent (widget), (CdkEvent*) event);
}

static void
ctk_real_menu_item_select (CtkMenuItem *menu_item)
{
  CtkMenuItemPrivate *priv = menu_item->priv;
  CdkDevice *source_device = NULL;
  CdkEvent *current_event;

  current_event = ctk_get_current_event ();

  if (current_event)
    {
      source_device = cdk_event_get_source_device (current_event);
      cdk_event_free (current_event);
    }

  if ((!source_device ||
       cdk_device_get_source (source_device) != CDK_SOURCE_TOUCHSCREEN) &&
      priv->submenu &&
      (!ctk_widget_get_mapped (priv->submenu) ||
       CTK_MENU (priv->submenu)->priv->tearoff_active))
    {
      _ctk_menu_item_popup_submenu (CTK_WIDGET (menu_item), TRUE);
    }

  ctk_widget_set_state_flags (CTK_WIDGET (menu_item),
                              CTK_STATE_FLAG_PRELIGHT, FALSE);
  ctk_widget_queue_draw (CTK_WIDGET (menu_item));
}

static void
ctk_real_menu_item_deselect (CtkMenuItem *menu_item)
{
  CtkMenuItemPrivate *priv = menu_item->priv;

  if (priv->submenu)
    _ctk_menu_item_popdown_submenu (CTK_WIDGET (menu_item));

  ctk_widget_unset_state_flags (CTK_WIDGET (menu_item),
                                CTK_STATE_FLAG_PRELIGHT);
  ctk_widget_queue_draw (CTK_WIDGET (menu_item));
}

static gboolean
ctk_menu_item_mnemonic_activate (CtkWidget *widget,
                                 gboolean   group_cycling)
{
  CtkWidget *parent;

  parent = ctk_widget_get_parent (widget);

  if (CTK_IS_MENU_SHELL (parent))
    _ctk_menu_shell_set_keyboard_mode (CTK_MENU_SHELL (parent), TRUE);

  if (group_cycling &&
      parent &&
      CTK_IS_MENU_SHELL (parent) &&
      CTK_MENU_SHELL (parent)->priv->active)
    {
      ctk_menu_shell_select_item (CTK_MENU_SHELL (parent), widget);
    }
  else
    g_signal_emit (widget, menu_item_signals[ACTIVATE_ITEM], 0);

  return TRUE;
}

static void
ctk_real_menu_item_activate (CtkMenuItem *menu_item)
{
  CtkMenuItemPrivate *priv = menu_item->priv;

  if (priv->action_helper)
    ctk_action_helper_activate (priv->action_helper);

  if (priv->action)
    ctk_action_activate (priv->action);
}


static void
ctk_real_menu_item_activate_item (CtkMenuItem *menu_item)
{
  CtkMenuItemPrivate *priv = menu_item->priv;
  CtkWidget *parent;
  CtkWidget *widget;

  widget = CTK_WIDGET (menu_item);
  parent = ctk_widget_get_parent (widget);

  if (parent && CTK_IS_MENU_SHELL (parent))
    {
      CtkMenuShell *menu_shell = CTK_MENU_SHELL (parent);

      if (priv->submenu == NULL)
        ctk_menu_shell_activate_item (menu_shell, widget, TRUE);
      else
        {
          ctk_menu_shell_select_item (menu_shell, widget);
          _ctk_menu_item_popup_submenu (widget, FALSE);

          ctk_menu_shell_select_first (CTK_MENU_SHELL (priv->submenu), TRUE);
        }
    }
}

static void
ctk_real_menu_item_toggle_size_request (CtkMenuItem *menu_item,
                                        gint        *requisition)
{
  g_return_if_fail (CTK_IS_MENU_ITEM (menu_item));

  *requisition = 0;
}

static void
ctk_real_menu_item_toggle_size_allocate (CtkMenuItem *menu_item,
                                         gint         allocation)
{
  g_return_if_fail (CTK_IS_MENU_ITEM (menu_item));

  menu_item->priv->toggle_size = allocation;
}

static void
ctk_real_menu_item_set_label (CtkMenuItem *menu_item,
                              const gchar *label)
{
  CtkWidget *child;

  ctk_menu_item_ensure_label (menu_item);

  child = ctk_bin_get_child (CTK_BIN (menu_item));
  if (CTK_IS_LABEL (child))
    {
      ctk_label_set_label (CTK_LABEL (child), label ? label : "");

      g_object_notify_by_pspec (G_OBJECT (menu_item), menu_item_props[PROP_LABEL]);
    }
}

static const gchar *
ctk_real_menu_item_get_label (CtkMenuItem *menu_item)
{
  CtkWidget *child;

  ctk_menu_item_ensure_label (menu_item);

  child = ctk_bin_get_child (CTK_BIN (menu_item));
  if (CTK_IS_LABEL (child))
    return ctk_label_get_label (CTK_LABEL (child));

  return NULL;
}

static void
popped_up_cb (CtkMenu            *menu,
              const CdkRectangle *flipped_rect,
              const CdkRectangle *final_rect,
              gboolean            flipped_x,
              gboolean            flipped_y,
              CtkMenuItem        *menu_item)
{
  CtkWidget *parent = ctk_widget_get_parent (CTK_WIDGET (menu_item));
  CtkMenu *parent_menu = CTK_IS_MENU (parent) ? CTK_MENU (parent) : NULL;

  if (parent_menu && CTK_IS_MENU_ITEM (parent_menu->priv->parent_menu_item))
    menu_item->priv->submenu_direction = CTK_MENU_ITEM (parent_menu->priv->parent_menu_item)->priv->submenu_direction;
  else
    {
      /* this case is stateful, do it at most once */
      g_signal_handlers_disconnect_by_func (menu, popped_up_cb, menu_item);
    }

  if (flipped_x)
    {
      switch (menu_item->priv->submenu_direction)
        {
        case CTK_DIRECTION_LEFT:
          menu_item->priv->submenu_direction = CTK_DIRECTION_RIGHT;
          break;

        case CTK_DIRECTION_RIGHT:
          menu_item->priv->submenu_direction = CTK_DIRECTION_LEFT;
          break;
        }
    }
}

static void
ctk_menu_item_real_popup_submenu (CtkWidget      *widget,
                                  const CdkEvent *trigger_event,
                                  gboolean        remember_exact_time)
{
  CtkMenuItem *menu_item = CTK_MENU_ITEM (widget);
  CtkMenuItemPrivate *priv = menu_item->priv;
  CtkSubmenuDirection submenu_direction;
  CtkStyleContext *context;
  CtkBorder parent_padding;
  CtkBorder menu_padding;
  gint horizontal_offset;
  gint vertical_offset;
  CtkWidget *parent;
  CtkMenu *parent_menu;

  parent = ctk_widget_get_parent (widget);
  parent_menu = CTK_IS_MENU (parent) ? CTK_MENU (parent) : NULL;

  if (ctk_widget_is_sensitive (priv->submenu) && parent)
    {
      gboolean take_focus;

      take_focus = ctk_menu_shell_get_take_focus (CTK_MENU_SHELL (parent));
      ctk_menu_shell_set_take_focus (CTK_MENU_SHELL (priv->submenu), take_focus);

      if (remember_exact_time)
        {
          gint64 popup_time;

          popup_time = g_get_monotonic_time ();

          g_object_set_data (G_OBJECT (priv->submenu),
                             "ctk-menu-exact-popup-time", (gpointer) popup_time);
        }
      else
        {
          g_object_set_data (G_OBJECT (priv->submenu),
                             "ctk-menu-exact-popup-time", NULL);
        }

      /* Position the submenu at the menu item if it is mapped.
       * Otherwise, position the submenu at the pointer device.
       */
      if (ctk_widget_get_window (widget))
        {
          switch (priv->submenu_placement)
            {
            case CTK_TOP_BOTTOM:
              g_object_set (priv->submenu,
                            "anchor-hints", (CDK_ANCHOR_FLIP_Y |
                                             CDK_ANCHOR_SLIDE |
                                             CDK_ANCHOR_RESIZE),
                            "menu-type-hint", (priv->from_menubar ?
                                               CDK_WINDOW_TYPE_HINT_DROPDOWN_MENU :
                                               CDK_WINDOW_TYPE_HINT_POPUP_MENU),
                            NULL);

              ctk_menu_popup_at_widget (CTK_MENU (priv->submenu),
                                        widget,
                                        CDK_GRAVITY_SOUTH_WEST,
                                        CDK_GRAVITY_NORTH_WEST,
                                        trigger_event);

              break;

            case CTK_LEFT_RIGHT:
              if (parent_menu && CTK_IS_MENU_ITEM (parent_menu->priv->parent_menu_item))
                submenu_direction = CTK_MENU_ITEM (parent_menu->priv->parent_menu_item)->priv->submenu_direction;
              else
                submenu_direction = priv->submenu_direction;

              g_signal_handlers_disconnect_by_func (priv->submenu, popped_up_cb, menu_item);
              g_signal_connect (priv->submenu, "popped-up", G_CALLBACK (popped_up_cb), menu_item);

              ctk_widget_style_get (priv->submenu,
                                    "horizontal-offset", &horizontal_offset,
                                    "vertical-offset", &vertical_offset,
                                    NULL);

              context = ctk_widget_get_style_context (parent);
              ctk_style_context_get_padding (context, ctk_style_context_get_state (context), &parent_padding);
              context = ctk_widget_get_style_context (priv->submenu);
              ctk_style_context_get_padding (context, ctk_style_context_get_state (context), &menu_padding);

              g_object_set (priv->submenu,
                            "anchor-hints", (CDK_ANCHOR_FLIP_X |
                                             CDK_ANCHOR_SLIDE |
                                             CDK_ANCHOR_RESIZE),
                            "rect-anchor-dy", vertical_offset - menu_padding.top,
                            NULL);

              switch (submenu_direction)
                {
                case CTK_DIRECTION_RIGHT:
                  g_object_set (priv->submenu,
                                "rect-anchor-dx", horizontal_offset + parent_padding.right + menu_padding.left,
                                NULL);

                  ctk_menu_popup_at_widget (CTK_MENU (priv->submenu),
                                            widget,
                                            CDK_GRAVITY_NORTH_EAST,
                                            CDK_GRAVITY_NORTH_WEST,
                                            trigger_event);

                  break;

                case CTK_DIRECTION_LEFT:
                  g_object_set (priv->submenu,
                                "rect-anchor-dx", -(horizontal_offset + parent_padding.left + menu_padding.right),
                                NULL);

                  ctk_menu_popup_at_widget (CTK_MENU (priv->submenu),
                                            widget,
                                            CDK_GRAVITY_NORTH_WEST,
                                            CDK_GRAVITY_NORTH_EAST,
                                            trigger_event);

                  break;
                }

              break;
            }
        }
      else
        ctk_menu_popup_at_pointer (CTK_MENU (priv->submenu), trigger_event);
    }

  /* Enable themeing of the parent menu item depending on whether
   * its submenu is shown or not.
   */
  ctk_widget_queue_draw (widget);
}

typedef struct
{
  CtkMenuItem *menu_item;
  CdkEvent    *trigger_event;
} PopupInfo;

static gint
ctk_menu_item_popup_timeout (gpointer data)
{
  PopupInfo *info = data;
  CtkMenuItem *menu_item = info->menu_item;
  CtkMenuItemPrivate *priv = menu_item->priv;
  CtkWidget *parent;

  parent = ctk_widget_get_parent (CTK_WIDGET (menu_item));

  if ((CTK_IS_MENU_SHELL (parent) && CTK_MENU_SHELL (parent)->priv->active) ||
      (CTK_IS_MENU (parent) && CTK_MENU (parent)->priv->torn_off))
    {
      ctk_menu_item_real_popup_submenu (CTK_WIDGET (menu_item), info->trigger_event, TRUE);
      if (info->trigger_event &&
          info->trigger_event->type != CDK_BUTTON_PRESS &&
          info->trigger_event->type != CDK_ENTER_NOTIFY &&
          priv->submenu)
        CTK_MENU_SHELL (priv->submenu)->priv->ignore_enter = TRUE;
    }

  priv->timer = 0;

  g_clear_pointer (&info->trigger_event, cdk_event_free);
  g_slice_free (PopupInfo, info);

  return FALSE;
}

static gint
get_popup_delay (CtkWidget *widget)
{
  CtkWidget *parent;

  parent = ctk_widget_get_parent (widget);
  if (CTK_IS_MENU_SHELL (parent))
    return _ctk_menu_shell_get_popup_delay (CTK_MENU_SHELL (parent));
  else
    return MENU_POPUP_DELAY;
}

void
_ctk_menu_item_popup_submenu (CtkWidget *widget,
                              gboolean   with_delay)
{
  CtkMenuItem *menu_item = CTK_MENU_ITEM (widget);
  CtkMenuItemPrivate *priv = menu_item->priv;

  if (priv->timer)
    {
      g_source_remove (priv->timer);
      priv->timer = 0;
      with_delay = FALSE;
    }

  if (with_delay)
    {
      gint popup_delay = get_popup_delay (widget);

      if (popup_delay > 0)
        {
          PopupInfo *info = g_slice_new (PopupInfo);

          info->menu_item = menu_item;
          info->trigger_event = ctk_get_current_event ();

          priv->timer = cdk_threads_add_timeout (popup_delay,
                                                 ctk_menu_item_popup_timeout,
                                                 info);
          g_source_set_name_by_id (priv->timer, "[ctk+] ctk_menu_item_popup_timeout");

          return;
        }
    }

  ctk_menu_item_real_popup_submenu (widget, NULL, FALSE);
}

void
_ctk_menu_item_popdown_submenu (CtkWidget *widget)
{
  CtkMenuItem *menu_item = CTK_MENU_ITEM (widget);
  CtkMenuItemPrivate *priv = menu_item->priv;

  if (priv->submenu)
    {
      g_object_set_data (G_OBJECT (priv->submenu),
                         "ctk-menu-exact-popup-time", NULL);

      if (priv->timer)
        {
          g_source_remove (priv->timer);
          priv->timer = 0;
        }
      else
        ctk_menu_popdown (CTK_MENU (priv->submenu));

      ctk_widget_queue_draw (widget);
    }
}

/**
 * ctk_menu_item_set_right_justified:
 * @menu_item: a #CtkMenuItem.
 * @right_justified: if %TRUE the menu item will appear at the
 *   far right if added to a menu bar
 *
 * Sets whether the menu item appears justified at the right
 * side of a menu bar. This was traditionally done for “Help”
 * menu items, but is now considered a bad idea. (If the widget
 * layout is reversed for a right-to-left language like Hebrew
 * or Arabic, right-justified-menu-items appear at the left.)
 *
 * Deprecated: 3.2: If you insist on using it, use
 *   ctk_widget_set_hexpand() and ctk_widget_set_halign().
 **/
void
ctk_menu_item_set_right_justified (CtkMenuItem *menu_item,
                                   gboolean     right_justified)
{
  g_return_if_fail (CTK_IS_MENU_ITEM (menu_item));

  ctk_menu_item_do_set_right_justified (menu_item, right_justified);
}

/**
 * ctk_menu_item_get_right_justified:
 * @menu_item: a #CtkMenuItem
 *
 * Gets whether the menu item appears justified at the right
 * side of the menu bar.
 *
 * Returns: %TRUE if the menu item will appear at the
 *   far right if added to a menu bar.
 *
 * Deprecated: 3.2: See ctk_menu_item_set_right_justified()
 **/
gboolean
ctk_menu_item_get_right_justified (CtkMenuItem *menu_item)
{
  g_return_val_if_fail (CTK_IS_MENU_ITEM (menu_item), FALSE);

  return menu_item->priv->right_justify;
}


static void
ctk_menu_item_show_all (CtkWidget *widget)
{
  CtkMenuItem *menu_item = CTK_MENU_ITEM (widget);
  CtkMenuItemPrivate *priv = menu_item->priv;

  /* show children including submenu */
  if (priv->submenu)
    ctk_widget_show_all (priv->submenu);
  ctk_container_foreach (CTK_CONTAINER (widget), (CtkCallback) ctk_widget_show_all, NULL);

  ctk_widget_show (widget);
}

static gboolean
ctk_menu_item_can_activate_accel (CtkWidget *widget,
                                  guint      signal_id)
{
  CtkWidget *parent;

  parent = ctk_widget_get_parent (widget);

  /* Chain to the parent CtkMenu for further checks */
  return (ctk_widget_is_sensitive (widget) && ctk_widget_get_visible (widget) &&
          parent && ctk_widget_can_activate_accel (parent, signal_id));
}

static void
ctk_menu_item_accel_name_foreach (CtkWidget *widget,
                                  gpointer   data)
{
  const gchar **path_p = data;

  if (!*path_p)
    {
      if (CTK_IS_LABEL (widget))
        {
          *path_p = ctk_label_get_text (CTK_LABEL (widget));
          if (*path_p && (*path_p)[0] == 0)
            *path_p = NULL;
        }
      else if (CTK_IS_CONTAINER (widget))
        ctk_container_foreach (CTK_CONTAINER (widget),
                               ctk_menu_item_accel_name_foreach,
                               data);
    }
}

static void
ctk_menu_item_parent_set (CtkWidget *widget,
                          CtkWidget *previous_parent)
{
  CtkMenuItem *menu_item = CTK_MENU_ITEM (widget);
  CtkMenu *menu;
  CtkWidget *parent;

  parent = ctk_widget_get_parent (widget);
  menu = CTK_IS_MENU (parent) ? CTK_MENU (parent) : NULL;

  if (menu)
    _ctk_menu_item_refresh_accel_path (menu_item,
                                       menu->priv->accel_path,
                                       menu->priv->accel_group,
                                       TRUE);

  update_arrow_gadget (menu_item);

  if (CTK_WIDGET_CLASS (ctk_menu_item_parent_class)->parent_set)
    CTK_WIDGET_CLASS (ctk_menu_item_parent_class)->parent_set (widget, previous_parent);
}

static void
ctk_menu_item_direction_changed (CtkWidget        *widget,
                                 CtkTextDirection  previous_dir)
{
  CtkMenuItem *menu_item = CTK_MENU_ITEM (widget);

  update_node_classes (menu_item);

  CTK_WIDGET_CLASS (ctk_menu_item_parent_class)->direction_changed (widget, previous_dir);
}

void
_ctk_menu_item_refresh_accel_path (CtkMenuItem   *menu_item,
                                   const gchar   *prefix,
                                   CtkAccelGroup *accel_group,
                                   gboolean       group_changed)
{
  CtkMenuItemPrivate *priv = menu_item->priv;
  const gchar *path;
  CtkWidget *widget;

  g_return_if_fail (CTK_IS_MENU_ITEM (menu_item));
  g_return_if_fail (!accel_group || CTK_IS_ACCEL_GROUP (accel_group));

  widget = CTK_WIDGET (menu_item);

  if (!accel_group)
    {
      ctk_widget_set_accel_path (widget, NULL, NULL);
      return;
    }

  path = _ctk_widget_get_accel_path (widget, NULL);
  if (!path)  /* no active accel_path yet */
    {
      path = priv->accel_path;
      if (!path && prefix)
        {
          const gchar *postfix = NULL;
          gchar *new_path;

          /* try to construct one from label text */
          ctk_container_foreach (CTK_CONTAINER (menu_item),
                                 ctk_menu_item_accel_name_foreach,
                                 &postfix);
          if (postfix)
            {
              new_path = g_strconcat (prefix, "/", postfix, NULL);
              path = priv->accel_path = g_intern_string (new_path);
              g_free (new_path);
            }
        }
      if (path)
        ctk_widget_set_accel_path (widget, path, accel_group);
    }
  else if (group_changed)    /* reinstall accelerators */
    ctk_widget_set_accel_path (widget, path, accel_group);
}

/**
 * ctk_menu_item_set_accel_path:
 * @menu_item:  a valid #CtkMenuItem
 * @accel_path: (allow-none): accelerator path, corresponding to this menu
 *     item’s functionality, or %NULL to unset the current path.
 *
 * Set the accelerator path on @menu_item, through which runtime
 * changes of the menu item’s accelerator caused by the user can be
 * identified and saved to persistent storage (see ctk_accel_map_save()
 * on this). To set up a default accelerator for this menu item, call
 * ctk_accel_map_add_entry() with the same @accel_path. See also
 * ctk_accel_map_add_entry() on the specifics of accelerator paths,
 * and ctk_menu_set_accel_path() for a more convenient variant of
 * this function.
 *
 * This function is basically a convenience wrapper that handles
 * calling ctk_widget_set_accel_path() with the appropriate accelerator
 * group for the menu item.
 *
 * Note that you do need to set an accelerator on the parent menu with
 * ctk_menu_set_accel_group() for this to work.
 *
 * Note that @accel_path string will be stored in a #GQuark.
 * Therefore, if you pass a static string, you can save some memory
 * by interning it first with g_intern_static_string().
 */
void
ctk_menu_item_set_accel_path (CtkMenuItem *menu_item,
                              const gchar *accel_path)
{
  CtkMenuItemPrivate *priv = menu_item->priv;
  CtkWidget *parent;
  CtkWidget *widget;

  g_return_if_fail (CTK_IS_MENU_ITEM (menu_item));
  g_return_if_fail (accel_path == NULL ||
                    (accel_path[0] == '<' && strchr (accel_path, '/')));

  widget = CTK_WIDGET (menu_item);

  /* store new path */
  priv->accel_path = g_intern_string (accel_path);

  /* forget accelerators associated with old path */
  ctk_widget_set_accel_path (widget, NULL, NULL);

  /* install accelerators associated with new path */
  parent = ctk_widget_get_parent (widget);
  if (CTK_IS_MENU (parent))
    {
      CtkMenu *menu = CTK_MENU (parent);

      if (menu->priv->accel_group)
        _ctk_menu_item_refresh_accel_path (CTK_MENU_ITEM (widget),
                                           NULL,
                                           menu->priv->accel_group,
                                           FALSE);
    }
}

/**
 * ctk_menu_item_get_accel_path:
 * @menu_item:  a valid #CtkMenuItem
 *
 * Retrieve the accelerator path that was previously set on @menu_item.
 *
 * See ctk_menu_item_set_accel_path() for details.
 *
 * Returns: (nullable) (transfer none): the accelerator path corresponding to
 *     this menu item’s functionality, or %NULL if not set
 *
 * Since: 2.14
 */
const gchar *
ctk_menu_item_get_accel_path (CtkMenuItem *menu_item)
{
  g_return_val_if_fail (CTK_IS_MENU_ITEM (menu_item), NULL);

  return menu_item->priv->accel_path;
}

static void
ctk_menu_item_forall (CtkContainer *container,
                      gboolean      include_internals,
                      CtkCallback   callback,
                      gpointer      callback_data)
{
  CtkWidget *child;

  g_return_if_fail (CTK_IS_MENU_ITEM (container));
  g_return_if_fail (callback != NULL);

  child = ctk_bin_get_child (CTK_BIN (container));
  if (child)
    callback (child, callback_data);
}

gboolean
_ctk_menu_item_is_selectable (CtkWidget *menu_item)
{
  if ((!ctk_bin_get_child (CTK_BIN (menu_item)) &&
       G_OBJECT_TYPE (menu_item) == CTK_TYPE_MENU_ITEM) ||
      CTK_IS_SEPARATOR_MENU_ITEM (menu_item) ||
      !ctk_widget_is_sensitive (menu_item) ||
      !ctk_widget_get_visible (menu_item))
    return FALSE;

  return TRUE;
}

static void
ctk_menu_item_ensure_label (CtkMenuItem *menu_item)
{
  CtkWidget *accel_label;

  if (!ctk_bin_get_child (CTK_BIN (menu_item)))
    {
      accel_label = g_object_new (CTK_TYPE_ACCEL_LABEL, "xalign", 0.0, NULL);
      ctk_widget_set_halign (accel_label, CTK_ALIGN_FILL);
      ctk_widget_set_valign (accel_label, CTK_ALIGN_CENTER);

      ctk_container_add (CTK_CONTAINER (menu_item), accel_label);
      ctk_accel_label_set_accel_widget (CTK_ACCEL_LABEL (accel_label),
                                        CTK_WIDGET (menu_item));
      ctk_widget_show (accel_label);
    }
}

/**
 * ctk_menu_item_set_label:
 * @menu_item: a #CtkMenuItem
 * @label: the text you want to set
 *
 * Sets @text on the @menu_item label
 *
 * Since: 2.16
 */
void
ctk_menu_item_set_label (CtkMenuItem *menu_item,
                         const gchar *label)
{
  g_return_if_fail (CTK_IS_MENU_ITEM (menu_item));

  CTK_MENU_ITEM_GET_CLASS (menu_item)->set_label (menu_item, label);
}

/**
 * ctk_menu_item_get_label:
 * @menu_item: a #CtkMenuItem
 *
 * Sets @text on the @menu_item label
 *
 * Returns: The text in the @menu_item label. This is the internal
 *   string used by the label, and must not be modified.
 *
 * Since: 2.16
 */
const gchar *
ctk_menu_item_get_label (CtkMenuItem *menu_item)
{
  g_return_val_if_fail (CTK_IS_MENU_ITEM (menu_item), NULL);

  return CTK_MENU_ITEM_GET_CLASS (menu_item)->get_label (menu_item);
}

/**
 * ctk_menu_item_set_use_underline:
 * @menu_item: a #CtkMenuItem
 * @setting: %TRUE if underlines in the text indicate mnemonics
 *
 * If true, an underline in the text indicates the next character
 * should be used for the mnemonic accelerator key.
 *
 * Since: 2.16
 */
void
ctk_menu_item_set_use_underline (CtkMenuItem *menu_item,
                                 gboolean     setting)
{
  CtkWidget *child;

  g_return_if_fail (CTK_IS_MENU_ITEM (menu_item));

  ctk_menu_item_ensure_label (menu_item);

  child = ctk_bin_get_child (CTK_BIN (menu_item));
  if (CTK_IS_LABEL (child) &&
      ctk_label_get_use_underline (CTK_LABEL (child)) != setting)
    {
      ctk_label_set_use_underline (CTK_LABEL (child), setting);
      g_object_notify_by_pspec (G_OBJECT (menu_item), menu_item_props[PROP_USE_UNDERLINE]);
    }
}

/**
 * ctk_menu_item_get_use_underline:
 * @menu_item: a #CtkMenuItem
 *
 * Checks if an underline in the text indicates the next character
 * should be used for the mnemonic accelerator key.
 *
 * Returns: %TRUE if an embedded underline in the label
 *     indicates the mnemonic accelerator key.
 *
 * Since: 2.16
 */
gboolean
ctk_menu_item_get_use_underline (CtkMenuItem *menu_item)
{
  CtkWidget *child;

  g_return_val_if_fail (CTK_IS_MENU_ITEM (menu_item), FALSE);

  ctk_menu_item_ensure_label (menu_item);

  child = ctk_bin_get_child (CTK_BIN (menu_item));
  if (CTK_IS_LABEL (child))
    return ctk_label_get_use_underline (CTK_LABEL (child));

  return FALSE;
}

/**
 * ctk_menu_item_set_reserve_indicator:
 * @menu_item: a #CtkMenuItem
 * @reserve: the new value
 *
 * Sets whether the @menu_item should reserve space for
 * the submenu indicator, regardless if it actually has
 * a submenu or not.
 *
 * There should be little need for applications to call
 * this functions.
 *
 * Since: 3.0
 */
void
ctk_menu_item_set_reserve_indicator (CtkMenuItem *menu_item,
                                     gboolean     reserve)
{
  CtkMenuItemPrivate *priv;

  g_return_if_fail (CTK_IS_MENU_ITEM (menu_item));

  priv = menu_item->priv;

  if (priv->reserve_indicator != reserve)
    {
      priv->reserve_indicator = reserve;
      update_arrow_gadget (menu_item);
      ctk_widget_queue_resize (CTK_WIDGET (menu_item));
    }
}

/**
 * ctk_menu_item_get_reserve_indicator:
 * @menu_item: a #CtkMenuItem
 *
 * Returns whether the @menu_item reserves space for
 * the submenu indicator, regardless if it has a submenu
 * or not.
 *
 * Returns: %TRUE if @menu_item always reserves space for the
 *     submenu indicator
 *
 * Since: 3.0
 */
gboolean
ctk_menu_item_get_reserve_indicator (CtkMenuItem *menu_item)
{
  g_return_val_if_fail (CTK_IS_MENU_ITEM (menu_item), FALSE);

  return menu_item->priv->reserve_indicator;
}
