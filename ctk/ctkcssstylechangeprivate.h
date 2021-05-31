/* CTK - The GIMP Toolkit
 * Copyright (C) 2015 Benjamin Otte <otte@gnome.org>
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

#ifndef __CTK_CSS_STYLE_CHANGE_PRIVATE_H__
#define __CTK_CSS_STYLE_CHANGE_PRIVATE_H__

#include "ctkcssstyleprivate.h"

G_BEGIN_DECLS

typedef struct _CtkCssStyleChange CtkCssStyleChange;

struct _CtkCssStyleChange {
  CtkCssStyle   *old_style;
  CtkCssStyle   *new_style;

  guint          n_compared;

  CtkCssAffects  affects;
  CtkBitmask    *changes;
};

void            ctk_css_style_change_init               (CtkCssStyleChange      *change,
                                                         CtkCssStyle            *old_style,
                                                         CtkCssStyle            *new_style);
void            ctk_css_style_change_finish             (CtkCssStyleChange      *change);

CtkCssStyle *   ctk_css_style_change_get_old_style      (CtkCssStyleChange      *change);
CtkCssStyle *   ctk_css_style_change_get_new_style      (CtkCssStyleChange      *change);

gboolean        ctk_css_style_change_has_change         (CtkCssStyleChange      *change);
gboolean        ctk_css_style_change_affects            (CtkCssStyleChange      *change,
                                                         CtkCssAffects           affects);
gboolean        ctk_css_style_change_changes_property   (CtkCssStyleChange      *change,
                                                         guint                   id);
void            ctk_css_style_change_print              (CtkCssStyleChange      *change, GString *string);

char *          ctk_css_style_change_to_string          (CtkCssStyleChange      *change);
G_END_DECLS

#endif /* __CTK_CSS_STYLE_CHANGE_PRIVATE_H__ */
