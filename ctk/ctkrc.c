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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/stat.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <glib.h>
#include <glib/gstdio.h>
#include "cdk/cdk.h"

#include "ctkversion.h"
#include "ctkrc.h"
#include "ctkstyle.h"
#include "ctkbindings.h"
#include "ctkintl.h"
#include "ctkiconfactory.h"
#include "ctkmain.h"
#include "ctkmodules.h"
#include "ctkmodulesprivate.h"
#include "ctkprivate.h"
#include "ctksettingsprivate.h"
#include "ctkwidgetpath.h"
#include "ctkwindow.h"

#ifdef G_OS_WIN32
#include <io.h>
#endif


/**
 * SECTION:ctkrc
 * @Short_description: Routines for handling resource files
 * @Title: Resource Files
 *
 * CTK+ provides resource file mechanism for configuring
 * various aspects of the operation of a CTK+ program
 * at runtime.
 *
 * # Default Files #
 *
 * An application can cause CTK+ to parse a specific RC
 * file by calling ctk_rc_parse(). In addition to this,
 * certain files will be read at the end of ctk_init().
 * Unless modified, the files looked for will be
 * `SYSCONFDIR/ctk-2.0/ctkrc`
 * and `.ctkrc-3.0` in the users home directory.
 * (`SYSCONFDIR` defaults to
 * `/usr/local/etc`. It can be changed with the
 * `--prefix` or `--sysconfdir` options when
 * configuring CTK+.)
 *
 * The set of these “default” files
 * can be retrieved with ctk_rc_get_default_files()
 * and modified with ctk_rc_add_default_file() and
 * ctk_rc_set_default_files().
 * Additionally, the `CTK2_RC_FILES` environment variable
 * can be set to a #G_SEARCHPATH_SEPARATOR_S-separated list of files
 * in order to overwrite the set of default files at runtime.
 *
 * # Locale Specific Files # {#locale-specific-rc}
 *
 * For each RC file, in addition to the file itself, CTK+ will look for
 * a locale-specific file that will be parsed after the main file.
 * For instance, if `LANG` is set to `ja_JP.ujis`,
 * when loading the default file `~/.ctkrc` then CTK+ looks
 * for `~/.ctkrc.ja_JP` and `~/.ctkrc.ja`,
 * and parses the first of those that exists.
 *
 * # Pathnames and Patterns #
 *
 * A resource file defines a number of styles and key bindings and
 * attaches them to particular widgets. The attachment is done
 * by the `widget`, `widget_class`,
 * and `class` declarations. As an example
 * of such a statement:
 *
 * |[
 * widget "mywindow.*.CtkEntry" style "my-entry-class"
 * ]|
 *
 * attaches the style `"my-entry-class"` to all
 * widgets  whose “widget path” matches the
 * “pattern” `"mywindow.*.CtkEntry"`.
 * That is, all #CtkEntry widgets which are part of a #CtkWindow named
 * `"mywindow"`.
 *
 * The patterns here are given in the standard shell glob syntax.
 * The `"?"` wildcard matches any character, while
 * `"*"` matches zero or more of any character.
 * The three types of matching are against the widget path, the
 * “class path” and the class hierarchy. Both the
 * widget path and the class path consist of a `"."`
 * separated list of all the parents of the widget and the widget itself
 * from outermost to innermost. The difference is that in the widget path,
 * the name assigned by ctk_widget_set_name() is used if present, otherwise
 * the class name of the widget, while for the class path, the class name is
 * always used.
 *
 * Since CTK+ 2.10, `widget_class` paths can also contain <classname>
 * substrings, which are matching the class with the given name and any
 * derived classes. For instance,
 * |[
 * widget_class "*<CtkMenuItem>.CtkLabel" style "my-style"
 * ]|
 * will match #CtkLabel widgets which are contained in any kind of menu item.
 *
 * So, if you have a #CtkEntry named `"myentry"`, inside of a horizontal
 * box in a window named `"mywindow"`, then the widget path is:
 * `"mywindow.CtkHBox.myentry"` while the class path is:
 * `"CtkWindow.CtkHBox.CtkEntry"`.
 *
 * Matching against class is a little different. The pattern match is done
 * against all class names in the widgets class hierarchy (not the layout
 * hierarchy) in sequence, so the pattern:
 * |[
 * class "CtkButton" style "my-style"
 * ]|
 * will match not just #CtkButton widgets, but also #CtkToggleButton and
 * #CtkCheckButton widgets, since those classes derive from #CtkButton.
 *
 * Additionally, a priority can be specified for each pattern, and styles
 * override other styles first by priority, then by pattern type and then
 * by order of specification (later overrides earlier). The priorities
 * that can be specified are (highest to lowest):
 *
 * - `highest`
 *
 * - `rc`
 *
 * - `theme`
 *
 * - `application`
 *
 * - `ctk`
 *
 * - `lowest`
 *
 * `rc` is the default for styles
 * read from an RC file, `theme`
 * is the default for styles read from theme RC files,
 * `application`
 * should be used for styles an application sets
 * up, and `ctk` is used for styles
 * that CTK+ creates internally.
 *
 * # Theme ctkrc Files #
 *
 * Theme RC files are loaded first from under the `~/.themes/`,
 * then from the directory from ctk_rc_get_theme_dir(). The files looked at will
 * be `ctk-3.0/ctkrc`.
 *
 * When the application prefers dark themes
 * (see the #CtkSettings:ctk-application-prefer-dark-theme property for details),
 * `ctk-3.0/ctkrc-dark` will be loaded first, and if not present
 * `ctk-3.0/ctkrc` will be loaded.
 *
 * # Optimizing RC Style Matches #
 *
 * Everytime a widget is created and added to the layout hierarchy of a #CtkWindow
 * ("anchored" to be exact), a list of matching RC styles out of all RC styles read
 * in so far is composed.
 * For this, every RC style is matched against the widgets class path,
 * the widgets name path and widgets inheritance hierarchy.
 * As a consequence, significant slowdown can be caused by utilization of many
 * RC styles and by using RC style patterns that are slow or complicated to match
 * against a given widget.
 * The following ordered list provides a number of advices (prioritized by
 * effectiveness) to reduce the performance overhead associated with RC style
 * matches:
 *
 * 1. Move RC styles for specific applications into RC files dedicated to those
 *   applications and parse application specific RC files only from
 *   applications that are affected by them.
 *   This reduces the overall amount of RC styles that have to be considered
 *   for a match across a group of applications.
 *
 * 2.  Merge multiple styles which use the same matching rule, for instance:
 *   |[
 *      style "Foo" { foo_content }
 *      class "X" style "Foo"
 *      style "Bar" { bar_content }
 *      class "X" style "Bar"
 *   ]|
 *   is faster to match as:
 *   |[
 *      style "FooBar" { foo_content bar_content }
 *      class "X" style "FooBar"
 *   ]|
 *
 * 3. Use of wildcards should be avoided, this can reduce the individual RC style
 *   match to a single integer comparison in most cases.
 *
 * 4. To avoid complex recursive matching, specification of full class names
 *   (for `class` matches) or full path names (for
 *   `widget` and `widget_class` matches)
 *   is to be preferred over shortened names
 *   containing `"*"` or `"?"`.
 *
 * 5. If at all necessary, wildcards should only be used at the tail or head
 *   of a pattern. This reduces the match complexity to a string comparison
 *   per RC style.
 *
 * 6. When using wildcards, use of `"?"` should be preferred
 *   over `"*"`. This can reduce the matching complexity from
 *   O(n^2) to O(n). For example `"Ctk*Box"` can be turned into
 *   `"Ctk?Box"` and will still match #CtkHBox and #CtkVBox.
 *
 * 7. The use of `"*"` wildcards should be restricted as much
 *   as possible, because matching `"A*B*C*RestString"` can
 *   result in matching complexities of O(n^2) worst case.
 *
 * # Toplevel Declarations #
 *
 * An RC file is a text file which is composed of a sequence
 * of declarations. `“#”` characters delimit comments and
 * the portion of a line after a `“#”` is ignored when parsing
 * an RC file.
 *
 * The possible toplevel declarations are:
 *
 * * `binding name
 *      { ... }`
 *
 *    Declares a binding set.
 *
 * * `class pattern
 *           [ style | binding ][ : priority ]
 *           name`
 *
 *    Specifies a style or binding set for a particular
 *      branch of the inheritance hierarchy.
 *
 * * `include filename`
 *
 *    Parses another file at this point. If
 *         filename is not an absolute filename,
 *         it is searched in the directories of the currently open RC files.
 *
 *    CTK+ also tries to load a
 *         [locale-specific variant][locale-specific-rc] of
 *         the included file.
 *
 * * `module_path path`
 *
 *    Sets a path (a list of directories separated
 *       by colons) that will be searched for theme engines referenced in
 *       RC files.
 *
 * * `pixmap_path path`
 *
 *    Sets a path (a list of directories separated
 *       by colons) that will be searched for pixmaps referenced in
 *       RC files.
 *
 * * `im_module_file pathname`
 *
 *    Sets the pathname for the IM modules file.
 *
 * * `style name [ =
 *     parent ] { ... }`
 *
 *    Declares a style.
 *
 * * `widget pattern
 *           [ style | binding ][ : priority ]
 *           name`
 *
 *      Specifies a style or binding set for a particular
 *      group of widgets by matching on the widget pathname.
 *
 * * `widget_class pattern
 *           [ style | binding ][ : priority ]
 *           name`
 *
 *      Specifies a style or binding set for a particular
 *      group of widgets by matching on the class pathname.
 *
 * * setting = value
 *
 *    Specifies a value for a [setting][CtkSettings].
 *         Note that settings in RC files are overwritten by system-wide settings
 *         (which are managed by an XSettings manager on X11).
 *
 * # Styles #
 *
 * A RC style is specified by a `style`
 * declaration in a RC file, and then bound to widgets
 * with a `widget`, `widget_class`,
 * or `class` declaration. All styles
 * applying to a particular widget are composited together
 * with `widget` declarations overriding
 * `widget_class` declarations which, in
 * turn, override `class` declarations.
 * Within each type of declaration, later declarations override
 * earlier ones.
 *
 * Within a `style` declaration, the possible
 * elements are:
 *
 * * `bg[state] = color`
 *
 *   Sets the color used for the background of most widgets.
 *
 * * `fg[state] = color`
 *
 *   Sets the color used for the foreground of most widgets.
 *
 * * `base[state] = color`
 *
 *          Sets the color used for the background of widgets displaying
 *          editable text. This color is used for the background
 *          of, among others, #CtkTextView, #CtkEntry.
 *
 * * `text[state] =
 *       color`
 *
 *          Sets the color used for foreground of widgets using
 *          `base` for the background color.
 *
 * * `xthickness =
 *       number`
 *
 *          Sets the xthickness, which is used for various horizontal padding
 *          values in CTK+.
 *
 * * `ythickness =
 *       number`
 *
 *          Sets the ythickness, which is used for various vertical padding
 *          values in CTK+.
 *
 * * `bg_pixmap[state] =
 *       pixmap`
 *
 *          Sets a background pixmap to be used in place of the `bg` color
 *          (or for #CtkText, in place of the `base` color. The special
 *          value `"<parent>"` may be used to indicate that the widget should
 *          use the same background pixmap as its parent. The special value
 *          `"<none>"` may be used to indicate no background pixmap.

 * * `font = font`
 *
 *          Starting with CTK+ 2.0, the “font”  and “fontset” 
 *          declarations are ignored; use “font_name”  declarations instead.
 *
 * * `fontset = font`
 *
 *          Starting with CTK+ 2.0, the “font”  and “fontset” 
 *          declarations are ignored; use “font_name”  declarations instead.
 *
 * * `font_name = font`
 *
 *          Sets the font for a widget. font must be
 *          a Pango font name, e.g. “Sans Italic 10” .
 *          For details about Pango font names, see
 *          pango_font_description_from_string().
 *
 * * `stock["stock-id"] = { icon source specifications }`
 *
 *         Defines the icon for a stock item.
 *
 * * `color["color-name"] = color specification`
 *
 *         Since 2.10, this element can be used to defines symbolic colors. See below for
 *         the syntax of color specifications.
 *
 * * `engine "engine" { engine-specific
 * settings }`
 *
 *         Defines the engine to be used when drawing with this style.
 *
 * * `class::property = value`
 *
 *         Sets a [style property][style-properties] for a widget class.
 *
 * The colors and background pixmaps are specified as a function of the
 * state of the widget. The states are:
 *
 * * `NORMAL`
 *
 *         A color used for a widget in its normal state.
 *
 * * `ACTIVE`
 *
 *         A variant of the `NORMAL` color used when the
 *         widget is in the %CTK_STATE_ACTIVE state, and also for
 *         the trough of a ScrollBar, tabs of a NoteBook
 *         other than the current tab and similar areas.
 *         Frequently, this should be a darker variant
 *         of the `NORMAL` color.
 *
 * * `PRELIGHT`
 *
 *         A color used for widgets in the %CTK_STATE_PRELIGHT state. This
 *         state is the used for Buttons and MenuItems
 *         that have the mouse cursor over them, and for
 *         their children.
 *
 * * `SELECTED`
 *
 *         A color used to highlight data selected by the user.
 *         for instance, the selected items in a list widget, and the
 *         selection in an editable widget.
 *
 * * `INSENSITIVE`
 *
 *         A color used for the background of widgets that have
 *         been set insensitive with ctk_widget_set_sensitive().
 *
 * ## Color Format ## {#color-format}
 *
 * Colors can be specified as a string containing a color name (CTK+ knows
 * all names from the X color database `/usr/lib/X11/rgb.txt`),
 * in one of the hexadecimal forms `#rrrrggggbbbb`,
 * `#rrrgggbbb`, `#rrggbb`,
 * or `#rgb`, where `r`,
 * `g` and `b` are
 * hex digits, or they can be specified as a triplet
 * `{ r, g,
 * b}`, where `r`,
 * `g` and `b` are either integers in
 * the range 0-65535 or floats in the range 0.0-1.0.
 *
 * Since 2.10, colors can also be specified by refering to a symbolic color, as
 * follows: `@color-name`, or by using expressions to combine
 * colors. The following expressions are currently supported:
 *
 * * mix (factor, color1, color2)
 *
 *         Computes a new color by mixing color1 and
 *         color2. The factor
 *         determines how close the new color is to color1.
 *         A factor of 1.0 gives pure color1, a factor of
 *         0.0 gives pure color2.
 *
 * * shade (factor, color)
 *
 *         Computes a lighter or darker variant of color.
 *         A factor of 1.0 leaves the color unchanged, smaller
 *         factors yield darker colors, larger factors yield lighter colors.
 *
 * * lighter (color)
 *
 *         This is an abbreviation for
 *         `shade (1.3, color)`.
 *
 * * darker (color)
 *
 *         This is an abbreviation for
 *         `shade (0.7, color)`.
 *
 * Here are some examples of color expressions:
 *
 * |[
 *  mix (0.5, "red", "blue")
 *  shade (1.5, mix (0.3, "#0abbc0", { 0.3, 0.5, 0.9 }))
 *  lighter (@foreground)
 * ]|
 *
 * In a `stock` definition, icon sources are specified as a
 * 4-tuple of image filename or icon name, text direction, widget state, and size, in that
 * order.  Each icon source specifies an image filename or icon name to use with a given
 * direction, state, and size. Filenames are specified as a string such
 * as `"itemltr.png"`, while icon names (looked up
 * in the current icon theme), are specified with a leading
 * `@`, such as `@"item-ltr"`.
 * The `*` character can be used as a
 * wildcard, and if direction/state/size are omitted they default to
 * `*`. So for example, the following specifies different icons to
 * use for left-to-right and right-to-left languages:
 *
 * |[<!-- language="C" -->
 * stock["my-stock-item"] =
 * {
 *   { "itemltr.png", LTR, *, * },
 *   { "itemrtl.png", RTL, *, * }
 * }
 * ]|
 *
 * This could be abbreviated as follows:
 *
 * |[<!-- language="C" -->
 * stock["my-stock-item"] =
 * {
 *   { "itemltr.png", LTR },
 *   { "itemrtl.png", RTL }
 * }
 * ]|
 *
 * You can specify custom icons for specific sizes, as follows:
 *
 * |[<!-- language="C" -->
 * stock["my-stock-item"] =
 * {
 *   { "itemmenusize.png", *, *, "ctk-menu" },
 *   { "itemtoolbarsize.png", *, *, "ctk-large-toolbar" }
 *   { "itemgeneric.png" } // implicit *, *, * as a fallback
 * }
 * ]|
 *
 * The sizes that come with CTK+ itself are `"ctk-menu"`,
 * `"ctk-small-toolbar"`, `"ctk-large-toolbar"`,
 * `"ctk-button"`, `"ctk-dialog"`. Applications
 * can define other sizes.
 *
 * It’s also possible to use custom icons for a given state, for example:
 *
 * |[<!-- language="C" -->
 * stock["my-stock-item"] =
 * {
 *   { "itemprelight.png", *, PRELIGHT },
 *   { "iteminsensitive.png", *, INSENSITIVE },
 *   { "itemgeneric.png" } // implicit *, *, * as a fallback
 * }
 * ]|
 *
 * When selecting an icon source to use, CTK+ will consider text direction most
 * important, state second, and size third. It will select the best match based on
 * those criteria. If an attribute matches exactly (e.g. you specified
 * `PRELIGHT` or specified the size), CTK+ won’t modify the image;
 * if the attribute matches with a wildcard, CTK+ will scale or modify the image to
 * match the state and size the user requested.
 *
 * # Key bindings #
 *
 * Key bindings allow the user to specify actions to be
 * taken on particular key presses. The form of a binding
 * set declaration is:
 *
 * |[
 * binding name {
 *   bind key {
 *     signalname (param, ...)
 *     ...
 *   }
 *   ...
 * }
 * ]|
 *
 * `key` is a string consisting of a series of modifiers followed by
 * the name of a key. The modifiers can be:
 *
 * - `<alt>`
 *
 * - `<ctl>`
 *
 * - `<control>`
 *
 * - `<meta>`
 *
 * - `<hyper>`
 *
 * - `<super>`
 *
 * - `<mod1>`
 *
 * - `<mod2>`
 *
 * - `<mod3>`
 *
 * - `<mod4>`
 *
 * - `<mod5>`
 *
 * - `<release>`
 *
 * - `<shft>`
 *
 * - `<shift>`
 *
 * `<shft>` is an alias for `<shift>`, `<ctl>` is an alias for
 * `<control>`, and `<alt>` is an alias for `<mod1>`.
 *
 * The action that is bound to the key is a sequence of signal names
 * (strings) followed by parameters for each signal. The signals must
 * be action signals. (See g_signal_new()). Each parameter can be a
 * float, integer, string, or unquoted string representing an enumeration
 * value. The types of the parameters specified must match the types of
 * the parameters of the signal.
 *
 * Binding sets are connected to widgets in the same manner as styles,
 * with one difference: Binding sets override other binding sets first
 * by pattern type, then by priority and then by order of specification.
 * The priorities that can be specified and their default values are the
 * same as for styles.
 */


enum 
{
  PATH_ELT_PSPEC,
  PATH_ELT_UNRESOLVED,
  PATH_ELT_TYPE
};

typedef struct
{
  gint type;
  union 
  {
    GType         class_type;
    gchar        *class_name;
    GPatternSpec *pspec;
  } elt;
} PathElt;

typedef struct {
  GSList *color_hashes;
} CtkRcStylePrivate;

#define CTK_RC_STYLE_GET_PRIVATE(obj) ((CtkRcStylePrivate *) ctk_rc_style_get_instance_private ((CtkRcStyle *) (obj)))

static void        ctk_rc_style_finalize             (GObject         *object);
static void        ctk_rc_style_real_merge           (CtkRcStyle      *dest,
                                                      CtkRcStyle      *src);
static CtkRcStyle* ctk_rc_style_real_create_rc_style (CtkRcStyle      *rc_style);
static CtkStyle*   ctk_rc_style_real_create_style    (CtkRcStyle      *rc_style);
static gint	   ctk_rc_properties_cmp	     (gconstpointer    bsearch_node1,
						      gconstpointer    bsearch_node2);

static void	   insert_rc_property		     (CtkRcStyle      *style,
						      CtkRcProperty   *property,
						      gboolean         replace);


static const GScannerConfig ctk_rc_scanner_config =
{
  .cset_skip_characters = (
                           " \t\r\n"
                          ),
  .cset_identifier_first = (
                            "_"
                            G_CSET_a_2_z
                            G_CSET_A_2_Z
                           ),
  .cset_identifier_nth = (
                          G_CSET_DIGITS
                          "-_"
                          G_CSET_a_2_z
                          G_CSET_A_2_Z
                         ),
  .cpair_comment_single = ( "#\n" ),
  
  .case_sensitive = TRUE,
  
  .skip_comment_multi = TRUE,
  .skip_comment_single = TRUE,
  .scan_comment_multi = TRUE,
  .scan_identifier = TRUE,
  .scan_identifier_1char = FALSE,
  .scan_identifier_NULL = FALSE,
  .scan_symbols = TRUE,
  .scan_binary = TRUE,
  .scan_octal = TRUE,
  .scan_float = TRUE,
  .scan_hex = TRUE,
  .scan_hex_dollar = TRUE,
  .scan_string_sq = TRUE,
  .scan_string_dq = TRUE,
  .numbers_2_int = TRUE,
  .int_2_float = FALSE,
  .identifier_2_string = FALSE,
  .char_2_token = TRUE,
  .symbol_2_token = TRUE,
  .scope_0_fallback = FALSE,
};

static GHashTable *realized_style_ht = NULL;

static gchar *im_module_file = NULL;

static gchar **ctk_rc_default_files = NULL;

/* RC file handling */

static gchar *
ctk_rc_make_default_dir (const gchar *type)
{
  const gchar *var;
  gchar *path;

  var = g_getenv ("CTK_EXE_PREFIX");

  if (var)
    path = g_build_filename (var, "lib", "ctk-3.0", CTK_BINARY_VERSION, type, NULL);
  else
    path = g_build_filename (_ctk_get_libdir (), "ctk-3.0", CTK_BINARY_VERSION, type, NULL);

  return path;
}

/**
 * ctk_rc_get_im_module_path:
 *
 * Obtains the path in which to look for IM modules. See the documentation
 * of the `CTK_PATH`
 * environment variable for more details about looking up modules. This
 * function is useful solely for utilities supplied with CTK+ and should
 * not be used by applications under normal circumstances.
 *
 * Returns: (type filename): a newly-allocated string containing the
 *    path in which to look for IM modules.
 */
gchar *
ctk_rc_get_im_module_path (void)
{
  gchar **paths = _ctk_get_module_path ("immodules");
  gchar *result = g_strjoinv (G_SEARCHPATH_SEPARATOR_S, paths);
  g_strfreev (paths);

  return result;
}

/**
 * ctk_rc_get_im_module_file:
 *
 * Obtains the path to the IM modules file. See the documentation
 * of the `CTK_IM_MODULE_FILE`
 * environment variable for more details.
 *
 * Returns: (type filename): a newly-allocated string containing the
 *    name of the file listing the IM modules available for loading
 */
gchar *
ctk_rc_get_im_module_file (void)
{
  const gchar *var = g_getenv ("CTK_IM_MODULE_FILE");
  gchar *result = NULL;

  if (var)
    result = g_strdup (var);

  if (!result)
    {
      if (im_module_file)
        result = g_strdup (im_module_file);
      else
        result = ctk_rc_make_default_dir ("immodules.cache");
    }

  return result;
}

/**
 * ctk_rc_get_theme_dir:
 *
 * Returns the standard directory in which themes should
 * be installed. (CTK+ does not actually use this directory
 * itself.)
 *
 * Returns: The directory (must be freed with g_free()).
 */
gchar *
ctk_rc_get_theme_dir (void)
{
  const gchar *var;
  gchar *path;

  var = g_getenv ("CTK_DATA_PREFIX");

  if (var)
    path = g_build_filename (var, "share", "themes", NULL);
  else
    path = g_build_filename (_ctk_get_data_prefix (), "share", "themes", NULL);

  return path;
}

/**
 * ctk_rc_get_module_dir:
 *
 * Returns a directory in which CTK+ looks for theme engines.
 * For full information about the search for theme engines,
 * see the docs for `CTK_PATH` in [Running CTK+ Applications][ctk-running].
 *
 * return value: (type filename): the directory. (Must be freed with g_free())
 **/
gchar *
ctk_rc_get_module_dir (void)
{
  return ctk_rc_make_default_dir ("engines");
}

/**
 * ctk_rc_add_default_file:
 * @filename: (type filename): the pathname to the file. If @filename
 *    is not absolute, it is searched in the current directory.
 *
 * Adds a file to the list of files to be parsed at the
 * end of ctk_init().
 **/
void
ctk_rc_add_default_file (const gchar *filename G_GNUC_UNUSED)
{
}

/**
 * ctk_rc_set_default_files:
 * @filenames: (array zero-terminated=1) (element-type filename): A
 *     %NULL-terminated list of filenames.
 *
 * Sets the list of files that CTK+ will read at the
 * end of ctk_init().
 **/
void
ctk_rc_set_default_files (gchar **filenames G_GNUC_UNUSED)
{
}

/**
 * ctk_rc_get_default_files:
 *
 * Retrieves the current list of RC files that will be parsed
 * at the end of ctk_init().
 *
 * Returns: (transfer none) (array zero-terminated=1) (element-type filename):
 *      A %NULL-terminated array of filenames.  This memory is owned
 *     by CTK+ and must not be freed by the application.  If you want
 *     to store this information, you should make a copy.
 **/
gchar **
ctk_rc_get_default_files (void)
{
  return ctk_rc_default_files;
}

/**
 * ctk_rc_parse_string:
 * @rc_string: a string to parse.
 *
 * Parses resource information directly from a string.
 */
void
ctk_rc_parse_string (const gchar *rc_string)
{
  g_return_if_fail (rc_string != NULL);
}

/**
 * ctk_rc_parse:
 * @filename: the filename of a file to parse. If @filename is not absolute, it
 *  is searched in the current directory.
 *
 * Parses a given resource file.
 */
void
ctk_rc_parse (const gchar *filename)
{
  g_return_if_fail (filename != NULL);
}

/* Handling of RC styles */

G_DEFINE_TYPE_WITH_PRIVATE (CtkRcStyle, ctk_rc_style, G_TYPE_OBJECT)

static void
ctk_rc_style_init (CtkRcStyle *style)
{
  CtkRcStylePrivate *priv = CTK_RC_STYLE_GET_PRIVATE (style);
  guint i;

  style->name = NULL;
  style->font_desc = NULL;

  for (i = 0; i < 5; i++)
    {
      static const CdkColor init_color = { 0, 0, 0, 0, };

      style->bg_pixmap_name[i] = NULL;
      style->color_flags[i] = 0;
      style->fg[i] = init_color;
      style->bg[i] = init_color;
      style->text[i] = init_color;
      style->base[i] = init_color;
    }
  style->xthickness = -1;
  style->ythickness = -1;
  style->rc_properties = NULL;

  style->rc_style_lists = NULL;
  style->icon_factories = NULL;

  priv->color_hashes = NULL;
}

static void
ctk_rc_style_class_init (CtkRcStyleClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = ctk_rc_style_finalize;

  klass->parse = NULL;
  klass->create_rc_style = ctk_rc_style_real_create_rc_style;
  klass->merge = ctk_rc_style_real_merge;
  klass->create_style = ctk_rc_style_real_create_style;
}

static void
ctk_rc_style_finalize (GObject *object)
{
  GSList *tmp_list1, *tmp_list2;
  CtkRcStyle *rc_style;
  CtkRcStylePrivate *rc_priv;
  gint i;

  rc_style = CTK_RC_STYLE (object);
  rc_priv = CTK_RC_STYLE_GET_PRIVATE (rc_style);

  g_free (rc_style->name);
  if (rc_style->font_desc)
    pango_font_description_free (rc_style->font_desc);

  for (i = 0; i < 5; i++)
    g_free (rc_style->bg_pixmap_name[i]);

  /* Now remove all references to this rc_style from
   * realized_style_ht
   */
  tmp_list1 = rc_style->rc_style_lists;
  while (tmp_list1)
    {
      GSList *rc_styles = tmp_list1->data;
      CtkStyle *style = g_hash_table_lookup (realized_style_ht, rc_styles);
      g_object_unref (style);

      /* Remove the list of styles from the other rc_styles
       * in the list
       */
      tmp_list2 = rc_styles;
      while (tmp_list2)
        {
          CtkRcStyle *other_style = tmp_list2->data;

          if (other_style != rc_style)
            other_style->rc_style_lists = g_slist_remove_all (other_style->rc_style_lists,
							      rc_styles);
          tmp_list2 = tmp_list2->next;
        }

      /* And from the hash table itself
       */
      g_hash_table_remove (realized_style_ht, rc_styles);
      g_slist_free (rc_styles);

      tmp_list1 = tmp_list1->next;
    }
  g_slist_free (rc_style->rc_style_lists);

  if (rc_style->rc_properties)
    {
      for (i = 0; i < rc_style->rc_properties->len; i++)
	{
	  CtkRcProperty *node = &g_array_index (rc_style->rc_properties, CtkRcProperty, i);

	  g_free (node->origin);
	  g_value_unset (&node->value);
	}
      g_array_free (rc_style->rc_properties, TRUE);
      rc_style->rc_properties = NULL;
    }

  g_slist_free_full (rc_style->icon_factories, g_object_unref);
  g_slist_free_full (rc_priv->color_hashes, (GDestroyNotify)g_hash_table_unref);

  G_OBJECT_CLASS (ctk_rc_style_parent_class)->finalize (object);
}

/**
 * ctk_rc_style_new:
 *
 * Creates a new #CtkRcStyle with no fields set and
 * a reference count of 1.
 *
 * Returns: the newly-created #CtkRcStyle
 */
CtkRcStyle *
ctk_rc_style_new (void)
{
  CtkRcStyle *style;

  style = g_object_new (CTK_TYPE_RC_STYLE, NULL);

  return style;
}

/**
 * ctk_rc_style_copy:
 * @orig: the style to copy
 *
 * Makes a copy of the specified #CtkRcStyle. This function
 * will correctly copy an RC style that is a member of a class
 * derived from #CtkRcStyle.
 *
 * Returns: (transfer full): the resulting #CtkRcStyle
 **/
CtkRcStyle *
ctk_rc_style_copy (CtkRcStyle *orig)
{
  CtkRcStyle *style;

  g_return_val_if_fail (CTK_IS_RC_STYLE (orig), NULL);
  
  style = CTK_RC_STYLE_GET_CLASS (orig)->create_rc_style (orig);
  CTK_RC_STYLE_GET_CLASS (style)->merge (style, orig);

  return style;
}

static CtkRcStyle *
ctk_rc_style_real_create_rc_style (CtkRcStyle *style)
{
  return g_object_new (G_OBJECT_TYPE (style), NULL);
}

static gint
ctk_rc_properties_cmp (gconstpointer bsearch_node1,
		       gconstpointer bsearch_node2)
{
  const CtkRcProperty *prop1 = bsearch_node1;
  const CtkRcProperty *prop2 = bsearch_node2;

  if (prop1->type_name == prop2->type_name)
    return prop1->property_name < prop2->property_name ? -1 : prop1->property_name == prop2->property_name ? 0 : 1;
  else
    return prop1->type_name < prop2->type_name ? -1 : 1;
}

static void
insert_rc_property (CtkRcStyle    *style,
		    CtkRcProperty *property,
		    gboolean       replace)
{
  guint i;
  CtkRcProperty *new_property = NULL;
  CtkRcProperty key = { 0, 0, NULL, { 0, }, };

  key.type_name = property->type_name;
  key.property_name = property->property_name;

  if (!style->rc_properties)
    style->rc_properties = g_array_new (FALSE, FALSE, sizeof (CtkRcProperty));

  i = 0;
  while (i < style->rc_properties->len)
    {
      gint cmp = ctk_rc_properties_cmp (&key, &g_array_index (style->rc_properties, CtkRcProperty, i));

      if (cmp == 0)
	{
	  if (replace)
	    {
	      new_property = &g_array_index (style->rc_properties, CtkRcProperty, i);
	      
	      g_free (new_property->origin);
	      g_value_unset (&new_property->value);
	      
	      *new_property = key;
	      break;
	    }
	  else
	    return;
	}
      else if (cmp < 0)
	break;

      i++;
    }

  if (!new_property)
    {
      g_array_insert_val (style->rc_properties, i, key);
      new_property = &g_array_index (style->rc_properties, CtkRcProperty, i);
    }

  new_property->origin = g_strdup (property->origin);
  g_value_init (&new_property->value, G_VALUE_TYPE (&property->value));
  g_value_copy (&property->value, &new_property->value);
}

static void
ctk_rc_style_real_merge (CtkRcStyle *dest,
			 CtkRcStyle *src)
{
  gint i;

  for (i = 0; i < 5; i++)
    {
      if (!dest->bg_pixmap_name[i] && src->bg_pixmap_name[i])
	dest->bg_pixmap_name[i] = g_strdup (src->bg_pixmap_name[i]);
      
      if (!(dest->color_flags[i] & CTK_RC_FG) && 
	  src->color_flags[i] & CTK_RC_FG)
	{
	  dest->fg[i] = src->fg[i];
	  dest->color_flags[i] |= CTK_RC_FG;
	}
      if (!(dest->color_flags[i] & CTK_RC_BG) && 
	  src->color_flags[i] & CTK_RC_BG)
	{
	  dest->bg[i] = src->bg[i];
	  dest->color_flags[i] |= CTK_RC_BG;
	}
      if (!(dest->color_flags[i] & CTK_RC_TEXT) && 
	  src->color_flags[i] & CTK_RC_TEXT)
	{
	  dest->text[i] = src->text[i];
	  dest->color_flags[i] |= CTK_RC_TEXT;
	}
      if (!(dest->color_flags[i] & CTK_RC_BASE) && 
	  src->color_flags[i] & CTK_RC_BASE)
	{
	  dest->base[i] = src->base[i];
	  dest->color_flags[i] |= CTK_RC_BASE;
	}
    }

  if (dest->xthickness < 0 && src->xthickness >= 0)
    dest->xthickness = src->xthickness;
  if (dest->ythickness < 0 && src->ythickness >= 0)
    dest->ythickness = src->ythickness;

  if (src->font_desc)
    {
      if (!dest->font_desc)
	dest->font_desc = pango_font_description_copy (src->font_desc);
      else
	pango_font_description_merge (dest->font_desc, src->font_desc, FALSE);
    }

  if (src->rc_properties)
    {
      for (i = 0; i < src->rc_properties->len; i++)
	insert_rc_property (dest,
			    &g_array_index (src->rc_properties, CtkRcProperty, i),
			    FALSE);
    }
}

static CtkStyle *
ctk_rc_style_real_create_style (CtkRcStyle *rc_style G_GNUC_UNUSED)
{
  return ctk_style_new ();
}

/**
 * ctk_rc_reset_styles:
 * @settings: a #CtkSettings
 *
 * This function recomputes the styles for all widgets that use a
 * particular #CtkSettings object. (There is one #CtkSettings object
 * per #CdkScreen, see ctk_settings_get_for_screen()); It is useful
 * when some global parameter has changed that affects the appearance
 * of all widgets, because when a widget gets a new style, it will
 * both redraw and recompute any cached information about its
 * appearance. As an example, it is used when the default font size
 * set by the operating system changes. Note that this function
 * doesn’t affect widgets that have a style set explicitly on them
 * with ctk_widget_set_style().
 *
 * Since: 2.4
 **/
void
ctk_rc_reset_styles (CtkSettings *settings)
{
  ctk_style_context_reset_widgets (_ctk_settings_get_screen (settings));
}

/**
 * ctk_rc_reparse_all_for_settings:
 * @settings: a #CtkSettings
 * @force_load: load whether or not anything changed
 *
 * If the modification time on any previously read file
 * for the given #CtkSettings has changed, discard all style information
 * and then reread all previously read RC files.
 *
 * Returns: %TRUE if the files were reread.
 **/
gboolean
ctk_rc_reparse_all_for_settings (CtkSettings *settings G_GNUC_UNUSED,
				 gboolean     force_load G_GNUC_UNUSED)
{
  return FALSE;
}

/**
 * ctk_rc_reparse_all:
 *
 * If the modification time on any previously read file for the
 * default #CtkSettings has changed, discard all style information
 * and then reread all previously read RC files.
 *
 * Returns:  %TRUE if the files were reread.
 **/
gboolean
ctk_rc_reparse_all (void)
{
  return FALSE;
}

/**
 * ctk_rc_get_style:
 * @widget: a #CtkWidget
 *
 * Finds all matching RC styles for a given widget,
 * composites them together, and then creates a
 * #CtkStyle representing the composite appearance.
 * (CTK+ actually keeps a cache of previously
 * created styles, so a new style may not be
 * created.)
 *
 * Returns: (transfer none): the resulting style. No refcount is added
 *   to the returned style, so if you want to save this style around,
 *   you should add a reference yourself.
 **/
CtkStyle *
ctk_rc_get_style (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  ctk_widget_ensure_style (widget);

  return ctk_widget_get_style (widget);
}

/**
 * ctk_rc_get_style_by_paths:
 * @settings: a #CtkSettings object
 * @widget_path: (allow-none): the widget path to use when looking up the
 *     style, or %NULL if no matching against the widget path should be done
 * @class_path: (allow-none): the class path to use when looking up the style,
 *     or %NULL if no matching against the class path should be done.
 * @type: a type that will be used along with parent types of this type
 *     when matching against class styles, or #G_TYPE_NONE
 *
 * Creates up a #CtkStyle from styles defined in a RC file by providing
 * the raw components used in matching. This function may be useful
 * when creating pseudo-widgets that should be themed like widgets but
 * don’t actually have corresponding CTK+ widgets. An example of this
 * would be items inside a GNOME canvas widget.
 *
 * The action of ctk_rc_get_style() is similar to:
 * |[<!-- language="C" -->
 *  ctk_widget_path (widget, NULL, &path, NULL);
 *  ctk_widget_class_path (widget, NULL, &class_path, NULL);
 *  ctk_rc_get_style_by_paths (ctk_widget_get_settings (widget),
 *                             path, class_path,
 *                             G_OBJECT_TYPE (widget));
 * ]|
 *
 * Returns: (nullable) (transfer none): A style created by matching
 *     with the supplied paths, or %NULL if nothing matching was
 *     specified and the default style should be used. The returned
 *     value is owned by CTK+ as part of an internal cache, so you
 *     must call g_object_ref() on the returned value if you want to
 *     keep a reference to it.
 **/
CtkStyle *
ctk_rc_get_style_by_paths (CtkSettings *settings,
			   const char  *widget_path,
			   const char  *class_path,
			   GType        type)
{
  CtkWidgetPath *path;
  CtkStyle *style;

  path = ctk_widget_path_new ();

  /* For compatibility, we return a CtkStyle based on a CtkStyleContext
   * with a CtkWidgetPath appropriate for the supplied information.
   *
   * CtkWidgetPath is composed of a list of GTypes with optional names;
   * In CTK+-2.0, widget_path consisted of the widget names, or
   * the class names for unnamed widgets, while class_path had the
   * class names always. So, use class_path to determine the GTypes
   * and extract widget names from widget_path as applicable.
   */
  if (class_path == NULL)
    {
      ctk_widget_path_append_type (path, type == G_TYPE_NONE ? CTK_TYPE_WIDGET : type);
    }
  else
    {
      const gchar *widget_p, *widget_next;
      const gchar *class_p, *class_next;

      widget_next = widget_path;
      class_next = class_path;

      while (*class_next)
	{
	  GType component_type;
	  gchar *component_class;
	  gchar *component_name;
	  gint pos;

	  class_p = class_next;
	  if (*class_p == '.')
	    class_p++;

	  widget_p = widget_next; /* Might be NULL */
	  if (widget_p && *widget_p == '.')
	    widget_p++;

	  class_next = strchr (class_p, '.');
	  if (class_next == NULL)
	    class_next = class_p + strlen (class_p);

	  if (widget_p)
	    {
	      widget_next = strchr (widget_p, '.');
	      if (widget_next == NULL)
		widget_next = widget_p + strlen (widget_p);
	    }

	  component_class = g_strndup (class_p, class_next - class_p);
	  if (widget_p && *widget_p)
	    component_name = g_strndup (widget_p, widget_next - widget_p);
	  else
	    component_name = NULL;

	  component_type = g_type_from_name (component_class);
	  if (component_type == G_TYPE_INVALID)
	    component_type = CTK_TYPE_WIDGET;

	  pos = ctk_widget_path_append_type (path, component_type);
	  if (component_name != NULL && strcmp (component_name, component_name) != 0)
	    ctk_widget_path_iter_set_name (path, pos, component_name);

	  g_free (component_class);
	  g_free (component_name);
	}
    }

  style = _ctk_style_new_for_path (_ctk_settings_get_screen (settings),
				   path);

  ctk_widget_path_free (path);

  return style;
}

/**
 * ctk_rc_scanner_new: (skip)
 */
GScanner*
ctk_rc_scanner_new (void)
{
  return g_scanner_new (&ctk_rc_scanner_config);
}

/*********************
 * Parsing functions *
 *********************/

static gboolean
lookup_color (CtkRcStyle *style,
              const char *color_name,
              CdkColor   *color)
{
  CtkRcStylePrivate *priv = CTK_RC_STYLE_GET_PRIVATE (style);
  GSList *iter;

  for (iter = priv->color_hashes; iter != NULL; iter = iter->next)
    {
      GHashTable *hash  = iter->data;
      CdkColor   *match = g_hash_table_lookup (hash, color_name);

      if (match)
        {
          color->red = match->red;
          color->green = match->green;
          color->blue = match->blue;
          return TRUE;
        }
    }

  return FALSE;
}

/**
 * ctk_rc_find_pixmap_in_path:
 * @settings: a #CtkSettings
 * @scanner: Scanner used to get line number information for the
 *   warning message, or %NULL
 * @pixmap_file: name of the pixmap file to locate.
 *
 * Looks up a file in pixmap path for the specified #CtkSettings.
 * If the file is not found, it outputs a warning message using
 * g_warning() and returns %NULL.
 *
 * Returns: (type filename): the filename.
 **/
gchar*
ctk_rc_find_pixmap_in_path (CtkSettings  *settings G_GNUC_UNUSED,
			    GScanner     *scanner G_GNUC_UNUSED,
			    const gchar  *pixmap_file)
{
  g_warning ("Unable to locate image file in pixmap_path: \"%s\"",
             pixmap_file);
  return NULL;
}

/**
 * ctk_rc_find_module_in_path:
 * @module_file: name of a theme engine
 *
 * Searches for a theme engine in the CTK+ search path. This function
 * is not useful for applications and should not be used.
 *
 * Returns: (type filename): The filename, if found (must be
 *   freed with g_free()), otherwise %NULL.
 **/
gchar*
ctk_rc_find_module_in_path (const gchar *module_file)
{
  return _ctk_find_module (module_file, "engines");
}

/**
 * ctk_rc_parse_state:
 * @scanner: a #GScanner (must be initialized for parsing an RC file)
 * @state: (out): A pointer to a #CtkStateType variable in which to
 *  store the result.
 *
 * Parses a #CtkStateType variable from the format expected
 * in a RC file.
 *
 * Returns: %G_TOKEN_NONE if parsing succeeded, otherwise the token
 *   that was expected but not found.
 */
guint
ctk_rc_parse_state (GScanner	 *scanner,
		    CtkStateType *state)
{
  guint old_scope;
  guint token;

  g_return_val_if_fail (scanner != NULL, G_TOKEN_ERROR);
  g_return_val_if_fail (state != NULL, G_TOKEN_ERROR);
  
  /* we don't know where we got called from, so we reset the scope here.
   * if we bail out due to errors, we *don't* reset the scope, so the
   * error messaging code can make sense of our tokens.
   */
  old_scope = g_scanner_set_scope (scanner, 0);
  
  token = g_scanner_get_next_token (scanner);
  if (token != G_TOKEN_LEFT_BRACE)
    return G_TOKEN_LEFT_BRACE;
  
  token = g_scanner_get_next_token (scanner);
  switch (token)
    {
    case CTK_RC_TOKEN_ACTIVE:
      *state = CTK_STATE_ACTIVE;
      break;
    case CTK_RC_TOKEN_INSENSITIVE:
      *state = CTK_STATE_INSENSITIVE;
      break;
    case CTK_RC_TOKEN_NORMAL:
      *state = CTK_STATE_NORMAL;
      break;
    case CTK_RC_TOKEN_PRELIGHT:
      *state = CTK_STATE_PRELIGHT;
      break;
    case CTK_RC_TOKEN_SELECTED:
      *state = CTK_STATE_SELECTED;
      break;
    default:
      return /* G_TOKEN_SYMBOL */ CTK_RC_TOKEN_NORMAL;
    }
  
  token = g_scanner_get_next_token (scanner);
  if (token != G_TOKEN_RIGHT_BRACE)
    return G_TOKEN_RIGHT_BRACE;
  
  g_scanner_set_scope (scanner, old_scope);

  return G_TOKEN_NONE;
}

/**
 * ctk_rc_parse_priority:
 * @scanner: a #GScanner (must be initialized for parsing an RC file)
 * @priority: A pointer to #CtkPathPriorityType variable in which
 *  to store the result.
 *
 * Parses a #CtkPathPriorityType variable from the format expected
 * in a RC file.
 *
 * Returns: %G_TOKEN_NONE if parsing succeeded, otherwise the token
 *   that was expected but not found.
 */
guint
ctk_rc_parse_priority (GScanner	           *scanner,
		       CtkPathPriorityType *priority)
{
  guint old_scope;
  guint token;

  g_return_val_if_fail (scanner != NULL, G_TOKEN_ERROR);
  g_return_val_if_fail (priority != NULL, G_TOKEN_ERROR);

  /* we don't know where we got called from, so we reset the scope here.
   * if we bail out due to errors, we *don't* reset the scope, so the
   * error messaging code can make sense of our tokens.
   */
  old_scope = g_scanner_set_scope (scanner, 0);
  
  token = g_scanner_get_next_token (scanner);
  if (token != ':')
    return ':';
  
  token = g_scanner_get_next_token (scanner);
  switch (token)
    {
    case CTK_RC_TOKEN_LOWEST:
      *priority = CTK_PATH_PRIO_LOWEST;
      break;
    case CTK_RC_TOKEN_CTK:
      *priority = CTK_PATH_PRIO_CTK;
      break;
    case CTK_RC_TOKEN_APPLICATION:
      *priority = CTK_PATH_PRIO_APPLICATION;
      break;
    case CTK_RC_TOKEN_THEME:
      *priority = CTK_PATH_PRIO_THEME;
      break;
    case CTK_RC_TOKEN_RC:
      *priority = CTK_PATH_PRIO_RC;
      break;
    case CTK_RC_TOKEN_HIGHEST:
      *priority = CTK_PATH_PRIO_HIGHEST;
      break;
    default:
      return /* G_TOKEN_SYMBOL */ CTK_RC_TOKEN_APPLICATION;
    }
  
  g_scanner_set_scope (scanner, old_scope);

  return G_TOKEN_NONE;
}

/**
 * ctk_rc_parse_color:
 * @scanner: a #GScanner
 * @color: (out): a pointer to a #CdkColor in which to store
 *     the result
 *
 * Parses a color in the format expected
 * in a RC file.
 *
 * Note that theme engines should use ctk_rc_parse_color_full() in
 * order to support symbolic colors.
 *
 * Returns: %G_TOKEN_NONE if parsing succeeded, otherwise the token
 *     that was expected but not found
 */
guint
ctk_rc_parse_color (GScanner *scanner,
		    CdkColor *color)
{
  return ctk_rc_parse_color_full (scanner, NULL, color);
}

/**
 * ctk_rc_parse_color_full:
 * @scanner: a #GScanner
 * @style: (allow-none): a #CtkRcStyle, or %NULL
 * @color: (out): a pointer to a #CdkColor in which to store
 *     the result
 *
 * Parses a color in the format expected
 * in a RC file. If @style is not %NULL, it will be consulted to resolve
 * references to symbolic colors.
 *
 * Returns: %G_TOKEN_NONE if parsing succeeded, otherwise the token
 *     that was expected but not found
 *
 * Since: 2.12
 */
guint
ctk_rc_parse_color_full (GScanner   *scanner,
                         CtkRcStyle *style,
                         CdkColor   *color)
{
  guint token;

  g_return_val_if_fail (scanner != NULL, G_TOKEN_ERROR);

  /* we don't need to set our own scope here, because
   * we don't need own symbols
   */
  
  token = g_scanner_get_next_token (scanner);
  switch (token)
    {
      gint token_int;
      CdkColor c1, c2;
      gboolean negate;
      gdouble l;

    case G_TOKEN_LEFT_CURLY:
      token = g_scanner_get_next_token (scanner);
      if (token == G_TOKEN_INT)
	token_int = scanner->value.v_int;
      else if (token == G_TOKEN_FLOAT)
	token_int = scanner->value.v_float * 65535.0;
      else
	return G_TOKEN_FLOAT;
      color->red = CLAMP (token_int, 0, 65535);
      
      token = g_scanner_get_next_token (scanner);
      if (token != G_TOKEN_COMMA)
	return G_TOKEN_COMMA;
      
      token = g_scanner_get_next_token (scanner);
      if (token == G_TOKEN_INT)
	token_int = scanner->value.v_int;
      else if (token == G_TOKEN_FLOAT)
	token_int = scanner->value.v_float * 65535.0;
      else
	return G_TOKEN_FLOAT;
      color->green = CLAMP (token_int, 0, 65535);
      
      token = g_scanner_get_next_token (scanner);
      if (token != G_TOKEN_COMMA)
	return G_TOKEN_COMMA;
      
      token = g_scanner_get_next_token (scanner);
      if (token == G_TOKEN_INT)
	token_int = scanner->value.v_int;
      else if (token == G_TOKEN_FLOAT)
	token_int = scanner->value.v_float * 65535.0;
      else
	return G_TOKEN_FLOAT;
      color->blue = CLAMP (token_int, 0, 65535);
      
      token = g_scanner_get_next_token (scanner);
      if (token != G_TOKEN_RIGHT_CURLY)
	return G_TOKEN_RIGHT_CURLY;
      return G_TOKEN_NONE;
      
    case G_TOKEN_STRING:
      if (!cdk_color_parse (scanner->value.v_string, color))
	{
          g_scanner_warn (scanner, "Invalid color constant '%s'",
                          scanner->value.v_string);
          return G_TOKEN_STRING;
	}
      return G_TOKEN_NONE;

    case '@':
      token = g_scanner_get_next_token (scanner);
      if (token != G_TOKEN_IDENTIFIER)
	return G_TOKEN_IDENTIFIER;

      if (!style || !lookup_color (style, scanner->value.v_identifier, color))
        {
          g_scanner_warn (scanner, "Invalid symbolic color '%s'",
                          scanner->value.v_identifier);
          return G_TOKEN_IDENTIFIER;
        }

      return G_TOKEN_NONE;

    case G_TOKEN_IDENTIFIER:
      if (strcmp (scanner->value.v_identifier, "mix") == 0)
        {
          token = g_scanner_get_next_token (scanner);
          if (token != G_TOKEN_LEFT_PAREN)
            return G_TOKEN_LEFT_PAREN;

          negate = FALSE;
          if (g_scanner_peek_next_token (scanner) == '-')
            {
              g_scanner_get_next_token (scanner); /* eat sign */
              negate = TRUE;
            }

          token = g_scanner_get_next_token (scanner);
          if (token != G_TOKEN_FLOAT)
            return G_TOKEN_FLOAT;

          l = negate ? -scanner->value.v_float : scanner->value.v_float;

          token = g_scanner_get_next_token (scanner);
          if (token != G_TOKEN_COMMA)
            return G_TOKEN_COMMA;

          token = ctk_rc_parse_color_full (scanner, style, &c1);
          if (token != G_TOKEN_NONE)
            return token;

	  token = g_scanner_get_next_token (scanner);
	  if (token != G_TOKEN_COMMA)
            return G_TOKEN_COMMA;

	  token = ctk_rc_parse_color_full (scanner, style, &c2);
	  if (token != G_TOKEN_NONE)
            return token;

	  token = g_scanner_get_next_token (scanner);
	  if (token != G_TOKEN_RIGHT_PAREN)
            return G_TOKEN_RIGHT_PAREN;

	  color->red   = l * c1.red   + (1.0 - l) * c2.red;
	  color->green = l * c1.green + (1.0 - l) * c2.green;
	  color->blue  = l * c1.blue  + (1.0 - l) * c2.blue;

	  return G_TOKEN_NONE;
	}
      else if (strcmp (scanner->value.v_identifier, "shade") == 0)
        {
	  token = g_scanner_get_next_token (scanner);
          if (token != G_TOKEN_LEFT_PAREN)
            return G_TOKEN_LEFT_PAREN;

          negate = FALSE;
          if (g_scanner_peek_next_token (scanner) == '-')
            {
              g_scanner_get_next_token (scanner); /* eat sign */
              negate = TRUE;
            }

          token = g_scanner_get_next_token (scanner);
          if (token != G_TOKEN_FLOAT)
            return G_TOKEN_FLOAT;

          l = negate ? -scanner->value.v_float : scanner->value.v_float;

          token = g_scanner_get_next_token (scanner);
          if (token != G_TOKEN_COMMA)
            return G_TOKEN_COMMA;

          token = ctk_rc_parse_color_full (scanner, style, &c1);
          if (token != G_TOKEN_NONE)
            return token;

          token = g_scanner_get_next_token (scanner);
          if (token != G_TOKEN_RIGHT_PAREN)
            return G_TOKEN_RIGHT_PAREN;

          _ctk_style_shade (&c1, color, l);

          return G_TOKEN_NONE;
        }
      else if (strcmp (scanner->value.v_identifier, "lighter") == 0 ||
               strcmp (scanner->value.v_identifier, "darker") == 0)
        {
          if (scanner->value.v_identifier[0] == 'l')
            l = 1.3;
          else
	    l = 0.7;

	  token = g_scanner_get_next_token (scanner);
          if (token != G_TOKEN_LEFT_PAREN)
            return G_TOKEN_LEFT_PAREN;

          token = ctk_rc_parse_color_full (scanner, style, &c1);
          if (token != G_TOKEN_NONE)
            return token;

          token = g_scanner_get_next_token (scanner);
          if (token != G_TOKEN_RIGHT_PAREN)
            return G_TOKEN_RIGHT_PAREN;

          _ctk_style_shade (&c1, color, l);

          return G_TOKEN_NONE;
        }
      else
        return G_TOKEN_IDENTIFIER;

    default:
      return G_TOKEN_STRING;
    }
}

typedef struct {
  CtkPathType   type;
  GPatternSpec *pspec;
  gpointer      user_data;
  guint         seq_id;
} PatternSpec;

static void
pattern_spec_free (PatternSpec *pspec)
{
  if (pspec->pspec)
    g_pattern_spec_free (pspec->pspec);
  g_free (pspec);
}

/**
 * ctk_binding_set_add_path:
 * @binding_set: a #CtkBindingSet to add a path to
 * @path_type: path type the pattern applies to
 * @path_pattern: the actual match pattern
 * @priority: binding priority
 *
 * This function was used internally by the CtkRC parsing mechanism
 * to assign match patterns to #CtkBindingSet structures.
 *
 * In CTK+ 3, these match patterns are unused.
 */
void
ctk_binding_set_add_path (CtkBindingSet       *binding_set,
                          CtkPathType          path_type,
                          const gchar         *path_pattern,
                          CtkPathPriorityType  priority)
{
  PatternSpec *pspec;
  GSList **slist_p, *slist;
  static guint seq_id = 0;

  g_return_if_fail (binding_set != NULL);
  g_return_if_fail (path_pattern != NULL);
  g_return_if_fail (priority <= CTK_PATH_PRIO_MASK);

  priority &= CTK_PATH_PRIO_MASK;

  switch (path_type)
    {
    case  CTK_PATH_WIDGET:
      slist_p = &binding_set->widget_path_pspecs;
      break;
    case  CTK_PATH_WIDGET_CLASS:
      slist_p = &binding_set->widget_class_pspecs;
      break;
    case  CTK_PATH_CLASS:
      slist_p = &binding_set->class_branch_pspecs;
      break;
    default:
      g_assert_not_reached ();
      slist_p = NULL;
      break;
    }

  pspec = g_new (PatternSpec, 1);
  pspec->type = path_type;
  if (path_type == CTK_PATH_WIDGET_CLASS)
    pspec->pspec = NULL;
  else
    pspec->pspec = g_pattern_spec_new (path_pattern);
  pspec->seq_id = priority << 28;
  pspec->user_data = binding_set;

  slist = *slist_p;
  while (slist)
    {
      PatternSpec *tmp_pspec;

      tmp_pspec = slist->data;
      slist = slist->next;

      if (g_pattern_spec_equal (tmp_pspec->pspec, pspec->pspec))
        {
          CtkPathPriorityType lprio = tmp_pspec->seq_id >> 28;

          pattern_spec_free (pspec);
          pspec = NULL;
          if (lprio < priority)
            {
              tmp_pspec->seq_id &= 0x0fffffff;
              tmp_pspec->seq_id |= priority << 28;
            }
          break;
        }
    }
  if (pspec)
    {
      pspec->seq_id |= seq_id++ & 0x0fffffff;
      *slist_p = g_slist_prepend (*slist_p, pspec);
    }
}
