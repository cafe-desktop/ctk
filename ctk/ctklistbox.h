/*
 * Copyright (c) 2013 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Alexander Larsson <alexl@redhat.com>
 *
 */

#ifndef __CTK_LIST_BOX_H__
#define __CTK_LIST_BOX_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkbin.h>

G_BEGIN_DECLS


#define CTK_TYPE_LIST_BOX (ctk_list_box_get_type ())
#define CTK_LIST_BOX(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_LIST_BOX, CtkListBox))
#define CTK_LIST_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_LIST_BOX, CtkListBoxClass))
#define CTK_IS_LIST_BOX(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_LIST_BOX))
#define CTK_IS_LIST_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_LIST_BOX))
#define CTK_LIST_BOX_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_LIST_BOX, CtkListBoxClass))

typedef struct _CtkListBox        CtkListBox;
typedef struct _CtkListBoxClass   CtkListBoxClass;

typedef struct _CtkListBoxRow        CtkListBoxRow;
typedef struct _CtkListBoxRowClass   CtkListBoxRowClass;

struct _CtkListBox
{
  CtkContainer parent_instance;
};

/**
 * CtkListBoxClass:
 * @parent_class: The parent class.
 * @row_selected: Class handler for the #CtkListBox::row-selected signal
 * @row_activated: Class handler for the #CtkListBox::row-activated signal
 * @activate_cursor_row: Class handler for the #CtkListBox::activate-cursor-row signal
 * @toggle_cursor_row: Class handler for the #CtkListBox::toggle-cursor-row signal
 * @move_cursor: Class handler for the #CtkListBox::move-cursor signal
 * @selected_rows_changed: Class handler for the #CtkListBox::selected-rows-changed signal
 * @select_all: Class handler for the #CtkListBox::select-all signal
 * @unselect_all: Class handler for the #CtkListBox::unselect-all signal
 */
struct _CtkListBoxClass
{
  CtkContainerClass parent_class;

  /*< public >*/

  void (*row_selected)        (CtkListBox      *box,
                               CtkListBoxRow   *row);
  void (*row_activated)       (CtkListBox      *box,
                               CtkListBoxRow   *row);
  void (*activate_cursor_row) (CtkListBox      *box);
  void (*toggle_cursor_row)   (CtkListBox      *box);
  void (*move_cursor)         (CtkListBox      *box,
                               CtkMovementStep  step,
                               gint             count);
  void (*selected_rows_changed) (CtkListBox    *box);
  void (*select_all)            (CtkListBox    *box);
  void (*unselect_all)          (CtkListBox    *box);

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
};

#define CTK_TYPE_LIST_BOX_ROW            (ctk_list_box_row_get_type ())
#define CTK_LIST_BOX_ROW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_LIST_BOX_ROW, CtkListBoxRow))
#define CTK_LIST_BOX_ROW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_LIST_BOX_ROW, CtkListBoxRowClass))
#define CTK_IS_LIST_BOX_ROW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_LIST_BOX_ROW))
#define CTK_IS_LIST_BOX_ROW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_LIST_BOX_ROW))
#define CTK_LIST_BOX_ROW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_LIST_BOX_ROW, CtkListBoxRowClass))

struct _CtkListBoxRow
{
  CtkBin parent_instance;
};

/**
 * CtkListBoxRowClass:
 * @parent_class: The parent class.
 * @activate: 
 */
struct _CtkListBoxRowClass
{
  CtkBinClass parent_class;

  /*< public >*/

  void (* activate) (CtkListBoxRow *row);

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
};

/**
 * CtkListBoxFilterFunc:
 * @row: the row that may be filtered
 * @user_data: (closure): user data
 *
 * Will be called whenever the row changes or is added and lets you control
 * if the row should be visible or not.
 *
 * Returns: %TRUE if the row should be visible, %FALSE otherwise
 *
 * Since: 3.10
 */
typedef gboolean (*CtkListBoxFilterFunc) (CtkListBoxRow *row,
                                          gpointer       user_data);

/**
 * CtkListBoxSortFunc:
 * @row1: the first row
 * @row2: the second row
 * @user_data: (closure): user data
 *
 * Compare two rows to determine which should be first.
 *
 * Returns: < 0 if @row1 should be before @row2, 0 if they are
 *     equal and > 0 otherwise
 *
 * Since: 3.10
 */
typedef gint (*CtkListBoxSortFunc) (CtkListBoxRow *row1,
                                    CtkListBoxRow *row2,
                                    gpointer       user_data);

/**
 * CtkListBoxUpdateHeaderFunc:
 * @row: the row to update
 * @before: (allow-none): the row before @row, or %NULL if it is first
 * @user_data: (closure): user data
 *
 * Whenever @row changes or which row is before @row changes this
 * is called, which lets you update the header on @row. You may
 * remove or set a new one via ctk_list_box_row_set_header() or
 * just change the state of the current header widget.
 *
 * Since: 3.10
 */
typedef void (*CtkListBoxUpdateHeaderFunc) (CtkListBoxRow *row,
                                            CtkListBoxRow *before,
                                            gpointer       user_data);

/**
 * CtkListBoxCreateWidgetFunc:
 * @item: (type GObject): the item from the model for which to create a widget for
 * @user_data: (closure): user data
 *
 * Called for list boxes that are bound to a #GListModel with
 * ctk_list_box_bind_model() for each item that gets added to the model.
 *
 * Versions of CTK+ prior to 3.18 called ctk_widget_show_all() on the rows
 * created by the CtkListBoxCreateWidgetFunc, but this forced all widgets
 * inside the row to be shown, and is no longer the case. Applications should
 * be updated to show the desired row widgets.
 *
 * Returns: (transfer full): a #CtkWidget that represents @item
 *
 * Since: 3.16
 */
typedef CtkWidget * (*CtkListBoxCreateWidgetFunc) (gpointer item,
                                                   gpointer user_data);

GDK_AVAILABLE_IN_3_10
GType      ctk_list_box_row_get_type      (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_3_10
CtkWidget* ctk_list_box_row_new           (void);
GDK_AVAILABLE_IN_3_10
CtkWidget* ctk_list_box_row_get_header    (CtkListBoxRow *row);
GDK_AVAILABLE_IN_3_10
void       ctk_list_box_row_set_header    (CtkListBoxRow *row,
                                           CtkWidget     *header);
GDK_AVAILABLE_IN_3_10
gint       ctk_list_box_row_get_index     (CtkListBoxRow *row);
GDK_AVAILABLE_IN_3_10
void       ctk_list_box_row_changed       (CtkListBoxRow *row);

GDK_AVAILABLE_IN_3_14
gboolean   ctk_list_box_row_is_selected   (CtkListBoxRow *row);

GDK_AVAILABLE_IN_3_14
void       ctk_list_box_row_set_selectable (CtkListBoxRow *row,
                                            gboolean       selectable);
GDK_AVAILABLE_IN_3_14
gboolean   ctk_list_box_row_get_selectable (CtkListBoxRow *row);


GDK_AVAILABLE_IN_3_14
void       ctk_list_box_row_set_activatable (CtkListBoxRow *row,
                                             gboolean       activatable);
GDK_AVAILABLE_IN_3_14
gboolean   ctk_list_box_row_get_activatable (CtkListBoxRow *row);

GDK_AVAILABLE_IN_3_10
GType          ctk_list_box_get_type                     (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_3_10
void           ctk_list_box_prepend                      (CtkListBox                    *box,
                                                          CtkWidget                     *child);
GDK_AVAILABLE_IN_3_10
void           ctk_list_box_insert                       (CtkListBox                    *box,
                                                          CtkWidget                     *child,
                                                          gint                           position);
GDK_AVAILABLE_IN_3_10
CtkListBoxRow* ctk_list_box_get_selected_row             (CtkListBox                    *box);
GDK_AVAILABLE_IN_3_10
CtkListBoxRow* ctk_list_box_get_row_at_index             (CtkListBox                    *box,
                                                          gint                           index_);
GDK_AVAILABLE_IN_3_10
CtkListBoxRow* ctk_list_box_get_row_at_y                 (CtkListBox                    *box,
                                                          gint                           y);
GDK_AVAILABLE_IN_3_10
void           ctk_list_box_select_row                   (CtkListBox                    *box,
                                                          CtkListBoxRow                 *row);
GDK_AVAILABLE_IN_3_10
void           ctk_list_box_set_placeholder              (CtkListBox                    *box,
                                                          CtkWidget                     *placeholder);
GDK_AVAILABLE_IN_3_10
void           ctk_list_box_set_adjustment               (CtkListBox                    *box,
                                                          CtkAdjustment                 *adjustment);
GDK_AVAILABLE_IN_3_10
CtkAdjustment *ctk_list_box_get_adjustment               (CtkListBox                    *box);

typedef void (* CtkListBoxForeachFunc) (CtkListBox      *box,
                                        CtkListBoxRow   *row,
                                        gpointer         user_data);

GDK_AVAILABLE_IN_3_14
void           ctk_list_box_selected_foreach             (CtkListBox                    *box,
                                                          CtkListBoxForeachFunc          func,
                                                          gpointer                       data);
GDK_AVAILABLE_IN_3_14
GList         *ctk_list_box_get_selected_rows            (CtkListBox                    *box);
GDK_AVAILABLE_IN_3_14
void           ctk_list_box_unselect_row                 (CtkListBox                    *box,
                                                          CtkListBoxRow                 *row);
GDK_AVAILABLE_IN_3_14
void           ctk_list_box_select_all                   (CtkListBox                    *box);
GDK_AVAILABLE_IN_3_14
void           ctk_list_box_unselect_all                 (CtkListBox                    *box);

GDK_AVAILABLE_IN_3_10
void           ctk_list_box_set_selection_mode           (CtkListBox                    *box,
                                                          CtkSelectionMode               mode);
GDK_AVAILABLE_IN_3_10
CtkSelectionMode ctk_list_box_get_selection_mode         (CtkListBox                    *box);
GDK_AVAILABLE_IN_3_10
void           ctk_list_box_set_filter_func              (CtkListBox                    *box,
                                                          CtkListBoxFilterFunc           filter_func,
                                                          gpointer                       user_data,
                                                          GDestroyNotify                 destroy);
GDK_AVAILABLE_IN_3_10
void           ctk_list_box_set_header_func              (CtkListBox                    *box,
                                                          CtkListBoxUpdateHeaderFunc     update_header,
                                                          gpointer                       user_data,
                                                          GDestroyNotify                 destroy);
GDK_AVAILABLE_IN_3_10
void           ctk_list_box_invalidate_filter            (CtkListBox                    *box);
GDK_AVAILABLE_IN_3_10
void           ctk_list_box_invalidate_sort              (CtkListBox                    *box);
GDK_AVAILABLE_IN_3_10
void           ctk_list_box_invalidate_headers           (CtkListBox                    *box);
GDK_AVAILABLE_IN_3_10
void           ctk_list_box_set_sort_func                (CtkListBox                    *box,
                                                          CtkListBoxSortFunc             sort_func,
                                                          gpointer                       user_data,
                                                          GDestroyNotify                 destroy);
GDK_AVAILABLE_IN_3_10
void           ctk_list_box_set_activate_on_single_click (CtkListBox                    *box,
                                                          gboolean                       single);
GDK_AVAILABLE_IN_3_10
gboolean       ctk_list_box_get_activate_on_single_click (CtkListBox                    *box);
GDK_AVAILABLE_IN_3_10
void           ctk_list_box_drag_unhighlight_row         (CtkListBox                    *box);
GDK_AVAILABLE_IN_3_10
void           ctk_list_box_drag_highlight_row           (CtkListBox                    *box,
                                                          CtkListBoxRow                 *row);
GDK_AVAILABLE_IN_3_10
CtkWidget*     ctk_list_box_new                          (void);


GDK_AVAILABLE_IN_3_16
void           ctk_list_box_bind_model                   (CtkListBox                   *box,
                                                          GListModel                   *model,
                                                          CtkListBoxCreateWidgetFunc    create_widget_func,
                                                          gpointer                      user_data,
                                                          GDestroyNotify                user_data_free_func);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkListBox, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkListBoxRow, g_object_unref)

G_END_DECLS

#endif
