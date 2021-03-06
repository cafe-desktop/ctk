/* CTK - The GIMP Toolkit
 * ctkxembed.c: Utilities for the XEMBED protocol
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

#ifndef __CTK_XEMBED_H__
#define __CTK_XEMBED_H__

#include "xembed.h"
#include "x11/cdkx.h"

G_BEGIN_DECLS

/* Latest version we implement */
#define CTK_XEMBED_PROTOCOL_VERSION 1

void _ctk_xembed_send_message       (CdkWindow         *recipient,
				     XEmbedMessageType  message,
				     glong              detail,
				     glong              data1,
				     glong              data2);
void _ctk_xembed_send_focus_message (CdkWindow         *recipient,
				     XEmbedMessageType  message,
				     glong              detail);

void        _ctk_xembed_push_message       (XEvent    *xevent);
void        _ctk_xembed_pop_message        (void);
void        _ctk_xembed_set_focus_wrapped  (void);
gboolean    _ctk_xembed_get_focus_wrapped  (void);
const char *_ctk_xembed_message_name       (XEmbedMessageType message);

G_END_DECLS

#endif /*  __CTK_XEMBED_H__ */
