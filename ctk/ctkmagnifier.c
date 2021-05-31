/* GTK - The GIMP Toolkit
 * Copyright © 2013 Carlos Garnacho <carlosg@gnome.org>
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
#include "ctk/ctk.h"
#include "ctkmagnifierprivate.h"
#include "ctkintl.h"

enum {
  PROP_INSPECTED = 1,
  PROP_RESIZE,
  PROP_MAGNIFICATION
};

typedef struct _CtkMagnifierPrivate CtkMagnifierPrivate;

struct _CtkMagnifierPrivate
{
  CtkWidget *inspected;
  gdouble magnification;
  gint x;
  gint y;
  gboolean resize;
  gulong draw_handler;
  gulong resize_handler;
};

G_DEFINE_TYPE_WITH_PRIVATE (CtkMagnifier, _ctk_magnifier,
                            CTK_TYPE_WIDGET)

static void
_ctk_magnifier_set_property (GObject      *object,
                             guint         param_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  switch (param_id)
    {
    case PROP_INSPECTED:
      _ctk_magnifier_set_inspected (CTK_MAGNIFIER (object),
                                    g_value_get_object (value));
      break;
    case PROP_MAGNIFICATION:
      _ctk_magnifier_set_magnification (CTK_MAGNIFIER (object),
                                        g_value_get_double (value));
      break;
    case PROP_RESIZE:
      _ctk_magnifier_set_resize (CTK_MAGNIFIER (object),
                                 g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
    }
}

static void
_ctk_magnifier_get_property (GObject    *object,
                             guint       param_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  CtkMagnifier *magnifier;
  CtkMagnifierPrivate *priv;

  magnifier = CTK_MAGNIFIER (object);
  priv = _ctk_magnifier_get_instance_private (magnifier);

  switch (param_id)
    {
    case PROP_INSPECTED:
      g_value_set_object (value, priv->inspected);
      break;
    case PROP_MAGNIFICATION:
      g_value_set_double (value, priv->magnification);
      break;
    case PROP_RESIZE:
      g_value_set_boolean (value, priv->resize);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
    }
}

static gboolean
_ctk_magnifier_draw (CtkWidget *widget,
                     cairo_t   *cr)
{
  CtkAllocation allocation, inspected_alloc;
  CtkMagnifier *magnifier;
  CtkMagnifierPrivate *priv;
  gdouble x, y;

  magnifier = CTK_MAGNIFIER (widget);
  priv = _ctk_magnifier_get_instance_private (magnifier);

  if (priv->inspected == NULL)
    return FALSE;

  if (!ctk_widget_is_visible (priv->inspected))
    return FALSE;

  ctk_widget_get_allocation (widget, &allocation);
  ctk_widget_get_allocation (priv->inspected, &inspected_alloc);

  if (!priv->resize)
    cairo_translate (cr, allocation.width / 2, allocation.height / 2);

  x = CLAMP (priv->x, 0, inspected_alloc.width);
  y = CLAMP (priv->y, 0, inspected_alloc.height);

  cairo_save (cr);
  cairo_scale (cr, priv->magnification, priv->magnification);
  cairo_translate (cr, -x, -y);
  g_signal_handler_block (priv->inspected, priv->draw_handler);
  ctk_widget_draw (priv->inspected, cr);
  g_signal_handler_unblock (priv->inspected, priv->draw_handler);
  cairo_restore (cr);

  return TRUE;
}

static void
ctk_magnifier_get_preferred_width (CtkWidget *widget,
                                   gint      *minimum_width,
                                   gint      *natural_width)
{
  CtkMagnifier *magnifier;
  CtkMagnifierPrivate *priv;
  gint width;

  magnifier = CTK_MAGNIFIER (widget);
  priv = _ctk_magnifier_get_instance_private (magnifier);

  if (priv->resize && priv->inspected)
    width = priv->magnification * ctk_widget_get_allocated_width (priv->inspected);
  else
    width = 0;

  *minimum_width = width;
  *natural_width = width;
}

static void
ctk_magnifier_get_preferred_height (CtkWidget *widget,
                                    gint      *minimum_height,
                                    gint      *natural_height)
{
  CtkMagnifier *magnifier;
  CtkMagnifierPrivate *priv;
  gint height;

  magnifier = CTK_MAGNIFIER (widget);
  priv = _ctk_magnifier_get_instance_private (magnifier);

  if (priv->resize && priv->inspected)
    height = priv->magnification * ctk_widget_get_allocated_height (priv->inspected);
  else
    height = 0;

  *minimum_height = height;
  *natural_height = height;
}

static void
resize_handler (CtkWidget     *widget,
                CtkAllocation *alloc,
                CtkWidget     *magnifier)
{
  ctk_widget_queue_resize (magnifier);
}

static void
connect_resize_handler (CtkMagnifier *magnifier)
{
  CtkMagnifierPrivate *priv;

  priv = _ctk_magnifier_get_instance_private (magnifier);

  if (priv->inspected && priv->resize)
    priv->resize_handler = g_signal_connect (priv->inspected, "size-allocate",
                                             G_CALLBACK (resize_handler), magnifier);
}

static void
disconnect_resize_handler (CtkMagnifier *magnifier)
{
  CtkMagnifierPrivate *priv;

  priv = _ctk_magnifier_get_instance_private (magnifier);

  if (priv->resize_handler)
    {
      if (priv->inspected)
        g_signal_handler_disconnect (priv->inspected, priv->resize_handler);
      priv->resize_handler = 0;
    }
}

static gboolean
draw_handler (CtkWidget     *widget,
              cairo_t       *cr,
              CtkWidget     *magnifier)
{
  ctk_widget_queue_draw (magnifier);
  return FALSE;
}

static void
connect_draw_handler (CtkMagnifier *magnifier)
{
  CtkMagnifierPrivate *priv;

  priv = _ctk_magnifier_get_instance_private (magnifier);

  if (!priv->draw_handler)
    {
      if (priv->inspected)
        priv->draw_handler = g_signal_connect (priv->inspected, "draw",
                                               G_CALLBACK (draw_handler), magnifier);
    }
}

static void
disconnect_draw_handler (CtkMagnifier *magnifier)
{
  CtkMagnifierPrivate *priv;

  priv = _ctk_magnifier_get_instance_private (magnifier);

  if (priv->draw_handler)
    {
      if (priv->inspected)
        g_signal_handler_disconnect (priv->inspected, priv->draw_handler);
      priv->draw_handler = 0;
    }
}

static void
_ctk_magnifier_destroy (CtkWidget *widget)
{
  _ctk_magnifier_set_inspected (CTK_MAGNIFIER (widget), NULL);

  CTK_WIDGET_CLASS (_ctk_magnifier_parent_class)->destroy (widget);
}

static void
ctk_magnifier_map (CtkWidget *widget)
{
  connect_draw_handler (CTK_MAGNIFIER (widget));

  CTK_WIDGET_CLASS (_ctk_magnifier_parent_class)->map (widget);
}

static void
ctk_magnifier_unmap (CtkWidget *widget)
{
  CTK_WIDGET_CLASS (_ctk_magnifier_parent_class)->unmap (widget);

  disconnect_draw_handler (CTK_MAGNIFIER (widget));
}

static void
_ctk_magnifier_class_init (CtkMagnifierClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);

  object_class->set_property = _ctk_magnifier_set_property;
  object_class->get_property = _ctk_magnifier_get_property;

  widget_class->destroy = _ctk_magnifier_destroy;
  widget_class->draw = _ctk_magnifier_draw;
  widget_class->get_preferred_width = ctk_magnifier_get_preferred_width;
  widget_class->get_preferred_height = ctk_magnifier_get_preferred_height;
  widget_class->map = ctk_magnifier_map;
  widget_class->unmap = ctk_magnifier_unmap;

  g_object_class_install_property (object_class,
                                   PROP_INSPECTED,
                                   g_param_spec_object ("inspected",
                                                        P_("Inspected"),
                                                        P_("Inspected widget"),
                                                        CTK_TYPE_WIDGET,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_MAGNIFICATION,
                                   g_param_spec_double ("magnification",
                                                        P_("magnification"),
                                                        P_("magnification"),
                                                        1, G_MAXDOUBLE, 1,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_RESIZE,
                                   g_param_spec_boolean ("resize",
                                                         P_("resize"),
                                                         P_("resize"),
                                                         FALSE,
                                                         G_PARAM_READWRITE));
}

static void
_ctk_magnifier_init (CtkMagnifier *magnifier)
{
  CtkWidget *widget = CTK_WIDGET (magnifier);
  CtkMagnifierPrivate *priv;

  priv = _ctk_magnifier_get_instance_private (magnifier);

  ctk_widget_set_events (widget,
                         ctk_widget_get_events (widget));

  ctk_widget_set_has_window (widget, FALSE);
  priv->magnification = 1;
  priv->resize = FALSE;
}

CtkWidget *
_ctk_magnifier_new (CtkWidget *inspected)
{
  g_return_val_if_fail (CTK_IS_WIDGET (inspected), NULL);

  return g_object_new (CTK_TYPE_MAGNIFIER,
                       "inspected", inspected,
                       NULL);
}

CtkWidget *
_ctk_magnifier_get_inspected (CtkMagnifier *magnifier)
{
  CtkMagnifierPrivate *priv;

  g_return_val_if_fail (CTK_IS_MAGNIFIER (magnifier), NULL);

  priv = _ctk_magnifier_get_instance_private (magnifier);

  return priv->inspected;
}

void
_ctk_magnifier_set_inspected (CtkMagnifier *magnifier,
                              CtkWidget    *inspected)
{
  CtkMagnifierPrivate *priv;

  g_return_if_fail (CTK_IS_MAGNIFIER (magnifier));

  priv = _ctk_magnifier_get_instance_private (magnifier);

  if (priv->inspected == inspected)
    return;

  disconnect_draw_handler (magnifier);
  disconnect_resize_handler (magnifier);

  if (priv->inspected)
    g_object_remove_weak_pointer (G_OBJECT (priv->inspected),
                                  (gpointer *) &priv->inspected);
  priv->inspected = inspected;
  if (priv->inspected)
    g_object_add_weak_pointer (G_OBJECT (priv->inspected),
                               (gpointer *) &priv->inspected);

  if (ctk_widget_get_mapped (CTK_WIDGET (magnifier)))
    connect_draw_handler (magnifier);
  connect_resize_handler (magnifier);

  g_object_notify (G_OBJECT (magnifier), "inspected");
}

void
_ctk_magnifier_set_coords (CtkMagnifier *magnifier,
                           gdouble       x,
                           gdouble       y)
{
  CtkMagnifierPrivate *priv;

  g_return_if_fail (CTK_IS_MAGNIFIER (magnifier));

  priv = _ctk_magnifier_get_instance_private (magnifier);

  if (priv->x == x && priv->y == y)
    return;

  priv->x = x;
  priv->y = y;

  if (ctk_widget_is_visible (CTK_WIDGET (magnifier)))
    ctk_widget_queue_draw (CTK_WIDGET (magnifier));
}

void
_ctk_magnifier_get_coords (CtkMagnifier *magnifier,
                           gdouble      *x,
                           gdouble      *y)
{
  CtkMagnifierPrivate *priv;

  g_return_if_fail (CTK_IS_MAGNIFIER (magnifier));

  priv = _ctk_magnifier_get_instance_private (magnifier);

  if (x)
    *x = priv->x;

  if (y)
    *y = priv->y;
}

void
_ctk_magnifier_set_magnification (CtkMagnifier *magnifier,
                                  gdouble       magnification)
{
  CtkMagnifierPrivate *priv;

  g_return_if_fail (CTK_IS_MAGNIFIER (magnifier));

  priv = _ctk_magnifier_get_instance_private (magnifier);

  if (priv->magnification == magnification)
    return;

  priv->magnification = magnification;
  g_object_notify (G_OBJECT (magnifier), "magnification");

  if (priv->resize)
    ctk_widget_queue_resize (CTK_WIDGET (magnifier));

  if (ctk_widget_is_visible (CTK_WIDGET (magnifier)))
    ctk_widget_queue_draw (CTK_WIDGET (magnifier));
}

gdouble
_ctk_magnifier_get_magnification (CtkMagnifier *magnifier)
{
  CtkMagnifierPrivate *priv;

  g_return_val_if_fail (CTK_IS_MAGNIFIER (magnifier), 1);

  priv = _ctk_magnifier_get_instance_private (magnifier);

  return priv->magnification;
}

void
_ctk_magnifier_set_resize (CtkMagnifier *magnifier,
                           gboolean      resize)
{
  CtkMagnifierPrivate *priv;

  g_return_if_fail (CTK_IS_MAGNIFIER (magnifier));

  priv = _ctk_magnifier_get_instance_private (magnifier);

  if (priv->resize == resize)
    return;

  priv->resize = resize;

  ctk_widget_queue_resize (CTK_WIDGET (magnifier));
  if (resize)
    connect_resize_handler (magnifier);
  else
    disconnect_resize_handler (magnifier);
}

gboolean
_ctk_magnifier_get_resize (CtkMagnifier *magnifier)
{
  CtkMagnifierPrivate *priv;

  g_return_val_if_fail (CTK_IS_MAGNIFIER (magnifier), FALSE);

  priv = _ctk_magnifier_get_instance_private (magnifier);

  return priv->resize;
}
