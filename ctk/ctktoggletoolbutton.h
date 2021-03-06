/* ctktoggletoolbutton.h
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

#ifndef __CTK_TOGGLE_TOOL_BUTTON_H__
#define __CTK_TOGGLE_TOOL_BUTTON_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctktoolbutton.h>

G_BEGIN_DECLS

#define CTK_TYPE_TOGGLE_TOOL_BUTTON             (ctk_toggle_tool_button_get_type ())
#define CTK_TOGGLE_TOOL_BUTTON(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_TOGGLE_TOOL_BUTTON, CtkToggleToolButton))
#define CTK_TOGGLE_TOOL_BUTTON_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_TOGGLE_TOOL_BUTTON, CtkToggleToolButtonClass))
#define CTK_IS_TOGGLE_TOOL_BUTTON(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_TOGGLE_TOOL_BUTTON))
#define CTK_IS_TOGGLE_TOOL_BUTTON_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_TOGGLE_TOOL_BUTTON))
#define CTK_TOGGLE_TOOL_BUTTON_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), CTK_TYPE_TOGGLE_TOOL_BUTTON, CtkToggleToolButtonClass))

typedef struct _CtkToggleToolButton        CtkToggleToolButton;
typedef struct _CtkToggleToolButtonClass   CtkToggleToolButtonClass;
typedef struct _CtkToggleToolButtonPrivate CtkToggleToolButtonPrivate;

struct _CtkToggleToolButton
{
  CtkToolButton parent;

  /*< private >*/
  CtkToggleToolButtonPrivate *priv;
};

/**
 * CtkToggleToolButtonClass:
 * @parent_class: The parent class.
 * @toggled: Signal emitted whenever the toggle tool button changes state.
 */
struct _CtkToggleToolButtonClass
{
  CtkToolButtonClass parent_class;

  /*< public >*/

  /* signal */
  void (* toggled) (CtkToggleToolButton *button);

  /*< private >*/

  /* Padding for future expansion */
  void (* _ctk_reserved1) (void);
  void (* _ctk_reserved2) (void);
  void (* _ctk_reserved3) (void);
  void (* _ctk_reserved4) (void);
};

CDK_AVAILABLE_IN_ALL
GType        ctk_toggle_tool_button_get_type       (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkToolItem *ctk_toggle_tool_button_new            (void);
CDK_DEPRECATED_IN_3_10_FOR(ctk_toggle_tool_button_new)
CtkToolItem *ctk_toggle_tool_button_new_from_stock (const gchar *stock_id);

CDK_AVAILABLE_IN_ALL
void         ctk_toggle_tool_button_set_active     (CtkToggleToolButton *button,
						    gboolean             is_active);
CDK_AVAILABLE_IN_ALL
gboolean     ctk_toggle_tool_button_get_active     (CtkToggleToolButton *button);

G_END_DECLS

#endif /* __CTK_TOGGLE_TOOL_BUTTON_H__ */
