/* ctktrayicon.c
 * Copyright (C) 2002 Anders Carlsson <andersca@gnu.org>
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
 * This is an implementation of the freedesktop.org “system tray” spec,
 * http://www.freedesktop.org/wiki/Standards/systemtray-spec
 */

#include "config.h"

#include <math.h>
#include <string.h>

#include "x11/gdkx.h"
#include <X11/Xatom.h>
#include <cairo-xlib.h>

#include "ctkintl.h"
#include "ctkprivate.h"
#include "ctktrayicon.h"
#include "ctktestutils.h"
#include "ctkdebug.h"
#include "ctktypebuiltins.h"
#include "ctkrender.h"

#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2

#define SYSTEM_TRAY_ORIENTATION_HORZ 0
#define SYSTEM_TRAY_ORIENTATION_VERT 1

enum {
  PROP_0,
  PROP_ORIENTATION,
  PROP_FG_COLOR,
  PROP_ERROR_COLOR,
  PROP_WARNING_COLOR,
  PROP_SUCCESS_COLOR,
  PROP_PADDING,
  PROP_ICON_SIZE
};

struct _CtkTrayIconPrivate
{
  guint stamp;
  
  Atom selection_atom;
  Atom manager_atom;
  Atom system_tray_opcode_atom;
  Atom orientation_atom;
  Atom visual_atom;
  Atom colors_atom;
  Atom padding_atom;
  Atom icon_size_atom;
  Window manager_window;
  GdkVisual *manager_visual;
  gboolean manager_visual_rgba;

  CtkOrientation orientation;
  GdkRGBA fg_color;
  GdkRGBA error_color;
  GdkRGBA warning_color;
  GdkRGBA success_color;
  gint padding;
  gint icon_size;
};

static void ctk_tray_icon_constructed   (GObject     *object);
static void ctk_tray_icon_dispose       (GObject     *object);

static void ctk_tray_icon_get_property  (GObject     *object,
				 	 guint        prop_id,
					 GValue      *value,
					 GParamSpec  *pspec);

static void     ctk_tray_icon_realize   (CtkWidget   *widget);
static void     ctk_tray_icon_style_updated (CtkWidget   *widget);
static gboolean ctk_tray_icon_delete    (CtkWidget   *widget,
					 GdkEventAny *event);
static gboolean ctk_tray_icon_draw      (CtkWidget   *widget,
                                         cairo_t     *cr);

static void ctk_tray_icon_clear_manager_window     (CtkTrayIcon *icon);
static void ctk_tray_icon_update_manager_window    (CtkTrayIcon *icon);
static void ctk_tray_icon_manager_window_destroyed (CtkTrayIcon *icon);

static GdkFilterReturn ctk_tray_icon_manager_filter (GdkXEvent *xevent,
						     GdkEvent  *event,
						     gpointer   user_data);


G_DEFINE_TYPE_WITH_PRIVATE (CtkTrayIcon, ctk_tray_icon, CTK_TYPE_PLUG)

static void
ctk_tray_icon_class_init (CtkTrayIconClass *class)
{
  GObjectClass *gobject_class = (GObjectClass *)class;
  CtkWidgetClass *widget_class = (CtkWidgetClass *)class;

  gobject_class->get_property = ctk_tray_icon_get_property;
  gobject_class->constructed = ctk_tray_icon_constructed;
  gobject_class->dispose = ctk_tray_icon_dispose;

  widget_class->realize = ctk_tray_icon_realize;
  widget_class->style_updated = ctk_tray_icon_style_updated;
  widget_class->delete_event = ctk_tray_icon_delete;
  widget_class->draw = ctk_tray_icon_draw;

  g_object_class_install_property (gobject_class,
				   PROP_ORIENTATION,
				   g_param_spec_enum ("orientation",
						      P_("Orientation"),
						      P_("The orientation of the tray"),
						      CTK_TYPE_ORIENTATION,
						      CTK_ORIENTATION_HORIZONTAL,
						      CTK_PARAM_READABLE));

  g_object_class_install_property (gobject_class,
                                   PROP_FG_COLOR,
                                   g_param_spec_boxed ("fg-color",
                                                       P_("Foreground color"),
                                                       P_("Foreground color for symbolic icons"),
                                                       GDK_TYPE_RGBA,
                                                       CTK_PARAM_READABLE));

  g_object_class_install_property (gobject_class,
                                   PROP_ERROR_COLOR,
                                   g_param_spec_boxed ("error-color",
                                                       P_("Error color"),
                                                       P_("Error color for symbolic icons"),
                                                       GDK_TYPE_RGBA,
                                                       CTK_PARAM_READABLE));

  g_object_class_install_property (gobject_class,
                                   PROP_WARNING_COLOR,
                                   g_param_spec_boxed ("warning-color",
                                                       P_("Warning color"),
                                                       P_("Warning color for symbolic icons"),
                                                       GDK_TYPE_RGBA,
                                                       CTK_PARAM_READABLE));

  g_object_class_install_property (gobject_class,
                                   PROP_SUCCESS_COLOR,
                                   g_param_spec_boxed ("success-color",
                                                       P_("Success color"),
                                                       P_("Success color for symbolic icons"),
                                                       GDK_TYPE_RGBA,
                                                       CTK_PARAM_READABLE));

  g_object_class_install_property (gobject_class,
				   PROP_PADDING,
				   g_param_spec_int ("padding",
						     P_("Padding"),
						     P_("Padding that should be put around icons in the tray"),
						     0,
                                                     G_MAXINT,
                                                     0,
						     CTK_PARAM_READABLE));

  g_object_class_install_property (gobject_class,
				   PROP_ICON_SIZE,
				   g_param_spec_int ("icon-size",
						     P_("Icon Size"),
						     P_("The pixel size that icons should be forced to, or zero"),
						     0,
                                                     G_MAXINT,
                                                     0,
						     CTK_PARAM_READABLE));
}

static void
ctk_tray_icon_init (CtkTrayIcon *icon)
{
  icon->priv = ctk_tray_icon_get_instance_private (icon);
  icon->priv->stamp = 1;
  icon->priv->orientation = CTK_ORIENTATION_HORIZONTAL;
  icon->priv->fg_color.red        = 0.0;
  icon->priv->fg_color.green      = 0.0;
  icon->priv->fg_color.blue       = 0.0;
  icon->priv->fg_color.alpha      = 1.0;
  icon->priv->error_color.red     = 0.7968;
  icon->priv->error_color.green   = 0.0;
  icon->priv->error_color.blue    = 0.0;
  icon->priv->error_color.alpha   = 1.0;
  icon->priv->warning_color.red   = 0.9570;
  icon->priv->warning_color.green = 0.4726;
  icon->priv->warning_color.blue  = 0.2421;
  icon->priv->warning_color.alpha = 1.0;
  icon->priv->success_color.red   = 0.3047;
  icon->priv->success_color.green = 0.6016;
  icon->priv->success_color.blue  = 0.0234;
  icon->priv->success_color.alpha = 1.0;
  icon->priv->padding = 0;
  icon->priv->icon_size = 0;

  ctk_widget_set_app_paintable (CTK_WIDGET (icon), TRUE);
  ctk_widget_add_events (CTK_WIDGET (icon), GDK_PROPERTY_CHANGE_MASK);
}

static void
ctk_tray_icon_constructed (GObject *object)
{
  /* Do setup that depends on the screen; screen has been set at this point */

  CtkTrayIcon *icon = CTK_TRAY_ICON (object);
  GdkScreen *screen = ctk_widget_get_screen (CTK_WIDGET (object));
  GdkWindow *root_window = gdk_screen_get_root_window (screen);
  GdkDisplay *display = ctk_widget_get_display (CTK_WIDGET (object));
  Display *xdisplay = gdk_x11_display_get_xdisplay (display);
  char buffer[256];
  
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  g_snprintf (buffer, sizeof (buffer),
	      "_NET_SYSTEM_TRAY_S%d",
	      gdk_screen_get_number (screen));
G_GNUC_END_IGNORE_DEPRECATIONS

  icon->priv->selection_atom = XInternAtom (xdisplay, buffer, False);
  
  icon->priv->manager_atom = XInternAtom (xdisplay, "MANAGER", False);
  
  icon->priv->system_tray_opcode_atom = XInternAtom (xdisplay,
						     "_NET_SYSTEM_TRAY_OPCODE",
						     False);

  icon->priv->orientation_atom = XInternAtom (xdisplay,
					      "_NET_SYSTEM_TRAY_ORIENTATION",
					      False);

  icon->priv->visual_atom = XInternAtom (xdisplay,
					 "_NET_SYSTEM_TRAY_VISUAL",
					 False);

  icon->priv->colors_atom = XInternAtom (xdisplay,
                                         "_NET_SYSTEM_TRAY_COLORS",
                                         False);

  icon->priv->padding_atom = XInternAtom (xdisplay,
					 "_NET_SYSTEM_TRAY_PADDING",
					 False);

  icon->priv->icon_size_atom = XInternAtom (xdisplay,
                                            "_NET_SYSTEM_TRAY_ICON_SIZE",
                                            False);

  /* Add a root window filter so that we get changes on MANAGER */
  gdk_window_add_filter (root_window,
			 ctk_tray_icon_manager_filter, icon);

  ctk_tray_icon_update_manager_window (icon);
}

static void
ctk_tray_icon_clear_manager_window (CtkTrayIcon *icon)
{
  GdkDisplay *display = ctk_widget_get_display (CTK_WIDGET (icon));

  if (icon->priv->manager_window != None)
    {
      GdkWindow *gdkwin;

      gdkwin = gdk_x11_window_lookup_for_display (display,
                                                  icon->priv->manager_window);

      gdk_window_remove_filter (gdkwin, ctk_tray_icon_manager_filter, icon);

      icon->priv->manager_window = None;
      icon->priv->manager_visual = NULL;
    }
}

static void
ctk_tray_icon_dispose (GObject *object)
{
  CtkTrayIcon *icon = CTK_TRAY_ICON (object);
  CtkWidget *widget = CTK_WIDGET (object);
  GdkWindow *root_window = gdk_screen_get_root_window (ctk_widget_get_screen (widget));

  ctk_tray_icon_clear_manager_window (icon);

  gdk_window_remove_filter (root_window, ctk_tray_icon_manager_filter, icon);

  G_OBJECT_CLASS (ctk_tray_icon_parent_class)->dispose (object);
}

static void
ctk_tray_icon_get_property (GObject    *object,
			    guint       prop_id,
			    GValue     *value,
			    GParamSpec *pspec)
{
  CtkTrayIcon *icon = CTK_TRAY_ICON (object);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, icon->priv->orientation);
      break;
    case PROP_FG_COLOR:
      g_value_set_boxed (value, &icon->priv->fg_color);
      break;
    case PROP_ERROR_COLOR:
      g_value_set_boxed (value, &icon->priv->error_color);
      break;
    case PROP_WARNING_COLOR:
      g_value_set_boxed (value, &icon->priv->warning_color);
      break;
    case PROP_SUCCESS_COLOR:
      g_value_set_boxed (value, &icon->priv->success_color);
      break;
    case PROP_PADDING:
      g_value_set_int (value, icon->priv->padding);
      break;
    case PROP_ICON_SIZE:
      g_value_set_int (value, icon->priv->icon_size);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static gboolean
ctk_tray_icon_draw (CtkWidget *widget, 
		    cairo_t   *cr)
{
  CtkTrayIcon *icon = CTK_TRAY_ICON (widget);
  CtkWidget *focus_child;
  GdkWindow *window;
  gint border_width;
  gboolean retval = FALSE;
  cairo_surface_t *target;

  window = ctk_widget_get_window (widget);
  target = cairo_get_group_target (cr);

  if (icon->priv->manager_visual_rgba ||
      cairo_surface_get_type (target) != CAIRO_SURFACE_TYPE_XLIB ||
      cairo_xlib_surface_get_drawable (target) != GDK_WINDOW_XID (window))
    {
      /* Clear to transparent */
      cairo_set_source_rgba (cr, 0, 0, 0, 0);
      cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
      cairo_paint (cr);
    }
  else
    {
      GdkRectangle clip;

      if (gdk_cairo_get_clip_rectangle (cr, &clip))
        {
          /* Clear to parent-relative pixmap
           * We need to use direct X access here because GDK doesn't know about
           * the parent realtive pixmap. */
          cairo_surface_flush (target);

          XClearArea (GDK_WINDOW_XDISPLAY (window),
                      GDK_WINDOW_XID (window),
                      clip.x, clip.y,
                      clip.width, clip.height,
                      False);
          cairo_surface_mark_dirty_rectangle (target, 
                                              clip.x, clip.y,
                                              clip.width, clip.height);
        }
    }

  if (CTK_WIDGET_CLASS (ctk_tray_icon_parent_class)->draw)
    retval = CTK_WIDGET_CLASS (ctk_tray_icon_parent_class)->draw (widget, cr);

  focus_child = ctk_container_get_focus_child (CTK_CONTAINER (widget));
  if (focus_child && ctk_widget_has_visible_focus (focus_child))
    {
      CtkStyleContext *context;

      border_width = ctk_container_get_border_width (CTK_CONTAINER (widget));
      context = ctk_widget_get_style_context (widget);

      ctk_render_focus (context, cr, border_width, border_width,
                        ctk_widget_get_allocated_width (widget) - 2 * border_width,
                        ctk_widget_get_allocated_height (widget) - 2 * border_width);
    }

  return retval;
}

static void
ctk_tray_icon_get_orientation_property (CtkTrayIcon *icon)
{
  GdkScreen *screen = ctk_widget_get_screen (CTK_WIDGET (icon));
  GdkDisplay *display = gdk_screen_get_display (screen);
  Display *xdisplay = GDK_DISPLAY_XDISPLAY (display);

  Atom type;
  int format;
  union {
	gulong *prop;
	guchar *prop_ch;
  } prop = { NULL };
  gulong nitems;
  gulong bytes_after;
  int error, result;

  g_assert (icon->priv->manager_window != None);
  
  gdk_x11_display_error_trap_push (display);
  type = None;
  result = XGetWindowProperty (xdisplay,
			       icon->priv->manager_window,
			       icon->priv->orientation_atom,
			       0, G_MAXLONG, FALSE,
			       XA_CARDINAL,
			       &type, &format, &nitems,
			       &bytes_after, &(prop.prop_ch));
  error = gdk_x11_display_error_trap_pop (display);

  if (error || result != Success)
    return;

  if (type == XA_CARDINAL && nitems == 1 && format == 32)
    {
      CtkOrientation orientation;

      orientation = (prop.prop [0] == SYSTEM_TRAY_ORIENTATION_HORZ) ?
					CTK_ORIENTATION_HORIZONTAL :
					CTK_ORIENTATION_VERTICAL;

      if (icon->priv->orientation != orientation)
	{
	  icon->priv->orientation = orientation;

	  g_object_notify (G_OBJECT (icon), "orientation");
	}
    }

  if (type != None)
    XFree (prop.prop);
}

static void
ctk_tray_icon_get_visual_property (CtkTrayIcon *icon)
{
  GdkScreen *screen = ctk_widget_get_screen (CTK_WIDGET (icon));
  GdkDisplay *display = gdk_screen_get_display (screen);
  Display *xdisplay = GDK_DISPLAY_XDISPLAY (display);

  Atom type;
  int format;
  union {
	gulong *prop;
	guchar *prop_ch;
  } prop = { NULL };
  gulong nitems;
  gulong bytes_after;
  int error, result;

  g_assert (icon->priv->manager_window != None);

  gdk_x11_display_error_trap_push (display);
  type = None;
  result = XGetWindowProperty (xdisplay,
			       icon->priv->manager_window,
			       icon->priv->visual_atom,
			       0, G_MAXLONG, FALSE,
			       XA_VISUALID,
			       &type, &format, &nitems,
			       &bytes_after, &(prop.prop_ch));
  error = gdk_x11_display_error_trap_pop (display);

  if (!error && result == Success &&
      type == XA_VISUALID && nitems == 1 && format == 32)
    {
      VisualID visual_id;
      GdkVisual *visual;
      gint red_prec, green_prec, blue_prec;

      visual_id = prop.prop[0];
      visual = gdk_x11_screen_lookup_visual (screen, visual_id);
      gdk_visual_get_red_pixel_details (visual, NULL, NULL, &red_prec);
      gdk_visual_get_green_pixel_details (visual, NULL, NULL, &green_prec);
      gdk_visual_get_blue_pixel_details (visual, NULL, NULL, &blue_prec);
      icon->priv->manager_visual = visual;
      icon->priv->manager_visual_rgba =
          (red_prec + blue_prec + green_prec < gdk_visual_get_depth (visual));
    }
  else
    {
      icon->priv->manager_visual = NULL;
      icon->priv->manager_visual_rgba = FALSE;
    }


G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  /* For the background-relative hack we use when we aren't
   * using a real RGBA visual, we can't be double-buffered
   */
  ctk_widget_set_double_buffered (CTK_WIDGET (icon), icon->priv->manager_visual_rgba);
G_GNUC_END_IGNORE_DEPRECATIONS

  if (type != None)
    XFree (prop.prop);
}

static void
ctk_tray_icon_get_colors_property (CtkTrayIcon *icon)
{
  GdkScreen *screen = ctk_widget_get_screen (CTK_WIDGET (icon));
  GdkDisplay *display = gdk_screen_get_display (screen);
  Display *xdisplay = GDK_DISPLAY_XDISPLAY (display);

  Atom type;
  int format;
  union {
        gulong *prop;
        guchar *prop_ch;
  } prop = { NULL };
  gulong nitems;
  gulong bytes_after;
  int error, result;

  g_assert (icon->priv->manager_window != None);

  gdk_x11_display_error_trap_push (display);
  type = None;
  result = XGetWindowProperty (xdisplay,
			       icon->priv->manager_window,
			       icon->priv->colors_atom,
			       0, G_MAXLONG, FALSE,
			       XA_CARDINAL,
			       &type, &format, &nitems,
			       &bytes_after, &(prop.prop_ch));
  error = gdk_x11_display_error_trap_pop (display);

  if (error || result != Success)
    return;

  if (type == XA_CARDINAL && nitems == 12 && format == 32)
    {
      GdkRGBA color;

      color.alpha = 1.0;
      g_object_freeze_notify (G_OBJECT (icon));

      color.red = prop.prop[0] / 65535.0;
      color.green = prop.prop[1] / 65535.0;
      color.blue = prop.prop[2] / 65535.0;

      if (!gdk_rgba_equal (&icon->priv->fg_color, &color))
        {
          icon->priv->fg_color = color;

          g_object_notify (G_OBJECT (icon), "fg-color");
        }

      color.red = prop.prop[3] / 65535.0;
      color.green = prop.prop[4] / 65535.0;
      color.blue = prop.prop[5] / 65535.0;

      if (!gdk_rgba_equal (&icon->priv->error_color, &color))
        {
          icon->priv->error_color = color;

          g_object_notify (G_OBJECT (icon), "error-color");
        }

      color.red = prop.prop[6] / 65535.0;
      color.green = prop.prop[7] / 65535.0;
      color.blue = prop.prop[8] / 65535.0;

      if (!gdk_rgba_equal (&icon->priv->warning_color, &color))
        {
          icon->priv->warning_color = color;

          g_object_notify (G_OBJECT (icon), "warning-color");
        }

      color.red = prop.prop[9] / 65535.0;
      color.green = prop.prop[10] /  65535.0;
      color.blue = prop.prop[11] / 65535.0;

      if (!gdk_rgba_equal (&icon->priv->success_color, &color))
        {
          icon->priv->success_color = color;

          g_object_notify (G_OBJECT (icon), "success-color");
        }

      g_object_thaw_notify (G_OBJECT (icon));
    }

  if (type != None)
    XFree (prop.prop);
}

static void
ctk_tray_icon_get_padding_property (CtkTrayIcon *icon)
{
  GdkScreen *screen = ctk_widget_get_screen (CTK_WIDGET (icon));
  GdkDisplay *display = gdk_screen_get_display (screen);
  Display *xdisplay = GDK_DISPLAY_XDISPLAY (display);

  Atom type;
  int format;
  union {
	gulong *prop;
	guchar *prop_ch;
  } prop = { NULL };
  gulong nitems;
  gulong bytes_after;
  int error, result;

  g_assert (icon->priv->manager_window != None);

  gdk_x11_display_error_trap_push (display);
  type = None;
  result = XGetWindowProperty (xdisplay,
			       icon->priv->manager_window,
			       icon->priv->padding_atom,
			       0, G_MAXLONG, FALSE,
			       XA_CARDINAL,
			       &type, &format, &nitems,
			       &bytes_after, &(prop.prop_ch));
  error = gdk_x11_display_error_trap_pop (display);

  if (!error && result == Success &&
      type == XA_CARDINAL && nitems == 1 && format == 32)
    {
      gint padding;

      padding = prop.prop[0];

      if (icon->priv->padding != padding)
	{
	  icon->priv->padding = padding;

	  g_object_notify (G_OBJECT (icon), "padding");
	}
    }

  if (type != None)
    XFree (prop.prop);
}

static void
ctk_tray_icon_get_icon_size_property (CtkTrayIcon *icon)
{
  GdkScreen *screen = ctk_widget_get_screen (CTK_WIDGET (icon));
  GdkDisplay *display = gdk_screen_get_display (screen);
  Display *xdisplay = GDK_DISPLAY_XDISPLAY (display);

  Atom type;
  int format;
  union {
	gulong *prop;
	guchar *prop_ch;
  } prop = { NULL };
  gulong nitems;
  gulong bytes_after;
  int error, result;

  g_assert (icon->priv->manager_window != None);

  gdk_x11_display_error_trap_push (display);
  type = None;
  result = XGetWindowProperty (xdisplay,
			       icon->priv->manager_window,
			       icon->priv->icon_size_atom,
			       0, G_MAXLONG, FALSE,
			       XA_CARDINAL,
			       &type, &format, &nitems,
			       &bytes_after, &(prop.prop_ch));
  error = gdk_x11_display_error_trap_pop (display);

  if (!error && result == Success &&
      type == XA_CARDINAL && nitems == 1 && format == 32)
    {
      gint icon_size;

      icon_size = prop.prop[0];

      if (icon->priv->icon_size != icon_size)
	{
	  icon->priv->icon_size = icon_size;

	  g_object_notify (G_OBJECT (icon), "icon-size");
	}
    }

  if (type != None)
    XFree (prop.prop);
}

static GdkFilterReturn
ctk_tray_icon_manager_filter (GdkXEvent *xevent,
			      GdkEvent  *event,
			      gpointer   user_data)
{
  CtkTrayIcon *icon = user_data;
  XEvent *xev = (XEvent *)xevent;

  if (xev->xany.type == ClientMessage &&
      xev->xclient.message_type == icon->priv->manager_atom &&
      xev->xclient.data.l[1] == icon->priv->selection_atom)
    {
      CTK_NOTE (PLUGSOCKET,
                g_message ("CtkStatusIcon %p: tray manager appeared", icon));

      ctk_tray_icon_update_manager_window (icon);
    }
  else if (xev->xany.window == icon->priv->manager_window)
    {
      if (xev->xany.type == PropertyNotify &&
	  xev->xproperty.atom == icon->priv->orientation_atom)
	{
          CTK_NOTE (PLUGSOCKET,
                    g_message ("CtkStatusIcon %p: got PropertyNotify on manager window for orientation atom", icon));

	  ctk_tray_icon_get_orientation_property (icon);
	}
      else if (xev->xany.type == PropertyNotify &&
               xev->xproperty.atom == icon->priv->colors_atom)
        {
          CTK_NOTE (PLUGSOCKET,
                    g_message ("CtkStatusIcon %p: got PropertyNotify on manager window for colors atom", icon));

          ctk_tray_icon_get_colors_property (icon);
        }
      else if (xev->xany.type == PropertyNotify &&
               xev->xproperty.atom == icon->priv->padding_atom)
        {
          ctk_tray_icon_get_padding_property (icon);
        }
      else if (xev->xany.type == PropertyNotify &&
               xev->xproperty.atom == icon->priv->icon_size_atom)
        {
          ctk_tray_icon_get_icon_size_property (icon);
        }
      else if (xev->xany.type == DestroyNotify)
	{
          CTK_NOTE (PLUGSOCKET,
                    g_message ("CtkStatusIcon %p: got DestroyNotify for manager window", icon));

	  ctk_tray_icon_manager_window_destroyed (icon);
	}
      else
        {
          CTK_NOTE (PLUGSOCKET,
                    g_message ("CtkStatusIcon %p: got other message on manager window", icon));
        }
    }
  
  return GDK_FILTER_CONTINUE;
}

static void
ctk_tray_icon_send_manager_message (CtkTrayIcon *icon,
				    long         message,
				    Window       window,
				    long         data1,
				    long         data2,
				    long         data3)
{
  CtkWidget *widget;
  XClientMessageEvent ev;
  GdkDisplay *display;
  Display *xdisplay;

  widget = CTK_WIDGET (icon);

  memset (&ev, 0, sizeof (ev));
  ev.type = ClientMessage;
  ev.window = window;
  ev.message_type = icon->priv->system_tray_opcode_atom;
  ev.format = 32;
  ev.data.l[0] = gdk_x11_get_server_time (ctk_widget_get_window (widget));
  ev.data.l[1] = message;
  ev.data.l[2] = data1;
  ev.data.l[3] = data2;
  ev.data.l[4] = data3;

  display = ctk_widget_get_display (widget);
  xdisplay = GDK_DISPLAY_XDISPLAY (display);

  gdk_x11_display_error_trap_push (display);
  XSendEvent (xdisplay,
	      icon->priv->manager_window,
              False, NoEventMask,
              (XEvent *)&ev);
  gdk_x11_display_error_trap_pop_ignored (display);
}

static void
ctk_tray_icon_send_dock_request (CtkTrayIcon *icon)
{
  CTK_NOTE (PLUGSOCKET,
            g_message ("CtkStatusIcon %p: sending dock request to manager window %lx",
                       icon, (gulong) icon->priv->manager_window));

  ctk_tray_icon_send_manager_message (icon,
				      SYSTEM_TRAY_REQUEST_DOCK,
				      icon->priv->manager_window,
				      ctk_plug_get_id (CTK_PLUG (icon)),
				      0, 0);
}

static void
ctk_tray_icon_update_manager_window (CtkTrayIcon *icon)
{
  CtkWidget *widget = CTK_WIDGET (icon);
  GdkScreen *screen = ctk_widget_get_screen (widget);
  GdkDisplay *display = gdk_screen_get_display (screen);
  Display *xdisplay = GDK_DISPLAY_XDISPLAY (display);

  CTK_NOTE (PLUGSOCKET,
            g_message ("CtkStatusIcon %p: updating tray icon manager window, current manager window: %lx",
                       icon, (gulong) icon->priv->manager_window));

  if (icon->priv->manager_window != None)
    return;

  CTK_NOTE (PLUGSOCKET,
            g_message ("CtkStatusIcon %p: trying to find manager window", icon));

  XGrabServer (xdisplay);
  
  icon->priv->manager_window = XGetSelectionOwner (xdisplay,
						   icon->priv->selection_atom);

  if (icon->priv->manager_window != None)
    XSelectInput (xdisplay,
		  icon->priv->manager_window, StructureNotifyMask|PropertyChangeMask);

  XUngrabServer (xdisplay);
  XFlush (xdisplay);
  
  if (icon->priv->manager_window != None)
    {
      GdkWindow *gdkwin;

      CTK_NOTE (PLUGSOCKET,
                g_message ("CtkStatusIcon %p: is being managed by window %lx",
                           icon, (gulong) icon->priv->manager_window));

      gdkwin = gdk_x11_window_lookup_for_display (display,
                                                  icon->priv->manager_window);

      gdk_window_add_filter (gdkwin, ctk_tray_icon_manager_filter, icon);

      ctk_tray_icon_get_orientation_property (icon);
      ctk_tray_icon_get_visual_property (icon);
      ctk_tray_icon_get_colors_property (icon);
      ctk_tray_icon_get_padding_property (icon);
      ctk_tray_icon_get_icon_size_property (icon);

      if (ctk_widget_get_realized (CTK_WIDGET (icon)))
	{
	  if ((icon->priv->manager_visual == NULL &&
	       ctk_widget_get_visual (widget) == gdk_screen_get_system_visual (screen)) ||
	      (icon->priv->manager_visual == ctk_widget_get_visual (widget)))
	    {
	      /* Already have the right visual, can just dock
	       */
	      ctk_tray_icon_send_dock_request (icon);
	    }
	  else
	    {
	      /* Need to re-realize the widget to get the right visual
	       */
	      ctk_widget_hide (widget);
	      ctk_widget_unrealize (widget);
	      ctk_widget_show (widget);
	    }
	}
    }
  else
    {
      CTK_NOTE (PLUGSOCKET,
                g_message ("CtkStatusIcon %p: no tray manager found", icon));
    }
}

static void
ctk_tray_icon_manager_window_destroyed (CtkTrayIcon *icon)
{
  g_return_if_fail (icon->priv->manager_window != None);

  CTK_NOTE (PLUGSOCKET,
            g_message ("CtkStatusIcon %p: tray manager window destroyed", icon));

  ctk_tray_icon_clear_manager_window (icon);
}

static gboolean
ctk_tray_icon_delete (CtkWidget   *widget,
		      GdkEventAny *event)
{
  CTK_NOTE (PLUGSOCKET,
            g_message ("CtkStatusIcon %p: delete notify, tray manager window %lx",
                       widget, (gulong) CTK_TRAY_ICON (widget)->priv->manager_window));

  /* A bug in X server versions up to x.org 1.5.0 means that:
   * XFixesChangeSaveSet(...., SaveSetRoot, SaveSetUnmap) doesn't work properly
   * and we'll left mapped in a separate toplevel window if the tray is destroyed.
   * For simplicity just get rid of our X window and start over.
   */
  ctk_widget_hide (widget);
  ctk_widget_unrealize (widget);
  ctk_widget_show (widget);

  /* Handled it, don't destroy the tray icon */
  return TRUE;
}

static void
ctk_tray_icon_set_visual (CtkTrayIcon *icon)
{
  GdkScreen *screen = ctk_widget_get_screen (CTK_WIDGET (icon));
  GdkVisual *visual = icon->priv->manager_visual;

  /* To avoid uncertainty about colormaps, _NET_SYSTEM_TRAY_VISUAL is supposed
   * to be either the screen default visual or a TrueColor visual; ignore it
   * if it is something else
   */
  if (visual && gdk_visual_get_visual_type (visual) != GDK_VISUAL_TRUE_COLOR)
    visual = NULL;

  if (visual == NULL)
    visual = gdk_screen_get_system_visual (screen);

  ctk_widget_set_visual (CTK_WIDGET (icon), visual);
}

static void
ctk_tray_icon_realize (CtkWidget *widget)
{
  CtkTrayIcon *icon = CTK_TRAY_ICON (widget);
  GdkWindow *window;

  /* Set our visual before realizing */
  ctk_tray_icon_set_visual (icon);

  CTK_WIDGET_CLASS (ctk_tray_icon_parent_class)->realize (widget);
  window = ctk_widget_get_window (widget);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  if (icon->priv->manager_visual_rgba)
    {
      /* Set a transparent background */
      GdkRGBA transparent = { 0.0, 0.0, 0.0, 0.0 };
      gdk_window_set_background_rgba (window, &transparent);
    }
  else
    {
      /* Set a parent-relative background pixmap */
      cairo_pattern_t *parent_relative = gdk_x11_get_parent_relative_pattern ();
      gdk_window_set_background_pattern (window, parent_relative);
    }
G_GNUC_END_IGNORE_DEPRECATIONS

  CTK_NOTE (PLUGSOCKET,
            g_message ("CtkStatusIcon %p: realized, window: %lx, socket window: %lx",
                       widget,
                       (gulong) GDK_WINDOW_XID (window),
                       ctk_plug_get_socket_window (CTK_PLUG (icon))
                       ? (gulong) GDK_WINDOW_XID (ctk_plug_get_socket_window (CTK_PLUG (icon)))
                       : 0UL));

  if (icon->priv->manager_window != None)
    ctk_tray_icon_send_dock_request (icon);
}

static void
ctk_tray_icon_style_updated (CtkWidget   *widget)
{
  /* The default handler resets the background according to the style. We either
   * use a transparent background or a parent-relative background and ignore the
   * style background. So, just don't chain up.
   */
}

guint
_ctk_tray_icon_send_message (CtkTrayIcon *icon,
			     gint         timeout,
			     const gchar *message,
			     gint         len)
{
  guint stamp;
  GdkDisplay *display;
  Display *xdisplay;
 
  g_return_val_if_fail (CTK_IS_TRAY_ICON (icon), 0);
  g_return_val_if_fail (timeout >= 0, 0);
  g_return_val_if_fail (message != NULL, 0);

  if (icon->priv->manager_window == None)
    return 0;

  if (len < 0)
    len = strlen (message);

  stamp = icon->priv->stamp++;
  
  /* Get ready to send the message */
  ctk_tray_icon_send_manager_message (icon, SYSTEM_TRAY_BEGIN_MESSAGE,
				      (Window)ctk_plug_get_id (CTK_PLUG (icon)),
				      timeout, len, stamp);

  /* Now to send the actual message */
  display = ctk_widget_get_display (CTK_WIDGET (icon));
  xdisplay = GDK_DISPLAY_XDISPLAY (display);
  gdk_x11_display_error_trap_push (display);
  while (len > 0)
    {
      XClientMessageEvent ev;

      memset (&ev, 0, sizeof (ev));
      ev.type = ClientMessage;
      ev.window = (Window)ctk_plug_get_id (CTK_PLUG (icon));
      ev.format = 8;
      ev.message_type = XInternAtom (xdisplay,
				     "_NET_SYSTEM_TRAY_MESSAGE_DATA", False);
      if (len > 20)
	{
	  memcpy (&ev.data, message, 20);
	  len -= 20;
	  message += 20;
	}
      else
	{
	  memcpy (&ev.data, message, len);
	  len = 0;
	}

      XSendEvent (xdisplay,
		  icon->priv->manager_window, False,
		  StructureNotifyMask, (XEvent *)&ev);
    }
  gdk_x11_display_error_trap_pop_ignored (display);

  return stamp;
}

void
_ctk_tray_icon_cancel_message (CtkTrayIcon *icon,
			       guint        id)
{
  g_return_if_fail (CTK_IS_TRAY_ICON (icon));
  g_return_if_fail (id > 0);
  
  ctk_tray_icon_send_manager_message (icon, SYSTEM_TRAY_CANCEL_MESSAGE,
				      (Window)ctk_plug_get_id (CTK_PLUG (icon)),
				      id, 0, 0);
}

CtkTrayIcon *
_ctk_tray_icon_new_for_screen (GdkScreen  *screen, 
			       const gchar *name)
{
  g_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);

  return g_object_new (CTK_TYPE_TRAY_ICON, 
		       "screen", screen, 
		       "title", name, 
		       NULL);
}

CtkTrayIcon*
_ctk_tray_icon_new (const gchar *name)
{
  return g_object_new (CTK_TYPE_TRAY_ICON, 
		       "title", name, 
		       NULL);
}

CtkOrientation
_ctk_tray_icon_get_orientation (CtkTrayIcon *icon)
{
  g_return_val_if_fail (CTK_IS_TRAY_ICON (icon), CTK_ORIENTATION_HORIZONTAL);

  return icon->priv->orientation;
}

gint
_ctk_tray_icon_get_padding (CtkTrayIcon *icon)
{
  g_return_val_if_fail (CTK_IS_TRAY_ICON (icon), 0);

  return icon->priv->padding;
}

gint
_ctk_tray_icon_get_icon_size (CtkTrayIcon *icon)
{
  g_return_val_if_fail (CTK_IS_TRAY_ICON (icon), 0);

  return icon->priv->icon_size;
}
