/* CTK - The GIMP Toolkit
 * Copyright © 2013 Carlos Garnacho <carlosg@gnome.org>
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

#ifndef __CTK_MAGNIFIER_H__
#define __CTK_MAGNIFIER_H__

G_BEGIN_DECLS

#define CTK_TYPE_MAGNIFIER           (_ctk_magnifier_get_type ())
#define CTK_MAGNIFIER(o)             (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_MAGNIFIER, CtkMagnifier))
#define CTK_MAGNIFIER_CLASS(c)       (G_TYPE_CHECK_CLASS_CAST ((c), CTK_TYPE_MAGNIFIER, CtkMagnifierClass))
#define CTK_IS_MAGNIFIER(o)          (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_MAGNIFIER))
#define CTK_IS_MAGNIFIER_CLASS(o)    (G_TYPE_CHECK_CLASS_TYPE ((o), CTK_TYPE_MAGNIFIER))
#define CTK_MAGNIFIER_GET_CLASS(o)   (G_TYPE_INSTANCE_GET_CLASS ((o), CTK_TYPE_MAGNIFIER, CtkMagnifierClass))

typedef struct _CtkMagnifier CtkMagnifier;
typedef struct _CtkMagnifierClass CtkMagnifierClass;

struct _CtkMagnifier
{
  CtkWidget parent_instance;
};

struct _CtkMagnifierClass
{
  CtkWidgetClass parent_class;
};

GType       _ctk_magnifier_get_type          (void) G_GNUC_CONST;

CtkWidget * _ctk_magnifier_new               (CtkWidget       *inspected);

CtkWidget * _ctk_magnifier_get_inspected     (CtkMagnifier *magnifier);
void        _ctk_magnifier_set_inspected     (CtkMagnifier *magnifier,
                                              CtkWidget    *inspected);

void        _ctk_magnifier_set_coords        (CtkMagnifier *magnifier,
                                              gdouble       x,
                                              gdouble       y);
void        _ctk_magnifier_get_coords        (CtkMagnifier *magnifier,
                                              gdouble      *x,
                                              gdouble      *y);

void        _ctk_magnifier_set_magnification (CtkMagnifier *magnifier,
                                              gdouble       magnification);
gdouble     _ctk_magnifier_get_magnification (CtkMagnifier *magnifier);

void        _ctk_magnifier_set_resize        (CtkMagnifier *magnifier,
                                              gboolean      resize);
gboolean    _ctk_magnifier_get_resize        (CtkMagnifier *magnifier);

G_END_DECLS

#endif /* __CTK_MAGNIFIER_H__ */
