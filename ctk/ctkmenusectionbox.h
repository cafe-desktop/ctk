/*
 * Copyright © 2014 Codethink Limited
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

#ifndef __CTK_MENU_SECTION_BOX_H__
#define __CTK_MENU_SECTION_BOX_H__

#include <ctk/ctkmenutrackeritem.h>
#include <ctk/ctkstack.h>
#include <ctk/ctkbox.h>
#include <ctk/ctkpopover.h>

G_BEGIN_DECLS

#define CTK_TYPE_MENU_SECTION_BOX                           (ctk_menu_section_box_get_type ())
#define CTK_MENU_SECTION_BOX(inst)                          (G_TYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             CTK_TYPE_MENU_SECTION_BOX, CtkMenuSectionBox))
#define CTK_MENU_SECTION_BOX_CLASS(class)                   (G_TYPE_CHECK_CLASS_CAST ((class),                       \
                                                             CTK_TYPE_MENU_SECTION_BOX, CtkMenuSectionBoxClass))
#define CTK_IS_MENU_SECTION_BOX(inst)                       (G_TYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             CTK_TYPE_MENU_SECTION_BOX))
#define CTK_IS_MENU_SECTION_BOX_CLASS(class)                (G_TYPE_CHECK_CLASS_TYPE ((class),                       \
                                                             CTK_TYPE_MENU_SECTION_BOX))
#define CTK_MENU_SECTION_BOX_GET_CLASS(inst)                (G_TYPE_INSTANCE_GET_CLASS ((inst),                      \
                                                             CTK_TYPE_MENU_SECTION_BOX, CtkMenuSectionBoxClass))

typedef struct _CtkMenuSectionBox                           CtkMenuSectionBox;

GType                   ctk_menu_section_box_get_type                   (void) G_GNUC_CONST;
void                    ctk_menu_section_box_new_toplevel               (CtkStack    *stack,
                                                                         GMenuModel  *model,
                                                                         const gchar *action_namespace,
                                                                         CtkPopover  *popover);

G_END_DECLS

#endif /* __CTK_MENU_SECTION_BOX_H__ */
