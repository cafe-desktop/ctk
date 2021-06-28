/* CTK - The GIMP Toolkit
 * Copyright 1998-2002 Tim Janik, Red Hat, Inc., and others.
 * Copyright (C) 2003 Alex Graveley
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

#ifndef __CTK_MODULES_H__
#define __CTK_MODULES_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <cdk/cdk.h>

G_BEGIN_DECLS

/**
 * CtkModuleInitFunc:
 * @argc: (allow-none): CTK+ always passes %NULL for this argument
 * @argv: (allow-none) (array length=argc): CTK+ always passes %NULL for this argument
 *
 * Each CTK+ module must have a function ctk_module_init() with this prototype.
 * This function is called after loading the module.
 */
typedef void     (*CtkModuleInitFunc)        (gint        *argc,
                                              gchar      ***argv);

/**
 * CtkModuleDisplayInitFunc:
 * @display: an open #GdkDisplay
 *
 * A multihead-aware CTK+ module may have a ctk_module_display_init() function
 * with this prototype. CTK+ calls this function for each opened display.
 *
 * Since: 2.2
 */
typedef void     (*CtkModuleDisplayInitFunc) (GdkDisplay   *display);


G_END_DECLS


#endif /* __CTK_MODULES_H__ */
