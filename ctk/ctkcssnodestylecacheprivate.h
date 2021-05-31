/* GTK - The GIMP Toolkit
 * Copyright (C) 2016 Benjamin Otte <otte@gnome.org>
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

#ifndef __CTK_CSS_NODE_STYLE_CACHE_PRIVATE_H__
#define __CTK_CSS_NODE_STYLE_CACHE_PRIVATE_H__

#include "ctkcssnodedeclarationprivate.h"
#include "ctkcssstyleprivate.h"

G_BEGIN_DECLS

typedef struct _CtkCssNodeStyleCache CtkCssNodeStyleCache;

CtkCssNodeStyleCache *  ctk_css_node_style_cache_new            (CtkCssStyle            *style);
CtkCssNodeStyleCache *  ctk_css_node_style_cache_ref            (CtkCssNodeStyleCache   *cache);
void                    ctk_css_node_style_cache_unref          (CtkCssNodeStyleCache   *cache);

CtkCssStyle *           ctk_css_node_style_cache_get_style      (CtkCssNodeStyleCache   *cache);

CtkCssNodeStyleCache *  ctk_css_node_style_cache_insert         (CtkCssNodeStyleCache   *parent,
                                                                 CtkCssNodeDeclaration  *decl,
                                                                 gboolean                is_first,
                                                                 gboolean                is_last,
                                                                 CtkCssStyle            *style);
CtkCssNodeStyleCache *  ctk_css_node_style_cache_lookup         (CtkCssNodeStyleCache        *parent,
                                                                 const CtkCssNodeDeclaration *decl,
                                                                 gboolean                     is_first,
                                                                 gboolean                     is_last);

G_END_DECLS

#endif /* __CTK_CSS_NODE_STYLE_CACHE_PRIVATE_H__ */
