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

#include "config.h"

#include "ctkcsslookupprivate.h"

#include "ctkcssstylepropertyprivate.h"
#include "ctkcsstypesprivate.h"
#include "ctkprivatetypebuiltins.h"
#include "ctkprivate.h"

CtkCssLookup *
_ctk_css_lookup_new (const CtkBitmask *relevant)
{
  CtkCssLookup *lookup;

  lookup = g_malloc0 (sizeof (CtkCssLookup));

  if (relevant)
    {
      lookup->missing = _ctk_bitmask_copy (relevant);
    }
  else
    {
      lookup->missing = _ctk_bitmask_new ();
      lookup->missing = _ctk_bitmask_invert_range (lookup->missing, 0, CTK_CSS_PROPERTY_N_PROPERTIES);
    }

  return lookup;
}

void
_ctk_css_lookup_free (CtkCssLookup *lookup)
{
  ctk_internal_return_if_fail (lookup != NULL);

  _ctk_bitmask_free (lookup->missing);
  g_free (lookup);
}

gboolean
_ctk_css_lookup_is_missing (const CtkCssLookup *lookup,
                            guint               id)
{
  ctk_internal_return_val_if_fail (lookup != NULL, FALSE);

  return _ctk_bitmask_get (lookup->missing, id);
}

/**
 * _ctk_css_lookup_set:
 * @lookup: the lookup
 * @id: id of the property to set, see _ctk_style_property_get_id()
 * @section: (allow-none): The @section the value was defined in or %NULL
 * @value: the “cascading value” to use
 *
 * Sets the @value for a given @id. No value may have been set for @id
 * before. See _ctk_css_lookup_is_missing(). This function is used to
 * set the “winning declaration” of a lookup. Note that for performance
 * reasons @value and @section are not copied. It is your responsibility
 * to ensure they are kept alive until _ctk_css_lookup_free() is called.
 **/
void
_ctk_css_lookup_set (CtkCssLookup  *lookup,
                     guint          id,
                     CtkCssSection *section,
                     CtkCssValue   *value)
{
  ctk_internal_return_if_fail (lookup != NULL);
  ctk_internal_return_if_fail (_ctk_bitmask_get (lookup->missing, id));
  ctk_internal_return_if_fail (value != NULL);

  lookup->missing = _ctk_bitmask_set (lookup->missing, id, FALSE);
  lookup->values[id].value = value;
  lookup->values[id].section = section;
}

/**
 * _ctk_css_lookup_resolve:
 * @lookup: the lookup
 * @context: the context the values are resolved for
 * @values: a new #CtkCssStyle to be filled with the new properties
 *
 * Resolves the current lookup into a styleproperties object. This is done
 * by converting from the “winning declaration” to the “computed value”.
 *
 * XXX: This bypasses the notion of “specified value”. If this ever becomes
 * an issue, go fix it.
 **/
void
_ctk_css_lookup_resolve (CtkCssLookup            *lookup,
                         CtkStyleProviderPrivate *provider,
                         CtkCssStaticStyle       *style,
                         CtkCssStyle             *parent_style)
{
  guint i;

  ctk_internal_return_if_fail (lookup != NULL);
  ctk_internal_return_if_fail (CTK_IS_STYLE_PROVIDER_PRIVATE (provider));
  ctk_internal_return_if_fail (CTK_IS_CSS_STATIC_STYLE (style));
  ctk_internal_return_if_fail (parent_style == NULL || CTK_IS_CSS_STYLE (parent_style));

  for (i = 0; i < CTK_CSS_PROPERTY_N_PROPERTIES; i++)
    {
      if (lookup->values[i].value ||
          _ctk_bitmask_get (lookup->missing, i))
        ctk_css_static_style_compute_value (style,
                                            provider,
                                            parent_style,
                                            i,
                                            lookup->values[i].value,
                                            lookup->values[i].section);
      /* else not a relevant property */
    }
}
