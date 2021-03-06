/* CTK - The GIMP Toolkit
 * Copyright (C) 2012 Benjamin Otte <otte@gnome.org>
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

#ifndef __CTK_HSLA_PRIVATE_H__
#define __CTK_HSLA_PRIVATE_H__

#include <cdk/cdk.h>

G_BEGIN_DECLS

typedef struct _CtkHSLA CtkHSLA;

struct _CtkHSLA {
  double hue;
  double saturation;
  double lightness;
  double alpha;
};

void            _ctk_hsla_init              (CtkHSLA          *hsla,
                                             double            hue,
                                             double            saturation,
                                             double            lightness,
                                             double            alpha);
void            _ctk_hsla_init_from_rgba    (CtkHSLA          *hsla,
                                             const CdkRGBA    *rgba);
/* Yes, I can name that function like this! */
void            _cdk_rgba_init_from_hsla    (CdkRGBA          *rgba,
                                             const CtkHSLA    *hsla);

void            _ctk_hsla_shade             (CtkHSLA          *dest,
                                             const CtkHSLA    *src,
                                             double            factor);

G_END_DECLS

#endif /* __CTK_HSLA_PRIVATE_H__ */
