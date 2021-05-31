/*
 * Copyright © 2012 Red Hat Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Alexander Larsson <alexl@gnome.org>
 */

#ifndef __CTK_CSS_KEYFRAMES_PRIVATE_H__
#define __CTK_CSS_KEYFRAMES_PRIVATE_H__

#include "ctkcssparserprivate.h"
#include "ctkcssvalueprivate.h"
#include "ctktypes.h"

G_BEGIN_DECLS

typedef struct _CtkCssKeyframes CtkCssKeyframes;

CtkCssKeyframes *   _ctk_css_keyframes_parse                  (CtkCssParser           *parser);

CtkCssKeyframes *   _ctk_css_keyframes_ref                    (CtkCssKeyframes        *keyframes);
void                _ctk_css_keyframes_unref                  (CtkCssKeyframes        *keyframes);

void                _ctk_css_keyframes_print                  (CtkCssKeyframes        *keyframes,
                                                               GString                *string);

CtkCssKeyframes *   _ctk_css_keyframes_compute                (CtkCssKeyframes         *keyframes,
                                                               CtkStyleProviderPrivate *provider,
                                                               CtkCssStyle             *style,
                                                               CtkCssStyle             *parent_style);

guint               _ctk_css_keyframes_get_n_properties       (CtkCssKeyframes        *keyframes);
guint               _ctk_css_keyframes_get_property_id        (CtkCssKeyframes        *keyframes,
                                                               guint                   id);
CtkCssValue *       _ctk_css_keyframes_get_value              (CtkCssKeyframes        *keyframes,
                                                               guint                   id,
                                                               double                  progress,
                                                               CtkCssValue            *default_value);

G_END_DECLS

#endif /* __CTK_CSS_KEYFRAMES_PRIVATE_H__ */
