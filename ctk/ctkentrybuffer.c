/* ctkentrybuffer.c
 * Copyright (C) 2009  Stefan Walter <stef@memberwebs.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "ctkentrybuffer.h"
#include "ctkintl.h"
#include "ctkmarshalers.h"
#include "ctkprivate.h"
#include "ctkwidget.h"

#include <cdk/cdk.h>

#include <string.h>

/**
 * SECTION:ctkentrybuffer
 * @title: CtkEntryBuffer
 * @short_description: Text buffer for CtkEntry
 *
 * The #CtkEntryBuffer class contains the actual text displayed in a
 * #CtkEntry widget.
 *
 * A single #CtkEntryBuffer object can be shared by multiple #CtkEntry
 * widgets which will then share the same text content, but not the cursor
 * position, visibility attributes, icon etc.
 *
 * #CtkEntryBuffer may be derived from. Such a derived class might allow
 * text to be stored in an alternate location, such as non-pageable memory,
 * useful in the case of important passwords. Or a derived class could 
 * integrate with an application’s concept of undo/redo.
 *
 * Since: 2.18
 */

/* Initial size of buffer, in bytes */
#define MIN_SIZE 16

enum {
  PROP_0,
  PROP_TEXT,
  PROP_LENGTH,
  PROP_MAX_LENGTH,
  NUM_PROPERTIES
};

static GParamSpec *entry_buffer_props[NUM_PROPERTIES] = { NULL, };

enum {
  INSERTED_TEXT,
  DELETED_TEXT,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

struct _CtkEntryBufferPrivate
{
  /* Only valid if this class is not derived */
  gchar *normal_text;
  gsize  normal_text_size;
  gsize  normal_text_bytes;
  guint  normal_text_chars;

  gint   max_length;
};

G_DEFINE_TYPE_WITH_PRIVATE (CtkEntryBuffer, ctk_entry_buffer, G_TYPE_OBJECT)

/* --------------------------------------------------------------------------------
 * DEFAULT IMPLEMENTATIONS OF TEXT BUFFER
 *
 * These may be overridden by a derived class, behavior may be changed etc...
 * The normal_text and normal_text_xxxx fields may not be valid when
 * this class is derived from.
 */

/* Overwrite a memory that might contain sensitive information. */
static void
trash_area (gchar *area,
            gsize  len)
{
  volatile gchar *varea = (volatile gchar *)area;
  while (len-- > 0)
    *varea++ = 0;
}

static const gchar*
ctk_entry_buffer_normal_get_text (CtkEntryBuffer *buffer,
                                  gsize          *n_bytes)
{
  if (n_bytes)
    *n_bytes = buffer->priv->normal_text_bytes;
  if (!buffer->priv->normal_text)
      return "";
  return buffer->priv->normal_text;
}

static guint
ctk_entry_buffer_normal_get_length (CtkEntryBuffer *buffer)
{
  return buffer->priv->normal_text_chars;
}

static guint
ctk_entry_buffer_normal_insert_text (CtkEntryBuffer *buffer,
                                     guint           position,
                                     const gchar    *chars,
                                     guint           n_chars)
{
  CtkEntryBufferPrivate *pv = buffer->priv;
  gsize prev_size;
  gsize n_bytes;
  gsize at;

  n_bytes = g_utf8_offset_to_pointer (chars, n_chars) - chars;

  /* Need more memory */
  if (n_bytes + pv->normal_text_bytes + 1 > pv->normal_text_size)
    {
      gchar *et_new;

      prev_size = pv->normal_text_size;

      /* Calculate our new buffer size */
      while (n_bytes + pv->normal_text_bytes + 1 > pv->normal_text_size)
        {
          if (pv->normal_text_size == 0)
            pv->normal_text_size = MIN_SIZE;
          else
            {
              if (2 * pv->normal_text_size < CTK_ENTRY_BUFFER_MAX_SIZE)
                pv->normal_text_size *= 2;
              else
                {
                  pv->normal_text_size = CTK_ENTRY_BUFFER_MAX_SIZE;
                  if (n_bytes > pv->normal_text_size - pv->normal_text_bytes - 1)
                    {
                      n_bytes = pv->normal_text_size - pv->normal_text_bytes - 1;
                      n_bytes = g_utf8_find_prev_char (chars, chars + n_bytes + 1) - chars;
                      n_chars = g_utf8_strlen (chars, n_bytes);
                    }
                  break;
                }
            }
        }

      /* Could be a password, so can't leave stuff in memory. */
      et_new = g_malloc (pv->normal_text_size);
      memcpy (et_new, pv->normal_text, MIN (prev_size, pv->normal_text_size));
      trash_area (pv->normal_text, prev_size);
      g_free (pv->normal_text);
      pv->normal_text = et_new;
    }

  /* Actual text insertion */
  at = g_utf8_offset_to_pointer (pv->normal_text, position) - pv->normal_text;
  memmove (pv->normal_text + at + n_bytes, pv->normal_text + at, pv->normal_text_bytes - at);
  memcpy (pv->normal_text + at, chars, n_bytes);

  /* Book keeping */
  pv->normal_text_bytes += n_bytes;
  pv->normal_text_chars += n_chars;
  pv->normal_text[pv->normal_text_bytes] = '\0';

  ctk_entry_buffer_emit_inserted_text (buffer, position, chars, n_chars);
  return n_chars;
}

static guint
ctk_entry_buffer_normal_delete_text (CtkEntryBuffer *buffer,
                                     guint           position,
                                     guint           n_chars)
{
  CtkEntryBufferPrivate *pv = buffer->priv;
  gsize start, end;

  if (position > pv->normal_text_chars)
    position = pv->normal_text_chars;
  if (position + n_chars > pv->normal_text_chars)
    n_chars = pv->normal_text_chars - position;

  if (n_chars > 0)
    {
      start = g_utf8_offset_to_pointer (pv->normal_text, position) - pv->normal_text;
      end = g_utf8_offset_to_pointer (pv->normal_text, position + n_chars) - pv->normal_text;

      memmove (pv->normal_text + start, pv->normal_text + end, pv->normal_text_bytes + 1 - end);
      pv->normal_text_chars -= n_chars;
      pv->normal_text_bytes -= (end - start);

      /*
       * Could be a password, make sure we don't leave anything sensitive after
       * the terminating zero.  Note, that the terminating zero already trashed
       * one byte.
       */
      trash_area (pv->normal_text + pv->normal_text_bytes + 1, end - start - 1);

      ctk_entry_buffer_emit_deleted_text (buffer, position, n_chars);
    }

  return n_chars;
}

/* --------------------------------------------------------------------------------
 *
 */

static void
ctk_entry_buffer_real_inserted_text (CtkEntryBuffer *buffer,
                                     guint           position G_GNUC_UNUSED,
                                     const gchar    *chars G_GNUC_UNUSED,
                                     guint           n_chars G_GNUC_UNUSED)
{
  g_object_notify_by_pspec (G_OBJECT (buffer), entry_buffer_props[PROP_TEXT]);
  g_object_notify_by_pspec (G_OBJECT (buffer), entry_buffer_props[PROP_LENGTH]);
}

static void
ctk_entry_buffer_real_deleted_text (CtkEntryBuffer *buffer,
                                    guint           position G_GNUC_UNUSED,
                                    guint           n_chars G_GNUC_UNUSED)
{
  g_object_notify_by_pspec (G_OBJECT (buffer), entry_buffer_props[PROP_TEXT]);
  g_object_notify_by_pspec (G_OBJECT (buffer), entry_buffer_props[PROP_LENGTH]);
}

/* --------------------------------------------------------------------------------
 *
 */

static void
ctk_entry_buffer_init (CtkEntryBuffer *buffer)
{
  CtkEntryBufferPrivate *pv;

  pv = buffer->priv = ctk_entry_buffer_get_instance_private (buffer);

  pv->normal_text = NULL;
  pv->normal_text_chars = 0;
  pv->normal_text_bytes = 0;
  pv->normal_text_size = 0;
}

static void
ctk_entry_buffer_finalize (GObject *obj)
{
  CtkEntryBuffer *buffer = CTK_ENTRY_BUFFER (obj);
  CtkEntryBufferPrivate *pv = buffer->priv;

  if (pv->normal_text)
    {
      trash_area (pv->normal_text, pv->normal_text_size);
      g_free (pv->normal_text);
      pv->normal_text = NULL;
      pv->normal_text_bytes = pv->normal_text_size = 0;
      pv->normal_text_chars = 0;
    }

  G_OBJECT_CLASS (ctk_entry_buffer_parent_class)->finalize (obj);
}

static void
ctk_entry_buffer_set_property (GObject      *obj,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  CtkEntryBuffer *buffer = CTK_ENTRY_BUFFER (obj);

  switch (prop_id)
    {
    case PROP_TEXT:
      ctk_entry_buffer_set_text (buffer, g_value_get_string (value), -1);
      break;
    case PROP_MAX_LENGTH:
      ctk_entry_buffer_set_max_length (buffer, g_value_get_int (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
      break;
    }
}

static void
ctk_entry_buffer_get_property (GObject    *obj,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  CtkEntryBuffer *buffer = CTK_ENTRY_BUFFER (obj);

  switch (prop_id)
    {
    case PROP_TEXT:
      g_value_set_string (value, ctk_entry_buffer_get_text (buffer));
      break;
    case PROP_LENGTH:
      g_value_set_uint (value, ctk_entry_buffer_get_length (buffer));
      break;
    case PROP_MAX_LENGTH:
      g_value_set_int (value, ctk_entry_buffer_get_max_length (buffer));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
      break;
    }
}

static void
ctk_entry_buffer_class_init (CtkEntryBufferClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = ctk_entry_buffer_finalize;
  gobject_class->set_property = ctk_entry_buffer_set_property;
  gobject_class->get_property = ctk_entry_buffer_get_property;

  klass->get_text = ctk_entry_buffer_normal_get_text;
  klass->get_length = ctk_entry_buffer_normal_get_length;
  klass->insert_text = ctk_entry_buffer_normal_insert_text;
  klass->delete_text = ctk_entry_buffer_normal_delete_text;

  klass->inserted_text = ctk_entry_buffer_real_inserted_text;
  klass->deleted_text = ctk_entry_buffer_real_deleted_text;

  /**
   * CtkEntryBuffer:text:
   *
   * The contents of the buffer.
   *
   * Since: 2.18
   */
  entry_buffer_props[PROP_TEXT] =
      g_param_spec_string ("text",
                           P_("Text"),
                           P_("The contents of the buffer"),
                           "",
                           CTK_PARAM_READWRITE);

  /**
   * CtkEntryBuffer:length:
   *
   * The length (in characters) of the text in buffer.
   *
   * Since: 2.18
   */
   entry_buffer_props[PROP_LENGTH] =
       g_param_spec_uint ("length",
                          P_("Text length"),
                          P_("Length of the text currently in the buffer"),
                          0, CTK_ENTRY_BUFFER_MAX_SIZE, 0,
                          CTK_PARAM_READABLE);

  /**
   * CtkEntryBuffer:max-length:
   *
   * The maximum length (in characters) of the text in the buffer.
   *
   * Since: 2.18
   */
  entry_buffer_props[PROP_MAX_LENGTH] =
      g_param_spec_int ("max-length",
                        P_("Maximum length"),
                        P_("Maximum number of characters for this entry. Zero if no maximum"),
                        0, CTK_ENTRY_BUFFER_MAX_SIZE, 0,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class, NUM_PROPERTIES, entry_buffer_props);

  /**
   * CtkEntryBuffer::inserted-text:
   * @buffer: a #CtkEntryBuffer
   * @position: the position the text was inserted at.
   * @chars: The text that was inserted.
   * @n_chars: The number of characters that were inserted.
   *
   * This signal is emitted after text is inserted into the buffer.
   *
   * Since: 2.18
   */
  signals[INSERTED_TEXT] = g_signal_new (I_("inserted-text"),
                                         CTK_TYPE_ENTRY_BUFFER,
                                         G_SIGNAL_RUN_FIRST,
                                         G_STRUCT_OFFSET (CtkEntryBufferClass, inserted_text),
                                         NULL, NULL,
                                         _ctk_marshal_VOID__UINT_STRING_UINT,
                                         G_TYPE_NONE, 3,
                                         G_TYPE_UINT,
                                         G_TYPE_STRING,
                                         G_TYPE_UINT);

  /**
   * CtkEntryBuffer::deleted-text:
   * @buffer: a #CtkEntryBuffer
   * @position: the position the text was deleted at.
   * @n_chars: The number of characters that were deleted.
   *
   * This signal is emitted after text is deleted from the buffer.
   *
   * Since: 2.18
   */
  signals[DELETED_TEXT] =  g_signal_new (I_("deleted-text"),
                                         CTK_TYPE_ENTRY_BUFFER,
                                         G_SIGNAL_RUN_FIRST,
                                         G_STRUCT_OFFSET (CtkEntryBufferClass, deleted_text),
                                         NULL, NULL,
                                         _ctk_marshal_VOID__UINT_UINT,
                                         G_TYPE_NONE, 2,
                                         G_TYPE_UINT,
                                         G_TYPE_UINT);
}

/* --------------------------------------------------------------------------------
 *
 */

/**
 * ctk_entry_buffer_new:
 * @initial_chars: (allow-none): initial buffer text, or %NULL
 * @n_initial_chars: number of characters in @initial_chars, or -1
 *
 * Create a new CtkEntryBuffer object.
 *
 * Optionally, specify initial text to set in the buffer.
 *
 * Returns: A new CtkEntryBuffer object.
 *
 * Since: 2.18
 **/
CtkEntryBuffer*
ctk_entry_buffer_new (const gchar *initial_chars,
                      gint         n_initial_chars)
{
  CtkEntryBuffer *buffer = g_object_new (CTK_TYPE_ENTRY_BUFFER, NULL);
  if (initial_chars)
    ctk_entry_buffer_set_text (buffer, initial_chars, n_initial_chars);
  return buffer;
}

/**
 * ctk_entry_buffer_get_length:
 * @buffer: a #CtkEntryBuffer
 *
 * Retrieves the length in characters of the buffer.
 *
 * Returns: The number of characters in the buffer.
 *
 * Since: 2.18
 **/
guint
ctk_entry_buffer_get_length (CtkEntryBuffer *buffer)
{
  CtkEntryBufferClass *klass;

  g_return_val_if_fail (CTK_IS_ENTRY_BUFFER (buffer), 0);

  klass = CTK_ENTRY_BUFFER_GET_CLASS (buffer);
  g_return_val_if_fail (klass->get_length != NULL, 0);

  return (*klass->get_length) (buffer);
}

/**
 * ctk_entry_buffer_get_bytes:
 * @buffer: a #CtkEntryBuffer
 *
 * Retrieves the length in bytes of the buffer.
 * See ctk_entry_buffer_get_length().
 *
 * Returns: The byte length of the buffer.
 *
 * Since: 2.18
 **/
gsize
ctk_entry_buffer_get_bytes (CtkEntryBuffer *buffer)
{
  CtkEntryBufferClass *klass;
  gsize bytes = 0;

  g_return_val_if_fail (CTK_IS_ENTRY_BUFFER (buffer), 0);

  klass = CTK_ENTRY_BUFFER_GET_CLASS (buffer);
  g_return_val_if_fail (klass->get_text != NULL, 0);

  (*klass->get_text) (buffer, &bytes);
  return bytes;
}

/**
 * ctk_entry_buffer_get_text:
 * @buffer: a #CtkEntryBuffer
 *
 * Retrieves the contents of the buffer.
 *
 * The memory pointer returned by this call will not change
 * unless this object emits a signal, or is finalized.
 *
 * Returns: a pointer to the contents of the widget as a
 *      string. This string points to internally allocated
 *      storage in the buffer and must not be freed, modified or
 *      stored.
 *
 * Since: 2.18
 **/
const gchar*
ctk_entry_buffer_get_text (CtkEntryBuffer *buffer)
{
  CtkEntryBufferClass *klass;

  g_return_val_if_fail (CTK_IS_ENTRY_BUFFER (buffer), NULL);

  klass = CTK_ENTRY_BUFFER_GET_CLASS (buffer);
  g_return_val_if_fail (klass->get_text != NULL, NULL);

  return (*klass->get_text) (buffer, NULL);
}

/**
 * ctk_entry_buffer_set_text:
 * @buffer: a #CtkEntryBuffer
 * @chars: the new text
 * @n_chars: the number of characters in @text, or -1
 *
 * Sets the text in the buffer.
 *
 * This is roughly equivalent to calling ctk_entry_buffer_delete_text()
 * and ctk_entry_buffer_insert_text().
 *
 * Note that @n_chars is in characters, not in bytes.
 *
 * Since: 2.18
 **/
void
ctk_entry_buffer_set_text (CtkEntryBuffer *buffer,
                           const gchar    *chars,
                           gint            n_chars)
{
  g_return_if_fail (CTK_IS_ENTRY_BUFFER (buffer));
  g_return_if_fail (chars != NULL);

  g_object_freeze_notify (G_OBJECT (buffer));
  ctk_entry_buffer_delete_text (buffer, 0, -1);
  ctk_entry_buffer_insert_text (buffer, 0, chars, n_chars);
  g_object_thaw_notify (G_OBJECT (buffer));
}

/**
 * ctk_entry_buffer_set_max_length:
 * @buffer: a #CtkEntryBuffer
 * @max_length: the maximum length of the entry buffer, or 0 for no maximum.
 *   (other than the maximum length of entries.) The value passed in will
 *   be clamped to the range 0-65536.
 *
 * Sets the maximum allowed length of the contents of the buffer. If
 * the current contents are longer than the given length, then they
 * will be truncated to fit.
 *
 * Since: 2.18
 **/
void
ctk_entry_buffer_set_max_length (CtkEntryBuffer *buffer,
                                 gint            max_length)
{
  g_return_if_fail (CTK_IS_ENTRY_BUFFER (buffer));

  max_length = CLAMP (max_length, 0, CTK_ENTRY_BUFFER_MAX_SIZE);

  if (buffer->priv->max_length == max_length)
    return;

  if (max_length > 0 && ctk_entry_buffer_get_length (buffer) > max_length)
    ctk_entry_buffer_delete_text (buffer, max_length, -1);

  buffer->priv->max_length = max_length;
  g_object_notify_by_pspec (G_OBJECT (buffer), entry_buffer_props[PROP_MAX_LENGTH]);
}

/**
 * ctk_entry_buffer_get_max_length:
 * @buffer: a #CtkEntryBuffer
 *
 * Retrieves the maximum allowed length of the text in
 * @buffer. See ctk_entry_buffer_set_max_length().
 *
 * Returns: the maximum allowed number of characters
 *               in #CtkEntryBuffer, or 0 if there is no maximum.
 *
 * Since: 2.18
 */
gint
ctk_entry_buffer_get_max_length (CtkEntryBuffer *buffer)
{
  g_return_val_if_fail (CTK_IS_ENTRY_BUFFER (buffer), 0);
  return buffer->priv->max_length;
}

/**
 * ctk_entry_buffer_insert_text:
 * @buffer: a #CtkEntryBuffer
 * @position: the position at which to insert text.
 * @chars: the text to insert into the buffer.
 * @n_chars: the length of the text in characters, or -1
 *
 * Inserts @n_chars characters of @chars into the contents of the
 * buffer, at position @position.
 *
 * If @n_chars is negative, then characters from chars will be inserted
 * until a null-terminator is found. If @position or @n_chars are out of
 * bounds, or the maximum buffer text length is exceeded, then they are
 * coerced to sane values.
 *
 * Note that the position and length are in characters, not in bytes.
 *
 * Returns: The number of characters actually inserted.
 *
 * Since: 2.18
 */
guint
ctk_entry_buffer_insert_text (CtkEntryBuffer *buffer,
                              guint           position,
                              const gchar    *chars,
                              gint            n_chars)
{
  CtkEntryBufferClass *klass;
  CtkEntryBufferPrivate *pv;
  guint length;

  g_return_val_if_fail (CTK_IS_ENTRY_BUFFER (buffer), 0);

  length = ctk_entry_buffer_get_length (buffer);
  pv = buffer->priv;

  if (n_chars < 0)
    n_chars = g_utf8_strlen (chars, -1);

  /* Bring position into bounds */
  if (position > length)
    position = length;

  /* Make sure not entering too much data */
  if (pv->max_length > 0)
    {
      if (length >= pv->max_length)
        n_chars = 0;
      else if (length + n_chars > pv->max_length)
        n_chars -= (length + n_chars) - pv->max_length;
    }

  if (n_chars == 0)
    return 0;

  klass = CTK_ENTRY_BUFFER_GET_CLASS (buffer);
  g_return_val_if_fail (klass->insert_text != NULL, 0);

  return (*klass->insert_text) (buffer, position, chars, n_chars);
}

/**
 * ctk_entry_buffer_delete_text:
 * @buffer: a #CtkEntryBuffer
 * @position: position at which to delete text
 * @n_chars: number of characters to delete
 *
 * Deletes a sequence of characters from the buffer. @n_chars characters are
 * deleted starting at @position. If @n_chars is negative, then all characters
 * until the end of the text are deleted.
 *
 * If @position or @n_chars are out of bounds, then they are coerced to sane
 * values.
 *
 * Note that the positions are specified in characters, not bytes.
 *
 * Returns: The number of characters deleted.
 *
 * Since: 2.18
 */
guint
ctk_entry_buffer_delete_text (CtkEntryBuffer *buffer,
                              guint           position,
                              gint            n_chars)
{
  CtkEntryBufferClass *klass;
  guint length;

  g_return_val_if_fail (CTK_IS_ENTRY_BUFFER (buffer), 0);

  length = ctk_entry_buffer_get_length (buffer);
  if (n_chars < 0)
    n_chars = length;
  if (position > length)
    position = length;
  if (position + n_chars > length)
    n_chars = length - position;

  klass = CTK_ENTRY_BUFFER_GET_CLASS (buffer);
  g_return_val_if_fail (klass->delete_text != NULL, 0);

  return (*klass->delete_text) (buffer, position, n_chars);
}

/**
 * ctk_entry_buffer_emit_inserted_text:
 * @buffer: a #CtkEntryBuffer
 * @position: position at which text was inserted
 * @chars: text that was inserted
 * @n_chars: number of characters inserted
 *
 * Used when subclassing #CtkEntryBuffer
 *
 * Since: 2.18
 */
void
ctk_entry_buffer_emit_inserted_text (CtkEntryBuffer *buffer,
                                     guint           position,
                                     const gchar    *chars,
                                     guint           n_chars)
{
  g_return_if_fail (CTK_IS_ENTRY_BUFFER (buffer));
  g_signal_emit (buffer, signals[INSERTED_TEXT], 0, position, chars, n_chars);
}

/**
 * ctk_entry_buffer_emit_deleted_text:
 * @buffer: a #CtkEntryBuffer
 * @position: position at which text was deleted
 * @n_chars: number of characters deleted
 *
 * Used when subclassing #CtkEntryBuffer
 *
 * Since: 2.18
 */
void
ctk_entry_buffer_emit_deleted_text (CtkEntryBuffer *buffer,
                                    guint           position,
                                    guint           n_chars)
{
  g_return_if_fail (CTK_IS_ENTRY_BUFFER (buffer));
  g_signal_emit (buffer, signals[DELETED_TEXT], 0, position, n_chars);
}
