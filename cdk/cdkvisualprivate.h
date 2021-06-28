/* CDK - The GIMP Drawing Kit
 * Copyright (C) 2010 Red Hat, Inc
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

#ifndef __CDK_VISUAL_PRIVATE_H__
#define __CDK_VISUAL_PRIVATE_H__

#include "cdkvisual.h"

G_BEGIN_DECLS

#define CDK_VISUAL_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CDK_TYPE_VISUAL, CdkVisualClass))
#define CDK_IS_VISUAL_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CDK_TYPE_VISUAL))
#define CDK_VISUAL_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CDK_TYPE_VISUAL, CdkVisualClass))

typedef struct _CdkVisualClass    CdkVisualClass;

struct _CdkVisual
{
  GObject parent_instance;

  /*< private >*/
  CdkVisualType type;
  gint depth;
  CdkByteOrder byte_order;
  gint colormap_size;
  gint bits_per_rgb;

  guint32 red_mask;
  guint32 green_mask;
  guint32 blue_mask;

  CdkScreen *screen;
};

struct _CdkVisualClass
{
  GObjectClass parent_class;
};

G_END_DECLS

#endif
