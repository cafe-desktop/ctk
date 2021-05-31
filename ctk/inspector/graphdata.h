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

#ifndef __CTK_GRAPH_DATA_H__
#define __CTK_GRAPH_DATA_H__

#include <ctk/ctk.h>

G_BEGIN_DECLS

#define CTK_TYPE_GRAPH_DATA            (ctk_graph_data_get_type ())
#define CTK_GRAPH_DATA(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_GRAPH_DATA, CtkGraphData))
#define CTK_GRAPH_DATA_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_GRAPH_DATA, CtkGraphDataClass))
#define CTK_IS_GRAPH_DATA(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_GRAPH_DATA))
#define CTK_IS_GRAPH_DATA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_GRAPH_DATA))
#define CTK_GRAPH_DATA_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_GRAPH_DATA, CtkGraphDataClass))

typedef struct _CtkGraphData        CtkGraphData;
typedef struct _CtkGraphDataClass   CtkGraphDataClass;
typedef struct _CtkGraphDataPrivate CtkGraphDataPrivate;

struct _CtkGraphData
{
  GObject              object;

  /*< private >*/
  CtkGraphDataPrivate *priv;
};

struct _CtkGraphDataClass
{
  GObjectClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


GType            ctk_graph_data_get_type        (void) G_GNUC_CONST;

CtkGraphData    *ctk_graph_data_new             (guint           n_values);

guint            ctk_graph_data_get_n_values    (CtkGraphData   *data);
double           ctk_graph_data_get_value       (CtkGraphData   *data,
                                                 guint           i);
double           ctk_graph_data_get_minimum     (CtkGraphData   *data);
double           ctk_graph_data_get_maximum     (CtkGraphData   *data);

void             ctk_graph_data_prepend_value   (CtkGraphData   *data,
                                                 double          value);

G_END_DECLS

#endif /* __CTK_GRAPH_DATA_H__ */
