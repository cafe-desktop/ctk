/* GDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1999 Peter Mattis, Spencer Kimball and Josh MacDonald
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

#include "config.h"

#include "cdkdndprivate.h"
#include "cdkdisplay.h"
#include "cdkwindow.h"
#include "cdkintl.h"
#include "cdkenumtypes.h"
#include "cdkcursor.h"

static struct {
  CdkDragAction action;
  const gchar  *name;
  CdkCursor    *cursor;
} drag_cursors[] = {
  { GDK_ACTION_DEFAULT, NULL,       NULL },
  { GDK_ACTION_ASK,     "dnd-ask",  NULL },
  { GDK_ACTION_COPY,    "dnd-copy", NULL },
  { GDK_ACTION_MOVE,    "dnd-move", NULL },
  { GDK_ACTION_LINK,    "dnd-link", NULL },
  { 0,                  "dnd-none", NULL },
};

enum {
  CANCEL,
  DROP_PERFORMED,
  DND_FINISHED,
  ACTION_CHANGED,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0 };
static GList *contexts = NULL;

/**
 * SECTION:dnd
 * @title: Drag And Drop
 * @short_description: Functions for controlling drag and drop handling
 *
 * These functions provide a low level interface for drag and drop.
 * The X backend of GDK supports both the Xdnd and Motif drag and drop
 * protocols transparently, the Win32 backend supports the WM_DROPFILES
 * protocol.
 *
 * CTK+ provides a higher level abstraction based on top of these functions,
 * and so they are not normally needed in CTK+ applications.
 * See the [Drag and Drop][ctk3-Drag-and-Drop] section of
 * the CTK+ documentation for more information.
 */

/**
 * cdk_drag_context_list_targets:
 * @context: a #CdkDragContext
 *
 * Retrieves the list of targets of the context.
 *
 * Returns: (transfer none) (element-type CdkAtom): a #GList of targets
 *
 * Since: 2.22
 **/
GList *
cdk_drag_context_list_targets (CdkDragContext *context)
{
  g_return_val_if_fail (GDK_IS_DRAG_CONTEXT (context), NULL);

  return context->targets;
}

/**
 * cdk_drag_context_get_actions:
 * @context: a #CdkDragContext
 *
 * Determines the bitmask of actions proposed by the source if
 * cdk_drag_context_get_suggested_action() returns %GDK_ACTION_ASK.
 *
 * Returns: the #CdkDragAction flags
 *
 * Since: 2.22
 **/
CdkDragAction
cdk_drag_context_get_actions (CdkDragContext *context)
{
  g_return_val_if_fail (GDK_IS_DRAG_CONTEXT (context), GDK_ACTION_DEFAULT);

  return context->actions;
}

/**
 * cdk_drag_context_get_suggested_action:
 * @context: a #CdkDragContext
 *
 * Determines the suggested drag action of the context.
 *
 * Returns: a #CdkDragAction value
 *
 * Since: 2.22
 **/
CdkDragAction
cdk_drag_context_get_suggested_action (CdkDragContext *context)
{
  g_return_val_if_fail (GDK_IS_DRAG_CONTEXT (context), 0);

  return context->suggested_action;
}

/**
 * cdk_drag_context_get_selected_action:
 * @context: a #CdkDragContext
 *
 * Determines the action chosen by the drag destination.
 *
 * Returns: a #CdkDragAction value
 *
 * Since: 2.22
 **/
CdkDragAction
cdk_drag_context_get_selected_action (CdkDragContext *context)
{
  g_return_val_if_fail (GDK_IS_DRAG_CONTEXT (context), 0);

  return context->action;
}

/**
 * cdk_drag_context_get_source_window:
 * @context: a #CdkDragContext
 *
 * Returns the #CdkWindow where the DND operation started.
 *
 * Returns: (transfer none): a #CdkWindow
 *
 * Since: 2.22
 **/
CdkWindow *
cdk_drag_context_get_source_window (CdkDragContext *context)
{
  g_return_val_if_fail (GDK_IS_DRAG_CONTEXT (context), NULL);

  return context->source_window;
}

/**
 * cdk_drag_context_get_dest_window:
 * @context: a #CdkDragContext
 *
 * Returns the destination window for the DND operation.
 *
 * Returns: (transfer none): a #CdkWindow
 *
 * Since: 3.0
 **/
CdkWindow *
cdk_drag_context_get_dest_window (CdkDragContext *context)
{
  g_return_val_if_fail (GDK_IS_DRAG_CONTEXT (context), NULL);

  return context->dest_window;
}

/**
 * cdk_drag_context_get_protocol:
 * @context: a #CdkDragContext
 *
 * Returns the drag protocol that is used by this context.
 *
 * Returns: the drag protocol
 *
 * Since: 3.0
 */
CdkDragProtocol
cdk_drag_context_get_protocol (CdkDragContext *context)
{
  g_return_val_if_fail (GDK_IS_DRAG_CONTEXT (context), GDK_DRAG_PROTO_NONE);

  return context->protocol;
}

/**
 * cdk_drag_context_set_device:
 * @context: a #CdkDragContext
 * @device: a #CdkDevice
 *
 * Associates a #CdkDevice to @context, so all Drag and Drop events
 * for @context are emitted as if they came from this device.
 */
void
cdk_drag_context_set_device (CdkDragContext *context,
                             CdkDevice      *device)
{
  g_return_if_fail (GDK_IS_DRAG_CONTEXT (context));
  g_return_if_fail (GDK_IS_DEVICE (device));

  if (context->device)
    g_object_unref (context->device);

  context->device = device;

  if (context->device)
    g_object_ref (context->device);
}

/**
 * cdk_drag_context_get_device:
 * @context: a #CdkDragContext
 *
 * Returns the #CdkDevice associated to the drag context.
 *
 * Returns: (transfer none): The #CdkDevice associated to @context.
 **/
CdkDevice *
cdk_drag_context_get_device (CdkDragContext *context)
{
  g_return_val_if_fail (GDK_IS_DRAG_CONTEXT (context), NULL);

  return context->device;
}

G_DEFINE_TYPE (CdkDragContext, cdk_drag_context, G_TYPE_OBJECT)

static void
cdk_drag_context_init (CdkDragContext *context)
{
  contexts = g_list_prepend (contexts, context);
}

static void
cdk_drag_context_finalize (GObject *object)
{
  CdkDragContext *context = GDK_DRAG_CONTEXT (object);

  contexts = g_list_remove (contexts, context);
  g_list_free (context->targets);

  if (context->source_window)
    g_object_unref (context->source_window);

  if (context->dest_window)
    g_object_unref (context->dest_window);

  G_OBJECT_CLASS (cdk_drag_context_parent_class)->finalize (object);
}

static void
cdk_drag_context_class_init (CdkDragContextClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = cdk_drag_context_finalize;

  /**
   * CdkDragContext::cancel:
   * @context: The object on which the signal is emitted
   * @reason: The reason the context was cancelled
   *
   * The drag and drop operation was cancelled.
   *
   * This signal will only be emitted if the #CdkDragContext manages
   * the drag and drop operation. See cdk_drag_context_manage_dnd()
   * for more information.
   *
   * Since: 3.20
   */
  signals[CANCEL] =
    g_signal_new ("cancel",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CdkDragContextClass, cancel),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1, GDK_TYPE_DRAG_CANCEL_REASON);

  /**
   * CdkDragContext::drop-performed:
   * @context: The object on which the signal is emitted
   * @time: the time at which the drop happened.
   *
   * The drag and drop operation was performed on an accepting client.
   *
   * This signal will only be emitted if the #CdkDragContext manages
   * the drag and drop operation. See cdk_drag_context_manage_dnd()
   * for more information.
   *
   * Since: 3.20
   */
  signals[DROP_PERFORMED] =
    g_signal_new ("drop-performed",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CdkDragContextClass, drop_performed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1, G_TYPE_INT);

  /**
   * CdkDragContext::dnd-finished:
   * @context: The object on which the signal is emitted
   *
   * The drag and drop operation was finished, the drag destination
   * finished reading all data. The drag source can now free all
   * miscellaneous data.
   *
   * This signal will only be emitted if the #CdkDragContext manages
   * the drag and drop operation. See cdk_drag_context_manage_dnd()
   * for more information.
   *
   * Since: 3.20
   */
  signals[DND_FINISHED] =
    g_signal_new ("dnd-finished",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CdkDragContextClass, dnd_finished),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * CdkDragContext::action-changed:
   * @context: The object on which the signal is emitted
   * @action: The action currently chosen
   *
   * A new action is being chosen for the drag and drop operation.
   *
   * This signal will only be emitted if the #CdkDragContext manages
   * the drag and drop operation. See cdk_drag_context_manage_dnd()
   * for more information.
   *
   * Since: 3.20
   */
  signals[ACTION_CHANGED] =
    g_signal_new ("action-changed",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CdkDragContextClass, action_changed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1, GDK_TYPE_DRAG_ACTION);
}

/**
 * cdk_drag_find_window_for_screen:
 * @context: a #CdkDragContext
 * @drag_window: a window which may be at the pointer position, but
 *     should be ignored, since it is put up by the drag source as an icon
 * @screen: the screen where the destination window is sought
 * @x_root: the x position of the pointer in root coordinates
 * @y_root: the y position of the pointer in root coordinates
 * @dest_window: (out): location to store the destination window in
 * @protocol: (out): location to store the DND protocol in
 *
 * Finds the destination window and DND protocol to use at the
 * given pointer position.
 *
 * This function is called by the drag source to obtain the
 * @dest_window and @protocol parameters for cdk_drag_motion().
 *
 * Since: 2.2
 */
void
cdk_drag_find_window_for_screen (CdkDragContext  *context,
                                 CdkWindow       *drag_window,
                                 CdkScreen       *screen,
                                 gint             x_root,
                                 gint             y_root,
                                 CdkWindow      **dest_window,
                                 CdkDragProtocol *protocol)
{
  g_return_if_fail (GDK_IS_DRAG_CONTEXT (context));

  *dest_window = GDK_DRAG_CONTEXT_GET_CLASS (context)
      ->find_window (context, drag_window, screen, x_root, y_root, protocol);
}

/**
 * cdk_drag_status:
 * @context: a #CdkDragContext
 * @action: the selected action which will be taken when a drop happens,
 *    or 0 to indicate that a drop will not be accepted
 * @time_: the timestamp for this operation
 *
 * Selects one of the actions offered by the drag source.
 *
 * This function is called by the drag destination in response to
 * cdk_drag_motion() called by the drag source.
 */
void
cdk_drag_status (CdkDragContext *context,
                 CdkDragAction   action,
                 guint32         time_)
{
  g_return_if_fail (GDK_IS_DRAG_CONTEXT (context));

  GDK_DRAG_CONTEXT_GET_CLASS (context)->drag_status (context, action, time_);
}

/**
 * cdk_drag_motion:
 * @context: a #CdkDragContext
 * @dest_window: the new destination window, obtained by
 *     cdk_drag_find_window()
 * @protocol: the DND protocol in use, obtained by cdk_drag_find_window()
 * @x_root: the x position of the pointer in root coordinates
 * @y_root: the y position of the pointer in root coordinates
 * @suggested_action: the suggested action
 * @possible_actions: the possible actions
 * @time_: the timestamp for this operation
 *
 * Updates the drag context when the pointer moves or the
 * set of actions changes.
 *
 * This function is called by the drag source.
 *
 * This function does not need to be called in managed drag and drop
 * operations. See cdk_drag_context_manage_dnd() for more information.
 *
 * Returns:
 */
gboolean
cdk_drag_motion (CdkDragContext *context,
                 CdkWindow      *dest_window,
                 CdkDragProtocol protocol,
                 gint            x_root,
                 gint            y_root,
                 CdkDragAction   suggested_action,
                 CdkDragAction   possible_actions,
                 guint32         time_)
{
  g_return_val_if_fail (GDK_IS_DRAG_CONTEXT (context), FALSE);

  return GDK_DRAG_CONTEXT_GET_CLASS (context)
       ->drag_motion (context,
                      dest_window,
                      protocol,
                      x_root,
                      y_root,
                      suggested_action,
                      possible_actions,
                      time_);
}

/**
 * cdk_drag_abort:
 * @context: a #CdkDragContext
 * @time_: the timestamp for this operation
 *
 * Aborts a drag without dropping.
 *
 * This function is called by the drag source.
 *
 * This function does not need to be called in managed drag and drop
 * operations. See cdk_drag_context_manage_dnd() for more information.
 */
void
cdk_drag_abort (CdkDragContext *context,
                guint32         time_)
{
  g_return_if_fail (GDK_IS_DRAG_CONTEXT (context));

  GDK_DRAG_CONTEXT_GET_CLASS (context)->drag_abort (context, time_);
}

/**
 * cdk_drag_drop:
 * @context: a #CdkDragContext
 * @time_: the timestamp for this operation
 *
 * Drops on the current destination.
 *
 * This function is called by the drag source.
 *
 * This function does not need to be called in managed drag and drop
 * operations. See cdk_drag_context_manage_dnd() for more information.
 */
void
cdk_drag_drop (CdkDragContext *context,
               guint32         time_)
{
  g_return_if_fail (GDK_IS_DRAG_CONTEXT (context));

  GDK_DRAG_CONTEXT_GET_CLASS (context)->drag_drop (context, time_);
}

/**
 * cdk_drop_reply:
 * @context: a #CdkDragContext
 * @accepted: %TRUE if the drop is accepted
 * @time_: the timestamp for this operation
 *
 * Accepts or rejects a drop.
 *
 * This function is called by the drag destination in response
 * to a drop initiated by the drag source.
 */
void
cdk_drop_reply (CdkDragContext *context,
                gboolean        accepted,
                guint32         time_)
{
  g_return_if_fail (GDK_IS_DRAG_CONTEXT (context));

  GDK_DRAG_CONTEXT_GET_CLASS (context)->drop_reply (context, accepted, time_);
}

/**
 * cdk_drop_finish:
 * @context: a #CdkDragContext
 * @success: %TRUE if the data was successfully received
 * @time_: the timestamp for this operation
 *
 * Ends the drag operation after a drop.
 *
 * This function is called by the drag destination.
 */
void
cdk_drop_finish (CdkDragContext *context,
                 gboolean        success,
                 guint32         time_)
{
  g_return_if_fail (GDK_IS_DRAG_CONTEXT (context));

  GDK_DRAG_CONTEXT_GET_CLASS (context)->drop_finish (context, success, time_);
}

/**
 * cdk_drag_drop_succeeded:
 * @context: a #CdkDragContext
 *
 * Returns whether the dropped data has been successfully
 * transferred. This function is intended to be used while
 * handling a %GDK_DROP_FINISHED event, its return value is
 * meaningless at other times.
 *
 * Returns: %TRUE if the drop was successful.
 *
 * Since: 2.6
 **/
gboolean
cdk_drag_drop_succeeded (CdkDragContext *context)
{
  g_return_val_if_fail (GDK_IS_DRAG_CONTEXT (context), FALSE);

  return GDK_DRAG_CONTEXT_GET_CLASS (context)->drop_status (context);
}

/**
 * cdk_drag_get_selection:
 * @context: a #CdkDragContext.
 *
 * Returns the selection atom for the current source window.
 *
 * Returns: (transfer none): the selection atom, or %GDK_NONE
 */
CdkAtom
cdk_drag_get_selection (CdkDragContext *context)
{
  g_return_val_if_fail (GDK_IS_DRAG_CONTEXT (context), GDK_NONE);

  return GDK_DRAG_CONTEXT_GET_CLASS (context)->get_selection (context);
}

/**
 * cdk_drag_context_get_drag_window:
 * @context: a #CdkDragContext
 *
 * Returns the window on which the drag icon should be rendered
 * during the drag operation. Note that the window may not be
 * available until the drag operation has begun. GDK will move
 * the window in accordance with the ongoing drag operation.
 * The window is owned by @context and will be destroyed when
 * the drag operation is over.
 *
 * Returns: (nullable) (transfer none): the drag window, or %NULL
 *
 * Since: 3.20
 */
CdkWindow *
cdk_drag_context_get_drag_window (CdkDragContext *context)
{
  g_return_val_if_fail (GDK_IS_DRAG_CONTEXT (context), NULL);

  if (GDK_DRAG_CONTEXT_GET_CLASS (context)->get_drag_window)
    return GDK_DRAG_CONTEXT_GET_CLASS (context)->get_drag_window (context);

  return NULL;
}

/**
 * cdk_drag_context_set_hotspot:
 * @context: a #CdkDragContext
 * @hot_x: x coordinate of the drag window hotspot
 * @hot_y: y coordinate of the drag window hotspot
 *
 * Sets the position of the drag window that will be kept
 * under the cursor hotspot. Initially, the hotspot is at the
 * top left corner of the drag window.
 *
 * Since: 3.20
 */
void
cdk_drag_context_set_hotspot (CdkDragContext *context,
                              gint            hot_x,
                              gint            hot_y)
{
  g_return_if_fail (GDK_IS_DRAG_CONTEXT (context));

  if (GDK_DRAG_CONTEXT_GET_CLASS (context)->set_hotspot)
    GDK_DRAG_CONTEXT_GET_CLASS (context)->set_hotspot (context, hot_x, hot_y);
}

/**
 * cdk_drag_drop_done:
 * @context: a #CdkDragContext
 * @success: whether the drag was ultimatively successful
 *
 * Inform GDK if the drop ended successfully. Passing %FALSE
 * for @success may trigger a drag cancellation animation.
 *
 * This function is called by the drag source, and should
 * be the last call before dropping the reference to the
 * @context.
 *
 * The #CdkDragContext will only take the first cdk_drag_drop_done()
 * call as effective, if this function is called multiple times,
 * all subsequent calls will be ignored.
 *
 * Since: 3.20
 */
void
cdk_drag_drop_done (CdkDragContext *context,
                    gboolean        success)
{
  g_return_if_fail (GDK_IS_DRAG_CONTEXT (context));

  if (context->drop_done)
    return;

  context->drop_done = TRUE;

  if (GDK_DRAG_CONTEXT_GET_CLASS (context)->drop_done)
    GDK_DRAG_CONTEXT_GET_CLASS (context)->drop_done (context, success);
}

/**
 * cdk_drag_context_manage_dnd:
 * @context: a #CdkDragContext
 * @ipc_window: Window to use for IPC messaging/events
 * @actions: the actions supported by the drag source
 *
 * Requests the drag and drop operation to be managed by @context.
 * When a drag and drop operation becomes managed, the #CdkDragContext
 * will internally handle all input and source-side #CdkEventDND events
 * as required by the windowing system.
 *
 * Once the drag and drop operation is managed, the drag context will
 * emit the following signals:
 * - The #CdkDragContext::action-changed signal whenever the final action
 *   to be performed by the drag and drop operation changes.
 * - The #CdkDragContext::drop-performed signal after the user performs
 *   the drag and drop gesture (typically by releasing the mouse button).
 * - The #CdkDragContext::dnd-finished signal after the drag and drop
 *   operation concludes (after all #CdkSelection transfers happen).
 * - The #CdkDragContext::cancel signal if the drag and drop operation is
 *   finished but doesn't happen over an accepting destination, or is
 *   cancelled through other means.
 *
 * Returns: #TRUE if the drag and drop operation is managed.
 *
 * Since: 3.20
 **/
gboolean
cdk_drag_context_manage_dnd (CdkDragContext *context,
                             CdkWindow      *ipc_window,
                             CdkDragAction   actions)
{
  g_return_val_if_fail (GDK_IS_DRAG_CONTEXT (context), FALSE);
  g_return_val_if_fail (GDK_IS_WINDOW (ipc_window), FALSE);

  if (GDK_DRAG_CONTEXT_GET_CLASS (context)->manage_dnd)
    return GDK_DRAG_CONTEXT_GET_CLASS (context)->manage_dnd (context, ipc_window, actions);

  return FALSE;
}

void
cdk_drag_context_set_cursor (CdkDragContext *context,
                             CdkCursor      *cursor)
{
  g_return_if_fail (GDK_IS_DRAG_CONTEXT (context));

  if (GDK_DRAG_CONTEXT_GET_CLASS (context)->set_cursor)
    GDK_DRAG_CONTEXT_GET_CLASS (context)->set_cursor (context, cursor);
}

void
cdk_drag_context_cancel (CdkDragContext      *context,
                         CdkDragCancelReason  reason)
{
  g_return_if_fail (GDK_IS_DRAG_CONTEXT (context));

  g_signal_emit (context, signals[CANCEL], 0, reason);
}

GList *
cdk_drag_context_list (void)
{
  return contexts;
}

gboolean
cdk_drag_context_handle_source_event (CdkEvent *event)
{
  CdkDragContext *context;
  GList *l;

  for (l = contexts; l; l = l->next)
    {
      context = l->data;

      if (!context->is_source)
        continue;

      if (!GDK_DRAG_CONTEXT_GET_CLASS (context)->handle_event)
        continue;

      if (GDK_DRAG_CONTEXT_GET_CLASS (context)->handle_event (context, event))
        return TRUE;
    }

  return FALSE;
}

CdkCursor *
cdk_drag_get_cursor (CdkDragContext *context,
                     CdkDragAction   action)
{
  gint i;

  for (i = 0 ; i < G_N_ELEMENTS (drag_cursors) - 1; i++)
    if (drag_cursors[i].action == action)
      break;

  if (drag_cursors[i].cursor == NULL)
    drag_cursors[i].cursor = cdk_cursor_new_from_name (context->display,
                                                       drag_cursors[i].name);
  return drag_cursors[i].cursor;
}

static void
cdk_drag_context_commit_drag_status (CdkDragContext *context)
{
  CdkDragContextClass *context_class;

  g_return_if_fail (GDK_IS_DRAG_CONTEXT (context));
  g_return_if_fail (!context->is_source);

  context_class = GDK_DRAG_CONTEXT_GET_CLASS (context);

  if (context_class->commit_drag_status)
    context_class->commit_drag_status (context);
}

gboolean
cdk_drag_context_handle_dest_event (CdkEvent *event)
{
  CdkDragContext *context = NULL;
  GList *l;

  switch (event->type)
    {
    case GDK_DRAG_MOTION:
    case GDK_DROP_START:
      context = event->dnd.context;
      break;
    case GDK_SELECTION_NOTIFY:
      for (l = contexts; l; l = l->next)
        {
          CdkDragContext *c = l->data;

          if (!c->is_source &&
              event->selection.selection == cdk_drag_get_selection (c))
            {
              context = c;
              break;
            }
        }
      break;
    default:
      return FALSE;
    }

  if (!context)
    return FALSE;

  cdk_drag_context_commit_drag_status (context);
  return TRUE;;
}
