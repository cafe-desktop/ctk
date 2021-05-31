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

#include "gtk/gtkcssmatcherprivate.h"
#include "gtk/gtkcssparserprivate.h"

G_BEGIN_DECLS

typedef union _GtkCssSelector GtkCssSelector;
typedef struct _GtkCssSelectorTree GtkCssSelectorTree;
typedef struct _GtkCssSelectorTreeBuilder GtkCssSelectorTreeBuilder;

GtkCssSelector *  _ctk_css_selector_parse           (GtkCssParser           *parser);
void              _ctk_css_selector_free            (GtkCssSelector         *selector);

char *            _ctk_css_selector_to_string       (const GtkCssSelector   *selector);
void              _ctk_css_selector_print           (const GtkCssSelector   *selector,
                                                     GString                *str);

gboolean          _ctk_css_selector_matches         (const GtkCssSelector   *selector,
                                                     const GtkCssMatcher    *matcher);
GtkCssChange      _ctk_css_selector_get_change      (const GtkCssSelector   *selector);
int               _ctk_css_selector_compare         (const GtkCssSelector   *a,
                                                     const GtkCssSelector   *b);

void         _ctk_css_selector_tree_free             (GtkCssSelectorTree       *tree);
GPtrArray *  _ctk_css_selector_tree_match_all        (const GtkCssSelectorTree *tree,
						      const GtkCssMatcher      *matcher);
GtkCssChange _ctk_css_selector_tree_get_change_all   (const GtkCssSelectorTree *tree,
						      const GtkCssMatcher *matcher);
void         _ctk_css_selector_tree_match_print      (const GtkCssSelectorTree *tree,
						      GString                  *str);


GtkCssSelectorTreeBuilder *_ctk_css_selector_tree_builder_new   (void);
void                       _ctk_css_selector_tree_builder_add   (GtkCssSelectorTreeBuilder *builder,
								 GtkCssSelector            *selectors,
								 GtkCssSelectorTree       **selector_match,
								 gpointer                   match);
GtkCssSelectorTree *       _ctk_css_selector_tree_builder_build (GtkCssSelectorTreeBuilder *builder);
void                       _ctk_css_selector_tree_builder_free  (GtkCssSelectorTreeBuilder *builder);

const char *ctk_css_pseudoclass_name (GtkStateFlags flags);

G_END_DECLS

#endif /* __CTK_CSS_SELECTOR_PRIVATE_H__ */
