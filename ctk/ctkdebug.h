/* CTK - The GIMP Toolkit
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

#ifndef __CTK_DEBUG_H__
#define __CTK_DEBUG_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <glib.h>
#include <cdk/cdk.h>

G_BEGIN_DECLS

typedef enum {
  CTK_DEBUG_MISC            = 1 << 0,
  CTK_DEBUG_PLUGSOCKET      = 1 << 1,
  CTK_DEBUG_TEXT            = 1 << 2,
  CTK_DEBUG_TREE            = 1 << 3,
  CTK_DEBUG_UPDATES         = 1 << 4,
  CTK_DEBUG_KEYBINDINGS     = 1 << 5,
  CTK_DEBUG_MULTIHEAD       = 1 << 6,
  CTK_DEBUG_MODULES         = 1 << 7,
  CTK_DEBUG_GEOMETRY        = 1 << 8,
  CTK_DEBUG_ICONTHEME       = 1 << 9,
  CTK_DEBUG_PRINTING        = 1 << 10,
  CTK_DEBUG_BUILDER         = 1 << 11,
  CTK_DEBUG_SIZE_REQUEST    = 1 << 12,
  CTK_DEBUG_NO_CSS_CACHE    = 1 << 13,
  CTK_DEBUG_BASELINES       = 1 << 14,
  CTK_DEBUG_PIXEL_CACHE     = 1 << 15,
  CTK_DEBUG_NO_PIXEL_CACHE  = 1 << 16,
  CTK_DEBUG_INTERACTIVE     = 1 << 17,
  CTK_DEBUG_TOUCHSCREEN     = 1 << 18,
  CTK_DEBUG_ACTIONS         = 1 << 19,
  CTK_DEBUG_RESIZE          = 1 << 20,
  CTK_DEBUG_LAYOUT          = 1 << 21
} CtkDebugFlag;

#ifdef G_ENABLE_DEBUG

#define CTK_DEBUG_CHECK(type) G_UNLIKELY (ctk_get_debug_flags () & CTK_DEBUG_##type)

#define CTK_NOTE(type,action)                G_STMT_START {     \
    if (CTK_DEBUG_CHECK (type))		                        \
       { action; };                          } G_STMT_END

#else /* !G_ENABLE_DEBUG */

#define CTK_DEBUG_CHECK(type) 0
#define CTK_NOTE(type, action)

#endif /* G_ENABLE_DEBUG */

GDK_AVAILABLE_IN_ALL
guint ctk_get_debug_flags (void);
GDK_AVAILABLE_IN_ALL
void  ctk_set_debug_flags  (guint flags);

G_END_DECLS

#endif /* __CTK_DEBUG_H__ */
