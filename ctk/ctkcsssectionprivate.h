/* GTK - The GIMP Toolkit
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

#ifndef __CTK_CSS_SECTION_PRIVATE_H__
#define __CTK_CSS_SECTION_PRIVATE_H__

#include "ctkcsssection.h"

#include "ctkcssparserprivate.h"

G_BEGIN_DECLS

CtkCssSection *    _ctk_css_section_new                (CtkCssSection        *parent,
                                                        CtkCssSectionType     type,
                                                        CtkCssParser         *parser);
CtkCssSection *    _ctk_css_section_new_for_file       (CtkCssSectionType     type,
                                                        GFile                *file);

void               _ctk_css_section_end                (CtkCssSection        *section);

void               _ctk_css_section_print              (const CtkCssSection  *section,
                                                        GString              *string);
char *             _ctk_css_section_to_string          (const CtkCssSection  *section);

G_END_DECLS

#endif /* __CTK_CSS_SECTION_PRIVATE_H__ */
