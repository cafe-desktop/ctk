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

#ifndef __CTK_STYLE_H__
#define __CTK_STYLE_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <cdk/cdk.h>
#include <ctk/ctkenums.h>
#include <ctk/ctktypes.h>


G_BEGIN_DECLS

#define CTK_TYPE_STYLE              (ctk_style_get_type ())
#define CTK_STYLE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), CTK_TYPE_STYLE, CtkStyle))
#define CTK_STYLE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_STYLE, CtkStyleClass))
#define CTK_IS_STYLE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), CTK_TYPE_STYLE))
#define CTK_IS_STYLE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_STYLE))
#define CTK_STYLE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_STYLE, CtkStyleClass))

/* Some forward declarations needed to rationalize the header
 * files.
 */
typedef struct _CtkStyleClass  CtkStyleClass;
typedef struct _CtkThemeEngine CtkThemeEngine;
typedef struct _CtkRcProperty  CtkRcProperty;

/**
 * CtkExpanderStyle:
 * @CTK_EXPANDER_COLLAPSED: The style used for a collapsed subtree.
 * @CTK_EXPANDER_SEMI_COLLAPSED: Intermediate style used during animation.
 * @CTK_EXPANDER_SEMI_EXPANDED: Intermediate style used during animation.
 * @CTK_EXPANDER_EXPANDED: The style used for an expanded subtree.
 *
 * Used to specify the style of the expanders drawn by a #CtkTreeView.
 */
typedef enum
{
  CTK_EXPANDER_COLLAPSED,
  CTK_EXPANDER_SEMI_COLLAPSED,
  CTK_EXPANDER_SEMI_EXPANDED,
  CTK_EXPANDER_EXPANDED
} CtkExpanderStyle;

/**
 * CTK_STYLE_ATTACHED:
 * @style: a #CtkStyle.
 *
 * Returns: whether the style is attached to a window.
 */
#define CTK_STYLE_ATTACHED(style)       (CTK_STYLE (style)->attach_count > 0)

/**
 * CtkStyle:
 * @fg: Set of foreground #CdkColor
 * @bg: Set of background #CdkColor
 * @light: Set of light #CdkColor
 * @dark: Set of dark #CdkColor
 * @mid: Set of mid #CdkColor
 * @text: Set of text #CdkColor
 * @base: Set of base #CdkColor
 * @text_aa: Color halfway between text/base
 * @black: #CdkColor to use for black
 * @white: #CdkColor to use for white
 * @font_desc: #PangoFontDescription
 * @xthickness: Thickness in X direction
 * @ythickness: Thickness in Y direction
 * @background: Set of background #cairo_pattern_t
 */
struct _CtkStyle
{
  /*< private >*/
  GObject parent_instance;

  /*< public >*/

  CdkColor fg[5];
  CdkColor bg[5];
  CdkColor light[5];
  CdkColor dark[5];
  CdkColor mid[5];
  CdkColor text[5];
  CdkColor base[5];
  CdkColor text_aa[5];          /* Halfway between text/base */

  CdkColor black;
  CdkColor white;
  PangoFontDescription *font_desc;

  gint xthickness;
  gint ythickness;

  cairo_pattern_t *background[5];

  /*< private >*/

  gint attach_count;

  CdkVisual *visual;
  PangoFontDescription *private_font_desc; /* Font description for style->private_font or %NULL */

  /* the RcStyle from which this style was created */
  CtkRcStyle     *rc_style;

  GSList         *styles;         /* of type CtkStyle* */
  GArray         *property_cache;
  GSList         *icon_factories; /* of type CtkIconFactory* */
};

/**
 * CtkStyleClass:
 * @parent_class: The parent class.
 * @realize: 
 * @unrealize: 
 * @copy: 
 * @clone: 
 * @init_from_rc: 
 * @set_background: 
 * @render_icon: 
 * @draw_hline: 
 * @draw_vline: 
 * @draw_shadow: 
 * @draw_arrow: 
 * @draw_diamond: 
 * @draw_box: 
 * @draw_flat_box: 
 * @draw_check: 
 * @draw_option: 
 * @draw_tab: 
 * @draw_shadow_gap: 
 * @draw_box_gap: 
 * @draw_extension: 
 * @draw_focus: 
 * @draw_slider: 
 * @draw_handle: 
 * @draw_expander: 
 * @draw_layout: 
 * @draw_resize_grip: 
 * @draw_spinner: 
 */
struct _CtkStyleClass
{
  GObjectClass parent_class;

  /*< public >*/

  /* Initialize for a particular visual. style->visual
   * will have been set at this point. Will typically chain
   * to parent.
   */
  void (*realize)               (CtkStyle               *style);

  /* Clean up for a particular visual. Will typically chain
   * to parent.
   */
  void (*unrealize)             (CtkStyle               *style);

  /* Make style an exact duplicate of src.
   */
  void (*copy)                  (CtkStyle               *style,
                                 CtkStyle               *src);

  /* Create an empty style of the same type as this style.
   * The default implementation, which does
   * g_object_new (G_OBJECT_TYPE (style), NULL);
   * should work in most cases.
   */
  CtkStyle *(*clone)             (CtkStyle               *style);

  /* Initialize the CtkStyle with the values in the CtkRcStyle.
   * should chain to the parent implementation.
   */
  void     (*init_from_rc)      (CtkStyle               *style,
                                 CtkRcStyle             *rc_style);

  void (*set_background)        (CtkStyle               *style,
                                 CdkWindow              *window,
                                 CtkStateType            state_type);


  GdkPixbuf * (* render_icon)   (CtkStyle               *style,
                                 const CtkIconSource    *source,
                                 CtkTextDirection        direction,
                                 CtkStateType            state,
                                 CtkIconSize             size,
                                 CtkWidget              *widget,
                                 const gchar            *detail);

  /* Drawing functions
   */

  void (*draw_hline)            (CtkStyle               *style,
                                 cairo_t                *cr,
                                 CtkStateType            state_type,
                                 CtkWidget              *widget,
                                 const gchar            *detail,
                                 gint                    x1,
                                 gint                    x2,
                                 gint                    y);
  void (*draw_vline)            (CtkStyle               *style,
                                 cairo_t                *cr,
                                 CtkStateType            state_type,
                                 CtkWidget              *widget,
                                 const gchar            *detail,
                                 gint                    y1_,
                                 gint                    y2_,
                                 gint                    x);
  void (*draw_shadow)           (CtkStyle               *style,
                                 cairo_t                *cr,
                                 CtkStateType            state_type,
                                 CtkShadowType           shadow_type,
                                 CtkWidget              *widget,
                                 const gchar            *detail,
                                 gint                    x,
                                 gint                    y,
                                 gint                    width,
                                 gint                    height);
  void (*draw_arrow)            (CtkStyle               *style,
                                 cairo_t                *cr,
                                 CtkStateType            state_type,
                                 CtkShadowType           shadow_type,
                                 CtkWidget              *widget,
                                 const gchar            *detail,
                                 CtkArrowType            arrow_type,
                                 gboolean                fill,
                                 gint                    x,
                                 gint                    y,
                                 gint                    width,
                                 gint                    height);
  void (*draw_diamond)          (CtkStyle               *style,
                                 cairo_t                *cr,
                                 CtkStateType            state_type,
                                 CtkShadowType           shadow_type,
                                 CtkWidget              *widget,
                                 const gchar            *detail,
                                 gint                    x,
                                 gint                    y,
                                 gint                    width,
                                 gint                    height);
  void (*draw_box)              (CtkStyle               *style,
                                 cairo_t                *cr,
                                 CtkStateType            state_type,
                                 CtkShadowType           shadow_type,
                                 CtkWidget              *widget,
                                 const gchar            *detail,
                                 gint                    x,
                                 gint                    y,
                                 gint                    width,
                                 gint                    height);
  void (*draw_flat_box)         (CtkStyle               *style,
                                 cairo_t                *cr,
                                 CtkStateType            state_type,
                                 CtkShadowType           shadow_type,
                                 CtkWidget              *widget,
                                 const gchar            *detail,
                                 gint                    x,
                                 gint                    y,
                                 gint                    width,
                                 gint                    height);
  void (*draw_check)            (CtkStyle               *style,
                                 cairo_t                *cr,
                                 CtkStateType            state_type,
                                 CtkShadowType           shadow_type,
                                 CtkWidget              *widget,
                                 const gchar            *detail,
                                 gint                    x,
                                 gint                    y,
                                 gint                    width,
                                 gint                    height);
  void (*draw_option)           (CtkStyle               *style,
                                 cairo_t                *cr,
                                 CtkStateType            state_type,
                                 CtkShadowType           shadow_type,
                                 CtkWidget              *widget,
                                 const gchar            *detail,
                                 gint                    x,
                                 gint                    y,
                                 gint                    width,
                                 gint                    height);
  void (*draw_tab)              (CtkStyle               *style,
                                 cairo_t                *cr,
                                 CtkStateType            state_type,
                                 CtkShadowType           shadow_type,
                                 CtkWidget              *widget,
                                 const gchar            *detail,
                                 gint                    x,
                                 gint                    y,
                                 gint                    width,
                                 gint                    height);
  void (*draw_shadow_gap)       (CtkStyle               *style,
                                 cairo_t                *cr,
                                 CtkStateType            state_type,
                                 CtkShadowType           shadow_type,
                                 CtkWidget              *widget,
                                 const gchar            *detail,
                                 gint                    x,
                                 gint                    y,
                                 gint                    width,
                                 gint                    height,
                                 CtkPositionType         gap_side,
                                 gint                    gap_x,
                                 gint                    gap_width);
  void (*draw_box_gap)          (CtkStyle               *style,
                                 cairo_t                *cr,
                                 CtkStateType            state_type,
                                 CtkShadowType           shadow_type,
                                 CtkWidget              *widget,
                                 const gchar            *detail,
                                 gint                    x,
                                 gint                    y,
                                 gint                    width,
                                 gint                    height,
                                 CtkPositionType         gap_side,
                                 gint                    gap_x,
                                 gint                    gap_width);
  void (*draw_extension)        (CtkStyle               *style,
                                 cairo_t                *cr,
                                 CtkStateType            state_type,
                                 CtkShadowType           shadow_type,
                                 CtkWidget              *widget,
                                 const gchar            *detail,
                                 gint                    x,
                                 gint                    y,
                                 gint                    width,
                                 gint                    height,
                                 CtkPositionType         gap_side);
  void (*draw_focus)            (CtkStyle               *style,
                                 cairo_t                *cr,
                                 CtkStateType            state_type,
                                 CtkWidget              *widget,
                                 const gchar            *detail,
                                 gint                    x,
                                 gint                    y,
                                 gint                    width,
                                 gint                    height);
  void (*draw_slider)           (CtkStyle               *style,
                                 cairo_t                *cr,
                                 CtkStateType            state_type,
                                 CtkShadowType           shadow_type,
                                 CtkWidget              *widget,
                                 const gchar            *detail,
                                 gint                    x,
                                 gint                    y,
                                 gint                    width,
                                 gint                    height,
                                 CtkOrientation          orientation);
  void (*draw_handle)           (CtkStyle               *style,
                                 cairo_t                *cr,
                                 CtkStateType            state_type,
                                 CtkShadowType           shadow_type,
                                 CtkWidget              *widget,
                                 const gchar            *detail,
                                 gint                    x,
                                 gint                    y,
                                 gint                    width,
                                 gint                    height,
                                 CtkOrientation          orientation);

  void (*draw_expander)         (CtkStyle               *style,
                                 cairo_t                *cr,
                                 CtkStateType            state_type,
                                 CtkWidget              *widget,
                                 const gchar            *detail,
                                 gint                    x,
                                 gint                    y,
                                 CtkExpanderStyle        expander_style);
  void (*draw_layout)           (CtkStyle               *style,
                                 cairo_t                *cr,
                                 CtkStateType            state_type,
                                 gboolean                use_text,
                                 CtkWidget              *widget,
                                 const gchar            *detail,
                                 gint                    x,
                                 gint                    y,
                                 PangoLayout            *layout);
  void (*draw_resize_grip)      (CtkStyle               *style,
                                 cairo_t                *cr,
                                 CtkStateType            state_type,
                                 CtkWidget              *widget,
                                 const gchar            *detail,
                                 CdkWindowEdge           edge,
                                 gint                    x,
                                 gint                    y,
                                 gint                    width,
                                 gint                    height);
  void (*draw_spinner)          (CtkStyle               *style,
                                 cairo_t                *cr,
                                 CtkStateType            state_type,
                                 CtkWidget              *widget,
                                 const gchar            *detail,
                                 guint                   step,
                                 gint                    x,
                                 gint                    y,
                                 gint                    width,
                                 gint                    height);

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1)  (void);
  void (*_ctk_reserved2)  (void);
  void (*_ctk_reserved3)  (void);
  void (*_ctk_reserved4)  (void);
  void (*_ctk_reserved5)  (void);
  void (*_ctk_reserved6)  (void);
  void (*_ctk_reserved7)  (void);
  void (*_ctk_reserved8)  (void);
  void (*_ctk_reserved9)  (void);
  void (*_ctk_reserved10) (void);
  void (*_ctk_reserved11) (void);
};

CDK_DEPRECATED_IN_3_0
GType     ctk_style_get_type                 (void) G_GNUC_CONST;
CDK_DEPRECATED_IN_3_0_FOR(CtkStyleContext)
CtkStyle* ctk_style_new                      (void);
CDK_DEPRECATED_IN_3_0_FOR(CtkStyleContext)
CtkStyle* ctk_style_copy                     (CtkStyle     *style);
CDK_DEPRECATED_IN_3_0_FOR(CtkStyleContext)
CtkStyle* ctk_style_attach                   (CtkStyle     *style,
                                              CdkWindow    *window);
CDK_DEPRECATED_IN_3_0_FOR(CtkStyleContext)
void      ctk_style_detach                   (CtkStyle     *style);
CDK_DEPRECATED_IN_3_0_FOR(CtkStyleContext)
void      ctk_style_set_background           (CtkStyle     *style,
                                              CdkWindow    *window,
                                              CtkStateType  state_type);
CDK_DEPRECATED_IN_3_0_FOR(CtkStyleContext and ctk_render_background)
void      ctk_style_apply_default_background (CtkStyle     *style,
                                              cairo_t      *cr,
                                              CdkWindow    *window,
                                              CtkStateType  state_type,
                                              gint          x,
                                              gint          y,
                                              gint          width,
                                              gint          height);

CDK_DEPRECATED_IN_3_0_FOR(CtkStyleContext and a style class)
CtkIconSet* ctk_style_lookup_icon_set        (CtkStyle     *style,
                                              const gchar  *stock_id);
CDK_DEPRECATED_IN_3_0_FOR(CtkStyleContext and a style class)
gboolean    ctk_style_lookup_color           (CtkStyle     *style,
                                              const gchar  *color_name,
                                              CdkColor     *color);

CDK_DEPRECATED_IN_3_0_FOR(CtkStyleContext and ctk_render_icon)
GdkPixbuf*  ctk_style_render_icon     (CtkStyle            *style,
                                       const CtkIconSource *source,
                                       CtkTextDirection     direction,
                                       CtkStateType         state,
                                       CtkIconSize          size,
                                       CtkWidget           *widget,
                                       const gchar         *detail);

CDK_DEPRECATED_IN_3_0_FOR(CtkStyleContext and ctk_render_line)
void ctk_paint_hline             (CtkStyle           *style,
                                  cairo_t            *cr,
                                  CtkStateType        state_type,
                                  CtkWidget          *widget,
                                  const gchar        *detail,
                                  gint                x1,
                                  gint                x2,
                                  gint                y);
CDK_DEPRECATED_IN_3_0_FOR(CtkStyleContext and ctk_render_line)
void ctk_paint_vline             (CtkStyle           *style,
                                  cairo_t            *cr,
                                  CtkStateType        state_type,
                                  CtkWidget          *widget,
                                  const gchar        *detail,
                                  gint                y1_,
                                  gint                y2_,
                                  gint                x);
CDK_DEPRECATED_IN_3_0_FOR(CtkStyleContext and ctk_render_background)
void ctk_paint_shadow            (CtkStyle           *style,
                                  cairo_t            *cr,
                                  CtkStateType        state_type,
                                  CtkShadowType       shadow_type,
                                  CtkWidget          *widget,
                                  const gchar        *detail,
                                  gint                x,
                                  gint                y,
                                  gint                width,
                                  gint                height);
CDK_DEPRECATED_IN_3_0_FOR(CtkStyleContext and ctk_render_arrow)
void ctk_paint_arrow             (CtkStyle           *style,
                                  cairo_t            *cr,
                                  CtkStateType        state_type,
                                  CtkShadowType       shadow_type,
                                  CtkWidget          *widget,
                                  const gchar        *detail,
                                  CtkArrowType        arrow_type,
                                  gboolean            fill,
                                  gint                x,
                                  gint                y,
                                  gint                width,
                                  gint                height);
CDK_DEPRECATED_IN_3_0_FOR(CtkStyleContext and ctk_render_icon)
void ctk_paint_diamond           (CtkStyle           *style,
                                  cairo_t            *cr,
                                  CtkStateType        state_type,
                                  CtkShadowType       shadow_type,
                                  CtkWidget          *widget,
                                  const gchar        *detail,
                                  gint                x,
                                  gint                y,
                                  gint                width,
                                  gint                height);
CDK_DEPRECATED_IN_3_0_FOR(CtkStyleContext and ctk_render_frame)
void ctk_paint_box               (CtkStyle           *style,
                                  cairo_t            *cr,
                                  CtkStateType        state_type,
                                  CtkShadowType       shadow_type,
                                  CtkWidget          *widget,
                                  const gchar        *detail,
                                  gint                x,
                                  gint                y,
                                  gint                width,
                                  gint                height);
CDK_DEPRECATED_IN_3_0_FOR(CtkStyleContext and ctk_render_background)
void ctk_paint_flat_box          (CtkStyle           *style,
                                  cairo_t            *cr,
                                  CtkStateType        state_type,
                                  CtkShadowType       shadow_type,
                                  CtkWidget          *widget,
                                  const gchar        *detail,
                                  gint                x,
                                  gint                y,
                                  gint                width,
                                  gint                height);
CDK_DEPRECATED_IN_3_0_FOR(CtkStyleContext and ctk_render_check)
void ctk_paint_check             (CtkStyle           *style,
                                  cairo_t            *cr,
                                  CtkStateType        state_type,
                                  CtkShadowType       shadow_type,
                                  CtkWidget          *widget,
                                  const gchar        *detail,
                                  gint                x,
                                  gint                y,
                                  gint                width,
                                  gint                height);
CDK_DEPRECATED_IN_3_0_FOR(CtkStyleContext and ctk_render_option)
void ctk_paint_option            (CtkStyle           *style,
                                  cairo_t            *cr,
                                  CtkStateType        state_type,
                                  CtkShadowType       shadow_type,
                                  CtkWidget          *widget,
                                  const gchar        *detail,
                                  gint                x,
                                  gint                y,
                                  gint                width,
                                  gint                height);
CDK_DEPRECATED_IN_3_0_FOR(CtkStyleContext and ctk_render_background)
void ctk_paint_tab               (CtkStyle           *style,
                                  cairo_t            *cr,
                                  CtkStateType        state_type,
                                  CtkShadowType       shadow_type,
                                  CtkWidget          *widget,
                                  const gchar        *detail,
                                  gint                x,
                                  gint                y,
                                  gint                width,
                                  gint                height);
CDK_DEPRECATED_IN_3_0_FOR(CtkStyleContext)
void ctk_paint_shadow_gap        (CtkStyle           *style,
                                  cairo_t            *cr,
                                  CtkStateType        state_type,
                                  CtkShadowType       shadow_type,
                                  CtkWidget          *widget,
                                  const gchar        *detail,
                                  gint                x,
                                  gint                y,
                                  gint                width,
                                  gint                height,
                                  CtkPositionType     gap_side,
                                  gint                gap_x,
                                  gint                gap_width);
CDK_DEPRECATED_IN_3_0_FOR(CtkStyleContext)
void ctk_paint_box_gap           (CtkStyle           *style,
                                  cairo_t            *cr,
                                  CtkStateType        state_type,
                                  CtkShadowType       shadow_type,
                                  CtkWidget          *widget,
                                  const gchar        *detail,
                                  gint                x,
                                  gint                y,
                                  gint                width,
                                  gint                height,
                                  CtkPositionType     gap_side,
                                  gint                gap_x,
                                  gint                gap_width);
CDK_DEPRECATED_IN_3_0_FOR(CtkStyleContext and ctk_render_extension)
void ctk_paint_extension         (CtkStyle           *style,
                                  cairo_t            *cr,
                                  CtkStateType        state_type,
                                  CtkShadowType       shadow_type,
                                  CtkWidget          *widget,
                                  const gchar        *detail,
                                  gint                x,
                                  gint                y,
                                  gint                width,
                                  gint                height,
                                  CtkPositionType     gap_side);
CDK_DEPRECATED_IN_3_0_FOR(CtkStyleContext and ctk_render_focus)
void ctk_paint_focus             (CtkStyle           *style,
                                  cairo_t            *cr,
                                  CtkStateType        state_type,
                                  CtkWidget          *widget,
                                  const gchar        *detail,
                                  gint                x,
                                  gint                y,
                                  gint                width,
                                  gint                height);
CDK_DEPRECATED_IN_3_0_FOR(CtkStyleContext and ctk_render_focus)
void ctk_paint_slider            (CtkStyle           *style,
                                  cairo_t            *cr,
                                  CtkStateType        state_type,
                                  CtkShadowType       shadow_type,
                                  CtkWidget          *widget,
                                  const gchar        *detail,
                                  gint                x,
                                  gint                y,
                                  gint                width,
                                  gint                height,
                                  CtkOrientation      orientation);
CDK_DEPRECATED_IN_3_0_FOR(CtkStyleContext and ctk_render_handle)
void ctk_paint_handle            (CtkStyle           *style,
                                  cairo_t            *cr,
                                  CtkStateType        state_type,
                                  CtkShadowType       shadow_type,
                                  CtkWidget          *widget,
                                  const gchar        *detail,
                                  gint                x,
                                  gint                y,
                                  gint                width,
                                  gint                height,
                                  CtkOrientation      orientation);
CDK_DEPRECATED_IN_3_0_FOR(CtkStyleContext and ctk_render_expander)
void ctk_paint_expander          (CtkStyle           *style,
                                  cairo_t            *cr,
                                  CtkStateType        state_type,
                                  CtkWidget          *widget,
                                  const gchar        *detail,
                                  gint                x,
                                  gint                y,
                                  CtkExpanderStyle    expander_style);
CDK_DEPRECATED_IN_3_0_FOR(CtkStyleContext and ctk_render_layout)
void ctk_paint_layout            (CtkStyle           *style,
                                  cairo_t            *cr,
                                  CtkStateType        state_type,
                                  gboolean            use_text,
                                  CtkWidget          *widget,
                                  const gchar        *detail,
                                  gint                x,
                                  gint                y,
                                  PangoLayout        *layout);
CDK_DEPRECATED_IN_3_0_FOR(CtkStyleContext and ctk_render_handle)
void ctk_paint_resize_grip       (CtkStyle           *style,
                                  cairo_t            *cr,
                                  CtkStateType        state_type,
                                  CtkWidget          *widget,
                                  const gchar        *detail,
                                  CdkWindowEdge       edge,
                                  gint                x,
                                  gint                y,
                                  gint                width,
                                  gint                height);
CDK_DEPRECATED_IN_3_0_FOR(CtkStyleContext and ctk_render_icon)
void ctk_paint_spinner           (CtkStyle           *style,
                                  cairo_t            *cr,
                                  CtkStateType        state_type,
                                  CtkWidget          *widget,
                                  const gchar        *detail,
                                  guint               step,
                                  gint                x,
                                  gint                y,
                                  gint                width,
                                  gint                height);

CDK_DEPRECATED_IN_3_0_FOR(CtkStyleContext and ctk_style_context_get_property)
void ctk_style_get_style_property (CtkStyle    *style,
                                   GType        widget_type,
                                   const gchar *property_name,
                                   GValue      *value);
CDK_DEPRECATED_IN_3_0_FOR(CtkStyleContext and ctk_style_context_get_property)
void ctk_style_get_valist         (CtkStyle    *style,
                                   GType        widget_type,
                                   const gchar *first_property_name,
                                   va_list      var_args);
CDK_DEPRECATED_IN_3_0_FOR(CtkStyleContext and ctk_style_context_get_property)
void ctk_style_get                (CtkStyle    *style,
                                   GType        widget_type,
                                   const gchar *first_property_name,
                                   ...) G_GNUC_NULL_TERMINATED;


/* --- private API --- */
CtkStyle*     _ctk_style_new_for_path     (CdkScreen          *screen,
                                           CtkWidgetPath      *path);
void          _ctk_style_shade            (const CdkColor     *a,
                                           CdkColor           *b,
                                           gdouble             k);

CDK_DEPRECATED_IN_3_0_FOR(CtkStyleContext)
gboolean    ctk_style_has_context         (CtkStyle *style);

CDK_DEPRECATED_IN_3_0
void        ctk_widget_style_attach       (CtkWidget     *widget);
CDK_DEPRECATED_IN_3_0_FOR(CtkStyleContext)
gboolean    ctk_widget_has_rc_style       (CtkWidget            *widget);
CDK_DEPRECATED_IN_3_0
void        ctk_widget_set_style          (CtkWidget            *widget,
                                           CtkStyle             *style);
CDK_DEPRECATED_IN_3_0
void        ctk_widget_ensure_style       (CtkWidget            *widget);
CDK_DEPRECATED_IN_3_0_FOR(ctk_widget_get_style_context)
CtkStyle *  ctk_widget_get_style          (CtkWidget            *widget);
CDK_DEPRECATED_IN_3_0_FOR(CtkStyleContext)
void        ctk_widget_modify_style       (CtkWidget            *widget,
                                           CtkRcStyle           *style);
CDK_DEPRECATED_IN_3_0_FOR(CtkStyleContext)
CtkRcStyle *ctk_widget_get_modifier_style (CtkWidget            *widget);
CDK_DEPRECATED_IN_3_0_FOR(ctk_widget_override_color)
void        ctk_widget_modify_fg          (CtkWidget            *widget,
                                           CtkStateType          state,
                                           const CdkColor       *color);
CDK_DEPRECATED_IN_3_0_FOR(ctk_widget_override_background_color)
void        ctk_widget_modify_bg          (CtkWidget            *widget,
                                           CtkStateType          state,
                                           const CdkColor       *color);
CDK_DEPRECATED_IN_3_0_FOR(CSS style classes)
void        ctk_widget_modify_text        (CtkWidget            *widget,
                                           CtkStateType          state,
                                           const CdkColor       *color);
CDK_DEPRECATED_IN_3_0_FOR(CSS style classes)
void        ctk_widget_modify_base        (CtkWidget            *widget,
                                           CtkStateType          state,
                                           const CdkColor       *color);
CDK_DEPRECATED_IN_3_0_FOR(CSS style classes)
void        ctk_widget_modify_cursor      (CtkWidget            *widget,
                                           const CdkColor       *primary,
                                           const CdkColor       *secondary);
CDK_DEPRECATED_IN_3_0_FOR(ctk_widget_override_font)
void        ctk_widget_modify_font        (CtkWidget            *widget,
                                           PangoFontDescription *font_desc);
CDK_DEPRECATED_IN_3_0_FOR(ctk_widget_reset_style)
void       ctk_widget_reset_rc_styles     (CtkWidget      *widget);
CDK_DEPRECATED_IN_3_0_FOR(ctk_style_context_new)
CtkStyle*  ctk_widget_get_default_style   (void);
CDK_DEPRECATED_IN_3_0_FOR(ctk_widget_get_path)
void       ctk_widget_path                (CtkWidget *widget,
                                           guint     *path_length,
                                           gchar    **path,
                                           gchar    **path_reversed);
CDK_DEPRECATED_IN_3_0
void       ctk_widget_class_path          (CtkWidget *widget,
                                           guint     *path_length,
                                           gchar    **path,
                                           gchar    **path_reversed);
CDK_DEPRECATED_IN_3_0_FOR(ctk_widget_render_icon_pixbuf)
GdkPixbuf *ctk_widget_render_icon         (CtkWidget   *widget,
                                           const gchar *stock_id,
                                           CtkIconSize  size,
                                           const gchar *detail);

G_END_DECLS

#endif /* __CTK_STYLE_H__ */
