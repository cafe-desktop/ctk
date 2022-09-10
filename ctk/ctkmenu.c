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
 * SECTION:ctkmenu
 * @Short_description: A menu widget
 * @Title: CtkMenu
 *
 * A #CtkMenu is a #CtkMenuShell that implements a drop down menu
 * consisting of a list of #CtkMenuItem objects which can be navigated
 * and activated by the user to perform application functions.
 *
 * A #CtkMenu is most commonly dropped down by activating a
 * #CtkMenuItem in a #CtkMenuBar or popped up by activating a
 * #CtkMenuItem in another #CtkMenu.
 *
 * A #CtkMenu can also be popped up by activating a #CtkComboBox.
 * Other composite widgets such as the #CtkNotebook can pop up a
 * #CtkMenu as well.
 *
 * Applications can display a #CtkMenu as a popup menu by calling the 
 * ctk_menu_popup() function.  The example below shows how an application
 * can pop up a menu when the 3rd mouse button is pressed.  
 *
 * ## Connecting the popup signal handler.
 *
 * |[<!-- language="C" -->
 *   // connect our handler which will popup the menu
 *   g_signal_connect_swapped (window, "button_press_event",
 *	G_CALLBACK (my_popup_handler), menu);
 * ]|
 *
 * ## Signal handler which displays a popup menu.
 *
 * |[<!-- language="C" -->
 * static gint
 * my_popup_handler (CtkWidget *widget, CdkEvent *event)
 * {
 *   CtkMenu *menu;
 *   CdkEventButton *event_button;
 *
 *   g_return_val_if_fail (widget != NULL, FALSE);
 *   g_return_val_if_fail (CTK_IS_MENU (widget), FALSE);
 *   g_return_val_if_fail (event != NULL, FALSE);
 *
 *   // The "widget" is the menu that was supplied when 
 *   // g_signal_connect_swapped() was called.
 *   menu = CTK_MENU (widget);
 *
 *   if (event->type == CDK_BUTTON_PRESS)
 *     {
 *       event_button = (CdkEventButton *) event;
 *       if (event_button->button == CDK_BUTTON_SECONDARY)
 *         {
 *           ctk_menu_popup (menu, NULL, NULL, NULL, NULL, 
 *                           event_button->button, event_button->time);
 *           return TRUE;
 *         }
 *     }
 *
 *   return FALSE;
 * }
 * ]|
 *
 * # CSS nodes
 *
 * |[<!-- language="plain" -->
 * menu
 * ├── arrow.top
 * ├── <child>
 * ┊
 * ├── <child>
 * ╰── arrow.bottom
 * ]|
 *
 * The main CSS node of CtkMenu has name menu, and there are two subnodes
 * with name arrow, for scrolling menu arrows. These subnodes get the
 * .top and .bottom style classes.
 */

#include "config.h"

#include <math.h>
#include <string.h>

#include  <gobject/gvaluecollector.h>

#include "ctkaccellabel.h"
#include "ctkaccelmap.h"
#include "ctkadjustment.h"
#include "ctkbindings.h"
#include "ctkbuiltiniconprivate.h"
#include "ctkcheckmenuitem.h"
#include "ctkcheckmenuitemprivate.h"
#include "ctkmain.h"
#include "ctkmarshalers.h"
#include "ctkmenuprivate.h"
#include "ctkmenuitemprivate.h"
#include "ctkmenushellprivate.h"
#include "ctkwindow.h"
#include "ctkbox.h"
#include "ctkscrollbar.h"
#include "ctksettings.h"
#include "ctkprivate.h"
#include "ctkwidgetpath.h"
#include "ctkwidgetprivate.h"
#include "ctkdnd.h"
#include "ctkintl.h"
#include "ctktypebuiltins.h"
#include "ctkwidgetprivate.h"
#include "ctkwindowprivate.h"
#include "ctkcssnodeprivate.h"
#include "ctkstylecontextprivate.h"
#include "ctkcssstylepropertyprivate.h"
#include "ctktooltipprivate.h"

#include "ctktearoffmenuitem.h"


#include "a11y/ctkmenuaccessible.h"
#include "cdk/cdk-private.h"

#define NAVIGATION_REGION_OVERSHOOT 50  /* How much the navigation region
                                         * extends below the submenu
                                         */

#define MENU_SCROLL_STEP1      8
#define MENU_SCROLL_STEP2     15
#define MENU_SCROLL_FAST_ZONE  8
#define MENU_SCROLL_TIMEOUT1  50
#define MENU_SCROLL_TIMEOUT2  20

#define MENU_POPUP_DELAY     225
#define MENU_POPDOWN_DELAY  1000

#define ATTACH_INFO_KEY "ctk-menu-child-attach-info-key"
#define ATTACHED_MENUS "ctk-attached-menus"

typedef struct _CtkMenuAttachData  CtkMenuAttachData;
typedef struct _CtkMenuPopdownData CtkMenuPopdownData;

struct _CtkMenuAttachData
{
  CtkWidget *attach_widget;
  CtkMenuDetachFunc detacher;
};

struct _CtkMenuPopdownData
{
  CtkMenu *menu;
  CdkDevice *device;
};

typedef struct
{
  gint left_attach;
  gint right_attach;
  gint top_attach;
  gint bottom_attach;
  gint effective_left_attach;
  gint effective_right_attach;
  gint effective_top_attach;
  gint effective_bottom_attach;
} AttachInfo;

enum {
  MOVE_SCROLL,
  POPPED_UP,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_ACTIVE,
  PROP_ACCEL_GROUP,
  PROP_ACCEL_PATH,
  PROP_ATTACH_WIDGET,
  PROP_TEAROFF_STATE,
  PROP_TEAROFF_TITLE,
  PROP_MONITOR,
  PROP_RESERVE_TOGGLE_SIZE,
  PROP_ANCHOR_HINTS,
  PROP_RECT_ANCHOR_DX,
  PROP_RECT_ANCHOR_DY,
  PROP_MENU_TYPE_HINT
};

enum {
  CHILD_PROP_0,
  CHILD_PROP_LEFT_ATTACH,
  CHILD_PROP_RIGHT_ATTACH,
  CHILD_PROP_TOP_ATTACH,
  CHILD_PROP_BOTTOM_ATTACH
};

typedef enum _CtkMenuScrollFlag
{
  CTK_MENU_SCROLL_FLAG_NONE = 0,
  CTK_MENU_SCROLL_FLAG_ADAPT = 1 << 0,
} CtkMenuScrollFlag;

static void     ctk_menu_set_property      (GObject          *object,
                                            guint             prop_id,
                                            const GValue     *value,
                                            GParamSpec       *pspec);
static void     ctk_menu_get_property      (GObject          *object,
                                            guint             prop_id,
                                            GValue           *value,
                                            GParamSpec       *pspec);
static void     ctk_menu_finalize          (GObject          *object);
static void     ctk_menu_set_child_property(CtkContainer     *container,
                                            CtkWidget        *child,
                                            guint             property_id,
                                            const GValue     *value,
                                            GParamSpec       *pspec);
static void     ctk_menu_get_child_property(CtkContainer     *container,
                                            CtkWidget        *child,
                                            guint             property_id,
                                            GValue           *value,
                                            GParamSpec       *pspec);
static void     ctk_menu_destroy           (CtkWidget        *widget);
static void     ctk_menu_realize           (CtkWidget        *widget);
static void     ctk_menu_unrealize         (CtkWidget        *widget);
static void     ctk_menu_size_allocate     (CtkWidget        *widget,
                                            CtkAllocation    *allocation);
static void     ctk_menu_show              (CtkWidget        *widget);
static gboolean ctk_menu_draw              (CtkWidget        *widget,
                                            cairo_t          *cr);
static gboolean ctk_menu_key_press         (CtkWidget        *widget,
                                            CdkEventKey      *event);
static gboolean ctk_menu_scroll            (CtkWidget        *widget,
                                            CdkEventScroll   *event);
static gboolean ctk_menu_button_press      (CtkWidget        *widget,
                                            CdkEventButton   *event);
static gboolean ctk_menu_button_release    (CtkWidget        *widget,
                                            CdkEventButton   *event);
static gboolean ctk_menu_motion_notify     (CtkWidget        *widget,
                                            CdkEventMotion   *event);
static gboolean ctk_menu_enter_notify      (CtkWidget        *widget,
                                            CdkEventCrossing *event);
static gboolean ctk_menu_leave_notify      (CtkWidget        *widget,
                                            CdkEventCrossing *event);
static void     ctk_menu_scroll_to         (CtkMenu          *menu,
                                            gint              offset,
                                            CtkMenuScrollFlag flags);
static void     ctk_menu_grab_notify       (CtkWidget        *widget,
                                            gboolean          was_grabbed);
static gboolean ctk_menu_captured_event    (CtkWidget        *widget,
                                            CdkEvent         *event);


static void     ctk_menu_stop_scrolling         (CtkMenu  *menu);
static void     ctk_menu_remove_scroll_timeout  (CtkMenu  *menu);
static gboolean ctk_menu_scroll_timeout         (gpointer  data);

static void     ctk_menu_scroll_item_visible (CtkMenuShell    *menu_shell,
                                              CtkWidget       *menu_item);
static void     ctk_menu_select_item       (CtkMenuShell     *menu_shell,
                                            CtkWidget        *menu_item);
static void     ctk_menu_real_insert       (CtkMenuShell     *menu_shell,
                                            CtkWidget        *child,
                                            gint              position);
static void     ctk_menu_scrollbar_changed (CtkAdjustment    *adjustment,
                                            CtkMenu          *menu);
static void     ctk_menu_handle_scrolling  (CtkMenu          *menu,
                                            gint              event_x,
                                            gint              event_y,
                                            gboolean          enter,
                                            gboolean          motion);
static void     ctk_menu_set_tearoff_hints (CtkMenu          *menu,
                                            gint             width);
static gboolean ctk_menu_focus             (CtkWidget        *widget,
                                            CtkDirectionType direction);
static gint     ctk_menu_get_popup_delay   (CtkMenuShell     *menu_shell);
static void     ctk_menu_move_current      (CtkMenuShell     *menu_shell,
                                            CtkMenuDirectionType direction);
static void     ctk_menu_real_move_scroll  (CtkMenu          *menu,
                                            CtkScrollType     type);

static void     ctk_menu_stop_navigating_submenu       (CtkMenu          *menu);
static gboolean ctk_menu_stop_navigating_submenu_cb    (gpointer          user_data);
static gboolean ctk_menu_navigating_submenu            (CtkMenu          *menu,
                                                        gint              event_x,
                                                        gint              event_y);
static void     ctk_menu_set_submenu_navigation_region (CtkMenu          *menu,
                                                        CtkMenuItem      *menu_item,
                                                        CdkEventCrossing *event);
 
static void ctk_menu_deactivate     (CtkMenuShell      *menu_shell);
static void ctk_menu_show_all       (CtkWidget         *widget);
static void ctk_menu_position       (CtkMenu           *menu,
                                     gboolean           set_scroll_offset);
static void ctk_menu_reparent       (CtkMenu           *menu,
                                     CtkWidget         *new_parent,
                                     gboolean           unrealize);
static void ctk_menu_remove         (CtkContainer      *menu,
                                     CtkWidget         *widget);

static void ctk_menu_update_title   (CtkMenu           *menu);

static void       menu_grab_transfer_window_destroy (CtkMenu *menu);
static CdkWindow *menu_grab_transfer_window_get     (CtkMenu *menu);

static gboolean ctk_menu_real_can_activate_accel (CtkWidget *widget,
                                                  guint      signal_id);
static void _ctk_menu_refresh_accel_paths (CtkMenu *menu,
                                           gboolean group_changed);

static void ctk_menu_get_preferred_width            (CtkWidget           *widget,
                                                     gint                *minimum_size,
                                                     gint                *natural_size);
static void ctk_menu_get_preferred_height           (CtkWidget           *widget,
                                                     gint                *minimum_size,
                                                     gint                *natural_size);
static void ctk_menu_get_preferred_height_for_width (CtkWidget           *widget,
                                                     gint                 for_size,
                                                     gint                *minimum_size,
                                                     gint                *natural_size);


static const gchar attach_data_key[] = "ctk-menu-attach-data";

static guint menu_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (CtkMenu, ctk_menu, CTK_TYPE_MENU_SHELL)

static void
menu_queue_resize (CtkMenu *menu)
{
  CtkMenuPrivate *priv = menu->priv;

  priv->have_layout = FALSE;
  ctk_widget_queue_resize (CTK_WIDGET (menu));
}

static void
attach_info_free (AttachInfo *info)
{
  g_slice_free (AttachInfo, info);
}

static AttachInfo *
get_attach_info (CtkWidget *child)
{
  GObject *object = G_OBJECT (child);
  AttachInfo *ai = g_object_get_data (object, ATTACH_INFO_KEY);

  if (!ai)
    {
      ai = g_slice_new0 (AttachInfo);
      g_object_set_data_full (object, I_(ATTACH_INFO_KEY), ai,
                              (GDestroyNotify) attach_info_free);
    }

  return ai;
}

static gboolean
is_grid_attached (AttachInfo *ai)
{
  return (ai->left_attach >= 0 &&
          ai->right_attach >= 0 &&
          ai->top_attach >= 0 &&
          ai->bottom_attach >= 0);
}

static void
menu_ensure_layout (CtkMenu *menu)
{
  CtkMenuPrivate *priv = menu->priv;

  if (!priv->have_layout)
    {
      CtkMenuShell *menu_shell = CTK_MENU_SHELL (menu);
      GList *l;
      gchar *row_occupied;
      gint current_row;
      gint max_right_attach;
      gint max_bottom_attach;

      /* Find extents of gridded portion
       */
      max_right_attach = 1;
      max_bottom_attach = 0;

      for (l = menu_shell->priv->children; l; l = l->next)
        {
          CtkWidget *child = l->data;
          AttachInfo *ai = get_attach_info (child);

          if (is_grid_attached (ai))
            {
              max_bottom_attach = MAX (max_bottom_attach, ai->bottom_attach);
              max_right_attach = MAX (max_right_attach, ai->right_attach);
            }
        }

      /* Find empty rows */
      row_occupied = g_malloc0 (max_bottom_attach);

      for (l = menu_shell->priv->children; l; l = l->next)
        {
          CtkWidget *child = l->data;
          AttachInfo *ai = get_attach_info (child);

          if (is_grid_attached (ai))
            {
              gint i;

              for (i = ai->top_attach; i < ai->bottom_attach; i++)
                row_occupied[i] = TRUE;
            }
        }

      /* Lay non-grid-items out in those rows
       */
      current_row = 0;
      for (l = menu_shell->priv->children; l; l = l->next)
        {
          CtkWidget *child = l->data;
          AttachInfo *ai = get_attach_info (child);

          if (!is_grid_attached (ai))
            {
              while (current_row < max_bottom_attach && row_occupied[current_row])
                current_row++;

              ai->effective_left_attach = 0;
              ai->effective_right_attach = max_right_attach;
              ai->effective_top_attach = current_row;
              ai->effective_bottom_attach = current_row + 1;

              current_row++;
            }
          else
            {
              ai->effective_left_attach = ai->left_attach;
              ai->effective_right_attach = ai->right_attach;
              ai->effective_top_attach = ai->top_attach;
              ai->effective_bottom_attach = ai->bottom_attach;
            }
        }

      g_free (row_occupied);

      priv->n_rows = MAX (current_row, max_bottom_attach);
      priv->n_columns = max_right_attach;
      priv->have_layout = TRUE;
    }
}


static gint
ctk_menu_get_n_columns (CtkMenu *menu)
{
  CtkMenuPrivate *priv = menu->priv;

  menu_ensure_layout (menu);

  return priv->n_columns;
}

static gint
ctk_menu_get_n_rows (CtkMenu *menu)
{
  CtkMenuPrivate *priv = menu->priv;

  menu_ensure_layout (menu);

  return priv->n_rows;
}

static void
get_effective_child_attach (CtkWidget *child,
                            int       *l,
                            int       *r,
                            int       *t,
                            int       *b)
{
  CtkMenu *menu = CTK_MENU (ctk_widget_get_parent (child));
  AttachInfo *ai;
  
  menu_ensure_layout (menu);

  ai = get_attach_info (child);

  if (l)
    *l = ai->effective_left_attach;
  if (r)
    *r = ai->effective_right_attach;
  if (t)
    *t = ai->effective_top_attach;
  if (b)
    *b = ai->effective_bottom_attach;

}

static void
ctk_menu_class_init (CtkMenuClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (class);
  CtkContainerClass *container_class = CTK_CONTAINER_CLASS (class);
  CtkMenuShellClass *menu_shell_class = CTK_MENU_SHELL_CLASS (class);
  CtkBindingSet *binding_set;
  
  gobject_class->set_property = ctk_menu_set_property;
  gobject_class->get_property = ctk_menu_get_property;
  gobject_class->finalize = ctk_menu_finalize;

  widget_class->destroy = ctk_menu_destroy;
  widget_class->realize = ctk_menu_realize;
  widget_class->unrealize = ctk_menu_unrealize;
  widget_class->size_allocate = ctk_menu_size_allocate;
  widget_class->show = ctk_menu_show;
  widget_class->draw = ctk_menu_draw;
  widget_class->scroll_event = ctk_menu_scroll;
  widget_class->key_press_event = ctk_menu_key_press;
  widget_class->button_press_event = ctk_menu_button_press;
  widget_class->button_release_event = ctk_menu_button_release;
  widget_class->motion_notify_event = ctk_menu_motion_notify;
  widget_class->show_all = ctk_menu_show_all;
  widget_class->enter_notify_event = ctk_menu_enter_notify;
  widget_class->leave_notify_event = ctk_menu_leave_notify;
  widget_class->focus = ctk_menu_focus;
  widget_class->can_activate_accel = ctk_menu_real_can_activate_accel;
  widget_class->grab_notify = ctk_menu_grab_notify;
  widget_class->get_preferred_width = ctk_menu_get_preferred_width;
  widget_class->get_preferred_height = ctk_menu_get_preferred_height;
  widget_class->get_preferred_height_for_width = ctk_menu_get_preferred_height_for_width;

  container_class->remove = ctk_menu_remove;
  container_class->get_child_property = ctk_menu_get_child_property;
  container_class->set_child_property = ctk_menu_set_child_property;
  
  menu_shell_class->submenu_placement = CTK_LEFT_RIGHT;
  menu_shell_class->deactivate = ctk_menu_deactivate;
  menu_shell_class->select_item = ctk_menu_select_item;
  menu_shell_class->insert = ctk_menu_real_insert;
  menu_shell_class->get_popup_delay = ctk_menu_get_popup_delay;
  menu_shell_class->move_current = ctk_menu_move_current;

  /**
   * CtkMenu::move-scroll:
   * @menu: a #CtkMenu
   * @scroll_type: a #CtkScrollType
   */
  menu_signals[MOVE_SCROLL] =
    g_signal_new_class_handler (I_("move-scroll"),
                                G_OBJECT_CLASS_TYPE (gobject_class),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                G_CALLBACK (ctk_menu_real_move_scroll),
                                NULL, NULL,
                                NULL,
                                G_TYPE_NONE, 1,
                                CTK_TYPE_SCROLL_TYPE);

  /**
   * CtkMenu::popped-up:
   * @menu: the #CtkMenu that popped up
   * @flipped_rect: (nullable): the position of @menu after any possible
   *                flipping or %NULL if the backend can't obtain it
   * @final_rect: (nullable): the final position of @menu or %NULL if the
   *              backend can't obtain it
   * @flipped_x: %TRUE if the anchors were flipped horizontally
   * @flipped_y: %TRUE if the anchors were flipped vertically
   *
   * Emitted when the position of @menu is finalized after being popped up
   * using ctk_menu_popup_at_rect (), ctk_menu_popup_at_widget (), or
   * ctk_menu_popup_at_pointer ().
   *
   * @menu might be flipped over the anchor rectangle in order to keep it
   * on-screen, in which case @flipped_x and @flipped_y will be set to %TRUE
   * accordingly.
   *
   * @flipped_rect is the ideal position of @menu after any possible flipping,
   * but before any possible sliding. @final_rect is @flipped_rect, but possibly
   * translated in the case that flipping is still ineffective in keeping @menu
   * on-screen.
   *
   * ![](popup-slide.png)
   *
   * The blue menu is @menu's ideal position, the green menu is @flipped_rect,
   * and the red menu is @final_rect.
   *
   * See ctk_menu_popup_at_rect (), ctk_menu_popup_at_widget (),
   * ctk_menu_popup_at_pointer (), #CtkMenu:anchor-hints,
   * #CtkMenu:rect-anchor-dx, #CtkMenu:rect-anchor-dy, and
   * #CtkMenu:menu-type-hint.
   *
   * Since: 3.22
   */
  menu_signals[POPPED_UP] =
    g_signal_new_class_handler (I_("popped-up"),
                                G_OBJECT_CLASS_TYPE (gobject_class),
                                G_SIGNAL_RUN_FIRST,
                                NULL,
                                NULL,
                                NULL,
                                _ctk_marshal_VOID__POINTER_POINTER_BOOLEAN_BOOLEAN,
                                G_TYPE_NONE,
                                4,
                                G_TYPE_POINTER,
                                G_TYPE_POINTER,
                                G_TYPE_BOOLEAN,
                                G_TYPE_BOOLEAN);

  /**
   * CtkMenu:active:
   *
   * The index of the currently selected menu item, or -1 if no
   * menu item is selected.
   *
   * Since: 2.14
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACTIVE,
                                   g_param_spec_int ("active",
                                                     P_("Active"),
                                                     P_("The currently selected menu item"),
                                                     -1, G_MAXINT, -1,
                                                     CTK_PARAM_READWRITE));

  /**
   * CtkMenu:accel-group:
   *
   * The accel group holding accelerators for the menu.
   *
   * Since: 2.14
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_GROUP,
                                   g_param_spec_object ("accel-group",
                                                        P_("Accel Group"),
                                                        P_("The accel group holding accelerators for the menu"),
                                                        CTK_TYPE_ACCEL_GROUP,
                                                        CTK_PARAM_READWRITE));

  /**
   * CtkMenu:accel-path:
   *
   * An accel path used to conveniently construct accel paths of child items.
   *
   * Since: 2.14
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_PATH,
                                   g_param_spec_string ("accel-path",
                                                        P_("Accel Path"),
                                                        P_("An accel path used to conveniently construct accel paths of child items"),
                                                        NULL,
                                                        CTK_PARAM_READWRITE));

  /**
   * CtkMenu:attach-widget:
   *
   * The widget the menu is attached to. Setting this property attaches
   * the menu without a #CtkMenuDetachFunc. If you need to use a detacher,
   * use ctk_menu_attach_to_widget() directly.
   *
   * Since: 2.14
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ATTACH_WIDGET,
                                   g_param_spec_object ("attach-widget",
                                                        P_("Attach Widget"),
                                                        P_("The widget the menu is attached to"),
                                                        CTK_TYPE_WIDGET,
                                                        CTK_PARAM_READWRITE));

  /**
   * CtkMenu:tearoff-title:
   *
   * A title that may be displayed by the window manager when this
   * menu is torn-off.
   *
   * Deprecated: 3.10
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_TEAROFF_TITLE,
                                   g_param_spec_string ("tearoff-title",
                                                        P_("Tearoff Title"),
                                                        P_("A title that may be displayed by the window manager when this menu is torn-off"),
                                                        NULL,
                                                        CTK_PARAM_READWRITE));

  /**
   * CtkMenu:tearoff-state:
   *
   * A boolean that indicates whether the menu is torn-off.
   *
   * Since: 2.6
   *
   * Deprecated: 3.10
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_TEAROFF_STATE,
                                   g_param_spec_boolean ("tearoff-state",
                                                         P_("Tearoff State"),
                                                         P_("A boolean that indicates whether the menu is torn-off"),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE));

  /**
   * CtkMenu:monitor:
   *
   * The monitor the menu will be popped up on.
   *
   * Since: 2.14
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_MONITOR,
                                   g_param_spec_int ("monitor",
                                                     P_("Monitor"),
                                                     P_("The monitor the menu will be popped up on"),
                                                     -1, G_MAXINT, -1,
                                                     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkMenu:reserve-toggle-size:
   *
   * A boolean that indicates whether the menu reserves space for
   * toggles and icons, regardless of their actual presence.
   *
   * This property should only be changed from its default value
   * for special-purposes such as tabular menus. Regular menus that
   * are connected to a menu bar or context menus should reserve
   * toggle space for consistency.
   *
   * Since: 2.18
   */
  g_object_class_install_property (gobject_class,
                                   PROP_RESERVE_TOGGLE_SIZE,
                                   g_param_spec_boolean ("reserve-toggle-size",
                                                         P_("Reserve Toggle Size"),
                                                         P_("A boolean that indicates whether the menu reserves space for toggles and icons"),
                                                         TRUE,
                                                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkMenu:anchor-hints:
   *
   * Positioning hints for aligning the menu relative to a rectangle.
   *
   * These hints determine how the menu should be positioned in the case that
   * the menu would fall off-screen if placed in its ideal position.
   *
   * ![](popup-flip.png)
   *
   * For example, %CDK_ANCHOR_FLIP_Y will replace %CDK_GRAVITY_NORTH_WEST with
   * %CDK_GRAVITY_SOUTH_WEST and vice versa if the menu extends beyond the
   * bottom edge of the monitor.
   *
   * See ctk_menu_popup_at_rect (), ctk_menu_popup_at_widget (),
   * ctk_menu_popup_at_pointer (), #CtkMenu:rect-anchor-dx,
   * #CtkMenu:rect-anchor-dy, #CtkMenu:menu-type-hint, and #CtkMenu::popped-up.
   *
   * Since: 3.22
   */
  g_object_class_install_property (gobject_class,
                                   PROP_ANCHOR_HINTS,
                                   g_param_spec_flags ("anchor-hints",
                                                       P_("Anchor hints"),
                                                       P_("Positioning hints for when the menu might fall off-screen"),
                                                       CDK_TYPE_ANCHOR_HINTS,
                                                       CDK_ANCHOR_FLIP |
                                                       CDK_ANCHOR_SLIDE |
                                                       CDK_ANCHOR_RESIZE,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT |
                                                       G_PARAM_STATIC_NAME |
                                                       G_PARAM_STATIC_NICK |
                                                       G_PARAM_STATIC_BLURB |
                                                       G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkMenu:rect-anchor-dx:
   *
   * Horizontal offset to apply to the menu, i.e. the rectangle or widget
   * anchor.
   *
   * See ctk_menu_popup_at_rect (), ctk_menu_popup_at_widget (),
   * ctk_menu_popup_at_pointer (), #CtkMenu:anchor-hints,
   * #CtkMenu:rect-anchor-dy, #CtkMenu:menu-type-hint, and #CtkMenu::popped-up.
   *
   * Since: 3.22
   */
  g_object_class_install_property (gobject_class,
                                   PROP_RECT_ANCHOR_DX,
                                   g_param_spec_int ("rect-anchor-dx",
                                                     P_("Rect anchor dx"),
                                                     P_("Rect anchor horizontal offset"),
                                                     G_MININT,
                                                     G_MAXINT,
                                                     0,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT |
                                                     G_PARAM_STATIC_NAME |
                                                     G_PARAM_STATIC_NICK |
                                                     G_PARAM_STATIC_BLURB |
                                                     G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkMenu:rect-anchor-dy:
   *
   * Vertical offset to apply to the menu, i.e. the rectangle or widget anchor.
   *
   * See ctk_menu_popup_at_rect (), ctk_menu_popup_at_widget (),
   * ctk_menu_popup_at_pointer (), #CtkMenu:anchor-hints,
   * #CtkMenu:rect-anchor-dx, #CtkMenu:menu-type-hint, and #CtkMenu::popped-up.
   *
   * Since: 3.22
   */
  g_object_class_install_property (gobject_class,
                                   PROP_RECT_ANCHOR_DY,
                                   g_param_spec_int ("rect-anchor-dy",
                                                     P_("Rect anchor dy"),
                                                     P_("Rect anchor vertical offset"),
                                                     G_MININT,
                                                     G_MAXINT,
                                                     0,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT |
                                                     G_PARAM_STATIC_NAME |
                                                     G_PARAM_STATIC_NICK |
                                                     G_PARAM_STATIC_BLURB |
                                                     G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkMenu:menu-type-hint:
   *
   * The #CdkWindowTypeHint to use for the menu's #CdkWindow.
   *
   * See ctk_menu_popup_at_rect (), ctk_menu_popup_at_widget (),
   * ctk_menu_popup_at_pointer (), #CtkMenu:anchor-hints,
   * #CtkMenu:rect-anchor-dx, #CtkMenu:rect-anchor-dy, and #CtkMenu::popped-up.
   *
   * Since: 3.22
   */
  g_object_class_install_property (gobject_class,
                                   PROP_MENU_TYPE_HINT,
                                   g_param_spec_enum ("menu-type-hint",
                                                      P_("Menu type hint"),
                                                      P_("Menu window type hint"),
                                                      CDK_TYPE_WINDOW_TYPE_HINT,
                                                      CDK_WINDOW_TYPE_HINT_POPUP_MENU,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT |
                                                      G_PARAM_STATIC_NAME |
                                                      G_PARAM_STATIC_NICK |
                                                      G_PARAM_STATIC_BLURB |
                                                      G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkMenu:horizontal-padding:
   *
   * Extra space at the left and right edges of the menu.
   *
   * Deprecated: 3.8: use the standard padding CSS property (through objects
   *   like #CtkStyleContext and #CtkCssProvider); the value of this style
   *   property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("horizontal-padding",
                                                             P_("Horizontal Padding"),
                                                             P_("Extra space at the left and right edges of the menu"),
                                                             0,
                                                             G_MAXINT,
                                                             0,
                                                             CTK_PARAM_READABLE |
                                                             G_PARAM_DEPRECATED));

  /**
   * CtkMenu:vertical-padding:
   *
   * Extra space at the top and bottom of the menu.
   *
   * Deprecated: 3.8: use the standard padding CSS property (through objects
   *   like #CtkStyleContext and #CtkCssProvider); the value of this style
   *   property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("vertical-padding",
                                                             P_("Vertical Padding"),
                                                             P_("Extra space at the top and bottom of the menu"),
                                                             0,
                                                             G_MAXINT,
                                                             1,
                                                             CTK_PARAM_READABLE |
                                                             G_PARAM_DEPRECATED));

  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("vertical-offset",
                                                             P_("Vertical Offset"),
                                                             P_("When the menu is a submenu, position it this number of pixels offset vertically"),
                                                             G_MININT,
                                                             G_MAXINT,
                                                             0,
                                                             CTK_PARAM_READABLE));

  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("horizontal-offset",
                                                             P_("Horizontal Offset"),
                                                             P_("When the menu is a submenu, position it this number of pixels offset horizontally"),
                                                             G_MININT,
                                                             G_MAXINT,
                                                             -2,
                                                             CTK_PARAM_READABLE));

  /**
   * CtkMenu:double-arrows:
   *
   * When %TRUE, both arrows are shown when scrolling.
   *
   * Deprecated: 3.20: the value of this style property is ignored.
   **/
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_boolean ("double-arrows",
                                                                 P_("Double Arrows"),
                                                                 P_("When scrolling, always show both arrows."),
                                                                 TRUE,
                                                                 CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  /**
   * CtkMenu:arrow-placement:
   *
   * Indicates where scroll arrows should be placed.
   *
   * Since: 2.16
   *
   * Deprecated: 3.20: the value of this style property is ignored.
   **/
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_enum ("arrow-placement",
                                                              P_("Arrow Placement"),
                                                              P_("Indicates where scroll arrows should be placed"),
                                                              CTK_TYPE_ARROW_PLACEMENT,
                                                              CTK_ARROWS_BOTH,
                                                              CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

 ctk_container_class_install_child_property (container_class,
                                             CHILD_PROP_LEFT_ATTACH,
                                              g_param_spec_int ("left-attach",
                                                               P_("Left Attach"),
                                                               P_("The column number to attach the left side of the child to"),
                                                                -1, INT_MAX, -1,
                                                               CTK_PARAM_READWRITE));

 ctk_container_class_install_child_property (container_class,
                                             CHILD_PROP_RIGHT_ATTACH,
                                              g_param_spec_int ("right-attach",
                                                               P_("Right Attach"),
                                                               P_("The column number to attach the right side of the child to"),
                                                                -1, INT_MAX, -1,
                                                               CTK_PARAM_READWRITE));

 ctk_container_class_install_child_property (container_class,
                                             CHILD_PROP_TOP_ATTACH,
                                              g_param_spec_int ("top-attach",
                                                               P_("Top Attach"),
                                                               P_("The row number to attach the top of the child to"),
                                                                -1, INT_MAX, -1,
                                                               CTK_PARAM_READWRITE));

 ctk_container_class_install_child_property (container_class,
                                             CHILD_PROP_BOTTOM_ATTACH,
                                              g_param_spec_int ("bottom-attach",
                                                               P_("Bottom Attach"),
                                                               P_("The row number to attach the bottom of the child to"),
                                                                -1, INT_MAX, -1,
                                                               CTK_PARAM_READWRITE));

 /**
  * CtkMenu:arrow-scaling:
  *
  * Arbitrary constant to scale down the size of the scroll arrow.
  *
  * Since: 2.16
  *
  * Deprecated: 3.20: use the standard min-width/min-height CSS properties on
  *   the arrow node; the value of this style property is ignored.
  */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_float ("arrow-scaling",
                                                               P_("Arrow Scaling"),
                                                               P_("Arbitrary constant to scale down the size of the scroll arrow"),
                                                               0.0, 1.0, 0.7,
                                                               CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  binding_set = ctk_binding_set_by_class (class);
  ctk_binding_entry_add_signal (binding_set,
                                CDK_KEY_Up, 0,
                                I_("move-current"), 1,
                                CTK_TYPE_MENU_DIRECTION_TYPE,
                                CTK_MENU_DIR_PREV);
  ctk_binding_entry_add_signal (binding_set,
                                CDK_KEY_KP_Up, 0,
                                "move-current", 1,
                                CTK_TYPE_MENU_DIRECTION_TYPE,
                                CTK_MENU_DIR_PREV);
  ctk_binding_entry_add_signal (binding_set,
                                CDK_KEY_Down, 0,
                                "move-current", 1,
                                CTK_TYPE_MENU_DIRECTION_TYPE,
                                CTK_MENU_DIR_NEXT);
  ctk_binding_entry_add_signal (binding_set,
                                CDK_KEY_KP_Down, 0,
                                "move-current", 1,
                                CTK_TYPE_MENU_DIRECTION_TYPE,
                                CTK_MENU_DIR_NEXT);
  ctk_binding_entry_add_signal (binding_set,
                                CDK_KEY_Left, 0,
                                "move-current", 1,
                                CTK_TYPE_MENU_DIRECTION_TYPE,
                                CTK_MENU_DIR_PARENT);
  ctk_binding_entry_add_signal (binding_set,
                                CDK_KEY_KP_Left, 0,
                                "move-current", 1,
                                CTK_TYPE_MENU_DIRECTION_TYPE,
                                CTK_MENU_DIR_PARENT);
  ctk_binding_entry_add_signal (binding_set,
                                CDK_KEY_Right, 0,
                                "move-current", 1,
                                CTK_TYPE_MENU_DIRECTION_TYPE,
                                CTK_MENU_DIR_CHILD);
  ctk_binding_entry_add_signal (binding_set,
                                CDK_KEY_KP_Right, 0,
                                "move-current", 1,
                                CTK_TYPE_MENU_DIRECTION_TYPE,
                                CTK_MENU_DIR_CHILD);
  ctk_binding_entry_add_signal (binding_set,
                                CDK_KEY_Home, 0,
                                "move-scroll", 1,
                                CTK_TYPE_SCROLL_TYPE,
                                CTK_SCROLL_START);
  ctk_binding_entry_add_signal (binding_set,
                                CDK_KEY_KP_Home, 0,
                                "move-scroll", 1,
                                CTK_TYPE_SCROLL_TYPE,
                                CTK_SCROLL_START);
  ctk_binding_entry_add_signal (binding_set,
                                CDK_KEY_End, 0,
                                "move-scroll", 1,
                                CTK_TYPE_SCROLL_TYPE,
                                CTK_SCROLL_END);
  ctk_binding_entry_add_signal (binding_set,
                                CDK_KEY_KP_End, 0,
                                "move-scroll", 1,
                                CTK_TYPE_SCROLL_TYPE,
                                CTK_SCROLL_END);
  ctk_binding_entry_add_signal (binding_set,
                                CDK_KEY_Page_Up, 0,
                                "move-scroll", 1,
                                CTK_TYPE_SCROLL_TYPE,
                                CTK_SCROLL_PAGE_UP);
  ctk_binding_entry_add_signal (binding_set,
                                CDK_KEY_KP_Page_Up, 0,
                                "move-scroll", 1,
                                CTK_TYPE_SCROLL_TYPE,
                                CTK_SCROLL_PAGE_UP);
  ctk_binding_entry_add_signal (binding_set,
                                CDK_KEY_Page_Down, 0,
                                "move-scroll", 1,
                                CTK_TYPE_SCROLL_TYPE,
                                CTK_SCROLL_PAGE_DOWN);
  ctk_binding_entry_add_signal (binding_set,
                                CDK_KEY_KP_Page_Down, 0,
                                "move-scroll", 1,
                                CTK_TYPE_SCROLL_TYPE,
                                CTK_SCROLL_PAGE_DOWN);

  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_MENU_ACCESSIBLE);
  ctk_widget_class_set_css_name (widget_class, "menu");
}


static void
ctk_menu_set_property (GObject      *object,
                       guint         prop_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
  CtkMenu *menu = CTK_MENU (object);

  switch (prop_id)
    {
    case PROP_ACTIVE:
      ctk_menu_set_active (menu, g_value_get_int (value));
      break;
    case PROP_ACCEL_GROUP:
      ctk_menu_set_accel_group (menu, g_value_get_object (value));
      break;
    case PROP_ACCEL_PATH:
      ctk_menu_set_accel_path (menu, g_value_get_string (value));
      break;
    case PROP_ATTACH_WIDGET:
      {
        CtkWidget *widget;

        widget = ctk_menu_get_attach_widget (menu);
        if (widget)
          ctk_menu_detach (menu);

        widget = (CtkWidget*) g_value_get_object (value); 
        if (widget)
          ctk_menu_attach_to_widget (menu, widget, NULL);
      }
      break;
    case PROP_TEAROFF_STATE:
      ctk_menu_set_tearoff_state (menu, g_value_get_boolean (value));
      break;
    case PROP_TEAROFF_TITLE:
      ctk_menu_set_title (menu, g_value_get_string (value));
      break;
    case PROP_MONITOR:
      ctk_menu_set_monitor (menu, g_value_get_int (value));
      break;
    case PROP_RESERVE_TOGGLE_SIZE:
      ctk_menu_set_reserve_toggle_size (menu, g_value_get_boolean (value));
      break;
    case PROP_ANCHOR_HINTS:
      if (menu->priv->anchor_hints != g_value_get_flags (value))
        {
          menu->priv->anchor_hints = g_value_get_flags (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_RECT_ANCHOR_DX:
      if (menu->priv->rect_anchor_dx != g_value_get_int (value))
        {
          menu->priv->rect_anchor_dx = g_value_get_int (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_RECT_ANCHOR_DY:
      if (menu->priv->rect_anchor_dy != g_value_get_int (value))
        {
          menu->priv->rect_anchor_dy = g_value_get_int (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_MENU_TYPE_HINT:
      if (menu->priv->menu_type_hint != g_value_get_enum (value))
        {
          menu->priv->menu_type_hint = g_value_get_enum (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_menu_get_property (GObject     *object,
                       guint        prop_id,
                       GValue      *value,
                       GParamSpec  *pspec)
{
  CtkMenu *menu = CTK_MENU (object);

  switch (prop_id)
    {
    case PROP_ACTIVE:
      g_value_set_int (value, g_list_index (CTK_MENU_SHELL (menu)->priv->children, ctk_menu_get_active (menu)));
      break;
    case PROP_ACCEL_GROUP:
      g_value_set_object (value, ctk_menu_get_accel_group (menu));
      break;
    case PROP_ACCEL_PATH:
      g_value_set_string (value, ctk_menu_get_accel_path (menu));
      break;
    case PROP_ATTACH_WIDGET:
      g_value_set_object (value, ctk_menu_get_attach_widget (menu));
      break;
    case PROP_TEAROFF_STATE:
      g_value_set_boolean (value, ctk_menu_get_tearoff_state (menu));
      break;
    case PROP_TEAROFF_TITLE:
      g_value_set_string (value, ctk_menu_get_title (menu));
      break;
    case PROP_MONITOR:
      g_value_set_int (value, ctk_menu_get_monitor (menu));
      break;
    case PROP_RESERVE_TOGGLE_SIZE:
      g_value_set_boolean (value, ctk_menu_get_reserve_toggle_size (menu));
      break;
    case PROP_ANCHOR_HINTS:
      g_value_set_flags (value, menu->priv->anchor_hints);
      break;
    case PROP_RECT_ANCHOR_DX:
      g_value_set_int (value, menu->priv->rect_anchor_dx);
      break;
    case PROP_RECT_ANCHOR_DY:
      g_value_set_int (value, menu->priv->rect_anchor_dy);
      break;
    case PROP_MENU_TYPE_HINT:
      g_value_set_enum (value, menu->priv->menu_type_hint);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_menu_set_child_property (CtkContainer *container,
                             CtkWidget    *child,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  CtkMenu *menu = CTK_MENU (container);
  AttachInfo *ai = get_attach_info (child);

  switch (property_id)
    {
    case CHILD_PROP_LEFT_ATTACH:
      ai->left_attach = g_value_get_int (value);
      break;
    case CHILD_PROP_RIGHT_ATTACH:
      ai->right_attach = g_value_get_int (value);
      break;
    case CHILD_PROP_TOP_ATTACH:
      ai->top_attach = g_value_get_int (value);
      break;
    case CHILD_PROP_BOTTOM_ATTACH:
      ai->bottom_attach = g_value_get_int (value);
      break;

    default:
      CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      return;
    }

  menu_queue_resize (menu);
}

static void
ctk_menu_get_child_property (CtkContainer *container,
                             CtkWidget    *child,
                             guint         property_id,
                             GValue       *value,
                             GParamSpec   *pspec)
{
  AttachInfo *ai = get_attach_info (child);

  switch (property_id)
    {
    case CHILD_PROP_LEFT_ATTACH:
      g_value_set_int (value, ai->left_attach);
      break;
    case CHILD_PROP_RIGHT_ATTACH:
      g_value_set_int (value, ai->right_attach);
      break;
    case CHILD_PROP_TOP_ATTACH:
      g_value_set_int (value, ai->top_attach);
      break;
    case CHILD_PROP_BOTTOM_ATTACH:
      g_value_set_int (value, ai->bottom_attach);
      break;
      
    default:
      CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      return;
    }
}

static gboolean
ctk_menu_window_event (CtkWidget *window,
                       CdkEvent  *event,
                       CtkWidget *menu)
{
  gboolean handled = FALSE;

  g_object_ref (window);
  g_object_ref (menu);

  switch (event->type)
    {
    case CDK_KEY_PRESS:
    case CDK_KEY_RELEASE:
      handled = ctk_widget_event (menu, event);
      break;
    case CDK_WINDOW_STATE:
      /* Window for the menu has been closed by the display server or by CDK.
       * Update the internal state as if the user had clicked outside the
       * menu
       */
      if (event->window_state.new_window_state & CDK_WINDOW_STATE_WITHDRAWN &&
          event->window_state.changed_mask & CDK_WINDOW_STATE_WITHDRAWN)
        ctk_menu_shell_deactivate (CTK_MENU_SHELL(menu));
      break;
    default:
      break;
    }

  g_object_unref (window);
  g_object_unref (menu);

  return handled;
}

static void
ctk_menu_init (CtkMenu *menu)
{
  CtkMenuPrivate *priv;
  CtkCssNode *top_arrow_node, *bottom_arrow_node, *widget_node;

  priv = ctk_menu_get_instance_private (menu);

  menu->priv = priv;

  priv->toplevel = ctk_window_new (CTK_WINDOW_POPUP);
  ctk_container_add (CTK_CONTAINER (priv->toplevel), CTK_WIDGET (menu));
  g_signal_connect (priv->toplevel, "event", G_CALLBACK (ctk_menu_window_event), menu);
  g_signal_connect (priv->toplevel, "destroy", G_CALLBACK (ctk_widget_destroyed), &priv->toplevel);
  ctk_window_set_resizable (CTK_WINDOW (priv->toplevel), FALSE);
  ctk_window_set_mnemonic_modifier (CTK_WINDOW (priv->toplevel), 0);

  _ctk_window_request_csd (CTK_WINDOW (priv->toplevel));
  ctk_style_context_add_class (ctk_widget_get_style_context (priv->toplevel),
                               CTK_STYLE_CLASS_POPUP);

  /* Refloat the menu, so that reference counting for the menu isn't
   * affected by it being a child of the toplevel
   */
  g_object_force_floating (G_OBJECT (menu));
  priv->needs_destruction_ref = TRUE;

  priv->monitor_num = -1;
  priv->drag_start_y = -1;

  priv->anchor_hints = CDK_ANCHOR_FLIP | CDK_ANCHOR_SLIDE | CDK_ANCHOR_RESIZE;
  priv->rect_anchor_dx = 0;
  priv->rect_anchor_dy = 0;
  priv->menu_type_hint = CDK_WINDOW_TYPE_HINT_POPUP_MENU;

  _ctk_widget_set_captured_event_handler (CTK_WIDGET (menu), ctk_menu_captured_event);

  widget_node = ctk_widget_get_css_node (CTK_WIDGET (menu));
  priv->top_arrow_gadget = ctk_builtin_icon_new ("arrow",
                                                 CTK_WIDGET (menu),
                                                 NULL, NULL);
  ctk_css_gadget_add_class (priv->top_arrow_gadget, CTK_STYLE_CLASS_TOP);
  top_arrow_node = ctk_css_gadget_get_node (priv->top_arrow_gadget);
  ctk_css_node_set_parent (top_arrow_node, widget_node);
  ctk_css_node_set_visible (top_arrow_node, FALSE);
  ctk_css_node_set_state (top_arrow_node, ctk_css_node_get_state (widget_node));

  priv->bottom_arrow_gadget = ctk_builtin_icon_new ("arrow",
                                                    CTK_WIDGET (menu),
                                                    NULL, NULL);
  ctk_css_gadget_add_class (priv->bottom_arrow_gadget, CTK_STYLE_CLASS_BOTTOM);
  bottom_arrow_node = ctk_css_gadget_get_node (priv->bottom_arrow_gadget);
  ctk_css_node_set_parent (bottom_arrow_node, widget_node);
  ctk_css_node_set_visible (bottom_arrow_node, FALSE);
  ctk_css_node_set_state (bottom_arrow_node, ctk_css_node_get_state (widget_node));
}

static void
moved_to_rect_cb (CdkWindow          *window,
                  const CdkRectangle *flipped_rect,
                  const CdkRectangle *final_rect,
                  gboolean            flipped_x,
                  gboolean            flipped_y,
                  CtkMenu            *menu)
{
  CtkMenuPrivate *priv = menu->priv;

  ctk_window_fixate_size (CTK_WINDOW (priv->toplevel));

  if (!priv->emulated_move_to_rect)
    g_signal_emit (menu,
                   menu_signals[POPPED_UP],
                   0,
                   flipped_rect,
                   final_rect,
                   flipped_x,
                   flipped_y);
}

static void
ctk_menu_destroy (CtkWidget *widget)
{
  CtkMenu *menu = CTK_MENU (widget);
  CtkMenuPrivate *priv = menu->priv;
  CtkMenuAttachData *data;

  ctk_menu_remove_scroll_timeout (menu);

  data = g_object_get_data (G_OBJECT (widget), attach_data_key);
  if (data)
    ctk_menu_detach (menu);

  ctk_menu_stop_navigating_submenu (menu);

  g_clear_object (&priv->old_active_menu_item);

  /* Add back the reference count for being a child */
  if (priv->needs_destruction_ref)
    {
      priv->needs_destruction_ref = FALSE;
      g_object_ref (widget);
    }

  g_clear_object (&priv->accel_group);

  if (priv->toplevel)
    {
      g_signal_handlers_disconnect_by_func (priv->toplevel, moved_to_rect_cb, menu);
      ctk_widget_destroy (priv->toplevel);
    }

  if (priv->tearoff_window)
    ctk_widget_destroy (priv->tearoff_window);

  g_clear_pointer (&priv->heights, g_free);

  g_clear_pointer (&priv->title, g_free);

  if (priv->position_func_data_destroy)
    {
      priv->position_func_data_destroy (priv->position_func_data);
      priv->position_func_data = NULL;
      priv->position_func_data_destroy = NULL;
    }

  CTK_WIDGET_CLASS (ctk_menu_parent_class)->destroy (widget);
}

static void
ctk_menu_finalize (GObject *object)
{
  CtkMenu *menu = CTK_MENU (object);
  CtkMenuPrivate *priv = menu->priv;

  g_clear_object (&priv->top_arrow_gadget);
  g_clear_object (&priv->bottom_arrow_gadget);

  G_OBJECT_CLASS (ctk_menu_parent_class)->finalize (object);
}

static void
menu_change_screen (CtkMenu   *menu,
                    CdkScreen *new_screen)
{
  CtkMenuPrivate *priv = menu->priv;

  if (ctk_widget_has_screen (CTK_WIDGET (menu)))
    {
      if (new_screen == ctk_widget_get_screen (CTK_WIDGET (menu)))
        return;
    }

  if (priv->torn_off)
    {
      ctk_window_set_screen (CTK_WINDOW (priv->tearoff_window), new_screen);
      ctk_menu_position (menu, TRUE);
    }

  ctk_window_set_screen (CTK_WINDOW (priv->toplevel), new_screen);
  priv->monitor_num = -1;
}

static void
attach_widget_screen_changed (CtkWidget *attach_widget,
                              CdkScreen *previous_screen,
                              CtkMenu   *menu)
{
  if (ctk_widget_has_screen (attach_widget) &&
      !g_object_get_data (G_OBJECT (menu), "ctk-menu-explicit-screen"))
    menu_change_screen (menu, ctk_widget_get_screen (attach_widget));
}

static void
menu_toplevel_attached_to (CtkWindow *toplevel, GParamSpec *pspec, CtkMenu *menu)
{
  CtkMenuAttachData *data;

  data = g_object_get_data (G_OBJECT (menu), attach_data_key);

  g_return_if_fail (data);

  ctk_menu_detach (menu);
}

/**
 * ctk_menu_attach_to_widget:
 * @menu: a #CtkMenu
 * @attach_widget: the #CtkWidget that the menu will be attached to
 * @detacher: (scope async)(allow-none): the user supplied callback function
 *             that will be called when the menu calls ctk_menu_detach()
 *
 * Attaches the menu to the widget and provides a callback function
 * that will be invoked when the menu calls ctk_menu_detach() during
 * its destruction.
 *
 * If the menu is attached to the widget then it will be destroyed
 * when the widget is destroyed, as if it was a child widget.
 * An attached menu will also move between screens correctly if the
 * widgets moves between screens.
 */
void
ctk_menu_attach_to_widget (CtkMenu           *menu,
                           CtkWidget         *attach_widget,
                           CtkMenuDetachFunc  detacher)
{
  CtkMenuAttachData *data;
  GList *list;

  g_return_if_fail (CTK_IS_MENU (menu));
  g_return_if_fail (CTK_IS_WIDGET (attach_widget));

  /* keep this function in sync with ctk_widget_set_parent() */
  data = g_object_get_data (G_OBJECT (menu), attach_data_key);
  if (data)
    {
      g_warning ("ctk_menu_attach_to_widget(): menu already attached to %s",
                 g_type_name (G_TYPE_FROM_INSTANCE (data->attach_widget)));
     return;
    }

  g_object_ref_sink (menu);

  data = g_slice_new (CtkMenuAttachData);
  data->attach_widget = attach_widget;

  g_signal_connect (attach_widget, "screen-changed",
                    G_CALLBACK (attach_widget_screen_changed), menu);
  attach_widget_screen_changed (attach_widget, NULL, menu);

  data->detacher = detacher;
  g_object_set_data (G_OBJECT (menu), I_(attach_data_key), data);
  list = g_object_steal_data (G_OBJECT (attach_widget), ATTACHED_MENUS);
  if (!g_list_find (list, menu))
    list = g_list_prepend (list, menu);

  g_object_set_data_full (G_OBJECT (attach_widget), I_(ATTACHED_MENUS), list,
                          (GDestroyNotify) g_list_free);

  /* Attach the widget to the toplevel window. */
  ctk_window_set_attached_to (CTK_WINDOW (menu->priv->toplevel), attach_widget);
  g_signal_connect (CTK_WINDOW (menu->priv->toplevel), "notify::attached-to",
                    G_CALLBACK (menu_toplevel_attached_to), menu);

  _ctk_widget_update_parent_muxer (CTK_WIDGET (menu));

  /* Fallback title for menu comes from attach widget */
  ctk_menu_update_title (menu);

  g_object_notify (G_OBJECT (menu), "attach-widget");
}

/**
 * ctk_menu_get_attach_widget:
 * @menu: a #CtkMenu
 *
 * Returns the #CtkWidget that the menu is attached to.
 *
 * Returns: (transfer none): the #CtkWidget that the menu is attached to
 */
CtkWidget*
ctk_menu_get_attach_widget (CtkMenu *menu)
{
  CtkMenuAttachData *data;

  g_return_val_if_fail (CTK_IS_MENU (menu), NULL);

  data = g_object_get_data (G_OBJECT (menu), attach_data_key);
  if (data)
    return data->attach_widget;
  return NULL;
}

/**
 * ctk_menu_detach:
 * @menu: a #CtkMenu
 *
 * Detaches the menu from the widget to which it had been attached.
 * This function will call the callback function, @detacher, provided
 * when the ctk_menu_attach_to_widget() function was called.
 */
void
ctk_menu_detach (CtkMenu *menu)
{
  CtkWindow *toplevel;
  CtkMenuAttachData *data;
  GList *list;

  g_return_if_fail (CTK_IS_MENU (menu));
  toplevel = CTK_WINDOW (menu->priv->toplevel);

  /* keep this function in sync with ctk_widget_unparent() */
  data = g_object_get_data (G_OBJECT (menu), attach_data_key);
  if (!data)
    {
      g_warning ("ctk_menu_detach(): menu is not attached");
      return;
    }
  g_object_set_data (G_OBJECT (menu), I_(attach_data_key), NULL);

  /* Detach the toplevel window. */
  if (toplevel)
    {
      g_signal_handlers_disconnect_by_func (toplevel,
                                            (gpointer) menu_toplevel_attached_to,
                                            menu);
      if (ctk_window_get_attached_to (toplevel) == data->attach_widget)
        ctk_window_set_attached_to (toplevel, NULL);
    }

  g_signal_handlers_disconnect_by_func (data->attach_widget,
                                        (gpointer) attach_widget_screen_changed,
                                        menu);

  if (data->detacher)
    data->detacher (data->attach_widget, menu);
  list = g_object_steal_data (G_OBJECT (data->attach_widget), ATTACHED_MENUS);
  list = g_list_remove (list, menu);
  if (list)
    g_object_set_data_full (G_OBJECT (data->attach_widget), I_(ATTACHED_MENUS), list,
                            (GDestroyNotify) g_list_free);
  else
    g_object_set_data (G_OBJECT (data->attach_widget), I_(ATTACHED_MENUS), NULL);

  if (ctk_widget_get_realized (CTK_WIDGET (menu)))
    ctk_widget_unrealize (CTK_WIDGET (menu));

  g_slice_free (CtkMenuAttachData, data);

  _ctk_widget_update_parent_muxer (CTK_WIDGET (menu));

  /* Fallback title for menu comes from attach widget */
  ctk_menu_update_title (menu);

  g_object_notify (G_OBJECT (menu), "attach-widget");
  g_object_unref (menu);
}

static void
ctk_menu_remove (CtkContainer *container,
                 CtkWidget    *widget)
{
  CtkMenu *menu = CTK_MENU (container);
  CtkMenuPrivate *priv = menu->priv;

  /* Clear out old_active_menu_item if it matches the item we are removing */
  if (priv->old_active_menu_item == widget)
    g_clear_object (&priv->old_active_menu_item);

  CTK_CONTAINER_CLASS (ctk_menu_parent_class)->remove (container, widget);

  g_object_set_data (G_OBJECT (widget), I_(ATTACH_INFO_KEY), NULL);

  menu_queue_resize (menu);
}

/**
 * ctk_menu_new:
 *
 * Creates a new #CtkMenu
 *
 * Returns: a new #CtkMenu
 */
CtkWidget*
ctk_menu_new (void)
{
  return g_object_new (CTK_TYPE_MENU, NULL);
}

static void
ctk_menu_real_insert (CtkMenuShell *menu_shell,
                      CtkWidget    *child,
                      gint          position)
{
  CtkMenu *menu = CTK_MENU (menu_shell);
  CtkMenuPrivate *priv = menu->priv;
  AttachInfo *ai = get_attach_info (child);
  CtkCssNode *widget_node, *child_node;

  ai->left_attach = -1;
  ai->right_attach = -1;
  ai->top_attach = -1;
  ai->bottom_attach = -1;

  if (ctk_widget_get_realized (CTK_WIDGET (menu_shell)))
    ctk_widget_set_parent_window (child, priv->bin_window);

  widget_node = ctk_widget_get_css_node (CTK_WIDGET (menu));
  child_node = ctk_widget_get_css_node (child);
  ctk_css_node_insert_before (widget_node, child_node,
                              ctk_css_gadget_get_node (priv->bottom_arrow_gadget));

  CTK_MENU_SHELL_CLASS (ctk_menu_parent_class)->insert (menu_shell, child, position);

  menu_queue_resize (menu);
}

static void
ctk_menu_tearoff_bg_copy (CtkMenu *menu)
{
  CtkMenuPrivate *priv = menu->priv;
  gint width, height;

  if (priv->torn_off)
    {
      CdkWindow *window;
      cairo_surface_t *surface;
      cairo_pattern_t *pattern;
      cairo_t *cr;

      priv->tearoff_active = FALSE;
      priv->saved_scroll_offset = priv->scroll_offset;

      window = ctk_widget_get_window (priv->tearoff_window);
      width = cdk_window_get_width (window);
      height = cdk_window_get_height (window);

      surface = cdk_window_create_similar_surface (window,
                                                   CAIRO_CONTENT_COLOR,
                                                   width,
                                                   height);

      cr = cairo_create (surface);
      cdk_cairo_set_source_window (cr, window, 0, 0);
      cairo_paint (cr);
      cairo_destroy (cr);

      ctk_widget_set_size_request (priv->tearoff_window, width, height);

      pattern = cairo_pattern_create_for_surface (surface);
      cdk_window_set_background_pattern (window, pattern);

      cairo_pattern_destroy (pattern);
      cairo_surface_destroy (surface);
    }
}

static gboolean
popup_grab_on_window (CdkWindow *window,
                      CdkDevice *pointer)
{
  CdkGrabStatus status;
  CdkSeat *seat;

  seat = cdk_device_get_seat (pointer);

/* Let CtkMenu use pointer emulation instead of touch events under X11. */
#define CDK_SEAT_CAPABILITY_NO_TOUCH (CDK_SEAT_CAPABILITY_POINTER | \
                                      CDK_SEAT_CAPABILITY_TABLET_STYLUS | \
                                      CDK_SEAT_CAPABILITY_KEYBOARD)
  status = cdk_seat_grab (seat, window,
                          CDK_SEAT_CAPABILITY_NO_TOUCH, TRUE,
                          NULL, NULL, NULL, NULL);

  return status == CDK_GRAB_SUCCESS;
}

static void
associate_menu_grab_transfer_window (CtkMenu *menu)
{
  CtkMenuPrivate *priv = menu->priv;
  CdkWindow *toplevel_window;
  CdkWindow *transfer_window;

  toplevel_window = ctk_widget_get_window (priv->toplevel);
  transfer_window = g_object_get_data (G_OBJECT (menu), "ctk-menu-transfer-window");

  if (toplevel_window == NULL || transfer_window == NULL)
    return;

  g_object_set_data (G_OBJECT (toplevel_window), I_("cdk-attached-grab-window"), transfer_window);
}

static void
ctk_menu_popup_internal (CtkMenu             *menu,
                         CdkDevice           *device,
                         CtkWidget           *parent_menu_shell,
                         CtkWidget           *parent_menu_item,
                         CtkMenuPositionFunc  func,
                         gpointer             data,
                         GDestroyNotify       destroy,
                         guint                button,
                         guint32              activate_time)
{
  CtkMenuPrivate *priv = menu->priv;
  CtkWidget *widget;
  CtkWidget *xgrab_shell;
  CtkWidget *parent;
  CdkEvent *current_event;
  CtkMenuShell *menu_shell;
  gboolean grab_keyboard;
  CtkWidget *parent_toplevel;
  CdkDevice *pointer, *source_device = NULL;
  CdkDisplay *display;

  g_return_if_fail (CTK_IS_MENU (menu));
  g_return_if_fail (device == NULL || CDK_IS_DEVICE (device));

  _ctk_tooltip_hide_in_display (ctk_widget_get_display (CTK_WIDGET (menu)));
  display = ctk_widget_get_display (CTK_WIDGET (menu));

  if (device == NULL)
    device = ctk_get_current_event_device ();

  if (device && cdk_device_get_display (device) != display)
    device = NULL;

  if (device == NULL)
    device = cdk_seat_get_pointer (cdk_display_get_default_seat (display));

  widget = CTK_WIDGET (menu);
  menu_shell = CTK_MENU_SHELL (menu);

  if (cdk_device_get_source (device) == CDK_SOURCE_KEYBOARD)
    pointer = cdk_device_get_associated_device (device);
  else
    pointer = device;

  menu_shell->priv->parent_menu_shell = parent_menu_shell;

  priv->seen_item_enter = FALSE;

  /* Find the last viewable ancestor, and make an X grab on it
   */
  parent = CTK_WIDGET (menu);
  xgrab_shell = NULL;
  while (parent)
    {
      gboolean viewable = TRUE;
      CtkWidget *tmp = parent;

      while (tmp)
        {
          if (!ctk_widget_get_mapped (tmp))
            {
              viewable = FALSE;
              break;
            }
          tmp = ctk_widget_get_parent (tmp);
        }

      if (viewable)
        xgrab_shell = parent;

      parent = CTK_MENU_SHELL (parent)->priv->parent_menu_shell;
    }

  /* We want to receive events generated when we map the menu;
   * unfortunately, since there is probably already an implicit
   * grab in place from the button that the user used to pop up
   * the menu, we won't receive then -- in particular, the EnterNotify
   * when the menu pops up under the pointer.
   *
   * If we are grabbing on a parent menu shell, no problem; just grab
   * on that menu shell first before popping up the window with
   * owner_events = TRUE.
   *
   * When grabbing on the menu itself, things get more convoluted --
   * we do an explicit grab on a specially created window with
   * owner_events = TRUE, which we override further down with a
   * grab on the menu. (We can't grab on the menu until it is mapped;
   * we probably could just leave the grab on the other window,
   * with a little reorganization of the code in ctkmenu*).
   */
  grab_keyboard = ctk_menu_shell_get_take_focus (menu_shell);
  ctk_window_set_accept_focus (CTK_WINDOW (priv->toplevel), grab_keyboard);

  if (xgrab_shell && xgrab_shell != widget)
    {
      if (popup_grab_on_window (ctk_widget_get_window (xgrab_shell), pointer))
        {
          _ctk_menu_shell_set_grab_device (CTK_MENU_SHELL (xgrab_shell), pointer);
          CTK_MENU_SHELL (xgrab_shell)->priv->have_xgrab = TRUE;
        }
    }
  else
    {
      CdkWindow *transfer_window;

      xgrab_shell = widget;
      transfer_window = menu_grab_transfer_window_get (menu);
      if (popup_grab_on_window (transfer_window, pointer))
        {
          _ctk_menu_shell_set_grab_device (CTK_MENU_SHELL (xgrab_shell), pointer);
          CTK_MENU_SHELL (xgrab_shell)->priv->have_xgrab = TRUE;
        }
    }

  if (!CTK_MENU_SHELL (xgrab_shell)->priv->have_xgrab)
    {
      /* We failed to make our pointer/keyboard grab.
       * Rather than leaving the user with a stuck up window,
       * we just abort here. Presumably the user will try again.
       */
      menu_shell->priv->parent_menu_shell = NULL;
      menu_grab_transfer_window_destroy (menu);
      return;
    }

  _ctk_menu_shell_set_grab_device (CTK_MENU_SHELL (menu), pointer);
  menu_shell->priv->active = TRUE;
  menu_shell->priv->button = button;

  /* If we are popping up the menu from something other than, a button
   * press then, as a heuristic, we ignore enter events for the menu
   * until we get a MOTION_NOTIFY.
   */

  current_event = ctk_get_current_event ();
  if (current_event)
    {
      if ((current_event->type != CDK_BUTTON_PRESS) &&
          (current_event->type != CDK_ENTER_NOTIFY))
        menu_shell->priv->ignore_enter = TRUE;

      source_device = cdk_event_get_source_device (current_event);
      cdk_event_free (current_event);
    }
  else
    menu_shell->priv->ignore_enter = TRUE;

  if (priv->torn_off)
    {
      ctk_menu_tearoff_bg_copy (menu);

      ctk_menu_reparent (menu, priv->toplevel, FALSE);
    }

  parent_toplevel = NULL;
  if (parent_menu_shell)
    parent_toplevel = ctk_widget_get_toplevel (parent_menu_shell);
  else if (!g_object_get_data (G_OBJECT (menu), "ctk-menu-explicit-screen"))
    {
      CtkWidget *attach_widget = ctk_menu_get_attach_widget (menu);
      if (attach_widget)
        parent_toplevel = ctk_widget_get_toplevel (attach_widget);
    }

  /* Set transient for to get the right window group and parent */
  if (CTK_IS_WINDOW (parent_toplevel))
    ctk_window_set_transient_for (CTK_WINDOW (priv->toplevel),
                                  CTK_WINDOW (parent_toplevel));

  priv->parent_menu_item = parent_menu_item;
  priv->position_func = func;
  priv->position_func_data = data;
  priv->position_func_data_destroy = destroy;
  menu_shell->priv->activate_time = activate_time;

  /* We need to show the menu here rather in the init function
   * because code expects to be able to tell if the menu is onscreen
   * by looking at ctk_widget_get_visible (menu)
   */
  ctk_widget_show (CTK_WIDGET (menu));

  /* Position the menu, possibly changing the size request
   */
  ctk_menu_position (menu, TRUE);

  associate_menu_grab_transfer_window (menu);

  ctk_menu_scroll_to (menu, priv->scroll_offset, CTK_MENU_SCROLL_FLAG_NONE);

  /* if no item is selected, select the first one */
  if (!menu_shell->priv->active_menu_item &&
      source_device && cdk_device_get_source (source_device) == CDK_SOURCE_TOUCHSCREEN)
    ctk_menu_shell_select_first (menu_shell, TRUE);

  /* Once everything is set up correctly, map the toplevel */
  ctk_window_force_resize (CTK_WINDOW (priv->toplevel));
  ctk_widget_show (priv->toplevel);

  if (xgrab_shell == widget)
    popup_grab_on_window (ctk_widget_get_window (widget), pointer); /* Should always succeed */

  ctk_grab_add (CTK_WIDGET (menu));

  if (parent_menu_shell)
    {
      gboolean keyboard_mode;

      keyboard_mode = _ctk_menu_shell_get_keyboard_mode (CTK_MENU_SHELL (parent_menu_shell));
      _ctk_menu_shell_set_keyboard_mode (menu_shell, keyboard_mode);
    }
  else if (menu_shell->priv->button == 0) /* a keynav-activated context menu */
    _ctk_menu_shell_set_keyboard_mode (menu_shell, TRUE);

  _ctk_menu_shell_update_mnemonics (menu_shell);
}

/**
 * ctk_menu_popup_for_device:
 * @menu: a #CtkMenu
 * @device: (allow-none): a #CdkDevice
 * @parent_menu_shell: (allow-none): the menu shell containing the triggering
 *     menu item, or %NULL
 * @parent_menu_item: (allow-none): the menu item whose activation triggered
 *     the popup, or %NULL
 * @func: (allow-none): a user supplied function used to position the menu,
 *     or %NULL
 * @data: (allow-none): user supplied data to be passed to @func
 * @destroy: (allow-none): destroy notify for @data
 * @button: the mouse button which was pressed to initiate the event
 * @activate_time: the time at which the activation event occurred
 *
 * Displays a menu and makes it available for selection.
 *
 * Applications can use this function to display context-sensitive menus,
 * and will typically supply %NULL for the @parent_menu_shell,
 * @parent_menu_item, @func, @data and @destroy parameters. The default
 * menu positioning function will position the menu at the current position
 * of @device (or its corresponding pointer).
 *
 * The @button parameter should be the mouse button pressed to initiate
 * the menu popup. If the menu popup was initiated by something other than
 * a mouse button press, such as a mouse button release or a keypress,
 * @button should be 0.
 *
 * The @activate_time parameter is used to conflict-resolve initiation of
 * concurrent requests for mouse/keyboard grab requests. To function
 * properly, this needs to be the time stamp of the user event (such as
 * a mouse click or key press) that caused the initiation of the popup.
 * Only if no such event is available, ctk_get_current_event_time() can
 * be used instead.
 *
 * Note that this function does not work very well on CDK backends that
 * do not have global coordinates, such as Wayland or Mir. You should
 * probably use one of the ctk_menu_popup_at_ variants, which do not
 * have this problem.
 *
 * Since: 3.0
 */
void
ctk_menu_popup_for_device (CtkMenu             *menu,
                           CdkDevice           *device,
                           CtkWidget           *parent_menu_shell,
                           CtkWidget           *parent_menu_item,
                           CtkMenuPositionFunc  func,
                           gpointer             data,
                           GDestroyNotify       destroy,
                           guint                button,
                           guint32              activate_time)
{
  CtkMenuPrivate *priv;

  g_return_if_fail (CTK_IS_MENU (menu));

  priv = menu->priv;
  priv->rect_window = NULL;
  priv->widget = NULL;

  ctk_menu_popup_internal (menu,
                           device,
                           parent_menu_shell,
                           parent_menu_item,
                           func,
                           data,
                           destroy,
                           button,
                           activate_time);
}

/**
 * ctk_menu_popup:
 * @menu: a #CtkMenu
 * @parent_menu_shell: (allow-none): the menu shell containing the
 *     triggering menu item, or %NULL
 * @parent_menu_item: (allow-none): the menu item whose activation
 *     triggered the popup, or %NULL
 * @func: (scope async) (allow-none): a user supplied function used to position
 *     the menu, or %NULL
 * @data: user supplied data to be passed to @func.
 * @button: the mouse button which was pressed to initiate the event.
 * @activate_time: the time at which the activation event occurred.
 *
 * Displays a menu and makes it available for selection.
 *
 * Applications can use this function to display context-sensitive
 * menus, and will typically supply %NULL for the @parent_menu_shell,
 * @parent_menu_item, @func and @data parameters. The default menu
 * positioning function will position the menu at the current mouse
 * cursor position.
 *
 * The @button parameter should be the mouse button pressed to initiate
 * the menu popup. If the menu popup was initiated by something other
 * than a mouse button press, such as a mouse button release or a keypress,
 * @button should be 0.
 *
 * The @activate_time parameter is used to conflict-resolve initiation
 * of concurrent requests for mouse/keyboard grab requests. To function
 * properly, this needs to be the timestamp of the user event (such as
 * a mouse click or key press) that caused the initiation of the popup.
 * Only if no such event is available, ctk_get_current_event_time() can
 * be used instead.
 *
 * Note that this function does not work very well on CDK backends that
 * do not have global coordinates, such as Wayland or Mir. You should
 * probably use one of the ctk_menu_popup_at_ variants, which do not
 * have this problem.
 */
void
ctk_menu_popup (CtkMenu             *menu,
                CtkWidget           *parent_menu_shell,
                CtkWidget           *parent_menu_item,
                CtkMenuPositionFunc  func,
                gpointer             data,
                guint                button,
                guint32              activate_time)
{
  g_return_if_fail (CTK_IS_MENU (menu));

  ctk_menu_popup_for_device (menu,
                             NULL,
                             parent_menu_shell,
                             parent_menu_item,

                             func, data, NULL,
                             button, activate_time);
}

static CdkDevice *
get_device_for_event (const CdkEvent *event)
{
  CdkDevice *device = NULL;
  CdkSeat *seat = NULL;
  CdkScreen *screen = NULL;
  CdkDisplay *display = NULL;

  device = cdk_event_get_device (event);

  if (device)
    return device;

  seat = cdk_event_get_seat (event);

  if (!seat)
    {
      screen = cdk_event_get_screen (event);

      if (screen)
        display = cdk_screen_get_display (screen);

      if (!display)
        {
          g_warning ("no display for event, using default");
          display = cdk_display_get_default ();
        }

      if (display)
        seat = cdk_display_get_default_seat (display);
    }

  return seat ? cdk_seat_get_pointer (seat) : NULL;
}

/**
 * ctk_menu_popup_at_rect:
 * @menu: the #CtkMenu to pop up
 * @rect_window: (not nullable): the #CdkWindow @rect is relative to
 * @rect: (not nullable): the #CdkRectangle to align @menu with
 * @rect_anchor: the point on @rect to align with @menu's anchor point
 * @menu_anchor: the point on @menu to align with @rect's anchor point
 * @trigger_event: (nullable): the #CdkEvent that initiated this request or
 *                 %NULL if it's the current event
 *
 * Displays @menu and makes it available for selection.
 *
 * See ctk_menu_popup_at_widget () and ctk_menu_popup_at_pointer (), which
 * handle more common cases for popping up menus.
 *
 * @menu will be positioned at @rect, aligning their anchor points. @rect is
 * relative to the top-left corner of @rect_window. @rect_anchor and
 * @menu_anchor determine anchor points on @rect and @menu to pin together.
 * @menu can optionally be offset by #CtkMenu:rect-anchor-dx and
 * #CtkMenu:rect-anchor-dy.
 *
 * Anchors should be specified under the assumption that the text direction is
 * left-to-right; they will be flipped horizontally automatically if the text
 * direction is right-to-left.
 *
 * Other properties that influence the behaviour of this function are
 * #CtkMenu:anchor-hints and #CtkMenu:menu-type-hint. Connect to the
 * #CtkMenu::popped-up signal to find out how it was actually positioned.
 *
 * Since: 3.22
 */
void
ctk_menu_popup_at_rect (CtkMenu            *menu,
                        CdkWindow          *rect_window,
                        const CdkRectangle *rect,
                        CdkGravity          rect_anchor,
                        CdkGravity          menu_anchor,
                        const CdkEvent     *trigger_event)
{
  CtkMenuPrivate *priv;
  CdkEvent *current_event = NULL;
  CdkDevice *device = NULL;
  guint button = 0;
  guint32 activate_time = CDK_CURRENT_TIME;

  g_return_if_fail (CTK_IS_MENU (menu));
  g_return_if_fail (CDK_IS_WINDOW (rect_window));
  g_return_if_fail (rect);

  priv = menu->priv;
  priv->rect_window = rect_window;
  priv->rect = *rect;
  priv->widget = NULL;
  priv->rect_anchor = rect_anchor;
  priv->menu_anchor = menu_anchor;

  if (!trigger_event)
    {
      current_event = ctk_get_current_event ();
      trigger_event = current_event;
    }

  if (trigger_event)
    {
      device = get_device_for_event (trigger_event);
      cdk_event_get_button (trigger_event, &button);
      activate_time = cdk_event_get_time (trigger_event);
    }
  else
    g_warning ("no trigger event for menu popup");

  ctk_menu_popup_internal (menu,
                           device,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           button,
                           activate_time);

  g_clear_pointer (&current_event, cdk_event_free);
}

/**
 * ctk_menu_popup_at_widget:
 * @menu: the #CtkMenu to pop up
 * @widget: (not nullable): the #CtkWidget to align @menu with
 * @widget_anchor: the point on @widget to align with @menu's anchor point
 * @menu_anchor: the point on @menu to align with @widget's anchor point
 * @trigger_event: (nullable): the #CdkEvent that initiated this request or
 *                 %NULL if it's the current event
 *
 * Displays @menu and makes it available for selection.
 *
 * See ctk_menu_popup_at_pointer () to pop up a menu at the master pointer.
 * ctk_menu_popup_at_rect () also allows you to position a menu at an arbitrary
 * rectangle.
 *
 * ![](popup-anchors.png)
 *
 * @menu will be positioned at @widget, aligning their anchor points.
 * @widget_anchor and @menu_anchor determine anchor points on @widget and @menu
 * to pin together. @menu can optionally be offset by #CtkMenu:rect-anchor-dx
 * and #CtkMenu:rect-anchor-dy.
 *
 * Anchors should be specified under the assumption that the text direction is
 * left-to-right; they will be flipped horizontally automatically if the text
 * direction is right-to-left.
 *
 * Other properties that influence the behaviour of this function are
 * #CtkMenu:anchor-hints and #CtkMenu:menu-type-hint. Connect to the
 * #CtkMenu::popped-up signal to find out how it was actually positioned.
 *
 * Since: 3.22
 */
void
ctk_menu_popup_at_widget (CtkMenu        *menu,
                          CtkWidget      *widget,
                          CdkGravity      widget_anchor,
                          CdkGravity      menu_anchor,
                          const CdkEvent *trigger_event)
{
  CtkMenuPrivate *priv;
  CdkEvent *current_event = NULL;
  CdkDevice *device = NULL;
  guint button = 0;
  guint32 activate_time = CDK_CURRENT_TIME;
  CtkWidget *parent_menu_shell = NULL;
  CtkWidget *parent_menu_item = NULL;

  g_return_if_fail (CTK_IS_MENU (menu));
  g_return_if_fail (CTK_IS_WIDGET (widget));

  priv = menu->priv;
  priv->rect_window = NULL;
  priv->widget = widget;
  priv->rect_anchor = widget_anchor;
  priv->menu_anchor = menu_anchor;

  if (!trigger_event)
    {
      current_event = ctk_get_current_event ();
      trigger_event = current_event;
    }

  if (trigger_event)
    {
      device = get_device_for_event (trigger_event);
      cdk_event_get_button (trigger_event, &button);
      activate_time = cdk_event_get_time (trigger_event);
    }
  else
    g_warning ("no trigger event for menu popup");

  if (CTK_IS_MENU_ITEM (priv->widget))
    {
      parent_menu_item = priv->widget;

      if (CTK_IS_MENU_SHELL (ctk_widget_get_parent (parent_menu_item)))
        parent_menu_shell = ctk_widget_get_parent (parent_menu_item);
    }

  ctk_menu_popup_internal (menu,
                           device,
                           parent_menu_shell,
                           parent_menu_item,
                           NULL,
                           NULL,
                           NULL,
                           button,
                           activate_time);

  g_clear_pointer (&current_event, cdk_event_free);
}

/**
 * ctk_menu_popup_at_pointer:
 * @menu: the #CtkMenu to pop up
 * @trigger_event: (nullable): the #CdkEvent that initiated this request or
 *                 %NULL if it's the current event
 *
 * Displays @menu and makes it available for selection.
 *
 * See ctk_menu_popup_at_widget () to pop up a menu at a widget.
 * ctk_menu_popup_at_rect () also allows you to position a menu at an arbitrary
 * rectangle.
 *
 * @menu will be positioned at the pointer associated with @trigger_event.
 *
 * Properties that influence the behaviour of this function are
 * #CtkMenu:anchor-hints, #CtkMenu:rect-anchor-dx, #CtkMenu:rect-anchor-dy, and
 * #CtkMenu:menu-type-hint. Connect to the #CtkMenu::popped-up signal to find
 * out how it was actually positioned.
 *
 * Since: 3.22
 */
void
ctk_menu_popup_at_pointer (CtkMenu        *menu,
                           const CdkEvent *trigger_event)
{
  CdkEvent *current_event = NULL;
  CdkWindow *rect_window = NULL;
  CdkDevice *device = NULL;
  CdkRectangle rect = { 0, 0, 1, 1 };

  g_return_if_fail (CTK_IS_MENU (menu));

  if (!trigger_event)
    {
      current_event = ctk_get_current_event ();
      trigger_event = current_event;
    }

  if (trigger_event)
    {
      rect_window = cdk_event_get_window (trigger_event);

      if (rect_window)
        {
          device = get_device_for_event (trigger_event);

          if (device && cdk_device_get_source (device) == CDK_SOURCE_KEYBOARD)
            device = cdk_device_get_associated_device (device);

          if (device)
            cdk_window_get_device_position (rect_window, device, &rect.x, &rect.y, NULL);
        }
    }
  else
    g_warning ("no trigger event for menu popup");

  ctk_menu_popup_at_rect (menu,
                          rect_window,
                          &rect,
                          CDK_GRAVITY_SOUTH_EAST,
                          CDK_GRAVITY_NORTH_WEST,
                          trigger_event);

  g_clear_pointer (&current_event, cdk_event_free);
}

static void
get_arrows_border (CtkMenu   *menu,
                   CtkBorder *border)
{
  CtkMenuPrivate *priv = menu->priv;
  gint top_arrow_height, bottom_arrow_height;

  ctk_css_gadget_get_preferred_size (priv->top_arrow_gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     -1,
                                     &top_arrow_height, NULL,
                                     NULL, NULL);
  ctk_css_gadget_get_preferred_size (priv->bottom_arrow_gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     -1,
                                     &bottom_arrow_height, NULL,
                                     NULL, NULL);

  border->top = priv->upper_arrow_visible ? top_arrow_height : 0;
  border->bottom = priv->lower_arrow_visible ? bottom_arrow_height : 0;
  border->left = border->right = 0;
}

/**
 * ctk_menu_update_scroll_offset:
 * @menu: the #CtkMenu that popped up
 * @flipped_rect: (nullable): the position of @menu after any possible flipping
 *                or %NULL if unknown
 * @final_rect: (nullable): the final position of @menu or %NULL if unknown
 * @flipped_x: %TRUE if the anchors were flipped horizontally
 * @flipped_y: %TRUE if the anchors were flipped vertically
 * @user_data: user data
 *
 * Updates the scroll offset of @menu based on the amount of sliding done while
 * positioning @menu. Connect this to the #CtkMenu::popped-up signal to keep the
 * contents of the menu vertically aligned with their ideal position, for combo
 * boxes for example.
 *
 * Since: 3.22
 * Stability: Private
 */
void
ctk_menu_update_scroll_offset (CtkMenu            *menu,
                               const CdkRectangle *flipped_rect,
                               const CdkRectangle *final_rect,
                               gboolean            flipped_x,
                               gboolean            flipped_y,
                               gpointer            user_data)
{
  CtkBorder arrows_border;

  g_return_if_fail (CTK_IS_MENU (menu));

  if (!flipped_rect || !final_rect)
    return;

  get_arrows_border (menu, &arrows_border);
  menu->priv->scroll_offset = arrows_border.top + (final_rect->y - flipped_rect->y);
  ctk_menu_scroll_to (menu, menu->priv->scroll_offset,
                      CTK_MENU_SCROLL_FLAG_ADAPT);
}

/**
 * ctk_menu_popdown:
 * @menu: a #CtkMenu
 *
 * Removes the menu from the screen.
 */
void
ctk_menu_popdown (CtkMenu *menu)
{
  CtkMenuPrivate *priv;
  CtkMenuShell *menu_shell;
  CdkDevice *pointer;

  g_return_if_fail (CTK_IS_MENU (menu));

  menu_shell = CTK_MENU_SHELL (menu);
  priv = menu->priv;

  menu_shell->priv->parent_menu_shell = NULL;
  menu_shell->priv->active = FALSE;
  menu_shell->priv->ignore_enter = FALSE;

  priv->have_position = FALSE;

  ctk_menu_stop_scrolling (menu);
  ctk_menu_stop_navigating_submenu (menu);

  if (menu_shell->priv->active_menu_item)
    {
      if (priv->old_active_menu_item)
        g_object_unref (priv->old_active_menu_item);
      priv->old_active_menu_item = menu_shell->priv->active_menu_item;
      g_object_ref (priv->old_active_menu_item);
    }

  ctk_menu_shell_deselect (menu_shell);

  /* The X Grab, if present, will automatically be removed
   * when we hide the window
   */
  if (priv->toplevel)
    {
      ctk_widget_hide (priv->toplevel);
      ctk_window_set_transient_for (CTK_WINDOW (priv->toplevel), NULL);
    }

  pointer = _ctk_menu_shell_get_grab_device (menu_shell);

  if (priv->torn_off)
    {
      ctk_widget_set_size_request (priv->tearoff_window, -1, -1);

      if (ctk_bin_get_child (CTK_BIN (priv->toplevel)))
        {
          ctk_menu_reparent (menu, priv->tearoff_hbox, TRUE);
        }
      else
        {
          /* We popped up the menu from the tearoff, so we need to
           * release the grab - we aren't actually hiding the menu.
           */
          if (menu_shell->priv->have_xgrab && pointer)
            cdk_seat_ungrab (cdk_device_get_seat (pointer));
        }

      /* ctk_menu_popdown is called each time a menu item is selected from
       * a torn off menu. Only scroll back to the saved position if the
       * non-tearoff menu was popped down.
       */
      if (!priv->tearoff_active)
        ctk_menu_scroll_to (menu, priv->saved_scroll_offset,
                            CTK_MENU_SCROLL_FLAG_NONE);
      priv->tearoff_active = TRUE;
    }
  else
    ctk_widget_hide (CTK_WIDGET (menu));

  menu_shell->priv->have_xgrab = FALSE;

  ctk_grab_remove (CTK_WIDGET (menu));

  _ctk_menu_shell_set_grab_device (menu_shell, NULL);

  menu_grab_transfer_window_destroy (menu);
}

/**
 * ctk_menu_get_active:
 * @menu: a #CtkMenu
 *
 * Returns the selected menu item from the menu.  This is used by the
 * #CtkComboBox.
 *
 * Returns: (transfer none): the #CtkMenuItem that was last selected
 *          in the menu.  If a selection has not yet been made, the
 *          first menu item is selected.
 */
CtkWidget*
ctk_menu_get_active (CtkMenu *menu)
{
  CtkMenuPrivate *priv;
  CtkWidget *child;
  GList *children;

  g_return_val_if_fail (CTK_IS_MENU (menu), NULL);

  priv = menu->priv;

  if (!priv->old_active_menu_item)
    {
      child = NULL;
      children = CTK_MENU_SHELL (menu)->priv->children;

      while (children)
        {
          child = children->data;
          children = children->next;

          if (ctk_bin_get_child (CTK_BIN (child)))
            break;
          child = NULL;
        }

      priv->old_active_menu_item = child;
      if (priv->old_active_menu_item)
        g_object_ref (priv->old_active_menu_item);
    }

  return priv->old_active_menu_item;
}

/**
 * ctk_menu_set_active:
 * @menu: a #CtkMenu
 * @index: the index of the menu item to select.  Index values are
 *         from 0 to n-1
 *
 * Selects the specified menu item within the menu.  This is used by
 * the #CtkComboBox and should not be used by anyone else.
 */
void
ctk_menu_set_active (CtkMenu *menu,
                     guint    index)
{
  CtkMenuPrivate *priv;
  CtkWidget *child;
  GList *tmp_list;

  g_return_if_fail (CTK_IS_MENU (menu));

  priv = menu->priv;

  tmp_list = g_list_nth (CTK_MENU_SHELL (menu)->priv->children, index);
  if (tmp_list)
    {
      child = tmp_list->data;
      if (ctk_bin_get_child (CTK_BIN (child)))
        {
          if (priv->old_active_menu_item)
            g_object_unref (priv->old_active_menu_item);
          priv->old_active_menu_item = child;
          g_object_ref (priv->old_active_menu_item);
        }
    }
  g_object_notify (G_OBJECT (menu), "active");
}

/**
 * ctk_menu_set_accel_group:
 * @menu: a #CtkMenu
 * @accel_group: (allow-none): the #CtkAccelGroup to be associated
 *               with the menu.
 *
 * Set the #CtkAccelGroup which holds global accelerators for the
 * menu.  This accelerator group needs to also be added to all windows
 * that this menu is being used in with ctk_window_add_accel_group(),
 * in order for those windows to support all the accelerators
 * contained in this group.
 */
void
ctk_menu_set_accel_group (CtkMenu       *menu,
                          CtkAccelGroup *accel_group)
{
  CtkMenuPrivate *priv;

  g_return_if_fail (CTK_IS_MENU (menu));
  g_return_if_fail (!accel_group || CTK_IS_ACCEL_GROUP (accel_group));

  priv = menu->priv;

  if (priv->accel_group != accel_group)
    {
      if (priv->accel_group)
        g_object_unref (priv->accel_group);
      priv->accel_group = accel_group;
      if (priv->accel_group)
        g_object_ref (priv->accel_group);
      _ctk_menu_refresh_accel_paths (menu, TRUE);
    }
}

/**
 * ctk_menu_get_accel_group:
 * @menu: a #CtkMenu
 *
 * Gets the #CtkAccelGroup which holds global accelerators for the
 * menu. See ctk_menu_set_accel_group().
 *
 * Returns: (transfer none): the #CtkAccelGroup associated with the menu
 */
CtkAccelGroup*
ctk_menu_get_accel_group (CtkMenu *menu)
{
  g_return_val_if_fail (CTK_IS_MENU (menu), NULL);

  return menu->priv->accel_group;
}

static gboolean
ctk_menu_real_can_activate_accel (CtkWidget *widget,
                                  guint      signal_id)
{
  /* Menu items chain here to figure whether they can activate their
   * accelerators.  Unlike ordinary widgets, menus allow accel
   * activation even if invisible since that's the usual case for
   * submenus/popup-menus. however, the state of the attach widget
   * affects the "activeness" of the menu.
   */
  CtkWidget *awidget = ctk_menu_get_attach_widget (CTK_MENU (widget));

  if (awidget)
    return ctk_widget_can_activate_accel (awidget, signal_id);
  else
    return ctk_widget_is_sensitive (widget);
}

/**
 * ctk_menu_set_accel_path:
 * @menu:       a valid #CtkMenu
 * @accel_path: (nullable): a valid accelerator path, or %NULL to unset the path
 *
 * Sets an accelerator path for this menu from which accelerator paths
 * for its immediate children, its menu items, can be constructed.
 * The main purpose of this function is to spare the programmer the
 * inconvenience of having to call ctk_menu_item_set_accel_path() on
 * each menu item that should support runtime user changable accelerators.
 * Instead, by just calling ctk_menu_set_accel_path() on their parent,
 * each menu item of this menu, that contains a label describing its
 * purpose, automatically gets an accel path assigned.
 *
 * For example, a menu containing menu items “New” and “Exit”, will, after
 * `ctk_menu_set_accel_path (menu, "<Gnumeric-Sheet>/File");` has been
 * called, assign its items the accel paths: `"<Gnumeric-Sheet>/File/New"`
 * and `"<Gnumeric-Sheet>/File/Exit"`.
 *
 * Assigning accel paths to menu items then enables the user to change
 * their accelerators at runtime. More details about accelerator paths
 * and their default setups can be found at ctk_accel_map_add_entry().
 *
 * Note that @accel_path string will be stored in a #GQuark. Therefore,
 * if you pass a static string, you can save some memory by interning
 * it first with g_intern_static_string().
 */
void
ctk_menu_set_accel_path (CtkMenu     *menu,
                         const gchar *accel_path)
{
  CtkMenuPrivate *priv;

  g_return_if_fail (CTK_IS_MENU (menu));

  priv = menu->priv;

  if (accel_path)
    g_return_if_fail (accel_path[0] == '<' && strchr (accel_path, '/')); /* simplistic check */

  priv->accel_path = g_intern_string (accel_path);
  if (priv->accel_path)
    _ctk_menu_refresh_accel_paths (menu, FALSE);
}

/**
 * ctk_menu_get_accel_path:
 * @menu: a valid #CtkMenu
 *
 * Retrieves the accelerator path set on the menu.
 *
 * Returns: the accelerator path set on the menu.
 *
 * Since: 2.14
 */
const gchar*
ctk_menu_get_accel_path (CtkMenu *menu)
{
  g_return_val_if_fail (CTK_IS_MENU (menu), NULL);

  return menu->priv->accel_path;
}

typedef struct {
  CtkMenu *menu;
  gboolean group_changed;
} AccelPropagation;

static void
refresh_accel_paths_foreach (CtkWidget *widget,
                             gpointer   data)
{
  CtkMenuPrivate *priv;
  AccelPropagation *prop = data;

  if (CTK_IS_MENU_ITEM (widget))  /* should always be true */
    {
      priv = prop->menu->priv;
      _ctk_menu_item_refresh_accel_path (CTK_MENU_ITEM (widget),
                                         priv->accel_path,
                                         priv->accel_group,
                                         prop->group_changed);
    }
}

static void
_ctk_menu_refresh_accel_paths (CtkMenu  *menu,
                               gboolean  group_changed)
{
  CtkMenuPrivate *priv = menu->priv;

  if (priv->accel_path)
    {
      AccelPropagation prop;

      prop.menu = menu;
      prop.group_changed = group_changed;
      ctk_container_foreach (CTK_CONTAINER (menu),
                             refresh_accel_paths_foreach,
                             &prop);
    }
}

/**
 * ctk_menu_reposition:
 * @menu: a #CtkMenu
 *
 * Repositions the menu according to its position function.
 */
void
ctk_menu_reposition (CtkMenu *menu)
{
  g_return_if_fail (CTK_IS_MENU (menu));

  if (!menu->priv->torn_off && ctk_widget_is_drawable (CTK_WIDGET (menu)))
    ctk_menu_position (menu, FALSE);
}

static void
ctk_menu_scrollbar_changed (CtkAdjustment *adjustment,
                            CtkMenu       *menu)
{
  double value;

  value = ctk_adjustment_get_value (adjustment);
  if (menu->priv->scroll_offset != value)
    ctk_menu_scroll_to (menu, value, CTK_MENU_SCROLL_FLAG_NONE);
}

static void
ctk_menu_set_tearoff_hints (CtkMenu *menu,
                            gint     width)
{
  CtkMenuPrivate *priv = menu->priv;
  CdkGeometry geometry_hints;

  if (!priv->tearoff_window)
    return;

  if (ctk_widget_get_visible (priv->tearoff_scrollbar))
    {
      CtkRequisition requisition;

      ctk_widget_get_preferred_size (priv->tearoff_scrollbar,
                                     &requisition, NULL);
      width += requisition.width;
    }

  geometry_hints.min_width = width;
  geometry_hints.max_width = width;

  geometry_hints.min_height = 0;
  geometry_hints.max_height = priv->requested_height;

  ctk_window_set_geometry_hints (CTK_WINDOW (priv->tearoff_window),
                                 NULL,
                                 &geometry_hints,
                                 CDK_HINT_MAX_SIZE|CDK_HINT_MIN_SIZE);
}

static void
ctk_menu_update_title (CtkMenu *menu)
{
  CtkMenuPrivate *priv = menu->priv;

  if (priv->tearoff_window)
    {
      const gchar *title;
      CtkWidget *attach_widget;

      title = ctk_menu_get_title (menu);

      if (!title)
        {
          attach_widget = ctk_menu_get_attach_widget (menu);
          if (CTK_IS_MENU_ITEM (attach_widget))
            {
              CtkWidget *child = ctk_bin_get_child (CTK_BIN (attach_widget));
              if (CTK_IS_LABEL (child))
                title = ctk_label_get_text (CTK_LABEL (child));
            }
        }

      if (title)
        ctk_window_set_title (CTK_WINDOW (priv->tearoff_window), title);
    }
}

static CtkWidget*
ctk_menu_get_toplevel (CtkWidget *menu)
{
  CtkWidget *attach, *toplevel;

  attach = ctk_menu_get_attach_widget (CTK_MENU (menu));

  if (CTK_IS_MENU_ITEM (attach))
    attach = ctk_widget_get_parent (attach);

  if (CTK_IS_MENU (attach))
    return ctk_menu_get_toplevel (attach);
  else if (CTK_IS_WIDGET (attach))
    {
      toplevel = ctk_widget_get_toplevel (attach);
      if (ctk_widget_is_toplevel (toplevel))
        return toplevel;
    }

  return NULL;
}

static void
tearoff_window_destroyed (CtkWidget *widget,
                          CtkMenu   *menu)
{
  ctk_menu_set_tearoff_state (menu, FALSE);
}

/**
 * ctk_menu_set_tearoff_state:
 * @menu: a #CtkMenu
 * @torn_off: If %TRUE, menu is displayed as a tearoff menu.
 *
 * Changes the tearoff state of the menu.  A menu is normally
 * displayed as drop down menu which persists as long as the menu is
 * active.  It can also be displayed as a tearoff menu which persists
 * until it is closed or reattached.
 */
void
ctk_menu_set_tearoff_state (CtkMenu  *menu,
                            gboolean  torn_off)
{
  CtkMenuPrivate *priv;
  gint height;

  g_return_if_fail (CTK_IS_MENU (menu));

  priv = menu->priv;

  torn_off = !!torn_off;
  if (priv->torn_off != torn_off)
    {
      priv->torn_off = torn_off;
      priv->tearoff_active = torn_off;

      if (priv->torn_off)
        {
          if (ctk_widget_get_visible (CTK_WIDGET (menu)))
            ctk_menu_popdown (menu);

          if (!priv->tearoff_window)
            {
              CtkWidget *toplevel;

              priv->tearoff_window = g_object_new (CTK_TYPE_WINDOW,
                                                   "type", CTK_WINDOW_TOPLEVEL,
                                                   "screen", ctk_widget_get_screen (priv->toplevel),
                                                   "app-paintable", TRUE,
                                                   NULL);

              ctk_window_set_type_hint (CTK_WINDOW (priv->tearoff_window),
                                        CDK_WINDOW_TYPE_HINT_MENU);
              ctk_window_set_mnemonic_modifier (CTK_WINDOW (priv->tearoff_window), 0);
              g_signal_connect (priv->tearoff_window, "destroy",
                                G_CALLBACK (tearoff_window_destroyed), menu);
              g_signal_connect (priv->tearoff_window, "event",
                                G_CALLBACK (ctk_menu_window_event), menu);

              ctk_menu_update_title (menu);

              ctk_widget_realize (priv->tearoff_window);

              toplevel = ctk_menu_get_toplevel (CTK_WIDGET (menu));
              if (toplevel != NULL)
                ctk_window_set_transient_for (CTK_WINDOW (priv->tearoff_window),
                                              CTK_WINDOW (toplevel));

              priv->tearoff_hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
              ctk_container_add (CTK_CONTAINER (priv->tearoff_window),
                                 priv->tearoff_hbox);

              height = cdk_window_get_height (ctk_widget_get_window (CTK_WIDGET (menu)));
              priv->tearoff_adjustment = ctk_adjustment_new (0,
                                                             0, priv->requested_height,
                                                             MENU_SCROLL_STEP2,
                                                             height/2,
                                                             height);
              g_object_connect (priv->tearoff_adjustment,
                                "signal::value-changed", ctk_menu_scrollbar_changed, menu,
                                NULL);
              priv->tearoff_scrollbar = ctk_scrollbar_new (CTK_ORIENTATION_VERTICAL, priv->tearoff_adjustment);

              ctk_box_pack_end (CTK_BOX (priv->tearoff_hbox),
                                priv->tearoff_scrollbar,
                                FALSE, FALSE, 0);

              if (ctk_adjustment_get_upper (priv->tearoff_adjustment) > height)
                ctk_widget_show (priv->tearoff_scrollbar);

              ctk_widget_show (priv->tearoff_hbox);
            }

          ctk_menu_reparent (menu, priv->tearoff_hbox, FALSE);

          /* Update menu->requisition */
          ctk_widget_get_preferred_size (CTK_WIDGET (menu), NULL, NULL);

          ctk_menu_set_tearoff_hints (menu, cdk_window_get_width (ctk_widget_get_window (CTK_WIDGET (menu))));

          ctk_widget_realize (priv->tearoff_window);
          ctk_menu_position (menu, TRUE);

          ctk_widget_show (CTK_WIDGET (menu));
          ctk_widget_show (priv->tearoff_window);

          ctk_menu_scroll_to (menu, 0, CTK_MENU_SCROLL_FLAG_NONE);

        }
      else
        {
          ctk_widget_hide (CTK_WIDGET (menu));
          ctk_widget_hide (priv->tearoff_window);
          if (CTK_IS_CONTAINER (priv->toplevel))
            ctk_menu_reparent (menu, priv->toplevel, FALSE);
          ctk_widget_destroy (priv->tearoff_window);

          priv->tearoff_window = NULL;
          priv->tearoff_hbox = NULL;
          priv->tearoff_scrollbar = NULL;
          priv->tearoff_adjustment = NULL;
        }

      g_object_notify (G_OBJECT (menu), "tearoff-state");
    }
}

/**
 * ctk_menu_get_tearoff_state:
 * @menu: a #CtkMenu
 *
 * Returns whether the menu is torn off.
 * See ctk_menu_set_tearoff_state().
 *
 * Returns: %TRUE if the menu is currently torn off.
 */
gboolean
ctk_menu_get_tearoff_state (CtkMenu *menu)
{
  g_return_val_if_fail (CTK_IS_MENU (menu), FALSE);

  return menu->priv->torn_off;
}

/**
 * ctk_menu_set_title:
 * @menu: a #CtkMenu
 * @title: (nullable): a string containing the title for the menu, or %NULL to
 *   inherit the title of the parent menu item, if any
 *
 * Sets the title string for the menu.
 *
 * The title is displayed when the menu is shown as a tearoff
 * menu. If @title is %NULL, the menu will see if it is attached
 * to a parent menu item, and if so it will try to use the same
 * text as that menu item’s label.
 */
void
ctk_menu_set_title (CtkMenu     *menu,
                    const gchar *title)
{
  CtkMenuPrivate *priv;
  char *old_title;

  g_return_if_fail (CTK_IS_MENU (menu));

  priv = menu->priv;

  old_title = priv->title;
  priv->title = g_strdup (title);
  g_free (old_title);

  ctk_menu_update_title (menu);
  g_object_notify (G_OBJECT (menu), "tearoff-title");
}

/**
 * ctk_menu_get_title:
 * @menu: a #CtkMenu
 *
 * Returns the title of the menu. See ctk_menu_set_title().
 *
 * Returns: the title of the menu, or %NULL if the menu
 *     has no title set on it. This string is owned by CTK+
 *     and should not be modified or freed.
 **/
const gchar *
ctk_menu_get_title (CtkMenu *menu)
{
  g_return_val_if_fail (CTK_IS_MENU (menu), NULL);

  return menu->priv->title;
}

/**
 * ctk_menu_reorder_child:
 * @menu: a #CtkMenu
 * @child: the #CtkMenuItem to move
 * @position: the new position to place @child.
 *     Positions are numbered from 0 to n - 1
 *
 * Moves @child to a new @position in the list of @menu
 * children.
 */
void
ctk_menu_reorder_child (CtkMenu   *menu,
                        CtkWidget *child,
                        gint       position)
{
  CtkMenuShell *menu_shell;

  g_return_if_fail (CTK_IS_MENU (menu));
  g_return_if_fail (CTK_IS_MENU_ITEM (child));

  menu_shell = CTK_MENU_SHELL (menu);

  if (g_list_find (menu_shell->priv->children, child))
    {
      menu_shell->priv->children = g_list_remove (menu_shell->priv->children, child);
      menu_shell->priv->children = g_list_insert (menu_shell->priv->children, child, position);

      menu_queue_resize (menu);
    }
}

static void
get_menu_padding (CtkWidget *widget,
                  CtkBorder *padding)
{
  CtkStyleContext *context;

  context = ctk_widget_get_style_context (widget);

  ctk_style_context_get_padding (context,
                                 ctk_style_context_get_state (context),
                                 padding);
}

static void
get_menu_margin (CtkWidget *widget,
                 CtkBorder *margin)
{
  CtkStyleContext *context;

  context = ctk_widget_get_style_context (widget);

  ctk_style_context_get_margin (context,
                                ctk_style_context_get_state (context),
                                margin);
}

static void
ctk_menu_realize (CtkWidget *widget)
{
  CtkMenu *menu = CTK_MENU (widget);
  CtkMenuPrivate *priv = menu->priv;
  CtkAllocation allocation;
  CdkWindow *window;
  CdkWindowAttr attributes;
  gint attributes_mask;
  gint border_width;
  CtkWidget *child;
  GList *children;
  CtkBorder arrow_border, padding;

  g_return_if_fail (CTK_IS_MENU (widget));

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
  attributes.event_mask |= (CDK_KEY_PRESS_MASK |
                            CDK_ENTER_NOTIFY_MASK | CDK_LEAVE_NOTIFY_MASK );

  attributes_mask = CDK_WA_X | CDK_WA_Y | CDK_WA_VISUAL;

  window = cdk_window_new (ctk_widget_get_parent_window (widget),
                           &attributes, attributes_mask);
  ctk_widget_set_window (widget, window);
  ctk_widget_register_window (widget, window);

  get_menu_padding (widget, &padding);
  border_width = ctk_container_get_border_width (CTK_CONTAINER (widget));

  ctk_widget_get_allocation (widget, &allocation);

  attributes.x = border_width + padding.left;
  attributes.y = border_width + padding.top;
  attributes.width = allocation.width -
    (2 * border_width) - padding.left - padding.right;
  attributes.height = allocation.height -
    (2 * border_width) - padding.top - padding.bottom;

  get_arrows_border (menu, &arrow_border);
  attributes.y += arrow_border.top;
  attributes.height -= arrow_border.top;
  attributes.height -= arrow_border.bottom;

  attributes.width = MAX (1, attributes.width);
  attributes.height = MAX (1, attributes.height);

  priv->view_window = cdk_window_new (window,
                                      &attributes, attributes_mask);
  ctk_widget_register_window (widget, priv->view_window);

  ctk_widget_get_allocation (widget, &allocation);

  attributes.x = 0;
  attributes.y = - priv->scroll_offset;
  attributes.width = allocation.width + (2 * border_width) +
    padding.left + padding.right;
  attributes.height = priv->requested_height - (2 * border_width) +
    padding.top + padding.bottom;

  attributes.width = MAX (1, attributes.width);
  attributes.height = MAX (1, attributes.height);

  priv->bin_window = cdk_window_new (priv->view_window,
                                     &attributes, attributes_mask);
  ctk_widget_register_window (widget, priv->bin_window);

  children = CTK_MENU_SHELL (menu)->priv->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      ctk_widget_set_parent_window (child, priv->bin_window);
    }

  if (CTK_MENU_SHELL (widget)->priv->active_menu_item)
    ctk_menu_scroll_item_visible (CTK_MENU_SHELL (widget),
                                  CTK_MENU_SHELL (widget)->priv->active_menu_item);

  cdk_window_show (priv->bin_window);
  cdk_window_show (priv->view_window);
}

static gboolean
ctk_menu_focus (CtkWidget       *widget,
                CtkDirectionType direction)
{
  /* A menu or its menu items cannot have focus */
  return FALSE;
}

/* See notes in ctk_menu_popup() for information
 * about the “grab transfer window”
 */
static CdkWindow *
menu_grab_transfer_window_get (CtkMenu *menu)
{
  CdkWindow *window = g_object_get_data (G_OBJECT (menu), "ctk-menu-transfer-window");
  if (!window)
    {
      CdkWindowAttr attributes;
      gint attributes_mask;
      CdkWindow *parent;

      attributes.x = -100;
      attributes.y = -100;
      attributes.width = 10;
      attributes.height = 10;
      attributes.window_type = CDK_WINDOW_TEMP;
      attributes.wclass = CDK_INPUT_ONLY;
      attributes.override_redirect = TRUE;
      attributes.event_mask = 0;

      attributes_mask = CDK_WA_X | CDK_WA_Y | CDK_WA_NOREDIR;

      parent = cdk_screen_get_root_window (ctk_widget_get_screen (CTK_WIDGET (menu)));
      window = cdk_window_new (parent,
                               &attributes, attributes_mask);
      ctk_widget_register_window (CTK_WIDGET (menu), window);

      cdk_window_show (window);

      g_object_set_data (G_OBJECT (menu), I_("ctk-menu-transfer-window"), window);
    }

  return window;
}

static void
menu_grab_transfer_window_destroy (CtkMenu *menu)
{
  CtkMenuPrivate *priv = menu->priv;
  CdkWindow *window = g_object_get_data (G_OBJECT (menu), "ctk-menu-transfer-window");
  if (window)
    {
      CdkWindow *toplevel_window;

      ctk_widget_unregister_window (CTK_WIDGET (menu), window);
      cdk_window_destroy (window);
      g_object_set_data (G_OBJECT (menu), I_("ctk-menu-transfer-window"), NULL);

      toplevel_window = ctk_widget_get_window (priv->toplevel);

      if (toplevel_window != NULL)
        g_object_set_data (G_OBJECT (toplevel_window), I_("cdk-attached-grab-window"), NULL);
    }
}

static void
ctk_menu_unrealize (CtkWidget *widget)
{
  CtkMenu *menu = CTK_MENU (widget);
  CtkMenuPrivate *priv = menu->priv;

  menu_grab_transfer_window_destroy (menu);

  ctk_widget_unregister_window (widget, priv->view_window);
  cdk_window_destroy (priv->view_window);
  priv->view_window = NULL;

  ctk_widget_unregister_window (widget, priv->bin_window);
  cdk_window_destroy (priv->bin_window);
  priv->bin_window = NULL;

  CTK_WIDGET_CLASS (ctk_menu_parent_class)->unrealize (widget);
}

static gint
calculate_line_heights (CtkMenu *menu,
                        gint     for_width,
                        guint  **ret_min_heights,
                        guint  **ret_nat_heights)
{
  CtkBorder       padding;
  CtkMenuPrivate *priv;
  CtkMenuShell   *menu_shell;
  CtkWidget      *child, *widget;
  GList          *children;
  guint           border_width;
  guint           n_columns;
  gint            n_heights;
  guint          *min_heights;
  guint          *nat_heights;
  gint            avail_width;

  priv         = menu->priv;
  widget       = CTK_WIDGET (menu);
  menu_shell   = CTK_MENU_SHELL (widget);

  min_heights  = g_new0 (guint, ctk_menu_get_n_rows (menu));
  nat_heights  = g_new0 (guint, ctk_menu_get_n_rows (menu));
  n_heights    = ctk_menu_get_n_rows (menu);
  n_columns    = ctk_menu_get_n_columns (menu);
  avail_width  = for_width - (2 * priv->toggle_size + priv->accel_size) * n_columns;

  get_menu_padding (widget, &padding);

  border_width = ctk_container_get_border_width (CTK_CONTAINER (menu));
  avail_width -= (border_width) * 2 + padding.left + padding.right;

  for (children = menu_shell->priv->children; children; children = children->next)
    {
      gint part;
      gint toggle_size;
      gint l, r, t, b;
      gint child_min, child_nat;

      child = children->data;

      if (!ctk_widget_get_visible (child))
        continue;

      get_effective_child_attach (child, &l, &r, &t, &b);

      part = avail_width / (r - l);

      ctk_widget_get_preferred_height_for_width (child, part,
                                                 &child_min, &child_nat);

      ctk_menu_item_toggle_size_request (CTK_MENU_ITEM (child), &toggle_size);

      part = MAX (child_min, toggle_size) / (b - t);
      min_heights[t] = MAX (min_heights[t], part);

      part = MAX (child_nat, toggle_size) / (b - t);
      nat_heights[t] = MAX (nat_heights[t], part);
    }

  if (ret_min_heights)
    *ret_min_heights = min_heights;
  else
    g_free (min_heights);

  if (ret_nat_heights)
    *ret_nat_heights = nat_heights;
  else
    g_free (nat_heights);

  return n_heights;
}

static void
ctk_menu_size_allocate (CtkWidget     *widget,
                        CtkAllocation *allocation)
{
  CtkMenu *menu;
  CtkMenuPrivate *priv;
  CtkMenuShell *menu_shell;
  CtkWidget *child;
  CtkAllocation arrow_allocation, child_allocation;
  CtkAllocation clip;
  GList *children;
  gint x, y, i;
  gint width, height;
  guint border_width;
  CtkBorder arrow_border, padding;

  g_return_if_fail (CTK_IS_MENU (widget));
  g_return_if_fail (allocation != NULL);

  menu = CTK_MENU (widget);
  menu_shell = CTK_MENU_SHELL (widget);
  priv = menu->priv;

  ctk_widget_set_allocation (widget, allocation);

  get_menu_padding (widget, &padding);
  border_width = ctk_container_get_border_width (CTK_CONTAINER (menu));

  g_free (priv->heights);
  priv->heights_length = calculate_line_heights (menu,
                                                 allocation->width,
                                                 &priv->heights,
                                                 NULL);

  /* refresh our cached height request */
  priv->requested_height = (2 * border_width) + padding.top + padding.bottom;
  for (i = 0; i < priv->heights_length; i++)
    priv->requested_height += priv->heights[i];

  x = border_width + padding.left;
  y = border_width + padding.top;
  width = allocation->width - (2 * border_width) - padding.left - padding.right;
  height = allocation->height - (2 * border_width) - padding.top - padding.bottom;

  if (menu_shell->priv->active)
    ctk_menu_scroll_to (menu, priv->scroll_offset, CTK_MENU_SCROLL_FLAG_NONE);

  get_arrows_border (menu, &arrow_border);

  arrow_allocation.x = x;
  arrow_allocation.y = y;
  arrow_allocation.width = width;
  arrow_allocation.height = arrow_border.top;

  if (priv->upper_arrow_visible)
    ctk_css_gadget_allocate (priv->top_arrow_gadget,
                             &arrow_allocation, -1,
                             &clip);

  arrow_allocation.y = height - y - arrow_border.bottom;
  arrow_allocation.height = arrow_border.bottom;

  if (priv->lower_arrow_visible)
    ctk_css_gadget_allocate (priv->bottom_arrow_gadget,
                             &arrow_allocation, -1,
                             &clip);

  if (!priv->tearoff_active)
    {
      y += arrow_border.top;
      height -= arrow_border.top;
      height -= arrow_border.bottom;
    }

  width = MAX (1, width);
  height = MAX (1, height);

  if (ctk_widget_get_realized (widget))
    {
      cdk_window_move_resize (ctk_widget_get_window (widget),
                              allocation->x, allocation->y,
                              allocation->width, allocation->height);

      cdk_window_move_resize (priv->view_window, x, y, width, height);
    }

  if (menu_shell->priv->children)
    {
      gint base_width = width / ctk_menu_get_n_columns (menu);

      children = menu_shell->priv->children;
      while (children)
        {
          child = children->data;
          children = children->next;

          if (ctk_widget_get_visible (child))
            {
              gint l, r, t, b;

              get_effective_child_attach (child, &l, &r, &t, &b);

              if (ctk_widget_get_direction (CTK_WIDGET (menu)) == CTK_TEXT_DIR_RTL)
                {
                  guint tmp;
                  tmp = ctk_menu_get_n_columns (menu) - l;
                  l = ctk_menu_get_n_columns (menu) - r;
                  r = tmp;
                }

              child_allocation.width = (r - l) * base_width;
              child_allocation.height = 0;
              child_allocation.x = l * base_width;
              child_allocation.y = 0;

              for (i = 0; i < b; i++)
                {
                  if (i < t)
                    child_allocation.y += priv->heights[i];
                  else
                    child_allocation.height += priv->heights[i];
                }

              ctk_menu_item_toggle_size_allocate (CTK_MENU_ITEM (child),
                                                  priv->toggle_size);

              ctk_widget_size_allocate (child, &child_allocation);
              ctk_widget_queue_draw (child);
            }
        }

      /* Resize the item window */
      if (ctk_widget_get_realized (widget))
        {
          gint w, h;

          h = 0;
          for (i = 0; i < ctk_menu_get_n_rows (menu); i++)
            h += priv->heights[i];

          w = ctk_menu_get_n_columns (menu) * base_width;
          cdk_window_resize (priv->bin_window, w, h);
        }

      if (priv->tearoff_active)
        {
          if (height >= priv->requested_height)
            {
              if (ctk_widget_get_visible (priv->tearoff_scrollbar))
                {
                  ctk_widget_hide (priv->tearoff_scrollbar);
                  ctk_menu_set_tearoff_hints (menu, allocation->width);

                  ctk_menu_scroll_to (menu, 0, CTK_MENU_SCROLL_FLAG_NONE);
                }
            }
          else
            {
              ctk_adjustment_configure (priv->tearoff_adjustment,
                                        ctk_adjustment_get_value (priv->tearoff_adjustment),
                                        0,
                                        priv->requested_height,
                                        ctk_adjustment_get_step_increment (priv->tearoff_adjustment),
                                        ctk_adjustment_get_page_increment (priv->tearoff_adjustment),
                                        allocation->height);

              if (!ctk_widget_get_visible (priv->tearoff_scrollbar))
                {
                  ctk_widget_show (priv->tearoff_scrollbar);
                  ctk_menu_set_tearoff_hints (menu, allocation->width);
                }
            }
        }
    }
}

static gboolean
ctk_menu_draw (CtkWidget *widget,
               cairo_t   *cr)
{
  CtkMenu *menu;
  CtkMenuPrivate *priv;
  CtkStyleContext *context;
  gint width, height;

  menu = CTK_MENU (widget);
  priv = menu->priv;
  context = ctk_widget_get_style_context (widget);

  width = ctk_widget_get_allocated_width (widget);
  height = ctk_widget_get_allocated_height (widget);

  if (ctk_cairo_should_draw_window (cr, ctk_widget_get_window (widget)))
    {
      ctk_render_background (context, cr, 0, 0,
                             width, height);
      ctk_render_frame (context, cr, 0, 0,
                        width, height);

      if (priv->upper_arrow_visible && !priv->tearoff_active)
        ctk_css_gadget_draw (priv->top_arrow_gadget, cr);

      if (priv->lower_arrow_visible && !priv->tearoff_active)
        ctk_css_gadget_draw (priv->bottom_arrow_gadget, cr);
    }

  if (ctk_cairo_should_draw_window (cr, priv->bin_window))
    {
      int x, y;

      cdk_window_get_position (priv->view_window, &x, &y);
      cairo_rectangle (cr,
                       x, y,
                       cdk_window_get_width (priv->view_window),
                       cdk_window_get_height (priv->view_window));
      cairo_clip (cr);

      CTK_WIDGET_CLASS (ctk_menu_parent_class)->draw (widget, cr);
    }

  return FALSE;
}

static void
ctk_menu_show (CtkWidget *widget)
{
  CtkMenu *menu = CTK_MENU (widget);

  _ctk_menu_refresh_accel_paths (menu, FALSE);

  CTK_WIDGET_CLASS (ctk_menu_parent_class)->show (widget);
}


static void 
ctk_menu_get_preferred_width (CtkWidget *widget,
                              gint      *minimum_size,
                              gint      *natural_size)
{
  CtkMenu        *menu;
  CtkMenuShell   *menu_shell;
  CtkMenuPrivate *priv;
  CtkWidget      *child;
  GList          *children;
  guint           max_toggle_size;
  guint           max_accel_width;
  guint           border_width;
  gint            child_min, child_nat;
  gint            min_width, nat_width;
  CtkBorder       padding;

  menu       = CTK_MENU (widget);
  menu_shell = CTK_MENU_SHELL (widget);
  priv       = menu->priv;

  min_width = nat_width = 0;

  max_toggle_size = 0;
  max_accel_width = 0;

  children = menu_shell->priv->children;
  while (children)
    {
      gint part;
      gint toggle_size;
      gint l, r, t, b;

      child = children->data;
      children = children->next;

      if (! ctk_widget_get_visible (child))
        continue;

      get_effective_child_attach (child, &l, &r, &t, &b);

      /* It's important to size_request the child
       * before doing the toggle size request, in
       * case the toggle size request depends on the size
       * request of a child of the child (e.g. for ImageMenuItem)
       */
       ctk_widget_get_preferred_width (child, &child_min, &child_nat);

       ctk_menu_item_toggle_size_request (CTK_MENU_ITEM (child), &toggle_size);
       max_toggle_size = MAX (max_toggle_size, toggle_size);
       max_accel_width = MAX (max_accel_width,
                              CTK_MENU_ITEM (child)->priv->accelerator_width);

       part = child_min / (r - l);
       min_width = MAX (min_width, part);

       part = child_nat / (r - l);
       nat_width = MAX (nat_width, part);
    }

  /* If the menu doesn't include any images or check items
   * reserve the space so that all menus are consistent.
   * We only do this for 'ordinary' menus, not for combobox
   * menus or multi-column menus
   */
  if (max_toggle_size == 0 &&
      ctk_menu_get_n_columns (menu) == 1 &&
      !priv->no_toggle_size)
    {
      CtkWidget *menu_item;
      CtkCssGadget *indicator_gadget;
      gint indicator_width;

      /* Create a CtkCheckMenuItem, to query indicator size */
      menu_item = ctk_check_menu_item_new ();
      indicator_gadget = _ctk_check_menu_item_get_indicator_gadget
        (CTK_CHECK_MENU_ITEM (menu_item));

      ctk_css_gadget_get_preferred_size (indicator_gadget,
                                         CTK_ORIENTATION_HORIZONTAL,
                                         -1,
                                         &indicator_width, NULL,
                                         NULL, NULL);
      max_toggle_size = indicator_width;

      ctk_widget_destroy (menu_item);
      g_object_ref_sink (menu_item);
      g_object_unref (menu_item);
    }

  min_width += 2 * max_toggle_size + max_accel_width;
  min_width *= ctk_menu_get_n_columns (menu);

  nat_width += 2 * max_toggle_size + max_accel_width;
  nat_width *= ctk_menu_get_n_columns (menu);

  get_menu_padding (widget, &padding);
  border_width = ctk_container_get_border_width (CTK_CONTAINER (menu));
  min_width   += (2 * border_width) + padding.left + padding.right;
  nat_width   += (2 * border_width) + padding.left + padding.right;

  priv->toggle_size = max_toggle_size;
  priv->accel_size  = max_accel_width;

  *minimum_size = min_width;
  *natural_size = nat_width;

  /* Don't resize the tearoff if it is not active,
   * because it won't redraw (it is only a background pixmap).
   */
  if (priv->tearoff_active)
    ctk_menu_set_tearoff_hints (menu, min_width);
}

static void
ctk_menu_get_preferred_height (CtkWidget *widget,
                               gint      *minimum_size,
                               gint      *natural_size)
{
  gint min_width, nat_width;

  /* Menus are height-for-width only, just return the height
   * for the minimum width
   */
  CTK_WIDGET_GET_CLASS (widget)->get_preferred_width (widget, &min_width, &nat_width);
  CTK_WIDGET_GET_CLASS (widget)->get_preferred_height_for_width (widget, min_width, minimum_size, natural_size);
}

static void
ctk_menu_get_preferred_height_for_width (CtkWidget *widget,
                                         gint       for_size,
                                         gint      *minimum_size,
                                         gint      *natural_size)
{
  CtkBorder       padding, arrow_border;
  CtkMenu        *menu = CTK_MENU (widget);
  CtkMenuPrivate *priv = menu->priv;
  guint          *min_heights, *nat_heights;
  guint           border_width;
  gint            n_heights, i;
  gint            min_height, single_height, nat_height;

  border_width = ctk_container_get_border_width (CTK_CONTAINER (menu));
  get_menu_padding (widget, &padding);

  min_height = nat_height = (2 * border_width) + padding.top + padding.bottom;
  single_height = 0;

  n_heights =
    calculate_line_heights (menu, for_size, &min_heights, &nat_heights);

  for (i = 0; i < n_heights; i++)
    {
      min_height += min_heights[i];
      single_height = MAX (single_height, min_heights[i]);
      nat_height += nat_heights[i];
    }

  get_arrows_border (menu, &arrow_border);
  single_height += (2 * border_width) 
                   + padding.top + padding.bottom
                   + arrow_border.top + arrow_border.bottom;
  min_height = MIN (min_height, single_height);

  if (priv->have_position)
    {
      CdkDisplay *display;
      CdkMonitor *monitor;
      CdkRectangle workarea;
      CtkBorder border;

      display = ctk_widget_get_display (priv->toplevel);
      monitor = cdk_display_get_monitor (display, priv->monitor_num);
      cdk_monitor_get_workarea (monitor, &workarea);

      if (priv->position_y + min_height > workarea.y + workarea.height)
        min_height = workarea.y + workarea.height - priv->position_y;

      if (priv->position_y + nat_height > workarea.y + workarea.height)
        nat_height = workarea.y + workarea.height - priv->position_y;

      _ctk_window_get_shadow_width (CTK_WINDOW (priv->toplevel), &border);

      if (priv->position_y + border.top < workarea.y)
        {
          min_height -= workarea.y - (priv->position_y + border.top);
          nat_height -= workarea.y - (priv->position_y + border.top);
        }
    }

  *minimum_size = min_height;
  *natural_size = nat_height;

  g_free (min_heights);
  g_free (nat_heights);
}

static gboolean
pointer_in_menu_window (CtkWidget *widget,
                        gdouble    x_root,
                        gdouble    y_root)
{
  CtkMenu *menu = CTK_MENU (widget);
  CtkMenuPrivate *priv = menu->priv;
  CtkAllocation allocation;

  if (ctk_widget_get_mapped (priv->toplevel))
    {
      CtkMenuShell *menu_shell;
      gint          window_x, window_y;

      cdk_window_get_position (ctk_widget_get_window (priv->toplevel),
                               &window_x, &window_y);

      ctk_widget_get_allocation (widget, &allocation);
      if (x_root >= window_x && x_root < window_x + allocation.width &&
          y_root >= window_y && y_root < window_y + allocation.height)
        return TRUE;

      menu_shell = CTK_MENU_SHELL (widget);

      if (CTK_IS_MENU (menu_shell->priv->parent_menu_shell))
        return pointer_in_menu_window (menu_shell->priv->parent_menu_shell,
                                       x_root, y_root);
    }

  return FALSE;
}

static gboolean
ctk_menu_button_press (CtkWidget      *widget,
                       CdkEventButton *event)
{
  CdkDevice *source_device;
  CtkWidget *event_widget;
  CtkMenu *menu;

  if (event->type != CDK_BUTTON_PRESS)
    return FALSE;

  source_device = cdk_event_get_source_device ((CdkEvent *) event);
  event_widget = ctk_get_event_widget ((CdkEvent *) event);
  menu = CTK_MENU (widget);

  /*  Don't pass down to menu shell if a non-menuitem part of the menu
   *  was clicked. The check for the event_widget being a CtkMenuShell
   *  works because we have the pointer grabbed on menu_shell->window
   *  with owner_events=TRUE, so all events that are either outside
   *  the menu or on its border are delivered relative to
   *  menu_shell->window.
   */
  if (CTK_IS_MENU_SHELL (event_widget) &&
      pointer_in_menu_window (widget, event->x_root, event->y_root))
    return TRUE;

  if (CTK_IS_MENU_ITEM (event_widget) &&
      cdk_device_get_source (source_device) == CDK_SOURCE_TOUCHSCREEN &&
      CTK_MENU_ITEM (event_widget)->priv->submenu != NULL &&
      !ctk_widget_is_drawable (CTK_MENU_ITEM (event_widget)->priv->submenu))
    menu->priv->ignore_button_release = TRUE;

  return CTK_WIDGET_CLASS (ctk_menu_parent_class)->button_press_event (widget, event);
}

static gboolean
ctk_menu_button_release (CtkWidget      *widget,
                         CdkEventButton *event)
{
  CtkMenuPrivate *priv = CTK_MENU (widget)->priv;

  if (priv->ignore_button_release)
    {
      priv->ignore_button_release = FALSE;
      return FALSE;
    }

  if (event->type != CDK_BUTTON_RELEASE)
    return FALSE;

  /*  Don't pass down to menu shell if a non-menuitem part of the menu
   *  was clicked (see comment in button_press()).
   */
  if (CTK_IS_MENU_SHELL (ctk_get_event_widget ((CdkEvent *) event)) &&
      pointer_in_menu_window (widget, event->x_root, event->y_root))
    {
      /*  Ugly: make sure menu_shell->button gets reset to 0 when we
       *  bail out early here so it is in a consistent state for the
       *  next button_press/button_release in CtkMenuShell.
       *  See bug #449371.
       */
      if (CTK_MENU_SHELL (widget)->priv->active)
        CTK_MENU_SHELL (widget)->priv->button = 0;

      return TRUE;
    }

  return CTK_WIDGET_CLASS (ctk_menu_parent_class)->button_release_event (widget, event);
}

static gboolean
ctk_menu_key_press (CtkWidget   *widget,
                    CdkEventKey *event)
{
  CtkMenu *menu;

  g_return_val_if_fail (CTK_IS_MENU (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  menu = CTK_MENU (widget);

  ctk_menu_stop_navigating_submenu (menu);

  return CTK_WIDGET_CLASS (ctk_menu_parent_class)->key_press_event (widget, event);
}

static gboolean
check_threshold (CtkWidget *widget,
                 gint       start_x,
                 gint       start_y,
                 gint       x,
                 gint       y)
{
#define THRESHOLD 8

  return
    ABS (start_x - x) > THRESHOLD ||
    ABS (start_y - y) > THRESHOLD;
}

static gboolean
definitely_within_item (CtkWidget *widget,
                        gint       x,
                        gint       y)
{
  CdkWindow *window = CTK_MENU_ITEM (widget)->priv->event_window;
  int w, h;

  w = cdk_window_get_width (window);
  h = cdk_window_get_height (window);

  return
    check_threshold (widget, 0, 0, x, y) &&
    check_threshold (widget, w - 1, 0, x, y) &&
    check_threshold (widget, w - 1, h - 1, x, y) &&
    check_threshold (widget, 0, h - 1, x, y);
}

static gboolean
ctk_menu_has_navigation_triangle (CtkMenu *menu)
{
  CtkMenuPrivate *priv = menu->priv;

  return priv->navigation_height && priv->navigation_width;
}

static gboolean
ctk_menu_motion_notify (CtkWidget      *widget,
                        CdkEventMotion *event)
{
  CtkWidget *menu_item;
  CtkMenu *menu;
  CtkMenuShell *menu_shell;
  CtkWidget *parent;
  CdkDevice *source_device;

  gboolean need_enter;

  source_device = cdk_event_get_source_device ((CdkEvent *) event);

  if (CTK_IS_MENU (widget) &&
      cdk_device_get_source (source_device) != CDK_SOURCE_TOUCHSCREEN)
    {
      CtkMenuPrivate *priv = CTK_MENU(widget)->priv;

      if (priv->ignore_button_release)
        priv->ignore_button_release = FALSE;

      ctk_menu_handle_scrolling (CTK_MENU (widget), event->x_root, event->y_root,
                                 TRUE, TRUE);
    }

  /* We received the event for one of two reasons:
   *
   * a) We are the active menu, and did ctk_grab_add()
   * b) The widget is a child of ours, and the event was propagated
   *
   * Since for computation of navigation regions, we want the menu which
   * is the parent of the menu item, for a), we need to find that menu,
   * which may be different from 'widget'.
   */
  menu_item = ctk_get_event_widget ((CdkEvent*) event);
  parent = ctk_widget_get_parent (menu_item);
  if (!CTK_IS_MENU_ITEM (menu_item) ||
      !CTK_IS_MENU (parent))
    return FALSE;

  menu_shell = CTK_MENU_SHELL (parent);
  menu = CTK_MENU (menu_shell);

  if (definitely_within_item (menu_item, event->x, event->y))
    menu_shell->priv->activate_time = 0;

  need_enter = (ctk_menu_has_navigation_triangle (menu) || menu_shell->priv->ignore_enter);

  /* Check to see if we are within an active submenu's navigation region
   */
  if (ctk_menu_navigating_submenu (menu, event->x_root, event->y_root))
    return TRUE;

  /* Make sure we pop down if we enter a non-selectable menu item, so we
   * don't show a submenu when the cursor is outside the stay-up triangle.
   */
  if (!_ctk_menu_item_is_selectable (menu_item))
    {
      /* We really want to deselect, but this gives the menushell code
       * a chance to do some bookkeeping about the menuitem.
       */
      ctk_menu_shell_select_item (menu_shell, menu_item);
      return FALSE;
    }

  if (need_enter)
    {
      /* The menu is now sensitive to enter events on its items, but
       * was previously sensitive.  So we fake an enter event.
       */
      menu_shell->priv->ignore_enter = FALSE;

      if (event->x >= 0 && event->x < cdk_window_get_width (event->window) &&
          event->y >= 0 && event->y < cdk_window_get_height (event->window))
        {
          CdkEvent *send_event = cdk_event_new (CDK_ENTER_NOTIFY);
          gboolean result;

          send_event->crossing.window = g_object_ref (event->window);
          send_event->crossing.time = event->time;
          send_event->crossing.send_event = TRUE;
          send_event->crossing.x_root = event->x_root;
          send_event->crossing.y_root = event->y_root;
          send_event->crossing.x = event->x;
          send_event->crossing.y = event->y;
          send_event->crossing.state = event->state;
          cdk_event_set_device (send_event, cdk_event_get_device ((CdkEvent *) event));

          /* We send the event to 'widget', the currently active menu,
           * instead of 'menu', the menu that the pointer is in. This
           * will ensure that the event will be ignored unless the
           * menuitem is a child of the active menu or some parent
           * menu of the active menu.
           */
          result = ctk_widget_event (widget, send_event);
          cdk_event_free (send_event);

          return result;
        }
    }

  return FALSE;
}

static void
ctk_menu_scroll_by (CtkMenu *menu,
                    gint     step)
{
  CtkMenuPrivate *priv = menu->priv;
  CtkBorder arrow_border;
  CtkWidget *widget;
  gint offset;
  gint view_height;

  widget = CTK_WIDGET (menu);
  offset = priv->scroll_offset + step;

  get_arrows_border (menu, &arrow_border);

  /* Don't scroll over the top if we weren't before: */
  if ((priv->scroll_offset >= 0) && (offset < 0))
    offset = 0;

  view_height = cdk_window_get_height (ctk_widget_get_window (widget));

  if (priv->scroll_offset == 0 &&
      view_height >= priv->requested_height)
    return;

  /* Don't scroll past the bottom if we weren't before: */
  if (priv->scroll_offset > 0)
    view_height -= arrow_border.top;

  /* Since arrows are shown, reduce view height even more */
  view_height -= arrow_border.bottom;

  if ((priv->scroll_offset + view_height <= priv->requested_height) &&
      (offset + view_height > priv->requested_height))
    offset = priv->requested_height - view_height;

  if (offset != priv->scroll_offset)
    ctk_menu_scroll_to (menu, offset, CTK_MENU_SCROLL_FLAG_NONE);
}

static gboolean
ctk_menu_scroll_timeout (gpointer data)
{
  CtkMenu  *menu;

  menu = CTK_MENU (data);
  ctk_menu_scroll_by (menu, menu->priv->scroll_step);

  return TRUE;
}

static gboolean
ctk_menu_scroll (CtkWidget      *widget,
                 CdkEventScroll *event)
{
  CtkMenu *menu = CTK_MENU (widget);

  if (cdk_event_get_pointer_emulated ((CdkEvent *) event))
    return CDK_EVENT_PROPAGATE;

  switch (event->direction)
    {
    case CDK_SCROLL_DOWN:
      ctk_menu_scroll_by (menu, MENU_SCROLL_STEP2);
      break;
    case CDK_SCROLL_UP:
      ctk_menu_scroll_by (menu, - MENU_SCROLL_STEP2);
      break;
    case CDK_SCROLL_SMOOTH:
      ctk_menu_scroll_by (menu, event->delta_y * MENU_SCROLL_STEP2);
      break;
    default:
      return CDK_EVENT_PROPAGATE;
      break;
    }

  return CDK_EVENT_STOP;
}

static void
get_arrows_sensitive_area (CtkMenu      *menu,
                           CdkRectangle *upper,
                           CdkRectangle *lower)
{
  CtkMenuPrivate *priv = menu->priv;
  CtkWidget *widget = CTK_WIDGET (menu);
  CdkWindow *window;
  gint width, height;
  guint border;
  gint win_x, win_y;
  CtkBorder padding;
  gint top_arrow_height, bottom_arrow_height;

  ctk_css_gadget_get_preferred_size (priv->top_arrow_gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     -1,
                                     &top_arrow_height, NULL,
                                     NULL, NULL);
  ctk_css_gadget_get_preferred_size (priv->bottom_arrow_gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     -1,
                                     &bottom_arrow_height, NULL,
                                     NULL, NULL);

  window = ctk_widget_get_window (widget);
  width = cdk_window_get_width (window);
  height = cdk_window_get_height (window);

  border = ctk_container_get_border_width (CTK_CONTAINER (menu));
  get_menu_padding (widget, &padding);

  cdk_window_get_position (window, &win_x, &win_y);

  if (upper)
    {
      upper->x = win_x;
      upper->y = win_y;
      upper->width = width;
      upper->height = top_arrow_height + border + padding.top;
    }

  if (lower)
    {
      lower->x = win_x;
      lower->y = win_y + height - border - padding.bottom - bottom_arrow_height;
      lower->width = width;
      lower->height = bottom_arrow_height + border + padding.bottom;
    }
}

static void
ctk_menu_handle_scrolling (CtkMenu *menu,
                           gint     x,
                           gint     y,
                           gboolean enter,
                           gboolean motion)
{
  CtkMenuPrivate *priv = menu->priv;
  CtkMenuShell *menu_shell;
  CdkRectangle rect;
  gboolean in_arrow;
  gboolean scroll_fast = FALSE;
  gint top_x, top_y;

  menu_shell = CTK_MENU_SHELL (menu);

  cdk_window_get_position (ctk_widget_get_window (priv->toplevel),
                           &top_x, &top_y);
  x -= top_x;
  y -= top_y;

  /*  upper arrow handling  */

  get_arrows_sensitive_area (menu, &rect, NULL);

  in_arrow = FALSE;
  if (priv->upper_arrow_visible && !priv->tearoff_active &&
      (x >= rect.x) && (x < rect.x + rect.width) &&
      (y >= rect.y) && (y < rect.y + rect.height))
    {
      in_arrow = TRUE;
    }

  if ((priv->upper_arrow_state & CTK_STATE_FLAG_INSENSITIVE) == 0)
    {
      gboolean arrow_pressed = FALSE;

      if (priv->upper_arrow_visible && !priv->tearoff_active)
        {
          scroll_fast = (y < rect.y + MENU_SCROLL_FAST_ZONE);

          if (enter && in_arrow &&
              (!priv->upper_arrow_prelight ||
               priv->scroll_fast != scroll_fast))
            {
              priv->upper_arrow_prelight = TRUE;
              priv->scroll_fast = scroll_fast;

              /* Deselect the active item so that
               * any submenus are popped down
               */
              ctk_menu_shell_deselect (menu_shell);

              ctk_menu_remove_scroll_timeout (menu);
              priv->scroll_step = scroll_fast
                                    ? -MENU_SCROLL_STEP2
                                    : -MENU_SCROLL_STEP1;

              priv->scroll_timeout =
                cdk_threads_add_timeout (scroll_fast
                                           ? MENU_SCROLL_TIMEOUT2
                                           : MENU_SCROLL_TIMEOUT1,
                                         ctk_menu_scroll_timeout, menu);
              g_source_set_name_by_id (priv->scroll_timeout, "[ctk+] ctk_menu_scroll_timeout");
            }
          else if (!enter && !in_arrow && priv->upper_arrow_prelight)
            {
              ctk_menu_stop_scrolling (menu);
            }
        }

      /*  check if the button isn't insensitive before
       *  changing it to something else.
       */
      if ((priv->upper_arrow_state & CTK_STATE_FLAG_INSENSITIVE) == 0)
        {
          CtkStateFlags arrow_state = 0;

          if (arrow_pressed)
            arrow_state |= CTK_STATE_FLAG_ACTIVE;

          if (priv->upper_arrow_prelight)
            arrow_state |= CTK_STATE_FLAG_PRELIGHT;

          if (arrow_state != priv->upper_arrow_state)
            {
              priv->upper_arrow_state = arrow_state;
              ctk_css_gadget_set_state (priv->top_arrow_gadget, arrow_state);

              cdk_window_invalidate_rect (ctk_widget_get_window (CTK_WIDGET (menu)),
                                          &rect, FALSE);
            }
        }
    }

  /*  lower arrow handling  */

  get_arrows_sensitive_area (menu, NULL, &rect);

  in_arrow = FALSE;
  if (priv->lower_arrow_visible && !priv->tearoff_active &&
      (x >= rect.x) && (x < rect.x + rect.width) &&
      (y >= rect.y) && (y < rect.y + rect.height))
    {
      in_arrow = TRUE;
    }

  if ((priv->lower_arrow_state & CTK_STATE_FLAG_INSENSITIVE) == 0)
    {
      gboolean arrow_pressed = FALSE;

      if (priv->lower_arrow_visible && !priv->tearoff_active)
        {
          scroll_fast = (y > rect.y + rect.height - MENU_SCROLL_FAST_ZONE);

          if (enter && in_arrow &&
              (!priv->lower_arrow_prelight ||
               priv->scroll_fast != scroll_fast))
            {
              priv->lower_arrow_prelight = TRUE;
              priv->scroll_fast = scroll_fast;

              /* Deselect the active item so that
               * any submenus are popped down
               */
              ctk_menu_shell_deselect (menu_shell);

              ctk_menu_remove_scroll_timeout (menu);
              priv->scroll_step = scroll_fast
                                    ? MENU_SCROLL_STEP2
                                    : MENU_SCROLL_STEP1;

              priv->scroll_timeout =
                cdk_threads_add_timeout (scroll_fast
                                           ? MENU_SCROLL_TIMEOUT2
                                           : MENU_SCROLL_TIMEOUT1,
                                         ctk_menu_scroll_timeout, menu);
              g_source_set_name_by_id (priv->scroll_timeout, "[ctk+] ctk_menu_scroll_timeout");
            }
          else if (!enter && !in_arrow && priv->lower_arrow_prelight)
            {
              ctk_menu_stop_scrolling (menu);
            }
        }

      /*  check if the button isn't insensitive before
       *  changing it to something else.
       */
      if ((priv->lower_arrow_state & CTK_STATE_FLAG_INSENSITIVE) == 0)
        {
          CtkStateFlags arrow_state = 0;

          if (arrow_pressed)
            arrow_state |= CTK_STATE_FLAG_ACTIVE;

          if (priv->lower_arrow_prelight)
            arrow_state |= CTK_STATE_FLAG_PRELIGHT;

          if (arrow_state != priv->lower_arrow_state)
            {
              priv->lower_arrow_state = arrow_state;
              ctk_css_gadget_set_state (priv->bottom_arrow_gadget, arrow_state);

              cdk_window_invalidate_rect (ctk_widget_get_window (CTK_WIDGET (menu)),
                                          &rect, FALSE);
            }
        }
    }
}

static gboolean
ctk_menu_enter_notify (CtkWidget        *widget,
                       CdkEventCrossing *event)
{
  CtkWidget *menu_item;
  CtkWidget *parent;
  CdkDevice *source_device;

  if (event->mode == CDK_CROSSING_CTK_GRAB ||
      event->mode == CDK_CROSSING_CTK_UNGRAB ||
      event->mode == CDK_CROSSING_STATE_CHANGED)
    return TRUE;

  source_device = cdk_event_get_source_device ((CdkEvent *) event);
  menu_item = ctk_get_event_widget ((CdkEvent*) event);

  if (CTK_IS_MENU (widget) &&
      cdk_device_get_source (source_device) != CDK_SOURCE_TOUCHSCREEN)
    {
      CtkMenuShell *menu_shell = CTK_MENU_SHELL (widget);

      if (!menu_shell->priv->ignore_enter)
        ctk_menu_handle_scrolling (CTK_MENU (widget),
                                   event->x_root, event->y_root, TRUE, TRUE);
    }

  if (cdk_device_get_source (source_device) != CDK_SOURCE_TOUCHSCREEN &&
      CTK_IS_MENU_ITEM (menu_item))
    {
      CtkWidget *menu = ctk_widget_get_parent (menu_item);

      if (CTK_IS_MENU (menu))
        {
          CtkMenuPrivate *priv = (CTK_MENU (menu))->priv;
          CtkMenuShell *menu_shell = CTK_MENU_SHELL (menu);

          if (priv->seen_item_enter)
            {
              /* This is the second enter we see for an item
               * on this menu. This means a release should always
               * mean activate.
               */
              menu_shell->priv->activate_time = 0;
            }
          else if ((event->detail != CDK_NOTIFY_NONLINEAR &&
                    event->detail != CDK_NOTIFY_NONLINEAR_VIRTUAL))
            {
              if (definitely_within_item (menu_item, event->x, event->y))
                {
                  /* This is an actual user-enter (ie. not a pop-under)
                   * In this case, the user must either have entered
                   * sufficiently far enough into the item, or he must move
                   * far enough away from the enter point. (see
                   * ctk_menu_motion_notify())
                   */
                  menu_shell->priv->activate_time = 0;
                }
            }

          priv->seen_item_enter = TRUE;
        }
    }

  /* If this is a faked enter (see ctk_menu_motion_notify), 'widget'
   * will not correspond to the event widget's parent.  Check to see
   * if we are in the parent's navigation region.
   */
  parent = ctk_widget_get_parent (menu_item);
  if (CTK_IS_MENU_ITEM (menu_item) && CTK_IS_MENU (parent) &&
      ctk_menu_navigating_submenu (CTK_MENU (parent),
                                   event->x_root, event->y_root))
    return TRUE;

  return CTK_WIDGET_CLASS (ctk_menu_parent_class)->enter_notify_event (widget, event);
}

static gboolean
ctk_menu_leave_notify (CtkWidget        *widget,
                       CdkEventCrossing *event)
{
  CtkMenuShell *menu_shell;
  CtkMenu *menu;
  CtkMenuItem *menu_item;
  CtkWidget *event_widget;
  CdkDevice *source_device;

  if (event->mode == CDK_CROSSING_CTK_GRAB ||
      event->mode == CDK_CROSSING_CTK_UNGRAB ||
      event->mode == CDK_CROSSING_STATE_CHANGED)
    return TRUE;

  menu = CTK_MENU (widget);
  menu_shell = CTK_MENU_SHELL (widget);

  if (ctk_menu_navigating_submenu (menu, event->x_root, event->y_root))
    return TRUE;

  source_device = cdk_event_get_source_device ((CdkEvent *) event);

  if (cdk_device_get_source (source_device) != CDK_SOURCE_TOUCHSCREEN)
    ctk_menu_handle_scrolling (menu, event->x_root, event->y_root, FALSE, TRUE);

  event_widget = ctk_get_event_widget ((CdkEvent*) event);

  if (!CTK_IS_MENU_ITEM (event_widget))
    return TRUE;

  menu_item = CTK_MENU_ITEM (event_widget);

  /* Here we check to see if we're leaving an active menu item
   * with a submenu, in which case we enter submenu navigation mode.
   */
  if (menu_shell->priv->active_menu_item != NULL
      && menu_item->priv->submenu != NULL
      && menu_item->priv->submenu_placement == CTK_LEFT_RIGHT)
    {
      if (CTK_MENU_SHELL (menu_item->priv->submenu)->priv->active)
        {
          ctk_menu_set_submenu_navigation_region (menu, menu_item, event);
          return TRUE;
        }
      else if (menu_item == CTK_MENU_ITEM (menu_shell->priv->active_menu_item))
        {
          /* We are leaving an active menu item with nonactive submenu.
           * Deselect it so we don't surprise the user with by popping
           * up a submenu _after_ he left the item.
           */
          ctk_menu_shell_deselect (menu_shell);
          return TRUE;
        }
    }

  return CTK_WIDGET_CLASS (ctk_menu_parent_class)->leave_notify_event (widget, event);
}

static gboolean
pointer_on_menu_widget (CtkMenu *menu,
                        gdouble  x_root,
                        gdouble  y_root)
{
  CtkMenuPrivate *priv = menu->priv;
  CtkAllocation allocation;
  gint window_x, window_y;

  ctk_widget_get_allocation (CTK_WIDGET (menu), &allocation);
  cdk_window_get_position (ctk_widget_get_window (priv->toplevel),
                           &window_x, &window_y);

  if (x_root >= window_x && x_root < window_x + allocation.width &&
      y_root >= window_y && y_root < window_y + allocation.height)
    return TRUE;

  return FALSE;
}

static gboolean
ctk_menu_captured_event (CtkWidget *widget,
                         CdkEvent  *event)
{
  CdkDevice *source_device;
  gboolean retval = FALSE;
  CtkMenuPrivate *priv;
  CtkMenu *menu;
  gdouble x_root, y_root;
  guint button;
  CdkModifierType state;

  menu = CTK_MENU (widget);
  priv = menu->priv;

  if (!priv->upper_arrow_visible && !priv->lower_arrow_visible && priv->drag_start_y < 0)
    return retval;

  source_device = cdk_event_get_source_device (event);
  cdk_event_get_root_coords (event, &x_root, &y_root);

  switch (event->type)
    {
    case CDK_TOUCH_BEGIN:
    case CDK_BUTTON_PRESS:
      if ((!cdk_event_get_button (event, &button) || button == 1) &&
          cdk_device_get_source (source_device) == CDK_SOURCE_TOUCHSCREEN &&
          pointer_on_menu_widget (menu, x_root, y_root))
        {
          priv->drag_start_y = event->button.y_root;
          priv->initial_drag_offset = priv->scroll_offset;
          priv->drag_scroll_started = FALSE;
        }
      else
        priv->drag_start_y = -1;

      priv->drag_already_pressed = TRUE;
      break;
    case CDK_TOUCH_END:
    case CDK_BUTTON_RELEASE:
      if (priv->drag_scroll_started)
        {
          priv->drag_scroll_started = FALSE;
          priv->drag_start_y = -1;
          priv->drag_already_pressed = FALSE;
          retval = TRUE;
        }
      break;
    case CDK_TOUCH_UPDATE:
    case CDK_MOTION_NOTIFY:
      if ((!cdk_event_get_state (event, &state) || (state & CDK_BUTTON1_MASK) != 0) &&
          cdk_device_get_source (source_device) == CDK_SOURCE_TOUCHSCREEN)
        {
          if (!priv->drag_already_pressed)
            {
              if (pointer_on_menu_widget (menu, x_root, y_root))
                {
                  priv->drag_start_y = y_root;
                  priv->initial_drag_offset = priv->scroll_offset;
                  priv->drag_scroll_started = FALSE;
                }
              else
                priv->drag_start_y = -1;

              priv->drag_already_pressed = TRUE;
            }

          if (priv->drag_start_y < 0 && !priv->drag_scroll_started)
            break;

          if (priv->drag_scroll_started)
            {
              gint offset, view_height;
              CtkBorder arrow_border;
              gdouble y_diff;

              y_diff = y_root - priv->drag_start_y;
              offset = priv->initial_drag_offset - y_diff;

              view_height = cdk_window_get_height (ctk_widget_get_window (widget));
              get_arrows_border (menu, &arrow_border);

              if (priv->upper_arrow_visible)
                view_height -= arrow_border.top;

              if (priv->lower_arrow_visible)
                view_height -= arrow_border.bottom;

              offset = CLAMP (offset,
                              MIN (priv->scroll_offset, 0),
                              MAX (priv->scroll_offset, priv->requested_height - view_height));

              ctk_menu_scroll_to (menu, offset, CTK_MENU_SCROLL_FLAG_NONE);

              retval = TRUE;
            }
          else if (ctk_drag_check_threshold (widget,
                                             0, priv->drag_start_y,
                                             0, y_root))
            {
              priv->drag_scroll_started = TRUE;
              ctk_menu_shell_deselect (CTK_MENU_SHELL (menu));
              retval = TRUE;
            }
        }
      break;
    case CDK_ENTER_NOTIFY:
    case CDK_LEAVE_NOTIFY:
      if (priv->drag_scroll_started)
        retval = TRUE;
      break;
    default:
      break;
    }

  return retval;
}

static void
ctk_menu_stop_navigating_submenu (CtkMenu *menu)
{
  CtkMenuPrivate *priv = menu->priv;

  priv->navigation_x = 0;
  priv->navigation_y = 0;
  priv->navigation_width = 0;
  priv->navigation_height = 0;

  if (priv->navigation_timeout)
    {
      g_source_remove (priv->navigation_timeout);
      priv->navigation_timeout = 0;
    }
}

/* When the timeout is elapsed, the navigation region is destroyed
 * and the menuitem under the pointer (if any) is selected.
 */
static gboolean
ctk_menu_stop_navigating_submenu_cb (gpointer user_data)
{
  CtkMenuPopdownData *popdown_data = user_data;
  CtkMenu *menu = popdown_data->menu;
  CtkMenuPrivate *priv = menu->priv;
  CdkWindow *child_window;

  ctk_menu_stop_navigating_submenu (menu);

  if (ctk_widget_get_realized (CTK_WIDGET (menu)))
    {
      child_window = cdk_window_get_device_position (priv->bin_window,
                                                     popdown_data->device,
                                                     NULL, NULL, NULL);

      if (child_window)
        {
          CdkEvent *send_event = cdk_event_new (CDK_ENTER_NOTIFY);

          send_event->crossing.window = g_object_ref (child_window);
          send_event->crossing.time = CDK_CURRENT_TIME; /* Bogus */
          send_event->crossing.send_event = TRUE;
          cdk_event_set_device (send_event, popdown_data->device);

          CTK_WIDGET_CLASS (ctk_menu_parent_class)->enter_notify_event (CTK_WIDGET (menu), (CdkEventCrossing *)send_event);

          cdk_event_free (send_event);
        }
    }

  return FALSE;
}

static gboolean
ctk_menu_navigating_submenu (CtkMenu *menu,
                             gint     event_x,
                             gint     event_y)
{
  CtkMenuPrivate *priv = menu->priv;
  gint width, height;

  if (!ctk_menu_has_navigation_triangle (menu))
    return FALSE;

  width = priv->navigation_width;
  height = priv->navigation_height;

  /* Check if x/y are in the triangle spanned by the navigation parameters */

  /* 1) Move the coordinates so the triangle starts at 0,0 */
  event_x -= priv->navigation_x;
  event_y -= priv->navigation_y;

  /* 2) Ensure both legs move along the positive axis */
  if (width < 0)
    {
      event_x = -event_x;
      width = -width;
    }
  if (height < 0)
    {
      event_y = -event_y;
      height = -height;
    }

  /* 3) Check that the given coordinate is inside the triangle. The formula
   * is a transformed form of this formula: x/w + y/h <= 1
   */
  if (event_x >= 0 && event_y >= 0 &&
      event_x * height + event_y * width <= width * height)
    {
      return TRUE;
    }
  else
    {
      ctk_menu_stop_navigating_submenu (menu);
      return FALSE;
    }
}

static void
ctk_menu_set_submenu_navigation_region (CtkMenu          *menu,
                                        CtkMenuItem      *menu_item,
                                        CdkEventCrossing *event)
{
  CtkMenuPrivate *priv = menu->priv;
  CtkWidget *event_widget;
  CdkWindow *window;
  int submenu_left;
  int submenu_right;
  int submenu_top;
  int submenu_bottom;
  int width;

  g_return_if_fail (menu_item->priv->submenu != NULL);
  g_return_if_fail (event != NULL);

  event_widget = ctk_get_event_widget ((CdkEvent*) event);

  window = ctk_widget_get_window (menu_item->priv->submenu);
  cdk_window_get_origin (window, &submenu_left, &submenu_top);

  submenu_right = submenu_left + cdk_window_get_width (window);
  submenu_bottom = submenu_top + cdk_window_get_height (window);

  width = cdk_window_get_width (ctk_widget_get_window (event_widget));

  if (event->x >= 0 && event->x < width)
    {
      CtkMenuPopdownData *popdown_data;
      /* The calculations below assume floored coordinates */
      int x_root = floor (event->x_root);
      int y_root = floor (event->y_root);

      ctk_menu_stop_navigating_submenu (menu);

      /* The navigation region is the triangle closest to the x/y
       * location of the rectangle. This is why the width or height
       * can be negative.
       */
      if (menu_item->priv->submenu_direction == CTK_DIRECTION_RIGHT)
        {
          /* right */
          priv->navigation_x = submenu_left;
          priv->navigation_width = x_root - submenu_left;
        }
      else
        {
          /* left */
          priv->navigation_x = submenu_right;
          priv->navigation_width = x_root - submenu_right;
        }

      if (event->y < 0)
        {
          /* top */
          priv->navigation_y = y_root;
          priv->navigation_height = submenu_top - y_root - NAVIGATION_REGION_OVERSHOOT;

          if (priv->navigation_height >= 0)
            return;
        }
      else
        {
          /* bottom */
          priv->navigation_y = y_root;
          priv->navigation_height = submenu_bottom - y_root + NAVIGATION_REGION_OVERSHOOT;

          if (priv->navigation_height <= 0)
            return;
        }

      popdown_data = g_new (CtkMenuPopdownData, 1);
      popdown_data->menu = menu;
      popdown_data->device = cdk_event_get_device ((CdkEvent *) event);

      priv->navigation_timeout = cdk_threads_add_timeout_full (G_PRIORITY_DEFAULT,
                                                               MENU_POPDOWN_DELAY,
                                                               ctk_menu_stop_navigating_submenu_cb,
                                                               popdown_data,
                                                               (GDestroyNotify) g_free);
      g_source_set_name_by_id (priv->navigation_timeout, "[ctk+] ctk_menu_stop_navigating_submenu_cb");
    }
}

static void
ctk_menu_deactivate (CtkMenuShell *menu_shell)
{
  CtkWidget *parent;

  g_return_if_fail (CTK_IS_MENU (menu_shell));

  parent = menu_shell->priv->parent_menu_shell;

  menu_shell->priv->activate_time = 0;
  ctk_menu_popdown (CTK_MENU (menu_shell));

  if (parent)
    ctk_menu_shell_deactivate (CTK_MENU_SHELL (parent));
}

static void
ctk_menu_position_legacy (CtkMenu  *menu,
                          gboolean  set_scroll_offset)
{
  CtkMenuPrivate *priv = menu->priv;
  CtkWidget *widget;
  CtkRequisition requisition;
  gint x, y;
  gint scroll_offset;
  CdkDisplay *display;
  CdkMonitor *monitor;
  CdkRectangle workarea;
  gint monitor_num;
  CdkDevice *pointer;
  CtkBorder border;
  gint i;

  widget = CTK_WIDGET (menu);

  display = ctk_widget_get_display (widget);
  pointer = _ctk_menu_shell_get_grab_device (CTK_MENU_SHELL (menu));
  cdk_device_get_position (pointer, NULL, &x, &y);

  /* Realize so we have the proper width and height to figure out
   * the right place to popup the menu.
   */
  ctk_widget_realize (priv->toplevel);

  _ctk_window_get_shadow_width (CTK_WINDOW (priv->toplevel), &border);

  requisition.width = ctk_widget_get_allocated_width (widget);
  requisition.height = ctk_widget_get_allocated_height (widget);

  monitor = cdk_display_get_monitor_at_point (display, x, y);
  monitor_num = 0;
  for (i = 0; ; i++)
    {
      CdkMonitor *m = cdk_display_get_monitor (display, i);

      if (m == monitor)
        {
          monitor_num = i;
          break;
        }
      if (m == NULL)
        break;
    }

  priv->monitor_num = monitor_num;
  priv->initially_pushed_in = FALSE;

  /* Set the type hint here to allow custom position functions
   * to set a different hint
   */
  if (!ctk_widget_get_visible (priv->toplevel))
    ctk_window_set_type_hint (CTK_WINDOW (priv->toplevel), CDK_WINDOW_TYPE_HINT_POPUP_MENU);

  if (priv->position_func)
    {
      (* priv->position_func) (menu, &x, &y, &priv->initially_pushed_in,
                               priv->position_func_data);

      if (priv->monitor_num < 0)
        priv->monitor_num = monitor_num;

      monitor = cdk_display_get_monitor (display, priv->monitor_num);
      cdk_monitor_get_workarea (monitor, &workarea);
    }
  else
    {
      gint space_left, space_right, space_above, space_below;
      gint needed_width;
      gint needed_height;
      CtkBorder padding;
      CtkBorder margin;
      gboolean rtl = (ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL);

      get_menu_padding (widget, &padding);
      get_menu_margin (widget, &margin);

      /* The placement of popup menus horizontally works like this (with
       * RTL in parentheses)
       *
       * - If there is enough room to the right (left) of the mouse cursor,
       *   position the menu there.
       *
       * - Otherwise, if if there is enough room to the left (right) of the
       *   mouse cursor, position the menu there.
       *
       * - Otherwise if the menu is smaller than the monitor, position it
       *   on the side of the mouse cursor that has the most space available
       *
       * - Otherwise (if there is simply not enough room for the menu on the
       *   monitor), position it as far left (right) as possible.
       *
       * Positioning in the vertical direction is similar: first try below
       * mouse cursor, then above.
       */
      monitor = cdk_display_get_monitor (display, priv->monitor_num);
      cdk_monitor_get_workarea (monitor, &workarea);

      space_left = x - workarea.x;
      space_right = workarea.x + workarea.width - x - 1;
      space_above = y - workarea.y;
      space_below = workarea.y + workarea.height - y - 1;

      /* Position horizontally. */

      /* the amount of space we need to position the menu.
       * Note the menu is offset "thickness" pixels
       */
      needed_width = requisition.width - padding.left;

      if (needed_width <= space_left ||
          needed_width <= space_right)
        {
          if ((rtl  && needed_width <= space_left) ||
              (!rtl && needed_width >  space_right))
            {
              /* position left */
              x = x - margin.left + padding.left - requisition.width + 1;
            }
          else
            {
              /* position right */
              x = x + margin.right - padding.right;
            }

          /* x is clamped on-screen further down */
        }
      else if (requisition.width <= workarea.width)
        {
          /* the menu is too big to fit on either side of the mouse
           * cursor, but smaller than the monitor. Position it on
           * the side that has the most space
           */
          if (space_left > space_right)
            {
              /* left justify */
              x = workarea.x;
            }
          else
            {
              /* right justify */
              x = workarea.x + workarea.width - requisition.width;
            }
        }
      else /* menu is simply too big for the monitor */
        {
          if (rtl)
            {
              /* right justify */
              x = workarea.x + workarea.width - requisition.width;
            }
          else
            {
              /* left justify */
              x = workarea.x;
            }
        }

      /* Position vertically.
       * The algorithm is the same as above, but simpler
       * because we don't have to take RTL into account.
       */
      needed_height = requisition.height - padding.top;

      if (needed_height <= space_above ||
          needed_height <= space_below)
        {
          if (needed_height <= space_below)
            y = y + margin.top - padding.top;
          else
            y = y - margin.bottom + padding.bottom - requisition.height + 1;

          y = CLAMP (y, workarea.y,
                     workarea.y + workarea.height - requisition.height);
        }
      else if (needed_height > space_below && needed_height > space_above)
        {
          if (space_below >= space_above)
            y = workarea.y + workarea.height - requisition.height;
          else
            y = workarea.y;
        }
      else
        {
          y = workarea.y;
        }
    }

  scroll_offset = 0;

  if (y + requisition.height > workarea.y + workarea.height)
    {
      if (priv->initially_pushed_in)
        scroll_offset += (workarea.y + workarea.height) - requisition.height - y;
      y = (workarea.y + workarea.height) - requisition.height;
    }

  if (y < workarea.y)
    {
      if (priv->initially_pushed_in)
        scroll_offset += workarea.y - y;
      y = workarea.y;
    }

  x = CLAMP (x, workarea.x, MAX (workarea.x, workarea.x + workarea.width - requisition.width));

  x -= border.left;
  y -= border.top;

  if (CTK_MENU_SHELL (menu)->priv->active)
    {
      priv->have_position = TRUE;
      priv->position_x = x;
      priv->position_y = y;
    }

  if (scroll_offset != 0)
    {
      CtkBorder arrow_border;

      get_arrows_border (menu, &arrow_border);
      scroll_offset += arrow_border.top;
    }

  ctk_window_move (CTK_WINDOW (CTK_MENU_SHELL (menu)->priv->active ? priv->toplevel : priv->tearoff_window),
                   x, y);

  if (!CTK_MENU_SHELL (menu)->priv->active)
    {
      ctk_window_resize (CTK_WINDOW (priv->tearoff_window),
                         requisition.width, requisition.height);
    }

  if (set_scroll_offset)
    priv->scroll_offset = scroll_offset;
}

static CdkGravity
get_horizontally_flipped_anchor (CdkGravity anchor)
{
  switch (anchor)
    {
    case CDK_GRAVITY_STATIC:
    case CDK_GRAVITY_NORTH_WEST:
      return CDK_GRAVITY_NORTH_EAST;
    case CDK_GRAVITY_NORTH:
      return CDK_GRAVITY_NORTH;
    case CDK_GRAVITY_NORTH_EAST:
      return CDK_GRAVITY_NORTH_WEST;
    case CDK_GRAVITY_WEST:
      return CDK_GRAVITY_EAST;
    case CDK_GRAVITY_CENTER:
      return CDK_GRAVITY_CENTER;
    case CDK_GRAVITY_EAST:
      return CDK_GRAVITY_WEST;
    case CDK_GRAVITY_SOUTH_WEST:
      return CDK_GRAVITY_SOUTH_EAST;
    case CDK_GRAVITY_SOUTH:
      return CDK_GRAVITY_SOUTH;
    case CDK_GRAVITY_SOUTH_EAST:
      return CDK_GRAVITY_SOUTH_WEST;
    }

  g_warning ("unknown CdkGravity: %d", anchor);
  return anchor;
}

static void
ctk_menu_position (CtkMenu  *menu,
                   gboolean  set_scroll_offset)
{
  CtkMenuPrivate *priv = menu->priv;
  CdkWindow *rect_window = NULL;
  CdkRectangle rect;
  CtkTextDirection text_direction = CTK_TEXT_DIR_NONE;
  CdkGravity rect_anchor;
  CdkGravity menu_anchor;
  CdkAnchorHints anchor_hints;
  gint rect_anchor_dx, rect_anchor_dy;
  CdkWindow *toplevel;
  gboolean emulated_move_to_rect = FALSE;

  rect_anchor = priv->rect_anchor;
  menu_anchor = priv->menu_anchor;
  anchor_hints = priv->anchor_hints;
  rect_anchor_dx = priv->rect_anchor_dx;
  rect_anchor_dy = priv->rect_anchor_dy;

  if (priv->rect_window)
    {
      rect_window = priv->rect_window;
      rect = priv->rect;
    }
  else if (priv->widget)
    {
      rect_window = ctk_widget_get_window (priv->widget);
      ctk_widget_get_allocation (priv->widget, &rect);
      text_direction = ctk_widget_get_direction (priv->widget);
    }
  else if (!priv->position_func)
    {
      CtkWidget *attach_widget;
      CdkDevice *grab_device;

      /*
       * One of the legacy ctk_menu_popup*() functions were used to popup but
       * without a custom positioning function, so make an attempt to let the
       * backend do the position constraining when required conditions are met.
       */

      grab_device = _ctk_menu_shell_get_grab_device (CTK_MENU_SHELL (menu));
      attach_widget = ctk_menu_get_attach_widget (menu);

      if (grab_device && attach_widget)
        {
          rect.x = 0;
          rect.y = 0;
          rect.width = 1;
          rect.height = 1;

          rect_window = ctk_widget_get_window (attach_widget);
          cdk_window_get_device_position (rect_window, grab_device,
                                          &rect.x, &rect.y, NULL);
          text_direction = ctk_widget_get_direction (attach_widget);
          rect_anchor = CDK_GRAVITY_SOUTH_EAST;
          menu_anchor = CDK_GRAVITY_NORTH_WEST;
          anchor_hints = CDK_ANCHOR_FLIP | CDK_ANCHOR_SLIDE | CDK_ANCHOR_RESIZE;
          rect_anchor_dx = 0;
          rect_anchor_dy = 0;
          emulated_move_to_rect = TRUE;
        }
    }

  if (!rect_window)
    {
      ctk_window_set_unlimited_guessed_size (CTK_WINDOW (priv->toplevel),
                                             FALSE, FALSE);
      ctk_menu_position_legacy (menu, set_scroll_offset);
      return;
    }

  ctk_window_set_unlimited_guessed_size (CTK_WINDOW (priv->toplevel),
                                         !!(anchor_hints & CDK_ANCHOR_RESIZE_X),
                                         !!(anchor_hints & CDK_ANCHOR_RESIZE_Y));

  if (!ctk_widget_get_visible (priv->toplevel))
    ctk_window_set_type_hint (CTK_WINDOW (priv->toplevel), priv->menu_type_hint);

  /* Realize so we have the proper width and height to figure out
   * the right place to popup the menu.
   */
  ctk_widget_realize (priv->toplevel);
  ctk_window_move_resize (CTK_WINDOW (priv->toplevel));

  if (text_direction == CTK_TEXT_DIR_NONE)
    text_direction = ctk_widget_get_direction (CTK_WIDGET (menu));

  if (text_direction == CTK_TEXT_DIR_RTL)
    {
      rect_anchor = get_horizontally_flipped_anchor (rect_anchor);
      menu_anchor = get_horizontally_flipped_anchor (menu_anchor);
    }

  toplevel = ctk_widget_get_window (priv->toplevel);

  cdk_window_set_transient_for (toplevel, rect_window);

  g_signal_handlers_disconnect_by_func (toplevel, moved_to_rect_cb, menu);

  g_signal_connect (toplevel, "moved-to-rect", G_CALLBACK (moved_to_rect_cb),
                    menu);
  priv->emulated_move_to_rect = emulated_move_to_rect;

  cdk_window_move_to_rect (toplevel,
                           &rect,
                           rect_anchor,
                           menu_anchor,
                           anchor_hints,
                           rect_anchor_dx,
                           rect_anchor_dy);
}

static void
ctk_menu_remove_scroll_timeout (CtkMenu *menu)
{
  CtkMenuPrivate *priv = menu->priv;

  if (priv->scroll_timeout)
    {
      g_source_remove (priv->scroll_timeout);
      priv->scroll_timeout = 0;
    }
}

static void
ctk_menu_stop_scrolling (CtkMenu *menu)
{
  CtkMenuPrivate *priv = menu->priv;
  CtkCssNode *top_arrow_node, *bottom_arrow_node;
  CtkStateFlags state;

  ctk_menu_remove_scroll_timeout (menu);
  priv->upper_arrow_prelight = FALSE;
  priv->lower_arrow_prelight = FALSE;

  top_arrow_node = ctk_css_gadget_get_node (priv->top_arrow_gadget);
  state = ctk_css_node_get_state (top_arrow_node);
  ctk_css_node_set_state (top_arrow_node, state & ~CTK_STATE_FLAG_PRELIGHT);

  bottom_arrow_node = ctk_css_gadget_get_node (priv->bottom_arrow_gadget);
  state = ctk_css_node_get_state (bottom_arrow_node);
  ctk_css_node_set_state (bottom_arrow_node, state & ~CTK_STATE_FLAG_PRELIGHT);
}

static void
sync_arrows_state (CtkMenu *menu)
{
  CtkMenuPrivate *priv = menu->priv;
  CtkCssNode *top_arrow_node, *bottom_arrow_node;

  top_arrow_node = ctk_css_gadget_get_node (priv->top_arrow_gadget);
  ctk_css_node_set_visible (top_arrow_node, priv->upper_arrow_visible);
  ctk_css_node_set_state (top_arrow_node, priv->upper_arrow_state);

  bottom_arrow_node = ctk_css_gadget_get_node (priv->bottom_arrow_gadget);
  ctk_css_node_set_visible (bottom_arrow_node, priv->lower_arrow_visible);
  ctk_css_node_set_state (bottom_arrow_node, priv->lower_arrow_state);
}

static void
ctk_menu_scroll_to (CtkMenu           *menu,
                    gint               offset,
                    CtkMenuScrollFlag  flags)
{
  CtkMenuPrivate *priv = menu->priv;
  CtkBorder arrow_border, padding;
  CtkWidget *widget;
  gint x, y;
  gint view_width, view_height;
  gint border_width;
  gint menu_height;

  widget = CTK_WIDGET (menu);

  if (priv->tearoff_active && priv->tearoff_adjustment)
    ctk_adjustment_set_value (priv->tearoff_adjustment, offset);

  /* Move/resize the viewport according to arrows: */
  view_width = ctk_widget_get_allocated_width (widget);
  view_height = ctk_widget_get_allocated_height (widget);

  get_menu_padding (widget, &padding);

  border_width = ctk_container_get_border_width (CTK_CONTAINER (menu));

  view_width -= (2 * border_width) + padding.left + padding.right;
  view_height -= (2 * border_width) + padding.top + padding.bottom;
  menu_height = priv->requested_height - (2 * border_width) - padding.top - padding.bottom;

  x = border_width + padding.left;
  y = border_width + padding.top;

  if (!priv->tearoff_active)
    {
      if (view_height < menu_height               ||
          (offset > 0 && priv->scroll_offset > 0) ||
          (offset < 0 && priv->scroll_offset < 0))
        {
          CtkStateFlags upper_arrow_previous_state = priv->upper_arrow_state;
          CtkStateFlags lower_arrow_previous_state = priv->lower_arrow_state;
          gboolean should_offset_by_arrow;

          if (!priv->upper_arrow_visible || !priv->lower_arrow_visible)
            ctk_widget_queue_draw (CTK_WIDGET (menu));

          if (!priv->upper_arrow_visible &&
              flags & CTK_MENU_SCROLL_FLAG_ADAPT)
            should_offset_by_arrow = TRUE;
          else
            should_offset_by_arrow = FALSE;

          priv->upper_arrow_visible = priv->lower_arrow_visible = TRUE;

          if (flags & CTK_MENU_SCROLL_FLAG_ADAPT)
            sync_arrows_state (menu);

          get_arrows_border (menu, &arrow_border);
          if (should_offset_by_arrow)
            offset += arrow_border.top;
          y += arrow_border.top;
          view_height -= arrow_border.top;
          view_height -= arrow_border.bottom;

          if (offset <= 0)
            priv->upper_arrow_state |= CTK_STATE_FLAG_INSENSITIVE;
          else
            {
              priv->upper_arrow_state &= ~(CTK_STATE_FLAG_INSENSITIVE);

              if (priv->upper_arrow_prelight)
                priv->upper_arrow_state |= CTK_STATE_FLAG_PRELIGHT;
              else
                priv->upper_arrow_state &= ~(CTK_STATE_FLAG_PRELIGHT);
            }

          if (offset >= menu_height - view_height)
            priv->lower_arrow_state |= CTK_STATE_FLAG_INSENSITIVE;
          else
            {
              priv->lower_arrow_state &= ~(CTK_STATE_FLAG_INSENSITIVE);

              if (priv->lower_arrow_prelight)
                priv->lower_arrow_state |= CTK_STATE_FLAG_PRELIGHT;
              else
                priv->lower_arrow_state &= ~(CTK_STATE_FLAG_PRELIGHT);
            }

          if ((priv->upper_arrow_state != upper_arrow_previous_state) ||
              (priv->lower_arrow_state != lower_arrow_previous_state))
            ctk_widget_queue_draw (CTK_WIDGET (menu));

          if ((upper_arrow_previous_state & CTK_STATE_FLAG_INSENSITIVE) == 0 &&
              (priv->upper_arrow_state & CTK_STATE_FLAG_INSENSITIVE) != 0)
            {
              /* At the upper border, possibly remove timeout */
              if (priv->scroll_step < 0)
                {
                  ctk_menu_stop_scrolling (menu);
                  ctk_widget_queue_draw (CTK_WIDGET (menu));
                }
            }

          if ((lower_arrow_previous_state & CTK_STATE_FLAG_INSENSITIVE) == 0 &&
              (priv->lower_arrow_state & CTK_STATE_FLAG_INSENSITIVE) != 0)
            {
              /* At the lower border, possibly remove timeout */
              if (priv->scroll_step > 0)
                {
                  ctk_menu_stop_scrolling (menu);
                  ctk_widget_queue_draw (CTK_WIDGET (menu));
                }
            }
        }
      else if (priv->upper_arrow_visible || priv->lower_arrow_visible)
        {
          offset = 0;

          priv->upper_arrow_visible = priv->lower_arrow_visible = FALSE;
          priv->upper_arrow_prelight = priv->lower_arrow_prelight = FALSE;

          ctk_menu_stop_scrolling (menu);
          ctk_widget_queue_draw (CTK_WIDGET (menu));
        }
    }

  sync_arrows_state (menu);

  /* Scroll the menu: */
  if (ctk_widget_get_realized (widget))
    {
      cdk_window_move (priv->bin_window, 0, -offset);
      cdk_window_move_resize (priv->view_window, x, y, view_width, view_height);
    }

  priv->scroll_offset = offset;
}

static gboolean
compute_child_offset (CtkMenu   *menu,
                      CtkWidget *menu_item,
                      gint      *offset,
                      gint      *height,
                      gboolean  *is_last_child)
{
  CtkMenuPrivate *priv = menu->priv;
  gint item_top_attach;
  gint item_bottom_attach;
  gint child_offset = 0;
  gint i;

  get_effective_child_attach (menu_item, NULL, NULL,
                              &item_top_attach, &item_bottom_attach);

  /* there is a possibility that we get called before _size_request,
   * so check the height table for safety.
   */
  if (!priv->heights || priv->heights_length < ctk_menu_get_n_rows (menu))
    return FALSE;

  /* when we have a row with only invisible children, its height will
   * be zero, so there's no need to check WIDGET_VISIBLE here
   */
  for (i = 0; i < item_top_attach; i++)
    child_offset += priv->heights[i];

  if (is_last_child)
    *is_last_child = (item_bottom_attach == ctk_menu_get_n_rows (menu));
  if (offset)
    *offset = child_offset;
  if (height)
    *height = priv->heights[item_top_attach];

  return TRUE;
}

static void
ctk_menu_scroll_item_visible (CtkMenuShell *menu_shell,
                              CtkWidget    *menu_item)
{
  CtkMenu *menu = CTK_MENU (menu_shell);
  CtkMenuPrivate *priv = menu->priv;
  CtkWidget *widget = CTK_WIDGET (menu_shell);
  gint child_offset, child_height;
  gint height;
  gint y;
  gint arrow_height;
  gboolean last_child = 0;

  /* We need to check if the selected item fully visible.
   * If not we need to scroll the menu so that it becomes fully
   * visible.
   */
  if (compute_child_offset (menu, menu_item,
                            &child_offset, &child_height, &last_child))
    {
      CtkBorder padding;

      y = priv->scroll_offset;
      height = cdk_window_get_height (ctk_widget_get_window (widget));

      get_menu_padding (widget, &padding);

      height -= 2 * ctk_container_get_border_width (CTK_CONTAINER (menu)) +
        padding.top + padding.bottom;

      if (child_offset < y)
        {
          /* Ignore the enter event we might get if the pointer
           * is on the menu
           */
          menu_shell->priv->ignore_enter = TRUE;
          ctk_menu_scroll_to (menu, child_offset, CTK_MENU_SCROLL_FLAG_NONE);
        }
      else
        {
          CtkBorder arrow_border;

          arrow_height = 0;

          get_arrows_border (menu, &arrow_border);
          if (!priv->tearoff_active)
            arrow_height = arrow_border.top + arrow_border.bottom;

          if (child_offset + child_height > y + height - arrow_height)
            {
              arrow_height = arrow_border.bottom + arrow_border.top;
              y = child_offset + child_height - height + arrow_height;

              /* Ignore the enter event we might get if the pointer
               * is on the menu
               */
              menu_shell->priv->ignore_enter = TRUE;
              ctk_menu_scroll_to (menu, y, CTK_MENU_SCROLL_FLAG_NONE);
            }
        }
    }
}

static void
ctk_menu_select_item (CtkMenuShell *menu_shell,
                      CtkWidget    *menu_item)
{
  CtkMenu *menu = CTK_MENU (menu_shell);

  if (ctk_widget_get_realized (CTK_WIDGET (menu)))
    ctk_menu_scroll_item_visible (menu_shell, menu_item);

  CTK_MENU_SHELL_CLASS (ctk_menu_parent_class)->select_item (menu_shell, menu_item);
}


/* Reparent the menu, taking care of the refcounting
 *
 * If unrealize is true we force a unrealize while reparenting the parent.
 * This can help eliminate flicker in some cases.
 *
 * What happens is that when the menu is unrealized and then re-realized,
 * the allocations are as follows:
 *
 *  parent - 1x1 at (0,0)
 *  child1 - 100x20 at (0,0)
 *  child2 - 100x20 at (0,20)
 *  child3 - 100x20 at (0,40)
 *
 * That is, the parent is small but the children are full sized. Then,
 * when the queued_resize gets processed, the parent gets resized to
 * full size.
 *
 * But in order to eliminate flicker when scrolling, cdkgeometry-x11.c
 * contains the following logic:
 *
 * - if a move or resize operation on a window would change the clip
 *   region on the children, then before the window is resized
 *   the background for children is temporarily set to None, the
 *   move/resize done, and the background for the children restored.
 *
 * So, at the point where the parent is resized to final size, the
 * background for the children is temporarily None, and thus they
 * are not cleared to the background color and the previous background
 * (the image of the menu) is left in place.
 */
static void
ctk_menu_reparent (CtkMenu   *menu,
                   CtkWidget *new_parent,
                   gboolean   unrealize)
{
  GObject *object = G_OBJECT (menu);
  CtkWidget *widget = CTK_WIDGET (menu);
  gboolean was_floating = g_object_is_floating (object);

  g_object_ref_sink (object);

  if (unrealize)
    {
      g_object_ref (object);
      ctk_container_remove (CTK_CONTAINER (ctk_widget_get_parent (widget)), widget);
      ctk_container_add (CTK_CONTAINER (new_parent), widget);
      g_object_unref (object);
    }
  else
    {
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      ctk_widget_reparent (widget, new_parent);
      G_GNUC_END_IGNORE_DEPRECATIONS;
    }

  if (was_floating)
    g_object_force_floating (object);
  else
    g_object_unref (object);
}

static void
ctk_menu_show_all (CtkWidget *widget)
{
  /* Show children, but not self. */
  ctk_container_foreach (CTK_CONTAINER (widget), (CtkCallback) ctk_widget_show_all, NULL);
}

/**
 * ctk_menu_set_screen:
 * @menu: a #CtkMenu
 * @screen: (allow-none): a #CdkScreen, or %NULL if the screen should be
 *          determined by the widget the menu is attached to
 *
 * Sets the #CdkScreen on which the menu will be displayed.
 *
 * Since: 2.2
 */
void
ctk_menu_set_screen (CtkMenu   *menu,
                     CdkScreen *screen)
{
  g_return_if_fail (CTK_IS_MENU (menu));
  g_return_if_fail (screen == NULL || CDK_IS_SCREEN (screen));

  g_object_set_data (G_OBJECT (menu), I_("ctk-menu-explicit-screen"), screen);

  if (screen)
    {
      menu_change_screen (menu, screen);
    }
  else
    {
      CtkWidget *attach_widget = ctk_menu_get_attach_widget (menu);
      if (attach_widget)
        attach_widget_screen_changed (attach_widget, NULL, menu);
    }
}

/**
 * ctk_menu_attach:
 * @menu: a #CtkMenu
 * @child: a #CtkMenuItem
 * @left_attach: The column number to attach the left side of the item to
 * @right_attach: The column number to attach the right side of the item to
 * @top_attach: The row number to attach the top of the item to
 * @bottom_attach: The row number to attach the bottom of the item to
 *
 * Adds a new #CtkMenuItem to a (table) menu. The number of “cells” that
 * an item will occupy is specified by @left_attach, @right_attach,
 * @top_attach and @bottom_attach. These each represent the leftmost,
 * rightmost, uppermost and lower column and row numbers of the table.
 * (Columns and rows are indexed from zero).
 *
 * Note that this function is not related to ctk_menu_detach().
 *
 * Since: 2.4
 */
void
ctk_menu_attach (CtkMenu   *menu,
                 CtkWidget *child,
                 guint      left_attach,
                 guint      right_attach,
                 guint      top_attach,
                 guint      bottom_attach)
{
  CtkMenuShell *menu_shell;
  CtkWidget *parent;
  CtkCssNode *widget_node, *child_node;

  g_return_if_fail (CTK_IS_MENU (menu));
  g_return_if_fail (CTK_IS_MENU_ITEM (child));
  parent = ctk_widget_get_parent (child);
  g_return_if_fail (parent == NULL || parent == CTK_WIDGET (menu));
  g_return_if_fail (left_attach < right_attach);
  g_return_if_fail (top_attach < bottom_attach);

  menu_shell = CTK_MENU_SHELL (menu);

  if (!parent)
    {
      AttachInfo *ai = get_attach_info (child);

      ai->left_attach = left_attach;
      ai->right_attach = right_attach;
      ai->top_attach = top_attach;
      ai->bottom_attach = bottom_attach;

      menu_shell->priv->children = g_list_append (menu_shell->priv->children, child);

      widget_node = ctk_widget_get_css_node (CTK_WIDGET (menu));
      child_node = ctk_widget_get_css_node (child);
      ctk_css_node_insert_before (widget_node, child_node,
                                  ctk_css_gadget_get_node (menu->priv->bottom_arrow_gadget));

      ctk_widget_set_parent (child, CTK_WIDGET (menu));

      menu_queue_resize (menu);
    }
  else
    {
      ctk_container_child_set (CTK_CONTAINER (parent), child,
                               "left-attach",   left_attach,
                               "right-attach",  right_attach,
                               "top-attach",    top_attach,
                               "bottom-attach", bottom_attach,
                               NULL);
    }
}

static gint
ctk_menu_get_popup_delay (CtkMenuShell *menu_shell)
{
  return MENU_POPUP_DELAY;
}

static CtkWidget *
find_child_containing (CtkMenuShell *menu_shell,
                       int           left,
                       int           right,
                       int           top,
                       int           bottom)
{
  GList *list;

  /* find a child which includes the area given by
   * left, right, top, bottom.
   */
  for (list = menu_shell->priv->children; list; list = list->next)
    {
      gint l, r, t, b;

      if (!_ctk_menu_item_is_selectable (list->data))
        continue;

      get_effective_child_attach (list->data, &l, &r, &t, &b);

      if (l <= left && right <= r && t <= top && bottom <= b)
        return CTK_WIDGET (list->data);
    }

  return NULL;
}

static void
ctk_menu_move_current (CtkMenuShell         *menu_shell,
                       CtkMenuDirectionType  direction)
{
  CtkMenu *menu = CTK_MENU (menu_shell);
  gint i;
  gint l, r, t, b;
  CtkWidget *match = NULL;

  if (ctk_widget_get_direction (CTK_WIDGET (menu_shell)) == CTK_TEXT_DIR_RTL)
    {
      switch (direction)
        {
        case CTK_MENU_DIR_CHILD:
          direction = CTK_MENU_DIR_PARENT;
          break;
        case CTK_MENU_DIR_PARENT:
          direction = CTK_MENU_DIR_CHILD;
          break;
        default: ;
        }
    }

  /* use special table menu key bindings */
  if (menu_shell->priv->active_menu_item && ctk_menu_get_n_columns (menu) > 1)
    {
      get_effective_child_attach (menu_shell->priv->active_menu_item, &l, &r, &t, &b);

      if (direction == CTK_MENU_DIR_NEXT)
        {
          for (i = b; i < ctk_menu_get_n_rows (menu); i++)
            {
              match = find_child_containing (menu_shell, l, l + 1, i, i + 1);
              if (match)
                break;
            }

          if (!match)
            {
              /* wrap around */
              for (i = 0; i < t; i++)
                {
                  match = find_child_containing (menu_shell,
                                                 l, l + 1, i, i + 1);
                  if (match)
                    break;
                }
            }
        }
      else if (direction == CTK_MENU_DIR_PREV)
        {
          for (i = t; i > 0; i--)
            {
              match = find_child_containing (menu_shell,
                                             l, l + 1, i - 1, i);
              if (match)
                break;
            }

          if (!match)
            {
              /* wrap around */
              for (i = ctk_menu_get_n_rows (menu); i > b; i--)
                {
                  match = find_child_containing (menu_shell,
                                                 l, l + 1, i - 1, i);
                  if (match)
                    break;
                }
            }
        }
      else if (direction == CTK_MENU_DIR_PARENT)
        {
          /* we go one left if possible */
          if (l > 0)
            match = find_child_containing (menu_shell,
                                           l - 1, l, t, t + 1);

          if (!match)
            {
              CtkWidget *parent = menu_shell->priv->parent_menu_shell;

              if (!parent
                  || g_list_length (CTK_MENU_SHELL (parent)->priv->children) <= 1)
                match = menu_shell->priv->active_menu_item;
            }
        }
      else if (direction == CTK_MENU_DIR_CHILD)
        {
          /* we go one right if possible */
          if (r < ctk_menu_get_n_columns (menu))
            match = find_child_containing (menu_shell, r, r + 1, t, t + 1);

          if (!match)
            {
              CtkWidget *parent = menu_shell->priv->parent_menu_shell;

              if (! CTK_MENU_ITEM (menu_shell->priv->active_menu_item)->priv->submenu &&
                  (!parent ||
                   g_list_length (CTK_MENU_SHELL (parent)->priv->children) <= 1))
                match = menu_shell->priv->active_menu_item;
            }
        }

      if (match)
        {
          ctk_menu_shell_select_item (menu_shell, match);
          return;
        }
    }

  CTK_MENU_SHELL_CLASS (ctk_menu_parent_class)->move_current (menu_shell, direction);
}

static gint
get_visible_size (CtkMenu *menu)
{
  CtkMenuPrivate *priv = menu->priv;
  CtkAllocation allocation;
  CtkWidget *widget = CTK_WIDGET (menu);
  CtkContainer *container = CTK_CONTAINER (menu);
  CtkBorder padding;
  gint menu_height;

  ctk_widget_get_allocation (widget, &allocation);
  get_menu_padding (widget, &padding);

  menu_height = (allocation.height -
                 (2 * ctk_container_get_border_width (container)) -
                 padding.top - padding.bottom);

  if (!priv->tearoff_active)
    {
      CtkBorder arrow_border;

      get_arrows_border (menu, &arrow_border);
      menu_height -= arrow_border.top;
      menu_height -= arrow_border.bottom;
    }

  return menu_height;
}

/* Find the sensitive on-screen child containing @y, or if none,
 * the nearest selectable onscreen child. (%NULL if none)
 */
static CtkWidget *
child_at (CtkMenu *menu,
          gint     y)
{
  CtkMenuPrivate *priv = menu->priv;
  CtkMenuShell *menu_shell = CTK_MENU_SHELL (menu);
  CtkWidget *child = NULL;
  gint child_offset = 0;
  GList *children;
  gint menu_height;
  gint lower, upper; /* Onscreen bounds */

  menu_height = get_visible_size (menu);
  lower = priv->scroll_offset;
  upper = priv->scroll_offset + menu_height;

  for (children = menu_shell->priv->children; children; children = children->next)
    {
      if (ctk_widget_get_visible (children->data))
        {
          CtkRequisition child_requisition;

          ctk_widget_get_preferred_size (children->data,
                                         &child_requisition, NULL);

          if (_ctk_menu_item_is_selectable (children->data) &&
              child_offset >= lower &&
              child_offset + child_requisition.height <= upper)
            {
              child = children->data;

              if (child_offset + child_requisition.height > y &&
                  !CTK_IS_TEAROFF_MENU_ITEM (child))
                return child;
            }

          child_offset += child_requisition.height;
        }
    }

  return child;
}

static gint
get_menu_height (CtkMenu *menu)
{
  CtkMenuPrivate *priv = menu->priv;
  CtkWidget *widget = CTK_WIDGET (menu);
  CtkBorder padding;
  gint height;

  get_menu_padding (widget, &padding);

  height = priv->requested_height;
  height -= (ctk_container_get_border_width (CTK_CONTAINER (widget)) * 2) +
    padding.top + padding.bottom;

  if (!priv->tearoff_active)
    {
      CtkBorder arrow_border;

      get_arrows_border (menu, &arrow_border);
      height -= arrow_border.top;
      height -= arrow_border.bottom;
    }

  return height;
}

static void
ctk_menu_real_move_scroll (CtkMenu       *menu,
                           CtkScrollType  type)
{
  CtkMenuPrivate *priv = menu->priv;
  gint page_size = get_visible_size (menu);
  gint end_position = get_menu_height (menu);
  CtkMenuShell *menu_shell = CTK_MENU_SHELL (menu);

  switch (type)
    {
    case CTK_SCROLL_PAGE_UP:
    case CTK_SCROLL_PAGE_DOWN:
      {
        gint old_offset;
        gint new_offset;
        gint child_offset = 0;
        gboolean old_upper_arrow_visible;
        gint step;

        if (type == CTK_SCROLL_PAGE_UP)
          step = - page_size;
        else
          step = page_size;

        if (menu_shell->priv->active_menu_item)
          {
            gint child_height;

            if (compute_child_offset (menu, menu_shell->priv->active_menu_item,
                                      &child_offset, &child_height, NULL))
              child_offset += child_height / 2;
          }

        menu_shell->priv->ignore_enter = TRUE;
        old_upper_arrow_visible = priv->upper_arrow_visible && !priv->tearoff_active;
        old_offset = priv->scroll_offset;

        new_offset = priv->scroll_offset + step;
        new_offset = CLAMP (new_offset, 0, end_position - page_size);

        ctk_menu_scroll_to (menu, new_offset, CTK_MENU_SCROLL_FLAG_NONE);

        if (menu_shell->priv->active_menu_item)
          {
            CtkWidget *new_child;
            gboolean new_upper_arrow_visible = priv->upper_arrow_visible && !priv->tearoff_active;
            CtkBorder arrow_border;

            get_arrows_border (menu, &arrow_border);

            if (priv->scroll_offset != old_offset)
              step = priv->scroll_offset - old_offset;

            step -= (new_upper_arrow_visible - old_upper_arrow_visible) * arrow_border.top;

            new_child = child_at (menu, child_offset + step);
            if (new_child)
              ctk_menu_shell_select_item (menu_shell, new_child);
          }
      }
      break;
    case CTK_SCROLL_START:
      /* Ignore the enter event we might get if the pointer is on the menu */
      menu_shell->priv->ignore_enter = TRUE;
      ctk_menu_shell_select_first (menu_shell, TRUE);
      break;
    case CTK_SCROLL_END:
      /* Ignore the enter event we might get if the pointer is on the menu */
      menu_shell->priv->ignore_enter = TRUE;
      _ctk_menu_shell_select_last (menu_shell, TRUE);
      break;
    default:
      break;
    }
}

/**
 * ctk_menu_set_monitor:
 * @menu: a #CtkMenu
 * @monitor_num: the number of the monitor on which the menu should
 *    be popped up
 *
 * Informs CTK+ on which monitor a menu should be popped up.
 * See cdk_monitor_get_geometry().
 *
 * This function should be called from a #CtkMenuPositionFunc
 * if the menu should not appear on the same monitor as the pointer.
 * This information can’t be reliably inferred from the coordinates
 * returned by a #CtkMenuPositionFunc, since, for very long menus,
 * these coordinates may extend beyond the monitor boundaries or even
 * the screen boundaries.
 *
 * Since: 2.4
 */
void
ctk_menu_set_monitor (CtkMenu *menu,
                      gint     monitor_num)
{
  CtkMenuPrivate *priv;

  g_return_if_fail (CTK_IS_MENU (menu));

  priv = menu->priv;

  if (priv->monitor_num != monitor_num)
    {
      priv->monitor_num = monitor_num;
      g_object_notify (G_OBJECT (menu), "monitor");
    }
}

/**
 * ctk_menu_get_monitor:
 * @menu: a #CtkMenu
 *
 * Retrieves the number of the monitor on which to show the menu.
 *
 * Returns: the number of the monitor on which the menu should
 *    be popped up or -1, if no monitor has been set
 *
 * Since: 2.14
 */
gint
ctk_menu_get_monitor (CtkMenu *menu)
{
  g_return_val_if_fail (CTK_IS_MENU (menu), -1);

  return menu->priv->monitor_num;
}

/**
 * ctk_menu_place_on_monitor:
 * @menu: a #CtkMenu
 * @monitor: the monitor to place the menu on
 *
 * Places @menu on the given monitor.
 *
 * Since: 3.22
 */
void
ctk_menu_place_on_monitor (CtkMenu    *menu,
                           CdkMonitor *monitor)
{
  CdkDisplay *display;
  gint i, monitor_num;

  g_return_if_fail (CTK_IS_MENU (menu));

  display = ctk_widget_get_display (CTK_WIDGET (menu));
  monitor_num = 0;
  for (i = 0; ; i++)
    {
      CdkMonitor *m = cdk_display_get_monitor (display, i);
      if (m == monitor)
        {
          monitor_num = i;
          break;
        }
      if (m == NULL)
        break;
    }

  ctk_menu_set_monitor (menu, monitor_num);
}

/**
 * ctk_menu_get_for_attach_widget:
 * @widget: a #CtkWidget
 *
 * Returns a list of the menus which are attached to this widget.
 * This list is owned by CTK+ and must not be modified.
 *
 * Returns: (element-type CtkWidget) (transfer none): the list
 *     of menus attached to his widget.
 *
 * Since: 2.6
 */
GList*
ctk_menu_get_for_attach_widget (CtkWidget *widget)
{
  GList *list;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  list = g_object_get_data (G_OBJECT (widget), ATTACHED_MENUS);

  return list;
}

static void
ctk_menu_grab_notify (CtkWidget *widget,
                      gboolean   was_grabbed)
{
  CtkMenu *menu;
  CtkWidget *toplevel;
  CtkWindowGroup *group;
  CtkWidget *grab;
  CdkDevice *pointer;

  menu = CTK_MENU (widget);
  pointer = _ctk_menu_shell_get_grab_device (CTK_MENU_SHELL (widget));

  if (!pointer ||
      !ctk_widget_device_is_shadowed (widget, pointer))
    return;

  toplevel = ctk_widget_get_toplevel (widget);

  if (!CTK_IS_WINDOW (toplevel))
    return;

  group = ctk_window_get_group (CTK_WINDOW (toplevel));
  grab = ctk_window_group_get_current_grab (group);

  if (CTK_MENU_SHELL (widget)->priv->active && !CTK_IS_MENU_SHELL (grab) &&
      !ctk_widget_is_ancestor (grab, widget))
    ctk_menu_shell_cancel (CTK_MENU_SHELL (widget));

  menu->priv->drag_scroll_started = FALSE;
}

/**
 * ctk_menu_set_reserve_toggle_size:
 * @menu: a #CtkMenu
 * @reserve_toggle_size: whether to reserve size for toggles
 *
 * Sets whether the menu should reserve space for drawing toggles
 * or icons, regardless of their actual presence.
 *
 * Since: 2.18
 */
void
ctk_menu_set_reserve_toggle_size (CtkMenu  *menu,
                                  gboolean  reserve_toggle_size)
{
  CtkMenuPrivate *priv;
  gboolean no_toggle_size;

  g_return_if_fail (CTK_IS_MENU (menu));

  priv = menu->priv;

  no_toggle_size = !reserve_toggle_size;
  if (priv->no_toggle_size != no_toggle_size)
    {
      priv->no_toggle_size = no_toggle_size;

      g_object_notify (G_OBJECT (menu), "reserve-toggle-size");
    }
}

/**
 * ctk_menu_get_reserve_toggle_size:
 * @menu: a #CtkMenu
 *
 * Returns whether the menu reserves space for toggles and
 * icons, regardless of their actual presence.
 *
 * Returns: Whether the menu reserves toggle space
 *
 * Since: 2.18
 */
gboolean
ctk_menu_get_reserve_toggle_size (CtkMenu *menu)
{
  g_return_val_if_fail (CTK_IS_MENU (menu), FALSE);

  return !menu->priv->no_toggle_size;
}

/**
 * ctk_menu_new_from_model:
 * @model: a #GMenuModel
 *
 * Creates a #CtkMenu and populates it with menu items and
 * submenus according to @model.
 *
 * The created menu items are connected to actions found in the
 * #CtkApplicationWindow to which the menu belongs - typically
 * by means of being attached to a widget (see ctk_menu_attach_to_widget())
 * that is contained within the #CtkApplicationWindows widget hierarchy.
 *
 * Actions can also be added using ctk_widget_insert_action_group() on the menu's
 * attach widget or on any of its parent widgets.
 *
 * Returns: a new #CtkMenu
 *
 * Since: 3.4
 */
CtkWidget *
ctk_menu_new_from_model (GMenuModel *model)
{
  CtkWidget *menu;

  g_return_val_if_fail (G_IS_MENU_MODEL (model), NULL);

  menu = ctk_menu_new ();
  ctk_menu_shell_bind_model (CTK_MENU_SHELL (menu), model, NULL, TRUE);

  return menu;
}
