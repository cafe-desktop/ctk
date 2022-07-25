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

#include <string.h>
#include <glib.h>
#include "ctkcolorseldialog.h"
#include "ctkframe.h"
#include "ctkbutton.h"
#include "ctkstock.h"
#include "ctkintl.h"
#include "ctkbuildable.h"


/**
 * SECTION:ctkcolorseldlg
 * @Short_description: Dialog box for selecting a color
 * @Title: CtkColorSelectionDialog
 *
 * The #CtkColorSelectionDialog provides a standard dialog which
 * allows the user to select a color much like the #CtkFileChooserDialog
 * provides a standard dialog for file selection.
 *
 * Use ctk_color_selection_dialog_get_color_selection() to get the
 * #CtkColorSelection widget contained within the dialog. Use this widget
 * and its ctk_color_selection_get_current_color()
 * function to gain access to the selected color.  Connect a handler
 * for this widget’s #CtkColorSelection::color-changed signal to be notified
 * when the color changes.
 *
 * # CtkColorSelectionDialog as CtkBuildable # {#CtkColorSelectionDialog-BUILDER-UI}
 *
 * The CtkColorSelectionDialog implementation of the CtkBuildable interface
 * exposes the embedded #CtkColorSelection as internal child with the
 * name “color_selection”. It also exposes the buttons with the names
 * “ok_button”, “cancel_button” and “help_button”.
 */


struct _CtkColorSelectionDialogPrivate
{
  CtkWidget *colorsel;
  CtkWidget *ok_button;
  CtkWidget *cancel_button;
  CtkWidget *help_button;
};

enum {
  PROP_0,
  PROP_COLOR_SELECTION,
  PROP_OK_BUTTON,
  PROP_CANCEL_BUTTON,
  PROP_HELP_BUTTON
};


/***************************/
/* CtkColorSelectionDialog */
/***************************/

static void ctk_color_selection_dialog_buildable_interface_init     (CtkBuildableIface *iface);
static GObject * ctk_color_selection_dialog_buildable_get_internal_child (CtkBuildable *buildable,
									  CtkBuilder   *builder,
									  const gchar  *childname);

G_DEFINE_TYPE_WITH_CODE (CtkColorSelectionDialog, ctk_color_selection_dialog, CTK_TYPE_DIALOG,
                         G_ADD_PRIVATE (CtkColorSelectionDialog)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_BUILDABLE,
                                                ctk_color_selection_dialog_buildable_interface_init))

static CtkBuildableIface *parent_buildable_iface;

static void
ctk_color_selection_dialog_get_property (GObject         *object,
					 guint            prop_id,
					 GValue          *value,
					 GParamSpec      *pspec)
{
  CtkColorSelectionDialog *colorsel = CTK_COLOR_SELECTION_DIALOG (object);
  CtkColorSelectionDialogPrivate *priv = colorsel->priv;

  switch (prop_id)
    {
    case PROP_COLOR_SELECTION:
      g_value_set_object (value, priv->colorsel);
      break;
    case PROP_OK_BUTTON:
      g_value_set_object (value, priv->ok_button);
      break;
    case PROP_CANCEL_BUTTON:
      g_value_set_object (value, priv->cancel_button);
      break;
    case PROP_HELP_BUTTON:
      g_value_set_object (value, priv->help_button);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_color_selection_dialog_class_init (CtkColorSelectionDialogClass *klass)
{
  GObjectClass   *gobject_class = G_OBJECT_CLASS (klass);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);

  gobject_class->get_property = ctk_color_selection_dialog_get_property;

  g_object_class_install_property (gobject_class,
				   PROP_COLOR_SELECTION,
				   g_param_spec_object ("color-selection",
						     P_("Color Selection"),
						     P_("The color selection embedded in the dialog."),
						     CTK_TYPE_WIDGET,
						     G_PARAM_READABLE));
  g_object_class_install_property (gobject_class,
				   PROP_OK_BUTTON,
				   g_param_spec_object ("ok-button",
						     P_("OK Button"),
						     P_("The OK button of the dialog."),
						     CTK_TYPE_WIDGET,
						     G_PARAM_READABLE));
  g_object_class_install_property (gobject_class,
				   PROP_CANCEL_BUTTON,
				   g_param_spec_object ("cancel-button",
						     P_("Cancel Button"),
						     P_("The cancel button of the dialog."),
						     CTK_TYPE_WIDGET,
						     G_PARAM_READABLE));
  g_object_class_install_property (gobject_class,
				   PROP_HELP_BUTTON,
				   g_param_spec_object ("help-button",
						     P_("Help Button"),
						     P_("The help button of the dialog."),
						     CTK_TYPE_WIDGET,
						     G_PARAM_READABLE));

  ctk_widget_class_set_accessible_role (widget_class, ATK_ROLE_COLOR_CHOOSER);
}

static void
ctk_color_selection_dialog_init (CtkColorSelectionDialog *colorseldiag)
{
  CtkColorSelectionDialogPrivate *priv;
  CtkDialog *dialog = CTK_DIALOG (colorseldiag);
  CtkWidget *action_area, *content_area;

  colorseldiag->priv = ctk_color_selection_dialog_get_instance_private (colorseldiag);
  priv = colorseldiag->priv;

  content_area = ctk_dialog_get_content_area (dialog);
  action_area = ctk_dialog_get_action_area (dialog);

  ctk_container_set_border_width (CTK_CONTAINER (dialog), 5);
  ctk_box_set_spacing (CTK_BOX (content_area), 2); /* 2 * 5 + 2 = 12 */
  ctk_container_set_border_width (CTK_CONTAINER (action_area), 5);
  ctk_box_set_spacing (CTK_BOX (action_area), 6);

  priv->colorsel = ctk_color_selection_new ();
  ctk_container_set_border_width (CTK_CONTAINER (priv->colorsel), 5);
  ctk_color_selection_set_has_palette (CTK_COLOR_SELECTION (priv->colorsel), FALSE);
  ctk_color_selection_set_has_opacity_control (CTK_COLOR_SELECTION (priv->colorsel), FALSE);
  ctk_container_add (CTK_CONTAINER (content_area), priv->colorsel);
  ctk_widget_show (priv->colorsel);

  priv->cancel_button = ctk_dialog_add_button (dialog,
                                               _("_Cancel"),
                                               CTK_RESPONSE_CANCEL);

  priv->ok_button = ctk_dialog_add_button (dialog,
                                           _("_Select"),
                                           CTK_RESPONSE_OK);

  ctk_widget_grab_default (priv->ok_button);

  priv->help_button = ctk_dialog_add_button (dialog,
                                             _("_Help"),
                                             CTK_RESPONSE_HELP);

  ctk_widget_hide (priv->help_button);

  ctk_dialog_set_alternative_button_order (dialog,
					   CTK_RESPONSE_OK,
					   CTK_RESPONSE_CANCEL,
					   CTK_RESPONSE_HELP,
					   -1);

  ctk_window_set_title (CTK_WINDOW (colorseldiag),
                        _("Color Selection"));
}

/**
 * ctk_color_selection_dialog_new:
 * @title: a string containing the title text for the dialog.
 *
 * Creates a new #CtkColorSelectionDialog.
 *
 * Returns: a #CtkColorSelectionDialog.
 */
CtkWidget*
ctk_color_selection_dialog_new (const gchar *title)
{
  CtkColorSelectionDialog *colorseldiag;
  
  colorseldiag = g_object_new (CTK_TYPE_COLOR_SELECTION_DIALOG, NULL);

  if (title)
    ctk_window_set_title (CTK_WINDOW (colorseldiag), title);

  ctk_window_set_resizable (CTK_WINDOW (colorseldiag), FALSE);
  
  return CTK_WIDGET (colorseldiag);
}

/**
 * ctk_color_selection_dialog_get_color_selection:
 * @colorsel: a #CtkColorSelectionDialog
 *
 * Retrieves the #CtkColorSelection widget embedded in the dialog.
 *
 * Returns: (transfer none): the embedded #CtkColorSelection
 *
 * Since: 2.14
 **/
CtkWidget*
ctk_color_selection_dialog_get_color_selection (CtkColorSelectionDialog *colorsel)
{
  g_return_val_if_fail (CTK_IS_COLOR_SELECTION_DIALOG (colorsel), NULL);

  return colorsel->priv->colorsel;
}

static void
ctk_color_selection_dialog_buildable_interface_init (CtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->get_internal_child = ctk_color_selection_dialog_buildable_get_internal_child;
}

static GObject *
ctk_color_selection_dialog_buildable_get_internal_child (CtkBuildable *buildable,
							 CtkBuilder   *builder,
							 const gchar  *childname)
{
  CtkColorSelectionDialog *selection_dialog = CTK_COLOR_SELECTION_DIALOG (buildable);
  CtkColorSelectionDialogPrivate *priv = selection_dialog->priv;

  if (g_strcmp0 (childname, "ok_button") == 0)
    return G_OBJECT (priv->ok_button);
  else if (g_strcmp0 (childname, "cancel_button") == 0)
    return G_OBJECT (priv->cancel_button);
  else if (g_strcmp0 (childname, "help_button") == 0)
    return G_OBJECT (priv->help_button);
  else if (g_strcmp0 (childname, "color_selection") == 0)
    return G_OBJECT (priv->colorsel);

  return parent_buildable_iface->get_internal_child (buildable, builder, childname);
}
