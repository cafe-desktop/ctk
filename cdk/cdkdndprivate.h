/* GDK - The GIMP Drawing Kit
 * Copyright (C) 2010, Red Hat, Inc
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

#ifndef __GDK_DND_PRIVATE_H__
#define __GDK_DND_PRIVATE_H__

#include "cdkdnd.h"

G_BEGIN_DECLS


#define GDK_DRAG_CONTEXT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_DRAG_CONTEXT, CdkDragContextClass))
#define GDK_IS_DRAG_CONTEXT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_DRAG_CONTEXT))
#define GDK_DRAG_CONTEXT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_DRAG_CONTEXT, CdkDragContextClass))

typedef struct _CdkDragContextClass CdkDragContextClass;


struct _CdkDragContextClass {
  GObjectClass parent_class;

  CdkWindow * (*find_window)   (CdkDragContext  *context,
                                CdkWindow       *drag_window,
                                CdkScreen       *screen,
                                gint             x_root,
                                gint             y_root,
                                CdkDragProtocol *protocol);
  CdkAtom     (*get_selection) (CdkDragContext  *context);
  gboolean    (*drag_motion)   (CdkDragContext  *context,
                                CdkWindow       *dest_window,
                                CdkDragProtocol  protocol,
                                gint             root_x,
                                gint             root_y,
                                CdkDragAction    suggested_action,
                                CdkDragAction    possible_actions,
                                guint32          time_);
  void        (*drag_status)   (CdkDragContext  *context,
                                CdkDragAction    action,
                                guint32          time_);
  void        (*drag_abort)    (CdkDragContext  *context,
                                guint32          time_);
  void        (*drag_drop)     (CdkDragContext  *context,
                                guint32          time_);
  void        (*drop_reply)    (CdkDragContext  *context,
                                gboolean         accept,
                                guint32          time_);
  void        (*drop_finish)   (CdkDragContext  *context,
                                gboolean         success,
                                guint32          time_);
  gboolean    (*drop_status)   (CdkDragContext  *context);
  CdkWindow*  (*get_drag_window) (CdkDragContext *context);
  void        (*set_hotspot)   (CdkDragContext  *context,
                                gint             hot_x,
                                gint             hot_y);
  void        (*drop_done)     (CdkDragContext   *context,
                                gboolean          success);

  gboolean    (*manage_dnd)     (CdkDragContext  *context,
                                 CdkWindow       *ipc_window,
                                 CdkDragAction    actions);
  void        (*set_cursor)     (CdkDragContext  *context,
                                 CdkCursor       *cursor);
  void        (*cancel)         (CdkDragContext      *context,
                                 CdkDragCancelReason  reason);
  void        (*drop_performed) (CdkDragContext  *context,
                                 guint32          time);
  void        (*dnd_finished)   (CdkDragContext  *context);

  gboolean    (*handle_event)   (CdkDragContext  *context,
                                 const CdkEvent  *event);
  void        (*action_changed) (CdkDragContext  *context,
                                 CdkDragAction    action);

  void        (*commit_drag_status) (CdkDragContext  *context);
};

struct _CdkDragContext {
  GObject parent_instance;

  /*< private >*/
  CdkDragProtocol protocol;

  CdkDisplay *display;

  gboolean is_source;
  CdkWindow *source_window;
  CdkWindow *dest_window;
  CdkWindow *drag_window;

  GList *targets;
  CdkDragAction actions;
  CdkDragAction suggested_action;
  CdkDragAction action;

  guint32 start_time;

  CdkDevice *device;

  guint drop_done : 1; /* Whether cdk_drag_drop_done() was performed */
};

GList *  cdk_drag_context_list (void);

void     cdk_drag_context_set_cursor          (CdkDragContext *context,
                                               CdkCursor      *cursor);
void     cdk_drag_context_cancel              (CdkDragContext      *context,
                                               CdkDragCancelReason  reason);
gboolean cdk_drag_context_handle_source_event (CdkEvent *event);
gboolean cdk_drag_context_handle_dest_event   (CdkEvent *event);
CdkCursor * cdk_drag_get_cursor               (CdkDragContext *context,
                                               CdkDragAction   action);

G_END_DECLS

#endif
