/* GTK - The GIMP Toolkit
 * ctkrecentchooserdialog.c: Recent files selector dialog
 * Copyright (C) 2006 Emmanuele Bassi
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

#include "config.h"

#include "ctkrecentchooserdialog.h"
#include "ctkrecentchooserwidget.h"
#include "ctkrecentchooserutils.h"
#include "ctkrecentmanager.h"
#include "ctktypebuiltins.h"
#include "ctksettings.h"
#include "ctkdialogprivate.h"

#include <stdarg.h>


/**
 * SECTION:ctkrecentchooserdialog
 * @Short_description: Displays recently used files in a dialog
 * @Title: GtkRecentChooserDialog
 * @See_also:#GtkRecentChooser, #GtkDialog
 *
 * #GtkRecentChooserDialog is a dialog box suitable for displaying the recently
 * used documents.  This widgets works by putting a #GtkRecentChooserWidget inside
 * a #GtkDialog.  It exposes the #GtkRecentChooserIface interface, so you can use
 * all the #GtkRecentChooser functions on the recent chooser dialog as well as
 * those for #GtkDialog.
 *
 * Note that #GtkRecentChooserDialog does not have any methods of its own.
 * Instead, you should use the functions that work on a #GtkRecentChooser.
 *
 * ## Typical usage ## {#ctkrecentchooser-typical-usage}
 *
 * In the simplest of cases, you can use the following code to use
 * a #GtkRecentChooserDialog to select a recently used file:
 *
 * |[<!-- language="C" -->
 * GtkWidget *dialog;
 * gint res;
 *
 * dialog = ctk_recent_chooser_dialog_new ("Recent Documents",
 *                                         parent_window,
 *                                         _("_Cancel"),
 *                                         CTK_RESPONSE_CANCEL,
 *                                         _("_Open"),
 *                                         CTK_RESPONSE_ACCEPT,
 *                                         NULL);
 *
 * res = ctk_dialog_run (CTK_DIALOG (dialog));
 * if (res == CTK_RESPONSE_ACCEPT)
 *   {
 *     GtkRecentInfo *info;
 *     GtkRecentChooser *chooser = CTK_RECENT_CHOOSER (dialog);
 *
 *     info = ctk_recent_chooser_get_current_item (chooser);
 *     open_file (ctk_recent_info_get_uri (info));
 *     ctk_recent_info_unref (info);
 *   }
 *
 * ctk_widget_destroy (dialog);
 * ]|
 *
 * Recently used files are supported since GTK+ 2.10.
 */


struct _GtkRecentChooserDialogPrivate
{
  GtkRecentManager *manager;
  
  GtkWidget *chooser;
};

#define CTK_RECENT_CHOOSER_DIALOG_GET_PRIVATE(obj)	(CTK_RECENT_CHOOSER_DIALOG (obj)->priv)

static void ctk_recent_chooser_dialog_class_init (GtkRecentChooserDialogClass *klass);
static void ctk_recent_chooser_dialog_init       (GtkRecentChooserDialog      *dialog);
static void ctk_recent_chooser_dialog_finalize   (GObject                     *object);

static void ctk_recent_chooser_dialog_constructed (GObject *object);

static void ctk_recent_chooser_dialog_set_property (GObject      *object,
						    guint         prop_id,
						    const GValue *value,
						    GParamSpec   *pspec);
static void ctk_recent_chooser_dialog_get_property (GObject      *object,
						    guint         prop_id,
						    GValue       *value,
						    GParamSpec   *pspec);

G_DEFINE_TYPE_WITH_CODE (GtkRecentChooserDialog,
			 ctk_recent_chooser_dialog,
			 CTK_TYPE_DIALOG,
                         G_ADD_PRIVATE (GtkRecentChooserDialog)
			 G_IMPLEMENT_INTERFACE (CTK_TYPE_RECENT_CHOOSER,
		       				_ctk_recent_chooser_delegate_iface_init))

static void
ctk_recent_chooser_dialog_class_init (GtkRecentChooserDialogClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  
  gobject_class->set_property = ctk_recent_chooser_dialog_set_property;
  gobject_class->get_property = ctk_recent_chooser_dialog_get_property;
  gobject_class->constructed = ctk_recent_chooser_dialog_constructed;
  gobject_class->finalize = ctk_recent_chooser_dialog_finalize;
  
  _ctk_recent_chooser_install_properties (gobject_class);
}

static void
ctk_recent_chooser_dialog_init (GtkRecentChooserDialog *dialog)
{
  GtkRecentChooserDialogPrivate *priv;
  GtkWidget *content_area, *action_area;
  GtkDialog *rc_dialog = CTK_DIALOG (dialog);

  priv = ctk_recent_chooser_dialog_get_instance_private (dialog);
  dialog->priv = priv;
  ctk_dialog_set_use_header_bar_from_setting (CTK_DIALOG (dialog));

  content_area = ctk_dialog_get_content_area (rc_dialog);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  action_area = ctk_dialog_get_action_area (rc_dialog);
G_GNUC_END_IGNORE_DEPRECATIONS

  ctk_container_set_border_width (CTK_CONTAINER (rc_dialog), 5);
  ctk_box_set_spacing (CTK_BOX (content_area), 2); /* 2 * 5 + 2 = 12 */
  ctk_container_set_border_width (CTK_CONTAINER (action_area), 5);
}

/* we intercept the GtkRecentChooser::item_activated signal and try to
 * make the dialog emit a valid response signal
 */
static void
ctk_recent_chooser_item_activated_cb (GtkRecentChooser *chooser,
				      gpointer          user_data)
{
  GtkDialog *rc_dialog;
  GtkRecentChooserDialog *dialog;
  GtkWidget *action_area;
  GList *children, *l;

  dialog = CTK_RECENT_CHOOSER_DIALOG (user_data);
  rc_dialog = CTK_DIALOG (dialog);

  if (ctk_window_activate_default (CTK_WINDOW (dialog)))
    return;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  action_area = ctk_dialog_get_action_area (rc_dialog);
G_GNUC_END_IGNORE_DEPRECATIONS
  children = ctk_container_get_children (CTK_CONTAINER (action_area));
  
  for (l = children; l; l = l->next)
    {
      GtkWidget *widget;
      gint response_id;
      
      widget = CTK_WIDGET (l->data);
      response_id = ctk_dialog_get_response_for_widget (rc_dialog, widget);
      
      if (response_id == CTK_RESPONSE_ACCEPT ||
          response_id == CTK_RESPONSE_OK     ||
          response_id == CTK_RESPONSE_YES    ||
          response_id == CTK_RESPONSE_APPLY)
        {
          g_list_free (children);
	  
          ctk_dialog_response (CTK_DIALOG (dialog), response_id);

          return;
        }
    }
  
  g_list_free (children);
}

static void
ctk_recent_chooser_dialog_constructed (GObject *object)
{
  GtkRecentChooserDialogPrivate *priv;
  GtkWidget *content_area;

  G_OBJECT_CLASS (ctk_recent_chooser_dialog_parent_class)->constructed (object);
  priv = CTK_RECENT_CHOOSER_DIALOG_GET_PRIVATE (object);

  if (priv->manager)
    priv->chooser = g_object_new (CTK_TYPE_RECENT_CHOOSER_WIDGET,
  				  "recent-manager", priv->manager,
  				  NULL);
  else
    priv->chooser = g_object_new (CTK_TYPE_RECENT_CHOOSER_WIDGET, NULL);
  
  g_signal_connect (priv->chooser, "item-activated",
  		    G_CALLBACK (ctk_recent_chooser_item_activated_cb),
  		    object);

  content_area = ctk_dialog_get_content_area (CTK_DIALOG (object));

  ctk_container_set_border_width (CTK_CONTAINER (priv->chooser), 5);
  ctk_box_pack_start (CTK_BOX (content_area),
                      priv->chooser, TRUE, TRUE, 0);
  ctk_widget_show (priv->chooser);
  
  _ctk_recent_chooser_set_delegate (CTK_RECENT_CHOOSER (object),
  				    CTK_RECENT_CHOOSER (priv->chooser));
}

static void
ctk_recent_chooser_dialog_set_property (GObject      *object,
					guint         prop_id,
					const GValue *value,
					GParamSpec   *pspec)
{
  GtkRecentChooserDialogPrivate *priv;
  
  priv = CTK_RECENT_CHOOSER_DIALOG_GET_PRIVATE (object);
  
  switch (prop_id)
    {
    case CTK_RECENT_CHOOSER_PROP_RECENT_MANAGER:
      priv->manager = g_value_get_object (value);
      break;
    default:
      g_object_set_property (G_OBJECT (priv->chooser), pspec->name, value);
      break;
    }
}

static void
ctk_recent_chooser_dialog_get_property (GObject      *object,
					guint         prop_id,
					GValue       *value,
					GParamSpec   *pspec)
{
  GtkRecentChooserDialogPrivate *priv;
  
  priv = CTK_RECENT_CHOOSER_DIALOG_GET_PRIVATE (object);
  
  g_object_get_property (G_OBJECT (priv->chooser), pspec->name, value);
}

static void
ctk_recent_chooser_dialog_finalize (GObject *object)
{
  GtkRecentChooserDialog *dialog = CTK_RECENT_CHOOSER_DIALOG (object);
 
  dialog->priv->manager = NULL;
  
  G_OBJECT_CLASS (ctk_recent_chooser_dialog_parent_class)->finalize (object);
}

static GtkWidget *
ctk_recent_chooser_dialog_new_valist (const gchar      *title,
				      GtkWindow        *parent,
				      GtkRecentManager *manager,
				      const gchar      *first_button_text,
				      va_list           varargs)
{
  GtkWidget *result;
  const char *button_text = first_button_text;
  gint response_id;

  result = g_object_new (CTK_TYPE_RECENT_CHOOSER_DIALOG,
                         "title", title,
                         "recent-manager", manager,
                         NULL);
  
  if (parent)
    ctk_window_set_transient_for (CTK_WINDOW (result), parent);
  
  while (button_text)
    {
      response_id = va_arg (varargs, gint);
      ctk_dialog_add_button (CTK_DIALOG (result), button_text, response_id);
      button_text = va_arg (varargs, const gchar *);
    }
  
  return result;
}

/**
 * ctk_recent_chooser_dialog_new:
 * @title: (allow-none): Title of the dialog, or %NULL
 * @parent: (allow-none): Transient parent of the dialog, or %NULL,
 * @first_button_text: (allow-none): stock ID or text to go in the first button, or %NULL
 * @...: response ID for the first button, then additional (button, id)
 *   pairs, ending with %NULL
 *
 * Creates a new #GtkRecentChooserDialog.  This function is analogous to
 * ctk_dialog_new_with_buttons().
 *
 * Returns: a new #GtkRecentChooserDialog
 *
 * Since: 2.10
 */
GtkWidget *
ctk_recent_chooser_dialog_new (const gchar *title,
			       GtkWindow   *parent,
			       const gchar *first_button_text,
			       ...)
{
  GtkWidget *result;
  va_list varargs;
  
  va_start (varargs, first_button_text);
  result = ctk_recent_chooser_dialog_new_valist (title,
  						 parent,
  						 NULL,
  						 first_button_text,
  						 varargs);
  va_end (varargs);
  
  return result;
}

/**
 * ctk_recent_chooser_dialog_new_for_manager:
 * @title: (allow-none): Title of the dialog, or %NULL
 * @parent: (allow-none): Transient parent of the dialog, or %NULL,
 * @manager: a #GtkRecentManager
 * @first_button_text: (allow-none): stock ID or text to go in the first button, or %NULL
 * @...: response ID for the first button, then additional (button, id)
 *   pairs, ending with %NULL
 *
 * Creates a new #GtkRecentChooserDialog with a specified recent manager.
 *
 * This is useful if you have implemented your own recent manager, or if you
 * have a customized instance of a #GtkRecentManager object.
 *
 * Returns: a new #GtkRecentChooserDialog
 *
 * Since: 2.10
 */
GtkWidget *
ctk_recent_chooser_dialog_new_for_manager (const gchar      *title,
			                   GtkWindow        *parent,
			                   GtkRecentManager *manager,
			                   const gchar      *first_button_text,
			                   ...)
{
  GtkWidget *result;
  va_list varargs;
  
  va_start (varargs, first_button_text);
  result = ctk_recent_chooser_dialog_new_valist (title,
  						 parent,
  						 manager,
  						 first_button_text,
  						 varargs);
  va_end (varargs);
  
  return result;
}
