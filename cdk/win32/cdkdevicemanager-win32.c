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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <cdk/cdk.h>
#include "cdkwin32.h"
#include "cdkprivate-win32.h"
#include "cdkdevicemanager-win32.h"
#include "cdkdeviceprivate.h"
#include "cdkdevice-win32.h"
#include "cdkdevice-virtual.h"
#include "cdkdevice-wintab.h"
#include "cdkdisplayprivate.h"
#include "cdkseatdefaultprivate.h"

#define WINTAB32_DLL "Wintab32.dll"

#define PACKETDATA (PK_CONTEXT | PK_CURSOR | PK_BUTTONS | PK_X | PK_Y  | PK_NORMAL_PRESSURE | PK_ORIENTATION)
/* We want everything in absolute mode */
#define PACKETMODE (0)
#include <pktdef.h>

#define DEBUG_WINTAB 1		/* Verbose debug messages enabled */
#define TWOPI (2 * G_PI)

static GList     *wintab_contexts = NULL;
static CdkWindow *wintab_window = NULL;
extern gint       _cdk_input_ignore_core;

typedef UINT (WINAPI *t_WTInfoA) (UINT a, UINT b, LPVOID c);
typedef UINT (WINAPI *t_WTInfoW) (UINT a, UINT b, LPVOID c);
typedef BOOL (WINAPI *t_WTEnable) (HCTX a, BOOL b);
typedef HCTX (WINAPI *t_WTOpenA) (HWND a, LPLOGCONTEXTA b, BOOL c);
typedef BOOL (WINAPI *t_WTGetA) (HCTX a, LPLOGCONTEXTA b);
typedef BOOL (WINAPI *t_WTSetA) (HCTX a, LPLOGCONTEXTA b);
typedef BOOL (WINAPI *t_WTOverlap) (HCTX a, BOOL b);
typedef BOOL (WINAPI *t_WTPacket) (HCTX a, UINT b, LPVOID c);
typedef int (WINAPI *t_WTQueueSizeSet) (HCTX a, int b);

static t_WTInfoA p_WTInfoA;
static t_WTInfoW p_WTInfoW;
static t_WTEnable p_WTEnable;
static t_WTOpenA p_WTOpenA;
static t_WTGetA p_WTGetA;
static t_WTSetA p_WTSetA;
static t_WTOverlap p_WTOverlap;
static t_WTPacket p_WTPacket;
static t_WTQueueSizeSet p_WTQueueSizeSet;

static gboolean default_display_opened = FALSE;

G_DEFINE_TYPE (CdkDeviceManagerWin32, cdk_device_manager_win32, CDK_TYPE_DEVICE_MANAGER)

static CdkDevice *
create_pointer (CdkDeviceManager *device_manager,
		GType g_type,
		const char *name,
		CdkDeviceType type)
{
  return g_object_new (g_type,
                       "name", name,
                       "type", type,
                       "input-source", CDK_SOURCE_MOUSE,
                       "input-mode", CDK_MODE_SCREEN,
                       "has-cursor", type == CDK_DEVICE_TYPE_MASTER,
                       "display", cdk_device_manager_get_display (device_manager),
                       "device-manager", device_manager,
                       NULL);
}

static CdkDevice *
create_keyboard (CdkDeviceManager *device_manager,
		 GType g_type,
		 const char *name,
		 CdkDeviceType type)
{
  return g_object_new (g_type,
                       "name", name,
                       "type", type,
                       "input-source", CDK_SOURCE_KEYBOARD,
                       "input-mode", CDK_MODE_SCREEN,
                       "has-cursor", FALSE,
                       "display", cdk_device_manager_get_display (device_manager),
                       "device-manager", device_manager,
                       NULL);
}

static void
cdk_device_manager_win32_init (CdkDeviceManagerWin32 *device_manager_win32)
{
}

static void
cdk_device_manager_win32_finalize (GObject *object)
{
  CdkDeviceManagerWin32 *device_manager_win32;

  device_manager_win32 = CDK_DEVICE_MANAGER_WIN32 (object);

  g_object_unref (device_manager_win32->core_pointer);
  g_object_unref (device_manager_win32->core_keyboard);

  G_OBJECT_CLASS (cdk_device_manager_win32_parent_class)->finalize (object);
}

#if DEBUG_WINTAB

static void
print_lc(LOGCONTEXT *lc)
{
  g_print ("lcName = %s\n", lc->lcName);
  g_print ("lcOptions =");
  if (lc->lcOptions & CXO_SYSTEM) g_print (" CXO_SYSTEM");
  if (lc->lcOptions & CXO_PEN) g_print (" CXO_PEN");
  if (lc->lcOptions & CXO_MESSAGES) g_print (" CXO_MESSAGES");
  if (lc->lcOptions & CXO_MARGIN) g_print (" CXO_MARGIN");
  if (lc->lcOptions & CXO_MGNINSIDE) g_print (" CXO_MGNINSIDE");
  if (lc->lcOptions & CXO_CSRMESSAGES) g_print (" CXO_CSRMESSAGES");
  g_print ("\n");
  g_print ("lcStatus =");
  if (lc->lcStatus & CXS_DISABLED) g_print (" CXS_DISABLED");
  if (lc->lcStatus & CXS_OBSCURED) g_print (" CXS_OBSCURED");
  if (lc->lcStatus & CXS_ONTOP) g_print (" CXS_ONTOP");
  g_print ("\n");
  g_print ("lcLocks =");
  if (lc->lcLocks & CXL_INSIZE) g_print (" CXL_INSIZE");
  if (lc->lcLocks & CXL_INASPECT) g_print (" CXL_INASPECT");
  if (lc->lcLocks & CXL_SENSITIVITY) g_print (" CXL_SENSITIVITY");
  if (lc->lcLocks & CXL_MARGIN) g_print (" CXL_MARGIN");
  g_print ("\n");
  g_print ("lcMsgBase = %#x, lcDevice = %#x, lcPktRate = %d\n",
	  lc->lcMsgBase, lc->lcDevice, lc->lcPktRate);
  g_print ("lcPktData =");
  if (lc->lcPktData & PK_CONTEXT) g_print (" PK_CONTEXT");
  if (lc->lcPktData & PK_STATUS) g_print (" PK_STATUS");
  if (lc->lcPktData & PK_TIME) g_print (" PK_TIME");
  if (lc->lcPktData & PK_CHANGED) g_print (" PK_CHANGED");
  if (lc->lcPktData & PK_SERIAL_NUMBER) g_print (" PK_SERIAL_NUMBER");
  if (lc->lcPktData & PK_CURSOR) g_print (" PK_CURSOR");
  if (lc->lcPktData & PK_BUTTONS) g_print (" PK_BUTTONS");
  if (lc->lcPktData & PK_X) g_print (" PK_X");
  if (lc->lcPktData & PK_Y) g_print (" PK_Y");
  if (lc->lcPktData & PK_Z) g_print (" PK_Z");
  if (lc->lcPktData & PK_NORMAL_PRESSURE) g_print (" PK_NORMAL_PRESSURE");
  if (lc->lcPktData & PK_TANGENT_PRESSURE) g_print (" PK_TANGENT_PRESSURE");
  if (lc->lcPktData & PK_ORIENTATION) g_print (" PK_ORIENTATION");
  if (lc->lcPktData & PK_ROTATION) g_print (" PK_ROTATION");
  g_print ("\n");
  g_print ("lcPktMode =");
  if (lc->lcPktMode & PK_CONTEXT) g_print (" PK_CONTEXT");
  if (lc->lcPktMode & PK_STATUS) g_print (" PK_STATUS");
  if (lc->lcPktMode & PK_TIME) g_print (" PK_TIME");
  if (lc->lcPktMode & PK_CHANGED) g_print (" PK_CHANGED");
  if (lc->lcPktMode & PK_SERIAL_NUMBER) g_print (" PK_SERIAL_NUMBER");
  if (lc->lcPktMode & PK_CURSOR) g_print (" PK_CURSOR");
  if (lc->lcPktMode & PK_BUTTONS) g_print (" PK_BUTTONS");
  if (lc->lcPktMode & PK_X) g_print (" PK_X");
  if (lc->lcPktMode & PK_Y) g_print (" PK_Y");
  if (lc->lcPktMode & PK_Z) g_print (" PK_Z");
  if (lc->lcPktMode & PK_NORMAL_PRESSURE) g_print (" PK_NORMAL_PRESSURE");
  if (lc->lcPktMode & PK_TANGENT_PRESSURE) g_print (" PK_TANGENT_PRESSURE");
  if (lc->lcPktMode & PK_ORIENTATION) g_print (" PK_ORIENTATION");
  if (lc->lcPktMode & PK_ROTATION) g_print (" PK_ROTATION");
  g_print ("\n");
  g_print ("lcMoveMask =");
  if (lc->lcMoveMask & PK_CONTEXT) g_print (" PK_CONTEXT");
  if (lc->lcMoveMask & PK_STATUS) g_print (" PK_STATUS");
  if (lc->lcMoveMask & PK_TIME) g_print (" PK_TIME");
  if (lc->lcMoveMask & PK_CHANGED) g_print (" PK_CHANGED");
  if (lc->lcMoveMask & PK_SERIAL_NUMBER) g_print (" PK_SERIAL_NUMBER");
  if (lc->lcMoveMask & PK_CURSOR) g_print (" PK_CURSOR");
  if (lc->lcMoveMask & PK_BUTTONS) g_print (" PK_BUTTONS");
  if (lc->lcMoveMask & PK_X) g_print (" PK_X");
  if (lc->lcMoveMask & PK_Y) g_print (" PK_Y");
  if (lc->lcMoveMask & PK_Z) g_print (" PK_Z");
  if (lc->lcMoveMask & PK_NORMAL_PRESSURE) g_print (" PK_NORMAL_PRESSURE");
  if (lc->lcMoveMask & PK_TANGENT_PRESSURE) g_print (" PK_TANGENT_PRESSURE");
  if (lc->lcMoveMask & PK_ORIENTATION) g_print (" PK_ORIENTATION");
  if (lc->lcMoveMask & PK_ROTATION) g_print (" PK_ROTATION");
  g_print ("\n");
  g_print ("lcBtnDnMask = %#x, lcBtnUpMask = %#x\n",
	  (guint) lc->lcBtnDnMask, (guint) lc->lcBtnUpMask);
  g_print ("lcInOrgX = %ld, lcInOrgY = %ld, lcInOrgZ = %ld\n",
	  lc->lcInOrgX, lc->lcInOrgY, lc->lcInOrgZ);
  g_print ("lcInExtX = %ld, lcInExtY = %ld, lcInExtZ = %ld\n",
	  lc->lcInExtX, lc->lcInExtY, lc->lcInExtZ);
  g_print ("lcOutOrgX = %ld, lcOutOrgY = %ld, lcOutOrgZ = %ld\n",
	  lc->lcOutOrgX, lc->lcOutOrgY, lc->lcOutOrgZ);
  g_print ("lcOutExtX = %ld, lcOutExtY = %ld, lcOutExtZ = %ld\n",
	  lc->lcOutExtX, lc->lcOutExtY, lc->lcOutExtZ);
  g_print ("lcSensX = %g, lcSensY = %g, lcSensZ = %g\n",
	  lc->lcSensX / 65536., lc->lcSensY / 65536., lc->lcSensZ / 65536.);
  g_print ("lcSysMode = %d\n", lc->lcSysMode);
  g_print ("lcSysOrgX = %d, lcSysOrgY = %d\n",
	  lc->lcSysOrgX, lc->lcSysOrgY);
  g_print ("lcSysExtX = %d, lcSysExtY = %d\n",
	  lc->lcSysExtX, lc->lcSysExtY);
  g_print ("lcSysSensX = %g, lcSysSensY = %g\n",
	  lc->lcSysSensX / 65536., lc->lcSysSensY / 65536.);
}

static void
print_cursor (int index)
{
  int size;
  int i;
  char *name;
  BOOL active;
  WTPKT wtpkt;
  BYTE buttons;
  BYTE buttonbits;
  char *btnnames;
  BYTE buttonmap[32];
  BYTE sysbtnmap[32];
  BYTE npbutton;
  UINT npbtnmarks[2];
  BYTE tpbutton;
  UINT tpbtnmarks[2];
  DWORD physid;
  UINT mode;
  UINT minpktdata;
  UINT minbuttons;
  UINT capabilities;

  size = (*p_WTInfoA) (WTI_CURSORS + index, CSR_NAME, NULL);
  name = g_malloc (size + 1);
  (*p_WTInfoA) (WTI_CURSORS + index, CSR_NAME, name);
  g_print ("NAME: %s\n", name);
  (*p_WTInfoA) (WTI_CURSORS + index, CSR_ACTIVE, &active);
  g_print ("ACTIVE: %s\n", active ? "YES" : "NO");
  (*p_WTInfoA) (WTI_CURSORS + index, CSR_PKTDATA, &wtpkt);
  g_print ("PKTDATA: %#x:", (guint) wtpkt);
#define BIT(x) if (wtpkt & PK_##x) g_print (" " #x)
  BIT (CONTEXT);
  BIT (STATUS);
  BIT (TIME);
  BIT (CHANGED);
  BIT (SERIAL_NUMBER);
  BIT (BUTTONS);
  BIT (X);
  BIT (Y);
  BIT (Z);
  BIT (NORMAL_PRESSURE);
  BIT (TANGENT_PRESSURE);
  BIT (ORIENTATION);
  BIT (ROTATION);
#undef BIT
  g_print ("\n");
  (*p_WTInfoA) (WTI_CURSORS + index, CSR_BUTTONS, &buttons);
  g_print ("BUTTONS: %d\n", buttons);
  (*p_WTInfoA) (WTI_CURSORS + index, CSR_BUTTONBITS, &buttonbits);
  g_print ("BUTTONBITS: %d\n", buttonbits);
  size = (*p_WTInfoA) (WTI_CURSORS + index, CSR_BTNNAMES, NULL);
  g_print ("BTNNAMES:");
  if (size > 0)
    {
      char *p;

      btnnames = g_malloc (size + 1);
      (*p_WTInfoA) (WTI_CURSORS + index, CSR_BTNNAMES, btnnames);
      p = btnnames;
      while (*p)
        {
          g_print (" %s", p);
          p += strlen (p) + 1;
        }
    }
  g_print ("\n");
  (*p_WTInfoA) (WTI_CURSORS + index, CSR_BUTTONMAP, buttonmap);
  g_print ("BUTTONMAP:");
  for (i = 0; i < buttons; i++)
    g_print (" %d", buttonmap[i]);
  g_print ("\n");
  (*p_WTInfoA) (WTI_CURSORS + index, CSR_SYSBTNMAP, sysbtnmap);
  g_print ("SYSBTNMAP:");
  for (i = 0; i < buttons; i++)
    g_print (" %d", sysbtnmap[i]);
  g_print ("\n");
  (*p_WTInfoA) (WTI_CURSORS + index, CSR_NPBUTTON, &npbutton);
  g_print ("NPBUTTON: %d\n", npbutton);
  (*p_WTInfoA) (WTI_CURSORS + index, CSR_NPBTNMARKS, npbtnmarks);
  g_print ("NPBTNMARKS: %d %d\n", npbtnmarks[0], npbtnmarks[1]);
  size = (*p_WTInfoA) (WTI_CURSORS + index, CSR_NPRESPONSE, NULL);
  g_print ("NPRESPONSE:");
  if (size > 0)
    {
      UINT *npresponse;

      npresponse = g_malloc (size);
      (*p_WTInfoA) (WTI_CURSORS + index, CSR_NPRESPONSE, npresponse);
      for (i = 0; i < size / sizeof (UINT); i++)
        g_print (" %d", npresponse[i]);
    }
  g_print ("\n");
  (*p_WTInfoA) (WTI_CURSORS + index, CSR_TPBUTTON, &tpbutton);
  g_print ("TPBUTTON: %d\n", tpbutton);
  (*p_WTInfoA) (WTI_CURSORS + index, CSR_TPBTNMARKS, tpbtnmarks);
  g_print ("TPBTNMARKS: %d %d\n", tpbtnmarks[0], tpbtnmarks[1]);
  size = (*p_WTInfoA) (WTI_CURSORS + index, CSR_TPRESPONSE, NULL);
  g_print ("TPRESPONSE:");
  if (size > 0)
    {
      UINT *tpresponse;

      tpresponse = g_malloc (size);
      (*p_WTInfoA) (WTI_CURSORS + index, CSR_TPRESPONSE, tpresponse);
      for (i = 0; i < size / sizeof (UINT); i++)
        g_print (" %d", tpresponse[i]);
    }
  g_print ("\n");
  (*p_WTInfoA) (WTI_CURSORS + index, CSR_PHYSID, &physid);
  g_print ("PHYSID: %#x\n", (guint) physid);
  (*p_WTInfoA) (WTI_CURSORS + index, CSR_CAPABILITIES, &capabilities);
  g_print ("CAPABILITIES: %#x:", capabilities);
#define BIT(x) if (capabilities & CRC_##x) g_print (" " #x)
  BIT (MULTIMODE);
  BIT (AGGREGATE);
  BIT (INVERT);
#undef BIT
  g_print ("\n");
  if (capabilities & CRC_MULTIMODE)
    {
      (*p_WTInfoA) (WTI_CURSORS + index, CSR_MODE, &mode);
      g_print ("MODE: %d\n", mode);
    }
  if (capabilities & CRC_AGGREGATE)
    {
      (*p_WTInfoA) (WTI_CURSORS + index, CSR_MINPKTDATA, &minpktdata);
      g_print ("MINPKTDATA: %d\n", minpktdata);
      (*p_WTInfoA) (WTI_CURSORS + index, CSR_MINBUTTONS, &minbuttons);
      g_print ("MINBUTTONS: %d\n", minbuttons);
    }
}
#endif

static void
wintab_init_check (CdkDeviceManagerWin32 *device_manager)
{
  CdkDisplay *display = cdk_device_manager_get_display (CDK_DEVICE_MANAGER (device_manager));
  CdkWindow *root = cdk_screen_get_root_window (cdk_display_get_default_screen (display));
  static gboolean wintab_initialized = FALSE;
  CdkDeviceWintab *device;
  CdkWindowAttr wa;
  WORD specversion;
  HCTX *hctx;
  UINT ndevices, ncursors, ncsrtypes, firstcsr, hardware;
  BOOL active;
  DWORD physid;
  AXIS axis_x, axis_y, axis_npressure, axis_or[3];
  UINT devix, cursorix;
  int i, num_axes = 0;
  wchar_t devname[100], csrname[100];
  gchar *devname_utf8, *csrname_utf8, *device_name;
  BOOL defcontext_done;
  HMODULE wintab32;
  char *wintab32_dll_path;
  char dummy;
  int n, k;

  if (wintab_initialized)
    return;

  wintab_initialized = TRUE;

  wintab_contexts = NULL;

  if (_cdk_input_ignore_wintab)
    return;

  n = GetSystemDirectory (&dummy, 0);

  if (n <= 0)
    return;

  wintab32_dll_path = g_malloc (n + 1 + strlen (WINTAB32_DLL));
  k = GetSystemDirectory (wintab32_dll_path, n);

  if (k == 0 || k > n)
    {
      g_free (wintab32_dll_path);
      return;
    }

  if (!G_IS_DIR_SEPARATOR (wintab32_dll_path[strlen (wintab32_dll_path) -1]))
    strcat (wintab32_dll_path, G_DIR_SEPARATOR_S);
  strcat (wintab32_dll_path, WINTAB32_DLL);

  if ((wintab32 = LoadLibrary (wintab32_dll_path)) == NULL)
    return;

  if ((p_WTInfoA = (t_WTInfoA) GetProcAddress (wintab32, "WTInfoA")) == NULL)
    return;
  if ((p_WTInfoW = (t_WTInfoW) GetProcAddress (wintab32, "WTInfoW")) == NULL)
    return;
  if ((p_WTEnable = (t_WTEnable) GetProcAddress (wintab32, "WTEnable")) == NULL)
    return;
  if ((p_WTOpenA = (t_WTOpenA) GetProcAddress (wintab32, "WTOpenA")) == NULL)
    return;
  if ((p_WTGetA = (t_WTGetA) GetProcAddress (wintab32, "WTGetA")) == NULL)
    return;
  if ((p_WTSetA = (t_WTSetA) GetProcAddress (wintab32, "WTSetA")) == NULL)
    return;
  if ((p_WTOverlap = (t_WTOverlap) GetProcAddress (wintab32, "WTOverlap")) == NULL)
    return;
  if ((p_WTPacket = (t_WTPacket) GetProcAddress (wintab32, "WTPacket")) == NULL)
    return;
  if ((p_WTQueueSizeSet = (t_WTQueueSizeSet) GetProcAddress (wintab32, "WTQueueSizeSet")) == NULL)
    return;

  if (!(*p_WTInfoA) (0, 0, NULL))
    return;

  (*p_WTInfoA) (WTI_INTERFACE, IFC_SPECVERSION, &specversion);
  CDK_NOTE (INPUT, g_print ("Wintab interface version %d.%d\n",
			    HIBYTE (specversion), LOBYTE (specversion)));
  (*p_WTInfoA) (WTI_INTERFACE, IFC_NDEVICES, &ndevices);
  (*p_WTInfoA) (WTI_INTERFACE, IFC_NCURSORS, &ncursors);
#if DEBUG_WINTAB
  CDK_NOTE (INPUT, g_print ("NDEVICES: %d, NCURSORS: %d\n",
			    ndevices, ncursors));
#endif
  /* Create a dummy window to receive wintab events */
  wa.wclass = CDK_INPUT_OUTPUT;
  wa.event_mask = CDK_ALL_EVENTS_MASK;
  wa.width = 2;
  wa.height = 2;
  wa.x = -100;
  wa.y = -100;
  wa.window_type = CDK_WINDOW_TOPLEVEL;
  if ((wintab_window = cdk_window_new (root, &wa, CDK_WA_X | CDK_WA_Y)) == NULL)
    {
      g_warning ("cdk_input_wintab_init: cdk_window_new failed");
      return;
    }
  g_object_ref (wintab_window);

  for (devix = 0; devix < ndevices; devix++)
    {
      LOGCONTEXT lc;

      /* We open the Wintab device (hmm, what if there are several, or
       * can there even be several, probably not?) as a system
       * pointing device, i.e. it controls the normal Windows
       * cursor. This seems much more natural.
       */

      (*p_WTInfoW) (WTI_DEVICES + devix, DVC_NAME, devname);
      devname_utf8 = g_utf16_to_utf8 (devname, -1, NULL, NULL, NULL);
#ifdef DEBUG_WINTAB
      CDK_NOTE (INPUT, (g_print("Device %u: %s\n", devix, devname_utf8)));
#endif
      (*p_WTInfoA) (WTI_DEVICES + devix, DVC_NCSRTYPES, &ncsrtypes);
      (*p_WTInfoA) (WTI_DEVICES + devix, DVC_FIRSTCSR, &firstcsr);
      (*p_WTInfoA) (WTI_DEVICES + devix, DVC_HARDWARE, &hardware);
      (*p_WTInfoA) (WTI_DEVICES + devix, DVC_X, &axis_x);
      (*p_WTInfoA) (WTI_DEVICES + devix, DVC_Y, &axis_y);
      (*p_WTInfoA) (WTI_DEVICES + devix, DVC_NPRESSURE, &axis_npressure);
      (*p_WTInfoA) (WTI_DEVICES + devix, DVC_ORIENTATION, axis_or);

      defcontext_done = FALSE;
      if (HIBYTE (specversion) > 1 || LOBYTE (specversion) >= 1)
        {
          /* Try to get device-specific default context */
          /* Some drivers, e.g. Aiptek, don't provide this info */
          if ((*p_WTInfoA) (WTI_DSCTXS + devix, 0, &lc) > 0)
            defcontext_done = TRUE;
#if DEBUG_WINTAB
          if (defcontext_done)
            CDK_NOTE (INPUT, (g_print("Using device-specific default context\n")));
          else
            CDK_NOTE (INPUT, (g_print("Note: Driver did not provide device specific default context info despite claiming to support version 1.1\n")));
#endif
        }

      if (!defcontext_done)
        (*p_WTInfoA) (WTI_DEFSYSCTX, 0, &lc);
#if DEBUG_WINTAB
      CDK_NOTE (INPUT, (g_print("Default context:\n"), print_lc(&lc)));
#endif
      lc.lcOptions |= CXO_MESSAGES | CXO_CSRMESSAGES;
      lc.lcStatus = 0;
      lc.lcMsgBase = WT_DEFBASE;
      lc.lcPktRate = 0;
      lc.lcPktData = PACKETDATA;
      lc.lcPktMode = PACKETMODE;
      lc.lcMoveMask = PACKETDATA;
      lc.lcBtnUpMask = lc.lcBtnDnMask = ~0;
      lc.lcOutOrgX = axis_x.axMin;
      lc.lcOutOrgY = axis_y.axMin;
      lc.lcOutExtX = axis_x.axMax - axis_x.axMin + 1;
      lc.lcOutExtY = axis_y.axMax - axis_y.axMin + 1;
      lc.lcOutExtY = -lc.lcOutExtY; /* We want Y growing downward */
#if DEBUG_WINTAB
      CDK_NOTE (INPUT, (g_print("context for device %u:\n", devix),
			print_lc(&lc)));
#endif
      hctx = g_new (HCTX, 1);
      if ((*hctx = (*p_WTOpenA) (CDK_WINDOW_HWND (wintab_window), &lc, TRUE)) == NULL)
        {
          g_warning ("cdk_input_wintab_init: WTOpen failed");
          return;
        }
      CDK_NOTE (INPUT, g_print ("opened Wintab device %u %p\n",
                                devix, *hctx));

      wintab_contexts = g_list_append (wintab_contexts, hctx);
#if 0
      (*p_WTEnable) (*hctx, TRUE);
#endif
      (*p_WTOverlap) (*hctx, TRUE);

#if DEBUG_WINTAB
      CDK_NOTE (INPUT, (g_print("context for device %u after WTOpen:\n", devix),
			print_lc(&lc)));
#endif
      /* Increase packet queue size to reduce the risk of lost packets.
       * According to the specs, if the function fails we must try again
       * with a smaller queue size.
       */
      CDK_NOTE (INPUT, g_print("Attempting to increase queue size\n"));
      for (i = 128; i >= 1; i >>= 1)
        {
          if ((*p_WTQueueSizeSet) (*hctx, i))
            {
              CDK_NOTE (INPUT, g_print("Queue size set to %d\n", i));
              break;
            }
        }
      if (!i)
        CDK_NOTE (INPUT, g_print("Whoops, no queue size could be set\n"));
      for (cursorix = firstcsr; cursorix < firstcsr + ncsrtypes; cursorix++)
        {
#ifdef DEBUG_WINTAB
          CDK_NOTE (INPUT, (g_print("Cursor %u:\n", cursorix), print_cursor (cursorix)));
#endif
          active = FALSE;
          (*p_WTInfoA) (WTI_CURSORS + cursorix, CSR_ACTIVE, &active);
          if (!active)
            continue;

          /* Wacom tablets seem to report cursors corresponding to
           * nonexistent pens or pucks. At least my ArtPad II reports
           * six cursors: a puck, pressure stylus and eraser stylus,
           * and then the same three again. I only have a
           * pressure-sensitive pen. The puck instances, and the
           * second instances of the styluses report physid zero. So
           * at least for Wacom, skip cursors with physid zero.
           */
          (*p_WTInfoA) (WTI_CURSORS + cursorix, CSR_PHYSID, &physid);
          if (wcscmp (devname, L"WACOM Tablet") == 0 && physid == 0)
            continue;

          (*p_WTInfoW) (WTI_CURSORS + cursorix, CSR_NAME, csrname);
          csrname_utf8 = g_utf16_to_utf8 (csrname, -1, NULL, NULL, NULL);
          device_name = g_strconcat (devname_utf8, " ", csrname_utf8, NULL);

          device = g_object_new (CDK_TYPE_DEVICE_WINTAB,
                                 "name", device_name,
                                 "type", CDK_DEVICE_TYPE_FLOATING,
                                 "input-source", CDK_SOURCE_PEN,
                                 "input-mode", CDK_MODE_SCREEN,
                                 "has-cursor", lc.lcOptions & CXO_SYSTEM,
                                 "display", display,
                                 "device-manager", device_manager,
                                 NULL);

	  device->sends_core = lc.lcOptions & CXO_SYSTEM;
	  if (device->sends_core)
	    {
	      _cdk_device_set_associated_device (device_manager->system_pointer, CDK_DEVICE (device));
	      _cdk_device_add_slave (device_manager->core_pointer, CDK_DEVICE (device));
	    }

          g_free (csrname_utf8);

          device->hctx = *hctx;
          device->cursor = cursorix;
          (*p_WTInfoA) (WTI_CURSORS + cursorix, CSR_PKTDATA, &device->pktdata);

          if (device->pktdata & PK_X)
            {
              _cdk_device_add_axis (CDK_DEVICE (device),
                                    CDK_NONE,
                                    CDK_AXIS_X,
                                    axis_x.axMin,
                                    axis_x.axMax,
                                    axis_x.axResolution / 65535);
              num_axes++;
            }

          if (device->pktdata & PK_Y)
            {
              _cdk_device_add_axis (CDK_DEVICE (device),
                                    CDK_NONE,
                                    CDK_AXIS_Y,
                                    axis_y.axMin,
                                    axis_y.axMax,
                                    axis_y.axResolution / 65535);
              num_axes++;
            }


          if (device->pktdata & PK_NORMAL_PRESSURE)
            {
              _cdk_device_add_axis (CDK_DEVICE (device),
                                    CDK_NONE,
                                    CDK_AXIS_PRESSURE,
                                    axis_npressure.axMin,
                                    axis_npressure.axMax,
                                    axis_npressure.axResolution / 65535);
              num_axes++;
            }

          if (device->pktdata & PK_ORIENTATION)
            {
              device->orientation_axes[0] = axis_or[0];
              device->orientation_axes[1] = axis_or[1];

              /* Wintab gives us azimuth and altitude, which
               * we convert to x and y tilt in the -1000..1000 range
               */
              _cdk_device_add_axis (CDK_DEVICE (device),
                                    CDK_NONE,
                                    CDK_AXIS_XTILT,
                                    -1000,
                                    1000,
                                    1000);

              _cdk_device_add_axis (CDK_DEVICE (device),
                                    CDK_NONE,
                                    CDK_AXIS_YTILT,
                                    -1000,
                                    1000,
                                    1000);
              num_axes += 2;
            }

          device->last_axis_data = g_new (gint, num_axes);

          CDK_NOTE (INPUT, g_print ("device: (%u) %s axes: %d\n",
                                    cursorix,
                                    device_name,
                                    num_axes));

#if 0
          for (i = 0; i < cdkdev->info.num_axes; i++)
            CDK_NOTE (INPUT, g_print ("... axis %d: %d--%d@%d\n",
                                      i,
                                      cdkdev->axes[i].min_value,
                                      cdkdev->axes[i].max_value,
                                      cdkdev->axes[i].resolution));
#endif

          device_manager->wintab_devices = g_list_append (device_manager->wintab_devices,
                                                          device);

          g_free (device_name);
        }

      g_free (devname_utf8);
    }
}

/* Only initialize Wintab after the default display is set for
 * the first time. WTOpenA() executes code beyond our control,
 * and it can cause messages to be sent to the application even
 * before a window is opened. CDK has to be in a fit state to
 * handle them when they come.
 *
 * https://bugzilla.gnome.org/show_bug.cgi?id=774379
 */
static void
wintab_default_display_notify_cb (CdkDisplayManager *display_manager)
{
  CdkDeviceManagerWin32 *device_manager = NULL;
  CdkDisplay *display = cdk_display_get_default();

  if (default_display_opened)
    return;

  g_assert (display != NULL);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  device_manager = CDK_DEVICE_MANAGER_WIN32 (cdk_display_get_device_manager (display));
G_GNUC_END_IGNORE_DEPRECATIONS;
  g_assert (display_manager != NULL);

  default_display_opened = TRUE;
  CDK_NOTE (INPUT, g_print ("wintab init: doing delayed initialization\n"));
  wintab_init_check (device_manager);
}

static void
cdk_device_manager_win32_constructed (GObject *object)
{
  CdkDeviceManagerWin32 *device_manager;
  CdkSeat *seat;
  CdkDisplayManager *display_manager = NULL;
  CdkDisplay *default_display = NULL;

  device_manager = CDK_DEVICE_MANAGER_WIN32 (object);
  device_manager->core_pointer =
    create_pointer (CDK_DEVICE_MANAGER (device_manager),
		    CDK_TYPE_DEVICE_VIRTUAL,
		    "Virtual Core Pointer",
		    CDK_DEVICE_TYPE_MASTER);
  device_manager->system_pointer =
    create_pointer (CDK_DEVICE_MANAGER (device_manager),
		    CDK_TYPE_DEVICE_WIN32,
		    "System Aggregated Pointer",
		    CDK_DEVICE_TYPE_SLAVE);
  _cdk_device_virtual_set_active (device_manager->core_pointer,
				  device_manager->system_pointer);
  _cdk_device_set_associated_device (device_manager->system_pointer, device_manager->core_pointer);
  _cdk_device_add_slave (device_manager->core_pointer, device_manager->system_pointer);

  device_manager->core_keyboard =
    create_keyboard (CDK_DEVICE_MANAGER (device_manager),
		     CDK_TYPE_DEVICE_VIRTUAL,
		     "Virtual Core Keyboard",
		     CDK_DEVICE_TYPE_MASTER);
  device_manager->system_keyboard =
    create_keyboard (CDK_DEVICE_MANAGER (device_manager),
		    CDK_TYPE_DEVICE_WIN32,
		     "System Aggregated Keyboard",
		     CDK_DEVICE_TYPE_SLAVE);
  _cdk_device_virtual_set_active (device_manager->core_keyboard,
				  device_manager->system_keyboard);
  _cdk_device_set_associated_device (device_manager->system_keyboard, device_manager->core_keyboard);
  _cdk_device_add_slave (device_manager->core_keyboard, device_manager->system_keyboard);

  _cdk_device_set_associated_device (device_manager->core_pointer, device_manager->core_keyboard);
  _cdk_device_set_associated_device (device_manager->core_keyboard, device_manager->core_pointer);

  seat = cdk_seat_default_new_for_master_pair (device_manager->core_pointer,
                                               device_manager->core_keyboard);
  cdk_display_add_seat (cdk_device_manager_get_display (CDK_DEVICE_MANAGER (object)), seat);
  cdk_seat_default_add_slave (CDK_SEAT_DEFAULT (seat), device_manager->system_pointer);
  cdk_seat_default_add_slave (CDK_SEAT_DEFAULT (seat), device_manager->system_keyboard);
  g_object_unref (seat);

  /* Only call Wintab init stuff after the default display
   * is globally known and accessible through the display manager
   * singleton. Approach lifted from ctkmodules.c.
   */
  display_manager = cdk_display_manager_get();
  g_assert (display_manager != NULL);
  default_display = cdk_display_manager_get_default_display (display_manager);
  g_assert (default_display == NULL);

  g_signal_connect (display_manager, "notify::default-display",
                    G_CALLBACK (wintab_default_display_notify_cb),
                    NULL);
}

static GList *
cdk_device_manager_win32_list_devices (CdkDeviceManager *device_manager,
                                       CdkDeviceType     type)
{
  CdkDeviceManagerWin32 *device_manager_win32;
  GList *devices = NULL, *l;

  device_manager_win32 = (CdkDeviceManagerWin32 *) device_manager;

  if (type == CDK_DEVICE_TYPE_MASTER)
    {
      devices = g_list_prepend (devices, device_manager_win32->core_keyboard);
      devices = g_list_prepend (devices, device_manager_win32->core_pointer);
    }
  else
    {
      if (type == CDK_DEVICE_TYPE_SLAVE)
	{
	  devices = g_list_prepend (devices, device_manager_win32->system_keyboard);
	  devices = g_list_prepend (devices, device_manager_win32->system_pointer);
	}

      for (l = device_manager_win32->wintab_devices; l != NULL; l = l->next)
	{
	  CdkDevice *device = l->data;

	  if (cdk_device_get_device_type (device) == type)
	    devices = g_list_prepend (devices, device);
	}
    }

  return g_list_reverse (devices);
}

static CdkDevice *
cdk_device_manager_win32_get_client_pointer (CdkDeviceManager *device_manager)
{
  CdkDeviceManagerWin32 *device_manager_win32;

  device_manager_win32 = (CdkDeviceManagerWin32 *) device_manager;
  return device_manager_win32->core_pointer;
}

static void
cdk_device_manager_win32_class_init (CdkDeviceManagerWin32Class *klass)
{
  CdkDeviceManagerClass *device_manager_class = CDK_DEVICE_MANAGER_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = cdk_device_manager_win32_finalize;
  object_class->constructed = cdk_device_manager_win32_constructed;
  device_manager_class->list_devices = cdk_device_manager_win32_list_devices;
  device_manager_class->get_client_pointer = cdk_device_manager_win32_get_client_pointer;
}

void
_cdk_input_set_tablet_active (void)
{
  GList *tmp_list;

  /* Bring the contexts to the top of the overlap order when one of the
   * application's windows is activated */

  if (!wintab_contexts)
    return; /* No tablet devices found, or Wintab not initialized yet */

  CDK_NOTE (INPUT, g_print ("_cdk_input_set_tablet_active: "
                            "Bringing Wintab contexts to the top of the overlap order\n"));

  tmp_list = wintab_contexts;

  while (tmp_list)
    {
      HCTX *hctx;

      hctx = (HCTX *) (tmp_list->data);
      (*p_WTOverlap) (*hctx, TRUE);
      tmp_list = tmp_list->next;
    }
}

static void
decode_tilt (gint   *axis_data,
             AXIS   *axes,
             PACKET *packet)
{
  double az, el;

  g_return_if_fail (axis_data != NULL);

  /* The wintab driver for the Wacom ArtPad II reports
   * PK_ORIENTATION in CSR_PKTDATA, but the tablet doesn't
   * actually sense tilt. Catch this by noticing that the
   * orientation axis's azimuth resolution is zero.
   *
   * The same is true of the Huion H610PRO, but in this case
   * it's the altitude resolution that's zero. CdkEvents with
   * sensible tilts will need both, so only add the CDK tilt axes
   * if both wintab axes are going to be well-behaved in use.
   */
  if ((axes == NULL) ||
      (axes[0].axResolution == 0) ||
      (axes[1].axResolution == 0))
    {
      axis_data[0] = 0;
      axis_data[1] = 0;
      return;
    }

  /*
   * Tested with a Wacom Intuos 5 touch M (PTH-650) + Wacom drivers 6.3.18-5.
   * Wintab's reference angle leads cdk's by 90 degrees.
   */
  az = TWOPI * packet->pkOrientation.orAzimuth /
    (axes[0].axResolution / 65536.);
  az -= G_PI / 2;
  el = TWOPI * packet->pkOrientation.orAltitude /
    (axes[1].axResolution / 65536.);

  /* X tilt */
  axis_data[0] = cos (az) * cos (el) * 1000;
  /* Y tilt */
  axis_data[1] = sin (az) * cos (el) * 1000;
}

/*
 * Get the currently active keyboard modifiers (ignoring the mouse buttons)
 * We could use cdk_window_get_pointer but that function does a lot of other
 * expensive things besides getting the modifiers. This code is somewhat based
 * on build_pointer_event_state from cdkevents-win32.c
 */
static guint
get_modifier_key_state (void)
{
  guint state;

  state = 0;
  /* High-order bit is up/down, low order bit is toggled/untoggled */
  if (GetKeyState (VK_CONTROL) < 0)
    state |= CDK_CONTROL_MASK;
  if (GetKeyState (VK_SHIFT) < 0)
    state |= CDK_SHIFT_MASK;
  if (GetKeyState (VK_MENU) < 0)
    state |= CDK_MOD1_MASK;
  if (GetKeyState (VK_CAPITAL) & 0x1)
    state |= CDK_LOCK_MASK;

  return state;
}

static CdkDeviceWintab *
cdk_device_manager_find_wintab_device (CdkDeviceManagerWin32 *device_manager,
                                       HCTX                   hctx,
                                       UINT                   cursor)
{
  GList *tmp_list;

  for (tmp_list = device_manager->wintab_devices; tmp_list != NULL; tmp_list = tmp_list->next)
    {
      CdkDeviceWintab *device;

      device = tmp_list->data;

      if (device->hctx == hctx &&
          device->cursor == cursor)
        return device;
    }

  return NULL;
}

gboolean
cdk_input_other_event (CdkDisplay *display,
                       CdkEvent   *event,
                       MSG        *msg,
                       CdkWindow  *window)
{
  CdkDeviceManagerWin32 *device_manager;
  CdkDeviceWintab *source_device = NULL;
  CdkDeviceGrabInfo *last_grab;
  CdkEventMask masktest;
  guint key_state;
  POINT pt;

  PACKET packet;
  gint root_x, root_y;
  gint num_axes;
  gint x, y;
  guint translated_buttons, button_diff, button_mask;
  /* Translation from tablet button state to CDK button state for
   * buttons 1-3 - swap button 2 and 3.
   */
  static guint button_map[8] = {0, 1, 4, 5, 2, 3, 6, 7};

  if (event->any.window != wintab_window)
    {
      g_warning ("cdk_input_other_event: not wintab_window?");
      return FALSE;
    }

G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  device_manager = CDK_DEVICE_MANAGER_WIN32 (cdk_display_get_device_manager (display));
G_GNUC_END_IGNORE_DEPRECATIONS;
  window = cdk_device_get_window_at_position (device_manager->core_pointer, &x, &y);
  if (window == NULL)
    window = cdk_get_default_root_window ();

  g_object_ref (window);

  CDK_NOTE (EVENTS_OR_INPUT,
	    g_print ("cdk_input_other_event: window=%p %+d%+d\n",
               CDK_WINDOW_HWND (window), x, y));

  if (msg->message == WT_PACKET || msg->message == WT_CSRCHANGE)
    {
      if (!(*p_WTPacket) ((HCTX) msg->lParam, msg->wParam, &packet))
        return FALSE;
    }

  switch (msg->message)
    {
    case WT_PACKET:
      source_device = cdk_device_manager_find_wintab_device (device_manager,
							     (HCTX) msg->lParam,
							     packet.pkCursor);

      /* Check this first, as we get WT_PROXIMITY for disabled devices too */
      if (device_manager->dev_entered_proximity > 0)
	{
	  /* This is the same code as in WT_CSRCHANGE. Some drivers send
	   * WT_CSRCHANGE after each WT_PROXIMITY with LOWORD(lParam) != 0,
	   * this code is for those that don't.
	   */
	  device_manager->dev_entered_proximity -= 1;

	  if (source_device != NULL &&
	      source_device->sends_core &&
	      cdk_device_get_mode (CDK_DEVICE (source_device)) != CDK_MODE_DISABLED)
	    {
	      _cdk_device_virtual_set_active (device_manager->core_pointer,
					      CDK_DEVICE (source_device));
	      _cdk_input_ignore_core += 1;
	    }
	}
      else if (source_device != NULL &&
	       source_device->sends_core &&
	       cdk_device_get_mode (CDK_DEVICE (source_device)) != CDK_MODE_DISABLED &&
               _cdk_input_ignore_core == 0)
        {
          /* A fallback for cases when two devices (disabled and enabled)
           * were in proximity simultaneously.
           * In this case the removal of a disabled device would also
           * make the system pointer active, as we don't know which
           * device was removed and assume it was the enabled one.
           * If we are still getting packets for the enabled device,
           * it means that the device that was removed was the disabled
           * device, so we must make the enabled device active again and
           * start ignoring the core pointer events. In practice this means that
           * removing a disabled device while an enabled device is still
           * in proximity might briefly make the core pointer active/visible.
           */
	  _cdk_device_virtual_set_active (device_manager->core_pointer,
					  CDK_DEVICE (source_device));
	  _cdk_input_ignore_core += 1;
        }

      if (source_device == NULL ||
	  cdk_device_get_mode (CDK_DEVICE (source_device)) == CDK_MODE_DISABLED)
	return FALSE;

      /* Don't produce any button or motion events while a window is being
       * moved or resized, see bug #151090.
       */
      if (_modal_operation_in_progress & CDK_WIN32_MODAL_OP_SIZEMOVE_MASK)
        {
          CDK_NOTE (EVENTS_OR_INPUT, g_print ("... ignored when moving/sizing\n"));
          return FALSE;
        }

      last_grab = _cdk_display_get_last_device_grab (display, CDK_DEVICE (source_device));

      if (last_grab && last_grab->window)
        {
          g_object_unref (window);

          window = g_object_ref (last_grab->window);
        }

      if (window == cdk_get_default_root_window ())
        {
          CDK_NOTE (EVENTS_OR_INPUT, g_print ("... is root\n"));
          return FALSE;
        }

      num_axes = 0;
      if (source_device->pktdata & PK_X)
        source_device->last_axis_data[num_axes++] = packet.pkX;
      if (source_device->pktdata & PK_Y)
        source_device->last_axis_data[num_axes++] = packet.pkY;
      if (source_device->pktdata & PK_NORMAL_PRESSURE)
        source_device->last_axis_data[num_axes++] = packet.pkNormalPressure;
      if (source_device->pktdata & PK_ORIENTATION)
        {
          decode_tilt (source_device->last_axis_data + num_axes,
                       source_device->orientation_axes, &packet);
          num_axes += 2;
        }

      translated_buttons = button_map[packet.pkButtons & 0x07] | (packet.pkButtons & ~0x07);

      if (translated_buttons != source_device->button_state)
        {
          /* At least one button has changed state so produce a button event
           * If more than one button has changed state (unlikely),
           * just care about the first and act on the next the next time
           * we get a packet
           */
          button_diff = translated_buttons ^ source_device->button_state;

          /* Cdk buttons are numbered 1.. */
          event->button.button = 1;

          for (button_mask = 1; button_mask != 0x80000000;
               button_mask <<= 1, event->button.button++)
            {
              if (button_diff & button_mask)
                {
                  /* Found a button that has changed state */
                  break;
                }
            }

          if (!(translated_buttons & button_mask))
            {
              event->any.type = CDK_BUTTON_RELEASE;
              masktest = CDK_BUTTON_RELEASE_MASK;
            }
          else
            {
              event->any.type = CDK_BUTTON_PRESS;
              masktest = CDK_BUTTON_PRESS_MASK;
            }
          source_device->button_state ^= button_mask;
        }
      else
        {
          event->any.type = CDK_MOTION_NOTIFY;
          masktest = CDK_POINTER_MOTION_MASK;
          if (source_device->button_state & (1 << 0))
            masktest |= CDK_BUTTON_MOTION_MASK | CDK_BUTTON1_MOTION_MASK;
          if (source_device->button_state & (1 << 1))
            masktest |= CDK_BUTTON_MOTION_MASK | CDK_BUTTON2_MOTION_MASK;
          if (source_device->button_state & (1 << 2))
            masktest |= CDK_BUTTON_MOTION_MASK | CDK_BUTTON3_MOTION_MASK;
        }

      /* Now we can check if the window wants the event, and
       * propagate if necessary.
       */
      while ((cdk_window_get_device_events (window, CDK_DEVICE (source_device)) & masktest) == 0 &&
	     (cdk_device_get_device_type (CDK_DEVICE (source_device)) == CDK_DEVICE_TYPE_SLAVE &&
	      (cdk_window_get_events (window) & masktest) == 0))
        {
          CdkWindowImplWin32 *impl;

          CDK_NOTE (EVENTS_OR_INPUT, g_print ("... not selected\n"));

          if (window->parent == cdk_get_default_root_window () || window->parent == NULL)
            return FALSE;

          impl = CDK_WINDOW_IMPL_WIN32 (window->impl);
          pt.x = x * impl->window_scale;
          pt.y = y * impl->window_scale;
          ClientToScreen (CDK_WINDOW_HWND (window), &pt);
          g_object_unref (window);
          window = window->parent;
          impl = CDK_WINDOW_IMPL_WIN32 (window->impl);
          g_object_ref (window);
          ScreenToClient (CDK_WINDOW_HWND (window), &pt);
          x = pt.x / impl->window_scale;
          y = pt.y / impl->window_scale;
          CDK_NOTE (EVENTS_OR_INPUT, g_print ("... propagating to %p %+d%+d\n",
                                              CDK_WINDOW_HWND (window), x, y));
        }

      event->any.window = window;
      key_state = get_modifier_key_state ();
      if (event->any.type == CDK_BUTTON_PRESS ||
          event->any.type == CDK_BUTTON_RELEASE)
        {
          event->button.time = _cdk_win32_get_next_tick (msg->time);
	  if (source_device->sends_core)
	    cdk_event_set_device (event, device_manager->core_pointer);
          cdk_event_set_source_device (event, CDK_DEVICE (source_device));
          cdk_event_set_seat (event, cdk_device_get_seat (device_manager->core_pointer));

          event->button.axes = g_new (gdouble, num_axes);
	  cdk_window_get_origin (window, &root_x, &root_y);

          _cdk_device_wintab_translate_axes (source_device,
                                             window,
                                             event->button.axes,
                                             &event->button.x,
                                             &event->button.y);

          event->button.x_root = event->button.x + root_x;
          event->button.y_root = event->button.y + root_y;

          event->button.state =
            key_state | ((source_device->button_state << 8)
                         & (CDK_BUTTON1_MASK | CDK_BUTTON2_MASK
                            | CDK_BUTTON3_MASK | CDK_BUTTON4_MASK
                            | CDK_BUTTON5_MASK));

          CDK_NOTE (EVENTS_OR_INPUT,
                    g_print ("WINTAB button %s:%d %g,%g\n",
                             (event->button.type == CDK_BUTTON_PRESS ?
                              "press" : "release"),
                             event->button.button,
                             event->button.x, event->button.y));
        }
      else
        {
          event->motion.time = _cdk_win32_get_next_tick (msg->time);
          event->motion.is_hint = FALSE;
          cdk_event_set_device (event, device_manager->core_pointer);
          cdk_event_set_source_device (event, CDK_DEVICE (source_device));
          cdk_event_set_seat (event, cdk_device_get_seat (device_manager->core_pointer));

          event->motion.axes = g_new (gdouble, num_axes);
	  cdk_window_get_origin (window, &root_x, &root_y);

          _cdk_device_wintab_translate_axes (source_device,
                                             window,
                                             event->motion.axes,
                                             &event->motion.x,
                                             &event->motion.y);

          event->motion.x_root = event->motion.x + root_x;
          event->motion.y_root = event->motion.y + root_y;

          event->motion.state =
            key_state | ((source_device->button_state << 8)
                         & (CDK_BUTTON1_MASK | CDK_BUTTON2_MASK
                            | CDK_BUTTON3_MASK | CDK_BUTTON4_MASK
                            | CDK_BUTTON5_MASK));

          CDK_NOTE (EVENTS_OR_INPUT,
                    g_print ("WINTAB motion: %g,%g\n",
                             event->motion.x, event->motion.y));
        }
      return TRUE;

    case WT_CSRCHANGE:
      if (device_manager->dev_entered_proximity > 0)
	device_manager->dev_entered_proximity -= 1;

      if ((source_device = cdk_device_manager_find_wintab_device (device_manager,
								  (HCTX) msg->lParam,
								  packet.pkCursor)) == NULL)
	return FALSE;

      if (source_device->sends_core &&
	  cdk_device_get_mode (CDK_DEVICE (source_device)) != CDK_MODE_DISABLED)
	{
	  _cdk_device_virtual_set_active (device_manager->core_pointer,
					  CDK_DEVICE (source_device));
	  _cdk_input_ignore_core += 1;
	}

      return FALSE;

    case WT_PROXIMITY:
      if (LOWORD (msg->lParam) == 0)
        {
          if (_cdk_input_ignore_core > 0)
            {
	      _cdk_input_ignore_core -= 1;

	      if (_cdk_input_ignore_core == 0)
		_cdk_device_virtual_set_active (device_manager->core_pointer,
						device_manager->system_pointer);
	    }
	}
      else
	{
	  device_manager->dev_entered_proximity += 1;
	}

      return FALSE;
    }

  return FALSE;
}
