/*
 * Copyright (c) 2014 Benjamin Otte <ottte@gnome.org>
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

#ifndef __CTK_CELL_RENDERER_GRAPH_H__
#define __CTK_CELL_RENDERER_GRAPH_H__

#include <ctk/ctk.h>

G_BEGIN_DECLS

#define CTK_TYPE_CELL_RENDERER_GRAPH            (ctk_cell_renderer_graph_get_type ())
#define CTK_CELL_RENDERER_GRAPH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_CELL_RENDERER_GRAPH, GtkCellRendererGraph))
#define CTK_CELL_RENDERER_GRAPH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_CELL_RENDERER_GRAPH, GtkCellRendererGraphClass))
#define CTK_IS_CELL_RENDERER_GRAPH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_CELL_RENDERER_GRAPH))
#define CTK_IS_CELL_RENDERER_GRAPH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_CELL_RENDERER_GRAPH))
#define CTK_CELL_RENDERER_GRAPH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CELL_RENDERER_GRAPH, GtkCellRendererGraphClass))

typedef struct _GtkCellRendererGraph        GtkCellRendererGraph;
typedef struct _GtkCellRendererGraphClass   GtkCellRendererGraphClass;
typedef struct _GtkCellRendererGraphPrivate GtkCellRendererGraphPrivate;

struct _GtkCellRendererGraph
{
  GtkCellRenderer                parent;

  /*< private >*/
  GtkCellRendererGraphPrivate *priv;
};

struct _GtkCellRendererGraphClass
{
  GtkCellRendererClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
GType            ctk_cell_renderer_graph_get_type (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
GtkCellRenderer *ctk_cell_renderer_graph_new      (void);

G_END_DECLS

#endif /* __CTK_CELL_RENDERER_GRAPH_H__ */
