/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* CTK - The GIMP Toolkit
 * Copyright (C) Christian Kellner <gicmo@gnome.org>
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

#include <errno.h>
#include <string.h>

#include "ctkmountoperationprivate.h"
#include "ctkbox.h"
#include "ctkcssiconthemevalueprivate.h"
#include "ctkdbusgenerated.h"
#include "ctkentry.h"
#include "ctkbox.h"
#include "ctkintl.h"
#include "ctklabel.h"
#include "ctkmessagedialog.h"
#include "ctkmountoperation.h"
#include "ctkprivate.h"
#include "ctkradiobutton.h"
#include "ctkgrid.h"
#include "ctkwindow.h"
#include "ctktreeview.h"
#include "ctktreeselection.h"
#include "ctkcellrenderertext.h"
#include "ctkcellrendererpixbuf.h"
#include "ctkscrolledwindow.h"
#include "ctkicontheme.h"
#include "ctkmenuitem.h"
#include "ctkmain.h"
#include "ctksettings.h"
#include "ctkstylecontextprivate.h"

#include <glib/gprintf.h>

/**
 * SECTION:filesystem
 * @short_description: Functions for working with GIO
 * @Title: Filesystem utilities
 *
 * The functions and objects described here make working with CTK+ and
 * GIO more convenient.
 *
 * #CtkMountOperation is needed when mounting volumes:
 * It is an implementation of #GMountOperation that can be used with
 * GIO functions for mounting volumes such as
 * g_file_mount_enclosing_volume(), g_file_mount_mountable(),
 * g_volume_mount(), g_mount_unmount_with_operation() and others.
 *
 * When necessary, #CtkMountOperation shows dialogs to ask for
 * passwords, questions or show processes blocking unmount.
 *
 * ctk_show_uri_on_window() is a convenient way to launch applications for URIs.
 *
 * Another object that is worth mentioning in this context is
 * #CdkAppLaunchContext, which provides visual feedback when lauching
 * applications.
 */

static void   ctk_mount_operation_finalize     (GObject          *object);
static void   ctk_mount_operation_set_property (GObject          *object,
                                                guint             prop_id,
                                                const GValue     *value,
                                                GParamSpec       *pspec);
static void   ctk_mount_operation_get_property (GObject          *object,
                                                guint             prop_id,
                                                GValue           *value,
                                                GParamSpec       *pspec);

static void   ctk_mount_operation_ask_password (GMountOperation *op,
                                                const char      *message,
                                                const char      *default_user,
                                                const char      *default_domain,
                                                GAskPasswordFlags flags);

static void   ctk_mount_operation_ask_question (GMountOperation *op,
                                                const char      *message,
                                                const char      *choices[]);

static void   ctk_mount_operation_show_processes (GMountOperation *op,
                                                  const char      *message,
                                                  GArray          *processes,
                                                  const char      *choices[]);

static void   ctk_mount_operation_aborted      (GMountOperation *op);

struct _CtkMountOperationPrivate {
  CtkWindow *parent_window;
  CtkDialog *dialog;
  CdkScreen *screen;

  /* bus proxy */
  _CtkMountOperationHandler *handler;
  GCancellable *cancellable;
  gboolean handler_showing;

  /* for the ask-password dialog */
  CtkWidget *grid;
  CtkWidget *username_entry;
  CtkWidget *domain_entry;
  CtkWidget *password_entry;
  CtkWidget *pim_entry;
  CtkWidget *anonymous_toggle;
  CtkWidget *tcrypt_hidden_toggle;
  CtkWidget *tcrypt_system_toggle;
  GList *user_widgets;

  GAskPasswordFlags ask_flags;
  GPasswordSave     password_save;
  gboolean          anonymous;

  /* for the show-processes dialog */
  CtkWidget *process_tree_view;
  CtkListStore *process_list_store;
};

enum {
  PROP_0,
  PROP_PARENT,
  PROP_IS_SHOWING,
  PROP_SCREEN

};

G_DEFINE_TYPE_WITH_PRIVATE (CtkMountOperation, ctk_mount_operation, G_TYPE_MOUNT_OPERATION)

static void
ctk_mount_operation_class_init (CtkMountOperationClass *klass)
{
  GObjectClass         *object_class = G_OBJECT_CLASS (klass);
  GMountOperationClass *mount_op_class = G_MOUNT_OPERATION_CLASS (klass);

  object_class->finalize     = ctk_mount_operation_finalize;
  object_class->get_property = ctk_mount_operation_get_property;
  object_class->set_property = ctk_mount_operation_set_property;

  mount_op_class->ask_password = ctk_mount_operation_ask_password;
  mount_op_class->ask_question = ctk_mount_operation_ask_question;
  mount_op_class->show_processes = ctk_mount_operation_show_processes;
  mount_op_class->aborted = ctk_mount_operation_aborted;

  g_object_class_install_property (object_class,
                                   PROP_PARENT,
                                   g_param_spec_object ("parent",
                                                        P_("Parent"),
                                                        P_("The parent window"),
                                                        CTK_TYPE_WINDOW,
                                                        CTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_IS_SHOWING,
                                   g_param_spec_boolean ("is-showing",
                                                         P_("Is Showing"),
                                                         P_("Are we showing a dialog"),
                                                         FALSE,
                                                         CTK_PARAM_READABLE));

  g_object_class_install_property (object_class,
                                   PROP_SCREEN,
                                   g_param_spec_object ("screen",
                                                        P_("Screen"),
                                                        P_("The screen where this window will be displayed."),
                                                        CDK_TYPE_SCREEN,
                                                        CTK_PARAM_READWRITE));
}

static void
ctk_mount_operation_init (CtkMountOperation *operation)
{
  gchar *name_owner;

  operation->priv = ctk_mount_operation_get_instance_private (operation);

  operation->priv->handler =
    _ctk_mount_operation_handler_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                         G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                                                         "org.ctk.MountOperationHandler",
                                                         "/org/ctk/MountOperationHandler",
                                                         NULL, NULL);
  name_owner = g_dbus_proxy_get_name_owner (G_DBUS_PROXY (operation->priv->handler));
  if (!name_owner)
    g_clear_object (&operation->priv->handler);
  g_free (name_owner);

  if (operation->priv->handler)
    g_dbus_proxy_set_default_timeout (G_DBUS_PROXY (operation->priv->handler), G_MAXINT);
}

static void
ctk_mount_operation_finalize (GObject *object)
{
  CtkMountOperation *operation = CTK_MOUNT_OPERATION (object);
  CtkMountOperationPrivate *priv = operation->priv;

  if (priv->user_widgets)
    g_list_free (priv->user_widgets);

  if (priv->parent_window)
    {
      g_signal_handlers_disconnect_by_func (priv->parent_window,
                                            ctk_widget_destroyed,
                                            &priv->parent_window);
      g_object_unref (priv->parent_window);
    }

  if (priv->screen)
    g_object_unref (priv->screen);

  if (priv->handler)
    g_object_unref (priv->handler);

  G_OBJECT_CLASS (ctk_mount_operation_parent_class)->finalize (object);
}

static void
ctk_mount_operation_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  CtkMountOperation *operation = CTK_MOUNT_OPERATION (object);

  switch (prop_id)
    {
    case PROP_PARENT:
      ctk_mount_operation_set_parent (operation, g_value_get_object (value));
      break;

    case PROP_SCREEN:
      ctk_mount_operation_set_screen (operation, g_value_get_object (value));
      break;

    case PROP_IS_SHOWING:
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_mount_operation_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  CtkMountOperation *operation = CTK_MOUNT_OPERATION (object);
  CtkMountOperationPrivate *priv = operation->priv;

  switch (prop_id)
    {
    case PROP_PARENT:
      g_value_set_object (value, priv->parent_window);
      break;

    case PROP_IS_SHOWING:
      g_value_set_boolean (value, priv->dialog != NULL || priv->handler_showing);
      break;

    case PROP_SCREEN:
      g_value_set_object (value, priv->screen);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_mount_operation_proxy_finish (CtkMountOperation     *op,
                                  GMountOperationResult  result)
{
  _ctk_mount_operation_handler_call_close (op->priv->handler, NULL, NULL, NULL);

  op->priv->handler_showing = FALSE;
  g_object_notify (G_OBJECT (op), "is-showing");

  g_mount_operation_reply (G_MOUNT_OPERATION (op), result);

  /* drop the reference acquired when calling the proxy method */
  g_object_unref (op);
}

static void
remember_button_toggled (CtkToggleButton   *button,
                         CtkMountOperation *operation)
{
  CtkMountOperationPrivate *priv = operation->priv;

  if (ctk_toggle_button_get_active (button))
    {
      gpointer data;

      data = g_object_get_data (G_OBJECT (button), "password-save");
      priv->password_save = GPOINTER_TO_INT (data);
    }
}

static void
pw_dialog_got_response (CtkDialog         *dialog,
                        gint               response_id,
                        CtkMountOperation *mount_op)
{
  CtkMountOperationPrivate *priv = mount_op->priv;
  GMountOperation *op = G_MOUNT_OPERATION (mount_op);

  if (response_id == CTK_RESPONSE_OK)
    {
      const char *text;

      if (priv->ask_flags & G_ASK_PASSWORD_ANONYMOUS_SUPPORTED)
        g_mount_operation_set_anonymous (op, priv->anonymous);

      if (priv->username_entry)
        {
          text = ctk_entry_get_text (CTK_ENTRY (priv->username_entry));
          g_mount_operation_set_username (op, text);
        }

      if (priv->domain_entry)
        {
          text = ctk_entry_get_text (CTK_ENTRY (priv->domain_entry));
          g_mount_operation_set_domain (op, text);
        }

      if (priv->password_entry)
        {
          text = ctk_entry_get_text (CTK_ENTRY (priv->password_entry));
          g_mount_operation_set_password (op, text);
        }

      if (priv->pim_entry)
        {
          text = ctk_entry_get_text (CTK_ENTRY (priv->pim_entry));
          if (text && strlen (text) > 0)
            {
              guint64 pim;
              gchar *end = NULL;

              errno = 0;
              pim = g_ascii_strtoull (text, &end, 10);
              if (errno == 0 && pim <= G_MAXUINT && end != text)
                g_mount_operation_set_pim (op, (guint) pim);
            }
        }

      if (priv->tcrypt_hidden_toggle && ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (priv->tcrypt_hidden_toggle)))
        g_mount_operation_set_is_tcrypt_hidden_volume (op, TRUE);

      if (priv->tcrypt_system_toggle && ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (priv->tcrypt_system_toggle)))
        g_mount_operation_set_is_tcrypt_system_volume (op, TRUE);

      if (priv->ask_flags & G_ASK_PASSWORD_SAVING_SUPPORTED)
        g_mount_operation_set_password_save (op, priv->password_save);

      g_mount_operation_reply (op, G_MOUNT_OPERATION_HANDLED);
    }
  else
    g_mount_operation_reply (op, G_MOUNT_OPERATION_ABORTED);

  priv->dialog = NULL;
  g_object_notify (G_OBJECT (op), "is-showing");
  ctk_widget_destroy (CTK_WIDGET (dialog));
  g_object_unref (op);
}

static gboolean
entry_has_input (CtkWidget *entry_widget)
{
  const char *text;

  if (entry_widget == NULL)
    return TRUE;

  text = ctk_entry_get_text (CTK_ENTRY (entry_widget));

  return text != NULL && text[0] != '\0';
}

static gboolean
pim_entry_is_valid (CtkWidget *entry_widget)
{
  const char *text;
  gchar *end = NULL;
  guint64 pim;

  if (entry_widget == NULL)
    return TRUE;

  text = ctk_entry_get_text (CTK_ENTRY (entry_widget));
  /* An empty PIM entry is OK */
  if (text == NULL || text[0] == '\0')
    return TRUE;

  errno = 0;
  pim = g_ascii_strtoull (text, &end, 10);
  if (errno || pim > G_MAXUINT || end == text)
    return FALSE;
  else
    return TRUE;
}

static gboolean
pw_dialog_input_is_valid (CtkMountOperation *operation)
{
  CtkMountOperationPrivate *priv = operation->priv;
  gboolean is_valid = TRUE;

  /* We don't require password to be non-empty here
   * since there are situations where it is not needed,
   * see bug 578365.
   * We may add a way for the backend to specify that it
   * definitively needs a password.
   */
  is_valid = entry_has_input (priv->username_entry) &&
             entry_has_input (priv->domain_entry) &&
             pim_entry_is_valid (priv->pim_entry);

  return is_valid;
}

static void
pw_dialog_verify_input (CtkEditable       *editable G_GNUC_UNUSED,
                        CtkMountOperation *operation)
{
  CtkMountOperationPrivate *priv = operation->priv;
  gboolean is_valid;

  is_valid = pw_dialog_input_is_valid (operation);
  ctk_dialog_set_response_sensitive (CTK_DIALOG (priv->dialog),
                                     CTK_RESPONSE_OK,
                                     is_valid);
}

static void
pw_dialog_anonymous_toggled (CtkWidget         *widget,
                             CtkMountOperation *operation)
{
  CtkMountOperationPrivate *priv = operation->priv;
  gboolean is_valid;
  GList *l;

  priv->anonymous = widget == priv->anonymous_toggle;

  if (priv->anonymous)
    is_valid = TRUE;
  else
    is_valid = pw_dialog_input_is_valid (operation);

  for (l = priv->user_widgets; l != NULL; l = l->next)
    {
      ctk_widget_set_sensitive (CTK_WIDGET (l->data), !priv->anonymous);
    }

  ctk_dialog_set_response_sensitive (CTK_DIALOG (priv->dialog),
                                     CTK_RESPONSE_OK,
                                     is_valid);
}


static void
pw_dialog_cycle_focus (CtkWidget         *widget,
                       CtkMountOperation *operation)
{
  CtkMountOperationPrivate *priv;
  CtkWidget *next_widget = NULL;

  priv = operation->priv;

  if (widget == priv->username_entry)
    {
      if (priv->domain_entry != NULL)
        next_widget = priv->domain_entry;
      else if (priv->password_entry != NULL)
        next_widget = priv->password_entry;
    }
  else if (widget == priv->domain_entry && priv->password_entry)
    next_widget = priv->password_entry;

  if (next_widget)
    ctk_widget_grab_focus (next_widget);
  else if (pw_dialog_input_is_valid (operation))
    ctk_window_activate_default (CTK_WINDOW (priv->dialog));
}

static CtkWidget *
table_add_entry (CtkMountOperation *operation,
                 int                row,
                 const char        *label_text,
                 const char        *value,
                 gpointer           user_data)
{
  CtkWidget *entry;
  CtkWidget *label;

  label = ctk_label_new_with_mnemonic (label_text);
  ctk_widget_set_halign (label, CTK_ALIGN_END);
  ctk_widget_set_valign (label, CTK_ALIGN_CENTER);
  ctk_widget_set_hexpand (label, FALSE);
  operation->priv->user_widgets = g_list_prepend (operation->priv->user_widgets, label);

  entry = ctk_entry_new ();
  ctk_widget_set_hexpand (entry, TRUE);

  if (value)
    ctk_entry_set_text (CTK_ENTRY (entry), value);

  ctk_grid_attach (CTK_GRID (operation->priv->grid), label, 0, row, 1, 1);
  ctk_grid_attach (CTK_GRID (operation->priv->grid), entry, 1, row, 1, 1);
  ctk_label_set_mnemonic_widget (CTK_LABEL (label), entry);
  operation->priv->user_widgets = g_list_prepend (operation->priv->user_widgets, entry);

  g_signal_connect (entry, "changed",
                    G_CALLBACK (pw_dialog_verify_input), user_data);

  g_signal_connect (entry, "activate",
                    G_CALLBACK (pw_dialog_cycle_focus), user_data);

  return entry;
}

static void
ctk_mount_operation_ask_password_do_ctk (CtkMountOperation *operation,
                                         const gchar       *message,
                                         const gchar       *default_user,
                                         const gchar       *default_domain)
{
  CtkMountOperationPrivate *priv;
  CtkWidget *widget;
  CtkDialog *dialog;
  CtkWindow *window;
  CtkWidget *hbox, *main_vbox, *icon;
  CtkWidget *grid;
  CtkWidget *label;
  CtkWidget *content_area, *action_area;
  gboolean   can_anonymous;
  guint      rows;
  gchar *primary;
  const gchar *secondary = NULL;
  PangoAttrList *attrs;
  gboolean use_header;

  priv = operation->priv;

  g_object_get (ctk_settings_get_default (),
                "ctk-dialogs-use-header", &use_header,
                NULL);
  widget = g_object_new (CTK_TYPE_DIALOG,
                         "use-header-bar", use_header,
                         NULL);
  dialog = CTK_DIALOG (widget);
  window = CTK_WINDOW (widget);

  priv->dialog = dialog;

  content_area = ctk_dialog_get_content_area (dialog);

  action_area = ctk_dialog_get_action_area (dialog);

  /* Set the dialog up with HIG properties */
  ctk_container_set_border_width (CTK_CONTAINER (dialog), 5);
  ctk_box_set_spacing (CTK_BOX (content_area), 2); /* 2 * 5 + 2 = 12 */
  ctk_container_set_border_width (CTK_CONTAINER (action_area), 5);
  ctk_box_set_spacing (CTK_BOX (action_area), 6);

  ctk_window_set_resizable (window, FALSE);
  ctk_window_set_title (window, "");
  ctk_window_set_icon_name (window, "dialog-password");

  ctk_dialog_add_buttons (dialog,
                          _("_Cancel"), CTK_RESPONSE_CANCEL,
                          _("Co_nnect"), CTK_RESPONSE_OK,
                          NULL);
  ctk_dialog_set_default_response (dialog, CTK_RESPONSE_OK);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  ctk_dialog_set_alternative_button_order (dialog,
                                           CTK_RESPONSE_OK,
                                           CTK_RESPONSE_CANCEL,
                                           -1);
G_GNUC_END_IGNORE_DEPRECATIONS

  /* Build contents */
  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 12);
  ctk_container_set_border_width (CTK_CONTAINER (hbox), 5);
  ctk_box_pack_start (CTK_BOX (content_area), hbox, TRUE, TRUE, 0);

  icon = ctk_image_new_from_icon_name ("dialog-password",
                                       CTK_ICON_SIZE_DIALOG);

  ctk_widget_set_halign (icon, CTK_ALIGN_CENTER);
  ctk_widget_set_valign (icon, CTK_ALIGN_START);
  ctk_box_pack_start (CTK_BOX (hbox), icon, FALSE, FALSE, 0);

  main_vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 18);
  ctk_box_pack_start (CTK_BOX (hbox), main_vbox, TRUE, TRUE, 0);

  primary = strstr (message, "\n");
  if (primary)
    {
      secondary = primary + 1;
      primary = g_strndup (message, primary - message);
    }

  label = ctk_label_new (primary != NULL ? primary : message);
  ctk_widget_set_halign (label, CTK_ALIGN_START);
  ctk_widget_set_valign (label, CTK_ALIGN_CENTER);
  ctk_label_set_line_wrap (CTK_LABEL (label), TRUE);
  ctk_box_pack_start (CTK_BOX (main_vbox), CTK_WIDGET (label),
                      FALSE, TRUE, 0);
  g_free (primary);
  attrs = pango_attr_list_new ();
  pango_attr_list_insert (attrs, pango_attr_weight_new (PANGO_WEIGHT_BOLD));
  ctk_label_set_attributes (CTK_LABEL (label), attrs);
  pango_attr_list_unref (attrs);

  if (secondary != NULL)
    {
      label = ctk_label_new (secondary);
      ctk_widget_set_halign (label, CTK_ALIGN_START);
      ctk_widget_set_valign (label, CTK_ALIGN_CENTER);
      ctk_label_set_line_wrap (CTK_LABEL (label), TRUE);
      ctk_box_pack_start (CTK_BOX (main_vbox), CTK_WIDGET (label),
                          FALSE, FALSE, 0);
    }

  grid = ctk_grid_new ();
  operation->priv->grid = grid;
  ctk_grid_set_row_spacing (CTK_GRID (grid), 12);
  ctk_grid_set_column_spacing (CTK_GRID (grid), 12);
  ctk_widget_set_margin_bottom (grid, 12);
  ctk_box_pack_start (CTK_BOX (main_vbox), grid, FALSE, FALSE, 0);

  can_anonymous = priv->ask_flags & G_ASK_PASSWORD_ANONYMOUS_SUPPORTED;

  rows = 0;

  priv->anonymous_toggle = NULL;
  if (can_anonymous)
    {
      CtkWidget *anon_box;
      CtkWidget *choice;
      GSList    *group;

      label = ctk_label_new (_("Connect As"));
      ctk_widget_set_halign (label, CTK_ALIGN_END);
      ctk_widget_set_valign (label, CTK_ALIGN_START);
      ctk_widget_set_hexpand (label, FALSE);
      ctk_grid_attach (CTK_GRID (grid), label, 0, rows, 1, 1);

      anon_box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_grid_attach (CTK_GRID (grid), anon_box, 1, rows++, 1, 1);

      choice = ctk_radio_button_new_with_mnemonic (NULL, _("_Anonymous"));
      ctk_box_pack_start (CTK_BOX (anon_box),
                          choice,
                          FALSE, FALSE, 0);
      g_signal_connect (choice, "toggled",
                        G_CALLBACK (pw_dialog_anonymous_toggled), operation);
      priv->anonymous_toggle = choice;

      group = ctk_radio_button_get_group (CTK_RADIO_BUTTON (choice));
      choice = ctk_radio_button_new_with_mnemonic (group, _("Registered U_ser"));
      ctk_box_pack_start (CTK_BOX (anon_box),
                          choice,
                          FALSE, FALSE, 0);
      g_signal_connect (choice, "toggled",
                        G_CALLBACK (pw_dialog_anonymous_toggled), operation);
    }

  priv->username_entry = NULL;

  if (priv->ask_flags & G_ASK_PASSWORD_NEED_USERNAME)
    priv->username_entry = table_add_entry (operation, rows++, _("_Username"),
                                            default_user, operation);

  priv->domain_entry = NULL;
  if (priv->ask_flags & G_ASK_PASSWORD_NEED_DOMAIN)
    priv->domain_entry = table_add_entry (operation, rows++, _("_Domain"),
                                          default_domain, operation);

  priv->pim_entry = NULL;
  if (priv->ask_flags & G_ASK_PASSWORD_TCRYPT)
    {
      CtkWidget *volume_type_label;
      CtkWidget *volume_type_box;

      volume_type_label = ctk_label_new (_("Volume type"));
      ctk_widget_set_halign (volume_type_label, CTK_ALIGN_END);
      ctk_widget_set_hexpand (volume_type_label, FALSE);
      ctk_grid_attach (CTK_GRID (grid), volume_type_label, 0, rows, 1, 1);
      priv->user_widgets = g_list_prepend (priv->user_widgets, volume_type_label);

      volume_type_box =  ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);
      ctk_grid_attach (CTK_GRID (grid), volume_type_box, 1, rows++, 1, 1);
      priv->user_widgets = g_list_prepend (priv->user_widgets, volume_type_box);

      priv->tcrypt_hidden_toggle = ctk_check_button_new_with_mnemonic (_("_Hidden"));
      ctk_container_add (CTK_CONTAINER (volume_type_box), priv->tcrypt_hidden_toggle);

      priv->tcrypt_system_toggle = ctk_check_button_new_with_mnemonic (_("_Windows system"));
      ctk_container_add (CTK_CONTAINER (volume_type_box), priv->tcrypt_system_toggle);

      priv->pim_entry = table_add_entry (operation, rows++, _("_PIM"), NULL, operation);
    }

  priv->password_entry = NULL;
  if (priv->ask_flags & G_ASK_PASSWORD_NEED_PASSWORD)
    {
      priv->password_entry = table_add_entry (operation, rows++, _("_Password"),
                                              NULL, operation);
      ctk_entry_set_visibility (CTK_ENTRY (priv->password_entry), FALSE);
    }

   if (priv->ask_flags & G_ASK_PASSWORD_SAVING_SUPPORTED)
    {
      CtkWidget    *remember_box;
      CtkWidget    *choice;
      GSList       *group;
      GPasswordSave password_save;

      remember_box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_grid_attach (CTK_GRID (grid), remember_box, 0, rows++, 2, 1);
      priv->user_widgets = g_list_prepend (priv->user_widgets, remember_box);

      label = ctk_label_new ("");
      ctk_container_add (CTK_CONTAINER (remember_box), label);

      password_save = g_mount_operation_get_password_save (G_MOUNT_OPERATION (operation));
      priv->password_save = password_save;

      choice = ctk_radio_button_new_with_mnemonic (NULL, _("Forget password _immediately"));
      ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (choice),
                                    password_save == G_PASSWORD_SAVE_NEVER);
      g_object_set_data (G_OBJECT (choice), "password-save",
                         GINT_TO_POINTER (G_PASSWORD_SAVE_NEVER));
      g_signal_connect (choice, "toggled",
                        G_CALLBACK (remember_button_toggled), operation);
      ctk_box_pack_start (CTK_BOX (remember_box), choice, FALSE, FALSE, 0);

      group = ctk_radio_button_get_group (CTK_RADIO_BUTTON (choice));
      choice = ctk_radio_button_new_with_mnemonic (group, _("Remember password until you _logout"));
      ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (choice),
                                    password_save == G_PASSWORD_SAVE_FOR_SESSION);
      g_object_set_data (G_OBJECT (choice), "password-save",
                         GINT_TO_POINTER (G_PASSWORD_SAVE_FOR_SESSION));
      g_signal_connect (choice, "toggled",
                        G_CALLBACK (remember_button_toggled), operation);
      ctk_box_pack_start (CTK_BOX (remember_box), choice, FALSE, FALSE, 0);

      group = ctk_radio_button_get_group (CTK_RADIO_BUTTON (choice));
      choice = ctk_radio_button_new_with_mnemonic (group, _("Remember _forever"));
      ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (choice),
                                    password_save == G_PASSWORD_SAVE_PERMANENTLY);
      g_object_set_data (G_OBJECT (choice), "password-save",
                         GINT_TO_POINTER (G_PASSWORD_SAVE_PERMANENTLY));
      g_signal_connect (choice, "toggled",
                        G_CALLBACK (remember_button_toggled), operation);
      ctk_box_pack_start (CTK_BOX (remember_box), choice, FALSE, FALSE, 0);
    }

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (pw_dialog_got_response), operation);

  if (can_anonymous)
    {
      /* The anonymous option will be active by default,
       * ensure the toggled signal is emitted for it.
       */
      ctk_toggle_button_toggled (CTK_TOGGLE_BUTTON (priv->anonymous_toggle));
    }
  else if (! pw_dialog_input_is_valid (operation))
    ctk_dialog_set_response_sensitive (dialog, CTK_RESPONSE_OK, FALSE);

  g_object_notify (G_OBJECT (operation), "is-showing");

  if (priv->parent_window)
    {
      ctk_window_set_transient_for (window, priv->parent_window);
      ctk_window_set_modal (window, TRUE);
    }
  else if (priv->screen)
    ctk_window_set_screen (CTK_WINDOW (dialog), priv->screen);

  ctk_widget_show_all (CTK_WIDGET (dialog));

  g_object_ref (operation);
}

static void
call_password_proxy_cb (GObject      *source,
                        GAsyncResult *res,
                        gpointer      user_data)
{
  _CtkMountOperationHandler *proxy = _CTK_MOUNT_OPERATION_HANDLER (source);
  GMountOperation *op = user_data;
  GMountOperationResult result;
  GVariant *result_details;
  GVariantIter iter;
  const gchar *key;
  GVariant *value;
  GError *error = NULL;

  if (!_ctk_mount_operation_handler_call_ask_password_finish (proxy,
                                                              &result,
                                                              &result_details,
                                                              res,
                                                              &error))
    {
      result = G_MOUNT_OPERATION_ABORTED;
      g_warning ("Shell mount operation error: %s", error->message);
      g_error_free (error);
      goto out;
    }

  g_variant_iter_init (&iter, result_details);
  while (g_variant_iter_loop (&iter, "{&sv}", &key, &value))
    {
      if (strcmp (key, "password") == 0)
        g_mount_operation_set_password (op, g_variant_get_string (value, NULL));
      else if (strcmp (key, "password_save") == 0)
        g_mount_operation_set_password_save (op, g_variant_get_uint32 (value));
      else if (strcmp (key, "hidden_volume") == 0)
        g_mount_operation_set_is_tcrypt_hidden_volume (op, g_variant_get_boolean (value));
      else if (strcmp (key, "system_volume") == 0)
        g_mount_operation_set_is_tcrypt_system_volume (op, g_variant_get_boolean (value));
      else if (strcmp (key, "pim") == 0)
        g_mount_operation_set_pim (op, g_variant_get_uint32 (value));
    }

 out:
  ctk_mount_operation_proxy_finish (CTK_MOUNT_OPERATION (op), result);
}

static void
ctk_mount_operation_ask_password_do_proxy (CtkMountOperation *operation,
                                           const char        *message,
                                           const char        *default_user,
                                           const char        *default_domain)
{
  gchar id[255];
  g_sprintf(id, "CtkMountOperation%p", operation);

  operation->priv->handler_showing = TRUE;
  g_object_notify (G_OBJECT (operation), "is-showing");

  /* keep a ref to the operation while the handler is showing */
  g_object_ref (operation);

  _ctk_mount_operation_handler_call_ask_password (operation->priv->handler, id,
                                                  message, "drive-harddisk",
                                                  default_user, default_domain,
                                                  operation->priv->ask_flags, NULL,
                                                  call_password_proxy_cb, operation);
}

static void
ctk_mount_operation_ask_password (GMountOperation   *mount_op,
                                  const char        *message,
                                  const char        *default_user,
                                  const char        *default_domain,
                                  GAskPasswordFlags  flags)
{
  CtkMountOperation *operation;
  CtkMountOperationPrivate *priv;
  gboolean use_ctk;

  operation = CTK_MOUNT_OPERATION (mount_op);
  priv = operation->priv;
  priv->ask_flags = flags;

  use_ctk = (operation->priv->handler == NULL) ||
    (priv->ask_flags & G_ASK_PASSWORD_NEED_DOMAIN) ||
    (priv->ask_flags & G_ASK_PASSWORD_NEED_USERNAME);

  if (use_ctk)
    ctk_mount_operation_ask_password_do_ctk (operation, message, default_user, default_domain);
  else
    ctk_mount_operation_ask_password_do_proxy (operation, message, default_user, default_domain);
}

static void
question_dialog_button_clicked (CtkDialog       *dialog,
                                gint             button_number,
                                GMountOperation *op)
{
  CtkMountOperationPrivate *priv;
  CtkMountOperation *operation;

  operation = CTK_MOUNT_OPERATION (op);
  priv = operation->priv;

  if (button_number >= 0)
    {
      g_mount_operation_set_choice (op, button_number);
      g_mount_operation_reply (op, G_MOUNT_OPERATION_HANDLED);
    }
  else
    g_mount_operation_reply (op, G_MOUNT_OPERATION_ABORTED);

  priv->dialog = NULL;
  g_object_notify (G_OBJECT (operation), "is-showing");
  ctk_widget_destroy (CTK_WIDGET (dialog));
  g_object_unref (op);
}

static void
ctk_mount_operation_ask_question_do_ctk (CtkMountOperation *op,
                                         const char        *message,
                                         const char        *choices[])
{
  CtkMountOperationPrivate *priv;
  CtkWidget  *dialog;
  const char *secondary = NULL;
  char       *primary;
  int        count, len = 0;

  g_return_if_fail (CTK_IS_MOUNT_OPERATION (op));
  g_return_if_fail (message != NULL);
  g_return_if_fail (choices != NULL);

  priv = op->priv;

  primary = strstr (message, "\n");
  if (primary)
    {
      secondary = primary + 1;
      primary = g_strndup (message, primary - message);
    }

  dialog = ctk_message_dialog_new (priv->parent_window, 0,
                                   CTK_MESSAGE_QUESTION,
                                   CTK_BUTTONS_NONE, "%s",
                                   primary != NULL ? primary : message);
  g_free (primary);

  if (secondary)
    ctk_message_dialog_format_secondary_text (CTK_MESSAGE_DIALOG (dialog),
                                              "%s", secondary);

  /* First count the items in the list then
   * add the buttons in reverse order */

  while (choices[len] != NULL)
    len++;

  for (count = len - 1; count >= 0; count--)
    ctk_dialog_add_button (CTK_DIALOG (dialog), choices[count], count);

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (question_dialog_button_clicked), op);

  priv->dialog = CTK_DIALOG (dialog);
  g_object_notify (G_OBJECT (op), "is-showing");

  if (priv->parent_window == NULL && priv->screen)
    ctk_window_set_screen (CTK_WINDOW (dialog), priv->screen);

  ctk_widget_show (dialog);
  g_object_ref (op);
}

static void
call_question_proxy_cb (GObject      *source,
                        GAsyncResult *res,
                        gpointer      user_data)
{
  _CtkMountOperationHandler *proxy = _CTK_MOUNT_OPERATION_HANDLER (source);
  GMountOperation *op = user_data;
  GMountOperationResult result;
  GVariant *result_details;
  GVariantIter iter;
  const gchar *key;
  GVariant *value;
  GError *error = NULL;

  if (!_ctk_mount_operation_handler_call_ask_question_finish (proxy,
                                                              &result,
                                                              &result_details,
                                                              res,
                                                              &error))
    {
      result = G_MOUNT_OPERATION_ABORTED;
      g_warning ("Shell mount operation error: %s", error->message);
      g_error_free (error);
      goto out;
    }

  g_variant_iter_init (&iter, result_details);
  while (g_variant_iter_loop (&iter, "{&sv}", &key, &value))
    {
      if (strcmp (key, "choice") == 0)
        g_mount_operation_set_choice (op, g_variant_get_int32 (value));
    }
 
 out:
  ctk_mount_operation_proxy_finish (CTK_MOUNT_OPERATION (op), result);
}

static void
ctk_mount_operation_ask_question_do_proxy (CtkMountOperation *operation,
                                           const char        *message,
                                           const char        *choices[])
{
  gchar id[255];
  g_sprintf(id, "CtkMountOperation%p", operation);

  operation->priv->handler_showing = TRUE;
  g_object_notify (G_OBJECT (operation), "is-showing");

  /* keep a ref to the operation while the handler is showing */
  g_object_ref (operation);

  _ctk_mount_operation_handler_call_ask_question (operation->priv->handler, id,
                                                  message, "drive-harddisk",
                                                  choices, NULL,
                                                  call_question_proxy_cb, operation);
}

static void
ctk_mount_operation_ask_question (GMountOperation *op,
                                  const char      *message,
                                  const char      *choices[])
{
  CtkMountOperation *operation;
  gboolean use_ctk;

  operation = CTK_MOUNT_OPERATION (op);
  use_ctk = (operation->priv->handler == NULL);

  if (use_ctk)
    ctk_mount_operation_ask_question_do_ctk (operation, message, choices);
  else
    ctk_mount_operation_ask_question_do_proxy (operation, message, choices);
}

static void
show_processes_button_clicked (CtkDialog       *dialog,
                               gint             button_number,
                               GMountOperation *op)
{
  CtkMountOperationPrivate *priv;
  CtkMountOperation *operation;

  operation = CTK_MOUNT_OPERATION (op);
  priv = operation->priv;

  if (button_number >= 0)
    {
      g_mount_operation_set_choice (op, button_number);
      g_mount_operation_reply (op, G_MOUNT_OPERATION_HANDLED);
    }
  else
    g_mount_operation_reply (op, G_MOUNT_OPERATION_ABORTED);

  priv->dialog = NULL;
  g_object_notify (G_OBJECT (operation), "is-showing");
  ctk_widget_destroy (CTK_WIDGET (dialog));
  g_object_unref (op);
}

static gint
pid_equal (gconstpointer a,
           gconstpointer b)
{
  GPid pa, pb;

  pa = *((GPid *) a);
  pb = *((GPid *) b);

  return GPOINTER_TO_INT(pb) - GPOINTER_TO_INT(pa);
}

static void
diff_sorted_arrays (GArray         *array1,
                    GArray         *array2,
                    GCompareFunc   compare,
                    GArray         *added_indices,
                    GArray         *removed_indices)
{
  gint order;
  guint n1, n2;
  guint elem_size;

  n1 = n2 = 0;

  elem_size = g_array_get_element_size (array1);
  g_assert (elem_size == g_array_get_element_size (array2));

  while (n1 < array1->len && n2 < array2->len)
    {
      order = (*compare) (((const char*) array1->data) + n1 * elem_size,
                          ((const char*) array2->data) + n2 * elem_size);
      if (order < 0)
        {
          g_array_append_val (removed_indices, n1);
          n1++;
        }
      else if (order > 0)
        {
          g_array_append_val (added_indices, n2);
          n2++;
        }
      else
        { /* same item */
          n1++;
          n2++;
        }
    }

  while (n1 < array1->len)
    {
      g_array_append_val (removed_indices, n1);
      n1++;
    }
  while (n2 < array2->len)
    {
      g_array_append_val (added_indices, n2);
      n2++;
    }
}


static void
add_pid_to_process_list_store (CtkMountOperation              *mount_operation,
                               CtkMountOperationLookupContext *lookup_context,
                               CtkListStore                   *list_store,
                               GPid                            pid)
{
  gchar *command_line;
  gchar *name;
  GdkPixbuf *pixbuf;
  gchar *markup;
  CtkTreeIter iter;

  name = NULL;
  pixbuf = NULL;
  command_line = NULL;
  _ctk_mount_operation_lookup_info (lookup_context,
                                    pid,
                                    24,
                                    &name,
                                    &command_line,
                                    &pixbuf);

  if (name == NULL)
    name = g_strdup_printf (_("Unknown Application (PID %d)"), (int) (gssize) pid);

  if (command_line == NULL)
    command_line = g_strdup ("");

  if (pixbuf == NULL)
    {
      CtkIconTheme *theme;
      theme = ctk_css_icon_theme_value_get_icon_theme
        (_ctk_style_context_peek_property (ctk_widget_get_style_context (CTK_WIDGET (mount_operation->priv->dialog)),
                                           CTK_CSS_PROPERTY_ICON_THEME));
      pixbuf = ctk_icon_theme_load_icon (theme,
                                         "application-x-executable",
                                         24,
                                         0,
                                         NULL);
    }

  markup = g_strdup_printf ("<b>%s</b>\n"
                            "<small>%s</small>",
                            name,
                            command_line);

  ctk_list_store_append (list_store, &iter);
  ctk_list_store_set (list_store, &iter,
                      0, pixbuf,
                      1, markup,
                      2, pid,
                      -1);

  if (pixbuf != NULL)
    g_object_unref (pixbuf);
  g_free (markup);
  g_free (name);
  g_free (command_line);
}

static void
remove_pid_from_process_list_store (CtkMountOperation *mount_operation G_GNUC_UNUSED,
                                    CtkListStore      *list_store,
                                    GPid               pid)
{
  CtkTreeIter iter;
  GPid pid_of_item;

  if (ctk_tree_model_get_iter_first (CTK_TREE_MODEL (list_store), &iter))
    {
      do
        {
          ctk_tree_model_get (CTK_TREE_MODEL (list_store),
                              &iter,
                              2, &pid_of_item,
                              -1);

          if (pid_of_item == pid)
            {
              ctk_list_store_remove (list_store, &iter);
              break;
            }
        }
      while (ctk_tree_model_iter_next (CTK_TREE_MODEL (list_store), &iter));
    }
}


static void
update_process_list_store (CtkMountOperation *mount_operation,
                           CtkListStore      *list_store,
                           GArray            *processes)
{
  guint n;
  CtkMountOperationLookupContext *lookup_context;
  GArray *current_pids;
  GArray *pid_indices_to_add;
  GArray *pid_indices_to_remove;
  CtkTreeIter iter;
  GPid pid;

  /* Just removing all items and adding new ones will screw up the
   * focus handling in the treeview - so compute the delta, and add/remove
   * items as appropriate
   */
  current_pids = g_array_new (FALSE, FALSE, sizeof (GPid));
  pid_indices_to_add = g_array_new (FALSE, FALSE, sizeof (gint));
  pid_indices_to_remove = g_array_new (FALSE, FALSE, sizeof (gint));

  if (ctk_tree_model_get_iter_first (CTK_TREE_MODEL (list_store), &iter))
    {
      do
        {
          ctk_tree_model_get (CTK_TREE_MODEL (list_store),
                              &iter,
                              2, &pid,
                              -1);

          g_array_append_val (current_pids, pid);
        }
      while (ctk_tree_model_iter_next (CTK_TREE_MODEL (list_store), &iter));
    }

  g_array_sort (current_pids, pid_equal);
  g_array_sort (processes, pid_equal);

  diff_sorted_arrays (current_pids, processes, pid_equal, pid_indices_to_add, pid_indices_to_remove);

  for (n = 0; n < pid_indices_to_remove->len; n++)
    {
      pid = g_array_index (current_pids, GPid, n);
      remove_pid_from_process_list_store (mount_operation, list_store, pid);
    }

  if (pid_indices_to_add->len > 0)
    {
      lookup_context = _ctk_mount_operation_lookup_context_get (ctk_widget_get_display (mount_operation->priv->process_tree_view));
      for (n = 0; n < pid_indices_to_add->len; n++)
        {
          pid = g_array_index (processes, GPid, n);
          add_pid_to_process_list_store (mount_operation, lookup_context, list_store, pid);
        }
      _ctk_mount_operation_lookup_context_free (lookup_context);
    }

  /* select the first item, if we went from a zero to a non-zero amount of processes */
  if (current_pids->len == 0 && pid_indices_to_add->len > 0)
    {
      if (ctk_tree_model_get_iter_first (CTK_TREE_MODEL (list_store), &iter))
        {
          CtkTreeSelection *tree_selection;
          tree_selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (mount_operation->priv->process_tree_view));
          ctk_tree_selection_select_iter (tree_selection, &iter);
        }
    }

  g_array_unref (current_pids);
  g_array_unref (pid_indices_to_add);
  g_array_unref (pid_indices_to_remove);
}

static void
on_end_process_activated (CtkMenuItem *item G_GNUC_UNUSED,
                          gpointer     user_data)
{
  CtkMountOperation *op = CTK_MOUNT_OPERATION (user_data);
  CtkTreeSelection *selection;
  CtkTreeIter iter;
  GPid pid_to_kill;
  GError *error;

  selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (op->priv->process_tree_view));

  if (!ctk_tree_selection_get_selected (selection,
                                        NULL,
                                        &iter))
    goto out;

  ctk_tree_model_get (CTK_TREE_MODEL (op->priv->process_list_store),
                      &iter,
                      2, &pid_to_kill,
                      -1);

  /* TODO: We might want to either
   *
   *       - Be smart about things and send SIGKILL rather than SIGTERM if
   *         this is the second time the user requests killing a process
   *
   *       - Or, easier (but worse user experience), offer both "End Process"
   *         and "Terminate Process" options
   *
   *      But that's not how things work right now....
   */
  error = NULL;
  if (!_ctk_mount_operation_kill_process (pid_to_kill, &error))
    {
      CtkWidget *dialog;
      gint response;

      /* Use CTK_DIALOG_DESTROY_WITH_PARENT here since the parent dialog can be
       * indeed be destroyed via the GMountOperation::abort signal... for example,
       * this is triggered if the user yanks the device while we are showing
       * the dialog...
       */
      dialog = ctk_message_dialog_new (CTK_WINDOW (op->priv->dialog),
                                       CTK_DIALOG_MODAL | CTK_DIALOG_DESTROY_WITH_PARENT,
                                       CTK_MESSAGE_ERROR,
                                       CTK_BUTTONS_CLOSE,
                                       _("Unable to end process"));
      ctk_message_dialog_format_secondary_text (CTK_MESSAGE_DIALOG (dialog),
                                                "%s",
                                                error->message);

      ctk_widget_show_all (dialog);
      response = ctk_dialog_run (CTK_DIALOG (dialog));

      /* CTK_RESPONSE_NONE means the dialog were programmatically destroy, e.g. that
       * CTK_DIALOG_DESTROY_WITH_PARENT kicked in - so it would trigger a warning to
       * destroy the dialog in that case
       */
      if (response != CTK_RESPONSE_NONE)
        ctk_widget_destroy (dialog);

      g_error_free (error);
    }

 out:
  ;
}

static gboolean
do_popup_menu_for_process_tree_view (CtkWidget         *widget G_GNUC_UNUSED,
                                     const CdkEvent    *event,
                                     CtkMountOperation *op)
{
  CtkWidget *menu;
  CtkWidget *item;

  menu = ctk_menu_new ();
  ctk_style_context_add_class (ctk_widget_get_style_context (menu),
                               CTK_STYLE_CLASS_CONTEXT_MENU);

  item = ctk_menu_item_new_with_mnemonic (_("_End Process"));
  g_signal_connect (item, "activate",
                    G_CALLBACK (on_end_process_activated),
                    op);
  ctk_menu_shell_append (CTK_MENU_SHELL (menu), item);
  ctk_widget_show_all (menu);

  if (event && cdk_event_triggers_context_menu (event))
    {
      CtkTreePath *path;
      CtkTreeSelection *selection;

      if (ctk_tree_view_get_path_at_pos (CTK_TREE_VIEW (op->priv->process_tree_view),
                                         (gint) event->button.x,
                                         (gint) event->button.y,
                                         &path,
                                         NULL,
                                         NULL,
                                         NULL))
        {
          selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (op->priv->process_tree_view));
          ctk_tree_selection_select_path (selection, path);
          ctk_tree_path_free (path);
        }
      else
        {
          /* don't popup a menu if the user right-clicked in an area with no rows */
          return FALSE;
        }
    }

  ctk_menu_popup_at_pointer (CTK_MENU (menu), event);
  return TRUE;
}

static gboolean
on_popup_menu_for_process_tree_view (CtkWidget *widget,
                                     gpointer   user_data)
{
  CtkMountOperation *op = CTK_MOUNT_OPERATION (user_data);
  return do_popup_menu_for_process_tree_view (widget, NULL, op);
}

static gboolean
on_button_press_event_for_process_tree_view (CtkWidget      *widget,
                                             CdkEventButton *event,
                                             gpointer        user_data)
{
  CtkMountOperation *op = CTK_MOUNT_OPERATION (user_data);
  gboolean ret;

  ret = FALSE;

  if (cdk_event_triggers_context_menu ((CdkEvent *) event))
    {
      ret = do_popup_menu_for_process_tree_view (widget, (CdkEvent *) event, op);
    }

  return ret;
}

static CtkWidget *
create_show_processes_dialog (CtkMountOperation *op,
                              const char      *message,
                              const char      *choices[])
{
  CtkMountOperationPrivate *priv;
  CtkWidget  *dialog;
  const char *secondary = NULL;
  char       *primary;
  int        count, len = 0;
  CtkWidget *label;
  CtkWidget *tree_view;
  CtkWidget *scrolled_window;
  CtkWidget *vbox;
  CtkWidget *content_area;
  CtkTreeViewColumn *column;
  CtkCellRenderer *renderer;
  CtkListStore *list_store;
  gchar *s;
  gboolean use_header;

  priv = op->priv;

  primary = strstr (message, "\n");
  if (primary)
    {
      secondary = primary + 1;
      primary = g_strndup (message, primary - message);
    }

  g_object_get (ctk_settings_get_default (),
                "ctk-dialogs-use-header", &use_header,
                NULL);
  dialog = g_object_new (CTK_TYPE_DIALOG,
                         "use-header-bar", use_header,
                         NULL);

  if (priv->parent_window != NULL)
    ctk_window_set_transient_for (CTK_WINDOW (dialog), priv->parent_window);
  ctk_window_set_title (CTK_WINDOW (dialog), "");

  content_area = ctk_dialog_get_content_area (CTK_DIALOG (dialog));
  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 12);
  ctk_container_set_border_width (CTK_CONTAINER (vbox), 12);
  ctk_box_pack_start (CTK_BOX (content_area), vbox, TRUE, TRUE, 0);

  if (secondary != NULL)
    s = g_strdup_printf ("<big><b>%s</b></big>\n\n%s", primary, secondary);
  else
    s = g_strdup_printf ("%s", primary);

  g_free (primary);
  label = ctk_label_new (NULL);
  ctk_label_set_markup (CTK_LABEL (label), s);
  g_free (s);
  ctk_box_pack_start (CTK_BOX (vbox), label, TRUE, TRUE, 0);

  /* First count the items in the list then
   * add the buttons in reverse order */

  while (choices[len] != NULL)
    len++;

  for (count = len - 1; count >= 0; count--)
    ctk_dialog_add_button (CTK_DIALOG (dialog), choices[count], count);

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (show_processes_button_clicked), op);

  priv->dialog = CTK_DIALOG (dialog);
  g_object_notify (G_OBJECT (op), "is-showing");

  if (priv->parent_window == NULL && priv->screen)
    ctk_window_set_screen (CTK_WINDOW (dialog), priv->screen);

  tree_view = ctk_tree_view_new ();
  /* TODO: should use EM's when ctk+ RI patches land */
  ctk_widget_set_size_request (tree_view,
                               300,
                               120);

  column = ctk_tree_view_column_new ();
  renderer = ctk_cell_renderer_pixbuf_new ();
  ctk_tree_view_column_pack_start (column, renderer, FALSE);
  ctk_tree_view_column_set_attributes (column, renderer,
                                       "pixbuf", 0,
                                       NULL);
  renderer = ctk_cell_renderer_text_new ();
  g_object_set (renderer,
                "ellipsize", PANGO_ELLIPSIZE_MIDDLE,
                "ellipsize-set", TRUE,
                NULL);
  ctk_tree_view_column_pack_start (column, renderer, TRUE);
  ctk_tree_view_column_set_attributes (column, renderer,
                                       "markup", 1,
                                       NULL);
  ctk_tree_view_append_column (CTK_TREE_VIEW (tree_view), column);
  ctk_tree_view_set_headers_visible (CTK_TREE_VIEW (tree_view), FALSE);


  scrolled_window = ctk_scrolled_window_new (NULL, NULL);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolled_window),
                                  CTK_POLICY_NEVER,
                                  CTK_POLICY_AUTOMATIC);
  ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (scrolled_window), CTK_SHADOW_IN);

  ctk_container_add (CTK_CONTAINER (scrolled_window), tree_view);
  ctk_box_pack_start (CTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);

  g_signal_connect (tree_view, "popup-menu",
                    G_CALLBACK (on_popup_menu_for_process_tree_view),
                    op);
  g_signal_connect (tree_view, "button-press-event",
                    G_CALLBACK (on_button_press_event_for_process_tree_view),
                    op);

  list_store = ctk_list_store_new (3,
                                   GDK_TYPE_PIXBUF,
                                   G_TYPE_STRING,
                                   G_TYPE_INT);

  ctk_tree_view_set_model (CTK_TREE_VIEW (tree_view), CTK_TREE_MODEL (list_store));

  priv->process_list_store = list_store;
  priv->process_tree_view = tree_view;
  /* set pointers to NULL when dialog goes away */
  g_object_add_weak_pointer (G_OBJECT (priv->process_list_store), (gpointer *) &priv->process_list_store);
  g_object_add_weak_pointer (G_OBJECT (priv->process_tree_view), (gpointer *) &priv->process_tree_view);

  g_object_unref (list_store);
  g_object_ref (op);

  return dialog;
}

static void
call_processes_proxy_cb (GObject     *source,
                        GAsyncResult *res,
                        gpointer      user_data)
{
  _CtkMountOperationHandler *proxy = _CTK_MOUNT_OPERATION_HANDLER (source);
  GMountOperation *op = user_data;
  GMountOperationResult result;
  GVariant *result_details;
  GVariantIter iter;
  const gchar *key;
  GVariant *value;
  GError *error = NULL;

  if (!_ctk_mount_operation_handler_call_show_processes_finish (proxy,
                                                                &result,
                                                                &result_details,
                                                                res,
                                                                &error))
    {
      result = G_MOUNT_OPERATION_ABORTED;
      g_warning ("Shell mount operation error: %s", error->message);
      g_error_free (error);
      goto out;
    }

  /* If the request was unhandled it means we called the method again;
   * in this case, just return and wait for the next response.
   */
  if (result == G_MOUNT_OPERATION_UNHANDLED)
    return;

  g_variant_iter_init (&iter, result_details);
  while (g_variant_iter_loop (&iter, "{&sv}", &key, &value))
    {
      if (strcmp (key, "choice") == 0)
        g_mount_operation_set_choice (op, g_variant_get_int32 (value));
    }

 out:
  ctk_mount_operation_proxy_finish (CTK_MOUNT_OPERATION (op), result);
}

static void
ctk_mount_operation_show_processes_do_proxy (CtkMountOperation *operation,
                                             const char        *message,
                                             GArray            *processes,
                                             const char        *choices[])
{
  gchar id[255];
  g_sprintf(id, "CtkMountOperation%p", operation);

  operation->priv->handler_showing = TRUE;
  g_object_notify (G_OBJECT (operation), "is-showing");

  /* keep a ref to the operation while the handler is showing */
  g_object_ref (operation);

  _ctk_mount_operation_handler_call_show_processes (operation->priv->handler, id,
                                                    message, "drive-harddisk",
                                                    g_variant_new_fixed_array (G_VARIANT_TYPE_INT32,
                                                                               processes->data, processes->len,
                                                                               sizeof (GPid)),
                                                    choices, NULL,
                                                    call_processes_proxy_cb, operation);
}

static void
ctk_mount_operation_show_processes_do_ctk (CtkMountOperation *op,
                                           const char        *message,
                                           GArray            *processes,
                                           const char        *choices[])
{
  CtkMountOperationPrivate *priv;
  CtkWidget *dialog = NULL;

  g_return_if_fail (CTK_IS_MOUNT_OPERATION (op));
  g_return_if_fail (message != NULL);
  g_return_if_fail (processes != NULL);
  g_return_if_fail (choices != NULL);

  priv = op->priv;

  if (priv->process_list_store == NULL)
    {
      /* need to create the dialog */
      dialog = create_show_processes_dialog (op, message, choices);
    }

  /* otherwise, we're showing the dialog, assume messages+choices hasn't changed */

  update_process_list_store (op,
                             priv->process_list_store,
                             processes);

  if (dialog != NULL)
    {
      ctk_widget_show_all (dialog);
    }
}


static void
ctk_mount_operation_show_processes (GMountOperation *op,
                                    const char      *message,
                                    GArray          *processes,
                                    const char      *choices[])
{

  CtkMountOperation *operation;
  gboolean use_ctk;

  operation = CTK_MOUNT_OPERATION (op);
  use_ctk = (operation->priv->handler == NULL);

  if (use_ctk)
    ctk_mount_operation_show_processes_do_ctk (operation, message, processes, choices);
  else
    ctk_mount_operation_show_processes_do_proxy (operation, message, processes, choices);
}

static void
ctk_mount_operation_aborted (GMountOperation *op)
{
  CtkMountOperationPrivate *priv;

  priv = CTK_MOUNT_OPERATION (op)->priv;

  if (priv->dialog != NULL)
    {
      ctk_widget_destroy (CTK_WIDGET (priv->dialog));
      priv->dialog = NULL;
      g_object_notify (G_OBJECT (op), "is-showing");
      g_object_unref (op);
    }

  if (priv->handler != NULL)
    {
      _ctk_mount_operation_handler_call_close (priv->handler, NULL, NULL, NULL);

      priv->handler_showing = FALSE;
      g_object_notify (G_OBJECT (op), "is-showing");
    }
}

/**
 * ctk_mount_operation_new:
 * @parent: (allow-none): transient parent of the window, or %NULL
 *
 * Creates a new #CtkMountOperation
 *
 * Returns: a new #CtkMountOperation
 *
 * Since: 2.14
 */
GMountOperation *
ctk_mount_operation_new (CtkWindow *parent)
{
  GMountOperation *mount_operation;

  mount_operation = g_object_new (CTK_TYPE_MOUNT_OPERATION,
                                  "parent", parent, NULL);

  return mount_operation;
}

/**
 * ctk_mount_operation_is_showing:
 * @op: a #CtkMountOperation
 *
 * Returns whether the #CtkMountOperation is currently displaying
 * a window.
 *
 * Returns: %TRUE if @op is currently displaying a window
 *
 * Since: 2.14
 */
gboolean
ctk_mount_operation_is_showing (CtkMountOperation *op)
{
  g_return_val_if_fail (CTK_IS_MOUNT_OPERATION (op), FALSE);

  return op->priv->dialog != NULL;
}

/**
 * ctk_mount_operation_set_parent:
 * @op: a #CtkMountOperation
 * @parent: (allow-none): transient parent of the window, or %NULL
 *
 * Sets the transient parent for windows shown by the
 * #CtkMountOperation.
 *
 * Since: 2.14
 */
void
ctk_mount_operation_set_parent (CtkMountOperation *op,
                                CtkWindow         *parent)
{
  CtkMountOperationPrivate *priv;

  g_return_if_fail (CTK_IS_MOUNT_OPERATION (op));
  g_return_if_fail (parent == NULL || CTK_IS_WINDOW (parent));

  priv = op->priv;

  if (priv->parent_window == parent)
    return;

  if (priv->parent_window)
    {
      g_signal_handlers_disconnect_by_func (priv->parent_window,
                                            ctk_widget_destroyed,
                                            &priv->parent_window);
      g_object_unref (priv->parent_window);
    }
  priv->parent_window = parent;
  if (priv->parent_window)
    {
      g_object_ref (priv->parent_window);
      g_signal_connect (priv->parent_window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed),
                        &priv->parent_window);
    }

  if (priv->dialog)
    ctk_window_set_transient_for (CTK_WINDOW (priv->dialog), priv->parent_window);

  g_object_notify (G_OBJECT (op), "parent");
}

/**
 * ctk_mount_operation_get_parent:
 * @op: a #CtkMountOperation
 *
 * Gets the transient parent used by the #CtkMountOperation
 *
 * Returns: (transfer none): the transient parent for windows shown by @op
 *
 * Since: 2.14
 */
CtkWindow *
ctk_mount_operation_get_parent (CtkMountOperation *op)
{
  g_return_val_if_fail (CTK_IS_MOUNT_OPERATION (op), NULL);

  return op->priv->parent_window;
}

/**
 * ctk_mount_operation_set_screen:
 * @op: a #CtkMountOperation
 * @screen: a #CdkScreen
 *
 * Sets the screen to show windows of the #CtkMountOperation on.
 *
 * Since: 2.14
 */
void
ctk_mount_operation_set_screen (CtkMountOperation *op,
                                CdkScreen         *screen)
{
  CtkMountOperationPrivate *priv;

  g_return_if_fail (CTK_IS_MOUNT_OPERATION (op));
  g_return_if_fail (CDK_IS_SCREEN (screen));

  priv = op->priv;

  if (priv->screen == screen)
    return;

  if (priv->screen)
    g_object_unref (priv->screen);

  priv->screen = g_object_ref (screen);

  if (priv->dialog)
    ctk_window_set_screen (CTK_WINDOW (priv->dialog), screen);

  g_object_notify (G_OBJECT (op), "screen");
}

/**
 * ctk_mount_operation_get_screen:
 * @op: a #CtkMountOperation
 *
 * Gets the screen on which windows of the #CtkMountOperation
 * will be shown.
 *
 * Returns: (transfer none): the screen on which windows of @op are shown
 *
 * Since: 2.14
 */
CdkScreen *
ctk_mount_operation_get_screen (CtkMountOperation *op)
{
  CtkMountOperationPrivate *priv;

  g_return_val_if_fail (CTK_IS_MOUNT_OPERATION (op), NULL);

  priv = op->priv;

  if (priv->dialog)
    return ctk_window_get_screen (CTK_WINDOW (priv->dialog));
  else if (priv->parent_window)
    return ctk_window_get_screen (CTK_WINDOW (priv->parent_window));
  else if (priv->screen)
    return priv->screen;
  else
    return cdk_screen_get_default ();
}
