/*
 * Copyright (C) 2010 Openismus GmbH
 * Copyright (C) 2013 Red Hat, Inc.
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.

 *
 * Authors:
 *      Tristan Van Berkom <tristanvb@openismus.com>
 *      Matthias Clasen <mclasen@redhat.com>
 *      William Jon McCann <jmccann@redhat.com>
 */

#ifndef __CTK_FLOW_BOX_H__
#define __CTK_FLOW_BOX_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkbin.h>

G_BEGIN_DECLS


#define CTK_TYPE_FLOW_BOX                  (ctk_flow_box_get_type ())
#define CTK_FLOW_BOX(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_FLOW_BOX, CtkFlowBox))
#define CTK_FLOW_BOX_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_FLOW_BOX, CtkFlowBoxClass))
#define CTK_IS_FLOW_BOX(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_FLOW_BOX))
#define CTK_IS_FLOW_BOX_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_FLOW_BOX))
#define CTK_FLOW_BOX_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_FLOW_BOX, CtkFlowBoxClass))

typedef struct _CtkFlowBox            CtkFlowBox;
typedef struct _CtkFlowBoxClass       CtkFlowBoxClass;

typedef struct _CtkFlowBoxChild       CtkFlowBoxChild;
typedef struct _CtkFlowBoxChildClass  CtkFlowBoxChildClass;

struct _CtkFlowBox
{
  CtkContainer container;
};

struct _CtkFlowBoxClass
{
  CtkContainerClass parent_class;

  void (*child_activated)            (CtkFlowBox        *box,
                                      CtkFlowBoxChild   *child);
  void (*selected_children_changed)  (CtkFlowBox        *box);
  void (*activate_cursor_child)      (CtkFlowBox        *box);
  void (*toggle_cursor_child)        (CtkFlowBox        *box);
  gboolean (*move_cursor)            (CtkFlowBox        *box,
                                      CtkMovementStep    step,
                                      gint               count);
  void (*select_all)                 (CtkFlowBox        *box);
  void (*unselect_all)               (CtkFlowBox        *box);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
  void (*_ctk_reserved5) (void);
  void (*_ctk_reserved6) (void);
};

#define CTK_TYPE_FLOW_BOX_CHILD            (ctk_flow_box_child_get_type ())
#define CTK_FLOW_BOX_CHILD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_FLOW_BOX_CHILD, CtkFlowBoxChild))
#define CTK_FLOW_BOX_CHILD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_FLOW_BOX_CHILD, CtkFlowBoxChildClass))
#define CTK_IS_FLOW_BOX_CHILD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_FLOW_BOX_CHILD))
#define CTK_IS_FLOW_BOX_CHILD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_FLOW_BOX_CHILD))
#define CTK_FLOW_BOX_CHILD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), EG_TYPE_FLOW_BOX_CHILD, CtkFlowBoxChildClass))

struct _CtkFlowBoxChild
{
  CtkBin parent_instance;
};

struct _CtkFlowBoxChildClass
{
  CtkBinClass parent_class;

  void (* activate) (CtkFlowBoxChild *child);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
};

/**
 * CtkFlowBoxCreateWidgetFunc:
 * @item: (type GObject): the item from the model for which to create a widget for
 * @user_data: (closure): user data from ctk_flow_box_bind_model()
 *
 * Called for flow boxes that are bound to a #GListModel with
 * ctk_flow_box_bind_model() for each item that gets added to the model.
 *
 * Returns: (transfer full): a #CtkWidget that represents @item
 *
 * Since: 3.18
 */
typedef CtkWidget * (*CtkFlowBoxCreateWidgetFunc) (gpointer item,
                                                   gpointer  user_data);

CDK_AVAILABLE_IN_3_12
GType                 ctk_flow_box_child_get_type            (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_3_12
CtkWidget*            ctk_flow_box_child_new                 (void);
CDK_AVAILABLE_IN_3_12
gint                  ctk_flow_box_child_get_index           (CtkFlowBoxChild *child);
CDK_AVAILABLE_IN_3_12
gboolean              ctk_flow_box_child_is_selected         (CtkFlowBoxChild *child);
CDK_AVAILABLE_IN_3_12
void                  ctk_flow_box_child_changed             (CtkFlowBoxChild *child);


CDK_AVAILABLE_IN_3_12
GType                 ctk_flow_box_get_type                  (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_3_12
CtkWidget            *ctk_flow_box_new                       (void);

CDK_AVAILABLE_IN_3_18
void                  ctk_flow_box_bind_model                (CtkFlowBox                 *box,
                                                              GListModel                 *model,
                                                              CtkFlowBoxCreateWidgetFunc  create_widget_func,
                                                              gpointer                    user_data,
                                                              GDestroyNotify              user_data_free_func);

CDK_AVAILABLE_IN_3_12
void                  ctk_flow_box_set_homogeneous           (CtkFlowBox           *box,
                                                              gboolean              homogeneous);
CDK_AVAILABLE_IN_3_12
gboolean              ctk_flow_box_get_homogeneous           (CtkFlowBox           *box);
CDK_AVAILABLE_IN_3_12
void                  ctk_flow_box_set_row_spacing           (CtkFlowBox           *box,
                                                              guint                 spacing);
CDK_AVAILABLE_IN_3_12
guint                 ctk_flow_box_get_row_spacing           (CtkFlowBox           *box);

CDK_AVAILABLE_IN_3_12
void                  ctk_flow_box_set_column_spacing        (CtkFlowBox           *box,
                                                              guint                 spacing);
CDK_AVAILABLE_IN_3_12
guint                 ctk_flow_box_get_column_spacing        (CtkFlowBox           *box);

CDK_AVAILABLE_IN_3_12
void                  ctk_flow_box_set_min_children_per_line (CtkFlowBox           *box,
                                                              guint                 n_children);
CDK_AVAILABLE_IN_3_12
guint                 ctk_flow_box_get_min_children_per_line (CtkFlowBox           *box);

CDK_AVAILABLE_IN_3_12
void                  ctk_flow_box_set_max_children_per_line (CtkFlowBox           *box,
                                                              guint                 n_children);
CDK_AVAILABLE_IN_3_12
guint                 ctk_flow_box_get_max_children_per_line (CtkFlowBox           *box);
CDK_AVAILABLE_IN_3_12
void                  ctk_flow_box_set_activate_on_single_click (CtkFlowBox        *box,
                                                                 gboolean           single);
CDK_AVAILABLE_IN_3_12
gboolean              ctk_flow_box_get_activate_on_single_click (CtkFlowBox        *box);

CDK_AVAILABLE_IN_3_12
void                  ctk_flow_box_insert                       (CtkFlowBox        *box,
                                                                 CtkWidget         *widget,
                                                                 gint               position);
CDK_AVAILABLE_IN_3_12
CtkFlowBoxChild      *ctk_flow_box_get_child_at_index           (CtkFlowBox        *box,
                                                                 gint               idx);

CDK_AVAILABLE_IN_3_22
CtkFlowBoxChild      *ctk_flow_box_get_child_at_pos             (CtkFlowBox        *box,
                                                                 gint               x,
                                                                 gint               y);

typedef void (* CtkFlowBoxForeachFunc) (CtkFlowBox      *box,
                                        CtkFlowBoxChild *child,
                                        gpointer         user_data);

CDK_AVAILABLE_IN_3_12
void                  ctk_flow_box_selected_foreach             (CtkFlowBox        *box,
                                                                 CtkFlowBoxForeachFunc func,
                                                                 gpointer           data);
CDK_AVAILABLE_IN_3_12
GList                *ctk_flow_box_get_selected_children        (CtkFlowBox        *box);
CDK_AVAILABLE_IN_3_12
void                  ctk_flow_box_select_child                 (CtkFlowBox        *box,
                                                                 CtkFlowBoxChild   *child);
CDK_AVAILABLE_IN_3_12
void                  ctk_flow_box_unselect_child               (CtkFlowBox        *box,
                                                                 CtkFlowBoxChild   *child);
CDK_AVAILABLE_IN_3_12
void                  ctk_flow_box_select_all                   (CtkFlowBox        *box);
CDK_AVAILABLE_IN_3_12
void                  ctk_flow_box_unselect_all                 (CtkFlowBox        *box);
CDK_AVAILABLE_IN_3_12
void                  ctk_flow_box_set_selection_mode           (CtkFlowBox        *box,
                                                                 CtkSelectionMode   mode);
CDK_AVAILABLE_IN_3_12
CtkSelectionMode      ctk_flow_box_get_selection_mode           (CtkFlowBox        *box);
CDK_AVAILABLE_IN_3_12
void                  ctk_flow_box_set_hadjustment              (CtkFlowBox        *box,
                                                                 CtkAdjustment     *adjustment);
CDK_AVAILABLE_IN_3_12
void                  ctk_flow_box_set_vadjustment              (CtkFlowBox        *box,
                                                                 CtkAdjustment     *adjustment);

typedef gboolean (*CtkFlowBoxFilterFunc) (CtkFlowBoxChild *child,
                                          gpointer         user_data);

CDK_AVAILABLE_IN_3_12
void                  ctk_flow_box_set_filter_func              (CtkFlowBox        *box,
                                                                 CtkFlowBoxFilterFunc filter_func,
                                                                 gpointer             user_data,
                                                                 GDestroyNotify       destroy);
CDK_AVAILABLE_IN_3_12
void                  ctk_flow_box_invalidate_filter            (CtkFlowBox        *box);

typedef gint (*CtkFlowBoxSortFunc) (CtkFlowBoxChild *child1,
                                    CtkFlowBoxChild *child2,
                                    gpointer         user_data);

CDK_AVAILABLE_IN_3_12
void                  ctk_flow_box_set_sort_func                (CtkFlowBox        *box,
                                                                 CtkFlowBoxSortFunc  sort_func,
                                                                 gpointer            user_data,
                                                                 GDestroyNotify      destroy);
CDK_AVAILABLE_IN_3_12
void                  ctk_flow_box_invalidate_sort              (CtkFlowBox         *box);

G_END_DECLS


#endif /* __CTK_FLOW_BOX_H__ */
