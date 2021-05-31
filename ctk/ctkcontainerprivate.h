/* GTK - The GIMP Toolkit
 *
 * Copyright (C) 2011 Javier Jardón
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __CTK_CONTAINER_PRIVATE_H__
#define __CTK_CONTAINER_PRIVATE_H__

#include "ctkcontainer.h"

G_BEGIN_DECLS


void     ctk_container_queue_resize_handler    (CtkContainer *container);
void     _ctk_container_queue_restyle          (CtkContainer *container);
void     _ctk_container_clear_resize_widgets   (CtkContainer *container);
gchar*   _ctk_container_child_composite_name   (CtkContainer *container,
                                                CtkWidget    *child);
void     _ctk_container_dequeue_resize_handler (CtkContainer *container);
GList *  _ctk_container_focus_sort             (CtkContainer     *container,
                                                GList            *children,
                                                CtkDirectionType  direction,
                                                CtkWidget        *old_focus);
gboolean _ctk_container_get_reallocate_redraws (CtkContainer *container);

void      _ctk_container_stop_idle_sizer        (CtkContainer *container);
void      _ctk_container_maybe_start_idle_sizer (CtkContainer *container);
gboolean  _ctk_container_get_border_width_set   (CtkContainer *container);
void      _ctk_container_set_border_width_set   (CtkContainer *container,
                                                 gboolean      border_width_set);
GList *   ctk_container_get_all_children        (CtkContainer  *container);
void      ctk_container_get_children_clip       (CtkContainer  *container,
                                                 CtkAllocation *out_clip);
void      ctk_container_set_default_resize_mode (CtkContainer *container,
                                                 CtkResizeMode resize_mode);

G_END_DECLS

#endif /* __CTK_CONTAINER_PRIVATE_H__ */
