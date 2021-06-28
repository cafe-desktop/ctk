/* GDK - The GIMP Drawing Kit
 * Copyright (C) 2015 Red Hat
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
 *
 * Author: Carlos Garnacho <carlosg@gnome.org>
 */

#include "cdkseatdefaultprivate.h"
#include "cdkdevicetoolprivate.h"

typedef struct _CdkSeatDefaultPrivate CdkSeatDefaultPrivate;

struct _CdkSeatDefaultPrivate
{
  CdkDevice *master_pointer;
  CdkDevice *master_keyboard;
  GList *slave_pointers;
  GList *slave_keyboards;
  CdkSeatCapabilities capabilities;

  GPtrArray *tools;
};

#define KEYBOARD_EVENTS (GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK |    \
                         GDK_FOCUS_CHANGE_MASK)
#define TOUCH_EVENTS    (GDK_TOUCH_MASK)
#define POINTER_EVENTS  (GDK_POINTER_MOTION_MASK |                      \
                         GDK_BUTTON_PRESS_MASK |                        \
                         GDK_BUTTON_RELEASE_MASK |                      \
                         GDK_SCROLL_MASK | GDK_SMOOTH_SCROLL_MASK |     \
                         GDK_ENTER_NOTIFY_MASK |                        \
                         GDK_LEAVE_NOTIFY_MASK |                        \
                         GDK_PROXIMITY_IN_MASK |                        \
                         GDK_PROXIMITY_OUT_MASK)

G_DEFINE_TYPE_WITH_PRIVATE (CdkSeatDefault, cdk_seat_default, GDK_TYPE_SEAT)

static void
cdk_seat_dispose (GObject *object)
{
  CdkSeatDefault *seat = GDK_SEAT_DEFAULT (object);
  CdkSeatDefaultPrivate *priv = cdk_seat_default_get_instance_private (seat);
  GList *l;

  if (priv->master_pointer)
    {
      cdk_seat_device_removed (GDK_SEAT (seat), priv->master_pointer);
      g_clear_object (&priv->master_pointer);
    }

  if (priv->master_keyboard)
    {
      cdk_seat_device_removed (GDK_SEAT (seat), priv->master_keyboard);
      g_clear_object (&priv->master_pointer);
    }

  for (l = priv->slave_pointers; l; l = l->next)
    {
      cdk_seat_device_removed (GDK_SEAT (seat), l->data);
      g_object_unref (l->data);
    }

  for (l = priv->slave_keyboards; l; l = l->next)
    {
      cdk_seat_device_removed (GDK_SEAT (seat), l->data);
      g_object_unref (l->data);
    }

  if (priv->tools)
    {
      g_ptr_array_unref (priv->tools);
      priv->tools = NULL;
    }

  g_list_free (priv->slave_pointers);
  g_list_free (priv->slave_keyboards);
  priv->slave_pointers = NULL;
  priv->slave_keyboards = NULL;

  G_OBJECT_CLASS (cdk_seat_default_parent_class)->dispose (object);
}

static CdkSeatCapabilities
cdk_seat_default_get_capabilities (CdkSeat *seat)
{
  CdkSeatDefaultPrivate *priv;

  priv = cdk_seat_default_get_instance_private (GDK_SEAT_DEFAULT (seat));

  return priv->capabilities;
}

static CdkGrabStatus
cdk_seat_default_grab (CdkSeat                *seat,
                       CdkWindow              *window,
                       CdkSeatCapabilities     capabilities,
                       gboolean                owner_events,
                       CdkCursor              *cursor,
                       const CdkEvent         *event,
                       CdkSeatGrabPrepareFunc  prepare_func,
                       gpointer                prepare_func_data)
{
  CdkSeatDefaultPrivate *priv;
  guint32 evtime = event ? cdk_event_get_time (event) : GDK_CURRENT_TIME;
  CdkGrabStatus status = GDK_GRAB_SUCCESS;
  gboolean was_visible;

  priv = cdk_seat_default_get_instance_private (GDK_SEAT_DEFAULT (seat));
  was_visible = cdk_window_is_visible (window);

  if (prepare_func)
    (prepare_func) (seat, window, prepare_func_data);

  if (!cdk_window_is_visible (window))
    {
      g_critical ("Window %p has not been made visible in CdkSeatGrabPrepareFunc",
                  window);
      return GDK_GRAB_NOT_VIEWABLE;
    }

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;

  if (capabilities & GDK_SEAT_CAPABILITY_ALL_POINTING)
    {
      /* ALL_POINTING spans 3 capabilities; get the mask for the ones we have */
      CdkEventMask pointer_evmask = 0;

      /* We let tablet styli take over the pointer cursor */
      if (capabilities & (GDK_SEAT_CAPABILITY_POINTER |
                          GDK_SEAT_CAPABILITY_TABLET_STYLUS))
        {
          pointer_evmask |= POINTER_EVENTS;
        }

      if (capabilities & GDK_SEAT_CAPABILITY_TOUCH)
        pointer_evmask |= TOUCH_EVENTS;

      status = cdk_device_grab (priv->master_pointer, window,
                                GDK_OWNERSHIP_NONE, owner_events,
                                pointer_evmask, cursor,
                                evtime);
    }

  if (status == GDK_GRAB_SUCCESS &&
      capabilities & GDK_SEAT_CAPABILITY_KEYBOARD)
    {
      status = cdk_device_grab (priv->master_keyboard, window,
                                GDK_OWNERSHIP_NONE, owner_events,
                                KEYBOARD_EVENTS, cursor,
                                evtime);

      if (status != GDK_GRAB_SUCCESS)
        {
          if (capabilities & ~GDK_SEAT_CAPABILITY_KEYBOARD)
            cdk_device_ungrab (priv->master_pointer, evtime);
        }
    }

  if (status != GDK_GRAB_SUCCESS && !was_visible)
    cdk_window_hide (window);

  G_GNUC_END_IGNORE_DEPRECATIONS;

  return status;
}

static void
cdk_seat_default_ungrab (CdkSeat *seat)
{
  CdkSeatDefaultPrivate *priv;

  priv = cdk_seat_default_get_instance_private (GDK_SEAT_DEFAULT (seat));

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  cdk_device_ungrab (priv->master_pointer, GDK_CURRENT_TIME);
  cdk_device_ungrab (priv->master_keyboard, GDK_CURRENT_TIME);
  G_GNUC_END_IGNORE_DEPRECATIONS;
}

static CdkDevice *
cdk_seat_default_get_master (CdkSeat             *seat,
                             CdkSeatCapabilities  capability)
{
  CdkSeatDefaultPrivate *priv;

  priv = cdk_seat_default_get_instance_private (GDK_SEAT_DEFAULT (seat));

  /* There must be only one flag set */
  switch (capability)
    {
    case GDK_SEAT_CAPABILITY_POINTER:
    case GDK_SEAT_CAPABILITY_TOUCH:
      return priv->master_pointer;
    case GDK_SEAT_CAPABILITY_KEYBOARD:
      return priv->master_keyboard;
    default:
      g_warning ("Unhandled capability %x", capability);
      break;
    }

  return NULL;
}

static CdkSeatCapabilities
device_get_capability (CdkDevice *device)
{
  CdkInputSource source;

  source = cdk_device_get_source (device);

  switch (source)
    {
    case GDK_SOURCE_KEYBOARD:
      return GDK_SEAT_CAPABILITY_KEYBOARD;
    case GDK_SOURCE_TOUCHSCREEN:
      return GDK_SEAT_CAPABILITY_TOUCH;
    case GDK_SOURCE_MOUSE:
    case GDK_SOURCE_TOUCHPAD:
    default:
      return GDK_SEAT_CAPABILITY_POINTER;
    }

  return GDK_SEAT_CAPABILITY_NONE;
}

static GList *
append_filtered (GList               *list,
                 GList               *devices,
                 CdkSeatCapabilities  capabilities)
{
  GList *l;

  for (l = devices; l; l = l->next)
    {
      CdkSeatCapabilities device_cap;

      device_cap = device_get_capability (l->data);

      if ((device_cap & capabilities) != 0)
        list = g_list_prepend (list, l->data);
    }

  return list;
}

static GList *
cdk_seat_default_get_slaves (CdkSeat             *seat,
                             CdkSeatCapabilities  capabilities)
{
  CdkSeatDefaultPrivate *priv;
  GList *devices = NULL;

  priv = cdk_seat_default_get_instance_private (GDK_SEAT_DEFAULT (seat));

  if (capabilities & (GDK_SEAT_CAPABILITY_POINTER | GDK_SEAT_CAPABILITY_TOUCH))
    devices = append_filtered (devices, priv->slave_pointers, capabilities);

  if (capabilities & GDK_SEAT_CAPABILITY_KEYBOARD)
    devices = append_filtered (devices, priv->slave_keyboards, capabilities);

  return devices;
}

static CdkDeviceTool *
cdk_seat_default_get_tool (CdkSeat *seat,
                           guint64  serial,
                           guint64  hw_id)
{
  CdkSeatDefaultPrivate *priv;
  CdkDeviceTool *tool;
  guint i;

  priv = cdk_seat_default_get_instance_private (GDK_SEAT_DEFAULT (seat));

  if (!priv->tools)
    return NULL;

  for (i = 0; i < priv->tools->len; i++)
    {
      tool = g_ptr_array_index (priv->tools, i);

      if (tool->serial == serial && tool->hw_id == hw_id)
        return tool;
    }

  return NULL;
}

static void
cdk_seat_default_class_init (CdkSeatDefaultClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CdkSeatClass *seat_class = GDK_SEAT_CLASS (klass);

  object_class->dispose = cdk_seat_dispose;

  seat_class->get_capabilities = cdk_seat_default_get_capabilities;

  seat_class->grab = cdk_seat_default_grab;
  seat_class->ungrab = cdk_seat_default_ungrab;

  seat_class->get_master = cdk_seat_default_get_master;
  seat_class->get_slaves = cdk_seat_default_get_slaves;

  seat_class->get_tool = cdk_seat_default_get_tool;
}

static void
cdk_seat_default_init (CdkSeatDefault *seat)
{
}

CdkSeat *
cdk_seat_default_new_for_master_pair (CdkDevice *pointer,
                                      CdkDevice *keyboard)
{
  CdkSeatDefaultPrivate *priv;
  CdkDisplay *display;
  CdkSeat *seat;

  display = cdk_device_get_display (pointer);

  seat = g_object_new (GDK_TYPE_SEAT_DEFAULT,
                       "display", display,
                       NULL);

  priv = cdk_seat_default_get_instance_private (GDK_SEAT_DEFAULT (seat));
  priv->master_pointer = g_object_ref (pointer);
  priv->master_keyboard = g_object_ref (keyboard);

  cdk_seat_device_added (seat, priv->master_pointer);
  cdk_seat_device_added (seat, priv->master_keyboard);

  return seat;
}

void
cdk_seat_default_add_slave (CdkSeatDefault *seat,
                            CdkDevice      *device)
{
  CdkSeatDefaultPrivate *priv;
  CdkSeatCapabilities capability;

  g_return_if_fail (GDK_IS_SEAT_DEFAULT (seat));
  g_return_if_fail (GDK_IS_DEVICE (device));

  priv = cdk_seat_default_get_instance_private (seat);
  capability = device_get_capability (device);

  if (capability & (GDK_SEAT_CAPABILITY_POINTER | GDK_SEAT_CAPABILITY_TOUCH))
    priv->slave_pointers = g_list_prepend (priv->slave_pointers, g_object_ref (device));
  else if (capability & GDK_SEAT_CAPABILITY_KEYBOARD)
    priv->slave_keyboards = g_list_prepend (priv->slave_keyboards, g_object_ref (device));
  else
    {
      g_critical ("Unhandled capability %x for device '%s'",
                  capability, cdk_device_get_name (device));
      return;
    }

  priv->capabilities |= capability;

  cdk_seat_device_added (GDK_SEAT (seat), device);
}

void
cdk_seat_default_remove_slave (CdkSeatDefault *seat,
                               CdkDevice      *device)
{
  CdkSeatDefaultPrivate *priv;
  GList *l;

  g_return_if_fail (GDK_IS_SEAT_DEFAULT (seat));
  g_return_if_fail (GDK_IS_DEVICE (device));

  priv = cdk_seat_default_get_instance_private (seat);

  if (g_list_find (priv->slave_pointers, device))
    {
      priv->slave_pointers = g_list_remove (priv->slave_pointers, device);

      priv->capabilities &= ~(GDK_SEAT_CAPABILITY_POINTER | GDK_SEAT_CAPABILITY_TOUCH);
      for (l = priv->slave_pointers; l; l = l->next)
        priv->capabilities |= device_get_capability (GDK_DEVICE (l->data));

      cdk_seat_device_removed (GDK_SEAT (seat), device);
      g_object_unref (device);
    }
  else if (g_list_find (priv->slave_keyboards, device))
    {
      priv->slave_keyboards = g_list_remove (priv->slave_keyboards, device);

      if (priv->slave_keyboards == NULL)
        priv->capabilities &= ~GDK_SEAT_CAPABILITY_KEYBOARD;

      cdk_seat_device_removed (GDK_SEAT (seat), device);
      g_object_unref (device);
    }
}

void
cdk_seat_default_add_tool (CdkSeatDefault *seat,
                           CdkDeviceTool  *tool)
{
  CdkSeatDefaultPrivate *priv;

  g_return_if_fail (GDK_IS_SEAT_DEFAULT (seat));
  g_return_if_fail (tool != NULL);

  priv = cdk_seat_default_get_instance_private (seat);

  if (!priv->tools)
    priv->tools = g_ptr_array_new_with_free_func ((GDestroyNotify) g_object_unref);

  g_ptr_array_add (priv->tools, g_object_ref (tool));
  g_signal_emit_by_name (seat, "tool-added", tool);
}

void
cdk_seat_default_remove_tool (CdkSeatDefault *seat,
                              CdkDeviceTool  *tool)
{
  CdkSeatDefaultPrivate *priv;

  g_return_if_fail (GDK_IS_SEAT_DEFAULT (seat));
  g_return_if_fail (tool != NULL);

  priv = cdk_seat_default_get_instance_private (seat);

  if (tool != cdk_seat_get_tool (GDK_SEAT (seat), tool->serial, tool->hw_id))
    return;

  g_signal_emit_by_name (seat, "tool-removed", tool);
  g_ptr_array_remove (priv->tools, tool);
}
