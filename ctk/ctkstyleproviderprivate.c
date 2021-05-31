/* GTK - The GIMP Toolkit
 * Copyright (C) 2011 Benjamin Otte <otte@gnome.org>
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

#include "ctkstyleproviderprivate.h"

#include "ctkintl.h"
#include "ctkstyleprovider.h"
#include "ctkprivate.h"

enum {
  CHANGED,
  LAST_SIGNAL
};

G_DEFINE_INTERFACE (GtkStyleProviderPrivate, _ctk_style_provider_private, CTK_TYPE_STYLE_PROVIDER)

static guint signals[LAST_SIGNAL];

static void
_ctk_style_provider_private_default_init (GtkStyleProviderPrivateInterface *iface)
{
  signals[CHANGED] = g_signal_new (I_("-ctk-private-changed"),
                                   G_TYPE_FROM_INTERFACE (iface),
                                   G_SIGNAL_RUN_LAST,
                                   G_STRUCT_OFFSET (GtkStyleProviderPrivateInterface, changed),
                                   NULL, NULL,
                                   NULL,
                                   G_TYPE_NONE, 0);

}

GtkCssValue *
_ctk_style_provider_private_get_color (GtkStyleProviderPrivate *provider,
                                       const char              *name)
{
  GtkStyleProviderPrivateInterface *iface;

  /* for compat with ctk_symbolic_color_resolve() */
  if (provider == NULL)
    return NULL;

  ctk_internal_return_val_if_fail (CTK_IS_STYLE_PROVIDER_PRIVATE (provider), NULL);

  iface = CTK_STYLE_PROVIDER_PRIVATE_GET_INTERFACE (provider);

  if (!iface->get_color)
    return NULL;

  return iface->get_color (provider, name);
}

GtkCssKeyframes *
_ctk_style_provider_private_get_keyframes (GtkStyleProviderPrivate *provider,
                                           const char              *name)
{
  GtkStyleProviderPrivateInterface *iface;

  ctk_internal_return_val_if_fail (CTK_IS_STYLE_PROVIDER_PRIVATE (provider), NULL);
  ctk_internal_return_val_if_fail (name != NULL, NULL);

  iface = CTK_STYLE_PROVIDER_PRIVATE_GET_INTERFACE (provider);

  if (!iface->get_keyframes)
    return NULL;

  return iface->get_keyframes (provider, name);
}

void
_ctk_style_provider_private_lookup (GtkStyleProviderPrivate *provider,
                                    const GtkCssMatcher     *matcher,
                                    GtkCssLookup            *lookup,
                                    GtkCssChange            *out_change)
{
  GtkStyleProviderPrivateInterface *iface;

  ctk_internal_return_if_fail (CTK_IS_STYLE_PROVIDER_PRIVATE (provider));
  ctk_internal_return_if_fail (matcher != NULL);
  ctk_internal_return_if_fail (lookup != NULL);

  if (out_change)
    *out_change = 0;

  iface = CTK_STYLE_PROVIDER_PRIVATE_GET_INTERFACE (provider);

  if (!iface->lookup)
    return;

  iface->lookup (provider, matcher, lookup, out_change);
}

void
_ctk_style_provider_private_changed (GtkStyleProviderPrivate *provider)
{
  ctk_internal_return_if_fail (CTK_IS_STYLE_PROVIDER_PRIVATE (provider));

  g_signal_emit (provider, signals[CHANGED], 0);
}

GtkSettings *
_ctk_style_provider_private_get_settings (GtkStyleProviderPrivate *provider)
{
  GtkStyleProviderPrivateInterface *iface;

  g_return_val_if_fail (CTK_IS_STYLE_PROVIDER_PRIVATE (provider), NULL);

  iface = CTK_STYLE_PROVIDER_PRIVATE_GET_INTERFACE (provider);

  if (!iface->get_settings)
    return NULL;

  return iface->get_settings (provider);
}

int
_ctk_style_provider_private_get_scale (GtkStyleProviderPrivate *provider)
{
  GtkStyleProviderPrivateInterface *iface;

  ctk_internal_return_val_if_fail (CTK_IS_STYLE_PROVIDER_PRIVATE (provider), 1);

  iface = CTK_STYLE_PROVIDER_PRIVATE_GET_INTERFACE (provider);

  if (!iface->get_scale)
    return 1;

  return iface->get_scale (provider);
}

void
_ctk_style_provider_private_emit_error (GtkStyleProviderPrivate *provider,
                                        GtkCssSection           *section,
                                        GError                  *error)
{
  GtkStyleProviderPrivateInterface *iface;

  iface = CTK_STYLE_PROVIDER_PRIVATE_GET_INTERFACE (provider);

  if (iface->emit_error)
    iface->emit_error (provider, section, error);
}
