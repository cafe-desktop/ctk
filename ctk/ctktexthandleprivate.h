/* GTK - The GIMP Toolkit
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
#define CTK_TEXT_HANDLE(o)             (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_TEXT_HANDLE, GtkTextHandle))
#define CTK_TEXT_HANDLE_CLASS(c)       (G_TYPE_CHECK_CLASS_CAST ((c), CTK_TYPE_TEXT_HANDLE, GtkTextHandleClass))
#define CTK_IS_TEXT_HANDLE(o)          (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_TEXT_HANDLE))
#define CTK_IS_TEXT_HANDLE_CLASS(o)    (G_TYPE_CHECK_CLASS_TYPE ((o), CTK_TYPE_TEXT_HANDLE))
#define CTK_TEXT_HANDLE_GET_CLASS(o)   (G_TYPE_INSTANCE_GET_CLASS ((o), CTK_TYPE_TEXT_HANDLE, GtkTextHandleClass))

typedef struct _GtkTextHandle GtkTextHandle;
typedef struct _GtkTextHandleClass GtkTextHandleClass;

typedef enum
{
  CTK_TEXT_HANDLE_POSITION_CURSOR,
  CTK_TEXT_HANDLE_POSITION_SELECTION_START,
  CTK_TEXT_HANDLE_POSITION_SELECTION_END = CTK_TEXT_HANDLE_POSITION_CURSOR
} GtkTextHandlePosition;

typedef enum
{
  CTK_TEXT_HANDLE_MODE_NONE,
  CTK_TEXT_HANDLE_MODE_CURSOR,
  CTK_TEXT_HANDLE_MODE_SELECTION
} GtkTextHandleMode;

struct _GtkTextHandle
{
  GObject parent_instance;
  gpointer priv;
};

struct _GtkTextHandleClass
{
  GObjectClass parent_class;

  void (* handle_dragged) (GtkTextHandle         *handle,
                           GtkTextHandlePosition  pos,
                           gint                   x,
                           gint                   y);
  void (* drag_finished)  (GtkTextHandle         *handle,
                           GtkTextHandlePosition  pos);
};

GType           _ctk_text_handle_get_type     (void) G_GNUC_CONST;

GtkTextHandle * _ctk_text_handle_new          (GtkWidget             *parent);

void            _ctk_text_handle_set_mode     (GtkTextHandle         *handle,
                                               GtkTextHandleMode      mode);
GtkTextHandleMode
                _ctk_text_handle_get_mode     (GtkTextHandle         *handle);
void            _ctk_text_handle_set_position (GtkTextHandle         *handle,
                                               GtkTextHandlePosition  pos,
                                               GdkRectangle          *rect);
void            _ctk_text_handle_set_visible  (GtkTextHandle         *handle,
                                               GtkTextHandlePosition  pos,
                                               gboolean               visible);

gboolean        _ctk_text_handle_get_is_dragged (GtkTextHandle         *handle,
                                                 GtkTextHandlePosition  pos);
void            _ctk_text_handle_set_direction (GtkTextHandle         *handle,
                                                GtkTextHandlePosition  pos,
                                                GtkTextDirection       dir);

G_END_DECLS

#endif /* __CTK_TEXT_HANDLE_PRIVATE_H__ */
