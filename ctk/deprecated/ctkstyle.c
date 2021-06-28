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

#include "config.h"

#define CDK_DISABLE_DEPRECATION_WARNINGS

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <gobject/gvaluecollector.h>
#include "ctkmarshalers.h"
#include "ctkpango.h"
#include "ctkrc.h"
#include "ctkspinbutton.h"
#include "ctkstyle.h"
#include "ctkstylecontextprivate.h"
#include "ctkwidget.h"
#include "ctkwidgetprivate.h"
#include "ctkiconfactory.h"
#include "ctkintl.h"
#include "ctkdebug.h"
#include "ctkrender.h"
#include "ctkborder.h"
#include "ctkwidgetpath.h"

/**
 * SECTION:ctkstyle
 * @Short_description: Deprecated object that holds style information
 *     for widgets
 * @Title: CtkStyle
 *
 * A #CtkStyle object encapsulates the information that provides the look and
 * feel for a widget.
 *
 * > In CTK+ 3.0, CtkStyle has been deprecated and replaced by
 * > #CtkStyleContext.
 *
 * Each #CtkWidget has an associated #CtkStyle object that is used when
 * rendering that widget. Also, a #CtkStyle holds information for the five
 * possible widget states though not every widget supports all five
 * states; see #CtkStateType.
 *
 * Usually the #CtkStyle for a widget is the same as the default style that
 * is set by CTK+ and modified the theme engine.
 *
 * Usually applications should not need to use or modify the #CtkStyle of
 * their widgets.
 */


#define LIGHTNESS_MULT  1.3
#define DARKNESS_MULT   0.7

/* --- typedefs & structures --- */
typedef struct {
  GType       widget_type;
  GParamSpec *pspec;
  GValue      value;
} PropertyValue;

typedef struct {
  CtkStyleContext *context;
  gulong context_changed_id;
} CtkStylePrivate;

#define CTK_STYLE_GET_PRIVATE(obj) ((CtkStylePrivate *) ctk_style_get_instance_private ((CtkStyle *) (obj)))

enum {
  PROP_0,
  PROP_CONTEXT
};

/* --- prototypes --- */
static void	 ctk_style_finalize		(GObject	*object);
static void	 ctk_style_constructed		(GObject	*object);
static void      ctk_style_set_property         (GObject        *object,
                                                 guint           prop_id,
                                                 const GValue   *value,
                                                 GParamSpec     *pspec);
static void      ctk_style_get_property         (GObject        *object,
                                                 guint           prop_id,
                                                 GValue         *value,
                                                 GParamSpec     *pspec);

static void      ctk_style_real_realize        (CtkStyle	*style);
static void      ctk_style_real_unrealize      (CtkStyle	*style);
static void      ctk_style_real_copy           (CtkStyle	*style,
						CtkStyle	*src);
static void      ctk_style_real_set_background (CtkStyle	*style,
						CdkWindow	*window,
						CtkStateType	 state_type);
static CtkStyle *ctk_style_real_clone          (CtkStyle	*style);
static void      ctk_style_real_init_from_rc   (CtkStyle	*style,
                                                CtkRcStyle	*rc_style);
static GdkPixbuf *ctk_default_render_icon      (CtkStyle            *style,
                                                const CtkIconSource *source,
                                                CtkTextDirection     direction,
                                                CtkStateType         state,
                                                CtkIconSize          size,
                                                CtkWidget           *widget,
                                                const gchar         *detail);
static void ctk_default_draw_hline      (CtkStyle        *style,
					 cairo_t         *cr,
					 CtkStateType     state_type,
					 CtkWidget       *widget,
					 const gchar     *detail,
					 gint             x1,
					 gint             x2,
					 gint             y);
static void ctk_default_draw_vline      (CtkStyle        *style,
					 cairo_t         *cr,
					 CtkStateType     state_type,
					 CtkWidget       *widget,
					 const gchar     *detail,
					 gint             y1,
					 gint             y2,
					 gint             x);
static void ctk_default_draw_shadow     (CtkStyle        *style,
					 cairo_t         *cr,
					 CtkStateType     state_type,
					 CtkShadowType    shadow_type,
					 CtkWidget       *widget,
					 const gchar     *detail,
					 gint             x,
					 gint             y,
					 gint             width,
					 gint             height);
static void ctk_default_draw_arrow      (CtkStyle        *style,
					 cairo_t         *cr,
					 CtkStateType     state_type,
					 CtkShadowType    shadow_type,
					 CtkWidget       *widget,
					 const gchar     *detail,
					 CtkArrowType     arrow_type,
					 gboolean         fill,
					 gint             x,
					 gint             y,
					 gint             width,
					 gint             height);
static void ctk_default_draw_diamond    (CtkStyle        *style,
					 cairo_t         *cr,
					 CtkStateType     state_type,
					 CtkShadowType    shadow_type,
					 CtkWidget       *widget,
					 const gchar     *detail,
					 gint             x,
					 gint             y,
					 gint             width,
					 gint             height);
static void ctk_default_draw_box        (CtkStyle        *style,
					 cairo_t         *cr,
					 CtkStateType     state_type,
					 CtkShadowType    shadow_type,
					 CtkWidget       *widget,
					 const gchar     *detail,
					 gint             x,
					 gint             y,
					 gint             width,
					 gint             height);
static void ctk_default_draw_flat_box   (CtkStyle        *style,
					 cairo_t         *cr,
					 CtkStateType     state_type,
					 CtkShadowType    shadow_type,
					 CtkWidget       *widget,
					 const gchar     *detail,
					 gint             x,
					 gint             y,
					 gint             width,
					 gint             height);
static void ctk_default_draw_check      (CtkStyle        *style,
					 cairo_t         *cr,
					 CtkStateType     state_type,
					 CtkShadowType    shadow_type,
					 CtkWidget       *widget,
					 const gchar     *detail,
					 gint             x,
					 gint             y,
					 gint             width,
					 gint             height);
static void ctk_default_draw_option     (CtkStyle        *style,
					 cairo_t         *cr,
					 CtkStateType     state_type,
					 CtkShadowType    shadow_type,
					 CtkWidget       *widget,
					 const gchar     *detail,
					 gint             x,
					 gint             y,
					 gint             width,
					 gint             height);
static void ctk_default_draw_tab        (CtkStyle        *style,
					 cairo_t         *cr,
					 CtkStateType     state_type,
					 CtkShadowType    shadow_type,
					 CtkWidget       *widget,
					 const gchar     *detail,
					 gint             x,
					 gint             y,
					 gint             width,
					 gint             height);
static void ctk_default_draw_shadow_gap (CtkStyle        *style,
					 cairo_t         *cr,
					 CtkStateType     state_type,
					 CtkShadowType    shadow_type,
					 CtkWidget       *widget,
					 const gchar     *detail,
					 gint             x,
					 gint             y,
					 gint             width,
					 gint             height,
					 CtkPositionType  gap_side,
					 gint             gap_x,
					 gint             gap_width);
static void ctk_default_draw_box_gap    (CtkStyle        *style,
					 cairo_t         *cr,
					 CtkStateType     state_type,
					 CtkShadowType    shadow_type,
					 CtkWidget       *widget,
					 const gchar     *detail,
					 gint             x,
					 gint             y,
					 gint             width,
					 gint             height,
					 CtkPositionType  gap_side,
					 gint             gap_x,
					 gint             gap_width);
static void ctk_default_draw_extension  (CtkStyle        *style,
					 cairo_t         *cr,
					 CtkStateType     state_type,
					 CtkShadowType    shadow_type,
					 CtkWidget       *widget,
					 const gchar     *detail,
					 gint             x,
					 gint             y,
					 gint             width,
					 gint             height,
					 CtkPositionType  gap_side);
static void ctk_default_draw_focus      (CtkStyle        *style,
					 cairo_t         *cr,
					 CtkStateType     state_type,
					 CtkWidget       *widget,
					 const gchar     *detail,
					 gint             x,
					 gint             y,
					 gint             width,
					 gint             height);
static void ctk_default_draw_slider     (CtkStyle        *style,
					 cairo_t         *cr,
					 CtkStateType     state_type,
					 CtkShadowType    shadow_type,
					 CtkWidget       *widget,
					 const gchar     *detail,
					 gint             x,
					 gint             y,
					 gint             width,
					 gint             height,
					 CtkOrientation   orientation);
static void ctk_default_draw_handle     (CtkStyle        *style,
					 cairo_t         *cr,
					 CtkStateType     state_type,
					 CtkShadowType    shadow_type,
					 CtkWidget       *widget,
					 const gchar     *detail,
					 gint             x,
					 gint             y,
					 gint             width,
					 gint             height,
					 CtkOrientation   orientation);
static void ctk_default_draw_expander   (CtkStyle        *style,
                                         cairo_t         *cr,
                                         CtkStateType     state_type,
                                         CtkWidget       *widget,
                                         const gchar     *detail,
                                         gint             x,
                                         gint             y,
					 CtkExpanderStyle expander_style);
static void ctk_default_draw_layout     (CtkStyle        *style,
                                         cairo_t         *cr,
                                         CtkStateType     state_type,
					 gboolean         use_text,
                                         CtkWidget       *widget,
                                         const gchar     *detail,
                                         gint             x,
                                         gint             y,
                                         PangoLayout     *layout);
static void ctk_default_draw_resize_grip (CtkStyle       *style,
                                          cairo_t        *cr,
                                          CtkStateType    state_type,
                                          CtkWidget      *widget,
                                          const gchar    *detail,
                                          CdkWindowEdge   edge,
                                          gint            x,
                                          gint            y,
                                          gint            width,
                                          gint            height);
static void ctk_default_draw_spinner     (CtkStyle       *style,
                                          cairo_t        *cr,
					  CtkStateType    state_type,
                                          CtkWidget      *widget,
                                          const gchar    *detail,
					  guint           step,
					  gint            x,
					  gint            y,
					  gint            width,
					  gint            height);

static void rgb_to_hls			(gdouble	 *r,
					 gdouble	 *g,
					 gdouble	 *b);
static void hls_to_rgb			(gdouble	 *h,
					 gdouble	 *l,
					 gdouble	 *s);

static void transform_detail_string (const gchar     *detail,
                                     CtkStyleContext *context);

/*
 * Data for default check and radio buttons
 */

static const CtkRequisition default_option_indicator_size = { 7, 13 };
static const CtkBorder default_option_indicator_spacing = { 7, 5, 2, 2 };

#define CTK_GRAY		0xdcdc, 0xdada, 0xd5d5
#define CTK_DARK_GRAY		0xc4c4, 0xc2c2, 0xbdbd
#define CTK_LIGHT_GRAY		0xeeee, 0xebeb, 0xe7e7
#define CTK_WHITE		0xffff, 0xffff, 0xffff
#define CTK_BLUE		0x4b4b, 0x6969, 0x8383
#define CTK_VERY_DARK_GRAY	0x9c9c, 0x9a9a, 0x9494
#define CTK_BLACK		0x0000, 0x0000, 0x0000
#define CTK_WEAK_GRAY		0x7530, 0x7530, 0x7530

/* --- variables --- */
static const CdkColor ctk_default_normal_fg =      { 0, CTK_BLACK };
static const CdkColor ctk_default_active_fg =      { 0, CTK_BLACK };
static const CdkColor ctk_default_prelight_fg =    { 0, CTK_BLACK };
static const CdkColor ctk_default_selected_fg =    { 0, CTK_WHITE };
static const CdkColor ctk_default_insensitive_fg = { 0, CTK_WEAK_GRAY };

static const CdkColor ctk_default_normal_bg =      { 0, CTK_GRAY };
static const CdkColor ctk_default_active_bg =      { 0, CTK_DARK_GRAY };
static const CdkColor ctk_default_prelight_bg =    { 0, CTK_LIGHT_GRAY };
static const CdkColor ctk_default_selected_bg =    { 0, CTK_BLUE };
static const CdkColor ctk_default_insensitive_bg = { 0, CTK_GRAY };
static const CdkColor ctk_default_selected_base =  { 0, CTK_BLUE };
static const CdkColor ctk_default_active_base =    { 0, CTK_VERY_DARK_GRAY };

static GQuark quark_default_style;

/* --- signals --- */
static guint realize_signal = 0;
static guint unrealize_signal = 0;

G_DEFINE_TYPE_WITH_PRIVATE (CtkStyle, ctk_style, G_TYPE_OBJECT)

/* --- functions --- */

static void
ctk_style_init (CtkStyle *style)
{
  gint i;

  style->font_desc = pango_font_description_from_string ("Sans 10");

  style->attach_count = 0;
  
  style->black.red = 0;
  style->black.green = 0;
  style->black.blue = 0;
  
  style->white.red = 65535;
  style->white.green = 65535;
  style->white.blue = 65535;
  
  style->fg[CTK_STATE_NORMAL] = ctk_default_normal_fg;
  style->fg[CTK_STATE_ACTIVE] = ctk_default_active_fg;
  style->fg[CTK_STATE_PRELIGHT] = ctk_default_prelight_fg;
  style->fg[CTK_STATE_SELECTED] = ctk_default_selected_fg;
  style->fg[CTK_STATE_INSENSITIVE] = ctk_default_insensitive_fg;
  
  style->bg[CTK_STATE_NORMAL] = ctk_default_normal_bg;
  style->bg[CTK_STATE_ACTIVE] = ctk_default_active_bg;
  style->bg[CTK_STATE_PRELIGHT] = ctk_default_prelight_bg;
  style->bg[CTK_STATE_SELECTED] = ctk_default_selected_bg;
  style->bg[CTK_STATE_INSENSITIVE] = ctk_default_insensitive_bg;
  
  for (i = 0; i < 4; i++)
    {
      style->text[i] = style->fg[i];
      style->base[i] = style->white;
    }

  style->base[CTK_STATE_SELECTED] = ctk_default_selected_base;
  style->text[CTK_STATE_SELECTED] = style->white;
  style->base[CTK_STATE_ACTIVE] = ctk_default_active_base;
  style->text[CTK_STATE_ACTIVE] = style->white;
  style->base[CTK_STATE_INSENSITIVE] = ctk_default_prelight_bg;
  style->text[CTK_STATE_INSENSITIVE] = ctk_default_insensitive_fg;
  
  style->rc_style = NULL;
  
  style->xthickness = 2;
  style->ythickness = 2;

  style->property_cache = NULL;
}

static void
ctk_style_class_init (CtkStyleClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  object_class->finalize = ctk_style_finalize;
  object_class->set_property = ctk_style_set_property;
  object_class->get_property = ctk_style_get_property;
  object_class->constructed = ctk_style_constructed;

  klass->clone = ctk_style_real_clone;
  klass->copy = ctk_style_real_copy;
  klass->init_from_rc = ctk_style_real_init_from_rc;
  klass->realize = ctk_style_real_realize;
  klass->unrealize = ctk_style_real_unrealize;
  klass->set_background = ctk_style_real_set_background;
  klass->render_icon = ctk_default_render_icon;

  klass->draw_hline = ctk_default_draw_hline;
  klass->draw_vline = ctk_default_draw_vline;
  klass->draw_shadow = ctk_default_draw_shadow;
  klass->draw_arrow = ctk_default_draw_arrow;
  klass->draw_diamond = ctk_default_draw_diamond;
  klass->draw_box = ctk_default_draw_box;
  klass->draw_flat_box = ctk_default_draw_flat_box;
  klass->draw_check = ctk_default_draw_check;
  klass->draw_option = ctk_default_draw_option;
  klass->draw_tab = ctk_default_draw_tab;
  klass->draw_shadow_gap = ctk_default_draw_shadow_gap;
  klass->draw_box_gap = ctk_default_draw_box_gap;
  klass->draw_extension = ctk_default_draw_extension;
  klass->draw_focus = ctk_default_draw_focus;
  klass->draw_slider = ctk_default_draw_slider;
  klass->draw_handle = ctk_default_draw_handle;
  klass->draw_expander = ctk_default_draw_expander;
  klass->draw_layout = ctk_default_draw_layout;
  klass->draw_resize_grip = ctk_default_draw_resize_grip;
  klass->draw_spinner = ctk_default_draw_spinner;

  g_object_class_install_property (object_class,
				   PROP_CONTEXT,
				   g_param_spec_object ("context",
 							P_("Style context"),
							P_("CtkStyleContext to get style from"),
                                                        CTK_TYPE_STYLE_CONTEXT,
                                                        G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));

  /**
   * CtkStyle::realize:
   * @style: the object which received the signal
   *
   * Emitted when the style has been initialized for a particular
   * visual. Connecting to this signal is probably seldom
   * useful since most of the time applications and widgets only
   * deal with styles that have been already realized.
   *
   * Since: 2.4
   */
  realize_signal = g_signal_new (I_("realize"),
				 G_TYPE_FROM_CLASS (object_class),
				 G_SIGNAL_RUN_FIRST,
				 G_STRUCT_OFFSET (CtkStyleClass, realize),
				 NULL, NULL,
				 NULL,
				 G_TYPE_NONE, 0);
  /**
   * CtkStyle::unrealize:
   * @style: the object which received the signal
   *
   * Emitted when the aspects of the style specific to a particular visual
   * is being cleaned up. A connection to this signal can be useful
   * if a widget wants to cache objects as object data on #CtkStyle.
   * This signal provides a convenient place to free such cached objects.
   *
   * Since: 2.4
   */
  unrealize_signal = g_signal_new (I_("unrealize"),
				   G_TYPE_FROM_CLASS (object_class),
				   G_SIGNAL_RUN_FIRST,
				   G_STRUCT_OFFSET (CtkStyleClass, unrealize),
				   NULL, NULL,
				   NULL,
				   G_TYPE_NONE, 0);
}

static void
ctk_style_finalize (GObject *object)
{
  CtkStyle *style = CTK_STYLE (object);
  CtkStylePrivate *priv = CTK_STYLE_GET_PRIVATE (style);
  gint i;

  g_return_if_fail (style->attach_count == 0);

  /* All the styles in the list have the same 
   * style->styles pointer. If we delete the 
   * *first* style from the list, we need to update
   * the style->styles pointers from all the styles.
   * Otherwise we simply remove the node from
   * the list.
   */
  if (style->styles)
    {
      if (style->styles->data != style)
        style->styles = g_slist_remove (style->styles, style);
      else
        {
          GSList *tmp_list = style->styles->next;
	  
          while (tmp_list)
            {
              CTK_STYLE (tmp_list->data)->styles = style->styles->next;
              tmp_list = tmp_list->next;
            }
          g_slist_free_1 (style->styles);
        }
    }

  g_slist_free_full (style->icon_factories, g_object_unref);

  pango_font_description_free (style->font_desc);

  if (style->private_font_desc)
    pango_font_description_free (style->private_font_desc);

  if (style->rc_style)
    g_object_unref (style->rc_style);

  if (priv->context)
    {
      if (priv->context_changed_id)
        g_signal_handler_disconnect (priv->context, priv->context_changed_id);

      g_object_unref (priv->context);
    }

  for (i = 0; i < 5; i++)
    {
      if (style->background[i])
        cairo_pattern_destroy (style->background[i]);
    }

  G_OBJECT_CLASS (ctk_style_parent_class)->finalize (object);
}

static void
ctk_style_set_property (GObject      *object,
                        guint         prop_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  CtkStylePrivate *priv;

  priv = CTK_STYLE_GET_PRIVATE (object);

  switch (prop_id)
    {
    case PROP_CONTEXT:
      priv->context = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_style_get_property (GObject      *object,
                        guint         prop_id,
                        GValue       *value,
                        GParamSpec   *pspec)
{
  CtkStylePrivate *priv;

  priv = CTK_STYLE_GET_PRIVATE (object);

  switch (prop_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, priv->context);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static gboolean
set_color_from_context (CtkStyle *style,
                        CtkStateType state,
                        CtkStyleContext *context,
                        CtkRcFlags prop)
{
  CdkRGBA *color = NULL;
  CdkColor *dest = { 0 }; /* Shut up gcc */
  CtkStateFlags flags;

  flags = ctk_style_context_get_state (context);

  switch (prop)
    {
    case CTK_RC_BG:
      ctk_style_context_get (context, flags,
                             "background-color", &color,
                             NULL);
      dest = &style->bg[state];
      break;
    case CTK_RC_FG:
      ctk_style_context_get (context, flags,
                             "color", &color,
                             NULL);
      dest = &style->fg[state];
      break;
    case CTK_RC_TEXT:
      ctk_style_context_get (context, flags,
                             "color", &color,
                             NULL);
      dest = &style->text[state];
      break;
    case CTK_RC_BASE:
      ctk_style_context_get (context, flags,
                             "background-color", &color,
                             NULL);
      dest = &style->base[state];
      break;
    }

  if (!color)
    return FALSE;

  if (!(color->alpha > 0.01))
    {
      cdk_rgba_free (color);
      return FALSE;
    }

  dest->pixel = 0;
  dest->red = CLAMP ((guint) (color->red * 65535), 0, 65535);
  dest->green = CLAMP ((guint) (color->green * 65535), 0, 65535);
  dest->blue = CLAMP ((guint) (color->blue * 65535), 0, 65535);
  cdk_rgba_free (color);

  return TRUE;
}

static void
set_color (CtkStyle        *style,
           CtkStyleContext *context,
           CtkStateType     state,
           CtkRcFlags       prop)
{
  /* Try to fill in the values from the associated CtkStyleContext.
   * Since fully-transparent black is a very common default (e.g. for 
   * background-color properties), and we must store the result in a CdkColor
   * to retain API compatibility, in case the fetched color is fully transparent
   * we give themes a fallback style class they can style, before using the
   * hardcoded default values.
   */
  if (!set_color_from_context (style, state, context, prop))
    {
      ctk_style_context_save (context);
      ctk_style_context_add_class (context, "ctkstyle-fallback");
      set_color_from_context (style, state, context, prop);
      ctk_style_context_restore (context);
    }
}

static void
ctk_style_update_from_context (CtkStyle *style)
{
  CtkStylePrivate *priv;
  CtkStateType state;
  CtkStateFlags flags;
  CtkBorder padding;
  gint i;

  priv = CTK_STYLE_GET_PRIVATE (style);

  for (state = CTK_STATE_NORMAL; state <= CTK_STATE_INSENSITIVE; state++)
    {
      switch (state)
        {
        case CTK_STATE_ACTIVE:
          flags = CTK_STATE_FLAG_ACTIVE;
          break;
        case CTK_STATE_PRELIGHT:
          flags = CTK_STATE_FLAG_PRELIGHT;
          break;
        case CTK_STATE_SELECTED:
          flags = CTK_STATE_FLAG_SELECTED;
          break;
        case CTK_STATE_INSENSITIVE:
          flags = CTK_STATE_FLAG_INSENSITIVE;
          break;
        default:
          flags = 0;
        }

      ctk_style_context_save (priv->context);
      ctk_style_context_set_state (priv->context, flags);

      if (ctk_style_context_has_class (priv->context, "entry"))
        {
          ctk_style_context_save (priv->context);
          ctk_style_context_remove_class (priv->context, "entry");
          set_color (style, priv->context, state, CTK_RC_BG);
          set_color (style, priv->context, state, CTK_RC_FG);
          ctk_style_context_restore (priv->context);

          set_color (style, priv->context, state, CTK_RC_BASE);
          set_color (style, priv->context, state, CTK_RC_TEXT);
        }
      else
        {
          ctk_style_context_save (priv->context);
          ctk_style_context_add_class (priv->context, "entry");
          set_color (style, priv->context, state, CTK_RC_BASE);
          set_color (style, priv->context, state, CTK_RC_TEXT);
          ctk_style_context_restore (priv->context);

          set_color (style, priv->context, state, CTK_RC_BG);
          set_color (style, priv->context, state, CTK_RC_FG);
        }

      ctk_style_context_restore (priv->context);
    }

  if (style->font_desc)
    pango_font_description_free (style->font_desc);

  flags = ctk_style_context_get_state (priv->context);
  ctk_style_context_get (priv->context, flags,
                         "font", &style->font_desc,
                         NULL);
  ctk_style_context_get_padding (priv->context, flags, &padding);

  style->xthickness = padding.left;
  style->ythickness = padding.top;

  for (i = 0; i < 5; i++)
    {
      _ctk_style_shade (&style->bg[i], &style->light[i], LIGHTNESS_MULT);
      _ctk_style_shade (&style->bg[i], &style->dark[i], DARKNESS_MULT);

      style->mid[i].red = (style->light[i].red + style->dark[i].red) / 2;
      style->mid[i].green = (style->light[i].green + style->dark[i].green) / 2;
      style->mid[i].blue = (style->light[i].blue + style->dark[i].blue) / 2;

      style->text_aa[i].red = (style->text[i].red + style->base[i].red) / 2;
      style->text_aa[i].green = (style->text[i].green + style->base[i].green) / 2;
      style->text_aa[i].blue = (style->text[i].blue + style->base[i].blue) / 2;
    }

  style->black.red = 0x0000;
  style->black.green = 0x0000;
  style->black.blue = 0x0000;

  style->white.red = 0xffff;
  style->white.green = 0xffff;
  style->white.blue = 0xffff;

  for (i = 0; i < 5; i++)
    {
      if (style->background[i])
        cairo_pattern_destroy (style->background[i]);

      style->background[i] = cairo_pattern_create_rgb (style->bg[i].red / 65535.0,
                                                       style->bg[i].green / 65535.0,
                                                       style->bg[i].blue / 65535.0);
    }
}

static void
style_context_changed (CtkStyleContext *context,
                       gpointer         user_data)
{
  ctk_style_update_from_context (CTK_STYLE (user_data));
}

static void
ctk_style_constructed (GObject *object)
{
  CtkStylePrivate *priv;

  priv = CTK_STYLE_GET_PRIVATE (object);

  if (priv->context)
    {
      ctk_style_update_from_context (CTK_STYLE (object));

      priv->context_changed_id = g_signal_connect (priv->context, "changed",
                                                   G_CALLBACK (style_context_changed), object);
    }
}

/**
 * ctk_style_copy:
 * @style: a #CtkStyle
 *
 * Creates a copy of the passed in #CtkStyle object.
 *
 * Returns: (transfer full): a copy of @style
 *
 * Deprecated:3.0: Use #CtkStyleContext instead
 */
CtkStyle*
ctk_style_copy (CtkStyle *style)
{
  CtkStyle *new_style;
  
  g_return_val_if_fail (CTK_IS_STYLE (style), NULL);
  
  new_style = CTK_STYLE_GET_CLASS (style)->clone (style);
  CTK_STYLE_GET_CLASS (style)->copy (new_style, style);

  return new_style;
}

CtkStyle*
_ctk_style_new_for_path (CdkScreen     *screen,
                         CtkWidgetPath *path)
{
  CtkStyleContext *context;
  CtkStyle *style;

  context = ctk_style_context_new ();

  if (screen)
    ctk_style_context_set_screen (context, screen);

  ctk_style_context_set_path (context, path);

  style = g_object_new (CTK_TYPE_STYLE,
                        "context", context,
                        NULL);

  g_object_unref (context);

  return style;
}

/**
 * ctk_style_new:
 *
 * Creates a new #CtkStyle.
 *
 * Returns: a new #CtkStyle.
 *
 * Deprecated: 3.0: Use #CtkStyleContext
 */
CtkStyle*
ctk_style_new (void)
{
  CtkWidgetPath *path;
  CtkStyle *style;

  path = ctk_widget_path_new ();
  ctk_widget_path_append_type (path, CTK_TYPE_WIDGET);

  style = _ctk_style_new_for_path (cdk_screen_get_default (), path);

  ctk_widget_path_free (path);

  return style;
}

/**
 * ctk_style_has_context:
 * @style: a #CtkStyle
 *
 * Returns whether @style has an associated #CtkStyleContext.
 *
 * Returns: %TRUE if @style has a #CtkStyleContext
 *
 * Since: 3.0
 */
gboolean
ctk_style_has_context (CtkStyle *style)
{
  CtkStylePrivate *priv;

  priv = CTK_STYLE_GET_PRIVATE (style);

  return priv->context != NULL;
}

/**
 * ctk_style_attach: (skip)
 * @style: a #CtkStyle.
 * @window: a #CdkWindow.
 *
 * Attaches a style to a window; this process allocates the
 * colors and creates the GC’s for the style - it specializes
 * it to a particular visual. The process may involve the creation
 * of a new style if the style has already been attached to a
 * window with a different style and visual.
 *
 * Since this function may return a new object, you have to use it
 * in the following way:
 * `style = ctk_style_attach (style, window)`
 *
 * Returns: Either @style, or a newly-created #CtkStyle.
 *   If the style is newly created, the style parameter
 *   will be unref'ed, and the new style will have
 *   a reference count belonging to the caller.
 *
 * Deprecated:3.0: Use ctk_widget_style_attach() instead
 */
CtkStyle*
ctk_style_attach (CtkStyle  *style,
                  CdkWindow *window)
{
  g_return_val_if_fail (CTK_IS_STYLE (style), NULL);
  g_return_val_if_fail (window != NULL, NULL);

  return style;
}

/**
 * ctk_style_detach:
 * @style: a #CtkStyle
 *
 * Detaches a style from a window. If the style is not attached
 * to any windows anymore, it is unrealized. See ctk_style_attach().
 *
 * Deprecated:3.0: Use #CtkStyleContext instead
 */
void
ctk_style_detach (CtkStyle *style)
{
  g_return_if_fail (CTK_IS_STYLE (style));
}

/**
 * ctk_style_lookup_icon_set:
 * @style: a #CtkStyle
 * @stock_id: an icon name
 *
 * Looks up @stock_id in the icon factories associated with @style
 * and the default icon factory, returning an icon set if found,
 * otherwise %NULL.
 *
 * Returns: (transfer none): icon set of @stock_id
 *
 * Deprecated:3.0: Use ctk_style_context_lookup_icon_set() instead
 */
CtkIconSet*
ctk_style_lookup_icon_set (CtkStyle   *style,
                           const char *stock_id)
{
  CtkStylePrivate *priv;

  g_return_val_if_fail (CTK_IS_STYLE (style), NULL);
  g_return_val_if_fail (stock_id != NULL, NULL);

  priv = CTK_STYLE_GET_PRIVATE (style);

  if (priv->context)
    return ctk_style_context_lookup_icon_set (priv->context, stock_id);

  return ctk_icon_factory_lookup_default (stock_id);
}

/**
 * ctk_style_lookup_color:
 * @style: a #CtkStyle
 * @color_name: the name of the logical color to look up
 * @color: (out): the #CdkColor to fill in
 *
 * Looks up @color_name in the style’s logical color mappings,
 * filling in @color and returning %TRUE if found, otherwise
 * returning %FALSE. Do not cache the found mapping, because
 * it depends on the #CtkStyle and might change when a theme
 * switch occurs.
 *
 * Returns: %TRUE if the mapping was found.
 *
 * Since: 2.10
 *
 * Deprecated:3.0: Use ctk_style_context_lookup_color() instead
 **/
gboolean
ctk_style_lookup_color (CtkStyle   *style,
                        const char *color_name,
                        CdkColor   *color)
{
  CtkStylePrivate *priv;
  gboolean result;
  CdkRGBA rgba;

  g_return_val_if_fail (CTK_IS_STYLE (style), FALSE);
  g_return_val_if_fail (color_name != NULL, FALSE);
  g_return_val_if_fail (color != NULL, FALSE);

  priv = CTK_STYLE_GET_PRIVATE (style);

  if (!priv->context)
    return FALSE;

  result = ctk_style_context_lookup_color (priv->context, color_name, &rgba);

  if (color)
    {
      color->red = (guint16) (rgba.red * 65535);
      color->green = (guint16) (rgba.green * 65535);
      color->blue = (guint16) (rgba.blue * 65535);
      color->pixel = 0;
    }

  return result;
}

/**
 * ctk_style_set_background:
 * @style: a #CtkStyle
 * @window: a #CdkWindow
 * @state_type: a state
 * 
 * Sets the background of @window to the background color or pixmap
 * specified by @style for the given state.
 *
 * Deprecated:3.0: Use ctk_style_context_set_background() instead
 */
void
ctk_style_set_background (CtkStyle    *style,
                          CdkWindow   *window,
                          CtkStateType state_type)
{
  g_return_if_fail (CTK_IS_STYLE (style));
  g_return_if_fail (window != NULL);
  
  CTK_STYLE_GET_CLASS (style)->set_background (style, window, state_type);
}

/* Default functions */
static CtkStyle *
ctk_style_real_clone (CtkStyle *style)
{
  CtkStylePrivate *priv;

  priv = CTK_STYLE_GET_PRIVATE (style);

  return g_object_new (G_OBJECT_TYPE (style),
                       "context", priv->context,
                       NULL);
}

static void
ctk_style_real_copy (CtkStyle *style,
		     CtkStyle *src)
{
  gint i;
  
  for (i = 0; i < 5; i++)
    {
      style->fg[i] = src->fg[i];
      style->bg[i] = src->bg[i];
      style->text[i] = src->text[i];
      style->base[i] = src->base[i];

      if (style->background[i])
	cairo_pattern_destroy (style->background[i]),
      style->background[i] = src->background[i];
      if (style->background[i])
	cairo_pattern_reference (style->background[i]);
    }

  if (style->font_desc)
    pango_font_description_free (style->font_desc);
  if (src->font_desc)
    style->font_desc = pango_font_description_copy (src->font_desc);
  else
    style->font_desc = NULL;
  
  style->xthickness = src->xthickness;
  style->ythickness = src->ythickness;

  if (style->rc_style)
    g_object_unref (style->rc_style);
  style->rc_style = src->rc_style;
  if (src->rc_style)
    g_object_ref (src->rc_style);

  g_slist_free_full (style->icon_factories, g_object_unref);
  style->icon_factories = g_slist_copy (src->icon_factories);
  g_slist_foreach (style->icon_factories, (GFunc) g_object_ref, NULL);
}

static void
ctk_style_real_init_from_rc (CtkStyle   *style,
			     CtkRcStyle *rc_style)
{
}

/**
 * ctk_style_get_style_property:
 * @style: a #CtkStyle
 * @widget_type: the #GType of a descendant of #CtkWidget
 * @property_name: the name of the style property to get
 * @value: (out): a #GValue where the value of the property being
 *     queried will be stored
 *
 * Queries the value of a style property corresponding to a
 * widget class is in the given style.
 *
 * Since: 2.16
 */
void 
ctk_style_get_style_property (CtkStyle     *style,
                              GType        widget_type,
                              const gchar *property_name,
                              GValue      *value)
{
  CtkStylePrivate *priv;
  CtkWidgetClass *klass;
  GParamSpec *pspec;
  const GValue *peek_value;

  klass = g_type_class_ref (widget_type);
  pspec = ctk_widget_class_find_style_property (klass, property_name);
  g_type_class_unref (klass);

  if (!pspec)
    {
      g_warning ("%s: widget class `%s' has no property named `%s'",
                 G_STRLOC,
                 g_type_name (widget_type),
                 property_name);
      return;
    }

  priv = CTK_STYLE_GET_PRIVATE (style);
  peek_value = _ctk_style_context_peek_style_property (priv->context,
                                                       widget_type,
                                                       pspec);

  if (G_VALUE_TYPE (value) == G_PARAM_SPEC_VALUE_TYPE (pspec))
    g_value_copy (peek_value, value);
  else if (g_value_type_transformable (G_PARAM_SPEC_VALUE_TYPE (pspec), G_VALUE_TYPE (value)))
    g_value_transform (peek_value, value);
  else
    g_warning ("can't retrieve style property `%s' of type `%s' as value of type `%s'",
               pspec->name,
               g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)),
               G_VALUE_TYPE_NAME (value));
}

/**
 * ctk_style_get_valist:
 * @style: a #CtkStyle
 * @widget_type: the #GType of a descendant of #CtkWidget
 * @first_property_name: the name of the first style property to get
 * @var_args: a va_list of pairs of property names and
 *     locations to return the property values, starting with the
 *     location for @first_property_name.
 *
 * Non-vararg variant of ctk_style_get().
 * Used primarily by language bindings.
 *
 * Since: 2.16
 */
void 
ctk_style_get_valist (CtkStyle    *style,
                      GType        widget_type,
                      const gchar *first_property_name,
                      va_list      var_args)
{
  CtkStylePrivate *priv;
  const char *property_name;
  CtkWidgetClass *klass;

  g_return_if_fail (CTK_IS_STYLE (style));

  klass = g_type_class_ref (widget_type);

  priv = CTK_STYLE_GET_PRIVATE (style);
  property_name = first_property_name;
  while (property_name)
    {
      GParamSpec *pspec;
      const GValue *peek_value;
      gchar *error;

      pspec = ctk_widget_class_find_style_property (klass, property_name);

      if (!pspec)
        {
          g_warning ("%s: widget class `%s' has no property named `%s'",
                     G_STRLOC,
                     g_type_name (widget_type),
                     property_name);
          break;
        }

      peek_value = _ctk_style_context_peek_style_property (priv->context, widget_type,
                                                           pspec);
      G_VALUE_LCOPY (peek_value, var_args, 0, &error);
      if (error)
        {
          g_warning ("%s: %s", G_STRLOC, error);
          g_free (error);
          break;
        }

      property_name = va_arg (var_args, gchar*);
    }

  g_type_class_unref (klass);
}

/**
 * ctk_style_get:
 * @style: a #CtkStyle
 * @widget_type: the #GType of a descendant of #CtkWidget
 * @first_property_name: the name of the first style property to get
 * @...: pairs of property names and locations to
 *   return the property values, starting with the location for
 *   @first_property_name, terminated by %NULL.
 *
 * Gets the values of a multiple style properties for @widget_type
 * from @style.
 *
 * Since: 2.16
 */
void
ctk_style_get (CtkStyle    *style,
               GType        widget_type,
               const gchar *first_property_name,
               ...)
{
  va_list var_args;

  va_start (var_args, first_property_name);
  ctk_style_get_valist (style, widget_type, first_property_name, var_args);
  va_end (var_args);
}

static void
ctk_style_real_realize (CtkStyle *style)
{
}

static void
ctk_style_real_unrealize (CtkStyle *style)
{
}

static void
ctk_style_real_set_background (CtkStyle    *style,
			       CdkWindow   *window,
			       CtkStateType state_type)
{
  cdk_window_set_background_pattern (window, style->background[state_type]);
}

/**
 * ctk_style_render_icon:
 * @style: a #CtkStyle
 * @source: the #CtkIconSource specifying the icon to render
 * @direction: a text direction
 * @state: a state
 * @size: (type int): the size to render the icon at (#CtkIconSize). A size of
 *     `(CtkIconSize)-1` means render at the size of the source and
 *     don’t scale.
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 *
 * Renders the icon specified by @source at the given @size
 * according to the given parameters and returns the result in a
 * pixbuf.
 *
 * Returns: (transfer full): a newly-created #GdkPixbuf
 *     containing the rendered icon
 *
 * Deprecated:3.0: Use ctk_render_icon_pixbuf() instead
 */
GdkPixbuf *
ctk_style_render_icon (CtkStyle            *style,
                       const CtkIconSource *source,
                       CtkTextDirection     direction,
                       CtkStateType         state,
                       CtkIconSize          size,
                       CtkWidget           *widget,
                       const gchar         *detail)
{
  GdkPixbuf *pixbuf;
  
  g_return_val_if_fail (CTK_IS_STYLE (style), NULL);
  g_return_val_if_fail (CTK_STYLE_GET_CLASS (style)->render_icon != NULL, NULL);
  
  pixbuf = CTK_STYLE_GET_CLASS (style)->render_icon (style, source, direction, state,
                                                     size, widget, detail);

  g_return_val_if_fail (pixbuf != NULL, NULL);

  return pixbuf;
}

/* Default functions */

/**
 * ctk_style_apply_default_background:
 * @style:
 * @cr:
 * @window:
 * @state_type:
 * @x:
 * @y:
 * @width:
 * @height:
 *
 * Deprecated:3.0: Use #CtkStyleContext instead
 */
void
ctk_style_apply_default_background (CtkStyle          *style,
                                    cairo_t           *cr,
                                    CdkWindow         *window,
                                    CtkStateType       state_type,
                                    gint               x,
                                    gint               y,
                                    gint               width,
                                    gint               height)
{
  cairo_save (cr);

  if (style->background[state_type] == NULL)
    {
      CdkWindow *parent = cdk_window_get_parent (window);
      int x_offset, y_offset;

      if (parent)
        {
          cdk_window_get_position (window, &x_offset, &y_offset);
          cairo_translate (cr, -x_offset, -y_offset);
          ctk_style_apply_default_background (style, cr,
                                              parent, state_type,
                                              x + x_offset, y + y_offset,
                                              width, height);
          goto out;
        }
      else
        cdk_cairo_set_source_color (cr, &style->bg[state_type]);
    }
  else
    cairo_set_source (cr, style->background[state_type]);

  cairo_rectangle (cr, x, y, width, height);
  cairo_fill (cr);

out:
  cairo_restore (cr);
}

static GdkPixbuf *
ctk_default_render_icon (CtkStyle            *style,
                         const CtkIconSource *source,
                         CtkTextDirection     direction,
                         CtkStateType         state,
                         CtkIconSize          size,
                         CtkWidget           *widget,
                         const gchar         *detail)
{
  CtkStyleContext *context;
  CtkStylePrivate *priv;
  CtkStateFlags flags = 0;
  GdkPixbuf *pixbuf;

  if (widget)
    context = ctk_widget_get_style_context (widget);
  else
    {
      priv = CTK_STYLE_GET_PRIVATE (style);
      context = priv->context;
    }

  if (!context)
    return NULL;

  ctk_style_context_save (context);

  if (detail)
    transform_detail_string (detail, context);

  switch (state)
    {
    case CTK_STATE_PRELIGHT:
      flags |= CTK_STATE_FLAG_PRELIGHT;
      break;
    case CTK_STATE_INSENSITIVE:
      flags |= CTK_STATE_FLAG_INSENSITIVE;
      break;
    default:
      break;
    }

  ctk_style_context_set_state (context, flags);

  pixbuf = ctk_render_icon_pixbuf (context, source, size);

  ctk_style_context_restore (context);

  return pixbuf;
}

static void
_cairo_draw_line (cairo_t  *cr,
                  CdkColor *color,
                  gint      x1,
                  gint      y1,
                  gint      x2,
                  gint      y2)
{
  cairo_save (cr);

  cdk_cairo_set_source_color (cr, color);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);

  cairo_move_to (cr, x1 + 0.5, y1 + 0.5);
  cairo_line_to (cr, x2 + 0.5, y2 + 0.5);
  cairo_stroke (cr);

  cairo_restore (cr);
}

static void
transform_detail_string (const gchar     *detail,
			 CtkStyleContext *context)
{
  if (!detail)
    return;

  if (strcmp (detail, "arrow") == 0)
    ctk_style_context_add_class (context, "arrow");
  else if (strcmp (detail, "button") == 0)
    ctk_style_context_add_class (context, "button");
  else if (strcmp (detail, "buttondefault") == 0)
    {
      ctk_style_context_add_class (context, "button");
      ctk_style_context_add_class (context, "default");
    }
  else if (strcmp (detail, "calendar") == 0)
    ctk_style_context_add_class (context, "calendar");
  else if (strcmp (detail, "cellcheck") == 0)
    {
      ctk_style_context_add_class (context, "cell");
      ctk_style_context_add_class (context, "check");
    }
  else if (strcmp (detail, "cellradio") == 0)
    {
      ctk_style_context_add_class (context, "cell");
      ctk_style_context_add_class (context, "radio");
    }
  else if (strcmp (detail, "checkbutton") == 0)
    ctk_style_context_add_class (context, "check");
  else if (strcmp (detail, "check") == 0)
    {
      ctk_style_context_add_class (context, "check");
      ctk_style_context_add_class (context, "menu");
    }
  else if (strcmp (detail, "radiobutton") == 0)
    {
      ctk_style_context_add_class (context, "radio");
    }
  else if (strcmp (detail, "option") == 0)
    {
      ctk_style_context_add_class (context, "radio");
      ctk_style_context_add_class (context, "menu");
    }
  else if (strcmp (detail, "entry") == 0 ||
           strcmp (detail, "entry_bg") == 0)
    ctk_style_context_add_class (context, "entry");
  else if (strcmp (detail, "expander") == 0)
    ctk_style_context_add_class (context, "expander");
  else if (strcmp (detail, "tooltip") == 0)
    ctk_style_context_add_class (context, "tooltip");
  else if (strcmp (detail, "frame") == 0)
    ctk_style_context_add_class (context, "frame");
  else if (strcmp (detail, "scrolled_window") == 0)
    ctk_style_context_add_class (context, "scrolled-window");
  else if (strcmp (detail, "viewport") == 0 ||
	   strcmp (detail, "viewportbin") == 0)
    ctk_style_context_add_class (context, "viewport");
  else if (strncmp (detail, "trough", 6) == 0)
    ctk_style_context_add_class (context, "trough");
  else if (strcmp (detail, "spinbutton") == 0)
    ctk_style_context_add_class (context, "spinbutton");
  else if (strcmp (detail, "spinbutton_up") == 0)
    {
      ctk_style_context_add_class (context, "spinbutton");
      ctk_style_context_add_class (context, "button");
      ctk_style_context_set_junction_sides (context, CTK_JUNCTION_BOTTOM);
    }
  else if (strcmp (detail, "spinbutton_down") == 0)
    {
      ctk_style_context_add_class (context, "spinbutton");
      ctk_style_context_add_class (context, "button");
      ctk_style_context_set_junction_sides (context, CTK_JUNCTION_TOP);
    }
  else if ((detail[0] == 'h' || detail[0] == 'v') &&
           strncmp (&detail[1], "scrollbar_", 10) == 0)
    {
      ctk_style_context_add_class (context, "button");
      ctk_style_context_add_class (context, "scrollbar");
    }
  else if (strcmp (detail, "slider") == 0)
    {
      ctk_style_context_add_class (context, "slider");
      ctk_style_context_add_class (context, "scrollbar");
    }
  else if (strcmp (detail, "vscale") == 0 ||
           strcmp (detail, "hscale") == 0)
    {
      ctk_style_context_add_class (context, "slider");
      ctk_style_context_add_class (context, "scale");
    }
  else if (strcmp (detail, "menuitem") == 0)
    {
      ctk_style_context_add_class (context, "menuitem");
      ctk_style_context_add_class (context, "menu");
    }
  else if (strcmp (detail, "menu") == 0)
    {
      ctk_style_context_add_class (context, "popup");
      ctk_style_context_add_class (context, "menu");
    }
  else if (strcmp (detail, "accellabel") == 0)
    ctk_style_context_add_class (context, "accelerator");
  else if (strcmp (detail, "menubar") == 0)
    ctk_style_context_add_class (context, "menubar");
  else if (strcmp (detail, "base") == 0)
    ctk_style_context_add_class (context, "background");
  else if (strcmp (detail, "bar") == 0 ||
           strcmp (detail, "progressbar") == 0)
    ctk_style_context_add_class (context, "progressbar");
  else if (strcmp (detail, "toolbar") == 0)
    ctk_style_context_add_class (context, "toolbar");
  else if (strcmp (detail, "handlebox_bin") == 0)
    ctk_style_context_add_class (context, "dock");
  else if (strcmp (detail, "notebook") == 0)
    ctk_style_context_add_class (context, "notebook");
  else if (strcmp (detail, "tab") == 0)
    {
      ctk_style_context_add_class (context, "notebook");
      ctk_style_context_add_region (context, CTK_STYLE_REGION_TAB, 0);
    }
  else if (g_str_has_prefix (detail, "cell"))
    {
      CtkRegionFlags row, col;
      gboolean ruled = FALSE;
      GStrv tokens;
      guint i;

      tokens = g_strsplit (detail, "_", -1);
      row = col = 0;
      i = 0;

      while (tokens[i])
        {
          if (strcmp (tokens[i], "even") == 0)
            row |= CTK_REGION_EVEN;
          else if (strcmp (tokens[i], "odd") == 0)
            row |= CTK_REGION_ODD;
          else if (strcmp (tokens[i], "start") == 0)
            col |= CTK_REGION_FIRST;
          else if (strcmp (tokens[i], "end") == 0)
            col |= CTK_REGION_LAST;
          else if (strcmp (tokens[i], "ruled") == 0)
            ruled = TRUE;
          else if (strcmp (tokens[i], "sorted") == 0)
            col |= CTK_REGION_SORTED;

          i++;
        }

      if (!ruled)
        row &= ~(CTK_REGION_EVEN | CTK_REGION_ODD);

      ctk_style_context_add_class (context, "cell");
      ctk_style_context_add_region (context, "row", row);
      ctk_style_context_add_region (context, "column", col);

      g_strfreev (tokens);
    }
}

static void
ctk_default_draw_hline (CtkStyle     *style,
                        cairo_t       *cr,
                        CtkStateType  state_type,
                        CtkWidget     *widget,
                        const gchar   *detail,
                        gint          x1,
                        gint          x2,
                        gint          y)
{
  CtkStyleContext *context;
  CtkStylePrivate *priv;

  if (widget)
    context = ctk_widget_get_style_context (widget);
  else
    {
      priv = CTK_STYLE_GET_PRIVATE (style);
      context = priv->context;
    }

  ctk_style_context_save (context);

  if (detail)
    transform_detail_string (detail, context);

  cairo_save (cr);

  ctk_render_line (context, cr,
                   x1, y, x2, y);

  cairo_restore (cr);

  ctk_style_context_restore (context);
}


static void
ctk_default_draw_vline (CtkStyle      *style,
                        cairo_t       *cr,
                        CtkStateType  state_type,
                        CtkWidget     *widget,
                        const gchar   *detail,
                        gint          y1,
                        gint          y2,
                        gint          x)
{
  CtkStyleContext *context;
  CtkStylePrivate *priv;

  if (widget)
    context = ctk_widget_get_style_context (widget);
  else
    {
      priv = CTK_STYLE_GET_PRIVATE (style);
      context = priv->context;
    }

  ctk_style_context_save (context);

  if (detail)
    transform_detail_string (detail, context);

  cairo_save (cr);

  ctk_render_line (context, cr,
                   x, y1, x, y2);

  cairo_restore (cr);
  ctk_style_context_restore (context);
}

static void
ctk_default_draw_shadow (CtkStyle      *style,
                         cairo_t       *cr,
                         CtkStateType   state_type,
                         CtkShadowType  shadow_type,
                         CtkWidget     *widget,
                         const gchar   *detail,
                         gint           x,
                         gint           y,
                         gint           width,
                         gint           height)
{
  CtkStyleContext *context;
  CtkStylePrivate *priv;

  if (shadow_type == CTK_SHADOW_NONE)
    return;

  if (widget)
    context = ctk_widget_get_style_context (widget);
  else
    {
      priv = CTK_STYLE_GET_PRIVATE (style);
      context = priv->context;
    }

  ctk_style_context_save (context);

  if (detail)
    transform_detail_string (detail, context);

  cairo_save (cr);

  ctk_render_frame (context, cr,
                    (gdouble) x,
                    (gdouble) y,
                    (gdouble) width,
                    (gdouble) height);

  cairo_restore (cr);
  ctk_style_context_restore (context);
}

static void
draw_arrow (cairo_t       *cr,
	    CdkColor      *color,
	    CtkArrowType   arrow_type,
	    gint           x,
	    gint           y,
	    gint           width,
	    gint           height)
{
  cdk_cairo_set_source_color (cr, color);
  cairo_save (cr);
    
  if (arrow_type == CTK_ARROW_DOWN)
    {
      cairo_move_to (cr, x,              y);
      cairo_line_to (cr, x + width,      y);
      cairo_line_to (cr, x + width / 2., y + height);
    }
  else if (arrow_type == CTK_ARROW_UP)
    {
      cairo_move_to (cr, x,              y + height);
      cairo_line_to (cr, x + width / 2., y);
      cairo_line_to (cr, x + width,      y + height);
    }
  else if (arrow_type == CTK_ARROW_LEFT)
    {
      cairo_move_to (cr, x + width,      y);
      cairo_line_to (cr, x + width,      y + height);
      cairo_line_to (cr, x,              y + height / 2.);
    }
  else if (arrow_type == CTK_ARROW_RIGHT)
    {
      cairo_move_to (cr, x,              y);
      cairo_line_to (cr, x + width,      y + height / 2.);
      cairo_line_to (cr, x,              y + height);
    }

  cairo_close_path (cr);
  cairo_fill (cr);

  cairo_restore (cr);
}

static void
ctk_default_draw_arrow (CtkStyle      *style,
			cairo_t       *cr,
			CtkStateType   state,
			CtkShadowType  shadow,
			CtkWidget     *widget,
			const gchar   *detail,
			CtkArrowType   arrow_type,
			gboolean       fill,
			gint           x,
			gint           y,
			gint           width,
			gint           height)
{
  CtkStyleContext *context;
  CtkStylePrivate *priv;
  CtkStateFlags flags = 0;
  gdouble angle, size;

  if (arrow_type == CTK_ARROW_NONE)
    return;

  if (widget)
    context = ctk_widget_get_style_context (widget);
  else
    {
      priv = CTK_STYLE_GET_PRIVATE (style);
      context = priv->context;
    }

  ctk_style_context_save (context);

  if (detail)
    transform_detail_string (detail, context);

  switch (arrow_type)
    {
    case CTK_ARROW_UP:
      angle = 0;
      size = width;
      break;
    case CTK_ARROW_RIGHT:
      angle = G_PI / 2;
      size = height;
      break;
    case CTK_ARROW_DOWN:
      angle = G_PI;
      size = width;
      break;
    case CTK_ARROW_LEFT:
      angle = 3 * (G_PI / 2);
      size = height;
      break;
    default:
      g_assert_not_reached ();
    }

  switch (state)
    {
    case CTK_STATE_PRELIGHT:
      flags |= CTK_STATE_FLAG_PRELIGHT;
      break;
    case CTK_STATE_SELECTED:
      flags |= CTK_STATE_FLAG_SELECTED;
      break;
    case CTK_STATE_INSENSITIVE:
      flags |= CTK_STATE_FLAG_INSENSITIVE;
      break;
    case CTK_STATE_ACTIVE:
      flags |= CTK_STATE_FLAG_ACTIVE;
      break;
    default:
      break;
    }

  ctk_style_context_set_state (context, flags);

  cairo_save (cr);

  ctk_render_arrow (context,
                    cr, angle,
                    (gdouble) x,
                    (gdouble) y,
                    size);

  cairo_restore (cr);
  ctk_style_context_restore (context);
}

static void
ctk_default_draw_diamond (CtkStyle      *style,
                          cairo_t       *cr,
                          CtkStateType   state_type,
                          CtkShadowType  shadow_type,
                          CtkWidget     *widget,
                          const gchar   *detail,
                          gint           x,
                          gint           y,
                          gint           width,
                          gint           height)
{
  gint half_width;
  gint half_height;
  CdkColor *outer_nw = NULL;
  CdkColor *outer_ne = NULL;
  CdkColor *outer_sw = NULL;
  CdkColor *outer_se = NULL;
  CdkColor *middle_nw = NULL;
  CdkColor *middle_ne = NULL;
  CdkColor *middle_sw = NULL;
  CdkColor *middle_se = NULL;
  CdkColor *inner_nw = NULL;
  CdkColor *inner_ne = NULL;
  CdkColor *inner_sw = NULL;
  CdkColor *inner_se = NULL;
  
  half_width = width / 2;
  half_height = height / 2;
  
  switch (shadow_type)
    {
    case CTK_SHADOW_IN:
      inner_sw = inner_se = &style->bg[state_type];
      middle_sw = middle_se = &style->light[state_type];
      outer_sw = outer_se = &style->light[state_type];
      inner_nw = inner_ne = &style->black;
      middle_nw = middle_ne = &style->dark[state_type];
      outer_nw = outer_ne = &style->dark[state_type];
      break;
          
    case CTK_SHADOW_OUT:
      inner_sw = inner_se = &style->dark[state_type];
      middle_sw = middle_se = &style->dark[state_type];
      outer_sw = outer_se = &style->black;
      inner_nw = inner_ne = &style->bg[state_type];
      middle_nw = middle_ne = &style->light[state_type];
      outer_nw = outer_ne = &style->light[state_type];
      break;

    case CTK_SHADOW_ETCHED_IN:
      inner_sw = inner_se = &style->bg[state_type];
      middle_sw = middle_se = &style->dark[state_type];
      outer_sw = outer_se = &style->light[state_type];
      inner_nw = inner_ne = &style->bg[state_type];
      middle_nw = middle_ne = &style->light[state_type];
      outer_nw = outer_ne = &style->dark[state_type];
      break;

    case CTK_SHADOW_ETCHED_OUT:
      inner_sw = inner_se = &style->bg[state_type];
      middle_sw = middle_se = &style->light[state_type];
      outer_sw = outer_se = &style->dark[state_type];
      inner_nw = inner_ne = &style->bg[state_type];
      middle_nw = middle_ne = &style->dark[state_type];
      outer_nw = outer_ne = &style->light[state_type];
      break;
      
    default:

      break;
    }

  if (inner_sw)
    {
      _cairo_draw_line (cr, inner_sw,
                        x + 2, y + half_height,
                        x + half_width, y + height - 2);
      _cairo_draw_line (cr, inner_se,
                        x + half_width, y + height - 2,
                        x + width - 2, y + half_height);
      _cairo_draw_line (cr, middle_sw,
                        x + 1, y + half_height,
                        x + half_width, y + height - 1);
      _cairo_draw_line (cr, middle_se,
                        x + half_width, y + height - 1,
                        x + width - 1, y + half_height);
      _cairo_draw_line (cr, outer_sw,
                        x, y + half_height,
                        x + half_width, y + height);
      _cairo_draw_line (cr, outer_se,
                        x + half_width, y + height,
                        x + width, y + half_height);
  
      _cairo_draw_line (cr, inner_nw,
                        x + 2, y + half_height,
                        x + half_width, y + 2);
      _cairo_draw_line (cr, inner_ne,
                        x + half_width, y + 2,
                        x + width - 2, y + half_height);
      _cairo_draw_line (cr, middle_nw,
                        x + 1, y + half_height,
                        x + half_width, y + 1);
      _cairo_draw_line (cr, middle_ne,
                        x + half_width, y + 1,
                        x + width - 1, y + half_height);
      _cairo_draw_line (cr, outer_nw,
                        x, y + half_height,
                        x + half_width, y);
      _cairo_draw_line (cr, outer_ne,
                        x + half_width, y,
                        x + width, y + half_height);
    }
}

static void
option_menu_get_props (CtkWidget      *widget,
		       CtkRequisition *indicator_size,
		       CtkBorder      *indicator_spacing)
{
  CtkRequisition *tmp_size = NULL;
  CtkBorder *tmp_spacing = NULL;

  if (tmp_size)
    {
      *indicator_size = *tmp_size;
      ctk_requisition_free (tmp_size);
    }
  else
    *indicator_size = default_option_indicator_size;

  if (tmp_spacing)
    {
      *indicator_spacing = *tmp_spacing;
      ctk_border_free (tmp_spacing);
    }
  else
    *indicator_spacing = default_option_indicator_spacing;
}

static void 
ctk_default_draw_box (CtkStyle      *style,
		      cairo_t       *cr,
		      CtkStateType   state_type,
		      CtkShadowType  shadow_type,
		      CtkWidget     *widget,
		      const gchar   *detail,
		      gint           x,
		      gint           y,
		      gint           width,
		      gint           height)
{
  CtkStyleContext *context;
  CtkStylePrivate *priv;
  CtkStateFlags flags = 0;

  if (widget)
    context = ctk_widget_get_style_context (widget);
  else
    {
      priv = CTK_STYLE_GET_PRIVATE (style);
      context = priv->context;
    }

  ctk_style_context_save (context);

  if (detail)
    transform_detail_string (detail, context);

  switch (state_type)
    {
    case CTK_STATE_ACTIVE:
      flags |= CTK_STATE_FLAG_ACTIVE;
      break;
    case CTK_STATE_PRELIGHT:
      flags |= CTK_STATE_FLAG_PRELIGHT;
      break;
    case CTK_STATE_SELECTED:
      flags |= CTK_STATE_FLAG_SELECTED;
      break;
    case CTK_STATE_INSENSITIVE:
      flags |= CTK_STATE_FLAG_INSENSITIVE;
      break;
    default:
      break;
    }

  if (shadow_type == CTK_SHADOW_IN)
    flags |= CTK_STATE_FLAG_ACTIVE;

  ctk_style_context_set_state (context, flags);

  cairo_save (cr);

  ctk_render_background (context, cr, x, y, width, height);

  if (shadow_type != CTK_SHADOW_NONE)
    ctk_render_frame (context, cr, x, y, width, height);

  cairo_restore (cr);
  ctk_style_context_restore (context);
}

static void 
ctk_default_draw_flat_box (CtkStyle      *style,
                           cairo_t       *cr,
                           CtkStateType   state_type,
                           CtkShadowType  shadow_type,
                           CtkWidget     *widget,
                           const gchar   *detail,
                           gint           x,
                           gint           y,
                           gint           width,
                           gint           height)
{
  CtkStyleContext *context;
  CtkStylePrivate *priv;
  CtkStateFlags flags = 0;

  if (widget)
    context = ctk_widget_get_style_context (widget);
  else
    {
      priv = CTK_STYLE_GET_PRIVATE (style);
      context = priv->context;
    }

  ctk_style_context_save (context);

  if (detail)
    transform_detail_string (detail, context);

  switch (state_type)
    {
    case CTK_STATE_PRELIGHT:
      flags |= CTK_STATE_FLAG_PRELIGHT;
      break;
    case CTK_STATE_SELECTED:
      flags |= CTK_STATE_FLAG_SELECTED;
      break;
    case CTK_STATE_INSENSITIVE:
      flags |= CTK_STATE_FLAG_INSENSITIVE;
      break;
    case CTK_STATE_ACTIVE:
      flags |= CTK_STATE_FLAG_ACTIVE;
      break;
    case CTK_STATE_FOCUSED:
      flags |= CTK_STATE_FLAG_FOCUSED;
      break;
    default:
      break;
    }

  ctk_style_context_set_state (context, flags);

  cairo_save (cr);

  ctk_render_background (context, cr,
                         (gdouble) x,
                         (gdouble) y,
                         (gdouble) width,
                         (gdouble) height);

  cairo_restore (cr);
  ctk_style_context_restore (context);
}

static void 
ctk_default_draw_check (CtkStyle      *style,
			cairo_t       *cr,
			CtkStateType   state_type,
			CtkShadowType  shadow_type,
			CtkWidget     *widget,
			const gchar   *detail,
			gint           x,
			gint           y,
			gint           width,
			gint           height)
{
  CtkStyleContext *context;
  CtkStylePrivate *priv;
  CtkStateFlags flags = 0;

  if (widget)
    context = ctk_widget_get_style_context (widget);
  else
    {
      priv = CTK_STYLE_GET_PRIVATE (style);
      context = priv->context;
    }

  ctk_style_context_save (context);

  if (detail)
    transform_detail_string (detail, context);

  switch (state_type)
    {
    case CTK_STATE_PRELIGHT:
      flags |= CTK_STATE_FLAG_PRELIGHT;
      break;
    case CTK_STATE_SELECTED:
      flags |= CTK_STATE_FLAG_SELECTED;
      break;
    case CTK_STATE_INSENSITIVE:
      flags |= CTK_STATE_FLAG_INSENSITIVE;
      break;
    default:
      break;
    }

  if (shadow_type == CTK_SHADOW_IN)
    flags |= CTK_STATE_FLAG_ACTIVE;
  else if (shadow_type == CTK_SHADOW_ETCHED_IN)
    flags |= CTK_STATE_FLAG_INCONSISTENT;

  ctk_style_context_set_state (context, flags);

  cairo_save (cr);

  ctk_render_check (context,
                    cr, x, y,
                    width, height);

  cairo_restore (cr);
  ctk_style_context_restore (context);
}

static void 
ctk_default_draw_option (CtkStyle      *style,
			 cairo_t       *cr,
			 CtkStateType   state_type,
			 CtkShadowType  shadow_type,
			 CtkWidget     *widget,
			 const gchar   *detail,
			 gint           x,
			 gint           y,
			 gint           width,
			 gint           height)
{
  CtkStyleContext *context;
  CtkStylePrivate *priv;
  CtkStateFlags flags = 0;

  if (widget)
    context = ctk_widget_get_style_context (widget);
  else
    {
      priv = CTK_STYLE_GET_PRIVATE (style);
      context = priv->context;
    }

  ctk_style_context_save (context);

  if (detail)
    transform_detail_string (detail, context);

  switch (state_type)
    {
    case CTK_STATE_PRELIGHT:
      flags |= CTK_STATE_FLAG_PRELIGHT;
      break;
    case CTK_STATE_SELECTED:
      flags |= CTK_STATE_FLAG_SELECTED;
      break;
    case CTK_STATE_INSENSITIVE:
      flags |= CTK_STATE_FLAG_INSENSITIVE;
      break;
    default:
      break;
    }

  if (shadow_type == CTK_SHADOW_IN)
    flags |= CTK_STATE_FLAG_ACTIVE;
  else if (shadow_type == CTK_SHADOW_ETCHED_IN)
    flags |= CTK_STATE_FLAG_INCONSISTENT;

  ctk_style_context_set_state (context, flags);

  cairo_save (cr);
  ctk_render_option (context, cr,
                     (gdouble) x,
                     (gdouble) y,
                     (gdouble) width,
                     (gdouble) height);

  cairo_restore (cr);
  ctk_style_context_restore (context);
}

static void
ctk_default_draw_tab (CtkStyle      *style,
		      cairo_t       *cr,
		      CtkStateType   state_type,
		      CtkShadowType  shadow_type,
		      CtkWidget     *widget,
		      const gchar   *detail,
		      gint           x,
		      gint           y,
		      gint           width,
		      gint           height)
{
#define ARROW_SPACE 4

  CtkRequisition indicator_size;
  CtkBorder indicator_spacing;
  gint arrow_height;

  option_menu_get_props (widget, &indicator_size, &indicator_spacing);

  indicator_size.width += (indicator_size.width % 2) - 1;
  arrow_height = indicator_size.width / 2 + 1;

  x += (width - indicator_size.width) / 2;
  y += (height - (2 * arrow_height + ARROW_SPACE)) / 2;

  if (state_type == CTK_STATE_INSENSITIVE)
    {
      draw_arrow (cr, &style->white,
		  CTK_ARROW_UP, x + 1, y + 1,
		  indicator_size.width, arrow_height);
      
      draw_arrow (cr, &style->white,
		  CTK_ARROW_DOWN, x + 1, y + arrow_height + ARROW_SPACE + 1,
		  indicator_size.width, arrow_height);
    }
  
  draw_arrow (cr, &style->fg[state_type],
	      CTK_ARROW_UP, x, y,
	      indicator_size.width, arrow_height);
  
  
  draw_arrow (cr, &style->fg[state_type],
	      CTK_ARROW_DOWN, x, y + arrow_height + ARROW_SPACE,
	      indicator_size.width, arrow_height);
}

static void 
ctk_default_draw_shadow_gap (CtkStyle       *style,
                             cairo_t        *cr,
                             CtkStateType    state_type,
                             CtkShadowType   shadow_type,
                             CtkWidget      *widget,
                             const gchar    *detail,
                             gint            x,
                             gint            y,
                             gint            width,
                             gint            height,
                             CtkPositionType gap_side,
                             gint            gap_x,
                             gint            gap_width)
{
  CtkStyleContext *context;
  CtkStylePrivate *priv;
  CtkStateFlags flags = 0;

  if (shadow_type == CTK_SHADOW_NONE)
    return;

  if (widget)
    context = ctk_widget_get_style_context (widget);
  else
    {
      priv = CTK_STYLE_GET_PRIVATE (style);
      context = priv->context;
    }

  ctk_style_context_save (context);

  if (detail)
    transform_detail_string (detail, context);

  switch (state_type)
    {
    case CTK_STATE_ACTIVE:
      flags |= CTK_STATE_FLAG_ACTIVE;
      break;
    case CTK_STATE_PRELIGHT:
      flags |= CTK_STATE_FLAG_PRELIGHT;
      break;
    case CTK_STATE_SELECTED:
      flags |= CTK_STATE_FLAG_SELECTED;
      break;
    case CTK_STATE_INSENSITIVE:
      flags |= CTK_STATE_FLAG_INSENSITIVE;
      break;
    default:
      break;
    }

  ctk_style_context_set_state (context, flags);

  cairo_save (cr);
  ctk_render_frame_gap (context, cr,
                        (gdouble) x,
                        (gdouble) y,
                        (gdouble) width,
                        (gdouble) height,
                        gap_side,
                        (gdouble) gap_x,
                        (gdouble) gap_x + gap_width);

  cairo_restore (cr);
  ctk_style_context_restore (context);
}

static void 
ctk_default_draw_box_gap (CtkStyle       *style,
                          cairo_t        *cr,
                          CtkStateType    state_type,
                          CtkShadowType   shadow_type,
                          CtkWidget      *widget,
                          const gchar    *detail,
                          gint            x,
                          gint            y,
                          gint            width,
                          gint            height,
                          CtkPositionType gap_side,
                          gint            gap_x,
                          gint            gap_width)
{
  CtkStyleContext *context;
  CtkStylePrivate *priv;
  CtkStateFlags flags = 0;

  if (widget)
    context = ctk_widget_get_style_context (widget);
  else
    {
      priv = CTK_STYLE_GET_PRIVATE (style);
      context = priv->context;
    }

  ctk_style_context_save (context);

  if (detail)
    transform_detail_string (detail, context);

  switch (state_type)
    {
    case CTK_STATE_ACTIVE:
      flags |= CTK_STATE_FLAG_ACTIVE;
      break;
    case CTK_STATE_PRELIGHT:
      flags |= CTK_STATE_FLAG_PRELIGHT;
      break;
    case CTK_STATE_SELECTED:
      flags |= CTK_STATE_FLAG_SELECTED;
      break;
    case CTK_STATE_INSENSITIVE:
      flags |= CTK_STATE_FLAG_INSENSITIVE;
      break;
    default:
      break;
    }

  ctk_style_context_set_state (context, flags);

  cairo_save (cr);
  ctk_render_background (context, cr,
                         (gdouble) x,
                         (gdouble) y,
                         (gdouble) width,
                         (gdouble) height);


  if (shadow_type != CTK_SHADOW_NONE)
    ctk_render_frame_gap (context, cr,
			  (gdouble) x,
			  (gdouble) y,
			  (gdouble) width,
			  (gdouble) height,
			  gap_side,
			  (gdouble) gap_x,
			  (gdouble) gap_x + gap_width);
  
  cairo_restore (cr);
  ctk_style_context_restore (context);
}

static void 
ctk_default_draw_extension (CtkStyle       *style,
                            cairo_t        *cr,
                            CtkStateType    state_type,
                            CtkShadowType   shadow_type,
                            CtkWidget      *widget,
                            const gchar    *detail,
                            gint            x,
                            gint            y,
                            gint            width,
                            gint            height,
                            CtkPositionType gap_side)
{
  CtkStyleContext *context;
  CtkStylePrivate *priv;
  CtkStateFlags flags = 0;

  if (widget)
    context = ctk_widget_get_style_context (widget);
  else
    {
      priv = CTK_STYLE_GET_PRIVATE (style);
      context = priv->context;
    }

  ctk_style_context_save (context);

  if (detail)
    transform_detail_string (detail, context);

  switch (state_type)
    {
    case CTK_STATE_ACTIVE:
      flags |= CTK_STATE_FLAG_ACTIVE;
      break;
    case CTK_STATE_PRELIGHT:
      flags |= CTK_STATE_FLAG_PRELIGHT;
      break;
    case CTK_STATE_SELECTED:
      flags |= CTK_STATE_FLAG_SELECTED;
      break;
    case CTK_STATE_INSENSITIVE:
      flags |= CTK_STATE_FLAG_INSENSITIVE;
      break;
    default:
      break;
    }

  ctk_style_context_set_state (context, flags);

  cairo_save (cr);

  ctk_render_extension (context, cr,
                        (gdouble) x,
                        (gdouble) y,
                        (gdouble) width,
                        (gdouble) height,
                        gap_side);

  cairo_restore (cr);
  ctk_style_context_restore (context);
}

static void 
ctk_default_draw_focus (CtkStyle      *style,
			cairo_t       *cr,
			CtkStateType   state_type,
			CtkWidget     *widget,
			const gchar   *detail,
			gint           x,
			gint           y,
			gint           width,
			gint           height)
{
  CtkStyleContext *context;
  CtkStylePrivate *priv;

  if (widget)
    context = ctk_widget_get_style_context (widget);
  else
    {
      priv = CTK_STYLE_GET_PRIVATE (style);
      context = priv->context;
    }

  ctk_style_context_save (context);

  if (detail)
    transform_detail_string (detail, context);

  cairo_save (cr);

  ctk_render_focus (context, cr,
                    (gdouble) x,
                    (gdouble) y,
                    (gdouble) width,
                    (gdouble) height);

  cairo_restore (cr);
  ctk_style_context_restore (context);
}

static void 
ctk_default_draw_slider (CtkStyle      *style,
                         cairo_t       *cr,
                         CtkStateType   state_type,
                         CtkShadowType  shadow_type,
                         CtkWidget     *widget,
                         const gchar   *detail,
                         gint           x,
                         gint           y,
                         gint           width,
                         gint           height,
                         CtkOrientation orientation)
{
  CtkStyleContext *context;
  CtkStylePrivate *priv;
  CtkStateFlags flags = 0;

  if (widget)
    context = ctk_widget_get_style_context (widget);
  else
    {
      priv = CTK_STYLE_GET_PRIVATE (style);
      context = priv->context;
    }

  ctk_style_context_save (context);

  if (detail)
    transform_detail_string (detail, context);

  switch (state_type)
    {
    case CTK_STATE_PRELIGHT:
      flags |= CTK_STATE_FLAG_PRELIGHT;
      break;
    case CTK_STATE_SELECTED:
      flags |= CTK_STATE_FLAG_SELECTED;
      break;
    case CTK_STATE_INSENSITIVE:
      flags |= CTK_STATE_FLAG_INSENSITIVE;
      break;
    default:
      break;
    }

  ctk_style_context_set_state (context, flags);

  cairo_save (cr);

  ctk_render_slider (context, cr,  x, y, width, height, orientation);

  cairo_restore (cr);
  ctk_style_context_restore (context);
}

static void 
ctk_default_draw_handle (CtkStyle      *style,
			 cairo_t       *cr,
			 CtkStateType   state_type,
			 CtkShadowType  shadow_type,
			 CtkWidget     *widget,
			 const gchar   *detail,
			 gint           x,
			 gint           y,
			 gint           width,
			 gint           height,
			 CtkOrientation orientation)
{
  CtkStyleContext *context;
  CtkStylePrivate *priv;
  CtkStateFlags flags = 0;

  if (widget)
    context = ctk_widget_get_style_context (widget);
  else
    {
      priv = CTK_STYLE_GET_PRIVATE (style);
      context = priv->context;
    }

  ctk_style_context_save (context);

  if (detail)
    transform_detail_string (detail, context);

  switch (state_type)
    {
    case CTK_STATE_PRELIGHT:
      flags |= CTK_STATE_FLAG_PRELIGHT;
      break;
    case CTK_STATE_SELECTED:
      flags |= CTK_STATE_FLAG_SELECTED;
      break;
    case CTK_STATE_INSENSITIVE:
      flags |= CTK_STATE_FLAG_INSENSITIVE;
      break;
    default:
      break;
    }

  ctk_style_context_set_state (context, flags);

  cairo_save (cr);

  ctk_render_handle (context, cr,
                     (gdouble) x,
                     (gdouble) y,
                     (gdouble) width,
                     (gdouble) height);

  cairo_restore (cr);
  ctk_style_context_restore (context);
}

static void
ctk_default_draw_expander (CtkStyle        *style,
                           cairo_t         *cr,
                           CtkStateType     state_type,
                           CtkWidget       *widget,
                           const gchar     *detail,
                           gint             x,
                           gint             y,
			   CtkExpanderStyle expander_style)
{
  CtkStyleContext *context;
  CtkStylePrivate *priv;
  CtkStateFlags flags = 0;
  gint size;

  if (widget)
    context = ctk_widget_get_style_context (widget);
  else
    {
      priv = CTK_STYLE_GET_PRIVATE (style);
      context = priv->context;
    }

  ctk_style_context_save (context);

  if (detail)
    transform_detail_string (detail, context);

  ctk_style_context_add_class (context, "expander");

  switch (state_type)
    {
    case CTK_STATE_PRELIGHT:
      flags |= CTK_STATE_FLAG_PRELIGHT;
      break;
    case CTK_STATE_SELECTED:
      flags |= CTK_STATE_FLAG_SELECTED;
      break;
    case CTK_STATE_INSENSITIVE:
      flags |= CTK_STATE_FLAG_INSENSITIVE;
      break;
    default:
      break;
    }

  if (widget &&
      ctk_widget_class_find_style_property (CTK_WIDGET_GET_CLASS (widget),
                                            "expander-size"))
    ctk_widget_style_get (widget, "expander-size", &size, NULL);
  else
    size = 12;

  if (expander_style == CTK_EXPANDER_EXPANDED)
    flags |= CTK_STATE_FLAG_ACTIVE;

  ctk_style_context_set_state (context, flags);

  cairo_save (cr);

  ctk_render_expander (context, cr,
                       (gdouble) x - (size / 2),
                       (gdouble) y - (size / 2),
                       (gdouble) size,
                       (gdouble) size);

  cairo_restore (cr);
  ctk_style_context_restore (context);
}

static void
ctk_default_draw_layout (CtkStyle        *style,
                         cairo_t         *cr,
                         CtkStateType     state_type,
			 gboolean         use_text,
                         CtkWidget       *widget,
                         const gchar     *detail,
                         gint             x,
                         gint             y,
                         PangoLayout     *layout)
{
  CtkStyleContext *context;
  CtkStylePrivate *priv;
  CtkStateFlags flags = 0;

  if (widget)
    context = ctk_widget_get_style_context (widget);
  else
    {
      priv = CTK_STYLE_GET_PRIVATE (style);
      context = priv->context;
    }

  ctk_style_context_save (context);

  if (detail)
    transform_detail_string (detail, context);

  switch (state_type)
    {
    case CTK_STATE_PRELIGHT:
      flags |= CTK_STATE_FLAG_PRELIGHT;
      break;
    case CTK_STATE_SELECTED:
      flags |= CTK_STATE_FLAG_SELECTED;
      break;
    case CTK_STATE_INSENSITIVE:
      flags |= CTK_STATE_FLAG_INSENSITIVE;
      break;
    default:
      break;
    }

  ctk_style_context_set_state (context, flags);

  cairo_save (cr);

  ctk_render_layout (context, cr,
                     (gdouble) x,
                     (gdouble) y,
                     layout);

  cairo_restore (cr);
  ctk_style_context_restore (context);
}

static void
ctk_default_draw_resize_grip (CtkStyle       *style,
                              cairo_t        *cr,
                              CtkStateType    state_type,
                              CtkWidget      *widget,
                              const gchar    *detail,
                              CdkWindowEdge   edge,
                              gint            x,
                              gint            y,
                              gint            width,
                              gint            height)
{
  CtkStyleContext *context;
  CtkStylePrivate *priv;
  CtkStateFlags flags = 0;
  CtkJunctionSides sides = 0;

  if (widget)
    context = ctk_widget_get_style_context (widget);
  else
    {
      priv = CTK_STYLE_GET_PRIVATE (style);
      context = priv->context;
    }

  ctk_style_context_save (context);

  if (detail)
    transform_detail_string (detail, context);

  ctk_style_context_add_class (context, "grip");

  switch (state_type)
    {
    case CTK_STATE_PRELIGHT:
      flags |= CTK_STATE_FLAG_PRELIGHT;
      break;
    case CTK_STATE_SELECTED:
      flags |= CTK_STATE_FLAG_SELECTED;
      break;
    case CTK_STATE_INSENSITIVE:
      flags |= CTK_STATE_FLAG_INSENSITIVE;
      break;
    default:
      break;
    }

  ctk_style_context_set_state (context, flags);

  switch (edge)
    {
    case CDK_WINDOW_EDGE_NORTH_WEST:
      sides = CTK_JUNCTION_CORNER_TOPLEFT;
      break;
    case CDK_WINDOW_EDGE_NORTH:
      sides = CTK_JUNCTION_TOP;
      break;
    case CDK_WINDOW_EDGE_NORTH_EAST:
      sides = CTK_JUNCTION_CORNER_TOPRIGHT;
      break;
    case CDK_WINDOW_EDGE_WEST:
      sides = CTK_JUNCTION_LEFT;
      break;
    case CDK_WINDOW_EDGE_EAST:
      sides = CTK_JUNCTION_RIGHT;
      break;
    case CDK_WINDOW_EDGE_SOUTH_WEST:
      sides = CTK_JUNCTION_CORNER_BOTTOMLEFT;
      break;
    case CDK_WINDOW_EDGE_SOUTH:
      sides = CTK_JUNCTION_BOTTOM;
      break;
    case CDK_WINDOW_EDGE_SOUTH_EAST:
      sides = CTK_JUNCTION_CORNER_BOTTOMRIGHT;
      break;
    }

  ctk_style_context_set_junction_sides (context, sides);

  cairo_save (cr);

  ctk_render_handle (context, cr,
                     (gdouble) x,
                     (gdouble) y,
                     (gdouble) width,
                     (gdouble) height);

  cairo_restore (cr);
  ctk_style_context_restore (context);
}

static void
ctk_default_draw_spinner (CtkStyle     *style,
                          cairo_t      *cr,
                          CtkStateType  state_type,
                          CtkWidget    *widget,
                          const gchar  *detail,
                          guint         step,
                          gint          x,
                          gint          y,
                          gint          width,
                          gint          height)
{
  CdkColor *color;
  guint num_steps;
  gdouble dx, dy;
  gdouble radius;
  gdouble half;
  gint i;
  guint real_step;

  num_steps = 12;
  real_step = step % num_steps;

  /* set a clip region for the expose event */
  cairo_rectangle (cr, x, y, width, height);
  cairo_clip (cr);

  cairo_translate (cr, x, y);

  /* draw clip region */
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  color = &style->fg[state_type];
  dx = width / 2;
  dy = height / 2;
  radius = MIN (width / 2, height / 2);
  half = num_steps / 2;

  for (i = 0; i < num_steps; i++)
    {
      gint inset = 0.7 * radius;

      /* transparency is a function of time and intial value */
      gdouble t = (gdouble) ((i + num_steps - real_step)
                             % num_steps) / num_steps;

      cairo_save (cr);

      cairo_set_source_rgba (cr,
                             color->red / 65535.,
                             color->green / 65535.,
                             color->blue / 65535.,
                             t);

      cairo_set_line_width (cr, 2.0);
      cairo_move_to (cr,
                     dx + (radius - inset) * cos (i * G_PI / half),
                     dy + (radius - inset) * sin (i * G_PI / half));
      cairo_line_to (cr,
                     dx + radius * cos (i * G_PI / half),
                     dy + radius * sin (i * G_PI / half));
      cairo_stroke (cr);

      cairo_restore (cr);
    }
}

void
_ctk_style_shade (const CdkColor *a,
                  CdkColor       *b,
                  gdouble         k)
{
  gdouble red;
  gdouble green;
  gdouble blue;
  
  red = (gdouble) a->red / 65535.0;
  green = (gdouble) a->green / 65535.0;
  blue = (gdouble) a->blue / 65535.0;
  
  rgb_to_hls (&red, &green, &blue);
  
  green *= k;
  if (green > 1.0)
    green = 1.0;
  else if (green < 0.0)
    green = 0.0;
  
  blue *= k;
  if (blue > 1.0)
    blue = 1.0;
  else if (blue < 0.0)
    blue = 0.0;
  
  hls_to_rgb (&red, &green, &blue);
  
  b->red = red * 65535.0;
  b->green = green * 65535.0;
  b->blue = blue * 65535.0;
}

static void
rgb_to_hls (gdouble *r,
            gdouble *g,
            gdouble *b)
{
  gdouble min;
  gdouble max;
  gdouble red;
  gdouble green;
  gdouble blue;
  gdouble h, l, s;
  gdouble delta;
  
  red = *r;
  green = *g;
  blue = *b;
  
  if (red > green)
    {
      if (red > blue)
        max = red;
      else
        max = blue;
      
      if (green < blue)
        min = green;
      else
        min = blue;
    }
  else
    {
      if (green > blue)
        max = green;
      else
        max = blue;
      
      if (red < blue)
        min = red;
      else
        min = blue;
    }
  
  l = (max + min) / 2;
  s = 0;
  h = 0;
  
  if (max != min)
    {
      if (l <= 0.5)
        s = (max - min) / (max + min);
      else
        s = (max - min) / (2 - max - min);
      
      delta = max -min;
      if (red == max)
        h = (green - blue) / delta;
      else if (green == max)
        h = 2 + (blue - red) / delta;
      else if (blue == max)
        h = 4 + (red - green) / delta;
      
      h *= 60;
      if (h < 0.0)
        h += 360;
    }
  
  *r = h;
  *g = l;
  *b = s;
}

static void
hls_to_rgb (gdouble *h,
            gdouble *l,
            gdouble *s)
{
  gdouble hue;
  gdouble lightness;
  gdouble saturation;
  gdouble m1, m2;
  gdouble r, g, b;
  
  lightness = *l;
  saturation = *s;
  
  if (lightness <= 0.5)
    m2 = lightness * (1 + saturation);
  else
    m2 = lightness + saturation - lightness * saturation;
  m1 = 2 * lightness - m2;
  
  if (saturation == 0)
    {
      *h = lightness;
      *l = lightness;
      *s = lightness;
    }
  else
    {
      hue = *h + 120;
      while (hue > 360)
        hue -= 360;
      while (hue < 0)
        hue += 360;
      
      if (hue < 60)
        r = m1 + (m2 - m1) * hue / 60;
      else if (hue < 180)
        r = m2;
      else if (hue < 240)
        r = m1 + (m2 - m1) * (240 - hue) / 60;
      else
        r = m1;
      
      hue = *h;
      while (hue > 360)
        hue -= 360;
      while (hue < 0)
        hue += 360;
      
      if (hue < 60)
        g = m1 + (m2 - m1) * hue / 60;
      else if (hue < 180)
        g = m2;
      else if (hue < 240)
        g = m1 + (m2 - m1) * (240 - hue) / 60;
      else
        g = m1;
      
      hue = *h - 120;
      while (hue > 360)
        hue -= 360;
      while (hue < 0)
        hue += 360;
      
      if (hue < 60)
        b = m1 + (m2 - m1) * hue / 60;
      else if (hue < 180)
        b = m2;
      else if (hue < 240)
        b = m1 + (m2 - m1) * (240 - hue) / 60;
      else
        b = m1;
      
      *h = r;
      *l = g;
      *s = b;
    }
}


/**
 * ctk_paint_hline:
 * @style: a #CtkStyle
 * @cr: a #caio_t
 * @state_type: a state
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @x1: the starting x coordinate
 * @x2: the ending x coordinate
 * @y: the y coordinate
 *
 * Draws a horizontal line from (@x1, @y) to (@x2, @y) in @cr
 * using the given style and state.
 *
 * Deprecated:3.0: Use ctk_render_line() instead
 **/
void
ctk_paint_hline (CtkStyle           *style,
                 cairo_t            *cr,
                 CtkStateType        state_type,
                 CtkWidget          *widget,
                 const gchar        *detail,
                 gint                x1,
                 gint                x2,
                 gint                y)
{
  g_return_if_fail (CTK_IS_STYLE (style));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (CTK_STYLE_GET_CLASS (style)->draw_hline != NULL);

  cairo_save (cr);

  CTK_STYLE_GET_CLASS (style)->draw_hline (style, cr, state_type,
                                           widget, detail,
                                           x1, x2, y);

  cairo_restore (cr);
}

/**
 * ctk_paint_vline:
 * @style: a #CtkStyle
 * @cr: a #cairo_t
 * @state_type: a state
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @y1_: the starting y coordinate
 * @y2_: the ending y coordinate
 * @x: the x coordinate
 *
 * Draws a vertical line from (@x, @y1_) to (@x, @y2_) in @cr
 * using the given style and state.
 *
 * Deprecated:3.0: Use ctk_render_line() instead
 */
void
ctk_paint_vline (CtkStyle           *style,
                 cairo_t            *cr,
                 CtkStateType        state_type,
                 CtkWidget          *widget,
                 const gchar        *detail,
                 gint                y1_,
                 gint                y2_,
                 gint                x)
{
  g_return_if_fail (CTK_IS_STYLE (style));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (CTK_STYLE_GET_CLASS (style)->draw_vline != NULL);

  cairo_save (cr);

  CTK_STYLE_GET_CLASS (style)->draw_vline (style, cr, state_type,
                                           widget, detail,
                                           y1_, y2_, x);

  cairo_restore (cr);
}

/**
 * ctk_paint_shadow:
 * @style: a #CtkStyle
 * @cr: a #cairo_t
 * @state_type: a state
 * @shadow_type: type of shadow to draw
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @x: x origin of the rectangle
 * @y: y origin of the rectangle
 * @width: width of the rectangle
 * @height: width of the rectangle
 *
 * Draws a shadow around the given rectangle in @cr
 * using the given style and state and shadow type.
 *
 * Deprecated:3.0: Use ctk_render_frame() instead
 */
void
ctk_paint_shadow (CtkStyle           *style,
                  cairo_t            *cr,
                  CtkStateType        state_type,
                  CtkShadowType       shadow_type,
                  CtkWidget          *widget,
                  const gchar        *detail,
                  gint                x,
                  gint                y,
                  gint                width,
                  gint                height)
{
  g_return_if_fail (CTK_IS_STYLE (style));
  g_return_if_fail (CTK_STYLE_GET_CLASS (style)->draw_shadow != NULL);
  g_return_if_fail (cr != NULL);
  g_return_if_fail (width >= 0);
  g_return_if_fail (height >= 0);

  cairo_save (cr);

  CTK_STYLE_GET_CLASS (style)->draw_shadow (style, cr, state_type, shadow_type,
                                            widget, detail,
                                            x, y, width, height);

  cairo_restore (cr);
}

/**
 * ctk_paint_arrow:
 * @style: a #CtkStyle
 * @cr: a #cairo_t
 * @state_type: a state
 * @shadow_type: the type of shadow to draw
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @arrow_type: the type of arrow to draw
 * @fill: %TRUE if the arrow tip should be filled
 * @x: x origin of the rectangle to draw the arrow in
 * @y: y origin of the rectangle to draw the arrow in
 * @width: width of the rectangle to draw the arrow in
 * @height: height of the rectangle to draw the arrow in
 *
 * Draws an arrow in the given rectangle on @cr using the given
 * parameters. @arrow_type determines the direction of the arrow.
 *
 * Deprecated:3.0: Use ctk_render_arrow() instead
 */
void
ctk_paint_arrow (CtkStyle           *style,
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
                 gint                height)
{
  g_return_if_fail (CTK_IS_STYLE (style));
  g_return_if_fail (CTK_STYLE_GET_CLASS (style)->draw_arrow != NULL);
  g_return_if_fail (cr != NULL);
  g_return_if_fail (width >= 0);
  g_return_if_fail (height >= 0);

  cairo_save (cr);

  CTK_STYLE_GET_CLASS (style)->draw_arrow (style, cr, state_type, shadow_type,
                                           widget, detail,
                                           arrow_type, fill, x, y, width, height);

  cairo_restore (cr);
}

/**
 * ctk_paint_diamond:
 * @style: a #CtkStyle
 * @cr: a #cairo_t
 * @state_type: a state
 * @shadow_type: the type of shadow to draw
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @x: x origin of the rectangle to draw the diamond in
 * @y: y origin of the rectangle to draw the diamond in
 * @width: width of the rectangle to draw the diamond in
 * @height: height of the rectangle to draw the diamond in
 *
 * Draws a diamond in the given rectangle on @window using the given
 * parameters.
 *
 * Deprecated:3.0: Use cairo instead
 */
void
ctk_paint_diamond (CtkStyle           *style,
                   cairo_t            *cr,
                   CtkStateType        state_type,
                   CtkShadowType       shadow_type,
                   CtkWidget          *widget,
                   const gchar        *detail,
                   gint                x,
                   gint                y,
                   gint                width,
                   gint                height)
{
  g_return_if_fail (CTK_IS_STYLE (style));
  g_return_if_fail (CTK_STYLE_GET_CLASS (style)->draw_diamond != NULL);
  g_return_if_fail (cr != NULL);
  g_return_if_fail (width >= 0);
  g_return_if_fail (height >= 0);

  cairo_save (cr);

  CTK_STYLE_GET_CLASS (style)->draw_diamond (style, cr, state_type, shadow_type,
                                             widget, detail,
                                             x, y, width, height);

  cairo_restore (cr);
}

/**
 * ctk_paint_box:
 * @style: a #CtkStyle
 * @cr: a #cairo_t
 * @state_type: a state
 * @shadow_type: the type of shadow to draw
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @x: x origin of the box
 * @y: y origin of the box
 * @width: the width of the box
 * @height: the height of the box
 *
 * Draws a box on @cr with the given parameters.
 *
 * Deprecated:3.0: Use ctk_render_frame() and ctk_render_background() instead
 */
void
ctk_paint_box (CtkStyle           *style,
               cairo_t            *cr,
               CtkStateType        state_type,
               CtkShadowType       shadow_type,
               CtkWidget          *widget,
               const gchar        *detail,
               gint                x,
               gint                y,
               gint                width,
               gint                height)
{
  g_return_if_fail (CTK_IS_STYLE (style));
  g_return_if_fail (CTK_STYLE_GET_CLASS (style)->draw_box != NULL);
  g_return_if_fail (cr != NULL);

  cairo_save (cr);

  CTK_STYLE_GET_CLASS (style)->draw_box (style, cr, state_type, shadow_type,
                                         widget, detail,
                                         x, y, width, height);

  cairo_restore (cr);
}

/**
 * ctk_paint_flat_box:
 * @style: a #CtkStyle
 * @cr: a #cairo_t
 * @state_type: a state
 * @shadow_type: the type of shadow to draw
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @x: x origin of the box
 * @y: y origin of the box
 * @width: the width of the box
 * @height: the height of the box
 *
 * Draws a flat box on @cr with the given parameters.
 *
 * Deprecated:3.0: Use ctk_render_frame() and ctk_render_background() instead
 */
void
ctk_paint_flat_box (CtkStyle           *style,
                    cairo_t            *cr,
                    CtkStateType        state_type,
                    CtkShadowType       shadow_type,
                    CtkWidget          *widget,
                    const gchar        *detail,
                    gint                x,
                    gint                y,
                    gint                width,
                    gint                height)
{
  g_return_if_fail (CTK_IS_STYLE (style));
  g_return_if_fail (CTK_STYLE_GET_CLASS (style)->draw_flat_box != NULL);
  g_return_if_fail (cr != NULL);
  g_return_if_fail (width >= 0);
  g_return_if_fail (height >= 0);

  cairo_save (cr);

  CTK_STYLE_GET_CLASS (style)->draw_flat_box (style, cr, state_type, shadow_type,
                                              widget, detail,
                                              x, y, width, height);

  cairo_restore (cr);
}

/**
 * ctk_paint_check:
 * @style: a #CtkStyle
 * @cr: a #cairo_t
 * @state_type: a state
 * @shadow_type: the type of shadow to draw
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @x: x origin of the rectangle to draw the check in
 * @y: y origin of the rectangle to draw the check in
 * @width: the width of the rectangle to draw the check in
 * @height: the height of the rectangle to draw the check in
 *
 * Draws a check button indicator in the given rectangle on @cr with
 * the given parameters.
 *
 * Deprecated:3.0: Use ctk_render_check() instead
 */
void
ctk_paint_check (CtkStyle           *style,
                 cairo_t            *cr,
                 CtkStateType        state_type,
                 CtkShadowType       shadow_type,
                 CtkWidget          *widget,
                 const gchar        *detail,
                 gint                x,
                 gint                y,
                 gint                width,
                 gint                height)
{
  g_return_if_fail (CTK_IS_STYLE (style));
  g_return_if_fail (CTK_STYLE_GET_CLASS (style)->draw_check != NULL);
  g_return_if_fail (cr != NULL);

  cairo_save (cr);

  CTK_STYLE_GET_CLASS (style)->draw_check (style, cr, state_type, shadow_type,
                                           widget, detail,
                                           x, y, width, height);

  cairo_restore (cr);
}

/**
 * ctk_paint_option:
 * @style: a #CtkStyle
 * @cr: a #cairo_t
 * @state_type: a state
 * @shadow_type: the type of shadow to draw
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @x: x origin of the rectangle to draw the option in
 * @y: y origin of the rectangle to draw the option in
 * @width: the width of the rectangle to draw the option in
 * @height: the height of the rectangle to draw the option in
 *
 * Draws a radio button indicator in the given rectangle on @cr with
 * the given parameters.
 *
 * Deprecated:3.0: Use ctk_render_option() instead
 */
void
ctk_paint_option (CtkStyle           *style,
                  cairo_t            *cr,
                  CtkStateType        state_type,
                  CtkShadowType       shadow_type,
                  CtkWidget          *widget,
                  const gchar        *detail,
                  gint                x,
                  gint                y,
                  gint                width,
                  gint                height)
{
  g_return_if_fail (CTK_IS_STYLE (style));
  g_return_if_fail (CTK_STYLE_GET_CLASS (style)->draw_option != NULL);
  g_return_if_fail (cr != NULL);

  cairo_save (cr);

  CTK_STYLE_GET_CLASS (style)->draw_option (style, cr, state_type, shadow_type,
                                            widget, detail,
                                            x, y, width, height);

  cairo_restore (cr);
}

/**
 * ctk_paint_tab:
 * @style: a #CtkStyle
 * @cr: a #cairo_t
 * @state_type: a state
 * @shadow_type: the type of shadow to draw
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @x: x origin of the rectangle to draw the tab in
 * @y: y origin of the rectangle to draw the tab in
 * @width: the width of the rectangle to draw the tab in
 * @height: the height of the rectangle to draw the tab in
 *
 * Draws an option menu tab (i.e. the up and down pointing arrows)
 * in the given rectangle on @cr using the given parameters.
 *
 * Deprecated:3.0: Use cairo instead
 */
void
ctk_paint_tab (CtkStyle           *style,
               cairo_t            *cr,
               CtkStateType        state_type,
               CtkShadowType       shadow_type,
               CtkWidget          *widget,
               const gchar        *detail,
               gint                x,
               gint                y,
               gint                width,
               gint                height)
{
  g_return_if_fail (CTK_IS_STYLE (style));
  g_return_if_fail (CTK_STYLE_GET_CLASS (style)->draw_tab != NULL);
  g_return_if_fail (cr != NULL);

  cairo_save (cr);

  CTK_STYLE_GET_CLASS (style)->draw_tab (style, cr, state_type, shadow_type,
                                         widget, detail,
                                         x, y, width, height);

  cairo_restore (cr);
}

/**
 * ctk_paint_shadow_gap:
 * @style: a #CtkStyle
 * @cr: a #cairo_t
 * @state_type: a state
 * @shadow_type: type of shadow to draw
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @x: x origin of the rectangle
 * @y: y origin of the rectangle
 * @width: width of the rectangle
 * @height: width of the rectangle
 * @gap_side: side in which to leave the gap
 * @gap_x: starting position of the gap
 * @gap_width: width of the gap
 *
 * Draws a shadow around the given rectangle in @cr
 * using the given style and state and shadow type, leaving a
 * gap in one side.
 *
 * Deprecated:3.0: Use ctk_render_frame_gap() instead
 */
void
ctk_paint_shadow_gap (CtkStyle           *style,
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
                      gint                gap_width)
{
  g_return_if_fail (CTK_IS_STYLE (style));
  g_return_if_fail (CTK_STYLE_GET_CLASS (style)->draw_shadow_gap != NULL);
  g_return_if_fail (cr != NULL);
  g_return_if_fail (width >= 0);
  g_return_if_fail (height >= 0);

  cairo_save (cr);

  CTK_STYLE_GET_CLASS (style)->draw_shadow_gap (style, cr, state_type, shadow_type,
                                                widget, detail,
                                                x, y, width, height, gap_side, gap_x, gap_width);

  cairo_restore (cr);
}

/**
 * ctk_paint_box_gap:
 * @style: a #CtkStyle
 * @cr: a #cairo_t
 * @state_type: a state
 * @shadow_type: type of shadow to draw
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @x: x origin of the rectangle
 * @y: y origin of the rectangle
 * @width: width of the rectangle
 * @height: width of the rectangle
 * @gap_side: side in which to leave the gap
 * @gap_x: starting position of the gap
 * @gap_width: width of the gap
 *
 * Draws a box in @cr using the given style and state and shadow type,
 * leaving a gap in one side.
 *
 * Deprecated:3.0: Use ctk_render_frame_gap() instead
 */
void
ctk_paint_box_gap (CtkStyle           *style,
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
                   gint                gap_width)
{
  g_return_if_fail (CTK_IS_STYLE (style));
  g_return_if_fail (CTK_STYLE_GET_CLASS (style)->draw_box_gap != NULL);
  g_return_if_fail (cr != NULL);
  g_return_if_fail (width >= 0);
  g_return_if_fail (height >= 0);

  cairo_save (cr);

  CTK_STYLE_GET_CLASS (style)->draw_box_gap (style, cr, state_type, shadow_type,
                                             widget, detail,
                                             x, y, width, height, gap_side, gap_x, gap_width);

  cairo_restore (cr);
}

/**
 * ctk_paint_extension:
 * @style: a #CtkStyle
 * @cr: a #cairo_t
 * @state_type: a state
 * @shadow_type: type of shadow to draw
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @x: x origin of the extension
 * @y: y origin of the extension
 * @width: width of the extension
 * @height: width of the extension
 * @gap_side: the side on to which the extension is attached
 *
 * Draws an extension, i.e. a notebook tab.
 *
 * Deprecated:3.0: Use ctk_render_extension() instead
 **/
void
ctk_paint_extension (CtkStyle           *style,
                     cairo_t            *cr,
                     CtkStateType        state_type,
                     CtkShadowType       shadow_type,
                     CtkWidget          *widget,
                     const gchar        *detail,
                     gint                x,
                     gint                y,
                     gint                width,
                     gint                height,
                     CtkPositionType     gap_side)
{
  g_return_if_fail (CTK_IS_STYLE (style));
  g_return_if_fail (CTK_STYLE_GET_CLASS (style)->draw_extension != NULL);
  g_return_if_fail (cr != NULL);
  g_return_if_fail (width >= 0);
  g_return_if_fail (height >= 0);

  cairo_save (cr);

  CTK_STYLE_GET_CLASS (style)->draw_extension (style, cr, state_type, shadow_type,
                                               widget, detail,
                                               x, y, width, height, gap_side);

  cairo_restore (cr);
}

/**
 * ctk_paint_focus:
 * @style: a #CtkStyle
 * @cr: a #cairo_t
 * @state_type: a state
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @x: the x origin of the rectangle around which to draw a focus indicator
 * @y: the y origin of the rectangle around which to draw a focus indicator
 * @width: the width of the rectangle around which to draw a focus indicator
 * @height: the height of the rectangle around which to draw a focus indicator
 *
 * Draws a focus indicator around the given rectangle on @cr using the
 * given style.
 *
 * Deprecated:3.0: Use ctk_render_focus() instead
 */
void
ctk_paint_focus (CtkStyle           *style,
                 cairo_t            *cr,
                 CtkStateType        state_type,
                 CtkWidget          *widget,
                 const gchar        *detail,
                 gint                x,
                 gint                y,
                 gint                width,
                 gint                height)
{
  g_return_if_fail (CTK_IS_STYLE (style));
  g_return_if_fail (CTK_STYLE_GET_CLASS (style)->draw_focus != NULL);
  g_return_if_fail (cr != NULL);
  g_return_if_fail (width >= 0);
  g_return_if_fail (height >= 0);

  cairo_save (cr);

  CTK_STYLE_GET_CLASS (style)->draw_focus (style, cr, state_type,
                                           widget, detail,
                                           x, y, width, height);

  cairo_restore (cr);
}

/**
 * ctk_paint_slider:
 * @style: a #CtkStyle
 * @cr: a #cairo_t
 * @state_type: a state
 * @shadow_type: a shadow
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @x: the x origin of the rectangle in which to draw a slider
 * @y: the y origin of the rectangle in which to draw a slider
 * @width: the width of the rectangle in which to draw a slider
 * @height: the height of the rectangle in which to draw a slider
 * @orientation: the orientation to be used
 *
 * Draws a slider in the given rectangle on @cr using the
 * given style and orientation.
 *
 * Deprecated:3.0: Use ctk_render_slider() instead
 **/
void
ctk_paint_slider (CtkStyle           *style,
                  cairo_t            *cr,
                  CtkStateType        state_type,
                  CtkShadowType       shadow_type,
                  CtkWidget          *widget,
                  const gchar        *detail,
                  gint                x,
                  gint                y,
                  gint                width,
                  gint                height,
                  CtkOrientation      orientation)
{
  g_return_if_fail (CTK_IS_STYLE (style));
  g_return_if_fail (CTK_STYLE_GET_CLASS (style)->draw_slider != NULL);
  g_return_if_fail (cr != NULL);
  g_return_if_fail (width >= 0);
  g_return_if_fail (height >= 0);

  cairo_save (cr);

  CTK_STYLE_GET_CLASS (style)->draw_slider (style, cr, state_type, shadow_type,
                                            widget, detail,
                                            x, y, width, height, orientation);

  cairo_restore (cr);
}

/**
 * ctk_paint_handle:
 * @style: a #CtkStyle
 * @cr: a #cairo_t
 * @state_type: a state
 * @shadow_type: type of shadow to draw
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @x: x origin of the handle
 * @y: y origin of the handle
 * @width: with of the handle
 * @height: height of the handle
 * @orientation: the orientation of the handle
 *
 * Draws a handle as used in #CtkHandleBox and #CtkPaned.
 *
 * Deprecated:3.0: Use ctk_render_handle() instead
 **/
void
ctk_paint_handle (CtkStyle           *style,
                  cairo_t            *cr,
                  CtkStateType        state_type,
                  CtkShadowType       shadow_type,
                  CtkWidget          *widget,
                  const gchar        *detail,
                  gint                x,
                  gint                y,
                  gint                width,
                  gint                height,
                  CtkOrientation      orientation)
{
  g_return_if_fail (CTK_IS_STYLE (style));
  g_return_if_fail (CTK_STYLE_GET_CLASS (style)->draw_handle != NULL);
  g_return_if_fail (cr != NULL);
  g_return_if_fail (width >= 0);
  g_return_if_fail (height >= 0);

  cairo_save (cr);

  CTK_STYLE_GET_CLASS (style)->draw_handle (style, cr, state_type, shadow_type,
                                            widget, detail,
                                            x, y, width, height, orientation);

  cairo_restore (cr);
}

/**
 * ctk_paint_expander:
 * @style: a #CtkStyle
 * @cr: a #cairo_t
 * @state_type: a state
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @x: the x position to draw the expander at
 * @y: the y position to draw the expander at
 * @expander_style: the style to draw the expander in; determines
 *   whether the expander is collapsed, expanded, or in an
 *   intermediate state.
 *
 * Draws an expander as used in #CtkTreeView. @x and @y specify the
 * center the expander. The size of the expander is determined by the
 * “expander-size” style property of @widget.  (If widget is not
 * specified or doesn’t have an “expander-size” property, an
 * unspecified default size will be used, since the caller doesn't
 * have sufficient information to position the expander, this is
 * likely not useful.) The expander is expander_size pixels tall
 * in the collapsed position and expander_size pixels wide in the
 * expanded position.
 *
 * Deprecated:3.0: Use ctk_render_expander() instead
 **/
void
ctk_paint_expander (CtkStyle           *style,
                    cairo_t            *cr,
                    CtkStateType        state_type,
                    CtkWidget          *widget,
                    const gchar        *detail,
                    gint                x,
                    gint                y,
                    CtkExpanderStyle    expander_style)
{
  g_return_if_fail (CTK_IS_STYLE (style));
  g_return_if_fail (CTK_STYLE_GET_CLASS (style)->draw_expander != NULL);
  g_return_if_fail (cr != NULL);

  cairo_save (cr);

  CTK_STYLE_GET_CLASS (style)->draw_expander (style, cr, state_type,
                                              widget, detail,
                                              x, y, expander_style);

  cairo_restore (cr);
}

/**
 * ctk_paint_layout:
 * @style: a #CtkStyle
 * @cr: a #cairo_t
 * @state_type: a state
 * @use_text: whether to use the text or foreground
 *            graphics context of @style
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @x: x origin
 * @y: y origin
 * @layout: the layout to draw
 *
 * Draws a layout on @cr using the given parameters.
 *
 * Deprecated:3.0: Use ctk_render_layout() instead
 **/
void
ctk_paint_layout (CtkStyle           *style,
                  cairo_t            *cr,
                  CtkStateType        state_type,
                  gboolean            use_text,
                  CtkWidget          *widget,
                  const gchar        *detail,
                  gint                x,
                  gint                y,
                  PangoLayout        *layout)
{
  g_return_if_fail (CTK_IS_STYLE (style));
  g_return_if_fail (CTK_STYLE_GET_CLASS (style)->draw_layout != NULL);
  g_return_if_fail (cr != NULL);

  cairo_save (cr);

  CTK_STYLE_GET_CLASS (style)->draw_layout (style, cr, state_type, use_text,
                                            widget, detail,
                                            x, y, layout);

  cairo_restore (cr);
}

/**
 * ctk_paint_resize_grip:
 * @style: a #CtkStyle
 * @cr: a #cairo_t
 * @state_type: a state
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @edge: the edge in which to draw the resize grip
 * @x: the x origin of the rectangle in which to draw the resize grip
 * @y: the y origin of the rectangle in which to draw the resize grip
 * @width: the width of the rectangle in which to draw the resize grip
 * @height: the height of the rectangle in which to draw the resize grip
 *
 * Draws a resize grip in the given rectangle on @cr using the given
 * parameters.
 *
 * Deprecated:3.0: Use ctk_render_handle() instead
 */
void
ctk_paint_resize_grip (CtkStyle           *style,
                       cairo_t            *cr,
                       CtkStateType        state_type,
                       CtkWidget          *widget,
                       const gchar        *detail,
                       CdkWindowEdge       edge,
                       gint                x,
                       gint                y,
                       gint                width,
                       gint                height)
{
  g_return_if_fail (CTK_IS_STYLE (style));
  g_return_if_fail (CTK_STYLE_GET_CLASS (style)->draw_resize_grip != NULL);
  g_return_if_fail (cr != NULL);

  cairo_save (cr);

  CTK_STYLE_GET_CLASS (style)->draw_resize_grip (style, cr, state_type,
                                                 widget, detail,
                                                 edge, x, y, width, height);
  cairo_restore (cr);
}

/**
 * ctk_paint_spinner:
 * @style: a #CtkStyle
 * @cr: a #cairo_t
 * @state_type: a state
 * @widget: (allow-none): the widget (may be %NULL)
 * @detail: (allow-none): a style detail (may be %NULL)
 * @step: the nth step
 * @x: the x origin of the rectangle in which to draw the spinner
 * @y: the y origin of the rectangle in which to draw the spinner
 * @width: the width of the rectangle in which to draw the spinner
 * @height: the height of the rectangle in which to draw the spinner
 *
 * Draws a spinner on @window using the given parameters.
 *
 * Deprecated: 3.0: Use ctk_render_icon() and the #CtkStyleContext
 *   you are drawing instead
 */
void
ctk_paint_spinner (CtkStyle           *style,
                   cairo_t            *cr,
                   CtkStateType        state_type,
                   CtkWidget          *widget,
                   const gchar        *detail,
                   guint               step,
                   gint                x,
                   gint                y,
                   gint                width,
                   gint                height)
{
  g_return_if_fail (CTK_IS_STYLE (style));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (CTK_STYLE_GET_CLASS (style)->draw_spinner != NULL);

  cairo_save (cr);

  CTK_STYLE_GET_CLASS (style)->draw_spinner (style, cr, state_type,
                                             widget, detail,
					     step, x, y, width, height);

  cairo_restore (cr);
}

static CtkStyle *
ctk_widget_get_default_style_for_screen (CdkScreen *screen)
{
  CtkStyle *default_style;

  if G_UNLIKELY (quark_default_style == 0)
    quark_default_style = g_quark_from_static_string ("ctk-legacy-default-style");

  default_style = g_object_get_qdata (G_OBJECT (screen), quark_default_style);
  if (default_style == NULL)
    {
      default_style = ctk_style_new ();
      g_object_set_qdata_full (G_OBJECT (screen),
                               quark_default_style,
                               default_style,
                               g_object_unref);
    }

  return default_style;
}

/**
 * ctk_widget_get_default_style:
 *
 * Returns the default style used by all widgets initially.
 *
 * Returns: (transfer none): the default style. This #CtkStyle
 *     object is owned by CTK+ and should not be modified or freed.
 *
 * Deprecated:3.0: Use #CtkStyleContext instead, and
 *     ctk_css_provider_get_default() to obtain a #CtkStyleProvider
 *     with the default widget style information.
 */
CtkStyle *
ctk_widget_get_default_style (void)
{
  static CtkStyle *default_style = NULL;
  CtkStyle *style = NULL;
  CdkScreen *screen = cdk_screen_get_default ();

  if (screen)
    style = ctk_widget_get_default_style_for_screen (screen);
  else
    {
      if (default_style == NULL)
        default_style = ctk_style_new ();
      style = default_style;
    }

  return style;
}

/**
 * ctk_widget_style_attach:
 * @widget: a #CtkWidget
 *
 * This function attaches the widget’s #CtkStyle to the widget's
 * #CdkWindow. It is a replacement for
 *
 * |[
 * widget->style = ctk_style_attach (widget->style, widget->window);
 * ]|
 *
 * and should only ever be called in a derived widget’s “realize”
 * implementation which does not chain up to its parent class'
 * “realize” implementation, because one of the parent classes
 * (finally #CtkWidget) would attach the style itself.
 *
 * Since: 2.20
 *
 * Deprecated: 3.0: This step is unnecessary with #CtkStyleContext.
 **/
void
ctk_widget_style_attach (CtkWidget *widget)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (ctk_widget_get_realized (widget));
}

/**
 * ctk_widget_has_rc_style:
 * @widget: a #CtkWidget
 *
 * Determines if the widget style has been looked up through the rc mechanism.
 *
 * Returns: %TRUE if the widget has been looked up through the rc
 *   mechanism, %FALSE otherwise.
 *
 * Since: 2.20
 *
 * Deprecated:3.0: Use #CtkStyleContext instead
 **/
gboolean
ctk_widget_has_rc_style (CtkWidget *widget)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  return FALSE;
}

/**
 * ctk_widget_set_style:
 * @widget: a #CtkWidget
 * @style: (allow-none): a #CtkStyle, or %NULL to remove the effect
 *     of a previous call to ctk_widget_set_style() and go back to
 *     the default style
 *
 * Used to set the #CtkStyle for a widget (@widget->style). Since
 * CTK 3, this function does nothing, the passed in style is ignored.
 *
 * Deprecated:3.0: Use #CtkStyleContext instead
 */
void
ctk_widget_set_style (CtkWidget *widget,
                      CtkStyle  *style)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));
}

/**
 * ctk_widget_ensure_style:
 * @widget: a #CtkWidget
 *
 * Ensures that @widget has a style (@widget->style).
 *
 * Not a very useful function; most of the time, if you
 * want the style, the widget is realized, and realized
 * widgets are guaranteed to have a style already.
 *
 * Deprecated:3.0: Use #CtkStyleContext instead
 */
void
ctk_widget_ensure_style (CtkWidget *widget)
{
  CtkStyle *style;
  g_return_if_fail (CTK_IS_WIDGET (widget));

  style = _ctk_widget_get_style (widget);
  if (style == ctk_widget_get_default_style ())
    {
      g_object_unref (style);
      _ctk_widget_set_style (widget, NULL);
    }
}

/**
 * ctk_widget_get_style:
 * @widget: a #CtkWidget
 *
 * Simply an accessor function that returns @widget->style.
 *
 * Returns: (transfer none): the widget’s #CtkStyle
 *
 * Deprecated:3.0: Use #CtkStyleContext instead
 */
CtkStyle*
ctk_widget_get_style (CtkWidget *widget)
{
  CtkStyle *style;
  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  style = _ctk_widget_get_style (widget);

  if (style == NULL)
    {
      style = g_object_new (CTK_TYPE_STYLE,
                            "context", ctk_widget_get_style_context (widget),
                            NULL);
      _ctk_widget_set_style (widget, style);
    }

  return style;
}

/**
 * ctk_widget_modify_style:
 * @widget: a #CtkWidget
 * @style: the #CtkRcStyle-struct holding the style modifications
 *
 * Modifies style values on the widget.
 *
 * Modifications made using this technique take precedence over
 * style values set via an RC file, however, they will be overridden
 * if a style is explicitly set on the widget using ctk_widget_set_style().
 * The #CtkRcStyle-struct is designed so each field can either be
 * set or unset, so it is possible, using this function, to modify some
 * style values and leave the others unchanged.
 *
 * Note that modifications made with this function are not cumulative
 * with previous calls to ctk_widget_modify_style() or with such
 * functions as ctk_widget_modify_fg(). If you wish to retain
 * previous values, you must first call ctk_widget_get_modifier_style(),
 * make your modifications to the returned style, then call
 * ctk_widget_modify_style() with that style. On the other hand,
 * if you first call ctk_widget_modify_style(), subsequent calls
 * to such functions ctk_widget_modify_fg() will have a cumulative
 * effect with the initial modifications.
 *
 * Deprecated:3.0: Use #CtkStyleContext with a custom #CtkStyleProvider instead
 */
void
ctk_widget_modify_style (CtkWidget      *widget,
                         CtkRcStyle     *style)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (CTK_IS_RC_STYLE (style));

  g_object_set_data_full (G_OBJECT (widget),
                          "ctk-rc-style",
                           ctk_rc_style_copy (style),
                           (GDestroyNotify) g_object_unref);
}

/**
 * ctk_widget_get_modifier_style:
 * @widget: a #CtkWidget
 *
 * Returns the current modifier style for the widget. (As set by
 * ctk_widget_modify_style().) If no style has previously set, a new
 * #CtkRcStyle will be created with all values unset, and set as the
 * modifier style for the widget. If you make changes to this rc
 * style, you must call ctk_widget_modify_style(), passing in the
 * returned rc style, to make sure that your changes take effect.
 *
 * Caution: passing the style back to ctk_widget_modify_style() will
 * normally end up destroying it, because ctk_widget_modify_style() copies
 * the passed-in style and sets the copy as the new modifier style,
 * thus dropping any reference to the old modifier style. Add a reference
 * to the modifier style if you want to keep it alive.
 *
 * Returns: (transfer none): the modifier style for the widget.
 *     This rc style is owned by the widget. If you want to keep a
 *     pointer to value this around, you must add a refcount using
 *     g_object_ref().
 *
 * Deprecated:3.0: Use #CtkStyleContext with a custom #CtkStyleProvider instead
 */
CtkRcStyle *
ctk_widget_get_modifier_style (CtkWidget *widget)
{
  CtkRcStyle *rc_style;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  rc_style = g_object_get_data (G_OBJECT (widget), "ctk-rc-style");

  if (!rc_style)
    {
      rc_style = ctk_rc_style_new ();
      g_object_set_data_full (G_OBJECT (widget),
                              "ctk-rc-style",
                              rc_style,
                              (GDestroyNotify) g_object_unref);
    }

  return rc_style;
}

static void
ctk_widget_modify_color_component (CtkWidget      *widget,
                                   CtkRcFlags      component,
                                   CtkStateType    state,
                                   const CdkColor *color)
{
  CtkRcStyle *rc_style = ctk_widget_get_modifier_style (widget);

  if (color)
    {
      switch (component)
        {
        case CTK_RC_FG:
          rc_style->fg[state] = *color;
          break;
        case CTK_RC_BG:
          rc_style->bg[state] = *color;
          break;
        case CTK_RC_TEXT:
          rc_style->text[state] = *color;
          break;
        case CTK_RC_BASE:
          rc_style->base[state] = *color;
          break;
        default:
          g_assert_not_reached();
        }

      rc_style->color_flags[state] |= component;
    }
  else
    rc_style->color_flags[state] &= ~component;

  ctk_widget_modify_style (widget, rc_style);
}

/**
 * ctk_widget_modify_fg:
 * @widget: a #CtkWidget
 * @state: the state for which to set the foreground color
 * @color: (allow-none): the color to assign (does not need to be allocated),
 *     or %NULL to undo the effect of previous calls to
 *     of ctk_widget_modify_fg().
 *
 * Sets the foreground color for a widget in a particular state.
 *
 * All other style values are left untouched.
 * See also ctk_widget_modify_style().
 *
 * Deprecated:3.0: Use ctk_widget_override_color() instead
 */
void
ctk_widget_modify_fg (CtkWidget      *widget,
                      CtkStateType    state,
                      const CdkColor *color)
{
  CtkStateFlags flags;
  CdkRGBA rgba;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (state >= CTK_STATE_NORMAL && state <= CTK_STATE_INSENSITIVE);

  switch (state)
    {
    case CTK_STATE_ACTIVE:
      flags = CTK_STATE_FLAG_ACTIVE;
      break;
    case CTK_STATE_PRELIGHT:
      flags = CTK_STATE_FLAG_PRELIGHT;
      break;
    case CTK_STATE_SELECTED:
      flags = CTK_STATE_FLAG_SELECTED;
      break;
    case CTK_STATE_INSENSITIVE:
      flags = CTK_STATE_FLAG_INSENSITIVE;
      break;
    case CTK_STATE_NORMAL:
    default:
      flags = 0;
    }

  if (color)
    {
      rgba.red = color->red / 65535.;
      rgba.green = color->green / 65535.;
      rgba.blue = color->blue / 65535.;
      rgba.alpha = 1;

      ctk_widget_override_color (widget, flags, &rgba);
    }
  else
    ctk_widget_override_color (widget, flags, NULL);
}

/**
 * ctk_widget_modify_bg:
 * @widget: a #CtkWidget
 * @state: the state for which to set the background color
 * @color: (allow-none): the color to assign (does not need
 *     to be allocated), or %NULL to undo the effect of previous
 *     calls to of ctk_widget_modify_bg().
 *
 * Sets the background color for a widget in a particular state.
 *
 * All other style values are left untouched.
 * See also ctk_widget_modify_style().
 *
 * > Note that “no window” widgets (which have the %CTK_NO_WINDOW
 * > flag set) draw on their parent container’s window and thus may
 * > not draw any background themselves. This is the case for e.g.
 * > #CtkLabel.
 * >
 * > To modify the background of such widgets, you have to set the
 * > background color on their parent; if you want to set the background
 * > of a rectangular area around a label, try placing the label in
 * > a #CtkEventBox widget and setting the background color on that.
 *
 * Deprecated:3.0: Use ctk_widget_override_background_color() instead
 */
void
ctk_widget_modify_bg (CtkWidget      *widget,
                      CtkStateType    state,
                      const CdkColor *color)
{
  CtkStateFlags flags;
  CdkRGBA rgba;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (state >= CTK_STATE_NORMAL && state <= CTK_STATE_INSENSITIVE);

  switch (state)
    {
    case CTK_STATE_ACTIVE:
      flags = CTK_STATE_FLAG_ACTIVE;
      break;
    case CTK_STATE_PRELIGHT:
      flags = CTK_STATE_FLAG_PRELIGHT;
      break;
    case CTK_STATE_SELECTED:
      flags = CTK_STATE_FLAG_SELECTED;
      break;
    case CTK_STATE_INSENSITIVE:
      flags = CTK_STATE_FLAG_INSENSITIVE;
      break;
    case CTK_STATE_NORMAL:
    default:
      flags = 0;
    }

  if (color)
    {
      rgba.red = color->red / 65535.;
      rgba.green = color->green / 65535.;
      rgba.blue = color->blue / 65535.;
      rgba.alpha = 1;

      ctk_widget_override_background_color (widget, flags, &rgba);
    }
  else
    ctk_widget_override_background_color (widget, flags, NULL);
}

/**
 * ctk_widget_modify_text:
 * @widget: a #CtkWidget
 * @state: the state for which to set the text color
 * @color: (allow-none): the color to assign (does not need to
 *     be allocated), or %NULL to undo the effect of previous
 *     calls to of ctk_widget_modify_text().
 *
 * Sets the text color for a widget in a particular state.
 *
 * All other style values are left untouched.
 * The text color is the foreground color used along with the
 * base color (see ctk_widget_modify_base()) for widgets such
 * as #CtkEntry and #CtkTextView.
 * See also ctk_widget_modify_style().
 *
 * Deprecated:3.0: Use ctk_widget_override_color() instead
 */
void
ctk_widget_modify_text (CtkWidget      *widget,
                        CtkStateType    state,
                        const CdkColor *color)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (state >= CTK_STATE_NORMAL && state <= CTK_STATE_INSENSITIVE);

  ctk_widget_modify_color_component (widget, CTK_RC_TEXT, state, color);
}

/**
 * ctk_widget_modify_base:
 * @widget: a #CtkWidget
 * @state: the state for which to set the base color
 * @color: (allow-none): the color to assign (does not need to
 *     be allocated), or %NULL to undo the effect of previous
 *     calls to of ctk_widget_modify_base().
 *
 * Sets the base color for a widget in a particular state.
 * All other style values are left untouched. The base color
 * is the background color used along with the text color
 * (see ctk_widget_modify_text()) for widgets such as #CtkEntry
 * and #CtkTextView. See also ctk_widget_modify_style().
 *
 * > Note that “no window” widgets (which have the %CTK_NO_WINDOW
 * > flag set) draw on their parent container’s window and thus may
 * > not draw any background themselves. This is the case for e.g.
 * > #CtkLabel.
 * >
 * > To modify the background of such widgets, you have to set the
 * > base color on their parent; if you want to set the background
 * > of a rectangular area around a label, try placing the label in
 * > a #CtkEventBox widget and setting the base color on that.
 *
 * Deprecated:3.0: Use ctk_widget_override_background_color() instead
 */
void
ctk_widget_modify_base (CtkWidget      *widget,
                        CtkStateType    state,
                        const CdkColor *color)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (state >= CTK_STATE_NORMAL && state <= CTK_STATE_INSENSITIVE);

  ctk_widget_modify_color_component (widget, CTK_RC_BASE, state, color);
}

/**
 * ctk_widget_modify_cursor:
 * @widget: a #CtkWidget
 * @primary: (nullable): the color to use for primary cursor (does not
 *     need to be allocated), or %NULL to undo the effect of previous
 *     calls to of ctk_widget_modify_cursor().
 * @secondary: (nullable): the color to use for secondary cursor (does
 *     not need to be allocated), or %NULL to undo the effect of
 *     previous calls to of ctk_widget_modify_cursor().
 *
 * Sets the cursor color to use in a widget, overriding the #CtkWidget
 * cursor-color and secondary-cursor-color
 * style properties.
 *
 * All other style values are left untouched.
 * See also ctk_widget_modify_style().
 *
 * Since: 2.12
 *
 * Deprecated: 3.0: Use ctk_widget_override_cursor() instead.
 */
void
ctk_widget_modify_cursor (CtkWidget      *widget,
                          const CdkColor *primary,
                          const CdkColor *secondary)
{
  CdkRGBA primary_rgba, secondary_rgba;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  primary_rgba.red = primary->red / 65535.;
  primary_rgba.green = primary->green / 65535.;
  primary_rgba.blue = primary->blue / 65535.;
  primary_rgba.alpha = 1;

  secondary_rgba.red = secondary->red / 65535.;
  secondary_rgba.green = secondary->green / 65535.;
  secondary_rgba.blue = secondary->blue / 65535.;
  secondary_rgba.alpha = 1;

  ctk_widget_override_cursor (widget, &primary_rgba, &secondary_rgba);
}

/**
 * ctk_widget_modify_font:
 * @widget: a #CtkWidget
 * @font_desc: (allow-none): the font description to use, or %NULL
 *     to undo the effect of previous calls to ctk_widget_modify_font()
 *
 * Sets the font to use for a widget.
 *
 * All other style values are left untouched.
 * See also ctk_widget_modify_style().
 *
 * Deprecated:3.0: Use ctk_widget_override_font() instead
 */
void
ctk_widget_modify_font (CtkWidget            *widget,
                        PangoFontDescription *font_desc)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  ctk_widget_override_font (widget, font_desc);
}

/**
 * ctk_widget_reset_rc_styles:
 * @widget: a #CtkWidget.
 *
 * Reset the styles of @widget and all descendents, so when
 * they are looked up again, they get the correct values
 * for the currently loaded RC file settings.
 *
 * This function is not useful for applications.
 *
 * Deprecated:3.0: Use #CtkStyleContext instead, and ctk_widget_reset_style()
 */
void
ctk_widget_reset_rc_styles (CtkWidget *widget)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  ctk_widget_reset_style (widget);
}

/**
 * ctk_widget_path:
 * @widget: a #CtkWidget
 * @path_length: (out) (allow-none): location to store length of the path,
 *     or %NULL
 * @path: (out) (allow-none): location to store allocated path string,
 *     or %NULL
 * @path_reversed: (out) (allow-none): location to store allocated reverse
 *     path string, or %NULL
 *
 * Obtains the full path to @widget. The path is simply the name of a
 * widget and all its parents in the container hierarchy, separated by
 * periods. The name of a widget comes from
 * ctk_widget_get_name(). Paths are used to apply styles to a widget
 * in ctkrc configuration files. Widget names are the type of the
 * widget by default (e.g. “CtkButton”) or can be set to an
 * application-specific value with ctk_widget_set_name(). By setting
 * the name of a widget, you allow users or theme authors to apply
 * styles to that specific widget in their ctkrc
 * file. @path_reversed_p fills in the path in reverse order,
 * i.e. starting with @widget’s name instead of starting with the name
 * of @widget’s outermost ancestor.
 *
 * Deprecated:3.0: Use ctk_widget_get_path() instead
 **/
void
ctk_widget_path (CtkWidget *widget,
                 guint     *path_length,
                 gchar    **path,
                 gchar    **path_reversed)
{
  static gchar *rev_path = NULL;
  static guint tmp_path_len = 0;
  guint len;

#define INIT_PATH_SIZE (512)

  g_return_if_fail (CTK_IS_WIDGET (widget));

  len = 0;
  do
    {
      const gchar *string;
      const gchar *s;
      gchar *d;
      guint l;

      string = ctk_widget_get_name (widget);
      l = strlen (string);
      while (tmp_path_len <= len + l + 1)
        {
          tmp_path_len += INIT_PATH_SIZE;
          rev_path = g_realloc (rev_path, tmp_path_len);
        }
      s = string + l - 1;
      d = rev_path + len;
      while (s >= string)
        *(d++) = *(s--);
      len += l;

      widget = ctk_widget_get_parent (widget);

      if (widget)
        rev_path[len++] = '.';
      else
        rev_path[len++] = 0;
    }
  while (widget);

  if (path_length)
    *path_length = len - 1;
  if (path_reversed)
    *path_reversed = g_strdup (rev_path);
  if (path)
    {
      *path = g_strdup (rev_path);
      g_strreverse (*path);
    }
}

/**
 * ctk_widget_class_path:
 * @widget: a #CtkWidget
 * @path_length: (out) (optional): location to store the length of the
 *     class path, or %NULL
 * @path: (out) (optional): location to store the class path as an
 *     allocated string, or %NULL
 * @path_reversed: (out) (optional): location to store the reverse
 *     class path as an allocated string, or %NULL
 *
 * Same as ctk_widget_path(), but always uses the name of a widget’s type,
 * never uses a custom name set with ctk_widget_set_name().
 *
 * Deprecated:3.0: Use ctk_widget_get_path() instead
 **/
void
ctk_widget_class_path (CtkWidget *widget,
                       guint     *path_length,
                       gchar    **path,
                       gchar    **path_reversed)
{
  static gchar *rev_path = NULL;
  static guint tmp_path_len = 0;
  guint len;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  len = 0;
  do
    {
      const gchar *string;
      const gchar *s;
      gchar *d;
      guint l;

      string = g_type_name (G_OBJECT_TYPE (widget));
      l = strlen (string);
      while (tmp_path_len <= len + l + 1)
        {
          tmp_path_len += INIT_PATH_SIZE;
          rev_path = g_realloc (rev_path, tmp_path_len);
        }
      s = string + l - 1;
      d = rev_path + len;
      while (s >= string)
        *(d++) = *(s--);
      len += l;

      widget = ctk_widget_get_parent (widget);

      if (widget)
        rev_path[len++] = '.';
      else
        rev_path[len++] = 0;
    }
  while (widget);

  if (path_length)
    *path_length = len - 1;
  if (path_reversed)
    *path_reversed = g_strdup (rev_path);
  if (path)
    {
      *path = g_strdup (rev_path);
      g_strreverse (*path);
    }
}

/**
 * ctk_widget_render_icon:
 * @widget: a #CtkWidget
 * @stock_id: a stock ID
 * @size: (type int): a stock size (#CtkIconSize). A size of `(CtkIconSize)-1`
 *     means render at the size of the source and don’t scale (if there are
 *     multiple source sizes, CTK+ picks one of the available sizes).
 * @detail: (allow-none): render detail to pass to theme engine
 *
 * A convenience function that uses the theme settings for @widget
 * to look up @stock_id and render it to a pixbuf. @stock_id should
 * be a stock icon ID such as #CTK_STOCK_OPEN or #CTK_STOCK_OK. @size
 * should be a size such as #CTK_ICON_SIZE_MENU. @detail should be a
 * string that identifies the widget or code doing the rendering, so
 * that theme engines can special-case rendering for that widget or
 * code.
 *
 * The pixels in the returned #GdkPixbuf are shared with the rest of
 * the application and should not be modified. The pixbuf should be
 * freed after use with g_object_unref().
 *
 * Returns: (nullable) (transfer full): a new pixbuf, or %NULL if the
 *     stock ID wasn’t known
 *
 * Deprecated: 3.0: Use ctk_widget_render_icon_pixbuf() instead.
 **/
GdkPixbuf*
ctk_widget_render_icon (CtkWidget      *widget,
                        const gchar    *stock_id,
                        CtkIconSize     size,
                        const gchar    *detail)
{
  ctk_widget_ensure_style (widget);

  return ctk_widget_render_icon_pixbuf (widget, stock_id, size);
}
