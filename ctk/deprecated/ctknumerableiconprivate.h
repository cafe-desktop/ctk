/*
 * ctknumerableiconprivate.h: private methods for CtkNumerableIcon
 *
 * Copyright (C) 2010 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Cosimo Cecchi <cosimoc@redhat.com>
 */

#ifndef __CTK_NUMERABLE_ICON_PRIVATE_H__
#define __CTK_NUMERABLE_ICON_PRIVATE_H__

#include "ctknumerableicon.h"

G_BEGIN_DECLS

void _ctk_numerable_icon_set_background_icon_size (CtkNumerableIcon *self,
                                                   gint icon_size);

G_END_DECLS

#endif /* __CTK_NUMERABLE_ICON_PRIVATE_H__ */
