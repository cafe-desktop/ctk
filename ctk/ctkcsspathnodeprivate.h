/* CTK - The GIMP Toolkit
 * Copyright (C) 2014 Benjamin Otte <otte@gnome.org>
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

#ifndef __CTK_CSS_PATH_NODE_PRIVATE_H__
#define __CTK_CSS_PATH_NODE_PRIVATE_H__

#include "ctkcssnodeprivate.h"
#include "ctkwidgetpath.h"

G_BEGIN_DECLS

#define CTK_TYPE_CSS_PATH_NODE           (ctk_css_path_node_get_type ())
#define CTK_CSS_PATH_NODE(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, CTK_TYPE_CSS_PATH_NODE, CtkCssPathNode))
#define CTK_CSS_PATH_NODE_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, CTK_TYPE_CSS_PATH_NODE, CtkCssPathNodeClass))
#define CTK_IS_CSS_PATH_NODE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, CTK_TYPE_CSS_PATH_NODE))
#define CTK_IS_CSS_PATH_NODE_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, CTK_TYPE_CSS_PATH_NODE))
#define CTK_CSS_PATH_NODE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CSS_PATH_NODE, CtkCssPathNodeClass))

typedef struct _CtkCssPathNode           CtkCssPathNode;
typedef struct _CtkCssPathNodeClass      CtkCssPathNodeClass;

struct _CtkCssPathNode
{
  CtkCssNode node;

  CtkStyleContext *context;
  CtkWidgetPath *path;
};

struct _CtkCssPathNodeClass
{
  CtkCssNodeClass node_class;
};

GType                   ctk_css_path_node_get_type         (void) G_GNUC_CONST;

CtkCssNode *            ctk_css_path_node_new              (CtkStyleContext *context);

void                    ctk_css_path_node_unset_context    (CtkCssPathNode *node);

void                    ctk_css_path_node_set_widget_path  (CtkCssPathNode *node,
                                                            CtkWidgetPath  *path);
CtkWidgetPath *         ctk_css_path_node_get_widget_path  (CtkCssPathNode *node);

G_END_DECLS

#endif /* __CTK_CSS_PATH_NODE_PRIVATE_H__ */
