/* GTK - The GIMP Toolkit
 * ctkrecentchoosermenu.c - Recently used items menu widget
 * Copyright (C) 2005, Emmanuele Bassi
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

#include <string.h>

#include "ctkrecentmanager.h"
#include "ctkrecentfilter.h"
#include "ctkrecentchooser.h"
#include "ctkrecentchooserutils.h"
#include "ctkrecentchooserprivate.h"
#include "ctkrecentchoosermenu.h"

#include "ctkicontheme.h"
#include "ctkintl.h"
#include "ctksettings.h"
#include "ctkmenushell.h"
#include "ctkmenuitem.h"
#include "deprecated/ctkimagemenuitem.h"
#include "ctkseparatormenuitem.h"
#include "ctkmenu.h"
#include "ctkimage.h"
#include "ctklabel.h"
#include "ctktooltip.h"
#include "deprecated/ctkactivatable.h"
#include "ctktypebuiltins.h"
#include "ctkprivate.h"

/**
 * SECTION:ctkrecentchoosermenu
 * @Short_description: Displays recently used files in a menu
 * @Title: GtkRecentChooserMenu
 * @See_also:#GtkRecentChooser
 *
 * #GtkRecentChooserMenu is a widget suitable for displaying recently used files
 * inside a menu.  It can be used to set a sub-menu of a #GtkMenuItem using
 * ctk_menu_item_set_submenu(), or as the menu of a #GtkMenuToolButton.
 *
 * Note that #GtkRecentChooserMenu does not have any methods of its own. Instead,
 * you should use the functions that work on a #GtkRecentChooser.
 *
 * Note also that #GtkRecentChooserMenu does not support multiple filters, as it
 * has no way to let the user choose between them as the #GtkRecentChooserWidget
 * and #GtkRecentChooserDialog widgets do. Thus using ctk_recent_chooser_add_filter()
 * on a #GtkRecentChooserMenu widget will yield the same effects as using
 * ctk_recent_chooser_set_filter(), replacing any currently set filter
 * with the supplied filter; ctk_recent_chooser_remove_filter() will remove
 * any currently set #GtkRecentFilter object and will unset the current filter;
 * ctk_recent_chooser_list_filters() will return a list containing a single
 * #GtkRecentFilter object.
 *
 * Recently used files are supported since GTK+ 2.10.
 */


struct _GtkRecentChooserMenuPrivate
{
  /* the recent manager object */
  GtkRecentManager *manager;
  
  /* max size of the menu item label */
  gint label_width;

  gint first_recent_item_pos;
  GtkWidget *placeholder;

  /* RecentChooser properties */
  gint limit;  
  guint show_private : 1;
  guint show_not_found : 1;
  guint show_tips : 1;
  guint show_icons : 1;
  guint local_only : 1;
  
  guint show_numbers : 1;
  
  GtkRecentSortType sort_type;
  GtkRecentSortFunc sort_func;
  gpointer sort_data;
  GDestroyNotify sort_data_destroy;
  
  GSList *filters;
  GtkRecentFilter *current_filter;
 
  guint local_manager : 1;
  gulong manager_changed_id;

  gulong populate_id;
};

enum {
  PROP_0,
  PROP_SHOW_NUMBERS,

  /* activatable properties */
  PROP_ACTIVATABLE_RELATED_ACTION,
  PROP_ACTIVATABLE_USE_ACTION_APPEARANCE
};


#define FALLBACK_ITEM_LIMIT 	10
#define DEFAULT_LABEL_WIDTH     30


static void     ctk_recent_chooser_menu_finalize    (GObject                   *object);
static void     ctk_recent_chooser_menu_dispose     (GObject                   *object);
static void     ctk_recent_chooser_menu_constructed (GObject                   *object);

static void ctk_recent_chooser_iface_init      (GtkRecentChooserIface     *iface);

static void ctk_recent_chooser_menu_set_property (GObject      *object,
						  guint         prop_id,
						  const GValue *value,
						  GParamSpec   *pspec);
static void ctk_recent_chooser_menu_get_property (GObject      *object,
						  guint         prop_id,
						  GValue       *value,
						  GParamSpec   *pspec);

static gboolean          ctk_recent_chooser_menu_set_current_uri    (GtkRecentChooser  *chooser,
							             const gchar       *uri,
							             GError           **error);
static gchar *           ctk_recent_chooser_menu_get_current_uri    (GtkRecentChooser  *chooser);
static gboolean          ctk_recent_chooser_menu_select_uri         (GtkRecentChooser  *chooser,
								     const gchar       *uri,
								     GError           **error);
static void              ctk_recent_chooser_menu_unselect_uri       (GtkRecentChooser  *chooser,
								     const gchar       *uri);
static void              ctk_recent_chooser_menu_select_all         (GtkRecentChooser  *chooser);
static void              ctk_recent_chooser_menu_unselect_all       (GtkRecentChooser  *chooser);
static GList *           ctk_recent_chooser_menu_get_items          (GtkRecentChooser  *chooser);
static GtkRecentManager *ctk_recent_chooser_menu_get_recent_manager (GtkRecentChooser  *chooser);
static void              ctk_recent_chooser_menu_set_sort_func      (GtkRecentChooser  *chooser,
								     GtkRecentSortFunc  sort_func,
								     gpointer           sort_data,
								     GDestroyNotify     data_destroy);
static void              ctk_recent_chooser_menu_add_filter         (GtkRecentChooser  *chooser,
								     GtkRecentFilter   *filter);
static void              ctk_recent_chooser_menu_remove_filter      (GtkRecentChooser  *chooser,
								     GtkRecentFilter   *filter);
static GSList *          ctk_recent_chooser_menu_list_filters       (GtkRecentChooser  *chooser);
static void              ctk_recent_chooser_menu_set_current_filter (GtkRecentChooserMenu *menu,
								     GtkRecentFilter      *filter);

static void              ctk_recent_chooser_menu_populate           (GtkRecentChooserMenu *menu);
static void              ctk_recent_chooser_menu_set_show_tips      (GtkRecentChooserMenu *menu,
								     gboolean              show_tips);

static void     set_recent_manager (GtkRecentChooserMenu *menu,
				    GtkRecentManager     *manager);

static void     item_activate_cb   (GtkWidget        *widget,
			            gpointer          user_data);
static void     manager_changed_cb (GtkRecentManager *manager,
				    gpointer          user_data);

static void ctk_recent_chooser_activatable_iface_init (GtkActivatableIface  *iface);
static void ctk_recent_chooser_update                 (GtkActivatable       *activatable,
						       GtkAction            *action,
						       const gchar          *property_name);
static void ctk_recent_chooser_sync_action_properties (GtkActivatable       *activatable,
						       GtkAction            *action);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
G_DEFINE_TYPE_WITH_CODE (GtkRecentChooserMenu,
			 ctk_recent_chooser_menu,
			 CTK_TYPE_MENU,
                         G_ADD_PRIVATE (GtkRecentChooserMenu)
			 G_IMPLEMENT_INTERFACE (CTK_TYPE_RECENT_CHOOSER,
				 		ctk_recent_chooser_iface_init)
			 G_IMPLEMENT_INTERFACE (CTK_TYPE_ACTIVATABLE,
				 		ctk_recent_chooser_activatable_iface_init))
G_GNUC_END_IGNORE_DEPRECATIONS;

static void
ctk_recent_chooser_iface_init (GtkRecentChooserIface *iface)
{
  iface->set_current_uri = ctk_recent_chooser_menu_set_current_uri;
  iface->get_current_uri = ctk_recent_chooser_menu_get_current_uri;
  iface->select_uri = ctk_recent_chooser_menu_select_uri;
  iface->unselect_uri = ctk_recent_chooser_menu_unselect_uri;
  iface->select_all = ctk_recent_chooser_menu_select_all;
  iface->unselect_all = ctk_recent_chooser_menu_unselect_all;
  iface->get_items = ctk_recent_chooser_menu_get_items;
  iface->get_recent_manager = ctk_recent_chooser_menu_get_recent_manager;
  iface->set_sort_func = ctk_recent_chooser_menu_set_sort_func;
  iface->add_filter = ctk_recent_chooser_menu_add_filter;
  iface->remove_filter = ctk_recent_chooser_menu_remove_filter;
  iface->list_filters = ctk_recent_chooser_menu_list_filters;
}

static void
ctk_recent_chooser_activatable_iface_init (GtkActivatableIface *iface)
{
  iface->update = ctk_recent_chooser_update;
  iface->sync_action_properties = ctk_recent_chooser_sync_action_properties;
}

static void
ctk_recent_chooser_menu_class_init (GtkRecentChooserMenuClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructed = ctk_recent_chooser_menu_constructed;
  gobject_class->dispose = ctk_recent_chooser_menu_dispose;
  gobject_class->finalize = ctk_recent_chooser_menu_finalize;
  gobject_class->set_property = ctk_recent_chooser_menu_set_property;
  gobject_class->get_property = ctk_recent_chooser_menu_get_property;

  _ctk_recent_chooser_install_properties (gobject_class);

  /**
   * GtkRecentChooserMenu:show-numbers:
   *
   * Whether the first ten items in the menu should be prepended by
   * a number acting as a unique mnemonic.
   *
   * Since: 2.10
   */
  g_object_class_install_property (gobject_class,
		  		   PROP_SHOW_NUMBERS,
				   g_param_spec_boolean ("show-numbers",
							 P_("Show Numbers"),
							 P_("Whether the items should be displayed with a number"),
							 FALSE,
							 CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
  

  g_object_class_override_property (gobject_class, PROP_ACTIVATABLE_RELATED_ACTION, "related-action");
  g_object_class_override_property (gobject_class, PROP_ACTIVATABLE_USE_ACTION_APPEARANCE, "use-action-appearance");
}

static void
ctk_recent_chooser_menu_init (GtkRecentChooserMenu *menu)
{
  GtkRecentChooserMenuPrivate *priv;

  menu->priv = ctk_recent_chooser_menu_get_instance_private (menu);
  priv = menu->priv;

  priv->show_icons= TRUE;
  priv->show_numbers = FALSE;
  priv->show_tips = FALSE;
  priv->show_not_found = TRUE;
  priv->show_private = FALSE;
  priv->local_only = TRUE;
  
  priv->limit = FALLBACK_ITEM_LIMIT;
  priv->sort_type = CTK_RECENT_SORT_NONE;
  priv->label_width = DEFAULT_LABEL_WIDTH;
  
  priv->first_recent_item_pos = -1;
  priv->placeholder = NULL;

  priv->current_filter = NULL;
}

static void
ctk_recent_chooser_menu_finalize (GObject *object)
{
  GtkRecentChooserMenu *menu = CTK_RECENT_CHOOSER_MENU (object);
  GtkRecentChooserMenuPrivate *priv = menu->priv;
  
  priv->manager = NULL;
  
  if (priv->sort_data_destroy)
    {
      priv->sort_data_destroy (priv->sort_data);
      priv->sort_data_destroy = NULL;
    }

  priv->sort_data = NULL;
  priv->sort_func = NULL;
  
  G_OBJECT_CLASS (ctk_recent_chooser_menu_parent_class)->finalize (object);
}

static void
ctk_recent_chooser_menu_dispose (GObject *object)
{
  GtkRecentChooserMenu *menu = CTK_RECENT_CHOOSER_MENU (object);
  GtkRecentChooserMenuPrivate *priv = menu->priv;

  if (priv->manager_changed_id)
    {
      if (priv->manager)
        g_signal_handler_disconnect (priv->manager, priv->manager_changed_id);

      priv->manager_changed_id = 0;
    }
  
  if (priv->populate_id)
    {
      g_source_remove (priv->populate_id);
      priv->populate_id = 0;
    }

  if (priv->current_filter)
    {
      g_object_unref (priv->current_filter);
      priv->current_filter = NULL;
    }
  
  G_OBJECT_CLASS (ctk_recent_chooser_menu_parent_class)->dispose (object);
}

static void
ctk_recent_chooser_menu_constructed (GObject *object)
{
  GtkRecentChooserMenu *menu = CTK_RECENT_CHOOSER_MENU (object);
  GtkRecentChooserMenuPrivate *priv = menu->priv;

  G_OBJECT_CLASS (ctk_recent_chooser_menu_parent_class)->constructed (object);
  
  g_assert (priv->manager);

  /* we create a placeholder menuitem, to be used in case
   * the menu is empty. this placeholder will stay around
   * for the entire lifetime of the menu, and we just hide it
   * when it's not used. we have to do this, and do it here,
   * because we need a marker for the beginning of the recent
   * items list, so that we can insert the new items at the
   * right place when idly populating the menu in case the
   * user appended or prepended custom menu items to the
   * recent chooser menu widget.
   */
  priv->placeholder = ctk_menu_item_new_with_label (_("No items found"));
  ctk_widget_set_sensitive (priv->placeholder, FALSE);
  g_object_set_data (G_OBJECT (priv->placeholder),
                     "ctk-recent-menu-placeholder",
                     GINT_TO_POINTER (TRUE));

  ctk_menu_shell_insert (CTK_MENU_SHELL (menu), priv->placeholder, 0);
  ctk_widget_set_no_show_all (priv->placeholder, TRUE);
  ctk_widget_show (priv->placeholder);

  /* (re)populate the menu */
  ctk_recent_chooser_menu_populate (menu);
}

static void
ctk_recent_chooser_menu_set_property (GObject      *object,
				      guint         prop_id,
				      const GValue *value,
				      GParamSpec   *pspec)
{
  GtkRecentChooserMenu *menu = CTK_RECENT_CHOOSER_MENU (object);
  GtkRecentChooserMenuPrivate *priv = menu->priv;
  
  switch (prop_id)
    {
    case PROP_SHOW_NUMBERS:
      if (priv->show_numbers != g_value_get_boolean (value))
        {
          priv->show_numbers = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case CTK_RECENT_CHOOSER_PROP_RECENT_MANAGER:
      set_recent_manager (menu, g_value_get_object (value));
      break;
    case CTK_RECENT_CHOOSER_PROP_SHOW_PRIVATE:
      if (priv->show_private != g_value_get_boolean (value))
        {
          priv->show_private = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case CTK_RECENT_CHOOSER_PROP_SHOW_NOT_FOUND:
      if (priv->show_not_found != g_value_get_boolean (value))
        {
          priv->show_not_found = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case CTK_RECENT_CHOOSER_PROP_SHOW_TIPS:
      ctk_recent_chooser_menu_set_show_tips (menu, g_value_get_boolean (value));
      break;
    case CTK_RECENT_CHOOSER_PROP_SHOW_ICONS:
      if (priv->show_icons != g_value_get_boolean (value))
        {
          priv->show_icons = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case CTK_RECENT_CHOOSER_PROP_SELECT_MULTIPLE:
      g_warning ("%s: Choosers of type '%s' do not support selecting multiple items.",
                 G_STRFUNC,
                 G_OBJECT_TYPE_NAME (object));
      break;
    case CTK_RECENT_CHOOSER_PROP_LOCAL_ONLY:
      if (priv->local_only != g_value_get_boolean (value))
        {
          priv->local_only = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case CTK_RECENT_CHOOSER_PROP_LIMIT:
      if (priv->limit != g_value_get_int (value))
        {
          priv->limit = g_value_get_int (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case CTK_RECENT_CHOOSER_PROP_SORT_TYPE:
      if (priv->sort_type != g_value_get_enum (value))
        {
          priv->sort_type = g_value_get_enum (value);
          g_object_notify_by_pspec (object, pspec);
        } 
      break;
    case CTK_RECENT_CHOOSER_PROP_FILTER:
      ctk_recent_chooser_menu_set_current_filter (menu, g_value_get_object (value));
      break;
    case PROP_ACTIVATABLE_RELATED_ACTION:
      _ctk_recent_chooser_set_related_action (CTK_RECENT_CHOOSER (menu), g_value_get_object (value));
      break;
    case PROP_ACTIVATABLE_USE_ACTION_APPEARANCE: 
      _ctk_recent_chooser_set_use_action_appearance (CTK_RECENT_CHOOSER (menu), g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_recent_chooser_menu_get_property (GObject    *object,
				      guint       prop_id,
				      GValue     *value,
				      GParamSpec *pspec)
{
  GtkRecentChooserMenu *menu = CTK_RECENT_CHOOSER_MENU (object);
  GtkRecentChooserMenuPrivate *priv = menu->priv;
  
  switch (prop_id)
    {
    case PROP_SHOW_NUMBERS:
      g_value_set_boolean (value, priv->show_numbers);
      break;
    case CTK_RECENT_CHOOSER_PROP_SHOW_TIPS:
      g_value_set_boolean (value, priv->show_tips);
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
    case CTK_RECENT_CHOOSER_PROP_SHOW_PRIVATE:
      g_value_set_boolean (value, priv->show_private);
      break;
    case CTK_RECENT_CHOOSER_PROP_SHOW_NOT_FOUND:
      g_value_set_boolean (value, priv->show_not_found);
      break;
    case CTK_RECENT_CHOOSER_PROP_SHOW_ICONS:
      g_value_set_boolean (value, priv->show_icons);
      break;
    case CTK_RECENT_CHOOSER_PROP_SELECT_MULTIPLE:
      g_value_set_boolean (value, FALSE);
      break;
    case CTK_RECENT_CHOOSER_PROP_FILTER:
      g_value_set_object (value, priv->current_filter);
      break;
    case PROP_ACTIVATABLE_RELATED_ACTION:
      g_value_set_object (value, _ctk_recent_chooser_get_related_action (CTK_RECENT_CHOOSER (menu)));
      break;
    case PROP_ACTIVATABLE_USE_ACTION_APPEARANCE: 
      g_value_set_boolean (value, _ctk_recent_chooser_get_use_action_appearance (CTK_RECENT_CHOOSER (menu)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static gboolean
ctk_recent_chooser_menu_set_current_uri (GtkRecentChooser  *chooser,
					 const gchar       *uri,
					 GError           **error)
{
  GtkRecentChooserMenu *menu = CTK_RECENT_CHOOSER_MENU (chooser);
  GList *children, *l;
  GtkWidget *menu_item = NULL;
  gboolean found = FALSE;
  gint i = 0;
  
  children = ctk_container_get_children (CTK_CONTAINER (menu));
  
  for (l = children; l != NULL; l = l->next, i++)
    {
      GtkRecentInfo *info;
      
      menu_item = CTK_WIDGET (l->data);
      
      info = g_object_get_data (G_OBJECT (menu_item), "ctk-recent-info");
      if (!info)
        continue;
      
      if (strcmp (uri, ctk_recent_info_get_uri (info)) == 0)
        {
          ctk_menu_set_active (CTK_MENU (menu), i);
	  found = TRUE;

	  break;
	}
    }

  g_list_free (children);
  
  if (!found)  
    {
      g_set_error (error, CTK_RECENT_CHOOSER_ERROR,
      		   CTK_RECENT_CHOOSER_ERROR_NOT_FOUND,
                   _("No recently used resource found with URI '%s'"),
      		   uri);
    }
  
  return found;
}

static gchar *
ctk_recent_chooser_menu_get_current_uri (GtkRecentChooser  *chooser)
{
  GtkRecentChooserMenu *menu = CTK_RECENT_CHOOSER_MENU (chooser);
  GtkWidget *menu_item;
  GtkRecentInfo *info;
  
  menu_item = ctk_menu_get_active (CTK_MENU (menu));
  if (!menu_item)
    return NULL;
  
  info = g_object_get_data (G_OBJECT (menu_item), "ctk-recent-info");
  if (!info)
    return NULL;
  
  return g_strdup (ctk_recent_info_get_uri (info));
}

static gboolean
ctk_recent_chooser_menu_select_uri (GtkRecentChooser  *chooser,
				    const gchar       *uri,
				    GError           **error)
{
  GtkRecentChooserMenu *menu = CTK_RECENT_CHOOSER_MENU (chooser);
  GList *children, *l;
  GtkWidget *menu_item = NULL;
  gboolean found = FALSE;
  
  children = ctk_container_get_children (CTK_CONTAINER (menu));
  for (l = children; l != NULL; l = l->next)
    {
      GtkRecentInfo *info;
      
      menu_item = CTK_WIDGET (l->data);
      
      info = g_object_get_data (G_OBJECT (menu_item), "ctk-recent-info");
      if (!info)
        continue;
      
      if (0 == strcmp (uri, ctk_recent_info_get_uri (info)))
        found = TRUE;
    }

  g_list_free (children);
  
  if (!found)  
    {
      g_set_error (error, CTK_RECENT_CHOOSER_ERROR,
      		   CTK_RECENT_CHOOSER_ERROR_NOT_FOUND,
                   _("No recently used resource found with URI '%s'"),
      		   uri);
      return FALSE;
    }
  else
    {
      ctk_menu_shell_select_item (CTK_MENU_SHELL (menu), menu_item);

      return TRUE;
    }
}

static void
ctk_recent_chooser_menu_unselect_uri (GtkRecentChooser *chooser,
				       const gchar     *uri)
{
  GtkRecentChooserMenu *menu = CTK_RECENT_CHOOSER_MENU (chooser);
  
  ctk_menu_shell_deselect (CTK_MENU_SHELL (menu));
}

static void
ctk_recent_chooser_menu_select_all (GtkRecentChooser *chooser)
{
  g_warning ("This function is not implemented for "
	     "widgets of class '%s'",
             g_type_name (G_OBJECT_TYPE (chooser)));
}

static void
ctk_recent_chooser_menu_unselect_all (GtkRecentChooser *chooser)
{
  g_warning ("This function is not implemented for "
	     "widgets of class '%s'",
             g_type_name (G_OBJECT_TYPE (chooser)));
}

static void
ctk_recent_chooser_menu_set_sort_func (GtkRecentChooser  *chooser,
				       GtkRecentSortFunc  sort_func,
				       gpointer           sort_data,
				       GDestroyNotify     data_destroy)
{
  GtkRecentChooserMenu *menu = CTK_RECENT_CHOOSER_MENU (chooser);
  GtkRecentChooserMenuPrivate *priv = menu->priv;
  
  if (priv->sort_data_destroy)
    {
      priv->sort_data_destroy (priv->sort_data);
      priv->sort_data_destroy = NULL;
    }
      
  priv->sort_func = NULL;
  priv->sort_data = NULL;
  priv->sort_data_destroy = NULL;
  
  if (sort_func)
    {
      priv->sort_func = sort_func;
      priv->sort_data = sort_data;
      priv->sort_data_destroy = data_destroy;
    }
}

static GList *
ctk_recent_chooser_menu_get_items (GtkRecentChooser *chooser)
{
  GtkRecentChooserMenu *menu = CTK_RECENT_CHOOSER_MENU (chooser);
  GtkRecentChooserMenuPrivate *priv = menu->priv;

  return _ctk_recent_chooser_get_items (chooser,
                                        priv->current_filter,
                                        priv->sort_func,
                                        priv->sort_data);
}

static GtkRecentManager *
ctk_recent_chooser_menu_get_recent_manager (GtkRecentChooser *chooser)
{
  GtkRecentChooserMenuPrivate *priv;
 
  priv = CTK_RECENT_CHOOSER_MENU (chooser)->priv;
  
  return priv->manager;
}

static void
ctk_recent_chooser_menu_add_filter (GtkRecentChooser *chooser,
				    GtkRecentFilter  *filter)
{
  GtkRecentChooserMenu *menu;

  menu = CTK_RECENT_CHOOSER_MENU (chooser);
  
  ctk_recent_chooser_menu_set_current_filter (menu, filter);
}

static void
ctk_recent_chooser_menu_remove_filter (GtkRecentChooser *chooser,
				       GtkRecentFilter  *filter)
{
  GtkRecentChooserMenu *menu;

  menu = CTK_RECENT_CHOOSER_MENU (chooser);
  
  if (filter == menu->priv->current_filter)
    {
      g_object_unref (menu->priv->current_filter);
      menu->priv->current_filter = NULL;

      g_object_notify (G_OBJECT (menu), "filter");
    }
}

static GSList *
ctk_recent_chooser_menu_list_filters (GtkRecentChooser *chooser)
{
  GtkRecentChooserMenu *menu;
  GSList *retval = NULL;

  menu = CTK_RECENT_CHOOSER_MENU (chooser);
 
  if (menu->priv->current_filter)
    retval = g_slist_prepend (retval, menu->priv->current_filter);

  return retval;
}

static void
ctk_recent_chooser_menu_set_current_filter (GtkRecentChooserMenu *menu,
					    GtkRecentFilter      *filter)
{
  GtkRecentChooserMenuPrivate *priv;

  priv = menu->priv;
  
  if (priv->current_filter)
    g_object_unref (G_OBJECT (priv->current_filter));
  
  priv->current_filter = filter;

  if (priv->current_filter)
    g_object_ref_sink (priv->current_filter);

  ctk_recent_chooser_menu_populate (menu);
  
  g_object_notify (G_OBJECT (menu), "filter");
}

/* taken from libeel/eel-strings.c */
static gchar *
escape_underscores (const gchar *string)
{
  gint underscores;
  const gchar *p;
  gchar *q;
  gchar *escaped;

  if (!string)
    return NULL;
	
  underscores = 0;
  for (p = string; *p != '\0'; p++)
    underscores += (*p == '_');

  if (underscores == 0)
    return g_strdup (string);

  escaped = g_new (char, strlen (string) + underscores + 1);
  for (p = string, q = escaped; *p != '\0'; p++, q++)
    {
      /* Add an extra underscore. */
      if (*p == '_')
        *q++ = '_';
      
      *q = *p;
    }
  
  *q = '\0';
	
  return escaped;
}

static void
ctk_recent_chooser_menu_add_tip (GtkRecentChooserMenu *menu,
				 GtkRecentInfo        *info,
				 GtkWidget            *item)
{
  GtkRecentChooserMenuPrivate *priv;
  gchar *path;

  g_assert (info != NULL);
  g_assert (item != NULL);

  priv = menu->priv;
  
  path = ctk_recent_info_get_uri_display (info);
  if (path)
    {
      gchar *tip_text = g_strdup_printf (_("Open '%s'"), path);

      ctk_widget_set_tooltip_text (item, tip_text);
      ctk_widget_set_has_tooltip (item, priv->show_tips);

      g_free (path);
      g_free (tip_text);
    }
}

static GtkWidget *
ctk_recent_chooser_menu_create_item (GtkRecentChooserMenu *menu,
				     GtkRecentInfo        *info,
				     gint                  count)
{
  GtkRecentChooserMenuPrivate *priv;
  gchar *text;
  GtkWidget *item, *image, *label;
  GIcon *icon;

  g_assert (info != NULL);

  priv = menu->priv;

  if (priv->show_numbers)
    {
      gchar *name, *escaped;
      
      name = g_strdup (ctk_recent_info_get_display_name (info));
      if (!name)
        name = g_strdup (_("Unknown item"));
      
      escaped = escape_underscores (name);
      
      /* avoid clashing mnemonics */
      if (count <= 10)
        /* This is the label format that is used for the first 10 items
         * in a recent files menu. The %d is the number of the item,
         * the %s is the name of the item. Please keep the _ in front
         * of the number to give these menu items a mnemonic.
         */
        text = g_strdup_printf (C_("recent menu label", "_%d. %s"), count, escaped);
      else
        /* This is the format that is used for items in a recent files menu.
         * The %d is the number of the item, the %s is the name of the item.
         */
        text = g_strdup_printf (C_("recent menu label", "%d. %s"), count, escaped);

      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      item = ctk_image_menu_item_new_with_mnemonic (text);
      G_GNUC_END_IGNORE_DEPRECATIONS;

      g_free (escaped);
      g_free (name);
    }
  else
    {
      text = g_strdup (ctk_recent_info_get_display_name (info));
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      item = ctk_image_menu_item_new_with_label (text);
      G_GNUC_END_IGNORE_DEPRECATIONS;
    }

  g_free (text);

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  ctk_image_menu_item_set_always_show_image (CTK_IMAGE_MENU_ITEM (item),
                                             TRUE);
  G_GNUC_END_IGNORE_DEPRECATIONS;

  /* ellipsize the menu item label, in case the recent document
   * display name is huge.
   */
  label = ctk_bin_get_child (CTK_BIN (item));
  if (CTK_IS_LABEL (label))
    {
      ctk_label_set_ellipsize (CTK_LABEL (label), PANGO_ELLIPSIZE_END);
      ctk_label_set_max_width_chars (CTK_LABEL (label), priv->label_width);
    }
  
  if (priv->show_icons)
    {
      icon = ctk_recent_info_get_gicon (info);

      image = ctk_image_new_from_gicon (icon, CTK_ICON_SIZE_MENU);
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      ctk_image_menu_item_set_image (CTK_IMAGE_MENU_ITEM (item), image);
      ctk_image_menu_item_set_always_show_image (CTK_IMAGE_MENU_ITEM (item), TRUE);
      G_GNUC_END_IGNORE_DEPRECATIONS;
      if (icon)
        g_object_unref (icon);
    }

  g_signal_connect (item, "activate",
  		    G_CALLBACK (item_activate_cb),
  		    menu);

  return item;
}

static void
ctk_recent_chooser_menu_insert_item (GtkRecentChooserMenu *menu,
                                     GtkWidget            *menuitem,
                                     gint                  position)
{
  GtkRecentChooserMenuPrivate *priv = menu->priv;
  gint real_position;

  if (priv->first_recent_item_pos == -1)
    {
      GList *children, *l;

      children = ctk_container_get_children (CTK_CONTAINER (menu));

      for (real_position = 0, l = children;
           l != NULL;
           real_position += 1, l = l->next)
        {
          GObject *child = l->data;
          gboolean is_placeholder = FALSE;

          is_placeholder =
            GPOINTER_TO_INT (g_object_get_data (child, "ctk-recent-menu-placeholder"));

          if (is_placeholder)
            break;
        }

      g_list_free (children);
      priv->first_recent_item_pos = real_position;
    }
  else
    real_position = priv->first_recent_item_pos;

  ctk_menu_shell_insert (CTK_MENU_SHELL (menu), menuitem,
                         real_position + position);
  ctk_widget_show (menuitem);
}

/* removes the items we own from the menu */
static void
ctk_recent_chooser_menu_dispose_items (GtkRecentChooserMenu *menu)
{
  GList *children, *l;
 
  children = ctk_container_get_children (CTK_CONTAINER (menu));
  for (l = children; l != NULL; l = l->next)
    {
      GtkWidget *menu_item = CTK_WIDGET (l->data);
      gboolean has_mark = FALSE;
      
      /* check for our mark, in order to remove just the items we own */
      has_mark =
        GPOINTER_TO_INT (g_object_get_data (G_OBJECT (menu_item), "ctk-recent-menu-mark"));

      if (has_mark)
        {
          GtkRecentInfo *info;
          
          /* destroy the attached RecentInfo struct, if found */
          info = g_object_get_data (G_OBJECT (menu_item), "ctk-recent-info");
          if (info)
            g_object_set_data_full (G_OBJECT (menu_item), "ctk-recent-info",
            			    NULL, NULL);
          
          /* and finally remove the item from the menu */
          ctk_container_remove (CTK_CONTAINER (menu), menu_item);
        }
    }

  /* recalculate the position of the first recent item */
  menu->priv->first_recent_item_pos = -1;

  g_list_free (children);
}

typedef struct
{
  GList *items;
  gint n_items;
  gint loaded_items;
  gint displayed_items;
  GtkRecentChooserMenu *menu;
  GtkWidget *placeholder;
} MenuPopulateData;

static MenuPopulateData *
create_menu_populate_data (GtkRecentChooserMenu *menu)
{
  MenuPopulateData *pdata;

  pdata = g_slice_new (MenuPopulateData);
  pdata->items = NULL;
  pdata->n_items = 0;
  pdata->loaded_items = 0;
  pdata->displayed_items = 0;
  pdata->menu = menu;
  pdata->placeholder = g_object_ref (menu->priv->placeholder);

  return pdata;
}

static void
free_menu_populate_data (MenuPopulateData *pdata)
{
  if (pdata->placeholder)
    g_object_unref (pdata->placeholder);
  g_slice_free (MenuPopulateData, pdata);
}

static gboolean
idle_populate_func (gpointer data)
{
  MenuPopulateData *pdata;
  GtkRecentChooserMenuPrivate *priv;
  GtkRecentInfo *info;
  gboolean retval;
  GtkWidget *item;

  pdata = (MenuPopulateData *) data;
  priv = pdata->menu->priv;

  if (!pdata->items)
    {
      pdata->items = ctk_recent_chooser_get_items (CTK_RECENT_CHOOSER (pdata->menu));
      if (!pdata->items)
        {
          /* show the placeholder here */
          ctk_widget_show (pdata->placeholder);
          pdata->displayed_items = 1;
          priv->populate_id = 0;

	  return FALSE;
	}
      else
        ctk_widget_hide (pdata->placeholder);
      
      pdata->n_items = g_list_length (pdata->items);
      pdata->loaded_items = 0;
    }

  info = g_list_nth_data (pdata->items, pdata->loaded_items);
  item = ctk_recent_chooser_menu_create_item (pdata->menu,
                                              info,
					      pdata->displayed_items);
  if (!item)
    goto check_and_return;
      
  ctk_recent_chooser_menu_add_tip (pdata->menu, info, item);
  ctk_recent_chooser_menu_insert_item (pdata->menu, item,
                                       pdata->displayed_items);
  
  pdata->displayed_items += 1;
      
  /* mark the menu item as one of our own */
  g_object_set_data (G_OBJECT (item),
                     "ctk-recent-menu-mark",
      		     GINT_TO_POINTER (TRUE));
      
  /* attach the RecentInfo object to the menu item, and own a reference
   * to it, so that it will be destroyed with the menu item when it's
   * not needed anymore.
   */
  g_object_set_data_full (G_OBJECT (item), "ctk-recent-info",
      			  ctk_recent_info_ref (info),
      			  (GDestroyNotify) ctk_recent_info_unref);
  
check_and_return:
  pdata->loaded_items += 1;

  if (pdata->loaded_items == pdata->n_items)
    {
      g_list_free_full (pdata->items, (GDestroyNotify) ctk_recent_info_unref);

      priv->populate_id = 0;

      retval = FALSE;
    }
  else
    retval = TRUE;

  return retval;
}

static void
idle_populate_clean_up (gpointer data)
{
  MenuPopulateData *pdata = data;

  if (pdata->menu->priv->populate_id == 0)
    {
      /* show the placeholder in case no item survived
       * the filtering process in the idle loop
       */
      if (!pdata->displayed_items)
        ctk_widget_show (pdata->placeholder);
    }

  free_menu_populate_data (pdata);
}

static void
ctk_recent_chooser_menu_populate (GtkRecentChooserMenu *menu)
{
  MenuPopulateData *pdata;
  GtkRecentChooserMenuPrivate *priv = menu->priv;

  if (priv->populate_id)
    return;

  pdata = create_menu_populate_data (menu);

  /* remove our menu items first */
  ctk_recent_chooser_menu_dispose_items (menu);

  priv->populate_id = gdk_threads_add_idle_full (G_PRIORITY_HIGH_IDLE + 30,
  					         idle_populate_func,
					         pdata,
                                                 idle_populate_clean_up);
  g_source_set_name_by_id (priv->populate_id, "[ctk+] idle_populate_func");
}

/* bounce activate signal from the recent menu item widget 
 * to the recent menu widget
 */
static void
item_activate_cb (GtkWidget *widget,
		  gpointer   user_data)
{
  GtkRecentChooser *chooser = CTK_RECENT_CHOOSER (user_data);
  GtkRecentInfo *info = g_object_get_data (G_OBJECT (widget), "ctk-recent-info");

  ctk_recent_chooser_menu_set_current_uri (chooser, ctk_recent_info_get_uri (info), NULL);
  _ctk_recent_chooser_item_activated (chooser);
}

/* we force a redraw if the manager changes when we are showing */
static void
manager_changed_cb (GtkRecentManager *manager,
		    gpointer          user_data)
{
  GtkRecentChooserMenu *menu = CTK_RECENT_CHOOSER_MENU (user_data);

  ctk_recent_chooser_menu_populate (menu);
}

static void
set_recent_manager (GtkRecentChooserMenu *menu,
		    GtkRecentManager     *manager)
{
  GtkRecentChooserMenuPrivate *priv = menu->priv;

  if (priv->manager)
    {
      if (priv->manager_changed_id)
        {
          g_signal_handler_disconnect (priv->manager, priv->manager_changed_id);
          priv->manager_changed_id = 0;
        }

      if (priv->populate_id)
        {
          g_source_remove (priv->populate_id);
          priv->populate_id = 0;
        }

      priv->manager = NULL;
    }
  
  if (manager)
    priv->manager = manager;
  else
    priv->manager = ctk_recent_manager_get_default ();
  
  if (priv->manager)
    priv->manager_changed_id = g_signal_connect (priv->manager, "changed",
                                                 G_CALLBACK (manager_changed_cb),
                                                 menu);
}

static void
foreach_set_shot_tips (GtkWidget *widget,
                       gpointer   user_data)
{
  GtkRecentChooserMenu *menu = user_data;
  GtkRecentChooserMenuPrivate *priv = menu->priv;
  gboolean has_mark;

  /* toggle the tooltip only on the items we create */
  has_mark =
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "ctk-recent-menu-mark"));

  if (has_mark)
    ctk_widget_set_has_tooltip (widget, priv->show_tips);
}

static void
ctk_recent_chooser_menu_set_show_tips (GtkRecentChooserMenu *menu,
				       gboolean              show_tips)
{
  GtkRecentChooserMenuPrivate *priv = menu->priv;

  if (priv->show_tips == show_tips)
    return;
  
  priv->show_tips = show_tips;
  ctk_container_foreach (CTK_CONTAINER (menu), foreach_set_shot_tips, menu);
  g_object_notify (G_OBJECT (menu), "show-tips");
}

static void
ctk_recent_chooser_update (GtkActivatable *activatable,
			   GtkAction      *action,
			   const gchar    *property_name)
{
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;

  if (strcmp (property_name, "sensitive") == 0)
    ctk_widget_set_sensitive (CTK_WIDGET (activatable), ctk_action_is_sensitive (action));

  G_GNUC_END_IGNORE_DEPRECATIONS;

  _ctk_recent_chooser_update (activatable, action, property_name);
}

static void
ctk_recent_chooser_sync_action_properties (GtkActivatable *activatable,
				           GtkAction      *action)
{
  if (!action)
    return;

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;

  ctk_widget_set_sensitive (CTK_WIDGET (activatable), ctk_action_is_sensitive (action));

  G_GNUC_END_IGNORE_DEPRECATIONS;

  _ctk_recent_chooser_sync_action_properties (activatable, action);
}


/*
 * Public API
 */

/**
 * ctk_recent_chooser_menu_new:
 *
 * Creates a new #GtkRecentChooserMenu widget.
 *
 * This kind of widget shows the list of recently used resources as
 * a menu, each item as a menu item.  Each item inside the menu might
 * have an icon, representing its MIME type, and a number, for mnemonic
 * access.
 *
 * This widget implements the #GtkRecentChooser interface.
 *
 * This widget creates its own #GtkRecentManager object.  See the
 * ctk_recent_chooser_menu_new_for_manager() function to know how to create
 * a #GtkRecentChooserMenu widget bound to another #GtkRecentManager object.
 *
 * Returns: a new #GtkRecentChooserMenu
 *
 * Since: 2.10
 */
GtkWidget *
ctk_recent_chooser_menu_new (void)
{
  return g_object_new (CTK_TYPE_RECENT_CHOOSER_MENU,
  		       "recent-manager", NULL,
  		       NULL);
}

/**
 * ctk_recent_chooser_menu_new_for_manager:
 * @manager: a #GtkRecentManager
 *
 * Creates a new #GtkRecentChooserMenu widget using @manager as
 * the underlying recently used resources manager.
 *
 * This is useful if you have implemented your own recent manager,
 * or if you have a customized instance of a #GtkRecentManager
 * object or if you wish to share a common #GtkRecentManager object
 * among multiple #GtkRecentChooser widgets.
 *
 * Returns: a new #GtkRecentChooserMenu, bound to @manager.
 *
 * Since: 2.10
 */
GtkWidget *
ctk_recent_chooser_menu_new_for_manager (GtkRecentManager *manager)
{
  g_return_val_if_fail (manager == NULL || CTK_IS_RECENT_MANAGER (manager), NULL);
  
  return g_object_new (CTK_TYPE_RECENT_CHOOSER_MENU,
  		       "recent-manager", manager,
  		       NULL);
}

/**
 * ctk_recent_chooser_menu_get_show_numbers:
 * @menu: a #GtkRecentChooserMenu
 *
 * Returns the value set by ctk_recent_chooser_menu_set_show_numbers().
 * 
 * Returns: %TRUE if numbers should be shown.
 *
 * Since: 2.10
 */
gboolean
ctk_recent_chooser_menu_get_show_numbers (GtkRecentChooserMenu *menu)
{
  g_return_val_if_fail (CTK_IS_RECENT_CHOOSER_MENU (menu), FALSE);

  return menu->priv->show_numbers;
}

/**
 * ctk_recent_chooser_menu_set_show_numbers:
 * @menu: a #GtkRecentChooserMenu
 * @show_numbers: whether to show numbers
 *
 * Sets whether a number should be added to the items of @menu.  The
 * numbers are shown to provide a unique character for a mnemonic to
 * be used inside ten menu item’s label.  Only the first the items
 * get a number to avoid clashes.
 *
 * Since: 2.10
 */
void
ctk_recent_chooser_menu_set_show_numbers (GtkRecentChooserMenu *menu,
					  gboolean              show_numbers)
{
  g_return_if_fail (CTK_IS_RECENT_CHOOSER_MENU (menu));

  if (menu->priv->show_numbers == show_numbers)
    return;

  menu->priv->show_numbers = show_numbers;
  g_object_notify (G_OBJECT (menu), "show-numbers");
}
