/* GTK - The GIMP Toolkit
 * Copyright (C) 2012 Red Hat, Inc.
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

#ifndef __CTK_COLOR_SWATCH_PRIVATE_H__
#define __CTK_COLOR_SWATCH_PRIVATE_H__

#include <ctk/ctkdrawingarea.h>

G_BEGIN_DECLS

#define CTK_TYPE_COLOR_SWATCH                  (ctk_color_swatch_get_type ())
#define CTK_COLOR_SWATCH(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_COLOR_SWATCH, CtkColorSwatch))
#define CTK_COLOR_SWATCH_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_COLOR_SWATCH, CtkColorSwatchClass))
#define CTK_IS_COLOR_SWATCH(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_COLOR_SWATCH))
#define CTK_IS_COLOR_SWATCH_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_COLOR_SWATCH))
#define CTK_COLOR_SWATCH_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_COLOR_SWATCH, CtkColorSwatchClass))


typedef struct _CtkColorSwatch        CtkColorSwatch;
typedef struct _CtkColorSwatchClass   CtkColorSwatchClass;
typedef struct _CtkColorSwatchPrivate CtkColorSwatchPrivate;

struct _CtkColorSwatch
{
  CtkWidget parent;

  /*< private >*/
  CtkColorSwatchPrivate *priv;
};

struct _CtkColorSwatchClass
{
  CtkWidgetClass parent_class;

  void ( * activate)  (CtkColorSwatch *swatch);
  void ( * customize) (CtkColorSwatch *swatch);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


GType       ctk_color_swatch_get_type         (void) G_GNUC_CONST;
CtkWidget * ctk_color_swatch_new              (void);
void        ctk_color_swatch_set_rgba         (CtkColorSwatch *swatch,
                                               const GdkRGBA  *color);
gboolean    ctk_color_swatch_get_rgba         (CtkColorSwatch *swatch,
                                               GdkRGBA        *color);
void        ctk_color_swatch_set_hsva         (CtkColorSwatch *swatch,
                                               gdouble         h,
                                               gdouble         s,
                                               gdouble         v,
                                               gdouble         a);
void        ctk_color_swatch_set_can_drop     (CtkColorSwatch *swatch,
                                               gboolean        can_drop);
void        ctk_color_swatch_set_icon         (CtkColorSwatch *swatch,
                                               const gchar    *icon);
void        ctk_color_swatch_set_use_alpha    (CtkColorSwatch *swatch,
                                               gboolean        use_alpha);
void        ctk_color_swatch_set_selectable   (CtkColorSwatch *swatch,
                                               gboolean        selectable);
gboolean    ctk_color_swatch_get_selectable   (CtkColorSwatch *swatch);

G_END_DECLS

#endif /* __CTK_COLOR_SWATCH_PRIVATE_H__ */
