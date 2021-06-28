/* CTK - The GIMP Toolkit
 * cdkasync.h: Utility functions using the Xlib asynchronous interfaces
 * Copyright (C) 2003, Red Hat, Inc.
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

#ifndef __GDK_ASYNC_H__
#define __GDK_ASYNC_H__

#include "cdkdisplay.h"
#include <X11/Xlib.h>

G_BEGIN_DECLS

typedef struct _CdkChildInfoX11 CdkChildInfoX11;

typedef void (*CdkSendXEventCallback) (Window   window,
				       gboolean success,
				       gpointer data);
typedef void (*CdkRoundTripCallback)  (CdkDisplay *display,
				       gpointer data,
				       gulong serial);

struct _CdkChildInfoX11
{
  Window window;
  gint x;
  gint y;
  gint width;
  gint height;
  guint is_mapped : 1;
  guint has_wm_state : 1;
  guint window_class : 2;
};

void _cdk_x11_send_client_message_async (CdkDisplay            *display,
					 Window                 window,
					 gboolean               propagate,
					 glong                  event_mask,
					 XClientMessageEvent   *event_send,
					 CdkSendXEventCallback  callback,
					 gpointer               data);

gboolean _cdk_x11_get_window_child_info (CdkDisplay       *display,
					 Window            window,
					 gboolean          get_wm_state,
					 gboolean         *win_has_wm_state,
					 CdkChildInfoX11 **children,
					 guint            *nchildren);

void _cdk_x11_roundtrip_async           (CdkDisplay           *display, 
					 CdkRoundTripCallback callback,
					 gpointer              data);

G_END_DECLS

#endif /* __GDK_ASYNC_H__ */
