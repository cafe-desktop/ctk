/* GTK - The GIMP Toolkit
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
#define CTK_MENU_TOOL_BUTTON(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_MENU_TOOL_BUTTON, GtkMenuToolButton))
#define CTK_MENU_TOOL_BUTTON_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), CTK_TYPE_MENU_TOOL_BUTTON, GtkMenuToolButtonClass))
#define CTK_IS_MENU_TOOL_BUTTON(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_MENU_TOOL_BUTTON))
#define CTK_IS_MENU_TOOL_BUTTON_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), CTK_TYPE_MENU_TOOL_BUTTON))
#define CTK_MENU_TOOL_BUTTON_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CTK_TYPE_MENU_TOOL_BUTTON, GtkMenuToolButtonClass))

typedef struct _GtkMenuToolButtonClass   GtkMenuToolButtonClass;
typedef struct _GtkMenuToolButton        GtkMenuToolButton;
typedef struct _GtkMenuToolButtonPrivate GtkMenuToolButtonPrivate;

struct _GtkMenuToolButton
{
  GtkToolButton parent;

  /*< private >*/
  GtkMenuToolButtonPrivate *priv;
};

/**
 * GtkMenuToolButtonClass:
 * @parent_class: The parent class.
 * @show_menu: Signal emitted before the menu is shown.
 */
struct _GtkMenuToolButtonClass
{
  GtkToolButtonClass parent_class;

  /*< public >*/

  void (*show_menu) (GtkMenuToolButton *button);

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
GtkToolItem  *ctk_menu_tool_button_new            (GtkWidget   *icon_widget,
                                                   const gchar *label);
GDK_DEPRECATED_IN_3_10_FOR(ctk_menu_tool_button_new)
GtkToolItem  *ctk_menu_tool_button_new_from_stock (const gchar *stock_id);

GDK_AVAILABLE_IN_ALL
void          ctk_menu_tool_button_set_menu       (GtkMenuToolButton *button,
                                                   GtkWidget         *menu);
GDK_AVAILABLE_IN_ALL
GtkWidget    *ctk_menu_tool_button_get_menu       (GtkMenuToolButton *button);
GDK_AVAILABLE_IN_ALL
void          ctk_menu_tool_button_set_arrow_tooltip_text   (GtkMenuToolButton *button,
							     const gchar       *text);
GDK_AVAILABLE_IN_ALL
void          ctk_menu_tool_button_set_arrow_tooltip_markup (GtkMenuToolButton *button,
							     const gchar       *markup);

G_END_DECLS

#endif /* __CTK_MENU_TOOL_BUTTON_H__ */
