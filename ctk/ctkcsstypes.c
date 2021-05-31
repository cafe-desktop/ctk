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

#include "ctkcsstypesprivate.h"

#include "ctkcssnumbervalueprivate.h"
#include "ctkstylecontextprivate.h"

cairo_operator_t
_ctk_css_blend_mode_get_operator (CtkCssBlendMode mode)
{
  switch (mode)
    {
    case CTK_CSS_BLEND_MODE_COLOR:
      return CAIRO_OPERATOR_HSL_COLOR;
    case CTK_CSS_BLEND_MODE_COLOR_BURN:
      return CAIRO_OPERATOR_COLOR_BURN;
    case CTK_CSS_BLEND_MODE_COLOR_DODGE:
      return CAIRO_OPERATOR_COLOR_DODGE;
    case CTK_CSS_BLEND_MODE_DARKEN:
      return CAIRO_OPERATOR_DARKEN;
    case CTK_CSS_BLEND_MODE_DIFFERENCE:
      return CAIRO_OPERATOR_DIFFERENCE;
    case CTK_CSS_BLEND_MODE_EXCLUSION:
      return CAIRO_OPERATOR_EXCLUSION;
    case CTK_CSS_BLEND_MODE_HARD_LIGHT:
      return CAIRO_OPERATOR_HARD_LIGHT;
    case CTK_CSS_BLEND_MODE_HUE:
      return CAIRO_OPERATOR_HSL_HUE;
    case CTK_CSS_BLEND_MODE_LIGHTEN:
      return CAIRO_OPERATOR_LIGHTEN;
    case CTK_CSS_BLEND_MODE_LUMINOSITY:
      return CAIRO_OPERATOR_HSL_LUMINOSITY;
    case CTK_CSS_BLEND_MODE_MULTIPLY:
      return CAIRO_OPERATOR_MULTIPLY;
    case CTK_CSS_BLEND_MODE_OVERLAY:
      return CAIRO_OPERATOR_OVERLAY;
    case CTK_CSS_BLEND_MODE_SATURATE:
      return CAIRO_OPERATOR_SATURATE;
    case CTK_CSS_BLEND_MODE_SCREEN:
      return CAIRO_OPERATOR_SCREEN;

    case CTK_CSS_BLEND_MODE_NORMAL:
    default:
      return CAIRO_OPERATOR_OVER;
    }
}

CtkCssChange
_ctk_css_change_for_sibling (CtkCssChange match)
{
#define BASE_STATES ( CTK_CSS_CHANGE_CLASS \
                    | CTK_CSS_CHANGE_NAME \
                    | CTK_CSS_CHANGE_ID \
                    | CTK_CSS_CHANGE_FIRST_CHILD \
                    | CTK_CSS_CHANGE_LAST_CHILD \
                    | CTK_CSS_CHANGE_NTH_CHILD \
                    | CTK_CSS_CHANGE_NTH_LAST_CHILD \
                    | CTK_CSS_CHANGE_STATE )

#define KEEP_STATES ( ~(BASE_STATES|CTK_CSS_CHANGE_SOURCE|CTK_CSS_CHANGE_PARENT_STYLE) \
                    | CTK_CSS_CHANGE_NTH_CHILD \
                    | CTK_CSS_CHANGE_NTH_LAST_CHILD)

#define SIBLING_SHIFT 8

  return (match & KEEP_STATES) | ((match & BASE_STATES) << SIBLING_SHIFT);

#undef BASE_STATES
#undef KEEP_STATES
#undef SIBLING_SHIFT
}

CtkCssChange
_ctk_css_change_for_child (CtkCssChange match)
{
#define BASE_STATES ( CTK_CSS_CHANGE_CLASS \
                    | CTK_CSS_CHANGE_NAME \
                    | CTK_CSS_CHANGE_ID \
                    | CTK_CSS_CHANGE_FIRST_CHILD \
                    | CTK_CSS_CHANGE_LAST_CHILD \
                    | CTK_CSS_CHANGE_NTH_CHILD \
                    | CTK_CSS_CHANGE_NTH_LAST_CHILD \
                    | CTK_CSS_CHANGE_STATE \
                    | CTK_CSS_CHANGE_SIBLING_CLASS \
                    | CTK_CSS_CHANGE_SIBLING_NAME \
                    | CTK_CSS_CHANGE_SIBLING_ID \
                    | CTK_CSS_CHANGE_SIBLING_FIRST_CHILD \
                    | CTK_CSS_CHANGE_SIBLING_LAST_CHILD \
                    | CTK_CSS_CHANGE_SIBLING_NTH_CHILD \
                    | CTK_CSS_CHANGE_SIBLING_NTH_LAST_CHILD \
                    | CTK_CSS_CHANGE_SIBLING_STATE )

#define PARENT_SHIFT 16

  return (match & ~(BASE_STATES|CTK_CSS_CHANGE_SOURCE|CTK_CSS_CHANGE_PARENT_STYLE)) | ((match & BASE_STATES) << PARENT_SHIFT);

#undef BASE_STATES
#undef PARENT_SHIFT
}

void
ctk_css_change_print (CtkCssChange  change,
                      GString      *string)
{
  const struct {
    CtkCssChange flags;
    const char *name;
  } names[] = {
    { CTK_CSS_CHANGE_CLASS, "class" },
    { CTK_CSS_CHANGE_NAME, "name" },
    { CTK_CSS_CHANGE_ID, "id" },
    { CTK_CSS_CHANGE_FIRST_CHILD, "first-child" },
    { CTK_CSS_CHANGE_LAST_CHILD, "last-child" },
    { CTK_CSS_CHANGE_NTH_CHILD, "nth-child" },
    { CTK_CSS_CHANGE_NTH_LAST_CHILD, "nth-last-child" },
    { CTK_CSS_CHANGE_STATE, "state" },
    { CTK_CSS_CHANGE_SIBLING_CLASS, "sibling-class" },
    { CTK_CSS_CHANGE_SIBLING_NAME, "sibling-name" },
    { CTK_CSS_CHANGE_SIBLING_ID, "sibling-id" },
    { CTK_CSS_CHANGE_SIBLING_FIRST_CHILD, "sibling-first-child" },
    { CTK_CSS_CHANGE_SIBLING_LAST_CHILD, "sibling-last-child" },
    { CTK_CSS_CHANGE_SIBLING_NTH_CHILD, "sibling-nth-child" },
    { CTK_CSS_CHANGE_SIBLING_NTH_LAST_CHILD, "sibling-nth-last-child" },
    { CTK_CSS_CHANGE_SIBLING_STATE, "sibling-state" },
    { CTK_CSS_CHANGE_PARENT_CLASS, "parent-class" },
    { CTK_CSS_CHANGE_PARENT_NAME, "parent-name" },
    { CTK_CSS_CHANGE_PARENT_ID, "parent-id" },
    { CTK_CSS_CHANGE_PARENT_FIRST_CHILD, "parent-first-child" },
    { CTK_CSS_CHANGE_PARENT_LAST_CHILD, "parent-last-child" },
    { CTK_CSS_CHANGE_PARENT_NTH_CHILD, "parent-nth-child" },
    { CTK_CSS_CHANGE_PARENT_NTH_LAST_CHILD, "parent-nth-last-child" },
    { CTK_CSS_CHANGE_PARENT_STATE, "parent-state" },
    { CTK_CSS_CHANGE_PARENT_SIBLING_CLASS, "parent-sibling-" },
    { CTK_CSS_CHANGE_PARENT_SIBLING_NAME, "parent-sibling-name" },
    { CTK_CSS_CHANGE_PARENT_SIBLING_ID, "parent-sibling-id" },
    { CTK_CSS_CHANGE_PARENT_SIBLING_FIRST_CHILD, "parent-sibling-first-child" },
    { CTK_CSS_CHANGE_PARENT_SIBLING_LAST_CHILD, "parent-sibling-last-child" },
    { CTK_CSS_CHANGE_PARENT_SIBLING_NTH_CHILD, "parent-sibling-nth-child" },
    { CTK_CSS_CHANGE_PARENT_SIBLING_NTH_LAST_CHILD, "parent-sibling-nth-last-child" },
    { CTK_CSS_CHANGE_PARENT_SIBLING_STATE, "parent-sibling-state" },
    { CTK_CSS_CHANGE_SOURCE, "source" },
    { CTK_CSS_CHANGE_PARENT_STYLE, "parent-style" },
    { CTK_CSS_CHANGE_TIMESTAMP, "timestamp" },
    { CTK_CSS_CHANGE_ANIMATIONS, "animations" },
  };
  guint i;
  gboolean first;

  first = TRUE;

  for (i = 0; i < G_N_ELEMENTS (names); i++)
    {
      if (change & names[i].flags)
        {
          if (first)
            first = FALSE;
          else
            g_string_append (string, "|");
          g_string_append (string, names[i].name);
        }
    }
}

CtkCssDimension
ctk_css_unit_get_dimension (CtkCssUnit unit)
{
  switch (unit)
    {
    case CTK_CSS_NUMBER:
      return CTK_CSS_DIMENSION_NUMBER;

    case CTK_CSS_PERCENT:
      return CTK_CSS_DIMENSION_PERCENTAGE;

    case CTK_CSS_PX:
    case CTK_CSS_PT:
    case CTK_CSS_EM:
    case CTK_CSS_EX:
    case CTK_CSS_REM:
    case CTK_CSS_PC:
    case CTK_CSS_IN:
    case CTK_CSS_CM:
    case CTK_CSS_MM:
      return CTK_CSS_DIMENSION_LENGTH;

    case CTK_CSS_RAD:
    case CTK_CSS_DEG:
    case CTK_CSS_GRAD:
    case CTK_CSS_TURN:
      return CTK_CSS_DIMENSION_ANGLE;

    case CTK_CSS_S:
    case CTK_CSS_MS:
      return CTK_CSS_DIMENSION_TIME;

    default:
      g_assert_not_reached ();
      return CTK_CSS_DIMENSION_PERCENTAGE;
    }
}

char *
ctk_css_change_to_string (CtkCssChange change)
{
  GString *string = g_string_new (NULL);

  ctk_css_change_print (change, string);

  return g_string_free (string, FALSE);
}

