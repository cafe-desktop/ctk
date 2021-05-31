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

#include "config.h"

#include "ctkcsstransientnodeprivate.h"
#include "ctkprivate.h"

G_DEFINE_TYPE (CtkCssTransientNode, ctk_css_transient_node, CTK_TYPE_CSS_NODE)

static CtkWidgetPath *
ctk_css_transient_node_create_widget_path (CtkCssNode *node)
{
  CtkWidgetPath *result;
  CtkCssNode *parent;

  parent = ctk_css_node_get_parent (node);
  if (parent == NULL)
    result = ctk_widget_path_new ();
  else
    result = ctk_css_node_create_widget_path (parent);

  ctk_widget_path_append_type (result, ctk_css_node_get_widget_type (node));
  ctk_css_node_declaration_add_to_widget_path (ctk_css_node_get_declaration (node), result, -1);
  
  return result;
}

static const CtkWidgetPath *
ctk_css_transient_node_get_widget_path (CtkCssNode *node)
{
  CtkCssNode *parent;

  parent = ctk_css_node_get_parent (node);
  if (parent == NULL)
    return NULL;

  return ctk_css_node_get_widget_path (parent);
}

static CtkCssStyle *
ctk_css_transient_node_update_style (CtkCssNode   *cssnode,
                                     CtkCssChange  change,
                                     gint64        timestamp,
                                     CtkCssStyle  *style)
{
  /* This should get rid of animations */
  return CTK_CSS_NODE_CLASS (ctk_css_transient_node_parent_class)->update_style (cssnode, change, 0, style);
}

static void
ctk_css_transient_node_class_init (CtkCssTransientNodeClass *klass)
{
  CtkCssNodeClass *node_class = CTK_CSS_NODE_CLASS (klass);

  node_class->create_widget_path = ctk_css_transient_node_create_widget_path;
  node_class->get_widget_path = ctk_css_transient_node_get_widget_path;
  node_class->update_style = ctk_css_transient_node_update_style;
}

static void
ctk_css_transient_node_init (CtkCssTransientNode *cssnode)
{
  ctk_css_node_set_visible (CTK_CSS_NODE (cssnode), FALSE);
}

CtkCssNode *
ctk_css_transient_node_new (CtkCssNode *parent)
{
  CtkCssNode *result;

  ctk_internal_return_val_if_fail (CTK_IS_CSS_NODE (parent), NULL);

  result = g_object_new (CTK_TYPE_CSS_TRANSIENT_NODE, NULL);
  ctk_css_node_declaration_unref (result->decl);
  result->decl = ctk_css_node_declaration_ref (parent->decl);

  return result;
}

