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

#ifndef __CTK_CSS_RGBA_VALUE_PRIVATE_H__
#define __CTK_CSS_RGBA_VALUE_PRIVATE_H__

#include "ctkcssparserprivate.h"
#include "ctkcsstypesprivate.h"
#include "ctkcssvalueprivate.h"

G_BEGIN_DECLS

CtkCssValue *   _ctk_css_rgba_value_new_from_rgba (const GdkRGBA          *rgba);

const GdkRGBA * _ctk_css_rgba_value_get_rgba      (const CtkCssValue      *rgba);


G_END_DECLS

#endif /* __CTK_CSS_RGBA_VALUE_PRIVATE_H__ */
