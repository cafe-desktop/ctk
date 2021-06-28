/* CDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#include "config.h"

#include "cdkvisualprivate.h"

#include "cdkprivate-broadway.h"
#include "cdkscreen-broadway.h"
#include "cdkinternals.h"

struct _CdkBroadwayVisual
{
  CdkVisual visual;
};

struct _CdkBroadwayVisualClass
{
  GObjectClass parent_class;
};

G_DEFINE_TYPE (CdkBroadwayVisual, cdk_broadway_visual, CDK_TYPE_VISUAL)

static void
cdk_broadway_visual_finalize (GObject *object)
{
  G_OBJECT_CLASS (cdk_broadway_visual_parent_class)->finalize (object);
}

static void
cdk_broadway_visual_class_init (CdkBroadwayVisualClass *visual_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (visual_class);

  object_class->finalize = cdk_broadway_visual_finalize;
}

static void
cdk_broadway_visual_init (CdkBroadwayVisual *visual)
{
}

void
_cdk_broadway_screen_init_visuals (CdkScreen *screen)
{
  CdkBroadwayScreen *broadway_screen;
  CdkVisual **visuals;
  int nvisuals;

  g_return_if_fail (CDK_IS_SCREEN (screen));
  broadway_screen = CDK_BROADWAY_SCREEN (screen);

  nvisuals = 2;
  visuals = g_new (CdkVisual *, nvisuals);

  visuals[0] = g_object_new (CDK_TYPE_BROADWAY_VISUAL, NULL);
  visuals[0]->screen = screen;
  visuals[0]->type = CDK_VISUAL_TRUE_COLOR;
  visuals[0]->depth = 32;
  visuals[0]->byte_order = (G_BYTE_ORDER == G_LITTLE_ENDIAN) ? CDK_LSB_FIRST : CDK_MSB_FIRST;
  visuals[0]->red_mask = 0xff0000;
  visuals[0]->green_mask = 0xff00;
  visuals[0]->blue_mask = 0xff;
  visuals[0]->colormap_size = 256;
  visuals[0]->bits_per_rgb = 8;

  visuals[1] = g_object_new (CDK_TYPE_BROADWAY_VISUAL, NULL);
  visuals[1]->screen = screen;
  visuals[1]->type = CDK_VISUAL_TRUE_COLOR;
  visuals[1]->depth = 24;
  visuals[1]->byte_order = (G_BYTE_ORDER == G_LITTLE_ENDIAN) ? CDK_LSB_FIRST : CDK_MSB_FIRST;
  visuals[1]->red_mask = 0xff0000;
  visuals[1]->green_mask = 0xff00;
  visuals[1]->blue_mask = 0xff;
  visuals[1]->colormap_size = 256;
  visuals[1]->bits_per_rgb = 8;

  broadway_screen->system_visual = visuals[1];
  broadway_screen->rgba_visual = visuals[0];

  broadway_screen->navailable_depths = 2;
  broadway_screen->available_depths[0] = 32;
  broadway_screen->available_depths[1] = 24;

  broadway_screen->navailable_types = 1;
  broadway_screen->available_types[0] = CDK_VISUAL_TRUE_COLOR;

  broadway_screen->visuals = visuals;
  broadway_screen->nvisuals = nvisuals;
}

gint
_cdk_broadway_screen_visual_get_best_depth (CdkScreen * screen)
{
  return CDK_BROADWAY_SCREEN (screen)->available_depths[0];
}

CdkVisualType
_cdk_broadway_screen_visual_get_best_type (CdkScreen * screen)
{
  return CDK_BROADWAY_SCREEN (screen)->available_types[0];
}

CdkVisual *
_cdk_broadway_screen_get_system_visual (CdkScreen * screen)
{
  g_return_val_if_fail (CDK_IS_SCREEN (screen), NULL);

  return ((CdkVisual *) CDK_BROADWAY_SCREEN (screen)->system_visual);
}

CdkVisual*
_cdk_broadway_screen_visual_get_best (CdkScreen * screen)
{
  CdkBroadwayScreen *broadway_screen = CDK_BROADWAY_SCREEN (screen);

  return (CdkVisual *)broadway_screen->visuals[0];
}

CdkVisual*
_cdk_broadway_screen_visual_get_best_with_depth (CdkScreen * screen,
						 gint depth)
{
  CdkBroadwayScreen *broadway_screen = CDK_BROADWAY_SCREEN (screen);
  CdkVisual *return_val;
  int i;

  return_val = NULL;
  for (i = 0; i < broadway_screen->nvisuals; i++)
    if (depth == broadway_screen->visuals[i]->depth)
      {
	return_val = (CdkVisual *) broadway_screen->visuals[i];
	break;
      }

  return return_val;
}

CdkVisual*
_cdk_broadway_screen_visual_get_best_with_type (CdkScreen * screen,
						CdkVisualType visual_type)
{
  CdkBroadwayScreen *broadway_screen = CDK_BROADWAY_SCREEN (screen);
  CdkVisual *return_val;
  int i;

  return_val = NULL;
  for (i = 0; i < broadway_screen->nvisuals; i++)
    if (visual_type == broadway_screen->visuals[i]->type)
      {
	return_val = (CdkVisual *) broadway_screen->visuals[i];
	break;
      }

  return return_val;
}

CdkVisual*
_cdk_broadway_screen_visual_get_best_with_both (CdkScreen * screen,
						gint          depth,
						CdkVisualType visual_type)
{
  CdkBroadwayScreen *broadway_screen = CDK_BROADWAY_SCREEN (screen);
  CdkVisual *return_val;
  int i;

  return_val = NULL;
  for (i = 0; i < broadway_screen->nvisuals; i++)
    if ((depth == broadway_screen->visuals[i]->depth) &&
	(visual_type == broadway_screen->visuals[i]->type))
      {
	return_val = (CdkVisual *) broadway_screen->visuals[i];
	break;
      }

  return return_val;
}

void
_cdk_broadway_screen_query_depths  (CdkScreen * screen,
				    gint **depths,
				    gint  *count)
{
  CdkBroadwayScreen *broadway_screen = CDK_BROADWAY_SCREEN (screen);

  *count = broadway_screen->navailable_depths;
  *depths = broadway_screen->available_depths;
}

void
_cdk_broadway_screen_query_visual_types (CdkScreen * screen,
					 CdkVisualType **visual_types,
					 gint           *count)
{
  CdkBroadwayScreen *broadway_screen = CDK_BROADWAY_SCREEN (screen);

  *count = broadway_screen->navailable_types;
  *visual_types = broadway_screen->available_types;
}

GList *
_cdk_broadway_screen_list_visuals (CdkScreen *screen)
{
  GList *list;
  CdkBroadwayScreen *broadway_screen;
  guint i;

  g_return_val_if_fail (CDK_IS_SCREEN (screen), NULL);
  broadway_screen = CDK_BROADWAY_SCREEN (screen);

  list = NULL;

  for (i = 0; i < broadway_screen->nvisuals; ++i)
    list = g_list_append (list, broadway_screen->visuals[i]);

  return list;
}
