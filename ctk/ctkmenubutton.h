/* CTK - The GIMP Toolkit
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
#define CTK_MENU_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_MENU_BUTTON, CtkMenuButton))
#define CTK_MENU_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_MENU_BUTTON, CtkMenuButtonClass))
#define CTK_IS_MENU_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_MENU_BUTTON))
#define CTK_IS_MENU_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_MENU_BUTTON))
#define CTK_MENU_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_MENU_BUTTON, CtkMenuButtonClass))

typedef struct _CtkMenuButton        CtkMenuButton;
typedef struct _CtkMenuButtonClass   CtkMenuButtonClass;
typedef struct _CtkMenuButtonPrivate CtkMenuButtonPrivate;

struct _CtkMenuButton
{
  CtkToggleButton parent;

  /*< private >*/
  CtkMenuButtonPrivate *priv;
};

struct _CtkMenuButtonClass
{
  CtkToggleButtonClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

CDK_AVAILABLE_IN_3_6
GType        ctk_menu_button_get_type       (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_3_6
CtkWidget   *ctk_menu_button_new            (void);

CDK_AVAILABLE_IN_3_6
void         ctk_menu_button_set_popup      (CtkMenuButton *menu_button,
                                             CtkWidget     *menu);
CDK_AVAILABLE_IN_3_6
CtkMenu     *ctk_menu_button_get_popup      (CtkMenuButton *menu_button);

CDK_AVAILABLE_IN_3_12
void         ctk_menu_button_set_popover    (CtkMenuButton *menu_button,
                                             CtkWidget     *popover);
CDK_AVAILABLE_IN_3_12
CtkPopover  *ctk_menu_button_get_popover    (CtkMenuButton *menu_button);

CDK_AVAILABLE_IN_3_6
void         ctk_menu_button_set_direction  (CtkMenuButton *menu_button,
                                             CtkArrowType   direction);
CDK_AVAILABLE_IN_3_6
CtkArrowType ctk_menu_button_get_direction  (CtkMenuButton *menu_button);

CDK_AVAILABLE_IN_3_6
void         ctk_menu_button_set_menu_model (CtkMenuButton *menu_button,
                                             GMenuModel    *menu_model);
CDK_AVAILABLE_IN_3_6
GMenuModel  *ctk_menu_button_get_menu_model (CtkMenuButton *menu_button);

CDK_AVAILABLE_IN_3_6
void         ctk_menu_button_set_align_widget (CtkMenuButton *menu_button,
                                               CtkWidget     *align_widget);
CDK_AVAILABLE_IN_3_6
CtkWidget   *ctk_menu_button_get_align_widget (CtkMenuButton *menu_button);

CDK_AVAILABLE_IN_3_12
void         ctk_menu_button_set_use_popover (CtkMenuButton *menu_button,
                                              gboolean       use_popover);

CDK_AVAILABLE_IN_3_12
gboolean     ctk_menu_button_get_use_popover (CtkMenuButton *menu_button);


G_END_DECLS

#endif /* __CTK_MENU_BUTTON_H__ */
