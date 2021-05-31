/*
 * Copyright © 2011 Canonical Limited
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the licence, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#ifndef __CTK_MODEL_MENU_ITEM_H__
#define __CTK_MODEL_MENU_ITEM_H__

#include <ctk/ctkcheckmenuitem.h>
#include <ctk/ctkmenutrackeritem.h>

#define CTK_TYPE_MODEL_MENU_ITEM                            (ctk_model_menu_item_get_type ())
#define CTK_MODEL_MENU_ITEM(inst)                           (G_TYPE_CHECK_INSTANCE_CAST ((inst),                      \
                                                             CTK_TYPE_MODEL_MENU_ITEM, GtkModelMenuItem))
#define CTK_IS_MODEL_MENU_ITEM(inst)                        (G_TYPE_CHECK_INSTANCE_TYPE ((inst),                      \
                                                             CTK_TYPE_MODEL_MENU_ITEM))

typedef struct _GtkModelMenuItem                            GtkModelMenuItem;

G_GNUC_INTERNAL
GType                   ctk_model_menu_item_get_type                    (void) G_GNUC_CONST;

G_GNUC_INTERNAL
GtkWidget *             ctk_model_menu_item_new                         (void);

#endif /* __CTK_MODEL_MENU_ITEM_H__ */
