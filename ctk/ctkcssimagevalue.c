/* GTK - The GIMP Toolkit
 * Copyright (C) 2011 Red Hat, Inc.
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

#include "gtkcssimagevalueprivate.h"

#include "gtkcssimagecrossfadeprivate.h"

struct _GtkCssValue {
  CTK_CSS_VALUE_BASE
  GtkCssImage *image;
};

static void
ctk_css_value_image_free (GtkCssValue *value)
{
  g_object_unref (value->image);
  g_slice_free (GtkCssValue, value);
}

static GtkCssValue *
ctk_css_value_image_compute (GtkCssValue             *value,
                             guint                    property_id,
                             GtkStyleProviderPrivate *provider,
                             GtkCssStyle             *style,
                             GtkCssStyle             *parent_style)
{
  GtkCssImage *image, *computed;
  
  image = _ctk_css_image_value_get_image (value);

  if (image == NULL)
    return _ctk_css_value_ref (value);

  computed = _ctk_css_image_compute (image, property_id, provider, style, parent_style);

  if (computed == image)
    {
      g_object_unref (computed);
      return _ctk_css_value_ref (value);
    }

  return _ctk_css_image_value_new (computed);
}

static gboolean
ctk_css_value_image_equal (const GtkCssValue *value1,
                           const GtkCssValue *value2)
{
  return _ctk_css_image_equal (value1->image, value2->image);
}

static GtkCssValue *
ctk_css_value_image_transition (GtkCssValue *start,
                                GtkCssValue *end,
                                guint        property_id,
                                double       progress)
{
  GtkCssImage *transition;

  transition = _ctk_css_image_transition (_ctk_css_image_value_get_image (start),
                                          _ctk_css_image_value_get_image (end),
                                          property_id,
                                          progress);
      
  return _ctk_css_image_value_new (transition);
}

static void
ctk_css_value_image_print (const GtkCssValue *value,
                           GString           *string)
{
  if (value->image)
    _ctk_css_image_print (value->image, string);
  else
    g_string_append (string, "none");
}

static const GtkCssValueClass CTK_CSS_VALUE_IMAGE = {
  ctk_css_value_image_free,
  ctk_css_value_image_compute,
  ctk_css_value_image_equal,
  ctk_css_value_image_transition,
  ctk_css_value_image_print
};

GtkCssValue *
_ctk_css_image_value_new (GtkCssImage *image)
{
  static GtkCssValue none_singleton = { &CTK_CSS_VALUE_IMAGE, 1, NULL };
  GtkCssValue *value;

  if (image == NULL)
    return _ctk_css_value_ref (&none_singleton);

  value = _ctk_css_value_new (GtkCssValue, &CTK_CSS_VALUE_IMAGE);
  value->image = image;

  return value;
}

GtkCssImage *
_ctk_css_image_value_get_image (const GtkCssValue *value)
{
  g_return_val_if_fail (value->class == &CTK_CSS_VALUE_IMAGE, NULL);

  return value->image;
}

