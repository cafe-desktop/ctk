 /*
 * cdkscreen-x11.c
 *
 * Copyright 2001 Sun Microsystems Inc.
 *
 * Erwann Chenede <erwann.chenede@sun.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "cdkscreen-x11.h"
#include "cdkdisplay-x11.h"
#include "cdkprivate-x11.h"
#include "xsettings-client.h"
#include "cdkmonitor-x11.h"

#include <glib.h>

#include <stdlib.h>
#include <string.h>

#include <X11/Xatom.h>

#ifdef HAVE_XFREE_XINERAMA
#include <X11/extensions/Xinerama.h>
#endif

#ifdef HAVE_RANDR
#include <X11/extensions/Xrandr.h>
#endif

#ifdef HAVE_XFIXES
#include <X11/extensions/Xfixes.h>
#endif

static void         cdk_x11_screen_dispose     (GObject		  *object);
static void         cdk_x11_screen_finalize    (GObject		  *object);
static void	    init_randr_support	       (CdkScreen	  *screen);
static void         process_monitors_change    (CdkScreen         *screen);

enum
{
  WINDOW_MANAGER_CHANGED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (CdkX11Screen, cdk_x11_screen, CDK_TYPE_SCREEN)

typedef struct _NetWmSupportedAtoms NetWmSupportedAtoms;

struct _NetWmSupportedAtoms
{
  Atom *atoms;
  gulong n_atoms;
};

static void
cdk_x11_screen_init (CdkX11Screen *screen G_GNUC_UNUSED)
{
}

static CdkDisplay *
cdk_x11_screen_get_display (CdkScreen *screen)
{
  return CDK_X11_SCREEN (screen)->display;
}

gint
cdk_x11_screen_get_width (CdkScreen *screen)
{
  return CDK_X11_SCREEN (screen)->width;
}

gint
cdk_x11_screen_get_height (CdkScreen *screen)
{
  return CDK_X11_SCREEN (screen)->height;
}

static gint
cdk_x11_screen_get_width_mm (CdkScreen *screen)
{
  return WidthMMOfScreen (CDK_X11_SCREEN (screen)->xscreen);
}

static gint
cdk_x11_screen_get_height_mm (CdkScreen *screen)
{
  return HeightMMOfScreen (CDK_X11_SCREEN (screen)->xscreen);
}

gint
cdk_x11_screen_get_number (CdkScreen *screen)
{
  return CDK_X11_SCREEN (screen)->screen_num;
}

static CdkWindow *
cdk_x11_screen_get_root_window (CdkScreen *screen)
{
  return CDK_X11_SCREEN (screen)->root_window;
}

static void
cdk_x11_screen_dispose (GObject *object)
{
  CdkX11Screen *x11_screen = CDK_X11_SCREEN (object);
  int i;

  for (i = 0; i < 32; ++i)
    {
      if (x11_screen->subwindow_gcs[i])
        {
          XFreeGC (x11_screen->xdisplay, x11_screen->subwindow_gcs[i]);
          x11_screen->subwindow_gcs[i] = 0;
        }
    }

  _cdk_x11_xsettings_finish (x11_screen);

  if (x11_screen->root_window)
    _cdk_window_destroy (x11_screen->root_window, TRUE);

  for (i = 0; i < x11_screen->nvisuals; i++)
    g_object_run_dispose (G_OBJECT (x11_screen->visuals[i]));

  G_OBJECT_CLASS (cdk_x11_screen_parent_class)->dispose (object);

  x11_screen->xdisplay = NULL;
  x11_screen->xscreen = NULL;
  x11_screen->screen_num = -1;
  x11_screen->xroot_window = None;
  x11_screen->wmspec_check_window = None;
}

static void
cdk_x11_screen_finalize (GObject *object)
{
  CdkX11Screen *x11_screen = CDK_X11_SCREEN (object);
  gint          i;

  if (x11_screen->root_window)
    g_object_unref (x11_screen->root_window);

  /* Visual Part */
  for (i = 0; i < x11_screen->nvisuals; i++)
    g_object_unref (x11_screen->visuals[i]);
  g_free (x11_screen->visuals);
  g_hash_table_destroy (x11_screen->visual_hash);

  g_free (x11_screen->window_manager_name);

  G_OBJECT_CLASS (cdk_x11_screen_parent_class)->finalize (object);
}

/**
 * cdk_x11_screen_get_monitor_output:
 * @screen: (type CdkX11Screen): a #CdkScreen
 * @monitor_num: number of the monitor, between 0 and cdk_screen_get_n_monitors (screen)
 *
 * Gets the XID of the specified output/monitor.
 * If the X server does not support version 1.2 of the RANDR
 * extension, 0 is returned.
 *
 * Returns: the XID of the monitor
 *
 * Since: 2.14
 */
XID
cdk_x11_screen_get_monitor_output (CdkScreen *screen,
                                   gint       monitor_num)
{
  CdkX11Screen *x11_screen = CDK_X11_SCREEN (screen);
  CdkX11Display *x11_display = CDK_X11_DISPLAY (x11_screen->display);
  CdkX11Monitor *monitor;

  g_return_val_if_fail (CDK_IS_SCREEN (screen), None);
  g_return_val_if_fail (monitor_num >= 0, None);
  g_return_val_if_fail (monitor_num < x11_display->monitors->len, None);

  monitor = x11_display->monitors->pdata[monitor_num];
  return monitor->output;
}

static int
get_current_desktop (CdkScreen *screen)
{
  Display *display;
  Window win;
  Atom current_desktop, type;
  int format;
  unsigned long n_items, bytes_after;
  unsigned char *data_return = NULL;
  int workspace = 0;

  if (!cdk_x11_screen_supports_net_wm_hint (screen,
                                            cdk_atom_intern_static_string ("_NET_CURRENT_DESKTOP")))
    return workspace;

  display = CDK_DISPLAY_XDISPLAY (cdk_screen_get_display (screen));
  win = XRootWindow (display, CDK_SCREEN_XNUMBER (screen));

  current_desktop = XInternAtom (display, "_NET_CURRENT_DESKTOP", True);

  XGetWindowProperty (display,
                      win,
                      current_desktop,
                      0, G_MAXLONG,
                      False, XA_CARDINAL,
                      &type, &format, &n_items, &bytes_after,
                      &data_return);

  if (type == XA_CARDINAL && format == 32 && n_items > 0)
    workspace = ((long *) data_return)[0];

  if (data_return)
    XFree (data_return);

  return workspace;
}

gboolean
_cdk_x11_screen_get_monitor_work_area (CdkScreen    *screen,
                                       CdkMonitor   *monitor,
                                       CdkRectangle *area)
{
  CdkX11Screen *x11_screen;
  CdkAtom net_workareas;
  CdkDisplay *display;
  Display *xdisplay;
  int current_desktop;
  char *workareas_dn_name;
  Atom workareas_dn;
  int screen_number;
  Window xroot;
  int result;
  Atom type;
  int format;
  gulong num;
  gulong leftovers;
  guchar *ret_workarea;
  long *workareas;
  CdkRectangle geometry;
  int i;

  x11_screen = CDK_X11_SCREEN (screen);

  net_workareas = cdk_atom_intern_static_string ("_CTK_WORKAREAS");
  if (!cdk_x11_screen_supports_net_wm_hint (screen, net_workareas))
    return FALSE;

  display = cdk_screen_get_display (screen);
  xdisplay = cdk_x11_display_get_xdisplay (display);

  current_desktop = get_current_desktop (screen);
  workareas_dn_name = g_strdup_printf ("_CTK_WORKAREAS_D%d", current_desktop);
  workareas_dn = XInternAtom (xdisplay, workareas_dn_name, True);
  g_free (workareas_dn_name);

  if (workareas_dn == None)
    return FALSE;

  screen_number = cdk_x11_screen_get_screen_number (screen);
  xroot = XRootWindow (xdisplay, screen_number);

  cdk_x11_display_error_trap_push (display);

  ret_workarea = NULL;
  result = XGetWindowProperty (xdisplay,
                               xroot,
                               workareas_dn,
                               0,
                               G_MAXLONG,
                               False,
                               AnyPropertyType,
                               &type,
                               &format,
                               &num,
                               &leftovers,
                               &ret_workarea);

  cdk_x11_display_error_trap_pop_ignored (display);

  if (result != Success ||
      type == None ||
      format == 0 ||
      leftovers ||
      num % 4 != 0)
    {
      XFree (ret_workarea);

      return FALSE;
    }

  workareas = (long *) ret_workarea;

  cdk_monitor_get_geometry (monitor, &geometry);
  *area = geometry;

  for (i = 0; i < num / 4; i++)
    {
      CdkRectangle work_area;

      work_area = (CdkRectangle) {
        .x = workareas[0] / x11_screen->window_scale,
        .y = workareas[1] / x11_screen->window_scale,
        .width = workareas[2] / x11_screen->window_scale,
        .height = workareas[3] / x11_screen->window_scale,
      };

      if (cdk_rectangle_intersect (area, &work_area, &work_area))
        *area = work_area;

      workareas += 4;
    }

  XFree (ret_workarea);

  return TRUE;
}

void
cdk_x11_screen_get_work_area (CdkScreen    *screen,
                              CdkRectangle *area)
{
  CdkX11Screen   *x11_screen = CDK_X11_SCREEN (screen);
  Atom            workarea;
  Atom            type;
  Window          win;
  int             format;
  gulong          num;
  gulong          leftovers;
  gulong          max_len = 4 * 32;
  guchar         *ret_workarea = NULL;
  long           *workareas;
  int             result;
  int             disp_screen;
  int             desktop;
  Display        *display;

  display = CDK_DISPLAY_XDISPLAY (cdk_screen_get_display (screen));
  disp_screen = CDK_SCREEN_XNUMBER (screen);
  workarea = XInternAtom (display, "_NET_WORKAREA", True);

  /* Defaults in case of error */
  area->x = 0;
  area->y = 0;
  area->width = cdk_x11_screen_get_width (screen);
  area->height = cdk_x11_screen_get_height (screen);

  if (!cdk_x11_screen_supports_net_wm_hint (screen,
                                            cdk_atom_intern_static_string ("_NET_WORKAREA")))
    return;

  if (workarea == None)
    return;

  win = XRootWindow (display, disp_screen);
  result = XGetWindowProperty (display,
                               win,
                               workarea,
                               0,
                               max_len,
                               False,
                               AnyPropertyType,
                               &type,
                               &format,
                               &num,
                               &leftovers,
                               &ret_workarea);
  if (result != Success ||
      type == None ||
      format == 0 ||
      leftovers ||
      num % 4 != 0)
    goto out;

  desktop = get_current_desktop (screen);
  if (desktop + 1 > num / 4) /* fvwm gets this wrong */
    goto out;

  workareas = (long *) ret_workarea;
  area->x = workareas[desktop * 4];
  area->y = workareas[desktop * 4 + 1];
  area->width = workareas[desktop * 4 + 2];
  area->height = workareas[desktop * 4 + 3];

  area->x /= x11_screen->window_scale;
  area->y /= x11_screen->window_scale;
  area->width /= x11_screen->window_scale;
  area->height /= x11_screen->window_scale;

out:
  if (ret_workarea)
    XFree (ret_workarea);
}

static CdkVisual *
cdk_x11_screen_get_rgba_visual (CdkScreen *screen)
{
  CdkX11Screen *x11_screen = CDK_X11_SCREEN (screen);

  return x11_screen->rgba_visual;
}

/**
 * cdk_x11_screen_get_xscreen:
 * @screen: (type CdkX11Screen): a #CdkScreen
 *
 * Returns the screen of a #CdkScreen.
 *
 * Returns: (transfer none): an Xlib Screen*
 *
 * Since: 2.2
 */
Screen *
cdk_x11_screen_get_xscreen (CdkScreen *screen)
{
  return CDK_X11_SCREEN (screen)->xscreen;
}

/**
 * cdk_x11_screen_get_screen_number:
 * @screen: (type CdkX11Screen): a #CdkScreen
 *
 * Returns the index of a #CdkScreen.
 *
 * Returns: the position of @screen among the screens
 *     of its display
 *
 * Since: 2.2
 */
int
cdk_x11_screen_get_screen_number (CdkScreen *screen)
{
  return CDK_X11_SCREEN (screen)->screen_num;
}

static Atom
get_cm_atom (CdkX11Screen *x11_screen)
{
  return _cdk_x11_get_xatom_for_display_printf (x11_screen->display, "_NET_WM_CM_S%d", x11_screen->screen_num);
}

static gboolean
check_is_composited (CdkDisplay *display,
		     CdkX11Screen *x11_screen)
{
  Window xwindow;
  
  xwindow = XGetSelectionOwner (CDK_DISPLAY_XDISPLAY (display), get_cm_atom (x11_screen));

  return xwindow != None;
}

static CdkX11Monitor *
find_monitor_by_output (CdkX11Display *x11_display, XID output)
{
  int i;

  for (i = 0; i < x11_display->monitors->len; i++)
    {
      CdkX11Monitor *monitor = x11_display->monitors->pdata[i];
      if (monitor->output == output)
        return monitor;
    }

  return NULL;
}

static CdkSubpixelLayout
translate_subpixel_order (int subpixel)
{
  switch (subpixel)
    {
    case 1: return CDK_SUBPIXEL_LAYOUT_HORIZONTAL_RGB;
    case 2: return CDK_SUBPIXEL_LAYOUT_HORIZONTAL_BGR;
    case 3: return CDK_SUBPIXEL_LAYOUT_VERTICAL_RGB;
    case 4: return CDK_SUBPIXEL_LAYOUT_VERTICAL_BGR;
    case 5: return CDK_SUBPIXEL_LAYOUT_NONE;
    default: return CDK_SUBPIXEL_LAYOUT_UNKNOWN;
    }
}

static gboolean
init_randr15 (CdkScreen *screen, gboolean *changed)
{
#ifdef HAVE_RANDR15
  CdkDisplay *display = cdk_screen_get_display (screen);
  CdkX11Display *x11_display = CDK_X11_DISPLAY (display);
  CdkX11Screen *x11_screen = CDK_X11_SCREEN (screen);
  XRRScreenResources *resources;
  RROutput primary_output = None;
  RROutput first_output = None;
  int i;
  gboolean randr12_compat = FALSE;
  XRRMonitorInfo *rr_monitors;
  int num_rr_monitors;
  int old_primary;

  if (!x11_display->have_randr15)
    return FALSE;

  resources = XRRGetScreenResourcesCurrent (x11_screen->xdisplay,
                                            x11_screen->xroot_window);
  if (!resources)
    return FALSE;

  rr_monitors = XRRGetMonitors (x11_screen->xdisplay,
                                x11_screen->xroot_window,
                                True,
                                &num_rr_monitors);
  if (!rr_monitors)
    return FALSE;

  for (i = 0; i < x11_display->monitors->len; i++)
    {
      CdkX11Monitor *monitor = x11_display->monitors->pdata[i];
      monitor->add = FALSE;
      monitor->remove = TRUE;
    }

  for (i = 0; i < num_rr_monitors; i++)
    {
      RROutput output = rr_monitors[i].outputs[0];
      XRROutputInfo *output_info;
      CdkX11Monitor *monitor;
      CdkRectangle geometry;
      CdkRectangle newgeo;
      char *name;
      char *manufacturer = NULL;
      int refresh_rate = 0;

      cdk_x11_display_error_trap_push (display);
      output_info = XRRGetOutputInfo (x11_screen->xdisplay, resources, output);
      if (cdk_x11_display_error_trap_pop (display))
        continue;

      if (output_info == NULL)
        continue;

      /* Non RandR1.2+ X driver have output name "default" */
      randr12_compat |= !g_strcmp0 (output_info->name, "default");

      if (output_info->connection == RR_Disconnected)
        {
          XRRFreeOutputInfo (output_info);
          continue;
        }

      if (first_output == None)
        first_output = output;

      if (output_info->crtc)
        {
          XRRCrtcInfo *crtc = XRRGetCrtcInfo (x11_screen->xdisplay, resources, output_info->crtc);
          int j;

          for (j = 0; j < resources->nmode; j++)
            {
              XRRModeInfo *xmode = &resources->modes[j];
              if (xmode->id == crtc->mode)
                {
                  if (xmode->hTotal != 0 && xmode->vTotal != 0)
                    refresh_rate = (1000 * xmode->dotClock) / (xmode->hTotal * xmode->vTotal);
                  break;
                }
            }

          XRRFreeCrtcInfo (crtc);
        }

      monitor = find_monitor_by_output (x11_display, output);
      if (monitor)
        monitor->remove = FALSE;
      else
        {
          monitor = g_object_new (CDK_TYPE_X11_MONITOR,
                                  "display", display,
                                  NULL);
          monitor->output = output;
          monitor->add = TRUE;
          g_ptr_array_add (x11_display->monitors, monitor);
        }

      /* Fetch minimal manufacturer information (PNP ID) from EDID */
      {
        #define EDID_LENGTH 128
        Atom actual_type, edid_atom;
        char tmp[3];
        int actual_format;
        unsigned char *prop;
        unsigned long nbytes, bytes_left;
        Display *disp = CDK_DISPLAY_XDISPLAY (x11_display);

        edid_atom = XInternAtom (disp, RR_PROPERTY_RANDR_EDID, FALSE);

        XRRGetOutputProperty (disp, output,
                              edid_atom,
                              0,
                              EDID_LENGTH,
                              FALSE,
                              FALSE,
                              AnyPropertyType,
                              &actual_type,
                              &actual_format,
                              &nbytes,
                              &bytes_left,
                              &prop);

        // Check partial EDID header (whole header: 00 ff ff ff ff ff ff 00)
        if (nbytes >= EDID_LENGTH && prop[0] == 0x00 && prop[1] == 0xff)
          {
            /* decode the Vendor ID from three 5 bit words packed into 2 bytes
             * /--08--\/--09--\
             * 7654321076543210
             * |\---/\---/\---/
             * R  C1   C2   C3 */
            tmp[0] = 'A' + ((prop[8] & 0x7c) / 4) - 1;
            tmp[1] = 'A' + ((prop[8] & 0x3) * 8) + ((prop[9] & 0xe0) / 32) - 1;
            tmp[2] = 'A' + (prop[9] & 0x1f) - 1;

            manufacturer = g_strndup (tmp, sizeof (tmp));
          }

        XFree(prop);
        #undef EDID_LENGTH
      }

      cdk_monitor_get_geometry (CDK_MONITOR (monitor), &geometry);
      name = g_strndup (output_info->name, output_info->nameLen);

      newgeo.x = rr_monitors[i].x / x11_screen->window_scale;
      newgeo.y = rr_monitors[i].y / x11_screen->window_scale;
      newgeo.width = rr_monitors[i].width / x11_screen->window_scale;
      newgeo.height = rr_monitors[i].height / x11_screen->window_scale;
      if (newgeo.x != geometry.x ||
          newgeo.y != geometry.y ||
          newgeo.width != geometry.width ||
          newgeo.height != geometry.height ||
          rr_monitors[i].mwidth != cdk_monitor_get_width_mm (CDK_MONITOR (monitor)) ||
          rr_monitors[i].mheight != cdk_monitor_get_height_mm (CDK_MONITOR (monitor)) ||
          g_strcmp0 (name, cdk_monitor_get_model (CDK_MONITOR (monitor))))
        *changed = TRUE;

      cdk_monitor_set_position (CDK_MONITOR (monitor), newgeo.x, newgeo.y);
      cdk_monitor_set_size (CDK_MONITOR (monitor), newgeo.width, newgeo.height);
      g_object_notify (G_OBJECT (monitor), "workarea");
      cdk_monitor_set_physical_size (CDK_MONITOR (monitor),
                                     rr_monitors[i].mwidth,
                                     rr_monitors[i].mheight);
      cdk_monitor_set_subpixel_layout (CDK_MONITOR (monitor),
                                       translate_subpixel_order (output_info->subpixel_order));
      cdk_monitor_set_refresh_rate (CDK_MONITOR (monitor), refresh_rate);
      cdk_monitor_set_scale_factor (CDK_MONITOR (monitor), x11_screen->window_scale);
      cdk_monitor_set_model (CDK_MONITOR (monitor), name);
      cdk_monitor_set_connector (CDK_MONITOR (monitor), name);
      cdk_monitor_set_manufacturer (CDK_MONITOR (monitor), manufacturer);
      g_free (manufacturer);
      g_free (name);

      if (rr_monitors[i].primary)
        primary_output = monitor->output;

      XRRFreeOutputInfo (output_info);
    }

  XRRFreeMonitors (rr_monitors);
  XRRFreeScreenResources (resources);

  /* non RandR 1.2+ X driver doesn't return any usable multihead data */
  if (randr12_compat)
    {
      for (i = 0; i < x11_display->monitors->len; i++)
        {
          CdkX11Monitor *monitor = x11_display->monitors->pdata[i];
          if (monitor->remove)
            cdk_display_monitor_removed (display, CDK_MONITOR (monitor));
        }
      g_ptr_array_remove_range (x11_display->monitors, 0, x11_display->monitors->len);
      return FALSE;
    }

  for (i = x11_display->monitors->len - 1; i >= 0; i--)
    {
      CdkX11Monitor *monitor = x11_display->monitors->pdata[i];
      if (monitor->add)
        {
          cdk_display_monitor_added (display, CDK_MONITOR (monitor));
          *changed = TRUE;
        }
      else if (monitor->remove)
        {
          g_object_ref (monitor);
          g_ptr_array_remove (x11_display->monitors, monitor);
          cdk_display_monitor_removed (display, CDK_MONITOR (monitor));
          g_object_unref (monitor);
          *changed = TRUE;
        }
    }

  old_primary = x11_display->primary_monitor;
  x11_display->primary_monitor = 0;
  for (i = 0; i < x11_display->monitors->len; ++i)
    {
      CdkX11Monitor *monitor = x11_display->monitors->pdata[i];
      if (monitor->output == primary_output)
        {
          x11_display->primary_monitor = i;
          break;
        }

      /* No RandR1.3+ available or no primary set, fall back to prefer LVDS as primary if present */
      if (primary_output == None &&
          g_ascii_strncasecmp (cdk_monitor_get_model (CDK_MONITOR (monitor)), "LVDS", 4) == 0)
        {
          x11_display->primary_monitor = i;
          break;
        }

      /* No primary specified and no LVDS found */
      if (monitor->output == first_output)
        x11_display->primary_monitor = i;
    }

  if (x11_display->primary_monitor != old_primary)
    *changed = TRUE;

  return x11_display->monitors->len > 0;
#endif

  return FALSE;
}

static gboolean
init_randr13 (CdkScreen *screen, gboolean *changed)
{
#ifdef HAVE_RANDR
  CdkDisplay *display = cdk_screen_get_display (screen);
  CdkX11Display *x11_display = CDK_X11_DISPLAY (display);
  CdkX11Screen *x11_screen = CDK_X11_SCREEN (screen);
  XRRScreenResources *resources;
  RROutput primary_output = None;
  RROutput first_output = None;
  int i;
  gboolean randr12_compat = FALSE;
  int old_primary;

  if (!x11_display->have_randr13)
      return FALSE;

  resources = XRRGetScreenResourcesCurrent (x11_screen->xdisplay,
                                            x11_screen->xroot_window);
  if (!resources)
    return FALSE;

  for (i = 0; i < x11_display->monitors->len; i++)
    {
      CdkX11Monitor *monitor = x11_display->monitors->pdata[i];
      monitor->add = FALSE;
      monitor->remove = TRUE;
    }

  for (i = 0; i < resources->noutput; ++i)
    {
      RROutput output = resources->outputs[i];
      XRROutputInfo *output_info =
        XRRGetOutputInfo (x11_screen->xdisplay, resources, output);

      /* Non RandR1.2+ X driver have output name "default" */
      randr12_compat |= !g_strcmp0 (output_info->name, "default");

      if (output_info->connection == RR_Disconnected)
        {
          XRRFreeOutputInfo (output_info);
          continue;
        }

      if (output_info->crtc)
	{
	  CdkX11Monitor *monitor;
	  XRRCrtcInfo *crtc = XRRGetCrtcInfo (x11_screen->xdisplay, resources, output_info->crtc);
          char *name;
          CdkRectangle geometry;
          CdkRectangle newgeo;
          int j;
          int refresh_rate = 0;

          for (j = 0; j < resources->nmode; j++)
            {
              XRRModeInfo *xmode = &resources->modes[j];
              if (xmode->id == crtc->mode)
                {
                  if (xmode->hTotal != 0 && xmode->vTotal != 0)
                    refresh_rate = (1000 * xmode->dotClock) / (xmode->hTotal * xmode->vTotal);
                  break;
                }
            }

          monitor = find_monitor_by_output (x11_display, output);
          if (monitor)
            monitor->remove = FALSE;
          else
            {
              monitor = g_object_new (cdk_x11_monitor_get_type (),
                                      "display", display,
                                      NULL);
              monitor->output = output;
              monitor->add = TRUE;
              g_ptr_array_add (x11_display->monitors, monitor);
            }

          cdk_monitor_get_geometry (CDK_MONITOR (monitor), &geometry);
          name = g_strndup (output_info->name, output_info->nameLen);

          newgeo.x = crtc->x / x11_screen->window_scale;
          newgeo.y = crtc->y / x11_screen->window_scale;
          newgeo.width = crtc->width / x11_screen->window_scale;
          newgeo.height = crtc->height / x11_screen->window_scale;
          if (newgeo.x != geometry.x ||
              newgeo.y != geometry.y ||
              newgeo.width != geometry.width ||
              newgeo.height != geometry.height ||
              output_info->mm_width != cdk_monitor_get_width_mm (CDK_MONITOR (monitor)) ||
              output_info->mm_height != cdk_monitor_get_height_mm (CDK_MONITOR (monitor)) ||
              g_strcmp0 (name, cdk_monitor_get_model (CDK_MONITOR (monitor))) != 0)
            *changed = TRUE;

          cdk_monitor_set_position (CDK_MONITOR (monitor), newgeo.x, newgeo.y);
          cdk_monitor_set_size (CDK_MONITOR (monitor), newgeo.width, newgeo.height);
          g_object_notify (G_OBJECT (monitor), "workarea");
          cdk_monitor_set_physical_size (CDK_MONITOR (monitor),
                                         output_info->mm_width,
                                         output_info->mm_height);
          cdk_monitor_set_subpixel_layout (CDK_MONITOR (monitor),
                                           translate_subpixel_order (output_info->subpixel_order));
          cdk_monitor_set_refresh_rate (CDK_MONITOR (monitor), refresh_rate);
          cdk_monitor_set_scale_factor (CDK_MONITOR (monitor), x11_screen->window_scale);
          cdk_monitor_set_model (CDK_MONITOR (monitor), name);

          g_free (name);

          XRRFreeCrtcInfo (crtc);
	}

      XRRFreeOutputInfo (output_info);
    }

  if (resources->noutput > 0)
    first_output = resources->outputs[0];

  XRRFreeScreenResources (resources);

  if (randr12_compat)
    {
      for (i = 0; i < x11_display->monitors->len; i++)
        {
          CdkX11Monitor *monitor = x11_display->monitors->pdata[i];
          if (monitor->remove)
            cdk_display_monitor_removed (display, CDK_MONITOR (monitor));
        }
      g_ptr_array_remove_range (x11_display->monitors, 0, x11_display->monitors->len);
      return FALSE;
    }

  for (i = x11_display->monitors->len - 1; i >= 0; i--)
    {
      CdkX11Monitor *monitor = x11_display->monitors->pdata[i];
      if (monitor->add)
        {
          cdk_display_monitor_added (display, CDK_MONITOR (monitor));
          *changed = TRUE;
        }
      else if (monitor->remove)
        {
          g_object_ref (monitor);
          g_ptr_array_remove (x11_display->monitors, monitor);
          cdk_display_monitor_removed (display, CDK_MONITOR (monitor));
          g_object_unref (monitor);
          *changed = TRUE;
        }
    }

  old_primary = x11_display->primary_monitor;
  x11_display->primary_monitor = 0;
  primary_output = XRRGetOutputPrimary (x11_screen->xdisplay,
                                        x11_screen->xroot_window);

  for (i = 0; i < x11_display->monitors->len; ++i)
    {
      CdkX11Monitor *monitor = x11_display->monitors->pdata[i];
      if (monitor->output == primary_output)
        {
          x11_display->primary_monitor = i;
          break;
        }

      /* No RandR1.3+ available or no primary set, fall back to prefer LVDS as primary if present */
      if (primary_output == None &&
          g_ascii_strncasecmp (cdk_monitor_get_model (CDK_MONITOR (monitor)), "LVDS", 4) == 0)
        {
          x11_display->primary_monitor = i;
          break;
        }

      /* No primary specified and no LVDS found */
      if (monitor->output == first_output)
        x11_display->primary_monitor = i;
    }

  if (x11_display->primary_monitor != old_primary)
    *changed = TRUE;

  return x11_display->monitors->len > 0;
#endif

  return FALSE;
}

static void
init_no_multihead (CdkScreen *screen, gboolean *changed)
{
  CdkDisplay *display = cdk_screen_get_display (screen);
  CdkX11Display *x11_display = CDK_X11_DISPLAY (display);
  CdkX11Screen *x11_screen = CDK_X11_SCREEN (screen);
  CdkX11Monitor *monitor;
  CdkRectangle geometry;
  CdkRectangle newgeo;
  int i;

  for (i = 0; i < x11_display->monitors->len; i++)
    {
      CdkX11Monitor *monitor = x11_display->monitors->pdata[i];
      monitor->add = FALSE;
      monitor->remove = TRUE;
    }

  monitor = find_monitor_by_output (x11_display, 0);
  if (monitor)
    monitor->remove = FALSE;
  else
    {
      monitor = g_object_new (cdk_x11_monitor_get_type (),
                              "display", x11_display,
                              NULL);
      monitor->output = 0;
      monitor->add = TRUE;
      g_ptr_array_add (x11_display->monitors, monitor);
    }

  cdk_monitor_get_geometry (CDK_MONITOR (monitor), &geometry);

  newgeo.x = 0;
  newgeo.y = 0;
  newgeo.width = DisplayWidth (x11_display->xdisplay, x11_screen->screen_num) /
                               x11_screen->window_scale;
  newgeo.height = DisplayHeight (x11_display->xdisplay, x11_screen->screen_num) /
                                 x11_screen->window_scale;

  if (newgeo.x != geometry.x ||
      newgeo.y != geometry.y ||
      newgeo.width != geometry.width ||
      newgeo.height != geometry.height ||
      cdk_x11_screen_get_width_mm (screen) != cdk_monitor_get_width_mm (CDK_MONITOR (monitor)) ||
      cdk_x11_screen_get_height_mm (screen) != cdk_monitor_get_height_mm (CDK_MONITOR (monitor)))
    *changed = TRUE;

  cdk_monitor_set_position (CDK_MONITOR (monitor), newgeo.x, newgeo.y);
  cdk_monitor_set_size (CDK_MONITOR (monitor), newgeo.width, newgeo.height);

  g_object_notify (G_OBJECT (monitor), "workarea");
  cdk_monitor_set_physical_size (CDK_MONITOR (monitor),
                                 cdk_x11_screen_get_width_mm (screen),
                                 cdk_x11_screen_get_height_mm (screen));
  cdk_monitor_set_scale_factor (CDK_MONITOR (monitor), x11_screen->window_scale);

  if (x11_display->primary_monitor != 0)
    *changed = TRUE;
  x11_display->primary_monitor = 0;

  for (i = x11_display->monitors->len - 1; i >= 0; i--)
    {
      CdkX11Monitor *monitor = x11_display->monitors->pdata[i];
      if (monitor->add)
        {
          cdk_display_monitor_added (CDK_DISPLAY (x11_display), CDK_MONITOR (monitor));
          *changed = TRUE;
        }
      else if (monitor->remove)
        {
          g_object_ref (monitor);
          g_ptr_array_remove (x11_display->monitors, monitor);
          cdk_display_monitor_removed (CDK_DISPLAY (x11_display), CDK_MONITOR (monitor));
          g_object_unref (monitor);
          *changed = TRUE;
        }
    }
}

static gboolean
init_multihead (CdkScreen *screen)
{
  gboolean any_changed = FALSE;

  if (!init_randr15 (screen, &any_changed) &&
      !init_randr13 (screen, &any_changed))
    init_no_multihead (screen, &any_changed);

  return any_changed;
}

static void
update_bounding_box (CdkScreen *screen)
{
  CdkX11Screen *x11_screen = CDK_X11_SCREEN (screen);
  CdkX11Display *x11_display = CDK_X11_DISPLAY (x11_screen->display);
  gint i, x1, y1, x2, y2;

  x1 = y1 = G_MAXINT;
  x2 = y2 = G_MININT;

  for (i = 0; i < x11_display->monitors->len; i++)
    {
      CdkX11Monitor *monitor = x11_display->monitors->pdata[i];
      CdkRectangle geometry;

      cdk_monitor_get_geometry (CDK_MONITOR (monitor), &geometry);
      x1 = MIN (x1, geometry.x);
      y1 = MIN (y1, geometry.y);
      x2 = MAX (x2, geometry.x + geometry.width);
      y2 = MAX (y2, geometry.y + geometry.height);
    }

  x11_screen->width = x2 - x1;
  x11_screen->height = y2 - y1;
}

CdkScreen *
_cdk_x11_screen_new (CdkDisplay *display,
		     gint	 screen_number) 
{
  CdkScreen *screen;
  CdkX11Screen *x11_screen;
  CdkX11Display *display_x11 = CDK_X11_DISPLAY (display);
  const char *scale_str;

  screen = g_object_new (CDK_TYPE_X11_SCREEN, NULL);

  x11_screen = CDK_X11_SCREEN (screen);
  x11_screen->display = display;
  x11_screen->xdisplay = display_x11->xdisplay;
  x11_screen->xscreen = ScreenOfDisplay (display_x11->xdisplay, screen_number);
  x11_screen->screen_num = screen_number;
  x11_screen->xroot_window = RootWindow (display_x11->xdisplay,screen_number);
  x11_screen->wmspec_check_window = None;
  /* we want this to be always non-null */
  x11_screen->window_manager_name = g_strdup ("unknown");

  scale_str = g_getenv ("CDK_SCALE");
  if (scale_str)
    {
      x11_screen->fixed_window_scale = TRUE;
      x11_screen->window_scale = atol (scale_str);
      if (x11_screen->window_scale == 0)
        x11_screen->window_scale = 1;
    }
  else
    x11_screen->window_scale = 1;

  init_randr_support (screen);
  init_multihead (screen);

  _cdk_x11_screen_init_visuals (screen);
  _cdk_x11_screen_init_root_window (screen);
  update_bounding_box (screen);

  return screen;
}

void
_cdk_x11_screen_set_window_scale (CdkX11Screen *x11_screen,
				  gint          scale)
{
  GList *toplevels, *l;
  CdkWindow *root;

  if (x11_screen->window_scale == scale)
    return;

  x11_screen->window_scale = scale;

  root = x11_screen->root_window;
  CDK_WINDOW_IMPL_X11 (root->impl)->window_scale = scale;

  if (CDK_WINDOW_IMPL_X11 (root->impl)->cairo_surface)
    cairo_surface_set_device_scale (CDK_WINDOW_IMPL_X11 (root->impl)->cairo_surface,
                                    scale, scale);

  toplevels = cdk_screen_get_toplevel_windows (CDK_SCREEN (x11_screen));

  for (l = toplevels; l != NULL; l = l->next)
    {
      CdkWindow *window = l->data;

      _cdk_x11_window_set_window_scale (window, scale);
    }

  process_monitors_change (CDK_SCREEN (x11_screen));
}

/*
 * It is important that we first request the selection
 * notification, and then setup the initial state of
 * is_composited to avoid a race condition here.
 */
void
_cdk_x11_screen_setup (CdkScreen *screen)
{
  CdkX11Screen *x11_screen = CDK_X11_SCREEN (screen);

  cdk_display_request_selection_notification (x11_screen->display,
					      cdk_x11_xatom_to_atom_for_display (x11_screen->display, get_cm_atom (x11_screen)));
  x11_screen->is_composited = check_is_composited (x11_screen->display, x11_screen);
}

static gboolean
cdk_x11_screen_is_composited (CdkScreen *screen)
{
  CdkX11Screen *x11_screen = CDK_X11_SCREEN (screen);

  return x11_screen->is_composited;
}

static void
init_randr_support (CdkScreen *screen)
{
  CdkX11Screen *x11_screen = CDK_X11_SCREEN (screen);

  /* NB: This is also needed for XSettings, so don't remove. */
  XSelectInput (CDK_SCREEN_XDISPLAY (screen),
                x11_screen->xroot_window,
                StructureNotifyMask);

#ifdef HAVE_RANDR
  if (!CDK_X11_DISPLAY (cdk_screen_get_display (screen))->have_randr12)
    return;

  XRRSelectInput (CDK_SCREEN_XDISPLAY (screen),
                  x11_screen->xroot_window,
                  RRScreenChangeNotifyMask
                  | RRCrtcChangeNotifyMask
                  | RROutputPropertyNotifyMask);
#endif
}

static void
process_monitors_change (CdkScreen *screen)
{
  if (init_multihead (screen))
    {
      update_bounding_box (screen);
      g_signal_emit_by_name (screen, "monitors-changed");
    }
}

void
_cdk_x11_screen_size_changed (CdkScreen *screen,
			      XEvent    *event)
{
  gint width, height;
#ifdef HAVE_RANDR
  CdkX11Display *display_x11;
#endif

  width = cdk_x11_screen_get_width (screen);
  height = cdk_x11_screen_get_height (screen);

#ifdef HAVE_RANDR
  display_x11 = CDK_X11_DISPLAY (cdk_screen_get_display (screen));

  if (display_x11->have_randr13 && event->type == ConfigureNotify)
    return;

  XRRUpdateConfiguration (event);
#else
  if (event->type == ConfigureNotify)
    {
      XConfigureEvent *rcevent = (XConfigureEvent *) event;
      Screen	    *xscreen = cdk_x11_screen_get_xscreen (screen);

      xscreen->width   = rcevent->width;
      xscreen->height  = rcevent->height;
    }
  else
    return;
#endif

  process_monitors_change (screen);

  if (width != cdk_x11_screen_get_width (screen) ||
      height != cdk_x11_screen_get_height (screen))
    g_signal_emit_by_name (screen, "size-changed");
}

void
_cdk_x11_screen_get_edge_monitors (CdkScreen *screen,
                                   gint      *top,
                                   gint      *bottom,
                                   gint      *left,
                                   gint      *right)
{
#ifdef HAVE_XFREE_XINERAMA
  CdkX11Screen *x11_screen = CDK_X11_SCREEN (screen);
  gint          top_most_pos = x11_screen->height;
  gint          left_most_pos = x11_screen->width;
  gint          bottom_most_pos = 0;
  gint          right_most_pos = 0;
  gint          i;
  XineramaScreenInfo *x_monitors;
  int x_n_monitors;
#endif

  *top = *bottom = *left = *right = -1;

#ifdef HAVE_XFREE_XINERAMA
  if (!XineramaIsActive (x11_screen->xdisplay))
    return;

  x_monitors = XineramaQueryScreens (x11_screen->xdisplay, &x_n_monitors);
  if (x_n_monitors <= 0 || x_monitors == NULL)
    {
      if (x_monitors)
        XFree (x_monitors);

      return;
    }

  for (i = 0; i < x_n_monitors; i++)
    {
      if (left && left_most_pos > x_monitors[i].x_org)
	{
	  left_most_pos = x_monitors[i].x_org;
	  *left = i;
	}
      if (right && right_most_pos < x_monitors[i].x_org + x_monitors[i].width)
	{
	  right_most_pos = x_monitors[i].x_org + x_monitors[i].width;
	  *right = i;
	}
      if (top && top_most_pos > x_monitors[i].y_org)
	{
	  top_most_pos = x_monitors[i].y_org;
	  *top = i;
	}
      if (bottom && bottom_most_pos < x_monitors[i].y_org + x_monitors[i].height)
	{
	  bottom_most_pos = x_monitors[i].y_org + x_monitors[i].height;
	  *bottom = i;
	}
    }

  XFree (x_monitors);
#endif
}

void
_cdk_x11_screen_window_manager_changed (CdkScreen *screen)
{
  g_signal_emit (screen, signals[WINDOW_MANAGER_CHANGED], 0);
}

void
_cdk_x11_screen_process_owner_change (CdkScreen *screen,
				      XEvent *event)
{
#ifdef HAVE_XFIXES
  XFixesSelectionNotifyEvent *selection_event = (XFixesSelectionNotifyEvent *)event;
  CdkX11Screen *x11_screen = CDK_X11_SCREEN (screen);

  if (selection_event->selection == get_cm_atom (x11_screen))
    {
      gboolean composited = selection_event->owner != None;

      if (composited != x11_screen->is_composited)
	{
	  x11_screen->is_composited = composited;

	  g_signal_emit_by_name (screen, "composited-changed");
	}
    }
#endif
}

static gchar *
substitute_screen_number (const gchar *display_name,
                          gint         screen_number)
{
  GString *str;
  gchar   *p;

  str = g_string_new (display_name);

  p = strrchr (str->str, '.');
  if (p && p >	strchr (str->str, ':'))
    g_string_truncate (str, p - str->str);

  g_string_append_printf (str, ".%d", screen_number);

  return g_string_free (str, FALSE);
}

static gchar *
cdk_x11_screen_make_display_name (CdkScreen *screen)
{
  const gchar *old_display;

  old_display = cdk_display_get_name (cdk_screen_get_display (screen));

  return substitute_screen_number (old_display,
                                   cdk_x11_screen_get_number (screen));
}

static CdkWindow *
cdk_x11_screen_get_active_window (CdkScreen *screen)
{
  CdkX11Screen *x11_screen = CDK_X11_SCREEN (screen);
  CdkWindow *ret = NULL;
  Atom type_return;
  gint format_return;
  gulong nitems_return;
  gulong bytes_after_return;
  guchar *data = NULL;

  if (!cdk_x11_screen_supports_net_wm_hint (screen,
                                            cdk_atom_intern_static_string ("_NET_ACTIVE_WINDOW")))
    return NULL;

  if (XGetWindowProperty (x11_screen->xdisplay, x11_screen->xroot_window,
	                  cdk_x11_get_xatom_by_name_for_display (x11_screen->display,
			                                         "_NET_ACTIVE_WINDOW"),
		          0, 1, False, XA_WINDOW, &type_return,
		          &format_return, &nitems_return,
                          &bytes_after_return, &data)
      == Success)
    {
      if ((type_return == XA_WINDOW) && (format_return == 32) && (data))
        {
          Window window = *(Window *) data;

          if (window != None)
            {
              ret = cdk_x11_window_foreign_new_for_display (x11_screen->display,
                                                            window);
            }
        }
    }

  if (data)
    XFree (data);

  return ret;
}

static GList *
cdk_x11_screen_get_window_stack (CdkScreen *screen)
{
  CdkX11Screen *x11_screen = CDK_X11_SCREEN (screen);
  GList *ret = NULL;
  Atom type_return;
  gint format_return;
  gulong nitems_return;
  gulong bytes_after_return;
  guchar *data = NULL;

  if (!cdk_x11_screen_supports_net_wm_hint (screen,
                                            cdk_atom_intern_static_string ("_NET_CLIENT_LIST_STACKING")))
    return NULL;

  if (XGetWindowProperty (x11_screen->xdisplay, x11_screen->xroot_window,
	                  cdk_x11_get_xatom_by_name_for_display (x11_screen->display,
			                                         "_NET_CLIENT_LIST_STACKING"),
		          0, G_MAXLONG, False, XA_WINDOW, &type_return,
		          &format_return, &nitems_return,
                          &bytes_after_return, &data)
      == Success)
    {
      if ((type_return == XA_WINDOW) && (format_return == 32) &&
          (data) && (nitems_return > 0))
        {
          gulong *stack = (gulong *) data;
          CdkWindow *win;
          int i;

          for (i = 0; i < nitems_return; i++)
            {
              win = cdk_x11_window_foreign_new_for_display (x11_screen->display,
                                                            (Window)stack[i]);

              if (win != NULL)
                ret = g_list_append (ret, win);
            }
        }
    }

  if (data)
    XFree (data);

  return ret;
}

static gboolean
cdk_x11_screen_get_setting (CdkScreen   *screen,
			    const gchar *name,
			    GValue      *value)
{
  CdkX11Screen *x11_screen = CDK_X11_SCREEN (screen);
  const GValue *setting;

  if (x11_screen->xsettings == NULL)
    goto out;
  setting = g_hash_table_lookup (x11_screen->xsettings, name);
  if (setting == NULL)
    goto out;

  if (!g_value_transform (setting, value))
    {
      g_warning ("Cannot transform xsetting %s of type %s to type %s\n",
		 name,
		 g_type_name (G_VALUE_TYPE (setting)),
		 g_type_name (G_VALUE_TYPE (value)));
      goto out;
    }

  return TRUE;

 out:
  return _cdk_x11_get_xft_setting (screen, name, value);
}

static void
cleanup_atoms(gpointer data)
{
  NetWmSupportedAtoms *supported_atoms = data;
  if (supported_atoms->atoms)
      XFree (supported_atoms->atoms);
  g_free (supported_atoms);
}

static Window
get_net_supporting_wm_check (CdkX11Screen *screen,
                             Window        window)
{
  CdkDisplay *display;
  Atom type;
  gint format;
  gulong n_items;
  gulong bytes_after;
  guchar *data;
  Window value;

  display = screen->display;
  type = None;
  data = NULL;
  value = None;

  cdk_x11_display_error_trap_push (display);
  XGetWindowProperty (screen->xdisplay, window,
		      cdk_x11_get_xatom_by_name_for_display (display, "_NET_SUPPORTING_WM_CHECK"),
		      0, G_MAXLONG, False, XA_WINDOW, &type, &format,
		      &n_items, &bytes_after, &data);
  cdk_x11_display_error_trap_pop_ignored (display);

  if (type == XA_WINDOW)
    value = *(Window *)data;

  if (data)
    XFree (data);

  return value;
}

static void
fetch_net_wm_check_window (CdkScreen *screen)
{
  CdkX11Screen *x11_screen;
  CdkDisplay *display;
  Window window;
  guint64 now;
  gint error;

  x11_screen = CDK_X11_SCREEN (screen);
  display = x11_screen->display;

  g_return_if_fail (CDK_X11_DISPLAY (display)->trusted_client);

  if (x11_screen->wmspec_check_window != None)
    return; /* already have it */

  now = g_get_monotonic_time ();

  if ((now - x11_screen->last_wmspec_check_time) / 1e6 < 15)
    return; /* we've checked recently */

  window = get_net_supporting_wm_check (x11_screen, x11_screen->xroot_window);
  if (window == None)
    return;

  if (window != get_net_supporting_wm_check (x11_screen, window))
    return;

  cdk_x11_display_error_trap_push (display);

  /* Find out if this WM goes away, so we can reset everything. */
  XSelectInput (x11_screen->xdisplay, window, StructureNotifyMask);

  error = cdk_x11_display_error_trap_pop (display);
  if (!error)
    {
      /* We check the window property again because after XGetWindowProperty()
       * and before XSelectInput() the window may have been recycled in such a
       * way that XSelectInput() doesn't fail but the window is no longer what
       * we want.
       */
      if (window != get_net_supporting_wm_check (x11_screen, window))
        return;

      x11_screen->wmspec_check_window = window;
      x11_screen->last_wmspec_check_time = now;
      x11_screen->need_refetch_net_supported = TRUE;
      x11_screen->need_refetch_wm_name = TRUE;

      /* Careful, reentrancy */
      _cdk_x11_screen_window_manager_changed (screen);
    }
}

/**
 * cdk_x11_screen_supports_net_wm_hint:
 * @screen: (type CdkX11Screen): the relevant #CdkScreen.
 * @property: a property atom.
 *
 * This function is specific to the X11 backend of CDK, and indicates
 * whether the window manager supports a certain hint from the
 * [Extended Window Manager Hints](http://www.freedesktop.org/Standards/wm-spec) specification.
 *
 * When using this function, keep in mind that the window manager
 * can change over time; so you shouldn’t use this function in
 * a way that impacts persistent application state. A common bug
 * is that your application can start up before the window manager
 * does when the user logs in, and before the window manager starts
 * cdk_x11_screen_supports_net_wm_hint() will return %FALSE for every property.
 * You can monitor the window_manager_changed signal on #CdkScreen to detect
 * a window manager change.
 *
 * Returns: %TRUE if the window manager supports @property
 *
 * Since: 2.2
 **/
gboolean
cdk_x11_screen_supports_net_wm_hint (CdkScreen *screen,
				     CdkAtom    property)
{
  gulong i;
  CdkX11Screen *x11_screen;
  NetWmSupportedAtoms *supported_atoms;
  CdkDisplay *display;
  Atom atom;

  g_return_val_if_fail (CDK_IS_SCREEN (screen), FALSE);

  x11_screen = CDK_X11_SCREEN (screen);
  display = x11_screen->display;

  if (!G_LIKELY (CDK_X11_DISPLAY (display)->trusted_client))
    return FALSE;

  supported_atoms = g_object_get_data (G_OBJECT (screen), "cdk-net-wm-supported-atoms");
  if (!supported_atoms)
    {
      supported_atoms = g_new0 (NetWmSupportedAtoms, 1);
      g_object_set_data_full (G_OBJECT (screen), "cdk-net-wm-supported-atoms", supported_atoms, cleanup_atoms);
    }

  fetch_net_wm_check_window (screen);

  if (x11_screen->wmspec_check_window == None)
    return FALSE;

  if (x11_screen->need_refetch_net_supported)
    {
      /* WM has changed since we last got the supported list,
       * refetch it.
       */
      Atom type;
      gint format;
      gulong bytes_after;

      x11_screen->need_refetch_net_supported = FALSE;

      if (supported_atoms->atoms)
        XFree (supported_atoms->atoms);

      supported_atoms->atoms = NULL;
      supported_atoms->n_atoms = 0;

      XGetWindowProperty (CDK_DISPLAY_XDISPLAY (display), x11_screen->xroot_window,
                          cdk_x11_get_xatom_by_name_for_display (display, "_NET_SUPPORTED"),
                          0, G_MAXLONG, False, XA_ATOM, &type, &format,
                          &supported_atoms->n_atoms, &bytes_after,
                          (guchar **)&supported_atoms->atoms);

      if (type != XA_ATOM)
        return FALSE;
    }

  if (supported_atoms->atoms == NULL)
    return FALSE;

  atom = cdk_x11_atom_to_xatom_for_display (display, property);

  for (i = 0; i < supported_atoms->n_atoms; i++)
    {
      if (supported_atoms->atoms[i] == atom)
        return TRUE;
    }

  return FALSE;
}

/**
 * cdk_x11_screen_get_window_manager_name:
 * @screen: (type CdkX11Screen): a #CdkScreen
 *
 * Returns the name of the window manager for @screen.
 *
 * Returns: the name of the window manager screen @screen, or
 * "unknown" if the window manager is unknown. The string is owned by CDK
 * and should not be freed.
 *
 * Since: 2.2
 **/
const char*
cdk_x11_screen_get_window_manager_name (CdkScreen *screen)
{
  CdkX11Screen *x11_screen;
  CdkDisplay *display;

  x11_screen = CDK_X11_SCREEN (screen);
  display = x11_screen->display;

  if (!G_LIKELY (CDK_X11_DISPLAY (display)->trusted_client))
    return x11_screen->window_manager_name;

  fetch_net_wm_check_window (screen);

  if (x11_screen->need_refetch_wm_name)
    {
      /* Get the name of the window manager */
      x11_screen->need_refetch_wm_name = FALSE;

      g_free (x11_screen->window_manager_name);
      x11_screen->window_manager_name = g_strdup ("unknown");

      if (x11_screen->wmspec_check_window != None)
        {
          Atom type;
          gint format;
          gulong n_items;
          gulong bytes_after;
          gchar *name;

          name = NULL;

          cdk_x11_display_error_trap_push (display);

          XGetWindowProperty (CDK_DISPLAY_XDISPLAY (display),
                              x11_screen->wmspec_check_window,
                              cdk_x11_get_xatom_by_name_for_display (display,
                                                                     "_NET_WM_NAME"),
                              0, G_MAXLONG, False,
                              cdk_x11_get_xatom_by_name_for_display (display,
                                                                     "UTF8_STRING"),
                              &type, &format,
                              &n_items, &bytes_after,
                              (guchar **)&name);

          cdk_x11_display_error_trap_pop_ignored (display);

          if (name != NULL)
            {
              g_free (x11_screen->window_manager_name);
              x11_screen->window_manager_name = g_strdup (name);
              XFree (name);
            }
        }
    }

  return CDK_X11_SCREEN (screen)->window_manager_name;
}

static void
cdk_x11_screen_class_init (CdkX11ScreenClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CdkScreenClass *screen_class = CDK_SCREEN_CLASS (klass);

  object_class->dispose = cdk_x11_screen_dispose;
  object_class->finalize = cdk_x11_screen_finalize;

  screen_class->get_display = cdk_x11_screen_get_display;
  screen_class->get_width = cdk_x11_screen_get_width;
  screen_class->get_height = cdk_x11_screen_get_height;
  screen_class->get_width_mm = cdk_x11_screen_get_width_mm;
  screen_class->get_height_mm = cdk_x11_screen_get_height_mm;
  screen_class->get_number = cdk_x11_screen_get_number;
  screen_class->get_root_window = cdk_x11_screen_get_root_window;
  screen_class->get_system_visual = _cdk_x11_screen_get_system_visual;
  screen_class->get_rgba_visual = cdk_x11_screen_get_rgba_visual;
  screen_class->is_composited = cdk_x11_screen_is_composited;
  screen_class->make_display_name = cdk_x11_screen_make_display_name;
  screen_class->get_active_window = cdk_x11_screen_get_active_window;
  screen_class->get_window_stack = cdk_x11_screen_get_window_stack;
  screen_class->get_setting = cdk_x11_screen_get_setting;
  screen_class->visual_get_best_depth = _cdk_x11_screen_visual_get_best_depth;
  screen_class->visual_get_best_type = _cdk_x11_screen_visual_get_best_type;
  screen_class->visual_get_best = _cdk_x11_screen_visual_get_best;
  screen_class->visual_get_best_with_depth = _cdk_x11_screen_visual_get_best_with_depth;
  screen_class->visual_get_best_with_type = _cdk_x11_screen_visual_get_best_with_type;
  screen_class->visual_get_best_with_both = _cdk_x11_screen_visual_get_best_with_both;
  screen_class->query_depths = _cdk_x11_screen_query_depths;
  screen_class->query_visual_types = _cdk_x11_screen_query_visual_types;
  screen_class->list_visuals = _cdk_x11_screen_list_visuals;

  signals[WINDOW_MANAGER_CHANGED] =
    g_signal_new (g_intern_static_string ("window-manager-changed"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CdkX11ScreenClass, window_manager_changed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  0);
}

static guint32
get_netwm_cardinal_property (CdkScreen   *screen,
                             const gchar *name)
{
  CdkX11Screen *x11_screen = CDK_X11_SCREEN (screen);
  CdkAtom atom;
  guint32 prop = 0;
  Atom type;
  gint format;
  gulong nitems;
  gulong bytes_after;
  guchar *data;

  atom = cdk_atom_intern_static_string (name);

  if (!cdk_x11_screen_supports_net_wm_hint (screen, atom))
    return 0;

  XGetWindowProperty (x11_screen->xdisplay,
                      x11_screen->xroot_window,
                      cdk_x11_get_xatom_by_name_for_display (CDK_SCREEN_DISPLAY (screen), name),
                      0, G_MAXLONG,
                      False, XA_CARDINAL, &type, &format, &nitems,
                      &bytes_after, &data);
  if (type == XA_CARDINAL)
    {
      prop = *(gulong *)data;
      XFree (data);
    }

  return prop;
}

/**
 * cdk_x11_screen_get_number_of_desktops:
 * @screen: (type CdkX11Screen): a #CdkScreen
 *
 * Returns the number of workspaces for @screen when running under a
 * window manager that supports multiple workspaces, as described
 * in the
 * [Extended Window Manager Hints](http://www.freedesktop.org/Standards/wm-spec) specification.
 *
 * Returns: the number of workspaces, or 0 if workspaces are not supported
 *
 * Since: 3.10
 */
guint32
cdk_x11_screen_get_number_of_desktops (CdkScreen *screen)
{
  return get_netwm_cardinal_property (screen, "_NET_NUMBER_OF_DESKTOPS");
}

/**
 * cdk_x11_screen_get_current_desktop:
 * @screen: (type CdkX11Screen): a #CdkScreen
 *
 * Returns the current workspace for @screen when running under a
 * window manager that supports multiple workspaces, as described
 * in the
 * [Extended Window Manager Hints](http://www.freedesktop.org/Standards/wm-spec) specification.
 *
 * Returns: the current workspace, or 0 if workspaces are not supported
 *
 * Since: 3.10
 */
guint32
cdk_x11_screen_get_current_desktop (CdkScreen *screen)
{
  return get_netwm_cardinal_property (screen, "_NET_CURRENT_DESKTOP");
}
