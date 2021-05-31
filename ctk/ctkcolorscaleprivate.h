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

#ifndef __CTK_COLOR_SCALE_H__
#define __CTK_COLOR_SCALE_H__

#include <ctk/ctkscale.h>

G_BEGIN_DECLS

#define CTK_TYPE_COLOR_SCALE            (ctk_color_scale_get_type ())
#define CTK_COLOR_SCALE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_COLOR_SCALE, CtkColorScale))
#define CTK_COLOR_SCALE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_COLOR_SCALE, CtkColorScaleClass))
#define CTK_IS_COLOR_SCALE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_COLOR_SCALE))
#define CTK_IS_COLOR_SCALE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_COLOR_SCALE))
#define CTK_COLOR_SCALE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_COLOR_SCALE, CtkColorScaleClass))


typedef struct _CtkColorScale         CtkColorScale;
typedef struct _CtkColorScaleClass    CtkColorScaleClass;
typedef struct _CtkColorScalePrivate  CtkColorScalePrivate;

struct _CtkColorScale
{
  CtkScale parent_instance;

  CtkColorScalePrivate *priv;
};

struct _CtkColorScaleClass
{
  CtkScaleClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

typedef enum
{
  CTK_COLOR_SCALE_HUE,
  CTK_COLOR_SCALE_ALPHA
} CtkColorScaleType;

GType       ctk_color_scale_get_type (void) G_GNUC_CONST;
CtkWidget * ctk_color_scale_new      (CtkAdjustment     *adjustment,
                                      CtkColorScaleType  type);
void        ctk_color_scale_set_rgba (CtkColorScale     *scale,
                                      const GdkRGBA     *color);

void        ctk_color_scale_draw_trough (CtkColorScale  *scale,
                                         cairo_t        *cr,
                                         int             x,
                                         int             y,
                                         int             width,
                                         int             height);

G_END_DECLS

#endif /* __CTK_COLOR_SCALE_H__ */
