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

#ifndef __CTK_CSS_LOOKUP_PRIVATE_H__
#define __CTK_CSS_LOOKUP_PRIVATE_H__

#include <glib-object.h>
#include "ctk/ctkbitmaskprivate.h"
#include "ctk/ctkcssstaticstyleprivate.h"
#include "ctk/ctkcsssection.h"


G_BEGIN_DECLS

typedef struct _GtkCssLookup GtkCssLookup;

typedef struct {
  GtkCssSection     *section;
  GtkCssValue       *value;
} GtkCssLookupValue;

struct _GtkCssLookup {
  GtkBitmask        *missing;
  GtkCssLookupValue  values[CTK_CSS_PROPERTY_N_PROPERTIES];
};

GtkCssLookup *          _ctk_css_lookup_new                     (const GtkBitmask           *relevant);
void                    _ctk_css_lookup_free                    (GtkCssLookup               *lookup);

static inline const GtkBitmask *_ctk_css_lookup_get_missing     (const GtkCssLookup         *lookup);
gboolean                _ctk_css_lookup_is_missing              (const GtkCssLookup         *lookup,
                                                                 guint                       id);
void                    _ctk_css_lookup_set                     (GtkCssLookup               *lookup,
                                                                 guint                       id,
                                                                 GtkCssSection              *section,
                                                                 GtkCssValue                *value);
void                    _ctk_css_lookup_resolve                 (GtkCssLookup               *lookup,
                                                                 GtkStyleProviderPrivate    *provider,
                                                                 GtkCssStaticStyle          *style,
                                                                 GtkCssStyle                *parent_style);

static inline const GtkBitmask *
_ctk_css_lookup_get_missing (const GtkCssLookup *lookup)
{
  return lookup->missing;
}



G_END_DECLS

#endif /* __CTK_CSS_LOOKUP_PRIVATE_H__ */
