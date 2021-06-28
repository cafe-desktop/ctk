/* cdkvisual-quartz.c
 *
 * Copyright (C) 2005 Imendio AB
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

#include "cdkvisualprivate.h"
#include "cdkquartzvisual.h"
#include "cdkprivate-quartz.h"


struct _CdkQuartzVisual
{
  CdkVisual visual;
};

struct _CdkQuartzVisualClass
{
  CdkVisualClass visual_class;
};


static CdkVisual *system_visual;
static CdkVisual *rgba_visual;
static CdkVisual *gray_visual;

static CdkVisual *
create_standard_visual (CdkScreen *screen,
                        gint       depth)
{
  CdkVisual *visual = g_object_new (CDK_TYPE_QUARTZ_VISUAL, NULL);

  visual->screen = screen;

  visual->depth = depth;
  visual->byte_order = CDK_MSB_FIRST; /* FIXME: Should this be different on intel macs? */
  visual->colormap_size = 0;

  visual->type = CDK_VISUAL_TRUE_COLOR;

  visual->red_mask = 0xff0000;
  visual->green_mask = 0xff00;
  visual->blue_mask = 0xff;

  return visual;
}

static CdkVisual *
create_gray_visual (CdkScreen *screen)
{
  CdkVisual *visual = g_object_new (CDK_TYPE_QUARTZ_VISUAL, NULL);

  visual->screen = screen;

  visual->depth = 1;
  visual->byte_order = CDK_MSB_FIRST;
  visual->colormap_size = 0;

  visual->type = CDK_VISUAL_STATIC_GRAY;

  return visual;
}


G_DEFINE_TYPE (CdkQuartzVisual, cdk_quartz_visual, CDK_TYPE_VISUAL)

static void
cdk_quartz_visual_init (CdkQuartzVisual *quartz_visual)
{
}

static void
cdk_quartz_visual_class_init (CdkQuartzVisualClass *class)
{
}

/* We prefer the system visual for now ... */
gint
_cdk_quartz_screen_visual_get_best_depth (CdkScreen *screen)
{
  return system_visual->depth;
}

CdkVisualType
_cdk_quartz_screen_visual_get_best_type (CdkScreen *screen)
{
  return system_visual->type;
}

CdkVisual *
_cdk_quartz_screen_get_rgba_visual (CdkScreen *screen)
{
  return rgba_visual;
}

CdkVisual*
_cdk_quartz_screen_get_system_visual (CdkScreen *screen)
{
  return system_visual;
}

CdkVisual*
_cdk_quartz_screen_visual_get_best (CdkScreen *screen)
{
  return system_visual;
}

CdkVisual*
_cdk_quartz_screen_visual_get_best_with_depth (CdkScreen *screen,
                                               gint       depth)
{
  CdkVisual *visual = NULL;

  switch (depth)
    {
      case 32:
        visual = rgba_visual;
        break;

      case 24:
        visual = system_visual;
        break;

      case 1:
        visual = gray_visual;
        break;

      default:
        visual = NULL;
    }

  return visual;
}

CdkVisual*
_cdk_quartz_screen_visual_get_best_with_type (CdkScreen     *screen,
                                              CdkVisualType  visual_type)
{
  if (system_visual->type == visual_type)
    return system_visual;
  else if (gray_visual->type == visual_type)
    return gray_visual;

  return NULL;
}

CdkVisual*
_cdk_quartz_screen_visual_get_best_with_both (CdkScreen     *screen,
                                              gint           depth,
                                              CdkVisualType  visual_type)
{
  if (system_visual->depth == depth
      && system_visual->type == visual_type)
    return system_visual;
  else if (rgba_visual->depth == depth
           && rgba_visual->type == visual_type)
    return rgba_visual;
  else if (gray_visual->depth == depth
           && gray_visual->type == visual_type)
    return gray_visual;

  return NULL;
}

/* For these, we also prefer the system visual */
void
_cdk_quartz_screen_query_depths  (CdkScreen  *screen,
                                  gint      **depths,
                                  gint       *count)
{
  *count = 1;
  *depths = &system_visual->depth;
}

void
_cdk_quartz_screen_query_visual_types (CdkScreen      *screen,
                                       CdkVisualType **visual_types,
                                       gint           *count)
{
  *count = 1;
  *visual_types = &system_visual->type;
}

void
_cdk_quartz_screen_init_visuals (CdkScreen *screen)
{
  system_visual = create_standard_visual (screen, 24);
  rgba_visual = create_standard_visual (screen, 32);
  gray_visual = create_gray_visual (screen);
}

GList*
_cdk_quartz_screen_list_visuals (CdkScreen *screen)
{
  GList *visuals = NULL;

  visuals = g_list_append (visuals, system_visual);
  visuals = g_list_append (visuals, rgba_visual);
  visuals = g_list_append (visuals, gray_visual);

  return visuals;
}
