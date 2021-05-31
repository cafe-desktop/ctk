/*
 * Copyright © 2012 Red Hat Inc.
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
 *
 * Authors: Alexander Larsson <alexl@gnome.org>
 */

#ifndef __CTK_CSS_ENUM_VALUE_PRIVATE_H__
#define __CTK_CSS_ENUM_VALUE_PRIVATE_H__

#include "ctkenums.h"
#include "ctkcssparserprivate.h"
#include "ctkcsstypesprivate.h"
#include "ctkcssvalueprivate.h"

G_BEGIN_DECLS

CtkCssValue *   _ctk_css_blend_mode_value_new         (CtkCssBlendMode    blend_mode);
CtkCssValue *   _ctk_css_blend_mode_value_try_parse   (CtkCssParser      *parser);
CtkCssBlendMode _ctk_css_blend_mode_value_get         (const CtkCssValue *value);

CtkCssValue *   _ctk_css_border_style_value_new       (CtkBorderStyle     border_style);
CtkCssValue *   _ctk_css_border_style_value_try_parse (CtkCssParser      *parser);
CtkBorderStyle  _ctk_css_border_style_value_get       (const CtkCssValue *value);

CtkCssValue *   _ctk_css_font_size_value_new          (CtkCssFontSize     size);
CtkCssValue *   _ctk_css_font_size_value_try_parse    (CtkCssParser      *parser);
CtkCssFontSize  _ctk_css_font_size_value_get          (const CtkCssValue *value);
double          ctk_css_font_size_get_default_px      (CtkStyleProviderPrivate *provider,
                                                       CtkCssStyle             *style);

CtkCssValue *   _ctk_css_font_style_value_new         (PangoStyle         style);
CtkCssValue *   _ctk_css_font_style_value_try_parse   (CtkCssParser      *parser);
PangoStyle      _ctk_css_font_style_value_get         (const CtkCssValue *value);

CtkCssValue *   _ctk_css_font_variant_value_new       (PangoVariant       variant);
CtkCssValue *   _ctk_css_font_variant_value_try_parse (CtkCssParser      *parser);
PangoVariant    _ctk_css_font_variant_value_get       (const CtkCssValue *value);

CtkCssValue *   _ctk_css_font_weight_value_new        (PangoWeight        weight);
CtkCssValue *   _ctk_css_font_weight_value_try_parse  (CtkCssParser      *parser);
PangoWeight     _ctk_css_font_weight_value_get        (const CtkCssValue *value);

CtkCssValue *   _ctk_css_font_stretch_value_new       (PangoStretch       stretch);
CtkCssValue *   _ctk_css_font_stretch_value_try_parse (CtkCssParser      *parser);
PangoStretch    _ctk_css_font_stretch_value_get       (const CtkCssValue *value);

CtkCssValue *         _ctk_css_text_decoration_line_value_new       (CtkTextDecorationLine  line);
CtkCssValue *         _ctk_css_text_decoration_line_value_try_parse (CtkCssParser          *parser);
CtkTextDecorationLine _ctk_css_text_decoration_line_value_get       (const CtkCssValue     *value);

CtkCssValue *          _ctk_css_text_decoration_style_value_new       (CtkTextDecorationStyle  style);
CtkCssValue *          _ctk_css_text_decoration_style_value_try_parse (CtkCssParser           *parser);
CtkTextDecorationStyle _ctk_css_text_decoration_style_value_get       (const CtkCssValue      *value);

CtkCssValue *   _ctk_css_area_value_new               (CtkCssArea         area);
CtkCssValue *   _ctk_css_area_value_try_parse         (CtkCssParser      *parser);
CtkCssArea      _ctk_css_area_value_get               (const CtkCssValue *value);

CtkCssValue *   _ctk_css_direction_value_new          (CtkCssDirection    direction);
CtkCssValue *   _ctk_css_direction_value_try_parse    (CtkCssParser      *parser);
CtkCssDirection _ctk_css_direction_value_get          (const CtkCssValue *value);

CtkCssValue *   _ctk_css_play_state_value_new         (CtkCssPlayState    play_state);
CtkCssValue *   _ctk_css_play_state_value_try_parse   (CtkCssParser      *parser);
CtkCssPlayState _ctk_css_play_state_value_get         (const CtkCssValue *value);

CtkCssValue *   _ctk_css_fill_mode_value_new          (CtkCssFillMode     fill_mode);
CtkCssValue *   _ctk_css_fill_mode_value_try_parse    (CtkCssParser      *parser);
CtkCssFillMode  _ctk_css_fill_mode_value_get          (const CtkCssValue *value);

CtkCssValue *   _ctk_css_icon_effect_value_new        (CtkCssIconEffect   image_effect);
CtkCssValue *   _ctk_css_icon_effect_value_try_parse  (CtkCssParser      *parser);
CtkCssIconEffect _ctk_css_icon_effect_value_get       (const CtkCssValue *value);
void            ctk_css_icon_effect_apply             (CtkCssIconEffect   icon_effect,
                                                       cairo_surface_t   *surface);

CtkCssValue *   _ctk_css_icon_style_value_new         (CtkCssIconStyle    icon_style);
CtkCssValue *   _ctk_css_icon_style_value_try_parse   (CtkCssParser      *parser);
CtkCssIconStyle _ctk_css_icon_style_value_get         (const CtkCssValue *value);

G_END_DECLS

#endif /* __CTK_CSS_ENUM_VALUE_PRIVATE_H__ */
