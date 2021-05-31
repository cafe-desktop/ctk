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

#ifndef __CTK_TOOL_PALETTE_H__
#define __CTK_TOOL_PALETTE_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkcontainer.h>
#include <ctk/ctkdnd.h>
#include <ctk/ctktoolitem.h>

G_BEGIN_DECLS

#define CTK_TYPE_TOOL_PALETTE           (ctk_tool_palette_get_type ())
#define CTK_TOOL_PALETTE(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, CTK_TYPE_TOOL_PALETTE, CtkToolPalette))
#define CTK_TOOL_PALETTE_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, CTK_TYPE_TOOL_PALETTE, CtkToolPaletteClass))
#define CTK_IS_TOOL_PALETTE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, CTK_TYPE_TOOL_PALETTE))
#define CTK_IS_TOOL_PALETTE_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, CTK_TYPE_TOOL_PALETTE))
#define CTK_TOOL_PALETTE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_TOOL_PALETTE, CtkToolPaletteClass))

typedef struct _CtkToolPalette           CtkToolPalette;
typedef struct _CtkToolPaletteClass      CtkToolPaletteClass;
typedef struct _CtkToolPalettePrivate    CtkToolPalettePrivate;

/**
 * CtkToolPaletteDragTargets:
 * @CTK_TOOL_PALETTE_DRAG_ITEMS: Support drag of items.
 * @CTK_TOOL_PALETTE_DRAG_GROUPS: Support drag of groups.
 *
 * Flags used to specify the supported drag targets.
 */
typedef enum /*< flags >*/
{
  CTK_TOOL_PALETTE_DRAG_ITEMS = (1 << 0),
  CTK_TOOL_PALETTE_DRAG_GROUPS = (1 << 1)
}
CtkToolPaletteDragTargets;

/**
 * CtkToolPalette:
 *
 * This should not be accessed directly. Use the accessor functions below.
 */
struct _CtkToolPalette
{
  CtkContainer parent_instance;
  CtkToolPalettePrivate *priv;
};

/**
 * CtkToolPaletteClass:
 * @parent_class: The parent class.
 */
struct _CtkToolPaletteClass
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
GType                          ctk_tool_palette_get_type              (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
CtkWidget*                     ctk_tool_palette_new                   (void);

GDK_AVAILABLE_IN_ALL
void                           ctk_tool_palette_set_group_position    (CtkToolPalette            *palette,
                                                                       CtkToolItemGroup          *group,
                                                                       gint                       position);
GDK_AVAILABLE_IN_ALL
void                           ctk_tool_palette_set_exclusive         (CtkToolPalette            *palette,
                                                                       CtkToolItemGroup          *group,
                                                                       gboolean                   exclusive);
GDK_AVAILABLE_IN_ALL
void                           ctk_tool_palette_set_expand            (CtkToolPalette            *palette,
                                                                       CtkToolItemGroup          *group,
                                                                       gboolean                   expand);

GDK_AVAILABLE_IN_ALL
gint                           ctk_tool_palette_get_group_position    (CtkToolPalette            *palette,
                                                                       CtkToolItemGroup          *group);
GDK_AVAILABLE_IN_ALL
gboolean                       ctk_tool_palette_get_exclusive         (CtkToolPalette            *palette,
                                                                       CtkToolItemGroup          *group);
GDK_AVAILABLE_IN_ALL
gboolean                       ctk_tool_palette_get_expand            (CtkToolPalette            *palette,
                                                                       CtkToolItemGroup          *group);

GDK_AVAILABLE_IN_ALL
void                           ctk_tool_palette_set_icon_size         (CtkToolPalette            *palette,
                                                                       CtkIconSize                icon_size);
GDK_AVAILABLE_IN_ALL
void                           ctk_tool_palette_unset_icon_size       (CtkToolPalette            *palette);
GDK_AVAILABLE_IN_ALL
void                           ctk_tool_palette_set_style             (CtkToolPalette            *palette,
                                                                       CtkToolbarStyle            style);
GDK_AVAILABLE_IN_ALL
void                           ctk_tool_palette_unset_style           (CtkToolPalette            *palette);

GDK_AVAILABLE_IN_ALL
CtkIconSize                    ctk_tool_palette_get_icon_size         (CtkToolPalette            *palette);
GDK_AVAILABLE_IN_ALL
CtkToolbarStyle                ctk_tool_palette_get_style             (CtkToolPalette            *palette);

GDK_AVAILABLE_IN_ALL
CtkToolItem*                   ctk_tool_palette_get_drop_item         (CtkToolPalette            *palette,
                                                                       gint                       x,
                                                                       gint                       y);
GDK_AVAILABLE_IN_ALL
CtkToolItemGroup*              ctk_tool_palette_get_drop_group        (CtkToolPalette            *palette,
                                                                       gint                       x,
                                                                       gint                       y);
GDK_AVAILABLE_IN_ALL
CtkWidget*                     ctk_tool_palette_get_drag_item         (CtkToolPalette            *palette,
                                                                       const CtkSelectionData    *selection);

GDK_AVAILABLE_IN_ALL
void                           ctk_tool_palette_set_drag_source       (CtkToolPalette            *palette,
                                                                       CtkToolPaletteDragTargets  targets);
GDK_AVAILABLE_IN_ALL
void                           ctk_tool_palette_add_drag_dest         (CtkToolPalette            *palette,
                                                                       CtkWidget                 *widget,
                                                                       CtkDestDefaults            flags,
                                                                       CtkToolPaletteDragTargets  targets,
                                                                       GdkDragAction              actions);


GDK_DEPRECATED_IN_3_0_FOR(ctk_scrollable_get_hadjustment)
CtkAdjustment*                 ctk_tool_palette_get_hadjustment       (CtkToolPalette            *palette);
GDK_DEPRECATED_IN_3_0_FOR(ctk_scrollable_get_vadjustment)
CtkAdjustment*                 ctk_tool_palette_get_vadjustment       (CtkToolPalette            *palette);

GDK_AVAILABLE_IN_ALL
const CtkTargetEntry*          ctk_tool_palette_get_drag_target_item  (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
const CtkTargetEntry*          ctk_tool_palette_get_drag_target_group (void) G_GNUC_CONST;


G_END_DECLS

#endif /* __CTK_TOOL_PALETTE_H__ */
