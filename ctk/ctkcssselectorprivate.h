/* GTK - The GIMP Toolkit
 * Copyright (C) 2011 Benjamin Otte <otte@gnome.org>
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

#ifndef __CTK_CSS_SELECTOR_PRIVATE_H__
#define __CTK_CSS_SELECTOR_PRIVATE_H__

#include "ctk/ctkcssmatcherprivate.h"
#include "ctk/ctkcssparserprivate.h"

G_BEGIN_DECLS

typedef union _CtkCssSelector CtkCssSelector;
typedef struct _CtkCssSelectorTree CtkCssSelectorTree;
typedef struct _CtkCssSelectorTreeBuilder CtkCssSelectorTreeBuilder;

CtkCssSelector *  _ctk_css_selector_parse           (CtkCssParser           *parser);
void              _ctk_css_selector_free            (CtkCssSelector         *selector);

char *            _ctk_css_selector_to_string       (const CtkCssSelector   *selector);
void              _ctk_css_selector_print           (const CtkCssSelector   *selector,
                                                     GString                *str);

gboolean          _ctk_css_selector_matches         (const CtkCssSelector   *selector,
                                                     const CtkCssMatcher    *matcher);
CtkCssChange      _ctk_css_selector_get_change      (const CtkCssSelector   *selector);
int               _ctk_css_selector_compare         (const CtkCssSelector   *a,
                                                     const CtkCssSelector   *b);

void         _ctk_css_selector_tree_free             (CtkCssSelectorTree       *tree);
GPtrArray *  _ctk_css_selector_tree_match_all        (const CtkCssSelectorTree *tree,
						      const CtkCssMatcher      *matcher);
CtkCssChange _ctk_css_selector_tree_get_change_all   (const CtkCssSelectorTree *tree,
						      const CtkCssMatcher *matcher);
void         _ctk_css_selector_tree_match_print      (const CtkCssSelectorTree *tree,
						      GString                  *str);


CtkCssSelectorTreeBuilder *_ctk_css_selector_tree_builder_new   (void);
void                       _ctk_css_selector_tree_builder_add   (CtkCssSelectorTreeBuilder *builder,
								 CtkCssSelector            *selectors,
								 CtkCssSelectorTree       **selector_match,
								 gpointer                   match);
CtkCssSelectorTree *       _ctk_css_selector_tree_builder_build (CtkCssSelectorTreeBuilder *builder);
void                       _ctk_css_selector_tree_builder_free  (CtkCssSelectorTreeBuilder *builder);

const char *ctk_css_pseudoclass_name (CtkStateFlags flags);

G_END_DECLS

#endif /* __CTK_CSS_SELECTOR_PRIVATE_H__ */
