/* CTK - The GIMP Toolkit
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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.Free
 */


#include "config.h"

#include <string.h>

#include "ctksettings.h"

#include "ctkmodules.h"
#include "ctkmodulesprivate.h"
#include "ctksettingsprivate.h"
#include "ctkintl.h"
#include "ctkwidget.h"
#include "ctkprivate.h"
#include "ctkcssproviderprivate.h"
#include "ctkstyleproviderprivate.h"
#include "ctktypebuiltins.h"
#include "ctkversion.h"
#include "ctkscrolledwindow.h"

#ifdef CDK_WINDOWING_X11
#include "x11/cdkx.h"
#include <pango/pangofc-fontmap.h>
#endif

#ifdef CDK_WINDOWING_WAYLAND
#include "wayland/cdkwayland.h"
#include <pango/pangofc-fontmap.h>
#endif

#ifdef CDK_WINDOWING_BROADWAY
#include "broadway/cdkbroadway.h"
#endif

#ifdef CDK_WINDOWING_QUARTZ
#include "quartz/cdkquartz.h"
#endif

#ifdef CDK_WINDOWING_WIN32
#include "win32/cdkwin32.h"
#endif

#include "ctkrc.h"

#ifdef CDK_WINDOWING_QUARTZ
#define PRINT_PREVIEW_COMMAND "open -b com.apple.Preview %f"
#else
#define PRINT_PREVIEW_COMMAND "evince --unlink-tempfile --preview --print-settings %s %f"
#endif

/**
 * SECTION:ctksettings
 * @Short_description: Sharing settings between applications
 * @Title: Settings
 *
 * CtkSettings provide a mechanism to share global settings between
 * applications.
 *
 * On the X window system, this sharing is realized by an
 * [XSettings](http://www.freedesktop.org/wiki/Specifications/xsettings-spec)
 * manager that is usually part of the desktop environment, along with
 * utilities that let the user change these settings. In the absence of
 * an Xsettings manager, CTK+ reads default values for settings from
 * `settings.ini` files in
 * `/etc/ctk-3.0`, `$XDG_CONFIG_DIRS/ctk-3.0`
 * and `$XDG_CONFIG_HOME/ctk-3.0`.
 * These files must be valid key files (see #GKeyFile), and have
 * a section called Settings. Themes can also provide default values
 * for settings by installing a `settings.ini` file
 * next to their `ctk.css` file.
 *
 * Applications can override system-wide settings by setting the property
 * of the CtkSettings object with g_object_set(). This should be restricted
 * to special cases though; CtkSettings are not meant as an application
 * configuration facility. When doing so, you need to be aware that settings
 * that are specific to individual widgets may not be available before the
 * widget type has been realized at least once. The following example
 * demonstrates a way to do this:
 * |[<!-- language="C" -->
 *   ctk_init (&argc, &argv);
 *
 *   // make sure the type is realized
 *   g_type_class_unref (g_type_class_ref (CTK_TYPE_IMAGE_MENU_ITEM));
 *
 *   g_object_set (ctk_settings_get_default (), "ctk-enable-animations", FALSE, NULL);
 * ]|
 *
 * There is one CtkSettings instance per screen. It can be obtained with
 * ctk_settings_get_for_screen(), but in many cases, it is more convenient
 * to use ctk_widget_get_settings(). ctk_settings_get_default() returns the
 * CtkSettings instance for the default screen.
 */


#define DEFAULT_TIMEOUT_INITIAL 500
#define DEFAULT_TIMEOUT_REPEAT   50
#define DEFAULT_TIMEOUT_EXPAND  500

typedef struct _CtkSettingsPropertyValue CtkSettingsPropertyValue;
typedef struct _CtkSettingsValuePrivate CtkSettingsValuePrivate;

struct _CtkSettingsPrivate
{
  GData *queued_settings;      /* of type CtkSettingsValue* */
  CtkSettingsPropertyValue *property_values;
  CdkScreen *screen;
  GSList *style_cascades;
  CtkCssProvider *theme_provider;
  CtkCssProvider *key_theme_provider;
  gint font_size;
  gboolean font_size_absolute;
  gchar *font_family;
};

struct _CtkSettingsValuePrivate
{
  CtkSettingsValue public;
  CtkSettingsSource source;
};

struct _CtkSettingsPropertyValue
{
  GValue value;
  CtkSettingsSource source;
};

enum {
  PROP_0,
  PROP_DOUBLE_CLICK_TIME,
  PROP_DOUBLE_CLICK_DISTANCE,
  PROP_CURSOR_BLINK,
  PROP_CURSOR_BLINK_TIME,
  PROP_CURSOR_BLINK_TIMEOUT,
  PROP_SPLIT_CURSOR,
  PROP_CURSOR_ASPECT_RATIO,
  PROP_THEME_NAME,
  PROP_ICON_THEME_NAME,
  PROP_FALLBACK_ICON_THEME,
  PROP_KEY_THEME_NAME,
  PROP_MENU_BAR_ACCEL,
  PROP_DND_DRAG_THRESHOLD,
  PROP_FONT_NAME,
  PROP_ICON_SIZES,
  PROP_MODULES,
  PROP_XFT_ANTIALIAS,
  PROP_XFT_HINTING,
  PROP_XFT_HINTSTYLE,
  PROP_XFT_RGBA,
  PROP_XFT_DPI,
  PROP_CURSOR_THEME_NAME,
  PROP_CURSOR_THEME_SIZE,
  PROP_ALTERNATIVE_BUTTON_ORDER,
  PROP_ALTERNATIVE_SORT_ARROWS,
  PROP_SHOW_INPUT_METHOD_MENU,
  PROP_SHOW_UNICODE_MENU,
  PROP_TIMEOUT_INITIAL,
  PROP_TIMEOUT_REPEAT,
  PROP_TIMEOUT_EXPAND,
  PROP_COLOR_SCHEME,
  PROP_ENABLE_ANIMATIONS,
  PROP_TOUCHSCREEN_MODE,
  PROP_TOOLTIP_TIMEOUT,
  PROP_TOOLTIP_BROWSE_TIMEOUT,
  PROP_TOOLTIP_BROWSE_MODE_TIMEOUT,
  PROP_KEYNAV_CURSOR_ONLY,
  PROP_KEYNAV_WRAP_AROUND,
  PROP_ERROR_BELL,
  PROP_COLOR_HASH,
  PROP_FILE_CHOOSER_BACKEND,
  PROP_PRINT_BACKENDS,
  PROP_PRINT_PREVIEW_COMMAND,
  PROP_ENABLE_MNEMONICS,
  PROP_ENABLE_ACCELS,
  PROP_RECENT_FILES_LIMIT,
  PROP_IM_MODULE,
  PROP_RECENT_FILES_MAX_AGE,
  PROP_FONTCONFIG_TIMESTAMP,
  PROP_SOUND_THEME_NAME,
  PROP_ENABLE_INPUT_FEEDBACK_SOUNDS,
  PROP_ENABLE_EVENT_SOUNDS,
  PROP_ENABLE_TOOLTIPS,
  PROP_TOOLBAR_STYLE,
  PROP_TOOLBAR_ICON_SIZE,
  PROP_AUTO_MNEMONICS,
  PROP_PRIMARY_BUTTON_WARPS_SLIDER,
  PROP_VISIBLE_FOCUS,
  PROP_APPLICATION_PREFER_DARK_THEME,
  PROP_BUTTON_IMAGES,
  PROP_ENTRY_SELECT_ON_FOCUS,
  PROP_ENTRY_PASSWORD_HINT_TIMEOUT,
  PROP_MENU_IMAGES,
  PROP_MENU_BAR_POPUP_DELAY,
  PROP_SCROLLED_WINDOW_PLACEMENT,
  PROP_CAN_CHANGE_ACCELS,
  PROP_MENU_POPUP_DELAY,
  PROP_MENU_POPDOWN_DELAY,
  PROP_LABEL_SELECT_ON_FOCUS,
  PROP_COLOR_PALETTE,
  PROP_IM_PREEDIT_STYLE,
  PROP_IM_STATUS_STYLE,
  PROP_SHELL_SHOWS_APP_MENU,
  PROP_SHELL_SHOWS_MENUBAR,
  PROP_SHELL_SHOWS_DESKTOP,
  PROP_DECORATION_LAYOUT,
  PROP_TITLEBAR_DOUBLE_CLICK,
  PROP_TITLEBAR_MIDDLE_CLICK,
  PROP_TITLEBAR_RIGHT_CLICK,
  PROP_DIALOGS_USE_HEADER,
  PROP_ENABLE_PRIMARY_PASTE,
  PROP_RECENT_FILES_ENABLED,
  PROP_LONG_PRESS_TIME,
  PROP_KEYNAV_USE_CARET,
  PROP_OVERLAY_SCROLLING
};

/* --- prototypes --- */
static void     ctk_settings_provider_iface_init (CtkStyleProviderIface *iface);
static void     ctk_settings_provider_private_init (CtkStyleProviderPrivateInterface *iface);

static void     ctk_settings_finalize            (GObject               *object);
static void     ctk_settings_get_property        (GObject               *object,
                                                  guint                  property_id,
                                                  GValue                *value,
                                                  GParamSpec            *pspec);
static void     ctk_settings_set_property        (GObject               *object,
                                                  guint                  property_id,
                                                  const GValue          *value,
                                                  GParamSpec            *pspec);
static void     ctk_settings_notify              (GObject               *object,
                                                  GParamSpec            *pspec);
static guint    settings_install_property_parser (CtkSettingsClass      *class,
                                                  GParamSpec            *pspec,
                                                  CtkRcPropertyParser    parser);
static void    settings_update_double_click      (CtkSettings           *settings);
static void    settings_update_modules           (CtkSettings           *settings);

static void    settings_update_cursor_theme      (CtkSettings           *settings);
static void    settings_update_resolution        (CtkSettings           *settings);
static void    settings_update_font_options      (CtkSettings           *settings);
static void    settings_update_font_values       (CtkSettings           *settings);
static gboolean settings_update_fontconfig       (CtkSettings           *settings);
static void    settings_update_theme             (CtkSettings           *settings);
static void    settings_update_key_theme         (CtkSettings           *settings);
static gboolean settings_update_xsetting         (CtkSettings           *settings,
                                                  GParamSpec            *pspec,
                                                  gboolean               force);
static void    settings_update_xsettings         (CtkSettings           *settings);

static void ctk_settings_load_from_key_file      (CtkSettings           *settings,
                                                  const gchar           *path,
                                                  CtkSettingsSource      source);
static void settings_update_provider             (CdkScreen             *screen,
                                                  CtkCssProvider       **old,
                                                  CtkCssProvider        *new);

/* the default palette for CtkColorSelelection */
static const gchar default_color_palette[] =
  "black:white:gray50:red:purple:blue:light blue:green:yellow:orange:"
  "lavender:brown:goldenrod4:dodger blue:pink:light green:gray10:gray30:gray75:gray90";

/* --- variables --- */
static GQuark            quark_property_parser = 0;
static GQuark            quark_ctk_settings = 0;
static GSList           *object_list = NULL;
static guint             class_n_properties = 0;

typedef struct {
  CdkDisplay *display;
  CtkSettings *settings;
} DisplaySettings;

static GArray *display_settings;


G_DEFINE_TYPE_EXTENDED (CtkSettings, ctk_settings, G_TYPE_OBJECT, 0,
                        G_ADD_PRIVATE (CtkSettings)
                        G_IMPLEMENT_INTERFACE (CTK_TYPE_STYLE_PROVIDER,
                                               ctk_settings_provider_iface_init)
                        G_IMPLEMENT_INTERFACE (CTK_TYPE_STYLE_PROVIDER_PRIVATE,
                                               ctk_settings_provider_private_init));

/* --- functions --- */
static void
ctk_settings_init (CtkSettings *settings)
{
  CtkSettingsPrivate *priv;
  GParamSpec **pspecs, **p;
  guint i = 0;
  gchar *path;
  const gchar * const *config_dirs;

  priv = ctk_settings_get_instance_private (settings);
  settings->priv = priv;

  g_datalist_init (&priv->queued_settings);
  object_list = g_slist_prepend (object_list, settings);

  priv->style_cascades = g_slist_prepend (NULL, _ctk_style_cascade_new ());
  priv->theme_provider = ctk_css_provider_new ();

  /* build up property array for all yet existing properties and queue
   * notification for them (at least notification for internal properties
   * will instantly be caught)
   */
  pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (settings), NULL);
  for (p = pspecs; *p; p++)
    if ((*p)->owner_type == G_OBJECT_TYPE (settings))
      i++;
  priv->property_values = g_new0 (CtkSettingsPropertyValue, i);
  i = 0;
  g_object_freeze_notify (G_OBJECT (settings));

  for (p = pspecs; *p; p++)
    {
      GParamSpec *pspec = *p;
      GType value_type = G_PARAM_SPEC_VALUE_TYPE (pspec);

      if (pspec->owner_type != G_OBJECT_TYPE (settings))
        continue;
      g_value_init (&priv->property_values[i].value, value_type);
      g_param_value_set_default (pspec, &priv->property_values[i].value);

      g_object_notify_by_pspec (G_OBJECT (settings), pspec);
      priv->property_values[i].source = CTK_SETTINGS_SOURCE_DEFAULT;
      i++;
    }
  g_free (pspecs);

  path = g_build_filename (_ctk_get_data_prefix (), "share", "ctk-3.0", "settings.ini", NULL);
  if (g_file_test (path, G_FILE_TEST_EXISTS))
    ctk_settings_load_from_key_file (settings, path, CTK_SETTINGS_SOURCE_DEFAULT);
  g_free (path);

  path = g_build_filename (_ctk_get_sysconfdir (), "ctk-3.0", "settings.ini", NULL);
  if (g_file_test (path, G_FILE_TEST_EXISTS))
    ctk_settings_load_from_key_file (settings, path, CTK_SETTINGS_SOURCE_DEFAULT);
  g_free (path);

  config_dirs = g_get_system_config_dirs ();
  for (i = 0; config_dirs[i] != NULL; i++)
    {
      path = g_build_filename (config_dirs[i], "ctk-3.0", "settings.ini", NULL);
      if (g_file_test (path, G_FILE_TEST_EXISTS))
        ctk_settings_load_from_key_file (settings, path, CTK_SETTINGS_SOURCE_DEFAULT);
      g_free (path);
    }

  path = g_build_filename (g_get_user_config_dir (), "ctk-3.0", "settings.ini", NULL);
  if (g_file_test (path, G_FILE_TEST_EXISTS))
    ctk_settings_load_from_key_file (settings, path, CTK_SETTINGS_SOURCE_DEFAULT);
  g_free (path);

  g_object_thaw_notify (G_OBJECT (settings));

  /* ensure that derived fields are initialized */
  if (priv->font_size == 0)
    settings_update_font_values (settings);
}

static void
ctk_settings_class_init (CtkSettingsClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  guint result;

  gobject_class->finalize = ctk_settings_finalize;
  gobject_class->get_property = ctk_settings_get_property;
  gobject_class->set_property = ctk_settings_set_property;
  gobject_class->notify = ctk_settings_notify;

  quark_property_parser = g_quark_from_static_string ("ctk-rc-property-parser");
  quark_ctk_settings = g_quark_from_static_string ("ctk-settings");

  result = settings_install_property_parser (class,
                                             g_param_spec_int ("ctk-double-click-time",
                                                               P_("Double Click Time"),
                                                               P_("Maximum time allowed between two clicks for them to be considered a double click (in milliseconds)"),
                                                               0, G_MAXINT, 400,
                                                               CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_DOUBLE_CLICK_TIME);
  result = settings_install_property_parser (class,
                                             g_param_spec_int ("ctk-double-click-distance",
                                                               P_("Double Click Distance"),
                                                               P_("Maximum distance allowed between two clicks for them to be considered a double click (in pixels)"),
                                                               0, G_MAXINT, 5,
                                                               CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_DOUBLE_CLICK_DISTANCE);

  /**
   * CtkSettings:ctk-cursor-blink:
   *
   * Whether the cursor should blink.
   *
   * Also see the #CtkSettings:ctk-cursor-blink-timeout setting,
   * which allows more flexible control over cursor blinking.
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("ctk-cursor-blink",
                                                                   P_("Cursor Blink"),
                                                                   P_("Whether the cursor should blink"),
                                                                   TRUE,
                                                                   CTK_PARAM_READWRITE ),
                                             NULL);
  g_assert (result == PROP_CURSOR_BLINK);
  result = settings_install_property_parser (class,
                                             g_param_spec_int ("ctk-cursor-blink-time",
                                                               P_("Cursor Blink Time"),
                                                               P_("Length of the cursor blink cycle, in milliseconds"),
                                                               100, G_MAXINT, 1200,
                                                               CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_CURSOR_BLINK_TIME);

  /**
   * CtkSettings:ctk-cursor-blink-timeout:
   *
   * Time after which the cursor stops blinking, in seconds.
   * The timer is reset after each user interaction.
   *
   * Setting this to zero has the same effect as setting
   * #CtkSettings:ctk-cursor-blink to %FALSE.
   *
   * Since: 2.12
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_int ("ctk-cursor-blink-timeout",
                                                               P_("Cursor Blink Timeout"),
                                                               P_("Time after which the cursor stops blinking, in seconds"),
                                                               1, G_MAXINT, 10,
                                                               CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_CURSOR_BLINK_TIMEOUT);
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("ctk-split-cursor",
                                                                   P_("Split Cursor"),
                                                                   P_("Whether two cursors should be displayed for mixed left-to-right and right-to-left text"),
                                                                   TRUE,
                                                                   CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_SPLIT_CURSOR);
  result = settings_install_property_parser (class,
                                             g_param_spec_float ("ctk-cursor-aspect-ratio",
                                                                 P_("Cursor Aspect Ratio"),
                                                                 P_("The aspect ratio of the text caret"),
                                                                 0.0, 1.0, 0.04,
                                                                 CTK_PARAM_READWRITE),
                                             NULL);

  g_assert (result == PROP_CURSOR_ASPECT_RATIO);
  result = settings_install_property_parser (class,
                                             g_param_spec_string ("ctk-theme-name",
                                                                   P_("Theme Name"),
                                                                   P_("Name of theme to load"),
                                                                  DEFAULT_THEME_NAME,
                                                                  CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_THEME_NAME);

  result = settings_install_property_parser (class,
                                             g_param_spec_string ("ctk-icon-theme-name",
                                                                  P_("Icon Theme Name"),
                                                                  P_("Name of icon theme to use"),
                                                                  DEFAULT_ICON_THEME,
                                                                  CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_ICON_THEME_NAME);

  /**
   * CtkSettings:ctk-fallback-icon-theme:
   *
   * Name of a icon theme to fall back to.
   *
   * Deprecated: 3.10: This setting is ignored.
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_string ("ctk-fallback-icon-theme",
                                                                  P_("Fallback Icon Theme Name"),
                                                                  P_("Name of a icon theme to fall back to"),
                                                                  NULL,
                                                                  CTK_PARAM_READWRITE | G_PARAM_DEPRECATED),
                                             NULL);
  g_assert (result == PROP_FALLBACK_ICON_THEME);

  result = settings_install_property_parser (class,
                                             g_param_spec_string ("ctk-key-theme-name",
                                                                  P_("Key Theme Name"),
                                                                  P_("Name of key theme to load"),
                                                                  NULL,
                                                                  CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_KEY_THEME_NAME);

  /**
   * CtkSettings:ctk-menu-bar-accel:
   *
   * Keybinding to activate the menu bar.
   *
   * Deprecated: 3.10: This setting can still be used for application
   *      overrides, but will be ignored in the future
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_string ("ctk-menu-bar-accel",
                                                                  P_("Menu bar accelerator"),
                                                                  P_("Keybinding to activate the menu bar"),
                                                                  "F10",
                                                                  CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_MENU_BAR_ACCEL);

  result = settings_install_property_parser (class,
                                             g_param_spec_int ("ctk-dnd-drag-threshold",
                                                               P_("Drag threshold"),
                                                               P_("Number of pixels the cursor can move before dragging"),
                                                               1, G_MAXINT, 8,
                                                               CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_DND_DRAG_THRESHOLD);

  /**
   * CtkSettings:ctk-font-name:
   *
   * The default font to use. CTK+ uses the family name and size from this string.
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_string ("ctk-font-name",
                                                                   P_("Font Name"),
                                                                   P_("The default font family and size to use"),
                                                                  "Sans 10",
                                                                  CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_FONT_NAME);

  /**
   * CtkSettings:ctk-icon-sizes:
   *
   * A list of icon sizes. The list is separated by colons, and
   * item has the form:
   *
   * `size-name` = `width` , `height`
   *
   * E.g. "ctk-menu=16,16:ctk-button=20,20:ctk-dialog=48,48".
   * CTK+ itself use the following named icon sizes: ctk-menu,
   * ctk-button, ctk-small-toolbar, ctk-large-toolbar, ctk-dnd,
   * ctk-dialog. Applications can register their own named icon
   * sizes with ctk_icon_size_register().
   *
   * Deprecated: 3.10: This setting is ignored.
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_string ("ctk-icon-sizes",
                                                                   P_("Icon Sizes"),
                                                                   P_("List of icon sizes (ctk-menu=16,16:ctk-button=20,20..."),
                                                                  NULL,
                                                                  CTK_PARAM_READWRITE | G_PARAM_DEPRECATED),
                                             NULL);
  g_assert (result == PROP_ICON_SIZES);

  result = settings_install_property_parser (class,
                                             g_param_spec_string ("ctk-modules",
                                                                  P_("CTK Modules"),
                                                                  P_("List of currently active CTK modules"),
                                                                  NULL,
                                                                  CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_MODULES);

  result = settings_install_property_parser (class,
                                             g_param_spec_int ("ctk-xft-antialias",
                                                               P_("Xft Antialias"),
                                                               P_("Whether to antialias Xft fonts; 0=no, 1=yes, -1=default"),
                                                               -1, 1, -1,
                                                               CTK_PARAM_READWRITE),
                                             NULL);

  g_assert (result == PROP_XFT_ANTIALIAS);

  result = settings_install_property_parser (class,
                                             g_param_spec_int ("ctk-xft-hinting",
                                                               P_("Xft Hinting"),
                                                               P_("Whether to hint Xft fonts; 0=no, 1=yes, -1=default"),
                                                               -1, 1, -1,
                                                               CTK_PARAM_READWRITE),
                                             NULL);

  g_assert (result == PROP_XFT_HINTING);

  result = settings_install_property_parser (class,
                                             g_param_spec_string ("ctk-xft-hintstyle",
                                                                  P_("Xft Hint Style"),
                                                                  P_("What degree of hinting to use; hintnone, hintslight, hintmedium, or hintfull"),
                                                                  NULL,
                                                                  CTK_PARAM_READWRITE),
                                              NULL);

  g_assert (result == PROP_XFT_HINTSTYLE);

  result = settings_install_property_parser (class,
                                             g_param_spec_string ("ctk-xft-rgba",
                                                                  P_("Xft RGBA"),
                                                                  P_("Type of subpixel antialiasing; none, rgb, bgr, vrgb, vbgr"),
                                                                  NULL,
                                                                  CTK_PARAM_READWRITE),
                                             NULL);

  g_assert (result == PROP_XFT_RGBA);

  result = settings_install_property_parser (class,
                                             g_param_spec_int ("ctk-xft-dpi",
                                                               P_("Xft DPI"),
                                                               P_("Resolution for Xft, in 1024 * dots/inch. -1 to use default value"),
                                                               -1, 1024*1024, -1,
                                                               CTK_PARAM_READWRITE),
                                             NULL);

  g_assert (result == PROP_XFT_DPI);

  result = settings_install_property_parser (class,
                                             g_param_spec_string ("ctk-cursor-theme-name",
                                                                  P_("Cursor theme name"),
                                                                  P_("Name of the cursor theme to use, or NULL to use the default theme"),
                                                                  NULL,
                                                                  CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_CURSOR_THEME_NAME);

  result = settings_install_property_parser (class,
                                             g_param_spec_int ("ctk-cursor-theme-size",
                                                               P_("Cursor theme size"),
                                                               P_("Size to use for cursors, or 0 to use the default size"),
                                                               0, 128, 0,
                                                               CTK_PARAM_READWRITE),
                                             NULL);

  g_assert (result == PROP_CURSOR_THEME_SIZE);

  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("ctk-alternative-button-order",
                                                                   P_("Alternative button order"),
                                                                   P_("Whether buttons in dialogs should use the alternative button order"),
                                                                   FALSE,
                                                                   CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_ALTERNATIVE_BUTTON_ORDER);

  /**
   * CtkSettings:ctk-alternative-sort-arrows:
   *
   * Controls the direction of the sort indicators in sorted list and tree
   * views. By default an arrow pointing down means the column is sorted
   * in ascending order. When set to %TRUE, this order will be inverted.
   *
   * Since: 2.12
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("ctk-alternative-sort-arrows",
                                                                   P_("Alternative sort indicator direction"),
                                                                   P_("Whether the direction of the sort indicators in list and tree views is inverted compared to the default (where down means ascending)"),
                                                                   FALSE,
                                                                   CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_ALTERNATIVE_SORT_ARROWS);

  /**
   * CtkSettings:ctk-show-input-method-menu:
   *
   * Deprecated: 3.10: This setting is ignored.
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("ctk-show-input-method-menu",
                                                                   P_("Show the 'Input Methods' menu"),
                                                                   P_("Whether the context menus of entries and text views should offer to change the input method"),
                                                                   FALSE,
                                                                   CTK_PARAM_READWRITE | G_PARAM_DEPRECATED),
                                             NULL);
  g_assert (result == PROP_SHOW_INPUT_METHOD_MENU);

  /**
   * CtkSettings:ctk-show-unicode-menu:
   *
   * Deprecated: 3.10: This setting is ignored.
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("ctk-show-unicode-menu",
                                                                   P_("Show the 'Insert Unicode Control Character' menu"),
                                                                   P_("Whether the context menus of entries and text views should offer to insert control characters"),
                                                                   FALSE,
                                                                   CTK_PARAM_READWRITE | G_PARAM_DEPRECATED),
                                             NULL);
  g_assert (result == PROP_SHOW_UNICODE_MENU);

  /**
   * CtkSettings:ctk-timeout-initial:
   *
   * Deprecated: 3.10: This setting is ignored.
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_int ("ctk-timeout-initial",
                                                               P_("Start timeout"),
                                                               P_("Starting value for timeouts, when button is pressed"),
                                                               0, G_MAXINT, DEFAULT_TIMEOUT_INITIAL,
                                                               CTK_PARAM_READWRITE | G_PARAM_DEPRECATED),
                                             NULL);

  g_assert (result == PROP_TIMEOUT_INITIAL);

  /**
   * CtkSettings:ctk-timeout-repeat:
   *
   * Deprecated: 3.10: This setting is ignored.
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_int ("ctk-timeout-repeat",
                                                               P_("Repeat timeout"),
                                                               P_("Repeat value for timeouts, when button is pressed"),
                                                               0, G_MAXINT, DEFAULT_TIMEOUT_REPEAT,
                                                               CTK_PARAM_READWRITE | G_PARAM_DEPRECATED),
                                             NULL);

  g_assert (result == PROP_TIMEOUT_REPEAT);

  /**
   * CtkSettings:ctk-timeout-expand:
   *
   * Deprecated: 3.10: This setting is ignored.
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_int ("ctk-timeout-expand",
                                                               P_("Expand timeout"),
                                                               P_("Expand value for timeouts, when a widget is expanding a new region"),
                                                               0, G_MAXINT, DEFAULT_TIMEOUT_EXPAND,
                                                               CTK_PARAM_READWRITE | G_PARAM_DEPRECATED),
                                             NULL);

  g_assert (result == PROP_TIMEOUT_EXPAND);

  /**
   * CtkSettings:ctk-color-scheme:
   *
   * A palette of named colors for use in themes. The format of the string is
   * |[
   * name1: color1
   * name2: color2
   * ...
   * ]|
   * Color names must be acceptable as identifiers in the
   * [ctkrc][ctk3-Resource-Files] syntax, and
   * color specifications must be in the format accepted by
   * cdk_color_parse().
   *
   * Note that due to the way the color tables from different sources are
   * merged, color specifications will be converted to hexadecimal form
   * when getting this property.
   *
   * Starting with CTK+ 2.12, the entries can alternatively be separated
   * by ';' instead of newlines:
   * |[
   * name1: color1; name2: color2; ...
   * ]|
   *
   * Since: 2.10
   *
   * Deprecated: 3.8: Color scheme support was dropped and is no longer supported.
   *     You can still set this property, but it will be ignored.
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_string ("ctk-color-scheme",
                                                                  P_("Color scheme"),
                                                                  P_("A palette of named colors for use in themes"),
                                                                  "",
                                                                  CTK_PARAM_READWRITE | G_PARAM_DEPRECATED),
                                             NULL);

  g_assert (result == PROP_COLOR_SCHEME);

  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("ctk-enable-animations",
                                                                   P_("Enable Animations"),
                                                                   P_("Whether to enable toolkit-wide animations."),
                                                                   TRUE,
                                                                   CTK_PARAM_READWRITE),
                                             NULL);

  g_assert (result == PROP_ENABLE_ANIMATIONS);

  /**
   * CtkSettings:ctk-touchscreen-mode:
   *
   * When %TRUE, there are no motion notify events delivered on this screen,
   * and widgets can't use the pointer hovering them for any essential
   * functionality.
   *
   * Since: 2.10
   *
   * Deprecated: 3.4. Generally, the behavior for touchscreen input should be
   *             performed dynamically based on cdk_event_get_source_device().
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("ctk-touchscreen-mode",
                                                                   P_("Enable Touchscreen Mode"),
                                                                   P_("When TRUE, there are no motion notify events delivered on this screen"),
                                                                   FALSE,
                                                                   CTK_PARAM_READWRITE | G_PARAM_DEPRECATED),
                                             NULL);

  g_assert (result == PROP_TOUCHSCREEN_MODE);

  /**
   * CtkSettings:ctk-tooltip-timeout:
   *
   * Time, in milliseconds, after which a tooltip could appear if the
   * cursor is hovering on top of a widget.
   *
   * Since: 2.12
   *
   * Deprecated: 3.10: This setting is ignored.
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_int ("ctk-tooltip-timeout",
                                                               P_("Tooltip timeout"),
                                                               P_("Timeout before tooltip is shown"),
                                                               0, G_MAXINT,
                                                               500,
                                                               CTK_PARAM_READWRITE | G_PARAM_DEPRECATED),
                                             NULL);

  g_assert (result == PROP_TOOLTIP_TIMEOUT);

  /**
   * CtkSettings:ctk-tooltip-browse-timeout:
   *
   * Controls the time after which tooltips will appear when
   * browse mode is enabled, in milliseconds.
   *
   * Browse mode is enabled when the mouse pointer moves off an object
   * where a tooltip was currently being displayed. If the mouse pointer
   * hits another object before the browse mode timeout expires (see
   * #CtkSettings:ctk-tooltip-browse-mode-timeout), it will take the
   * amount of milliseconds specified by this setting to popup the tooltip
   * for the new object.
   *
   * Since: 2.12
   *
   * Deprecated: 3.10: This setting is ignored.
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_int ("ctk-tooltip-browse-timeout",
                                                               P_("Tooltip browse timeout"),
                                                               P_("Timeout before tooltip is shown when browse mode is enabled"),
                                                               0, G_MAXINT,
                                                               60,
                                                               CTK_PARAM_READWRITE | G_PARAM_DEPRECATED),
                                             NULL);

  g_assert (result == PROP_TOOLTIP_BROWSE_TIMEOUT);

  /**
   * CtkSettings:ctk-tooltip-browse-mode-timeout:
   *
   * Amount of time, in milliseconds, after which the browse mode
   * will be disabled.
   *
   * See #CtkSettings:ctk-tooltip-browse-timeout for more information
   * about browse mode.
   *
   * Since: 2.12
   *
   * Deprecated: 3.10: This setting is ignored.
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_int ("ctk-tooltip-browse-mode-timeout",
                                                               P_("Tooltip browse mode timeout"),
                                                               P_("Timeout after which browse mode is disabled"),
                                                               0, G_MAXINT,
                                                               500,
                                                               CTK_PARAM_READWRITE | G_PARAM_DEPRECATED),
                                             NULL);

  g_assert (result == PROP_TOOLTIP_BROWSE_MODE_TIMEOUT);

  /**
   * CtkSettings:ctk-keynav-cursor-only:
   *
   * When %TRUE, keyboard navigation should be able to reach all widgets
   * by using the cursor keys only. Tab, Shift etc. keys can't be expected
   * to be present on the used input device.
   *
   * Since: 2.12
   *
   * Deprecated: 3.10: Generally, the behavior for touchscreen input should be
   *             performed dynamically based on cdk_event_get_source_device().
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("ctk-keynav-cursor-only",
                                                                   P_("Keynav Cursor Only"),
                                                                   P_("When TRUE, there are only cursor keys available to navigate widgets"),
                                                                   FALSE,
                                                                   CTK_PARAM_READWRITE | G_PARAM_DEPRECATED),
                                             NULL);

  g_assert (result == PROP_KEYNAV_CURSOR_ONLY);

  /**
   * CtkSettings:ctk-keynav-wrap-around:
   *
   * When %TRUE, some widgets will wrap around when doing keyboard
   * navigation, such as menus, menubars and notebooks.
   *
   * Since: 2.12
   *
   * Deprecated: 3.10: This setting is ignored.
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("ctk-keynav-wrap-around",
                                                                   P_("Keynav Wrap Around"),
                                                                   P_("Whether to wrap around when keyboard-navigating widgets"),
                                                                   TRUE,
                                                                   CTK_PARAM_READWRITE | G_PARAM_DEPRECATED),
                                             NULL);

  g_assert (result == PROP_KEYNAV_WRAP_AROUND);

  /**
   * CtkSettings:ctk-error-bell:
   *
   * When %TRUE, keyboard navigation and other input-related errors
   * will cause a beep. Since the error bell is implemented using
   * cdk_window_beep(), the windowing system may offer ways to
   * configure the error bell in many ways, such as flashing the
   * window or similar visual effects.
   *
   * Since: 2.12
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("ctk-error-bell",
                                                                   P_("Error Bell"),
                                                                   P_("When TRUE, keyboard navigation and other errors will cause a beep"),
                                                                   TRUE,
                                                                   CTK_PARAM_READWRITE),
                                             NULL);

  g_assert (result == PROP_ERROR_BELL);

  /**
   * CtkSettings:color-hash: (type GLib.HashTable(utf8,Cdk.Color)) (transfer container)
   *
   * Holds a hash table representation of the #CtkSettings:ctk-color-scheme
   * setting, mapping color names to #CdkColors.
   *
   * Since: 2.10
   *
   * Deprecated: 3.8: Will always return an empty hash table.
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boxed ("color-hash",
                                                                 P_("Color Hash"),
                                                                 P_("A hash table representation of the color scheme."),
                                                                 G_TYPE_HASH_TABLE,
                                                                 CTK_PARAM_READABLE | G_PARAM_DEPRECATED),
                                             NULL);
  g_assert (result == PROP_COLOR_HASH);

  /**
   * CtkSettings:ctk-file-chooser-backend:
   *
   * Name of the CtkFileChooser backend to use by default.
   *
   * Deprecated: 3.10: This setting is ignored. #CtkFileChooser uses GIO by default.
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_string ("ctk-file-chooser-backend",
                                                                  P_("Default file chooser backend"),
                                                                  P_("Name of the CtkFileChooser backend to use by default"),
                                                                  NULL,
                                                                  CTK_PARAM_READWRITE | G_PARAM_DEPRECATED),
                                             NULL);
  g_assert (result == PROP_FILE_CHOOSER_BACKEND);

  /**
   * CtkSettings:ctk-print-backends:
   *
   * A comma-separated list of print backends to use in the print
   * dialog. Available print backends depend on the CTK+ installation,
   * and may include "file", "cups", "lpr" or "papi".
   *
   * Since: 2.10
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_string ("ctk-print-backends",
                                                                  P_("Default print backend"),
                                                                  P_("List of the CtkPrintBackend backends to use by default"),
                                                                  CTK_PRINT_BACKENDS,
                                                                  CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_PRINT_BACKENDS);

  /**
   * CtkSettings:ctk-print-preview-command:
   *
   * A command to run for displaying the print preview. The command
   * should contain a `%f` placeholder, which will get replaced by
   * the path to the pdf file. The command may also contain a `%s`
   * placeholder, which will get replaced by the path to a file
   * containing the print settings in the format produced by
   * ctk_print_settings_to_file().
   *
   * The preview application is responsible for removing the pdf file
   * and the print settings file when it is done.
   *
   * Since: 2.10
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_string ("ctk-print-preview-command",
                                                                  P_("Default command to run when displaying a print preview"),
                                                                  P_("Command to run when displaying a print preview"),
                                                                  PRINT_PREVIEW_COMMAND,
                                                                  CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_PRINT_PREVIEW_COMMAND);

  /**
   * CtkSettings:ctk-enable-mnemonics:
   *
   * Whether labels and menu items should have visible mnemonics which
   * can be activated.
   *
   * Since: 2.12
   *
   * Deprecated: 3.10: This setting can still be used for application
   *      overrides, but will be ignored in the future
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("ctk-enable-mnemonics",
                                                                   P_("Enable Mnemonics"),
                                                                   P_("Whether labels should have mnemonics"),
                                                                   TRUE,
                                                                   CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_ENABLE_MNEMONICS);

  /**
   * CtkSettings:ctk-enable-accels:
   *
   * Whether menu items should have visible accelerators which can be
   * activated.
   *
   * Since: 2.12
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("ctk-enable-accels",
                                                                   P_("Enable Accelerators"),
                                                                   P_("Whether menu items should have accelerators"),
                                                                   TRUE,
                                                                   CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_ENABLE_ACCELS);

  /**
   * CtkSettings:ctk-recent-files-limit:
   *
   * The number of recently used files that should be displayed by default by
   * #CtkRecentChooser implementations and by the #CtkFileChooser. A value of
   * -1 means every recently used file stored.
   *
   * Since: 2.12
   *
   * Deprecated: 3.10: This setting is ignored
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_int ("ctk-recent-files-limit",
                                                               P_("Recent Files Limit"),
                                                               P_("Number of recently used files"),
                                                               -1, G_MAXINT,
                                                               50,
                                                               CTK_PARAM_READWRITE | G_PARAM_DEPRECATED),
                                             NULL);
  g_assert (result == PROP_RECENT_FILES_LIMIT);

  /**
   * CtkSettings:ctk-im-module:
   *
   * Which IM (input method) module should be used by default. This is the
   * input method that will be used if the user has not explicitly chosen
   * another input method from the IM context menu.
   * This also can be a colon-separated list of input methods, which CTK+
   * will try in turn until it finds one available on the system.
   *
   * See #CtkIMContext.
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_string ("ctk-im-module",
                                                                  P_("Default IM module"),
                                                                  P_("Which IM module should be used by default"),
                                                                  NULL,
                                                                  CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_IM_MODULE);

  /**
   * CtkSettings:ctk-recent-files-max-age:
   *
   * The maximum age, in days, of the items inside the recently used
   * resources list. Items older than this setting will be excised
   * from the list. If set to 0, the list will always be empty; if
   * set to -1, no item will be removed.
   *
   * Since: 2.14
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_int ("ctk-recent-files-max-age",
                                                               P_("Recent Files Max Age"),
                                                               P_("Maximum age of recently used files, in days"),
                                                               -1, G_MAXINT,
                                                               30,
                                                               CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_RECENT_FILES_MAX_AGE);

  result = settings_install_property_parser (class,
                                             g_param_spec_uint ("ctk-fontconfig-timestamp",
                                                                P_("Fontconfig configuration timestamp"),
                                                                P_("Timestamp of current fontconfig configuration"),
                                                                0, G_MAXUINT, 0,
                                                                CTK_PARAM_READWRITE),
                                             NULL);

  g_assert (result == PROP_FONTCONFIG_TIMESTAMP);

  /**
   * CtkSettings:ctk-sound-theme-name:
   *
   * The XDG sound theme to use for event sounds.
   *
   * See the [Sound Theme Specifications](http://www.freedesktop.org/wiki/Specifications/sound-theme-spec)
   * for more information on event sounds and sound themes.
   *
   * CTK+ itself does not support event sounds, you have to use a loadable
   * module like the one that comes with libcanberra.
   *
   * Since: 2.14
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_string ("ctk-sound-theme-name",
                                                                  P_("Sound Theme Name"),
                                                                  P_("XDG sound theme name"),
                                                                  "freedesktop",
                                                                  CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_SOUND_THEME_NAME);

  /**
   * CtkSettings:ctk-enable-input-feedback-sounds:
   *
   * Whether to play event sounds as feedback to user input.
   *
   * See the [Sound Theme Specifications](http://www.freedesktop.org/wiki/Specifications/sound-theme-spec)
   * for more information on event sounds and sound themes.
   *
   * CTK+ itself does not support event sounds, you have to use a loadable
   * module like the one that comes with libcanberra.
   *
   * Since: 2.14
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("ctk-enable-input-feedback-sounds",
                                                                   /* Translators: this means sounds that are played as feedback to user input */
                                                                   P_("Audible Input Feedback"),
                                                                   P_("Whether to play event sounds as feedback to user input"),
                                                                   TRUE,
                                                                   CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_ENABLE_INPUT_FEEDBACK_SOUNDS);

  /**
   * CtkSettings:ctk-enable-event-sounds:
   *
   * Whether to play any event sounds at all.
   *
   * See the [Sound Theme Specifications](http://www.freedesktop.org/wiki/Specifications/sound-theme-spec)
   * for more information on event sounds and sound themes.
   *
   * CTK+ itself does not support event sounds, you have to use a loadable
   * module like the one that comes with libcanberra.
   *
   * Since: 2.14
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("ctk-enable-event-sounds",
                                                                   P_("Enable Event Sounds"),
                                                                   P_("Whether to play any event sounds at all"),
                                                                   TRUE,
                                                                   CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_ENABLE_EVENT_SOUNDS);

  /**
   * CtkSettings:ctk-enable-tooltips:
   *
   * Whether tooltips should be shown on widgets.
   *
   * Since: 2.14
   *
   * Deprecated: 3.10: This setting is ignored.
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("ctk-enable-tooltips",
                                                                   P_("Enable Tooltips"),
                                                                   P_("Whether tooltips should be shown on widgets"),
                                                                   TRUE,
                                                                   CTK_PARAM_READWRITE | G_PARAM_DEPRECATED),
                                             NULL);
  g_assert (result == PROP_ENABLE_TOOLTIPS);

  /**
   * CtkSettings:ctk-toolbar-style:
   *
   * The size of icons in default toolbars.
   *
   * Deprecated: 3.10: This setting is ignored.
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_enum ("ctk-toolbar-style",
                                                                   P_("Toolbar style"),
                                                                   P_("Whether default toolbars have text only, text and icons, icons only, etc."),
                                                                   CTK_TYPE_TOOLBAR_STYLE,
                                                                   CTK_TOOLBAR_BOTH_HORIZ,
                                                                   CTK_PARAM_READWRITE | G_PARAM_DEPRECATED),
                                             ctk_rc_property_parse_enum);
  g_assert (result == PROP_TOOLBAR_STYLE);

  /**
   * CtkSettings:ctk-toolbar-icon-size:
   *
   * The size of icons in default toolbars.
   *
   * Deprecated: 3.10: This setting is ignored.
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_enum ("ctk-toolbar-icon-size",
                                                                   P_("Toolbar Icon Size"),
                                                                   P_("The size of icons in default toolbars."),
                                                                   CTK_TYPE_ICON_SIZE,
                                                                   CTK_ICON_SIZE_LARGE_TOOLBAR,
                                                                   CTK_PARAM_READWRITE | G_PARAM_DEPRECATED),
                                             ctk_rc_property_parse_enum);
  g_assert (result == PROP_TOOLBAR_ICON_SIZE);

  /**
   * CtkSettings:ctk-auto-mnemonics:
   *
   * Whether mnemonics should be automatically shown and hidden when the user
   * presses the mnemonic activator.
   *
   * Since: 2.20
   *
   * Deprecated: 3.10: This setting is ignored
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("ctk-auto-mnemonics",
                                                                   P_("Auto Mnemonics"),
                                                                   P_("Whether mnemonics should be automatically shown and hidden when the user presses the mnemonic activator."),
                                                                   TRUE,
                                                                   CTK_PARAM_READWRITE | G_PARAM_DEPRECATED),
                                             NULL);
  g_assert (result == PROP_AUTO_MNEMONICS);

  /**
   * CtkSettings:ctk-primary-button-warps-slider:
   *
   * If the value of this setting is %TRUE, clicking the primary button in a
   * #CtkRange trough will move the slider, and hence set the range’s value, to
   * the point that you clicked. If it is %FALSE, a primary click will cause the
   * slider/value to move by the range’s page-size towards the point clicked.
   *
   * Whichever action you choose for the primary button, the other action will
   * be available by holding Shift and primary-clicking, or (since CTK+ 3.22.25)
   * clicking the middle mouse button.
   *
   * Since: 3.6
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("ctk-primary-button-warps-slider",
                                                                   P_("Primary button warps slider"),
                                                                   P_("Whether a primary click on the trough should warp the slider into position"),
                                                                   TRUE,
                                                                   CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_PRIMARY_BUTTON_WARPS_SLIDER);

  /**
   * CtkSettings:ctk-visible-focus:
   *
   * Whether 'focus rectangles' should be always visible, never visible,
   * or hidden until the user starts to use the keyboard.
   *
   * Since: 3.2
   *
   * Deprecated: 3.10: This setting is ignored
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_enum ("ctk-visible-focus",
                                                                P_("Visible Focus"),
                                                                P_("Whether 'focus rectangles' should be hidden until the user starts to use the keyboard."),
                                                                CTK_TYPE_POLICY_TYPE,
                                                                CTK_POLICY_AUTOMATIC,
                                                                CTK_PARAM_READWRITE | G_PARAM_DEPRECATED),
                                             ctk_rc_property_parse_enum);
  g_assert (result == PROP_VISIBLE_FOCUS);

  /**
   * CtkSettings:ctk-application-prefer-dark-theme:
   *
   * Whether the application prefers to use a dark theme. If a CTK+ theme
   * includes a dark variant, it will be used instead of the configured
   * theme.
   *
   * Some applications benefit from minimizing the amount of light pollution that
   * interferes with the content. Good candidates for dark themes are photo and
   * video editors that make the actual content get all the attention and minimize
   * the distraction of the chrome.
   *
   * Dark themes should not be used for documents, where large spaces are white/light
   * and the dark chrome creates too much contrast (web browser, text editor...).
   *
   * Since: 3.0
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("ctk-application-prefer-dark-theme",
                                                                 P_("Application prefers a dark theme"),
                                                                 P_("Whether the application prefers to have a dark theme."),
                                                                 FALSE,
                                                                 CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_APPLICATION_PREFER_DARK_THEME);

  /**
   * CtkSettings:ctk-button-images:
   *
   * Whether images should be shown on buttons
   *
   * Since: 2.4
   *
   * Deprecated: 3.10: This setting is deprecated. Application developers
   *   control whether a button should show an icon or not, on a
   *   per-button basis. If a #CtkButton should show an icon, use the
   *   #CtkButton:always-show-image property of #CtkButton, and pack a
   *   #CtkImage inside the #CtkButton
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("ctk-button-images",
                                                                   P_("Show button images"),
                                                                   P_("Whether images should be shown on buttons"),
                                                                   FALSE,
                                                                   CTK_PARAM_READWRITE | G_PARAM_DEPRECATED),
                                             NULL);
  g_assert (result == PROP_BUTTON_IMAGES);

  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("ctk-entry-select-on-focus",
                                                                   P_("Select on focus"),
                                                                   P_("Whether to select the contents of an entry when it is focused"),
                                                                   TRUE,
                                                                   CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_ENTRY_SELECT_ON_FOCUS);

  /**
   * CtkSettings:ctk-entry-password-hint-timeout:
   *
   * How long to show the last input character in hidden
   * entries. This value is in milliseconds. 0 disables showing the
   * last char. 600 is a good value for enabling it.
   *
   * Since: 2.10
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_uint ("ctk-entry-password-hint-timeout",
                                                                P_("Password Hint Timeout"),
                                                                P_("How long to show the last input character in hidden entries"),
                                                                0, G_MAXUINT,
                                                                0,
                                                                CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_ENTRY_PASSWORD_HINT_TIMEOUT);

  /**
   * CtkSettings:ctk-menu-images:
   *
   * Whether images should be shown in menu items
   *
   * Deprecated: 3.10: This setting is deprecated. Application developers
   *   control whether or not a #CtkMenuItem should have an icon or not,
   *   on a per widget basis. Either use a #CtkMenuItem with a #CtkBox
   *   containing a #CtkImage and a #CtkAccelLabel, or describe your menus
   *   using a #GMenu XML description
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("ctk-menu-images",
                                                                   P_("Show menu images"),
                                                                   P_("Whether images should be shown in menus"),
                                                                   FALSE,
                                                                   CTK_PARAM_READWRITE | G_PARAM_DEPRECATED),
                                             NULL);
  g_assert (result == PROP_MENU_IMAGES);

  /**
   * CtkSettings:ctk-menu-bar-popup-delay:
   *
   * Delay before the submenus of a menu bar appear.
   *
   * Deprecated: 3.10: This setting is ignored.
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_int ("ctk-menu-bar-popup-delay",
                                                               P_("Delay before drop down menus appear"),
                                                               P_("Delay before the submenus of a menu bar appear"),
                                                               0, G_MAXINT,
                                                               0,
                                                               CTK_PARAM_READWRITE | G_PARAM_DEPRECATED),
                                             NULL);
  g_assert (result == PROP_MENU_BAR_POPUP_DELAY);

  /**
   * CtkSettings:ctk-scrolled-window-placement:
   *
   * Where the contents of scrolled windows are located with respect to the
   * scrollbars, if not overridden by the scrolled window's own placement.
   *
   * Since: 2.10
   *
   * Deprecated: 3.10: This setting is ignored.
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_enum ("ctk-scrolled-window-placement",
                                                                P_("Scrolled Window Placement"),
                                                                P_("Where the contents of scrolled windows are located with respect to the scrollbars, if not overridden by the scrolled window's own placement."),
                                                                CTK_TYPE_CORNER_TYPE,
                                                                CTK_CORNER_TOP_LEFT,
                                                                CTK_PARAM_READWRITE | G_PARAM_DEPRECATED),
                                             ctk_rc_property_parse_enum);
  g_assert (result == PROP_SCROLLED_WINDOW_PLACEMENT);

  /**
   * CtkSettings:ctk-can-change-accels:
   *
   * Whether menu accelerators can be changed by pressing a key over the menu item.
   *
   * Deprecated: 3.10: This setting is ignored.
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("ctk-can-change-accels",
                                                                   P_("Can change accelerators"),
                                                                   P_("Whether menu accelerators can be changed by pressing a key over the menu item"),
                                                                   FALSE,
                                                                   CTK_PARAM_READWRITE | G_PARAM_DEPRECATED),
                                             NULL);
  g_assert (result == PROP_CAN_CHANGE_ACCELS);

  /**
   * CtkSettings:ctk-menu-popup-delay:
   *
   * Minimum time the pointer must stay over a menu item before the submenu appear.
   *
   * Deprecated: 3.10: This setting is ignored.
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_int ("ctk-menu-popup-delay",
                                                               P_("Delay before submenus appear"),
                                                               P_("Minimum time the pointer must stay over a menu item before the submenu appear"),
                                                               0, G_MAXINT,
                                                               225,
                                                               CTK_PARAM_READWRITE | G_PARAM_DEPRECATED),
                                             NULL);
  g_assert (result == PROP_MENU_POPUP_DELAY);

  /**
   * CtkSettings:ctk-menu-popdown-delay:
   *
   * The time before hiding a submenu when the pointer is moving towards the submenu.
   *
   * Deprecated: 3.10: This setting is ignored.
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_int ("ctk-menu-popdown-delay",
                                                               P_("Delay before hiding a submenu"),
                                                               P_("The time before hiding a submenu when the pointer is moving towards the submenu"),
                                                               0, G_MAXINT,
                                                               1000,
                                                               CTK_PARAM_READWRITE | G_PARAM_DEPRECATED),
                                             NULL);
  g_assert (result == PROP_MENU_POPDOWN_DELAY);

  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("ctk-label-select-on-focus",
                                                                   P_("Select on focus"),
                                                                   P_("Whether to select the contents of a selectable label when it is focused"),
                                                                   TRUE,
                                                                   CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_LABEL_SELECT_ON_FOCUS);

  /**
   * CtkSettings:ctk-color-palette:
   *
   * Palette to use in the deprecated color selector.
   *
   * Deprecated: 3.10: Only used by the deprecated color selector widget.
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_string ("ctk-color-palette",
                                                                  P_("Custom palette"),
                                                                  P_("Palette to use in the color selector"),
                                                                  default_color_palette,
                                                                  CTK_PARAM_READWRITE | G_PARAM_DEPRECATED),
                                             NULL);
  g_assert (result == PROP_COLOR_PALETTE);

  /**
   * CtkSettings:ctk-im-preedit-style:
   *
   * How to draw the input method preedit string.
   *
   * Deprecated: 3.10: This setting is ignored.
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_enum ("ctk-im-preedit-style",
                                                                P_("IM Preedit style"),
                                                                P_("How to draw the input method preedit string"),
                                                                CTK_TYPE_IM_PREEDIT_STYLE,
                                                                CTK_IM_PREEDIT_CALLBACK,
                                                                CTK_PARAM_READWRITE | G_PARAM_DEPRECATED),
                                             ctk_rc_property_parse_enum);
  g_assert (result == PROP_IM_PREEDIT_STYLE);

  /**
   * CtkSettings:ctk-im-status-style:
   *
   * How to draw the input method statusbar.
   *
   * Deprecated: 3.10: This setting is ignored.
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_enum ("ctk-im-status-style",
                                                                P_("IM Status style"),
                                                                P_("How to draw the input method statusbar"),
                                                                CTK_TYPE_IM_STATUS_STYLE,
                                                                CTK_IM_STATUS_CALLBACK,
                                                                CTK_PARAM_READWRITE | G_PARAM_DEPRECATED),
                                             ctk_rc_property_parse_enum);
  g_assert (result == PROP_IM_STATUS_STYLE);

  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("ctk-shell-shows-app-menu",
                                                                   P_("Desktop shell shows app menu"),
                                                                   P_("Set to TRUE if the desktop environment "
                                                                      "is displaying the app menu, FALSE if "
                                                                      "the app should display it itself."),
                                                                   FALSE, CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_SHELL_SHOWS_APP_MENU);

  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("ctk-shell-shows-menubar",
                                                                   P_("Desktop shell shows the menubar"),
                                                                   P_("Set to TRUE if the desktop environment "
                                                                      "is displaying the menubar, FALSE if "
                                                                      "the app should display it itself."),
                                                                   FALSE, CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_SHELL_SHOWS_MENUBAR);

  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("ctk-shell-shows-desktop",
                                                                   P_("Desktop environment shows the desktop folder"),
                                                                   P_("Set to TRUE if the desktop environment "
                                                                      "is displaying the desktop folder, FALSE "
                                                                      "if not."),
                                                                   TRUE, CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_SHELL_SHOWS_DESKTOP);

  /**
   * CtkSettings:ctk-decoration-layout:
   *
   * This setting determines which buttons should be put in the
   * titlebar of client-side decorated windows, and whether they
   * should be placed at the left of right.
   *
   * The format of the string is button names, separated by commas.
   * A colon separates the buttons that should appear on the left
   * from those on the right. Recognized button names are minimize,
   * maximize, close, icon (the window icon) and menu (a menu button
   * for the fallback app menu).
   *
   * For example, "menu:minimize,maximize,close" specifies a menu
   * on the left, and minimize, maximize and close buttons on the right.
   *
   * Note that buttons will only be shown when they are meaningful.
   * E.g. a menu button only appears when the desktop shell does not
   * show the app menu, and a close button only appears on a window
   * that can be closed.
   *
   * Also note that the setting can be overridden with the
   * #CtkHeaderBar:decoration-layout property.
   *
   * Since: 3.12
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_string ("ctk-decoration-layout",
                                                                  P_("Decoration Layout"),
                                                                   P_("The layout for window decorations"),
                                                                   "menu:minimize,maximize,close", CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_DECORATION_LAYOUT);

  /**
   * CtkSettings:ctk-titlebar-double-click:
   *
   * This setting determines the action to take when a double-click
   * occurs on the titlebar of client-side decorated windows.
   *
   * Recognized actions are minimize, toggle-maximize, menu, lower
   * or none.
   *
   * Since: 3.14
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_string ("ctk-titlebar-double-click",
                                                                  P_("Titlebar double-click action"),
                                                                   P_("The action to take on titlebar double-click"),
                                                                   "toggle-maximize", CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_TITLEBAR_DOUBLE_CLICK);

  /**
   * CtkSettings:ctk-titlebar-middle-click:
   *
   * This setting determines the action to take when a middle-click
   * occurs on the titlebar of client-side decorated windows.
   *
   * Recognized actions are minimize, toggle-maximize, menu, lower
   * or none.
   *
   * Since: 3.14
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_string ("ctk-titlebar-middle-click",
                                                                  P_("Titlebar middle-click action"),
                                                                   P_("The action to take on titlebar middle-click"),
                                                                   "none", CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_TITLEBAR_MIDDLE_CLICK);

  /**
   * CtkSettings:ctk-titlebar-right-click:
   *
   * This setting determines the action to take when a right-click
   * occurs on the titlebar of client-side decorated windows.
   *
   * Recognized actions are minimize, toggle-maximize, menu, lower
   * or none.
   *
   * Since: 3.14
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_string ("ctk-titlebar-right-click",
                                                                  P_("Titlebar right-click action"),
                                                                   P_("The action to take on titlebar right-click"),
                                                                   "menu", CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_TITLEBAR_RIGHT_CLICK);




  /**
   * CtkSettings:ctk-dialogs-use-header:
   *
   * Whether builtin CTK+ dialogs such as the file chooser, the
   * color chooser or the font chooser will use a header bar at
   * the top to show action widgets, or an action area at the bottom.
   *
   * This setting does not affect custom dialogs using CtkDialog
   * directly, or message dialogs.
   *
   * Since: 3.12
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("ctk-dialogs-use-header",
                                                                   P_("Dialogs use header bar"),
                                                                   P_("Whether builtin CTK+ dialogs should use a header bar instead of an action area."),
                                                                   FALSE,
                                                                   CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_DIALOGS_USE_HEADER);

  /**
   * CtkSettings:ctk-enable-primary-paste:
   *
   * Whether a middle click on a mouse should paste the
   * 'PRIMARY' clipboard content at the cursor location.
   *
   * Since: 3.4
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("ctk-enable-primary-paste",
                                                                   P_("Enable primary paste"),
                                                                   P_("Whether a middle click on a mouse should paste the 'PRIMARY' clipboard content at the cursor location."),
                                                                   TRUE,
                                                                   CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_ENABLE_PRIMARY_PASTE);

  /**
   * CtkSettings:ctk-recent-files-enabled:
   *
   * Whether CTK+ should keep track of items inside the recently used
   * resources list. If set to %FALSE, the list will always be empty.
   *
   * Since: 3.8
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("ctk-recent-files-enabled",
                                                                   P_("Recent Files Enabled"),
                                                                   P_("Whether CTK+ remembers recent files"),
                                                                   TRUE,
                                                                   CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_RECENT_FILES_ENABLED);

  /**
   * CtkSettings:ctk-long-press-time:
   *
   * The time for a button or touch press to be considered a "long press".
   *
   * Since: 3.14
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_uint ("ctk-long-press-time",
								P_("Long press time"),
								P_("Time for a button/touch press to be considered a long press (in milliseconds)"),
								0, G_MAXINT, 500,
								CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_LONG_PRESS_TIME);

  /**
   * CtkSettings:ctk-keynav-use-caret:
   *
   * Whether CTK+ should make sure that text can be navigated with
   * a caret, even if it is not editable. This is useful when using
   * a screen reader.
   *
   * Since: 3.20
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("ctk-keynav-use-caret",
                                                                   P_("Whether to show cursor in text"),
                                                                   P_("Whether to show cursor in text"),
                                                                   FALSE,
                                                                   CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_KEYNAV_USE_CARET);

  /**
   * CtkSettings:ctk-overlay-scrolling:
   *
   * Whether scrolled windows may use overlayed scrolling indicators.
   * If this is set to %FALSE, scrolled windows will have permanent
   * scrollbars.
   *
   * Since: 3.24.9
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("ctk-overlay-scrolling",
                                                                   P_("Whether to use overlay scrollbars"),
                                                                   P_("Whether to use overlay scrollbars"),
                                                                   TRUE,
                                                                   CTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_OVERLAY_SCROLLING);
}

static void
ctk_settings_provider_iface_init (CtkStyleProviderIface *iface)
{
}

static CtkSettings *
ctk_settings_style_provider_get_settings (CtkStyleProviderPrivate *provider)
{
  return CTK_SETTINGS (provider);
}

static void
ctk_settings_provider_private_init (CtkStyleProviderPrivateInterface *iface)
{
  iface->get_settings = ctk_settings_style_provider_get_settings;
}

static void
ctk_settings_finalize (GObject *object)
{
  CtkSettings *settings = CTK_SETTINGS (object);
  CtkSettingsPrivate *priv = settings->priv;
  guint i;

  object_list = g_slist_remove (object_list, settings);

  for (i = 0; i < class_n_properties; i++)
    g_value_unset (&priv->property_values[i].value);
  g_free (priv->property_values);

  g_datalist_clear (&priv->queued_settings);

  settings_update_provider (priv->screen, &priv->theme_provider, NULL);
  settings_update_provider (priv->screen, &priv->key_theme_provider, NULL);
  g_slist_free_full (priv->style_cascades, g_object_unref);

  g_free (priv->font_family);

  G_OBJECT_CLASS (ctk_settings_parent_class)->finalize (object);
}

CtkStyleCascade *
_ctk_settings_get_style_cascade (CtkSettings *settings,
                                 gint         scale)
{
  CtkSettingsPrivate *priv;
  CtkStyleCascade *new_cascade;
  GSList *list;

  g_return_val_if_fail (CTK_IS_SETTINGS (settings), NULL);

  priv = settings->priv;
  for (list = priv->style_cascades; list; list = list->next)
    {
      if (_ctk_style_cascade_get_scale (list->data) == scale)
        return list->data;
    }

  /* We are guaranteed to have the special cascade with scale == 1.
   * It's created in ctk_settings_init()
   */
  g_assert (scale != 1);

  new_cascade = _ctk_style_cascade_new ();
  _ctk_style_cascade_set_parent (new_cascade, _ctk_settings_get_style_cascade (settings, 1));
  _ctk_style_cascade_set_scale (new_cascade, scale);

  priv->style_cascades = g_slist_prepend (priv->style_cascades, new_cascade);

  return new_cascade;
}

static void
settings_init_style (CtkSettings *settings)
{
  static CtkCssProvider *css_provider = NULL;
  CtkStyleCascade *cascade;

  /* Add provider for user file */
  if (G_UNLIKELY (!css_provider))
    {
      gchar *css_path;

      css_provider = ctk_css_provider_new ();
      css_path = g_build_filename (g_get_user_config_dir (),
                                   "ctk-3.0",
                                   "ctk.css",
                                   NULL);

      if (g_file_test (css_path,
                       G_FILE_TEST_IS_REGULAR))
        ctk_css_provider_load_from_path (css_provider, css_path, NULL);

      g_free (css_path);
    }

  cascade = _ctk_settings_get_style_cascade (settings, 1);
  _ctk_style_cascade_add_provider (cascade,
                                   CTK_STYLE_PROVIDER (css_provider),
                                   CTK_STYLE_PROVIDER_PRIORITY_USER);

  _ctk_style_cascade_add_provider (cascade,
                                   CTK_STYLE_PROVIDER (settings),
                                   CTK_STYLE_PROVIDER_PRIORITY_SETTINGS);

  _ctk_style_cascade_add_provider (cascade,
                                   CTK_STYLE_PROVIDER (settings->priv->theme_provider),
                                   CTK_STYLE_PROVIDER_PRIORITY_SETTINGS);

  settings_update_theme (settings);
  settings_update_key_theme (settings);
}

static void
settings_display_closed (CdkDisplay *display,
                         gboolean    is_error,
                         gpointer    data)
{
  DisplaySettings *ds;
  int i;

  if (G_UNLIKELY (display_settings == NULL))
    return;

  ds = (DisplaySettings *)display_settings->data;
  for (i = 0; i < display_settings->len; i++)
    {
      if (ds[i].display == display)
        {
          g_clear_object (&ds[i].settings);
          display_settings = g_array_remove_index_fast (display_settings, i);
          break;
        }
    }
}

static CtkSettings *
ctk_settings_create_for_display (CdkDisplay *display)
{
  CtkSettings *settings;
  DisplaySettings v;

#ifdef CDK_WINDOWING_QUARTZ
  if (CDK_IS_QUARTZ_DISPLAY (display))
    settings = g_object_new (CTK_TYPE_SETTINGS,
                             "ctk-key-theme-name", "Mac",
                             "ctk-shell-shows-app-menu", TRUE,
                             "ctk-shell-shows-menubar", TRUE,
                             NULL);
  else
#endif
#ifdef CDK_WINDOWING_BROADWAY
    if (CDK_IS_BROADWAY_DISPLAY (display))
      settings = g_object_new (CTK_TYPE_SETTINGS,
                               "ctk-im-module", "broadway",
                               NULL);
  else
#endif
#ifdef CDK_WINDOWING_WAYLAND
    if (CDK_IS_WAYLAND_DISPLAY (display))
      {
        if (cdk_wayland_display_query_registry (display,
                                                "zwp_text_input_manager_v3"))
          {
            settings = g_object_new (CTK_TYPE_SETTINGS,
                                     "ctk-im-module", "wayland",
                                     NULL);
          }
        else if (cdk_wayland_display_query_registry (display,
                                                "ctk_text_input_manager"))
          {
            settings = g_object_new (CTK_TYPE_SETTINGS,
                                     "ctk-im-module", "waylandctk",
                                     NULL);
          }
        else
          {
            /* Fallback to other IM methods if the compositor does not
             * implement the interface(s).
             */
            settings = g_object_new (CTK_TYPE_SETTINGS, NULL);
          }
      }
  else
#endif
    settings = g_object_new (CTK_TYPE_SETTINGS, NULL);

  settings->priv->screen = cdk_display_get_default_screen (display);

  v.display = display;
  v.settings = settings;
  display_settings = g_array_append_val (display_settings, v);
  g_signal_connect (display, "closed",
                    G_CALLBACK (settings_display_closed), NULL);

  settings_init_style (settings);
  settings_update_xsettings (settings);
  settings_update_modules (settings);
  settings_update_double_click (settings);
  settings_update_cursor_theme (settings);
  settings_update_resolution (settings);
  settings_update_font_options (settings);
  settings_update_font_values (settings);

  return settings;
}

static CtkSettings *
ctk_settings_get_for_display (CdkDisplay *display)
{
  DisplaySettings *ds;
  int i;

  /* If the display is closed, we don't want to recreate the settings! */
  if G_UNLIKELY (cdk_display_is_closed (display))
    return NULL;

  if G_UNLIKELY (display_settings == NULL)
    display_settings = g_array_new (FALSE, TRUE, sizeof (DisplaySettings));

  ds = (DisplaySettings *)display_settings->data;
  for (i = 0; i < display_settings->len; i++)
    {
      if (ds[i].display == display)
        return ds[i].settings;
    }

  return ctk_settings_create_for_display (display);
}

/**
 * ctk_settings_get_for_screen:
 * @screen: a #CdkScreen.
 *
 * Gets the #CtkSettings object for @screen, creating it if necessary.
 *
 * Returns: (transfer none): a #CtkSettings object.
 *
 * Since: 2.2
 */
CtkSettings *
ctk_settings_get_for_screen (CdkScreen *screen)
{
  g_return_val_if_fail (CDK_IS_SCREEN (screen), NULL);

  return ctk_settings_get_for_display (cdk_screen_get_display (screen));
}

/**
 * ctk_settings_get_default:
 *
 * Gets the #CtkSettings object for the default CDK screen, creating
 * it if necessary. See ctk_settings_get_for_screen().
 *
 * Returns: (nullable) (transfer none): a #CtkSettings object. If there is
 * no default screen, then returns %NULL.
 **/
CtkSettings*
ctk_settings_get_default (void)
{
  CdkDisplay *display = cdk_display_get_default ();

  if (display)
    return ctk_settings_get_for_display (display);
  else
    return NULL;
}

static void
ctk_settings_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  CtkSettings *settings = CTK_SETTINGS (object);
  CtkSettingsPrivate *priv = settings->priv;

  g_value_copy (value, &priv->property_values[property_id - 1].value);
  priv->property_values[property_id - 1].source = CTK_SETTINGS_SOURCE_APPLICATION;
}

static void
settings_invalidate_style (CtkSettings *settings)
{
  _ctk_style_provider_private_changed (CTK_STYLE_PROVIDER_PRIVATE (settings));
}

static void
settings_update_font_values (CtkSettings *settings)
{
  CtkSettingsPrivate *priv = settings->priv;
  PangoFontDescription *desc;
  const gchar *font_name;

  font_name = g_value_get_string (&priv->property_values[PROP_FONT_NAME - 1].value);
  desc = pango_font_description_from_string (font_name);

  if (desc != NULL &&
      (pango_font_description_get_set_fields (desc) & PANGO_FONT_MASK_SIZE) != 0)
    {
      priv->font_size = pango_font_description_get_size (desc);
      priv->font_size_absolute = pango_font_description_get_size_is_absolute (desc);
    }
  else
    {
      priv->font_size = 10 * PANGO_SCALE;
      priv->font_size_absolute = FALSE;
    }

  g_free (priv->font_family);

  if (desc != NULL &&
      (pango_font_description_get_set_fields (desc) & PANGO_FONT_MASK_FAMILY) != 0)
    {
      priv->font_family = g_strdup (pango_font_description_get_family (desc));
    }
  else
    {
      priv->font_family = g_strdup ("Sans");
    }

  if (desc)
    pango_font_description_free (desc);
}

static void
ctk_settings_notify (GObject    *object,
                     GParamSpec *pspec)
{
  CtkSettings *settings = CTK_SETTINGS (object);
  CtkSettingsPrivate *priv = settings->priv;
  guint property_id = pspec->param_id;

  if (priv->screen == NULL) /* initialization */
    return;

  switch (property_id)
    {
    case PROP_MODULES:
      settings_update_modules (settings);
      break;
    case PROP_DOUBLE_CLICK_TIME:
    case PROP_DOUBLE_CLICK_DISTANCE:
      settings_update_double_click (settings);
      break;
    case PROP_FONT_NAME:
      settings_update_font_values (settings);
      settings_invalidate_style (settings);
      ctk_style_context_reset_widgets (priv->screen);
      break;
    case PROP_KEY_THEME_NAME:
      settings_update_key_theme (settings);
      break;
    case PROP_THEME_NAME:
    case PROP_APPLICATION_PREFER_DARK_THEME:
      settings_update_theme (settings);
      break;
    case PROP_XFT_DPI:
      settings_update_resolution (settings);
      /* This is a hack because with ctk_rc_reset_styles() doesn't get
       * widgets with ctk_widget_style_set(), and also causes more
       * recomputation than necessary.
       */
      ctk_style_context_reset_widgets (priv->screen);
      break;
    case PROP_XFT_ANTIALIAS:
    case PROP_XFT_HINTING:
    case PROP_XFT_HINTSTYLE:
    case PROP_XFT_RGBA:
      settings_update_font_options (settings);
      ctk_style_context_reset_widgets (priv->screen);
      break;
    case PROP_FONTCONFIG_TIMESTAMP:
      if (settings_update_fontconfig (settings))
        ctk_style_context_reset_widgets (priv->screen);
      break;
    case PROP_ENABLE_ANIMATIONS:
      ctk_style_context_reset_widgets (priv->screen);
      break;
    case PROP_CURSOR_THEME_NAME:
    case PROP_CURSOR_THEME_SIZE:
      settings_update_cursor_theme (settings);
      break;
    }
}

gboolean
_ctk_settings_parse_convert (CtkRcPropertyParser parser,
                             const GValue       *src_value,
                             GParamSpec         *pspec,
                             GValue             *dest_value)
{
  gboolean success = FALSE;

  g_return_val_if_fail (G_VALUE_HOLDS (dest_value, G_PARAM_SPEC_VALUE_TYPE (pspec)), FALSE);

  if (parser)
    {
      GString *gstring;
      gboolean free_gstring = TRUE;

      if (G_VALUE_HOLDS (src_value, G_TYPE_GSTRING))
        {
          gstring = g_value_get_boxed (src_value);
          free_gstring = FALSE;
        }
      else if (G_VALUE_HOLDS_LONG (src_value))
        {
          gstring = g_string_new (NULL);
          g_string_append_printf (gstring, "%ld", g_value_get_long (src_value));
        }
      else if (G_VALUE_HOLDS_DOUBLE (src_value))
        {
          gstring = g_string_new (NULL);
          g_string_append_printf (gstring, "%f", g_value_get_double (src_value));
        }
      else if (G_VALUE_HOLDS_STRING (src_value))
        {
          gchar *tstr = g_strescape (g_value_get_string (src_value), NULL);

          gstring = g_string_new (NULL);
          g_string_append_c (gstring, '\"');
          g_string_append (gstring, tstr);
          g_string_append_c (gstring, '\"');
          g_free (tstr);
        }
      else
        {
          g_return_val_if_fail (G_VALUE_HOLDS (src_value, G_TYPE_GSTRING), FALSE);
          gstring = NULL; /* silence compiler */
        }

      success = (parser (pspec, gstring, dest_value) &&
                 !g_param_value_validate (pspec, dest_value));

      if (free_gstring)
        g_string_free (gstring, TRUE);
    }
  else if (G_VALUE_HOLDS (src_value, G_TYPE_GSTRING))
    {
      if (G_VALUE_HOLDS (dest_value, G_TYPE_STRING))
        {
          GString *gstring = g_value_get_boxed (src_value);

          g_value_set_string (dest_value, gstring ? gstring->str : NULL);
          success = !g_param_value_validate (pspec, dest_value);
        }
    }
  else if (g_value_type_transformable (G_VALUE_TYPE (src_value), G_VALUE_TYPE (dest_value)))
    success = g_param_value_convert (pspec, src_value, dest_value, TRUE);

  return success;
}

static void
apply_queued_setting (CtkSettings             *settings,
                      GParamSpec              *pspec,
                      CtkSettingsValuePrivate *qvalue)
{
  CtkSettingsPrivate *priv = settings->priv;
  GValue tmp_value = G_VALUE_INIT;
  CtkRcPropertyParser parser = (CtkRcPropertyParser) g_param_spec_get_qdata (pspec, quark_property_parser);

  g_value_init (&tmp_value, G_PARAM_SPEC_VALUE_TYPE (pspec));
  if (_ctk_settings_parse_convert (parser, &qvalue->public.value,
                                   pspec, &tmp_value))
    {
      if (priv->property_values[pspec->param_id - 1].source <= qvalue->source)
        {
          g_value_copy (&tmp_value, &priv->property_values[pspec->param_id - 1].value);
          priv->property_values[pspec->param_id - 1].source = qvalue->source;
          g_object_notify_by_pspec (G_OBJECT (settings), pspec);
        }

    }
  else
    {
      gchar *debug = g_strdup_value_contents (&qvalue->public.value);

      g_message ("%s: failed to retrieve property '%s' of type '%s' from rc file value \"%s\" of type '%s'",
                 qvalue->public.origin ? qvalue->public.origin : "(for origin information, set CTK_DEBUG)",
                 pspec->name,
                 g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)),
                 debug,
                 G_VALUE_TYPE_NAME (&tmp_value));
      g_free (debug);
    }
  g_value_unset (&tmp_value);
}

static guint
settings_install_property_parser (CtkSettingsClass   *class,
                                  GParamSpec         *pspec,
                                  CtkRcPropertyParser parser)
{
  GSList *node, *next;

  switch (G_TYPE_FUNDAMENTAL (G_PARAM_SPEC_VALUE_TYPE (pspec)))
    {
    case G_TYPE_BOOLEAN:
    case G_TYPE_UCHAR:
    case G_TYPE_CHAR:
    case G_TYPE_UINT:
    case G_TYPE_INT:
    case G_TYPE_ULONG:
    case G_TYPE_LONG:
    case G_TYPE_FLOAT:
    case G_TYPE_DOUBLE:
    case G_TYPE_STRING:
    case G_TYPE_ENUM:
      break;
    case G_TYPE_BOXED:
      if (strcmp (g_param_spec_get_name (pspec), "color-hash") == 0)
        {
          break;
        }
      /* fall through */
    default:
      if (!parser)
        {
          g_warning (G_STRLOC ": parser needs to be specified for property \"%s\" of type '%s'",
                     pspec->name, g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)));
          return 0;
        }
    }
  if (g_object_class_find_property (G_OBJECT_CLASS (class), pspec->name))
    {
      g_warning (G_STRLOC ": an rc-data property \"%s\" already exists",
                 pspec->name);
      return 0;
    }

  for (node = object_list; node; node = node->next)
    g_object_freeze_notify (node->data);

  g_object_class_install_property (G_OBJECT_CLASS (class), ++class_n_properties, pspec);
  g_param_spec_set_qdata (pspec, quark_property_parser, (gpointer) parser);

  for (node = object_list; node; node = node->next)
    {
      CtkSettings *settings = node->data;
      CtkSettingsPrivate *priv = settings->priv;
      CtkSettingsValuePrivate *qvalue;

      priv->property_values = g_renew (CtkSettingsPropertyValue, priv->property_values, class_n_properties);
      priv->property_values[class_n_properties - 1].value.g_type = 0;
      g_value_init (&priv->property_values[class_n_properties - 1].value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      g_param_value_set_default (pspec, &priv->property_values[class_n_properties - 1].value);
      priv->property_values[class_n_properties - 1].source = CTK_SETTINGS_SOURCE_DEFAULT;
      g_object_notify_by_pspec (G_OBJECT (settings), pspec);

      qvalue = g_datalist_id_dup_data (&priv->queued_settings, g_param_spec_get_name_quark (pspec), NULL, NULL);
      if (qvalue)
        apply_queued_setting (settings, pspec, qvalue);
    }

  for (node = object_list; node; node = next)
    {
      next = node->next;
      g_object_thaw_notify (node->data);
    }

  return class_n_properties;
}

CtkRcPropertyParser
_ctk_rc_property_parser_from_type (GType type)
{
  if (type == g_type_from_name ("CdkColor"))
    return ctk_rc_property_parse_color;
  else if (type == CTK_TYPE_REQUISITION)
    return ctk_rc_property_parse_requisition;
  else if (type == CTK_TYPE_BORDER)
    return ctk_rc_property_parse_border;
  else if (G_TYPE_FUNDAMENTAL (type) == G_TYPE_ENUM && G_TYPE_IS_DERIVED (type))
    return ctk_rc_property_parse_enum;
  else if (G_TYPE_FUNDAMENTAL (type) == G_TYPE_FLAGS && G_TYPE_IS_DERIVED (type))
    return ctk_rc_property_parse_flags;
  else
    return NULL;
}

/**
 * ctk_settings_install_property:
 * @pspec:
 *
 * Deprecated: 3.16: This function is not useful outside CTK+.
 */
void
ctk_settings_install_property (GParamSpec *pspec)
{
  static CtkSettingsClass *klass = NULL;

  CtkRcPropertyParser parser;

  g_return_if_fail (G_IS_PARAM_SPEC (pspec));

  if (! klass)
    klass = g_type_class_ref (CTK_TYPE_SETTINGS);

  parser = _ctk_rc_property_parser_from_type (G_PARAM_SPEC_VALUE_TYPE (pspec));

  settings_install_property_parser (klass, pspec, parser);
}

/**
 * ctk_settings_install_property_parser:
 * @pspec:
 * @parser: (scope call):
 *
 * Deprecated: 3.16: This function is not useful outside CTK+.
 */
void
ctk_settings_install_property_parser (GParamSpec          *pspec,
                                      CtkRcPropertyParser  parser)
{
  static CtkSettingsClass *klass = NULL;

  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  g_return_if_fail (parser != NULL);

  if (! klass)
    klass = g_type_class_ref (CTK_TYPE_SETTINGS);

  settings_install_property_parser (klass, pspec, parser);
}

static void
free_value (gpointer data)
{
  CtkSettingsValuePrivate *qvalue = data;

  g_value_unset (&qvalue->public.value);
  g_free (qvalue->public.origin);
  g_slice_free (CtkSettingsValuePrivate, qvalue);
}

static void
ctk_settings_set_property_value_internal (CtkSettings            *settings,
                                          const gchar            *prop_name,
                                          const CtkSettingsValue *new_value,
                                          CtkSettingsSource       source)
{
  CtkSettingsPrivate *priv = settings->priv;
  CtkSettingsValuePrivate *qvalue;
  GParamSpec *pspec;
  gchar *name;
  GQuark name_quark;

  if (!G_VALUE_HOLDS_LONG (&new_value->value) &&
      !G_VALUE_HOLDS_DOUBLE (&new_value->value) &&
      !G_VALUE_HOLDS_STRING (&new_value->value) &&
      !G_VALUE_HOLDS (&new_value->value, G_TYPE_GSTRING))
    {
      g_warning (G_STRLOC ": value type invalid (%s)", g_type_name (G_VALUE_TYPE (&new_value->value)));
      return;
    }

  name = g_strdup (prop_name);
  g_strcanon (name, G_CSET_DIGITS "-" G_CSET_a_2_z G_CSET_A_2_Z, '-');
  name_quark = g_quark_from_string (name);
  g_free (name);

  qvalue = g_datalist_id_dup_data (&priv->queued_settings, name_quark, NULL, NULL);
  if (!qvalue)
    {
      qvalue = g_slice_new0 (CtkSettingsValuePrivate);
      g_datalist_id_set_data_full (&priv->queued_settings, name_quark, qvalue, free_value);
    }
  else
    {
      g_free (qvalue->public.origin);
      g_value_unset (&qvalue->public.value);
    }
  qvalue->public.origin = g_strdup (new_value->origin);
  g_value_init (&qvalue->public.value, G_VALUE_TYPE (&new_value->value));
  g_value_copy (&new_value->value, &qvalue->public.value);
  qvalue->source = source;
  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (settings), g_quark_to_string (name_quark));
  if (pspec)
    apply_queued_setting (settings, pspec, qvalue);
}

/**
 * ctk_settings_set_property_value:
 * @settings:
 * @name:
 * @svalue:
 *
 * Deprecated: 3.16: Use g_object_set() instead.
 */
void
ctk_settings_set_property_value (CtkSettings            *settings,
                                 const gchar            *name,
                                 const CtkSettingsValue *svalue)
{
  g_return_if_fail (CTK_SETTINGS (settings));
  g_return_if_fail (name != NULL);
  g_return_if_fail (svalue != NULL);

  ctk_settings_set_property_value_internal (settings, name, svalue,
                                            CTK_SETTINGS_SOURCE_APPLICATION);
}

void
_ctk_settings_set_property_value_from_rc (CtkSettings            *settings,
                                          const gchar            *prop_name,
                                          const CtkSettingsValue *new_value)
{
  g_return_if_fail (CTK_SETTINGS (settings));
  g_return_if_fail (prop_name != NULL);
  g_return_if_fail (new_value != NULL);

  ctk_settings_set_property_value_internal (settings, prop_name, new_value,
                                            CTK_SETTINGS_SOURCE_THEME);
}

/**
 * ctk_settings_set_string_property:
 * @settings:
 * @name:
 * @v_string:
 * @origin:
 *
 * Deprecated: 3.16: Use g_object_set() instead.
 */
void
ctk_settings_set_string_property (CtkSettings *settings,
                                  const gchar *name,
                                  const gchar *v_string,
                                  const gchar *origin)
{
  CtkSettingsValue svalue = { NULL, { 0, }, };

  g_return_if_fail (CTK_SETTINGS (settings));
  g_return_if_fail (name != NULL);
  g_return_if_fail (v_string != NULL);

  svalue.origin = (gchar*) origin;
  g_value_init (&svalue.value, G_TYPE_STRING);
  g_value_set_static_string (&svalue.value, v_string);
  ctk_settings_set_property_value_internal (settings, name, &svalue,
                                            CTK_SETTINGS_SOURCE_APPLICATION);
  g_value_unset (&svalue.value);
}

/**
 * ctk_settings_set_long_property:
 * @settings:
 * @name:
 * @v_long:
 * @origin:
 *
 * Deprecated: 3.16: Use g_object_set() instead.
 */
void
ctk_settings_set_long_property (CtkSettings *settings,
                                const gchar *name,
                                glong        v_long,
                                const gchar *origin)
{
  CtkSettingsValue svalue = { NULL, { 0, }, };

  g_return_if_fail (CTK_SETTINGS (settings));
  g_return_if_fail (name != NULL);

  svalue.origin = (gchar*) origin;
  g_value_init (&svalue.value, G_TYPE_LONG);
  g_value_set_long (&svalue.value, v_long);
  ctk_settings_set_property_value_internal (settings, name, &svalue,
                                            CTK_SETTINGS_SOURCE_APPLICATION);
  g_value_unset (&svalue.value);
}

/**
 * ctk_settings_set_double_property:
 * @settings:
 * @name:
 * @v_double:
 * @origin:
 *
 * Deprecated: 3.16: Use g_object_set() instead.
 */
void
ctk_settings_set_double_property (CtkSettings *settings,
                                  const gchar *name,
                                  gdouble      v_double,
                                  const gchar *origin)
{
  CtkSettingsValue svalue = { NULL, { 0, }, };

  g_return_if_fail (CTK_SETTINGS (settings));
  g_return_if_fail (name != NULL);

  svalue.origin = (gchar*) origin;
  g_value_init (&svalue.value, G_TYPE_DOUBLE);
  g_value_set_double (&svalue.value, v_double);
  ctk_settings_set_property_value_internal (settings, name, &svalue,
                                            CTK_SETTINGS_SOURCE_APPLICATION);
  g_value_unset (&svalue.value);
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * ctk_rc_property_parse_color:
 * @pspec: a #GParamSpec
 * @gstring: the #GString to be parsed
 * @property_value: a #GValue which must hold #CdkColor values.
 *
 * A #CtkRcPropertyParser for use with ctk_settings_install_property_parser()
 * or ctk_widget_class_install_style_property_parser() which parses a
 * color given either by its name or in the form
 * `{ red, green, blue }` where red, green and
 * blue are integers between 0 and 65535 or floating-point numbers
 * between 0 and 1.
 *
 * Returns: %TRUE if @gstring could be parsed and @property_value
 * has been set to the resulting #CdkColor.
 **/
gboolean
ctk_rc_property_parse_color (const GParamSpec *pspec,
                             const GString    *gstring,
                             GValue           *property_value)
{
  CdkColor color = { 0, 0, 0, 0, };
  GScanner *scanner;
  gboolean success;

  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), FALSE);
  g_return_val_if_fail (G_VALUE_HOLDS (property_value, CDK_TYPE_COLOR), FALSE);

  scanner = ctk_rc_scanner_new ();
  g_scanner_input_text (scanner, gstring->str, gstring->len);
  if (ctk_rc_parse_color (scanner, &color) == G_TOKEN_NONE &&
      g_scanner_get_next_token (scanner) == G_TOKEN_EOF)
    {
      g_value_set_boxed (property_value, &color);
      success = TRUE;
    }
  else
    success = FALSE;
  g_scanner_destroy (scanner);

  return success;
}

/**
 * ctk_rc_property_parse_enum:
 * @pspec: a #GParamSpec
 * @gstring: the #GString to be parsed
 * @property_value: a #GValue which must hold enum values.
 *
 * A #CtkRcPropertyParser for use with ctk_settings_install_property_parser()
 * or ctk_widget_class_install_style_property_parser() which parses a single
 * enumeration value.
 *
 * The enumeration value can be specified by its name, its nickname or
 * its numeric value. For consistency with flags parsing, the value
 * may be surrounded by parentheses.
 *
 * Returns: %TRUE if @gstring could be parsed and @property_value
 * has been set to the resulting #GEnumValue.
 **/
gboolean
ctk_rc_property_parse_enum (const GParamSpec *pspec,
                            const GString    *gstring,
                            GValue           *property_value)
{
  gboolean need_closing_brace = FALSE, success = FALSE;
  GScanner *scanner;
  GEnumValue *enum_value = NULL;

  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), FALSE);
  g_return_val_if_fail (G_VALUE_HOLDS_ENUM (property_value), FALSE);

  scanner = ctk_rc_scanner_new ();
  g_scanner_input_text (scanner, gstring->str, gstring->len);

  /* we just want to parse _one_ value, but for consistency with flags parsing
   * we support optional parenthesis
   */
  g_scanner_get_next_token (scanner);
  if (scanner->token == '(')
    {
      need_closing_brace = TRUE;
      g_scanner_get_next_token (scanner);
    }
  if (scanner->token == G_TOKEN_IDENTIFIER)
    {
      GEnumClass *class = G_PARAM_SPEC_ENUM (pspec)->enum_class;

      enum_value = g_enum_get_value_by_name (class, scanner->value.v_identifier);
      if (!enum_value)
        enum_value = g_enum_get_value_by_nick (class, scanner->value.v_identifier);
      if (enum_value)
        {
          g_value_set_enum (property_value, enum_value->value);
          success = TRUE;
        }
    }
  else if (scanner->token == G_TOKEN_INT)
    {
      g_value_set_enum (property_value, scanner->value.v_int);
      success = TRUE;
    }
  if (need_closing_brace && g_scanner_get_next_token (scanner) != ')')
    success = FALSE;
  if (g_scanner_get_next_token (scanner) != G_TOKEN_EOF)
    success = FALSE;

  g_scanner_destroy (scanner);

  return success;
}

static guint
parse_flags_value (GScanner    *scanner,
                   GFlagsClass *class,
                   guint       *number)
{
  g_scanner_get_next_token (scanner);
  if (scanner->token == G_TOKEN_IDENTIFIER)
    {
      GFlagsValue *flags_value;

      flags_value = g_flags_get_value_by_name (class, scanner->value.v_identifier);
      if (!flags_value)
        flags_value = g_flags_get_value_by_nick (class, scanner->value.v_identifier);
      if (flags_value)
        {
          *number |= flags_value->value;
          return G_TOKEN_NONE;
        }
    }
  else if (scanner->token == G_TOKEN_INT)
    {
      *number |= scanner->value.v_int;
      return G_TOKEN_NONE;
    }
  return G_TOKEN_IDENTIFIER;
}

/**
 * ctk_rc_property_parse_flags:
 * @pspec: a #GParamSpec
 * @gstring: the #GString to be parsed
 * @property_value: a #GValue which must hold flags values.
 *
 * A #CtkRcPropertyParser for use with ctk_settings_install_property_parser()
 * or ctk_widget_class_install_style_property_parser() which parses flags.
 *
 * Flags can be specified by their name, their nickname or
 * numerically. Multiple flags can be specified in the form
 * `"( flag1 | flag2 | ... )"`.
 *
 * Returns: %TRUE if @gstring could be parsed and @property_value
 * has been set to the resulting flags value.
 **/
gboolean
ctk_rc_property_parse_flags (const GParamSpec *pspec,
                             const GString    *gstring,
                             GValue           *property_value)
{
  GFlagsClass *class;
   gboolean success = FALSE;
  GScanner *scanner;

  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), FALSE);
  g_return_val_if_fail (G_VALUE_HOLDS_FLAGS (property_value), FALSE);

  class = G_PARAM_SPEC_FLAGS (pspec)->flags_class;
  scanner = ctk_rc_scanner_new ();
  g_scanner_input_text (scanner, gstring->str, gstring->len);

  /* parse either a single flags value or a "\( ... [ \| ... ] \)" compound */
  if (g_scanner_peek_next_token (scanner) == G_TOKEN_IDENTIFIER ||
      scanner->next_token == G_TOKEN_INT)
    {
      guint token, flags_value = 0;

      token = parse_flags_value (scanner, class, &flags_value);

      if (token == G_TOKEN_NONE && g_scanner_peek_next_token (scanner) == G_TOKEN_EOF)
        {
          success = TRUE;
          g_value_set_flags (property_value, flags_value);
        }

    }
  else if (g_scanner_get_next_token (scanner) == '(')
    {
      guint token, flags_value = 0;

      /* parse first value */
      token = parse_flags_value (scanner, class, &flags_value);

      /* parse nth values, preceeded by '|' */
      while (token == G_TOKEN_NONE && g_scanner_get_next_token (scanner) == '|')
        token = parse_flags_value (scanner, class, &flags_value);

      /* done, last token must have closed expression */
      if (token == G_TOKEN_NONE && scanner->token == ')' &&
          g_scanner_peek_next_token (scanner) == G_TOKEN_EOF)
        {
          g_value_set_flags (property_value, flags_value);
          success = TRUE;
        }
    }
  g_scanner_destroy (scanner);

  return success;
}

static gboolean
get_braced_int (GScanner *scanner,
                gboolean  first,
                gboolean  last,
                gint     *value)
{
  if (first)
    {
      g_scanner_get_next_token (scanner);
      if (scanner->token != '{')
        return FALSE;
    }

  g_scanner_get_next_token (scanner);
  if (scanner->token != G_TOKEN_INT)
    return FALSE;

  *value = scanner->value.v_int;

  if (last)
    {
      g_scanner_get_next_token (scanner);
      if (scanner->token != '}')
        return FALSE;
    }
  else
    {
      g_scanner_get_next_token (scanner);
      if (scanner->token != ',')
        return FALSE;
    }

  return TRUE;
}

/**
 * ctk_rc_property_parse_requisition:
 * @pspec: a #GParamSpec
 * @gstring: the #GString to be parsed
 * @property_value: a #GValue which must hold boxed values.
 *
 * A #CtkRcPropertyParser for use with ctk_settings_install_property_parser()
 * or ctk_widget_class_install_style_property_parser() which parses a
 * requisition in the form
 * `"{ width, height }"` for integers %width and %height.
 *
 * Returns: %TRUE if @gstring could be parsed and @property_value
 * has been set to the resulting #CtkRequisition.
 **/
gboolean
ctk_rc_property_parse_requisition  (const GParamSpec *pspec,
                                    const GString    *gstring,
                                    GValue           *property_value)
{
  CtkRequisition requisition;
  GScanner *scanner;
  gboolean success = FALSE;

  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), FALSE);
  g_return_val_if_fail (G_VALUE_HOLDS_BOXED (property_value), FALSE);

  scanner = ctk_rc_scanner_new ();
  g_scanner_input_text (scanner, gstring->str, gstring->len);

  if (get_braced_int (scanner, TRUE, FALSE, &requisition.width) &&
      get_braced_int (scanner, FALSE, TRUE, &requisition.height))
    {
      g_value_set_boxed (property_value, &requisition);
      success = TRUE;
    }

  g_scanner_destroy (scanner);

  return success;
}

/**
 * ctk_rc_property_parse_border:
 * @pspec: a #GParamSpec
 * @gstring: the #GString to be parsed
 * @property_value: a #GValue which must hold boxed values.
 *
 * A #CtkRcPropertyParser for use with ctk_settings_install_property_parser()
 * or ctk_widget_class_install_style_property_parser() which parses
 * borders in the form
 * `"{ left, right, top, bottom }"` for integers
 * left, right, top and bottom.
 *
 * Returns: %TRUE if @gstring could be parsed and @property_value
 * has been set to the resulting #CtkBorder.
 **/
gboolean
ctk_rc_property_parse_border (const GParamSpec *pspec,
                              const GString    *gstring,
                              GValue           *property_value)
{
  CtkBorder border;
  GScanner *scanner;
  gboolean success = FALSE;
  int left, right, top, bottom;

  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), FALSE);
  g_return_val_if_fail (G_VALUE_HOLDS_BOXED (property_value), FALSE);

  scanner = ctk_rc_scanner_new ();
  g_scanner_input_text (scanner, gstring->str, gstring->len);

  if (get_braced_int (scanner, TRUE, FALSE, &left) &&
      get_braced_int (scanner, FALSE, FALSE, &right) &&
      get_braced_int (scanner, FALSE, FALSE, &top) &&
      get_braced_int (scanner, FALSE, TRUE, &bottom))
    {
      border.left = left;
      border.right = right;
      border.top = top;
      border.bottom = bottom;
      g_value_set_boxed (property_value, &border);
      success = TRUE;
    }

  g_scanner_destroy (scanner);

  return success;
}

G_GNUC_END_IGNORE_DEPRECATIONS

void
_ctk_settings_handle_event (CdkEventSetting *event)
{
  CdkScreen *screen;
  CtkSettings *settings;
  GParamSpec *pspec;

  screen = cdk_window_get_screen (event->window);
  settings = ctk_settings_get_for_screen (screen);
  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (settings), event->name);

  if (!pspec)
    return;

  if (settings_update_xsetting (settings, pspec, TRUE))
    g_object_notify_by_pspec (G_OBJECT (settings), pspec);
}

static void
reset_rc_values_foreach (GQuark   key_id,
                         gpointer data,
                         gpointer user_data)
{
  CtkSettingsValuePrivate *qvalue = data;
  GSList **to_reset = user_data;

  if (qvalue->source == CTK_SETTINGS_SOURCE_THEME)
    *to_reset = g_slist_prepend (*to_reset, GUINT_TO_POINTER (key_id));
}

void
_ctk_settings_reset_rc_values (CtkSettings *settings)
{
  CtkSettingsPrivate *priv = settings->priv;
  GSList *to_reset = NULL;
  GSList *tmp_list;
  GParamSpec **pspecs, **p;
  gint i;

  /* Remove any queued settings */
  g_datalist_foreach (&priv->queued_settings,
                      reset_rc_values_foreach,
                      &to_reset);

  for (tmp_list = to_reset; tmp_list; tmp_list = tmp_list->next)
    {
      GQuark key_id = GPOINTER_TO_UINT (tmp_list->data);
      g_datalist_id_remove_data (&priv->queued_settings, key_id);
    }

   g_slist_free (to_reset);

  /* Now reset the active settings
   */
  pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (settings), NULL);
  i = 0;

  g_object_freeze_notify (G_OBJECT (settings));
  for (p = pspecs; *p; p++)
    {
      if (priv->property_values[i].source == CTK_SETTINGS_SOURCE_THEME)
        {
          GParamSpec *pspec = *p;

          g_param_value_set_default (pspec, &priv->property_values[i].value);
          g_object_notify_by_pspec (G_OBJECT (settings), pspec);
        }
      i++;
    }
  g_object_thaw_notify (G_OBJECT (settings));
  g_free (pspecs);
}

static void
settings_update_double_click (CtkSettings *settings)
{
  CtkSettingsPrivate *priv = settings->priv;
  CdkDisplay *display = cdk_screen_get_display (priv->screen);
  gint double_click_time;
  gint double_click_distance;

  g_object_get (settings,
                "ctk-double-click-time", &double_click_time,
                "ctk-double-click-distance", &double_click_distance,
                NULL);

  cdk_display_set_double_click_time (display, double_click_time);
  cdk_display_set_double_click_distance (display, double_click_distance);
}

static void
settings_update_modules (CtkSettings *settings)
{
  gchar *modules;

  g_object_get (settings,
                "ctk-modules", &modules,
                NULL);

  _ctk_modules_settings_changed (settings, modules);

  g_free (modules);
}

static void
settings_update_cursor_theme (CtkSettings *settings)
{
  gchar *theme = NULL;
  gint size = 0;
#if defined(CDK_WINDOWING_X11) || defined(CDK_WINDOWING_WAYLAND) || defined(CDK_WINDOWING_WIN32)
  CdkDisplay *display = cdk_screen_get_display (settings->priv->screen);
#endif

  g_object_get (settings,
                "ctk-cursor-theme-name", &theme,
                "ctk-cursor-theme-size", &size,
                NULL);
  if (theme == NULL)
    return;
#ifdef CDK_WINDOWING_X11
  if (CDK_IS_X11_DISPLAY (display))
    cdk_x11_display_set_cursor_theme (display, theme, size);
  else
#endif
#ifdef CDK_WINDOWING_WAYLAND
  if (CDK_IS_WAYLAND_DISPLAY (display))
    cdk_wayland_display_set_cursor_theme (display, theme, size);
  else
#endif
#ifdef CDK_WINDOWING_WIN32
  if (CDK_IS_WIN32_DISPLAY (display))
    cdk_win32_display_set_cursor_theme (display, theme, size);
  else
#endif
    g_warning ("CtkSettings Cursor Theme: Unsupported CDK backend");
  g_free (theme);
}

static void
settings_update_font_options (CtkSettings *settings)
{
  CtkSettingsPrivate *priv = settings->priv;
  gint hinting;
  gchar *hint_style_str;
  cairo_hint_style_t hint_style;
  gint antialias;
  cairo_antialias_t antialias_mode;
  gchar *rgba_str;
  cairo_subpixel_order_t subpixel_order;
  cairo_font_options_t *options;

  g_object_get (settings,
                "ctk-xft-antialias", &antialias,
                "ctk-xft-hinting", &hinting,
                "ctk-xft-hintstyle", &hint_style_str,
                "ctk-xft-rgba", &rgba_str,
                NULL);

  options = cairo_font_options_create ();

  cairo_font_options_set_hint_metrics (options, CAIRO_HINT_METRICS_ON);

  hint_style = CAIRO_HINT_STYLE_DEFAULT;
  if (hinting == 0)
    {
      hint_style = CAIRO_HINT_STYLE_NONE;
    }
  else if (hinting == 1)
    {
      if (hint_style_str)
        {
          if (strcmp (hint_style_str, "hintnone") == 0)
            hint_style = CAIRO_HINT_STYLE_NONE;
          else if (strcmp (hint_style_str, "hintslight") == 0)
            hint_style = CAIRO_HINT_STYLE_SLIGHT;
          else if (strcmp (hint_style_str, "hintmedium") == 0)
            hint_style = CAIRO_HINT_STYLE_MEDIUM;
          else if (strcmp (hint_style_str, "hintfull") == 0)
            hint_style = CAIRO_HINT_STYLE_FULL;
        }
    }

  g_free (hint_style_str);

  cairo_font_options_set_hint_style (options, hint_style);

  subpixel_order = CAIRO_SUBPIXEL_ORDER_DEFAULT;
  if (rgba_str)
    {
      if (strcmp (rgba_str, "rgb") == 0)
        subpixel_order = CAIRO_SUBPIXEL_ORDER_RGB;
      else if (strcmp (rgba_str, "bgr") == 0)
        subpixel_order = CAIRO_SUBPIXEL_ORDER_BGR;
      else if (strcmp (rgba_str, "vrgb") == 0)
        subpixel_order = CAIRO_SUBPIXEL_ORDER_VRGB;
      else if (strcmp (rgba_str, "vbgr") == 0)
        subpixel_order = CAIRO_SUBPIXEL_ORDER_VBGR;
    }

  g_free (rgba_str);

  cairo_font_options_set_subpixel_order (options, subpixel_order);

  antialias_mode = CAIRO_ANTIALIAS_DEFAULT;
  if (antialias == 0)
    {
      antialias_mode = CAIRO_ANTIALIAS_NONE;
    }
  else if (antialias == 1)
    {
      if (subpixel_order != CAIRO_SUBPIXEL_ORDER_DEFAULT)
        antialias_mode = CAIRO_ANTIALIAS_SUBPIXEL;
      else
        antialias_mode = CAIRO_ANTIALIAS_GRAY;
    }

  cairo_font_options_set_antialias (options, antialias_mode);

  cdk_screen_set_font_options (priv->screen, options);

  cairo_font_options_destroy (options);
}

static gboolean
settings_update_fontconfig (CtkSettings *settings)
{
#if defined(CDK_WINDOWING_X11) || defined(CDK_WINDOWING_WAYLAND)
  static guint    last_update_timestamp;
  static gboolean last_update_needed;

  guint timestamp;

  g_object_get (settings,
                "ctk-fontconfig-timestamp", &timestamp,
                NULL);

  /* if timestamp is the same as last_update_timestamp, we already have
   * updated fontconig on this timestamp (another screen requested it perhaps?),
   * just return the cached result.*/

  if (timestamp != last_update_timestamp)
    {
      PangoFontMap *fontmap = pango_cairo_font_map_get_default ();
      gboolean update_needed = FALSE;

      /* bug 547680 */
      if (PANGO_IS_FC_FONT_MAP (fontmap) && !FcConfigUptoDate (NULL))
        {
          pango_fc_font_map_config_changed (PANGO_FC_FONT_MAP (fontmap));
          if (FcInitReinitialize ())
            update_needed = TRUE;
        }

      last_update_timestamp = timestamp;
      last_update_needed = update_needed;
    }

  return last_update_needed;
#else
  return FALSE;
#endif /* CDK_WINDOWING_X11 || CDK_WINDOWING_WAYLAND */
}

static void
settings_update_resolution (CtkSettings *settings)
{
  CtkSettingsPrivate *priv = settings->priv;
  gint dpi_int;
  gdouble dpi;
  const char *scale_env;
  double scale;

  /* We handle this here in the case that the dpi was set on the CtkSettings
   * object by the application. Other cases are handled in
   * xsettings-client.c:read-settings(). See comment there for the rationale.
   */
  if (priv->property_values[PROP_XFT_DPI - 1].source == CTK_SETTINGS_SOURCE_APPLICATION)
    {
      g_object_get (settings,
                    "ctk-xft-dpi", &dpi_int,
                    NULL);

      if (dpi_int > 0)
        dpi = dpi_int / 1024.;
      else
        dpi = -1.;

      scale_env = g_getenv ("CDK_DPI_SCALE");
      if (scale_env)
        {
          scale = g_ascii_strtod (scale_env, NULL);
          if (scale != 0 && dpi > 0)
            dpi *= scale;
        }

      cdk_screen_set_resolution (priv->screen, dpi);
    }
}

static void
settings_update_provider (CdkScreen       *screen,
                          CtkCssProvider **old,
                          CtkCssProvider  *new)
{
  if (screen != NULL && *old != new)
    {
      if (*old)
        {
          ctk_style_context_remove_provider_for_screen (screen,
                                                        CTK_STYLE_PROVIDER (*old));
          g_object_unref (*old);
          *old = NULL;
        }

      if (new)
        {
          ctk_style_context_add_provider_for_screen (screen,
                                                     CTK_STYLE_PROVIDER (new),
                                                     CTK_STYLE_PROVIDER_PRIORITY_THEME);
          *old = g_object_ref (new);
        }
    }
}

static void
get_theme_name (CtkSettings  *settings,
                gchar       **theme_name,
                gchar       **theme_variant)
{
  gboolean prefer_dark;

  *theme_name = NULL;
  *theme_variant = NULL;

  if (g_getenv ("CTK_THEME"))
    *theme_name = g_strdup (g_getenv ("CTK_THEME"));

  if (*theme_name && **theme_name)
    {
      char *p;
      p = strrchr (*theme_name, ':');
      if (p) {
        *p = '\0';
        p++;
        *theme_variant = g_strdup (p);
      }

      return;
    }

  g_free (*theme_name);

  g_object_get (settings,
                "ctk-theme-name", theme_name,
                "ctk-application-prefer-dark-theme", &prefer_dark,
                NULL);

  if (prefer_dark)
    *theme_variant = g_strdup ("dark");

  if (*theme_name && **theme_name)
    return;

  g_free (*theme_name);
  *theme_name = g_strdup (DEFAULT_THEME_NAME);
}

static void
settings_update_theme (CtkSettings *settings)
{
  CtkSettingsPrivate *priv = settings->priv;
  gchar *theme_name;
  gchar *theme_variant;
  const gchar *theme_dir;
  gchar *path;

  get_theme_name (settings, &theme_name, &theme_variant);

  _ctk_css_provider_load_named (priv->theme_provider,
                                theme_name, theme_variant);

  /* reload per-theme settings */
  theme_dir = _ctk_css_provider_get_theme_dir (priv->theme_provider);
  if (theme_dir)
    {
      path = g_build_filename (theme_dir, "settings.ini", NULL);
      if (g_file_test (path, G_FILE_TEST_EXISTS))
        ctk_settings_load_from_key_file (settings, path, CTK_SETTINGS_SOURCE_THEME);
      g_free (path);
    }

  g_free (theme_name);
  g_free (theme_variant);
}

static void
settings_update_key_theme (CtkSettings *settings)
{
  CtkSettingsPrivate *priv = settings->priv;
  CtkCssProvider *provider = NULL;
  gchar *key_theme_name;

  g_object_get (settings,
                "ctk-key-theme-name", &key_theme_name,
                NULL);

  if (key_theme_name && *key_theme_name)
    provider = ctk_css_provider_get_named (key_theme_name, "keys");

  settings_update_provider (priv->screen, &priv->key_theme_provider, provider);
  g_free (key_theme_name);
}


CdkScreen *
_ctk_settings_get_screen (CtkSettings *settings)
{
  return settings->priv->screen;
}

static void
gvalue_free (gpointer data)
{
  g_value_unset (data);
  g_free (data);
}

static void
ctk_settings_load_from_key_file (CtkSettings       *settings,
                                 const gchar       *path,
                                 CtkSettingsSource  source)
{
  GError *error;
  GKeyFile *keyfile;
  gchar **keys;
  gsize n_keys;
  gint i;

  error = NULL;
  keys = NULL;

  keyfile = g_key_file_new ();

  if (!g_key_file_load_from_file (keyfile, path, G_KEY_FILE_NONE, &error))
    {
      g_warning ("Failed to parse %s: %s", path, error->message);

      g_error_free (error);

      goto out;
    }

  keys = g_key_file_get_keys (keyfile, "Settings", &n_keys, &error);
  if (error)
    {
      g_warning ("Failed to parse %s: %s", path, error->message);
      g_error_free (error);
      goto out;
    }

  for (i = 0; i < n_keys; i++)
    {
      gchar *key;
      GParamSpec *pspec;
      GType value_type;
      CtkSettingsValue svalue = { NULL, { 0, }, };

      key = keys[i];
      pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (settings), key);
      if (!pspec)
        {
          g_warning ("Unknown key %s in %s", key, path);
          continue;
        }

      if (pspec->owner_type != G_OBJECT_TYPE (settings))
        continue;

      value_type = G_PARAM_SPEC_VALUE_TYPE (pspec);
      switch (value_type)
        {
        case G_TYPE_BOOLEAN:
          {
            gboolean b_val;

            g_value_init (&svalue.value, G_TYPE_LONG);
            b_val = g_key_file_get_boolean (keyfile, "Settings", key, &error);
            if (!error)
              g_value_set_long (&svalue.value, b_val);
            break;
          }

        case G_TYPE_INT:
        case G_TYPE_UINT:
          {
            gint i_val;

            g_value_init (&svalue.value, G_TYPE_LONG);
            i_val = g_key_file_get_integer (keyfile, "Settings", key, &error);
            if (!error)
              g_value_set_long (&svalue.value, i_val);
            break;
          }

        case G_TYPE_DOUBLE:
          {
            gdouble d_val;

            g_value_init (&svalue.value, G_TYPE_DOUBLE);
            d_val = g_key_file_get_double (keyfile, "Settings", key, &error);
            if (!error)
              g_value_set_double (&svalue.value, d_val);
            break;
          }

        default:
          {
            gchar *s_val;

            g_value_init (&svalue.value, G_TYPE_GSTRING);
            s_val = g_key_file_get_string (keyfile, "Settings", key, &error);
            if (!error)
              g_value_take_boxed (&svalue.value, g_string_new (s_val));
            g_free (s_val);
            break;
          }
        }
      if (error)
        {
          g_warning ("Error setting %s in %s: %s", key, path, error->message);
          g_error_free (error);
          error = NULL;
        }
      else
        {
          GValue *copy;

          copy = g_new0 (GValue, 1);

          g_value_init (copy, G_VALUE_TYPE (&svalue.value));
          g_value_copy (&svalue.value, copy);

          g_param_spec_set_qdata_full (pspec, g_quark_from_string (key),
                                       copy, gvalue_free);

          if (g_getenv ("CTK_DEBUG"))
            svalue.origin = (gchar *)path;

          ctk_settings_set_property_value_internal (settings, key, &svalue, source);
          g_value_unset (&svalue.value);
        }
    }

 out:
  g_strfreev (keys);
  g_key_file_free (keyfile);
}

static gboolean
settings_update_xsetting (CtkSettings *settings,
                          GParamSpec  *pspec,
                          gboolean     force)
{
  CtkSettingsPrivate *priv = settings->priv;
  GType value_type;
  GType fundamental_type;
  gboolean retval = FALSE;

  if (priv->property_values[pspec->param_id - 1].source == CTK_SETTINGS_SOURCE_APPLICATION)
    return FALSE;

  if (priv->property_values[pspec->param_id - 1].source == CTK_SETTINGS_SOURCE_XSETTING && !force)
    return FALSE;

  value_type = G_PARAM_SPEC_VALUE_TYPE (pspec);
  fundamental_type = G_TYPE_FUNDAMENTAL (value_type);

  if ((g_value_type_transformable (G_TYPE_INT, value_type) &&
       !(fundamental_type == G_TYPE_ENUM || fundamental_type == G_TYPE_FLAGS)) ||
      g_value_type_transformable (G_TYPE_STRING, value_type) ||
      g_value_type_transformable (CDK_TYPE_RGBA, value_type))
    {
      GValue val = G_VALUE_INIT;

      g_value_init (&val, value_type);

      if (!cdk_screen_get_setting (priv->screen, pspec->name, &val))
        return FALSE;

      g_param_value_validate (pspec, &val);
      g_value_copy (&val, &priv->property_values[pspec->param_id - 1].value);
      priv->property_values[pspec->param_id - 1].source = CTK_SETTINGS_SOURCE_XSETTING;

      g_value_unset (&val);

      retval = TRUE;
    }
  else
    {
      GValue tmp_value = G_VALUE_INIT;
      GValue gstring_value = G_VALUE_INIT;
      GValue val = G_VALUE_INIT;
      CtkRcPropertyParser parser = (CtkRcPropertyParser) g_param_spec_get_qdata (pspec, quark_property_parser);

      g_value_init (&val, G_TYPE_STRING);

      if (!cdk_screen_get_setting (priv->screen, pspec->name, &val))
        return FALSE;

      g_value_init (&gstring_value, G_TYPE_GSTRING);
      g_value_take_boxed (&gstring_value, g_string_new (g_value_get_string (&val)));

      g_value_init (&tmp_value, value_type);
      if (parser && _ctk_settings_parse_convert (parser, &gstring_value,
                                                 pspec, &tmp_value))
        {
          g_param_value_validate (pspec, &tmp_value);
          g_value_copy (&tmp_value, &priv->property_values[pspec->param_id - 1].value);
          priv->property_values[pspec->param_id - 1].source = CTK_SETTINGS_SOURCE_XSETTING;
          retval = TRUE;
        }

      g_value_unset (&gstring_value);
      g_value_unset (&tmp_value);

      g_value_unset (&val);
    }

  return retval;
}

static void
settings_update_xsettings (CtkSettings *settings)
{
  GParamSpec **pspecs;
  gint i;

  pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (settings), NULL);
  for (i = 0; pspecs[i]; i++)
    settings_update_xsetting (settings, pspecs[i], FALSE);
  g_free (pspecs);
}

static void
ctk_settings_get_property (GObject     *object,
                           guint        property_id,
                           GValue      *value,
                           GParamSpec  *pspec)
{
  CtkSettings *settings = CTK_SETTINGS (object);
  CtkSettingsPrivate *priv = settings->priv;

  /* handle internal properties */
  switch (property_id)
    {
    case PROP_COLOR_HASH:
      g_value_take_boxed (value, g_hash_table_new (g_str_hash, g_str_equal));
      return;
    default: ;
    }

  settings_update_xsetting (settings, pspec, FALSE);

  g_value_copy (&priv->property_values[property_id - 1].value, value);
}

CtkSettingsSource
_ctk_settings_get_setting_source (CtkSettings *settings,
                                  const gchar *name)
{
  CtkSettingsPrivate *priv = settings->priv;
  GParamSpec *pspec;

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (settings), name);
  if (!pspec)
    return CTK_SETTINGS_SOURCE_DEFAULT;

  return priv->property_values[pspec->param_id - 1].source;
}

/**
 * ctk_settings_reset_property:
 * @settings: a #CtkSettings object
 * @name: the name of the setting to reset
 *
 * Undoes the effect of calling g_object_set() to install an
 * application-specific value for a setting. After this call,
 * the setting will again follow the session-wide value for
 * this setting.
 *
 * Since: 3.20
 */
void
ctk_settings_reset_property (CtkSettings *settings,
                             const gchar *name)
{
  CtkSettingsPrivate *priv = settings->priv;
  GParamSpec *pspec;
  CtkRcPropertyParser parser;
  GValue *value;
  GValue tmp_value = G_VALUE_INIT;

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (settings), name);

  g_return_if_fail (pspec != NULL);

  parser = (CtkRcPropertyParser) g_param_spec_get_qdata (pspec, quark_property_parser);
  value = g_param_spec_get_qdata (pspec, g_quark_from_string (name));

  g_value_init (&tmp_value, G_PARAM_SPEC_VALUE_TYPE (pspec));
  if (value && _ctk_settings_parse_convert (parser, value, pspec, &tmp_value))
    g_value_copy (&tmp_value, &priv->property_values[pspec->param_id - 1].value);
  else
    g_param_value_set_default (pspec, &priv->property_values[pspec->param_id - 1].value);

  priv->property_values[pspec->param_id - 1].source = CTK_SETTINGS_SOURCE_DEFAULT;
  g_object_notify_by_pspec (G_OBJECT (settings), pspec);
}

gboolean
ctk_settings_get_enable_animations (CtkSettings *settings)
{
  CtkSettingsPrivate *priv = settings->priv;
  CtkSettingsPropertyValue *svalue = &priv->property_values[PROP_ENABLE_ANIMATIONS - 1];

  if (svalue->source < CTK_SETTINGS_SOURCE_XSETTING)
    {
      GParamSpec *pspec;

      pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (settings), "ctk-enable-animations");
      if (settings_update_xsetting (settings, pspec, FALSE))
        g_object_notify_by_pspec (G_OBJECT (settings), pspec);
    }

  return g_value_get_boolean (&svalue->value);
}

gint
ctk_settings_get_dnd_drag_threshold (CtkSettings *settings)
{
  CtkSettingsPrivate *priv = settings->priv;
  CtkSettingsPropertyValue *svalue = &priv->property_values[PROP_DND_DRAG_THRESHOLD - 1];

  if (svalue->source < CTK_SETTINGS_SOURCE_XSETTING)
    {
      GParamSpec *pspec;

      pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (settings), "ctk-dnd-drag-threshold");
      if (settings_update_xsetting (settings, pspec, FALSE))
        g_object_notify_by_pspec (G_OBJECT (settings), pspec);
    }

  return g_value_get_int (&svalue->value);
}

static void
settings_update_font_name (CtkSettings *settings)
{
  CtkSettingsPrivate *priv = settings->priv;
  CtkSettingsPropertyValue *svalue = &priv->property_values[PROP_FONT_NAME - 1];

  if (svalue->source < CTK_SETTINGS_SOURCE_XSETTING)
    {
      GParamSpec *pspec;

      pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (settings), "ctk-font-name");
      if (settings_update_xsetting (settings, pspec, FALSE))
        g_object_notify_by_pspec (G_OBJECT (settings), pspec);
    }
}

const gchar *
ctk_settings_get_font_family (CtkSettings *settings)
{
  settings_update_font_name (settings);

  return settings->priv->font_family;
}

gint
ctk_settings_get_font_size (CtkSettings *settings)
{
  settings_update_font_name (settings);

  return settings->priv->font_size;
}

gboolean
ctk_settings_get_font_size_is_absolute (CtkSettings *settings)
{
  settings_update_font_name (settings);

  return settings->priv->font_size_absolute;
}
