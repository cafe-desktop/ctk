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

#ifndef __CTK_STYLE_PROVIDER_PRIVATE_H__
#define __CTK_STYLE_PROVIDER_PRIVATE_H__

#include <glib-object.h>
#include "ctk/ctkcsskeyframesprivate.h"
#include "ctk/ctkcsslookupprivate.h"
#include "ctk/ctkcssmatcherprivate.h"
#include "ctk/ctkcssvalueprivate.h"
#include <ctk/ctktypes.h>

G_BEGIN_DECLS

#define CTK_TYPE_STYLE_PROVIDER_PRIVATE          (_ctk_style_provider_private_get_type ())
#define CTK_STYLE_PROVIDER_PRIVATE(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_STYLE_PROVIDER_PRIVATE, CtkStyleProviderPrivate))
#define CTK_IS_STYLE_PROVIDER_PRIVATE(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_STYLE_PROVIDER_PRIVATE))
#define CTK_STYLE_PROVIDER_PRIVATE_GET_INTERFACE(o)  (G_TYPE_INSTANCE_GET_INTERFACE ((o), CTK_TYPE_STYLE_PROVIDER_PRIVATE, CtkStyleProviderPrivateInterface))

typedef struct _CtkStyleProviderPrivateInterface CtkStyleProviderPrivateInterface;
/* typedef struct _CtkStyleProviderPrivate CtkStyleProviderPrivate; */ /* dummy typedef */

struct _CtkStyleProviderPrivateInterface
{
  GTypeInterface g_iface;

  CtkCssValue *         (* get_color)           (CtkStyleProviderPrivate *provider,
                                                 const char              *name);
  CtkSettings *         (* get_settings)        (CtkStyleProviderPrivate *provider);
  CtkCssKeyframes *     (* get_keyframes)       (CtkStyleProviderPrivate *provider,
                                                 const char              *name);
  int                   (* get_scale)           (CtkStyleProviderPrivate *provider);
  void                  (* lookup)              (CtkStyleProviderPrivate *provider,
                                                 const CtkCssMatcher     *matcher,
                                                 CtkCssLookup            *lookup,
                                                 CtkCssChange            *out_change);
  void                  (* emit_error)          (CtkStyleProviderPrivate *provider,
                                                 CtkCssSection           *section,
                                                 const GError            *error);
  /* signal */
  void                  (* changed)             (CtkStyleProviderPrivate *provider);
};

GType                   _ctk_style_provider_private_get_type     (void) G_GNUC_CONST;

CtkSettings *           _ctk_style_provider_private_get_settings (CtkStyleProviderPrivate *provider);
CtkCssValue *           _ctk_style_provider_private_get_color    (CtkStyleProviderPrivate *provider,
                                                                  const char              *name);
CtkCssKeyframes *       _ctk_style_provider_private_get_keyframes(CtkStyleProviderPrivate *provider,
                                                                  const char              *name);
int                     _ctk_style_provider_private_get_scale    (CtkStyleProviderPrivate *provider);
void                    _ctk_style_provider_private_lookup       (CtkStyleProviderPrivate *provider,
                                                                  const CtkCssMatcher     *matcher,
                                                                  CtkCssLookup            *lookup,
                                                                  CtkCssChange            *out_change);

void                    _ctk_style_provider_private_changed      (CtkStyleProviderPrivate *provider);

void                    _ctk_style_provider_private_emit_error   (CtkStyleProviderPrivate *provider,
                                                                  CtkCssSection           *section,
                                                                  GError                  *error);

G_END_DECLS

#endif /* __CTK_STYLE_PROVIDER_PRIVATE_H__ */
