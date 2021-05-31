/* gtktreestore.h
 * Copyright (C) 2014 Benjamin Otte <otte@gnome.org>
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

#ifndef __CTK_TREE_MODEL_CSS_NODE_H__
#define __CTK_TREE_MODEL_CSS_NODE_H__

#include <gtk/gtktreemodel.h>

#include "gtk/gtkcssnodeprivate.h"


G_BEGIN_DECLS


#define CTK_TYPE_TREE_MODEL_CSS_NODE			(ctk_tree_model_css_node_get_type ())
#define CTK_TREE_MODEL_CSS_NODE(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_TREE_MODEL_CSS_NODE, GtkTreeModelCssNode))
#define CTK_TREE_MODEL_CSS_NODE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_TREE_MODEL_CSS_NODE, GtkTreeModelCssNodeClass))
#define CTK_IS_TREE_MODEL_CSS_NODE(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_TREE_MODEL_CSS_NODE))
#define CTK_IS_TREE_MODEL_CSS_NODE_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_TREE_MODEL_CSS_NODE))
#define CTK_TREE_MODEL_CSS_NODE_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_TREE_MODEL_CSS_NODE, GtkTreeModelCssNodeClass))

typedef struct _GtkTreeModelCssNode        GtkTreeModelCssNode;
typedef struct _GtkTreeModelCssNodeClass   GtkTreeModelCssNodeClass;
typedef struct _GtkTreeModelCssNodePrivate GtkTreeModelCssNodePrivate;

typedef void (* GtkTreeModelCssNodeGetFunc)             (GtkTreeModelCssNode   *model,
                                                         GtkCssNode            *node,
                                                         int                    column,
                                                         GValue                *value);

struct _GtkTreeModelCssNode
{
  GObject parent;

  GtkTreeModelCssNodePrivate *priv;
};

struct _GtkTreeModelCssNodeClass
{
  GObjectClass parent_class;
};


GType         ctk_tree_model_css_node_get_type          (void) G_GNUC_CONST;

GtkTreeModel *ctk_tree_model_css_node_new               (GtkTreeModelCssNodeGetFunc get_func,
                                                         gint            n_columns,
					                 ...);
GtkTreeModel *ctk_tree_model_css_node_newv              (GtkTreeModelCssNodeGetFunc get_func,
                                                         gint            n_columns,
					                 GType          *types);

void          ctk_tree_model_css_node_set_root_node     (GtkTreeModelCssNode    *model,
                                                         GtkCssNode             *node);
GtkCssNode   *ctk_tree_model_css_node_get_root_node     (GtkTreeModelCssNode    *model);
GtkCssNode   *ctk_tree_model_css_node_get_node_from_iter(GtkTreeModelCssNode    *model,
                                                         GtkTreeIter            *iter);
void          ctk_tree_model_css_node_get_iter_from_node(GtkTreeModelCssNode    *model,
                                                         GtkTreeIter            *iter,
                                                         GtkCssNode             *node);


G_END_DECLS


#endif /* __CTK_TREE_MODEL_CSS_NODE_H__ */
