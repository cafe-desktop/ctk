/* CTK - The GIMP Toolkit
 * ctktextiterprivate.h Copyright (C) 2000 Red Hat, Inc.
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

#ifndef __CTK_TEXT_ITER_PRIVATE_H__
#define __CTK_TEXT_ITER_PRIVATE_H__

#include <ctk/ctktextiter.h>

G_BEGIN_DECLS

#include <ctk/ctktextiter.h>
#include <ctk/ctktextbtree.h>

CtkTextLineSegment *_ctk_text_iter_get_indexable_segment      (const CtkTextIter *iter);
CtkTextLineSegment *_ctk_text_iter_get_any_segment            (const CtkTextIter *iter);
CtkTextLine *       _ctk_text_iter_get_text_line              (const CtkTextIter *iter);
CtkTextBTree *      _ctk_text_iter_get_btree                  (const CtkTextIter *iter);
gboolean            _ctk_text_iter_forward_indexable_segment  (CtkTextIter       *iter);
gboolean            _ctk_text_iter_backward_indexable_segment (CtkTextIter       *iter);
gint                _ctk_text_iter_get_segment_byte           (const CtkTextIter *iter);
gint                _ctk_text_iter_get_segment_char           (const CtkTextIter *iter);


/* debug */
void _ctk_text_iter_check (const CtkTextIter *iter);

G_END_DECLS

#endif


