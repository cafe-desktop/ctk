/* ctktoolitem.c
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

#ifndef __CTK_TOOL_ITEM_H__
#define __CTK_TOOL_ITEM_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkbin.h>
#include <ctk/ctkmenuitem.h>
#include <ctk/ctksizegroup.h>

G_BEGIN_DECLS

#define CTK_TYPE_TOOL_ITEM            (ctk_tool_item_get_type ())
#define CTK_TOOL_ITEM(o)              (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_TOOL_ITEM, CtkToolItem))
#define CTK_TOOL_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_TOOL_ITEM, CtkToolItemClass))
#define CTK_IS_TOOL_ITEM(o)           (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_TOOL_ITEM))
#define CTK_IS_TOOL_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_TOOL_ITEM))
#define CTK_TOOL_ITEM_GET_CLASS(o)    (G_TYPE_INSTANCE_GET_CLASS((o), CTK_TYPE_TOOL_ITEM, CtkToolItemClass))

typedef struct _CtkToolItem        CtkToolItem;
typedef struct _CtkToolItemClass   CtkToolItemClass;
typedef struct _CtkToolItemPrivate CtkToolItemPrivate;

struct _CtkToolItem
{
  CtkBin parent;

  /*< private >*/
  CtkToolItemPrivate *priv;
};

/**
 * CtkToolItemClass:
 * @parent_class: The parent class.
 * @create_menu_proxy: Signal emitted when the toolbar needs
 *    information from tool_item about whether the item should appear in
 *    the toolbar overflow menu.
 * @toolbar_reconfigured: Signal emitted when some property of the
 *    toolbar that the item is a child of changes.
 */
struct _CtkToolItemClass
{
  CtkBinClass parent_class;

  /* signals */
  gboolean   (* create_menu_proxy)    (CtkToolItem *tool_item);
  void       (* toolbar_reconfigured) (CtkToolItem *tool_item);

  /*< private >*/

  /* Padding for future expansion */
  void (* _ctk_reserved1) (void);
  void (* _ctk_reserved2) (void);
  void (* _ctk_reserved3) (void);
  void (* _ctk_reserved4) (void);
};


CDK_AVAILABLE_IN_ALL
GType        ctk_tool_item_get_type (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkToolItem *ctk_tool_item_new      (void);

CDK_AVAILABLE_IN_ALL
void            ctk_tool_item_set_homogeneous          (CtkToolItem *tool_item,
							gboolean     homogeneous);
CDK_AVAILABLE_IN_ALL
gboolean        ctk_tool_item_get_homogeneous          (CtkToolItem *tool_item);

CDK_AVAILABLE_IN_ALL
void            ctk_tool_item_set_expand               (CtkToolItem *tool_item,
							gboolean     expand);
CDK_AVAILABLE_IN_ALL
gboolean        ctk_tool_item_get_expand               (CtkToolItem *tool_item);
CDK_AVAILABLE_IN_ALL
void            ctk_tool_item_set_tooltip_text         (CtkToolItem *tool_item,
							const gchar *text);
CDK_AVAILABLE_IN_ALL
void            ctk_tool_item_set_tooltip_markup       (CtkToolItem *tool_item,
							const gchar *markup);

CDK_AVAILABLE_IN_ALL
void            ctk_tool_item_set_use_drag_window      (CtkToolItem *tool_item,
							gboolean     use_drag_window);
CDK_AVAILABLE_IN_ALL
gboolean        ctk_tool_item_get_use_drag_window      (CtkToolItem *tool_item);

CDK_AVAILABLE_IN_ALL
void            ctk_tool_item_set_visible_horizontal   (CtkToolItem *tool_item,
							gboolean     visible_horizontal);
CDK_AVAILABLE_IN_ALL
gboolean        ctk_tool_item_get_visible_horizontal   (CtkToolItem *tool_item);

CDK_AVAILABLE_IN_ALL
void            ctk_tool_item_set_visible_vertical     (CtkToolItem *tool_item,
							gboolean     visible_vertical);
CDK_AVAILABLE_IN_ALL
gboolean        ctk_tool_item_get_visible_vertical     (CtkToolItem *tool_item);

CDK_AVAILABLE_IN_ALL
gboolean        ctk_tool_item_get_is_important         (CtkToolItem *tool_item);
CDK_AVAILABLE_IN_ALL
void            ctk_tool_item_set_is_important         (CtkToolItem *tool_item,
							gboolean     is_important);

CDK_AVAILABLE_IN_ALL
PangoEllipsizeMode ctk_tool_item_get_ellipsize_mode    (CtkToolItem *tool_item);
CDK_AVAILABLE_IN_ALL
CtkIconSize     ctk_tool_item_get_icon_size            (CtkToolItem *tool_item);
CDK_AVAILABLE_IN_ALL
CtkOrientation  ctk_tool_item_get_orientation          (CtkToolItem *tool_item);
CDK_AVAILABLE_IN_ALL
CtkToolbarStyle ctk_tool_item_get_toolbar_style        (CtkToolItem *tool_item);
CDK_AVAILABLE_IN_ALL
CtkReliefStyle  ctk_tool_item_get_relief_style         (CtkToolItem *tool_item);
CDK_AVAILABLE_IN_ALL
gfloat          ctk_tool_item_get_text_alignment       (CtkToolItem *tool_item);
CDK_AVAILABLE_IN_ALL
CtkOrientation  ctk_tool_item_get_text_orientation     (CtkToolItem *tool_item);
CDK_AVAILABLE_IN_ALL
CtkSizeGroup *  ctk_tool_item_get_text_size_group      (CtkToolItem *tool_item);

CDK_AVAILABLE_IN_ALL
CtkWidget *     ctk_tool_item_retrieve_proxy_menu_item (CtkToolItem *tool_item);
CDK_AVAILABLE_IN_ALL
CtkWidget *     ctk_tool_item_get_proxy_menu_item      (CtkToolItem *tool_item,
							const gchar *menu_item_id);
CDK_AVAILABLE_IN_ALL
void            ctk_tool_item_set_proxy_menu_item      (CtkToolItem *tool_item,
							const gchar *menu_item_id,
							CtkWidget   *menu_item);
CDK_AVAILABLE_IN_ALL
void		ctk_tool_item_rebuild_menu	       (CtkToolItem *tool_item);

CDK_AVAILABLE_IN_ALL
void            ctk_tool_item_toolbar_reconfigured     (CtkToolItem *tool_item);

/* private */

gboolean       _ctk_tool_item_create_menu_proxy        (CtkToolItem *tool_item);

G_END_DECLS

#endif /* __CTK_TOOL_ITEM_H__ */
