/* ctktoolbutton.h
 *
 * Copyright (C) 2002 Anders Carlsson <andersca@gnome.org>
 * Copyright (C) 2002 James Henstridge <james@daa.com.au>
 * Copyright (C) 2003 Soeren Sandmann <sandmann@daimi.au.dk>
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

#ifndef __CTK_TOOL_BUTTON_H__
#define __CTK_TOOL_BUTTON_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctktoolitem.h>

G_BEGIN_DECLS

#define CTK_TYPE_TOOL_BUTTON            (ctk_tool_button_get_type ())
#define CTK_TOOL_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_TOOL_BUTTON, CtkToolButton))
#define CTK_TOOL_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_TOOL_BUTTON, CtkToolButtonClass))
#define CTK_IS_TOOL_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_TOOL_BUTTON))
#define CTK_IS_TOOL_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_TOOL_BUTTON))
#define CTK_TOOL_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), CTK_TYPE_TOOL_BUTTON, CtkToolButtonClass))

typedef struct _CtkToolButton        CtkToolButton;
typedef struct _CtkToolButtonClass   CtkToolButtonClass;
typedef struct _CtkToolButtonPrivate CtkToolButtonPrivate;

struct _CtkToolButton
{
  CtkToolItem parent;

  /*< private >*/
  CtkToolButtonPrivate *priv;
};

/**
 * CtkToolButtonClass:
 * @parent_class: The parent class.
 * @button_type: 
 * @clicked: Signal emitted when the tool button is clicked with the
 *    mouse or activated with the keyboard.
 */
struct _CtkToolButtonClass
{
  CtkToolItemClass parent_class;

  GType button_type;

  /*< public >*/

  /* signal */
  void       (* clicked)             (CtkToolButton    *tool_item);

  /*< private >*/

  /* Padding for future expansion */
  void (* _ctk_reserved1) (void);
  void (* _ctk_reserved2) (void);
  void (* _ctk_reserved3) (void);
  void (* _ctk_reserved4) (void);
};

CDK_AVAILABLE_IN_ALL
GType        ctk_tool_button_get_type       (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkToolItem *ctk_tool_button_new            (CtkWidget   *icon_widget,
					     const gchar *label);
CDK_AVAILABLE_IN_ALL
CtkToolItem *ctk_tool_button_new_from_stock (const gchar *stock_id);

CDK_AVAILABLE_IN_ALL
void                  ctk_tool_button_set_label         (CtkToolButton *button,
							 const gchar   *label);
CDK_AVAILABLE_IN_ALL
const gchar *         ctk_tool_button_get_label         (CtkToolButton *button);
CDK_AVAILABLE_IN_ALL
void                  ctk_tool_button_set_use_underline (CtkToolButton *button,
							 gboolean       use_underline);
CDK_AVAILABLE_IN_ALL
gboolean              ctk_tool_button_get_use_underline (CtkToolButton *button);
CDK_AVAILABLE_IN_ALL
void                  ctk_tool_button_set_stock_id      (CtkToolButton *button,
							 const gchar   *stock_id);
CDK_AVAILABLE_IN_ALL
const gchar *         ctk_tool_button_get_stock_id      (CtkToolButton *button);
CDK_AVAILABLE_IN_ALL
void                  ctk_tool_button_set_icon_name     (CtkToolButton *button,
							 const gchar   *icon_name);
CDK_AVAILABLE_IN_ALL
const gchar *         ctk_tool_button_get_icon_name     (CtkToolButton *button);
CDK_AVAILABLE_IN_ALL
void                  ctk_tool_button_set_icon_widget   (CtkToolButton *button,
							 CtkWidget     *icon_widget);
CDK_AVAILABLE_IN_ALL
CtkWidget *           ctk_tool_button_get_icon_widget   (CtkToolButton *button);
CDK_AVAILABLE_IN_ALL
void                  ctk_tool_button_set_label_widget  (CtkToolButton *button,
							 CtkWidget     *label_widget);
CDK_AVAILABLE_IN_ALL
CtkWidget *           ctk_tool_button_get_label_widget  (CtkToolButton *button);


/* internal function */
CtkWidget *_ctk_tool_button_get_button (CtkToolButton *button);

G_END_DECLS

#endif /* __CTK_TOOL_BUTTON_H__ */
