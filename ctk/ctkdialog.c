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

#include "ctkbutton.h"
#include "ctkdialog.h"
#include "ctkdialogprivate.h"
#include "ctkheaderbar.h"
#include "ctkbbox.h"
#include "ctklabel.h"
#include "ctkmarshalers.h"
#include "ctkbox.h"
#include "ctkboxprivate.h"
#include "ctkcontainerprivate.h"
#include "ctkmain.h"
#include "ctkintl.h"
#include "ctkbindings.h"
#include "ctkprivate.h"
#include "ctkbuildable.h"
#include "ctkbuilderprivate.h"
#include "ctksettings.h"
#include "ctktypebuiltins.h"
#include "ctkstock.h"
#include "ctksizegroup.h"

/**
 * SECTION:ctkdialog
 * @Short_description: Create popup windows
 * @Title: CtkDialog
 * @See_also: #CtkVBox, #CtkWindow, #CtkButton
 *
 * Dialog boxes are a convenient way to prompt the user for a small amount
 * of input, e.g. to display a message, ask a question, or anything else
 * that does not require extensive effort on the user’s part.
 *
 * CTK+ treats a dialog as a window split vertically. The top section is a
 * #CtkVBox, and is where widgets such as a #CtkLabel or a #CtkEntry should
 * be packed. The bottom area is known as the
 * “action area”. This is generally used for
 * packing buttons into the dialog which may perform functions such as
 * cancel, ok, or apply.
 *
 * #CtkDialog boxes are created with a call to ctk_dialog_new() or
 * ctk_dialog_new_with_buttons(). ctk_dialog_new_with_buttons() is
 * recommended; it allows you to set the dialog title, some convenient
 * flags, and add simple buttons.
 *
 * If “dialog” is a newly created dialog, the two primary areas of the
 * window can be accessed through ctk_dialog_get_content_area() and
 * ctk_dialog_get_action_area(), as can be seen from the example below.
 *
 * A “modal” dialog (that is, one which freezes the rest of the application
 * from user input), can be created by calling ctk_window_set_modal() on the
 * dialog. Use the CTK_WINDOW() macro to cast the widget returned from
 * ctk_dialog_new() into a #CtkWindow. When using ctk_dialog_new_with_buttons()
 * you can also pass the #CTK_DIALOG_MODAL flag to make a dialog modal.
 *
 * If you add buttons to #CtkDialog using ctk_dialog_new_with_buttons(),
 * ctk_dialog_add_button(), ctk_dialog_add_buttons(), or
 * ctk_dialog_add_action_widget(), clicking the button will emit a signal
 * called #CtkDialog::response with a response ID that you specified. CTK+
 * will never assign a meaning to positive response IDs; these are entirely
 * user-defined. But for convenience, you can use the response IDs in the
 * #CtkResponseType enumeration (these all have values less than zero). If
 * a dialog receives a delete event, the #CtkDialog::response signal will
 * be emitted with a response ID of #CTK_RESPONSE_DELETE_EVENT.
 *
 * If you want to block waiting for a dialog to return before returning
 * control flow to your code, you can call ctk_dialog_run(). This function
 * enters a recursive main loop and waits for the user to respond to the
 * dialog, returning the response ID corresponding to the button the user
 * clicked.
 *
 * For the simple dialog in the following example, in reality you’d probably
 * use #CtkMessageDialog to save yourself some effort. But you’d need to
 * create the dialog contents manually if you had more than a simple message
 * in the dialog.
 *
 * An example for simple CtkDialog usage:
 * |[<!-- language="C" -->
 * // Function to open a dialog box with a message
 * void
 * quick_message (CtkWindow *parent, gchar *message)
 * {
 *  CtkWidget *dialog, *label, *content_area;
 *  CtkDialogFlags flags;
 *
 *  // Create the widgets
 *  flags = CTK_DIALOG_DESTROY_WITH_PARENT;
 *  dialog = ctk_dialog_new_with_buttons ("Message",
 *                                        parent,
 *                                        flags,
 *                                        _("_OK"),
 *                                        CTK_RESPONSE_NONE,
 *                                        NULL);
 *  content_area = ctk_dialog_get_content_area (CTK_DIALOG (dialog));
 *  label = ctk_label_new (message);
 *
 *  // Ensure that the dialog box is destroyed when the user responds
 *
 *  g_signal_connect_swapped (dialog,
 *                            "response",
 *                            G_CALLBACK (ctk_widget_destroy),
 *                            dialog);
 *
 *  // Add the label, and show everything we’ve added
 *
 *  ctk_container_add (CTK_CONTAINER (content_area), label);
 *  ctk_widget_show_all (dialog);
 * }
 * ]|
 *
 * # CtkDialog as CtkBuildable
 *
 * The CtkDialog implementation of the #CtkBuildable interface exposes the
 * @vbox and @action_area as internal children with the names “vbox” and
 * “action_area”.
 *
 * CtkDialog supports a custom <action-widgets> element, which can contain
 * multiple <action-widget> elements. The “response” attribute specifies a
 * numeric response, and the content of the element is the id of widget
 * (which should be a child of the dialogs @action_area). To mark a response
 * as default, set the “default“ attribute of the <action-widget> element
 * to true.
 *
 * CtkDialog supports adding action widgets by specifying “action“ as
 * the “type“ attribute of a <child> element. The widget will be added
 * either to the action area or the headerbar of the dialog, depending
 * on the “use-header-bar“ property. The response id has to be associated
 * with the action widget using the <action-widgets> element.
 *
 * An example of a #CtkDialog UI definition fragment:
 * |[
 * <object class="CtkDialog" id="dialog1">
 *   <child type="action">
 *     <object class="CtkButton" id="button_cancel"/>
 *   </child>
 *   <child type="action">
 *     <object class="CtkButton" id="button_ok">
 *       <property name="can-default">True</property>
 *     </object>
 *   </child>
 *   <action-widgets>
 *     <action-widget response="cancel">button_cancel</action-widget>
 *     <action-widget response="ok" default="true">button_ok</action-widget>
 *   </action-widgets>
 * </object>
 * ]|
 */

struct _CtkDialogPrivate
{
  CtkWidget *vbox;
  CtkWidget *headerbar;
  CtkWidget *action_area;
  CtkWidget *action_box;
  CtkSizeGroup *size_group;

  gint use_header_bar;
  gboolean constructed;
};

typedef struct _ResponseData ResponseData;

struct _ResponseData
{
  gint response_id;
};

static void      ctk_dialog_add_buttons_valist   (CtkDialog    *dialog,
                                                  const gchar  *first_button_text,
                                                  va_list       args);

static gboolean  ctk_dialog_delete_event_handler (CtkWidget    *widget,
                                                  CdkEventAny  *event,
                                                  gpointer      user_data);
static void      ctk_dialog_style_updated        (CtkWidget    *widget);
static void      ctk_dialog_map                  (CtkWidget    *widget);

static void      ctk_dialog_close                (CtkDialog    *dialog);

static ResponseData * get_response_data          (CtkWidget    *widget,
                                                  gboolean      create);

static void      ctk_dialog_buildable_interface_init     (CtkBuildableIface *iface);
static gboolean  ctk_dialog_buildable_custom_tag_start   (CtkBuildable  *buildable,
                                                          CtkBuilder    *builder,
                                                          GObject       *child,
                                                          const gchar   *tagname,
                                                          GMarkupParser *parser,
                                                          gpointer      *data);
static void      ctk_dialog_buildable_custom_finished    (CtkBuildable  *buildable,
                                                          CtkBuilder    *builder,
                                                          GObject       *child,
                                                          const gchar   *tagname,
                                                          gpointer       user_data);
static void      ctk_dialog_buildable_add_child          (CtkBuildable  *buildable,
                                                          CtkBuilder    *builder,
                                                          GObject       *child,
                                                          const gchar   *type);


enum {
  PROP_0,
  PROP_USE_HEADER_BAR
};

enum {
  RESPONSE,
  CLOSE,
  LAST_SIGNAL
};

static guint dialog_signals[LAST_SIGNAL];

G_DEFINE_TYPE_WITH_CODE (CtkDialog, ctk_dialog, CTK_TYPE_WINDOW,
                         G_ADD_PRIVATE (CtkDialog)
			 G_IMPLEMENT_INTERFACE (CTK_TYPE_BUILDABLE,
						ctk_dialog_buildable_interface_init))

static void
set_use_header_bar (CtkDialog *dialog,
                    gint       use_header_bar)
{
  CtkDialogPrivate *priv = dialog->priv;

  if (use_header_bar == -1)
    return;

  priv->use_header_bar = use_header_bar;
}

/* A convenience helper for built-in dialogs */
void
ctk_dialog_set_use_header_bar_from_setting (CtkDialog *dialog)
{
  CtkDialogPrivate *priv = dialog->priv;

  g_assert (!priv->constructed);

  g_object_get (ctk_widget_get_settings (CTK_WIDGET (dialog)),
                "ctk-dialogs-use-header", &priv->use_header_bar,
                NULL);
}

static void
ctk_dialog_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  CtkDialog *dialog = CTK_DIALOG (object);

  switch (prop_id)
    {
    case PROP_USE_HEADER_BAR:
      set_use_header_bar (dialog, g_value_get_int (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_dialog_get_property (GObject      *object,
                         guint         prop_id,
                         GValue       *value,
                         GParamSpec   *pspec)
{
  CtkDialog *dialog = CTK_DIALOG (object);
  CtkDialogPrivate *priv = dialog->priv;

  switch (prop_id)
    {
    case PROP_USE_HEADER_BAR:
      g_value_set_int (value, priv->use_header_bar);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
action_widget_activated (CtkWidget *widget, CtkDialog *dialog)
{
  gint response_id;

  response_id = ctk_dialog_get_response_for_widget (dialog, widget);

  ctk_dialog_response (dialog, response_id);
}

typedef struct {
  CtkWidget *child;
  gint       response_id;
} ActionWidgetData;

static void
add_response_data (CtkDialog *dialog,
                   CtkWidget *child,
                   gint       response_id)
{
  ResponseData *ad;
  guint signal_id;

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
                                       G_OBJECT (dialog));
      g_signal_connect_closure_by_id (child, signal_id, 0, closure, FALSE);
    }
  else
    g_warning ("Only 'activatable' widgets can be packed into the action area of a CtkDialog");
}

static void
apply_response_for_header_bar (CtkDialog *dialog,
                               CtkWidget *child,
                               gint       response_id)
{
  CtkDialogPrivate *priv = dialog->priv;
  CtkPackType pack;

  g_assert (ctk_widget_get_parent (child) == priv->headerbar);

  if (response_id == CTK_RESPONSE_CANCEL || response_id == CTK_RESPONSE_HELP)
    pack = CTK_PACK_START;
  else
    pack = CTK_PACK_END;

  ctk_container_child_set (CTK_CONTAINER (priv->headerbar), child,
                           "pack-type", pack,
                           NULL);

  if (response_id == CTK_RESPONSE_CANCEL || response_id == CTK_RESPONSE_CLOSE)
    ctk_header_bar_set_show_close_button (CTK_HEADER_BAR (priv->headerbar), FALSE);
}

static void
add_to_header_bar (CtkDialog *dialog,
                   CtkWidget *child,
                   gint       response_id)
{
  CtkDialogPrivate *priv = dialog->priv;

  ctk_widget_set_valign (child, CTK_ALIGN_CENTER);
  ctk_container_add (CTK_CONTAINER (priv->headerbar), child);
  ctk_size_group_add_widget (priv->size_group, child);
  apply_response_for_header_bar (dialog, child, response_id);

}

static void
apply_response_for_action_area (CtkDialog *dialog,
                                CtkWidget *child,
                                gint       response_id)
{
  CtkDialogPrivate *priv = dialog->priv;

  g_assert (ctk_widget_get_parent (child) == priv->action_area);

  if (response_id == CTK_RESPONSE_HELP)
    ctk_button_box_set_child_secondary (CTK_BUTTON_BOX (priv->action_area), child, TRUE);
}

static void
add_to_action_area (CtkDialog *dialog,
                    CtkWidget *child,
                    gint       response_id)
{
  CtkDialogPrivate *priv = dialog->priv;

  ctk_widget_set_valign (child, CTK_ALIGN_BASELINE);
  ctk_container_add (CTK_CONTAINER (priv->action_area), child);
  apply_response_for_action_area (dialog, child, response_id);
}

static void
update_suggested_action (CtkDialog *dialog)
{
  CtkDialogPrivate *priv = dialog->priv;

  if (priv->use_header_bar)
    {
      GList *children, *l;

      children = ctk_container_get_children (CTK_CONTAINER (priv->headerbar));
      for (l = children; l != NULL; l = l->next)
        {
          CtkWidget *child = l->data;
	  CtkStyleContext *context = ctk_widget_get_style_context (child);

          if (ctk_style_context_has_class (context, CTK_STYLE_CLASS_DEFAULT))
            ctk_style_context_add_class (context, CTK_STYLE_CLASS_SUGGESTED_ACTION);
          else
            ctk_style_context_remove_class (context, CTK_STYLE_CLASS_SUGGESTED_ACTION);
        }
      g_list_free (children);
    }
}

static void
add_cb (CtkContainer *container,
        CtkWidget    *widget,
        CtkDialog    *dialog)
{
  CtkDialogPrivate *priv = dialog->priv;

  if (priv->use_header_bar)
    g_warning ("Content added to the action area of a dialog using header bars");

  ctk_widget_set_visible (priv->action_box, TRUE);
  ctk_widget_set_no_show_all (priv->action_box, FALSE);
}

static void
ctk_dialog_constructed (GObject *object)
{
  CtkDialog *dialog = CTK_DIALOG (object);
  CtkDialogPrivate *priv = dialog->priv;

  G_OBJECT_CLASS (ctk_dialog_parent_class)->constructed (object);

  priv->constructed = TRUE;
  if (priv->use_header_bar == -1)
    priv->use_header_bar = FALSE;

  if (priv->use_header_bar)
    {
      GList *children, *l;

      children = ctk_container_get_children (CTK_CONTAINER (priv->action_area));
      for (l = children; l != NULL; l = l->next)
        {
          CtkWidget *child = l->data;
          gboolean has_default;
          ResponseData *rd;
          gint response_id;

          has_default = ctk_widget_has_default (child);
          rd = get_response_data (child, FALSE);
          response_id = rd ? rd->response_id : CTK_RESPONSE_NONE;

          g_object_ref (child);
          ctk_container_remove (CTK_CONTAINER (priv->action_area), child);
          add_to_header_bar (dialog, child, response_id);
          g_object_unref (child);

          if (has_default)
            ctk_widget_grab_default (child);
        }
      g_list_free (children);

      update_suggested_action (dialog);

      g_signal_connect (priv->action_area, "add", G_CALLBACK (add_cb), dialog);
    }
  else
    {
      ctk_window_set_titlebar (CTK_WINDOW (dialog), NULL);
      priv->headerbar = NULL;
    }

  ctk_widget_set_visible (priv->action_box, !priv->use_header_bar);
  ctk_widget_set_no_show_all (priv->action_box, priv->use_header_bar);
}

static void
ctk_dialog_finalize (GObject *obj)
{
  CtkDialog *dialog = CTK_DIALOG (obj);

  g_object_unref (dialog->priv->size_group);

  G_OBJECT_CLASS (ctk_dialog_parent_class)->finalize (obj);
}

static void
ctk_dialog_class_init (CtkDialogClass *class)
{
  GObjectClass *gobject_class;
  CtkWidgetClass *widget_class;
  CtkBindingSet *binding_set;

  gobject_class = G_OBJECT_CLASS (class);
  widget_class = CTK_WIDGET_CLASS (class);

  gobject_class->constructed  = ctk_dialog_constructed;
  gobject_class->set_property = ctk_dialog_set_property;
  gobject_class->get_property = ctk_dialog_get_property;
  gobject_class->finalize = ctk_dialog_finalize;

  widget_class->map = ctk_dialog_map;
  widget_class->style_updated = ctk_dialog_style_updated;

  ctk_widget_class_set_accessible_role (widget_class, ATK_ROLE_DIALOG);

  class->close = ctk_dialog_close;

  /**
   * CtkDialog::response:
   * @dialog: the object on which the signal is emitted
   * @response_id: the response ID
   *
   * Emitted when an action widget is clicked, the dialog receives a
   * delete event, or the application programmer calls ctk_dialog_response().
   * On a delete event, the response ID is #CTK_RESPONSE_DELETE_EVENT.
   * Otherwise, it depends on which action widget was clicked.
   */
  dialog_signals[RESPONSE] =
    g_signal_new (I_("response"),
		  G_OBJECT_CLASS_TYPE (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkDialogClass, response),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1,
		  G_TYPE_INT);

  /**
   * CtkDialog::close:
   *
   * The ::close signal is a
   * [keybinding signal][CtkBindingSignal]
   * which gets emitted when the user uses a keybinding to close
   * the dialog.
   *
   * The default binding for this signal is the Escape key.
   */
  dialog_signals[CLOSE] =
    g_signal_new (I_("close"),
		  G_OBJECT_CLASS_TYPE (class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkDialogClass, close),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * CtkDialog:content-area-border:
   *
   * The default border width used around the
   * content area of the dialog, as returned by
   * ctk_dialog_get_content_area(), unless ctk_container_set_border_width()
   * was called on that widget directly.
   */
  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("content-area-border",
                                                             P_("Content area border"),
                                                             P_("Width of border around the main dialog area"),
                                                             0,
                                                             G_MAXINT,
                                                             2,
                                                             CTK_PARAM_READABLE));
  /**
   * CtkDialog:content-area-spacing:
   *
   * The default spacing used between elements of the
   * content area of the dialog, as returned by
   * ctk_dialog_get_content_area(), unless ctk_box_set_spacing()
   * was called on that widget directly.
   *
   * Since: 2.16
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("content-area-spacing",
                                                             P_("Content area spacing"),
                                                             P_("Spacing between elements of the main dialog area"),
                                                             0,
                                                             G_MAXINT,
                                                             0,
                                                             CTK_PARAM_READABLE));
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("button-spacing",
                                                             P_("Button spacing"),
                                                             P_("Spacing between buttons"),
                                                             0,
                                                             G_MAXINT,
                                                             6,
                                                             CTK_PARAM_READABLE));

  /**
   * CtkDialog:action-area-border:
   *
   * The default border width used around the
   * action area of the dialog, as returned by
   * ctk_dialog_get_action_area(), unless ctk_container_set_border_width()
   * was called on that widget directly.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("action-area-border",
                                                             P_("Action area border"),
                                                             P_("Width of border around the button area at the bottom of the dialog"),
                                                             0,
                                                             G_MAXINT,
                                                             5,
                                                             CTK_PARAM_READABLE));

  /**
   * CtkDialog:use-header-bar:
   *
   * %TRUE if the dialog uses a #CtkHeaderBar for action buttons
   * instead of the action-area.
   *
   * For technical reasons, this property is declared as an integer
   * property, but you should only set it to %TRUE or %FALSE.
   *
   * Since: 3.12
   */
  g_object_class_install_property (gobject_class,
                                   PROP_USE_HEADER_BAR,
                                   g_param_spec_int ("use-header-bar",
                                                     P_("Use Header Bar"),
                                                     P_("Use Header Bar for actions."),
                                                     -1, 1, -1,
                                                     CTK_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));

  binding_set = ctk_binding_set_by_class (class);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Escape, 0, "close", 0);

  /* Bind class to template
   */
  ctk_widget_class_set_template_from_resource (widget_class, "/org/ctk/libctk/ui/ctkdialog.ui");
  ctk_widget_class_bind_template_child_internal_private (widget_class, CtkDialog, vbox);
  ctk_widget_class_bind_template_child_internal_private (widget_class, CtkDialog, headerbar);
  ctk_widget_class_bind_template_child_internal_private (widget_class, CtkDialog, action_area);
  ctk_widget_class_bind_template_child_private (widget_class, CtkDialog, action_box);
  ctk_widget_class_bind_template_callback (widget_class, ctk_dialog_delete_event_handler);

  ctk_widget_class_set_css_name (widget_class, "dialog");
}

static void
update_spacings (CtkDialog *dialog)
{
  CtkDialogPrivate *priv = dialog->priv;
  gint content_area_border;
  gint content_area_spacing;
  gint button_spacing;
  gint action_area_border;

  ctk_widget_style_get (CTK_WIDGET (dialog),
                        "content-area-border", &content_area_border,
                        "content-area-spacing", &content_area_spacing,
                        "button-spacing", &button_spacing,
                        "action-area-border", &action_area_border,
                        NULL);

  if (!_ctk_container_get_border_width_set (CTK_CONTAINER (priv->vbox)))
    {
      ctk_container_set_border_width (CTK_CONTAINER (priv->vbox),
                                      content_area_border);
      _ctk_container_set_border_width_set (CTK_CONTAINER (priv->vbox), FALSE);
    }

  if (!_ctk_box_get_spacing_set (CTK_BOX (priv->vbox)))
    {
      ctk_box_set_spacing (CTK_BOX (priv->vbox), content_area_spacing);
      _ctk_box_set_spacing_set (CTK_BOX (priv->vbox), FALSE);
    }

  /* don't set spacing when buttons are linked */
  if (ctk_button_box_get_layout (CTK_BUTTON_BOX (priv->action_area)) != CTK_BUTTONBOX_EXPAND)
    ctk_box_set_spacing (CTK_BOX (priv->action_area), button_spacing);

  if (!_ctk_container_get_border_width_set (CTK_CONTAINER (priv->action_area)))
    {
      ctk_container_set_border_width (CTK_CONTAINER (priv->action_area),
                                      action_area_border);
      _ctk_container_set_border_width_set (CTK_CONTAINER (priv->action_area), FALSE);
    }
}

static void
ctk_dialog_init (CtkDialog *dialog)
{
  dialog->priv = ctk_dialog_get_instance_private (dialog);

  dialog->priv->use_header_bar = -1;
  dialog->priv->size_group = ctk_size_group_new (CTK_SIZE_GROUP_HORIZONTAL);

  ctk_widget_init_template (CTK_WIDGET (dialog));

  update_spacings (dialog);
}

static CtkBuildableIface *parent_buildable_iface;

static void
ctk_dialog_buildable_interface_init (CtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->custom_tag_start = ctk_dialog_buildable_custom_tag_start;
  iface->custom_finished = ctk_dialog_buildable_custom_finished;
  iface->add_child = ctk_dialog_buildable_add_child;
}

static gboolean
ctk_dialog_delete_event_handler (CtkWidget   *widget,
                                 CdkEventAny *event,
                                 gpointer     user_data)
{
  /* emit response signal */
  ctk_dialog_response (CTK_DIALOG (widget), CTK_RESPONSE_DELETE_EVENT);

  /* Do the destroy by default */
  return FALSE;
}

static GList *
get_action_children (CtkDialog *dialog)
{
  CtkDialogPrivate *priv = dialog->priv;
  GList *children;

  if (priv->constructed && priv->use_header_bar)
    children = ctk_container_get_children (CTK_CONTAINER (priv->headerbar));
  else
    children = ctk_container_get_children (CTK_CONTAINER (priv->action_area));

  return children;
}

/* A far too tricky heuristic for getting the right initial
 * focus widget if none was set. What we do is we focus the first
 * widget in the tab chain, but if this results in the focus
 * ending up on one of the response widgets _other_ than the
 * default response, we focus the default response instead.
 *
 * Additionally, skip selectable labels when looking for the
 * right initial focus widget.
 */
static void
ctk_dialog_map (CtkWidget *widget)
{
  CtkWidget *default_widget, *focus;
  CtkWindow *window = CTK_WINDOW (widget);
  CtkDialog *dialog = CTK_DIALOG (widget);

  CTK_WIDGET_CLASS (ctk_dialog_parent_class)->map (widget);

  focus = ctk_window_get_focus (window);
  if (!focus)
    {
      GList *children, *tmp_list;
      CtkWidget *first_focus = NULL;

      do
        {
          g_signal_emit_by_name (window, "move_focus", CTK_DIR_TAB_FORWARD);

          focus = ctk_window_get_focus (window);
          if (CTK_IS_LABEL (focus) &&
              !ctk_label_get_current_uri (CTK_LABEL (focus)))
            ctk_label_select_region (CTK_LABEL (focus), 0, 0);

          if (first_focus == NULL)
            first_focus = focus;
          else if (first_focus == focus)
            break;

          if (!CTK_IS_LABEL (focus))
            break;
        }
      while (TRUE);

      tmp_list = children = get_action_children (dialog);

      while (tmp_list)
	{
	  CtkWidget *child = tmp_list->data;

          default_widget = ctk_window_get_default_widget (window);
	  if ((focus == NULL || child == focus) &&
	      child != default_widget &&
	      default_widget)
	    {
	      ctk_widget_grab_focus (default_widget);
	      break;
	    }

	  tmp_list = tmp_list->next;
	}

      g_list_free (children);
    }
}

static void
ctk_dialog_style_updated (CtkWidget *widget)
{
  CTK_WIDGET_CLASS (ctk_dialog_parent_class)->style_updated (widget);

  update_spacings (CTK_DIALOG (widget));
}

static CtkWidget *
dialog_find_button (CtkDialog *dialog,
                   gint       response_id)
{
  CtkWidget *child = NULL;
  GList *children, *tmp_list;

  children = get_action_children (dialog);

  for (tmp_list = children; tmp_list; tmp_list = tmp_list->next)
    {
      ResponseData *rd = get_response_data (tmp_list->data, FALSE);

      if (rd && rd->response_id == response_id)
       {
         child = tmp_list->data;
         break;
       }
    }

  g_list_free (children);

  return child;
}

static void
ctk_dialog_close (CtkDialog *dialog)
{
  ctk_window_close (CTK_WINDOW (dialog));
}

/**
 * ctk_dialog_new:
 *
 * Creates a new dialog box.
 *
 * Widgets should not be packed into this #CtkWindow
 * directly, but into the @vbox and @action_area, as described above.
 *
 * Returns: the new dialog as a #CtkWidget
 */
CtkWidget*
ctk_dialog_new (void)
{
  return g_object_new (CTK_TYPE_DIALOG, NULL);
}

static CtkWidget*
ctk_dialog_new_empty (const gchar     *title,
                      CtkWindow       *parent,
                      CtkDialogFlags   flags)
{
  CtkDialog *dialog;

  dialog = g_object_new (CTK_TYPE_DIALOG,
                         "use-header-bar", (flags & CTK_DIALOG_USE_HEADER_BAR) != 0,
                         NULL);

  if (title)
    ctk_window_set_title (CTK_WINDOW (dialog), title);

  if (parent)
    ctk_window_set_transient_for (CTK_WINDOW (dialog), parent);

  if (flags & CTK_DIALOG_MODAL)
    ctk_window_set_modal (CTK_WINDOW (dialog), TRUE);

  if (flags & CTK_DIALOG_DESTROY_WITH_PARENT)
    ctk_window_set_destroy_with_parent (CTK_WINDOW (dialog), TRUE);

  return CTK_WIDGET (dialog);
}

/**
 * ctk_dialog_new_with_buttons:
 * @title: (allow-none): Title of the dialog, or %NULL
 * @parent: (allow-none): Transient parent of the dialog, or %NULL
 * @flags: from #CtkDialogFlags
 * @first_button_text: (allow-none): text to go in first button, or %NULL
 * @...: response ID for first button, then additional buttons, ending with %NULL
 *
 * Creates a new #CtkDialog with title @title (or %NULL for the default
 * title; see ctk_window_set_title()) and transient parent @parent (or
 * %NULL for none; see ctk_window_set_transient_for()). The @flags
 * argument can be used to make the dialog modal (#CTK_DIALOG_MODAL)
 * and/or to have it destroyed along with its transient parent
 * (#CTK_DIALOG_DESTROY_WITH_PARENT). After @flags, button
 * text/response ID pairs should be listed, with a %NULL pointer ending
 * the list. Button text can be arbitrary text. A response ID can be
 * any positive number, or one of the values in the #CtkResponseType
 * enumeration. If the user clicks one of these dialog buttons,
 * #CtkDialog will emit the #CtkDialog::response signal with the corresponding
 * response ID. If a #CtkDialog receives the #CtkWidget::delete-event signal,
 * it will emit ::response with a response ID of #CTK_RESPONSE_DELETE_EVENT.
 * However, destroying a dialog does not emit the ::response signal;
 * so be careful relying on ::response when using the
 * #CTK_DIALOG_DESTROY_WITH_PARENT flag. Buttons are from left to right,
 * so the first button in the list will be the leftmost button in the dialog.
 *
 * Here’s a simple example:
 * |[<!-- language="C" -->
 *  CtkWidget *main_app_window; // Window the dialog should show up on
 *  CtkWidget *dialog;
 *  CtkDialogFlags flags = CTK_DIALOG_MODAL | CTK_DIALOG_DESTROY_WITH_PARENT;
 *  dialog = ctk_dialog_new_with_buttons ("My dialog",
 *                                        main_app_window,
 *                                        flags,
 *                                        _("_OK"),
 *                                        CTK_RESPONSE_ACCEPT,
 *                                        _("_Cancel"),
 *                                        CTK_RESPONSE_REJECT,
 *                                        NULL);
 * ]|
 *
 * Returns: a new #CtkDialog
 */
CtkWidget*
ctk_dialog_new_with_buttons (const gchar    *title,
                             CtkWindow      *parent,
                             CtkDialogFlags  flags,
                             const gchar    *first_button_text,
                             ...)
{
  CtkDialog *dialog;
  va_list args;

  dialog = CTK_DIALOG (ctk_dialog_new_empty (title, parent, flags));

  va_start (args, first_button_text);

  ctk_dialog_add_buttons_valist (dialog,
                                 first_button_text,
                                 args);

  va_end (args);

  return CTK_WIDGET (dialog);
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
                                        "ctk-dialog-response-data");

  if (ad == NULL && create)
    {
      ad = g_slice_new (ResponseData);

      g_object_set_data_full (G_OBJECT (widget),
                              I_("ctk-dialog-response-data"),
                              ad,
			      response_data_free);
    }

  return ad;
}

/**
 * ctk_dialog_add_action_widget:
 * @dialog: a #CtkDialog
 * @child: an activatable widget
 * @response_id: response ID for @child
 *
 * Adds an activatable widget to the action area of a #CtkDialog,
 * connecting a signal handler that will emit the #CtkDialog::response
 * signal on the dialog when the widget is activated. The widget is
 * appended to the end of the dialog’s action area. If you want to add a
 * non-activatable widget, simply pack it into the @action_area field
 * of the #CtkDialog struct.
 **/
void
ctk_dialog_add_action_widget (CtkDialog *dialog,
                              CtkWidget *child,
                              gint       response_id)
{
  CtkDialogPrivate *priv = dialog->priv;

  g_return_if_fail (CTK_IS_DIALOG (dialog));
  g_return_if_fail (CTK_IS_WIDGET (child));

  add_response_data (dialog, child, response_id);

  if (priv->constructed && priv->use_header_bar)
    {
      add_to_header_bar (dialog, child, response_id);

      if (ctk_widget_has_default (child))
        {
          ctk_widget_grab_default (child);
          update_suggested_action (dialog);
        }
    }
  else
    add_to_action_area (dialog, child, response_id);
}

/**
 * ctk_dialog_add_button:
 * @dialog: a #CtkDialog
 * @button_text: text of button
 * @response_id: response ID for the button
 *
 * Adds a button with the given text and sets things up so that
 * clicking the button will emit the #CtkDialog::response signal with
 * the given @response_id. The button is appended to the end of the
 * dialog’s action area. The button widget is returned, but usually
 * you don’t need it.
 *
 * Returns: (transfer none): the #CtkButton widget that was added
 **/
CtkWidget*
ctk_dialog_add_button (CtkDialog   *dialog,
                       const gchar *button_text,
                       gint         response_id)
{
  CtkWidget *button;

  g_return_val_if_fail (CTK_IS_DIALOG (dialog), NULL);
  g_return_val_if_fail (button_text != NULL, NULL);

  button = ctk_button_new_with_label (button_text);
  ctk_button_set_use_underline (CTK_BUTTON (button), TRUE);

  if (button_text)
    {
      CtkStockItem item;
      if (ctk_stock_lookup (button_text, &item))
        g_object_set (button, "use-stock", TRUE, NULL);
    }

  ctk_style_context_add_class (ctk_widget_get_style_context (button), "text-button");
  ctk_widget_set_can_default (button, TRUE);

  ctk_widget_show (button);

  ctk_dialog_add_action_widget (dialog, button, response_id);

  return button;
}

/**
 * ctk_dialog_add_button_with_icon_name:
 * @dialog: a #CtkDialog
 * @button_text: text of button
 * @icon_name: icon name of button
 * @response_id: response ID for the button
 *
 * Adds a button with the given text beside icon and sets things up so that
 * clicking the button will emit the #CtkDialog::response signal with
 * the given @response_id. The button is appended to the end of the
 * dialog’s action area. The button widget is returned.
 *
 * Returns: (transfer none): the #CtkButton widget that was added
 *
 * Since: 3.25.2
 **/
CtkWidget *
ctk_dialog_add_button_with_icon_name (CtkDialog   *dialog,
                                      const gchar *button_text,
                                      const gchar *icon_name,
                                      gint         response_id)
{
  CtkWidget *button;

  g_return_val_if_fail (CTK_IS_DIALOG (dialog), NULL);
  g_return_val_if_fail (button_text != NULL, NULL);
  g_return_val_if_fail (icon_name != NULL, NULL);

  button = ctk_button_new_with_mnemonic (button_text);
  ctk_button_set_image (CTK_BUTTON (button), ctk_image_new_from_icon_name (icon_name, CTK_ICON_SIZE_BUTTON));

  ctk_button_set_use_underline (CTK_BUTTON (button), TRUE);
  ctk_style_context_add_class (ctk_widget_get_style_context (button), "text-button");
  ctk_widget_set_can_default (button, TRUE);
  ctk_widget_show (button);
  ctk_dialog_add_action_widget (CTK_DIALOG (dialog), button, response_id);

  return button;
}

static void
ctk_dialog_add_buttons_valist (CtkDialog      *dialog,
                               const gchar    *first_button_text,
                               va_list         args)
{
  const gchar* text;
  gint response_id;

  g_return_if_fail (CTK_IS_DIALOG (dialog));

  if (first_button_text == NULL)
    return;

  text = first_button_text;
  response_id = va_arg (args, gint);

  while (text != NULL)
    {
      ctk_dialog_add_button (dialog, text, response_id);

      text = va_arg (args, gchar*);
      if (text == NULL)
        break;
      response_id = va_arg (args, int);
    }
}

/**
 * ctk_dialog_add_buttons:
 * @dialog: a #CtkDialog
 * @first_button_text: button text
 * @...: response ID for first button, then more text-response_id pairs
 *
 * Adds more buttons, same as calling ctk_dialog_add_button()
 * repeatedly.  The variable argument list should be %NULL-terminated
 * as with ctk_dialog_new_with_buttons(). Each button must have both
 * text and response ID.
 */
void
ctk_dialog_add_buttons (CtkDialog   *dialog,
                        const gchar *first_button_text,
                        ...)
{
  va_list args;

  va_start (args, first_button_text);

  ctk_dialog_add_buttons_valist (dialog,
                                 first_button_text,
                                 args);

  va_end (args);
}

/**
 * ctk_dialog_set_response_sensitive:
 * @dialog: a #CtkDialog
 * @response_id: a response ID
 * @setting: %TRUE for sensitive
 *
 * Calls `ctk_widget_set_sensitive (widget, @setting)`
 * for each widget in the dialog’s action area with the given @response_id.
 * A convenient way to sensitize/desensitize dialog buttons.
 **/
void
ctk_dialog_set_response_sensitive (CtkDialog *dialog,
                                   gint       response_id,
                                   gboolean   setting)
{
  GList *children;
  GList *tmp_list;

  g_return_if_fail (CTK_IS_DIALOG (dialog));

  children = get_action_children (dialog);

  tmp_list = children;
  while (tmp_list != NULL)
    {
      CtkWidget *widget = tmp_list->data;
      ResponseData *rd = get_response_data (widget, FALSE);

      if (rd && rd->response_id == response_id)
        ctk_widget_set_sensitive (widget, setting);

      tmp_list = tmp_list->next;
    }

  g_list_free (children);
}

/**
 * ctk_dialog_set_default_response:
 * @dialog: a #CtkDialog
 * @response_id: a response ID
 *
 * Sets the last widget in the dialog’s action area with the given @response_id
 * as the default widget for the dialog. Pressing “Enter” normally activates
 * the default widget.
 **/
void
ctk_dialog_set_default_response (CtkDialog *dialog,
                                 gint       response_id)
{
  GList *children;
  GList *tmp_list;

  g_return_if_fail (CTK_IS_DIALOG (dialog));

  children = get_action_children (dialog);

  tmp_list = children;
  while (tmp_list != NULL)
    {
      CtkWidget *widget = tmp_list->data;
      ResponseData *rd = get_response_data (widget, FALSE);

      if (rd && rd->response_id == response_id)
	ctk_widget_grab_default (widget);

      tmp_list = tmp_list->next;
    }

  g_list_free (children);

  if (dialog->priv->use_header_bar)
    update_suggested_action (dialog);
}

/**
 * ctk_dialog_response:
 * @dialog: a #CtkDialog
 * @response_id: response ID
 *
 * Emits the #CtkDialog::response signal with the given response ID.
 * Used to indicate that the user has responded to the dialog in some way;
 * typically either you or ctk_dialog_run() will be monitoring the
 * ::response signal and take appropriate action.
 **/
void
ctk_dialog_response (CtkDialog *dialog,
                     gint       response_id)
{
  g_return_if_fail (CTK_IS_DIALOG (dialog));

  g_signal_emit (dialog,
		 dialog_signals[RESPONSE],
		 0,
		 response_id);
}

typedef struct
{
  CtkDialog *dialog;
  gint response_id;
  GMainLoop *loop;
  gboolean destroyed;
} RunInfo;

static void
shutdown_loop (RunInfo *ri)
{
  if (g_main_loop_is_running (ri->loop))
    g_main_loop_quit (ri->loop);
}

static void
run_unmap_handler (CtkDialog *dialog, gpointer data)
{
  RunInfo *ri = data;

  shutdown_loop (ri);
}

static void
run_response_handler (CtkDialog *dialog,
                      gint response_id,
                      gpointer data)
{
  RunInfo *ri;

  ri = data;

  ri->response_id = response_id;

  shutdown_loop (ri);
}

static gint
run_delete_handler (CtkDialog *dialog,
                    CdkEventAny *event,
                    gpointer data)
{
  RunInfo *ri = data;

  shutdown_loop (ri);

  return TRUE; /* Do not destroy */
}

static void
run_destroy_handler (CtkDialog *dialog, gpointer data)
{
  RunInfo *ri = data;

  /* shutdown_loop will be called by run_unmap_handler */

  ri->destroyed = TRUE;
}

/**
 * ctk_dialog_run:
 * @dialog: a #CtkDialog
 *
 * Blocks in a recursive main loop until the @dialog either emits the
 * #CtkDialog::response signal, or is destroyed. If the dialog is
 * destroyed during the call to ctk_dialog_run(), ctk_dialog_run() returns
 * #CTK_RESPONSE_NONE. Otherwise, it returns the response ID from the
 * ::response signal emission.
 *
 * Before entering the recursive main loop, ctk_dialog_run() calls
 * ctk_widget_show() on the dialog for you. Note that you still
 * need to show any children of the dialog yourself.
 *
 * During ctk_dialog_run(), the default behavior of #CtkWidget::delete-event
 * is disabled; if the dialog receives ::delete_event, it will not be
 * destroyed as windows usually are, and ctk_dialog_run() will return
 * #CTK_RESPONSE_DELETE_EVENT. Also, during ctk_dialog_run() the dialog
 * will be modal. You can force ctk_dialog_run() to return at any time by
 * calling ctk_dialog_response() to emit the ::response signal. Destroying
 * the dialog during ctk_dialog_run() is a very bad idea, because your
 * post-run code won’t know whether the dialog was destroyed or not.
 *
 * After ctk_dialog_run() returns, you are responsible for hiding or
 * destroying the dialog if you wish to do so.
 *
 * Typical usage of this function might be:
 * |[<!-- language="C" -->
 *   CtkWidget *dialog = ctk_dialog_new ();
 *   // Set up dialog...
 *
 *   int result = ctk_dialog_run (CTK_DIALOG (dialog));
 *   switch (result)
 *     {
 *       case CTK_RESPONSE_ACCEPT:
 *          // do_application_specific_something ();
 *          break;
 *       default:
 *          // do_nothing_since_dialog_was_cancelled ();
 *          break;
 *     }
 *   ctk_widget_destroy (dialog);
 * ]|
 *
 * Note that even though the recursive main loop gives the effect of a
 * modal dialog (it prevents the user from interacting with other
 * windows in the same window group while the dialog is run), callbacks
 * such as timeouts, IO channel watches, DND drops, etc, will
 * be triggered during a ctk_dialog_run() call.
 *
 * Returns: response ID
 **/
gint
ctk_dialog_run (CtkDialog *dialog)
{
  RunInfo ri = { NULL, CTK_RESPONSE_NONE, NULL, FALSE };
  gboolean was_modal;
  gulong response_handler;
  gulong unmap_handler;
  gulong destroy_handler;
  gulong delete_handler;

  g_return_val_if_fail (CTK_IS_DIALOG (dialog), -1);

  g_object_ref (dialog);

  was_modal = ctk_window_get_modal (CTK_WINDOW (dialog));
  if (!was_modal)
    ctk_window_set_modal (CTK_WINDOW (dialog), TRUE);

  if (!ctk_widget_get_visible (CTK_WIDGET (dialog)))
    ctk_widget_show (CTK_WIDGET (dialog));

  response_handler =
    g_signal_connect (dialog,
                      "response",
                      G_CALLBACK (run_response_handler),
                      &ri);

  unmap_handler =
    g_signal_connect (dialog,
                      "unmap",
                      G_CALLBACK (run_unmap_handler),
                      &ri);

  delete_handler =
    g_signal_connect (dialog,
                      "delete-event",
                      G_CALLBACK (run_delete_handler),
                      &ri);

  destroy_handler =
    g_signal_connect (dialog,
                      "destroy",
                      G_CALLBACK (run_destroy_handler),
                      &ri);

  ri.loop = g_main_loop_new (NULL, FALSE);

  cdk_threads_leave ();
  g_main_loop_run (ri.loop);
  cdk_threads_enter ();

  g_main_loop_unref (ri.loop);

  ri.loop = NULL;

  if (!ri.destroyed)
    {
      if (!was_modal)
        ctk_window_set_modal (CTK_WINDOW(dialog), FALSE);

      g_signal_handler_disconnect (dialog, response_handler);
      g_signal_handler_disconnect (dialog, unmap_handler);
      g_signal_handler_disconnect (dialog, delete_handler);
      g_signal_handler_disconnect (dialog, destroy_handler);
    }

  g_object_unref (dialog);

  return ri.response_id;
}

/**
 * ctk_dialog_get_widget_for_response:
 * @dialog: a #CtkDialog
 * @response_id: the response ID used by the @dialog widget
 *
 * Gets the widget button that uses the given response ID in the action area
 * of a dialog.
 *
 * Returns: (nullable) (transfer none): the @widget button that uses the given
 *     @response_id, or %NULL.
 *
 * Since: 2.20
 */
CtkWidget*
ctk_dialog_get_widget_for_response (CtkDialog *dialog,
				    gint       response_id)
{
  GList *children;
  GList *tmp_list;

  g_return_val_if_fail (CTK_IS_DIALOG (dialog), NULL);

  children = get_action_children (dialog);

  tmp_list = children;
  while (tmp_list != NULL)
    {
      CtkWidget *widget = tmp_list->data;
      ResponseData *rd = get_response_data (widget, FALSE);

      if (rd && rd->response_id == response_id)
        {
          g_list_free (children);
          return widget;
        }

      tmp_list = tmp_list->next;
    }

  g_list_free (children);

  return NULL;
}

/**
 * ctk_dialog_get_response_for_widget:
 * @dialog: a #CtkDialog
 * @widget: a widget in the action area of @dialog
 *
 * Gets the response id of a widget in the action area
 * of a dialog.
 *
 * Returns: the response id of @widget, or %CTK_RESPONSE_NONE
 *  if @widget doesn’t have a response id set.
 *
 * Since: 2.8
 */
gint
ctk_dialog_get_response_for_widget (CtkDialog *dialog,
				    CtkWidget *widget)
{
  ResponseData *rd;

  rd = get_response_data (widget, FALSE);
  if (!rd)
    return CTK_RESPONSE_NONE;
  else
    return rd->response_id;
}

static gboolean
ctk_alt_dialog_button_order (void)
{
  gboolean result;
  g_object_get (ctk_settings_get_default (),
		"ctk-alternative-button-order", &result, NULL);
  return result;
}

/**
 * ctk_alternative_dialog_button_order:
 * @screen: (allow-none): a #CdkScreen, or %NULL to use the default screen
 *
 * Returns %TRUE if dialogs are expected to use an alternative
 * button order on the screen @screen. See
 * ctk_dialog_set_alternative_button_order() for more details
 * about alternative button order.
 *
 * If you need to use this function, you should probably connect
 * to the ::notify:ctk-alternative-button-order signal on the
 * #CtkSettings object associated to @screen, in order to be
 * notified if the button order setting changes.
 *
 * Returns: Whether the alternative button order should be used
 *
 * Since: 2.6
 * Deprecated: 3.10: Deprecated
 */
gboolean
ctk_alternative_dialog_button_order (CdkScreen *screen)
{
  return ctk_alt_dialog_button_order ();
}

static void
ctk_dialog_set_alternative_button_order_valist (CtkDialog *dialog,
                                                gint       first_response_id,
                                                va_list    args)
{
  CtkDialogPrivate *priv = dialog->priv;
  CtkWidget *child;
  gint response_id;
  gint position;

  response_id = first_response_id;
  position = 0;
  while (response_id != -1)
    {
      /* reorder child with response_id to position */
      child = dialog_find_button (dialog, response_id);
      if (child != NULL)
        ctk_box_reorder_child (CTK_BOX (priv->action_area), child, position);
      else
        g_warning ("%s : no child button with response id %d.", G_STRFUNC,
                   response_id);

      response_id = va_arg (args, gint);
      position++;
    }
}

/**
 * ctk_dialog_set_alternative_button_order:
 * @dialog: a #CtkDialog
 * @first_response_id: a response id used by one @dialog’s buttons
 * @...: a list of more response ids of @dialog’s buttons, terminated by -1
 *
 * Sets an alternative button order. If the
 * #CtkSettings:ctk-alternative-button-order setting is set to %TRUE,
 * the dialog buttons are reordered according to the order of the
 * response ids passed to this function.
 *
 * By default, CTK+ dialogs use the button order advocated by the
 * [GNOME Human Interface Guidelines](http://library.gnome.org/devel/hig-book/stable/)
 * with the affirmative button at the far
 * right, and the cancel button left of it. But the builtin CTK+ dialogs
 * and #CtkMessageDialogs do provide an alternative button order,
 * which is more suitable on some platforms, e.g. Windows.
 *
 * Use this function after adding all the buttons to your dialog, as the
 * following example shows:
 *
 * |[<!-- language="C" -->
 * cancel_button = ctk_dialog_add_button (CTK_DIALOG (dialog),
 *                                        _("_Cancel"),
 *                                        CTK_RESPONSE_CANCEL);
 *
 * ok_button = ctk_dialog_add_button (CTK_DIALOG (dialog),
 *                                    _("_OK"),
 *                                    CTK_RESPONSE_OK);
 *
 * ctk_widget_grab_default (ok_button);
 *
 * help_button = ctk_dialog_add_button (CTK_DIALOG (dialog),
 *                                      _("_Help"),
 *                                      CTK_RESPONSE_HELP);
 *
 * ctk_dialog_set_alternative_button_order (CTK_DIALOG (dialog),
 *                                          CTK_RESPONSE_OK,
 *                                          CTK_RESPONSE_CANCEL,
 *                                          CTK_RESPONSE_HELP,
 *                                          -1);
 * ]|
 *
 * Since: 2.6
 * Deprecated: 3.10: Deprecated
 */
void
ctk_dialog_set_alternative_button_order (CtkDialog *dialog,
					 gint       first_response_id,
					 ...)
{
  CtkDialogPrivate *priv = dialog->priv;
  va_list args;

  g_return_if_fail (CTK_IS_DIALOG (dialog));

  if (priv->constructed && priv->use_header_bar)
    return;

  if (!ctk_alt_dialog_button_order ())
    return;

  va_start (args, first_response_id);

  ctk_dialog_set_alternative_button_order_valist (dialog,
                                                 first_response_id,
                                                 args);
  va_end (args);
}
/**
 * ctk_dialog_set_alternative_button_order_from_array:
 * @dialog: a #CtkDialog
 * @n_params: the number of response ids in @new_order
 * @new_order: (array length=n_params): an array of response ids of
 *     @dialog’s buttons
 *
 * Sets an alternative button order. If the
 * #CtkSettings:ctk-alternative-button-order setting is set to %TRUE,
 * the dialog buttons are reordered according to the order of the
 * response ids in @new_order.
 *
 * See ctk_dialog_set_alternative_button_order() for more information.
 *
 * This function is for use by language bindings.
 *
 * Since: 2.6
 * Deprecated: 3.10: Deprecated
 */
void
ctk_dialog_set_alternative_button_order_from_array (CtkDialog *dialog,
                                                    gint       n_params,
                                                    gint      *new_order)
{
  CtkDialogPrivate *priv = dialog->priv;
  CtkWidget *child;
  gint position;

  g_return_if_fail (CTK_IS_DIALOG (dialog));
  g_return_if_fail (new_order != NULL);

  if (dialog->priv->use_header_bar)
    return;

  if (!ctk_alt_dialog_button_order ())
    return;

  for (position = 0; position < n_params; position++)
  {
      /* reorder child with response_id to position */
      child = dialog_find_button (dialog, new_order[position]);
      if (child != NULL)
        ctk_box_reorder_child (CTK_BOX (priv->action_area), child, position);
      else
        g_warning ("%s : no child button with response id %d.", G_STRFUNC,
                   new_order[position]);
    }
}

typedef struct {
  gchar *widget_name;
  gint response_id;
  gboolean is_default;
  gint line;
  gint col;
} ActionWidgetInfo;

typedef struct {
  CtkDialog *dialog;
  CtkBuilder *builder;
  GSList *items;
  gint response_id;
  gboolean is_default;
  gboolean is_text;
  GString *string;
  gboolean in_action_widgets;
  gint line;
  gint col;
} SubParserData;

static void
free_action_widget_info (gpointer data)
{
  ActionWidgetInfo *item = data;

  g_free (item->widget_name);
  g_free (item);
}

static void
parser_start_element (GMarkupParseContext *context,
                      const gchar         *element_name,
                      const gchar        **names,
                      const gchar        **values,
                      gpointer             user_data,
                      GError             **error)
{
  SubParserData *data = (SubParserData*)user_data;

  if (strcmp (element_name, "action-widget") == 0)
    {
      const gchar *response;
      gboolean is_default = FALSE;
      GValue gvalue = G_VALUE_INIT;

      if (!_ctk_builder_check_parent (data->builder, context, "action-widgets", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_STRING, "response", &response,
                                        G_MARKUP_COLLECT_BOOLEAN|G_MARKUP_COLLECT_OPTIONAL, "default", &is_default,
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
      data->is_default = is_default;
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

      data->in_action_widgets = TRUE;
    }
  else
    {
      _ctk_builder_error_unhandled_tag (data->builder, context,
                                        "CtkDialog", element_name,
                                        error);
    }
}

static void
parser_text_element (GMarkupParseContext *context,
                     const gchar         *text,
                     gsize                text_len,
                     gpointer             user_data,
                     GError             **error)
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
      item->widget_name = g_strdup (data->string->str);
      item->response_id = data->response_id;
      item->is_default = data->is_default;
      item->line = data->line;
      item->col = data->col;

      data->items = g_slist_prepend (data->items, item);
      data->is_default = FALSE;
      data->is_text = FALSE;
    }
}

static const GMarkupParser sub_parser =
  {
    .start_element = parser_start_element,
    .end_element = parser_end_element,
    .text = parser_text_element,
  };

static gboolean
ctk_dialog_buildable_custom_tag_start (CtkBuildable  *buildable,
                                       CtkBuilder    *builder,
                                       GObject       *child,
                                       const gchar   *tagname,
                                       GMarkupParser *parser,
                                       gpointer      *parser_data)
{
  SubParserData *data;

  if (child)
    return FALSE;

  if (strcmp (tagname, "action-widgets") == 0)
    {
      data = g_slice_new0 (SubParserData);
      data->dialog = CTK_DIALOG (buildable);
      data->builder = builder;
      data->string = g_string_new ("");
      data->items = NULL;
      data->in_action_widgets = FALSE;

      *parser = sub_parser;
      *parser_data = data;
      return TRUE;
    }

  return parent_buildable_iface->custom_tag_start (buildable, builder, child,
						   tagname, parser, parser_data);
}

static void
ctk_dialog_buildable_custom_finished (CtkBuildable *buildable,
				      CtkBuilder   *builder,
				      GObject      *child,
				      const gchar  *tagname,
				      gpointer      user_data)
{
  CtkDialog *dialog = CTK_DIALOG (buildable);
  CtkDialogPrivate *priv = dialog->priv;
  GSList *l;
  SubParserData *data;
  GObject *object;
  ResponseData *ad;
  guint signal_id;

  if (strcmp (tagname, "action-widgets") != 0)
    {
      parent_buildable_iface->custom_finished (buildable, builder, child,
                                               tagname, user_data);
      return;
    }

  data = (SubParserData*)user_data;
  data->items = g_slist_reverse (data->items);

  for (l = data->items; l; l = l->next)
    {
      ActionWidgetInfo *item = l->data;
      gboolean is_action;

      object = _ctk_builder_lookup_object (builder, item->widget_name, item->line, item->col);
      if (!object)
        continue;

      /* If the widget already has reponse data at this point, it
       * was either added by ctk_dialog_add_action_widget(), or via
       * <child type="action"> or by moving an action area child
       * to the header bar. In these cases, apply placement heuristics
       * based on the response id.
       */
      is_action = get_response_data (CTK_WIDGET (object), FALSE) != NULL;

      ad = get_response_data (CTK_WIDGET (object), TRUE);
      ad->response_id = item->response_id;

      if (CTK_IS_BUTTON (object))
	signal_id = g_signal_lookup ("clicked", CTK_TYPE_BUTTON);
      else
	signal_id = CTK_WIDGET_GET_CLASS (object)->activate_signal;

      if (signal_id && !is_action)
	{
	  GClosure *closure;

	  closure = g_cclosure_new_object (G_CALLBACK (action_widget_activated),
					   G_OBJECT (dialog));
	  g_signal_connect_closure_by_id (object, signal_id, 0, closure, FALSE);
	}

      if (ctk_widget_get_parent (CTK_WIDGET (object)) == priv->action_area)
        {
          apply_response_for_action_area (dialog, CTK_WIDGET (object), ad->response_id);
        }
      else if (ctk_widget_get_parent (CTK_WIDGET (object)) == priv->headerbar)
        {
          if (is_action)
            apply_response_for_header_bar (dialog, CTK_WIDGET (object), ad->response_id);
        }

      if (item->is_default)
        ctk_widget_grab_default (CTK_WIDGET (object));
    }

  g_slist_free_full (data->items, free_action_widget_info);
  g_string_free (data->string, TRUE);
  g_slice_free (SubParserData, data);

  update_suggested_action (dialog);
}

static void
ctk_dialog_buildable_add_child (CtkBuildable  *buildable,
                                CtkBuilder    *builder,
                                GObject       *child,
                                const gchar   *type)
{
  CtkDialog *dialog = CTK_DIALOG (buildable);
  CtkDialogPrivate *priv = dialog->priv;

  if (type == NULL)
    ctk_container_add (CTK_CONTAINER (buildable), CTK_WIDGET (child));
  else if (g_str_equal (type, "titlebar"))
    {
      priv->headerbar = CTK_WIDGET (child);
      ctk_window_set_titlebar (CTK_WINDOW (buildable), priv->headerbar);
    }
  else if (g_str_equal (type, "action"))
    ctk_dialog_add_action_widget (CTK_DIALOG (buildable), CTK_WIDGET (child), CTK_RESPONSE_NONE);
  else
    CTK_BUILDER_WARN_INVALID_CHILD_TYPE (buildable, type);
}

/**
 * ctk_dialog_get_action_area:
 * @dialog: a #CtkDialog
 *
 * Returns the action area of @dialog.
 *
 * Returns: (type Ctk.Box) (transfer none): the action area
 *
 * Since: 2.14
 */
CtkWidget *
ctk_dialog_get_action_area (CtkDialog *dialog)
{
  g_return_val_if_fail (CTK_IS_DIALOG (dialog), NULL);

  return dialog->priv->action_area;
}

/**
 * ctk_dialog_get_header_bar:
 * @dialog: a #CtkDialog
 *
 * Returns the header bar of @dialog. Note that the
 * headerbar is only used by the dialog if the
 * #CtkDialog:use-header-bar property is %TRUE.
 *
 * Returns: (type Ctk.HeaderBar) (transfer none): the header bar
 *
 * Since: 3.12
 */
CtkWidget *
ctk_dialog_get_header_bar (CtkDialog *dialog)
{
  g_return_val_if_fail (CTK_IS_DIALOG (dialog), NULL);

  return dialog->priv->headerbar;
}

/**
 * ctk_dialog_get_content_area:
 * @dialog: a #CtkDialog
 *
 * Returns the content area of @dialog.
 *
 * Returns: (type Ctk.Box) (transfer none): the content area #CtkBox.
 *
 * Since: 2.14
 **/
CtkWidget *
ctk_dialog_get_content_area (CtkDialog *dialog)
{
  g_return_val_if_fail (CTK_IS_DIALOG (dialog), NULL);

  return dialog->priv->vbox;
}
