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

/**
 * SECTION:ctkactiongroup
 * @Short_description: A group of actions
 * @Title: CtkActionGroup
 *
 * Actions are organised into groups. An action group is essentially a
 * map from names to #CtkAction objects.
 *
 * All actions that would make sense to use in a particular context
 * should be in a single group. Multiple action groups may be used for a
 * particular user interface. In fact, it is expected that most nontrivial
 * applications will make use of multiple groups. For example, in an
 * application that can edit multiple documents, one group holding global
 * actions (e.g. quit, about, new), and one group per document holding
 * actions that act on that document (eg. save, cut/copy/paste, etc). Each
 * window’s menus would be constructed from a combination of two action
 * groups.
 *
 * ## Accelerators ## {#Action-Accel}
 *
 * Accelerators are handled by the CTK+ accelerator map. All actions are
 * assigned an accelerator path (which normally has the form
 * `<Actions>/group-name/action-name`) and a shortcut is associated with
 * this accelerator path. All menuitems and toolitems take on this accelerator
 * path. The CTK+ accelerator map code makes sure that the correct shortcut
 * is displayed next to the menu item.
 *
 * # CtkActionGroup as CtkBuildable # {#CtkActionGroup-BUILDER-UI}
 *
 * The #CtkActionGroup implementation of the #CtkBuildable interface accepts
 * #CtkAction objects as <child> elements in UI definitions.
 *
 * Note that it is probably more common to define actions and action groups
 * in the code, since they are directly related to what the code can do.
 *
 * The CtkActionGroup implementation of the CtkBuildable interface supports
 * a custom <accelerator> element, which has attributes named “key“ and
 * “modifiers“ and allows to specify accelerators. This is similar to the
 * <accelerator> element of #CtkWidget, the main difference is that
 * it doesn’t allow you to specify a signal.
 *
 * ## A #CtkDialog UI definition fragment. ##
 * |[
 * <object class="CtkActionGroup" id="actiongroup">
 *   <child>
 *       <object class="CtkAction" id="About">
 *           <property name="name">About</property>
 *           <property name="stock_id">ctk-about</property>
 *           <signal handler="about_activate" name="activate"/>
 *       </object>
 *       <accelerator key="F1" modifiers="CDK_CONTROL_MASK | CDK_SHIFT_MASK"/>
 *   </child>
 * </object>
 * ]|
 *
 */

#include "config.h"
#include <string.h>

#include "ctkactiongroup.h"
#include "ctkbuildable.h"
#include "ctkiconfactory.h"
#include "ctkicontheme.h"
#include "ctkstock.h"
#include "ctktoggleaction.h"
#include "ctkradioaction.h"
#include "ctkaccelmap.h"
#include "ctkmarshalers.h"
#include "ctkbuilderprivate.h"
#include "ctkprivate.h"
#include "ctkintl.h"


struct _CtkActionGroupPrivate 
{
  gchar           *name;
  gboolean	   sensitive;
  gboolean	   visible;
  GHashTable      *actions;
  CtkAccelGroup   *accel_group;

  CtkTranslateFunc translate_func;
  gpointer         translate_data;
  GDestroyNotify   translate_notify;
};

enum 
{
  CONNECT_PROXY,
  DISCONNECT_PROXY,
  PRE_ACTIVATE,
  POST_ACTIVATE,
  LAST_SIGNAL
};

enum 
{
  PROP_0,
  PROP_NAME,
  PROP_SENSITIVE,
  PROP_VISIBLE,
  PROP_ACCEL_GROUP
};

static void       ctk_action_group_init            (CtkActionGroup      *self);
static void       ctk_action_group_class_init      (CtkActionGroupClass *class);
static void       ctk_action_group_finalize        (GObject             *object);
static void       ctk_action_group_set_property    (GObject             *object,
						    guint                prop_id,
						    const GValue        *value,
						    GParamSpec          *pspec);
static void       ctk_action_group_get_property    (GObject             *object,
						    guint                prop_id,
						    GValue              *value,
						    GParamSpec          *pspec);
static CtkAction *ctk_action_group_real_get_action (CtkActionGroup      *self,
						    const gchar         *name);

/* CtkBuildable */
static void ctk_action_group_buildable_init (CtkBuildableIface *iface);
static void ctk_action_group_buildable_add_child (CtkBuildable  *buildable,
						  CtkBuilder    *builder,
						  GObject       *child,
						  const gchar   *type);
static void ctk_action_group_buildable_set_name (CtkBuildable *buildable,
						 const gchar  *name);
static const gchar* ctk_action_group_buildable_get_name (CtkBuildable *buildable);
static gboolean ctk_action_group_buildable_custom_tag_start (CtkBuildable     *buildable,
							     CtkBuilder       *builder,
							     GObject          *child,
							     const gchar      *tagname,
							     GMarkupParser    *parser,
							     gpointer         *data);
static void ctk_action_group_buildable_custom_tag_end (CtkBuildable *buildable,
						       CtkBuilder   *builder,
						       GObject      *child,
						       const gchar  *tagname,
						       gpointer     *user_data);

static guint         action_group_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_CODE (CtkActionGroup, ctk_action_group, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (CtkActionGroup)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_BUILDABLE,
                                                ctk_action_group_buildable_init))

static void
ctk_action_group_class_init (CtkActionGroupClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = ctk_action_group_finalize;
  gobject_class->set_property = ctk_action_group_set_property;
  gobject_class->get_property = ctk_action_group_get_property;
  klass->get_action = ctk_action_group_real_get_action;

  /**
   * CtkActionGroup:name:
   *
   * A name for the action.
   */
  g_object_class_install_property (gobject_class,
				   PROP_NAME,
				   g_param_spec_string ("name",
							P_("Name"),
							P_("A name for the action group."),
							NULL,
							CTK_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  /**
   * CtkActionGroup:sensitive:
   *
   * Whether the action group is enabled.
   */
  g_object_class_install_property (gobject_class,
				   PROP_SENSITIVE,
				   g_param_spec_boolean ("sensitive",
							 P_("Sensitive"),
							 P_("Whether the action group is enabled."),
							 TRUE,
							 CTK_PARAM_READWRITE));
  /**
   * CtkActionGroup:visible:
   *
   * Whether the action group is visible.
   */
  g_object_class_install_property (gobject_class,
				   PROP_VISIBLE,
				   g_param_spec_boolean ("visible",
							 P_("Visible"),
							 P_("Whether the action group is visible."),
							 TRUE,
							 CTK_PARAM_READWRITE));
  /**
   * CtkActionGroup:accel-group:
   *
   * The accelerator group the actions of this group should use.
   */
  g_object_class_install_property (gobject_class,
				   PROP_ACCEL_GROUP,
				   g_param_spec_object ("accel-group",
							P_("Accelerator Group"),
							P_("The accelerator group the actions of this group should use."),
							CTK_TYPE_ACCEL_GROUP,
							CTK_PARAM_READWRITE));

  /**
   * CtkActionGroup::connect-proxy:
   * @action_group: the group
   * @action: the action
   * @proxy: the proxy
   *
   * The ::connect-proxy signal is emitted after connecting a proxy to 
   * an action in the group. Note that the proxy may have been connected 
   * to a different action before.
   *
   * This is intended for simple customizations for which a custom action
   * class would be too clumsy, e.g. showing tooltips for menuitems in the
   * statusbar.
   *
   * #CtkUIManager proxies the signal and provides global notification 
   * just before any action is connected to a proxy, which is probably more
   * convenient to use.
   *
   * Since: 2.4
   */
  action_group_signals[CONNECT_PROXY] =
    g_signal_new (I_("connect-proxy"),
		  G_OBJECT_CLASS_TYPE (klass),
		  0, 0, NULL, NULL,
		  _ctk_marshal_VOID__OBJECT_OBJECT,
		  G_TYPE_NONE, 2,
		  CTK_TYPE_ACTION, CTK_TYPE_WIDGET);

  /**
   * CtkActionGroup::disconnect-proxy:
   * @action_group: the group
   * @action: the action
   * @proxy: the proxy
   *
   * The ::disconnect-proxy signal is emitted after disconnecting a proxy 
   * from an action in the group. 
   *
   * #CtkUIManager proxies the signal and provides global notification 
   * just before any action is connected to a proxy, which is probably more
   * convenient to use.
   *
   * Since: 2.4
   */
  action_group_signals[DISCONNECT_PROXY] =
    g_signal_new (I_("disconnect-proxy"),
		  G_OBJECT_CLASS_TYPE (klass),
		  0, 0, NULL, NULL,
		  _ctk_marshal_VOID__OBJECT_OBJECT,
		  G_TYPE_NONE, 2, 
		  CTK_TYPE_ACTION, CTK_TYPE_WIDGET);

  /**
   * CtkActionGroup::pre-activate:
   * @action_group: the group
   * @action: the action
   *
   * The ::pre-activate signal is emitted just before the @action in the
   * @action_group is activated
   *
   * This is intended for #CtkUIManager to proxy the signal and provide global
   * notification just before any action is activated.
   *
   * Since: 2.4
   */
  action_group_signals[PRE_ACTIVATE] =
    g_signal_new (I_("pre-activate"),
		  G_OBJECT_CLASS_TYPE (klass),
		  0, 0, NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1, 
		  CTK_TYPE_ACTION);

  /**
   * CtkActionGroup::post-activate:
   * @action_group: the group
   * @action: the action
   *
   * The ::post-activate signal is emitted just after the @action in the
   * @action_group is activated
   *
   * This is intended for #CtkUIManager to proxy the signal and provide global
   * notification just after any action is activated.
   *
   * Since: 2.4
   */
  action_group_signals[POST_ACTIVATE] =
    g_signal_new (I_("post-activate"),
		  G_OBJECT_CLASS_TYPE (klass),
		  0, 0, NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1, 
		  CTK_TYPE_ACTION);
}


static void 
remove_action (CtkAction *action) 
{
  g_object_set (action, I_("action-group"), NULL, NULL);
  g_object_unref (action);
}

static void
ctk_action_group_init (CtkActionGroup *action_group)
{
  action_group->priv = ctk_action_group_get_instance_private (action_group);
  action_group->priv->name = NULL;
  action_group->priv->sensitive = TRUE;
  action_group->priv->visible = TRUE;
  action_group->priv->actions = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                       NULL,
                                                       (GDestroyNotify) remove_action);
  action_group->priv->translate_func = NULL;
  action_group->priv->translate_data = NULL;
  action_group->priv->translate_notify = NULL;
}

static void
ctk_action_group_buildable_init (CtkBuildableIface *iface)
{
  iface->add_child = ctk_action_group_buildable_add_child;
  iface->set_name = ctk_action_group_buildable_set_name;
  iface->get_name = ctk_action_group_buildable_get_name;
  iface->custom_tag_start = ctk_action_group_buildable_custom_tag_start;
  iface->custom_tag_end = ctk_action_group_buildable_custom_tag_end;
}

static void
ctk_action_group_buildable_add_child (CtkBuildable  *buildable,
				      CtkBuilder    *builder G_GNUC_UNUSED,
				      GObject       *child,
				      const gchar   *type G_GNUC_UNUSED)
{
  ctk_action_group_add_action_with_accel (CTK_ACTION_GROUP (buildable),
					  CTK_ACTION (child), NULL);
}

static void
ctk_action_group_buildable_set_name (CtkBuildable *buildable,
				     const gchar  *name)
{
  CtkActionGroup *self = CTK_ACTION_GROUP (buildable);
  CtkActionGroupPrivate *private = self->priv;

  private->name = g_strdup (name);
}

static const gchar *
ctk_action_group_buildable_get_name (CtkBuildable *buildable)
{
  CtkActionGroup *self = CTK_ACTION_GROUP (buildable);
  CtkActionGroupPrivate *private = self->priv;

  return private->name;
}

typedef struct {
  GObject         *child;
  guint            key;
  CdkModifierType  modifiers;
} AcceleratorParserData;

static void
accelerator_start_element (GMarkupParseContext *context G_GNUC_UNUSED,
			   const gchar         *element_name,
			   const gchar        **names,
			   const gchar        **values,
			   gpointer             user_data,
			   GError             **error)
{
  gint i;
  guint key = 0;
  CdkModifierType modifiers = 0;
  AcceleratorParserData *parser_data = (AcceleratorParserData*)user_data;

  if (strcmp (element_name, "accelerator") != 0)
    g_warning ("Unknown <accelerator> tag: %s", element_name);

  for (i = 0; names[i]; i++)
    {
      if (strcmp (names[i], "key") == 0)
	key = cdk_keyval_from_name (values[i]);
      else if (strcmp (names[i], "modifiers") == 0)
	{
	  if (!_ctk_builder_flags_from_string (CDK_TYPE_MODIFIER_TYPE,
                                               NULL,
					       values[i],
					       &modifiers,
					       error))
	      return;
	}
    }

  if (key == 0)
    {
      g_warning ("<accelerator> requires a key attribute");
      return;
    }
  parser_data->key = key;
  parser_data->modifiers = modifiers;
}

static const GMarkupParser accelerator_parser =
  {
    .start_element = accelerator_start_element
  };

static gboolean
ctk_action_group_buildable_custom_tag_start (CtkBuildable  *buildable G_GNUC_UNUSED,
					     CtkBuilder    *builder G_GNUC_UNUSED,
					     GObject       *child,
					     const gchar   *tagname,
					     GMarkupParser *parser,
					     gpointer      *user_data)
{
  AcceleratorParserData *parser_data;

  if (child && strcmp (tagname, "accelerator") == 0)
    {
      parser_data = g_slice_new0 (AcceleratorParserData);
      parser_data->child = child;
      *user_data = parser_data;
      *parser = accelerator_parser;

      return TRUE;
    }
  return FALSE;
}

static void
ctk_action_group_buildable_custom_tag_end (CtkBuildable *buildable,
					   CtkBuilder   *builder G_GNUC_UNUSED,
					   GObject      *child,
					   const gchar  *tagname,
					   gpointer     *user_data)
{
  AcceleratorParserData *data;
  
  if (strcmp (tagname, "accelerator") == 0)
    {
      CtkActionGroup *action_group;
      CtkActionGroupPrivate *private;
      CtkAction *action;
      gchar *accel_path;
      
      data = (AcceleratorParserData*)user_data;
      action_group = CTK_ACTION_GROUP (buildable);
      private = action_group->priv;
      action = CTK_ACTION (child);
	
      accel_path = g_strconcat ("<Actions>/",
				private->name, "/",
				ctk_action_get_name (action), NULL);

      if (ctk_accel_map_lookup_entry (accel_path, NULL))
	ctk_accel_map_change_entry (accel_path, data->key, data->modifiers, TRUE);
      else
	ctk_accel_map_add_entry (accel_path, data->key, data->modifiers);

      ctk_action_set_accel_path (action, accel_path);
      
      g_free (accel_path);
      g_slice_free (AcceleratorParserData, data);
    }
}

/**
 * ctk_action_group_new:
 * @name: the name of the action group.
 *
 * Creates a new #CtkActionGroup object. The name of the action group
 * is used when associating [keybindings][Action-Accel] 
 * with the actions.
 *
 * Returns: the new #CtkActionGroup
 *
 * Since: 2.4
 */
CtkActionGroup *
ctk_action_group_new (const gchar *name)
{
  CtkActionGroup *self;
  CtkActionGroupPrivate *private;

  self = g_object_new (CTK_TYPE_ACTION_GROUP, NULL);
  private = self->priv;
  private->name = g_strdup (name);

  return self;
}

static void
ctk_action_group_finalize (GObject *object)
{
  CtkActionGroup *self = CTK_ACTION_GROUP (object);

  g_free (self->priv->name);

  g_hash_table_destroy (self->priv->actions);

  g_clear_object (&self->priv->accel_group);

  if (self->priv->translate_notify != NULL)
    self->priv->translate_notify (self->priv->translate_data);

  G_OBJECT_CLASS (ctk_action_group_parent_class)->finalize (object);
}

static void
ctk_action_group_set_property (GObject         *object,
			       guint            prop_id,
			       const GValue    *value,
			       GParamSpec      *pspec)
{
  CtkActionGroup *self;
  CtkActionGroupPrivate *private;
  gchar *tmp;
  
  self = CTK_ACTION_GROUP (object);
  private = self->priv;

  switch (prop_id)
    {
    case PROP_NAME:
      tmp = private->name;
      private->name = g_value_dup_string (value);
      g_free (tmp);
      break;
    case PROP_SENSITIVE:
      ctk_action_group_set_sensitive (self, g_value_get_boolean (value));
      break;
    case PROP_VISIBLE:
      ctk_action_group_set_visible (self, g_value_get_boolean (value));
      break;
    case PROP_ACCEL_GROUP:
      ctk_action_group_set_accel_group (self, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_action_group_get_property (GObject    *object,
			       guint       prop_id,
			       GValue     *value,
			       GParamSpec *pspec)
{
  CtkActionGroup *self;
  CtkActionGroupPrivate *private;
  
  self = CTK_ACTION_GROUP (object);
  private = self->priv;

  switch (prop_id)
    {
    case PROP_NAME:
      g_value_set_string (value, private->name);
      break;
    case PROP_SENSITIVE:
      g_value_set_boolean (value, private->sensitive);
      break;
    case PROP_VISIBLE:
      g_value_set_boolean (value, private->visible);
      break;
    case PROP_ACCEL_GROUP:
      g_value_set_object (value, private->accel_group);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static CtkAction *
ctk_action_group_real_get_action (CtkActionGroup *self,
				  const gchar    *action_name)
{
  CtkActionGroupPrivate *private;

  private = self->priv;

  return g_hash_table_lookup (private->actions, action_name);
}

/**
 * ctk_action_group_get_name:
 * @action_group: the action group
 *
 * Gets the name of the action group.
 *
 * Returns: the name of the action group.
 * 
 * Since: 2.4
 */
const gchar *
ctk_action_group_get_name (CtkActionGroup *action_group)
{
  CtkActionGroupPrivate *private;

  g_return_val_if_fail (CTK_IS_ACTION_GROUP (action_group), NULL);

  private = action_group->priv;

  return private->name;
}

/**
 * ctk_action_group_get_sensitive:
 * @action_group: the action group
 *
 * Returns %TRUE if the group is sensitive.  The constituent actions
 * can only be logically sensitive (see ctk_action_is_sensitive()) if
 * they are sensitive (see ctk_action_get_sensitive()) and their group
 * is sensitive.
 * 
 * Returns: %TRUE if the group is sensitive.
 *
 * Since: 2.4
 */
gboolean
ctk_action_group_get_sensitive (CtkActionGroup *action_group)
{
  CtkActionGroupPrivate *private;

  g_return_val_if_fail (CTK_IS_ACTION_GROUP (action_group), FALSE);

  private = action_group->priv;

  return private->sensitive;
}

static void
cb_set_action_sensitivity (const gchar *name G_GNUC_UNUSED,
			   CtkAction   *action)
{
  /* Minor optimization, the action_groups state only affects actions 
   * that are themselves sensitive */
  g_object_notify (G_OBJECT (action), "sensitive");

}

/**
 * ctk_action_group_set_sensitive:
 * @action_group: the action group
 * @sensitive: new sensitivity
 *
 * Changes the sensitivity of @action_group
 * 
 * Since: 2.4
 */
void
ctk_action_group_set_sensitive (CtkActionGroup *action_group, 
				gboolean        sensitive)
{
  CtkActionGroupPrivate *private;

  g_return_if_fail (CTK_IS_ACTION_GROUP (action_group));

  private = action_group->priv;
  sensitive = sensitive != FALSE;

  if (private->sensitive != sensitive)
    {
      private->sensitive = sensitive;
      g_hash_table_foreach (private->actions, 
			    (GHFunc) cb_set_action_sensitivity, NULL);

      g_object_notify (G_OBJECT (action_group), "sensitive");
    }
}

/**
 * ctk_action_group_get_visible:
 * @action_group: the action group
 *
 * Returns %TRUE if the group is visible.  The constituent actions
 * can only be logically visible (see ctk_action_is_visible()) if
 * they are visible (see ctk_action_get_visible()) and their group
 * is visible.
 * 
 * Returns: %TRUE if the group is visible.
 * 
 * Since: 2.4
 */
gboolean
ctk_action_group_get_visible (CtkActionGroup *action_group)
{
  CtkActionGroupPrivate *private;

  g_return_val_if_fail (CTK_IS_ACTION_GROUP (action_group), FALSE);

  private = action_group->priv;

  return private->visible;
}

/**
 * ctk_action_group_get_accel_group:
 * @action_group: a #CtkActionGroup
 *
 * Gets the accelerator group.
 * 
 * Returns: (transfer none): the accelerator group associated with this action
 * group or %NULL if there is none.
 *
 * Since: 3.6
 */
CtkAccelGroup *
ctk_action_group_get_accel_group (CtkActionGroup *action_group)
{
  g_return_val_if_fail (CTK_IS_ACTION_GROUP (action_group), FALSE);

  return action_group->priv->accel_group;
}

static void
cb_set_action_visiblity (const gchar *name G_GNUC_UNUSED,
			 CtkAction   *action)
{
  /* Minor optimization, the action_groups state only affects actions 
   * that are themselves visible */
  g_object_notify (G_OBJECT (action), "visible");
}

/**
 * ctk_action_group_set_visible:
 * @action_group: the action group
 * @visible: new visiblity
 *
 * Changes the visible of @action_group.
 * 
 * Since: 2.4
 */
void
ctk_action_group_set_visible (CtkActionGroup *action_group, 
			      gboolean        visible)
{
  CtkActionGroupPrivate *private;

  g_return_if_fail (CTK_IS_ACTION_GROUP (action_group));

  private = action_group->priv;
  visible = visible != FALSE;

  if (private->visible != visible)
    {
      private->visible = visible;
      g_hash_table_foreach (private->actions, 
			    (GHFunc) cb_set_action_visiblity, NULL);

      g_object_notify (G_OBJECT (action_group), "visible");
    }
}

static void 
ctk_action_group_accel_group_foreach (gpointer key G_GNUC_UNUSED,
				      gpointer val,
				      gpointer data)
{
  ctk_action_set_accel_group (val, data);
}

/**
 * ctk_action_group_set_accel_group:
 * @action_group: a #CtkActionGroup
 * @accel_group: (allow-none): a #CtkAccelGroup to set or %NULL
 *
 * Sets the accelerator group to be used by every action in this group.
 * 
 * Since: 3.6
 */
void
ctk_action_group_set_accel_group (CtkActionGroup *action_group,
                                  CtkAccelGroup  *accel_group)
{
  CtkActionGroupPrivate *private;

  g_return_if_fail (CTK_IS_ACTION_GROUP (action_group));

  private = action_group->priv;

  if (private->accel_group == accel_group)
    return;

  g_clear_object (&private->accel_group);

  if (accel_group)
    private->accel_group = g_object_ref (accel_group);

  /* Set the new accel group on every action */
  g_hash_table_foreach (private->actions,
                        ctk_action_group_accel_group_foreach,
                        accel_group);

  g_object_notify (G_OBJECT (action_group), "accel-group");
}

/**
 * ctk_action_group_get_action:
 * @action_group: the action group
 * @action_name: the name of the action
 *
 * Looks up an action in the action group by name.
 *
 * Returns: (transfer none): the action, or %NULL if no action by that name exists
 *
 * Since: 2.4
 */
CtkAction *
ctk_action_group_get_action (CtkActionGroup *action_group,
			     const gchar    *action_name)
{
  g_return_val_if_fail (CTK_IS_ACTION_GROUP (action_group), NULL);
  g_return_val_if_fail (CTK_ACTION_GROUP_GET_CLASS (action_group)->get_action != NULL, NULL);

  return CTK_ACTION_GROUP_GET_CLASS (action_group)->get_action (action_group,
                                                                action_name);
}

static gboolean
check_unique_action (CtkActionGroup *action_group,
	             const gchar    *action_name)
{
  if (ctk_action_group_get_action (action_group, action_name) != NULL)
    {
      CtkActionGroupPrivate *private;

      private = action_group->priv;

      g_warning ("Refusing to add non-unique action '%s' to action group '%s'",
	 	 action_name,
		 private->name);
      return FALSE;
    }

  return TRUE;
}

/**
 * ctk_action_group_add_action:
 * @action_group: the action group
 * @action: an action
 *
 * Adds an action object to the action group. Note that this function
 * does not set up the accel path of the action, which can lead to problems
 * if a user tries to modify the accelerator of a menuitem associated with
 * the action. Therefore you must either set the accel path yourself with
 * ctk_action_set_accel_path(), or use 
 * `ctk_action_group_add_action_with_accel (..., NULL)`.
 *
 * Since: 2.4
 */
void
ctk_action_group_add_action (CtkActionGroup *action_group,
			     CtkAction      *action)
{
  CtkActionGroupPrivate *private;
  const gchar *name;

  g_return_if_fail (CTK_IS_ACTION_GROUP (action_group));
  g_return_if_fail (CTK_IS_ACTION (action));

  name = ctk_action_get_name (action);
  g_return_if_fail (name != NULL);
  
  if (!check_unique_action (action_group, name))
    return;

  private = action_group->priv;

  g_hash_table_insert (private->actions, 
		       (gpointer) name,
                       g_object_ref (action));
  g_object_set (action, I_("action-group"), action_group, NULL);
  
  if (private->accel_group)
    ctk_action_set_accel_group (action, private->accel_group);
}

/**
 * ctk_action_group_add_action_with_accel:
 * @action_group: the action group
 * @action: the action to add
 * @accelerator: (allow-none): the accelerator for the action, in
 *   the format understood by ctk_accelerator_parse(), or "" for no accelerator, or
 *   %NULL to use the stock accelerator
 *
 * Adds an action object to the action group and sets up the accelerator.
 *
 * If @accelerator is %NULL, attempts to use the accelerator associated 
 * with the stock_id of the action. 
 *
 * Accel paths are set to `<Actions>/group-name/action-name`.
 *
 * Since: 2.4
 */
void
ctk_action_group_add_action_with_accel (CtkActionGroup *action_group,
					CtkAction      *action,
					const gchar    *accelerator)
{
  CtkActionGroupPrivate *private;
  gchar *accel_path;
  guint  accel_key = 0;
  CdkModifierType accel_mods;
  const gchar *name;

  name = ctk_action_get_name (action);
  if (!check_unique_action (action_group, name))
    return;

  private = action_group->priv;
  accel_path = g_strconcat ("<Actions>/",
			    private->name, "/", name, NULL);

  if (accelerator)
    {
      if (accelerator[0] == 0) 
	accel_key = 0;
      else
	{
	  ctk_accelerator_parse (accelerator, &accel_key, &accel_mods);
	  if (accel_key == 0)
	    g_warning ("Unable to parse accelerator '%s' for action '%s'",
		       accelerator, name);
	}
    }
  else 
    {
      gchar *stock_id;
      CtkStockItem stock_item;

      g_object_get (action, "stock-id", &stock_id, NULL);

      if (stock_id && ctk_stock_lookup (stock_id, &stock_item))
        {
          accel_key = stock_item.keyval;
          accel_mods = stock_item.modifier;
	}

      g_free (stock_id);
    }

  if (accel_key)
    ctk_accel_map_add_entry (accel_path, accel_key, accel_mods);

  ctk_action_set_accel_path (action, accel_path);
  ctk_action_group_add_action (action_group, action);

  g_free (accel_path);
}

/**
 * ctk_action_group_remove_action:
 * @action_group: the action group
 * @action: an action
 *
 * Removes an action object from the action group.
 *
 * Since: 2.4
 */
void
ctk_action_group_remove_action (CtkActionGroup *action_group,
				CtkAction      *action)
{
  CtkActionGroupPrivate *private;
  const gchar *name;

  g_return_if_fail (CTK_IS_ACTION_GROUP (action_group));
  g_return_if_fail (CTK_IS_ACTION (action));

  name = ctk_action_get_name (action);
  g_return_if_fail (name != NULL);

  private = action_group->priv;

  g_hash_table_remove (private->actions, name);
}

static void
add_single_action (gpointer key G_GNUC_UNUSED,
		   gpointer value,
		   gpointer user_data)
{
  GList **list = user_data;

  *list = g_list_prepend (*list, value);
}

/**
 * ctk_action_group_list_actions:
 * @action_group: the action group
 *
 * Lists the actions in the action group.
 *
 * Returns: (element-type CtkAction) (transfer container): an allocated list of the action objects in the action group
 *
 * Since: 2.4
 */
GList *
ctk_action_group_list_actions (CtkActionGroup *action_group)
{
  CtkActionGroupPrivate *private;
  GList *actions = NULL;

  g_return_val_if_fail (CTK_IS_ACTION_GROUP (action_group), NULL);

  private = action_group->priv;
  
  g_hash_table_foreach (private->actions, add_single_action, &actions);

  return g_list_reverse (actions);
}


/**
 * ctk_action_group_add_actions: (skip)
 * @action_group: the action group
 * @entries: (array length=n_entries): an array of action descriptions
 * @n_entries: the number of entries
 * @user_data: data to pass to the action callbacks
 *
 * This is a convenience function to create a number of actions and add them 
 * to the action group.
 *
 * The “activate” signals of the actions are connected to the callbacks
 * and their accel paths are set to `<Actions>/group-name/action-name`.  
 * 
 * Since: 2.4
 */
void
ctk_action_group_add_actions (CtkActionGroup       *action_group,
			      const CtkActionEntry *entries,
			      guint                 n_entries,
			      gpointer              user_data)
{
  ctk_action_group_add_actions_full (action_group, 
				     entries, n_entries, 
				     user_data, NULL);
}

typedef struct _SharedData  SharedData;

struct _SharedData {
  guint          ref_count;
  gpointer       data;
  GDestroyNotify destroy;
};

static void
shared_data_unref (gpointer data)
{
  SharedData *shared_data = (SharedData *)data;

  shared_data->ref_count--;
  if (shared_data->ref_count == 0)
    {
      if (shared_data->destroy)
	shared_data->destroy (shared_data->data);

      g_slice_free (SharedData, shared_data);
    }
}


/**
 * ctk_action_group_add_actions_full: (skip)
 * @action_group: the action group
 * @entries: (array length=n_entries): an array of action descriptions
 * @n_entries: the number of entries
 * @user_data: data to pass to the action callbacks
 * @destroy: (nullable): destroy notification callback for @user_data
 *
 * This variant of ctk_action_group_add_actions() adds a #GDestroyNotify
 * callback for @user_data. 
 * 
 * Since: 2.4
 */
void
ctk_action_group_add_actions_full (CtkActionGroup       *action_group,
				   const CtkActionEntry *entries,
				   guint                 n_entries,
				   gpointer              user_data,
				   GDestroyNotify        destroy)
{

  /* Keep this in sync with the other 
   * ctk_action_group_add_..._actions_full() functions.
   */
  guint i;
  SharedData *shared_data;

  g_return_if_fail (CTK_IS_ACTION_GROUP (action_group));

  shared_data = g_slice_new0 (SharedData);
  shared_data->ref_count = 1;
  shared_data->data = user_data;
  shared_data->destroy = destroy;

  for (i = 0; i < n_entries; i++)
    {
      CtkAction *action;
      const gchar *label;
      const gchar *tooltip;

      if (!check_unique_action (action_group, entries[i].name))
        continue;

      label = ctk_action_group_translate_string (action_group, entries[i].label);
      tooltip = ctk_action_group_translate_string (action_group, entries[i].tooltip);

      action = ctk_action_new (entries[i].name,
			       label,
			       tooltip,
			       NULL);

      if (entries[i].stock_id) 
	{
	  g_object_set (action, "stock-id", entries[i].stock_id, NULL);
	  if (ctk_icon_theme_has_icon (ctk_icon_theme_get_default (), 
				       entries[i].stock_id))
	    g_object_set (action, "icon-name", entries[i].stock_id, NULL);
	}
	  
      if (entries[i].callback)
	{
	  GClosure *closure;

	  closure = g_cclosure_new (entries[i].callback, user_data, NULL);
	  g_closure_add_finalize_notifier (closure, shared_data, 
					   (GClosureNotify)shared_data_unref);
	  shared_data->ref_count++;

	  g_signal_connect_closure (action, "activate", closure, FALSE);
	}
	  
      ctk_action_group_add_action_with_accel (action_group, 
					      action,
					      entries[i].accelerator);
      g_object_unref (action);
    }

  shared_data_unref (shared_data);
}

/**
 * ctk_action_group_add_toggle_actions: (skip)
 * @action_group: the action group
 * @entries: (array length=n_entries): an array of toggle action descriptions
 * @n_entries: the number of entries
 * @user_data: data to pass to the action callbacks
 *
 * This is a convenience function to create a number of toggle actions and add them 
 * to the action group.
 *
 * The “activate” signals of the actions are connected to the callbacks
 * and their accel paths are set to `<Actions>/group-name/action-name`.  
 * 
 * Since: 2.4
 */
void
ctk_action_group_add_toggle_actions (CtkActionGroup             *action_group,
				     const CtkToggleActionEntry *entries,
				     guint                       n_entries,
				     gpointer                    user_data)
{
  ctk_action_group_add_toggle_actions_full (action_group, 
					    entries, n_entries, 
					    user_data, NULL);
}


/**
 * ctk_action_group_add_toggle_actions_full: (skip)
 * @action_group: the action group
 * @entries: (array length=n_entries): an array of toggle action descriptions
 * @n_entries: the number of entries
 * @user_data: data to pass to the action callbacks
 * @destroy: (nullable): destroy notification callback for @user_data
 *
 * This variant of ctk_action_group_add_toggle_actions() adds a 
 * #GDestroyNotify callback for @user_data. 
 * 
 * Since: 2.4
 */
void
ctk_action_group_add_toggle_actions_full (CtkActionGroup             *action_group,
					  const CtkToggleActionEntry *entries,
					  guint                       n_entries,
					  gpointer                    user_data,
					  GDestroyNotify              destroy)
{
  /* Keep this in sync with the other 
   * ctk_action_group_add_..._actions_full() functions.
   */
  guint i;
  SharedData *shared_data;

  g_return_if_fail (CTK_IS_ACTION_GROUP (action_group));

  shared_data = g_slice_new0 (SharedData);
  shared_data->ref_count = 1;
  shared_data->data = user_data;
  shared_data->destroy = destroy;

  for (i = 0; i < n_entries; i++)
    {
      CtkToggleAction *action;
      const gchar *label;
      const gchar *tooltip;

      if (!check_unique_action (action_group, entries[i].name))
        continue;

      label = ctk_action_group_translate_string (action_group, entries[i].label);
      tooltip = ctk_action_group_translate_string (action_group, entries[i].tooltip);

      action = ctk_toggle_action_new (entries[i].name,
				      label,
				      tooltip,
				      NULL);

      if (entries[i].stock_id) 
	{
	  if (ctk_icon_factory_lookup_default (entries[i].stock_id))
	    g_object_set (action, "stock-id", entries[i].stock_id, NULL);
	  else
	    g_object_set (action, "icon-name", entries[i].stock_id, NULL);
	}

      ctk_toggle_action_set_active (action, entries[i].is_active);

      if (entries[i].callback)
	{
	  GClosure *closure;

	  closure = g_cclosure_new (entries[i].callback, user_data, NULL);
	  g_closure_add_finalize_notifier (closure, shared_data, 
					   (GClosureNotify)shared_data_unref);
	  shared_data->ref_count++;

	  g_signal_connect_closure (action, "activate", closure, FALSE);
	}
	  
      ctk_action_group_add_action_with_accel (action_group, 
					      CTK_ACTION (action),
					      entries[i].accelerator);
      g_object_unref (action);
    }

  shared_data_unref (shared_data);
}

/**
 * ctk_action_group_add_radio_actions: (skip)
 * @action_group: the action group
 * @entries: (array length=n_entries): an array of radio action descriptions
 * @n_entries: the number of entries
 * @value: the value of the action to activate initially, or -1 if
 *   no action should be activated
 * @on_change: the callback to connect to the changed signal
 * @user_data: data to pass to the action callbacks
 * 
 * This is a convenience routine to create a group of radio actions and
 * add them to the action group. 
 *
 * The “changed” signal of the first radio action is connected to the 
 * @on_change callback and the accel paths of the actions are set to 
 * `<Actions>/group-name/action-name`.  
 * 
 * Since: 2.4
 **/
void            
ctk_action_group_add_radio_actions (CtkActionGroup            *action_group,
				    const CtkRadioActionEntry *entries,
				    guint                      n_entries,
				    gint                       value,
				    GCallback                  on_change,
				    gpointer                   user_data)
{
  ctk_action_group_add_radio_actions_full (action_group, 
					   entries, n_entries, 
					   value,
					   on_change, user_data, NULL);
}

/**
 * ctk_action_group_add_radio_actions_full: (skip)
 * @action_group: the action group
 * @entries: (array length=n_entries): an array of radio action descriptions
 * @n_entries: the number of entries
 * @value: the value of the action to activate initially, or -1 if
 *   no action should be activated
 * @on_change: the callback to connect to the changed signal
 * @user_data: data to pass to the action callbacks
 * @destroy: destroy notification callback for @user_data
 *
 * This variant of ctk_action_group_add_radio_actions() adds a 
 * #GDestroyNotify callback for @user_data. 
 * 
 * Since: 2.4
 **/
void            
ctk_action_group_add_radio_actions_full (CtkActionGroup            *action_group,
					 const CtkRadioActionEntry *entries,
					 guint                      n_entries,
					 gint                       value,
					 GCallback                  on_change,
					 gpointer                   user_data,
					 GDestroyNotify             destroy)
{
  /* Keep this in sync with the other 
   * ctk_action_group_add_..._actions_full() functions.
   */
  guint i;
  GSList *group = NULL;
  CtkRadioAction *first_action = NULL;

  g_return_if_fail (CTK_IS_ACTION_GROUP (action_group));

  for (i = 0; i < n_entries; i++)
    {
      CtkRadioAction *action;
      const gchar *label;
      const gchar *tooltip; 

      if (!check_unique_action (action_group, entries[i].name))
        continue;

      label = ctk_action_group_translate_string (action_group, entries[i].label);
      tooltip = ctk_action_group_translate_string (action_group, entries[i].tooltip);

      action = ctk_radio_action_new (entries[i].name,
				     label,
				     tooltip,
				     NULL,
				     entries[i].value);

      if (entries[i].stock_id) 
	{
	  if (ctk_icon_factory_lookup_default (entries[i].stock_id))
	    g_object_set (action, "stock-id", entries[i].stock_id, NULL);
	  else
	    g_object_set (action, "icon-name", entries[i].stock_id, NULL);
	}

      if (i == 0) 
	first_action = action;

      ctk_radio_action_set_group (action, group);
      group = ctk_radio_action_get_group (action);

      if (value == entries[i].value)
	ctk_toggle_action_set_active (CTK_TOGGLE_ACTION (action), TRUE);

      ctk_action_group_add_action_with_accel (action_group, 
					      CTK_ACTION (action),
					      entries[i].accelerator);
      g_object_unref (action);
    }

  if (on_change && first_action)
    g_signal_connect_data (first_action, "changed",
			   on_change, user_data, 
			   (GClosureNotify)destroy, 0);
}

/**
 * ctk_action_group_set_translate_func:
 * @action_group: a #CtkActionGroup
 * @func: a #CtkTranslateFunc
 * @data: data to be passed to @func and @notify
 * @notify: a #GDestroyNotify function to be called when @action_group is
 *   destroyed and when the translation function is changed again
 *
 * Sets a function to be used for translating the @label and @tooltip of 
 * #CtkActionEntrys added by ctk_action_group_add_actions().
 *
 * If you’re using gettext(), it is enough to set the translation domain
 * with ctk_action_group_set_translation_domain().
 *
 * Since: 2.4
 **/
void
ctk_action_group_set_translate_func (CtkActionGroup   *action_group,
				     CtkTranslateFunc  func,
				     gpointer          data,
				     GDestroyNotify    notify)
{
  CtkActionGroupPrivate *private;

  g_return_if_fail (CTK_IS_ACTION_GROUP (action_group));
  
  private = action_group->priv;

  if (private->translate_notify)
    private->translate_notify (private->translate_data);
      
  private->translate_func = func;
  private->translate_data = data;
  private->translate_notify = notify;
}

static gchar *
dgettext_swapped (const gchar *msgid, 
		  const gchar *domainname)
{
  /* Pass through g_dgettext if and only if msgid is nonempty. */
  if (msgid && *msgid) 
    return (gchar*) g_dgettext (domainname, msgid); 
  else
    return (gchar*) msgid;
}

/**
 * ctk_action_group_set_translation_domain:
 * @action_group: a #CtkActionGroup
 * @domain: (allow-none): the translation domain to use for g_dgettext()
 * calls, or %NULL to use the domain set with textdomain()
 * 
 * Sets the translation domain and uses g_dgettext() for translating the 
 * @label and @tooltip of #CtkActionEntrys added by 
 * ctk_action_group_add_actions().
 *
 * If you’re not using gettext() for localization, see 
 * ctk_action_group_set_translate_func().
 *
 * Since: 2.4
 **/
void 
ctk_action_group_set_translation_domain (CtkActionGroup *action_group,
					 const gchar    *domain)
{
  g_return_if_fail (CTK_IS_ACTION_GROUP (action_group));

  ctk_action_group_set_translate_func (action_group, 
				       (CtkTranslateFunc)dgettext_swapped,
				       g_strdup (domain),
				       g_free);
} 


/**
 * ctk_action_group_translate_string:
 * @action_group: a #CtkActionGroup
 * @string: a string
 *
 * Translates a string using the function set with 
 * ctk_action_group_set_translate_func(). This
 * is mainly intended for language bindings.
 *
 * Returns: the translation of @string
 *
 * Since: 2.6
 **/
const gchar *
ctk_action_group_translate_string (CtkActionGroup *action_group,
				   const gchar    *string)
{
  CtkActionGroupPrivate *private;
  CtkTranslateFunc translate_func;
  gpointer translate_data;
  
  g_return_val_if_fail (CTK_IS_ACTION_GROUP (action_group), string);
  
  if (string == NULL)
    return NULL;

  private = action_group->priv;

  translate_func = private->translate_func;
  translate_data = private->translate_data;
  
  if (translate_func)
    return translate_func (string, translate_data);
  else
    return string;
}

/* Protected for use by CtkAction */
void
_ctk_action_group_emit_connect_proxy  (CtkActionGroup *action_group,
                                       CtkAction      *action,
                                       CtkWidget      *proxy)
{
  g_signal_emit (action_group, action_group_signals[CONNECT_PROXY], 0, 
                 action, proxy);
}

void
_ctk_action_group_emit_disconnect_proxy  (CtkActionGroup *action_group,
                                          CtkAction      *action,
                                          CtkWidget      *proxy)
{
  g_signal_emit (action_group, action_group_signals[DISCONNECT_PROXY], 0, 
                 action, proxy);
}

void
_ctk_action_group_emit_pre_activate  (CtkActionGroup *action_group,
				      CtkAction      *action)
{
  g_signal_emit (action_group, action_group_signals[PRE_ACTIVATE], 0, action);
}

void
_ctk_action_group_emit_post_activate (CtkActionGroup *action_group,
				      CtkAction      *action)
{
  g_signal_emit (action_group, action_group_signals[POST_ACTIVATE], 0, action);
}
