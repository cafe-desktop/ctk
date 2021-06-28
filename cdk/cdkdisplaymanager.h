/* CDK - The GIMP Drawing Kit
 * Copyright (C) 2000 Red Hat, Inc.
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

#ifndef __CDK_DISPLAY_MANAGER_H__
#define __CDK_DISPLAY_MANAGER_H__

#if !defined (__CDK_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdk.h> can be included directly."
#endif

#include <cdk/cdktypes.h>
#include <cdk/cdkdisplay.h>

G_BEGIN_DECLS


#define CDK_TYPE_DISPLAY_MANAGER              (cdk_display_manager_get_type ())
#define CDK_DISPLAY_MANAGER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), CDK_TYPE_DISPLAY_MANAGER, CdkDisplayManager))
#define CDK_IS_DISPLAY_MANAGER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), CDK_TYPE_DISPLAY_MANAGER))


CDK_AVAILABLE_IN_ALL
GType              cdk_display_manager_get_type            (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_ALL
CdkDisplayManager *cdk_display_manager_get                 (void);
CDK_AVAILABLE_IN_ALL
CdkDisplay *       cdk_display_manager_get_default_display (CdkDisplayManager *manager);
CDK_AVAILABLE_IN_ALL
void               cdk_display_manager_set_default_display (CdkDisplayManager *manager,
                                                            CdkDisplay        *display);
CDK_AVAILABLE_IN_ALL
GSList *           cdk_display_manager_list_displays       (CdkDisplayManager *manager);
CDK_AVAILABLE_IN_ALL
CdkDisplay *       cdk_display_manager_open_display        (CdkDisplayManager *manager,
                                                            const gchar       *name);

G_END_DECLS

#endif /* __CDK_DISPLAY_MANAGER_H__ */
