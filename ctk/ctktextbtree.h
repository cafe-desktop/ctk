/* CTK - The GIMP Toolkit
 * ctktextbtree.h Copyright (C) 2000 Red Hat, Inc.
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

#ifndef __CTK_TEXT_BTREE_H__
#define __CTK_TEXT_BTREE_H__

#if 0
#define DEBUG_VALIDATION_AND_SCROLLING
#endif

#ifdef DEBUG_VALIDATION_AND_SCROLLING
#define DV(x) (x)
#else
#define DV(x)
#endif

#include <ctk/ctktextbuffer.h>
#include <ctk/ctktexttag.h>
#include <ctk/ctktextmark.h>
#include <ctk/ctktextchild.h>
#include <ctk/ctktextsegment.h>
#include <ctk/ctktextiter.h>

G_BEGIN_DECLS

CtkTextBTree  *_ctk_text_btree_new        (CtkTextTagTable *table,
                                           CtkTextBuffer   *buffer);
void           _ctk_text_btree_ref        (CtkTextBTree    *tree);
void           _ctk_text_btree_unref      (CtkTextBTree    *tree);
CtkTextBuffer *_ctk_text_btree_get_buffer (CtkTextBTree    *tree);


guint _ctk_text_btree_get_chars_changed_stamp    (CtkTextBTree *tree);
guint _ctk_text_btree_get_segments_changed_stamp (CtkTextBTree *tree);
void  _ctk_text_btree_segments_changed           (CtkTextBTree *tree);

gboolean _ctk_text_btree_is_end (CtkTextBTree       *tree,
                                 CtkTextLine        *line,
                                 CtkTextLineSegment *seg,
                                 int                 byte_index,
                                 int                 char_offset);

/* Indexable segment mutation */

void _ctk_text_btree_delete        (CtkTextIter *start,
                                    CtkTextIter *end);
void _ctk_text_btree_insert        (CtkTextIter *iter,
                                    const gchar *text,
                                    gint         len);
void _ctk_text_btree_insert_pixbuf (CtkTextIter *iter,
                                    CdkPixbuf   *pixbuf);

void _ctk_text_btree_insert_child_anchor (CtkTextIter        *iter,
                                          CtkTextChildAnchor *anchor);

void _ctk_text_btree_unregister_child_anchor (CtkTextChildAnchor *anchor);

/* View stuff */
CtkTextLine *_ctk_text_btree_find_line_by_y    (CtkTextBTree      *tree,
                                                gpointer           view_id,
                                                gint               ypixel,
                                                gint              *line_top_y);
gint         _ctk_text_btree_find_line_top     (CtkTextBTree      *tree,
                                                CtkTextLine       *line,
                                                gpointer           view_id);
void         _ctk_text_btree_add_view          (CtkTextBTree      *tree,
                                                CtkTextLayout     *layout);
void         _ctk_text_btree_remove_view       (CtkTextBTree      *tree,
                                                gpointer           view_id);
void         _ctk_text_btree_invalidate_region (CtkTextBTree      *tree,
                                                const CtkTextIter *start,
                                                const CtkTextIter *end,
                                                gboolean           cursors_only);
void         _ctk_text_btree_get_view_size     (CtkTextBTree      *tree,
                                                gpointer           view_id,
                                                gint              *width,
                                                gint              *height);
gboolean     _ctk_text_btree_is_valid          (CtkTextBTree      *tree,
                                                gpointer           view_id);
gboolean     _ctk_text_btree_validate          (CtkTextBTree      *tree,
                                                gpointer           view_id,
                                                gint               max_pixels,
                                                gint              *y,
                                                gint              *old_height,
                                                gint              *new_height);
void         _ctk_text_btree_validate_line     (CtkTextBTree      *tree,
                                                CtkTextLine       *line,
                                                gpointer           view_id);

/* Tag */

void _ctk_text_btree_tag (const CtkTextIter *start,
                          const CtkTextIter *end,
                          CtkTextTag        *tag,
                          gboolean           apply);

/* "Getters" */

CtkTextLine * _ctk_text_btree_get_line          (CtkTextBTree      *tree,
                                                 gint               line_number,
                                                 gint              *real_line_number);
CtkTextLine * _ctk_text_btree_get_line_no_last  (CtkTextBTree      *tree,
                                                 gint               line_number,
                                                 gint              *real_line_number);
CtkTextLine * _ctk_text_btree_get_end_iter_line (CtkTextBTree      *tree);
CtkTextLine * _ctk_text_btree_get_line_at_char  (CtkTextBTree      *tree,
                                                 gint               char_index,
                                                 gint              *line_start_index,
                                                 gint              *real_char_index);
CtkTextTag**  _ctk_text_btree_get_tags          (const CtkTextIter *iter,
                                                 gint              *num_tags);
gchar        *_ctk_text_btree_get_text          (const CtkTextIter *start,
                                                 const CtkTextIter *end,
                                                 gboolean           include_hidden,
                                                 gboolean           include_nonchars);
gint          _ctk_text_btree_line_count        (CtkTextBTree      *tree);
gint          _ctk_text_btree_char_count        (CtkTextBTree      *tree);
gboolean      _ctk_text_btree_char_is_invisible (const CtkTextIter *iter);



/* Get iterators (these are implemented in ctktextiter.c) */
void     _ctk_text_btree_get_iter_at_char         (CtkTextBTree       *tree,
                                                   CtkTextIter        *iter,
                                                   gint                char_index);
void     _ctk_text_btree_get_iter_at_line_char    (CtkTextBTree       *tree,
                                                   CtkTextIter        *iter,
                                                   gint                line_number,
                                                   gint                char_index);
void     _ctk_text_btree_get_iter_at_line_byte    (CtkTextBTree       *tree,
                                                   CtkTextIter        *iter,
                                                   gint                line_number,
                                                   gint                byte_index);
gboolean _ctk_text_btree_get_iter_from_string     (CtkTextBTree       *tree,
                                                   CtkTextIter        *iter,
                                                   const gchar        *string);
gboolean _ctk_text_btree_get_iter_at_mark_name    (CtkTextBTree       *tree,
                                                   CtkTextIter        *iter,
                                                   const gchar        *mark_name);
void     _ctk_text_btree_get_iter_at_mark         (CtkTextBTree       *tree,
                                                   CtkTextIter        *iter,
                                                   CtkTextMark        *mark);
void     _ctk_text_btree_get_end_iter             (CtkTextBTree       *tree,
                                                   CtkTextIter        *iter);
void     _ctk_text_btree_get_iter_at_line         (CtkTextBTree       *tree,
                                                   CtkTextIter        *iter,
                                                   CtkTextLine        *line,
                                                   gint                byte_offset);
gboolean _ctk_text_btree_get_iter_at_first_toggle (CtkTextBTree       *tree,
                                                   CtkTextIter        *iter,
                                                   CtkTextTag         *tag);
gboolean _ctk_text_btree_get_iter_at_last_toggle  (CtkTextBTree       *tree,
                                                   CtkTextIter        *iter,
                                                   CtkTextTag         *tag);

void     _ctk_text_btree_get_iter_at_child_anchor  (CtkTextBTree       *tree,
                                                    CtkTextIter        *iter,
                                                    CtkTextChildAnchor *anchor);



/* Manipulate marks */
CtkTextMark        *_ctk_text_btree_set_mark                (CtkTextBTree       *tree,
                                                             CtkTextMark         *existing_mark,
                                                             const gchar        *name,
                                                             gboolean            left_gravity,
                                                             const CtkTextIter  *index,
                                                             gboolean           should_exist);
void                _ctk_text_btree_remove_mark_by_name     (CtkTextBTree       *tree,
                                                             const gchar        *name);
void                _ctk_text_btree_remove_mark             (CtkTextBTree       *tree,
                                                             CtkTextMark        *segment);
gboolean            _ctk_text_btree_get_selection_bounds    (CtkTextBTree       *tree,
                                                             CtkTextIter        *start,
                                                             CtkTextIter        *end);
void                _ctk_text_btree_place_cursor            (CtkTextBTree       *tree,
                                                             const CtkTextIter  *where);
void                _ctk_text_btree_select_range            (CtkTextBTree       *tree,
                                                             const CtkTextIter  *ins,
							     const CtkTextIter  *bound);
gboolean            _ctk_text_btree_mark_is_insert          (CtkTextBTree       *tree,
                                                             CtkTextMark        *segment);
gboolean            _ctk_text_btree_mark_is_selection_bound (CtkTextBTree       *tree,
                                                             CtkTextMark        *segment);
CtkTextMark        *_ctk_text_btree_get_insert		    (CtkTextBTree       *tree);
CtkTextMark        *_ctk_text_btree_get_selection_bound	    (CtkTextBTree       *tree);
CtkTextMark        *_ctk_text_btree_get_mark_by_name        (CtkTextBTree       *tree,
                                                             const gchar        *name);
CtkTextLine *       _ctk_text_btree_first_could_contain_tag (CtkTextBTree       *tree,
                                                             CtkTextTag         *tag);
CtkTextLine *       _ctk_text_btree_last_could_contain_tag  (CtkTextBTree       *tree,
                                                             CtkTextTag         *tag);

/* Lines */

/* Chunk of data associated with a line; views can use this to store
   info at the line. They should "subclass" the header struct here. */
struct _CtkTextLineData {
  gpointer view_id;
  CtkTextLineData *next;
  gint height;
  gint top_ink : 16;
  gint bottom_ink : 16;
  signed int width : 24;
  guint valid : 8;		/* Actually a boolean */
};

/*
 * The data structure below defines a single line of text (from newline
 * to newline, not necessarily what appears on one line of the screen).
 *
 * You can consider this line a "paragraph" also
 */

struct _CtkTextLine {
  CtkTextBTreeNode *parent;             /* Pointer to parent node containing
                                         * line. */
  CtkTextLine *next;            /* Next in linked list of lines with
                                 * same parent node in B-tree.  NULL
                                 * means end of list. */
  CtkTextLineSegment *segments; /* First in ordered list of segments
                                 * that make up the line. */
  CtkTextLineData *views;      /* data stored here by views */
  guchar dir_strong;                /* BiDi algo dir of line */
  guchar dir_propagated_back;       /* BiDi algo dir of next line */
  guchar dir_propagated_forward;    /* BiDi algo dir of prev line */
};


gint                _ctk_text_line_get_number                 (CtkTextLine         *line);
gboolean            _ctk_text_line_char_has_tag               (CtkTextLine         *line,
                                                               CtkTextBTree        *tree,
                                                               gint                 char_in_line,
                                                               CtkTextTag          *tag);
gboolean            _ctk_text_line_byte_has_tag               (CtkTextLine         *line,
                                                               CtkTextBTree        *tree,
                                                               gint                 byte_in_line,
                                                               CtkTextTag          *tag);
gboolean            _ctk_text_line_is_last                    (CtkTextLine         *line,
                                                               CtkTextBTree        *tree);
gboolean            _ctk_text_line_contains_end_iter          (CtkTextLine         *line,
                                                               CtkTextBTree        *tree);
CtkTextLine *       _ctk_text_line_next                       (CtkTextLine         *line);
CtkTextLine *       _ctk_text_line_next_excluding_last        (CtkTextLine         *line);
CtkTextLine *       _ctk_text_line_previous                   (CtkTextLine         *line);
void                _ctk_text_line_add_data                   (CtkTextLine         *line,
                                                               CtkTextLineData     *data);
gpointer            _ctk_text_line_remove_data                (CtkTextLine         *line,
                                                               gpointer             view_id);
gpointer            _ctk_text_line_get_data                   (CtkTextLine         *line,
                                                               gpointer             view_id);
void                _ctk_text_line_invalidate_wrap            (CtkTextLine         *line,
                                                               CtkTextLineData     *ld);
gint                _ctk_text_line_char_count                 (CtkTextLine         *line);
gint                _ctk_text_line_byte_count                 (CtkTextLine         *line);
gint                _ctk_text_line_char_index                 (CtkTextLine         *line);
CtkTextLineSegment *_ctk_text_line_byte_to_segment            (CtkTextLine         *line,
                                                               gint                 byte_offset,
                                                               gint                *seg_offset);
CtkTextLineSegment *_ctk_text_line_char_to_segment            (CtkTextLine         *line,
                                                               gint                 char_offset,
                                                               gint                *seg_offset);
gboolean            _ctk_text_line_byte_locate                (CtkTextLine         *line,
                                                               gint                 byte_offset,
                                                               CtkTextLineSegment **segment,
                                                               CtkTextLineSegment **any_segment,
                                                               gint                *seg_byte_offset,
                                                               gint                *line_byte_offset);
gboolean            _ctk_text_line_char_locate                (CtkTextLine         *line,
                                                               gint                 char_offset,
                                                               CtkTextLineSegment **segment,
                                                               CtkTextLineSegment **any_segment,
                                                               gint                *seg_char_offset,
                                                               gint                *line_char_offset);
void                _ctk_text_line_byte_to_char_offsets       (CtkTextLine         *line,
                                                               gint                 byte_offset,
                                                               gint                *line_char_offset,
                                                               gint                *seg_char_offset);
void                _ctk_text_line_char_to_byte_offsets       (CtkTextLine         *line,
                                                               gint                 char_offset,
                                                               gint                *line_byte_offset,
                                                               gint                *seg_byte_offset);
CtkTextLineSegment *_ctk_text_line_byte_to_any_segment        (CtkTextLine         *line,
                                                               gint                 byte_offset,
                                                               gint                *seg_offset);
CtkTextLineSegment *_ctk_text_line_char_to_any_segment        (CtkTextLine         *line,
                                                               gint                 char_offset,
                                                               gint                *seg_offset);
gint                _ctk_text_line_byte_to_char               (CtkTextLine         *line,
                                                               gint                 byte_offset);
gint                _ctk_text_line_char_to_byte               (CtkTextLine         *line,
                                                               gint                 char_offset);
CtkTextLine    *    _ctk_text_line_next_could_contain_tag     (CtkTextLine         *line,
                                                               CtkTextBTree        *tree,
                                                               CtkTextTag          *tag);
CtkTextLine    *    _ctk_text_line_previous_could_contain_tag (CtkTextLine         *line,
                                                               CtkTextBTree        *tree,
                                                               CtkTextTag          *tag);

CtkTextLineData    *_ctk_text_line_data_new                   (CtkTextLayout     *layout,
                                                               CtkTextLine       *line);

/* Debug */
void _ctk_text_btree_check (CtkTextBTree *tree);
void _ctk_text_btree_spew (CtkTextBTree *tree);
extern gboolean _ctk_text_view_debug_btree;

/* ignore, exported only for ctktextsegment.c */
void _ctk_toggle_segment_check_func (CtkTextLineSegment *segPtr,
                                     CtkTextLine        *line);
void _ctk_change_node_toggle_count  (CtkTextBTreeNode   *node,
                                     CtkTextTagInfo     *info,
                                     gint                delta);

/* for ctktextmark.c */
void _ctk_text_btree_release_mark_segment (CtkTextBTree       *tree,
                                           CtkTextLineSegment *segment);

/* for coordination with the tag table */
void _ctk_text_btree_notify_will_remove_tag (CtkTextBTree *tree,
                                             CtkTextTag   *tag);

G_END_DECLS

#endif


