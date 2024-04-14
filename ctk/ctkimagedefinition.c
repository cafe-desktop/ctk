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

#include "ctkiconfactory.h"

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

  GdkPixbuf *pixbuf;
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

  GdkPixbufAnimation *animation;
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
  CtkImageDefinitionEmpty *empty_def;

  g_assert (type < G_N_ELEMENTS (sizes));

  def = g_malloc0 (sizes[type]);
  empty_def = (CtkImageDefinitionEmpty *) def;
  empty_def->type = type;
  empty_def->ref_count = 1;

  return def;
}

CtkImageDefinition *
ctk_image_definition_new_pixbuf (GdkPixbuf *pixbuf,
                                 int        scale)
{
  CtkImageDefinition *def;
  CtkImageDefinitionPixbuf *pixbuf_def;

  if (pixbuf == NULL || scale <= 0)
    return NULL;

  def = ctk_image_definition_alloc (CTK_IMAGE_PIXBUF);
  pixbuf_def = (CtkImageDefinitionPixbuf *) def;
  pixbuf_def->pixbuf = g_object_ref (pixbuf);
  pixbuf_def->scale = scale;

  return def;
}

CtkImageDefinition *
ctk_image_definition_new_stock (const char *stock_id)
{
  CtkImageDefinition *def;
  CtkImageDefinitionStock *stock_def;

  if (stock_id == NULL || stock_id[0] == '\0')
    return NULL;

  def = ctk_image_definition_alloc (CTK_IMAGE_STOCK);
  stock_def = (CtkImageDefinitionStock *) def;
  stock_def->id = g_strdup (stock_id);

  return def;
}

CtkImageDefinition *
ctk_image_definition_new_icon_set (CtkIconSet *icon_set)
{
  CtkImageDefinition *def;
  CtkImageDefinitionIconSet *icon_set_def;

  if (icon_set == NULL)
    return NULL;

  def = ctk_image_definition_alloc (CTK_IMAGE_ICON_SET);
  icon_set_def = (CtkImageDefinitionIconSet *) def;
  icon_set_def->icon_set = ctk_icon_set_ref (icon_set);

  return def;
}

CtkImageDefinition *
ctk_image_definition_new_animation (GdkPixbufAnimation *animation,
                                    int                 scale)
{
  CtkImageDefinition *def;
  CtkImageDefinitionAnimation *animation_def;

  if (animation == NULL || scale <= 0)
    return NULL;

  def = ctk_image_definition_alloc (CTK_IMAGE_ANIMATION);
  animation_def = (CtkImageDefinitionAnimation *) def;
  animation_def->animation = g_object_ref (animation);
  animation_def->scale = scale;

  return def;
}

CtkImageDefinition *
ctk_image_definition_new_icon_name (const char *icon_name)
{
  CtkImageDefinition *def;
  CtkImageDefinitionIconName *icon_name_def;

  if (icon_name == NULL || icon_name[0] == '\0')
    return NULL;

  def = ctk_image_definition_alloc (CTK_IMAGE_ICON_NAME);
  icon_name_def = (CtkImageDefinitionIconName *) def;
  icon_name_def->icon_name = g_strdup (icon_name);

  return def;
}

CtkImageDefinition *
ctk_image_definition_new_gicon (GIcon *gicon)
{
  CtkImageDefinition *def;
  CtkImageDefinitionGIcon *gicon_def;

  if (gicon == NULL)
    return NULL;

  def = ctk_image_definition_alloc (CTK_IMAGE_GICON);
  gicon_def = (CtkImageDefinitionGIcon *) def;
  gicon_def->gicon = g_object_ref (gicon);

  return def;
}

CtkImageDefinition *
ctk_image_definition_new_surface (cairo_surface_t *surface)
{
  CtkImageDefinition *def;
  CtkImageDefinitionSurface *surface_def;

  if (surface == NULL)
    return NULL;

  def = ctk_image_definition_alloc (CTK_IMAGE_SURFACE);
  surface_def = (CtkImageDefinitionSurface *) def;
  surface_def->surface = cairo_surface_reference (surface);

  return def;
}

CtkImageDefinition *
ctk_image_definition_ref (CtkImageDefinition *def)
{
  CtkImageDefinitionEmpty *empty_def;

  empty_def = (CtkImageDefinitionEmpty *) def;
  empty_def->ref_count++;

  return def;
}

void
ctk_image_definition_unref (CtkImageDefinition *def)
{
  CtkImageDefinitionEmpty *empty_def;
  CtkImageDefinitionPixbuf *pixbuf_def;
  CtkImageDefinitionAnimation *animation_def;
  CtkImageDefinitionSurface *surface_def;
  CtkImageDefinitionStock *stock_def;
  CtkImageDefinitionIconSet *icon_set_def;
  CtkImageDefinitionIconName *icon_name_def;
  CtkImageDefinitionGIcon *gicon_def;

  empty_def = (CtkImageDefinitionEmpty *) def;
  empty_def->ref_count--;

  if (empty_def->ref_count > 0)
    return;

  switch (def->type)
    {
    default:
    case CTK_IMAGE_EMPTY:
      g_assert_not_reached ();
      break;
    case CTK_IMAGE_PIXBUF:
      pixbuf_def = (CtkImageDefinitionPixbuf *) def;
      g_object_unref (pixbuf_def->pixbuf);
      break;
    case CTK_IMAGE_ANIMATION:
      animation_def = (CtkImageDefinitionAnimation *) def;
      g_object_unref (animation_def->animation);
      break;
    case CTK_IMAGE_SURFACE:
      surface_def = (CtkImageDefinitionSurface *) def;
      cairo_surface_destroy (surface_def->surface);
      break;
    case CTK_IMAGE_STOCK:
      stock_def = (CtkImageDefinitionStock *) def;
      g_free (stock_def->id);
      break;
    case CTK_IMAGE_ICON_SET:
      icon_set_def = (CtkImageDefinitionIconSet *) def;
      ctk_icon_set_unref (icon_set_def->icon_set);
      break;
    case CTK_IMAGE_ICON_NAME:
      icon_name_def = (CtkImageDefinitionIconName *) def;
      g_free (icon_name_def->icon_name);
      break;
    case CTK_IMAGE_GICON:
      gicon_def = (CtkImageDefinitionGIcon *) def;
      g_object_unref (gicon_def->gicon);
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

GdkPixbuf *
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

GdkPixbufAnimation *
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
