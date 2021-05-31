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

#ifndef __GTK_CSS_WIN32_SIZE_VALUE_PRIVATE_H__
#define __GTK_CSS_WIN32_SIZE_VALUE_PRIVATE_H__

#include "gtkcssnumbervalueprivate.h"

G_BEGIN_DECLS

GtkCssValue *   ctk_css_win32_size_value_parse      (GtkCssParser           *parser,
                                                     GtkCssNumberParseFlags  flags);

G_END_DECLS

#endif /* __GTK_CSS_WIN32_SIZE_VALUE_PRIVATE_H__ */
