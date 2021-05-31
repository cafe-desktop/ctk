/* CTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat, Inc.
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

#ifndef __CTK_COLOR_SELECTION_H__
#define __CTK_COLOR_SELECTION_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkdialog.h>
#include <ctk/ctkbox.h>

G_BEGIN_DECLS

#define CTK_TYPE_COLOR_SELECTION			(ctk_color_selection_get_type ())
#define CTK_COLOR_SELECTION(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_COLOR_SELECTION, CtkColorSelection))
#define CTK_COLOR_SELECTION_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_COLOR_SELECTION, CtkColorSelectionClass))
#define CTK_IS_COLOR_SELECTION(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_COLOR_SELECTION))
#define CTK_IS_COLOR_SELECTION_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_COLOR_SELECTION))
#define CTK_COLOR_SELECTION_GET_CLASS(obj)              (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_COLOR_SELECTION, CtkColorSelectionClass))


typedef struct _CtkColorSelection       CtkColorSelection;
typedef struct _CtkColorSelectionPrivate  CtkColorSelectionPrivate;
typedef struct _CtkColorSelectionClass  CtkColorSelectionClass;

/**
 * CtkColorSelectionChangePaletteFunc:
 * @colors: (array length=n_colors): Array of colors
 * @n_colors: Number of colors in the array
 *
 * Deprecated: 3.4
 */
typedef void (* CtkColorSelectionChangePaletteFunc) (const GdkColor    *colors,
                                                     gint               n_colors);

/**
 * CtkColorSelectionChangePaletteWithScreenFunc:
 * @screen:
 * @colors: (array length=n_colors): Array of colors
 * @n_colors: Number of colors in the array
 *
 * Since: 2.2
 * Deprecated: 3.4
 */
typedef void (* CtkColorSelectionChangePaletteWithScreenFunc) (GdkScreen         *screen,
							       const GdkColor    *colors,
							       gint               n_colors);

struct _CtkColorSelection
{
  CtkBox parent_instance;

  /*< private >*/
  CtkColorSelectionPrivate *private_data;
};

/**
 * CtkColorSelectionClass:
 * @parent_class: The parent class.
 * @color_changed:
 */
struct _CtkColorSelectionClass
{
  CtkBoxClass parent_class;

  void (*color_changed)	(CtkColorSelection *color_selection);

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


/* ColorSelection */

GDK_DEPRECATED_IN_3_4
GType      ctk_color_selection_get_type                (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_3_4_FOR(ctk_color_chooser_widget_new)
CtkWidget *ctk_color_selection_new                     (void);
GDK_DEPRECATED_IN_3_4_FOR(ctk_color_chooser_get_use_alpha)
gboolean   ctk_color_selection_get_has_opacity_control (CtkColorSelection *colorsel);
GDK_DEPRECATED_IN_3_4_FOR(ctk_color_chooser_set_use_alpha)
void       ctk_color_selection_set_has_opacity_control (CtkColorSelection *colorsel,
							gboolean           has_opacity);
GDK_DEPRECATED_IN_3_4
gboolean   ctk_color_selection_get_has_palette         (CtkColorSelection *colorsel);
GDK_DEPRECATED_IN_3_4
void       ctk_color_selection_set_has_palette         (CtkColorSelection *colorsel,
							gboolean           has_palette);


GDK_DEPRECATED_IN_3_4_FOR(ctk_color_chooser_set_rgba)
void     ctk_color_selection_set_current_alpha   (CtkColorSelection *colorsel,
						  guint16            alpha);
GDK_DEPRECATED_IN_3_4_FOR(ctk_color_chooser_get_rgba)
guint16  ctk_color_selection_get_current_alpha   (CtkColorSelection *colorsel);
GDK_DEPRECATED_IN_3_4
void     ctk_color_selection_set_previous_alpha  (CtkColorSelection *colorsel,
						  guint16            alpha);
GDK_DEPRECATED_IN_3_4
guint16  ctk_color_selection_get_previous_alpha  (CtkColorSelection *colorsel);

GDK_DEPRECATED_IN_3_4_FOR(ctk_color_chooser_set_rgba)
void     ctk_color_selection_set_current_rgba    (CtkColorSelection *colorsel,
                                                  const GdkRGBA     *rgba);
GDK_DEPRECATED_IN_3_4_FOR(ctk_color_chooser_get_rgba)
void     ctk_color_selection_get_current_rgba    (CtkColorSelection *colorsel,
                                                  GdkRGBA           *rgba);
GDK_DEPRECATED_IN_3_4
void     ctk_color_selection_set_previous_rgba   (CtkColorSelection *colorsel,
                                                  const GdkRGBA     *rgba);
GDK_DEPRECATED_IN_3_4
void     ctk_color_selection_get_previous_rgba   (CtkColorSelection *colorsel,
                                                  GdkRGBA           *rgba);

GDK_DEPRECATED_IN_3_4
gboolean ctk_color_selection_is_adjusting        (CtkColorSelection *colorsel);

GDK_DEPRECATED_IN_3_4
gboolean ctk_color_selection_palette_from_string (const gchar       *str,
                                                  GdkColor         **colors,
                                                  gint              *n_colors);
GDK_DEPRECATED_IN_3_4
gchar*   ctk_color_selection_palette_to_string   (const GdkColor    *colors,
                                                  gint               n_colors);

GDK_DEPRECATED_IN_3_4
CtkColorSelectionChangePaletteWithScreenFunc ctk_color_selection_set_change_palette_with_screen_hook (CtkColorSelectionChangePaletteWithScreenFunc func);

GDK_DEPRECATED_IN_3_4_FOR(ctk_color_chooser_set_rgba)
void     ctk_color_selection_set_current_color   (CtkColorSelection *colorsel,
                                                  const GdkColor    *color);
GDK_DEPRECATED_IN_3_4_FOR(ctk_color_chooser_get_rgba)
void     ctk_color_selection_get_current_color   (CtkColorSelection *colorsel,
                                                  GdkColor          *color);
GDK_DEPRECATED_IN_3_4
void     ctk_color_selection_set_previous_color  (CtkColorSelection *colorsel,
                                                  const GdkColor    *color);
GDK_DEPRECATED_IN_3_4
void     ctk_color_selection_get_previous_color  (CtkColorSelection *colorsel,
                                                  GdkColor          *color);


G_END_DECLS

#endif /* __CTK_COLOR_SELECTION_H__ */
