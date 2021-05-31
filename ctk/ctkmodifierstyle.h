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

#ifndef __CTK_MODIFIER_STYLE_H__
#define __CTK_MODIFIER_STYLE_H__

#include <glib-object.h>
#include <gdk/gdk.h>
#include <ctk/ctk.h>

G_BEGIN_DECLS

#define CTK_TYPE_MODIFIER_STYLE         (_ctk_modifier_style_get_type ())
#define CTK_MODIFIER_STYLE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_MODIFIER_STYLE, CtkModifierStyle))
#define CTK_MODIFIER_STYLE_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST    ((c), CTK_TYPE_MODIFIER_STYLE, CtkModifierStyleClass))
#define CTK_IS_MODIFIER_STYLE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_MODIFIER_STYLE))
#define CTK_IS_MODIFIER_STYLE_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE    ((c), CTK_TYPE_MODIFIER_STYLE))
#define CTK_MODIFIER_STYLE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS  ((o), CTK_TYPE_MODIFIER_STYLE, CtkModifierStyleClass))

typedef struct _CtkModifierStyle CtkModifierStyle;
typedef struct _CtkModifierStyleClass CtkModifierStyleClass;
typedef struct _CtkModifierStylePrivate CtkModifierStylePrivate;

struct _CtkModifierStyle
{
  GObject parent_object;
  CtkModifierStylePrivate *priv;
};

struct _CtkModifierStyleClass
{
  GObjectClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GType _ctk_modifier_style_get_type (void) G_GNUC_CONST;

CtkModifierStyle * _ctk_modifier_style_new (void);

void _ctk_modifier_style_set_background_color (CtkModifierStyle *style,
                                               CtkStateFlags     state,
                                               const GdkRGBA    *color);
void _ctk_modifier_style_set_color            (CtkModifierStyle *style,
                                               CtkStateFlags     state,
                                               const GdkRGBA    *color);
void _ctk_modifier_style_set_font             (CtkModifierStyle           *style,
                                               const PangoFontDescription *font_desc);

void _ctk_modifier_style_map_color            (CtkModifierStyle *style,
                                               const gchar      *name,
                                               const GdkRGBA    *color);

void _ctk_modifier_style_set_color_property   (CtkModifierStyle *style,
                                               GType             widget_type,
                                               const gchar      *prop_name,
                                               const GdkRGBA    *color);

G_END_DECLS

#endif /* __CTK_MODIFIER_STYLE_H__ */
