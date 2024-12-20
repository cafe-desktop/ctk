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

#include "ctkcolorpickerkwinprivate.h"
#include <gio/gio.h>

struct _CtkColorPickerKwin
{
  GObject parent_instance;

  GDBusProxy *kwin_proxy;
  GTask *task;
};

struct _CtkColorPickerKwinClass
{
  GObjectClass parent_class;
};

static GInitableIface *initable_parent_iface;
static void ctk_color_picker_kwin_initable_iface_init (GInitableIface *iface);
static void ctk_color_picker_kwin_iface_init (CtkColorPickerInterface *iface);

G_DEFINE_TYPE_WITH_CODE (CtkColorPickerKwin, ctk_color_picker_kwin, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, ctk_color_picker_kwin_initable_iface_init)
                         G_IMPLEMENT_INTERFACE (CTK_TYPE_COLOR_PICKER, ctk_color_picker_kwin_iface_init))

static gboolean
ctk_color_picker_kwin_initable_init (GInitable     *initable,
                                      GCancellable *cancellable G_GNUC_UNUSED,
                                      GError      **error)
{
  CtkColorPickerKwin *picker = CTK_COLOR_PICKER_KWIN (initable);
  char *owner;

  picker->kwin_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                      G_DBUS_PROXY_FLAGS_NONE,
                                                      NULL,
                                                      "org.kde.KWin",
                                                      "/ColorPicker",
                                                      "org.kde.kwin.ColorPicker",
                                                      NULL,
                                                      error);

  if (picker->kwin_proxy == NULL)
    {
      g_debug ("Failed to create kwin colorpicker proxy");
      return FALSE;
    }

  owner = g_dbus_proxy_get_name_owner (picker->kwin_proxy);
  if (owner == NULL)
    {
      g_debug ("org.kde.kwin.ColorPicker not provided");
      g_clear_object (&picker->kwin_proxy);
      return FALSE;
    }
  g_free (owner);

  return TRUE;
}

static void
ctk_color_picker_kwin_initable_iface_init (GInitableIface *iface)
{
  initable_parent_iface = g_type_interface_peek_parent (iface);
  iface->init = ctk_color_picker_kwin_initable_init;
}

static void
ctk_color_picker_kwin_init (CtkColorPickerKwin *picker G_GNUC_UNUSED)
{
}

static void
ctk_color_picker_kwin_finalize (GObject *object)
{
  CtkColorPickerKwin *picker = CTK_COLOR_PICKER_KWIN (object);

  g_clear_object (&picker->kwin_proxy);

  G_OBJECT_CLASS (ctk_color_picker_kwin_parent_class)->finalize (object);
}

static void
ctk_color_picker_kwin_class_init (CtkColorPickerKwinClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = ctk_color_picker_kwin_finalize;
}

CtkColorPicker *
ctk_color_picker_kwin_new (void)
{
  return CTK_COLOR_PICKER (g_initable_new (CTK_TYPE_COLOR_PICKER_KWIN, NULL, NULL, NULL));
}

static void
color_picked (GObject      *source G_GNUC_UNUSED,
              GAsyncResult *res,
              gpointer      data)
{
  CtkColorPickerKwin *picker = CTK_COLOR_PICKER_KWIN (data);
  GError *error = NULL;
  GVariant *ret;

  ret = g_dbus_proxy_call_finish (picker->kwin_proxy, res, &error);

  if (ret == NULL)
    {
      g_task_return_error (picker->task, error);
    }
  else
    {
      CdkRGBA c;
      guint32 color;

      g_variant_get (ret, "(u)", &color);

      c.blue  = ( color        & 0xff) / 255.0;
      c.green = ((color >>  8) & 0xff) / 255.0; 
      c.red   = ((color >> 16) & 0xff) / 255.0; 
      c.alpha = ((color >> 24) & 0xff) / 255.0; 

      g_task_return_pointer (picker->task, cdk_rgba_copy (&c), (GDestroyNotify)cdk_rgba_free);

      g_variant_unref (ret);
    }

  g_clear_object (&picker->task);
}

static void
ctk_color_picker_kwin_pick (CtkColorPicker      *cp,
                            GAsyncReadyCallback  callback,
                            gpointer             user_data)
{
  CtkColorPickerKwin *picker = CTK_COLOR_PICKER_KWIN (cp);

  if (picker->task)
    return;

  picker->task = g_task_new (picker, NULL, callback, user_data);

  g_dbus_proxy_call (picker->kwin_proxy,
                     "pick",
                     NULL,
                     0,
                     -1,
                     NULL,
                     color_picked,
                     picker);
}

static CdkRGBA *
ctk_color_picker_kwin_pick_finish (CtkColorPicker  *cp,
                                   GAsyncResult    *res,
                                   GError         **error)
{
  g_return_val_if_fail (g_task_is_valid (res, cp), NULL);

  return g_task_propagate_pointer (G_TASK (res), error);
}

static void
ctk_color_picker_kwin_iface_init (CtkColorPickerInterface *iface)
{
  iface->pick = ctk_color_picker_kwin_pick;
  iface->pick_finish = ctk_color_picker_kwin_pick_finish;
}
