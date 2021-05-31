/* GTK - The GIMP Toolkit
 * Copyright (C) 2010 Carlos Garnacho <carlosg@gnome.org>
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

#ifndef __CTK_RENDER_H__
#define __CTK_RENDER_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <cairo.h>
#include <pango/pango.h>
#include <gdk/gdk.h>

#include <gtk/gtkenums.h>
#include <gtk/gtktypes.h>

G_BEGIN_DECLS

GDK_AVAILABLE_IN_ALL
void        ctk_render_check       (GtkStyleContext     *context,
                                    cairo_t             *cr,
                                    gdouble              x,
                                    gdouble              y,
                                    gdouble              width,
                                    gdouble              height);
GDK_AVAILABLE_IN_ALL
void        ctk_render_option      (GtkStyleContext     *context,
                                    cairo_t             *cr,
                                    gdouble              x,
                                    gdouble              y,
                                    gdouble              width,
                                    gdouble              height);
GDK_AVAILABLE_IN_ALL
void        ctk_render_arrow       (GtkStyleContext     *context,
                                    cairo_t             *cr,
                                    gdouble              angle,
                                    gdouble              x,
                                    gdouble              y,
                                    gdouble              size);
GDK_AVAILABLE_IN_ALL
void        ctk_render_background  (GtkStyleContext     *context,
                                    cairo_t             *cr,
                                    gdouble              x,
                                    gdouble              y,
                                    gdouble              width,
                                    gdouble              height);

GDK_AVAILABLE_IN_3_20
void        ctk_render_background_get_clip  (GtkStyleContext     *context,
                                             gdouble              x,
                                             gdouble              y,
                                             gdouble              width,
                                             gdouble              height,
                                             GdkRectangle        *out_clip);

GDK_AVAILABLE_IN_ALL
void        ctk_render_frame       (GtkStyleContext     *context,
                                    cairo_t             *cr,
                                    gdouble              x,
                                    gdouble              y,
                                    gdouble              width,
                                    gdouble              height);
GDK_AVAILABLE_IN_ALL
void        ctk_render_expander    (GtkStyleContext     *context,
                                    cairo_t             *cr,
                                    gdouble              x,
                                    gdouble              y,
                                    gdouble              width,
                                    gdouble              height);
GDK_AVAILABLE_IN_ALL
void        ctk_render_focus       (GtkStyleContext     *context,
                                    cairo_t             *cr,
                                    gdouble              x,
                                    gdouble              y,
                                    gdouble              width,
                                    gdouble              height);
GDK_AVAILABLE_IN_ALL
void        ctk_render_layout      (GtkStyleContext     *context,
                                    cairo_t             *cr,
                                    gdouble              x,
                                    gdouble              y,
                                    PangoLayout         *layout);
GDK_AVAILABLE_IN_ALL
void        ctk_render_line        (GtkStyleContext     *context,
                                    cairo_t             *cr,
                                    gdouble              x0,
                                    gdouble              y0,
                                    gdouble              x1,
                                    gdouble              y1);
GDK_AVAILABLE_IN_ALL
void        ctk_render_slider      (GtkStyleContext     *context,
                                    cairo_t             *cr,
                                    gdouble              x,
                                    gdouble              y,
                                    gdouble              width,
                                    gdouble              height,
                                    GtkOrientation       orientation);
GDK_DEPRECATED_IN_3_24_FOR(ctk_render_frame)
void        ctk_render_frame_gap   (GtkStyleContext     *context,
                                    cairo_t             *cr,
                                    gdouble              x,
                                    gdouble              y,
                                    gdouble              width,
                                    gdouble              height,
                                    GtkPositionType      gap_side,
                                    gdouble              xy0_gap,
                                    gdouble              xy1_gap);
GDK_AVAILABLE_IN_ALL
void        ctk_render_extension   (GtkStyleContext     *context,
                                    cairo_t             *cr,
                                    gdouble              x,
                                    gdouble              y,
                                    gdouble              width,
                                    gdouble              height,
                                    GtkPositionType      gap_side);
GDK_AVAILABLE_IN_ALL
void        ctk_render_handle      (GtkStyleContext     *context,
                                    cairo_t             *cr,
                                    gdouble              x,
                                    gdouble              y,
                                    gdouble              width,
                                    gdouble              height);
GDK_AVAILABLE_IN_ALL
void        ctk_render_activity    (GtkStyleContext     *context,
                                    cairo_t             *cr,
                                    gdouble              x,
                                    gdouble              y,
                                    gdouble              width,
                                    gdouble              height);
GDK_DEPRECATED_IN_3_10_FOR(ctk_icon_theme_load_icon)
GdkPixbuf * ctk_render_icon_pixbuf (GtkStyleContext     *context,
                                    const GtkIconSource *source,
                                    GtkIconSize          size);
GDK_AVAILABLE_IN_3_2
void        ctk_render_icon        (GtkStyleContext     *context,
                                    cairo_t             *cr,
                                    GdkPixbuf           *pixbuf,
                                    gdouble              x,
                                    gdouble              y);
GDK_AVAILABLE_IN_3_10
void        ctk_render_icon_surface (GtkStyleContext    *context,
				     cairo_t            *cr,
				     cairo_surface_t    *surface,
				     gdouble             x,
				     gdouble             y);

G_END_DECLS

#endif /* __CTK_RENDER_H__ */
