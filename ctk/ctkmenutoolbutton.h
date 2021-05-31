/* CTK - The GIMP Toolkit
 *
 * Copyright (C) 2003 Ricardo Fernandez Pascual
 * Copyright (C) 2004 Paolo Borelli
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

#ifndef __CTK_MENU_TOOL_BUTTON_H__
#define __CTK_MENU_TOOL_BUTTON_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkmenu.h>
#include <ctk/ctktoolbutton.h>

G_BEGIN_DECLS

#define CTK_TYPE_MENU_TOOL_BUTTON         (ctk_menu_tool_button_get_type ())
#define CTK_MENU_TOOL_BUTTON(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_MENU_TOOL_BUTTON, CtkMenuToolButton))
#define CTK_MENU_TOOL_BUTTON_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), CTK_TYPE_MENU_TOOL_BUTTON, CtkMenuToolButtonClass))
#define CTK_IS_MENU_TOOL_BUTTON(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_MENU_TOOL_BUTTON))
#define CTK_IS_MENU_TOOL_BUTTON_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), CTK_TYPE_MENU_TOOL_BUTTON))
#define CTK_MENU_TOOL_BUTTON_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CTK_TYPE_MENU_TOOL_BUTTON, CtkMenuToolButtonClass))

typedef struct _CtkMenuToolButtonClass   CtkMenuToolButtonClass;
typedef struct _CtkMenuToolButton        CtkMenuToolButton;
typedef struct _CtkMenuToolButtonPrivate CtkMenuToolButtonPrivate;

struct _CtkMenuToolButton
{
  CtkToolButton parent;

  /*< private >*/
  CtkMenuToolButtonPrivate *priv;
};

/**
 * CtkMenuToolButtonClass:
 * @parent_class: The parent class.
 * @show_menu: Signal emitted before the menu is shown.
 */
struct _CtkMenuToolButtonClass
{
  CtkToolButtonClass parent_class;

  /*< public >*/

  void (*show_menu) (CtkMenuToolButton *button);

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
GType         ctk_menu_tool_button_get_type       (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
CtkToolItem  *ctk_menu_tool_button_new            (CtkWidget   *icon_widget,
                                                   const gchar *label);
GDK_DEPRECATED_IN_3_10_FOR(ctk_menu_tool_button_new)
CtkToolItem  *ctk_menu_tool_button_new_from_stock (const gchar *stock_id);

GDK_AVAILABLE_IN_ALL
void          ctk_menu_tool_button_set_menu       (CtkMenuToolButton *button,
                                                   CtkWidget         *menu);
GDK_AVAILABLE_IN_ALL
CtkWidget    *ctk_menu_tool_button_get_menu       (CtkMenuToolButton *button);
GDK_AVAILABLE_IN_ALL
void          ctk_menu_tool_button_set_arrow_tooltip_text   (CtkMenuToolButton *button,
							     const gchar       *text);
GDK_AVAILABLE_IN_ALL
void          ctk_menu_tool_button_set_arrow_tooltip_markup (CtkMenuToolButton *button,
							     const gchar       *markup);

G_END_DECLS

#endif /* __CTK_MENU_TOOL_BUTTON_H__ */
