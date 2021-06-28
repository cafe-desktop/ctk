/* cdkbroadwaydisplaymanager.h
 *
 * Copyright (C) 2005-2007  Imendio AB
 * Copyright 2010 Red Hat, Inc.
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

#ifndef __CDK_BROADWAY_DISPLAY_MANAGER_H__
#define __CDK_BROADWAY_DISPLAY_MANAGER_H__

#if !defined(__CDKBROADWAY_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdkbroadway.h> can be included directly."
#endif

#include <cdk/cdk.h>

G_BEGIN_DECLS

#define CDK_TYPE_BROADWAY_DISPLAY_MANAGER    (cdk_broadway_display_manager_get_type ())
#define CDK_BROADWAY_DISPLAY_MANAGER(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), CDK_TYPE_BROADWAY_DISPLAY_MANAGER, CdkBroadwayDisplayManager))

#ifdef CDK_COMPILATION
typedef struct _CdkBroadwayDisplayManager CdkBroadwayDisplayManager;
#else
typedef CdkDisplayManager _CdkBroadwayDisplayManager;
#endif
typedef struct _CdkDisplayManagerClass CdkBroadwayDisplayManagerClass;

CDK_AVAILABLE_IN_ALL
GType cdk_broadway_display_manager_get_type (void);

G_END_DECLS

#endif /* __CDK_BROADWAY_DISPLAY_MANAGER_H__ */
