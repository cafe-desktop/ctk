/* GTK+ - accessibility implementations
 * Copyright 2001 Sun Microsystems Inc.
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

#include <ctk/ctk.h>
#include "ctkimagecellaccessible.h"

struct _CtkImageCellAccessiblePrivate
{
  gchar *image_description;
};

static void atk_image_interface_init (AtkImageIface *iface);

G_DEFINE_TYPE_WITH_CODE (CtkImageCellAccessible, ctk_image_cell_accessible, CTK_TYPE_RENDERER_CELL_ACCESSIBLE,
                         G_ADD_PRIVATE (CtkImageCellAccessible)
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_IMAGE, atk_image_interface_init))

static void
ctk_image_cell_accessible_finalize (GObject *object)
{
  CtkImageCellAccessible *image_cell = CTK_IMAGE_CELL_ACCESSIBLE (object);

  g_free (image_cell->priv->image_description);
  G_OBJECT_CLASS (ctk_image_cell_accessible_parent_class)->finalize (object);
}

static void
ctk_image_cell_accessible_class_init (CtkImageCellAccessibleClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = ctk_image_cell_accessible_finalize;
}

static void
ctk_image_cell_accessible_init (CtkImageCellAccessible *image_cell)
{
  image_cell->priv = ctk_image_cell_accessible_get_instance_private (image_cell);
}

static const gchar *
ctk_image_cell_accessible_get_image_description (AtkImage *image)
{
  CtkImageCellAccessible *image_cell = CTK_IMAGE_CELL_ACCESSIBLE (image);

  return image_cell->priv->image_description;
}

static gboolean
ctk_image_cell_accessible_set_image_description (AtkImage    *image,
                                                 const gchar *description)
{
  CtkImageCellAccessible *image_cell = CTK_IMAGE_CELL_ACCESSIBLE (image);

  g_free (image_cell->priv->image_description);
  image_cell->priv->image_description = g_strdup (description);

  if (image_cell->priv->image_description)
    return TRUE;
  else
    return FALSE;
}

static void
ctk_image_cell_accessible_get_image_position (AtkImage     *image,
                                              gint         *x,
                                              gint         *y,
                                              AtkCoordType  coord_type)
{
  atk_component_get_extents (ATK_COMPONENT (image), x, y, NULL, NULL,
                             coord_type);
}

static void
ctk_image_cell_accessible_get_image_size (AtkImage *image,
                                          gint     *width,
                                          gint     *height)
{
  CtkImageCellAccessible *cell = CTK_IMAGE_CELL_ACCESSIBLE (image);
  CtkCellRenderer *cell_renderer;
  GdkPixbuf *pixbuf = NULL;

  *width = 0;
  *height = 0;

  g_object_get (cell, "renderer", &cell_renderer, NULL);
  g_object_get (cell_renderer,
                "pixbuf", &pixbuf,
                NULL);
  g_object_unref (cell_renderer);

  if (pixbuf)
    {
      *width = gdk_pixbuf_get_width (pixbuf);
      *height = gdk_pixbuf_get_height (pixbuf);
      g_object_unref (pixbuf);
    }
}

static void
atk_image_interface_init (AtkImageIface  *iface)
{
  iface->get_image_description = ctk_image_cell_accessible_get_image_description;
  iface->set_image_description = ctk_image_cell_accessible_set_image_description;
  iface->get_image_position = ctk_image_cell_accessible_get_image_position;
  iface->get_image_size = ctk_image_cell_accessible_get_image_size;
}
