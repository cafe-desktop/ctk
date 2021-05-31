/* CTK - The GIMP Toolkit
 * ctktextsegment.h Copyright (C) 2000 Red Hat, Inc.
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

#ifndef __CTK_TEXT_SEGMENT_H__
#define __CTK_TEXT_SEGMENT_H__

#include <ctk/ctktexttag.h>
#include <ctk/ctktextiter.h>
#include <ctk/ctktextmarkprivate.h>
#include <ctk/ctktextchild.h>
#include <ctk/ctktextchildprivate.h>

G_BEGIN_DECLS

/*
 * Segments: each line is divided into one or more segments, where each
 * segment is one of several things, such as a group of characters, a
 * tag toggle, a mark, or an embedded widget.  Each segment starts with
 * a standard header followed by a body that varies from type to type.
 */

/* This header has the segment type, and two specific segments
   (character and toggle segments) */

/* Information a BTree stores about a tag. */
typedef struct _CtkTextTagInfo CtkTextTagInfo;
struct _CtkTextTagInfo {
  CtkTextTag *tag;
  CtkTextBTreeNode *tag_root; /* highest-level node containing the tag */
  gint toggle_count;      /* total toggles of this tag below tag_root */
};

/* Body of a segment that toggles a tag on or off */
struct _CtkTextToggleBody {
  CtkTextTagInfo *info;             /* Tag that starts or ends here. */
  gboolean inNodeCounts;             /* TRUE means this toggle has been
                                      * accounted for in node toggle
                                      * counts; FALSE means it hasn't, yet. */
};


/* Class struct for segments */

/* Split seg at index, returning list of two new segments, and freeing seg */
typedef CtkTextLineSegment* (*CtkTextSegSplitFunc)      (CtkTextLineSegment *seg,
                                                         gint                index);

/* Delete seg which is contained in line; if tree_gone, the tree is being
 * freed in its entirety, which may matter for some reason (?)
 * Return TRUE if the segment is not deleteable, e.g. a mark.
 */
typedef gboolean            (*CtkTextSegDeleteFunc)     (CtkTextLineSegment *seg,
                                                         CtkTextLine        *line,
                                                         gboolean            tree_gone);

/* Called after segment structure of line changes, so segments can
 * cleanup (e.g. merge with adjacent segments). Returns a segment list
 * to replace the original segment list with. The line argument is
 * the current line.
 */
typedef CtkTextLineSegment* (*CtkTextSegCleanupFunc)    (CtkTextLineSegment *seg,
                                                         CtkTextLine        *line);

/* Called when a segment moves from one line to another. CleanupFunc is also
 * called in that case, so many segments just use CleanupFunc, I'm not sure
 * what’s up with that (this function may not be needed...)
 */
typedef void                (*CtkTextSegLineChangeFunc) (CtkTextLineSegment *seg,
                                                         CtkTextLine        *line);

/* Called to do debug checks on the segment. */
typedef void                (*CtkTextSegCheckFunc)      (CtkTextLineSegment *seg,
                                                         CtkTextLine        *line);

struct _CtkTextLineSegmentClass {
  char *name;                           /* Name of this kind of segment. */
  gboolean leftGravity;                 /* If a segment has zero size (e.g. a
                                         * mark or tag toggle), does it
                                         * attach to character to its left
                                         * or right?  1 means left, 0 means
                                         * right. */
  CtkTextSegSplitFunc splitFunc;        /* Procedure to split large segment
                                         * into two smaller ones. */
  CtkTextSegDeleteFunc deleteFunc;      /* Procedure to call to delete
                                         * segment. */
  CtkTextSegCleanupFunc cleanupFunc;   /* After any change to a line, this
                                        * procedure is invoked for all
                                        * segments left in the line to
                                        * perform any cleanup they wish
                                        * (e.g. joining neighboring
                                        * segments). */
  CtkTextSegLineChangeFunc lineChangeFunc;
  /* Invoked when a segment is about
   * to be moved from its current line
   * to an earlier line because of
   * a deletion.  The line is that
   * for the segment's old line.
   * CleanupFunc will be invoked after
   * the deletion is finished. */

  CtkTextSegCheckFunc checkFunc;       /* Called during consistency checks
                                        * to check internal consistency of
                                        * segment. */
};

/*
 * The data structure below defines line segments.
 */

struct _CtkTextLineSegment {
  const CtkTextLineSegmentClass *type;  /* Pointer to record describing
                                         * segment's type. */
  CtkTextLineSegment *next;             /* Next in list of segments for this
                                         * line, or NULL for end of list. */

  int char_count;                       /* # of chars of index space occupied */

  int byte_count;                       /* Size of this segment (# of bytes
                                         * of index space it occupies). */
  union {
    char chars[4];                      /* Characters that make up character
                                         * info.  Actual length varies to
                                         * hold as many characters as needed.*/
    CtkTextToggleBody toggle;              /* Information about tag toggle. */
    CtkTextMarkBody mark;              /* Information about mark. */
    CtkTextPixbuf pixbuf;              /* Child pixbuf */
    CtkTextChildBody child;            /* Child widget */
  } body;
};


GDK_AVAILABLE_IN_ALL
CtkTextLineSegment  *ctk_text_line_segment_split (const CtkTextIter *iter);

CtkTextLineSegment *_ctk_char_segment_new                  (const gchar    *text,
                                                            guint           len);
CtkTextLineSegment *_ctk_char_segment_new_from_two_strings (const gchar    *text1,
                                                            guint           len1,
							    guint           chars1,
                                                            const gchar    *text2,
                                                            guint           len2,
							    guint           chars2);
CtkTextLineSegment *_ctk_toggle_segment_new                (CtkTextTagInfo *info,
                                                            gboolean        on);

void                _ctk_toggle_segment_free               (CtkTextLineSegment *seg);

G_END_DECLS

#endif


