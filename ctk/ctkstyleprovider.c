/* GTK - The GIMP Toolkit
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

#include "ctkstyleprovider.h"

#include "ctkintl.h"
#include "ctkprivate.h"
#include "ctkwidgetpath.h"

/**
 * SECTION:ctkstyleprovider
 * @Short_description: Interface to provide style information to CtkStyleContext
 * @Title: CtkStyleProvider
 * @See_also: #CtkStyleContext, #CtkCssProvider
 *
 * CtkStyleProvider is an interface used to provide style information to a #CtkStyleContext.
 * See ctk_style_context_add_provider() and ctk_style_context_add_provider_for_screen().
 */

static void ctk_style_provider_iface_init (gpointer g_iface);

GType
ctk_style_provider_get_type (void)
{
  static GType style_provider_type = 0;

  if (!style_provider_type)
    style_provider_type = g_type_register_static_simple (G_TYPE_INTERFACE,
                                                         I_("CtkStyleProvider"),
                                                         sizeof (CtkStyleProviderIface),
                                                         (GClassInitFunc) ctk_style_provider_iface_init,
                                                         0, NULL, 0);
  return style_provider_type;
}

static void
ctk_style_provider_iface_init (gpointer g_iface)
{
}

/**
 * ctk_style_provider_get_style:
 * @provider: a #CtkStyleProvider
 * @path: #CtkWidgetPath to query
 *
 * Returns the style settings affecting a widget defined by @path, or %NULL if
 * @provider doesn’t contemplate styling @path.
 *
 * Returns: (nullable) (transfer full): a #CtkStyleProperties containing the
 * style settings affecting @path
 *
 * Since: 3.0
 *
 * Deprecated: 3.8: Will always return %NULL for all GTK-provided style providers
 *     as the interface cannot correctly work the way CSS is specified.
 **/
CtkStyleProperties *
ctk_style_provider_get_style (CtkStyleProvider *provider,
                              CtkWidgetPath    *path)
{
  CtkStyleProviderIface *iface;

  g_return_val_if_fail (CTK_IS_STYLE_PROVIDER (provider), NULL);

  iface = CTK_STYLE_PROVIDER_GET_IFACE (provider);

  if (!iface->get_style)
    return NULL;

  return iface->get_style (provider, path);
}

/**
 * ctk_style_provider_get_style_property:
 * @provider: a #CtkStyleProvider
 * @path: #CtkWidgetPath to query
 * @state: state to query the style property for
 * @pspec: The #GParamSpec to query
 * @value: (out): return location for the property value
 *
 * Looks up a widget style property as defined by @provider for
 * the widget represented by @path.
 *
 * Returns: %TRUE if the property was found and has a value, %FALSE otherwise
 *
 * Since: 3.0
 **/
gboolean
ctk_style_provider_get_style_property (CtkStyleProvider *provider,
                                       CtkWidgetPath    *path,
                                       CtkStateFlags     state,
                                       GParamSpec       *pspec,
                                       GValue           *value)
{
  CtkStyleProviderIface *iface;

  g_return_val_if_fail (CTK_IS_STYLE_PROVIDER (provider), FALSE);
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), FALSE);
  g_return_val_if_fail (path != NULL, FALSE);
  g_return_val_if_fail (g_type_is_a (ctk_widget_path_get_object_type (path), pspec->owner_type), FALSE);
  g_return_val_if_fail (value != NULL, FALSE);

  iface = CTK_STYLE_PROVIDER_GET_IFACE (provider);

  if (!iface->get_style_property)
    return FALSE;

  return iface->get_style_property (provider, path, state, pspec, value);
}

/**
 * ctk_style_provider_get_icon_factory:
 * @provider: a #CtkStyleProvider
 * @path: #CtkWidgetPath to query
 *
 * Returns the #CtkIconFactory defined to be in use for @path, or %NULL if none
 * is defined.
 *
 * Returns: (nullable) (transfer none): The icon factory to use for @path, or %NULL
 *
 * Since: 3.0
 *
 * Deprecated: 3.8: Will always return %NULL for all GTK-provided style providers.
 **/
CtkIconFactory *
ctk_style_provider_get_icon_factory (CtkStyleProvider *provider,
				     CtkWidgetPath    *path)
{
  CtkStyleProviderIface *iface;

  g_return_val_if_fail (CTK_IS_STYLE_PROVIDER (provider), NULL);
  g_return_val_if_fail (path != NULL, NULL);

  iface = CTK_STYLE_PROVIDER_GET_IFACE (provider);

  if (!iface->get_icon_factory)
    return NULL;

  return iface->get_icon_factory (provider, path);
}
