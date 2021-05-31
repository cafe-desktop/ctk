/* ctktreestore.h
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

#include <ctk/ctktreemodel.h>

#include "ctk/ctkcssnodeprivate.h"


G_BEGIN_DECLS


#define CTK_TYPE_TREE_MODEL_CSS_NODE			(ctk_tree_model_css_node_get_type ())
#define CTK_TREE_MODEL_CSS_NODE(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_TREE_MODEL_CSS_NODE, CtkTreeModelCssNode))
#define CTK_TREE_MODEL_CSS_NODE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_TREE_MODEL_CSS_NODE, CtkTreeModelCssNodeClass))
#define CTK_IS_TREE_MODEL_CSS_NODE(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_TREE_MODEL_CSS_NODE))
#define CTK_IS_TREE_MODEL_CSS_NODE_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_TREE_MODEL_CSS_NODE))
#define CTK_TREE_MODEL_CSS_NODE_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_TREE_MODEL_CSS_NODE, CtkTreeModelCssNodeClass))

typedef struct _CtkTreeModelCssNode        CtkTreeModelCssNode;
typedef struct _CtkTreeModelCssNodeClass   CtkTreeModelCssNodeClass;
typedef struct _CtkTreeModelCssNodePrivate CtkTreeModelCssNodePrivate;

typedef void (* CtkTreeModelCssNodeGetFunc)             (CtkTreeModelCssNode   *model,
                                                         CtkCssNode            *node,
                                                         int                    column,
                                                         GValue                *value);

struct _CtkTreeModelCssNode
{
  GObject parent;

  CtkTreeModelCssNodePrivate *priv;
};

struct _CtkTreeModelCssNodeClass
{
  GObjectClass parent_class;
};


GType         ctk_tree_model_css_node_get_type          (void) G_GNUC_CONST;

CtkTreeModel *ctk_tree_model_css_node_new               (CtkTreeModelCssNodeGetFunc get_func,
                                                         gint            n_columns,
					                 ...);
CtkTreeModel *ctk_tree_model_css_node_newv              (CtkTreeModelCssNodeGetFunc get_func,
                                                         gint            n_columns,
					                 GType          *types);

void          ctk_tree_model_css_node_set_root_node     (CtkTreeModelCssNode    *model,
                                                         CtkCssNode             *node);
CtkCssNode   *ctk_tree_model_css_node_get_root_node     (CtkTreeModelCssNode    *model);
CtkCssNode   *ctk_tree_model_css_node_get_node_from_iter(CtkTreeModelCssNode    *model,
                                                         CtkTreeIter            *iter);
void          ctk_tree_model_css_node_get_iter_from_node(CtkTreeModelCssNode    *model,
                                                         CtkTreeIter            *iter,
                                                         CtkCssNode             *node);


G_END_DECLS


#endif /* __CTK_TREE_MODEL_CSS_NODE_H__ */
