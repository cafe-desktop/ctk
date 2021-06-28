/* CTK - The GIMP Toolkit
 *
 * ctkglarea.c: A GL drawing area
 *
 * Copyright © 2014  Emmanuele Bassi
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

#include "config.h"
#include "ctkglarea.h"
#include "ctkintl.h"
#include "ctkmarshalers.h"
#include "ctkstylecontext.h"
#include "ctkmarshalers.h"
#include "ctkprivate.h"
#include "ctkrender.h"

#include <epoxy/gl.h>

/**
 * SECTION:ctkglarea
 * @Title: CtkGLArea
 * @Short_description: A widget for custom drawing with OpenGL
 *
 * #CtkGLArea is a widget that allows drawing with OpenGL.
 *
 * #CtkGLArea sets up its own #CdkGLContext for the window it creates, and
 * creates a custom GL framebuffer that the widget will do GL rendering onto.
 * It also ensures that this framebuffer is the default GL rendering target
 * when rendering.
 *
 * In order to draw, you have to connect to the #CtkGLArea::render signal,
 * or subclass #CtkGLArea and override the @CtkGLAreaClass.render() virtual
 * function.
 *
 * The #CtkGLArea widget ensures that the #CdkGLContext is associated with
 * the widget's drawing area, and it is kept updated when the size and
 * position of the drawing area changes.
 *
 * ## Drawing with CtkGLArea ##
 *
 * The simplest way to draw using OpenGL commands in a #CtkGLArea is to
 * create a widget instance and connect to the #CtkGLArea::render signal:
 *
 * |[<!-- language="C" -->
 *   // create a CtkGLArea instance
 *   CtkWidget *gl_area = ctk_gl_area_new ();
 *
 *   // connect to the "render" signal
 *   g_signal_connect (gl_area, "render", G_CALLBACK (render), NULL);
 * ]|
 *
 * The `render()` function will be called when the #CtkGLArea is ready
 * for you to draw its content:
 *
 * |[<!-- language="C" -->
 *   static gboolean
 *   render (CtkGLArea *area, CdkGLContext *context)
 *   {
 *     // inside this function it's safe to use GL; the given
 *     // #CdkGLContext has been made current to the drawable
 *     // surface used by the #CtkGLArea and the viewport has
 *     // already been set to be the size of the allocation
 *
 *     // we can start by clearing the buffer
 *     glClearColor (0, 0, 0, 0);
 *     glClear (GL_COLOR_BUFFER_BIT);
 *
 *     // draw your object
 *     draw_an_object ();
 *
 *     // we completed our drawing; the draw commands will be
 *     // flushed at the end of the signal emission chain, and
 *     // the buffers will be drawn on the window
 *     return TRUE;
 *   }
 * ]|
 *
 * If you need to initialize OpenGL state, e.g. buffer objects or
 * shaders, you should use the #CtkWidget::realize signal; you
 * can use the #CtkWidget::unrealize signal to clean up. Since the
 * #CdkGLContext creation and initialization may fail, you will
 * need to check for errors, using ctk_gl_area_get_error(). An example
 * of how to safely initialize the GL state is:
 *
 * |[<!-- language="C" -->
 *   static void
 *   on_realize (CtkGLarea *area)
 *   {
 *     // We need to make the context current if we want to
 *     // call GL API
 *     ctk_gl_area_make_current (area);
 *
 *     // If there were errors during the initialization or
 *     // when trying to make the context current, this
 *     // function will return a #GError for you to catch
 *     if (ctk_gl_area_get_error (area) != NULL)
 *       return;
 *
 *     // You can also use ctk_gl_area_set_error() in order
 *     // to show eventual initialization errors on the
 *     // CtkGLArea widget itself
 *     GError *internal_error = NULL;
 *     init_buffer_objects (&error);
 *     if (error != NULL)
 *       {
 *         ctk_gl_area_set_error (area, error);
 *         g_error_free (error);
 *         return;
 *       }
 *
 *     init_shaders (&error);
 *     if (error != NULL)
 *       {
 *         ctk_gl_area_set_error (area, error);
 *         g_error_free (error);
 *         return;
 *       }
 *   }
 * ]|
 *
 * If you need to change the options for creating the #CdkGLContext
 * you should use the #CtkGLArea::create-context signal.
 */

typedef struct {
  CdkGLContext *context;
  CdkWindow *event_window;
  GError *error;

  gboolean have_buffers;

  int required_gl_version;

  guint frame_buffer;
  guint render_buffer;
  guint texture;
  guint depth_stencil_buffer;

  gboolean has_alpha;
  gboolean has_depth_buffer;
  gboolean has_stencil_buffer;

  gboolean needs_resize;
  gboolean needs_render;
  gboolean auto_render;
  gboolean use_es;
} CtkGLAreaPrivate;

enum {
  PROP_0,

  PROP_CONTEXT,
  PROP_HAS_ALPHA,
  PROP_HAS_DEPTH_BUFFER,
  PROP_HAS_STENCIL_BUFFER,
  PROP_USE_ES,

  PROP_AUTO_RENDER,

  LAST_PROP
};

static GParamSpec *obj_props[LAST_PROP] = { NULL, };

enum {
  RENDER,
  RESIZE,
  CREATE_CONTEXT,

  LAST_SIGNAL
};

static void ctk_gl_area_allocate_buffers (CtkGLArea *area);

static guint area_signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE_WITH_PRIVATE (CtkGLArea, ctk_gl_area, CTK_TYPE_WIDGET)

static void
ctk_gl_area_dispose (GObject *gobject)
{
  CtkGLArea *area = CTK_GL_AREA (gobject);
  CtkGLAreaPrivate *priv = ctk_gl_area_get_instance_private (area);

  g_clear_object (&priv->context);

  G_OBJECT_CLASS (ctk_gl_area_parent_class)->dispose (gobject);
}

static void
ctk_gl_area_set_property (GObject      *gobject,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  CtkGLArea *self = CTK_GL_AREA (gobject);

  switch (prop_id)
    {
    case PROP_AUTO_RENDER:
      ctk_gl_area_set_auto_render (self, g_value_get_boolean (value));
      break;

    case PROP_HAS_ALPHA:
      ctk_gl_area_set_has_alpha (self, g_value_get_boolean (value));
      break;

    case PROP_HAS_DEPTH_BUFFER:
      ctk_gl_area_set_has_depth_buffer (self, g_value_get_boolean (value));
      break;

    case PROP_HAS_STENCIL_BUFFER:
      ctk_gl_area_set_has_stencil_buffer (self, g_value_get_boolean (value));
      break;

    case PROP_USE_ES:
      ctk_gl_area_set_use_es (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
ctk_gl_area_get_property (GObject    *gobject,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  CtkGLAreaPrivate *priv = ctk_gl_area_get_instance_private (CTK_GL_AREA (gobject));

  switch (prop_id)
    {
    case PROP_AUTO_RENDER:
      g_value_set_boolean (value, priv->auto_render);
      break;

    case PROP_HAS_ALPHA:
      g_value_set_boolean (value, priv->has_alpha);
      break;

    case PROP_HAS_DEPTH_BUFFER:
      g_value_set_boolean (value, priv->has_depth_buffer);
      break;

    case PROP_HAS_STENCIL_BUFFER:
      g_value_set_boolean (value, priv->has_stencil_buffer);
      break;

    case PROP_CONTEXT:
      g_value_set_object (value, priv->context);
      break;

    case PROP_USE_ES:
      g_value_set_boolean (value, priv->use_es);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
ctk_gl_area_realize (CtkWidget *widget)
{
  CtkGLArea *area = CTK_GL_AREA (widget);
  CtkGLAreaPrivate *priv = ctk_gl_area_get_instance_private (area);
  CtkAllocation allocation;
  CdkWindowAttr attributes;
  gint attributes_mask;

  CTK_WIDGET_CLASS (ctk_gl_area_parent_class)->realize (widget);

  ctk_widget_get_allocation (widget, &allocation);

  attributes.window_type = CDK_WINDOW_CHILD;
  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.wclass = CDK_INPUT_ONLY;
  attributes.event_mask = ctk_widget_get_events (widget);

  attributes_mask = CDK_WA_X | CDK_WA_Y;

  priv->event_window = cdk_window_new (ctk_widget_get_parent_window (widget),
                                       &attributes, attributes_mask);
  ctk_widget_register_window (widget, priv->event_window);

  g_clear_error (&priv->error);
  priv->context = NULL;
  g_signal_emit (area, area_signals[CREATE_CONTEXT], 0, &priv->context);

  /* In case the signal failed, but did not set an error */
  if (priv->context == NULL && priv->error == NULL)
    g_set_error_literal (&priv->error, CDK_GL_ERROR,
                         CDK_GL_ERROR_NOT_AVAILABLE,
                         _("OpenGL context creation failed"));

  priv->needs_resize = TRUE;
}

static void
ctk_gl_area_notify (GObject    *object,
                    GParamSpec *pspec)
{
  if (strcmp (pspec->name, "scale-factor") == 0)
    {
      CtkGLArea *area = CTK_GL_AREA (object);
      CtkGLAreaPrivate *priv = ctk_gl_area_get_instance_private (area);

      priv->needs_resize = TRUE;
    }

  if (G_OBJECT_CLASS (ctk_gl_area_parent_class)->notify)
    G_OBJECT_CLASS (ctk_gl_area_parent_class)->notify (object, pspec);
}

static CdkGLContext *
ctk_gl_area_real_create_context (CtkGLArea *area)
{
  CtkGLAreaPrivate *priv = ctk_gl_area_get_instance_private (area);
  CtkWidget *widget = CTK_WIDGET (area);
  GError *error = NULL;
  CdkGLContext *context;

  context = cdk_window_create_gl_context (ctk_widget_get_window (widget), &error);
  if (error != NULL)
    {
      ctk_gl_area_set_error (area, error);
      g_clear_object (&context);
      g_clear_error (&error);
      return NULL;
    }

  cdk_gl_context_set_use_es (context, priv->use_es);
  cdk_gl_context_set_required_version (context,
                                       priv->required_gl_version / 10,
                                       priv->required_gl_version % 10);

  cdk_gl_context_realize (context, &error);
  if (error != NULL)
    {
      ctk_gl_area_set_error (area, error);
      g_clear_object (&context);
      g_clear_error (&error);
      return NULL;
    }

  return context;
}

static void
ctk_gl_area_resize (CtkGLArea *area, int width, int height)
{
  glViewport (0, 0, width, height);
}

/*
 * Creates all the buffer objects needed for rendering the scene
 */
static void
ctk_gl_area_ensure_buffers (CtkGLArea *area)
{
  CtkGLAreaPrivate *priv = ctk_gl_area_get_instance_private (area);
  CtkWidget *widget = CTK_WIDGET (area);

  ctk_widget_realize (widget);

  if (priv->context == NULL)
    return;

  if (priv->have_buffers)
    return;

  priv->have_buffers = TRUE;

  glGenFramebuffersEXT (1, &priv->frame_buffer);

  if (priv->has_alpha)
    {
      /* For alpha we use textures as that is required for blending to work */
      if (priv->texture == 0)
        glGenTextures (1, &priv->texture);

      /* Delete old render buffer if any */
      if (priv->render_buffer != 0)
        {
          glDeleteRenderbuffersEXT(1, &priv->render_buffer);
          priv->render_buffer = 0;
        }
    }
  else
    {
    /* For non-alpha we use render buffers so we can blit instead of texture the result */
      if (priv->render_buffer == 0)
        glGenRenderbuffersEXT (1, &priv->render_buffer);

      /* Delete old texture if any */
      if (priv->texture != 0)
        {
          glDeleteTextures(1, &priv->texture);
          priv->texture = 0;
        }
    }

  if ((priv->has_depth_buffer || priv->has_stencil_buffer))
    {
      if (priv->depth_stencil_buffer == 0)
        glGenRenderbuffersEXT (1, &priv->depth_stencil_buffer);
    }
  else if (priv->depth_stencil_buffer != 0)
    {
      /* Delete old depth/stencil buffer */
      glDeleteRenderbuffersEXT (1, &priv->depth_stencil_buffer);
      priv->depth_stencil_buffer = 0;
    }

  ctk_gl_area_allocate_buffers (area);
}

/*
 * Allocates space of the right type and size for all the buffers
 */
static void
ctk_gl_area_allocate_buffers (CtkGLArea *area)
{
  CtkGLAreaPrivate *priv = ctk_gl_area_get_instance_private (area);
  CtkWidget *widget = CTK_WIDGET (area);
  int scale, width, height;

  if (priv->context == NULL)
    return;

  scale = ctk_widget_get_scale_factor (widget);
  width = ctk_widget_get_allocated_width (widget) * scale;
  height = ctk_widget_get_allocated_height (widget) * scale;

  if (priv->texture)
    {
      glBindTexture (GL_TEXTURE_2D, priv->texture);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

      if (cdk_gl_context_get_use_es (priv->context))
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
      else
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
    }

  if (priv->render_buffer)
    {
      glBindRenderbuffer (GL_RENDERBUFFER, priv->render_buffer);
      glRenderbufferStorage (GL_RENDERBUFFER, GL_RGB8, width, height);
    }

  if (priv->has_depth_buffer || priv->has_stencil_buffer)
    {
      glBindRenderbuffer (GL_RENDERBUFFER, priv->depth_stencil_buffer);
      if (priv->has_stencil_buffer)
        glRenderbufferStorage (GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
      else
        glRenderbufferStorage (GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    }

  priv->needs_render = TRUE;
}

/**
 * ctk_gl_area_attach_buffers:
 * @area: a #CtkGLArea
 *
 * Ensures that the @area framebuffer object is made the current draw
 * and read target, and that all the required buffers for the @area
 * are created and bound to the frambuffer.
 *
 * This function is automatically called before emitting the
 * #CtkGLArea::render signal, and doesn't normally need to be called
 * by application code.
 *
 * Since: 3.16
 */
void
ctk_gl_area_attach_buffers (CtkGLArea *area)
{
  CtkGLAreaPrivate *priv = ctk_gl_area_get_instance_private (area);

  g_return_if_fail (CTK_IS_GL_AREA (area));

  if (priv->context == NULL)
    return;

  ctk_gl_area_make_current (area);

  if (!priv->have_buffers)
    ctk_gl_area_ensure_buffers (area);
  else if (priv->needs_resize)
    ctk_gl_area_allocate_buffers (area);

  glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, priv->frame_buffer);

  if (priv->texture)
    glFramebufferTexture2D (GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                            GL_TEXTURE_2D, priv->texture, 0);
  else if (priv->render_buffer)
    glFramebufferRenderbufferEXT (GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                                  GL_RENDERBUFFER_EXT, priv->render_buffer);

  if (priv->depth_stencil_buffer)
    {
      if (priv->has_depth_buffer)
        glFramebufferRenderbufferEXT (GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                                      GL_RENDERBUFFER_EXT, priv->depth_stencil_buffer);
      if (priv->has_stencil_buffer)
        glFramebufferRenderbufferEXT (GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT,
                                      GL_RENDERBUFFER_EXT, priv->depth_stencil_buffer);
    }
}

static void
ctk_gl_area_delete_buffers (CtkGLArea *area)
{
  CtkGLAreaPrivate *priv = ctk_gl_area_get_instance_private (area);

  if (priv->context == NULL)
    return;

  priv->have_buffers = FALSE;

  if (priv->render_buffer != 0)
    {
      glDeleteRenderbuffersEXT (1, &priv->render_buffer);
      priv->render_buffer = 0;
    }

  if (priv->texture != 0)
    {
      glDeleteTextures(1, &priv->texture);
      priv->texture = 0;
    }

  if (priv->depth_stencil_buffer != 0)
    {
      glDeleteRenderbuffersEXT (1, &priv->depth_stencil_buffer);
      priv->depth_stencil_buffer = 0;
    }

  if (priv->frame_buffer != 0)
    {
      glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, 0);
      glDeleteFramebuffersEXT (1, &priv->frame_buffer);
      priv->frame_buffer = 0;
    }
}

static void
ctk_gl_area_unrealize (CtkWidget *widget)
{
  CtkGLArea *area = CTK_GL_AREA (widget);
  CtkGLAreaPrivate *priv = ctk_gl_area_get_instance_private (area);

  if (priv->context != NULL)
    {
      if (priv->have_buffers)
        {
          ctk_gl_area_make_current (area);
          ctk_gl_area_delete_buffers (area);
        }

      /* Make sure to unset the context if current */
      if (priv->context == cdk_gl_context_get_current ())
        cdk_gl_context_clear_current ();
    }

  g_clear_object (&priv->context);
  g_clear_error (&priv->error);

  if (priv->event_window != NULL)
    {
      ctk_widget_unregister_window (widget, priv->event_window);
      cdk_window_destroy (priv->event_window);
      priv->event_window = NULL;
    }

  CTK_WIDGET_CLASS (ctk_gl_area_parent_class)->unrealize (widget);
}

static void
ctk_gl_area_map (CtkWidget *widget)
{
  CtkGLArea *area = CTK_GL_AREA (widget);
  CtkGLAreaPrivate *priv = ctk_gl_area_get_instance_private (area);

  if (priv->event_window != NULL)
    cdk_window_show (priv->event_window);

  CTK_WIDGET_CLASS (ctk_gl_area_parent_class)->map (widget);
}

static void
ctk_gl_area_unmap (CtkWidget *widget)
{
  CtkGLArea *area = CTK_GL_AREA (widget);
  CtkGLAreaPrivate *priv = ctk_gl_area_get_instance_private (area);

  if (priv->event_window != NULL)
    cdk_window_hide (priv->event_window);

  CTK_WIDGET_CLASS (ctk_gl_area_parent_class)->unmap (widget);
}

static void
ctk_gl_area_size_allocate (CtkWidget     *widget,
                           CtkAllocation *allocation)
{
  CtkGLArea *area = CTK_GL_AREA (widget);
  CtkGLAreaPrivate *priv = ctk_gl_area_get_instance_private (area);

  CTK_WIDGET_CLASS (ctk_gl_area_parent_class)->size_allocate (widget, allocation);

  if (ctk_widget_get_realized (widget))
    {
      if (priv->event_window != NULL)
        cdk_window_move_resize (priv->event_window,
                                allocation->x,
                                allocation->y,
                                allocation->width,
                                allocation->height);

      priv->needs_resize = TRUE;
    }
}

static void
ctk_gl_area_draw_error_screen (CtkGLArea *area,
                               cairo_t   *cr,
                               gint       width,
                               gint       height)
{
  CtkGLAreaPrivate *priv = ctk_gl_area_get_instance_private (area);
  PangoLayout *layout;
  int layout_height;

  layout = ctk_widget_create_pango_layout (CTK_WIDGET (area),
                                           priv->error->message);
  pango_layout_set_width (layout, width * PANGO_SCALE);
  pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);
  pango_layout_get_pixel_size (layout, NULL, &layout_height);
  ctk_render_layout (ctk_widget_get_style_context (CTK_WIDGET (area)),
                     cr,
                     0, (height - layout_height) / 2,
                     layout);

  g_object_unref (layout);
}

static gboolean
ctk_gl_area_draw (CtkWidget *widget,
                  cairo_t   *cr)
{
  CtkGLArea *area = CTK_GL_AREA (widget);
  CtkGLAreaPrivate *priv = ctk_gl_area_get_instance_private (area);
  gboolean unused;
  int w, h, scale;
  GLenum status;

  if (priv->error != NULL)
    {
      ctk_gl_area_draw_error_screen (area,
                                     cr,
                                     ctk_widget_get_allocated_width (widget),
                                     ctk_widget_get_allocated_height (widget));
      return FALSE;
    }

  if (priv->context == NULL)
    return FALSE;

  ctk_gl_area_make_current (area);

  ctk_gl_area_attach_buffers (area);

 if (priv->has_depth_buffer)
   glEnable (GL_DEPTH_TEST);
 else
   glDisable (GL_DEPTH_TEST);

  scale = ctk_widget_get_scale_factor (widget);
  w = ctk_widget_get_allocated_width (widget) * scale;
  h = ctk_widget_get_allocated_height (widget) * scale;

  status = glCheckFramebufferStatusEXT (GL_FRAMEBUFFER_EXT);
  if (status == GL_FRAMEBUFFER_COMPLETE_EXT)
    {
      if (priv->needs_render || priv->auto_render)
        {
          if (priv->needs_resize)
            {
              g_signal_emit (area, area_signals[RESIZE], 0, w, h, NULL);
              priv->needs_resize = FALSE;
            }

          g_signal_emit (area, area_signals[RENDER], 0, priv->context, &unused);
        }

      priv->needs_render = FALSE;

      cdk_cairo_draw_from_gl (cr,
                              ctk_widget_get_window (widget),
                              priv->texture ? priv->texture : priv->render_buffer,
                              priv->texture ? GL_TEXTURE : GL_RENDERBUFFER,
                              scale, 0, 0, w, h);
      ctk_gl_area_make_current (area);
    }
  else
    {
      g_warning ("fb setup not supported");
    }

  return TRUE;
}

static gboolean
create_context_accumulator (GSignalInvocationHint *ihint,
                            GValue *return_accu,
                            const GValue *handler_return,
                            gpointer data)
{
  g_value_copy (handler_return, return_accu);

  /* stop after the first handler returning a valid object */
  return g_value_get_object (handler_return) == NULL;
}

static void
ctk_gl_area_class_init (CtkGLAreaClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);

  klass->resize = ctk_gl_area_resize;
  klass->create_context = ctk_gl_area_real_create_context;

  widget_class->realize = ctk_gl_area_realize;
  widget_class->unrealize = ctk_gl_area_unrealize;
  widget_class->map = ctk_gl_area_map;
  widget_class->unmap = ctk_gl_area_unmap;
  widget_class->size_allocate = ctk_gl_area_size_allocate;
  widget_class->draw = ctk_gl_area_draw;

  ctk_widget_class_set_accessible_role (widget_class, ATK_ROLE_DRAWING_AREA);

  /**
   * CtkGLArea:context:
   *
   * The #CdkGLContext used by the #CtkGLArea widget.
   *
   * The #CtkGLArea widget is responsible for creating the #CdkGLContext
   * instance. If you need to render with other kinds of buffers (stencil,
   * depth, etc), use render buffers.
   *
   * Since: 3.16
   */
  obj_props[PROP_CONTEXT] =
    g_param_spec_object ("context",
                         P_("Context"),
                         P_("The GL context"),
                         CDK_TYPE_GL_CONTEXT,
                         G_PARAM_READABLE |
                         G_PARAM_STATIC_STRINGS);

  /**
   * CtkGLArea:auto-render:
   *
   * If set to %TRUE the #CtkGLArea::render signal will be emitted every time
   * the widget draws. This is the default and is useful if drawing the widget
   * is faster.
   *
   * If set to %FALSE the data from previous rendering is kept around and will
   * be used for drawing the widget the next time, unless the window is resized.
   * In order to force a rendering ctk_gl_area_queue_render() must be called.
   * This mode is useful when the scene changes seldomly, but takes a long time
   * to redraw.
   *
   * Since: 3.16
   */
  obj_props[PROP_AUTO_RENDER] =
    g_param_spec_boolean ("auto-render",
                          P_("Auto render"),
                          P_("Whether the CtkGLArea renders on each redraw"),
                          TRUE,
                          CTK_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS |
                          G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkGLArea:has-alpha:
   *
   * If set to %TRUE the buffer allocated by the widget will have an alpha channel
   * component, and when rendering to the window the result will be composited over
   * whatever is below the widget.
   *
   * If set to %FALSE there will be no alpha channel, and the buffer will fully
   * replace anything below the widget.
   *
   * Since: 3.16
   */
  obj_props[PROP_HAS_ALPHA] =
    g_param_spec_boolean ("has-alpha",
                          P_("Has alpha"),
                          P_("Whether the color buffer has an alpha component"),
                          FALSE,
                          CTK_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS |
                          G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkGLArea:has-depth-buffer:
   *
   * If set to %TRUE the widget will allocate and enable a depth buffer for the
   * target framebuffer.
   *
   * Since: 3.16
   */
  obj_props[PROP_HAS_DEPTH_BUFFER] =
    g_param_spec_boolean ("has-depth-buffer",
                          P_("Has depth buffer"),
                          P_("Whether a depth buffer is allocated"),
                          FALSE,
                          CTK_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS |
                          G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkGLArea:has-stencil-buffer:
   *
   * If set to %TRUE the widget will allocate and enable a stencil buffer for the
   * target framebuffer.
   *
   * Since: 3.16
   */
  obj_props[PROP_HAS_STENCIL_BUFFER] =
    g_param_spec_boolean ("has-stencil-buffer",
                          P_("Has stencil buffer"),
                          P_("Whether a stencil buffer is allocated"),
                          FALSE,
                          CTK_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS |
                          G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkGLArea:use-es:
   *
   * If set to %TRUE the widget will try to create a #CdkGLContext using
   * OpenGL ES instead of OpenGL.
   *
   * See also: cdk_gl_context_set_use_es()
   *
   * Since: 3.22
   */
  obj_props[PROP_USE_ES] =
    g_param_spec_boolean ("use-es",
                          P_("Use OpenGL ES"),
                          P_("Whether the context uses OpenGL or OpenGL ES"),
                          FALSE,
                          CTK_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS |
                          G_PARAM_EXPLICIT_NOTIFY);

  gobject_class->set_property = ctk_gl_area_set_property;
  gobject_class->get_property = ctk_gl_area_get_property;
  gobject_class->dispose = ctk_gl_area_dispose;
  gobject_class->notify = ctk_gl_area_notify;

  g_object_class_install_properties (gobject_class, LAST_PROP, obj_props);

  /**
   * CtkGLArea::render:
   * @area: the #CtkGLArea that emitted the signal
   * @context: the #CdkGLContext used by @area
   *
   * The ::render signal is emitted every time the contents
   * of the #CtkGLArea should be redrawn.
   *
   * The @context is bound to the @area prior to emitting this function,
   * and the buffers are painted to the window once the emission terminates.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event.
   *   %FALSE to propagate the event further.
   *
   * Since: 3.16
   */
  area_signals[RENDER] =
    g_signal_new (I_("render"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkGLAreaClass, render),
                  _ctk_boolean_handled_accumulator, NULL,
                  _ctk_marshal_BOOLEAN__OBJECT,
                  G_TYPE_BOOLEAN, 1,
                  CDK_TYPE_GL_CONTEXT);
  g_signal_set_va_marshaller (area_signals[RENDER],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__OBJECTv);

  /**
   * CtkGLArea::resize:
   * @area: the #CtkGLArea that emitted the signal
   * @width: the width of the viewport
   * @height: the height of the viewport
   *
   * The ::resize signal is emitted once when the widget is realized, and
   * then each time the widget is changed while realized. This is useful
   * in order to keep GL state up to date with the widget size, like for
   * instance camera properties which may depend on the width/height ratio.
   *
   * The GL context for the area is guaranteed to be current when this signal
   * is emitted.
   *
   * The default handler sets up the GL viewport.
   *
   * Since: 3.16
   */
  area_signals[RESIZE] =
    g_signal_new (I_("resize"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkGLAreaClass, resize),
                  NULL, NULL,
                  _ctk_marshal_VOID__INT_INT,
                  G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_INT);
  g_signal_set_va_marshaller (area_signals[RESIZE],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_VOID__INT_INTv);

  /**
   * CtkGLArea::create-context:
   * @area: the #CtkGLArea that emitted the signal
   * @error: (allow-none): location to store error information on failure
   *
   * The ::create-context signal is emitted when the widget is being
   * realized, and allows you to override how the GL context is
   * created. This is useful when you want to reuse an existing GL
   * context, or if you want to try creating different kinds of GL
   * options.
   *
   * If context creation fails then the signal handler can use
   * ctk_gl_area_set_error() to register a more detailed error
   * of how the construction failed.
   *
   * Returns: (transfer full): a newly created #CdkGLContext;
   *     the #CtkGLArea widget will take ownership of the returned value.
   *
   * Since: 3.16
   */
  area_signals[CREATE_CONTEXT] =
    g_signal_new (I_("create-context"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkGLAreaClass, create_context),
                  create_context_accumulator, NULL,
                  _ctk_marshal_OBJECT__VOID,
                  CDK_TYPE_GL_CONTEXT, 0);
  g_signal_set_va_marshaller (area_signals[CREATE_CONTEXT],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_OBJECT__VOIDv);
}

static void
ctk_gl_area_init (CtkGLArea *area)
{
  CtkGLAreaPrivate *priv = ctk_gl_area_get_instance_private (area);

  ctk_widget_set_has_window (CTK_WIDGET (area), FALSE);
  ctk_widget_set_app_paintable (CTK_WIDGET (area), TRUE);

  priv->auto_render = TRUE;
  priv->needs_render = TRUE;
  priv->required_gl_version = 0;
}

/**
 * ctk_gl_area_new:
 *
 * Creates a new #CtkGLArea widget.
 *
 * Returns: a new #CtkGLArea
 *
 * Since: 3.16
 */
CtkWidget *
ctk_gl_area_new (void)
{
  return g_object_new (CTK_TYPE_GL_AREA, NULL);
}

/**
 * ctk_gl_area_set_error:
 * @area: a #CtkGLArea
 * @error: (allow-none): a new #GError, or %NULL to unset the error
 *
 * Sets an error on the area which will be shown instead of the
 * GL rendering. This is useful in the #CtkGLArea::create-context
 * signal if GL context creation fails.
 *
 * Since: 3.16
 */
void
ctk_gl_area_set_error (CtkGLArea    *area,
                       const GError *error)
{
  CtkGLAreaPrivate *priv = ctk_gl_area_get_instance_private (area);

  g_return_if_fail (CTK_IS_GL_AREA (area));

  g_clear_error (&priv->error);
  if (error)
    priv->error = g_error_copy (error);
}

/**
 * ctk_gl_area_get_error:
 * @area: a #CtkGLArea
 *
 * Gets the current error set on the @area.
 *
 * Returns: (nullable) (transfer none): the #GError or %NULL
 *
 * Since: 3.16
 */
GError *
ctk_gl_area_get_error (CtkGLArea *area)
{
  CtkGLAreaPrivate *priv = ctk_gl_area_get_instance_private (area);

  g_return_val_if_fail (CTK_IS_GL_AREA (area), NULL);

  return priv->error;
}

/**
 * ctk_gl_area_set_use_es:
 * @area: a #CtkGLArea
 * @use_es: whether to use OpenGL or OpenGL ES
 *
 * Sets whether the @area should create an OpenGL or an OpenGL ES context.
 *
 * You should check the capabilities of the #CdkGLContext before drawing
 * with either API.
 *
 * Since: 3.22
 */
void
ctk_gl_area_set_use_es (CtkGLArea *area,
                        gboolean   use_es)
{
  CtkGLAreaPrivate *priv = ctk_gl_area_get_instance_private (area);

  g_return_if_fail (CTK_IS_GL_AREA (area));
  g_return_if_fail (!ctk_widget_get_realized (CTK_WIDGET (area)));

  use_es = !!use_es;

  if (priv->use_es != use_es)
    {
      priv->use_es = use_es;

      g_object_notify_by_pspec (G_OBJECT (area), obj_props[PROP_USE_ES]);
    }
}

/**
 * ctk_gl_area_get_use_es:
 * @area: a #CtkGLArea
 *
 * Retrieves the value set by ctk_gl_area_set_use_es().
 *
 * Returns: %TRUE if the #CtkGLArea should create an OpenGL ES context
 *   and %FALSE otherwise
 *
 * Since: 3.22
 */
gboolean
ctk_gl_area_get_use_es (CtkGLArea *area)
{
  CtkGLAreaPrivate *priv = ctk_gl_area_get_instance_private (area);

  g_return_val_if_fail (CTK_IS_GL_AREA (area), FALSE);

  return priv->use_es;
}

/**
 * ctk_gl_area_set_required_version:
 * @area: a #CtkGLArea
 * @major: the major version
 * @minor: the minor version
 *
 * Sets the required version of OpenGL to be used when creating the context
 * for the widget.
 *
 * This function must be called before the area has been realized.
 *
 * Since: 3.16
 */
void
ctk_gl_area_set_required_version (CtkGLArea *area,
                                  gint       major,
                                  gint       minor)
{
  CtkGLAreaPrivate *priv = ctk_gl_area_get_instance_private (area);

  g_return_if_fail (CTK_IS_GL_AREA (area));
  g_return_if_fail (!ctk_widget_get_realized (CTK_WIDGET (area)));

  priv->required_gl_version = major * 10 + minor;
}

/**
 * ctk_gl_area_get_required_version:
 * @area: a #CtkGLArea
 * @major: (out): return location for the required major version
 * @minor: (out): return location for the required minor version
 *
 * Retrieves the required version of OpenGL set
 * using ctk_gl_area_set_required_version().
 *
 * Since: 3.16
 */
void
ctk_gl_area_get_required_version (CtkGLArea *area,
                                  gint      *major,
                                  gint      *minor)
{
  CtkGLAreaPrivate *priv = ctk_gl_area_get_instance_private (area);

  g_return_if_fail (CTK_IS_GL_AREA (area));

  if (major != NULL)
    *major = priv->required_gl_version / 10;
  if (minor != NULL)
    *minor = priv->required_gl_version % 10;
}

/**
 * ctk_gl_area_get_has_alpha:
 * @area: a #CtkGLArea
 *
 * Returns whether the area has an alpha component.
 *
 * Returns: %TRUE if the @area has an alpha component, %FALSE otherwise
 *
 * Since: 3.16
 */
gboolean
ctk_gl_area_get_has_alpha (CtkGLArea *area)
{
  CtkGLAreaPrivate *priv = ctk_gl_area_get_instance_private (area);

  g_return_val_if_fail (CTK_IS_GL_AREA (area), FALSE);

  return priv->has_alpha;
}

/**
 * ctk_gl_area_set_has_alpha:
 * @area: a #CtkGLArea
 * @has_alpha: %TRUE to add an alpha component
 *
 * If @has_alpha is %TRUE the buffer allocated by the widget will have
 * an alpha channel component, and when rendering to the window the
 * result will be composited over whatever is below the widget.
 *
 * If @has_alpha is %FALSE there will be no alpha channel, and the
 * buffer will fully replace anything below the widget.
 *
 * Since: 3.16
 */
void
ctk_gl_area_set_has_alpha (CtkGLArea *area,
                           gboolean   has_alpha)
{
  CtkGLAreaPrivate *priv = ctk_gl_area_get_instance_private (area);

  g_return_if_fail (CTK_IS_GL_AREA (area));

  has_alpha = !!has_alpha;

  if (priv->has_alpha != has_alpha)
    {
      priv->has_alpha = has_alpha;

      g_object_notify (G_OBJECT (area), "has-alpha");

      ctk_gl_area_delete_buffers (area);
    }
}

/**
 * ctk_gl_area_get_has_depth_buffer:
 * @area: a #CtkGLArea
 *
 * Returns whether the area has a depth buffer.
 *
 * Returns: %TRUE if the @area has a depth buffer, %FALSE otherwise
 *
 * Since: 3.16
 */
gboolean
ctk_gl_area_get_has_depth_buffer (CtkGLArea *area)
{
  CtkGLAreaPrivate *priv = ctk_gl_area_get_instance_private (area);

  g_return_val_if_fail (CTK_IS_GL_AREA (area), FALSE);

  return priv->has_depth_buffer;
}

/**
 * ctk_gl_area_set_has_depth_buffer:
 * @area: a #CtkGLArea
 * @has_depth_buffer: %TRUE to add a depth buffer
 *
 * If @has_depth_buffer is %TRUE the widget will allocate and
 * enable a depth buffer for the target framebuffer. Otherwise
 * there will be none.
 *
 * Since: 3.16
 */
void
ctk_gl_area_set_has_depth_buffer (CtkGLArea *area,
                                  gboolean   has_depth_buffer)
{
  CtkGLAreaPrivate *priv = ctk_gl_area_get_instance_private (area);

  g_return_if_fail (CTK_IS_GL_AREA (area));

  has_depth_buffer = !!has_depth_buffer;

  if (priv->has_depth_buffer != has_depth_buffer)
    {
      priv->has_depth_buffer = has_depth_buffer;

      g_object_notify (G_OBJECT (area), "has-depth-buffer");

      priv->have_buffers = FALSE;
    }
}

/**
 * ctk_gl_area_get_has_stencil_buffer:
 * @area: a #CtkGLArea
 *
 * Returns whether the area has a stencil buffer.
 *
 * Returns: %TRUE if the @area has a stencil buffer, %FALSE otherwise
 *
 * Since: 3.16
 */
gboolean
ctk_gl_area_get_has_stencil_buffer (CtkGLArea *area)
{
  CtkGLAreaPrivate *priv = ctk_gl_area_get_instance_private (area);

  g_return_val_if_fail (CTK_IS_GL_AREA (area), FALSE);

  return priv->has_stencil_buffer;
}

/**
 * ctk_gl_area_set_has_stencil_buffer:
 * @area: a #CtkGLArea
 * @has_stencil_buffer: %TRUE to add a stencil buffer
 *
 * If @has_stencil_buffer is %TRUE the widget will allocate and
 * enable a stencil buffer for the target framebuffer. Otherwise
 * there will be none.
 *
 * Since: 3.16
 */
void
ctk_gl_area_set_has_stencil_buffer (CtkGLArea *area,
                                    gboolean   has_stencil_buffer)
{
  CtkGLAreaPrivate *priv = ctk_gl_area_get_instance_private (area);

  g_return_if_fail (CTK_IS_GL_AREA (area));

  has_stencil_buffer = !!has_stencil_buffer;

  if (priv->has_stencil_buffer != has_stencil_buffer)
    {
      priv->has_stencil_buffer = has_stencil_buffer;

      g_object_notify (G_OBJECT (area), "has-stencil-buffer");

      priv->have_buffers = FALSE;
    }
}

/**
 * ctk_gl_area_queue_render:
 * @area: a #CtkGLArea
 *
 * Marks the currently rendered data (if any) as invalid, and queues
 * a redraw of the widget, ensuring that the #CtkGLArea::render signal
 * is emitted during the draw.
 *
 * This is only needed when the ctk_gl_area_set_auto_render() has
 * been called with a %FALSE value. The default behaviour is to
 * emit #CtkGLArea::render on each draw.
 *
 * Since: 3.16
 */
void
ctk_gl_area_queue_render (CtkGLArea *area)
{
  CtkGLAreaPrivate *priv = ctk_gl_area_get_instance_private (area);

  g_return_if_fail (CTK_IS_GL_AREA (area));

  priv->needs_render = TRUE;

  ctk_widget_queue_draw (CTK_WIDGET (area));
}


/**
 * ctk_gl_area_get_auto_render:
 * @area: a #CtkGLArea
 *
 * Returns whether the area is in auto render mode or not.
 *
 * Returns: %TRUE if the @area is auto rendering, %FALSE otherwise
 *
 * Since: 3.16
 */
gboolean
ctk_gl_area_get_auto_render (CtkGLArea *area)
{
  CtkGLAreaPrivate *priv = ctk_gl_area_get_instance_private (area);

  g_return_val_if_fail (CTK_IS_GL_AREA (area), FALSE);

  return priv->auto_render;
}

/**
 * ctk_gl_area_set_auto_render:
 * @area: a #CtkGLArea
 * @auto_render: a boolean
 *
 * If @auto_render is %TRUE the #CtkGLArea::render signal will be
 * emitted every time the widget draws. This is the default and is
 * useful if drawing the widget is faster.
 *
 * If @auto_render is %FALSE the data from previous rendering is kept
 * around and will be used for drawing the widget the next time,
 * unless the window is resized. In order to force a rendering
 * ctk_gl_area_queue_render() must be called. This mode is useful when
 * the scene changes seldomly, but takes a long time to redraw.
 *
 * Since: 3.16
 */
void
ctk_gl_area_set_auto_render (CtkGLArea *area,
                             gboolean   auto_render)
{
  CtkGLAreaPrivate *priv = ctk_gl_area_get_instance_private (area);

  g_return_if_fail (CTK_IS_GL_AREA (area));

  auto_render = !!auto_render;

  if (priv->auto_render != auto_render)
    {
      priv->auto_render = auto_render;

      g_object_notify (G_OBJECT (area), "auto-render");

      if (auto_render)
        ctk_widget_queue_draw (CTK_WIDGET (area));
    }
}

/**
 * ctk_gl_area_get_context:
 * @area: a #CtkGLArea
 *
 * Retrieves the #CdkGLContext used by @area.
 *
 * Returns: (transfer none): the #CdkGLContext
 *
 * Since: 3.16
 */
CdkGLContext *
ctk_gl_area_get_context (CtkGLArea *area)
{
  CtkGLAreaPrivate *priv = ctk_gl_area_get_instance_private (area);

  g_return_val_if_fail (CTK_IS_GL_AREA (area), NULL);

  return priv->context;
}

/**
 * ctk_gl_area_make_current:
 * @area: a #CtkGLArea
 *
 * Ensures that the #CdkGLContext used by @area is associated with
 * the #CtkGLArea.
 *
 * This function is automatically called before emitting the
 * #CtkGLArea::render signal, and doesn't normally need to be called
 * by application code.
 *
 * Since: 3.16
 */
void
ctk_gl_area_make_current (CtkGLArea *area)
{
  CtkGLAreaPrivate *priv = ctk_gl_area_get_instance_private (area);
  CtkWidget *widget;

  g_return_if_fail (CTK_IS_GL_AREA (area));

  widget = CTK_WIDGET (area);

  g_return_if_fail (ctk_widget_get_realized (widget));

  if (priv->context != NULL)
    cdk_gl_context_make_current (priv->context);
}
