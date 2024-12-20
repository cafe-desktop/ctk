/* CTK - The GIMP Toolkit
 * Copyright (C) 2012 Red Hat, Inc.
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

#include "config.h"

#include "ctkcolorscaleprivate.h"

#include "ctkcolorchooserprivate.h"
#include "ctkgesturelongpress.h"
#include "ctkcolorutils.h"
#include "ctkorientable.h"
#include "ctkrangeprivate.h"
#include "ctkstylecontext.h"
#include "ctkaccessible.h"
#include "ctkprivate.h"
#include "ctkintl.h"
#include "ctkrenderprivate.h"

#include <math.h>

struct _CtkColorScalePrivate
{
  CdkRGBA color;
  CtkColorScaleType type;

  CtkGesture *long_press_gesture;
};

enum
{
  PROP_ZERO,
  PROP_SCALE_TYPE
};

static void hold_action (CtkGestureLongPress *gesture,
                         gdouble              x,
                         gdouble              y,
                         CtkColorScale       *scale);

G_DEFINE_TYPE_WITH_PRIVATE (CtkColorScale, ctk_color_scale, CTK_TYPE_SCALE)

void
ctk_color_scale_draw_trough (CtkColorScale  *scale,
                             cairo_t        *cr,
                             int             x,
                             int             y,
                             int             width,
                             int             height)
{
  CtkWidget *widget;

  if (width <= 1 || height <= 1)
    return;

  cairo_save (cr);
  cairo_translate (cr, x, y);

  widget = CTK_WIDGET (scale);
  cairo_rectangle (cr, 0, 0, width, height);
  cairo_clip (cr);

  if (ctk_orientable_get_orientation (CTK_ORIENTABLE (widget)) == CTK_ORIENTATION_HORIZONTAL &&
      ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL)
    {
      cairo_translate (cr, width, 0);
      cairo_scale (cr, -1, 1);
    }

  if (scale->priv->type == CTK_COLOR_SCALE_HUE)
    {
      gint stride;
      cairo_surface_t *tmp;
      guint red, green, blue;
      guint32 *data, *p;
      gdouble h;
      gdouble r, g, b;
      gdouble f;
      gint x, y;

      stride = cairo_format_stride_for_width (CAIRO_FORMAT_RGB24, width);

      data = g_malloc (height * stride);

      f = 1.0 / (height - 1);
      for (y = 0; y < height; y++)
        {
          h = CLAMP (y * f, 0.0, 1.0);
          p = data + y * (stride / 4);
          for (x = 0; x < width; x++)
            {
              ctk_hsv_to_rgb (h, 1, 1, &r, &g, &b);
              red = CLAMP (r * 255, 0, 255);
              green = CLAMP (g * 255, 0, 255);
              blue = CLAMP (b * 255, 0, 255);
              p[x] = (red << 16) | (green << 8) | blue;
            }
        }

      tmp = cairo_image_surface_create_for_data ((guchar *)data, CAIRO_FORMAT_RGB24,
                                                 width, height, stride);

      cairo_set_source_surface (cr, tmp, 0, 0);
      cairo_paint (cr);

      cairo_surface_destroy (tmp);
      g_free (data);
    }
  else if (scale->priv->type == CTK_COLOR_SCALE_ALPHA)
    {
      cairo_pattern_t *pattern;
      cairo_matrix_t matrix;
      CdkRGBA *color;

      cairo_set_source_rgb (cr, 0.33, 0.33, 0.33);
      cairo_paint (cr);
      cairo_set_source_rgb (cr, 0.66, 0.66, 0.66);

      pattern = _ctk_color_chooser_get_checkered_pattern ();
      cairo_matrix_init_scale (&matrix, 0.125, 0.125);
      cairo_pattern_set_matrix (pattern, &matrix);
      cairo_mask (cr, pattern);
      cairo_pattern_destroy (pattern);

      color = &scale->priv->color;

      pattern = cairo_pattern_create_linear (0, 0, width, 0);
      cairo_pattern_add_color_stop_rgba (pattern, 0, color->red, color->green, color->blue, 0);
      cairo_pattern_add_color_stop_rgba (pattern, width, color->red, color->green, color->blue, 1);
      cairo_set_source (cr, pattern);
      cairo_paint (cr);
      cairo_pattern_destroy (pattern);
    }

  cairo_restore (cr);
}

static void
ctk_color_scale_init (CtkColorScale *scale)
{
  CtkStyleContext *context;

  scale->priv = ctk_color_scale_get_instance_private (scale);

  ctk_widget_add_events (CTK_WIDGET (scale), CDK_TOUCH_MASK);

  scale->priv->long_press_gesture = ctk_gesture_long_press_new (CTK_WIDGET (scale));
  g_signal_connect (scale->priv->long_press_gesture, "pressed",
                    G_CALLBACK (hold_action), scale);
  ctk_event_controller_set_propagation_phase (CTK_EVENT_CONTROLLER (scale->priv->long_press_gesture),
                                              CTK_PHASE_TARGET);

  context = ctk_widget_get_style_context (CTK_WIDGET (scale));
  ctk_style_context_add_class (context, "color");
}

static void
scale_finalize (GObject *object)
{
  CtkColorScale *scale = CTK_COLOR_SCALE (object);

  g_clear_object (&scale->priv->long_press_gesture);

  G_OBJECT_CLASS (ctk_color_scale_parent_class)->finalize (object);
}

static void
scale_get_property (GObject    *object,
                    guint       prop_id,
                    GValue     *value,
                    GParamSpec *pspec)
{
  CtkColorScale *scale = CTK_COLOR_SCALE (object);

  switch (prop_id)
    {
    case PROP_SCALE_TYPE:
      g_value_set_int (value, scale->priv->type);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
scale_set_type (CtkColorScale     *scale,
                CtkColorScaleType  type)
{
  AtkObject *atk_obj;

  scale->priv->type = type;

  atk_obj = ctk_widget_get_accessible (CTK_WIDGET (scale));
  if (CTK_IS_ACCESSIBLE (atk_obj))
    {
      if (type == CTK_COLOR_SCALE_HUE)
        atk_object_set_name (atk_obj, C_("Color channel", "Hue"));
      else if (type == CTK_COLOR_SCALE_ALPHA)
        atk_object_set_name (atk_obj, C_("Color channel", "Alpha"));
      atk_object_set_role (atk_obj, ATK_ROLE_COLOR_CHOOSER);
    }
}

static void
scale_set_property (GObject      *object,
                    guint         prop_id,
                    const GValue *value,
                    GParamSpec   *pspec)
{
  CtkColorScale *scale = CTK_COLOR_SCALE (object);

  switch (prop_id)
    {
    case PROP_SCALE_TYPE:
      scale_set_type (scale, (CtkColorScaleType)g_value_get_int (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
hold_action (CtkGestureLongPress *gesture G_GNUC_UNUSED,
             gdouble              x G_GNUC_UNUSED,
             gdouble              y G_GNUC_UNUSED,
             CtkColorScale       *scale)
{
  gboolean handled;

  g_signal_emit_by_name (scale, "popup-menu", &handled);
}

static void
ctk_color_scale_class_init (CtkColorScaleClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = scale_finalize;
  object_class->get_property = scale_get_property;
  object_class->set_property = scale_set_property;

  g_object_class_install_property (object_class, PROP_SCALE_TYPE,
      g_param_spec_int ("scale-type", P_("Scale type"), P_("Scale type"),
                        0, 1, 0,
                        CTK_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

void
ctk_color_scale_set_rgba (CtkColorScale *scale,
                          const CdkRGBA *color)
{
  scale->priv->color = *color;
  ctk_widget_queue_draw (CTK_WIDGET (scale));
}

CtkWidget *
ctk_color_scale_new (CtkAdjustment     *adjustment,
                     CtkColorScaleType  type)
{
  return g_object_new (CTK_TYPE_COLOR_SCALE,
                       "adjustment", adjustment,
                       "draw-value", FALSE,
                       "scale-type", type,
                       NULL);
}
