/* ctktoggletoolbutton.h
 *
 * Copyright (C) 2002 Anders Carlsson <andersca@gnome.org>
 * Copyright (C) 2002 James Henstridge <james@daa.com.au>
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

#ifndef __CTK_SEPARATOR_TOOL_ITEM_H__
#define __CTK_SEPARATOR_TOOL_ITEM_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctktoolitem.h>

G_BEGIN_DECLS

#define CTK_TYPE_SEPARATOR_TOOL_ITEM            (ctk_separator_tool_item_get_type ())
#define CTK_SEPARATOR_TOOL_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_SEPARATOR_TOOL_ITEM, CtkSeparatorToolItem))
#define CTK_SEPARATOR_TOOL_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_SEPARATOR_TOOL_ITEM, CtkSeparatorToolItemClass))
#define CTK_IS_SEPARATOR_TOOL_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_SEPARATOR_TOOL_ITEM))
#define CTK_IS_SEPARATOR_TOOL_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_SEPARATOR_TOOL_ITEM))
#define CTK_SEPARATOR_TOOL_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), CTK_TYPE_SEPARATOR_TOOL_ITEM, CtkSeparatorToolItemClass))

typedef struct _CtkSeparatorToolItem        CtkSeparatorToolItem;
typedef struct _CtkSeparatorToolItemClass   CtkSeparatorToolItemClass;
typedef struct _CtkSeparatorToolItemPrivate CtkSeparatorToolItemPrivate;

struct _CtkSeparatorToolItem
{
  CtkToolItem parent;

  /*< private >*/
  CtkSeparatorToolItemPrivate *priv;
};

/**
 * CtkSeparatorToolItemClass:
 * @parent_class: The parent class.
 */
struct _CtkSeparatorToolItemClass
{
  CtkToolItemClass parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (* _ctk_reserved1) (void);
  void (* _ctk_reserved2) (void);
  void (* _ctk_reserved3) (void);
  void (* _ctk_reserved4) (void);
};

CDK_AVAILABLE_IN_ALL
GType        ctk_separator_tool_item_get_type (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkToolItem *ctk_separator_tool_item_new      (void);

CDK_AVAILABLE_IN_ALL
gboolean     ctk_separator_tool_item_get_draw (CtkSeparatorToolItem *item);
CDK_AVAILABLE_IN_ALL
void         ctk_separator_tool_item_set_draw (CtkSeparatorToolItem *item,
					       gboolean              draw);

G_END_DECLS

#endif /* __CTK_SEPARATOR_TOOL_ITEM_H__ */
