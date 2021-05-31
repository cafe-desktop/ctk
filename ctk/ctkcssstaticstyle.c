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
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#include "config.h"

#include "ctkcssstaticstyleprivate.h"

#include "ctkcssanimationprivate.h"
#include "ctkcssarrayvalueprivate.h"
#include "ctkcssenumvalueprivate.h"
#include "ctkcssinheritvalueprivate.h"
#include "ctkcssinitialvalueprivate.h"
#include "ctkcssnumbervalueprivate.h"
#include "ctkcsssectionprivate.h"
#include "ctkcssshorthandpropertyprivate.h"
#include "ctkcssstringvalueprivate.h"
#include "ctkcssstylepropertyprivate.h"
#include "ctkcsstransitionprivate.h"
#include "ctkprivate.h"
#include "ctksettings.h"
#include "ctkstyleanimationprivate.h"
#include "ctkstylepropertyprivate.h"
#include "ctkstyleproviderprivate.h"

G_DEFINE_TYPE (CtkCssStaticStyle, ctk_css_static_style, CTK_TYPE_CSS_STYLE)

static CtkCssValue *
ctk_css_static_style_get_value (CtkCssStyle *style,
                                guint        id)
{
  CtkCssStaticStyle *sstyle = CTK_CSS_STATIC_STYLE (style);

  if (G_UNLIKELY (id >= CTK_CSS_PROPERTY_N_PROPERTIES))
    {
      CtkCssStyleProperty *prop = _ctk_css_style_property_lookup_by_id (id);

      return _ctk_css_style_property_get_initial_value (prop);
    }

  return sstyle->values[id];
}

static CtkCssSection *
ctk_css_static_style_get_section (CtkCssStyle *style,
                                    guint        id)
{
  CtkCssStaticStyle *sstyle = CTK_CSS_STATIC_STYLE (style);

  if (sstyle->sections == NULL ||
      id >= sstyle->sections->len)
    return NULL;

  return g_ptr_array_index (sstyle->sections, id);
}

static void
ctk_css_static_style_dispose (GObject *object)
{
  CtkCssStaticStyle *style = CTK_CSS_STATIC_STYLE (object);
  guint i;

  for (i = 0; i < CTK_CSS_PROPERTY_N_PROPERTIES; i++)
    {
      if (style->values[i])
        _ctk_css_value_unref (style->values[i]);
    }
  if (style->sections)
    {
      g_ptr_array_unref (style->sections);
      style->sections = NULL;
    }

  G_OBJECT_CLASS (ctk_css_static_style_parent_class)->dispose (object);
}

static void
ctk_css_static_style_class_init (CtkCssStaticStyleClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CtkCssStyleClass *style_class = CTK_CSS_STYLE_CLASS (klass);

  object_class->dispose = ctk_css_static_style_dispose;

  style_class->get_value = ctk_css_static_style_get_value;
  style_class->get_section = ctk_css_static_style_get_section;
}

static void
ctk_css_static_style_init (CtkCssStaticStyle *style)
{
}

static void
maybe_unref_section (gpointer section)
{
  if (section)
    ctk_css_section_unref (section);
}

static void
ctk_css_static_style_set_value (CtkCssStaticStyle *style,
                                guint              id,
                                CtkCssValue       *value,
                                CtkCssSection     *section)
{
  if (style->values[id])
    _ctk_css_value_unref (style->values[id]);
  style->values[id] = _ctk_css_value_ref (value);

  if (style->sections && style->sections->len > id && g_ptr_array_index (style->sections, id))
    {
      ctk_css_section_unref (g_ptr_array_index (style->sections, id));
      g_ptr_array_index (style->sections, id) = NULL;
    }

  if (section)
    {
      if (style->sections == NULL)
        style->sections = g_ptr_array_new_with_free_func (maybe_unref_section);
      if (style->sections->len <= id)
        g_ptr_array_set_size (style->sections, id + 1);

      g_ptr_array_index (style->sections, id) = ctk_css_section_ref (section);
    }
}

static CtkCssStyle *default_style;

static void
clear_default_style (gpointer data)
{
  g_set_object (&default_style, NULL);
}

CtkCssStyle *
ctk_css_static_style_get_default (void)
{
  /* FIXME: This really depends on the screen, but we don't have
   * a screen at hand when we call this function, and in practice,
   * the default style is always replaced by something else
   * before we use it.
   */
  if (default_style == NULL)
    {
      CtkSettings *settings;

      settings = ctk_settings_get_default ();
      default_style = ctk_css_static_style_new_compute (CTK_STYLE_PROVIDER_PRIVATE (settings),
                                                        NULL,
                                                        NULL);
      g_object_set_data_full (G_OBJECT (settings), "ctk-default-style",
                              default_style, clear_default_style);
    }

  return default_style;
}

CtkCssStyle *
ctk_css_static_style_new_compute (CtkStyleProviderPrivate *provider,
                                  const CtkCssMatcher     *matcher,
                                  CtkCssStyle             *parent)
{
  CtkCssStaticStyle *result;
  CtkCssLookup *lookup;
  CtkCssChange change = CTK_CSS_CHANGE_ANY_SELF | CTK_CSS_CHANGE_ANY_SIBLING | CTK_CSS_CHANGE_ANY_PARENT;

  lookup = _ctk_css_lookup_new (NULL);

  if (matcher)
    _ctk_style_provider_private_lookup (provider,
                                        matcher,
                                        lookup,
                                        &change);

  result = g_object_new (CTK_TYPE_CSS_STATIC_STYLE, NULL);

  result->change = change;

  _ctk_css_lookup_resolve (lookup,
                           provider,
                           result,
                           parent);

  _ctk_css_lookup_free (lookup);

  return CTK_CSS_STYLE (result);
}

void
ctk_css_static_style_compute_value (CtkCssStaticStyle       *style,
                                    CtkStyleProviderPrivate *provider,
                                    CtkCssStyle             *parent_style,
                                    guint                    id,
                                    CtkCssValue             *specified,
                                    CtkCssSection           *section)
{
  CtkCssValue *value;

  ctk_internal_return_if_fail (CTK_IS_CSS_STATIC_STYLE (style));
  ctk_internal_return_if_fail (CTK_IS_STYLE_PROVIDER_PRIVATE (provider));
  ctk_internal_return_if_fail (parent_style == NULL || CTK_IS_CSS_STYLE (parent_style));
  ctk_internal_return_if_fail (id < CTK_CSS_PROPERTY_N_PROPERTIES);

  /* http://www.w3.org/TR/css3-cascade/#cascade
   * Then, for every element, the value for each property can be found
   * by following this pseudo-algorithm:
   * 1) Identify all declarations that apply to the element
   */
  if (specified == NULL)
    {
      CtkCssStyleProperty *prop = _ctk_css_style_property_lookup_by_id (id);

      if (_ctk_css_style_property_is_inherit (prop))
        specified = _ctk_css_inherit_value_new ();
      else
        specified = _ctk_css_initial_value_new ();
    }
  else
    _ctk_css_value_ref (specified);

  value = _ctk_css_value_compute (specified, id, provider, CTK_CSS_STYLE (style), parent_style);

  ctk_css_static_style_set_value (style, id, value, section);

  _ctk_css_value_unref (value);
  _ctk_css_value_unref (specified);
}

CtkCssChange
ctk_css_static_style_get_change (CtkCssStaticStyle *style)
{
  g_return_val_if_fail (CTK_IS_CSS_STATIC_STYLE (style), CTK_CSS_CHANGE_ANY);

  return style->change;
}
