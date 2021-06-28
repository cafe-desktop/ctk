/* CTK - The GIMP Toolkit
 * ctktextbuffer.h Copyright (C) 2000 Red Hat, Inc.
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

#ifndef __CTK_TEXT_BUFFER_H__
#define __CTK_TEXT_BUFFER_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkwidget.h>
#include <ctk/ctkclipboard.h>
#include <ctk/ctktexttagtable.h>
#include <ctk/ctktextiter.h>
#include <ctk/ctktextmark.h>
#include <ctk/ctktextchild.h>

G_BEGIN_DECLS

/*
 * This is the PUBLIC representation of a text buffer.
 * CtkTextBTree is the PRIVATE internal representation of it.
 */

/**
 * CtkTextBufferTargetInfo:
 * @CTK_TEXT_BUFFER_TARGET_INFO_BUFFER_CONTENTS: Buffer contents
 * @CTK_TEXT_BUFFER_TARGET_INFO_RICH_TEXT: Rich text
 * @CTK_TEXT_BUFFER_TARGET_INFO_TEXT: Text
 *
 * These values are used as “info” for the targets contained in the
 * lists returned by ctk_text_buffer_get_copy_target_list() and
 * ctk_text_buffer_get_paste_target_list().
 *
 * The values counts down from `-1` to avoid clashes
 * with application added drag destinations which usually start at 0.
 */
typedef enum
{
  CTK_TEXT_BUFFER_TARGET_INFO_BUFFER_CONTENTS = - 1,
  CTK_TEXT_BUFFER_TARGET_INFO_RICH_TEXT       = - 2,
  CTK_TEXT_BUFFER_TARGET_INFO_TEXT            = - 3
} CtkTextBufferTargetInfo;

typedef struct _CtkTextBTree CtkTextBTree;

#define CTK_TYPE_TEXT_BUFFER            (ctk_text_buffer_get_type ())
#define CTK_TEXT_BUFFER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_TEXT_BUFFER, CtkTextBuffer))
#define CTK_TEXT_BUFFER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_TEXT_BUFFER, CtkTextBufferClass))
#define CTK_IS_TEXT_BUFFER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_TEXT_BUFFER))
#define CTK_IS_TEXT_BUFFER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_TEXT_BUFFER))
#define CTK_TEXT_BUFFER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_TEXT_BUFFER, CtkTextBufferClass))

typedef struct _CtkTextBufferPrivate CtkTextBufferPrivate;
typedef struct _CtkTextBufferClass CtkTextBufferClass;

struct _CtkTextBuffer
{
  GObject parent_instance;

  CtkTextBufferPrivate *priv;
};

/**
 * CtkTextBufferClass:
 * @parent_class: The object class structure needs to be the first.
 * @insert_text: The class handler for the #CtkTextBuffer::insert-text signal.
 * @insert_pixbuf: The class handler for the #CtkTextBuffer::insert-pixbuf
 *   signal.
 * @insert_child_anchor: The class handler for the
 *   #CtkTextBuffer::insert-child-anchor signal.
 * @delete_range: The class handler for the #CtkTextBuffer::delete-range signal.
 * @changed: The class handler for the #CtkTextBuffer::changed signal.
 * @modified_changed: The class handler for the #CtkTextBuffer::modified-changed
 *   signal.
 * @mark_set: The class handler for the #CtkTextBuffer::mark-set signal.
 * @mark_deleted: The class handler for the #CtkTextBuffer::mark-deleted signal.
 * @apply_tag: The class handler for the #CtkTextBuffer::apply-tag signal.
 * @remove_tag: The class handler for the #CtkTextBuffer::remove-tag signal.
 * @begin_user_action: The class handler for the
 *   #CtkTextBuffer::begin-user-action signal.
 * @end_user_action: The class handler for the #CtkTextBuffer::end-user-action
 *   signal.
 * @paste_done: The class handler for the #CtkTextBuffer::paste-done signal.
 */
struct _CtkTextBufferClass
{
  GObjectClass parent_class;

  void (* insert_text)            (CtkTextBuffer      *buffer,
                                   CtkTextIter        *pos,
                                   const gchar        *new_text,
                                   gint                new_text_length);

  void (* insert_pixbuf)          (CtkTextBuffer      *buffer,
                                   CtkTextIter        *iter,
                                   GdkPixbuf          *pixbuf);

  void (* insert_child_anchor)    (CtkTextBuffer      *buffer,
                                   CtkTextIter        *iter,
                                   CtkTextChildAnchor *anchor);

  void (* delete_range)           (CtkTextBuffer      *buffer,
                                   CtkTextIter        *start,
                                   CtkTextIter        *end);

  void (* changed)                (CtkTextBuffer      *buffer);

  void (* modified_changed)       (CtkTextBuffer      *buffer);

  void (* mark_set)               (CtkTextBuffer      *buffer,
                                   const CtkTextIter  *location,
                                   CtkTextMark        *mark);

  void (* mark_deleted)           (CtkTextBuffer      *buffer,
                                   CtkTextMark        *mark);

  void (* apply_tag)              (CtkTextBuffer      *buffer,
                                   CtkTextTag         *tag,
                                   const CtkTextIter  *start,
                                   const CtkTextIter  *end);

  void (* remove_tag)             (CtkTextBuffer      *buffer,
                                   CtkTextTag         *tag,
                                   const CtkTextIter  *start,
                                   const CtkTextIter  *end);

  void (* begin_user_action)      (CtkTextBuffer      *buffer);

  void (* end_user_action)        (CtkTextBuffer      *buffer);

  void (* paste_done)             (CtkTextBuffer      *buffer,
                                   CtkClipboard       *clipboard);

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

CDK_AVAILABLE_IN_ALL
GType        ctk_text_buffer_get_type       (void) G_GNUC_CONST;



/* table is NULL to create a new one */
CDK_AVAILABLE_IN_ALL
CtkTextBuffer *ctk_text_buffer_new            (CtkTextTagTable *table);
CDK_AVAILABLE_IN_ALL
gint           ctk_text_buffer_get_line_count (CtkTextBuffer   *buffer);
CDK_AVAILABLE_IN_ALL
gint           ctk_text_buffer_get_char_count (CtkTextBuffer   *buffer);


CDK_AVAILABLE_IN_ALL
CtkTextTagTable* ctk_text_buffer_get_tag_table (CtkTextBuffer  *buffer);

/* Delete whole buffer, then insert */
CDK_AVAILABLE_IN_ALL
void ctk_text_buffer_set_text          (CtkTextBuffer *buffer,
                                        const gchar   *text,
                                        gint           len);

/* Insert into the buffer */
CDK_AVAILABLE_IN_ALL
void ctk_text_buffer_insert            (CtkTextBuffer *buffer,
                                        CtkTextIter   *iter,
                                        const gchar   *text,
                                        gint           len);
CDK_AVAILABLE_IN_ALL
void ctk_text_buffer_insert_at_cursor  (CtkTextBuffer *buffer,
                                        const gchar   *text,
                                        gint           len);

CDK_AVAILABLE_IN_ALL
gboolean ctk_text_buffer_insert_interactive           (CtkTextBuffer *buffer,
                                                       CtkTextIter   *iter,
                                                       const gchar   *text,
                                                       gint           len,
                                                       gboolean       default_editable);
CDK_AVAILABLE_IN_ALL
gboolean ctk_text_buffer_insert_interactive_at_cursor (CtkTextBuffer *buffer,
                                                       const gchar   *text,
                                                       gint           len,
                                                       gboolean       default_editable);

CDK_AVAILABLE_IN_ALL
void     ctk_text_buffer_insert_range             (CtkTextBuffer     *buffer,
                                                   CtkTextIter       *iter,
                                                   const CtkTextIter *start,
                                                   const CtkTextIter *end);
CDK_AVAILABLE_IN_ALL
gboolean ctk_text_buffer_insert_range_interactive (CtkTextBuffer     *buffer,
                                                   CtkTextIter       *iter,
                                                   const CtkTextIter *start,
                                                   const CtkTextIter *end,
                                                   gboolean           default_editable);

CDK_AVAILABLE_IN_ALL
void    ctk_text_buffer_insert_with_tags          (CtkTextBuffer     *buffer,
                                                   CtkTextIter       *iter,
                                                   const gchar       *text,
                                                   gint               len,
                                                   CtkTextTag        *first_tag,
                                                   ...) G_GNUC_NULL_TERMINATED;

CDK_AVAILABLE_IN_ALL
void    ctk_text_buffer_insert_with_tags_by_name  (CtkTextBuffer     *buffer,
                                                   CtkTextIter       *iter,
                                                   const gchar       *text,
                                                   gint               len,
                                                   const gchar       *first_tag_name,
                                                   ...) G_GNUC_NULL_TERMINATED;

CDK_AVAILABLE_IN_3_16
void     ctk_text_buffer_insert_markup            (CtkTextBuffer     *buffer,
                                                   CtkTextIter       *iter,
                                                   const gchar       *markup,
                                                   gint               len);

/* Delete from the buffer */
CDK_AVAILABLE_IN_ALL
void     ctk_text_buffer_delete             (CtkTextBuffer *buffer,
					     CtkTextIter   *start,
					     CtkTextIter   *end);
CDK_AVAILABLE_IN_ALL
gboolean ctk_text_buffer_delete_interactive (CtkTextBuffer *buffer,
					     CtkTextIter   *start_iter,
					     CtkTextIter   *end_iter,
					     gboolean       default_editable);
CDK_AVAILABLE_IN_ALL
gboolean ctk_text_buffer_backspace          (CtkTextBuffer *buffer,
					     CtkTextIter   *iter,
					     gboolean       interactive,
					     gboolean       default_editable);

/* Obtain strings from the buffer */
CDK_AVAILABLE_IN_ALL
gchar          *ctk_text_buffer_get_text            (CtkTextBuffer     *buffer,
                                                     const CtkTextIter *start,
                                                     const CtkTextIter *end,
                                                     gboolean           include_hidden_chars);

CDK_AVAILABLE_IN_ALL
gchar          *ctk_text_buffer_get_slice           (CtkTextBuffer     *buffer,
                                                     const CtkTextIter *start,
                                                     const CtkTextIter *end,
                                                     gboolean           include_hidden_chars);

/* Insert a pixbuf */
CDK_AVAILABLE_IN_ALL
void ctk_text_buffer_insert_pixbuf         (CtkTextBuffer *buffer,
                                            CtkTextIter   *iter,
                                            GdkPixbuf     *pixbuf);

/* Insert a child anchor */
CDK_AVAILABLE_IN_ALL
void               ctk_text_buffer_insert_child_anchor (CtkTextBuffer      *buffer,
                                                        CtkTextIter        *iter,
                                                        CtkTextChildAnchor *anchor);

/* Convenience, create and insert a child anchor */
CDK_AVAILABLE_IN_ALL
CtkTextChildAnchor *ctk_text_buffer_create_child_anchor (CtkTextBuffer *buffer,
                                                         CtkTextIter   *iter);

/* Mark manipulation */
CDK_AVAILABLE_IN_ALL
void           ctk_text_buffer_add_mark    (CtkTextBuffer     *buffer,
                                            CtkTextMark       *mark,
                                            const CtkTextIter *where);
CDK_AVAILABLE_IN_ALL
CtkTextMark   *ctk_text_buffer_create_mark (CtkTextBuffer     *buffer,
                                            const gchar       *mark_name,
                                            const CtkTextIter *where,
                                            gboolean           left_gravity);
CDK_AVAILABLE_IN_ALL
void           ctk_text_buffer_move_mark   (CtkTextBuffer     *buffer,
                                            CtkTextMark       *mark,
                                            const CtkTextIter *where);
CDK_AVAILABLE_IN_ALL
void           ctk_text_buffer_delete_mark (CtkTextBuffer     *buffer,
                                            CtkTextMark       *mark);
CDK_AVAILABLE_IN_ALL
CtkTextMark*   ctk_text_buffer_get_mark    (CtkTextBuffer     *buffer,
                                            const gchar       *name);

CDK_AVAILABLE_IN_ALL
void ctk_text_buffer_move_mark_by_name   (CtkTextBuffer     *buffer,
                                          const gchar       *name,
                                          const CtkTextIter *where);
CDK_AVAILABLE_IN_ALL
void ctk_text_buffer_delete_mark_by_name (CtkTextBuffer     *buffer,
                                          const gchar       *name);

CDK_AVAILABLE_IN_ALL
CtkTextMark* ctk_text_buffer_get_insert          (CtkTextBuffer *buffer);
CDK_AVAILABLE_IN_ALL
CtkTextMark* ctk_text_buffer_get_selection_bound (CtkTextBuffer *buffer);

/* efficiently move insert and selection_bound at the same time */
CDK_AVAILABLE_IN_ALL
void ctk_text_buffer_place_cursor (CtkTextBuffer     *buffer,
                                   const CtkTextIter *where);
CDK_AVAILABLE_IN_ALL
void ctk_text_buffer_select_range (CtkTextBuffer     *buffer,
                                   const CtkTextIter *ins,
				   const CtkTextIter *bound);



/* Tag manipulation */
CDK_AVAILABLE_IN_ALL
void ctk_text_buffer_apply_tag             (CtkTextBuffer     *buffer,
                                            CtkTextTag        *tag,
                                            const CtkTextIter *start,
                                            const CtkTextIter *end);
CDK_AVAILABLE_IN_ALL
void ctk_text_buffer_remove_tag            (CtkTextBuffer     *buffer,
                                            CtkTextTag        *tag,
                                            const CtkTextIter *start,
                                            const CtkTextIter *end);
CDK_AVAILABLE_IN_ALL
void ctk_text_buffer_apply_tag_by_name     (CtkTextBuffer     *buffer,
                                            const gchar       *name,
                                            const CtkTextIter *start,
                                            const CtkTextIter *end);
CDK_AVAILABLE_IN_ALL
void ctk_text_buffer_remove_tag_by_name    (CtkTextBuffer     *buffer,
                                            const gchar       *name,
                                            const CtkTextIter *start,
                                            const CtkTextIter *end);
CDK_AVAILABLE_IN_ALL
void ctk_text_buffer_remove_all_tags       (CtkTextBuffer     *buffer,
                                            const CtkTextIter *start,
                                            const CtkTextIter *end);


/* You can either ignore the return value, or use it to
 * set the attributes of the tag. tag_name can be NULL
 */
CDK_AVAILABLE_IN_ALL
CtkTextTag    *ctk_text_buffer_create_tag (CtkTextBuffer *buffer,
                                           const gchar   *tag_name,
                                           const gchar   *first_property_name,
                                           ...);

/* Obtain iterators pointed at various places, then you can move the
 * iterator around using the CtkTextIter operators
 */
CDK_AVAILABLE_IN_ALL
void ctk_text_buffer_get_iter_at_line_offset (CtkTextBuffer *buffer,
                                              CtkTextIter   *iter,
                                              gint           line_number,
                                              gint           char_offset);
CDK_AVAILABLE_IN_ALL
void ctk_text_buffer_get_iter_at_line_index  (CtkTextBuffer *buffer,
                                              CtkTextIter   *iter,
                                              gint           line_number,
                                              gint           byte_index);
CDK_AVAILABLE_IN_ALL
void ctk_text_buffer_get_iter_at_offset      (CtkTextBuffer *buffer,
                                              CtkTextIter   *iter,
                                              gint           char_offset);
CDK_AVAILABLE_IN_ALL
void ctk_text_buffer_get_iter_at_line        (CtkTextBuffer *buffer,
                                              CtkTextIter   *iter,
                                              gint           line_number);
CDK_AVAILABLE_IN_ALL
void ctk_text_buffer_get_start_iter          (CtkTextBuffer *buffer,
                                              CtkTextIter   *iter);
CDK_AVAILABLE_IN_ALL
void ctk_text_buffer_get_end_iter            (CtkTextBuffer *buffer,
                                              CtkTextIter   *iter);
CDK_AVAILABLE_IN_ALL
void ctk_text_buffer_get_bounds              (CtkTextBuffer *buffer,
                                              CtkTextIter   *start,
                                              CtkTextIter   *end);
CDK_AVAILABLE_IN_ALL
void ctk_text_buffer_get_iter_at_mark        (CtkTextBuffer *buffer,
                                              CtkTextIter   *iter,
                                              CtkTextMark   *mark);

CDK_AVAILABLE_IN_ALL
void ctk_text_buffer_get_iter_at_child_anchor (CtkTextBuffer      *buffer,
                                               CtkTextIter        *iter,
                                               CtkTextChildAnchor *anchor);

/* There's no get_first_iter because you just get the iter for
   line or char 0 */

/* Used to keep track of whether the buffer needs saving; anytime the
   buffer contents change, the modified flag is turned on. Whenever
   you save, turn it off. Tags and marks do not affect the modified
   flag, but if you would like them to you can connect a handler to
   the tag/mark signals and call set_modified in your handler */

CDK_AVAILABLE_IN_ALL
gboolean        ctk_text_buffer_get_modified            (CtkTextBuffer *buffer);
CDK_AVAILABLE_IN_ALL
void            ctk_text_buffer_set_modified            (CtkTextBuffer *buffer,
                                                         gboolean       setting);

CDK_AVAILABLE_IN_ALL
gboolean        ctk_text_buffer_get_has_selection       (CtkTextBuffer *buffer);

CDK_AVAILABLE_IN_ALL
void ctk_text_buffer_add_selection_clipboard    (CtkTextBuffer     *buffer,
						 CtkClipboard      *clipboard);
CDK_AVAILABLE_IN_ALL
void ctk_text_buffer_remove_selection_clipboard (CtkTextBuffer     *buffer,
						 CtkClipboard      *clipboard);

CDK_AVAILABLE_IN_ALL
void            ctk_text_buffer_cut_clipboard           (CtkTextBuffer *buffer,
							 CtkClipboard  *clipboard,
                                                         gboolean       default_editable);
CDK_AVAILABLE_IN_ALL
void            ctk_text_buffer_copy_clipboard          (CtkTextBuffer *buffer,
							 CtkClipboard  *clipboard);
CDK_AVAILABLE_IN_ALL
void            ctk_text_buffer_paste_clipboard         (CtkTextBuffer *buffer,
							 CtkClipboard  *clipboard,
							 CtkTextIter   *override_location,
                                                         gboolean       default_editable);

CDK_AVAILABLE_IN_ALL
gboolean        ctk_text_buffer_get_selection_bounds    (CtkTextBuffer *buffer,
                                                         CtkTextIter   *start,
                                                         CtkTextIter   *end);
CDK_AVAILABLE_IN_ALL
gboolean        ctk_text_buffer_delete_selection        (CtkTextBuffer *buffer,
                                                         gboolean       interactive,
                                                         gboolean       default_editable);

/* Called to specify atomic user actions, used to implement undo */
CDK_AVAILABLE_IN_ALL
void            ctk_text_buffer_begin_user_action       (CtkTextBuffer *buffer);
CDK_AVAILABLE_IN_ALL
void            ctk_text_buffer_end_user_action         (CtkTextBuffer *buffer);

CDK_AVAILABLE_IN_ALL
CtkTargetList * ctk_text_buffer_get_copy_target_list    (CtkTextBuffer *buffer);
CDK_AVAILABLE_IN_ALL
CtkTargetList * ctk_text_buffer_get_paste_target_list   (CtkTextBuffer *buffer);


G_END_DECLS

#endif
