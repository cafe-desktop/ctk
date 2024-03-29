/* CTK - The GIMP Toolkit
 * Recent chooser action for CtkUIManager
 *
 * Copyright (C) 2007, Emmanuele Bassi
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

#include "ctkintl.h"
#include "ctkrecentaction.h"
#include "ctkimagemenuitem.h"
#include "ctkmenutoolbutton.h"
#include "ctkrecentchooser.h"
#include "ctkrecentchoosermenu.h"
#include "ctkrecentchooserutils.h"
#include "ctkrecentchooserprivate.h"
#include "ctkprivate.h"

/**
 * SECTION:ctkrecentaction
 * @Short_description: An action of which represents a list of recently used files
 * @Title: CtkRecentAction
 *
 * A #CtkRecentAction represents a list of recently used files, which
 * can be shown by widgets such as #CtkRecentChooserDialog or
 * #CtkRecentChooserMenu.
 *
 * To construct a submenu showing recently used files, use a #CtkRecentAction
 * as the action for a <menuitem>. To construct a menu toolbutton showing
 * the recently used files in the popup menu, use a #CtkRecentAction as the
 * action for a <toolitem> element.
 */


#define FALLBACK_ITEM_LIMIT     10


struct _CtkRecentActionPrivate
{
  CtkRecentManager *manager;

  guint show_numbers   : 1;

  /* RecentChooser properties */
  guint show_private   : 1;
  guint show_not_found : 1;
  guint show_tips      : 1;
  guint show_icons     : 1;
  guint local_only     : 1;

  gint limit;

  CtkRecentSortType sort_type;
  CtkRecentSortFunc sort_func;
  gpointer          sort_data;
  GDestroyNotify    data_destroy;

  CtkRecentFilter *current_filter;

  GSList *choosers;
  CtkRecentChooser *current_chooser;
};

enum
{
  PROP_0,

  PROP_SHOW_NUMBERS
};

static void ctk_recent_chooser_iface_init (CtkRecentChooserIface *iface);

G_DEFINE_TYPE_WITH_CODE (CtkRecentAction,
                         ctk_recent_action,
                         CTK_TYPE_ACTION,
                         G_ADD_PRIVATE (CtkRecentAction)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_RECENT_CHOOSER,
                                                ctk_recent_chooser_iface_init));

static gboolean
ctk_recent_action_set_current_uri (CtkRecentChooser  *chooser,
                                   const gchar       *uri,
                                   GError           **error)
{
  CtkRecentAction *action = CTK_RECENT_ACTION (chooser);
  CtkRecentActionPrivate *priv = action->priv;
  GSList *l;

  for (l = priv->choosers; l; l = l->next)
    {
      CtkRecentChooser *recent_chooser = l->data;

      if (!ctk_recent_chooser_set_current_uri (recent_chooser, uri, error))
        return FALSE;
    }

  return TRUE;
}

static gchar *
ctk_recent_action_get_current_uri (CtkRecentChooser *chooser)
{
  CtkRecentAction *recent_action = CTK_RECENT_ACTION (chooser);
  CtkRecentActionPrivate *priv = recent_action->priv;

  if (priv->current_chooser)
    return ctk_recent_chooser_get_current_uri (priv->current_chooser);

  return NULL;
}

static gboolean
ctk_recent_action_select_uri (CtkRecentChooser  *chooser,
                              const gchar       *uri,
                              GError           **error)
{
  CtkRecentAction *action = CTK_RECENT_ACTION (chooser);
  CtkRecentActionPrivate *priv = action->priv;
  GSList *l;

  for (l = priv->choosers; l; l = l->next)
    {
      CtkRecentChooser *recent_chooser = l->data;

      if (!ctk_recent_chooser_select_uri (recent_chooser, uri, error))
        return FALSE;
    }

  return TRUE;
}

static void
ctk_recent_action_unselect_uri (CtkRecentChooser *chooser,
                                const gchar      *uri)
{
  CtkRecentAction *action = CTK_RECENT_ACTION (chooser);
  CtkRecentActionPrivate *priv = action->priv;
  GSList *l;

  for (l = priv->choosers; l; l = l->next)
    {
      CtkRecentChooser *c = l->data;
      ctk_recent_chooser_unselect_uri (c, uri);
    }
}

static void
ctk_recent_action_select_all (CtkRecentChooser *chooser)
{
  g_warning ("This function is not implemented for widgets of class '%s'",
             g_type_name (G_OBJECT_TYPE (chooser)));
}

static void
ctk_recent_action_unselect_all (CtkRecentChooser *chooser)
{
  g_warning ("This function is not implemented for widgets of class '%s'",
             g_type_name (G_OBJECT_TYPE (chooser)));
}


static GList *
ctk_recent_action_get_items (CtkRecentChooser *chooser)
{
  CtkRecentAction *action = CTK_RECENT_ACTION (chooser);
  CtkRecentActionPrivate *priv = action->priv;

  return _ctk_recent_chooser_get_items (chooser,
                                        priv->current_filter,
                                        priv->sort_func,
                                        priv->sort_data);
}

static CtkRecentManager *
ctk_recent_action_get_recent_manager (CtkRecentChooser *chooser)
{
  return CTK_RECENT_ACTION (chooser)->priv->manager;
}

static void
ctk_recent_action_set_sort_func (CtkRecentChooser  *chooser,
                                 CtkRecentSortFunc  sort_func,
                                 gpointer           sort_data,
                                 GDestroyNotify     data_destroy)
{
  CtkRecentAction *action = CTK_RECENT_ACTION (chooser);
  CtkRecentActionPrivate *priv = action->priv;
  GSList *l;
  
  if (priv->data_destroy)
    {
      priv->data_destroy (priv->sort_data);
      priv->data_destroy = NULL;
    }
      
  priv->sort_func = NULL;
  priv->sort_data = NULL;
  
  if (sort_func)
    {
      priv->sort_func = sort_func;
      priv->sort_data = sort_data;
      priv->data_destroy = data_destroy;
    }

  for (l = priv->choosers; l; l = l->next)
    {
      CtkRecentChooser *chooser_menu = l->data;
      
      ctk_recent_chooser_set_sort_func (chooser_menu, priv->sort_func,
                                        priv->sort_data,
                                        priv->data_destroy);
    }
}

static void
set_current_filter (CtkRecentAction *action,
                    CtkRecentFilter *filter)
{
  CtkRecentActionPrivate *priv = action->priv;

  g_object_ref (action);

  if (priv->current_filter)
    g_object_unref (priv->current_filter);

  priv->current_filter = filter;

  if (priv->current_filter)
    g_object_ref_sink (priv->current_filter);

  g_object_notify (G_OBJECT (action), "filter");

  g_object_unref (action);
}

static void
ctk_recent_action_add_filter (CtkRecentChooser *chooser,
                              CtkRecentFilter  *filter)
{
  CtkRecentAction *action = CTK_RECENT_ACTION (chooser);
  CtkRecentActionPrivate *priv = action->priv;

  if (priv->current_filter != filter)
    set_current_filter (CTK_RECENT_ACTION (chooser), filter);
}

static void
ctk_recent_action_remove_filter (CtkRecentChooser *chooser,
                                 CtkRecentFilter  *filter)
{
  CtkRecentAction *action = CTK_RECENT_ACTION (chooser);
  CtkRecentActionPrivate *priv = action->priv;

  if (priv->current_filter == filter)
    set_current_filter (CTK_RECENT_ACTION (chooser), NULL);
}

static GSList *
ctk_recent_action_list_filters (CtkRecentChooser *chooser)
{
  CtkRecentAction *action = CTK_RECENT_ACTION (chooser);
  CtkRecentActionPrivate *priv = action->priv;
  GSList *retval = NULL;
  CtkRecentFilter *current_filter;

  current_filter = priv->current_filter;
  retval = g_slist_prepend (retval, current_filter);

  return retval;
}


static void
ctk_recent_chooser_iface_init (CtkRecentChooserIface *iface)
{
  iface->set_current_uri = ctk_recent_action_set_current_uri;
  iface->get_current_uri = ctk_recent_action_get_current_uri;
  iface->select_uri = ctk_recent_action_select_uri;
  iface->unselect_uri = ctk_recent_action_unselect_uri;
  iface->select_all = ctk_recent_action_select_all;
  iface->unselect_all = ctk_recent_action_unselect_all;
  iface->get_items = ctk_recent_action_get_items;
  iface->get_recent_manager = ctk_recent_action_get_recent_manager;
  iface->set_sort_func = ctk_recent_action_set_sort_func;
  iface->add_filter = ctk_recent_action_add_filter;
  iface->remove_filter = ctk_recent_action_remove_filter;
  iface->list_filters = ctk_recent_action_list_filters;
}

static void
ctk_recent_action_activate (CtkAction *action)
{
  CtkRecentAction *recent_action = CTK_RECENT_ACTION (action);
  CtkRecentActionPrivate *priv = recent_action->priv;

  /* we have probably been invoked by a menu tool button or by a
   * direct call of ctk_action_activate(); since no item has been
   * selected, we must unset the current recent chooser pointer
   */
  priv->current_chooser = NULL;
}

static void
delegate_selection_changed (CtkRecentAction  *action,
                            CtkRecentChooser *chooser)
{
  CtkRecentActionPrivate *priv = action->priv;

  priv->current_chooser = chooser;

  g_signal_emit_by_name (action, "selection-changed");
}

static void
delegate_item_activated (CtkRecentAction  *action,
                         CtkRecentChooser *chooser)
{
  CtkRecentActionPrivate *priv = action->priv;

  priv->current_chooser = chooser;

  g_signal_emit_by_name (action, "item-activated");
}

static void
ctk_recent_action_connect_proxy (CtkAction *action,
                                 CtkWidget *widget)
{
  CtkRecentAction *recent_action = CTK_RECENT_ACTION (action);
  CtkRecentActionPrivate *priv = recent_action->priv;

  /* it can only be a recent chooser implementor anyway... */
  if (CTK_IS_RECENT_CHOOSER (widget) &&
      !g_slist_find (priv->choosers, widget))
    {
      if (priv->sort_func)
        {
          ctk_recent_chooser_set_sort_func (CTK_RECENT_CHOOSER (widget),
                                            priv->sort_func,
                                            priv->sort_data,
                                            priv->data_destroy);
        }

      g_signal_connect_swapped (widget, "selection_changed",
                                G_CALLBACK (delegate_selection_changed),
                                action);
      g_signal_connect_swapped (widget, "item-activated",
                                G_CALLBACK (delegate_item_activated),
                                action);
    }

  if (CTK_ACTION_CLASS (ctk_recent_action_parent_class)->connect_proxy)
    CTK_ACTION_CLASS (ctk_recent_action_parent_class)->connect_proxy (action, widget);
}

static void
ctk_recent_action_disconnect_proxy (CtkAction *action,
                                    CtkWidget *widget)
{
  CtkRecentAction *recent_action = CTK_RECENT_ACTION (action);
  CtkRecentActionPrivate *priv = recent_action->priv;

  /* if it was one of the recent choosers we created, remove it
   * from the list
   */
  if (g_slist_find (priv->choosers, widget))
    priv->choosers = g_slist_remove (priv->choosers, widget);

  if (CTK_ACTION_CLASS (ctk_recent_action_parent_class)->disconnect_proxy)
    CTK_ACTION_CLASS (ctk_recent_action_parent_class)->disconnect_proxy (action, widget);
}

static CtkWidget *
ctk_recent_action_create_menu (CtkAction *action)
{
  CtkRecentAction *recent_action = CTK_RECENT_ACTION (action);
  CtkRecentActionPrivate *priv = recent_action->priv;
  CtkWidget *widget;

  widget = g_object_new (CTK_TYPE_RECENT_CHOOSER_MENU,
                         "show-private", priv->show_private,
                         "show-not-found", priv->show_not_found,
                         "show-tips", priv->show_tips,
                         "show-icons", priv->show_icons,
                         "show-numbers", priv->show_numbers,
                         "limit", priv->limit,
                         "sort-type", priv->sort_type,
                         "recent-manager", priv->manager,
                         "filter", priv->current_filter,
                         "local-only", priv->local_only,
                         NULL);
  
  if (priv->sort_func)
    {
      ctk_recent_chooser_set_sort_func (CTK_RECENT_CHOOSER (widget),
                                        priv->sort_func,
                                        priv->sort_data,
                                        priv->data_destroy);
    }

  g_signal_connect_swapped (widget, "selection_changed",
                            G_CALLBACK (delegate_selection_changed),
                            recent_action);
  g_signal_connect_swapped (widget, "item-activated",
                            G_CALLBACK (delegate_item_activated),
                            recent_action);

  /* keep track of the choosers we create */
  priv->choosers = g_slist_prepend (priv->choosers, widget);

  return widget;
}

static CtkWidget *
ctk_recent_action_create_menu_item (CtkAction *action)
{
  CtkWidget *menu;
  CtkWidget *menuitem;

  menu = ctk_recent_action_create_menu (action);
  menuitem = g_object_new (CTK_TYPE_IMAGE_MENU_ITEM, NULL);
  ctk_menu_item_set_submenu (CTK_MENU_ITEM (menuitem), menu);
  ctk_widget_show (menu);

  return menuitem;
}

static CtkWidget *
ctk_recent_action_create_tool_item (CtkAction *action)
{
  CtkWidget *menu;
  CtkWidget *toolitem;

  menu = ctk_recent_action_create_menu (action);
  toolitem = g_object_new (CTK_TYPE_MENU_TOOL_BUTTON, NULL);
  ctk_menu_tool_button_set_menu (CTK_MENU_TOOL_BUTTON (toolitem), menu);
  ctk_widget_show (menu);

  return toolitem;
}

static void
set_recent_manager (CtkRecentAction  *action,
                    CtkRecentManager *manager)
{
  CtkRecentActionPrivate *priv = action->priv;

  if (manager)
    priv->manager = manager;
  else
    priv->manager = ctk_recent_manager_get_default ();
}

static void
ctk_recent_action_finalize (GObject *gobject)
{
  CtkRecentAction *action = CTK_RECENT_ACTION (gobject);
  CtkRecentActionPrivate *priv = action->priv;

  if (priv->data_destroy)
    {
      priv->data_destroy (priv->sort_data);
      priv->data_destroy = NULL;
    }

  priv->sort_data = NULL;
  priv->sort_func = NULL;

  g_slist_free (priv->choosers);

  G_OBJECT_CLASS (ctk_recent_action_parent_class)->finalize (gobject);
}

static void
ctk_recent_action_dispose (GObject *gobject)
{
  CtkRecentAction *action = CTK_RECENT_ACTION (gobject);
  CtkRecentActionPrivate *priv = action->priv;

  if (priv->current_filter)
    {
      g_object_unref (priv->current_filter);
      priv->current_filter = NULL;
    }

  priv->manager = NULL;
  
  G_OBJECT_CLASS (ctk_recent_action_parent_class)->dispose (gobject);
}

static void
ctk_recent_action_set_property (GObject      *gobject,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  CtkRecentAction *action = CTK_RECENT_ACTION (gobject);
  CtkRecentActionPrivate *priv = action->priv;

  switch (prop_id)
    {
    case PROP_SHOW_NUMBERS:
      if (priv->show_numbers != g_value_get_boolean (value))
        {
          priv->show_numbers = g_value_get_boolean (value);
          g_object_notify_by_pspec (gobject, pspec);
        }
      break;
    case CTK_RECENT_CHOOSER_PROP_SHOW_PRIVATE:
      if (priv->show_private != g_value_get_boolean (value))
        {
          priv->show_private = g_value_get_boolean (value);
          g_object_notify_by_pspec (gobject, pspec);
        }
      break;
    case CTK_RECENT_CHOOSER_PROP_SHOW_NOT_FOUND:
      if (priv->show_not_found != g_value_get_boolean (value))
        {
          priv->show_not_found = g_value_get_boolean (value);
          g_object_notify_by_pspec (gobject, pspec);
        }
      break;
    case CTK_RECENT_CHOOSER_PROP_SHOW_TIPS:
      if (priv->show_tips != g_value_get_boolean (value))
        {
          priv->show_tips = g_value_get_boolean (value);
          g_object_notify_by_pspec (gobject, pspec);
        }
      break;
    case CTK_RECENT_CHOOSER_PROP_SHOW_ICONS:
      if (priv->show_icons != g_value_get_boolean (value))
        {
          priv->show_icons = g_value_get_boolean (value);
          g_object_notify_by_pspec (gobject, pspec);
        }
      break;
    case CTK_RECENT_CHOOSER_PROP_LIMIT:
      if (priv->limit != g_value_get_int (value))
        {
          priv->limit = g_value_get_int (value);
          g_object_notify_by_pspec (gobject, pspec);
        }
      break;
    case CTK_RECENT_CHOOSER_PROP_LOCAL_ONLY:
      if (priv->local_only != g_value_get_boolean (value))
        {
          priv->local_only = g_value_get_boolean (value);
          g_object_notify_by_pspec (gobject, pspec);
        }
      break;
    case CTK_RECENT_CHOOSER_PROP_SORT_TYPE:
      if (priv->sort_type != g_value_get_enum (value))
        {
          priv->sort_type = g_value_get_enum (value);
          g_object_notify_by_pspec (gobject, pspec);
        }
      break;
    case CTK_RECENT_CHOOSER_PROP_FILTER:
      set_current_filter (action, g_value_get_object (value));
      break;
    case CTK_RECENT_CHOOSER_PROP_SELECT_MULTIPLE:
      g_warning ("%s: Choosers of type `%s' do not support selecting multiple items.",
                 G_STRFUNC,
                 G_OBJECT_TYPE_NAME (gobject));
      return;
    case CTK_RECENT_CHOOSER_PROP_RECENT_MANAGER:
      set_recent_manager (action, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      return;
    }
}

static void
ctk_recent_action_get_property (GObject    *gobject,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  CtkRecentAction *action = CTK_RECENT_ACTION (gobject);
  CtkRecentActionPrivate *priv = action->priv;

  switch (prop_id)
    {
    case PROP_SHOW_NUMBERS:
      g_value_set_boolean (value, priv->show_numbers);
      break;
    case CTK_RECENT_CHOOSER_PROP_SHOW_PRIVATE:
      g_value_set_boolean (value, priv->show_private);
      break;
    case CTK_RECENT_CHOOSER_PROP_SHOW_NOT_FOUND:
      g_value_set_boolean (value, priv->show_not_found);
      break;
    case CTK_RECENT_CHOOSER_PROP_SHOW_TIPS:
      g_value_set_boolean (value, priv->show_tips);
      break;
    case CTK_RECENT_CHOOSER_PROP_SHOW_ICONS:
      g_value_set_boolean (value, priv->show_icons);
      break;
    case CTK_RECENT_CHOOSER_PROP_LIMIT:
      g_value_set_int (value, priv->limit);
      break;
    case CTK_RECENT_CHOOSER_PROP_LOCAL_ONLY:
      g_value_set_boolean (value, priv->local_only);
      break;
    case CTK_RECENT_CHOOSER_PROP_SORT_TYPE:
      g_value_set_enum (value, priv->sort_type);
      break;
    case CTK_RECENT_CHOOSER_PROP_FILTER:
      g_value_set_object (value, priv->current_filter);
      break;
    case CTK_RECENT_CHOOSER_PROP_SELECT_MULTIPLE:
      g_value_set_boolean (value, FALSE);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
ctk_recent_action_class_init (CtkRecentActionClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  CtkActionClass *action_class = CTK_ACTION_CLASS (klass);

  gobject_class->finalize = ctk_recent_action_finalize;
  gobject_class->dispose = ctk_recent_action_dispose;
  gobject_class->set_property = ctk_recent_action_set_property;
  gobject_class->get_property = ctk_recent_action_get_property;

  action_class->activate = ctk_recent_action_activate;
  action_class->connect_proxy = ctk_recent_action_connect_proxy;
  action_class->disconnect_proxy = ctk_recent_action_disconnect_proxy;
  action_class->create_menu_item = ctk_recent_action_create_menu_item;
  action_class->create_tool_item = ctk_recent_action_create_tool_item;
  action_class->create_menu = ctk_recent_action_create_menu;
  action_class->menu_item_type = CTK_TYPE_IMAGE_MENU_ITEM;
  action_class->toolbar_item_type = CTK_TYPE_MENU_TOOL_BUTTON;

  _ctk_recent_chooser_install_properties (gobject_class);

  /**
   * CtkRecentAction:show-numbers:
   *
   * Whether the items should be displayed with a number.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_NUMBERS,
                                   g_param_spec_boolean ("show-numbers",
                                                         P_("Show Numbers"),
                                                         P_("Whether the items should be displayed with a number"),
                                                         FALSE,
                                                         G_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

}

static void
ctk_recent_action_init (CtkRecentAction *action)
{
  CtkRecentActionPrivate *priv;

  action->priv = priv = ctk_recent_action_get_instance_private (action);

  priv->show_numbers = FALSE;
  priv->show_icons = TRUE;
  priv->show_tips = FALSE;
  priv->show_not_found = TRUE;
  priv->show_private = FALSE;
  priv->local_only = TRUE;

  priv->limit = FALLBACK_ITEM_LIMIT;

  priv->sort_type = CTK_RECENT_SORT_NONE;
  priv->sort_func = NULL;
  priv->sort_data = NULL;
  priv->data_destroy = NULL;

  priv->current_filter = NULL;

  priv->manager = NULL;
}

/**
 * ctk_recent_action_new:
 * @name: a unique name for the action
 * @label: (allow-none): the label displayed in menu items and on buttons,
 *   or %NULL
 * @tooltip: (allow-none): a tooltip for the action, or %NULL
 * @stock_id: (allow-none): the stock icon to display in widgets representing
 *   the action, or %NULL
 *
 * Creates a new #CtkRecentAction object. To add the action to
 * a #CtkActionGroup and set the accelerator for the action,
 * call ctk_action_group_add_action_with_accel().
 *
 * Returns: the newly created #CtkRecentAction.
 *
 * Since: 2.12
 */
CtkAction *
ctk_recent_action_new (const gchar *name,
                       const gchar *label,
                       const gchar *tooltip,
                       const gchar *stock_id)
{
  g_return_val_if_fail (name != NULL, NULL);

  return g_object_new (CTK_TYPE_RECENT_ACTION,
                       "name", name,
                       "label", label,
                       "tooltip", tooltip,
                       "stock-id", stock_id,
                       NULL);
}

/**
 * ctk_recent_action_new_for_manager:
 * @name: a unique name for the action
 * @label: (allow-none): the label displayed in menu items and on buttons,
 *   or %NULL
 * @tooltip: (allow-none): a tooltip for the action, or %NULL
 * @stock_id: (allow-none): the stock icon to display in widgets representing
 *   the action, or %NULL
 * @manager: (allow-none): a #CtkRecentManager, or %NULL for using the default
 *   #CtkRecentManager
 *
 * Creates a new #CtkRecentAction object. To add the action to
 * a #CtkActionGroup and set the accelerator for the action,
 * call ctk_action_group_add_action_with_accel().
 *
 * Returns: the newly created #CtkRecentAction
 * 
 * Since: 2.12
 */
CtkAction *
ctk_recent_action_new_for_manager (const gchar      *name,
                                   const gchar      *label,
                                   const gchar      *tooltip,
                                   const gchar      *stock_id,
                                   CtkRecentManager *manager)
{
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (manager == NULL || CTK_IS_RECENT_MANAGER (manager), NULL);

  return g_object_new (CTK_TYPE_RECENT_ACTION,
                       "name", name,
                       "label", label,
                       "tooltip", tooltip,
                       "stock-id", stock_id,
                       "recent-manager", manager,
                       NULL);
}

/**
 * ctk_recent_action_get_show_numbers:
 * @action: a #CtkRecentAction
 *
 * Returns the value set by ctk_recent_chooser_menu_set_show_numbers().
 *
 * Returns: %TRUE if numbers should be shown.
 *
 * Since: 2.12
 */
gboolean
ctk_recent_action_get_show_numbers (CtkRecentAction *action)
{
  g_return_val_if_fail (CTK_IS_RECENT_ACTION (action), FALSE);

  return action->priv->show_numbers;
}

/**
 * ctk_recent_action_set_show_numbers:
 * @action: a #CtkRecentAction
 * @show_numbers: %TRUE if the shown items should be numbered
 *
 * Sets whether a number should be added to the items shown by the
 * widgets representing @action. The numbers are shown to provide
 * a unique character for a mnemonic to be used inside the menu item's
 * label. Only the first ten items get a number to avoid clashes.
 *
 * Since: 2.12
 */
void
ctk_recent_action_set_show_numbers (CtkRecentAction *action,
                                    gboolean         show_numbers)
{
  CtkRecentActionPrivate *priv;

  g_return_if_fail (CTK_IS_RECENT_ACTION (action));

  priv = action->priv;

  if (priv->show_numbers != show_numbers)
    {
      g_object_ref (action);

      priv->show_numbers = show_numbers;

      g_object_notify (G_OBJECT (action), "show-numbers");
      g_object_unref (action);
    }
}
