/* cdkscreen-quartz.h
 *
 * Copyright (C) 2009,2010  Kristian Rietveld  <kris@ctk.org>
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

#ifndef __CDK_QUARTZ_SCREEN__
#define __CDK_QUARTZ_SCREEN__

#include <cdkscreenprivate.h>

G_BEGIN_DECLS

struct _CdkQuartzScreen
{
  CdkScreen parent_instance;

  CdkDisplay *display;

  /* Origin of "root window" in AppKit coordinates */
  gint orig_x;
  gint orig_y;

  gint width;
  gint height;
  gint mm_width;
  gint mm_height;

  guint screen_changed_id;

  guint emit_monitors_changed : 1;
};

struct _CdkQuartzScreenClass
{
  CdkScreenClass parent_class;
};

G_END_DECLS

#endif /* __CDK_QUARTZ_SCREEN__ */
