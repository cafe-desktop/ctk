/* gtkactivatable.c
 * Copyright (C) 2008 Tristan Van Berkom <tristan.van.berkom@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * SECTION:gtkactivatable
 * @Short_Description: An interface for activatable widgets
 * @Title: GtkActivatable
 *
 * Activatable widgets can be connected to a #GtkAction and reflects
 * the state of its action. A #GtkActivatable can also provide feedback
 * through its action, as they are responsible for activating their
 * related actions.
 *
 * # Implementing GtkActivatable
 *
 * When extending a class that is already #GtkActivatable; it is only
 * necessary to implement the #GtkActivatable->sync_action_properties()
 * and #GtkActivatable->update() methods and chain up to the parent
 * implementation, however when introducing
 * a new #GtkActivatable class; the #GtkActivatable:related-action and
 * #GtkActivatable:use-action-appearance properties need to be handled by
 * the implementor. Handling these properties is mostly a matter of installing
 * the action pointer and boolean flag on your instance, and calling
 * ctk_activatable_do_set_related_action() and
 * ctk_activatable_sync_action_properties() at the appropriate times.
 *
 * ## A class fragment implementing #GtkActivatable
 *
 * |[<!-- language="C" -->
 *
 * enum {
 * ...
 *
 * PROP_ACTIVATABLE_RELATED_ACTION,
 * PROP_ACTIVATABLE_USE_ACTION_APPEARANCE
 * }
 * 
 * struct _FooBarPrivate
 * {
 * 
 *   ...
 * 
 *   GtkAction      *action;
 *   gboolean        use_action_appearance;
 * };
 * 
 * ...
 * 
 * static void foo_bar_activatable_interface_init         (GtkActivatableIface  *iface);
 * static void foo_bar_activatable_update                 (GtkActivatable       *activatable,
 * 						           GtkAction            *action,
 * 						           const gchar          *property_name);
 * static void foo_bar_activatable_sync_action_properties (GtkActivatable       *activatable,
 * 						           GtkAction            *action);
 * ...
 *
 *
 * static void
 * foo_bar_class_init (FooBarClass *klass)
 * {
 *
 *   ...
 *
 *   g_object_class_override_property (gobject_class, PROP_ACTIVATABLE_RELATED_ACTION, "related-action");
 *   g_object_class_override_property (gobject_class, PROP_ACTIVATABLE_USE_ACTION_APPEARANCE, "use-action-appearance");
 *
 *   ...
 * }
 *
 *
 * static void
 * foo_bar_activatable_interface_init (GtkActivatableIface  *iface)
 * {
 *   iface->update = foo_bar_activatable_update;
 *   iface->sync_action_properties = foo_bar_activatable_sync_action_properties;
 * }
 * 
 * ... Break the reference using ctk_activatable_do_set_related_action()...
 *
 * static void 
 * foo_bar_dispose (GObject *object)
 * {
 *   FooBar *bar = FOO_BAR (object);
 *   FooBarPrivate *priv = FOO_BAR_GET_PRIVATE (bar);
 * 
 *   ...
 * 
 *   if (priv->action)
 *     {
 *       ctk_activatable_do_set_related_action (CTK_ACTIVATABLE (bar), NULL);
 *       priv->action = NULL;
 *     }
 *   G_OBJECT_CLASS (foo_bar_parent_class)->dispose (object);
 * }
 * 
 * ... Handle the “related-action” and “use-action-appearance” properties ...
 *
 * static void
 * foo_bar_set_property (GObject         *object,
 *                       guint            prop_id,
 *                       const GValue    *value,
 *                       GParamSpec      *pspec)
 * {
 *   FooBar *bar = FOO_BAR (object);
 *   FooBarPrivate *priv = FOO_BAR_GET_PRIVATE (bar);
 * 
 *   switch (prop_id)
 *     {
 * 
 *       ...
 * 
 *     case PROP_ACTIVATABLE_RELATED_ACTION:
 *       foo_bar_set_related_action (bar, g_value_get_object (value));
 *       break;
 *     case PROP_ACTIVATABLE_USE_ACTION_APPEARANCE:
 *       foo_bar_set_use_action_appearance (bar, g_value_get_boolean (value));
 *       break;
 *     default:
 *       G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
 *       break;
 *     }
 * }
 * 
 * static void
 * foo_bar_get_property (GObject         *object,
 *                          guint            prop_id,
 *                          GValue          *value,
 *                          GParamSpec      *pspec)
 * {
 *   FooBar *bar = FOO_BAR (object);
 *   FooBarPrivate *priv = FOO_BAR_GET_PRIVATE (bar);
 * 
 *   switch (prop_id)
 *     { 
 * 
 *       ...
 * 
 *     case PROP_ACTIVATABLE_RELATED_ACTION:
 *       g_value_set_object (value, priv->action);
 *       break;
 *     case PROP_ACTIVATABLE_USE_ACTION_APPEARANCE:
 *       g_value_set_boolean (value, priv->use_action_appearance);
 *       break;
 *     default:
 *       G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
 *       break;
 *     }
 * }
 * 
 * 
 * static void
 * foo_bar_set_use_action_appearance (FooBar   *bar, 
 * 				   gboolean  use_appearance)
 * {
 *   FooBarPrivate *priv = FOO_BAR_GET_PRIVATE (bar);
 * 
 *   if (priv->use_action_appearance != use_appearance)
 *     {
 *       priv->use_action_appearance = use_appearance;
 *       
 *       ctk_activatable_sync_action_properties (CTK_ACTIVATABLE (bar), priv->action);
 *     }
 * }
 * 
 * ... call ctk_activatable_do_set_related_action() and then assign the action pointer, 
 * no need to reference the action here since ctk_activatable_do_set_related_action() already 
 * holds a reference here for you...
 * static void
 * foo_bar_set_related_action (FooBar    *bar, 
 * 			    GtkAction *action)
 * {
 *   FooBarPrivate *priv = FOO_BAR_GET_PRIVATE (bar);
 * 
 *   if (priv->action == action)
 *     return;
 * 
 *   ctk_activatable_do_set_related_action (CTK_ACTIVATABLE (bar), action);
 * 
 *   priv->action = action;
 * }
 * 
 * ... Selectively reset and update activatable depending on the use-action-appearance property ...
 * static void
 * ctk_button_activatable_sync_action_properties (GtkActivatable       *activatable,
 * 		                                  GtkAction            *action)
 * {
 *   GtkButtonPrivate *priv = CTK_BUTTON_GET_PRIVATE (activatable);
 * 
 *   if (!action)
 *     return;
 * 
 *   if (ctk_action_is_visible (action))
 *     ctk_widget_show (CTK_WIDGET (activatable));
 *   else
 *     ctk_widget_hide (CTK_WIDGET (activatable));
 *   
 *   ctk_widget_set_sensitive (CTK_WIDGET (activatable), ctk_action_is_sensitive (action));
 * 
 *   ...
 *   
 *   if (priv->use_action_appearance)
 *     {
 *       if (ctk_action_get_stock_id (action))
 * 	foo_bar_set_stock (button, ctk_action_get_stock_id (action));
 *       else if (ctk_action_get_label (action))
 * 	foo_bar_set_label (button, ctk_action_get_label (action));
 * 
 *       ...
 * 
 *     }
 * }
 * 
 * static void 
 * foo_bar_activatable_update (GtkActivatable       *activatable,
 * 			       GtkAction            *action,
 * 			       const gchar          *property_name)
 * {
 *   FooBarPrivate *priv = FOO_BAR_GET_PRIVATE (activatable);
 * 
 *   if (strcmp (property_name, "visible") == 0)
 *     {
 *       if (ctk_action_is_visible (action))
 * 	ctk_widget_show (CTK_WIDGET (activatable));
 *       else
 * 	ctk_widget_hide (CTK_WIDGET (activatable));
 *     }
 *   else if (strcmp (property_name, "sensitive") == 0)
 *     ctk_widget_set_sensitive (CTK_WIDGET (activatable), ctk_action_is_sensitive (action));
 * 
 *   ...
 * 
 *   if (!priv->use_action_appearance)
 *     return;
 * 
 *   if (strcmp (property_name, "stock-id") == 0)
 *     foo_bar_set_stock (button, ctk_action_get_stock_id (action));
 *   else if (strcmp (property_name, "label") == 0)
 *     foo_bar_set_label (button, ctk_action_get_label (action));
 * 
 *   ...
 * }
 * ]|
 */

#include "config.h"

#define GDK_DISABLE_DEPRECATION_WARNINGS

#include "gtkactivatable.h"
#include "gtkactiongroup.h"
#include "gtkprivate.h"
#include "gtkintl.h"


typedef GtkActivatableIface GtkActivatableInterface;
G_DEFINE_INTERFACE (GtkActivatable, ctk_activatable, G_TYPE_OBJECT)

static void
ctk_activatable_default_init (GtkActivatableInterface *iface)
{
  /**
   * GtkActivatable:related-action:
   *
   * The action that this activatable will activate and receive
   * updates from for various states and possibly appearance.
   *
   * > #GtkActivatable implementors need to handle the this property and
   * > call ctk_activatable_do_set_related_action() when it changes.
   *
   * Since: 2.16
   *
   * Deprecated: 3.10
   */
  g_object_interface_install_property (iface,
				       g_param_spec_object ("related-action",
							    P_("Related Action"),
							    P_("The action this activatable will activate and receive updates from"),
							    CTK_TYPE_ACTION,
							    CTK_PARAM_READWRITE));

  /**
   * GtkActivatable:use-action-appearance:
   *
   * Whether this activatable should reset its layout
   * and appearance when setting the related action or when
   * the action changes appearance.
   *
   * See the #GtkAction documentation directly to find which properties
   * should be ignored by the #GtkActivatable when this property is %FALSE.
   *
   * > #GtkActivatable implementors need to handle this property
   * > and call ctk_activatable_sync_action_properties() on the activatable
   * > widget when it changes.
   *
   * Since: 2.16
   *
   * Deprecated: 3.10
   */
  g_object_interface_install_property (iface,
				       g_param_spec_boolean ("use-action-appearance",
							     P_("Use Action Appearance"),
							     P_("Whether to use the related actions appearance properties"),
							     TRUE,
							     CTK_PARAM_READWRITE));


}

static void
ctk_activatable_update (GtkActivatable *activatable,
			GtkAction      *action,
			const gchar    *property_name)
{
  GtkActivatableIface *iface;

  g_return_if_fail (CTK_IS_ACTIVATABLE (activatable));

  iface = CTK_ACTIVATABLE_GET_IFACE (activatable);
  if (iface->update)
    iface->update (activatable, action, property_name);
  else
    g_critical ("GtkActivatable->update() unimplemented for type %s", 
		g_type_name (G_OBJECT_TYPE (activatable)));
}

/**
 * ctk_activatable_sync_action_properties:
 * @activatable: a #GtkActivatable
 * @action: (allow-none): the related #GtkAction or %NULL
 *
 * This is called to update the activatable completely, this is called
 * internally when the #GtkActivatable:related-action property is set
 * or unset and by the implementing class when
 * #GtkActivatable:use-action-appearance changes.
 *
 * Since: 2.16
 *
 * Deprecated: 3.10
 **/
void
ctk_activatable_sync_action_properties (GtkActivatable *activatable,
		                        GtkAction      *action)
{
  GtkActivatableIface *iface;

  g_return_if_fail (CTK_IS_ACTIVATABLE (activatable));

  iface = CTK_ACTIVATABLE_GET_IFACE (activatable);
  if (iface->sync_action_properties)
    iface->sync_action_properties (activatable, action);
  else
    g_critical ("GtkActivatable->sync_action_properties() unimplemented for type %s", 
		g_type_name (G_OBJECT_TYPE (activatable)));
}


/**
 * ctk_activatable_set_related_action:
 * @activatable: a #GtkActivatable
 * @action: the #GtkAction to set
 *
 * Sets the related action on the @activatable object.
 *
 * > #GtkActivatable implementors need to handle the #GtkActivatable:related-action
 * > property and call ctk_activatable_do_set_related_action() when it changes.
 *
 * Since: 2.16
 *
 * Deprecated: 3.10
 **/
void
ctk_activatable_set_related_action (GtkActivatable *activatable,
				    GtkAction      *action)
{
  g_return_if_fail (CTK_IS_ACTIVATABLE (activatable));
  g_return_if_fail (action == NULL || CTK_IS_ACTION (action));

  g_object_set (activatable, "related-action", action, NULL);
}

static void
ctk_activatable_action_notify (GtkAction      *action,
			       GParamSpec     *pspec,
			       GtkActivatable *activatable)
{
  ctk_activatable_update (activatable, action, pspec->name);
}

/**
 * ctk_activatable_do_set_related_action:
 * @activatable: a #GtkActivatable
 * @action: the #GtkAction to set
 * 
 * This is a utility function for #GtkActivatable implementors.
 * 
 * When implementing #GtkActivatable you must call this when
 * handling changes of the #GtkActivatable:related-action, and
 * you must also use this to break references in #GObject->dispose().
 *
 * This function adds a reference to the currently set related
 * action for you, it also makes sure the #GtkActivatable->update()
 * method is called when the related #GtkAction properties change
 * and registers to the action’s proxy list.
 *
 * > Be careful to call this before setting the local
 * > copy of the #GtkAction property, since this function uses 
 * > ctk_activatable_get_related_action() to retrieve the
 * > previous action.
 *
 * Since: 2.16
 *
 * Deprecated: 3.10
 */
void
ctk_activatable_do_set_related_action (GtkActivatable *activatable,
				       GtkAction      *action)
{
  GtkAction *prev_action;

  prev_action = ctk_activatable_get_related_action (activatable);
  
  if (prev_action != action)
    {
      if (prev_action)
	{
	  g_signal_handlers_disconnect_by_func (prev_action, ctk_activatable_action_notify, activatable);
	  
          /* Check the type so that actions can be activatable too. */
          if (CTK_IS_WIDGET (activatable))
            _ctk_action_remove_from_proxy_list (prev_action, CTK_WIDGET (activatable));
	  
          /* Some apps are using the object data directly...
           * so continue to set it for a bit longer
           */
          g_object_set_data (G_OBJECT (activatable), "gtk-action", NULL);

          /*
           * We don't want prev_action to be activated
           * during the sync_action_properties() call when syncing "active".
           */ 
          ctk_action_block_activate (prev_action);
	}
      
      /* Some applications rely on their proxy UI to be set up
       * before they receive the ::connect-proxy signal, so we
       * need to call sync_action_properties() before add_to_proxy_list().
       */
      ctk_activatable_sync_action_properties (activatable, action);

      if (prev_action)
        {
          ctk_action_unblock_activate (prev_action);
	  g_object_unref (prev_action);
        }

      if (action)
	{
	  g_object_ref (action);

	  g_signal_connect (G_OBJECT (action), "notify", G_CALLBACK (ctk_activatable_action_notify), activatable);

          if (CTK_IS_WIDGET (activatable))
            _ctk_action_add_to_proxy_list (action, CTK_WIDGET (activatable));

          g_object_set_data (G_OBJECT (activatable), "gtk-action", action);
	}
    }
}

/**
 * ctk_activatable_get_related_action:
 * @activatable: a #GtkActivatable
 *
 * Gets the related #GtkAction for @activatable.
 *
 * Returns: (transfer none): the related #GtkAction if one is set.
 *
 * Since: 2.16
 *
 * Deprecated: 3.10
 **/
GtkAction *
ctk_activatable_get_related_action (GtkActivatable *activatable)
{
  GtkAction *action;

  g_return_val_if_fail (CTK_IS_ACTIVATABLE (activatable), NULL);

  g_object_get (activatable, "related-action", &action, NULL);

  /* g_object_get() gives us a ref... */
  if (action)
    g_object_unref (action);

  return action;
}

/**
 * ctk_activatable_set_use_action_appearance:
 * @activatable: a #GtkActivatable
 * @use_appearance: whether to use the actions appearance
 *
 * Sets whether this activatable should reset its layout and appearance
 * when setting the related action or when the action changes appearance
 *
 * > #GtkActivatable implementors need to handle the
 * > #GtkActivatable:use-action-appearance property and call
 * > ctk_activatable_sync_action_properties() to update @activatable
 * > if needed.
 *
 * Since: 2.16
  *
 * Deprecated: 3.10
**/
void
ctk_activatable_set_use_action_appearance (GtkActivatable *activatable,
					   gboolean        use_appearance)
{
  g_object_set (activatable, "use-action-appearance", use_appearance, NULL);
}

/**
 * ctk_activatable_get_use_action_appearance:
 * @activatable: a #GtkActivatable
 *
 * Gets whether this activatable should reset its layout
 * and appearance when setting the related action or when
 * the action changes appearance.
 *
 * Returns: whether @activatable uses its actions appearance.
 *
 * Since: 2.16
  *
 * Deprecated: 3.10
**/
gboolean
ctk_activatable_get_use_action_appearance  (GtkActivatable *activatable)
{
  gboolean use_appearance;

  g_object_get (activatable, "use-action-appearance", &use_appearance, NULL);  

  return use_appearance;
}
