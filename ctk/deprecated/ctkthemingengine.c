/* CTK - The GIMP Toolkit
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

#include "config.h"

#include <math.h>
#include <ctk/ctk.h>

#include <ctk/ctkstylecontext.h>
#include <ctk/ctkintl.h>

#include "ctkprivate.h"
#include "ctkmodulesprivate.h"
#include "ctkpango.h"
#include "ctkrenderprivate.h"
#include "ctkstylecontextprivate.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * SECTION:ctkthemingengine
 * @Short_description: Theming renderers
 * @Title: CtkThemingEngine
 * @See_also: #CtkStyleContext
 *
 * #CtkThemingEngine was the object used for rendering themed content
 * in CTK+ widgets. It used to allow overriding CTK+'s default
 * implementation of rendering functions by allowing engines to be
 * loaded as modules.
 *
 * #CtkThemingEngine has been deprecated in CTK+ 3.14 and will be
 * ignored for rendering. The advancements in CSS theming are good
 * enough to allow themers to achieve their goals without the need
 * to modify source code.
 */

enum {
  PROP_0,
  PROP_NAME
};

struct CtkThemingEnginePrivate
{
  CtkStyleContext *context;
  gchar *name;
};

static void ctk_theming_engine_finalize          (GObject      *object);
static void ctk_theming_engine_impl_set_property (GObject      *object,
                                                  guint         prop_id,
                                                  const GValue *value,
                                                  GParamSpec   *pspec);
static void ctk_theming_engine_impl_get_property (GObject      *object,
                                                  guint         prop_id,
                                                  GValue       *value,
                                                  GParamSpec   *pspec);

static void ctk_theming_engine_render_check (CtkThemingEngine *engine,
                                             cairo_t          *cr,
                                             gdouble           x,
                                             gdouble           y,
                                             gdouble           width,
                                             gdouble           height);
static void ctk_theming_engine_render_option (CtkThemingEngine *engine,
                                              cairo_t          *cr,
                                              gdouble           x,
                                              gdouble           y,
                                              gdouble           width,
                                              gdouble           height);
static void ctk_theming_engine_render_arrow  (CtkThemingEngine *engine,
                                              cairo_t          *cr,
                                              gdouble           angle,
                                              gdouble           x,
                                              gdouble           y,
                                              gdouble           size);
static void ctk_theming_engine_render_background (CtkThemingEngine *engine,
                                                  cairo_t          *cr,
                                                  gdouble           x,
                                                  gdouble           y,
                                                  gdouble           width,
                                                  gdouble           height);
static void ctk_theming_engine_render_frame  (CtkThemingEngine *engine,
                                              cairo_t          *cr,
                                              gdouble           x,
                                              gdouble           y,
                                              gdouble           width,
                                              gdouble           height);
static void ctk_theming_engine_render_expander (CtkThemingEngine *engine,
                                                cairo_t          *cr,
                                                gdouble           x,
                                                gdouble           y,
                                                gdouble           width,
                                                gdouble           height);
static void ctk_theming_engine_render_focus    (CtkThemingEngine *engine,
                                                cairo_t          *cr,
                                                gdouble           x,
                                                gdouble           y,
                                                gdouble           width,
                                                gdouble           height);
static void ctk_theming_engine_render_layout   (CtkThemingEngine *engine,
                                                cairo_t          *cr,
                                                gdouble           x,
                                                gdouble           y,
                                                PangoLayout      *layout);
static void ctk_theming_engine_render_line     (CtkThemingEngine *engine,
                                                cairo_t          *cr,
                                                gdouble           x0,
                                                gdouble           y0,
                                                gdouble           x1,
                                                gdouble           y1);
static void ctk_theming_engine_render_slider   (CtkThemingEngine *engine,
                                                cairo_t          *cr,
                                                gdouble           x,
                                                gdouble           y,
                                                gdouble           width,
                                                gdouble           height,
                                                CtkOrientation    orientation);
static void ctk_theming_engine_render_frame_gap (CtkThemingEngine *engine,
                                                 cairo_t          *cr,
                                                 gdouble           x,
                                                 gdouble           y,
                                                 gdouble           width,
                                                 gdouble           height,
                                                 CtkPositionType   gap_side,
                                                 gdouble           xy0_gap,
                                                 gdouble           xy1_gap);
static void ctk_theming_engine_render_extension (CtkThemingEngine *engine,
                                                 cairo_t          *cr,
                                                 gdouble           x,
                                                 gdouble           y,
                                                 gdouble           width,
                                                 gdouble           height,
                                                 CtkPositionType   gap_side);
static void ctk_theming_engine_render_handle    (CtkThemingEngine *engine,
                                                 cairo_t          *cr,
                                                 gdouble           x,
                                                 gdouble           y,
                                                 gdouble           width,
                                                 gdouble           height);
static void ctk_theming_engine_render_activity  (CtkThemingEngine *engine,
                                                 cairo_t          *cr,
                                                 gdouble           x,
                                                 gdouble           y,
                                                 gdouble           width,
                                                 gdouble           height);
static CdkPixbuf * ctk_theming_engine_render_icon_pixbuf (CtkThemingEngine    *engine,
                                                          const CtkIconSource *source,
                                                          CtkIconSize          size);
static void ctk_theming_engine_render_icon (CtkThemingEngine *engine,
                                            cairo_t *cr,
					    CdkPixbuf *pixbuf,
                                            gdouble x,
                                            gdouble y);
static void ctk_theming_engine_render_icon_surface (CtkThemingEngine *engine,
						    cairo_t *cr,
						    cairo_surface_t *surface,
						    gdouble x,
						    gdouble y);

G_DEFINE_TYPE_WITH_PRIVATE (CtkThemingEngine, ctk_theming_engine, G_TYPE_OBJECT)

typedef struct CtkThemingModule CtkThemingModule;
typedef struct CtkThemingModuleClass CtkThemingModuleClass;

struct CtkThemingModule
{
  GTypeModule parent_instance;
  GModule *module;
  gchar *name;

  void (*init) (GTypeModule *module);
  void (*exit) (void);
  CtkThemingEngine * (*create_engine) (void);
};

struct CtkThemingModuleClass
{
  GTypeModuleClass parent_class;
};

#define CTK_TYPE_THEMING_MODULE  (ctk_theming_module_get_type ())
#define CTK_THEMING_MODULE(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_THEMING_MODULE, CtkThemingModule))
#define CTK_IS_THEMING_MODULE(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_THEMING_MODULE))

_GDK_EXTERN
GType ctk_theming_module_get_type (void);

G_DEFINE_TYPE (CtkThemingModule, ctk_theming_module, G_TYPE_TYPE_MODULE);

static void
ctk_theming_engine_class_init (CtkThemingEngineClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = ctk_theming_engine_finalize;
  object_class->set_property = ctk_theming_engine_impl_set_property;
  object_class->get_property = ctk_theming_engine_impl_get_property;

  klass->render_icon = ctk_theming_engine_render_icon;
  klass->render_check = ctk_theming_engine_render_check;
  klass->render_option = ctk_theming_engine_render_option;
  klass->render_arrow = ctk_theming_engine_render_arrow;
  klass->render_background = ctk_theming_engine_render_background;
  klass->render_frame = ctk_theming_engine_render_frame;
  klass->render_expander = ctk_theming_engine_render_expander;
  klass->render_focus = ctk_theming_engine_render_focus;
  klass->render_layout = ctk_theming_engine_render_layout;
  klass->render_line = ctk_theming_engine_render_line;
  klass->render_slider = ctk_theming_engine_render_slider;
  klass->render_frame_gap = ctk_theming_engine_render_frame_gap;
  klass->render_extension = ctk_theming_engine_render_extension;
  klass->render_handle = ctk_theming_engine_render_handle;
  klass->render_activity = ctk_theming_engine_render_activity;
  klass->render_icon_pixbuf = ctk_theming_engine_render_icon_pixbuf;
  klass->render_icon_surface = ctk_theming_engine_render_icon_surface;

  /**
   * CtkThemingEngine:name:
   *
   * The theming engine name, this name will be used when registering
   * custom properties, for a theming engine named "Clearlooks" registering
   * a "glossy" custom property, it could be referenced in the CSS file as
   *
   * |[
   * -Clearlooks-glossy: true;
   * ]|
   *
   * Since: 3.0
   */
  g_object_class_install_property (object_class,
				   PROP_NAME,
				   g_param_spec_string ("name",
                                                        P_("Name"),
                                                        P_("Theming engine name"),
                                                        NULL,
                                                        G_PARAM_CONSTRUCT_ONLY | CTK_PARAM_READWRITE));
}

static void
ctk_theming_engine_init (CtkThemingEngine *engine)
{
  engine->priv = ctk_theming_engine_get_instance_private (engine);
}

static void
ctk_theming_engine_finalize (GObject *object)
{
  CtkThemingEnginePrivate *priv;

  priv = CTK_THEMING_ENGINE (object)->priv;
  g_free (priv->name);

  G_OBJECT_CLASS (ctk_theming_engine_parent_class)->finalize (object);
}

static void
ctk_theming_engine_impl_set_property (GObject      *object,
                                      guint         prop_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  CtkThemingEnginePrivate *priv;

  priv = CTK_THEMING_ENGINE (object)->priv;

  switch (prop_id)
    {
    case PROP_NAME:
      g_free (priv->name);

      priv->name = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_theming_engine_impl_get_property (GObject    *object,
                                      guint       prop_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  CtkThemingEnginePrivate *priv;

  priv = CTK_THEMING_ENGINE (object)->priv;

  switch (prop_id)
    {
    case PROP_NAME:
      g_value_set_string (value, priv->name);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * ctk_theming_engine_get_property:
 * @engine: a #CtkThemingEngine
 * @property: the property name
 * @state: state to retrieve the value for
 * @value: (out) (transfer full): return location for the property value,
 *         you must free this memory using g_value_unset() once you are
 *         done with it.
 *
 * Gets a property value as retrieved from the style settings that apply
 * to the currently rendered element.
 *
 * Since: 3.0
 *
 * Deprecated: 3.14
 **/
void
ctk_theming_engine_get_property (CtkThemingEngine *engine,
                                 const gchar      *property,
                                 CtkStateFlags     state,
                                 GValue           *value)
{
  CtkThemingEnginePrivate *priv;

  g_return_if_fail (CTK_IS_THEMING_ENGINE (engine));
  g_return_if_fail (property != NULL);
  g_return_if_fail (value != NULL);

  priv = engine->priv;
  ctk_style_context_get_property (priv->context, property, state, value);
}

/**
 * ctk_theming_engine_get_valist:
 * @engine: a #CtkThemingEngine
 * @state: state to retrieve values for
 * @args: va_list of property name/return location pairs, followed by %NULL
 *
 * Retrieves several style property values that apply to the currently
 * rendered element.
 *
 * Since: 3.0
 *
 * Deprecated: 3.14
 **/
void
ctk_theming_engine_get_valist (CtkThemingEngine *engine,
                               CtkStateFlags     state,
                               va_list           args)
{
  CtkThemingEnginePrivate *priv;

  g_return_if_fail (CTK_IS_THEMING_ENGINE (engine));

  priv = engine->priv;
  ctk_style_context_get_valist (priv->context, state, args);
}

/**
 * ctk_theming_engine_get:
 * @engine: a #CtkThemingEngine
 * @state: state to retrieve values for
 * @...: property name /return value pairs, followed by %NULL
 *
 * Retrieves several style property values that apply to the currently
 * rendered element.
 *
 * Since: 3.0
 *
 * Deprecated: 3.14
 **/
void
ctk_theming_engine_get (CtkThemingEngine *engine,
                        CtkStateFlags     state,
                        ...)
{
  CtkThemingEnginePrivate *priv;
  va_list args;

  g_return_if_fail (CTK_IS_THEMING_ENGINE (engine));

  priv = engine->priv;

  va_start (args, state);
  ctk_style_context_get_valist (priv->context, state, args);
  va_end (args);
}

/**
 * ctk_theming_engine_get_style_property:
 * @engine: a #CtkThemingEngine
 * @property_name: the name of the widget style property
 * @value: (out): Return location for the property value, free with
 *         g_value_unset() after use.
 *
 * Gets the value for a widget style property.
 *
 * Since: 3.0
 *
 * Deprecated: 3.14
 **/
void
ctk_theming_engine_get_style_property (CtkThemingEngine *engine,
                                       const gchar      *property_name,
                                       GValue           *value)
{
  CtkThemingEnginePrivate *priv;

  g_return_if_fail (CTK_IS_THEMING_ENGINE (engine));
  g_return_if_fail (property_name != NULL);

  priv = engine->priv;
  ctk_style_context_get_style_property (priv->context, property_name, value);
}

/**
 * ctk_theming_engine_get_style_valist:
 * @engine: a #CtkThemingEngine
 * @args: va_list of property name/return location pairs, followed by %NULL
 *
 * Retrieves several widget style properties from @engine according to the
 * currently rendered content’s style.
 *
 * Since: 3.0
 *
 * Deprecated: 3.14
 **/
void
ctk_theming_engine_get_style_valist (CtkThemingEngine *engine,
                                     va_list           args)
{
  CtkThemingEnginePrivate *priv;

  g_return_if_fail (CTK_IS_THEMING_ENGINE (engine));

  priv = engine->priv;
  ctk_style_context_get_style_valist (priv->context, args);
}

/**
 * ctk_theming_engine_get_style:
 * @engine: a #CtkThemingEngine
 * @...: property name /return value pairs, followed by %NULL
 *
 * Retrieves several widget style properties from @engine according
 * to the currently rendered content’s style.
 *
 * Since: 3.0
 *
 * Deprecated: 3.14
 **/
void
ctk_theming_engine_get_style (CtkThemingEngine *engine,
                              ...)
{
  CtkThemingEnginePrivate *priv;
  va_list args;

  g_return_if_fail (CTK_IS_THEMING_ENGINE (engine));

  priv = engine->priv;

  va_start (args, engine);
  ctk_style_context_get_style_valist (priv->context, args);
  va_end (args);
}

/**
 * ctk_theming_engine_lookup_color:
 * @engine: a #CtkThemingEngine
 * @color_name: color name to lookup
 * @color: (out): Return location for the looked up color
 *
 * Looks up and resolves a color name in the current style’s color map.
 *
 * Returns: %TRUE if @color_name was found and resolved, %FALSE otherwise
 *
 * Since: 3.0
 *
 * Deprecated: 3.14
 **/
gboolean
ctk_theming_engine_lookup_color (CtkThemingEngine *engine,
                                 const gchar      *color_name,
                                 CdkRGBA          *color)
{
  CtkThemingEnginePrivate *priv;

  g_return_val_if_fail (CTK_IS_THEMING_ENGINE (engine), FALSE);
  g_return_val_if_fail (color_name != NULL, FALSE);

  priv = engine->priv;
  return ctk_style_context_lookup_color (priv->context, color_name, color);
}

/**
 * ctk_theming_engine_get_state:
 * @engine: a #CtkThemingEngine
 *
 * returns the state used when rendering.
 *
 * Returns: the state flags
 *
 * Since: 3.0
 *
 * Deprecated: 3.14
 **/
CtkStateFlags
ctk_theming_engine_get_state (CtkThemingEngine *engine)
{
  CtkThemingEnginePrivate *priv;

  g_return_val_if_fail (CTK_IS_THEMING_ENGINE (engine), 0);

  priv = engine->priv;
  return ctk_style_context_get_state (priv->context);
}

/**
 * ctk_theming_engine_state_is_running:
 * @engine: a #CtkThemingEngine
 * @state: a widget state
 * @progress: (out): return location for the transition progress
 *
 * Returns %TRUE if there is a transition animation running for the
 * current region (see ctk_style_context_push_animatable_region()).
 *
 * If @progress is not %NULL, the animation progress will be returned
 * there, 0.0 means the state is closest to being %FALSE, while 1.0 means
 * it’s closest to being %TRUE. This means transition animations will
 * run from 0 to 1 when @state is being set to %TRUE and from 1 to 0 when
 * it’s being set to %FALSE.
 *
 * Returns: %TRUE if there is a running transition animation for @state.
 *
 * Since: 3.0
 *
 * Deprecated: 3.6: Always returns %FALSE
 **/
gboolean
ctk_theming_engine_state_is_running (CtkThemingEngine *engine,
                                     CtkStateType      state,
                                     gdouble          *progress)
{
  g_return_val_if_fail (CTK_IS_THEMING_ENGINE (engine), FALSE);

  return FALSE;
}

/**
 * ctk_theming_engine_get_path:
 * @engine: a #CtkThemingEngine
 *
 * Returns the widget path used for style matching.
 *
 * Returns: (transfer none): A #CtkWidgetPath
 *
 * Since: 3.0
 *
 * Deprecated: 3.14
 **/
const CtkWidgetPath *
ctk_theming_engine_get_path (CtkThemingEngine *engine)
{
  CtkThemingEnginePrivate *priv;

  g_return_val_if_fail (CTK_IS_THEMING_ENGINE (engine), NULL);

  priv = engine->priv;
  return ctk_style_context_get_path (priv->context);
}

/**
 * ctk_theming_engine_has_class:
 * @engine: a #CtkThemingEngine
 * @style_class: class name to look up
 *
 * Returns %TRUE if the currently rendered contents have
 * defined the given class name.
 *
 * Returns: %TRUE if @engine has @class_name defined
 *
 * Since: 3.0
 *
 * Deprecated: 3.14
 **/
gboolean
ctk_theming_engine_has_class (CtkThemingEngine *engine,
                              const gchar      *style_class)
{
  CtkThemingEnginePrivate *priv;

  g_return_val_if_fail (CTK_IS_THEMING_ENGINE (engine), FALSE);

  priv = engine->priv;
  return ctk_style_context_has_class (priv->context, style_class);
}

/**
 * ctk_theming_engine_has_region:
 * @engine: a #CtkThemingEngine
 * @style_region: a region name
 * @flags: (out) (allow-none): return location for region flags
 *
 * Returns %TRUE if the currently rendered contents have the
 * region defined. If @flags_return is not %NULL, it is set
 * to the flags affecting the region.
 *
 * Returns: %TRUE if region is defined
 *
 * Since: 3.0
 *
 * Deprecated: 3.14
 **/
gboolean
ctk_theming_engine_has_region (CtkThemingEngine *engine,
                               const gchar      *style_region,
                               CtkRegionFlags   *flags)
{
  CtkThemingEnginePrivate *priv;

  if (flags)
    *flags = 0;

  g_return_val_if_fail (CTK_IS_THEMING_ENGINE (engine), FALSE);

  priv = engine->priv;
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  return ctk_style_context_has_region (priv->context, style_region, flags);
G_GNUC_END_IGNORE_DEPRECATIONS
}

/**
 * ctk_theming_engine_get_direction:
 * @engine: a #CtkThemingEngine
 *
 * Returns the widget direction used for rendering.
 *
 * Returns: the widget direction
 *
 * Since: 3.0
 *
 * Deprecated: 3.8: Use ctk_theming_engine_get_state() and
 *   check for #CTK_STATE_FLAG_DIR_LTR and
 *   #CTK_STATE_FLAG_DIR_RTL instead.
 **/
CtkTextDirection
ctk_theming_engine_get_direction (CtkThemingEngine *engine)
{
  CtkThemingEnginePrivate *priv;

  g_return_val_if_fail (CTK_IS_THEMING_ENGINE (engine), CTK_TEXT_DIR_LTR);

  priv = engine->priv;
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  return ctk_style_context_get_direction (priv->context);
  G_GNUC_END_IGNORE_DEPRECATIONS;
}

/**
 * ctk_theming_engine_get_junction_sides:
 * @engine: a #CtkThemingEngine
 *
 * Returns the widget direction used for rendering.
 *
 * Returns: the widget direction
 *
 * Since: 3.0
 *
 * Deprecated: 3.14
 **/
CtkJunctionSides
ctk_theming_engine_get_junction_sides (CtkThemingEngine *engine)
{
  CtkThemingEnginePrivate *priv;

  g_return_val_if_fail (CTK_IS_THEMING_ENGINE (engine), 0);

  priv = engine->priv;
  return ctk_style_context_get_junction_sides (priv->context);
}

/**
 * ctk_theming_engine_get_color:
 * @engine: a #CtkThemingEngine
 * @state: state to retrieve the color for
 * @color: (out): return value for the foreground color
 *
 * Gets the foreground color for a given state.
 *
 * Since: 3.0
 *
 * Deprecated: 3.14
 **/
void
ctk_theming_engine_get_color (CtkThemingEngine *engine,
                              CtkStateFlags     state,
                              CdkRGBA          *color)
{
  CtkThemingEnginePrivate *priv;

  g_return_if_fail (CTK_IS_THEMING_ENGINE (engine));

  priv = engine->priv;
  ctk_style_context_get_color (priv->context, state, color);
}

/**
 * ctk_theming_engine_get_background_color:
 * @engine: a #CtkThemingEngine
 * @state: state to retrieve the color for
 * @color: (out): return value for the background color
 *
 * Gets the background color for a given state.
 *
 * Since: 3.0
 *
 * Deprecated: 3.14
 **/
void
ctk_theming_engine_get_background_color (CtkThemingEngine *engine,
                                         CtkStateFlags     state,
                                         CdkRGBA          *color)
{
  CtkThemingEnginePrivate *priv;

  g_return_if_fail (CTK_IS_THEMING_ENGINE (engine));

  priv = engine->priv;
  ctk_style_context_get_background_color (priv->context, state, color);
}

/**
 * ctk_theming_engine_get_border_color:
 * @engine: a #CtkThemingEngine
 * @state: state to retrieve the color for
 * @color: (out): return value for the border color
 *
 * Gets the border color for a given state.
 *
 * Since: 3.0
 *
 * Deprecated: 3.14
 **/
void
ctk_theming_engine_get_border_color (CtkThemingEngine *engine,
                                     CtkStateFlags     state,
                                     CdkRGBA          *color)
{
  CtkThemingEnginePrivate *priv;

  g_return_if_fail (CTK_IS_THEMING_ENGINE (engine));

  priv = engine->priv;
  ctk_style_context_get_border_color (priv->context, state, color);
}

/**
 * ctk_theming_engine_get_border:
 * @engine: a #CtkThemingEngine
 * @state: state to retrieve the border for
 * @border: (out): return value for the border settings
 *
 * Gets the border for a given state as a #CtkBorder.
 *
 * Since: 3.0
 *
 * Deprecated: 3.14
 **/
void
ctk_theming_engine_get_border (CtkThemingEngine *engine,
                               CtkStateFlags     state,
                               CtkBorder        *border)
{
  CtkThemingEnginePrivate *priv;

  g_return_if_fail (CTK_IS_THEMING_ENGINE (engine));

  priv = engine->priv;
  ctk_style_context_get_border (priv->context, state, border);
}

/**
 * ctk_theming_engine_get_padding:
 * @engine: a #CtkThemingEngine
 * @state: state to retrieve the padding for
 * @padding: (out): return value for the padding settings
 *
 * Gets the padding for a given state as a #CtkBorder.
 *
 * Since: 3.0
 *
 * Deprecated: 3.14
 **/
void
ctk_theming_engine_get_padding (CtkThemingEngine *engine,
                                CtkStateFlags     state,
                                CtkBorder        *padding)
{
  CtkThemingEnginePrivate *priv;

  g_return_if_fail (CTK_IS_THEMING_ENGINE (engine));

  priv = engine->priv;
  ctk_style_context_get_padding (priv->context, state, padding);
}

/**
 * ctk_theming_engine_get_margin:
 * @engine: a #CtkThemingEngine
 * @state: state to retrieve the border for
 * @margin: (out): return value for the margin settings
 *
 * Gets the margin for a given state as a #CtkBorder.
 *
 * Since: 3.0
 *
 * Deprecated: 3.14
 **/
void
ctk_theming_engine_get_margin (CtkThemingEngine *engine,
                               CtkStateFlags     state,
                               CtkBorder        *margin)
{
  CtkThemingEnginePrivate *priv;

  g_return_if_fail (CTK_IS_THEMING_ENGINE (engine));

  priv = engine->priv;
  ctk_style_context_get_margin (priv->context, state, margin);
}

/**
 * ctk_theming_engine_get_font:
 * @engine: a #CtkThemingEngine
 * @state: state to retrieve the font for
 *
 * Returns the font description for a given state.
 *
 * Returns: (transfer none): the #PangoFontDescription for the given
 *          state. This object is owned by CTK+ and should not be
 *          freed.
 *
 * Since: 3.0
 *
 * Deprecated: 3.8: Use ctk_theming_engine_get()
 **/
const PangoFontDescription *
ctk_theming_engine_get_font (CtkThemingEngine *engine,
                             CtkStateFlags     state)
{
  CtkThemingEnginePrivate *priv;

  g_return_val_if_fail (CTK_IS_THEMING_ENGINE (engine), NULL);

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  priv = engine->priv;
  return ctk_style_context_get_font (priv->context, state);
  G_GNUC_END_IGNORE_DEPRECATIONS;
}

/* CtkThemingModule */

static gboolean
ctk_theming_module_load (GTypeModule *type_module)
{
  CtkThemingModule *theming_module;
  GModule *module;
  gchar *name, *module_path;

  theming_module = CTK_THEMING_MODULE (type_module);
  name = theming_module->name;
  module_path = _ctk_find_module (name, "theming-engines");

  if (!module_path)
    return FALSE;

  module = g_module_open (module_path, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);
  g_free (module_path);

  if (!module)
    return FALSE;

  if (!g_module_symbol (module, "theme_init",
                        (gpointer *) &theming_module->init) ||
      !g_module_symbol (module, "theme_exit",
                        (gpointer *) &theming_module->exit) ||
      !g_module_symbol (module, "create_engine",
                        (gpointer *) &theming_module->create_engine))
    {
      g_module_close (module);

      return FALSE;
    }

  theming_module->module = module;

  theming_module->init (G_TYPE_MODULE (theming_module));

  return TRUE;
}

static void
ctk_theming_module_unload (GTypeModule *type_module)
{
  CtkThemingModule *theming_module;

  theming_module = CTK_THEMING_MODULE (type_module);

  theming_module->exit ();

  g_module_close (theming_module->module);

  theming_module->module = NULL;
  theming_module->init = NULL;
  theming_module->exit = NULL;
  theming_module->create_engine = NULL;
}

static void
ctk_theming_module_class_init (CtkThemingModuleClass *klass)
{
  GTypeModuleClass *module_class = G_TYPE_MODULE_CLASS (klass);

  module_class->load = ctk_theming_module_load;
  module_class->unload = ctk_theming_module_unload;
}

static void
ctk_theming_module_init (CtkThemingModule *module)
{
}

/**
 * ctk_theming_engine_load:
 * @name: Theme engine name to load
 *
 * Loads and initializes a theming engine module from the
 * standard directories.
 *
 * Returns: (nullable) (transfer none): A theming engine, or %NULL if
 * the engine @name doesn’t exist.
 *
 * Deprecated: 3.14
 **/
CtkThemingEngine *
ctk_theming_engine_load (const gchar *name)
{
  static GHashTable *engines = NULL;
  static CtkThemingEngine *default_engine;
  CtkThemingEngine *engine = NULL;

  if (name)
    {
      if (!engines)
        engines = g_hash_table_new (g_str_hash, g_str_equal);

      engine = g_hash_table_lookup (engines, name);

      if (!engine)
        {
          CtkThemingModule *module;

          module = g_object_new (CTK_TYPE_THEMING_MODULE, NULL);
          g_type_module_set_name (G_TYPE_MODULE (module), name);
          module->name = g_strdup (name);

          if (module && g_type_module_use (G_TYPE_MODULE (module)))
            {
              engine = (module->create_engine) ();

              if (engine)
                g_hash_table_insert (engines, module->name, engine);
            }
        }
    }
  else
    {
      if (G_UNLIKELY (!default_engine))
        default_engine = g_object_new (CTK_TYPE_THEMING_ENGINE, NULL);

      engine = default_engine;
    }

  return engine;
}

/**
 * ctk_theming_engine_get_screen:
 * @engine: a #CtkThemingEngine
 *
 * Returns the #CdkScreen to which @engine currently rendering to.
 *
 * Returns: (nullable) (transfer none): a #CdkScreen, or %NULL.
 *
 * Deprecated: 3.14
 **/
CdkScreen *
ctk_theming_engine_get_screen (CtkThemingEngine *engine)
{
  CtkThemingEnginePrivate *priv;

  g_return_val_if_fail (CTK_IS_THEMING_ENGINE (engine), NULL);

  priv = engine->priv;
  return ctk_style_context_get_screen (priv->context);
}

/* Paint method implementations */
static void
ctk_theming_engine_render_check (CtkThemingEngine *engine,
                                 cairo_t          *cr,
                                 gdouble           x,
                                 gdouble           y,
                                 gdouble           width,
                                 gdouble           height)
{
  ctk_render_check (engine->priv->context, cr, x, y, width, height);
}

static void
ctk_theming_engine_render_option (CtkThemingEngine *engine,
                                  cairo_t          *cr,
                                  gdouble           x,
                                  gdouble           y,
                                  gdouble           width,
                                  gdouble           height)
{
  ctk_render_option (engine->priv->context, cr, x, y, width, height);
}

static void
ctk_theming_engine_render_arrow (CtkThemingEngine *engine,
                                 cairo_t          *cr,
                                 gdouble           angle,
                                 gdouble           x,
                                 gdouble           y,
                                 gdouble           size)
{
  ctk_render_arrow (engine->priv->context, cr, angle, x, y, size);
}

static void
ctk_theming_engine_render_background (CtkThemingEngine *engine,
                                      cairo_t          *cr,
                                      gdouble           x,
                                      gdouble           y,
                                      gdouble           width,
                                      gdouble           height)
{
  ctk_render_background (engine->priv->context, cr, x, y, width, height);
}

static void
ctk_theming_engine_render_frame (CtkThemingEngine *engine,
                                 cairo_t          *cr,
                                 gdouble           x,
                                 gdouble           y,
                                 gdouble           width,
                                 gdouble           height)
{
  ctk_render_frame (engine->priv->context, cr, x, y, width, height);
}

static void
ctk_theming_engine_render_expander (CtkThemingEngine *engine,
                                    cairo_t          *cr,
                                    gdouble           x,
                                    gdouble           y,
                                    gdouble           width,
                                    gdouble           height)
{
  ctk_render_expander (engine->priv->context, cr, x, y, width, height);
}

static void
ctk_theming_engine_render_focus (CtkThemingEngine *engine,
                                 cairo_t          *cr,
                                 gdouble           x,
                                 gdouble           y,
                                 gdouble           width,
                                 gdouble           height)
{
  ctk_render_focus (engine->priv->context, cr, x, y, width, height);
}

static void
ctk_theming_engine_render_line (CtkThemingEngine *engine,
                                cairo_t          *cr,
                                gdouble           x0,
                                gdouble           y0,
                                gdouble           x1,
                                gdouble           y1)
{
  ctk_render_line (engine->priv->context, cr, x0, y0, x1, y1);
}

static void
ctk_theming_engine_render_layout (CtkThemingEngine *engine,
                                  cairo_t          *cr,
                                  gdouble           x,
                                  gdouble           y,
                                  PangoLayout      *layout)
{
  ctk_render_layout (engine->priv->context, cr, x, y, layout);
}

static void
ctk_theming_engine_render_slider (CtkThemingEngine *engine,
                                  cairo_t          *cr,
                                  gdouble           x,
                                  gdouble           y,
                                  gdouble           width,
                                  gdouble           height,
                                  CtkOrientation    orientation)
{
  ctk_render_slider (engine->priv->context, cr, x, y, width, height, orientation);
}

static void
ctk_theming_engine_render_frame_gap (CtkThemingEngine *engine,
                                     cairo_t          *cr,
                                     gdouble           x,
                                     gdouble           y,
                                     gdouble           width,
                                     gdouble           height,
                                     CtkPositionType   gap_side,
                                     gdouble           xy0_gap,
                                     gdouble           xy1_gap)
{
  ctk_render_frame_gap (engine->priv->context, cr, x, y, width, height, gap_side, xy0_gap, xy1_gap);
}

static void
ctk_theming_engine_render_extension (CtkThemingEngine *engine,
                                     cairo_t          *cr,
                                     gdouble           x,
                                     gdouble           y,
                                     gdouble           width,
                                     gdouble           height,
                                     CtkPositionType   gap_side)
{
  ctk_render_extension (engine->priv->context, cr, x, y, width, height, gap_side);
}

static void
ctk_theming_engine_render_handle (CtkThemingEngine *engine,
                                  cairo_t          *cr,
                                  gdouble           x,
                                  gdouble           y,
                                  gdouble           width,
                                  gdouble           height)
{
  ctk_render_handle (engine->priv->context, cr, x, y, width, height);
}

static void
ctk_theming_engine_render_activity (CtkThemingEngine *engine,
                                    cairo_t          *cr,
                                    gdouble           x,
                                    gdouble           y,
                                    gdouble           width,
                                    gdouble           height)
{
  ctk_render_activity (engine->priv->context, cr, x, y, width, height);
}

static CdkPixbuf *
ctk_theming_engine_render_icon_pixbuf (CtkThemingEngine    *engine,
                                       const CtkIconSource *source,
                                       CtkIconSize          size)
{
  return ctk_render_icon_pixbuf (engine->priv->context, source, size);
}

static void
ctk_theming_engine_render_icon (CtkThemingEngine *engine,
                                cairo_t *cr,
				CdkPixbuf *pixbuf,
                                gdouble x,
                                gdouble y)
{
  ctk_render_icon (engine->priv->context, cr, pixbuf, x, y);
}

static void
ctk_theming_engine_render_icon_surface (CtkThemingEngine *engine,
					cairo_t *cr,
					cairo_surface_t *surface,
					gdouble x,
					gdouble y)
{
  ctk_render_icon_surface (engine->priv->context, cr, surface, x, y);
}

G_GNUC_END_IGNORE_DEPRECATIONS
