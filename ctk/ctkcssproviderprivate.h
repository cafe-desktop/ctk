/* CTK - The GIMP Toolkit
 * Copyright (C) 2011 Red Hat, Inc.
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

#ifndef __CTK_CSS_PROVIDER_PRIVATE_H__
#define __CTK_CSS_PROVIDER_PRIVATE_H__

#include "ctkcssprovider.h"

G_BEGIN_DECLS

gchar *_ctk_get_theme_dir (void);

const gchar *_ctk_css_provider_get_theme_dir (CtkCssProvider *provider);

void   _ctk_css_provider_load_named    (CtkCssProvider *provider,
                                        const gchar    *name,
                                        const gchar    *variant);

void   ctk_css_provider_set_keep_css_sections (void);

G_END_DECLS

#endif /* __CTK_CSS_PROVIDER_PRIVATE_H__ */
