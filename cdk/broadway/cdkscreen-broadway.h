/*
 * cdkscreen-broadway.h
 * 
 * Copyright 2001 Sun Microsystems Inc. 
 *
 * Erwann Chenede <erwann.chenede@sun.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CDK_BROADWAY_SCREEN_H__
#define __CDK_BROADWAY_SCREEN_H__

#include <cdk/cdkscreenprivate.h>
#include <cdk/cdkvisual.h>
#include "cdkprivate-broadway.h"

G_BEGIN_DECLS

typedef struct _CdkBroadwayScreen CdkBroadwayScreen;
typedef struct _CdkBroadwayScreenClass CdkBroadwayScreenClass;

#define CDK_TYPE_BROADWAY_SCREEN              (cdk_broadway_screen_get_type ())
#define CDK_BROADWAY_SCREEN(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), CDK_TYPE_BROADWAY_SCREEN, CdkBroadwayScreen))
#define CDK_BROADWAY_SCREEN_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CDK_TYPE_BROADWAY_SCREEN, CdkBroadwayScreenClass))
#define CDK_IS_BROADWAY_SCREEN(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), CDK_TYPE_BROADWAY_SCREEN))
#define CDK_IS_BROADWAY_SCREEN_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CDK_TYPE_BROADWAY_SCREEN))
#define CDK_BROADWAY_SCREEN_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CDK_TYPE_BROADWAY_SCREEN, CdkBroadwayScreenClass))

typedef struct _CdkBroadwayMonitor CdkBroadwayMonitor;

struct _CdkBroadwayScreen
{
  CdkScreen parent_instance;

  CdkDisplay *display;
  CdkWindow *root_window;

  int width;
  int height;

  /* Visual Part */
  CdkVisual **visuals;
  gint nvisuals;
  CdkVisual *system_visual;
  CdkVisual *rgba_visual;
  gint available_depths[7];
  gint navailable_depths;
  CdkVisualType available_types[6];
  gint navailable_types;
};

struct _CdkBroadwayScreenClass
{
  CdkScreenClass parent_class;

  void (* window_manager_changed) (CdkBroadwayScreen *screen);
};

GType       cdk_broadway_screen_get_type (void);
CdkScreen * _cdk_broadway_screen_new      (CdkDisplay *display,
					   gint	  screen_number);
void _cdk_broadway_screen_setup           (CdkScreen *screen);

G_END_DECLS

#endif /* __CDK_BROADWAY_SCREEN_H__ */
