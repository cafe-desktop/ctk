/* cdkquartzutils.h
 *
 * Copyright (C) 2005  Imendio AB
 * Copyright (C) 2010  Kristian Rietveld  <kris@gtk.org>
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

#ifndef __CDK_QUARTZ_UTILS_H__
#define __CDK_QUARTZ_UTILS_H__

#if !defined (__CDKQUARTZ_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdkquartz.h> can be included directly."
#endif

#include <cdk/cdk.h>

G_BEGIN_DECLS

CDK_AVAILABLE_IN_3_12
gunichar  cdk_quartz_get_key_equivalent                         (guint           key);

G_END_DECLS

#endif /* __CDK_QUARTZ_UTILS_H__ */
