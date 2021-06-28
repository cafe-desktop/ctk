/* GDK - The GIMP Drawing Kit
 *
 * cdkglcontext.c: GL context abstraction
 * 
 * Copyright © 2014  Emmanuele Bassi
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * SECTION:cdkglcontext
 * @Title: CdkGLContext
 * @Short_description: OpenGL context
 *
 * #CdkGLContext is an object representing the platform-specific
 * OpenGL drawing context.
 *
 * #CdkGLContexts are created for a #CdkWindow using
 * cdk_window_create_gl_context(), and the context will match
 * the #CdkVisual of the window.
 *
 * A #CdkGLContext is not tied to any particular normal framebuffer.
 * For instance, it cannot draw to the #CdkWindow back buffer. The GDK
 * repaint system is in full control of the painting to that. Instead,
 * you can create render buffers or textures and use cdk_cairo_draw_from_gl()
 * in the draw function of your widget to draw them. Then GDK will handle
 * the integration of your rendering with that of other widgets.
 *
 * Support for #CdkGLContext is platform-specific, context creation
 * can fail, returning %NULL context.
 *
 * A #CdkGLContext has to be made "current" in order to start using
 * it, otherwise any OpenGL call will be ignored.
 *
 * ## Creating a new OpenGL context ##
 *
 * In order to create a new #CdkGLContext instance you need a
 * #CdkWindow, which you typically get during the realize call
 * of a widget.
 *
 * A #CdkGLContext is not realized until either cdk_gl_context_make_current(),
 * or until it is realized using cdk_gl_context_realize(). It is possible to
 * specify details of the GL context like the OpenGL version to be used, or
 * whether the GL context should have extra state validation enabled after
 * calling cdk_window_create_gl_context() by calling cdk_gl_context_realize().
 * If the realization fails you have the option to change the settings of the
 * #CdkGLContext and try again.
 *
 * ## Using a CdkGLContext ##
 *
 * You will need to make the #CdkGLContext the current context
 * before issuing OpenGL calls; the system sends OpenGL commands to
 * whichever context is current. It is possible to have multiple
 * contexts, so you always need to ensure that the one which you
 * want to draw with is the current one before issuing commands:
 *
 * |[<!-- language="C" -->
 *   cdk_gl_context_make_current (context);
 * ]|
 *
 * You can now perform your drawing using OpenGL commands.
 *
 * You can check which #CdkGLContext is the current one by using
 * cdk_gl_context_get_current(); you can also unset any #CdkGLContext
 * that is currently set by calling cdk_gl_context_clear_current().
 */

#include "config.h"

#include "cdkglcontextprivate.h"
#include "cdkdisplayprivate.h"
#include "cdkinternals.h"

#include "cdkintl.h"
#include "cdk-private.h"

#include <epoxy/gl.h>

typedef struct {
  CdkDisplay *display;
  CdkWindow *window;
  CdkGLContext *shared_context;

  int major;
  int minor;
  int gl_version;

  guint realized : 1;
  guint use_texture_rectangle : 1;
  guint has_gl_framebuffer_blit : 1;
  guint has_frame_terminator : 1;
  guint has_unpack_subimage : 1;
  guint extensions_checked : 1;
  guint debug_enabled : 1;
  guint forward_compatible : 1;
  guint is_legacy : 1;

  int use_es;

  CdkGLContextPaintData *paint_data;
} CdkGLContextPrivate;

enum {
  PROP_0,

  PROP_DISPLAY,
  PROP_WINDOW,
  PROP_SHARED_CONTEXT,

  LAST_PROP
};

static GParamSpec *obj_pspecs[LAST_PROP] = { NULL, };

G_DEFINE_QUARK (cdk-gl-error-quark, cdk_gl_error)

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (CdkGLContext, cdk_gl_context, G_TYPE_OBJECT)

static GPrivate thread_current_context = G_PRIVATE_INIT (g_object_unref);

static void
cdk_gl_context_dispose (GObject *gobject)
{
  CdkGLContext *context = GDK_GL_CONTEXT (gobject);
  CdkGLContextPrivate *priv = cdk_gl_context_get_instance_private (context);
  CdkGLContext *current;

  current = g_private_get (&thread_current_context);
  if (current == context)
    g_private_replace (&thread_current_context, NULL);

  g_clear_object (&priv->display);
  g_clear_object (&priv->window);
  g_clear_object (&priv->shared_context);

  G_OBJECT_CLASS (cdk_gl_context_parent_class)->dispose (gobject);
}

static void
cdk_gl_context_finalize (GObject *gobject)
{
  CdkGLContext *context = GDK_GL_CONTEXT (gobject);
  CdkGLContextPrivate *priv = cdk_gl_context_get_instance_private (context);

  g_clear_pointer (&priv->paint_data, g_free);
  G_OBJECT_CLASS (cdk_gl_context_parent_class)->finalize (gobject);
}

static void
cdk_gl_context_set_property (GObject      *gobject,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  CdkGLContextPrivate *priv = cdk_gl_context_get_instance_private ((CdkGLContext *) gobject);

  switch (prop_id)
    {
    case PROP_DISPLAY:
      {
        CdkDisplay *display = g_value_get_object (value);

        if (display)
          g_object_ref (display);

        if (priv->display)
          g_object_unref (priv->display);

        priv->display = display;
      }
      break;

    case PROP_WINDOW:
      {
        CdkWindow *window = g_value_get_object (value);

        if (window)
          g_object_ref (window);

        if (priv->window)
          g_object_unref (priv->window);

        priv->window = window;
      }
      break;

    case PROP_SHARED_CONTEXT:
      {
        CdkGLContext *context = g_value_get_object (value);

        if (context != NULL)
          priv->shared_context = g_object_ref (context);
      }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
cdk_gl_context_get_property (GObject    *gobject,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  CdkGLContextPrivate *priv = cdk_gl_context_get_instance_private ((CdkGLContext *) gobject);

  switch (prop_id)
    {
    case PROP_DISPLAY:
      g_value_set_object (value, priv->display);
      break;

    case PROP_WINDOW:
      g_value_set_object (value, priv->window);
      break;

    case PROP_SHARED_CONTEXT:
      g_value_set_object (value, priv->shared_context);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

void
cdk_gl_context_upload_texture (CdkGLContext    *context,
                               cairo_surface_t *image_surface,
                               int              width,
                               int              height,
                               guint            texture_target)
{
  CdkGLContextPrivate *priv = cdk_gl_context_get_instance_private (context);

  g_return_if_fail (GDK_IS_GL_CONTEXT (context));

  /* GL_UNPACK_ROW_LENGTH is available on desktop GL, OpenGL ES >= 3.0, or if
   * the GL_EXT_unpack_subimage extension for OpenGL ES 2.0 is available
   */
  if (!priv->use_es ||
      (priv->use_es && (priv->gl_version >= 30 || priv->has_unpack_subimage)))
    {
      glPixelStorei (GL_UNPACK_ALIGNMENT, 4);
      glPixelStorei (GL_UNPACK_ROW_LENGTH, cairo_image_surface_get_stride (image_surface) / 4);

      if (priv->use_es)
        glTexImage2D (texture_target, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                      cairo_image_surface_get_data (image_surface));
      else
        glTexImage2D (texture_target, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,
                      cairo_image_surface_get_data (image_surface));

      glPixelStorei (GL_UNPACK_ROW_LENGTH, 0);
    }
  else
    {
      GLvoid *data = cairo_image_surface_get_data (image_surface);
      int stride = cairo_image_surface_get_stride (image_surface);
      int i;

      if (priv->use_es)
        {
          glTexImage2D (texture_target, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

          for (i = 0; i < height; i++)
            glTexSubImage2D (texture_target, 0, 0, i, width, 1, GL_RGBA, GL_UNSIGNED_BYTE, (unsigned char*) data + (i * stride));
        }
      else
        {
          glTexImage2D (texture_target, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, NULL);

          for (i = 0; i < height; i++)
            glTexSubImage2D (texture_target, 0, 0, i, width, 1, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, (unsigned char*) data + (i * stride));
        }
    }
}

static gboolean
cdk_gl_context_real_realize (CdkGLContext  *self,
                             GError       **error)
{
  g_set_error_literal (error, GDK_GL_ERROR, GDK_GL_ERROR_NOT_AVAILABLE,
                       "The current backend does not support OpenGL");

  return FALSE;
}

static void
cdk_gl_context_class_init (CdkGLContextClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  klass->realize = cdk_gl_context_real_realize;

  /**
   * CdkGLContext:display:
   *
   * The #CdkDisplay used to create the #CdkGLContext.
   *
   * Since: 3.16
   */
  obj_pspecs[PROP_DISPLAY] =
    g_param_spec_object ("display",
                         P_("Display"),
                         P_("The GDK display used to create the GL context"),
                         GDK_TYPE_DISPLAY,
                         G_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY |
                         G_PARAM_STATIC_STRINGS);

  /**
   * CdkGLContext:window:
   *
   * The #CdkWindow the gl context is bound to.
   *
   * Since: 3.16
   */
  obj_pspecs[PROP_WINDOW] =
    g_param_spec_object ("window",
                         P_("Window"),
                         P_("The GDK window bound to the GL context"),
                         GDK_TYPE_WINDOW,
                         G_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY |
                         G_PARAM_STATIC_STRINGS);

  /**
   * CdkGLContext:shared-context:
   *
   * The #CdkGLContext that this context is sharing data with, or %NULL
   *
   * Since: 3.16
   */
  obj_pspecs[PROP_SHARED_CONTEXT] =
    g_param_spec_object ("shared-context",
                         P_("Shared context"),
                         P_("The GL context this context shares data with"),
                         GDK_TYPE_GL_CONTEXT,
                         G_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY |
                         G_PARAM_STATIC_STRINGS);

  gobject_class->set_property = cdk_gl_context_set_property;
  gobject_class->get_property = cdk_gl_context_get_property;
  gobject_class->dispose = cdk_gl_context_dispose;
  gobject_class->finalize = cdk_gl_context_finalize;

  g_object_class_install_properties (gobject_class, LAST_PROP, obj_pspecs);
}

static void
cdk_gl_context_init (CdkGLContext *self)
{
  CdkGLContextPrivate *priv = cdk_gl_context_get_instance_private (self);

  priv->use_es = -1;
}

/*< private >
 * cdk_gl_context_end_frame:
 * @context: a #CdkGLContext
 * @painted: The area that has been redrawn this frame
 * @damage: The area that we know is actually different from the last frame
 *
 * Copies the back buffer to the front buffer.
 *
 * This function may call `glFlush()` implicitly before returning; it
 * is not recommended to call `glFlush()` explicitly before calling
 * this function.
 *
 * Since: 3.16
 */
void
cdk_gl_context_end_frame (CdkGLContext   *context,
                          cairo_region_t *painted,
                          cairo_region_t *damage)
{
  g_return_if_fail (GDK_IS_GL_CONTEXT (context));

  GDK_GL_CONTEXT_GET_CLASS (context)->end_frame (context, painted, damage);
}

CdkGLContextPaintData *
cdk_gl_context_get_paint_data (CdkGLContext *context)
{

  CdkGLContextPrivate *priv = cdk_gl_context_get_instance_private (context);

  if (priv->paint_data == NULL)
    {
      priv->paint_data = g_new0 (CdkGLContextPaintData, 1);
      priv->paint_data->is_legacy = priv->is_legacy;
      priv->paint_data->use_es = priv->use_es;
    }

  return priv->paint_data;
}

gboolean
cdk_gl_context_use_texture_rectangle (CdkGLContext *context)
{
  CdkGLContextPrivate *priv = cdk_gl_context_get_instance_private (context);

  return priv->use_texture_rectangle;
}

gboolean
cdk_gl_context_has_framebuffer_blit (CdkGLContext *context)
{
  CdkGLContextPrivate *priv = cdk_gl_context_get_instance_private (context);

  return priv->has_gl_framebuffer_blit;
}

gboolean
cdk_gl_context_has_frame_terminator (CdkGLContext *context)
{
  CdkGLContextPrivate *priv = cdk_gl_context_get_instance_private (context);

  return priv->has_frame_terminator;
}

gboolean
cdk_gl_context_has_unpack_subimage (CdkGLContext *context)
{
  CdkGLContextPrivate *priv = cdk_gl_context_get_instance_private (context);

  return priv->has_unpack_subimage;
}

/**
 * cdk_gl_context_set_debug_enabled:
 * @context: a #CdkGLContext
 * @enabled: whether to enable debugging in the context
 *
 * Sets whether the #CdkGLContext should perform extra validations and
 * run time checking. This is useful during development, but has
 * additional overhead.
 *
 * The #CdkGLContext must not be realized or made current prior to
 * calling this function.
 *
 * Since: 3.16
 */
void
cdk_gl_context_set_debug_enabled (CdkGLContext *context,
                                  gboolean      enabled)
{
  CdkGLContextPrivate *priv = cdk_gl_context_get_instance_private (context);

  g_return_if_fail (GDK_IS_GL_CONTEXT (context));
  g_return_if_fail (!priv->realized);

  enabled = !!enabled;

  priv->debug_enabled = enabled;
}

/**
 * cdk_gl_context_get_debug_enabled:
 * @context: a #CdkGLContext
 *
 * Retrieves the value set using cdk_gl_context_set_debug_enabled().
 *
 * Returns: %TRUE if debugging is enabled
 *
 * Since: 3.16
 */
gboolean
cdk_gl_context_get_debug_enabled (CdkGLContext *context)
{
  CdkGLContextPrivate *priv = cdk_gl_context_get_instance_private (context);

  g_return_val_if_fail (GDK_IS_GL_CONTEXT (context), FALSE);

  return priv->debug_enabled;
}

/**
 * cdk_gl_context_set_forward_compatible:
 * @context: a #CdkGLContext
 * @compatible: whether the context should be forward compatible
 *
 * Sets whether the #CdkGLContext should be forward compatible.
 *
 * Forward compatibile contexts must not support OpenGL functionality that
 * has been marked as deprecated in the requested version; non-forward
 * compatible contexts, on the other hand, must support both deprecated and
 * non deprecated functionality.
 *
 * The #CdkGLContext must not be realized or made current prior to calling
 * this function.
 *
 * Since: 3.16
 */
void
cdk_gl_context_set_forward_compatible (CdkGLContext *context,
                                       gboolean      compatible)
{
  CdkGLContextPrivate *priv = cdk_gl_context_get_instance_private (context);

  g_return_if_fail (GDK_IS_GL_CONTEXT (context));
  g_return_if_fail (!priv->realized);

  compatible = !!compatible;

  priv->forward_compatible = compatible;
}

/**
 * cdk_gl_context_get_forward_compatible:
 * @context: a #CdkGLContext
 *
 * Retrieves the value set using cdk_gl_context_set_forward_compatible().
 *
 * Returns: %TRUE if the context should be forward compatible
 *
 * Since: 3.16
 */
gboolean
cdk_gl_context_get_forward_compatible (CdkGLContext *context)
{
  CdkGLContextPrivate *priv = cdk_gl_context_get_instance_private (context);

  g_return_val_if_fail (GDK_IS_GL_CONTEXT (context), FALSE);

  return priv->forward_compatible;
}

/**
 * cdk_gl_context_set_required_version:
 * @context: a #CdkGLContext
 * @major: the major version to request
 * @minor: the minor version to request
 *
 * Sets the major and minor version of OpenGL to request.
 *
 * Setting @major and @minor to zero will use the default values.
 *
 * The #CdkGLContext must not be realized or made current prior to calling
 * this function.
 *
 * Since: 3.16
 */
void
cdk_gl_context_set_required_version (CdkGLContext *context,
                                     int           major,
                                     int           minor)
{
  CdkGLContextPrivate *priv = cdk_gl_context_get_instance_private (context);
  int version, min_ver;

  g_return_if_fail (GDK_IS_GL_CONTEXT (context));
  g_return_if_fail (!priv->realized);

  /* this will take care of the default */
  if (major == 0 && minor == 0)
    {
      priv->major = 0;
      priv->minor = 0;
      return;
    }

  /* Enforce a minimum context version number of 3.2 */
  version = (major * 100) + minor;

  if (priv->use_es > 0 || (_cdk_gl_flags & GDK_GL_GLES) != 0)
    min_ver = 200;
  else
    min_ver = 302;

  if (version < min_ver)
    {
      g_warning ("cdk_gl_context_set_required_version - GL context versions less than 3.2 are not supported.");
      version = min_ver;
    }
  priv->major = version / 100;
  priv->minor = version % 100;
}

/**
 * cdk_gl_context_get_required_version:
 * @context: a #CdkGLContext
 * @major: (out) (nullable): return location for the major version to request
 * @minor: (out) (nullable): return location for the minor version to request
 *
 * Retrieves the major and minor version requested by calling
 * cdk_gl_context_set_required_version().
 *
 * Since: 3.16
 */
void
cdk_gl_context_get_required_version (CdkGLContext *context,
                                     int          *major,
                                     int          *minor)
{
  CdkGLContextPrivate *priv = cdk_gl_context_get_instance_private (context);
  int default_major, default_minor;
  int maj, min;

  g_return_if_fail (GDK_IS_GL_CONTEXT (context));

  if (priv->use_es > 0 || (_cdk_gl_flags & GDK_GL_GLES) != 0)
    {
      default_major = 2;
      default_minor = 0;
    }
  else
    {
      default_major = 3;
      default_minor = 2;
    }

  if (priv->major > 0)
    maj = priv->major;
  else
    maj = default_major;

  if (priv->minor > 0)
    min = priv->minor;
  else
    min = default_minor;

  if (major != NULL)
    *major = maj;
  if (minor != NULL)
    *minor = min;
}

/**
 * cdk_gl_context_is_legacy:
 * @context: a #CdkGLContext
 *
 * Whether the #CdkGLContext is in legacy mode or not.
 *
 * The #CdkGLContext must be realized before calling this function.
 *
 * When realizing a GL context, GDK will try to use the OpenGL 3.2 core
 * profile; this profile removes all the OpenGL API that was deprecated
 * prior to the 3.2 version of the specification. If the realization is
 * successful, this function will return %FALSE.
 *
 * If the underlying OpenGL implementation does not support core profiles,
 * GDK will fall back to a pre-3.2 compatibility profile, and this function
 * will return %TRUE.
 *
 * You can use the value returned by this function to decide which kind
 * of OpenGL API to use, or whether to do extension discovery, or what
 * kind of shader programs to load.
 *
 * Returns: %TRUE if the GL context is in legacy mode
 *
 * Since: 3.20
 */
gboolean
cdk_gl_context_is_legacy (CdkGLContext *context)
{
  CdkGLContextPrivate *priv = cdk_gl_context_get_instance_private (context);

  g_return_val_if_fail (GDK_IS_GL_CONTEXT (context), FALSE);
  g_return_val_if_fail (priv->realized, FALSE);

  return priv->is_legacy;
}

void
cdk_gl_context_set_is_legacy (CdkGLContext *context,
                              gboolean      is_legacy)
{
  CdkGLContextPrivate *priv = cdk_gl_context_get_instance_private (context);

  priv->is_legacy = !!is_legacy;
}

/**
 * cdk_gl_context_set_use_es:
 * @context: a #CdkGLContext:
 * @use_es: whether the context should use OpenGL ES instead of OpenGL,
 *   or -1 to allow auto-detection
 *
 * Requests that GDK create a OpenGL ES context instead of an OpenGL one,
 * if the platform and windowing system allows it.
 *
 * The @context must not have been realized.
 *
 * By default, GDK will attempt to automatically detect whether the
 * underlying GL implementation is OpenGL or OpenGL ES once the @context
 * is realized.
 *
 * You should check the return value of cdk_gl_context_get_use_es() after
 * calling cdk_gl_context_realize() to decide whether to use the OpenGL or
 * OpenGL ES API, extensions, or shaders.
 *
 * Since: 3.22
 */
void
cdk_gl_context_set_use_es (CdkGLContext *context,
                           int           use_es)
{
  CdkGLContextPrivate *priv = cdk_gl_context_get_instance_private (context);

  g_return_if_fail (GDK_IS_GL_CONTEXT (context));
  g_return_if_fail (!priv->realized);

  if (priv->use_es != use_es)
    priv->use_es = use_es;
}

/**
 * cdk_gl_context_get_use_es:
 * @context: a #CdkGLContext
 *
 * Checks whether the @context is using an OpenGL or OpenGL ES profile.
 *
 * Returns: %TRUE if the #CdkGLContext is using an OpenGL ES profile
 *
 * Since: 3.22
 */
gboolean
cdk_gl_context_get_use_es (CdkGLContext *context)
{
  CdkGLContextPrivate *priv = cdk_gl_context_get_instance_private (context);

  g_return_val_if_fail (GDK_IS_GL_CONTEXT (context), FALSE);

  if (!priv->realized)
    return FALSE;

  return priv->use_es > 0;
}

/**
 * cdk_gl_context_realize:
 * @context: a #CdkGLContext
 * @error: return location for a #GError
 *
 * Realizes the given #CdkGLContext.
 *
 * It is safe to call this function on a realized #CdkGLContext.
 *
 * Returns: %TRUE if the context is realized
 *
 * Since: 3.16
 */
gboolean
cdk_gl_context_realize (CdkGLContext  *context,
                        GError       **error)
{
  CdkGLContextPrivate *priv = cdk_gl_context_get_instance_private (context);

  g_return_val_if_fail (GDK_IS_GL_CONTEXT (context), FALSE);

  if (priv->realized)
    return TRUE;

  priv->realized = GDK_GL_CONTEXT_GET_CLASS (context)->realize (context, error);

  return priv->realized;
}

static void
cdk_gl_context_check_extensions (CdkGLContext *context)
{
  CdkGLContextPrivate *priv = cdk_gl_context_get_instance_private (context);
  gboolean has_npot, has_texture_rectangle;

  if (!priv->realized)
    return;

  if (priv->extensions_checked)
    return;

  priv->gl_version = epoxy_gl_version ();

  if (priv->use_es < 0)
    priv->use_es = !epoxy_is_desktop_gl ();

  if (priv->use_es)
    {
      has_npot = priv->gl_version >= 20;
      has_texture_rectangle = FALSE;

      /* This should check for GL_NV_framebuffer_blit as well - see extension at:
       *
       * https://www.khronos.org/registry/gles/extensions/NV/NV_framebuffer_blit.txt
       *
       * for ANGLE, we can enable bit blitting if we have the
       * GL_ANGLE_framebuffer_blit extension
       */
      if (epoxy_has_gl_extension ("GL_ANGLE_framebuffer_blit"))
        priv->has_gl_framebuffer_blit = TRUE;
      else
        priv->has_gl_framebuffer_blit = FALSE;

      /* No OES version */
      priv->has_frame_terminator = FALSE;

      priv->has_unpack_subimage = epoxy_has_gl_extension ("GL_EXT_unpack_subimage");
    }
  else
    {
      has_npot = priv->gl_version >= 20 || epoxy_has_gl_extension ("GL_ARB_texture_non_power_of_two");
      has_texture_rectangle = priv->gl_version >= 31 || epoxy_has_gl_extension ("GL_ARB_texture_rectangle");

      priv->has_gl_framebuffer_blit = priv->gl_version >= 30 || epoxy_has_gl_extension ("GL_EXT_framebuffer_blit");
      priv->has_frame_terminator = epoxy_has_gl_extension ("GL_GREMEDY_frame_terminator");
      priv->has_unpack_subimage = TRUE;

      /* We asked for a core profile, but we didn't get one, so we're in legacy mode */
      if (priv->gl_version < 32)
        priv->is_legacy = TRUE;
    }

  if (!priv->use_es && G_UNLIKELY (_cdk_gl_flags & GDK_GL_TEXTURE_RECTANGLE))
    priv->use_texture_rectangle = TRUE;
  else if (has_npot)
    priv->use_texture_rectangle = FALSE;
  else if (has_texture_rectangle)
    priv->use_texture_rectangle = TRUE;
  else
    g_warning ("GL implementation doesn't support any form of non-power-of-two textures");

  GDK_NOTE (OPENGL,
            g_message ("%s version: %d.%d (%s)\n"
                       "* GLSL version: %s\n"
                       "* Extensions checked:\n"
                       " - GL_ARB_texture_non_power_of_two: %s\n"
                       " - GL_ARB_texture_rectangle: %s\n"
                       " - GL_EXT_framebuffer_blit: %s\n"
                       " - GL_GREMEDY_frame_terminator: %s\n"
                       "* Using texture rectangle: %s",
                       priv->use_es ? "OpenGL ES" : "OpenGL",
                       priv->gl_version / 10, priv->gl_version % 10,
                       priv->is_legacy ? "legacy" : "core",
                       glGetString (GL_SHADING_LANGUAGE_VERSION),
                       has_npot ? "yes" : "no",
                       has_texture_rectangle ? "yes" : "no",
                       priv->has_gl_framebuffer_blit ? "yes" : "no",
                       priv->has_frame_terminator ? "yes" : "no",
                       priv->use_texture_rectangle ? "yes" : "no"));

  priv->extensions_checked = TRUE;
}

/**
 * cdk_gl_context_make_current:
 * @context: a #CdkGLContext
 *
 * Makes the @context the current one.
 *
 * Since: 3.16
 */
void
cdk_gl_context_make_current (CdkGLContext *context)
{
  CdkGLContextPrivate *priv = cdk_gl_context_get_instance_private (context);
  CdkGLContext *current;

  g_return_if_fail (GDK_IS_GL_CONTEXT (context));

  current = g_private_get (&thread_current_context);
  if (current == context)
    return;

  /* we need to realize the CdkGLContext if it wasn't explicitly realized */
  if (!priv->realized)
    {
      GError *error = NULL;

      cdk_gl_context_realize (context, &error);
      if (error != NULL)
        {
          g_critical ("Could not realize the GL context: %s", error->message);
          g_error_free (error);
          return;
        }
    }

  if (cdk_display_make_gl_context_current (priv->display, context))
    {
      g_private_replace (&thread_current_context, g_object_ref (context));
      cdk_gl_context_check_extensions (context);
    }
}

/**
 * cdk_gl_context_get_display:
 * @context: a #CdkGLContext
 *
 * Retrieves the #CdkDisplay the @context is created for
 *
 * Returns: (nullable) (transfer none): a #CdkDisplay or %NULL
 *
 * Since: 3.16
 */
CdkDisplay *
cdk_gl_context_get_display (CdkGLContext *context)
{
  CdkGLContextPrivate *priv = cdk_gl_context_get_instance_private (context);

  g_return_val_if_fail (GDK_IS_GL_CONTEXT (context), NULL);

  return priv->display;
}

/**
 * cdk_gl_context_get_window:
 * @context: a #CdkGLContext
 *
 * Retrieves the #CdkWindow used by the @context.
 *
 * Returns: (nullable) (transfer none): a #CdkWindow or %NULL
 *
 * Since: 3.16
 */
CdkWindow *
cdk_gl_context_get_window (CdkGLContext *context)
{
  CdkGLContextPrivate *priv = cdk_gl_context_get_instance_private (context);

  g_return_val_if_fail (GDK_IS_GL_CONTEXT (context), NULL);

  return priv->window;
}

/**
 * cdk_gl_context_get_shared_context:
 * @context: a #CdkGLContext
 *
 * Retrieves the #CdkGLContext that this @context share data with.
 *
 * Returns: (nullable) (transfer none): a #CdkGLContext or %NULL
 *
 * Since: 3.16
 */
CdkGLContext *
cdk_gl_context_get_shared_context (CdkGLContext *context)
{
  CdkGLContextPrivate *priv = cdk_gl_context_get_instance_private (context);

  g_return_val_if_fail (GDK_IS_GL_CONTEXT (context), NULL);

  return priv->shared_context;
}

/**
 * cdk_gl_context_get_version:
 * @context: a #CdkGLContext
 * @major: (out): return location for the major version
 * @minor: (out): return location for the minor version
 *
 * Retrieves the OpenGL version of the @context.
 *
 * The @context must be realized prior to calling this function.
 *
 * Since: 3.16
 */
void
cdk_gl_context_get_version (CdkGLContext *context,
                            int          *major,
                            int          *minor)
{
  CdkGLContextPrivate *priv = cdk_gl_context_get_instance_private (context);

  g_return_if_fail (GDK_IS_GL_CONTEXT (context));
  g_return_if_fail (priv->realized);

  if (major != NULL)
    *major = priv->gl_version / 10;
  if (minor != NULL)
    *minor = priv->gl_version % 10;
}

/**
 * cdk_gl_context_clear_current:
 *
 * Clears the current #CdkGLContext.
 *
 * Any OpenGL call after this function returns will be ignored
 * until cdk_gl_context_make_current() is called.
 *
 * Since: 3.16
 */
void
cdk_gl_context_clear_current (void)
{
  CdkGLContext *current;

  current = g_private_get (&thread_current_context);
  if (current != NULL)
    {
      CdkGLContextPrivate *priv = cdk_gl_context_get_instance_private (current);

      if (cdk_display_make_gl_context_current (priv->display, NULL))
        g_private_replace (&thread_current_context, NULL);
    }
}

/**
 * cdk_gl_context_get_current:
 *
 * Retrieves the current #CdkGLContext.
 *
 * Returns: (nullable) (transfer none): the current #CdkGLContext, or %NULL
 *
 * Since: 3.16
 */
CdkGLContext *
cdk_gl_context_get_current (void)
{
  CdkGLContext *current;

  current = g_private_get (&thread_current_context);

  return current;
}

/**
 * cdk_gl_get_flags:
 *
 * Returns the currently active GL flags.
 *
 * Returns: the GL flags
 *
 * Since: 3.16
 */
CdkGLFlags
cdk_gl_get_flags (void)
{
  return _cdk_gl_flags;
}

/**
 * cdk_gl_set_flags:
 * @flags: #CdkGLFlags to set
 *
 * Sets GL flags.
 *
 * Since: 3.16
 */
void
cdk_gl_set_flags (CdkGLFlags flags)
{
  _cdk_gl_flags = flags;
}
