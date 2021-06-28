/* cdkdnd-quartz.c
 *
 * Copyright (C) 2005 Imendio AB
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

#include "config.h"
#include "cdkdnd.h"
#include "cdkquartzdnd.h"
#include "cdkprivate-quartz.h"
#include "cdkinternal-quartz.h"
#include "cdkquartz-ctk-only.h"

G_DEFINE_TYPE (CdkQuartzDragContext, cdk_quartz_drag_context, CDK_TYPE_DRAG_CONTEXT)


CdkDragContext *_cdk_quartz_drag_source_context = NULL;

CdkDragContext *
cdk_quartz_drag_source_context_libctk_only ()
{
  return _cdk_quartz_drag_source_context;
}

CdkDragContext *
_cdk_quartz_window_drag_begin (CdkWindow *window,
                               CdkDevice *device,
                               GList     *targets,
                               gint       x_root,
                               gint       y_root)
{
  g_assert (_cdk_quartz_drag_source_context == NULL);

  /* Create fake context */
  _cdk_quartz_drag_source_context = g_object_new (CDK_TYPE_QUARTZ_DRAG_CONTEXT,
                                                  NULL);
  _cdk_quartz_drag_source_context->display = cdk_window_get_display (window);
  _cdk_quartz_drag_source_context->is_source = TRUE;

  _cdk_quartz_drag_source_context->source_window = window;
  g_object_ref (window);

  _cdk_quartz_drag_source_context->targets = targets;

  cdk_drag_context_set_device (_cdk_quartz_drag_source_context, device);

  return _cdk_quartz_drag_source_context;
}

static gboolean
cdk_quartz_drag_context_drag_motion (CdkDragContext  *context,
                                     CdkWindow       *dest_window,
                                     CdkDragProtocol  protocol,
                                     gint             x_root,
                                     gint             y_root,
                                     CdkDragAction    suggested_action,
                                     CdkDragAction    possible_actions,
                                     guint32          time)
{
  /* FIXME: Implement */
  return FALSE;
}

static CdkWindow *
cdk_quartz_drag_context_find_window (CdkDragContext  *context,
                                     CdkWindow       *drag_window,
                                     CdkScreen       *screen,
                                     gint             x_root,
                                     gint             y_root,
                                     CdkDragProtocol *protocol)
{
  /* FIXME: Implement */
  return NULL;
}

static void
cdk_quartz_drag_context_drag_drop (CdkDragContext *context,
                                   guint32         time)
{
  /* FIXME: Implement */
}

static void
cdk_quartz_drag_context_drag_abort (CdkDragContext *context,
                                    guint32         time)
{
  /* FIXME: Implement */
}

static void
cdk_quartz_drag_context_drag_status (CdkDragContext *context,
                                     CdkDragAction   action,
                                     guint32         time)
{
  context->action = action;
}

static void
cdk_quartz_drag_context_drop_reply (CdkDragContext *context,
                                    gboolean        ok,
                                    guint32         time)
{
  /* FIXME: Implement */
}

static void
cdk_quartz_drag_context_drop_finish (CdkDragContext *context,
                                     gboolean        success,
                                     guint32         time)
{
  /* FIXME: Implement */
}

void
_cdk_quartz_window_register_dnd (CdkWindow *window)
{
  /* FIXME: Implement */
}

static CdkAtom
cdk_quartz_drag_context_get_selection (CdkDragContext *context)
{
  /* FIXME: Implement */
  return CDK_NONE;
}

static gboolean
cdk_quartz_drag_context_drop_status (CdkDragContext *context)
{
  /* FIXME: Implement */
  return FALSE;
}

id
cdk_quartz_drag_context_get_dragging_info_libctk_only (CdkDragContext *context)
{
  return CDK_QUARTZ_DRAG_CONTEXT (context)->dragging_info;
}

static void
cdk_quartz_drag_context_init (CdkQuartzDragContext *context)
{
}

static void
cdk_quartz_drag_context_finalize (GObject *object)
{
  G_OBJECT_CLASS (cdk_quartz_drag_context_parent_class)->finalize (object);
}

static void
cdk_quartz_drag_context_class_init (CdkQuartzDragContextClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CdkDragContextClass *context_class = CDK_DRAG_CONTEXT_CLASS (klass);

  object_class->finalize = cdk_quartz_drag_context_finalize;

  context_class->find_window = cdk_quartz_drag_context_find_window;
  context_class->drag_status = cdk_quartz_drag_context_drag_status;
  context_class->drag_motion = cdk_quartz_drag_context_drag_motion;
  context_class->drag_abort = cdk_quartz_drag_context_drag_abort;
  context_class->drag_drop = cdk_quartz_drag_context_drag_drop;
  context_class->drop_reply = cdk_quartz_drag_context_drop_reply;
  context_class->drop_finish = cdk_quartz_drag_context_drop_finish;
  context_class->drop_status = cdk_quartz_drag_context_drop_status;
  context_class->get_selection = cdk_quartz_drag_context_get_selection;
}
