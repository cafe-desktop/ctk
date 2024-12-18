/*
 * Copyright © 2010 Codethink Limited
 * Copyright © 2013 Canonical Limited
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the licence, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#include "config.h"

#include "ctkapplication.h"
#include "cdkprivate.h"

#ifdef G_OS_UNIX
#include <gio/gunixfdlist.h>
#endif

#include <stdlib.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "cdk/cdk-private.h"

#include "ctkapplicationprivate.h"
#include "ctkclipboardprivate.h"
#include "ctkmarshalers.h"
#include "ctkmain.h"
#include "ctkrecentmanager.h"
#include "ctkaccelmapprivate.h"
#include "ctkicontheme.h"
#include "ctkbuilder.h"
#include "ctkshortcutswindow.h"
#include "ctkintl.h"

/* NB: please do not add backend-specific CDK headers here.  This should
 * be abstracted via CtkApplicationImpl.
 */

/**
 * SECTION:ctkapplication
 * @title: CtkApplication
 * @short_description: Application class
 *
 * #CtkApplication is a class that handles many important aspects
 * of a CTK+ application in a convenient fashion, without enforcing
 * a one-size-fits-all application model.
 *
 * Currently, CtkApplication handles CTK+ initialization, application
 * uniqueness, session management, provides some basic scriptability and
 * desktop shell integration by exporting actions and menus and manages a
 * list of toplevel windows whose life-cycle is automatically tied to the
 * life-cycle of your application.
 *
 * While CtkApplication works fine with plain #CtkWindows, it is recommended
 * to use it together with #CtkApplicationWindow.
 *
 * When CDK threads are enabled, CtkApplication will acquire the CDK
 * lock when invoking actions that arrive from other processes.  The CDK
 * lock is not touched for local action invocations.  In order to have
 * actions invoked in a predictable context it is therefore recommended
 * that the CDK lock be held while invoking actions locally with
 * g_action_group_activate_action().  The same applies to actions
 * associated with #CtkApplicationWindow and to the “activate” and
 * “open” #GApplication methods.
 *
 * ## Automatic resources ## {#automatic-resources}
 *
 * #CtkApplication will automatically load menus from the #CtkBuilder
 * resource located at "ctk/menus.ui", relative to the application's
 * resource base path (see g_application_set_resource_base_path()).  The
 * menu with the ID "app-menu" is taken as the application's app menu
 * and the menu with the ID "menubar" is taken as the application's
 * menubar.  Additional menus (most interesting submenus) can be named
 * and accessed via ctk_application_get_menu_by_id() which allows for
 * dynamic population of a part of the menu structure.
 *
 * If the resources "ctk/menus-appmenu.ui" or "ctk/menus-traditional.ui" are
 * present then these files will be used in preference, depending on the value
 * of ctk_application_prefers_app_menu(). If the resource "ctk/menus-common.ui"
 * is present it will be loaded as well. This is useful for storing items that
 * are referenced from both "ctk/menus-appmenu.ui" and
 * "ctk/menus-traditional.ui".
 *
 * It is also possible to provide the menus manually using
 * ctk_application_set_app_menu() and ctk_application_set_menubar().
 *
 * #CtkApplication will also automatically setup an icon search path for
 * the default icon theme by appending "icons" to the resource base
 * path.  This allows your application to easily store its icons as
 * resources.  See ctk_icon_theme_add_resource_path() for more
 * information.
 *
 * If there is a resource located at "ctk/help-overlay.ui" which
 * defines a #CtkShortcutsWindow with ID "help_overlay" then CtkApplication
 * associates an instance of this shortcuts window with each
 * #CtkApplicationWindow and sets up keyboard accelerators (Control-F1
 * and Control-?) to open it. To create a menu item that displays the
 * shortcuts window, associate the item with the action win.show-help-overlay.
 *
 * ## A simple application ## {#ctkapplication}
 *
 * [A simple example](https://git.gnome.org/browse/ctk+/tree/examples/bp/bloatpad.c)
 *
 * CtkApplication optionally registers with a session manager
 * of the users session (if you set the #CtkApplication:register-session
 * property) and offers various functionality related to the session
 * life-cycle.
 *
 * An application can block various ways to end the session with
 * the ctk_application_inhibit() function. Typical use cases for
 * this kind of inhibiting are long-running, uninterruptible operations,
 * such as burning a CD or performing a disk backup. The session
 * manager may not honor the inhibitor, but it can be expected to
 * inform the user about the negative consequences of ending the
 * session while inhibitors are present.
 *
 * ## See Also ## {#seealso}
 * [HowDoI: Using CtkApplication](https://wiki.gnome.org/HowDoI/CtkApplication),
 * [Getting Started with CTK+: Basics](https://developer.gnome.org/ctk3/stable/ctk-getting-started.html#id-1.2.3.3)
 */

enum {
  WINDOW_ADDED,
  WINDOW_REMOVED,
  QUERY_END,
  LAST_SIGNAL
};

static guint ctk_application_signals[LAST_SIGNAL];

enum {
  PROP_ZERO,
  PROP_REGISTER_SESSION,
  PROP_SCREENSAVER_ACTIVE,
  PROP_APP_MENU,
  PROP_MENUBAR,
  PROP_ACTIVE_WINDOW,
  NUM_PROPERTIES
};

static GParamSpec *ctk_application_props[NUM_PROPERTIES];

struct _CtkApplicationPrivate
{
  CtkApplicationImpl *impl;
  CtkApplicationAccels *accels;

  GList *windows;

  GMenuModel      *app_menu;
  GMenuModel      *menubar;
  guint            last_window_id;

  gboolean         register_session;
  gboolean         screensaver_active;
  CtkActionMuxer  *muxer;
  CtkBuilder      *menus_builder;
  gchar           *help_overlay_path;
  guint            profiler_id;
};

G_DEFINE_TYPE_WITH_PRIVATE (CtkApplication, ctk_application, G_TYPE_APPLICATION)

static gboolean
ctk_application_focus_in_event_cb (CtkWindow      *window,
                                   CdkEventFocus  *event G_GNUC_UNUSED,
                                   CtkApplication *application)
{
  CtkApplicationPrivate *priv = application->priv;
  GList *link;

  /* Keep the window list sorted by most-recently-focused. */
  link = g_list_find (priv->windows, window);
  if (link != NULL && link != priv->windows)
    {
      priv->windows = g_list_remove_link (priv->windows, link);
      priv->windows = g_list_concat (link, priv->windows);
    }

  if (application->priv->impl)
    ctk_application_impl_active_window_changed (application->priv->impl, window);

  g_object_notify_by_pspec (G_OBJECT (application), ctk_application_props[PROP_ACTIVE_WINDOW]);

  return CDK_EVENT_PROPAGATE;
}

static void
ctk_application_load_resources (CtkApplication *application)
{
  const gchar *base_path;

  base_path = g_application_get_resource_base_path (G_APPLICATION (application));

  if (base_path == NULL)
    return;

  /* Expand the icon search path */
  {
    CtkIconTheme *default_theme;
    gchar *iconspath;

    default_theme = ctk_icon_theme_get_default ();
    iconspath = g_strconcat (base_path, "/icons/", NULL);
    ctk_icon_theme_add_resource_path (default_theme, iconspath);
    g_free (iconspath);
  }

  /* Load the menus */
  {
    gchar *menuspath;

    /* If the user has given a specific file for the variant of menu
     * that we are looking for, use it with preference.
     */
    if (ctk_application_prefers_app_menu (application))
      menuspath = g_strconcat (base_path, "/ctk/menus-appmenu.ui", NULL);
    else
      menuspath = g_strconcat (base_path, "/ctk/menus-traditional.ui", NULL);

    if (g_resources_get_info (menuspath, G_RESOURCE_LOOKUP_FLAGS_NONE, NULL, NULL, NULL))
      application->priv->menus_builder = ctk_builder_new_from_resource (menuspath);
    g_free (menuspath);

    /* If we didn't get the specific file, fall back. */
    if (application->priv->menus_builder == NULL)
      {
        menuspath = g_strconcat (base_path, "/ctk/menus.ui", NULL);
        if (g_resources_get_info (menuspath, G_RESOURCE_LOOKUP_FLAGS_NONE, NULL, NULL, NULL))
          application->priv->menus_builder = ctk_builder_new_from_resource (menuspath);
        g_free (menuspath);
      }

    /* Always load from -common as well, if we have it */
    menuspath = g_strconcat (base_path, "/ctk/menus-common.ui", NULL);
    if (g_resources_get_info (menuspath, G_RESOURCE_LOOKUP_FLAGS_NONE, NULL, NULL, NULL))
      {
        GError *error = NULL;

        if (application->priv->menus_builder == NULL)
          application->priv->menus_builder = ctk_builder_new ();

        if (!ctk_builder_add_from_resource (application->priv->menus_builder, menuspath, &error))
          g_error ("failed to load menus-common.ui: %s", error->message);
      }
    g_free (menuspath);

    if (application->priv->menus_builder)
      {
        GObject *menu;

        menu = ctk_builder_get_object (application->priv->menus_builder, "app-menu");
        if (menu != NULL && G_IS_MENU_MODEL (menu))
          ctk_application_set_app_menu (application, G_MENU_MODEL (menu));
        menu = ctk_builder_get_object (application->priv->menus_builder, "menubar");
        if (menu != NULL && G_IS_MENU_MODEL (menu))
          ctk_application_set_menubar (application, G_MENU_MODEL (menu));
      }
  }

  /* Help overlay */
  {
    gchar *path;

    path = g_strconcat (base_path, "/ctk/help-overlay.ui", NULL);
    if (g_resources_get_info (path, G_RESOURCE_LOOKUP_FLAGS_NONE, NULL, NULL, NULL))
      {
        const gchar * const accels[] = { "<Primary>F1", "<Primary>question", NULL };

        application->priv->help_overlay_path = path;
        ctk_application_set_accels_for_action (application, "win.show-help-overlay", accels);
      }
    else
      {
        g_free (path);
      }
  }
}


static void
ctk_application_startup (GApplication *g_application)
{
  CtkApplication *application = CTK_APPLICATION (g_application);

  G_APPLICATION_CLASS (ctk_application_parent_class)->startup (g_application);

  ctk_action_muxer_insert (application->priv->muxer, "app", G_ACTION_GROUP (application));

  ctk_init (NULL, NULL);

  application->priv->impl = ctk_application_impl_new (application, cdk_display_get_default ());
  ctk_application_impl_startup (application->priv->impl, application->priv->register_session);

  ctk_application_load_resources (application);
}

static void
ctk_application_shutdown (GApplication *g_application)
{
  CtkApplication *application = CTK_APPLICATION (g_application);

  if (application->priv->impl == NULL)
    return;

  ctk_application_impl_shutdown (application->priv->impl);
  g_clear_object (&application->priv->impl);

  ctk_action_muxer_remove (application->priv->muxer, "app");

  /* Keep this section in sync with ctk_main() */

  /* Try storing all clipboard data we have */
  _ctk_clipboard_store_all ();

  /* Synchronize the recent manager singleton */
  _ctk_recent_manager_sync ();

  G_APPLICATION_CLASS (ctk_application_parent_class)->shutdown (g_application);
}

static gboolean
ctk_application_local_command_line (GApplication   *application,
                                    gchar        ***arguments,
                                    gint           *exit_status)
{
  g_application_add_option_group (application, ctk_get_option_group (FALSE));

  return G_APPLICATION_CLASS (ctk_application_parent_class)->local_command_line (application, arguments, exit_status);
}

static void
ctk_application_add_platform_data (GApplication    *application G_GNUC_UNUSED,
                                   GVariantBuilder *builder)
{
  /* This is slightly evil.
   *
   * We don't have an impl here because we're remote so we can't figure
   * out what to do on a per-display-server basis.
   *
   * So we do all the things... which currently is just one thing.
   */
  const gchar *desktop_startup_id =
    CDK_PRIVATE_CALL (cdk_get_desktop_startup_id) ();
  if (desktop_startup_id)
    g_variant_builder_add (builder, "{sv}", "desktop-startup-id",
                           g_variant_new_string (desktop_startup_id));
}

static void
ctk_application_before_emit (GApplication *g_application,
                             GVariant     *platform_data)
{
  CtkApplication *application = CTK_APPLICATION (g_application);

  cdk_threads_enter ();

  ctk_application_impl_before_emit (application->priv->impl, platform_data);
}

static void
ctk_application_after_emit (GApplication *application G_GNUC_UNUSED,
                            GVariant     *platform_data)
{
  const char *startup_notification_id = NULL;

  g_variant_lookup (platform_data, "desktop-startup-id", "&s", &startup_notification_id);
  if (startup_notification_id)
    {
      CdkDisplay *display;

      display = cdk_display_get_default ();
      if (display)
        cdk_display_notify_startup_complete (display, startup_notification_id);
    }

  cdk_threads_leave ();
}

static void
ctk_application_init (CtkApplication *application)
{
  application->priv = ctk_application_get_instance_private (application);

  application->priv->muxer = ctk_action_muxer_new ();

  application->priv->accels = ctk_application_accels_new ();

  /* getenv now at the latest */
  CDK_PRIVATE_CALL (cdk_get_desktop_startup_id) ();
}

static void
ctk_application_window_added (CtkApplication *application,
                              CtkWindow      *window)
{
  CtkApplicationPrivate *priv = application->priv;

  if (CTK_IS_APPLICATION_WINDOW (window))
    {
      ctk_application_window_set_id (CTK_APPLICATION_WINDOW (window), ++priv->last_window_id);
      if (priv->help_overlay_path)
        {
          CtkBuilder *builder;
          CtkWidget *help_overlay;

          builder = ctk_builder_new_from_resource (priv->help_overlay_path);
          help_overlay = CTK_WIDGET (ctk_builder_get_object (builder, "help_overlay"));
          if (CTK_IS_SHORTCUTS_WINDOW (help_overlay))
            ctk_application_window_set_help_overlay (CTK_APPLICATION_WINDOW (window),
                                                     CTK_SHORTCUTS_WINDOW (help_overlay));
          g_object_unref (builder);
        }
    }

  priv->windows = g_list_prepend (priv->windows, window);
  ctk_window_set_application (window, application);
  g_application_hold (G_APPLICATION (application));

  g_signal_connect (window, "focus-in-event",
                    G_CALLBACK (ctk_application_focus_in_event_cb),
                    application);

  ctk_application_impl_window_added (priv->impl, window);

  ctk_application_impl_active_window_changed (priv->impl, window);

  g_object_notify_by_pspec (G_OBJECT (application), ctk_application_props[PROP_ACTIVE_WINDOW]);
}

static void
ctk_application_window_removed (CtkApplication *application,
                                CtkWindow      *window)
{
  CtkApplicationPrivate *priv = application->priv;
  gpointer old_active;

  old_active = priv->windows;

  if (priv->impl)
    ctk_application_impl_window_removed (priv->impl, window);

  g_signal_handlers_disconnect_by_func (window,
                                        ctk_application_focus_in_event_cb,
                                        application);

  g_application_release (G_APPLICATION (application));
  priv->windows = g_list_remove (priv->windows, window);
  ctk_window_set_application (window, NULL);

  if (priv->windows != old_active && priv->impl)
    {
      ctk_application_impl_active_window_changed (priv->impl, priv->windows ? priv->windows->data : NULL);
      g_object_notify_by_pspec (G_OBJECT (application), ctk_application_props[PROP_ACTIVE_WINDOW]);
    }
}

static void
extract_accel_from_menu_item (GMenuModel     *model,
                              gint            item,
                              CtkApplication *app)
{
  GMenuAttributeIter *iter;
  const gchar *key;
  GVariant *value;
  const gchar *accel = NULL;
  const gchar *action = NULL;
  GVariant *target = NULL;

  iter = g_menu_model_iterate_item_attributes (model, item);
  while (g_menu_attribute_iter_get_next (iter, &key, &value))
    {
      if (g_str_equal (key, "action") && g_variant_is_of_type (value, G_VARIANT_TYPE_STRING))
        action = g_variant_get_string (value, NULL);
      else if (g_str_equal (key, "accel") && g_variant_is_of_type (value, G_VARIANT_TYPE_STRING))
        accel = g_variant_get_string (value, NULL);
      else if (g_str_equal (key, "target"))
        target = g_variant_ref (value);
      g_variant_unref (value);
    }
  g_object_unref (iter);

  if (accel && action)
    {
      const gchar *accels[2] = { accel, NULL };
      gchar *detailed_action_name;

      detailed_action_name = g_action_print_detailed_name (action, target);
      ctk_application_set_accels_for_action (app, detailed_action_name, accels);
      g_free (detailed_action_name);
    }

  if (target)
    g_variant_unref (target);
}

static void
extract_accels_from_menu (GMenuModel     *model,
                          CtkApplication *app)
{
  gint i;

  for (i = 0; i < g_menu_model_get_n_items (model); i++)
    {
      GMenuLinkIter *iter;
      GMenuModel *sub_model;

      extract_accel_from_menu_item (model, i, app);

      iter = g_menu_model_iterate_item_links (model, i);
      while (g_menu_link_iter_get_next (iter, NULL, &sub_model))
        {
          extract_accels_from_menu (sub_model, app);
          g_object_unref (sub_model);
        }
      g_object_unref (iter);
    }
}

static void
ctk_application_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  CtkApplication *application = CTK_APPLICATION (object);

  switch (prop_id)
    {
    case PROP_REGISTER_SESSION:
      g_value_set_boolean (value, application->priv->register_session);
      break;

    case PROP_SCREENSAVER_ACTIVE:
      g_value_set_boolean (value, application->priv->screensaver_active);
      break;

    case PROP_APP_MENU:
      g_value_set_object (value, ctk_application_get_app_menu (application));
      break;

    case PROP_MENUBAR:
      g_value_set_object (value, ctk_application_get_menubar (application));
      break;

    case PROP_ACTIVE_WINDOW:
      g_value_set_object (value, ctk_application_get_active_window (application));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_application_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  CtkApplication *application = CTK_APPLICATION (object);

  switch (prop_id)
    {
    case PROP_REGISTER_SESSION:
      application->priv->register_session = g_value_get_boolean (value);
      break;

    case PROP_APP_MENU:
      ctk_application_set_app_menu (application, g_value_get_object (value));
      break;

    case PROP_MENUBAR:
      ctk_application_set_menubar (application, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_application_finalize (GObject *object)
{
  CtkApplication *application = CTK_APPLICATION (object);

  g_clear_object (&application->priv->menus_builder);
  g_clear_object (&application->priv->app_menu);
  g_clear_object (&application->priv->menubar);
  g_clear_object (&application->priv->muxer);
  g_clear_object (&application->priv->accels);

  g_free (application->priv->help_overlay_path);

  G_OBJECT_CLASS (ctk_application_parent_class)->finalize (object);
}

#ifdef G_OS_UNIX

static const gchar org_gnome_Sysprof3_Profiler_xml[] =
  "<node>"
    "<interface name='org.gnome.Sysprof3.Profiler'>"
      "<property name='Capabilities' type='a{sv}' access='read'/>"
      "<method name='Start'>"
        "<arg type='a{sv}' name='options' direction='in'/>"
        "<arg type='h' name='fd' direction='in'/>"
      "</method>"
      "<method name='Stop'>"
      "</method>"
    "</interface>"
  "</node>";

static GDBusInterfaceInfo *org_gnome_Sysprof3_Profiler;

static void
sysprof_profiler_method_call (GDBusConnection       *connection G_GNUC_UNUSED,
                              const gchar           *sender G_GNUC_UNUSED,
                              const gchar           *object_path G_GNUC_UNUSED,
                              const gchar           *interface_name G_GNUC_UNUSED,
                              const gchar           *method_name,
                              GVariant              *parameters,
                              GDBusMethodInvocation *invocation,
                              gpointer               user_data G_GNUC_UNUSED)
{
  if (strcmp (method_name, "Start") == 0)
    {
      GDBusMessage *message;
      GUnixFDList *fd_list;
      GVariant *options;
      int fd = -1;
      int idx;

      if (CDK_PRIVATE_CALL (cdk_profiler_is_running) ())
        {
          g_dbus_method_invocation_return_error (invocation,
                                                 G_DBUS_ERROR,
                                                 G_DBUS_ERROR_FAILED,
                                                 "Profiler already running");
          return;
        }

      g_variant_get (parameters, "(@a{sv}h)", &options, &idx);

      message = g_dbus_method_invocation_get_message (invocation);
      fd_list = g_dbus_message_get_unix_fd_list (message);
      if (fd_list)
        fd = g_unix_fd_list_get (fd_list, idx, NULL);

      CDK_PRIVATE_CALL (cdk_profiler_start) (fd);

      g_variant_unref (options);
    }
  else if (strcmp (method_name, "Stop") == 0)
    {
      if (!CDK_PRIVATE_CALL (cdk_profiler_is_running) ())
        {
          g_dbus_method_invocation_return_error (invocation,
                                                 G_DBUS_ERROR,
                                                 G_DBUS_ERROR_FAILED,
                                                 "Profiler not running");
          return;
        }

      CDK_PRIVATE_CALL (cdk_profiler_stop) ();
    }
  else
    {
      g_dbus_method_invocation_return_error (invocation,
                                             G_DBUS_ERROR,
                                             G_DBUS_ERROR_UNKNOWN_METHOD,
                                             "Unknown method");
      return;
    }

  g_dbus_method_invocation_return_value (invocation, NULL);
}

static gboolean
ctk_application_dbus_register (GApplication     *application,
                               GDBusConnection  *connection,
                               const char       *obect_path G_GNUC_UNUSED,
                               GError          **error)
{
  CtkApplicationPrivate *priv = ctk_application_get_instance_private (CTK_APPLICATION (application));
  GDBusInterfaceVTable vtable = {
    .method_call = sysprof_profiler_method_call
  };

  if (org_gnome_Sysprof3_Profiler == NULL)
    {
      GDBusNodeInfo *info;

      info = g_dbus_node_info_new_for_xml (org_gnome_Sysprof3_Profiler_xml, error);
      if (info == NULL)
        return FALSE;

      org_gnome_Sysprof3_Profiler = g_dbus_node_info_lookup_interface (info, "org.gnome.Sysprof3.Profiler");
      g_dbus_interface_info_ref (org_gnome_Sysprof3_Profiler);
      g_dbus_node_info_unref (info);
    }

  priv->profiler_id = g_dbus_connection_register_object (connection,
                                                         "/org/ctk/Profiler",
                                                         org_gnome_Sysprof3_Profiler,
                                                         &vtable,
                                                         NULL,
                                                         NULL,
                                                         error);

  return TRUE;
}

static void
ctk_application_dbus_unregister (GApplication     *application,
                                 GDBusConnection  *connection,
                                 const char       *obect_path G_GNUC_UNUSED)
{
  CtkApplicationPrivate *priv = ctk_application_get_instance_private (CTK_APPLICATION (application));

  g_dbus_connection_unregister_object (connection, priv->profiler_id);
}

#else

static gboolean
ctk_application_dbus_register (GApplication     *application,
                               GDBusConnection  *connection,
                               const char       *obect_path,
                               GError          **error)
{
  return TRUE;
}

static void
ctk_application_dbus_unregister (GApplication     *application,
                                 GDBusConnection  *connection,
                                 const char       *obect_path)
{
}

#endif

static void
ctk_application_class_init (CtkApplicationClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GApplicationClass *application_class = G_APPLICATION_CLASS (class);

  object_class->get_property = ctk_application_get_property;
  object_class->set_property = ctk_application_set_property;
  object_class->finalize = ctk_application_finalize;

  application_class->local_command_line = ctk_application_local_command_line;
  application_class->add_platform_data = ctk_application_add_platform_data;
  application_class->before_emit = ctk_application_before_emit;
  application_class->after_emit = ctk_application_after_emit;
  application_class->startup = ctk_application_startup;
  application_class->shutdown = ctk_application_shutdown;
  application_class->dbus_register = ctk_application_dbus_register;
  application_class->dbus_unregister = ctk_application_dbus_unregister;

  class->window_added = ctk_application_window_added;
  class->window_removed = ctk_application_window_removed;

  /**
   * CtkApplication::window-added:
   * @application: the #CtkApplication which emitted the signal
   * @window: the newly-added #CtkWindow
   *
   * Emitted when a #CtkWindow is added to @application through
   * ctk_application_add_window().
   *
   * Since: 3.2
   */
  ctk_application_signals[WINDOW_ADDED] =
    g_signal_new (I_("window-added"), CTK_TYPE_APPLICATION, G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (CtkApplicationClass, window_added),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1, CTK_TYPE_WINDOW);

  /**
   * CtkApplication::window-removed:
   * @application: the #CtkApplication which emitted the signal
   * @window: the #CtkWindow that is being removed
   *
   * Emitted when a #CtkWindow is removed from @application,
   * either as a side-effect of being destroyed or explicitly
   * through ctk_application_remove_window().
   *
   * Since: 3.2
   */
  ctk_application_signals[WINDOW_REMOVED] =
    g_signal_new (I_("window-removed"), CTK_TYPE_APPLICATION, G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (CtkApplicationClass, window_removed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1, CTK_TYPE_WINDOW);

  /**
   * CtkApplication::query-end:
   * @application: the #CtkApplication which emitted the signal
   *
   * Emitted when the session manager is about to end the session, only
   * if #CtkApplication::register-session is %TRUE. Applications can
   * connect to this signal and call ctk_application_inhibit() with
   * %CTK_APPLICATION_INHIBIT_LOGOUT to delay the end of the session
   * until state has been saved.
   *
   * Since: 3.24.8
   */
  ctk_application_signals[QUERY_END] =
    g_signal_new (I_("query-end"), CTK_TYPE_APPLICATION, G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);
  /**
   * CtkApplication:register-session:
   *
   * Set this property to %TRUE to register with the session manager.
   *
   * Since: 3.4
   */
  ctk_application_props[PROP_REGISTER_SESSION] =
    g_param_spec_boolean ("register-session",
                          P_("Register session"),
                          P_("Register with the session manager"),
                          FALSE,
                          G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS);

  /**
   * CtkApplication:screensaver-active:
   *
   * This property is %TRUE if CTK+ believes that the screensaver is
   * currently active. CTK+ only tracks session state (including this)
   * when #CtkApplication::register-session is set to %TRUE.
   *
   * Tracking the screensaver state is supported on Linux.
   *
   * Since: 3.24
   */
  ctk_application_props[PROP_SCREENSAVER_ACTIVE] =
    g_param_spec_boolean ("screensaver-active",
                          P_("Screensaver Active"),
                          P_("Whether the screensaver is active"),
                          FALSE,
                          G_PARAM_READABLE|G_PARAM_STATIC_STRINGS);

  ctk_application_props[PROP_APP_MENU] =
    g_param_spec_object ("app-menu",
                         P_("Application menu"),
                         P_("The GMenuModel for the application menu"),
                         G_TYPE_MENU_MODEL,
                         G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS);

  ctk_application_props[PROP_MENUBAR] =
    g_param_spec_object ("menubar",
                         P_("Menubar"),
                         P_("The GMenuModel for the menubar"),
                         G_TYPE_MENU_MODEL,
                         G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS);

  ctk_application_props[PROP_ACTIVE_WINDOW] =
    g_param_spec_object ("active-window",
                         P_("Active window"),
                         P_("The window which most recently had focus"),
                         CTK_TYPE_WINDOW,
                         G_PARAM_READABLE|G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, ctk_application_props);
}

/**
 * ctk_application_new:
 * @application_id: (allow-none): The application ID.
 * @flags: the application flags
 *
 * Creates a new #CtkApplication instance.
 *
 * When using #CtkApplication, it is not necessary to call ctk_init()
 * manually. It is called as soon as the application gets registered as
 * the primary instance.
 *
 * Concretely, ctk_init() is called in the default handler for the
 * #GApplication::startup signal. Therefore, #CtkApplication subclasses should
 * chain up in their #GApplication::startup handler before using any CTK+ API.
 *
 * Note that commandline arguments are not passed to ctk_init().
 * All CTK+ functionality that is available via commandline arguments
 * can also be achieved by setting suitable environment variables
 * such as `G_DEBUG`, so this should not be a big
 * problem. If you absolutely must support CTK+ commandline arguments,
 * you can explicitly call ctk_init() before creating the application
 * instance.
 *
 * If non-%NULL, the application ID must be valid.  See
 * g_application_id_is_valid().
 *
 * If no application ID is given then some features (most notably application 
 * uniqueness) will be disabled. A null application ID is only allowed with 
 * CTK+ 3.6 or later.
 *
 * Returns: a new #CtkApplication instance
 *
 * Since: 3.0
 */
CtkApplication *
ctk_application_new (const gchar       *application_id,
                     GApplicationFlags  flags)
{
  g_return_val_if_fail (application_id == NULL || g_application_id_is_valid (application_id), NULL);

  return g_object_new (CTK_TYPE_APPLICATION,
                       "application-id", application_id,
                       "flags", flags,
                       NULL);
}

/**
 * ctk_application_add_window:
 * @application: a #CtkApplication
 * @window: a #CtkWindow
 *
 * Adds a window to @application.
 *
 * This call can only happen after the @application has started;
 * typically, you should add new application windows in response
 * to the emission of the #GApplication::activate signal.
 *
 * This call is equivalent to setting the #CtkWindow:application
 * property of @window to @application.
 *
 * Normally, the connection between the application and the window
 * will remain until the window is destroyed, but you can explicitly
 * remove it with ctk_application_remove_window().
 *
 * CTK+ will keep the @application running as long as it has
 * any windows.
 *
 * Since: 3.0
 **/
void
ctk_application_add_window (CtkApplication *application,
                            CtkWindow      *window)
{
  g_return_if_fail (CTK_IS_APPLICATION (application));
  g_return_if_fail (CTK_IS_WINDOW (window));

  if (!g_application_get_is_registered (G_APPLICATION (application)))
    {
      g_critical ("New application windows must be added after the "
                  "GApplication::startup signal has been emitted.");
      return;
    }

  if (!g_list_find (application->priv->windows, window))
    g_signal_emit (application,
                   ctk_application_signals[WINDOW_ADDED], 0, window);
}

/**
 * ctk_application_remove_window:
 * @application: a #CtkApplication
 * @window: a #CtkWindow
 *
 * Remove a window from @application.
 *
 * If @window belongs to @application then this call is equivalent to
 * setting the #CtkWindow:application property of @window to
 * %NULL.
 *
 * The application may stop running as a result of a call to this
 * function.
 *
 * Since: 3.0
 **/
void
ctk_application_remove_window (CtkApplication *application,
                               CtkWindow      *window)
{
  g_return_if_fail (CTK_IS_APPLICATION (application));
  g_return_if_fail (CTK_IS_WINDOW (window));

  if (g_list_find (application->priv->windows, window))
    g_signal_emit (application,
                   ctk_application_signals[WINDOW_REMOVED], 0, window);
}

/**
 * ctk_application_get_windows:
 * @application: a #CtkApplication
 *
 * Gets a list of the #CtkWindows associated with @application.
 *
 * The list is sorted by most recently focused window, such that the first
 * element is the currently focused window. (Useful for choosing a parent
 * for a transient window.)
 *
 * The list that is returned should not be modified in any way. It will
 * only remain valid until the next focus change or window creation or
 * deletion.
 *
 * Returns: (element-type CtkWindow) (transfer none): a #GList of #CtkWindow
 *
 * Since: 3.0
 **/
GList *
ctk_application_get_windows (CtkApplication *application)
{
  g_return_val_if_fail (CTK_IS_APPLICATION (application), NULL);

  return application->priv->windows;
}

/**
 * ctk_application_get_window_by_id:
 * @application: a #CtkApplication
 * @id: an identifier number
 *
 * Returns the #CtkApplicationWindow with the given ID.
 *
 * The ID of a #CtkApplicationWindow can be retrieved with
 * ctk_application_window_get_id().
 *
 * Returns: (nullable) (transfer none): the window with ID @id, or
 *   %NULL if there is no window with this ID
 *
 * Since: 3.6
 */
CtkWindow *
ctk_application_get_window_by_id (CtkApplication *application,
                                  guint           id)
{
  GList *l;

  g_return_val_if_fail (CTK_IS_APPLICATION (application), NULL);

  for (l = application->priv->windows; l != NULL; l = l->next) 
    {
      if (CTK_IS_APPLICATION_WINDOW (l->data) &&
          ctk_application_window_get_id (CTK_APPLICATION_WINDOW (l->data)) == id)
        return l->data;
    }

  return NULL;
}

/**
 * ctk_application_get_active_window:
 * @application: a #CtkApplication
 *
 * Gets the “active” window for the application.
 *
 * The active window is the one that was most recently focused (within
 * the application).  This window may not have the focus at the moment
 * if another application has it — this is just the most
 * recently-focused window within this application.
 *
 * Returns: (transfer none) (nullable): the active window, or %NULL if
 *   there isn't one.
 *
 * Since: 3.6
 **/
CtkWindow *
ctk_application_get_active_window (CtkApplication *application)
{
  g_return_val_if_fail (CTK_IS_APPLICATION (application), NULL);

  return application->priv->windows ? application->priv->windows->data : NULL;
}

static void
ctk_application_update_accels (CtkApplication *application)
{
  GList *l;

  for (l = application->priv->windows; l != NULL; l = l->next)
    _ctk_window_notify_keys_changed (l->data);
}

/**
 * ctk_application_add_accelerator:
 * @application: a #CtkApplication
 * @accelerator: accelerator string
 * @action_name: the name of the action to activate
 * @parameter: (allow-none): parameter to pass when activating the action,
 *   or %NULL if the action does not accept an activation parameter
 *
 * Installs an accelerator that will cause the named action
 * to be activated when the key combination specificed by @accelerator
 * is pressed.
 *
 * @accelerator must be a string that can be parsed by ctk_accelerator_parse(),
 * e.g. "<Primary>q" or “<Control><Alt>p”.
 *
 * @action_name must be the name of an action as it would be used
 * in the app menu, i.e. actions that have been added to the application
 * are referred to with an “app.” prefix, and window-specific actions
 * with a “win.” prefix.
 *
 * CtkApplication also extracts accelerators out of “accel” attributes
 * in the #GMenuModels passed to ctk_application_set_app_menu() and
 * ctk_application_set_menubar(), which is usually more convenient
 * than calling this function for each accelerator.
 *
 * Since: 3.4
 *
 * Deprecated: 3.14: Use ctk_application_set_accels_for_action() instead
 */
void
ctk_application_add_accelerator (CtkApplication *application,
                                 const gchar    *accelerator,
                                 const gchar    *action_name,
                                 GVariant       *parameter)
{
  const gchar *accelerators[2] = { accelerator, NULL };
  gchar *detailed_action_name;

  g_return_if_fail (CTK_IS_APPLICATION (application));
  g_return_if_fail (accelerator != NULL);
  g_return_if_fail (action_name != NULL);

  detailed_action_name = g_action_print_detailed_name (action_name, parameter);
  ctk_application_set_accels_for_action (application, detailed_action_name, accelerators);
  g_free (detailed_action_name);
}

/**
 * ctk_application_remove_accelerator:
 * @application: a #CtkApplication
 * @action_name: the name of the action to activate
 * @parameter: (allow-none): parameter to pass when activating the action,
 *   or %NULL if the action does not accept an activation parameter
 *
 * Removes an accelerator that has been previously added
 * with ctk_application_add_accelerator().
 *
 * Since: 3.4
 *
 * Deprecated: 3.14: Use ctk_application_set_accels_for_action() instead
 */
void
ctk_application_remove_accelerator (CtkApplication *application,
                                    const gchar    *action_name,
                                    GVariant       *parameter)
{
  const gchar *accelerators[1] = { NULL };
  gchar *detailed_action_name;

  g_return_if_fail (CTK_IS_APPLICATION (application));
  g_return_if_fail (action_name != NULL);

  detailed_action_name = g_action_print_detailed_name (action_name, parameter);
  ctk_application_set_accels_for_action (application, detailed_action_name, accelerators);
  g_free (detailed_action_name);
}

/**
 * ctk_application_prefers_app_menu:
 * @application: a #CtkApplication
 *
 * Determines if the desktop environment in which the application is
 * running would prefer an application menu be shown.
 *
 * If this function returns %TRUE then the application should call
 * ctk_application_set_app_menu() with the contents of an application
 * menu, which will be shown by the desktop environment.  If it returns
 * %FALSE then you should consider using an alternate approach, such as
 * a menubar.
 *
 * The value returned by this function is purely advisory and you are
 * free to ignore it.  If you call ctk_application_set_app_menu() even
 * if the desktop environment doesn't support app menus, then a fallback
 * will be provided.
 *
 * Applications are similarly free not to set an app menu even if the
 * desktop environment wants to show one.  In that case, a fallback will
 * also be created by the desktop environment (GNOME, for example, uses
 * a menu with only a "Quit" item in it).
 *
 * The value returned by this function never changes.  Once it returns a
 * particular value, it is guaranteed to always return the same value.
 *
 * You may only call this function after the application has been
 * registered and after the base startup handler has run.  You're most
 * likely to want to use this from your own startup handler.  It may
 * also make sense to consult this function while constructing UI (in
 * activate, open or an action activation handler) in order to determine
 * if you should show a gear menu or not.
 *
 * This function will return %FALSE on Mac OS and a default app menu
 * will be created automatically with the "usual" contents of that menu
 * typical to most Mac OS applications.  If you call
 * ctk_application_set_app_menu() anyway, then this menu will be
 * replaced with your own.
 *
 * Returns: %TRUE if you should set an app menu
 *
 * Since: 3.14
 **/
gboolean
ctk_application_prefers_app_menu (CtkApplication *application)
{
  g_return_val_if_fail (CTK_IS_APPLICATION (application), FALSE);
  g_return_val_if_fail (application->priv->impl != NULL, FALSE);

  return ctk_application_impl_prefers_app_menu (application->priv->impl);
}

/**
 * ctk_application_set_app_menu:
 * @application: a #CtkApplication
 * @app_menu: (allow-none): a #GMenuModel, or %NULL
 *
 * Sets or unsets the application menu for @application.
 *
 * This can only be done in the primary instance of the application,
 * after it has been registered.  #GApplication::startup is a good place
 * to call this.
 *
 * The application menu is a single menu containing items that typically
 * impact the application as a whole, rather than acting on a specific
 * window or document.  For example, you would expect to see
 * “Preferences” or “Quit” in an application menu, but not “Save” or
 * “Print”.
 *
 * If supported, the application menu will be rendered by the desktop
 * environment.
 *
 * Use the base #GActionMap interface to add actions, to respond to the user
 * selecting these menu items.
 *
 * Since: 3.4
 */
void
ctk_application_set_app_menu (CtkApplication *application,
                              GMenuModel     *app_menu)
{
  g_return_if_fail (CTK_IS_APPLICATION (application));
  g_return_if_fail (g_application_get_is_registered (G_APPLICATION (application)));
  g_return_if_fail (!g_application_get_is_remote (G_APPLICATION (application)));
  g_return_if_fail (app_menu == NULL || G_IS_MENU_MODEL (app_menu));

  if (g_set_object (&application->priv->app_menu, app_menu))
    {
      if (app_menu)
        extract_accels_from_menu (app_menu, application);

      ctk_application_impl_set_app_menu (application->priv->impl, app_menu);

      g_object_notify_by_pspec (G_OBJECT (application), ctk_application_props[PROP_APP_MENU]);
    }
}

/**
 * ctk_application_get_app_menu:
 * @application: a #CtkApplication
 *
 * Returns the menu model that has been set with
 * ctk_application_set_app_menu().
 *
 * Returns: (transfer none) (nullable): the application menu of @application
 *   or %NULL if no application menu has been set.
 *
 * Since: 3.4
 */
GMenuModel *
ctk_application_get_app_menu (CtkApplication *application)
{
  g_return_val_if_fail (CTK_IS_APPLICATION (application), NULL);

  return application->priv->app_menu;
}

/**
 * ctk_application_set_menubar:
 * @application: a #CtkApplication
 * @menubar: (allow-none): a #GMenuModel, or %NULL
 *
 * Sets or unsets the menubar for windows of @application.
 *
 * This is a menubar in the traditional sense.
 *
 * This can only be done in the primary instance of the application,
 * after it has been registered.  #GApplication::startup is a good place
 * to call this.
 *
 * Depending on the desktop environment, this may appear at the top of
 * each window, or at the top of the screen.  In some environments, if
 * both the application menu and the menubar are set, the application
 * menu will be presented as if it were the first item of the menubar.
 * Other environments treat the two as completely separate — for example,
 * the application menu may be rendered by the desktop shell while the
 * menubar (if set) remains in each individual window.
 *
 * Use the base #GActionMap interface to add actions, to respond to the
 * user selecting these menu items.
 *
 * Since: 3.4
 */
void
ctk_application_set_menubar (CtkApplication *application,
                             GMenuModel     *menubar)
{
  g_return_if_fail (CTK_IS_APPLICATION (application));
  g_return_if_fail (g_application_get_is_registered (G_APPLICATION (application)));
  g_return_if_fail (!g_application_get_is_remote (G_APPLICATION (application)));
  g_return_if_fail (menubar == NULL || G_IS_MENU_MODEL (menubar));

  if (g_set_object (&application->priv->menubar, menubar))
    {
      if (menubar)
        extract_accels_from_menu (menubar, application);

      ctk_application_impl_set_menubar (application->priv->impl, menubar);

      g_object_notify_by_pspec (G_OBJECT (application), ctk_application_props[PROP_MENUBAR]);
    }
}

/**
 * ctk_application_get_menubar:
 * @application: a #CtkApplication
 *
 * Returns the menu model that has been set with
 * ctk_application_set_menubar().
 *
 * Returns: (transfer none): the menubar for windows of @application
 *
 * Since: 3.4
 */
GMenuModel *
ctk_application_get_menubar (CtkApplication *application)
{
  g_return_val_if_fail (CTK_IS_APPLICATION (application), NULL);

  return application->priv->menubar;
}

/**
 * CtkApplicationInhibitFlags:
 * @CTK_APPLICATION_INHIBIT_LOGOUT: Inhibit ending the user session
 *     by logging out or by shutting down the computer
 * @CTK_APPLICATION_INHIBIT_SWITCH: Inhibit user switching
 * @CTK_APPLICATION_INHIBIT_SUSPEND: Inhibit suspending the
 *     session or computer
 * @CTK_APPLICATION_INHIBIT_IDLE: Inhibit the session being
 *     marked as idle (and possibly locked)
 *
 * Types of user actions that may be blocked by ctk_application_inhibit().
 *
 * Since: 3.4
 */

/**
 * ctk_application_inhibit:
 * @application: the #CtkApplication
 * @window: (allow-none): a #CtkWindow, or %NULL
 * @flags: what types of actions should be inhibited
 * @reason: (allow-none): a short, human-readable string that explains
 *     why these operations are inhibited
 *
 * Inform the session manager that certain types of actions should be
 * inhibited. This is not guaranteed to work on all platforms and for
 * all types of actions.
 *
 * Applications should invoke this method when they begin an operation
 * that should not be interrupted, such as creating a CD or DVD. The
 * types of actions that may be blocked are specified by the @flags
 * parameter. When the application completes the operation it should
 * call ctk_application_uninhibit() to remove the inhibitor. Note that
 * an application can have multiple inhibitors, and all of them must
 * be individually removed. Inhibitors are also cleared when the
 * application exits.
 *
 * Applications should not expect that they will always be able to block
 * the action. In most cases, users will be given the option to force
 * the action to take place.
 *
 * Reasons should be short and to the point.
 *
 * If @window is given, the session manager may point the user to
 * this window to find out more about why the action is inhibited.
 *
 * Returns: A non-zero cookie that is used to uniquely identify this
 *     request. It should be used as an argument to ctk_application_uninhibit()
 *     in order to remove the request. If the platform does not support
 *     inhibiting or the request failed for some reason, 0 is returned.
 *
 * Since: 3.4
 */
guint
ctk_application_inhibit (CtkApplication             *application,
                         CtkWindow                  *window,
                         CtkApplicationInhibitFlags  flags,
                         const gchar                *reason)
{
  g_return_val_if_fail (CTK_IS_APPLICATION (application), 0);
  g_return_val_if_fail (!g_application_get_is_remote (G_APPLICATION (application)), 0);
  g_return_val_if_fail (window == NULL || CTK_IS_WINDOW (window), 0);

  return ctk_application_impl_inhibit (application->priv->impl, window, flags, reason);
}

/**
 * ctk_application_uninhibit:
 * @application: the #CtkApplication
 * @cookie: a cookie that was returned by ctk_application_inhibit()
 *
 * Removes an inhibitor that has been established with ctk_application_inhibit().
 * Inhibitors are also cleared when the application exits.
 *
 * Since: 3.4
 */
void
ctk_application_uninhibit (CtkApplication *application,
                           guint           cookie)
{
  g_return_if_fail (CTK_IS_APPLICATION (application));
  g_return_if_fail (!g_application_get_is_remote (G_APPLICATION (application)));
  g_return_if_fail (cookie > 0);

  ctk_application_impl_uninhibit (application->priv->impl, cookie);
}

/**
 * ctk_application_is_inhibited:
 * @application: the #CtkApplication
 * @flags: what types of actions should be queried
 *
 * Determines if any of the actions specified in @flags are
 * currently inhibited (possibly by another application).
 *
 * Note that this information may not be available (for example
 * when the application is running in a sandbox).
 *
 * Returns: %TRUE if any of the actions specified in @flags are inhibited
 *
 * Since: 3.4
 */
gboolean
ctk_application_is_inhibited (CtkApplication             *application,
                              CtkApplicationInhibitFlags  flags)
{
  g_return_val_if_fail (CTK_IS_APPLICATION (application), FALSE);
  g_return_val_if_fail (!g_application_get_is_remote (G_APPLICATION (application)), FALSE);

  return ctk_application_impl_is_inhibited (application->priv->impl, flags);
}

CtkActionMuxer *
ctk_application_get_parent_muxer_for_window (CtkWindow *window)
{
  CtkApplication *application;

  application = ctk_window_get_application (window);

  if (!application)
    return NULL;

  return application->priv->muxer;
}

CtkApplicationAccels *
ctk_application_get_application_accels (CtkApplication *application)
{
  return application->priv->accels;
}

/**
 * ctk_application_list_action_descriptions:
 * @application: a #CtkApplication
 *
 * Lists the detailed action names which have associated accelerators.
 * See ctk_application_set_accels_for_action().
 *
 * Returns: (transfer full): a %NULL-terminated array of strings,
 *     free with g_strfreev() when done
 *
 * Since: 3.12
 */
gchar **
ctk_application_list_action_descriptions (CtkApplication *application)
{
  g_return_val_if_fail (CTK_IS_APPLICATION (application), NULL);

  return ctk_application_accels_list_action_descriptions (application->priv->accels);
}

/**
 * ctk_application_set_accels_for_action:
 * @application: a #CtkApplication
 * @detailed_action_name: a detailed action name, specifying an action
 *     and target to associate accelerators with
 * @accels: (array zero-terminated=1): a list of accelerators in the format
 *     understood by ctk_accelerator_parse()
 *
 * Sets zero or more keyboard accelerators that will trigger the
 * given action. The first item in @accels will be the primary
 * accelerator, which may be displayed in the UI.
 *
 * To remove all accelerators for an action, use an empty, zero-terminated
 * array for @accels.
 *
 * For the @detailed_action_name, see g_action_parse_detailed_name() and
 * g_action_print_detailed_name().
 *
 * Since: 3.12
 */
void
ctk_application_set_accels_for_action (CtkApplication      *application,
                                       const gchar         *detailed_action_name,
                                       const gchar * const *accels)
{
  gchar *action_and_target;

  g_return_if_fail (CTK_IS_APPLICATION (application));
  g_return_if_fail (detailed_action_name != NULL);
  g_return_if_fail (accels != NULL);

  ctk_application_accels_set_accels_for_action (application->priv->accels,
                                                detailed_action_name,
                                                accels);

  action_and_target = ctk_normalise_detailed_action_name (detailed_action_name);
  ctk_action_muxer_set_primary_accel (application->priv->muxer, action_and_target, accels[0]);
  g_free (action_and_target);

  ctk_application_update_accels (application);
}

/**
 * ctk_application_get_accels_for_action:
 * @application: a #CtkApplication
 * @detailed_action_name: a detailed action name, specifying an action
 *     and target to obtain accelerators for
 *
 * Gets the accelerators that are currently associated with
 * the given action.
 *
 * Returns: (transfer full): accelerators for @detailed_action_name, as
 *     a %NULL-terminated array. Free with g_strfreev() when no longer needed
 *
 * Since: 3.12
 */
gchar **
ctk_application_get_accels_for_action (CtkApplication *application,
                                       const gchar    *detailed_action_name)
{
  g_return_val_if_fail (CTK_IS_APPLICATION (application), NULL);
  g_return_val_if_fail (detailed_action_name != NULL, NULL);

  return ctk_application_accels_get_accels_for_action (application->priv->accels,
                                                       detailed_action_name);
}

/**
 * ctk_application_get_actions_for_accel:
 * @application: a #CtkApplication
 * @accel: an accelerator that can be parsed by ctk_accelerator_parse()
 *
 * Returns the list of actions (possibly empty) that @accel maps to.
 * Each item in the list is a detailed action name in the usual form.
 *
 * This might be useful to discover if an accel already exists in
 * order to prevent installation of a conflicting accelerator (from
 * an accelerator editor or a plugin system, for example). Note that
 * having more than one action per accelerator may not be a bad thing
 * and might make sense in cases where the actions never appear in the
 * same context.
 *
 * In case there are no actions for a given accelerator, an empty array
 * is returned.  %NULL is never returned.
 *
 * It is a programmer error to pass an invalid accelerator string.
 * If you are unsure, check it with ctk_accelerator_parse() first.
 *
 * Returns: (transfer full): a %NULL-terminated array of actions for @accel
 *
 * Since: 3.14
 */
gchar **
ctk_application_get_actions_for_accel (CtkApplication *application,
                                       const gchar    *accel)
{
  g_return_val_if_fail (CTK_IS_APPLICATION (application), NULL);
  g_return_val_if_fail (accel != NULL, NULL);

  return ctk_application_accels_get_actions_for_accel (application->priv->accels, accel);
}

CtkActionMuxer *
ctk_application_get_action_muxer (CtkApplication *application)
{
  g_assert (application->priv->muxer);

  return application->priv->muxer;
}

void
ctk_application_insert_action_group (CtkApplication *application,
                                     const gchar    *name,
                                     GActionGroup   *action_group)
{
  ctk_action_muxer_insert (application->priv->muxer, name, action_group);
}

void
ctk_application_handle_window_realize (CtkApplication *application,
                                       CtkWindow      *window)
{
  if (application->priv->impl)
    ctk_application_impl_handle_window_realize (application->priv->impl, window);
}

void
ctk_application_handle_window_map (CtkApplication *application,
                                   CtkWindow      *window)
{
  if (application->priv->impl)
    ctk_application_impl_handle_window_map (application->priv->impl, window);
}

/**
 * ctk_application_get_menu_by_id:
 * @application: a #CtkApplication
 * @id: the id of the menu to look up
 *
 * Gets a menu from automatically loaded resources.
 * See [Automatic resources][automatic-resources]
 * for more information.
 *
 * Returns: (transfer none): Gets the menu with the
 *     given id from the automatically loaded resources
 *
 * Since: 3.14
 */
GMenu *
ctk_application_get_menu_by_id (CtkApplication *application,
                                const gchar    *id)
{
  GObject *object;

  g_return_val_if_fail (CTK_IS_APPLICATION (application), NULL);
  g_return_val_if_fail (id != NULL, NULL);

  if (!application->priv->menus_builder)
    return NULL;

  object = ctk_builder_get_object (application->priv->menus_builder, id);

  if (!object || !G_IS_MENU (object))
    return NULL;

  return G_MENU (object);
}

void
ctk_application_set_screensaver_active (CtkApplication *application,
                                        gboolean        active)
{
  CtkApplicationPrivate *priv = ctk_application_get_instance_private (application);

  if (priv->screensaver_active != active)
    {
      priv->screensaver_active = active;
      g_object_notify (G_OBJECT (application), "screensaver-active");
    }
}
