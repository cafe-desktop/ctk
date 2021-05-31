/* GTK - The GIMP Toolkit
 * Copyright (C) 2010 Carlos Garnacho <carlosg@gnome.org>
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

#ifndef __CTK_STYLE_CONTEXT_PRIVATE_H__
#define __CTK_STYLE_CONTEXT_PRIVATE_H__

#include "ctkstylecontext.h"

#include "ctkcssnodeprivate.h"
#include "ctkicontheme.h"
#include "ctkstyleproviderprivate.h"
#include "ctkbitmaskprivate.h"
#include "ctkcssvalueprivate.h"

G_BEGIN_DECLS

CtkStyleContext *ctk_style_context_new_for_node              (CtkCssNode      *node);

CtkCssNode     *ctk_style_context_get_node                   (CtkStyleContext *context);
void            ctk_style_context_set_id                     (CtkStyleContext *context,
                                                              const char      *id);
const char *    ctk_style_context_get_id                     (CtkStyleContext *context);
CtkStyleProviderPrivate *
                ctk_style_context_get_style_provider         (CtkStyleContext *context);

void            ctk_style_context_save_named                 (CtkStyleContext *context,
                                                              const char      *name);
void            ctk_style_context_save_to_node               (CtkStyleContext *context,
                                                              CtkCssNode      *node);

CtkCssStyleChange *
                ctk_style_context_get_change                 (CtkStyleContext *context);

CtkCssStyle *   ctk_style_context_lookup_style               (CtkStyleContext *context);
CtkCssValue   * _ctk_style_context_peek_property             (CtkStyleContext *context,
                                                              guint            property_id);
const GValue * _ctk_style_context_peek_style_property        (CtkStyleContext *context,
                                                              GType            widget_type,
                                                              GParamSpec      *pspec);
void            ctk_style_context_validate                   (CtkStyleContext *context,
                                                              CtkCssStyleChange *change);
void            ctk_style_context_clear_property_cache       (CtkStyleContext *context);
gboolean       _ctk_style_context_check_region_name          (const gchar     *str);

gboolean       _ctk_style_context_resolve_color              (CtkStyleContext    *context,
                                                              CtkCssValue        *color,
                                                              GdkRGBA            *result);
void           _ctk_style_context_get_cursor_color           (CtkStyleContext    *context,
                                                              GdkRGBA            *primary_color,
                                                              GdkRGBA            *secondary_color);

void           _ctk_style_context_get_icon_extents           (CtkStyleContext    *context,
                                                              GdkRectangle       *extents,
                                                              gint                x,
                                                              gint                y,
                                                              gint                width,
                                                              gint                height);

PangoAttrList *_ctk_style_context_get_pango_attributes       (CtkStyleContext *context);

/* Accessibility support */
AtkAttributeSet *_ctk_style_context_get_attributes           (AtkAttributeSet    *attributes,
                                                              CtkStyleContext    *context,
                                                              CtkStateFlags       flags);

G_END_DECLS

#endif /* __CTK_STYLE_CONTEXT_PRIVATE_H__ */
