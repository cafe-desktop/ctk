/* CDK - The GIMP Drawing Kit
 * Copyright (C) 2010 Red Hat, Inc.
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

#ifndef __CDK_WIN32_DND_PRIVATE_H__
#define __CDK_WIN32_DND_PRIVATE_H__

#if !defined (__CDKWIN32_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdkwin32.h> can be included directly."
#endif

#include <cdk/cdk.h>

G_BEGIN_DECLS

struct _CdkWin32DragContext
{
  CdkDragContext context;
  CdkWindow *ipc_window;
  CdkWindow *drag_window;
  CdkCursor *cursor;
  CdkSeat *grab_seat;
  CdkDragAction actions;
  CdkDragAction current_action;

  guint drag_status : 4;             /* Current status of drag */
  guint drop_failed : 1;             /* Whether the drop was unsuccessful */
  guint has_image_format : 1;
  guint has_text_uri_list : 1;
  guint has_shell_id_list : 1;
  guint has_unicodetext : 1;
  guint has_cf_png : 1;
  guint has_cf_dib : 1;
  guint has_gif : 1;
  guint has_jfif : 1;

  guint scale;              /* Temporarily caches the HiDPI scale */
  gint hot_x;             /* Hotspot offset from the top-left of the drag-window, scaled (can be added to CDK space coordinates) */
  gint hot_y;
  gint last_x;            /* Coordinates from last event, in CDK space */
  gint last_y;
  gint start_x;           /* Coordinates of the drag start, in CDK space */
  gint start_y;
  DWORD last_key_state;     /* Key state from last event */

  /* Just like context->targets, but an array, and with format IDs
   * stored inside.
   */
  GArray *droptarget_format_target_map;
};

struct _CdkWin32DragContextClass
{
  CdkDragContextClass parent_class;
};

G_END_DECLS

#endif /* __CDK_WIN32_DND_PRIVATE_H__ */
