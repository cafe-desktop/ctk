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

#ifndef __CTK_CSS_SECTION_H__
#define __CTK_CSS_SECTION_H__

#include <gio/gio.h>
#include <cdk/cdk.h>

G_BEGIN_DECLS

#define CTK_TYPE_CSS_SECTION         (ctk_css_section_get_type ())

/**
 * CtkCssSectionType:
 * @CTK_CSS_SECTION_DOCUMENT: The section describes a complete document.
 *   This section time is the only one where ctk_css_section_get_parent()
 *   might return %NULL.
 * @CTK_CSS_SECTION_IMPORT: The section defines an import rule.
 * @CTK_CSS_SECTION_COLOR_DEFINITION: The section defines a color. This
 *   is a CTK extension to CSS.
 * @CTK_CSS_SECTION_BINDING_SET: The section defines a binding set. This
 *   is a CTK extension to CSS.
 * @CTK_CSS_SECTION_RULESET: The section defines a CSS ruleset.
 * @CTK_CSS_SECTION_SELECTOR: The section defines a CSS selector.
 * @CTK_CSS_SECTION_DECLARATION: The section defines the declaration of
 *   a CSS variable.
 * @CTK_CSS_SECTION_VALUE: The section defines the value of a CSS declaration.
 * @CTK_CSS_SECTION_KEYFRAMES: The section defines keyframes. See [CSS
 *   Animations](http://dev.w3.org/csswg/css3-animations/#keyframes) for details. Since 3.6
 *
 * The different types of sections indicate parts of a CSS document as
 * parsed by CTK’s CSS parser. They are oriented towards the
 * [CSS Grammar](http://www.w3.org/TR/CSS21/grammar.html),
 * but may contain extensions.
 *
 * More types might be added in the future as the parser incorporates
 * more features.
 *
 * Since: 3.2
 */
typedef enum
{
  CTK_CSS_SECTION_DOCUMENT,
  CTK_CSS_SECTION_IMPORT,
  CTK_CSS_SECTION_COLOR_DEFINITION,
  CTK_CSS_SECTION_BINDING_SET,
  CTK_CSS_SECTION_RULESET,
  CTK_CSS_SECTION_SELECTOR,
  CTK_CSS_SECTION_DECLARATION,
  CTK_CSS_SECTION_VALUE,
  CTK_CSS_SECTION_KEYFRAMES
} CtkCssSectionType;

/**
 * CtkCssSection:
 *
 * Defines a part of a CSS document. Because sections are nested into
 * one another, you can use ctk_css_section_get_parent() to get the
 * containing region.
 *
 * Since: 3.2
 */
typedef struct _CtkCssSection CtkCssSection;

CDK_AVAILABLE_IN_3_2
GType              ctk_css_section_get_type            (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_3_2
CtkCssSection *    ctk_css_section_ref                 (CtkCssSection        *section);
CDK_AVAILABLE_IN_3_2
void               ctk_css_section_unref               (CtkCssSection        *section);

CDK_AVAILABLE_IN_3_2
CtkCssSectionType  ctk_css_section_get_section_type    (const CtkCssSection  *section);
CDK_AVAILABLE_IN_3_2
CtkCssSection *    ctk_css_section_get_parent          (const CtkCssSection  *section);
CDK_AVAILABLE_IN_3_2
GFile *            ctk_css_section_get_file            (const CtkCssSection  *section);
CDK_AVAILABLE_IN_3_2
guint              ctk_css_section_get_start_line      (const CtkCssSection  *section);
CDK_AVAILABLE_IN_3_2
guint              ctk_css_section_get_start_position  (const CtkCssSection  *section);
CDK_AVAILABLE_IN_3_2
guint              ctk_css_section_get_end_line        (const CtkCssSection  *section);
CDK_AVAILABLE_IN_3_2
guint              ctk_css_section_get_end_position    (const CtkCssSection  *section);

G_END_DECLS

#endif /* __CTK_CSS_SECTION_H__ */
