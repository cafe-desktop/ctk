/*
 * Copyright © 2014 Benjamin Otte <otte@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CTK_CSS_NODE_DECLARATION_PRIVATE_H__
#define __CTK_CSS_NODE_DECLARATION_PRIVATE_H__

#include "ctkcsstypesprivate.h"
#include "ctkenums.h"
#include "ctkwidgetpath.h"

G_BEGIN_DECLS

CtkCssNodeDeclaration * ctk_css_node_declaration_new                    (void);
CtkCssNodeDeclaration * ctk_css_node_declaration_ref                    (CtkCssNodeDeclaration         *decl);
void                    ctk_css_node_declaration_unref                  (CtkCssNodeDeclaration         *decl);

gboolean                ctk_css_node_declaration_set_junction_sides     (CtkCssNodeDeclaration        **decl,
                                                                         CtkJunctionSides               junction_sides);
CtkJunctionSides        ctk_css_node_declaration_get_junction_sides     (const CtkCssNodeDeclaration   *decl);
gboolean                ctk_css_node_declaration_set_type               (CtkCssNodeDeclaration        **decl,
                                                                         GType                          type);
GType                   ctk_css_node_declaration_get_type               (const CtkCssNodeDeclaration   *decl);
gboolean                ctk_css_node_declaration_set_name               (CtkCssNodeDeclaration        **decl,
                                                                         /*interned*/ const char       *name);
/*interned*/ const char*ctk_css_node_declaration_get_name               (const CtkCssNodeDeclaration   *decl);
gboolean                ctk_css_node_declaration_set_id                 (CtkCssNodeDeclaration        **decl,
                                                                         const char                    *id);
const char *            ctk_css_node_declaration_get_id                 (const CtkCssNodeDeclaration   *decl);
gboolean                ctk_css_node_declaration_set_state              (CtkCssNodeDeclaration        **decl,
                                                                         CtkStateFlags                  flags);
CtkStateFlags           ctk_css_node_declaration_get_state              (const CtkCssNodeDeclaration   *decl);

gboolean                ctk_css_node_declaration_add_class              (CtkCssNodeDeclaration        **decl,
                                                                         GQuark                         class_quark);
gboolean                ctk_css_node_declaration_remove_class           (CtkCssNodeDeclaration        **decl,
                                                                         GQuark                         class_quark);
gboolean                ctk_css_node_declaration_clear_classes          (CtkCssNodeDeclaration        **decl);
gboolean                ctk_css_node_declaration_has_class              (const CtkCssNodeDeclaration   *decl,
                                                                         GQuark                         class_quark);
const GQuark *          ctk_css_node_declaration_get_classes            (const CtkCssNodeDeclaration   *decl,
                                                                         guint                         *n_classes);

gboolean                ctk_css_node_declaration_add_region             (CtkCssNodeDeclaration        **decl,
                                                                         GQuark                         region_quark,
                                                                         CtkRegionFlags                 flags);
gboolean                ctk_css_node_declaration_remove_region          (CtkCssNodeDeclaration        **decl,
                                                                         GQuark                         region_quark);
gboolean                ctk_css_node_declaration_clear_regions          (CtkCssNodeDeclaration        **decl);
gboolean                ctk_css_node_declaration_has_region             (const CtkCssNodeDeclaration   *decl,
                                                                         GQuark                         region_quark,
                                                                         CtkRegionFlags                *flags_return);
GList *                 ctk_css_node_declaration_list_regions           (const CtkCssNodeDeclaration   *decl);

guint                   ctk_css_node_declaration_hash                   (gconstpointer                  elem);
gboolean                ctk_css_node_declaration_equal                  (gconstpointer                  elem1,
                                                                         gconstpointer                  elem2);

void                    ctk_css_node_declaration_add_to_widget_path     (const CtkCssNodeDeclaration   *decl,
                                                                         CtkWidgetPath                 *path,
                                                                         guint                          pos);

void                    ctk_css_node_declaration_print                  (const CtkCssNodeDeclaration   *decl,
                                                                         GString                       *string);

G_END_DECLS

#endif /* __CTK_CSS_NODE_DECLARATION_PRIVATE_H__ */
