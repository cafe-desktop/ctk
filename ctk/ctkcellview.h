/* ctkcellview.h
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

#ifndef __CTK_CELL_VIEW_H__
#define __CTK_CELL_VIEW_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkwidget.h>
#include <ctk/ctkcellrenderer.h>
#include <ctk/ctkcellarea.h>
#include <ctk/ctkcellareacontext.h>
#include <ctk/ctktreemodel.h>

G_BEGIN_DECLS

#define CTK_TYPE_CELL_VIEW                (ctk_cell_view_get_type ())
#define CTK_CELL_VIEW(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_CELL_VIEW, CtkCellView))
#define CTK_CELL_VIEW_CLASS(vtable)       (G_TYPE_CHECK_CLASS_CAST ((vtable), CTK_TYPE_CELL_VIEW, CtkCellViewClass))
#define CTK_IS_CELL_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_CELL_VIEW))
#define CTK_IS_CELL_VIEW_CLASS(vtable)    (G_TYPE_CHECK_CLASS_TYPE ((vtable), CTK_TYPE_CELL_VIEW))
#define CTK_CELL_VIEW_GET_CLASS(inst)     (G_TYPE_INSTANCE_GET_CLASS ((inst), CTK_TYPE_CELL_VIEW, CtkCellViewClass))

typedef struct _CtkCellView             CtkCellView;
typedef struct _CtkCellViewClass        CtkCellViewClass;
typedef struct _CtkCellViewPrivate      CtkCellViewPrivate;

struct _CtkCellView
{
  CtkWidget parent_instance;

  /*< private >*/
  CtkCellViewPrivate *priv;
};

/**
 * CtkCellViewClass:
 * @parent_class: The parent class.
 */
struct _CtkCellViewClass
{
  CtkWidgetClass parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

CDK_AVAILABLE_IN_ALL
GType             ctk_cell_view_get_type                (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkWidget        *ctk_cell_view_new                     (void);
CDK_AVAILABLE_IN_ALL
CtkWidget        *ctk_cell_view_new_with_context        (CtkCellArea        *area,
                                                         CtkCellAreaContext *context);
CDK_AVAILABLE_IN_ALL
CtkWidget        *ctk_cell_view_new_with_text           (const gchar     *text);
CDK_AVAILABLE_IN_ALL
CtkWidget        *ctk_cell_view_new_with_markup         (const gchar     *markup);
CDK_AVAILABLE_IN_ALL
CtkWidget        *ctk_cell_view_new_with_pixbuf         (GdkPixbuf       *pixbuf);
CDK_AVAILABLE_IN_ALL
void              ctk_cell_view_set_model               (CtkCellView     *cell_view,
                                                         CtkTreeModel    *model);
CDK_AVAILABLE_IN_ALL
CtkTreeModel     *ctk_cell_view_get_model               (CtkCellView     *cell_view);
CDK_AVAILABLE_IN_ALL
void              ctk_cell_view_set_displayed_row       (CtkCellView     *cell_view,
                                                         CtkTreePath     *path);
CDK_AVAILABLE_IN_ALL
CtkTreePath      *ctk_cell_view_get_displayed_row       (CtkCellView     *cell_view);
CDK_AVAILABLE_IN_ALL
void              ctk_cell_view_set_background_rgba     (CtkCellView     *cell_view,
                                                         const CdkRGBA   *rgba);
CDK_AVAILABLE_IN_ALL
gboolean          ctk_cell_view_get_draw_sensitive      (CtkCellView     *cell_view);
CDK_AVAILABLE_IN_ALL
void              ctk_cell_view_set_draw_sensitive      (CtkCellView     *cell_view,
                                                         gboolean         draw_sensitive);
CDK_AVAILABLE_IN_ALL
gboolean          ctk_cell_view_get_fit_model           (CtkCellView     *cell_view);
CDK_AVAILABLE_IN_ALL
void              ctk_cell_view_set_fit_model           (CtkCellView     *cell_view,
                                                         gboolean         fit_model);

CDK_DEPRECATED_IN_3_0_FOR(ctk_widget_get_preferred_size)
gboolean          ctk_cell_view_get_size_of_row         (CtkCellView     *cell_view,
                                                         CtkTreePath     *path,
                                                         CtkRequisition  *requisition);
CDK_DEPRECATED_IN_3_4_FOR(ctk_cell_view_set_background_rgba)
void              ctk_cell_view_set_background_color    (CtkCellView     *cell_view,
                                                         const CdkColor  *color);

G_END_DECLS

#endif /* __CTK_CELL_VIEW_H__ */
