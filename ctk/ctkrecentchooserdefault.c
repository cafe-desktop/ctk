/* CTK - The GIMP Toolkit
 * ctkrecentchooserdefault.c
 * Copyright (C) 2005-2006, Emmanuele Bassi
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
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "ctkicontheme.h"
#include "ctksettings.h"
#include "ctktreeview.h"
#include "ctkliststore.h"
#include "ctkbutton.h"
#include "ctkcelllayout.h"
#include "ctkcellrendererpixbuf.h"
#include "ctkcellrenderertext.h"
#include "ctkcheckmenuitem.h"
#include "ctkclipboard.h"
#include "ctkcomboboxtext.h"
#include "ctkcssiconthemevalueprivate.h"
#include "ctkdragsource.h"
#include "ctkentry.h"
#include "ctkeventbox.h"
#include "ctkexpander.h"
#include "ctkframe.h"
#include "ctkbox.h"
#include "ctkpaned.h"
#include "ctkimage.h"
#include "ctkintl.h"
#include "ctklabel.h"
#include "ctkmenuitem.h"
#include "ctkmessagedialog.h"
#include "ctkscrolledwindow.h"
#include "ctkseparatormenuitem.h"
#include "ctksizegroup.h"
#include "ctksizerequest.h"
#include "ctkstylecontextprivate.h"
#include "ctktreemodelsort.h"
#include "ctktreemodelfilter.h"
#include "ctktreeselection.h"
#include "ctktreestore.h"
#include "ctktooltip.h"
#include "ctktypebuiltins.h"
#include "ctkorientable.h"
#include "ctkwindowgroup.h"
#include "ctkactivatable.h"

#include "ctkrecentmanager.h"
#include "ctkrecentfilter.h"
#include "ctkrecentchooser.h"
#include "ctkrecentchooserprivate.h"
#include "ctkrecentchooserutils.h"
#include "ctkrecentchooserdefault.h"


enum 
{
  PROP_0,

  /* activatable properties */
  PROP_ACTIVATABLE_RELATED_ACTION,
  PROP_ACTIVATABLE_USE_ACTION_APPEARANCE
};

typedef struct
{
  CtkRecentManager *manager;
  gulong manager_changed_id;
  guint local_manager : 1;

  gint icon_size;

  /* RecentChooser properties */
  gint limit;
  CtkRecentSortType sort_type;
  guint show_private : 1;
  guint show_not_found : 1;
  guint select_multiple : 1;
  guint show_tips : 1;
  guint show_icons : 1;
  guint local_only : 1;

  guint limit_set : 1;
  
  GSList *filters;
  CtkRecentFilter *current_filter;
  CtkWidget *filter_combo_hbox;
  CtkWidget *filter_combo;
  
  CtkRecentSortFunc sort_func;
  gpointer sort_data;
  GDestroyNotify sort_data_destroy;

  CtkIconTheme *icon_theme;
  
  CtkWidget *recent_view;
  CtkListStore *recent_store;
  CtkTreeViewColumn *icon_column;
  CtkTreeViewColumn *meta_column;
  CtkCellRenderer *icon_renderer;
  CtkCellRenderer *meta_renderer;
  CtkTreeSelection *selection;
  
  CtkWidget *recent_popup_menu;
  CtkWidget *recent_popup_menu_copy_item;
  CtkWidget *recent_popup_menu_remove_item;
  CtkWidget *recent_popup_menu_clear_item;
  CtkWidget *recent_popup_menu_show_private_item;
 
  guint load_id;
  GList *recent_items;
  gint n_recent_items;
  gint loaded_items;
  guint load_state;
} CtkRecentChooserDefaultPrivate;

struct _CtkRecentChooserDefault
{
  CtkBox parent_instance;

  CtkRecentChooserDefaultPrivate *priv;
};

typedef struct _CtkRecentChooserDefaultClass
{
  CtkBoxClass parent_class;
} CtkRecentChooserDefaultClass;

/* Keep inline with CtkTreeStore defined in ctkrecentchooserdefault.ui */
enum {
  RECENT_URI_COLUMN,
  RECENT_DISPLAY_NAME_COLUMN,
  RECENT_INFO_COLUMN,
  N_RECENT_COLUMNS
};

enum {
  LOAD_EMPTY,    /* initial state: the model is empty */
  LOAD_PRELOAD,  /* the model is loading and not inserted in the tree yet */
  LOAD_LOADING,  /* the model is fully loaded but not inserted */
  LOAD_FINISHED  /* the model is fully loaded and inserted */
};

/* Icon size for if we can't get it from the theme */
#define FALLBACK_ICON_SIZE  48
#define FALLBACK_ITEM_LIMIT 20

#define NUM_CHARS 40
#define NUM_LINES 9

#define DEFAULT_RECENT_FILES_LIMIT 50



/* GObject */
static void     _ctk_recent_chooser_default_class_init  (CtkRecentChooserDefaultClass *klass);
static void     _ctk_recent_chooser_default_init        (CtkRecentChooserDefault      *impl);
static void     ctk_recent_chooser_default_finalize     (GObject                      *object);
static void     ctk_recent_chooser_default_dispose      (GObject                      *object);
static void     ctk_recent_chooser_default_set_property (GObject                      *object,
						         guint                         prop_id,
						         const GValue                 *value,
						         GParamSpec                   *pspec);
static void     ctk_recent_chooser_default_get_property (GObject                      *object,
						         guint                         prop_id,
						         GValue                       *value,
						         GParamSpec                   *pspec);

/* CtkRecentChooserIface */
static void              ctk_recent_chooser_iface_init                 (CtkRecentChooserIface  *iface);
static gboolean          ctk_recent_chooser_default_set_current_uri    (CtkRecentChooser       *chooser,
								        const gchar            *uri,
								        GError                **error);
static gchar *           ctk_recent_chooser_default_get_current_uri    (CtkRecentChooser       *chooser);
static gboolean          ctk_recent_chooser_default_select_uri         (CtkRecentChooser       *chooser,
								        const gchar            *uri,
								        GError                **error);
static void              ctk_recent_chooser_default_unselect_uri       (CtkRecentChooser       *chooser,
								        const gchar            *uri);
static void              ctk_recent_chooser_default_select_all         (CtkRecentChooser       *chooser);
static void              ctk_recent_chooser_default_unselect_all       (CtkRecentChooser       *chooser);
static GList *           ctk_recent_chooser_default_get_items          (CtkRecentChooser       *chooser);
static CtkRecentManager *ctk_recent_chooser_default_get_recent_manager (CtkRecentChooser       *chooser);
static void              ctk_recent_chooser_default_set_sort_func      (CtkRecentChooser       *chooser,
									CtkRecentSortFunc       sort_func,
									gpointer                sort_data,
									GDestroyNotify          data_destroy);
static void              ctk_recent_chooser_default_add_filter         (CtkRecentChooser       *chooser,
								        CtkRecentFilter        *filter);
static void              ctk_recent_chooser_default_remove_filter      (CtkRecentChooser       *chooser,
								        CtkRecentFilter        *filter);
static GSList *          ctk_recent_chooser_default_list_filters       (CtkRecentChooser       *chooser);


static void ctk_recent_chooser_default_map      (CtkWidget *widget);
static void ctk_recent_chooser_default_show_all (CtkWidget *widget);

static void set_current_filter        (CtkRecentChooserDefault *impl,
				       CtkRecentFilter         *filter);

static CtkIconTheme *get_icon_theme_for_widget (CtkWidget   *widget);
static gint          get_icon_size_for_widget  (CtkWidget   *widget,
						CtkIconSize  icon_size);

static void reload_recent_items (CtkRecentChooserDefault *impl);
static void chooser_set_model   (CtkRecentChooserDefault *impl);

static void set_recent_manager (CtkRecentChooserDefault *impl,
				CtkRecentManager        *manager);

static void chooser_set_sort_type (CtkRecentChooserDefault *impl,
				   CtkRecentSortType        sort_type);

static void recent_manager_changed_cb (CtkRecentManager  *manager,
			               gpointer           user_data);
static void recent_icon_data_func     (CtkTreeViewColumn *tree_column,
				       CtkCellRenderer   *cell,
				       CtkTreeModel      *model,
				       CtkTreeIter       *iter,
				       gpointer           user_data);
static void recent_meta_data_func     (CtkTreeViewColumn *tree_column,
				       CtkCellRenderer   *cell,
				       CtkTreeModel      *model,
				       CtkTreeIter       *iter,
				       gpointer           user_data);

static void selection_changed_cb      (CtkTreeSelection  *z,
				       gpointer           user_data);
static void row_activated_cb          (CtkTreeView       *tree_view,
				       CtkTreePath       *tree_path,
				       CtkTreeViewColumn *tree_column,
				       gpointer           user_data);
static void filter_combo_changed_cb   (CtkComboBox       *combo_box,
				       gpointer           user_data);

static void remove_all_activated_cb   (CtkMenuItem       *menu_item,
				       gpointer           user_data);
static void remove_item_activated_cb  (CtkMenuItem       *menu_item,
				       gpointer           user_data);
static void show_private_toggled_cb   (CtkCheckMenuItem  *menu_item,
				       gpointer           user_data);

static gboolean recent_view_popup_menu_cb   (CtkWidget      *widget,
					     gpointer        user_data);
static gboolean recent_view_button_press_cb (CtkWidget      *widget,
					     CdkEventButton *event,
					     gpointer        user_data);

static void     recent_view_drag_begin_cb         (CtkWidget        *widget,
						   CdkDragContext   *context,
						   gpointer          user_data);
static void     recent_view_drag_data_get_cb      (CtkWidget        *widget,
						   CdkDragContext   *context,
						   CtkSelectionData *selection_data,
						   guint             info,
						   guint32           time_,
						   gpointer          data);
static gboolean recent_view_query_tooltip_cb      (CtkWidget        *widget,
                                                   gint              x,
                                                   gint              y,
                                                   gboolean          keyboard_tip,
                                                   CtkTooltip       *tooltip,
                                                   gpointer          user_data);

static void ctk_recent_chooser_activatable_iface_init (CtkActivatableIface  *iface);
static void ctk_recent_chooser_update                 (CtkActivatable       *activatable,
						       CtkAction            *action,
						       const gchar          *property_name);
static void ctk_recent_chooser_sync_action_properties (CtkActivatable       *activatable,
						       CtkAction            *action);

G_DEFINE_TYPE_WITH_CODE (CtkRecentChooserDefault,
			 _ctk_recent_chooser_default,
			 CTK_TYPE_BOX,
                         G_ADD_PRIVATE (CtkRecentChooserDefault)
			 G_IMPLEMENT_INTERFACE (CTK_TYPE_RECENT_CHOOSER,
				 		ctk_recent_chooser_iface_init)
			 G_IMPLEMENT_INTERFACE (CTK_TYPE_ACTIVATABLE,
				 		ctk_recent_chooser_activatable_iface_init))


static void
ctk_recent_chooser_iface_init (CtkRecentChooserIface *iface)
{
  iface->set_current_uri = ctk_recent_chooser_default_set_current_uri;
  iface->get_current_uri = ctk_recent_chooser_default_get_current_uri;
  iface->select_uri = ctk_recent_chooser_default_select_uri;
  iface->unselect_uri = ctk_recent_chooser_default_unselect_uri;
  iface->select_all = ctk_recent_chooser_default_select_all;
  iface->unselect_all = ctk_recent_chooser_default_unselect_all;
  iface->get_items = ctk_recent_chooser_default_get_items;
  iface->get_recent_manager = ctk_recent_chooser_default_get_recent_manager;
  iface->set_sort_func = ctk_recent_chooser_default_set_sort_func;
  iface->add_filter = ctk_recent_chooser_default_add_filter;
  iface->remove_filter = ctk_recent_chooser_default_remove_filter;
  iface->list_filters = ctk_recent_chooser_default_list_filters;
}

static void
ctk_recent_chooser_activatable_iface_init (CtkActivatableIface *iface)

{
  iface->update = ctk_recent_chooser_update;
  iface->sync_action_properties = ctk_recent_chooser_sync_action_properties;
}

static void
_ctk_recent_chooser_default_class_init (CtkRecentChooserDefaultClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);

  gobject_class->set_property = ctk_recent_chooser_default_set_property;
  gobject_class->get_property = ctk_recent_chooser_default_get_property;
  gobject_class->dispose = ctk_recent_chooser_default_dispose;
  gobject_class->finalize = ctk_recent_chooser_default_finalize;
  
  widget_class->map = ctk_recent_chooser_default_map;
  widget_class->show_all = ctk_recent_chooser_default_show_all;
  
  _ctk_recent_chooser_install_properties (gobject_class);

  g_object_class_override_property (gobject_class, PROP_ACTIVATABLE_RELATED_ACTION, "related-action");
  g_object_class_override_property (gobject_class, PROP_ACTIVATABLE_USE_ACTION_APPEARANCE, "use-action-appearance");

  /* Bind class to template
   */
  ctk_widget_class_set_template_from_resource (widget_class,
					       "/org/ctk/libctk/ui/ctkrecentchooserdefault.ui");

  ctk_widget_class_bind_template_child_private (widget_class, CtkRecentChooserDefault, filter_combo_hbox);
  ctk_widget_class_bind_template_child_private (widget_class, CtkRecentChooserDefault, filter_combo);
  ctk_widget_class_bind_template_child_private (widget_class, CtkRecentChooserDefault, recent_view);
  ctk_widget_class_bind_template_child_private (widget_class, CtkRecentChooserDefault, recent_store);
  ctk_widget_class_bind_template_child_private (widget_class, CtkRecentChooserDefault, icon_column);
  ctk_widget_class_bind_template_child_private (widget_class, CtkRecentChooserDefault, meta_column);
  ctk_widget_class_bind_template_child_private (widget_class, CtkRecentChooserDefault, icon_renderer);
  ctk_widget_class_bind_template_child_private (widget_class, CtkRecentChooserDefault, meta_renderer);
  ctk_widget_class_bind_template_child_private (widget_class, CtkRecentChooserDefault, selection);

  ctk_widget_class_bind_template_callback (widget_class, selection_changed_cb);
  ctk_widget_class_bind_template_callback (widget_class, row_activated_cb);
  ctk_widget_class_bind_template_callback (widget_class, filter_combo_changed_cb);
  ctk_widget_class_bind_template_callback (widget_class, recent_view_popup_menu_cb);
  ctk_widget_class_bind_template_callback (widget_class, recent_view_button_press_cb);
  ctk_widget_class_bind_template_callback (widget_class, recent_view_drag_begin_cb);
  ctk_widget_class_bind_template_callback (widget_class, recent_view_drag_data_get_cb);
  ctk_widget_class_bind_template_callback (widget_class, recent_view_query_tooltip_cb);
}

static void
_ctk_recent_chooser_default_init (CtkRecentChooserDefault *impl)
{
  CtkRecentChooserDefaultPrivate *priv;

  impl->priv = priv = _ctk_recent_chooser_default_get_instance_private (impl);

  /* by default, we use the global manager */
  priv->local_manager = FALSE;

  priv->limit = FALLBACK_ITEM_LIMIT;
  priv->sort_type = CTK_RECENT_SORT_NONE;

  priv->show_icons = TRUE;
  priv->show_private = FALSE;
  priv->show_not_found = TRUE;
  priv->show_tips = FALSE;
  priv->select_multiple = FALSE;
  priv->local_only = TRUE;
  
  priv->icon_size = FALLBACK_ICON_SIZE;
  priv->icon_theme = NULL;
  
  priv->current_filter = NULL;

  priv->recent_items = NULL;
  priv->n_recent_items = 0;
  priv->loaded_items = 0;
  
  priv->load_state = LOAD_EMPTY;

  ctk_widget_init_template (CTK_WIDGET (impl));

  g_object_set_data (G_OBJECT (priv->recent_view),
                     "CtkRecentChooserDefault", impl);

  ctk_tree_view_column_set_cell_data_func (priv->icon_column,
  					   priv->icon_renderer,
  					   recent_icon_data_func,
  					   impl,
  					   NULL);
  ctk_tree_view_column_set_cell_data_func (priv->meta_column,
  					   priv->meta_renderer,
  					   recent_meta_data_func,
  					   impl,
  					   NULL);
  ctk_drag_source_set (priv->recent_view,
		       CDK_BUTTON1_MASK,
		       NULL, 0,
		       CDK_ACTION_COPY);
  ctk_drag_source_add_uri_targets (priv->recent_view);
}

static void
ctk_recent_chooser_default_set_property (GObject      *object,
				         guint         prop_id,
					 const GValue *value,
					 GParamSpec   *pspec)
{
  CtkRecentChooserDefault *impl = CTK_RECENT_CHOOSER_DEFAULT (object);
  
  switch (prop_id)
    {
    case CTK_RECENT_CHOOSER_PROP_RECENT_MANAGER:
      set_recent_manager (impl, g_value_get_object (value));
      break;
    case CTK_RECENT_CHOOSER_PROP_SHOW_PRIVATE:
      if (impl->priv->show_private != g_value_get_boolean (value))
        {
          impl->priv->show_private = g_value_get_boolean (value);
          if (impl->priv->recent_popup_menu_show_private_item)
            {
              CtkCheckMenuItem *item = CTK_CHECK_MENU_ITEM (impl->priv->recent_popup_menu_show_private_item);
              g_signal_handlers_block_by_func (item, G_CALLBACK (show_private_toggled_cb), impl);
              ctk_check_menu_item_set_active (item, impl->priv->show_private);
              g_signal_handlers_unblock_by_func (item, G_CALLBACK (show_private_toggled_cb), impl);
            }
          reload_recent_items (impl);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case CTK_RECENT_CHOOSER_PROP_SHOW_NOT_FOUND:
      if (impl->priv->show_not_found != g_value_get_boolean (value))
        {
          impl->priv->show_not_found = g_value_get_boolean (value);
          reload_recent_items (impl);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case CTK_RECENT_CHOOSER_PROP_SHOW_TIPS:
      if (impl->priv->show_tips != g_value_get_boolean (value))
        {
          impl->priv->show_tips = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case CTK_RECENT_CHOOSER_PROP_SHOW_ICONS:
      if (impl->priv->show_icons != g_value_get_boolean (value))
        {
          impl->priv->show_icons = g_value_get_boolean (value);
          ctk_tree_view_column_set_visible (impl->priv->icon_column, impl->priv->show_icons);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case CTK_RECENT_CHOOSER_PROP_SELECT_MULTIPLE:
      if (impl->priv->select_multiple != g_value_get_boolean (value))
        {
          impl->priv->select_multiple = g_value_get_boolean (value);
          if (impl->priv->select_multiple)
            ctk_tree_selection_set_mode (impl->priv->selection, CTK_SELECTION_MULTIPLE);
          else
            ctk_tree_selection_set_mode (impl->priv->selection, CTK_SELECTION_SINGLE);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case CTK_RECENT_CHOOSER_PROP_LOCAL_ONLY:
      if (impl->priv->local_only != g_value_get_boolean (value))
        {
          impl->priv->local_only = g_value_get_boolean (value);
          reload_recent_items (impl);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case CTK_RECENT_CHOOSER_PROP_LIMIT:
      if (impl->priv->limit != g_value_get_int (value))
        {
          impl->priv->limit = g_value_get_int (value);
          impl->priv->limit_set = TRUE;
          reload_recent_items (impl);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case CTK_RECENT_CHOOSER_PROP_SORT_TYPE:
      chooser_set_sort_type (impl, g_value_get_enum (value));
      break;
    case CTK_RECENT_CHOOSER_PROP_FILTER:
      set_current_filter (impl, g_value_get_object (value));
      break;
    case PROP_ACTIVATABLE_RELATED_ACTION:
      _ctk_recent_chooser_set_related_action (CTK_RECENT_CHOOSER (impl), g_value_get_object (value));
      break;
    case PROP_ACTIVATABLE_USE_ACTION_APPEARANCE: 
      _ctk_recent_chooser_set_use_action_appearance (CTK_RECENT_CHOOSER (impl), g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_recent_chooser_default_get_property (GObject    *object,
					 guint       prop_id,
					 GValue     *value,
					 GParamSpec *pspec)
{
  CtkRecentChooserDefault *impl = CTK_RECENT_CHOOSER_DEFAULT (object);
  
  switch (prop_id)
    {
    case CTK_RECENT_CHOOSER_PROP_LIMIT:
      g_value_set_int (value, impl->priv->limit);
      break;
    case CTK_RECENT_CHOOSER_PROP_SORT_TYPE:
      g_value_set_enum (value, impl->priv->sort_type);
      break;
    case CTK_RECENT_CHOOSER_PROP_SHOW_PRIVATE:
      g_value_set_boolean (value, impl->priv->show_private);
      break;
    case CTK_RECENT_CHOOSER_PROP_SHOW_ICONS:
      g_value_set_boolean (value, impl->priv->show_icons);
      break;
    case CTK_RECENT_CHOOSER_PROP_SHOW_NOT_FOUND:
      g_value_set_boolean (value, impl->priv->show_not_found);
      break;
    case CTK_RECENT_CHOOSER_PROP_SHOW_TIPS:
      g_value_set_boolean (value, impl->priv->show_tips);
      break;
    case CTK_RECENT_CHOOSER_PROP_LOCAL_ONLY:
      g_value_set_boolean (value, impl->priv->local_only);
      break;
    case CTK_RECENT_CHOOSER_PROP_SELECT_MULTIPLE:
      g_value_set_boolean (value, impl->priv->select_multiple);
      break;
    case CTK_RECENT_CHOOSER_PROP_FILTER:
      g_value_set_object (value, impl->priv->current_filter);
      break;
    case PROP_ACTIVATABLE_RELATED_ACTION:
      g_value_set_object (value, _ctk_recent_chooser_get_related_action (CTK_RECENT_CHOOSER (impl)));
      break;
    case PROP_ACTIVATABLE_USE_ACTION_APPEARANCE: 
      g_value_set_boolean (value, _ctk_recent_chooser_get_use_action_appearance (CTK_RECENT_CHOOSER (impl)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_recent_chooser_default_dispose (GObject *object)
{
  CtkRecentChooserDefault *impl = CTK_RECENT_CHOOSER_DEFAULT (object);

  if (impl->priv->load_id)
    {
      g_source_remove (impl->priv->load_id);
      impl->priv->load_state = LOAD_EMPTY;
      impl->priv->load_id = 0;
    }

  g_list_free_full (impl->priv->recent_items, (GDestroyNotify) ctk_recent_info_unref);
  impl->priv->recent_items = NULL;

  if (impl->priv->manager && impl->priv->manager_changed_id)
    {
      g_signal_handler_disconnect (impl->priv->manager, impl->priv->manager_changed_id);
      impl->priv->manager_changed_id = 0;
    }

  if (impl->priv->filters)
    {
      g_slist_free_full (impl->priv->filters, g_object_unref);
      impl->priv->filters = NULL;
    }
  
  if (impl->priv->current_filter)
    {
      g_object_unref (impl->priv->current_filter);
      impl->priv->current_filter = NULL;
    }

  G_OBJECT_CLASS (_ctk_recent_chooser_default_parent_class)->dispose (object);
}

static void
ctk_recent_chooser_default_finalize (GObject *object)
{
  CtkRecentChooserDefault *impl = CTK_RECENT_CHOOSER_DEFAULT (object);

  impl->priv->manager = NULL; 
  
  if (impl->priv->sort_data_destroy)
    {
      impl->priv->sort_data_destroy (impl->priv->sort_data);
      impl->priv->sort_data_destroy = NULL;
    }
  
  impl->priv->sort_data = NULL;
  impl->priv->sort_func = NULL;
  
  G_OBJECT_CLASS (_ctk_recent_chooser_default_parent_class)->finalize (object);
}

/* override CtkWidget::show_all since we have internal widgets we wish to keep
 * hidden unless we decide otherwise, like the filter combo box.
 */
static void
ctk_recent_chooser_default_show_all (CtkWidget *widget)
{
  ctk_widget_show (widget);
}



/* Shows an error dialog set as transient for the specified window */
static void
error_message_with_parent (CtkWindow   *parent,
			   const gchar *msg,
			   const gchar *detail)
{
  CtkWidget *dialog;

  dialog = ctk_message_dialog_new (parent,
				   CTK_DIALOG_MODAL | CTK_DIALOG_DESTROY_WITH_PARENT,
				   CTK_MESSAGE_ERROR,
				   CTK_BUTTONS_OK,
				   "%s",
				   msg);
  ctk_message_dialog_format_secondary_text (CTK_MESSAGE_DIALOG (dialog),
					    "%s", detail);

  if (ctk_window_has_group (parent))
    ctk_window_group_add_window (ctk_window_get_group (parent),
                                 CTK_WINDOW (dialog));

  ctk_dialog_run (CTK_DIALOG (dialog));
  ctk_widget_destroy (dialog);
}

/* Returns a toplevel CtkWindow, or NULL if none */
static CtkWindow *
get_toplevel (CtkWidget *widget)
{
  CtkWidget *toplevel;

  toplevel = ctk_widget_get_toplevel (widget);
  if (!ctk_widget_is_toplevel (toplevel))
    return NULL;
  else
    return CTK_WINDOW (toplevel);
}

/* Shows an error dialog for the file chooser */
static void
error_message (CtkRecentChooserDefault *impl,
	       const gchar             *msg,
	       const gchar             *detail)
{
  error_message_with_parent (get_toplevel (CTK_WIDGET (impl)), msg, detail);
}

static void
set_busy_cursor (CtkRecentChooserDefault *impl,
		 gboolean                 busy)
{
  CtkWindow *toplevel;
  CdkDisplay *display;
  CdkCursor *cursor;

  toplevel = get_toplevel (CTK_WIDGET (impl));
  if (!toplevel || !ctk_widget_get_realized (CTK_WIDGET (toplevel)))
    return;

  display = ctk_widget_get_display (CTK_WIDGET (toplevel));

  if (busy)
    cursor = cdk_cursor_new_from_name (display, "progress");
  else
    cursor = NULL;

  cdk_window_set_cursor (ctk_widget_get_window (CTK_WIDGET (toplevel)), cursor);
  cdk_display_flush (display);

  if (cursor)
    g_object_unref (cursor);
}

static void
chooser_set_model (CtkRecentChooserDefault *impl)
{
  g_assert (impl->priv->recent_store != NULL);
  g_assert (impl->priv->load_state == LOAD_LOADING);

  ctk_tree_view_set_model (CTK_TREE_VIEW (impl->priv->recent_view),
                           CTK_TREE_MODEL (impl->priv->recent_store));
  ctk_tree_view_columns_autosize (CTK_TREE_VIEW (impl->priv->recent_view));
  ctk_tree_view_set_enable_search (CTK_TREE_VIEW (impl->priv->recent_view), TRUE);
  ctk_tree_view_set_search_column (CTK_TREE_VIEW (impl->priv->recent_view),
  				   RECENT_DISPLAY_NAME_COLUMN);

  impl->priv->load_state = LOAD_FINISHED;
}

static gboolean
load_recent_items (gpointer user_data)
{
  CtkRecentChooserDefault *impl;
  CtkRecentInfo *info;
  CtkTreeIter iter;
  const gchar *uri, *name;
  gboolean retval;
  
  impl = CTK_RECENT_CHOOSER_DEFAULT (user_data);
  
  g_assert ((impl->priv->load_state == LOAD_EMPTY) ||
            (impl->priv->load_state == LOAD_PRELOAD));
  
  /* store the items for multiple runs */
  if (!impl->priv->recent_items)
    {
      impl->priv->recent_items = ctk_recent_chooser_get_items (CTK_RECENT_CHOOSER (impl));
      if (!impl->priv->recent_items)
        {
	  impl->priv->load_state = LOAD_FINISHED;
          impl->priv->load_id = 0;
          return FALSE;
        }
        
      impl->priv->n_recent_items = g_list_length (impl->priv->recent_items);
      impl->priv->loaded_items = 0;
      impl->priv->load_state = LOAD_PRELOAD;
    }
  
  info = (CtkRecentInfo *) g_list_nth_data (impl->priv->recent_items,
                                            impl->priv->loaded_items);
  g_assert (info);

  uri = ctk_recent_info_get_uri (info);
  name = ctk_recent_info_get_display_name (info);
  
  /* at this point, everything goes inside the model; operations on the
   * visualization of items inside the model are done in the cell data
   * funcs (remember that there are two of those: one for the icon and
   * one for the text), while the filtering is done only when a filter
   * is actually loaded. */
  ctk_list_store_append (impl->priv->recent_store, &iter);
  ctk_list_store_set (impl->priv->recent_store, &iter,
  		      RECENT_URI_COLUMN, uri,           /* uri  */
  		      RECENT_DISPLAY_NAME_COLUMN, name, /* display_name */
  		      RECENT_INFO_COLUMN, info,         /* info */
  		      -1);
  
  impl->priv->loaded_items += 1;

  if (impl->priv->loaded_items == impl->priv->n_recent_items)
    {
      /* we have finished loading, so we remove the items cache */
      impl->priv->load_state = LOAD_LOADING;
      
      g_list_free_full (impl->priv->recent_items, (GDestroyNotify) ctk_recent_info_unref);
      
      impl->priv->recent_items = NULL;
      impl->priv->n_recent_items = 0;
      impl->priv->loaded_items = 0;

      /* load the filled up model */
      chooser_set_model (impl);

      retval = FALSE;
      impl->priv->load_id = 0;
    }
  else
    {
      /* we did not finish, so continue loading */
      retval = TRUE;
    }
  
  return retval;
}

static void
cleanup_after_load (gpointer user_data)
{
  CtkRecentChooserDefault *impl;
  
  impl = CTK_RECENT_CHOOSER_DEFAULT (user_data);

  if (impl->priv->load_id != 0)
    {
      g_assert ((impl->priv->load_state == LOAD_EMPTY) ||
                (impl->priv->load_state == LOAD_PRELOAD) ||
		(impl->priv->load_state == LOAD_LOADING) ||
		(impl->priv->load_state == LOAD_FINISHED));
      
      /* we have officialy finished loading all the items,
       * so we can reset the state machine
       */
      impl->priv->load_id = 0;
      impl->priv->load_state = LOAD_EMPTY;
    }
  else
    g_assert ((impl->priv->load_state == LOAD_EMPTY) ||
	      (impl->priv->load_state == LOAD_LOADING) ||
	      (impl->priv->load_state == LOAD_FINISHED));

  set_busy_cursor (impl, FALSE);
}

/* clears the current model and reloads the recently used resources */
static void
reload_recent_items (CtkRecentChooserDefault *impl)
{
  CtkWidget *widget;

  /* reload is already in progress - do not disturb */
  if (impl->priv->load_id)
    return;
  
  widget = CTK_WIDGET (impl);

  ctk_tree_view_set_model (CTK_TREE_VIEW (impl->priv->recent_view), NULL);
  ctk_list_store_clear (impl->priv->recent_store);
  
  if (!impl->priv->icon_theme)
    impl->priv->icon_theme = get_icon_theme_for_widget (widget);

  impl->priv->icon_size = get_icon_size_for_widget (widget,
		  			      CTK_ICON_SIZE_BUTTON);

  if (!impl->priv->limit_set)
    impl->priv->limit = DEFAULT_RECENT_FILES_LIMIT;

  set_busy_cursor (impl, TRUE);

  impl->priv->load_state = LOAD_EMPTY;
  impl->priv->load_id = cdk_threads_add_idle_full (G_PRIORITY_HIGH_IDLE + 30,
                                             load_recent_items,
                                             impl,
                                             cleanup_after_load);
  g_source_set_name_by_id (impl->priv->load_id, "[ctk+] load_recent_items");
}

/* taken form ctkfilechooserdialog.c */
static void
set_default_size (CtkRecentChooserDefault *impl)
{
  CtkScrolledWindow *scrollw;
  CtkWidget *widget;
  gint width, height;
  double font_size;
  CdkDisplay *display;
  CdkMonitor *monitor;
  CtkRequisition req;
  CdkRectangle workarea;
  CtkStyleContext *context;

  widget = CTK_WIDGET (impl);
  context = ctk_widget_get_style_context (widget);

  /* Size based on characters and the icon size */
  ctk_style_context_get (context, ctk_style_context_get_state (context), "font-size", &font_size, NULL);

  width = impl->priv->icon_size + font_size * NUM_CHARS + 0.5;
  height = (impl->priv->icon_size + font_size) * NUM_LINES + 0.5;

  /* Use at least the requisition size... */
  ctk_widget_get_preferred_size (widget, &req, NULL);
  width = MAX (width, req.width);
  height = MAX (height, req.height);

  /* ... but no larger than the monitor */
  display = ctk_widget_get_display (widget);
  monitor = cdk_display_get_monitor_at_window (display, ctk_widget_get_window (widget));
  cdk_monitor_get_workarea (monitor, &workarea);

  width = MIN (width, workarea.width * 3 / 4);
  height = MIN (height, workarea.height * 3 / 4);

  /* Set size */
  scrollw = CTK_SCROLLED_WINDOW (ctk_widget_get_parent (impl->priv->recent_view));
  ctk_scrolled_window_set_min_content_width (scrollw, width);
  ctk_scrolled_window_set_min_content_height (scrollw, height);
}

static void
ctk_recent_chooser_default_map (CtkWidget *widget)
{
  CtkRecentChooserDefault *impl = CTK_RECENT_CHOOSER_DEFAULT (widget);

  CTK_WIDGET_CLASS (_ctk_recent_chooser_default_parent_class)->map (widget);

  /* reloads everything */
  reload_recent_items (impl);

  set_default_size (impl);
}

static void
recent_icon_data_func (CtkTreeViewColumn *tree_column G_GNUC_UNUSED,
		       CtkCellRenderer   *cell,
		       CtkTreeModel      *model,
		       CtkTreeIter       *iter,
		       gpointer           user_data G_GNUC_UNUSED)
{
  CtkRecentInfo *info = NULL;
  GIcon *icon;

  ctk_tree_model_get (model, iter, RECENT_INFO_COLUMN, &info, -1);
  g_assert (info != NULL);

  icon = ctk_recent_info_get_gicon (info);
  g_object_set (cell, "gicon", icon, NULL);

  if (icon != NULL)
    g_object_unref (icon);

  ctk_recent_info_unref (info);
}

static void
recent_meta_data_func (CtkTreeViewColumn *tree_column G_GNUC_UNUSED,
		       CtkCellRenderer   *cell,
		       CtkTreeModel      *model,
		       CtkTreeIter       *iter,
		       gpointer           user_data G_GNUC_UNUSED)
{
  CtkRecentInfo *info = NULL;
  gchar *name;
  
  ctk_tree_model_get (model, iter,
                      RECENT_DISPLAY_NAME_COLUMN, &name,
                      RECENT_INFO_COLUMN, &info,
                      -1);
  g_assert (info != NULL);
  
  if (!name)
    name = ctk_recent_info_get_short_name (info);

  g_object_set (cell, "text", name, NULL);
  
  g_free (name);
  ctk_recent_info_unref (info);
}


static gchar *
ctk_recent_chooser_default_get_current_uri (CtkRecentChooser *chooser)
{
  CtkRecentChooserDefault *impl = CTK_RECENT_CHOOSER_DEFAULT (chooser);
  
  g_assert (impl->priv->selection != NULL);
  
  if (!impl->priv->select_multiple)
    {
      CtkTreeModel *model;
      CtkTreeIter iter;
      gchar *uri = NULL;
      
      if (!ctk_tree_selection_get_selected (impl->priv->selection, &model, &iter))
        return NULL;
      
      ctk_tree_model_get (model, &iter, RECENT_URI_COLUMN, &uri, -1);
      
      return uri;
    }
  
  return NULL;
}

typedef struct
{
  guint found : 1;
  guint do_select : 1;
  guint do_activate : 1;
  
  gchar *uri;
  
  CtkRecentChooserDefault *impl;
} SelectURIData;

static gboolean
scan_for_uri_cb (CtkTreeModel *model,
		 CtkTreePath  *path,
		 CtkTreeIter  *iter,
		 gpointer      user_data)
{
  SelectURIData *select_data = (SelectURIData *) user_data;
  gchar *uri = NULL;
  
  if (!select_data)
    return TRUE;
  
  if (select_data->found)
    return TRUE;
  
  ctk_tree_model_get (model, iter, RECENT_URI_COLUMN, &uri, -1);
  if (!uri)
    return FALSE;
  
  if (strcmp (uri, select_data->uri) == 0)
    {
      select_data->found = TRUE;
      
      if (select_data->do_activate)
        ctk_tree_view_row_activated (CTK_TREE_VIEW (select_data->impl->priv->recent_view),
        			     path,
        			     select_data->impl->priv->meta_column);
      
      if (select_data->do_select)
        ctk_tree_selection_select_path (select_data->impl->priv->selection, path);
      else
        ctk_tree_selection_unselect_path (select_data->impl->priv->selection, path);

      g_free (uri);
      
      return TRUE;
    }

  g_free (uri);
  
  return FALSE;
}

static gboolean
ctk_recent_chooser_default_set_current_uri (CtkRecentChooser  *chooser,
					    const gchar       *uri,
					    GError           **error)
{
  CtkRecentChooserDefault *impl = CTK_RECENT_CHOOSER_DEFAULT (chooser);
  SelectURIData *data;
  
  data = g_new0 (SelectURIData, 1);
  data->uri = g_strdup (uri);
  data->impl = impl;
  data->found = FALSE;
  data->do_activate = TRUE;
  data->do_select = TRUE;
  
  ctk_tree_model_foreach (CTK_TREE_MODEL (impl->priv->recent_store),
  			  scan_for_uri_cb,
  			  data);
  
  if (!data->found)
    {
      g_free (data->uri);
      g_free (data);
      
      g_set_error (error, CTK_RECENT_CHOOSER_ERROR,
      		   CTK_RECENT_CHOOSER_ERROR_NOT_FOUND,
      		   _("No item for URI '%s' found"),
      		   uri);
      return FALSE;
    }
  
  g_free (data->uri);
  g_free (data);

  return TRUE;
}

static gboolean
ctk_recent_chooser_default_select_uri (CtkRecentChooser  *chooser,
				       const gchar       *uri,
				       GError           **error)
{
  CtkRecentChooserDefault *impl = CTK_RECENT_CHOOSER_DEFAULT (chooser);
  SelectURIData *data;
  
  data = g_new0 (SelectURIData, 1);
  data->uri = g_strdup (uri);
  data->impl = impl;
  data->found = FALSE;
  data->do_activate = FALSE;
  data->do_select = TRUE;
  
  ctk_tree_model_foreach (CTK_TREE_MODEL (impl->priv->recent_store),
  			  scan_for_uri_cb,
  			  data);
  
  if (!data->found)
    {
      g_free (data->uri);
      g_free (data);
      
      g_set_error (error, CTK_RECENT_CHOOSER_ERROR,
      		   CTK_RECENT_CHOOSER_ERROR_NOT_FOUND,
      		   _("No item for URI '%s' found"),
      		   uri);
      return FALSE;
    }
  
  g_free (data->uri);
  g_free (data);

  return TRUE;
}

static void
ctk_recent_chooser_default_unselect_uri (CtkRecentChooser *chooser,
					 const gchar      *uri)
{
  CtkRecentChooserDefault *impl = CTK_RECENT_CHOOSER_DEFAULT (chooser);
  SelectURIData *data;
  
  data = g_new0 (SelectURIData, 1);
  data->uri = g_strdup (uri);
  data->impl = impl;
  data->found = FALSE;
  data->do_activate = FALSE;
  data->do_select = FALSE;
  
  ctk_tree_model_foreach (CTK_TREE_MODEL (impl->priv->recent_store),
  			  scan_for_uri_cb,
  			  data);
  
  g_free (data->uri);
  g_free (data);
}

static void
ctk_recent_chooser_default_select_all (CtkRecentChooser *chooser)
{
  CtkRecentChooserDefault *impl = CTK_RECENT_CHOOSER_DEFAULT (chooser);
  
  if (!impl->priv->select_multiple)
    return;
  
  ctk_tree_selection_select_all (impl->priv->selection);
}

static void
ctk_recent_chooser_default_unselect_all (CtkRecentChooser *chooser)
{
  CtkRecentChooserDefault *impl = CTK_RECENT_CHOOSER_DEFAULT (chooser);
  
  ctk_tree_selection_unselect_all (impl->priv->selection);
}

static void
ctk_recent_chooser_default_set_sort_func (CtkRecentChooser  *chooser,
					  CtkRecentSortFunc  sort_func,
					  gpointer           sort_data,
					  GDestroyNotify     data_destroy)
{
  CtkRecentChooserDefault *impl = CTK_RECENT_CHOOSER_DEFAULT (chooser);
  
  if (impl->priv->sort_data_destroy)
    {
      impl->priv->sort_data_destroy (impl->priv->sort_data);
      impl->priv->sort_data_destroy = NULL;
    }
      
  impl->priv->sort_func = NULL;
  impl->priv->sort_data = NULL;
  
  if (sort_func)
    {
      impl->priv->sort_func = sort_func;
      impl->priv->sort_data = sort_data;
      impl->priv->sort_data_destroy = data_destroy;
    }
}

static GList *
ctk_recent_chooser_default_get_items (CtkRecentChooser *chooser)
{
  CtkRecentChooserDefault *impl;

  impl = CTK_RECENT_CHOOSER_DEFAULT (chooser);

  return _ctk_recent_chooser_get_items (chooser,
                                        impl->priv->current_filter,
                                        impl->priv->sort_func,
                                        impl->priv->sort_data);
}

static CtkRecentManager *
ctk_recent_chooser_default_get_recent_manager (CtkRecentChooser *chooser)
{
  return CTK_RECENT_CHOOSER_DEFAULT (chooser)->priv->manager;
}

static void
show_filters (CtkRecentChooserDefault *impl,
              gboolean                 show)
{
  if (show)
    ctk_widget_show (impl->priv->filter_combo_hbox);
  else
    ctk_widget_hide (impl->priv->filter_combo_hbox);
}

static void
ctk_recent_chooser_default_add_filter (CtkRecentChooser *chooser,
				       CtkRecentFilter  *filter)
{
  CtkRecentChooserDefault *impl;
  const gchar *name;

  impl = CTK_RECENT_CHOOSER_DEFAULT (chooser);
  
  if (g_slist_find (impl->priv->filters, filter))
    {
      g_warning ("ctk_recent_chooser_add_filter() called on filter already in list");
      return;
    }
  
  g_object_ref_sink (filter);
  impl->priv->filters = g_slist_append (impl->priv->filters, filter);
  
  /* display new filter */
  name = ctk_recent_filter_get_name (filter);
  if (!name)
    name = _("Untitled filter");

  ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (impl->priv->filter_combo), name);

  if (!g_slist_find (impl->priv->filters, impl->priv->current_filter))
    set_current_filter (impl, filter);
  
  show_filters (impl, TRUE);
}

static void
ctk_recent_chooser_default_remove_filter (CtkRecentChooser *chooser,
					  CtkRecentFilter  *filter)
{
  CtkRecentChooserDefault *impl = CTK_RECENT_CHOOSER_DEFAULT (chooser);
  CtkTreeModel *model;
  CtkTreeIter iter;
  gint filter_idx;
  
  filter_idx = g_slist_index (impl->priv->filters, filter);
  
  if (filter_idx < 0)
    {
      g_warning ("ctk_recent_chooser_remove_filter() called on filter not in list");
      return;  
    }
  
  impl->priv->filters = g_slist_remove (impl->priv->filters, filter);
  
  if (filter == impl->priv->current_filter)
    {
      if (impl->priv->filters)
        set_current_filter (impl, impl->priv->filters->data);
      else
        set_current_filter (impl, NULL);
    }
  
  model = ctk_combo_box_get_model (CTK_COMBO_BOX (impl->priv->filter_combo));
  ctk_tree_model_iter_nth_child (model, &iter, NULL, filter_idx);
  ctk_list_store_remove (CTK_LIST_STORE (model), &iter);
  
  g_object_unref (filter);
  
  if (!impl->priv->filters)
    show_filters (impl, FALSE);
}

static GSList *
ctk_recent_chooser_default_list_filters (CtkRecentChooser *chooser)
{
  CtkRecentChooserDefault *impl = CTK_RECENT_CHOOSER_DEFAULT (chooser);
  
  return g_slist_copy (impl->priv->filters);
}

static void
set_current_filter (CtkRecentChooserDefault *impl,
		    CtkRecentFilter         *filter)
{
  if (impl->priv->current_filter != filter)
    {
      gint filter_idx;
      
      filter_idx = g_slist_index (impl->priv->filters, filter);
      if (impl->priv->filters && filter && filter_idx < 0)
        return;
      
      if (impl->priv->current_filter)
        g_object_unref (impl->priv->current_filter);
      
      impl->priv->current_filter = filter;
      
      if (impl->priv->current_filter)     
        {
          g_object_ref_sink (impl->priv->current_filter);
        }
      
      if (impl->priv->filters)
        ctk_combo_box_set_active (CTK_COMBO_BOX (impl->priv->filter_combo),
                                  filter_idx);
      
      if (impl->priv->recent_store)
        reload_recent_items (impl);

      g_object_notify (G_OBJECT (impl), "filter");
    }
}

static void
chooser_set_sort_type (CtkRecentChooserDefault *impl,
		       CtkRecentSortType        sort_type)
{
  if (impl->priv->sort_type != sort_type)
    {
      impl->priv->sort_type = sort_type;
      reload_recent_items (impl);
      g_object_notify (G_OBJECT (impl), "sort-type");
    }
}


static CtkIconTheme *
get_icon_theme_for_widget (CtkWidget *widget)
{
  return ctk_css_icon_theme_value_get_icon_theme
    (_ctk_style_context_peek_property (ctk_widget_get_style_context (widget),
                                       CTK_CSS_PROPERTY_ICON_THEME));
}

static gint
get_icon_size_for_widget (CtkWidget   *widget G_GNUC_UNUSED,
			  CtkIconSize  icon_size)
{
  gint width, height;

  if (ctk_icon_size_lookup (icon_size, &width, &height))
    return MAX (width, height);

  return FALLBACK_ICON_SIZE;
}

static void
recent_manager_changed_cb (CtkRecentManager *manager G_GNUC_UNUSED,
			   gpointer          user_data)
{
  CtkRecentChooserDefault *impl = CTK_RECENT_CHOOSER_DEFAULT (user_data);

  reload_recent_items (impl);
}

static void
selection_changed_cb (CtkTreeSelection *selection G_GNUC_UNUSED,
		      gpointer          user_data)
{
  _ctk_recent_chooser_selection_changed (CTK_RECENT_CHOOSER (user_data));
}

static void
row_activated_cb (CtkTreeView       *tree_view G_GNUC_UNUSED,
		  CtkTreePath       *tree_path G_GNUC_UNUSED,
		  CtkTreeViewColumn *tree_column G_GNUC_UNUSED,
		  gpointer           user_data)
{
  _ctk_recent_chooser_item_activated (CTK_RECENT_CHOOSER (user_data));
}

static void
filter_combo_changed_cb (CtkComboBox *combo_box,
			 gpointer     user_data)
{
  CtkRecentChooserDefault *impl;
  gint new_index;
  CtkRecentFilter *filter;
  
  impl = CTK_RECENT_CHOOSER_DEFAULT (user_data);
  
  new_index = ctk_combo_box_get_active (combo_box);
  filter = g_slist_nth_data (impl->priv->filters, new_index);
  
  set_current_filter (impl, filter);
}

static GdkPixbuf *
get_drag_pixbuf (CtkRecentChooserDefault *impl)
{
  CtkRecentInfo *info;
  GdkPixbuf *retval;
  gint size;
  
  g_assert (CTK_IS_RECENT_CHOOSER_DEFAULT (impl));

  info = ctk_recent_chooser_get_current_item (CTK_RECENT_CHOOSER (impl));
  if (!info)
    return NULL;

  size = get_icon_size_for_widget (CTK_WIDGET (impl), CTK_ICON_SIZE_DND);

  retval = ctk_recent_info_get_icon (info, size);
  ctk_recent_info_unref (info);

  return retval;
}

static void
recent_view_drag_begin_cb (CtkWidget      *widget G_GNUC_UNUSED,
			   CdkDragContext *context,
			   gpointer        user_data)
{
  CtkRecentChooserDefault *impl = CTK_RECENT_CHOOSER_DEFAULT (user_data);
  GdkPixbuf *pixbuf;

  pixbuf = get_drag_pixbuf (impl);
  if (pixbuf)
    {
      ctk_drag_set_icon_pixbuf (context, pixbuf, 0, 0);
      g_object_unref (pixbuf);
    }
  else
    ctk_drag_set_icon_default (context);
}

typedef struct
{
  gchar **uri_list;
  gsize next_pos;
} DragData;

static void
append_uri_to_urilist (CtkTreeModel *model,
		       CtkTreePath  *path G_GNUC_UNUSED,
		       CtkTreeIter  *iter,
		       gpointer      user_data)
{
  DragData *drag_data = (DragData *) user_data;
  gchar *uri = NULL;
  gsize pos;

  ctk_tree_model_get (model, iter,
  		      RECENT_URI_COLUMN, &uri,
  		      -1);
  g_assert (uri != NULL);

  pos = drag_data->next_pos;
  drag_data->uri_list[pos] = g_strdup (uri);
  drag_data->next_pos = pos + 1;
}

static void
recent_view_drag_data_get_cb (CtkWidget        *widget G_GNUC_UNUSED,
			      CdkDragContext   *context G_GNUC_UNUSED,
			      CtkSelectionData *selection_data,
			      guint             info G_GNUC_UNUSED,
			      guint32           time_ G_GNUC_UNUSED,
			      gpointer          data)
{
  CtkRecentChooserDefault *impl = CTK_RECENT_CHOOSER_DEFAULT (data);
  DragData drag_data;
  gsize n_uris;
  
  n_uris = ctk_tree_selection_count_selected_rows (impl->priv->selection);
  if (n_uris == 0)
    return;

  drag_data.uri_list = g_new0 (gchar *, n_uris + 1);
  drag_data.next_pos = 0;
  
  ctk_tree_selection_selected_foreach (impl->priv->selection,
      				       append_uri_to_urilist,
      				       &drag_data);
  
  ctk_selection_data_set_uris (selection_data, drag_data.uri_list);

  g_strfreev (drag_data.uri_list);
}

static gboolean
recent_view_query_tooltip_cb (CtkWidget  *widget G_GNUC_UNUSED,
                              gint        x,
                              gint        y,
                              gboolean    keyboard_tip,
                              CtkTooltip *tooltip,
                              gpointer    user_data)
{
  CtkRecentChooserDefault *impl = user_data;
  CtkTreeView *tree_view;
  CtkTreeIter iter;
  CtkTreePath *path = NULL;
  CtkRecentInfo *info = NULL;
  gchar *uri_display;

  if (!impl->priv->show_tips)
    return FALSE;

  tree_view = CTK_TREE_VIEW (impl->priv->recent_view);

  ctk_tree_view_get_tooltip_context (tree_view,
                                     &x, &y,
                                     keyboard_tip,
                                     NULL, &path, NULL);
  if (!path)
    return FALSE;

  if (!ctk_tree_model_get_iter (CTK_TREE_MODEL (impl->priv->recent_store), &iter, path))
    {
      ctk_tree_path_free (path);
      return FALSE;
    }

  ctk_tree_model_get (CTK_TREE_MODEL (impl->priv->recent_store), &iter,
                      RECENT_INFO_COLUMN, &info,
                      -1);

  uri_display = ctk_recent_info_get_uri_display (info);
  
  ctk_tooltip_set_text (tooltip, uri_display);
  ctk_tree_view_set_tooltip_row (tree_view, tooltip, path);

  g_free (uri_display);
  ctk_tree_path_free (path);
  ctk_recent_info_unref (info);

  return TRUE;
}

static void
remove_selected_from_list (CtkRecentChooserDefault *impl)
{
  gchar *uri;
  GError *err;
  
  if (impl->priv->select_multiple)
    return;
  
  uri = ctk_recent_chooser_get_current_uri (CTK_RECENT_CHOOSER (impl));
  if (!uri)
    return;
  
  err = NULL;
  if (!ctk_recent_manager_remove_item (impl->priv->manager, uri, &err))
    {
      gchar *msg;
   
      msg = g_strdup (_("Could not remove item"));
      error_message (impl, msg, err->message);
      
      g_free (msg);
      g_error_free (err);
    }
  
  g_free (uri);
}

static void
copy_activated_cb (CtkMenuItem *menu_item G_GNUC_UNUSED,
		   gpointer     user_data)
{
  CtkRecentChooserDefault *impl = CTK_RECENT_CHOOSER_DEFAULT (user_data);
  CtkRecentInfo *info;
  gchar *utf8_uri;

  info = ctk_recent_chooser_get_current_item (CTK_RECENT_CHOOSER (impl));
  if (!info)
    return;

  utf8_uri = ctk_recent_info_get_uri_display (info);
  
  ctk_clipboard_set_text (ctk_widget_get_clipboard (CTK_WIDGET (impl),
			  			    CDK_SELECTION_CLIPBOARD),
                          utf8_uri, -1);

  ctk_recent_info_unref (info);
  g_free (utf8_uri);
}

static void
remove_all_activated_cb (CtkMenuItem *menu_item G_GNUC_UNUSED,
			 gpointer     user_data)
{
  CtkRecentChooserDefault *impl = CTK_RECENT_CHOOSER_DEFAULT (user_data);
  GError *err = NULL;
  
  ctk_recent_manager_purge_items (impl->priv->manager, &err);
  if (err)
    {
       gchar *msg;

       msg = g_strdup (_("Could not clear list"));

       error_message (impl, msg, err->message);
       
       g_free (msg);
       g_error_free (err);
    }
}

static void
remove_item_activated_cb (CtkMenuItem *menu_item G_GNUC_UNUSED,
			  gpointer     user_data)
{
  CtkRecentChooserDefault *impl = CTK_RECENT_CHOOSER_DEFAULT (user_data);
  
  remove_selected_from_list (impl);
}

static void
show_private_toggled_cb (CtkCheckMenuItem *menu_item,
			 gpointer          user_data)
{
  CtkRecentChooserDefault *impl = CTK_RECENT_CHOOSER_DEFAULT (user_data);
  
  g_object_set (G_OBJECT (impl),
  		"show-private", ctk_check_menu_item_get_active (menu_item),
  		NULL);
}

static void
recent_popup_menu_detach_cb (CtkWidget *attach_widget,
			     CtkMenu   *menu G_GNUC_UNUSED)
{
  CtkRecentChooserDefault *impl;
  
  impl = g_object_get_data (G_OBJECT (attach_widget), "CtkRecentChooserDefault");
  g_assert (CTK_IS_RECENT_CHOOSER_DEFAULT (impl));
  
  impl->priv->recent_popup_menu = NULL;
  impl->priv->recent_popup_menu_remove_item = NULL;
  impl->priv->recent_popup_menu_copy_item = NULL;
  impl->priv->recent_popup_menu_clear_item = NULL;
  impl->priv->recent_popup_menu_show_private_item = NULL;
}

static void
recent_view_menu_ensure_state (CtkRecentChooserDefault *impl)
{
  gint count;
  
  g_assert (CTK_IS_RECENT_CHOOSER_DEFAULT (impl));
  g_assert (impl->priv->recent_popup_menu != NULL);

  if (!impl->priv->manager)
    count = 0;
  else
    g_object_get (G_OBJECT (impl->priv->manager), "size", &count, NULL);

  if (count == 0)
    {
      ctk_widget_set_sensitive (impl->priv->recent_popup_menu_remove_item, FALSE);
      ctk_widget_set_sensitive (impl->priv->recent_popup_menu_copy_item, FALSE);
      ctk_widget_set_sensitive (impl->priv->recent_popup_menu_clear_item, FALSE);
      ctk_widget_set_sensitive (impl->priv->recent_popup_menu_show_private_item, FALSE);
    }
}

static void
recent_view_menu_build (CtkRecentChooserDefault *impl)
{
  CtkWidget *item;
  
  if (impl->priv->recent_popup_menu)
    {
      recent_view_menu_ensure_state (impl);
      
      return;
    }
  
  impl->priv->recent_popup_menu = ctk_menu_new ();
  ctk_menu_attach_to_widget (CTK_MENU (impl->priv->recent_popup_menu),
  			     impl->priv->recent_view,
  			     recent_popup_menu_detach_cb);
  
  item = ctk_menu_item_new_with_mnemonic (_("Copy _Location"));
  impl->priv->recent_popup_menu_copy_item = item;
  g_signal_connect (item, "activate",
		    G_CALLBACK (copy_activated_cb), impl);
  ctk_widget_show (item);
  ctk_menu_shell_append (CTK_MENU_SHELL (impl->priv->recent_popup_menu), item);

  item = ctk_separator_menu_item_new ();
  ctk_widget_show (item);
  ctk_menu_shell_append (CTK_MENU_SHELL (impl->priv->recent_popup_menu), item);
  
  item = ctk_menu_item_new_with_mnemonic (_("_Remove From List"));
  impl->priv->recent_popup_menu_remove_item = item;
  g_signal_connect (item, "activate",
  		    G_CALLBACK (remove_item_activated_cb), impl);
  ctk_widget_show (item);
  ctk_menu_shell_append (CTK_MENU_SHELL (impl->priv->recent_popup_menu), item);

  item = ctk_menu_item_new_with_mnemonic (_("_Clear List"));
  impl->priv->recent_popup_menu_clear_item = item;
  g_signal_connect (item, "activate",
		    G_CALLBACK (remove_all_activated_cb), impl);
  
  ctk_widget_show (item);
  ctk_menu_shell_append (CTK_MENU_SHELL (impl->priv->recent_popup_menu), item);
  
  item = ctk_separator_menu_item_new ();
  ctk_widget_show (item);
  ctk_menu_shell_append (CTK_MENU_SHELL (impl->priv->recent_popup_menu), item);
  
  item = ctk_check_menu_item_new_with_mnemonic (_("Show _Private Resources"));
  impl->priv->recent_popup_menu_show_private_item = item;
  ctk_check_menu_item_set_active (CTK_CHECK_MENU_ITEM (item), impl->priv->show_private);
  g_signal_connect (item, "toggled",
  		    G_CALLBACK (show_private_toggled_cb), impl);
  ctk_widget_show (item);
  ctk_menu_shell_append (CTK_MENU_SHELL (impl->priv->recent_popup_menu), item);
  
  recent_view_menu_ensure_state (impl);
}

static void
recent_view_menu_popup (CtkRecentChooserDefault *impl,
			CdkEventButton          *event)
{
  recent_view_menu_build (impl);
  
  if (event && cdk_event_triggers_context_menu ((CdkEvent *) event))
    ctk_menu_popup_at_pointer (CTK_MENU (impl->priv->recent_popup_menu), (CdkEvent *) event);
  else
    {
      ctk_menu_popup_at_widget (CTK_MENU (impl->priv->recent_popup_menu),
                                impl->priv->recent_view,
                                CDK_GRAVITY_CENTER,
                                CDK_GRAVITY_CENTER,
                                (CdkEvent *) event);

      ctk_menu_shell_select_first (CTK_MENU_SHELL (impl->priv->recent_popup_menu), FALSE);
    }
}

static gboolean
recent_view_popup_menu_cb (CtkWidget *widget G_GNUC_UNUSED,
			   gpointer   user_data)
{
  recent_view_menu_popup (CTK_RECENT_CHOOSER_DEFAULT (user_data), NULL);
  return TRUE;
}

static gboolean
recent_view_button_press_cb (CtkWidget      *widget G_GNUC_UNUSED,
			     CdkEventButton *event,
			     gpointer        user_data)
{
  CtkRecentChooserDefault *impl = CTK_RECENT_CHOOSER_DEFAULT (user_data);

  if (cdk_event_triggers_context_menu ((CdkEvent *) event))
    {
      CtkTreePath *path;
      gboolean res;

      if (event->window != ctk_tree_view_get_bin_window (CTK_TREE_VIEW (impl->priv->recent_view)))
        return FALSE;

      res = ctk_tree_view_get_path_at_pos (CTK_TREE_VIEW (impl->priv->recent_view),
		      			   event->x, event->y,
					   &path,
					   NULL, NULL, NULL);
      if (!res)
        return FALSE;

      /* select the path before creating the popup menu */
      ctk_tree_selection_select_path (impl->priv->selection, path);
      ctk_tree_path_free (path);
      
      recent_view_menu_popup (impl, event);

      return TRUE;
    }
  
  return FALSE;
}

static void
set_recent_manager (CtkRecentChooserDefault *impl,
		    CtkRecentManager        *manager)
{
  if (impl->priv->manager)
    {
      if (impl->priv->manager_changed_id)
        {
          g_signal_handler_disconnect (impl, impl->priv->manager_changed_id);
          impl->priv->manager_changed_id = 0;
        }

      impl->priv->manager = NULL;
    }
  
  if (manager)
    impl->priv->manager = manager;
  else
    impl->priv->manager = ctk_recent_manager_get_default ();
  
  if (impl->priv->manager)
    {
      impl->priv->manager_changed_id = g_signal_connect (impl->priv->manager, "changed",
							 G_CALLBACK (recent_manager_changed_cb),
							 impl);
    }
}

static void
ctk_recent_chooser_update (CtkActivatable *activatable,
			   CtkAction      *action,
			   const gchar    *property_name)
{
  if (strcmp (property_name, "visible") == 0)
    {
      if (ctk_action_is_visible (action))
	ctk_widget_show (CTK_WIDGET (activatable));
      else
	ctk_widget_hide (CTK_WIDGET (activatable));
    }

  if (strcmp (property_name, "sensitive") == 0)
    ctk_widget_set_sensitive (CTK_WIDGET (activatable), ctk_action_is_sensitive (action));

  _ctk_recent_chooser_update (activatable, action, property_name);
}


static void 
ctk_recent_chooser_sync_action_properties (CtkActivatable *activatable,
				           CtkAction      *action)
{
  if (action)
    {
      if (ctk_action_is_visible (action))
	ctk_widget_show (CTK_WIDGET (activatable));
      else
	ctk_widget_hide (CTK_WIDGET (activatable));
      
      ctk_widget_set_sensitive (CTK_WIDGET (activatable), ctk_action_is_sensitive (action));
    }

  _ctk_recent_chooser_sync_action_properties (activatable, action);
}


CtkWidget *
_ctk_recent_chooser_default_new (CtkRecentManager *manager)
{
  return g_object_new (CTK_TYPE_RECENT_CHOOSER_DEFAULT,
  		       "recent-manager", manager,
  		       NULL);
}
