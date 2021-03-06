/*
 * ctkappchooserprivate.h: app-chooser interface
 *
 * Copyright (C) 2010 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Cosimo Cecchi <ccecchi@redhat.com>
 */

#ifndef __CTK_APP_CHOOSER_PRIVATE_H__
#define __CTK_APP_CHOOSER_PRIVATE_H__

#include <glib.h>
#include <gio/gio.h>

#include "ctkappchooser.h"
#include "ctkappchooserwidget.h"
#include "ctkentry.h"

G_BEGIN_DECLS

typedef struct _CtkAppChooserIface CtkAppChooserIface;
typedef CtkAppChooserIface CtkAppChooserInterface;

#define CTK_APP_CHOOSER_GET_IFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), CTK_TYPE_APP_CHOOSER, CtkAppChooserIface))

struct _CtkAppChooserIface {
  GTypeInterface base_iface;

  GAppInfo * (* get_app_info) (CtkAppChooser *object);
  void       (* refresh)      (CtkAppChooser *object);
};

void
_ctk_app_chooser_widget_set_search_entry (CtkAppChooserWidget *self,
                                          CtkEntry            *entry);


G_END_DECLS

#endif /* __CTK_APP_CHOOSER_PRIVATE_H__ */
