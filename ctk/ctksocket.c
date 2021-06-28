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

/* By Owen Taylor <otaylor@ctk.org>              98/4/4 */

/*
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/. 
 */

#include "config.h"

#include "ctksocketprivate.h"

#include <string.h>

#include "ctkmarshalers.h"
#include "ctksizerequest.h"
#include "ctkplug.h"
#include "ctkprivate.h"
#include "ctkrender.h"
#include "ctkdnd.h"
#include "ctkdragdest.h"
#include "ctkdebug.h"
#include "ctkintl.h"
#include "ctkmain.h"
#include "ctkwidgetprivate.h"

#include <cdk/cdkx.h>
#include <cdk/cdkprivate.h>

#ifdef HAVE_XFIXES
#include <X11/extensions/Xfixes.h>
#endif

#include "ctkxembed.h"

#include "a11y/ctksocketaccessible.h"


/**
 * SECTION:ctksocket
 * @Short_description: Container for widgets from other processes
 * @Title: CtkSocket
 * @include: ctk/ctkx.h
 * @See_also: #CtkPlug, [XEmbed Protocol](http://www.freedesktop.org/Standards/xembed-spec)
 *
 * Together with #CtkPlug, #CtkSocket provides the ability to embed
 * widgets from one process into another process in a fashion that
 * is transparent to the user. One process creates a #CtkSocket widget
 * and passes that widget’s window ID to the other process, which then
 * creates a #CtkPlug with that window ID. Any widgets contained in the
 * #CtkPlug then will appear inside the first application’s window.
 *
 * The socket’s window ID is obtained by using ctk_socket_get_id().
 * Before using this function, the socket must have been realized,
 * and for hence, have been added to its parent.
 *
 * ## Obtaining the window ID of a socket.
 *
 * |[<!-- language="C" -->
 * CtkWidget *socket = ctk_socket_new ();
 * ctk_widget_show (socket);
 * ctk_container_add (CTK_CONTAINER (parent), socket);
 *
 * // The following call is only necessary if one of
 * // the ancestors of the socket is not yet visible.
 * ctk_widget_realize (socket);
 * g_print ("The ID of the sockets window is %#x\n",
 *          ctk_socket_get_id (socket));
 * ]|
 *
 * Note that if you pass the window ID of the socket to another
 * process that will create a plug in the socket, you must make
 * sure that the socket widget is not destroyed until that plug
 * is created. Violating this rule will cause unpredictable
 * consequences, the most likely consequence being that the plug
 * will appear as a separate toplevel window. You can check if
 * the plug has been created by using ctk_socket_get_plug_window().
 * If it returns a non-%NULL value, then the plug has been
 * successfully created inside of the socket.
 *
 * When CTK+ is notified that the embedded window has been destroyed,
 * then it will destroy the socket as well. You should always,
 * therefore, be prepared for your sockets to be destroyed at any
 * time when the main event loop is running. To prevent this from
 * happening, you can connect to the #CtkSocket::plug-removed signal.
 *
 * The communication between a #CtkSocket and a #CtkPlug follows the
 * [XEmbed Protocol](http://www.freedesktop.org/Standards/xembed-spec).
 * This protocol has also been implemented in other toolkits, e.g. Qt,
 * allowing the same level of integration when embedding a Qt widget
 * in CTK or vice versa.
 *
 * The #CtkPlug and #CtkSocket widgets are only available when CTK+
 * is compiled for the X11 platform and %GDK_WINDOWING_X11 is defined.
 * They can only be used on a #GdkX11Display. To use #CtkPlug and
 * #CtkSocket, you need to include the `ctk/ctkx.h` header.
 */

/* Forward declararations */

static void     ctk_socket_finalize             (GObject          *object);
static void     ctk_socket_notify               (GObject          *object,
						 GParamSpec       *pspec);
static void     ctk_socket_realize              (CtkWidget        *widget);
static void     ctk_socket_unrealize            (CtkWidget        *widget);
static void     ctk_socket_get_preferred_width  (CtkWidget        *widget,
                                                 gint             *minimum,
                                                 gint             *natural);
static void     ctk_socket_get_preferred_height (CtkWidget        *widget,
                                                 gint             *minimum,
                                                 gint             *natural);
static void     ctk_socket_size_allocate        (CtkWidget        *widget,
						 CtkAllocation    *allocation);
static void     ctk_socket_hierarchy_changed    (CtkWidget        *widget,
						 CtkWidget        *old_toplevel);
static void     ctk_socket_grab_notify          (CtkWidget        *widget,
						 gboolean          was_grabbed);
static gboolean ctk_socket_key_event            (CtkWidget        *widget,
						 GdkEventKey      *event);
static gboolean ctk_socket_focus                (CtkWidget        *widget,
						 CtkDirectionType  direction);
static void     ctk_socket_remove               (CtkContainer     *container,
						 CtkWidget        *widget);
static void     ctk_socket_forall               (CtkContainer     *container,
						 gboolean          include_internals,
						 CtkCallback       callback,
						 gpointer          callback_data);
static void     ctk_socket_add_window           (CtkSocket        *socket,
                                                 Window            xid,
                                                 gboolean          need_reparent);
static GdkFilterReturn ctk_socket_filter_func   (GdkXEvent        *cdk_xevent,
                                                 GdkEvent         *event,
                                                 gpointer          data);

static gboolean xembed_get_info                 (GdkWindow        *cdk_window,
                                                 unsigned long    *version,
                                                 unsigned long    *flags);

static void     _ctk_socket_accessible_embed    (CtkWidget *socket,
                                                 GdkWindow *window);

/* From Tk */
#define EMBEDDED_APP_WANTS_FOCUS NotifyNormal+20


/* Local data */

typedef struct
{
  guint			 accel_key;
  GdkModifierType	 accel_mods;
} GrabbedKey;

enum {
  PLUG_ADDED,
  PLUG_REMOVED,
  LAST_SIGNAL
}; 

static guint socket_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (CtkSocket, ctk_socket, CTK_TYPE_CONTAINER)

static void
ctk_socket_finalize (GObject *object)
{
  CtkSocket *socket = CTK_SOCKET (object);
  CtkSocketPrivate *priv = socket->priv;

  g_object_unref (priv->accel_group);

  G_OBJECT_CLASS (ctk_socket_parent_class)->finalize (object);
}

static gboolean
ctk_socket_draw (CtkWidget *widget,
                 cairo_t   *cr)
{
  ctk_render_background (ctk_widget_get_style_context (widget), cr,
                         0, 0,
                         ctk_widget_get_allocated_width (widget),
                         ctk_widget_get_allocated_height (widget));

  return CTK_WIDGET_CLASS (ctk_socket_parent_class)->draw (widget, cr);
}

static void
ctk_socket_class_init (CtkSocketClass *class)
{
  CtkWidgetClass *widget_class;
  CtkContainerClass *container_class;
  GObjectClass *gobject_class;

  gobject_class = (GObjectClass *) class;
  widget_class = (CtkWidgetClass*) class;
  container_class = (CtkContainerClass*) class;

  gobject_class->finalize = ctk_socket_finalize;
  gobject_class->notify = ctk_socket_notify;

  widget_class->realize = ctk_socket_realize;
  widget_class->unrealize = ctk_socket_unrealize;
  widget_class->get_preferred_width = ctk_socket_get_preferred_width;
  widget_class->get_preferred_height = ctk_socket_get_preferred_height;
  widget_class->size_allocate = ctk_socket_size_allocate;
  widget_class->hierarchy_changed = ctk_socket_hierarchy_changed;
  widget_class->grab_notify = ctk_socket_grab_notify;
  widget_class->key_press_event = ctk_socket_key_event;
  widget_class->key_release_event = ctk_socket_key_event;
  widget_class->focus = ctk_socket_focus;
  widget_class->draw = ctk_socket_draw;

  /* We don't want to show_all the in-process plug, if any.
   */
  widget_class->show_all = ctk_widget_show;

  container_class->remove = ctk_socket_remove;
  container_class->forall = ctk_socket_forall;

  /**
   * CtkSocket::plug-added:
   * @socket_: the object which received the signal
   *
   * This signal is emitted when a client is successfully
   * added to the socket. 
   */
  socket_signals[PLUG_ADDED] =
    g_signal_new (I_("plug-added"),
		  G_OBJECT_CLASS_TYPE (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkSocketClass, plug_added),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * CtkSocket::plug-removed:
   * @socket_: the object which received the signal
   *
   * This signal is emitted when a client is removed from the socket. 
   * The default action is to destroy the #CtkSocket widget, so if you 
   * want to reuse it you must add a signal handler that returns %TRUE. 
   *
   * Returns: %TRUE to stop other handlers from being invoked.
   */
  socket_signals[PLUG_REMOVED] =
    g_signal_new (I_("plug-removed"),
		  G_OBJECT_CLASS_TYPE (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkSocketClass, plug_removed),
                  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__VOID,
		  G_TYPE_BOOLEAN, 0);


  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_SOCKET_ACCESSIBLE);
}

static void
ctk_socket_init (CtkSocket *socket)
{
  CtkSocketPrivate *priv;

  priv = ctk_socket_get_instance_private (socket);
  socket->priv = priv;

  priv->request_width = 0;
  priv->request_height = 0;
  priv->current_width = 0;
  priv->current_height = 0;

  priv->plug_window = NULL;
  priv->plug_widget = NULL;
  priv->focus_in = FALSE;
  priv->have_size = FALSE;
  priv->need_map = FALSE;
  priv->active = FALSE;

  priv->accel_group = ctk_accel_group_new ();
  g_object_set_data (G_OBJECT (priv->accel_group), I_("ctk-socket"), socket);
}

/**
 * ctk_socket_new:
 * 
 * Create a new empty #CtkSocket.
 * 
 * Returns:  the new #CtkSocket.
 **/
CtkWidget*
ctk_socket_new (void)
{
  CtkSocket *socket;

  socket = g_object_new (CTK_TYPE_SOCKET, NULL);

  return CTK_WIDGET (socket);
}

/**
 * ctk_socket_add_id:
 * @socket_: a #CtkSocket
 * @window: the Window of a client participating in the XEMBED protocol.
 *
 * Adds an XEMBED client, such as a #CtkPlug, to the #CtkSocket.  The
 * client may be in the same process or in a different process. 
 * 
 * To embed a #CtkPlug in a #CtkSocket, you can either create the
 * #CtkPlug with `ctk_plug_new (0)`, call 
 * ctk_plug_get_id() to get the window ID of the plug, and then pass that to the
 * ctk_socket_add_id(), or you can call ctk_socket_get_id() to get the
 * window ID for the socket, and call ctk_plug_new() passing in that
 * ID.
 *
 * The #CtkSocket must have already be added into a toplevel window
 *  before you can make this call.
 **/
void           
ctk_socket_add_id (CtkSocket      *socket,
		   Window          window)
{
  g_return_if_fail (CTK_IS_SOCKET (socket));
  g_return_if_fail (_ctk_widget_get_anchored (CTK_WIDGET (socket)));

  if (!ctk_widget_get_realized (CTK_WIDGET (socket)))
    ctk_widget_realize (CTK_WIDGET (socket));

  ctk_socket_add_window (socket, window, TRUE);
}

/**
 * ctk_socket_get_id:
 * @socket_: a #CtkSocket.
 * 
 * Gets the window ID of a #CtkSocket widget, which can then
 * be used to create a client embedded inside the socket, for
 * instance with ctk_plug_new(). 
 *
 * The #CtkSocket must have already be added into a toplevel window 
 * before you can make this call.
 * 
 * Returns: the window ID for the socket
 **/
Window
ctk_socket_get_id (CtkSocket *socket)
{
  g_return_val_if_fail (CTK_IS_SOCKET (socket), 0);
  g_return_val_if_fail (_ctk_widget_get_anchored (CTK_WIDGET (socket)), 0);

  if (!ctk_widget_get_realized (CTK_WIDGET (socket)))
    ctk_widget_realize (CTK_WIDGET (socket));

  return GDK_WINDOW_XID (ctk_widget_get_window (CTK_WIDGET (socket)));
}

/**
 * ctk_socket_get_plug_window:
 * @socket_: a #CtkSocket.
 *
 * Retrieves the window of the plug. Use this to check if the plug has
 * been created inside of the socket.
 *
 * Returns: (nullable) (transfer none): the window of the plug if
 * available, or %NULL
 *
 * Since:  2.14
 **/
GdkWindow*
ctk_socket_get_plug_window (CtkSocket *socket)
{
  g_return_val_if_fail (CTK_IS_SOCKET (socket), NULL);

  return socket->priv->plug_window;
}

static void
ctk_socket_realize (CtkWidget *widget)
{
  CtkAllocation allocation;
  GdkWindow *window;
  GdkWindowAttr attributes;
  XWindowAttributes xattrs;
  gint attributes_mask;
  GdkScreen *screen;

  ctk_widget_set_realized (widget, TRUE);

  screen = ctk_widget_get_screen (widget);
  if (!GDK_IS_X11_SCREEN (screen))
    g_warning ("CtkSocket: only works under X11");

  ctk_widget_get_allocation (widget, &allocation);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = ctk_widget_get_visual (widget);
  attributes.event_mask = GDK_FOCUS_CHANGE_MASK;

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;

  window = cdk_window_new (ctk_widget_get_parent_window (widget),
                           &attributes, attributes_mask);
  ctk_widget_set_window (widget, window);
  ctk_widget_register_window (widget, window);

  XGetWindowAttributes (GDK_WINDOW_XDISPLAY (window),
			GDK_WINDOW_XID (window),
			&xattrs);

  /* Sooooo, it turns out that mozilla, as per the ctk2xt code selects
     for input on the socket with a mask of 0x0fffff (for god knows why)
     which includes ButtonPressMask causing a BadAccess if someone else
     also selects for this. As per the client-side windows merge we always
     normally selects for button press so we can emulate it on client
     side children that selects for button press. However, we don't need
     this for CtkSocket, so we unselect it here, fixing the crashes in
     firefox. */
  XSelectInput (GDK_WINDOW_XDISPLAY (window),
		GDK_WINDOW_XID (window), 
		(xattrs.your_event_mask & ~ButtonPressMask) |
		SubstructureNotifyMask | SubstructureRedirectMask);

  cdk_window_add_filter (window,
			 ctk_socket_filter_func,
			 widget);

  /* We sync here so that we make sure that if the XID for
   * our window is passed to another application, SubstructureRedirectMask
   * will be set by the time the other app creates its window.
   */
  cdk_display_sync (ctk_widget_get_display (widget));
}

/**
 * ctk_socket_end_embedding:
 * @socket: a #CtkSocket
 *
 * Called to end the embedding of a plug in the socket.
 */
static void
ctk_socket_end_embedding (CtkSocket *socket)
{
  CtkSocketPrivate *private = socket->priv;

  g_object_unref (private->plug_window);
  private->plug_window = NULL;
  private->current_width = 0;
  private->current_height = 0;
  private->resize_count = 0;

  ctk_accel_group_disconnect (private->accel_group, NULL);
}

static void
ctk_socket_unrealize (CtkWidget *widget)
{
  CtkSocket *socket = CTK_SOCKET (widget);
  CtkSocketPrivate *private = socket->priv;

  ctk_widget_set_realized (widget, FALSE);

  if (private->plug_widget)
    {
      _ctk_plug_remove_from_socket (CTK_PLUG (private->plug_widget), socket);
    }
  else if (private->plug_window)
    {
      ctk_socket_end_embedding (socket);
    }

  CTK_WIDGET_CLASS (ctk_socket_parent_class)->unrealize (widget);
}

static void
ctk_socket_size_request (CtkSocket *socket)
{
  CtkSocketPrivate *private = socket->priv;
  GdkDisplay *display;
  XSizeHints hints;
  long supplied;
  int scale;

  display = ctk_widget_get_display (CTK_WIDGET (socket));
  cdk_x11_display_error_trap_push (display);

  private->request_width = 1;
  private->request_height = 1;
  scale = ctk_widget_get_scale_factor (CTK_WIDGET(socket));

  if (XGetWMNormalHints (GDK_WINDOW_XDISPLAY (private->plug_window),
			 GDK_WINDOW_XID (private->plug_window),
			 &hints, &supplied))
    {
      if (hints.flags & PMinSize)
	{
	  private->request_width = MAX (hints.min_width / scale, 1);
	  private->request_height = MAX (hints.min_height / scale, 1);
	}
      else if (hints.flags & PBaseSize)
	{
	  private->request_width = MAX (hints.base_width / scale, 1);
	  private->request_height = MAX (hints.base_height / scale, 1);
	}
    }
  private->have_size = TRUE;
  
  cdk_x11_display_error_trap_pop_ignored (display);
}

static void
ctk_socket_get_preferred_width (CtkWidget *widget,
                                gint      *minimum,
                                gint      *natural)
{
  CtkSocket *socket = CTK_SOCKET (widget);
  CtkSocketPrivate *private = socket->priv;

  if (private->plug_widget)
    {
      ctk_widget_get_preferred_width (private->plug_widget, minimum, natural);
    }
  else
    {
      if (private->is_mapped && !private->have_size && private->plug_window)
        ctk_socket_size_request (socket);

      if (private->is_mapped && private->have_size)
        *minimum = *natural = MAX (private->request_width, 1);
      else
        *minimum = *natural = 1;
    }
}

static void
ctk_socket_get_preferred_height (CtkWidget *widget,
                                 gint      *minimum,
                                 gint      *natural)
{
  CtkSocket *socket = CTK_SOCKET (widget);
  CtkSocketPrivate *private = socket->priv;

  if (private->plug_widget)
    {
      ctk_widget_get_preferred_height (private->plug_widget, minimum, natural);
    }
  else
    {
      if (private->is_mapped && !private->have_size && private->plug_window)
        ctk_socket_size_request (socket);

      if (private->is_mapped && private->have_size)
        *minimum = *natural = MAX (private->request_height, 1);
      else
        *minimum = *natural = 1;
    }
}

static void
ctk_socket_send_configure_event (CtkSocket *socket)
{
  CtkAllocation allocation;
  XConfigureEvent xconfigure;
  GdkDisplay *display;
  int x, y, scale;

  g_return_if_fail (socket->priv->plug_window != NULL);

  memset (&xconfigure, 0, sizeof (xconfigure));
  xconfigure.type = ConfigureNotify;

  xconfigure.event = GDK_WINDOW_XID (socket->priv->plug_window);
  xconfigure.window = GDK_WINDOW_XID (socket->priv->plug_window);

  /* The ICCCM says that synthetic events should have root relative
   * coordinates. We still aren't really ICCCM compliant, since
   * we don't send events when the real toplevel is moved.
   */
  display = cdk_window_get_display (socket->priv->plug_window);
  cdk_x11_display_error_trap_push (display);
  cdk_window_get_origin (socket->priv->plug_window, &x, &y);
  cdk_x11_display_error_trap_pop_ignored (display);

  ctk_widget_get_allocation (CTK_WIDGET(socket), &allocation);
  scale = ctk_widget_get_scale_factor (CTK_WIDGET(socket));
  xconfigure.x = x * scale;
  xconfigure.y = y * scale;
  xconfigure.width = allocation.width * scale;
  xconfigure.height = allocation.height * scale;

  xconfigure.border_width = 0;
  xconfigure.above = None;
  xconfigure.override_redirect = False;

  cdk_x11_display_error_trap_push (display);
  XSendEvent (GDK_WINDOW_XDISPLAY (socket->priv->plug_window),
	      GDK_WINDOW_XID (socket->priv->plug_window),
	      False, NoEventMask, (XEvent *)&xconfigure);
  cdk_x11_display_error_trap_pop_ignored (display);
}

static void
ctk_socket_size_allocate (CtkWidget     *widget,
			  CtkAllocation *allocation)
{
  CtkSocket *socket = CTK_SOCKET (widget);
  CtkSocketPrivate *private = socket->priv;

  ctk_widget_set_allocation (widget, allocation);
  if (ctk_widget_get_realized (widget))
    {
      cdk_window_move_resize (ctk_widget_get_window (widget),
			      allocation->x, allocation->y,
			      allocation->width, allocation->height);

      if (private->plug_widget)
	{
	  CtkAllocation child_allocation;

	  child_allocation.x = 0;
	  child_allocation.y = 0;
	  child_allocation.width = allocation->width;
	  child_allocation.height = allocation->height;

	  ctk_widget_size_allocate (private->plug_widget, &child_allocation);
	}
      else if (private->plug_window)
	{
          GdkDisplay *display = cdk_window_get_display (private->plug_window);

	  cdk_x11_display_error_trap_push (display);

	  if (allocation->width != private->current_width ||
	      allocation->height != private->current_height)
	    {
	      cdk_window_move_resize (private->plug_window,
				      0, 0,
				      allocation->width, allocation->height);
	      if (private->resize_count)
		private->resize_count--;
	      
	      CTK_NOTE (PLUGSOCKET,
			g_message ("CtkSocket - allocated: %d %d",
				   allocation->width, allocation->height));
	      private->current_width = allocation->width;
	      private->current_height = allocation->height;
	    }

	  if (private->need_map)
	    {
	      cdk_window_show (private->plug_window);
	      private->need_map = FALSE;
	    }

	  while (private->resize_count)
 	    {
 	      ctk_socket_send_configure_event (socket);
 	      private->resize_count--;
 	      CTK_NOTE (PLUGSOCKET,
			g_message ("CtkSocket - sending synthetic configure: %d %d",
				   allocation->width, allocation->height));
 	    }

	  cdk_x11_display_error_trap_pop_ignored (display);
	}
    }
}

static void
ctk_socket_send_key_event (CtkSocket *socket,
			   GdkEvent  *cdk_event,
			   gboolean   mask_key_presses)
{
  XKeyEvent xkey;
  GdkScreen *screen = cdk_window_get_screen (socket->priv->plug_window);

  memset (&xkey, 0, sizeof (xkey));
  xkey.type = (cdk_event->type == GDK_KEY_PRESS) ? KeyPress : KeyRelease;
  xkey.window = GDK_WINDOW_XID (socket->priv->plug_window);
  xkey.root = GDK_WINDOW_XID (cdk_screen_get_root_window (screen));
  xkey.subwindow = None;
  xkey.time = cdk_event->key.time;
  xkey.x = 0;
  xkey.y = 0;
  xkey.x_root = 0;
  xkey.y_root = 0;
  xkey.state = cdk_event->key.state;
  xkey.keycode = cdk_event->key.hardware_keycode;
  xkey.same_screen = True;/* FIXME ? */

  cdk_x11_display_error_trap_push (cdk_window_get_display (socket->priv->plug_window));
  XSendEvent (GDK_WINDOW_XDISPLAY (socket->priv->plug_window),
	      GDK_WINDOW_XID (socket->priv->plug_window),
	      False,
	      (mask_key_presses ? KeyPressMask : NoEventMask),
	      (XEvent *)&xkey);
  cdk_x11_display_error_trap_pop_ignored (cdk_window_get_display (socket->priv->plug_window));
}

static gboolean
activate_key (CtkAccelGroup  *accel_group,
	      GObject        *acceleratable,
	      guint           accel_key,
	      GdkModifierType accel_mods,
	      GrabbedKey     *grabbed_key)
{
  GdkEvent *cdk_event = ctk_get_current_event ();
  
  CtkSocket *socket = g_object_get_data (G_OBJECT (accel_group), "ctk-socket");
  gboolean retval = FALSE;

  if (cdk_event && cdk_event->type == GDK_KEY_PRESS && socket->priv->plug_window)
    {
      ctk_socket_send_key_event (socket, cdk_event, FALSE);
      retval = TRUE;
    }

  if (cdk_event)
    cdk_event_free (cdk_event);

  return retval;
}

static gboolean
find_accel_key (CtkAccelKey *key,
		GClosure    *closure,
		gpointer     data)
{
  GrabbedKey *grabbed_key = data;
  
  return (key->accel_key == grabbed_key->accel_key &&
	  key->accel_mods == grabbed_key->accel_mods);
}

/**
 * ctk_socket_add_grabbed_key:
 * @socket: a #CtkSocket
 * @keyval: a key
 * @modifiers: modifiers for the key
 *
 * Called from the CtkSocket platform-specific backend when the
 * corresponding plug has told the socket to grab a key.
 */
static void
ctk_socket_add_grabbed_key (CtkSocket       *socket,
			    guint            keyval,
			    GdkModifierType  modifiers)
{
  GClosure *closure;
  GrabbedKey *grabbed_key;

  grabbed_key = g_new (GrabbedKey, 1);
  
  grabbed_key->accel_key = keyval;
  grabbed_key->accel_mods = modifiers;

  if (ctk_accel_group_find (socket->priv->accel_group,
			    find_accel_key,
			    &grabbed_key))
    {
      g_warning ("CtkSocket: request to add already present grabbed key %u,%#x",
		 keyval, modifiers);
      g_free (grabbed_key);
      return;
    }

  closure = g_cclosure_new (G_CALLBACK (activate_key), grabbed_key, (GClosureNotify)g_free);

  ctk_accel_group_connect (socket->priv->accel_group, keyval, modifiers, CTK_ACCEL_LOCKED,
			   closure);
}

/**
 * ctk_socket_remove_grabbed_key:
 * @socket: a #CtkSocket
 * @keyval: a key
 * @modifiers: modifiers for the key
 *
 * Called from the CtkSocket backend when the corresponding plug has
 * told the socket to remove a key grab.
 */
static void
ctk_socket_remove_grabbed_key (CtkSocket      *socket,
			       guint           keyval,
			       GdkModifierType modifiers)
{
  if (!ctk_accel_group_disconnect_key (socket->priv->accel_group, keyval, modifiers))
    g_warning ("CtkSocket: request to remove non-present grabbed key %u,%#x",
	       keyval, modifiers);
}

static void
socket_update_focus_in (CtkSocket *socket)
{
  CtkSocketPrivate *private = socket->priv;
  gboolean focus_in = FALSE;

  if (private->plug_window)
    {
      CtkWidget *toplevel = ctk_widget_get_toplevel (CTK_WIDGET (socket));

      if (ctk_widget_is_toplevel (toplevel) &&
	  ctk_window_has_toplevel_focus (CTK_WINDOW (toplevel)) &&
	  ctk_widget_is_focus (CTK_WIDGET (socket)))
	focus_in = TRUE;
    }

  if (focus_in != private->focus_in)
    {
      private->focus_in = focus_in;

      if (focus_in)
        _ctk_xembed_send_focus_message (private->plug_window,
                                        XEMBED_FOCUS_IN, XEMBED_FOCUS_CURRENT);
      else
        _ctk_xembed_send_message (private->plug_window,
                                  XEMBED_FOCUS_OUT, 0, 0, 0);
    }
}

static void
socket_update_active (CtkSocket *socket)
{
  CtkSocketPrivate *private = socket->priv;
  gboolean active = FALSE;

  if (private->plug_window)
    {
      CtkWidget *toplevel = ctk_widget_get_toplevel (CTK_WIDGET (socket));

      if (ctk_widget_is_toplevel (toplevel) &&
	  ctk_window_is_active  (CTK_WINDOW (toplevel)))
	active = TRUE;
    }

  if (active != private->active)
    {
      private->active = active;

      _ctk_xembed_send_message (private->plug_window,
                                active ? XEMBED_WINDOW_ACTIVATE : XEMBED_WINDOW_DEACTIVATE,
                                0, 0, 0);
    }
}

static void
ctk_socket_hierarchy_changed (CtkWidget *widget,
			      CtkWidget *old_toplevel)
{
  CtkSocket *socket = CTK_SOCKET (widget);
  CtkSocketPrivate *private = socket->priv;
  CtkWidget *toplevel = ctk_widget_get_toplevel (widget);

  if (toplevel && !CTK_IS_WINDOW (toplevel))
    toplevel = NULL;

  if (toplevel != private->toplevel)
    {
      if (private->toplevel)
	{
	  ctk_window_remove_accel_group (CTK_WINDOW (private->toplevel), private->accel_group);
	  g_signal_handlers_disconnect_by_func (private->toplevel,
						socket_update_focus_in,
						socket);
	  g_signal_handlers_disconnect_by_func (private->toplevel,
						socket_update_active,
						socket);
	}

      private->toplevel = toplevel;

      if (toplevel)
	{
	  ctk_window_add_accel_group (CTK_WINDOW (private->toplevel), private->accel_group);
	  g_signal_connect_swapped (private->toplevel, "notify::has-toplevel-focus",
				    G_CALLBACK (socket_update_focus_in), socket);
	  g_signal_connect_swapped (private->toplevel, "notify::is-active",
				    G_CALLBACK (socket_update_active), socket);
	}

      socket_update_focus_in (socket);
      socket_update_active (socket);
    }
}

static void
ctk_socket_grab_notify (CtkWidget *widget,
			gboolean   was_grabbed)
{
  CtkSocket *socket = CTK_SOCKET (widget);

  if (!socket->priv->same_app)
    _ctk_xembed_send_message (socket->priv->plug_window,
                              was_grabbed ? XEMBED_MODALITY_OFF : XEMBED_MODALITY_ON,
                              0, 0, 0);
}

static gboolean
ctk_socket_key_event (CtkWidget   *widget,
                      GdkEventKey *event)
{
  CtkSocket *socket = CTK_SOCKET (widget);
  CtkSocketPrivate *private = socket->priv;
  
  if (ctk_widget_has_focus (widget) && private->plug_window && !private->plug_widget)
    {
      ctk_socket_send_key_event (socket, (GdkEvent *) event, FALSE);

      return TRUE;
    }
  else
    return FALSE;
}

static void
ctk_socket_notify (GObject    *object,
		   GParamSpec *pspec)
{
  if (strcmp (pspec->name, "is-focus") == 0)
    socket_update_focus_in (CTK_SOCKET (object));

  if (G_OBJECT_CLASS (ctk_socket_parent_class)->notify)
    G_OBJECT_CLASS (ctk_socket_parent_class)->notify (object, pspec);
}

/**
 * ctk_socket_claim_focus:
 * @socket: a #CtkSocket
 * @send_event: huh?
 *
 * Claims focus for the socket. XXX send_event?
 */
static void
ctk_socket_claim_focus (CtkSocket *socket,
			gboolean   send_event)
{
  CtkWidget *widget = CTK_WIDGET (socket);
  CtkSocketPrivate *private = socket->priv;

  if (!send_event)
    private->focus_in = TRUE;	/* Otherwise, our notify handler will send FOCUS_IN  */
      
  /* Oh, the trickery... */
  
  ctk_widget_set_can_focus (widget, TRUE);
  ctk_widget_grab_focus (widget);
  ctk_widget_set_can_focus (widget, FALSE);
}

static gboolean
ctk_socket_focus (CtkWidget       *widget,
		  CtkDirectionType direction)
{
  CtkSocket *socket = CTK_SOCKET (widget);
  CtkSocketPrivate *private = socket->priv;

  if (private->plug_widget)
    return ctk_widget_child_focus (private->plug_widget, direction);

  if (!ctk_widget_is_focus (widget))
    {
      gint detail = -1;

      switch (direction)
        {
        case CTK_DIR_UP:
        case CTK_DIR_LEFT:
        case CTK_DIR_TAB_BACKWARD:
          detail = XEMBED_FOCUS_LAST;
          break;
        case CTK_DIR_DOWN:
        case CTK_DIR_RIGHT:
        case CTK_DIR_TAB_FORWARD:
          detail = XEMBED_FOCUS_FIRST;
          break;
        }
      
      _ctk_xembed_send_focus_message (private->plug_window, XEMBED_FOCUS_IN, detail);
      ctk_socket_claim_focus (socket, FALSE);
 
      return TRUE;
    }
  else
    return FALSE;
}

static void
ctk_socket_remove (CtkContainer *container,
		   CtkWidget    *child)
{
  CtkSocket *socket = CTK_SOCKET (container);
  CtkSocketPrivate *private = socket->priv;

  g_return_if_fail (child == private->plug_widget);

  _ctk_plug_remove_from_socket (CTK_PLUG (private->plug_widget), socket);
}

static void
ctk_socket_forall (CtkContainer *container,
		   gboolean      include_internals,
		   CtkCallback   callback,
		   gpointer      callback_data)
{
  CtkSocket *socket = CTK_SOCKET (container);
  CtkSocketPrivate *private = socket->priv;

  if (private->plug_widget)
    (* callback) (private->plug_widget, callback_data);
}

/**
 * ctk_socket_add_window:
 * @socket: a #CtkSocket
 * @xid: the native identifier for a window
 * @need_reparent: whether the socket’s plug’s window needs to be
 *                 reparented to the socket
 *
 * Adds a window to a CtkSocket.
 */
static void
ctk_socket_add_window (CtkSocket       *socket,
		       Window           xid,
		       gboolean         need_reparent)
{
  CtkWidget *widget = CTK_WIDGET (socket);
  GdkDisplay *display = ctk_widget_get_display (widget);
  gpointer user_data = NULL;
  CtkSocketPrivate *private = socket->priv;
  unsigned long version;
  unsigned long flags;

  if (GDK_IS_X11_DISPLAY (display))
    private->plug_window = cdk_x11_window_lookup_for_display (display, xid);
  else
    private->plug_window = NULL;

  if (private->plug_window)
    {
      g_object_ref (private->plug_window);
      cdk_window_get_user_data (private->plug_window, &user_data);
    }

  if (user_data) /* A widget's window in this process */
    {
      CtkWidget *child_widget = user_data;

      if (!CTK_IS_PLUG (child_widget))
        {
          g_warning (G_STRLOC ": Can't add non-CtkPlug to CtkSocket");
          private->plug_window = NULL;
          cdk_x11_display_error_trap_pop_ignored (display);

          return;
        }

      _ctk_plug_add_to_socket (CTK_PLUG (child_widget), socket);
    }
  else  /* A foreign window */
    {
      GdkDragProtocol protocol;

      cdk_x11_display_error_trap_push (display);

      if (!private->plug_window)
        {
          if (GDK_IS_X11_DISPLAY (display))
            private->plug_window = cdk_x11_window_foreign_new_for_display (display, xid);
          if (!private->plug_window) /* was deleted before we could get it */
            {
              cdk_x11_display_error_trap_pop_ignored (display);
              return;
            }
        }

      XSelectInput (GDK_DISPLAY_XDISPLAY (display),
                    GDK_WINDOW_XID (private->plug_window),
                    StructureNotifyMask | PropertyChangeMask);

      if (cdk_x11_display_error_trap_pop (display))
	{
	  g_object_unref (private->plug_window);
	  private->plug_window = NULL;
	  return;
	}
      
      /* OK, we now will reliably get destroy notification on socket->plug_window */

      cdk_x11_display_error_trap_push (display);

      if (need_reparent)
	{
	  cdk_window_hide (private->plug_window); /* Shouldn't actually be necessary for XEMBED, but just in case */
	  cdk_window_reparent (private->plug_window,
                               ctk_widget_get_window (widget),
                               0, 0);
	}

      private->have_size = FALSE;

      private->xembed_version = -1;
      if (xembed_get_info (private->plug_window, &version, &flags))
        {
          private->xembed_version = MIN (CTK_XEMBED_PROTOCOL_VERSION, version);
          private->is_mapped = (flags & XEMBED_MAPPED) != 0;
        }
      else
        {
          /* FIXME, we should probably actually check the state before we started */
          private->is_mapped = TRUE;
        }

      private->need_map = private->is_mapped;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      protocol = cdk_window_get_drag_protocol (private->plug_window, NULL);
      if (protocol)
	ctk_drag_dest_set_proxy (widget, private->plug_window, protocol, TRUE);
G_GNUC_END_IGNORE_DEPRECATIONS

      cdk_x11_display_error_trap_pop_ignored (display);

      cdk_window_add_filter (private->plug_window,
			     ctk_socket_filter_func,
			     socket);

#ifdef HAVE_XFIXES
      cdk_x11_display_error_trap_push (display);
      XFixesChangeSaveSet (GDK_DISPLAY_XDISPLAY (display),
                           GDK_WINDOW_XID (private->plug_window),
                           SetModeInsert, SaveSetRoot, SaveSetUnmap);
      cdk_x11_display_error_trap_pop_ignored (display);
#endif
      _ctk_xembed_send_message (private->plug_window,
                                XEMBED_EMBEDDED_NOTIFY, 0,
                                GDK_WINDOW_XID (ctk_widget_get_window (widget)),
                                private->xembed_version);

      socket_update_active (socket);
      socket_update_focus_in (socket);

      ctk_widget_queue_resize (CTK_WIDGET (socket));

      _ctk_socket_accessible_embed (CTK_WIDGET (socket), private->plug_window);
    }

  if (private->plug_window)
    g_signal_emit (socket, socket_signals[PLUG_ADDED], 0);
}

/**
 * ctk_socket_handle_map_request:
 * @socket: a #CtkSocket
 *
 * Called from the CtkSocket backend when the plug has been mapped.
 */
static void
ctk_socket_handle_map_request (CtkSocket *socket)
{
  CtkSocketPrivate *private = socket->priv;
  if (!private->is_mapped)
    {
      private->is_mapped = TRUE;
      private->need_map = TRUE;

      ctk_widget_queue_resize (CTK_WIDGET (socket));
    }
}

/**
 * ctk_socket_unmap_notify:
 * @socket: a #CtkSocket
 *
 * Called from the CtkSocket backend when the plug has been unmapped ???
 */
static void
ctk_socket_unmap_notify (CtkSocket *socket)
{
  CtkSocketPrivate *private = socket->priv;
  if (private->is_mapped)
    {
      private->is_mapped = FALSE;
      ctk_widget_queue_resize (CTK_WIDGET (socket));
    }
}

/**
 * ctk_socket_advance_toplevel_focus:
 * @socket: a #CtkSocket
 * @direction: a direction
 *
 * Called from the CtkSocket backend when the corresponding plug
 * has told the socket to move the focus.
 */
static void
ctk_socket_advance_toplevel_focus (CtkSocket        *socket,
				   CtkDirectionType  direction)
{
  CtkBin *bin;
  CtkWindow *window;
  CtkContainer *container;
  CtkWidget *child;
  CtkWidget *focus_widget;
  CtkWidget *toplevel;
  CtkWidget *old_focus_child;
  CtkWidget *parent;

  toplevel = ctk_widget_get_toplevel (CTK_WIDGET (socket));
  if (!toplevel)
    return;

  if (!ctk_widget_is_toplevel (toplevel) || CTK_IS_PLUG (toplevel))
    {
      ctk_widget_child_focus (toplevel,direction);
      return;
    }

  container = CTK_CONTAINER (toplevel);
  window = CTK_WINDOW (toplevel);
  bin = CTK_BIN (toplevel);

  /* This is a copy of ctk_window_focus(), modified so that we
   * can detect wrap-around.
   */
  old_focus_child = ctk_container_get_focus_child (container);
  
  if (old_focus_child)
    {
      if (ctk_widget_child_focus (old_focus_child, direction))
	return;

      /* We are allowed exactly one wrap-around per sequence of focus
       * events
       */
      if (_ctk_xembed_get_focus_wrapped ())
	return;
      else
	_ctk_xembed_set_focus_wrapped ();
    }

  focus_widget = ctk_window_get_focus (window);
  if (window)
    {
      /* Wrapped off the end, clear the focus setting for the toplevel */
      parent = ctk_widget_get_parent (focus_widget);
      while (parent)
	{
	  ctk_container_set_focus_child (CTK_CONTAINER (parent), NULL);
          parent = ctk_widget_get_parent (parent);
	}
      
      ctk_window_set_focus (CTK_WINDOW (container), NULL);
    }

  /* Now try to focus the first widget in the window */
  child = ctk_bin_get_child (bin);
  if (child)
    {
      if (ctk_widget_child_focus (child, direction))
        return;
    }
}

static gboolean
xembed_get_info (GdkWindow     *window,
		 unsigned long *version,
		 unsigned long *flags)
{
  GdkDisplay *display = cdk_window_get_display (window);
  Atom xembed_info_atom = cdk_x11_get_xatom_by_name_for_display (display, "_XEMBED_INFO");
  Atom type;
  int format;
  unsigned long nitems, bytes_after;
  unsigned char *data;
  unsigned long *data_long;
  int status;
  
  cdk_x11_display_error_trap_push (display);
  status = XGetWindowProperty (GDK_DISPLAY_XDISPLAY (display),
			       GDK_WINDOW_XID (window),
			       xembed_info_atom,
			       0, 2, False,
			       xembed_info_atom, &type, &format,
			       &nitems, &bytes_after, &data);
  cdk_x11_display_error_trap_pop_ignored (display);

  if (status != Success)
    return FALSE;		/* Window vanished? */

  if (type == None)		/* No info property */
    return FALSE;

  if (type != xembed_info_atom)
    {
      g_warning ("_XEMBED_INFO property has wrong type");
      return FALSE;
    }
  
  if (nitems < 2)
    {
      g_warning ("_XEMBED_INFO too short");
      XFree (data);
      return FALSE;
    }
  
  data_long = (unsigned long *)data;
  if (version)
    *version = data_long[0];
  if (flags)
    *flags = data_long[1] & XEMBED_MAPPED;
  
  XFree (data);
  return TRUE;
}

static void
handle_xembed_message (CtkSocket        *socket,
		       XEmbedMessageType message,
		       glong             detail,
		       glong             data1,
		       glong             data2,
		       guint32           time)
{
  CTK_NOTE (PLUGSOCKET,
	    g_message ("CtkSocket: %s received", _ctk_xembed_message_name (message)));
  
  switch (message)
    {
    case XEMBED_EMBEDDED_NOTIFY:
    case XEMBED_WINDOW_ACTIVATE:
    case XEMBED_WINDOW_DEACTIVATE:
    case XEMBED_MODALITY_ON:
    case XEMBED_MODALITY_OFF:
    case XEMBED_FOCUS_IN:
    case XEMBED_FOCUS_OUT:
      g_warning ("CtkSocket: Invalid _XEMBED message %s received", _ctk_xembed_message_name (message));
      break;
      
    case XEMBED_REQUEST_FOCUS:
      ctk_socket_claim_focus (socket, TRUE);
      break;

    case XEMBED_FOCUS_NEXT:
    case XEMBED_FOCUS_PREV:
      ctk_socket_advance_toplevel_focus (socket,
					 (message == XEMBED_FOCUS_NEXT ?
					  CTK_DIR_TAB_FORWARD : CTK_DIR_TAB_BACKWARD));
      break;
      
    case XEMBED_CTK_GRAB_KEY:
      ctk_socket_add_grabbed_key (socket, data1, data2);
      break; 
    case XEMBED_CTK_UNGRAB_KEY:
      ctk_socket_remove_grabbed_key (socket, data1, data2);
      break;

    case XEMBED_GRAB_KEY:
    case XEMBED_UNGRAB_KEY:
      break;
      
    default:
      CTK_NOTE (PLUGSOCKET,
		g_message ("CtkSocket: Ignoring unknown _XEMBED message of type %d", message));
      break;
    }
}

static void
_ctk_socket_accessible_embed (CtkWidget *socket, GdkWindow *window)
{
  GdkDisplay *display = cdk_window_get_display (window);
  Atom net_at_spi_path_atom = cdk_x11_get_xatom_by_name_for_display (display, "_XEMBED_AT_SPI_PATH");
  Atom type;
  int format;
  unsigned long nitems, bytes_after;
  unsigned char *data;
  int status;

  cdk_x11_display_error_trap_push (display);
  status = XGetWindowProperty (GDK_DISPLAY_XDISPLAY (display),
			       GDK_WINDOW_XID (window),
			       net_at_spi_path_atom,
			       0, INT_MAX / 4, False,
			       net_at_spi_path_atom, &type, &format,
			       &nitems, &bytes_after, &data);
  cdk_x11_display_error_trap_pop_ignored (display);

  if (status != Success)
    return;			/* Window vanished? */

  if (type == None)		/* No info property */
    return;

  if (type != net_at_spi_path_atom)
    {
      g_warning ("_XEMBED_AT_SPI_PATH property has wrong type");
      return;
    }

  if (nitems == 0)
    {
      g_warning ("_XEMBED_AT_SPI_PATH too short");
      XFree (data);
      return;
    }

  if (nitems > INT_MAX)
    {
      g_warning ("_XEMBED_AT_SPI_PATH too long");
      XFree (data);
      return;
    }

  ctk_socket_accessible_embed (CTK_SOCKET_ACCESSIBLE (ctk_widget_get_accessible (socket)), (gchar*) data);
  XFree (data);

  return;
}

static GdkFilterReturn
ctk_socket_filter_func (GdkXEvent *cdk_xevent,
			GdkEvent  *event,
			gpointer   data)
{
  CtkSocket *socket;
  CtkWidget *widget;
  GdkDisplay *display;
  XEvent *xevent;
  CtkSocketPrivate *private;

  GdkFilterReturn return_val;

  socket = CTK_SOCKET (data);
  private = socket->priv;

  return_val = GDK_FILTER_CONTINUE;

  if (private->plug_widget)
    return return_val;

  widget = CTK_WIDGET (socket);
  xevent = (XEvent *)cdk_xevent;
  display = ctk_widget_get_display (widget);

  switch (xevent->type)
    {
    case ClientMessage:
      if (xevent->xclient.message_type == cdk_x11_get_xatom_by_name_for_display (display, "_XEMBED"))
	{
	  _ctk_xembed_push_message (xevent);
	  handle_xembed_message (socket,
				 xevent->xclient.data.l[1],
				 xevent->xclient.data.l[2],
				 xevent->xclient.data.l[3],
				 xevent->xclient.data.l[4],
				 xevent->xclient.data.l[0]);
	  _ctk_xembed_pop_message ();
	  
	  return_val = GDK_FILTER_REMOVE;
	}
      break;

    case CreateNotify:
      {
	XCreateWindowEvent *xcwe = &xevent->xcreatewindow;

	if (!private->plug_window)
	  {
	    ctk_socket_add_window (socket, xcwe->window, FALSE);

	    if (private->plug_window)
	      {
		CTK_NOTE (PLUGSOCKET, g_message ("CtkSocket - window created"));
	      }
	  }
	
	return_val = GDK_FILTER_REMOVE;
	
	break;
      }

    case ConfigureRequest:
      {
	XConfigureRequestEvent *xcre = &xevent->xconfigurerequest;
	
	if (!private->plug_window)
	  ctk_socket_add_window (socket, xcre->window, FALSE);
	
	if (private->plug_window)
	  {
	    if (xcre->value_mask & (CWWidth | CWHeight))
	      {
		CTK_NOTE (PLUGSOCKET,
			  g_message ("CtkSocket - configure request: %d %d",
				     private->request_width,
				     private->request_height));

		private->resize_count++;
		ctk_widget_queue_resize (widget);
	      }
	    else if (xcre->value_mask & (CWX | CWY))
	      {
		ctk_socket_send_configure_event (socket);
	      }
	    /* Ignore stacking requests. */
	    
	    return_val = GDK_FILTER_REMOVE;
	  }
	break;
      }

    case DestroyNotify:
      {
	XDestroyWindowEvent *xdwe = &xevent->xdestroywindow;

	/* Note that we get destroy notifies both from SubstructureNotify on
	 * our window and StructureNotify on socket->plug_window
	 */
	if (private->plug_window && (xdwe->window == GDK_WINDOW_XID (private->plug_window)))
	  {
	    gboolean result;
	    
	    CTK_NOTE (PLUGSOCKET, g_message ("CtkSocket - destroy notify"));
	    
	    cdk_window_destroy_notify (private->plug_window);
	    ctk_socket_end_embedding (socket);

	    g_object_ref (widget);
	    g_signal_emit_by_name (widget, "plug-removed", &result);
	    if (!result)
	      ctk_widget_destroy (widget);
	    g_object_unref (widget);
	    
	    return_val = GDK_FILTER_REMOVE;
	  }
	break;
      }

    case FocusIn:
      if (xevent->xfocus.mode == EMBEDDED_APP_WANTS_FOCUS)
	{
	  ctk_socket_claim_focus (socket, TRUE);
	}
      return_val = GDK_FILTER_REMOVE;
      break;
    case FocusOut:
      return_val = GDK_FILTER_REMOVE;
      break;
    case MapRequest:
      if (!private->plug_window)
	{
	  ctk_socket_add_window (socket, xevent->xmaprequest.window, FALSE);
	}
	
      if (private->plug_window)
	{
	  CTK_NOTE (PLUGSOCKET, g_message ("CtkSocket - Map Request"));

	  ctk_socket_handle_map_request (socket);
	  return_val = GDK_FILTER_REMOVE;
	}
      break;
    case PropertyNotify:
      if (private->plug_window &&
	  xevent->xproperty.window == GDK_WINDOW_XID (private->plug_window))
	{
	  GdkDragProtocol protocol;

	  if (xevent->xproperty.atom == cdk_x11_get_xatom_by_name_for_display (display, "WM_NORMAL_HINTS"))
	    {
	      CTK_NOTE (PLUGSOCKET, g_message ("CtkSocket - received PropertyNotify for plug's WM_NORMAL_HINTS"));
	      private->have_size = FALSE;
	      ctk_widget_queue_resize (widget);
	      return_val = GDK_FILTER_REMOVE;
	    }
	  else if ((xevent->xproperty.atom == cdk_x11_get_xatom_by_name_for_display (display, "XdndAware")) ||
	      (xevent->xproperty.atom == cdk_x11_get_xatom_by_name_for_display (display, "_MOTIF_DRAG_RECEIVER_INFO")))
	    {
	      cdk_x11_display_error_trap_push (display);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
              protocol = cdk_window_get_drag_protocol (private->plug_window, NULL);
              if (protocol)
		ctk_drag_dest_set_proxy (CTK_WIDGET (socket),
					 private->plug_window,
					 protocol, TRUE);
G_GNUC_END_IGNORE_DEPRECATIONS

	      cdk_x11_display_error_trap_pop_ignored (display);
	      return_val = GDK_FILTER_REMOVE;
	    }
	  else if (xevent->xproperty.atom == cdk_x11_get_xatom_by_name_for_display (display, "_XEMBED_INFO"))
	    {
	      unsigned long flags;
	      
	      if (xembed_get_info (private->plug_window, NULL, &flags))
		{
		  gboolean was_mapped = private->is_mapped;
		  gboolean is_mapped = (flags & XEMBED_MAPPED) != 0;

		  if (was_mapped != is_mapped)
		    {
		      if (is_mapped)
			ctk_socket_handle_map_request (socket);
		      else
			{
			  cdk_x11_display_error_trap_push (display);
			  cdk_window_show (private->plug_window);
			  cdk_x11_display_error_trap_pop_ignored (display);
			  
			  ctk_socket_unmap_notify (socket);
			}
		    }
		}
	      return_val = GDK_FILTER_REMOVE;
	    }
	}
      break;
    case ReparentNotify:
      {
        GdkWindow *window;
	XReparentEvent *xre = &xevent->xreparent;

        window = ctk_widget_get_window (widget);

	CTK_NOTE (PLUGSOCKET, g_message ("CtkSocket - ReparentNotify received"));
	if (!private->plug_window &&
            xre->parent == GDK_WINDOW_XID (window))
	  {
	    ctk_socket_add_window (socket, xre->window, FALSE);
	    
	    if (private->plug_window)
	      {
		CTK_NOTE (PLUGSOCKET, g_message ("CtkSocket - window reparented"));
	      }
	    
	    return_val = GDK_FILTER_REMOVE;
	  }
        else
          {
            if (private->plug_window &&
                xre->window == GDK_WINDOW_XID (private->plug_window) &&
                xre->parent != GDK_WINDOW_XID (window))
              {
                gboolean result;

                ctk_socket_end_embedding (socket);

                g_object_ref (widget);
                g_signal_emit_by_name (widget, "plug-removed", &result);
                if (!result)
                  ctk_widget_destroy (widget);
                g_object_unref (widget);

                return_val = GDK_FILTER_REMOVE;
              }
          }

	break;
      }
    case UnmapNotify:
      if (private->plug_window &&
	  xevent->xunmap.window == GDK_WINDOW_XID (private->plug_window))
	{
	  CTK_NOTE (PLUGSOCKET, g_message ("CtkSocket - Unmap notify"));

	  ctk_socket_unmap_notify (socket);
	  return_val = GDK_FILTER_REMOVE;
	}
      break;
      
    }
  
  return return_val;
}
