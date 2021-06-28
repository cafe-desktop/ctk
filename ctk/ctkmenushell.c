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
 * SECTION:ctkmenushell
 * @Title: CtkMenuShell
 * @Short_description: A base class for menu objects
 *
 * A #CtkMenuShell is the abstract base class used to derive the
 * #CtkMenu and #CtkMenuBar subclasses.
 *
 * A #CtkMenuShell is a container of #CtkMenuItem objects arranged
 * in a list which can be navigated, selected, and activated by the
 * user to perform application functions. A #CtkMenuItem can have a
 * submenu associated with it, allowing for nested hierarchical menus.
 *
 * # Terminology
 *
 * A menu item can be “selected”, this means that it is displayed
 * in the prelight state, and if it has a submenu, that submenu
 * will be popped up.
 *
 * A menu is “active” when it is visible onscreen and the user
 * is selecting from it. A menubar is not active until the user
 * clicks on one of its menuitems. When a menu is active,
 * passing the mouse over a submenu will pop it up.
 *
 * There is also is a concept of the current menu and a current
 * menu item. The current menu item is the selected menu item
 * that is furthest down in the hierarchy. (Every active menu shell
 * does not necessarily contain a selected menu item, but if
 * it does, then the parent menu shell must also contain
 * a selected menu item.) The current menu is the menu that
 * contains the current menu item. It will always have a CTK
 * grab and receive all key presses.
 */
#include "config.h"

#include "ctkbindings.h"
#include "ctkkeyhash.h"
#include "ctklabel.h"
#include "ctkmain.h"
#include "ctkmarshalers.h"
#include "ctkmenubar.h"
#include "ctkmenuitemprivate.h"
#include "ctkmenushellprivate.h"
#include "ctkmnemonichash.h"
#include "ctkrender.h"
#include "ctkwindow.h"
#include "ctkwindowprivate.h"
#include "ctkprivate.h"
#include "ctkmain.h"
#include "ctkintl.h"
#include "ctktypebuiltins.h"
#include "ctkmodelmenuitem.h"
#include "ctkwidgetprivate.h"
#include "ctklabelprivate.h"

#include "deprecated/ctktearoffmenuitem.h"

#include "a11y/ctkmenushellaccessible.h"


#define MENU_SHELL_TIMEOUT   500
#define MENU_POPUP_DELAY     225
#define MENU_POPDOWN_DELAY   1000

#define PACK_DIRECTION(m)                                 \
   (CTK_IS_MENU_BAR (m)                                   \
     ? ctk_menu_bar_get_pack_direction (CTK_MENU_BAR (m)) \
     : CTK_PACK_DIRECTION_LTR)

enum {
  DEACTIVATE,
  SELECTION_DONE,
  MOVE_CURRENT,
  ACTIVATE_CURRENT,
  CANCEL,
  CYCLE_FOCUS,
  MOVE_SELECTED,
  INSERT,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_TAKE_FOCUS
};


static void ctk_menu_shell_set_property      (GObject           *object,
                                              guint              prop_id,
                                              const GValue      *value,
                                              GParamSpec        *pspec);
static void ctk_menu_shell_get_property      (GObject           *object,
                                              guint              prop_id,
                                              GValue            *value,
                                              GParamSpec        *pspec);
static void ctk_menu_shell_realize           (CtkWidget         *widget);
static void ctk_menu_shell_finalize          (GObject           *object);
static void ctk_menu_shell_dispose           (GObject           *object);
static gint ctk_menu_shell_button_press      (CtkWidget         *widget,
                                              GdkEventButton    *event);
static gint ctk_menu_shell_button_release    (CtkWidget         *widget,
                                              GdkEventButton    *event);
static gint ctk_menu_shell_key_press         (CtkWidget         *widget,
                                              GdkEventKey       *event);
static gint ctk_menu_shell_enter_notify      (CtkWidget         *widget,
                                              GdkEventCrossing  *event);
static gint ctk_menu_shell_leave_notify      (CtkWidget         *widget,
                                              GdkEventCrossing  *event);
static void ctk_menu_shell_screen_changed    (CtkWidget         *widget,
                                              GdkScreen         *previous_screen);
static gboolean ctk_menu_shell_grab_broken       (CtkWidget         *widget,
                                              GdkEventGrabBroken *event);
static void ctk_menu_shell_add               (CtkContainer      *container,
                                              CtkWidget         *widget);
static void ctk_menu_shell_remove            (CtkContainer      *container,
                                              CtkWidget         *widget);
static void ctk_menu_shell_forall            (CtkContainer      *container,
                                              gboolean           include_internals,
                                              CtkCallback        callback,
                                              gpointer           callback_data);
static void ctk_menu_shell_real_insert       (CtkMenuShell *menu_shell,
                                              CtkWidget    *child,
                                              gint          position);
static void ctk_real_menu_shell_deactivate   (CtkMenuShell      *menu_shell);
static gint ctk_menu_shell_is_item           (CtkMenuShell      *menu_shell,
                                              CtkWidget         *child);
static CtkWidget *ctk_menu_shell_get_item    (CtkMenuShell      *menu_shell,
                                              GdkEvent          *event);
static GType    ctk_menu_shell_child_type  (CtkContainer      *container);
static void ctk_menu_shell_real_select_item  (CtkMenuShell      *menu_shell,
                                              CtkWidget         *menu_item);
static gboolean ctk_menu_shell_select_submenu_first (CtkMenuShell   *menu_shell); 

static void ctk_real_menu_shell_move_current (CtkMenuShell      *menu_shell,
                                              CtkMenuDirectionType direction);
static void ctk_real_menu_shell_activate_current (CtkMenuShell      *menu_shell,
                                                  gboolean           force_hide);
static void ctk_real_menu_shell_cancel           (CtkMenuShell      *menu_shell);
static void ctk_real_menu_shell_cycle_focus      (CtkMenuShell      *menu_shell,
                                                  CtkDirectionType   dir);

static void     ctk_menu_shell_reset_key_hash    (CtkMenuShell *menu_shell);
static gboolean ctk_menu_shell_activate_mnemonic (CtkMenuShell *menu_shell,
                                                  GdkEventKey  *event);
static gboolean ctk_menu_shell_real_move_selected (CtkMenuShell  *menu_shell, 
                                                   gint           distance);

static guint menu_shell_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (CtkMenuShell, ctk_menu_shell, CTK_TYPE_CONTAINER)

static void
ctk_menu_shell_class_init (CtkMenuShellClass *klass)
{
  GObjectClass *object_class;
  CtkWidgetClass *widget_class;
  CtkContainerClass *container_class;

  CtkBindingSet *binding_set;

  object_class = (GObjectClass*) klass;
  widget_class = (CtkWidgetClass*) klass;
  container_class = (CtkContainerClass*) klass;

  object_class->set_property = ctk_menu_shell_set_property;
  object_class->get_property = ctk_menu_shell_get_property;
  object_class->finalize = ctk_menu_shell_finalize;
  object_class->dispose = ctk_menu_shell_dispose;

  widget_class->realize = ctk_menu_shell_realize;
  widget_class->button_press_event = ctk_menu_shell_button_press;
  widget_class->button_release_event = ctk_menu_shell_button_release;
  widget_class->grab_broken_event = ctk_menu_shell_grab_broken;
  widget_class->key_press_event = ctk_menu_shell_key_press;
  widget_class->enter_notify_event = ctk_menu_shell_enter_notify;
  widget_class->leave_notify_event = ctk_menu_shell_leave_notify;
  widget_class->screen_changed = ctk_menu_shell_screen_changed;

  container_class->add = ctk_menu_shell_add;
  container_class->remove = ctk_menu_shell_remove;
  container_class->forall = ctk_menu_shell_forall;
  container_class->child_type = ctk_menu_shell_child_type;

  klass->submenu_placement = CTK_TOP_BOTTOM;
  klass->deactivate = ctk_real_menu_shell_deactivate;
  klass->selection_done = NULL;
  klass->move_current = ctk_real_menu_shell_move_current;
  klass->activate_current = ctk_real_menu_shell_activate_current;
  klass->cancel = ctk_real_menu_shell_cancel;
  klass->select_item = ctk_menu_shell_real_select_item;
  klass->insert = ctk_menu_shell_real_insert;
  klass->move_selected = ctk_menu_shell_real_move_selected;

  /**
   * CtkMenuShell::deactivate:
   * @menushell: the object which received the signal
   *
   * This signal is emitted when a menu shell is deactivated.
   */
  menu_shell_signals[DEACTIVATE] =
    g_signal_new (I_("deactivate"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (CtkMenuShellClass, deactivate),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * CtkMenuShell::selection-done:
   * @menushell: the object which received the signal
   *
   * This signal is emitted when a selection has been
   * completed within a menu shell.
   */
  menu_shell_signals[SELECTION_DONE] =
    g_signal_new (I_("selection-done"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (CtkMenuShellClass, selection_done),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * CtkMenuShell::move-current:
   * @menushell: the object which received the signal
   * @direction: the direction to move
   *
   * An keybinding signal which moves the current menu item
   * in the direction specified by @direction.
   */
  menu_shell_signals[MOVE_CURRENT] =
    g_signal_new (I_("move-current"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (CtkMenuShellClass, move_current),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  CTK_TYPE_MENU_DIRECTION_TYPE);

  /**
   * CtkMenuShell::activate-current:
   * @menushell: the object which received the signal
   * @force_hide: if %TRUE, hide the menu after activating the menu item
   *
   * An action signal that activates the current menu item within
   * the menu shell.
   */
  menu_shell_signals[ACTIVATE_CURRENT] =
    g_signal_new (I_("activate-current"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (CtkMenuShellClass, activate_current),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_BOOLEAN);

  /**
   * CtkMenuShell::cancel:
   * @menushell: the object which received the signal
   *
   * An action signal which cancels the selection within the menu shell.
   * Causes the #CtkMenuShell::selection-done signal to be emitted.
   */
  menu_shell_signals[CANCEL] =
    g_signal_new (I_("cancel"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (CtkMenuShellClass, cancel),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * CtkMenuShell::cycle-focus:
   * @menushell: the object which received the signal
   * @direction: the direction to cycle in
   *
   * A keybinding signal which moves the focus in the
   * given @direction.
   */
  menu_shell_signals[CYCLE_FOCUS] =
    g_signal_new_class_handler (I_("cycle-focus"),
                                G_OBJECT_CLASS_TYPE (object_class),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                G_CALLBACK (ctk_real_menu_shell_cycle_focus),
                                NULL, NULL,
                                NULL,
                                G_TYPE_NONE, 1,
                                CTK_TYPE_DIRECTION_TYPE);

  /**
   * CtkMenuShell::move-selected:
   * @menu_shell: the object on which the signal is emitted
   * @distance: +1 to move to the next item, -1 to move to the previous
   *
   * The ::move-selected signal is emitted to move the selection to
   * another item.
   *
   * Returns: %TRUE to stop the signal emission, %FALSE to continue
   *
   * Since: 2.12
   */
  menu_shell_signals[MOVE_SELECTED] =
    g_signal_new (I_("move-selected"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkMenuShellClass, move_selected),
                  _ctk_boolean_handled_accumulator, NULL,
                  _ctk_marshal_BOOLEAN__INT,
                  G_TYPE_BOOLEAN, 1,
                  G_TYPE_INT);

  /**
   * CtkMenuShell::insert:
   * @menu_shell: the object on which the signal is emitted
   * @child: the #CtkMenuItem that is being inserted
   * @position: the position at which the insert occurs
   *
   * The ::insert signal is emitted when a new #CtkMenuItem is added to
   * a #CtkMenuShell.  A separate signal is used instead of
   * CtkContainer::add because of the need for an additional position
   * parameter.
   *
   * The inverse of this signal is the CtkContainer::removed signal.
   *
   * Since: 3.2
   **/
  menu_shell_signals[INSERT] =
    g_signal_new (I_("insert"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (CtkMenuShellClass, insert),
                  NULL, NULL,
                  _ctk_marshal_VOID__OBJECT_INT,
                  G_TYPE_NONE, 2, CTK_TYPE_WIDGET, G_TYPE_INT);
  g_signal_set_va_marshaller (menu_shell_signals[INSERT],
                              G_OBJECT_CLASS_TYPE (object_class),
                              _ctk_marshal_VOID__OBJECT_INTv);


  binding_set = ctk_binding_set_by_class (klass);
  ctk_binding_entry_add_signal (binding_set,
                                GDK_KEY_Escape, 0,
                                "cancel", 0);
  ctk_binding_entry_add_signal (binding_set,
                                GDK_KEY_Return, 0,
                                "activate-current", 1,
                                G_TYPE_BOOLEAN,
                                TRUE);
  ctk_binding_entry_add_signal (binding_set,
                                GDK_KEY_ISO_Enter, 0,
                                "activate-current", 1,
                                G_TYPE_BOOLEAN,
                                TRUE);
  ctk_binding_entry_add_signal (binding_set,
                                GDK_KEY_KP_Enter, 0,
                                "activate-current", 1,
                                G_TYPE_BOOLEAN,
                                TRUE);
  ctk_binding_entry_add_signal (binding_set,
                                GDK_KEY_space, 0,
                                "activate-current", 1,
                                G_TYPE_BOOLEAN,
                                FALSE);
  ctk_binding_entry_add_signal (binding_set,
                                GDK_KEY_KP_Space, 0,
                                "activate-current", 1,
                                G_TYPE_BOOLEAN,
                                FALSE);
  ctk_binding_entry_add_signal (binding_set,
                                GDK_KEY_F10, 0,
                                "cycle-focus", 1,
                                CTK_TYPE_DIRECTION_TYPE, CTK_DIR_TAB_FORWARD);
  ctk_binding_entry_add_signal (binding_set,
                                GDK_KEY_F10, GDK_SHIFT_MASK,
                                "cycle-focus", 1,
                                CTK_TYPE_DIRECTION_TYPE, CTK_DIR_TAB_BACKWARD);

  /**
   * CtkMenuShell:take-focus:
   *
   * A boolean that determines whether the menu and its submenus grab the
   * keyboard focus. See ctk_menu_shell_set_take_focus() and
   * ctk_menu_shell_get_take_focus().
   *
   * Since: 2.8
   **/
  g_object_class_install_property (object_class,
                                   PROP_TAKE_FOCUS,
                                   g_param_spec_boolean ("take-focus",
                                                         P_("Take Focus"),
                                                         P_("A boolean that determines whether the menu grabs the keyboard focus"),
                                                         TRUE,
                                                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_MENU_SHELL_ACCESSIBLE);
}

static GType
ctk_menu_shell_child_type (CtkContainer *container)
{
  return CTK_TYPE_MENU_ITEM;
}

static void
ctk_menu_shell_init (CtkMenuShell *menu_shell)
{
  menu_shell->priv = ctk_menu_shell_get_instance_private (menu_shell);
  menu_shell->priv->take_focus = TRUE;
}

static void
ctk_menu_shell_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  CtkMenuShell *menu_shell = CTK_MENU_SHELL (object);

  switch (prop_id)
    {
    case PROP_TAKE_FOCUS:
      ctk_menu_shell_set_take_focus (menu_shell, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_menu_shell_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  CtkMenuShell *menu_shell = CTK_MENU_SHELL (object);

  switch (prop_id)
    {
    case PROP_TAKE_FOCUS:
      g_value_set_boolean (value, ctk_menu_shell_get_take_focus (menu_shell));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_menu_shell_finalize (GObject *object)
{
  CtkMenuShell *menu_shell = CTK_MENU_SHELL (object);
  CtkMenuShellPrivate *priv = menu_shell->priv;

  if (priv->mnemonic_hash)
    _ctk_mnemonic_hash_free (priv->mnemonic_hash);
  if (priv->key_hash)
    _ctk_key_hash_free (priv->key_hash);

  G_OBJECT_CLASS (ctk_menu_shell_parent_class)->finalize (object);
}


static void
ctk_menu_shell_dispose (GObject *object)
{
  CtkMenuShell *menu_shell = CTK_MENU_SHELL (object);

  g_clear_pointer (&menu_shell->priv->tracker, ctk_menu_tracker_free);
  ctk_menu_shell_deactivate (menu_shell);

  G_OBJECT_CLASS (ctk_menu_shell_parent_class)->dispose (object);
}

/**
 * ctk_menu_shell_append:
 * @menu_shell: a #CtkMenuShell
 * @child: (type Ctk.MenuItem): The #CtkMenuItem to add
 *
 * Adds a new #CtkMenuItem to the end of the menu shell's
 * item list.
 */
void
ctk_menu_shell_append (CtkMenuShell *menu_shell,
                       CtkWidget    *child)
{
  ctk_menu_shell_insert (menu_shell, child, -1);
}

/**
 * ctk_menu_shell_prepend:
 * @menu_shell: a #CtkMenuShell
 * @child: The #CtkMenuItem to add
 *
 * Adds a new #CtkMenuItem to the beginning of the menu shell's
 * item list.
 */
void
ctk_menu_shell_prepend (CtkMenuShell *menu_shell,
                        CtkWidget    *child)
{
  ctk_menu_shell_insert (menu_shell, child, 0);
}

/**
 * ctk_menu_shell_insert:
 * @menu_shell: a #CtkMenuShell
 * @child: The #CtkMenuItem to add
 * @position: The position in the item list where @child
 *     is added. Positions are numbered from 0 to n-1
 *
 * Adds a new #CtkMenuItem to the menu shell’s item list
 * at the position indicated by @position.
 */
void
ctk_menu_shell_insert (CtkMenuShell *menu_shell,
                       CtkWidget    *child,
                       gint          position)
{
  g_return_if_fail (CTK_IS_MENU_SHELL (menu_shell));
  g_return_if_fail (CTK_IS_MENU_ITEM (child));

  g_signal_emit (menu_shell, menu_shell_signals[INSERT], 0, child, position);
}

static void
ctk_menu_shell_real_insert (CtkMenuShell *menu_shell,
                            CtkWidget    *child,
                            gint          position)
{
  CtkMenuShellPrivate *priv = menu_shell->priv;

  priv->children = g_list_insert (priv->children, child, position);

  ctk_widget_set_parent (child, CTK_WIDGET (menu_shell));
}

/**
 * ctk_menu_shell_deactivate:
 * @menu_shell: a #CtkMenuShell
 *
 * Deactivates the menu shell.
 *
 * Typically this results in the menu shell being erased
 * from the screen.
 */
void
ctk_menu_shell_deactivate (CtkMenuShell *menu_shell)
{
  g_return_if_fail (CTK_IS_MENU_SHELL (menu_shell));

  if (menu_shell->priv->active)
    g_signal_emit (menu_shell, menu_shell_signals[DEACTIVATE], 0);
}

static void
ctk_menu_shell_realize (CtkWidget *widget)
{
  CtkAllocation allocation;
  GdkWindow *window;
  GdkWindowAttr attributes;
  gint attributes_mask;

  ctk_widget_set_realized (widget, TRUE);

  ctk_widget_get_allocation (widget, &allocation);

  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = ctk_widget_get_visual (widget);
  attributes.event_mask = ctk_widget_get_events (widget);
  attributes.event_mask |= (GDK_BUTTON_PRESS_MASK |
                            GDK_BUTTON_RELEASE_MASK |
                            GDK_POINTER_MOTION_MASK |
                            GDK_KEY_PRESS_MASK |
                            GDK_ENTER_NOTIFY_MASK |
                            GDK_LEAVE_NOTIFY_MASK);

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;

  window = cdk_window_new (ctk_widget_get_parent_window (widget),
                           &attributes, attributes_mask);
  ctk_widget_set_window (widget, window);
  ctk_widget_register_window (widget, window);
}

static void
ctk_menu_shell_activate (CtkMenuShell *menu_shell)
{
  CtkMenuShellPrivate *priv = menu_shell->priv;

  if (!priv->active)
    {
      GdkDevice *device;

      device = ctk_get_current_event_device ();

      _ctk_menu_shell_set_grab_device (menu_shell, device);
      ctk_grab_add (CTK_WIDGET (menu_shell));

      priv->have_grab = TRUE;
      priv->active = TRUE;
    }
}

static gint
ctk_menu_shell_button_press (CtkWidget      *widget,
                             GdkEventButton *event)
{
  CtkMenuShell *menu_shell;
  CtkMenuShellPrivate *priv;
  CtkWidget *menu_item;
  CtkWidget *parent;

  if (event->type != GDK_BUTTON_PRESS)
    return FALSE;

  menu_shell = CTK_MENU_SHELL (widget);
  priv = menu_shell->priv;

  if (priv->parent_menu_shell)
    return ctk_widget_event (priv->parent_menu_shell, (GdkEvent*) event);

  menu_item = ctk_menu_shell_get_item (menu_shell, (GdkEvent *)event);

  if (menu_item && _ctk_menu_item_is_selectable (menu_item))
    {
      parent = ctk_widget_get_parent (menu_item);

      if (menu_item != CTK_MENU_SHELL (parent)->priv->active_menu_item)
        {
          /*  select the menu item *before* activating the shell, so submenus
           *  which might be open are closed the friendly way. If we activate
           *  (and thus grab) this menu shell first, we might get grab_broken
           *  events which will close the entire menu hierarchy. Selecting the
           *  menu item also fixes up the state as if enter_notify() would
           *  have run before (which normally selects the item).
           */
          if (CTK_MENU_SHELL_GET_CLASS (parent)->submenu_placement != CTK_TOP_BOTTOM)
            ctk_menu_shell_select_item (CTK_MENU_SHELL (parent), menu_item);
        }
    }

  if (!priv->active || !priv->button)
    {
      gboolean initially_active = priv->active;

      priv->button = event->button;

      if (menu_item)
        {
          if (_ctk_menu_item_is_selectable (menu_item) &&
              ctk_widget_get_parent (menu_item) == widget &&
              menu_item != priv->active_menu_item)
            {
              ctk_menu_shell_activate (menu_shell);
              priv->button = event->button;

              if (CTK_MENU_SHELL_GET_CLASS (menu_shell)->submenu_placement == CTK_TOP_BOTTOM)
                {
                  priv->activate_time = event->time;
                  ctk_menu_shell_select_item (menu_shell, menu_item);
                }
            }
        }
      else
        {
          if (!initially_active)
            {
              ctk_menu_shell_deactivate (menu_shell);
              return FALSE;
            }
        }
    }
  else
    {
      widget = ctk_get_event_widget ((GdkEvent*) event);
      if (widget == CTK_WIDGET (menu_shell))
        {
          ctk_menu_shell_deactivate (menu_shell);
          g_signal_emit (menu_shell, menu_shell_signals[SELECTION_DONE], 0);
        }
    }

  if (menu_item &&
      _ctk_menu_item_is_selectable (menu_item) &&
      CTK_MENU_ITEM (menu_item)->priv->submenu != NULL &&
      !ctk_widget_get_visible (CTK_MENU_ITEM (menu_item)->priv->submenu))
    {
      _ctk_menu_item_popup_submenu (menu_item, FALSE);
      priv->activated_submenu = TRUE;
    }

  return TRUE;
}

static gboolean
ctk_menu_shell_grab_broken (CtkWidget          *widget,
                            GdkEventGrabBroken *event)
{
  CtkMenuShell *menu_shell = CTK_MENU_SHELL (widget);
  CtkMenuShellPrivate *priv = menu_shell->priv;

  if (priv->have_xgrab && event->grab_window == NULL)
    {
      /* Unset the active menu item so ctk_menu_popdown() doesn't see it. */
      ctk_menu_shell_deselect (menu_shell);
      ctk_menu_shell_deactivate (menu_shell);
      g_signal_emit (menu_shell, menu_shell_signals[SELECTION_DONE], 0);
    }

  return TRUE;
}

static gint
ctk_menu_shell_button_release (CtkWidget      *widget,
                               GdkEventButton *event)
{
  CtkMenuShell *menu_shell = CTK_MENU_SHELL (widget);
  CtkMenuShellPrivate *priv = menu_shell->priv;

  if (priv->parent_menu_shell &&
      (event->time - CTK_MENU_SHELL (priv->parent_menu_shell)->priv->activate_time) < MENU_SHELL_TIMEOUT)
    {
      /* The button-press originated in the parent menu bar and we are
       * a pop-up menu. It was a quick press-and-release so we don't want
       * to activate an item but we leave the popup in place instead.
       * https://bugzilla.gnome.org/show_bug.cgi?id=703069
       */
      CTK_MENU_SHELL (priv->parent_menu_shell)->priv->activate_time = 0;
      return TRUE;
    }

  if (priv->active)
    {
      CtkWidget *menu_item;
      gboolean   deactivate = TRUE;

      if (priv->button && (event->button != priv->button))
        {
          priv->button = 0;
          if (priv->parent_menu_shell)
            return ctk_widget_event (priv->parent_menu_shell, (GdkEvent*) event);
        }

      priv->button = 0;
      menu_item = ctk_menu_shell_get_item (menu_shell, (GdkEvent*) event);

      if ((event->time - priv->activate_time) > MENU_SHELL_TIMEOUT)
        {
          if (menu_item && (priv->active_menu_item == menu_item) &&
              _ctk_menu_item_is_selectable (menu_item))
            {
              CtkWidget *submenu = CTK_MENU_ITEM (menu_item)->priv->submenu;

              if (submenu == NULL)
                {
                  ctk_menu_shell_activate_item (menu_shell, menu_item, TRUE);
                  deactivate = FALSE;
                }
              else if (CTK_MENU_SHELL_GET_CLASS (menu_shell)->submenu_placement != CTK_TOP_BOTTOM ||
                       priv->activated_submenu)
                {
                  GTimeVal *popup_time;
                  gint64 usec_since_popup = 0;

                  popup_time = g_object_get_data (G_OBJECT (submenu),
                                                  "ctk-menu-exact-popup-time");

                  if (popup_time)
                    {
                      GTimeVal current_time;

                      g_get_current_time (&current_time);

                      usec_since_popup = ((gint64) current_time.tv_sec * 1000 * 1000 +
                                          (gint64) current_time.tv_usec -
                                          (gint64) popup_time->tv_sec * 1000 * 1000 -
                                          (gint64) popup_time->tv_usec);

                      g_object_set_data (G_OBJECT (submenu),
                                         "ctk-menu-exact-popup-time", NULL);
                    }

                  /* Only close the submenu on click if we opened the
                   * menu explicitly (usec_since_popup == 0) or
                   * enough time has passed since it was opened by
                   * CtkMenuItem's timeout (usec_since_popup > delay).
                   */
                  if (!priv->activated_submenu &&
                      (usec_since_popup == 0 ||
                       usec_since_popup > MENU_POPDOWN_DELAY * 1000))
                    {
                      _ctk_menu_item_popdown_submenu (menu_item);
                    }
                  else
                    {
                      ctk_menu_item_select (CTK_MENU_ITEM (menu_item));
                    }

                  deactivate = FALSE;
                }
            }
          else if (menu_item &&
                   !_ctk_menu_item_is_selectable (menu_item) &&
                   CTK_MENU_SHELL_GET_CLASS (menu_shell)->submenu_placement != CTK_TOP_BOTTOM)
            {
              deactivate = FALSE;
            }
          else if (priv->parent_menu_shell)
            {
              priv->active = TRUE;
              ctk_widget_event (priv->parent_menu_shell, (GdkEvent*) event);
              deactivate = FALSE;
            }

          /* If we ended up on an item with a submenu, leave the menu up. */
          if (menu_item &&
              (priv->active_menu_item == menu_item) &&
              CTK_MENU_SHELL_GET_CLASS (menu_shell)->submenu_placement != CTK_TOP_BOTTOM)
            {
              deactivate = FALSE;
            }
        }
      else /* a very fast press-release */
        {
          /* We only ever want to prevent deactivation on the first
           * press/release. Setting the time to zero is a bit of a
           * hack, since we could be being triggered in the first
           * few fractions of a second after a server time wraparound.
           * the chances of that happening are ~1/10^6, without
           * serious harm if we lose.
           */
          priv->activate_time = 0;
          deactivate = FALSE;
        }

      if (deactivate)
        {
          ctk_menu_shell_deactivate (menu_shell);
          g_signal_emit (menu_shell, menu_shell_signals[SELECTION_DONE], 0);
        }

      priv->activated_submenu = FALSE;
    }

  return TRUE;
}

void
_ctk_menu_shell_set_keyboard_mode (CtkMenuShell *menu_shell,
                                   gboolean      keyboard_mode)
{
  menu_shell->priv->keyboard_mode = keyboard_mode;
}

gboolean
_ctk_menu_shell_get_keyboard_mode (CtkMenuShell *menu_shell)
{
  return menu_shell->priv->keyboard_mode;
}

void
_ctk_menu_shell_update_mnemonics (CtkMenuShell *menu_shell)
{
  CtkMenuShell *target;
  gboolean found;
  gboolean mnemonics_visible;

  target = menu_shell;
  found = FALSE;
  while (target)
    {
      CtkMenuShellPrivate *priv = target->priv;
      CtkWidget *toplevel = ctk_widget_get_toplevel (CTK_WIDGET (target));

      /* The idea with keyboard mode is that once you start using
       * the keyboard to navigate the menus, we show mnemonics
       * until the menu navigation is over. To that end, we spread
       * the keyboard mode upwards in the menu hierarchy here.
       * Also see ctk_menu_popup, where we inherit it downwards.
       */
      if (menu_shell->priv->keyboard_mode)
        target->priv->keyboard_mode = TRUE;

      /* While navigating menus, the first parent menu with an active
       * item is the one where mnemonics are effective, as can be seen
       * in ctk_menu_shell_key_press below.
       * We also show mnemonics in context menus. The grab condition is
       * necessary to ensure we remove underlines from menu bars when
       * dismissing menus.
       */
      mnemonics_visible = target->priv->keyboard_mode &&
                          (((target->priv->active_menu_item || priv->in_unselectable_item) && !found) ||
                           (target == menu_shell &&
                            !target->priv->parent_menu_shell &&
                            ctk_widget_has_grab (CTK_WIDGET (target))));

      /* While menus are up, only show underlines inside the menubar,
       * not in the entire window.
       */
      if (CTK_IS_MENU_BAR (target))
        {
          ctk_window_set_mnemonics_visible (CTK_WINDOW (toplevel), FALSE);
          _ctk_label_mnemonics_visible_apply_recursively (CTK_WIDGET (target),
                                                          mnemonics_visible);
        }
      else
        ctk_window_set_mnemonics_visible (CTK_WINDOW (toplevel), mnemonics_visible);

      if (target->priv->active_menu_item || priv->in_unselectable_item)
        found = TRUE;

      target = CTK_MENU_SHELL (target->priv->parent_menu_shell);
    }
}

static gint
ctk_menu_shell_key_press (CtkWidget   *widget,
                          GdkEventKey *event)
{
  CtkMenuShell *menu_shell = CTK_MENU_SHELL (widget);
  CtkMenuShellPrivate *priv = menu_shell->priv;
  gboolean enable_mnemonics;

  priv->keyboard_mode = TRUE;

  if (!(priv->active_menu_item || priv->in_unselectable_item) &&
      priv->parent_menu_shell)
    return ctk_widget_event (priv->parent_menu_shell, (GdkEvent *)event);

  if (ctk_bindings_activate_event (G_OBJECT (widget), event))
    return TRUE;

  g_object_get (ctk_widget_get_settings (widget),
                "ctk-enable-mnemonics", &enable_mnemonics,
                NULL);

  if (enable_mnemonics)
    return ctk_menu_shell_activate_mnemonic (menu_shell, event);

  return FALSE;
}

static gint
ctk_menu_shell_enter_notify (CtkWidget        *widget,
                             GdkEventCrossing *event)
{
  CtkMenuShell *menu_shell = CTK_MENU_SHELL (widget);
  CtkMenuShellPrivate *priv = menu_shell->priv;

  if (event->mode == GDK_CROSSING_CTK_GRAB ||
      event->mode == GDK_CROSSING_CTK_UNGRAB ||
      event->mode == GDK_CROSSING_STATE_CHANGED)
    return TRUE;

  if (priv->active)
    {
      CtkWidget *menu_item;
      CtkWidget *parent;

      menu_item = ctk_get_event_widget ((GdkEvent*) event);

      if (!menu_item)
        return TRUE;

      if (CTK_IS_MENU_ITEM (menu_item) &&
          !_ctk_menu_item_is_selectable (menu_item))
        {
          priv->in_unselectable_item = TRUE;
          return TRUE;
        }

      parent = ctk_widget_get_parent (menu_item);
      if (parent == widget &&
          CTK_IS_MENU_ITEM (menu_item))
        {
          if (priv->ignore_enter)
            return TRUE;

          if (event->detail != GDK_NOTIFY_INFERIOR)
            {
              if ((ctk_widget_get_state_flags (menu_item) & CTK_STATE_FLAG_PRELIGHT) == 0)
                ctk_menu_shell_select_item (menu_shell, menu_item);

              /* If any mouse button is down, and there is a submenu
               * that is not yet visible, activate it. It's sufficient
               * to check for any button's mask (not only the one
               * matching menu_shell->button), because there is no
               * situation a mouse button could be pressed while
               * entering a menu item where we wouldn't want to show
               * its submenu.
               */
              if ((event->state & (GDK_BUTTON1_MASK|GDK_BUTTON2_MASK|GDK_BUTTON3_MASK)) &&
                  CTK_MENU_ITEM (menu_item)->priv->submenu != NULL)
                {
                  CTK_MENU_SHELL (parent)->priv->activated_submenu = TRUE;

                  if (!ctk_widget_get_visible (CTK_MENU_ITEM (menu_item)->priv->submenu))
                    {
                      GdkDevice *source_device;

                      source_device = cdk_event_get_source_device ((GdkEvent *) event);

                      if (cdk_device_get_source (source_device) == GDK_SOURCE_TOUCHSCREEN)
                        _ctk_menu_item_popup_submenu (menu_item, TRUE);
                    }
                }
            }
        }
      else if (priv->parent_menu_shell)
        {
          ctk_widget_event (priv->parent_menu_shell, (GdkEvent*) event);
        }
    }

  return TRUE;
}

static gint
ctk_menu_shell_leave_notify (CtkWidget        *widget,
                             GdkEventCrossing *event)
{
  if (event->mode == GDK_CROSSING_CTK_GRAB ||
      event->mode == GDK_CROSSING_CTK_UNGRAB ||
      event->mode == GDK_CROSSING_STATE_CHANGED)
    return TRUE;

  if (ctk_widget_get_visible (widget))
    {
      CtkMenuShell *menu_shell = CTK_MENU_SHELL (widget);
      CtkMenuShellPrivate *priv = menu_shell->priv;
      CtkWidget *event_widget = ctk_get_event_widget ((GdkEvent*) event);
      CtkMenuItem *menu_item;

      if (!event_widget || !CTK_IS_MENU_ITEM (event_widget))
        return TRUE;

      menu_item = CTK_MENU_ITEM (event_widget);

      if (!_ctk_menu_item_is_selectable (event_widget))
        {
          priv->in_unselectable_item = TRUE;
          return TRUE;
        }

      if ((priv->active_menu_item == event_widget) &&
          (menu_item->priv->submenu == NULL))
        {
          if ((event->detail != GDK_NOTIFY_INFERIOR) &&
              (ctk_widget_get_state_flags (CTK_WIDGET (menu_item)) & CTK_STATE_FLAG_PRELIGHT) != 0)
            {
              ctk_menu_shell_deselect (menu_shell);
            }
        }
      else if (priv->parent_menu_shell)
        {
          ctk_widget_event (priv->parent_menu_shell, (GdkEvent*) event);
        }
    }

  return TRUE;
}

static void
ctk_menu_shell_screen_changed (CtkWidget *widget,
                               GdkScreen *previous_screen)
{
  ctk_menu_shell_reset_key_hash (CTK_MENU_SHELL (widget));
}

static void
ctk_menu_shell_add (CtkContainer *container,
                    CtkWidget    *widget)
{
  ctk_menu_shell_append (CTK_MENU_SHELL (container), widget);
}

static void
ctk_menu_shell_remove (CtkContainer *container,
                       CtkWidget    *widget)
{
  CtkMenuShell *menu_shell = CTK_MENU_SHELL (container);
  CtkMenuShellPrivate *priv = menu_shell->priv;
  gint was_visible;

  was_visible = ctk_widget_get_visible (widget);
  priv->children = g_list_remove (priv->children, widget);

  if (widget == priv->active_menu_item)
    {
      g_signal_emit_by_name (priv->active_menu_item, "deselect");
      priv->active_menu_item = NULL;
    }

  ctk_widget_unparent (widget);

  /* Queue resize regardless of ctk_widget_get_visible (container),
   * since that's what is needed by toplevels.
   */
  if (was_visible)
    ctk_widget_queue_resize (CTK_WIDGET (container));
}

static void
ctk_menu_shell_forall (CtkContainer *container,
                       gboolean      include_internals,
                       CtkCallback   callback,
                       gpointer      callback_data)
{
  CtkMenuShell *menu_shell = CTK_MENU_SHELL (container);
  CtkWidget *child;
  GList *children;

  children = menu_shell->priv->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      (* callback) (child, callback_data);
    }
}


static void
ctk_real_menu_shell_deactivate (CtkMenuShell *menu_shell)
{
  CtkMenuShellPrivate *priv = menu_shell->priv;

  if (priv->active)
    {
      priv->button = 0;
      priv->active = FALSE;
      priv->activate_time = 0;

      if (priv->active_menu_item)
        {
          ctk_menu_item_deselect (CTK_MENU_ITEM (priv->active_menu_item));
          priv->active_menu_item = NULL;
        }

      if (priv->have_grab)
        {
          priv->have_grab = FALSE;
          ctk_grab_remove (CTK_WIDGET (menu_shell));
        }
      if (priv->have_xgrab)
        {
          cdk_seat_ungrab (cdk_device_get_seat (priv->grab_pointer));
          priv->have_xgrab = FALSE;
        }

      priv->keyboard_mode = FALSE;
      _ctk_menu_shell_set_grab_device (menu_shell, NULL);

      _ctk_menu_shell_update_mnemonics (menu_shell);
    }
}

static gint
ctk_menu_shell_is_item (CtkMenuShell *menu_shell,
                        CtkWidget    *child)
{
  CtkWidget *parent;

  g_return_val_if_fail (CTK_IS_MENU_SHELL (menu_shell), FALSE);
  g_return_val_if_fail (child != NULL, FALSE);

  parent = ctk_widget_get_parent (child);
  while (CTK_IS_MENU_SHELL (parent))
    {
      if (parent == (CtkWidget*) menu_shell)
        return TRUE;
      parent = CTK_MENU_SHELL (parent)->priv->parent_menu_shell;
    }

  return FALSE;
}

static CtkWidget*
ctk_menu_shell_get_item (CtkMenuShell *menu_shell,
                         GdkEvent     *event)
{
  CtkWidget *menu_item;

  menu_item = ctk_get_event_widget ((GdkEvent*) event);

  while (menu_item && !CTK_IS_MENU_ITEM (menu_item))
    menu_item = ctk_widget_get_parent (menu_item);

  if (menu_item && ctk_menu_shell_is_item (menu_shell, menu_item))
    return menu_item;
  else
    return NULL;
}

/* Handlers for action signals */

/**
 * ctk_menu_shell_select_item:
 * @menu_shell: a #CtkMenuShell
 * @menu_item: The #CtkMenuItem to select
 *
 * Selects the menu item from the menu shell.
 */
void
ctk_menu_shell_select_item (CtkMenuShell *menu_shell,
                            CtkWidget    *menu_item)
{
  CtkMenuShellPrivate *priv;
  CtkMenuShellClass *class;

  g_return_if_fail (CTK_IS_MENU_SHELL (menu_shell));
  g_return_if_fail (CTK_IS_MENU_ITEM (menu_item));

  priv = menu_shell->priv;
  class = CTK_MENU_SHELL_GET_CLASS (menu_shell);

  if (class->select_item &&
      !(priv->active &&
        priv->active_menu_item == menu_item))
    class->select_item (menu_shell, menu_item);
}

void _ctk_menu_item_set_placement (CtkMenuItem         *menu_item,
                                   CtkSubmenuPlacement  placement);

static void
ctk_menu_shell_real_select_item (CtkMenuShell *menu_shell,
                                 CtkWidget    *menu_item)
{
  CtkMenuShellPrivate *priv = menu_shell->priv;
  CtkPackDirection pack_dir = PACK_DIRECTION (menu_shell);

  if (priv->active_menu_item)
    {
      ctk_menu_item_deselect (CTK_MENU_ITEM (priv->active_menu_item));
      priv->active_menu_item = NULL;
    }

  if (!_ctk_menu_item_is_selectable (menu_item))
    {
      priv->in_unselectable_item = TRUE;
      _ctk_menu_shell_update_mnemonics (menu_shell);
      return;
    }

  ctk_menu_shell_activate (menu_shell);

  priv->active_menu_item = menu_item;
  if (pack_dir == CTK_PACK_DIRECTION_TTB || pack_dir == CTK_PACK_DIRECTION_BTT)
    _ctk_menu_item_set_placement (CTK_MENU_ITEM (priv->active_menu_item),
                                  CTK_LEFT_RIGHT);
  else
    _ctk_menu_item_set_placement (CTK_MENU_ITEM (priv->active_menu_item),
                                  CTK_MENU_SHELL_GET_CLASS (menu_shell)->submenu_placement);
  ctk_menu_item_select (CTK_MENU_ITEM (priv->active_menu_item));

  _ctk_menu_shell_update_mnemonics (menu_shell);

  /* This allows the bizarre radio buttons-with-submenus-display-history
   * behavior
   */
  if (CTK_MENU_ITEM (priv->active_menu_item)->priv->submenu)
    ctk_widget_activate (priv->active_menu_item);
}

/**
 * ctk_menu_shell_deselect:
 * @menu_shell: a #CtkMenuShell
 *
 * Deselects the currently selected item from the menu shell,
 * if any.
 */
void
ctk_menu_shell_deselect (CtkMenuShell *menu_shell)
{
  CtkMenuShellPrivate *priv;

  g_return_if_fail (CTK_IS_MENU_SHELL (menu_shell));

  priv = menu_shell->priv;

  if (priv->active_menu_item)
    {
      ctk_menu_item_deselect (CTK_MENU_ITEM (priv->active_menu_item));
      priv->active_menu_item = NULL;
      _ctk_menu_shell_update_mnemonics (menu_shell);
    }
}

/**
 * ctk_menu_shell_activate_item:
 * @menu_shell: a #CtkMenuShell
 * @menu_item: the #CtkMenuItem to activate
 * @force_deactivate: if %TRUE, force the deactivation of the
 *     menu shell after the menu item is activated
 *
 * Activates the menu item within the menu shell.
 */
void
ctk_menu_shell_activate_item (CtkMenuShell *menu_shell,
                              CtkWidget    *menu_item,
                              gboolean      force_deactivate)
{
  GSList *slist, *shells = NULL;
  gboolean deactivate = force_deactivate;

  g_return_if_fail (CTK_IS_MENU_SHELL (menu_shell));
  g_return_if_fail (CTK_IS_MENU_ITEM (menu_item));

  if (!deactivate)
    deactivate = CTK_MENU_ITEM_GET_CLASS (menu_item)->hide_on_activate;

  g_object_ref (menu_shell);
  g_object_ref (menu_item);

  if (deactivate)
    {
      CtkMenuShell *parent_menu_shell = menu_shell;

      do
        {
          parent_menu_shell->priv->selection_done_coming_soon = TRUE;

          g_object_ref (parent_menu_shell);
          shells = g_slist_prepend (shells, parent_menu_shell);
          parent_menu_shell = (CtkMenuShell*) parent_menu_shell->priv->parent_menu_shell;
        }
      while (parent_menu_shell);
      shells = g_slist_reverse (shells);

      ctk_menu_shell_deactivate (menu_shell);

      /* Flush the x-queue, so any grabs are removed and
       * the menu is actually taken down
       */
      cdk_display_sync (ctk_widget_get_display (menu_item));
    }

  ctk_widget_activate (menu_item);

  for (slist = shells; slist; slist = slist->next)
    {
      CtkMenuShell *parent_menu_shell = slist->data;

      g_signal_emit (parent_menu_shell, menu_shell_signals[SELECTION_DONE], 0);
      parent_menu_shell->priv->selection_done_coming_soon = FALSE;
      g_object_unref (slist->data);
    }
  g_slist_free (shells);

  g_object_unref (menu_shell);
  g_object_unref (menu_item);
}

/* Distance should be +/- 1 */
static gboolean
ctk_menu_shell_real_move_selected (CtkMenuShell  *menu_shell,
                                   gint           distance)
{
  CtkMenuShellPrivate *priv = menu_shell->priv;

  if (priv->active_menu_item)
    {
      GList *node = g_list_find (priv->children, priv->active_menu_item);
      GList *start_node = node;

      if (distance > 0)
        {
          node = node->next;
          while (node != start_node &&
                 (!node || !_ctk_menu_item_is_selectable (node->data)))
            {
              if (node)
                node = node->next;
              else
                node = priv->children;
            }
        }
      else
        {
          node = node->prev;
          while (node != start_node &&
                 (!node || !_ctk_menu_item_is_selectable (node->data)))
            {
              if (node)
                node = node->prev;
              else
                node = g_list_last (priv->children);
            }
        }
      
      if (node)
        ctk_menu_shell_select_item (menu_shell, node->data);
    }

  return TRUE;
}

/* Distance should be +/- 1 */
static void
ctk_menu_shell_move_selected (CtkMenuShell  *menu_shell,
                              gint           distance)
{
  gboolean handled = FALSE;

  g_signal_emit (menu_shell, menu_shell_signals[MOVE_SELECTED], 0,
                 distance, &handled);
}

/**
 * ctk_menu_shell_select_first:
 * @menu_shell: a #CtkMenuShell
 * @search_sensitive: if %TRUE, search for the first selectable
 *                    menu item, otherwise select nothing if
 *                    the first item isn’t sensitive. This
 *                    should be %FALSE if the menu is being
 *                    popped up initially.
 *
 * Select the first visible or selectable child of the menu shell;
 * don’t select tearoff items unless the only item is a tearoff
 * item.
 *
 * Since: 2.2
 */
void
ctk_menu_shell_select_first (CtkMenuShell *menu_shell,
                             gboolean      search_sensitive)
{
  CtkMenuShellPrivate *priv;
  CtkWidget *to_select = NULL;
  GList *tmp_list;

  g_return_if_fail (CTK_IS_MENU_SHELL (menu_shell));

  priv = menu_shell->priv;

  tmp_list = priv->children;
  while (tmp_list)
    {
      CtkWidget *child = tmp_list->data;

      if ((!search_sensitive && ctk_widget_get_visible (child)) ||
          _ctk_menu_item_is_selectable (child))
        {
          to_select = child;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

          if (!CTK_IS_TEAROFF_MENU_ITEM (child))
            break;

G_GNUC_END_IGNORE_DEPRECATIONS

        }

      tmp_list = tmp_list->next;
    }

  if (to_select)
    ctk_menu_shell_select_item (menu_shell, to_select);
}

void
_ctk_menu_shell_select_last (CtkMenuShell *menu_shell,
                             gboolean      search_sensitive)
{
  CtkMenuShellPrivate *priv = menu_shell->priv;
  CtkWidget *to_select = NULL;
  GList *tmp_list;

  tmp_list = g_list_last (priv->children);
  while (tmp_list)
    {
      CtkWidget *child = tmp_list->data;

      if ((!search_sensitive && ctk_widget_get_visible (child)) ||
          _ctk_menu_item_is_selectable (child))
        {
          to_select = child;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

          if (!CTK_IS_TEAROFF_MENU_ITEM (child))
            break;

G_GNUC_END_IGNORE_DEPRECATIONS
        }

      tmp_list = tmp_list->prev;
    }

  if (to_select)
    ctk_menu_shell_select_item (menu_shell, to_select);
}

static gboolean
ctk_menu_shell_select_submenu_first (CtkMenuShell *menu_shell)
{
  CtkMenuShellPrivate *priv = menu_shell->priv;
  CtkMenuItem *menu_item;

  if (priv->active_menu_item == NULL)
    return FALSE;

  menu_item = CTK_MENU_ITEM (priv->active_menu_item);

  if (menu_item->priv->submenu)
    {
      _ctk_menu_item_popup_submenu (CTK_WIDGET (menu_item), FALSE);
      ctk_menu_shell_select_first (CTK_MENU_SHELL (menu_item->priv->submenu), TRUE);
      if (CTK_MENU_SHELL (menu_item->priv->submenu)->priv->active_menu_item)
        return TRUE;
    }

  return FALSE;
}

/* Moves the current menu item in direction 'direction':
 *
 * - CTK_MENU_DIR_PARENT: To the parent menu shell
 * - CTK_MENU_DIR_CHILD: To the child menu shell (if this item has a submenu).
 * - CTK_MENU_DIR_NEXT/PREV: To the next or previous item in this menu.
 *
 * As a bit of a hack to get movement between menus and
 * menubars working, if submenu_placement is different for
 * the menu and its MenuShell then the following apply:
 *
 * - For “parent” the current menu is not just moved to
 *   the parent, but moved to the previous entry in the parent
 * - For 'child', if there is no child, then current is
 *   moved to the next item in the parent.
 *
 * Note that the above explanation of ::move_current was written
 * before menus and menubars had support for RTL flipping and
 * different packing directions, and therefore only applies for
 * when text direction and packing direction are both left-to-right.
 */
static void
ctk_real_menu_shell_move_current (CtkMenuShell         *menu_shell,
                                  CtkMenuDirectionType  direction)
{
  CtkMenuShellPrivate *priv = menu_shell->priv;
  CtkMenuShell *parent_menu_shell = NULL;
  gboolean had_selection;

  priv->in_unselectable_item = FALSE;

  had_selection = priv->active_menu_item != NULL;

  if (priv->parent_menu_shell)
    parent_menu_shell = CTK_MENU_SHELL (priv->parent_menu_shell);

  switch (direction)
    {
    case CTK_MENU_DIR_PARENT:
      if (parent_menu_shell)
        {
          if (CTK_MENU_SHELL_GET_CLASS (parent_menu_shell)->submenu_placement ==
              CTK_MENU_SHELL_GET_CLASS (menu_shell)->submenu_placement)
            ctk_menu_shell_deselect (menu_shell);
          else
            {
              if (PACK_DIRECTION (parent_menu_shell) == CTK_PACK_DIRECTION_LTR)
                ctk_menu_shell_move_selected (parent_menu_shell, -1);
              else
                ctk_menu_shell_move_selected (parent_menu_shell, 1);
              ctk_menu_shell_select_submenu_first (parent_menu_shell);
            }
        }
      /* If there is no parent and the submenu is in the opposite direction
       * to the menu, then make the PARENT direction wrap around to
       * the bottom of the submenu.
       */
      else if (priv->active_menu_item &&
               _ctk_menu_item_is_selectable (priv->active_menu_item) &&
               CTK_MENU_ITEM (priv->active_menu_item)->priv->submenu)
        {
          CtkMenuShell *submenu = CTK_MENU_SHELL (CTK_MENU_ITEM (priv->active_menu_item)->priv->submenu);

          if (CTK_MENU_SHELL_GET_CLASS (menu_shell)->submenu_placement !=
              CTK_MENU_SHELL_GET_CLASS (submenu)->submenu_placement)
            _ctk_menu_shell_select_last (submenu, TRUE);
        }
      break;

    case CTK_MENU_DIR_CHILD:
      if (priv->active_menu_item &&
          _ctk_menu_item_is_selectable (priv->active_menu_item) &&
          CTK_MENU_ITEM (priv->active_menu_item)->priv->submenu)
        {
          if (ctk_menu_shell_select_submenu_first (menu_shell))
            break;
        }

      /* Try to find a menu running the opposite direction */
      while (parent_menu_shell &&
             (CTK_MENU_SHELL_GET_CLASS (parent_menu_shell)->submenu_placement ==
              CTK_MENU_SHELL_GET_CLASS (menu_shell)->submenu_placement))
        {
          parent_menu_shell = CTK_MENU_SHELL (parent_menu_shell->priv->parent_menu_shell);
        }

      if (parent_menu_shell)
        {
          if (PACK_DIRECTION (parent_menu_shell) == CTK_PACK_DIRECTION_LTR)
            ctk_menu_shell_move_selected (parent_menu_shell, 1);
          else
            ctk_menu_shell_move_selected (parent_menu_shell, -1);

          ctk_menu_shell_select_submenu_first (parent_menu_shell);
        }
      break;

    case CTK_MENU_DIR_PREV:
      ctk_menu_shell_move_selected (menu_shell, -1);
      if (!had_selection && !priv->active_menu_item && priv->children)
        _ctk_menu_shell_select_last (menu_shell, TRUE);
      break;

    case CTK_MENU_DIR_NEXT:
      ctk_menu_shell_move_selected (menu_shell, 1);
      if (!had_selection && !priv->active_menu_item && priv->children)
        ctk_menu_shell_select_first (menu_shell, TRUE);
      break;
    }
}

/* Activate the current item. If 'force_hide' is true, hide
 * the current menu item always. Otherwise, only hide
 * it if menu_item->klass->hide_on_activate is true.
 */
static void
ctk_real_menu_shell_activate_current (CtkMenuShell *menu_shell,
                                      gboolean      force_hide)
{
  CtkMenuShellPrivate *priv = menu_shell->priv;

  if (priv->active_menu_item &&
      _ctk_menu_item_is_selectable (priv->active_menu_item))
  {
    if (CTK_MENU_ITEM (priv->active_menu_item)->priv->submenu == NULL)
      ctk_menu_shell_activate_item (menu_shell,
                                    priv->active_menu_item,
                                    force_hide);
    else
      ctk_menu_shell_select_submenu_first (menu_shell);
  }
}

static void
ctk_real_menu_shell_cancel (CtkMenuShell *menu_shell)
{
  /* Unset the active menu item so ctk_menu_popdown() doesn't see it. */
  ctk_menu_shell_deselect (menu_shell);
  ctk_menu_shell_deactivate (menu_shell);
  g_signal_emit (menu_shell, menu_shell_signals[SELECTION_DONE], 0);
}

static void
ctk_real_menu_shell_cycle_focus (CtkMenuShell     *menu_shell,
                                 CtkDirectionType  dir)
{
  while (menu_shell && !CTK_IS_MENU_BAR (menu_shell))
    {
      if (menu_shell->priv->parent_menu_shell)
        menu_shell = CTK_MENU_SHELL (menu_shell->priv->parent_menu_shell);
      else
        menu_shell = NULL;
    }

  if (menu_shell)
    _ctk_menu_bar_cycle_focus (CTK_MENU_BAR (menu_shell), dir);
}

gint
_ctk_menu_shell_get_popup_delay (CtkMenuShell *menu_shell)
{
  CtkMenuShellClass *klass = CTK_MENU_SHELL_GET_CLASS (menu_shell);
  
  if (klass->get_popup_delay)
    {
      return klass->get_popup_delay (menu_shell);
    }
  else
    {
      return MENU_POPUP_DELAY;
    }
}

/**
 * ctk_menu_shell_cancel:
 * @menu_shell: a #CtkMenuShell
 *
 * Cancels the selection within the menu shell.
 *
 * Since: 2.4
 */
void
ctk_menu_shell_cancel (CtkMenuShell *menu_shell)
{
  g_return_if_fail (CTK_IS_MENU_SHELL (menu_shell));

  g_signal_emit (menu_shell, menu_shell_signals[CANCEL], 0);
}

static CtkMnemonicHash *
ctk_menu_shell_get_mnemonic_hash (CtkMenuShell *menu_shell,
                                  gboolean      create)
{
  CtkMenuShellPrivate *priv = menu_shell->priv;

  if (!priv->mnemonic_hash && create)
    priv->mnemonic_hash = _ctk_mnemonic_hash_new ();
  
  return priv->mnemonic_hash;
}

static void
menu_shell_add_mnemonic_foreach (guint     keyval,
                                 GSList   *targets,
                                 gpointer  data)
{
  CtkKeyHash *key_hash = data;

  _ctk_key_hash_add_entry (key_hash, keyval, 0, GUINT_TO_POINTER (keyval));
}

static CtkKeyHash *
ctk_menu_shell_get_key_hash (CtkMenuShell *menu_shell,
                             gboolean      create)
{
  CtkMenuShellPrivate *priv = menu_shell->priv;
  CtkWidget *widget = CTK_WIDGET (menu_shell);

  if (!priv->key_hash && create && ctk_widget_has_screen (widget))
    {
      CtkMnemonicHash *mnemonic_hash = ctk_menu_shell_get_mnemonic_hash (menu_shell, FALSE);
      GdkScreen *screen = ctk_widget_get_screen (widget);
      GdkKeymap *keymap = cdk_keymap_get_for_display (cdk_screen_get_display (screen));

      if (!mnemonic_hash)
        return NULL;

      priv->key_hash = _ctk_key_hash_new (keymap, NULL);

      _ctk_mnemonic_hash_foreach (mnemonic_hash,
                                  menu_shell_add_mnemonic_foreach,
                                  priv->key_hash);
    }

  return priv->key_hash;
}

static void
ctk_menu_shell_reset_key_hash (CtkMenuShell *menu_shell)
{
  CtkMenuShellPrivate *priv = menu_shell->priv;

  if (priv->key_hash)
    {
      _ctk_key_hash_free (priv->key_hash);
      priv->key_hash = NULL;
    }
}

static gboolean
ctk_menu_shell_activate_mnemonic (CtkMenuShell *menu_shell,
                                  GdkEventKey  *event)
{
  CtkMnemonicHash *mnemonic_hash;
  CtkKeyHash *key_hash;
  GSList *entries;
  gboolean result = FALSE;

  mnemonic_hash = ctk_menu_shell_get_mnemonic_hash (menu_shell, FALSE);
  if (!mnemonic_hash)
    return FALSE;

  key_hash = ctk_menu_shell_get_key_hash (menu_shell, TRUE);
  if (!key_hash)
    return FALSE;

  entries = _ctk_key_hash_lookup (key_hash,
                                  event->hardware_keycode,
                                  event->state,
                                  ctk_accelerator_get_default_mod_mask (),
                                  event->group);

  if (entries)
    {
      result = _ctk_mnemonic_hash_activate (mnemonic_hash,
                                            GPOINTER_TO_UINT (entries->data));
      g_slist_free (entries);
    }

  return result;
}

void
_ctk_menu_shell_add_mnemonic (CtkMenuShell *menu_shell,
                              guint         keyval,
                              CtkWidget    *target)
{
  g_return_if_fail (CTK_IS_MENU_SHELL (menu_shell));
  g_return_if_fail (CTK_IS_WIDGET (target));

  _ctk_mnemonic_hash_add (ctk_menu_shell_get_mnemonic_hash (menu_shell, TRUE),
                          keyval, target);
  ctk_menu_shell_reset_key_hash (menu_shell);
}

void
_ctk_menu_shell_remove_mnemonic (CtkMenuShell *menu_shell,
                                 guint         keyval,
                                 CtkWidget    *target)
{
  g_return_if_fail (CTK_IS_MENU_SHELL (menu_shell));
  g_return_if_fail (CTK_IS_WIDGET (target));

  _ctk_mnemonic_hash_remove (ctk_menu_shell_get_mnemonic_hash (menu_shell, TRUE),
                             keyval, target);
  ctk_menu_shell_reset_key_hash (menu_shell);
}

void
_ctk_menu_shell_set_grab_device (CtkMenuShell *menu_shell,
                                 GdkDevice    *device)
{
  CtkMenuShellPrivate *priv;

  g_return_if_fail (CTK_IS_MENU_SHELL (menu_shell));
  g_return_if_fail (device == NULL || GDK_IS_DEVICE (device));

  priv = menu_shell->priv;

  if (!device)
    priv->grab_pointer = NULL;
  else if (cdk_device_get_source (device) == GDK_SOURCE_KEYBOARD)
    priv->grab_pointer = cdk_device_get_associated_device (device);
  else
    priv->grab_pointer = device;
}

GdkDevice *
_ctk_menu_shell_get_grab_device (CtkMenuShell *menu_shell)
{
  g_return_val_if_fail (CTK_IS_MENU_SHELL (menu_shell), NULL);

  return menu_shell->priv->grab_pointer;
}

/**
 * ctk_menu_shell_get_take_focus:
 * @menu_shell: a #CtkMenuShell
 *
 * Returns %TRUE if the menu shell will take the keyboard focus on popup.
 *
 * Returns: %TRUE if the menu shell will take the keyboard focus on popup.
 *
 * Since: 2.8
 */
gboolean
ctk_menu_shell_get_take_focus (CtkMenuShell *menu_shell)
{
  g_return_val_if_fail (CTK_IS_MENU_SHELL (menu_shell), FALSE);

  return menu_shell->priv->take_focus;
}

/**
 * ctk_menu_shell_set_take_focus:
 * @menu_shell: a #CtkMenuShell
 * @take_focus: %TRUE if the menu shell should take the keyboard
 *     focus on popup
 *
 * If @take_focus is %TRUE (the default) the menu shell will take
 * the keyboard focus so that it will receive all keyboard events
 * which is needed to enable keyboard navigation in menus.
 *
 * Setting @take_focus to %FALSE is useful only for special applications
 * like virtual keyboard implementations which should not take keyboard
 * focus.
 *
 * The @take_focus state of a menu or menu bar is automatically
 * propagated to submenus whenever a submenu is popped up, so you
 * don’t have to worry about recursively setting it for your entire
 * menu hierarchy. Only when programmatically picking a submenu and
 * popping it up manually, the @take_focus property of the submenu
 * needs to be set explicitly.
 *
 * Note that setting it to %FALSE has side-effects:
 *
 * If the focus is in some other app, it keeps the focus and keynav in
 * the menu doesn’t work. Consequently, keynav on the menu will only
 * work if the focus is on some toplevel owned by the onscreen keyboard.
 *
 * To avoid confusing the user, menus with @take_focus set to %FALSE
 * should not display mnemonics or accelerators, since it cannot be
 * guaranteed that they will work.
 *
 * See also cdk_keyboard_grab()
 *
 * Since: 2.8
 */
void
ctk_menu_shell_set_take_focus (CtkMenuShell *menu_shell,
                               gboolean      take_focus)
{
  CtkMenuShellPrivate *priv;

  g_return_if_fail (CTK_IS_MENU_SHELL (menu_shell));

  priv = menu_shell->priv;

  take_focus = !!take_focus;
  if (priv->take_focus != take_focus)
    {
      priv->take_focus = take_focus;
      g_object_notify (G_OBJECT (menu_shell), "take-focus");
    }
}

/**
 * ctk_menu_shell_get_selected_item:
 * @menu_shell: a #CtkMenuShell
 *
 * Gets the currently selected item.
 *
 * Returns: (transfer none): the currently selected item
 *
 * Since: 3.0
 */
CtkWidget *
ctk_menu_shell_get_selected_item (CtkMenuShell *menu_shell)
{
  g_return_val_if_fail (CTK_IS_MENU_SHELL (menu_shell), NULL);

  return menu_shell->priv->active_menu_item;
}

/**
 * ctk_menu_shell_get_parent_shell:
 * @menu_shell: a #CtkMenuShell
 *
 * Gets the parent menu shell.
 *
 * The parent menu shell of a submenu is the #CtkMenu or #CtkMenuBar
 * from which it was opened up.
 *
 * Returns: (transfer none): the parent #CtkMenuShell
 *
 * Since: 3.0
 */
CtkWidget *
ctk_menu_shell_get_parent_shell (CtkMenuShell *menu_shell)
{
  g_return_val_if_fail (CTK_IS_MENU_SHELL (menu_shell), NULL);

  return menu_shell->priv->parent_menu_shell;
}

static void
ctk_menu_shell_item_activate (CtkMenuItem *menuitem,
                              gpointer     user_data)
{
  CtkMenuTrackerItem *item = user_data;

  ctk_menu_tracker_item_activated (item);
}

static void
ctk_menu_shell_submenu_shown (CtkWidget *submenu,
                              gpointer   user_data)
{
  CtkMenuTrackerItem *item = user_data;

  ctk_menu_tracker_item_request_submenu_shown (item, TRUE);
}

static void
ctk_menu_shell_submenu_hidden (CtkWidget *submenu,
                               gpointer   user_data)
{
  CtkMenuTrackerItem *item = user_data;

  if (!CTK_MENU_SHELL (submenu)->priv->selection_done_coming_soon)
    ctk_menu_tracker_item_request_submenu_shown (item, FALSE);
}

static void
ctk_menu_shell_submenu_selection_done (CtkWidget *submenu,
                                       gpointer   user_data)
{
  CtkMenuTrackerItem *item = user_data;

  if (CTK_MENU_SHELL (submenu)->priv->selection_done_coming_soon)
    ctk_menu_tracker_item_request_submenu_shown (item, FALSE);
}

static void
ctk_menu_shell_tracker_remove_func (gint     position,
                                    gpointer user_data)
{
  CtkMenuShell *menu_shell = user_data;
  CtkWidget *child;

  child = g_list_nth_data (menu_shell->priv->children, position);
  /* We use destroy here because in the case of an item with a submenu,
   * the attached-to from the submenu holds a ref on the item and a
   * simple ctk_container_remove() isn't good enough to break that.
   */
  ctk_widget_destroy (child);
}

static void
ctk_menu_shell_tracker_insert_func (CtkMenuTrackerItem *item,
                                    gint                position,
                                    gpointer            user_data)
{
  CtkMenuShell *menu_shell = user_data;
  CtkWidget *widget;

  if (ctk_menu_tracker_item_get_is_separator (item))
    {
      const gchar *label;

      widget = ctk_separator_menu_item_new ();

      /* For separators, we may have a section heading, so check the
       * "label" property.
       *
       * Note: we only do this once, and we only do it if the label is
       * non-NULL because even setting a NULL label on the separator
       * will be enough to create a CtkLabel and add it, changing the
       * appearance in the process.
       */

      label = ctk_menu_tracker_item_get_label (item);
      if (label)
        ctk_menu_item_set_label (CTK_MENU_ITEM (widget), label);

      ctk_widget_show (widget);
    }
  else if (ctk_menu_tracker_item_get_has_link (item, G_MENU_LINK_SUBMENU))
    {
      CtkMenuShell *submenu;

      widget = ctk_model_menu_item_new ();
      g_object_bind_property (item, "label", widget, "text", G_BINDING_SYNC_CREATE);

      submenu = CTK_MENU_SHELL (ctk_menu_new ());

      /* We recurse directly here: we could use an idle instead to
       * prevent arbitrary recursion depth.  We could also do it
       * lazy...
       */
      submenu->priv->tracker = ctk_menu_tracker_new_for_item_link (item,
                                                                   G_MENU_LINK_SUBMENU, TRUE, FALSE,
                                                                   ctk_menu_shell_tracker_insert_func,
                                                                   ctk_menu_shell_tracker_remove_func,
                                                                   submenu);
      ctk_menu_item_set_submenu (CTK_MENU_ITEM (widget), CTK_WIDGET (submenu));

      if (ctk_menu_tracker_item_get_should_request_show (item))
        {
          /* We don't request show in the strictest sense of the
           * word: we just notify when we are showing and don't
           * bother waiting for the reply.
           *
           * This could be fixed one day, but it would be slightly
           * complicated and would have a strange interaction with
           * the submenu pop-up delay.
           *
           * Note: 'item' is already kept alive from above.
           */
          g_signal_connect (submenu, "show", G_CALLBACK (ctk_menu_shell_submenu_shown), item);
          g_signal_connect (submenu, "hide", G_CALLBACK (ctk_menu_shell_submenu_hidden), item);
          g_signal_connect (submenu, "selection-done", G_CALLBACK (ctk_menu_shell_submenu_selection_done), item);
        }

      ctk_widget_show (widget);
    }
  else
    {
      widget = ctk_model_menu_item_new ();

      /* We bind to "text" instead of "label" because CtkModelMenuItem
       * uses this property (along with "icon") to control its child
       * widget.  Once this is merged into CtkMenuItem we can go back to
       * using "label".
       */
      g_object_bind_property (item, "label", widget, "text", G_BINDING_SYNC_CREATE);
      g_object_bind_property (item, "icon", widget, "icon", G_BINDING_SYNC_CREATE);
      g_object_bind_property (item, "sensitive", widget, "sensitive", G_BINDING_SYNC_CREATE);
      g_object_bind_property (item, "role", widget, "action-role", G_BINDING_SYNC_CREATE);
      g_object_bind_property (item, "toggled", widget, "toggled", G_BINDING_SYNC_CREATE);
      g_object_bind_property (item, "accel", widget, "accel", G_BINDING_SYNC_CREATE);

      g_signal_connect (widget, "activate", G_CALLBACK (ctk_menu_shell_item_activate), item);
      ctk_widget_show (widget);
    }

  /* TODO: drop this when we have bindings that ref the source */
  g_object_set_data_full (G_OBJECT (widget), "CtkMenuTrackerItem", g_object_ref (item), g_object_unref);

  ctk_menu_shell_insert (menu_shell, widget, position);
}

/**
 * ctk_menu_shell_bind_model:
 * @menu_shell: a #CtkMenuShell
 * @model: (allow-none): the #GMenuModel to bind to or %NULL to remove
 *   binding
 * @action_namespace: (allow-none): the namespace for actions in @model
 * @with_separators: %TRUE if toplevel items in @shell should have
 *   separators between them
 *
 * Establishes a binding between a #CtkMenuShell and a #GMenuModel.
 *
 * The contents of @shell are removed and then refilled with menu items
 * according to @model.  When @model changes, @shell is updated.
 * Calling this function twice on @shell with different @model will
 * cause the first binding to be replaced with a binding to the new
 * model. If @model is %NULL then any previous binding is undone and
 * all children are removed.
 *
 * @with_separators determines if toplevel items (eg: sections) have
 * separators inserted between them.  This is typically desired for
 * menus but doesn’t make sense for menubars.
 *
 * If @action_namespace is non-%NULL then the effect is as if all
 * actions mentioned in the @model have their names prefixed with the
 * namespace, plus a dot.  For example, if the action “quit” is
 * mentioned and @action_namespace is “app” then the effective action
 * name is “app.quit”.
 *
 * This function uses #CtkActionable to define the action name and
 * target values on the created menu items.  If you want to use an
 * action group other than “app” and “win”, or if you want to use a
 * #CtkMenuShell outside of a #CtkApplicationWindow, then you will need
 * to attach your own action group to the widget hierarchy using
 * ctk_widget_insert_action_group().  As an example, if you created a
 * group with a “quit” action and inserted it with the name “mygroup”
 * then you would use the action name “mygroup.quit” in your
 * #GMenuModel.
 *
 * For most cases you are probably better off using
 * ctk_menu_new_from_model() or ctk_menu_bar_new_from_model() or just
 * directly passing the #GMenuModel to ctk_application_set_app_menu() or
 * ctk_application_set_menubar().
 *
 * Since: 3.6
 */
void
ctk_menu_shell_bind_model (CtkMenuShell *menu_shell,
                           GMenuModel   *model,
                           const gchar  *action_namespace,
                           gboolean      with_separators)
{
  CtkActionMuxer *muxer;

  g_return_if_fail (CTK_IS_MENU_SHELL (menu_shell));
  g_return_if_fail (model == NULL || G_IS_MENU_MODEL (model));

  muxer = _ctk_widget_get_action_muxer (CTK_WIDGET (menu_shell), TRUE);

  g_clear_pointer (&menu_shell->priv->tracker, ctk_menu_tracker_free);

  while (menu_shell->priv->children)
    ctk_container_remove (CTK_CONTAINER (menu_shell), menu_shell->priv->children->data);

  if (model)
    menu_shell->priv->tracker = ctk_menu_tracker_new (CTK_ACTION_OBSERVABLE (muxer), model,
                                                      with_separators, TRUE, FALSE, action_namespace,
                                                      ctk_menu_shell_tracker_insert_func,
                                                      ctk_menu_shell_tracker_remove_func,
                                                      menu_shell);
}
