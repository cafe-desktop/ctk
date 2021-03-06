/* CTK - The GIMP Toolkit
 * ctktexttypes.h Copyright (C) 2000 Red Hat, Inc.
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

#ifndef __CTK_TEXT_TYPES_H__
#define __CTK_TEXT_TYPES_H__

#include <ctk/ctk.h>
#include <ctk/ctktexttagprivate.h>

G_BEGIN_DECLS

typedef struct _CtkTextCounter CtkTextCounter;
typedef struct _CtkTextLineSegment CtkTextLineSegment;
typedef struct _CtkTextLineSegmentClass CtkTextLineSegmentClass;
typedef struct _CtkTextToggleBody CtkTextToggleBody;
typedef struct _CtkTextMarkBody CtkTextMarkBody;

/*
 * Declarations for variables shared among the text-related files:
 */

/* In ctktextbtree.c */
extern G_GNUC_INTERNAL const CtkTextLineSegmentClass ctk_text_char_type;
extern G_GNUC_INTERNAL const CtkTextLineSegmentClass ctk_text_toggle_on_type;
extern G_GNUC_INTERNAL const CtkTextLineSegmentClass ctk_text_toggle_off_type;

/* In ctktextmark.c */
extern G_GNUC_INTERNAL const CtkTextLineSegmentClass ctk_text_left_mark_type;
extern G_GNUC_INTERNAL const CtkTextLineSegmentClass ctk_text_right_mark_type;

/* In ctktextchild.c */
extern G_GNUC_INTERNAL const CtkTextLineSegmentClass ctk_text_pixbuf_type;
extern G_GNUC_INTERNAL const CtkTextLineSegmentClass ctk_text_child_type;

/*
 * UTF 8 Stubs
 */

#define CTK_TEXT_UNKNOWN_CHAR 0xFFFC
#define CTK_TEXT_UNKNOWN_CHAR_UTF8_LEN 3
CDK_AVAILABLE_IN_ALL
const gchar *ctk_text_unknown_char_utf8_ctk_tests_only (void);
extern const gchar _ctk_text_unknown_char_utf8[CTK_TEXT_UNKNOWN_CHAR_UTF8_LEN+1];

CDK_AVAILABLE_IN_ALL
gboolean ctk_text_byte_begins_utf8_char (const gchar *byte);

G_END_DECLS

#endif

