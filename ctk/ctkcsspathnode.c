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

#include "ctkcsspathnodeprivate.h"
#include "ctkcssstylepropertyprivate.h"
#include "ctkprivate.h"
#include "ctkstylecontextprivate.h"

G_DEFINE_TYPE (CtkCssPathNode, ctk_css_path_node, CTK_TYPE_CSS_NODE)

static void
ctk_css_path_node_finalize (GObject *object)
{
  CtkCssPathNode *node = CTK_CSS_PATH_NODE (object);

  if (node->path)
    ctk_widget_path_unref (node->path);

  G_OBJECT_CLASS (ctk_css_path_node_parent_class)->finalize (object);
}

static void
ctk_css_path_node_invalidate (CtkCssNode *node)
{
  CtkCssPathNode *path_node = CTK_CSS_PATH_NODE (node);

  if (path_node->context)
    ctk_style_context_validate (path_node->context, NULL);
}

gboolean
ctk_css_path_node_real_init_matcher (CtkCssNode     *node,
                                     CtkCssMatcher  *matcher)
{
  CtkCssPathNode *path_node = CTK_CSS_PATH_NODE (node);

  if (path_node->path == NULL ||
      ctk_widget_path_length (path_node->path) == 0)
    return FALSE;

  return _ctk_css_matcher_init (matcher,
                                path_node->path,
                                ctk_css_node_get_declaration (node));
}

static CtkWidgetPath *
ctk_css_path_node_real_create_widget_path (CtkCssNode *node)
{
  CtkCssPathNode *path_node = CTK_CSS_PATH_NODE (node);
  CtkWidgetPath *path;
  guint length;

  if (path_node->path == NULL)
    path = ctk_widget_path_new ();
  else
    path = ctk_widget_path_copy (path_node->path);

  length = ctk_widget_path_length (path);
  if (length > 0)
    {
      ctk_css_node_declaration_add_to_widget_path (ctk_css_node_get_declaration (node),
                                                   path,
                                                   length - 1);
    }

  return path;
}

static const CtkWidgetPath *
ctk_css_path_node_real_get_widget_path (CtkCssNode *node)
{
  CtkCssPathNode *path_node = CTK_CSS_PATH_NODE (node);

  return path_node->path;
}

static CtkCssStyle *
ctk_css_path_node_update_style (CtkCssNode   *cssnode,
                                CtkCssChange  change,
                                gint64        timestamp,
                                CtkCssStyle  *style)
{
  /* This should get rid of animations */
  return CTK_CSS_NODE_CLASS (ctk_css_path_node_parent_class)->update_style (cssnode, change, 0, style);
}

static CtkStyleProviderPrivate *
ctk_css_path_node_get_style_provider (CtkCssNode *node)
{
  CtkCssPathNode *path_node = CTK_CSS_PATH_NODE (node);

  if (path_node->context == NULL)
    return NULL;

  return ctk_style_context_get_style_provider (path_node->context);
}

static void
ctk_css_path_node_class_init (CtkCssPathNodeClass *klass)
{
  CtkCssNodeClass *node_class = CTK_CSS_NODE_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = ctk_css_path_node_finalize;

  node_class->invalidate = ctk_css_path_node_invalidate;
  node_class->update_style = ctk_css_path_node_update_style;
  node_class->init_matcher = ctk_css_path_node_real_init_matcher;
  node_class->create_widget_path = ctk_css_path_node_real_create_widget_path;
  node_class->get_widget_path = ctk_css_path_node_real_get_widget_path;
  node_class->get_style_provider = ctk_css_path_node_get_style_provider;
}

static void
ctk_css_path_node_init (CtkCssPathNode *cssnode)
{
}

CtkCssNode *
ctk_css_path_node_new (CtkStyleContext *context)
{
  CtkCssPathNode *node;
  
  g_return_val_if_fail (context == NULL || CTK_IS_STYLE_CONTEXT (context), NULL);

  node = g_object_new (CTK_TYPE_CSS_PATH_NODE, NULL);
  node->context = context;

  return CTK_CSS_NODE (node);
}

void
ctk_css_path_node_unset_context (CtkCssPathNode *node)
{
  ctk_internal_return_if_fail (CTK_IS_CSS_PATH_NODE (node));
  ctk_internal_return_if_fail (node->context != NULL);

  node->context = NULL;

  ctk_css_node_invalidate_style_provider (CTK_CSS_NODE (node));
}

void
ctk_css_path_node_set_widget_path (CtkCssPathNode *node,
                                   CtkWidgetPath  *path)
{
  ctk_internal_return_if_fail (CTK_IS_CSS_PATH_NODE (node));

  if (node->path == path)
    return;

  if (node->path)
    ctk_widget_path_unref (node->path);

  if (path)
    ctk_widget_path_ref (path);

  node->path = path;

  ctk_css_node_invalidate (CTK_CSS_NODE (node), CTK_CSS_CHANGE_ANY);
}

CtkWidgetPath *
ctk_css_path_node_get_widget_path (CtkCssPathNode *node)
{
  ctk_internal_return_val_if_fail (CTK_IS_CSS_PATH_NODE (node), NULL);

  return node->path;
}

