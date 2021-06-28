/* CDK - The GIMP Drawing Kit
 * Copyright (C) 2010, Red Hat, Inc
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

#ifndef __CDK_DISPLAY_MANAGER_PRIVATE_H__
#define __CDK_DISPLAY_MANAGER_PRIVATE_H__

#include "cdkdisplaymanager.h"

G_BEGIN_DECLS

#define CDK_DISPLAY_MANAGER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CDK_TYPE_DISPLAY_MANAGER, CdkDisplayManagerClass))
#define CDK_IS_DISPLAY_MANAGER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CDK_TYPE_DISPLAY_MANAGER))
#define CDK_DISPLAY_MANAGER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CDK_TYPE_DISPLAY_MANAGER, CdkDisplayManagerClass))

typedef struct _CdkDisplayManagerClass CdkDisplayManagerClass;

struct _CdkDisplayManager
{
  GObject parent_instance;

  CdkDisplay *default_display;

  GSList *displays;
};

struct _CdkDisplayManagerClass
{
  GObjectClass parent_class;

  /* signals */
  void         (*display_opened)      (CdkDisplayManager *manager,
                                       CdkDisplay        *display);
};

void            _cdk_display_manager_add_display        (CdkDisplayManager      *manager,
                                                         CdkDisplay             *display);
void            _cdk_display_manager_remove_display     (CdkDisplayManager      *manager,
                                                         CdkDisplay             *display);

G_END_DECLS

#endif
