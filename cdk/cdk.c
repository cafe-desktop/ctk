/* CDK - The GIMP Drawing Kit
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

#include "cdkversionmacros.h"
#include "cdkmain.h"

#include "cdkprofilerprivate.h"
#include "cdkinternals.h"
#include "cdkintl.h"

#include "cdkresources.h"

#include "cdk-private.h"

#ifndef HAVE_XCONVERTCASE
#include "cdkkeysyms.h"
#endif

#include "cdkconstructor.h"

#include <string.h>
#include <stdlib.h>

#include <fribidi.h>


/**
 * SECTION:general
 * @Short_description: Library initialization and miscellaneous functions
 * @Title: General
 *
 * This section describes the CDK initialization functions and miscellaneous
 * utility functions, as well as deprecation facilities.
 *
 * The CDK and CTK+ headers annotate deprecated APIs in a way that produces
 * compiler warnings if these deprecated APIs are used. The warnings
 * can be turned off by defining the macro %CDK_DISABLE_DEPRECATION_WARNINGS
 * before including the glib.h header.
 *
 * CDK and CTK+ also provide support for building applications against
 * defined subsets of deprecated or new APIs. Define the macro
 * %CDK_VERSION_MIN_REQUIRED to specify up to what version
 * you want to receive warnings about deprecated APIs. Define the
 * macro %CDK_VERSION_MAX_ALLOWED to specify the newest version
 * whose API you want to use.
 */

/**
 * CDK_WINDOWING_X11:
 *
 * The #CDK_WINDOWING_X11 macro is defined if the X11 backend
 * is supported.
 *
 * Use this macro to guard code that is specific to the X11 backend.
 */

/**
 * CDK_WINDOWING_WIN32:
 *
 * The #CDK_WINDOWING_WIN32 macro is defined if the Win32 backend
 * is supported.
 *
 * Use this macro to guard code that is specific to the Win32 backend.
 */

/**
 * CDK_WINDOWING_QUARTZ:
 *
 * The #CDK_WINDOWING_QUARTZ macro is defined if the Quartz backend
 * is supported.
 *
 * Use this macro to guard code that is specific to the Quartz backend.
 */

/**
 * CDK_WINDOWING_WAYLAND:
 *
 * The #CDK_WINDOWING_WAYLAND macro is defined if the Wayland backend
 * is supported.
 *
 * Use this macro to guard code that is specific to the Wayland backend.
 */

typedef struct _CdkPredicate  CdkPredicate;

struct _CdkPredicate
{
  CdkEventFunc func;
  gpointer data;
};

typedef struct _CdkThreadsDispatch CdkThreadsDispatch;

struct _CdkThreadsDispatch
{
  GSourceFunc func;
  gpointer data;
  GDestroyNotify destroy;
};


/* Private variable declarations
 */
static int cdk_initialized = 0;                     /* 1 if the library is initialized,
                                                     * 0 otherwise.
                                                     */

static gchar  *cdk_progclass = NULL;
static gboolean cdk_progclass_overridden;

static GMutex cdk_threads_mutex;

static GCallback cdk_threads_lock = NULL;
static GCallback cdk_threads_unlock = NULL;

static const GDebugKey cdk_gl_keys[] = {
  { "disable",               CDK_GL_DISABLE },
  { "always",                CDK_GL_ALWAYS },
  { "software-draw",         CDK_GL_SOFTWARE_DRAW_GL | CDK_GL_SOFTWARE_DRAW_SURFACE} ,
  { "software-draw-gl",      CDK_GL_SOFTWARE_DRAW_GL },
  { "software-draw-surface", CDK_GL_SOFTWARE_DRAW_SURFACE },
  { "texture-rectangle",     CDK_GL_TEXTURE_RECTANGLE },
  { "legacy",                CDK_GL_LEGACY },
  { "gles",                  CDK_GL_GLES },
};

#ifdef G_ENABLE_DEBUG
static const GDebugKey cdk_debug_keys[] = {
  { "events",        CDK_DEBUG_EVENTS },
  { "misc",          CDK_DEBUG_MISC },
  { "dnd",           CDK_DEBUG_DND },
  { "xim",           CDK_DEBUG_XIM },
  { "nograbs",       CDK_DEBUG_NOGRABS },
  { "input",         CDK_DEBUG_INPUT },
  { "cursor",        CDK_DEBUG_CURSOR },
  { "multihead",     CDK_DEBUG_MULTIHEAD },
  { "xinerama",      CDK_DEBUG_XINERAMA },
  { "draw",          CDK_DEBUG_DRAW },
  { "eventloop",     CDK_DEBUG_EVENTLOOP },
  { "frames",        CDK_DEBUG_FRAMES },
  { "settings",      CDK_DEBUG_SETTINGS },
  { "opengl",        CDK_DEBUG_OPENGL }
};

static gboolean
cdk_arg_debug_cb (const char *key, const char *value, gpointer user_data, GError **error)
{
  guint debug_value = g_parse_debug_string (value,
                                            (GDebugKey *) cdk_debug_keys,
                                            G_N_ELEMENTS (cdk_debug_keys));

  if (debug_value == 0 && value != NULL && strcmp (value, "") != 0)
    {
      g_set_error (error,
                   G_OPTION_ERROR, G_OPTION_ERROR_FAILED,
                   _("Error parsing option --cdk-debug"));
      return FALSE;
    }

  _cdk_debug_flags |= debug_value;

  return TRUE;
}

static gboolean
cdk_arg_no_debug_cb (const char *key, const char *value, gpointer user_data, GError **error)
{
  guint debug_value = g_parse_debug_string (value,
                                            (GDebugKey *) cdk_debug_keys,
                                            G_N_ELEMENTS (cdk_debug_keys));

  if (debug_value == 0 && value != NULL && strcmp (value, "") != 0)
    {
      g_set_error (error,
                   G_OPTION_ERROR, G_OPTION_ERROR_FAILED,
                   _("Error parsing option --cdk-no-debug"));
      return FALSE;
    }

  _cdk_debug_flags &= ~debug_value;

  return TRUE;
}
#endif /* G_ENABLE_DEBUG */

static gboolean
cdk_arg_class_cb (const char *key, const char *value, gpointer user_data, GError **error)
{
  cdk_set_program_class (value);
  cdk_progclass_overridden = TRUE;

  return TRUE;
}

static gboolean
cdk_arg_name_cb (const char *key, const char *value, gpointer user_data, GError **error)
{
  g_set_prgname (value);

  return TRUE;
}

static const GOptionEntry cdk_args[] = {
  { "class",        0, 0,                     G_OPTION_ARG_CALLBACK, cdk_arg_class_cb,
    /* Description of --class=CLASS in --help output */        N_("Program class as used by the window manager"),
    /* Placeholder in --class=CLASS in --help output */        N_("CLASS") },
  { "name",         0, 0,                     G_OPTION_ARG_CALLBACK, cdk_arg_name_cb,
    /* Description of --name=NAME in --help output */          N_("Program name as used by the window manager"),
    /* Placeholder in --name=NAME in --help output */          N_("NAME") },
#ifndef G_OS_WIN32
  { "display",      0, G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING,   &_cdk_display_name,
    /* Description of --display=DISPLAY in --help output */    N_("X display to use"),
    /* Placeholder in --display=DISPLAY in --help output */    N_("DISPLAY") },
#endif
#ifdef G_ENABLE_DEBUG
  { "cdk-debug",    0, 0, G_OPTION_ARG_CALLBACK, cdk_arg_debug_cb,  
    /* Description of --cdk-debug=FLAGS in --help output */    N_("CDK debugging flags to set"),
    /* Placeholder in --cdk-debug=FLAGS in --help output */    N_("FLAGS") },
  { "cdk-no-debug", 0, 0, G_OPTION_ARG_CALLBACK, cdk_arg_no_debug_cb, 
    /* Description of --cdk-no-debug=FLAGS in --help output */ N_("CDK debugging flags to unset"),
    /* Placeholder in --cdk-no-debug=FLAGS in --help output */ N_("FLAGS") },
#endif 
  { NULL }
};

void
cdk_add_option_entries (GOptionGroup *group)
{
  g_option_group_add_entries (group, cdk_args);
}

/**
 * cdk_add_option_entries_libctk_only:
 * @group: An option group.
 *
 * Appends cdk option entries to the passed in option group. This is
 * not public API and must not be used by applications.
 *
 * Deprecated: 3.16: This symbol was never meant to be used outside
 *   of CTK+
 */
void
cdk_add_option_entries_libctk_only (GOptionGroup *group)
{
  cdk_add_option_entries (group);
}

static gpointer
register_resources (gpointer dummy G_GNUC_UNUSED)
{
  _cdk_register_resource ();

  return NULL;
}

static void
cdk_ensure_resources (void)
{
  static GOnce register_resources_once = G_ONCE_INIT;

  g_once (&register_resources_once, register_resources, NULL);
}

void
cdk_pre_parse (void)
{
  const char *rendering_mode;
  const gchar *gl_string;

  cdk_initialized = TRUE;

  cdk_ensure_resources ();

  /* We set the fallback program class here, rather than lazily in
   * cdk_get_program_class, since we don't want -name to override it.
   */
  cdk_progclass = g_strdup (g_get_prgname ());
  if (cdk_progclass && cdk_progclass[0])
    cdk_progclass[0] = g_ascii_toupper (cdk_progclass[0]);
  
#ifdef G_ENABLE_DEBUG
  {
    gchar *debug_string = getenv("CDK_DEBUG");
    if (debug_string != NULL)
      _cdk_debug_flags = g_parse_debug_string (debug_string,
                                              (GDebugKey *) cdk_debug_keys,
                                              G_N_ELEMENTS (cdk_debug_keys));
    if (g_getenv ("CTK_TRACE_FD"))
      cdk_profiler_start (atoi (g_getenv ("CTK_TRACE_FD")));
    else if (g_getenv ("CTK_TRACE"))
      cdk_profiler_start (-1);
  }
#endif  /* G_ENABLE_DEBUG */

  gl_string = getenv("CDK_GL");
  if (gl_string != NULL)
    _cdk_gl_flags = g_parse_debug_string (gl_string,
                                          (GDebugKey *) cdk_gl_keys,
                                          G_N_ELEMENTS (cdk_gl_keys));

  if (getenv ("CDK_NATIVE_WINDOWS"))
    {
      g_warning ("The CDK_NATIVE_WINDOWS environment variable is not supported in CTK3.\n"
                 "See the documentation for cdk_window_ensure_native() on how to get native windows.");
      g_unsetenv ("CDK_NATIVE_WINDOWS");
    }

  rendering_mode = g_getenv ("CDK_RENDERING");
  if (rendering_mode)
    {
      if (g_str_equal (rendering_mode, "similar"))
        _cdk_rendering_mode = CDK_RENDERING_MODE_SIMILAR;
      else if (g_str_equal (rendering_mode, "image"))
        _cdk_rendering_mode = CDK_RENDERING_MODE_IMAGE;
      else if (g_str_equal (rendering_mode, "recording"))
        _cdk_rendering_mode = CDK_RENDERING_MODE_RECORDING;
    }
}

/**
 * cdk_pre_parse_libctk_only:
 *
 * Prepare for parsing command line arguments for CDK. This is not
 * public API and should not be used in application code.
 *
 * Deprecated: 3.16: This symbol was never meant to be used outside
 *   of CTK+
 */
void
cdk_pre_parse_libctk_only (void)
{
  cdk_pre_parse ();
}
  
/**
 * cdk_parse_args:
 * @argc: the number of command line arguments.
 * @argv: (inout) (array length=argc): the array of command line arguments.
 * 
 * Parse command line arguments, and store for future
 * use by calls to cdk_display_open().
 *
 * Any arguments used by CDK are removed from the array and @argc and @argv are
 * updated accordingly.
 *
 * You shouldn’t call this function explicitly if you are using
 * ctk_init(), ctk_init_check(), cdk_init(), or cdk_init_check().
 *
 * Since: 2.2
 **/
void
cdk_parse_args (int    *argc,
                char ***argv)
{
  GOptionContext *option_context;
  GOptionGroup *option_group;
  GError *error = NULL;

  if (cdk_initialized)
    return;

  cdk_pre_parse ();

  option_context = g_option_context_new (NULL);
  g_option_context_set_ignore_unknown_options (option_context, TRUE);
  g_option_context_set_help_enabled (option_context, FALSE);
  option_group = g_option_group_new (NULL, NULL, NULL, NULL, NULL);
  g_option_context_set_main_group (option_context, option_group);

  g_option_group_add_entries (option_group, cdk_args);

  if (!g_option_context_parse (option_context, argc, argv, &error))
    {
      g_warning ("%s", error->message);
      g_error_free (error);
    }
  g_option_context_free (option_context);

  CDK_NOTE (MISC, g_message ("progname: \"%s\"", g_get_prgname ()));
}

/**
 * cdk_get_display:
 *
 * Gets the name of the display, which usually comes from the
 * `DISPLAY` environment variable or the
 * `--display` command line option.
 *
 * Returns: the name of the display.
 *
 * Deprecated: 3.8: Call cdk_display_get_name (cdk_display_get_default ()))
 *    instead.
 */
gchar *
cdk_get_display (void)
{
  return g_strdup (cdk_display_get_name (cdk_display_get_default ()));
}

/**
 * cdk_get_display_arg_name:
 *
 * Gets the display name specified in the command line arguments passed
 * to cdk_init() or cdk_parse_args(), if any.
 *
 * Returns: (nullable): the display name, if specified explicitly,
 *   otherwise %NULL this string is owned by CTK+ and must not be
 *   modified or freed.
 *
 * Since: 2.2
 */
const gchar *
cdk_get_display_arg_name (void)
{
  if (!_cdk_display_arg_name)
    _cdk_display_arg_name = g_strdup (_cdk_display_name);

   return _cdk_display_arg_name;
}

/*< private >
 * cdk_display_open_default:
 *
 * Opens the default display specified by command line arguments or
 * environment variables, sets it as the default display, and returns
 * it. cdk_parse_args() must have been called first. If the default
 * display has previously been set, simply returns that. An internal
 * function that should not be used by applications.
 *
 * Returns: (nullable) (transfer none): the default display, if it
 *   could be opened, otherwise %NULL.
 */
CdkDisplay *
cdk_display_open_default (void)
{
  CdkDisplay *display;

  g_return_val_if_fail (cdk_initialized, NULL);

  display = cdk_display_get_default ();
  if (display)
    return display;

  display = cdk_display_open (cdk_get_display_arg_name ());

  return display;
}

gboolean
cdk_running_in_sandbox (void)
{
  return g_file_test ("/.flatpak-info", G_FILE_TEST_EXISTS);
}

gboolean
cdk_should_use_portal (void)
{
  static const char *use_portal = NULL;

  if (G_UNLIKELY (use_portal == NULL))
    {
      if (cdk_running_in_sandbox ())
        use_portal = "1";
      else
        {
          use_portal = g_getenv ("CTK_USE_PORTAL");
          if (!use_portal)
            use_portal = "";
        }
    }

  return use_portal[0] == '1';
}

/**
 * cdk_display_open_default_libctk_only:
 *
 * Opens the default display specified by command line arguments or
 * environment variables, sets it as the default display, and returns
 * it. cdk_parse_args() must have been called first. If the default
 * display has previously been set, simply returns that. An internal
 * function that should not be used by applications.
 *
 * Returns: (nullable) (transfer none): the default display, if it
 *   could be opened, otherwise %NULL.
 *
 * Deprecated: 3.16: This symbol was never meant to be used outside
 *   of CTK+
 */
CdkDisplay *
cdk_display_open_default_libctk_only (void)
{
  return cdk_display_open_default ();
}

/**
 * cdk_init_check:
 * @argc: (inout): the number of command line arguments.
 * @argv: (array length=argc) (inout): the array of command line arguments.
 *
 * Initializes the CDK library and connects to the windowing system,
 * returning %TRUE on success.
 *
 * Any arguments used by CDK are removed from the array and @argc and @argv
 * are updated accordingly.
 *
 * CTK+ initializes CDK in ctk_init() and so this function is not usually
 * needed by CTK+ applications.
 *
 * Returns: %TRUE if initialization succeeded.
 */
gboolean
cdk_init_check (int    *argc,
                char ***argv)
{
  cdk_parse_args (argc, argv);

  return cdk_display_open_default () != NULL;
}


/**
 * cdk_init:
 * @argc: (inout): the number of command line arguments.
 * @argv: (array length=argc) (inout): the array of command line arguments.
 *
 * Initializes the CDK library and connects to the windowing system.
 * If initialization fails, a warning message is output and the application
 * terminates with a call to `exit(1)`.
 *
 * Any arguments used by CDK are removed from the array and @argc and @argv
 * are updated accordingly.
 *
 * CTK+ initializes CDK in ctk_init() and so this function is not usually
 * needed by CTK+ applications.
 */
void
cdk_init (int *argc, char ***argv)
{
  if (!cdk_init_check (argc, argv))
    {
      const char *display_name = cdk_get_display_arg_name ();
      g_warning ("cannot open display: %s", display_name ? display_name : "");
      exit(1);
    }
}



/**
 * SECTION:threads
 * @Short_description: Functions for using CDK in multi-threaded programs
 * @Title: Threads
 *
 * For thread safety, CDK relies on the thread primitives in GLib,
 * and on the thread-safe GLib main loop.
 *
 * GLib is completely thread safe (all global data is automatically
 * locked), but individual data structure instances are not automatically
 * locked for performance reasons. So e.g. you must coordinate
 * accesses to the same #GHashTable from multiple threads.
 *
 * CTK+, however, is not thread safe. You should only use CTK+ and CDK
 * from the thread ctk_init() and ctk_main() were called on.
 * This is usually referred to as the “main thread”.
 *
 * Signals on CTK+ and CDK types, as well as non-signal callbacks, are
 * emitted in the main thread.
 *
 * You can schedule work in the main thread safely from other threads
 * by using cdk_threads_add_idle() and cdk_threads_add_timeout():
 *
 * |[<!-- language="C" -->
 * static void
 * worker_thread (void)
 * {
 *   ExpensiveData *expensive_data = do_expensive_computation ();
 *
 *   cdk_threads_add_idle (got_value, expensive_data);
 * }
 *
 * static gboolean
 * got_value (gpointer user_data)
 * {
 *   ExpensiveData *expensive_data = user_data;
 *
 *   my_app->expensive_data = expensive_data;
 *   ctk_button_set_sensitive (my_app->button, TRUE);
 *   ctk_button_set_label (my_app->button, expensive_data->result_label);
 *
 *   return G_SOURCE_REMOVE;
 * }
 * ]|
 *
 * You should use cdk_threads_add_idle() and cdk_threads_add_timeout()
 * instead of g_idle_add() and g_timeout_add() since libraries not under
 * your control might be using the deprecated CDK locking mechanism.
 * If you are sure that none of the code in your application and libraries
 * use the deprecated cdk_threads_enter() or cdk_threads_leave() methods,
 * then you can safely use g_idle_add() and g_timeout_add().
 *
 * For more information on this "worker thread" pattern, you should
 * also look at #GTask, which gives you high-level tools to perform
 * expensive tasks from worker threads, and will handle thread
 * management for you.
 */

/**
 * cdk_threads_enter:
 *
 * This function marks the beginning of a critical section in which
 * CDK and CTK+ functions can be called safely and without causing race
 * conditions. Only one thread at a time can be in such a critial
 * section.
 *
 * Deprecated:3.6: All CDK and CTK+ calls should be made from the main
 *     thread
 */
void
cdk_threads_enter (void)
{
  if (cdk_threads_lock)
    (*cdk_threads_lock) ();
}

/**
 * cdk_threads_leave:
 *
 * Leaves a critical region begun with cdk_threads_enter().
 *
 * Deprecated:3.6: All CDK and CTK+ calls should be made from the main
 *     thread
 */
void
cdk_threads_leave (void)
{
  if (cdk_threads_unlock)
    (*cdk_threads_unlock) ();
}

static void
cdk_threads_impl_lock (void)
{
  g_mutex_lock (&cdk_threads_mutex);
}

static void
cdk_threads_impl_unlock (void)
{
  /* we need a trylock() here because trying to unlock a mutex
   * that hasn't been locked yet is:
   *
   *  a) not portable
   *  b) fail on GLib ≥ 2.41
   *
   * trylock() will either succeed because nothing is holding the
   * CDK mutex, and will be unlocked right afterwards; or it's
   * going to fail because the mutex is locked already, in which
   * case we unlock it as expected.
   *
   * this is needed in the case somebody called cdk_threads_init()
   * without calling cdk_threads_enter() before calling ctk_main().
   * in theory, we could just say that this is undefined behaviour,
   * but our documentation has always been *less* than explicit as
   * to what the behaviour should actually be.
   *
   * see bug: https://bugzilla.gnome.org/show_bug.cgi?id=735428
   */
  g_mutex_trylock (&cdk_threads_mutex);
  g_mutex_unlock (&cdk_threads_mutex);
}

/**
 * cdk_threads_init:
 *
 * Initializes CDK so that it can be used from multiple threads
 * in conjunction with cdk_threads_enter() and cdk_threads_leave().
 *
 * This call must be made before any use of the main loop from
 * CTK+; to be safe, call it before ctk_init().
 *
 * Deprecated:3.6: All CDK and CTK+ calls should be made from the main
 *     thread
 */
void
cdk_threads_init (void)
{
  if (!cdk_threads_lock)
    cdk_threads_lock = cdk_threads_impl_lock;
  if (!cdk_threads_unlock)
    cdk_threads_unlock = cdk_threads_impl_unlock;
}

/**
 * cdk_threads_set_lock_functions: (skip)
 * @enter_fn:   function called to guard CDK
 * @leave_fn: function called to release the guard
 *
 * Allows the application to replace the standard method that
 * CDK uses to protect its data structures. Normally, CDK
 * creates a single #GMutex that is locked by cdk_threads_enter(),
 * and released by cdk_threads_leave(); using this function an
 * application provides, instead, a function @enter_fn that is
 * called by cdk_threads_enter() and a function @leave_fn that is
 * called by cdk_threads_leave().
 *
 * The functions must provide at least same locking functionality
 * as the default implementation, but can also do extra application
 * specific processing.
 *
 * As an example, consider an application that has its own recursive
 * lock that when held, holds the CTK+ lock as well. When CTK+ unlocks
 * the CTK+ lock when entering a recursive main loop, the application
 * must temporarily release its lock as well.
 *
 * Most threaded CTK+ apps won’t need to use this method.
 *
 * This method must be called before cdk_threads_init(), and cannot
 * be called multiple times.
 *
 * Deprecated:3.6: All CDK and CTK+ calls should be made from the main
 *     thread
 *
 * Since: 2.4
 **/
void
cdk_threads_set_lock_functions (GCallback enter_fn,
                                GCallback leave_fn)
{
  g_return_if_fail (cdk_threads_lock == NULL &&
                    cdk_threads_unlock == NULL);

  cdk_threads_lock = enter_fn;
  cdk_threads_unlock = leave_fn;
}

static gboolean
cdk_threads_dispatch (gpointer data)
{
  CdkThreadsDispatch *dispatch = data;
  gboolean ret = FALSE;

  cdk_threads_enter ();

  if (!g_source_is_destroyed (g_main_current_source ()))
    ret = dispatch->func (dispatch->data);

  cdk_threads_leave ();

  return ret;
}

static void
cdk_threads_dispatch_free (gpointer data)
{
  CdkThreadsDispatch *dispatch = data;

  if (dispatch->destroy && dispatch->data)
    dispatch->destroy (dispatch->data);

  g_slice_free (CdkThreadsDispatch, data);
}


/**
 * cdk_threads_add_idle_full: (rename-to cdk_threads_add_idle)
 * @priority: the priority of the idle source. Typically this will be in the
 *            range between #G_PRIORITY_DEFAULT_IDLE and #G_PRIORITY_HIGH_IDLE
 * @function: function to call
 * @data:     data to pass to @function
 * @notify: (allow-none):   function to call when the idle is removed, or %NULL
 *
 * Adds a function to be called whenever there are no higher priority
 * events pending.  If the function returns %FALSE it is automatically
 * removed from the list of event sources and will not be called again.
 *
 * This variant of g_idle_add_full() calls @function with the CDK lock
 * held. It can be thought of a MT-safe version for CTK+ widgets for the
 * following use case, where you have to worry about idle_callback()
 * running in thread A and accessing @self after it has been finalized
 * in thread B:
 *
 * |[<!-- language="C" -->
 * static gboolean
 * idle_callback (gpointer data)
 * {
 *    // cdk_threads_enter(); would be needed for g_idle_add()
 *
 *    SomeWidget *self = data;
 *    // do stuff with self
 *
 *    self->idle_id = 0;
 *
 *    // cdk_threads_leave(); would be needed for g_idle_add()
 *    return FALSE;
 * }
 *
 * static void
 * some_widget_do_stuff_later (SomeWidget *self)
 * {
 *    self->idle_id = cdk_threads_add_idle (idle_callback, self)
 *    // using g_idle_add() here would require thread protection in the callback
 * }
 *
 * static void
 * some_widget_finalize (GObject *object)
 * {
 *    SomeWidget *self = SOME_WIDGET (object);
 *    if (self->idle_id)
 *      g_source_remove (self->idle_id);
 *    G_OBJECT_CLASS (parent_class)->finalize (object);
 * }
 * ]|
 *
 * Returns: the ID (greater than 0) of the event source.
 *
 * Since: 2.12
 */
guint
cdk_threads_add_idle_full (gint           priority,
                           GSourceFunc    function,
                           gpointer       data,
                           GDestroyNotify notify)
{
  CdkThreadsDispatch *dispatch;

  g_return_val_if_fail (function != NULL, 0);

  dispatch = g_slice_new (CdkThreadsDispatch);
  dispatch->func = function;
  dispatch->data = data;
  dispatch->destroy = notify;

  return g_idle_add_full (priority,
                          cdk_threads_dispatch,
                          dispatch,
                          cdk_threads_dispatch_free);
}

/**
 * cdk_threads_add_idle: (skip)
 * @function: function to call
 * @data:     data to pass to @function
 *
 * A wrapper for the common usage of cdk_threads_add_idle_full() 
 * assigning the default priority, #G_PRIORITY_DEFAULT_IDLE.
 *
 * See cdk_threads_add_idle_full().
 *
 * Returns: the ID (greater than 0) of the event source.
 * 
 * Since: 2.12
 */
guint
cdk_threads_add_idle (GSourceFunc    function,
                      gpointer       data)
{
  return cdk_threads_add_idle_full (G_PRIORITY_DEFAULT_IDLE,
                                    function, data, NULL);
}


/**
 * cdk_threads_add_timeout_full: (rename-to cdk_threads_add_timeout)
 * @priority: the priority of the timeout source. Typically this will be in the
 *            range between #G_PRIORITY_DEFAULT_IDLE and #G_PRIORITY_HIGH_IDLE.
 * @interval: the time between calls to the function, in milliseconds
 *             (1/1000ths of a second)
 * @function: function to call
 * @data:     data to pass to @function
 * @notify: (allow-none):   function to call when the timeout is removed, or %NULL
 *
 * Sets a function to be called at regular intervals holding the CDK lock,
 * with the given priority.  The function is called repeatedly until it 
 * returns %FALSE, at which point the timeout is automatically destroyed 
 * and the function will not be called again.  The @notify function is
 * called when the timeout is destroyed.  The first call to the
 * function will be at the end of the first @interval.
 *
 * Note that timeout functions may be delayed, due to the processing of other
 * event sources. Thus they should not be relied on for precise timing.
 * After each call to the timeout function, the time of the next
 * timeout is recalculated based on the current time and the given interval
 * (it does not try to “catch up” time lost in delays).
 *
 * This variant of g_timeout_add_full() can be thought of a MT-safe version 
 * for CTK+ widgets for the following use case:
 *
 * |[<!-- language="C" -->
 * static gboolean timeout_callback (gpointer data)
 * {
 *    SomeWidget *self = data;
 *    
 *    // do stuff with self
 *    
 *    self->timeout_id = 0;
 *    
 *    return G_SOURCE_REMOVE;
 * }
 *  
 * static void some_widget_do_stuff_later (SomeWidget *self)
 * {
 *    self->timeout_id = g_timeout_add (timeout_callback, self)
 * }
 *  
 * static void some_widget_finalize (GObject *object)
 * {
 *    SomeWidget *self = SOME_WIDGET (object);
 *    
 *    if (self->timeout_id)
 *      g_source_remove (self->timeout_id);
 *    
 *    G_OBJECT_CLASS (parent_class)->finalize (object);
 * }
 * ]|
 *
 * Returns: the ID (greater than 0) of the event source.
 * 
 * Since: 2.12
 */
guint
cdk_threads_add_timeout_full (gint           priority,
                              guint          interval,
                              GSourceFunc    function,
                              gpointer       data,
                              GDestroyNotify notify)
{
  CdkThreadsDispatch *dispatch;

  g_return_val_if_fail (function != NULL, 0);

  dispatch = g_slice_new (CdkThreadsDispatch);
  dispatch->func = function;
  dispatch->data = data;
  dispatch->destroy = notify;

  return g_timeout_add_full (priority, 
                             interval,
                             cdk_threads_dispatch, 
                             dispatch, 
                             cdk_threads_dispatch_free);
}

/**
 * cdk_threads_add_timeout: (skip)
 * @interval: the time between calls to the function, in milliseconds
 *             (1/1000ths of a second)
 * @function: function to call
 * @data:     data to pass to @function
 *
 * A wrapper for the common usage of cdk_threads_add_timeout_full() 
 * assigning the default priority, #G_PRIORITY_DEFAULT.
 *
 * See cdk_threads_add_timeout_full().
 * 
 * Returns: the ID (greater than 0) of the event source.
 *
 * Since: 2.12
 */
guint
cdk_threads_add_timeout (guint       interval,
                         GSourceFunc function,
                         gpointer    data)
{
  return cdk_threads_add_timeout_full (G_PRIORITY_DEFAULT,
                                       interval, function, data, NULL);
}


/**
 * cdk_threads_add_timeout_seconds_full: (rename-to cdk_threads_add_timeout_seconds)
 * @priority: the priority of the timeout source. Typically this will be in the
 *            range between #G_PRIORITY_DEFAULT_IDLE and #G_PRIORITY_HIGH_IDLE.
 * @interval: the time between calls to the function, in seconds
 * @function: function to call
 * @data:     data to pass to @function
 * @notify: (allow-none): function to call when the timeout is removed, or %NULL
 *
 * A variant of cdk_threads_add_timeout_full() with second-granularity.
 * See g_timeout_add_seconds_full() for a discussion of why it is
 * a good idea to use this function if you don’t need finer granularity.
 *
 * Returns: the ID (greater than 0) of the event source.
 * 
 * Since: 2.14
 */
guint
cdk_threads_add_timeout_seconds_full (gint           priority,
                                      guint          interval,
                                      GSourceFunc    function,
                                      gpointer       data,
                                      GDestroyNotify notify)
{
  CdkThreadsDispatch *dispatch;

  g_return_val_if_fail (function != NULL, 0);

  dispatch = g_slice_new (CdkThreadsDispatch);
  dispatch->func = function;
  dispatch->data = data;
  dispatch->destroy = notify;

  return g_timeout_add_seconds_full (priority, 
                                     interval,
                                     cdk_threads_dispatch, 
                                     dispatch, 
                                     cdk_threads_dispatch_free);
}

/**
 * cdk_threads_add_timeout_seconds: (skip)
 * @interval: the time between calls to the function, in seconds
 * @function: function to call
 * @data:     data to pass to @function
 *
 * A wrapper for the common usage of cdk_threads_add_timeout_seconds_full() 
 * assigning the default priority, #G_PRIORITY_DEFAULT.
 *
 * For details, see cdk_threads_add_timeout_full().
 * 
 * Returns: the ID (greater than 0) of the event source.
 *
 * Since: 2.14
 */
guint
cdk_threads_add_timeout_seconds (guint       interval,
                                 GSourceFunc function,
                                 gpointer    data)
{
  return cdk_threads_add_timeout_seconds_full (G_PRIORITY_DEFAULT,
                                               interval, function, data, NULL);
}

/**
 * cdk_get_program_class:
 *
 * Gets the program class. Unless the program class has explicitly
 * been set with cdk_set_program_class() or with the `--class`
 * commandline option, the default value is the program name (determined
 * with g_get_prgname()) with the first character converted to uppercase.
 *
 * Returns: the program class.
 */
const char *
cdk_get_program_class (void)
{
  return cdk_progclass;
}

/**
 * cdk_set_program_class:
 * @program_class: a string.
 *
 * Sets the program class. The X11 backend uses the program class to set
 * the class name part of the `WM_CLASS` property on
 * toplevel windows; see the ICCCM.
 *
 * The program class can still be overridden with the --class command
 * line option.
 */
void
cdk_set_program_class (const char *program_class)
{
  if (cdk_progclass_overridden)
    return;

  g_free (cdk_progclass);

  cdk_progclass = g_strdup (program_class);
}

/**
 * cdk_disable_multidevice:
 *
 * Disables multidevice support in CDK. This call must happen prior
 * to cdk_display_open(), ctk_init(), ctk_init_with_args() or
 * ctk_init_check() in order to take effect.
 *
 * Most common CTK+ applications won’t ever need to call this. Only
 * applications that do mixed CDK/Xlib calls could want to disable
 * multidevice support if such Xlib code deals with input devices in
 * any way and doesn’t observe the presence of XInput 2.
 *
 * Since: 3.0
 */
void
cdk_disable_multidevice (void)
{
  if (cdk_initialized)
    return;

  _cdk_disable_multidevice = TRUE;
}

PangoDirection
cdk_unichar_direction (gunichar ch)
{
  FriBidiCharType fribidi_ch_type;

  G_STATIC_ASSERT (sizeof (FriBidiChar) == sizeof (gunichar));

  fribidi_ch_type = fribidi_get_bidi_type (ch);

  if (!FRIBIDI_IS_STRONG (fribidi_ch_type))
    return PANGO_DIRECTION_NEUTRAL;
  else if (FRIBIDI_IS_RTL (fribidi_ch_type))
    return PANGO_DIRECTION_RTL;
  else
    return PANGO_DIRECTION_LTR;
}

#ifdef G_HAS_CONSTRUCTORS
#ifdef G_DEFINE_CONSTRUCTOR_NEEDS_PRAGMA
#pragma G_DEFINE_CONSTRUCTOR_PRAGMA_ARGS(stash_startup_id)
#pragma G_DEFINE_CONSTRUCTOR_PRAGMA_ARGS(stash_autostart_id)
#endif
G_DEFINE_CONSTRUCTOR(stash_startup_id)
G_DEFINE_CONSTRUCTOR(stash_autostart_id)
#endif

static char *desktop_startup_id = NULL;
static char *desktop_autostart_id = NULL;

static void
stash_startup_id (void)
{
  const char *startup_id = g_getenv ("DESKTOP_STARTUP_ID");

  if (startup_id == NULL || startup_id[0] == '\0')
    return;

  if (!g_utf8_validate (startup_id, -1, NULL))
    {
      g_warning ("DESKTOP_STARTUP_ID contains invalid UTF-8");
      return;
    }

  desktop_startup_id = g_strdup (startup_id);
}

static void
stash_autostart_id (void)
{
  const char *autostart_id = g_getenv ("DESKTOP_AUTOSTART_ID");
  desktop_autostart_id = g_strdup (autostart_id ? autostart_id : "");
}

const gchar *
cdk_get_desktop_startup_id (void)
{
  static gsize init = 0;

  if (g_once_init_enter (&init))
    {
#ifndef G_HAS_CONSTRUCTORS
      stash_startup_id ();
#endif
      /* Clear the environment variable so it won't be inherited by
       * child processes and confuse things.
       */
      g_unsetenv ("DESKTOP_STARTUP_ID");

      g_once_init_leave (&init, 1);
    }

  return desktop_startup_id;
}

const gchar *
cdk_get_desktop_autostart_id (void)
{
  static gsize init = 0;

  if (g_once_init_enter (&init))
    {
#ifndef G_HAS_CONSTRUCTORS
      stash_autostart_id ();
#endif
      /* Clear the environment variable so it won't be inherited by
       * child processes and confuse things.
       */
      g_unsetenv ("DESKTOP_AUTOSTART_ID");

      g_once_init_leave (&init, 1);
    }

  return desktop_autostart_id;
}
