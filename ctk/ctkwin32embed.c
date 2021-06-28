/* CTK - The GIMP Toolkit
 * ctkwin32embed.c: Utilities for Win32 embedding
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

#include "config.h"

#include "win32/cdkwin32.h"

#include "ctkwin32embed.h"


static guint message_type[CTK_WIN32_EMBED_LAST];

static GSList *current_messages;

guint
_ctk_win32_embed_message_type (CtkWin32EmbedMessageType type)
{
  if (type < 0 || type >= CTK_WIN32_EMBED_LAST)
    return 0;

  if (message_type[type] == 0)
    {
      char name[100];
      sprintf (name, "ctk-win32-embed:%d", type);
      message_type[type] = RegisterWindowMessage (name);
    }

  return message_type[type];
}

void
_ctk_win32_embed_push_message (MSG *msg)
{
  MSG *message = g_new (MSG, 1);

  *message = *msg;

  current_messages = g_slist_prepend (current_messages, message);
}

void
_ctk_win32_embed_pop_message (void)
{
  MSG *message = current_messages->data;

  current_messages = g_slist_delete_link (current_messages, current_messages);

  g_free (message);
}

void
_ctk_win32_embed_send (CdkWindow               *recipient,
		       CtkWin32EmbedMessageType message,
		       WPARAM		        wparam,
		       LPARAM			lparam)
{
  PostMessage (CDK_WINDOW_HWND (recipient),
	       _ctk_win32_embed_message_type (message),
	       wparam, lparam);
}

void
_ctk_win32_embed_send_focus_message (CdkWindow               *recipient,
				     CtkWin32EmbedMessageType message,
				     WPARAM		      wparam)
{
  int lparam = 0;

  if (!recipient)
    return;
  
  g_return_if_fail (CDK_IS_WINDOW (recipient));
  g_return_if_fail (message == CTK_WIN32_EMBED_FOCUS_IN ||
		    message == CTK_WIN32_EMBED_FOCUS_NEXT ||
		    message == CTK_WIN32_EMBED_FOCUS_PREV);
		    
  if (current_messages)
    {
      MSG *msg = current_messages->data;
      if (msg->message == _ctk_win32_embed_message_type (CTK_WIN32_EMBED_FOCUS_IN) ||
	  msg->message == _ctk_win32_embed_message_type (CTK_WIN32_EMBED_FOCUS_NEXT) ||
	  msg->message == _ctk_win32_embed_message_type (CTK_WIN32_EMBED_FOCUS_PREV))
	lparam = (msg->lParam & CTK_WIN32_EMBED_FOCUS_WRAPAROUND);
    }

  _ctk_win32_embed_send (recipient, message, wparam, lparam);
}

void
_ctk_win32_embed_set_focus_wrapped (void)
{
  MSG *msg;
  
  g_return_if_fail (current_messages != NULL);

  msg = current_messages->data;

  g_return_if_fail (msg->message == _ctk_win32_embed_message_type (CTK_WIN32_EMBED_FOCUS_PREV) ||
		    msg->message == _ctk_win32_embed_message_type (CTK_WIN32_EMBED_FOCUS_NEXT));
  
  msg->lParam |= CTK_WIN32_EMBED_FOCUS_WRAPAROUND;
}

gboolean
_ctk_win32_embed_get_focus_wrapped (void)
{
  MSG *msg;
  
  g_return_val_if_fail (current_messages != NULL, FALSE);

  msg = current_messages->data;

  return (msg->lParam & CTK_WIN32_EMBED_FOCUS_WRAPAROUND) != 0;
}
