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

/* By Owen Taylor <otaylor@ctk.org>              98/4/4 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.ctk.org/pub/ctk/. 
 */

#include "config.h"

#include "ctkdebug.h"
#include "ctkmain.h"
#include "ctkmarshalers.h"
#include "ctkplug.h"
#include "ctkintl.h"
#include "ctkprivate.h"
#include "ctkrender.h"
#include "ctksocketprivate.h"
#include "ctkwidgetprivate.h"
#include "ctkwindowgroup.h"
#include "ctkwindowprivate.h"
#include "ctkxembed.h"

#include "a11y/ctkplugaccessible.h"

#include <gdk/gdkx.h>

/**
 * SECTION:ctkplug
 * @Short_description: Toplevel for embedding into other processes
 * @Title: CtkPlug
 * @include: ctk/ctkx.h
 * @See_also: #CtkSocket
 *
 * Together with #CtkSocket, #CtkPlug provides the ability to embed
 * widgets from one process into another process in a fashion that is
 * transparent to the user. One process creates a #CtkSocket widget
 * and passes the ID of that widget’s window to the other process,
 * which then creates a #CtkPlug with that window ID. Any widgets
 * contained in the #CtkPlug then will appear inside the first
 * application’s window.
 *
 * The communication between a #CtkSocket and a #CtkPlug follows the
 * [XEmbed Protocol](http://www.freedesktop.org/Standards/xembed-spec).
 * This protocol has also been implemented in other toolkits, e.g. Qt,
 * allowing the same level of integration when embedding a Qt widget
 * in GTK+ or vice versa.
 *
 * The #CtkPlug and #CtkSocket widgets are only available when GTK+
 * is compiled for the X11 platform and %GDK_WINDOWING_X11 is defined.
 * They can only be used on a #GdkX11Display. To use #CtkPlug and
 * #CtkSocket, you need to include the `ctk/ctkx.h` header.
 */

struct _CtkPlugPrivate
{
  CtkWidget      *modality_window;
  CtkWindowGroup *modality_group;

  GdkWindow      *socket_window;

  GHashTable     *grabbed_keys;

  guint  same_app : 1;
};

static void            ctk_plug_get_property          (GObject     *object,
						       guint        prop_id,
						       GValue      *value,
						       GParamSpec  *pspec);
static void            ctk_plug_finalize              (GObject          *object);
static void            ctk_plug_realize               (CtkWidget        *widget);
static void            ctk_plug_unrealize             (CtkWidget        *widget);
static void            ctk_plug_show                  (CtkWidget        *widget);
static void            ctk_plug_hide                  (CtkWidget        *widget);
static void            ctk_plug_map                   (CtkWidget        *widget);
static void            ctk_plug_unmap                 (CtkWidget        *widget);
static gboolean        ctk_plug_key_press_event       (CtkWidget        *widget,
						       GdkEventKey      *event);
static gboolean        ctk_plug_focus_event           (CtkWidget        *widget,
						       GdkEventFocus    *event);
static void            ctk_plug_set_focus             (CtkWindow        *window,
						       CtkWidget        *focus);
static gboolean        ctk_plug_focus                 (CtkWidget        *widget,
						       CtkDirectionType  direction);
static void            ctk_plug_check_resize          (CtkContainer     *container);
static void            ctk_plug_keys_changed          (CtkWindow        *window);

static void            xembed_set_info                (GdkWindow        *window,
				                       unsigned long     flags);

static CtkBinClass *bin_class = NULL;

typedef struct
{
  guint			 accelerator_key;
  GdkModifierType	 accelerator_mods;
} GrabbedKey;

enum {
  PROP_0,
  PROP_EMBEDDED,
  PROP_SOCKET_WINDOW
};

enum {
  EMBEDDED,
  LAST_SIGNAL
}; 

static guint plug_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (CtkPlug, ctk_plug, CTK_TYPE_WINDOW)

static void
ctk_plug_get_property (GObject    *object,
		       guint       prop_id,
		       GValue     *value,
		       GParamSpec *pspec)
{
  CtkPlug *plug = CTK_PLUG (object);
  CtkPlugPrivate *priv = plug->priv;

  switch (prop_id)
    {
    case PROP_EMBEDDED:
      g_value_set_boolean (value, priv->socket_window != NULL);
      break;
    case PROP_SOCKET_WINDOW:
      g_value_set_object (value, priv->socket_window);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_plug_class_init (CtkPlugClass *class)
{
  GObjectClass *gobject_class = (GObjectClass *)class;
  CtkWidgetClass *widget_class = (CtkWidgetClass *)class;
  CtkWindowClass *window_class = (CtkWindowClass *)class;
  CtkContainerClass *container_class = (CtkContainerClass *)class;

  bin_class = g_type_class_peek (CTK_TYPE_BIN);

  gobject_class->get_property = ctk_plug_get_property;
  gobject_class->finalize = ctk_plug_finalize;
  
  widget_class->realize = ctk_plug_realize;
  widget_class->unrealize = ctk_plug_unrealize;
  widget_class->key_press_event = ctk_plug_key_press_event;
  widget_class->focus_in_event = ctk_plug_focus_event;
  widget_class->focus_out_event = ctk_plug_focus_event;

  widget_class->show = ctk_plug_show;
  widget_class->hide = ctk_plug_hide;
  widget_class->map = ctk_plug_map;
  widget_class->unmap = ctk_plug_unmap;
  widget_class->focus = ctk_plug_focus;

  ctk_widget_class_set_accessible_role (widget_class, ATK_ROLE_PANEL);

  container_class->check_resize = ctk_plug_check_resize;

  window_class->set_focus = ctk_plug_set_focus;
  window_class->keys_changed = ctk_plug_keys_changed;

  /**
   * CtkPlug:embedded:
   *
   * %TRUE if the plug is embedded in a socket.
   *
   * Since: 2.12
   */
  g_object_class_install_property (gobject_class,
				   PROP_EMBEDDED,
				   g_param_spec_boolean ("embedded",
							 P_("Embedded"),
							 P_("Whether the plug is embedded"),
							 FALSE,
							 CTK_PARAM_READABLE));

  /**
   * CtkPlug:socket-window:
   *
   * The window of the socket the plug is embedded in.
   *
   * Since: 2.14
   */
  g_object_class_install_property (gobject_class,
				   PROP_SOCKET_WINDOW,
				   g_param_spec_object ("socket-window",
							P_("Socket Window"),
							P_("The window of the socket the plug is embedded in"),
							GDK_TYPE_WINDOW,
							CTK_PARAM_READABLE));

  /**
   * CtkPlug::embedded:
   * @plug: the object on which the signal was emitted
   *
   * Gets emitted when the plug becomes embedded in a socket.
   */ 
  plug_signals[EMBEDDED] =
    g_signal_new (I_("embedded"),
		  G_OBJECT_CLASS_TYPE (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkPlugClass, embedded),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

#ifdef CTK_HAVE_ATK_PLUG_SET_CHILD
  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_PLUG_ACCESSIBLE);
#endif /* CTK_HAVE_ATK_PLUG_SET_CHILD */
}

static void
ctk_plug_init (CtkPlug *plug)
{
  plug->priv = ctk_plug_get_instance_private (plug);

  ctk_window_set_decorated (CTK_WINDOW (plug), FALSE);
}

/**
 * ctk_plug_handle_modality_on:
 * @plug: a #CtkPlug
 *
 * Called from the CtkPlug backend when the corresponding socket has
 * told the plug that it modality has toggled on.
 */
static void
ctk_plug_handle_modality_on (CtkPlug *plug)
{
  CtkPlugPrivate *priv = plug->priv;

  if (!priv->modality_window)
    {
      priv->modality_window = ctk_window_new (CTK_WINDOW_POPUP);
      ctk_window_set_screen (CTK_WINDOW (priv->modality_window),
			     ctk_widget_get_screen (CTK_WIDGET (plug)));
      ctk_widget_realize (priv->modality_window);
      ctk_window_group_add_window (priv->modality_group, CTK_WINDOW (priv->modality_window));
      ctk_grab_add (priv->modality_window);
    }
}

/**
 * ctk_plug_handle_modality_off:
 * @plug: a #CtkPlug
 *
 * Called from the CtkPlug backend when the corresponding socket has
 * told the plug that it modality has toggled off.
 */
static void
ctk_plug_handle_modality_off (CtkPlug *plug)
{
  CtkPlugPrivate *priv = plug->priv;

  if (priv->modality_window)
    {
      ctk_widget_destroy (priv->modality_window);
      priv->modality_window = NULL;
    }
}

static void
ctk_plug_set_is_child (CtkPlug  *plug,
		       gboolean  is_child)
{
  CtkPlugPrivate *priv = plug->priv;
  CtkWidget *widget = CTK_WIDGET (plug);

  g_assert (!ctk_widget_get_parent (widget));

  if (is_child)
    {
      if (priv->modality_window)
	ctk_plug_handle_modality_off (plug);

      if (priv->modality_group)
	{
	  ctk_window_group_remove_window (priv->modality_group, CTK_WINDOW (plug));
	  g_object_unref (priv->modality_group);
	  priv->modality_group = NULL;
	}
      
      /* As a toplevel, the MAPPED flag doesn't correspond
       * to whether the widget->window is mapped; we unmap
       * here, but don't bother remapping -- we will get mapped
       * by ctk_widget_set_parent ().
       */
      if (ctk_widget_get_mapped (widget))
	ctk_widget_unmap (widget);

      _ctk_window_set_is_toplevel (CTK_WINDOW (plug), FALSE);
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      ctk_container_set_resize_mode (CTK_CONTAINER (plug), CTK_RESIZE_PARENT);
      G_GNUC_END_IGNORE_DEPRECATIONS;

      _ctk_widget_propagate_hierarchy_changed (widget, widget);
    }
  else
    {
      if (ctk_window_get_focus (CTK_WINDOW (plug)))
	ctk_window_set_focus (CTK_WINDOW (plug), NULL);
      if (ctk_window_get_default_widget (CTK_WINDOW (plug)))
	ctk_window_set_default (CTK_WINDOW (plug), NULL);

      priv->modality_group = ctk_window_group_new ();
      ctk_window_group_add_window (priv->modality_group, CTK_WINDOW (plug));

      _ctk_window_set_is_toplevel (CTK_WINDOW (plug), TRUE);
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      ctk_container_set_resize_mode (CTK_CONTAINER (plug), CTK_RESIZE_QUEUE);
      G_GNUC_END_IGNORE_DEPRECATIONS;

      _ctk_widget_propagate_hierarchy_changed (CTK_WIDGET (plug), NULL);
    }
}

/**
 * ctk_plug_get_id:
 * @plug: a #CtkPlug.
 * 
 * Gets the window ID of a #CtkPlug widget, which can then
 * be used to embed this window inside another window, for
 * instance with ctk_socket_add_id().
 * 
 * Returns: the window ID for the plug
 **/
Window
ctk_plug_get_id (CtkPlug *plug)
{
  g_return_val_if_fail (CTK_IS_PLUG (plug), 0);

  if (!ctk_widget_get_realized (CTK_WIDGET (plug)))
    ctk_widget_realize (CTK_WIDGET (plug));

  return GDK_WINDOW_XID (ctk_widget_get_window (CTK_WIDGET (plug)));
}

/**
 * ctk_plug_get_embedded:
 * @plug: a #CtkPlug
 *
 * Determines whether the plug is embedded in a socket.
 *
 * Returns: %TRUE if the plug is embedded in a socket
 *
 * Since: 2.14
 **/
gboolean
ctk_plug_get_embedded (CtkPlug *plug)
{
  g_return_val_if_fail (CTK_IS_PLUG (plug), FALSE);

  return plug->priv->socket_window != NULL;
}

/**
 * ctk_plug_get_socket_window:
 * @plug: a #CtkPlug
 *
 * Retrieves the socket the plug is embedded in.
 *
 * Returns: (nullable) (transfer none): the window of the socket, or %NULL
 *
 * Since: 2.14
 **/
GdkWindow *
ctk_plug_get_socket_window (CtkPlug *plug)
{
  g_return_val_if_fail (CTK_IS_PLUG (plug), NULL);

  return plug->priv->socket_window;
}

/**
 * _ctk_plug_add_to_socket:
 * @plug: a #CtkPlug
 * @socket_: a #CtkSocket
 * 
 * Adds a plug to a socket within the same application.
 **/
void
_ctk_plug_add_to_socket (CtkPlug   *plug,
			 CtkSocket *socket_)
{
  CtkPlugPrivate *priv;
  CtkWidget *widget;
  
  g_return_if_fail (CTK_IS_PLUG (plug));
  g_return_if_fail (CTK_IS_SOCKET (socket_));
  g_return_if_fail (ctk_widget_get_realized (CTK_WIDGET (socket_)));

  priv = plug->priv;
  widget = CTK_WIDGET (plug);

  ctk_plug_set_is_child (plug, TRUE);
  priv->same_app = TRUE;
  socket_->priv->same_app = TRUE;
  socket_->priv->plug_widget = widget;

  priv->socket_window = ctk_widget_get_window (CTK_WIDGET (socket_));
  g_object_ref (priv->socket_window);
  g_signal_emit (plug, plug_signals[EMBEDDED], 0);
  g_object_notify (G_OBJECT (plug), "embedded");

  if (ctk_widget_get_realized (widget))
    {
      GdkWindow *window;

      window = ctk_widget_get_window (widget);
      gdk_window_reparent (window, priv->socket_window,
                           -gdk_window_get_width (window),
                           -gdk_window_get_height (window));
    }

  ctk_widget_set_parent (widget, CTK_WIDGET (socket_));

  g_signal_emit_by_name (socket_, "plug-added");
}

/*
 * ctk_plug_send_delete_event:
 * @widget: a #CtkWidget
 *
 * Send a GDK_DELETE event to the @widget and destroy it if
 * necessary. Internal GTK function, called from this file or the
 * backend-specific CtkPlug implementation.
 */
static void
ctk_plug_send_delete_event (CtkWidget *widget)
{
  GdkEvent *event = gdk_event_new (GDK_DELETE);

  event->any.window = g_object_ref (ctk_widget_get_window (widget));
  event->any.send_event = FALSE;

  g_object_ref (widget);

  if (!ctk_widget_event (widget, event))
    ctk_widget_destroy (widget);

  g_object_unref (widget);

  gdk_event_free (event);
}

/**
 * _ctk_plug_remove_from_socket:
 * @plug: a #CtkPlug
 * @socket_: a #CtkSocket
 * 
 * Removes a plug from a socket within the same application.
 **/
void
_ctk_plug_remove_from_socket (CtkPlug   *plug,
			      CtkSocket *socket_)
{
  CtkPlugPrivate *priv;
  CtkWidget *widget;
  GdkWindow *window;
  GdkWindow *root_window;
  gboolean result;
  gboolean widget_was_visible;

  g_return_if_fail (CTK_IS_PLUG (plug));
  g_return_if_fail (CTK_IS_SOCKET (socket_));
  g_return_if_fail (ctk_widget_get_realized (CTK_WIDGET (plug)));

  priv = plug->priv;
  widget = CTK_WIDGET (plug);

  if (_ctk_widget_get_in_reparent (widget))
    return;

  g_object_ref (plug);
  g_object_ref (socket_);

  widget_was_visible = ctk_widget_get_visible (widget);
  window = ctk_widget_get_window (widget);
  root_window = gdk_screen_get_root_window (ctk_widget_get_screen (widget));

  gdk_window_hide (window);
  _ctk_widget_set_in_reparent (widget, TRUE);
  gdk_window_reparent (window, root_window, 0, 0);
  ctk_widget_unparent (CTK_WIDGET (plug));
  _ctk_widget_set_in_reparent (widget, FALSE);
  
  socket_->priv->plug_widget = NULL;
  if (socket_->priv->plug_window != NULL)
    {
      g_object_unref (socket_->priv->plug_window);
      socket_->priv->plug_window = NULL;
    }
  
  socket_->priv->same_app = FALSE;

  priv->same_app = FALSE;
  if (priv->socket_window != NULL)
    {
      g_object_unref (priv->socket_window);
      priv->socket_window = NULL;
    }
  ctk_plug_set_is_child (plug, FALSE);

  g_signal_emit_by_name (socket_, "plug-removed", &result);
  if (!result)
    ctk_widget_destroy (CTK_WIDGET (socket_));

  if (window)
    ctk_plug_send_delete_event (widget);

  g_object_unref (plug);

  if (widget_was_visible && ctk_widget_get_visible (CTK_WIDGET (socket_)))
    ctk_widget_queue_resize (CTK_WIDGET (socket_));

  g_object_unref (socket_);
}

/**
 * ctk_plug_construct:
 * @plug: a #CtkPlug.
 * @socket_id: the XID of the socket’s window.
 *
 * Finish the initialization of @plug for a given #CtkSocket identified by
 * @socket_id. This function will generally only be used by classes deriving from #CtkPlug.
 **/
void
ctk_plug_construct (CtkPlug *plug,
		    Window   socket_id)
{
  ctk_plug_construct_for_display (plug, gdk_display_get_default (), socket_id);
}

/**
 * ctk_plug_construct_for_display:
 * @plug: a #CtkPlug.
 * @display: the #GdkDisplay associated with @socket_id’s 
 *	     #CtkSocket.
 * @socket_id: the XID of the socket’s window.
 *
 * Finish the initialization of @plug for a given #CtkSocket identified by
 * @socket_id which is currently displayed on @display.
 * This function will generally only be used by classes deriving from #CtkPlug.
 *
 * Since: 2.2
 **/
void
ctk_plug_construct_for_display (CtkPlug    *plug,
				GdkDisplay *display,
				Window      socket_id)
{
  CtkPlugPrivate *priv;

  g_return_if_fail (CTK_IS_PLUG (plug));
  g_return_if_fail (GDK_IS_DISPLAY (display));

  priv = plug->priv;

  if (socket_id)
    {
      gpointer user_data = NULL;

      if (GDK_IS_X11_DISPLAY (display))
        priv->socket_window = gdk_x11_window_lookup_for_display (display, socket_id);
      else
        priv->socket_window = NULL;

      if (priv->socket_window)
	{
	  gdk_window_get_user_data (priv->socket_window, &user_data);

	  if (user_data)
	    {
	      if (CTK_IS_SOCKET (user_data))
		_ctk_plug_add_to_socket (plug, user_data);
	      else
		{
		  g_warning (G_STRLOC "Can't create CtkPlug as child of non-CtkSocket");
		  priv->socket_window = NULL;
		}
	    }
	  else
	    g_object_ref (priv->socket_window);
	}
      else if (GDK_IS_X11_DISPLAY (display))
        priv->socket_window = gdk_x11_window_foreign_new_for_display (display, socket_id);

      if (priv->socket_window) {
	g_signal_emit (plug, plug_signals[EMBEDDED], 0);

        g_object_notify (G_OBJECT (plug), "embedded");
      }
    }
}

/**
 * ctk_plug_new:
 * @socket_id:  the window ID of the socket, or 0.
 * 
 * Creates a new plug widget inside the #CtkSocket identified
 * by @socket_id. If @socket_id is 0, the plug is left “unplugged” and
 * can later be plugged into a #CtkSocket by  ctk_socket_add_id().
 * 
 * Returns: the new #CtkPlug widget.
 **/
CtkWidget*
ctk_plug_new (Window socket_id)
{
  return ctk_plug_new_for_display (gdk_display_get_default (), socket_id);
}

/**
 * ctk_plug_new_for_display:
 * @display: the #GdkDisplay on which @socket_id is displayed
 * @socket_id: the XID of the socket’s window.
 * 
 * Create a new plug widget inside the #CtkSocket identified by socket_id.
 *
 * Returns: the new #CtkPlug widget.
 *
 * Since: 2.2
 */
CtkWidget*
ctk_plug_new_for_display (GdkDisplay *display,
			  Window      socket_id)
{
  CtkPlug *plug;

  g_return_val_if_fail (GDK_IS_DISPLAY (display), NULL);

  plug = g_object_new (CTK_TYPE_PLUG, NULL);
  ctk_plug_construct_for_display (plug, display, socket_id);
  return CTK_WIDGET (plug);
}

static void
ctk_plug_finalize (GObject *object)
{
  CtkPlug *plug = CTK_PLUG (object);
  CtkPlugPrivate *priv = plug->priv;

  if (priv->grabbed_keys)
    g_hash_table_destroy (priv->grabbed_keys);

  G_OBJECT_CLASS (ctk_plug_parent_class)->finalize (object);
}

static void
ctk_plug_unrealize (CtkWidget *widget)
{
  CtkPlug *plug = CTK_PLUG (widget);
  CtkPlugPrivate *priv = plug->priv;

  if (priv->socket_window != NULL)
    {
      g_object_unref (priv->socket_window);
      priv->socket_window = NULL;

      g_object_notify (G_OBJECT (widget), "embedded");
    }

  if (!priv->same_app)
    {
      if (priv->modality_window)
	ctk_plug_handle_modality_off (plug);

      ctk_window_group_remove_window (priv->modality_group, CTK_WINDOW (plug));
      g_object_unref (priv->modality_group);
    }

  CTK_WIDGET_CLASS (ctk_plug_parent_class)->unrealize (widget);
}

static void
xembed_set_info (GdkWindow     *window,
		 unsigned long  flags)
{
  GdkDisplay *display = gdk_window_get_display (window);
  unsigned long buffer[2];

  Atom xembed_info_atom = gdk_x11_get_xatom_by_name_for_display (display, "_XEMBED_INFO");

  buffer[0] = CTK_XEMBED_PROTOCOL_VERSION;
  buffer[1] = flags;

  XChangeProperty (GDK_DISPLAY_XDISPLAY (display),
		   GDK_WINDOW_XID (window),
		   xembed_info_atom, xembed_info_atom, 32,
		   PropModeReplace,
		   (unsigned char *)buffer, 2);
}

#ifdef CTK_HAVE_ATK_PLUG_SET_CHILD
static void
_ctk_plug_accessible_embed_set_info (CtkWidget *widget, GdkWindow *window)
{
  GdkDisplay *display = gdk_window_get_display (window);
  gchar *buffer = ctk_plug_accessible_get_id (CTK_PLUG_ACCESSIBLE (ctk_widget_get_accessible (widget)));
  Atom net_at_spi_path_atom = gdk_x11_get_xatom_by_name_for_display (display, "_XEMBED_AT_SPI_PATH");

  if (!buffer)
    return;

  XChangeProperty (GDK_DISPLAY_XDISPLAY (display),
		   GDK_WINDOW_XID (window),
		   net_at_spi_path_atom, net_at_spi_path_atom, 8,
		   PropModeReplace,
		   (unsigned char *)buffer, strlen(buffer));
  g_free (buffer);
}
#endif /* CTK_HAVE_ATK_PLUG_SET_CHILD */

/**
 * ctk_plug_focus_first_last:
 * @plug: a #CtkPlug
 * @direction: a direction
 *
 * Called from the CtkPlug backend when the corresponding socket has
 * told the plug that it has received the focus.
 */
static void
ctk_plug_focus_first_last (CtkPlug          *plug,
			   CtkDirectionType  direction)
{
  CtkWindow *window = CTK_WINDOW (plug);
  CtkWidget *focus_widget;
  CtkWidget *parent;

  focus_widget = ctk_window_get_focus (window);
  if (focus_widget)
    {
      parent = ctk_widget_get_parent (focus_widget);
      while (parent)
	{
	  ctk_container_set_focus_child (CTK_CONTAINER (parent), NULL);
	  parent = ctk_widget_get_parent (parent);
	}
      
      ctk_window_set_focus (CTK_WINDOW (plug), NULL);
    }

  ctk_widget_child_focus (CTK_WIDGET (plug), direction);
}

static void
handle_xembed_message (CtkPlug           *plug,
		       XEmbedMessageType  message,
		       glong              detail,
		       glong              data1,
		       glong              data2,
		       guint32            time)
{
  CtkWindow *window = CTK_WINDOW (plug);

  CTK_NOTE (PLUGSOCKET,
	    g_message ("CtkPlug: %s received", _ctk_xembed_message_name (message)));
  
  switch (message)
    {
    case XEMBED_EMBEDDED_NOTIFY:
      break;
    case XEMBED_WINDOW_ACTIVATE:
      _ctk_window_set_is_active (window, TRUE);
      break;
    case XEMBED_WINDOW_DEACTIVATE:
      _ctk_window_set_is_active (window, FALSE);
      break;
      
    case XEMBED_MODALITY_ON:
      ctk_plug_handle_modality_on (plug);
      break;
    case XEMBED_MODALITY_OFF:
      ctk_plug_handle_modality_off (plug);
      break;

    case XEMBED_FOCUS_IN:
      _ctk_window_set_has_toplevel_focus (window, TRUE);
      switch (detail)
	{
	case XEMBED_FOCUS_FIRST:
	  ctk_plug_focus_first_last (plug, CTK_DIR_TAB_FORWARD);
	  break;
	case XEMBED_FOCUS_LAST:
	  ctk_plug_focus_first_last (plug, CTK_DIR_TAB_BACKWARD);
	  break;
	case XEMBED_FOCUS_CURRENT:
	  break;
	}
      break;

    case XEMBED_FOCUS_OUT:
      _ctk_window_set_has_toplevel_focus (window, FALSE);
      break;
      
    case XEMBED_GRAB_KEY:
    case XEMBED_UNGRAB_KEY:
    case XEMBED_CTK_GRAB_KEY:
    case XEMBED_CTK_UNGRAB_KEY:
    case XEMBED_REQUEST_FOCUS:
    case XEMBED_FOCUS_NEXT:
    case XEMBED_FOCUS_PREV:
      g_warning ("CtkPlug: Invalid _XEMBED message %s received", _ctk_xembed_message_name (message));
      break;
      
    default:
      CTK_NOTE(PLUGSOCKET,
	       g_message ("CtkPlug: Ignoring unknown _XEMBED message of type %d", message));
      break;
    }
}
static GdkFilterReturn
ctk_plug_filter_func (GdkXEvent *gdk_xevent,
		      GdkEvent  *event,
		      gpointer   data)
{
  GdkScreen *screen = gdk_window_get_screen (event->any.window);
  GdkDisplay *display = gdk_screen_get_display (screen);
  CtkPlug *plug = CTK_PLUG (data);
  CtkPlugPrivate *priv = plug->priv;
  XEvent *xevent = (XEvent *)gdk_xevent;
  GHashTableIter iter;
  gpointer key;
  GdkFilterReturn return_val;

  return_val = GDK_FILTER_CONTINUE;

  switch (xevent->type)
    {
    case ClientMessage:
      if (xevent->xclient.message_type == gdk_x11_get_xatom_by_name_for_display (display, "_XEMBED"))
	{
	  _ctk_xembed_push_message (xevent);
	  handle_xembed_message (plug,
				 xevent->xclient.data.l[1],
				 xevent->xclient.data.l[2],
				 xevent->xclient.data.l[3],
				 xevent->xclient.data.l[4],
				 xevent->xclient.data.l[0]);
	  _ctk_xembed_pop_message ();

	  return_val = GDK_FILTER_REMOVE;
	}
      else if (xevent->xclient.message_type == gdk_x11_get_xatom_by_name_for_display (display, "WM_DELETE_WINDOW"))
	{
	  /* We filter these out because we take being reparented back to the
	   * root window as the reliable end of the embedding protocol
	   */

	  return_val = GDK_FILTER_REMOVE;
	}
      break;
    case ReparentNotify:
      {
	XReparentEvent *xre = &xevent->xreparent;
        gboolean was_embedded = priv->socket_window != NULL;

	CTK_NOTE (PLUGSOCKET, g_message("CtkPlug: ReparentNotify received"));

	return_val = GDK_FILTER_REMOVE;
	
	g_object_ref (plug);
	
	if (was_embedded)
	  {
	    /* End of embedding protocol for previous socket */
	    
	    CTK_NOTE (PLUGSOCKET, g_message ("CtkPlug: end of embedding"));
	    /* FIXME: race if we remove from another socket and
	     * then add to a local window before we get notification
	     * Probably need check in _ctk_plug_add_to_socket
	     */

            if (xre->parent != GDK_WINDOW_XID (priv->socket_window))
	      {
		CtkWidget *widget = CTK_WIDGET (plug);

		g_object_unref (priv->socket_window);
		priv->socket_window = NULL;

		/* Emit a delete window, as if the user attempted
		 * to close the toplevel. Simple as to how we
		 * handle WM_DELETE_WINDOW, if it isn't handled
		 * we destroy the widget. BUt only do this if
		 * we are being reparented to the root window.
		 * Moving from one embedder to another should
		 * be invisible to the app.
		 */

		if (xre->parent == GDK_WINDOW_XID (gdk_screen_get_root_window (screen)))
		  {
		    CTK_NOTE (PLUGSOCKET, g_message ("CtkPlug: calling ctk_plug_send_delete_event()"));
		    ctk_plug_send_delete_event (widget);

		    g_object_notify (G_OBJECT (plug), "embedded");
		  }
	      }
	    else
	      goto done;
	  }

	if (xre->parent != GDK_WINDOW_XID (gdk_screen_get_root_window (screen)))
	  {
	    /* Start of embedding protocol */

	    CTK_NOTE (PLUGSOCKET, g_message ("CtkPlug: start of embedding"));

            priv->socket_window = gdk_x11_window_lookup_for_display (display, xre->parent);
            if (priv->socket_window)
	      {
		gpointer user_data = NULL;
		gdk_window_get_user_data (priv->socket_window, &user_data);

		if (user_data)
		  {
		    g_warning (G_STRLOC "Plug reparented unexpectedly into window in the same process");
                    priv->socket_window = NULL;
		    break; /* FIXME: shouldn't this unref the plug? i.e. "goto done;" instead */
		  }

		g_object_ref (priv->socket_window);
	      }
	    else
	      {
		priv->socket_window = gdk_x11_window_foreign_new_for_display (display, xre->parent);
		if (!priv->socket_window) /* Already gone */
		  break; /* FIXME: shouldn't this unref the plug? i.e. "goto done;" instead */
	      }

            if (priv->grabbed_keys)
              {
                g_hash_table_iter_init (&iter, priv->grabbed_keys);
                while (g_hash_table_iter_next (&iter, &key, NULL))
                  {
                    GrabbedKey *grabbed_key = key;

                    _ctk_xembed_send_message (priv->socket_window, XEMBED_CTK_GRAB_KEY, 0,
                                              grabbed_key->accelerator_key,
                                              grabbed_key->accelerator_mods);
                  }
              }

	    if (!was_embedded)
	      g_signal_emit_by_name (plug, "embedded");

	    g_object_notify (G_OBJECT (plug), "embedded");
	  }

      done:
	g_object_unref (plug);
	
	break;
      }
    case KeyPress:
    case KeyRelease:
      {
        GdkModifierType state, consumed;
        GdkDevice *keyboard;
        GdkKeymap *keymap;
        GdkSeat *seat;

        if (xevent->type == KeyPress)
          event->key.type = GDK_KEY_PRESS;
        else
          event->key.type = GDK_KEY_RELEASE;

        event->key.window = gdk_x11_window_lookup_for_display (display, xevent->xany.window);
        event->key.send_event = TRUE;
        event->key.time = xevent->xkey.time;
        event->key.state = (GdkModifierType) xevent->xkey.state;
        event->key.hardware_keycode = xevent->xkey.keycode;
        event->key.keyval = GDK_KEY_VoidSymbol;

        seat = gdk_display_get_default_seat (display);
        keyboard = gdk_seat_get_keyboard (seat);
        gdk_event_set_device (event, keyboard);

        keymap = gdk_keymap_get_for_display (display);

        event->key.group = gdk_x11_keymap_get_group_for_state (keymap, xevent->xkey.state);
        event->key.is_modifier = gdk_x11_keymap_key_is_modifier (keymap, event->key.hardware_keycode);

        gdk_keymap_translate_keyboard_state (keymap,
                                             event->key.hardware_keycode,
                                             event->key.state,
                                             event->key.group,
                                             &event->key.keyval,
                                             NULL, NULL, &consumed);

        state = event->key.state & ~consumed;
        gdk_keymap_add_virtual_modifiers (keymap, &state);
        event->key.state |= state;

        event->key.length = 0;
        event->key.string = g_strdup ("");

        return_val = GDK_FILTER_TRANSLATE;
      }
    }

  return return_val;
}
static void
ctk_plug_realize (CtkWidget *widget)
{
  CtkAllocation allocation;
  CtkPlug *plug = CTK_PLUG (widget);
  CtkPlugPrivate *priv = plug->priv;
  CtkWindow *window = CTK_WINDOW (widget);
  GdkWindow *gdk_window;
  GdkWindowAttr attributes;
  const gchar *title;
  gchar *wmclass_name, *wmclass_class;
  gint attributes_mask;
  GdkScreen *screen;

  ctk_widget_set_realized (widget, TRUE);

  screen = ctk_widget_get_screen (widget);
  if (!GDK_IS_X11_SCREEN (screen))
    g_warning ("CtkPlug only works under X11");

  title = ctk_window_get_title (window);
  _ctk_window_get_wmclass (window, &wmclass_name, &wmclass_class);
  ctk_widget_get_allocation (widget, &allocation);

  attributes.window_type = GDK_WINDOW_CHILD;	/* XXX GDK_WINDOW_PLUG ? */
  attributes.title = (gchar *) title;
  attributes.wmclass_name = wmclass_name;
  attributes.wmclass_class = wmclass_class;
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
			    GDK_STRUCTURE_MASK);

  attributes_mask = GDK_WA_VISUAL;
  attributes_mask |= (title ? GDK_WA_TITLE : 0);
  attributes_mask |= (wmclass_name ? GDK_WA_WMCLASS : 0);

  if (ctk_widget_is_toplevel (widget))
    {
      GdkDisplay *display = ctk_widget_get_display (widget);
      GdkWindow *root_window;
      attributes.window_type = GDK_WINDOW_TOPLEVEL;

      root_window = gdk_screen_get_root_window (screen);

      gdk_x11_display_error_trap_push (display);
      if (priv->socket_window)
        gdk_window = gdk_window_new (priv->socket_window,
                                     &attributes, attributes_mask);
      else /* If it's a passive plug, we use the root window */
        gdk_window = gdk_window_new (root_window,
                                     &attributes, attributes_mask);
      /* Because the window isn't known to the window manager,
       * frame sync won't work. In theory, XEMBED could be extended
       * so that embedder did frame sync like a window manager, but
       * it's just not worth the effort considering the current
       * minimal use of XEMBED.
       */
      gdk_x11_window_set_frame_sync_enabled (gdk_window, FALSE);
      ctk_widget_set_window (widget, gdk_window);

      gdk_display_sync (ctk_widget_get_display (widget));
      if (gdk_x11_display_error_trap_pop (display)) /* Uh-oh */
	{
	  gdk_x11_display_error_trap_push (display);
	  gdk_window_destroy (gdk_window);
	  gdk_x11_display_error_trap_pop_ignored (display);
	  gdk_window = gdk_window_new (root_window,
                                       &attributes, attributes_mask);
          ctk_widget_set_window (widget, gdk_window);
	}

      gdk_window_add_filter (gdk_window,
			     ctk_plug_filter_func,
			     widget);

      priv->modality_group = ctk_window_group_new ();
      ctk_window_group_add_window (priv->modality_group, window);

      xembed_set_info (ctk_widget_get_window (CTK_WIDGET (plug)), 0);
    }
  else
    {
      gdk_window = gdk_window_new (ctk_widget_get_parent_window (widget),
                                   &attributes, attributes_mask);
      ctk_widget_set_window (widget, gdk_window);
    }

  ctk_widget_register_window (widget, gdk_window);

#ifdef CTK_HAVE_ATK_PLUG_SET_CHILD
  _ctk_plug_accessible_embed_set_info (widget, gdk_window);
#endif /* CTK_HAVE_ATK_PLUG_SET_CHILD */
}

static void
ctk_plug_show (CtkWidget *widget)
{
  if (ctk_widget_is_toplevel (widget))
    CTK_WIDGET_CLASS (ctk_plug_parent_class)->show (widget);
  else
    CTK_WIDGET_CLASS (bin_class)->show (widget);
}

static void
ctk_plug_hide (CtkWidget *widget)
{
  if (ctk_widget_is_toplevel (widget))
    CTK_WIDGET_CLASS (ctk_plug_parent_class)->hide (widget);
  else
    CTK_WIDGET_CLASS (bin_class)->hide (widget);
}

/* From gdkprivate.h */
void gdk_synthesize_window_state (GdkWindow     *window,
                                  GdkWindowState unset_flags,
                                  GdkWindowState set_flags);

static void
ctk_plug_map (CtkWidget *widget)
{
  if (ctk_widget_is_toplevel (widget))
    {
      CtkBin *bin = CTK_BIN (widget);
      CtkPlug *plug = CTK_PLUG (widget);
      CtkWidget *child;

      ctk_widget_set_mapped (widget, TRUE);

      child = ctk_bin_get_child (bin);
      if (child != NULL &&
          ctk_widget_get_visible (child) &&
          !ctk_widget_get_mapped (child))
        ctk_widget_map (child);

      xembed_set_info (ctk_widget_get_window (CTK_WIDGET (plug)), XEMBED_MAPPED);

      gdk_synthesize_window_state (ctk_widget_get_window (widget),
				   GDK_WINDOW_STATE_WITHDRAWN,
				   0);
    }
  else
    CTK_WIDGET_CLASS (bin_class)->map (widget);
}

static void
ctk_plug_unmap (CtkWidget *widget)
{
  if (ctk_widget_is_toplevel (widget))
    {
      CtkPlug *plug = CTK_PLUG (widget);
      GdkWindow *window;
      CtkWidget *child;

      window = ctk_widget_get_window (widget);

      ctk_widget_set_mapped (widget, FALSE);

      gdk_window_hide (window);

      child = ctk_bin_get_child (CTK_BIN (widget));
      if (child != NULL)
        ctk_widget_unmap (child);

      xembed_set_info (ctk_widget_get_window (CTK_WIDGET (plug)), 0);

      gdk_synthesize_window_state (window,
				   0,
				   GDK_WINDOW_STATE_WITHDRAWN);
    }
  else
    CTK_WIDGET_CLASS (bin_class)->unmap (widget);
}

static gboolean
ctk_plug_key_press_event (CtkWidget   *widget,
			  GdkEventKey *event)
{
  if (ctk_widget_is_toplevel (widget))
    return CTK_WIDGET_CLASS (ctk_plug_parent_class)->key_press_event (widget, event);
  else
    return FALSE;
}

static gboolean
ctk_plug_focus_event (CtkWidget      *widget,
		      GdkEventFocus  *event)
{
  /* We eat focus-in events and focus-out events, since they
   * can be generated by something like a keyboard grab on
   * a child of the plug.
   */
  return FALSE;
}

static void
ctk_plug_set_focus (CtkWindow *window,
		    CtkWidget *focus)
{
  CtkPlug *plug = CTK_PLUG (window);
  CtkPlugPrivate *priv = plug->priv;

  CTK_WINDOW_CLASS (ctk_plug_parent_class)->set_focus (window, focus);
  
  /* Ask for focus from embedder
   */

  if (focus && !ctk_window_has_toplevel_focus (window))
    _ctk_xembed_send_message (priv->socket_window,
                              XEMBED_REQUEST_FOCUS, 0, 0, 0);
}

static guint
grabbed_key_hash (gconstpointer a)
{
  const GrabbedKey *key = a;
  guint h;
  
  h = key->accelerator_key << 16;
  h ^= key->accelerator_key >> 16;
  h ^= key->accelerator_mods;

  return h;
}

static gboolean
grabbed_key_equal (gconstpointer a, gconstpointer b)
{
  const GrabbedKey *keya = a;
  const GrabbedKey *keyb = b;

  return (keya->accelerator_key == keyb->accelerator_key &&
	  keya->accelerator_mods == keyb->accelerator_mods);
}

static void
add_grabbed_key (gpointer key, gpointer val, gpointer data)
{
  GrabbedKey *grabbed_key = key;
  CtkPlug *plug = data;
  CtkPlugPrivate *priv = plug->priv;

  if (!priv->grabbed_keys ||
      !g_hash_table_lookup (priv->grabbed_keys, grabbed_key))
    {
      _ctk_xembed_send_message (priv->socket_window, XEMBED_CTK_GRAB_KEY, 0,
                                grabbed_key->accelerator_key,
                                grabbed_key->accelerator_mods);
    }
}

static void
remove_grabbed_key (gpointer key, gpointer val, gpointer data)
{
  GrabbedKey *grabbed_key = key;
  CtkPlug *plug = data;
  CtkPlugPrivate *priv = plug->priv;

  if (!priv->grabbed_keys ||
      !g_hash_table_lookup (priv->grabbed_keys, grabbed_key))
    {
      _ctk_xembed_send_message (priv->socket_window, XEMBED_CTK_UNGRAB_KEY, 0,
                                grabbed_key->accelerator_key,
                                grabbed_key->accelerator_mods);
    }
}

static void
keys_foreach (CtkWindow      *window,
	      guint           keyval,
	      GdkModifierType modifiers,
	      gboolean        is_mnemonic,
	      gpointer        data)
{
  GHashTable *new_grabbed_keys = data;
  GrabbedKey *key = g_slice_new (GrabbedKey);

  key->accelerator_key = keyval;
  key->accelerator_mods = modifiers;
  
  g_hash_table_replace (new_grabbed_keys, key, key);
}

static void
grabbed_key_free (gpointer data)
{
  g_slice_free (GrabbedKey, data);
}

static void
ctk_plug_keys_changed (CtkWindow *window)
{
  GHashTable *new_grabbed_keys, *old_grabbed_keys;
  CtkPlug *plug = CTK_PLUG (window);
  CtkPlugPrivate *priv = plug->priv;

  new_grabbed_keys = g_hash_table_new_full (grabbed_key_hash, grabbed_key_equal, (GDestroyNotify)grabbed_key_free, NULL);
  _ctk_window_keys_foreach (window, keys_foreach, new_grabbed_keys);

  if (priv->socket_window)
    g_hash_table_foreach (new_grabbed_keys, add_grabbed_key, plug);

  old_grabbed_keys = priv->grabbed_keys;
  priv->grabbed_keys = new_grabbed_keys;

  if (old_grabbed_keys)
    {
      if (priv->socket_window)
        g_hash_table_foreach (old_grabbed_keys, remove_grabbed_key, plug);
      g_hash_table_destroy (old_grabbed_keys);
    }
}

static void
ctk_plug_focus_to_parent (CtkPlug         *plug,
			  CtkDirectionType direction)
{
  CtkPlugPrivate *priv = plug->priv;
  XEmbedMessageType message;
  
  switch (direction)
    {
    case CTK_DIR_UP:
    case CTK_DIR_LEFT:
    case CTK_DIR_TAB_BACKWARD:
      message = XEMBED_FOCUS_PREV;
      break;
    case CTK_DIR_DOWN:
    case CTK_DIR_RIGHT:
    case CTK_DIR_TAB_FORWARD:
      message = XEMBED_FOCUS_NEXT;
      break;
    default:
      g_assert_not_reached ();
      message = XEMBED_FOCUS_PREV;
      break;
    }

  _ctk_xembed_send_focus_message (priv->socket_window, message, 0);
}

static gboolean
ctk_plug_focus (CtkWidget        *widget,
		CtkDirectionType  direction)
{
  CtkBin *bin = CTK_BIN (widget);
  CtkPlug *plug = CTK_PLUG (widget);
  CtkWindow *window = CTK_WINDOW (widget);
  CtkContainer *container = CTK_CONTAINER (widget);
  CtkWidget *child;
  CtkWidget *old_focus_child;
  CtkWidget *parent;

  old_focus_child = ctk_container_get_focus_child (container);
  /* We override CtkWindow's behavior, since we don't want wrapping here.
   */
  if (old_focus_child)
    {
      CtkWidget *focus_widget;

      if (ctk_widget_child_focus (old_focus_child, direction))
	return TRUE;

      focus_widget = ctk_window_get_focus (window);
      if (focus_widget)
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
    }
  else
    {
      /* Try to focus the first widget in the window */
      child = ctk_bin_get_child (bin);
      if (child && ctk_widget_child_focus (child, direction))
        return TRUE;
    }

  if (!ctk_container_get_focus_child (CTK_CONTAINER (window)))
    ctk_plug_focus_to_parent (plug, direction);

  return FALSE;
}

static void
ctk_plug_check_resize (CtkContainer *container)
{
  if (ctk_widget_is_toplevel (CTK_WIDGET (container)))
    CTK_CONTAINER_CLASS (ctk_plug_parent_class)->check_resize (container);
  else
    CTK_CONTAINER_CLASS (bin_class)->check_resize (container);
}

