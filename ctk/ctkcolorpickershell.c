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

#include "ctkcolorpickershellprivate.h"
#include <gio/gio.h>

struct _CtkColorPickerShell
{
  GObject parent_instance;

  GDBusProxy *shell_proxy;
  GTask *task;
};

struct _CtkColorPickerShellClass
{
  GObjectClass parent_class;
};

static GInitableIface *initable_parent_iface;
static void ctk_color_picker_shell_initable_iface_init (GInitableIface *iface);
static void ctk_color_picker_shell_iface_init (CtkColorPickerInterface *iface);

G_DEFINE_TYPE_WITH_CODE (CtkColorPickerShell, ctk_color_picker_shell, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, ctk_color_picker_shell_initable_iface_init)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_COLOR_PICKER, ctk_color_picker_shell_iface_init))

static gboolean
ctk_color_picker_shell_initable_init (GInitable     *initable,
                                      GCancellable  *cancellable G_GNUC_UNUSED,
                                      GError       **error)
{
  CtkColorPickerShell *picker = CTK_COLOR_PICKER_SHELL (initable);
  char *owner;

  picker->shell_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                        G_DBUS_PROXY_FLAGS_NONE,
                                                        NULL,
                                                        "org.gnome.Shell.Screenshot",
                                                        "/org/gnome/Shell/Screenshot",
                                                        "org.gnome.Shell.Screenshot",
                                                        NULL,
                                                        error);

  if (picker->shell_proxy == NULL)
    {
      g_debug ("Failed to create shell screenshot proxy");
      return FALSE;
    }

  owner = g_dbus_proxy_get_name_owner (picker->shell_proxy);
  if (owner == NULL)
    {
      g_debug ("org.gnome.Shell.Screenshot not provided");
      g_clear_object (&picker->shell_proxy);
      return FALSE;
    }
  g_free (owner);

  return TRUE;
}

static void
ctk_color_picker_shell_initable_iface_init (GInitableIface *iface)
{
  initable_parent_iface = g_type_interface_peek_parent (iface);
  iface->init = ctk_color_picker_shell_initable_init;
}

static void
ctk_color_picker_shell_init (CtkColorPickerShell *picker G_GNUC_UNUSED)
{
}

static void
ctk_color_picker_shell_finalize (GObject *object)
{
  CtkColorPickerShell *picker = CTK_COLOR_PICKER_SHELL (object);

  g_clear_object (&picker->shell_proxy);

  G_OBJECT_CLASS (ctk_color_picker_shell_parent_class)->finalize (object);
}

static void
ctk_color_picker_shell_class_init (CtkColorPickerShellClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = ctk_color_picker_shell_finalize;
}

CtkColorPicker *
ctk_color_picker_shell_new (void)
{
  return CTK_COLOR_PICKER (g_initable_new (CTK_TYPE_COLOR_PICKER_SHELL, NULL, NULL, NULL));
}

static void
color_picked (GObject      *source G_GNUC_UNUSED,
              GAsyncResult *res,
              gpointer      data)
{
  CtkColorPickerShell *picker = CTK_COLOR_PICKER_SHELL (data);
  GError *error = NULL;
  GVariant *ret, *dict;

  ret = g_dbus_proxy_call_finish (picker->shell_proxy, res, &error);

  if (ret == NULL)
    {
      g_task_return_error (picker->task, error);
    }
  else
    {
      CdkRGBA c;

      g_variant_get (ret, "(@a{sv})", &dict);

      c.alpha = 1;
      if (!g_variant_lookup (dict, "color", "(ddd)", &c.red, &c.green, &c.blue))
        g_task_return_new_error (picker->task, G_IO_ERROR, G_IO_ERROR_FAILED, "No color received");
      else
        g_task_return_pointer (picker->task, cdk_rgba_copy (&c), (GDestroyNotify)cdk_rgba_free);

      g_variant_unref (dict);
      g_variant_unref (ret);
    }

  g_clear_object (&picker->task);
}

static void
ctk_color_picker_shell_pick (CtkColorPicker      *cp,
                             GAsyncReadyCallback  callback,
                             gpointer             user_data)
{
  CtkColorPickerShell *picker = CTK_COLOR_PICKER_SHELL (cp);

  if (picker->task)
    return;

  picker->task = g_task_new (picker, NULL, callback, user_data);

  g_dbus_proxy_call (picker->shell_proxy,
                     "PickColor",
                     NULL,
                     0,
                     -1,
                     NULL,
                     color_picked,
                     picker);
}

static CdkRGBA *
ctk_color_picker_shell_pick_finish (CtkColorPicker  *cp,
                                     GAsyncResult    *res,
                                     GError         **error)
{
  g_return_val_if_fail (g_task_is_valid (res, cp), NULL);

  return g_task_propagate_pointer (G_TASK (res), error);
}

static void
ctk_color_picker_shell_iface_init (CtkColorPickerInterface *iface)
{
  iface->pick = ctk_color_picker_shell_pick;
  iface->pick_finish = ctk_color_picker_shell_pick_finish;
}
