/* GTK - The GIMP Toolkit
 * Copyright © 2014 Red Hat, Inc.
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

#ifndef __CTK_POPOVER_MENU_H__
#define __CTK_POPOVER_MENU_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkpopover.h>

G_BEGIN_DECLS

#define CTK_TYPE_POPOVER_MENU           (ctk_popover_menu_get_type ())
#define CTK_POPOVER_MENU(o)             (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_POPOVER_MENU, CtkPopoverMenu))
#define CTK_POPOVER_MENU_CLASS(c)       (G_TYPE_CHECK_CLASS_CAST ((c), CTK_TYPE_POPOVER_MENU, CtkPopoverMenuClass))
#define CTK_IS_POPOVER_MENU(o)          (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_POPOVER_MENU))
#define CTK_IS_POPOVER_MENU_CLASS(o)    (G_TYPE_CHECK_CLASS_TYPE ((o), CTK_TYPE_POPOVER_MENU))
#define CTK_POPOVER_MENU_GET_CLASS(o)   (G_TYPE_INSTANCE_GET_CLASS ((o), CTK_TYPE_POPOVER_MENU, CtkPopoverMenuClass))

typedef struct _CtkPopoverMenu CtkPopoverMenu;
typedef struct _CtkPopoverMenuClass CtkPopoverMenuClass;

struct _CtkPopoverMenuClass
{
  CtkPopoverClass parent_class;

  /*< private >*/

  /* Padding for future expansion */
  gpointer reserved[10];
};

GDK_AVAILABLE_IN_3_16
GType       ctk_popover_menu_get_type (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_3_16
CtkWidget * ctk_popover_menu_new      (void);

GDK_AVAILABLE_IN_3_16
void        ctk_popover_menu_open_submenu (CtkPopoverMenu *popover,
                                           const gchar    *name);


G_END_DECLS

#endif /* __CTK_POPOVER_MENU_H__ */
