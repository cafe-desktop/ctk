/* CTK - The GIMP Toolkit
 * ctktextiter.h Copyright (C) 2000 Red Hat, Inc.
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

#ifndef __CTK_TEXT_ITER_H__
#define __CTK_TEXT_ITER_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctktextattributes.h>
#include <ctk/ctktextchild.h>
#include <ctk/ctktexttag.h>

G_BEGIN_DECLS

/**
 * CtkTextSearchFlags:
 * @CTK_TEXT_SEARCH_VISIBLE_ONLY: Search only visible data. A search match may
 * have invisible text interspersed.
 * @CTK_TEXT_SEARCH_TEXT_ONLY: Search only text. A match may have pixbufs or
 * child widgets mixed inside the matched range.
 * @CTK_TEXT_SEARCH_CASE_INSENSITIVE: The text will be matched regardless of
 * what case it is in.
 *
 * Flags affecting how a search is done.
 *
 * If neither #CTK_TEXT_SEARCH_VISIBLE_ONLY nor #CTK_TEXT_SEARCH_TEXT_ONLY are
 * enabled, the match must be exact; the special 0xFFFC character will match
 * embedded pixbufs or child widgets.
 */
typedef enum {
  CTK_TEXT_SEARCH_VISIBLE_ONLY     = 1 << 0,
  CTK_TEXT_SEARCH_TEXT_ONLY        = 1 << 1,
  CTK_TEXT_SEARCH_CASE_INSENSITIVE = 1 << 2
  /* Possible future plans: SEARCH_REGEXP */
} CtkTextSearchFlags;

/*
 * Iter: represents a location in the text. Becomes invalid if the
 * characters/pixmaps/widgets (indexable objects) in the text buffer
 * are changed.
 */

typedef struct _CtkTextBuffer CtkTextBuffer;

#define CTK_TYPE_TEXT_ITER     (ctk_text_iter_get_type ())

struct _CtkTextIter {
  /* CtkTextIter is an opaque datatype; ignore all these fields.
   * Initialize the iter with ctk_text_buffer_get_iter_*
   * functions
   */
  /*< private >*/
  gpointer dummy1;
  gpointer dummy2;
  gint dummy3;
  gint dummy4;
  gint dummy5;
  gint dummy6;
  gint dummy7;
  gint dummy8;
  gpointer dummy9;
  gpointer dummy10;
  gint dummy11;
  gint dummy12;
  /* padding */
  gint dummy13;
  gpointer dummy14;
};


/* This is primarily intended for language bindings that want to avoid
   a "buffer" argument to text insertions, deletions, etc. */
GDK_AVAILABLE_IN_ALL
CtkTextBuffer *ctk_text_iter_get_buffer (const CtkTextIter *iter);

/*
 * Life cycle
 */

GDK_AVAILABLE_IN_ALL
CtkTextIter *ctk_text_iter_copy     (const CtkTextIter *iter);
GDK_AVAILABLE_IN_ALL
void         ctk_text_iter_free     (CtkTextIter       *iter);
GDK_AVAILABLE_IN_3_2
void         ctk_text_iter_assign   (CtkTextIter       *iter,
                                     const CtkTextIter *other);

GDK_AVAILABLE_IN_ALL
GType        ctk_text_iter_get_type (void) G_GNUC_CONST;

/*
 * Convert to different kinds of index
 */

GDK_AVAILABLE_IN_ALL
gint ctk_text_iter_get_offset      (const CtkTextIter *iter);
GDK_AVAILABLE_IN_ALL
gint ctk_text_iter_get_line        (const CtkTextIter *iter);
GDK_AVAILABLE_IN_ALL
gint ctk_text_iter_get_line_offset (const CtkTextIter *iter);
GDK_AVAILABLE_IN_ALL
gint ctk_text_iter_get_line_index  (const CtkTextIter *iter);

GDK_AVAILABLE_IN_ALL
gint ctk_text_iter_get_visible_line_offset (const CtkTextIter *iter);
GDK_AVAILABLE_IN_ALL
gint ctk_text_iter_get_visible_line_index (const CtkTextIter *iter);


/*
 * “Dereference” operators
 */
GDK_AVAILABLE_IN_ALL
gunichar ctk_text_iter_get_char          (const CtkTextIter  *iter);

/* includes the 0xFFFC char for pixmaps/widgets, so char offsets
 * into the returned string map properly into buffer char offsets
 */
GDK_AVAILABLE_IN_ALL
gchar   *ctk_text_iter_get_slice         (const CtkTextIter  *start,
                                          const CtkTextIter  *end);

/* includes only text, no 0xFFFC */
GDK_AVAILABLE_IN_ALL
gchar   *ctk_text_iter_get_text          (const CtkTextIter  *start,
                                          const CtkTextIter  *end);
/* exclude invisible chars */
GDK_AVAILABLE_IN_ALL
gchar   *ctk_text_iter_get_visible_slice (const CtkTextIter  *start,
                                          const CtkTextIter  *end);
GDK_AVAILABLE_IN_ALL
gchar   *ctk_text_iter_get_visible_text  (const CtkTextIter  *start,
                                          const CtkTextIter  *end);

GDK_AVAILABLE_IN_ALL
CdkPixbuf* ctk_text_iter_get_pixbuf (const CtkTextIter *iter);
GDK_AVAILABLE_IN_ALL
GSList  *  ctk_text_iter_get_marks  (const CtkTextIter *iter);

GDK_AVAILABLE_IN_ALL
CtkTextChildAnchor* ctk_text_iter_get_child_anchor (const CtkTextIter *iter);

/* Return list of tags toggled at this point (toggled_on determines
 * whether the list is of on-toggles or off-toggles)
 */
GDK_AVAILABLE_IN_ALL
GSList  *ctk_text_iter_get_toggled_tags  (const CtkTextIter  *iter,
                                          gboolean            toggled_on);

GDK_AVAILABLE_IN_3_20
gboolean ctk_text_iter_starts_tag        (const CtkTextIter  *iter,
                                          CtkTextTag         *tag);

GDK_DEPRECATED_IN_3_20_FOR(ctk_text_iter_starts_tag)
gboolean ctk_text_iter_begins_tag        (const CtkTextIter  *iter,
                                          CtkTextTag         *tag);

GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_ends_tag          (const CtkTextIter  *iter,
                                          CtkTextTag         *tag);

GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_toggles_tag       (const CtkTextIter  *iter,
                                          CtkTextTag         *tag);

GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_has_tag           (const CtkTextIter   *iter,
                                          CtkTextTag          *tag);
GDK_AVAILABLE_IN_ALL
GSList  *ctk_text_iter_get_tags          (const CtkTextIter   *iter);

GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_editable          (const CtkTextIter   *iter,
                                          gboolean             default_setting);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_can_insert        (const CtkTextIter   *iter,
                                          gboolean             default_editability);

GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_starts_word        (const CtkTextIter   *iter);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_ends_word          (const CtkTextIter   *iter);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_inside_word        (const CtkTextIter   *iter);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_starts_sentence    (const CtkTextIter   *iter);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_ends_sentence      (const CtkTextIter   *iter);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_inside_sentence    (const CtkTextIter   *iter);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_starts_line        (const CtkTextIter   *iter);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_ends_line          (const CtkTextIter   *iter);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_is_cursor_position (const CtkTextIter   *iter);

GDK_AVAILABLE_IN_ALL
gint     ctk_text_iter_get_chars_in_line (const CtkTextIter   *iter);
GDK_AVAILABLE_IN_ALL
gint     ctk_text_iter_get_bytes_in_line (const CtkTextIter   *iter);

GDK_AVAILABLE_IN_ALL
gboolean       ctk_text_iter_get_attributes (const CtkTextIter *iter,
					     CtkTextAttributes *values);
GDK_AVAILABLE_IN_ALL
PangoLanguage* ctk_text_iter_get_language   (const CtkTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean       ctk_text_iter_is_end         (const CtkTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean       ctk_text_iter_is_start       (const CtkTextIter *iter);

/*
 * Moving around the buffer
 */

GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_forward_char         (CtkTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_backward_char        (CtkTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_forward_chars        (CtkTextIter *iter,
                                             gint         count);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_backward_chars       (CtkTextIter *iter,
                                             gint         count);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_forward_line         (CtkTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_backward_line        (CtkTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_forward_lines        (CtkTextIter *iter,
                                             gint         count);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_backward_lines       (CtkTextIter *iter,
                                             gint         count);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_forward_word_end     (CtkTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_backward_word_start  (CtkTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_forward_word_ends    (CtkTextIter *iter,
                                             gint         count);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_backward_word_starts (CtkTextIter *iter,
                                             gint         count);
                                             
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_forward_visible_line   (CtkTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_backward_visible_line  (CtkTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_forward_visible_lines  (CtkTextIter *iter,
                                               gint         count);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_backward_visible_lines (CtkTextIter *iter,
                                               gint         count);

GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_forward_visible_word_end     (CtkTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_backward_visible_word_start  (CtkTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_forward_visible_word_ends    (CtkTextIter *iter,
                                             gint         count);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_backward_visible_word_starts (CtkTextIter *iter,
                                             gint         count);

GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_forward_sentence_end     (CtkTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_backward_sentence_start  (CtkTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_forward_sentence_ends    (CtkTextIter *iter,
                                                 gint         count);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_backward_sentence_starts (CtkTextIter *iter,
                                                 gint         count);
/* cursor positions are almost equivalent to chars, but not quite;
 * in some languages, you can’t put the cursor between certain
 * chars. Also, you can’t put the cursor between \r\n at the end
 * of a line.
 */
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_forward_cursor_position   (CtkTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_backward_cursor_position  (CtkTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_forward_cursor_positions  (CtkTextIter *iter,
                                                  gint         count);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_backward_cursor_positions (CtkTextIter *iter,
                                                  gint         count);

GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_forward_visible_cursor_position   (CtkTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_backward_visible_cursor_position  (CtkTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_forward_visible_cursor_positions  (CtkTextIter *iter,
                                                          gint         count);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_backward_visible_cursor_positions (CtkTextIter *iter,
                                                          gint         count);

GDK_AVAILABLE_IN_ALL
void     ctk_text_iter_set_offset         (CtkTextIter *iter,
                                           gint         char_offset);
GDK_AVAILABLE_IN_ALL
void     ctk_text_iter_set_line           (CtkTextIter *iter,
                                           gint         line_number);
GDK_AVAILABLE_IN_ALL
void     ctk_text_iter_set_line_offset    (CtkTextIter *iter,
                                           gint         char_on_line);
GDK_AVAILABLE_IN_ALL
void     ctk_text_iter_set_line_index     (CtkTextIter *iter,
                                           gint         byte_on_line);
GDK_AVAILABLE_IN_ALL
void     ctk_text_iter_forward_to_end     (CtkTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_forward_to_line_end (CtkTextIter *iter);

GDK_AVAILABLE_IN_ALL
void     ctk_text_iter_set_visible_line_offset (CtkTextIter *iter,
                                                gint         char_on_line);
GDK_AVAILABLE_IN_ALL
void     ctk_text_iter_set_visible_line_index  (CtkTextIter *iter,
                                                gint         byte_on_line);

/* returns TRUE if a toggle was found; NULL for the tag pointer
 * means “any tag toggle”, otherwise the next toggle of the
 * specified tag is located.
 */
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_forward_to_tag_toggle (CtkTextIter *iter,
                                              CtkTextTag  *tag);

GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_backward_to_tag_toggle (CtkTextIter *iter,
                                               CtkTextTag  *tag);

typedef gboolean (* CtkTextCharPredicate) (gunichar ch, gpointer user_data);

GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_forward_find_char  (CtkTextIter          *iter,
                                           CtkTextCharPredicate  pred,
                                           gpointer              user_data,
                                           const CtkTextIter    *limit);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_backward_find_char (CtkTextIter          *iter,
                                           CtkTextCharPredicate  pred,
                                           gpointer              user_data,
                                           const CtkTextIter    *limit);

GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_forward_search  (const CtkTextIter *iter,
                                        const gchar       *str,
                                        CtkTextSearchFlags flags,
                                        CtkTextIter       *match_start,
                                        CtkTextIter       *match_end,
                                        const CtkTextIter *limit);

GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_backward_search (const CtkTextIter *iter,
                                        const gchar       *str,
                                        CtkTextSearchFlags flags,
                                        CtkTextIter       *match_start,
                                        CtkTextIter       *match_end,
                                        const CtkTextIter *limit);

/*
 * Comparisons
 */
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_equal           (const CtkTextIter *lhs,
                                        const CtkTextIter *rhs);
GDK_AVAILABLE_IN_ALL
gint     ctk_text_iter_compare         (const CtkTextIter *lhs,
                                        const CtkTextIter *rhs);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_iter_in_range        (const CtkTextIter *iter,
                                        const CtkTextIter *start,
                                        const CtkTextIter *end);

/* Put these two in ascending order */
GDK_AVAILABLE_IN_ALL
void     ctk_text_iter_order           (CtkTextIter *first,
                                        CtkTextIter *second);

G_END_DECLS

#endif
