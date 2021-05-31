/*
 * GTK - The GIMP Toolkit
 * Copyright (C) 2008  Jaap Haitsma <jaap@haitsma.org>
 *
 * All rights reserved.
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

#ifndef __CTK_SHOW_H__
#define __CTK_SHOW_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkwindow.h>

G_BEGIN_DECLS

GDK_DEPRECATED_IN_3_22_FOR(ctk_show_uri_on_window)
gboolean ctk_show_uri  (GdkScreen   *screen,
                        const gchar *uri,
                        guint32      timestamp,
                        GError     **error);

GDK_AVAILABLE_IN_3_22
gboolean ctk_show_uri_on_window (CtkWindow   *parent,
                                 const char  *uri,
                                 guint32      timestamp,
                                 GError     **error);

G_END_DECLS

#endif /* __CTK_SHOW_H__ */
