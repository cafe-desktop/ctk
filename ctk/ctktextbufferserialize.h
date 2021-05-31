/* ctktextbufferserialize.h
 *
 * Copyright (C) 2004 Nokia Corporation.
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

#ifndef __CTK_TEXT_BUFFER_SERIALIZE_H__
#define __CTK_TEXT_BUFFER_SERIALIZE_H__

#include <ctk/ctktextbuffer.h>

guint8 * _ctk_text_buffer_serialize_rich_text   (CtkTextBuffer     *register_buffer,
                                                 CtkTextBuffer     *content_buffer,
                                                 const CtkTextIter *start,
                                                 const CtkTextIter *end,
                                                 gsize             *length,
                                                 gpointer           user_data);

gboolean _ctk_text_buffer_deserialize_rich_text (CtkTextBuffer     *register_buffer,
                                                 CtkTextBuffer     *content_buffer,
                                                 CtkTextIter       *iter,
                                                 const guint8      *data,
                                                 gsize              length,
                                                 gboolean           create_tags,
                                                 gpointer           user_data,
                                                 GError           **error);


#endif /* __CTK_TEXT_BUFFER_SERIALIZE_H__ */
