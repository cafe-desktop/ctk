/* GTK - The GIMP Toolkit
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

/* By Owen Taylor <otaylor@gtk.org>              98/4/4 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#include "config.h"

#include "gtksocketprivate.h"

#include <string.h>

#include "gtkmarshalers.h"
#include "gtksizerequest.h"
#include "gtkplug.h"
#include "gtkprivate.h"
#include "gtkrender.h"
#include "gtkdnd.h"
#include "gtkdragdest.h"
#include "gtkdebug.h"
#include "gtkintl.h"
#include "gtkmain.h"
#include "gtkwidgetprivate.h"

#include <gdk/gdkx.h>
#include <gdk/gdkprivate.h>

#ifdef HAVE_XFIXES
#include <X11/extensions/Xfixes.h>
#endif

#include "gtkxembed.h"

#include "a11y/gtksocketaccessible.h"


/**
 * SECTION:gtksocket
 * @Short_description: Container for widgets from other processes
 * @Title: GtkSocket
 * @include: gtk/gtkx.h
 * @See_also: #GtkPlug, [XEmbed Protocol](http://www.freedesktop.org/Standards/xembed-spec)
 *
 * Together with #GtkPlug, #GtkSocket provides the ability to embed
 * widgets from one process into another process in a fashion that
 * is transparent to the user. One process creates a #GtkSocket widget
 * and passes that widget’s window ID to the other process, which then
 * creates a #GtkPlug with that window ID. Any widgets contained in the
 * #GtkPlug then will appear inside the first application’s window.
 *
 * The socket’s window ID is obtained by using ctk_socket_get_id().
 * Before using this function, the socket must have been realized,
 * and for hence, have been added to its parent.
 *
 * ## Obtaining the window ID of a socket.
 *
 * |[<!-- language="C" -->
 * GtkWidget *socket = ctk_socket_new ();
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
 * When GTK+ is notified that the embedded window has been destroyed,
 * then it will destroy the socket as well. You should always,
 * therefore, be prepared for your sockets to be destroyed at any
 * time when the main event loop is running. To prevent this from
 * happening, you can connect to the #GtkSocket::plug-removed signal.
 *
 * The communication between a #GtkSocket and a #GtkPlug follows the
 * [XEmbed Protocol](http://www.freedesktop.org/Standards/xembed-spec).
 * This protocol has also been implemented in other toolkits, e.g. Qt,
 * allowing the same level of integration when embedding a Qt widget
 * in GTK or vice versa.
 *
 * The #GtkPlug and #GtkSocket widgets are only available when GTK+
 * is compiled for the X11 platform and %GDK_WINDOWING_X11 is defined.
 * They can only be used on a #GdkX11Display. To use #GtkPlug and
 * #GtkSocket, you need to include the `gtk/gtkx.h` header.
 */

/* Forward declararations */

static void     ctk_socket_finalize             (GObject          *object);
static void     ctk_socket_notify               (GObject          *object,
						 GParamSpec       *pspec);
static void     ctk_socket_realize              (GtkWidget        *widget);
static void     ctk_socket_unrealize            (GtkWidget        *widget);
static void     ctk_socket_get_preferred_width  (GtkWidget        *widget,
                                                 gint             *minimum,
                                                 gint             *natural);
static void     ctk_socket_get_preferred_height (GtkWidget        *widget,
                                                 gint             *minimum,
                                                 gint             *natural);
static void     ctk_socket_size_allocate        (GtkWidget        *widget,
						 GtkAllocation    *allocation);
static void     ctk_socket_hierarchy_changed    (GtkWidget        *widget,
						 GtkWidget        *old_toplevel);
static void     ctk_socket_grab_notify          (GtkWidget        *widget,
						 gboolean          was_grabbed);
static gboolean ctk_socket_key_event            (GtkWidget        *widget,
						 GdkEventKey      *event);
static gboolean ctk_socket_focus                (GtkWidget        *widget,
						 GtkDirectionType  direction);
static void     ctk_socket_remove               (GtkContainer     *container,
						 GtkWidget        *widget);
static void     ctk_socket_forall               (GtkContainer     *container,
						 gboolean          include_internals,
						 GtkCallback       callback,
						 gpointer          callback_data);
static void     ctk_socket_add_window           (GtkSocket        *socket,
                                                 Window            xid,
                                                 gboolean          need_reparent);
static GdkFilterReturn ctk_socket_filter_func   (GdkXEvent        *gdk_xevent,
                                                 GdkEvent         *event,
                                                 gpointer          data);

static gboolean xembed_get_info                 (GdkWindow        *gdk_window,
                                                 unsigned long    *version,
                                                 unsigned long    *flags);

static void     _ctk_socket_accessible_embed    (GtkWidget *socket,
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

G_DEFINE_TYPE_WITH_PRIVATE (GtkSocket, ctk_socket, CTK_TYPE_CONTAINER)

static void
ctk_socket_finalize (GObject *object)
{
  GtkSocket *socket = CTK_SOCKET (object);
  GtkSocketPrivate *priv = socket->priv;

  g_object_unref (priv->accel_group);

  G_OBJECT_CLASS (ctk_socket_parent_class)->finalize (object);
}

static gboolean
ctk_socket_draw (GtkWidget *widget,
                 cairo_t   *cr)
{
  ctk_render_background (ctk_widget_get_style_context (widget), cr,
                         0, 0,
                         ctk_widget_get_allocated_width (widget),
                         ctk_widget_get_allocated_height (widget));

  return CTK_WIDGET_CLASS (ctk_socket_parent_class)->draw (widget, cr);
}

static void
ctk_socket_class_init (GtkSocketClass *class)
{
  GtkWidgetClass *widget_class;
  GtkContainerClass *container_class;
  GObjectClass *gobject_class;

  gobject_class = (GObjectClass *) class;
  widget_class = (GtkWidgetClass*) class;
  container_class = (GtkContainerClass*) class;

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
   * GtkSocket::plug-added:
   * @socket_: the object which received the signal
   *
   * This signal is emitted when a client is successfully
   * added to the socket. 
   */
  socket_signals[PLUG_ADDED] =
    g_signal_new (I_("plug-added"),
		  G_OBJECT_CLASS_TYPE (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GtkSocketClass, plug_added),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * GtkSocket::plug-removed:
   * @socket_: the object which received the signal
   *
   * This signal is emitted when a client is removed from the socket. 
   * The default action is to destroy the #GtkSocket widget, so if you 
   * want to reuse it you must add a signal handler that returns %TRUE. 
   *
   * Returns: %TRUE to stop other handlers from being invoked.
   */
  socket_signals[PLUG_REMOVED] =
    g_signal_new (I_("plug-removed"),
		  G_OBJECT_CLASS_TYPE (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GtkSocketClass, plug_removed),
                  _ctk_boolean_handled_accumulator, NULL,
		  _ctk_marshal_BOOLEAN__VOID,
		  G_TYPE_BOOLEAN, 0);


  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_SOCKET_ACCESSIBLE);
}

static void
ctk_socket_init (GtkSocket *socket)
{
  GtkSocketPrivate *priv;

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
  g_object_set_data (G_OBJECT (priv->accel_group), I_("gtk-socket"), socket);
}

/**
 * ctk_socket_new:
 * 
 * Create a new empty #GtkSocket.
 * 
 * Returns:  the new #GtkSocket.
 **/
GtkWidget*
ctk_socket_new (void)
{
  GtkSocket *socket;

  socket = g_object_new (CTK_TYPE_SOCKET, NULL);

  return CTK_WIDGET (socket);
}

/**
 * ctk_socket_add_id:
 * @socket_: a #GtkSocket
 * @window: the Window of a client participating in the XEMBED protocol.
 *
 * Adds an XEMBED client, such as a #GtkPlug, to the #GtkSocket.  The
 * client may be in the same process or in a different process. 
 * 
 * To embed a #GtkPlug in a #GtkSocket, you can either create the
 * #GtkPlug with `ctk_plug_new (0)`, call 
 * ctk_plug_get_id() to get the window ID of the plug, and then pass that to the
 * ctk_socket_add_id(), or you can call ctk_socket_get_id() to get the
 * window ID for the socket, and call ctk_plug_new() passing in that
 * ID.
 *
 * The #GtkSocket must have already be added into a toplevel window
 *  before you can make this call.
 **/
void           
ctk_socket_add_id (GtkSocket      *socket,
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
 * @socket_: a #GtkSocket.
 * 
 * Gets the window ID of a #GtkSocket widget, which can then
 * be used to create a client embedded inside the socket, for
 * instance with ctk_plug_new(). 
 *
 * The #GtkSocket must have already be added into a toplevel window 
 * before you can make this call.
 * 
 * Returns: the window ID for the socket
 **/
Window
ctk_socket_get_id (GtkSocket *socket)
{
  g_return_val_if_fail (CTK_IS_SOCKET (socket), 0);
  g_return_val_if_fail (_ctk_widget_get_anchored (CTK_WIDGET (socket)), 0);

  if (!ctk_widget_get_realized (CTK_WIDGET (socket)))
    ctk_widget_realize (CTK_WIDGET (socket));

  return GDK_WINDOW_XID (ctk_widget_get_window (CTK_WIDGET (socket)));
}

/**
 * ctk_socket_get_plug_window:
 * @socket_: a #GtkSocket.
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
ctk_socket_get_plug_window (GtkSocket *socket)
{
  g_return_val_if_fail (CTK_IS_SOCKET (socket), NULL);

  return socket->priv->plug_window;
}

static void
ctk_socket_realize (GtkWidget *widget)
{
  GtkAllocation allocation;
  GdkWindow *window;
  GdkWindowAttr attributes;
  XWindowAttributes xattrs;
  gint attributes_mask;
  GdkScreen *screen;

  ctk_widget_set_realized (widget, TRUE);

  screen = ctk_widget_get_screen (widget);
  if (!GDK_IS_X11_SCREEN (screen))
    g_warning ("GtkSocket: only works under X11");

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

  window = gdk_window_new (ctk_widget_get_parent_window (widget),
                           &attributes, attributes_mask);
  ctk_widget_set_window (widget, window);
  ctk_widget_register_window (widget, window);

  XGetWindowAttributes (GDK_WINDOW_XDISPLAY (window),
			GDK_WINDOW_XID (window),
			&xattrs);

  /* Sooooo, it turns out that mozilla, as per the gtk2xt code selects
     for input on the socket with a mask of 0x0fffff (for god knows why)
     which includes ButtonPressMask causing a BadAccess if someone else
     also selects for this. As per the client-side windows merge we always
     normally selects for button press so we can emulate it on client
     side children that selects for button press. However, we don't need
     this for GtkSocket, so we unselect it here, fixing the crashes in
     firefox. */
  XSelectInput (GDK_WINDOW_XDISPLAY (window),
		GDK_WINDOW_XID (window), 
		(xattrs.your_event_mask & ~ButtonPressMask) |
		SubstructureNotifyMask | SubstructureRedirectMask);

  gdk_window_add_filter (window,
			 ctk_socket_filter_func,
			 widget);

  /* We sync here so that we make sure that if the XID for
   * our window is passed to another application, SubstructureRedirectMask
   * will be set by the time the other app creates its window.
   */
  gdk_display_sync (ctk_widget_get_display (widget));
}

/**
 * ctk_socket_end_embedding:
 * @socket: a #GtkSocket
 *
 * Called to end the embedding of a plug in the socket.
 */
static void
ctk_socket_end_embedding (GtkSocket *socket)
{
  GtkSocketPrivate *private = socket->priv;

  g_object_unref (private->plug_window);
  private->plug_window = NULL;
  private->current_width = 0;
  private->current_height = 0;
  private->resize_count = 0;

  ctk_accel_group_disconnect (private->accel_group, NULL);
}

static void
ctk_socket_unrealize (GtkWidget *widget)
{
  GtkSocket *socket = CTK_SOCKET (widget);
  GtkSocketPrivate *private = socket->priv;

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
ctk_socket_size_request (GtkSocket *socket)
{
  GtkSocketPrivate *private = socket->priv;
  GdkDisplay *display;
  XSizeHints hints;
  long supplied;
  int scale;

  display = ctk_widget_get_display (CTK_WIDGET (socket));
  gdk_x11_display_error_trap_push (display);

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
  
  gdk_x11_display_error_trap_pop_ignored (display);
}

static void
ctk_socket_get_preferred_width (GtkWidget *widget,
                                gint      *minimum,
                                gint      *natural)
{
  GtkSocket *socket = CTK_SOCKET (widget);
  GtkSocketPrivate *private = socket->priv;

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
ctk_socket_get_preferred_height (GtkWidget *widget,
                                 gint      *minimum,
                                 gint      *natural)
{
  GtkSocket *socket = CTK_SOCKET (widget);
  GtkSocketPrivate *private = socket->priv;

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
ctk_socket_send_configure_event (GtkSocket *socket)
{
  GtkAllocation allocation;
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
  display = gdk_window_get_display (socket->priv->plug_window);
  gdk_x11_display_error_trap_push (display);
  gdk_window_get_origin (socket->priv->plug_window, &x, &y);
  gdk_x11_display_error_trap_pop_ignored (display);

  ctk_widget_get_allocation (CTK_WIDGET(socket), &allocation);
  scale = ctk_widget_get_scale_factor (CTK_WIDGET(socket));
  xconfigure.x = x * scale;
  xconfigure.y = y * scale;
  xconfigure.width = allocation.width * scale;
  xconfigure.height = allocation.height * scale;

  xconfigure.border_width = 0;
  xconfigure.above = None;
  xconfigure.override_redirect = False;

  gdk_x11_display_error_trap_push (display);
  XSendEvent (GDK_WINDOW_XDISPLAY (socket->priv->plug_window),
	      GDK_WINDOW_XID (socket->priv->plug_window),
	      False, NoEventMask, (XEvent *)&xconfigure);
  gdk_x11_display_error_trap_pop_ignored (display);
}

static void
ctk_socket_size_allocate (GtkWidget     *widget,
			  GtkAllocation *allocation)
{
  GtkSocket *socket = CTK_SOCKET (widget);
  GtkSocketPrivate *private = socket->priv;

  ctk_widget_set_allocation (widget, allocation);
  if (ctk_widget_get_realized (widget))
    {
      gdk_window_move_resize (ctk_widget_get_window (widget),
			      allocation->x, allocation->y,
			      allocation->width, allocation->height);

      if (private->plug_widget)
	{
	  GtkAllocation child_allocation;

	  child_allocation.x = 0;
	  child_allocation.y = 0;
	  child_allocation.width = allocation->width;
	  child_allocation.height = allocation->height;

	  ctk_widget_size_allocate (private->plug_widget, &child_allocation);
	}
      else if (private->plug_window)
	{
          GdkDisplay *display = gdk_window_get_display (private->plug_window);

	  gdk_x11_display_error_trap_push (display);

	  if (allocation->width != private->current_width ||
	      allocation->height != private->current_height)
	    {
	      gdk_window_move_resize (private->plug_window,
				      0, 0,
				      allocation->width, allocation->height);
	      if (private->resize_count)
		private->resize_count--;
	      
	      CTK_NOTE (PLUGSOCKET,
			g_message ("GtkSocket - allocated: %d %d",
				   allocation->width, allocation->height));
	      private->current_width = allocation->width;
	      private->current_height = allocation->height;
	    }

	  if (private->need_map)
	    {
	      gdk_window_show (private->plug_window);
	      private->need_map = FALSE;
	    }

	  while (private->resize_count)
 	    {
 	      ctk_socket_send_configure_event (socket);
 	      private->resize_count--;
 	      CTK_NOTE (PLUGSOCKET,
			g_message ("GtkSocket - sending synthetic configure: %d %d",
				   allocation->width, allocation->height));
 	    }

	  gdk_x11_display_error_trap_pop_ignored (display);
	}
    }
}

static void
ctk_socket_send_key_event (GtkSocket *socket,
			   GdkEvent  *gdk_event,
			   gboolean   mask_key_presses)
{
  XKeyEvent xkey;
  GdkScreen *screen = gdk_window_get_screen (socket->priv->plug_window);

  memset (&xkey, 0, sizeof (xkey));
  xkey.type = (gdk_event->type == GDK_KEY_PRESS) ? KeyPress : KeyRelease;
  xkey.window = GDK_WINDOW_XID (socket->priv->plug_window);
  xkey.root = GDK_WINDOW_XID (gdk_screen_get_root_window (screen));
  xkey.subwindow = None;
  xkey.time = gdk_event->key.time;
  xkey.x = 0;
  xkey.y = 0;
  xkey.x_root = 0;
  xkey.y_root = 0;
  xkey.state = gdk_event->key.state;
  xkey.keycode = gdk_event->key.hardware_keycode;
  xkey.same_screen = True;/* FIXME ? */

  gdk_x11_display_error_trap_push (gdk_window_get_display (socket->priv->plug_window));
  XSendEvent (GDK_WINDOW_XDISPLAY (socket->priv->plug_window),
	      GDK_WINDOW_XID (socket->priv->plug_window),
	      False,
	      (mask_key_presses ? KeyPressMask : NoEventMask),
	      (XEvent *)&xkey);
  gdk_x11_display_error_trap_pop_ignored (gdk_window_get_display (socket->priv->plug_window));
}

static gboolean
activate_key (GtkAccelGroup  *accel_group,
	      GObject        *acceleratable,
	      guint           accel_key,
	      GdkModifierType accel_mods,
	      GrabbedKey     *grabbed_key)
{
  GdkEvent *gdk_event = ctk_get_current_event ();
  
  GtkSocket *socket = g_object_get_data (G_OBJECT (accel_group), "gtk-socket");
  gboolean retval = FALSE;

  if (gdk_event && gdk_event->type == GDK_KEY_PRESS && socket->priv->plug_window)
    {
      ctk_socket_send_key_event (socket, gdk_event, FALSE);
      retval = TRUE;
    }

  if (gdk_event)
    gdk_event_free (gdk_event);

  return retval;
}

static gboolean
find_accel_key (GtkAccelKey *key,
		GClosure    *closure,
		gpointer     data)
{
  GrabbedKey *grabbed_key = data;
  
  return (key->accel_key == grabbed_key->accel_key &&
	  key->accel_mods == grabbed_key->accel_mods);
}

/**
 * ctk_socket_add_grabbed_key:
 * @socket: a #GtkSocket
 * @keyval: a key
 * @modifiers: modifiers for the key
 *
 * Called from the GtkSocket platform-specific backend when the
 * corresponding plug has told the socket to grab a key.
 */
static void
ctk_socket_add_grabbed_key (GtkSocket       *socket,
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
      g_warning ("GtkSocket: request to add already present grabbed key %u,%#x",
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
 * @socket: a #GtkSocket
 * @keyval: a key
 * @modifiers: modifiers for the key
 *
 * Called from the GtkSocket backend when the corresponding plug has
 * told the socket to remove a key grab.
 */
static void
ctk_socket_remove_grabbed_key (GtkSocket      *socket,
			       guint           keyval,
			       GdkModifierType modifiers)
{
  if (!ctk_accel_group_disconnect_key (socket->priv->accel_group, keyval, modifiers))
    g_warning ("GtkSocket: request to remove non-present grabbed key %u,%#x",
	       keyval, modifiers);
}

static void
socket_update_focus_in (GtkSocket *socket)
{
  GtkSocketPrivate *private = socket->priv;
  gboolean focus_in = FALSE;

  if (private->plug_window)
    {
      GtkWidget *toplevel = ctk_widget_get_toplevel (CTK_WIDGET (socket));

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
socket_update_active (GtkSocket *socket)
{
  GtkSocketPrivate *private = socket->priv;
  gboolean active = FALSE;

  if (private->plug_window)
    {
      GtkWidget *toplevel = ctk_widget_get_toplevel (CTK_WIDGET (socket));

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
ctk_socket_hierarchy_changed (GtkWidget *widget,
			      GtkWidget *old_toplevel)
{
  GtkSocket *socket = CTK_SOCKET (widget);
  GtkSocketPrivate *private = socket->priv;
  GtkWidget *toplevel = ctk_widget_get_toplevel (widget);

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
ctk_socket_grab_notify (GtkWidget *widget,
			gboolean   was_grabbed)
{
  GtkSocket *socket = CTK_SOCKET (widget);

  if (!socket->priv->same_app)
    _ctk_xembed_send_message (socket->priv->plug_window,
                              was_grabbed ? XEMBED_MODALITY_OFF : XEMBED_MODALITY_ON,
                              0, 0, 0);
}

static gboolean
ctk_socket_key_event (GtkWidget   *widget,
                      GdkEventKey *event)
{
  GtkSocket *socket = CTK_SOCKET (widget);
  GtkSocketPrivate *private = socket->priv;
  
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
 * @socket: a #GtkSocket
 * @send_event: huh?
 *
 * Claims focus for the socket. XXX send_event?
 */
static void
ctk_socket_claim_focus (GtkSocket *socket,
			gboolean   send_event)
{
  GtkWidget *widget = CTK_WIDGET (socket);
  GtkSocketPrivate *private = socket->priv;

  if (!send_event)
    private->focus_in = TRUE;	/* Otherwise, our notify handler will send FOCUS_IN  */
      
  /* Oh, the trickery... */
  
  ctk_widget_set_can_focus (widget, TRUE);
  ctk_widget_grab_focus (widget);
  ctk_widget_set_can_focus (widget, FALSE);
}

static gboolean
ctk_socket_focus (GtkWidget       *widget,
		  GtkDirectionType direction)
{
  GtkSocket *socket = CTK_SOCKET (widget);
  GtkSocketPrivate *private = socket->priv;

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
ctk_socket_remove (GtkContainer *container,
		   GtkWidget    *child)
{
  GtkSocket *socket = CTK_SOCKET (container);
  GtkSocketPrivate *private = socket->priv;

  g_return_if_fail (child == private->plug_widget);

  _ctk_plug_remove_from_socket (CTK_PLUG (private->plug_widget), socket);
}

static void
ctk_socket_forall (GtkContainer *container,
		   gboolean      include_internals,
		   GtkCallback   callback,
		   gpointer      callback_data)
{
  GtkSocket *socket = CTK_SOCKET (container);
  GtkSocketPrivate *private = socket->priv;

  if (private->plug_widget)
    (* callback) (private->plug_widget, callback_data);
}

/**
 * ctk_socket_add_window:
 * @socket: a #GtkSocket
 * @xid: the native identifier for a window
 * @need_reparent: whether the socket’s plug’s window needs to be
 *                 reparented to the socket
 *
 * Adds a window to a GtkSocket.
 */
static void
ctk_socket_add_window (GtkSocket       *socket,
		       Window           xid,
		       gboolean         need_reparent)
{
  GtkWidget *widget = CTK_WIDGET (socket);
  GdkDisplay *display = ctk_widget_get_display (widget);
  gpointer user_data = NULL;
  GtkSocketPrivate *private = socket->priv;
  unsigned long version;
  unsigned long flags;

  if (GDK_IS_X11_DISPLAY (display))
    private->plug_window = gdk_x11_window_lookup_for_display (display, xid);
  else
    private->plug_window = NULL;

  if (private->plug_window)
    {
      g_object_ref (private->plug_window);
      gdk_window_get_user_data (private->plug_window, &user_data);
    }

  if (user_data) /* A widget's window in this process */
    {
      GtkWidget *child_widget = user_data;

      if (!CTK_IS_PLUG (child_widget))
        {
          g_warning (G_STRLOC ": Can't add non-GtkPlug to GtkSocket");
          private->plug_window = NULL;
          gdk_x11_display_error_trap_pop_ignored (display);

          return;
        }

      _ctk_plug_add_to_socket (CTK_PLUG (child_widget), socket);
    }
  else  /* A foreign window */
    {
      GdkDragProtocol protocol;

      gdk_x11_display_error_trap_push (display);

      if (!private->plug_window)
        {
          if (GDK_IS_X11_DISPLAY (display))
            private->plug_window = gdk_x11_window_foreign_new_for_display (display, xid);
          if (!private->plug_window) /* was deleted before we could get it */
            {
              gdk_x11_display_error_trap_pop_ignored (display);
              return;
            }
        }

      XSelectInput (GDK_DISPLAY_XDISPLAY (display),
                    GDK_WINDOW_XID (private->plug_window),
                    StructureNotifyMask | PropertyChangeMask);

      if (gdk_x11_display_error_trap_pop (display))
	{
	  g_object_unref (private->plug_window);
	  private->plug_window = NULL;
	  return;
	}
      
      /* OK, we now will reliably get destroy notification on socket->plug_window */

      gdk_x11_display_error_trap_push (display);

      if (need_reparent)
	{
	  gdk_window_hide (private->plug_window); /* Shouldn't actually be necessary for XEMBED, but just in case */
	  gdk_window_reparent (private->plug_window,
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
      protocol = gdk_window_get_drag_protocol (private->plug_window, NULL);
      if (protocol)
	ctk_drag_dest_set_proxy (widget, private->plug_window, protocol, TRUE);
G_GNUC_END_IGNORE_DEPRECATIONS

      gdk_x11_display_error_trap_pop_ignored (display);

      gdk_window_add_filter (private->plug_window,
			     ctk_socket_filter_func,
			     socket);

#ifdef HAVE_XFIXES
      gdk_x11_display_error_trap_push (display);
      XFixesChangeSaveSet (GDK_DISPLAY_XDISPLAY (display),
                           GDK_WINDOW_XID (private->plug_window),
                           SetModeInsert, SaveSetRoot, SaveSetUnmap);
      gdk_x11_display_error_trap_pop_ignored (display);
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
 * @socket: a #GtkSocket
 *
 * Called from the GtkSocket backend when the plug has been mapped.
 */
static void
ctk_socket_handle_map_request (GtkSocket *socket)
{
  GtkSocketPrivate *private = socket->priv;
  if (!private->is_mapped)
    {
      private->is_mapped = TRUE;
      private->need_map = TRUE;

      ctk_widget_queue_resize (CTK_WIDGET (socket));
    }
}

/**
 * ctk_socket_unmap_notify:
 * @socket: a #GtkSocket
 *
 * Called from the GtkSocket backend when the plug has been unmapped ???
 */
static void
ctk_socket_unmap_notify (GtkSocket *socket)
{
  GtkSocketPrivate *private = socket->priv;
  if (private->is_mapped)
    {
      private->is_mapped = FALSE;
      ctk_widget_queue_resize (CTK_WIDGET (socket));
    }
}

/**
 * ctk_socket_advance_toplevel_focus:
 * @socket: a #GtkSocket
 * @direction: a direction
 *
 * Called from the GtkSocket backend when the corresponding plug
 * has told the socket to move the focus.
 */
static void
ctk_socket_advance_toplevel_focus (GtkSocket        *socket,
				   GtkDirectionType  direction)
{
  GtkBin *bin;
  GtkWindow *window;
  GtkContainer *container;
  GtkWidget *child;
  GtkWidget *focus_widget;
  GtkWidget *toplevel;
  GtkWidget *old_focus_child;
  GtkWidget *parent;

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
  GdkDisplay *display = gdk_window_get_display (window);
  Atom xembed_info_atom = gdk_x11_get_xatom_by_name_for_display (display, "_XEMBED_INFO");
  Atom type;
  int format;
  unsigned long nitems, bytes_after;
  unsigned char *data;
  unsigned long *data_long;
  int status;
  
  gdk_x11_display_error_trap_push (display);
  status = XGetWindowProperty (GDK_DISPLAY_XDISPLAY (display),
			       GDK_WINDOW_XID (window),
			       xembed_info_atom,
			       0, 2, False,
			       xembed_info_atom, &type, &format,
			       &nitems, &bytes_after, &data);
  gdk_x11_display_error_trap_pop_ignored (display);

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
handle_xembed_message (GtkSocket        *socket,
		       XEmbedMessageType message,
		       glong             detail,
		       glong             data1,
		       glong             data2,
		       guint32           time)
{
  CTK_NOTE (PLUGSOCKET,
	    g_message ("GtkSocket: %s received", _ctk_xembed_message_name (message)));
  
  switch (message)
    {
    case XEMBED_EMBEDDED_NOTIFY:
    case XEMBED_WINDOW_ACTIVATE:
    case XEMBED_WINDOW_DEACTIVATE:
    case XEMBED_MODALITY_ON:
    case XEMBED_MODALITY_OFF:
    case XEMBED_FOCUS_IN:
    case XEMBED_FOCUS_OUT:
      g_warning ("GtkSocket: Invalid _XEMBED message %s received", _ctk_xembed_message_name (message));
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
		g_message ("GtkSocket: Ignoring unknown _XEMBED message of type %d", message));
      break;
    }
}

static void
_ctk_socket_accessible_embed (GtkWidget *socket, GdkWindow *window)
{
  GdkDisplay *display = gdk_window_get_display (window);
  Atom net_at_spi_path_atom = gdk_x11_get_xatom_by_name_for_display (display, "_XEMBED_AT_SPI_PATH");
  Atom type;
  int format;
  unsigned long nitems, bytes_after;
  unsigned char *data;
  int status;

  gdk_x11_display_error_trap_push (display);
  status = XGetWindowProperty (GDK_DISPLAY_XDISPLAY (display),
			       GDK_WINDOW_XID (window),
			       net_at_spi_path_atom,
			       0, INT_MAX / 4, False,
			       net_at_spi_path_atom, &type, &format,
			       &nitems, &bytes_after, &data);
  gdk_x11_display_error_trap_pop_ignored (display);

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
ctk_socket_filter_func (GdkXEvent *gdk_xevent,
			GdkEvent  *event,
			gpointer   data)
{
  GtkSocket *socket;
  GtkWidget *widget;
  GdkDisplay *display;
  XEvent *xevent;
  GtkSocketPrivate *private;

  GdkFilterReturn return_val;

  socket = CTK_SOCKET (data);
  private = socket->priv;

  return_val = GDK_FILTER_CONTINUE;

  if (private->plug_widget)
    return return_val;

  widget = CTK_WIDGET (socket);
  xevent = (XEvent *)gdk_xevent;
  display = ctk_widget_get_display (widget);

  switch (xevent->type)
    {
    case ClientMessage:
      if (xevent->xclient.message_type == gdk_x11_get_xatom_by_name_for_display (display, "_XEMBED"))
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
		CTK_NOTE (PLUGSOCKET, g_message ("GtkSocket - window created"));
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
			  g_message ("GtkSocket - configure request: %d %d",
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
	    
	    CTK_NOTE (PLUGSOCKET, g_message ("GtkSocket - destroy notify"));
	    
	    gdk_window_destroy_notify (private->plug_window);
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
	  CTK_NOTE (PLUGSOCKET, g_message ("GtkSocket - Map Request"));

	  ctk_socket_handle_map_request (socket);
	  return_val = GDK_FILTER_REMOVE;
	}
      break;
    case PropertyNotify:
      if (private->plug_window &&
	  xevent->xproperty.window == GDK_WINDOW_XID (private->plug_window))
	{
	  GdkDragProtocol protocol;

	  if (xevent->xproperty.atom == gdk_x11_get_xatom_by_name_for_display (display, "WM_NORMAL_HINTS"))
	    {
	      CTK_NOTE (PLUGSOCKET, g_message ("GtkSocket - received PropertyNotify for plug's WM_NORMAL_HINTS"));
	      private->have_size = FALSE;
	      ctk_widget_queue_resize (widget);
	      return_val = GDK_FILTER_REMOVE;
	    }
	  else if ((xevent->xproperty.atom == gdk_x11_get_xatom_by_name_for_display (display, "XdndAware")) ||
	      (xevent->xproperty.atom == gdk_x11_get_xatom_by_name_for_display (display, "_MOTIF_DRAG_RECEIVER_INFO")))
	    {
	      gdk_x11_display_error_trap_push (display);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
              protocol = gdk_window_get_drag_protocol (private->plug_window, NULL);
              if (protocol)
		ctk_drag_dest_set_proxy (CTK_WIDGET (socket),
					 private->plug_window,
					 protocol, TRUE);
G_GNUC_END_IGNORE_DEPRECATIONS

	      gdk_x11_display_error_trap_pop_ignored (display);
	      return_val = GDK_FILTER_REMOVE;
	    }
	  else if (xevent->xproperty.atom == gdk_x11_get_xatom_by_name_for_display (display, "_XEMBED_INFO"))
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
			  gdk_x11_display_error_trap_push (display);
			  gdk_window_show (private->plug_window);
			  gdk_x11_display_error_trap_pop_ignored (display);
			  
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

	CTK_NOTE (PLUGSOCKET, g_message ("GtkSocket - ReparentNotify received"));
	if (!private->plug_window &&
            xre->parent == GDK_WINDOW_XID (window))
	  {
	    ctk_socket_add_window (socket, xre->window, FALSE);
	    
	    if (private->plug_window)
	      {
		CTK_NOTE (PLUGSOCKET, g_message ("GtkSocket - window reparented"));
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
	  CTK_NOTE (PLUGSOCKET, g_message ("GtkSocket - Unmap notify"));

	  ctk_socket_unmap_notify (socket);
	  return_val = GDK_FILTER_REMOVE;
	}
      break;
      
    }
  
  return return_val;
}
