/* CTK - The GIMP Toolkit
 * ctktexttypes.c Copyright (C) 2000 Red Hat, Inc.
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
#include "ctktexttypes.h"

/* These are used to represent embedded non-character objects
 * if you return a string representation of a text buffer
 */
const gchar _ctk_text_unknown_char_utf8[] = { '\xEF', '\xBF', '\xBC', '\0' };

/* This is to be used only by libctk test programs */
const gchar *
ctk_text_unknown_char_utf8_ctk_tests_only (void)
{
  return _ctk_text_unknown_char_utf8;
}

gboolean
ctk_text_byte_begins_utf8_char (const gchar *byte)
{
  return ((*byte & 0xC0) != 0x80);
}
