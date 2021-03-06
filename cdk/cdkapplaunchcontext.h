/* cdkapplaunchcontext.h - Ctk+ implementation for GAppLaunchContext
 *
 * Copyright (C) 2007 Red Hat, Inc.
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
 *
 * Author: Alexander Larsson <alexl@redhat.com>
 */

#ifndef __CDK_APP_LAUNCH_CONTEXT_H__
#define __CDK_APP_LAUNCH_CONTEXT_H__

#if !defined (__CDK_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdk.h> can be included directly."
#endif

#include <gio/gio.h>
#include <cdk/cdkversionmacros.h>
#include <cdk/cdktypes.h>
#include <cdk/cdkscreen.h>

G_BEGIN_DECLS

#define CDK_TYPE_APP_LAUNCH_CONTEXT         (cdk_app_launch_context_get_type ())
#define CDK_APP_LAUNCH_CONTEXT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CDK_TYPE_APP_LAUNCH_CONTEXT, CdkAppLaunchContext))
        #define CDK_IS_APP_LAUNCH_CONTEXT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CDK_TYPE_APP_LAUNCH_CONTEXT))


CDK_AVAILABLE_IN_ALL
GType                cdk_app_launch_context_get_type      (void);

CDK_DEPRECATED_IN_3_0_FOR(cdk_display_get_app_launch_context)
CdkAppLaunchContext *cdk_app_launch_context_new           (void);
CDK_DEPRECATED_IN_3_0_FOR(cdk_display_get_app_launch_context)
void                 cdk_app_launch_context_set_display   (CdkAppLaunchContext *context,
                                                           CdkDisplay          *display);
CDK_AVAILABLE_IN_ALL
void                 cdk_app_launch_context_set_screen    (CdkAppLaunchContext *context,
                                                           CdkScreen           *screen);
CDK_AVAILABLE_IN_ALL
void                 cdk_app_launch_context_set_desktop   (CdkAppLaunchContext *context,
                                                           gint                 desktop);
CDK_AVAILABLE_IN_ALL
void                 cdk_app_launch_context_set_timestamp (CdkAppLaunchContext *context,
                                                           guint32              timestamp);
CDK_AVAILABLE_IN_ALL
void                 cdk_app_launch_context_set_icon      (CdkAppLaunchContext *context,
                                                           GIcon               *icon);
CDK_AVAILABLE_IN_ALL
void                 cdk_app_launch_context_set_icon_name (CdkAppLaunchContext *context,
                                                           const char          *icon_name);

G_END_DECLS

#endif /* __CDK_APP_LAUNCH_CONTEXT_H__ */
