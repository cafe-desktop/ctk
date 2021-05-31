/* GTK - The GIMP Toolkit
 * ctktextbuffer.c Copyright (C) 2000 Red Hat, Inc.
 *                 Copyright (C) 2004 Nokia Corporation
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
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#include "config.h"
#include <string.h>
#include <stdarg.h>

#define CTK_TEXT_USE_INTERNAL_UNSUPPORTED_API
#include "ctkclipboard.h"
#include "ctkdnd.h"
#include "ctkinvisible.h"
#include "ctkmarshalers.h"
#include "ctktextbuffer.h"
#include "ctktextbufferprivate.h"
#include "ctktextbufferrichtext.h"
#include "ctktextbtree.h"
#include "ctktextiterprivate.h"
#include "ctktexttagprivate.h"
#include "ctktexttagtableprivate.h"
#include "ctkprivate.h"
#include "ctkintl.h"

/**
 * SECTION:ctktextbuffer
 * @Short_description: Stores attributed text for display in a CtkTextView
 * @Title: CtkTextBuffer
 * @See_also: #CtkTextView, #CtkTextIter, #CtkTextMark
 *
 * You may wish to begin by reading the
 * [text widget conceptual overview][TextWidget]
 * which gives an overview of all the objects and data
 * types related to the text widget and how they work together.
 */

typedef struct _CtkTextLogAttrCache CtkTextLogAttrCache;

struct _CtkTextBufferPrivate
{
  CtkTargetList  *copy_target_list;
  CtkTargetEntry *copy_target_entries;
  CtkTargetList  *paste_target_list;
  CtkTargetEntry *paste_target_entries;

  gint            n_copy_target_entries;
  gint            n_paste_target_entries;

  CtkTextTagTable *tag_table;
  CtkTextBTree *btree;

  GSList *clipboard_contents_buffers;
  GSList *selection_clipboards;

  CtkTextLogAttrCache *log_attr_cache;

  guint user_action_count;

  /* Whether the buffer has been modified since last save */
  guint modified : 1;
  guint has_selection : 1;
};

typedef struct _ClipboardRequest ClipboardRequest;

struct _ClipboardRequest
{
  CtkTextBuffer *buffer;
  guint interactive : 1;
  guint default_editable : 1;
  guint replace_selection : 1;
};

enum {
  INSERT_TEXT,
  INSERT_PIXBUF,
  INSERT_CHILD_ANCHOR,
  DELETE_RANGE,
  CHANGED,
  MODIFIED_CHANGED,
  MARK_SET,
  MARK_DELETED,
  APPLY_TAG,
  REMOVE_TAG,
  BEGIN_USER_ACTION,
  END_USER_ACTION,
  PASTE_DONE,
  LAST_SIGNAL
};

enum {
  PROP_0,

  /* Construct */
  PROP_TAG_TABLE,

  /* Normal */
  PROP_TEXT,
  PROP_HAS_SELECTION,
  PROP_CURSOR_POSITION,
  PROP_COPY_TARGET_LIST,
  PROP_PASTE_TARGET_LIST,
  LAST_PROP
};

static void ctk_text_buffer_finalize   (GObject            *object);

static void ctk_text_buffer_real_insert_text           (CtkTextBuffer     *buffer,
                                                        CtkTextIter       *iter,
                                                        const gchar       *text,
                                                        gint               len);
static void ctk_text_buffer_real_insert_pixbuf         (CtkTextBuffer     *buffer,
                                                        CtkTextIter       *iter,
                                                        GdkPixbuf         *pixbuf);
static void ctk_text_buffer_real_insert_anchor         (CtkTextBuffer     *buffer,
                                                        CtkTextIter       *iter,
                                                        CtkTextChildAnchor *anchor);
static void ctk_text_buffer_real_delete_range          (CtkTextBuffer     *buffer,
                                                        CtkTextIter       *start,
                                                        CtkTextIter       *end);
static void ctk_text_buffer_real_apply_tag             (CtkTextBuffer     *buffer,
                                                        CtkTextTag        *tag,
                                                        const CtkTextIter *start_char,
                                                        const CtkTextIter *end_char);
static void ctk_text_buffer_real_remove_tag            (CtkTextBuffer     *buffer,
                                                        CtkTextTag        *tag,
                                                        const CtkTextIter *start_char,
                                                        const CtkTextIter *end_char);
static void ctk_text_buffer_real_changed               (CtkTextBuffer     *buffer);
static void ctk_text_buffer_real_mark_set              (CtkTextBuffer     *buffer,
                                                        const CtkTextIter *iter,
                                                        CtkTextMark       *mark);

static CtkTextBTree* get_btree (CtkTextBuffer *buffer);
static void          free_log_attr_cache (CtkTextLogAttrCache *cache);

static void remove_all_selection_clipboards       (CtkTextBuffer *buffer);
static void update_selection_clipboards           (CtkTextBuffer *buffer);

static CtkTextBuffer *create_clipboard_contents_buffer (CtkTextBuffer *buffer);

static void ctk_text_buffer_free_target_lists     (CtkTextBuffer *buffer);

static void ctk_text_buffer_set_property (GObject         *object,
				          guint            prop_id,
				          const GValue    *value,
				          GParamSpec      *pspec);
static void ctk_text_buffer_get_property (GObject         *object,
				          guint            prop_id,
				          GValue          *value,
				          GParamSpec      *pspec);
static void ctk_text_buffer_notify       (GObject         *object,
                                          GParamSpec      *pspec);

static guint signals[LAST_SIGNAL] = { 0 };
static GParamSpec *text_buffer_props[LAST_PROP];

G_DEFINE_TYPE_WITH_PRIVATE (CtkTextBuffer, ctk_text_buffer, G_TYPE_OBJECT)

static void
ctk_text_buffer_class_init (CtkTextBufferClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = ctk_text_buffer_finalize;
  object_class->set_property = ctk_text_buffer_set_property;
  object_class->get_property = ctk_text_buffer_get_property;
  object_class->notify       = ctk_text_buffer_notify;
 
  klass->insert_text = ctk_text_buffer_real_insert_text;
  klass->insert_pixbuf = ctk_text_buffer_real_insert_pixbuf;
  klass->insert_child_anchor = ctk_text_buffer_real_insert_anchor;
  klass->delete_range = ctk_text_buffer_real_delete_range;
  klass->apply_tag = ctk_text_buffer_real_apply_tag;
  klass->remove_tag = ctk_text_buffer_real_remove_tag;
  klass->changed = ctk_text_buffer_real_changed;
  klass->mark_set = ctk_text_buffer_real_mark_set;

  /* Construct */
  text_buffer_props[PROP_TAG_TABLE] =
      g_param_spec_object ("tag-table",
                           P_("Tag Table"),
                           P_("Text Tag Table"),
                           CTK_TYPE_TEXT_TAG_TABLE,
                           CTK_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  /* Normal properties */

  /**
   * CtkTextBuffer:text:
   *
   * The text content of the buffer. Without child widgets and images,
   * see ctk_text_buffer_get_text() for more information.
   *
   * Since: 2.8
   */
  text_buffer_props[PROP_TEXT] =
      g_param_spec_string ("text",
                           P_("Text"),
                           P_("Current text of the buffer"),
                           "",
                           CTK_PARAM_READWRITE);

  /**
   * CtkTextBuffer:has-selection:
   *
   * Whether the buffer has some text currently selected.
   *
   * Since: 2.10
   */
  text_buffer_props[PROP_HAS_SELECTION] =
      g_param_spec_boolean ("has-selection",
                            P_("Has selection"),
                            P_("Whether the buffer has some text currently selected"),
                            FALSE,
                            CTK_PARAM_READABLE);

  /**
   * CtkTextBuffer:cursor-position:
   *
   * The position of the insert mark (as offset from the beginning
   * of the buffer). It is useful for getting notified when the
   * cursor moves.
   *
   * Since: 2.10
   */
  text_buffer_props[PROP_CURSOR_POSITION] =
      g_param_spec_int ("cursor-position",
                        P_("Cursor position"),
                        P_("The position of the insert mark (as offset from the beginning of the buffer)"),
			0, G_MAXINT,
                        0,
                        CTK_PARAM_READABLE);

  /**
   * CtkTextBuffer:copy-target-list:
   *
   * The list of targets this buffer supports for clipboard copying
   * and as DND source.
   *
   * Since: 2.10
   */
  text_buffer_props[PROP_COPY_TARGET_LIST] =
      g_param_spec_boxed ("copy-target-list",
                          P_("Copy target list"),
                          P_("The list of targets this buffer supports for clipboard copying and DND source"),
                          CTK_TYPE_TARGET_LIST,
                          CTK_PARAM_READABLE);

  /**
   * CtkTextBuffer:paste-target-list:
   *
   * The list of targets this buffer supports for clipboard pasting
   * and as DND destination.
   *
   * Since: 2.10
   */
  text_buffer_props[PROP_PASTE_TARGET_LIST] =
      g_param_spec_boxed ("paste-target-list",
                          P_("Paste target list"),
                          P_("The list of targets this buffer supports for clipboard pasting and DND destination"),
                          CTK_TYPE_TARGET_LIST,
                          CTK_PARAM_READABLE);

  g_object_class_install_properties (object_class, LAST_PROP, text_buffer_props);

  /**
   * CtkTextBuffer::insert-text:
   * @textbuffer: the object which received the signal
   * @location: position to insert @text in @textbuffer
   * @text: the UTF-8 text to be inserted
   * @len: length of the inserted text in bytes
   * 
   * The ::insert-text signal is emitted to insert text in a #CtkTextBuffer.
   * Insertion actually occurs in the default handler.  
   * 
   * Note that if your handler runs before the default handler it must not 
   * invalidate the @location iter (or has to revalidate it). 
   * The default signal handler revalidates it to point to the end of the 
   * inserted text.
   * 
   * See also: 
   * ctk_text_buffer_insert(), 
   * ctk_text_buffer_insert_range().
   */
  signals[INSERT_TEXT] =
    g_signal_new (I_("insert-text"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkTextBufferClass, insert_text),
                  NULL, NULL,
                  _ctk_marshal_VOID__BOXED_STRING_INT,
                  G_TYPE_NONE,
                  3,
                  CTK_TYPE_TEXT_ITER | G_SIGNAL_TYPE_STATIC_SCOPE,
                  G_TYPE_STRING | G_SIGNAL_TYPE_STATIC_SCOPE,
                  G_TYPE_INT);
  g_signal_set_va_marshaller (signals[INSERT_TEXT], G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_VOID__BOXED_STRING_INTv);

  /**
   * CtkTextBuffer::insert-pixbuf:
   * @textbuffer: the object which received the signal
   * @location: position to insert @pixbuf in @textbuffer
   * @pixbuf: the #GdkPixbuf to be inserted
   * 
   * The ::insert-pixbuf signal is emitted to insert a #GdkPixbuf 
   * in a #CtkTextBuffer. Insertion actually occurs in the default handler.
   * 
   * Note that if your handler runs before the default handler it must not 
   * invalidate the @location iter (or has to revalidate it). 
   * The default signal handler revalidates it to be placed after the 
   * inserted @pixbuf.
   * 
   * See also: ctk_text_buffer_insert_pixbuf().
   */
  signals[INSERT_PIXBUF] =
    g_signal_new (I_("insert-pixbuf"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkTextBufferClass, insert_pixbuf),
                  NULL, NULL,
                  _ctk_marshal_VOID__BOXED_OBJECT,
                  G_TYPE_NONE,
                  2,
                  CTK_TYPE_TEXT_ITER | G_SIGNAL_TYPE_STATIC_SCOPE,
                  GDK_TYPE_PIXBUF);
  g_signal_set_va_marshaller (signals[INSERT_PIXBUF],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_VOID__BOXED_OBJECTv);


  /**
   * CtkTextBuffer::insert-child-anchor:
   * @textbuffer: the object which received the signal
   * @location: position to insert @anchor in @textbuffer
   * @anchor: the #CtkTextChildAnchor to be inserted
   * 
   * The ::insert-child-anchor signal is emitted to insert a
   * #CtkTextChildAnchor in a #CtkTextBuffer.
   * Insertion actually occurs in the default handler.
   * 
   * Note that if your handler runs before the default handler it must
   * not invalidate the @location iter (or has to revalidate it). 
   * The default signal handler revalidates it to be placed after the 
   * inserted @anchor.
   * 
   * See also: ctk_text_buffer_insert_child_anchor().
   */
  signals[INSERT_CHILD_ANCHOR] =
    g_signal_new (I_("insert-child-anchor"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkTextBufferClass, insert_child_anchor),
                  NULL, NULL,
                  _ctk_marshal_VOID__BOXED_OBJECT,
                  G_TYPE_NONE,
                  2,
                  CTK_TYPE_TEXT_ITER | G_SIGNAL_TYPE_STATIC_SCOPE,
                  CTK_TYPE_TEXT_CHILD_ANCHOR);
  g_signal_set_va_marshaller (signals[INSERT_CHILD_ANCHOR],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_VOID__BOXED_OBJECTv);
  
  /**
   * CtkTextBuffer::delete-range:
   * @textbuffer: the object which received the signal
   * @start: the start of the range to be deleted
   * @end: the end of the range to be deleted
   * 
   * The ::delete-range signal is emitted to delete a range 
   * from a #CtkTextBuffer. 
   * 
   * Note that if your handler runs before the default handler it must not 
   * invalidate the @start and @end iters (or has to revalidate them). 
   * The default signal handler revalidates the @start and @end iters to 
   * both point to the location where text was deleted. Handlers
   * which run after the default handler (see g_signal_connect_after())
   * do not have access to the deleted text.
   * 
   * See also: ctk_text_buffer_delete().
   */
  signals[DELETE_RANGE] =
    g_signal_new (I_("delete-range"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkTextBufferClass, delete_range),
                  NULL, NULL,
                  _ctk_marshal_VOID__BOXED_BOXED,
                  G_TYPE_NONE,
                  2,
                  CTK_TYPE_TEXT_ITER | G_SIGNAL_TYPE_STATIC_SCOPE,
                  CTK_TYPE_TEXT_ITER | G_SIGNAL_TYPE_STATIC_SCOPE);
  g_signal_set_va_marshaller (signals[DELETE_RANGE],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_VOID__BOXED_BOXEDv);

  /**
   * CtkTextBuffer::changed:
   * @textbuffer: the object which received the signal
   * 
   * The ::changed signal is emitted when the content of a #CtkTextBuffer 
   * has changed.
   */
  signals[CHANGED] =
    g_signal_new (I_("changed"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,                   
                  G_STRUCT_OFFSET (CtkTextBufferClass, changed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  0);

  /**
   * CtkTextBuffer::modified-changed:
   * @textbuffer: the object which received the signal
   * 
   * The ::modified-changed signal is emitted when the modified bit of a 
   * #CtkTextBuffer flips.
   * 
   * See also:
   * ctk_text_buffer_set_modified().
   */
  signals[MODIFIED_CHANGED] =
    g_signal_new (I_("modified-changed"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkTextBufferClass, modified_changed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  0);

  /**
   * CtkTextBuffer::mark-set:
   * @textbuffer: the object which received the signal
   * @location: The location of @mark in @textbuffer
   * @mark: The mark that is set
   * 
   * The ::mark-set signal is emitted as notification
   * after a #CtkTextMark is set.
   * 
   * See also: 
   * ctk_text_buffer_create_mark(),
   * ctk_text_buffer_move_mark().
   */
  signals[MARK_SET] =
    g_signal_new (I_("mark-set"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkTextBufferClass, mark_set),
                  NULL, NULL,
                  _ctk_marshal_VOID__BOXED_OBJECT,
                  G_TYPE_NONE,
                  2,
                  CTK_TYPE_TEXT_ITER,
                  CTK_TYPE_TEXT_MARK);
  g_signal_set_va_marshaller (signals[MARK_SET],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_VOID__BOXED_OBJECTv);

  /**
   * CtkTextBuffer::mark-deleted:
   * @textbuffer: the object which received the signal
   * @mark: The mark that was deleted
   * 
   * The ::mark-deleted signal is emitted as notification
   * after a #CtkTextMark is deleted. 
   * 
   * See also:
   * ctk_text_buffer_delete_mark().
   */
  signals[MARK_DELETED] =
    g_signal_new (I_("mark-deleted"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,                   
                  G_STRUCT_OFFSET (CtkTextBufferClass, mark_deleted),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  1,
                  CTK_TYPE_TEXT_MARK);

   /**
   * CtkTextBuffer::apply-tag:
   * @textbuffer: the object which received the signal
   * @tag: the applied tag
   * @start: the start of the range the tag is applied to
   * @end: the end of the range the tag is applied to
   * 
   * The ::apply-tag signal is emitted to apply a tag to a
   * range of text in a #CtkTextBuffer. 
   * Applying actually occurs in the default handler.
   * 
   * Note that if your handler runs before the default handler it must not 
   * invalidate the @start and @end iters (or has to revalidate them). 
   * 
   * See also: 
   * ctk_text_buffer_apply_tag(),
   * ctk_text_buffer_insert_with_tags(),
   * ctk_text_buffer_insert_range().
   */ 
  signals[APPLY_TAG] =
    g_signal_new (I_("apply-tag"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkTextBufferClass, apply_tag),
                  NULL, NULL,
                  _ctk_marshal_VOID__OBJECT_BOXED_BOXED,
                  G_TYPE_NONE,
                  3,
                  CTK_TYPE_TEXT_TAG,
                  CTK_TYPE_TEXT_ITER,
                  CTK_TYPE_TEXT_ITER);
  g_signal_set_va_marshaller (signals[APPLY_TAG],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_VOID__OBJECT_BOXED_BOXEDv);


   /**
   * CtkTextBuffer::remove-tag:
   * @textbuffer: the object which received the signal
   * @tag: the tag to be removed
   * @start: the start of the range the tag is removed from
   * @end: the end of the range the tag is removed from
   * 
   * The ::remove-tag signal is emitted to remove all occurrences of @tag from
   * a range of text in a #CtkTextBuffer. 
   * Removal actually occurs in the default handler.
   * 
   * Note that if your handler runs before the default handler it must not 
   * invalidate the @start and @end iters (or has to revalidate them). 
   * 
   * See also: 
   * ctk_text_buffer_remove_tag(). 
   */ 
  signals[REMOVE_TAG] =
    g_signal_new (I_("remove-tag"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkTextBufferClass, remove_tag),
                  NULL, NULL,
                  _ctk_marshal_VOID__OBJECT_BOXED_BOXED,
                  G_TYPE_NONE,
                  3,
                  CTK_TYPE_TEXT_TAG,
                  CTK_TYPE_TEXT_ITER,
                  CTK_TYPE_TEXT_ITER);
  g_signal_set_va_marshaller (signals[REMOVE_TAG],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_VOID__OBJECT_BOXED_BOXEDv);

   /**
   * CtkTextBuffer::begin-user-action:
   * @textbuffer: the object which received the signal
   * 
   * The ::begin-user-action signal is emitted at the beginning of a single
   * user-visible operation on a #CtkTextBuffer.
   * 
   * See also: 
   * ctk_text_buffer_begin_user_action(),
   * ctk_text_buffer_insert_interactive(),
   * ctk_text_buffer_insert_range_interactive(),
   * ctk_text_buffer_delete_interactive(),
   * ctk_text_buffer_backspace(),
   * ctk_text_buffer_delete_selection().
   */ 
  signals[BEGIN_USER_ACTION] =
    g_signal_new (I_("begin-user-action"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,                   
                  G_STRUCT_OFFSET (CtkTextBufferClass, begin_user_action),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  0);

   /**
   * CtkTextBuffer::end-user-action:
   * @textbuffer: the object which received the signal
   * 
   * The ::end-user-action signal is emitted at the end of a single
   * user-visible operation on the #CtkTextBuffer.
   * 
   * See also: 
   * ctk_text_buffer_end_user_action(),
   * ctk_text_buffer_insert_interactive(),
   * ctk_text_buffer_insert_range_interactive(),
   * ctk_text_buffer_delete_interactive(),
   * ctk_text_buffer_backspace(),
   * ctk_text_buffer_delete_selection(),
   * ctk_text_buffer_backspace().
   */ 
  signals[END_USER_ACTION] =
    g_signal_new (I_("end-user-action"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,                   
                  G_STRUCT_OFFSET (CtkTextBufferClass, end_user_action),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  0);

   /**
   * CtkTextBuffer::paste-done:
   * @textbuffer: the object which received the signal
   * @clipboard: the #CtkClipboard pasted from
   * 
   * The paste-done signal is emitted after paste operation has been completed.
   * This is useful to properly scroll the view to the end of the pasted text.
   * See ctk_text_buffer_paste_clipboard() for more details.
   * 
   * Since: 2.16
   */ 
  signals[PASTE_DONE] =
    g_signal_new (I_("paste-done"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkTextBufferClass, paste_done),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  1,
                  CTK_TYPE_CLIPBOARD);
}

static void
ctk_text_buffer_init (CtkTextBuffer *buffer)
{
  buffer->priv = ctk_text_buffer_get_instance_private (buffer);
  buffer->priv->clipboard_contents_buffers = NULL;
  buffer->priv->tag_table = NULL;

  /* allow copying of arbiatray stuff in the internal rich text format */
  ctk_text_buffer_register_serialize_tagset (buffer, NULL);
}

static void
set_table (CtkTextBuffer *buffer, CtkTextTagTable *table)
{
  CtkTextBufferPrivate *priv = buffer->priv;

  g_return_if_fail (priv->tag_table == NULL);

  if (table)
    {
      priv->tag_table = table;
      g_object_ref (priv->tag_table);
      _ctk_text_tag_table_add_buffer (table, buffer);
    }
}

static CtkTextTagTable*
get_table (CtkTextBuffer *buffer)
{
  CtkTextBufferPrivate *priv = buffer->priv;

  if (priv->tag_table == NULL)
    {
      priv->tag_table = ctk_text_tag_table_new ();
      _ctk_text_tag_table_add_buffer (priv->tag_table, buffer);
    }

  return priv->tag_table;
}

static void
ctk_text_buffer_set_property (GObject         *object,
                              guint            prop_id,
                              const GValue    *value,
                              GParamSpec      *pspec)
{
  CtkTextBuffer *text_buffer;

  text_buffer = CTK_TEXT_BUFFER (object);

  switch (prop_id)
    {
    case PROP_TAG_TABLE:
      set_table (text_buffer, g_value_get_object (value));
      break;

    case PROP_TEXT:
      ctk_text_buffer_set_text (text_buffer,
				g_value_get_string (value), -1);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_text_buffer_get_property (GObject         *object,
                              guint            prop_id,
                              GValue          *value,
                              GParamSpec      *pspec)
{
  CtkTextBuffer *text_buffer;
  CtkTextIter iter;

  text_buffer = CTK_TEXT_BUFFER (object);

  switch (prop_id)
    {
    case PROP_TAG_TABLE:
      g_value_set_object (value, get_table (text_buffer));
      break;

    case PROP_TEXT:
      {
        CtkTextIter start, end;

        ctk_text_buffer_get_start_iter (text_buffer, &start);
        ctk_text_buffer_get_end_iter (text_buffer, &end);

        g_value_take_string (value,
                            ctk_text_buffer_get_text (text_buffer,
                                                      &start, &end, FALSE));
        break;
      }

    case PROP_HAS_SELECTION:
      g_value_set_boolean (value, text_buffer->priv->has_selection);
      break;

    case PROP_CURSOR_POSITION:
      ctk_text_buffer_get_iter_at_mark (text_buffer, &iter, 
    				        ctk_text_buffer_get_insert (text_buffer));
      g_value_set_int (value, ctk_text_iter_get_offset (&iter));
      break;

    case PROP_COPY_TARGET_LIST:
      g_value_set_boxed (value, ctk_text_buffer_get_copy_target_list (text_buffer));
      break;

    case PROP_PASTE_TARGET_LIST:
      g_value_set_boxed (value, ctk_text_buffer_get_paste_target_list (text_buffer));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_text_buffer_notify (GObject    *object,
                        GParamSpec *pspec)
{
  if (!strcmp (pspec->name, "copy-target-list") ||
      !strcmp (pspec->name, "paste-target-list"))
    {
      ctk_text_buffer_free_target_lists (CTK_TEXT_BUFFER (object));
    }
}

/**
 * ctk_text_buffer_new:
 * @table: (allow-none): a tag table, or %NULL to create a new one
 *
 * Creates a new text buffer.
 *
 * Returns: a new text buffer
 **/
CtkTextBuffer*
ctk_text_buffer_new (CtkTextTagTable *table)
{
  CtkTextBuffer *text_buffer;

  text_buffer = g_object_new (CTK_TYPE_TEXT_BUFFER, "tag-table", table, NULL);

  return text_buffer;
}

static void
ctk_text_buffer_finalize (GObject *object)
{
  CtkTextBuffer *buffer;
  CtkTextBufferPrivate *priv;

  buffer = CTK_TEXT_BUFFER (object);
  priv = buffer->priv;

  remove_all_selection_clipboards (buffer);

  if (priv->tag_table)
    {
      _ctk_text_tag_table_remove_buffer (priv->tag_table, buffer);
      g_object_unref (priv->tag_table);
      priv->tag_table = NULL;
    }

  if (priv->btree)
    {
      _ctk_text_btree_unref (priv->btree);
      priv->btree = NULL;
    }

  if (priv->log_attr_cache)
    free_log_attr_cache (priv->log_attr_cache);

  priv->log_attr_cache = NULL;

  ctk_text_buffer_free_target_lists (buffer);

  G_OBJECT_CLASS (ctk_text_buffer_parent_class)->finalize (object);
}

static CtkTextBTree*
get_btree (CtkTextBuffer *buffer)
{
  CtkTextBufferPrivate *priv = buffer->priv;

  if (priv->btree == NULL)
    priv->btree = _ctk_text_btree_new (ctk_text_buffer_get_tag_table (buffer),
                                       buffer);

  return priv->btree;
}

CtkTextBTree*
_ctk_text_buffer_get_btree (CtkTextBuffer *buffer)
{
  return get_btree (buffer);
}

/**
 * ctk_text_buffer_get_tag_table:
 * @buffer: a #CtkTextBuffer
 *
 * Get the #CtkTextTagTable associated with this buffer.
 *
 * Returns: (transfer none): the buffer’s tag table
 **/
CtkTextTagTable*
ctk_text_buffer_get_tag_table (CtkTextBuffer *buffer)
{
  g_return_val_if_fail (CTK_IS_TEXT_BUFFER (buffer), NULL);

  return get_table (buffer);
}

/**
 * ctk_text_buffer_set_text:
 * @buffer: a #CtkTextBuffer
 * @text: UTF-8 text to insert
 * @len: length of @text in bytes
 *
 * Deletes current contents of @buffer, and inserts @text instead. If
 * @len is -1, @text must be nul-terminated. @text must be valid UTF-8.
 **/
void
ctk_text_buffer_set_text (CtkTextBuffer *buffer,
                          const gchar   *text,
                          gint           len)
{
  CtkTextIter start, end;

  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (text != NULL);

  if (len < 0)
    len = strlen (text);

  ctk_text_buffer_get_bounds (buffer, &start, &end);

  ctk_text_buffer_delete (buffer, &start, &end);

  if (len > 0)
    {
      ctk_text_buffer_get_iter_at_offset (buffer, &start, 0);
      ctk_text_buffer_insert (buffer, &start, text, len);
    }
}

 

/*
 * Insertion
 */

static void
ctk_text_buffer_real_insert_text (CtkTextBuffer *buffer,
                                  CtkTextIter   *iter,
                                  const gchar   *text,
                                  gint           len)
{
  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (iter != NULL);
  
  _ctk_text_btree_insert (iter, text, len);

  g_signal_emit (buffer, signals[CHANGED], 0);
  g_object_notify_by_pspec (G_OBJECT (buffer), text_buffer_props[PROP_CURSOR_POSITION]);
}

static void
ctk_text_buffer_emit_insert (CtkTextBuffer *buffer,
                             CtkTextIter   *iter,
                             const gchar   *text,
                             gint           len)
{
  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (text != NULL);

  if (len < 0)
    len = strlen (text);

  g_return_if_fail (g_utf8_validate (text, len, NULL));
  
  if (len > 0)
    {
      g_signal_emit (buffer, signals[INSERT_TEXT], 0,
                     iter, text, len);
    }
}

/**
 * ctk_text_buffer_insert:
 * @buffer: a #CtkTextBuffer
 * @iter: a position in the buffer
 * @text: text in UTF-8 format
 * @len: length of text in bytes, or -1
 *
 * Inserts @len bytes of @text at position @iter.  If @len is -1,
 * @text must be nul-terminated and will be inserted in its
 * entirety. Emits the “insert-text” signal; insertion actually occurs
 * in the default handler for the signal. @iter is invalidated when
 * insertion occurs (because the buffer contents change), but the
 * default signal handler revalidates it to point to the end of the
 * inserted text.
 **/
void
ctk_text_buffer_insert (CtkTextBuffer *buffer,
                        CtkTextIter   *iter,
                        const gchar   *text,
                        gint           len)
{
  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (text != NULL);
  g_return_if_fail (ctk_text_iter_get_buffer (iter) == buffer);
  
  ctk_text_buffer_emit_insert (buffer, iter, text, len);
}

/**
 * ctk_text_buffer_insert_at_cursor:
 * @buffer: a #CtkTextBuffer
 * @text: text in UTF-8 format
 * @len: length of text, in bytes
 *
 * Simply calls ctk_text_buffer_insert(), using the current
 * cursor position as the insertion point.
 **/
void
ctk_text_buffer_insert_at_cursor (CtkTextBuffer *buffer,
                                  const gchar   *text,
                                  gint           len)
{
  CtkTextIter iter;

  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (text != NULL);

  ctk_text_buffer_get_iter_at_mark (buffer, &iter,
                                    ctk_text_buffer_get_insert (buffer));

  ctk_text_buffer_insert (buffer, &iter, text, len);
}

/**
 * ctk_text_buffer_insert_interactive:
 * @buffer: a #CtkTextBuffer
 * @iter: a position in @buffer
 * @text: some UTF-8 text
 * @len: length of text in bytes, or -1
 * @default_editable: default editability of buffer
 *
 * Like ctk_text_buffer_insert(), but the insertion will not occur if
 * @iter is at a non-editable location in the buffer. Usually you
 * want to prevent insertions at ineditable locations if the insertion
 * results from a user action (is interactive).
 *
 * @default_editable indicates the editability of text that doesn't
 * have a tag affecting editability applied to it. Typically the
 * result of ctk_text_view_get_editable() is appropriate here.
 *
 * Returns: whether text was actually inserted
 **/
gboolean
ctk_text_buffer_insert_interactive (CtkTextBuffer *buffer,
                                    CtkTextIter   *iter,
                                    const gchar   *text,
                                    gint           len,
                                    gboolean       default_editable)
{
  g_return_val_if_fail (CTK_IS_TEXT_BUFFER (buffer), FALSE);
  g_return_val_if_fail (text != NULL, FALSE);
  g_return_val_if_fail (ctk_text_iter_get_buffer (iter) == buffer, FALSE);

  if (ctk_text_iter_can_insert (iter, default_editable))
    {
      ctk_text_buffer_begin_user_action (buffer);
      ctk_text_buffer_emit_insert (buffer, iter, text, len);
      ctk_text_buffer_end_user_action (buffer);
      return TRUE;
    }
  else
    return FALSE;
}

/**
 * ctk_text_buffer_insert_interactive_at_cursor:
 * @buffer: a #CtkTextBuffer
 * @text: text in UTF-8 format
 * @len: length of text in bytes, or -1
 * @default_editable: default editability of buffer
 *
 * Calls ctk_text_buffer_insert_interactive() at the cursor
 * position.
 *
 * @default_editable indicates the editability of text that doesn't
 * have a tag affecting editability applied to it. Typically the
 * result of ctk_text_view_get_editable() is appropriate here.
 * 
 * Returns: whether text was actually inserted
 **/
gboolean
ctk_text_buffer_insert_interactive_at_cursor (CtkTextBuffer *buffer,
                                              const gchar   *text,
                                              gint           len,
                                              gboolean       default_editable)
{
  CtkTextIter iter;

  g_return_val_if_fail (CTK_IS_TEXT_BUFFER (buffer), FALSE);
  g_return_val_if_fail (text != NULL, FALSE);

  ctk_text_buffer_get_iter_at_mark (buffer, &iter,
                                    ctk_text_buffer_get_insert (buffer));

  return ctk_text_buffer_insert_interactive (buffer, &iter, text, len,
                                             default_editable);
}

static gboolean
possibly_not_text (gunichar ch,
                   gpointer user_data)
{
  return ch == CTK_TEXT_UNKNOWN_CHAR;
}

static void
insert_text_range (CtkTextBuffer     *buffer,
                   CtkTextIter       *iter,
                   const CtkTextIter *orig_start,
                   const CtkTextIter *orig_end,
                   gboolean           interactive)
{
  gchar *text;

  text = ctk_text_iter_get_text (orig_start, orig_end);

  ctk_text_buffer_emit_insert (buffer, iter, text, -1);

  g_free (text);
}

typedef struct _Range Range;
struct _Range
{
  CtkTextBuffer *buffer;
  CtkTextMark *start_mark;
  CtkTextMark *end_mark;
  CtkTextMark *whole_end_mark;
  CtkTextIter *range_start;
  CtkTextIter *range_end;
  CtkTextIter *whole_end;
};

static Range*
save_range (CtkTextIter *range_start,
            CtkTextIter *range_end,
            CtkTextIter *whole_end)
{
  Range *r;

  r = g_slice_new (Range);

  r->buffer = ctk_text_iter_get_buffer (range_start);
  g_object_ref (r->buffer);
  
  r->start_mark = 
    ctk_text_buffer_create_mark (ctk_text_iter_get_buffer (range_start),
                                 NULL,
                                 range_start,
                                 FALSE);
  r->end_mark = 
    ctk_text_buffer_create_mark (ctk_text_iter_get_buffer (range_start),
                                 NULL,
                                 range_end,
                                 TRUE);

  r->whole_end_mark = 
    ctk_text_buffer_create_mark (ctk_text_iter_get_buffer (range_start),
                                 NULL,
                                 whole_end,
                                 TRUE);

  r->range_start = range_start;
  r->range_end = range_end;
  r->whole_end = whole_end;

  return r;
}

static void
restore_range (Range *r)
{
  ctk_text_buffer_get_iter_at_mark (r->buffer,
                                    r->range_start,
                                    r->start_mark);
      
  ctk_text_buffer_get_iter_at_mark (r->buffer,
                                    r->range_end,
                                    r->end_mark);
      
  ctk_text_buffer_get_iter_at_mark (r->buffer,
                                    r->whole_end,
                                    r->whole_end_mark);  
  
  ctk_text_buffer_delete_mark (r->buffer, r->start_mark);
  ctk_text_buffer_delete_mark (r->buffer, r->end_mark);
  ctk_text_buffer_delete_mark (r->buffer, r->whole_end_mark);

  /* Due to the gravities on the marks, the ordering could have
   * gotten mangled; we switch to an empty range in that
   * case
   */
  
  if (ctk_text_iter_compare (r->range_start, r->range_end) > 0)
    *r->range_start = *r->range_end;

  if (ctk_text_iter_compare (r->range_end, r->whole_end) > 0)
    *r->range_end = *r->whole_end;
  
  g_object_unref (r->buffer);
  g_slice_free (Range, r);
}

static void
insert_range_untagged (CtkTextBuffer     *buffer,
                       CtkTextIter       *iter,
                       const CtkTextIter *orig_start,
                       const CtkTextIter *orig_end,
                       gboolean           interactive)
{
  CtkTextIter range_start;
  CtkTextIter range_end;
  CtkTextIter start, end;
  Range *r;
  
  if (ctk_text_iter_equal (orig_start, orig_end))
    return;

  start = *orig_start;
  end = *orig_end;
  
  range_start = start;
  range_end = start;
  
  while (TRUE)
    {
      if (ctk_text_iter_equal (&range_start, &range_end))
        {
          /* Figure out how to move forward */

          g_assert (ctk_text_iter_compare (&range_end, &end) <= 0);
          
          if (ctk_text_iter_equal (&range_end, &end))
            {
              /* nothing left to do */
              break;
            }
          else if (ctk_text_iter_get_char (&range_end) == CTK_TEXT_UNKNOWN_CHAR)
            {
              GdkPixbuf *pixbuf = NULL;
              CtkTextChildAnchor *anchor = NULL;
              pixbuf = ctk_text_iter_get_pixbuf (&range_end);
              anchor = ctk_text_iter_get_child_anchor (&range_end);

              if (pixbuf)
                {
                  r = save_range (&range_start,
                                  &range_end,
                                  &end);

                  ctk_text_buffer_insert_pixbuf (buffer,
                                                 iter,
                                                 pixbuf);

                  restore_range (r);
                  r = NULL;
                  
                  ctk_text_iter_forward_char (&range_end);
                  
                  range_start = range_end;
                }
              else if (anchor)
                {
                  /* Just skip anchors */

                  ctk_text_iter_forward_char (&range_end);
                  range_start = range_end;
                }
              else
                {
                  /* The CTK_TEXT_UNKNOWN_CHAR was in a text segment, so
                   * keep going. 
                   */
                  ctk_text_iter_forward_find_char (&range_end,
                                                   possibly_not_text, NULL,
                                                   &end);
                  
                  g_assert (ctk_text_iter_compare (&range_end, &end) <= 0);
                }
            }
          else
            {
              /* Text segment starts here, so forward search to
               * find its possible endpoint
               */
              ctk_text_iter_forward_find_char (&range_end,
                                               possibly_not_text, NULL,
                                               &end);
              
              g_assert (ctk_text_iter_compare (&range_end, &end) <= 0);
            }
        }
      else
        {
          r = save_range (&range_start,
                          &range_end,
                          &end);
          
          insert_text_range (buffer,
                             iter,
                             &range_start,
                             &range_end,
                             interactive);

          restore_range (r);
          r = NULL;
          
          range_start = range_end;
        }
    }
}

static void
insert_range_not_inside_self (CtkTextBuffer     *buffer,
                              CtkTextIter       *iter,
                              const CtkTextIter *orig_start,
                              const CtkTextIter *orig_end,
                              gboolean           interactive)
{
  /* Find each range of uniformly-tagged text, insert it,
   * then apply the tags.
   */
  CtkTextIter start = *orig_start;
  CtkTextIter end = *orig_end;
  CtkTextIter range_start;
  CtkTextIter range_end;
  
  if (ctk_text_iter_equal (orig_start, orig_end))
    return;
  
  ctk_text_iter_order (&start, &end);

  range_start = start;
  range_end = start;  
  
  while (TRUE)
    {
      gint start_offset;
      CtkTextIter start_iter;
      GSList *tags;
      GSList *tmp_list;
      Range *r;
      
      if (ctk_text_iter_equal (&range_start, &end))
        break; /* All done */

      g_assert (ctk_text_iter_compare (&range_start, &end) < 0);
      
      ctk_text_iter_forward_to_tag_toggle (&range_end, NULL);

      g_assert (!ctk_text_iter_equal (&range_start, &range_end));

      /* Clamp to the end iterator */
      if (ctk_text_iter_compare (&range_end, &end) > 0)
        range_end = end;
      
      /* We have a range with unique tags; insert it, and
       * apply all tags.
       */
      start_offset = ctk_text_iter_get_offset (iter);

      r = save_range (&range_start, &range_end, &end);
      
      insert_range_untagged (buffer, iter, &range_start, &range_end, interactive);

      restore_range (r);
      r = NULL;
      
      ctk_text_buffer_get_iter_at_offset (buffer, &start_iter, start_offset);
      
      tags = ctk_text_iter_get_tags (&range_start);
      tmp_list = tags;
      while (tmp_list != NULL)
        {
          ctk_text_buffer_apply_tag (buffer,
                                     tmp_list->data,
                                     &start_iter,
                                     iter);

          tmp_list = tmp_list->next;
        }
      g_slist_free (tags);

      range_start = range_end;
    }
}

static void
ctk_text_buffer_real_insert_range (CtkTextBuffer     *buffer,
                                   CtkTextIter       *iter,
                                   const CtkTextIter *orig_start,
                                   const CtkTextIter *orig_end,
                                   gboolean           interactive)
{
  CtkTextBuffer *src_buffer;
  
  /* Find each range of uniformly-tagged text, insert it,
   * then apply the tags.
   */  
  if (ctk_text_iter_equal (orig_start, orig_end))
    return;

  if (interactive)
    ctk_text_buffer_begin_user_action (buffer);
  
  src_buffer = ctk_text_iter_get_buffer (orig_start);
  
  if (ctk_text_iter_get_buffer (iter) != src_buffer ||
      !ctk_text_iter_in_range (iter, orig_start, orig_end))
    {
      insert_range_not_inside_self (buffer, iter, orig_start, orig_end, interactive);
    }
  else
    {
      /* If you insert a range into itself, it could loop infinitely
       * because the region being copied keeps growing as we insert. So
       * we have to separately copy the range before and after
       * the insertion point.
       */
      CtkTextIter start = *orig_start;
      CtkTextIter end = *orig_end;
      CtkTextIter range_start;
      CtkTextIter range_end;
      Range *first_half;
      Range *second_half;

      ctk_text_iter_order (&start, &end);
      
      range_start = start;
      range_end = *iter;
      first_half = save_range (&range_start, &range_end, &end);

      range_start = *iter;
      range_end = end;
      second_half = save_range (&range_start, &range_end, &end);

      restore_range (first_half);
      insert_range_not_inside_self (buffer, iter, &range_start, &range_end, interactive);

      restore_range (second_half);
      insert_range_not_inside_self (buffer, iter, &range_start, &range_end, interactive);
    }
  
  if (interactive)
    ctk_text_buffer_end_user_action (buffer);
}

/**
 * ctk_text_buffer_insert_range:
 * @buffer: a #CtkTextBuffer
 * @iter: a position in @buffer
 * @start: a position in a #CtkTextBuffer
 * @end: another position in the same buffer as @start
 *
 * Copies text, tags, and pixbufs between @start and @end (the order
 * of @start and @end doesn’t matter) and inserts the copy at @iter.
 * Used instead of simply getting/inserting text because it preserves
 * images and tags. If @start and @end are in a different buffer from
 * @buffer, the two buffers must share the same tag table.
 *
 * Implemented via emissions of the insert_text and apply_tag signals,
 * so expect those.
 **/
void
ctk_text_buffer_insert_range (CtkTextBuffer     *buffer,
                              CtkTextIter       *iter,
                              const CtkTextIter *start,
                              const CtkTextIter *end)
{
  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (start != NULL);
  g_return_if_fail (end != NULL);
  g_return_if_fail (ctk_text_iter_get_buffer (start) ==
                    ctk_text_iter_get_buffer (end));
  g_return_if_fail (ctk_text_iter_get_buffer (start)->priv->tag_table ==
                    buffer->priv->tag_table);
  g_return_if_fail (ctk_text_iter_get_buffer (iter) == buffer);
  
  ctk_text_buffer_real_insert_range (buffer, iter, start, end, FALSE);
}

/**
 * ctk_text_buffer_insert_range_interactive:
 * @buffer: a #CtkTextBuffer
 * @iter: a position in @buffer
 * @start: a position in a #CtkTextBuffer
 * @end: another position in the same buffer as @start
 * @default_editable: default editability of the buffer
 *
 * Same as ctk_text_buffer_insert_range(), but does nothing if the
 * insertion point isn’t editable. The @default_editable parameter
 * indicates whether the text is editable at @iter if no tags
 * enclosing @iter affect editability. Typically the result of
 * ctk_text_view_get_editable() is appropriate here.
 *
 * Returns: whether an insertion was possible at @iter
 **/
gboolean
ctk_text_buffer_insert_range_interactive (CtkTextBuffer     *buffer,
                                          CtkTextIter       *iter,
                                          const CtkTextIter *start,
                                          const CtkTextIter *end,
                                          gboolean           default_editable)
{
  g_return_val_if_fail (CTK_IS_TEXT_BUFFER (buffer), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);
  g_return_val_if_fail (start != NULL, FALSE);
  g_return_val_if_fail (end != NULL, FALSE);
  g_return_val_if_fail (ctk_text_iter_get_buffer (start) ==
                        ctk_text_iter_get_buffer (end), FALSE);
  g_return_val_if_fail (ctk_text_iter_get_buffer (start)->priv->tag_table ==
                        buffer->priv->tag_table, FALSE);

  if (ctk_text_iter_can_insert (iter, default_editable))
    {
      ctk_text_buffer_real_insert_range (buffer, iter, start, end, TRUE);
      return TRUE;
    }
  else
    return FALSE;
}

/**
 * ctk_text_buffer_insert_with_tags:
 * @buffer: a #CtkTextBuffer
 * @iter: an iterator in @buffer
 * @text: UTF-8 text
 * @len: length of @text, or -1
 * @first_tag: first tag to apply to @text
 * @...: %NULL-terminated list of tags to apply
 *
 * Inserts @text into @buffer at @iter, applying the list of tags to
 * the newly-inserted text. The last tag specified must be %NULL to
 * terminate the list. Equivalent to calling ctk_text_buffer_insert(),
 * then ctk_text_buffer_apply_tag() on the inserted text;
 * ctk_text_buffer_insert_with_tags() is just a convenience function.
 **/
void
ctk_text_buffer_insert_with_tags (CtkTextBuffer *buffer,
                                  CtkTextIter   *iter,
                                  const gchar   *text,
                                  gint           len,
                                  CtkTextTag    *first_tag,
                                  ...)
{
  gint start_offset;
  CtkTextIter start;
  va_list args;
  CtkTextTag *tag;

  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (text != NULL);
  g_return_if_fail (ctk_text_iter_get_buffer (iter) == buffer);
  
  start_offset = ctk_text_iter_get_offset (iter);

  ctk_text_buffer_insert (buffer, iter, text, len);

  if (first_tag == NULL)
    return;

  ctk_text_buffer_get_iter_at_offset (buffer, &start, start_offset);

  va_start (args, first_tag);
  tag = first_tag;
  while (tag)
    {
      ctk_text_buffer_apply_tag (buffer, tag, &start, iter);

      tag = va_arg (args, CtkTextTag*);
    }

  va_end (args);
}

/**
 * ctk_text_buffer_insert_with_tags_by_name:
 * @buffer: a #CtkTextBuffer
 * @iter: position in @buffer
 * @text: UTF-8 text
 * @len: length of @text, or -1
 * @first_tag_name: name of a tag to apply to @text
 * @...: more tag names
 *
 * Same as ctk_text_buffer_insert_with_tags(), but allows you
 * to pass in tag names instead of tag objects.
 **/
void
ctk_text_buffer_insert_with_tags_by_name  (CtkTextBuffer *buffer,
                                           CtkTextIter   *iter,
                                           const gchar   *text,
                                           gint           len,
                                           const gchar   *first_tag_name,
                                           ...)
{
  gint start_offset;
  CtkTextIter start;
  va_list args;
  const gchar *tag_name;

  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (text != NULL);
  g_return_if_fail (ctk_text_iter_get_buffer (iter) == buffer);
  
  start_offset = ctk_text_iter_get_offset (iter);

  ctk_text_buffer_insert (buffer, iter, text, len);

  if (first_tag_name == NULL)
    return;

  ctk_text_buffer_get_iter_at_offset (buffer, &start, start_offset);

  va_start (args, first_tag_name);
  tag_name = first_tag_name;
  while (tag_name)
    {
      CtkTextTag *tag;

      tag = ctk_text_tag_table_lookup (buffer->priv->tag_table,
                                       tag_name);

      if (tag == NULL)
        {
          g_warning ("%s: no tag with name '%s'!", G_STRLOC, tag_name);
          va_end (args);
          return;
        }

      ctk_text_buffer_apply_tag (buffer, tag, &start, iter);

      tag_name = va_arg (args, const gchar*);
    }

  va_end (args);
}


/*
 * Deletion
 */

static void
ctk_text_buffer_real_delete_range (CtkTextBuffer *buffer,
                                   CtkTextIter   *start,
                                   CtkTextIter   *end)
{
  gboolean has_selection;

  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (start != NULL);
  g_return_if_fail (end != NULL);

  _ctk_text_btree_delete (start, end);

  /* may have deleted the selection... */
  update_selection_clipboards (buffer);

  has_selection = ctk_text_buffer_get_selection_bounds (buffer, NULL, NULL);
  if (has_selection != buffer->priv->has_selection)
    {
      buffer->priv->has_selection = has_selection;
      g_object_notify_by_pspec (G_OBJECT (buffer), text_buffer_props[PROP_HAS_SELECTION]);
    }

  g_signal_emit (buffer, signals[CHANGED], 0);
  g_object_notify_by_pspec (G_OBJECT (buffer), text_buffer_props[PROP_CURSOR_POSITION]);
}

static void
ctk_text_buffer_emit_delete (CtkTextBuffer *buffer,
                             CtkTextIter *start,
                             CtkTextIter *end)
{
  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (start != NULL);
  g_return_if_fail (end != NULL);

  if (ctk_text_iter_equal (start, end))
    return;

  ctk_text_iter_order (start, end);

  g_signal_emit (buffer,
                 signals[DELETE_RANGE],
                 0,
                 start, end);
}

/**
 * ctk_text_buffer_delete:
 * @buffer: a #CtkTextBuffer
 * @start: a position in @buffer
 * @end: another position in @buffer
 *
 * Deletes text between @start and @end. The order of @start and @end
 * is not actually relevant; ctk_text_buffer_delete() will reorder
 * them. This function actually emits the “delete-range” signal, and
 * the default handler of that signal deletes the text. Because the
 * buffer is modified, all outstanding iterators become invalid after
 * calling this function; however, the @start and @end will be
 * re-initialized to point to the location where text was deleted.
 **/
void
ctk_text_buffer_delete (CtkTextBuffer *buffer,
                        CtkTextIter   *start,
                        CtkTextIter   *end)
{
  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (start != NULL);
  g_return_if_fail (end != NULL);
  g_return_if_fail (ctk_text_iter_get_buffer (start) == buffer);
  g_return_if_fail (ctk_text_iter_get_buffer (end) == buffer);
  
  ctk_text_buffer_emit_delete (buffer, start, end);
}

/**
 * ctk_text_buffer_delete_interactive:
 * @buffer: a #CtkTextBuffer
 * @start_iter: start of range to delete
 * @end_iter: end of range
 * @default_editable: whether the buffer is editable by default
 *
 * Deletes all editable text in the given range.
 * Calls ctk_text_buffer_delete() for each editable sub-range of
 * [@start,@end). @start and @end are revalidated to point to
 * the location of the last deleted range, or left untouched if
 * no text was deleted.
 *
 * Returns: whether some text was actually deleted
 **/
gboolean
ctk_text_buffer_delete_interactive (CtkTextBuffer *buffer,
                                    CtkTextIter   *start_iter,
                                    CtkTextIter   *end_iter,
                                    gboolean       default_editable)
{
  CtkTextMark *end_mark;
  CtkTextMark *start_mark;
  CtkTextIter iter;
  gboolean current_state;
  gboolean deleted_stuff = FALSE;

  /* Delete all editable text in the range start_iter, end_iter */

  g_return_val_if_fail (CTK_IS_TEXT_BUFFER (buffer), FALSE);
  g_return_val_if_fail (start_iter != NULL, FALSE);
  g_return_val_if_fail (end_iter != NULL, FALSE);
  g_return_val_if_fail (ctk_text_iter_get_buffer (start_iter) == buffer, FALSE);
  g_return_val_if_fail (ctk_text_iter_get_buffer (end_iter) == buffer, FALSE);

  
  ctk_text_buffer_begin_user_action (buffer);
  
  ctk_text_iter_order (start_iter, end_iter);

  start_mark = ctk_text_buffer_create_mark (buffer, NULL,
                                            start_iter, TRUE);
  end_mark = ctk_text_buffer_create_mark (buffer, NULL,
                                          end_iter, FALSE);

  ctk_text_buffer_get_iter_at_mark (buffer, &iter, start_mark);

  current_state = ctk_text_iter_editable (&iter, default_editable);

  while (TRUE)
    {
      gboolean new_state;
      gboolean done = FALSE;
      CtkTextIter end;

      ctk_text_iter_forward_to_tag_toggle (&iter, NULL);

      ctk_text_buffer_get_iter_at_mark (buffer, &end, end_mark);

      if (ctk_text_iter_compare (&iter, &end) >= 0)
        {
          done = TRUE;
          iter = end; /* clamp to the last boundary */
        }

      new_state = ctk_text_iter_editable (&iter, default_editable);

      if (current_state == new_state)
        {
          if (done)
            {
              if (current_state)
                {
                  /* We're ending an editable region. Delete said region. */
                  CtkTextIter start;

                  ctk_text_buffer_get_iter_at_mark (buffer, &start, start_mark);

                  ctk_text_buffer_emit_delete (buffer, &start, &iter);

                  deleted_stuff = TRUE;

                  /* revalidate user's iterators. */
                  *start_iter = start;
                  *end_iter = iter;
                }

              break;
            }
          else
            continue;
        }

      if (current_state && !new_state)
        {
          /* End of an editable region. Delete it. */
          CtkTextIter start;

          ctk_text_buffer_get_iter_at_mark (buffer, &start, start_mark);

          ctk_text_buffer_emit_delete (buffer, &start, &iter);

	  /* It's more robust to ask for the state again then to assume that
	   * we're on the next not-editable segment. We don't know what the
	   * ::delete-range handler did.... maybe it deleted the following
           * not-editable segment because it was associated with the editable
           * segment.
	   */
	  current_state = ctk_text_iter_editable (&iter, default_editable);
          deleted_stuff = TRUE;

          /* revalidate user's iterators. */
          *start_iter = start;
          *end_iter = iter;
        }
      else
        {
          /* We are at the start of an editable region. We won't be deleting
           * the previous region. Move start mark to start of this region.
           */

          g_assert (!current_state && new_state);

          ctk_text_buffer_move_mark (buffer, start_mark, &iter);

          current_state = TRUE;
        }

      if (done)
        break;
    }

  ctk_text_buffer_delete_mark (buffer, start_mark);
  ctk_text_buffer_delete_mark (buffer, end_mark);

  ctk_text_buffer_end_user_action (buffer);
  
  return deleted_stuff;
}

/*
 * Extracting textual buffer contents
 */

/**
 * ctk_text_buffer_get_text:
 * @buffer: a #CtkTextBuffer
 * @start: start of a range
 * @end: end of a range
 * @include_hidden_chars: whether to include invisible text
 *
 * Returns the text in the range [@start,@end). Excludes undisplayed
 * text (text marked with tags that set the invisibility attribute) if
 * @include_hidden_chars is %FALSE. Does not include characters
 * representing embedded images, so byte and character indexes into
 * the returned string do not correspond to byte
 * and character indexes into the buffer. Contrast with
 * ctk_text_buffer_get_slice().
 *
 * Returns: (transfer full): an allocated UTF-8 string
 **/
gchar*
ctk_text_buffer_get_text (CtkTextBuffer     *buffer,
                          const CtkTextIter *start,
                          const CtkTextIter *end,
                          gboolean           include_hidden_chars)
{
  g_return_val_if_fail (CTK_IS_TEXT_BUFFER (buffer), NULL);
  g_return_val_if_fail (start != NULL, NULL);
  g_return_val_if_fail (end != NULL, NULL);
  g_return_val_if_fail (ctk_text_iter_get_buffer (start) == buffer, NULL);
  g_return_val_if_fail (ctk_text_iter_get_buffer (end) == buffer, NULL);
  
  if (include_hidden_chars)
    return ctk_text_iter_get_text (start, end);
  else
    return ctk_text_iter_get_visible_text (start, end);
}

/**
 * ctk_text_buffer_get_slice:
 * @buffer: a #CtkTextBuffer
 * @start: start of a range
 * @end: end of a range
 * @include_hidden_chars: whether to include invisible text
 *
 * Returns the text in the range [@start,@end). Excludes undisplayed
 * text (text marked with tags that set the invisibility attribute) if
 * @include_hidden_chars is %FALSE. The returned string includes a
 * 0xFFFC character whenever the buffer contains
 * embedded images, so byte and character indexes into
 * the returned string do correspond to byte
 * and character indexes into the buffer. Contrast with
 * ctk_text_buffer_get_text(). Note that 0xFFFC can occur in normal
 * text as well, so it is not a reliable indicator that a pixbuf or
 * widget is in the buffer.
 *
 * Returns: (transfer full): an allocated UTF-8 string
 **/
gchar*
ctk_text_buffer_get_slice (CtkTextBuffer     *buffer,
                           const CtkTextIter *start,
                           const CtkTextIter *end,
                           gboolean           include_hidden_chars)
{
  g_return_val_if_fail (CTK_IS_TEXT_BUFFER (buffer), NULL);
  g_return_val_if_fail (start != NULL, NULL);
  g_return_val_if_fail (end != NULL, NULL);
  g_return_val_if_fail (ctk_text_iter_get_buffer (start) == buffer, NULL);
  g_return_val_if_fail (ctk_text_iter_get_buffer (end) == buffer, NULL);
  
  if (include_hidden_chars)
    return ctk_text_iter_get_slice (start, end);
  else
    return ctk_text_iter_get_visible_slice (start, end);
}

/*
 * Pixbufs
 */

static void
ctk_text_buffer_real_insert_pixbuf (CtkTextBuffer *buffer,
                                    CtkTextIter   *iter,
                                    GdkPixbuf     *pixbuf)
{ 
  _ctk_text_btree_insert_pixbuf (iter, pixbuf);

  g_signal_emit (buffer, signals[CHANGED], 0);
}

/**
 * ctk_text_buffer_insert_pixbuf:
 * @buffer: a #CtkTextBuffer
 * @iter: location to insert the pixbuf
 * @pixbuf: a #GdkPixbuf
 *
 * Inserts an image into the text buffer at @iter. The image will be
 * counted as one character in character counts, and when obtaining
 * the buffer contents as a string, will be represented by the Unicode
 * “object replacement character” 0xFFFC. Note that the “slice”
 * variants for obtaining portions of the buffer as a string include
 * this character for pixbufs, but the “text” variants do
 * not. e.g. see ctk_text_buffer_get_slice() and
 * ctk_text_buffer_get_text().
 **/
void
ctk_text_buffer_insert_pixbuf (CtkTextBuffer *buffer,
                               CtkTextIter   *iter,
                               GdkPixbuf     *pixbuf)
{
  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (GDK_IS_PIXBUF (pixbuf));
  g_return_if_fail (ctk_text_iter_get_buffer (iter) == buffer);
  
  g_signal_emit (buffer, signals[INSERT_PIXBUF], 0,
                 iter, pixbuf);
}

/*
 * Child anchor
 */


static void
ctk_text_buffer_real_insert_anchor (CtkTextBuffer      *buffer,
                                    CtkTextIter        *iter,
                                    CtkTextChildAnchor *anchor)
{
  _ctk_text_btree_insert_child_anchor (iter, anchor);

  g_signal_emit (buffer, signals[CHANGED], 0);
}

/**
 * ctk_text_buffer_insert_child_anchor:
 * @buffer: a #CtkTextBuffer
 * @iter: location to insert the anchor
 * @anchor: a #CtkTextChildAnchor
 *
 * Inserts a child widget anchor into the text buffer at @iter. The
 * anchor will be counted as one character in character counts, and
 * when obtaining the buffer contents as a string, will be represented
 * by the Unicode “object replacement character” 0xFFFC. Note that the
 * “slice” variants for obtaining portions of the buffer as a string
 * include this character for child anchors, but the “text” variants do
 * not. E.g. see ctk_text_buffer_get_slice() and
 * ctk_text_buffer_get_text(). Consider
 * ctk_text_buffer_create_child_anchor() as a more convenient
 * alternative to this function. The buffer will add a reference to
 * the anchor, so you can unref it after insertion.
 **/
void
ctk_text_buffer_insert_child_anchor (CtkTextBuffer      *buffer,
                                     CtkTextIter        *iter,
                                     CtkTextChildAnchor *anchor)
{
  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (CTK_IS_TEXT_CHILD_ANCHOR (anchor));
  g_return_if_fail (ctk_text_iter_get_buffer (iter) == buffer);
  
  g_signal_emit (buffer, signals[INSERT_CHILD_ANCHOR], 0,
                 iter, anchor);
}

/**
 * ctk_text_buffer_create_child_anchor:
 * @buffer: a #CtkTextBuffer
 * @iter: location in the buffer
 * 
 * This is a convenience function which simply creates a child anchor
 * with ctk_text_child_anchor_new() and inserts it into the buffer
 * with ctk_text_buffer_insert_child_anchor(). The new anchor is
 * owned by the buffer; no reference count is returned to
 * the caller of ctk_text_buffer_create_child_anchor().
 * 
 * Returns: (transfer none): the created child anchor
 **/
CtkTextChildAnchor*
ctk_text_buffer_create_child_anchor (CtkTextBuffer *buffer,
                                     CtkTextIter   *iter)
{
  CtkTextChildAnchor *anchor;
  
  g_return_val_if_fail (CTK_IS_TEXT_BUFFER (buffer), NULL);
  g_return_val_if_fail (iter != NULL, NULL);
  g_return_val_if_fail (ctk_text_iter_get_buffer (iter) == buffer, NULL);
  
  anchor = ctk_text_child_anchor_new ();

  ctk_text_buffer_insert_child_anchor (buffer, iter, anchor);

  g_object_unref (anchor);

  return anchor;
}

/*
 * Mark manipulation
 */

static void
ctk_text_buffer_mark_set (CtkTextBuffer     *buffer,
                          const CtkTextIter *location,
                          CtkTextMark       *mark)
{
  /* IMO this should NOT work like insert_text and delete_range,
   * where the real action happens in the default handler.
   * 
   * The reason is that the default handler would be _required_,
   * i.e. the whole widget would start breaking and segfaulting if the
   * default handler didn't get run. So you can't really override the
   * default handler or stop the emission; that is, this signal is
   * purely for notification, and not to allow users to modify the
   * default behavior.
   */

  g_object_ref (mark);

  g_signal_emit (buffer,
                 signals[MARK_SET],
                 0,
                 location,
                 mark);

  g_object_unref (mark);
}

/**
 * ctk_text_buffer_set_mark:
 * @buffer:       a #CtkTextBuffer
 * @mark_name:    name of the mark
 * @iter:         location for the mark
 * @left_gravity: if the mark is created by this function, gravity for 
 *                the new mark
 * @should_exist: if %TRUE, warn if the mark does not exist, and return
 *                immediately
 *
 * Move the mark to the given position, if not @should_exist, 
 * create the mark.
 *
 * Returns: mark
 **/
static CtkTextMark*
ctk_text_buffer_set_mark (CtkTextBuffer     *buffer,
                          CtkTextMark       *existing_mark,
                          const gchar       *mark_name,
                          const CtkTextIter *iter,
                          gboolean           left_gravity,
                          gboolean           should_exist)
{
  CtkTextIter location;
  CtkTextMark *mark;

  g_return_val_if_fail (ctk_text_iter_get_buffer (iter) == buffer, NULL);
  
  mark = _ctk_text_btree_set_mark (get_btree (buffer),
                                   existing_mark,
                                   mark_name,
                                   left_gravity,
                                   iter,
                                   should_exist);
  
  _ctk_text_btree_get_iter_at_mark (get_btree (buffer),
                                   &location,
                                   mark);

  ctk_text_buffer_mark_set (buffer, &location, mark);

  return mark;
}

/**
 * ctk_text_buffer_create_mark:
 * @buffer: a #CtkTextBuffer
 * @mark_name: (allow-none): name for mark, or %NULL
 * @where: location to place mark
 * @left_gravity: whether the mark has left gravity
 *
 * Creates a mark at position @where. If @mark_name is %NULL, the mark
 * is anonymous; otherwise, the mark can be retrieved by name using
 * ctk_text_buffer_get_mark(). If a mark has left gravity, and text is
 * inserted at the mark’s current location, the mark will be moved to
 * the left of the newly-inserted text. If the mark has right gravity
 * (@left_gravity = %FALSE), the mark will end up on the right of
 * newly-inserted text. The standard left-to-right cursor is a mark
 * with right gravity (when you type, the cursor stays on the right
 * side of the text you’re typing).
 *
 * The caller of this function does not own a 
 * reference to the returned #CtkTextMark, so you can ignore the 
 * return value if you like. Marks are owned by the buffer and go 
 * away when the buffer does.
 *
 * Emits the #CtkTextBuffer::mark-set signal as notification of the mark's
 * initial placement.
 *
 * Returns: (transfer none): the new #CtkTextMark object
 **/
CtkTextMark*
ctk_text_buffer_create_mark (CtkTextBuffer     *buffer,
                             const gchar       *mark_name,
                             const CtkTextIter *where,
                             gboolean           left_gravity)
{
  g_return_val_if_fail (CTK_IS_TEXT_BUFFER (buffer), NULL);

  return ctk_text_buffer_set_mark (buffer, NULL, mark_name, where,
                                   left_gravity, FALSE);
}

/**
 * ctk_text_buffer_add_mark:
 * @buffer: a #CtkTextBuffer
 * @mark: the mark to add
 * @where: location to place mark
 *
 * Adds the mark at position @where. The mark must not be added to
 * another buffer, and if its name is not %NULL then there must not
 * be another mark in the buffer with the same name.
 *
 * Emits the #CtkTextBuffer::mark-set signal as notification of the mark's
 * initial placement.
 *
 * Since: 2.12
 **/
void
ctk_text_buffer_add_mark (CtkTextBuffer     *buffer,
                          CtkTextMark       *mark,
                          const CtkTextIter *where)
{
  const gchar *name;

  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (CTK_IS_TEXT_MARK (mark));
  g_return_if_fail (where != NULL);
  g_return_if_fail (ctk_text_mark_get_buffer (mark) == NULL);

  name = ctk_text_mark_get_name (mark);

  if (name != NULL && ctk_text_buffer_get_mark (buffer, name) != NULL)
    {
      g_critical ("Mark %s already exists in the buffer", name);
      return;
    }

  ctk_text_buffer_set_mark (buffer, mark, NULL, where, FALSE, FALSE);
}

/**
 * ctk_text_buffer_move_mark:
 * @buffer: a #CtkTextBuffer
 * @mark: a #CtkTextMark
 * @where: new location for @mark in @buffer
 *
 * Moves @mark to the new location @where. Emits the #CtkTextBuffer::mark-set
 * signal as notification of the move.
 **/
void
ctk_text_buffer_move_mark (CtkTextBuffer     *buffer,
                           CtkTextMark       *mark,
                           const CtkTextIter *where)
{
  g_return_if_fail (CTK_IS_TEXT_MARK (mark));
  g_return_if_fail (!ctk_text_mark_get_deleted (mark));
  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));

  ctk_text_buffer_set_mark (buffer, mark, NULL, where, FALSE, TRUE);
}

/**
 * ctk_text_buffer_get_iter_at_mark:
 * @buffer: a #CtkTextBuffer
 * @iter: (out): iterator to initialize
 * @mark: a #CtkTextMark in @buffer
 *
 * Initializes @iter with the current position of @mark.
 **/
void
ctk_text_buffer_get_iter_at_mark (CtkTextBuffer *buffer,
                                  CtkTextIter   *iter,
                                  CtkTextMark   *mark)
{
  g_return_if_fail (CTK_IS_TEXT_MARK (mark));
  g_return_if_fail (!ctk_text_mark_get_deleted (mark));
  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));

  _ctk_text_btree_get_iter_at_mark (get_btree (buffer),
                                    iter,
                                    mark);
}

/**
 * ctk_text_buffer_delete_mark:
 * @buffer: a #CtkTextBuffer
 * @mark: a #CtkTextMark in @buffer
 *
 * Deletes @mark, so that it’s no longer located anywhere in the
 * buffer. Removes the reference the buffer holds to the mark, so if
 * you haven’t called g_object_ref() on the mark, it will be freed. Even
 * if the mark isn’t freed, most operations on @mark become
 * invalid, until it gets added to a buffer again with 
 * ctk_text_buffer_add_mark(). Use ctk_text_mark_get_deleted() to  
 * find out if a mark has been removed from its buffer.
 * The #CtkTextBuffer::mark-deleted signal will be emitted as notification after
 * the mark is deleted.
 **/
void
ctk_text_buffer_delete_mark (CtkTextBuffer *buffer,
                             CtkTextMark   *mark)
{
  g_return_if_fail (CTK_IS_TEXT_MARK (mark));
  g_return_if_fail (!ctk_text_mark_get_deleted (mark));
  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));

  g_object_ref (mark);

  _ctk_text_btree_remove_mark (get_btree (buffer), mark);

  /* See rationale above for MARK_SET on why we emit this after
   * removing the mark, rather than removing the mark in a default
   * handler.
   */
  g_signal_emit (buffer, signals[MARK_DELETED],
                 0,
                 mark);

  g_object_unref (mark);
}

/**
 * ctk_text_buffer_get_mark:
 * @buffer: a #CtkTextBuffer
 * @name: a mark name
 *
 * Returns the mark named @name in buffer @buffer, or %NULL if no such
 * mark exists in the buffer.
 *
 * Returns: (nullable) (transfer none): a #CtkTextMark, or %NULL
 **/
CtkTextMark*
ctk_text_buffer_get_mark (CtkTextBuffer *buffer,
                          const gchar   *name)
{
  CtkTextMark *mark;

  g_return_val_if_fail (CTK_IS_TEXT_BUFFER (buffer), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  mark = _ctk_text_btree_get_mark_by_name (get_btree (buffer), name);

  return mark;
}

/**
 * ctk_text_buffer_move_mark_by_name:
 * @buffer: a #CtkTextBuffer
 * @name: name of a mark
 * @where: new location for mark
 *
 * Moves the mark named @name (which must exist) to location @where.
 * See ctk_text_buffer_move_mark() for details.
 **/
void
ctk_text_buffer_move_mark_by_name (CtkTextBuffer     *buffer,
                                   const gchar       *name,
                                   const CtkTextIter *where)
{
  CtkTextMark *mark;

  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (name != NULL);

  mark = _ctk_text_btree_get_mark_by_name (get_btree (buffer), name);

  if (mark == NULL)
    {
      g_warning ("%s: no mark named '%s'", G_STRLOC, name);
      return;
    }

  ctk_text_buffer_move_mark (buffer, mark, where);
}

/**
 * ctk_text_buffer_delete_mark_by_name:
 * @buffer: a #CtkTextBuffer
 * @name: name of a mark in @buffer
 *
 * Deletes the mark named @name; the mark must exist. See
 * ctk_text_buffer_delete_mark() for details.
 **/
void
ctk_text_buffer_delete_mark_by_name (CtkTextBuffer *buffer,
                                     const gchar   *name)
{
  CtkTextMark *mark;

  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (name != NULL);

  mark = _ctk_text_btree_get_mark_by_name (get_btree (buffer), name);

  if (mark == NULL)
    {
      g_warning ("%s: no mark named '%s'", G_STRLOC, name);
      return;
    }

  ctk_text_buffer_delete_mark (buffer, mark);
}

/**
 * ctk_text_buffer_get_insert:
 * @buffer: a #CtkTextBuffer
 *
 * Returns the mark that represents the cursor (insertion point).
 * Equivalent to calling ctk_text_buffer_get_mark() to get the mark
 * named “insert”, but very slightly more efficient, and involves less
 * typing.
 *
 * Returns: (transfer none): insertion point mark
 **/
CtkTextMark*
ctk_text_buffer_get_insert (CtkTextBuffer *buffer)
{
  g_return_val_if_fail (CTK_IS_TEXT_BUFFER (buffer), NULL);

  return _ctk_text_btree_get_insert (get_btree (buffer));
}

/**
 * ctk_text_buffer_get_selection_bound:
 * @buffer: a #CtkTextBuffer
 *
 * Returns the mark that represents the selection bound.  Equivalent
 * to calling ctk_text_buffer_get_mark() to get the mark named
 * “selection_bound”, but very slightly more efficient, and involves
 * less typing.
 *
 * The currently-selected text in @buffer is the region between the
 * “selection_bound” and “insert” marks. If “selection_bound” and
 * “insert” are in the same place, then there is no current selection.
 * ctk_text_buffer_get_selection_bounds() is another convenient function
 * for handling the selection, if you just want to know whether there’s a
 * selection and what its bounds are.
 *
 * Returns: (transfer none): selection bound mark
 **/
CtkTextMark*
ctk_text_buffer_get_selection_bound (CtkTextBuffer *buffer)
{
  g_return_val_if_fail (CTK_IS_TEXT_BUFFER (buffer), NULL);

  return _ctk_text_btree_get_selection_bound (get_btree (buffer));
}

/**
 * ctk_text_buffer_get_iter_at_child_anchor:
 * @buffer: a #CtkTextBuffer
 * @iter: (out): an iterator to be initialized
 * @anchor: a child anchor that appears in @buffer
 *
 * Obtains the location of @anchor within @buffer.
 **/
void
ctk_text_buffer_get_iter_at_child_anchor (CtkTextBuffer      *buffer,
                                          CtkTextIter        *iter,
                                          CtkTextChildAnchor *anchor)
{
  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (CTK_IS_TEXT_CHILD_ANCHOR (anchor));
  g_return_if_fail (!ctk_text_child_anchor_get_deleted (anchor));
  
  _ctk_text_btree_get_iter_at_child_anchor (get_btree (buffer),
                                           iter,
                                           anchor);
}

/**
 * ctk_text_buffer_place_cursor:
 * @buffer: a #CtkTextBuffer
 * @where: where to put the cursor
 *
 * This function moves the “insert” and “selection_bound” marks
 * simultaneously.  If you move them to the same place in two steps
 * with ctk_text_buffer_move_mark(), you will temporarily select a
 * region in between their old and new locations, which can be pretty
 * inefficient since the temporarily-selected region will force stuff
 * to be recalculated. This function moves them as a unit, which can
 * be optimized.
 **/
void
ctk_text_buffer_place_cursor (CtkTextBuffer     *buffer,
                              const CtkTextIter *where)
{
  ctk_text_buffer_select_range (buffer, where, where);
}

/**
 * ctk_text_buffer_select_range:
 * @buffer: a #CtkTextBuffer
 * @ins: where to put the “insert” mark
 * @bound: where to put the “selection_bound” mark
 *
 * This function moves the “insert” and “selection_bound” marks
 * simultaneously.  If you move them in two steps
 * with ctk_text_buffer_move_mark(), you will temporarily select a
 * region in between their old and new locations, which can be pretty
 * inefficient since the temporarily-selected region will force stuff
 * to be recalculated. This function moves them as a unit, which can
 * be optimized.
 *
 * Since: 2.4
 **/
void
ctk_text_buffer_select_range (CtkTextBuffer     *buffer,
			      const CtkTextIter *ins,
                              const CtkTextIter *bound)
{
  CtkTextIter real_ins;
  CtkTextIter real_bound;

  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));

  real_ins = *ins;
  real_bound = *bound;

  _ctk_text_btree_select_range (get_btree (buffer), &real_ins, &real_bound);
  ctk_text_buffer_mark_set (buffer, &real_ins,
                            ctk_text_buffer_get_insert (buffer));
  ctk_text_buffer_mark_set (buffer, &real_bound,
                            ctk_text_buffer_get_selection_bound (buffer));
}

/*
 * Tags
 */

/**
 * ctk_text_buffer_create_tag:
 * @buffer: a #CtkTextBuffer
 * @tag_name: (allow-none): name of the new tag, or %NULL
 * @first_property_name: (allow-none): name of first property to set, or %NULL
 * @...: %NULL-terminated list of property names and values
 *
 * Creates a tag and adds it to the tag table for @buffer.
 * Equivalent to calling ctk_text_tag_new() and then adding the
 * tag to the buffer’s tag table. The returned tag is owned by
 * the buffer’s tag table, so the ref count will be equal to one.
 *
 * If @tag_name is %NULL, the tag is anonymous.
 *
 * If @tag_name is non-%NULL, a tag called @tag_name must not already
 * exist in the tag table for this buffer.
 *
 * The @first_property_name argument and subsequent arguments are a list
 * of properties to set on the tag, as with g_object_set().
 *
 * Returns: (transfer none): a new tag
 */
CtkTextTag*
ctk_text_buffer_create_tag (CtkTextBuffer *buffer,
                            const gchar   *tag_name,
                            const gchar   *first_property_name,
                            ...)
{
  CtkTextTag *tag;
  va_list list;
  
  g_return_val_if_fail (CTK_IS_TEXT_BUFFER (buffer), NULL);

  tag = ctk_text_tag_new (tag_name);

  if (!ctk_text_tag_table_add (get_table (buffer), tag))
    {
      g_object_unref (tag);
      return NULL;
    }

  if (first_property_name)
    {
      va_start (list, first_property_name);
      g_object_set_valist (G_OBJECT (tag), first_property_name, list);
      va_end (list);
    }
  
  g_object_unref (tag);

  return tag;
}

static void
ctk_text_buffer_real_apply_tag (CtkTextBuffer     *buffer,
                                CtkTextTag        *tag,
                                const CtkTextIter *start,
                                const CtkTextIter *end)
{
  if (tag->priv->table != buffer->priv->tag_table)
    {
      g_warning ("Can only apply tags that are in the tag table for the buffer");
      return;
    }
  
  _ctk_text_btree_tag (start, end, tag, TRUE);
}

static void
ctk_text_buffer_real_remove_tag (CtkTextBuffer     *buffer,
                                 CtkTextTag        *tag,
                                 const CtkTextIter *start,
                                 const CtkTextIter *end)
{
  if (tag->priv->table != buffer->priv->tag_table)
    {
      g_warning ("Can only remove tags that are in the tag table for the buffer");
      return;
    }
  
  _ctk_text_btree_tag (start, end, tag, FALSE);
}

static void
ctk_text_buffer_real_changed (CtkTextBuffer *buffer)
{
  ctk_text_buffer_set_modified (buffer, TRUE);

  g_object_notify_by_pspec (G_OBJECT (buffer), text_buffer_props[PROP_TEXT]);
}

static void
ctk_text_buffer_real_mark_set (CtkTextBuffer     *buffer,
                               const CtkTextIter *iter,
                               CtkTextMark       *mark)
{
  CtkTextMark *insert;
  
  insert = ctk_text_buffer_get_insert (buffer);

  if (mark == insert || mark == ctk_text_buffer_get_selection_bound (buffer))
    {
      gboolean has_selection;

      update_selection_clipboards (buffer);
    
      has_selection = ctk_text_buffer_get_selection_bounds (buffer,
                                                            NULL,
                                                            NULL);

      if (has_selection != buffer->priv->has_selection)
        {
          buffer->priv->has_selection = has_selection;
          g_object_notify_by_pspec (G_OBJECT (buffer), text_buffer_props[PROP_HAS_SELECTION]);
        }
    }
    
    if (mark == insert)
      g_object_notify_by_pspec (G_OBJECT (buffer), text_buffer_props[PROP_CURSOR_POSITION]);
}

static void
ctk_text_buffer_emit_tag (CtkTextBuffer     *buffer,
                          CtkTextTag        *tag,
                          gboolean           apply,
                          const CtkTextIter *start,
                          const CtkTextIter *end)
{
  CtkTextIter start_tmp = *start;
  CtkTextIter end_tmp = *end;

  g_return_if_fail (tag != NULL);

  ctk_text_iter_order (&start_tmp, &end_tmp);

  if (apply)
    g_signal_emit (buffer, signals[APPLY_TAG],
                   0,
                   tag, &start_tmp, &end_tmp);
  else
    g_signal_emit (buffer, signals[REMOVE_TAG],
                   0,
                   tag, &start_tmp, &end_tmp);
}

/**
 * ctk_text_buffer_apply_tag:
 * @buffer: a #CtkTextBuffer
 * @tag: a #CtkTextTag
 * @start: one bound of range to be tagged
 * @end: other bound of range to be tagged
 *
 * Emits the “apply-tag” signal on @buffer. The default
 * handler for the signal applies @tag to the given range.
 * @start and @end do not have to be in order.
 **/
void
ctk_text_buffer_apply_tag (CtkTextBuffer     *buffer,
                           CtkTextTag        *tag,
                           const CtkTextIter *start,
                           const CtkTextIter *end)
{
  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (CTK_IS_TEXT_TAG (tag));
  g_return_if_fail (start != NULL);
  g_return_if_fail (end != NULL);
  g_return_if_fail (ctk_text_iter_get_buffer (start) == buffer);
  g_return_if_fail (ctk_text_iter_get_buffer (end) == buffer);
  g_return_if_fail (tag->priv->table == buffer->priv->tag_table);
  
  ctk_text_buffer_emit_tag (buffer, tag, TRUE, start, end);
}

/**
 * ctk_text_buffer_remove_tag:
 * @buffer: a #CtkTextBuffer
 * @tag: a #CtkTextTag
 * @start: one bound of range to be untagged
 * @end: other bound of range to be untagged
 *
 * Emits the “remove-tag” signal. The default handler for the signal
 * removes all occurrences of @tag from the given range. @start and
 * @end don’t have to be in order.
 **/
void
ctk_text_buffer_remove_tag (CtkTextBuffer     *buffer,
                            CtkTextTag        *tag,
                            const CtkTextIter *start,
                            const CtkTextIter *end)

{
  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (CTK_IS_TEXT_TAG (tag));
  g_return_if_fail (start != NULL);
  g_return_if_fail (end != NULL);
  g_return_if_fail (ctk_text_iter_get_buffer (start) == buffer);
  g_return_if_fail (ctk_text_iter_get_buffer (end) == buffer);
  g_return_if_fail (tag->priv->table == buffer->priv->tag_table);
  
  ctk_text_buffer_emit_tag (buffer, tag, FALSE, start, end);
}

/**
 * ctk_text_buffer_apply_tag_by_name:
 * @buffer: a #CtkTextBuffer
 * @name: name of a named #CtkTextTag
 * @start: one bound of range to be tagged
 * @end: other bound of range to be tagged
 *
 * Calls ctk_text_tag_table_lookup() on the buffer’s tag table to
 * get a #CtkTextTag, then calls ctk_text_buffer_apply_tag().
 **/
void
ctk_text_buffer_apply_tag_by_name (CtkTextBuffer     *buffer,
                                   const gchar       *name,
                                   const CtkTextIter *start,
                                   const CtkTextIter *end)
{
  CtkTextTag *tag;

  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (name != NULL);
  g_return_if_fail (start != NULL);
  g_return_if_fail (end != NULL);
  g_return_if_fail (ctk_text_iter_get_buffer (start) == buffer);
  g_return_if_fail (ctk_text_iter_get_buffer (end) == buffer);

  tag = ctk_text_tag_table_lookup (get_table (buffer),
                                   name);

  if (tag == NULL)
    {
      g_warning ("Unknown tag '%s'", name);
      return;
    }

  ctk_text_buffer_emit_tag (buffer, tag, TRUE, start, end);
}

/**
 * ctk_text_buffer_remove_tag_by_name:
 * @buffer: a #CtkTextBuffer
 * @name: name of a #CtkTextTag
 * @start: one bound of range to be untagged
 * @end: other bound of range to be untagged
 *
 * Calls ctk_text_tag_table_lookup() on the buffer’s tag table to
 * get a #CtkTextTag, then calls ctk_text_buffer_remove_tag().
 **/
void
ctk_text_buffer_remove_tag_by_name (CtkTextBuffer     *buffer,
                                    const gchar       *name,
                                    const CtkTextIter *start,
                                    const CtkTextIter *end)
{
  CtkTextTag *tag;

  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (name != NULL);
  g_return_if_fail (start != NULL);
  g_return_if_fail (end != NULL);
  g_return_if_fail (ctk_text_iter_get_buffer (start) == buffer);
  g_return_if_fail (ctk_text_iter_get_buffer (end) == buffer);
  
  tag = ctk_text_tag_table_lookup (get_table (buffer),
                                   name);

  if (tag == NULL)
    {
      g_warning ("Unknown tag '%s'", name);
      return;
    }

  ctk_text_buffer_emit_tag (buffer, tag, FALSE, start, end);
}

static gint
pointer_cmp (gconstpointer a,
             gconstpointer b)
{
  if (a < b)
    return -1;
  else if (a > b)
    return 1;
  else
    return 0;
}

/**
 * ctk_text_buffer_remove_all_tags:
 * @buffer: a #CtkTextBuffer
 * @start: one bound of range to be untagged
 * @end: other bound of range to be untagged
 * 
 * Removes all tags in the range between @start and @end.  Be careful
 * with this function; it could remove tags added in code unrelated to
 * the code you’re currently writing. That is, using this function is
 * probably a bad idea if you have two or more unrelated code sections
 * that add tags.
 **/
void
ctk_text_buffer_remove_all_tags (CtkTextBuffer     *buffer,
                                 const CtkTextIter *start,
                                 const CtkTextIter *end)
{
  CtkTextIter first, second, tmp;
  GSList *tags;
  GSList *tmp_list;
  GSList *prev, *next;
  CtkTextTag *tag;
  
  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (start != NULL);
  g_return_if_fail (end != NULL);
  g_return_if_fail (ctk_text_iter_get_buffer (start) == buffer);
  g_return_if_fail (ctk_text_iter_get_buffer (end) == buffer);
  
  first = *start;
  second = *end;

  ctk_text_iter_order (&first, &second);

  /* Get all tags turned on at the start */
  tags = ctk_text_iter_get_tags (&first);
  
  /* Find any that are toggled on within the range */
  tmp = first;
  while (ctk_text_iter_forward_to_tag_toggle (&tmp, NULL))
    {
      GSList *toggled;
      GSList *tmp_list2;

      if (ctk_text_iter_compare (&tmp, &second) >= 0)
        break; /* past the end of the range */
      
      toggled = ctk_text_iter_get_toggled_tags (&tmp, TRUE);

      /* We could end up with a really big-ass list here.
       * Fix it someday.
       */
      tmp_list2 = toggled;
      while (tmp_list2 != NULL)
        {
          tags = g_slist_prepend (tags, tmp_list2->data);

          tmp_list2 = tmp_list2->next;
        }

      g_slist_free (toggled);
    }
  
  /* Sort the list */
  tags = g_slist_sort (tags, pointer_cmp);

  /* Strip duplicates */
  tag = NULL;
  prev = NULL;
  tmp_list = tags;
  while (tmp_list != NULL)
    {
      if (tag == tmp_list->data)
        {
          /* duplicate */
          next = tmp_list->next;
          if (prev)
            prev->next = next;

          tmp_list->next = NULL;

          g_slist_free (tmp_list);

          tmp_list = next;
          /* prev is unchanged */
        }
      else
        {
          /* not a duplicate */
          tag = CTK_TEXT_TAG (tmp_list->data);
          prev = tmp_list;
          tmp_list = tmp_list->next;
        }
    }

  g_slist_foreach (tags, (GFunc) g_object_ref, NULL);
  
  tmp_list = tags;
  while (tmp_list != NULL)
    {
      tag = CTK_TEXT_TAG (tmp_list->data);

      ctk_text_buffer_remove_tag (buffer, tag, &first, &second);
      
      tmp_list = tmp_list->next;
    }

  g_slist_free_full (tags, g_object_unref);
}


/*
 * Obtain various iterators
 */

/**
 * ctk_text_buffer_get_iter_at_line_offset:
 * @buffer: a #CtkTextBuffer
 * @iter: (out): iterator to initialize
 * @line_number: line number counting from 0
 * @char_offset: char offset from start of line
 *
 * Obtains an iterator pointing to @char_offset within the given line. Note
 * characters, not bytes; UTF-8 may encode one character as multiple bytes.
 *
 * Before the 3.20 version, it was not allowed to pass an invalid location.
 *
 * Since the 3.20 version, if @line_number is greater than the number of lines
 * in the @buffer, the end iterator is returned. And if @char_offset is off the
 * end of the line, the iterator at the end of the line is returned.
 **/
void
ctk_text_buffer_get_iter_at_line_offset (CtkTextBuffer *buffer,
                                         CtkTextIter   *iter,
                                         gint           line_number,
                                         gint           char_offset)
{
  CtkTextIter end_line_iter;

  g_return_if_fail (iter != NULL);
  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));

  if (line_number >= ctk_text_buffer_get_line_count (buffer))
    {
      ctk_text_buffer_get_end_iter (buffer, iter);
      return;
    }

  _ctk_text_btree_get_iter_at_line_char (get_btree (buffer), iter, line_number, 0);

  end_line_iter = *iter;
  if (!ctk_text_iter_ends_line (&end_line_iter))
    ctk_text_iter_forward_to_line_end (&end_line_iter);

  if (char_offset <= ctk_text_iter_get_line_offset (&end_line_iter))
    ctk_text_iter_set_line_offset (iter, char_offset);
  else
    *iter = end_line_iter;
}

/**
 * ctk_text_buffer_get_iter_at_line_index:
 * @buffer: a #CtkTextBuffer 
 * @iter: (out): iterator to initialize 
 * @line_number: line number counting from 0
 * @byte_index: byte index from start of line
 *
 * Obtains an iterator pointing to @byte_index within the given line.
 * @byte_index must be the start of a UTF-8 character. Note bytes, not
 * characters; UTF-8 may encode one character as multiple bytes.
 *
 * Before the 3.20 version, it was not allowed to pass an invalid location.
 *
 * Since the 3.20 version, if @line_number is greater than the number of lines
 * in the @buffer, the end iterator is returned. And if @byte_index is off the
 * end of the line, the iterator at the end of the line is returned.
 **/
void
ctk_text_buffer_get_iter_at_line_index  (CtkTextBuffer *buffer,
                                         CtkTextIter   *iter,
                                         gint           line_number,
                                         gint           byte_index)
{
  CtkTextIter end_line_iter;

  g_return_if_fail (iter != NULL);
  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));

  if (line_number >= ctk_text_buffer_get_line_count (buffer))
    {
      ctk_text_buffer_get_end_iter (buffer, iter);
      return;
    }

  ctk_text_buffer_get_iter_at_line (buffer, iter, line_number);

  end_line_iter = *iter;
  if (!ctk_text_iter_ends_line (&end_line_iter))
    ctk_text_iter_forward_to_line_end (&end_line_iter);

  if (byte_index <= ctk_text_iter_get_line_index (&end_line_iter))
    ctk_text_iter_set_line_index (iter, byte_index);
  else
    *iter = end_line_iter;
}

/**
 * ctk_text_buffer_get_iter_at_line:
 * @buffer: a #CtkTextBuffer 
 * @iter: (out): iterator to initialize
 * @line_number: line number counting from 0
 *
 * Initializes @iter to the start of the given line. If @line_number is greater
 * than the number of lines in the @buffer, the end iterator is returned.
 **/
void
ctk_text_buffer_get_iter_at_line (CtkTextBuffer *buffer,
                                  CtkTextIter   *iter,
                                  gint           line_number)
{
  g_return_if_fail (iter != NULL);
  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));

  ctk_text_buffer_get_iter_at_line_offset (buffer, iter, line_number, 0);
}

/**
 * ctk_text_buffer_get_iter_at_offset:
 * @buffer: a #CtkTextBuffer 
 * @iter: (out): iterator to initialize
 * @char_offset: char offset from start of buffer, counting from 0, or -1
 *
 * Initializes @iter to a position @char_offset chars from the start
 * of the entire buffer. If @char_offset is -1 or greater than the number
 * of characters in the buffer, @iter is initialized to the end iterator,
 * the iterator one past the last valid character in the buffer.
 **/
void
ctk_text_buffer_get_iter_at_offset (CtkTextBuffer *buffer,
                                    CtkTextIter   *iter,
                                    gint           char_offset)
{
  g_return_if_fail (iter != NULL);
  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));

  _ctk_text_btree_get_iter_at_char (get_btree (buffer), iter, char_offset);
}

/**
 * ctk_text_buffer_get_start_iter:
 * @buffer: a #CtkTextBuffer
 * @iter: (out): iterator to initialize
 *
 * Initialized @iter with the first position in the text buffer. This
 * is the same as using ctk_text_buffer_get_iter_at_offset() to get
 * the iter at character offset 0.
 **/
void
ctk_text_buffer_get_start_iter (CtkTextBuffer *buffer,
                                CtkTextIter   *iter)
{
  g_return_if_fail (iter != NULL);
  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));

  _ctk_text_btree_get_iter_at_char (get_btree (buffer), iter, 0);
}

/**
 * ctk_text_buffer_get_end_iter:
 * @buffer: a #CtkTextBuffer 
 * @iter: (out): iterator to initialize
 *
 * Initializes @iter with the “end iterator,” one past the last valid
 * character in the text buffer. If dereferenced with
 * ctk_text_iter_get_char(), the end iterator has a character value of 0.
 * The entire buffer lies in the range from the first position in
 * the buffer (call ctk_text_buffer_get_start_iter() to get
 * character position 0) to the end iterator.
 **/
void
ctk_text_buffer_get_end_iter (CtkTextBuffer *buffer,
                              CtkTextIter   *iter)
{
  g_return_if_fail (iter != NULL);
  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));

  _ctk_text_btree_get_end_iter (get_btree (buffer), iter);
}

/**
 * ctk_text_buffer_get_bounds:
 * @buffer: a #CtkTextBuffer 
 * @start: (out): iterator to initialize with first position in the buffer
 * @end: (out): iterator to initialize with the end iterator
 *
 * Retrieves the first and last iterators in the buffer, i.e. the
 * entire buffer lies within the range [@start,@end).
 **/
void
ctk_text_buffer_get_bounds (CtkTextBuffer *buffer,
                            CtkTextIter   *start,
                            CtkTextIter   *end)
{
  g_return_if_fail (start != NULL);
  g_return_if_fail (end != NULL);
  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));

  _ctk_text_btree_get_iter_at_char (get_btree (buffer), start, 0);
  _ctk_text_btree_get_end_iter (get_btree (buffer), end);
}

/*
 * Modified flag
 */

/**
 * ctk_text_buffer_get_modified:
 * @buffer: a #CtkTextBuffer 
 * 
 * Indicates whether the buffer has been modified since the last call
 * to ctk_text_buffer_set_modified() set the modification flag to
 * %FALSE. Used for example to enable a “save” function in a text
 * editor.
 * 
 * Returns: %TRUE if the buffer has been modified
 **/
gboolean
ctk_text_buffer_get_modified (CtkTextBuffer *buffer)
{
  g_return_val_if_fail (CTK_IS_TEXT_BUFFER (buffer), FALSE);

  return buffer->priv->modified;
}

/**
 * ctk_text_buffer_set_modified:
 * @buffer: a #CtkTextBuffer 
 * @setting: modification flag setting
 *
 * Used to keep track of whether the buffer has been modified since the
 * last time it was saved. Whenever the buffer is saved to disk, call
 * ctk_text_buffer_set_modified (@buffer, FALSE). When the buffer is modified,
 * it will automatically toggled on the modified bit again. When the modified
 * bit flips, the buffer emits the #CtkTextBuffer::modified-changed signal.
 **/
void
ctk_text_buffer_set_modified (CtkTextBuffer *buffer,
                              gboolean       setting)
{
  gboolean fixed_setting;

  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));

  fixed_setting = setting != FALSE;

  if (buffer->priv->modified == fixed_setting)
    return;
  else
    {
      buffer->priv->modified = fixed_setting;
      g_signal_emit (buffer, signals[MODIFIED_CHANGED], 0);
    }
}

/**
 * ctk_text_buffer_get_has_selection:
 * @buffer: a #CtkTextBuffer 
 * 
 * Indicates whether the buffer has some text currently selected.
 * 
 * Returns: %TRUE if the there is text selected
 *
 * Since: 2.10
 **/
gboolean
ctk_text_buffer_get_has_selection (CtkTextBuffer *buffer)
{
  g_return_val_if_fail (CTK_IS_TEXT_BUFFER (buffer), FALSE);

  return buffer->priv->has_selection;
}


/*
 * Assorted other stuff
 */

/**
 * ctk_text_buffer_get_line_count:
 * @buffer: a #CtkTextBuffer 
 * 
 * Obtains the number of lines in the buffer. This value is cached, so
 * the function is very fast.
 * 
 * Returns: number of lines in the buffer
 **/
gint
ctk_text_buffer_get_line_count (CtkTextBuffer *buffer)
{
  g_return_val_if_fail (CTK_IS_TEXT_BUFFER (buffer), 0);

  return _ctk_text_btree_line_count (get_btree (buffer));
}

/**
 * ctk_text_buffer_get_char_count:
 * @buffer: a #CtkTextBuffer 
 * 
 * Gets the number of characters in the buffer; note that characters
 * and bytes are not the same, you can’t e.g. expect the contents of
 * the buffer in string form to be this many bytes long. The character
 * count is cached, so this function is very fast.
 * 
 * Returns: number of characters in the buffer
 **/
gint
ctk_text_buffer_get_char_count (CtkTextBuffer *buffer)
{
  g_return_val_if_fail (CTK_IS_TEXT_BUFFER (buffer), 0);

  return _ctk_text_btree_char_count (get_btree (buffer));
}

/* Called when we lose the primary selection.
 */
static void
clipboard_clear_selection_cb (CtkClipboard *clipboard,
                              gpointer      data)
{
  /* Move selection_bound to the insertion point */
  CtkTextIter insert;
  CtkTextIter selection_bound;
  CtkTextBuffer *buffer = CTK_TEXT_BUFFER (data);

  ctk_text_buffer_get_iter_at_mark (buffer, &insert,
                                    ctk_text_buffer_get_insert (buffer));
  ctk_text_buffer_get_iter_at_mark (buffer, &selection_bound,
                                    ctk_text_buffer_get_selection_bound (buffer));

  if (!ctk_text_iter_equal (&insert, &selection_bound))
    ctk_text_buffer_move_mark (buffer,
                               ctk_text_buffer_get_selection_bound (buffer),
                               &insert);
}

/* Called when we have the primary selection and someone else wants our
 * data in order to paste it.
 */
static void
clipboard_get_selection_cb (CtkClipboard     *clipboard,
                            CtkSelectionData *selection_data,
                            guint             info,
                            gpointer          data)
{
  CtkTextBuffer *buffer = CTK_TEXT_BUFFER (data);
  CtkTextIter start, end;

  if (ctk_text_buffer_get_selection_bounds (buffer, &start, &end))
    {
      if (info == CTK_TEXT_BUFFER_TARGET_INFO_BUFFER_CONTENTS)
        {
          /* Provide the address of the buffer; this will only be
           * used within-process
           */
          ctk_selection_data_set (selection_data,
                                  ctk_selection_data_get_target (selection_data),
                                  8, /* bytes */
                                  (void*)&buffer,
                                  sizeof (buffer));
        }
      else if (info == CTK_TEXT_BUFFER_TARGET_INFO_RICH_TEXT)
        {
          guint8 *str;
          gsize   len;

          str = ctk_text_buffer_serialize (buffer, buffer,
                                           ctk_selection_data_get_target (selection_data),
                                           &start, &end, &len);

          ctk_selection_data_set (selection_data,
                                  ctk_selection_data_get_target (selection_data),
                                  8, /* bytes */
                                  str, len);
          g_free (str);
        }
      else
        {
          gchar *str;

          str = ctk_text_iter_get_visible_text (&start, &end);
          ctk_selection_data_set_text (selection_data, str, -1);
          g_free (str);
        }
    }
}

static CtkTextBuffer *
create_clipboard_contents_buffer (CtkTextBuffer *buffer)
{
  CtkTextBuffer *contents;

  contents = ctk_text_buffer_new (ctk_text_buffer_get_tag_table (buffer));

  g_object_set_data (G_OBJECT (contents), I_("ctk-text-buffer-clipboard-source"),
                     buffer);
  g_object_set_data (G_OBJECT (contents), I_("ctk-text-buffer-clipboard"),
                     GINT_TO_POINTER (1));

  /*  Ref the source buffer as long as the clipboard contents buffer
   *  exists, because it's needed for serializing the contents buffer.
   *  See http://bugzilla.gnome.org/show_bug.cgi?id=339195
   */
  g_object_ref (buffer);
  g_object_weak_ref (G_OBJECT (contents), (GWeakNotify) g_object_unref, buffer);

  return contents;
}

/* Provide cut/copied data */
static void
clipboard_get_contents_cb (CtkClipboard     *clipboard,
                           CtkSelectionData *selection_data,
                           guint             info,
                           gpointer          data)
{
  CtkTextBuffer *contents = CTK_TEXT_BUFFER (data);

  g_assert (contents); /* This should never be called unless we own the clipboard */

  if (info == CTK_TEXT_BUFFER_TARGET_INFO_BUFFER_CONTENTS)
    {
      /* Provide the address of the clipboard buffer; this will only
       * be used within-process. OK to supply a NULL value for contents.
       */
      ctk_selection_data_set (selection_data,
                              ctk_selection_data_get_target (selection_data),
                              8, /* bytes */
                              (void*)&contents,
                              sizeof (contents));
    }
  else if (info == CTK_TEXT_BUFFER_TARGET_INFO_RICH_TEXT)
    {
      CtkTextBuffer *clipboard_source_buffer;
      CtkTextIter start, end;
      guint8 *str;
      gsize   len;

      clipboard_source_buffer = g_object_get_data (G_OBJECT (contents),
                                                   "ctk-text-buffer-clipboard-source");

      ctk_text_buffer_get_bounds (contents, &start, &end);

      str = ctk_text_buffer_serialize (clipboard_source_buffer, contents,
                                       ctk_selection_data_get_target (selection_data),
                                       &start, &end, &len);

      ctk_selection_data_set (selection_data,
			      ctk_selection_data_get_target (selection_data),
			      8, /* bytes */
			      str, len);
      g_free (str);
    }
  else
    {
      gchar *str;
      CtkTextIter start, end;

      ctk_text_buffer_get_bounds (contents, &start, &end);

      str = ctk_text_iter_get_visible_text (&start, &end);
      ctk_selection_data_set_text (selection_data, str, -1);
      g_free (str);
    }
}

static void
clipboard_clear_contents_cb (CtkClipboard *clipboard,
                             gpointer      data)
{
  CtkTextBuffer *contents = CTK_TEXT_BUFFER (data);

  g_object_unref (contents);
}

static void
get_paste_point (CtkTextBuffer *buffer,
                 CtkTextIter   *iter,
                 gboolean       clear_afterward)
{
  CtkTextIter insert_point;
  CtkTextMark *paste_point_override;

  paste_point_override = ctk_text_buffer_get_mark (buffer,
                                                   "ctk_paste_point_override");

  if (paste_point_override != NULL)
    {
      ctk_text_buffer_get_iter_at_mark (buffer, &insert_point,
                                        paste_point_override);
      if (clear_afterward)
        ctk_text_buffer_delete_mark (buffer, paste_point_override);
    }
  else
    {
      ctk_text_buffer_get_iter_at_mark (buffer, &insert_point,
                                        ctk_text_buffer_get_insert (buffer));
    }

  *iter = insert_point;
}

static void
pre_paste_prep (ClipboardRequest *request_data,
                CtkTextIter      *insert_point)
{
  CtkTextBuffer *buffer = request_data->buffer;
  
  get_paste_point (buffer, insert_point, TRUE);

  if (request_data->replace_selection)
    {
      CtkTextIter start, end;
      
      if (ctk_text_buffer_get_selection_bounds (buffer, &start, &end))
        {
          if (request_data->interactive)
            ctk_text_buffer_delete_interactive (request_data->buffer,
                                                &start,
                                                &end,
                                                request_data->default_editable);
          else
            ctk_text_buffer_delete (request_data->buffer, &start, &end);

          *insert_point = start;
        }
    }
}

static void
emit_paste_done (CtkTextBuffer *buffer,
                 CtkClipboard  *clipboard)
{
  g_signal_emit (buffer, signals[PASTE_DONE], 0, clipboard);
}

static void
free_clipboard_request (ClipboardRequest *request_data)
{
  g_object_unref (request_data->buffer);
  g_slice_free (ClipboardRequest, request_data);
}

/* Called when we request a paste and receive the text data
 */
static void
clipboard_text_received (CtkClipboard *clipboard,
                         const gchar  *str,
                         gpointer      data)
{
  ClipboardRequest *request_data = data;
  CtkTextBuffer *buffer = request_data->buffer;

  if (str)
    {
      CtkTextIter insert_point;
      
      if (request_data->interactive) 
	ctk_text_buffer_begin_user_action (buffer);

      pre_paste_prep (request_data, &insert_point);
      
      if (request_data->interactive) 
	ctk_text_buffer_insert_interactive (buffer, &insert_point,
					    str, -1, request_data->default_editable);
      else
        ctk_text_buffer_insert (buffer, &insert_point,
                                str, -1);

      if (request_data->interactive) 
	ctk_text_buffer_end_user_action (buffer);

      emit_paste_done (buffer, clipboard);
    }
  else
    {
      /* It may happen that we set a point override but we are not inserting
         any text, so we must remove it afterwards */
      CtkTextMark *paste_point_override;

      paste_point_override = ctk_text_buffer_get_mark (buffer,
                                                       "ctk_paste_point_override");

      if (paste_point_override != NULL)
        ctk_text_buffer_delete_mark (buffer, paste_point_override);
    }

  free_clipboard_request (request_data);
}

static CtkTextBuffer*
selection_data_get_buffer (CtkSelectionData *selection_data,
                           ClipboardRequest *request_data)
{
  GdkWindow *owner;
  CtkTextBuffer *src_buffer = NULL;

  /* If we can get the owner, the selection is in-process */
  owner = gdk_selection_owner_get_for_display (ctk_selection_data_get_display (selection_data),
					       ctk_selection_data_get_selection (selection_data));

  if (owner == NULL)
    return NULL;
  
  if (gdk_window_get_window_type (owner) == GDK_WINDOW_FOREIGN)
    return NULL;

  if (ctk_selection_data_get_data_type (selection_data) != gdk_atom_intern_static_string ("CTK_TEXT_BUFFER_CONTENTS"))
    return NULL;

  if (ctk_selection_data_get_length (selection_data) != sizeof (src_buffer))
    return NULL;

  memcpy (&src_buffer, ctk_selection_data_get_data (selection_data), sizeof (src_buffer));

  if (src_buffer == NULL)
    return NULL;
  
  g_return_val_if_fail (CTK_IS_TEXT_BUFFER (src_buffer), NULL);

  if (ctk_text_buffer_get_tag_table (src_buffer) !=
      ctk_text_buffer_get_tag_table (request_data->buffer))
    return NULL;
  
  return src_buffer;
}

#if 0
/* These are pretty handy functions; maybe something like them
 * should be in the public API. Also, there are other places in this
 * file where they could be used.
 */
static gpointer
save_iter (const CtkTextIter *iter,
           gboolean           left_gravity)
{
  return ctk_text_buffer_create_mark (ctk_text_iter_get_buffer (iter),
                                      NULL,
                                      iter,
                                      TRUE);
}

static void
restore_iter (const CtkTextIter *iter,
              gpointer           save_id)
{
  ctk_text_buffer_get_iter_at_mark (ctk_text_mark_get_buffer (save_id),
                                    (CtkTextIter*) iter,
                                    save_id);
  ctk_text_buffer_delete_mark (ctk_text_mark_get_buffer (save_id),
                               save_id);
}
#endif

static void
clipboard_rich_text_received (CtkClipboard *clipboard,
                              GdkAtom       format,
                              const guint8 *text,
                              gsize         length,
                              gpointer      data)
{
  ClipboardRequest *request_data = data;
  CtkTextIter insert_point;
  gboolean retval = TRUE;
  GError *error = NULL;

  if (text != NULL && length > 0)
    {
      if (request_data->interactive)
        ctk_text_buffer_begin_user_action (request_data->buffer);

      pre_paste_prep (request_data, &insert_point);

      if (!request_data->interactive ||
          ctk_text_iter_can_insert (&insert_point,
                                    request_data->default_editable))
        {
          retval = ctk_text_buffer_deserialize (request_data->buffer,
                                                request_data->buffer,
                                                format,
                                                &insert_point,
                                                text, length,
                                                &error);
        }

      if (!retval)
        {
          g_warning ("error pasting: %s\n", error->message);
          g_clear_error (&error);
        }

      if (request_data->interactive)
        ctk_text_buffer_end_user_action (request_data->buffer);

      emit_paste_done (request_data->buffer, clipboard);

      if (retval)
        return;
    }

  /* Request the text selection instead */
  ctk_clipboard_request_text (clipboard,
                              clipboard_text_received,
                              data);
}

static void
paste_from_buffer (CtkClipboard      *clipboard,
                   ClipboardRequest  *request_data,
                   CtkTextBuffer     *src_buffer,
                   const CtkTextIter *start,
                   const CtkTextIter *end)
{
  CtkTextIter insert_point;
  CtkTextBuffer *buffer = request_data->buffer;

  /* We're about to emit a bunch of signals, so be safe */
  g_object_ref (src_buffer);

  /* Replacing the selection with itself */
  if (request_data->replace_selection &&
      buffer == src_buffer)
    {
      /* Clear the paste point if needed */
      get_paste_point (buffer, &insert_point, TRUE);
      goto done;
    }

  if (request_data->interactive) 
    ctk_text_buffer_begin_user_action (buffer);

  pre_paste_prep (request_data, &insert_point);

  if (!ctk_text_iter_equal (start, end))
    {
      if (!request_data->interactive ||
          (ctk_text_iter_can_insert (&insert_point,
                                     request_data->default_editable)))
        ctk_text_buffer_real_insert_range (buffer,
                                           &insert_point,
                                           start,
                                           end,
                                           request_data->interactive);
    }

  if (request_data->interactive) 
    ctk_text_buffer_end_user_action (buffer);

done:
  emit_paste_done (buffer, clipboard);

  g_object_unref (src_buffer);

  free_clipboard_request (request_data);
}

static void
clipboard_clipboard_buffer_received (CtkClipboard     *clipboard,
                                     CtkSelectionData *selection_data,
                                     gpointer          data)
{
  ClipboardRequest *request_data = data;
  CtkTextBuffer *src_buffer;

  src_buffer = selection_data_get_buffer (selection_data, request_data); 

  if (src_buffer)
    {
      CtkTextIter start, end;

      if (g_object_get_data (G_OBJECT (src_buffer), "ctk-text-buffer-clipboard"))
	{
	  ctk_text_buffer_get_bounds (src_buffer, &start, &end);

	  paste_from_buffer (clipboard, request_data, src_buffer,
			     &start, &end);
	}
      else
	{
	  if (ctk_text_buffer_get_selection_bounds (src_buffer, &start, &end))
	    paste_from_buffer (clipboard, request_data, src_buffer,
			       &start, &end);
	}
    }
  else
    {
      if (ctk_clipboard_wait_is_rich_text_available (clipboard,
                                                     request_data->buffer))
        {
          /* Request rich text */
          ctk_clipboard_request_rich_text (clipboard,
                                           request_data->buffer,
                                           clipboard_rich_text_received,
                                           data);
        }
      else
        {
          /* Request the text selection instead */
          ctk_clipboard_request_text (clipboard,
                                      clipboard_text_received,
                                      data);
        }
    }
}

typedef struct
{
  CtkClipboard *clipboard;
  guint ref_count;
} SelectionClipboard;

static void
update_selection_clipboards (CtkTextBuffer *buffer)
{
  CtkTextBufferPrivate *priv;
  gboolean has_selection;
  CtkTextIter start;
  CtkTextIter end;
  GSList *tmp_list;

  priv = buffer->priv;

  ctk_text_buffer_get_copy_target_list (buffer);
  has_selection = ctk_text_buffer_get_selection_bounds (buffer, &start, &end);
  tmp_list = buffer->priv->selection_clipboards;

  while (tmp_list)
    {
      SelectionClipboard *selection_clipboard = tmp_list->data;
      CtkClipboard *clipboard = selection_clipboard->clipboard;

      if (has_selection)
        {
          /* Even if we already have the selection, we need to update our
           * timestamp.
           */
          ctk_clipboard_set_with_owner (clipboard,
                                        priv->copy_target_entries,
                                        priv->n_copy_target_entries,
                                        clipboard_get_selection_cb,
                                        clipboard_clear_selection_cb,
                                        G_OBJECT (buffer));
        }
      else if (ctk_clipboard_get_owner (clipboard) == G_OBJECT (buffer))
        ctk_clipboard_clear (clipboard);

      tmp_list = tmp_list->next;
    }
}

static SelectionClipboard *
find_selection_clipboard (CtkTextBuffer *buffer,
			  CtkClipboard  *clipboard)
{
  GSList *tmp_list = buffer->priv->selection_clipboards;
  while (tmp_list)
    {
      SelectionClipboard *selection_clipboard = tmp_list->data;
      if (selection_clipboard->clipboard == clipboard)
	return selection_clipboard;
      
      tmp_list = tmp_list->next;
    }

  return NULL;
}

/**
 * ctk_text_buffer_add_selection_clipboard:
 * @buffer: a #CtkTextBuffer
 * @clipboard: a #CtkClipboard
 * 
 * Adds @clipboard to the list of clipboards in which the selection 
 * contents of @buffer are available. In most cases, @clipboard will be 
 * the #CtkClipboard of type %GDK_SELECTION_PRIMARY for a view of @buffer.
 **/
void
ctk_text_buffer_add_selection_clipboard (CtkTextBuffer *buffer,
					 CtkClipboard  *clipboard)
{
  SelectionClipboard *selection_clipboard;

  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (clipboard != NULL);

  selection_clipboard = find_selection_clipboard (buffer, clipboard);
  if (selection_clipboard)
    {
      selection_clipboard->ref_count++;
    }
  else
    {
      selection_clipboard = g_slice_new (SelectionClipboard);

      selection_clipboard->clipboard = clipboard;
      selection_clipboard->ref_count = 1;

      buffer->priv->selection_clipboards = g_slist_prepend (buffer->priv->selection_clipboards,
                                                            selection_clipboard);
    }
}

/**
 * ctk_text_buffer_remove_selection_clipboard:
 * @buffer: a #CtkTextBuffer
 * @clipboard: a #CtkClipboard added to @buffer by 
 *             ctk_text_buffer_add_selection_clipboard()
 * 
 * Removes a #CtkClipboard added with 
 * ctk_text_buffer_add_selection_clipboard().
 **/
void 
ctk_text_buffer_remove_selection_clipboard (CtkTextBuffer *buffer,
					    CtkClipboard  *clipboard)
{
  SelectionClipboard *selection_clipboard;

  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (clipboard != NULL);

  selection_clipboard = find_selection_clipboard (buffer, clipboard);
  g_return_if_fail (selection_clipboard != NULL);

  selection_clipboard->ref_count--;
  if (selection_clipboard->ref_count == 0)
    {
      if (ctk_clipboard_get_owner (selection_clipboard->clipboard) == G_OBJECT (buffer))
	ctk_clipboard_clear (selection_clipboard->clipboard);

      buffer->priv->selection_clipboards = g_slist_remove (buffer->priv->selection_clipboards,
                                                           selection_clipboard);

      g_slice_free (SelectionClipboard, selection_clipboard);
    }
}

static void
remove_all_selection_clipboards (CtkTextBuffer *buffer)
{
  CtkTextBufferPrivate *priv = buffer->priv;
  GSList *l;

  for (l = priv->selection_clipboards; l != NULL; l = l->next)
    {
      SelectionClipboard *selection_clipboard = l->data;
      g_slice_free (SelectionClipboard, selection_clipboard);
    }

  g_slist_free (priv->selection_clipboards);
  priv->selection_clipboards = NULL;
}

/**
 * ctk_text_buffer_paste_clipboard:
 * @buffer: a #CtkTextBuffer
 * @clipboard: the #CtkClipboard to paste from
 * @override_location: (allow-none): location to insert pasted text, or %NULL
 * @default_editable: whether the buffer is editable by default
 *
 * Pastes the contents of a clipboard. If @override_location is %NULL, the
 * pasted text will be inserted at the cursor position, or the buffer selection
 * will be replaced if the selection is non-empty.
 *
 * Note: pasting is asynchronous, that is, we’ll ask for the paste data and
 * return, and at some point later after the main loop runs, the paste data will
 * be inserted.
 **/
void
ctk_text_buffer_paste_clipboard (CtkTextBuffer *buffer,
				 CtkClipboard  *clipboard,
				 CtkTextIter   *override_location,
                                 gboolean       default_editable)
{
  ClipboardRequest *data = g_slice_new (ClipboardRequest);
  CtkTextIter paste_point;
  CtkTextIter start, end;

  if (override_location != NULL)
    ctk_text_buffer_create_mark (buffer,
                                 "ctk_paste_point_override",
                                 override_location, FALSE);

  data->buffer = g_object_ref (buffer);
  data->interactive = TRUE;
  data->default_editable = !!default_editable;

  /* When pasting with the cursor inside the selection area, you
   * replace the selection with the new text, otherwise, you
   * simply insert the new text at the point where the click
   * occurred, unselecting any selected text. The replace_selection
   * flag toggles this behavior.
   */
  data->replace_selection = FALSE;
  
  get_paste_point (buffer, &paste_point, FALSE);
  if (ctk_text_buffer_get_selection_bounds (buffer, &start, &end) &&
      (ctk_text_iter_in_range (&paste_point, &start, &end) ||
       ctk_text_iter_equal (&paste_point, &end)))
    data->replace_selection = TRUE;

  ctk_clipboard_request_contents (clipboard,
				  gdk_atom_intern_static_string ("CTK_TEXT_BUFFER_CONTENTS"),
				  clipboard_clipboard_buffer_received, data);
}

/**
 * ctk_text_buffer_delete_selection:
 * @buffer: a #CtkTextBuffer 
 * @interactive: whether the deletion is caused by user interaction
 * @default_editable: whether the buffer is editable by default
 *
 * Deletes the range between the “insert” and “selection_bound” marks,
 * that is, the currently-selected text. If @interactive is %TRUE,
 * the editability of the selection will be considered (users can’t delete
 * uneditable text).
 * 
 * Returns: whether there was a non-empty selection to delete
 **/
gboolean
ctk_text_buffer_delete_selection (CtkTextBuffer *buffer,
                                  gboolean interactive,
                                  gboolean default_editable)
{
  CtkTextIter start;
  CtkTextIter end;

  if (!ctk_text_buffer_get_selection_bounds (buffer, &start, &end))
    {
      return FALSE; /* No selection */
    }
  else
    {
      if (interactive)
        ctk_text_buffer_delete_interactive (buffer, &start, &end, default_editable);
      else
        ctk_text_buffer_delete (buffer, &start, &end);

      return TRUE; /* We deleted stuff */
    }
}

/**
 * ctk_text_buffer_backspace:
 * @buffer: a #CtkTextBuffer
 * @iter: a position in @buffer
 * @interactive: whether the deletion is caused by user interaction
 * @default_editable: whether the buffer is editable by default
 * 
 * Performs the appropriate action as if the user hit the delete
 * key with the cursor at the position specified by @iter. In the
 * normal case a single character will be deleted, but when
 * combining accents are involved, more than one character can
 * be deleted, and when precomposed character and accent combinations
 * are involved, less than one character will be deleted.
 * 
 * Because the buffer is modified, all outstanding iterators become 
 * invalid after calling this function; however, the @iter will be
 * re-initialized to point to the location where text was deleted. 
 *
 * Returns: %TRUE if the buffer was modified
 *
 * Since: 2.6
 **/
gboolean
ctk_text_buffer_backspace (CtkTextBuffer *buffer,
			   CtkTextIter   *iter,
			   gboolean       interactive,
			   gboolean       default_editable)
{
  gchar *cluster_text;
  CtkTextIter start;
  CtkTextIter end;
  gboolean retval = FALSE;
  const PangoLogAttr *attrs;
  gint offset;
  gboolean backspace_deletes_character;

  g_return_val_if_fail (CTK_IS_TEXT_BUFFER (buffer), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  start = *iter;
  end = *iter;

  attrs = _ctk_text_buffer_get_line_log_attrs (buffer, &start, NULL);
  offset = ctk_text_iter_get_line_offset (&start);
  backspace_deletes_character = attrs[offset].backspace_deletes_character;

  ctk_text_iter_backward_cursor_position (&start);

  if (ctk_text_iter_equal (&start, &end))
    return FALSE;
    
  cluster_text = ctk_text_iter_get_text (&start, &end);

  if (interactive)
    ctk_text_buffer_begin_user_action (buffer);
  
  if (ctk_text_buffer_delete_interactive (buffer, &start, &end,
					  default_editable))
    {
      /* special case \r\n, since we never want to reinsert \r */
      if (backspace_deletes_character && strcmp ("\r\n", cluster_text))
	{
	  gchar *normalized_text = g_utf8_normalize (cluster_text,
						     strlen (cluster_text),
						     G_NORMALIZE_NFD);
	  glong len = g_utf8_strlen (normalized_text, -1);

	  if (len > 1)
	    ctk_text_buffer_insert_interactive (buffer,
						&start,
						normalized_text,
						g_utf8_offset_to_pointer (normalized_text, len - 1) - normalized_text,
						default_editable);
	  
	  g_free (normalized_text);
	}

      retval = TRUE;
    }
  
  if (interactive)
    ctk_text_buffer_end_user_action (buffer);
  
  g_free (cluster_text);

  /* Revalidate the users iter */
  *iter = start;

  return retval;
}

static void
cut_or_copy (CtkTextBuffer *buffer,
	     CtkClipboard  *clipboard,
             gboolean       delete_region_after,
             gboolean       interactive,
             gboolean       default_editable)
{
  CtkTextBufferPrivate *priv;

  /* We prefer to cut the selected region between selection_bound and
   * insertion point. If that region is empty, then we cut the region
   * between the "anchor" and the insertion point (this is for
   * C-space and M-w and other Emacs-style copy/yank keys). Note that
   * insert and selection_bound are guaranteed to exist, but the
   * anchor only exists sometimes.
   */
  CtkTextIter start;
  CtkTextIter end;

  priv = buffer->priv;

  ctk_text_buffer_get_copy_target_list (buffer);

  if (!ctk_text_buffer_get_selection_bounds (buffer, &start, &end))
    {
      /* Let's try the anchor thing */
      CtkTextMark * anchor = ctk_text_buffer_get_mark (buffer, "anchor");

      if (anchor == NULL)
        return;
      else
        {
          ctk_text_buffer_get_iter_at_mark (buffer, &end, anchor);
          ctk_text_iter_order (&start, &end);
        }
    }

  if (!ctk_text_iter_equal (&start, &end))
    {
      CtkTextIter ins;
      CtkTextBuffer *contents;

      contents = create_clipboard_contents_buffer (buffer);

      ctk_text_buffer_get_iter_at_offset (contents, &ins, 0);
      
      ctk_text_buffer_insert_range (contents, &ins, &start, &end);
                                    
      if (!ctk_clipboard_set_with_data (clipboard,
                                        priv->copy_target_entries,
                                        priv->n_copy_target_entries,
					clipboard_get_contents_cb,
					clipboard_clear_contents_cb,
					contents))
	g_object_unref (contents);
      else
	ctk_clipboard_set_can_store (clipboard,
                                     priv->copy_target_entries + 1,
                                     priv->n_copy_target_entries - 1);

      if (delete_region_after)
        {
          if (interactive)
            ctk_text_buffer_delete_interactive (buffer, &start, &end,
                                                default_editable);
          else
            ctk_text_buffer_delete (buffer, &start, &end);
        }
    }
}

/**
 * ctk_text_buffer_cut_clipboard:
 * @buffer: a #CtkTextBuffer
 * @clipboard: the #CtkClipboard object to cut to
 * @default_editable: default editability of the buffer
 *
 * Copies the currently-selected text to a clipboard, then deletes
 * said text if it’s editable.
 **/
void
ctk_text_buffer_cut_clipboard (CtkTextBuffer *buffer,
			       CtkClipboard  *clipboard,
                               gboolean       default_editable)
{
  ctk_text_buffer_begin_user_action (buffer);
  cut_or_copy (buffer, clipboard, TRUE, TRUE, default_editable);
  ctk_text_buffer_end_user_action (buffer);
}

/**
 * ctk_text_buffer_copy_clipboard:
 * @buffer: a #CtkTextBuffer 
 * @clipboard: the #CtkClipboard object to copy to
 *
 * Copies the currently-selected text to a clipboard.
 **/
void
ctk_text_buffer_copy_clipboard (CtkTextBuffer *buffer,
				CtkClipboard  *clipboard)
{
  cut_or_copy (buffer, clipboard, FALSE, TRUE, TRUE);
}

/**
 * ctk_text_buffer_get_selection_bounds:
 * @buffer: a #CtkTextBuffer a #CtkTextBuffer
 * @start: (out): iterator to initialize with selection start
 * @end: (out): iterator to initialize with selection end
 *
 * Returns %TRUE if some text is selected; places the bounds
 * of the selection in @start and @end (if the selection has length 0,
 * then @start and @end are filled in with the same value).
 * @start and @end will be in ascending order. If @start and @end are
 * NULL, then they are not filled in, but the return value still indicates
 * whether text is selected.
 *
 * Returns: whether the selection has nonzero length
 **/
gboolean
ctk_text_buffer_get_selection_bounds (CtkTextBuffer *buffer,
                                      CtkTextIter   *start,
                                      CtkTextIter   *end)
{
  g_return_val_if_fail (CTK_IS_TEXT_BUFFER (buffer), FALSE);

  return _ctk_text_btree_get_selection_bounds (get_btree (buffer), start, end);
}

/**
 * ctk_text_buffer_begin_user_action:
 * @buffer: a #CtkTextBuffer
 * 
 * Called to indicate that the buffer operations between here and a
 * call to ctk_text_buffer_end_user_action() are part of a single
 * user-visible operation. The operations between
 * ctk_text_buffer_begin_user_action() and
 * ctk_text_buffer_end_user_action() can then be grouped when creating
 * an undo stack. #CtkTextBuffer maintains a count of calls to
 * ctk_text_buffer_begin_user_action() that have not been closed with
 * a call to ctk_text_buffer_end_user_action(), and emits the 
 * “begin-user-action” and “end-user-action” signals only for the 
 * outermost pair of calls. This allows you to build user actions 
 * from other user actions.
 *
 * The “interactive” buffer mutation functions, such as
 * ctk_text_buffer_insert_interactive(), automatically call begin/end
 * user action around the buffer operations they perform, so there's
 * no need to add extra calls if you user action consists solely of a
 * single call to one of those functions.
 **/
void
ctk_text_buffer_begin_user_action (CtkTextBuffer *buffer)
{
  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));

  buffer->priv->user_action_count += 1;
  
  if (buffer->priv->user_action_count == 1)
    {
      /* Outermost nested user action begin emits the signal */
      g_signal_emit (buffer, signals[BEGIN_USER_ACTION], 0);
    }
}

/**
 * ctk_text_buffer_end_user_action:
 * @buffer: a #CtkTextBuffer
 * 
 * Should be paired with a call to ctk_text_buffer_begin_user_action().
 * See that function for a full explanation.
 **/
void
ctk_text_buffer_end_user_action (CtkTextBuffer *buffer)
{
  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (buffer->priv->user_action_count > 0);
  
  buffer->priv->user_action_count -= 1;
  
  if (buffer->priv->user_action_count == 0)
    {
      /* Ended the outermost-nested user action end, so emit the signal */
      g_signal_emit (buffer, signals[END_USER_ACTION], 0);
    }
}

static void
ctk_text_buffer_free_target_lists (CtkTextBuffer *buffer)
{
  CtkTextBufferPrivate *priv = buffer->priv;

  if (priv->copy_target_list)
    {
      ctk_target_list_unref (priv->copy_target_list);
      priv->copy_target_list = NULL;

      ctk_target_table_free (priv->copy_target_entries,
                             priv->n_copy_target_entries);
      priv->copy_target_entries = NULL;
      priv->n_copy_target_entries = 0;
    }

  if (priv->paste_target_list)
    {
      ctk_target_list_unref (priv->paste_target_list);
      priv->paste_target_list = NULL;

      ctk_target_table_free (priv->paste_target_entries,
                             priv->n_paste_target_entries);
      priv->paste_target_entries = NULL;
      priv->n_paste_target_entries = 0;
    }
}

static CtkTargetList *
ctk_text_buffer_get_target_list (CtkTextBuffer   *buffer,
                                 gboolean         deserializable,
                                 CtkTargetEntry **entries,
                                 gint            *n_entries)
{
  CtkTargetList *target_list;

  target_list = ctk_target_list_new (NULL, 0);

  ctk_target_list_add (target_list,
                       gdk_atom_intern_static_string ("CTK_TEXT_BUFFER_CONTENTS"),
                       CTK_TARGET_SAME_APP,
                       CTK_TEXT_BUFFER_TARGET_INFO_BUFFER_CONTENTS);

  ctk_target_list_add_rich_text_targets (target_list,
                                         CTK_TEXT_BUFFER_TARGET_INFO_RICH_TEXT,
                                         deserializable,
                                         buffer);

  ctk_target_list_add_text_targets (target_list,
                                    CTK_TEXT_BUFFER_TARGET_INFO_TEXT);

  *entries = ctk_target_table_new_from_list (target_list, n_entries);

  return target_list;
}

/**
 * ctk_text_buffer_get_copy_target_list:
 * @buffer: a #CtkTextBuffer
 *
 * This function returns the list of targets this text buffer can
 * provide for copying and as DND source. The targets in the list are
 * added with @info values from the #CtkTextBufferTargetInfo enum,
 * using ctk_target_list_add_rich_text_targets() and
 * ctk_target_list_add_text_targets().
 *
 * Returns: (transfer none): the #CtkTargetList
 *
 * Since: 2.10
 **/
CtkTargetList *
ctk_text_buffer_get_copy_target_list (CtkTextBuffer *buffer)
{
  CtkTextBufferPrivate *priv;

  g_return_val_if_fail (CTK_IS_TEXT_BUFFER (buffer), NULL);

  priv = buffer->priv;

  if (! priv->copy_target_list)
    priv->copy_target_list =
      ctk_text_buffer_get_target_list (buffer, FALSE,
                                       &priv->copy_target_entries,
                                       &priv->n_copy_target_entries);

  return priv->copy_target_list;
}

/**
 * ctk_text_buffer_get_paste_target_list:
 * @buffer: a #CtkTextBuffer
 *
 * This function returns the list of targets this text buffer supports
 * for pasting and as DND destination. The targets in the list are
 * added with @info values from the #CtkTextBufferTargetInfo enum,
 * using ctk_target_list_add_rich_text_targets() and
 * ctk_target_list_add_text_targets().
 *
 * Returns: (transfer none): the #CtkTargetList
 *
 * Since: 2.10
 **/
CtkTargetList *
ctk_text_buffer_get_paste_target_list (CtkTextBuffer *buffer)
{
  CtkTextBufferPrivate *priv;

  g_return_val_if_fail (CTK_IS_TEXT_BUFFER (buffer), NULL);

  priv = buffer->priv;

  if (! priv->paste_target_list)
    priv->paste_target_list =
      ctk_text_buffer_get_target_list (buffer, TRUE,
                                       &priv->paste_target_entries,
                                       &priv->n_paste_target_entries);

  return priv->paste_target_list;
}

/*
 * Logical attribute cache
 */

#define ATTR_CACHE_SIZE 2

typedef struct _CacheEntry CacheEntry;
struct _CacheEntry
{
  gint line;
  gint char_len;
  PangoLogAttr *attrs;
};

struct _CtkTextLogAttrCache
{
  gint chars_changed_stamp;
  CacheEntry entries[ATTR_CACHE_SIZE];
};

static void
free_log_attr_cache (CtkTextLogAttrCache *cache)
{
  gint i;

  for (i = 0; i < ATTR_CACHE_SIZE; i++)
    g_free (cache->entries[i].attrs);

  g_slice_free (CtkTextLogAttrCache, cache);
}

static void
clear_log_attr_cache (CtkTextLogAttrCache *cache)
{
  gint i;

  for (i = 0; i < ATTR_CACHE_SIZE; i++)
    {
      g_free (cache->entries[i].attrs);
      cache->entries[i].attrs = NULL;
    }
}

static PangoLogAttr*
compute_log_attrs (const CtkTextIter *iter,
                   gint              *char_lenp)
{
  CtkTextIter start;
  CtkTextIter end;
  gchar *paragraph;
  gint char_len, byte_len;
  PangoLogAttr *attrs = NULL;
  
  start = *iter;
  end = *iter;

  ctk_text_iter_set_line_offset (&start, 0);
  ctk_text_iter_forward_line (&end);

  paragraph = ctk_text_iter_get_slice (&start, &end);
  char_len = g_utf8_strlen (paragraph, -1);
  byte_len = strlen (paragraph);

  if (char_lenp != NULL)
    *char_lenp = char_len;

  attrs = g_new (PangoLogAttr, char_len + 1);

  /* FIXME we need to follow PangoLayout and allow different language
   * tags within the paragraph
   */
  pango_get_log_attrs (paragraph, byte_len, -1,
		       ctk_text_iter_get_language (&start),
                       attrs,
                       char_len + 1);
  
  g_free (paragraph);

  return attrs;
}

/* The return value from this is valid until you call this a second time.
 * Returns (char_len + 1) PangoLogAttr's, one for each text position.
 */
const PangoLogAttr *
_ctk_text_buffer_get_line_log_attrs (CtkTextBuffer     *buffer,
                                     const CtkTextIter *anywhere_in_line,
                                     gint              *char_len)
{
  CtkTextBufferPrivate *priv;
  gint line;
  CtkTextLogAttrCache *cache;
  gint i;
  
  g_return_val_if_fail (CTK_IS_TEXT_BUFFER (buffer), NULL);
  g_return_val_if_fail (anywhere_in_line != NULL, NULL);

  priv = buffer->priv;

  /* FIXME we also need to recompute log attrs if the language tag at
   * the start of a paragraph changes
   */
  
  if (priv->log_attr_cache == NULL)
    {
      priv->log_attr_cache = g_slice_new0 (CtkTextLogAttrCache);
      priv->log_attr_cache->chars_changed_stamp =
        _ctk_text_btree_get_chars_changed_stamp (get_btree (buffer));
    }
  else if (priv->log_attr_cache->chars_changed_stamp !=
           _ctk_text_btree_get_chars_changed_stamp (get_btree (buffer)))
    {
      clear_log_attr_cache (priv->log_attr_cache);
    }
  
  cache = priv->log_attr_cache;
  line = ctk_text_iter_get_line (anywhere_in_line);

  for (i = 0; i < ATTR_CACHE_SIZE; i++)
    {
      if (cache->entries[i].attrs != NULL &&
          cache->entries[i].line == line)
        {
          if (char_len != NULL)
            *char_len = cache->entries[i].char_len;
          return cache->entries[i].attrs;
        }
    }

  /* Not in cache; open up the first cache entry */
  g_free (cache->entries[ATTR_CACHE_SIZE-1].attrs);

  memmove (cache->entries + 1, cache->entries,
           sizeof (CacheEntry) * (ATTR_CACHE_SIZE - 1));

  cache->entries[0].line = line;
  cache->entries[0].attrs = compute_log_attrs (anywhere_in_line,
                                               &cache->entries[0].char_len);

  if (char_len != NULL)
    *char_len = cache->entries[0].char_len;

  return cache->entries[0].attrs;
}

void
_ctk_text_buffer_notify_will_remove_tag (CtkTextBuffer *buffer,
                                         CtkTextTag    *tag)
{
  /* This removes tag from the buffer, but DOESN'T emit the
   * remove-tag signal, because we can't afford to have user
   * code messing things up at this point; the tag MUST be removed
   * entirely.
   */
  if (buffer->priv->btree)
    _ctk_text_btree_notify_will_remove_tag (buffer->priv->btree, tag);
}

/*
 * Debug spew
 */

void
_ctk_text_buffer_spew (CtkTextBuffer *buffer)
{
  _ctk_text_btree_spew (get_btree (buffer));
}

void
_ctk_text_buffer_get_text_before (CtkTextBuffer   *buffer,
                                  AtkTextBoundary  boundary_type,
                                  CtkTextIter     *position,
                                  CtkTextIter     *start,
                                  CtkTextIter     *end)
{
  gint line_number;

  *start = *position;
  *end = *start;

  switch (boundary_type)
    {
    case ATK_TEXT_BOUNDARY_CHAR:
      ctk_text_iter_backward_char (start);
      break;

    case ATK_TEXT_BOUNDARY_WORD_START:
      if (!ctk_text_iter_starts_word (start))
        ctk_text_iter_backward_word_start (start);
      *end = *start;
      ctk_text_iter_backward_word_start (start);
      break;

    case ATK_TEXT_BOUNDARY_WORD_END:
      if (ctk_text_iter_inside_word (start) &&
          !ctk_text_iter_starts_word (start))
        ctk_text_iter_backward_word_start (start);
      while (!ctk_text_iter_ends_word (start))
        {
          if (!ctk_text_iter_backward_char (start))
            break;
        }
      *end = *start;
      ctk_text_iter_backward_word_start (start);
      while (!ctk_text_iter_ends_word (start))
        {
          if (!ctk_text_iter_backward_char (start))
            break;
        }
      break;

    case ATK_TEXT_BOUNDARY_SENTENCE_START:
      if (!ctk_text_iter_starts_sentence (start))
        ctk_text_iter_backward_sentence_start (start);
      *end = *start;
      ctk_text_iter_backward_sentence_start (start);
      break;

    case ATK_TEXT_BOUNDARY_SENTENCE_END:
      if (ctk_text_iter_inside_sentence (start) &&
          !ctk_text_iter_starts_sentence (start))
        ctk_text_iter_backward_sentence_start (start);
      while (!ctk_text_iter_ends_sentence (start))
        {
          if (!ctk_text_iter_backward_char (start))
            break;
        }
      *end = *start;
      ctk_text_iter_backward_sentence_start (start);
      while (!ctk_text_iter_ends_sentence (start))
        {
          if (!ctk_text_iter_backward_char (start))
            break;
        }
      break;

    case ATK_TEXT_BOUNDARY_LINE_START:
      line_number = ctk_text_iter_get_line (start);
      if (line_number == 0)
        {
          ctk_text_buffer_get_iter_at_offset (buffer, start, 0);
        }
      else
        {
          ctk_text_iter_backward_line (start);
          ctk_text_iter_forward_line (start);
        }
      *end = *start;
      ctk_text_iter_backward_line (start);
      break;

    case ATK_TEXT_BOUNDARY_LINE_END:
      line_number = ctk_text_iter_get_line (start);
      if (line_number == 0)
        {
          ctk_text_buffer_get_iter_at_offset (buffer, start, 0);
          *end = *start;
        }
      else
        {
          ctk_text_iter_backward_line (start);
          *end = *start;
          while (!ctk_text_iter_ends_line (start))
            {
              if (!ctk_text_iter_backward_char (start))
                break;
            }
          ctk_text_iter_forward_to_line_end (end);
        }
      break;
    }
}

void
_ctk_text_buffer_get_text_at (CtkTextBuffer   *buffer,
                              AtkTextBoundary  boundary_type,
                              CtkTextIter     *position,
                              CtkTextIter     *start,
                              CtkTextIter     *end)
{
  gint line_number;

  *start = *position;
  *end = *start;

  switch (boundary_type)
    {
    case ATK_TEXT_BOUNDARY_CHAR:
      ctk_text_iter_forward_char (end);
      break;

    case ATK_TEXT_BOUNDARY_WORD_START:
      if (!ctk_text_iter_starts_word (start))
        ctk_text_iter_backward_word_start (start);
      if (ctk_text_iter_inside_word (end))
        ctk_text_iter_forward_word_end (end);
      while (!ctk_text_iter_starts_word (end))
        {
          if (!ctk_text_iter_forward_char (end))
            break;
        }
      break;

    case ATK_TEXT_BOUNDARY_WORD_END:
      if (ctk_text_iter_inside_word (start) &&
          !ctk_text_iter_starts_word (start))
        ctk_text_iter_backward_word_start (start);
      while (!ctk_text_iter_ends_word (start))
        {
          if (!ctk_text_iter_backward_char (start))
            break;
        }
      ctk_text_iter_forward_word_end (end);
      break;

    case ATK_TEXT_BOUNDARY_SENTENCE_START:
      if (!ctk_text_iter_starts_sentence (start))
        ctk_text_iter_backward_sentence_start (start);
      if (ctk_text_iter_inside_sentence (end))
        ctk_text_iter_forward_sentence_end (end);
      while (!ctk_text_iter_starts_sentence (end))
        {
          if (!ctk_text_iter_forward_char (end))
            break;
        }
      break;

    case ATK_TEXT_BOUNDARY_SENTENCE_END:
      if (ctk_text_iter_inside_sentence (start) &&
          !ctk_text_iter_starts_sentence (start))
        ctk_text_iter_backward_sentence_start (start);
      while (!ctk_text_iter_ends_sentence (start))
        {
          if (!ctk_text_iter_backward_char (start))
            break;
        }
      ctk_text_iter_forward_sentence_end (end);
      break;

    case ATK_TEXT_BOUNDARY_LINE_START:
      line_number = ctk_text_iter_get_line (start);
      if (line_number == 0)
        {
          ctk_text_buffer_get_iter_at_offset (buffer, start, 0);
        }
      else
        {
          ctk_text_iter_backward_line (start);
          ctk_text_iter_forward_line (start);
        }
      ctk_text_iter_forward_line (end);
      break;

    case ATK_TEXT_BOUNDARY_LINE_END:
      line_number = ctk_text_iter_get_line (start);
      if (line_number == 0)
        {
          ctk_text_buffer_get_iter_at_offset (buffer, start, 0);
        }
      else
        {
          ctk_text_iter_backward_line (start);
          ctk_text_iter_forward_line (start);
        }
      while (!ctk_text_iter_ends_line (start))
        {
          if (!ctk_text_iter_backward_char (start))
            break;
        }
      ctk_text_iter_forward_to_line_end (end);
      break;
   }
}

void
_ctk_text_buffer_get_text_after (CtkTextBuffer   *buffer,
                                 AtkTextBoundary  boundary_type,
                                 CtkTextIter     *position,
                                 CtkTextIter     *start,
                                 CtkTextIter     *end)
{
  *start = *position;
  *end = *start;

  switch (boundary_type)
    {
    case ATK_TEXT_BOUNDARY_CHAR:
      ctk_text_iter_forward_char (start);
      ctk_text_iter_forward_chars (end, 2);
      break;

    case ATK_TEXT_BOUNDARY_WORD_START:
      if (ctk_text_iter_inside_word (end))
        ctk_text_iter_forward_word_end (end);
      while (!ctk_text_iter_starts_word (end))
        {
          if (!ctk_text_iter_forward_char (end))
            break;
        }
      *start = *end;
      if (!ctk_text_iter_is_end (end))
        {
          ctk_text_iter_forward_word_end (end);
          while (!ctk_text_iter_starts_word (end))
            {
              if (!ctk_text_iter_forward_char (end))
                break;
            }
        }
      break;

    case ATK_TEXT_BOUNDARY_WORD_END:
      ctk_text_iter_forward_word_end (end);
      *start = *end;
      if (!ctk_text_iter_is_end (end))
        ctk_text_iter_forward_word_end (end);
      break;

    case ATK_TEXT_BOUNDARY_SENTENCE_START:
      if (ctk_text_iter_inside_sentence (end))
        ctk_text_iter_forward_sentence_end (end);
      while (!ctk_text_iter_starts_sentence (end))
        {
          if (!ctk_text_iter_forward_char (end))
            break;
        }
      *start = *end;
      if (!ctk_text_iter_is_end (end))
        {
          ctk_text_iter_forward_sentence_end (end);
          while (!ctk_text_iter_starts_sentence (end))
            {
              if (!ctk_text_iter_forward_char (end))
                break;
            }
        }
      break;

    case ATK_TEXT_BOUNDARY_SENTENCE_END:
      ctk_text_iter_forward_sentence_end (end);
      *start = *end;
      if (!ctk_text_iter_is_end (end))
        ctk_text_iter_forward_sentence_end (end);
      break;

    case ATK_TEXT_BOUNDARY_LINE_START:
      ctk_text_iter_forward_line (end);
      *start = *end;
      ctk_text_iter_forward_line (end);
      break;

    case ATK_TEXT_BOUNDARY_LINE_END:
      ctk_text_iter_forward_line (start);
      *end = *start;
      if (!ctk_text_iter_is_end (start))
        {
          while (!ctk_text_iter_ends_line (start))
            {
              if (!ctk_text_iter_backward_char (start))
                break;
            }
          ctk_text_iter_forward_to_line_end (end);
        }
      break;
    }
}

static CtkTextTag *
get_tag_for_attributes (PangoAttrIterator *iter)
{
  PangoAttribute *attr;
  CtkTextTag *tag;

  tag = ctk_text_tag_new (NULL);

  attr = pango_attr_iterator_get (iter, PANGO_ATTR_LANGUAGE);
  if (attr)
    g_object_set (tag, "language", pango_language_to_string (((PangoAttrLanguage*)attr)->value), NULL);

  attr = pango_attr_iterator_get (iter, PANGO_ATTR_FAMILY);
  if (attr)
    g_object_set (tag, "family", ((PangoAttrString*)attr)->value, NULL);

  attr = pango_attr_iterator_get (iter, PANGO_ATTR_STYLE);
  if (attr)
    g_object_set (tag, "style", ((PangoAttrInt*)attr)->value, NULL);

  attr = pango_attr_iterator_get (iter, PANGO_ATTR_WEIGHT);
  if (attr)
    g_object_set (tag, "weight", ((PangoAttrInt*)attr)->value, NULL);

  attr = pango_attr_iterator_get (iter, PANGO_ATTR_VARIANT);
  if (attr)
    g_object_set (tag, "variant", ((PangoAttrInt*)attr)->value, NULL);

  attr = pango_attr_iterator_get (iter, PANGO_ATTR_STRETCH);
  if (attr)
    g_object_set (tag, "stretch", ((PangoAttrInt*)attr)->value, NULL);

  attr = pango_attr_iterator_get (iter, PANGO_ATTR_SIZE);
  if (attr)
    g_object_set (tag, "size", ((PangoAttrInt*)attr)->value, NULL);

  attr = pango_attr_iterator_get (iter, PANGO_ATTR_FONT_DESC);
  if (attr)
    g_object_set (tag, "font-desc", ((PangoAttrFontDesc*)attr)->desc, NULL);

  attr = pango_attr_iterator_get (iter, PANGO_ATTR_FOREGROUND);
  if (attr)
    {
      PangoColor *color;
      GdkRGBA rgba;
     
      color = &((PangoAttrColor*)attr)->color;
      rgba.red = color->red / 65535.;
      rgba.green = color->green / 65535.;
      rgba.blue = color->blue / 65535.;
      rgba.alpha = 1.;
      g_object_set (tag, "foreground-rgba", &rgba, NULL);
    };

  attr = pango_attr_iterator_get (iter, PANGO_ATTR_BACKGROUND);
  if (attr)
    {
      PangoColor *color;
      GdkRGBA rgba;
     
      color = &((PangoAttrColor*)attr)->color;
      rgba.red = color->red / 65535.;
      rgba.green = color->green / 65535.;
      rgba.blue = color->blue / 65535.;
      rgba.alpha = 1.;
      g_object_set (tag, "background-rgba", &rgba, NULL);
    };

  attr = pango_attr_iterator_get (iter, PANGO_ATTR_UNDERLINE);
  if (attr)
    g_object_set (tag, "underline", ((PangoAttrInt*)attr)->value, NULL);

  attr = pango_attr_iterator_get (iter, PANGO_ATTR_UNDERLINE_COLOR);
  if (attr)
    {
      PangoColor *color;
      GdkRGBA rgba;

      color = &((PangoAttrColor*)attr)->color;
      rgba.red = color->red / 65535.;
      rgba.green = color->green / 65535.;
      rgba.blue = color->blue / 65535.;
      rgba.alpha = 1.;
      g_object_set (tag, "underline-rgba", &rgba, NULL);
    }

  attr = pango_attr_iterator_get (iter, PANGO_ATTR_STRIKETHROUGH);
  if (attr)
    g_object_set (tag, "strikethrough", (gboolean) (((PangoAttrInt*)attr)->value != 0), NULL);

  attr = pango_attr_iterator_get (iter, PANGO_ATTR_STRIKETHROUGH_COLOR);
  if (attr)
    {
      PangoColor *color;
      GdkRGBA rgba;

      color = &((PangoAttrColor*)attr)->color;
      rgba.red = color->red / 65535.;
      rgba.green = color->green / 65535.;
      rgba.blue = color->blue / 65535.;
      rgba.alpha = 1.;
      g_object_set (tag, "strikethrough-rgba", &rgba, NULL);
    }

  attr = pango_attr_iterator_get (iter, PANGO_ATTR_RISE);
  if (attr)
    g_object_set (tag, "rise", ((PangoAttrInt*)attr)->value, NULL);

  attr = pango_attr_iterator_get (iter, PANGO_ATTR_SCALE);
  if (attr)
    g_object_set (tag, "scale", ((PangoAttrFloat*)attr)->value, NULL);

  attr = pango_attr_iterator_get (iter, PANGO_ATTR_FALLBACK);
  if (attr)
    g_object_set (tag, "fallback", (gboolean) (((PangoAttrInt*)attr)->value != 0), NULL);

  attr = pango_attr_iterator_get (iter, PANGO_ATTR_LETTER_SPACING);
  if (attr)
    g_object_set (tag, "letter-spacing", ((PangoAttrInt*)attr)->value, NULL);

  attr = pango_attr_iterator_get (iter, PANGO_ATTR_FONT_FEATURES);
  if (attr)
    g_object_set (tag, "font-features", ((PangoAttrString*)attr)->value, NULL);

  return tag;
}

static void
ctk_text_buffer_insert_with_attributes (CtkTextBuffer *buffer,
                                        CtkTextIter   *iter,
                                        const gchar   *text,
                                        PangoAttrList *attributes)
{
  CtkTextMark *mark;
  PangoAttrIterator *attr;
  CtkTextTagTable *tags;

  g_return_if_fail (CTK_IS_TEXT_BUFFER (buffer));

  if (!attributes)
    {
      ctk_text_buffer_insert (buffer, iter, text, -1);
      return;
    }

  /* create mark with right gravity */
  mark = ctk_text_buffer_create_mark (buffer, NULL, iter, FALSE);
  attr = pango_attr_list_get_iterator (attributes);
  tags = ctk_text_buffer_get_tag_table (buffer);

  do
    {
      CtkTextTag *tag;
      gint start, end;

      pango_attr_iterator_range (attr, &start, &end);

      if (end == G_MAXINT) /* last chunk */
        end = start - 1; /* resulting in -1 to be passed to _insert */

      tag = get_tag_for_attributes (attr);
      ctk_text_tag_table_add (tags, tag);

      ctk_text_buffer_insert_with_tags (buffer, iter, text + start, end - start, tag, NULL);

      ctk_text_buffer_get_iter_at_mark (buffer, iter, mark);
    }
  while (pango_attr_iterator_next (attr));
  
  ctk_text_buffer_delete_mark (buffer, mark);
  pango_attr_iterator_destroy (attr);
} 

/**
 * ctk_text_buffer_insert_markup:
 * @buffer: a #CtkTextBuffer
 * @iter: location to insert the markup
 * @markup: a nul-terminated UTF-8 string containing [Pango markup][PangoMarkupFormat]
 * @len: length of @markup in bytes, or -1
 *
 * Inserts the text in @markup at position @iter. @markup will be inserted
 * in its entirety and must be nul-terminated and valid UTF-8. Emits the
 * #CtkTextBuffer::insert-text signal, possibly multiple times; insertion
 * actually occurs in the default handler for the signal. @iter will point
 * to the end of the inserted text on return.
 *
 * Since: 3.16
 */
void
ctk_text_buffer_insert_markup (CtkTextBuffer *buffer,
                               CtkTextIter   *iter,
                               const gchar   *markup,
                               gint           len)
{
  PangoAttrList *attributes;
  gchar *text;
  GError *error = NULL;

  if (!pango_parse_markup (markup, len, 0, &attributes, &text, NULL, &error))
    {
      g_warning ("Invalid markup string: %s", error->message);
      g_error_free (error);
      return;
    }

  ctk_text_buffer_insert_with_attributes (buffer, iter, text, attributes);

  pango_attr_list_unref (attributes);
  g_free (text); 
}
