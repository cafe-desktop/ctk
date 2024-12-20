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

/**
 * SECTION:ctkmain
 * @Short_description: Library initialization, main event loop, and events
 * @Title: Main loop and Events
 * @See_also:See the GLib manual, especially #GMainLoop and signal-related
 *    functions such as g_signal_connect()
 *
 * Before using CTK+, you need to initialize it; initialization connects to the
 * window system display, and parses some standard command line arguments. The
 * ctk_init() macro initializes CTK+. ctk_init() exits the application if errors
 * occur; to avoid this, use ctk_init_check(). ctk_init_check() allows you to
 * recover from a failed CTK+ initialization - you might start up your
 * application in text mode instead.
 *
 * Like all GUI toolkits, CTK+ uses an event-driven programming model. When the
 * user is doing nothing, CTK+ sits in the “main loop” and
 * waits for input. If the user performs some action - say, a mouse click - then
 * the main loop “wakes up” and delivers an event to CTK+. CTK+ forwards the
 * event to one or more widgets.
 *
 * When widgets receive an event, they frequently emit one or more
 * “signals”. Signals notify your program that "something
 * interesting happened" by invoking functions you’ve connected to the signal
 * with g_signal_connect(). Functions connected to a signal are often termed
 * “callbacks”.
 *
 * When your callbacks are invoked, you would typically take some action - for
 * example, when an Open button is clicked you might display a
 * #CtkFileChooserDialog. After a callback finishes, CTK+ will return to the
 * main loop and await more user input.
 *
 * ## Typical main() function for a CTK+ application
 *
 * |[<!-- language="C" -->
 * int
 * main (int argc, char **argv)
 * {
 *  CtkWidget *mainwin;
 *   // Initialize i18n support with bindtextdomain(), etc.
 *
 *   // ...
 *
 *   // Initialize the widget set
 *   ctk_init (&argc, &argv);
 *
 *   // Create the main window
 *   mainwin = ctk_window_new (CTK_WINDOW_TOPLEVEL);
 *
 *   // Set up our GUI elements
 *
 *   // ...
 *
 *   // Show the application window
 *   ctk_widget_show_all (mainwin);
 *
 *   // Enter the main event loop, and wait for user interaction
 *   ctk_main ();
 *
 *   // The user lost interest
 *   return 0;
 * }
 * ]|
 *
 * It’s OK to use the GLib main loop directly instead of ctk_main(), though it
 * involves slightly more typing. See #GMainLoop in the GLib documentation.
 */

#include "config.h"

#include "cdk/cdk.h"
#include "cdk/cdk-private.h"

#include <locale.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>          /* For uid_t, gid_t */

#ifdef G_OS_WIN32
#define STRICT
#include <windows.h>
#undef STRICT
#endif

#include "ctkintl.h"

#include "ctkaccelmapprivate.h"
#include "ctkbox.h"
#include "ctkclipboardprivate.h"
#include "ctkdebug.h"
#include "ctkdndprivate.h"
#include "ctkmain.h"
#include "ctkmenu.h"
#include "ctkmodules.h"
#include "ctkmodulesprivate.h"
#include "ctkprivate.h"
#include "ctkrecentmanager.h"
#include "ctkselectionprivate.h"
#include "ctksettingsprivate.h"
#include "ctktooltipprivate.h"
#include "ctkversion.h"
#include "ctkwidgetprivate.h"
#include "ctkwindowprivate.h"
#include "ctkwindowgroup.h"

#include "a11y/ctkaccessibility.h"

/* Private type definitions
 */
typedef struct _CtkKeySnooperData        CtkKeySnooperData;

struct _CtkKeySnooperData
{
  CtkKeySnoopFunc func;
  gpointer func_data;
  guint id;
};

static gint  ctk_invoke_key_snoopers     (CtkWidget          *grab_widget,
                                          CdkEvent           *event);

static CtkWindowGroup *ctk_main_get_window_group (CtkWidget   *widget);

static guint ctk_main_loop_level = 0;
static gint pre_initialized = FALSE;
static gint ctk_initialized = FALSE;
static GList *current_events = NULL;

static GSList *main_loops = NULL;      /* stack of currently executing main loops */

static GSList *key_snoopers = NULL;

typedef struct {
  CdkDisplay *display;
  guint flags;
} DisplayDebugFlags;

#define N_DEBUG_DISPLAYS 4

DisplayDebugFlags debug_flags[N_DEBUG_DISPLAYS];

#ifdef G_ENABLE_DEBUG
static const GDebugKey ctk_debug_keys[] = {
  { "misc", CTK_DEBUG_MISC },
  { "plugsocket", CTK_DEBUG_PLUGSOCKET },
  { "text", CTK_DEBUG_TEXT },
  { "tree", CTK_DEBUG_TREE },
  { "updates", CTK_DEBUG_UPDATES },
  { "keybindings", CTK_DEBUG_KEYBINDINGS },
  { "multihead", CTK_DEBUG_MULTIHEAD },
  { "modules", CTK_DEBUG_MODULES },
  { "geometry", CTK_DEBUG_GEOMETRY },
  { "icontheme", CTK_DEBUG_ICONTHEME },
  { "printing", CTK_DEBUG_PRINTING} ,
  { "builder", CTK_DEBUG_BUILDER },
  { "size-request", CTK_DEBUG_SIZE_REQUEST },
  { "no-css-cache", CTK_DEBUG_NO_CSS_CACHE },
  { "baselines", CTK_DEBUG_BASELINES },
  { "pixel-cache", CTK_DEBUG_PIXEL_CACHE },
  { "no-pixel-cache", CTK_DEBUG_NO_PIXEL_CACHE },
  { "interactive", CTK_DEBUG_INTERACTIVE },
  { "touchscreen", CTK_DEBUG_TOUCHSCREEN },
  { "actions", CTK_DEBUG_ACTIONS },
  { "resize", CTK_DEBUG_RESIZE },
  { "layout", CTK_DEBUG_LAYOUT }
};
#endif /* G_ENABLE_DEBUG */

/**
 * ctk_get_major_version:
 *
 * Returns the major version number of the CTK+ library.
 * (e.g. in CTK+ version 3.1.5 this is 3.)
 *
 * This function is in the library, so it represents the CTK+ library
 * your code is running against. Contrast with the #CTK_MAJOR_VERSION
 * macro, which represents the major version of the CTK+ headers you
 * have included when compiling your code.
 *
 * Returns: the major version number of the CTK+ library
 *
 * Since: 3.0
 */
guint
ctk_get_major_version (void)
{
  return CTK_MAJOR_VERSION;
}

/**
 * ctk_get_minor_version:
 *
 * Returns the minor version number of the CTK+ library.
 * (e.g. in CTK+ version 3.1.5 this is 1.)
 *
 * This function is in the library, so it represents the CTK+ library
 * your code is are running against. Contrast with the
 * #CTK_MINOR_VERSION macro, which represents the minor version of the
 * CTK+ headers you have included when compiling your code.
 *
 * Returns: the minor version number of the CTK+ library
 *
 * Since: 3.0
 */
guint
ctk_get_minor_version (void)
{
  return CTK_MINOR_VERSION;
}

/**
 * ctk_get_micro_version:
 *
 * Returns the micro version number of the CTK+ library.
 * (e.g. in CTK+ version 3.1.5 this is 5.)
 *
 * This function is in the library, so it represents the CTK+ library
 * your code is are running against. Contrast with the
 * #CTK_MICRO_VERSION macro, which represents the micro version of the
 * CTK+ headers you have included when compiling your code.
 *
 * Returns: the micro version number of the CTK+ library
 *
 * Since: 3.0
 */
guint
ctk_get_micro_version (void)
{
  return CTK_MICRO_VERSION;
}

/**
 * ctk_get_binary_age:
 *
 * Returns the binary age as passed to `libtool`
 * when building the CTK+ library the process is running against.
 * If `libtool` means nothing to you, don't
 * worry about it.
 *
 * Returns: the binary age of the CTK+ library
 *
 * Since: 3.0
 */
guint
ctk_get_binary_age (void)
{
  return CTK_BINARY_AGE;
}

/**
 * ctk_get_interface_age:
 *
 * Returns the interface age as passed to `libtool`
 * when building the CTK+ library the process is running against.
 * If `libtool` means nothing to you, don't
 * worry about it.
 *
 * Returns: the interface age of the CTK+ library
 *
 * Since: 3.0
 */
guint
ctk_get_interface_age (void)
{
  return CTK_INTERFACE_AGE;
}

/**
 * ctk_check_version:
 * @required_major: the required major version
 * @required_minor: the required minor version
 * @required_micro: the required micro version
 *
 * Checks that the CTK+ library in use is compatible with the
 * given version. Generally you would pass in the constants
 * #CTK_MAJOR_VERSION, #CTK_MINOR_VERSION, #CTK_MICRO_VERSION
 * as the three arguments to this function; that produces
 * a check that the library in use is compatible with
 * the version of CTK+ the application or module was compiled
 * against.
 *
 * Compatibility is defined by two things: first the version
 * of the running library is newer than the version
 * @required_major.required_minor.@required_micro. Second
 * the running library must be binary compatible with the
 * version @required_major.required_minor.@required_micro
 * (same major version.)
 *
 * This function is primarily for CTK+ modules; the module
 * can call this function to check that it wasn’t loaded
 * into an incompatible version of CTK+. However, such a
 * check isn’t completely reliable, since the module may be
 * linked against an old version of CTK+ and calling the
 * old version of ctk_check_version(), but still get loaded
 * into an application using a newer version of CTK+.
 *
 * Returns: (nullable): %NULL if the CTK+ library is compatible with the
 *   given version, or a string describing the version mismatch.
 *   The returned string is owned by CTK+ and should not be modified
 *   or freed.
 */
const gchar*
ctk_check_version (guint required_major,
                   guint required_minor,
                   guint required_micro)
{
  gint ctk_effective_micro = 100 * CTK_MINOR_VERSION + CTK_MICRO_VERSION;
  gint required_effective_micro = 100 * required_minor + required_micro;

  if (required_major > CTK_MAJOR_VERSION)
    return "CTK+ version too old (major mismatch)";
  if (required_major < CTK_MAJOR_VERSION)
    return "CTK+ version too new (major mismatch)";
  if (required_effective_micro < ctk_effective_micro - CTK_BINARY_AGE)
    return "CTK+ version too new (micro mismatch)";
  if (required_effective_micro > ctk_effective_micro)
    return "CTK+ version too old (micro mismatch)";
  return NULL;
}

/* This checks to see if the process is running suid or sgid
 * at the current time. If so, we don’t allow CTK+ to be initialized.
 * This is meant to be a mild check - we only error out if we
 * can prove the programmer is doing something wrong, not if
 * they could be doing something wrong. For this reason, we
 * don’t use issetugid() on BSD or prctl (PR_GET_DUMPABLE).
 */
static gboolean
check_setugid (void)
{
/* this isn't at all relevant on MS Windows and doesn't compile ... --hb */
#ifndef G_OS_WIN32
  uid_t ruid, euid, suid; /* Real, effective and saved user ID's */
  gid_t rgid, egid, sgid; /* Real, effective and saved group ID's */
  
#ifdef HAVE_GETRESUID
  /* These aren't in the header files, so we prototype them here.
   */
  int getresuid(uid_t *ruid, uid_t *euid, uid_t *suid);
  int getresgid(gid_t *rgid, gid_t *egid, gid_t *sgid);

  if (getresuid (&ruid, &euid, &suid) != 0 ||
      getresgid (&rgid, &egid, &sgid) != 0)
#endif /* HAVE_GETRESUID */
    {
      suid = ruid = getuid ();
      sgid = rgid = getgid ();
      euid = geteuid ();
      egid = getegid ();
    }

  if (ruid != euid || ruid != suid ||
      rgid != egid || rgid != sgid)
    {
      g_warning ("This process is currently running setuid or setgid.\n"
                 "This is not a supported use of CTK+. You must create a helper\n"
                 "program instead. For further details, see:\n\n"
                 "    http://github.com/cafe-desktop/ctk/setuid.html\n\n"
                 "Refusing to initialize CTK+.");
      exit (1);
    }
#endif
  return TRUE;
}

static gboolean do_setlocale = TRUE;

/**
 * ctk_disable_setlocale:
 * 
 * Prevents ctk_init(), ctk_init_check(), ctk_init_with_args() and
 * ctk_parse_args() from automatically
 * calling `setlocale (LC_ALL, "")`. You would
 * want to use this function if you wanted to set the locale for
 * your program to something other than the user’s locale, or if
 * you wanted to set different values for different locale categories.
 *
 * Most programs should not need to call this function.
 **/
void
ctk_disable_setlocale (void)
{
  if (pre_initialized)
    g_warning ("ctk_disable_setlocale() must be called before ctk_init()");
    
  do_setlocale = FALSE;
}

#ifdef G_PLATFORM_WIN32
#undef ctk_init_check
#endif

static GString *ctk_modules_string = NULL;
static gboolean g_fatal_warnings = FALSE;

#ifdef G_ENABLE_DEBUG
static gboolean
ctk_arg_debug_cb (const char *key G_GNUC_UNUSED,
		  const char *value,
		  gpointer    user_data G_GNUC_UNUSED)
{
  debug_flags[0].flags |= g_parse_debug_string (value,
                                                ctk_debug_keys,
                                                G_N_ELEMENTS (ctk_debug_keys));

  return TRUE;
}

static gboolean
ctk_arg_no_debug_cb (const char *key G_GNUC_UNUSED,
		     const char *value,
		     gpointer    user_data G_GNUC_UNUSED)
{
  debug_flags[0].flags &= ~g_parse_debug_string (value,
                                                 ctk_debug_keys,
                                                 G_N_ELEMENTS (ctk_debug_keys));

  return TRUE;
}
#endif /* G_ENABLE_DEBUG */

static gboolean
ctk_arg_module_cb (const char *key G_GNUC_UNUSED,
		   const char *value,
		   gpointer    user_data G_GNUC_UNUSED)
{
  if (value && *value)
    {
      if (ctk_modules_string)
        g_string_append_c (ctk_modules_string, G_SEARCHPATH_SEPARATOR);
      else
        ctk_modules_string = g_string_new (NULL);
      
      g_string_append (ctk_modules_string, value);
    }

  return TRUE;
}

static const GOptionEntry ctk_args[] = {
  { "ctk-module",       0, 0, G_OPTION_ARG_CALLBACK, ctk_arg_module_cb,   
    /* Description of --ctk-module=MODULES in --help output */ N_("Load additional CTK+ modules"), 
    /* Placeholder in --ctk-module=MODULES in --help output */ N_("MODULES") },
  { "g-fatal-warnings", 0, 0, G_OPTION_ARG_NONE, &g_fatal_warnings, 
    /* Description of --g-fatal-warnings in --help output */   N_("Make all warnings fatal"), NULL },
#ifdef G_ENABLE_DEBUG
  { "ctk-debug",        0, 0, G_OPTION_ARG_CALLBACK, ctk_arg_debug_cb,    
    /* Description of --ctk-debug=FLAGS in --help output */    N_("CTK+ debugging flags to set"), 
    /* Placeholder in --ctk-debug=FLAGS in --help output */    N_("FLAGS") },
  { "ctk-no-debug",     0, 0, G_OPTION_ARG_CALLBACK, ctk_arg_no_debug_cb, 
    /* Description of --ctk-no-debug=FLAGS in --help output */ N_("CTK+ debugging flags to unset"), 
    /* Placeholder in --ctk-no-debug=FLAGS in --help output */ N_("FLAGS") },
#endif 
  { NULL }
};

#ifdef G_OS_WIN32

static char *iso639_to_check = NULL;
static char *iso3166_to_check = NULL;
static char *script_to_check = NULL;
static gboolean setlocale_called = FALSE;

static BOOL CALLBACK
enum_locale_proc (LPTSTR locale)
{
  LCID lcid;
  char iso639[10];
  char iso3166[10];
  char *endptr;


  lcid = strtoul (locale, &endptr, 16);
  if (*endptr == '\0' &&
      GetLocaleInfo (lcid, LOCALE_SISO639LANGNAME, iso639, sizeof (iso639)) &&
      GetLocaleInfo (lcid, LOCALE_SISO3166CTRYNAME, iso3166, sizeof (iso3166)))
    {
      if (strcmp (iso639, iso639_to_check) == 0 &&
          ((iso3166_to_check != NULL &&
            strcmp (iso3166, iso3166_to_check) == 0) ||
           (iso3166_to_check == NULL &&
            SUBLANGID (LANGIDFROMLCID (lcid)) == SUBLANG_DEFAULT)))
        {
          char language[100], country[100];
          char locale[300];

          if (script_to_check != NULL)
            {
              /* If lcid is the "other" script for this language,
               * return TRUE, i.e. continue looking.
               */
              if (strcmp (script_to_check, "Latn") == 0)
                {
                  switch (LANGIDFROMLCID (lcid))
                    {
                    case MAKELANGID (LANG_AZERI, SUBLANG_AZERI_CYRILLIC):
                      return TRUE;
                    case MAKELANGID (LANG_UZBEK, SUBLANG_UZBEK_CYRILLIC):
                      return TRUE;
                    case MAKELANGID (LANG_SERBIAN, SUBLANG_SERBIAN_CYRILLIC):
                      return TRUE;
                    case MAKELANGID (LANG_SERBIAN, 0x07):
                      /* Serbian in Bosnia and Herzegovina, Cyrillic */
                      return TRUE;
                    }
                }
              else if (strcmp (script_to_check, "Cyrl") == 0)
                {
                  switch (LANGIDFROMLCID (lcid))
                    {
                    case MAKELANGID (LANG_AZERI, SUBLANG_AZERI_LATIN):
                      return TRUE;
                    case MAKELANGID (LANG_UZBEK, SUBLANG_UZBEK_LATIN):
                      return TRUE;
                    case MAKELANGID (LANG_SERBIAN, SUBLANG_SERBIAN_LATIN):
                      return TRUE;
                    case MAKELANGID (LANG_SERBIAN, 0x06):
                      /* Serbian in Bosnia and Herzegovina, Latin */
                      return TRUE;
                    }
                }
            }

          SetThreadLocale (lcid);

          if (GetLocaleInfo (lcid, LOCALE_SENGLANGUAGE, language, sizeof (language)) &&
              GetLocaleInfo (lcid, LOCALE_SENGCOUNTRY, country, sizeof (country)))
            {
              strcpy (locale, language);
              strcat (locale, "_");
              strcat (locale, country);

              if (setlocale (LC_ALL, locale) != NULL)
                setlocale_called = TRUE;
            }

          return FALSE;
        }
    }

  return TRUE;
}
  
#endif

static void
setlocale_initialization (void)
{
  static gboolean initialized = FALSE;

  if (initialized)
    return;
  initialized = TRUE;

  if (do_setlocale)
    {
#ifdef G_OS_WIN32
      /* If some of the POSIXish environment variables are set, set
       * the Win32 thread locale correspondingly.
       */ 
      char *p = getenv ("LC_ALL");
      if (p == NULL)
        p = getenv ("LANG");

      if (p != NULL)
        {
          p = g_strdup (p);
          if (strcmp (p, "C") == 0)
            SetThreadLocale (LOCALE_SYSTEM_DEFAULT);
          else
            {
              /* Check if one of the supported locales match the
               * environment variable. If so, use that locale.
               */
              iso639_to_check = p;
              iso3166_to_check = strchr (iso639_to_check, '_');
              if (iso3166_to_check != NULL)
                {
                  *iso3166_to_check++ = '\0';

                  script_to_check = strchr (iso3166_to_check, '@');
                  if (script_to_check != NULL)
                    *script_to_check++ = '\0';

                  /* Handle special cases. */

                  /* The standard code for Serbia and Montenegro was
                   * "CS", but MSFT uses for some reason "SP". By now
                   * (October 2006), SP has split into two, "RS" and
                   * "ME", but don't bother trying to handle those
                   * yet. Do handle the even older "YU", though.
                   */
                  if (strcmp (iso3166_to_check, "CS") == 0 ||
                      strcmp (iso3166_to_check, "YU") == 0)
                    iso3166_to_check = "SP";
                }
              else
                {
                  script_to_check = strchr (iso639_to_check, '@');
                  if (script_to_check != NULL)
                    *script_to_check++ = '\0';
                  /* LANG_SERBIAN == LANG_CROATIAN, recognize just "sr" */
                  if (strcmp (iso639_to_check, "sr") == 0)
                    iso3166_to_check = "SP";
                }

              EnumSystemLocales (enum_locale_proc, LCID_SUPPORTED);
            }
          g_free (p);
        }
      if (!setlocale_called)
        setlocale (LC_ALL, "");
#else
      if (!setlocale (LC_ALL, ""))
        g_warning ("Locale not supported by C library.\n\tUsing the fallback 'C' locale.");
#endif
    }
}

static void
do_pre_parse_initialization (int    *argc G_GNUC_UNUSED,
                             char ***argv G_GNUC_UNUSED)
{
  const gchar *env_string;
  double slowdown;
  
  if (pre_initialized)
    return;

  pre_initialized = TRUE;

  if (_ctk_module_has_mixed_deps (NULL))
    g_error ("CTK+ 2.x symbols detected. Using CTK+ 2.x and CTK+ 3 in the same process is not supported");

  CDK_PRIVATE_CALL (cdk_pre_parse) ();
  cdk_event_handler_set ((CdkEventFunc)ctk_main_do_event, NULL, NULL);

  env_string = g_getenv ("CTK_DEBUG");
  if (env_string != NULL)
    {
#ifdef G_ENABLE_DEBUG
      debug_flags[0].flags = g_parse_debug_string (env_string,
                                                   ctk_debug_keys,
                                                   G_N_ELEMENTS (ctk_debug_keys));
#else
      g_warning ("CTK_DEBUG set but ignored because ctk isn't built with G_ENABLE_DEBUG");
#endif  /* G_ENABLE_DEBUG */
      env_string = NULL;
    }

  env_string = g_getenv ("CTK3_MODULES");
  if (env_string)
    ctk_modules_string = g_string_new (env_string);

  env_string = g_getenv ("CTK_MODULES");
  if (env_string)
    {
      if (ctk_modules_string)
        g_string_append_c (ctk_modules_string, G_SEARCHPATH_SEPARATOR);
      else
        ctk_modules_string = g_string_new (NULL);

      g_string_append (ctk_modules_string, env_string);
    }

  env_string = g_getenv ("CTK_SLOWDOWN");
  if (env_string)
    {
      slowdown = g_ascii_strtod (env_string, NULL);
      _ctk_set_slowdown (slowdown);
    }
}

static void
gettext_initialization (void)
{
  setlocale_initialization ();

#ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, _ctk_get_localedir ());
  bindtextdomain (GETTEXT_PACKAGE "-properties", _ctk_get_localedir ());
#    ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  bind_textdomain_codeset (GETTEXT_PACKAGE "-properties", "UTF-8");
#    endif
#endif  
}

static void
default_display_notify_cb (CdkDisplayManager *dm G_GNUC_UNUSED)
{
  _ctk_accessibility_init ();
  debug_flags[0].display = cdk_display_get_default ();
}

static void
do_post_parse_initialization (int    *argc,
                              char ***argv)
{
  CdkDisplayManager *display_manager;

  if (ctk_initialized)
    return;

  gettext_initialization ();

#ifdef SIGPIPE
  signal (SIGPIPE, SIG_IGN);
#endif

  if (g_fatal_warnings)
    {
      GLogLevelFlags fatal_mask;

      fatal_mask = g_log_set_always_fatal (G_LOG_FATAL_MASK);
      fatal_mask |= G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL;
      g_log_set_always_fatal (fatal_mask);
    }

  if (debug_flags[0].flags & CTK_DEBUG_UPDATES)
    cdk_window_set_debug_updates (TRUE);

  ctk_widget_set_default_direction (ctk_get_locale_direction ());

  _ctk_ensure_resources ();

  _ctk_accel_map_init ();

  ctk_initialized = TRUE;

  if (ctk_modules_string)
    {
      _ctk_modules_init (argc, argv, ctk_modules_string->str);
      g_string_free (ctk_modules_string, TRUE);
    }
  else
    {
      _ctk_modules_init (argc, argv, NULL);
    }

  display_manager = cdk_display_manager_get ();
  if (cdk_display_manager_get_default_display (display_manager) != NULL)
    default_display_notify_cb (display_manager);

  g_signal_connect (display_manager, "notify::default-display",
                    G_CALLBACK (default_display_notify_cb),
                    NULL);
}


typedef struct
{
  gboolean open_default_display;
} OptionGroupInfo;

static gboolean
pre_parse_hook (GOptionContext *context G_GNUC_UNUSED,
                GOptionGroup   *group G_GNUC_UNUSED,
                gpointer        data G_GNUC_UNUSED,
                GError        **error G_GNUC_UNUSED)
{
  do_pre_parse_initialization (NULL, NULL);
  
  return TRUE;
}

static gboolean
post_parse_hook (GOptionContext *context G_GNUC_UNUSED,
                 GOptionGroup   *group G_GNUC_UNUSED,
                 gpointer       data,
                 GError        **error)
{
  OptionGroupInfo *info = data;

  
  do_post_parse_initialization (NULL, NULL);
  
  if (info->open_default_display)
    {
      if (CDK_PRIVATE_CALL (cdk_display_open_default) () == NULL)
        {
          const char *display_name = cdk_get_display_arg_name ();
          g_set_error (error,
                       G_OPTION_ERROR,
                       G_OPTION_ERROR_FAILED,
                       _("Cannot open display: %s"),
                       display_name ? display_name : "" );

          return FALSE;
        }

      if (ctk_get_debug_flags () & CTK_DEBUG_INTERACTIVE)
        ctk_window_set_interactive_debugging (TRUE);
    }

  return TRUE;
}

guint
ctk_get_display_debug_flags (CdkDisplay *display)
{
  gint i;

  for (i = 0; i < N_DEBUG_DISPLAYS; i++)
    {
      if (debug_flags[i].display == display)
        return debug_flags[i].flags;
    }

  return 0;
}

void
ctk_set_display_debug_flags (CdkDisplay *display,
                             guint       flags)
{
  gint i;

  for (i = 0; i < N_DEBUG_DISPLAYS; i++)
    {
      if (debug_flags[i].display == NULL)
        debug_flags[i].display = display;

      if (debug_flags[i].display == display)
        {
          debug_flags[i].flags = flags;
          return;
        }
    }
}

/**
 * ctk_get_debug_flags:
 *
 * Returns the CTK+ debug flags.
 *
 * This function is intended for CTK+ modules that want
 * to adjust their debug output based on CTK+ debug flags.
 *
 * Returns: the CTK+ debug flags.
 */
guint
ctk_get_debug_flags (void)
{
  return ctk_get_display_debug_flags (cdk_display_get_default ());
}

/**
 * ctk_set_debug_flags:
 *
 * Sets the CTK+ debug flags.
 */
void
ctk_set_debug_flags (guint flags)
{
  ctk_set_display_debug_flags (cdk_display_get_default (), flags);
}

gboolean
ctk_simulate_touchscreen (void)
{
  static gint test_touchscreen;

  if (test_touchscreen == 0)
    test_touchscreen = g_getenv ("CTK_TEST_TOUCHSCREEN") != NULL ? 1 : -1;

  return test_touchscreen > 0 || (ctk_get_debug_flags () & CTK_DEBUG_TOUCHSCREEN) != 0;
 }

/**
 * ctk_get_option_group:
 * @open_default_display: whether to open the default display
 *     when parsing the commandline arguments
 *
 * Returns a #GOptionGroup for the commandline arguments recognized
 * by CTK+ and CDK.
 *
 * You should add this group to your #GOptionContext
 * with g_option_context_add_group(), if you are using
 * g_option_context_parse() to parse your commandline arguments.
 *
 * Returns: (transfer full): a #GOptionGroup for the commandline
 *     arguments recognized by CTK+
 *
 * Since: 2.6
 */
GOptionGroup *
ctk_get_option_group (gboolean open_default_display)
{
  GOptionGroup *group;
  OptionGroupInfo *info;

  gettext_initialization ();

  info = g_new0 (OptionGroupInfo, 1);
  info->open_default_display = open_default_display;
  
  group = g_option_group_new ("ctk", _("CTK+ Options"), _("Show CTK+ Options"), info, g_free);
  g_option_group_set_parse_hooks (group, pre_parse_hook, post_parse_hook);

  CDK_PRIVATE_CALL (cdk_add_option_entries) (group);
  g_option_group_add_entries (group, ctk_args);
  g_option_group_set_translation_domain (group, GETTEXT_PACKAGE);
  
  return group;
}

/**
 * ctk_init_with_args:
 * @argc: (inout): Address of the `argc` parameter of
 *     your main() function (or 0 if @argv is %NULL). This will be changed if 
 *     any arguments were handled.
 * @argv: (array length=argc) (inout) (allow-none): Address of the
 *     `argv` parameter of main(), or %NULL. Any options
 *     understood by CTK+ are stripped before return.
 * @parameter_string: (allow-none): a string which is displayed in
 *    the first line of `--help` output, after
 *    `programname [OPTION...]`
 * @entries: (array zero-terminated=1): a %NULL-terminated array
 *    of #GOptionEntrys describing the options of your program
 * @translation_domain: (nullable): a translation domain to use for translating
 *    the `--help` output for the options in @entries
 *    and the @parameter_string with gettext(), or %NULL
 * @error: a return location for errors
 *
 * This function does the same work as ctk_init_check().
 * Additionally, it allows you to add your own commandline options,
 * and it automatically generates nicely formatted
 * `--help` output. Note that your program will
 * be terminated after writing out the help output.
 *
 * Returns: %TRUE if the commandline arguments (if any) were valid and
 *     if the windowing system has been successfully initialized,
 *     %FALSE otherwise
 *
 * Since: 2.6
 */
gboolean
ctk_init_with_args (gint                 *argc,
                    gchar              ***argv,
                    const gchar          *parameter_string,
                    const GOptionEntry   *entries,
                    const gchar          *translation_domain,
                    GError              **error)
{
  GOptionContext *context;
  GOptionGroup *ctk_group;
  gboolean retval;

  if (ctk_initialized)
    goto done;

  gettext_initialization ();

  if (!check_setugid ())
    return FALSE;

  ctk_group = ctk_get_option_group (FALSE);

  context = g_option_context_new (parameter_string);
  g_option_context_add_group (context, ctk_group);
  g_option_context_set_translation_domain (context, translation_domain);

  if (entries)
    g_option_context_add_main_entries (context, entries, translation_domain);
  retval = g_option_context_parse (context, argc, argv, error);

  g_option_context_free (context);

  if (!retval)
    return FALSE;

done:
  if (CDK_PRIVATE_CALL (cdk_display_open_default) () == NULL)
    {
      const char *display_name = cdk_get_display_arg_name ();
      g_set_error (error,
                   G_OPTION_ERROR,
                   G_OPTION_ERROR_FAILED,
                   _("Cannot open display: %s"),
                   display_name ? display_name : "" );

      return FALSE;
    }

  if (ctk_get_debug_flags () & CTK_DEBUG_INTERACTIVE)
    ctk_window_set_interactive_debugging (TRUE);

  return TRUE;
}


/**
 * ctk_parse_args:
 * @argc: (inout): a pointer to the number of command line arguments
 * @argv: (array length=argc) (inout): a pointer to the array of
 *     command line arguments
 *
 * Parses command line arguments, and initializes global
 * attributes of CTK+, but does not actually open a connection
 * to a display. (See cdk_display_open(), cdk_get_display_arg_name())
 *
 * Any arguments used by CTK+ or CDK are removed from the array and
 * @argc and @argv are updated accordingly.
 *
 * There is no need to call this function explicitly if you are using
 * ctk_init(), or ctk_init_check().
 *
 * Note that many aspects of CTK+ require a display connection to
 * function, so this way of initializing CTK+ is really only useful
 * for specialized use cases.
 *
 * Returns: %TRUE if initialization succeeded, otherwise %FALSE
 */
gboolean
ctk_parse_args (int    *argc,
                char ***argv)
{
  GOptionContext *option_context;
  GOptionGroup *ctk_group;
  GError *error = NULL;
  
  if (ctk_initialized)
    return TRUE;

  gettext_initialization ();

  if (!check_setugid ())
    return FALSE;

  option_context = g_option_context_new (NULL);
  g_option_context_set_ignore_unknown_options (option_context, TRUE);
  g_option_context_set_help_enabled (option_context, FALSE);
  ctk_group = ctk_get_option_group (FALSE);
  g_option_context_set_main_group (option_context, ctk_group);
  if (!g_option_context_parse (option_context, argc, argv, &error))
    {
      g_warning ("%s", error->message);
      g_error_free (error);
    }

  g_option_context_free (option_context);

  return TRUE;
}

#ifdef G_PLATFORM_WIN32
#undef ctk_init_check
#endif

/**
 * ctk_init_check:
 * @argc: (inout): Address of the `argc` parameter of
 *     your main() function (or 0 if @argv is %NULL). This will be changed if 
 *     any arguments were handled.
 * @argv: (array length=argc) (inout) (allow-none): Address of the
 *     `argv` parameter of main(), or %NULL. Any options
 *     understood by CTK+ are stripped before return.
 *
 * This function does the same work as ctk_init() with only a single
 * change: It does not terminate the program if the commandline
 * arguments couldn’t be parsed or the windowing system can’t be
 * initialized. Instead it returns %FALSE on failure.
 *
 * This way the application can fall back to some other means of
 * communication with the user - for example a curses or command line
 * interface.
 *
 * Note that calling any CTK function or instantiating any CTK type after
 * this function returns %FALSE results in undefined behavior.
 *
 * Returns: %TRUE if the commandline arguments (if any) were valid and
 *     the windowing system has been successfully initialized, %FALSE
 *     otherwise
 */
gboolean
ctk_init_check (int    *argc,
                char ***argv)
{
  gboolean ret;

  if (!ctk_parse_args (argc, argv))
    return FALSE;

  ret = CDK_PRIVATE_CALL (cdk_display_open_default) () != NULL;

  if (ctk_get_debug_flags () & CTK_DEBUG_INTERACTIVE)
    ctk_window_set_interactive_debugging (TRUE);

  return ret;
}

#ifdef G_PLATFORM_WIN32
#undef ctk_init
#endif

/**
 * ctk_init:
 * @argc: (inout): Address of the `argc` parameter of
 *     your main() function (or 0 if @argv is %NULL). This will be changed if 
 *     any arguments were handled.
 * @argv: (array length=argc) (inout) (allow-none): Address of the
 *     `argv` parameter of main(), or %NULL. Any options
 *     understood by CTK+ are stripped before return.
 *
 * Call this function before using any other CTK+ functions in your GUI
 * applications.  It will initialize everything needed to operate the
 * toolkit and parses some standard command line options.
 *
 * Although you are expected to pass the @argc, @argv parameters from main() to 
 * this function, it is possible to pass %NULL if @argv is not available or 
 * commandline handling is not required.
 *
 * @argc and @argv are adjusted accordingly so your own code will
 * never see those standard arguments.
 *
 * Note that there are some alternative ways to initialize CTK+:
 * if you are calling ctk_parse_args(), ctk_init_check(),
 * ctk_init_with_args() or g_option_context_parse() with
 * the option group returned by ctk_get_option_group(),
 * you don’t have to call ctk_init().
 *
 * And if you are using #CtkApplication, you don't have to call any of the
 * initialization functions either; the #CtkApplication::startup handler
 * does it for you.
 *
 * This function will terminate your program if it was unable to
 * initialize the windowing system for some reason. If you want
 * your program to fall back to a textual interface you want to
 * call ctk_init_check() instead.
 *
 * Since 2.18, CTK+ calls `signal (SIGPIPE, SIG_IGN)`
 * during initialization, to ignore SIGPIPE signals, since these are
 * almost never wanted in graphical applications. If you do need to
 * handle SIGPIPE for some reason, reset the handler after ctk_init(),
 * but notice that other libraries (e.g. libdbus or gvfs) might do
 * similar things.
 */
void
ctk_init (int *argc, char ***argv)
{
  if (!ctk_init_check (argc, argv))
    {
      const char *display_name_arg = cdk_get_display_arg_name ();
      if (display_name_arg == NULL)
        display_name_arg = getenv("DISPLAY");
      g_warning ("cannot open display: %s", display_name_arg ? display_name_arg : "");
      exit (1);
    }
}

#ifdef G_OS_WIN32

/* This is relevant when building with gcc for Windows (MinGW),
 * where we want to be struct packing compatible with MSVC,
 * i.e. use the -mms-bitfields switch.
 * For Cygwin there should be no need to be compatible with MSVC,
 * so no need to use G_PLATFORM_WIN32.
 */

static void
check_sizeof_CtkWindow (size_t sizeof_CtkWindow)
{
  if (sizeof_CtkWindow != sizeof (CtkWindow))
    g_error ("Incompatible build!\n"
             "The code using CTK+ thinks CtkWindow is of different\n"
             "size than it actually is in this build of CTK+.\n"
             "On Windows, this probably means that you have compiled\n"
             "your code with gcc without the -mms-bitfields switch,\n"
             "or that you are using an unsupported compiler.");
}

/* In CTK+ 2.0 the CtkWindow struct actually is the same size in
 * gcc-compiled code on Win32 whether compiled with -fnative-struct or
 * not. Unfortunately this wan’t noticed until after CTK+ 2.0.1. So,
 * from CTK+ 2.0.2 on, check some other struct, too, where the use of
 * -fnative-struct still matters. CtkBox is one such.
 */
static void
check_sizeof_CtkBox (size_t sizeof_CtkBox)
{
  if (sizeof_CtkBox != sizeof (CtkBox))
    g_error ("Incompatible build!\n"
             "The code using CTK+ thinks CtkBox is of different\n"
             "size than it actually is in this build of CTK+.\n"
             "On Windows, this probably means that you have compiled\n"
             "your code with gcc without the -mms-bitfields switch,\n"
             "or that you are using an unsupported compiler.");
}

/* These two functions might get more checks added later, thus pass
 * in the number of extra args.
 */
void
ctk_init_abi_check (int *argc, char ***argv, int num_checks, size_t sizeof_CtkWindow, size_t sizeof_CtkBox)
{
  check_sizeof_CtkWindow (sizeof_CtkWindow);
  if (num_checks >= 2)
    check_sizeof_CtkBox (sizeof_CtkBox);
  ctk_init (argc, argv);
}

gboolean
ctk_init_check_abi_check (int *argc, char ***argv, int num_checks, size_t sizeof_CtkWindow, size_t sizeof_CtkBox)
{
  check_sizeof_CtkWindow (sizeof_CtkWindow);
  if (num_checks >= 2)
    check_sizeof_CtkBox (sizeof_CtkBox);
  return ctk_init_check (argc, argv);
}

#endif

/**
 * ctk_get_locale_direction:
 *
 * Get the direction of the current locale. This is the expected
 * reading direction for text and UI.
 *
 * This function depends on the current locale being set with
 * setlocale() and will default to setting the %CTK_TEXT_DIR_LTR
 * direction otherwise. %CTK_TEXT_DIR_NONE will never be returned.
 *
 * CTK+ sets the default text direction according to the locale
 * during ctk_init(), and you should normally use
 * ctk_widget_get_direction() or ctk_widget_get_default_direction()
 * to obtain the current direcion.
 *
 * This function is only needed rare cases when the locale is
 * changed after CTK+ has already been initialized. In this case,
 * you can use it to update the default text direction as follows:
 *
 * |[<!-- language="C" -->
 * setlocale (LC_ALL, new_locale);
 * direction = ctk_get_locale_direction ();
 * ctk_widget_set_default_direction (direction);
 * ]|
 *
 * Returns: the #CtkTextDirection of the current locale
 *
 * Since: 3.12
 */
CtkTextDirection
ctk_get_locale_direction (void)
{
  /* Translate to default:RTL if you want your widgets
   * to be RTL, otherwise translate to default:LTR.
   * Do *not* translate it to "predefinito:LTR", if it
   * it isn't default:LTR or default:RTL it will not work
   */
  gchar            *e   = _("default:LTR");
  CtkTextDirection  dir = CTK_TEXT_DIR_LTR;

  if (g_strcmp0 (e, "default:RTL") == 0)
    dir = CTK_TEXT_DIR_RTL;
  else if (g_strcmp0 (e, "default:LTR") != 0)
    g_warning ("Whoever translated default:LTR did so wrongly. Defaulting to LTR.");

  return dir;
}

/**
 * ctk_get_default_language:
 *
 * Returns the #PangoLanguage for the default language currently in
 * effect. (Note that this can change over the life of an
 * application.) The default language is derived from the current
 * locale. It determines, for example, whether CTK+ uses the
 * right-to-left or left-to-right text direction.
 *
 * This function is equivalent to pango_language_get_default().
 * See that function for details.
 *
 * Returns: (transfer none): the default language as a #PangoLanguage,
 *     must not be freed
 */
PangoLanguage *
ctk_get_default_language (void)
{
  return pango_language_get_default ();
}

/**
 * ctk_main:
 *
 * Runs the main loop until ctk_main_quit() is called.
 *
 * You can nest calls to ctk_main(). In that case ctk_main_quit()
 * will make the innermost invocation of the main loop return.
 */
void
ctk_main (void)
{
  GMainLoop *loop;

  ctk_main_loop_level++;

  loop = g_main_loop_new (NULL, TRUE);
  main_loops = g_slist_prepend (main_loops, loop);

  if (g_main_loop_is_running (main_loops->data))
    {
      cdk_threads_leave ();
      g_main_loop_run (loop);
      cdk_threads_enter ();

      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      cdk_flush ();
      G_GNUC_END_IGNORE_DEPRECATIONS;
    }

  main_loops = g_slist_remove (main_loops, loop);

  g_main_loop_unref (loop);

  ctk_main_loop_level--;

  if (ctk_main_loop_level == 0)
    {
      /* Keep this section in sync with ctk_application_shutdown() */

      /* Try storing all clipboard data we have */
      _ctk_clipboard_store_all ();

      /* Synchronize the recent manager singleton */
      _ctk_recent_manager_sync ();
    }
}

/**
 * ctk_main_level:
 *
 * Asks for the current nesting level of the main loop.
 *
 * Returns: the nesting level of the current invocation
 *     of the main loop
 */
guint
ctk_main_level (void)
{
  return ctk_main_loop_level;
}

/**
 * ctk_main_quit:
 *
 * Makes the innermost invocation of the main loop return
 * when it regains control.
 */
void
ctk_main_quit (void)
{
  g_return_if_fail (main_loops != NULL);

  g_main_loop_quit (main_loops->data);
}

/**
 * ctk_events_pending:
 *
 * Checks if any events are pending.
 *
 * This can be used to update the UI and invoke timeouts etc.
 * while doing some time intensive computation.
 *
 * ## Updating the UI during a long computation
 *
 * |[<!-- language="C" -->
 *  // computation going on...
 *
 *  while (ctk_events_pending ())
 *    ctk_main_iteration ();
 *
 *  // ...computation continued
 * ]|
 *
 * Returns: %TRUE if any events are pending, %FALSE otherwise
 */
gboolean
ctk_events_pending (void)
{
  gboolean result;

  cdk_threads_leave ();
  result = g_main_context_pending (NULL);
  cdk_threads_enter ();

  return result;
}

/**
 * ctk_main_iteration:
 *
 * Runs a single iteration of the mainloop.
 *
 * If no events are waiting to be processed CTK+ will block
 * until the next event is noticed. If you don’t want to block
 * look at ctk_main_iteration_do() or check if any events are
 * pending with ctk_events_pending() first.
 *
 * Returns: %TRUE if ctk_main_quit() has been called for the
 *     innermost mainloop
 */
gboolean
ctk_main_iteration (void)
{
  cdk_threads_leave ();
  g_main_context_iteration (NULL, TRUE);
  cdk_threads_enter ();

  if (main_loops)
    return !g_main_loop_is_running (main_loops->data);
  else
    return TRUE;
}

/**
 * ctk_main_iteration_do:
 * @blocking: %TRUE if you want CTK+ to block if no events are pending
 *
 * Runs a single iteration of the mainloop.
 * If no events are available either return or block depending on
 * the value of @blocking.
 *
 * Returns: %TRUE if ctk_main_quit() has been called for the
 *     innermost mainloop
 */
gboolean
ctk_main_iteration_do (gboolean blocking)
{
  cdk_threads_leave ();
  g_main_context_iteration (NULL, blocking);
  cdk_threads_enter ();

  if (main_loops)
    return !g_main_loop_is_running (main_loops->data);
  else
    return TRUE;
}

static void
rewrite_events_translate (CdkWindow *old_window,
                          CdkWindow *new_window,
                          gdouble   *x,
                          gdouble   *y)
{
  gint old_origin_x, old_origin_y;
  gint new_origin_x, new_origin_y;

  cdk_window_get_origin (old_window, &old_origin_x, &old_origin_y);
  cdk_window_get_origin (new_window, &new_origin_x, &new_origin_y);

  *x += old_origin_x - new_origin_x;
  *y += old_origin_y - new_origin_y;
}

static CdkEvent *
rewrite_event_for_window (CdkEvent  *event,
                          CdkWindow *new_window)
{
  event = cdk_event_copy (event);

  switch (event->type)
    {
    case CDK_SCROLL:
      rewrite_events_translate (event->any.window,
                                new_window,
                                &event->scroll.x, &event->scroll.y);
      break;
    case CDK_BUTTON_PRESS:
    case CDK_2BUTTON_PRESS:
    case CDK_3BUTTON_PRESS:
    case CDK_BUTTON_RELEASE:
      rewrite_events_translate (event->any.window,
                                new_window,
                                &event->button.x, &event->button.y);
      break;
    case CDK_MOTION_NOTIFY:
      rewrite_events_translate (event->any.window,
                                new_window,
                                &event->motion.x, &event->motion.y);
      break;
    case CDK_TOUCH_BEGIN:
    case CDK_TOUCH_UPDATE:
    case CDK_TOUCH_END:
    case CDK_TOUCH_CANCEL:
      rewrite_events_translate (event->any.window,
                                new_window,
                                &event->touch.x, &event->touch.y);
      break;
    case CDK_TOUCHPAD_SWIPE:
      rewrite_events_translate (event->any.window,
                                new_window,
                                &event->touchpad_swipe.x,
                                &event->touchpad_swipe.y);
      break;
    case CDK_TOUCHPAD_PINCH:
      rewrite_events_translate (event->any.window,
                                new_window,
                                &event->touchpad_pinch.x,
                                &event->touchpad_pinch.y);
      break;
    case CDK_KEY_PRESS:
    case CDK_KEY_RELEASE:
    case CDK_PROXIMITY_IN:
    case CDK_PROXIMITY_OUT:
      break;

    default:
      return event;
    }

  g_object_unref (event->any.window);
  event->any.window = g_object_ref (new_window);

  return event;
}

/* If there is a pointer or keyboard grab in effect with owner_events = TRUE,
 * then what X11 does is deliver the event normally if it was going to this
 * client, otherwise, delivers it in terms of the grab window. This function
 * rewrites events to the effect that events going to the same window group
 * are delivered normally, otherwise, the event is delivered in terms of the
 * grab window.
 */
static CdkEvent *
rewrite_event_for_grabs (CdkEvent *event)
{
  CdkWindow *grab_window;
  CtkWidget *event_widget, *grab_widget;
  gpointer grab_widget_ptr;
  gboolean owner_events;
  CdkDisplay *display;
  CdkDevice *device;

  switch (event->type)
    {
    case CDK_SCROLL:
    case CDK_BUTTON_PRESS:
    case CDK_2BUTTON_PRESS:
    case CDK_3BUTTON_PRESS:
    case CDK_BUTTON_RELEASE:
    case CDK_MOTION_NOTIFY:
    case CDK_PROXIMITY_IN:
    case CDK_PROXIMITY_OUT:
    case CDK_KEY_PRESS:
    case CDK_KEY_RELEASE:
    case CDK_TOUCH_BEGIN:
    case CDK_TOUCH_UPDATE:
    case CDK_TOUCH_END:
    case CDK_TOUCH_CANCEL:
    case CDK_TOUCHPAD_SWIPE:
    case CDK_TOUCHPAD_PINCH:
      display = cdk_window_get_display (event->any.window);
      device = cdk_event_get_device (event);

      if (!CDK_PRIVATE_CALL (cdk_device_grab_info) (display, device, &grab_window, &owner_events) ||
          !owner_events)
        return NULL;
      break;
    default:
      return NULL;
    }

  event_widget = ctk_get_event_widget (event);
  cdk_window_get_user_data (grab_window, &grab_widget_ptr);
  grab_widget = grab_widget_ptr;

  if (grab_widget &&
      ctk_main_get_window_group (grab_widget) != ctk_main_get_window_group (event_widget))
    return rewrite_event_for_window (event, grab_window);
  else
    return NULL;
}

static CtkWidget *
widget_get_popover_ancestor (CtkWidget *widget,
                             CtkWindow *window)
{
  CtkWidget *parent = ctk_widget_get_parent (widget);

  while (parent && parent != CTK_WIDGET (window))
    {
      widget = parent;
      parent = ctk_widget_get_parent (widget);
    }

  if (!parent || parent != CTK_WIDGET (window))
    return NULL;

  if (_ctk_window_is_popover_widget (CTK_WINDOW (window), widget))
    return widget;

  return NULL;
}

static gboolean
check_event_in_child_popover (CtkWidget *event_widget,
                              CtkWidget *grab_widget)
{
  CtkWidget *window, *popover = NULL, *popover_parent = NULL;

  if (grab_widget == event_widget)
    return FALSE;

  window = ctk_widget_get_ancestor (event_widget, CTK_TYPE_WINDOW);

  if (!window)
    return FALSE;

  popover = widget_get_popover_ancestor (event_widget, CTK_WINDOW (window));

  if (!popover)
    return FALSE;

  popover_parent = _ctk_window_get_popover_parent (CTK_WINDOW (window), popover);

  if (!popover_parent)
    return FALSE;

  return (popover_parent == grab_widget || ctk_widget_is_ancestor (popover_parent, grab_widget));
}

/**
 * ctk_main_do_event:
 * @event: An event to process (normally passed by CDK)
 *
 * Processes a single CDK event.
 *
 * This is public only to allow filtering of events between CDK and CTK+.
 * You will not usually need to call this function directly.
 *
 * While you should not call this function directly, you might want to
 * know how exactly events are handled. So here is what this function
 * does with the event:
 *
 * 1. Compress enter/leave notify events. If the event passed build an
 *    enter/leave pair together with the next event (peeked from CDK), both
 *    events are thrown away. This is to avoid a backlog of (de-)highlighting
 *    widgets crossed by the pointer.
 * 
 * 2. Find the widget which got the event. If the widget can’t be determined
 *    the event is thrown away unless it belongs to a INCR transaction.
 *
 * 3. Then the event is pushed onto a stack so you can query the currently
 *    handled event with ctk_get_current_event().
 * 
 * 4. The event is sent to a widget. If a grab is active all events for widgets
 *    that are not in the contained in the grab widget are sent to the latter
 *    with a few exceptions:
 *    - Deletion and destruction events are still sent to the event widget for
 *      obvious reasons.
 *    - Events which directly relate to the visual representation of the event
 *      widget.
 *    - Leave events are delivered to the event widget if there was an enter
 *      event delivered to it before without the paired leave event.
 *    - Drag events are not redirected because it is unclear what the semantics
 *      of that would be.
 *    Another point of interest might be that all key events are first passed
 *    through the key snooper functions if there are any. Read the description
 *    of ctk_key_snooper_install() if you need this feature.
 * 
 * 5. After finishing the delivery the event is popped from the event stack.
 */
void
ctk_main_do_event (CdkEvent *event)
{
  CtkWidget *event_widget;
  CtkWidget *grab_widget = NULL;
  CtkWidget *topmost_widget = NULL;
  CtkWindowGroup *window_group;
  CdkEvent *rewritten_event = NULL;
  CdkDevice *device;
  GList *tmp_list;

  if (event->type == CDK_SETTING)
    {
      _ctk_settings_handle_event (&event->setting);
      return;
    }

  if (event->type == CDK_OWNER_CHANGE)
    {
      _ctk_clipboard_handle_event (&event->owner_change);
      return;
    }

  /* Find the widget which got the event. We store the widget
   * in the user_data field of CdkWindow's. Ignore the event
   * if we don't have a widget for it, except for CDK_PROPERTY_NOTIFY
   * events which are handled specially. Though this happens rarely,
   * bogus events can occur for e.g. destroyed CdkWindows.
   */
  event_widget = ctk_get_event_widget (event);
  if (!event_widget)
    {
      /* To handle selection INCR transactions, we select
       * PropertyNotify events on the requestor window and create
       * a corresponding (fake) CdkWindow so that events get here.
       * There won't be a widget though, so we have to handle
       * them specially
       */
      if (event->type == CDK_PROPERTY_NOTIFY)
        _ctk_selection_incr_event (event->any.window,
                                   &event->property);

      return;
    }

  /* If pointer or keyboard grabs are in effect, munge the events
   * so that each window group looks like a separate app.
   */
  rewritten_event = rewrite_event_for_grabs (event);
  if (rewritten_event)
    {
      event = rewritten_event;
      event_widget = ctk_get_event_widget (event);
    }

  /* Push the event onto a stack of current events for
   * ctk_current_event_get().
   */
  current_events = g_list_prepend (current_events, event);

  window_group = ctk_main_get_window_group (event_widget);
  device = cdk_event_get_device (event);

  /* check whether there is a (device) grab in effect... */
  if (device)
    grab_widget = ctk_window_group_get_current_device_grab (window_group, device);

  if (!grab_widget)
    grab_widget = ctk_window_group_get_current_grab (window_group);

  if (CTK_IS_WINDOW (event_widget) ||
      (grab_widget && grab_widget != event_widget &&
       !ctk_widget_is_ancestor (event_widget, grab_widget)))
    {
      /* Ignore event if we got a grab on another toplevel */
      if (!grab_widget ||
          ctk_widget_get_toplevel (event_widget) == ctk_widget_get_toplevel (grab_widget))
        {
          if (_ctk_window_check_handle_wm_event (event))
            goto cleanup;
        }
    }

  /* Find out the topmost widget where captured event propagation
   * should start, which is the widget holding the CTK+ grab
   * if any, otherwise it's left NULL and events are emitted
   * from the toplevel (or topmost parentless parent).
   */
  if (grab_widget)
    topmost_widget = grab_widget;

  /* If the grab widget is an ancestor of the event widget
   * then we send the event to the original event widget.
   * This is the key to implementing modality.
   */
  if (!grab_widget ||
      ((ctk_widget_is_sensitive (event_widget) || event->type == CDK_SCROLL) &&
       ctk_widget_is_ancestor (event_widget, grab_widget)))
    grab_widget = event_widget;

  /* popovers are not really a "child" of their "parent" in the widget/window
   * hierarchy sense, we however want to interact with popovers spawn by widgets
   * within grab_widget. If this is the case, we let the event go through
   * unaffected by the grab.
   */
  if (check_event_in_child_popover (event_widget, grab_widget))
    grab_widget = event_widget;

  /* If the widget receiving events is actually blocked by another
   * device CTK+ grab
   */
  if (device &&
      _ctk_window_group_widget_is_blocked_for_device (window_group, grab_widget, device))
    goto cleanup;

  /* Not all events get sent to the grabbing widget.
   * The delete, destroy, expose, focus change and resize
   * events still get sent to the event widget because
   * 1) these events have no meaning for the grabbing widget
   * and 2) redirecting these events to the grabbing widget
   * could cause the display to be messed up.
   *
   * Drag events are also not redirected, since it isn't
   * clear what the semantics of that would be.
   */
  switch (event->type)
    {
    case CDK_NOTHING:
      break;

    case CDK_DELETE:
      g_object_ref (event_widget);
      if ((!ctk_window_group_get_current_grab (window_group) || ctk_widget_get_toplevel (ctk_window_group_get_current_grab (window_group)) == event_widget) &&
          !ctk_widget_event (event_widget, event))
        ctk_widget_destroy (event_widget);
      g_object_unref (event_widget);
      break;

    case CDK_DESTROY:
      /* Unexpected CDK_DESTROY from the outside, ignore for
       * child windows, handle like a CDK_DELETE for toplevels
       */
      if (!ctk_widget_get_parent (event_widget))
        {
          g_object_ref (event_widget);
          if (!ctk_widget_event (event_widget, event) &&
              ctk_widget_get_realized (event_widget))
            ctk_widget_destroy (event_widget);
          g_object_unref (event_widget);
        }
      break;

    case CDK_EXPOSE:
      if (event->any.window)
        ctk_widget_render (event_widget, event->any.window, event->expose.region);
      break;

    case CDK_PROPERTY_NOTIFY:
    case CDK_FOCUS_CHANGE:
    case CDK_CONFIGURE:
    case CDK_MAP:
    case CDK_UNMAP:
    case CDK_SELECTION_CLEAR:
    case CDK_SELECTION_REQUEST:
    case CDK_SELECTION_NOTIFY:
    case CDK_CLIENT_EVENT:
    case CDK_VISIBILITY_NOTIFY:
    case CDK_WINDOW_STATE:
    case CDK_GRAB_BROKEN:
    case CDK_DAMAGE:
      if (!_ctk_widget_captured_event (event_widget, event))
        ctk_widget_event (event_widget, event);
      break;

    case CDK_KEY_PRESS:
    case CDK_KEY_RELEASE:
      if (ctk_invoke_key_snoopers (grab_widget, event))
        break;

      /* make focus visible in a window that receives a key event */
      {
        CtkWidget *window;

        window = ctk_widget_get_toplevel (grab_widget);
        if (CTK_IS_WINDOW (window))
          ctk_window_set_focus_visible (CTK_WINDOW (window), TRUE);
      }

      /* Catch alt press to enable auto-mnemonics;
       * menus are handled elsewhere
       * FIXME: this does not work with mnemonic modifiers other than Alt
       */
      if ((event->key.keyval == CDK_KEY_Alt_L || event->key.keyval == CDK_KEY_Alt_R) &&
          ((event->key.state & (ctk_accelerator_get_default_mod_mask ()) & ~(CDK_RELEASE_MASK|CDK_MOD1_MASK)) == 0) &&
          !CTK_IS_MENU_SHELL (grab_widget))
        {
          gboolean mnemonics_visible;
          CtkWidget *window;

          mnemonics_visible = (event->type == CDK_KEY_PRESS);

          window = ctk_widget_get_toplevel (grab_widget);
          if (CTK_IS_WINDOW (window))
            {
              if (mnemonics_visible)
                _ctk_window_schedule_mnemonics_visible (CTK_WINDOW (window));
              else
                ctk_window_set_mnemonics_visible (CTK_WINDOW (window), FALSE);
            }
        }
      /* else fall through */
    case CDK_SCROLL:
    case CDK_BUTTON_PRESS:
    case CDK_2BUTTON_PRESS:
    case CDK_3BUTTON_PRESS:
    case CDK_TOUCH_BEGIN:
    case CDK_MOTION_NOTIFY:
    case CDK_BUTTON_RELEASE:
    case CDK_PROXIMITY_IN:
    case CDK_PROXIMITY_OUT:
    case CDK_TOUCH_UPDATE:
    case CDK_TOUCH_END:
    case CDK_TOUCH_CANCEL:
    case CDK_TOUCHPAD_SWIPE:
    case CDK_TOUCHPAD_PINCH:
    case CDK_PAD_BUTTON_PRESS:
    case CDK_PAD_BUTTON_RELEASE:
    case CDK_PAD_RING:
    case CDK_PAD_STRIP:
    case CDK_PAD_GROUP_MODE:
      if (!_ctk_propagate_captured_event (grab_widget, event, topmost_widget))
        ctk_propagate_event (grab_widget, event);
      break;

    case CDK_ENTER_NOTIFY:
    case CDK_LEAVE_NOTIFY:
      if (ctk_widget_is_sensitive (grab_widget) &&
          !_ctk_propagate_captured_event (grab_widget, event, topmost_widget))
        ctk_widget_event (grab_widget, event);
      break;

    case CDK_DRAG_STATUS:
    case CDK_DROP_FINISHED:
      _ctk_drag_source_handle_event (event_widget, event);
      break;
    case CDK_DRAG_ENTER:
    case CDK_DRAG_LEAVE:
    case CDK_DRAG_MOTION:
    case CDK_DROP_START:
      _ctk_drag_dest_handle_event (event_widget, event);
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  if (event->type == CDK_ENTER_NOTIFY
      || event->type == CDK_LEAVE_NOTIFY
      || event->type == CDK_BUTTON_PRESS
      || event->type == CDK_2BUTTON_PRESS
      || event->type == CDK_3BUTTON_PRESS
      || event->type == CDK_KEY_PRESS
      || event->type == CDK_DRAG_ENTER
      || event->type == CDK_GRAB_BROKEN
      || event->type == CDK_MOTION_NOTIFY
      || event->type == CDK_TOUCH_UPDATE
      || event->type == CDK_SCROLL)
    {
      _ctk_tooltip_handle_event (event);
    }

 cleanup:
  tmp_list = current_events;
  current_events = g_list_remove_link (current_events, tmp_list);
  g_list_free_1 (tmp_list);

  if (rewritten_event)
    cdk_event_free (rewritten_event);
}

/**
 * ctk_true:
 *
 * All this function does it to return %TRUE.
 *
 * This can be useful for example if you want to inhibit the deletion
 * of a window. Of course you should not do this as the user expects
 * a reaction from clicking the close icon of the window...
 *
 * ## A persistent window
 *
 * |[<!-- language="C" -->
 * #include <ctk/ctk.h>
 *
 * int
 * main (int argc, char **argv)
 * {
 *   CtkWidget *win, *but;
 *   const char *text = "Close yourself. I mean it!";
 *
 *   ctk_init (&argc, &argv);
 *
 *   win = ctk_window_new (CTK_WINDOW_TOPLEVEL);
 *   g_signal_connect (win,
 *                     "delete-event",
 *                     G_CALLBACK (ctk_true),
 *                     NULL);
 *   g_signal_connect (win, "destroy",
 *                     G_CALLBACK (ctk_main_quit),
 *                     NULL);
 *
 *   but = ctk_button_new_with_label (text);
 *   g_signal_connect_swapped (but, "clicked",
 *                             G_CALLBACK (ctk_object_destroy),
 *                             win);
 *   ctk_container_add (CTK_CONTAINER (win), but);
 *
 *   ctk_widget_show_all (win);
 *
 *   ctk_main ();
 *
 *   return 0;
 * }
 * ]|
 *
 * Returns: %TRUE
 */
gboolean
ctk_true (void)
{
  return TRUE;
}

/**
 * ctk_false:
 *
 * Analogical to ctk_true(), this function does nothing
 * but always returns %FALSE.
 *
 * Returns: %FALSE
 */
gboolean
ctk_false (void)
{
  return FALSE;
}

static CtkWindowGroup *
ctk_main_get_window_group (CtkWidget *widget)
{
  CtkWidget *toplevel = NULL;

  if (widget)
    toplevel = ctk_widget_get_toplevel (widget);

  if (CTK_IS_WINDOW (toplevel))
    return ctk_window_get_group (CTK_WINDOW (toplevel));
  else
    return ctk_window_get_group (NULL);
}

typedef struct
{
  CtkWidget *old_grab_widget;
  CtkWidget *new_grab_widget;
  gboolean   was_grabbed;
  gboolean   is_grabbed;
  gboolean   from_grab;
  GList     *notified_windows;
  CdkDevice *device;
} GrabNotifyInfo;

static void
synth_crossing_for_grab_notify (CtkWidget       *from,
                                CtkWidget       *to,
                                GrabNotifyInfo  *info,
                                GList           *devices,
                                CdkCrossingMode  mode)
{
  while (devices)
    {
      CdkDevice *device = devices->data;
      CdkWindow *from_window, *to_window;

      /* Do not propagate events more than once to
       * the same windows if non-multidevice aware.
       */
      if (!from)
        from_window = NULL;
      else
        {
          from_window = _ctk_widget_get_device_window (from, device);

          if (from_window &&
              !cdk_window_get_support_multidevice (from_window) &&
              g_list_find (info->notified_windows, from_window))
            from_window = NULL;
        }

      if (!to)
        to_window = NULL;
      else
        {
          to_window = _ctk_widget_get_device_window (to, device);

          if (to_window &&
              !cdk_window_get_support_multidevice (to_window) &&
              g_list_find (info->notified_windows, to_window))
            to_window = NULL;
        }

      if (from_window || to_window)
        {
          _ctk_widget_synthesize_crossing ((from_window) ? from : NULL,
                                           (to_window) ? to : NULL,
                                           device, mode);

          if (from_window)
            info->notified_windows = g_list_prepend (info->notified_windows, from_window);

          if (to_window)
            info->notified_windows = g_list_prepend (info->notified_windows, to_window);
        }

      devices = devices->next;
    }
}

static void
ctk_grab_notify_foreach (CtkWidget *child,
                         gpointer   data)
{
  GrabNotifyInfo *info = data;
  gboolean was_grabbed, is_grabbed, was_shadowed, is_shadowed;
  GList *devices;

  was_grabbed = info->was_grabbed;
  is_grabbed = info->is_grabbed;

  info->was_grabbed = info->was_grabbed || (child == info->old_grab_widget);
  info->is_grabbed = info->is_grabbed || (child == info->new_grab_widget);

  was_shadowed = info->old_grab_widget && !info->was_grabbed;
  is_shadowed = info->new_grab_widget && !info->is_grabbed;

  g_object_ref (child);

  if ((was_shadowed || is_shadowed) && CTK_IS_CONTAINER (child))
    ctk_container_forall (CTK_CONTAINER (child), ctk_grab_notify_foreach, info);

  if (info->device &&
      _ctk_widget_get_device_window (child, info->device))
    {
      /* Device specified and is on widget */
      devices = g_list_prepend (NULL, info->device);
    }
  else
    devices = _ctk_widget_list_devices (child);

  if (is_shadowed)
    {
      _ctk_widget_set_shadowed (child, TRUE);
      if (!was_shadowed && devices &&
          ctk_widget_is_sensitive (child))
        synth_crossing_for_grab_notify (child, info->new_grab_widget,
                                        info, devices,
                                        CDK_CROSSING_CTK_GRAB);
    }
  else
    {
      _ctk_widget_set_shadowed (child, FALSE);
      if (was_shadowed && devices &&
          ctk_widget_is_sensitive (child))
        synth_crossing_for_grab_notify (info->old_grab_widget, child,
                                        info, devices,
                                        info->from_grab ? CDK_CROSSING_CTK_GRAB :
                                        CDK_CROSSING_CTK_UNGRAB);
    }

  if (was_shadowed != is_shadowed)
    _ctk_widget_grab_notify (child, was_shadowed);

  g_object_unref (child);
  g_list_free (devices);

  info->was_grabbed = was_grabbed;
  info->is_grabbed = is_grabbed;
}

static void
ctk_grab_notify (CtkWindowGroup *group,
                 CdkDevice      *device,
                 CtkWidget      *old_grab_widget,
                 CtkWidget      *new_grab_widget,
                 gboolean        from_grab)
{
  GList *toplevels;
  GrabNotifyInfo info = { 0 };

  if (old_grab_widget == new_grab_widget)
    return;

  info.old_grab_widget = old_grab_widget;
  info.new_grab_widget = new_grab_widget;
  info.from_grab = from_grab;
  info.device = device;

  g_object_ref (group);

  toplevels = ctk_window_list_toplevels ();
  g_list_foreach (toplevels, (GFunc)g_object_ref, NULL);

  while (toplevels)
    {
      CtkWindow *toplevel = toplevels->data;
      toplevels = g_list_delete_link (toplevels, toplevels);

      info.was_grabbed = FALSE;
      info.is_grabbed = FALSE;

      if (group == ctk_window_get_group (toplevel))
        ctk_grab_notify_foreach (CTK_WIDGET (toplevel), &info);
      g_object_unref (toplevel);
    }

  g_list_free (info.notified_windows);
  g_object_unref (group);
}

/**
 * ctk_grab_add: (method)
 * @widget: The widget that grabs keyboard and pointer events
 *
 * Makes @widget the current grabbed widget.
 *
 * This means that interaction with other widgets in the same
 * application is blocked and mouse as well as keyboard events
 * are delivered to this widget.
 *
 * If @widget is not sensitive, it is not set as the current
 * grabbed widget and this function does nothing.
 */
void
ctk_grab_add (CtkWidget *widget)
{
  CtkWindowGroup *group;
  CtkWidget *old_grab_widget;
  CtkWidget *toplevel;

  g_return_if_fail (widget != NULL);

  toplevel = ctk_widget_get_toplevel (widget);
  if (toplevel && cdk_window_get_window_type (ctk_widget_get_window (toplevel)) == CDK_WINDOW_OFFSCREEN)
    return;

  if (!ctk_widget_has_grab (widget) && ctk_widget_is_sensitive (widget))
    {
      _ctk_widget_set_has_grab (widget, TRUE);

      group = ctk_main_get_window_group (widget);

      old_grab_widget = ctk_window_group_get_current_grab (group);

      g_object_ref (widget);
      _ctk_window_group_add_grab (group, widget);

      ctk_grab_notify (group, NULL, old_grab_widget, widget, TRUE);
    }
}

/**
 * ctk_grab_get_current:
 *
 * Queries the current grab of the default window group.
 *
 * Returns: (transfer none) (nullable): The widget which currently
 *     has the grab or %NULL if no grab is active
 */
CtkWidget*
ctk_grab_get_current (void)
{
  CtkWindowGroup *group;

  group = ctk_main_get_window_group (NULL);

  return ctk_window_group_get_current_grab (group);
}

/**
 * ctk_grab_remove: (method)
 * @widget: The widget which gives up the grab
 *
 * Removes the grab from the given widget.
 *
 * You have to pair calls to ctk_grab_add() and ctk_grab_remove().
 *
 * If @widget does not have the grab, this function does nothing.
 */
void
ctk_grab_remove (CtkWidget *widget)
{
  CtkWindowGroup *group;
  CtkWidget *new_grab_widget;

  g_return_if_fail (widget != NULL);

  if (ctk_widget_has_grab (widget))
    {
      _ctk_widget_set_has_grab (widget, FALSE);

      group = ctk_main_get_window_group (widget);
      _ctk_window_group_remove_grab (group, widget);
      new_grab_widget = ctk_window_group_get_current_grab (group);

      ctk_grab_notify (group, NULL, widget, new_grab_widget, FALSE);

      g_object_unref (widget);
    }
}

/**
 * ctk_device_grab_add:
 * @widget: a #CtkWidget
 * @device: a #CdkDevice to grab on.
 * @block_others: %TRUE to prevent other devices to interact with @widget.
 *
 * Adds a CTK+ grab on @device, so all the events on @device and its
 * associated pointer or keyboard (if any) are delivered to @widget.
 * If the @block_others parameter is %TRUE, any other devices will be
 * unable to interact with @widget during the grab.
 *
 * Since: 3.0
 */
void
ctk_device_grab_add (CtkWidget *widget,
                     CdkDevice *device,
                     gboolean   block_others)
{
  CtkWindowGroup *group;
  CtkWidget *old_grab_widget;
  CdkWindow *toplevel;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (CDK_IS_DEVICE (device));

  toplevel = cdk_window_get_toplevel (ctk_widget_get_window (widget));
  if (toplevel && cdk_window_get_window_type (toplevel) == CDK_WINDOW_OFFSCREEN)
    return;

  group = ctk_main_get_window_group (widget);
  old_grab_widget = ctk_window_group_get_current_device_grab (group, device);

  if (old_grab_widget != widget)
    _ctk_window_group_add_device_grab (group, widget, device, block_others);

  ctk_grab_notify (group, device, old_grab_widget, widget, TRUE);
}

/**
 * ctk_device_grab_remove:
 * @widget: a #CtkWidget
 * @device: a #CdkDevice
 *
 * Removes a device grab from the given widget.
 *
 * You have to pair calls to ctk_device_grab_add() and
 * ctk_device_grab_remove().
 *
 * Since: 3.0
 */
void
ctk_device_grab_remove (CtkWidget *widget,
                        CdkDevice *device)
{
  CtkWindowGroup *group;
  CtkWidget *new_grab_widget;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (CDK_IS_DEVICE (device));

  group = ctk_main_get_window_group (widget);
  _ctk_window_group_remove_device_grab (group, widget, device);
  new_grab_widget = ctk_window_group_get_current_device_grab (group, device);

  ctk_grab_notify (group, device, widget, new_grab_widget, FALSE);
}

/**
 * ctk_key_snooper_install: (skip)
 * @snooper: a #CtkKeySnoopFunc
 * @func_data: (closure): data to pass to @snooper
 *
 * Installs a key snooper function, which will get called on all
 * key events before delivering them normally.
 *
 * Returns: a unique id for this key snooper for use with
 *    ctk_key_snooper_remove().
 *
 * Deprecated: 3.4: Key snooping should not be done. Events should
 *     be handled by widgets.
 */
guint
ctk_key_snooper_install (CtkKeySnoopFunc snooper,
                         gpointer        func_data)
{
  CtkKeySnooperData *data;
  static guint snooper_id = 1;

  g_return_val_if_fail (snooper != NULL, 0);

  data = g_new (CtkKeySnooperData, 1);
  data->func = snooper;
  data->func_data = func_data;
  data->id = snooper_id++;
  key_snoopers = g_slist_prepend (key_snoopers, data);

  return data->id;
}

/**
 * ctk_key_snooper_remove:
 * @snooper_handler_id: Identifies the key snooper to remove
 *
 * Removes the key snooper function with the given id.
 *
 * Deprecated: 3.4: Key snooping should not be done. Events should
 *     be handled by widgets.
 */
void
ctk_key_snooper_remove (guint snooper_id)
{
  CtkKeySnooperData *data = NULL;
  GSList *slist;

  slist = key_snoopers;
  while (slist)
    {
      data = slist->data;
      if (data->id == snooper_id)
        break;

      slist = slist->next;
      data = NULL;
    }
  if (data)
    {
      key_snoopers = g_slist_remove (key_snoopers, data);
      g_free (data);
    }
}

static gint
ctk_invoke_key_snoopers (CtkWidget *grab_widget,
                         CdkEvent  *event)
{
  GSList *slist;
  gint return_val = FALSE;

  return_val = _ctk_accessibility_key_snooper (grab_widget, (CdkEventKey *) event);

  slist = key_snoopers;
  while (slist && !return_val)
    {
      CtkKeySnooperData *data;

      data = slist->data;
      slist = slist->next;
      return_val = (*data->func) (grab_widget, (CdkEventKey*) event, data->func_data);
    }

  return return_val;
}

/**
 * ctk_get_current_event:
 *
 * Obtains a copy of the event currently being processed by CTK+.
 *
 * For example, if you are handling a #CtkButton::clicked signal,
 * the current event will be the #CdkEventButton that triggered
 * the ::clicked signal.
 *
 * Returns: (transfer full) (nullable): a copy of the current event, or
 *     %NULL if there is no current event. The returned event must be
 *     freed with cdk_event_free().
 */
CdkEvent*
ctk_get_current_event (void)
{
  if (current_events)
    return cdk_event_copy (current_events->data);
  else
    return NULL;
}

/**
 * ctk_get_current_event_time:
 *
 * If there is a current event and it has a timestamp,
 * return that timestamp, otherwise return %CDK_CURRENT_TIME.
 *
 * Returns: the timestamp from the current event,
 *     or %CDK_CURRENT_TIME.
 */
guint32
ctk_get_current_event_time (void)
{
  if (current_events)
    return cdk_event_get_time (current_events->data);
  else
    return CDK_CURRENT_TIME;
}

/**
 * ctk_get_current_event_state:
 * @state: (out): a location to store the state of the current event
 *
 * If there is a current event and it has a state field, place
 * that state field in @state and return %TRUE, otherwise return
 * %FALSE.
 *
 * Returns: %TRUE if there was a current event and it
 *     had a state field
 */
gboolean
ctk_get_current_event_state (CdkModifierType *state)
{
  g_return_val_if_fail (state != NULL, FALSE);

  if (current_events)
    return cdk_event_get_state (current_events->data, state);
  else
    {
      *state = 0;
      return FALSE;
    }
}

/**
 * ctk_get_current_event_device:
 *
 * If there is a current event and it has a device, return that
 * device, otherwise return %NULL.
 *
 * Returns: (transfer none) (nullable): a #CdkDevice, or %NULL
 */
CdkDevice *
ctk_get_current_event_device (void)
{
  if (current_events)
    return cdk_event_get_device (current_events->data);
  else
    return NULL;
}

/**
 * ctk_get_event_widget:
 * @event: a #CdkEvent
 *
 * If @event is %NULL or the event was not associated with any widget,
 * returns %NULL, otherwise returns the widget that received the event
 * originally.
 *
 * Returns: (transfer none) (nullable): the widget that originally
 *     received @event, or %NULL
 */
CtkWidget*
ctk_get_event_widget (CdkEvent *event)
{
  CtkWidget *widget;
  gpointer widget_ptr;

  widget = NULL;
  if (event && event->any.window &&
      (event->type == CDK_DESTROY || !cdk_window_is_destroyed (event->any.window)))
    {
      cdk_window_get_user_data (event->any.window, &widget_ptr);
      widget = widget_ptr;
    }

  return widget;
}

static gboolean
propagate_event_up (CtkWidget *widget,
                    CdkEvent  *event,
                    CtkWidget *topmost)
{
  gboolean handled_event = FALSE;

  /* Propagate event up the widget tree so that
   * parents can see the button and motion
   * events of the children.
   */
  while (TRUE)
    {
      CtkWidget *tmp;

      g_object_ref (widget);

      /* Scroll events are special cased here because it
       * feels wrong when scrolling a CtkViewport, say,
       * to have children of the viewport eat the scroll
       * event
       */
      if (!ctk_widget_is_sensitive (widget))
        handled_event = event->type != CDK_SCROLL;
      else
        handled_event = ctk_widget_event (widget, event);

      tmp = ctk_widget_get_parent (widget);
      g_object_unref (widget);

      if (widget == topmost)
        break;

      widget = tmp;

      if (handled_event || !widget)
        break;
    }

  return handled_event;
}

static gboolean
propagate_event_down (CtkWidget *widget,
                      CdkEvent  *event,
                      CtkWidget *topmost)
{
  gint handled_event = FALSE;
  GList *widgets = NULL;
  GList *l;

  widgets = g_list_prepend (widgets, g_object_ref (widget));
  while (widget && widget != topmost)
    {
      widget = ctk_widget_get_parent (widget);
      if (!widget)
        break;

      widgets = g_list_prepend (widgets, g_object_ref (widget));

      if (widget == topmost)
        break;
    }

  for (l = widgets; l && !handled_event; l = l->next)
    {
      widget = (CtkWidget *)l->data;

      if (!ctk_widget_is_sensitive (widget))
        {
          /* stop propagating on SCROLL, but don't handle the event, so it
           * can propagate up again and reach its handling widget
           */
          if (event->type == CDK_SCROLL)
            break;
          else
            handled_event = TRUE;
        }
      else
        handled_event = _ctk_widget_captured_event (widget, event);
    }
  g_list_free_full (widgets, (GDestroyNotify)g_object_unref);

  return handled_event;
}

static gboolean
propagate_event (CtkWidget *widget,
                 CdkEvent  *event,
                 gboolean   captured,
                 CtkWidget *topmost)
{
  gboolean handled_event = FALSE;
  gboolean (* propagate_func) (CtkWidget *widget, CdkEvent  *event);

  propagate_func = captured ? _ctk_widget_captured_event : ctk_widget_event;

  if (event->type == CDK_KEY_PRESS || event->type == CDK_KEY_RELEASE)
    {
      /* Only send key events within Window widgets to the Window
       * The Window widget will in turn pass the
       * key event on to the currently focused widget
       * for that window.
       */
      CtkWidget *window;

      window = ctk_widget_get_toplevel (widget);
      if (CTK_IS_WINDOW (window))
        {
          g_object_ref (widget);
          /* If there is a grab within the window, give the grab widget
           * a first crack at the key event
           */
          if (widget != window && ctk_widget_has_grab (widget))
            handled_event = propagate_func (widget, event);

          if (!handled_event &&
              ctk_widget_is_sensitive (window))
            handled_event = propagate_func (window, event);

          g_object_unref (widget);
          return handled_event;
        }
    }

  /* Other events get propagated up/down the widget tree */
  return captured ?
    propagate_event_down (widget, event, topmost) :
    propagate_event_up (widget, event, topmost);
}

/**
 * ctk_propagate_event:
 * @widget: a #CtkWidget
 * @event: an event
 *
 * Sends an event to a widget, propagating the event to parent widgets
 * if the event remains unhandled.
 *
 * Events received by CTK+ from CDK normally begin in ctk_main_do_event().
 * Depending on the type of event, existence of modal dialogs, grabs, etc.,
 * the event may be propagated; if so, this function is used.
 *
 * ctk_propagate_event() calls ctk_widget_event() on each widget it
 * decides to send the event to. So ctk_widget_event() is the lowest-level
 * function; it simply emits the #CtkWidget::event and possibly an
 * event-specific signal on a widget. ctk_propagate_event() is a bit
 * higher-level, and ctk_main_do_event() is the highest level.
 *
 * All that said, you most likely don’t want to use any of these
 * functions; synthesizing events is rarely needed. There are almost
 * certainly better ways to achieve your goals. For example, use
 * cdk_window_invalidate_rect() or ctk_widget_queue_draw() instead
 * of making up expose events.
 */
void
ctk_propagate_event (CtkWidget *widget,
                     CdkEvent  *event)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (event != NULL);

  propagate_event (widget, event, FALSE, NULL);
}

gboolean
_ctk_propagate_captured_event (CtkWidget *widget,
                               CdkEvent  *event,
                               CtkWidget *topmost)
{
  return propagate_event (widget, event, TRUE, topmost);
}
