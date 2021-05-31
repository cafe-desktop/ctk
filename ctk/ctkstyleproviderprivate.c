/* CTK - The GIMP Toolkit
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

G_DEFINE_INTERFACE (CtkStyleProviderPrivate, _ctk_style_provider_private, CTK_TYPE_STYLE_PROVIDER)

static guint signals[LAST_SIGNAL];

static void
_ctk_style_provider_private_default_init (CtkStyleProviderPrivateInterface *iface)
{
  signals[CHANGED] = g_signal_new (I_("-ctk-private-changed"),
                                   G_TYPE_FROM_INTERFACE (iface),
                                   G_SIGNAL_RUN_LAST,
                                   G_STRUCT_OFFSET (CtkStyleProviderPrivateInterface, changed),
                                   NULL, NULL,
                                   NULL,
                                   G_TYPE_NONE, 0);

}

CtkCssValue *
_ctk_style_provider_private_get_color (CtkStyleProviderPrivate *provider,
                                       const char              *name)
{
  CtkStyleProviderPrivateInterface *iface;

  /* for compat with ctk_symbolic_color_resolve() */
  if (provider == NULL)
    return NULL;

  ctk_internal_return_val_if_fail (CTK_IS_STYLE_PROVIDER_PRIVATE (provider), NULL);

  iface = CTK_STYLE_PROVIDER_PRIVATE_GET_INTERFACE (provider);

  if (!iface->get_color)
    return NULL;

  return iface->get_color (provider, name);
}

CtkCssKeyframes *
_ctk_style_provider_private_get_keyframes (CtkStyleProviderPrivate *provider,
                                           const char              *name)
{
  CtkStyleProviderPrivateInterface *iface;

  ctk_internal_return_val_if_fail (CTK_IS_STYLE_PROVIDER_PRIVATE (provider), NULL);
  ctk_internal_return_val_if_fail (name != NULL, NULL);

  iface = CTK_STYLE_PROVIDER_PRIVATE_GET_INTERFACE (provider);

  if (!iface->get_keyframes)
    return NULL;

  return iface->get_keyframes (provider, name);
}

void
_ctk_style_provider_private_lookup (CtkStyleProviderPrivate *provider,
                                    const CtkCssMatcher     *matcher,
                                    CtkCssLookup            *lookup,
                                    CtkCssChange            *out_change)
{
  CtkStyleProviderPrivateInterface *iface;

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
_ctk_style_provider_private_changed (CtkStyleProviderPrivate *provider)
{
  ctk_internal_return_if_fail (CTK_IS_STYLE_PROVIDER_PRIVATE (provider));

  g_signal_emit (provider, signals[CHANGED], 0);
}

CtkSettings *
_ctk_style_provider_private_get_settings (CtkStyleProviderPrivate *provider)
{
  CtkStyleProviderPrivateInterface *iface;

  g_return_val_if_fail (CTK_IS_STYLE_PROVIDER_PRIVATE (provider), NULL);

  iface = CTK_STYLE_PROVIDER_PRIVATE_GET_INTERFACE (provider);

  if (!iface->get_settings)
    return NULL;

  return iface->get_settings (provider);
}

int
_ctk_style_provider_private_get_scale (CtkStyleProviderPrivate *provider)
{
  CtkStyleProviderPrivateInterface *iface;

  ctk_internal_return_val_if_fail (CTK_IS_STYLE_PROVIDER_PRIVATE (provider), 1);

  iface = CTK_STYLE_PROVIDER_PRIVATE_GET_INTERFACE (provider);

  if (!iface->get_scale)
    return 1;

  return iface->get_scale (provider);
}

void
_ctk_style_provider_private_emit_error (CtkStyleProviderPrivate *provider,
                                        CtkCssSection           *section,
                                        GError                  *error)
{
  CtkStyleProviderPrivateInterface *iface;

  iface = CTK_STYLE_PROVIDER_PRIVATE_GET_INTERFACE (provider);

  if (iface->emit_error)
    iface->emit_error (provider, section, error);
}
