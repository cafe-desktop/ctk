/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/* CTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat, Inc.
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

/*
 * Modified by the CTK+ Team and others 1997-2003.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/. 
 */

#include "config.h"

#include <string.h>

#include "ctkmessagedialog.h"
#include "ctkaccessible.h"
#include "ctkbuildable.h"
#include "ctklabel.h"
#include "ctkbox.h"
#include "ctkbbox.h"
#include "ctkimage.h"
#include "ctkintl.h"
#include "ctkprivate.h"
#include "ctktypebuiltins.h"

/**
 * SECTION:ctkmessagedialog
 * @Short_description: A convenient message window
 * @Title: CtkMessageDialog
 * @See_also:#CtkDialog
 *
 * #CtkMessageDialog presents a dialog with some message text. It’s simply a
 * convenience widget; you could construct the equivalent of #CtkMessageDialog
 * from #CtkDialog without too much effort, but #CtkMessageDialog saves typing.
 *
 * One difference from #CtkDialog is that #CtkMessageDialog sets the
 * #CtkWindow:skip-taskbar-hint property to %TRUE, so that the dialog is hidden
 * from the taskbar by default.
 *
 * The easiest way to do a modal message dialog is to use ctk_dialog_run(), though
 * you can also pass in the %CTK_DIALOG_MODAL flag, ctk_dialog_run() automatically
 * makes the dialog modal and waits for the user to respond to it. ctk_dialog_run()
 * returns when any dialog button is clicked.
 *
 * An example for using a modal dialog:
 * |[<!-- language="C" -->
 *  CtkDialogFlags flags = CTK_DIALOG_DESTROY_WITH_PARENT;
 *  dialog = ctk_message_dialog_new (parent_window,
 *                                   flags,
 *                                   CTK_MESSAGE_ERROR,
 *                                   CTK_BUTTONS_CLOSE,
 *                                   "Error reading “%s”: %s",
 *                                   filename,
 *                                   g_strerror (errno));
 *  ctk_dialog_run (CTK_DIALOG (dialog));
 *  ctk_widget_destroy (dialog);
 * ]|
 *
 * You might do a non-modal #CtkMessageDialog as follows:
 *
 * An example for a non-modal dialog:
 * |[<!-- language="C" -->
 *  CtkDialogFlags flags = CTK_DIALOG_DESTROY_WITH_PARENT;
 *  dialog = ctk_message_dialog_new (parent_window,
 *                                   flags,
 *                                   CTK_MESSAGE_ERROR,
 *                                   CTK_BUTTONS_CLOSE,
 *                                   "Error reading “%s”: %s",
 *                                   filename,
 *                                   g_strerror (errno));
 *
 *  // Destroy the dialog when the user responds to it
 *  // (e.g. clicks a button)
 *
 *  g_signal_connect_swapped (dialog, "response",
 *                            G_CALLBACK (ctk_widget_destroy),
 *                            dialog);
 * ]|
 *
 * # CtkMessageDialog as CtkBuildable
 *
 * The CtkMessageDialog implementation of the CtkBuildable interface exposes
 * the message area as an internal child with the name “message_area”.
 */

struct _CtkMessageDialogPrivate
{
  CtkWidget     *image;
  CtkWidget     *label;
  CtkWidget     *message_area; /* vbox for the primary and secondary labels, and any extra content from the caller */
  CtkWidget     *secondary_label;

  guint          has_primary_markup : 1;
  guint          has_secondary_text : 1;
  guint          message_type       : 3;
};

static void ctk_message_dialog_style_updated (CtkWidget       *widget);

static void ctk_message_dialog_constructed  (GObject          *object);
static void ctk_message_dialog_set_property (GObject          *object,
					     guint             prop_id,
					     const GValue     *value,
					     GParamSpec       *pspec);
static void ctk_message_dialog_get_property (GObject          *object,
					     guint             prop_id,
					     GValue           *value,
					     GParamSpec       *pspec);
static void ctk_message_dialog_add_buttons  (CtkMessageDialog *message_dialog,
					     CtkButtonsType    buttons);
static void      ctk_message_dialog_buildable_interface_init     (CtkBuildableIface *iface);

enum {
  PROP_0,
  PROP_MESSAGE_TYPE,
  PROP_BUTTONS,
  PROP_TEXT,
  PROP_USE_MARKUP,
  PROP_SECONDARY_TEXT,
  PROP_SECONDARY_USE_MARKUP,
  PROP_IMAGE,
  PROP_MESSAGE_AREA
};

G_DEFINE_TYPE_WITH_CODE (CtkMessageDialog, ctk_message_dialog, CTK_TYPE_DIALOG,
                         G_ADD_PRIVATE (CtkMessageDialog)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_BUILDABLE,
                                                ctk_message_dialog_buildable_interface_init))

static CtkBuildableIface *parent_buildable_iface;

static void
ctk_message_dialog_buildable_interface_init (CtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->custom_tag_start = parent_buildable_iface->custom_tag_start;
  iface->custom_finished = parent_buildable_iface->custom_finished;
}

static void
ctk_message_dialog_class_init (CtkMessageDialogClass *class)
{
  CtkWidgetClass *widget_class;
  GObjectClass *gobject_class;

  widget_class = CTK_WIDGET_CLASS (class);
  gobject_class = G_OBJECT_CLASS (class);
  
  widget_class->style_updated = ctk_message_dialog_style_updated;

  ctk_widget_class_set_accessible_role (widget_class, ATK_ROLE_ALERT);

  gobject_class->constructed = ctk_message_dialog_constructed;
  gobject_class->set_property = ctk_message_dialog_set_property;
  gobject_class->get_property = ctk_message_dialog_get_property;
  
  ctk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("message-border",
                                                             P_("label border"),
                                                             P_("Width of border around the label in the message dialog"),
                                                             0,
                                                             G_MAXINT,
                                                             12,
                                                             CTK_PARAM_READABLE));

  /**
   * CtkMessageDialog:message-type:
   *
   * The type of the message.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_MESSAGE_TYPE,
                                   g_param_spec_enum ("message-type",
						      P_("Message Type"),
						      P_("The type of message"),
						      CTK_TYPE_MESSAGE_TYPE,
                                                      CTK_MESSAGE_INFO,
                                                      CTK_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY));
  g_object_class_install_property (gobject_class,
                                   PROP_BUTTONS,
                                   g_param_spec_enum ("buttons",
						      P_("Message Buttons"),
						      P_("The buttons shown in the message dialog"),
						      CTK_TYPE_BUTTONS_TYPE,
                                                      CTK_BUTTONS_NONE,
                                                      CTK_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));

  /**
   * CtkMessageDialog:text:
   * 
   * The primary text of the message dialog. If the dialog has 
   * a secondary text, this will appear as the title.
   *
   * Since: 2.10
   */
  g_object_class_install_property (gobject_class,
                                   PROP_TEXT,
                                   g_param_spec_string ("text",
                                                        P_("Text"),
                                                        P_("The primary text of the message dialog"),
                                                        "",
                                                        CTK_PARAM_READWRITE));

  /**
   * CtkMessageDialog:use-markup:
   * 
   * %TRUE if the primary text of the dialog includes Pango markup. 
   * See pango_parse_markup(). 
   *
   * Since: 2.10
   */
  g_object_class_install_property (gobject_class,
				   PROP_USE_MARKUP,
				   g_param_spec_boolean ("use-markup",
							 P_("Use Markup"),
							 P_("The primary text of the title includes Pango markup."),
							 FALSE,
							 CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
  
  /**
   * CtkMessageDialog:secondary-text:
   * 
   * The secondary text of the message dialog. 
   *
   * Since: 2.10
   */
  g_object_class_install_property (gobject_class,
                                   PROP_SECONDARY_TEXT,
                                   g_param_spec_string ("secondary-text",
                                                        P_("Secondary Text"),
                                                        P_("The secondary text of the message dialog"),
                                                        NULL,
                                                        CTK_PARAM_READWRITE));

  /**
   * CtkMessageDialog:secondary-use-markup:
   * 
   * %TRUE if the secondary text of the dialog includes Pango markup. 
   * See pango_parse_markup(). 
   *
   * Since: 2.10
   */
  g_object_class_install_property (gobject_class,
				   PROP_SECONDARY_USE_MARKUP,
				   g_param_spec_boolean ("secondary-use-markup",
							 P_("Use Markup in secondary"),
							 P_("The secondary text includes Pango markup."),
							 FALSE,
							 CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkMessageDialog:image:
   *
   * The image for this dialog.
   *
   * Since: 2.10
   */
  g_object_class_install_property (gobject_class,
                                   PROP_IMAGE,
                                   g_param_spec_object ("image",
                                                        P_("Image"),
                                                        P_("The image"),
                                                        CTK_TYPE_WIDGET,
                                                        CTK_PARAM_READWRITE));

  /**
   * CtkMessageDialog:message-area:
   *
   * The #CtkBox that corresponds to the message area of this dialog.  See
   * ctk_message_dialog_get_message_area() for a detailed description of this
   * area.
   *
   * Since: 2.22
   */
  g_object_class_install_property (gobject_class,
				   PROP_MESSAGE_AREA,
				   g_param_spec_object ("message-area",
							P_("Message area"),
							P_("CtkBox that holds the dialog's primary and secondary labels"),
							CTK_TYPE_WIDGET,
							CTK_PARAM_READABLE));

  /* Setup Composite data */
  ctk_widget_class_set_template_from_resource (widget_class, "/org/ctk/libctk/ui/ctkmessagedialog.ui");
  ctk_widget_class_bind_template_child_private (widget_class, CtkMessageDialog, label);
  ctk_widget_class_bind_template_child_private (widget_class, CtkMessageDialog, secondary_label);
  ctk_widget_class_bind_template_child_internal_private (widget_class, CtkMessageDialog, message_area);

  ctk_widget_class_set_css_name (widget_class, "messagedialog");
}

static void
ctk_message_dialog_init (CtkMessageDialog *dialog)
{
  CtkMessageDialogPrivate *priv;
  CtkWidget *action_area;
  CtkSettings *settings;
  gboolean use_caret;

  dialog->priv = ctk_message_dialog_get_instance_private (dialog);
  priv = dialog->priv;

  priv->has_primary_markup = FALSE;
  priv->has_secondary_text = FALSE;
  priv->has_primary_markup = FALSE;
  priv->has_secondary_text = FALSE;
  priv->message_type = CTK_MESSAGE_OTHER;

  ctk_widget_init_template (CTK_WIDGET (dialog));
  ctk_message_dialog_style_updated (CTK_WIDGET (dialog));

  action_area = ctk_dialog_get_action_area (CTK_DIALOG (dialog));

  ctk_button_box_set_layout (CTK_BUTTON_BOX (action_area), CTK_BUTTONBOX_EXPAND);

  settings = ctk_widget_get_settings (CTK_WIDGET (dialog));
  g_object_get (settings, "ctk-keynav-use-caret", &use_caret, NULL);
  ctk_label_set_selectable (CTK_LABEL (priv->label), use_caret);
  ctk_label_set_selectable (CTK_LABEL (priv->secondary_label), use_caret);
}

static void
setup_primary_label_font (CtkMessageDialog *dialog)
{
  CtkMessageDialogPrivate *priv = dialog->priv;

  if (!priv->has_primary_markup)
    {
      PangoAttrList *attributes;
      PangoAttribute *attr;

      attributes = pango_attr_list_new ();

      attr = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
      pango_attr_list_insert (attributes, attr);

      if (priv->has_secondary_text)
        {
          attr = pango_attr_scale_new (PANGO_SCALE_LARGE);
          pango_attr_list_insert (attributes, attr);
        }

      ctk_label_set_attributes (CTK_LABEL (priv->label), attributes);
      pango_attr_list_unref (attributes);
    }
  else
    {
      /* unset the font settings */
      ctk_label_set_attributes (CTK_LABEL (priv->label), NULL);
    }
}

static void
setup_type (CtkMessageDialog *dialog,
	    CtkMessageType    type)
{
  CtkMessageDialogPrivate *priv = dialog->priv;
  const gchar *name = NULL;
  AtkObject *atk_obj;

  if (priv->message_type == type)
    return;

  priv->message_type = type;

  switch (type)
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
      g_warning ("Unknown CtkMessageType %u", type);
      break;
    }

  atk_obj = ctk_widget_get_accessible (CTK_WIDGET (dialog));
  if (CTK_IS_ACCESSIBLE (atk_obj))
    {
      atk_object_set_role (atk_obj, ATK_ROLE_ALERT);
      if (name)
        atk_object_set_name (atk_obj, name);
    }

  g_object_notify (G_OBJECT (dialog), "message-type");
}

static void
update_title (GObject    *dialog,
              GParamSpec *pspec G_GNUC_UNUSED,
              CtkWidget  *label)
{
  const gchar *title;

  title = ctk_window_get_title (CTK_WINDOW (dialog));
  ctk_label_set_label (CTK_LABEL (label), title);
  ctk_widget_set_visible (label, title && title[0]);
}

static void
ctk_message_dialog_constructed (GObject *object)
{
  CtkMessageDialog *dialog = CTK_MESSAGE_DIALOG (object);
  gboolean use_header;

  G_OBJECT_CLASS (ctk_message_dialog_parent_class)->constructed (object);

  g_object_get (ctk_widget_get_settings (CTK_WIDGET (dialog)),
                "ctk-dialogs-use-header", &use_header,
                NULL);

  if (use_header)
    {
      CtkWidget *box;
      CtkWidget *label;

      box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
      ctk_widget_show (box);
      ctk_widget_set_size_request (box, -1, 16);
      label = ctk_label_new ("");
      ctk_widget_set_no_show_all (label, TRUE);
      ctk_widget_set_margin_top (label, 6);
      ctk_widget_set_margin_bottom (label, 6);
      ctk_style_context_add_class (ctk_widget_get_style_context (label), "title");
      ctk_box_set_center_widget (CTK_BOX (box), label);
      g_signal_connect_object (dialog, "notify::title", G_CALLBACK (update_title), label, 0);

      ctk_window_set_titlebar (CTK_WINDOW (dialog), box);
    }
}

static void 
ctk_message_dialog_set_property (GObject      *object,
				 guint         prop_id,
				 const GValue *value,
				 GParamSpec   *pspec)
{
  CtkMessageDialog *dialog = CTK_MESSAGE_DIALOG (object);
  CtkMessageDialogPrivate *priv = dialog->priv;

  switch (prop_id)
    {
    case PROP_MESSAGE_TYPE:
      setup_type (dialog, g_value_get_enum (value));
      break;
    case PROP_BUTTONS:
      ctk_message_dialog_add_buttons (dialog, g_value_get_enum (value));
      break;
    case PROP_TEXT:
      if (priv->has_primary_markup)
	ctk_label_set_markup (CTK_LABEL (priv->label),
			      g_value_get_string (value));
      else
	ctk_label_set_text (CTK_LABEL (priv->label),
			    g_value_get_string (value));
      break;
    case PROP_USE_MARKUP:
      if (priv->has_primary_markup != g_value_get_boolean (value))
        {
          priv->has_primary_markup = g_value_get_boolean (value);
          ctk_label_set_use_markup (CTK_LABEL (priv->label), priv->has_primary_markup);
          g_object_notify_by_pspec (object, pspec);
        }
        setup_primary_label_font (dialog);
      break;
    case PROP_SECONDARY_TEXT:
      {
	const gchar *txt = g_value_get_string (value);

	if (ctk_label_get_use_markup (CTK_LABEL (priv->secondary_label)))
	  ctk_label_set_markup (CTK_LABEL (priv->secondary_label), txt);
	else
	  ctk_label_set_text (CTK_LABEL (priv->secondary_label), txt);

	if (txt)
	  {
	    priv->has_secondary_text = TRUE;
	    ctk_widget_show (priv->secondary_label);
	  }
	else
	  {
	    priv->has_secondary_text = FALSE;
	    ctk_widget_hide (priv->secondary_label);
	  }
	setup_primary_label_font (dialog);
      }
      break;
    case PROP_SECONDARY_USE_MARKUP:
      if (ctk_label_get_use_markup (CTK_LABEL (priv->secondary_label)) != g_value_get_boolean (value))
        {
          ctk_label_set_use_markup (CTK_LABEL (priv->secondary_label), g_value_get_boolean (value));
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_IMAGE:
      ctk_message_dialog_set_image (dialog, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void 
ctk_message_dialog_get_property (GObject     *object,
				 guint        prop_id,
				 GValue      *value,
				 GParamSpec  *pspec)
{
  CtkMessageDialog *dialog = CTK_MESSAGE_DIALOG (object);
  CtkMessageDialogPrivate *priv = dialog->priv;

  switch (prop_id)
    {
    case PROP_MESSAGE_TYPE:
      g_value_set_enum (value, (CtkMessageType) priv->message_type);
      break;
    case PROP_TEXT:
      g_value_set_string (value, ctk_label_get_label (CTK_LABEL (priv->label)));
      break;
    case PROP_USE_MARKUP:
      g_value_set_boolean (value, priv->has_primary_markup);
      break;
    case PROP_SECONDARY_TEXT:
      if (priv->has_secondary_text)
      g_value_set_string (value, 
			  ctk_label_get_label (CTK_LABEL (priv->secondary_label)));
      else
	g_value_set_string (value, NULL);
      break;
    case PROP_SECONDARY_USE_MARKUP:
      if (priv->has_secondary_text)
	g_value_set_boolean (value, 
			     ctk_label_get_use_markup (CTK_LABEL (priv->secondary_label)));
      else
	g_value_set_boolean (value, FALSE);
      break;
    case PROP_IMAGE:
      g_value_set_object (value, priv->image);
      break;
    case PROP_MESSAGE_AREA:
      g_value_set_object (value, priv->message_area);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * ctk_message_dialog_new:
 * @parent: (allow-none): transient parent, or %NULL for none
 * @flags: flags
 * @type: type of message
 * @buttons: set of buttons to use
 * @message_format: (allow-none): printf()-style format string, or %NULL
 * @...: arguments for @message_format
 *
 * Creates a new message dialog, which is a simple dialog with some text
 * the user may want to see. When the user clicks a button a “response”
 * signal is emitted with response IDs from #CtkResponseType. See
 * #CtkDialog for more details.
 *
 * Returns: (transfer none): a new #CtkMessageDialog
 */
CtkWidget*
ctk_message_dialog_new (CtkWindow     *parent,
                        CtkDialogFlags flags,
                        CtkMessageType type,
                        CtkButtonsType buttons,
                        const gchar   *message_format,
                        ...)
{
  CtkWidget *widget;
  CtkDialog *dialog;
  gchar* msg = NULL;
  va_list args;

  g_return_val_if_fail (parent == NULL || CTK_IS_WINDOW (parent), NULL);

  widget = g_object_new (CTK_TYPE_MESSAGE_DIALOG,
                         "use-header-bar", FALSE,
			 "message-type", type,
			 "buttons", buttons,
			 NULL);
  dialog = CTK_DIALOG (widget);

  if (message_format)
    {
      va_start (args, message_format);
      msg = g_strdup_vprintf (message_format, args);
      va_end (args);

      ctk_label_set_text (CTK_LABEL (CTK_MESSAGE_DIALOG (widget)->priv->label), msg);

      g_free (msg);
    }

  if (parent != NULL)
    ctk_window_set_transient_for (CTK_WINDOW (widget), CTK_WINDOW (parent));
  
  if (flags & CTK_DIALOG_MODAL)
    ctk_window_set_modal (CTK_WINDOW (dialog), TRUE);

  if (flags & CTK_DIALOG_DESTROY_WITH_PARENT)
    ctk_window_set_destroy_with_parent (CTK_WINDOW (dialog), TRUE);

  return widget;
}

/**
 * ctk_message_dialog_new_with_markup:
 * @parent: (allow-none): transient parent, or %NULL for none
 * @flags: flags
 * @type: type of message
 * @buttons: set of buttons to use
 * @message_format: (allow-none): printf()-style format string, or %NULL
 * @...: arguments for @message_format
 *
 * Creates a new message dialog, which is a simple dialog with some text that
 * is marked up with the [Pango text markup language][PangoMarkupFormat].
 * When the user clicks a button a “response” signal is emitted with
 * response IDs from #CtkResponseType. See #CtkDialog for more details.
 *
 * Special XML characters in the printf() arguments passed to this
 * function will automatically be escaped as necessary.
 * (See g_markup_printf_escaped() for how this is implemented.)
 * Usually this is what you want, but if you have an existing
 * Pango markup string that you want to use literally as the
 * label, then you need to use ctk_message_dialog_set_markup()
 * instead, since you can’t pass the markup string either
 * as the format (it might contain “%” characters) or as a string
 * argument.
 * |[<!-- language="C" -->
 *  CtkWidget *dialog;
 *  CtkDialogFlags flags = CTK_DIALOG_DESTROY_WITH_PARENT;
 *  dialog = ctk_message_dialog_new (parent_window,
 *                                   flags,
 *                                   CTK_MESSAGE_ERROR,
 *                                   CTK_BUTTONS_CLOSE,
 *                                   NULL);
 *  ctk_message_dialog_set_markup (CTK_MESSAGE_DIALOG (dialog),
 *                                 markup);
 * ]|
 * 
 * Returns: a new #CtkMessageDialog
 *
 * Since: 2.4
 **/
CtkWidget*
ctk_message_dialog_new_with_markup (CtkWindow     *parent,
                                    CtkDialogFlags flags,
                                    CtkMessageType type,
                                    CtkButtonsType buttons,
                                    const gchar   *message_format,
                                    ...)
{
  CtkWidget *widget;
  va_list args;
  gchar *msg = NULL;

  g_return_val_if_fail (parent == NULL || CTK_IS_WINDOW (parent), NULL);

  widget = ctk_message_dialog_new (parent, flags, type, buttons, NULL);

  if (message_format)
    {
      va_start (args, message_format);
      msg = g_markup_vprintf_escaped (message_format, args);
      va_end (args);

      ctk_message_dialog_set_markup (CTK_MESSAGE_DIALOG (widget), msg);

      g_free (msg);
    }

  return widget;
}

/**
 * ctk_message_dialog_set_image:
 * @dialog: a #CtkMessageDialog
 * @image: the image
 * 
 * Sets the dialog’s image to @image.
 *
 * Since: 2.10
 **/
void
ctk_message_dialog_set_image (CtkMessageDialog *dialog,
			      CtkWidget        *image)
{
  CtkMessageDialogPrivate *priv;
  CtkWidget *parent;

  g_return_if_fail (CTK_IS_MESSAGE_DIALOG (dialog));
  g_return_if_fail (image == NULL || CTK_IS_WIDGET (image));

  priv = dialog->priv;
  
  if (priv->image)
    ctk_widget_destroy (priv->image);

  priv->image = image;
 
  if (priv->image)
    { 
      ctk_widget_set_halign (priv->image, CTK_ALIGN_CENTER);
      ctk_widget_set_valign (priv->image, CTK_ALIGN_START);
      parent = ctk_widget_get_parent (priv->message_area);
      ctk_container_add (CTK_CONTAINER (parent), priv->image);
      ctk_box_reorder_child (CTK_BOX (parent), priv->image, 0);
    }

  priv->message_type = CTK_MESSAGE_OTHER;

  g_object_notify (G_OBJECT (dialog), "image");
  g_object_notify (G_OBJECT (dialog), "message-type");
}

/**
 * ctk_message_dialog_get_image:
 * @dialog: a #CtkMessageDialog
 *
 * Gets the dialog’s image.
 *
 * Returns: (transfer none): the dialog’s image
 *
 * Since: 2.14
 **/
CtkWidget *
ctk_message_dialog_get_image (CtkMessageDialog *dialog)
{
  g_return_val_if_fail (CTK_IS_MESSAGE_DIALOG (dialog), NULL);

  return dialog->priv->image;
}

/**
 * ctk_message_dialog_set_markup:
 * @message_dialog: a #CtkMessageDialog
 * @str: markup string (see [Pango markup format][PangoMarkupFormat])
 * 
 * Sets the text of the message dialog to be @str, which is marked
 * up with the [Pango text markup language][PangoMarkupFormat].
 *
 * Since: 2.4
 **/
void
ctk_message_dialog_set_markup (CtkMessageDialog *message_dialog,
                               const gchar      *str)
{
  CtkMessageDialogPrivate *priv;

  g_return_if_fail (CTK_IS_MESSAGE_DIALOG (message_dialog));

  priv = message_dialog->priv;

  priv->has_primary_markup = TRUE;
  ctk_label_set_markup (CTK_LABEL (priv->label), str);
}

/**
 * ctk_message_dialog_format_secondary_text:
 * @message_dialog: a #CtkMessageDialog
 * @message_format: (allow-none): printf()-style format string, or %NULL
 * @...: arguments for @message_format
 *
 * Sets the secondary text of the message dialog to be @message_format
 * (with printf()-style).
 *
 * Since: 2.6
 */
void
ctk_message_dialog_format_secondary_text (CtkMessageDialog *message_dialog,
                                          const gchar      *message_format,
                                          ...)
{
  va_list args;
  gchar *msg = NULL;
  CtkMessageDialogPrivate *priv;

  g_return_if_fail (CTK_IS_MESSAGE_DIALOG (message_dialog));

  priv = message_dialog->priv;

  if (message_format)
    {
      priv->has_secondary_text = TRUE;

      va_start (args, message_format);
      msg = g_strdup_vprintf (message_format, args);
      va_end (args);

      ctk_widget_show (priv->secondary_label);
      ctk_label_set_text (CTK_LABEL (priv->secondary_label), msg);

      g_free (msg);
    }
  else
    {
      priv->has_secondary_text = FALSE;
      ctk_widget_hide (priv->secondary_label);
    }

  setup_primary_label_font (message_dialog);
}

/**
 * ctk_message_dialog_format_secondary_markup:
 * @message_dialog: a #CtkMessageDialog
 * @message_format: printf()-style markup string (see
     [Pango markup format][PangoMarkupFormat]), or %NULL
 * @...: arguments for @message_format
 *
 * Sets the secondary text of the message dialog to be @message_format (with
 * printf()-style), which is marked up with the
 * [Pango text markup language][PangoMarkupFormat].
 *
 * Due to an oversight, this function does not escape special XML characters
 * like ctk_message_dialog_new_with_markup() does. Thus, if the arguments
 * may contain special XML characters, you should use g_markup_printf_escaped()
 * to escape it.

 * |[<!-- language="C" -->
 * gchar *msg;
 *
 * msg = g_markup_printf_escaped (message_format, ...);
 * ctk_message_dialog_format_secondary_markup (message_dialog,
 *                                             "%s", msg);
 * g_free (msg);
 * ]|
 *
 * Since: 2.6
 */
void
ctk_message_dialog_format_secondary_markup (CtkMessageDialog *message_dialog,
                                            const gchar      *message_format,
                                            ...)
{
  va_list args;
  gchar *msg = NULL;
  CtkMessageDialogPrivate *priv;

  g_return_if_fail (CTK_IS_MESSAGE_DIALOG (message_dialog));

  priv = message_dialog->priv;

  if (message_format)
    {
      priv->has_secondary_text = TRUE;

      va_start (args, message_format);
      msg = g_strdup_vprintf (message_format, args);
      va_end (args);

      ctk_widget_show (priv->secondary_label);
      ctk_label_set_markup (CTK_LABEL (priv->secondary_label), msg);

      g_free (msg);
    }
  else
    {
      priv->has_secondary_text = FALSE;
      ctk_widget_hide (priv->secondary_label);
    }

  setup_primary_label_font (message_dialog);
}

/**
 * ctk_message_dialog_get_message_area:
 * @message_dialog: a #CtkMessageDialog
 *
 * Returns the message area of the dialog. This is the box where the
 * dialog’s primary and secondary labels are packed. You can add your
 * own extra content to that box and it will appear below those labels.
 * See ctk_dialog_get_content_area() for the corresponding
 * function in the parent #CtkDialog.
 *
 * Returns: (transfer none): A #CtkBox corresponding to the
 *     “message area” in the @message_dialog.
 *
 * Since: 2.22
 **/
CtkWidget *
ctk_message_dialog_get_message_area (CtkMessageDialog *message_dialog)
{
  g_return_val_if_fail (CTK_IS_MESSAGE_DIALOG (message_dialog), NULL);

  return message_dialog->priv->message_area;
}

static void
ctk_message_dialog_add_buttons (CtkMessageDialog* message_dialog,
				CtkButtonsType buttons)
{
  CtkDialog* dialog = CTK_DIALOG (message_dialog);

  switch (buttons)
    {
    case CTK_BUTTONS_NONE:
      /* nothing */
      break;

    case CTK_BUTTONS_OK:
      ctk_dialog_add_button (dialog, _("_OK"), CTK_RESPONSE_OK);
      break;

    case CTK_BUTTONS_CLOSE:
      ctk_dialog_add_button (dialog, _("_Close"), CTK_RESPONSE_CLOSE);
      break;

    case CTK_BUTTONS_CANCEL:
      ctk_dialog_add_button (dialog, _("_Cancel"), CTK_RESPONSE_CANCEL);
      break;

    case CTK_BUTTONS_YES_NO:
      ctk_dialog_add_button (dialog, _("_No"), CTK_RESPONSE_NO);
      ctk_dialog_add_button (dialog, _("_Yes"), CTK_RESPONSE_YES);
      ctk_dialog_set_alternative_button_order (CTK_DIALOG (dialog),
					       CTK_RESPONSE_YES,
					       CTK_RESPONSE_NO,
					       -1);
      break;

    case CTK_BUTTONS_OK_CANCEL:
      ctk_dialog_add_button (dialog, _("_Cancel"), CTK_RESPONSE_CANCEL);
      ctk_dialog_add_button (dialog, _("_OK"), CTK_RESPONSE_OK);
      ctk_dialog_set_alternative_button_order (CTK_DIALOG (dialog),
					       CTK_RESPONSE_OK,
					       CTK_RESPONSE_CANCEL,
					       -1);
      break;
      
    default:
      g_warning ("Unknown CtkButtonsType");
      break;
    } 

  g_object_notify (G_OBJECT (message_dialog), "buttons");
}

static void
ctk_message_dialog_style_updated (CtkWidget *widget)
{
  CtkMessageDialog *dialog = CTK_MESSAGE_DIALOG (widget);
  CtkWidget *parent;
  gint border_width;

  parent = ctk_widget_get_parent (dialog->priv->message_area);

  if (parent)
    {
      ctk_widget_style_get (widget, "message-border",
                            &border_width, NULL);

      ctk_container_set_border_width (CTK_CONTAINER (parent),
                                      MAX (0, border_width - 7));
    }

  CTK_WIDGET_CLASS (ctk_message_dialog_parent_class)->style_updated (widget);
}
