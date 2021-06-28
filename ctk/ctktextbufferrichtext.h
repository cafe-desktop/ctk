/* ctkrichtext.h
 *
 * Copyright (C) 2006 Imendio AB
 * Contact: Michael Natterer <mitch@imendio.com>
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

#ifndef __CTK_TEXT_BUFFER_RICH_TEXT_H__
#define __CTK_TEXT_BUFFER_RICH_TEXT_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctktextbuffer.h>

G_BEGIN_DECLS

/**
 * CtkTextBufferSerializeFunc:
 * @register_buffer: the #CtkTextBuffer for which the format is registered
 * @content_buffer: the #CtkTextBuffer to serialize
 * @start: start of the block of text to serialize
 * @end: end of the block of text to serialize
 * @length: Return location for the length of the serialized data
 * @user_data: user data that was specified when registering the format
 *
 * A function that is called to serialize the content of a text buffer.
 * It must return the serialized form of the content.
 *
 * Returns: (nullable): a newly-allocated array of guint8 which contains
 * the serialized data, or %NULL if an error occurred
 */
typedef guint8 * (* CtkTextBufferSerializeFunc)   (CtkTextBuffer     *register_buffer,
                                                   CtkTextBuffer     *content_buffer,
                                                   const CtkTextIter *start,
                                                   const CtkTextIter *end,
                                                   gsize             *length,
                                                   gpointer           user_data);

/**
 * CtkTextBufferDeserializeFunc:
 * @register_buffer: the #CtkTextBuffer the format is registered with
 * @content_buffer: the #CtkTextBuffer to deserialize into
 * @iter: insertion point for the deserialized text
 * @data: (array length=length): data to deserialize
 * @length: length of @data
 * @create_tags: %TRUE if deserializing may create tags
 * @user_data: user data that was specified when registering the format
 * @error: return location for a #GError
 *
 * A function that is called to deserialize rich text that has been
 * serialized with ctk_text_buffer_serialize(), and insert it at @iter.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 */
typedef gboolean (* CtkTextBufferDeserializeFunc) (CtkTextBuffer     *register_buffer,
                                                   CtkTextBuffer     *content_buffer,
                                                   CtkTextIter       *iter,
                                                   const guint8      *data,
                                                   gsize              length,
                                                   gboolean           create_tags,
                                                   gpointer           user_data,
                                                   GError           **error);

GDK_AVAILABLE_IN_ALL
CdkAtom   ctk_text_buffer_register_serialize_format   (CtkTextBuffer                *buffer,
                                                       const gchar                  *mime_type,
                                                       CtkTextBufferSerializeFunc    function,
                                                       gpointer                      user_data,
                                                       GDestroyNotify                user_data_destroy);
GDK_AVAILABLE_IN_ALL
CdkAtom   ctk_text_buffer_register_serialize_tagset   (CtkTextBuffer                *buffer,
                                                       const gchar                  *tagset_name);

GDK_AVAILABLE_IN_ALL
CdkAtom   ctk_text_buffer_register_deserialize_format (CtkTextBuffer                *buffer,
                                                       const gchar                  *mime_type,
                                                       CtkTextBufferDeserializeFunc  function,
                                                       gpointer                      user_data,
                                                       GDestroyNotify                user_data_destroy);
GDK_AVAILABLE_IN_ALL
CdkAtom   ctk_text_buffer_register_deserialize_tagset (CtkTextBuffer                *buffer,
                                                       const gchar                  *tagset_name);

GDK_AVAILABLE_IN_ALL
void    ctk_text_buffer_unregister_serialize_format   (CtkTextBuffer                *buffer,
                                                       CdkAtom                       format);
GDK_AVAILABLE_IN_ALL
void    ctk_text_buffer_unregister_deserialize_format (CtkTextBuffer                *buffer,
                                                       CdkAtom                       format);

GDK_AVAILABLE_IN_ALL
void     ctk_text_buffer_deserialize_set_can_create_tags (CtkTextBuffer             *buffer,
                                                          CdkAtom                    format,
                                                          gboolean                   can_create_tags);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_buffer_deserialize_get_can_create_tags (CtkTextBuffer             *buffer,
                                                          CdkAtom                    format);

GDK_AVAILABLE_IN_ALL
CdkAtom * ctk_text_buffer_get_serialize_formats       (CtkTextBuffer                *buffer,
                                                       gint                         *n_formats);
GDK_AVAILABLE_IN_ALL
CdkAtom * ctk_text_buffer_get_deserialize_formats     (CtkTextBuffer                *buffer,
                                                       gint                         *n_formats);

GDK_AVAILABLE_IN_ALL
guint8  * ctk_text_buffer_serialize                   (CtkTextBuffer                *register_buffer,
                                                       CtkTextBuffer                *content_buffer,
                                                       CdkAtom                       format,
                                                       const CtkTextIter            *start,
                                                       const CtkTextIter            *end,
                                                       gsize                        *length);
GDK_AVAILABLE_IN_ALL
gboolean  ctk_text_buffer_deserialize                 (CtkTextBuffer                *register_buffer,
                                                       CtkTextBuffer                *content_buffer,
                                                       CdkAtom                       format,
                                                       CtkTextIter                  *iter,
                                                       const guint8                 *data,
                                                       gsize                         length,
                                                       GError                      **error);

G_END_DECLS

#endif /* __CTK_TEXT_BUFFER_RICH_TEXT_H__ */
