/* GTK - The GIMP Toolkit
 *
 * Copyright (C) 2003 Ricardo Fernandez Pascual
 * Copyright (C) 2004 Paolo Borelli
 * Copyright (C) 2012 Bastien Nocera
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

#ifndef __CTK_MENU_BUTTON_H__
#define __CTK_MENU_BUTTON_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctktogglebutton.h>
#include <ctk/ctkmenu.h>
#include <ctk/ctkpopover.h>

G_BEGIN_DECLS

#define CTK_TYPE_MENU_BUTTON            (ctk_menu_button_get_type ())
#define CTK_MENU_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_MENU_BUTTON, GtkMenuButton))
#define CTK_MENU_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_MENU_BUTTON, GtkMenuButtonClass))
#define CTK_IS_MENU_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_MENU_BUTTON))
#define CTK_IS_MENU_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_MENU_BUTTON))
#define CTK_MENU_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_MENU_BUTTON, GtkMenuButtonClass))

typedef struct _GtkMenuButton        GtkMenuButton;
typedef struct _GtkMenuButtonClass   GtkMenuButtonClass;
typedef struct _GtkMenuButtonPrivate GtkMenuButtonPrivate;

struct _GtkMenuButton
{
  GtkToggleButton parent;

  /*< private >*/
  GtkMenuButtonPrivate *priv;
};

struct _GtkMenuButtonClass
{
  GtkToggleButtonClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_3_6
GType        ctk_menu_button_get_type       (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_3_6
GtkWidget   *ctk_menu_button_new            (void);

GDK_AVAILABLE_IN_3_6
void         ctk_menu_button_set_popup      (GtkMenuButton *menu_button,
                                             GtkWidget     *menu);
GDK_AVAILABLE_IN_3_6
GtkMenu     *ctk_menu_button_get_popup      (GtkMenuButton *menu_button);

GDK_AVAILABLE_IN_3_12
void         ctk_menu_button_set_popover    (GtkMenuButton *menu_button,
                                             GtkWidget     *popover);
GDK_AVAILABLE_IN_3_12
GtkPopover  *ctk_menu_button_get_popover    (GtkMenuButton *menu_button);

GDK_AVAILABLE_IN_3_6
void         ctk_menu_button_set_direction  (GtkMenuButton *menu_button,
                                             GtkArrowType   direction);
GDK_AVAILABLE_IN_3_6
GtkArrowType ctk_menu_button_get_direction  (GtkMenuButton *menu_button);

GDK_AVAILABLE_IN_3_6
void         ctk_menu_button_set_menu_model (GtkMenuButton *menu_button,
                                             GMenuModel    *menu_model);
GDK_AVAILABLE_IN_3_6
GMenuModel  *ctk_menu_button_get_menu_model (GtkMenuButton *menu_button);

GDK_AVAILABLE_IN_3_6
void         ctk_menu_button_set_align_widget (GtkMenuButton *menu_button,
                                               GtkWidget     *align_widget);
GDK_AVAILABLE_IN_3_6
GtkWidget   *ctk_menu_button_get_align_widget (GtkMenuButton *menu_button);

GDK_AVAILABLE_IN_3_12
void         ctk_menu_button_set_use_popover (GtkMenuButton *menu_button,
                                              gboolean       use_popover);

GDK_AVAILABLE_IN_3_12
gboolean     ctk_menu_button_get_use_popover (GtkMenuButton *menu_button);


G_END_DECLS

#endif /* __CTK_MENU_BUTTON_H__ */
