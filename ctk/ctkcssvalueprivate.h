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

#ifndef __CTK_CSS_VALUE_PRIVATE_H__
#define __CTK_CSS_VALUE_PRIVATE_H__

#include <glib-object.h>
#include "ctkcsstypesprivate.h"

G_BEGIN_DECLS

#define CTK_TYPE_CSS_VALUE           (_ctk_css_value_get_type ())

/* A CtkCssValue is a refcounted immutable value type */

typedef struct _CtkCssValue           CtkCssValue;
typedef struct _CtkCssValueClass      CtkCssValueClass;

/* using define instead of struct here so compilers get the packing right */
#define CTK_CSS_VALUE_BASE \
  const CtkCssValueClass *class; \
  gint ref_count;

struct _CtkCssValueClass {
  void          (* free)                              (CtkCssValue                *value);

  CtkCssValue * (* compute)                           (CtkCssValue                *value,
                                                       guint                       property_id,
                                                       CtkStyleProviderPrivate    *provider,
                                                       CtkCssStyle                *style,
                                                       CtkCssStyle                *parent_style);
  gboolean      (* equal)                             (const CtkCssValue          *value1,
                                                       const CtkCssValue          *value2);
  CtkCssValue * (* transition)                        (CtkCssValue                *start,
                                                       CtkCssValue                *end,
                                                       guint                       property_id,
                                                       double                      progress);
  void          (* print)                             (const CtkCssValue          *value,
                                                       GString                    *string);
};

GType        _ctk_css_value_get_type                  (void) G_GNUC_CONST;

CtkCssValue *_ctk_css_value_alloc                     (const CtkCssValueClass     *klass,
                                                       gsize                       size);
#define _ctk_css_value_new(_name, _klass) ((_name *) _ctk_css_value_alloc ((_klass), sizeof (_name)))

CtkCssValue *_ctk_css_value_ref                       (CtkCssValue                *value);
void         _ctk_css_value_unref                     (CtkCssValue                *value);

CtkCssValue *_ctk_css_value_compute                   (CtkCssValue                *value,
                                                       guint                       property_id,
                                                       CtkStyleProviderPrivate    *provider,
                                                       CtkCssStyle                *style,
                                                       CtkCssStyle                *parent_style);
gboolean     _ctk_css_value_equal                     (const CtkCssValue          *value1,
                                                       const CtkCssValue          *value2);
gboolean     _ctk_css_value_equal0                    (const CtkCssValue          *value1,
                                                       const CtkCssValue          *value2);
CtkCssValue *_ctk_css_value_transition                (CtkCssValue                *start,
                                                       CtkCssValue                *end,
                                                       guint                       property_id,
                                                       double                      progress);

char *       _ctk_css_value_to_string                 (const CtkCssValue          *value);
void         _ctk_css_value_print                     (const CtkCssValue          *value,
                                                       GString                    *string);

G_END_DECLS

#endif /* __CTK_CSS_VALUE_PRIVATE_H__ */
