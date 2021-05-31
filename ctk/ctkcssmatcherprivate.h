/* GTK - The GIMP Toolkit
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

#ifndef __CTK_CSS_MATCHER_PRIVATE_H__
#define __CTK_CSS_MATCHER_PRIVATE_H__

#include <ctk/ctkenums.h>
#include <ctk/ctktypes.h>
#include "ctk/ctkcsstypesprivate.h"

G_BEGIN_DECLS

typedef struct _CtkCssMatcherNode CtkCssMatcherNode;
typedef struct _CtkCssMatcherSuperset CtkCssMatcherSuperset;
typedef struct _CtkCssMatcherWidgetPath CtkCssMatcherWidgetPath;
typedef struct _CtkCssMatcherClass CtkCssMatcherClass;

struct _CtkCssMatcherClass {
  gboolean        (* get_parent)                  (CtkCssMatcher          *matcher,
                                                   const CtkCssMatcher    *child);
  gboolean        (* get_previous)                (CtkCssMatcher          *matcher,
                                                   const CtkCssMatcher    *next);

  CtkStateFlags   (* get_state)                   (const CtkCssMatcher   *matcher);
  gboolean        (* has_name)                    (const CtkCssMatcher   *matcher,
                                                   /*interned*/const char*name);
  gboolean        (* has_class)                   (const CtkCssMatcher   *matcher,
                                                   GQuark                 class_name);
  gboolean        (* has_id)                      (const CtkCssMatcher   *matcher,
                                                   const char            *id);
  gboolean        (* has_position)                (const CtkCssMatcher   *matcher,
                                                   gboolean               forward,
                                                   int                    a,
                                                   int                    b);
  gboolean is_any;
};

struct _CtkCssMatcherWidgetPath {
  const CtkCssMatcherClass *klass;
  const CtkCssNodeDeclaration *decl;
  const CtkWidgetPath      *path;
  guint                     index;
  guint                     sibling_index;
};

struct _CtkCssMatcherNode {
  const CtkCssMatcherClass *klass;
  CtkCssNode               *node;
};

struct _CtkCssMatcherSuperset {
  const CtkCssMatcherClass *klass;
  const CtkCssMatcher      *subset;
  CtkCssChange              relevant;
};

union _CtkCssMatcher {
  const CtkCssMatcherClass *klass;
  CtkCssMatcherWidgetPath   path;
  CtkCssMatcherNode         node;
  CtkCssMatcherSuperset     superset;
};

gboolean          _ctk_css_matcher_init           (CtkCssMatcher          *matcher,
                                                   const CtkWidgetPath    *path,
                                                   const CtkCssNodeDeclaration *decl) G_GNUC_WARN_UNUSED_RESULT;
void              _ctk_css_matcher_node_init      (CtkCssMatcher          *matcher,
                                                   CtkCssNode             *node);
void              _ctk_css_matcher_any_init       (CtkCssMatcher          *matcher);
void              _ctk_css_matcher_superset_init  (CtkCssMatcher          *matcher,
                                                   const CtkCssMatcher    *subset,
                                                   CtkCssChange            relevant);


static inline gboolean
_ctk_css_matcher_get_parent (CtkCssMatcher       *matcher,
                             const CtkCssMatcher *child)
{
  return child->klass->get_parent (matcher, child);
}

static inline gboolean
_ctk_css_matcher_get_previous (CtkCssMatcher       *matcher,
                               const CtkCssMatcher *next)
{
  return next->klass->get_previous (matcher, next);
}

static inline CtkStateFlags
_ctk_css_matcher_get_state (const CtkCssMatcher *matcher)
{
  return matcher->klass->get_state (matcher);
}

static inline gboolean
_ctk_css_matcher_has_name (const CtkCssMatcher     *matcher,
                           /*interned*/ const char *name)
{
  return matcher->klass->has_name (matcher, name);
}

static inline gboolean
_ctk_css_matcher_has_class (const CtkCssMatcher *matcher,
                            GQuark               class_name)
{
  return matcher->klass->has_class (matcher, class_name);
}

static inline gboolean
_ctk_css_matcher_has_id (const CtkCssMatcher *matcher,
                         const char          *id)
{
  return matcher->klass->has_id (matcher, id);
}

static inline guint
_ctk_css_matcher_has_position (const CtkCssMatcher *matcher,
                               gboolean             forward,
                               int                  a,
                               int                  b)
{
  return matcher->klass->has_position (matcher, forward, a, b);
}

static inline gboolean
_ctk_css_matcher_matches_any (const CtkCssMatcher *matcher)
{
  return matcher->klass->is_any;
}


G_END_DECLS

#endif /* __CTK_CSS_MATCHER_PRIVATE_H__ */
