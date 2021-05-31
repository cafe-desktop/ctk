/* GTK - The GIMP Toolkit
 * Copyright © 2014 Red Hat, Inc.
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
#include "ctkpopovermenu.h"
#include "ctkstack.h"
#include "ctkstylecontext.h"
#include "ctkintl.h"


/**
 * SECTION:ctkpopovermenu
 * @Short_description: Popovers to use as menus
 * @Title: CtkPopoverMenu
 *
 * CtkPopoverMenu is a subclass of #CtkPopover that treats its
 * children like menus and allows switching between them. It is
 * meant to be used primarily together with #CtkModelButton, but
 * any widget can be used, such as #CtkSpinButton or #CtkScale.
 * In this respect, CtkPopoverMenu is more flexible than popovers
 * that are created from a #GMenuModel with ctk_popover_new_from_model().
 *
 * To add a child as a submenu, set the #CtkPopoverMenu:submenu
 * child property to the name of the submenu. To let the user open
 * this submenu, add a #CtkModelButton whose #CtkModelButton:menu-name
 * property is set to the name you've given to the submenu.
 *
 * By convention, the first child of a submenu should be a #CtkModelButton
 * to switch back to the parent menu. Such a button should use the
 * #CtkModelButton:inverted and #CtkModelButton:centered properties
 * to achieve a title-like appearance and place the submenu indicator
 * at the opposite side. To switch back to the main menu, use "main"
 * as the menu name.
 *
 * # Example
 *
 * |[
 * <object class="CtkPopoverMenu">
 *   <child>
 *     <object class="CtkBox">
 *       <property name="visible">True</property>
 *       <property name="margin">10</property>
 *       <child>
 *         <object class="CtkModelButton">
 *           <property name="visible">True</property>
 *           <property name="action-name">win.frob</property>
 *           <property name="text" translatable="yes">Frob</property>
 *         </object>
 *       </child>
 *       <child>
 *         <object class="CtkModelButton">
 *           <property name="visible">True</property>
 *           <property name="menu-name">more</property>
 *           <property name="text" translatable="yes">More</property>
 *         </object>
 *       </child>
 *     </object>
 *   </child>
 *   <child>
 *     <object class="CtkBox">
 *       <property name="visible">True</property>
 *       <property name="margin">10</property>
 *       <child>
 *         <object class="CtkModelButton">
 *           <property name="visible">True</property>
 *           <property name="action-name">win.foo</property>
 *           <property name="text" translatable="yes">Foo</property>
 *         </object>
 *       </child>
 *       <child>
 *         <object class="CtkModelButton">
 *           <property name="visible">True</property>
 *           <property name="action-name">win.bar</property>
 *           <property name="text" translatable="yes">Bar</property>
 *         </object>
 *       </child>
 *     </object>
 *     <packing>
 *       <property name="submenu">more</property>
 *     </packing>
 *   </child>
 * </object>
 * ]|
 *
 * Just like normal popovers created using ctk_popover_new_from_model,
 * #CtkPopoverMenu instances have a single css node called "popover"
 * and get the .menu style class.
 */

struct _CtkPopoverMenu
{
  CtkPopover parent_instance;
};

enum {
  PROP_VISIBLE_SUBMENU = 1
};

enum {
  CHILD_PROP_SUBMENU = 1,
  CHILD_PROP_POSITION
};

G_DEFINE_TYPE (CtkPopoverMenu, ctk_popover_menu, CTK_TYPE_POPOVER)

static void
visible_submenu_changed (GObject        *object,
                         GParamSpec     *pspec,
                         CtkPopoverMenu *popover)
{
  g_object_notify (G_OBJECT (popover), "visible-submenu");
}

static void
ctk_popover_menu_init (CtkPopoverMenu *popover)
{
  CtkWidget *stack;
  CtkStyleContext *style_context;

  stack = ctk_stack_new ();
  ctk_stack_set_vhomogeneous (CTK_STACK (stack), FALSE);
  ctk_stack_set_transition_type (CTK_STACK (stack), CTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
  ctk_stack_set_interpolate_size (CTK_STACK (stack), TRUE);
  ctk_widget_show (stack);
  ctk_container_add (CTK_CONTAINER (popover), stack);
  g_signal_connect (stack, "notify::visible-child-name",
                    G_CALLBACK (visible_submenu_changed), popover);

  style_context = ctk_widget_get_style_context (CTK_WIDGET (popover));
  ctk_style_context_add_class (style_context, CTK_STYLE_CLASS_MENU);
}

static void
ctk_popover_menu_map (CtkWidget *widget)
{
  CTK_WIDGET_CLASS (ctk_popover_menu_parent_class)->map (widget);
  ctk_popover_menu_open_submenu (CTK_POPOVER_MENU (widget), "main");
}

static void
ctk_popover_menu_unmap (CtkWidget *widget)
{
  ctk_popover_menu_open_submenu (CTK_POPOVER_MENU (widget), "main");
  CTK_WIDGET_CLASS (ctk_popover_menu_parent_class)->unmap (widget);
}

static void
ctk_popover_menu_add (CtkContainer *container,
                      CtkWidget    *child)
{
  CtkWidget *stack;

  stack = ctk_bin_get_child (CTK_BIN (container));

  if (stack == NULL)
    {
      ctk_widget_set_parent (child, CTK_WIDGET (container));
      _ctk_bin_set_child (CTK_BIN (container), child);
    }
  else
    {
      gchar *name;

      if (ctk_stack_get_child_by_name (CTK_STACK (stack), "main"))
        name = "submenu";
      else
        name = "main";

      ctk_stack_add_named (CTK_STACK (stack), child, name);
    }
}

static void
ctk_popover_menu_remove (CtkContainer *container,
                         CtkWidget    *child)
{
  CtkWidget *stack;

  stack = ctk_bin_get_child (CTK_BIN (container));

  if (child == stack)
    CTK_CONTAINER_CLASS (ctk_popover_menu_parent_class)->remove (container, child);
  else
    ctk_container_remove (CTK_CONTAINER (stack), child);
}

static void
ctk_popover_menu_forall (CtkContainer *container,
                         gboolean      include_internals,
                         CtkCallback   callback,
                         gpointer      callback_data)
{
  CtkWidget *stack;

  stack = ctk_bin_get_child (CTK_BIN (container));

  if (include_internals)
    (* callback) (stack, callback_data);

  ctk_container_forall (CTK_CONTAINER (stack), callback, callback_data);
}

static void
ctk_popover_menu_get_child_property (CtkContainer *container,
                                     CtkWidget    *child,
                                     guint         property_id,
                                     GValue       *value,
                                     GParamSpec   *pspec)
{
  CtkWidget *stack;

  stack = ctk_bin_get_child (CTK_BIN (container));

  if (child == stack)
    return;

  switch (property_id)
    {
    case CHILD_PROP_SUBMENU:
      {
        gchar *name;
        ctk_container_child_get (CTK_CONTAINER (stack), child, "name", &name, NULL);
        g_value_set_string (value, name);
      }
      break;

    case CHILD_PROP_POSITION:
      {
        gint position;
        ctk_container_child_get (CTK_CONTAINER (stack), child, "position", &position, NULL);
        g_value_set_int (value, position);
      }
      break;

    default:
      CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec); 
      break;
    }
}

static void
ctk_popover_menu_set_child_property (CtkContainer *container,
                                     CtkWidget    *child,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  CtkWidget *stack;

  stack = ctk_bin_get_child (CTK_BIN (container));

  if (child == stack)
    return;

  switch (property_id)
    {
    case CHILD_PROP_SUBMENU:
      {
        const gchar *name;
        name = g_value_get_string (value);
        ctk_container_child_set (CTK_CONTAINER (stack), child, "name", name, NULL);
      }
      break;

    case CHILD_PROP_POSITION:
      {
        gint position;
        position = g_value_get_int (value);
        ctk_container_child_set (CTK_CONTAINER (stack), child, "position", position, NULL);
      }
      break;

    default:
      CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec); 
      break;
    }
}

static void
ctk_popover_menu_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  CtkWidget *stack;

  stack = ctk_bin_get_child (CTK_BIN (object));

  switch (property_id)
    {
    case PROP_VISIBLE_SUBMENU:
      g_value_set_string (value, ctk_stack_get_visible_child_name (CTK_STACK (stack)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ctk_popover_menu_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  CtkWidget *stack;

  stack = ctk_bin_get_child (CTK_BIN (object));

  switch (property_id)
    {
    case PROP_VISIBLE_SUBMENU:
      ctk_stack_set_visible_child_name (CTK_STACK (stack), g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ctk_popover_menu_class_init (CtkPopoverMenuClass *klass)
{
  CtkContainerClass *container_class = CTK_CONTAINER_CLASS (klass);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = ctk_popover_menu_set_property;
  object_class->get_property = ctk_popover_menu_get_property;

  widget_class->map = ctk_popover_menu_map;
  widget_class->unmap = ctk_popover_menu_unmap;

  container_class->add = ctk_popover_menu_add;
  container_class->remove = ctk_popover_menu_remove;
  container_class->forall = ctk_popover_menu_forall;
  container_class->set_child_property = ctk_popover_menu_set_child_property;
  container_class->get_child_property = ctk_popover_menu_get_child_property;

  g_object_class_install_property (object_class,
                                   PROP_VISIBLE_SUBMENU,
                                   g_param_spec_string ("visible-submenu",
                                                        P_("Visible submenu"),
                                                        P_("The name of the visible submenu"),
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * CtkPopoverMenu:submenu:
   *
   * The submenu child property specifies the name of the submenu
   * If it is %NULL or "main", the child is used as the main menu,
   * which is shown initially when the popover is mapped.
   *
   * Since: 3.16
   */
  ctk_container_class_install_child_property (container_class,
                                              CHILD_PROP_SUBMENU,
                                              g_param_spec_string ("submenu",
                                                                   P_("Submenu"),
                                                                   P_("The name of the submenu"),
                                                                   NULL,
                                                                   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  ctk_container_class_install_child_property (container_class,
                                              CHILD_PROP_POSITION,
                                              g_param_spec_int ("position",
                                                                P_("Position"),
                                                                P_("The index of the child in the parent"),
                                                                -1, G_MAXINT, 0,
                                                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

/**
 * ctk_popover_menu_new:
 *
 * Creates a new popover menu.
 *
 * Returns: a new #CtkPopoverMenu
 *
 * Since: 3.16
 */
CtkWidget *
ctk_popover_menu_new (void)
{
  return g_object_new (CTK_TYPE_POPOVER_MENU, NULL);
}

/**
 * ctk_popover_menu_open_submenu:
 * @popover: a #CtkPopoverMenu
 * @name: the name of the menu to switch to
 *
 * Opens a submenu of the @popover. The @name
 * must be one of the names given to the submenus
 * of @popover with #CtkPopoverMenu:submenu, or
 * "main" to switch back to the main menu.
 *
 * #CtkModelButton will open submenus automatically
 * when the #CtkModelButton:menu-name property is set,
 * so this function is only needed when you are using
 * other kinds of widgets to initiate menu changes.
 *
 * Since: 3.16
 */
void
ctk_popover_menu_open_submenu (CtkPopoverMenu *popover,
                               const gchar    *name)
{
  CtkWidget *stack;

  g_return_if_fail (CTK_IS_POPOVER_MENU (popover));

  stack = ctk_bin_get_child (CTK_BIN (popover));
  ctk_stack_set_visible_child_name (CTK_STACK (stack), name);
}
