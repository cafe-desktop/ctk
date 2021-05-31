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

#ifndef __CTK_THEMING_ENGINE_H__
#define __CTK_THEMING_ENGINE_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <glib-object.h>
#include <cairo.h>

#include <ctk/ctkborder.h>
#include <ctk/ctkenums.h>
#include <ctk/deprecated/ctkstyleproperties.h>
#include <ctk/ctktypes.h>

G_BEGIN_DECLS

#define CTK_TYPE_THEMING_ENGINE         (ctk_theming_engine_get_type ())
#define CTK_THEMING_ENGINE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_THEMING_ENGINE, CtkThemingEngine))
#define CTK_THEMING_ENGINE_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST    ((c), CTK_TYPE_THEMING_ENGINE, CtkThemingEngineClass))
#define CTK_IS_THEMING_ENGINE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_THEMING_ENGINE))
#define CTK_IS_THEMING_ENGINE_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE    ((c), CTK_TYPE_THEMING_ENGINE))
#define CTK_THEMING_ENGINE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS  ((o), CTK_TYPE_THEMING_ENGINE, CtkThemingEngineClass))

typedef struct _CtkThemingEngine CtkThemingEngine;
typedef struct CtkThemingEnginePrivate CtkThemingEnginePrivate;
typedef struct _CtkThemingEngineClass CtkThemingEngineClass;

struct _CtkThemingEngine
{
  GObject parent_object;
  CtkThemingEnginePrivate *priv;
};

/**
 * CtkThemingEngineClass:
 * @parent_class: The parent class.
 * @render_line: Renders a line between two points.
 * @render_background: Renders the background area of a widget region.
 * @render_frame: Renders the frame around a widget area.
 * @render_frame_gap: Renders the frame around a widget area with a gap in it.
 * @render_extension: Renders a extension to a box, usually a notebook tab.
 * @render_check: Renders a checkmark, as in #CtkCheckButton.
 * @render_option: Renders an option, as in #CtkRadioButton.
 * @render_arrow: Renders an arrow pointing to a certain direction.
 * @render_expander: Renders an element what will expose/expand part of
 *                   the UI, as in #CtkExpander.
 * @render_focus: Renders the focus indicator.
 * @render_layout: Renders a #PangoLayout
 * @render_slider: Renders a slider control, as in #CtkScale.
 * @render_handle: Renders a handle to drag UI elements, as in #CtkPaned.
 * @render_activity: Renders an area displaying activity, such as in #CtkSpinner,
 *                   or #CtkProgressBar.
 * @render_icon_pixbuf: Renders an icon as a #GdkPixbuf.
 * @render_icon: Renders an icon given as a #GdkPixbuf.
 * @render_icon_surface: Renders an icon given as a #cairo_surface_t.
 *
 * Base class for theming engines.
 */
struct _CtkThemingEngineClass
{
  GObjectClass parent_class;

  /*< public >*/

  void (* render_line) (CtkThemingEngine *engine,
                        cairo_t          *cr,
                        gdouble           x0,
                        gdouble           y0,
                        gdouble           x1,
                        gdouble           y1);
  void (* render_background) (CtkThemingEngine *engine,
                              cairo_t          *cr,
                              gdouble           x,
                              gdouble           y,
                              gdouble           width,
                              gdouble           height);
  void (* render_frame) (CtkThemingEngine *engine,
                         cairo_t          *cr,
                         gdouble           x,
                         gdouble           y,
                         gdouble           width,
                         gdouble           height);
  void (* render_frame_gap) (CtkThemingEngine *engine,
                             cairo_t          *cr,
                             gdouble           x,
                             gdouble           y,
                             gdouble           width,
                             gdouble           height,
                             CtkPositionType   gap_side,
                             gdouble           xy0_gap,
                             gdouble           xy1_gap);
  void (* render_extension) (CtkThemingEngine *engine,
                             cairo_t          *cr,
                             gdouble           x,
                             gdouble           y,
                             gdouble           width,
                             gdouble           height,
                             CtkPositionType   gap_side);
  void (* render_check) (CtkThemingEngine *engine,
                         cairo_t          *cr,
                         gdouble           x,
                         gdouble           y,
                         gdouble           width,
                         gdouble           height);
  void (* render_option) (CtkThemingEngine *engine,
                          cairo_t          *cr,
                          gdouble           x,
                          gdouble           y,
                          gdouble           width,
                          gdouble           height);
  void (* render_arrow) (CtkThemingEngine *engine,
                         cairo_t          *cr,
                         gdouble           angle,
                         gdouble           x,
                         gdouble           y,
                         gdouble           size);
  void (* render_expander) (CtkThemingEngine *engine,
                            cairo_t          *cr,
                            gdouble           x,
                            gdouble           y,
                            gdouble           width,
                            gdouble           height);
  void (* render_focus) (CtkThemingEngine *engine,
                         cairo_t          *cr,
                         gdouble           x,
                         gdouble           y,
                         gdouble           width,
                         gdouble           height);
  void (* render_layout) (CtkThemingEngine *engine,
                          cairo_t          *cr,
                          gdouble           x,
                          gdouble           y,
                          PangoLayout      *layout);
  void (* render_slider) (CtkThemingEngine *engine,
                          cairo_t          *cr,
                          gdouble           x,
                          gdouble           y,
                          gdouble           width,
                          gdouble           height,
                          CtkOrientation    orientation);
  void (* render_handle)    (CtkThemingEngine *engine,
                             cairo_t          *cr,
                             gdouble           x,
                             gdouble           y,
                             gdouble           width,
                             gdouble           height);
  void (* render_activity) (CtkThemingEngine *engine,
                            cairo_t          *cr,
                            gdouble           x,
                            gdouble           y,
                            gdouble           width,
                            gdouble           height);

  GdkPixbuf * (* render_icon_pixbuf) (CtkThemingEngine    *engine,
                                      const CtkIconSource *source,
                                      CtkIconSize          size);
  void (* render_icon) (CtkThemingEngine *engine,
                        cairo_t          *cr,
			GdkPixbuf        *pixbuf,
                        gdouble           x,
                        gdouble           y);
  void (* render_icon_surface) (CtkThemingEngine *engine,
				cairo_t          *cr,
				cairo_surface_t  *surface,
				gdouble           x,
				gdouble           y);

  /*< private >*/
  gpointer padding[14];
};

GDK_DEPRECATED_IN_3_14
GType ctk_theming_engine_get_type (void) G_GNUC_CONST;

/* function implemented in ctkcsscustomproperty.c */
GDK_DEPRECATED_IN_3_8
void ctk_theming_engine_register_property (const gchar            *name_space,
                                           CtkStylePropertyParser  parse_func,
                                           GParamSpec             *pspec);

GDK_DEPRECATED_IN_3_14
void ctk_theming_engine_get_property (CtkThemingEngine *engine,
                                      const gchar      *property,
                                      CtkStateFlags     state,
                                      GValue           *value);
GDK_DEPRECATED_IN_3_14
void ctk_theming_engine_get_valist   (CtkThemingEngine *engine,
                                      CtkStateFlags     state,
                                      va_list           args);
GDK_DEPRECATED_IN_3_14
void ctk_theming_engine_get          (CtkThemingEngine *engine,
                                      CtkStateFlags     state,
                                      ...) G_GNUC_NULL_TERMINATED;

GDK_DEPRECATED_IN_3_14
void ctk_theming_engine_get_style_property (CtkThemingEngine *engine,
                                            const gchar      *property_name,
                                            GValue           *value);
GDK_DEPRECATED_IN_3_14
void ctk_theming_engine_get_style_valist   (CtkThemingEngine *engine,
                                            va_list           args);
GDK_DEPRECATED_IN_3_14
void ctk_theming_engine_get_style          (CtkThemingEngine *engine,
                                            ...);

GDK_DEPRECATED_IN_3_14
gboolean ctk_theming_engine_lookup_color (CtkThemingEngine *engine,
                                          const gchar      *color_name,
                                          GdkRGBA          *color);

GDK_DEPRECATED_IN_3_14
const CtkWidgetPath * ctk_theming_engine_get_path (CtkThemingEngine *engine);

GDK_DEPRECATED_IN_3_14
gboolean ctk_theming_engine_has_class  (CtkThemingEngine *engine,
                                        const gchar      *style_class);
GDK_DEPRECATED_IN_3_14
gboolean ctk_theming_engine_has_region (CtkThemingEngine *engine,
                                        const gchar      *style_region,
                                        CtkRegionFlags   *flags);

GDK_DEPRECATED_IN_3_14
CtkStateFlags ctk_theming_engine_get_state        (CtkThemingEngine *engine);
GDK_DEPRECATED_IN_3_6
gboolean      ctk_theming_engine_state_is_running (CtkThemingEngine *engine,
                                                   CtkStateType      state,
                                                   gdouble          *progress);

GDK_DEPRECATED_IN_3_8_FOR(ctk_theming_engine_get_state)
CtkTextDirection ctk_theming_engine_get_direction (CtkThemingEngine *engine);

GDK_DEPRECATED_IN_3_14
CtkJunctionSides ctk_theming_engine_get_junction_sides (CtkThemingEngine *engine);

/* Helper functions */
GDK_DEPRECATED_IN_3_14
void ctk_theming_engine_get_color            (CtkThemingEngine *engine,
                                              CtkStateFlags     state,
                                              GdkRGBA          *color);
GDK_DEPRECATED_IN_3_14
void ctk_theming_engine_get_background_color (CtkThemingEngine *engine,
                                              CtkStateFlags     state,
                                              GdkRGBA          *color);
GDK_DEPRECATED_IN_3_14
void ctk_theming_engine_get_border_color     (CtkThemingEngine *engine,
                                              CtkStateFlags     state,
                                              GdkRGBA          *color);

GDK_DEPRECATED_IN_3_14
void ctk_theming_engine_get_border  (CtkThemingEngine *engine,
                                     CtkStateFlags     state,
                                     CtkBorder        *border);
GDK_DEPRECATED_IN_3_14
void ctk_theming_engine_get_padding (CtkThemingEngine *engine,
                                     CtkStateFlags     state,
                                     CtkBorder        *padding);
GDK_DEPRECATED_IN_3_14
void ctk_theming_engine_get_margin  (CtkThemingEngine *engine,
                                     CtkStateFlags     state,
                                     CtkBorder        *margin);

GDK_DEPRECATED_IN_3_8_FOR(ctk_theming_engine_get)
const PangoFontDescription * ctk_theming_engine_get_font (CtkThemingEngine *engine,
                                                          CtkStateFlags     state);

GDK_DEPRECATED_IN_3_14
CtkThemingEngine * ctk_theming_engine_load (const gchar *name);

GDK_DEPRECATED_IN_3_14
GdkScreen * ctk_theming_engine_get_screen (CtkThemingEngine *engine);

G_END_DECLS

#endif /* __CTK_THEMING_ENGINE_H__ */
