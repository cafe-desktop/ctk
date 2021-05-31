/* GTK - The GIMP Toolkit
 * ctklinkbutton.c - an hyperlink-enabled button
 *
 * Copyright (C) 2006 Emmanuele Bassi <ebassi@gmail.com>
 * All rights reserved.
 *
 * Based on gnome-href code by:
 *      James Henstridge <james@daa.com.au>
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
 */

/**
 * SECTION:ctklinkbutton
 * @Title: GtkLinkButton
 * @Short_description: Create buttons bound to a URL
 * @See_also: #GtkButton
 *
 * A GtkLinkButton is a #GtkButton with a hyperlink, similar to the one
 * used by web browsers, which triggers an action when clicked. It is useful
 * to show quick links to resources.
 *
 * A link button is created by calling either ctk_link_button_new() or
 * ctk_link_button_new_with_label(). If using the former, the URI you pass
 * to the constructor is used as a label for the widget.
 *
 * The URI bound to a GtkLinkButton can be set specifically using
 * ctk_link_button_set_uri(), and retrieved using ctk_link_button_get_uri().
 *
 * By default, GtkLinkButton calls ctk_show_uri_on_window() when the button is
 * clicked. This behaviour can be overridden by connecting to the
 * #GtkLinkButton::activate-link signal and returning %TRUE from the
 * signal handler.
 *
 * # CSS nodes
 *
 * GtkLinkButton has a single CSS node with name button. To differentiate
 * it from a plain #GtkButton, it gets the .link style class.
 */

#include "config.h"

#include "ctklinkbutton.h"

#include <string.h>

#include "ctkclipboard.h"
#include "ctkdnd.h"
#include "ctklabel.h"
#include "ctkmain.h"
#include "ctkmarshalers.h"
#include "ctkmenu.h"
#include "ctkmenuitem.h"
#include "ctksizerequest.h"
#include "ctkshow.h"
#include "ctktooltip.h"
#include "ctkprivate.h"
#include "ctkintl.h"
#include "ctktextutil.h"

#include "a11y/ctklinkbuttonaccessible.h"

struct _GtkLinkButtonPrivate
{
  gchar *uri;

  gboolean visited;

  GtkWidget *popup_menu;
};

enum
{
  PROP_0,
  PROP_URI,
  PROP_VISITED
};

enum
{
  ACTIVATE_LINK,

  LAST_SIGNAL
};

static void     ctk_link_button_finalize     (GObject          *object);
static void     ctk_link_button_get_property (GObject          *object,
					      guint             prop_id,
					      GValue           *value,
					      GParamSpec       *pspec);
static void     ctk_link_button_set_property (GObject          *object,
					      guint             prop_id,
					      const GValue     *value,
					      GParamSpec       *pspec);
static gboolean ctk_link_button_button_press (GtkWidget        *widget,
					      GdkEventButton   *event);
static void     ctk_link_button_clicked      (GtkButton        *button);
static gboolean ctk_link_button_popup_menu   (GtkWidget        *widget);
static void     ctk_link_button_realize      (GtkWidget        *widget);
static void     ctk_link_button_unrealize    (GtkWidget        *widget);
static void     ctk_link_button_drag_begin   (GtkWidget        *widget,
                                              GdkDragContext   *context);
static void ctk_link_button_drag_data_get_cb (GtkWidget        *widget,
					      GdkDragContext   *context,
					      GtkSelectionData *selection,
					      guint             _info,
					      guint             _time,
					      gpointer          user_data);
static gboolean ctk_link_button_query_tooltip_cb (GtkWidget    *widget,
                                                  gint          x,
                                                  gint          y,
                                                  gboolean      keyboard_tip,
                                                  GtkTooltip   *tooltip,
                                                  gpointer      data);
static gboolean ctk_link_button_activate_link (GtkLinkButton *link_button);

static const GtkTargetEntry link_drop_types[] = {
  { "text/uri-list", 0, 0 },
  { "_NETSCAPE_URL", 0, 0 }
};

static guint link_signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE_WITH_PRIVATE (GtkLinkButton, ctk_link_button, CTK_TYPE_BUTTON)

static void
ctk_link_button_class_init (GtkLinkButtonClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);
  GtkButtonClass *button_class = CTK_BUTTON_CLASS (klass);

  gobject_class->set_property = ctk_link_button_set_property;
  gobject_class->get_property = ctk_link_button_get_property;
  gobject_class->finalize = ctk_link_button_finalize;

  widget_class->button_press_event = ctk_link_button_button_press;
  widget_class->popup_menu = ctk_link_button_popup_menu;
  widget_class->realize = ctk_link_button_realize;
  widget_class->unrealize = ctk_link_button_unrealize;
  widget_class->drag_begin = ctk_link_button_drag_begin;

  button_class->clicked = ctk_link_button_clicked;

  klass->activate_link = ctk_link_button_activate_link;

  /**
   * GtkLinkButton:uri:
   *
   * The URI bound to this button.
   *
   * Since: 2.10
   */
  g_object_class_install_property (gobject_class,
                                   PROP_URI,
                                   g_param_spec_string ("uri",
                                                        P_("URI"),
                                                        P_("The URI bound to this button"),
                                                        NULL,
                                                        CTK_PARAM_READWRITE));

  /**
   * GtkLinkButton:visited:
   *
   * The 'visited' state of this button. A visited link is drawn in a
   * different color.
   *
   * Since: 2.14
   */
  g_object_class_install_property (gobject_class,
                                   PROP_VISITED,
                                   g_param_spec_boolean ("visited",
                                                         P_("Visited"),
                                                         P_("Whether this link has been visited."),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * GtkLinkButton::activate-link:
   * @button: the #GtkLinkButton that emitted the signal
   *
   * The ::activate-link signal is emitted each time the #GtkLinkButton
   * has been clicked.
   *
   * The default handler will call ctk_show_uri_on_window() with the URI stored inside
   * the #GtkLinkButton:uri property.
   *
   * To override the default behavior, you can connect to the ::activate-link
   * signal and stop the propagation of the signal by returning %TRUE from
   * your handler.
   */
  link_signals[ACTIVATE_LINK] =
    g_signal_new (I_("activate-link"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GtkLinkButtonClass, activate_link),
                  _ctk_boolean_handled_accumulator, NULL,
                  _ctk_marshal_BOOLEAN__VOID,
                  G_TYPE_BOOLEAN, 0);

  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_LINK_BUTTON_ACCESSIBLE);
  ctk_widget_class_set_css_name (widget_class, "button");
}

static void
ctk_link_button_init (GtkLinkButton *link_button)
{
  GtkStyleContext *context;

  link_button->priv = ctk_link_button_get_instance_private (link_button);

  ctk_button_set_relief (CTK_BUTTON (link_button), CTK_RELIEF_NONE);
  ctk_widget_set_state_flags (CTK_WIDGET (link_button), CTK_STATE_FLAG_LINK, FALSE);

  g_signal_connect (link_button, "drag-data-get",
  		    G_CALLBACK (ctk_link_button_drag_data_get_cb), NULL);

  g_object_set (link_button, "has-tooltip", TRUE, NULL);
  g_signal_connect (link_button, "query-tooltip",
                    G_CALLBACK (ctk_link_button_query_tooltip_cb), NULL);

  /* enable drag source */
  ctk_drag_source_set (CTK_WIDGET (link_button),
  		       GDK_BUTTON1_MASK,
  		       link_drop_types, G_N_ELEMENTS (link_drop_types),
  		       GDK_ACTION_COPY);

  context = ctk_widget_get_style_context (CTK_WIDGET (link_button));
  ctk_style_context_add_class (context, "link");
}

static void
ctk_link_button_finalize (GObject *object)
{
  GtkLinkButton *link_button = CTK_LINK_BUTTON (object);
  
  g_free (link_button->priv->uri);
  
  G_OBJECT_CLASS (ctk_link_button_parent_class)->finalize (object);
}

static void
ctk_link_button_get_property (GObject    *object,
			      guint       prop_id,
			      GValue     *value,
			      GParamSpec *pspec)
{
  GtkLinkButton *link_button = CTK_LINK_BUTTON (object);
  
  switch (prop_id)
    {
    case PROP_URI:
      g_value_set_string (value, link_button->priv->uri);
      break;
    case PROP_VISITED:
      g_value_set_boolean (value, link_button->priv->visited);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_link_button_set_property (GObject      *object,
			      guint         prop_id,
			      const GValue *value,
			      GParamSpec   *pspec)
{
  GtkLinkButton *link_button = CTK_LINK_BUTTON (object);
  
  switch (prop_id)
    {
    case PROP_URI:
      ctk_link_button_set_uri (link_button, g_value_get_string (value));
      break;
    case PROP_VISITED:
      ctk_link_button_set_visited (link_button, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
set_hand_cursor (GtkWidget *widget,
		 gboolean   show_hand)
{
  GdkDisplay *display;
  GdkCursor *cursor;

  display = ctk_widget_get_display (widget);

  cursor = NULL;
  if (show_hand)
    cursor = gdk_cursor_new_from_name (display, "pointer");

  gdk_window_set_cursor (ctk_button_get_event_window (CTK_BUTTON (widget)), cursor);
  gdk_display_flush (display);

  if (cursor)
    g_object_unref (cursor);
}

static void
ctk_link_button_realize (GtkWidget *widget)
{
  CTK_WIDGET_CLASS (ctk_link_button_parent_class)->realize (widget);

  set_hand_cursor (widget, TRUE);
}

static void
ctk_link_button_unrealize (GtkWidget *widget)
{
  set_hand_cursor (widget, FALSE);

  CTK_WIDGET_CLASS (ctk_link_button_parent_class)->unrealize (widget);
}

static void
popup_menu_detach (GtkWidget *attach_widget,
		   GtkMenu   *menu)
{
  GtkLinkButton *link_button = CTK_LINK_BUTTON (attach_widget);

  link_button->priv->popup_menu = NULL;
}

static void
copy_activate_cb (GtkWidget     *widget,
		  GtkLinkButton *link_button)
{
  GtkLinkButtonPrivate *priv = link_button->priv;
  
  ctk_clipboard_set_text (ctk_widget_get_clipboard (CTK_WIDGET (link_button),
			  			    GDK_SELECTION_CLIPBOARD),
		  	  priv->uri, -1);
}

static void
ctk_link_button_do_popup (GtkLinkButton  *link_button,
                          const GdkEvent *event)
{
  GtkLinkButtonPrivate *priv = link_button->priv;

  if (ctk_widget_get_realized (CTK_WIDGET (link_button)))
    {
      GtkWidget *menu_item;

      if (priv->popup_menu)
	ctk_widget_destroy (priv->popup_menu);

      priv->popup_menu = ctk_menu_new ();
      ctk_style_context_add_class (ctk_widget_get_style_context (priv->popup_menu),
                                   CTK_STYLE_CLASS_CONTEXT_MENU);

      ctk_menu_attach_to_widget (CTK_MENU (priv->popup_menu),
		      		 CTK_WIDGET (link_button),
				 popup_menu_detach);

      menu_item = ctk_menu_item_new_with_mnemonic (_("Copy URL"));
      g_signal_connect (menu_item, "activate",
		        G_CALLBACK (copy_activate_cb), link_button);
      ctk_widget_show (menu_item);
      ctk_menu_shell_append (CTK_MENU_SHELL (priv->popup_menu), menu_item);

      if (event && gdk_event_triggers_context_menu (event))
        ctk_menu_popup_at_pointer (CTK_MENU (priv->popup_menu), event);
      else
        {
          ctk_menu_popup_at_widget (CTK_MENU (priv->popup_menu),
                                    CTK_WIDGET (link_button),
                                    GDK_GRAVITY_SOUTH,
                                    GDK_GRAVITY_NORTH_WEST,
                                    event);

          ctk_menu_shell_select_first (CTK_MENU_SHELL (priv->popup_menu), FALSE);
        }
    }
}

static gboolean
ctk_link_button_button_press (GtkWidget      *widget,
			      GdkEventButton *event)
{
  if (!ctk_widget_has_focus (widget))
    ctk_widget_grab_focus (widget);

  /* Don't popup the menu if there's no URI set,
   * otherwise the menu item will trigger a warning */
  if (gdk_event_triggers_context_menu ((GdkEvent *) event) &&
      CTK_LINK_BUTTON (widget)->priv->uri != NULL)
    {
      ctk_link_button_do_popup (CTK_LINK_BUTTON (widget), (GdkEvent *) event);

      return TRUE;
    }

  if (CTK_WIDGET_CLASS (ctk_link_button_parent_class)->button_press_event)
    return CTK_WIDGET_CLASS (ctk_link_button_parent_class)->button_press_event (widget, event);
  
  return FALSE;
}

static gboolean
ctk_link_button_activate_link (GtkLinkButton *link_button)
{
  GtkWidget *toplevel;
  GError *error;

  toplevel = ctk_widget_get_toplevel (CTK_WIDGET (link_button));

  error = NULL;
  ctk_show_uri_on_window (CTK_WINDOW (toplevel), link_button->priv->uri, GDK_CURRENT_TIME, &error);
  if (error)
    {
      g_warning ("Unable to show '%s': %s",
                 link_button->priv->uri,
                 error->message);
      g_error_free (error);

      return FALSE;
    }

  ctk_link_button_set_visited (link_button, TRUE);

  return TRUE;
}

static void
ctk_link_button_clicked (GtkButton *button)
{
  gboolean retval = FALSE;

  g_signal_emit (button, link_signals[ACTIVATE_LINK], 0, &retval);
}

static gboolean
ctk_link_button_popup_menu (GtkWidget *widget)
{
  ctk_link_button_do_popup (CTK_LINK_BUTTON (widget), NULL);

  return TRUE; 
}

static void
ctk_link_button_drag_begin (GtkWidget      *widget,
                            GdkDragContext *context)
{
  ctk_drag_set_icon_name (context, "text-x-generic", 0, 0);
}

static void
ctk_link_button_drag_data_get_cb (GtkWidget        *widget,
				  GdkDragContext   *context,
				  GtkSelectionData *selection,
				  guint             _info,
				  guint             _time,
				  gpointer          user_data)
{
  GtkLinkButton *link_button = CTK_LINK_BUTTON (widget);
  gchar *uri;
  
  uri = g_strdup_printf ("%s\r\n", link_button->priv->uri);
  ctk_selection_data_set (selection,
                          ctk_selection_data_get_target (selection),
  			  8,
  			  (guchar *) uri,
			  strlen (uri));
  
  g_free (uri);
}

/**
 * ctk_link_button_new:
 * @uri: a valid URI
 *
 * Creates a new #GtkLinkButton with the URI as its text.
 *
 * Returns: a new link button widget.
 *
 * Since: 2.10
 */
GtkWidget *
ctk_link_button_new (const gchar *uri)
{
  gchar *utf8_uri = NULL;
  GtkWidget *retval;
  
  g_return_val_if_fail (uri != NULL, NULL);
  
  if (g_utf8_validate (uri, -1, NULL))
    {
      utf8_uri = g_strdup (uri);
    }
  else
    {
      GError *conv_err = NULL;
    
      utf8_uri = g_locale_to_utf8 (uri, -1, NULL, NULL, &conv_err);
      if (conv_err)
        {
          g_warning ("Attempting to convert URI '%s' to UTF-8, but failed "
                     "with error: %s",
                     uri,
                     conv_err->message);
          g_error_free (conv_err);
        
          utf8_uri = g_strdup (_("Invalid URI"));
        }
    }
  
  retval = g_object_new (CTK_TYPE_LINK_BUTTON,
  			 "label", utf8_uri,
  			 "uri", uri,
  			 NULL);
  
  g_free (utf8_uri);
  
  return retval;
}

/**
 * ctk_link_button_new_with_label:
 * @uri: a valid URI
 * @label: (allow-none): the text of the button
 *
 * Creates a new #GtkLinkButton containing a label.
 *
 * Returns: (transfer none): a new link button widget.
 *
 * Since: 2.10
 */
GtkWidget *
ctk_link_button_new_with_label (const gchar *uri,
				const gchar *label)
{
  GtkWidget *retval;
  
  g_return_val_if_fail (uri != NULL, NULL);
  
  if (!label)
    return ctk_link_button_new (uri);

  retval = g_object_new (CTK_TYPE_LINK_BUTTON,
		         "label", label,
			 "uri", uri,
			 NULL);

  return retval;
}

static gboolean 
ctk_link_button_query_tooltip_cb (GtkWidget    *widget,
                                  gint          x,
                                  gint          y,
                                  gboolean      keyboard_tip,
                                  GtkTooltip   *tooltip,
                                  gpointer      data)
{
  GtkLinkButton *link_button = CTK_LINK_BUTTON (widget);
  const gchar *label, *uri;
  gchar *text, *markup;

  label = ctk_button_get_label (CTK_BUTTON (link_button));
  uri = link_button->priv->uri;
  text = ctk_widget_get_tooltip_text (widget);
  markup = ctk_widget_get_tooltip_markup (widget);

  if (text == NULL &&
      markup == NULL &&
      label && *label != '\0' && uri && strcmp (label, uri) != 0)
    {
      ctk_tooltip_set_text (tooltip, uri);
      return TRUE;
    }

  g_free (text);
  g_free (markup);

  return FALSE;
}



/**
 * ctk_link_button_set_uri:
 * @link_button: a #GtkLinkButton
 * @uri: a valid URI
 *
 * Sets @uri as the URI where the #GtkLinkButton points. As a side-effect
 * this unsets the “visited” state of the button.
 *
 * Since: 2.10
 */
void
ctk_link_button_set_uri (GtkLinkButton *link_button,
			 const gchar   *uri)
{
  GtkLinkButtonPrivate *priv;

  g_return_if_fail (CTK_IS_LINK_BUTTON (link_button));
  g_return_if_fail (uri != NULL);

  priv = link_button->priv;

  g_free (priv->uri);
  priv->uri = g_strdup (uri);

  g_object_notify (G_OBJECT (link_button), "uri");

  ctk_link_button_set_visited (link_button, FALSE);
}

/**
 * ctk_link_button_get_uri:
 * @link_button: a #GtkLinkButton
 *
 * Retrieves the URI set using ctk_link_button_set_uri().
 *
 * Returns: a valid URI.  The returned string is owned by the link button
 *   and should not be modified or freed.
 *
 * Since: 2.10
 */
const gchar *
ctk_link_button_get_uri (GtkLinkButton *link_button)
{
  g_return_val_if_fail (CTK_IS_LINK_BUTTON (link_button), NULL);
  
  return link_button->priv->uri;
}

/**
 * ctk_link_button_set_visited:
 * @link_button: a #GtkLinkButton
 * @visited: the new “visited” state
 *
 * Sets the “visited” state of the URI where the #GtkLinkButton
 * points.  See ctk_link_button_get_visited() for more details.
 *
 * Since: 2.14
 */
void
ctk_link_button_set_visited (GtkLinkButton *link_button,
                             gboolean       visited)
{
  g_return_if_fail (CTK_IS_LINK_BUTTON (link_button));

  visited = visited != FALSE;

  if (link_button->priv->visited != visited)
    {
      link_button->priv->visited = visited;

      if (visited)
        {
          ctk_widget_unset_state_flags (CTK_WIDGET (link_button), CTK_STATE_FLAG_LINK);
          ctk_widget_set_state_flags (CTK_WIDGET (link_button), CTK_STATE_FLAG_VISITED, FALSE);
        }
      else
        {
          ctk_widget_unset_state_flags (CTK_WIDGET (link_button), CTK_STATE_FLAG_VISITED);
          ctk_widget_set_state_flags (CTK_WIDGET (link_button), CTK_STATE_FLAG_LINK, FALSE);
        }

      g_object_notify (G_OBJECT (link_button), "visited");
    }
}

/**
 * ctk_link_button_get_visited:
 * @link_button: a #GtkLinkButton
 *
 * Retrieves the “visited” state of the URI where the #GtkLinkButton
 * points. The button becomes visited when it is clicked. If the URI
 * is changed on the button, the “visited” state is unset again.
 *
 * The state may also be changed using ctk_link_button_set_visited().
 *
 * Returns: %TRUE if the link has been visited, %FALSE otherwise
 *
 * Since: 2.14
 */
gboolean
ctk_link_button_get_visited (GtkLinkButton *link_button)
{
  g_return_val_if_fail (CTK_IS_LINK_BUTTON (link_button), FALSE);
  
  return link_button->priv->visited;
}
