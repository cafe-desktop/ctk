/*
 * ctkinfobar.c
 * This file is part of CTK+
 *
 * Copyright (C) 2005 - Paolo Maggi
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
 * Modified by the gedit Team, 2005. See the AUTHORS file for a
 * list of people on the ctk Team.
 * See the gedit ChangeLog files for a list of changes.
 *
 * Modified by the CTK+ team, 2008-2009.
 */


#include "config.h"

#include <stdlib.h>

#include "ctkinfobar.h"
#include "ctkaccessible.h"
#include "ctkbuildable.h"
#include "ctkbuilderprivate.h"
#include "ctkbbox.h"
#include "ctkbox.h"
#include "ctklabel.h"
#include "ctkbutton.h"
#include "ctkenums.h"
#include "ctkbindings.h"
#include "ctkdialog.h"
#include "ctkrevealer.h"
#include "ctkintl.h"
#include "ctkprivate.h"
#include "ctkorientable.h"
#include "ctkrender.h"
#include "ctktypebuiltins.h"
#include "ctkwidgetprivate.h"
#include "ctkstock.h"
#include "ctkgesturemultipress.h"

/**
 * SECTION:ctkinfobar
 * @short_description: Report important messages to the user
 * @include: ctk/ctk.h
 * @see_also: #CtkStatusbar, #CtkMessageDialog
 *
 * #CtkInfoBar is a widget that can be used to show messages to
 * the user without showing a dialog. It is often temporarily shown
 * at the top or bottom of a document. In contrast to #CtkDialog, which
 * has a action area at the bottom, #CtkInfoBar has an action area
 * at the side.
 *
 * The API of #CtkInfoBar is very similar to #CtkDialog, allowing you
 * to add buttons to the action area with ctk_info_bar_add_button() or
 * ctk_info_bar_new_with_buttons(). The sensitivity of action widgets
 * can be controlled with ctk_info_bar_set_response_sensitive().
 * To add widgets to the main content area of a #CtkInfoBar, use
 * ctk_info_bar_get_content_area() and add your widgets to the container.
 *
 * Similar to #CtkMessageDialog, the contents of a #CtkInfoBar can by
 * classified as error message, warning, informational message, etc,
 * by using ctk_info_bar_set_message_type(). CTK+ may use the message type
 * to determine how the message is displayed.
 *
 * A simple example for using a #CtkInfoBar:
 * |[<!-- language="C" -->
 * CtkWidget *widget, *message_label, *content_area;
 * CtkWidget *grid;
 * CtkInfoBar *bar;
 *
 * // set up info bar
 * widget = ctk_info_bar_new ();
 * bar = CTK_INFO_BAR (widget);
 * grid = ctk_grid_new ();
 *
 * ctk_widget_set_no_show_all (widget, TRUE);
 * message_label = ctk_label_new ("");
 * content_area = ctk_info_bar_get_content_area (bar);
 * ctk_container_add (CTK_CONTAINER (content_area),
 *                    message_label);
 * ctk_info_bar_add_button (bar,
 *                          _("_OK"),
 *                          CTK_RESPONSE_OK);
 * g_signal_connect (bar,
 *                   "response",
 *                   G_CALLBACK (ctk_widget_hide),
 *                   NULL);
 * ctk_grid_attach (CTK_GRID (grid),
 *                  widget,
 *                  0, 2, 1, 1);
 *
 * // ...
 *
 * // show an error message
 * ctk_label_set_text (CTK_LABEL (message_label), "An error occurred!");
 * ctk_info_bar_set_message_type (bar,
 *                                CTK_MESSAGE_ERROR);
 * ctk_widget_show (bar);
 * ]|
 *
 * # CtkInfoBar as CtkBuildable
 *
 * The CtkInfoBar implementation of the CtkBuildable interface exposes
 * the content area and action area as internal children with the names
 * “content_area” and “action_area”.
 *
 * CtkInfoBar supports a custom <action-widgets> element, which can contain
 * multiple <action-widget> elements. The “response” attribute specifies a
 * numeric response, and the content of the element is the id of widget
 * (which should be a child of the dialogs @action_area).
 *
 * # CSS nodes
 *
 * CtkInfoBar has a single CSS node with name infobar. The node may get
 * one of the style classes .info, .warning, .error or .question, depending
 * on the message type.
 */

enum
{
  PROP_0,
  PROP_MESSAGE_TYPE,
  PROP_SHOW_CLOSE_BUTTON,
  PROP_REVEALED,
  LAST_PROP
};

struct _CtkInfoBarPrivate
{
  CtkWidget *content_area;
  CtkWidget *action_area;
  CtkWidget *close_button;
  CtkWidget *revealer;

  gboolean show_close_button;
  CtkMessageType message_type;
  int default_response;
  gboolean default_response_sensitive;

  CtkGesture *gesture;
};

typedef struct _ResponseData ResponseData;

struct _ResponseData
{
  gint response_id;
};

enum
{
  RESPONSE,
  CLOSE,
  LAST_SIGNAL
};

static GParamSpec *props[LAST_PROP] = { NULL, };
static guint signals[LAST_SIGNAL];

#define ACTION_AREA_DEFAULT_BORDER 5
#define ACTION_AREA_DEFAULT_SPACING 6
#define CONTENT_AREA_DEFAULT_BORDER 8
#define CONTENT_AREA_DEFAULT_SPACING 16

static void     ctk_info_bar_set_property (GObject        *object,
                                           guint           prop_id,
                                           const GValue   *value,
                                           GParamSpec     *pspec);
static void     ctk_info_bar_get_property (GObject        *object,
                                           guint           prop_id,
                                           GValue         *value,
                                           GParamSpec     *pspec);
static void     ctk_info_bar_buildable_interface_init     (CtkBuildableIface *iface);
static gboolean  ctk_info_bar_buildable_custom_tag_start   (CtkBuildable  *buildable,
                                                            CtkBuilder    *builder,
                                                            GObject       *child,
                                                            const gchar   *tagname,
                                                            GMarkupParser *parser,
                                                            gpointer      *data);
static void      ctk_info_bar_buildable_custom_finished    (CtkBuildable  *buildable,
                                                            CtkBuilder    *builder,
                                                            GObject       *child,
                                                            const gchar   *tagname,
                                                            gpointer       user_data);


G_DEFINE_TYPE_WITH_CODE (CtkInfoBar, ctk_info_bar, CTK_TYPE_BOX,
                         G_ADD_PRIVATE (CtkInfoBar)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_BUILDABLE,
                                                ctk_info_bar_buildable_interface_init))

static void
ctk_info_bar_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  CtkInfoBar *info_bar = CTK_INFO_BAR (object);

  switch (prop_id)
    {
    case PROP_MESSAGE_TYPE:
      ctk_info_bar_set_message_type (info_bar, g_value_get_enum (value));
      break;
    case PROP_SHOW_CLOSE_BUTTON:
      ctk_info_bar_set_show_close_button (info_bar, g_value_get_boolean (value));
      break;
    case PROP_REVEALED:
      ctk_info_bar_set_revealed (info_bar, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_info_bar_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  CtkInfoBar *info_bar = CTK_INFO_BAR (object);

  switch (prop_id)
    {
    case PROP_MESSAGE_TYPE:
      g_value_set_enum (value, ctk_info_bar_get_message_type (info_bar));
      break;
    case PROP_SHOW_CLOSE_BUTTON:
      g_value_set_boolean (value, ctk_info_bar_get_show_close_button (info_bar));
      break;
    case PROP_REVEALED:
      g_value_set_boolean (value, ctk_info_bar_get_revealed (info_bar));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
response_data_free (gpointer data)
{
  g_slice_free (ResponseData, data);
}

static ResponseData *
get_response_data (CtkWidget *widget,
                   gboolean   create)
{
  ResponseData *ad = g_object_get_data (G_OBJECT (widget),
                                        "ctk-info-bar-response-data");

  if (ad == NULL && create)
    {
      ad = g_slice_new (ResponseData);

      g_object_set_data_full (G_OBJECT (widget),
                              I_("ctk-info-bar-response-data"),
                              ad,
                              response_data_free);
    }

  return ad;
}

static CtkWidget *
find_button (CtkInfoBar *info_bar,
             gint        response_id)
{
  GList *children, *list;
  CtkWidget *child = NULL;

  children = ctk_container_get_children (CTK_CONTAINER (info_bar->priv->action_area));

  for (list = children; list; list = list->next)
    {
      ResponseData *rd = get_response_data (list->data, FALSE);

      if (rd && rd->response_id == response_id)
        {
          child = list->data;
          break;
        }
    }

  g_list_free (children);

  return child;
}

static void
update_state (CtkWidget *widget,
              gboolean   in)
{
  CtkStateFlags state;

  state = ctk_widget_get_state_flags (widget);
  if (in)
    state |= CTK_STATE_FLAG_PRELIGHT;
  else
    state &= ~CTK_STATE_FLAG_PRELIGHT;

  ctk_widget_set_state_flags (widget, state, TRUE);
}

static gboolean
ctk_info_bar_enter_notify (CtkWidget        *widget,
                           CdkEventCrossing *event)
{
  if (event->detail != CDK_NOTIFY_INFERIOR)
    update_state (widget, TRUE);

  return FALSE;
}

static gboolean
ctk_info_bar_leave_notify (CtkWidget        *widget,
                           CdkEventCrossing *event)
{
  if (event->detail != CDK_NOTIFY_INFERIOR)
    update_state (widget, FALSE);

  return FALSE;
}

static void
ctk_info_bar_realize (CtkWidget *widget)
{
  CtkAllocation allocation;
  CdkWindow *window;
  CdkWindowAttr attributes;
  gint attributes_mask;

  ctk_widget_get_allocation (widget, &allocation);

  ctk_widget_set_realized (widget, TRUE);

  attributes.window_type = CDK_WINDOW_CHILD;
  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.wclass = CDK_INPUT_ONLY;
  attributes.event_mask = ctk_widget_get_events (widget);
  attributes.event_mask |= (CDK_BUTTON_PRESS_MASK |
                            CDK_BUTTON_RELEASE_MASK |
                            CDK_TOUCH_MASK |
                            CDK_ENTER_NOTIFY_MASK |
                            CDK_LEAVE_NOTIFY_MASK);

  attributes_mask = CDK_WA_X | CDK_WA_Y;

  window = cdk_window_new (ctk_widget_get_parent_window (widget), &attributes, attributes_mask);
  ctk_widget_register_window (widget, window);
  ctk_widget_set_window (widget, window);
}

static void
ctk_info_bar_size_allocate (CtkWidget     *widget,
                            CtkAllocation *allocation)
{
  CtkAllocation tmp_allocation;
  CdkWindow *window;

  tmp_allocation = *allocation;
  tmp_allocation.x = 0;
  tmp_allocation.y = 0;

  CTK_WIDGET_CLASS (ctk_info_bar_parent_class)->size_allocate (widget,
                                                               &tmp_allocation);

  ctk_widget_set_allocation (widget, allocation);

  window = ctk_widget_get_window (widget);
  if (window != NULL)
    cdk_window_move_resize (window,
                            allocation->x, allocation->y,
                            allocation->width, allocation->height);
}

static void
ctk_info_bar_close (CtkInfoBar *info_bar)
{
  if (!ctk_widget_get_visible (info_bar->priv->close_button)
      && !find_button (info_bar, CTK_RESPONSE_CANCEL))
    return;

  ctk_info_bar_response (CTK_INFO_BAR (info_bar),
                         CTK_RESPONSE_CANCEL);
}

static void
ctk_info_bar_finalize (GObject *object)
{
  CtkInfoBar *info_bar = CTK_INFO_BAR (object);

  g_object_unref (info_bar->priv->gesture);

  G_OBJECT_CLASS (ctk_info_bar_parent_class)->finalize (object);
}

static void
ctk_info_bar_class_init (CtkInfoBarClass *klass)
{
  CtkWidgetClass *widget_class;
  GObjectClass *object_class;
  CtkBindingSet *binding_set;

  widget_class = CTK_WIDGET_CLASS (klass);
  object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = ctk_info_bar_finalize;
  object_class->get_property = ctk_info_bar_get_property;
  object_class->set_property = ctk_info_bar_set_property;

  widget_class->realize = ctk_info_bar_realize;
  widget_class->enter_notify_event = ctk_info_bar_enter_notify;
  widget_class->leave_notify_event = ctk_info_bar_leave_notify;
  widget_class->size_allocate = ctk_info_bar_size_allocate;

  klass->close = ctk_info_bar_close;

  /**
   * CtkInfoBar:message-type:
   *
   * The type of the message.
   *
   * The type may be used to determine the appearance of the info bar.
   *
   * Since: 2.18
   */
  props[PROP_MESSAGE_TYPE] =
    g_param_spec_enum ("message-type",
                       P_("Message Type"),
                       P_("The type of message"),
                       CTK_TYPE_MESSAGE_TYPE,
                       CTK_MESSAGE_INFO,
                       CTK_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkInfoBar:show-close-button:
   *
   * Whether to include a standard close button.
   *
   * Since: 3.10
   */
  props[PROP_SHOW_CLOSE_BUTTON] =
    g_param_spec_boolean ("show-close-button",
                          P_("Show Close Button"),
                          P_("Whether to include a standard close button"),
                          FALSE,
                          CTK_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_REVEALED] =
    g_param_spec_boolean ("revealed",
                          P_("Reveal"),
                          P_("Controls whether the action bar shows its contents or not"),
                          TRUE,
                          CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  /**
   * CtkInfoBar::response:
   * @info_bar: the object on which the signal is emitted
   * @response_id: the response ID
   *
   * Emitted when an action widget is clicked or the application programmer
   * calls ctk_dialog_response(). The @response_id depends on which action
   * widget was clicked.
   *
   * Since: 2.18
   */
  signals[RESPONSE] = g_signal_new (I_("response"),
                                    G_OBJECT_CLASS_TYPE (klass),
                                    G_SIGNAL_RUN_LAST,
                                    G_STRUCT_OFFSET (CtkInfoBarClass, response),
                                    NULL, NULL,
                                    NULL,
                                    G_TYPE_NONE, 1,
                                    G_TYPE_INT);

  /**
   * CtkInfoBar::close:
   *
   * The ::close signal is a
   * [keybinding signal][CtkBindingSignal]
   * which gets emitted when the user uses a keybinding to dismiss
   * the info bar.
   *
   * The default binding for this signal is the Escape key.
   *
   * Since: 2.18
   */
  signals[CLOSE] =  g_signal_new (I_("close"),
                                  G_OBJECT_CLASS_TYPE (klass),
                                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                  G_STRUCT_OFFSET (CtkInfoBarClass, close),
                                  NULL, NULL,
                                  NULL,
                                  G_TYPE_NONE, 0);

  /**
   * CtkInfoBar:content-area-border:
   *
   * The width of the border around the content
   * content area of the info bar.
   *
   * Since: 2.18
   * Deprecated: 3.6: Use ctk_container_set_border_width()
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("content-area-border",
                                                             P_("Content area border"),
                                                             P_("Width of border around the content area"),
                                                             0,
                                                             G_MAXINT,
                                                             CONTENT_AREA_DEFAULT_BORDER,
                                                             CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  /**
   * CtkInfoBar:content-area-spacing:
   *
   * The default spacing used between elements of the
   * content area of the info bar.
   *
   * Since: 2.18
   * Deprecated: 3.6: Use ctk_box_set_spacing()
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("content-area-spacing",
                                                             P_("Content area spacing"),
                                                             P_("Spacing between elements of the area"),
                                                             0,
                                                             G_MAXINT,
                                                             CONTENT_AREA_DEFAULT_SPACING,
                                                             CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  /**
   * CtkInfoBar:button-spacing:
   *
   * Spacing between buttons in the action area of the info bar.
   *
   * Since: 2.18
   * Deprecated: 3.6: Use ctk_box_set_spacing()
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("button-spacing",
                                                             P_("Button spacing"),
                                                             P_("Spacing between buttons"),
                                                             0,
                                                             G_MAXINT,
                                                             ACTION_AREA_DEFAULT_SPACING,
                                                             CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  /**
   * CtkInfoBar:action-area-border:
   *
   * Width of the border around the action area of the info bar.
   *
   * Since: 2.18
   * Deprecated: 3.6: Use ctk_container_set_border_width()
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("action-area-border",
                                                             P_("Action area border"),
                                                             P_("Width of border around the action area"),
                                                             0,
                                                             G_MAXINT,
                                                             ACTION_AREA_DEFAULT_BORDER,
                                                             CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  binding_set = ctk_binding_set_by_class (klass);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Escape, 0, "close", 0);

  /* Bind class to template
   */
  ctk_widget_class_set_template_from_resource (widget_class, "/org/ctk/libctk/ui/ctkinfobar.ui");
  ctk_widget_class_bind_template_child_internal_private (widget_class, CtkInfoBar, content_area);
  ctk_widget_class_bind_template_child_internal_private (widget_class, CtkInfoBar, action_area);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInfoBar, close_button);
  ctk_widget_class_bind_template_child_private (widget_class, CtkInfoBar, revealer);

  ctk_widget_class_set_css_name (widget_class, "infobar");
}

static void
close_button_clicked_cb (CtkWidget  *button,
                         CtkInfoBar *info_bar)
{
  ctk_info_bar_response (CTK_INFO_BAR (info_bar),
                         CTK_RESPONSE_CLOSE);
}

static void
click_pressed_cb (CtkGestureMultiPress *gesture,
                   guint            n_press,
                   gdouble          x,
                   gdouble          y,
                   CtkInfoBar      *info_bar)
{
  CtkInfoBarPrivate *priv = ctk_info_bar_get_instance_private (info_bar);

  if (priv->default_response && priv->default_response_sensitive)
    ctk_info_bar_response (info_bar, priv->default_response);
}

static void
ctk_info_bar_init (CtkInfoBar *info_bar)
{
  CtkInfoBarPrivate *priv;
  CtkWidget *widget = CTK_WIDGET (info_bar);

  priv = info_bar->priv = ctk_info_bar_get_instance_private (info_bar);

  /* message-type is a CONSTRUCT property, so we init to a value
   * different from its default to trigger its property setter
   * during construction */
  priv->message_type = CTK_MESSAGE_OTHER;

  ctk_widget_set_has_window (widget, TRUE);
  ctk_widget_init_template (widget);

  ctk_widget_set_no_show_all (priv->close_button, TRUE);
  g_signal_connect (priv->close_button, "clicked",
                    G_CALLBACK (close_button_clicked_cb), info_bar);

  priv->gesture = ctk_gesture_multi_press_new (widget);
  ctk_gesture_single_set_button (CTK_GESTURE_SINGLE (priv->gesture), CDK_BUTTON_PRIMARY);
  g_signal_connect (priv->gesture, "pressed", G_CALLBACK (click_pressed_cb), widget);
}

static CtkBuildableIface *parent_buildable_iface;

static void
ctk_info_bar_buildable_interface_init (CtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->custom_tag_start = ctk_info_bar_buildable_custom_tag_start;
  iface->custom_finished = ctk_info_bar_buildable_custom_finished;
}

static gint
get_response_for_widget (CtkInfoBar *info_bar,
                         CtkWidget  *widget)
{
  ResponseData *rd;

  rd = get_response_data (widget, FALSE);
  if (!rd)
    return CTK_RESPONSE_NONE;
  else
    return rd->response_id;
}

static void
action_widget_activated (CtkWidget  *widget,
                         CtkInfoBar *info_bar)
{
  gint response_id;

  response_id = get_response_for_widget (info_bar, widget);
  ctk_info_bar_response (info_bar, response_id);
}

/**
 * ctk_info_bar_add_action_widget:
 * @info_bar: a #CtkInfoBar
 * @child: an activatable widget
 * @response_id: response ID for @child
 *
 * Add an activatable widget to the action area of a #CtkInfoBar,
 * connecting a signal handler that will emit the #CtkInfoBar::response
 * signal on the message area when the widget is activated. The widget
 * is appended to the end of the message areas action area.
 *
 * Since: 2.18
 */
void
ctk_info_bar_add_action_widget (CtkInfoBar *info_bar,
                                CtkWidget  *child,
                                gint        response_id)
{
  ResponseData *ad;
  guint signal_id;

  g_return_if_fail (CTK_IS_INFO_BAR (info_bar));
  g_return_if_fail (CTK_IS_WIDGET (child));

  ad = get_response_data (child, TRUE);

  ad->response_id = response_id;

  if (CTK_IS_BUTTON (child))
    signal_id = g_signal_lookup ("clicked", CTK_TYPE_BUTTON);
  else
    signal_id = CTK_WIDGET_GET_CLASS (child)->activate_signal;

  if (signal_id)
    {
      GClosure *closure;

      closure = g_cclosure_new_object (G_CALLBACK (action_widget_activated),
                                       G_OBJECT (info_bar));
      g_signal_connect_closure_by_id (child, signal_id, 0, closure, FALSE);
    }
  else
    g_warning ("Only 'activatable' widgets can be packed into the action area of a CtkInfoBar");

  ctk_box_pack_end (CTK_BOX (info_bar->priv->action_area),
                    child, FALSE, FALSE, 0);
  if (response_id == CTK_RESPONSE_HELP)
    ctk_button_box_set_child_secondary (CTK_BUTTON_BOX (info_bar->priv->action_area),
                                        child, TRUE);
}

/**
 * ctk_info_bar_get_action_area:
 * @info_bar: a #CtkInfoBar
 *
 * Returns the action area of @info_bar.
 *
 * Returns: (type Ctk.Box) (transfer none): the action area
 *
 * Since: 2.18
 */
CtkWidget*
ctk_info_bar_get_action_area (CtkInfoBar *info_bar)
{
  g_return_val_if_fail (CTK_IS_INFO_BAR (info_bar), NULL);

  return info_bar->priv->action_area;
}

/**
 * ctk_info_bar_get_content_area:
 * @info_bar: a #CtkInfoBar
 *
 * Returns the content area of @info_bar.
 *
 * Returns: (type Ctk.Box) (transfer none): the content area
 *
 * Since: 2.18
 */
CtkWidget*
ctk_info_bar_get_content_area (CtkInfoBar *info_bar)
{
  g_return_val_if_fail (CTK_IS_INFO_BAR (info_bar), NULL);

  return info_bar->priv->content_area;
}

/**
 * ctk_info_bar_add_button:
 * @info_bar: a #CtkInfoBar
 * @button_text: text of button
 * @response_id: response ID for the button
 *
 * Adds a button with the given text and sets things up so that
 * clicking the button will emit the “response” signal with the given
 * response_id. The button is appended to the end of the info bars's
 * action area. The button widget is returned, but usually you don't
 * need it.
 *
 * Returns: (transfer none) (type Ctk.Button): the #CtkButton widget
 * that was added
 *
 * Since: 2.18
 */
CtkWidget*
ctk_info_bar_add_button (CtkInfoBar  *info_bar,
                         const gchar *button_text,
                         gint         response_id)
{
  CtkWidget *button;

  g_return_val_if_fail (CTK_IS_INFO_BAR (info_bar), NULL);
  g_return_val_if_fail (button_text != NULL, NULL);

  button = ctk_button_new_with_label (button_text);
  ctk_button_set_use_underline (CTK_BUTTON (button), TRUE);

  if (button_text)
    {
      CtkStockItem item;
      if (ctk_stock_lookup (button_text, &item))
        g_object_set (button, "use-stock", TRUE, NULL);
    }

  ctk_widget_set_can_default (button, TRUE);

  ctk_widget_show (button);

  ctk_info_bar_add_action_widget (info_bar, button, response_id);

  return button;
}

static void
add_buttons_valist (CtkInfoBar  *info_bar,
                    const gchar *first_button_text,
                    va_list      args)
{
  const gchar* text;
  gint response_id;

  g_return_if_fail (CTK_IS_INFO_BAR (info_bar));

  if (first_button_text == NULL)
    return;

  text = first_button_text;
  response_id = va_arg (args, gint);

  while (text != NULL)
    {
      ctk_info_bar_add_button (info_bar, text, response_id);

      text = va_arg (args, gchar*);
      if (text == NULL)
        break;

      response_id = va_arg (args, int);
    }
}

/**
 * ctk_info_bar_add_buttons:
 * @info_bar: a #CtkInfoBar
 * @first_button_text: button text or stock ID
 * @...: response ID for first button, then more text-response_id pairs,
 *     ending with %NULL
 *
 * Adds more buttons, same as calling ctk_info_bar_add_button()
 * repeatedly. The variable argument list should be %NULL-terminated
 * as with ctk_info_bar_new_with_buttons(). Each button must have both
 * text and response ID.
 *
 * Since: 2.18
 */
void
ctk_info_bar_add_buttons (CtkInfoBar  *info_bar,
                          const gchar *first_button_text,
                          ...)
{
  va_list args;

  va_start (args, first_button_text);
  add_buttons_valist (info_bar, first_button_text, args);
  va_end (args);
}

/**
 * ctk_info_bar_new:
 *
 * Creates a new #CtkInfoBar object.
 *
 * Returns: a new #CtkInfoBar object
 *
 * Since: 2.18
 */
CtkWidget *
ctk_info_bar_new (void)
{
   return g_object_new (CTK_TYPE_INFO_BAR, NULL);
}

/**
 * ctk_info_bar_new_with_buttons:
 * @first_button_text: (allow-none): stock ID or text to go in first button, or %NULL
 * @...: response ID for first button, then additional buttons, ending
 *    with %NULL
 *
 * Creates a new #CtkInfoBar with buttons. Button text/response ID
 * pairs should be listed, with a %NULL pointer ending the list.
 * Button text can be either a stock ID such as %CTK_STOCK_OK, or
 * some arbitrary text. A response ID can be any positive number,
 * or one of the values in the #CtkResponseType enumeration. If the
 * user clicks one of these dialog buttons, CtkInfoBar will emit
 * the “response” signal with the corresponding response ID.
 *
 * Returns: a new #CtkInfoBar
 */
CtkWidget*
ctk_info_bar_new_with_buttons (const gchar *first_button_text,
                               ...)
{
  CtkInfoBar *info_bar;
  va_list args;

  info_bar = CTK_INFO_BAR (ctk_info_bar_new ());

  va_start (args, first_button_text);
  add_buttons_valist (info_bar, first_button_text, args);
  va_end (args);

  return CTK_WIDGET (info_bar);
}

static void
update_default_response (CtkInfoBar *info_bar,
                         int         response_id,
                         gboolean    sensitive)
{
  CtkInfoBarPrivate *priv = ctk_info_bar_get_instance_private (info_bar);

  priv->default_response = response_id;
  priv->default_response_sensitive = sensitive;

  if (response_id && sensitive)
    ctk_style_context_add_class (ctk_widget_get_style_context (CTK_WIDGET (info_bar)), "action");
  else
    ctk_style_context_remove_class (ctk_widget_get_style_context (CTK_WIDGET (info_bar)), "action");
}

/**
 * ctk_info_bar_set_response_sensitive:
 * @info_bar: a #CtkInfoBar
 * @response_id: a response ID
 * @setting: TRUE for sensitive
 *
 * Calls ctk_widget_set_sensitive (widget, setting) for each
 * widget in the info bars’s action area with the given response_id.
 * A convenient way to sensitize/desensitize dialog buttons.
 *
 * Since: 2.18
 */
void
ctk_info_bar_set_response_sensitive (CtkInfoBar *info_bar,
                                     gint        response_id,
                                     gboolean    setting)
{
  GList *children, *list;

  g_return_if_fail (CTK_IS_INFO_BAR (info_bar));

  if (info_bar->priv->default_response == response_id)
    info_bar->priv->default_response_sensitive = setting;

  children = ctk_container_get_children (CTK_CONTAINER (info_bar->priv->action_area));

  for (list = children; list; list = list->next)
    {
      CtkWidget *widget = list->data;
      ResponseData *rd = get_response_data (widget, FALSE);

      if (rd && rd->response_id == response_id)
        ctk_widget_set_sensitive (widget, setting);
    }

  g_list_free (children);

  if (response_id == info_bar->priv->default_response)
    update_default_response (info_bar, response_id, setting);
}

/**
 * ctk_info_bar_set_default_response:
 * @info_bar: a #CtkInfoBar
 * @response_id: a response ID
 *
 * Sets the last widget in the info bar’s action area with
 * the given response_id as the default widget for the dialog.
 * Pressing “Enter” normally activates the default widget.
 *
 * Note that this function currently requires @info_bar to
 * be added to a widget hierarchy. 
 *
 * Since: 2.18
 */
void
ctk_info_bar_set_default_response (CtkInfoBar *info_bar,
                                   gint        response_id)
{
  GList *children, *list;
  gboolean sensitive = TRUE;

  g_return_if_fail (CTK_IS_INFO_BAR (info_bar));

  children = ctk_container_get_children (CTK_CONTAINER (info_bar->priv->action_area));

  for (list = children; list; list = list->next)
    {
      CtkWidget *widget = list->data;
      ResponseData *rd = get_response_data (widget, FALSE);

      if (rd && rd->response_id == response_id)
        {
          ctk_widget_grab_default (widget);
          sensitive = ctk_widget_get_sensitive (widget);
        }
    }

  g_list_free (children);

  update_default_response (info_bar, response_id, sensitive);
}

/**
 * ctk_info_bar_response:
 * @info_bar: a #CtkInfoBar
 * @response_id: a response ID
 *
 * Emits the “response” signal with the given @response_id.
 *
 * Since: 2.18
 */
void
ctk_info_bar_response (CtkInfoBar *info_bar,
                       gint        response_id)
{
  g_return_if_fail (CTK_IS_INFO_BAR (info_bar));

  g_signal_emit (info_bar, signals[RESPONSE], 0, response_id);
}

typedef struct
{
  gchar *name;
  gint response_id;
  gint line;
  gint col;
} ActionWidgetInfo;

typedef struct
{
  CtkInfoBar *info_bar;
  CtkBuilder *builder;
  GSList *items;
  gint response_id;
  gboolean is_text;
  GString *string;
  gint line;
  gint col;
} SubParserData;

static void
action_widget_info_free (gpointer data)
{
  ActionWidgetInfo *item = data;

  g_free (item->name);
  g_free (item);
}

static void
parser_start_element (GMarkupParseContext  *context,
                      const gchar          *element_name,
                      const gchar         **names,
                      const gchar         **values,
                      gpointer              user_data,
                      GError              **error)
{
  SubParserData *data = (SubParserData*)user_data;

  if (strcmp (element_name, "action-widget") == 0)
    {
      const gchar *response;
      GValue gvalue = G_VALUE_INIT;

      if (!_ctk_builder_check_parent (data->builder, context, "action-widgets", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_STRING, "response", &response,
                                        G_MARKUP_COLLECT_INVALID))
        {
          _ctk_builder_prefix_error (data->builder, context, error);
          return;
        }

      if (!ctk_builder_value_from_string_type (data->builder, CTK_TYPE_RESPONSE_TYPE, response, &gvalue, error))
        {
          _ctk_builder_prefix_error (data->builder, context, error);
          return;
        }

      data->response_id = g_value_get_enum (&gvalue);
      data->is_text = TRUE;
      g_string_set_size (data->string, 0);
      g_markup_parse_context_get_position (context, &data->line, &data->col);
    }
  else if (strcmp (element_name, "action-widgets") == 0)
    {
      if (!_ctk_builder_check_parent (data->builder, context, "object", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_INVALID, NULL, NULL,
                                        G_MARKUP_COLLECT_INVALID))
        _ctk_builder_prefix_error (data->builder, context, error);
    }
  else
    {
      _ctk_builder_error_unhandled_tag (data->builder, context,
                                        "CtkInfoBar", element_name,
                                        error);
    }
}

static void
parser_text_element (GMarkupParseContext  *context,
                     const gchar          *text,
                     gsize                 text_len,
                     gpointer              user_data,
                     GError              **error)
{
  SubParserData *data = (SubParserData*)user_data;

  if (data->is_text)
    g_string_append_len (data->string, text, text_len);
}

static void
parser_end_element (GMarkupParseContext  *context,
                    const gchar          *element_name,
                    gpointer              user_data,
                    GError              **error)
{
  SubParserData *data = (SubParserData*)user_data;

  if (data->is_text)
    {
      ActionWidgetInfo *item;

      item = g_new (ActionWidgetInfo, 1);
      item->name = g_strdup (data->string->str);
      item->response_id = data->response_id;
      item->line = data->line;
      item->col = data->col;

      data->items = g_slist_prepend (data->items, item);
      data->is_text = FALSE;
    }
}

static const GMarkupParser sub_parser =
{
  .start_element = parser_start_element,
  .end_element = parser_end_element,
  .text = parser_text_element,
};

gboolean
ctk_info_bar_buildable_custom_tag_start (CtkBuildable  *buildable,
                                         CtkBuilder    *builder,
                                         GObject       *child,
                                         const gchar   *tagname,
                                         GMarkupParser *parser,
                                         gpointer      *parser_data)
{
  SubParserData *data;

  if (parent_buildable_iface->custom_tag_start (buildable, builder, child,
                                                tagname, parser, parser_data))
    return TRUE;

  if (!child && strcmp (tagname, "action-widgets") == 0)
    {
      data = g_slice_new0 (SubParserData);
      data->info_bar = CTK_INFO_BAR (buildable);
      data->builder = builder;
      data->string = g_string_new ("");
      data->items = NULL;

      *parser = sub_parser;
      *parser_data = data;
      return TRUE;
    }

  return FALSE;
}

static void
ctk_info_bar_buildable_custom_finished (CtkBuildable *buildable,
                                        CtkBuilder   *builder,
                                        GObject      *child,
                                        const gchar  *tagname,
                                        gpointer      user_data)
{
  GSList *l;
  SubParserData *data;
  GObject *object;
  CtkInfoBar *info_bar;
  ResponseData *ad;
  guint signal_id;

  if (strcmp (tagname, "action-widgets"))
    {
      parent_buildable_iface->custom_finished (buildable, builder, child,
                                               tagname, user_data);
      return;
    }

  info_bar = CTK_INFO_BAR (buildable);
  data = (SubParserData*)user_data;
  data->items = g_slist_reverse (data->items);

  for (l = data->items; l; l = l->next)
    {
      ActionWidgetInfo *item = l->data;

      object = _ctk_builder_lookup_object (builder, item->name, item->line, item->col);
      if (!object)
        continue;

      ad = get_response_data (CTK_WIDGET (object), TRUE);
      ad->response_id = item->response_id;

      if (CTK_IS_BUTTON (object))
        signal_id = g_signal_lookup ("clicked", CTK_TYPE_BUTTON);
      else
        signal_id = CTK_WIDGET_GET_CLASS (object)->activate_signal;

      if (signal_id)
        {
          GClosure *closure;

          closure = g_cclosure_new_object (G_CALLBACK (action_widget_activated),
                                           G_OBJECT (info_bar));
          g_signal_connect_closure_by_id (object, signal_id, 0, closure, FALSE);
        }

      if (ad->response_id == CTK_RESPONSE_HELP)
        ctk_button_box_set_child_secondary (CTK_BUTTON_BOX (info_bar->priv->action_area),
                                            CTK_WIDGET (object), TRUE);
    }

  g_slist_free_full (data->items, action_widget_info_free);
  g_string_free (data->string, TRUE);
  g_slice_free (SubParserData, data);
}

/**
 * ctk_info_bar_set_message_type:
 * @info_bar: a #CtkInfoBar
 * @message_type: a #CtkMessageType
 *
 * Sets the message type of the message area.
 *
 * CTK+ uses this type to determine how the message is displayed.
 *
 * Since: 2.18
 */
void
ctk_info_bar_set_message_type (CtkInfoBar     *info_bar,
                               CtkMessageType  message_type)
{
  CtkInfoBarPrivate *priv;

  g_return_if_fail (CTK_IS_INFO_BAR (info_bar));

  priv = info_bar->priv;

  if (priv->message_type != message_type)
    {
      CtkStyleContext *context;
      AtkObject *atk_obj;
      const char *type_class[] = {
        CTK_STYLE_CLASS_INFO,
        CTK_STYLE_CLASS_WARNING,
        CTK_STYLE_CLASS_QUESTION,
        CTK_STYLE_CLASS_ERROR,
        NULL
      };

      context = ctk_widget_get_style_context (CTK_WIDGET (info_bar));

      if (type_class[priv->message_type])
        ctk_style_context_remove_class (context, type_class[priv->message_type]);

      priv->message_type = message_type;

      ctk_widget_queue_draw (CTK_WIDGET (info_bar));

      atk_obj = ctk_widget_get_accessible (CTK_WIDGET (info_bar));
      if (CTK_IS_ACCESSIBLE (atk_obj))
        {
          const char *name = NULL;

          atk_object_set_role (atk_obj, ATK_ROLE_INFO_BAR);

          switch (message_type)
            {
            case CTK_MESSAGE_INFO:
              name = _("Information");
              break;

            case CTK_MESSAGE_QUESTION:
              name = _("Question");
              break;

            case CTK_MESSAGE_WARNING:
              name = _("Warning");
              break;

            case CTK_MESSAGE_ERROR:
              name = _("Error");
              break;

            case CTK_MESSAGE_OTHER:
              break;

            default:
              g_warning ("Unknown CtkMessageType %u", message_type);
              break;
            }

          if (name)
            atk_object_set_name (atk_obj, name);
        }

      if (type_class[priv->message_type])
        ctk_style_context_add_class (context, type_class[priv->message_type]);

      g_object_notify_by_pspec (G_OBJECT (info_bar), props[PROP_MESSAGE_TYPE]);
    }
}

/**
 * ctk_info_bar_get_message_type:
 * @info_bar: a #CtkInfoBar
 *
 * Returns the message type of the message area.
 *
 * Returns: the message type of the message area.
 *
 * Since: 2.18
 */
CtkMessageType
ctk_info_bar_get_message_type (CtkInfoBar *info_bar)
{
  g_return_val_if_fail (CTK_IS_INFO_BAR (info_bar), CTK_MESSAGE_OTHER);

  return info_bar->priv->message_type;
}


/**
 * ctk_info_bar_set_show_close_button:
 * @info_bar: a #CtkInfoBar
 * @setting: %TRUE to include a close button
 *
 * If true, a standard close button is shown. When clicked it emits
 * the response %CTK_RESPONSE_CLOSE.
 *
 * Since: 3.10
 */
void
ctk_info_bar_set_show_close_button (CtkInfoBar *info_bar,
                                    gboolean    setting)
{
  g_return_if_fail (CTK_IS_INFO_BAR (info_bar));

  if (setting != info_bar->priv->show_close_button)
    {
      info_bar->priv->show_close_button = setting;
      ctk_widget_set_visible (info_bar->priv->close_button, setting);
      g_object_notify_by_pspec (G_OBJECT (info_bar), props[PROP_SHOW_CLOSE_BUTTON]);
    }
}

/**
 * ctk_info_bar_get_show_close_button:
 * @info_bar: a #CtkInfoBar
 *
 * Returns whether the widget will display a standard close button.
 *
 * Returns: %TRUE if the widget displays standard close button
 *
 * Since: 3.10
 */
gboolean
ctk_info_bar_get_show_close_button (CtkInfoBar *info_bar)
{
  g_return_val_if_fail (CTK_IS_INFO_BAR (info_bar), FALSE);

  return info_bar->priv->show_close_button;
}

/**
 * ctk_info_bar_set_revealed:
 * @info_bar: a #CtkInfoBar
 * @revealed: The new value of the property
 *
 * Sets the CtkInfoBar:revealed property to @revealed. This will cause
 * @info_bar to show up with a slide-in transition.
 *
 * Note that this property does not automatically show @info_bar and thus won’t
 * have any effect if it is invisible.
 *
 * Since: 3.22.29
 */
void
ctk_info_bar_set_revealed (CtkInfoBar *info_bar,
                           gboolean    revealed)
{
  CtkInfoBarPrivate *priv = ctk_info_bar_get_instance_private (info_bar);

  g_return_if_fail (CTK_IS_INFO_BAR (info_bar));

  revealed = !!revealed;
  if (revealed != ctk_revealer_get_reveal_child (CTK_REVEALER (priv->revealer)))
    {
      ctk_revealer_set_reveal_child (CTK_REVEALER (priv->revealer), revealed);
      g_object_notify_by_pspec (G_OBJECT (info_bar), props[PROP_REVEALED]);
    }
}

/**
 * ctk_info_bar_get_revealed:
 * @info_bar: a #CtkInfoBar
 *
 * Returns: the current value of the CtkInfoBar:revealed property.
 *
 * Since: 3.22.29
 */
gboolean
ctk_info_bar_get_revealed (CtkInfoBar *info_bar)
{
  CtkInfoBarPrivate *priv = ctk_info_bar_get_instance_private (info_bar);

  g_return_val_if_fail (CTK_IS_INFO_BAR (info_bar), FALSE);

  return ctk_revealer_get_reveal_child (CTK_REVEALER (priv->revealer));
}
