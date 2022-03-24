/* ctkcombobox.h
 * Copyright (C) 2002, 2003  Kristian Rietveld <kris@gtk.org>
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

#ifndef __CTK_COMBO_BOX_H__
#define __CTK_COMBO_BOX_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkbin.h>
#include <ctk/ctktreemodel.h>
#include <ctk/ctktreeview.h>

G_BEGIN_DECLS

#define CTK_TYPE_COMBO_BOX             (ctk_combo_box_get_type ())
#define CTK_COMBO_BOX(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_COMBO_BOX, CtkComboBox))
#define CTK_COMBO_BOX_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), CTK_TYPE_COMBO_BOX, CtkComboBoxClass))
#define CTK_IS_COMBO_BOX(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_COMBO_BOX))
#define CTK_IS_COMBO_BOX_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), CTK_TYPE_COMBO_BOX))
#define CTK_COMBO_BOX_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), CTK_TYPE_COMBO_BOX, CtkComboBoxClass))

typedef struct _CtkComboBox        CtkComboBox;
typedef struct _CtkComboBoxClass   CtkComboBoxClass;
typedef struct _CtkComboBoxPrivate CtkComboBoxPrivate;

struct _CtkComboBox
{
  CtkBin parent_instance;

  /*< private >*/
  CtkComboBoxPrivate *priv;
};

/**
 * CtkComboBoxClass:
 * @parent_class: The parent class.
 * @changed: Signal is emitted when the active item is changed.
 * @format_entry_text: Signal which allows you to change how the text
 *    displayed in a combo box’s entry is displayed.
 */
struct _CtkComboBoxClass
{
  CtkBinClass parent_class;

  /*< public >*/

  /* signals */
  void     (* changed)           (CtkComboBox *combo_box);
  gchar   *(* format_entry_text) (CtkComboBox *combo_box,
                                  const gchar *path);

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
};


/* construction */
CDK_AVAILABLE_IN_ALL
GType         ctk_combo_box_get_type                 (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkWidget    *ctk_combo_box_new                      (void);
CDK_AVAILABLE_IN_ALL
CtkWidget    *ctk_combo_box_new_with_area            (CtkCellArea  *area);
CDK_AVAILABLE_IN_ALL
CtkWidget    *ctk_combo_box_new_with_area_and_entry  (CtkCellArea  *area);
CDK_AVAILABLE_IN_ALL
CtkWidget    *ctk_combo_box_new_with_entry           (void);
CDK_AVAILABLE_IN_ALL
CtkWidget    *ctk_combo_box_new_with_model           (CtkTreeModel *model);
CDK_AVAILABLE_IN_ALL
CtkWidget    *ctk_combo_box_new_with_model_and_entry (CtkTreeModel *model);

/* grids */
CDK_AVAILABLE_IN_ALL
gint          ctk_combo_box_get_wrap_width         (CtkComboBox *combo_box);
CDK_AVAILABLE_IN_ALL
void          ctk_combo_box_set_wrap_width         (CtkComboBox *combo_box,
                                                    gint         width);
CDK_AVAILABLE_IN_ALL
gint          ctk_combo_box_get_row_span_column    (CtkComboBox *combo_box);
CDK_AVAILABLE_IN_ALL
void          ctk_combo_box_set_row_span_column    (CtkComboBox *combo_box,
                                                    gint         row_span);
CDK_AVAILABLE_IN_ALL
gint          ctk_combo_box_get_column_span_column (CtkComboBox *combo_box);
CDK_AVAILABLE_IN_ALL
void          ctk_combo_box_set_column_span_column (CtkComboBox *combo_box,
                                                    gint         column_span);

CDK_DEPRECATED_IN_3_10
gboolean      ctk_combo_box_get_add_tearoffs       (CtkComboBox *combo_box);
CDK_DEPRECATED_IN_3_10
void          ctk_combo_box_set_add_tearoffs       (CtkComboBox *combo_box,
                                                    gboolean     add_tearoffs);

CDK_DEPRECATED_IN_3_10
const gchar * ctk_combo_box_get_title              (CtkComboBox *combo_box);
CDK_DEPRECATED_IN_3_10
void          ctk_combo_box_set_title              (CtkComboBox *combo_box,
                                                    const gchar *title);

CDK_DEPRECATED_IN_3_20_FOR(ctk_widget_get_focus_on_click)
gboolean      ctk_combo_box_get_focus_on_click     (CtkComboBox *combo);
CDK_DEPRECATED_IN_3_20_FOR(ctk_widget_set_focus_on_click)
void          ctk_combo_box_set_focus_on_click     (CtkComboBox *combo,
                                                    gboolean     focus_on_click);

/* get/set active item */
CDK_AVAILABLE_IN_ALL
gint          ctk_combo_box_get_active       (CtkComboBox     *combo_box);
CDK_AVAILABLE_IN_ALL
void          ctk_combo_box_set_active       (CtkComboBox     *combo_box,
                                              gint             index_);
CDK_AVAILABLE_IN_ALL
gboolean      ctk_combo_box_get_active_iter  (CtkComboBox     *combo_box,
                                              CtkTreeIter     *iter);
CDK_AVAILABLE_IN_ALL
void          ctk_combo_box_set_active_iter  (CtkComboBox     *combo_box,
                                              CtkTreeIter     *iter);

/* getters and setters */
CDK_AVAILABLE_IN_ALL
void          ctk_combo_box_set_model        (CtkComboBox     *combo_box,
                                              CtkTreeModel    *model);
CDK_AVAILABLE_IN_ALL
CtkTreeModel *ctk_combo_box_get_model        (CtkComboBox     *combo_box);

CDK_AVAILABLE_IN_ALL
CtkTreeViewRowSeparatorFunc ctk_combo_box_get_row_separator_func (CtkComboBox                *combo_box);
CDK_AVAILABLE_IN_ALL
void                        ctk_combo_box_set_row_separator_func (CtkComboBox                *combo_box,
                                                                  CtkTreeViewRowSeparatorFunc func,
                                                                  gpointer                    data,
                                                                  GDestroyNotify              destroy);

CDK_AVAILABLE_IN_ALL
void               ctk_combo_box_set_button_sensitivity (CtkComboBox        *combo_box,
                                                         CtkSensitivityType  sensitivity);
CDK_AVAILABLE_IN_ALL
CtkSensitivityType ctk_combo_box_get_button_sensitivity (CtkComboBox        *combo_box);

CDK_AVAILABLE_IN_ALL
gboolean           ctk_combo_box_get_has_entry          (CtkComboBox        *combo_box);
CDK_AVAILABLE_IN_ALL
void               ctk_combo_box_set_entry_text_column  (CtkComboBox        *combo_box,
                                                         gint                text_column);
CDK_AVAILABLE_IN_ALL
gint               ctk_combo_box_get_entry_text_column  (CtkComboBox        *combo_box);

CDK_AVAILABLE_IN_ALL
void               ctk_combo_box_set_popup_fixed_width  (CtkComboBox      *combo_box,
                                                         gboolean          fixed);
CDK_AVAILABLE_IN_ALL
gboolean           ctk_combo_box_get_popup_fixed_width  (CtkComboBox      *combo_box);

/* programmatic control */
CDK_AVAILABLE_IN_ALL
void          ctk_combo_box_popup            (CtkComboBox     *combo_box);
CDK_AVAILABLE_IN_ALL
void          ctk_combo_box_popup_for_device (CtkComboBox     *combo_box,
                                              CdkDevice       *device);
CDK_AVAILABLE_IN_ALL
void          ctk_combo_box_popdown          (CtkComboBox     *combo_box);
CDK_AVAILABLE_IN_ALL
AtkObject *   ctk_combo_box_get_popup_accessible (CtkComboBox *combo_box);

CDK_AVAILABLE_IN_ALL
gint          ctk_combo_box_get_id_column        (CtkComboBox *combo_box);
CDK_AVAILABLE_IN_ALL
void          ctk_combo_box_set_id_column        (CtkComboBox *combo_box,
                                                  gint         id_column);
CDK_AVAILABLE_IN_ALL
const gchar * ctk_combo_box_get_active_id        (CtkComboBox *combo_box);
CDK_AVAILABLE_IN_ALL
gboolean      ctk_combo_box_set_active_id        (CtkComboBox *combo_box,
                                                  const gchar *active_id);

G_END_DECLS

#endif /* __CTK_COMBO_BOX_H__ */
