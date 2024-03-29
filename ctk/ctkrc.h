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

#ifndef __CTK_RC_H__
#define __CTK_RC_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkwidget.h>
#include <ctk/ctkbindings.h>

G_BEGIN_DECLS

/* Forward declarations */
typedef struct _CtkRcContext    CtkRcContext;
typedef struct _CtkRcStyleClass CtkRcStyleClass;

#define CTK_TYPE_RC_STYLE              (ctk_rc_style_get_type ())
#define CTK_RC_STYLE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), CTK_TYPE_RC_STYLE, CtkRcStyle))
#define CTK_RC_STYLE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_RC_STYLE, CtkRcStyleClass))
#define CTK_IS_RC_STYLE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), CTK_TYPE_RC_STYLE))
#define CTK_IS_RC_STYLE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_RC_STYLE))
#define CTK_RC_STYLE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_RC_STYLE, CtkRcStyleClass))

/**
 * CtkRcFlags:
 */
typedef enum
{
  /*< private >*/
  CTK_RC_FG             = 1 << 0,
  CTK_RC_BG             = 1 << 1,
  CTK_RC_TEXT           = 1 << 2,
  CTK_RC_BASE           = 1 << 3
} CtkRcFlags;

/**
 * CtkRcStyle:
 * @name: Name
 * @bg_pixmap_name: Pixmap name
 * @font_desc: A #PangoFontDescription
 * @color_flags: #CtkRcFlags
 * @fg: Foreground colors
 * @bg: Background colors
 * @text: Text colors
 * @base: Base colors
 * @xthickness: X thickness
 * @ythickness: Y thickness
 *
 * The #CtkRcStyle-struct is used to represent a set
 * of information about the appearance of a widget.
 * This can later be composited together with other
 * #CtkRcStyle-struct<!-- -->s to form a #CtkStyle.
 */
struct _CtkRcStyle
{
  GObject parent_instance;

  /*< public >*/

  gchar *name;
  gchar *bg_pixmap_name[5];
  PangoFontDescription *font_desc;

  CtkRcFlags color_flags[5];
  CdkColor   fg[5];
  CdkColor   bg[5];
  CdkColor   text[5];
  CdkColor   base[5];

  gint xthickness;
  gint ythickness;

  /*< private >*/
  GArray *rc_properties;

  /* list of RC style lists including this RC style */
  GSList *rc_style_lists;

  GSList *icon_factories;

  guint engine_specified : 1;   /* The RC file specified the engine */
};

/**
 * CtkRcStyleClass:
 * @parent_class: The parent class.
 * @create_rc_style: 
 * @parse: 
 * @merge: 
 * @create_style: 
 */
struct _CtkRcStyleClass
{
  GObjectClass parent_class;

  /*< public >*/

  /* Create an empty RC style of the same type as this RC style.
   * The default implementation, which does
   * g_object_new (G_OBJECT_TYPE (style), NULL);
   * should work in most cases.
   */
  CtkRcStyle * (*create_rc_style) (CtkRcStyle *rc_style);

  /* Fill in engine specific parts of CtkRcStyle by parsing contents
   * of brackets. Returns G_TOKEN_NONE if successful, otherwise returns
   * the token it expected but didn't get.
   */
  guint     (*parse)  (CtkRcStyle   *rc_style,
                       CtkSettings  *settings,
                       GScanner     *scanner);

  /* Combine RC style data from src into dest. If overridden, this
   * function should chain to the parent.
   */
  void      (*merge)  (CtkRcStyle *dest,
                       CtkRcStyle *src);

  /* Create an empty style suitable to this RC style
   */
  CtkStyle * (*create_style) (CtkRcStyle *rc_style);

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GSList*   _ctk_rc_parse_widget_class_path (const gchar *pattern);
void      _ctk_rc_free_widget_class_path (GSList       *list);
gboolean  _ctk_rc_match_widget_class     (GSList       *list,
                                          gint          length,
                                          gchar        *path,
                                          gchar        *path_reversed);

CDK_AVAILABLE_IN_ALL
void      ctk_rc_add_default_file       (const gchar *filename);
CDK_AVAILABLE_IN_ALL
void      ctk_rc_set_default_files      (gchar **filenames);
CDK_AVAILABLE_IN_ALL
gchar**   ctk_rc_get_default_files      (void);
CDK_AVAILABLE_IN_ALL
CtkStyle* ctk_rc_get_style              (CtkWidget   *widget);
CDK_AVAILABLE_IN_ALL
CtkStyle* ctk_rc_get_style_by_paths     (CtkSettings *settings,
                                         const char  *widget_path,
                                         const char  *class_path,
                                         GType        type);

CDK_AVAILABLE_IN_ALL
gboolean ctk_rc_reparse_all_for_settings (CtkSettings *settings,
                                          gboolean     force_load);
CDK_AVAILABLE_IN_ALL
void     ctk_rc_reset_styles             (CtkSettings *settings);

CDK_AVAILABLE_IN_ALL
gchar*   ctk_rc_find_pixmap_in_path (CtkSettings  *settings,
                                     GScanner     *scanner,
                                     const gchar  *pixmap_file);

CDK_AVAILABLE_IN_ALL
void     ctk_rc_parse                   (const gchar *filename);
CDK_AVAILABLE_IN_ALL
void      ctk_rc_parse_string           (const gchar *rc_string);
CDK_AVAILABLE_IN_ALL
gboolean  ctk_rc_reparse_all            (void);

CDK_AVAILABLE_IN_ALL
GType       ctk_rc_style_get_type   (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkRcStyle* ctk_rc_style_new        (void);
CDK_AVAILABLE_IN_ALL
CtkRcStyle* ctk_rc_style_copy       (CtkRcStyle *orig);

CDK_AVAILABLE_IN_ALL
gchar*      ctk_rc_find_module_in_path (const gchar *module_file);
CDK_AVAILABLE_IN_ALL
gchar*      ctk_rc_get_theme_dir       (void);
CDK_AVAILABLE_IN_ALL
gchar*      ctk_rc_get_module_dir      (void);
CDK_AVAILABLE_IN_ALL
gchar*      ctk_rc_get_im_module_path  (void);
CDK_AVAILABLE_IN_ALL
gchar*      ctk_rc_get_im_module_file  (void);

/* private functions/definitions */

/**
 * CtkRcTokenType:
 * The #CtkRcTokenType enumeration represents the tokens
 * in the RC file. It is exposed so that theme engines
 * can reuse these tokens when parsing the theme-engine
 * specific portions of a RC file.
 */
typedef enum {
  /*< private >*/
  CTK_RC_TOKEN_INVALID = G_TOKEN_LAST,
  CTK_RC_TOKEN_INCLUDE,
  CTK_RC_TOKEN_NORMAL,
  CTK_RC_TOKEN_ACTIVE,
  CTK_RC_TOKEN_PRELIGHT,
  CTK_RC_TOKEN_SELECTED,
  CTK_RC_TOKEN_INSENSITIVE,
  CTK_RC_TOKEN_FG,
  CTK_RC_TOKEN_BG,
  CTK_RC_TOKEN_TEXT,
  CTK_RC_TOKEN_BASE,
  CTK_RC_TOKEN_XTHICKNESS,
  CTK_RC_TOKEN_YTHICKNESS,
  CTK_RC_TOKEN_FONT,
  CTK_RC_TOKEN_FONTSET,
  CTK_RC_TOKEN_FONT_NAME,
  CTK_RC_TOKEN_BG_PIXMAP,
  CTK_RC_TOKEN_PIXMAP_PATH,
  CTK_RC_TOKEN_STYLE,
  CTK_RC_TOKEN_BINDING,
  CTK_RC_TOKEN_BIND,
  CTK_RC_TOKEN_WIDGET,
  CTK_RC_TOKEN_WIDGET_CLASS,
  CTK_RC_TOKEN_CLASS,
  CTK_RC_TOKEN_LOWEST,
  CTK_RC_TOKEN_CTK,
  CTK_RC_TOKEN_APPLICATION,
  CTK_RC_TOKEN_THEME,
  CTK_RC_TOKEN_RC,
  CTK_RC_TOKEN_HIGHEST,
  CTK_RC_TOKEN_ENGINE,
  CTK_RC_TOKEN_MODULE_PATH,
  CTK_RC_TOKEN_IM_MODULE_PATH,
  CTK_RC_TOKEN_IM_MODULE_FILE,
  CTK_RC_TOKEN_STOCK,
  CTK_RC_TOKEN_LTR,
  CTK_RC_TOKEN_RTL,
  CTK_RC_TOKEN_COLOR,
  CTK_RC_TOKEN_UNBIND,
  CTK_RC_TOKEN_LAST
} CtkRcTokenType;


/**
 * CtkPathPriorityType:
 * Priorities for path lookups.
 * See also ctk_binding_set_add_path().
 */
typedef enum
{
  /*< private >*/
  CTK_PATH_PRIO_LOWEST      = 0,
  CTK_PATH_PRIO_CTK         = 4,
  CTK_PATH_PRIO_APPLICATION = 8,
  CTK_PATH_PRIO_THEME       = 10,
  CTK_PATH_PRIO_RC          = 12,
  CTK_PATH_PRIO_HIGHEST     = 15
} CtkPathPriorityType;
#define CTK_PATH_PRIO_MASK 0x0f

/**
 * CtkPathType:
 * Widget path types.
 * See also ctk_binding_set_add_path().
 */
typedef enum
{
  /*< private >*/
  CTK_PATH_WIDGET,
  CTK_PATH_WIDGET_CLASS,
  CTK_PATH_CLASS
} CtkPathType;

CDK_AVAILABLE_IN_ALL
GScanner* ctk_rc_scanner_new    (void);
CDK_AVAILABLE_IN_ALL
guint     ctk_rc_parse_color    (GScanner            *scanner,
                                 CdkColor            *color);
CDK_AVAILABLE_IN_ALL
guint     ctk_rc_parse_color_full (GScanner          *scanner,
                                   CtkRcStyle        *style,
                                   CdkColor          *color);
CDK_AVAILABLE_IN_ALL
guint     ctk_rc_parse_state    (GScanner            *scanner,
                                 CtkStateType        *state);
CDK_AVAILABLE_IN_ALL
guint     ctk_rc_parse_priority (GScanner            *scanner,
                                 CtkPathPriorityType *priority);

/* rc properties
 * (structure forward declared in ctkstyle.h)
 */
/**
 * CtkRcProperty:
 * @type_name: quark-ified type identifier
 * @property_name: quark-ified property identifier like
 *   “CtkScrollbar::spacing”
 * @origin: field similar to one found in #CtkSettingsValue
 * @value:field similar to one found in #CtkSettingsValue
 */
struct _CtkRcProperty
{
  /* quark-ified property identifier like “CtkScrollbar::spacing” */
  GQuark type_name;
  GQuark property_name;

  /* fields similar to CtkSettingsValue */
  gchar *origin;
  GValue value;
};

CDK_AVAILABLE_IN_ALL
void      ctk_binding_set_add_path (CtkBindingSet       *binding_set,
                                    CtkPathType          path_type,
                                    const gchar         *path_pattern,
                                    CtkPathPriorityType  priority);

G_END_DECLS

#endif /* __CTK_RC_H__ */
