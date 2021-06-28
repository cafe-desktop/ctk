/* ctkiconview.h
 * Copyright (C) 2002, 2004  Anders Carlsson <andersca@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ctk/ctkiconview.h"
#include "ctk/ctkcssnodeprivate.h"

#ifndef __CTK_ICON_VIEW_PRIVATE_H__
#define __CTK_ICON_VIEW_PRIVATE_H__

typedef struct _CtkIconViewItem CtkIconViewItem;
struct _CtkIconViewItem
{
  CdkRectangle cell_area;

  gint index;
  
  gint row, col;

  guint selected : 1;
  guint selected_before_rubberbanding : 1;

};

struct _CtkIconViewPrivate
{
  CtkCellArea        *cell_area;
  CtkCellAreaContext *cell_area_context;

  gulong              add_editable_id;
  gulong              remove_editable_id;
  gulong              context_changed_id;

  GPtrArray          *row_contexts;

  gint width, height;

  CtkSelectionMode selection_mode;

  CdkWindow *bin_window;

  GList *children;

  CtkTreeModel *model;

  GList *items;

  CtkAdjustment *hadjustment;
  CtkAdjustment *vadjustment;

  gint rubberband_x1, rubberband_y1;
  gint rubberband_x2, rubberband_y2;
  CdkDevice *rubberband_device;
  CtkCssNode *rubberband_node;

  guint scroll_timeout_id;
  gint scroll_value_diff;
  gint event_last_x, event_last_y;

  CtkIconViewItem *anchor_item;
  CtkIconViewItem *cursor_item;

  CtkIconViewItem *last_single_clicked;
  CtkIconViewItem *last_prelight;

  CtkOrientation item_orientation;

  gint columns;
  gint item_width;
  gint spacing;
  gint row_spacing;
  gint column_spacing;
  gint margin;
  gint item_padding;

  gint text_column;
  gint markup_column;
  gint pixbuf_column;
  gint tooltip_column;

  CtkCellRenderer *pixbuf_cell;
  CtkCellRenderer *text_cell;

  /* Drag-and-drop. */
  CdkModifierType start_button_mask;
  gint pressed_button;
  gint press_start_x;
  gint press_start_y;

  CdkDragAction source_actions;
  CdkDragAction dest_actions;

  CtkTreeRowReference *dest_item;
  CtkIconViewDropPosition dest_pos;

  /* scroll to */
  CtkTreeRowReference *scroll_to_path;
  gfloat scroll_to_row_align;
  gfloat scroll_to_col_align;
  guint scroll_to_use_align : 1;

  guint source_set : 1;
  guint dest_set : 1;
  guint reorderable : 1;
  guint empty_view_drop :1;
  guint activate_on_single_click : 1;

  guint modify_selection_pressed : 1;
  guint extend_selection_pressed : 1;

  guint draw_focus : 1;

  /* CtkScrollablePolicy needs to be checked when
   * driving the scrollable adjustment values */
  guint hscroll_policy : 1;
  guint vscroll_policy : 1;

  guint doing_rubberband : 1;

};

void                 _ctk_icon_view_set_cell_data                  (CtkIconView            *icon_view,
                                                                    CtkIconViewItem        *item);
void                 _ctk_icon_view_set_cursor_item                (CtkIconView            *icon_view,
                                                                    CtkIconViewItem        *item,
                                                                    CtkCellRenderer        *cursor_cell);
CtkIconViewItem *    _ctk_icon_view_get_item_at_coords             (CtkIconView            *icon_view,
                                                                    gint                    x,
                                                                    gint                    y,
                                                                    gboolean                only_in_cell,
                                                                    CtkCellRenderer       **cell_at_pos);
void                 _ctk_icon_view_select_item                    (CtkIconView            *icon_view,
                                                                    CtkIconViewItem        *item);
void                 _ctk_icon_view_unselect_item                  (CtkIconView            *icon_view,
                                                                    CtkIconViewItem        *item);

G_END_DECLS

#endif /* __CTK_ICON_VIEW_PRIVATE_H__ */
