/* CDK - The GIMP Drawing Kit
 * Copyright (C) 2002 Hans Breuer
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

#define _WIN32_WINNT 0x0600

#include "cdk.h"
#include "cdkprivate-win32.h"
#include "cdkscreenprivate.h"
#include "cdkwin32screen.h"
#include "cdkdisplayprivate.h"
#include "cdkvisualprivate.h"
#include "cdkdisplay-win32.h"
#include "cdkmonitor-win32.h"

#include <dwmapi.h>

struct _CdkWin32Screen
{
  CdkScreen parent_instance;

  CdkVisual *system_visual;
  CdkVisual *rgba_visual;
  gint available_visual_depths[1];
  CdkVisualType available_visual_types[1];

  CdkWindow *root_window;

  gint always_composited : 1;
};

struct _CdkWin32ScreenClass
{
  CdkScreenClass parent_class;
};

G_DEFINE_TYPE (CdkWin32Screen, cdk_win32_screen, CDK_TYPE_SCREEN)

static gint
get_color_precision (gulong mask)
{
  gint p = 0;

  while (mask & 0x1)
    {
      p++;
      mask >>= 1;
    }

  return p;
}

static CdkVisual *
init_visual (CdkScreen *screen,
             gboolean is_rgba)
{
  CdkVisual *visual;
  struct
  {
    BITMAPINFOHEADER bi;
    union
    {
      RGBQUAD colors[256];
      DWORD fields[256];
    } u;
  } bmi;
  HBITMAP hbm;

  const gint rastercaps = GetDeviceCaps (_cdk_display_hdc, RASTERCAPS);
  const int numcolors = GetDeviceCaps (_cdk_display_hdc, NUMCOLORS);
  gint bitspixel = GetDeviceCaps (_cdk_display_hdc, BITSPIXEL);
  gint map_entries = 0;

  visual = g_object_new (CDK_TYPE_VISUAL, NULL);
  visual->screen = screen;

  if (rastercaps & RC_PALETTE)
    {
      const int sizepalette = GetDeviceCaps (_cdk_display_hdc, SIZEPALETTE);
      gchar *max_colors = getenv ("CDK_WIN32_MAX_COLORS");
      visual->type = CDK_VISUAL_PSEUDO_COLOR;

      g_assert (sizepalette == 256);

      if (max_colors != NULL)
        _cdk_max_colors = atoi (max_colors);

      map_entries = _cdk_max_colors;

      if (map_entries >= 16 && map_entries < sizepalette)
        {
          if (map_entries < 32)
            {
              map_entries = 16;
              visual->type = CDK_VISUAL_STATIC_COLOR;
              bitspixel = 4;
            }
          else if (map_entries < 64)
            {
              map_entries = 32;
              bitspixel = 5;
            }
          else if (map_entries < 128)
            {
              map_entries = 64;
              bitspixel = 6;
            }
          else if (map_entries < 256)
            {
              map_entries = 128;
              bitspixel = 7;
            }
          else
            g_assert_not_reached ();
        }
      else
        map_entries = sizepalette;
    }
  else if (bitspixel == 1 && numcolors == 16)
    {
      bitspixel = 4;
      visual->type = CDK_VISUAL_STATIC_COLOR;
      map_entries = 16;
    }
  else if (bitspixel == 1)
    {
      visual->type = CDK_VISUAL_STATIC_GRAY;
      map_entries = 2;
    }
  else if (bitspixel == 4)
    {
      visual->type = CDK_VISUAL_STATIC_COLOR;
      map_entries = 16;
    }
  else if (bitspixel == 8)
    {
      visual->type = CDK_VISUAL_STATIC_COLOR;
      map_entries = 256;
    }
  else if (bitspixel == 16)
    {
      visual->type = CDK_VISUAL_TRUE_COLOR;
#if 1
      /* This code by Mike Enright,
       * see http://www.users.cts.com/sd/m/menright/display.html
       */
      memset (&bmi, 0, sizeof (bmi));
      bmi.bi.biSize = sizeof (bmi.bi);

      hbm = CreateCompatibleBitmap (_cdk_display_hdc, 1, 1);
      GetDIBits (_cdk_display_hdc, hbm, 0, 1, NULL,
                 (BITMAPINFO *) &bmi, DIB_RGB_COLORS);
      GetDIBits (_cdk_display_hdc, hbm, 0, 1, NULL,
                 (BITMAPINFO *) &bmi, DIB_RGB_COLORS);
      DeleteObject (hbm);

      if (bmi.bi.biCompression != BI_BITFIELDS)
        {
          /* Either BI_RGB or BI_RLE_something
           * .... or perhaps (!!) something else.
           * Theoretically biCompression might be
           * mmioFourCC('c','v','i','d') but I doubt it.
           */
          if (bmi.bi.biCompression == BI_RGB)
            {
              /* It's 555 */
              bitspixel = 15;
              visual->red_mask   = 0x00007C00;
              visual->green_mask = 0x000003E0;
              visual->blue_mask  = 0x0000001F;
            }
          else
            {
              g_assert_not_reached ();
            }
        }
      else
        {
          DWORD allmasks =
            bmi.u.fields[0] | bmi.u.fields[1] | bmi.u.fields[2];
          int k = 0;
          while (allmasks)
            {
              if (allmasks&1)
                k++;
              allmasks/=2;
            }
          bitspixel = k;
          visual->red_mask = bmi.u.fields[0];
          visual->green_mask = bmi.u.fields[1];
          visual->blue_mask  = bmi.u.fields[2];
        }
#else
      /* Old, incorrect (but still working) code. */
#if 0
      visual->red_mask   = 0x0000F800;
      visual->green_mask = 0x000007E0;
      visual->blue_mask  = 0x0000001F;
#else
      visual->red_mask   = 0x00007C00;
      visual->green_mask = 0x000003E0;
      visual->blue_mask  = 0x0000001F;
#endif
#endif
    }
  else if (bitspixel == 24 || bitspixel == 32)
    {
      if (!is_rgba)
        bitspixel = 24;
      visual->type = CDK_VISUAL_TRUE_COLOR;
      visual->red_mask   = 0x00FF0000;
      visual->green_mask = 0x0000FF00;
      visual->blue_mask  = 0x000000FF;
    }
  else
    g_error ("_cdk_visual_init: unsupported BITSPIXEL: %d\n", bitspixel);

  visual->depth = bitspixel;
  visual->byte_order = CDK_LSB_FIRST;
  visual->bits_per_rgb = 42; /* Not used? */

  if ((visual->type != CDK_VISUAL_TRUE_COLOR) &&
      (visual->type != CDK_VISUAL_DIRECT_COLOR))
    {
      visual->red_mask = 0;
      visual->green_mask = 0;
      visual->blue_mask = 0;
    }
  else
    map_entries = 1 << (MAX (get_color_precision (visual->red_mask),
                             MAX (get_color_precision (visual->green_mask),
                                  get_color_precision (visual->blue_mask))));

  visual->colormap_size = map_entries;

  return visual;
}

static gboolean
init_root_window_size (CdkWin32Screen *screen)
{
  CdkRectangle result;
  int i;
  CdkDisplay *display = _cdk_display;
  int monitor_count;
  CdkMonitor *monitor;
  gboolean changed;
  CdkWindowImplWin32 *root_impl;

  monitor_count = cdk_display_get_n_monitors (display);
  monitor = cdk_display_get_monitor (display, 0);
  cdk_monitor_get_geometry (monitor, &result);

  for (i = 1; i < monitor_count; i++)
  {
    CdkRectangle rect;

    monitor = cdk_display_get_monitor (display, i);
    cdk_monitor_get_geometry (monitor, &rect);
    cdk_rectangle_union (&result, &rect, &result);
  }

  changed = screen->root_window->width != result.width ||
            screen->root_window->height != result.height;
  screen->root_window->width = result.width;
  screen->root_window->height = result.height;
  root_impl = CDK_WINDOW_IMPL_WIN32 (screen->root_window->impl);

  root_impl->unscaled_width = result.width * root_impl->window_scale;
  root_impl->unscaled_height = result.height * root_impl->window_scale;

  return changed;
}

static gboolean
init_root_window (CdkWin32Screen *screen_win32)
{
  CdkScreen *screen;
  CdkWindow *window;
  CdkWindowImplWin32 *impl_win32;
  gboolean changed;
  CdkWin32Display *win32_display;

  screen = CDK_SCREEN (screen_win32);

  g_assert (screen_win32->root_window == NULL);

  window = _cdk_display_create_window (_cdk_display);
  window->impl = g_object_new (CDK_TYPE_WINDOW_IMPL_WIN32, NULL);
  impl_win32 = CDK_WINDOW_IMPL_WIN32 (window->impl);
  impl_win32->wrapper = window;

  window->impl_window = window;
  window->visual = cdk_screen_get_system_visual (screen);

  window->window_type = CDK_WINDOW_ROOT;
  window->depth = window->visual->depth;

  screen_win32->root_window = window;

  changed = init_root_window_size (screen_win32);

  window->x = 0;
  window->y = 0;
  window->abs_x = 0;
  window->abs_y = 0;
  /* width and height already initialised in init_root_window_size() */
  window->viewable = TRUE;
  win32_display = CDK_WIN32_DISPLAY (_cdk_display);

  if (win32_display->dpi_aware_type != PROCESS_DPI_UNAWARE)
    impl_win32->window_scale = _cdk_win32_display_get_monitor_scale_factor (win32_display,
                                                                            NULL,
                                                                            impl_win32->handle,
                                                                            NULL);
  else
    impl_win32->window_scale = 1;

  impl_win32->unscaled_width = window->width * impl_win32->window_scale;
  impl_win32->unscaled_height = window->height * impl_win32->window_scale;

  cdk_win32_handle_table_insert ((HANDLE *) &impl_win32->handle, window);

  CDK_NOTE (MISC, g_print ("screen->root_window=%p\n", window));

  return changed;
}

static void
cdk_win32_screen_init (CdkWin32Screen *win32_screen)
{
  CdkScreen *screen = CDK_SCREEN (win32_screen);
  _cdk_win32_screen_set_font_resolution (win32_screen);

  win32_screen->system_visual = init_visual (screen, FALSE);
  win32_screen->rgba_visual = init_visual (screen, TRUE);

  win32_screen->available_visual_depths[0] = win32_screen->rgba_visual->depth;
  win32_screen->available_visual_types[0] = win32_screen->rgba_visual->type;

  _cdk_win32_display_init_monitors (CDK_WIN32_DISPLAY (_cdk_display));
  init_root_window (win32_screen);

  /* On Windows 8 and later, DWM (composition) is always enabled */
  win32_screen->always_composited = g_win32_check_windows_version (6, 2, 0, G_WIN32_OS_ANY);
}

void
_cdk_win32_screen_on_displaychange_event (CdkWin32Screen *screen)
{
  gboolean monitors_changed;

  monitors_changed = _cdk_win32_display_init_monitors (CDK_WIN32_DISPLAY (_cdk_display));

  if (init_root_window_size (screen))
    g_signal_emit_by_name (screen, "size-changed");

  if (monitors_changed)
    g_signal_emit_by_name (screen, "monitors-changed");
}

void
_cdk_win32_screen_set_font_resolution (CdkWin32Screen *win32_screen)
{
  CdkScreen *screen = CDK_SCREEN (win32_screen);
  int logpixelsx = -1;
  const gchar *font_resolution;

  font_resolution = g_getenv ("CDK_WIN32_FONT_RESOLUTION");
  if (font_resolution)
    {
      int env_logpixelsx = atol (font_resolution);
      if (env_logpixelsx > 0)
        logpixelsx = env_logpixelsx;
    }
  else
    {
      gint dpi = -1;
      CdkWin32Display *win32_display = CDK_WIN32_DISPLAY (cdk_screen_get_display (screen));
      guint scale = _cdk_win32_display_get_monitor_scale_factor (win32_display, NULL, NULL, &dpi);

      /* If we have a scale that is at least 2, don't scale up the fonts */
      if (scale >= 2)
        logpixelsx = USER_DEFAULT_SCREEN_DPI;
      else
        logpixelsx = dpi;
    }

  if (logpixelsx > 0)
    _cdk_screen_set_resolution (screen, logpixelsx);
}

static CdkDisplay *
cdk_win32_screen_get_display (CdkScreen *screen)
{
  return _cdk_display;
}

static gint
cdk_win32_screen_get_width (CdkScreen *screen)
{
  return CDK_WIN32_SCREEN (screen)->root_window->width;
}

static gint
cdk_win32_screen_get_height (CdkScreen *screen)
{
  return CDK_WIN32_SCREEN (screen)->root_window->height;
}

static gint
cdk_win32_screen_get_width_mm (CdkScreen *screen)
{
  return (double) cdk_screen_get_width (screen) / GetDeviceCaps (_cdk_display_hdc, LOGPIXELSX) * 25.4;
}

static gint
cdk_win32_screen_get_height_mm (CdkScreen *screen)
{
  return (double) cdk_screen_get_height (screen) / GetDeviceCaps (_cdk_display_hdc, LOGPIXELSY) * 25.4;
}

static CdkWindow *
cdk_win32_screen_get_root_window (CdkScreen *screen)
{
  return CDK_WIN32_SCREEN (screen)->root_window;
}

static gint
cdk_win32_screen_get_number (CdkScreen *screen)
{
  g_return_val_if_fail (screen == cdk_display_get_default_screen (cdk_display_get_default ()), 0);

  return 0;
}

static gchar *
cdk_win32_screen_make_display_name (CdkScreen *screen)
{
  return g_strdup (cdk_display_get_name (_cdk_display));
}

static CdkWindow *
cdk_win32_screen_get_active_window (CdkScreen *screen)
{
  g_return_val_if_fail (CDK_IS_SCREEN (screen), NULL);

  return NULL;
}

static GList *
cdk_win32_screen_get_window_stack (CdkScreen *screen)
{
  g_return_val_if_fail (CDK_IS_SCREEN (screen), NULL);

  return NULL;
}

static gboolean
cdk_win32_screen_is_composited (CdkScreen *screen)
{
  if (CDK_WIN32_SCREEN (screen)->always_composited)
    return TRUE;
  else
    {
      gboolean is_composited;

      if (DwmIsCompositionEnabled (&is_composited) != S_OK)
        return FALSE;
      return is_composited;
    }
}

static gint
cdk_win32_screen_visual_get_best_depth (CdkScreen *screen)
{
  return CDK_WIN32_SCREEN (screen)->available_visual_depths[0];
}

static CdkVisualType
cdk_win32_screen_visual_get_best_type (CdkScreen *screen)
{
  return CDK_WIN32_SCREEN (screen)->available_visual_types[0];
}

static CdkVisual *
cdk_win32_screen_get_system_visual (CdkScreen *screen)
{
  return CDK_WIN32_SCREEN (screen)->system_visual;
}

static CdkVisual *
cdk_win32_screen_get_rgba_visual (CdkScreen *screen)
{
  return CDK_WIN32_SCREEN (screen)->rgba_visual;
}

static CdkVisual*
cdk_win32_screen_visual_get_best (CdkScreen *screen)
{
  return CDK_WIN32_SCREEN (screen)->rgba_visual;
}

static CdkVisual *
cdk_win32_screen_visual_get_best_with_depth (CdkScreen *screen,
                                             gint       depth)
{
  CdkWin32Screen *win32_screen = CDK_WIN32_SCREEN (screen);

  if (depth == win32_screen->rgba_visual->depth)
    return win32_screen->rgba_visual;
  else if (depth == win32_screen->system_visual->depth)
    return win32_screen->system_visual;

  return NULL;
}

static CdkVisual *
cdk_win32_screen_visual_get_best_with_type (CdkScreen     *screen,
                                            CdkVisualType  visual_type)
{
  CdkWin32Screen *win32_screen = CDK_WIN32_SCREEN (screen);

  if (visual_type == win32_screen->rgba_visual->type)
    return win32_screen->rgba_visual;
  else if (visual_type == win32_screen->system_visual->type)
    return win32_screen->system_visual;

  return NULL;
}

static CdkVisual *
cdk_win32_screen_visual_get_best_with_both (CdkScreen     *screen,
                                            gint           depth,
                                            CdkVisualType  visual_type)
{
  CdkWin32Screen *win32_screen = CDK_WIN32_SCREEN (screen);

  if ((depth == win32_screen->rgba_visual->depth) && (visual_type == win32_screen->rgba_visual->type))
    return win32_screen->rgba_visual;
  else if ((depth == win32_screen->system_visual->depth) && (visual_type == win32_screen->system_visual->type))
    return win32_screen->system_visual;

  return NULL;
}

static void
cdk_win32_screen_query_depths (CdkScreen  *screen,
                               gint      **depths,
                               gint       *count)
{
  *count = 1;
  *depths = CDK_WIN32_SCREEN (screen)->available_visual_depths;
}

static void
cdk_win32_screen_query_visual_types (CdkScreen      *screen,
                                     CdkVisualType **visual_types,
                                     gint           *count)
{
  *count = 1;
  *visual_types = CDK_WIN32_SCREEN (screen)->available_visual_types;
}

static GList *
cdk_win32_screen_list_visuals (CdkScreen *screen)
{
  CdkWin32Screen *win32_screen = CDK_WIN32_SCREEN (screen);
  GList *result = NULL;

  result = g_list_append (result, win32_screen->rgba_visual);
  result = g_list_append (result, win32_screen->system_visual);

  return result;
}

static void
cdk_win32_screen_finalize (GObject *object)
{
  G_OBJECT_CLASS (cdk_win32_screen_parent_class)->finalize (object);
}

static void
cdk_win32_screen_class_init (CdkWin32ScreenClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CdkScreenClass *screen_class = CDK_SCREEN_CLASS (klass);

  object_class->finalize = cdk_win32_screen_finalize;

  screen_class->get_display = cdk_win32_screen_get_display;
  screen_class->get_width = cdk_win32_screen_get_width;
  screen_class->get_height = cdk_win32_screen_get_height;
  screen_class->get_width_mm = cdk_win32_screen_get_width_mm;
  screen_class->get_height_mm = cdk_win32_screen_get_height_mm;
  screen_class->get_number = cdk_win32_screen_get_number;
  screen_class->get_root_window = cdk_win32_screen_get_root_window;
  screen_class->is_composited = cdk_win32_screen_is_composited;
  screen_class->make_display_name = cdk_win32_screen_make_display_name;
  screen_class->get_active_window = cdk_win32_screen_get_active_window;
  screen_class->get_window_stack = cdk_win32_screen_get_window_stack;
  screen_class->get_setting = _cdk_win32_screen_get_setting;
  screen_class->get_system_visual = cdk_win32_screen_get_system_visual;
  screen_class->get_rgba_visual = cdk_win32_screen_get_rgba_visual;
  screen_class->visual_get_best_depth = cdk_win32_screen_visual_get_best_depth;
  screen_class->visual_get_best_type = cdk_win32_screen_visual_get_best_type;
  screen_class->visual_get_best = cdk_win32_screen_visual_get_best;
  screen_class->visual_get_best_with_depth = cdk_win32_screen_visual_get_best_with_depth;
  screen_class->visual_get_best_with_type = cdk_win32_screen_visual_get_best_with_type;
  screen_class->visual_get_best_with_both = cdk_win32_screen_visual_get_best_with_both;
  screen_class->query_depths = cdk_win32_screen_query_depths;
  screen_class->query_visual_types = cdk_win32_screen_query_visual_types;
  screen_class->list_visuals = cdk_win32_screen_list_visuals;
}
