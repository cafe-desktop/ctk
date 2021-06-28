/* CDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

#ifndef __CDK_DND_H__
#define __CDK_DND_H__

#if !defined (__CDK_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdk.h> can be included directly."
#endif

#include <cdk/cdktypes.h>
#include <cdk/cdkdevice.h>
#include <cdk/cdkevents.h>

G_BEGIN_DECLS

#define CDK_TYPE_DRAG_CONTEXT              (cdk_drag_context_get_type ())
#define CDK_DRAG_CONTEXT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), CDK_TYPE_DRAG_CONTEXT, CdkDragContext))
#define CDK_IS_DRAG_CONTEXT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), CDK_TYPE_DRAG_CONTEXT))

/**
 * CdkDragAction:
 * @CDK_ACTION_DEFAULT: Means nothing, and should not be used.
 * @CDK_ACTION_COPY: Copy the data.
 * @CDK_ACTION_MOVE: Move the data, i.e. first copy it, then delete
 *  it from the source using the DELETE target of the X selection protocol.
 * @CDK_ACTION_LINK: Add a link to the data. Note that this is only
 *  useful if source and destination agree on what it means.
 * @CDK_ACTION_PRIVATE: Special action which tells the source that the
 *  destination will do something that the source doesn’t understand.
 * @CDK_ACTION_ASK: Ask the user what to do with the data.
 *
 * Used in #CdkDragContext to indicate what the destination
 * should do with the dropped data.
 */
typedef enum
{
  CDK_ACTION_DEFAULT = 1 << 0,
  CDK_ACTION_COPY    = 1 << 1,
  CDK_ACTION_MOVE    = 1 << 2,
  CDK_ACTION_LINK    = 1 << 3,
  CDK_ACTION_PRIVATE = 1 << 4,
  CDK_ACTION_ASK     = 1 << 5
} CdkDragAction;

/**
 * CdkDragCancelReason:
 * @CDK_DRAG_CANCEL_NO_TARGET: There is no suitable drop target.
 * @CDK_DRAG_CANCEL_USER_CANCELLED: Drag cancelled by the user
 * @CDK_DRAG_CANCEL_ERROR: Unspecified error.
 *
 * Used in #CdkDragContext to the reason of a cancelled DND operation.
 *
 * Since: 3.20
 */
typedef enum {
  CDK_DRAG_CANCEL_NO_TARGET,
  CDK_DRAG_CANCEL_USER_CANCELLED,
  CDK_DRAG_CANCEL_ERROR
} CdkDragCancelReason;

/**
 * CdkDragProtocol:
 * @CDK_DRAG_PROTO_NONE: no protocol.
 * @CDK_DRAG_PROTO_MOTIF: The Motif DND protocol. No longer supported
 * @CDK_DRAG_PROTO_XDND: The Xdnd protocol.
 * @CDK_DRAG_PROTO_ROOTWIN: An extension to the Xdnd protocol for
 *  unclaimed root window drops.
 * @CDK_DRAG_PROTO_WIN32_DROPFILES: The simple WM_DROPFILES protocol.
 * @CDK_DRAG_PROTO_OLE2: The complex OLE2 DND protocol (not implemented).
 * @CDK_DRAG_PROTO_LOCAL: Intra-application DND.
 * @CDK_DRAG_PROTO_WAYLAND: Wayland DND protocol.
 *
 * Used in #CdkDragContext to indicate the protocol according to
 * which DND is done.
 */
typedef enum
{
  CDK_DRAG_PROTO_NONE = 0,
  CDK_DRAG_PROTO_MOTIF,
  CDK_DRAG_PROTO_XDND,
  CDK_DRAG_PROTO_ROOTWIN,
  CDK_DRAG_PROTO_WIN32_DROPFILES,
  CDK_DRAG_PROTO_OLE2,
  CDK_DRAG_PROTO_LOCAL,
  CDK_DRAG_PROTO_WAYLAND
} CdkDragProtocol;


CDK_AVAILABLE_IN_ALL
GType            cdk_drag_context_get_type             (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_ALL
void             cdk_drag_context_set_device           (CdkDragContext *context,
                                                        CdkDevice      *device);
CDK_AVAILABLE_IN_ALL
CdkDevice *      cdk_drag_context_get_device           (CdkDragContext *context);

CDK_AVAILABLE_IN_ALL
GList           *cdk_drag_context_list_targets         (CdkDragContext *context);
CDK_AVAILABLE_IN_ALL
CdkDragAction    cdk_drag_context_get_actions          (CdkDragContext *context);
CDK_AVAILABLE_IN_ALL
CdkDragAction    cdk_drag_context_get_suggested_action (CdkDragContext *context);
CDK_AVAILABLE_IN_ALL
CdkDragAction    cdk_drag_context_get_selected_action  (CdkDragContext *context);

CDK_AVAILABLE_IN_ALL
CdkWindow       *cdk_drag_context_get_source_window    (CdkDragContext *context);
CDK_AVAILABLE_IN_ALL
CdkWindow       *cdk_drag_context_get_dest_window      (CdkDragContext *context);
CDK_AVAILABLE_IN_ALL
CdkDragProtocol  cdk_drag_context_get_protocol         (CdkDragContext *context);

/* Destination side */

CDK_AVAILABLE_IN_ALL
void             cdk_drag_status        (CdkDragContext   *context,
                                         CdkDragAction     action,
                                         guint32           time_);
CDK_AVAILABLE_IN_ALL
void             cdk_drop_reply         (CdkDragContext   *context,
                                         gboolean          accepted,
                                         guint32           time_);
CDK_AVAILABLE_IN_ALL
void             cdk_drop_finish        (CdkDragContext   *context,
                                         gboolean          success,
                                         guint32           time_);
CDK_AVAILABLE_IN_ALL
CdkAtom          cdk_drag_get_selection (CdkDragContext   *context);

/* Source side */

CDK_AVAILABLE_IN_ALL
CdkDragContext * cdk_drag_begin            (CdkWindow      *window,
                                            GList          *targets);

CDK_AVAILABLE_IN_ALL
CdkDragContext * cdk_drag_begin_for_device (CdkWindow      *window,
                                            CdkDevice      *device,
                                            GList          *targets);
CDK_AVAILABLE_IN_3_20
CdkDragContext * cdk_drag_begin_from_point  (CdkWindow      *window,
                                             CdkDevice      *device,
                                             GList          *targets,
                                             gint            x_root,
                                             gint            y_root);

CDK_AVAILABLE_IN_ALL
void    cdk_drag_find_window_for_screen   (CdkDragContext   *context,
                                           CdkWindow        *drag_window,
                                           CdkScreen        *screen,
                                           gint              x_root,
                                           gint              y_root,
                                           CdkWindow       **dest_window,
                                           CdkDragProtocol  *protocol);

CDK_AVAILABLE_IN_ALL
gboolean        cdk_drag_motion      (CdkDragContext *context,
                                      CdkWindow      *dest_window,
                                      CdkDragProtocol protocol,
                                      gint            x_root,
                                      gint            y_root,
                                      CdkDragAction   suggested_action,
                                      CdkDragAction   possible_actions,
                                      guint32         time_);
CDK_AVAILABLE_IN_ALL
void            cdk_drag_drop        (CdkDragContext *context,
                                      guint32         time_);
CDK_AVAILABLE_IN_ALL
void            cdk_drag_abort       (CdkDragContext *context,
                                      guint32         time_);
CDK_AVAILABLE_IN_ALL
gboolean        cdk_drag_drop_succeeded (CdkDragContext *context);

CDK_AVAILABLE_IN_3_20
void            cdk_drag_drop_done   (CdkDragContext *context,
                                      gboolean        success);

CDK_AVAILABLE_IN_3_20
CdkWindow      *cdk_drag_context_get_drag_window (CdkDragContext *context);

CDK_AVAILABLE_IN_3_20
void            cdk_drag_context_set_hotspot (CdkDragContext *context,
                                              gint            hot_x,
                                              gint            hot_y);

CDK_AVAILABLE_IN_3_20
gboolean        cdk_drag_context_manage_dnd (CdkDragContext *context,
                                             CdkWindow      *ipc_window,
                                             CdkDragAction   actions);
G_END_DECLS

#endif /* __CDK_DND_H__ */
