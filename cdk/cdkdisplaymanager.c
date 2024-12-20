/* CDK - The GIMP Drawing Kit
 * Copyright (C) 2000 Red Hat, Inc.
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

#include "cdkconfig.h"
#include "cdkdisplaymanagerprivate.h"
#include "cdkdisplayprivate.h"
#include "cdkinternals.h"
#include "cdkkeysprivate.h"
#include "cdkmarshalers.h"
#include "cdkintl.h"

#ifdef CDK_WINDOWING_X11
#include "x11/cdkx.h"
#include "x11/cdkprivate-x11.h"
#endif

#ifdef CDK_WINDOWING_QUARTZ
#include "quartz/cdkprivate-quartz.h"
#endif

#ifdef CDK_WINDOWING_BROADWAY
#include "broadway/cdkprivate-broadway.h"
#endif

#ifdef CDK_WINDOWING_WIN32
#include "win32/cdkwin32.h"
#include "win32/cdkprivate-win32.h"
#endif

#ifdef CDK_WINDOWING_WAYLAND
#include "wayland/cdkprivate-wayland.h"
#endif

/**
 * SECTION:cdkdisplaymanager
 * @Short_description: Maintains a list of all open CdkDisplays
 * @Title: CdkDisplayManager
 *
 * The purpose of the #CdkDisplayManager singleton object is to offer
 * notification when displays appear or disappear or the default display
 * changes.
 *
 * You can use cdk_display_manager_get() to obtain the #CdkDisplayManager
 * singleton, but that should be rarely necessary. Typically, initializing
 * CTK+ opens a display that you can work with without ever accessing the
 * #CdkDisplayManager.
 *
 * The CDK library can be built with support for multiple backends.
 * The #CdkDisplayManager object determines which backend is used
 * at runtime.
 *
 * When writing backend-specific code that is supposed to work with
 * multiple CDK backends, you have to consider both compile time and
 * runtime. At compile time, use the #CDK_WINDOWING_X11, #CDK_WINDOWING_WIN32
 * macros, etc. to find out which backends are present in the CDK library
 * you are building your application against. At runtime, use type-check
 * macros like CDK_IS_X11_DISPLAY() to find out which backend is in use:
 *
 * ## Backend-specific code ## {#backend-specific}
 *
 * |[<!-- language="C" -->
 * #ifdef CDK_WINDOWING_X11
 *   if (CDK_IS_X11_DISPLAY (display))
 *     {
 *       // make X11-specific calls here
 *     }
 *   else
 * #endif
 * #ifdef CDK_WINDOWING_QUARTZ
 *   if (CDK_IS_QUARTZ_DISPLAY (display))
 *     {
 *       // make Quartz-specific calls here
*     }
 *   else
 * #endif
 *   g_error ("Unsupported CDK backend");
 * ]|
 */


enum {
  PROP_0,
  PROP_DEFAULT_DISPLAY
};

enum {
  DISPLAY_OPENED,
  LAST_SIGNAL
};

static void cdk_display_manager_class_init   (CdkDisplayManagerClass *klass);
static void cdk_display_manager_set_property (GObject                *object,
                                              guint                   prop_id,
                                              const GValue           *value,
                                              GParamSpec             *pspec);
static void cdk_display_manager_get_property (GObject                *object,
                                              guint                   prop_id,
                                              GValue                 *value,
                                              GParamSpec             *pspec);

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (CdkDisplayManager, cdk_display_manager, G_TYPE_OBJECT)

static void
cdk_display_manager_class_init (CdkDisplayManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = cdk_display_manager_set_property;
  object_class->get_property = cdk_display_manager_get_property;

  /**
   * CdkDisplayManager::display-opened:
   * @manager: the object on which the signal is emitted
   * @display: the opened display
   *
   * The ::display-opened signal is emitted when a display is opened.
   *
   * Since: 2.2
   */
  signals[DISPLAY_OPENED] =
    g_signal_new (g_intern_static_string ("display-opened"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CdkDisplayManagerClass, display_opened),
                  NULL, NULL,
                  _cdk_marshal_VOID__OBJECT,
                  G_TYPE_NONE,
                  1,
                  CDK_TYPE_DISPLAY);
  g_signal_set_va_marshaller (signals[DISPLAY_OPENED],
                              G_TYPE_FROM_CLASS (klass),
                              _cdk_marshal_VOID__OBJECTv);

  g_object_class_install_property (object_class,
                                   PROP_DEFAULT_DISPLAY,
                                   g_param_spec_object ("default-display",
                                                        P_("Default Display"),
                                                        P_("The default display for CDK"),
                                                        CDK_TYPE_DISPLAY,
                                                        G_PARAM_READWRITE|G_PARAM_STATIC_NAME|
                                                        G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB));
}

static void
cdk_display_manager_init (CdkDisplayManager *manager G_GNUC_UNUSED)
{
}

static void
cdk_display_manager_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  switch (prop_id)
    {
    case PROP_DEFAULT_DISPLAY:
      cdk_display_manager_set_default_display (CDK_DISPLAY_MANAGER (object),
                                               g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
cdk_display_manager_get_property (GObject      *object,
                                  guint         prop_id,
                                  GValue       *value,
                                  GParamSpec   *pspec)
{
  switch (prop_id)
    {
    case PROP_DEFAULT_DISPLAY:
      g_value_set_object (value,
                          cdk_display_manager_get_default_display (CDK_DISPLAY_MANAGER (object)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static const gchar *allowed_backends;

/**
 * cdk_set_allowed_backends:
 * @backends: a comma-separated list of backends
 *
 * Sets a list of backends that CDK should try to use.
 *
 * This can be be useful if your application does not
 * work with certain CDK backends.
 *
 * By default, CDK tries all included backends.
 *
 * For example,
 * |[<!-- language="C" -->
 * cdk_set_allowed_backends ("wayland,quartz,*");
 * ]|
 * instructs CDK to try the Wayland backend first,
 * followed by the Quartz backend, and then all
 * others.
 *
 * If the `CDK_BACKEND` environment variable
 * is set, it determines what backends are tried in what
 * order, while still respecting the set of allowed backends
 * that are specified by this function.
 *
 * The possible backend names are x11, win32, quartz,
 * broadway, wayland. You can also include a * in the
 * list to try all remaining backends.
 *
 * This call must happen prior to cdk_display_open(),
 * ctk_init(), ctk_init_with_args() or ctk_init_check()
 * in order to take effect.
 *
 * Since: 3.10
 */
void
cdk_set_allowed_backends (const gchar *backends)
{
  allowed_backends = g_strdup (backends);
}

typedef struct _CdkBackend CdkBackend;

struct _CdkBackend {
  const char *name;
  CdkDisplay * (* open_display) (const char *name);
};

static CdkBackend cdk_backends[] = {
#ifdef CDK_WINDOWING_QUARTZ
  { "quartz",   _cdk_quartz_display_open },
#endif
#ifdef CDK_WINDOWING_WIN32
  { "win32",    _cdk_win32_display_open },
#endif
#ifdef CDK_WINDOWING_WAYLAND
  { "wayland",  _cdk_wayland_display_open },
#endif
#ifdef CDK_WINDOWING_X11
  { "x11",      _cdk_x11_display_open },
#endif
#ifdef CDK_WINDOWING_BROADWAY
  { "broadway", _cdk_broadway_display_open },
#endif
  /* NULL-terminating this array so we can use commas above */
  { NULL, NULL }
};

/**
 * cdk_display_manager_get:
 *
 * Gets the singleton #CdkDisplayManager object.
 *
 * When called for the first time, this function consults the
 * `CDK_BACKEND` environment variable to find out which
 * of the supported CDK backends to use (in case CDK has been compiled
 * with multiple backends). Applications can use cdk_set_allowed_backends()
 * to limit what backends can be used.
 *
 * Returns: (transfer none): The global #CdkDisplayManager singleton;
 *     cdk_parse_args(), cdk_init(), or cdk_init_check() must have
 *     been called first.
 *
 * Since: 2.2
 **/
CdkDisplayManager*
cdk_display_manager_get (void)
{
  static CdkDisplayManager *manager = NULL;

  if (manager == NULL)
    manager = g_object_new (CDK_TYPE_DISPLAY_MANAGER, NULL);
  
  return manager;
}

/**
 * cdk_display_manager_get_default_display:
 * @manager: a #CdkDisplayManager
 *
 * Gets the default #CdkDisplay.
 *
 * Returns: (nullable) (transfer none): a #CdkDisplay, or %NULL if
 *     there is no default display.
 *
 * Since: 2.2
 */
CdkDisplay *
cdk_display_manager_get_default_display (CdkDisplayManager *manager)
{
  return manager->default_display;
}

/**
 * cdk_display_get_default:
 *
 * Gets the default #CdkDisplay. This is a convenience
 * function for:
 * `cdk_display_manager_get_default_display (cdk_display_manager_get ())`.
 *
 * Returns: (nullable) (transfer none): a #CdkDisplay, or %NULL if
 *   there is no default display.
 *
 * Since: 2.2
 */
CdkDisplay *
cdk_display_get_default (void)
{
  return cdk_display_manager_get_default_display (cdk_display_manager_get ());
}

/**
 * cdk_screen_get_default:
 *
 * Gets the default screen for the default display. (See
 * cdk_display_get_default ()).
 *
 * Returns: (nullable) (transfer none): a #CdkScreen, or %NULL if
 *     there is no default display.
 *
 * Since: 2.2
 */
CdkScreen *
cdk_screen_get_default (void)
{
  CdkDisplay *display;

  display = cdk_display_get_default ();

  if (display)
    return CDK_DISPLAY_GET_CLASS (display)->get_default_screen (display);
  else
    return NULL;
}

/**
 * cdk_display_manager_set_default_display:
 * @manager: a #CdkDisplayManager
 * @display: a #CdkDisplay
 * 
 * Sets @display as the default display.
 *
 * Since: 2.2
 **/
void
cdk_display_manager_set_default_display (CdkDisplayManager *manager,
                                         CdkDisplay        *display)
{
  manager->default_display = display;

  if (display)
    CDK_DISPLAY_GET_CLASS (display)->make_default (display);

  g_object_notify (G_OBJECT (manager), "default-display");
}

/**
 * cdk_display_manager_list_displays:
 * @manager: a #CdkDisplayManager
 *
 * List all currently open displays.
 *
 * Returns: (transfer container) (element-type CdkDisplay): a newly
 *     allocated #GSList of #CdkDisplay objects. Free with g_slist_free()
 *     when you are done with it.
 *
 * Since: 2.2
 **/
GSList *
cdk_display_manager_list_displays (CdkDisplayManager *manager)
{
  return g_slist_copy (manager->displays);
}

/**
 * cdk_display_manager_open_display:
 * @manager: a #CdkDisplayManager
 * @name: the name of the display to open
 *
 * Opens a display.
 *
 * Returns: (nullable) (transfer none): a #CdkDisplay, or %NULL if the
 *     display could not be opened
 *
 * Since: 3.0
 */
CdkDisplay *
cdk_display_manager_open_display (CdkDisplayManager *manager G_GNUC_UNUSED,
                                  const gchar       *name)
{
  const gchar *backend_list;
  CdkDisplay *display;
  gchar **backends;
  gint i, j;
  gboolean allow_any;

  if (allowed_backends == NULL)
    allowed_backends = "*";
  allow_any = strstr (allowed_backends, "*") != NULL;

  backend_list = g_getenv ("CDK_BACKEND");
  if (backend_list == NULL)
    backend_list = allowed_backends;
  else if (g_strcmp0 (backend_list, "help") == 0)
    {
      fprintf (stderr, "Supported CDK backends:");
      for (i = 0; cdk_backends[i].name != NULL; i++)
        fprintf (stderr, " %s", cdk_backends[i].name);
      fprintf (stderr, "\n");

      backend_list = allowed_backends;
    }
  backends = g_strsplit (backend_list, ",", 0);

  display = NULL;

  for (i = 0; display == NULL && backends[i] != NULL; i++)
    {
      const gchar *backend = backends[i];
      gboolean any = g_str_equal (backend, "*");

      if (!allow_any && !any && !strstr (allowed_backends, backend))
        continue;

      for (j = 0; cdk_backends[j].name != NULL; j++)
        {
          if ((any && allow_any) ||
              (any && strstr (allowed_backends, cdk_backends[j].name)) ||
              g_str_equal (backend, cdk_backends[j].name))
            {
              CDK_NOTE (MISC, g_message ("Trying %s backend", cdk_backends[j].name));
              display = cdk_backends[j].open_display (name);
              if (display)
                break;
            }
        }
    }

  g_strfreev (backends);

  return display;
}

void
_cdk_display_manager_add_display (CdkDisplayManager *manager,
                                  CdkDisplay        *display)
{
  if (manager->displays == NULL)
    cdk_display_manager_set_default_display (manager, display);

  manager->displays = g_slist_prepend (manager->displays, display);

  g_signal_emit (manager, signals[DISPLAY_OPENED], 0, display);
}

/* NB: This function can be called multiple times per display. */
void
_cdk_display_manager_remove_display (CdkDisplayManager *manager,
                                     CdkDisplay        *display)
{
  manager->displays = g_slist_remove (manager->displays, display);

  if (manager->default_display == display)
    {
      if (manager->displays)
        cdk_display_manager_set_default_display (manager, manager->displays->data);
      else
        cdk_display_manager_set_default_display (manager, NULL);
    }
}
