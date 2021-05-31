/* CtkToolPalette -- A tool palette with categories and DnD support
 * Copyright (C) 2008  Openismus GmbH
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *      Mathias Hasselmann
 */

#ifndef __CTK_TOOL_ITEM_GROUP_H__
#define __CTK_TOOL_ITEM_GROUP_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkcontainer.h>
#include <ctk/ctktoolitem.h>

G_BEGIN_DECLS

#define CTK_TYPE_TOOL_ITEM_GROUP           (ctk_tool_item_group_get_type ())
#define CTK_TOOL_ITEM_GROUP(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, CTK_TYPE_TOOL_ITEM_GROUP, CtkToolItemGroup))
#define CTK_TOOL_ITEM_GROUP_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, CTK_TYPE_TOOL_ITEM_GROUP, CtkToolItemGroupClass))
#define CTK_IS_TOOL_ITEM_GROUP(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, CTK_TYPE_TOOL_ITEM_GROUP))
#define CTK_IS_TOOL_ITEM_GROUP_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, CTK_TYPE_TOOL_ITEM_GROUP))
#define CTK_TOOL_ITEM_GROUP_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_TOOL_ITEM_GROUP, CtkToolItemGroupClass))

typedef struct _CtkToolItemGroup        CtkToolItemGroup;
typedef struct _CtkToolItemGroupClass   CtkToolItemGroupClass;
typedef struct _CtkToolItemGroupPrivate CtkToolItemGroupPrivate;

/**
 * CtkToolItemGroup:
 *
 * This should not be accessed directly. Use the accessor functions below.
 */
struct _CtkToolItemGroup
{
  CtkContainer parent_instance;
  CtkToolItemGroupPrivate *priv;
};

/**
 * CtkToolItemGroupClass:
 * @parent_class: The parent class.
 */
struct _CtkToolItemGroupClass
{
  CtkContainerClass parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
GType                 ctk_tool_item_group_get_type          (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
CtkWidget*            ctk_tool_item_group_new               (const gchar        *label);

GDK_AVAILABLE_IN_ALL
void                  ctk_tool_item_group_set_label         (CtkToolItemGroup   *group,
                                                             const gchar        *label);
GDK_AVAILABLE_IN_ALL
void                  ctk_tool_item_group_set_label_widget  (CtkToolItemGroup   *group,
                                                             CtkWidget          *label_widget);
GDK_AVAILABLE_IN_ALL
void                  ctk_tool_item_group_set_collapsed      (CtkToolItemGroup  *group,
                                                             gboolean            collapsed);
GDK_AVAILABLE_IN_ALL
void                  ctk_tool_item_group_set_ellipsize     (CtkToolItemGroup   *group,
                                                             PangoEllipsizeMode  ellipsize);
GDK_AVAILABLE_IN_ALL
void                  ctk_tool_item_group_set_header_relief (CtkToolItemGroup   *group,
                                                             CtkReliefStyle      style);

GDK_AVAILABLE_IN_ALL
const gchar *         ctk_tool_item_group_get_label         (CtkToolItemGroup   *group);
GDK_AVAILABLE_IN_ALL
CtkWidget            *ctk_tool_item_group_get_label_widget  (CtkToolItemGroup   *group);
GDK_AVAILABLE_IN_ALL
gboolean              ctk_tool_item_group_get_collapsed     (CtkToolItemGroup   *group);
GDK_AVAILABLE_IN_ALL
PangoEllipsizeMode    ctk_tool_item_group_get_ellipsize     (CtkToolItemGroup   *group);
GDK_AVAILABLE_IN_ALL
CtkReliefStyle        ctk_tool_item_group_get_header_relief (CtkToolItemGroup   *group);

GDK_AVAILABLE_IN_ALL
void                  ctk_tool_item_group_insert            (CtkToolItemGroup   *group,
                                                             CtkToolItem        *item,
                                                             gint                position);
GDK_AVAILABLE_IN_ALL
void                  ctk_tool_item_group_set_item_position (CtkToolItemGroup   *group,
                                                             CtkToolItem        *item,
                                                             gint                position);
GDK_AVAILABLE_IN_ALL
gint                  ctk_tool_item_group_get_item_position (CtkToolItemGroup   *group,
                                                             CtkToolItem        *item);

GDK_AVAILABLE_IN_ALL
guint                 ctk_tool_item_group_get_n_items       (CtkToolItemGroup   *group);
GDK_AVAILABLE_IN_ALL
CtkToolItem*          ctk_tool_item_group_get_nth_item      (CtkToolItemGroup   *group,
                                                             guint               index);
GDK_AVAILABLE_IN_ALL
CtkToolItem*          ctk_tool_item_group_get_drop_item     (CtkToolItemGroup   *group,
                                                             gint                x,
                                                             gint                y);

G_END_DECLS

#endif /* __CTK_TOOL_ITEM_GROUP_H__ */
