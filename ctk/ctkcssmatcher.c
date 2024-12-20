/* CTK - The GIMP Toolkit
 * Copyright (C) 2012 Benjamin Otte <otte@gnome.org>
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

#include "ctkcssmatcherprivate.h"

#include "ctkcssnodedeclarationprivate.h"
#include "ctkcssnodeprivate.h"
#include "ctkwidgetpath.h"

/* CTK_CSS_MATCHER_WIDGET_PATH */

static gboolean
ctk_css_matcher_widget_path_get_parent (CtkCssMatcher       *matcher,
                                        const CtkCssMatcher *child)
{
  if (child->path.index == 0)
    return FALSE;

  matcher->path.klass = child->path.klass;
  matcher->path.decl = NULL;
  matcher->path.path = child->path.path;
  matcher->path.index = child->path.index - 1;
  matcher->path.sibling_index = ctk_widget_path_iter_get_sibling_index (matcher->path.path, matcher->path.index);

  return TRUE;
}

static gboolean
ctk_css_matcher_widget_path_get_previous (CtkCssMatcher       *matcher,
                                          const CtkCssMatcher *next)
{
  if (next->path.sibling_index == 0)
    return FALSE;

  matcher->path.klass = next->path.klass;
  matcher->path.decl = NULL;
  matcher->path.path = next->path.path;
  matcher->path.index = next->path.index;
  matcher->path.sibling_index = next->path.sibling_index - 1;

  return TRUE;
}

static CtkStateFlags
ctk_css_matcher_widget_path_get_state (const CtkCssMatcher *matcher)
{
  const CtkWidgetPath *siblings;
  
  if (matcher->path.decl)
    return ctk_css_node_declaration_get_state (matcher->path.decl);

  siblings = ctk_widget_path_iter_get_siblings (matcher->path.path, matcher->path.index);
  if (siblings && matcher->path.sibling_index != ctk_widget_path_iter_get_sibling_index (matcher->path.path, matcher->path.index))
    return ctk_widget_path_iter_get_state (siblings, matcher->path.sibling_index);
  else
    return ctk_widget_path_iter_get_state (matcher->path.path, matcher->path.index);
}

static gboolean
ctk_css_matcher_widget_path_has_name (const CtkCssMatcher     *matcher,
                                      /*interned*/ const char *name)
{
  const CtkWidgetPath *siblings;

  siblings = ctk_widget_path_iter_get_siblings (matcher->path.path, matcher->path.index);
  if (siblings && matcher->path.sibling_index != ctk_widget_path_iter_get_sibling_index (matcher->path.path, matcher->path.index))
    {
      const char *path_name = ctk_widget_path_iter_get_object_name (siblings, matcher->path.sibling_index);

      if (path_name == NULL)
        path_name = g_type_name (ctk_widget_path_iter_get_object_type (siblings, matcher->path.sibling_index));

      return path_name == name;
    }
  else
    {
      const char *path_name = ctk_widget_path_iter_get_object_name (matcher->path.path, matcher->path.index);

      if (path_name == NULL)
        path_name = g_type_name (ctk_widget_path_iter_get_object_type (matcher->path.path, matcher->path.index));

      return path_name == name;
    }
}

static gboolean
ctk_css_matcher_widget_path_has_class (const CtkCssMatcher *matcher,
                                       GQuark               class_name)
{
  const CtkWidgetPath *siblings;
  
  if (matcher->path.decl &&
      ctk_css_node_declaration_has_class (matcher->path.decl, class_name))
    return TRUE;

  siblings = ctk_widget_path_iter_get_siblings (matcher->path.path, matcher->path.index);
  if (siblings && matcher->path.sibling_index != ctk_widget_path_iter_get_sibling_index (matcher->path.path, matcher->path.index))
    return ctk_widget_path_iter_has_qclass (siblings, matcher->path.sibling_index, class_name);
  else
    return ctk_widget_path_iter_has_qclass (matcher->path.path, matcher->path.index, class_name);
}

static gboolean
ctk_css_matcher_widget_path_has_id (const CtkCssMatcher *matcher,
                                    const char          *id)
{
  const CtkWidgetPath *siblings;
  
  siblings = ctk_widget_path_iter_get_siblings (matcher->path.path, matcher->path.index);
  if (siblings && matcher->path.sibling_index != ctk_widget_path_iter_get_sibling_index (matcher->path.path, matcher->path.index))
    return ctk_widget_path_iter_has_name (siblings, matcher->path.sibling_index, id);
  else
    return ctk_widget_path_iter_has_name (matcher->path.path, matcher->path.index, id);
}

static gboolean
ctk_css_matcher_widget_path_has_position (const CtkCssMatcher *matcher,
                                          gboolean             forward,
                                          int                  a,
                                          int                  b)
{
  const CtkWidgetPath *siblings;
  int x;

  siblings = ctk_widget_path_iter_get_siblings (matcher->path.path, matcher->path.index);
  if (!siblings)
    return FALSE;

  if (forward)
    x = matcher->path.sibling_index + 1;
  else
    x = ctk_widget_path_length (siblings) - matcher->path.sibling_index;

  x -= b;

  if (a == 0)
    return x == 0;

  if (x % a)
    return FALSE;

  return x / a >= 0;
}

static const CtkCssMatcherClass CTK_CSS_MATCHER_WIDGET_PATH = {
  ctk_css_matcher_widget_path_get_parent,
  ctk_css_matcher_widget_path_get_previous,
  ctk_css_matcher_widget_path_get_state,
  ctk_css_matcher_widget_path_has_name,
  ctk_css_matcher_widget_path_has_class,
  ctk_css_matcher_widget_path_has_id,
  ctk_css_matcher_widget_path_has_position,
  FALSE
};

gboolean
_ctk_css_matcher_init (CtkCssMatcher               *matcher,
                       const CtkWidgetPath         *path,
                       const CtkCssNodeDeclaration *decl)
{
  if (ctk_widget_path_length (path) == 0)
    return FALSE;

  matcher->path.klass = &CTK_CSS_MATCHER_WIDGET_PATH;
  matcher->path.decl = decl;
  matcher->path.path = path;
  matcher->path.index = ctk_widget_path_length (path) - 1;
  matcher->path.sibling_index = ctk_widget_path_iter_get_sibling_index (path, matcher->path.index);

  return TRUE;
}

/* CTK_CSS_MATCHER_NODE */

static gboolean
ctk_css_matcher_node_get_parent (CtkCssMatcher       *matcher,
                                 const CtkCssMatcher *child)
{
  CtkCssNode *node;
  
  node = ctk_css_node_get_parent (child->node.node);
  if (node == NULL)
    return FALSE;

  return ctk_css_node_init_matcher (node, matcher);
}

static CtkCssNode *
get_previous_visible_sibling (CtkCssNode *node)
{
  do {
    node = ctk_css_node_get_previous_sibling (node);
  } while (node && !ctk_css_node_get_visible (node));

  return node;
}

static CtkCssNode *
get_next_visible_sibling (CtkCssNode *node)
{
  do {
    node = ctk_css_node_get_next_sibling (node);
  } while (node && !ctk_css_node_get_visible (node));

  return node;
}

static gboolean
ctk_css_matcher_node_get_previous (CtkCssMatcher       *matcher,
                                   const CtkCssMatcher *next)
{
  CtkCssNode *node;
  
  node = get_previous_visible_sibling (next->node.node);
  if (node == NULL)
    return FALSE;

  return ctk_css_node_init_matcher (node, matcher);
}

static CtkStateFlags
ctk_css_matcher_node_get_state (const CtkCssMatcher *matcher)
{
  return ctk_css_node_get_state (matcher->node.node);
}

static gboolean
ctk_css_matcher_node_has_name (const CtkCssMatcher     *matcher,
                               /*interned*/ const char *name)
{
  return ctk_css_node_get_name (matcher->node.node) == name;
}

static gboolean
ctk_css_matcher_node_has_class (const CtkCssMatcher *matcher,
                                GQuark               class_name)
{
  return ctk_css_node_has_class (matcher->node.node, class_name);
}

static gboolean
ctk_css_matcher_node_has_id (const CtkCssMatcher *matcher,
                             const char          *id)
{
  /* assume all callers pass an interned string */
  return ctk_css_node_get_id (matcher->node.node) == id;
}

static gboolean
ctk_css_matcher_node_nth_child (CtkCssNode *node,
                                CtkCssNode *(* prev_node_func) (CtkCssNode *),
                                int         a,
                                int         b)
{
  int pos, x;

  /* special-case the common "first-child" and "last-child" */
  if (a == 0)
    {
      while (b > 0 && node != NULL)
        {
          b--;
          node = prev_node_func (node);
        }

      return b == 0 && node == NULL;
    }

  /* count nodes */
  for (pos = 0; node != NULL; pos++)
    node = prev_node_func (node);

  /* solve pos = a * X + b
   * and return TRUE if X is integer >= 0 */
  x = pos - b;

  if (x % a)
    return FALSE;

  return x / a >= 0;
}

static gboolean
ctk_css_matcher_node_has_position (const CtkCssMatcher *matcher,
                                   gboolean             forward,
                                   int                  a,
                                   int                  b)
{
  return ctk_css_matcher_node_nth_child (matcher->node.node,
                                         forward ? get_previous_visible_sibling 
                                                 : get_next_visible_sibling,
                                         a, b);
}

static const CtkCssMatcherClass CTK_CSS_MATCHER_NODE = {
  ctk_css_matcher_node_get_parent,
  ctk_css_matcher_node_get_previous,
  ctk_css_matcher_node_get_state,
  ctk_css_matcher_node_has_name,
  ctk_css_matcher_node_has_class,
  ctk_css_matcher_node_has_id,
  ctk_css_matcher_node_has_position,
  FALSE
};

void
_ctk_css_matcher_node_init (CtkCssMatcher *matcher,
                            CtkCssNode    *node)
{
  matcher->node.klass = &CTK_CSS_MATCHER_NODE;
  matcher->node.node = node;
}

/* CTK_CSS_MATCHER_WIDGET_ANY */

static gboolean
ctk_css_matcher_any_get_parent (CtkCssMatcher       *matcher,
                                const CtkCssMatcher *child G_GNUC_UNUSED)
{
  _ctk_css_matcher_any_init (matcher);

  return TRUE;
}

static gboolean
ctk_css_matcher_any_get_previous (CtkCssMatcher       *matcher,
                                  const CtkCssMatcher *next G_GNUC_UNUSED)
{
  _ctk_css_matcher_any_init (matcher);

  return TRUE;
}

static CtkStateFlags
ctk_css_matcher_any_get_state (const CtkCssMatcher *matcher G_GNUC_UNUSED)
{
  /* XXX: This gets tricky when we implement :not() */

  return CTK_STATE_FLAG_ACTIVE | CTK_STATE_FLAG_PRELIGHT | CTK_STATE_FLAG_SELECTED
    | CTK_STATE_FLAG_INSENSITIVE | CTK_STATE_FLAG_INCONSISTENT
    | CTK_STATE_FLAG_FOCUSED | CTK_STATE_FLAG_BACKDROP | CTK_STATE_FLAG_LINK
    | CTK_STATE_FLAG_VISITED;
}

static gboolean
ctk_css_matcher_any_has_name (const CtkCssMatcher     *matcher G_GNUC_UNUSED,
                              /*interned*/ const char *name G_GNUC_UNUSED)
{
  return TRUE;
}

static gboolean
ctk_css_matcher_any_has_class (const CtkCssMatcher *matcher G_GNUC_UNUSED,
                               GQuark               class_name G_GNUC_UNUSED)
{
  return TRUE;
}

static gboolean
ctk_css_matcher_any_has_id (const CtkCssMatcher *matcher G_GNUC_UNUSED,
                            const char          *id G_GNUC_UNUSED)
{
  return TRUE;
}

static gboolean
ctk_css_matcher_any_has_position (const CtkCssMatcher *matcher G_GNUC_UNUSED,
                                  gboolean             forward G_GNUC_UNUSED,
                                  int                  a G_GNUC_UNUSED,
                                  int                  b G_GNUC_UNUSED)
{
  return TRUE;
}

static const CtkCssMatcherClass CTK_CSS_MATCHER_ANY = {
  ctk_css_matcher_any_get_parent,
  ctk_css_matcher_any_get_previous,
  ctk_css_matcher_any_get_state,
  ctk_css_matcher_any_has_name,
  ctk_css_matcher_any_has_class,
  ctk_css_matcher_any_has_id,
  ctk_css_matcher_any_has_position,
  TRUE
};

void
_ctk_css_matcher_any_init (CtkCssMatcher *matcher)
{
  matcher->klass = &CTK_CSS_MATCHER_ANY;
}

/* CTK_CSS_MATCHER_WIDGET_SUPERSET */

static gboolean
ctk_css_matcher_superset_get_parent (CtkCssMatcher       *matcher,
                                     const CtkCssMatcher *child G_GNUC_UNUSED)
{
  _ctk_css_matcher_any_init (matcher);

  return TRUE;
}

static gboolean
ctk_css_matcher_superset_get_previous (CtkCssMatcher       *matcher,
                                       const CtkCssMatcher *next G_GNUC_UNUSED)
{
  _ctk_css_matcher_any_init (matcher);

  return TRUE;
}

static CtkStateFlags
ctk_css_matcher_superset_get_state (const CtkCssMatcher *matcher)
{
  /* XXX: This gets tricky when we implement :not() */

  if (matcher->superset.relevant & CTK_CSS_CHANGE_STATE)
    return _ctk_css_matcher_get_state (matcher->superset.subset);
  else
    return CTK_STATE_FLAG_ACTIVE | CTK_STATE_FLAG_PRELIGHT | CTK_STATE_FLAG_SELECTED
      | CTK_STATE_FLAG_INSENSITIVE | CTK_STATE_FLAG_INCONSISTENT
      | CTK_STATE_FLAG_FOCUSED | CTK_STATE_FLAG_BACKDROP | CTK_STATE_FLAG_LINK
      | CTK_STATE_FLAG_VISITED;
}

static gboolean
ctk_css_matcher_superset_has_name (const CtkCssMatcher     *matcher,
                                   /*interned*/ const char *name)
{
  if (matcher->superset.relevant & CTK_CSS_CHANGE_NAME)
    return _ctk_css_matcher_has_name (matcher->superset.subset, name);
  else
    return TRUE;
}

static gboolean
ctk_css_matcher_superset_has_class (const CtkCssMatcher *matcher,
                                    GQuark               class_name)
{
  if (matcher->superset.relevant & CTK_CSS_CHANGE_CLASS)
    return _ctk_css_matcher_has_class (matcher->superset.subset, class_name);
  else
    return TRUE;
}

static gboolean
ctk_css_matcher_superset_has_id (const CtkCssMatcher *matcher,
                                 const char          *id)
{
  if (matcher->superset.relevant & CTK_CSS_CHANGE_NAME)
    return _ctk_css_matcher_has_id (matcher->superset.subset, id);
  else
    return TRUE;
}

static gboolean
ctk_css_matcher_superset_has_position (const CtkCssMatcher *matcher,
                                       gboolean             forward,
                                       int                  a,
                                       int                  b)
{
  if (matcher->superset.relevant & CTK_CSS_CHANGE_POSITION)
    return _ctk_css_matcher_has_position (matcher->superset.subset, forward, a, b);
  else
    return TRUE;
}

static const CtkCssMatcherClass CTK_CSS_MATCHER_SUPERSET = {
  ctk_css_matcher_superset_get_parent,
  ctk_css_matcher_superset_get_previous,
  ctk_css_matcher_superset_get_state,
  ctk_css_matcher_superset_has_name,
  ctk_css_matcher_superset_has_class,
  ctk_css_matcher_superset_has_id,
  ctk_css_matcher_superset_has_position,
  FALSE
};

void
_ctk_css_matcher_superset_init (CtkCssMatcher       *matcher,
                                const CtkCssMatcher *subset,
                                CtkCssChange         relevant)
{
  g_return_if_fail (subset != NULL);
  g_return_if_fail ((relevant & ~(CTK_CSS_CHANGE_CLASS | CTK_CSS_CHANGE_NAME | CTK_CSS_CHANGE_POSITION | CTK_CSS_CHANGE_STATE)) == 0);

  matcher->superset.klass = &CTK_CSS_MATCHER_SUPERSET;
  matcher->superset.subset = subset;
  matcher->superset.relevant = relevant;
}

