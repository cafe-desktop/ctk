/* CTK - The GIMP Toolkit
 * ctkwin32embed.h: Utilities for Win32 embedding
 * Copyright (C) 2005, Novell, Inc.
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

/* By Tor Lillqvist <tml@novell.com> 2005 */

#ifndef __CTK_WIN32_EMBED_H__
#define __CTK_WIN32_EMBED_H__

G_BEGIN_DECLS

#define CTK_WIN32_EMBED_PROTOCOL_VERSION 1

/*
 * When the plug and socket are in separate processes they use a
 * simple protocol, more or less based on XEMBED. The protocol uses
 * registered window messages. The name passed to
 * RegisterWindowMessage() is ctk-win32-embed:%d, with %d being the
 * numeric value of an CtkWin32EmbedMessageType enum. Each message
 * carries the message type enum value and two integers, the “wparam”
 * and “lparam”, like all window messages.
 *
 * So far all the window messages are posted to the other
 * process. Maybe some later enhancement will add also messages that
 * are sent, i.e. where the sending process waits for the receiving
 * process’s window procedure to handle the message.
 */

typedef enum {					/* send or post? */
  /* First those sent from the socket
   * to the plug
   */
  CTK_WIN32_EMBED_WINDOW_ACTIVATE,		/* post */
  CTK_WIN32_EMBED_WINDOW_DEACTIVATE,		/* post */
  CTK_WIN32_EMBED_FOCUS_IN,			/* post */
  CTK_WIN32_EMBED_FOCUS_OUT,			/* post */
  CTK_WIN32_EMBED_MODALITY_ON,			/* post */
  CTK_WIN32_EMBED_MODALITY_OFF,			/* post */

  /* Then the ones sent from the plug
   * to the socket.
   */
  CTK_WIN32_EMBED_PARENT_NOTIFY,		/* post */
  CTK_WIN32_EMBED_EVENT_PLUG_MAPPED,		/* post */
  CTK_WIN32_EMBED_PLUG_RESIZED,			/* post */
  CTK_WIN32_EMBED_REQUEST_FOCUS,		/* post */
  CTK_WIN32_EMBED_FOCUS_NEXT,			/* post */
  CTK_WIN32_EMBED_FOCUS_PREV,			/* post */
  CTK_WIN32_EMBED_GRAB_KEY,			/* post */
  CTK_WIN32_EMBED_UNGRAB_KEY,			/* post */
  CTK_WIN32_EMBED_LAST
} CtkWin32EmbedMessageType;

/* wParam values for CTK_WIN32_EMBED_FOCUS_IN: */
#define CTK_WIN32_EMBED_FOCUS_CURRENT 0
#define CTK_WIN32_EMBED_FOCUS_FIRST 1
#define CTK_WIN32_EMBED_FOCUS_LAST 2

/* Flags for lParam in CTK_WIN32_EMBED_FOCUS_IN, CTK_WIN32_EMBED_FOCUS_NEXT,
 * CTK_WIN32_EMBED_FOCUS_PREV
 */
#define CTK_WIN32_EMBED_FOCUS_WRAPAROUND         (1 << 0)

guint _ctk_win32_embed_message_type (CtkWin32EmbedMessageType type);
void _ctk_win32_embed_push_message (MSG *msg);
void _ctk_win32_embed_pop_message (void);
void _ctk_win32_embed_send (GdkWindow		    *recipient,
			    CtkWin32EmbedMessageType message,
			    WPARAM		     wparam,
			    LPARAM                   lparam);
void _ctk_win32_embed_send_focus_message (GdkWindow		  *recipient,
					  CtkWin32EmbedMessageType message,
					  WPARAM	           wparam);
void     _ctk_win32_embed_set_focus_wrapped  (void);
gboolean _ctk_win32_embed_get_focus_wrapped  (void);

G_END_DECLS

#endif /*  __CTK_WIN32_EMBED_H__ */
