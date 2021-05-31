/* GTK - The GIMP Toolkit
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

#include "config.h"

#include "gtkcssstylechangeprivate.h"

#include "gtkcssstylepropertyprivate.h"

void
ctk_css_style_change_init (GtkCssStyleChange *change,
                           GtkCssStyle       *old_style,
                           GtkCssStyle       *new_style)
{
  change->old_style = g_object_ref (old_style);
  change->new_style = g_object_ref (new_style);

  change->n_compared = 0;

  change->affects = 0;
  change->changes = _ctk_bitmask_new ();
  
  /* Make sure we don't do extra work if old and new are equal. */
  if (old_style == new_style)
    change->n_compared = CTK_CSS_PROPERTY_N_PROPERTIES;
}

void
ctk_css_style_change_finish (GtkCssStyleChange *change)
{
  g_object_unref (change->old_style);
  g_object_unref (change->new_style);
  _ctk_bitmask_free (change->changes);
}

GtkCssStyle *
ctk_css_style_change_get_old_style (GtkCssStyleChange *change)
{
  return change->old_style;
}

GtkCssStyle *
ctk_css_style_change_get_new_style (GtkCssStyleChange *change)
{
  return change->new_style;
}

static gboolean
ctk_css_style_compare_next_value (GtkCssStyleChange *change)
{
  if (change->n_compared == CTK_CSS_PROPERTY_N_PROPERTIES)
    return FALSE;

  if (!_ctk_css_value_equal (ctk_css_style_get_value (change->old_style, change->n_compared),
                             ctk_css_style_get_value (change->new_style, change->n_compared)))
    {
      change->affects |= _ctk_css_style_property_get_affects (_ctk_css_style_property_lookup_by_id (change->n_compared));
      change->changes = _ctk_bitmask_set (change->changes, change->n_compared, TRUE);
    }

  change->n_compared++;

  return TRUE;
}

gboolean
ctk_css_style_change_has_change (GtkCssStyleChange *change)
{
  do {
    if (!_ctk_bitmask_is_empty (change->changes))
      return TRUE;
  } while (ctk_css_style_compare_next_value (change));

  return FALSE;
}

gboolean
ctk_css_style_change_affects (GtkCssStyleChange *change,
                              GtkCssAffects      affects)
{
  do {
    if (change->affects & affects)
      return TRUE;
  } while (ctk_css_style_compare_next_value (change));

  return FALSE;
}

gboolean
ctk_css_style_change_changes_property (GtkCssStyleChange *change,
                                       guint              id)
{
  while (change->n_compared <= id)
    ctk_css_style_compare_next_value (change);

  return _ctk_bitmask_get (change->changes, id);
}

void
ctk_css_style_change_print (GtkCssStyleChange *change,
                            GString           *string)
{
  int i;
  GtkCssStyle *old = ctk_css_style_change_get_old_style (change);
  GtkCssStyle *new = ctk_css_style_change_get_new_style (change);

  for (i = 0; i < CTK_CSS_PROPERTY_N_PROPERTIES; i ++)
    {
      if (ctk_css_style_change_changes_property (change, i))
        {
          GtkCssStyleProperty *prop;
          GtkCssValue *value;
          const char *name;

          prop = _ctk_css_style_property_lookup_by_id (i);
          name = _ctk_style_property_get_name (CTK_STYLE_PROPERTY (prop));

          value = ctk_css_style_get_value (old, i);
          _ctk_css_value_print (value, string);

          g_string_append_printf (string, "%s: ", name);
          _ctk_css_value_print (value, string);
          g_string_append (string, "\n");

          g_string_append_printf (string, "%s: ", name);
          value = ctk_css_style_get_value (new, i);
          _ctk_css_value_print (value, string);
          g_string_append (string, "\n");
        }
    }

}

char *
ctk_css_style_change_to_string (GtkCssStyleChange *change)
{
  GString *string = g_string_new ("");

  ctk_css_style_change_print (change, string);

  return g_string_free (string, FALSE);
}
