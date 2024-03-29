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
 * Modified by the CTK+ Team and others 1997-2001.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

/**
 * SECTION:ctkarrow
 * @Short_description: Displays an arrow
 * @Title: CtkArrow
 * @See_also: ctk_render_arrow()
 *
 * CtkArrow should be used to draw simple arrows that need to point in
 * one of the four cardinal directions (up, down, left, or right).  The
 * style of the arrow can be one of shadow in, shadow out, etched in, or
 * etched out.  Note that these directions and style types may be
 * amended in versions of CTK+ to come.
 *
 * CtkArrow will fill any space alloted to it, but since it is inherited
 * from #CtkMisc, it can be padded and/or aligned, to fill exactly the
 * space the programmer desires.
 *
 * Arrows are created with a call to ctk_arrow_new().  The direction or
 * style of an arrow can be changed after creation by using ctk_arrow_set().
 *
 * CtkArrow has been deprecated; you can simply use a #CtkImage with a
 * suitable icon name, such as “pan-down-symbolic“. When replacing 
 * CtkArrow by an image, pay attention to the fact that CtkArrow is
 * doing automatic flipping between #CTK_ARROW_LEFT and #CTK_ARROW_RIGHT,
 * depending on the text direction. To get the same effect with an image,
 * use the icon names “pan-start-symbolic“ and “pan-end-symbolic“, which
 * react to the text direction.
 */

#include "config.h"
#include <math.h>
#include "ctkarrow.h"
#include "ctksizerequest.h"
#include "ctktypebuiltins.h"
#include "ctkprivate.h"
#include "ctkintl.h"

#include "a11y/ctkarrowaccessible.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

#define MIN_ARROW_SIZE  15

struct _CtkArrowPrivate
{
  gint16 arrow_type;
  gint16 shadow_type;
};

enum {
  PROP_0,
  PROP_ARROW_TYPE,
  PROP_SHADOW_TYPE
};


static void     ctk_arrow_set_property (GObject        *object,
                                        guint           prop_id,
                                        const GValue   *value,
                                        GParamSpec     *pspec);
static void     ctk_arrow_get_property (GObject        *object,
                                        guint           prop_id,
                                        GValue         *value,
                                        GParamSpec     *pspec);
static gboolean ctk_arrow_draw         (CtkWidget      *widget,
                                        cairo_t        *cr);

static void     ctk_arrow_get_preferred_width         (CtkWidget           *widget,
                                                       gint                *minimum_size,
                                                       gint                *natural_size);
static void     ctk_arrow_get_preferred_height        (CtkWidget           *widget,
                                                       gint                *minimum_size,
                                                       gint                *natural_size);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
G_DEFINE_TYPE_WITH_PRIVATE (CtkArrow, ctk_arrow, CTK_TYPE_MISC)
G_GNUC_END_IGNORE_DEPRECATIONS

static void
ctk_arrow_class_init (CtkArrowClass *class)
{
  GObjectClass *gobject_class;
  CtkWidgetClass *widget_class;

  gobject_class = (GObjectClass*) class;
  widget_class = (CtkWidgetClass*) class;

  gobject_class->set_property = ctk_arrow_set_property;
  gobject_class->get_property = ctk_arrow_get_property;

  widget_class->draw = ctk_arrow_draw;
  widget_class->get_preferred_width  = ctk_arrow_get_preferred_width;
  widget_class->get_preferred_height = ctk_arrow_get_preferred_height;

  g_object_class_install_property (gobject_class,
                                   PROP_ARROW_TYPE,
                                   g_param_spec_enum ("arrow-type",
                                                      P_("Arrow direction"),
                                                      P_("The direction the arrow should point"),
						      CTK_TYPE_ARROW_TYPE,
						      CTK_ARROW_RIGHT,
                                                      CTK_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_SHADOW_TYPE,
                                   g_param_spec_enum ("shadow-type",
                                                      P_("Arrow shadow"),
                                                      P_("Appearance of the shadow surrounding the arrow"),
						      CTK_TYPE_SHADOW_TYPE,
						      CTK_SHADOW_OUT,
                                                      CTK_PARAM_READWRITE));

  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_float ("arrow-scaling",
                                                               P_("Arrow Scaling"),
                                                               P_("Amount of space used up by arrow"),
                                                               0.0, 1.0, 0.7,
                                                               CTK_PARAM_READABLE));

  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_ARROW_ACCESSIBLE);
}

static void
ctk_arrow_set_property (GObject         *object,
			guint            prop_id,
			const GValue    *value,
			GParamSpec      *pspec)
{
  CtkArrow *arrow = CTK_ARROW (object);
  CtkArrowPrivate *priv = arrow->priv;

  switch (prop_id)
    {
    case PROP_ARROW_TYPE:
      ctk_arrow_set (arrow,
		     g_value_get_enum (value),
		     priv->shadow_type);
      break;
    case PROP_SHADOW_TYPE:
      ctk_arrow_set (arrow,
		     priv->arrow_type,
		     g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_arrow_get_property (GObject         *object,
			guint            prop_id,
			GValue          *value,
			GParamSpec      *pspec)
{
  CtkArrow *arrow = CTK_ARROW (object);
  CtkArrowPrivate *priv = arrow->priv;

  switch (prop_id)
    {
    case PROP_ARROW_TYPE:
      g_value_set_enum (value, priv->arrow_type);
      break;
    case PROP_SHADOW_TYPE:
      g_value_set_enum (value, priv->shadow_type);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_arrow_init (CtkArrow *arrow)
{
  arrow->priv = ctk_arrow_get_instance_private (arrow);

  ctk_widget_set_has_window (CTK_WIDGET (arrow), FALSE);

  arrow->priv->arrow_type = CTK_ARROW_RIGHT;
  arrow->priv->shadow_type = CTK_SHADOW_OUT;
}

static void
ctk_arrow_get_preferred_width (CtkWidget *widget,
                               gint      *minimum_size,
                               gint      *natural_size)
{
  CtkBorder border;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  _ctk_misc_get_padding_and_border (CTK_MISC (widget), &border);
G_GNUC_END_IGNORE_DEPRECATIONS

  *minimum_size = MIN_ARROW_SIZE + border.left + border.right;
  *natural_size = MIN_ARROW_SIZE + border.left + border.right;
}

static void
ctk_arrow_get_preferred_height (CtkWidget *widget,
                                gint      *minimum_size,
                                gint      *natural_size)
{
  CtkBorder border;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  _ctk_misc_get_padding_and_border (CTK_MISC (widget), &border);
G_GNUC_END_IGNORE_DEPRECATIONS

  *minimum_size = MIN_ARROW_SIZE + border.top + border.bottom;
  *natural_size = MIN_ARROW_SIZE + border.top + border.bottom;
}

/**
 * ctk_arrow_new:
 * @arrow_type: a valid #CtkArrowType.
 * @shadow_type: a valid #CtkShadowType.
 *
 * Creates a new #CtkArrow widget.
 *
 * Returns: the new #CtkArrow widget.
 *
 * Deprecated: 3.14: Use a #CtkImage with a suitable icon.
 */
CtkWidget*
ctk_arrow_new (CtkArrowType  arrow_type,
	       CtkShadowType shadow_type)
{
  CtkArrowPrivate *priv;
  CtkArrow *arrow;

  arrow = g_object_new (CTK_TYPE_ARROW, NULL);

  priv = arrow->priv;

  priv->arrow_type = arrow_type;
  priv->shadow_type = shadow_type;

  return CTK_WIDGET (arrow);
}

/**
 * ctk_arrow_set:
 * @arrow: a widget of type #CtkArrow.
 * @arrow_type: a valid #CtkArrowType.
 * @shadow_type: a valid #CtkShadowType.
 *
 * Sets the direction and style of the #CtkArrow, @arrow.
 *
 * Deprecated: 3.14: Use a #CtkImage with a suitable icon.
 */
void
ctk_arrow_set (CtkArrow      *arrow,
	       CtkArrowType   arrow_type,
	       CtkShadowType  shadow_type)
{
  CtkArrowPrivate *priv;

  g_return_if_fail (CTK_IS_ARROW (arrow));

  priv = arrow->priv;

  if (priv->arrow_type != arrow_type
      || priv->shadow_type != shadow_type)
    {
      CtkWidget *widget;

      g_object_freeze_notify (G_OBJECT (arrow));

      if ((CtkArrowType) priv->arrow_type != arrow_type)
        {
          priv->arrow_type = arrow_type;
          g_object_notify (G_OBJECT (arrow), "arrow-type");
        }

      if (priv->shadow_type != shadow_type)
        {
          priv->shadow_type = shadow_type;
          g_object_notify (G_OBJECT (arrow), "shadow-type");
        }

      g_object_thaw_notify (G_OBJECT (arrow));

      widget = CTK_WIDGET (arrow);
      if (ctk_widget_is_drawable (widget))
        ctk_widget_queue_draw (widget);
    }
}

static gboolean
ctk_arrow_draw (CtkWidget *widget,
                cairo_t   *cr)
{
  CtkArrow *arrow = CTK_ARROW (widget);
  CtkArrowPrivate *priv = arrow->priv;
  CtkStyleContext *context;
  gdouble x, y;
  gint width, height;
  gint extent;
  CtkBorder border;
  gfloat xalign, yalign;
  CtkArrowType effective_arrow_type;
  gfloat arrow_scaling;
  gdouble angle;

  if (priv->arrow_type == CTK_ARROW_NONE)
    return FALSE;

  context = ctk_widget_get_style_context (widget);
  ctk_widget_style_get (widget, "arrow-scaling", &arrow_scaling, NULL);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  _ctk_misc_get_padding_and_border (CTK_MISC (widget), &border);
  ctk_misc_get_alignment (CTK_MISC (widget), &xalign, &yalign);
G_GNUC_END_IGNORE_DEPRECATIONS

  width = ctk_widget_get_allocated_width (widget) - border.left - border.right;
  height = ctk_widget_get_allocated_height (widget) - border.top - border.bottom;

  extent = MIN (width, height) * arrow_scaling;
  effective_arrow_type = priv->arrow_type;

  if (ctk_widget_get_direction (widget) != CTK_TEXT_DIR_LTR)
    {
      xalign = 1.0 - xalign;
      if (priv->arrow_type == CTK_ARROW_LEFT)
        effective_arrow_type = CTK_ARROW_RIGHT;
      else if (priv->arrow_type == CTK_ARROW_RIGHT)
        effective_arrow_type = CTK_ARROW_LEFT;
    }

  x = border.left + ((width - extent) * xalign);
  y = border.top + ((height - extent) * yalign);

  switch (effective_arrow_type)
    {
    case CTK_ARROW_UP:
      angle = 0;
      break;
    case CTK_ARROW_RIGHT:
      angle = G_PI / 2;
      break;
    case CTK_ARROW_DOWN:
      angle = G_PI;
      break;
    case CTK_ARROW_LEFT:
    default:
      angle = (3 * G_PI) / 2;
      break;
    }

  ctk_render_arrow (context, cr, angle, x, y, extent);

  return FALSE;
}
