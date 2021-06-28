/* CTK - The GIMP Toolkit
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

#include "ctkdragsource.h"

#include "ctkdnd.h"
#include "ctkdndprivate.h"
#include "ctkgesturedrag.h"
#include "ctkimagedefinitionprivate.h"
#include "ctkintl.h"


typedef struct _CtkDragSourceSite CtkDragSourceSite;

struct _CtkDragSourceSite 
{
  CdkModifierType    start_button_mask;
  CtkTargetList     *target_list;        /* Targets for drag data */
  CdkDragAction      actions;            /* Possible actions */

  CtkImageDefinition *image_def;
  CtkGesture        *drag_gesture;
};
  
static void
ctk_drag_source_gesture_begin (CtkGesture       *gesture,
                               CdkEventSequence *sequence,
                               gpointer          data)
{
  CtkDragSourceSite *site = data;
  guint button;

  if (ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (gesture)))
    button = 1;
  else
    button = ctk_gesture_single_get_current_button (CTK_GESTURE_SINGLE (gesture));

  g_assert (button >= 1);

  if (!site->start_button_mask ||
      !(site->start_button_mask & (CDK_BUTTON1_MASK << (button - 1))))
    ctk_gesture_set_state (gesture, CTK_EVENT_SEQUENCE_DENIED);
}

static gboolean
ctk_drag_source_event_cb (CtkWidget *widget,
                          CdkEvent  *event,
                          gpointer   data)
{
  gdouble start_x, start_y, offset_x, offset_y;
  CtkDragSourceSite *site = data;

  ctk_event_controller_handle_event (CTK_EVENT_CONTROLLER (site->drag_gesture), event);

  if (ctk_gesture_is_recognized (site->drag_gesture))
    {
      ctk_gesture_drag_get_start_point (CTK_GESTURE_DRAG (site->drag_gesture),
                                        &start_x, &start_y);
      ctk_gesture_drag_get_offset (CTK_GESTURE_DRAG (site->drag_gesture),
                                   &offset_x, &offset_y);

      if (ctk_drag_check_threshold (widget, start_x, start_y,
                                    start_x + offset_x, start_y + offset_y))
        {
          CdkEventSequence *sequence;
          CdkEvent *last_event;
          guint button;
          gboolean needs_icon;
          CdkDragContext *context;

          sequence = ctk_gesture_single_get_current_sequence (CTK_GESTURE_SINGLE (site->drag_gesture));
          last_event = cdk_event_copy (ctk_gesture_get_last_event (site->drag_gesture, sequence));

          button = ctk_gesture_single_get_current_button (CTK_GESTURE_SINGLE (site->drag_gesture));
          ctk_event_controller_reset (CTK_EVENT_CONTROLLER (site->drag_gesture));

          context = ctk_drag_begin_internal (widget, &needs_icon, site->target_list,
                                             site->actions, button, last_event,
                                             start_x, start_y);

          if (context != NULL && needs_icon)
            ctk_drag_set_icon_definition (context, site->image_def, 0, 0);

          cdk_event_free (last_event);

          return TRUE;
        }
    }

  return FALSE;
}

static void 
ctk_drag_source_site_destroy (gpointer data)
{
  CtkDragSourceSite *site = data;

  if (site->target_list)
    ctk_target_list_unref (site->target_list);

  ctk_image_definition_unref (site->image_def);
  g_clear_object (&site->drag_gesture);
  g_slice_free (CtkDragSourceSite, site);
}

/**
 * ctk_drag_source_set: (method)
 * @widget: a #CtkWidget
 * @start_button_mask: the bitmask of buttons that can start the drag
 * @targets: (allow-none) (array length=n_targets): the table of targets
 *     that the drag will support, may be %NULL
 * @n_targets: the number of items in @targets
 * @actions: the bitmask of possible actions for a drag from this widget
 *
 * Sets up a widget so that CTK+ will start a drag operation when the user
 * clicks and drags on the widget. The widget must have a window.
 */
void
ctk_drag_source_set (CtkWidget            *widget,
                     CdkModifierType       start_button_mask,
                     const CtkTargetEntry *targets,
                     gint                  n_targets,
                     CdkDragAction         actions)
{
  CtkDragSourceSite *site;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  site = g_object_get_data (G_OBJECT (widget), "ctk-site-data");

  ctk_widget_add_events (widget,
                         ctk_widget_get_events (widget) |
                         CDK_BUTTON_PRESS_MASK | CDK_BUTTON_RELEASE_MASK |
                         CDK_BUTTON_MOTION_MASK);

  if (site)
    {
      if (site->target_list)
        ctk_target_list_unref (site->target_list);
    }
  else
    {
      site = g_slice_new0 (CtkDragSourceSite);
      site->image_def = ctk_image_definition_new_empty ();
      site->drag_gesture = ctk_gesture_drag_new (widget);
      ctk_event_controller_set_propagation_phase (CTK_EVENT_CONTROLLER (site->drag_gesture),
                                                  CTK_PHASE_NONE);
      ctk_gesture_single_set_button (CTK_GESTURE_SINGLE (site->drag_gesture), 0);
      g_signal_connect (site->drag_gesture, "begin",
                        G_CALLBACK (ctk_drag_source_gesture_begin),
                        site);

      g_signal_connect (widget, "button-press-event",
                        G_CALLBACK (ctk_drag_source_event_cb),
                        site);
      g_signal_connect (widget, "button-release-event",
                        G_CALLBACK (ctk_drag_source_event_cb),
                        site);
      g_signal_connect (widget, "motion-notify-event",
                        G_CALLBACK (ctk_drag_source_event_cb),
                        site);
      g_object_set_data_full (G_OBJECT (widget),
                              I_("ctk-site-data"), 
                              site, ctk_drag_source_site_destroy);
    }

  site->start_button_mask = start_button_mask;

  site->target_list = ctk_target_list_new (targets, n_targets);

  site->actions = actions;
}

/**
 * ctk_drag_source_unset: (method)
 * @widget: a #CtkWidget
 *
 * Undoes the effects of ctk_drag_source_set().
 */
void 
ctk_drag_source_unset (CtkWidget *widget)
{
  CtkDragSourceSite *site;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  site = g_object_get_data (G_OBJECT (widget), "ctk-site-data");

  if (site)
    {
      g_signal_handlers_disconnect_by_func (widget,
                                            ctk_drag_source_event_cb,
                                            site);
      g_object_set_data (G_OBJECT (widget), I_("ctk-site-data"), NULL);
    }
}

/**
 * ctk_drag_source_get_target_list: (method)
 * @widget: a #CtkWidget
 *
 * Gets the list of targets this widget can provide for
 * drag-and-drop.
 *
 * Returns: (nullable) (transfer none): the #CtkTargetList, or %NULL if none
 *
 * Since: 2.4
 */
CtkTargetList *
ctk_drag_source_get_target_list (CtkWidget *widget)
{
  CtkDragSourceSite *site;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);

  site = g_object_get_data (G_OBJECT (widget), "ctk-site-data");

  return site ? site->target_list : NULL;
}

/**
 * ctk_drag_source_set_target_list: (method)
 * @widget: a #CtkWidget that’s a drag source
 * @target_list: (allow-none): list of draggable targets, or %NULL for none
 *
 * Changes the target types that this widget offers for drag-and-drop.
 * The widget must first be made into a drag source with
 * ctk_drag_source_set().
 *
 * Since: 2.4
 */
void
ctk_drag_source_set_target_list (CtkWidget     *widget,
                                 CtkTargetList *target_list)
{
  CtkDragSourceSite *site;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  site = g_object_get_data (G_OBJECT (widget), "ctk-site-data");
  if (site == NULL)
    {
      g_warning ("ctk_drag_source_set_target_list() requires the widget "
                 "to already be a drag source.");
      return;
    }

  if (target_list)
    ctk_target_list_ref (target_list);

  if (site->target_list)
    ctk_target_list_unref (site->target_list);

  site->target_list = target_list;
}

/**
 * ctk_drag_source_add_text_targets: (method)
 * @widget: a #CtkWidget that’s is a drag source
 *
 * Add the text targets supported by #CtkSelectionData to
 * the target list of the drag source.  The targets
 * are added with @info = 0. If you need another value, 
 * use ctk_target_list_add_text_targets() and
 * ctk_drag_source_set_target_list().
 * 
 * Since: 2.6
 */
void
ctk_drag_source_add_text_targets (CtkWidget *widget)
{
  CtkTargetList *target_list;

  target_list = ctk_drag_source_get_target_list (widget);
  if (target_list)
    ctk_target_list_ref (target_list);
  else
    target_list = ctk_target_list_new (NULL, 0);
  ctk_target_list_add_text_targets (target_list, 0);
  ctk_drag_source_set_target_list (widget, target_list);
  ctk_target_list_unref (target_list);
}

/**
 * ctk_drag_source_add_image_targets: (method)
 * @widget: a #CtkWidget that’s is a drag source
 *
 * Add the writable image targets supported by #CtkSelectionData to
 * the target list of the drag source. The targets
 * are added with @info = 0. If you need another value, 
 * use ctk_target_list_add_image_targets() and
 * ctk_drag_source_set_target_list().
 * 
 * Since: 2.6
 */
void
ctk_drag_source_add_image_targets (CtkWidget *widget)
{
  CtkTargetList *target_list;

  target_list = ctk_drag_source_get_target_list (widget);
  if (target_list)
    ctk_target_list_ref (target_list);
  else
    target_list = ctk_target_list_new (NULL, 0);
  ctk_target_list_add_image_targets (target_list, 0, TRUE);
  ctk_drag_source_set_target_list (widget, target_list);
  ctk_target_list_unref (target_list);
}

/**
 * ctk_drag_source_add_uri_targets: (method)
 * @widget: a #CtkWidget that’s is a drag source
 *
 * Add the URI targets supported by #CtkSelectionData to
 * the target list of the drag source.  The targets
 * are added with @info = 0. If you need another value, 
 * use ctk_target_list_add_uri_targets() and
 * ctk_drag_source_set_target_list().
 * 
 * Since: 2.6
 */
void
ctk_drag_source_add_uri_targets (CtkWidget *widget)
{
  CtkTargetList *target_list;

  target_list = ctk_drag_source_get_target_list (widget);
  if (target_list)
    ctk_target_list_ref (target_list);
  else
    target_list = ctk_target_list_new (NULL, 0);
  ctk_target_list_add_uri_targets (target_list, 0);
  ctk_drag_source_set_target_list (widget, target_list);
  ctk_target_list_unref (target_list);
}

/**
 * ctk_drag_source_set_icon_pixbuf: (method)
 * @widget: a #CtkWidget
 * @pixbuf: the #CdkPixbuf for the drag icon
 * 
 * Sets the icon that will be used for drags from a particular widget
 * from a #CdkPixbuf. CTK+ retains a reference for @pixbuf and will 
 * release it when it is no longer needed.
 */
void 
ctk_drag_source_set_icon_pixbuf (CtkWidget *widget,
                                 CdkPixbuf *pixbuf)
{
  CtkDragSourceSite *site;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (CDK_IS_PIXBUF (pixbuf));

  site = g_object_get_data (G_OBJECT (widget), "ctk-site-data");
  g_return_if_fail (site != NULL); 

  g_clear_pointer (&site->image_def, ctk_image_definition_unref);
  site->image_def = ctk_image_definition_new_pixbuf (pixbuf, 1);
}

/**
 * ctk_drag_source_set_icon_stock: (method)
 * @widget: a #CtkWidget
 * @stock_id: the ID of the stock icon to use
 *
 * Sets the icon that will be used for drags from a particular source
 * to a stock icon.
 *
 * Deprecated: 3.10: Use ctk_drag_source_set_icon_name() instead.
 */
void
ctk_drag_source_set_icon_stock (CtkWidget   *widget,
                                const gchar *stock_id)
{
  CtkDragSourceSite *site;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (stock_id != NULL);

  site = g_object_get_data (G_OBJECT (widget), "ctk-site-data");
  g_return_if_fail (site != NULL);

  ctk_image_definition_unref (site->image_def);
  site->image_def = ctk_image_definition_new_stock (stock_id);
}

/**
 * ctk_drag_source_set_icon_name: (method)
 * @widget: a #CtkWidget
 * @icon_name: name of icon to use
 * 
 * Sets the icon that will be used for drags from a particular source
 * to a themed icon. See the docs for #CtkIconTheme for more details.
 *
 * Since: 2.8
 */
void 
ctk_drag_source_set_icon_name (CtkWidget   *widget,
                               const gchar *icon_name)
{
  CtkDragSourceSite *site;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (icon_name != NULL);

  site = g_object_get_data (G_OBJECT (widget), "ctk-site-data");
  g_return_if_fail (site != NULL);

  ctk_image_definition_unref (site->image_def);
  site->image_def = ctk_image_definition_new_icon_name (icon_name);
}

/**
 * ctk_drag_source_set_icon_gicon: (method)
 * @widget: a #CtkWidget
 * @icon: A #GIcon
 * 
 * Sets the icon that will be used for drags from a particular source
 * to @icon. See the docs for #CtkIconTheme for more details.
 *
 * Since: 3.2
 */
void
ctk_drag_source_set_icon_gicon (CtkWidget *widget,
                                GIcon     *icon)
{
  CtkDragSourceSite *site;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (icon != NULL);
  
  site = g_object_get_data (G_OBJECT (widget), "ctk-site-data");
  g_return_if_fail (site != NULL);

  ctk_image_definition_unref (site->image_def);
  site->image_def = ctk_image_definition_new_gicon (icon);
}

