/* gtkcellview.h
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
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gtk/gtkwidget.h>
#include <gtk/gtkcellrenderer.h>
#include <gtk/gtkcellarea.h>
#include <gtk/gtkcellareacontext.h>
#include <gtk/gtktreemodel.h>

G_BEGIN_DECLS

#define CTK_TYPE_CELL_VIEW                (ctk_cell_view_get_type ())
#define CTK_CELL_VIEW(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_CELL_VIEW, GtkCellView))
#define CTK_CELL_VIEW_CLASS(vtable)       (G_TYPE_CHECK_CLASS_CAST ((vtable), CTK_TYPE_CELL_VIEW, GtkCellViewClass))
#define CTK_IS_CELL_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_CELL_VIEW))
#define CTK_IS_CELL_VIEW_CLASS(vtable)    (G_TYPE_CHECK_CLASS_TYPE ((vtable), CTK_TYPE_CELL_VIEW))
#define CTK_CELL_VIEW_GET_CLASS(inst)     (G_TYPE_INSTANCE_GET_CLASS ((inst), CTK_TYPE_CELL_VIEW, GtkCellViewClass))

typedef struct _GtkCellView             GtkCellView;
typedef struct _GtkCellViewClass        GtkCellViewClass;
typedef struct _GtkCellViewPrivate      GtkCellViewPrivate;

struct _GtkCellView
{
  GtkWidget parent_instance;

  /*< private >*/
  GtkCellViewPrivate *priv;
};

/**
 * GtkCellViewClass:
 * @parent_class: The parent class.
 */
struct _GtkCellViewClass
{
  GtkWidgetClass parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
GType             ctk_cell_view_get_type                (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
GtkWidget        *ctk_cell_view_new                     (void);
GDK_AVAILABLE_IN_ALL
GtkWidget        *ctk_cell_view_new_with_context        (GtkCellArea        *area,
                                                         GtkCellAreaContext *context);
GDK_AVAILABLE_IN_ALL
GtkWidget        *ctk_cell_view_new_with_text           (const gchar     *text);
GDK_AVAILABLE_IN_ALL
GtkWidget        *ctk_cell_view_new_with_markup         (const gchar     *markup);
GDK_AVAILABLE_IN_ALL
GtkWidget        *ctk_cell_view_new_with_pixbuf         (GdkPixbuf       *pixbuf);
GDK_AVAILABLE_IN_ALL
void              ctk_cell_view_set_model               (GtkCellView     *cell_view,
                                                         GtkTreeModel    *model);
GDK_AVAILABLE_IN_ALL
GtkTreeModel     *ctk_cell_view_get_model               (GtkCellView     *cell_view);
GDK_AVAILABLE_IN_ALL
void              ctk_cell_view_set_displayed_row       (GtkCellView     *cell_view,
                                                         GtkTreePath     *path);
GDK_AVAILABLE_IN_ALL
GtkTreePath      *ctk_cell_view_get_displayed_row       (GtkCellView     *cell_view);
GDK_AVAILABLE_IN_ALL
void              ctk_cell_view_set_background_rgba     (GtkCellView     *cell_view,
                                                         const GdkRGBA   *rgba);
GDK_AVAILABLE_IN_ALL
gboolean          ctk_cell_view_get_draw_sensitive      (GtkCellView     *cell_view);
GDK_AVAILABLE_IN_ALL
void              ctk_cell_view_set_draw_sensitive      (GtkCellView     *cell_view,
                                                         gboolean         draw_sensitive);
GDK_AVAILABLE_IN_ALL
gboolean          ctk_cell_view_get_fit_model           (GtkCellView     *cell_view);
GDK_AVAILABLE_IN_ALL
void              ctk_cell_view_set_fit_model           (GtkCellView     *cell_view,
                                                         gboolean         fit_model);

GDK_DEPRECATED_IN_3_0_FOR(ctk_widget_get_preferred_size)
gboolean          ctk_cell_view_get_size_of_row         (GtkCellView     *cell_view,
                                                         GtkTreePath     *path,
                                                         GtkRequisition  *requisition);
GDK_DEPRECATED_IN_3_4_FOR(ctk_cell_view_set_background_rgba)
void              ctk_cell_view_set_background_color    (GtkCellView     *cell_view,
                                                         const GdkColor  *color);

G_END_DECLS

#endif /* __CTK_CELL_VIEW_H__ */
