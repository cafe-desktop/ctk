/* GTK - The GIMP Toolkit
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

#include "gtkcssnodeprivate.h"
#include "gtkwidgetpath.h"

G_BEGIN_DECLS

#define CTK_TYPE_CSS_PATH_NODE           (ctk_css_path_node_get_type ())
#define CTK_CSS_PATH_NODE(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, CTK_TYPE_CSS_PATH_NODE, GtkCssPathNode))
#define CTK_CSS_PATH_NODE_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, CTK_TYPE_CSS_PATH_NODE, GtkCssPathNodeClass))
#define CTK_IS_CSS_PATH_NODE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, CTK_TYPE_CSS_PATH_NODE))
#define CTK_IS_CSS_PATH_NODE_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, CTK_TYPE_CSS_PATH_NODE))
#define CTK_CSS_PATH_NODE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CSS_PATH_NODE, GtkCssPathNodeClass))

typedef struct _GtkCssPathNode           GtkCssPathNode;
typedef struct _GtkCssPathNodeClass      GtkCssPathNodeClass;

struct _GtkCssPathNode
{
  GtkCssNode node;

  GtkStyleContext *context;
  GtkWidgetPath *path;
};

struct _GtkCssPathNodeClass
{
  GtkCssNodeClass node_class;
};

GType                   ctk_css_path_node_get_type         (void) G_GNUC_CONST;

GtkCssNode *            ctk_css_path_node_new              (GtkStyleContext *context);

void                    ctk_css_path_node_unset_context    (GtkCssPathNode *node);

void                    ctk_css_path_node_set_widget_path  (GtkCssPathNode *node,
                                                            GtkWidgetPath  *path);
GtkWidgetPath *         ctk_css_path_node_get_widget_path  (GtkCssPathNode *node);

G_END_DECLS

#endif /* __CTK_CSS_PATH_NODE_PRIVATE_H__ */
