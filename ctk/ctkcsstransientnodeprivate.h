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

#ifndef __CTK_CSS_TRANSIENT_NODE_PRIVATE_H__
#define __CTK_CSS_TRANSIENT_NODE_PRIVATE_H__

#include "ctkcssnodeprivate.h"

G_BEGIN_DECLS

#define CTK_TYPE_CSS_TRANSIENT_NODE           (ctk_css_transient_node_get_type ())
#define CTK_CSS_TRANSIENT_NODE(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, CTK_TYPE_CSS_TRANSIENT_NODE, CtkCssTransientNode))
#define CTK_CSS_TRANSIENT_NODE_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, CTK_TYPE_CSS_TRANSIENT_NODE, CtkCssTransientNodeClass))
#define CTK_IS_CSS_TRANSIENT_NODE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, CTK_TYPE_CSS_TRANSIENT_NODE))
#define CTK_IS_CSS_TRANSIENT_NODE_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, CTK_TYPE_CSS_TRANSIENT_NODE))
#define CTK_CSS_TRANSIENT_NODE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CSS_TRANSIENT_NODE, CtkCssTransientNodeClass))

typedef struct _CtkCssTransientNode           CtkCssTransientNode;
typedef struct _CtkCssTransientNodeClass      CtkCssTransientNodeClass;

struct _CtkCssTransientNode
{
  CtkCssNode node;
};

struct _CtkCssTransientNodeClass
{
  CtkCssNodeClass node_class;
};

GType                   ctk_css_transient_node_get_type         (void) G_GNUC_CONST;

CtkCssNode *            ctk_css_transient_node_new              (CtkCssNode     *parent);

G_END_DECLS

#endif /* __CTK_CSS_TRANSIENT_NODE_PRIVATE_H__ */
