/* CDK - The GIMP Drawing Kit
 * Copyright 2016  Endless
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

/**
 * SECTION:cdkdrawingcontext
 * @Title: CdkDrawingContext
 * @Short_description: Drawing context for CDK windows
 *
 * #CdkDrawingContext is an object that represents the current drawing
 * state of a #CdkWindow.
 *
 * It's possible to use a #CdkDrawingContext to draw on a #CdkWindow
 * via rendering API like Cairo or OpenGL.
 *
 * A #CdkDrawingContext can only be created by calling cdk_window_begin_draw_frame()
 * and will be valid until a call to cdk_window_end_draw_frame().
 *
 * #CdkDrawingContext is available since CDK 3.22
 */

#include "config.h"

#include <cairo-gobject.h>

#include "cdkdrawingcontextprivate.h"

#include "cdkrectangle.h"
#include "cdkinternals.h"
#include "cdkintl.h"
#include "cdkframeclockidle.h"
#include "cdkwindowimpl.h"
#include "cdkglcontextprivate.h"
#include "cdk-private.h"

G_DEFINE_TYPE (CdkDrawingContext, cdk_drawing_context, G_TYPE_OBJECT)

enum {
  PROP_0,

  PROP_WINDOW,
  PROP_CLIP,

  N_PROPS
};

static GParamSpec *obj_property[N_PROPS];

static void
cdk_drawing_context_dispose (GObject *gobject)
{
  CdkDrawingContext *self = CDK_DRAWING_CONTEXT (gobject);

  /* Unset the drawing context, in case somebody is holding
   * onto the Cairo context
   */
  if (self->cr != NULL)
    cdk_cairo_set_drawing_context (self->cr, NULL);

  g_clear_object (&self->window);
  g_clear_pointer (&self->clip, cairo_region_destroy);
  g_clear_pointer (&self->cr, cairo_destroy);

  G_OBJECT_CLASS (cdk_drawing_context_parent_class)->dispose (gobject);
}

static void
cdk_drawing_context_set_property (GObject      *gobject,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  CdkDrawingContext *self = CDK_DRAWING_CONTEXT (gobject);

  switch (prop_id)
    {
    case PROP_WINDOW:
      self->window = g_value_dup_object (value);
      break;

    case PROP_CLIP:
      self->clip = g_value_dup_boxed (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
cdk_drawing_context_get_property (GObject    *gobject,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  CdkDrawingContext *self = CDK_DRAWING_CONTEXT (gobject);

  switch (prop_id)
    {
    case PROP_WINDOW:
      g_value_set_object (value, self->window);
      break;

    case PROP_CLIP:
      g_value_set_boxed (value, self->clip);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
cdk_drawing_context_constructed (GObject *gobject)
{
  CdkDrawingContext *self = CDK_DRAWING_CONTEXT (gobject);

  if (self->window == NULL)
    {
      g_critical ("The drawing context of type %s does not have a window "
                  "associated to it. Drawing contexts can only be created "
                  "using cdk_window_begin_draw_frame().",
                  G_OBJECT_TYPE_NAME (gobject));
    }

  G_OBJECT_CLASS (cdk_drawing_context_parent_class)->constructed (gobject);
}

static void
cdk_drawing_context_class_init (CdkDrawingContextClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructed = cdk_drawing_context_constructed;
  gobject_class->set_property = cdk_drawing_context_set_property;
  gobject_class->get_property = cdk_drawing_context_get_property;
  gobject_class->dispose = cdk_drawing_context_dispose;

  /**
   * CdkDrawingContext:window:
   *
   * The #CdkWindow that created the drawing context.
   *
   * Since: 3.22
   */
  obj_property[PROP_WINDOW] =
    g_param_spec_object ("window", "Window", "The window that created the context",
                         CDK_TYPE_WINDOW,
                         G_PARAM_CONSTRUCT_ONLY |
                         G_PARAM_READWRITE |
                         G_PARAM_STATIC_STRINGS);
  /**
   * CdkDrawingContext:clip:
   *
   * The clip region applied to the drawing context.
   *
   * Since: 3.22
   */
  obj_property[PROP_CLIP] =
    g_param_spec_boxed ("clip", "Clip", "The clip region of the context",
                        CAIRO_GOBJECT_TYPE_REGION,
                        G_PARAM_CONSTRUCT_ONLY |
                        G_PARAM_READWRITE |
                        G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class, N_PROPS, obj_property);
}

static void
cdk_drawing_context_init (CdkDrawingContext *self)
{
}

static const cairo_user_data_key_t draw_context_key;

void
cdk_cairo_set_drawing_context (cairo_t           *cr,
                               CdkDrawingContext *context)
{
  cairo_set_user_data (cr, &draw_context_key, context, NULL);
}

/**
 * cdk_cairo_get_drawing_context:
 * @cr: a Cairo context
 *
 * Retrieves the #CdkDrawingContext that created the Cairo
 * context @cr.
 *
 * Returns: (transfer none) (nullable): a #CdkDrawingContext, if any is set
 *
 * Since: 3.22
 */
CdkDrawingContext *
cdk_cairo_get_drawing_context (cairo_t *cr)
{
  g_return_val_if_fail (cr != NULL, NULL);

  return cairo_get_user_data (cr, &draw_context_key);
}

/**
 * cdk_drawing_context_get_cairo_context:
 * @context: a #CdkDrawingContext
 *
 * Retrieves a Cairo context to be used to draw on the #CdkWindow
 * that created the #CdkDrawingContext.
 *
 * The returned context is guaranteed to be valid as long as the
 * #CdkDrawingContext is valid, that is between a call to
 * cdk_window_begin_draw_frame() and cdk_window_end_draw_frame().
 *
 * Returns: (transfer none): a Cairo context to be used to draw
 *   the contents of the #CdkWindow. The context is owned by the
 *   #CdkDrawingContext and should not be destroyed
 *
 * Since: 3.22
 */
cairo_t *
cdk_drawing_context_get_cairo_context (CdkDrawingContext *context)
{
  g_return_val_if_fail (CDK_IS_DRAWING_CONTEXT (context), NULL);
  g_return_val_if_fail (CDK_IS_WINDOW (context->window), NULL);

  if (context->cr == NULL)
    {
      cairo_region_t *region;
      cairo_surface_t *surface;

      surface = _cdk_window_ref_cairo_surface (context->window);
      context->cr = cairo_create (surface);

      cdk_cairo_set_drawing_context (context->cr, context);

      region = cdk_window_get_current_paint_region (context->window);
      cairo_region_union (region, context->clip);
      cdk_cairo_region (context->cr, region);
      cairo_clip (context->cr);

      cairo_region_destroy (region);
      cairo_surface_destroy (surface);
    }

  return context->cr;
}

/**
 * cdk_drawing_context_get_window:
 * @context: a #CdkDrawingContext
 *
 * Retrieves the window that created the drawing @context.
 *
 * Returns: (transfer none): a #CdkWindow
 *
 * Since: 3.22
 */
CdkWindow *
cdk_drawing_context_get_window (CdkDrawingContext *context)
{
  g_return_val_if_fail (CDK_IS_DRAWING_CONTEXT (context), NULL);

  return context->window;
}

/**
 * cdk_drawing_context_get_clip:
 * @context: a #CdkDrawingContext
 *
 * Retrieves a copy of the clip region used when creating the @context.
 *
 * Returns: (transfer full) (nullable): a Cairo region
 *
 * Since: 3.22
 */
cairo_region_t *
cdk_drawing_context_get_clip (CdkDrawingContext *context)
{
  g_return_val_if_fail (CDK_IS_DRAWING_CONTEXT (context), NULL);

  if (context->clip == NULL)
    return NULL;

  return cairo_region_copy (context->clip);
}

/**
 * cdk_drawing_context_is_valid:
 * @context: a #CdkDrawingContext
 *
 * Checks whether the given #CdkDrawingContext is valid.
 *
 * Returns: %TRUE if the context is valid
 *
 * Since: 3.22
 */
gboolean
cdk_drawing_context_is_valid (CdkDrawingContext *context)
{
  g_return_val_if_fail (CDK_IS_DRAWING_CONTEXT (context), FALSE);

  if (context->window == NULL)
    return FALSE;

  if (cdk_window_get_drawing_context (context->window) != context)
    return FALSE;

  return TRUE;
}
