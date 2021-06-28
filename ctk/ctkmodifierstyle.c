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
#include "ctkmodifierstyle.h"
#include "ctkstyleproviderprivate.h"
#include "ctkintl.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

typedef struct StylePropertyValue StylePropertyValue;

struct _CtkModifierStylePrivate
{
  CtkStyleProperties *style;
  GHashTable *color_properties;
};

static void ctk_modifier_style_provider_init         (CtkStyleProviderIface            *iface);
static void ctk_modifier_style_provider_private_init (CtkStyleProviderPrivateInterface *iface);
static void ctk_modifier_style_finalize              (GObject                          *object);

G_DEFINE_TYPE_EXTENDED (CtkModifierStyle, _ctk_modifier_style, G_TYPE_OBJECT, 0,
                        G_ADD_PRIVATE (CtkModifierStyle)
                        G_IMPLEMENT_INTERFACE (CTK_TYPE_STYLE_PROVIDER,
                                               ctk_modifier_style_provider_init)
                        G_IMPLEMENT_INTERFACE (CTK_TYPE_STYLE_PROVIDER_PRIVATE,
                                               ctk_modifier_style_provider_private_init));

static void
_ctk_modifier_style_class_init (CtkModifierStyleClass *klass)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = ctk_modifier_style_finalize;
}

static void
_ctk_modifier_style_init (CtkModifierStyle *modifier_style)
{
  CtkModifierStylePrivate *priv;

  priv = modifier_style->priv = _ctk_modifier_style_get_instance_private (modifier_style);

  priv->color_properties = g_hash_table_new_full (g_str_hash,
                                                  g_str_equal,
                                                  (GDestroyNotify) g_free,
                                                  (GDestroyNotify) cdk_rgba_free);
  priv->style = ctk_style_properties_new ();
}

static gboolean
ctk_modifier_style_get_style_property (CtkStyleProvider *provider,
                                       CtkWidgetPath    *path,
                                       CtkStateFlags     state,
                                       GParamSpec       *pspec,
                                       GValue           *value)
{
  CtkModifierStylePrivate *priv;
  GdkRGBA *rgba;
  GdkColor color;
  gchar *str;

  /* Reject non-color types for now */
  if (pspec->value_type != GDK_TYPE_COLOR)
    return FALSE;

  priv = CTK_MODIFIER_STYLE (provider)->priv;
  str = g_strdup_printf ("-%s-%s",
                         g_type_name (pspec->owner_type),
                         pspec->name);

  rgba = g_hash_table_lookup (priv->color_properties, str);
  g_free (str);

  if (!rgba)
    return FALSE;

  color.red = (guint) (rgba->red * 65535.) + 0.5;
  color.green = (guint) (rgba->green * 65535.) + 0.5;
  color.blue = (guint) (rgba->blue * 65535.) + 0.5;

  g_value_set_boxed (value, &color);
  return TRUE;
}

static void
ctk_modifier_style_provider_init (CtkStyleProviderIface *iface)
{
  iface->get_style_property = ctk_modifier_style_get_style_property;
}

static CtkCssValue *
ctk_modifier_style_provider_get_color (CtkStyleProviderPrivate *provider,
                                       const char              *name)
{
  CtkModifierStyle *style = CTK_MODIFIER_STYLE (provider);

  return _ctk_style_provider_private_get_color (CTK_STYLE_PROVIDER_PRIVATE (style->priv->style), name);
}

static void
ctk_modifier_style_provider_lookup (CtkStyleProviderPrivate *provider,
                                    const CtkCssMatcher     *matcher,
                                    CtkCssLookup            *lookup,
                                    CtkCssChange            *change)
{
  CtkModifierStyle *style = CTK_MODIFIER_STYLE (provider);

  _ctk_style_provider_private_lookup (CTK_STYLE_PROVIDER_PRIVATE (style->priv->style),
                                      matcher,
                                      lookup,
                                      change);
}

static void
ctk_modifier_style_provider_private_init (CtkStyleProviderPrivateInterface *iface)
{
  iface->get_color = ctk_modifier_style_provider_get_color;
  iface->lookup = ctk_modifier_style_provider_lookup;
}

static void
ctk_modifier_style_finalize (GObject *object)
{
  CtkModifierStylePrivate *priv;

  priv = CTK_MODIFIER_STYLE (object)->priv;
  g_hash_table_destroy (priv->color_properties);
  g_object_unref (priv->style);

  G_OBJECT_CLASS (_ctk_modifier_style_parent_class)->finalize (object);
}

CtkModifierStyle *
_ctk_modifier_style_new (void)
{
  return g_object_new (CTK_TYPE_MODIFIER_STYLE, NULL);
}

static void
modifier_style_set_color (CtkModifierStyle *style,
                          const gchar      *prop,
                          CtkStateFlags     state,
                          const GdkRGBA    *color)
{
  CtkModifierStylePrivate *priv;

  g_return_if_fail (CTK_IS_MODIFIER_STYLE (style));

  priv = style->priv;

  if (color)
    ctk_style_properties_set (priv->style, state,
                              prop, color,
                              NULL);
  else
    ctk_style_properties_unset_property (priv->style, prop, state);

  _ctk_style_provider_private_changed (CTK_STYLE_PROVIDER_PRIVATE (style));
}

void
_ctk_modifier_style_set_background_color (CtkModifierStyle *style,
                                          CtkStateFlags     state,
                                          const GdkRGBA    *color)
{
  g_return_if_fail (CTK_IS_MODIFIER_STYLE (style));

  modifier_style_set_color (style, "background-color", state, color);
}

void
_ctk_modifier_style_set_color (CtkModifierStyle *style,
                               CtkStateFlags     state,
                               const GdkRGBA    *color)
{
  g_return_if_fail (CTK_IS_MODIFIER_STYLE (style));

  modifier_style_set_color (style, "color", state, color);
}

void
_ctk_modifier_style_set_font (CtkModifierStyle           *style,
                              const PangoFontDescription *font_desc)
{
  CtkModifierStylePrivate *priv;

  g_return_if_fail (CTK_IS_MODIFIER_STYLE (style));

  priv = style->priv;

  if (font_desc)
    ctk_style_properties_set (priv->style, 0,
                              "font", font_desc,
                              NULL);
  else
    ctk_style_properties_unset_property (priv->style, "font", 0);

  _ctk_style_provider_private_changed (CTK_STYLE_PROVIDER_PRIVATE (style));
}

void
_ctk_modifier_style_map_color (CtkModifierStyle *style,
                               const gchar      *name,
                               const GdkRGBA    *color)
{
  CtkModifierStylePrivate *priv;
  CtkSymbolicColor *symbolic_color = NULL;

  g_return_if_fail (CTK_IS_MODIFIER_STYLE (style));
  g_return_if_fail (name != NULL);

  priv = style->priv;

  if (color)
    symbolic_color = ctk_symbolic_color_new_literal (color);

  ctk_style_properties_map_color (priv->style,
                                  name, symbolic_color);

  _ctk_style_provider_private_changed (CTK_STYLE_PROVIDER_PRIVATE (style));
}

void
_ctk_modifier_style_set_color_property (CtkModifierStyle *style,
                                        GType             widget_type,
                                        const gchar      *prop_name,
                                        const GdkRGBA    *color)
{
  CtkModifierStylePrivate *priv;
  const GdkRGBA *old_color;
  gchar *str;

  g_return_if_fail (CTK_IS_MODIFIER_STYLE (style));
  g_return_if_fail (g_type_is_a (widget_type, CTK_TYPE_WIDGET));
  g_return_if_fail (prop_name != NULL);

  priv = style->priv;
  str = g_strdup_printf ("-%s-%s", g_type_name (widget_type), prop_name);

  old_color = g_hash_table_lookup (priv->color_properties, str);

  if ((!color && !old_color) ||
      (color && old_color && cdk_rgba_equal (color, old_color)))
    {
      g_free (str);
      return;
    }

  if (color)
    {
      g_hash_table_insert (priv->color_properties, str,
                           cdk_rgba_copy (color));
    }
  else
    {
      g_hash_table_remove (priv->color_properties, str);
      g_free (str);
    }

  _ctk_style_provider_private_changed (CTK_STYLE_PROVIDER_PRIVATE (style));
}

G_GNUC_END_IGNORE_DEPRECATIONS
