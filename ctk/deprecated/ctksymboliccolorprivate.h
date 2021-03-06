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

#ifndef __CTK_SYMBOLIC_COLOR_PRIVATE_H__
#define __CTK_SYMBOLIC_COLOR_PRIVATE_H__

#include "ctk/deprecated/ctksymboliccolor.h"
#include "ctk/ctkcssparserprivate.h"
#include "ctk/ctkcssvalueprivate.h"

G_BEGIN_DECLS

CtkSymbolicColor * _ctk_css_symbolic_value_new            (CtkCssParser               *parser);

CtkCssValue *      _ctk_symbolic_color_get_css_value      (CtkSymbolicColor           *symbolic);

G_END_DECLS

#endif /* __CTK_SYMBOLIC_COLOR_PRIVATE_H__ */
