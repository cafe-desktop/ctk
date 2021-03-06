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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

#ifndef __CTK_TEXT_MARK_PRIVATE_H__
#define __CTK_TEXT_MARK_PRIVATE_H__

#include <ctk/ctktexttypes.h>
#include <ctk/ctktextlayout.h>

G_BEGIN_DECLS

#define CTK_IS_TEXT_MARK_SEGMENT(mark) (((CtkTextLineSegment*)mark)->type == &ctk_text_left_mark_type || \
                                ((CtkTextLineSegment*)mark)->type == &ctk_text_right_mark_type)

/*
 * The data structure below defines line segments that represent
 * marks.  There is one of these for each mark in the text.
 */

struct _CtkTextMarkBody {
  CtkTextMark *obj;
  gchar *name;
  CtkTextBTree *tree;
  CtkTextLine *line;
  guint visible : 1;
  guint not_deleteable : 1;
};

void _ctk_mark_segment_set_tree (CtkTextLineSegment *mark,
				 CtkTextBTree       *tree);

G_END_DECLS

#endif



