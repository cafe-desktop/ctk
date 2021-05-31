/* CTK - The GIMP Toolkit
 * Copyright © 2012 Carlos Garnacho <carlosg@gnome.org>
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

#ifndef __CTK_TEXT_HANDLE_PRIVATE_H__
#define __CTK_TEXT_HANDLE_PRIVATE_H__

#include <ctk/ctkwidget.h>

G_BEGIN_DECLS

#define CTK_TYPE_TEXT_HANDLE           (_ctk_text_handle_get_type ())
#define CTK_TEXT_HANDLE(o)             (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_TEXT_HANDLE, CtkTextHandle))
#define CTK_TEXT_HANDLE_CLASS(c)       (G_TYPE_CHECK_CLASS_CAST ((c), CTK_TYPE_TEXT_HANDLE, CtkTextHandleClass))
#define CTK_IS_TEXT_HANDLE(o)          (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_TEXT_HANDLE))
#define CTK_IS_TEXT_HANDLE_CLASS(o)    (G_TYPE_CHECK_CLASS_TYPE ((o), CTK_TYPE_TEXT_HANDLE))
#define CTK_TEXT_HANDLE_GET_CLASS(o)   (G_TYPE_INSTANCE_GET_CLASS ((o), CTK_TYPE_TEXT_HANDLE, CtkTextHandleClass))

typedef struct _CtkTextHandle CtkTextHandle;
typedef struct _CtkTextHandleClass CtkTextHandleClass;

typedef enum
{
  CTK_TEXT_HANDLE_POSITION_CURSOR,
  CTK_TEXT_HANDLE_POSITION_SELECTION_START,
  CTK_TEXT_HANDLE_POSITION_SELECTION_END = CTK_TEXT_HANDLE_POSITION_CURSOR
} CtkTextHandlePosition;

typedef enum
{
  CTK_TEXT_HANDLE_MODE_NONE,
  CTK_TEXT_HANDLE_MODE_CURSOR,
  CTK_TEXT_HANDLE_MODE_SELECTION
} CtkTextHandleMode;

struct _CtkTextHandle
{
  GObject parent_instance;
  gpointer priv;
};

struct _CtkTextHandleClass
{
  GObjectClass parent_class;

  void (* handle_dragged) (CtkTextHandle         *handle,
                           CtkTextHandlePosition  pos,
                           gint                   x,
                           gint                   y);
  void (* drag_finished)  (CtkTextHandle         *handle,
                           CtkTextHandlePosition  pos);
};

GType           _ctk_text_handle_get_type     (void) G_GNUC_CONST;

CtkTextHandle * _ctk_text_handle_new          (CtkWidget             *parent);

void            _ctk_text_handle_set_mode     (CtkTextHandle         *handle,
                                               CtkTextHandleMode      mode);
CtkTextHandleMode
                _ctk_text_handle_get_mode     (CtkTextHandle         *handle);
void            _ctk_text_handle_set_position (CtkTextHandle         *handle,
                                               CtkTextHandlePosition  pos,
                                               GdkRectangle          *rect);
void            _ctk_text_handle_set_visible  (CtkTextHandle         *handle,
                                               CtkTextHandlePosition  pos,
                                               gboolean               visible);

gboolean        _ctk_text_handle_get_is_dragged (CtkTextHandle         *handle,
                                                 CtkTextHandlePosition  pos);
void            _ctk_text_handle_set_direction (CtkTextHandle         *handle,
                                                CtkTextHandlePosition  pos,
                                                CtkTextDirection       dir);

G_END_DECLS

#endif /* __CTK_TEXT_HANDLE_PRIVATE_H__ */
