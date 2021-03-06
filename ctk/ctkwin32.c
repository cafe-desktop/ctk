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

#include "cdk/cdk.h"

#include "ctkprivate.h"

#define STRICT
#include <windows.h>
#include <commctrl.h>
#undef STRICT

/* In practice, resulting DLL will have manifest resource under index 2.
 * Fall back to that value if we can't find resource index programmatically.
 */
#define EMPIRIC_MANIFEST_RESOURCE_INDEX 2


static HMODULE ctk_dll;

BOOL WINAPI
DllMain (HINSTANCE hinstDLL,
         DWORD     fdwReason,
         LPVOID    lpvReserved)
{
  switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
      ctk_dll = (HMODULE) hinstDLL;
      break;
    }

  return TRUE;
}

static BOOL CALLBACK
find_first_manifest (HMODULE  module_handle,
                     LPCSTR   resource_type,
                     LPSTR    resource_name,
                     LONG_PTR user_data)
{
  LPSTR *result_name = (LPSTR *) user_data;

  if (resource_type == RT_MANIFEST)
    {
      if (IS_INTRESOURCE (resource_name))
        *result_name = resource_name;
      else
        *result_name = g_strdup (resource_name);
      return FALSE;
    }
  return TRUE;
}

/*
 * Grabs the first manifest it finds in libctk3 (which is expected to be the
 * common-controls-6.0.0.0 manifest we embedded to enable visual styles),
 * uses it to create a process-default activation context, activates that
 * context, loads up the library passed in @dllname, then deactivates and
 * releases the context.
 *
 * In practice this is used to force system DLLs (like comdlg32) to be
 * loaded as if the application had the same manifest as libctk3
 * (otherwise libctk3 manifest only affests libctk3 itself).
 * This way application does not need to have a manifest or to link
 * against comctl32.
 *
 * Note that loaded library handle leaks, so only use this function in
 * g_once_init_enter (leaking once is OK, Windows will clean up after us).
 */
void
_ctk_load_dll_with_libctk3_manifest (const gchar *dll_name)
{
  HANDLE activation_ctx_handle;
  ACTCTXA activation_ctx_descriptor;
  ULONG_PTR activation_cookie;
  LPSTR resource_name;
  BOOL activated;
  DWORD error_code;

  resource_name = NULL;
  EnumResourceNames (ctk_dll, RT_MANIFEST, find_first_manifest,
                     (LONG_PTR) &resource_name);

  if (resource_name == NULL)
    resource_name = MAKEINTRESOURCEA (EMPIRIC_MANIFEST_RESOURCE_INDEX);

  memset (&activation_ctx_descriptor, 0, sizeof (activation_ctx_descriptor));
  activation_ctx_descriptor.cbSize = sizeof (activation_ctx_descriptor);
  activation_ctx_descriptor.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID |
                                      ACTCTX_FLAG_HMODULE_VALID |
                                      ACTCTX_FLAG_SET_PROCESS_DEFAULT;
  activation_ctx_descriptor.hModule = ctk_dll;
  activation_ctx_descriptor.lpResourceName = resource_name;
  activation_ctx_handle = CreateActCtx (&activation_ctx_descriptor);
  error_code = GetLastError ();

  if (activation_ctx_handle == INVALID_HANDLE_VALUE &&
      error_code != ERROR_SXS_PROCESS_DEFAULT_ALREADY_SET)
    g_warning ("Failed to CreateActCtx for module %p, resource %p: %lu",
               ctk_dll, resource_name, GetLastError ());
  else if (error_code != ERROR_SXS_PROCESS_DEFAULT_ALREADY_SET)
    {
      activation_cookie = 0;
      activated = ActivateActCtx (activation_ctx_handle, &activation_cookie);

      if (!activated)
        g_warning ("Failed to ActivateActCtx: %lu", GetLastError ());

      LoadLibraryA (dll_name);

      if (activated && !DeactivateActCtx (0, activation_cookie))
        g_warning ("Failed to DeactivateActCtx: %lu", GetLastError ());

      ReleaseActCtx (activation_ctx_handle);
    }

  if (!IS_INTRESOURCE (resource_name))
    g_free (resource_name);
}

const gchar *
_ctk_get_libdir (void)
{
  static char *ctk_libdir = NULL;
  if (ctk_libdir == NULL)
    {
      gchar *root = g_win32_get_package_installation_directory_of_module (ctk_dll);
      gchar *slash = strrchr (root, '\\');
      if (slash != NULL &&
          g_ascii_strcasecmp (slash + 1, ".libs") == 0)
        ctk_libdir = CTK_LIBDIR;
      else
        ctk_libdir = g_build_filename (root, "lib", NULL);
      g_free (root);
    }

  return ctk_libdir;
}

const gchar *
_ctk_get_localedir (void)
{
  static char *ctk_localedir = NULL;
  if (ctk_localedir == NULL)
    {
      const gchar *p;
      gchar *root, *temp;

      /* CTK_LOCALEDIR ends in either /lib/locale or
       * /share/locale. Scan for that slash.
       */
      p = CTK_LOCALEDIR + strlen (CTK_LOCALEDIR);
      while (*--p != '/')
        ;
      while (*--p != '/')
        ;

      root = g_win32_get_package_installation_directory_of_module (ctk_dll);
      temp = g_build_filename (root, p, NULL);
      g_free (root);

      /* ctk_localedir is passed to bindtextdomain() which isn't
       * UTF-8-aware.
       */
      ctk_localedir = g_win32_locale_filename_from_utf8 (temp);
      g_free (temp);
    }
  return ctk_localedir;
}

const gchar *
_ctk_get_datadir (void)
{
  static char *ctk_datadir = NULL;
  if (ctk_datadir == NULL)
    {
      gchar *root = g_win32_get_package_installation_directory_of_module (ctk_dll);
      ctk_datadir = g_build_filename (root, "share", NULL);
      g_free (root);
    }

  return ctk_datadir;
}

const gchar *
_ctk_get_sysconfdir (void)
{
  static char *ctk_sysconfdir = NULL;
  if (ctk_sysconfdir == NULL)
    {
      gchar *root = g_win32_get_package_installation_directory_of_module (ctk_dll);
      ctk_sysconfdir = g_build_filename (root, "etc", NULL);
      g_free (root);
    }

  return ctk_sysconfdir;
}

const gchar *
_ctk_get_data_prefix (void)
{
  static char *ctk_data_prefix = NULL;
  if (ctk_data_prefix == NULL)
    ctk_data_prefix = g_win32_get_package_installation_directory_of_module (ctk_dll);

  return ctk_data_prefix;
}
