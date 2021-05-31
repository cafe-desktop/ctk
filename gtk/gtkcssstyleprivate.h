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
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#ifndef __CTK_CSS_STYLE_PRIVATE_H__
#define __CTK_CSS_STYLE_PRIVATE_H__

#include <glib-object.h>

#include "gtk/gtkbitmaskprivate.h"
#include "gtk/gtkcsssection.h"
#include "gtk/gtkcssvalueprivate.h"

G_BEGIN_DECLS

#define CTK_TYPE_CSS_STYLE           (ctk_css_style_get_type ())
#define CTK_CSS_STYLE(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, CTK_TYPE_CSS_STYLE, GtkCssStyle))
#define CTK_CSS_STYLE_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, CTK_TYPE_CSS_STYLE, GtkCssStyleClass))
#define CTK_IS_CSS_STYLE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, CTK_TYPE_CSS_STYLE))
#define CTK_IS_CSS_STYLE_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, CTK_TYPE_CSS_STYLE))
#define CTK_CSS_STYLE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CSS_STYLE, GtkCssStyleClass))

/* typedef struct _GtkCssStyle           GtkCssStyle; */
typedef struct _GtkCssStyleClass      GtkCssStyleClass;

struct _GtkCssStyle
{
  GObject parent;
};

struct _GtkCssStyleClass
{
  GObjectClass parent_class;

  /* Get the value for the given property id. This needs to be FAST. */
  GtkCssValue *         (* get_value)                           (GtkCssStyle            *style,
                                                                 guint                   id);
  /* Get the section the value at the given id was declared at or NULL if unavailable.
   * Optional: default impl will just return NULL */
  GtkCssSection *       (* get_section)                         (GtkCssStyle            *style,
                                                                 guint                   id);
  /* TRUE if this style will require changes based on timestamp */
  gboolean              (* is_static)                           (GtkCssStyle            *style);
};

GType                   ctk_css_style_get_type                  (void) G_GNUC_CONST;

GtkCssValue *           ctk_css_style_get_value                 (GtkCssStyle            *style,
                                                                 guint                   id);
GtkCssSection *         ctk_css_style_get_section               (GtkCssStyle            *style,
                                                                 guint                   id);
GtkBitmask *            ctk_css_style_add_difference            (GtkBitmask             *accumulated,
                                                                 GtkCssStyle            *style,
                                                                 GtkCssStyle            *other);
gboolean                ctk_css_style_is_static                 (GtkCssStyle            *style);

char *                  ctk_css_style_to_string                 (GtkCssStyle            *style);
gboolean                ctk_css_style_print                     (GtkCssStyle            *style,
                                                                 GString                *string,
                                                                 guint                   indent,
                                                                 gboolean                skip_initial);
PangoAttrList *         ctk_css_style_get_pango_attributes      (GtkCssStyle            *style);

PangoFontDescription *  ctk_css_style_get_pango_font            (GtkCssStyle            *style);

G_END_DECLS

#endif /* __CTK_CSS_STYLE_PRIVATE_H__ */
