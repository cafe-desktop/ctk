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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

#include <stdlib.h>
#include <string.h>

#include "cdk/cdk.h"

#include "ctkdnd.h"
#include "ctkdndprivate.h"
#include "deprecated/ctkiconfactory.h"
#include "ctkicontheme.h"
#include "ctkimageprivate.h"
#include "ctkinvisible.h"
#include "ctkmain.h"
#include "ctkoffscreenwindow.h"
#include "deprecated/ctkstock.h"
#include "ctkwindow.h"
#include "ctkintl.h"
#include "ctkquartz.h"
#include "cdk/quartz/cdkquartz.h"
#include "cdk/quartz/cdkquartz-ctk-only.h"
#include "cdk/quartz/cdkquartzdnd.h"
#include "ctkselectionprivate.h"
#include "ctksettings.h"
#include "ctkiconhelperprivate.h"

typedef struct _CtkDragSourceInfo CtkDragSourceInfo;
typedef struct _CtkDragDestInfo CtkDragDestInfo;
typedef struct _CtkDragFindData CtkDragFindData;

static void     ctk_drag_find_widget            (CtkWidget        *widget,
						 CtkDragFindData  *data);
static void     ctk_drag_dest_site_destroy      (gpointer          data);
static void     ctk_drag_dest_leave             (CtkWidget        *widget,
						 CdkDragContext   *context,
						 guint             time);
static CtkDragDestInfo *ctk_drag_get_dest_info  (CdkDragContext   *context,
						 gboolean          create);
static void ctk_drag_source_site_destroy        (gpointer           data);

static CtkDragSourceInfo *ctk_drag_get_source_info (CdkDragContext *context,
						    gboolean        create);

static void ctk_drag_drop_finished (CtkDragSourceInfo *info,
                                   CtkDragResult      result);

struct _CtkDragSourceInfo 
{
  CtkWidget         *source_widget;
  CtkWidget         *widget;
  CtkTargetList     *target_list; /* Targets for drag data */
  CdkDragAction      possible_actions; /* Actions allowed by source */
  CdkDragContext    *context;	  /* drag context */
  NSEvent           *nsevent;     /* what started it */
  gint hot_x, hot_y;		  /* Hot spot for drag */
  cairo_surface_t   *icon_surface;
  gboolean           success;
  gboolean           delete;
};

struct _CtkDragDestInfo 
{
  CtkWidget         *widget;	   /* Widget in which drag is in */
  CdkDragContext    *context;	   /* Drag context */
  guint              dropped : 1;     /* Set after we receive a drop */
  gint               drop_x, drop_y; /* Position of drop */
};

struct _CtkDragFindData 
{
  gint x;
  gint y;
  CdkDragContext *context;
  CtkDragDestInfo *info;
  gboolean found;
  gboolean toplevel;
  gboolean (*callback) (CtkWidget *widget, CdkDragContext *context,
			gint x, gint y, guint32 time);
  guint32 time;
};


@interface CtkDragSourceOwner : NSObject {
  CtkDragSourceInfo *info;
}

@end

@implementation CtkDragSourceOwner
-(void)pasteboard:(NSPasteboard *)sender provideDataForType:(NSString *)type
{
  guint target_info;
  CtkSelectionData selection_data;

  selection_data.selection = GDK_NONE;
  selection_data.data = NULL;
  selection_data.length = -1;
  selection_data.target = cdk_quartz_pasteboard_type_to_atom_libctk_only (type);
  selection_data.display = cdk_display_get_default ();

  if (ctk_target_list_find (info->target_list, 
			    selection_data.target, 
			    &target_info)) 
    {
      g_signal_emit_by_name (info->widget, "drag-data-get",
			     info->context,
			     &selection_data,
			     target_info,
			     time);

      if (selection_data.length >= 0)
        _ctk_quartz_set_selection_data_for_pasteboard (sender, &selection_data);
      
      g_free (selection_data.data);
    }
}

- (id)initWithInfo:(CtkDragSourceInfo *)anInfo
{
  self = [super init];

  if (self) 
    {
      info = anInfo;
    }

  return self;
}

@end

/**
 * ctk_drag_get_data: (method)
 * @widget: the widget that will receive the
 *   #CtkWidget::drag-data-received signal.
 * @context: the drag context
 * @target: the target (form of the data) to retrieve.
 * @time_: a timestamp for retrieving the data. This will
 *   generally be the time received in a #CtkWidget::drag-motion"
 *   or #CtkWidget::drag-drop" signal.
 */
void 
ctk_drag_get_data (CtkWidget      *widget,
		   CdkDragContext *context,
		   CdkAtom         target,
		   guint32         time)
{
  id <NSDraggingInfo> dragging_info;
  NSPasteboard *pasteboard;
  CtkSelectionData *selection_data;
  CtkDragDestInfo *info;
  CtkDragDestSite *site;

  dragging_info = cdk_quartz_drag_context_get_dragging_info_libctk_only (context);
  pasteboard = [dragging_info draggingPasteboard];

  info = ctk_drag_get_dest_info (context, FALSE);
  site = g_object_get_data (G_OBJECT (widget), "ctk-drag-dest");

  selection_data = _ctk_quartz_get_selection_data_from_pasteboard (pasteboard,
								   target, 0);

  if (site && site->target_list)
    {
      guint target_info;
      
      if (ctk_target_list_find (site->target_list, 
				selection_data->target,
				&target_info))
	{
	  if (!(site->flags & CTK_DEST_DEFAULT_DROP) ||
	      selection_data->length >= 0)
	    g_signal_emit_by_name (widget,
				   "drag-data-received",
				   context, info->drop_x, info->drop_y,
				   selection_data,
				   target_info, time);
	}
    }
  else
    {
      g_signal_emit_by_name (widget,
			     "drag-data-received",
			     context, info->drop_x, info->drop_y,
			     selection_data,
			     0, time);
    }
  
  if (site && site->flags & CTK_DEST_DEFAULT_DROP)
    {
      ctk_drag_finish (context, 
		       (selection_data->length >= 0),
		       (cdk_drag_context_get_selected_action (context) == GDK_ACTION_MOVE),
		       time);
    }      
}

/**
 * ctk_drag_finish: (method)
 * @context: the drag context.
 * @success: a flag indicating whether the drop was successful
 * @del: a flag indicating whether the source should delete the
 *   original data. (This should be %TRUE for a move)
 * @time_: the timestamp from the #CtkWidget::drag-drop signal.
 */
void 
ctk_drag_finish (CdkDragContext *context,
		 gboolean        success,
		 gboolean        del,
		 guint32         time)
{
  CtkDragSourceInfo *info;
  CdkDragContext* source_context = cdk_quartz_drag_source_context_libctk_only ();

  if (source_context)
    {
      info = ctk_drag_get_source_info (source_context, FALSE);
      if (info)
        {
          info->success = success;
          info->delete = del;
        }
    }
}

static void
ctk_drag_dest_info_destroy (gpointer data)
{
  CtkDragDestInfo *info = data;

  g_free (info);
}

static CtkDragDestInfo *
ctk_drag_get_dest_info (CdkDragContext *context,
			gboolean        create)
{
  CtkDragDestInfo *info;
  static GQuark info_quark = 0;
  if (!info_quark)
    info_quark = g_quark_from_static_string ("ctk-dest-info");
  
  info = g_object_get_qdata (G_OBJECT (context), info_quark);
  if (!info && create)
    {
      info = g_new (CtkDragDestInfo, 1);
      info->widget = NULL;
      info->context = context;
      info->dropped = FALSE;
      g_object_set_qdata_full (G_OBJECT (context), info_quark,
			       info, ctk_drag_dest_info_destroy);
    }

  return info;
}

static GQuark dest_info_quark = 0;

static CtkDragSourceInfo *
ctk_drag_get_source_info (CdkDragContext *context,
			  gboolean        create)
{
  CtkDragSourceInfo *info;

  if (!dest_info_quark)
    dest_info_quark = g_quark_from_static_string ("ctk-source-info");
  
  info = g_object_get_qdata (G_OBJECT (context), dest_info_quark);
  if (!info && create)
    {
      info = g_new0 (CtkDragSourceInfo, 1);
      info->context = context;
      g_object_set_qdata (G_OBJECT (context), dest_info_quark, info);
    }

  return info;
}

static void
ctk_drag_clear_source_info (CdkDragContext *context)
{
  g_object_set_qdata (G_OBJECT (context), dest_info_quark, NULL);
}

/**
 * ctk_drag_get_source_widget: (method)
 * @context: a (destination side) drag context
 *
 * Returns: (transfer none):
 */
CtkWidget *
ctk_drag_get_source_widget (CdkDragContext *context)
{
  CtkDragSourceInfo *info;
  CdkDragContext* real_source_context = cdk_quartz_drag_source_context_libctk_only ();

  if (!real_source_context)
    return NULL;

  info = ctk_drag_get_source_info (real_source_context, FALSE);
  if (!info)
     return NULL;

  return info->source_widget;
}

/**
 * ctk_drag_highlight: (method)
 * @widget: a widget to highlight
 */
void 
ctk_drag_highlight (CtkWidget  *widget)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  ctk_widget_set_state_flags (widget, CTK_STATE_FLAG_DROP_ACTIVE, FALSE);
}

/**
 * ctk_drag_unhighlight: (method)
 * @widget: a widget to remove the highlight from.
 */
void 
ctk_drag_unhighlight (CtkWidget *widget)
{
  g_return_if_fail (CTK_IS_WIDGET (widget));

  ctk_widget_unset_state_flags (widget, CTK_STATE_FLAG_DROP_ACTIVE);
}

static NSWindow *
get_toplevel_nswindow (CtkWidget *widget)
{
  CtkWidget *toplevel = ctk_widget_get_toplevel (widget);
  CdkWindow *window = ctk_widget_get_window (toplevel);

  /* Offscreen windows don't support drag and drop */
  if (CTK_IS_OFFSCREEN_WINDOW (toplevel))
    return NULL;

  if (ctk_widget_is_toplevel (toplevel) && window)
    return [cdk_quartz_window_get_nsview (window) window];
  else
    return NULL;
}

static void
register_types (CtkWidget *widget, CtkDragDestSite *site)
{
  if (site->target_list)
    {
      NSWindow *nswindow = get_toplevel_nswindow (widget);
      NSSet *types;
      NSAutoreleasePool *pool;

      if (!nswindow)
	return;

      pool = [[NSAutoreleasePool alloc] init];
      types = _ctk_quartz_target_list_to_pasteboard_types (site->target_list);

      [nswindow registerForDraggedTypes:[types allObjects]];

      [types release];
      [pool release];
    }
}

static void
ctk_drag_dest_realized (CtkWidget *widget, 
			gpointer   user_data)
{
  CtkDragDestSite *site = user_data;

  register_types (widget, site);
}

static void
ctk_drag_dest_hierarchy_changed (CtkWidget *widget,
				 CtkWidget *previous_toplevel,
				 gpointer   user_data)
{
  CtkDragDestSite *site = user_data;

  register_types (widget, site);
}

static void
ctk_drag_dest_site_destroy (gpointer data)
{
  CtkDragDestSite *site = data;
    
  if (site->target_list)
    ctk_target_list_unref (site->target_list);

  g_free (site);
}

/**
 * ctk_drag_dest_set: (method)
 * @widget: a #CtkWidget
 * @flags: which types of default drag behavior to use
 * @targets: (allow-none) (array length=n_targets): a pointer to an array of #CtkTargetEntrys
 *     indicating the drop types that this @widget will accept, or %NULL.
 *     Later you can access the list with ctk_drag_dest_get_target_list()
 *     and ctk_drag_dest_find_target().
 * @n_targets: the number of entries in @targets
 * @actions: a bitmask of possible actions for a drop onto this @widget.
 */
void 
ctk_drag_dest_set (CtkWidget            *widget,
		   CtkDestDefaults       flags,
		   const CtkTargetEntry *targets,
		   gint                  n_targets,
		   CdkDragAction         actions)
{
  CtkDragDestSite *old_site, *site;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  old_site = g_object_get_data (G_OBJECT (widget), "ctk-drag-dest");

  site = g_new0 (CtkDragDestSite, 1);
  site->flags = flags;
  site->have_drag = FALSE;
  if (targets)
    site->target_list = ctk_target_list_new (targets, n_targets);
  else
    site->target_list = NULL;
  site->actions = actions;

  if (old_site)
    site->track_motion = old_site->track_motion;
  else
    site->track_motion = FALSE;

  ctk_drag_dest_unset (widget);

  if (ctk_widget_get_realized (widget))
    ctk_drag_dest_realized (widget, site);

  g_signal_connect (widget, "realize",
		    G_CALLBACK (ctk_drag_dest_realized), site);
  g_signal_connect (widget, "hierarchy-changed",
		    G_CALLBACK (ctk_drag_dest_hierarchy_changed), site);

  g_object_set_data_full (G_OBJECT (widget), I_("ctk-drag-dest"),
			  site, ctk_drag_dest_site_destroy);
}

/**
 * ctk_drag_dest_set_proxy: (method)
 * @widget: a #CtkWidget
 * @proxy_window: the window to which to forward drag events
 * @protocol: the drag protocol which the @proxy_window accepts
 *   (You can use cdk_drag_get_protocol() to determine this)
 * @use_coordinates: If %TRUE, send the same coordinates to the
 *   destination, because it is an embedded
 *   subwindow.
 */
void 
ctk_drag_dest_set_proxy (CtkWidget      *widget,
			 CdkWindow      *proxy_window,
			 CdkDragProtocol protocol,
			 gboolean        use_coordinates)
{
  g_warning ("ctk_drag_dest_set_proxy is not supported on Mac OS X.");
}

/**
 * ctk_drag_dest_unset: (method)
 * @widget: a #CtkWidget
 */
void 
ctk_drag_dest_unset (CtkWidget *widget)
{
  CtkDragDestSite *old_site;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  old_site = g_object_get_data (G_OBJECT (widget), "ctk-drag-dest");
  if (old_site)
    {
      g_signal_handlers_disconnect_by_func (widget,
                                            ctk_drag_dest_realized,
                                            old_site);
      g_signal_handlers_disconnect_by_func (widget,
                                            ctk_drag_dest_hierarchy_changed,
                                            old_site);
    }

  g_object_set_data (G_OBJECT (widget), I_("ctk-drag-dest"), NULL);
}

/**
 * ctk_drag_dest_get_target_list: (method)
 * @widget: a #CtkWidget
 * Returns: (nullable) (transfer none): the #CtkTargetList, or %NULL if none
 */
CtkTargetList*
ctk_drag_dest_get_target_list (CtkWidget *widget)
{
  CtkDragDestSite *site;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);
  
  site = g_object_get_data (G_OBJECT (widget), "ctk-drag-dest");

  return site ? site->target_list : NULL;  
}

/**
 * ctk_drag_dest_set_target_list: (method)
 * @widget: a #CtkWidget that’s a drag destination
 * @target_list: (allow-none): list of droppable targets, or %NULL for none
 */
void
ctk_drag_dest_set_target_list (CtkWidget      *widget,
                               CtkTargetList  *target_list)
{
  CtkDragDestSite *site;

  g_return_if_fail (CTK_IS_WIDGET (widget));
  
  site = g_object_get_data (G_OBJECT (widget), "ctk-drag-dest");
  
  if (!site)
    {
      g_warning ("Can't set a target list on a widget until you've called ctk_drag_dest_set() "
                 "to make the widget into a drag destination");
      return;
    }

  if (target_list)
    ctk_target_list_ref (target_list);
  
  if (site->target_list)
    ctk_target_list_unref (site->target_list);

  site->target_list = target_list;

  register_types (widget, site);
}

/**
 * ctk_drag_dest_add_text_targets: (method)
 * @widget: a #CtkWidget that’s a drag destination
 */
void
ctk_drag_dest_add_text_targets (CtkWidget *widget)
{
  CtkTargetList *target_list;

  target_list = ctk_drag_dest_get_target_list (widget);
  if (target_list)
    ctk_target_list_ref (target_list);
  else
    target_list = ctk_target_list_new (NULL, 0);
  ctk_target_list_add_text_targets (target_list, 0);
  ctk_drag_dest_set_target_list (widget, target_list);
  ctk_target_list_unref (target_list);
}


/**
 * ctk_drag_dest_add_image_targets: (method)
 * @widget: a #CtkWidget that’s a drag destination
 */
void
ctk_drag_dest_add_image_targets (CtkWidget *widget)
{
  CtkTargetList *target_list;

  target_list = ctk_drag_dest_get_target_list (widget);
  if (target_list)
    ctk_target_list_ref (target_list);
  else
    target_list = ctk_target_list_new (NULL, 0);
  ctk_target_list_add_image_targets (target_list, 0, FALSE);
  ctk_drag_dest_set_target_list (widget, target_list);
  ctk_target_list_unref (target_list);
}

/**
 * ctk_drag_dest_add_uri_targets: (method)
 * @widget: a #CtkWidget that’s a drag destination
 */
void
ctk_drag_dest_add_uri_targets (CtkWidget *widget)
{
  CtkTargetList *target_list;

  target_list = ctk_drag_dest_get_target_list (widget);
  if (target_list)
    ctk_target_list_ref (target_list);
  else
    target_list = ctk_target_list_new (NULL, 0);
  ctk_target_list_add_uri_targets (target_list, 0);
  ctk_drag_dest_set_target_list (widget, target_list);
  ctk_target_list_unref (target_list);
}

static void
prepend_and_ref_widget (CtkWidget *widget,
			gpointer   data)
{
  GSList **slist_p = data;

  *slist_p = g_slist_prepend (*slist_p, g_object_ref (widget));
}

static void
ctk_drag_find_widget (CtkWidget       *widget,
		      CtkDragFindData *data)
{
  CtkAllocation new_allocation;
  gint allocation_to_window_x = 0;
  gint allocation_to_window_y = 0;
  gint x_offset = 0;
  gint y_offset = 0;

  if (data->found || !ctk_widget_get_mapped (widget) || !ctk_widget_get_sensitive (widget))
    return;

  /* Note that in the following code, we only count the
   * position as being inside a WINDOW widget if it is inside
   * widget->window; points that are outside of widget->window
   * but within the allocation are not counted. This is consistent
   * with the way we highlight drag targets.
   *
   * data->x,y are relative to widget->parent->window (if
   * widget is not a toplevel, widget->window otherwise).
   * We compute the allocation of widget in the same coordinates,
   * clipping to widget->window, and all intermediate
   * windows. If data->x,y is inside that, then we translate
   * our coordinates to be relative to widget->window and
   * recurse.
   */  
  ctk_widget_get_allocation (widget, &new_allocation);

  if (ctk_widget_get_parent (widget))
    {
      gint tx, ty;
      CdkWindow *window = ctk_widget_get_window (widget);
      CdkWindow *parent_window;
      CtkAllocation allocation;

      parent_window = ctk_widget_get_window (ctk_widget_get_parent (widget));

      /* Compute the offset from allocation-relative to
       * window-relative coordinates.
       */
      ctk_widget_get_allocation (widget, &allocation);
      allocation_to_window_x = allocation.x;
      allocation_to_window_y = allocation.y;

      if (ctk_widget_get_has_window (widget))
	{
	  /* The allocation is relative to the parent window for
	   * window widgets, not to widget->window.
	   */
          cdk_window_get_position (window, &tx, &ty);
	  
          allocation_to_window_x -= tx;
          allocation_to_window_y -= ty;
	}

      new_allocation.x = 0 + allocation_to_window_x;
      new_allocation.y = 0 + allocation_to_window_y;
      
      while (window && window != parent_window)
	{
	  CdkRectangle window_rect = { 0, 0, 0, 0 };
	  
          window_rect.width = cdk_window_get_width (window);
          window_rect.height = cdk_window_get_height (window);

	  cdk_rectangle_intersect (&new_allocation, &window_rect, &new_allocation);

	  cdk_window_get_position (window, &tx, &ty);
	  new_allocation.x += tx;
	  x_offset += tx;
	  new_allocation.y += ty;
	  y_offset += ty;
	  
	  window = cdk_window_get_parent (window);
	}

      if (!window)		/* Window and widget heirarchies didn't match. */
	return;
    }

  if (data->toplevel ||
      ((data->x >= new_allocation.x) && (data->y >= new_allocation.y) &&
       (data->x < new_allocation.x + new_allocation.width) && 
       (data->y < new_allocation.y + new_allocation.height)))
    {
      /* First, check if the drag is in a valid drop site in
       * one of our children 
       */
      if (CTK_IS_CONTAINER (widget))
	{
	  CtkDragFindData new_data = *data;
	  GSList *children = NULL;
	  GSList *tmp_list;
	  
	  new_data.x -= x_offset;
	  new_data.y -= y_offset;
	  new_data.found = FALSE;
	  new_data.toplevel = FALSE;
	  
	  /* need to reference children temporarily in case the
	   * ::drag-motion/::drag-drop callbacks change the widget hierarchy.
	   */
	  ctk_container_forall (CTK_CONTAINER (widget), prepend_and_ref_widget, &children);
	  for (tmp_list = children; tmp_list; tmp_list = tmp_list->next)
	    {
	      if (!new_data.found && ctk_widget_is_drawable (tmp_list->data))
		ctk_drag_find_widget (tmp_list->data, &new_data);
	      g_object_unref (tmp_list->data);
	    }
	  g_slist_free (children);
	  
	  data->found = new_data.found;
	}

      /* If not, and this widget is registered as a drop site, check to
       * emit "drag-motion" to check if we are actually in
       * a drop site.
       */
      if (!data->found &&
	  g_object_get_data (G_OBJECT (widget), "ctk-drag-dest"))
	{
	  data->found = data->callback (widget,
					data->context,
					data->x - x_offset - allocation_to_window_x,
					data->y - y_offset - allocation_to_window_y,
					data->time);
	  /* If so, send a "drag-leave" to the last widget */
	  if (data->found)
	    {
	      if (data->info->widget && data->info->widget != widget)
		{
		  ctk_drag_dest_leave (data->info->widget, data->context, data->time);
		}
	      data->info->widget = widget;
	    }
	}
    }
}

static void  
ctk_drag_dest_leave (CtkWidget      *widget,
		     CdkDragContext *context,
		     guint           time)
{
  CtkDragDestSite *site;

  site = g_object_get_data (G_OBJECT (widget), "ctk-drag-dest");
  g_return_if_fail (site != NULL);

  if ((site->flags & CTK_DEST_DEFAULT_HIGHLIGHT) && site->have_drag)
    ctk_drag_unhighlight (widget);
  
  if (!(site->flags & CTK_DEST_DEFAULT_MOTION) || site->have_drag ||
      site->track_motion)
    g_signal_emit_by_name (widget, "drag-leave", context, time);
  
  site->have_drag = FALSE;
}

static gboolean
ctk_drag_dest_motion (CtkWidget	     *widget,
		      CdkDragContext *context,
		      gint            x,
		      gint            y,
		      guint           time)
{
  CtkDragDestSite *site;
  CdkDragAction action = 0;
  gboolean retval;

  site = g_object_get_data (G_OBJECT (widget), "ctk-drag-dest");
  g_return_val_if_fail (site != NULL, FALSE);

  if (site->track_motion || site->flags & CTK_DEST_DEFAULT_MOTION)
    {
      if (cdk_drag_context_get_suggested_action (context) & site->actions)
	action = cdk_drag_context_get_suggested_action (context);
      
      if (action && ctk_drag_dest_find_target (widget, context, NULL))
	{
	  if (!site->have_drag)
	    {
	      site->have_drag = TRUE;
	      if (site->flags & CTK_DEST_DEFAULT_HIGHLIGHT)
		ctk_drag_highlight (widget);
	    }
	  
	  cdk_drag_status (context, action, time);
	}
      else
	{
	  cdk_drag_status (context, 0, time);
	  if (!site->track_motion)
	    return TRUE;
	}
    }

  g_signal_emit_by_name (widget, "drag-motion",
			 context, x, y, time, &retval);

  return (site->flags & CTK_DEST_DEFAULT_MOTION) ? TRUE : retval;
}

static gboolean
ctk_drag_dest_drop (CtkWidget	     *widget,
		    CdkDragContext   *context,
		    gint              x,
		    gint              y,
		    guint             time)
{
  CtkDragDestSite *site;
  CtkDragDestInfo *info;
  gboolean retval;

  site = g_object_get_data (G_OBJECT (widget), "ctk-drag-dest");
  g_return_val_if_fail (site != NULL, FALSE);

  info = ctk_drag_get_dest_info (context, FALSE);
  g_return_val_if_fail (info != NULL, FALSE);

  info->drop_x = x;
  info->drop_y = y;

  if (site->flags & CTK_DEST_DEFAULT_DROP)
    {
      CdkAtom target = ctk_drag_dest_find_target (widget, context, NULL);

      if (target == GDK_NONE)
	{
	  ctk_drag_finish (context, FALSE, FALSE, time);
	  return TRUE;
	}
      else
	ctk_drag_get_data (widget, context, target, time);
    }
  
  g_signal_emit_by_name (widget, "drag-drop",
			 context, x, y, time, &retval);

  return (site->flags & CTK_DEST_DEFAULT_DROP) ? TRUE : retval;
}

/**
 * ctk_drag_dest_set_track_motion: (method)
 * @widget: a #CtkWidget that’s a drag destination
 * @track_motion: whether to accept all targets
 */
void
ctk_drag_dest_set_track_motion (CtkWidget *widget,
				gboolean   track_motion)
{
  CtkDragDestSite *site;

  g_return_if_fail (CTK_IS_WIDGET (widget));

  site = g_object_get_data (G_OBJECT (widget), "ctk-drag-dest");
  
  g_return_if_fail (site != NULL);

  site->track_motion = track_motion != FALSE;
}

/**
 * ctk_drag_dest_get_track_motion: (method)
 * @widget: a #CtkWidget that’s a drag destination
 */
gboolean
ctk_drag_dest_get_track_motion (CtkWidget *widget)
{
  CtkDragDestSite *site;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  site = g_object_get_data (G_OBJECT (widget), "ctk-drag-dest");

  if (site)
    return site->track_motion;

  return FALSE;
}

void
_ctk_drag_dest_handle_event (CtkWidget *toplevel,
			     CdkEvent  *event)
{
  CtkDragDestInfo *info;
  CdkDragContext *context;

  g_return_if_fail (toplevel != NULL);
  g_return_if_fail (event != NULL);

  context = event->dnd.context;

  info = ctk_drag_get_dest_info (context, TRUE);

  /* Find the widget for the event */
  switch (event->type)
    {
    case GDK_DRAG_ENTER:
      break;

    case GDK_DRAG_LEAVE:
      if (info->widget)
	{
	  ctk_drag_dest_leave (info->widget, context, event->dnd.time);
	  info->widget = NULL;
	}
      break;

    case GDK_DRAG_MOTION:
    case GDK_DROP_START:
      {
	CtkDragFindData data;
	gint tx, ty;

	if (event->type == GDK_DROP_START)
	  {
	    info->dropped = TRUE;
	    /* We send a leave here so that the widget unhighlights
	     * properly.
	     */
	    if (info->widget)
	      {
		ctk_drag_dest_leave (info->widget, context, event->dnd.time);
		info->widget = NULL;
	      }
	  }

	cdk_window_get_position (ctk_widget_get_window (toplevel), &tx, &ty);
	
	data.x = event->dnd.x_root - tx;
	data.y = event->dnd.y_root - ty;
 	data.context = context;
	data.info = info;
	data.found = FALSE;
	data.toplevel = TRUE;
	data.callback = (event->type == GDK_DRAG_MOTION) ?
	  ctk_drag_dest_motion : ctk_drag_dest_drop;
	data.time = event->dnd.time;
	
	ctk_drag_find_widget (toplevel, &data);

	if (info->widget && !data.found)
	  {
	    ctk_drag_dest_leave (info->widget, context, event->dnd.time);
	    info->widget = NULL;
	  }

	/* Send a reply.
	 */
	if (event->type == GDK_DRAG_MOTION)
	  {
	    if (!data.found)
	      cdk_drag_status (context, 0, event->dnd.time);
	  }

	break;
      default:
	g_assert_not_reached ();
      }
    }
}


/**
 * ctk_drag_dest_find_target: (method)
 * @widget: drag destination widget
 * @context: drag context
 * @target_list: (allow-none): list of droppable targets, or %NULL to use
 *    ctk_drag_dest_get_target_list (@widget).
 *
 * Returns: (transfer none):
 */
CdkAtom
ctk_drag_dest_find_target (CtkWidget      *widget,
                           CdkDragContext *context,
                           CtkTargetList  *target_list)
{
  id <NSDraggingInfo> dragging_info;
  NSPasteboard *pasteboard;
  CtkWidget *source_widget;
  GList *tmp_target;
  GList *tmp_source = NULL;
  GList *source_targets;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), GDK_NONE);
  g_return_val_if_fail (GDK_IS_DRAG_CONTEXT (context), GDK_NONE);

  dragging_info = cdk_quartz_drag_context_get_dragging_info_libctk_only (context);
  pasteboard = [dragging_info draggingPasteboard];

  source_widget = ctk_drag_get_source_widget (context);

  if (target_list == NULL)
    target_list = ctk_drag_dest_get_target_list (widget);
  
  if (target_list == NULL)
    return GDK_NONE;

  source_targets = _ctk_quartz_pasteboard_types_to_atom_list ([pasteboard types]);
  tmp_target = target_list->list;
  while (tmp_target)
    {
      CtkTargetPair *pair = tmp_target->data;
      tmp_source = source_targets;
      while (tmp_source)
	{
	  if (tmp_source->data == GUINT_TO_POINTER (pair->target))
	    {
	      if ((!(pair->flags & CTK_TARGET_SAME_APP) || source_widget) &&
		  (!(pair->flags & CTK_TARGET_SAME_WIDGET) || (source_widget == widget)))
		{
		  g_list_free (source_targets);
		  return pair->target;
		}
	      else
		break;
	    }
	  tmp_source = tmp_source->next;
	}
      tmp_target = tmp_target->next;
    }

  g_list_free (source_targets);
  return GDK_NONE;
}

static gboolean
ctk_drag_begin_idle (gpointer arg)
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  CdkDragContext* context = (CdkDragContext*) arg;
  CtkDragSourceInfo* info = ctk_drag_get_source_info (context, FALSE);
  NSWindow *nswindow;
  NSPasteboard *pasteboard;
  CtkDragSourceOwner *owner;
  NSPoint point;
  NSSet *types;
  NSImage *drag_image;

  g_assert (info != NULL);

  pasteboard = [NSPasteboard pasteboardWithName:NSDragPboard];
  owner = [[CtkDragSourceOwner alloc] initWithInfo:info];

  types = _ctk_quartz_target_list_to_pasteboard_types (info->target_list);

  [pasteboard declareTypes:[types allObjects] owner:owner];

  [owner release];
  [types release];

  if ((nswindow = get_toplevel_nswindow (info->source_widget)) == NULL)
     return G_SOURCE_REMOVE;
  
  /* Ref the context. It's unreffed when the drag has been aborted */
  g_object_ref (info->context);

  /* FIXME: If the event isn't a mouse event, use the global cursor position instead */
  point = [info->nsevent locationInWindow];

  drag_image = _ctk_quartz_create_image_from_surface (info->icon_surface);

  if (drag_image == NULL)
    {
      g_object_unref (info->context);
      return G_SOURCE_REMOVE;
    }

  point.x -= info->hot_x;
  point.y -= [drag_image size].height - info->hot_y;

  [nswindow dragImage:drag_image
                   at:point
               offset:NSZeroSize
                event:info->nsevent
           pasteboard:pasteboard
               source:nswindow
            slideBack:YES];

  [info->nsevent release];
  [drag_image release];

  [pool release];

  return G_SOURCE_REMOVE;
}
/* Fake protocol to let us call CdkNSView cdkWindow without including
 * cdk/CdkNSView.h (which we can’t because it pulls in the internal-only
 * cdkwindow.h).
 */
@protocol CdkNSView
- (CdkWindow *)cdkWindow;
@end

CdkDragContext *
ctk_drag_begin_internal (CtkWidget         *widget,
			 gboolean          *out_needs_icon,
			 CtkTargetList     *target_list,
			 CdkDragAction      actions,
			 gint               button,
			 const CdkEvent    *event,
			 int               x,
			 int               y)
{
  CtkDragSourceInfo *info;
  CdkDevice *pointer;
  CdkWindow *window;
  CdkDragContext *context;
  NSWindow *nswindow = get_toplevel_nswindow (widget);
  NSPoint point = {0, 0};
  double time = (double)g_get_real_time ();
  NSEvent *nsevent;
  NSTimeInterval nstime;

  if ((x != -1 && y != -1) || event)
    {
      CdkWindow *window;
      gdouble dx, dy;
      if (x != -1 && y != -1)
	{
	  CtkWidget *toplevel = ctk_widget_get_toplevel (widget);
	  window = ctk_widget_get_window (toplevel);
	  ctk_widget_translate_coordinates (widget, toplevel, x, y, &x, &y);
	  cdk_window_get_root_coords (ctk_widget_get_window (toplevel), x, y,
							     &x, &y);
	  dx = (gdouble)x;
	  dy = (gdouble)y;
	}
      else if (event)
	{
	  if (cdk_event_get_coords (event, &dx, &dy))
	    {
	      /* We need to translate (x, y) to coordinates relative to the
	       * toplevel CdkWindow, which should be the CdkWindow backing
	       * nswindow. Then, we convert to the NSWindow coordinate system.
	       */
	      window = event->any.window;
	      CdkWindow *toplevel = cdk_window_get_effective_toplevel (window);

	      while (window != toplevel)
		{
		  double old_x = dx;
		  double old_y = dy;

		  cdk_window_coords_to_parent (window, old_x, old_y,
					       &dx, &dy);
		  window = cdk_window_get_effective_parent (window);
		}
	    }
	  time = (double)cdk_event_get_time (event);
	}
      point.x = dx;
      point.y = cdk_window_get_height (window) - dy;
    }

  nstime = [[NSDate dateWithTimeIntervalSince1970: time / 1000] timeIntervalSinceReferenceDate];
  nsevent = [NSEvent mouseEventWithType: NSLeftMouseDown
                      location: point
                      modifierFlags: 0
                      timestamp: nstime
                      windowNumber: [nswindow windowNumber]
                      context: [nswindow graphicsContext]
                      eventNumber: 0
                      clickCount: 1
                      pressure: 0.0 ];

  window = [(id<CdkNSView>)[nswindow contentView] cdkWindow];
  g_return_val_if_fail (nsevent != NULL, NULL);
  g_return_val_if_fail (target_list != NULL, NULL);
  
  context = cdk_drag_begin (window, g_list_copy (target_list->list));
  g_return_val_if_fail (context != NULL, NULL);

  info = ctk_drag_get_source_info (context, TRUE);
  info->nsevent = nsevent;
  [info->nsevent retain];

  info->source_widget = g_object_ref (widget);
  info->widget = g_object_ref (widget);
  info->target_list = target_list;
  ctk_target_list_ref (target_list);

  info->possible_actions = actions;

  g_signal_emit_by_name (widget, "drag-begin", info->context);

  /* Ensure that we have an icon before we start the drag; the
   * application may have set one in ::drag_begin, or it may
   * not have set one.
   */
  if (info->icon_surface == NULL && out_needs_icon == NULL)
    ctk_drag_set_icon_default (context);

  if (out_needs_icon != NULL)
    *out_needs_icon = (info->icon_surface == NULL);

  /* no image def or no supported type -> set the default */
  if (!info->icon_surface)
    ctk_drag_set_icon_default (context);

  /* drag will begin in an idle handler to avoid nested run loops */

  g_idle_add_full (G_PRIORITY_HIGH_IDLE, ctk_drag_begin_idle, context, NULL);

  pointer = cdk_drag_context_get_device (info->context);
  cdk_device_ungrab (pointer, 0);

  return context;
}

/**
 * ctk_drag_begin_with_coordinates: (method)
 * @widget:
 * @targets:
 * @actions:
 * @button:
 * @event:
 * @x:
 * @y:
 *
 * Returns: (transfer none):
 */
CdkDragContext *
ctk_drag_begin_with_coordinates (CtkWidget         *widget,
				 CtkTargetList     *targets,
				 CdkDragAction      actions,
				 gint               button,
				 CdkEvent          *event,
				 gint               x,
				 gint               y)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);
  g_return_val_if_fail (ctk_widget_get_realized (widget), NULL);
  g_return_val_if_fail (targets != NULL, NULL);

  return ctk_drag_begin_internal (widget, NULL, targets,
				  actions, button, event, x, y);
}
/**
 * ctk_drag_begin: (method)
 * @widget: the source widget.
 * @targets: The targets (data formats) in which the
 *    source can provide the data.
 * @actions: A bitmask of the allowed drag actions for this drag.
 * @button: The button the user clicked to start the drag.
 * @event: The event that triggered the start of the drag.
 *
 * Returns: (transfer none):
 */
CdkDragContext *
ctk_drag_begin (CtkWidget         *widget,
		CtkTargetList     *targets,
		CdkDragAction      actions,
		gint               button,
		CdkEvent          *event)
{
  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);
  g_return_val_if_fail (ctk_widget_get_realized (widget), NULL);
  g_return_val_if_fail (targets != NULL, NULL);

  return ctk_drag_begin_internal (widget, NULL, targets,
				  actions, button, event, -1, -1);
}


/**
 * ctk_drag_cancel:
 * @context: a #CdkDragContext, as e.g. returned by ctk_drag_begin_with_coordinates()
 *
 */
void
ctk_drag_cancel (CdkDragContext *context)
{
  CtkDragSourceInfo *info;

  g_return_if_fail (GDK_IS_DRAG_CONTEXT (context));

  info = ctk_drag_get_source_info (context, FALSE);
  if (info != NULL)
    ctk_drag_drop_finished (info, CTK_DRAG_RESULT_ERROR);
}


/**
 * ctk_drag_set_icon_widget: (method)
 * @context: the context for a drag. (This must be called 
          with a  context for the source side of a drag)
 * @widget: a toplevel window to use as an icon.
 * @hot_x: the X offset within @widget of the hotspot.
 * @hot_y: the Y offset within @widget of the hotspot.
 * 
 * Changes the icon for a widget to a given widget. CTK+
 * will not destroy the icon, so if you don’t want
 * it to persist, you should connect to the “drag-end” 
 * signal and destroy it yourself.
 **/
void 
ctk_drag_set_icon_widget (CdkDragContext    *context,
			  CtkWidget         *widget,
			  gint               hot_x,
			  gint               hot_y)
{
  g_return_if_fail (GDK_IS_DRAG_CONTEXT (context));
  g_return_if_fail (CTK_IS_WIDGET (widget));

  g_warning ("ctk_drag_set_icon_widget is not supported on Mac OS X");
}

static void
set_icon_stock_pixbuf (CdkDragContext    *context,
		       const gchar       *stock_id,
		       CdkPixbuf         *pixbuf,
		       gint               hot_x,
		       gint               hot_y)
{
  CtkDragSourceInfo *info;
  cairo_surface_t *surface;
  cairo_t *cr;

  info = ctk_drag_get_source_info (context, FALSE);

  if (stock_id)
    {
      pixbuf = ctk_widget_render_icon_pixbuf (info->widget, stock_id,
				              CTK_ICON_SIZE_DND);

      if (!pixbuf)
	{
	  g_warning ("Cannot load drag icon from stock_id %s", stock_id);
	  return;
	}
    }
  else
    g_object_ref (pixbuf);

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                        cdk_pixbuf_get_width (pixbuf),
                                        cdk_pixbuf_get_height (pixbuf));

  cr = cairo_create (surface);
  cdk_cairo_set_source_pixbuf (cr, pixbuf, 0, 0);
  cairo_paint (cr);
  cairo_destroy (cr);
  g_object_unref (pixbuf);

  cairo_surface_set_device_offset (surface, -hot_x, -hot_y);
  ctk_drag_set_icon_surface (context, surface);
  cairo_surface_destroy (surface);
}

void
ctk_drag_set_icon_definition (CdkDragContext     *context,
                              CtkImageDefinition *def,
                              gint                hot_x,
                              gint                hot_y)
{
  switch (ctk_image_definition_get_storage_type (def))
    {
    case CTK_IMAGE_EMPTY:
      ctk_drag_set_icon_default (context);
      break;

    case CTK_IMAGE_PIXBUF:
      ctk_drag_set_icon_pixbuf (context,
                                ctk_image_definition_get_pixbuf (def),
                                hot_x, hot_y);
      break;

    case CTK_IMAGE_STOCK:
      ctk_drag_set_icon_stock (context,
                               ctk_image_definition_get_stock (def),
                               hot_x, hot_y);
      break;

    case CTK_IMAGE_ICON_NAME:
      ctk_drag_set_icon_name (context,
                              ctk_image_definition_get_icon_name (def),
                              hot_x, hot_y);
      break;

    default:
      g_warning ("FIXME: setting drag icon of type %u not implemented, using default.", ctk_image_definition_get_storage_type (def));
      ctk_drag_set_icon_default (context);
      break;
  }
}

/**
 * ctk_drag_set_icon_pixbuf:
 * @context: the context for a drag. (This must be called 
 *            with a  context for the source side of a drag)
 * @pixbuf: the #CdkPixbuf to use as the drag icon.
 * @hot_x: the X offset within @widget of the hotspot.
 * @hot_y: the Y offset within @widget of the hotspot.
 * 
 * Sets @pixbuf as the icon for a given drag.
 **/
void 
ctk_drag_set_icon_pixbuf  (CdkDragContext *context,
			   CdkPixbuf      *pixbuf,
			   gint            hot_x,
			   gint            hot_y)
{
  g_return_if_fail (GDK_IS_DRAG_CONTEXT (context));
  g_return_if_fail (GDK_IS_PIXBUF (pixbuf));

  set_icon_stock_pixbuf (context, NULL, pixbuf, hot_x, hot_y);
}

/**
 * ctk_drag_set_icon_stock:
 * @context: the context for a drag. (This must be called 
 *            with a  context for the source side of a drag)
 * @stock_id: the ID of the stock icon to use for the drag.
 * @hot_x: the X offset within the icon of the hotspot.
 * @hot_y: the Y offset within the icon of the hotspot.
 * 
 * Sets the icon for a given drag from a stock ID.
 **/
void 
ctk_drag_set_icon_stock  (CdkDragContext *context,
			  const gchar    *stock_id,
			  gint            hot_x,
			  gint            hot_y)
{

  g_return_if_fail (GDK_IS_DRAG_CONTEXT (context));
  g_return_if_fail (stock_id != NULL);

  set_icon_stock_pixbuf (context, stock_id, NULL, hot_x, hot_y);
}

/**
 * ctk_drag_set_icon_surface:
 * @context: the context for a drag. (This must be called
 *            with a context for the source side of a drag)
 * @surface: the surface to use as icon
 *
 * Sets @surface as the icon for a given drag. CTK+ retains
 * references for the arguments, and will release them when
 * they are no longer needed.
 *
 * To position the surface relative to the mouse, use
 * cairo_surface_set_device_offset() on @surface. The mouse
 * cursor will be positioned at the (0,0) coordinate of the
 * surface.
 **/
void
ctk_drag_set_icon_surface (CdkDragContext  *context,
                           cairo_surface_t *surface)
{
  double x_offset, y_offset;
  CtkDragSourceInfo *info;

  g_return_if_fail (GDK_IS_DRAG_CONTEXT (context));
  g_return_if_fail (surface != NULL);

  cairo_surface_get_device_offset (surface, &x_offset, &y_offset);
  info = ctk_drag_get_source_info (context, FALSE);
  cairo_surface_reference (surface);

  if (info->icon_surface)
    cairo_surface_destroy (info->icon_surface);

  info->icon_surface = surface;
  info->hot_x = -x_offset;
  info->hot_y = -y_offset;
}

/**
 * ctk_drag_set_icon_name:
 * @context: the context for a drag. (This must be called 
 *            with a context for the source side of a drag)
 * @icon_name: name of icon to use
 * @hot_x: the X offset of the hotspot within the icon
 * @hot_y: the Y offset of the hotspot within the icon
 * 
 * Sets the icon for a given drag from a named themed icon. See
 * the docs for #CtkIconTheme for more details. Note that the
 * size of the icon depends on the icon theme (the icon is
 * loaded at the symbolic size #CTK_ICON_SIZE_DND), thus 
 * @hot_x and @hot_y have to be used with care.
 *
 * Since: 2.8
 **/
void 
ctk_drag_set_icon_name (CdkDragContext *context,
			const gchar    *icon_name,
			gint            hot_x,
			gint            hot_y)
{
  CdkScreen *screen;
  CtkIconTheme *icon_theme;
  CdkPixbuf *pixbuf;
  gint width, height, icon_size;

  g_return_if_fail (GDK_IS_DRAG_CONTEXT (context));
  g_return_if_fail (icon_name != NULL);

  screen = cdk_window_get_screen (cdk_drag_context_get_source_window (context));
  g_return_if_fail (screen != NULL);

  ctk_icon_size_lookup (CTK_ICON_SIZE_DND, &width, &height);
  icon_size = MAX (width, height);

  icon_theme = ctk_icon_theme_get_for_screen (screen);

  pixbuf = ctk_icon_theme_load_icon (icon_theme, icon_name,
		  		     icon_size, 0, NULL);
  if (pixbuf)
    set_icon_stock_pixbuf (context, NULL, pixbuf, hot_x, hot_y);
  else
    g_warning ("Cannot load drag icon from icon name %s", icon_name);
}

/**
 * ctk_drag_set_icon_default: (method)
 * @context: the context for a drag. (This must be called 
             with a  context for the source side of a drag)
 * 
 * Sets the icon for a particular drag to the default
 * icon.
 **/
void 
ctk_drag_set_icon_default (CdkDragContext    *context)
{
  g_return_if_fail (GDK_IS_DRAG_CONTEXT (context));

  ctk_drag_set_icon_name (context, "text-x-generic", -2, -2);
}

static void
ctk_drag_source_info_destroy (CtkDragSourceInfo *info)
{
  NSPasteboard *pasteboard;
  NSAutoreleasePool *pool;

  if (info->icon_surface)
    cairo_surface_destroy (info->icon_surface);

  g_signal_emit_by_name (info->widget, "drag-end", 
			 info->context);

  if (info->source_widget)
    g_object_unref (info->source_widget);

  if (info->widget)
    g_object_unref (info->widget);

  ctk_target_list_unref (info->target_list);

  pool = [[NSAutoreleasePool alloc] init];

  /* Empty the pasteboard, so that it will not accidentally access
   * info->context after it has been destroyed.
   */
  pasteboard = [NSPasteboard pasteboardWithName: NSDragPboard];
  [pasteboard clearContents];

  [pool release];

  ctk_drag_clear_source_info (info->context);
  g_object_unref (info->context);

  g_free (info);
  info = NULL;
}

static gboolean
drag_drop_finished_idle_cb (gpointer data)
{
  CtkDragSourceInfo* info = (CtkDragSourceInfo*) data;
  if (info->success)
    ctk_drag_source_info_destroy (data);
  return G_SOURCE_REMOVE;
}

static void
ctk_drag_drop_finished (CtkDragSourceInfo *info,
                        CtkDragResult      result)
{
  gboolean success = (result == CTK_DRAG_RESULT_SUCCESS);

  if (!success)
    g_signal_emit_by_name (info->source_widget, "drag-failed",
                           info->context, result, &success);

  if (success && info->delete)
    g_signal_emit_by_name (info->source_widget, "drag-data-delete",
                           info->context);

  /* Workaround for the fact that the NS API blocks until the drag is
   * over. This way the context is still valid when returning from
   * drag_begin, even if it will still be quite useless. See bug #501588.
  */
  g_idle_add (drag_drop_finished_idle_cb, info);
}

/*************************************************************
 * _ctk_drag_source_handle_event:
 *     Called from widget event handling code on Drag events
 *     for drag sources.
 *
 *   arguments:
 *     toplevel: Toplevel widget that received the event
 *     event:
 *   results:
 *************************************************************/

void
_ctk_drag_source_handle_event (CtkWidget *widget,
			       CdkEvent  *event)
{
  CtkDragSourceInfo *info;
  CdkDragContext *context;
  CtkDragResult result;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (event != NULL);

  context = event->dnd.context;
  info = ctk_drag_get_source_info (context, FALSE);
  if (!info)
    return;

  switch (event->type)
    {
    case GDK_DROP_FINISHED:
      result = (cdk_drag_context_get_dest_window (context) != NULL) ? CTK_DRAG_RESULT_SUCCESS : CTK_DRAG_RESULT_NO_TARGET;
      ctk_drag_drop_finished (info, result);
      break;
    default:
      g_assert_not_reached ();
    }  
}

/**
 * ctk_drag_check_threshold: (method)
 * @widget: a #CtkWidget
 * @start_x: X coordinate of start of drag
 * @start_y: Y coordinate of start of drag
 * @current_x: current X coordinate
 * @current_y: current Y coordinate
 */
gboolean
ctk_drag_check_threshold (CtkWidget *widget,
			  gint       start_x,
			  gint       start_y,
			  gint       current_x,
			  gint       current_y)
{
  gint drag_threshold;

  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);

  g_object_get (ctk_widget_get_settings (widget),
		"ctk-dnd-drag-threshold", &drag_threshold,
		NULL);
  
  return (ABS (current_x - start_x) > drag_threshold ||
	  ABS (current_y - start_y) > drag_threshold);
}
