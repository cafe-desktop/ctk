/* cdkquartzcursor.h
 *
 * Copyright (C) 2005-2007  Imendio AB
 * Copyright (C) 2010 Kristian Rietveld  <kris@ctk.org>
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

#ifndef __CDK_QUARTZ_CURSOR_H__
#define __CDK_QUARTZ_CURSOR_H__

#if !defined(__CDKQUARTZ_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdkquartz.h> can be included directly."
#endif

#include <cdk/cdk.h>

G_BEGIN_DECLS

#define CDK_TYPE_QUARTZ_CURSOR              (cdk_quartz_cursor_get_type ())
#define CDK_QUARTZ_CURSOR(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), CDK_TYPE_QUARTZ_CURSOR, CdkQuartzCursor))
#define CDK_QUARTZ_CURSOR_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CDK_TYPE_QUARTZ_CURSOR, CdkQuartzCursorClass))
#define CDK_IS_QUARTZ_CURSOR(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), CDK_TYPE_QUARTZ_CURSOR))
#define CDK_IS_QUARTZ_CURSOR_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CDK_TYPE_QUARTZ_CURSOR))
#define CDK_QUARTZ_CURSOR_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CDK_TYPE_QUARTZ_CURSOR, CdkQuartzCursorClass))

#ifdef CDK_COMPILATION
typedef struct _CdkQuartzCursor CdkQuartzCursor;
#else
typedef CdkCursor CdkQuartzCursor;
#endif
typedef struct _CdkQuartzCursorClass CdkQuartzCursorClass;

CDK_AVAILABLE_IN_ALL
GType cdk_quartz_cursor_get_type (void);

G_END_DECLS

#endif /* __CDK_QUARTZ_CURSOR_H__ */
