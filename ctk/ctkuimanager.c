/*
 * CTK - The GIMP Toolkit
 * Copyright (C) 1998, 1999 Red Hat, Inc.
 * All rights reserved.
 *
 * This Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Author: James Henstridge <james@daa.com.au>
 *
 * Modified by the CTK+ Team and others 2003.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#include "config.h"

#include <string.h>
#include "ctkaccellabel.h"
#include "ctkactivatable.h"
#include "ctkbuildable.h"
#include "ctkimagemenuitem.h"
#include "ctkintl.h"
#include "ctkmarshalers.h"
#include "ctkmenu.h"
#include "ctkmenushellprivate.h"
#include "ctkmenubar.h"
#include "ctkmenutoolbutton.h"
#include "ctkseparatormenuitem.h"
#include "ctkseparatortoolitem.h"
#include "ctktoolbar.h"
#include "ctkwindow.h"
#include "ctkprivate.h"

#include "ctkuimanager.h"
#include "ctktearoffmenuitem.h"

/**
 * SECTION:ctkuimanager
 * @Short_description: Constructing menus and toolbars from an XML description
 * @Title: CtkUIManager
 * @See_also: #CtkBuilder
 *
 * A #CtkUIManager constructs a user interface (menus and toolbars) from
 * one or more UI definitions, which reference actions from one or more
 * action groups.
 *
 * # UI Definitions # {#XML-UI}
 *
 * The UI definitions are specified in an XML format which can be
 * roughly described by the following DTD.
 *
 * > Do not confuse the CtkUIManager UI Definitions described here with
 * > the similarly named [CtkBuilder UI Definitions][BUILDER-UI].
 *
 * |[
 * <!ELEMENT ui          (menubar|toolbar|popup|accelerator)* >
 * <!ELEMENT menubar     (menuitem|separator|placeholder|menu)* >
 * <!ELEMENT menu        (menuitem|separator|placeholder|menu)* >
 * <!ELEMENT popup       (menuitem|separator|placeholder|menu)* >
 * <!ELEMENT toolbar     (toolitem|separator|placeholder)* >
 * <!ELEMENT placeholder (menuitem|toolitem|separator|placeholder|menu)* >
 * <!ELEMENT menuitem     EMPTY >
 * <!ELEMENT toolitem     (menu?) >
 * <!ELEMENT separator    EMPTY >
 * <!ELEMENT accelerator  EMPTY >
 * <!ATTLIST menubar      name                      #IMPLIED
 *                        action                    #IMPLIED >
 * <!ATTLIST toolbar      name                      #IMPLIED
 *                        action                    #IMPLIED >
 * <!ATTLIST popup        name                      #IMPLIED
 *                        action                    #IMPLIED
 *                        accelerators (true|false) #IMPLIED >
 * <!ATTLIST placeholder  name                      #IMPLIED
 *                        action                    #IMPLIED >
 * <!ATTLIST separator    name                      #IMPLIED
 *                        action                    #IMPLIED
 *                        expand       (true|false) #IMPLIED >
 * <!ATTLIST menu         name                      #IMPLIED
 *                        action                    #REQUIRED
 *                        position     (top|bot)    #IMPLIED >
 * <!ATTLIST menuitem     name                      #IMPLIED
 *                        action                    #REQUIRED
 *                        position     (top|bot)    #IMPLIED
 *                        always-show-image (true|false) #IMPLIED >
 * <!ATTLIST toolitem     name                      #IMPLIED
 *                        action                    #REQUIRED
 *                        position     (top|bot)    #IMPLIED >
 * <!ATTLIST accelerator  name                      #IMPLIED
 *                        action                    #REQUIRED >
 * ]|
 *
 * There are some additional restrictions beyond those specified in the
 * DTD, e.g. every toolitem must have a toolbar in its anchestry and
 * every menuitem must have a menubar or popup in its anchestry. Since
 * a #GMarkupParser is used to parse the UI description, it must not only
 * be valid XML, but valid markup.
 *
 * If a name is not specified, it defaults to the action. If an action is
 * not specified either, the element name is used. The name and action
 * attributes must not contain “/” characters after parsing (since that
 * would mess up path lookup) and must be usable as XML attributes when
 * enclosed in doublequotes, thus they must not “"” characters or references
 * to the &quot; entity.
 *
 * # A UI definition #
 *
 * |[
 * <ui>
 *   <menubar>
 *     <menu name="FileMenu" action="FileMenuAction">
 *       <menuitem name="New" action="New2Action" />
 *       <placeholder name="FileMenuAdditions" />
 *     </menu>
 *     <menu name="JustifyMenu" action="JustifyMenuAction">
 *       <menuitem name="Left" action="justify-left"/>
 *       <menuitem name="Centre" action="justify-center"/>
 *       <menuitem name="Right" action="justify-right"/>
 *       <menuitem name="Fill" action="justify-fill"/>
 *     </menu>
 *   </menubar>
 *   <toolbar action="toolbar1">
 *     <placeholder name="JustifyToolItems">
 *       <separator/>
 *       <toolitem name="Left" action="justify-left"/>
 *       <toolitem name="Centre" action="justify-center"/>
 *       <toolitem name="Right" action="justify-right"/>
 *       <toolitem name="Fill" action="justify-fill"/>
 *       <separator/>
 *     </placeholder>
 *   </toolbar>
 * </ui>
 * ]|
 *
 * The constructed widget hierarchy is very similar to the element tree
 * of the XML, with the exception that placeholders are merged into their
 * parents. The correspondence of XML elements to widgets should be
 * almost obvious:
 *
 * - menubar
 *
 *    a #CtkMenuBar
 *
 * - toolbar
 *
 *    a #CtkToolbar
 *
 * - popup
 *
 *    a toplevel #CtkMenu
 *
 * - menu
 *
 *    a #CtkMenu attached to a menuitem
 *
 * - menuitem
 *
 *    a #CtkMenuItem subclass, the exact type depends on the action
 *
 * - toolitem
 *
 *    a #CtkToolItem subclass, the exact type depends on the
 *    action. Note that toolitem elements may contain a menu element,
 *    but only if their associated action specifies a
 *    #CtkMenuToolButton as proxy.
 *
 * - separator
 *
 *    a #CtkSeparatorMenuItem or #CtkSeparatorToolItem
 *
 * - accelerator
 *
 *    a keyboard accelerator
 *
 * The “position” attribute determines where a constructed widget is positioned
 * wrt. to its siblings in the partially constructed tree. If it is
 * “top”, the widget is prepended, otherwise it is appended.
 *
 * # UI Merging # {#UI-Merging}
 *
 * The most remarkable feature of #CtkUIManager is that it can overlay a set
 * of menuitems and toolitems over another one, and demerge them later.
 *
 * Merging is done based on the names of the XML elements. Each element is
 * identified by a path which consists of the names of its anchestors, separated
 * by slashes. For example, the menuitem named “Left” in the example above
 * has the path `/ui/menubar/JustifyMenu/Left` and the
 * toolitem with the same name has path
 * `/ui/toolbar1/JustifyToolItems/Left`.
 *
 * # Accelerators #
 *
 * Every action has an accelerator path. Accelerators are installed together
 * with menuitem proxies, but they can also be explicitly added with
 * <accelerator> elements in the UI definition. This makes it possible to
 * have accelerators for actions even if they have no visible proxies.
 *
 * # Smart Separators # {#Smart-Separators}
 *
 * The separators created by #CtkUIManager are “smart”, i.e. they do not show up
 * in the UI unless they end up between two visible menu or tool items. Separators
 * which are located at the very beginning or end of the menu or toolbar
 * containing them, or multiple separators next to each other, are hidden. This
 * is a useful feature, since the merging of UI elements from multiple sources
 * can make it hard or impossible to determine in advance whether a separator
 * will end up in such an unfortunate position.
 *
 * For separators in toolbars, you can set `expand="true"` to
 * turn them from a small, visible separator to an expanding, invisible one.
 * Toolitems following an expanding separator are effectively right-aligned.
 *
 * # Empty Menus
 *
 * Submenus pose similar problems to separators inconnection with merging. It is
 * impossible to know in advance whether they will end up empty after merging.
 * #CtkUIManager offers two ways to treat empty submenus:
 *
 * - make them disappear by hiding the menu item they’re attached to
 *
 * - add an insensitive “Empty” item
 *
 * The behaviour is chosen based on the “hide_if_empty” property of the action
 * to which the submenu is associated.
 *
 * # CtkUIManager as CtkBuildable # {#CtkUIManager-BUILDER-UI}
 *
 * The CtkUIManager implementation of the CtkBuildable interface accepts
 * CtkActionGroup objects as <child> elements in UI definitions.
 *
 * A CtkUIManager UI definition as described above can be embedded in
 * an CtkUIManager <object> element in a CtkBuilder UI definition.
 *
 * The widgets that are constructed by a CtkUIManager can be embedded in
 * other parts of the constructed user interface with the help of the
 * “constructor” attribute. See the example below.
 *
 * ## An embedded CtkUIManager UI definition
 *
 * |[
 * <object class="CtkUIManager" id="uiman">
 *   <child>
 *     <object class="CtkActionGroup" id="actiongroup">
 *       <child>
 *         <object class="CtkAction" id="file">
 *           <property name="label">_File</property>
 *         </object>
 *       </child>
 *     </object>
 *   </child>
 *   <ui>
 *     <menubar name="menubar1">
 *       <menu action="file">
 *       </menu>
 *     </menubar>
 *   </ui>
 * </object>
 * <object class="CtkWindow" id="main-window">
 *   <child>
 *     <object class="CtkMenuBar" id="menubar1" constructor="uiman"/>
 *   </child>
 * </object>
 * ]|
 */

typedef enum
{
  NODE_TYPE_UNDECIDED,
  NODE_TYPE_ROOT,
  NODE_TYPE_MENUBAR,
  NODE_TYPE_MENU,
  NODE_TYPE_TOOLBAR,
  NODE_TYPE_MENU_PLACEHOLDER,
  NODE_TYPE_TOOLBAR_PLACEHOLDER,
  NODE_TYPE_POPUP,
  NODE_TYPE_MENUITEM,
  NODE_TYPE_TOOLITEM,
  NODE_TYPE_SEPARATOR,
  NODE_TYPE_ACCELERATOR
} NodeType;

typedef struct _Node Node;

struct _Node {
  NodeType type;

  gchar *name;

  GQuark action_name;
  CtkAction *action;
  CtkWidget *proxy;
  CtkWidget *extra; /* second separator for placeholders */

  GList *uifiles;

  guint dirty : 1;
  guint expand : 1;  /* used for separators */
  guint popup_accels : 1;
  guint always_show_image_set : 1; /* used for menu items */
  guint always_show_image     : 1; /* used for menu items */
};


struct _CtkUIManagerPrivate 
{
  CtkAccelGroup *accel_group;

  GNode *root_node;
  GList *action_groups;

  guint last_merge_id;

  guint update_tag;  

  gboolean add_tearoffs;
};

#define NODE_INFO(node) ((Node *)node->data)

typedef struct _NodeUIReference NodeUIReference;

struct _NodeUIReference 
{
  guint merge_id;
  GQuark action_quark;
};

static void        ctk_ui_manager_finalize        (GObject           *object);
static void        ctk_ui_manager_set_property    (GObject           *object,
                                                   guint              prop_id,
                                                   const GValue      *value,
                                                   GParamSpec        *pspec);
static void        ctk_ui_manager_get_property    (GObject           *object,
                                                   guint              prop_id,
                                                   GValue            *value,
                                                   GParamSpec        *pspec);
static CtkWidget * ctk_ui_manager_real_get_widget (CtkUIManager      *manager,
                                                   const gchar       *path);
static CtkAction * ctk_ui_manager_real_get_action (CtkUIManager      *manager,
                                                   const gchar       *path);
static void        queue_update                   (CtkUIManager      *manager);
static void        dirty_all_nodes                (CtkUIManager      *manager);
static void        mark_node_dirty                (GNode             *node);
static GNode     * get_child_node                 (CtkUIManager      *manager,
                                                   GNode             *parent,
						   GNode             *sibling,
                                                   const gchar       *childname,
                                                   gint               childname_length,
                                                   NodeType           node_type,
                                                   gboolean           create,
                                                   gboolean           top);
static GNode     * get_node                       (CtkUIManager      *manager,
                                                   const gchar       *path,
                                                   NodeType           node_type,
                                                   gboolean           create);
static gboolean    free_node                      (GNode             *node);
static void        node_prepend_ui_reference      (GNode             *node,
                                                   guint              merge_id,
                                                   GQuark             action_quark);
static void        node_remove_ui_reference       (GNode             *node,
                                                   guint              merge_id);

/* CtkBuildable */
static void ctk_ui_manager_buildable_init      (CtkBuildableIface *iface);
static void ctk_ui_manager_buildable_add_child (CtkBuildable  *buildable,
						CtkBuilder    *builder,
						GObject       *child,
						const gchar   *type);
static GObject* ctk_ui_manager_buildable_construct_child (CtkBuildable *buildable,
							  CtkBuilder   *builder,
							  const gchar  *name);
static gboolean ctk_ui_manager_buildable_custom_tag_start (CtkBuildable  *buildable,
							   CtkBuilder    *builder,
							   GObject       *child,
							   const gchar   *tagname,
							   GMarkupParser *parser,
							   gpointer      *data);
static void     ctk_ui_manager_buildable_custom_tag_end (CtkBuildable 	 *buildable,
							 CtkBuilder   	 *builder,
							 GObject      	 *child,
							 const gchar  	 *tagname,
							 gpointer     	 *data);
static void ctk_ui_manager_do_set_add_tearoffs          (CtkUIManager *manager,
                                                         gboolean      add_tearoffs);



enum 
{
  ADD_WIDGET,
  ACTIONS_CHANGED,
  CONNECT_PROXY,
  DISCONNECT_PROXY,
  PRE_ACTIVATE,
  POST_ACTIVATE,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_ADD_TEAROFFS,
  PROP_UI
};

static guint ui_manager_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_CODE (CtkUIManager, ctk_ui_manager, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (CtkUIManager)
			 G_IMPLEMENT_INTERFACE (CTK_TYPE_BUILDABLE,
						ctk_ui_manager_buildable_init))

static void
ctk_ui_manager_class_init (CtkUIManagerClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = ctk_ui_manager_finalize;
  gobject_class->set_property = ctk_ui_manager_set_property;
  gobject_class->get_property = ctk_ui_manager_get_property;
  klass->get_widget = ctk_ui_manager_real_get_widget;
  klass->get_action = ctk_ui_manager_real_get_action;

  /**
   * CtkUIManager:add-tearoffs:
   *
   * The "add-tearoffs" property controls whether generated menus 
   * have tearoff menu items. 
   *
   * Note that this only affects regular menus. Generated popup 
   * menus never have tearoff menu items.   
   *
   * Since: 2.4
   */
  g_object_class_install_property (gobject_class,
                                   PROP_ADD_TEAROFFS,
                                   g_param_spec_boolean ("add-tearoffs",
							 P_("Add tearoffs to menus"),
							 P_("Whether tearoff menu items should be added to menus"),
                                                         FALSE,
							 CTK_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
				   PROP_UI,
				   g_param_spec_string ("ui",
 							P_("Merged UI definition"),
							P_("An XML string describing the merged UI"),
							"<ui>\n</ui>\n",
							CTK_PARAM_READABLE));


  /**
   * CtkUIManager::add-widget:
   * @manager: a #CtkUIManager
   * @widget: the added widget
   *
   * The ::add-widget signal is emitted for each generated menubar and toolbar.
   * It is not emitted for generated popup menus, which can be obtained by 
   * ctk_ui_manager_get_widget().
   *
   * Since: 2.4
   */
  ui_manager_signals[ADD_WIDGET] =
    g_signal_new (I_("add-widget"),
		  G_OBJECT_CLASS_TYPE (klass),
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
		  G_STRUCT_OFFSET (CtkUIManagerClass, add_widget),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1, 
		  CTK_TYPE_WIDGET);

  /**
   * CtkUIManager::actions-changed:
   * @manager: a #CtkUIManager
   *
   * The ::actions-changed signal is emitted whenever the set of actions
   * changes.
   *
   * Since: 2.4
   */
  ui_manager_signals[ACTIONS_CHANGED] =
    g_signal_new (I_("actions-changed"),
		  G_OBJECT_CLASS_TYPE (klass),
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
		  G_STRUCT_OFFSET (CtkUIManagerClass, actions_changed),  
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);
  
  /**
   * CtkUIManager::connect-proxy:
   * @manager: the ui manager
   * @action: the action
   * @proxy: the proxy
   *
   * The ::connect-proxy signal is emitted after connecting a proxy to
   * an action in the group. 
   *
   * This is intended for simple customizations for which a custom action
   * class would be too clumsy, e.g. showing tooltips for menuitems in the
   * statusbar.
   *
   * Since: 2.4
   */
  ui_manager_signals[CONNECT_PROXY] =
    g_signal_new (I_("connect-proxy"),
		  G_OBJECT_CLASS_TYPE (klass),
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
		  G_STRUCT_OFFSET (CtkUIManagerClass, connect_proxy),
		  NULL, NULL,
		  _ctk_marshal_VOID__OBJECT_OBJECT,
		  G_TYPE_NONE, 2, 
		  CTK_TYPE_ACTION,
		  CTK_TYPE_WIDGET);

  /**
   * CtkUIManager::disconnect-proxy:
   * @manager: the ui manager
   * @action: the action
   * @proxy: the proxy
   *
   * The ::disconnect-proxy signal is emitted after disconnecting a proxy
   * from an action in the group. 
   *
   * Since: 2.4
   */
  ui_manager_signals[DISCONNECT_PROXY] =
    g_signal_new (I_("disconnect-proxy"),
		  G_OBJECT_CLASS_TYPE (klass),
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
		  G_STRUCT_OFFSET (CtkUIManagerClass, disconnect_proxy),
		  NULL, NULL,
		  _ctk_marshal_VOID__OBJECT_OBJECT,
		  G_TYPE_NONE, 2,
		  CTK_TYPE_ACTION,
		  CTK_TYPE_WIDGET);

  /**
   * CtkUIManager::pre-activate:
   * @manager: the ui manager
   * @action: the action
   *
   * The ::pre-activate signal is emitted just before the @action
   * is activated.
   *
   * This is intended for applications to get notification
   * just before any action is activated.
   *
   * Since: 2.4
   */
  ui_manager_signals[PRE_ACTIVATE] =
    g_signal_new (I_("pre-activate"),
		  G_OBJECT_CLASS_TYPE (klass),
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
		  G_STRUCT_OFFSET (CtkUIManagerClass, pre_activate),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1,
		  CTK_TYPE_ACTION);

  /**
   * CtkUIManager::post-activate:
   * @manager: the ui manager
   * @action: the action
   *
   * The ::post-activate signal is emitted just after the @action
   * is activated.
   *
   * This is intended for applications to get notification
   * just after any action is activated.
   *
   * Since: 2.4
   */
  ui_manager_signals[POST_ACTIVATE] =
    g_signal_new (I_("post-activate"),
		  G_OBJECT_CLASS_TYPE (klass),
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
		  G_STRUCT_OFFSET (CtkUIManagerClass, post_activate),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1,
		  CTK_TYPE_ACTION);

  klass->add_widget = NULL;
  klass->actions_changed = NULL;
  klass->connect_proxy = NULL;
  klass->disconnect_proxy = NULL;
  klass->pre_activate = NULL;
  klass->post_activate = NULL;
}

static void
ctk_ui_manager_init (CtkUIManager *manager)
{
  guint merge_id;
  GNode *node;

  manager->private_data = ctk_ui_manager_get_instance_private (manager);

  manager->private_data->accel_group = ctk_accel_group_new ();

  manager->private_data->root_node = NULL;
  manager->private_data->action_groups = NULL;

  manager->private_data->last_merge_id = 0;
  manager->private_data->add_tearoffs = FALSE;

  merge_id = ctk_ui_manager_new_merge_id (manager);
  node = get_child_node (manager, NULL, NULL, "ui", 2,
			 NODE_TYPE_ROOT, TRUE, FALSE);
  node_prepend_ui_reference (node, merge_id, 0);
}

static void
ctk_ui_manager_finalize (GObject *object)
{
  CtkUIManager *manager = CTK_UI_MANAGER (object);
  
  if (manager->private_data->update_tag != 0)
    {
      g_source_remove (manager->private_data->update_tag);
      manager->private_data->update_tag = 0;
    }
  
  g_node_traverse (manager->private_data->root_node, 
		   G_POST_ORDER, G_TRAVERSE_ALL, -1,
		   (GNodeTraverseFunc)free_node, NULL);
  g_node_destroy (manager->private_data->root_node);
  manager->private_data->root_node = NULL;
  
  g_list_free_full (manager->private_data->action_groups, g_object_unref);
  manager->private_data->action_groups = NULL;

  g_object_unref (manager->private_data->accel_group);
  manager->private_data->accel_group = NULL;

  G_OBJECT_CLASS (ctk_ui_manager_parent_class)->finalize (object);
}

static void
ctk_ui_manager_buildable_init (CtkBuildableIface *iface)
{
  iface->add_child = ctk_ui_manager_buildable_add_child;
  iface->construct_child = ctk_ui_manager_buildable_construct_child;
  iface->custom_tag_start = ctk_ui_manager_buildable_custom_tag_start;
  iface->custom_tag_end = ctk_ui_manager_buildable_custom_tag_end;
}

static void
ctk_ui_manager_buildable_add_child (CtkBuildable  *buildable,
				    CtkBuilder    *builder G_GNUC_UNUSED,
				    GObject       *child,
				    const gchar   *type G_GNUC_UNUSED)
{
  CtkUIManager *manager = CTK_UI_MANAGER (buildable);
  guint pos;

  g_return_if_fail (CTK_IS_ACTION_GROUP (child));

  pos = g_list_length (manager->private_data->action_groups);

  ctk_ui_manager_insert_action_group (manager,
				      CTK_ACTION_GROUP (child),
				      pos);
}

static void
child_hierarchy_changed_cb (CtkWidget *widget,
			    CtkWidget *unused G_GNUC_UNUSED,
			    CtkUIManager *uimgr)
{
  CtkWidget *toplevel;
  CtkAccelGroup *group;
  GSList *groups;

  toplevel = ctk_widget_get_toplevel (widget);
  if (!toplevel || !CTK_IS_WINDOW (toplevel))
    return;
  
  group = ctk_ui_manager_get_accel_group (uimgr);
  groups = ctk_accel_groups_from_object (G_OBJECT (toplevel));
  if (g_slist_find (groups, group) == NULL)
    ctk_window_add_accel_group (CTK_WINDOW (toplevel), group);

  g_signal_handlers_disconnect_by_func (widget,
					child_hierarchy_changed_cb,
					uimgr);
}

static GObject *
ctk_ui_manager_buildable_construct_child (CtkBuildable *buildable,
					  CtkBuilder   *builder G_GNUC_UNUSED,
					  const gchar  *id)
{
  CtkWidget *widget;
  char *name;

  name = g_strdup_printf ("ui/%s", id);
  widget = ctk_ui_manager_get_widget (CTK_UI_MANAGER (buildable), name);
  if (!widget)
    {
      g_error ("Unknown ui manager child: %s", name);
      g_free (name);
      return NULL;
    }
  g_free (name);

  g_signal_connect (widget, "hierarchy-changed",
		    G_CALLBACK (child_hierarchy_changed_cb),
		    CTK_UI_MANAGER (buildable));
  return G_OBJECT (g_object_ref (widget));
}

static void
ctk_ui_manager_set_property (GObject         *object,
			     guint            prop_id,
			     const GValue    *value,
			     GParamSpec      *pspec)
{
  CtkUIManager *manager = CTK_UI_MANAGER (object);
 
  switch (prop_id)
    {
    case PROP_ADD_TEAROFFS:
      ctk_ui_manager_do_set_add_tearoffs (manager, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_ui_manager_get_property (GObject         *object,
			     guint            prop_id,
			     GValue          *value,
			     GParamSpec      *pspec)
{
  CtkUIManager *manager = CTK_UI_MANAGER (object);

  switch (prop_id)
    {
    case PROP_ADD_TEAROFFS:
      g_value_set_boolean (value, manager->private_data->add_tearoffs);
      break;
    case PROP_UI:
      g_value_take_string (value, ctk_ui_manager_get_ui (manager));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static CtkWidget *
ctk_ui_manager_real_get_widget (CtkUIManager *manager,
                                const gchar  *path)
{
  GNode *node;

  /* ensure that there are no pending updates before we get the
   * widget */
  ctk_ui_manager_ensure_update (manager);

  node = get_node (manager, path, NODE_TYPE_UNDECIDED, FALSE);

  if (node == NULL)
    return NULL;

  return NODE_INFO (node)->proxy;
}

static CtkAction *
ctk_ui_manager_real_get_action (CtkUIManager *manager,
                                const gchar  *path)
{
  GNode *node;

  /* ensure that there are no pending updates before we get
   * the action */
  ctk_ui_manager_ensure_update (manager);

  node = get_node (manager, path, NODE_TYPE_UNDECIDED, FALSE);

  if (node == NULL)
    return NULL;

  return NODE_INFO (node)->action;
}


/**
 * ctk_ui_manager_new:
 * 
 * Creates a new ui manager object.
 * 
 * Returns: a new ui manager object.
 *
 * Since: 2.4
 **/
CtkUIManager*
ctk_ui_manager_new (void)
{
  return g_object_new (CTK_TYPE_UI_MANAGER, NULL);
}


/**
 * ctk_ui_manager_get_add_tearoffs:
 * @manager: a #CtkUIManager
 * 
 * Returns whether menus generated by this #CtkUIManager
 * will have tearoff menu items. 
 * 
 * Returns: whether tearoff menu items are added
 *
 * Since: 2.4
 **/
gboolean 
ctk_ui_manager_get_add_tearoffs (CtkUIManager *manager)
{
  g_return_val_if_fail (CTK_IS_UI_MANAGER (manager), FALSE);
  
  return manager->private_data->add_tearoffs;
}


/**
 * ctk_ui_manager_set_add_tearoffs:
 * @manager: a #CtkUIManager
 * @add_tearoffs: whether tearoff menu items are added
 * 
 * Sets the “add_tearoffs” property, which controls whether menus 
 * generated by this #CtkUIManager will have tearoff menu items. 
 *
 * Note that this only affects regular menus. Generated popup 
 * menus never have tearoff menu items.
 *
 * Since: 2.4
 **/
void
ctk_ui_manager_set_add_tearoffs (CtkUIManager *manager,
                                 gboolean      add_tearoffs)
{
  g_return_if_fail (CTK_IS_UI_MANAGER (manager));

  ctk_ui_manager_do_set_add_tearoffs (manager, add_tearoffs);
}

static void
ctk_ui_manager_do_set_add_tearoffs (CtkUIManager *manager,
                                    gboolean      add_tearoffs)
{
  add_tearoffs = add_tearoffs != FALSE;

  if (add_tearoffs != manager->private_data->add_tearoffs)
    {
      manager->private_data->add_tearoffs = add_tearoffs;

      dirty_all_nodes (manager);

      g_object_notify (G_OBJECT (manager), "add-tearoffs");
    }
}

static void
cb_proxy_connect_proxy (CtkActionGroup *group G_GNUC_UNUSED,
                        CtkAction      *action,
                        CtkWidget      *proxy,
                        CtkUIManager *manager)
{
  g_signal_emit (manager, ui_manager_signals[CONNECT_PROXY], 0, action, proxy);
}

static void
cb_proxy_disconnect_proxy (CtkActionGroup *group G_GNUC_UNUSED,
                           CtkAction      *action,
                           CtkWidget      *proxy,
                           CtkUIManager *manager)
{
  g_signal_emit (manager, ui_manager_signals[DISCONNECT_PROXY], 0, action, proxy);
}

static void
cb_proxy_pre_activate (CtkActionGroup *group G_GNUC_UNUSED,
                       CtkAction      *action,
                       CtkUIManager   *manager)
{
  g_signal_emit (manager, ui_manager_signals[PRE_ACTIVATE], 0, action);
}

static void
cb_proxy_post_activate (CtkActionGroup *group G_GNUC_UNUSED,
                        CtkAction      *action,
                        CtkUIManager   *manager)
{
  g_signal_emit (manager, ui_manager_signals[POST_ACTIVATE], 0, action);
}

/**
 * ctk_ui_manager_insert_action_group:
 * @manager: a #CtkUIManager object
 * @action_group: the action group to be inserted
 * @pos: the position at which the group will be inserted.
 * 
 * Inserts an action group into the list of action groups associated 
 * with @manager. Actions in earlier groups hide actions with the same 
 * name in later groups. 
 *
 * If @pos is larger than the number of action groups in @manager, or
 * negative, @action_group will be inserted at the end of the internal
 * list.
 *
 * Since: 2.4
 **/
void
ctk_ui_manager_insert_action_group (CtkUIManager   *manager,
				    CtkActionGroup *action_group, 
				    gint            pos)
{
#ifdef G_ENABLE_DEBUG
  GList *l;
  const char *group_name;
#endif 

  g_return_if_fail (CTK_IS_UI_MANAGER (manager));
  g_return_if_fail (CTK_IS_ACTION_GROUP (action_group));
  g_return_if_fail (g_list_find (manager->private_data->action_groups, 
				 action_group) == NULL);

#ifdef G_ENABLE_DEBUG
  group_name  = ctk_action_group_get_name (action_group);

  for (l = manager->private_data->action_groups; l; l = l->next) 
    {
      CtkActionGroup *group = l->data;

      if (strcmp (ctk_action_group_get_name (group), group_name) == 0)
        {
          g_warning ("Inserting action group '%s' into UI manager which "
		     "already has a group with this name", group_name);
          break;
        }
    }
#endif /* G_ENABLE_DEBUG */

  g_object_ref (action_group);
  manager->private_data->action_groups = 
    g_list_insert (manager->private_data->action_groups, action_group, pos);
  g_object_connect (action_group,
		    "object-signal::connect-proxy", G_CALLBACK (cb_proxy_connect_proxy), manager,
		    "object-signal::disconnect-proxy", G_CALLBACK (cb_proxy_disconnect_proxy), manager,
		    "object-signal::pre-activate", G_CALLBACK (cb_proxy_pre_activate), manager,
		    "object-signal::post-activate", G_CALLBACK (cb_proxy_post_activate), manager,
		    NULL);

  /* dirty all nodes, as action bindings may change */
  dirty_all_nodes (manager);

  g_signal_emit (manager, ui_manager_signals[ACTIONS_CHANGED], 0);
}

/**
 * ctk_ui_manager_remove_action_group:
 * @manager: a #CtkUIManager object
 * @action_group: the action group to be removed
 * 
 * Removes an action group from the list of action groups associated 
 * with @manager.
 *
 * Since: 2.4
 **/
void
ctk_ui_manager_remove_action_group (CtkUIManager   *manager,
				    CtkActionGroup *action_group)
{
  g_return_if_fail (CTK_IS_UI_MANAGER (manager));
  g_return_if_fail (CTK_IS_ACTION_GROUP (action_group));
  g_return_if_fail (g_list_find (manager->private_data->action_groups, 
				 action_group) != NULL);

  manager->private_data->action_groups =
    g_list_remove (manager->private_data->action_groups, action_group);

  g_object_disconnect (action_group,
                       "any-signal::connect-proxy", G_CALLBACK (cb_proxy_connect_proxy), manager,
                       "any-signal::disconnect-proxy", G_CALLBACK (cb_proxy_disconnect_proxy), manager,
                       "any-signal::pre-activate", G_CALLBACK (cb_proxy_pre_activate), manager,
                       "any-signal::post-activate", G_CALLBACK (cb_proxy_post_activate), manager, 
                       NULL);
  g_object_unref (action_group);

  /* dirty all nodes, as action bindings may change */
  dirty_all_nodes (manager);

  g_signal_emit (manager, ui_manager_signals[ACTIONS_CHANGED], 0);
}

/**
 * ctk_ui_manager_get_action_groups:
 * @manager: a #CtkUIManager object
 * 
 * Returns the list of action groups associated with @manager.
 *
 * Returns:  (element-type CtkActionGroup) (transfer none): a #GList of
 *   action groups. The list is owned by CTK+
 *   and should not be modified.
 *
 * Since: 2.4
 **/
GList *
ctk_ui_manager_get_action_groups (CtkUIManager *manager)
{
  g_return_val_if_fail (CTK_IS_UI_MANAGER (manager), NULL);

  return manager->private_data->action_groups;
}

/**
 * ctk_ui_manager_get_accel_group:
 * @manager: a #CtkUIManager object
 * 
 * Returns the #CtkAccelGroup associated with @manager.
 *
 * Returns: (transfer none): the #CtkAccelGroup.
 *
 * Since: 2.4
 **/
CtkAccelGroup *
ctk_ui_manager_get_accel_group (CtkUIManager *manager)
{
  g_return_val_if_fail (CTK_IS_UI_MANAGER (manager), NULL);

  return manager->private_data->accel_group;
}

/**
 * ctk_ui_manager_get_widget:
 * @manager: a #CtkUIManager
 * @path: a path
 * 
 * Looks up a widget by following a path. 
 * The path consists of the names specified in the XML description of the UI. 
 * separated by “/”. Elements which don’t have a name or action attribute in 
 * the XML (e.g. <popup>) can be addressed by their XML element name 
 * (e.g. "popup"). The root element ("/ui") can be omitted in the path.
 *
 * Note that the widget found by following a path that ends in a <menu>;
 * element is the menuitem to which the menu is attached, not the menu it
 * manages.
 *
 * Also note that the widgets constructed by a ui manager are not tied to 
 * the lifecycle of the ui manager. If you add the widgets returned by this 
 * function to some container or explicitly ref them, they will survive the
 * destruction of the ui manager.
 *
 * Returns: (transfer none): the widget found by following the path,
 *     or %NULL if no widget was found
 *
 * Since: 2.4
 **/
CtkWidget *
ctk_ui_manager_get_widget (CtkUIManager *manager,
			   const gchar  *path)
{
  g_return_val_if_fail (CTK_IS_UI_MANAGER (manager), NULL);
  g_return_val_if_fail (path != NULL, NULL);

  return CTK_UI_MANAGER_GET_CLASS (manager)->get_widget (manager, path);
}

typedef struct {
  CtkUIManagerItemType types;
  GSList *list;
} ToplevelData;

static void
collect_toplevels (GNode   *node, 
		   gpointer user_data)
{
  ToplevelData *data = user_data;

  if (NODE_INFO (node)->proxy)
    {
      switch (NODE_INFO (node)->type) 
	{
	case NODE_TYPE_MENUBAR:
	  if (data->types & CTK_UI_MANAGER_MENUBAR)
	data->list = g_slist_prepend (data->list, NODE_INFO (node)->proxy);
	  break;
	case NODE_TYPE_TOOLBAR:
      if (data->types & CTK_UI_MANAGER_TOOLBAR)
	data->list = g_slist_prepend (data->list, NODE_INFO (node)->proxy);
      break;
	case NODE_TYPE_POPUP:
	  if (data->types & CTK_UI_MANAGER_POPUP)
	    data->list = g_slist_prepend (data->list, NODE_INFO (node)->proxy);
	  break;
	default: ;
	}
    }
}

/**
 * ctk_ui_manager_get_toplevels:
 * @manager: a #CtkUIManager
 * @types: specifies the types of toplevel widgets to include. Allowed
 *   types are #CTK_UI_MANAGER_MENUBAR, #CTK_UI_MANAGER_TOOLBAR and
 *   #CTK_UI_MANAGER_POPUP.
 * 
 * Obtains a list of all toplevel widgets of the requested types.
 *
 * Returns: (element-type CtkWidget) (transfer container): a newly-allocated #GSList of
 * all toplevel widgets of the requested types.  Free the returned list with g_slist_free().
 *
 * Since: 2.4
 **/
GSList *
ctk_ui_manager_get_toplevels (CtkUIManager         *manager,
			      CtkUIManagerItemType  types)
{
  ToplevelData data;

  g_return_val_if_fail (CTK_IS_UI_MANAGER (manager), NULL);
  g_return_val_if_fail ((~(CTK_UI_MANAGER_MENUBAR | 
			   CTK_UI_MANAGER_TOOLBAR |
			   CTK_UI_MANAGER_POPUP) & types) == 0, NULL);
  
      
  data.types = types;
  data.list = NULL;

  g_node_children_foreach (manager->private_data->root_node, 
			   G_TRAVERSE_ALL, 
			   collect_toplevels, &data);

  return data.list;
}


/**
 * ctk_ui_manager_get_action:
 * @manager: a #CtkUIManager
 * @path: a path
 * 
 * Looks up an action by following a path. See ctk_ui_manager_get_widget()
 * for more information about paths.
 * 
 * Returns: (transfer none): the action whose proxy widget is found by following the path, 
 *     or %NULL if no widget was found.
 *
 * Since: 2.4
 **/
CtkAction *
ctk_ui_manager_get_action (CtkUIManager *manager,
			   const gchar  *path)
{
  g_return_val_if_fail (CTK_IS_UI_MANAGER (manager), NULL);
  g_return_val_if_fail (path != NULL, NULL);

  return CTK_UI_MANAGER_GET_CLASS (manager)->get_action (manager, path);
}

static gboolean
node_is_dead (GNode *node)
{
  GNode *child;

  if (NODE_INFO (node)->uifiles != NULL)
    return FALSE;

  for (child = node->children; child != NULL; child = child->next)
    {
      if (!node_is_dead (child))
	return FALSE;
    }

  return TRUE;
}

static GNode *
get_child_node (CtkUIManager *manager, 
		GNode        *parent,
		GNode        *sibling,
		const gchar  *childname, 
		gint          childname_length,
		NodeType      node_type,
		gboolean      create, 
		gboolean      top)
{
  GNode *child = NULL;

  if (parent)
    {
      if (childname)
	{
	  for (child = parent->children; child != NULL; child = child->next)
	    {
	      if (NODE_INFO (child)->name &&
		  strlen (NODE_INFO (child)->name) == childname_length &&
		  !strncmp (NODE_INFO (child)->name, childname, childname_length))
		{
		  /* if undecided about node type, set it */
		  if (NODE_INFO (child)->type == NODE_TYPE_UNDECIDED)
		    NODE_INFO (child)->type = node_type;
		  
		  /* warn about type mismatch */
		  if (NODE_INFO (child)->type != NODE_TYPE_UNDECIDED &&
		      node_type != NODE_TYPE_UNDECIDED &&
		      NODE_INFO (child)->type != node_type)
		    g_warning ("node type doesn't match %d (%s is type %d)",
			       node_type, 
			       NODE_INFO (child)->name,
			       NODE_INFO (child)->type);

                  if (node_is_dead (child))
                    {
                      /* This node was removed but is still dirty so
                       * it is still in the tree. We want to treat this
                       * as if it didn't exist, which means we move it
                       * to the position it would have been created at.
                       */
                      g_node_unlink (child);
                      goto insert_child;
                    }

		  return child;
		}
	    }
	}
      if (!child && create)
	{
	  Node *mnode;
	  
	  mnode = g_slice_new0 (Node);
	  mnode->type = node_type;
	  mnode->name = g_strndup (childname, childname_length);

	  child = g_node_new (mnode);
	insert_child:
	  if (sibling)
	    {
	      if (top)
		g_node_insert_before (parent, sibling, child);
	      else
		g_node_insert_after (parent, sibling, child);
	    }
	  else
	    {
	      if (top)
		g_node_prepend (parent, child);
	      else
		g_node_append (parent, child);
	    }

	  mark_node_dirty (child);
	}
    }
  else
    {
      /* handle root node */
      if (manager->private_data->root_node)
	{
	  child = manager->private_data->root_node;
	  if (strncmp (NODE_INFO (child)->name, childname, childname_length) != 0)
	    g_warning ("root node name '%s' doesn't match '%s'",
		       childname, NODE_INFO (child)->name);
	  if (NODE_INFO (child)->type != NODE_TYPE_ROOT)
	    g_warning ("base element must be of type ROOT");
	}
      else if (create)
	{
	  Node *mnode;

	  mnode = g_slice_new0 (Node);
	  mnode->type = node_type;
	  mnode->name = g_strndup (childname, childname_length);
	  mnode->dirty = TRUE;
	  
	  child = manager->private_data->root_node = g_node_new (mnode);
	}
    }

  return child;
}

static GNode *
get_node (CtkUIManager *manager, 
	  const gchar  *path,
	  NodeType      node_type, 
	  gboolean      create)
{
  const gchar *pos, *end;
  GNode *parent, *node;
  
  if (strncmp ("/ui", path, 3) == 0)
    path += 3;
  
  end = path + strlen (path);
  pos = path;
  parent = node = NULL;
  while (pos < end)
    {
      const gchar *slash;
      gsize length;

      slash = strchr (pos, '/');
      if (slash)
	length = slash - pos;
      else
	length = strlen (pos);

      node = get_child_node (manager, parent, NULL, pos, length, NODE_TYPE_UNDECIDED,
			     create, FALSE);
      if (!node)
	return NULL;

      pos += length + 1; /* move past the node name and the slash too */
      parent = node;
    }

  if (node != NULL && NODE_INFO (node)->type == NODE_TYPE_UNDECIDED)
    NODE_INFO (node)->type = node_type;

  return node;
}

static void
node_ui_reference_free (gpointer data)
{
  g_slice_free (NodeUIReference, data);
}

static gboolean
free_node (GNode *node)
{
  Node *info = NODE_INFO (node);

  g_list_free_full (info->uifiles, node_ui_reference_free);
  info->uifiles = NULL;

  g_clear_object (&info->action);
  g_clear_object (&info->proxy);
  g_clear_object (&info->extra);
  g_clear_pointer (&info->name, g_free);
  g_slice_free (Node, info);
  node->data = NULL;

  return FALSE;
}

/**
 * ctk_ui_manager_new_merge_id:
 * @manager: a #CtkUIManager
 * 
 * Returns an unused merge id, suitable for use with 
 * ctk_ui_manager_add_ui().
 * 
 * Returns: an unused merge id.
 *
 * Since: 2.4
 **/
guint
ctk_ui_manager_new_merge_id (CtkUIManager *manager)
{
  manager->private_data->last_merge_id++;

  return manager->private_data->last_merge_id;
}

static void
node_prepend_ui_reference (GNode  *gnode,
			   guint   merge_id, 
			   GQuark  action_quark)
{
  Node *node = NODE_INFO (gnode);
  NodeUIReference *reference = NULL;

  if (node->uifiles && 
      ((NodeUIReference *)node->uifiles->data)->merge_id == merge_id)
    reference = node->uifiles->data;
  else
    {
      reference = g_slice_new (NodeUIReference);
      node->uifiles = g_list_prepend (node->uifiles, reference);
    }

  reference->merge_id = merge_id;
  reference->action_quark = action_quark;

  mark_node_dirty (gnode);
}

static void
node_remove_ui_reference (GNode  *gnode,
			  guint  merge_id)
{
  Node *node = NODE_INFO (gnode);
  GList *p;
  
  for (p = node->uifiles; p != NULL; p = p->next)
    {
      NodeUIReference *reference = p->data;
      
      if (reference->merge_id == merge_id)
	{
	  if (p == node->uifiles)
	    mark_node_dirty (gnode);
	  node->uifiles = g_list_delete_link (node->uifiles, p);
	  g_slice_free (NodeUIReference, reference);

	  break;
	}
    }
}

/* -------------------- The UI file parser -------------------- */

typedef enum
{
  STATE_START,
  STATE_ROOT,
  STATE_MENU,
  STATE_TOOLBAR,
  STATE_MENUITEM,
  STATE_TOOLITEM,
  STATE_ACCELERATOR,
  STATE_END
} ParseState;

typedef struct _ParseContext ParseContext;
struct _ParseContext
{
  ParseState state;
  ParseState prev_state;

  CtkUIManager *manager;

  GNode *current;

  guint merge_id;
};

static void
start_element_handler (GMarkupParseContext *context,
		       const gchar         *element_name,
		       const gchar        **attribute_names,
		       const gchar        **attribute_values,
		       gpointer             user_data,
		       GError             **error)
{
  ParseContext *ctx = user_data;
  CtkUIManager *manager = ctx->manager;

  gint i;
  const gchar *node_name;
  const gchar *action;
  GQuark action_quark;
  gboolean top;
  gboolean expand = FALSE;
  gboolean accelerators = FALSE;
  gboolean always_show_image_set = FALSE, always_show_image = FALSE;

  gboolean raise_error = TRUE;

  node_name = NULL;
  action = NULL;
  action_quark = 0;
  top = FALSE;

  for (i = 0; attribute_names[i] != NULL; i++)
    {
      if (!strcmp (attribute_names[i], "name"))
	{
	  node_name = attribute_values[i];
	}
      else if (!strcmp (attribute_names[i], "action"))
	{
	  action = attribute_values[i];
	  action_quark = g_quark_from_string (attribute_values[i]);
	}
      else if (!strcmp (attribute_names[i], "position"))
	{
	  top = !strcmp (attribute_values[i], "top");
	}
      else if (!strcmp (attribute_names[i], "expand"))
	{
	  expand = !strcmp (attribute_values[i], "true");
	}
      else if (!strcmp (attribute_names[i], "accelerators"))
        {
          accelerators = !strcmp (attribute_values[i], "true");
        }
      else if (!strcmp (attribute_names[i], "always-show-image"))
        {
          always_show_image_set = TRUE;
          always_show_image = !strcmp (attribute_values[i], "true");
        }
      /*  else silently skip unknown attributes to be compatible with
       *  future additional attributes.
       */
    }

  /* Work out a name for this node.  Either the name attribute, or
   * the action, or the element name */
  if (node_name == NULL) 
    {
      if (action != NULL)
	node_name = action;
      else 
	node_name = element_name;
    }

  switch (element_name[0])
    {
    case 'a':
      if (ctx->state == STATE_ROOT && !strcmp (element_name, "accelerator"))
	{
	  ctx->state = STATE_ACCELERATOR;
	  ctx->current = get_child_node (manager, ctx->current, NULL,
					 node_name, strlen (node_name),
					 NODE_TYPE_ACCELERATOR,
					 TRUE, FALSE);
	  if (NODE_INFO (ctx->current)->action_name == 0)
	    NODE_INFO (ctx->current)->action_name = action_quark;

	  node_prepend_ui_reference (ctx->current, ctx->merge_id, action_quark);

	  raise_error = FALSE;
	}
      break;
    case 'u':
      if (ctx->state == STATE_START && !strcmp (element_name, "ui"))
	{
	  ctx->state = STATE_ROOT;
	  ctx->current = manager->private_data->root_node;
	  raise_error = FALSE;

	  node_prepend_ui_reference (ctx->current, ctx->merge_id, action_quark);
	}
      break;
    case 'm':
      if (ctx->state == STATE_ROOT && !strcmp (element_name, "menubar"))
	{
	  ctx->state = STATE_MENU;
	  ctx->current = get_child_node (manager, ctx->current, NULL,
					 node_name, strlen (node_name),
					 NODE_TYPE_MENUBAR,
					 TRUE, FALSE);
	  if (NODE_INFO (ctx->current)->action_name == 0)
	    NODE_INFO (ctx->current)->action_name = action_quark;

	  node_prepend_ui_reference (ctx->current, ctx->merge_id, action_quark);
	  mark_node_dirty (ctx->current);

	  raise_error = FALSE;
	}
      else if (ctx->state == STATE_MENU && !strcmp (element_name, "menu"))
	{
	  ctx->current = get_child_node (manager, ctx->current, NULL,
					 node_name, strlen (node_name),
					 NODE_TYPE_MENU,
					 TRUE, top);
	  if (NODE_INFO (ctx->current)->action_name == 0)
	    NODE_INFO (ctx->current)->action_name = action_quark;

	  node_prepend_ui_reference (ctx->current, ctx->merge_id, action_quark);
	  
	  raise_error = FALSE;
	}
      else if (ctx->state == STATE_TOOLITEM && !strcmp (element_name, "menu"))
	{
	  ctx->state = STATE_MENU;
	  
	  ctx->current = get_child_node (manager, g_node_last_child (ctx->current), NULL,
					 node_name, strlen (node_name),
					 NODE_TYPE_MENU,
					 TRUE, top);
	  if (NODE_INFO (ctx->current)->action_name == 0)
	    NODE_INFO (ctx->current)->action_name = action_quark;

	  node_prepend_ui_reference (ctx->current, ctx->merge_id, action_quark);
	  
	  raise_error = FALSE;
	}
      else if (ctx->state == STATE_MENU && !strcmp (element_name, "menuitem"))
	{
	  GNode *node;

	  ctx->state = STATE_MENUITEM;
	  node = get_child_node (manager, ctx->current, NULL,
				 node_name, strlen (node_name),
				 NODE_TYPE_MENUITEM,
				 TRUE, top);
	  if (NODE_INFO (node)->action_name == 0)
	    NODE_INFO (node)->action_name = action_quark;

	  NODE_INFO (node)->always_show_image_set = always_show_image_set;
	  NODE_INFO (node)->always_show_image = always_show_image;

	  node_prepend_ui_reference (node, ctx->merge_id, action_quark);
	  
	  raise_error = FALSE;
	}
      break;
    case 'p':
      if (ctx->state == STATE_ROOT && !strcmp (element_name, "popup"))
	{
	  ctx->state = STATE_MENU;
	  ctx->current = get_child_node (manager, ctx->current, NULL,
					 node_name, strlen (node_name),
					 NODE_TYPE_POPUP,
					 TRUE, FALSE);

          NODE_INFO (ctx->current)->popup_accels = accelerators;

	  if (NODE_INFO (ctx->current)->action_name == 0)
	    NODE_INFO (ctx->current)->action_name = action_quark;
	  
	  node_prepend_ui_reference (ctx->current, ctx->merge_id, action_quark);
	  
	  raise_error = FALSE;
	}
      else if ((ctx->state == STATE_MENU || ctx->state == STATE_TOOLBAR) &&
	       !strcmp (element_name, "placeholder"))
	{
	  if (ctx->state == STATE_TOOLBAR)
	    ctx->current = get_child_node (manager, ctx->current, NULL,
					   node_name, strlen (node_name),
					   NODE_TYPE_TOOLBAR_PLACEHOLDER,
					   TRUE, top);
	  else
	    ctx->current = get_child_node (manager, ctx->current, NULL,
					   node_name, strlen (node_name),
					   NODE_TYPE_MENU_PLACEHOLDER,
					   TRUE, top);
	  
	  node_prepend_ui_reference (ctx->current, ctx->merge_id, action_quark);
	  
	  raise_error = FALSE;
	}
      break;
    case 's':
      if ((ctx->state == STATE_MENU || ctx->state == STATE_TOOLBAR) &&
	  !strcmp (element_name, "separator"))
	{
	  GNode *node;
	  gint length;

	  if (ctx->state == STATE_TOOLBAR)
	    ctx->state = STATE_TOOLITEM;
	  else
	    ctx->state = STATE_MENUITEM;
	  if (!strcmp (node_name, "separator"))
	    {
	      node_name = NULL;
	      length = 0;
	    }
	  else
	    length = strlen (node_name);
	  node = get_child_node (manager, ctx->current, NULL,
				 node_name, length,
				 NODE_TYPE_SEPARATOR,
				 TRUE, top);

	  NODE_INFO (node)->expand = expand;

	  if (NODE_INFO (node)->action_name == 0)
	    NODE_INFO (node)->action_name = action_quark;

	  node_prepend_ui_reference (node, ctx->merge_id, action_quark);
	  
	  raise_error = FALSE;
	}
      break;
    case 't':
      if (ctx->state == STATE_ROOT && !strcmp (element_name, "toolbar"))
	{
	  ctx->state = STATE_TOOLBAR;
	  ctx->current = get_child_node (manager, ctx->current, NULL,
					 node_name, strlen (node_name),
					 NODE_TYPE_TOOLBAR,
					 TRUE, FALSE);
	  if (NODE_INFO (ctx->current)->action_name == 0)
	    NODE_INFO (ctx->current)->action_name = action_quark;
	  
	  node_prepend_ui_reference (ctx->current, ctx->merge_id, action_quark);
	  
	  raise_error = FALSE;
	}
      else if (ctx->state == STATE_TOOLBAR && !strcmp (element_name, "toolitem"))
	{
	  GNode *node;

	  ctx->state = STATE_TOOLITEM;
	  node = get_child_node (manager, ctx->current, NULL,
				node_name, strlen (node_name),
				 NODE_TYPE_TOOLITEM,
				 TRUE, top);
	  if (NODE_INFO (node)->action_name == 0)
	    NODE_INFO (node)->action_name = action_quark;
	  
	  node_prepend_ui_reference (node, ctx->merge_id, action_quark);

	  raise_error = FALSE;
	}
      break;
    default:
      break;
    }
  if (raise_error)
    {
      gint line_number, char_number;
 
      g_markup_parse_context_get_position (context,
					   &line_number, &char_number);
      g_set_error (error,
		   G_MARKUP_ERROR,
		   G_MARKUP_ERROR_UNKNOWN_ELEMENT,
		   _("Unexpected start tag '%s' on line %d char %d"),
		   element_name,
		   line_number, char_number);
    }
}

static void
end_element_handler (GMarkupParseContext *context G_GNUC_UNUSED,
		     const gchar         *element_name G_GNUC_UNUSED,
		     gpointer             user_data,
		     GError             **error G_GNUC_UNUSED)
{
  ParseContext *ctx = user_data;

  switch (ctx->state)
    {
    case STATE_START:
    case STATE_END:
      /* no need to GError here, GMarkup already catches this */
      break;
    case STATE_ROOT:
      ctx->current = NULL;
      ctx->state = STATE_END;
      break;
    case STATE_MENU:
    case STATE_TOOLBAR:
    case STATE_ACCELERATOR:
      ctx->current = ctx->current->parent;
      if (NODE_INFO (ctx->current)->type == NODE_TYPE_ROOT) 
	ctx->state = STATE_ROOT;
      else if (NODE_INFO (ctx->current)->type == NODE_TYPE_TOOLITEM)
	{
	  ctx->current = ctx->current->parent;
	  ctx->state = STATE_TOOLITEM;
	}
      /* else, stay in same state */
      break;
    case STATE_MENUITEM:
      ctx->state = STATE_MENU;
      break;
    case STATE_TOOLITEM:
      ctx->state = STATE_TOOLBAR;
      break;
    }
}

static void
cleanup (GMarkupParseContext *context G_GNUC_UNUSED,
	 GError              *error G_GNUC_UNUSED,
	 gpointer             user_data)
{
  ParseContext *ctx = user_data;

  ctx->current = NULL;
  /* should also walk through the tree and get rid of nodes related to
   * this UI file's tag */

  ctk_ui_manager_remove_ui (ctx->manager, ctx->merge_id);
}

static gboolean
xml_isspace (char c)
{
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static void 
text_handler (GMarkupParseContext *context,
	      const gchar         *text,
	      gsize                text_len,  
	      gpointer             user_data G_GNUC_UNUSED,
	      GError             **error)
{
  const gchar *p;
  const gchar *end;

  p = text;
  end = text + text_len;
  while (p != end && xml_isspace (*p))
    ++p;
  
  if (p != end)
    {
      gint line_number, char_number;
      
      g_markup_parse_context_get_position (context,
					   &line_number, &char_number);
      g_set_error (error,
		   G_MARKUP_ERROR,
		   G_MARKUP_ERROR_INVALID_CONTENT,
		   _("Unexpected character data on line %d char %d"),
		   line_number, char_number);
    }
}


static const GMarkupParser ui_parser = {
  start_element_handler,
  end_element_handler,
  text_handler,
  NULL,
  cleanup
};

static guint
add_ui_from_string (CtkUIManager *manager,
		    const gchar  *buffer, 
		    gssize        length,
		    gboolean      needs_root,
		    GError      **error)
{
  ParseContext ctx = { 0 };
  GMarkupParseContext *context;

  ctx.state = STATE_START;
  ctx.manager = manager;
  ctx.current = NULL;
  ctx.merge_id = ctk_ui_manager_new_merge_id (manager);

  context = g_markup_parse_context_new (&ui_parser, 0, &ctx, NULL);

  if (needs_root)
    if (!g_markup_parse_context_parse (context, "<ui>", -1, error))
      goto out;

  if (!g_markup_parse_context_parse (context, buffer, length, error))
    goto out;

  if (needs_root)
    if (!g_markup_parse_context_parse (context, "</ui>", -1, error))
      goto out;

  if (!g_markup_parse_context_end_parse (context, error))
    goto out;

  g_markup_parse_context_free (context);

  queue_update (manager);

  g_object_notify (G_OBJECT (manager), "ui");

  return ctx.merge_id;

 out:

  g_markup_parse_context_free (context);

  return 0;
}

/**
 * ctk_ui_manager_add_ui_from_string:
 * @manager: a #CtkUIManager object
 * @buffer: the string to parse
 * @length: the length of @buffer (may be -1 if @buffer is nul-terminated)
 * @error: return location for an error
 * 
 * Parses a string containing a [UI definition][XML-UI] and merges it with
 * the current contents of @manager. An enclosing <ui> element is added if
 * it is missing.
 * 
 * Returns: The merge id for the merged UI. The merge id can be used
 *   to unmerge the UI with ctk_ui_manager_remove_ui(). If an error occurred,
 *   the return value is 0.
 *
 * Since: 2.4
 **/
guint
ctk_ui_manager_add_ui_from_string (CtkUIManager *manager,
				   const gchar  *buffer, 
				   gssize        length,
				   GError      **error)
{
  gboolean needs_root = TRUE;
  const gchar *p;
  const gchar *end;

  g_return_val_if_fail (CTK_IS_UI_MANAGER (manager), 0);
  g_return_val_if_fail (buffer != NULL, 0);

  if (length < 0)
    length = strlen (buffer);

  p = buffer;
  end = buffer + length;
  while (p != end && xml_isspace (*p))
    ++p;

  if (end - p >= 4 && strncmp (p, "<ui>", 4) == 0)
    needs_root = FALSE;
  
  return add_ui_from_string (manager, buffer, length, needs_root, error);
}

/**
 * ctk_ui_manager_add_ui_from_file:
 * @manager: a #CtkUIManager object
 * @filename: (type filename): the name of the file to parse 
 * @error: return location for an error
 * 
 * Parses a file containing a [UI definition][XML-UI] and 
 * merges it with the current contents of @manager. 
 * 
 * Returns: The merge id for the merged UI. The merge id can be used
 *   to unmerge the UI with ctk_ui_manager_remove_ui(). If an error occurred,
 *   the return value is 0.
 *
 * Since: 2.4
 **/
guint
ctk_ui_manager_add_ui_from_file (CtkUIManager *manager,
				 const gchar  *filename,
				 GError      **error)
{
  gchar *buffer;
  gsize length;
  guint res;

  g_return_val_if_fail (CTK_IS_UI_MANAGER (manager), 0);

  if (!g_file_get_contents (filename, &buffer, &length, error))
    return 0;

  res = add_ui_from_string (manager, buffer, length, FALSE, error);
  g_free (buffer);

  return res;
}

/**
 * ctk_ui_manager_add_ui_from_resource:
 * @manager: a #CtkUIManager object
 * @resource_path: the resource path of the file to parse
 * @error: return location for an error
 *
 * Parses a resource file containing a [UI definition][XML-UI] and
 * merges it with the current contents of @manager.
 *
 * Returns: The merge id for the merged UI. The merge id can be used
 *   to unmerge the UI with ctk_ui_manager_remove_ui(). If an error occurred,
 *   the return value is 0.
 *
 * Since: 3.4
 **/
guint
ctk_ui_manager_add_ui_from_resource (CtkUIManager *manager,
				     const gchar  *resource_path,
				     GError      **error)
{
  GBytes *data;
  guint res;

  g_return_val_if_fail (CTK_IS_UI_MANAGER (manager), 0);

  data = g_resources_lookup_data (resource_path, 0, error);
  if (data == NULL)
    return 0;

  res = add_ui_from_string (manager, g_bytes_get_data (data, NULL), g_bytes_get_size (data), FALSE, error);
  g_bytes_unref (data);

  return res;
}

/**
 * ctk_ui_manager_add_ui:
 * @manager: a #CtkUIManager
 * @merge_id: the merge id for the merged UI, see ctk_ui_manager_new_merge_id()
 * @path: a path
 * @name: the name for the added UI element
 * @action: (allow-none): the name of the action to be proxied, or %NULL to add a separator
 * @type: the type of UI element to add.
 * @top: if %TRUE, the UI element is added before its siblings, otherwise it
 *   is added after its siblings.
 *
 * Adds a UI element to the current contents of @manager. 
 *
 * If @type is %CTK_UI_MANAGER_AUTO, CTK+ inserts a menuitem, toolitem or 
 * separator if such an element can be inserted at the place determined by 
 * @path. Otherwise @type must indicate an element that can be inserted at 
 * the place determined by @path.
 *
 * If @path points to a menuitem or toolitem, the new element will be inserted
 * before or after this item, depending on @top.
 * 
 * Since: 2.4
 **/
void
ctk_ui_manager_add_ui (CtkUIManager        *manager,
		       guint                merge_id,
		       const gchar         *path,
		       const gchar         *name,
		       const gchar         *action,
		       CtkUIManagerItemType type,
		       gboolean             top)
{
  GNode *node;
  GNode *sibling;
  GNode *child;
  NodeType node_type;
  GQuark action_quark = 0;

  g_return_if_fail (CTK_IS_UI_MANAGER (manager));  
  g_return_if_fail (merge_id > 0);
  g_return_if_fail (name != NULL || type == CTK_UI_MANAGER_SEPARATOR);

  node = get_node (manager, path, NODE_TYPE_UNDECIDED, FALSE);
  sibling = NULL;

  if (node == NULL)
    return;

  node_type = NODE_TYPE_UNDECIDED;

 reswitch:
  switch (NODE_INFO (node)->type) 
    {
    case NODE_TYPE_SEPARATOR:
    case NODE_TYPE_MENUITEM:
    case NODE_TYPE_TOOLITEM:
      sibling = node;
      node = node->parent;
      goto reswitch;
    case NODE_TYPE_MENUBAR:
    case NODE_TYPE_MENU:
    case NODE_TYPE_POPUP:
    case NODE_TYPE_MENU_PLACEHOLDER:
      switch (type) 
	{
	case CTK_UI_MANAGER_AUTO:
	  if (action != NULL)
	      node_type = NODE_TYPE_MENUITEM;
	  else
	      node_type = NODE_TYPE_SEPARATOR;
	  break;
	case CTK_UI_MANAGER_MENU:
	  node_type = NODE_TYPE_MENU;
	  break;
	case CTK_UI_MANAGER_MENUITEM:
	  node_type = NODE_TYPE_MENUITEM;
	  break;
	case CTK_UI_MANAGER_SEPARATOR:
	  node_type = NODE_TYPE_SEPARATOR;
	  break;
	case CTK_UI_MANAGER_PLACEHOLDER:
	  node_type = NODE_TYPE_MENU_PLACEHOLDER;
	  break;
	default: ;
	  /* do nothing */
	}
      break;
    case NODE_TYPE_TOOLBAR:
    case NODE_TYPE_TOOLBAR_PLACEHOLDER:
      switch (type) 
	{
	case CTK_UI_MANAGER_AUTO:
	  if (action != NULL)
	      node_type = NODE_TYPE_TOOLITEM;
	  else
	      node_type = NODE_TYPE_SEPARATOR;
	  break;
	case CTK_UI_MANAGER_TOOLITEM:
	  node_type = NODE_TYPE_TOOLITEM;
	  break;
	case CTK_UI_MANAGER_SEPARATOR:
	  node_type = NODE_TYPE_SEPARATOR;
	  break;
	case CTK_UI_MANAGER_PLACEHOLDER:
	  node_type = NODE_TYPE_TOOLBAR_PLACEHOLDER;
	  break;
	default: ;
	  /* do nothing */
	}
      break;
    case NODE_TYPE_ROOT:
      switch (type) 
	{
	case CTK_UI_MANAGER_MENUBAR:
	  node_type = NODE_TYPE_MENUBAR;
	  break;
	case CTK_UI_MANAGER_TOOLBAR:
	  node_type = NODE_TYPE_TOOLBAR;
	  break;
	case CTK_UI_MANAGER_POPUP:
	case CTK_UI_MANAGER_POPUP_WITH_ACCELS:
	  node_type = NODE_TYPE_POPUP;
	  break;
	case CTK_UI_MANAGER_ACCELERATOR:
	  node_type = NODE_TYPE_ACCELERATOR;
	  break;
	default: ;
	  /* do nothing */
	}
      break;
    default: ;
      /* do nothing */
    }

  if (node_type == NODE_TYPE_UNDECIDED)
    {
      g_warning ("item type %d not suitable for adding at '%s'", type, path);
      return;
    }
   
  child = get_child_node (manager, node, sibling,
			  name, name ? strlen (name) : 0,
			  node_type, TRUE, top);

  if (type == CTK_UI_MANAGER_POPUP_WITH_ACCELS)
    NODE_INFO (child)->popup_accels = TRUE;

  if (action != NULL)
    action_quark = g_quark_from_string (action);

  node_prepend_ui_reference (child, merge_id, action_quark);

  if (NODE_INFO (child)->action_name == 0)
    NODE_INFO (child)->action_name = action_quark;

  queue_update (manager);

  g_object_notify (G_OBJECT (manager), "ui");      
}

static gboolean
remove_ui (GNode   *node, 
	   gpointer user_data)
{
  guint merge_id = GPOINTER_TO_UINT (user_data);

  node_remove_ui_reference (node, merge_id);

  return FALSE; /* continue */
}

/**
 * ctk_ui_manager_remove_ui:
 * @manager: a #CtkUIManager object
 * @merge_id: a merge id as returned by ctk_ui_manager_add_ui_from_string()
 * 
 * Unmerges the part of @manager's content identified by @merge_id.
 *
 * Since: 2.4
 **/
void
ctk_ui_manager_remove_ui (CtkUIManager *manager, 
			  guint         merge_id)
{
  g_return_if_fail (CTK_IS_UI_MANAGER (manager));

  g_node_traverse (manager->private_data->root_node, 
		   G_POST_ORDER, G_TRAVERSE_ALL, -1,
		   remove_ui, GUINT_TO_POINTER (merge_id));

  queue_update (manager);

  g_object_notify (G_OBJECT (manager), "ui");      
}

/* -------------------- Updates -------------------- */


static CtkAction *
get_action_by_name (CtkUIManager *merge, 
		    const gchar  *action_name)
{
  GList *tmp;

  if (!action_name)
    return NULL;
  
  /* lookup name */
  for (tmp = merge->private_data->action_groups; tmp != NULL; tmp = tmp->next)
    {
      CtkActionGroup *action_group = tmp->data;
      CtkAction *action;
      
      action = ctk_action_group_get_action (action_group, action_name);

      if (action)
	return action;
    }

  return NULL;
}

static gboolean
find_menu_position (GNode      *node,
                    CtkWidget **menushell_p,
                    gint       *pos_p)
{
  CtkWidget *menushell;
  gint pos = 0;

  g_return_val_if_fail (node != NULL, FALSE);
  g_return_val_if_fail (NODE_INFO (node)->type == NODE_TYPE_MENU ||
		        NODE_INFO (node)->type == NODE_TYPE_POPUP ||
		        NODE_INFO (node)->type == NODE_TYPE_MENU_PLACEHOLDER ||
		        NODE_INFO (node)->type == NODE_TYPE_MENUITEM ||
		        NODE_INFO (node)->type == NODE_TYPE_SEPARATOR,
                        FALSE);

  /* first sibling -- look at parent */
  if (node->prev == NULL)
    {
      GNode *parent;
      GList *siblings;

      parent = node->parent;
      switch (NODE_INFO (parent)->type)
	{
	case NODE_TYPE_MENUBAR:
	case NODE_TYPE_POPUP:
	  menushell = NODE_INFO (parent)->proxy;
	  pos = 0;
	  break;
	case NODE_TYPE_MENU:
	  menushell = NODE_INFO (parent)->proxy;
	  if (CTK_IS_MENU_ITEM (menushell))
	    menushell = ctk_menu_item_get_submenu (CTK_MENU_ITEM (menushell));
	  siblings = ctk_container_get_children (CTK_CONTAINER (menushell));

	  if (siblings != NULL && CTK_IS_TEAROFF_MENU_ITEM (siblings->data))
	    pos = 1;
	  else
	    pos = 0;

	  g_list_free (siblings);
	  break;
	case NODE_TYPE_MENU_PLACEHOLDER:
	  menushell = ctk_widget_get_parent (NODE_INFO (parent)->proxy);
	  g_return_val_if_fail (CTK_IS_MENU_SHELL (menushell), FALSE);
	  pos = g_list_index (CTK_MENU_SHELL (menushell)->priv->children,
			      NODE_INFO (parent)->proxy) + 1;
	  break;
	default:
	  g_warning ("%s: bad parent node type %d", G_STRLOC,
		     NODE_INFO (parent)->type);
	  return FALSE;
	}
    }
  else
    {
      CtkWidget *prev_child;
      GNode *sibling;

      sibling = node->prev;
      if (NODE_INFO (sibling)->type == NODE_TYPE_MENU_PLACEHOLDER)
	prev_child = NODE_INFO (sibling)->extra; /* second Separator */
      else
	prev_child = NODE_INFO (sibling)->proxy;

      if (!CTK_IS_WIDGET (prev_child))
        return FALSE;

      menushell = ctk_widget_get_parent (prev_child);
      if (!CTK_IS_MENU_SHELL (menushell))
        return FALSE;

      pos = g_list_index (CTK_MENU_SHELL (menushell)->priv->children, prev_child) + 1;
    }

  if (menushell_p)
    *menushell_p = menushell;
  if (pos_p)
    *pos_p = pos;

  return TRUE;
}

static gboolean
find_toolbar_position (GNode      *node, 
		       CtkWidget **toolbar_p, 
		       gint       *pos_p)
{
  CtkWidget *toolbar;
  gint pos;

  g_return_val_if_fail (node != NULL, FALSE);
  g_return_val_if_fail (NODE_INFO (node)->type == NODE_TYPE_TOOLBAR ||
		        NODE_INFO (node)->type == NODE_TYPE_TOOLBAR_PLACEHOLDER ||
		        NODE_INFO (node)->type == NODE_TYPE_TOOLITEM ||
		        NODE_INFO (node)->type == NODE_TYPE_SEPARATOR,
                        FALSE);
  
  /* first sibling -- look at parent */
  if (node->prev == NULL)
    {
      GNode *parent;

      parent = node->parent;
      switch (NODE_INFO (parent)->type)
	{
	case NODE_TYPE_TOOLBAR:
	  toolbar = NODE_INFO (parent)->proxy;
	  pos = 0;
	  break;
	case NODE_TYPE_TOOLBAR_PLACEHOLDER:
	  toolbar = ctk_widget_get_parent (NODE_INFO (parent)->proxy);
	  g_return_val_if_fail (CTK_IS_TOOLBAR (toolbar), FALSE);
	  pos = ctk_toolbar_get_item_index (CTK_TOOLBAR (toolbar),
					    CTK_TOOL_ITEM (NODE_INFO (parent)->proxy)) + 1;
	  break;
	default:
	  g_warning ("%s: bad parent node type %d", G_STRLOC,
		     NODE_INFO (parent)->type);
	  return FALSE;
	}
    }
  else
    {
      CtkWidget *prev_child;
      GNode *sibling;

      sibling = node->prev;
      if (NODE_INFO (sibling)->type == NODE_TYPE_TOOLBAR_PLACEHOLDER)
	prev_child = NODE_INFO (sibling)->extra; /* second Separator */
      else
	prev_child = NODE_INFO (sibling)->proxy;

      if (!CTK_IS_WIDGET (prev_child))
        return FALSE;

      toolbar = ctk_widget_get_parent (prev_child);
      if (!CTK_IS_TOOLBAR (toolbar))
        return FALSE;

      pos = ctk_toolbar_get_item_index (CTK_TOOLBAR (toolbar),
					CTK_TOOL_ITEM (prev_child)) + 1;
    }
  
  if (toolbar_p)
    *toolbar_p = toolbar;
  if (pos_p)
    *pos_p = pos;

  return TRUE;
}

enum {
  SEPARATOR_MODE_SMART,
  SEPARATOR_MODE_VISIBLE,
  SEPARATOR_MODE_HIDDEN
};

static void
update_smart_separators (CtkWidget *proxy)
{
  CtkWidget *parent = NULL;
  
  if (CTK_IS_MENU (proxy) || CTK_IS_TOOLBAR (proxy))
    parent = proxy;
  else if (CTK_IS_MENU_ITEM (proxy) || CTK_IS_TOOL_ITEM (proxy))
    parent = ctk_widget_get_parent (proxy);

  if (parent) 
    {
      gboolean visible;
      gboolean empty;
      GList *children, *cur, *last;
      CtkWidget *filler;

      children = ctk_container_get_children (CTK_CONTAINER (parent));
      
      visible = FALSE;
      last = NULL;
      empty = TRUE;
      filler = NULL;

      cur = children;
      while (cur) 
	{
	  if (g_object_get_data (cur->data, "ctk-empty-menu-item"))
	    {
	      filler = cur->data;
	    }
	  else if (CTK_IS_SEPARATOR_MENU_ITEM (cur->data) ||
		   CTK_IS_SEPARATOR_TOOL_ITEM (cur->data))
	    {
	      gint mode = 
		GPOINTER_TO_INT (g_object_get_data (G_OBJECT (cur->data), 
						    "ctk-separator-mode"));
	      switch (mode) 
		{
		case SEPARATOR_MODE_VISIBLE:
		  ctk_widget_show (CTK_WIDGET (cur->data));
		  last = NULL;
		  visible = FALSE;
		  break;
		case SEPARATOR_MODE_HIDDEN:
		  ctk_widget_hide (CTK_WIDGET (cur->data));
		  break;
		case SEPARATOR_MODE_SMART:
		  if (visible)
		    {
		      ctk_widget_show (CTK_WIDGET (cur->data));
		      last = cur;
		      visible = FALSE;
		    }
		  else 
		    ctk_widget_hide (CTK_WIDGET (cur->data));
		  break;
		}
	    }
	  else if (ctk_widget_get_visible (cur->data))
	    {
	      last = NULL;

	      if (CTK_IS_TEAROFF_MENU_ITEM (cur->data) || cur->data == filler)
		visible = FALSE;
	      else
		{
		  visible = TRUE;
		  empty = FALSE;
		}

	    }

	  cur = cur->next;
	}

      if (last)
	ctk_widget_hide (CTK_WIDGET (last->data));

      if (CTK_IS_MENU (parent)) 
	{
	  CtkWidget *item;

	  item = ctk_menu_get_attach_widget (CTK_MENU (parent));
	  if (CTK_IS_MENU_ITEM (item))
	    _ctk_action_sync_menu_visible (NULL, item, empty);
	  if (CTK_IS_WIDGET (filler))
	    {
	      if (empty)
		ctk_widget_show (filler);
	      else
		ctk_widget_hide (filler);
	    }
	}

      g_list_free (children);
    }
}

static void
update_node (CtkUIManager *manager, 
	     GNode        *node,
	     gboolean      in_popup,
             gboolean      popup_accels)
{
  Node *info;
  GNode *child;
  CtkAction *action;
  const gchar *action_name;
  NodeUIReference *ref;

  g_return_if_fail (node != NULL);
  g_return_if_fail (NODE_INFO (node) != NULL);

  info = NODE_INFO (node);
  
  if (!info->dirty)
    return;

  if (info->type == NODE_TYPE_POPUP)
    {
      in_popup = TRUE;
      popup_accels = info->popup_accels;
    }

  if (info->uifiles == NULL) {
    /* We may need to remove this node.
     * This must be done in post order
     */
    goto recurse_children;
  }
  
  ref = info->uifiles->data;
  action_name = g_quark_to_string (ref->action_quark);
  action = get_action_by_name (manager, action_name);
  
  info->dirty = FALSE;
  
  /* Check if the node doesn't have an action and must have an action */
  if (action == NULL &&
      info->type != NODE_TYPE_ROOT &&
      info->type != NODE_TYPE_MENUBAR &&
      info->type != NODE_TYPE_TOOLBAR &&
      info->type != NODE_TYPE_POPUP &&
      info->type != NODE_TYPE_SEPARATOR &&
      info->type != NODE_TYPE_MENU_PLACEHOLDER &&
      info->type != NODE_TYPE_TOOLBAR_PLACEHOLDER)
    {
      g_warning ("%s: missing action %s", info->name, action_name);
      
      return;
    }
  
  if (action)
    ctk_action_set_accel_group (action, manager->private_data->accel_group);
  
  /* If the widget already has a proxy and the action hasn't changed, then
   * we only have to update the tearoff menu items.
   */
  if (info->proxy != NULL && action == info->action)
    {
      if (info->type == NODE_TYPE_MENU)
	{
	  CtkWidget *menu;
	  GList *siblings;

	  if (CTK_IS_MENU (info->proxy))
	    menu = info->proxy;
	  else
	    menu = ctk_menu_item_get_submenu (CTK_MENU_ITEM (info->proxy));
	  siblings = ctk_container_get_children (CTK_CONTAINER (menu));

	  if (siblings != NULL && CTK_IS_TEAROFF_MENU_ITEM (siblings->data))
	    {
	      if (manager->private_data->add_tearoffs && !in_popup)
		ctk_widget_show (CTK_WIDGET (siblings->data));
	      else
		ctk_widget_hide (CTK_WIDGET (siblings->data));
	    }

	  g_list_free (siblings);
	}

      goto recurse_children;
    }
  
  switch (info->type)
    {
    case NODE_TYPE_MENUBAR:
      if (info->proxy == NULL)
	{
	  info->proxy = ctk_menu_bar_new ();
	  g_object_ref_sink (info->proxy);
	  ctk_widget_set_name (info->proxy, info->name);
	  ctk_widget_show (info->proxy);
	  g_signal_emit (manager, ui_manager_signals[ADD_WIDGET], 0, info->proxy);
	}
      break;
    case NODE_TYPE_POPUP:
      if (info->proxy == NULL) 
	{
	  info->proxy = ctk_menu_new ();
	  g_object_ref_sink (info->proxy);
	}
      ctk_widget_set_name (info->proxy, info->name);
      break;
    case NODE_TYPE_MENU:
      {
	CtkWidget *prev_submenu = NULL;
	CtkWidget *menu = NULL;
	GList *siblings;

	/* remove the proxy if it is of the wrong type ... */
	if (info->proxy &&  
	    G_OBJECT_TYPE (info->proxy) != CTK_ACTION_GET_CLASS (action)->menu_item_type)
	  {
	    if (CTK_IS_MENU_ITEM (info->proxy))
	      {
		prev_submenu = ctk_menu_item_get_submenu (CTK_MENU_ITEM (info->proxy));
		if (prev_submenu)
		  {
		    g_object_ref (prev_submenu);
		    ctk_menu_item_set_submenu (CTK_MENU_ITEM (info->proxy), NULL);
		}
	      }

            ctk_activatable_set_related_action (CTK_ACTIVATABLE (info->proxy), NULL);
	    ctk_container_remove (CTK_CONTAINER (ctk_widget_get_parent (info->proxy)),
				  info->proxy);
	    g_object_unref (info->proxy);
	    info->proxy = NULL;
	  }

	/* create proxy if needed ... */
	if (info->proxy == NULL)
	  {
            /* ... if the action already provides a menu, then use
             * that menu instead of creating an empty one
             */
            if ((NODE_INFO (node->parent)->type == NODE_TYPE_TOOLITEM ||
                 NODE_INFO (node->parent)->type == NODE_TYPE_MENUITEM) &&
                CTK_ACTION_GET_CLASS (action)->create_menu)
              {
                menu = ctk_action_create_menu (action);
              }

            if (!menu)
              {
                CtkWidget *tearoff;
                CtkWidget *filler;

                menu = ctk_menu_new ();
                ctk_widget_set_name (menu, info->name);

                tearoff = ctk_tearoff_menu_item_new ();

                ctk_widget_set_no_show_all (tearoff, TRUE);
                ctk_menu_shell_append (CTK_MENU_SHELL (menu), tearoff);
                filler = ctk_menu_item_new_with_label (_("Empty"));
                g_object_set_data (G_OBJECT (filler),
                                   I_("ctk-empty-menu-item"),
                                   GINT_TO_POINTER (TRUE));
                ctk_widget_set_sensitive (filler, FALSE);
                ctk_widget_set_no_show_all (filler, TRUE);
                ctk_menu_shell_append (CTK_MENU_SHELL (menu), filler);
              }

            if (NODE_INFO (node->parent)->type == NODE_TYPE_TOOLITEM)
	      {
		info->proxy = menu;
		g_object_ref_sink (info->proxy);
		ctk_menu_tool_button_set_menu (CTK_MENU_TOOL_BUTTON (NODE_INFO (node->parent)->proxy),
					       menu);
	      }
	    else
	      {
		CtkWidget *menushell;
		gint pos;
		
		if (find_menu_position (node, &menushell, &pos))
                  {
		     info->proxy = ctk_action_create_menu_item (action);
		     g_object_ref_sink (info->proxy);
		     g_signal_connect (info->proxy, "notify::visible",
		   		       G_CALLBACK (update_smart_separators), NULL);
		     ctk_widget_set_name (info->proxy, info->name);

		     ctk_menu_item_set_submenu (CTK_MENU_ITEM (info->proxy), menu);
		     ctk_menu_shell_insert (CTK_MENU_SHELL (menushell), info->proxy, pos);
                 }
	      }
	  }
	else
          ctk_activatable_set_related_action (CTK_ACTIVATABLE (info->proxy), action);

	if (prev_submenu)
	  {
	    ctk_menu_item_set_submenu (CTK_MENU_ITEM (info->proxy),
				       prev_submenu);
	    g_object_unref (prev_submenu);
	  }

	if (CTK_IS_MENU (info->proxy))
	  menu = info->proxy;
	else
	  menu = ctk_menu_item_get_submenu (CTK_MENU_ITEM (info->proxy));

	siblings = ctk_container_get_children (CTK_CONTAINER (menu));

	if (siblings != NULL && CTK_IS_TEAROFF_MENU_ITEM (siblings->data))
	  {
	    if (manager->private_data->add_tearoffs && !in_popup)
	      ctk_widget_show (CTK_WIDGET (siblings->data));
	    else
	      ctk_widget_hide (CTK_WIDGET (siblings->data));
	  }

	g_list_free (siblings);
      }
      break;
    case NODE_TYPE_UNDECIDED:
      g_warning ("found undecided node!");
      break;
    case NODE_TYPE_ROOT:
      break;
    case NODE_TYPE_TOOLBAR:
      if (info->proxy == NULL)
	{
	  info->proxy = ctk_toolbar_new ();
	  g_object_ref_sink (info->proxy);
	  ctk_widget_set_name (info->proxy, info->name);
	  ctk_widget_show (info->proxy);
	  g_signal_emit (manager, ui_manager_signals[ADD_WIDGET], 0, info->proxy);
	}
      break;
    case NODE_TYPE_MENU_PLACEHOLDER:
      /* create menu items for placeholders if necessary ... */
      if (!CTK_IS_SEPARATOR_MENU_ITEM (info->proxy) ||
	  !CTK_IS_SEPARATOR_MENU_ITEM (info->extra))
	{
	  if (info->proxy)
	    {
	      ctk_container_remove (CTK_CONTAINER (ctk_widget_get_parent (info->proxy)),
				    info->proxy);
	      g_object_unref (info->proxy);
	      info->proxy = NULL;
	    }
	  if (info->extra)
	    {
	      ctk_container_remove (CTK_CONTAINER (ctk_widget_get_parent (info->extra)),
				    info->extra);
	      g_object_unref (info->extra);
	      info->extra = NULL;
	    }
	}
      if (info->proxy == NULL)
	{
	  CtkWidget *menushell;
	  gint pos;
	  
	  if (find_menu_position (node, &menushell, &pos))
            {
	      info->proxy = ctk_separator_menu_item_new ();
	      g_object_ref_sink (info->proxy);
	      g_object_set_data (G_OBJECT (info->proxy),
	  		         I_("ctk-separator-mode"),
			         GINT_TO_POINTER (SEPARATOR_MODE_HIDDEN));
	      ctk_widget_set_no_show_all (info->proxy, TRUE);
	      ctk_menu_shell_insert (CTK_MENU_SHELL (menushell),
	 			     NODE_INFO (node)->proxy, pos);
	  
	      info->extra = ctk_separator_menu_item_new ();
	      g_object_ref_sink (info->extra);
	      g_object_set_data (G_OBJECT (info->extra),
			         I_("ctk-separator-mode"),
			         GINT_TO_POINTER (SEPARATOR_MODE_HIDDEN));
	      ctk_widget_set_no_show_all (info->extra, TRUE);
	      ctk_menu_shell_insert (CTK_MENU_SHELL (menushell),
				     NODE_INFO (node)->extra, pos + 1);
            }
	}
      break;
    case NODE_TYPE_TOOLBAR_PLACEHOLDER:
      /* create toolbar items for placeholders if necessary ... */
      if (!CTK_IS_SEPARATOR_TOOL_ITEM (info->proxy) ||
	  !CTK_IS_SEPARATOR_TOOL_ITEM (info->extra))
	{
	  if (info->proxy)
	    {
	      ctk_container_remove (CTK_CONTAINER (ctk_widget_get_parent (info->proxy)),
				    info->proxy);
	      g_object_unref (info->proxy);
	      info->proxy = NULL;
	    }
	  if (info->extra)
	    {
	      ctk_container_remove (CTK_CONTAINER (ctk_widget_get_parent (info->extra)),
				    info->extra);
	      g_object_unref (info->extra);
	      info->extra = NULL;
	    }
	}
      if (info->proxy == NULL)
	{
	  CtkWidget *toolbar;
	  gint pos;
	  CtkToolItem *item;    
	  
	  if (find_toolbar_position (node, &toolbar, &pos))
            {
	      item = ctk_separator_tool_item_new ();
	      ctk_toolbar_insert (CTK_TOOLBAR (toolbar), item, pos);
	      info->proxy = CTK_WIDGET (item);
	      g_object_ref_sink (info->proxy);
	      g_object_set_data (G_OBJECT (info->proxy),
			         I_("ctk-separator-mode"),
			         GINT_TO_POINTER (SEPARATOR_MODE_HIDDEN));
	      ctk_widget_set_no_show_all (info->proxy, TRUE);
	  
	      item = ctk_separator_tool_item_new ();
	      ctk_toolbar_insert (CTK_TOOLBAR (toolbar), item, pos+1);
	      info->extra = CTK_WIDGET (item);
	      g_object_ref_sink (info->extra);
	      g_object_set_data (G_OBJECT (info->extra),
			         I_("ctk-separator-mode"),
			         GINT_TO_POINTER (SEPARATOR_MODE_HIDDEN));
	      ctk_widget_set_no_show_all (info->extra, TRUE);
            }
	}
      break;
    case NODE_TYPE_MENUITEM:
      /* remove the proxy if it is of the wrong type ... */
      if (info->proxy &&  
	  G_OBJECT_TYPE (info->proxy) != CTK_ACTION_GET_CLASS (action)->menu_item_type)
	{
	  g_signal_handlers_disconnect_by_func (info->proxy,
						G_CALLBACK (update_smart_separators),
						NULL);  
          ctk_activatable_set_related_action (CTK_ACTIVATABLE (info->proxy), NULL);
	  ctk_container_remove (CTK_CONTAINER (ctk_widget_get_parent (info->proxy)),
				info->proxy);
	  g_object_unref (info->proxy);
	  info->proxy = NULL;
	}
      /* create proxy if needed ... */
      if (info->proxy == NULL)
	{
	  CtkWidget *menushell;
	  gint pos;
	  
	  if (find_menu_position (node, &menushell, &pos))
            {
	      info->proxy = ctk_action_create_menu_item (action);
	      g_object_ref_sink (info->proxy);
	      ctk_widget_set_name (info->proxy, info->name);

              if (info->always_show_image_set &&
                  CTK_IS_IMAGE_MENU_ITEM (info->proxy))
                ctk_image_menu_item_set_always_show_image (CTK_IMAGE_MENU_ITEM (info->proxy),
                                                           info->always_show_image);

	      ctk_menu_shell_insert (CTK_MENU_SHELL (menushell),
				     info->proxy, pos);
           }
	}
      else
	{
	  g_signal_handlers_disconnect_by_func (info->proxy,
						G_CALLBACK (update_smart_separators),
						NULL);
	  ctk_menu_item_set_submenu (CTK_MENU_ITEM (info->proxy), NULL);
          ctk_activatable_set_related_action (CTK_ACTIVATABLE (info->proxy), action);
	}

      if (info->proxy)
        {
          g_signal_connect (info->proxy, "notify::visible",
			    G_CALLBACK (update_smart_separators), NULL);
          if (in_popup && !popup_accels)
	    {
	      /* don't show accels in popups */
	      CtkWidget *c = ctk_bin_get_child (CTK_BIN (info->proxy));
	      if (CTK_IS_ACCEL_LABEL (c))
	        g_object_set (c, "accel-closure", NULL, NULL);
	    }
        }
      
      break;
    case NODE_TYPE_TOOLITEM:
      /* remove the proxy if it is of the wrong type ... */
      if (info->proxy && 
	  G_OBJECT_TYPE (info->proxy) != CTK_ACTION_GET_CLASS (action)->toolbar_item_type)
	{
	  g_signal_handlers_disconnect_by_func (info->proxy,
						G_CALLBACK (update_smart_separators),
						NULL);
          ctk_activatable_set_related_action (CTK_ACTIVATABLE (info->proxy), NULL);
	  ctk_container_remove (CTK_CONTAINER (ctk_widget_get_parent (info->proxy)),
				info->proxy);
	  g_object_unref (info->proxy);
	  info->proxy = NULL;
	}
      /* create proxy if needed ... */
      if (info->proxy == NULL)
	{
	  CtkWidget *toolbar;
	  gint pos;
	  
	  if (find_toolbar_position (node, &toolbar, &pos))
            {
	      info->proxy = ctk_action_create_tool_item (action);
	      g_object_ref_sink (info->proxy);
	      ctk_widget_set_name (info->proxy, info->name);
	      
	      ctk_toolbar_insert (CTK_TOOLBAR (toolbar),
	  		          CTK_TOOL_ITEM (info->proxy), pos);
            }
	}
      else
	{
	  g_signal_handlers_disconnect_by_func (info->proxy,
						G_CALLBACK (update_smart_separators),
						NULL);
	  ctk_activatable_set_related_action (CTK_ACTIVATABLE (info->proxy), action);
	}

      if (info->proxy)
        {
          g_signal_connect (info->proxy, "notify::visible",
			    G_CALLBACK (update_smart_separators), NULL);
        }
      break;
    case NODE_TYPE_SEPARATOR:
      if (NODE_INFO (node->parent)->type == NODE_TYPE_TOOLBAR ||
	  NODE_INFO (node->parent)->type == NODE_TYPE_TOOLBAR_PLACEHOLDER)
	{
	  CtkWidget *toolbar;
	  gint pos;
	  gint separator_mode;
	  CtkToolItem *item;

	  if (CTK_IS_SEPARATOR_TOOL_ITEM (info->proxy))
	    {
	      ctk_container_remove (CTK_CONTAINER (ctk_widget_get_parent (info->proxy)),
				    info->proxy);
	      g_object_unref (info->proxy);
	      info->proxy = NULL;
	    }
	  
	  if (find_toolbar_position (node, &toolbar, &pos))
            {
	      item  = ctk_separator_tool_item_new ();
	      ctk_toolbar_insert (CTK_TOOLBAR (toolbar), item, pos);
	      info->proxy = CTK_WIDGET (item);
	      g_object_ref_sink (info->proxy);
	      ctk_widget_set_no_show_all (info->proxy, TRUE);
	      if (info->expand)
	        {
	          ctk_tool_item_set_expand (CTK_TOOL_ITEM (item), TRUE);
	          ctk_separator_tool_item_set_draw (CTK_SEPARATOR_TOOL_ITEM (item), FALSE);
	          separator_mode = SEPARATOR_MODE_VISIBLE;
	        }
	      else
	        separator_mode = SEPARATOR_MODE_SMART;
	  
	      g_object_set_data (G_OBJECT (info->proxy),
			         I_("ctk-separator-mode"),
			         GINT_TO_POINTER (separator_mode));
	      ctk_widget_show (info->proxy);
            }
	}
      else
	{
	  CtkWidget *menushell;
	  gint pos;
	  
	  if (CTK_IS_SEPARATOR_MENU_ITEM (info->proxy))
	    {
	      ctk_container_remove (CTK_CONTAINER (ctk_widget_get_parent (info->proxy)),
				    info->proxy);
	      g_object_unref (info->proxy);
	      info->proxy = NULL;
	    }
	  
	  if (find_menu_position (node, &menushell, &pos))
	    {
              info->proxy = ctk_separator_menu_item_new ();
	      g_object_ref_sink (info->proxy);
	      ctk_widget_set_no_show_all (info->proxy, TRUE);
	      g_object_set_data (G_OBJECT (info->proxy),
			         I_("ctk-separator-mode"),
			         GINT_TO_POINTER (SEPARATOR_MODE_SMART));
	      ctk_menu_shell_insert (CTK_MENU_SHELL (menushell),
				     info->proxy, pos);
	      ctk_widget_show (info->proxy);
            }
	}
      break;
    case NODE_TYPE_ACCELERATOR:
      ctk_action_connect_accelerator (action);
      break;
    }
  
  if (action)
    g_object_ref (action);
  if (info->action)
    g_object_unref (info->action);
  info->action = action;

 recurse_children:
  /* process children */
  child = node->children;
  while (child)
    {
      GNode *current;
      
      current = child;
      child = current->next;
      update_node (manager, current, in_popup, popup_accels);
    }
  
  if (info->proxy) 
    {
      if (info->type == NODE_TYPE_MENU && CTK_IS_MENU_ITEM (info->proxy)) 
	update_smart_separators (ctk_menu_item_get_submenu (CTK_MENU_ITEM (info->proxy)));
      else if (info->type == NODE_TYPE_MENU || 
	       info->type == NODE_TYPE_TOOLBAR || 
	       info->type == NODE_TYPE_POPUP) 
	update_smart_separators (info->proxy);
    }
  
  /* handle cleanup of dead nodes */
  if (node->children == NULL && info->uifiles == NULL)
    {
      if (info->proxy)
	ctk_widget_destroy (info->proxy);
      if (info->extra)
	ctk_widget_destroy (info->extra);
      if (info->type == NODE_TYPE_ACCELERATOR && info->action != NULL)
	ctk_action_disconnect_accelerator (info->action);
      free_node (node);
      g_node_destroy (node);
    }
}

static gboolean
do_updates (CtkUIManager *manager)
{
  /* this function needs to check through the tree for dirty nodes.
   * For such nodes, it needs to do the following:
   *
   * 1) check if they are referenced by any loaded UI files anymore.
   *    In which case, the proxy widget should be destroyed, unless
   *    there are any subnodes.
   *
   * 2) lookup the action for this node again.  If it is different to
   *    the current one (or if no previous action has been looked up),
   *    the proxy is reconnected to the new action (or a new proxy widget
   *    is created and added to the parent container).
   */
  update_node (manager, manager->private_data->root_node, FALSE, FALSE);

  manager->private_data->update_tag = 0;

  return FALSE;
}

static gboolean
do_updates_idle (CtkUIManager *manager)
{
  do_updates (manager);

  return FALSE;
}

static void
queue_update (CtkUIManager *manager)
{
  if (manager->private_data->update_tag != 0)
    return;

  manager->private_data->update_tag = cdk_threads_add_idle (
					       (GSourceFunc)do_updates_idle, 
					       manager);
  g_source_set_name_by_id (manager->private_data->update_tag, "[ctk+] do_updates_idle");
}


/**
 * ctk_ui_manager_ensure_update:
 * @manager: a #CtkUIManager
 * 
 * Makes sure that all pending updates to the UI have been completed.
 *
 * This may occasionally be necessary, since #CtkUIManager updates the 
 * UI in an idle function. A typical example where this function is
 * useful is to enforce that the menubar and toolbar have been added to 
 * the main window before showing it:
 * |[<!-- language="C" -->
 * ctk_container_add (CTK_CONTAINER (window), vbox); 
 * g_signal_connect (merge, "add-widget", 
 *                   G_CALLBACK (add_widget), vbox);
 * ctk_ui_manager_add_ui_from_file (merge, "my-menus");
 * ctk_ui_manager_add_ui_from_file (merge, "my-toolbars");
 * ctk_ui_manager_ensure_update (merge);  
 * ctk_widget_show (window);
 * ]|
 *
 * Since: 2.4
 **/
void
ctk_ui_manager_ensure_update (CtkUIManager *manager)
{
  if (manager->private_data->update_tag != 0)
    {
      g_source_remove (manager->private_data->update_tag);
      do_updates (manager);
    }
}

static gboolean
dirty_traverse_func (GNode   *node,
		     gpointer data G_GNUC_UNUSED)
{
  NODE_INFO (node)->dirty = TRUE;
  return FALSE;
}

static void
dirty_all_nodes (CtkUIManager *manager)
{
  g_node_traverse (manager->private_data->root_node,
		   G_PRE_ORDER, G_TRAVERSE_ALL, -1,
		   dirty_traverse_func, NULL);
  queue_update (manager);
}

static void
mark_node_dirty (GNode *node)
{
  GNode *p;

  /* FIXME could optimize this */
  for (p = node; p; p = p->parent)
    NODE_INFO (p)->dirty = TRUE;  
}

static void
print_node (CtkUIManager *manager,
	    GNode        *node,
	    gint          indent_level,
	    GString      *buffer)
{
  Node  *mnode;
  GNode *child;

  mnode = node->data;

  switch (mnode->type)
    {
    case NODE_TYPE_UNDECIDED:
      g_string_append_printf (buffer, "%*s<UNDECIDED", indent_level, "");
      break;
    case NODE_TYPE_ROOT:
      g_string_append_printf (buffer, "%*s<ui", indent_level, "");
      break;
    case NODE_TYPE_MENUBAR:
      g_string_append_printf (buffer, "%*s<menubar", indent_level, "");
      break;
    case NODE_TYPE_MENU:
      g_string_append_printf (buffer, "%*s<menu", indent_level, "");
      break;
    case NODE_TYPE_TOOLBAR:
      g_string_append_printf (buffer, "%*s<toolbar", indent_level, "");
      break;
    case NODE_TYPE_MENU_PLACEHOLDER:
    case NODE_TYPE_TOOLBAR_PLACEHOLDER:
      g_string_append_printf (buffer, "%*s<placeholder", indent_level, "");
      break;
    case NODE_TYPE_POPUP:
      g_string_append_printf (buffer, "%*s<popup", indent_level, "");
      break;
    case NODE_TYPE_MENUITEM:
      g_string_append_printf (buffer, "%*s<menuitem", indent_level, "");
      break;
    case NODE_TYPE_TOOLITEM:
      g_string_append_printf (buffer, "%*s<toolitem", indent_level, "");
      break;
    case NODE_TYPE_SEPARATOR:
      g_string_append_printf (buffer, "%*s<separator", indent_level, "");
      break;
    case NODE_TYPE_ACCELERATOR:
      g_string_append_printf (buffer, "%*s<accelerator", indent_level, "");
      break;
    default:
      ;; /* Nothing */
    }

  if (mnode->type != NODE_TYPE_ROOT)
    {
      if (mnode->name)
	g_string_append_printf (buffer, " name=\"%s\"", mnode->name);
      
      if (mnode->action_name)
	g_string_append_printf (buffer, " action=\"%s\"",
				g_quark_to_string (mnode->action_name));
    }

  switch (mnode->type)
    {
    case NODE_TYPE_UNDECIDED:
    case NODE_TYPE_ROOT:
    case NODE_TYPE_MENUBAR:
    case NODE_TYPE_MENU:
    case NODE_TYPE_TOOLBAR:
    case NODE_TYPE_MENU_PLACEHOLDER:
    case NODE_TYPE_TOOLBAR_PLACEHOLDER:
    case NODE_TYPE_POPUP:
      g_string_append (buffer, ">\n");
      break;
    default:
      g_string_append (buffer, "/>\n");
      break;
    }

  for (child = node->children; child != NULL; child = child->next)
    print_node (manager, child, indent_level + 2, buffer);

  switch (mnode->type)
    {
    case NODE_TYPE_UNDECIDED:
      g_string_append_printf (buffer, "%*s</UNDECIDED>\n", indent_level, "");
      break;
    case NODE_TYPE_ROOT:
      g_string_append_printf (buffer, "%*s</ui>\n", indent_level, "");
      break;
    case NODE_TYPE_MENUBAR:
      g_string_append_printf (buffer, "%*s</menubar>\n", indent_level, "");
      break;
    case NODE_TYPE_MENU:
      g_string_append_printf (buffer, "%*s</menu>\n", indent_level, "");
      break;
    case NODE_TYPE_TOOLBAR:
      g_string_append_printf (buffer, "%*s</toolbar>\n", indent_level, "");
      break;
    case NODE_TYPE_MENU_PLACEHOLDER:
    case NODE_TYPE_TOOLBAR_PLACEHOLDER:
      g_string_append_printf (buffer, "%*s</placeholder>\n", indent_level, "");
      break;
    case NODE_TYPE_POPUP:
      g_string_append_printf (buffer, "%*s</popup>\n", indent_level, "");
      break;
    default:
      ;; /* Nothing */
    }
}

static gboolean
ctk_ui_manager_buildable_custom_tag_start (CtkBuildable  *buildable,
					   CtkBuilder    *builder G_GNUC_UNUSED,
					   GObject       *child,
					   const gchar   *tagname,
					   GMarkupParser *parser,
					   gpointer      *data)
{
  if (child)
    return FALSE;

  if (strcmp (tagname, "ui") == 0)
    {
      ParseContext *ctx;

      ctx = g_new0 (ParseContext, 1);
      ctx->state = STATE_START;
      ctx->manager = CTK_UI_MANAGER (buildable);
      ctx->current = NULL;
      ctx->merge_id = ctk_ui_manager_new_merge_id (CTK_UI_MANAGER (buildable));

      *data = ctx;
      *parser = ui_parser;

      return TRUE;
    }

  return FALSE;

}

static void
ctk_ui_manager_buildable_custom_tag_end (CtkBuildable *buildable,
					 CtkBuilder   *builder G_GNUC_UNUSED,
					 GObject      *child G_GNUC_UNUSED,
					 const gchar  *tagname G_GNUC_UNUSED,
					 gpointer     *data)
{
  queue_update (CTK_UI_MANAGER (buildable));
  g_object_notify (G_OBJECT (buildable), "ui");
  g_free (data);
}

/**
 * ctk_ui_manager_get_ui:
 * @manager: a #CtkUIManager
 * 
 * Creates a [UI definition][XML-UI] of the merged UI.
 * 
 * Returns: A newly allocated string containing an XML representation of 
 * the merged UI.
 *
 * Since: 2.4
 **/
gchar *
ctk_ui_manager_get_ui (CtkUIManager *manager)
{
  GString *buffer;

  buffer = g_string_new (NULL);

  ctk_ui_manager_ensure_update (manager); 
 
  print_node (manager, manager->private_data->root_node, 0, buffer);  

  return g_string_free (buffer, FALSE);
}
