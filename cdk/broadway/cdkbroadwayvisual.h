/* cdkbroadwayvisual.h
 *
 * Copyright (C) 2011  Alexander Larsson  <alexl@redhat.com>
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

#ifndef __CDK_BROADWAY_VISUAL_H__
#define __CDK_BROADWAY_VISUAL_H__

#include <cdk/cdk.h>

G_BEGIN_DECLS

#define CDK_TYPE_BROADWAY_VISUAL              (cdk_broadway_visual_get_type ())
#define CDK_BROADWAY_VISUAL(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), CDK_TYPE_BROADWAY_VISUAL, CdkBroadwayVisual))
#define CDK_BROADWAY_VISUAL_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CDK_TYPE_BROADWAY_VISUAL, CdkBroadwayVisualClass))
#define CDK_IS_BROADWAY_VISUAL(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), CDK_TYPE_BROADWAY_VISUAL))
#define CDK_IS_BROADWAY_VISUAL_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CDK_TYPE_BROADWAY_VISUAL))
#define CDK_BROADWAY_VISUAL_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CDK_TYPE_BROADWAY_VISUAL, CdkBroadwayVisualClass))

#ifdef CDK_COMPILATION
typedef struct _CdkBroadwayVisual CdkBroadwayVisual;
#else
typedef CdkVisual CdkBroadwayVisual;
#endif
typedef struct _CdkBroadwayVisualClass CdkBroadwayVisualClass;


CDK_AVAILABLE_IN_ALL
GType cdk_broadway_visual_get_type (void);

G_END_DECLS

#endif /* __CDK_BROADWAY_VISUAL_H__ */
