/* CTK - The GIMP Toolkit
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

#include "config.h"

#include "ctkmain.h"
#include "ctkwindowprivate.h"
#include "ctkwindowgroup.h"


/**
 * SECTION:ctkwindowgroup
 * @Short_description: Limit the effect of grabs
 * @Title: CtkWindowGroup
 *
 * A #CtkWindowGroup restricts the effect of grabs to windows
 * in the same group, thereby making window groups almost behave
 * like separate applications. 
 *
 * A window can be a member in at most one window group at a time.
 * Windows that have not been explicitly assigned to a group are
 * implicitly treated like windows of the default window group.
 *
 * CtkWindowGroup objects are referenced by each window in the group,
 * so once you have added all windows to a CtkWindowGroup, you can drop
 * the initial reference to the window group with g_object_unref(). If the
 * windows in the window group are subsequently destroyed, then they will
 * be removed from the window group and drop their references on the window
 * group; when all window have been removed, the window group will be
 * freed.
 */

typedef struct _CtkDeviceGrabInfo CtkDeviceGrabInfo;
struct _CtkDeviceGrabInfo
{
  CtkWidget *widget;
  GdkDevice *device;
  guint block_others : 1;
};

struct _CtkWindowGroupPrivate
{
  GSList *grabs;
  GSList *device_grabs;
};

G_DEFINE_TYPE_WITH_PRIVATE (CtkWindowGroup, ctk_window_group, G_TYPE_OBJECT)

static void
ctk_window_group_init (CtkWindowGroup *group)
{
  group->priv = ctk_window_group_get_instance_private (group);
}

static void
ctk_window_group_class_init (CtkWindowGroupClass *klass)
{
}


/**
 * ctk_window_group_new:
 * 
 * Creates a new #CtkWindowGroup object. Grabs added with
 * ctk_grab_add() only affect windows within the same #CtkWindowGroup.
 * 
 * Returns: a new #CtkWindowGroup. 
 **/
CtkWindowGroup *
ctk_window_group_new (void)
{
  return g_object_new (CTK_TYPE_WINDOW_GROUP, NULL);
}

static void
window_group_cleanup_grabs (CtkWindowGroup *group,
                            CtkWindow      *window)
{
  CtkWindowGroupPrivate *priv;
  CtkDeviceGrabInfo *info;
  GSList *tmp_list;
  GSList *to_remove = NULL;

  priv = group->priv;

  tmp_list = priv->grabs;
  while (tmp_list)
    {
      if (ctk_widget_get_toplevel (tmp_list->data) == (CtkWidget*) window)
        to_remove = g_slist_prepend (to_remove, g_object_ref (tmp_list->data));
      tmp_list = tmp_list->next;
    }

  while (to_remove)
    {
      ctk_grab_remove (to_remove->data);
      g_object_unref (to_remove->data);
      to_remove = g_slist_delete_link (to_remove, to_remove);
    }

  tmp_list = priv->device_grabs;

  while (tmp_list)
    {
      info = tmp_list->data;

      if (ctk_widget_get_toplevel (info->widget) == (CtkWidget *) window)
        to_remove = g_slist_prepend (to_remove, info);

      tmp_list = tmp_list->next;
    }

  while (to_remove)
    {
      info = to_remove->data;

      ctk_device_grab_remove (info->widget, info->device);
      to_remove = g_slist_delete_link (to_remove, to_remove);
    }
}

/**
 * ctk_window_group_add_window:
 * @window_group: a #CtkWindowGroup
 * @window: the #CtkWindow to add
 * 
 * Adds a window to a #CtkWindowGroup. 
 **/
void
ctk_window_group_add_window (CtkWindowGroup *window_group,
                             CtkWindow      *window)
{
  CtkWindowGroup *old_group;

  g_return_if_fail (CTK_IS_WINDOW_GROUP (window_group));
  g_return_if_fail (CTK_IS_WINDOW (window));

  old_group = _ctk_window_get_window_group (window);

  if (old_group != window_group)
    {
      g_object_ref (window);
      g_object_ref (window_group);

      if (old_group)
        ctk_window_group_remove_window (old_group, window);
      else
        window_group_cleanup_grabs (ctk_window_get_group (NULL), window);

      _ctk_window_set_window_group (window, window_group);

      g_object_unref (window);
    }
}

/**
 * ctk_window_group_remove_window:
 * @window_group: a #CtkWindowGroup
 * @window: the #CtkWindow to remove
 * 
 * Removes a window from a #CtkWindowGroup.
 **/
void
ctk_window_group_remove_window (CtkWindowGroup *window_group,
                                CtkWindow      *window)
{
  g_return_if_fail (CTK_IS_WINDOW_GROUP (window_group));
  g_return_if_fail (CTK_IS_WINDOW (window));
  g_return_if_fail (_ctk_window_get_window_group (window) == window_group);

  g_object_ref (window);

  window_group_cleanup_grabs (window_group, window);
  _ctk_window_set_window_group (window, NULL);

  g_object_unref (window_group);
  g_object_unref (window);
}

/**
 * ctk_window_group_list_windows:
 * @window_group: a #CtkWindowGroup
 *
 * Returns a list of the #CtkWindows that belong to @window_group.
 *
 * Returns: (element-type CtkWindow) (transfer container): A
 *   newly-allocated list of windows inside the group.
 *
 * Since: 2.14
 **/
GList *
ctk_window_group_list_windows (CtkWindowGroup *window_group)
{
  GList *toplevels, *toplevel, *group_windows;

  g_return_val_if_fail (CTK_IS_WINDOW_GROUP (window_group), NULL);

  group_windows = NULL;
  toplevels = ctk_window_list_toplevels ();

  for (toplevel = toplevels; toplevel; toplevel = toplevel->next)
    {
      CtkWindow *window = toplevel->data;

      if (window_group == _ctk_window_get_window_group (window))
        group_windows = g_list_prepend (group_windows, window);
    }

  g_list_free (toplevels);

  return g_list_reverse (group_windows);
}

/**
 * ctk_window_group_get_current_grab:
 * @window_group: a #CtkWindowGroup
 *
 * Gets the current grab widget of the given group,
 * see ctk_grab_add().
 *
 * Returns: (transfer none): the current grab widget of the group
 *
 * Since: 2.22
 */
CtkWidget *
ctk_window_group_get_current_grab (CtkWindowGroup *window_group)
{
  g_return_val_if_fail (CTK_IS_WINDOW_GROUP (window_group), NULL);

  if (window_group->priv->grabs)
    return CTK_WIDGET (window_group->priv->grabs->data);
  return NULL;
}

void
_ctk_window_group_add_grab (CtkWindowGroup *window_group,
                            CtkWidget      *widget)
{
  CtkWindowGroupPrivate *priv;

  priv = window_group->priv;
  priv->grabs = g_slist_prepend (priv->grabs, widget);
}

void
_ctk_window_group_remove_grab (CtkWindowGroup *window_group,
                               CtkWidget      *widget)
{
  CtkWindowGroupPrivate *priv;

  priv = window_group->priv;
  priv->grabs = g_slist_remove (priv->grabs, widget);
}

void
_ctk_window_group_add_device_grab (CtkWindowGroup *window_group,
                                   CtkWidget      *widget,
                                   GdkDevice      *device,
                                   gboolean        block_others)
{
  CtkWindowGroupPrivate *priv;
  CtkDeviceGrabInfo *info;

  priv = window_group->priv;

  info = g_slice_new0 (CtkDeviceGrabInfo);
  info->widget = widget;
  info->device = device;
  info->block_others = block_others;

  priv->device_grabs = g_slist_prepend (priv->device_grabs, info);
}

void
_ctk_window_group_remove_device_grab (CtkWindowGroup *window_group,
                                      CtkWidget      *widget,
                                      GdkDevice      *device)
{
  CtkWindowGroupPrivate *priv;
  CtkDeviceGrabInfo *info;
  GSList *list, *node = NULL;
  GdkDevice *other_device;

  priv = window_group->priv;
  other_device = cdk_device_get_associated_device (device);
  list = priv->device_grabs;

  while (list)
    {
      info = list->data;

      if (info->widget == widget &&
          (info->device == device ||
           info->device == other_device))
        {
          node = list;
          break;
        }

      list = list->next;
    }

  if (node)
    {
      info = node->data;

      priv->device_grabs = g_slist_delete_link (priv->device_grabs, node);
      g_slice_free (CtkDeviceGrabInfo, info);
    }
}

/**
 * ctk_window_group_get_current_device_grab:
 * @window_group: a #CtkWindowGroup
 * @device: a #GdkDevice
 *
 * Returns the current grab widget for @device, or %NULL if none.
 *
 * Returns: (nullable) (transfer none): The grab widget, or %NULL
 *
 * Since: 3.0
 */
CtkWidget *
ctk_window_group_get_current_device_grab (CtkWindowGroup *window_group,
                                          GdkDevice      *device)
{
  CtkWindowGroupPrivate *priv;
  CtkDeviceGrabInfo *info;
  GdkDevice *other_device;
  GSList *list;

  g_return_val_if_fail (CTK_IS_WINDOW_GROUP (window_group), NULL);
  g_return_val_if_fail (GDK_IS_DEVICE (device), NULL);

  priv = window_group->priv;
  list = priv->device_grabs;
  other_device = cdk_device_get_associated_device (device);

  while (list)
    {
      info = list->data;
      list = list->next;

      if (info->device == device ||
          info->device == other_device)
        return info->widget;
    }

  return NULL;
}

gboolean
_ctk_window_group_widget_is_blocked_for_device (CtkWindowGroup *window_group,
                                                CtkWidget      *widget,
                                                GdkDevice      *device)
{
  CtkWindowGroupPrivate *priv;
  CtkDeviceGrabInfo *info;
  GdkDevice *other_device;
  GSList *list;

  priv = window_group->priv;
  other_device = cdk_device_get_associated_device (device);
  list = priv->device_grabs;

  while (list)
    {
      info = list->data;
      list = list->next;

      /* Look for blocking grabs on other device pairs
       * that have the passed widget within the CTK+ grab.
       */
      if (info->block_others &&
          info->device != device &&
          info->device != other_device &&
          (info->widget == widget ||
           ctk_widget_is_ancestor (widget, info->widget)))
        return TRUE;
    }

  return FALSE;
}
