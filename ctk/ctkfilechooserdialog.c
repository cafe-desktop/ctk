/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* CTK - The GIMP Toolkit
 * ctkfilechooserdialog.c: File selector dialog
 * Copyright (C) 2003, Red Hat, Inc.
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

#include "ctkfilechooserdialog.h"

#include "ctkfilechooserprivate.h"
#include "ctkfilechooserwidget.h"
#include "ctkfilechooserwidgetprivate.h"
#include "ctkfilechooserutils.h"
#include "ctkfilechooserembed.h"
#include "ctkfilesystem.h"
#include "ctksizerequest.h"
#include "ctktypebuiltins.h"
#include "ctkintl.h"
#include "ctksettings.h"
#include "ctktogglebutton.h"
#include "ctkstylecontext.h"
#include "ctkheaderbar.h"
#include "ctkdialogprivate.h"
#include "ctklabel.h"
#include "ctkfilechooserentry.h"

#include <stdarg.h>


/**
 * SECTION:ctkfilechooserdialog
 * @Short_description: A file chooser dialog, suitable for “File/Open” or “File/Save” commands
 * @Title: CtkFileChooserDialog
 * @See_also: #CtkFileChooser, #CtkDialog, CtkFileChooserNative
 *
 * #CtkFileChooserDialog is a dialog box suitable for use with
 * “File/Open” or “File/Save as” commands.  This widget works by
 * putting a #CtkFileChooserWidget inside a #CtkDialog.  It exposes
 * the #CtkFileChooser interface, so you can use all of the
 * #CtkFileChooser functions on the file chooser dialog as well as
 * those for #CtkDialog.
 *
 * Note that #CtkFileChooserDialog does not have any methods of its
 * own.  Instead, you should use the functions that work on a
 * #CtkFileChooser.
 *
 * If you want to integrate well with the platform you should use the
 * #CtkFileChooserNative API, which will use a platform-specific
 * dialog if available and fall back to CtkFileChooserDialog
 * otherwise.
 *
 * ## Typical usage ## {#ctkfilechooser-typical-usage}
 *
 * In the simplest of cases, you can the following code to use
 * #CtkFileChooserDialog to select a file for opening:
 *
 * |[
 * CtkWidget *dialog;
 * CtkFileChooserAction action = CTK_FILE_CHOOSER_ACTION_OPEN;
 * gint res;
 *
 * dialog = ctk_file_chooser_dialog_new ("Open File",
 *                                       parent_window,
 *                                       action,
 *                                       _("_Cancel"),
 *                                       CTK_RESPONSE_CANCEL,
 *                                       _("_Open"),
 *                                       CTK_RESPONSE_ACCEPT,
 *                                       NULL);
 *
 * res = ctk_dialog_run (CTK_DIALOG (dialog));
 * if (res == CTK_RESPONSE_ACCEPT)
 *   {
 *     char *filename;
 *     CtkFileChooser *chooser = CTK_FILE_CHOOSER (dialog);
 *     filename = ctk_file_chooser_get_filename (chooser);
 *     open_file (filename);
 *     g_free (filename);
 *   }
 *
 * ctk_widget_destroy (dialog);
 * ]|
 *
 * To use a dialog for saving, you can use this:
 *
 * |[
 * CtkWidget *dialog;
 * CtkFileChooser *chooser;
 * CtkFileChooserAction action = CTK_FILE_CHOOSER_ACTION_SAVE;
 * gint res;
 *
 * dialog = ctk_file_chooser_dialog_new ("Save File",
 *                                       parent_window,
 *                                       action,
 *                                       _("_Cancel"),
 *                                       CTK_RESPONSE_CANCEL,
 *                                       _("_Save"),
 *                                       CTK_RESPONSE_ACCEPT,
 *                                       NULL);
 * chooser = CTK_FILE_CHOOSER (dialog);
 *
 * ctk_file_chooser_set_do_overwrite_confirmation (chooser, TRUE);
 *
 * if (user_edited_a_new_document)
 *   ctk_file_chooser_set_current_name (chooser,
 *                                      _("Untitled document"));
 * else
 *   ctk_file_chooser_set_filename (chooser,
 *                                  existing_filename);
 *
 * res = ctk_dialog_run (CTK_DIALOG (dialog));
 * if (res == CTK_RESPONSE_ACCEPT)
 *   {
 *     char *filename;
 *
 *     filename = ctk_file_chooser_get_filename (chooser);
 *     save_to_file (filename);
 *     g_free (filename);
 *   }
 *
 * ctk_widget_destroy (dialog);
 * ]|
 *
 * ## Setting up a file chooser dialog ## {#ctkfilechooserdialog-setting-up}
 *
 * There are various cases in which you may need to use a #CtkFileChooserDialog:
 *
 * - To select a file for opening. Use #CTK_FILE_CHOOSER_ACTION_OPEN.
 *
 * - To save a file for the first time. Use #CTK_FILE_CHOOSER_ACTION_SAVE,
 *   and suggest a name such as “Untitled” with ctk_file_chooser_set_current_name().
 *
 * - To save a file under a different name. Use #CTK_FILE_CHOOSER_ACTION_SAVE,
 *   and set the existing filename with ctk_file_chooser_set_filename().
 *
 * - To choose a folder instead of a file. Use #CTK_FILE_CHOOSER_ACTION_SELECT_FOLDER.
 *
 * Note that old versions of the file chooser’s documentation suggested
 * using ctk_file_chooser_set_current_folder() in various
 * situations, with the intention of letting the application
 * suggest a reasonable default folder.  This is no longer
 * considered to be a good policy, as now the file chooser is
 * able to make good suggestions on its own.  In general, you
 * should only cause the file chooser to show a specific folder
 * when it is appropriate to use ctk_file_chooser_set_filename(),
 * i.e. when you are doing a Save As command and you already
 * have a file saved somewhere.

 * ## Response Codes ## {#ctkfilechooserdialog-responses}
 *
 * #CtkFileChooserDialog inherits from #CtkDialog, so buttons that
 * go in its action area have response codes such as
 * #CTK_RESPONSE_ACCEPT and #CTK_RESPONSE_CANCEL.  For example, you
 * could call ctk_file_chooser_dialog_new() as follows:
 *
 * |[
 * CtkWidget *dialog;
 * CtkFileChooserAction action = CTK_FILE_CHOOSER_ACTION_OPEN;
 *
 * dialog = ctk_file_chooser_dialog_new ("Open File",
 *                                       parent_window,
 *                                       action,
 *                                       _("_Cancel"),
 *                                       CTK_RESPONSE_CANCEL,
 *                                       _("_Open"),
 *                                       CTK_RESPONSE_ACCEPT,
 *                                       NULL);
 * ]|
 *
 * This will create buttons for “Cancel” and “Open” that use stock
 * response identifiers from #CtkResponseType.  For most dialog
 * boxes you can use your own custom response codes rather than the
 * ones in #CtkResponseType, but #CtkFileChooserDialog assumes that
 * its “accept”-type action, e.g. an “Open” or “Save” button,
 * will have one of the following response codes:
 *
 * - #CTK_RESPONSE_ACCEPT
 * - #CTK_RESPONSE_OK
 * - #CTK_RESPONSE_YES
 * - #CTK_RESPONSE_APPLY
 *
 * This is because #CtkFileChooserDialog must intercept responses
 * and switch to folders if appropriate, rather than letting the
 * dialog terminate — the implementation uses these known
 * response codes to know which responses can be blocked if
 * appropriate.
 *
 * To summarize, make sure you use a
 * [stock response code][ctkfilechooserdialog-responses]
 * when you use #CtkFileChooserDialog to ensure proper operation.
 */


struct _CtkFileChooserDialogPrivate
{
  CtkWidget *widget;

  CtkSizeGroup *buttons;

  /* for use with CtkFileChooserEmbed */
  gboolean response_requested;
  gboolean search_setup;
  gboolean has_entry;
};

static void     ctk_file_chooser_dialog_set_property (GObject               *object,
                                                      guint                  prop_id,
                                                      const GValue          *value,
                                                      GParamSpec            *pspec);
static void     ctk_file_chooser_dialog_get_property (GObject               *object,
                                                      guint                  prop_id,
                                                      GValue                *value,
                                                      GParamSpec            *pspec);
static void     ctk_file_chooser_dialog_notify       (GObject               *object,
                                                      GParamSpec            *pspec);

static void     ctk_file_chooser_dialog_map          (CtkWidget             *widget);
static void     ctk_file_chooser_dialog_unmap        (CtkWidget             *widget);
static void     ctk_file_chooser_dialog_size_allocate (CtkWidget             *widget,
                                                       CtkAllocation         *allocation);
static void     file_chooser_widget_file_activated   (CtkFileChooser        *chooser,
                                                      CtkFileChooserDialog  *dialog);
static void     file_chooser_widget_default_size_changed (CtkWidget            *widget,
                                                          CtkFileChooserDialog *dialog);
static void     file_chooser_widget_response_requested (CtkWidget            *widget,
                                                        CtkFileChooserDialog *dialog);
static void     file_chooser_widget_selection_changed (CtkWidget            *widget,
                                                        CtkFileChooserDialog *dialog);

static void response_cb (CtkDialog *dialog,
                         gint       response_id);

static void setup_save_entry (CtkFileChooserDialog *dialog);

G_DEFINE_TYPE_WITH_CODE (CtkFileChooserDialog, ctk_file_chooser_dialog, CTK_TYPE_DIALOG,
                         G_ADD_PRIVATE (CtkFileChooserDialog)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_FILE_CHOOSER,
                                                _ctk_file_chooser_delegate_iface_init))

static void
ctk_file_chooser_dialog_class_init (CtkFileChooserDialogClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (class);

  gobject_class->set_property = ctk_file_chooser_dialog_set_property;
  gobject_class->get_property = ctk_file_chooser_dialog_get_property;
  gobject_class->notify = ctk_file_chooser_dialog_notify;

  widget_class->map = ctk_file_chooser_dialog_map;
  widget_class->unmap = ctk_file_chooser_dialog_unmap;
  widget_class->size_allocate = ctk_file_chooser_dialog_size_allocate;

  ctk_widget_class_set_accessible_role (widget_class, ATK_ROLE_FILE_CHOOSER);

  _ctk_file_chooser_install_properties (gobject_class);

  /* Bind class to template
   */
  ctk_widget_class_set_template_from_resource (widget_class,
                                               "/org/ctk/libctk/ui/ctkfilechooserdialog.ui");

  ctk_widget_class_bind_template_child_private (widget_class, CtkFileChooserDialog, widget);
  ctk_widget_class_bind_template_child_private (widget_class, CtkFileChooserDialog, buttons);
  ctk_widget_class_bind_template_callback (widget_class, response_cb);
  ctk_widget_class_bind_template_callback (widget_class, file_chooser_widget_file_activated);
  ctk_widget_class_bind_template_callback (widget_class, file_chooser_widget_default_size_changed);
  ctk_widget_class_bind_template_callback (widget_class, file_chooser_widget_response_requested);
  ctk_widget_class_bind_template_callback (widget_class, file_chooser_widget_selection_changed);
}

static void
ctk_file_chooser_dialog_init (CtkFileChooserDialog *dialog)
{
  dialog->priv = ctk_file_chooser_dialog_get_instance_private (dialog);
  dialog->priv->response_requested = FALSE;

  ctk_widget_init_template (CTK_WIDGET (dialog));
  ctk_dialog_set_use_header_bar_from_setting (CTK_DIALOG (dialog));

  _ctk_file_chooser_set_delegate (CTK_FILE_CHOOSER (dialog),
                                  CTK_FILE_CHOOSER (dialog->priv->widget));
}

static CtkWidget *
get_accept_action_widget (CtkDialog *dialog,
                          gboolean   sensitive_only)
{
  gint response[] = {
    CTK_RESPONSE_ACCEPT,
    CTK_RESPONSE_OK,
    CTK_RESPONSE_YES,
    CTK_RESPONSE_APPLY
  };
  gint i;
  CtkWidget *widget;

  for (i = 0; i < G_N_ELEMENTS (response); i++)
    {
      widget = ctk_dialog_get_widget_for_response (dialog, response[i]);
      if (widget)
        {
          if (!sensitive_only)
            return widget;

          if (ctk_widget_is_sensitive (widget))
            return widget;
        }
    }

  return NULL;
}

static gboolean
is_stock_accept_response_id (gint response_id)
{
  return (response_id == CTK_RESPONSE_ACCEPT ||
          response_id == CTK_RESPONSE_OK ||
          response_id == CTK_RESPONSE_YES ||
          response_id == CTK_RESPONSE_APPLY);
}

/* Callback used when the user activates a file in the file chooser widget */
static void
file_chooser_widget_file_activated (CtkFileChooser       *chooser G_GNUC_UNUSED,
                                    CtkFileChooserDialog *dialog)
{
  CtkWidget *widget;

  if (ctk_window_activate_default (CTK_WINDOW (dialog)))
    return;

  /* There probably isn't a default widget, so make things easier for the
   * programmer by looking for a reasonable button on our own.
   */
  widget = get_accept_action_widget (CTK_DIALOG (dialog), TRUE);
  if (widget)
    ctk_widget_activate (widget);
}

static void
file_chooser_widget_default_size_changed (CtkWidget            *widget,
                                          CtkFileChooserDialog *dialog)
{
  CtkFileChooserDialogPrivate *priv;
  gint default_width, default_height;
  CtkRequisition req, widget_req;

  priv = ctk_file_chooser_dialog_get_instance_private (dialog);

  /* Unset any previously set size */
  ctk_widget_set_size_request (CTK_WIDGET (dialog), -1, -1);

  if (ctk_widget_is_drawable (widget))
    {
      /* Force a size request of everything before we start. This will make sure
       * that widget->requisition is meaningful.
       */
      ctk_widget_get_preferred_size (CTK_WIDGET (dialog), &req, NULL);
      ctk_widget_get_preferred_size (widget, &widget_req, NULL);
    }

  _ctk_file_chooser_embed_get_default_size (CTK_FILE_CHOOSER_EMBED (priv->widget),
                                            &default_width, &default_height);

  ctk_window_resize (CTK_WINDOW (dialog), default_width, default_height);
}

static void
file_chooser_widget_selection_changed (CtkWidget            *widget G_GNUC_UNUSED,
                                       CtkFileChooserDialog *dialog)
{
  CtkWidget *button;
  GSList *uris;
  gboolean sensitive;

  button = get_accept_action_widget (CTK_DIALOG (dialog), FALSE);
  if (button == NULL)
    return;

  uris = ctk_file_chooser_get_uris (CTK_FILE_CHOOSER (dialog->priv->widget));
  sensitive = (uris != NULL);
  ctk_widget_set_sensitive (button, sensitive);

  g_slist_free_full (uris, g_free);
}

static void
file_chooser_widget_response_requested (CtkWidget            *widget G_GNUC_UNUSED,
                                        CtkFileChooserDialog *dialog)
{
  CtkWidget *button;

  dialog->priv->response_requested = TRUE;

  if (ctk_window_activate_default (CTK_WINDOW (dialog)))
    return;

  /* There probably isn't a default widget, so make things easier for the
   * programmer by looking for a reasonable button on our own.
   */
  button = get_accept_action_widget (CTK_DIALOG (dialog), TRUE);
  if (button)
    {
      ctk_widget_activate (button);
      return;
    }

  dialog->priv->response_requested = FALSE;
}

static void
ctk_file_chooser_dialog_set_property (GObject      *object,
                                      guint         prop_id G_GNUC_UNUSED,
                                      const GValue *value,
                                      GParamSpec   *pspec)

{
  CtkFileChooserDialogPrivate *priv;

  priv = ctk_file_chooser_dialog_get_instance_private (CTK_FILE_CHOOSER_DIALOG (object));

  g_object_set_property (G_OBJECT (priv->widget), pspec->name, value);
}

static void
ctk_file_chooser_dialog_get_property (GObject    *object,
                                      guint       prop_id G_GNUC_UNUSED,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  CtkFileChooserDialogPrivate *priv;

  priv = ctk_file_chooser_dialog_get_instance_private (CTK_FILE_CHOOSER_DIALOG (object));

  g_object_get_property (G_OBJECT (priv->widget), pspec->name, value);
}

static void
ctk_file_chooser_dialog_notify (GObject    *object,
                                GParamSpec *pspec)
{
  if (strcmp (pspec->name, "action") == 0)
    setup_save_entry (CTK_FILE_CHOOSER_DIALOG (object));

  if (G_OBJECT_CLASS (ctk_file_chooser_dialog_parent_class)->notify)
    G_OBJECT_CLASS (ctk_file_chooser_dialog_parent_class)->notify (object, pspec);
}

static void
add_button (CtkWidget *button, gpointer data)
{
  CtkFileChooserDialog *dialog = data;

  if (CTK_IS_BUTTON (button))
    ctk_size_group_add_widget (dialog->priv->buttons, button);
}

static void
setup_search (CtkFileChooserDialog *dialog)
{
  gboolean use_header;

  if (dialog->priv->search_setup)
    return;

  dialog->priv->search_setup = TRUE;

  g_object_get (dialog, "use-header-bar", &use_header, NULL);
  if (use_header)
    {
      CtkWidget *button;
      CtkWidget *image;
      CtkWidget *header;

      button = ctk_toggle_button_new ();
      ctk_widget_set_focus_on_click (button, FALSE);
      ctk_widget_set_valign (button, CTK_ALIGN_CENTER);
      image = ctk_image_new_from_icon_name ("edit-find-symbolic", CTK_ICON_SIZE_MENU);
      ctk_container_add (CTK_CONTAINER (button), image);
      ctk_style_context_add_class (ctk_widget_get_style_context (button), "image-button");
      ctk_style_context_remove_class (ctk_widget_get_style_context (button), "text-button");
      ctk_widget_show (image);
      ctk_widget_show (button);

      header = ctk_dialog_get_header_bar (CTK_DIALOG (dialog));
      ctk_header_bar_pack_end (CTK_HEADER_BAR (header), button);

      g_object_bind_property (button, "active",
                              dialog->priv->widget, "search-mode",
                              G_BINDING_BIDIRECTIONAL);
      g_object_bind_property (dialog->priv->widget, "subtitle",
                              header, "subtitle",
                              G_BINDING_SYNC_CREATE);

      ctk_container_forall (CTK_CONTAINER (header), add_button, dialog);
    }
}

static void
setup_save_entry (CtkFileChooserDialog *dialog)
{
  gboolean use_header;
  CtkFileChooserAction action;
  gboolean need_entry;
  CtkWidget *header;

  g_object_get (dialog,
                "use-header-bar", &use_header,
                "action", &action,
                NULL);

  if (!use_header)
    return;

  header = ctk_dialog_get_header_bar (CTK_DIALOG (dialog));

  need_entry = action == CTK_FILE_CHOOSER_ACTION_SAVE ||
               action == CTK_FILE_CHOOSER_ACTION_CREATE_FOLDER;

  if (need_entry && !dialog->priv->has_entry)
    {
      CtkWidget *box;
      CtkWidget *label;
      CtkWidget *entry;

      box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
      label = ctk_label_new_with_mnemonic (_("_Name"));
      entry = _ctk_file_chooser_entry_new (FALSE, FALSE);
      g_object_set (label, "margin-start", 6, "margin-end", 6, NULL);
      g_object_set (entry, "margin-start", 6, "margin-end", 6, NULL);
      ctk_label_set_mnemonic_widget (CTK_LABEL (label), entry);
      ctk_container_add (CTK_CONTAINER (box), label);
      ctk_container_add (CTK_CONTAINER (box), entry);
      ctk_widget_show_all (box);

      ctk_header_bar_set_custom_title (CTK_HEADER_BAR (header), box);
      ctk_file_chooser_widget_set_save_entry (CTK_FILE_CHOOSER_WIDGET (dialog->priv->widget), entry);
    }
  else if (!need_entry && dialog->priv->has_entry)
    {
      ctk_header_bar_set_custom_title (CTK_HEADER_BAR (header), NULL);
      ctk_file_chooser_widget_set_save_entry (CTK_FILE_CHOOSER_WIDGET (dialog->priv->widget), NULL);
    }

  dialog->priv->has_entry = need_entry;
}

static void
ensure_default_response (CtkFileChooserDialog *dialog)
{
  CtkWidget *widget;

  widget = get_accept_action_widget (CTK_DIALOG (dialog), TRUE);
  if (widget)
    ctk_widget_grab_default (widget);
}

static void
ctk_file_chooser_dialog_map (CtkWidget *widget)
{
  CtkFileChooserDialog *dialog = CTK_FILE_CHOOSER_DIALOG (widget);
  CtkFileChooserDialogPrivate *priv = dialog->priv;

  setup_search (dialog);
  setup_save_entry (dialog);
  ensure_default_response (dialog);

  _ctk_file_chooser_embed_initial_focus (CTK_FILE_CHOOSER_EMBED (priv->widget));

  CTK_WIDGET_CLASS (ctk_file_chooser_dialog_parent_class)->map (widget);
}

static void
save_dialog_geometry (CtkFileChooserDialog *dialog)
{
  CtkWindow *window;
  GSettings *settings;
  int old_x, old_y, old_width, old_height;
  int x, y, width, height;

  settings = _ctk_file_chooser_get_settings_for_widget (CTK_WIDGET (dialog));

  window = CTK_WINDOW (dialog);

  ctk_window_get_position (window, &x, &y);
  ctk_window_get_size (window, &width, &height);

  g_settings_get (settings, SETTINGS_KEY_WINDOW_POSITION, "(ii)", &old_x, &old_y);
  if (old_x != x || old_y != y)
    g_settings_set (settings, SETTINGS_KEY_WINDOW_POSITION, "(ii)", x, y);

  g_settings_get (settings, SETTINGS_KEY_WINDOW_SIZE, "(ii)", &old_width, &old_height);
  if (old_width != width || old_height != height)
    g_settings_set (settings, SETTINGS_KEY_WINDOW_SIZE, "(ii)", width, height);

  g_settings_apply (settings);
}

static void
ctk_file_chooser_dialog_unmap (CtkWidget *widget)
{
  CtkFileChooserDialog *dialog = CTK_FILE_CHOOSER_DIALOG (widget);

  save_dialog_geometry (dialog);

  CTK_WIDGET_CLASS (ctk_file_chooser_dialog_parent_class)->unmap (widget);
}

static void
ctk_file_chooser_dialog_size_allocate (CtkWidget     *widget,
                                       CtkAllocation *allocation)
{
  CTK_WIDGET_CLASS (ctk_file_chooser_dialog_parent_class)->size_allocate (widget, allocation);

  if (ctk_widget_is_drawable (widget))
    save_dialog_geometry (CTK_FILE_CHOOSER_DIALOG (widget));
}

/* We do a signal connection here rather than overriding the method in
 * class_init because CtkDialog::response is a RUN_LAST signal.  We want *our*
 * handler to be run *first*, regardless of whether the user installs response
 * handlers of his own.
 */
static void
response_cb (CtkDialog *dialog,
             gint       response_id)
{
  CtkFileChooserDialogPrivate *priv;

  priv = ctk_file_chooser_dialog_get_instance_private (CTK_FILE_CHOOSER_DIALOG (dialog));

  /* Act only on response IDs we recognize */
  if (is_stock_accept_response_id (response_id) &&
      !priv->response_requested &&
      !_ctk_file_chooser_embed_should_respond (CTK_FILE_CHOOSER_EMBED (priv->widget)))
    {
      g_signal_stop_emission_by_name (dialog, "response");
    }

  priv->response_requested = FALSE;
}

static CtkWidget *
ctk_file_chooser_dialog_new_valist (const gchar          *title,
                                    CtkWindow            *parent,
                                    CtkFileChooserAction  action,
                                    const gchar          *first_button_text,
                                    va_list               varargs)
{
  CtkWidget *result;
  const char *button_text = first_button_text;
  gint response_id;

  result = g_object_new (CTK_TYPE_FILE_CHOOSER_DIALOG,
                         "title", title,
                         "action", action,
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
 * ctk_file_chooser_dialog_new:
 * @title: (allow-none): Title of the dialog, or %NULL
 * @parent: (allow-none): Transient parent of the dialog, or %NULL
 * @action: Open or save mode for the dialog
 * @first_button_text: (allow-none): stock ID or text to go in the first button, or %NULL
 * @...: response ID for the first button, then additional (button, id) pairs, ending with %NULL
 *
 * Creates a new #CtkFileChooserDialog.  This function is analogous to
 * ctk_dialog_new_with_buttons().
 *
 * Returns: a new #CtkFileChooserDialog
 *
 * Since: 2.4
 **/
CtkWidget *
ctk_file_chooser_dialog_new (const gchar          *title,
                             CtkWindow            *parent,
                             CtkFileChooserAction  action,
                             const gchar          *first_button_text,
                             ...)
{
  CtkWidget *result;
  va_list varargs;

  va_start (varargs, first_button_text);
  result = ctk_file_chooser_dialog_new_valist (title, parent, action,
                                               first_button_text,
                                               varargs);
  va_end (varargs);

  return result;
}
