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

#include "config.h"

#include "ctkimagedefinitionprivate.h"

#include "deprecated/ctkiconfactory.h"

typedef struct _CtkImageDefinitionEmpty CtkImageDefinitionEmpty;
typedef struct _CtkImageDefinitionPixbuf CtkImageDefinitionPixbuf;
typedef struct _CtkImageDefinitionStock CtkImageDefinitionStock;
typedef struct _CtkImageDefinitionIconSet CtkImageDefinitionIconSet;
typedef struct _CtkImageDefinitionAnimation CtkImageDefinitionAnimation;
typedef struct _CtkImageDefinitionIconName CtkImageDefinitionIconName;
typedef struct _CtkImageDefinitionGIcon CtkImageDefinitionGIcon;
typedef struct _CtkImageDefinitionSurface CtkImageDefinitionSurface;

struct _CtkImageDefinitionEmpty {
  CtkImageType type;
  gint ref_count;
};

struct _CtkImageDefinitionPixbuf {
  CtkImageType type;
  gint ref_count;

  CdkPixbuf *pixbuf;
  int scale;
};

struct _CtkImageDefinitionStock {
  CtkImageType type;
  gint ref_count;

  char *id;
};

struct _CtkImageDefinitionIconSet {
  CtkImageType type;
  gint ref_count;

  CtkIconSet *icon_set;
};

struct _CtkImageDefinitionAnimation {
  CtkImageType type;
  gint ref_count;

  CdkPixbufAnimation *animation;
  int scale;
};

struct _CtkImageDefinitionIconName {
  CtkImageType type;
  gint ref_count;

  char *icon_name;
};

struct _CtkImageDefinitionGIcon {
  CtkImageType type;
  gint ref_count;

  GIcon *gicon;
};

struct _CtkImageDefinitionSurface {
  CtkImageType type;
  gint ref_count;

  cairo_surface_t *surface;
};

union _CtkImageDefinition
{
  CtkImageType type;
  CtkImageDefinitionEmpty empty;
  CtkImageDefinitionPixbuf pixbuf;
  CtkImageDefinitionStock stock;
  CtkImageDefinitionIconSet icon_set;
  CtkImageDefinitionAnimation animation;
  CtkImageDefinitionIconName icon_name;
  CtkImageDefinitionGIcon gicon;
  CtkImageDefinitionSurface surface;
};

CtkImageDefinition *
ctk_image_definition_new_empty (void)
{
  static CtkImageDefinitionEmpty empty = { CTK_IMAGE_EMPTY, 1 };

  return ctk_image_definition_ref ((CtkImageDefinition *) &empty);
}

static inline CtkImageDefinition *
ctk_image_definition_alloc (CtkImageType type)
{
  static gsize sizes[] = {
    sizeof (CtkImageDefinitionEmpty),
    sizeof (CtkImageDefinitionPixbuf),
    sizeof (CtkImageDefinitionStock),
    sizeof (CtkImageDefinitionIconSet),
    sizeof (CtkImageDefinitionAnimation),
    sizeof (CtkImageDefinitionIconName),
    sizeof (CtkImageDefinitionGIcon),
    sizeof (CtkImageDefinitionSurface)
  };
  CtkImageDefinition *def;

  g_assert (type < G_N_ELEMENTS (sizes));

  def = g_malloc0 (sizes[type]);
  def->type = type;
  def->empty.ref_count = 1;

  return def;
}

CtkImageDefinition *
ctk_image_definition_new_pixbuf (CdkPixbuf *pixbuf,
                                 int        scale)
{
  CtkImageDefinition *def;

  if (pixbuf == NULL || scale <= 0)
    return NULL;

  def = ctk_image_definition_alloc (CTK_IMAGE_PIXBUF);
  def->pixbuf.pixbuf = g_object_ref (pixbuf);
  def->pixbuf.scale = scale;

  return def;
}

CtkImageDefinition *
ctk_image_definition_new_stock (const char *stock_id)
{
  CtkImageDefinition *def;

  if (stock_id == NULL || stock_id[0] == '\0')
    return NULL;

  def = ctk_image_definition_alloc (CTK_IMAGE_STOCK);
  def->stock.id = g_strdup (stock_id);

  return def;
}

CtkImageDefinition *
ctk_image_definition_new_icon_set (CtkIconSet *icon_set)
{
  CtkImageDefinition *def;

  if (icon_set == NULL)
    return NULL;

  def = ctk_image_definition_alloc (CTK_IMAGE_ICON_SET);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  def->icon_set.icon_set = ctk_icon_set_ref (icon_set);
G_GNUC_END_IGNORE_DEPRECATIONS;

  return def;
}

CtkImageDefinition *
ctk_image_definition_new_animation (CdkPixbufAnimation *animation,
                                    int                 scale)
{
  CtkImageDefinition *def;

  if (animation == NULL || scale <= 0)
    return NULL;

  def = ctk_image_definition_alloc (CTK_IMAGE_ANIMATION);
  def->animation.animation = g_object_ref (animation);
  def->animation.scale = scale;

  return def;
}

CtkImageDefinition *
ctk_image_definition_new_icon_name (const char *icon_name)
{
  CtkImageDefinition *def;

  if (icon_name == NULL || icon_name[0] == '\0')
    return NULL;

  def = ctk_image_definition_alloc (CTK_IMAGE_ICON_NAME);
  def->icon_name.icon_name = g_strdup (icon_name);

  return def;
}

CtkImageDefinition *
ctk_image_definition_new_gicon (GIcon *gicon)
{
  CtkImageDefinition *def;

  if (gicon == NULL)
    return NULL;

  def = ctk_image_definition_alloc (CTK_IMAGE_GICON);
  def->gicon.gicon = g_object_ref (gicon);

  return def;
}

CtkImageDefinition *
ctk_image_definition_new_surface (cairo_surface_t *surface)
{
  CtkImageDefinition *def;

  if (surface == NULL)
    return NULL;

  def = ctk_image_definition_alloc (CTK_IMAGE_SURFACE);
  def->surface.surface = cairo_surface_reference (surface);

  return def;
}

CtkImageDefinition *
ctk_image_definition_ref (CtkImageDefinition *def)
{
  def->empty.ref_count++;

  return def;
}

void
ctk_image_definition_unref (CtkImageDefinition *def)
{
  def->empty.ref_count--;

  if (def->empty.ref_count > 0)
    return;

  switch (def->type)
    {
    default:
    case CTK_IMAGE_EMPTY:
      g_assert_not_reached ();
      break;
    case CTK_IMAGE_PIXBUF:
      g_object_unref (def->pixbuf.pixbuf);
      break;
    case CTK_IMAGE_ANIMATION:
      g_object_unref (def->animation.animation);
      break;
    case CTK_IMAGE_SURFACE:
      cairo_surface_destroy (def->surface.surface);
      break;
    case CTK_IMAGE_STOCK:
      g_free (def->stock.id);
      break;
    case CTK_IMAGE_ICON_SET:
G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      ctk_icon_set_unref (def->icon_set.icon_set);
G_GNUC_END_IGNORE_DEPRECATIONS;
      break;
    case CTK_IMAGE_ICON_NAME:
      g_free (def->icon_name.icon_name);
      break;
    case CTK_IMAGE_GICON:
      g_object_unref (def->gicon.gicon);
      break;
    }

  g_free (def);
}

CtkImageType
ctk_image_definition_get_storage_type (const CtkImageDefinition *def)
{
  return def->type;
}

gint
ctk_image_definition_get_scale (const CtkImageDefinition *def)
{
  switch (def->type)
    {
    default:
      g_assert_not_reached ();
    case CTK_IMAGE_EMPTY:
    case CTK_IMAGE_SURFACE:
    case CTK_IMAGE_STOCK:
    case CTK_IMAGE_ICON_SET:
    case CTK_IMAGE_ICON_NAME:
    case CTK_IMAGE_GICON:
      return 1;
    case CTK_IMAGE_PIXBUF:
      return def->pixbuf.scale;
    case CTK_IMAGE_ANIMATION:
      return def->animation.scale;
    }
}

CdkPixbuf *
ctk_image_definition_get_pixbuf (const CtkImageDefinition *def)
{
  if (def->type != CTK_IMAGE_PIXBUF)
    return NULL;

  return def->pixbuf.pixbuf;
}

const gchar *
ctk_image_definition_get_stock (const CtkImageDefinition *def)
{
  if (def->type != CTK_IMAGE_STOCK)
    return NULL;

  return def->stock.id;
}

CtkIconSet *
ctk_image_definition_get_icon_set (const CtkImageDefinition *def)
{
  if (def->type != CTK_IMAGE_ICON_SET)
    return NULL;

  return def->icon_set.icon_set;
}

CdkPixbufAnimation *
ctk_image_definition_get_animation (const CtkImageDefinition *def)
{
  if (def->type != CTK_IMAGE_ANIMATION)
    return NULL;

  return def->animation.animation;
}

const gchar *
ctk_image_definition_get_icon_name (const CtkImageDefinition *def)
{
  if (def->type != CTK_IMAGE_ICON_NAME)
    return NULL;

  return def->icon_name.icon_name;
}

GIcon *
ctk_image_definition_get_gicon (const CtkImageDefinition *def)
{
  if (def->type != CTK_IMAGE_GICON)
    return NULL;

  return def->gicon.gicon;
}

cairo_surface_t *
ctk_image_definition_get_surface (const CtkImageDefinition *def)
{
  if (def->type != CTK_IMAGE_SURFACE)
    return NULL;

  return def->surface.surface;
}
