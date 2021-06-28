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
 * SECTION:ctkmenubar
 * @Title: CtkMenuBar
 * @Short_description: A subclass of CtkMenuShell which holds CtkMenuItem widgets
 * @See_also: #CtkMenuShell, #CtkMenu, #CtkMenuItem
 *
 * The #CtkMenuBar is a subclass of #CtkMenuShell which contains one or
 * more #CtkMenuItems. The result is a standard menu bar which can hold
 * many menu items.
 *
 * # CSS nodes
 *
 * CtkMenuBar has a single CSS node with name menubar.
 */

#include "config.h"

#include "ctkmenubar.h"

#include "ctkbindings.h"
#include "ctkcsscustomgadgetprivate.h"
#include "ctkmain.h"
#include "ctkmarshalers.h"
#include "ctkmenuitemprivate.h"
#include "ctkmenuprivate.h"
#include "ctkmenushellprivate.h"
#include "ctkrender.h"
#include "ctksettings.h"
#include "ctksizerequest.h"
#include "ctkwindow.h"
#include "ctkcontainerprivate.h"
#include "ctkintl.h"
#include "ctkprivate.h"
#include "ctktypebuiltins.h"
#include "ctkwidgetprivate.h"

#define MENU_BAR_POPUP_DELAY 0

/* Properties */
enum {
  PROP_0,
  PROP_PACK_DIRECTION,
  PROP_CHILD_PACK_DIRECTION
};

struct _CtkMenuBarPrivate
{
  CtkPackDirection pack_direction;
  CtkPackDirection child_pack_direction;

  CtkCssGadget *gadget;
};


static void ctk_menu_bar_set_property      (GObject             *object,
					    guint                prop_id,
					    const GValue        *value,
					    GParamSpec          *pspec);
static void ctk_menu_bar_get_property      (GObject             *object,
					    guint                prop_id,
					    GValue              *value,
					    GParamSpec          *pspec);
static void ctk_menu_bar_finalize          (GObject             *object);
static void ctk_menu_bar_get_preferred_width (CtkWidget     *widget,
					      gint          *minimum,
					      gint          *natural);
static void ctk_menu_bar_get_preferred_height (CtkWidget    *widget,
					       gint         *minimum,
					       gint         *natural);
static void ctk_menu_bar_get_preferred_width_for_height (CtkWidget    *widget,
                                                         gint          height,
                                                         gint         *minimum,
                                                         gint         *natural);
static void ctk_menu_bar_get_preferred_height_for_width (CtkWidget    *widget,
                                                         gint          width,
                                                         gint         *minimum,
                                                         gint         *natural);
static void ctk_menu_bar_size_allocate     (CtkWidget       *widget,
					    CtkAllocation   *allocation);
static gint ctk_menu_bar_draw              (CtkWidget       *widget,
                                            cairo_t         *cr);
static void ctk_menu_bar_hierarchy_changed (CtkWidget       *widget,
					    CtkWidget       *old_toplevel);
static gint ctk_menu_bar_get_popup_delay   (CtkMenuShell    *menu_shell);
static void ctk_menu_bar_move_current      (CtkMenuShell     *menu_shell,
                                            CtkMenuDirectionType direction);

static void ctk_menu_bar_measure (CtkCssGadget   *gadget,
                                  CtkOrientation  orientation,
                                  int             for_size,
                                  int            *minimum,
                                  int            *natural,
                                  int            *minimum_baseline,
                                  int            *natural_baseline,
                                  gpointer        data);
static void ctk_menu_bar_allocate (CtkCssGadget        *gadget,
                                   const CtkAllocation *allocation,
                                   int                  baseline,
                                   CtkAllocation       *out_clip,
                                   gpointer             data);
static gboolean ctk_menu_bar_render (CtkCssGadget *gadget,
                                     cairo_t      *cr,
                                     int           x,
                                     int           y,
                                     int           width,
                                     int           height,
                                     gpointer      data);

G_DEFINE_TYPE_WITH_PRIVATE (CtkMenuBar, ctk_menu_bar, CTK_TYPE_MENU_SHELL)

static void
ctk_menu_bar_class_init (CtkMenuBarClass *class)
{
  GObjectClass *gobject_class;
  CtkWidgetClass *widget_class;
  CtkMenuShellClass *menu_shell_class;
  CtkContainerClass *container_class;

  CtkBindingSet *binding_set;

  gobject_class = (GObjectClass*) class;
  widget_class = (CtkWidgetClass*) class;
  menu_shell_class = (CtkMenuShellClass*) class;
  container_class = (CtkContainerClass*) class;

  gobject_class->get_property = ctk_menu_bar_get_property;
  gobject_class->set_property = ctk_menu_bar_set_property;
  gobject_class->finalize = ctk_menu_bar_finalize;

  widget_class->get_preferred_width = ctk_menu_bar_get_preferred_width;
  widget_class->get_preferred_height = ctk_menu_bar_get_preferred_height;
  widget_class->get_preferred_width_for_height = ctk_menu_bar_get_preferred_width_for_height;
  widget_class->get_preferred_height_for_width = ctk_menu_bar_get_preferred_height_for_width;
  widget_class->size_allocate = ctk_menu_bar_size_allocate;
  widget_class->draw = ctk_menu_bar_draw;
  widget_class->hierarchy_changed = ctk_menu_bar_hierarchy_changed;

  menu_shell_class->submenu_placement = CTK_TOP_BOTTOM;
  menu_shell_class->get_popup_delay = ctk_menu_bar_get_popup_delay;
  menu_shell_class->move_current = ctk_menu_bar_move_current;

  binding_set = ctk_binding_set_by_class (class);
  ctk_binding_entry_add_signal (binding_set,
				CDK_KEY_Left, 0,
				"move-current", 1,
				CTK_TYPE_MENU_DIRECTION_TYPE,
				CTK_MENU_DIR_PREV);
  ctk_binding_entry_add_signal (binding_set,
				CDK_KEY_KP_Left, 0,
				"move-current", 1,
				CTK_TYPE_MENU_DIRECTION_TYPE,
				CTK_MENU_DIR_PREV);
  ctk_binding_entry_add_signal (binding_set,
				CDK_KEY_Right, 0,
				"move-current", 1,
				CTK_TYPE_MENU_DIRECTION_TYPE,
				CTK_MENU_DIR_NEXT);
  ctk_binding_entry_add_signal (binding_set,
				CDK_KEY_KP_Right, 0,
				"move-current", 1,
				CTK_TYPE_MENU_DIRECTION_TYPE,
				CTK_MENU_DIR_NEXT);
  ctk_binding_entry_add_signal (binding_set,
				CDK_KEY_Up, 0,
				"move-current", 1,
				CTK_TYPE_MENU_DIRECTION_TYPE,
				CTK_MENU_DIR_PARENT);
  ctk_binding_entry_add_signal (binding_set,
				CDK_KEY_KP_Up, 0,
				"move-current", 1,
				CTK_TYPE_MENU_DIRECTION_TYPE,
				CTK_MENU_DIR_PARENT);
  ctk_binding_entry_add_signal (binding_set,
				CDK_KEY_Down, 0,
				"move-current", 1,
				CTK_TYPE_MENU_DIRECTION_TYPE,
				CTK_MENU_DIR_CHILD);
  ctk_binding_entry_add_signal (binding_set,
				CDK_KEY_KP_Down, 0,
				"move-current", 1,
				CTK_TYPE_MENU_DIRECTION_TYPE,
				CTK_MENU_DIR_CHILD);

  /**
   * CtkMenuBar:pack-direction:
   *
   * The pack direction of the menubar. It determines how
   * menuitems are arranged in the menubar.
   *
   * Since: 2.8
   */
  g_object_class_install_property (gobject_class,
                                   PROP_PACK_DIRECTION,
                                   g_param_spec_enum ("pack-direction",
                                                      P_("Pack direction"),
                                                      P_("The pack direction of the menubar"),
                                                      CTK_TYPE_PACK_DIRECTION,
                                                      CTK_PACK_DIRECTION_LTR,
                                                      CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
  
  /**
   * CtkMenuBar:child-pack-direction:
   *
   * The child pack direction of the menubar. It determines how
   * the widgets contained in child menuitems are arranged.
   *
   * Since: 2.8
   */
  g_object_class_install_property (gobject_class,
                                   PROP_CHILD_PACK_DIRECTION,
                                   g_param_spec_enum ("child-pack-direction",
                                                      P_("Child Pack direction"),
                                                      P_("The child pack direction of the menubar"),
                                                      CTK_TYPE_PACK_DIRECTION,
                                                      CTK_PACK_DIRECTION_LTR,
                                                      CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
  

  /**
   * CtkMenuBar:shadow-type:
   *
   * The style of the shadow around the menubar.
   *
   * Deprecated: 3.20: Use CSS to determine the shadow; the value of
   *     this style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_enum ("shadow-type",
                                                              P_("Shadow type"),
                                                              P_("Style of bevel around the menubar"),
                                                              CTK_TYPE_SHADOW_TYPE,
                                                              CTK_SHADOW_OUT,
                                                              CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  /**
   * CtkMenuBar:internal-padding:
   *
   * Amount of border space between the menubar shadow and the menu items
   *
   * Deprecated: 3.8: use the standard padding CSS property (through objects
   *   like #CtkStyleContext and #CtkCssProvider); the value of this style
   *   property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("internal-padding",
							     P_("Internal padding"),
							     P_("Amount of border space between the menubar shadow and the menu items"),
							     0,
							     G_MAXINT,
                                                             0,
                                                             CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  ctk_container_class_handle_border_width (container_class);
  ctk_widget_class_set_accessible_role (widget_class, ATK_ROLE_MENU_BAR);
  ctk_widget_class_set_css_name (widget_class, "menubar");
}

static void
ctk_menu_bar_init (CtkMenuBar *menu_bar)
{
  CtkMenuBarPrivate *priv;
  CtkWidget *widget;
  CtkCssNode *widget_node;

  priv = menu_bar->priv = ctk_menu_bar_get_instance_private (menu_bar);

  widget = CTK_WIDGET (menu_bar);
  widget_node = ctk_widget_get_css_node (widget);
  priv->gadget = ctk_css_custom_gadget_new_for_node (widget_node,
                                                     widget,
                                                     ctk_menu_bar_measure,
                                                     ctk_menu_bar_allocate,
                                                     ctk_menu_bar_render,
                                                     NULL, NULL);
}

/**
 * ctk_menu_bar_new:
 *
 * Creates a new #CtkMenuBar
 *
 * Returns: the new menu bar, as a #CtkWidget
 */
CtkWidget*
ctk_menu_bar_new (void)
{
  return g_object_new (CTK_TYPE_MENU_BAR, NULL);
}

static void
ctk_menu_bar_finalize (GObject *object)
{
  CtkMenuBar *menu_bar = CTK_MENU_BAR (object);

  g_clear_object (&menu_bar->priv->gadget);

  G_OBJECT_CLASS (ctk_menu_bar_parent_class)->finalize (object);
}

static void
ctk_menu_bar_set_property (GObject      *object,
			   guint         prop_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{
  CtkMenuBar *menubar = CTK_MENU_BAR (object);
  
  switch (prop_id)
    {
    case PROP_PACK_DIRECTION:
      ctk_menu_bar_set_pack_direction (menubar, g_value_get_enum (value));
      break;
    case PROP_CHILD_PACK_DIRECTION:
      ctk_menu_bar_set_child_pack_direction (menubar, g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_menu_bar_get_property (GObject    *object,
			   guint       prop_id,
			   GValue     *value,
			   GParamSpec *pspec)
{
  CtkMenuBar *menubar = CTK_MENU_BAR (object);
  
  switch (prop_id)
    {
    case PROP_PACK_DIRECTION:
      g_value_set_enum (value, ctk_menu_bar_get_pack_direction (menubar));
      break;
    case PROP_CHILD_PACK_DIRECTION:
      g_value_set_enum (value, ctk_menu_bar_get_child_pack_direction (menubar));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_menu_bar_measure (CtkCssGadget   *gadget,
                      CtkOrientation  orientation,
                      int             size,
                      int            *minimum,
                      int            *natural,
                      int            *minimum_baseline,
                      int            *natural_baseline,
                      gpointer        data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkMenuBar *menu_bar;
  CtkMenuBarPrivate *priv;
  CtkMenuShell *menu_shell;
  CtkWidget *child;
  GList *children;
  gboolean use_toggle_size, use_maximize;
  gint child_minimum, child_natural;

  *minimum = 0;
  *natural = 0;

  menu_bar = CTK_MENU_BAR (widget);
  menu_shell = CTK_MENU_SHELL (widget);
  priv = menu_bar->priv;

  children = menu_shell->priv->children;

  if (priv->child_pack_direction == CTK_PACK_DIRECTION_LTR ||
      priv->child_pack_direction == CTK_PACK_DIRECTION_RTL)
    use_toggle_size = (orientation == CTK_ORIENTATION_HORIZONTAL);
  else
    use_toggle_size = (orientation == CTK_ORIENTATION_VERTICAL);

  if (priv->pack_direction == CTK_PACK_DIRECTION_LTR ||
      priv->pack_direction == CTK_PACK_DIRECTION_RTL)
    use_maximize = (orientation == CTK_ORIENTATION_VERTICAL);
  else
    use_maximize = (orientation == CTK_ORIENTATION_HORIZONTAL);

  while (children)
    {
      child = children->data;
      children = children->next;

      if (ctk_widget_get_visible (child))
        {
          _ctk_widget_get_preferred_size_for_size (child, orientation, size, &child_minimum, &child_natural, NULL, NULL);

          if (use_toggle_size)
            {
              gint toggle_size;

              ctk_menu_item_toggle_size_request (CTK_MENU_ITEM (child),
                                                 &toggle_size);

              child_minimum += toggle_size;
              child_natural += toggle_size;
            }

          if (use_maximize)
            {
              *minimum = MAX (*minimum, child_minimum);
              *natural = MAX (*natural, child_natural);
            }
          else
            {
              *minimum += child_minimum;
              *natural += child_natural;
            }
        }
    }
}

static void
ctk_menu_bar_get_preferred_width (CtkWidget *widget,
				  gint      *minimum,
				  gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CTK_MENU_BAR (widget)->priv->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_menu_bar_get_preferred_height (CtkWidget *widget,
				   gint      *minimum,
				   gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CTK_MENU_BAR (widget)->priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_menu_bar_get_preferred_width_for_height (CtkWidget *widget,
                                             gint       height,
                                             gint      *minimum,
                                             gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CTK_MENU_BAR (widget)->priv->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     height,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_menu_bar_get_preferred_height_for_width (CtkWidget *widget,
                                             gint       width,
                                             gint      *minimum,
                                             gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CTK_MENU_BAR (widget)->priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     width,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_menu_bar_allocate (CtkCssGadget        *gadget,
                       const CtkAllocation *allocation,
                       int                  baseline,
                       CtkAllocation       *out_clip,
                       gpointer             data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);
  CtkMenuBar *menu_bar;
  CtkMenuShell *menu_shell;
  CtkMenuBarPrivate *priv;
  CtkWidget *child;
  GList *children;
  CtkAllocation remaining_space;
  GArray *requested_sizes;
  gint toggle_size;
  guint i;

  menu_bar = CTK_MENU_BAR (widget);
  menu_shell = CTK_MENU_SHELL (widget);
  priv = menu_bar->priv;

  if (!menu_shell->priv->children)
    return;

  remaining_space = *allocation;
  requested_sizes = g_array_new (FALSE, FALSE, sizeof (CtkRequestedSize));

  if (priv->pack_direction == CTK_PACK_DIRECTION_LTR ||
      priv->pack_direction == CTK_PACK_DIRECTION_RTL)
    {
      int size = remaining_space.width;
      gboolean ltr = (ctk_widget_get_direction (widget) == CTK_TEXT_DIR_LTR) == (priv->pack_direction == CTK_PACK_DIRECTION_LTR);

      for (children = menu_shell->priv->children; children; children = children->next)
        {
          CtkRequestedSize request;
          child = children->data;

          if (!ctk_widget_get_visible (child))
            continue;

          request.data = child;
          ctk_widget_get_preferred_width_for_height (child, 
                                                     remaining_space.height,
                                                     &request.minimum_size, 
                                                     &request.natural_size);
          ctk_menu_item_toggle_size_request (CTK_MENU_ITEM (child),
                                             &toggle_size);
          request.minimum_size += toggle_size;
          request.natural_size += toggle_size;

          ctk_menu_item_toggle_size_allocate (CTK_MENU_ITEM (child), toggle_size);

          g_array_append_val (requested_sizes, request);

          size -= request.minimum_size;
        }

      size = ctk_distribute_natural_allocation (size,
                                                requested_sizes->len,
                                                (CtkRequestedSize *) requested_sizes->data);

      for (i = 0; i < requested_sizes->len; i++)
        {
          CtkAllocation child_allocation = remaining_space;
          CtkRequestedSize *request = &g_array_index (requested_sizes, CtkRequestedSize, i);

          child_allocation.width = request->minimum_size;
          remaining_space.width -= request->minimum_size;

          if (i + 1 == requested_sizes->len && CTK_IS_MENU_ITEM (request->data) &&
              CTK_MENU_ITEM (request->data)->priv->right_justify)
            ltr = !ltr;

          if (ltr)
            remaining_space.x += request->minimum_size;
          else
            child_allocation.x += remaining_space.width;

          ctk_widget_size_allocate (request->data, &child_allocation);
        }
    }
  else
    {
      int size = remaining_space.height;
      gboolean ttb = (priv->pack_direction == CTK_PACK_DIRECTION_TTB);

      for (children = menu_shell->priv->children; children; children = children->next)
        {
          CtkRequestedSize request;
          child = children->data;

          if (!ctk_widget_get_visible (child))
            continue;

          request.data = child;
          ctk_widget_get_preferred_height_for_width (child, 
                                                     remaining_space.width,
                                                     &request.minimum_size, 
                                                     &request.natural_size);
          ctk_menu_item_toggle_size_request (CTK_MENU_ITEM (child),
                                             &toggle_size);
          request.minimum_size += toggle_size;
          request.natural_size += toggle_size;

          ctk_menu_item_toggle_size_allocate (CTK_MENU_ITEM (child), toggle_size);

          g_array_append_val (requested_sizes, request);

          size -= request.minimum_size;
        }

      size = ctk_distribute_natural_allocation (size,
                                                requested_sizes->len,
                                                (CtkRequestedSize *) requested_sizes->data);

      for (i = 0; i < requested_sizes->len; i++)
        {
          CtkAllocation child_allocation = remaining_space;
          CtkRequestedSize *request = &g_array_index (requested_sizes, CtkRequestedSize, i);

          child_allocation.height = request->minimum_size;
          remaining_space.height -= request->minimum_size;

          if (i + 1 == requested_sizes->len && CTK_IS_MENU_ITEM (request->data) &&
              CTK_MENU_ITEM (request->data)->priv->right_justify)
            ttb = !ttb;

          if (ttb)
            remaining_space.y += request->minimum_size;
          else
            child_allocation.y += remaining_space.height;

          ctk_widget_size_allocate (request->data, &child_allocation);
        }
    }

  g_array_free (requested_sizes, TRUE);
}

static void
ctk_menu_bar_size_allocate (CtkWidget     *widget,
			    CtkAllocation *allocation)
{
  CtkMenuBar *menu_bar;
  CtkAllocation clip, content_allocation;

  menu_bar = CTK_MENU_BAR (widget);
  ctk_widget_set_allocation (widget, allocation);

  if (ctk_widget_get_realized (widget))
    cdk_window_move_resize (ctk_widget_get_window (widget),
                            allocation->x, allocation->y,
                            allocation->width, allocation->height);

  content_allocation = *allocation;
  content_allocation.x = content_allocation.y = 0;
  ctk_css_gadget_allocate (menu_bar->priv->gadget,
                           &content_allocation,
                           ctk_widget_get_allocated_baseline (widget),
                           &clip);

  clip.x += allocation->x;
  clip.y += allocation->y;
  ctk_widget_set_clip (widget, &clip);
}

static gboolean
ctk_menu_bar_render (CtkCssGadget *gadget,
                     cairo_t      *cr,
                     int           x,
                     int           y,
                     int           width,
                     int           height,
                     gpointer      data)
{
  CtkWidget *widget = ctk_css_gadget_get_owner (gadget);

  CTK_WIDGET_CLASS (ctk_menu_bar_parent_class)->draw (widget, cr);

  return FALSE;
}

static gboolean
ctk_menu_bar_draw (CtkWidget *widget,
		   cairo_t   *cr)
{
  ctk_css_gadget_draw (CTK_MENU_BAR (widget)->priv->gadget, cr);

  return FALSE;
}

static GList *
get_menu_bars (CtkWindow *window)
{
  return g_object_get_data (G_OBJECT (window), "ctk-menu-bar-list");
}

GList *
_ctk_menu_bar_get_viewable_menu_bars (CtkWindow *window)
{
  GList *menu_bars;
  GList *viewable_menu_bars = NULL;

  for (menu_bars = get_menu_bars (window);
       menu_bars;
       menu_bars = menu_bars->next)
    {
      CtkWidget *widget = menu_bars->data;
      gboolean viewable = TRUE;
      
      while (widget)
	{
	  if (!ctk_widget_get_mapped (widget))
	    viewable = FALSE;

          widget = ctk_widget_get_parent (widget);
	}

      if (viewable)
	viewable_menu_bars = g_list_prepend (viewable_menu_bars, menu_bars->data);
    }

  return g_list_reverse (viewable_menu_bars);
}

static void
set_menu_bars (CtkWindow *window,
	       GList     *menubars)
{
  g_object_set_data (G_OBJECT (window), I_("ctk-menu-bar-list"), menubars);
}

static void
add_to_window (CtkWindow  *window,
               CtkMenuBar *menubar)
{
  GList *menubars = get_menu_bars (window);

  set_menu_bars (window, g_list_prepend (menubars, menubar));
}

static void
remove_from_window (CtkWindow  *window,
                    CtkMenuBar *menubar)
{
  GList *menubars = get_menu_bars (window);

  menubars = g_list_remove (menubars, menubar);
  set_menu_bars (window, menubars);
}

static void
ctk_menu_bar_hierarchy_changed (CtkWidget *widget,
				CtkWidget *old_toplevel)
{
  CtkWidget *toplevel;  
  CtkMenuBar *menubar;

  menubar = CTK_MENU_BAR (widget);

  toplevel = ctk_widget_get_toplevel (widget);

  if (old_toplevel)
    remove_from_window (CTK_WINDOW (old_toplevel), menubar);
  
  if (ctk_widget_is_toplevel (toplevel))
    add_to_window (CTK_WINDOW (toplevel), menubar);
}

/**
 * _ctk_menu_bar_cycle_focus:
 * @menubar: a #CtkMenuBar
 * @dir: direction in which to cycle the focus
 * 
 * Move the focus between menubars in the toplevel.
 **/
void
_ctk_menu_bar_cycle_focus (CtkMenuBar       *menubar,
			   CtkDirectionType  dir)
{
  CtkWidget *toplevel = ctk_widget_get_toplevel (CTK_WIDGET (menubar));
  CtkMenuItem *to_activate = NULL;

  if (ctk_widget_is_toplevel (toplevel))
    {
      GList *tmp_menubars = _ctk_menu_bar_get_viewable_menu_bars (CTK_WINDOW (toplevel));
      GList *menubars;
      GList *current;

      menubars = _ctk_container_focus_sort (CTK_CONTAINER (toplevel), tmp_menubars,
					    dir, CTK_WIDGET (menubar));
      g_list_free (tmp_menubars);

      if (menubars)
	{
	  current = g_list_find (menubars, menubar);

	  if (current && current->next)
	    {
	      CtkMenuShell *new_menushell = CTK_MENU_SHELL (current->next->data);
	      if (new_menushell->priv->children)
		to_activate = new_menushell->priv->children->data;
	    }
	}
	  
      g_list_free (menubars);
    }

  ctk_menu_shell_cancel (CTK_MENU_SHELL (menubar));

  if (to_activate)
    g_signal_emit_by_name (to_activate, "activate_item");
}

static gint
ctk_menu_bar_get_popup_delay (CtkMenuShell *menu_shell)
{
  return MENU_BAR_POPUP_DELAY;
}

static void
ctk_menu_bar_move_current (CtkMenuShell         *menu_shell,
			   CtkMenuDirectionType  direction)
{
  CtkMenuBar *menubar = CTK_MENU_BAR (menu_shell);
  CtkTextDirection text_dir;
  CtkPackDirection pack_dir;

  text_dir = ctk_widget_get_direction (CTK_WIDGET (menubar));
  pack_dir = ctk_menu_bar_get_pack_direction (menubar);
  
  if (pack_dir == CTK_PACK_DIRECTION_LTR || pack_dir == CTK_PACK_DIRECTION_RTL)
     {
      if ((text_dir == CTK_TEXT_DIR_RTL) == (pack_dir == CTK_PACK_DIRECTION_LTR))
	{
	  switch (direction) 
	    {      
	    case CTK_MENU_DIR_PREV:
	      direction = CTK_MENU_DIR_NEXT;
	      break;
	    case CTK_MENU_DIR_NEXT:
	      direction = CTK_MENU_DIR_PREV;
	      break;
	    default: ;
	    }
	}
    }
  else
    {
      switch (direction) 
	{
	case CTK_MENU_DIR_PARENT:
	  if ((text_dir == CTK_TEXT_DIR_LTR) == (pack_dir == CTK_PACK_DIRECTION_TTB))
	    direction = CTK_MENU_DIR_PREV;
	  else
	    direction = CTK_MENU_DIR_NEXT;
	  break;
	case CTK_MENU_DIR_CHILD:
	  if ((text_dir == CTK_TEXT_DIR_LTR) == (pack_dir == CTK_PACK_DIRECTION_TTB))
	    direction = CTK_MENU_DIR_NEXT;
	  else
	    direction = CTK_MENU_DIR_PREV;
	  break;
	case CTK_MENU_DIR_PREV:
	  if (text_dir == CTK_TEXT_DIR_RTL)	  
	    direction = CTK_MENU_DIR_CHILD;
	  else
	    direction = CTK_MENU_DIR_PARENT;
	  break;
	case CTK_MENU_DIR_NEXT:
	  if (text_dir == CTK_TEXT_DIR_RTL)	  
	    direction = CTK_MENU_DIR_PARENT;
	  else
	    direction = CTK_MENU_DIR_CHILD;
	  break;
	default: ;
	}
    }
  
  CTK_MENU_SHELL_CLASS (ctk_menu_bar_parent_class)->move_current (menu_shell, direction);
}

/**
 * ctk_menu_bar_get_pack_direction:
 * @menubar: a #CtkMenuBar
 * 
 * Retrieves the current pack direction of the menubar. 
 * See ctk_menu_bar_set_pack_direction().
 *
 * Returns: the pack direction
 *
 * Since: 2.8
 */
CtkPackDirection
ctk_menu_bar_get_pack_direction (CtkMenuBar *menubar)
{
  g_return_val_if_fail (CTK_IS_MENU_BAR (menubar), 
			CTK_PACK_DIRECTION_LTR);

  return menubar->priv->pack_direction;
}

/**
 * ctk_menu_bar_set_pack_direction:
 * @menubar: a #CtkMenuBar
 * @pack_dir: a new #CtkPackDirection
 * 
 * Sets how items should be packed inside a menubar.
 * 
 * Since: 2.8
 */
void
ctk_menu_bar_set_pack_direction (CtkMenuBar       *menubar,
                                 CtkPackDirection  pack_dir)
{
  CtkMenuBarPrivate *priv;
  GList *l;

  g_return_if_fail (CTK_IS_MENU_BAR (menubar));

  priv = menubar->priv;

  if (priv->pack_direction != pack_dir)
    {
      priv->pack_direction = pack_dir;

      ctk_widget_queue_resize (CTK_WIDGET (menubar));

      for (l = CTK_MENU_SHELL (menubar)->priv->children; l; l = l->next)
	ctk_widget_queue_resize (CTK_WIDGET (l->data));

      g_object_notify (G_OBJECT (menubar), "pack-direction");
    }
}

/**
 * ctk_menu_bar_get_child_pack_direction:
 * @menubar: a #CtkMenuBar
 * 
 * Retrieves the current child pack direction of the menubar.
 * See ctk_menu_bar_set_child_pack_direction().
 *
 * Returns: the child pack direction
 *
 * Since: 2.8
 */
CtkPackDirection
ctk_menu_bar_get_child_pack_direction (CtkMenuBar *menubar)
{
  g_return_val_if_fail (CTK_IS_MENU_BAR (menubar), 
			CTK_PACK_DIRECTION_LTR);

  return menubar->priv->child_pack_direction;
}

/**
 * ctk_menu_bar_set_child_pack_direction:
 * @menubar: a #CtkMenuBar
 * @child_pack_dir: a new #CtkPackDirection
 * 
 * Sets how widgets should be packed inside the children of a menubar.
 * 
 * Since: 2.8
 */
void
ctk_menu_bar_set_child_pack_direction (CtkMenuBar       *menubar,
                                       CtkPackDirection  child_pack_dir)
{
  CtkMenuBarPrivate *priv;
  GList *l;

  g_return_if_fail (CTK_IS_MENU_BAR (menubar));

  priv = menubar->priv;

  if (priv->child_pack_direction != child_pack_dir)
    {
      priv->child_pack_direction = child_pack_dir;

      ctk_widget_queue_resize (CTK_WIDGET (menubar));

      for (l = CTK_MENU_SHELL (menubar)->priv->children; l; l = l->next)
	ctk_widget_queue_resize (CTK_WIDGET (l->data));

      g_object_notify (G_OBJECT (menubar), "child-pack-direction");
    }
}

/**
 * ctk_menu_bar_new_from_model:
 * @model: a #GMenuModel
 *
 * Creates a new #CtkMenuBar and populates it with menu items
 * and submenus according to @model.
 *
 * The created menu items are connected to actions found in the
 * #CtkApplicationWindow to which the menu bar belongs - typically
 * by means of being contained within the #CtkApplicationWindows
 * widget hierarchy.
 *
 * Returns: a new #CtkMenuBar
 *
 * Since: 3.4
 */
CtkWidget *
ctk_menu_bar_new_from_model (GMenuModel *model)
{
  CtkWidget *menubar;

  g_return_val_if_fail (G_IS_MENU_MODEL (model), NULL);

  menubar = ctk_menu_bar_new ();
  ctk_menu_shell_bind_model (CTK_MENU_SHELL (menubar), model, NULL, FALSE);

  return menubar;
}
