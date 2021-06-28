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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.Free
 */

/*
 * Modified by the CTK+ Team and others 1997-2006.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/. 
 */

#include "config.h"

#include "ctkmain.h"
#include "ctkmarshalers.h"
#include "ctksizerequest.h"
#include "ctkwin32embedwidget.h"
#include "ctkintl.h"
#include "ctkprivate.h"
#include "ctkwindowprivate.h"
#include "ctkwidgetprivate.h"
#include "ctkcontainerprivate.h"


static void            ctk_win32_embed_widget_realize               (CtkWidget        *widget);
static void            ctk_win32_embed_widget_unrealize             (CtkWidget        *widget);
static void            ctk_win32_embed_widget_show                  (CtkWidget        *widget);
static void            ctk_win32_embed_widget_hide                  (CtkWidget        *widget);
static void            ctk_win32_embed_widget_map                   (CtkWidget        *widget);
static void            ctk_win32_embed_widget_unmap                 (CtkWidget        *widget);
static void            ctk_win32_embed_widget_size_allocate         (CtkWidget        *widget,
								     CtkAllocation    *allocation);
static void            ctk_win32_embed_widget_set_focus             (CtkWindow        *window,
								     CtkWidget        *focus);
static gboolean        ctk_win32_embed_widget_focus                 (CtkWidget        *widget,
								     CtkDirectionType  direction);
static void            ctk_win32_embed_widget_check_resize          (CtkContainer     *container);

static CtkBinClass *bin_class = NULL;

G_DEFINE_TYPE (CtkWin32EmbedWidget, ctk_win32_embed_widget, CTK_TYPE_WINDOW)

static void
ctk_win32_embed_widget_class_init (CtkWin32EmbedWidgetClass *class)
{
  CtkWidgetClass *widget_class = (CtkWidgetClass *)class;
  CtkWindowClass *window_class = (CtkWindowClass *)class;
  CtkContainerClass *container_class = (CtkContainerClass *)class;

  bin_class = g_type_class_peek (CTK_TYPE_BIN);

  widget_class->realize = ctk_win32_embed_widget_realize;
  widget_class->unrealize = ctk_win32_embed_widget_unrealize;

  widget_class->show = ctk_win32_embed_widget_show;
  widget_class->hide = ctk_win32_embed_widget_hide;
  widget_class->map = ctk_win32_embed_widget_map;
  widget_class->unmap = ctk_win32_embed_widget_unmap;
  widget_class->size_allocate = ctk_win32_embed_widget_size_allocate;

  widget_class->focus = ctk_win32_embed_widget_focus;
  
  container_class->check_resize = ctk_win32_embed_widget_check_resize;

  window_class->set_focus = ctk_win32_embed_widget_set_focus;
}

static void
ctk_win32_embed_widget_init (CtkWin32EmbedWidget *embed_widget)
{
  _ctk_widget_set_is_toplevel (CTK_WIDGET (embed_widget), TRUE);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  ctk_container_set_resize_mode (CTK_CONTAINER (embed_widget), CTK_RESIZE_QUEUE);
G_GNUC_END_IGNORE_DEPRECATIONS;
}

CtkWidget*
_ctk_win32_embed_widget_new (HWND parent)
{
  CtkWin32EmbedWidget *embed_widget;

  embed_widget = g_object_new (CTK_TYPE_WIN32_EMBED_WIDGET, NULL);
  
  embed_widget->parent_window =
    cdk_win32_window_lookup_for_display (cdk_display_get_default (),
					 parent);
  
  if (!embed_widget->parent_window)
    embed_widget->parent_window =
      cdk_win32_window_foreign_new_for_display (cdk_display_get_default (),
					  parent);
  
  return CTK_WIDGET (embed_widget);
}

BOOL
_ctk_win32_embed_widget_dialog_procedure (CtkWin32EmbedWidget *embed_widget,
					  HWND wnd, UINT message, WPARAM wparam, LPARAM lparam)
{
  CtkAllocation allocation;
  CtkWidget *widget = CTK_WIDGET (embed_widget);
  
 if (message == WM_SIZE)
   {
     allocation.width = LOWORD(lparam);
     allocation.height = HIWORD(lparam);
     ctk_widget_set_allocation (widget, &allocation);

     ctk_widget_queue_resize (widget);
   }
        
 return 0;
}

static void
ctk_win32_embed_widget_unrealize (CtkWidget *widget)
{
  CtkWin32EmbedWidget *embed_widget = CTK_WIN32_EMBED_WIDGET (widget);

  embed_widget->old_window_procedure = NULL;
  
  g_clear_object (&embed_widget->parent_window);

  CTK_WIDGET_CLASS (ctk_win32_embed_widget_parent_class)->unrealize (widget);
}

static LRESULT CALLBACK
ctk_win32_embed_widget_window_process (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  GdkWindow *window;
  CtkWin32EmbedWidget *embed_widget;
  gpointer user_data;

  window = cdk_win32_window_lookup_for_display (cdk_display_get_default (), hwnd);
  if (window == NULL) {
    g_warning ("No such window!");
    return 0;
  }
  cdk_window_get_user_data (window, &user_data);
  embed_widget = CTK_WIN32_EMBED_WIDGET (user_data);

  if (msg == WM_GETDLGCODE) {
    return DLGC_WANTALLKEYS;
  }

  if (embed_widget && embed_widget->old_window_procedure)
    return CallWindowProc(embed_widget->old_window_procedure,
			  hwnd, msg, wparam, lparam);
  else
    return 0;
}

static void
ctk_win32_embed_widget_realize (CtkWidget *widget)
{
  CtkWindow *window = CTK_WINDOW (widget);
  CtkWin32EmbedWidget *embed_widget = CTK_WIN32_EMBED_WIDGET (widget);
  CtkAllocation allocation;
  GdkWindow *cdk_window;
  GdkWindowAttr attributes;
  gint attributes_mask;
  LONG_PTR styles;

  ctk_widget_get_allocation (widget, &allocation);

  /* ensure widget tree is properly size allocated */
  if (allocation.x == -1 && allocation.y == -1 &&
      allocation.width == 1 && allocation.height == 1)
    {
      CtkRequisition requisition;
      CtkAllocation allocation = { 0, 0, 200, 200 };

      ctk_widget_get_preferred_size (widget, &requisition, NULL);
      if (requisition.width || requisition.height)
	{
	  /* non-empty window */
	  allocation.width = requisition.width;
	  allocation.height = requisition.height;
	}
      ctk_widget_size_allocate (widget, &allocation);
      
      ctk_widget_queue_resize (widget);

      g_return_if_fail (!ctk_widget_get_realized (widget));
    }

  ctk_widget_set_realized (widget, TRUE);

  ctk_widget_get_allocation (widget, &allocation);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.title = (gchar *) ctk_window_get_title (window);
  _ctk_window_get_wmclass (window, &attributes.wmclass_name, &attributes.wmclass_class);
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;

  /* this isn't right - we should match our parent's visual/colormap.
   * though that will require handling "foreign" colormaps */
  attributes.visual = ctk_widget_get_visual (widget);
  attributes.event_mask = ctk_widget_get_events (widget);
  attributes.event_mask |= (GDK_EXPOSURE_MASK |
			    GDK_KEY_PRESS_MASK |
			    GDK_KEY_RELEASE_MASK |
			    GDK_ENTER_NOTIFY_MASK |
			    GDK_LEAVE_NOTIFY_MASK |
			    GDK_STRUCTURE_MASK |
			    GDK_FOCUS_CHANGE_MASK);

  attributes_mask = GDK_WA_VISUAL;
  attributes_mask |= (attributes.title ? GDK_WA_TITLE : 0);
  attributes_mask |= (attributes.wmclass_name ? GDK_WA_WMCLASS : 0);

  cdk_window = cdk_window_new (embed_widget->parent_window,
                               &attributes, attributes_mask);
  ctk_widget_set_window (widget, cdk_window);
  ctk_widget_register_window (widget, cdk_window);

  embed_widget->old_window_procedure = (gpointer)
    SetWindowLongPtrW(GDK_WINDOW_HWND (cdk_window),
		      GWLP_WNDPROC,
		      (LONG_PTR)ctk_win32_embed_widget_window_process);

  /* Enable tab to focus the widget */
  styles = GetWindowLongPtr(GDK_WINDOW_HWND (cdk_window), GWL_STYLE);
  SetWindowLongPtrW(GDK_WINDOW_HWND (cdk_window), GWL_STYLE, styles | WS_TABSTOP);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  ctk_style_context_set_background (ctk_widget_get_style_context (widget),
                                    cdk_window);
G_GNUC_END_IGNORE_DEPRECATIONS;
}

static void
ctk_win32_embed_widget_show (CtkWidget *widget)
{
  _ctk_widget_set_visible_flag (widget, TRUE);
  
  ctk_widget_realize (widget);
  ctk_container_check_resize (CTK_CONTAINER (widget));
  ctk_widget_map (widget);
}

static void
ctk_win32_embed_widget_hide (CtkWidget *widget)
{
  _ctk_widget_set_visible_flag (widget, FALSE);
  ctk_widget_unmap (widget);
}

static void
ctk_win32_embed_widget_map (CtkWidget *widget)
{
  CtkBin    *bin = CTK_BIN (widget);
  CtkWidget *child;

  ctk_widget_set_mapped (widget, TRUE);

  child = ctk_bin_get_child (bin);
  if (child &&
      ctk_widget_get_visible (child) &&
      !ctk_widget_get_mapped (child))
    ctk_widget_map (child);

  cdk_window_show (ctk_widget_get_window (widget));
}

static void
ctk_win32_embed_widget_unmap (CtkWidget *widget)
{
  ctk_widget_set_mapped (widget, FALSE);
  cdk_window_hide (ctk_widget_get_window (widget));
}

static void
ctk_win32_embed_widget_size_allocate (CtkWidget     *widget,
				      CtkAllocation *allocation)
{
  CtkBin    *bin = CTK_BIN (widget);
  CtkWidget *child;
  
  ctk_widget_set_allocation (widget, allocation);
  
  if (ctk_widget_get_realized (widget))
    cdk_window_move_resize (ctk_widget_get_window (widget),
			    allocation->x, allocation->y,
			    allocation->width, allocation->height);

  child = ctk_bin_get_child (bin);
  if (child && ctk_widget_get_visible (child))
    {
      CtkAllocation child_allocation;
      
      child_allocation.x = ctk_container_get_border_width (CTK_CONTAINER (widget));
      child_allocation.y = child_allocation.x;
      child_allocation.width =
	MAX (1, (gint)allocation->width - child_allocation.x * 2);
      child_allocation.height =
	MAX (1, (gint)allocation->height - child_allocation.y * 2);
      
      ctk_widget_size_allocate (child, &child_allocation);
    }
}

static void
ctk_win32_embed_widget_check_resize (CtkContainer *container)
{
  CTK_CONTAINER_CLASS (bin_class)->check_resize (container);
}

static gboolean
ctk_win32_embed_widget_focus (CtkWidget        *widget,
			      CtkDirectionType  direction)
{
  CtkBin *bin = CTK_BIN (widget);
  CtkWin32EmbedWidget *embed_widget = CTK_WIN32_EMBED_WIDGET (widget);
  CtkWindow *window = CTK_WINDOW (widget);
  CtkContainer *container = CTK_CONTAINER (widget);
  CtkWidget *old_focus_child = ctk_container_get_focus_child (container);
  CtkWidget *parent;
  CtkWidget *child;

  /* We override CtkWindow's behavior, since we don't want wrapping here.
   */
  if (old_focus_child)
    {
      if (ctk_widget_child_focus (old_focus_child, direction))
	return TRUE;

      if (ctk_window_get_focus (window))
	{
	  /* Wrapped off the end, clear the focus setting for the toplevel */
	  parent = ctk_widget_get_parent (ctk_window_get_focus (window));
	  while (parent)
	    {
	      ctk_container_set_focus_child (CTK_CONTAINER (parent), NULL);
	      parent = ctk_widget_get_parent (CTK_WIDGET (parent));
	    }
	  
	  ctk_window_set_focus (CTK_WINDOW (container), NULL);
	}
    }
  else
    {
      /* Try to focus the first widget in the window */
      child = ctk_bin_get_child (bin);
      if (child && ctk_widget_child_focus (child, direction))
        return TRUE;
    }

  if (!ctk_container_get_focus_child (CTK_CONTAINER (window)))
    {
      int backwards = FALSE;

      if (direction == CTK_DIR_TAB_BACKWARD ||
	  direction == CTK_DIR_LEFT)
	backwards = TRUE;
      
      PostMessage(GDK_WINDOW_HWND (embed_widget->parent_window),
				   WM_NEXTDLGCTL,
				   backwards, 0);
    }

  return FALSE;
}

static void
ctk_win32_embed_widget_set_focus (CtkWindow *window,
				  CtkWidget *focus)
{
  CTK_WINDOW_CLASS (ctk_win32_embed_widget_parent_class)->set_focus (window, focus);

  cdk_window_focus (ctk_widget_get_window (CTK_WIDGET(window)), 0);
}
