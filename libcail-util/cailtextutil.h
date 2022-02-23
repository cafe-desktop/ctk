/* CAIL - The GNOME Accessibility Implementation Library
 * Copyright 2001 Sun Microsystems Inc.
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

#ifndef __CAIL_TEXT_UTIL_H__
#define __CAIL_TEXT_UTIL_H__

#include <glib-object.h>
#include <ctk/ctk.h>

G_BEGIN_DECLS

#define CAIL_TYPE_TEXT_UTIL                  (cail_text_util_get_type ())
#define CAIL_TEXT_UTIL(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CAIL_TYPE_TEXT_UTIL, CailTextUtil))
#define CAIL_TEXT_UTIL_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CAIL_TYPE_TEXT_UTIL, CailTextUtilClass))
#define CAIL_IS_TEXT_UTIL(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAIL_TYPE_TEXT_UTIL))
#define CAIL_IS_TEXT_UTIL_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CAIL_TYPE_TEXT_UTIL))
#define CAIL_TEXT_UTIL_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CAIL_TYPE_TEXT_UTIL, CailTextUtilClass))

/**
 *CailOffsetType:
 *@CAIL_BEFORE_OFFSET: Text before offset is required.
 *@CAIL_AT_OFFSET: Text at offset is required,
 *@CAIL_AFTER_OFFSET: Text after offset is required.
 *
 * Specifies which of the functions atk_text_get_text_before_offset(),
 * atk_text_get_text_at_offset(), atk_text_get_text_after_offset() the
 * function cail_text_util_get_text() is being called for.
 **/
typedef enum
{
  CAIL_BEFORE_OFFSET,
  CAIL_AT_OFFSET,
  CAIL_AFTER_OFFSET
}CailOffsetType;

typedef struct _CailTextUtil		CailTextUtil;
typedef struct _CailTextUtilClass	CailTextUtilClass;

/**
 * CailTextUtil:
 * @buffer: The CtkTextBuffer which identifies the text.
 */
struct _CailTextUtil
{
  /*< private >*/
  GObject parent;

  /*< public >*/
  CtkTextBuffer *buffer;
};

struct _CailTextUtilClass
{
  GObjectClass parent_class;
};

CDK_AVAILABLE_IN_ALL
GType         cail_text_util_get_type      (void);
CDK_AVAILABLE_IN_ALL
CailTextUtil* cail_text_util_new           (void);

CDK_AVAILABLE_IN_ALL
void          cail_text_util_text_setup    (CailTextUtil    *textutil,
                                            const gchar     *text);
CDK_AVAILABLE_IN_ALL
void          cail_text_util_buffer_setup  (CailTextUtil    *textutil,
                                            CtkTextBuffer   *buffer);
CDK_AVAILABLE_IN_ALL
gchar*        cail_text_util_get_text      (CailTextUtil    *textutil,
                                             gpointer        layout,
                                            CailOffsetType  function,
                                            AtkTextBoundary boundary_type,
                                            gint            offset,
                                            gint            *start_offset,
                                            gint            *end_offset);
CDK_AVAILABLE_IN_ALL
gchar*        cail_text_util_get_substring (CailTextUtil    *textutil,
                                            gint            start_pos,
                                            gint            end_pos);

G_END_DECLS

#endif /*__CAIL_TEXT_UTIL_H__ */
