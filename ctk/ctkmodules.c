/* CTK - The GIMP Toolkit
 * Copyright 1998-2002 Tim Janik, Red Hat, Inc., and others.
 * Copyright (C) 2003 Alex Graveley
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

#include "ctkmodules.h"
#include "ctksettings.h"
#include "ctkdebug.h"
#include "ctkprivate.h"
#include "ctkmodulesprivate.h"
#include "ctkintl.h"
#include "ctkutilsprivate.h"

#include <gmodule.h>

typedef struct _CtkModuleInfo CtkModuleInfo;
struct _CtkModuleInfo
{
  GModule                 *module;
  gint                     ref_count;
  CtkModuleInitFunc        init_func;
  CtkModuleDisplayInitFunc display_init_func;
  GSList                  *names;
};

static GSList *ctk_modules = NULL;

static gboolean default_display_opened = FALSE;

/* Saved argc, argv for delayed module initialization
 */
static gint    ctk_argc = 0;
static gchar **ctk_argv = NULL;

static gchar **
get_module_path (void)
{
  const gchar *module_path_env;
  const gchar *exe_prefix;
  gchar *module_path;
  gchar *default_dir;
  static gchar **result = NULL;

  if (result)
    return result;

  module_path_env = g_getenv ("CTK_PATH");
  exe_prefix = g_getenv ("CTK_EXE_PREFIX");

  if (exe_prefix)
    default_dir = g_build_filename (exe_prefix, "lib", "ctk-3.0", NULL);
  else
    default_dir = g_build_filename (_ctk_get_libdir (), "ctk-3.0", NULL);

  if (module_path_env)
    module_path = g_build_path (G_SEARCHPATH_SEPARATOR_S,
				module_path_env, default_dir, NULL);
  else
    module_path = g_build_path (G_SEARCHPATH_SEPARATOR_S,
				default_dir, NULL);

  g_free (default_dir);

  result = ctk_split_file_list (module_path);
  g_free (module_path);

  return result;
}

/**
 * _ctk_get_module_path:
 * @type: the type of the module, for instance 'modules', 'engines', immodules'
 * 
 * Determines the search path for a particular type of module.
 * 
 * Returns: the search path for the module type. Free with g_strfreev().
 **/
gchar **
_ctk_get_module_path (const gchar *type)
{
  gchar **paths = get_module_path();
  gchar **path;
  gchar **result;
  gint count = 0;

  for (path = paths; *path; path++)
    count++;

  result = g_new (gchar *, count * 4 + 1);

  count = 0;
  for (path = get_module_path (); *path; path++)
    {
      gint use_version, use_host;
      
      for (use_version = TRUE; use_version >= FALSE; use_version--)
	for (use_host = TRUE; use_host >= FALSE; use_host--)
	  {
	    gchar *tmp_dir;
	    
	    if (use_version && use_host)
	      tmp_dir = g_build_filename (*path, CTK_BINARY_VERSION, CTK_HOST, type, NULL);
	    else if (use_version)
	      tmp_dir = g_build_filename (*path, CTK_BINARY_VERSION, type, NULL);
	    else if (use_host)
	      tmp_dir = g_build_filename (*path, CTK_HOST, type, NULL);
	    else
	      tmp_dir = g_build_filename (*path, type, NULL);

	    result[count++] = tmp_dir;
	  }
    }

  result[count++] = NULL;

  return result;
}

/* Like g_module_path, but use .la as the suffix
 */
static gchar*
module_build_la_path (const gchar *directory,
		      const gchar *module_name)
{
  gchar *filename;
  gchar *result;
	
  if (strncmp (module_name, "lib", 3) == 0)
    filename = (gchar *)module_name;
  else
    filename =  g_strconcat ("lib", module_name, ".la", NULL);

  if (directory && *directory)
    result = g_build_filename (directory, filename, NULL);
  else
    result = g_strdup (filename);

  if (filename != module_name)
    g_free (filename);

  return result;
}

/**
 * _ctk_find_module:
 * @name: the name of the module
 * @type: the type of the module, for instance 'modules', 'engines', immodules'
 * 
 * Looks for a dynamically module named @name of type @type in the standard CTK+
 *  module search path.
 * 
 * Returns: the pathname to the found module, or %NULL if it wasn’t found.
 *  Free with g_free().
 **/
gchar *
_ctk_find_module (const gchar *name,
		  const gchar *type)
{
  gchar **paths;
  gchar **path;
  gchar *module_name = NULL;

  if (g_path_is_absolute (name))
    return g_strdup (name);

  paths = _ctk_get_module_path (type);
  for (path = paths; *path; path++)
    {
      gchar *tmp_name;

      tmp_name = g_module_build_path (*path, name);
      if (g_file_test (tmp_name, G_FILE_TEST_EXISTS))
	{
	  module_name = tmp_name;
	  goto found;
	}
      g_free(tmp_name);

      tmp_name = module_build_la_path (*path, name);
      if (g_file_test (tmp_name, G_FILE_TEST_EXISTS))
	{
	  module_name = tmp_name;
	  goto found;
	}
      g_free(tmp_name);
    }

 found:
  g_strfreev (paths);
  return module_name;
}

static GModule *
find_module (const gchar *name)
{
  GModule *module;
  gchar *module_name;

  module_name = _ctk_find_module (name, "modules");
  if (!module_name)
    {
      /* As last resort, try loading without an absolute path (using system
       * library path)
       */
      module_name = g_module_build_path (NULL, name);
    }

  module = g_module_open (module_name, G_MODULE_BIND_LOCAL | G_MODULE_BIND_LAZY);

  if (_ctk_module_has_mixed_deps (module))
    {
      g_warning ("CTK+ module %s cannot be loaded.\n"
                 "CTK+ 2.x symbols detected. Using CTK+ 2.x and CTK+ 3 in the same process is not supported.", module_name);
      g_module_close (module);
      module = NULL;
    }

  g_free (module_name);

  return module;
}

static gint
cmp_module (CtkModuleInfo *info,
	    GModule       *module)
{
  return info->module != module;
}

static gboolean
module_is_blacklisted (const gchar *name,
                       gboolean     verbose)
{
  if (g_str_equal (name, "cail") ||
      g_str_equal (name, "atk-bridge"))
    {
      if (verbose)
        g_message ("Not loading module \"%s\": The functionality is provided by CTK natively. Please try to not load it.", name);

      return TRUE;
    }

  return FALSE;
}

static GSList *
load_module (GSList      *module_list,
	     const gchar *name)
{
  CtkModuleInitFunc modinit_func;
  gpointer modinit_func_ptr;
  CtkModuleInfo *info = NULL;
  GModule *module = NULL;
  GSList *l;
  gboolean success = FALSE;
  
  if (g_module_supported ())
    {
      for (l = ctk_modules; l; l = l->next)
	{
	  info = l->data;
	  if (g_slist_find_custom (info->names, name,
				   (GCompareFunc)strcmp))
	    {
	      info->ref_count++;
	      
	      success = TRUE;
              break;
	    }
          info = NULL;
	}

      if (!success)
	{
	  module = find_module (name);

	  if (module)
	    {
              /* Do the check this late so we only warn about existing modules,
               * not old modules that are still in the modules path. */
              if (module_is_blacklisted (name, TRUE))
                {
                  modinit_func = NULL;
                  success = TRUE;
                }
              else if (g_module_symbol (module, "ctk_module_init", &modinit_func_ptr))
		modinit_func = modinit_func_ptr;
	      else
		modinit_func = NULL;

	      if (!modinit_func)
		g_module_close (module);
	      else
		{
		  GSList *temp;

		  success = TRUE;
		  info = NULL;

		  temp = g_slist_find_custom (ctk_modules, module,
			(GCompareFunc)cmp_module);
		  if (temp != NULL)
			info = temp->data;

		  if (!info)
		    {
		      info = g_new0 (CtkModuleInfo, 1);
		      
		      info->names = g_slist_prepend (info->names, g_strdup (name));
		      info->module = module;
		      info->ref_count = 1;
		      info->init_func = modinit_func;
		      g_module_symbol (module, "ctk_module_display_init",
				       (gpointer *) &info->display_init_func);
		      
		      ctk_modules = g_slist_append (ctk_modules, info);
		      
		      /* display_init == NULL indicates a non-multihead aware module.
		       * For these, we delay the call to init_func until first display is
		       * opened, see default_display_notify_cb().
		       * For multihead aware modules, we call init_func immediately,
		       * and also call display_init_func on all opened displays.
		       */
		      if (default_display_opened || info->display_init_func)
			(* info->init_func) (&ctk_argc, &ctk_argv);
		      
		      if (info->display_init_func) 
			{
			  GSList *displays, *iter; 		  
			  displays = cdk_display_manager_list_displays (cdk_display_manager_get ());
			  for (iter = displays; iter; iter = iter->next)
			    {
			      CdkDisplay *display = iter->data;
			  (* info->display_init_func) (display);
			    }
			  g_slist_free (displays);
			}
		    }
		  else
		    {
		      CTK_NOTE (MODULES, g_message ("Module already loaded, ignoring: %s", name));
		      info->names = g_slist_prepend (info->names, g_strdup (name));
		      info->ref_count++;
		      /* remove new reference count on module, we already have one */
		      g_module_close (module);
		    }
		}
	    }
	}
    }

  if (success && info)
    {
      if (!g_slist_find (module_list, info))
	{
	  module_list = g_slist_prepend (module_list, info);
	}
    }
  else
    {
      if (!module_is_blacklisted (name, FALSE))
        {
          const gchar *error = g_module_error ();

          g_message ("Failed to load module \"%s\"%s%s",
                     name, error ? ": " : "", error ? error : "");
        }
    }

  return module_list;
}


static void
ctk_module_info_unref (CtkModuleInfo *info)
{
  GSList *l;

  info->ref_count--;

  if (info->ref_count == 0)
    {
      CTK_NOTE (MODULES,
		g_message ("Unloading module: %s", g_module_name (info->module)));

      ctk_modules = g_slist_remove (ctk_modules, info);
      g_module_close (info->module);
      for (l = info->names; l; l = l->next)
	g_free (l->data);
      g_slist_free (info->names);
      g_free (info);
    }
}

static GSList *
load_modules (const char *module_str)
{
  gchar **module_names;
  GSList *module_list = NULL;
  gint i;

  CTK_NOTE (MODULES, g_message ("Loading module list: %s", module_str));

  module_names = ctk_split_file_list (module_str);
  for (i = 0; module_names[i]; i++)
    module_list = load_module (module_list, module_names[i]);

  module_list = g_slist_reverse (module_list);
  g_strfreev (module_names);

  return module_list;
}

static void
default_display_notify_cb (CdkDisplayManager *display_manager G_GNUC_UNUSED)
{
  GSList *slist;

  /* Initialize non-multihead-aware modules when the
   * default display is first set to a non-NULL value.
   */

  if (!cdk_display_get_default () || default_display_opened)
    return;

  default_display_opened = TRUE;

  for (slist = ctk_modules; slist; slist = slist->next)
    {
      if (slist->data)
	{
	  CtkModuleInfo *info = slist->data;

	  if (!info->display_init_func)
	    (* info->init_func) (&ctk_argc, &ctk_argv);
	}
    }
}

static void
display_closed_cb (CdkDisplay *display,
		   gboolean    is_error G_GNUC_UNUSED)
{
  CdkScreen *screen;
  CtkSettings *settings;

  screen = cdk_display_get_default_screen (display);
  settings = ctk_settings_get_for_screen (screen);
  if (settings)
    g_object_set_data_full (G_OBJECT (settings),
			    I_("ctk-modules"),
			    NULL, NULL);
}
		   

static void
display_opened_cb (CdkDisplayManager *display_manager G_GNUC_UNUSED,
		   CdkDisplay        *display)
{
  GValue value = G_VALUE_INIT;
  GSList *slist;
  CdkScreen *screen;

  for (slist = ctk_modules; slist; slist = slist->next)
    {
      if (slist->data)
	{
	  CtkModuleInfo *info = slist->data;

	  if (info->display_init_func)
	    (* info->display_init_func) (display);
	}
    }
  
  g_value_init (&value, G_TYPE_STRING);
  screen = cdk_display_get_default_screen (display);

  if (cdk_screen_get_setting (screen, "ctk-modules", &value))
    {
      CtkSettings *settings;

      settings = ctk_settings_get_for_screen (screen);
      _ctk_modules_settings_changed (settings, g_value_get_string (&value));
      g_value_unset (&value);
    }

  /* Since closing display doesn't actually release the resources yet,
   * we have to connect to the ::closed signal.
   */
  g_signal_connect (display, "closed", G_CALLBACK (display_closed_cb), NULL);
}

void
_ctk_modules_init (gint        *argc,
		   gchar     ***argv,
		   const gchar *ctk_modules_args)
{
  CdkDisplayManager *display_manager;
  gint i;

  g_assert (ctk_argv == NULL);

  if (argc && argv)
    {
      /* store argc and argv for later use in mod initialization */
      ctk_argc = *argc;
      ctk_argv = g_new (gchar *, *argc + 1);
      for (i = 0; i < ctk_argc; i++)
	ctk_argv [i] = g_strdup ((*argv) [i]);
      ctk_argv [*argc] = NULL;
    }

  display_manager = cdk_display_manager_get ();
  default_display_opened = cdk_display_get_default () != NULL;
  g_signal_connect (display_manager, "notify::default-display",
                    G_CALLBACK (default_display_notify_cb),
                    NULL);
  g_signal_connect (display_manager, "display-opened",
                    G_CALLBACK (display_opened_cb),
                    NULL);

  if (ctk_modules_args)
    {
      /* Modules specified in the CTK_MODULES environment variable
       * or on the command line are always loaded, so we'll just leak
       * the refcounts.
       */
      g_slist_free (load_modules (ctk_modules_args));
    }
}

static void
settings_destroy_notify (gpointer data)
{
  GSList *iter, *modules = data;

  for (iter = modules; iter; iter = iter->next) 
    {
      CtkModuleInfo *info = iter->data;
      ctk_module_info_unref (info);
    }
  g_slist_free (modules);
}

void
_ctk_modules_settings_changed (CtkSettings *settings, 
			       const gchar *modules)
{
  GSList *new_modules = NULL;

  CTK_NOTE (MODULES, g_message ("ctk-modules setting changed to: %s", modules));

  /* load/ref before unreffing existing */
  if (modules && modules[0])
    new_modules = load_modules (modules);

  g_object_set_data_full (G_OBJECT (settings),
			  I_("ctk-modules"),
			  new_modules,
			  settings_destroy_notify);
}

/* Return TRUE if module_to_check causes version conflicts.
 * If module_to_check is NULL, check the main module.
 */
gboolean
_ctk_module_has_mixed_deps (GModule *module_to_check)
{
  GModule *module;
  gpointer func;
  gboolean result;

  if (!module_to_check)
    module = g_module_open (NULL, 0);
  else
    module = module_to_check;

  if (g_module_symbol (module, "ctk_progress_get_type", &func))
    result = TRUE;
  else
    result = FALSE;

  if (!module_to_check)
    g_module_close (module);

  return result;
}
