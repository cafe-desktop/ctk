/* GtkIconTheme - a loader for icon themes
 * gtk-icon-loader.h Copyright (C) 2002, 2003 Red Hat, Inc.
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

#ifndef __CTK_ICON_THEME_H__
#define __CTK_ICON_THEME_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>
#include <gtk/gtkstylecontext.h>

G_BEGIN_DECLS

#define CTK_TYPE_ICON_INFO              (ctk_icon_info_get_type ())
#define CTK_ICON_INFO(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_ICON_INFO, GtkIconInfo))
#define CTK_ICON_INFO_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_ICON_INFO, GtkIconInfoClass))
#define CTK_IS_ICON_INFO(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_ICON_INFO))
#define CTK_IS_ICON_INFO_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_ICON_INFO))
#define CTK_ICON_INFO_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_ICON_INFO, GtkIconInfoClass))

#define CTK_TYPE_ICON_THEME             (ctk_icon_theme_get_type ())
#define CTK_ICON_THEME(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_ICON_THEME, GtkIconTheme))
#define CTK_ICON_THEME_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_ICON_THEME, GtkIconThemeClass))
#define CTK_IS_ICON_THEME(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_ICON_THEME))
#define CTK_IS_ICON_THEME_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_ICON_THEME))
#define CTK_ICON_THEME_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_ICON_THEME, GtkIconThemeClass))

/**
 * GtkIconInfo:
 *
 * Contains information found when looking up an icon in
 * an icon theme.
 */
typedef struct _GtkIconInfo         GtkIconInfo;
typedef struct _GtkIconInfoClass    GtkIconInfoClass;
typedef struct _GtkIconTheme        GtkIconTheme;
typedef struct _GtkIconThemeClass   GtkIconThemeClass;
typedef struct _GtkIconThemePrivate GtkIconThemePrivate;

/**
 * GtkIconTheme:
 *
 * Acts as a database of information about an icon theme.
 * Normally, you retrieve the icon theme for a particular
 * screen using ctk_icon_theme_get_for_screen() and it
 * will contain information about current icon theme for
 * that screen, but you can also create a new #GtkIconTheme
 * object and set the icon theme name explicitly using
 * ctk_icon_theme_set_custom_theme().
 */
struct _GtkIconTheme
{
  /*< private >*/
  GObject parent_instance;

  GtkIconThemePrivate *priv;
};

/**
 * GtkIconThemeClass:
 * @parent_class: The parent class.
 * @changed: Signal emitted when the current icon theme is switched or
 *    GTK+ detects that a change has occurred in the contents of the
 *    current icon theme.
 */
struct _GtkIconThemeClass
{
  GObjectClass parent_class;

  /*< public >*/

  void (* changed)  (GtkIconTheme *icon_theme);

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

/**
 * GtkIconLookupFlags:
 * @CTK_ICON_LOOKUP_NO_SVG: Never get SVG icons, even if gdk-pixbuf
 *   supports them. Cannot be used together with %CTK_ICON_LOOKUP_FORCE_SVG.
 * @CTK_ICON_LOOKUP_FORCE_SVG: Get SVG icons, even if gdk-pixbuf
 *   doesn’t support them.
 *   Cannot be used together with %CTK_ICON_LOOKUP_NO_SVG.
 * @CTK_ICON_LOOKUP_USE_BUILTIN: When passed to
 *   ctk_icon_theme_lookup_icon() includes builtin icons
 *   as well as files. For a builtin icon, ctk_icon_info_get_filename()
 *   is %NULL and you need to call ctk_icon_info_get_builtin_pixbuf().
 * @CTK_ICON_LOOKUP_GENERIC_FALLBACK: Try to shorten icon name at '-'
 *   characters before looking at inherited themes. This flag is only
 *   supported in functions that take a single icon name. For more general
 *   fallback, see ctk_icon_theme_choose_icon(). Since 2.12.
 * @CTK_ICON_LOOKUP_FORCE_SIZE: Always get the icon scaled to the
 *   requested size. Since 2.14.
 * @CTK_ICON_LOOKUP_FORCE_REGULAR: Try to always load regular icons, even
 *   when symbolic icon names are given. Since 3.14.
 * @CTK_ICON_LOOKUP_FORCE_SYMBOLIC: Try to always load symbolic icons, even
 *   when regular icon names are given. Since 3.14.
 * @CTK_ICON_LOOKUP_DIR_LTR: Try to load a variant of the icon for left-to-right
 *   text direction. Since 3.14.
 * @CTK_ICON_LOOKUP_DIR_RTL: Try to load a variant of the icon for right-to-left
 *   text direction. Since 3.14.
 * 
 * Used to specify options for ctk_icon_theme_lookup_icon()
 */
typedef enum
{
  CTK_ICON_LOOKUP_NO_SVG           = 1 << 0,
  CTK_ICON_LOOKUP_FORCE_SVG        = 1 << 1,
  CTK_ICON_LOOKUP_USE_BUILTIN      = 1 << 2,
  CTK_ICON_LOOKUP_GENERIC_FALLBACK = 1 << 3,
  CTK_ICON_LOOKUP_FORCE_SIZE       = 1 << 4,
  CTK_ICON_LOOKUP_FORCE_REGULAR    = 1 << 5,
  CTK_ICON_LOOKUP_FORCE_SYMBOLIC   = 1 << 6,
  CTK_ICON_LOOKUP_DIR_LTR          = 1 << 7,
  CTK_ICON_LOOKUP_DIR_RTL          = 1 << 8
} GtkIconLookupFlags;

/**
 * CTK_ICON_THEME_ERROR:
 *
 * The #GQuark used for #GtkIconThemeError errors.
 */
#define CTK_ICON_THEME_ERROR ctk_icon_theme_error_quark ()

/**
 * GtkIconThemeError:
 * @CTK_ICON_THEME_NOT_FOUND: The icon specified does not exist in the theme
 * @CTK_ICON_THEME_FAILED: An unspecified error occurred.
 * 
 * Error codes for GtkIconTheme operations.
 **/
typedef enum {
  CTK_ICON_THEME_NOT_FOUND,
  CTK_ICON_THEME_FAILED
} GtkIconThemeError;

GDK_AVAILABLE_IN_ALL
GQuark ctk_icon_theme_error_quark (void);

GDK_AVAILABLE_IN_ALL
GType         ctk_icon_theme_get_type              (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
GtkIconTheme *ctk_icon_theme_new                   (void);
GDK_AVAILABLE_IN_ALL
GtkIconTheme *ctk_icon_theme_get_default           (void);
GDK_AVAILABLE_IN_ALL
GtkIconTheme *ctk_icon_theme_get_for_screen        (GdkScreen                   *screen);
GDK_AVAILABLE_IN_ALL
void          ctk_icon_theme_set_screen            (GtkIconTheme                *icon_theme,
						    GdkScreen                   *screen);

GDK_AVAILABLE_IN_ALL
void          ctk_icon_theme_set_search_path       (GtkIconTheme                *icon_theme,
						    const gchar                 *path[],
						    gint                         n_elements);
GDK_AVAILABLE_IN_ALL
void          ctk_icon_theme_get_search_path       (GtkIconTheme                *icon_theme,
						    gchar                      **path[],
						    gint                        *n_elements);
GDK_AVAILABLE_IN_ALL
void          ctk_icon_theme_append_search_path    (GtkIconTheme                *icon_theme,
						    const gchar                 *path);
GDK_AVAILABLE_IN_ALL
void          ctk_icon_theme_prepend_search_path   (GtkIconTheme                *icon_theme,
						    const gchar                 *path);

GDK_AVAILABLE_IN_3_14
void          ctk_icon_theme_add_resource_path     (GtkIconTheme                *icon_theme,
                                                    const gchar                 *path);

GDK_AVAILABLE_IN_ALL
void          ctk_icon_theme_set_custom_theme      (GtkIconTheme                *icon_theme,
						    const gchar                 *theme_name);

GDK_AVAILABLE_IN_ALL
gboolean      ctk_icon_theme_has_icon              (GtkIconTheme                *icon_theme,
						    const gchar                 *icon_name);
GDK_AVAILABLE_IN_ALL
gint         *ctk_icon_theme_get_icon_sizes        (GtkIconTheme                *icon_theme,
						    const gchar                 *icon_name);
GDK_AVAILABLE_IN_ALL
GtkIconInfo * ctk_icon_theme_lookup_icon           (GtkIconTheme                *icon_theme,
						    const gchar                 *icon_name,
						    gint                         size,
						    GtkIconLookupFlags           flags);
GDK_AVAILABLE_IN_3_10
GtkIconInfo * ctk_icon_theme_lookup_icon_for_scale (GtkIconTheme                *icon_theme,
                                                    const gchar                 *icon_name,
                                                    gint                         size,
                                                    gint                         scale,
                                                    GtkIconLookupFlags           flags);

GDK_AVAILABLE_IN_ALL
GtkIconInfo * ctk_icon_theme_choose_icon           (GtkIconTheme                *icon_theme,
						    const gchar                 *icon_names[],
						    gint                         size,
						    GtkIconLookupFlags           flags);
GDK_AVAILABLE_IN_3_10
GtkIconInfo * ctk_icon_theme_choose_icon_for_scale (GtkIconTheme                *icon_theme,
						    const gchar                 *icon_names[],
						    gint                         size,
                                                    gint                         scale,
						    GtkIconLookupFlags           flags);
GDK_AVAILABLE_IN_ALL
GdkPixbuf *   ctk_icon_theme_load_icon             (GtkIconTheme                *icon_theme,
						    const gchar                 *icon_name,
						    gint                         size,
						    GtkIconLookupFlags           flags,
						    GError                     **error);
GDK_AVAILABLE_IN_3_10
GdkPixbuf *   ctk_icon_theme_load_icon_for_scale   (GtkIconTheme                *icon_theme,
                                                    const gchar                 *icon_name,
                                                    gint                         size,
                                                    gint                         scale,
                                                    GtkIconLookupFlags           flags,
                                                    GError                     **error);
GDK_AVAILABLE_IN_3_10
cairo_surface_t * ctk_icon_theme_load_surface      (GtkIconTheme        *icon_theme,
						    const gchar         *icon_name,
						    gint                 size,
						    gint                 scale,
						    GdkWindow           *for_window,
						    GtkIconLookupFlags   flags,
						    GError             **error);

GDK_AVAILABLE_IN_ALL
GtkIconInfo * ctk_icon_theme_lookup_by_gicon       (GtkIconTheme                *icon_theme,
                                                    GIcon                       *icon,
                                                    gint                         size,
                                                    GtkIconLookupFlags           flags);
GDK_AVAILABLE_IN_3_10
GtkIconInfo * ctk_icon_theme_lookup_by_gicon_for_scale (GtkIconTheme             *icon_theme,
                                                        GIcon                    *icon,
                                                        gint                      size,
                                                        gint                      scale,
                                                        GtkIconLookupFlags        flags);


GDK_AVAILABLE_IN_ALL
GList *       ctk_icon_theme_list_icons            (GtkIconTheme                *icon_theme,
						    const gchar                 *context);
GDK_AVAILABLE_IN_ALL
GList *       ctk_icon_theme_list_contexts         (GtkIconTheme                *icon_theme);
GDK_AVAILABLE_IN_ALL
char *        ctk_icon_theme_get_example_icon_name (GtkIconTheme                *icon_theme);

GDK_AVAILABLE_IN_ALL
gboolean      ctk_icon_theme_rescan_if_needed      (GtkIconTheme                *icon_theme);

GDK_DEPRECATED_IN_3_14_FOR(ctk_icon_theme_add_resource_path)
void          ctk_icon_theme_add_builtin_icon      (const gchar *icon_name,
					            gint         size,
					            GdkPixbuf   *pixbuf);

GDK_AVAILABLE_IN_ALL
GType                 ctk_icon_info_get_type           (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_3_8_FOR(g_object_ref)
GtkIconInfo *         ctk_icon_info_copy               (GtkIconInfo  *icon_info);
GDK_DEPRECATED_IN_3_8_FOR(g_object_unref)
void                  ctk_icon_info_free               (GtkIconInfo  *icon_info);

GDK_AVAILABLE_IN_ALL
GtkIconInfo *         ctk_icon_info_new_for_pixbuf     (GtkIconTheme  *icon_theme,
                                                        GdkPixbuf     *pixbuf);

GDK_AVAILABLE_IN_ALL
gint                  ctk_icon_info_get_base_size      (GtkIconInfo   *icon_info);
GDK_AVAILABLE_IN_3_10
gint                  ctk_icon_info_get_base_scale     (GtkIconInfo   *icon_info);
GDK_AVAILABLE_IN_ALL
const gchar *         ctk_icon_info_get_filename       (GtkIconInfo   *icon_info);
GDK_DEPRECATED_IN_3_14
GdkPixbuf *           ctk_icon_info_get_builtin_pixbuf (GtkIconInfo   *icon_info);
GDK_AVAILABLE_IN_3_12
gboolean              ctk_icon_info_is_symbolic        (GtkIconInfo   *icon_info);
GDK_AVAILABLE_IN_ALL
GdkPixbuf *           ctk_icon_info_load_icon          (GtkIconInfo   *icon_info,
							GError       **error);
GDK_AVAILABLE_IN_3_10
cairo_surface_t *     ctk_icon_info_load_surface       (GtkIconInfo   *icon_info,
							GdkWindow     *for_window,
							GError       **error);
GDK_AVAILABLE_IN_3_8
void                  ctk_icon_info_load_icon_async   (GtkIconInfo          *icon_info,
						       GCancellable         *cancellable,
						       GAsyncReadyCallback   callback,
						       gpointer              user_data);
GDK_AVAILABLE_IN_3_8
GdkPixbuf *           ctk_icon_info_load_icon_finish  (GtkIconInfo          *icon_info,
						       GAsyncResult         *res,
						       GError              **error);
GDK_AVAILABLE_IN_ALL
GdkPixbuf *           ctk_icon_info_load_symbolic      (GtkIconInfo   *icon_info,
                                                        const GdkRGBA *fg,
                                                        const GdkRGBA *success_color,
                                                        const GdkRGBA *warning_color,
                                                        const GdkRGBA *error_color,
                                                        gboolean      *was_symbolic,
                                                        GError       **error);
GDK_AVAILABLE_IN_3_8
void                  ctk_icon_info_load_symbolic_async (GtkIconInfo   *icon_info,
							 const GdkRGBA *fg,
							 const GdkRGBA *success_color,
							 const GdkRGBA *warning_color,
							 const GdkRGBA *error_color,
							 GCancellable         *cancellable,
							 GAsyncReadyCallback   callback,
							 gpointer              user_data);
GDK_AVAILABLE_IN_3_8
GdkPixbuf *           ctk_icon_info_load_symbolic_finish (GtkIconInfo   *icon_info,
							  GAsyncResult         *res,
							  gboolean      *was_symbolic,
							  GError       **error);
GDK_AVAILABLE_IN_ALL
GdkPixbuf *           ctk_icon_info_load_symbolic_for_context (GtkIconInfo      *icon_info,
                                                               GtkStyleContext  *context,
                                                               gboolean         *was_symbolic,
                                                               GError          **error);
GDK_AVAILABLE_IN_3_8
void                  ctk_icon_info_load_symbolic_for_context_async (GtkIconInfo      *icon_info,
								     GtkStyleContext  *context,
								     GCancellable     *cancellable,
								     GAsyncReadyCallback callback,
								     gpointer          user_data);
GDK_AVAILABLE_IN_3_8
GdkPixbuf *           ctk_icon_info_load_symbolic_for_context_finish (GtkIconInfo      *icon_info,
								      GAsyncResult     *res,
								      gboolean         *was_symbolic,
								      GError          **error);
GDK_DEPRECATED_IN_3_0_FOR(ctk_icon_info_load_symbol_for_context)
GdkPixbuf *           ctk_icon_info_load_symbolic_for_style  (GtkIconInfo   *icon_info,
                                                              GtkStyle      *style,
                                                              GtkStateType   state,
                                                              gboolean      *was_symbolic,
                                                              GError       **error);
GDK_DEPRECATED_IN_3_14
void                  ctk_icon_info_set_raw_coordinates (GtkIconInfo  *icon_info,
							 gboolean      raw_coordinates);

GDK_DEPRECATED_IN_3_14
gboolean              ctk_icon_info_get_embedded_rect (GtkIconInfo    *icon_info,
						       GdkRectangle   *rectangle);
GDK_DEPRECATED_IN_3_14
gboolean              ctk_icon_info_get_attach_points (GtkIconInfo    *icon_info,
						       GdkPoint      **points,
						       gint           *n_points);
GDK_DEPRECATED_IN_3_14
const gchar *         ctk_icon_info_get_display_name  (GtkIconInfo    *icon_info);

G_END_DECLS

#endif /* __CTK_ICON_THEME_H__ */