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

/* A GtkCssValue is a refcounted immutable value type */

typedef struct _GtkCssValue           GtkCssValue;
typedef struct _GtkCssValueClass      GtkCssValueClass;

/* using define instead of struct here so compilers get the packing right */
#define CTK_CSS_VALUE_BASE \
  const GtkCssValueClass *class; \
  gint ref_count;

struct _GtkCssValueClass {
  void          (* free)                              (GtkCssValue                *value);

  GtkCssValue * (* compute)                           (GtkCssValue                *value,
                                                       guint                       property_id,
                                                       GtkStyleProviderPrivate    *provider,
                                                       GtkCssStyle                *style,
                                                       GtkCssStyle                *parent_style);
  gboolean      (* equal)                             (const GtkCssValue          *value1,
                                                       const GtkCssValue          *value2);
  GtkCssValue * (* transition)                        (GtkCssValue                *start,
                                                       GtkCssValue                *end,
                                                       guint                       property_id,
                                                       double                      progress);
  void          (* print)                             (const GtkCssValue          *value,
                                                       GString                    *string);
};

GType        _ctk_css_value_get_type                  (void) G_GNUC_CONST;

GtkCssValue *_ctk_css_value_alloc                     (const GtkCssValueClass     *klass,
                                                       gsize                       size);
#define _ctk_css_value_new(_name, _klass) ((_name *) _ctk_css_value_alloc ((_klass), sizeof (_name)))

GtkCssValue *_ctk_css_value_ref                       (GtkCssValue                *value);
void         _ctk_css_value_unref                     (GtkCssValue                *value);

GtkCssValue *_ctk_css_value_compute                   (GtkCssValue                *value,
                                                       guint                       property_id,
                                                       GtkStyleProviderPrivate    *provider,
                                                       GtkCssStyle                *style,
                                                       GtkCssStyle                *parent_style);
gboolean     _ctk_css_value_equal                     (const GtkCssValue          *value1,
                                                       const GtkCssValue          *value2);
gboolean     _ctk_css_value_equal0                    (const GtkCssValue          *value1,
                                                       const GtkCssValue          *value2);
GtkCssValue *_ctk_css_value_transition                (GtkCssValue                *start,
                                                       GtkCssValue                *end,
                                                       guint                       property_id,
                                                       double                      progress);

char *       _ctk_css_value_to_string                 (const GtkCssValue          *value);
void         _ctk_css_value_print                     (const GtkCssValue          *value,
                                                       GString                    *string);

G_END_DECLS

#endif /* __CTK_CSS_VALUE_PRIVATE_H__ */
