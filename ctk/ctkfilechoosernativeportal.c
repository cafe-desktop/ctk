/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* CTK - The GIMP Toolkit
 * ctkfilechoosernativeportal.c: Portal File selector dialog
 * Copyright (C) 2015, Red Hat, Inc.
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

#include "ctkfilechoosernativeprivate.h"
#include "ctknativedialogprivate.h"

#include "ctkprivate.h"
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
#include "ctklabel.h"
#include "ctkmain.h"
#include "ctkinvisible.h"
#include "ctkfilechooserentry.h"
#include "ctkfilefilterprivate.h"
#include "ctkwindowprivate.h"

typedef struct {
  CtkFileChooserNative *self;

  CtkWidget *grab_widget;

  GDBusConnection *connection;
  char *portal_handle;
  guint portal_response_signal_id;
  gboolean modal;

  gboolean hidden;

  const char *method_name;

  CtkWindow *exported_window;
} FilechooserPortalData;


static void
filechooser_portal_data_free (FilechooserPortalData *data)
{
  if (data->portal_response_signal_id != 0)
    g_dbus_connection_signal_unsubscribe (data->connection,
                                          data->portal_response_signal_id);

  g_object_unref (data->connection);

  if (data->grab_widget)
    {
      ctk_grab_remove (data->grab_widget);
      ctk_widget_destroy (data->grab_widget);
    }

  g_clear_object (&data->self);

  if (data->exported_window)
    ctk_window_unexport_handle (data->exported_window);

  g_free (data->portal_handle);

  g_free (data);
}

static void
response_cb (GDBusConnection  *connection G_GNUC_UNUSED,
             const gchar      *sender_name G_GNUC_UNUSED,
             const gchar      *object_path G_GNUC_UNUSED,
             const gchar      *interface_name G_GNUC_UNUSED,
             const gchar      *signal_name G_GNUC_UNUSED,
             GVariant         *parameters,
             gpointer          user_data)
{
  CtkFileChooserNative *self = user_data;
  FilechooserPortalData *data = self->mode_data;
  guint32 portal_response;
  int ctk_response;
  const char **uris;
  int i;
  GVariant *response_data;
  GVariant *choices = NULL;
  GVariant *current_filter = NULL;

  g_variant_get (parameters, "(u@a{sv})", &portal_response, &response_data);
  g_variant_lookup (response_data, "uris", "^a&s", &uris);

  choices = g_variant_lookup_value (response_data, "choices", G_VARIANT_TYPE ("a(ss)"));
  if (choices)
    {
      for (i = 0; i < g_variant_n_children (choices); i++)
        {
          const char *id;
          const char *selected;
          g_variant_get_child (choices, i, "(&s&s)", &id, &selected);
          ctk_file_chooser_set_choice (CTK_FILE_CHOOSER (self), id, selected);
        }
      g_variant_unref (choices);
    }

  current_filter = g_variant_lookup_value (response_data, "current_filter", G_VARIANT_TYPE ("(sa(us))"));
  if (current_filter)
    {
      CtkFileFilter *filter = ctk_file_filter_new_from_gvariant (current_filter);
      const gchar *current_filter_name = ctk_file_filter_get_name (filter);

      /* Try to find  the given filter in the list of filters.
       * Since filters are compared by pointer value, using the passed
       * filter would otherwise not match in a comparison, even if
       * a filter in the list of filters has been selected.
       * We'll use the heuristic that if two filters have the same name,
       * they must be the same.
       * If there is no match, just set the filter as it was retrieved.
       */
      CtkFileFilter *filter_to_select = filter;
      GSList *filters = ctk_file_chooser_list_filters (CTK_FILE_CHOOSER (self));
      GSList *l;

      for (l = filters; l; l = l->next)
        {
          CtkFileFilter *f = l->data;
          if (g_strcmp0 (ctk_file_filter_get_name (f), current_filter_name) == 0)
            {
              filter_to_select = f;
              break;
            }
        }
      g_slist_free (filters);
      ctk_file_chooser_set_filter (CTK_FILE_CHOOSER (self), filter_to_select);
    }

  g_slist_free_full (self->custom_files, g_object_unref);
  self->custom_files = NULL;
  for (i = 0; uris[i]; i++)
    self->custom_files = g_slist_prepend (self->custom_files, g_file_new_for_uri (uris[i]));

  switch (portal_response)
    {
    case 0:
      ctk_response = CTK_RESPONSE_ACCEPT;
      break;
    case 1:
      ctk_response = CTK_RESPONSE_CANCEL;
      break;
    case 2:
    default:
      ctk_response = CTK_RESPONSE_DELETE_EVENT;
      break;
    }

  filechooser_portal_data_free (data);
  self->mode_data = NULL;

  _ctk_native_dialog_emit_response (CTK_NATIVE_DIALOG (self), ctk_response);
}

static void
send_close (FilechooserPortalData *data)
{
  GDBusMessage *message;
  GError *error = NULL;

  message = g_dbus_message_new_method_call ("org.freedesktop.portal.Desktop",
                                            "/org/freedesktop/portal/desktop",
                                            "org.freedesktop.portal.FileChooser",
                                            "Close");
  g_dbus_message_set_body (message,
                           g_variant_new ("(o)", data->portal_handle));

  if (!g_dbus_connection_send_message (data->connection,
                                       message,
                                       G_DBUS_SEND_MESSAGE_FLAGS_NONE,
                                       NULL, &error))
    {
      g_warning ("unable to send FileChooser Close message: %s", error->message);
      g_error_free (error);
    }

  g_object_unref (message);
}

static void
open_file_msg_cb (GObject      *source_object G_GNUC_UNUSED,
                  GAsyncResult *res,
                  gpointer      user_data)
{
  FilechooserPortalData *data = user_data;
  CtkFileChooserNative *self = data->self;
  GDBusMessage *reply;
  GError *error = NULL;
  char *handle = NULL;

  reply = g_dbus_connection_send_message_with_reply_finish (data->connection, res, &error);

  if (reply && g_dbus_message_to_gerror (reply, &error))
    g_clear_object (&reply);

  if (reply == NULL)
    {
      if (!data->hidden)
        _ctk_native_dialog_emit_response (CTK_NATIVE_DIALOG (self), CTK_RESPONSE_DELETE_EVENT);
      g_warning ("Can't open portal file chooser: %s", error->message);
      g_error_free (error);
      filechooser_portal_data_free (data);
      self->mode_data = NULL;
      return;
    }

  g_variant_get_child (g_dbus_message_get_body (reply), 0, "o", &handle);

  if (data->hidden)
    {
      /* The dialog was hidden before we got the handle, close it now */
      send_close (data);
      filechooser_portal_data_free (data);
      self->mode_data = NULL;
    }
  else if (strcmp (handle, data->portal_handle) != 0)
    {
      g_free (data->portal_handle);
      data->portal_handle = g_steal_pointer (&handle);
      g_dbus_connection_signal_unsubscribe (data->connection,
                                            data->portal_response_signal_id);

      data->portal_response_signal_id =
        g_dbus_connection_signal_subscribe (data->connection,
                                            "org.freedesktop.portal.Desktop",
                                            "org.freedesktop.portal.Request",
                                            "Response",
                                            data->portal_handle,
                                            NULL,
                                            G_DBUS_SIGNAL_FLAGS_NO_MATCH_RULE,
                                            response_cb,
                                            self, NULL);
    }

  g_object_unref (reply);
  g_free (handle);
}

static GVariant *
get_filters (CtkFileChooser *self)
{
  GSList *list, *l;
  GVariantBuilder builder;

  g_variant_builder_init (&builder, G_VARIANT_TYPE ("a(sa(us))"));
  list = ctk_file_chooser_list_filters (self);
  for (l = list; l; l = l->next)
    {
      CtkFileFilter *filter = l->data;
      g_variant_builder_add (&builder, "@(sa(us))", ctk_file_filter_to_gvariant (filter));
    }
  g_slist_free (list);

  return g_variant_builder_end (&builder);
}

static GVariant *
ctk_file_chooser_native_choice_to_variant (CtkFileChooserNativeChoice *choice)
{
  GVariantBuilder choices;
  int i;

  g_variant_builder_init (&choices, G_VARIANT_TYPE ("a(ss)"));
  if (choice->options)
    {
      for (i = 0; choice->options[i]; i++)
        g_variant_builder_add (&choices, "(&s&s)", choice->options[i], choice->option_labels[i]);
    }

  return g_variant_new ("(&s&s@a(ss)&s)",
                        choice->id,
                        choice->label,
                        g_variant_builder_end (&choices),
                        choice->selected ? choice->selected : "");
}

static GVariant *
serialize_choices (CtkFileChooserNative *self)
{
  GVariantBuilder builder;
  GSList *l;

  g_variant_builder_init (&builder, G_VARIANT_TYPE ("a(ssa(ss)s)"));
  for (l = self->choices; l; l = l->next)
    {
      CtkFileChooserNativeChoice *choice = l->data;

      g_variant_builder_add (&builder, "@(ssa(ss)s)",
                             ctk_file_chooser_native_choice_to_variant (choice));
    }

  return g_variant_builder_end (&builder);
}

static void
show_portal_file_chooser (CtkFileChooserNative *self,
                          const char           *parent_window_str)
{
  FilechooserPortalData *data = self->mode_data;
  GDBusMessage *message;
  GVariantBuilder opt_builder;
  gboolean multiple;
  gboolean directory;
  const char *title;
  char *token;

  message = g_dbus_message_new_method_call ("org.freedesktop.portal.Desktop",
                                            "/org/freedesktop/portal/desktop",
                                            "org.freedesktop.portal.FileChooser",
                                            data->method_name);

  data->portal_handle = ctk_get_portal_request_path (data->connection, &token);
  data->portal_response_signal_id =
        g_dbus_connection_signal_subscribe (data->connection,
                                            "org.freedesktop.portal.Desktop",
                                            "org.freedesktop.portal.Request",
                                            "Response",
                                            data->portal_handle,
                                            NULL,
                                            G_DBUS_SIGNAL_FLAGS_NO_MATCH_RULE,
                                            response_cb,
                                            self, NULL);

  multiple = ctk_file_chooser_get_select_multiple (CTK_FILE_CHOOSER (self));
  directory = ctk_file_chooser_get_action (CTK_FILE_CHOOSER (self)) == CTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
  g_variant_builder_init (&opt_builder, G_VARIANT_TYPE_VARDICT);

  g_variant_builder_add (&opt_builder, "{sv}", "handle_token",
                         g_variant_new_string (token));
  g_free (token);

  g_variant_builder_add (&opt_builder, "{sv}", "multiple",
                         g_variant_new_boolean (multiple));
  g_variant_builder_add (&opt_builder, "{sv}", "directory",
                         g_variant_new_boolean (directory));
  if (self->accept_label)
    g_variant_builder_add (&opt_builder, "{sv}", "accept_label",
                           g_variant_new_string (self->accept_label));
  if (self->cancel_label)
    g_variant_builder_add (&opt_builder, "{sv}", "cancel_label",
                           g_variant_new_string (self->cancel_label));
  g_variant_builder_add (&opt_builder, "{sv}", "modal",
                         g_variant_new_boolean (data->modal));
  g_variant_builder_add (&opt_builder, "{sv}", "filters", get_filters (CTK_FILE_CHOOSER (self)));
  if (self->current_filter)
    g_variant_builder_add (&opt_builder, "{sv}", "current_filter",
                           ctk_file_filter_to_gvariant (self->current_filter));
  if (self->current_name)
    g_variant_builder_add (&opt_builder, "{sv}", "current_name",
                           g_variant_new_string (CTK_FILE_CHOOSER_NATIVE (self)->current_name));
  if (self->current_folder)
    {
      gchar *path;

      path = g_file_get_path (CTK_FILE_CHOOSER_NATIVE (self)->current_folder);
      g_variant_builder_add (&opt_builder, "{sv}", "current_folder",
                             g_variant_new_bytestring (path));
      g_free (path);
    }
  if (self->current_file)
    {
      gchar *path;

      path = g_file_get_path (CTK_FILE_CHOOSER_NATIVE (self)->current_file);
      g_variant_builder_add (&opt_builder, "{sv}", "current_file",
                             g_variant_new_bytestring (path));
      g_free (path);
    }

  if (self->choices)
    g_variant_builder_add (&opt_builder, "{sv}", "choices",
                           serialize_choices (CTK_FILE_CHOOSER_NATIVE (self)));

  title = ctk_native_dialog_get_title (CTK_NATIVE_DIALOG (self));

  g_dbus_message_set_body (message,
                           g_variant_new ("(ss@a{sv})",
                                          parent_window_str ? parent_window_str : "",
                                          title ? title : "",
                                          g_variant_builder_end (&opt_builder)));

  g_dbus_connection_send_message_with_reply (data->connection,
                                             message,
                                             G_DBUS_SEND_MESSAGE_FLAGS_NONE,
                                             G_MAXINT,
                                             NULL,
                                             NULL,
                                             open_file_msg_cb,
                                             data);

  g_object_unref (message);
}

static void
window_handle_exported (CtkWindow  *window,
                        const char *handle_str,
                        gpointer    user_data)
{
  CtkFileChooserNative *self = user_data;
  FilechooserPortalData *data = self->mode_data;

  if (data->modal)
    {
      CdkScreen *screen = ctk_widget_get_screen (CTK_WIDGET (window));

      data->grab_widget = ctk_invisible_new_for_screen (screen);
      ctk_grab_add (CTK_WIDGET (data->grab_widget));
    }

  show_portal_file_chooser (self, handle_str);
}

gboolean
ctk_file_chooser_native_portal_show (CtkFileChooserNative *self)
{
  FilechooserPortalData *data;
  CtkWindow *transient_for;
  GDBusConnection *connection;
  CtkFileChooserAction action;
  const char *method_name;

  if (!ctk_should_use_portal ())
    return FALSE;

  connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);
  if (connection == NULL)
    return FALSE;

  action = ctk_file_chooser_get_action (CTK_FILE_CHOOSER (self));

  if (action == CTK_FILE_CHOOSER_ACTION_OPEN)
    method_name = "OpenFile";
  else if (action == CTK_FILE_CHOOSER_ACTION_SAVE)
    method_name = "SaveFile";
  else if (action == CTK_FILE_CHOOSER_ACTION_SELECT_FOLDER)
    {
      if (ctk_get_portal_interface_version (connection, "org.freedesktop.portal.FileChooser") < 3)
        {
          g_warning ("CTK_FILE_CHOOSER_ACTION_SELECT_FOLDER is not supported by CtkFileChooserNativePortal because portal is too old");
          return FALSE;
        }
      method_name = "OpenFile";
    }
  else
    {
      g_warning ("CTK_FILE_CHOOSER_ACTION_CREATE_FOLDER is not supported by CtkFileChooserNativePortal");
      return FALSE;
    }

  data = g_new0 (FilechooserPortalData, 1);
  data->self = g_object_ref (self);
  data->connection = connection;

  data->method_name = method_name;

  if (ctk_native_dialog_get_modal (CTK_NATIVE_DIALOG (self)))
    data->modal = TRUE;

  self->mode_data = data;

  transient_for = ctk_native_dialog_get_transient_for (CTK_NATIVE_DIALOG (self));
  if (transient_for != NULL && ctk_widget_is_visible (CTK_WIDGET (transient_for)))
    {
      if (!ctk_window_export_handle (transient_for,
                                     window_handle_exported,
                                     self))
        {
          g_warning ("Failed to export handle, could not set transient for");
          show_portal_file_chooser (self, NULL);
        }
      else
        {
          data->exported_window = transient_for;
        }
    }
  else
    {
      show_portal_file_chooser (self, NULL);
    }

  return TRUE;
}

void
ctk_file_chooser_native_portal_hide (CtkFileChooserNative *self)
{
  FilechooserPortalData *data = self->mode_data;

  /* This is always set while dialog visible */
  g_assert (data != NULL);

  data->hidden = TRUE;

  if (data->portal_handle)
    {
      send_close (data);
      filechooser_portal_data_free (data);
    }

  self->mode_data = NULL;
}
