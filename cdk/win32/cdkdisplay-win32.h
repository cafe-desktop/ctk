/*
 * cdkdisplay-win32.h
 *
 * Copyright 2014 Chun-wei Fan <fanc999@yahoo.com.tw>
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

#include "cdkdisplayprivate.h"

#ifdef CDK_WIN32_ENABLE_EGL
# include <epoxy/egl.h>
#endif

#ifndef __CDK_DISPLAY__WIN32_H__
#define __CDK_DISPLAY__WIN32_H__

/* Define values used to set DPI-awareness */
typedef enum _CdkWin32ProcessDpiAwareness {
  PROCESS_DPI_UNAWARE = 0,
  PROCESS_SYSTEM_DPI_AWARE = 1,
  PROCESS_PER_MONITOR_DPI_AWARE = 2
} CdkWin32ProcessDpiAwareness;

/* APIs from shcore.dll */
typedef HRESULT (WINAPI *funcSetProcessDpiAwareness) (CdkWin32ProcessDpiAwareness value);
typedef HRESULT (WINAPI *funcGetProcessDpiAwareness) (HANDLE                       handle,
                                                      CdkWin32ProcessDpiAwareness *awareness);
typedef HRESULT (WINAPI *funcGetDpiForMonitor)       (HMONITOR                monitor,
                                                      CdkWin32MonitorDpiType  dpi_type,
                                                      UINT                   *dpi_x,
                                                      UINT                   *dpi_y);

typedef struct _CdkWin32ShcoreFuncs
{
  HMODULE hshcore;
  funcSetProcessDpiAwareness setDpiAwareFunc;
  funcGetProcessDpiAwareness getDpiAwareFunc;
  funcGetDpiForMonitor getDpiForMonitorFunc;
} CdkWin32ShcoreFuncs;

/* DPI awareness APIs from user32.dll */
typedef BOOL (WINAPI *funcSetProcessDPIAware) (void);
typedef BOOL (WINAPI *funcIsProcessDPIAware)  (void);

typedef struct _CdkWin32User32DPIFuncs
{
  funcSetProcessDPIAware setDpiAwareFunc;
  funcIsProcessDPIAware isDpiAwareFunc;
} CdkWin32User32DPIFuncs;

/* Detect running architecture */
typedef BOOL (WINAPI *funcIsWow64Process2) (HANDLE, USHORT *, USHORT *);
typedef struct _CdkWin32KernelCPUFuncs
{
  funcIsWow64Process2 isWow64Process2;
} CdkWin32KernelCPUFuncs;

struct _CdkWin32Display
{
  CdkDisplay display;

  CdkScreen *screen;

  Win32CursorTheme *cursor_theme;
  gchar *cursor_theme_name;
  int cursor_theme_size;
  GHashTable *cursor_cache;

  HWND hwnd;
  HWND clipboard_hwnd;

  /* WGL/OpenGL Items */
  guint have_wgl : 1;
  guint gl_version;
  HWND gl_hwnd;

#ifdef CDK_WIN32_ENABLE_EGL
  /* EGL (Angle) Items */
  guint have_egl : 1;
  guint egl_version;
  EGLDisplay egl_disp;
  HDC hdc_egl_temp;
#endif

  GPtrArray *monitors;

  guint hasWglARBCreateContext : 1;
  guint hasWglEXTSwapControl : 1;
  guint hasWglOMLSyncControl : 1;
  guint hasWglARBPixelFormat : 1;
  guint hasWglARBmultisample : 1;

#ifdef CDK_WIN32_ENABLE_EGL
  guint hasEglKHRCreateContext : 1;
  guint hasEglSurfacelessContext : 1;
  EGLint egl_min_swap_interval;
#endif

  /* HiDPI Items */
  guint have_at_least_win81 : 1;
  CdkWin32ProcessDpiAwareness dpi_aware_type;
  guint has_fixed_scale : 1;
  guint window_scale;

  CdkWin32ShcoreFuncs shcore_funcs;
  CdkWin32User32DPIFuncs user32_dpi_funcs;

  /* Running CPU items */
  guint running_on_arm64 : 1;
  CdkWin32KernelCPUFuncs cpu_funcs;
};

struct _CdkWin32DisplayClass
{
  CdkDisplayClass display_class;
};

gboolean   _cdk_win32_display_init_monitors    (CdkWin32Display *display);

GPtrArray *_cdk_win32_display_get_monitor_list (CdkWin32Display *display);

guint      _cdk_win32_display_get_monitor_scale_factor (CdkWin32Display *win32_display,
                                                        HMONITOR         hmonitor,
                                                        HWND             hwnd,
                                                        gint             *dpi);
#endif /* __CDK_DISPLAY__WIN32_H__ */
