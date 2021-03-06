/* Color utilties
 *
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors: Simon Budig <Simon.Budig@unix-ag.org> (original code)
 *          Federico Mena-Quintero <federico@gimp.org> (cleanup for CTK+)
 *          Jonathan Blandford <jrb@redhat.com> (cleanup for CTK+)
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

/*
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_COLOR_UTILS_H__
#define __CTK_COLOR_UTILS_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <glib.h>
#include <cdk/cdk.h>

G_BEGIN_DECLS

CDK_AVAILABLE_IN_ALL
void ctk_hsv_to_rgb (gdouble  h, gdouble  s, gdouble  v,
                     gdouble *r, gdouble *g, gdouble *b);
CDK_AVAILABLE_IN_ALL
void ctk_rgb_to_hsv (gdouble  r, gdouble  g, gdouble  b,
                     gdouble *h, gdouble *s, gdouble *v);

G_END_DECLS

#endif /* __CTK_COLOR_UTILS_H__ */
