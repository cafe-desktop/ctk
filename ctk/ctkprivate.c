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

#include "config.h"

#include <locale.h>
#include <stdlib.h>

#include "cdk/cdk.h"

#include "ctkprivate.h"
#include "ctkresources.h"


#if !defined G_OS_WIN32 && !(defined CDK_WINDOWING_QUARTZ && defined QUARTZ_RELOCATION)

const gchar *
_ctk_get_datadir (void)
{
  return CTK_DATADIR;
}

const gchar *
_ctk_get_libdir (void)
{
  return CTK_LIBDIR;
}

const gchar *
_ctk_get_sysconfdir (void)
{
  return CTK_SYSCONFDIR;
}

const gchar *
_ctk_get_localedir (void)
{
  return CTK_LOCALEDIR;
}

const gchar *
_ctk_get_data_prefix (void)
{
  return CTK_DATA_PREFIX;
}

#endif

/* _ctk_get_lc_ctype:
 *
 * Return the Unix-style locale string for the language currently in
 * effect. On Unix systems, this is the return value from
 * `setlocale(LC_CTYPE, NULL)`, and the user can
 * affect this through the environment variables LC_ALL, LC_CTYPE or
 * LANG (checked in that order). The locale strings typically is in
 * the form lang_COUNTRY, where lang is an ISO-639 language code, and
 * COUNTRY is an ISO-3166 country code. For instance, sv_FI for
 * Swedish as written in Finland or pt_BR for Portuguese as written in
 * Brazil.
 *
 * On Windows, the C library doesn’t use any such environment
 * variables, and setting them won’t affect the behaviour of functions
 * like ctime(). The user sets the locale through the Regional Options
 * in the Control Panel. The C library (in the setlocale() function)
 * does not use country and language codes, but country and language
 * names spelled out in English.
 * However, this function does check the above environment
 * variables, and does return a Unix-style locale string based on
 * either said environment variables or the thread’s current locale.
 *
 * Returns: a dynamically allocated string, free with g_free().
 */

gchar *
_ctk_get_lc_ctype (void)
{
#ifdef G_OS_WIN32
  /* Somebody might try to set the locale for this process using the
   * LANG or LC_ environment variables. The Microsoft C library
   * doesn't know anything about them. You set the locale in the
   * Control Panel. Setting these env vars won't have any affect on
   * locale-dependent C library functions like ctime(). But just for
   * kicks, do obey LC_ALL, LC_CTYPE and LANG in CTK. (This also makes
   * it easier to test CTK and Pango in various default languages, you
   * don't have to clickety-click in the Control Panel, you can simply
   * start the program with LC_ALL=something on the command line.)
   */
  gchar *p;

  p = getenv ("LC_ALL");
  if (p != NULL)
    return g_strdup (p);

  p = getenv ("LC_CTYPE");
  if (p != NULL)
    return g_strdup (p);

  p = getenv ("LANG");
  if (p != NULL)
    return g_strdup (p);

  return g_win32_getlocale ();
#else
  return g_strdup (setlocale (LC_CTYPE, NULL));
#endif
}

gboolean
_ctk_boolean_handled_accumulator (GSignalInvocationHint *ihint G_GNUC_UNUSED,
                                  GValue                *return_accu,
                                  const GValue          *handler_return,
                                  gpointer               dummy G_GNUC_UNUSED)
{
  gboolean continue_emission;
  gboolean signal_handled;

  signal_handled = g_value_get_boolean (handler_return);
  g_value_set_boolean (return_accu, signal_handled);
  continue_emission = !signal_handled;

  return continue_emission;
}

gboolean
_ctk_single_string_accumulator (GSignalInvocationHint *ihint G_GNUC_UNUSED,
				GValue                *return_accu,
				const GValue          *handler_return,
				gpointer               dummy G_GNUC_UNUSED)
{
  gboolean continue_emission;
  const gchar *str;

  str = g_value_get_string (handler_return);
  g_value_set_string (return_accu, str);
  continue_emission = str == NULL;

  return continue_emission;
}

CdkModifierType
_ctk_replace_virtual_modifiers (CdkKeymap       *keymap,
                                CdkModifierType  modifiers)
{
  CdkModifierType result = 0;
  gint            i;

  g_return_val_if_fail (CDK_IS_KEYMAP (keymap), 0);

  for (i = 0; i < 8; i++) /* SHIFT...MOD5 */
    {
      CdkModifierType real = 1 << i;

      if (modifiers & real)
        {
          CdkModifierType virtual = real;

          cdk_keymap_add_virtual_modifiers (keymap, &virtual);

          if (virtual == real)
            result |= virtual;
          else
            result |= virtual & ~real;
        }
    }

  return result;
}

CdkModifierType
_ctk_get_primary_accel_mod (void)
{
  static CdkModifierType primary = 0;

  if (! primary)
    {
      CdkDisplay *display = cdk_display_get_default ();

      primary = cdk_keymap_get_modifier_mask (cdk_keymap_get_for_display (display),
                                              CDK_MODIFIER_INTENT_PRIMARY_ACCELERATOR);
      primary = _ctk_replace_virtual_modifiers (cdk_keymap_get_for_display (display),
                                                primary);
    }

  return primary;
}

gboolean
_ctk_translate_keyboard_accel_state (CdkKeymap       *keymap,
                                     guint            hardware_keycode,
                                     CdkModifierType  state,
                                     CdkModifierType  accel_mask,
                                     gint             group,
                                     guint           *keyval,
                                     gint            *effective_group,
                                     gint            *level,
                                     CdkModifierType *consumed_modifiers)
{
  CdkModifierType shift_group_mask;
  gboolean group_mask_disabled = FALSE;
  gboolean retval;

  /* if the group-toggling modifier is part of the accel mod mask, and
   * it is active, disable it for matching
   */
  shift_group_mask = cdk_keymap_get_modifier_mask (keymap,
                                                   CDK_MODIFIER_INTENT_SHIFT_GROUP);
  if (accel_mask & state & shift_group_mask)
    {
      state &= ~shift_group_mask;
      group = 0;
      group_mask_disabled = TRUE;
    }

  retval = cdk_keymap_translate_keyboard_state (keymap,
                                                hardware_keycode, state, group,
                                                keyval,
                                                effective_group, level,
                                                consumed_modifiers);

  /* add back the group mask, we want to match against the modifier,
   * but not against the keyval from its group
   */
  if (group_mask_disabled)
    {
      if (effective_group)
        *effective_group = 1;

      if (consumed_modifiers)
        *consumed_modifiers &= ~shift_group_mask;
    }

  return retval;
}

static gpointer
register_resources (gpointer data G_GNUC_UNUSED)
{
  _ctk_register_resource ();
  return NULL;
}

void
_ctk_ensure_resources (void)
{
  static GOnce register_resources_once = G_ONCE_INIT;

  g_once (&register_resources_once, register_resources, NULL);
}

gboolean
ctk_should_use_portal (void)
{
  static const char *use_portal = NULL;

  if (G_UNLIKELY (use_portal == NULL))
    {
      char *path;

      path = g_build_filename (g_get_user_runtime_dir (), "flatpak-info", NULL);
      if (g_file_test (path, G_FILE_TEST_EXISTS))
        use_portal = "1";
      else
        {
          use_portal = g_getenv ("CTK_USE_PORTAL");
          if (!use_portal)
            use_portal = "";
        }
      g_free (path);
    }

  return use_portal[0] == '1';
}

/*
 * ctk_get_portal_interface_version:
 * @connection: a session #GDBusConnection
 * @interface_name: the interface name for the portal interface
 *   we're interested in.
 *
 * Returns: the version number of the portal, or 0 on error.
 */
guint
ctk_get_portal_interface_version (GDBusConnection *connection,
                                  const char      *interface_name)
{
  GDBusProxy *proxy = NULL;
  GError *error = NULL;
  GVariant *ret = NULL;
  char *owner = NULL;
  guint version = 0;

  proxy = g_dbus_proxy_new_sync (connection,
                                 0,
                                 NULL,
                                 "org.freedesktop.portal.Desktop",
                                 "/org/freedesktop/portal/desktop",
                                 interface_name,
                                 NULL,
                                 &error);
  if (!proxy)
    {
      if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        g_warning ("Could not query portal version on interface '%s': %s",
                   interface_name, error->message);
      goto out;
    }

  owner = g_dbus_proxy_get_name_owner (proxy);
  if (owner == NULL)
    {
      g_debug ("%s not provided by any service", interface_name);
      goto out;
    }

  ret = g_dbus_proxy_get_cached_property (proxy, "version");
  if (ret)
    version = g_variant_get_uint32 (ret);

  g_debug ("Got version %u for portal interface '%s'",
           version, interface_name);

out:
  g_clear_object (&proxy);
  g_clear_error (&error);
  g_clear_pointer (&ret, g_variant_unref);
  g_clear_pointer (&owner, g_free);

  return version;
}

static char *
get_portal_path (GDBusConnection  *connection,
                 const char       *kind,
                 char            **token)
{
  char *sender;
  int i;
  char *path;

  *token = g_strdup_printf ("ctk%d", g_random_int_range (0, G_MAXINT));
  sender = g_strdup (g_dbus_connection_get_unique_name (connection) + 1);
  for (i = 0; sender[i]; i++)
    if (sender[i] == '.')
      sender[i] = '_';

  path = g_strconcat ("/org/freedesktop/portal/desktop", "/", kind, "/", sender, "/", *token, NULL);

  g_free (sender);

  return path;
}

char *
ctk_get_portal_request_path (GDBusConnection  *connection,
                             char            **token)
{
   return get_portal_path (connection, "request", token);
}

char *
ctk_get_portal_session_path (GDBusConnection  *connection,
                             char            **token)
{
   return get_portal_path (connection, "session", token);
}
