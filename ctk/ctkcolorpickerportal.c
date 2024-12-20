/* CTK - The GIMP Toolkit
 * Copyright (C) 2018, Red Hat, Inc
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

#include "ctkcolorpickerportalprivate.h"
#include "ctkprivate.h"
#include <gio/gio.h>

struct _CtkColorPickerPortal
{
  GObject parent_instance;

  GDBusProxy *portal_proxy;
  guint portal_signal_id;
  GTask *task;
};

struct _CtkColorPickerPortalClass
{
  GObjectClass parent_class;
};

static GInitableIface *initable_parent_iface;
static void ctk_color_picker_portal_initable_iface_init (GInitableIface *iface);
static void ctk_color_picker_portal_iface_init (CtkColorPickerInterface *iface);

G_DEFINE_TYPE_WITH_CODE (CtkColorPickerPortal, ctk_color_picker_portal, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, ctk_color_picker_portal_initable_iface_init)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_COLOR_PICKER, ctk_color_picker_portal_iface_init))

static gboolean
ctk_color_picker_portal_initable_init (GInitable     *initable,
                                       GCancellable  *cancellable G_GNUC_UNUSED,
                                       GError       **error)
{
  CtkColorPickerPortal *picker = CTK_COLOR_PICKER_PORTAL (initable);
  char *owner;
  GVariant *ret;
  guint version = 0;

  if (!ctk_should_use_portal ())
    return FALSE;

  picker->portal_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                        G_DBUS_PROXY_FLAGS_NONE,
                                                        NULL,
                                                        "org.freedesktop.portal.Desktop",
                                                        "/org/freedesktop/portal/desktop",
                                                        "org.freedesktop.portal.Screenshot",
                                                        NULL,
                                                        error);

  if (picker->portal_proxy == NULL)
    {
      g_debug ("Failed to create screenshot portal proxy");
      return FALSE;
    }

  owner = g_dbus_proxy_get_name_owner (picker->portal_proxy);
  if (owner == NULL)
    {
      g_debug ("org.freedesktop.portal.Screenshot not provided");
      g_clear_object (&picker->portal_proxy);
      return FALSE;
    }
  g_free (owner);

  ret = g_dbus_proxy_get_cached_property (picker->portal_proxy, "version");
  if (ret)
    {
      version = g_variant_get_uint32 (ret);
      g_variant_unref (ret);
    }

  if (version != 2)
    {
      g_debug ("Screenshot portal version: %u", version);
      g_clear_object (&picker->portal_proxy);
      return FALSE;
    }

  return TRUE;
}

static void
ctk_color_picker_portal_initable_iface_init (GInitableIface *iface)
{
  initable_parent_iface = g_type_interface_peek_parent (iface);
  iface->init = ctk_color_picker_portal_initable_init;
}

static void
ctk_color_picker_portal_init (CtkColorPickerPortal *picker G_GNUC_UNUSED)
{
}

static void
ctk_color_picker_portal_finalize (GObject *object)
{
  CtkColorPickerPortal *picker = CTK_COLOR_PICKER_PORTAL (object);

  g_clear_object (&picker->portal_proxy);

  G_OBJECT_CLASS (ctk_color_picker_portal_parent_class)->finalize (object);
}

static void
ctk_color_picker_portal_class_init (CtkColorPickerPortalClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = ctk_color_picker_portal_finalize;
}

CtkColorPicker *
ctk_color_picker_portal_new (void)
{
  return CTK_COLOR_PICKER (g_initable_new (CTK_TYPE_COLOR_PICKER_PORTAL, NULL, NULL, NULL));
}

static void
portal_response_received (GDBusConnection *connection,
                          const char      *sender_name G_GNUC_UNUSED,
                          const char      *object_path G_GNUC_UNUSED,
                          const char      *interface_name G_GNUC_UNUSED,
                          const char      *signal_name G_GNUC_UNUSED,
                          GVariant        *parameters,
                          gpointer         user_data)
{
  CtkColorPickerPortal *picker = user_data;
  guint32 response;
  GVariant *ret;

  g_dbus_connection_signal_unsubscribe (connection, picker->portal_signal_id);
  picker->portal_signal_id = 0;

  g_variant_get (parameters, "(u@a{sv})", &response, &ret);

  if (response == 0)
    {
      CdkRGBA c;

      c.alpha = 1.0;
      if (g_variant_lookup (ret, "color", "(ddd)", &c.red, &c.green, &c.blue))
        g_task_return_pointer (picker->task, cdk_rgba_copy (&c), (GDestroyNotify)cdk_rgba_free);
      else
        g_task_return_new_error (picker->task,
                                 G_IO_ERROR,
                                 G_IO_ERROR_FAILED,
                                 "No color received");
    }
  else
    g_task_return_new_error (picker->task,
                             G_IO_ERROR,
                             G_IO_ERROR_FAILED,
                             "PickColor error");

  g_variant_unref (ret);

  g_clear_object (&picker->task);
}

static void
ctk_color_picker_portal_pick (CtkColorPicker      *cp,
                              GAsyncReadyCallback  callback,
                              gpointer             user_data)
{
  CtkColorPickerPortal *picker = CTK_COLOR_PICKER_PORTAL (cp);
  GVariantBuilder options;
  GDBusConnection *connection;
  char *token;
  char *handle;

  if (picker->task)
    return;

  picker->task = g_task_new (picker, NULL, callback, user_data);

  connection = g_dbus_proxy_get_connection (picker->portal_proxy);

  handle = ctk_get_portal_request_path (connection, &token);
  picker->portal_signal_id = g_dbus_connection_signal_subscribe (connection,
                                                                 "org.freedesktop.portal.Desktop",
                                                                 "org.freedesktop.portal.Request",
                                                                 "Response",
                                                                 handle,
                                                                 NULL,
                                                                 G_DBUS_SIGNAL_FLAGS_NO_MATCH_RULE,
                                                                 portal_response_received,
                                                                 picker,
                                                                 NULL);

  g_free (handle);

  g_variant_builder_init (&options, G_VARIANT_TYPE_VARDICT);
  g_variant_builder_add (&options, "{sv}", "handle_token", g_variant_new_string (token));
  g_free (token);

  g_dbus_proxy_call (picker->portal_proxy,
                     "PickColor",
                     g_variant_new ("(sa{sv})", "", &options),
                     0,
                     -1,
                     NULL,
                     NULL,
                     NULL);
}

static CdkRGBA *
ctk_color_picker_portal_pick_finish (CtkColorPicker  *cp,
                                     GAsyncResult    *res,
                                     GError         **error)
{
  g_return_val_if_fail (g_task_is_valid (res, cp), NULL);

  return g_task_propagate_pointer (G_TASK (res), error);
}

static void
ctk_color_picker_portal_iface_init (CtkColorPickerInterface *iface)
{
  iface->pick = ctk_color_picker_portal_pick;
  iface->pick_finish = ctk_color_picker_portal_pick_finish;
}
