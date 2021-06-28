/* ctkstatusicon.c:
 *
 * Copyright (C) 2003 Sun Microsystems, Inc.
 * Copyright (C) 2005 Hans Breuer <hans@breuer.org>
 * Copyright (C) 2005 Novell, Inc.
 * Copyright (C) 2006 Imendio AB
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *	Mark McLoughlin <mark@skynet.ie>
 *	Hans Breuer <hans@breuer.org>
 *	Tor Lillqvist <tml@novell.com>
 *	Mikael Hallendal <micke@imendio.com>
 */

#include "config.h"

#include <string.h>
#include <stdlib.h>

#define CDK_DISABLE_DEPRECATION_WARNINGS
#include "ctkstatusicon.h"

#include "ctkintl.h"
#include "ctkiconhelperprivate.h"
#include "ctkmain.h"
#include "ctkmarshalers.h"
#include "ctksizerequest.h"
#include "ctkprivate.h"
#include "ctkwidget.h"
#include "ctktooltip.h"
#include "ctkicontheme.h"
#include "ctklabel.h"
#include "ctkstylecontextprivate.h"
#include "ctktypebuiltins.h"

#ifdef CDK_WINDOWING_X11
#include "cdk/x11/cdkx.h"
#include "ctktrayicon.h"
#endif

#ifdef CDK_WINDOWING_WIN32
#include "cdk/win32/cdkwin32.h"
#define WM_CTK_TRAY_NOTIFICATION (WM_USER+1)
#endif


/**
 * SECTION:ctkstatusicon
 * @Short_description: Display an icon in the system tray
 * @Title: CtkStatusIcon
 *
 * The “system tray” or notification area is normally used for transient icons
 * that indicate some special state. For example, a system tray icon might
 * appear to tell the user that they have new mail, or have an incoming instant
 * message, or something along those lines. The basic idea is that creating an
 * icon in the notification area is less annoying than popping up a dialog.
 *
 * A #CtkStatusIcon object can be used to display an icon in a “system tray”.
 * The icon can have a tooltip, and the user can interact with it by
 * activating it or popping up a context menu.
 *
 * It is very important to notice that status icons depend on the existence
 * of a notification area being available to the user; you should not use status
 * icons as the only way to convey critical information regarding your application,
 * as the notification area may not exist on the user's environment, or may have
 * been removed. You should always check that a status icon has been embedded into
 * a notification area by using ctk_status_icon_is_embedded(), and gracefully
 * recover if the function returns %FALSE.
 *
 * On X11, the implementation follows the
 * [FreeDesktop System Tray Specification](http://www.freedesktop.org/wiki/Specifications/systemtray-spec).
 * Implementations of the “tray” side of this specification can
 * be found e.g. in the GNOME 2 and KDE panel applications.
 *
 * Note that a CtkStatusIcon is not a widget, but just a #GObject. Making it a
 * widget would be impractical, since the system tray on Windows doesn’t allow
 * to embed arbitrary widgets.
 *
 * CtkStatusIcon has been deprecated in 3.14. You should consider using
 * notifications or more modern platform-specific APIs instead. GLib provides
 * the #GNotification API which works well with #CtkApplication on multiple
 * platforms and environments, and should be the preferred mechanism to notify
 * the users of transient status updates. See this [HowDoI](https://wiki.gnome.org/HowDoI/GNotification)
 * for code examples.
 */


#define BLINK_TIMEOUT 500

enum
{
  PROP_0,
  PROP_PIXBUF,
  PROP_FILE,
  PROP_STOCK,
  PROP_ICON_NAME,
  PROP_GICON,
  PROP_STORAGE_TYPE,
  PROP_SIZE,
  PROP_SCREEN,
  PROP_VISIBLE,
  PROP_ORIENTATION,
  PROP_EMBEDDED,
  PROP_BLINKING,
  PROP_HAS_TOOLTIP,
  PROP_TOOLTIP_TEXT,
  PROP_TOOLTIP_MARKUP,
  PROP_TITLE
};

enum 
{
  ACTIVATE_SIGNAL,
  POPUP_MENU_SIGNAL,
  SIZE_CHANGED_SIGNAL,
  BUTTON_PRESS_EVENT_SIGNAL,
  BUTTON_RELEASE_EVENT_SIGNAL,
  SCROLL_EVENT_SIGNAL,
  QUERY_TOOLTIP_SIGNAL,
  LAST_SIGNAL
};

static guint status_icon_signals [LAST_SIGNAL] = { 0 };

#ifdef CDK_WINDOWING_QUARTZ
#include "ctkstatusicon-quartz.c"
#endif

struct _CtkStatusIconPrivate
{
#ifdef CDK_WINDOWING_X11
  CtkWidget    *tray_icon;
  CtkWidget    *image;
#else
  CtkWidget    *dummy_widget;
#endif

#ifdef CDK_WINDOWING_WIN32
  NOTIFYICONDATAW nid;
  gint          taskbar_top;
  gint		last_click_x, last_click_y;
  CtkOrientation orientation;
  gchar         *tooltip_text;
  gchar         *title;
#endif

#ifdef CDK_WINDOWING_QUARTZ
  CtkQuartzStatusIcon *status_item;
  gchar         *tooltip_text;
  gchar         *title;
#endif

  gint           size;
  CtkImageDefinition *image_def;
  guint          visible : 1;
};

static void     ctk_status_icon_constructed      (GObject        *object);
static void     ctk_status_icon_finalize         (GObject        *object);
static void     ctk_status_icon_set_property     (GObject        *object,
						  guint           prop_id,
						  const GValue   *value,
						  GParamSpec     *pspec);
static void     ctk_status_icon_get_property     (GObject        *object,
						  guint           prop_id,
						  GValue         *value,
					          GParamSpec     *pspec);

#ifdef CDK_WINDOWING_X11
static void     ctk_status_icon_size_allocate    (CtkStatusIcon  *status_icon,
						  CtkAllocation  *allocation);
static void     ctk_status_icon_screen_changed   (CtkStatusIcon  *status_icon,
						  CdkScreen      *old_screen);
static void     ctk_status_icon_embedded_changed (CtkStatusIcon *status_icon);
static void     ctk_status_icon_orientation_changed (CtkStatusIcon *status_icon);
static void     ctk_status_icon_padding_changed  (CtkStatusIcon *status_icon);
static void     ctk_status_icon_icon_size_changed(CtkStatusIcon *status_icon);
static void     ctk_status_icon_fg_changed       (CtkStatusIcon *status_icon);
static void     ctk_status_icon_color_changed    (CtkTrayIcon   *tray,
                                                  GParamSpec    *pspec,
                                                  CtkStatusIcon *status_icon);
static gboolean ctk_status_icon_scroll           (CtkStatusIcon  *status_icon,
						  CdkEventScroll *event);
static gboolean ctk_status_icon_query_tooltip    (CtkStatusIcon *status_icon,
						  gint           x,
						  gint           y,
						  gboolean       keyboard_tip,
						  CtkTooltip    *tooltip);

static gboolean ctk_status_icon_key_press        (CtkStatusIcon  *status_icon,
						  CdkEventKey    *event);
static void     ctk_status_icon_popup_menu       (CtkStatusIcon  *status_icon);
#endif
static gboolean ctk_status_icon_button_press     (CtkStatusIcon  *status_icon,
						  CdkEventButton *event);
static gboolean ctk_status_icon_button_release   (CtkStatusIcon  *status_icon,
						  CdkEventButton *event);
static void     ctk_status_icon_reset_image_data (CtkStatusIcon  *status_icon);
static void     ctk_status_icon_update_image    (CtkStatusIcon *status_icon);

G_DEFINE_TYPE_WITH_PRIVATE (CtkStatusIcon, ctk_status_icon, G_TYPE_OBJECT)

static void
ctk_status_icon_class_init (CtkStatusIconClass *class)
{
  GObjectClass *gobject_class = (GObjectClass *) class;

  gobject_class->constructed  = ctk_status_icon_constructed;
  gobject_class->finalize     = ctk_status_icon_finalize;
  gobject_class->set_property = ctk_status_icon_set_property;
  gobject_class->get_property = ctk_status_icon_get_property;

  class->button_press_event   = NULL;
  class->button_release_event = NULL;
  class->scroll_event         = NULL;
  class->query_tooltip        = NULL;

  g_object_class_install_property (gobject_class,
				   PROP_PIXBUF,
				   g_param_spec_object ("pixbuf",
							P_("Pixbuf"),
							P_("A GdkPixbuf to display"),
							CDK_TYPE_PIXBUF,
							CTK_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
				   PROP_FILE,
				   g_param_spec_string ("file",
							P_("Filename"),
							P_("Filename to load and display"),
							NULL,
							CTK_PARAM_WRITABLE));

  /**
   * CtkStatusIcon:stock:
   *
   * Deprecated: 3.10: Use #CtkStatusIcon:icon-name instead.
   */
  g_object_class_install_property (gobject_class,
				   PROP_STOCK,
				   g_param_spec_string ("stock",
							P_("Stock ID"),
							P_("Stock ID for a stock image to display"),
							NULL,
							CTK_PARAM_READWRITE | G_PARAM_DEPRECATED));
  
  g_object_class_install_property (gobject_class,
                                   PROP_ICON_NAME,
                                   g_param_spec_string ("icon-name",
                                                        P_("Icon Name"),
                                                        P_("The name of the icon from the icon theme"),
                                                        NULL,
                                                        CTK_PARAM_READWRITE));

  /**
   * CtkStatusIcon:gicon:
   *
   * The #GIcon displayed in the #CtkStatusIcon. For themed icons,
   * the image will be updated automatically if the theme changes.
   *
   * Since: 2.14
   */
  g_object_class_install_property (gobject_class,
                                   PROP_GICON,
                                   g_param_spec_object ("gicon",
                                                        P_("GIcon"),
                                                        P_("The GIcon being displayed"),
                                                        G_TYPE_ICON,
                                                        CTK_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
				   PROP_STORAGE_TYPE,
				   g_param_spec_enum ("storage-type",
						      P_("Storage type"),
						      P_("The representation being used for image data"),
						      CTK_TYPE_IMAGE_TYPE,
						      CTK_IMAGE_EMPTY,
						      CTK_PARAM_READABLE));

  g_object_class_install_property (gobject_class,
				   PROP_SIZE,
				   g_param_spec_int ("size",
						     P_("Size"),
						     P_("The size of the icon"),
						     0,
						     G_MAXINT,
						     0,
						     CTK_PARAM_READABLE));

  g_object_class_install_property (gobject_class,
				   PROP_SCREEN,
				   g_param_spec_object ("screen",
 							P_("Screen"),
 							P_("The screen where this status icon will be displayed"),
							CDK_TYPE_SCREEN,
 							CTK_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
				   PROP_VISIBLE,
				   g_param_spec_boolean ("visible",
							 P_("Visible"),
							 P_("Whether the status icon is visible"),
							 TRUE,
							 CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));


  /**
   * CtkStatusIcon:embedded: 
   *
   * %TRUE if the statusicon is embedded in a notification area.
   *
   * Since: 2.12
   */
  g_object_class_install_property (gobject_class,
				   PROP_EMBEDDED,
				   g_param_spec_boolean ("embedded",
							 P_("Embedded"),
							 P_("Whether the status icon is embedded"),
							 FALSE,
							 CTK_PARAM_READABLE));

  /**
   * CtkStatusIcon:orientation:
   *
   * The orientation of the tray in which the statusicon 
   * is embedded. 
   *
   * Since: 2.12
   */
  g_object_class_install_property (gobject_class,
				   PROP_ORIENTATION,
				   g_param_spec_enum ("orientation",
						      P_("Orientation"),
						      P_("The orientation of the tray"),
						      CTK_TYPE_ORIENTATION,
						      CTK_ORIENTATION_HORIZONTAL,
						      CTK_PARAM_READABLE));

/**
 * CtkStatusIcon:has-tooltip:
 *
 * Enables or disables the emission of #CtkStatusIcon::query-tooltip on
 * @status_icon.  A value of %TRUE indicates that @status_icon can have a
 * tooltip, in this case the status icon will be queried using
 * #CtkStatusIcon::query-tooltip to determine whether it will provide a
 * tooltip or not.
 *
 * Note that setting this property to %TRUE for the first time will change
 * the event masks of the windows of this status icon to include leave-notify
 * and motion-notify events. This will not be undone when the property is set
 * to %FALSE again.
 *
 * Whether this property is respected is platform dependent.
 * For plain text tooltips, use #CtkStatusIcon:tooltip-text in preference.
 *
 * Since: 2.16
 */
  g_object_class_install_property (gobject_class,
				   PROP_HAS_TOOLTIP,
				   g_param_spec_boolean ("has-tooltip",
 							 P_("Has tooltip"),
 							 P_("Whether this tray icon has a tooltip"),
 							 FALSE,
 							 CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkStatusIcon:tooltip-text:
   *
   * Sets the text of tooltip to be the given string.
   *
   * Also see ctk_tooltip_set_text().
   *
   * This is a convenience property which will take care of getting the
   * tooltip shown if the given string is not %NULL.
   * #CtkStatusIcon:has-tooltip will automatically be set to %TRUE and
   * the default handler for the #CtkStatusIcon::query-tooltip signal
   * will take care of displaying the tooltip.
   *
   * Note that some platforms have limitations on the length of tooltips
   * that they allow on status icons, e.g. Windows only shows the first
   * 64 characters.
   *
   * Since: 2.16
   */
  g_object_class_install_property (gobject_class,
                                   PROP_TOOLTIP_TEXT,
                                   g_param_spec_string ("tooltip-text",
                                                        P_("Tooltip Text"),
                                                        P_("The contents of the tooltip for this widget"),
                                                        NULL,
                                                        CTK_PARAM_READWRITE));
  /**
   * CtkStatusIcon:tooltip-markup:
   *
   * Sets the text of tooltip to be the given string, which is marked up
   * with the [Pango text markup language][PangoMarkupFormat].
   * Also see ctk_tooltip_set_markup().
   *
   * This is a convenience property which will take care of getting the
   * tooltip shown if the given string is not %NULL.
   * #CtkStatusIcon:has-tooltip will automatically be set to %TRUE and
   * the default handler for the #CtkStatusIcon::query-tooltip signal
   * will take care of displaying the tooltip.
   *
   * On some platforms, embedded markup will be ignored.
   *
   * Since: 2.16
   */
  g_object_class_install_property (gobject_class,
				   PROP_TOOLTIP_MARKUP,
				   g_param_spec_string ("tooltip-markup",
 							P_("Tooltip markup"),
							P_("The contents of the tooltip for this tray icon"),
							NULL,
							CTK_PARAM_READWRITE));


  /**
   * CtkStatusIcon:title:
   *
   * The title of this tray icon. This should be a short, human-readable,
   * localized string describing the tray icon. It may be used by tools
   * like screen readers to render the tray icon.
   *
   * Since: 2.18
   */
  g_object_class_install_property (gobject_class,
                                   PROP_TITLE,
                                   g_param_spec_string ("title",
                                                        P_("Title"),
                                                        P_("The title of this tray icon"),
                                                        NULL,
                                                        CTK_PARAM_READWRITE));

  /**
   * CtkStatusIcon::activate:
   * @status_icon: the object which received the signal
   *
   * Gets emitted when the user activates the status icon. 
   * If and how status icons can activated is platform-dependent.
   *
   * Unlike most G_SIGNAL_ACTION signals, this signal is meant to 
   * be used by applications and should be wrapped by language bindings.
   *
   * Since: 2.10
   */
  status_icon_signals [ACTIVATE_SIGNAL] =
    g_signal_new (I_("activate"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkStatusIconClass, activate),
		  NULL,
		  NULL,
		  NULL,
		  G_TYPE_NONE,
		  0);

  /**
   * CtkStatusIcon::popup-menu:
   * @status_icon: the object which received the signal
   * @button: the button that was pressed, or 0 if the 
   *   signal is not emitted in response to a button press event
   * @activate_time: the timestamp of the event that
   *   triggered the signal emission
   *
   * Gets emitted when the user brings up the context menu
   * of the status icon. Whether status icons can have context 
   * menus and how these are activated is platform-dependent.
   *
   * The @button and @activate_time parameters should be 
   * passed as the last to arguments to ctk_menu_popup().
   *
   * Unlike most G_SIGNAL_ACTION signals, this signal is meant to 
   * be used by applications and should be wrapped by language bindings.
   *
   * Since: 2.10
   */
  status_icon_signals [POPUP_MENU_SIGNAL] =
    g_signal_new (I_("popup-menu"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkStatusIconClass, popup_menu),
		  NULL,
		  NULL,
		  _ctk_marshal_VOID__UINT_UINT,
		  G_TYPE_NONE,
		  2,
		  G_TYPE_UINT,
		  G_TYPE_UINT);

  /**
   * CtkStatusIcon::size-changed:
   * @status_icon: the object which received the signal
   * @size: the new size
   *
   * Gets emitted when the size available for the image
   * changes, e.g. because the notification area got resized.
   *
   * Returns: %TRUE if the icon was updated for the new
   * size. Otherwise, CTK+ will scale the icon as necessary.
   *
   * Since: 2.10
   */
  status_icon_signals [SIZE_CHANGED_SIGNAL] =
    g_signal_new (I_("size-changed"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkStatusIconClass, size_changed),
		  g_signal_accumulator_true_handled,
		  NULL,
		  _ctk_marshal_BOOLEAN__INT,
		  G_TYPE_BOOLEAN,
		  1,
		  G_TYPE_INT);

  /**
   * CtkStatusIcon::button-press-event:
   * @status_icon: the object which received the signal
   * @event: (type Cdk.EventButton): the #CdkEventButton which triggered 
   *                                 this signal
   *
   * The ::button-press-event signal will be emitted when a button
   * (typically from a mouse) is pressed.
   *
   * Whether this event is emitted is platform-dependent.  Use the ::activate
   * and ::popup-menu signals in preference.
   *
   * Returns: %TRUE to stop other handlers from being invoked
   * for the event. %FALSE to propagate the event further.
   *
   * Since: 2.14
   */
  status_icon_signals [BUTTON_PRESS_EVENT_SIGNAL] =
    g_signal_new (I_("button_press_event"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkStatusIconClass, button_press_event),
		  g_signal_accumulator_true_handled, NULL,
		  _ctk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * CtkStatusIcon::button-release-event:
   * @status_icon: the object which received the signal
   * @event: (type Cdk.EventButton): the #CdkEventButton which triggered 
   *                                 this signal
   *
   * The ::button-release-event signal will be emitted when a button
   * (typically from a mouse) is released.
   *
   * Whether this event is emitted is platform-dependent.  Use the ::activate
   * and ::popup-menu signals in preference.
   *
   * Returns: %TRUE to stop other handlers from being invoked
   * for the event. %FALSE to propagate the event further.
   *
   * Since: 2.14
   */
  status_icon_signals [BUTTON_RELEASE_EVENT_SIGNAL] =
    g_signal_new (I_("button_release_event"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkStatusIconClass, button_release_event),
		  g_signal_accumulator_true_handled, NULL,
		  _ctk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * CtkStatusIcon::scroll-event:
   * @status_icon: the object which received the signal.
   * @event: (type Cdk.EventScroll): the #CdkEventScroll which triggered 
   *                                 this signal
   *
   * The ::scroll-event signal is emitted when a button in the 4 to 7
   * range is pressed. Wheel mice are usually configured to generate
   * button press events for buttons 4 and 5 when the wheel is turned.
   *
   * Whether this event is emitted is platform-dependent.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event.
   *   %FALSE to propagate the event further.
   *
   * Since: 2.16
   */
  status_icon_signals[SCROLL_EVENT_SIGNAL] =
    g_signal_new (I_("scroll_event"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkStatusIconClass, scroll_event),
		  g_signal_accumulator_true_handled, NULL,
		  _ctk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * CtkStatusIcon::query-tooltip:
   * @status_icon: the object which received the signal
   * @x: the x coordinate of the cursor position where the request has been
   *     emitted, relative to @status_icon
   * @y: the y coordinate of the cursor position where the request has been
   *     emitted, relative to @status_icon
   * @keyboard_mode: %TRUE if the tooltip was trigged using the keyboard
   * @tooltip: a #CtkTooltip
   *
   * Emitted when the hover timeout has expired with the
   * cursor hovering above @status_icon; or emitted when @status_icon got
   * focus in keyboard mode.
   *
   * Using the given coordinates, the signal handler should determine
   * whether a tooltip should be shown for @status_icon. If this is
   * the case %TRUE should be returned, %FALSE otherwise. Note that if
   * @keyboard_mode is %TRUE, the values of @x and @y are undefined and
   * should not be used.
   *
   * The signal handler is free to manipulate @tooltip with the therefore
   * destined function calls.
   *
   * Whether this signal is emitted is platform-dependent.
   * For plain text tooltips, use #CtkStatusIcon:tooltip-text in preference.
   *
   * Returns: %TRUE if @tooltip should be shown right now, %FALSE otherwise.
   *
   * Since: 2.16
   */
  status_icon_signals [QUERY_TOOLTIP_SIGNAL] =
    g_signal_new (I_("query_tooltip"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkStatusIconClass, query_tooltip),
		  g_signal_accumulator_true_handled, NULL,
		  _ctk_marshal_BOOLEAN__INT_INT_BOOLEAN_OBJECT,
		  G_TYPE_BOOLEAN, 4,
		  G_TYPE_INT,
		  G_TYPE_INT,
		  G_TYPE_BOOLEAN,
		  CTK_TYPE_TOOLTIP);
}

#ifdef CDK_WINDOWING_WIN32

static void
build_button_event (CtkStatusIconPrivate *priv,
		    CdkEventButton       *e,
		    guint                 button)
{
  POINT pos;
  CdkRectangle monitor0;

  /* We know that cdk/win32 puts the primary monitor at index 0 */
  cdk_screen_get_monitor_geometry (cdk_screen_get_default (), 0, &monitor0);
  e->window = g_object_ref (cdk_get_default_root_window ());
  e->send_event = TRUE;
  e->time = GetTickCount ();
  GetCursorPos (&pos);
  priv->last_click_x = e->x = pos.x + monitor0.x;
  priv->last_click_y = e->y = pos.y + monitor0.y;
  e->axes = NULL;
  e->state = 0;
  e->button = button;
  //FIXME: e->device = cdk_display_get_default ()->core_pointer;
  e->x_root = e->x;
  e->y_root = e->y;
}

typedef struct
{
  CtkStatusIcon *status_icon;
  CdkEventButton *event;
} ButtonCallbackData;

static gboolean
button_callback (gpointer data)
{
  ButtonCallbackData *bc = (ButtonCallbackData *) data;

  if (bc->event->type == CDK_BUTTON_PRESS)
    ctk_status_icon_button_press (bc->status_icon, bc->event);
  else
    ctk_status_icon_button_release (bc->status_icon, bc->event);

  cdk_event_free ((CdkEvent *) bc->event);
  g_free (data);

  return G_SOURCE_REMOVE;
}

static UINT taskbar_created_msg = 0;
static GSList *status_icons = NULL;
static UINT status_icon_id = 0;

static CtkStatusIcon *
find_status_icon (UINT id)
{
  GSList *rover;

  for (rover = status_icons; rover != NULL; rover = rover->next)
    {
      CtkStatusIcon *status_icon = CTK_STATUS_ICON (rover->data);
      CtkStatusIconPrivate *priv = status_icon->priv;

      if (priv->nid.uID == id)
        return status_icon;
    }

  return NULL;
}

static LRESULT CALLBACK
wndproc (HWND   hwnd,
	 UINT   message,
	 WPARAM wparam,
	 LPARAM lparam)
{
  if (message == taskbar_created_msg)
    {
      GSList *rover;

      for (rover = status_icons; rover != NULL; rover = rover->next)
	{
	  CtkStatusIcon *status_icon = CTK_STATUS_ICON (rover->data);
	  CtkStatusIconPrivate *priv = status_icon->priv;

          if (priv->visible)
            {
              /* taskbar_created_msg is also fired when DPI changes. Try to delete existing icons if possible. */
              if (priv->nid.hWnd)
                if (!Shell_NotifyIconW (NIM_DELETE, &priv->nid))
                  g_warning (G_STRLOC ": Shell_NotifyIcon(NIM_DELETE) on existing icon failed");

              priv->nid.hWnd = hwnd;
              priv->nid.uFlags &= ~NIF_ICON;

              if (!Shell_NotifyIconW (NIM_ADD, &priv->nid))
                {
                  g_warning (G_STRLOC ": Shell_NotifyIcon(NIM_ADD) failed");
                  priv->nid.hWnd = NULL;
                  continue;
                }

              ctk_status_icon_update_image (status_icon);
            }
	}
      return 0;
    }

  if (message == WM_CTK_TRAY_NOTIFICATION)
    {
      ButtonCallbackData *bc;
      guint button;
      
      switch (lparam)
	{
	case WM_LBUTTONDOWN:
	  button = 1;
	  goto buttondown0;

	case WM_MBUTTONDOWN:
	  button = 2;
	  goto buttondown0;

	case WM_RBUTTONDOWN:
	  button = 3;
	  goto buttondown0;

	case WM_XBUTTONDOWN:
	  if (HIWORD (wparam) == XBUTTON1)
	    button = 4;
	  else
	    button = 5;

	buttondown0:
	  bc = g_new (ButtonCallbackData, 1);
	  bc->event = (CdkEventButton *) cdk_event_new (CDK_BUTTON_PRESS);
	  bc->status_icon = find_status_icon (wparam);
	  build_button_event (bc->status_icon->priv, bc->event, button);
	  g_idle_add (button_callback, bc);
	  break;

	case WM_LBUTTONUP:
	  button = 1;
	  goto buttonup0;

	case WM_MBUTTONUP:
	  button = 2;
	  goto buttonup0;

	case WM_RBUTTONUP:
	  button = 3;
	  goto buttonup0;

	case WM_XBUTTONUP:
	  if (HIWORD (wparam) == XBUTTON1)
	    button = 4;
	  else
	    button = 5;

	buttonup0:
	  bc = g_new (ButtonCallbackData, 1);
	  bc->event = (CdkEventButton *) cdk_event_new (CDK_BUTTON_RELEASE);
	  bc->status_icon = find_status_icon (wparam);
	  build_button_event (bc->status_icon->priv, bc->event, button);
	  g_idle_add (button_callback, bc);
	  break;

	default :
	  break;
	}
	return 0;
    }
  else
    {
      return DefWindowProc (hwnd, message, wparam, lparam);
    }
}

static HWND
create_tray_observer (void)
{
  WNDCLASS    wclass;
  static HWND hwnd = NULL;
  ATOM        klass;
  HINSTANCE   hmodule = GetModuleHandle (NULL);

  if (hwnd)
    return hwnd;

  taskbar_created_msg = RegisterWindowMessage("TaskbarCreated");

  memset (&wclass, 0, sizeof(WNDCLASS));
  wclass.lpszClassName = "ctkstatusicon-observer";
  wclass.lpfnWndProc   = wndproc;
  wclass.hInstance     = hmodule;

  klass = RegisterClass (&wclass);
  if (!klass)
    return NULL;

  hwnd = CreateWindow (MAKEINTRESOURCE (klass),
                       NULL, WS_POPUP,
                       0, 0, 1, 1, NULL, NULL,
                       hmodule, NULL);
  if (!hwnd)
    {
      UnregisterClass (MAKEINTRESOURCE(klass), hmodule);
      return NULL;
    }

  return hwnd;
}

#endif

static void
ctk_status_icon_init (CtkStatusIcon *status_icon)
{
  CtkStatusIconPrivate *priv;

  priv = ctk_status_icon_get_instance_private (status_icon);
  status_icon->priv = priv;

  priv->image_def = ctk_image_definition_new_empty ();
  priv->visible = TRUE;

#ifdef CDK_WINDOWING_X11
  if (CDK_IS_X11_DISPLAY (cdk_display_get_default ()))
    {
      priv->size         = 0;
      priv->tray_icon = CTK_WIDGET (_ctk_tray_icon_new (NULL));

      ctk_widget_add_events (CTK_WIDGET (priv->tray_icon),
                             CDK_BUTTON_PRESS_MASK | CDK_BUTTON_RELEASE_MASK |
                             CDK_SCROLL_MASK);

      g_signal_connect_swapped (priv->tray_icon, "key-press-event",
                                G_CALLBACK (ctk_status_icon_key_press), status_icon);
      g_signal_connect_swapped (priv->tray_icon, "popup-menu",
                                G_CALLBACK (ctk_status_icon_popup_menu), status_icon);
      g_signal_connect_swapped (priv->tray_icon, "notify::embedded",
                                G_CALLBACK (ctk_status_icon_embedded_changed), status_icon);
      g_signal_connect_swapped (priv->tray_icon, "notify::orientation",
                                G_CALLBACK (ctk_status_icon_orientation_changed), status_icon);
      g_signal_connect_swapped (priv->tray_icon, "notify::padding",
                                G_CALLBACK (ctk_status_icon_padding_changed), status_icon);
      g_signal_connect_swapped (priv->tray_icon, "notify::icon-size",
                                G_CALLBACK (ctk_status_icon_icon_size_changed), status_icon);
      g_signal_connect_swapped (priv->tray_icon, "notify::fg-color",
                                G_CALLBACK (ctk_status_icon_fg_changed), status_icon);
      g_signal_connect (priv->tray_icon, "notify::error-color",
                        G_CALLBACK (ctk_status_icon_color_changed), status_icon);
      g_signal_connect (priv->tray_icon, "notify::warning-color",
                        G_CALLBACK (ctk_status_icon_color_changed), status_icon);
      g_signal_connect (priv->tray_icon, "notify::success-color",
                        G_CALLBACK (ctk_status_icon_color_changed), status_icon);
      g_signal_connect_swapped (priv->tray_icon, "button-press-event",
                                G_CALLBACK (ctk_status_icon_button_press), status_icon);
      g_signal_connect_swapped (priv->tray_icon, "button-release-event",
                                G_CALLBACK (ctk_status_icon_button_release), status_icon);
      g_signal_connect_swapped (priv->tray_icon, "scroll-event",
                                G_CALLBACK (ctk_status_icon_scroll), status_icon);
      g_signal_connect_swapped (priv->tray_icon, "query-tooltip",
                                G_CALLBACK (ctk_status_icon_query_tooltip), status_icon);
      g_signal_connect_swapped (priv->tray_icon, "screen-changed",
                                G_CALLBACK (ctk_status_icon_screen_changed), status_icon);
      priv->image = ctk_image_new ();
      ctk_widget_set_can_focus (priv->image, TRUE);
      ctk_container_add (CTK_CONTAINER (priv->tray_icon), priv->image);
      ctk_widget_show (priv->image);

      /* Force-initialize the symbolic colors */
      g_object_notify (G_OBJECT (priv->tray_icon), "fg-color");
      g_object_notify (G_OBJECT (priv->tray_icon), "error-color");
      g_object_notify (G_OBJECT (priv->tray_icon), "warning-color");
      g_object_notify (G_OBJECT (priv->tray_icon), "success-color");

      g_signal_connect_swapped (priv->image, "size-allocate",
                                G_CALLBACK (ctk_status_icon_size_allocate), status_icon);
    }
#else /* !CDK_WINDOWING_X11 */
  priv->dummy_widget = ctk_label_new ("");
#endif

#ifdef CDK_WINDOWING_WIN32

  /* Get position and orientation of Windows taskbar. */
  {
    APPBARDATA abd;

    abd.cbSize = sizeof (abd);
    SHAppBarMessage (ABM_GETTASKBARPOS, &abd);
    if (abd.rc.bottom - abd.rc.top > abd.rc.right - abd.rc.left)
      priv->orientation = CTK_ORIENTATION_VERTICAL;
    else
      priv->orientation = CTK_ORIENTATION_HORIZONTAL;

    priv->taskbar_top = abd.rc.top;
  }

  priv->last_click_x = priv->last_click_y = 0;

  /* Are the system tray icons always 16 pixels square? */
  priv->size         = 16;

  memset (&priv->nid, 0, sizeof (priv->nid));

  priv->nid.hWnd = create_tray_observer ();
  priv->nid.uID = status_icon_id++;
  priv->nid.uCallbackMessage = WM_CTK_TRAY_NOTIFICATION;
  priv->nid.uFlags = NIF_MESSAGE;

  /* To help win7 identify the icon create it with an application "unique" tip */
  if (g_get_prgname ())
  {
    WCHAR *wcs = g_utf8_to_utf16 (g_get_prgname (), -1, NULL, NULL, NULL);

    priv->nid.uFlags |= NIF_TIP;
    wcsncpy (priv->nid.szTip, wcs, G_N_ELEMENTS (priv->nid.szTip) - 1);
    priv->nid.szTip[G_N_ELEMENTS (priv->nid.szTip) - 1] = 0;
    g_free (wcs);
  }

  if (!Shell_NotifyIconW (NIM_ADD, &priv->nid))
    {
      g_warning (G_STRLOC ": Shell_NotifyIcon(NIM_ADD) failed");
      priv->nid.hWnd = NULL;
    }

  status_icons = g_slist_append (status_icons, status_icon);

#endif
	
#ifdef CDK_WINDOWING_QUARTZ
  QUARTZ_POOL_ALLOC;

  priv->status_item = [[CtkQuartzStatusIcon alloc] initWithStatusIcon:status_icon];
  priv->size = [priv->status_item getHeight];

  QUARTZ_POOL_RELEASE;

#endif 
}

static void
ctk_status_icon_constructed (GObject *object)
{
  G_OBJECT_CLASS (ctk_status_icon_parent_class)->constructed (object);

#ifdef CDK_WINDOWING_X11
  {
    CtkStatusIcon *status_icon = CTK_STATUS_ICON (object);
    CtkStatusIconPrivate *priv = status_icon->priv;

    if (priv->visible && priv->tray_icon)
      ctk_widget_show (priv->tray_icon);
  }
#endif
}

static void
ctk_status_icon_finalize (GObject *object)
{
  CtkStatusIcon *status_icon = CTK_STATUS_ICON (object);
  CtkStatusIconPrivate *priv = status_icon->priv;

  ctk_status_icon_reset_image_data (status_icon);
  ctk_image_definition_unref (priv->image_def);

#ifdef CDK_WINDOWING_X11
  if (priv->tray_icon)
    {
      g_signal_handlers_disconnect_by_func (priv->tray_icon,
                                            ctk_status_icon_key_press, status_icon);
      g_signal_handlers_disconnect_by_func (priv->tray_icon,
                                            ctk_status_icon_popup_menu, status_icon);
      g_signal_handlers_disconnect_by_func (priv->tray_icon,
                                            ctk_status_icon_embedded_changed, status_icon);
      g_signal_handlers_disconnect_by_func (priv->tray_icon,
                                            ctk_status_icon_orientation_changed, status_icon);
      g_signal_handlers_disconnect_by_func (priv->tray_icon,
                                            ctk_status_icon_padding_changed, status_icon);
      g_signal_handlers_disconnect_by_func (priv->tray_icon,
                                            ctk_status_icon_icon_size_changed, status_icon);
      g_signal_handlers_disconnect_by_func (priv->tray_icon,
                                            ctk_status_icon_fg_changed, status_icon);
      g_signal_handlers_disconnect_by_func (priv->tray_icon,
                                            ctk_status_icon_color_changed, status_icon);
      g_signal_handlers_disconnect_by_func (priv->tray_icon,
                                            ctk_status_icon_button_press, status_icon);
      g_signal_handlers_disconnect_by_func (priv->tray_icon,
                                            ctk_status_icon_button_release, status_icon);
      g_signal_handlers_disconnect_by_func (priv->tray_icon,
                                            ctk_status_icon_scroll, status_icon);
      g_signal_handlers_disconnect_by_func (priv->tray_icon,
                                            ctk_status_icon_query_tooltip, status_icon);
      g_signal_handlers_disconnect_by_func (priv->tray_icon,
                                            ctk_status_icon_screen_changed, status_icon);
      ctk_widget_destroy (priv->image);
      ctk_widget_destroy (priv->tray_icon);
    }
#else /* !CDK_WINDOWING_X11 */
  ctk_widget_destroy (priv->dummy_widget);
#endif

#ifdef CDK_WINDOWING_WIN32
  if (priv->nid.hWnd != NULL && priv->visible)
    Shell_NotifyIconW (NIM_DELETE, &priv->nid);
  if (priv->nid.hIcon)
    DestroyIcon (priv->nid.hIcon);
  g_free (priv->tooltip_text);

  status_icons = g_slist_remove (status_icons, status_icon);
#endif

#ifdef CDK_WINDOWING_QUARTZ
  QUARTZ_POOL_ALLOC;
  [priv->status_item release];
  QUARTZ_POOL_RELEASE;
  g_free (priv->tooltip_text);
#endif

  G_OBJECT_CLASS (ctk_status_icon_parent_class)->finalize (object);
}

static void
ctk_status_icon_set_property (GObject      *object,
			      guint         prop_id,
			      const GValue *value,
			      GParamSpec   *pspec)
{
  CtkStatusIcon *status_icon = CTK_STATUS_ICON (object);

  switch (prop_id)
    {
    case PROP_PIXBUF:
      ctk_status_icon_set_from_pixbuf (status_icon, g_value_get_object (value));
      break;
    case PROP_FILE:
      ctk_status_icon_set_from_file (status_icon, g_value_get_string (value));
      break;
    case PROP_STOCK:
      ctk_status_icon_set_from_stock (status_icon, g_value_get_string (value));
      break;
    case PROP_ICON_NAME:
      ctk_status_icon_set_from_icon_name (status_icon, g_value_get_string (value));
      break;
    case PROP_GICON:
      ctk_status_icon_set_from_gicon (status_icon, g_value_get_object (value));
      break;
    case PROP_SCREEN:
      ctk_status_icon_set_screen (status_icon, g_value_get_object (value));
      break;
    case PROP_VISIBLE:
      ctk_status_icon_set_visible (status_icon, g_value_get_boolean (value));
      break;
    case PROP_HAS_TOOLTIP:
      ctk_status_icon_set_has_tooltip (status_icon, g_value_get_boolean (value));
      break;
    case PROP_TOOLTIP_TEXT:
      ctk_status_icon_set_tooltip_text (status_icon, g_value_get_string (value));
      break;
    case PROP_TOOLTIP_MARKUP:
      ctk_status_icon_set_tooltip_markup (status_icon, g_value_get_string (value));
      break;
    case PROP_TITLE:
      ctk_status_icon_set_title (status_icon, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_status_icon_get_property (GObject    *object,
			      guint       prop_id,
			      GValue     *value,
			      GParamSpec *pspec)
{
  CtkStatusIcon *status_icon = CTK_STATUS_ICON (object);

  switch (prop_id)
    {
    case PROP_PIXBUF:
      g_value_set_object (value, ctk_status_icon_get_pixbuf (status_icon));
      break;
    case PROP_STOCK:
      g_value_set_string (value, ctk_status_icon_get_stock (status_icon));
      break;
    case PROP_ICON_NAME:
      g_value_set_string (value, ctk_status_icon_get_icon_name (status_icon));
      break;
    case PROP_GICON:
      g_value_set_object (value, ctk_status_icon_get_gicon (status_icon));
      break;
    case PROP_STORAGE_TYPE:
      g_value_set_enum (value, ctk_status_icon_get_storage_type (status_icon));
      break;
    case PROP_SIZE:
      g_value_set_int (value, ctk_status_icon_get_size (status_icon));
      break;
    case PROP_SCREEN:
      g_value_set_object (value, ctk_status_icon_get_screen (status_icon));
      break;
    case PROP_VISIBLE:
      g_value_set_boolean (value, ctk_status_icon_get_visible (status_icon));
      break;
    case PROP_EMBEDDED:
      g_value_set_boolean (value, ctk_status_icon_is_embedded (status_icon));
      break;
    case PROP_ORIENTATION:
#ifdef CDK_WINDOWING_X11
      if (status_icon->priv->tray_icon)
        g_value_set_enum (value, _ctk_tray_icon_get_orientation (CTK_TRAY_ICON (status_icon->priv->tray_icon)));
      else
        g_value_set_enum (value, CTK_ORIENTATION_HORIZONTAL);
#endif
#ifdef CDK_WINDOWING_WIN32
      g_value_set_enum (value, status_icon->priv->orientation);
#endif
      break;
    case PROP_HAS_TOOLTIP:
      g_value_set_boolean (value, ctk_status_icon_get_has_tooltip (status_icon));
      break;
    case PROP_TOOLTIP_TEXT:
      g_value_set_string (value, ctk_status_icon_get_tooltip_text (status_icon));
      break;
    case PROP_TOOLTIP_MARKUP:
      g_value_set_string (value, ctk_status_icon_get_tooltip_markup (status_icon));
      break;
    case PROP_TITLE:
      g_value_set_string (value, ctk_status_icon_get_title (status_icon));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * ctk_status_icon_new:
 * 
 * Creates an empty status icon object.
 * 
 * Returns: a new #CtkStatusIcon
 *
 * Since: 2.10
 *
 * Deprecated: 3.14: Use #GNotification and #CtkApplication to
 *   provide status notifications
 */
CtkStatusIcon *
ctk_status_icon_new (void)
{
  return g_object_new (CTK_TYPE_STATUS_ICON, NULL);
}

/**
 * ctk_status_icon_new_from_pixbuf:
 * @pixbuf: a #GdkPixbuf
 * 
 * Creates a status icon displaying @pixbuf. 
 *
 * The image will be scaled down to fit in the available 
 * space in the notification area, if necessary.
 * 
 * Returns: a new #CtkStatusIcon
 *
 * Since: 2.10
 *
 * Deprecated: 3.14: Use #GNotification and #CtkApplication to
 *   provide status notifications
 */
CtkStatusIcon *
ctk_status_icon_new_from_pixbuf (GdkPixbuf *pixbuf)
{
  return g_object_new (CTK_TYPE_STATUS_ICON,
		       "pixbuf", pixbuf,
		       NULL);
}

/**
 * ctk_status_icon_new_from_file:
 * @filename: (type filename): a filename
 * 
 * Creates a status icon displaying the file @filename. 
 *
 * The image will be scaled down to fit in the available 
 * space in the notification area, if necessary.
 * 
 * Returns: a new #CtkStatusIcon
 *
 * Since: 2.10
 *
 * Deprecated: 3.14: Use #GNotification and #CtkApplication to
 *   provide status notifications
 */
CtkStatusIcon *
ctk_status_icon_new_from_file (const gchar *filename)
{
  return g_object_new (CTK_TYPE_STATUS_ICON,
		       "file", filename,
		       NULL);
}

/**
 * ctk_status_icon_new_from_stock:
 * @stock_id: a stock icon id
 * 
 * Creates a status icon displaying a stock icon. Sample stock icon
 * names are #CTK_STOCK_OPEN, #CTK_STOCK_QUIT. You can register your 
 * own stock icon names, see ctk_icon_factory_add_default() and 
 * ctk_icon_factory_add(). 
 *
 * Returns: a new #CtkStatusIcon
 *
 * Since: 2.10
 *
 * Deprecated: 3.14: Use #GNotification and #CtkApplication to
 *   provide status notifications
 */
CtkStatusIcon *
ctk_status_icon_new_from_stock (const gchar *stock_id)
{
  return g_object_new (CTK_TYPE_STATUS_ICON,
		       "stock", stock_id,
		       NULL);
}

/**
 * ctk_status_icon_new_from_icon_name:
 * @icon_name: an icon name
 * 
 * Creates a status icon displaying an icon from the current icon theme.
 * If the current icon theme is changed, the icon will be updated 
 * appropriately.
 * 
 * Returns: a new #CtkStatusIcon
 *
 * Since: 2.10
 *
 * Deprecated: 3.14: Use #GNotification and #CtkApplication to
 *   provide status notifications
 */
CtkStatusIcon *
ctk_status_icon_new_from_icon_name (const gchar *icon_name)
{
  return g_object_new (CTK_TYPE_STATUS_ICON,
		       "icon-name", icon_name,
		       NULL);
}

/**
 * ctk_status_icon_new_from_gicon:
 * @icon: a #GIcon
 *
 * Creates a status icon displaying a #GIcon. If the icon is a
 * themed icon, it will be updated when the theme changes.
 *
 * Returns: a new #CtkStatusIcon
 *
 * Since: 2.14
 *
 * Deprecated: 3.14: Use #GNotification and #CtkApplication to
 *   provide status notifications
 */
CtkStatusIcon *
ctk_status_icon_new_from_gicon (GIcon *icon)
{
  return g_object_new (CTK_TYPE_STATUS_ICON,
		       "gicon", icon,
		       NULL);
}

static void
emit_activate_signal (CtkStatusIcon *status_icon)
{
  g_signal_emit (status_icon,
		 status_icon_signals [ACTIVATE_SIGNAL], 0);
}

static void
emit_popup_menu_signal (CtkStatusIcon *status_icon,
			guint          button,
			guint32        activate_time)
{
  g_signal_emit (status_icon,
		 status_icon_signals [POPUP_MENU_SIGNAL], 0,
		 button,
		 activate_time);
}

#ifdef CDK_WINDOWING_X11

static gboolean
emit_size_changed_signal (CtkStatusIcon *status_icon,
			  gint           size)
{
  gboolean handled = FALSE;
  
  g_signal_emit (status_icon,
		 status_icon_signals [SIZE_CHANGED_SIGNAL], 0,
		 size,
		 &handled);

  return handled;
}

#endif

/* rounds the pixel size to the nearest size avaiable in the theme */
static gint
round_pixel_size (CtkWidget *widget, 
                  gint       pixel_size)
{
  CtkIconSize s;
  gint w, h, d, dist, size;

  dist = G_MAXINT;
  size = 0;

  for (s = CTK_ICON_SIZE_MENU; s <= CTK_ICON_SIZE_DIALOG; s++)
    {
      if (ctk_icon_size_lookup (s, &w, &h))
	{
	  d = MAX (abs (pixel_size - w), abs (pixel_size - h));
	  if (d < dist)
	    {
	      dist = d;
              size = MAX (w, h);
	    }
	}
    }
  
  return size;
}

static void
ctk_status_icon_update_image (CtkStatusIcon *status_icon)
{
  CtkStatusIconPrivate *priv = status_icon->priv;
#ifdef CDK_WINDOWING_WIN32
  HICON prev_hicon;
#endif
  CtkIconHelper *icon_helper;
  cairo_surface_t *surface;
  CtkWidget *widget;
#ifndef CDK_WINDOWING_X11
  GdkPixbuf *pixbuf;
#endif
  gint round_size;
  gint scale;

#ifdef CDK_WINDOWING_X11
  widget = priv->image;
  scale = ctk_widget_get_scale_factor (widget);
#else
  widget = priv->dummy_widget;
  scale = 1;
#endif

  if (widget == NULL)
    return;

  round_size = round_pixel_size (widget, priv->size);

  icon_helper = ctk_icon_helper_new (ctk_style_context_get_node (ctk_widget_get_style_context (widget)), widget);
  _ctk_icon_helper_set_force_scale_pixbuf (icon_helper, TRUE);
  _ctk_icon_helper_set_definition (icon_helper, priv->image_def);
  _ctk_icon_helper_set_icon_size (icon_helper, CTK_ICON_SIZE_SMALL_TOOLBAR);
  _ctk_icon_helper_set_pixel_size (icon_helper, round_size);
  surface = ctk_icon_helper_load_surface (icon_helper, scale);

  g_object_unref (icon_helper);

#ifdef CDK_WINDOWING_X11
  if (surface)
    {
      ctk_image_set_from_surface (CTK_IMAGE (priv->image), surface);
      cairo_surface_destroy (surface);
    }
  else
    {
      ctk_image_set_from_pixbuf (CTK_IMAGE (priv->image), NULL);
    }
#endif

#ifdef CDK_WINDOWING_WIN32
  if (surface)
    {
      pixbuf = gdk_pixbuf_get_from_surface (surface, 0, 0,
                                            cairo_image_surface_get_width (surface),
                                            cairo_image_surface_get_height (surface));
      cairo_surface_destroy (surface);
    }

  if (pixbuf != NULL)
    {
      prev_hicon = priv->nid.hIcon;
      priv->nid.hIcon = cdk_win32_pixbuf_to_hicon_libctk_only (pixbuf);
      priv->nid.uFlags |= NIF_ICON;
      if (priv->nid.hWnd != NULL && priv->visible)
        if (!Shell_NotifyIconW (NIM_MODIFY, &priv->nid))
          g_warning (G_STRLOC ": Shell_NotifyIcon(NIM_MODIFY) failed");
      if (prev_hicon)
        DestroyIcon (prev_hicon);
    }
  else
    {
      priv->nid.uFlags &= ~NIF_ICON;
      if (priv->nid.hWnd != NULL && priv->visible)
        if (!Shell_NotifyIconW (NIM_MODIFY, &priv->nid))
          g_warning (G_STRLOC ": Shell_NotifyIcon(NIM_MODIFY) failed");
    }

  g_clear_object (&pixbuf);
#endif

#ifdef CDK_WINDOWING_QUARTZ
  if (surface)
    {
      pixbuf = gdk_pixbuf_get_from_surface (surface, 0, 0,
                                            cairo_image_surface_get_width (surface),
                                            cairo_image_surface_get_height (surface));
      cairo_surface_destroy (surface);
    }

  if (pixbuf != NULL)
    {
      QUARTZ_POOL_ALLOC;
      [priv->status_item setImage:pixbuf];
      QUARTZ_POOL_RELEASE;
    }
  else
    {
      [priv->status_item setImage:NULL];
    }

  g_clear_object (&pixbuf);
#endif
}

#ifdef CDK_WINDOWING_X11

static void
ctk_status_icon_size_allocate (CtkStatusIcon *status_icon,
			       CtkAllocation *allocation)
{
  CtkStatusIconPrivate *priv = status_icon->priv;
  CtkOrientation orientation;
  gint size;

  orientation = _ctk_tray_icon_get_orientation (CTK_TRAY_ICON (priv->tray_icon));

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    size = allocation->height;
  else
    size = allocation->width;

  if (priv->size - 1 > size || priv->size + 1 < size)
    {
      priv->size = size;

      g_object_notify (G_OBJECT (status_icon), "size");

      if (!emit_size_changed_signal (status_icon, size))
	ctk_status_icon_update_image (status_icon);
    }
}

static void
ctk_status_icon_screen_changed (CtkStatusIcon *status_icon,
				CdkScreen *old_screen)
{
  CtkStatusIconPrivate *priv = status_icon->priv;

  if (ctk_widget_get_screen (priv->tray_icon) != old_screen)
    {
      g_object_notify (G_OBJECT (status_icon), "screen");
    }
}

static void
ctk_status_icon_padding_changed (CtkStatusIcon *status_icon)
{
  CtkStatusIconPrivate *priv = status_icon->priv;
  CtkOrientation orientation;
  gint padding;

  orientation = _ctk_tray_icon_get_orientation (CTK_TRAY_ICON (priv->tray_icon));
  padding = _ctk_tray_icon_get_padding (CTK_TRAY_ICON (priv->tray_icon));

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      ctk_widget_set_margin_start (priv->image, padding);
      ctk_widget_set_margin_end (priv->image, padding);
    }
  else
    {
      ctk_widget_set_margin_bottom (priv->image, padding);
      ctk_widget_set_margin_top (priv->image, padding);
    }
}

static void
ctk_status_icon_icon_size_changed (CtkStatusIcon *status_icon)
{
  CtkStatusIconPrivate *priv = status_icon->priv;
  gint icon_size;

  icon_size = _ctk_tray_icon_get_icon_size (CTK_TRAY_ICON (priv->tray_icon));

  if (icon_size != 0)
    ctk_image_set_pixel_size (CTK_IMAGE (priv->image), icon_size);
  else
    ctk_image_set_pixel_size (CTK_IMAGE (priv->image), -1);
}

static void
ctk_status_icon_embedded_changed (CtkStatusIcon *status_icon)
{
  ctk_status_icon_padding_changed (status_icon);
  ctk_status_icon_icon_size_changed (status_icon);
  g_object_notify (G_OBJECT (status_icon), "embedded");
}

static void
ctk_status_icon_orientation_changed (CtkStatusIcon *status_icon)
{
  ctk_status_icon_padding_changed (status_icon);
  g_object_notify (G_OBJECT (status_icon), "orientation");
}

static void
ctk_status_icon_fg_changed (CtkStatusIcon *status_icon)
{
  CtkStatusIconPrivate *priv = status_icon->priv;
  CdkRGBA *rgba;

  g_object_get (priv->tray_icon, "fg-color", &rgba, NULL);

  ctk_widget_override_color (priv->image, CTK_STATE_FLAG_NORMAL, rgba);

  cdk_rgba_free (rgba);
}

static void
ctk_status_icon_color_changed (CtkTrayIcon   *tray,
                               GParamSpec    *pspec,
                               CtkStatusIcon *status_icon)
{
  CtkStatusIconPrivate *priv = status_icon->priv;
  const gchar *name;

  switch (pspec->name[0])
    {
    case 'e':
      name = "error";
      break;
    case 'w':
      name = "warning";
      break;
    case 's':
      name = "success";
      break;
    default:
      name = NULL;
      break;
    }

  if (name)
    {
      CdkRGBA rgba;

      g_object_get (priv->tray_icon, pspec->name, &rgba, NULL);

      rgba.alpha = 1;

      ctk_widget_override_symbolic_color (priv->image, name, &rgba);
    }
}

static gboolean
ctk_status_icon_key_press (CtkStatusIcon  *status_icon,
			   CdkEventKey    *event)
{
  guint state, keyval;

  state = event->state & ctk_accelerator_get_default_mod_mask ();
  keyval = event->keyval;
  if (state == 0 &&
      (keyval == CDK_KEY_Return ||
       keyval == CDK_KEY_KP_Enter ||
       keyval == CDK_KEY_ISO_Enter ||
       keyval == CDK_KEY_space ||
       keyval == CDK_KEY_KP_Space))
    {
      emit_activate_signal (status_icon);
      return TRUE;
    }

  return FALSE;
}

static void
ctk_status_icon_popup_menu (CtkStatusIcon  *status_icon)
{
  emit_popup_menu_signal (status_icon, 0, ctk_get_current_event_time ());
}

#endif  /* CDK_WINDOWING_X11 */

static gboolean
ctk_status_icon_button_press (CtkStatusIcon  *status_icon,
			      CdkEventButton *event)
{
  gboolean handled = FALSE;

  g_signal_emit (status_icon,
		 status_icon_signals [BUTTON_PRESS_EVENT_SIGNAL], 0,
		 event, &handled);
  if (handled)
    return TRUE;

  if (cdk_event_triggers_context_menu ((CdkEvent *) event))
    {
      emit_popup_menu_signal (status_icon, event->button, event->time);
      return TRUE;
    }
  else if (event->button == CDK_BUTTON_PRIMARY && event->type == CDK_BUTTON_PRESS)
    {
      emit_activate_signal (status_icon);
      return TRUE;
    }

  return FALSE;
}

static gboolean
ctk_status_icon_button_release (CtkStatusIcon  *status_icon,
				CdkEventButton *event)
{
  gboolean handled = FALSE;
  g_signal_emit (status_icon,
		 status_icon_signals [BUTTON_RELEASE_EVENT_SIGNAL], 0,
		 event, &handled);
  return handled;
}

#ifdef CDK_WINDOWING_X11

static gboolean
ctk_status_icon_scroll (CtkStatusIcon  *status_icon,
			CdkEventScroll *event)
{
  gboolean handled = FALSE;
  g_signal_emit (status_icon,
		 status_icon_signals [SCROLL_EVENT_SIGNAL], 0,
		 event, &handled);
  return handled;
}

static gboolean
ctk_status_icon_query_tooltip (CtkStatusIcon *status_icon,
			       gint           x,
			       gint           y,
			       gboolean       keyboard_tip,
			       CtkTooltip    *tooltip)
{
  gboolean handled = FALSE;
  g_signal_emit (status_icon,
		 status_icon_signals [QUERY_TOOLTIP_SIGNAL], 0,
		 x, y, keyboard_tip, tooltip, &handled);
  return handled;
}

#endif /* CDK_WINDOWING_X11 */

static void
ctk_status_icon_reset_image_data (CtkStatusIcon *status_icon)
{
  CtkStatusIconPrivate *priv = status_icon->priv;
  CtkImageType storage_type = ctk_image_definition_get_storage_type (priv->image_def);

  switch (storage_type)
  {
    case CTK_IMAGE_PIXBUF:
      g_object_notify (G_OBJECT (status_icon), "pixbuf");
      break;
    case CTK_IMAGE_STOCK:
      g_object_notify (G_OBJECT (status_icon), "stock");
      break;    
    case CTK_IMAGE_ICON_NAME:
      g_object_notify (G_OBJECT (status_icon), "icon-name");
      break;
    case CTK_IMAGE_GICON:
      g_object_notify (G_OBJECT (status_icon), "gicon");
      break;
    case CTK_IMAGE_EMPTY:
      break;
    default:
      g_assert_not_reached ();
      break;
  }

  ctk_image_definition_unref (priv->image_def);
  priv->image_def = ctk_image_definition_new_empty ();
  g_object_notify (G_OBJECT (status_icon), "storage-type");
}

static void
ctk_status_icon_take_image (CtkStatusIcon      *status_icon,
                            CtkImageDefinition *def)
{
  CtkStatusIconPrivate *priv = status_icon->priv;

  g_object_freeze_notify (G_OBJECT (status_icon));

  ctk_status_icon_reset_image_data (status_icon);

  g_object_notify (G_OBJECT (status_icon), "storage-type");

  if (def != NULL)
    {
      ctk_image_definition_unref (priv->image_def);
      priv->image_def = def;
      /* the icon size we pass here doesn't really matter, since
       * we force a pixel size before doing the actual rendering anyway.
       */
      switch (ctk_image_definition_get_storage_type (def))
        {
        case CTK_IMAGE_PIXBUF:
          g_object_notify (G_OBJECT (status_icon), "pixbuf");
          break;
        case CTK_IMAGE_STOCK:
          g_object_notify (G_OBJECT (status_icon), "stock");
          break;
        case CTK_IMAGE_ICON_NAME:
          g_object_notify (G_OBJECT (status_icon), "icon-name");
          break;
        case CTK_IMAGE_GICON:
          g_object_notify (G_OBJECT (status_icon), "gicon");
          break;
        default:
          g_warning ("Image type %u not handled by CtkStatusIcon", 
                     ctk_image_definition_get_storage_type (def));
        }
    }

  g_object_thaw_notify (G_OBJECT (status_icon));

  ctk_status_icon_update_image (status_icon);
}

/**
 * ctk_status_icon_set_from_pixbuf:
 * @status_icon: a #CtkStatusIcon
 * @pixbuf: (allow-none): a #GdkPixbuf or %NULL
 *
 * Makes @status_icon display @pixbuf.
 * See ctk_status_icon_new_from_pixbuf() for details.
 *
 * Since: 2.10
 *
 * Deprecated: 3.14: Use #GNotification and #CtkApplication to
 *   provide status notifications; you can use g_notification_set_icon()
 *   to associate a #GIcon with a notification
 */
void
ctk_status_icon_set_from_pixbuf (CtkStatusIcon *status_icon,
				 GdkPixbuf     *pixbuf)
{
  g_return_if_fail (CTK_IS_STATUS_ICON (status_icon));
  g_return_if_fail (pixbuf == NULL || CDK_IS_PIXBUF (pixbuf));

  ctk_status_icon_take_image (status_icon,
      			      ctk_image_definition_new_pixbuf (pixbuf, 1));
}

/**
 * ctk_status_icon_set_from_file:
 * @status_icon: a #CtkStatusIcon
 * @filename: (type filename): a filename
 * 
 * Makes @status_icon display the file @filename.
 * See ctk_status_icon_new_from_file() for details.
 *
 * Since: 2.10 
 *
 * Deprecated: 3.14: Use #GNotification and #CtkApplication to
 *   provide status notifications; you can use g_notification_set_icon()
 *   to associate a #GIcon with a notification
 */
void
ctk_status_icon_set_from_file (CtkStatusIcon *status_icon,
 			       const gchar   *filename)
{
  GdkPixbuf *pixbuf;
  
  g_return_if_fail (CTK_IS_STATUS_ICON (status_icon));
  g_return_if_fail (filename != NULL);
  
  pixbuf = gdk_pixbuf_new_from_file (filename, NULL);
 
  ctk_status_icon_set_from_pixbuf (status_icon, pixbuf);
  
  if (pixbuf)
    g_object_unref (pixbuf);
}

/**
 * ctk_status_icon_set_from_stock:
 * @status_icon: a #CtkStatusIcon
 * @stock_id: a stock icon id
 * 
 * Makes @status_icon display the stock icon with the id @stock_id.
 * See ctk_status_icon_new_from_stock() for details.
 *
 * Since: 2.10
 *
 * Deprecated: 3.10: Use ctk_status_icon_set_from_icon_name() instead.
 **/
void
ctk_status_icon_set_from_stock (CtkStatusIcon *status_icon,
				const gchar   *stock_id)
{
  g_return_if_fail (CTK_IS_STATUS_ICON (status_icon));
  g_return_if_fail (stock_id != NULL);

  ctk_status_icon_take_image (status_icon,
      			      ctk_image_definition_new_stock (stock_id));
}

/**
 * ctk_status_icon_set_from_icon_name:
 * @status_icon: a #CtkStatusIcon
 * @icon_name: an icon name
 * 
 * Makes @status_icon display the icon named @icon_name from the 
 * current icon theme.
 * See ctk_status_icon_new_from_icon_name() for details.
 *
 * Since: 2.10 
 *
 * Deprecated: 3.14: Use #GNotification and #CtkApplication to
 *   provide status notifications; you can use g_notification_set_icon()
 *   to associate a #GIcon with a notification
 */
void
ctk_status_icon_set_from_icon_name (CtkStatusIcon *status_icon,
				    const gchar   *icon_name)
{
  g_return_if_fail (CTK_IS_STATUS_ICON (status_icon));
  g_return_if_fail (icon_name != NULL);

  ctk_status_icon_take_image (status_icon,
      			      ctk_image_definition_new_icon_name (icon_name));
}

/**
 * ctk_status_icon_set_from_gicon:
 * @status_icon: a #CtkStatusIcon
 * @icon: a GIcon
 *
 * Makes @status_icon display the #GIcon.
 * See ctk_status_icon_new_from_gicon() for details.
 *
 * Since: 2.14
 *
 * Deprecated: 3.14: Use #GNotification and #CtkApplication to
 *   provide status notifications; you can use g_notification_set_icon()
 *   to associate a #GIcon with a notification
 */
void
ctk_status_icon_set_from_gicon (CtkStatusIcon *status_icon,
                                GIcon         *icon)
{
  g_return_if_fail (CTK_IS_STATUS_ICON (status_icon));
  g_return_if_fail (icon != NULL);

  ctk_status_icon_take_image (status_icon,
      			      ctk_image_definition_new_gicon (icon));
}

/**
 * ctk_status_icon_get_storage_type:
 * @status_icon: a #CtkStatusIcon
 * 
 * Gets the type of representation being used by the #CtkStatusIcon
 * to store image data. If the #CtkStatusIcon has no image data,
 * the return value will be %CTK_IMAGE_EMPTY. 
 * 
 * Returns: the image representation being used
 *
 * Since: 2.10
 *
 * Deprecated: 3.14: Use #GNotification and #CtkApplication to
 *   provide status notifications; there is no direct replacement
 *   for this function, and #GNotification only supports #GIcon
 *   instances
 */
CtkImageType
ctk_status_icon_get_storage_type (CtkStatusIcon *status_icon)
{
  g_return_val_if_fail (CTK_IS_STATUS_ICON (status_icon), CTK_IMAGE_EMPTY);

  return ctk_image_definition_get_storage_type (status_icon->priv->image_def);
}
/**
 * ctk_status_icon_get_pixbuf:
 * @status_icon: a #CtkStatusIcon
 * 
 * Gets the #GdkPixbuf being displayed by the #CtkStatusIcon.
 * The storage type of the status icon must be %CTK_IMAGE_EMPTY or
 * %CTK_IMAGE_PIXBUF (see ctk_status_icon_get_storage_type()).
 * The caller of this function does not own a reference to the
 * returned pixbuf.
 * 
 * Returns: (nullable) (transfer none): the displayed pixbuf,
 *     or %NULL if the image is empty.
 *
 * Since: 2.10
 *
 * Deprecated: 3.14: Use #GNotification and #CtkApplication to
 *   provide status notifications; there is no direct replacement
 *   for this function
 */
GdkPixbuf *
ctk_status_icon_get_pixbuf (CtkStatusIcon *status_icon)
{
  CtkStatusIconPrivate *priv;

  g_return_val_if_fail (CTK_IS_STATUS_ICON (status_icon), NULL);

  priv = status_icon->priv;

  return ctk_image_definition_get_pixbuf (priv->image_def);
}

/**
 * ctk_status_icon_get_stock:
 * @status_icon: a #CtkStatusIcon
 * 
 * Gets the id of the stock icon being displayed by the #CtkStatusIcon.
 * The storage type of the status icon must be %CTK_IMAGE_EMPTY or
 * %CTK_IMAGE_STOCK (see ctk_status_icon_get_storage_type()).
 * The returned string is owned by the #CtkStatusIcon and should not
 * be freed or modified.
 * 
 * Returns: (nullable): stock id of the displayed stock icon,
 *   or %NULL if the image is empty.
 *
 * Since: 2.10
 *
 * Deprecated: 3.10: Use ctk_status_icon_get_icon_name() instead.
 **/
const gchar *
ctk_status_icon_get_stock (CtkStatusIcon *status_icon)
{
  CtkStatusIconPrivate *priv;

  g_return_val_if_fail (CTK_IS_STATUS_ICON (status_icon), NULL);

  priv = status_icon->priv;

  return ctk_image_definition_get_stock (priv->image_def);
}

/**
 * ctk_status_icon_get_icon_name:
 * @status_icon: a #CtkStatusIcon
 * 
 * Gets the name of the icon being displayed by the #CtkStatusIcon.
 * The storage type of the status icon must be %CTK_IMAGE_EMPTY or
 * %CTK_IMAGE_ICON_NAME (see ctk_status_icon_get_storage_type()).
 * The returned string is owned by the #CtkStatusIcon and should not
 * be freed or modified.
 * 
 * Returns: (nullable): name of the displayed icon, or %NULL if the image is empty.
 *
 * Since: 2.10
 *
 * Deprecated: 3.14: Use #GNotification and #CtkApplication to
 *   provide status notifications; there is no direct replacement
 *   for this function
 */
const gchar *
ctk_status_icon_get_icon_name (CtkStatusIcon *status_icon)
{
  CtkStatusIconPrivate *priv;
  
  g_return_val_if_fail (CTK_IS_STATUS_ICON (status_icon), NULL);

  priv = status_icon->priv;

  return ctk_image_definition_get_icon_name (priv->image_def);
}

/**
 * ctk_status_icon_get_gicon:
 * @status_icon: a #CtkStatusIcon
 *
 * Retrieves the #GIcon being displayed by the #CtkStatusIcon.
 * The storage type of the status icon must be %CTK_IMAGE_EMPTY or
 * %CTK_IMAGE_GICON (see ctk_status_icon_get_storage_type()).
 * The caller of this function does not own a reference to the
 * returned #GIcon.
 *
 * If this function fails, @icon is left unchanged;
 *
 * Returns: (nullable) (transfer none): the displayed icon, or %NULL if the image is empty
 *
 * Since: 2.14
 *
 * Deprecated: 3.14: Use #GNotification and #CtkApplication to
 *   provide status notifications; there is no direct replacement
 *   for this function
 */
GIcon *
ctk_status_icon_get_gicon (CtkStatusIcon *status_icon)
{
  CtkStatusIconPrivate *priv;

  g_return_val_if_fail (CTK_IS_STATUS_ICON (status_icon), NULL);

  priv = status_icon->priv;

  return ctk_image_definition_get_gicon (priv->image_def);
}

/**
 * ctk_status_icon_get_size:
 * @status_icon: a #CtkStatusIcon
 * 
 * Gets the size in pixels that is available for the image. 
 * Stock icons and named icons adapt their size automatically
 * if the size of the notification area changes. For other
 * storage types, the size-changed signal can be used to
 * react to size changes.
 *
 * Note that the returned size is only meaningful while the 
 * status icon is embedded (see ctk_status_icon_is_embedded()).
 * 
 * Returns: the size that is available for the image
 *
 * Since: 2.10
 *
 * Deprecated: 3.14: Use #GNotification and #CtkApplication to
 *   provide status notifications; there is no direct replacement
 *   for this function, as the representation of a notification
 *   is left to the platform
 */
gint
ctk_status_icon_get_size (CtkStatusIcon *status_icon)
{
  g_return_val_if_fail (CTK_IS_STATUS_ICON (status_icon), 0);

  return status_icon->priv->size;
}

/**
 * ctk_status_icon_set_screen:
 * @status_icon: a #CtkStatusIcon
 * @screen: a #CdkScreen
 *
 * Sets the #CdkScreen where @status_icon is displayed; if
 * the icon is already mapped, it will be unmapped, and
 * then remapped on the new screen.
 *
 * Since: 2.12
 *
 * Deprecated: 3.14: Use #GNotification and #CtkApplication to
 *   provide status notifications; there is no direct replacement
 *   for this function, as CTK typically only has one #CdkScreen
 *   and notifications are managed by the platform
 */
void
ctk_status_icon_set_screen (CtkStatusIcon *status_icon,
                            CdkScreen     *screen)
{
  g_return_if_fail (CDK_IS_SCREEN (screen));

#ifdef CDK_WINDOWING_X11
  if (status_icon->priv->tray_icon)
    ctk_window_set_screen (CTK_WINDOW (status_icon->priv->tray_icon), screen);
#endif
}

/**
 * ctk_status_icon_get_screen:
 * @status_icon: a #CtkStatusIcon
 *
 * Returns the #CdkScreen associated with @status_icon.
 *
 * Returns: (transfer none): a #CdkScreen.
 *
 * Since: 2.12
 *
 * Deprecated: 3.14: Use #GNotification and #CtkApplication to
 *   provide status notifications; there is no direct replacement
 *   for this function, as notifications are managed by the platform
 */
CdkScreen *
ctk_status_icon_get_screen (CtkStatusIcon *status_icon)
{
  g_return_val_if_fail (CTK_IS_STATUS_ICON (status_icon), NULL);

#ifdef CDK_WINDOWING_X11
  if (status_icon->priv->tray_icon)
    return ctk_window_get_screen (CTK_WINDOW (status_icon->priv->tray_icon));
  else
#endif
  return cdk_screen_get_default ();
}

/**
 * ctk_status_icon_set_visible:
 * @status_icon: a #CtkStatusIcon
 * @visible: %TRUE to show the status icon, %FALSE to hide it
 * 
 * Shows or hides a status icon.
 *
 * Since: 2.10
 *
 * Deprecated: 3.14: Use #GNotification and #CtkApplication to
 *   provide status notifications; there is no direct replacement
 *   for this function, as notifications are managed by the platform
 */
void
ctk_status_icon_set_visible (CtkStatusIcon *status_icon,
			     gboolean       visible)
{
  CtkStatusIconPrivate *priv;

  g_return_if_fail (CTK_IS_STATUS_ICON (status_icon));

  priv = status_icon->priv;

  visible = visible != FALSE;

  if (priv->visible != visible)
    {
      priv->visible = visible;

#ifdef CDK_WINDOWING_X11
      if (priv->tray_icon)
        {
          if (visible)
	    ctk_widget_show (priv->tray_icon);
          else if (ctk_widget_get_realized (priv->tray_icon))
            {
	      ctk_widget_hide (priv->tray_icon);
	      ctk_widget_unrealize (priv->tray_icon);
            }
        }
#endif
#ifdef CDK_WINDOWING_WIN32
      if (priv->nid.hWnd != NULL)
	{
	  if (visible)
	    Shell_NotifyIconW (NIM_ADD, &priv->nid);
	  else
	    Shell_NotifyIconW (NIM_DELETE, &priv->nid);
	}
#endif
#ifdef CDK_WINDOWING_QUARTZ
      QUARTZ_POOL_ALLOC;
      [priv->status_item setVisible:visible];
      QUARTZ_POOL_RELEASE;
#endif
      g_object_notify (G_OBJECT (status_icon), "visible");
    }
}

/**
 * ctk_status_icon_get_visible:
 * @status_icon: a #CtkStatusIcon
 * 
 * Returns whether the status icon is visible or not. 
 * Note that being visible does not guarantee that 
 * the user can actually see the icon, see also 
 * ctk_status_icon_is_embedded().
 * 
 * Returns: %TRUE if the status icon is visible
 *
 * Since: 2.10
 *
 * Deprecated: 3.14: Use #GNotification and #CtkApplication to
 *   provide status notifications; there is no direct replacement
 *   for this function
 */
gboolean
ctk_status_icon_get_visible (CtkStatusIcon *status_icon)
{
  g_return_val_if_fail (CTK_IS_STATUS_ICON (status_icon), FALSE);

  return status_icon->priv->visible;
}

/**
 * ctk_status_icon_is_embedded:
 * @status_icon: a #CtkStatusIcon
 * 
 * Returns whether the status icon is embedded in a notification
 * area. 
 * 
 * Returns: %TRUE if the status icon is embedded in
 *   a notification area.
 *
 * Since: 2.10
 *
 * Deprecated: 3.14: Use #GNotification and #CtkApplication to
 *   provide status notifications; there is no direct replacement
 *   for this function
 */
gboolean
ctk_status_icon_is_embedded (CtkStatusIcon *status_icon)
{
  g_return_val_if_fail (CTK_IS_STATUS_ICON (status_icon), FALSE);

#ifdef CDK_WINDOWING_X11
  if (status_icon->priv->tray_icon == NULL ||
      !ctk_plug_get_embedded (CTK_PLUG (status_icon->priv->tray_icon)))
    return FALSE;
#endif
  return TRUE;
}

/**
 * ctk_status_icon_position_menu:
 * @menu: the #CtkMenu
 * @x: (inout): return location for the x position
 * @y: (inout): return location for the y position
 * @push_in: (out): whether the first menu item should be offset
 *           (pushed in) to be aligned with the menu popup position
 *           (only useful for CtkOptionMenu).
 * @user_data: (type CtkStatusIcon): the status icon to position the menu on
 *
 * Menu positioning function to use with ctk_menu_popup()
 * to position @menu aligned to the status icon @user_data.
 * 
 * Since: 2.10
 *
 * Deprecated: 3.14: Use #GNotification and #CtkApplication to
 *   provide status notifications; notifications do not have menus,
 *   but can have buttons, and actions associated with each button
 */
void
ctk_status_icon_position_menu (CtkMenu  *menu,
			       gint     *x,
			       gint     *y,
			       gboolean *push_in,
			       gpointer  user_data)
{
#ifdef CDK_WINDOWING_X11
  CtkStatusIcon *status_icon = CTK_STATUS_ICON (user_data);
  CtkStatusIconPrivate *priv = status_icon->priv;
  CtkAllocation allocation;
  CtkTrayIcon *tray_icon;
  CtkWidget *widget;
  CdkScreen *screen;
  CtkTextDirection direction;
  CtkRequisition menu_req;
  CdkRectangle monitor;
  CdkWindow *window;
  gint monitor_num, height, width, xoffset, yoffset;

  g_return_if_fail (CTK_IS_MENU (menu));
  g_return_if_fail (CTK_IS_STATUS_ICON (user_data));

  if (priv->tray_icon == NULL)
    {
      *x = 0;
      *y = 0;
      return;
    }

  tray_icon = CTK_TRAY_ICON (priv->tray_icon);
  widget = priv->tray_icon;

  direction = ctk_widget_get_direction (widget);

  screen = ctk_widget_get_screen (widget);
  ctk_menu_set_screen (menu, screen);

  window = ctk_widget_get_window (widget);
  monitor_num = cdk_screen_get_monitor_at_window (screen, window);
  if (monitor_num < 0)
    monitor_num = 0;
  ctk_menu_set_monitor (menu, monitor_num);

  cdk_screen_get_monitor_workarea (screen, monitor_num, &monitor);

  cdk_window_get_origin (window, x, y);

  menu_req.width = ctk_widget_get_allocated_width (CTK_WIDGET (menu));
  menu_req.height = ctk_widget_get_allocated_height (CTK_WIDGET (menu));

  ctk_widget_get_allocation (widget, &allocation);
  if (_ctk_tray_icon_get_orientation (tray_icon) == CTK_ORIENTATION_VERTICAL)
    {
      width = 0;
      height = allocation.height;
      xoffset = allocation.width;
      yoffset = 0;
    }
  else
    {
      width = allocation.width;
      height = 0;
      xoffset = 0;
      yoffset = allocation.height;
    }

  if (direction == CTK_TEXT_DIR_RTL)
    {
      if ((*x - (menu_req.width - width)) >= monitor.x)
        *x -= menu_req.width - width;
      else if ((*x + xoffset + menu_req.width) < (monitor.x + monitor.width))
        *x += xoffset;
      else if ((monitor.x + monitor.width - (*x + xoffset)) < *x)
        *x -= menu_req.width - width;
      else
        *x += xoffset;
    }
  else
    {
      if ((*x + xoffset + menu_req.width) < (monitor.x + monitor.width))
        *x += xoffset;
      else if ((*x - (menu_req.width - width)) >= monitor.x)
        *x -= menu_req.width - width;
      else if ((monitor.x + monitor.width - (*x + xoffset)) > *x)
        *x += xoffset;
      else 
        *x -= menu_req.width - width;
    }

  if ((*y + yoffset + menu_req.height) < (monitor.y + monitor.height))
    *y += yoffset;
  else if ((*y - (menu_req.height - height)) >= monitor.y)
    *y -= menu_req.height - height;
  else if (monitor.y + monitor.height - (*y + yoffset) > *y)
    *y += yoffset;
  else 
    *y -= menu_req.height - height;

  *push_in = FALSE;
#endif /* CDK_WINDOWING_X11 */

#ifdef CDK_WINDOWING_WIN32
  CtkStatusIcon *status_icon;
  CtkStatusIconPrivate *priv;
  CtkRequisition menu_req;
  
  g_return_if_fail (CTK_IS_MENU (menu));
  g_return_if_fail (CTK_IS_STATUS_ICON (user_data));

  status_icon = CTK_STATUS_ICON (user_data);
  priv = status_icon->priv;

  ctk_widget_get_preferred_size (CTK_WIDGET (menu), &menu_req, NULL);

  *x = priv->last_click_x;
  *y = priv->taskbar_top - menu_req.height;

  *push_in = TRUE;
#endif
}

/**
 * ctk_status_icon_get_geometry:
 * @status_icon: a #CtkStatusIcon
 * @screen: (out) (transfer none) (allow-none): return location for
 *          the screen, or %NULL if the information is not needed
 * @area: (out) (allow-none): return location for the area occupied by
 *        the status icon, or %NULL
 * @orientation: (out) (allow-none): return location for the
 *    orientation of the panel in which the status icon is embedded,
 *    or %NULL. A panel at the top or bottom of the screen is
 *    horizontal, a panel at the left or right is vertical.
 *
 * Obtains information about the location of the status icon
 * on screen. This information can be used to e.g. position 
 * popups like notification bubbles. 
 *
 * See ctk_status_icon_position_menu() for a more convenient 
 * alternative for positioning menus.
 *
 * Note that some platforms do not allow CTK+ to provide 
 * this information, and even on platforms that do allow it,
 * the information is not reliable unless the status icon
 * is embedded in a notification area, see
 * ctk_status_icon_is_embedded().
 *
 * Returns: %TRUE if the location information has 
 *               been filled in
 *
 * Since: 2.10
 *
 * Deprecated: 3.14: Use #GNotification and #CtkApplication to
 *   provide status notifications; there is no direct replacement
 *   for this function, as the platform is responsible for the
 *   presentation of notifications
 */
gboolean
ctk_status_icon_get_geometry (CtkStatusIcon    *status_icon,
			      CdkScreen       **screen,
			      CdkRectangle     *area,
			      CtkOrientation   *orientation)
{
#ifdef CDK_WINDOWING_X11   
  CtkStatusIconPrivate *priv = status_icon->priv;
  CtkAllocation allocation;
  CtkWidget *widget;
  gint x, y;

  g_return_val_if_fail (CTK_IS_STATUS_ICON (status_icon), FALSE);

  if (priv->tray_icon == NULL)
    return FALSE;

  widget = priv->tray_icon;

  if (screen)
    *screen = ctk_widget_get_screen (widget);

  if (area)
    {
      cdk_window_get_origin (ctk_widget_get_window (widget),
                             &x, &y);

      ctk_widget_get_allocation (widget, &allocation);
      area->x = x;
      area->y = y;
      area->width = allocation.width;
      area->height = allocation.height;
    }

  if (orientation)
    *orientation = _ctk_tray_icon_get_orientation (CTK_TRAY_ICON (widget));

  return TRUE;
#else
  return FALSE;
#endif /* CDK_WINDOWING_X11 */
}

/**
 * ctk_status_icon_set_has_tooltip:
 * @status_icon: a #CtkStatusIcon
 * @has_tooltip: whether or not @status_icon has a tooltip
 *
 * Sets the has-tooltip property on @status_icon to @has_tooltip.
 * See #CtkStatusIcon:has-tooltip for more information.
 *
 * Since: 2.16
 *
 * Deprecated: 3.14: Use #GNotification and #CtkApplication to
 *   provide status notifications; there is no direct replacement
 *   for this function, but notifications can display an arbitrary
 *   amount of text using g_notification_set_body()
 */
void
ctk_status_icon_set_has_tooltip (CtkStatusIcon *status_icon,
				 gboolean       has_tooltip)
{
  CtkStatusIconPrivate *priv;
  gboolean changed = FALSE;

  g_return_if_fail (CTK_IS_STATUS_ICON (status_icon));

  priv = status_icon->priv;

#ifdef CDK_WINDOWING_X11
  if (priv->tray_icon)
    {
      if (ctk_widget_get_has_tooltip (priv->tray_icon) != has_tooltip)
        {
          ctk_widget_set_has_tooltip (priv->tray_icon, has_tooltip);
          changed = TRUE;
        }
    }
#endif
#ifdef CDK_WINDOWING_WIN32
  changed = TRUE;
  if (!has_tooltip && priv->tooltip_text)
    ctk_status_icon_set_tooltip_text (status_icon, NULL);
#endif
#ifdef CDK_WINDOWING_QUARTZ
  changed = TRUE;
  if (!has_tooltip && priv->tooltip_text)
    ctk_status_icon_set_tooltip_text (status_icon, NULL);
#endif

  if (changed)
    g_object_notify (G_OBJECT (status_icon), "has-tooltip");
}

/**
 * ctk_status_icon_get_has_tooltip:
 * @status_icon: a #CtkStatusIcon
 *
 * Returns the current value of the has-tooltip property.
 * See #CtkStatusIcon:has-tooltip for more information.
 *
 * Returns: current value of has-tooltip on @status_icon.
 *
 * Since: 2.16
 *
 * Deprecated: 3.14: Use #GNotification and #CtkApplication to
 *   provide status notifications; there is no direct replacement
 *   for this function
 */
gboolean
ctk_status_icon_get_has_tooltip (CtkStatusIcon *status_icon)
{
  CtkStatusIconPrivate *priv;
  gboolean has_tooltip = FALSE;

  g_return_val_if_fail (CTK_IS_STATUS_ICON (status_icon), FALSE);

  priv = status_icon->priv;

#ifdef CDK_WINDOWING_X11
  if (priv->tray_icon)
    has_tooltip = ctk_widget_get_has_tooltip (priv->tray_icon);
#endif
#ifdef CDK_WINDOWING_WIN32
  has_tooltip = (priv->tooltip_text != NULL);
#endif
#ifdef CDK_WINDOWING_QUARTZ
  has_tooltip = (priv->tooltip_text != NULL);
#endif

  return has_tooltip;
}

/**
 * ctk_status_icon_set_tooltip_text:
 * @status_icon: a #CtkStatusIcon
 * @text: the contents of the tooltip for @status_icon
 *
 * Sets @text as the contents of the tooltip.
 *
 * This function will take care of setting #CtkStatusIcon:has-tooltip to
 * %TRUE and of the default handler for the #CtkStatusIcon::query-tooltip
 * signal.
 *
 * See also the #CtkStatusIcon:tooltip-text property and
 * ctk_tooltip_set_text().
 *
 * Since: 2.16
 *
 * Deprecated: 3.14: Use #GNotification and #CtkApplication to
 *   provide status notifications; there is no direct replacement
 *   for this function
 */
void
ctk_status_icon_set_tooltip_text (CtkStatusIcon *status_icon,
				  const gchar   *text)
{
  CtkStatusIconPrivate *priv;

  g_return_if_fail (CTK_IS_STATUS_ICON (status_icon));

  priv = status_icon->priv;

#ifdef CDK_WINDOWING_X11
  if (priv->tray_icon)
    ctk_widget_set_tooltip_text (priv->tray_icon, text);
#endif
#ifdef CDK_WINDOWING_WIN32
  if (text == NULL)
    priv->nid.uFlags &= ~NIF_TIP;
  else
    {
      WCHAR *wcs = g_utf8_to_utf16 (text, -1, NULL, NULL, NULL);

      priv->nid.uFlags |= NIF_TIP;
      wcsncpy (priv->nid.szTip, wcs, G_N_ELEMENTS (priv->nid.szTip) - 1);
      priv->nid.szTip[G_N_ELEMENTS (priv->nid.szTip) - 1] = 0;
      g_free (wcs);
    }
  if (priv->nid.hWnd != NULL && priv->visible)
    if (!Shell_NotifyIconW (NIM_MODIFY, &priv->nid))
      g_warning (G_STRLOC ": Shell_NotifyIconW(NIM_MODIFY) failed");

  g_free (priv->tooltip_text);
  priv->tooltip_text = g_strdup (text);
#endif
#ifdef CDK_WINDOWING_QUARTZ
  QUARTZ_POOL_ALLOC;
  [priv->status_item setToolTip:text];
  QUARTZ_POOL_RELEASE;

  g_free (priv->tooltip_text);
  priv->tooltip_text = g_strdup (text);
#endif
}

/**
 * ctk_status_icon_get_tooltip_text:
 * @status_icon: a #CtkStatusIcon
 *
 * Gets the contents of the tooltip for @status_icon.
 *
 * Returns: (nullable): the tooltip text, or %NULL. You should free the
 *   returned string with g_free() when done.
 *
 * Since: 2.16
 *
 * Deprecated: 3.14: Use #GNotification and #CtkApplication to
 *   provide status notifications; there is no direct replacement
 *   for this function
 */
gchar *
ctk_status_icon_get_tooltip_text (CtkStatusIcon *status_icon)
{
  CtkStatusIconPrivate *priv;
  gchar *tooltip_text = NULL;

  g_return_val_if_fail (CTK_IS_STATUS_ICON (status_icon), NULL);

  priv = status_icon->priv;

#ifdef CDK_WINDOWING_X11
  if (priv->tray_icon)
    tooltip_text = ctk_widget_get_tooltip_text (priv->tray_icon);
#endif
#ifdef CDK_WINDOWING_WIN32
  if (priv->tooltip_text)
    tooltip_text = g_strdup (priv->tooltip_text);
#endif
#ifdef CDK_WINDOWING_QUARTZ
  if (priv->tooltip_text)
    tooltip_text = g_strdup (priv->tooltip_text);
#endif

  return tooltip_text;
}

/**
 * ctk_status_icon_set_tooltip_markup:
 * @status_icon: a #CtkStatusIcon
 * @markup: (allow-none): the contents of the tooltip for @status_icon, or %NULL
 *
 * Sets @markup as the contents of the tooltip, which is marked up with
 *  the [Pango text markup language][PangoMarkupFormat].
 *
 * This function will take care of setting #CtkStatusIcon:has-tooltip to %TRUE
 * and of the default handler for the #CtkStatusIcon::query-tooltip signal.
 *
 * See also the #CtkStatusIcon:tooltip-markup property and
 * ctk_tooltip_set_markup().
 *
 * Since: 2.16
 *
 * Deprecated: 3.14: Use #GNotification and #CtkApplication to
 *   provide status notifications; there is no direct replacement
 *   for this function
 */
void
ctk_status_icon_set_tooltip_markup (CtkStatusIcon *status_icon,
				    const gchar   *markup)
{
#ifdef CDK_WINDOWING_X11
  CtkStatusIconPrivate *priv;
#endif
#if defined (CDK_WINDOWING_WIN32) || defined (CDK_WINDOWING_QUARTZ)
  gchar *text = NULL;
#endif

  g_return_if_fail (CTK_IS_STATUS_ICON (status_icon));

#ifdef CDK_WINDOWING_X11
  priv = status_icon->priv;

  if (priv->tray_icon)
    ctk_widget_set_tooltip_markup (priv->tray_icon, markup);
#endif
#ifdef CDK_WINDOWING_WIN32
  if (markup)
    pango_parse_markup (markup, -1, 0, NULL, &text, NULL, NULL);
  ctk_status_icon_set_tooltip_text (status_icon, text);
  g_free (text);
#endif
#ifdef CDK_WINDOWING_QUARTZ
  if (markup)
    pango_parse_markup (markup, -1, 0, NULL, &text, NULL, NULL);
  ctk_status_icon_set_tooltip_text (status_icon, text);
  g_free (text);
#endif
}

/**
 * ctk_status_icon_get_tooltip_markup:
 * @status_icon: a #CtkStatusIcon
 *
 * Gets the contents of the tooltip for @status_icon.
 *
 * Returns: (nullable): the tooltip text, or %NULL. You should free the
 *   returned string with g_free() when done.
 *
 * Since: 2.16
 *
 * Deprecated: 3.14: Use #GNotification and #CtkApplication to
 *   provide status notifications; there is no direct replacement
 *   for this function
 */
gchar *
ctk_status_icon_get_tooltip_markup (CtkStatusIcon *status_icon)
{
  CtkStatusIconPrivate *priv;
  gchar *markup = NULL;

  g_return_val_if_fail (CTK_IS_STATUS_ICON (status_icon), NULL);

  priv = status_icon->priv;

#ifdef CDK_WINDOWING_X11
  if (priv->tray_icon)
    markup = ctk_widget_get_tooltip_markup (priv->tray_icon);
#endif
#ifdef CDK_WINDOWING_WIN32
  if (priv->tooltip_text)
    markup = g_markup_escape_text (priv->tooltip_text, -1);
#endif
#ifdef CDK_WINDOWING_QUARTZ
  if (priv->tooltip_text)
    markup = g_markup_escape_text (priv->tooltip_text, -1);
#endif

  return markup;
}

/**
 * ctk_status_icon_get_x11_window_id:
 * @status_icon: a #CtkStatusIcon
 *
 * This function is only useful on the X11/freedesktop.org platform.
 *
 * It returns a window ID for the widget in the underlying
 * status icon implementation.  This is useful for the Galago 
 * notification service, which can send a window ID in the protocol 
 * in order for the server to position notification windows 
 * pointing to a status icon reliably.
 *
 * This function is not intended for other use cases which are
 * more likely to be met by one of the non-X11 specific methods, such
 * as ctk_status_icon_position_menu().
 *
 * Returns: An 32 bit unsigned integer identifier for the 
 * underlying X11 Window
 *
 * Since: 2.14
 *
 * Deprecated: 3.14: Use #GNotification and #CtkApplication to
 *   provide status notifications; there is no direct replacement
 *   for this function
 */
guint32
ctk_status_icon_get_x11_window_id (CtkStatusIcon *status_icon)
{
#ifdef CDK_WINDOWING_X11
  if (status_icon->priv->tray_icon)
    {
      ctk_widget_realize (CTK_WIDGET (status_icon->priv->tray_icon));
      return CDK_WINDOW_XID (ctk_widget_get_window (CTK_WIDGET (status_icon->priv->tray_icon)));
    }
  else
#endif
  return 0;
}

/**
 * ctk_status_icon_set_title:
 * @status_icon: a #CtkStatusIcon
 * @title: the title 
 *
 * Sets the title of this tray icon.
 * This should be a short, human-readable, localized string 
 * describing the tray icon. It may be used by tools like screen
 * readers to render the tray icon.
 *
 * Since: 2.18
 *
 * Deprecated: 3.14: Use #GNotification and #CtkApplication to
 *   provide status notifications; you should use g_notification_set_title()
 *   and g_notification_set_body() to present text inside your notification
 */
void
ctk_status_icon_set_title (CtkStatusIcon *status_icon,
                           const gchar   *title)
{
  CtkStatusIconPrivate *priv;

  g_return_if_fail (CTK_IS_STATUS_ICON (status_icon));

  priv = status_icon->priv;

#ifdef CDK_WINDOWING_X11
  if (priv->tray_icon)
    ctk_window_set_title (CTK_WINDOW (priv->tray_icon), title);
#endif
#ifdef CDK_WINDOWING_QUARTZ
  g_free (priv->title);
  priv->title = g_strdup (title);
#endif
#ifdef CDK_WINDOWING_WIN32
  g_free (priv->title);
  priv->title = g_strdup (title);
#endif

  g_object_notify (G_OBJECT (status_icon), "title");
}

/**
 * ctk_status_icon_get_title:
 * @status_icon: a #CtkStatusIcon
 *
 * Gets the title of this tray icon. See ctk_status_icon_set_title().
 *
 * Returns: the title of the status icon
 *
 * Since: 2.18
 *
 * Deprecated: 3.14: Use #GNotification and #CtkApplication to
 *   provide status notifications; there is no direct replacement
 *   for this function
 */
const gchar *
ctk_status_icon_get_title (CtkStatusIcon *status_icon)
{
  CtkStatusIconPrivate *priv;
  const gchar *title = NULL;

  g_return_val_if_fail (CTK_IS_STATUS_ICON (status_icon), NULL);

  priv = status_icon->priv;

#ifdef CDK_WINDOWING_X11
  if (priv->tray_icon)
    title = ctk_window_get_title (CTK_WINDOW (priv->tray_icon));
#endif
#ifdef CDK_WINDOWING_QUARTZ
  title = priv->title;
#endif
#ifdef CDK_WINDOWING_WIN32
  title = priv->title;
#endif

 return title;
}


/**
 * ctk_status_icon_set_name:
 * @status_icon: a #CtkStatusIcon
 * @name: the name
 *
 * Sets the name of this tray icon.
 * This should be a string identifying this icon. It is may be
 * used for sorting the icons in the tray and will not be shown to
 * the user.
 *
 * Since: 2.20
 *
 * Deprecated: 3.14: Use #GNotification and #CtkApplication to
 *   provide status notifications; there is no direct replacement
 *   for this function, as notifications are associated with a
 *   unique application identifier by #GApplication
 */
void
ctk_status_icon_set_name (CtkStatusIcon *status_icon,
                          const gchar   *name)
{
#ifdef CDK_WINDOWING_X11
  CtkStatusIconPrivate *priv;
#endif

  g_return_if_fail (CTK_IS_STATUS_ICON (status_icon));

#ifdef CDK_WINDOWING_X11
  priv = status_icon->priv;

  if (priv->tray_icon)
    {
      if (ctk_widget_get_realized (priv->tray_icon))
        {
          /* ctk_window_set_wmclass() only operates on non-realized windows,
           * so temporarily unrealize the tray here
           */
          ctk_widget_hide (priv->tray_icon);
          ctk_widget_unrealize (priv->tray_icon);
          ctk_window_set_wmclass (CTK_WINDOW (priv->tray_icon), name, name);
          ctk_widget_show (priv->tray_icon);
        }
      else
        ctk_window_set_wmclass (CTK_WINDOW (priv->tray_icon), name, name);
    }
#endif
}
