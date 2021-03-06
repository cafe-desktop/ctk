/* CTK - The GIMP Toolkit
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

typedef struct _CtkCssLookup CtkCssLookup;

typedef struct {
  CtkCssSection     *section;
  CtkCssValue       *value;
} CtkCssLookupValue;

struct _CtkCssLookup {
  CtkBitmask        *missing;
  CtkCssLookupValue  values[CTK_CSS_PROPERTY_N_PROPERTIES];
};

CtkCssLookup *          _ctk_css_lookup_new                     (const CtkBitmask           *relevant);
void                    _ctk_css_lookup_free                    (CtkCssLookup               *lookup);

static inline const CtkBitmask *_ctk_css_lookup_get_missing     (const CtkCssLookup         *lookup);
gboolean                _ctk_css_lookup_is_missing              (const CtkCssLookup         *lookup,
                                                                 guint                       id);
void                    _ctk_css_lookup_set                     (CtkCssLookup               *lookup,
                                                                 guint                       id,
                                                                 CtkCssSection              *section,
                                                                 CtkCssValue                *value);
void                    _ctk_css_lookup_resolve                 (CtkCssLookup               *lookup,
                                                                 CtkStyleProviderPrivate    *provider,
                                                                 CtkCssStaticStyle          *style,
                                                                 CtkCssStyle                *parent_style);

static inline const CtkBitmask *
_ctk_css_lookup_get_missing (const CtkCssLookup *lookup)
{
  return lookup->missing;
}



G_END_DECLS

#endif /* __CTK_CSS_LOOKUP_PRIVATE_H__ */
