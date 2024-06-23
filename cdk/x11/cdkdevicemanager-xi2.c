/* CDK - The GIMP Drawing Kit
 * Copyright (C) 2009 Carlos Garnacho <carlosg@gnome.org>
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

#include "cdkx11devicemanager-xi2.h"
#include "cdkx11device-xi2.h"

#include "cdkdevicemanagerprivate-core.h"
#include "cdkdeviceprivate.h"
#include "cdkdevicetoolprivate.h"
#include "cdkdisplayprivate.h"
#include "cdkeventtranslator.h"
#include "cdkprivate-x11.h"
#include "cdkintl.h"
#include "cdkkeysyms.h"
#include "cdkinternals.h"
#include "cdkseatdefaultprivate.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XInput2.h>
#include <X11/Xatom.h>

#include <string.h>

static const char *wacom_type_atoms[] = {
  "STYLUS",
  "CURSOR",
  "ERASER",
  "PAD",
  "TOUCH"
};
#define N_WACOM_TYPE_ATOMS G_N_ELEMENTS (wacom_type_atoms)

enum {
  WACOM_TYPE_STYLUS,
  WACOM_TYPE_CURSOR,
  WACOM_TYPE_ERASER,
  WACOM_TYPE_PAD,
  WACOM_TYPE_TOUCH,
};

struct _CdkX11DeviceManagerXI2
{
  CdkX11DeviceManagerCore parent_object;

  GHashTable *id_table;

  GList *devices;

  gint opcode;
  gint major;
  gint minor;
};

struct _CdkX11DeviceManagerXI2Class
{
  CdkDeviceManagerClass parent_class;
};

static void     cdk_x11_device_manager_xi2_event_translator_init (CdkEventTranslatorIface *iface);

G_DEFINE_TYPE_WITH_CODE (CdkX11DeviceManagerXI2, cdk_x11_device_manager_xi2, CDK_TYPE_X11_DEVICE_MANAGER_CORE,
                         G_IMPLEMENT_INTERFACE (CDK_TYPE_EVENT_TRANSLATOR,
                                                cdk_x11_device_manager_xi2_event_translator_init))

static void    cdk_x11_device_manager_xi2_constructed  (GObject      *object);
static void    cdk_x11_device_manager_xi2_dispose      (GObject      *object);
static void    cdk_x11_device_manager_xi2_set_property (GObject      *object,
                                                        guint         prop_id,
                                                        const GValue *value,
                                                        GParamSpec   *pspec);
static void    cdk_x11_device_manager_xi2_get_property (GObject      *object,
                                                        guint         prop_id,
                                                        GValue       *value,
                                                        GParamSpec   *pspec);

static GList * cdk_x11_device_manager_xi2_list_devices (CdkDeviceManager *device_manager,
                                                        CdkDeviceType     type);
static CdkDevice * cdk_x11_device_manager_xi2_get_client_pointer (CdkDeviceManager *device_manager);

static gboolean cdk_x11_device_manager_xi2_translate_event (CdkEventTranslator *translator,
                                                            CdkDisplay         *display,
                                                            CdkEvent           *event,
                                                            XEvent             *xevent);
static CdkEventMask cdk_x11_device_manager_xi2_get_handled_events   (CdkEventTranslator *translator);
static void         cdk_x11_device_manager_xi2_select_window_events (CdkEventTranslator *translator,
                                                                     Window              window,
                                                                     CdkEventMask        event_mask);
static CdkWindow *  cdk_x11_device_manager_xi2_get_window           (CdkEventTranslator *translator,
                                                                     XEvent             *xevent);

enum {
  PROP_0,
  PROP_OPCODE,
  PROP_MAJOR,
  PROP_MINOR
};

static void
cdk_x11_device_manager_xi2_class_init (CdkX11DeviceManagerXI2Class *klass)
{
  CdkDeviceManagerClass *device_manager_class = CDK_DEVICE_MANAGER_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = cdk_x11_device_manager_xi2_constructed;
  object_class->dispose = cdk_x11_device_manager_xi2_dispose;
  object_class->set_property = cdk_x11_device_manager_xi2_set_property;
  object_class->get_property = cdk_x11_device_manager_xi2_get_property;

  device_manager_class->list_devices = cdk_x11_device_manager_xi2_list_devices;
  device_manager_class->get_client_pointer = cdk_x11_device_manager_xi2_get_client_pointer;

  g_object_class_install_property (object_class,
                                   PROP_OPCODE,
                                   g_param_spec_int ("opcode",
                                                     P_("Opcode"),
                                                     P_("Opcode for XInput2 requests"),
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class,
                                   PROP_MAJOR,
                                   g_param_spec_int ("major",
                                                     P_("Major"),
                                                     P_("Major version number"),
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class,
                                   PROP_MINOR,
                                   g_param_spec_int ("minor",
                                                     P_("Minor"),
                                                     P_("Minor version number"),
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
cdk_x11_device_manager_xi2_init (CdkX11DeviceManagerXI2 *device_manager)
{
  device_manager->id_table = g_hash_table_new_full (g_direct_hash,
                                                    g_direct_equal,
                                                    NULL,
                                                    (GDestroyNotify) g_object_unref);
}

static void
_cdk_x11_device_manager_xi2_select_events (CdkDeviceManager *device_manager,
                                           Window            xwindow,
                                           XIEventMask      *event_mask)
{
  CdkDisplay *display;
  Display *xdisplay;

  display = cdk_device_manager_get_display (device_manager);
  xdisplay = CDK_DISPLAY_XDISPLAY (display);

  XISelectEvents (xdisplay, xwindow, event_mask, 1);
}

static void
translate_valuator_class (CdkDisplay          *display,
                          CdkDevice           *device,
                          Atom                 valuator_label,
                          gdouble              min,
                          gdouble              max,
                          gdouble              resolution)
{
  static gboolean initialized = FALSE;
  static Atom label_atoms [CDK_AXIS_LAST] = { 0 };
  CdkAxisUse use = CDK_AXIS_IGNORE;
  CdkAtom label;
  gint i;

  if (!initialized)
    {
      label_atoms [CDK_AXIS_X] = cdk_x11_get_xatom_by_name_for_display (display, "Abs X");
      label_atoms [CDK_AXIS_Y] = cdk_x11_get_xatom_by_name_for_display (display, "Abs Y");
      label_atoms [CDK_AXIS_PRESSURE] = cdk_x11_get_xatom_by_name_for_display (display, "Abs Pressure");
      label_atoms [CDK_AXIS_XTILT] = cdk_x11_get_xatom_by_name_for_display (display, "Abs Tilt X");
      label_atoms [CDK_AXIS_YTILT] = cdk_x11_get_xatom_by_name_for_display (display, "Abs Tilt Y");
      label_atoms [CDK_AXIS_WHEEL] = cdk_x11_get_xatom_by_name_for_display (display, "Abs Wheel");
      initialized = TRUE;
    }

  for (i = CDK_AXIS_IGNORE; i < CDK_AXIS_LAST; i++)
    {
      if (label_atoms[i] == valuator_label)
        {
          use = i;
          break;
        }
    }

  if (valuator_label != None)
    label = cdk_x11_xatom_to_atom_for_display (display, valuator_label);
  else
    label = CDK_NONE;

  _cdk_device_add_axis (device, label, use, min, max, resolution);
  CDK_NOTE (INPUT, g_message ("\n\taxis: %s %s", cdk_atom_name (label), use == CDK_AXIS_IGNORE ? "(ignored)" : "(used)"));
}

static void
translate_device_classes (CdkDisplay      *display,
                          CdkDevice       *device,
                          XIAnyClassInfo **classes,
                          guint            n_classes)
{
  gint i;

  g_object_freeze_notify (G_OBJECT (device));

  for (i = 0; i < n_classes; i++)
    {
      XIAnyClassInfo *class_info = classes[i];

      switch (class_info->type)
        {
        case XIKeyClass:
          {
            XIKeyClassInfo *key_info = (XIKeyClassInfo *) class_info;
            gint j;

            _cdk_device_set_keys (device, key_info->num_keycodes);

            for (j = 0; j < key_info->num_keycodes; j++)
              cdk_device_set_key (device, j, key_info->keycodes[j], 0);
          }
          break;
        case XIValuatorClass:
          {
            XIValuatorClassInfo *valuator_info = (XIValuatorClassInfo *) class_info;
            translate_valuator_class (display, device,
                                      valuator_info->label,
                                      valuator_info->min,
                                      valuator_info->max,
                                      valuator_info->resolution);
          }
          break;
#ifdef XINPUT_2_2
        case XIScrollClass:
          {
            XIScrollClassInfo *scroll_info = (XIScrollClassInfo *) class_info;
            CdkScrollDirection direction;

            if (scroll_info->scroll_type == XIScrollTypeVertical)
              direction = CDK_SCROLL_DOWN;
            else
              direction = CDK_SCROLL_RIGHT;

            CDK_NOTE (INPUT,
                      g_message ("\n\tscroll valuator %d: %s, increment %f",
                                 scroll_info->number,
                                 scroll_info->scroll_type == XIScrollTypeVertical
                                                ? "vertical"
                                                : "horizontal",
                                 scroll_info->increment));

            _cdk_x11_device_xi2_add_scroll_valuator (CDK_X11_DEVICE_XI2 (device),
                                                     scroll_info->number,
                                                     direction,
                                                     scroll_info->increment);
          }
#endif /* XINPUT_2_2 */
        default:
          /* Ignore */
          break;
        }
    }

  g_object_thaw_notify (G_OBJECT (device));
}

static gboolean
is_touch_device (XIAnyClassInfo **classes,
                 guint            n_classes,
                 CdkInputSource  *device_type,
                 gint            *num_touches)
{
#ifdef XINPUT_2_2
  guint i;

  for (i = 0; i < n_classes; i++)
    {
      XITouchClassInfo *class = (XITouchClassInfo *) classes[i];

      if (class->type != XITouchClass)
        continue;

      if (class->num_touches > 0)
        {
          if (class->mode == XIDirectTouch)
            *device_type = CDK_SOURCE_TOUCHSCREEN;
          else if (class->mode == XIDependentTouch)
            *device_type = CDK_SOURCE_TOUCHPAD;
          else
            continue;

          *num_touches = class->num_touches;

          return TRUE;
        }
    }
#endif

  return FALSE;
}

static gboolean
has_abs_axes (CdkDisplay      *display,
              XIAnyClassInfo **classes,
              guint            n_classes)
{
  gboolean has_x = FALSE, has_y = FALSE;
  Atom abs_x, abs_y;
  guint i;

  abs_x = cdk_x11_get_xatom_by_name_for_display (display, "Abs X");
  abs_y = cdk_x11_get_xatom_by_name_for_display (display, "Abs Y");

  for (i = 0; i < n_classes; i++)
    {
      XIValuatorClassInfo *class = (XIValuatorClassInfo *) classes[i];

      if (class->type != XIValuatorClass)
        continue;
      if (class->mode != XIModeAbsolute)
        continue;

      if (class->label == abs_x)
        has_x = TRUE;
      else if (class->label == abs_y)
        has_y = TRUE;

      if (has_x && has_y)
        break;
    }

  return (has_x && has_y);
}

static gboolean
get_device_ids (CdkDisplay    *display,
                XIDeviceInfo  *info,
                gchar        **vendor_id,
                gchar        **product_id)
{
  gulong nitems, bytes_after;
  guint32 *data;
  int rc, format;
  Atom prop, type;

  cdk_x11_display_error_trap_push (display);

  prop = XInternAtom (CDK_DISPLAY_XDISPLAY (display), "Device Product ID", True);

  if (prop == None)
    {
      cdk_x11_display_error_trap_pop_ignored (display);
      return 0;
    }

  rc = XIGetProperty (CDK_DISPLAY_XDISPLAY (display),
                      info->deviceid, prop,
                      0, 2, False, XA_INTEGER, &type, &format, &nitems, &bytes_after,
                      (guchar **) &data);
  cdk_x11_display_error_trap_pop_ignored (display);

  if (rc != Success || type != XA_INTEGER || format != 32 || nitems != 2)
    return FALSE;

  if (vendor_id)
    *vendor_id = g_strdup_printf ("%.4x", data[0]);
  if (product_id)
    *product_id = g_strdup_printf ("%.4x", data[1]);

  XFree (data);

  return TRUE;
}

static gboolean
is_touchpad_device (CdkDisplay   *display,
                    XIDeviceInfo *info)
{
  gulong nitems, bytes_after;
  guint32 *data;
  int rc, format;
  Atom type;

  cdk_x11_display_error_trap_push (display);

  rc = XIGetProperty (CDK_DISPLAY_XDISPLAY (display),
                      info->deviceid,
                      cdk_x11_get_xatom_by_name_for_display (display, "libinput Tapping Enabled"),
                      0, 1, False, XA_INTEGER, &type, &format, &nitems, &bytes_after,
                      (guchar **) &data);
  cdk_x11_display_error_trap_pop_ignored (display);

  if (rc != Success || type != XA_INTEGER || format != 8 || nitems != 1)
    return FALSE;

  XFree (data);

  return TRUE;
}

static CdkDevice *
create_device (CdkDeviceManager *device_manager,
               CdkDisplay       *display,
               XIDeviceInfo     *dev)
{
  CdkInputSource input_source;
  CdkInputSource touch_source;
  CdkDeviceType type;
  CdkDevice *device;
  CdkInputMode mode;
  gint num_touches = 0;
  gchar *vendor_id = NULL, *product_id = NULL;

  if (dev->use == XIMasterKeyboard || dev->use == XISlaveKeyboard)
    input_source = CDK_SOURCE_KEYBOARD;
  else if (is_touchpad_device (display, dev))
    input_source = CDK_SOURCE_TOUCHPAD;
  else if (dev->use == XISlavePointer &&
           is_touch_device (dev->classes, dev->num_classes, &touch_source, &num_touches))
    input_source = touch_source;
  else
    {
      gchar *tmp_name;

      tmp_name = g_ascii_strdown (dev->name, -1);

      if (strstr (tmp_name, "eraser"))
        input_source = CDK_SOURCE_ERASER;
      else if (strstr (tmp_name, "cursor"))
        input_source = CDK_SOURCE_CURSOR;
      else if (strstr (tmp_name, " pad"))
        input_source = CDK_SOURCE_TABLET_PAD;
      else if (strstr (tmp_name, "wacom") ||
               strstr (tmp_name, "pen"))
        input_source = CDK_SOURCE_PEN;
      else if (!strstr (tmp_name, "mouse") &&
               !strstr (tmp_name, "pointer") &&
               !strstr (tmp_name, "qemu usb tablet") &&
               !strstr (tmp_name, "spice vdagent tablet") &&
               !strstr (tmp_name, "virtualbox usb tablet") &&
               has_abs_axes (display, dev->classes, dev->num_classes))
        input_source = CDK_SOURCE_TOUCHSCREEN;
      else if (strstr (tmp_name, "trackpoint") ||
               strstr (tmp_name, "dualpoint stick"))
        input_source = CDK_SOURCE_TRACKPOINT;
      else
        input_source = CDK_SOURCE_MOUSE;

      g_free (tmp_name);
    }

  switch (dev->use)
    {
    case XIMasterKeyboard:
    case XIMasterPointer:
      type = CDK_DEVICE_TYPE_MASTER;
      mode = CDK_MODE_SCREEN;
      break;
    case XISlaveKeyboard:
    case XISlavePointer:
      type = CDK_DEVICE_TYPE_SLAVE;
      mode = CDK_MODE_DISABLED;
      break;
    case XIFloatingSlave:
    default:
      type = CDK_DEVICE_TYPE_FLOATING;
      mode = CDK_MODE_DISABLED;
      break;
    }

  CDK_NOTE (INPUT,
            ({
              const gchar *type_names[] = { "master", "slave", "floating" };
              const gchar *source_names[] = { "mouse", "pen", "eraser", "cursor", "keyboard", "direct touch", "indirect touch", "trackpoint", "pad" };
              const gchar *mode_names[] = { "disabled", "screen", "window" };
              g_message ("input device:\n\tname: %s\n\ttype: %s\n\tsource: %s\n\tmode: %s\n\thas cursor: %d\n\ttouches: %d",
                         dev->name,
                         type_names[type],
                         source_names[input_source],
                         mode_names[mode],
                         dev->use == XIMasterPointer,
                         num_touches);
            }));

  if (dev->use != XIMasterKeyboard &&
      dev->use != XIMasterPointer)
    get_device_ids (display, dev, &vendor_id, &product_id);

  device = g_object_new (CDK_TYPE_X11_DEVICE_XI2,
                         "name", dev->name,
                         "type", type,
                         "input-source", input_source,
                         "input-mode", mode,
                         "has-cursor", (dev->use == XIMasterPointer),
                         "display", display,
                         "device-manager", device_manager,
                         "device-id", dev->deviceid,
                         "vendor-id", vendor_id,
                         "product-id", product_id,
                         "num-touches", num_touches,
                         NULL);

  translate_device_classes (display, device, dev->classes, dev->num_classes);
  g_free (vendor_id);
  g_free (product_id);

  return device;
}

static void
ensure_seat_for_device_pair (CdkX11DeviceManagerXI2 *device_manager,
                             CdkDevice              *device1,
                             CdkDevice              *device2)
{
  CdkDisplay *display;
  CdkSeat *seat;

  display = cdk_device_manager_get_display (CDK_DEVICE_MANAGER (device_manager));
  seat = cdk_device_get_seat (device1);

  if (!seat)
    {
      CdkDevice *pointer, *keyboard;

      if (cdk_device_get_source (device1) == CDK_SOURCE_KEYBOARD)
        {
          keyboard = device1;
          pointer = device2;
        }
      else
        {
          pointer = device1;
          keyboard = device2;
        }

      seat = cdk_seat_default_new_for_master_pair (pointer, keyboard);
      cdk_display_add_seat (display, seat);
      g_object_unref (seat);
    }
}

static CdkDevice *
add_device (CdkX11DeviceManagerXI2 *device_manager,
            XIDeviceInfo           *dev,
            gboolean                emit_signal)
{
  CdkDisplay *display;
  CdkDevice *device;

  display = cdk_device_manager_get_display (CDK_DEVICE_MANAGER (device_manager));
  device = create_device (CDK_DEVICE_MANAGER (device_manager), display, dev);

  g_hash_table_replace (device_manager->id_table,
                        GINT_TO_POINTER (dev->deviceid),
                        g_object_ref (device));

  device_manager->devices = g_list_append (device_manager->devices, device);

  if (emit_signal)
    {
      if (dev->use == XISlavePointer || dev->use == XISlaveKeyboard)
        {
          CdkDevice *master;
          CdkSeat *seat;

          /* The device manager is already constructed, then
           * keep the hierarchy coherent for the added device.
           */
          master = g_hash_table_lookup (device_manager->id_table,
                                        GINT_TO_POINTER (dev->attachment));

          _cdk_device_set_associated_device (device, master);
          _cdk_device_add_slave (master, device);

          seat = cdk_device_get_seat (master);
          cdk_seat_default_add_slave (CDK_SEAT_DEFAULT (seat), device);
        }
      else if (dev->use == XIMasterPointer || dev->use == XIMasterKeyboard)
        {
          CdkDevice *relative;

          relative = g_hash_table_lookup (device_manager->id_table,
                                          GINT_TO_POINTER (dev->attachment));

          if (relative)
            {
              _cdk_device_set_associated_device (device, relative);
              _cdk_device_set_associated_device (relative, device);
              ensure_seat_for_device_pair (device_manager, device, relative);
            }
        }
    }

    g_signal_emit_by_name (device_manager, "device-added", device);

  return device;
}

static void
detach_from_seat (CdkDevice *device)
{
  CdkSeat *seat = cdk_device_get_seat (device);

  if (!seat)
    return;

  if (cdk_device_get_device_type (device) == CDK_DEVICE_TYPE_MASTER)
    cdk_display_remove_seat (cdk_device_get_display (device), seat);
  else if (cdk_device_get_device_type (device) == CDK_DEVICE_TYPE_SLAVE)
    cdk_seat_default_remove_slave (CDK_SEAT_DEFAULT (seat), device);
}

static void
remove_device (CdkX11DeviceManagerXI2 *device_manager,
               gint                    device_id)
{
  CdkDevice *device;

  device = g_hash_table_lookup (device_manager->id_table,
                                GINT_TO_POINTER (device_id));

  if (device)
    {
      detach_from_seat (device);

      g_hash_table_remove (device_manager->id_table,
                           GINT_TO_POINTER (device_id));

      device_manager->devices = g_list_remove (device_manager->devices, device);
      g_signal_emit_by_name (device_manager, "device-removed", device);
      g_object_run_dispose (G_OBJECT (device));
      g_object_unref (device);
    }
}

static void
relate_masters (gpointer key,
                gpointer value,
                gpointer user_data)
{
  CdkX11DeviceManagerXI2 *device_manager;
  CdkDevice *device, *relative;

  device_manager = user_data;
  device = g_hash_table_lookup (device_manager->id_table, key);
  relative = g_hash_table_lookup (device_manager->id_table, value);

  _cdk_device_set_associated_device (device, relative);
  _cdk_device_set_associated_device (relative, device);
  ensure_seat_for_device_pair (device_manager, device, relative);
}

static void
relate_slaves (gpointer key,
               gpointer value,
               gpointer user_data)
{
  CdkX11DeviceManagerXI2 *device_manager;
  CdkDevice *slave, *master;
  CdkSeat *seat;

  device_manager = user_data;
  slave = g_hash_table_lookup (device_manager->id_table, key);
  master = g_hash_table_lookup (device_manager->id_table, value);

  _cdk_device_set_associated_device (slave, master);
  _cdk_device_add_slave (master, slave);

  seat = cdk_device_get_seat (master);
  cdk_seat_default_add_slave (CDK_SEAT_DEFAULT (seat), slave);
}

static void
cdk_x11_device_manager_xi2_constructed (GObject *object)
{
  CdkX11DeviceManagerXI2 *device_manager;
  CdkDisplay *display;
  CdkScreen *screen;
  GHashTable *masters, *slaves;
  Display *xdisplay;
  XIDeviceInfo *info;
  int ndevices, i;
  XIEventMask event_mask;
  unsigned char mask[2] = { 0 };

  G_OBJECT_CLASS (cdk_x11_device_manager_xi2_parent_class)->constructed (object);

  device_manager = CDK_X11_DEVICE_MANAGER_XI2 (object);
  display = cdk_device_manager_get_display (CDK_DEVICE_MANAGER (object));
  xdisplay = CDK_DISPLAY_XDISPLAY (display);

  g_assert (device_manager->major == 2);

  masters = g_hash_table_new (NULL, NULL);
  slaves = g_hash_table_new (NULL, NULL);

  info = XIQueryDevice (xdisplay, XIAllDevices, &ndevices);

  /* Initialize devices list */
  for (i = 0; i < ndevices; i++)
    {
      XIDeviceInfo *dev;

      dev = &info[i];

      if (!dev->enabled)
	      continue;

      add_device (device_manager, dev, FALSE);

      if (dev->use == XIMasterPointer ||
          dev->use == XIMasterKeyboard)
        {
          g_hash_table_insert (masters,
                               GINT_TO_POINTER (dev->deviceid),
                               GINT_TO_POINTER (dev->attachment));
        }
      else if (dev->use == XISlavePointer ||
               dev->use == XISlaveKeyboard)
        {
          g_hash_table_insert (slaves,
                               GINT_TO_POINTER (dev->deviceid),
                               GINT_TO_POINTER (dev->attachment));
        }
    }

  XIFreeDeviceInfo (info);

  /* Stablish relationships between devices */
  g_hash_table_foreach (masters, relate_masters, object);
  g_hash_table_destroy (masters);

  g_hash_table_foreach (slaves, relate_slaves, object);
  g_hash_table_destroy (slaves);

  /* Connect to hierarchy change events */
  screen = cdk_display_get_default_screen (display);
  XISetMask (mask, XI_HierarchyChanged);
  XISetMask (mask, XI_DeviceChanged);
  XISetMask (mask, XI_PropertyEvent);

  event_mask.deviceid = XIAllDevices;
  event_mask.mask_len = sizeof (mask);
  event_mask.mask = mask;

  _cdk_x11_device_manager_xi2_select_events (CDK_DEVICE_MANAGER (object),
                                             CDK_WINDOW_XID (cdk_screen_get_root_window (screen)),
                                             &event_mask);
}

static void
cdk_x11_device_manager_xi2_dispose (GObject *object)
{
  CdkX11DeviceManagerXI2 *device_manager;

  device_manager = CDK_X11_DEVICE_MANAGER_XI2 (object);

  g_list_free_full (device_manager->devices, g_object_unref);
  device_manager->devices = NULL;

  if (device_manager->id_table)
    {
      g_hash_table_destroy (device_manager->id_table);
      device_manager->id_table = NULL;
    }

  G_OBJECT_CLASS (cdk_x11_device_manager_xi2_parent_class)->dispose (object);
}

static GList *
cdk_x11_device_manager_xi2_list_devices (CdkDeviceManager *device_manager,
                                         CdkDeviceType     type)
{
  CdkX11DeviceManagerXI2 *device_manager_xi2;
  GList *cur, *list = NULL;

  device_manager_xi2 = CDK_X11_DEVICE_MANAGER_XI2 (device_manager);

  for (cur = device_manager_xi2->devices; cur; cur = cur->next)
    {
      CdkDevice *dev = cur->data;

      if (type == cdk_device_get_device_type (dev))
        list = g_list_prepend (list, dev);
    }

  return list;
}

static CdkDevice *
cdk_x11_device_manager_xi2_get_client_pointer (CdkDeviceManager *device_manager)
{
  CdkX11DeviceManagerXI2 *device_manager_xi2;
  CdkDisplay *display;
  int device_id;

  device_manager_xi2 = (CdkX11DeviceManagerXI2 *) device_manager;
  display = cdk_device_manager_get_display (device_manager);

  XIGetClientPointer (CDK_DISPLAY_XDISPLAY (display),
                      None, &device_id);

  return g_hash_table_lookup (device_manager_xi2->id_table,
                              GINT_TO_POINTER (device_id));
}

static void
cdk_x11_device_manager_xi2_set_property (GObject      *object,
                                         guint         prop_id,
                                         const GValue *value,
                                         GParamSpec   *pspec)
{
  CdkX11DeviceManagerXI2 *device_manager;

  device_manager = CDK_X11_DEVICE_MANAGER_XI2 (object);

  switch (prop_id)
    {
    case PROP_OPCODE:
      device_manager->opcode = g_value_get_int (value);
      break;
    case PROP_MAJOR:
      device_manager->major = g_value_get_int (value);
      break;
    case PROP_MINOR:
      device_manager->minor = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
cdk_x11_device_manager_xi2_get_property (GObject    *object,
                                         guint       prop_id,
                                         GValue     *value,
                                         GParamSpec *pspec)
{
  CdkX11DeviceManagerXI2 *device_manager;

  device_manager = CDK_X11_DEVICE_MANAGER_XI2 (object);

  switch (prop_id)
    {
    case PROP_OPCODE:
      g_value_set_int (value, device_manager->opcode);
      break;
    case PROP_MAJOR:
      g_value_set_int (value, device_manager->major);
      break;
    case PROP_MINOR:
      g_value_set_int (value, device_manager->minor);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
cdk_x11_device_manager_xi2_event_translator_init (CdkEventTranslatorIface *iface)
{
  iface->translate_event = cdk_x11_device_manager_xi2_translate_event;
  iface->get_handled_events = cdk_x11_device_manager_xi2_get_handled_events;
  iface->select_window_events = cdk_x11_device_manager_xi2_select_window_events;
  iface->get_window = cdk_x11_device_manager_xi2_get_window;
}

static void
handle_hierarchy_changed (CdkX11DeviceManagerXI2 *device_manager,
                          XIHierarchyEvent       *ev)
{
  CdkDisplay *display;
  Display *xdisplay;
  XIDeviceInfo *info;
  int ndevices;
  gint i;

  display = cdk_device_manager_get_display (CDK_DEVICE_MANAGER (device_manager));
  xdisplay = CDK_DISPLAY_XDISPLAY (display);

  for (i = 0; i < ev->num_info; i++)
    {
      if (ev->info[i].flags & XIDeviceEnabled)
        {
          cdk_x11_display_error_trap_push (display);
          info = XIQueryDevice (xdisplay, ev->info[i].deviceid, &ndevices);
          cdk_x11_display_error_trap_pop_ignored (display);
          if (info)
            {
              add_device (device_manager, &info[0], TRUE);
              XIFreeDeviceInfo (info);
            }
        }
      else if (ev->info[i].flags & XIDeviceDisabled)
        remove_device (device_manager, ev->info[i].deviceid);
      else if (ev->info[i].flags & XISlaveAttached ||
               ev->info[i].flags & XISlaveDetached)
        {
          CdkDevice *master, *slave;
          CdkSeat *seat;

          slave = g_hash_table_lookup (device_manager->id_table,
                                       GINT_TO_POINTER (ev->info[i].deviceid));

          if (!slave)
            continue;

          /* Remove old master info */
          master = cdk_device_get_associated_device (slave);

          if (master)
            {
              _cdk_device_remove_slave (master, slave);
              _cdk_device_set_associated_device (slave, NULL);

              g_signal_emit_by_name (device_manager, "device-changed", master);

              seat = cdk_device_get_seat (master);
              cdk_seat_default_remove_slave (CDK_SEAT_DEFAULT (seat), slave);
            }

          /* Add new master if it's an attachment event */
          if (ev->info[i].flags & XISlaveAttached)
            {
              cdk_x11_display_error_trap_push (display);
              info = XIQueryDevice (xdisplay, ev->info[i].deviceid, &ndevices);
              cdk_x11_display_error_trap_pop_ignored (display);
              if (info)
                {
                  master = g_hash_table_lookup (device_manager->id_table,
                                                GINT_TO_POINTER (info->attachment));
                  XIFreeDeviceInfo (info);
                }

              if (master)
                {
                  _cdk_device_set_associated_device (slave, master);
                  _cdk_device_add_slave (master, slave);

                  seat = cdk_device_get_seat (master);
                  cdk_seat_default_add_slave (CDK_SEAT_DEFAULT (seat), slave);

                  g_signal_emit_by_name (device_manager, "device-changed", master);
                }
            }

          g_signal_emit_by_name (device_manager, "device-changed", slave);
        }
    }
}

static void
handle_device_changed (CdkX11DeviceManagerXI2 *device_manager,
                       XIDeviceChangedEvent   *ev)
{
  CdkDisplay *display;
  CdkDevice *device, *source_device;

  display = cdk_device_manager_get_display (CDK_DEVICE_MANAGER (device_manager));
  device = g_hash_table_lookup (device_manager->id_table,
                                GUINT_TO_POINTER (ev->deviceid));
  source_device = g_hash_table_lookup (device_manager->id_table,
                                       GUINT_TO_POINTER (ev->sourceid));

  if (device)
    {
      _cdk_device_reset_axes (device);
      _cdk_device_xi2_unset_scroll_valuators ((CdkX11DeviceXI2 *) device);
      cdk_x11_device_xi2_store_axes (CDK_X11_DEVICE_XI2 (device), NULL, 0);
      translate_device_classes (display, device, ev->classes, ev->num_classes);

      g_signal_emit_by_name (G_OBJECT (device), "changed");
    }

  if (source_device)
    _cdk_device_xi2_reset_scroll_valuators (CDK_X11_DEVICE_XI2 (source_device));
}

static gboolean
device_get_tool_serial_and_id (CdkDevice *device,
                               guint     *serial_id,
                               guint     *id)
{
  CdkDisplay *display;
  gulong nitems, bytes_after;
  guint32 *data;
  int rc, format;
  Atom type;

  display = cdk_device_get_display (device);

  cdk_x11_display_error_trap_push (display);

  rc = XIGetProperty (CDK_DISPLAY_XDISPLAY (display),
                      cdk_x11_device_get_id (device),
                      cdk_x11_get_xatom_by_name_for_display (display, "Wacom Serial IDs"),
                      0, 5, False, XA_INTEGER, &type, &format, &nitems, &bytes_after,
                      (guchar **) &data);
  cdk_x11_display_error_trap_pop_ignored (display);

  if (rc != Success)
    return FALSE;

  if (type == XA_INTEGER && format == 32)
    {
      if (nitems >= 4)
        *serial_id = data[3];
      if (nitems >= 5)
        *id = data[4];
    }

  XFree (data);

  return TRUE;
}

static CdkDeviceToolType
device_get_tool_type (CdkDevice *device)
{
  CdkDisplay *display;
  gulong nitems, bytes_after;
  guint32 *data;
  int rc, format;
  Atom type;
  Atom device_type;
  Atom types[N_WACOM_TYPE_ATOMS];
  CdkDeviceToolType tool_type = CDK_DEVICE_TOOL_TYPE_UNKNOWN;

  display = cdk_device_get_display (device);
  cdk_x11_display_error_trap_push (display);

  rc = XIGetProperty (CDK_DISPLAY_XDISPLAY (display),
                      cdk_x11_device_get_id (device),
                      cdk_x11_get_xatom_by_name_for_display (display, "Wacom Tool Type"),
                      0, 1, False, XA_ATOM, &type, &format, &nitems, &bytes_after,
                      (guchar **) &data);
  cdk_x11_display_error_trap_pop_ignored (display);

  if (rc != Success)
    return CDK_DEVICE_TOOL_TYPE_UNKNOWN;

  if (type != XA_ATOM || format != 32 || nitems != 1)
    {
      XFree (data);
      return CDK_DEVICE_TOOL_TYPE_UNKNOWN;
    }

  device_type = *data;
  XFree (data);

  if (device_type == 0)
    return CDK_DEVICE_TOOL_TYPE_UNKNOWN;

  cdk_x11_display_error_trap_push (display);
  rc = XInternAtoms (CDK_DISPLAY_XDISPLAY (display),
                     (char **) wacom_type_atoms,
                     N_WACOM_TYPE_ATOMS,
                     False,
                     types);
  cdk_x11_display_error_trap_pop_ignored (display);

  if (rc == 0)
    return CDK_DEVICE_TOOL_TYPE_UNKNOWN;

  if (device_type == types[WACOM_TYPE_STYLUS])
    tool_type = CDK_DEVICE_TOOL_TYPE_PEN;
  else if (device_type == types[WACOM_TYPE_CURSOR])
    tool_type = CDK_DEVICE_TOOL_TYPE_MOUSE;
  else if (device_type == types[WACOM_TYPE_ERASER])
    tool_type = CDK_DEVICE_TOOL_TYPE_ERASER;
  else if (device_type == types[WACOM_TYPE_TOUCH])
    tool_type = CDK_DEVICE_TOOL_TYPE_UNKNOWN;

  return tool_type;
}

static void
handle_property_change (CdkX11DeviceManagerXI2 *device_manager,
                        XIPropertyEvent        *ev)
{
  CdkDevice *device;

  device = g_hash_table_lookup (device_manager->id_table,
                                GUINT_TO_POINTER (ev->deviceid));

  if (device != NULL &&
      ev->property == cdk_x11_get_xatom_by_name ("Wacom Serial IDs"))
    {
      CdkDeviceTool *tool = NULL;
      guint serial_id = 0, tool_id = 0;

      if (ev->what != XIPropertyDeleted &&
          device_get_tool_serial_and_id (device, &serial_id, &tool_id))
        {
          CdkSeat *seat;

          seat = cdk_device_get_seat (device);
          tool = cdk_seat_get_tool (seat, serial_id, tool_id);

          if (!tool && serial_id > 0)
            {
              CdkDeviceToolType tool_type;

              tool_type = device_get_tool_type (device);
              if (tool_type != CDK_DEVICE_TOOL_TYPE_UNKNOWN)
                {
                  tool = cdk_device_tool_new (serial_id, tool_id, tool_type, 0);
                  cdk_seat_default_add_tool (CDK_SEAT_DEFAULT (seat), tool);
                }
            }
        }

      cdk_device_update_tool (device, tool);
    }
}

static CdkCrossingMode
translate_crossing_mode (gint mode)
{
  switch (mode)
    {
    case XINotifyNormal:
      return CDK_CROSSING_NORMAL;
    case XINotifyGrab:
    case XINotifyPassiveGrab:
      return CDK_CROSSING_GRAB;
    case XINotifyUngrab:
    case XINotifyPassiveUngrab:
      return CDK_CROSSING_UNGRAB;
    case XINotifyWhileGrabbed:
      /* Fall through, unexpected in pointer crossing events */
    default:
      g_assert_not_reached ();
    }
}

static CdkNotifyType
translate_notify_type (gint detail)
{
  switch (detail)
    {
    case NotifyInferior:
      return CDK_NOTIFY_INFERIOR;
    case NotifyAncestor:
      return CDK_NOTIFY_ANCESTOR;
    case NotifyVirtual:
      return CDK_NOTIFY_VIRTUAL;
    case NotifyNonlinear:
      return CDK_NOTIFY_NONLINEAR;
    case NotifyNonlinearVirtual:
      return CDK_NOTIFY_NONLINEAR_VIRTUAL;
    default:
      g_assert_not_reached ();
    }
}

static gboolean
set_screen_from_root (CdkDisplay *display,
                      CdkEvent   *event,
                      Window      xrootwin)
{
  CdkScreen *screen;

  screen = _cdk_x11_display_screen_for_xrootwin (display, xrootwin);

  if (screen)
    {
      cdk_event_set_screen (event, screen);

      return TRUE;
    }

  return FALSE;
}

static void
set_user_time (CdkEvent *event)
{
  CdkWindow *window;
  guint32 time;

  window = cdk_window_get_toplevel (event->any.window);
  g_return_if_fail (CDK_IS_WINDOW (window));

  time = cdk_event_get_time (event);

  /* If an event doesn't have a valid timestamp, we shouldn't use it
   * to update the latest user interaction time.
   */
  if (time != CDK_CURRENT_TIME)
    cdk_x11_window_set_user_time (window, time);
}

static gdouble *
translate_axes (CdkDevice       *device,
                gdouble          x,
                gdouble          y,
                CdkWindow       *window,
                XIValuatorState *valuators)
{
  guint n_axes, i;
  gdouble *axes;
  gdouble *vals;

  g_object_get (device, "n-axes", &n_axes, NULL);

  axes = g_new0 (gdouble, n_axes);
  vals = valuators->values;

  for (i = 0; i < MIN (valuators->mask_len * 8, n_axes); i++)
    {
      CdkAxisUse use;
      gdouble val;

      if (!XIMaskIsSet (valuators->mask, i))
        {
          axes[i] = cdk_x11_device_xi2_get_last_axis_value (CDK_X11_DEVICE_XI2 (device), i);
          continue;
        }

      use = cdk_device_get_axis_use (device, i);
      val = *vals++;

      switch (use)
        {
        case CDK_AXIS_X:
        case CDK_AXIS_Y:
          if (cdk_device_get_mode (device) == CDK_MODE_WINDOW)
            _cdk_device_translate_window_coord (device, window, i, val, &axes[i]);
          else
            {
              if (use == CDK_AXIS_X)
                axes[i] = x;
              else
                axes[i] = y;
            }
          break;
        default:
          _cdk_device_translate_axis (device, i, val, &axes[i]);
          break;
        }
    }

  cdk_x11_device_xi2_store_axes (CDK_X11_DEVICE_XI2 (device), axes, n_axes);

  return axes;
}

static gboolean
is_parent_of (CdkWindow *parent,
              CdkWindow *child)
{
  CdkWindow *w;

  w = child;
  while (w != NULL)
    {
      if (w == parent)
        return TRUE;

      w = cdk_window_get_parent (w);
    }

  return FALSE;
}

static gboolean
get_event_window (CdkEventTranslator *translator,
                  XIEvent            *ev,
                  CdkWindow         **window_p)
{
  CdkDisplay *display;
  CdkWindow *window = NULL;
  gboolean should_have_window = TRUE;

  display = cdk_device_manager_get_display (CDK_DEVICE_MANAGER (translator));

  switch (ev->evtype)
    {
    case XI_KeyPress:
    case XI_KeyRelease:
    case XI_ButtonPress:
    case XI_ButtonRelease:
    case XI_Motion:
#ifdef XINPUT_2_2
    case XI_TouchUpdate:
    case XI_TouchBegin:
    case XI_TouchEnd:
#endif /* XINPUT_2_2 */
      {
        XIDeviceEvent *xev = (XIDeviceEvent *) ev;

        window = cdk_x11_window_lookup_for_display (display, xev->event);

        /* Apply keyboard grabs to non-native windows */
        if (ev->evtype == XI_KeyPress || ev->evtype == XI_KeyRelease)
          {
            CdkDeviceGrabInfo *info;
            CdkDevice *device;
            gulong serial;

            device = g_hash_table_lookup (CDK_X11_DEVICE_MANAGER_XI2 (translator)->id_table,
                                          GUINT_TO_POINTER (((XIDeviceEvent *) ev)->deviceid));

            serial = _cdk_display_get_next_serial (display);
            info = _cdk_display_has_device_grab (display, device, serial);

            if (info &&
                (!is_parent_of (info->window, window) ||
                 !info->owner_events))
              {
                /* Report key event against grab window */
                window = info->window;
              }
          }
      }
      break;
    case XI_Enter:
    case XI_Leave:
    case XI_FocusIn:
    case XI_FocusOut:
      {
        XIEnterEvent *xev = (XIEnterEvent *) ev;

        window = cdk_x11_window_lookup_for_display (display, xev->event);
      }
      break;
    default:
      should_have_window = FALSE;
      break;
    }

  *window_p = window;

  if (should_have_window && !window)
    return FALSE;

  return TRUE;
}

static gboolean
cdk_x11_device_manager_xi2_translate_core_event (CdkEventTranslator *translator,
						 CdkDisplay         *display,
						 CdkEvent           *event,
						 XEvent             *xevent)
{
  CdkEventTranslatorIface *parent_iface;
  gboolean keyboard = FALSE;
  CdkDevice *device;

  if ((xevent->type == KeyPress || xevent->type == KeyRelease) &&
      (xevent->xkey.keycode == 0 || xevent->xkey.serial == 0))
    {
      /* The X input methods (when triggered via XFilterEvent)
       * generate a core key press event with keycode 0 to signal the
       * end of a key sequence. We use the core translate_event
       * implementation to translate this event.
       *
       * Other less educated IM modules like to filter every keypress,
       * only to have these replaced by their own homegrown events,
       * these events oddly have serial=0, so we try to catch these.
       *
       * This is just a bandaid fix to keep xim working with a single
       * keyboard until XFilterEvent learns about XI2.
       */
      keyboard = TRUE;
    }
  else if (xevent->xany.send_event)
    {
      /* If another process sends us core events, process them; we
       * assume that it won't send us redundant core and XI2 events.
       * (At the moment, it's not possible to send XI2 events anyway.
       * In the future, an app that was trying to decide whether to
       * send core or XI2 events could look at the event mask on the
       * window to see which kind we are listening to.)
       */
      switch (xevent->type)
	{
	case KeyPress:
	case KeyRelease:
	case FocusIn:
	case FocusOut:
	  keyboard = TRUE;
	  break;

	case ButtonPress:
	case ButtonRelease:
	case MotionNotify:
	case EnterNotify:
	case LeaveNotify:
	  break;

	default:
	  return FALSE;
	}
    }
  else
    return FALSE;

  parent_iface = g_type_interface_peek_parent (CDK_EVENT_TRANSLATOR_GET_IFACE (translator));
  if (!parent_iface->translate_event (translator, display, event, xevent))
    return FALSE;

  /* The core device manager sets a core device on the event.
   * We need to override that with an XI2 device, since we are
   * using XI2.
   */
  device = cdk_x11_device_manager_xi2_get_client_pointer ((CdkDeviceManager *)translator);
  if (keyboard)
    device = cdk_device_get_associated_device (device);
  cdk_event_set_device (event, device);

  return TRUE;
}

static gboolean
scroll_valuators_changed (CdkX11DeviceXI2 *device,
                          XIValuatorState *valuators,
                          gdouble         *dx,
                          gdouble         *dy)
{
  gboolean has_scroll_valuators = FALSE;
  CdkScrollDirection direction;
  guint n_axes, i, n_val;
  gdouble *vals;

  n_axes = cdk_device_get_n_axes (CDK_DEVICE (device));
  vals = valuators->values;
  *dx = *dy = 0;
  n_val = 0;

  for (i = 0; i < MIN (valuators->mask_len * 8, n_axes); i++)
    {
      gdouble delta;

      if (!XIMaskIsSet (valuators->mask, i))
        continue;

      if (_cdk_x11_device_xi2_get_scroll_delta (device, i, vals[n_val],
                                                &direction, &delta))
        {
          has_scroll_valuators = TRUE;

          if (direction == CDK_SCROLL_UP ||
              direction == CDK_SCROLL_DOWN)
            *dy = delta;
          else
            *dx = delta;
        }

      n_val++;
    }

  return has_scroll_valuators;
}

static gboolean
cdk_x11_device_manager_xi2_translate_event (CdkEventTranslator *translator,
                                            CdkDisplay         *display,
                                            CdkEvent           *event,
                                            XEvent             *xevent)
{
  CdkX11DeviceManagerXI2 *device_manager;
  XGenericEventCookie *cookie;
  CdkDevice *device, *source_device;
  gboolean return_val = TRUE;
  CdkWindow *window;
  CdkWindowImplX11 *impl;
  int scale;
  XIEvent *ev;

  device_manager = (CdkX11DeviceManagerXI2 *) translator;
  cookie = &xevent->xcookie;

  if (xevent->type != GenericEvent)
    return cdk_x11_device_manager_xi2_translate_core_event (translator, display, event, xevent);
  else if (cookie->extension != device_manager->opcode)
    return FALSE;

  ev = (XIEvent *) cookie->data;

  if (!ev)
    return FALSE;

  if (!get_event_window (translator, ev, &window))
    return FALSE;

  if (window && CDK_WINDOW_DESTROYED (window))
    return FALSE;

  scale = 1;
  if (window)
    {
      impl = CDK_WINDOW_IMPL_X11 (window->impl);
      scale = impl->window_scale;
    }

  if (ev->evtype == XI_Motion ||
      ev->evtype == XI_ButtonRelease)
    {
      if (_cdk_x11_moveresize_handle_event (xevent))
        return FALSE;
    }

  switch (ev->evtype)
    {
    case XI_HierarchyChanged:
      handle_hierarchy_changed (device_manager,
                                (XIHierarchyEvent *) ev);
      return_val = FALSE;
      break;
    case XI_DeviceChanged:
      handle_device_changed (device_manager,
                             (XIDeviceChangedEvent *) ev);
      return_val = FALSE;
      break;
    case XI_PropertyEvent:
      handle_property_change (device_manager,
                              (XIPropertyEvent *) ev);
      return_val = FALSE;
      break;
    case XI_KeyPress:
    case XI_KeyRelease:
      {
        XIDeviceEvent *xev = (XIDeviceEvent *) ev;
        CdkKeymap *keymap = cdk_keymap_get_for_display (display);
        CdkModifierType consumed, state;

        CDK_NOTE (EVENTS,
                  g_message ("key %s:\twindow %ld\n"
                             "\tdevice:%u\n"
                             "\tsource device:%u\n"
                             "\tkey number: %u\n",
                             (ev->evtype == XI_KeyPress) ? "press" : "release",
                             xev->event,
                             xev->deviceid,
                             xev->sourceid,
                             xev->detail));

        event->key.type = xev->evtype == XI_KeyPress ? CDK_KEY_PRESS : CDK_KEY_RELEASE;

        event->key.window = window;

        event->key.time = xev->time;
        event->key.state = _cdk_x11_device_xi2_translate_state (&xev->mods, &xev->buttons, &xev->group);
        event->key.group = xev->group.effective;

        event->key.hardware_keycode = xev->detail;
        cdk_event_set_scancode (event, xev->detail);
        event->key.is_modifier = cdk_x11_keymap_key_is_modifier (keymap, event->key.hardware_keycode);

        device = g_hash_table_lookup (device_manager->id_table,
                                      GUINT_TO_POINTER (xev->deviceid));
        cdk_event_set_device (event, device);

        source_device = g_hash_table_lookup (device_manager->id_table,
                                             GUINT_TO_POINTER (xev->sourceid));
        cdk_event_set_source_device (event, source_device);
        cdk_event_set_seat (event, cdk_device_get_seat (device));

        event->key.keyval = CDK_KEY_VoidSymbol;

        cdk_keymap_translate_keyboard_state (keymap,
                                             event->key.hardware_keycode,
                                             event->key.state,
                                             event->key.group,
                                             &event->key.keyval,
                                             NULL, NULL, &consumed);

        state = event->key.state & ~consumed;
        _cdk_x11_keymap_add_virt_mods (keymap, &state);
        event->key.state |= state;

        _cdk_x11_event_translate_keyboard_string (&event->key);

        if (ev->evtype == XI_KeyPress)
          set_user_time (event);

        /* FIXME: emulate autorepeat on key
         * release? XI2 seems attached to Xkb.
         */
      }

      break;
    case XI_ButtonPress:
    case XI_ButtonRelease:
      {
        XIDeviceEvent *xev = (XIDeviceEvent *) ev;

        CDK_NOTE (EVENTS,
                  g_message ("button %s:\twindow %ld\n"
                             "\tdevice:%u\n"
                             "\tsource device:%u\n"
                             "\tbutton number: %u\n"
                             "\tx,y: %.2f %.2f",
                             (ev->evtype == XI_ButtonPress) ? "press" : "release",
                             xev->event,
                             xev->deviceid,
                             xev->sourceid,
                             xev->detail,
                             xev->event_x, xev->event_y));

        if (ev->evtype == XI_ButtonRelease &&
            (xev->detail >= 4 && xev->detail <= 7))
          return FALSE;
        else if (ev->evtype == XI_ButtonPress &&
                 (xev->detail >= 4 && xev->detail <= 7))
          {
            /* Button presses of button 4-7 are scroll events */
            event->scroll.type = CDK_SCROLL;

            if (xev->detail == 4)
              event->scroll.direction = CDK_SCROLL_UP;
            else if (xev->detail == 5)
              event->scroll.direction = CDK_SCROLL_DOWN;
            else if (xev->detail == 6)
              event->scroll.direction = CDK_SCROLL_LEFT;
            else
              event->scroll.direction = CDK_SCROLL_RIGHT;

            event->scroll.window = window;
            event->scroll.time = xev->time;
            event->scroll.x = (gdouble) xev->event_x / scale;
            event->scroll.y = (gdouble) xev->event_y / scale;
            event->scroll.x_root = (gdouble) xev->root_x / scale;
            event->scroll.y_root = (gdouble) xev->root_y / scale;
            event->scroll.delta_x = 0;
            event->scroll.delta_y = 0;

            device = g_hash_table_lookup (device_manager->id_table,
                                          GUINT_TO_POINTER (xev->deviceid));
            cdk_event_set_device (event, device);

            source_device = g_hash_table_lookup (device_manager->id_table,
                                                 GUINT_TO_POINTER (xev->sourceid));
            cdk_event_set_source_device (event, source_device);
            cdk_event_set_seat (event, cdk_device_get_seat (device));

            event->scroll.state = _cdk_x11_device_xi2_translate_state (&xev->mods, &xev->buttons, &xev->group);

#ifdef XINPUT_2_2
            if (xev->flags & XIPointerEmulated)
              cdk_event_set_pointer_emulated (event, TRUE);
#endif
          }
        else
          {
            event->button.type = (ev->evtype == XI_ButtonPress) ? CDK_BUTTON_PRESS : CDK_BUTTON_RELEASE;

            event->button.window = window;
            event->button.time = xev->time;
            event->button.x = (gdouble) xev->event_x / scale;
            event->button.y = (gdouble) xev->event_y / scale;
            event->button.x_root = (gdouble) xev->root_x / scale;
            event->button.y_root = (gdouble) xev->root_y / scale;

            device = g_hash_table_lookup (device_manager->id_table,
                                          GUINT_TO_POINTER (xev->deviceid));
            cdk_event_set_device (event, device);

            source_device = g_hash_table_lookup (device_manager->id_table,
                                                 GUINT_TO_POINTER (xev->sourceid));
            cdk_event_set_source_device (event, source_device);
            cdk_event_set_seat (event, cdk_device_get_seat (device));
            cdk_event_set_device_tool (event, source_device->last_tool);

            event->button.axes = translate_axes (event->button.device,
                                                 event->button.x,
                                                 event->button.y,
                                                 event->button.window,
                                                 &xev->valuators);

            if (cdk_device_get_mode (event->button.device) == CDK_MODE_WINDOW)
              {
                CdkDevice *device = event->button.device;

                /* Update event coordinates from axes */
                cdk_device_get_axis (device, event->button.axes, CDK_AXIS_X, &event->button.x);
                cdk_device_get_axis (device, event->button.axes, CDK_AXIS_Y, &event->button.y);
              }

            event->button.state = _cdk_x11_device_xi2_translate_state (&xev->mods, &xev->buttons, &xev->group);

            event->button.button = xev->detail;
          }

#ifdef XINPUT_2_2
        if (xev->flags & XIPointerEmulated)
          cdk_event_set_pointer_emulated (event, TRUE);
#endif

        if (return_val == FALSE)
          break;

        if (!set_screen_from_root (display, event, xev->root))
          {
            return_val = FALSE;
            break;
          }

        if (ev->evtype == XI_ButtonPress)
	  set_user_time (event);

        break;
      }

    case XI_Motion:
      {
        XIDeviceEvent *xev = (XIDeviceEvent *) ev;
        gdouble delta_x, delta_y;

        source_device = g_hash_table_lookup (device_manager->id_table,
                                             GUINT_TO_POINTER (xev->sourceid));
        device = g_hash_table_lookup (device_manager->id_table,
                                      GUINT_TO_POINTER (xev->deviceid));

        /* When scrolling, X might send events twice here; once with both the
         * device and the source device set to the physical device, and once
         * with the device set to the master device.
         * Since we are only interested in the latter, and
         * scroll_valuators_changed() updates the valuator cache for the
         * source device, we need to explicitly ignore the first event in
         * order to get the correct delta for the second.
         */
        if (cdk_device_get_device_type (device) != CDK_DEVICE_TYPE_SLAVE &&
            scroll_valuators_changed (CDK_X11_DEVICE_XI2 (source_device),
                                      &xev->valuators, &delta_x, &delta_y))
          {
            event->scroll.type = CDK_SCROLL;
            event->scroll.direction = CDK_SCROLL_SMOOTH;

            if (delta_x == 0.0 && delta_y == 0.0)
              event->scroll.is_stop = TRUE;

            CDK_NOTE(EVENTS,
                     g_message ("smooth scroll: %s\n\tdevice: %u\n\tsource device: %u\n\twindow %ld\n\tdeltas: %f %f",
#ifdef XINPUT_2_2
                                (xev->flags & XIPointerEmulated) ? "emulated" : "",
#else
                                 "",
#endif
                                xev->deviceid, xev->sourceid,
                                xev->event, delta_x, delta_y));


            event->scroll.window = window;
            event->scroll.time = xev->time;
            event->scroll.x = (gdouble) xev->event_x / scale;
            event->scroll.y = (gdouble) xev->event_y / scale;
            event->scroll.x_root = (gdouble) xev->root_x / scale;
            event->scroll.y_root = (gdouble) xev->root_y / scale;
            event->scroll.delta_x = delta_x;
            event->scroll.delta_y = delta_y;

            event->scroll.device = device;
            cdk_event_set_source_device (event, source_device);
            cdk_event_set_seat (event, cdk_device_get_seat (device));

            event->scroll.state = _cdk_x11_device_xi2_translate_state (&xev->mods, &xev->buttons, &xev->group);
            break;
          }

        event->motion.type = CDK_MOTION_NOTIFY;
        event->motion.window = window;
        event->motion.time = xev->time;
        event->motion.x = (gdouble) xev->event_x / scale;
        event->motion.y = (gdouble) xev->event_y / scale;
        event->motion.x_root = (gdouble) xev->root_x / scale;
        event->motion.y_root = (gdouble) xev->root_y / scale;

        event->motion.device = device;
        cdk_event_set_source_device (event, source_device);
        cdk_event_set_seat (event, cdk_device_get_seat (device));
        cdk_event_set_device_tool (event, source_device->last_tool);

        event->motion.state = _cdk_x11_device_xi2_translate_state (&xev->mods, &xev->buttons, &xev->group);

#ifdef XINPUT_2_2
        if (xev->flags & XIPointerEmulated)
          cdk_event_set_pointer_emulated (event, TRUE);
#endif

        /* There doesn't seem to be motion hints in XI */
        event->motion.is_hint = FALSE;

        event->motion.axes = translate_axes (event->motion.device,
                                             event->motion.x,
                                             event->motion.y,
                                             event->motion.window,
                                             &xev->valuators);

        if (cdk_device_get_mode (event->motion.device) == CDK_MODE_WINDOW)
          {
            /* Update event coordinates from axes */
            cdk_device_get_axis (event->motion.device, event->motion.axes, CDK_AXIS_X, &event->motion.x);
            cdk_device_get_axis (event->motion.device, event->motion.axes, CDK_AXIS_Y, &event->motion.y);
          }
      }
      break;

#ifdef XINPUT_2_2
    case XI_TouchBegin:
    case XI_TouchEnd:
      {
        XIDeviceEvent *xev = (XIDeviceEvent *) ev;

        CDK_NOTE(EVENTS,
                 g_message ("touch %s:\twindow %ld\n\ttouch id: %u\n\tpointer emulating: %s",
                            ev->evtype == XI_TouchBegin ? "begin" : "end",
                            xev->event,
                            xev->detail,
                            xev->flags & XITouchEmulatingPointer ? "true" : "false"));

        if (ev->evtype == XI_TouchBegin)
          event->touch.type = CDK_TOUCH_BEGIN;
        else if (ev->evtype == XI_TouchEnd)
          event->touch.type = CDK_TOUCH_END;

        event->touch.window = window;
        event->touch.time = xev->time;
        event->touch.x = (gdouble) xev->event_x / scale;
        event->touch.y = (gdouble) xev->event_y / scale;
        event->touch.x_root = (gdouble) xev->root_x / scale;
        event->touch.y_root = (gdouble) xev->root_y / scale;

        device = g_hash_table_lookup (device_manager->id_table,
                                      GUINT_TO_POINTER (xev->deviceid));
        cdk_event_set_device (event, device);

        source_device = g_hash_table_lookup (device_manager->id_table,
                                             GUINT_TO_POINTER (xev->sourceid));
        cdk_event_set_source_device (event, source_device);
        cdk_event_set_seat (event, cdk_device_get_seat (device));

        event->touch.axes = translate_axes (event->touch.device,
                                            event->touch.x,
                                            event->touch.y,
                                            event->touch.window,
                                            &xev->valuators);

        if (cdk_device_get_mode (event->touch.device) == CDK_MODE_WINDOW)
          {
            CdkDevice *device = event->touch.device;

            /* Update event coordinates from axes */
            cdk_device_get_axis (device, event->touch.axes, CDK_AXIS_X, &event->touch.x);
            cdk_device_get_axis (device, event->touch.axes, CDK_AXIS_Y, &event->touch.y);
          }

        event->touch.state = _cdk_x11_device_xi2_translate_state (&xev->mods, &xev->buttons, &xev->group);

        if (ev->evtype == XI_TouchBegin)
          event->touch.state |= CDK_BUTTON1_MASK;

        event->touch.sequence = GUINT_TO_POINTER (xev->detail);

        if (xev->flags & XITouchEmulatingPointer)
          {
            event->touch.emulating_pointer = TRUE;
            cdk_event_set_pointer_emulated (event, TRUE);
          }

        if (return_val == FALSE)
          break;

        if (!set_screen_from_root (display, event, xev->root))
          {
            return_val = FALSE;
            break;
          }

        if (ev->evtype == XI_TouchBegin)
          set_user_time (event);
      }
      break;

    case XI_TouchUpdate:
      {
        XIDeviceEvent *xev = (XIDeviceEvent *) ev;

        CDK_NOTE(EVENTS,
                 g_message ("touch update:\twindow %ld\n\ttouch id: %u\n\tpointer emulating: %s",
                            xev->event,
                            xev->detail,
                            xev->flags & XITouchEmulatingPointer ? "true" : "false"));

        event->touch.window = window;
        event->touch.sequence = GUINT_TO_POINTER (xev->detail);
        event->touch.type = CDK_TOUCH_UPDATE;
        event->touch.time = xev->time;
        event->touch.x = (gdouble) xev->event_x / scale;
        event->touch.y = (gdouble) xev->event_y / scale;
        event->touch.x_root = (gdouble) xev->root_x / scale;
        event->touch.y_root = (gdouble) xev->root_y / scale;

        device = g_hash_table_lookup (device_manager->id_table,
                                      GINT_TO_POINTER (xev->deviceid));
        cdk_event_set_device (event, device);

        source_device = g_hash_table_lookup (device_manager->id_table,
                                             GUINT_TO_POINTER (xev->sourceid));
        cdk_event_set_source_device (event, source_device);
        cdk_event_set_seat (event, cdk_device_get_seat (device));

        event->touch.state = _cdk_x11_device_xi2_translate_state (&xev->mods, &xev->buttons, &xev->group);

        event->touch.state |= CDK_BUTTON1_MASK;

        if (xev->flags & XITouchEmulatingPointer)
          {
            event->touch.emulating_pointer = TRUE;
            cdk_event_set_pointer_emulated (event, TRUE);
          }

        event->touch.axes = translate_axes (event->touch.device,
                                            event->touch.x,
                                            event->touch.y,
                                            event->touch.window,
                                            &xev->valuators);

        if (cdk_device_get_mode (event->touch.device) == CDK_MODE_WINDOW)
          {
            CdkDevice *device = event->touch.device;

            /* Update event coordinates from axes */
            cdk_device_get_axis (device, event->touch.axes, CDK_AXIS_X, &event->touch.x);
            cdk_device_get_axis (device, event->touch.axes, CDK_AXIS_Y, &event->touch.y);
          }
      }
      break;
#endif  /* XINPUT_2_2 */

    case XI_Enter:
    case XI_Leave:
      {
        XIEnterEvent *xev = (XIEnterEvent *) ev;

        CDK_NOTE (EVENTS,
                  g_message ("%s notify:\twindow %ld\n\tsubwindow:%ld\n"
                             "\tdevice: %u\n\tsource device: %u\n"
                             "\tnotify type: %u\n\tcrossing mode: %u",
                             (ev->evtype == XI_Enter) ? "enter" : "leave",
                             xev->event, xev->child,
                             xev->deviceid, xev->sourceid,
                             xev->detail, xev->mode));

        event->crossing.type = (ev->evtype == XI_Enter) ? CDK_ENTER_NOTIFY : CDK_LEAVE_NOTIFY;

        event->crossing.x = (gdouble) xev->event_x / scale;
        event->crossing.y = (gdouble) xev->event_y / scale;
        event->crossing.x_root = (gdouble) xev->root_x / scale;
        event->crossing.y_root = (gdouble) xev->root_y / scale;
        event->crossing.time = xev->time;
        event->crossing.focus = xev->focus;

        event->crossing.window = window;
        event->crossing.subwindow = cdk_x11_window_lookup_for_display (display, xev->child);

        device = g_hash_table_lookup (device_manager->id_table,
                                      GINT_TO_POINTER (xev->deviceid));
        cdk_event_set_device (event, device);

        source_device = g_hash_table_lookup (device_manager->id_table,
                                             GUINT_TO_POINTER (xev->sourceid));
        cdk_event_set_source_device (event, source_device);
        cdk_event_set_seat (event, cdk_device_get_seat (device));

        if (ev->evtype == XI_Enter &&
            xev->detail != XINotifyInferior && xev->mode != XINotifyPassiveUngrab &&
	    cdk_window_get_window_type (window) == CDK_WINDOW_TOPLEVEL)
          {
            if (cdk_device_get_device_type (source_device) != CDK_DEVICE_TYPE_MASTER)
              _cdk_device_xi2_reset_scroll_valuators (CDK_X11_DEVICE_XI2 (source_device));
            else
              {
                GList *slaves, *l;

                slaves = cdk_device_list_slave_devices (source_device);

                for (l = slaves; l; l = l->next)
                  _cdk_device_xi2_reset_scroll_valuators (CDK_X11_DEVICE_XI2 (l->data));

                g_list_free (slaves);
              }
          }

        event->crossing.mode = translate_crossing_mode (xev->mode);
        event->crossing.detail = translate_notify_type (xev->detail);
        event->crossing.state = _cdk_x11_device_xi2_translate_state (&xev->mods, &xev->buttons, &xev->group);
      }
      break;
    case XI_FocusIn:
    case XI_FocusOut:
      {
        if (window)
          {
            XIEnterEvent *xev = (XIEnterEvent *) ev;

            device = g_hash_table_lookup (device_manager->id_table,
                                          GINT_TO_POINTER (xev->deviceid));

            source_device = g_hash_table_lookup (device_manager->id_table,
                                                 GUINT_TO_POINTER (xev->sourceid));

            _cdk_device_manager_core_handle_focus (window,
                                                   xev->event,
                                                   device,
                                                   source_device,
                                                   (ev->evtype == XI_FocusIn) ? TRUE : FALSE,
                                                   xev->detail,
                                                   xev->mode);
          }

        return_val = FALSE;
      }
      break;
    default:
      return_val = FALSE;
      break;
    }

  event->any.send_event = cookie->send_event;

  if (return_val)
    {
      if (event->any.window)
        g_object_ref (event->any.window);

      if (((event->any.type == CDK_ENTER_NOTIFY) ||
           (event->any.type == CDK_LEAVE_NOTIFY)) &&
          (event->crossing.subwindow != NULL))
        g_object_ref (event->crossing.subwindow);
    }
  else
    {
      /* Mark this event as having no resources to be freed */
      event->any.window = NULL;
      event->any.type = CDK_NOTHING;
    }

  return return_val;
}

static CdkEventMask
cdk_x11_device_manager_xi2_get_handled_events (CdkEventTranslator *translator G_GNUC_UNUSED)
{
  return (CDK_KEY_PRESS_MASK |
          CDK_KEY_RELEASE_MASK |
          CDK_BUTTON_PRESS_MASK |
          CDK_BUTTON_RELEASE_MASK |
          CDK_SCROLL_MASK |
          CDK_ENTER_NOTIFY_MASK |
          CDK_LEAVE_NOTIFY_MASK |
          CDK_POINTER_MOTION_MASK |
          CDK_POINTER_MOTION_HINT_MASK |
          CDK_BUTTON1_MOTION_MASK |
          CDK_BUTTON2_MOTION_MASK |
          CDK_BUTTON3_MOTION_MASK |
          CDK_BUTTON_MOTION_MASK |
          CDK_FOCUS_CHANGE_MASK |
          CDK_TOUCH_MASK);
}

static void
cdk_x11_device_manager_xi2_select_window_events (CdkEventTranslator *translator,
                                                 Window              window,
                                                 CdkEventMask        evmask)
{
  CdkDeviceManager *device_manager;
  XIEventMask event_mask;

  device_manager = CDK_DEVICE_MANAGER (translator);

  event_mask.deviceid = XIAllMasterDevices;
  event_mask.mask = _cdk_x11_device_xi2_translate_event_mask (CDK_X11_DEVICE_MANAGER_XI2 (device_manager),
                                                              evmask,
                                                              &event_mask.mask_len);

  _cdk_x11_device_manager_xi2_select_events (device_manager, window, &event_mask);
  g_free (event_mask.mask);
}

static CdkWindow *
cdk_x11_device_manager_xi2_get_window (CdkEventTranslator *translator,
                                       XEvent             *xevent)
{
  CdkX11DeviceManagerXI2 *device_manager;
  XIEvent *ev;
  CdkWindow *window = NULL;

  device_manager = (CdkX11DeviceManagerXI2 *) translator;

  if (xevent->type != GenericEvent ||
      xevent->xcookie.extension != device_manager->opcode)
    return NULL;

  ev = (XIEvent *) xevent->xcookie.data;
  if (!ev)
    return NULL;

  get_event_window (translator, ev, &window);
  return window;
}

CdkDevice *
_cdk_x11_device_manager_xi2_lookup (CdkX11DeviceManagerXI2 *device_manager_xi2,
                                    gint                    device_id)
{
  return g_hash_table_lookup (device_manager_xi2->id_table,
                              GINT_TO_POINTER (device_id));
}
