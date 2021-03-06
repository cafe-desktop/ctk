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

#ifndef __CTK_STYLE_PROPERTIES_PRIVATE_H__
#define __CTK_STYLE_PROPERTIES_PRIVATE_H__

#include "ctkstyleproperties.h"
#include "ctkcssstylepropertyprivate.h"
#include "ctkstylecontextprivate.h"

G_BEGIN_DECLS

void           _ctk_style_properties_set_property_by_property (CtkStyleProperties      *props,
                                                               CtkCssStyleProperty     *property,
                                                               CtkStateFlags            state,
                                                               CtkCssValue             *value);

G_END_DECLS

#endif /* __CTK_STYLE_PROPERTIES_PRIVATE_H__ */
