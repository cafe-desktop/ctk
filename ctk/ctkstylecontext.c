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

#include "ctkstylecontextprivate.h"

#include <cdk/cdk.h>
#include <math.h>
#include <stdlib.h>
#include <gobject/gvaluecollector.h>

#include "ctkcontainerprivate.h"
#include "ctkcssanimatedstyleprivate.h"
#include "ctkcsscolorvalueprivate.h"
#include "ctkcssenumvalueprivate.h"
#include "ctkcssimagevalueprivate.h"
#include "ctkcssnodedeclarationprivate.h"
#include "ctkcssnodeprivate.h"
#include "ctkcssnumbervalueprivate.h"
#include "ctkcsspathnodeprivate.h"
#include "ctkcssrgbavalueprivate.h"
#include "ctkcsscolorvalueprivate.h"
#include "ctkcssshadowsvalueprivate.h"
#include "ctkcssstaticstyleprivate.h"
#include "ctkcssstylepropertyprivate.h"
#include "ctkcsstransformvalueprivate.h"
#include "ctkcsstransientnodeprivate.h"
#include "ctkcsswidgetnodeprivate.h"
#include "ctkdebug.h"
#include "ctkintl.h"
#include "ctkprivate.h"
#include "ctkrenderbackgroundprivate.h"
#include "ctkrendericonprivate.h"
#include "ctksettings.h"
#include "ctksettingsprivate.h"
#include "ctkstylecascadeprivate.h"
#include "ctkstyleproviderprivate.h"
#include "ctktypebuiltins.h"
#include "ctkwindow.h"
#include "ctkwidgetpath.h"
#include "ctkwidgetprivate.h"

#include "deprecated/ctkgradientprivate.h"
#include "deprecated/ctksymboliccolorprivate.h"

#include "fallback-c89.c"

/**
 * SECTION:ctkstylecontext
 * @Short_description: Rendering UI elements
 * @Title: CtkStyleContext
 *
 * #CtkStyleContext is an object that stores styling information affecting
 * a widget defined by #CtkWidgetPath.
 *
 * In order to construct the final style information, #CtkStyleContext
 * queries information from all attached #CtkStyleProviders. Style providers
 * can be either attached explicitly to the context through
 * ctk_style_context_add_provider(), or to the screen through
 * ctk_style_context_add_provider_for_screen(). The resulting style is a
 * combination of all providers’ information in priority order.
 *
 * For CTK+ widgets, any #CtkStyleContext returned by
 * ctk_widget_get_style_context() will already have a #CtkWidgetPath, a
 * #CdkScreen and RTL/LTR information set. The style context will also be
 * updated automatically if any of these settings change on the widget.
 *
 * If you are using the theming layer standalone, you will need to set a
 * widget path and a screen yourself to the created style context through
 * ctk_style_context_set_path() and possibly ctk_style_context_set_screen(). See
 * the “Foreign drawing“ example in ctk3-demo.
 *
 * # Style Classes # {#ctkstylecontext-classes}
 *
 * Widgets can add style classes to their context, which can be used to associate
 * different styles by class. The documentation for individual widgets lists
 * which style classes it uses itself, and which style classes may be added by
 * applications to affect their appearance.
 *
 * CTK+ defines macros for a number of style classes.
 *
 * # Style Regions
 *
 * Widgets can also add regions with flags to their context. This feature is
 * deprecated and will be removed in a future CTK+ update. Please use style
 * classes instead.
 *
 * CTK+ defines macros for a number of style regions.
 *
 * # Custom styling in UI libraries and applications
 *
 * If you are developing a library with custom #CtkWidgets that
 * render differently than standard components, you may need to add a
 * #CtkStyleProvider yourself with the %CTK_STYLE_PROVIDER_PRIORITY_FALLBACK
 * priority, either a #CtkCssProvider or a custom object implementing the
 * #CtkStyleProvider interface. This way themes may still attempt
 * to style your UI elements in a different way if needed so.
 *
 * If you are using custom styling on an applications, you probably want then
 * to make your style information prevail to the theme’s, so you must use
 * a #CtkStyleProvider with the %CTK_STYLE_PROVIDER_PRIORITY_APPLICATION
 * priority, keep in mind that the user settings in
 * `XDG_CONFIG_HOME/ctk-3.0/ctk.css` will
 * still take precedence over your changes, as it uses the
 * %CTK_STYLE_PROVIDER_PRIORITY_USER priority.
 */

typedef struct PropertyValue PropertyValue;

struct PropertyValue
{
  GType       widget_type;
  GParamSpec *pspec;
  GValue      value;
};

struct _CtkStyleContextPrivate
{
  CdkScreen *screen;

  guint cascade_changed_id;
  CtkStyleCascade *cascade;
  CtkStyleContext *parent;
  CtkCssNode *cssnode;
  GSList *saved_nodes;
  GArray *property_cache;

  CdkFrameClock *frame_clock;

  CtkCssStyleChange *invalidating_context;
};

enum {
  PROP_0,
  PROP_SCREEN,
  PROP_DIRECTION,
  PROP_FRAME_CLOCK,
  PROP_PARENT,
  LAST_PROP
};

enum {
  CHANGED,
  LAST_SIGNAL
};

static GParamSpec *properties[LAST_PROP] = { NULL, };

static guint signals[LAST_SIGNAL] = { 0 };

static void ctk_style_context_finalize (GObject *object);

static void ctk_style_context_impl_set_property (GObject      *object,
                                                 guint         prop_id,
                                                 const GValue *value,
                                                 GParamSpec   *pspec);
static void ctk_style_context_impl_get_property (GObject      *object,
                                                 guint         prop_id,
                                                 GValue       *value,
                                                 GParamSpec   *pspec);

static CtkCssNode * ctk_style_context_get_root (CtkStyleContext *context);

G_DEFINE_TYPE_WITH_PRIVATE (CtkStyleContext, ctk_style_context, G_TYPE_OBJECT)

static void
ctk_style_context_real_changed (CtkStyleContext *context)
{
  CtkStyleContextPrivate *priv = context->priv;

  if (CTK_IS_CSS_WIDGET_NODE (priv->cssnode))
    {
      CtkWidget *widget = ctk_css_widget_node_get_widget (CTK_CSS_WIDGET_NODE (priv->cssnode));
      if (widget != NULL)
        _ctk_widget_style_context_invalidated (widget);
    }
}

static void
ctk_style_context_class_init (CtkStyleContextClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = ctk_style_context_finalize;
  object_class->set_property = ctk_style_context_impl_set_property;
  object_class->get_property = ctk_style_context_impl_get_property;

  klass->changed = ctk_style_context_real_changed;

  /**
   * CtkStyleContext::changed:
   *
   * The ::changed signal is emitted when there is a change in the
   * #CtkStyleContext.
   *
   * For a #CtkStyleContext returned by ctk_widget_get_style_context(), the
   * #CtkWidget::style-updated signal/vfunc might be more convenient to use.
   *
   * This signal is useful when using the theming layer standalone.
   *
   * Since: 3.0
   */
  signals[CHANGED] =
    g_signal_new (I_("changed"),
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (CtkStyleContextClass, changed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  properties[PROP_SCREEN] =
      g_param_spec_object ("screen",
                           P_("Screen"),
                           P_("The associated CdkScreen"),
                           CDK_TYPE_SCREEN,
                           CTK_PARAM_READWRITE);

  properties[PROP_FRAME_CLOCK] =
      g_param_spec_object ("paint-clock",
                           P_("FrameClock"),
                           P_("The associated CdkFrameClock"),
                           CDK_TYPE_FRAME_CLOCK,
                           CTK_PARAM_READWRITE);

  properties[PROP_DIRECTION] =
      g_param_spec_enum ("direction",
                         P_("Direction"),
                         P_("Text direction"),
                         CTK_TYPE_TEXT_DIRECTION,
                         CTK_TEXT_DIR_LTR,
                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_DEPRECATED);

  /**
   * CtkStyleContext:parent:
   *
   * Sets or gets the style context’s parent. See ctk_style_context_set_parent()
   * for details.
   *
   * Since: 3.4
   */
  properties[PROP_PARENT] =
      g_param_spec_object ("parent",
                           P_("Parent"),
                           P_("The parent style context"),
                           CTK_TYPE_STYLE_CONTEXT,
                           CTK_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROP, properties);
}

void
ctk_style_context_clear_property_cache (CtkStyleContext *context)
{
  CtkStyleContextPrivate *priv = context->priv;
  guint i;

  for (i = 0; i < priv->property_cache->len; i++)
    {
      PropertyValue *node = &g_array_index (priv->property_cache, PropertyValue, i);

      g_param_spec_unref (node->pspec);
      g_value_unset (&node->value);
    }

  g_array_set_size (priv->property_cache, 0);
}

static void
ctk_style_context_pop_style_node (CtkStyleContext *context)
{
  CtkStyleContextPrivate *priv = context->priv;

  g_return_if_fail (priv->saved_nodes != NULL);

  if (CTK_IS_CSS_TRANSIENT_NODE (priv->cssnode))
    ctk_css_node_set_parent (priv->cssnode, NULL);
  g_object_unref (priv->cssnode);
  priv->cssnode = priv->saved_nodes->data;
  priv->saved_nodes = g_slist_remove (priv->saved_nodes, priv->cssnode);
}

static void
ctk_style_context_cascade_changed (CtkStyleCascade *cascade,
                                   CtkStyleContext *context)
{
  ctk_css_node_invalidate_style_provider (ctk_style_context_get_root (context));
}

static void
ctk_style_context_set_cascade (CtkStyleContext *context,
                               CtkStyleCascade *cascade)
{
  CtkStyleContextPrivate *priv;

  priv = context->priv;

  if (priv->cascade == cascade)
    return;

  if (priv->cascade)
    {
      g_signal_handler_disconnect (priv->cascade, priv->cascade_changed_id);
      priv->cascade_changed_id = 0;
      g_object_unref (priv->cascade);
    }

  if (cascade)
    {
      g_object_ref (cascade);
      priv->cascade_changed_id = g_signal_connect (cascade,
                                                   "-ctk-private-changed",
                                                   G_CALLBACK (ctk_style_context_cascade_changed),
                                                   context);
    }

  priv->cascade = cascade;

  if (cascade && priv->cssnode != NULL)
    ctk_style_context_cascade_changed (cascade, context);
}

static void
ctk_style_context_init (CtkStyleContext *context)
{
  CtkStyleContextPrivate *priv;

  priv = context->priv = ctk_style_context_get_instance_private (context);

  priv->screen = cdk_screen_get_default ();

  if (priv->screen == NULL)
    g_error ("Can't create a CtkStyleContext without a display connection");

  priv->property_cache = g_array_new (FALSE, FALSE, sizeof (PropertyValue));

  ctk_style_context_set_cascade (context,
                                 _ctk_settings_get_style_cascade (ctk_settings_get_for_screen (priv->screen), 1));

  /* Create default info store */
  priv->cssnode = ctk_css_path_node_new (context);
  ctk_css_node_set_state (priv->cssnode, CTK_STATE_FLAG_DIR_LTR);
}

static void
ctk_style_context_clear_parent (CtkStyleContext *context)
{
  CtkStyleContextPrivate *priv = context->priv;

  if (priv->parent)
    g_object_unref (priv->parent);
}

static void
ctk_style_context_finalize (GObject *object)
{
  CtkStyleContext *context = CTK_STYLE_CONTEXT (object);
  CtkStyleContextPrivate *priv = context->priv;

  while (priv->saved_nodes)
    ctk_style_context_pop_style_node (context);

  if (CTK_IS_CSS_PATH_NODE (priv->cssnode))
    ctk_css_path_node_unset_context (CTK_CSS_PATH_NODE (priv->cssnode));

  ctk_style_context_clear_parent (context);
  ctk_style_context_set_cascade (context, NULL);

  g_object_unref (priv->cssnode);

  ctk_style_context_clear_property_cache (context);
  g_array_free (priv->property_cache, TRUE);

  G_OBJECT_CLASS (ctk_style_context_parent_class)->finalize (object);
}

static void
ctk_style_context_impl_set_property (GObject      *object,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  CtkStyleContext *context = CTK_STYLE_CONTEXT (object);

  switch (prop_id)
    {
    case PROP_SCREEN:
      ctk_style_context_set_screen (context, g_value_get_object (value));
      break;
    case PROP_DIRECTION:
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      ctk_style_context_set_direction (context, g_value_get_enum (value));
      G_GNUC_END_IGNORE_DEPRECATIONS;
      break;
    case PROP_FRAME_CLOCK:
      ctk_style_context_set_frame_clock (context, g_value_get_object (value));
      break;
    case PROP_PARENT:
      ctk_style_context_set_parent (context, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_style_context_impl_get_property (GObject    *object,
                                     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  CtkStyleContext *context = CTK_STYLE_CONTEXT (object);
  CtkStyleContextPrivate *priv = context->priv;

  switch (prop_id)
    {
    case PROP_SCREEN:
      g_value_set_object (value, priv->screen);
      break;
    case PROP_DIRECTION:
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      g_value_set_enum (value, ctk_style_context_get_direction (context));
      G_GNUC_END_IGNORE_DEPRECATIONS;
      break;
    case PROP_FRAME_CLOCK:
      g_value_set_object (value, priv->frame_clock);
      break;
    case PROP_PARENT:
      g_value_set_object (value, priv->parent);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/* returns TRUE if someone called ctk_style_context_save() but hasn’t
 * called ctk_style_context_restore() yet.
 * In those situations we don’t invalidate the context when somebody
 * changes state/regions/classes.
 */
static gboolean
ctk_style_context_is_saved (CtkStyleContext *context)
{
  return context->priv->saved_nodes != NULL;
}

static CtkCssNode *
ctk_style_context_get_root (CtkStyleContext *context)
{
  CtkStyleContextPrivate *priv = context->priv;

  if (priv->saved_nodes != NULL)
    return g_slist_last (priv->saved_nodes)->data;
  else
    return priv->cssnode;
}

CtkStyleProviderPrivate *
ctk_style_context_get_style_provider (CtkStyleContext *context)
{
  return CTK_STYLE_PROVIDER_PRIVATE (context->priv->cascade);
}

static gboolean
ctk_style_context_has_custom_cascade (CtkStyleContext *context)
{
  CtkStyleContextPrivate *priv = context->priv;
  CtkSettings *settings = ctk_settings_get_for_screen (priv->screen);

  return priv->cascade != _ctk_settings_get_style_cascade (settings, _ctk_style_cascade_get_scale (priv->cascade));
}

CtkCssStyle *
ctk_style_context_lookup_style (CtkStyleContext *context)
{
  /* Code will recreate style if it was changed */
  return ctk_css_node_get_style (context->priv->cssnode);
}

CtkCssNode*
ctk_style_context_get_node (CtkStyleContext *context)
{
  return context->priv->cssnode;
}

static CtkStateFlags
ctk_style_context_push_state (CtkStyleContext *context,
                              CtkStateFlags    state)
{
  CtkStyleContextPrivate *priv = context->priv;
  CtkStateFlags current_state;
  CtkCssNode *root;

  current_state = ctk_css_node_get_state (priv->cssnode);

  if (current_state == state)
    return state;

  root = ctk_style_context_get_root (context);

  if (CTK_IS_CSS_TRANSIENT_NODE (priv->cssnode))
    {
      /* don't emit a warning, changing state here is fine */
    }
  else if (CTK_IS_CSS_WIDGET_NODE (root))
    {
      CtkWidget *widget = ctk_css_widget_node_get_widget (CTK_CSS_WIDGET_NODE (root));
      g_debug ("State %u for %s %p doesn't match state %u set via ctk_style_context_set_state ()",
               state, (widget == NULL) ? "(null)" : ctk_widget_get_name (widget),
               widget, ctk_css_node_get_state (priv->cssnode));
    }
  else
    {
      g_debug ("State %u for context %p doesn't match state %u set via ctk_style_context_set_state ()",
               state, context, ctk_css_node_get_state (priv->cssnode));
    }

  ctk_css_node_set_state (priv->cssnode, state);

  return current_state;
}

static void
ctk_style_context_pop_state (CtkStyleContext *context,
                             CtkStateFlags    saved_state)
{
  ctk_css_node_set_state (context->priv->cssnode, saved_state);
}

/**
 * ctk_style_context_new:
 *
 * Creates a standalone #CtkStyleContext, this style context
 * won’t be attached to any widget, so you may want
 * to call ctk_style_context_set_path() yourself.
 *
 * This function is only useful when using the theming layer
 * separated from CTK+, if you are using #CtkStyleContext to
 * theme #CtkWidgets, use ctk_widget_get_style_context()
 * in order to get a style context ready to theme the widget.
 *
 * Returns: A newly created #CtkStyleContext.
 **/
CtkStyleContext *
ctk_style_context_new (void)
{
  return g_object_new (CTK_TYPE_STYLE_CONTEXT, NULL);
}

CtkStyleContext *
ctk_style_context_new_for_node (CtkCssNode *node)
{
  CtkStyleContext *context;

  g_return_val_if_fail (CTK_IS_CSS_NODE (node), NULL);

  context = ctk_style_context_new ();
  g_set_object (&context->priv->cssnode, node);

  return context;
}

/**
 * ctk_style_context_add_provider:
 * @context: a #CtkStyleContext
 * @provider: a #CtkStyleProvider
 * @priority: the priority of the style provider. The lower
 *            it is, the earlier it will be used in the style
 *            construction. Typically this will be in the range
 *            between %CTK_STYLE_PROVIDER_PRIORITY_FALLBACK and
 *            %CTK_STYLE_PROVIDER_PRIORITY_USER
 *
 * Adds a style provider to @context, to be used in style construction.
 * Note that a style provider added by this function only affects
 * the style of the widget to which @context belongs. If you want
 * to affect the style of all widgets, use
 * ctk_style_context_add_provider_for_screen().
 *
 * Note: If both priorities are the same, a #CtkStyleProvider
 * added through this function takes precedence over another added
 * through ctk_style_context_add_provider_for_screen().
 *
 * Since: 3.0
 **/
void
ctk_style_context_add_provider (CtkStyleContext  *context,
                                CtkStyleProvider *provider,
                                guint             priority)
{
  CtkStyleContextPrivate *priv;

  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
  g_return_if_fail (CTK_IS_STYLE_PROVIDER (provider));

  priv = context->priv;

  if (!ctk_style_context_has_custom_cascade (context))
    {
      CtkStyleCascade *new_cascade;

      new_cascade = _ctk_style_cascade_new ();
      _ctk_style_cascade_set_scale (new_cascade, _ctk_style_cascade_get_scale (priv->cascade));
      _ctk_style_cascade_set_parent (new_cascade,
                                     _ctk_settings_get_style_cascade (ctk_settings_get_for_screen (priv->screen), 1));
      _ctk_style_cascade_add_provider (new_cascade, provider, priority);
      ctk_style_context_set_cascade (context, new_cascade);
      g_object_unref (new_cascade);
    }
  else
    {
      _ctk_style_cascade_add_provider (priv->cascade, provider, priority);
    }
}

/**
 * ctk_style_context_remove_provider:
 * @context: a #CtkStyleContext
 * @provider: a #CtkStyleProvider
 *
 * Removes @provider from the style providers list in @context.
 *
 * Since: 3.0
 **/
void
ctk_style_context_remove_provider (CtkStyleContext  *context,
                                   CtkStyleProvider *provider)
{
  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
  g_return_if_fail (CTK_IS_STYLE_PROVIDER (provider));

  if (!ctk_style_context_has_custom_cascade (context))
    return;

  _ctk_style_cascade_remove_provider (context->priv->cascade, provider);
}

/**
 * ctk_style_context_reset_widgets:
 * @screen: a #CdkScreen
 *
 * This function recomputes the styles for all widgets under a particular
 * #CdkScreen. This is useful when some global parameter has changed that
 * affects the appearance of all widgets, because when a widget gets a new
 * style, it will both redraw and recompute any cached information about
 * its appearance. As an example, it is used when the color scheme changes
 * in the related #CtkSettings object.
 *
 * Since: 3.0
 **/
void
ctk_style_context_reset_widgets (CdkScreen *screen)
{
  GList *list, *toplevels;

  toplevels = ctk_window_list_toplevels ();
  g_list_foreach (toplevels, (GFunc) g_object_ref, NULL);

  for (list = toplevels; list; list = list->next)
    {
      if (ctk_widget_get_screen (list->data) == screen)
        ctk_widget_reset_style (list->data);

      g_object_unref (list->data);
    }

  g_list_free (toplevels);
}

/**
 * ctk_style_context_add_provider_for_screen:
 * @screen: a #CdkScreen
 * @provider: a #CtkStyleProvider
 * @priority: the priority of the style provider. The lower
 *            it is, the earlier it will be used in the style
 *            construction. Typically this will be in the range
 *            between %CTK_STYLE_PROVIDER_PRIORITY_FALLBACK and
 *            %CTK_STYLE_PROVIDER_PRIORITY_USER
 *
 * Adds a global style provider to @screen, which will be used
 * in style construction for all #CtkStyleContexts under @screen.
 *
 * CTK+ uses this to make styling information from #CtkSettings
 * available.
 *
 * Note: If both priorities are the same, A #CtkStyleProvider
 * added through ctk_style_context_add_provider() takes precedence
 * over another added through this function.
 *
 * Since: 3.0
 **/
void
ctk_style_context_add_provider_for_screen (CdkScreen        *screen,
                                           CtkStyleProvider *provider,
                                           guint             priority)
{
  CtkStyleCascade *cascade;

  g_return_if_fail (CDK_IS_SCREEN (screen));
  g_return_if_fail (CTK_IS_STYLE_PROVIDER (provider));
  g_return_if_fail (!CTK_IS_SETTINGS (provider) || _ctk_settings_get_screen (CTK_SETTINGS (provider)) == screen);

  cascade = _ctk_settings_get_style_cascade (ctk_settings_get_for_screen (screen), 1);
  _ctk_style_cascade_add_provider (cascade, provider, priority);
}

/**
 * ctk_style_context_remove_provider_for_screen:
 * @screen: a #CdkScreen
 * @provider: a #CtkStyleProvider
 *
 * Removes @provider from the global style providers list in @screen.
 *
 * Since: 3.0
 **/
void
ctk_style_context_remove_provider_for_screen (CdkScreen        *screen,
                                              CtkStyleProvider *provider)
{
  CtkStyleCascade *cascade;

  g_return_if_fail (CDK_IS_SCREEN (screen));
  g_return_if_fail (CTK_IS_STYLE_PROVIDER (provider));
  g_return_if_fail (!CTK_IS_SETTINGS (provider));

  cascade = _ctk_settings_get_style_cascade (ctk_settings_get_for_screen (screen), 1);
  _ctk_style_cascade_remove_provider (cascade, provider);
}

/**
 * ctk_style_context_get_section:
 * @context: a #CtkStyleContext
 * @property: style property name
 *
 * Queries the location in the CSS where @property was defined for the
 * current @context. Note that the state to be queried is taken from
 * ctk_style_context_get_state().
 *
 * If the location is not available, %NULL will be returned. The
 * location might not be available for various reasons, such as the
 * property being overridden, @property not naming a supported CSS
 * property or tracking of definitions being disabled for performance
 * reasons.
 *
 * Shorthand CSS properties cannot be queried for a location and will
 * always return %NULL.
 *
 * Returns: (nullable) (transfer none): %NULL or the section where a value
 * for @property was defined
 **/
CtkCssSection *
ctk_style_context_get_section (CtkStyleContext *context,
                               const gchar     *property)
{
  CtkCssStyle *values;
  CtkStyleProperty *prop;

  g_return_val_if_fail (CTK_IS_STYLE_CONTEXT (context), NULL);
  g_return_val_if_fail (property != NULL, NULL);

  prop = _ctk_style_property_lookup (property);
  if (!CTK_IS_CSS_STYLE_PROPERTY (prop))
    return NULL;

  values = ctk_style_context_lookup_style (context);
  return ctk_css_style_get_section (values, _ctk_css_style_property_get_id (CTK_CSS_STYLE_PROPERTY (prop)));
}

static CtkCssValue *
ctk_style_context_query_func (guint    id,
                              gpointer values)
{
  return ctk_css_style_get_value (values, id);
}

/**
 * ctk_style_context_get_property:
 * @context: a #CtkStyleContext
 * @property: style property name
 * @state: state to retrieve the property value for
 * @value: (out) (transfer full):  return location for the style property value
 *
 * Gets a style property from @context for the given state.
 *
 * Note that not all CSS properties that are supported by CTK+ can be
 * retrieved in this way, since they may not be representable as #GValue.
 * CTK+ defines macros for a number of properties that can be used
 * with this function.
 *
 * Note that passing a state other than the current state of @context
 * is not recommended unless the style context has been saved with
 * ctk_style_context_save().
 *
 * When @value is no longer needed, g_value_unset() must be called
 * to free any allocated memory.
 *
 * Since: 3.0
 **/
void
ctk_style_context_get_property (CtkStyleContext *context,
                                const gchar     *property,
                                CtkStateFlags    state,
                                GValue          *value)
{
  CtkStateFlags saved_state;
  CtkStyleProperty *prop;

  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
  g_return_if_fail (property != NULL);
  g_return_if_fail (value != NULL);

  prop = _ctk_style_property_lookup (property);
  if (prop == NULL)
    {
      g_warning ("Style property \"%s\" is not registered", property);
      return;
    }
  if (_ctk_style_property_get_value_type (prop) == G_TYPE_NONE)
    {
      g_warning ("Style property \"%s\" is not gettable", property);
      return;
    }

  saved_state = ctk_style_context_push_state (context, state);
  _ctk_style_property_query (prop,
                             value,
                             ctk_style_context_query_func,
                             ctk_css_node_get_style (context->priv->cssnode));
  ctk_style_context_pop_state (context, saved_state);
}

/**
 * ctk_style_context_get_valist:
 * @context: a #CtkStyleContext
 * @state: state to retrieve the property values for
 * @args: va_list of property name/return location pairs, followed by %NULL
 *
 * Retrieves several style property values from @context for a given state.
 *
 * See ctk_style_context_get_property() for details.
 *
 * Since: 3.0
 */
void
ctk_style_context_get_valist (CtkStyleContext *context,
                              CtkStateFlags    state,
                              va_list          args)
{
  const gchar *property_name;

  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));

  property_name = va_arg (args, const gchar *);

  while (property_name)
    {
      gchar *error = NULL;
      GValue value = G_VALUE_INIT;

      ctk_style_context_get_property (context,
                                      property_name,
                                      state,
                                      &value);

      G_VALUE_LCOPY (&value, args, 0, &error);
      g_value_unset (&value);

      if (error)
        {
          g_warning ("Could not get style property \"%s\": %s", property_name, error);
          g_free (error);
          break;
        }

      property_name = va_arg (args, const gchar *);
    }
}

/**
 * ctk_style_context_get:
 * @context: a #CtkStyleContext
 * @state: state to retrieve the property values for
 * @...: property name /return value pairs, followed by %NULL
 *
 * Retrieves several style property values from @context for a
 * given state.
 *
 * See ctk_style_context_get_property() for details.
 *
 * Since: 3.0
 */
void
ctk_style_context_get (CtkStyleContext *context,
                       CtkStateFlags    state,
                       ...)
{
  va_list args;

  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));

  va_start (args, state);
  ctk_style_context_get_valist (context, state, args);
  va_end (args);
}

/*
 * ctk_style_context_set_id:
 * @context: a #CtkStyleContext
 * @id: (allow-none): the id to use or %NULL for none.
 *
 * Sets the CSS ID to be used when obtaining style information.
 **/
void
ctk_style_context_set_id (CtkStyleContext *context,
                          const char      *id)
{
  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));

  ctk_css_node_set_id (context->priv->cssnode, id);
}

/*
 * ctk_style_context_get_id:
 * @context: a #CtkStyleContext
 *
 * Returns the CSS ID used when obtaining style information.
 *
 * Returns: (nullable): the ID or %NULL if no ID is set.
 **/
const char *
ctk_style_context_get_id (CtkStyleContext *context)
{
  g_return_val_if_fail (CTK_IS_STYLE_CONTEXT (context), NULL);

  return ctk_css_node_get_id (context->priv->cssnode);
}

/**
 * ctk_style_context_set_state:
 * @context: a #CtkStyleContext
 * @flags: state to represent
 *
 * Sets the state to be used for style matching.
 *
 * Since: 3.0
 **/
void
ctk_style_context_set_state (CtkStyleContext *context,
                             CtkStateFlags    flags)
{
  CtkStateFlags old_flags;

  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));

  old_flags = ctk_css_node_get_state (context->priv->cssnode);

  ctk_css_node_set_state (context->priv->cssnode, flags);

  if (((old_flags ^ flags) & (CTK_STATE_FLAG_DIR_LTR | CTK_STATE_FLAG_DIR_RTL)) &&
      !ctk_style_context_is_saved (context))
    g_object_notify_by_pspec (G_OBJECT (context), properties[PROP_DIRECTION]);
}

/**
 * ctk_style_context_get_state:
 * @context: a #CtkStyleContext
 *
 * Returns the state used for style matching.
 *
 * This method should only be used to retrieve the #CtkStateFlags
 * to pass to #CtkStyleContext methods, like ctk_style_context_get_padding().
 * If you need to retrieve the current state of a #CtkWidget, use
 * ctk_widget_get_state_flags().
 *
 * Returns: the state flags
 *
 * Since: 3.0
 **/
CtkStateFlags
ctk_style_context_get_state (CtkStyleContext *context)
{
  g_return_val_if_fail (CTK_IS_STYLE_CONTEXT (context), 0);

  return ctk_css_node_get_state (context->priv->cssnode);
}

/**
 * ctk_style_context_set_scale:
 * @context: a #CtkStyleContext
 * @scale: scale
 *
 * Sets the scale to use when getting image assets for the style.
 *
 * Since: 3.10
 **/
void
ctk_style_context_set_scale (CtkStyleContext *context,
                             gint             scale)
{
  CtkStyleContextPrivate *priv;

  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));

  priv = context->priv;

  if (scale == _ctk_style_cascade_get_scale (priv->cascade))
    return;

  if (ctk_style_context_has_custom_cascade (context))
    {
      _ctk_style_cascade_set_scale (priv->cascade, scale);
    }
  else
    {
      CtkStyleCascade *new_cascade;

      new_cascade = _ctk_settings_get_style_cascade (ctk_settings_get_for_screen (priv->screen),
                                                     scale);
      ctk_style_context_set_cascade (context, new_cascade);
    }
}

/**
 * ctk_style_context_get_scale:
 * @context: a #CtkStyleContext
 *
 * Returns the scale used for assets.
 *
 * Returns: the scale
 *
 * Since: 3.10
 **/
gint
ctk_style_context_get_scale (CtkStyleContext *context)
{
  g_return_val_if_fail (CTK_IS_STYLE_CONTEXT (context), 0);

  return _ctk_style_cascade_get_scale (context->priv->cascade);
}

/**
 * ctk_style_context_state_is_running:
 * @context: a #CtkStyleContext
 * @state: a widget state
 * @progress: (out): return location for the transition progress
 *
 * Returns %TRUE if there is a transition animation running for the
 * current region (see ctk_style_context_push_animatable_region()).
 *
 * If @progress is not %NULL, the animation progress will be returned
 * there, 0.0 means the state is closest to being unset, while 1.0 means
 * it’s closest to being set. This means transition animation will
 * run from 0 to 1 when @state is being set and from 1 to 0 when
 * it’s being unset.
 *
 * Returns: %TRUE if there is a running transition animation for @state.
 *
 * Since: 3.0
 *
 * Deprecated: 3.6: This function always returns %FALSE
 **/
gboolean
ctk_style_context_state_is_running (CtkStyleContext *context,
                                    CtkStateType     state,
                                    gdouble         *progress)
{
  g_return_val_if_fail (CTK_IS_STYLE_CONTEXT (context), FALSE);

  return FALSE;
}

/**
 * ctk_style_context_set_path:
 * @context: a #CtkStyleContext
 * @path: a #CtkWidgetPath
 *
 * Sets the #CtkWidgetPath used for style matching. As a
 * consequence, the style will be regenerated to match
 * the new given path.
 *
 * If you are using a #CtkStyleContext returned from
 * ctk_widget_get_style_context(), you do not need to call
 * this yourself.
 *
 * Since: 3.0
 **/
void
ctk_style_context_set_path (CtkStyleContext *context,
                            CtkWidgetPath   *path)
{
  CtkCssNode *root;

  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
  g_return_if_fail (path != NULL);

  root = ctk_style_context_get_root (context);
  g_return_if_fail (CTK_IS_CSS_PATH_NODE (root));

  if (path && ctk_widget_path_length (path) > 0)
    {
      CtkWidgetPath *copy = ctk_widget_path_copy (path);
      ctk_css_path_node_set_widget_path (CTK_CSS_PATH_NODE (root), copy);
      ctk_css_node_set_widget_type (root,
                                    ctk_widget_path_iter_get_object_type (copy, -1));
      ctk_css_node_set_name (root, ctk_widget_path_iter_get_object_name (copy, -1));
      ctk_widget_path_unref (copy);
    }
  else
    {
      ctk_css_path_node_set_widget_path (CTK_CSS_PATH_NODE (root), NULL);
      ctk_css_node_set_widget_type (root, G_TYPE_NONE);
      ctk_css_node_set_name (root, NULL);
    }
}

/**
 * ctk_style_context_get_path:
 * @context: a #CtkStyleContext
 *
 * Returns the widget path used for style matching.
 *
 * Returns: (transfer none): A #CtkWidgetPath
 *
 * Since: 3.0
 **/
const CtkWidgetPath *
ctk_style_context_get_path (CtkStyleContext *context)
{
  return ctk_css_node_get_widget_path (ctk_style_context_get_root (context));
}

/**
 * ctk_style_context_set_parent:
 * @context: a #CtkStyleContext
 * @parent: (allow-none): the new parent or %NULL
 *
 * Sets the parent style context for @context. The parent style
 * context is used to implement
 * [inheritance](http://www.w3.org/TR/css3-cascade/#inheritance)
 * of properties.
 *
 * If you are using a #CtkStyleContext returned from
 * ctk_widget_get_style_context(), the parent will be set for you.
 *
 * Since: 3.4
 **/
void
ctk_style_context_set_parent (CtkStyleContext *context,
                              CtkStyleContext *parent)
{
  CtkStyleContextPrivate *priv;

  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
  g_return_if_fail (parent == NULL || CTK_IS_STYLE_CONTEXT (parent));

  priv = context->priv;

  if (priv->parent == parent)
    return;

  if (parent)
    {
      CtkCssNode *root = ctk_style_context_get_root (context);
      g_object_ref (parent);

      if (ctk_css_node_get_parent (root) == NULL)
        ctk_css_node_set_parent (root, ctk_style_context_get_root (parent));
    }
  else
    {
      ctk_css_node_set_parent (ctk_style_context_get_root (context), NULL);
    }

  ctk_style_context_clear_parent (context);

  priv->parent = parent;

  g_object_notify_by_pspec (G_OBJECT (context), properties[PROP_PARENT]);
  ctk_css_node_invalidate (ctk_style_context_get_root (context), CTK_CSS_CHANGE_ANY_PARENT | CTK_CSS_CHANGE_ANY_SIBLING);
}

/**
 * ctk_style_context_get_parent:
 * @context: a #CtkStyleContext
 *
 * Gets the parent context set via ctk_style_context_set_parent().
 * See that function for details.
 *
 * Returns: (nullable) (transfer none): the parent context or %NULL
 *
 * Since: 3.4
 **/
CtkStyleContext *
ctk_style_context_get_parent (CtkStyleContext *context)
{
  g_return_val_if_fail (CTK_IS_STYLE_CONTEXT (context), NULL);

  return context->priv->parent;
}

/*
 * ctk_style_context_save_to_node:
 * @context: a #CtkStyleContext
 * @node: the node to save to
 *
 * Saves the @context state, so temporary modifications done through
 * ctk_style_context_add_class(), ctk_style_context_remove_class(),
 * ctk_style_context_set_state(), etc. and rendering using
 * ctk_render_background() or similar functions are done using the
 * given @node.
 *
 * To undo, call ctk_style_context_restore().
 *
 * The matching call to ctk_style_context_restore() must be done
 * before CTK returns to the main loop.
 **/
void
ctk_style_context_save_to_node (CtkStyleContext *context,
                                CtkCssNode      *node)
{
  CtkStyleContextPrivate *priv;

  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
  g_return_if_fail (CTK_IS_CSS_NODE (node));

  priv = context->priv;

  priv->saved_nodes = g_slist_prepend (priv->saved_nodes, priv->cssnode);
  priv->cssnode = g_object_ref (node);
}

void
ctk_style_context_save_named (CtkStyleContext *context,
                              const char      *name)
{
  CtkStyleContextPrivate *priv;
  CtkCssNode *cssnode;

  priv = context->priv;

  /* Make sure we have the style existing. It is the
   * parent of the new saved node after all.
   */
  if (!ctk_style_context_is_saved (context))
    ctk_style_context_lookup_style (context);

  cssnode = ctk_css_transient_node_new (priv->cssnode);
  ctk_css_node_set_parent (cssnode, ctk_style_context_get_root (context));
  if (name)
    ctk_css_node_set_name (cssnode, g_intern_string (name));

  ctk_style_context_save_to_node (context, cssnode);

  g_object_unref (cssnode);
}

/**
 * ctk_style_context_save:
 * @context: a #CtkStyleContext
 *
 * Saves the @context state, so temporary modifications done through
 * ctk_style_context_add_class(), ctk_style_context_remove_class(),
 * ctk_style_context_set_state(), etc. can quickly be reverted
 * in one go through ctk_style_context_restore().
 *
 * The matching call to ctk_style_context_restore() must be done
 * before CTK returns to the main loop.
 *
 * Since: 3.0
 **/
void
ctk_style_context_save (CtkStyleContext *context)
{
  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));

  ctk_style_context_save_named (context, NULL);
}

/**
 * ctk_style_context_restore:
 * @context: a #CtkStyleContext
 *
 * Restores @context state to a previous stage.
 * See ctk_style_context_save().
 *
 * Since: 3.0
 **/
void
ctk_style_context_restore (CtkStyleContext *context)
{
  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));

  if (context->priv->saved_nodes == NULL)
    {
      g_warning ("Unpaired ctk_style_context_restore() call");
      return;
    }

  ctk_style_context_pop_style_node (context);
}

/**
 * ctk_style_context_add_class:
 * @context: a #CtkStyleContext
 * @class_name: class name to use in styling
 *
 * Adds a style class to @context, so posterior calls to
 * ctk_style_context_get() or any of the ctk_render_*()
 * functions will make use of this new class for styling.
 *
 * In the CSS file format, a #CtkEntry defining a “search”
 * class, would be matched by:
 *
 * |[ <!-- language="CSS" -->
 * entry.search { ... }
 * ]|
 *
 * While any widget defining a “search” class would be
 * matched by:
 * |[ <!-- language="CSS" -->
 * .search { ... }
 * ]|
 *
 * Since: 3.0
 **/
void
ctk_style_context_add_class (CtkStyleContext *context,
                             const gchar     *class_name)
{
  GQuark class_quark;

  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
  g_return_if_fail (class_name != NULL);

  class_quark = g_quark_from_string (class_name);

  ctk_css_node_add_class (context->priv->cssnode, class_quark);
}

/**
 * ctk_style_context_remove_class:
 * @context: a #CtkStyleContext
 * @class_name: class name to remove
 *
 * Removes @class_name from @context.
 *
 * Since: 3.0
 **/
void
ctk_style_context_remove_class (CtkStyleContext *context,
                                const gchar     *class_name)
{
  GQuark class_quark;

  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
  g_return_if_fail (class_name != NULL);

  class_quark = g_quark_try_string (class_name);
  if (!class_quark)
    return;

  ctk_css_node_remove_class (context->priv->cssnode, class_quark);
}

/**
 * ctk_style_context_has_class:
 * @context: a #CtkStyleContext
 * @class_name: a class name
 *
 * Returns %TRUE if @context currently has defined the
 * given class name.
 *
 * Returns: %TRUE if @context has @class_name defined
 *
 * Since: 3.0
 **/
gboolean
ctk_style_context_has_class (CtkStyleContext *context,
                             const gchar     *class_name)
{
  GQuark class_quark;

  g_return_val_if_fail (CTK_IS_STYLE_CONTEXT (context), FALSE);
  g_return_val_if_fail (class_name != NULL, FALSE);

  class_quark = g_quark_try_string (class_name);
  if (!class_quark)
    return FALSE;

  return ctk_css_node_has_class (context->priv->cssnode, class_quark);
}

/**
 * ctk_style_context_list_classes:
 * @context: a #CtkStyleContext
 *
 * Returns the list of classes currently defined in @context.
 *
 * Returns: (transfer container) (element-type utf8): a #GList of
 *          strings with the currently defined classes. The contents
 *          of the list are owned by CTK+, but you must free the list
 *          itself with g_list_free() when you are done with it.
 *
 * Since: 3.0
 **/
GList *
ctk_style_context_list_classes (CtkStyleContext *context)
{
  GList *classes_list = NULL;
  const GQuark *classes;
  guint n_classes, i;

  g_return_val_if_fail (CTK_IS_STYLE_CONTEXT (context), NULL);

  classes = ctk_css_node_list_classes (context->priv->cssnode, &n_classes);
  for (i = n_classes; i > 0; i--)
    classes_list = g_list_prepend (classes_list, (gchar *)g_quark_to_string (classes[i - 1]));

  return classes_list;
}

/**
 * ctk_style_context_list_regions:
 * @context: a #CtkStyleContext
 *
 * Returns the list of regions currently defined in @context.
 *
 * Returns: (transfer container) (element-type utf8): a #GList of
 *          strings with the currently defined regions. The contents
 *          of the list are owned by CTK+, but you must free the list
 *          itself with g_list_free() when you are done with it.
 *
 * Since: 3.0
 *
 * Deprecated: 3.14
 **/
GList *
ctk_style_context_list_regions (CtkStyleContext *context)
{
  GList *regions, *l;

  g_return_val_if_fail (CTK_IS_STYLE_CONTEXT (context), NULL);

  regions = ctk_css_node_list_regions (context->priv->cssnode);
  for (l = regions; l; l = l->next)
    l->data = (char *) g_quark_to_string (GPOINTER_TO_UINT (l->data));

  return regions;
}

gboolean
_ctk_style_context_check_region_name (const gchar *str)
{
  g_return_val_if_fail (str != NULL, FALSE);

  if (!g_ascii_islower (str[0]))
    return FALSE;

  while (*str)
    {
      if (*str != '-' &&
          !g_ascii_islower (*str))
        return FALSE;

      str++;
    }

  return TRUE;
}

/**
 * ctk_style_context_add_region:
 * @context: a #CtkStyleContext
 * @region_name: region name to use in styling
 * @flags: flags that apply to the region
 *
 * Adds a region to @context, so posterior calls to
 * ctk_style_context_get() or any of the ctk_render_*()
 * functions will make use of this new region for styling.
 *
 * In the CSS file format, a #CtkTreeView defining a “row”
 * region, would be matched by:
 *
 * |[ <!-- language="CSS" -->
 * treeview row { ... }
 * ]|
 *
 * Pseudo-classes are used for matching @flags, so the two
 * following rules:
 * |[ <!-- language="CSS" -->
 * treeview row:nth-child(even) { ... }
 * treeview row:nth-child(odd) { ... }
 * ]|
 *
 * would apply to even and odd rows, respectively.
 *
 * Region names must only contain lowercase letters
 * and “-”, starting always with a lowercase letter.
 *
 * Since: 3.0
 *
 * Deprecated: 3.14
 **/
void
ctk_style_context_add_region (CtkStyleContext *context,
                              const gchar     *region_name,
                              CtkRegionFlags   flags)
{
  GQuark region_quark;

  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
  g_return_if_fail (region_name != NULL);
  g_return_if_fail (_ctk_style_context_check_region_name (region_name));

  region_quark = g_quark_from_string (region_name);

  ctk_css_node_add_region (context->priv->cssnode, region_quark, flags);
}

/**
 * ctk_style_context_remove_region:
 * @context: a #CtkStyleContext
 * @region_name: region name to unset
 *
 * Removes a region from @context.
 *
 * Since: 3.0
 *
 * Deprecated: 3.14
 **/
void
ctk_style_context_remove_region (CtkStyleContext *context,
                                 const gchar     *region_name)
{
  GQuark region_quark;

  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
  g_return_if_fail (region_name != NULL);

  region_quark = g_quark_try_string (region_name);
  if (!region_quark)
    return;

  ctk_css_node_remove_region (context->priv->cssnode, region_quark);
}

/**
 * ctk_style_context_has_region:
 * @context: a #CtkStyleContext
 * @region_name: a region name
 * @flags_return: (out) (allow-none): return location for region flags
 *
 * Returns %TRUE if @context has the region defined.
 * If @flags_return is not %NULL, it is set to the flags
 * affecting the region.
 *
 * Returns: %TRUE if region is defined
 *
 * Since: 3.0
 *
 * Deprecated: 3.14
 **/
gboolean
ctk_style_context_has_region (CtkStyleContext *context,
                              const gchar     *region_name,
                              CtkRegionFlags  *flags_return)
{
  GQuark region_quark;

  g_return_val_if_fail (CTK_IS_STYLE_CONTEXT (context), FALSE);
  g_return_val_if_fail (region_name != NULL, FALSE);

  if (flags_return)
    *flags_return = 0;

  region_quark = g_quark_try_string (region_name);
  if (!region_quark)
    return FALSE;

  return ctk_css_node_has_region (context->priv->cssnode, region_quark, flags_return);
}

static gint
style_property_values_cmp (gconstpointer bsearch_node1,
                           gconstpointer bsearch_node2)
{
  const PropertyValue *val1 = bsearch_node1;
  const PropertyValue *val2 = bsearch_node2;

  if (val1->widget_type != val2->widget_type)
    return val1->widget_type < val2->widget_type ? -1 : 1;

  if (val1->pspec != val2->pspec)
    return val1->pspec < val2->pspec ? -1 : 1;

  return 0;
}

CtkCssValue *
_ctk_style_context_peek_property (CtkStyleContext *context,
                                  guint            property_id)
{
  CtkCssStyle *values = ctk_style_context_lookup_style (context);

  return ctk_css_style_get_value (values, property_id);
}

const GValue *
_ctk_style_context_peek_style_property (CtkStyleContext *context,
                                        GType            widget_type,
                                        GParamSpec      *pspec)
{
  CtkStyleContextPrivate *priv;
  CtkWidgetPath *path;
  PropertyValue *pcache, key = { 0 };
  guint i;

  priv = context->priv;

  /* ensure the style cache is valid by forcing a validation */
  ctk_style_context_lookup_style (context);

  key.widget_type = widget_type;
  key.pspec = pspec;

  /* need value cache array */
  pcache = bsearch (&key,
                    priv->property_cache->data, priv->property_cache->len,
                    sizeof (PropertyValue), style_property_values_cmp);
  if (pcache)
    return &pcache->value;

  i = 0;
  while (i < priv->property_cache->len &&
         style_property_values_cmp (&key, &g_array_index (priv->property_cache, PropertyValue, i)) >= 0)
    i++;

  g_array_insert_val (priv->property_cache, i, key);
  pcache = &g_array_index (priv->property_cache, PropertyValue, i);

  /* cache miss, initialize value type, then set contents */
  g_param_spec_ref (pcache->pspec);
  g_value_init (&pcache->value, G_PARAM_SPEC_VALUE_TYPE (pspec));

  path = ctk_css_node_create_widget_path (ctk_style_context_get_root (context));
  if (path && ctk_widget_path_length (path) > 0)
    {
      if (ctk_style_provider_get_style_property (CTK_STYLE_PROVIDER (priv->cascade),
                                                 path,
                                                 ctk_widget_path_iter_get_state (path, -1),
                                                 pspec, &pcache->value))
        {
          G_GNUC_BEGIN_IGNORE_DEPRECATIONS;

          /* Resolve symbolic colors to CdkColor/CdkRGBA */
          if (G_VALUE_TYPE (&pcache->value) == CTK_TYPE_SYMBOLIC_COLOR)
            {
              CtkSymbolicColor *color;
              CdkRGBA rgba;

              color = g_value_dup_boxed (&pcache->value);

              g_value_unset (&pcache->value);

              if (G_PARAM_SPEC_VALUE_TYPE (pspec) == CDK_TYPE_RGBA)
                g_value_init (&pcache->value, CDK_TYPE_RGBA);
              else
                g_value_init (&pcache->value, CDK_TYPE_COLOR);

              if (_ctk_style_context_resolve_color (context, _ctk_symbolic_color_get_css_value (color), &rgba))
                {
                  if (G_PARAM_SPEC_VALUE_TYPE (pspec) == CDK_TYPE_RGBA)
                    g_value_set_boxed (&pcache->value, &rgba);
                  else
                    {
                      CdkColor rgb;

                      rgb.red = rgba.red * 65535. + 0.5;
                      rgb.green = rgba.green * 65535. + 0.5;
                      rgb.blue = rgba.blue * 65535. + 0.5;

                      g_value_set_boxed (&pcache->value, &rgb);
                    }
                }
              else
                g_param_value_set_default (pspec, &pcache->value);

              ctk_symbolic_color_unref (color);
            }

          G_GNUC_END_IGNORE_DEPRECATIONS;

          ctk_widget_path_unref (path);

          return &pcache->value;
        }
    }

  ctk_widget_path_unref (path);

  /* not supplied by any provider, revert to default */
  g_param_value_set_default (pspec, &pcache->value);

  return &pcache->value;
}

/**
 * ctk_style_context_get_style_property:
 * @context: a #CtkStyleContext
 * @property_name: the name of the widget style property
 * @value: Return location for the property value
 *
 * Gets the value for a widget style property.
 *
 * When @value is no longer needed, g_value_unset() must be called
 * to free any allocated memory.
 **/
void
ctk_style_context_get_style_property (CtkStyleContext *context,
                                      const gchar     *property_name,
                                      GValue          *value)
{
  CtkCssNode *root;
  CtkWidgetClass *widget_class;
  GParamSpec *pspec;
  const GValue *peek_value;
  GType widget_type;

  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
  g_return_if_fail (property_name != NULL);
  g_return_if_fail (value != NULL);

  root = ctk_style_context_get_root (context);

  if (CTK_IS_CSS_WIDGET_NODE (root))
    {
      CtkWidget *widget;

      widget = ctk_css_widget_node_get_widget (CTK_CSS_WIDGET_NODE (root));
      if (widget == NULL)
        return;

      widget_type = G_OBJECT_TYPE (widget);
    }
  else if (CTK_IS_CSS_PATH_NODE (root))
    {
      CtkWidgetPath *path;

      path = ctk_css_path_node_get_widget_path (CTK_CSS_PATH_NODE (root));
      if (path == NULL)
        return;

      widget_type = ctk_widget_path_get_object_type (path);

      if (!g_type_is_a (widget_type, CTK_TYPE_WIDGET))
        {
          g_warning ("%s: can't get style properties for non-widget class '%s'",
                     G_STRLOC,
                     g_type_name (widget_type));
          return;
        }
    }
  else
    {
      return;
    }

  widget_class = g_type_class_ref (widget_type);
  pspec = ctk_widget_class_find_style_property (widget_class, property_name);
  g_type_class_unref (widget_class);

  if (!pspec)
    {
      g_warning ("%s: widget class '%s' has no style property named '%s'",
                 G_STRLOC,
                 g_type_name (widget_type),
                 property_name);
      return;
    }

  peek_value = _ctk_style_context_peek_style_property (context, widget_type, pspec);

  if (G_VALUE_TYPE (value) == G_VALUE_TYPE (peek_value))
    g_value_copy (peek_value, value);
  else if (g_value_type_transformable (G_VALUE_TYPE (peek_value), G_VALUE_TYPE (value)))
    g_value_transform (peek_value, value);
  else
    g_warning ("can't retrieve style property '%s' of type '%s' as value of type '%s'",
               pspec->name,
               G_VALUE_TYPE_NAME (peek_value),
               G_VALUE_TYPE_NAME (value));
}

/**
 * ctk_style_context_get_style_valist:
 * @context: a #CtkStyleContext
 * @args: va_list of property name/return location pairs, followed by %NULL
 *
 * Retrieves several widget style properties from @context according to the
 * current style.
 *
 * Since: 3.0
 **/
void
ctk_style_context_get_style_valist (CtkStyleContext *context,
                                    va_list          args)
{
  CtkCssNode *root;
  const gchar *prop_name;
  GType widget_type;

  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));

  prop_name = va_arg (args, const gchar *);
  root = ctk_style_context_get_root (context);

  if (CTK_IS_CSS_WIDGET_NODE (root))
    {
      CtkWidget *widget;

      widget = ctk_css_widget_node_get_widget (CTK_CSS_WIDGET_NODE (root));
      if (widget == NULL)
        return;

      widget_type = G_OBJECT_TYPE (widget);
    }
  else if (CTK_IS_CSS_PATH_NODE (root))
    {
      CtkWidgetPath *path;

      path = ctk_css_path_node_get_widget_path (CTK_CSS_PATH_NODE (root));
      if (path == NULL)
        return;

      widget_type = ctk_widget_path_get_object_type (path);

      if (!g_type_is_a (widget_type, CTK_TYPE_WIDGET))
        {
          g_warning ("%s: can't get style properties for non-widget class '%s'",
                     G_STRLOC,
                     g_type_name (widget_type));
          return;
        }
    }
  else
    {
      return;
    }

  while (prop_name)
    {
      CtkWidgetClass *widget_class;
      GParamSpec *pspec;
      const GValue *peek_value;
      gchar *error;

      widget_class = g_type_class_ref (widget_type);
      pspec = ctk_widget_class_find_style_property (widget_class, prop_name);
      g_type_class_unref (widget_class);

      if (!pspec)
        {
          g_warning ("%s: widget class '%s' has no style property named '%s'",
                     G_STRLOC,
                     g_type_name (widget_type),
                     prop_name);
          break;
        }

      peek_value = _ctk_style_context_peek_style_property (context, widget_type, pspec);

      G_VALUE_LCOPY (peek_value, args, 0, &error);

      if (error)
        {
          g_warning ("can't retrieve style property '%s' of type '%s': %s",
                     pspec->name,
                     G_VALUE_TYPE_NAME (peek_value),
                     error);
          g_free (error);
          break;
        }

      prop_name = va_arg (args, const gchar *);
    }
}

/**
 * ctk_style_context_get_style:
 * @context: a #CtkStyleContext
 * @...: property name /return value pairs, followed by %NULL
 *
 * Retrieves several widget style properties from @context according to the
 * current style.
 *
 * Since: 3.0
 **/
void
ctk_style_context_get_style (CtkStyleContext *context,
                             ...)
{
  va_list args;

  va_start (args, context);
  ctk_style_context_get_style_valist (context, args);
  va_end (args);
}


/**
 * ctk_style_context_lookup_icon_set:
 * @context: a #CtkStyleContext
 * @stock_id: an icon name
 *
 * Looks up @stock_id in the icon factories associated to @context and
 * the default icon factory, returning an icon set if found, otherwise
 * %NULL.
 *
 * Returns: (nullable) (transfer none): The looked up %CtkIconSet, or %NULL
 *
 * Deprecated: 3.10: Use ctk_icon_theme_lookup_icon() instead.
 **/
CtkIconSet *
ctk_style_context_lookup_icon_set (CtkStyleContext *context,
                                   const gchar     *stock_id)
{
  CtkIconSet *icon_set;

  g_return_val_if_fail (CTK_IS_STYLE_CONTEXT (context), NULL);
  g_return_val_if_fail (stock_id != NULL, NULL);

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;

  icon_set = ctk_icon_factory_lookup_default (stock_id);

  G_GNUC_END_IGNORE_DEPRECATIONS;

  return icon_set;
}

/**
 * ctk_style_context_set_screen:
 * @context: a #CtkStyleContext
 * @screen: a #CdkScreen
 *
 * Attaches @context to the given screen.
 *
 * The screen is used to add style information from “global” style
 * providers, such as the screen’s #CtkSettings instance.
 *
 * If you are using a #CtkStyleContext returned from
 * ctk_widget_get_style_context(), you do not need to
 * call this yourself.
 *
 * Since: 3.0
 **/
void
ctk_style_context_set_screen (CtkStyleContext *context,
                              CdkScreen       *screen)
{
  CtkStyleContextPrivate *priv;
  CtkStyleCascade *screen_cascade;

  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
  g_return_if_fail (CDK_IS_SCREEN (screen));

  priv = context->priv;
  if (priv->screen == screen)
    return;

  if (ctk_style_context_has_custom_cascade (context))
    {
      screen_cascade = _ctk_settings_get_style_cascade (ctk_settings_get_for_screen (screen), 1);
      _ctk_style_cascade_set_parent (priv->cascade, screen_cascade);
    }
  else
    {
      screen_cascade = _ctk_settings_get_style_cascade (ctk_settings_get_for_screen (screen),
                                                        _ctk_style_cascade_get_scale (priv->cascade));
      ctk_style_context_set_cascade (context, screen_cascade);
    }

  priv->screen = screen;

  g_object_notify_by_pspec (G_OBJECT (context), properties[PROP_SCREEN]);
}

/**
 * ctk_style_context_get_screen:
 * @context: a #CtkStyleContext
 *
 * Returns the #CdkScreen to which @context is attached.
 *
 * Returns: (transfer none): a #CdkScreen.
 **/
CdkScreen *
ctk_style_context_get_screen (CtkStyleContext *context)
{
  g_return_val_if_fail (CTK_IS_STYLE_CONTEXT (context), NULL);

  return context->priv->screen;
}

/**
 * ctk_style_context_set_frame_clock:
 * @context: a #CdkFrameClock
 * @frame_clock: a #CdkFrameClock
 *
 * Attaches @context to the given frame clock.
 *
 * The frame clock is used for the timing of animations.
 *
 * If you are using a #CtkStyleContext returned from
 * ctk_widget_get_style_context(), you do not need to
 * call this yourself.
 *
 * Since: 3.8
 **/
void
ctk_style_context_set_frame_clock (CtkStyleContext *context,
                                   CdkFrameClock   *frame_clock)
{
  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
  g_return_if_fail (frame_clock == NULL || CDK_IS_FRAME_CLOCK (frame_clock));

  if (g_set_object (&context->priv->frame_clock, frame_clock))
    g_object_notify_by_pspec (G_OBJECT (context), properties[PROP_FRAME_CLOCK]);
}

/**
 * ctk_style_context_get_frame_clock:
 * @context: a #CtkStyleContext
 *
 * Returns the #CdkFrameClock to which @context is attached.
 *
 * Returns: (nullable) (transfer none): a #CdkFrameClock, or %NULL
 *  if @context does not have an attached frame clock.
 *
 * Since: 3.8
 **/
CdkFrameClock *
ctk_style_context_get_frame_clock (CtkStyleContext *context)
{
  g_return_val_if_fail (CTK_IS_STYLE_CONTEXT (context), NULL);

  return context->priv->frame_clock;
}

/**
 * ctk_style_context_set_direction:
 * @context: a #CtkStyleContext
 * @direction: the new direction.
 *
 * Sets the reading direction for rendering purposes.
 *
 * If you are using a #CtkStyleContext returned from
 * ctk_widget_get_style_context(), you do not need to
 * call this yourself.
 *
 * Since: 3.0
 *
 * Deprecated: 3.8: Use ctk_style_context_set_state() with
 *   #CTK_STATE_FLAG_DIR_LTR and #CTK_STATE_FLAG_DIR_RTL
 *   instead.
 **/
void
ctk_style_context_set_direction (CtkStyleContext  *context,
                                 CtkTextDirection  direction)
{
  CtkStateFlags state;

  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));

  state = ctk_style_context_get_state (context);
  state &= ~(CTK_STATE_FLAG_DIR_LTR | CTK_STATE_FLAG_DIR_RTL);

  switch (direction)
    {
    case CTK_TEXT_DIR_LTR:
      state |= CTK_STATE_FLAG_DIR_LTR;
      break;

    case CTK_TEXT_DIR_RTL:
      state |= CTK_STATE_FLAG_DIR_RTL;
      break;

    case CTK_TEXT_DIR_NONE:
    default:
      break;
    }

  ctk_style_context_set_state (context, state);
}

/**
 * ctk_style_context_get_direction:
 * @context: a #CtkStyleContext
 *
 * Returns the widget direction used for rendering.
 *
 * Returns: the widget direction
 *
 * Since: 3.0
 *
 * Deprecated: 3.8: Use ctk_style_context_get_state() and
 *   check for #CTK_STATE_FLAG_DIR_LTR and
 *   #CTK_STATE_FLAG_DIR_RTL instead.
 **/
CtkTextDirection
ctk_style_context_get_direction (CtkStyleContext *context)
{
  CtkStateFlags state;

  g_return_val_if_fail (CTK_IS_STYLE_CONTEXT (context), CTK_TEXT_DIR_LTR);

  state = ctk_style_context_get_state (context);

  if (state & CTK_STATE_FLAG_DIR_LTR)
    return CTK_TEXT_DIR_LTR;
  else if (state & CTK_STATE_FLAG_DIR_RTL)
    return CTK_TEXT_DIR_RTL;
  else
    return CTK_TEXT_DIR_NONE;
}

/**
 * ctk_style_context_set_junction_sides:
 * @context: a #CtkStyleContext
 * @sides: sides where rendered elements are visually connected to
 *     other elements
 *
 * Sets the sides where rendered elements (mostly through
 * ctk_render_frame()) will visually connect with other visual elements.
 *
 * This is merely a hint that may or may not be honored
 * by themes.
 *
 * Container widgets are expected to set junction hints as appropriate
 * for their children, so it should not normally be necessary to call
 * this function manually.
 *
 * Since: 3.0
 **/
void
ctk_style_context_set_junction_sides (CtkStyleContext  *context,
                                      CtkJunctionSides  sides)
{
  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));

  ctk_css_node_set_junction_sides (context->priv->cssnode, sides);
}

/**
 * ctk_style_context_get_junction_sides:
 * @context: a #CtkStyleContext
 *
 * Returns the sides where rendered elements connect visually with others.
 *
 * Returns: the junction sides
 *
 * Since: 3.0
 **/
CtkJunctionSides
ctk_style_context_get_junction_sides (CtkStyleContext *context)
{
  g_return_val_if_fail (CTK_IS_STYLE_CONTEXT (context), 0);

  return ctk_css_node_get_junction_sides (context->priv->cssnode);
}

gboolean
_ctk_style_context_resolve_color (CtkStyleContext    *context,
                                  CtkCssValue        *color,
                                  CdkRGBA            *result)
{
  CtkCssValue *val;

  g_return_val_if_fail (CTK_IS_STYLE_CONTEXT (context), FALSE);
  g_return_val_if_fail (color != NULL, FALSE);
  g_return_val_if_fail (result != NULL, FALSE);

  val = _ctk_css_color_value_resolve (color,
                                      CTK_STYLE_PROVIDER_PRIVATE (context->priv->cascade),
                                      _ctk_style_context_peek_property (context, CTK_CSS_PROPERTY_COLOR),
                                      NULL);
  if (val == NULL)
    return FALSE;

  *result = *_ctk_css_rgba_value_get_rgba (val);
  _ctk_css_value_unref (val);
  return TRUE;
}

/**
 * ctk_style_context_lookup_color:
 * @context: a #CtkStyleContext
 * @color_name: color name to lookup
 * @color: (out): Return location for the looked up color
 *
 * Looks up and resolves a color name in the @context color map.
 *
 * Returns: %TRUE if @color_name was found and resolved, %FALSE otherwise
 **/
gboolean
ctk_style_context_lookup_color (CtkStyleContext *context,
                                const gchar     *color_name,
                                CdkRGBA         *color)
{
  CtkCssValue *value;

  g_return_val_if_fail (CTK_IS_STYLE_CONTEXT (context), FALSE);
  g_return_val_if_fail (color_name != NULL, FALSE);
  g_return_val_if_fail (color != NULL, FALSE);

  value = _ctk_style_provider_private_get_color (CTK_STYLE_PROVIDER_PRIVATE (context->priv->cascade), color_name);
  if (value == NULL)
    return FALSE;

  return _ctk_style_context_resolve_color (context, value, color);
}

/**
 * ctk_style_context_notify_state_change:
 * @context: a #CtkStyleContext
 * @window: a #CdkWindow
 * @region_id: (allow-none): animatable region to notify on, or %NULL.
 *     See ctk_style_context_push_animatable_region()
 * @state: state to trigger transition for
 * @state_value: %TRUE if @state is the state we are changing to,
 *     %FALSE if we are changing away from it
 *
 * Notifies a state change on @context, so if the current style makes use
 * of transition animations, one will be started so all rendered elements
 * under @region_id are animated for state @state being set to value
 * @state_value.
 *
 * The @window parameter is used in order to invalidate the rendered area
 * as the animation runs, so make sure it is the same window that is being
 * rendered on by the ctk_render_*() functions.
 *
 * If @region_id is %NULL, all rendered elements using @context will be
 * affected by this state transition.
 *
 * As a practical example, a #CtkButton notifying a state transition on
 * the prelight state:
 * |[ <!-- language="C" -->
 * ctk_style_context_notify_state_change (context,
 *                                        ctk_widget_get_window (widget),
 *                                        NULL,
 *                                        CTK_STATE_PRELIGHT,
 *                                        button->in_button);
 * ]|
 *
 * Can be handled in the CSS file like this:
 * |[ <!-- language="CSS" -->
 * button {
 *     background-color: #f00
 * }
 *
 * button:hover {
 *     background-color: #fff;
 *     transition: 200ms linear
 * }
 * ]|
 *
 * This combination will animate the button background from red to white
 * if a pointer enters the button, and back to red if the pointer leaves
 * the button.
 *
 * Note that @state is used when finding the transition parameters, which
 * is why the style places the transition under the :hover pseudo-class.
 *
 * Since: 3.0
 *
 * Deprecated: 3.6: This function does nothing.
 **/
void
ctk_style_context_notify_state_change (CtkStyleContext *context,
                                       CdkWindow       *window,
                                       gpointer         region_id,
                                       CtkStateType     state,
                                       gboolean         state_value)
{
  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
  g_return_if_fail (CDK_IS_WINDOW (window));
  g_return_if_fail (state > CTK_STATE_NORMAL && state <= CTK_STATE_FOCUSED);
}

/**
 * ctk_style_context_cancel_animations:
 * @context: a #CtkStyleContext
 * @region_id: (allow-none): animatable region to stop, or %NULL.
 *     See ctk_style_context_push_animatable_region()
 *
 * Stops all running animations for @region_id and all animatable
 * regions underneath.
 *
 * A %NULL @region_id will stop all ongoing animations in @context,
 * when dealing with a #CtkStyleContext obtained through
 * ctk_widget_get_style_context(), this is normally done for you
 * in all circumstances you would expect all widget to be stopped,
 * so this should be only used in complex widgets with different
 * animatable regions.
 *
 * Since: 3.0
 *
 * Deprecated: 3.6: This function does nothing.
 **/
void
ctk_style_context_cancel_animations (CtkStyleContext *context,
                                     gpointer         region_id)
{
  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
}

/**
 * ctk_style_context_scroll_animations:
 * @context: a #CtkStyleContext
 * @window: a #CdkWindow used previously in
 *          ctk_style_context_notify_state_change()
 * @dx: Amount to scroll in the X axis
 * @dy: Amount to scroll in the Y axis
 *
 * This function is analogous to cdk_window_scroll(), and
 * should be called together with it so the invalidation
 * areas for any ongoing animation are scrolled together
 * with it.
 *
 * Since: 3.0
 *
 * Deprecated: 3.6: This function does nothing.
 **/
void
ctk_style_context_scroll_animations (CtkStyleContext *context,
                                     CdkWindow       *window,
                                     gint             dx,
                                     gint             dy)
{
  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
  g_return_if_fail (CDK_IS_WINDOW (window));
}

/**
 * ctk_style_context_push_animatable_region:
 * @context: a #CtkStyleContext
 * @region_id: unique identifier for the animatable region
 *
 * Pushes an animatable region, so all further ctk_render_*() calls between
 * this call and the following ctk_style_context_pop_animatable_region()
 * will potentially show transition animations for this region if
 * ctk_style_context_notify_state_change() is called for a given state,
 * and the current theme/style defines transition animations for state
 * changes.
 *
 * The @region_id used must be unique in @context so the themes
 * can uniquely identify rendered elements subject to a state transition.
 *
 * Since: 3.0
 *
 * Deprecated: 3.6: This function does nothing.
 **/
void
ctk_style_context_push_animatable_region (CtkStyleContext *context,
                                          gpointer         region_id)
{
  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
  g_return_if_fail (region_id != NULL);
}

/**
 * ctk_style_context_pop_animatable_region:
 * @context: a #CtkStyleContext
 *
 * Pops an animatable region from @context.
 * See ctk_style_context_push_animatable_region().
 *
 * Since: 3.0
 *
 * Deprecated: 3.6: This function does nothing.
 **/
void
ctk_style_context_pop_animatable_region (CtkStyleContext *context)
{
  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
}

static CtkCssStyleChange magic_number;

void
ctk_style_context_validate (CtkStyleContext  *context,
                            CtkCssStyleChange *change)
{
  CtkStyleContextPrivate *priv;

  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));

  priv = context->priv;

  /* Avoid reentrancy */
  if (priv->invalidating_context)
    return;

  if (change)
    priv->invalidating_context = change;
  else
    priv->invalidating_context = &magic_number;

  g_signal_emit (context, signals[CHANGED], 0);

  g_object_set_data (G_OBJECT (context), "font-cache-for-get_font", NULL);

  priv->invalidating_context = NULL;
}

/**
 * ctk_style_context_invalidate:
 * @context: a #CtkStyleContext.
 *
 * Invalidates @context style information, so it will be reconstructed
 * again. It is useful if you modify the @context and need the new
 * information immediately.
 *
 * Since: 3.0
 *
 * Deprecated: 3.12: Style contexts are invalidated automatically.
 **/
void
ctk_style_context_invalidate (CtkStyleContext *context)
{
  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));

  ctk_style_context_clear_property_cache (context);

  ctk_style_context_validate (context, NULL);
}

/**
 * ctk_style_context_set_background:
 * @context: a #CtkStyleContext
 * @window: a #CdkWindow
 *
 * Sets the background of @window to the background pattern or
 * color specified in @context for its current state.
 *
 * Since: 3.0
 *
 * Deprecated: 3.18: Use ctk_render_background() instead.
 *   Note that clients still using this function are now responsible
 *   for calling this function again whenever @context is invalidated.
 **/
void
ctk_style_context_set_background (CtkStyleContext *context,
                                  CdkWindow       *window)
{
  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
  g_return_if_fail (CDK_IS_WINDOW (window));

  /* This is a sophisticated optimization.
   * If we know the CDK window's background will be opaque, we mark
   * it as opaque. This is so CDK can do all the optimizations it does
   * for opaque windows and be fast.
   * This is mainly used when scrolling.
   *
   * We could indeed just set black instead of the color we have.
   */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  if (ctk_css_style_render_background_is_opaque (ctk_style_context_lookup_style (context)))
    {
      const CdkRGBA *color;

      color = _ctk_css_rgba_value_get_rgba (_ctk_style_context_peek_property (context, CTK_CSS_PROPERTY_BACKGROUND_COLOR));

      cdk_window_set_background_rgba (window, color);
    }
  else
    {
      CdkRGBA transparent = { 0.0, 0.0, 0.0, 0.0 };
      cdk_window_set_background_rgba (window, &transparent);
    }
G_GNUC_END_IGNORE_DEPRECATIONS
}

/**
 * ctk_style_context_get_color:
 * @context: a #CtkStyleContext
 * @state: state to retrieve the color for
 * @color: (out): return value for the foreground color
 *
 * Gets the foreground color for a given state.
 *
 * See ctk_style_context_get_property() and
 * #CTK_STYLE_PROPERTY_COLOR for details.
 *
 * Since: 3.0
 **/
void
ctk_style_context_get_color (CtkStyleContext *context,
                             CtkStateFlags    state,
                             CdkRGBA         *color)
{
  CdkRGBA *c;

  g_return_if_fail (color != NULL);
  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));

  ctk_style_context_get (context,
                         state,
                         "color", &c,
                         NULL);

  *color = *c;
  cdk_rgba_free (c);
}

/**
 * ctk_style_context_get_background_color:
 * @context: a #CtkStyleContext
 * @state: state to retrieve the color for
 * @color: (out): return value for the background color
 *
 * Gets the background color for a given state.
 *
 * This function is far less useful than it seems, and it should not be used in
 * newly written code. CSS has no concept of "background color", as a background
 * can be an image, or a gradient, or any other pattern including solid colors.
 *
 * The only reason why you would call ctk_style_context_get_background_color() is
 * to use the returned value to draw the background with it; the correct way to
 * achieve this result is to use ctk_render_background() instead, along with CSS
 * style classes to modify the color to be rendered.
 *
 * Since: 3.0
 *
 * Deprecated: 3.16: Use ctk_render_background() instead.
 **/
void
ctk_style_context_get_background_color (CtkStyleContext *context,
                                        CtkStateFlags    state,
                                        CdkRGBA         *color)
{
  CdkRGBA *c;

  g_return_if_fail (color != NULL);
  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));

  ctk_style_context_get (context,
                         state,
                         "background-color", &c,
                         NULL);

  *color = *c;
  cdk_rgba_free (c);
}

/**
 * ctk_style_context_get_border_color:
 * @context: a #CtkStyleContext
 * @state: state to retrieve the color for
 * @color: (out): return value for the border color
 *
 * Gets the border color for a given state.
 *
 * Since: 3.0
 *
 * Deprecated: 3.16: Use ctk_render_frame() instead.
 **/
void
ctk_style_context_get_border_color (CtkStyleContext *context,
                                    CtkStateFlags    state,
                                    CdkRGBA         *color)
{
  CdkRGBA *c;

  g_return_if_fail (color != NULL);
  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));

  ctk_style_context_get (context,
                         state,
                         "border-color", &c,
                         NULL);

  *color = *c;
  cdk_rgba_free (c);
}

/**
 * ctk_style_context_get_border:
 * @context: a #CtkStyleContext
 * @state: state to retrieve the border for
 * @border: (out): return value for the border settings
 *
 * Gets the border for a given state as a #CtkBorder.
 *
 * See ctk_style_context_get_property() and
 * #CTK_STYLE_PROPERTY_BORDER_WIDTH for details.
 *
 * Since: 3.0
 **/
void
ctk_style_context_get_border (CtkStyleContext *context,
                              CtkStateFlags    state,
                              CtkBorder       *border)
{
  CtkCssStyle *style;
  CtkStateFlags saved_state;
  double top, left, bottom, right;

  g_return_if_fail (border != NULL);
  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));

  saved_state = ctk_style_context_push_state (context, state);
  style = ctk_style_context_lookup_style (context);

  top = round (_ctk_css_number_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_BORDER_TOP_WIDTH), 100));
  right = round (_ctk_css_number_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_BORDER_RIGHT_WIDTH), 100));
  bottom = round (_ctk_css_number_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_BORDER_BOTTOM_WIDTH), 100));
  left = round (_ctk_css_number_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_BORDER_LEFT_WIDTH), 100));

  border->top = top;
  border->left = left;
  border->bottom = bottom;
  border->right = right;

  ctk_style_context_pop_state (context, saved_state);
}

/**
 * ctk_style_context_get_padding:
 * @context: a #CtkStyleContext
 * @state: state to retrieve the padding for
 * @padding: (out): return value for the padding settings
 *
 * Gets the padding for a given state as a #CtkBorder.
 * See ctk_style_context_get() and #CTK_STYLE_PROPERTY_PADDING
 * for details.
 *
 * Since: 3.0
 **/
void
ctk_style_context_get_padding (CtkStyleContext *context,
                               CtkStateFlags    state,
                               CtkBorder       *padding)
{
  CtkCssStyle *style;
  CtkStateFlags saved_state;
  double top, left, bottom, right;

  g_return_if_fail (padding != NULL);
  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));

  saved_state = ctk_style_context_push_state (context, state);
  style = ctk_style_context_lookup_style (context);

  top = round (_ctk_css_number_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_PADDING_TOP), 100));
  right = round (_ctk_css_number_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_PADDING_RIGHT), 100));
  bottom = round (_ctk_css_number_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_PADDING_BOTTOM), 100));
  left = round (_ctk_css_number_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_PADDING_LEFT), 100));

  padding->top = top;
  padding->left = left;
  padding->bottom = bottom;
  padding->right = right;

  ctk_style_context_pop_state (context, saved_state);
}

/**
 * ctk_style_context_get_margin:
 * @context: a #CtkStyleContext
 * @state: state to retrieve the border for
 * @margin: (out): return value for the margin settings
 *
 * Gets the margin for a given state as a #CtkBorder.
 * See ctk_style_property_get() and #CTK_STYLE_PROPERTY_MARGIN
 * for details.
 *
 * Since: 3.0
 **/
void
ctk_style_context_get_margin (CtkStyleContext *context,
                              CtkStateFlags    state,
                              CtkBorder       *margin)
{
  CtkCssStyle *style;
  CtkStateFlags saved_state;
  double top, left, bottom, right;

  g_return_if_fail (margin != NULL);
  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));

  saved_state = ctk_style_context_push_state (context, state);
  style = ctk_style_context_lookup_style (context);

  top = round (_ctk_css_number_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_MARGIN_TOP), 100));
  right = round (_ctk_css_number_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_MARGIN_RIGHT), 100));
  bottom = round (_ctk_css_number_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_MARGIN_BOTTOM), 100));
  left = round (_ctk_css_number_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_MARGIN_LEFT), 100));

  margin->top = top;
  margin->left = left;
  margin->bottom = bottom;
  margin->right = right;

  ctk_style_context_pop_state (context, saved_state);
}

/**
 * ctk_style_context_get_font:
 * @context: a #CtkStyleContext
 * @state: state to retrieve the font for
 *
 * Returns the font description for a given state. The returned
 * object is const and will remain valid until the
 * #CtkStyleContext::changed signal happens.
 *
 * Returns: (transfer none): the #PangoFontDescription for the given
 *          state.  This object is owned by CTK+ and should not be
 *          freed.
 *
 * Since: 3.0
 *
 * Deprecated: 3.8: Use ctk_style_context_get() for "font" or
 *     subproperties instead.
 **/
const PangoFontDescription *
ctk_style_context_get_font (CtkStyleContext *context,
                            CtkStateFlags    state)
{
  GHashTable *hash;
  PangoFontDescription *description, *previous;

  g_return_val_if_fail (CTK_IS_STYLE_CONTEXT (context), NULL);

  /* Yuck, fonts are created on-demand but we don't return a ref.
   * Do bad things to achieve this requirement */
  ctk_style_context_get (context, state, "font", &description, NULL);
  
  hash = g_object_get_data (G_OBJECT (context), "font-cache-for-get_font");

  if (hash == NULL)
    {
      hash = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                    NULL,
                                    (GDestroyNotify) pango_font_description_free);
      g_object_set_data_full (G_OBJECT (context),
                              "font-cache-for-get_font",
                              hash,
                              (GDestroyNotify) g_hash_table_unref);
    }

  previous = g_hash_table_lookup (hash, GUINT_TO_POINTER (state));
  if (previous)
    {
      pango_font_description_merge (previous, description, TRUE);
      pango_font_description_free (description);
      description = previous;
    }
  else
    {
      g_hash_table_insert (hash, GUINT_TO_POINTER (state), description);
    }

  return description;
}

void
_ctk_style_context_get_cursor_color (CtkStyleContext *context,
                                     CdkRGBA         *primary_color,
                                     CdkRGBA         *secondary_color)
{
  CdkRGBA *pc, *sc;

  ctk_style_context_get (context,
                         ctk_style_context_get_state (context),
                         "caret-color", &pc,
                         "-ctk-secondary-caret-color", &sc,
                         NULL);
  if (primary_color)
    *primary_color = *pc;

  if (secondary_color)
    *secondary_color = *sc;

  cdk_rgba_free (pc);
  cdk_rgba_free (sc);
}

static void
draw_insertion_cursor (CtkStyleContext *context,
                       cairo_t         *cr,
                       gdouble          x,
                       gdouble          y,
                       gdouble          height,
                       float            aspect_ratio,
                       gboolean         is_primary,
                       PangoDirection   direction,
                       gboolean         draw_arrow)

{
  CdkRGBA primary_color;
  CdkRGBA secondary_color;
  gint stem_width;
  gint offset;

  cairo_save (cr);
  cairo_new_path (cr);

  _ctk_style_context_get_cursor_color (context, &primary_color, &secondary_color);
  cdk_cairo_set_source_rgba (cr, is_primary ? &primary_color : &secondary_color);

  /* When changing the shape or size of the cursor here,
   * propagate the changes to ctktextview.c:text_window_invalidate_cursors().
   */

  stem_width = height * aspect_ratio + 1;

  /* put (stem_width % 2) on the proper side of the cursor */
  if (direction == PANGO_DIRECTION_LTR)
    offset = stem_width / 2;
  else
    offset = stem_width - stem_width / 2;

  cairo_rectangle (cr, x - offset, y, stem_width, height);
  cairo_fill (cr);

  if (draw_arrow)
    {
      gint arrow_width;
      gint ax, ay;

      arrow_width = stem_width + 1;

      if (direction == PANGO_DIRECTION_RTL)
        {
          ax = x - offset - 1;
          ay = y + height - arrow_width * 2 - arrow_width + 1;

          cairo_move_to (cr, ax, ay + 1);
          cairo_line_to (cr, ax - arrow_width, ay + arrow_width);
          cairo_line_to (cr, ax, ay + 2 * arrow_width);
          cairo_fill (cr);
        }
      else if (direction == PANGO_DIRECTION_LTR)
        {
          ax = x + stem_width - offset;
          ay = y + height - arrow_width * 2 - arrow_width + 1;

          cairo_move_to (cr, ax, ay + 1);
          cairo_line_to (cr, ax + arrow_width, ay + arrow_width);
          cairo_line_to (cr, ax, ay + 2 * arrow_width);
          cairo_fill (cr);
        }
      else
        g_assert_not_reached();
    }

  cairo_restore (cr);
}

/**
 * ctk_render_insertion_cursor:
 * @context: a #CtkStyleContext
 * @cr: a #cairo_t
 * @x: X origin
 * @y: Y origin
 * @layout: the #PangoLayout of the text
 * @index: the index in the #PangoLayout
 * @direction: the #PangoDirection of the text
 *
 * Draws a text caret on @cr at the specified index of @layout.
 *
 * Since: 3.4
 **/
void
ctk_render_insertion_cursor (CtkStyleContext *context,
                             cairo_t         *cr,
                             gdouble          x,
                             gdouble          y,
                             PangoLayout     *layout,
                             int              index,
                             PangoDirection   direction)
{
  CtkStyleContextPrivate *priv;
  gboolean split_cursor;
  float aspect_ratio;
  PangoRectangle strong_pos, weak_pos;
  PangoRectangle *cursor1, *cursor2;
  PangoDirection keymap_direction;
  PangoDirection direction2;

  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (PANGO_IS_LAYOUT (layout));
  g_return_if_fail (index >= 0);

  priv = context->priv;

  g_object_get (ctk_settings_get_for_screen (priv->screen),
                "ctk-split-cursor", &split_cursor,
                "ctk-cursor-aspect-ratio", &aspect_ratio,
                NULL);

  keymap_direction = cdk_keymap_get_direction (cdk_keymap_get_for_display (cdk_screen_get_display (priv->screen)));

  pango_layout_get_cursor_pos (layout, index, &strong_pos, &weak_pos);

  direction2 = PANGO_DIRECTION_NEUTRAL;

  if (split_cursor)
    {
      cursor1 = &strong_pos;

      if (strong_pos.x != weak_pos.x || strong_pos.y != weak_pos.y)
        {
          direction2 = (direction == PANGO_DIRECTION_LTR) ? PANGO_DIRECTION_RTL : PANGO_DIRECTION_LTR;
          cursor2 = &weak_pos;
        }
    }
  else
    {
      if (keymap_direction == direction)
        cursor1 = &strong_pos;
      else
        cursor1 = &weak_pos;
    }

  draw_insertion_cursor (context,
                         cr,
                         x + PANGO_PIXELS (cursor1->x),
                         y + PANGO_PIXELS (cursor1->y),
                         PANGO_PIXELS (cursor1->height),
                         aspect_ratio,
                         TRUE,
                         direction,
                         direction2 != PANGO_DIRECTION_NEUTRAL);

  if (direction2 != PANGO_DIRECTION_NEUTRAL)
    {
      draw_insertion_cursor (context,
                             cr,
                             x + PANGO_PIXELS (cursor2->x),
                             y + PANGO_PIXELS (cursor2->y),
                             PANGO_PIXELS (cursor2->height),
                             aspect_ratio,
                             FALSE,
                             direction2,
                             TRUE);
    }
}

/**
 * ctk_draw_insertion_cursor:
 * @widget:  a #CtkWidget
 * @cr: cairo context to draw to
 * @location: location where to draw the cursor (@location->width is ignored)
 * @is_primary: if the cursor should be the primary cursor color.
 * @direction: whether the cursor is left-to-right or
 *             right-to-left. Should never be #CTK_TEXT_DIR_NONE
 * @draw_arrow: %TRUE to draw a directional arrow on the
 *        cursor. Should be %FALSE unless the cursor is split.
 *
 * Draws a text caret on @cr at @location. This is not a style function
 * but merely a convenience function for drawing the standard cursor shape.
 *
 * Since: 3.0
 * Deprecated: 3.4: Use ctk_render_insertion_cursor() instead.
 */
void
ctk_draw_insertion_cursor (CtkWidget          *widget,
                           cairo_t            *cr,
                           const CdkRectangle *location,
                           gboolean            is_primary,
                           CtkTextDirection    direction,
                           gboolean            draw_arrow)
{
  CtkStyleContext *context;
  float aspect_ratio;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (location != NULL);
  g_return_if_fail (direction != CTK_TEXT_DIR_NONE);

  context = ctk_widget_get_style_context (widget);

  g_object_get (ctk_settings_get_for_screen (context->priv->screen),
                "ctk-cursor-aspect-ratio", &aspect_ratio,
                NULL);

  draw_insertion_cursor (context, cr,
                         location->x, location->y, location->height,
                         aspect_ratio,
                         is_primary,
                         (direction == CTK_TEXT_DIR_RTL) ? PANGO_DIRECTION_RTL : PANGO_DIRECTION_LTR,
                         draw_arrow);
}

/**
 * ctk_style_context_get_change:
 * @context: the context to query
 *
 * Queries the context for the changes for the currently executing
 * CtkStyleContext::invalidate signal. If no signal is currently
 * emitted or the signal has not been triggered by a CssNode
 * invalidation, this function returns %NULL.
 *
 * FIXME 4.0: Make this part of the signal.
 *
 * Returns: %NULL or the currently invalidating changes
 **/
CtkCssStyleChange *
ctk_style_context_get_change (CtkStyleContext *context)
{
  g_return_val_if_fail (CTK_IS_STYLE_CONTEXT (context), NULL);

  if (context->priv->invalidating_context == &magic_number)
    return NULL;

  return context->priv->invalidating_context;
}

void
_ctk_style_context_get_icon_extents (CtkStyleContext *context,
                                     CdkRectangle    *extents,
                                     gint             x,
                                     gint             y,
                                     gint             width,
                                     gint             height)
{
  g_return_if_fail (CTK_IS_STYLE_CONTEXT (context));
  g_return_if_fail (extents != NULL);

  if (_ctk_css_image_value_get_image (_ctk_style_context_peek_property (context, CTK_CSS_PROPERTY_ICON_SOURCE)) == NULL)
    {
      extents->x = extents->y = extents->width = extents->height = 0;
      return;
    }

  ctk_css_style_render_icon_get_extents (ctk_style_context_lookup_style (context),
                                         extents,
                                         x, y, width, height);
}

PangoAttrList *
_ctk_style_context_get_pango_attributes (CtkStyleContext *context)
{
  return ctk_css_style_get_pango_attributes (ctk_style_context_lookup_style (context));
}

static AtkAttributeSet *
add_attribute (AtkAttributeSet  *attributes,
               AtkTextAttribute  attr,
               const gchar      *value)
{
  AtkAttribute *at;

  at = g_new (AtkAttribute, 1);
  at->name = g_strdup (atk_text_attribute_get_name (attr));
  at->value = g_strdup (value);

  return g_slist_prepend (attributes, at);
}

/*
 * _ctk_style_context_get_attributes:
 * @attributes: a #AtkAttributeSet to add attributes to
 * @context: the #CtkStyleContext to get attributes from
 * @flags: the state to use with @context
 *
 * Adds the foreground and background color from @context to
 * @attributes, after translating them to ATK attributes.
 *
 * This is a convenience function that can be used in
 * implementing the #AtkText interface in widgets.
 *
 * Returns: the modified #AtkAttributeSet
 */
AtkAttributeSet *
_ctk_style_context_get_attributes (AtkAttributeSet *attributes,
                                   CtkStyleContext *context,
                                   CtkStateFlags    flags)
{
  CdkRGBA color;
  gchar *value;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  ctk_style_context_get_background_color (context, flags, &color);
G_GNUC_END_IGNORE_DEPRECATIONS
  value = g_strdup_printf ("%u,%u,%u",
                           (guint) ceil (color.red * 65536 - color.red),
                           (guint) ceil (color.green * 65536 - color.green),
                           (guint) ceil (color.blue * 65536 - color.blue));
  attributes = add_attribute (attributes, ATK_TEXT_ATTR_BG_COLOR, value);
  g_free (value);

  ctk_style_context_get_color (context, flags, &color);
  value = g_strdup_printf ("%u,%u,%u",
                           (guint) ceil (color.red * 65536 - color.red),
                           (guint) ceil (color.green * 65536 - color.green),
                           (guint) ceil (color.blue * 65536 - color.blue));
  attributes = add_attribute (attributes, ATK_TEXT_ATTR_FG_COLOR, value);
  g_free (value);

  return attributes;
}

cairo_pattern_t *
ctk_gradient_resolve_for_context (CtkGradient     *gradient,
                                  CtkStyleContext *context)
{
  CtkStyleContextPrivate *priv = context->priv;

  g_return_val_if_fail (gradient != NULL, NULL);
  g_return_val_if_fail (CTK_IS_STYLE_CONTEXT (context), NULL);

  return _ctk_gradient_resolve_full (gradient,
                                     CTK_STYLE_PROVIDER_PRIVATE (priv->cascade),
                                     ctk_style_context_lookup_style (context),
                                     priv->parent ? ctk_style_context_lookup_style (priv->parent) : NULL);
}

/**
 * CtkStyleContextPrintFlags:
 * @CTK_STYLE_CONTEXT_PRINT_RECURSE: Print the entire tree of
 *     CSS nodes starting at the style context's node
 * @CTK_STYLE_CONTEXT_PRINT_SHOW_STYLE: Show the values of the
 *     CSS properties for each node
 *
 * Flags that modify the behavior of ctk_style_context_to_string().
 * New values may be added to this enumeration.
 */

/**
 * ctk_style_context_to_string:
 * @context: a #CtkStyleContext
 * @flags: Flags that determine what to print
 *
 * Converts the style context into a string representation.
 *
 * The string representation always includes information about
 * the name, state, id, visibility and style classes of the CSS
 * node that is backing @context. Depending on the flags, more
 * information may be included.
 *
 * This function is intended for testing and debugging of the
 * CSS implementation in CTK+. There are no guarantees about
 * the format of the returned string, it may change.
 *
 * Returns: a newly allocated string representing @context
 *
 * Since: 3.20
 */
char *
ctk_style_context_to_string (CtkStyleContext           *context,
                             CtkStyleContextPrintFlags  flags)
{
  GString *string;

  g_return_val_if_fail (CTK_IS_STYLE_CONTEXT (context), NULL);

  string = g_string_new ("");

  ctk_css_node_print (context->priv->cssnode, flags, string, 0);

  return g_string_free (string, FALSE);
}
