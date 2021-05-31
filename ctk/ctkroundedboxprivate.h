/* GTK - The GIMP Toolkit
 * Copyright (C) 2011 Benjamin Otte <otte@gnome.org>
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

#ifndef __CTK_ROUNDED_BOX_PRIVATE_H__
#define __CTK_ROUNDED_BOX_PRIVATE_H__

#include <glib-object.h>
#include <cairo.h>
#include <ctk/ctkenums.h>
#include <ctk/ctktypes.h>

#include "ctkcsstypesprivate.h"

G_BEGIN_DECLS

typedef struct _CtkRoundedBox CtkRoundedBox;
typedef struct _CtkRoundedBoxCorner CtkRoundedBoxCorner;

struct _CtkRoundedBoxCorner {
  double                   horizontal;
  double                   vertical;
};

struct _CtkRoundedBox {
  /*< private >*/
  cairo_rectangle_t        box;
  CtkRoundedBoxCorner      corner[4];
};

void            _ctk_rounded_box_init_rect                      (CtkRoundedBox       *box,
                                                                 double               x,
                                                                 double               y,
                                                                 double               width,
                                                                 double               height);

void            _ctk_rounded_box_apply_border_radius_for_style  (CtkRoundedBox       *box,
                                                                 CtkCssStyle         *style,
                                                                 CtkJunctionSides     junction);

void            _ctk_rounded_box_apply_outline_radius_for_style (CtkRoundedBox       *box,
                                                                 CtkCssStyle         *style,
                                                                 CtkJunctionSides     junction);

void            _ctk_rounded_box_grow                           (CtkRoundedBox       *box,
                                                                 double               top,
                                                                 double               right,
                                                                 double               bottom,
                                                                 double               left);
void            _ctk_rounded_box_shrink                         (CtkRoundedBox       *box,
                                                                 double               top,
                                                                 double               right,
                                                                 double               bottom,
                                                                 double               left);
void            _ctk_rounded_box_move                           (CtkRoundedBox       *box,
                                                                 double               dx,
                                                                 double               dy);

double          _ctk_rounded_box_guess_length                   (const CtkRoundedBox *box,
                                                                 CtkCssSide           side);

void            _ctk_rounded_box_path                           (const CtkRoundedBox *box,
                                                                 cairo_t             *cr);
void            _ctk_rounded_box_path_side                      (const CtkRoundedBox *box,
                                                                 cairo_t             *cr,
                                                                 CtkCssSide           side);
void            _ctk_rounded_box_path_top                       (const CtkRoundedBox *outer,
                                                                 const CtkRoundedBox *inner,
                                                                 cairo_t             *cr);
void            _ctk_rounded_box_path_right                     (const CtkRoundedBox *outer,
                                                                 const CtkRoundedBox *inner,
                                                                 cairo_t             *cr);
void            _ctk_rounded_box_path_bottom                    (const CtkRoundedBox *outer,
                                                                 const CtkRoundedBox *inner,
                                                                 cairo_t             *cr);
void            _ctk_rounded_box_path_left                      (const CtkRoundedBox *outer,
                                                                 const CtkRoundedBox *inner,
                                                                 cairo_t             *cr);
void            _ctk_rounded_box_clip_path                      (const CtkRoundedBox *box,
                                                                 cairo_t             *cr);
gboolean        _ctk_rounded_box_intersects_rectangle           (const CtkRoundedBox *box,
                                                                 gdouble              x1,
                                                                 gdouble              y1,
                                                                 gdouble              x2,
                                                                 gdouble              y2);
gboolean        _ctk_rounded_box_contains_rectangle             (const CtkRoundedBox *box,
                                                                 gdouble              x1,
                                                                 gdouble              y1,
                                                                 gdouble              x2,
                                                                 gdouble              y2);

G_END_DECLS

#endif /* __CTK_ROUNDED_BOX_PRIVATE_H__ */
