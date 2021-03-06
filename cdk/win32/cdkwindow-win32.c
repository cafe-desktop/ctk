/* CDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 1998-2004 Tor Lillqvist
 * Copyright (C) 2001-2011 Hans Breuer
 * Copyright (C) 2007-2009 Cody Russell
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
#include <stdlib.h>

#include "cdk.h"
#include "cdkwindowimpl.h"
#include "cdkprivate-win32.h"
#include "cdkdeviceprivate.h"
#include "cdkdevicemanager-win32.h"
#include "cdkenumtypes.h"
#include "cdkwin32.h"
#include "cdkdisplayprivate.h"
#include "cdkvisualprivate.h"
#include "cdkmonitorprivate.h"
#include "cdkwin32window.h"
#include "cdkglcontext-win32.h"
#include "cdkdisplay-win32.h"

#include <cairo-win32.h>
#include <dwmapi.h>
#include <math.h>
#include "fallback-c89.c"

static void cdk_window_impl_win32_init       (CdkWindowImplWin32      *window);
static void cdk_window_impl_win32_class_init (CdkWindowImplWin32Class *klass);
static void cdk_window_impl_win32_finalize   (GObject                 *object);

static gpointer parent_class = NULL;
static GSList *modal_window_stack = NULL;

static const cairo_user_data_key_t cdk_win32_cairo_key;
typedef struct _FullscreenInfo FullscreenInfo;

struct _FullscreenInfo
{
  RECT  r;
  guint hint_flags;
  LONG  style;
};

struct _AeroSnapEdgeRegion
{
  /* The rectangle along the edge of the desktop
   * that allows application of the snap transformation.
   */
  CdkRectangle edge;

  /* A subregion of the "edge". When the pointer hits
   * this region, the transformation is revealed.
   * Usually it is 1-pixel thick and is located at the
   * very edge of the screen. When there's a toolbar
   * at that edge, the "trigger" and the "edge" regions
   * are extended to cover that toolbar.
   */
  CdkRectangle trigger;
};

typedef struct _AeroSnapEdgeRegion AeroSnapEdgeRegion;

/* Use this for hWndInsertAfter (2nd argument to SetWindowPos()) if
 * SWP_NOZORDER flag is used. Otherwise it's unobvious why a particular
 * argument is used. Using NULL is misleading, because
 * NULL is equivalent to HWND_TOP.
 */
#define SWP_NOZORDER_SPECIFIED HWND_TOP

/* Size of the regions at the edges of the desktop where
 * snapping can take place (in pixels)
 */
#define AEROSNAP_REGION_THICKNESS (20)
/* Size of the subregions that actually trigger the snapping prompt
 * (in pixels).
 */
#define AEROSNAP_REGION_TRIGGER_THICKNESS (1)

/* The gap between the snap indicator and the edge of the work area
 * (in pixels).
 */
#define AEROSNAP_INDICATOR_EDGE_GAP (10)

/* Width of the outline of the snap indicator
 * (in pixels).
 */
#define AEROSNAP_INDICATOR_LINE_WIDTH (3.0)

/* Corner radius of the snap indicator.
 */
#define AEROSNAP_INDICATOR_CORNER_RADIUS (3.0)

/* The time it takes for snap indicator to expand/shrink
 * from current window size to future position of the
 * snapped window (in microseconds).
 */
#define AEROSNAP_INDICATOR_ANIMATION_DURATION (200 * 1000)

/* Opacity if the snap indicator. */
#define AEROSNAP_INDICATOR_OPACITY (0.5)

/* The interval between snap indicator redraws (in milliseconds).
 * 16 is ~ 1/60 of a second, for ~60 FPS.
 */
#define AEROSNAP_INDICATOR_ANIMATION_TICK (16)

static gboolean _cdk_window_get_functions (CdkWindow         *window,
                                           CdkWMFunction     *functions);
static HDC     _cdk_win32_impl_acquire_dc (CdkWindowImplWin32 *impl);
static void    _cdk_win32_impl_release_dc (CdkWindowImplWin32 *impl);

#define WINDOW_IS_TOPLEVEL(window)		   \
  (CDK_WINDOW_TYPE (window) != CDK_WINDOW_CHILD && \
   CDK_WINDOW_TYPE (window) != CDK_WINDOW_FOREIGN && \
   CDK_WINDOW_TYPE (window) != CDK_WINDOW_OFFSCREEN)

CdkScreen *
CDK_WINDOW_SCREEN (GObject *win)
{
  return cdk_display_get_default_screen (cdk_display_get_default ());
}

struct _CdkWin32Window {
  CdkWindow parent;
};

struct _CdkWin32WindowClass {
  CdkWindowClass parent_class;
};

G_DEFINE_TYPE (CdkWin32Window, cdk_win32_window, CDK_TYPE_WINDOW)

static void
cdk_win32_window_class_init (CdkWin32WindowClass *window_class)
{
}

static void
cdk_win32_window_init (CdkWin32Window *window)
{
}


G_DEFINE_TYPE (CdkWindowImplWin32, cdk_window_impl_win32, CDK_TYPE_WINDOW_IMPL)

GType
_cdk_window_impl_win32_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      const GTypeInfo object_info =
      {
        sizeof (CdkWindowImplWin32Class),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) cdk_window_impl_win32_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (CdkWindowImplWin32),
        0,              /* n_preallocs */
        (GInstanceInitFunc) cdk_window_impl_win32_init,
      };

      object_type = g_type_register_static (CDK_TYPE_WINDOW_IMPL,
                                            "CdkWindowImplWin32",
                                            &object_info, 0);
    }

  return object_type;
}

static void
cdk_window_impl_win32_init (CdkWindowImplWin32 *impl)
{
  CdkDisplay *display = cdk_display_get_default ();

  impl->toplevel_window_type = -1;
  impl->cursor = NULL;
  impl->hicon_big = NULL;
  impl->hicon_small = NULL;
  impl->hint_flags = 0;
  impl->type_hint = CDK_WINDOW_TYPE_HINT_NORMAL;
  impl->transient_owner = NULL;
  impl->transient_children = NULL;
  impl->num_transients = 0;
  impl->changing_state = FALSE;
  impl->window_scale = 1;

  if (display != NULL)
    /* Replace WM-defined default cursor with the default cursor
     * from our theme. Otherwise newly-opened windows (such as popup
     * menus of all kinds) will have WM-default cursor when they are
     * first shown, which will be replaced by our cursor only later on.
     */
    impl->cursor = _cdk_win32_display_get_cursor_for_type (display, CDK_LEFT_PTR);
}

static void
cdk_window_impl_win32_finalize (GObject *object)
{
  CdkWindow *wrapper;
  CdkWindowImplWin32 *window_impl;

  g_return_if_fail (CDK_IS_WINDOW_IMPL_WIN32 (object));

  window_impl = CDK_WINDOW_IMPL_WIN32 (object);

  wrapper = window_impl->wrapper;

  if (!CDK_WINDOW_DESTROYED (wrapper))
    {
      cdk_win32_handle_table_remove (window_impl->handle);
    }

  g_clear_object (&window_impl->cursor);

  g_clear_pointer (&window_impl->snap_stash, g_free);
  g_clear_pointer (&window_impl->snap_stash_int, g_free);

  if (window_impl->hicon_big != NULL)
    {
      GDI_CALL (DestroyIcon, (window_impl->hicon_big));
      window_impl->hicon_big = NULL;
    }

  if (window_impl->hicon_small != NULL)
    {
      GDI_CALL (DestroyIcon, (window_impl->hicon_small));
      window_impl->hicon_small = NULL;
    }

  g_free (window_impl->decorations);

  if (window_impl->cache_surface)
    {
      cairo_surface_destroy (window_impl->cache_surface);
      window_impl->cache_surface = NULL;
    }

  if (window_impl->cairo_surface)
    {
      cairo_surface_destroy (window_impl->cairo_surface);
      window_impl->cairo_surface = NULL;
    }

  g_assert (window_impl->transient_owner == NULL);
  g_assert (window_impl->transient_children == NULL);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
cdk_win32_get_window_client_area_rect (CdkWindow *window,
                                       gint       scale,
                                       RECT      *rect)
{
  gint x, y, width, height;

  cdk_window_get_position (window, &x, &y);
  width = cdk_window_get_width (window);
  height = cdk_window_get_height (window);
  rect->left = x * scale;
  rect->top = y * scale;
  rect->right = rect->left + width * scale;
  rect->bottom = rect->top + height * scale;
}

static void
cdk_win32_window_get_queued_window_rect (CdkWindow *window,
                                         RECT      *return_window_rect)
{
  RECT window_rect;
  CdkWindowImplWin32 *impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  cdk_win32_get_window_client_area_rect (window, impl->window_scale, &window_rect);

  /* Turn client area into window area */
  _cdk_win32_adjust_client_rect (window, &window_rect);

  /* Convert CDK screen coordinates to W32 desktop coordinates */
  window_rect.left -= _cdk_offset_x * impl->window_scale;
  window_rect.right -= _cdk_offset_x * impl->window_scale;
  window_rect.top -= _cdk_offset_y * impl->window_scale;
  window_rect.bottom -= _cdk_offset_y * impl->window_scale;

  *return_window_rect = window_rect;
}

static void
cdk_win32_window_apply_queued_move_resize (CdkWindow *window,
                                           RECT       window_rect)
{
  if (!IsIconic (CDK_WINDOW_HWND (window)))
    {
      CdkWindowImplWin32 *impl = CDK_WINDOW_IMPL_WIN32 (window->impl);
      CDK_NOTE (EVENTS, g_print ("Setting window position ... "));

      API_CALL (SetWindowPos, (CDK_WINDOW_HWND (window),
                               SWP_NOZORDER_SPECIFIED,
                               window_rect.left, window_rect.top,
                               window_rect.right - window_rect.left,
                               window_rect.bottom - window_rect.top,
                               SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOREDRAW));

      CDK_NOTE (EVENTS, g_print (" ... set window position\n"));

      return;
    }

  /* Don't move iconic windows */
  /* TODO: use SetWindowPlacement() to change non-minimized window position */
}

static gboolean
cdk_win32_window_begin_paint (CdkWindow *window)
{
  CdkWindowImplWin32 *impl;
  RECT window_rect;

  if (window == NULL || CDK_WINDOW_DESTROYED (window))
    return TRUE;

  impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  /* Layered windows are moved *after* repaint.
   * We supply our own surface, return FALSE to make CDK use it.
   */
  if (impl->layered)
    return FALSE;

  /* Non-GL windows are moved *after* repaint.
   * We don't supply our own surface, return TRUE to make CDK create
   * one by itself.
   */
  if (!window->current_paint.use_gl)
    return TRUE;

  /* GL windows are moved *before* repaint (otherwise
   * repainting doesn't work), but if there's no move queued up,
   * return immediately. Doesn't matter what we return, CDK
   * will create a surface anyway, as if we returned TRUE.
   */
  if (!impl->drag_move_resize_context.native_move_resize_pending)
    return TRUE;

  impl->drag_move_resize_context.native_move_resize_pending = FALSE;

  /* Get the position/size of the window that CDK wants,
   * apply it.
   */
  cdk_win32_window_get_queued_window_rect (window, &window_rect);
  cdk_win32_window_apply_queued_move_resize (window, window_rect);

  return TRUE;
}

static void
cdk_win32_window_end_paint (CdkWindow *window)
{
  CdkWindowImplWin32 *impl;
  RECT window_rect;
  HDC hdc;
  POINT window_position;
  SIZE window_size;
  POINT source_point;
  BLENDFUNCTION blender;
  cairo_t *cr;

  if (window == NULL || CDK_WINDOW_DESTROYED (window))
    return;

  impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  /* GL windows are moved *before* repaint */
  if (window->current_paint.use_gl)
    return;

  /* No move/resize is queued up, and we don't need to update
   * the contents of a layered window, so return immediately.
   */
  if (!impl->layered &&
      !impl->drag_move_resize_context.native_move_resize_pending)
    return;

  impl->drag_move_resize_context.native_move_resize_pending = FALSE;

  /* Get the position/size of the window that CDK wants. */
  cdk_win32_window_get_queued_window_rect (window, &window_rect);

  if (!impl->layered)
    {
      cdk_win32_window_apply_queued_move_resize (window, window_rect);

      return;
    }

  window_position.x = window_rect.left;
  window_position.y = window_rect.top;

  window_size.cx = window_rect.right - window_rect.left;
  window_size.cy = window_rect.bottom - window_rect.top;

  cairo_surface_flush (impl->cairo_surface);

  /* we always draw in the top-left corner of the surface */
  source_point.x = source_point.y = 0;

  blender.BlendOp = AC_SRC_OVER;
  blender.BlendFlags = 0;
  blender.AlphaFormat = AC_SRC_ALPHA;
  blender.SourceConstantAlpha = impl->layered_opacity * 255;

  /* Update cache surface contents */
  cr = cairo_create (impl->cache_surface);

  cairo_set_source_surface (cr, window->current_paint.surface, 0, 0);
  cdk_cairo_region (cr, window->current_paint.region);
  cairo_clip (cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_paint (cr);

  cairo_destroy (cr);

  cairo_surface_flush (impl->cache_surface);
  hdc = cairo_win32_surface_get_dc (impl->cache_surface);

  /* Don't use UpdateLayeredWindow on minimized windows */
  if (IsIconic (CDK_WINDOW_HWND (window)))
    {
      cdk_win32_window_apply_queued_move_resize (window, window_rect);

      return;
    }

  /* Move, resize and redraw layered window in one call */
  API_CALL (UpdateLayeredWindow, (CDK_WINDOW_HWND (window), NULL,
                                  &window_position, &window_size,
                                  hdc, &source_point,
                                  0, &blender, ULW_ALPHA));
}

void
_cdk_win32_adjust_client_rect (CdkWindow *window,
			       RECT      *rect)
{
  LONG style, exstyle;

  style = GetWindowLong (CDK_WINDOW_HWND (window), GWL_STYLE);
  exstyle = GetWindowLong (CDK_WINDOW_HWND (window), GWL_EXSTYLE);
  API_CALL (AdjustWindowRectEx, (rect, style, FALSE, exstyle));
}

gboolean
_cdk_win32_window_enable_transparency (CdkWindow *window)
{
  CdkWindowImplWin32 *impl;
  CdkScreen *screen;
  DWM_BLURBEHIND blur_behind;
  HRGN empty_region;
  HRESULT call_result;
  HWND parent, thiswindow;

  if (window == NULL || CDK_WINDOW_HWND (window) == NULL)
    return FALSE;

  impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  /* layered windows don't need blurbehind for transparency */
  if (impl->layered)
    return TRUE;

  screen = cdk_window_get_screen (window);

  if (!cdk_screen_is_composited (screen))
    return FALSE;

  if (window == cdk_screen_get_root_window (screen))
    return FALSE;

  thiswindow = CDK_WINDOW_HWND (window);

  /* Blurbehind only works on toplevel windows */
  parent = GetAncestor (thiswindow, GA_PARENT);
  if (!(GetWindowLong (thiswindow, GWL_STYLE) & WS_POPUP) &&
      (parent == NULL || parent != GetDesktopWindow ()))
    return FALSE;

  empty_region = CreateRectRgn (0, 0, -1, -1);

  if (empty_region == NULL)
    return FALSE;

  memset (&blur_behind, 0, sizeof (blur_behind));
  blur_behind.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
  blur_behind.hRgnBlur = empty_region;
  blur_behind.fEnable = TRUE;
  call_result = DwmEnableBlurBehindWindow (thiswindow, &blur_behind);

  if (!SUCCEEDED (call_result))
    g_warning ("%s: %s (%p) failed: %" G_GINT32_MODIFIER "x",
        G_STRLOC, "DwmEnableBlurBehindWindow", thiswindow, (guint32) call_result);

  DeleteObject (empty_region);

  return SUCCEEDED (call_result);
}

static const gchar *
get_default_title (void)
{
  const char *title;
  title = g_get_application_name ();
  if (!title)
    title = g_get_prgname ();

  return title;
}

/* RegisterCdkClass
 *   is a wrapper function for RegisterWindowClassEx.
 *   It creates at least one unique class for every
 *   CdkWindowType. If support for single window-specific icons
 *   is ever needed (e.g Dialog specific), every such window should
 *   get its own class
 */
static ATOM
RegisterCdkClass (CdkWindowType wtype, CdkWindowTypeHint wtype_hint)
{
  static ATOM klassTOPLEVEL   = 0;
  static ATOM klassCHILD      = 0;
  static ATOM klassTEMP       = 0;
  static ATOM klassTEMPSHADOW = 0;
  static HICON hAppIcon = NULL;
  static HICON hAppIconSm = NULL;
  static WNDCLASSEXW wcl;
  ATOM klass = 0;

  wcl.cbSize = sizeof (WNDCLASSEX);
  wcl.style = 0; /* DON'T set CS_<H,V>REDRAW. It causes total redraw
                  * on WM_SIZE and WM_MOVE. Flicker, Performance!
                  */
  wcl.lpfnWndProc = _cdk_win32_window_procedure;
  wcl.cbClsExtra = 0;
  wcl.cbWndExtra = 0;
  wcl.hInstance = _cdk_app_hmodule;
  wcl.hIcon = 0;
  wcl.hIconSm = 0;

  /* initialize once! */
  if (0 == hAppIcon && 0 == hAppIconSm)
    {
      gchar sLoc [MAX_PATH+1];

      if (0 != GetModuleFileName (_cdk_app_hmodule, sLoc, MAX_PATH))
        {
          ExtractIconEx (sLoc, 0, &hAppIcon, &hAppIconSm, 1);

          if (0 == hAppIcon && 0 == hAppIconSm)
            {
              if (0 != GetModuleFileName (_cdk_dll_hinstance, sLoc, MAX_PATH))
		{
		  ExtractIconEx (sLoc, 0, &hAppIcon, &hAppIconSm, 1);
		}
            }
        }

      if (0 == hAppIcon && 0 == hAppIconSm)
        {
          hAppIcon = LoadImage (NULL, IDI_APPLICATION, IMAGE_ICON,
                                GetSystemMetrics (SM_CXICON),
                                GetSystemMetrics (SM_CYICON), 0);
          hAppIconSm = LoadImage (NULL, IDI_APPLICATION, IMAGE_ICON,
                                  GetSystemMetrics (SM_CXSMICON),
                                  GetSystemMetrics (SM_CYSMICON), 0);
        }
    }

  if (0 == hAppIcon)
    hAppIcon = hAppIconSm;
  else if (0 == hAppIconSm)
    hAppIconSm = hAppIcon;

  wcl.lpszMenuName = NULL;

  /* initialize once per class */
  /*
   * HB: Setting the background brush leads to flicker, because we
   * don't get asked how to clear the background. This is not what
   * we want, at least not for input_only windows ...
   */
#define ONCE_PER_CLASS() \
  wcl.hIcon = CopyIcon (hAppIcon); \
  wcl.hIconSm = CopyIcon (hAppIconSm); \
  wcl.hbrBackground = NULL; \
  wcl.hCursor = LoadCursor (NULL, IDC_ARROW);

  switch (wtype)
    {
    case CDK_WINDOW_TOPLEVEL:
      /* MSDN: CS_OWNDC is needed for OpenGL contexts */
      wcl.style |= CS_OWNDC;
      if (0 == klassTOPLEVEL)
        {
          wcl.lpszClassName = L"cdkWindowToplevel";

          ONCE_PER_CLASS ();
          klassTOPLEVEL = RegisterClassExW (&wcl);
        }
      klass = klassTOPLEVEL;
      break;

    case CDK_WINDOW_CHILD:
      if (0 == klassCHILD)
	{
	  wcl.lpszClassName = L"cdkWindowChild";

	  /* XXX: Find out whether GL Widgets are done for CDK_WINDOW_CHILD
	   *      MSDN says CS_PARENTDC should not be used for GL Context
	   *      creation
	   */
	  wcl.style |= CS_PARENTDC; /* MSDN: ... enhances system performance. */
	  ONCE_PER_CLASS ();
	  klassCHILD = RegisterClassExW (&wcl);
	}
      klass = klassCHILD;
      break;

    case CDK_WINDOW_TEMP:
      if ((wtype_hint == CDK_WINDOW_TYPE_HINT_MENU) ||
          (wtype_hint == CDK_WINDOW_TYPE_HINT_DROPDOWN_MENU) ||
          (wtype_hint == CDK_WINDOW_TYPE_HINT_POPUP_MENU))
        {
          if (klassTEMPSHADOW == 0)
            {
              wcl.lpszClassName = L"cdkWindowTempShadow";
              wcl.style |= CS_SAVEBITS;
              wcl.style |= 0x00020000; /* CS_DROPSHADOW */

              ONCE_PER_CLASS ();
              klassTEMPSHADOW = RegisterClassExW (&wcl);
            }

          klass = klassTEMPSHADOW;
        }
       else
        {
          if (klassTEMP == 0)
            {
              wcl.lpszClassName = L"cdkWindowTemp";
              wcl.style |= CS_SAVEBITS;
              ONCE_PER_CLASS ();
              klassTEMP = RegisterClassExW (&wcl);
            }

          klass = klassTEMP;
        }
      break;

    default:
      g_assert_not_reached ();
      break;
    }

  if (klass == 0)
    {
      WIN32_API_FAILED ("RegisterClassExW");
      g_error ("That is a fatal error");
    }
  return klass;
}

/*
 * Create native windows.
 *
 * With the default Cdk the created windows are mostly toplevel windows.
 *
 * Placement of the window is derived from the passed in window,
 * except for toplevel window where OS/Window Manager placement
 * is used.
 *
 * The visual parameter, is based on CDK_WA_VISUAL if set already.
 * From attributes the only things used is: colormap, title,
 * wmclass and type_hint. [1]. We are checking redundant information
 * and complain if that changes, which would break this implementation
 * again.
 *
 * [1] http://mail.gnome.org/archives/ctk-devel-list/2010-August/msg00214.html
 */
void
_cdk_win32_display_create_window_impl (CdkDisplay    *display,
				       CdkWindow     *window,
				       CdkWindow     *real_parent,
				       CdkScreen     *screen,
				       CdkEventMask   event_mask,
				       CdkWindowAttr *attributes,
				       gint           attributes_mask)
{
  HWND hwndNew;
  HANDLE hparent;
  ATOM klass = 0;
  DWORD dwStyle = 0, dwExStyle;
  RECT rect;
  CdkWindowImplWin32 *impl;
  CdkWin32Display *display_win32;
  const gchar *title;
  wchar_t *wtitle;
  gboolean override_redirect;
  gint window_width, window_height;
  gint offset_x = 0, offset_y = 0;
  gint x, y, real_x = 0, real_y = 0;
  /* check consistency of redundant information */
  guint remaining_mask = attributes_mask;

  g_return_if_fail (display == _cdk_display);

  CDK_NOTE (MISC,
	    g_print ("_cdk_window_impl_new: %s %s\n",
		     (window->window_type == CDK_WINDOW_TOPLEVEL ? "TOPLEVEL" :
		      (window->window_type == CDK_WINDOW_CHILD ? "CHILD" :
		       (window->window_type == CDK_WINDOW_TEMP ? "TEMP" :
			"???"))),
		     (attributes->wclass == CDK_INPUT_OUTPUT ? "" : "input-only"))
			   );

  /* to ensure to not miss important information some additional check against
   * attributes which may silently work on X11 */
  if ((attributes_mask & CDK_WA_X) != 0)
    {
      g_assert (attributes->x == window->x);
      remaining_mask &= ~CDK_WA_X;
    }
  if ((attributes_mask & CDK_WA_Y) != 0)
    {
      g_assert (attributes->y == window->y);
      remaining_mask &= ~CDK_WA_Y;
    }
  override_redirect = FALSE;
  if ((attributes_mask & CDK_WA_NOREDIR) != 0)
    {
      override_redirect = !!attributes->override_redirect;
      remaining_mask &= ~CDK_WA_NOREDIR;
    }

  if ((remaining_mask & ~(CDK_WA_WMCLASS|CDK_WA_VISUAL|CDK_WA_CURSOR|CDK_WA_TITLE|CDK_WA_TYPE_HINT)) != 0)
    g_warning ("_cdk_window_impl_new: uexpected attribute 0x%X",
               remaining_mask & ~(CDK_WA_WMCLASS|CDK_WA_VISUAL|CDK_WA_CURSOR|CDK_WA_TITLE|CDK_WA_TYPE_HINT));

  hparent = CDK_WINDOW_HWND (real_parent);

  impl = g_object_new (CDK_TYPE_WINDOW_IMPL_WIN32, NULL);
  impl->wrapper = CDK_WINDOW (window);
  window->impl = CDK_WINDOW_IMPL (impl);

  if (attributes_mask & CDK_WA_VISUAL)
    g_assert ((cdk_screen_get_system_visual (screen) == attributes->visual) ||
              (cdk_screen_get_rgba_visual (screen) == attributes->visual));

  impl->override_redirect = override_redirect;
  impl->layered = FALSE;
  impl->layered_opacity = 1.0;

  display_win32 = CDK_WIN32_DISPLAY (display);
  impl->window_scale = _cdk_win32_display_get_monitor_scale_factor (display_win32, NULL, NULL, NULL);
  impl->unscaled_width = window->width * impl->window_scale;
  impl->unscaled_height = window->height * impl->window_scale;

  /* wclass is not any longer set always, but if is ... */
  if ((attributes_mask & CDK_WA_WMCLASS) == CDK_WA_WMCLASS)
    g_assert ((attributes->wclass == CDK_INPUT_OUTPUT) == !window->input_only);

  if (!window->input_only)
    {
      dwExStyle = 0;
    }
  else
    {
      /* I very much doubt using WS_EX_TRANSPARENT actually
       * corresponds to how X11 InputOnly windows work, but it appears
       * to work well enough for the actual use cases in ctk.
       */
      dwExStyle = WS_EX_TRANSPARENT;
      CDK_NOTE (MISC, g_print ("... CDK_INPUT_ONLY\n"));
    }

  switch (window->window_type)
    {
    case CDK_WINDOW_TOPLEVEL:
      if (CDK_WINDOW_TYPE (window->parent) != CDK_WINDOW_ROOT)
	{
	  /* The common code warns for this case. */
	  hparent = GetDesktopWindow ();
	}
      /* Children of foreign windows aren't toplevel windows */
      if (CDK_WINDOW_TYPE (real_parent) == CDK_WINDOW_FOREIGN)
	{
	  dwStyle = WS_CHILDWINDOW | WS_CLIPCHILDREN;
	}
      else
	{
	  /* MSDN: We need WS_CLIPCHILDREN and WS_CLIPSIBLINGS for GL Context Creation */
	  if (window->window_type == CDK_WINDOW_TOPLEVEL)
	    dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	  else
	    dwStyle = WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU | WS_CAPTION | WS_THICKFRAME | WS_CLIPCHILDREN;

	  offset_x = _cdk_offset_x;
	  offset_y = _cdk_offset_y;
	}
      break;

    case CDK_WINDOW_CHILD:
      dwStyle = WS_CHILDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
      break;

    case CDK_WINDOW_TEMP:
      /* A temp window is not necessarily a top level window */
      dwStyle = (cdk_screen_get_root_window (screen) == real_parent ? WS_POPUP : WS_CHILDWINDOW);
      dwStyle |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
      dwExStyle |= WS_EX_TOOLWINDOW | WS_EX_TOPMOST;
      offset_x = _cdk_offset_x;
      offset_y = _cdk_offset_y;
      break;

    default:
      g_assert_not_reached ();
    }

  if (window->window_type != CDK_WINDOW_CHILD)
    {
      rect.left = window->x * impl->window_scale;
      rect.top = window->y * impl->window_scale;
      rect.right = rect.left + window->width * impl->window_scale;
      rect.bottom = rect.top + window->height * impl->window_scale;

      AdjustWindowRectEx (&rect, dwStyle, FALSE, dwExStyle);

      real_x = (window->x - offset_x) * impl->window_scale;
      real_y = (window->y - offset_y) * impl->window_scale;

      if (window->window_type == CDK_WINDOW_TOPLEVEL)
	{
	  /* We initially place it at default so that we can get the
	     default window positioning if we want */
	  x = y = CW_USEDEFAULT;
	}
      else
	{
	  /* TEMP, FOREIGN: Put these where requested */
	  x = real_x;
	  y = real_y;
	}

      window_width = rect.right - rect.left;
      window_height = rect.bottom - rect.top;
    }
  else
    {
      /* adjust position relative to real_parent */
      window_width = impl->unscaled_width;
      window_height = impl->unscaled_height;
      /* use given position for initial placement, native coordinates */
      x = (window->x + window->parent->abs_x - offset_x) * impl->window_scale;
      y = (window->y + window->parent->abs_y - offset_y) * impl->window_scale;
    }

  if (attributes_mask & CDK_WA_TITLE)
    title = attributes->title;
  else
    title = get_default_title ();
  if (!title || !*title)
    title = "";

  impl->native_event_mask = CDK_STRUCTURE_MASK | event_mask;

  if (attributes_mask & CDK_WA_TYPE_HINT)
    cdk_window_set_type_hint (window, attributes->type_hint);

  if (impl->type_hint == CDK_WINDOW_TYPE_HINT_UTILITY)
    dwExStyle |= WS_EX_TOOLWINDOW;

  /* WS_EX_TRANSPARENT means "try draw this window last, and ignore input".
   * It's the last part we're after. We don't want DND indicator to accept
   * input, because that will make it a potential drop target, and if it's
   * under the mouse cursor, this will kill any DND.
   */
  if (impl->type_hint == CDK_WINDOW_TYPE_HINT_DND)
    dwExStyle |= WS_EX_TRANSPARENT;

  klass = RegisterCdkClass (window->window_type, impl->type_hint);

  wtitle = g_utf8_to_utf16 (title, -1, NULL, NULL, NULL);

  hwndNew = CreateWindowExW (dwExStyle,
			     MAKEINTRESOURCEW (klass),
			     wtitle,
			     dwStyle,
			     x,
			     y,
			     window_width, window_height,
			     hparent,
			     NULL,
			     _cdk_app_hmodule,
			     window);
  if (CDK_WINDOW_HWND (window) != hwndNew)
    {
      g_warning ("cdk_window_new: cdk_event_translate::WM_CREATE (%p, %p) HWND mismatch.",
		 CDK_WINDOW_HWND (window),
		 hwndNew);

      /* HB: IHMO due to a race condition the handle was increased by
       * one, which causes much trouble. Because I can't find the
       * real bug, try to workaround it ...
       * To reproduce: compile with MSVC 5, DEBUG=1
       */
# if 0
      cdk_win32_handle_table_remove (CDK_WINDOW_HWND (window));
      CDK_WINDOW_HWND (window) = hwndNew;
      cdk_win32_handle_table_insert (&CDK_WINDOW_HWND (window), window);
# else
      /* the old behaviour, but with warning */
      impl->handle = hwndNew;
# endif

    }

  if (window->window_type != CDK_WINDOW_CHILD)
    {
      GetWindowRect (CDK_WINDOW_HWND (window), &rect);
      impl->initial_x = rect.left;
      impl->initial_y = rect.top;

      /* Now we know the initial position, move to actually specified position */
      if (real_x != x || real_y != y)
	{
	  API_CALL (SetWindowPos, (CDK_WINDOW_HWND (window),
				   SWP_NOZORDER_SPECIFIED,
				   real_x, real_y, 0, 0,
				   SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER));
	}
    }

  g_object_ref (window);
  cdk_win32_handle_table_insert (&CDK_WINDOW_HWND (window), window);

  CDK_NOTE (MISC, g_print ("... \"%s\" %dx%d@%+d%+d %p = %p\n",
			   title,
			   window_width, window_height,
			   window->x - offset_x,
			   window->y - offset_y,
			   hparent,
			   CDK_WINDOW_HWND (window)));

  /* Add window handle to title */
  CDK_NOTE (MISC_OR_EVENTS, cdk_window_set_title (window, title));

  g_free (wtitle);

  if (impl->handle == NULL)
    {
      WIN32_API_FAILED ("CreateWindowExW");
      g_object_unref (window);
      return;
    }

//  if (!from_set_skip_taskbar_hint && window->window_type == CDK_WINDOW_TEMP)
//    cdk_window_set_skip_taskbar_hint (window, TRUE);

  if (attributes_mask & CDK_WA_CURSOR)
    cdk_window_set_cursor (window, attributes->cursor);

  _cdk_win32_window_enable_transparency (window);
}

CdkWindow *
cdk_win32_window_foreign_new_for_display (CdkDisplay *display,
                                          HWND        anid)
{
  CdkWindow *window;
  CdkWindowImplWin32 *impl;

  HANDLE parent;
  RECT rect;
  POINT point;

  if ((window = cdk_win32_window_lookup_for_display (display, anid)) != NULL)
    return g_object_ref (window);

  window = _cdk_display_create_window (display);
  window->visual = cdk_screen_get_system_visual (cdk_display_get_default_screen (display));
  window->impl = g_object_new (CDK_TYPE_WINDOW_IMPL_WIN32, NULL);
  window->impl_window = window;
  impl = CDK_WINDOW_IMPL_WIN32 (window->impl);
  impl->wrapper = window;
  parent = GetParent (anid);

  window->parent = cdk_win32_handle_table_lookup (parent);
  if (!window->parent || CDK_WINDOW_TYPE (window->parent) == CDK_WINDOW_FOREIGN)
    window->parent = cdk_get_default_root_window ();

  window->parent->children = g_list_concat (&window->children_list_node, window->parent->children);
  window->parent->impl_window->native_children =
    g_list_prepend (window->parent->impl_window->native_children, window);

  GetClientRect ((HWND) anid, &rect);
  point.x = rect.left;
  point.y = rect.right;
  ClientToScreen ((HWND) anid, &point);
  if (parent != GetDesktopWindow ())
    ScreenToClient (parent, &point);
  window->x = point.x / impl->window_scale;
  window->y = point.y / impl->window_scale;
  impl->unscaled_width = rect.right - rect.left;
  impl->unscaled_height = rect.bottom - rect.top;
  window->width = (impl->unscaled_width + impl->window_scale - 1) / impl->window_scale;
  window->height = (impl->unscaled_height + impl->window_scale - 1) / impl->window_scale;
  window->window_type = CDK_WINDOW_FOREIGN;
  window->destroyed = FALSE;
  window->event_mask = CDK_ALL_EVENTS_MASK; /* XXX */
  if (IsWindowVisible ((HWND) anid))
    window->state &= (~CDK_WINDOW_STATE_WITHDRAWN);
  else
    window->state |= CDK_WINDOW_STATE_WITHDRAWN;
  if (GetWindowLong ((HWND)anid, GWL_EXSTYLE) & WS_EX_TOPMOST)
    window->state |= CDK_WINDOW_STATE_ABOVE;
  else
    window->state &= (~CDK_WINDOW_STATE_ABOVE);
  window->state &= (~CDK_WINDOW_STATE_BELOW);
  window->viewable = TRUE;

  window->depth = cdk_visual_get_system ()->depth;
  CDK_WINDOW_HWND (window) = anid;

  g_object_ref (window);
  cdk_win32_handle_table_insert (&CDK_WINDOW_HWND (window), window);

  CDK_NOTE (MISC, g_print ("cdk_win32_window_foreign_new_for_display: %p: %s@%+d%+d\n",
			   (HWND) anid,
			   _cdk_win32_window_description (window),
			   window->x, window->y));

  return window;
}

static void
cdk_win32_window_destroy (CdkWindow *window,
			  gboolean   recursing,
			  gboolean   foreign_destroy)
{
  CdkWindowImplWin32 *window_impl = CDK_WINDOW_IMPL_WIN32 (window->impl);
  GSList *tmp;
  CdkWin32Display *display = NULL;

  g_return_if_fail (CDK_IS_WINDOW (window));

  CDK_NOTE (MISC, g_print ("cdk_win32_window_destroy: %p\n",
			   CDK_WINDOW_HWND (window)));

  /* Remove ourself from the modal stack */
  _cdk_remove_modal_window (window);

  /* Remove all our transient children */
  while (window_impl->transient_children != NULL)
    {
      CdkWindow *child = window_impl->transient_children->data;
      cdk_window_set_transient_for (child, NULL);
    }

#ifdef CDK_WIN32_ENABLE_EGL
  display = CDK_WIN32_DISPLAY (cdk_window_get_display (window));

  /* Get rid of any EGLSurfaces that we might have created */
  if (window_impl->egl_surface != EGL_NO_SURFACE)
    {
      eglDestroySurface (display->egl_disp, window_impl->egl_surface);
      window_impl->egl_surface = EGL_NO_SURFACE;
    }
  if (window_impl->egl_dummy_surface != EGL_NO_SURFACE)
    {
      eglDestroySurface (display->egl_disp, window_impl->egl_dummy_surface);
      window_impl->egl_dummy_surface = EGL_NO_SURFACE;
    }
#endif

  /* Remove ourself from our transient owner */
  if (window_impl->transient_owner != NULL)
    {
      cdk_window_set_transient_for (window, NULL);
    }

  if (!recursing && !foreign_destroy)
    {
      window->destroyed = TRUE;
      DestroyWindow (CDK_WINDOW_HWND (window));
    }
}

static void
cdk_win32_window_destroy_foreign (CdkWindow *window)
{
  /* It's somebody else's window, but in our hierarchy, so reparent it
   * to the desktop, and then try to destroy it.
   */
  cdk_window_hide (window);
  cdk_window_reparent (window, NULL, 0, 0);

  PostMessage (CDK_WINDOW_HWND (window), WM_CLOSE, 0, 0);
}

/* This function is called when the window really gone.
 */
static void
cdk_win32_window_destroy_notify (CdkWindow *window)
{
  g_return_if_fail (CDK_IS_WINDOW (window));

  CDK_NOTE (EVENTS,
	    g_print ("cdk_window_destroy_notify: %p%s\n",
		     CDK_WINDOW_HWND (window),
		     (CDK_WINDOW_DESTROYED (window) ? " (destroyed)" : "")));

  if (!CDK_WINDOW_DESTROYED (window))
    {
      if (CDK_WINDOW_TYPE (window) != CDK_WINDOW_FOREIGN)
	g_warning ("window %p unexpectedly destroyed",
		   CDK_WINDOW_HWND (window));

      _cdk_window_destroy (window, TRUE);
    }

  cdk_win32_handle_table_remove (CDK_WINDOW_HWND (window));
  g_object_unref (window);
}

static void
get_outer_rect (CdkWindow *window,
		gint       width,
		gint       height,
		RECT      *rect)
{
  CdkWindowImplWin32 *impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  rect->left = rect->top = 0;
  rect->right = width * impl->window_scale;
  rect->bottom = height * impl->window_scale;

  _cdk_win32_adjust_client_rect (window, rect);
}

static void
adjust_for_gravity_hints (CdkWindow *window,
			  RECT      *outer_rect,
			  gint		*x,
			  gint		*y)
{
  CdkWindowImplWin32 *impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  if (impl->hint_flags & CDK_HINT_WIN_GRAVITY)
    {
#ifdef G_ENABLE_DEBUG
      gint orig_x = *x, orig_y = *y;
#endif

      switch (impl->hints.win_gravity)
	{
	case CDK_GRAVITY_NORTH:
	case CDK_GRAVITY_CENTER:
	case CDK_GRAVITY_SOUTH:
	  *x -= (outer_rect->right - outer_rect->left / 2) / impl->window_scale;
	  *x += window->width / 2;
	  break;

	case CDK_GRAVITY_SOUTH_EAST:
	case CDK_GRAVITY_EAST:
	case CDK_GRAVITY_NORTH_EAST:
	  *x -= (outer_rect->right - outer_rect->left) / impl->window_scale;
	  *x += window->width;
	  break;

	case CDK_GRAVITY_STATIC:
	  *x += outer_rect->left / impl->window_scale;
	  break;

	default:
	  break;
	}

      switch (impl->hints.win_gravity)
	{
	case CDK_GRAVITY_WEST:
	case CDK_GRAVITY_CENTER:
	case CDK_GRAVITY_EAST:
	  *y -= ((outer_rect->bottom - outer_rect->top) / 2) / impl->window_scale;
	  *y += window->height / 2;
	  break;

	case CDK_GRAVITY_SOUTH_WEST:
	case CDK_GRAVITY_SOUTH:
	case CDK_GRAVITY_SOUTH_EAST:
	  *y -= (outer_rect->bottom - outer_rect->top) / impl->window_scale;
	  *y += window->height;
	  break;

	case CDK_GRAVITY_STATIC:
	  *y += outer_rect->top * impl->window_scale;
	  break;

	default:
	  break;
	}
      CDK_NOTE (MISC,
		(orig_x != *x || orig_y != *y) ?
		g_print ("adjust_for_gravity_hints: x: %d->%d, y: %d->%d\n",
			 orig_x, *x, orig_y, *y)
		  : (void) 0);
    }
}

static void
show_window_internal (CdkWindow *window,
                      gboolean   already_mapped,
		      gboolean   deiconify)
{
  CdkWindowImplWin32 *window_impl;
  gboolean focus_on_map = FALSE;
  DWORD exstyle;

  if (window->destroyed)
    return;

  CDK_NOTE (MISC, g_print ("show_window_internal: %p: %s%s\n",
			   CDK_WINDOW_HWND (window),
			   _cdk_win32_window_state_to_string (window->state),
			   (deiconify ? " deiconify" : "")));

  /* If asked to show (not deiconify) an withdrawn and iconified
   * window, do that.
   */
  if (!deiconify &&
      !already_mapped &&
      (window->state & CDK_WINDOW_STATE_ICONIFIED))
    {
      CtkShowWindow (window, SW_SHOWMINNOACTIVE);
      return;
    }

  /* If asked to just show an iconified window, do nothing. */
  if (!deiconify && (window->state & CDK_WINDOW_STATE_ICONIFIED))
    return;

  /* If asked to deiconify an already noniconified window, do
   * nothing. (Especially, don't cause the window to rise and
   * activate. There are different calls for that.)
   */
  if (deiconify && !(window->state & CDK_WINDOW_STATE_ICONIFIED))
    return;

  /* If asked to show (but not raise) a window that is already
   * visible, do nothing.
   */
  if (!deiconify && !already_mapped && IsWindowVisible (CDK_WINDOW_HWND (window)))
    return;

  /* Other cases */

  if (!already_mapped)
    focus_on_map = window->focus_on_map;

  exstyle = GetWindowLong (CDK_WINDOW_HWND (window), GWL_EXSTYLE);

  /* Use SetWindowPos to show transparent windows so automatic redraws
   * in other windows can be suppressed.
   */
  if (exstyle & WS_EX_TRANSPARENT)
    {
      UINT flags = SWP_SHOWWINDOW | SWP_NOREDRAW | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER;

      if (CDK_WINDOW_TYPE (window) == CDK_WINDOW_TEMP || !focus_on_map)
	flags |= SWP_NOACTIVATE;

      SetWindowPos (CDK_WINDOW_HWND (window),
		    SWP_NOZORDER_SPECIFIED, 0, 0, 0, 0, flags);

      return;
    }

  /* For initial map of "normal" windows we want to emulate WM window
   * positioning behaviour, which means:
   * + Use user specified position if CDK_HINT_POS or CDK_HINT_USER_POS
   * otherwise:
   * + default to the initial CW_USEDEFAULT placement,
   *   no matter if the user moved the window before showing it.
   * + Certain window types and hints have more elaborate positioning
   *   schemes.
   */
  window_impl = CDK_WINDOW_IMPL_WIN32 (window->impl);
  if (!already_mapped &&
      CDK_WINDOW_TYPE (window) == CDK_WINDOW_TOPLEVEL &&
      (window_impl->hint_flags & (CDK_HINT_POS | CDK_HINT_USER_POS)) == 0 &&
      !window_impl->override_redirect)
    {
      gboolean center = FALSE;
      RECT window_rect, center_on_rect;
      int x, y;

      x = window_impl->initial_x;
      y = window_impl->initial_y;

      if (window_impl->type_hint == CDK_WINDOW_TYPE_HINT_SPLASHSCREEN)
	{
	  HMONITOR monitor;
	  MONITORINFO mi;

	  monitor = MonitorFromWindow (CDK_WINDOW_HWND (window), MONITOR_DEFAULTTONEAREST);
	  mi.cbSize = sizeof (mi);
	  if (monitor && GetMonitorInfo (monitor, &mi))
	    center_on_rect = mi.rcMonitor;
	  else
	    {
	      center_on_rect.left = 0;
	      center_on_rect.top = 0;
	      center_on_rect.right = GetSystemMetrics (SM_CXSCREEN);
	      center_on_rect.bottom = GetSystemMetrics (SM_CYSCREEN);
	    }
	  center = TRUE;
	}
      else if (window_impl->transient_owner != NULL &&
	       CDK_WINDOW_IS_MAPPED (window_impl->transient_owner))
	{
	  CdkWindow *owner = window_impl->transient_owner;
	  /* Center on transient parent */
	  center_on_rect.left = (owner->x - _cdk_offset_x) * window_impl->window_scale;
	  center_on_rect.top = (owner->y - _cdk_offset_y) * window_impl->window_scale;
	  center_on_rect.right = center_on_rect.left + owner->width * window_impl->window_scale;
	  center_on_rect.bottom = center_on_rect.top + owner->height * window_impl->window_scale;

	  _cdk_win32_adjust_client_rect (CDK_WINDOW (owner), &center_on_rect);
	  center = TRUE;
	}

      if (center)
	{
	  window_rect.left = 0;
	  window_rect.top = 0;
	  window_rect.right = window->width * window_impl->window_scale;
	  window_rect.bottom = window->height * window_impl->window_scale;
	  _cdk_win32_adjust_client_rect (window, &window_rect);

	  x = center_on_rect.left + ((center_on_rect.right - center_on_rect.left) - (window_rect.right - window_rect.left)) / 2;
	  y = center_on_rect.top + ((center_on_rect.bottom - center_on_rect.top) - (window_rect.bottom - window_rect.top)) / 2;
	}

      API_CALL (SetWindowPos, (CDK_WINDOW_HWND (window),
			       SWP_NOZORDER_SPECIFIED,
			       x, y, 0, 0,
			       SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER));
    }

  if (!already_mapped &&
      CDK_WINDOW_TYPE (window) == CDK_WINDOW_TOPLEVEL &&
      !window_impl->override_redirect)
    {
      /* Ensure new windows are fully onscreen */
      RECT window_rect;
      HMONITOR monitor;
      MONITORINFO mi;
      int x, y;

      GetWindowRect (CDK_WINDOW_HWND (window), &window_rect);

      monitor = MonitorFromWindow (CDK_WINDOW_HWND (window), MONITOR_DEFAULTTONEAREST);
      mi.cbSize = sizeof (mi);
      if (monitor && GetMonitorInfo (monitor, &mi))
	{
	  x = window_rect.left;
	  y = window_rect.top;

	  if (window_rect.right > mi.rcWork.right)
	    {
	      window_rect.left -= (window_rect.right - mi.rcWork.right);
	      window_rect.right -= (window_rect.right - mi.rcWork.right);
	    }

	  if (window_rect.bottom > mi.rcWork.bottom)
	    {
	      window_rect.top -= (window_rect.bottom - mi.rcWork.bottom);
	      window_rect.bottom -= (window_rect.bottom - mi.rcWork.bottom);
	    }

	  if (window_rect.left < mi.rcWork.left)
	    {
	      window_rect.right += (mi.rcWork.left - window_rect.left);
	      window_rect.left += (mi.rcWork.left - window_rect.left);
	    }

	  if (window_rect.top < mi.rcWork.top)
	    {
	      window_rect.bottom += (mi.rcWork.top - window_rect.top);
	      window_rect.top += (mi.rcWork.top - window_rect.top);
	    }

	  if (x != window_rect.left || y != window_rect.top)
	    API_CALL (SetWindowPos, (CDK_WINDOW_HWND (window),
				     SWP_NOZORDER_SPECIFIED,
				     window_rect.left, window_rect.top, 0, 0,
				     SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER));
	}
    }


  if (window->state & CDK_WINDOW_STATE_FULLSCREEN)
    {
      cdk_window_fullscreen (window);
    }
  else if (window->state & CDK_WINDOW_STATE_MAXIMIZED)
    {
      CtkShowWindow (window, SW_MAXIMIZE);
    }
  else if (window->state & CDK_WINDOW_STATE_ICONIFIED)
    {
      if (focus_on_map)
        CtkShowWindow (window, SW_RESTORE);
      else
        CtkShowWindow (window, SW_SHOWNOACTIVATE);
    }
  else if (CDK_WINDOW_TYPE (window) == CDK_WINDOW_TEMP || !focus_on_map)
    {
      if (!IsWindowVisible (CDK_WINDOW_HWND (window)))
        CtkShowWindow (window, SW_SHOWNOACTIVATE);
      else
        CtkShowWindow (window, SW_SHOWNA);
    }
  else if (!IsWindowVisible (CDK_WINDOW_HWND (window)))
    {
      CtkShowWindow (window, SW_SHOWNORMAL);
    }
  else
    {
      CtkShowWindow (window, SW_SHOW);
    }

  /* Sync STATE_ABOVE to TOPMOST */
  if (CDK_WINDOW_TYPE (window) != CDK_WINDOW_TEMP &&
      (((window->state & CDK_WINDOW_STATE_ABOVE) &&
	!(exstyle & WS_EX_TOPMOST)) ||
       (!(window->state & CDK_WINDOW_STATE_ABOVE) &&
	(exstyle & WS_EX_TOPMOST))))
    {
      API_CALL (SetWindowPos, (CDK_WINDOW_HWND (window),
			       (window->state & CDK_WINDOW_STATE_ABOVE)?HWND_TOPMOST:HWND_NOTOPMOST,
			       0, 0, 0, 0,
			       SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE));
    }
}

static void
cdk_win32_window_show (CdkWindow *window,
		       gboolean already_mapped)
{
  show_window_internal (window, FALSE, FALSE);
}

static void
cdk_win32_window_hide (CdkWindow *window)
{
  if (window->destroyed)
    return;

  CDK_NOTE (MISC, g_print ("cdk_win32_window_hide: %p: %s\n",
			   CDK_WINDOW_HWND (window),
			   _cdk_win32_window_state_to_string (window->state)));

  if (CDK_WINDOW_IS_MAPPED (window))
    cdk_synthesize_window_state (window,
				 0,
				 CDK_WINDOW_STATE_WITHDRAWN);

  _cdk_window_clear_update_area (window);

  if (CDK_WINDOW_TYPE (window) == CDK_WINDOW_TOPLEVEL)
    ShowOwnedPopups (CDK_WINDOW_HWND (window), FALSE);

  /* Use SetWindowPos to hide transparent windows so automatic redraws
   * in other windows can be suppressed.
   */
  if (GetWindowLong (CDK_WINDOW_HWND (window), GWL_EXSTYLE) & WS_EX_TRANSPARENT)
    {
      SetWindowPos (CDK_WINDOW_HWND (window), SWP_NOZORDER_SPECIFIED,
		    0, 0, 0, 0,
		    SWP_HIDEWINDOW | SWP_NOREDRAW | SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE);
    }
  else
    {
      CtkShowWindow (window, SW_HIDE);
    }
}

static void
cdk_win32_window_withdraw (CdkWindow *window)
{
  if (window->destroyed)
    return;

  CDK_NOTE (MISC, g_print ("cdk_win32_window_withdraw: %p: %s\n",
			   CDK_WINDOW_HWND (window),
			   _cdk_win32_window_state_to_string (window->state)));

  cdk_window_hide (window);	/* ??? */
}

static void
cdk_win32_window_move (CdkWindow *window,
		       gint x, gint y)
{
  g_return_if_fail (CDK_IS_WINDOW (window));

  if (CDK_WINDOW_DESTROYED (window))
    return;

  CDK_NOTE (MISC, g_print ("cdk_win32_window_move: %p: %+d%+d\n",
                           CDK_WINDOW_HWND (window), x, y));

  if (window->state & CDK_WINDOW_STATE_FULLSCREEN)
    return;

  /* Don't check CDK_WINDOW_TYPE (window) == CDK_WINDOW_CHILD.
   * Foreign windows (another app's windows) might be children of our
   * windows! Especially in the case of ctkplug/socket.
   */
  if (GetAncestor (CDK_WINDOW_HWND (window), GA_PARENT) != GetDesktopWindow ())
    {
      _cdk_window_move_resize_child (window, x, y, window->width, window->height);
    }
  else
    {
      RECT outer_rect;
      CdkWindowImplWin32 *impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

      get_outer_rect (window, window->width, window->height, &outer_rect);

      adjust_for_gravity_hints (window, &outer_rect, &x, &y);

      CDK_NOTE (MISC, g_print ("... SetWindowPos(%p,NULL,%d,%d,0,0,"
                               "NOACTIVATE|NOSIZE|NOZORDER)\n",
                               CDK_WINDOW_HWND (window),
                               (x - _cdk_offset_x) * impl->window_scale,
                               (y - _cdk_offset_y) * impl->window_scale));

      API_CALL (SetWindowPos, (CDK_WINDOW_HWND (window),
			       SWP_NOZORDER_SPECIFIED,
                               (x - _cdk_offset_x) * impl->window_scale,
                               (y - _cdk_offset_y) * impl->window_scale,
                               0, 0,
                               SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER));
    }
}

static void
cdk_win32_window_resize (CdkWindow *window,
			 gint width, gint height)
{
  g_return_if_fail (CDK_IS_WINDOW (window));

  if (CDK_WINDOW_DESTROYED (window))
    return;

  if (width < 1)
    width = 1;
  if (height < 1)
    height = 1;

  CDK_NOTE (MISC, g_print ("cdk_win32_window_resize: %p: %dx%d\n",
                           CDK_WINDOW_HWND (window), width, height));

  if (window->state & CDK_WINDOW_STATE_FULLSCREEN)
    return;

  if (GetAncestor (CDK_WINDOW_HWND (window), GA_PARENT) != GetDesktopWindow ())
    {
      _cdk_window_move_resize_child (window, window->x, window->y, width, height);
    }
  else
    {
      RECT outer_rect;
      CdkWindowImplWin32 *impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

      get_outer_rect (window, width, height, &outer_rect);

      CDK_NOTE (MISC, g_print ("... SetWindowPos(%p,NULL,0,0,%ld,%ld,"
                               "NOACTIVATE|NOMOVE|NOZORDER)\n",
                               CDK_WINDOW_HWND (window),
                               outer_rect.right - outer_rect.left,
                               outer_rect.bottom - outer_rect.top));

      API_CALL (SetWindowPos, (CDK_WINDOW_HWND (window),
			       SWP_NOZORDER_SPECIFIED,
                               0, 0,
                               outer_rect.right - outer_rect.left,
                               outer_rect.bottom - outer_rect.top,
                               SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER));
      window->resize_count += 1;
    }
}

static void
cdk_win32_window_move_resize_internal (CdkWindow *window,
				       gint       x,
				       gint       y,
				       gint       width,
				       gint       height)
{
  g_return_if_fail (CDK_IS_WINDOW (window));

  if (CDK_WINDOW_DESTROYED (window))
    return;

  if (width < 1)
    width = 1;
  if (height < 1)
    height = 1;

  if (window->state & CDK_WINDOW_STATE_FULLSCREEN)
    return;

  CDK_NOTE (MISC, g_print ("cdk_win32_window_move_resize: %p: %dx%d@%+d%+d\n",
                           CDK_WINDOW_HWND (window),
                           width, height, x, y));

  if (GetAncestor (CDK_WINDOW_HWND (window), GA_PARENT) != GetDesktopWindow ())
    {
      _cdk_window_move_resize_child (window, x, y, width, height);
    }
  else
    {
      RECT outer_rect;
      CdkWindowImplWin32 *impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

      get_outer_rect (window, width, height, &outer_rect);

      adjust_for_gravity_hints (window, &outer_rect, &x, &y);

      CDK_NOTE (MISC, g_print ("... SetWindowPos(%p,NULL,%d,%d,%ld,%ld,"
                               "NOACTIVATE|NOZORDER)\n",
                               CDK_WINDOW_HWND (window),
                               (x - _cdk_offset_x) * impl->window_scale,
                               (y - _cdk_offset_y) * impl->window_scale,
                               outer_rect.right - outer_rect.left,
                               outer_rect.bottom - outer_rect.top));

      API_CALL (SetWindowPos, (CDK_WINDOW_HWND (window),
			       SWP_NOZORDER_SPECIFIED,
                               (x - _cdk_offset_x) * impl->window_scale,
                               (y - _cdk_offset_y) * impl->window_scale,
                               outer_rect.right - outer_rect.left,
                               outer_rect.bottom - outer_rect.top,
                               SWP_NOACTIVATE | SWP_NOZORDER));
    }
}

static void
cdk_win32_window_move_resize (CdkWindow *window,
			      gboolean   with_move,
			      gint       x,
			      gint       y,
			      gint       width,
			      gint       height)
{
  CdkWindowImplWin32 *window_impl;

  window_impl = CDK_WINDOW_IMPL_WIN32 (window->impl);
  window_impl->inhibit_configure = TRUE;

  /* We ignore changes to the window being moved or resized by the
     user, as we don't want to fight the user */
  if (CDK_WINDOW_HWND (window) == _modal_move_resize_window)
    goto out;

  if (with_move && (width < 0 && height < 0))
    {
      cdk_win32_window_move (window, x, y);
    }
  else
    {
      _cdk_win32_window_invalidate_egl_framebuffer (window);
      if (with_move)
	{
	  cdk_win32_window_move_resize_internal (window, x, y, width, height);
	}
      else
	{
	  cdk_win32_window_resize (window, width, height);
	}
    }

 out:
  window_impl->inhibit_configure = FALSE;

  if (WINDOW_IS_TOPLEVEL (window))
    _cdk_win32_emit_configure_event (window);
}

static gboolean
cdk_win32_window_reparent (CdkWindow *window,
			   CdkWindow *new_parent,
			   gint       x,
			   gint       y)
{
  CdkScreen *screen;
  CdkWindowImplWin32 *impl;
  gboolean new_parent_is_root;
  gboolean was_toplevel;
  LONG style;

  screen = cdk_window_get_screen (window);

  if (!new_parent)
    {
      new_parent = cdk_screen_get_root_window (screen);
      new_parent_is_root = TRUE;
    }
  else
     new_parent_is_root = (cdk_screen_get_root_window (screen) == new_parent);

  impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  CDK_NOTE (MISC, g_print ("cdk_win32_window_reparent: %p: %p\n",
			   CDK_WINDOW_HWND (window),
			   CDK_WINDOW_HWND (new_parent)));

  style = GetWindowLong (CDK_WINDOW_HWND (window), GWL_STYLE);

  was_toplevel = GetAncestor (CDK_WINDOW_HWND (window), GA_PARENT) == GetDesktopWindow ();
  if (was_toplevel && !new_parent_is_root)
    {
      /* Reparenting from top-level (child of desktop). Clear out
       * decorations.
       */
      style &= ~(WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
      style |= WS_CHILD;
      SetWindowLong (CDK_WINDOW_HWND (window), GWL_STYLE, style);
    }
  else if (new_parent_is_root)
    {
      /* Reparenting to top-level. Add decorations. */
      style &= ~(WS_CHILD);
      style |= WS_OVERLAPPEDWINDOW;
      SetWindowLong (CDK_WINDOW_HWND (window), GWL_STYLE, style);
    }

  API_CALL (SetParent, (CDK_WINDOW_HWND (window),
			CDK_WINDOW_HWND (new_parent)));

  /* From here on, we treat parents of type CDK_WINDOW_FOREIGN like
   * the root window
   */
  if (CDK_WINDOW_TYPE (new_parent) == CDK_WINDOW_FOREIGN)
    new_parent = cdk_screen_get_root_window (screen);

  window->parent = new_parent;

  /* Switch the window type as appropriate */

  switch (CDK_WINDOW_TYPE (new_parent))
    {
    case CDK_WINDOW_ROOT:
      if (impl->toplevel_window_type != -1)
	CDK_WINDOW_TYPE (window) = impl->toplevel_window_type;
      else if (CDK_WINDOW_TYPE (window) == CDK_WINDOW_CHILD)
	CDK_WINDOW_TYPE (window) = CDK_WINDOW_TOPLEVEL;
      break;

    case CDK_WINDOW_TOPLEVEL:
    case CDK_WINDOW_CHILD:
    case CDK_WINDOW_TEMP:
      if (WINDOW_IS_TOPLEVEL (window))
	{
	  /* Save the original window type so we can restore it if the
	   * window is reparented back to be a toplevel.
	   */
	  impl->toplevel_window_type = CDK_WINDOW_TYPE (window);
	  CDK_WINDOW_TYPE (window) = CDK_WINDOW_CHILD;
	}
    }

  /* Move window into desired position while keeping the same client area */
  cdk_win32_window_move_resize (window, TRUE, x, y, window->width, window->height);

  return FALSE;
}

static void
cdk_win32_window_raise (CdkWindow *window)
{
  if (!CDK_WINDOW_DESTROYED (window))
    {
      CDK_NOTE (MISC, g_print ("cdk_win32_window_raise: %p\n",
			       CDK_WINDOW_HWND (window)));

      if (CDK_WINDOW_TYPE (window) == CDK_WINDOW_TEMP)
        API_CALL (SetWindowPos, (CDK_WINDOW_HWND (window), HWND_TOPMOST,
	                         0, 0, 0, 0,
				 SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER));
      else if (window->accept_focus)
        /* Do not wrap this in an API_CALL macro as SetForegroundWindow might
         * fail when for example dragging a window belonging to a different
         * application at the time of a ctk_window_present() call due to focus
         * stealing prevention. */
        SetForegroundWindow (CDK_WINDOW_HWND (window));
      else
        API_CALL (SetWindowPos, (CDK_WINDOW_HWND (window), HWND_TOP,
  			         0, 0, 0, 0,
			         SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE));
    }
}

static void
cdk_win32_window_lower (CdkWindow *window)
{
  if (!CDK_WINDOW_DESTROYED (window))
    {
      CDK_NOTE (MISC, g_print ("cdk_win32_window_lower: %p\n"
			       "... SetWindowPos(%p,HWND_BOTTOM,0,0,0,0,"
			       "NOACTIVATE|NOMOVE|NOSIZE)\n",
			       CDK_WINDOW_HWND (window),
			       CDK_WINDOW_HWND (window)));

      API_CALL (SetWindowPos, (CDK_WINDOW_HWND (window), HWND_BOTTOM,
			       0, 0, 0, 0,
			       SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE));
    }
}

static void
cdk_win32_window_set_urgency_hint (CdkWindow *window,
			     gboolean   urgent)
{
  FLASHWINFO flashwinfo;
  typedef BOOL (WINAPI *PFN_FlashWindowEx) (FLASHWINFO*);
  PFN_FlashWindowEx flashWindowEx = NULL;

  g_return_if_fail (CDK_IS_WINDOW (window));
  g_return_if_fail (CDK_WINDOW_TYPE (window) != CDK_WINDOW_CHILD);

  if (CDK_WINDOW_DESTROYED (window))
    return;

  flashWindowEx = (PFN_FlashWindowEx) GetProcAddress (GetModuleHandle ("user32.dll"), "FlashWindowEx");

  if (flashWindowEx)
    {
      flashwinfo.cbSize = sizeof (flashwinfo);
      flashwinfo.hwnd = CDK_WINDOW_HWND (window);
      if (urgent)
	flashwinfo.dwFlags = FLASHW_ALL | FLASHW_TIMER;
      else
	flashwinfo.dwFlags = FLASHW_STOP;
      flashwinfo.uCount = 0;
      flashwinfo.dwTimeout = 0;

      flashWindowEx (&flashwinfo);
    }
  else
    {
      FlashWindow (CDK_WINDOW_HWND (window), urgent);
    }
}

static gboolean
get_effective_window_decorations (CdkWindow       *window,
                                  CdkWMDecoration *decoration)
{
  CdkWindowImplWin32 *impl;

  impl = (CdkWindowImplWin32 *)window->impl;

  if (cdk_window_get_decorations (window, decoration))
    return TRUE;

  if (window->window_type != CDK_WINDOW_TOPLEVEL)
    {
      return FALSE;
    }

  if ((impl->hint_flags & CDK_HINT_MIN_SIZE) &&
      (impl->hint_flags & CDK_HINT_MAX_SIZE) &&
      impl->hints.min_width == impl->hints.max_width &&
      impl->hints.min_height == impl->hints.max_height)
    {
      *decoration = CDK_DECOR_ALL | CDK_DECOR_RESIZEH | CDK_DECOR_MAXIMIZE;

      if (impl->type_hint == CDK_WINDOW_TYPE_HINT_DIALOG ||
	  impl->type_hint == CDK_WINDOW_TYPE_HINT_MENU ||
	  impl->type_hint == CDK_WINDOW_TYPE_HINT_TOOLBAR)
	{
	  *decoration |= CDK_DECOR_MINIMIZE;
	}
      else if (impl->type_hint == CDK_WINDOW_TYPE_HINT_SPLASHSCREEN)
	{
	  *decoration |= CDK_DECOR_MENU | CDK_DECOR_MINIMIZE;
	}

      return TRUE;
    }
  else if (impl->hint_flags & CDK_HINT_MAX_SIZE)
    {
      *decoration = CDK_DECOR_ALL | CDK_DECOR_MAXIMIZE;
      if (impl->type_hint == CDK_WINDOW_TYPE_HINT_DIALOG ||
	  impl->type_hint == CDK_WINDOW_TYPE_HINT_MENU ||
	  impl->type_hint == CDK_WINDOW_TYPE_HINT_TOOLBAR)
	{
	  *decoration |= CDK_DECOR_MINIMIZE;
	}

      return TRUE;
    }
  else
    {
      switch (impl->type_hint)
	{
	case CDK_WINDOW_TYPE_HINT_DIALOG:
	  *decoration = (CDK_DECOR_ALL | CDK_DECOR_MINIMIZE | CDK_DECOR_MAXIMIZE);
	  return TRUE;

	case CDK_WINDOW_TYPE_HINT_MENU:
	  *decoration = (CDK_DECOR_ALL | CDK_DECOR_RESIZEH | CDK_DECOR_MINIMIZE | CDK_DECOR_MAXIMIZE);
	  return TRUE;

	case CDK_WINDOW_TYPE_HINT_TOOLBAR:
	case CDK_WINDOW_TYPE_HINT_UTILITY:
	  cdk_window_set_skip_taskbar_hint (window, TRUE);
	  cdk_window_set_skip_pager_hint (window, TRUE);
	  *decoration = (CDK_DECOR_ALL | CDK_DECOR_MINIMIZE | CDK_DECOR_MAXIMIZE);
	  return TRUE;

	case CDK_WINDOW_TYPE_HINT_SPLASHSCREEN:
	  *decoration = (CDK_DECOR_ALL | CDK_DECOR_RESIZEH | CDK_DECOR_MENU |
			 CDK_DECOR_MINIMIZE | CDK_DECOR_MAXIMIZE);
	  return TRUE;

	case CDK_WINDOW_TYPE_HINT_DOCK:
	  return FALSE;

	case CDK_WINDOW_TYPE_HINT_DESKTOP:
	  return FALSE;

	default:
	  /* Fall thru */
	case CDK_WINDOW_TYPE_HINT_NORMAL:
	  *decoration = CDK_DECOR_ALL;
	  return TRUE;
	}
    }

  return FALSE;
}

static void
cdk_win32_window_set_geometry_hints (CdkWindow         *window,
			       const CdkGeometry *geometry,
			       CdkWindowHints     geom_mask)
{
  CdkWindowImplWin32 *impl;
  FullscreenInfo *fi;

  g_return_if_fail (CDK_IS_WINDOW (window));

  if (CDK_WINDOW_DESTROYED (window))
    return;

  CDK_NOTE (MISC, g_print ("cdk_window_set_geometry_hints: %p\n",
			   CDK_WINDOW_HWND (window)));

  impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  fi = g_object_get_data (G_OBJECT (window), "fullscreen-info");
  if (fi)
    fi->hint_flags = geom_mask;
  else
    impl->hint_flags = geom_mask;
  impl->hints = *geometry;

  if (geom_mask & CDK_HINT_POS)
    {
      /* even the X11 mplementation doesn't care */
    }

  if (geom_mask & CDK_HINT_MIN_SIZE)
    {
      CDK_NOTE (MISC, g_print ("... MIN_SIZE: %dx%d\n",
			       geometry->min_width, geometry->min_height));
    }

  if (geom_mask & CDK_HINT_MAX_SIZE)
    {
      CDK_NOTE (MISC, g_print ("... MAX_SIZE: %dx%d\n",
			       geometry->max_width, geometry->max_height));
    }

  if (geom_mask & CDK_HINT_BASE_SIZE)
    {
      CDK_NOTE (MISC, g_print ("... BASE_SIZE: %dx%d\n",
			       geometry->base_width, geometry->base_height));
    }

  if (geom_mask & CDK_HINT_RESIZE_INC)
    {
      CDK_NOTE (MISC, g_print ("... RESIZE_INC: (%d,%d)\n",
			       geometry->width_inc, geometry->height_inc));
    }

  if (geom_mask & CDK_HINT_ASPECT)
    {
      CDK_NOTE (MISC, g_print ("... ASPECT: %g--%g\n",
			       geometry->min_aspect, geometry->max_aspect));
    }

  if (geom_mask & CDK_HINT_WIN_GRAVITY)
    {
      CDK_NOTE (MISC, g_print ("... GRAVITY: %d\n", geometry->win_gravity));
    }

  _cdk_win32_window_update_style_bits (window);
}

static void
cdk_win32_window_set_title (CdkWindow   *window,
		      const gchar *title)
{
  wchar_t *wtitle;

  g_return_if_fail (CDK_IS_WINDOW (window));
  g_return_if_fail (title != NULL);

  if (CDK_WINDOW_DESTROYED (window))
    return;

  /* Empty window titles not allowed, so set it to just a period. */
  if (!title[0])
    title = ".";

  CDK_NOTE (MISC, g_print ("cdk_window_set_title: %p: %s\n",
			   CDK_WINDOW_HWND (window), title));

  CDK_NOTE (MISC_OR_EVENTS, title = g_strdup_printf ("%p %s", CDK_WINDOW_HWND (window), title));

  wtitle = g_utf8_to_utf16 (title, -1, NULL, NULL, NULL);
  API_CALL (SetWindowTextW, (CDK_WINDOW_HWND (window), wtitle));
  g_free (wtitle);

  CDK_NOTE (MISC_OR_EVENTS, g_free ((char *) title));
}

static void
cdk_win32_window_set_role (CdkWindow   *window,
		     const gchar *role)
{
  g_return_if_fail (CDK_IS_WINDOW (window));

  CDK_NOTE (MISC, g_print ("cdk_window_set_role: %p: %s\n",
			   CDK_WINDOW_HWND (window),
			   (role ? role : "NULL")));
  /* XXX */
}

static void
cdk_win32_window_set_transient_for (CdkWindow *window,
			      CdkWindow *parent)
{
  HWND window_id, parent_id;
  LONG_PTR old_ptr;
  DWORD w32_error;
  CdkWindowImplWin32 *window_impl = CDK_WINDOW_IMPL_WIN32 (window->impl);
  CdkWindowImplWin32 *parent_impl = NULL;
  GSList *item;

  g_return_if_fail (CDK_IS_WINDOW (window));

  window_id = CDK_WINDOW_HWND (window);
  parent_id = parent != NULL ? CDK_WINDOW_HWND (parent) : NULL;

  CDK_NOTE (MISC, g_print ("cdk_window_set_transient_for: %p: %p\n", window_id, parent_id));

  if (CDK_WINDOW_DESTROYED (window) || (parent && CDK_WINDOW_DESTROYED (parent)))
    {
      if (CDK_WINDOW_DESTROYED (window))
	CDK_NOTE (MISC, g_print ("... destroyed!\n"));
      else
	CDK_NOTE (MISC, g_print ("... owner destroyed!\n"));

      return;
    }

  if (window->window_type == CDK_WINDOW_CHILD)
    {
      CDK_NOTE (MISC, g_print ("... a child window!\n"));
      return;
    }

  if (window_impl->transient_owner == parent)
    return;

  if (CDK_IS_WINDOW (window_impl->transient_owner))
    {
      CdkWindowImplWin32 *trans_impl = CDK_WINDOW_IMPL_WIN32 (window_impl->transient_owner->impl);
      item = g_slist_find (trans_impl->transient_children, window);
      item->data = NULL;
      trans_impl->transient_children = g_slist_delete_link (trans_impl->transient_children, item);
      trans_impl->num_transients--;

      if (!trans_impl->num_transients)
        {
          trans_impl->transient_children = NULL;
        }

      g_object_unref (G_OBJECT (window_impl->transient_owner));
      g_object_unref (G_OBJECT (window));

      window_impl->transient_owner = NULL;
    }

  if (parent)
    {
      parent_impl = CDK_WINDOW_IMPL_WIN32 (parent->impl);

      parent_impl->transient_children = g_slist_append (parent_impl->transient_children, window);
      g_object_ref (G_OBJECT (window));
      parent_impl->num_transients++;
      window_impl->transient_owner = parent;
      g_object_ref (G_OBJECT (parent));
    }

  SetLastError (0);
  old_ptr = GetWindowLongPtr (window_id, GWLP_HWNDPARENT);
  w32_error = GetLastError ();

  /* Don't re-set GWLP_HWNDPARENT to the same value */
  if ((HWND) old_ptr == parent_id && w32_error == NO_ERROR)
    return;

  /* Don't return if it failed, try SetWindowLongPtr() anyway */
  if (old_ptr == 0 && w32_error != NO_ERROR)
    WIN32_API_FAILED ("GetWindowLongPtr");

  /* This changes the *owner* of the window, despite the misleading
   * name. (Owner and parent are unrelated concepts.) At least that's
   * what people who seem to know what they talk about say on
   * USENET. Search on Google.
   */
  SetLastError (0);
  old_ptr = SetWindowLongPtr (window_id, GWLP_HWNDPARENT, (LONG_PTR) parent_id);
  w32_error = GetLastError ();

  if (old_ptr == 0 && w32_error != NO_ERROR)
    WIN32_API_FAILED ("SetWindowLongPtr");
}

void
_cdk_push_modal_window (CdkWindow *window)
{
  modal_window_stack = g_slist_prepend (modal_window_stack,
                                        window);
}

void
_cdk_remove_modal_window (CdkWindow *window)
{
  GSList *tmp;

  g_return_if_fail (window != NULL);

  /* It's possible to be NULL here if someone sets the modal hint of the window
   * to FALSE before a modal window stack has ever been created. */
  if (modal_window_stack == NULL)
    return;

  /* Find the requested window in the stack and remove it.  Yeah, I realize this
   * means we're not a 'real stack', strictly speaking.  Sue me. :) */
  tmp = g_slist_find (modal_window_stack, window);
  if (tmp != NULL)
    {
      modal_window_stack = g_slist_delete_link (modal_window_stack, tmp);
    }
}

gboolean
_cdk_modal_blocked (CdkWindow *window)
{
  GSList *l;
  gboolean found_any = FALSE;

  for (l = modal_window_stack; l != NULL; l = l->next)
    {
      CdkWindow *modal = l->data;

      if (modal == window)
	return FALSE;

      if (CDK_WINDOW_IS_MAPPED (modal))
	found_any = TRUE;
    }

  return found_any;
}

CdkWindow *
_cdk_modal_current (void)
{
  GSList *l;

  for (l = modal_window_stack; l != NULL; l = l->next)
    {
      CdkWindow *modal = l->data;

      if (CDK_WINDOW_IS_MAPPED (modal))
	return modal;
    }

  return NULL;
}

static void
cdk_win32_window_set_background (CdkWindow       *window,
				 cairo_pattern_t *pattern)
{
}

static void
cdk_win32_window_set_device_cursor (CdkWindow *window,
                                    CdkDevice *device,
                                    CdkCursor *cursor)
{
  CdkWindowImplWin32 *impl;
  CdkCursor *previous_cursor;

  impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  if (CDK_WINDOW_DESTROYED (window))
    return;

  CDK_NOTE (MISC, g_print ("cdk_win32_window_set_cursor: %p: %p\n",
			   CDK_WINDOW_HWND (window),
			   cursor));

  /* First get the old cursor, if any (we wait to free the old one
   * since it may be the current cursor set in the Win32 API right
   * now).
   */
  previous_cursor = impl->cursor;

  if (cursor)
    impl->cursor = g_object_ref (cursor);
  else
    /* Use default cursor otherwise. Don't just set NULL cursor,
     * because that will just hide the cursor, which is not
     * what the caller probably wanted.
     */
    impl->cursor = _cdk_win32_display_get_cursor_for_type (cdk_device_get_display (device),
                                                           CDK_LEFT_PTR);

  CDK_DEVICE_GET_CLASS (device)->set_window_cursor (device, window, impl->cursor);

  /* Destroy the previous cursor */
  if (previous_cursor != NULL)
    g_object_unref (previous_cursor);
}

static void
cdk_win32_window_get_geometry (CdkWindow *window,
			       gint      *x,
			       gint      *y,
			       gint      *width,
			       gint      *height)
{
  CdkScreen *screen;
  gboolean window_is_root;

  screen = cdk_window_get_screen (window);

  if (!window)
    {
      window = cdk_screen_get_root_window (screen);
      window_is_root = TRUE;
    }
  else
    window_is_root = (cdk_screen_get_root_window (screen) == window);

  if (!CDK_WINDOW_DESTROYED (window))
    {
      RECT rect;
      CdkWindowImplWin32 *impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

      API_CALL (GetClientRect, (CDK_WINDOW_HWND (window), &rect));

      if (!window_is_root)
	{
	  POINT pt;
	  CdkWindow *parent = cdk_window_get_parent (window);

	  pt.x = rect.left;
	  pt.y = rect.top;
	  ClientToScreen (CDK_WINDOW_HWND (window), &pt);
	  ScreenToClient (CDK_WINDOW_HWND (parent), &pt);
	  rect.left = pt.x;
	  rect.top = pt.y;

	  pt.x = rect.right;
	  pt.y = rect.bottom;
	  ClientToScreen (CDK_WINDOW_HWND (window), &pt);
	  ScreenToClient (CDK_WINDOW_HWND (parent), &pt);
	  rect.right = pt.x;
	  rect.bottom = pt.y;

	  if (cdk_screen_get_root_window (screen) == parent)
	    {
	      rect.left += _cdk_offset_x * impl->window_scale;
	      rect.top += _cdk_offset_y * impl->window_scale;
	      rect.right += _cdk_offset_x * impl->window_scale;
	      rect.bottom += _cdk_offset_y * impl->window_scale;
	    }
	}

      if (x)
	*x = rect.left / impl->window_scale;
      if (y)
	*y = rect.top / impl->window_scale;
      if (width)
	*width = (rect.right - rect.left) / impl->window_scale;
      if (height)
	*height = (rect.bottom - rect.top) / impl->window_scale;

      CDK_NOTE (MISC, g_print ("cdk_win32_window_get_geometry: %p: %ldx%ldx%d@%+ld%+ld\n",
			       CDK_WINDOW_HWND (window),
			       (rect.right - rect.left) / impl->window_scale,
             (rect.bottom - rect.top) / impl->window_scale,
			       cdk_window_get_visual (window)->depth,
			       rect.left, rect.top));
    }
}

static void
cdk_win32_window_get_root_coords (CdkWindow *window,
				  gint       x,
				  gint       y,
				  gint      *root_x,
				  gint      *root_y)
{
  gint tx;
  gint ty;
  POINT pt;
  CdkWindowImplWin32 *impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  pt.x = x * impl->window_scale;
  pt.y = y * impl->window_scale;
  ClientToScreen (CDK_WINDOW_HWND (window), &pt);
  tx = pt.x;
  ty = pt.y;

  if (root_x)
    *root_x = (tx + _cdk_offset_x) / impl->window_scale;
  if (root_y)
    *root_y = (ty + _cdk_offset_y) / impl->window_scale;

  CDK_NOTE (MISC, g_print ("cdk_win32_window_get_root_coords: %p: %+d%+d %+d%+d\n",
			   CDK_WINDOW_HWND (window),
			   x * impl->window_scale,
			   y * impl->window_scale,
			   (tx + _cdk_offset_x) / impl->window_scale,
			   (ty + _cdk_offset_y) / impl->window_scale));
}

static void
cdk_win32_window_restack_under (CdkWindow *window,
				GList *native_siblings)
{
	// ### TODO
}

static void
cdk_win32_window_restack_toplevel (CdkWindow *window,
				   CdkWindow *sibling,
				   gboolean   above)
{
	// ### TODO
}

static void
cdk_win32_window_get_frame_extents (CdkWindow    *window,
                              CdkRectangle *rect)
{
  HWND hwnd;
  RECT r;
  CdkWindowImplWin32 *impl;

  g_return_if_fail (CDK_IS_WINDOW (window));
  g_return_if_fail (rect != NULL);

  rect->x = 0;
  rect->y = 0;
  rect->width = 1;
  rect->height = 1;

  if (CDK_WINDOW_DESTROYED (window))
    return;

  /* FIXME: window is documented to be a toplevel CdkWindow, so is it really
   * necessary to walk its parent chain?
   */
  while (window->parent && window->parent->parent)
    window = window->parent;

  impl = CDK_WINDOW_IMPL_WIN32 (window->impl);
  hwnd = CDK_WINDOW_HWND (window);
  API_CALL (GetWindowRect, (hwnd, &r));

  /* Initialize to real, unscaled size */
  rect->x = r.left + _cdk_offset_x * impl->window_scale;
  rect->y = r.top + _cdk_offset_y * impl->window_scale;
  rect->width = (r.right - r.left);
  rect->height = (r.bottom - r.top);

  /* Extend width and height to ensure that they cover the real size when de-scaled,
   * and replace everyting with scaled values
   */
  rect->width = (rect->width + rect->x % impl->window_scale + impl->window_scale - 1) / impl->window_scale;
  rect->height = (rect->height + rect->y % impl->window_scale + impl->window_scale - 1) / impl->window_scale;
  rect->x = r.left / impl->window_scale + _cdk_offset_x;
  rect->y = r.top / impl->window_scale + _cdk_offset_y;

  CDK_NOTE (MISC, g_print ("cdk_window_get_frame_extents: %p: %ldx%ld@%+ld%+ld\n",
                           CDK_WINDOW_HWND (window),
                           rect->width,
                           rect->height,
                           rect->x, rect->y));
}

static gboolean
cdk_window_win32_get_device_state (CdkWindow       *window,
                                   CdkDevice       *device,
                                   gdouble         *x,
                                   gdouble         *y,
                                   CdkModifierType *mask)
{
  CdkWindow *child;

  g_return_val_if_fail (window == NULL || CDK_IS_WINDOW (window), FALSE);

  CDK_DEVICE_GET_CLASS (device)->query_state (device, window,
                                              NULL, &child,
                                              NULL, NULL,
                                              x, y, mask);
  return (child != NULL);
}

void
cdk_display_warp_device (CdkDisplay *display,
                         CdkDevice  *device,
                         CdkScreen  *screen,
                         gint        x,
                         gint        y)
{
  g_return_if_fail (display == cdk_display_get_default ());
  g_return_if_fail (screen == cdk_display_get_default_screen (display));
  g_return_if_fail (CDK_IS_DEVICE (device));
  g_return_if_fail (display == cdk_device_get_display (device));

  CDK_DEVICE_GET_CLASS (device)->warp (device, screen, x, y);
}

static CdkEventMask
cdk_win32_window_get_events (CdkWindow *window)
{
  CdkWindowImplWin32 *impl;

  if (CDK_WINDOW_DESTROYED (window))
    return 0;

  impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  return impl->native_event_mask;
}

static void
cdk_win32_window_set_events (CdkWindow   *window,
			     CdkEventMask event_mask)
{
  CdkWindowImplWin32 *impl;

  impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  /* cdk_window_new() always sets the CDK_STRUCTURE_MASK, so better
   * set it here, too. Not that I know or remember why it is
   * necessary, will have to test some day.
   */
  impl->native_event_mask = CDK_STRUCTURE_MASK | event_mask;
}

static void
do_shape_combine_region (CdkWindow *window,
			 HRGN	    hrgn,
			 gint       x, gint y)
{
  RECT rect;
  CdkWindowImplWin32 *impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  GetClientRect (CDK_WINDOW_HWND (window), &rect);

  _cdk_win32_adjust_client_rect (window, &rect);

  OffsetRgn (hrgn, -rect.left, -rect.top);
  OffsetRgn (hrgn, x, y);

  /* If this is a top-level window, add the title bar to the region */
  if (CDK_WINDOW_TYPE (window) == CDK_WINDOW_TOPLEVEL)
    {
      HRGN tmp = CreateRectRgn (0, 0, rect.right - rect.left, -rect.top);
      CombineRgn (hrgn, hrgn, tmp, RGN_OR);
      DeleteObject (tmp);
    }

  SetWindowRgn (CDK_WINDOW_HWND (window), hrgn, TRUE);
}

static void
cdk_win32_window_set_override_redirect (CdkWindow *window,
				  gboolean   override_redirect)
{
  CdkWindowImplWin32 *window_impl;

  g_return_if_fail (CDK_IS_WINDOW (window));

  window_impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  window_impl->override_redirect = !!override_redirect;
}

static void
cdk_win32_window_set_accept_focus (CdkWindow *window,
			     gboolean accept_focus)
{
  g_return_if_fail (CDK_IS_WINDOW (window));

  accept_focus = accept_focus != FALSE;

  if (window->accept_focus != accept_focus)
    window->accept_focus = accept_focus;
}

static void
cdk_win32_window_set_focus_on_map (CdkWindow *window,
			     gboolean focus_on_map)
{
  g_return_if_fail (CDK_IS_WINDOW (window));

  focus_on_map = focus_on_map != FALSE;

  if (window->focus_on_map != focus_on_map)
    window->focus_on_map = focus_on_map;
}

static void
cdk_win32_window_set_icon_list (CdkWindow *window,
			  GList     *pixbufs)
{
  GdkPixbuf *pixbuf, *big_pixbuf, *small_pixbuf;
  gint big_diff, small_diff;
  gint big_w, big_h, small_w, small_h;
  gint w, h;
  gint dw, dh, diff;
  HICON small_hicon, big_hicon;
  CdkWindowImplWin32 *impl;

  g_return_if_fail (CDK_IS_WINDOW (window));

  if (CDK_WINDOW_DESTROYED (window))
    return;

  impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  /* ideal sizes for small and large icons */
  big_w = GetSystemMetrics (SM_CXICON);
  big_h = GetSystemMetrics (SM_CYICON);
  small_w = GetSystemMetrics (SM_CXSMICON);
  small_h = GetSystemMetrics (SM_CYSMICON);

  /* find closest sized icons in the list */
  big_pixbuf = NULL;
  small_pixbuf = NULL;
  big_diff = 0;
  small_diff = 0;
  while (pixbufs)
    {
      pixbuf = (GdkPixbuf*) pixbufs->data;
      w = gdk_pixbuf_get_width (pixbuf);
      h = gdk_pixbuf_get_height (pixbuf);

      dw = ABS (w - big_w);
      dh = ABS (h - big_h);
      diff = dw*dw + dh*dh;
      if (big_pixbuf == NULL || diff < big_diff)
	{
	  big_pixbuf = pixbuf;
	  big_diff = diff;
	}

      dw = ABS (w - small_w);
      dh = ABS (h - small_h);
      diff = dw*dw + dh*dh;
      if (small_pixbuf == NULL || diff < small_diff)
	{
	  small_pixbuf = pixbuf;
	  small_diff = diff;
	}

      pixbufs = pixbufs->next;
    }

  /* Create the icons */
  big_hicon = _cdk_win32_pixbuf_to_hicon (big_pixbuf);
  small_hicon = _cdk_win32_pixbuf_to_hicon (small_pixbuf);

  /* Set the icons */
  SendMessageW (CDK_WINDOW_HWND (window), WM_SETICON, ICON_BIG,
		(LPARAM)big_hicon);
  SendMessageW (CDK_WINDOW_HWND (window), WM_SETICON, ICON_SMALL,
		(LPARAM)small_hicon);

  /* Store the icons, destroying any previous icons */
  if (impl->hicon_big)
    GDI_CALL (DestroyIcon, (impl->hicon_big));
  impl->hicon_big = big_hicon;
  if (impl->hicon_small)
    GDI_CALL (DestroyIcon, (impl->hicon_small));
  impl->hicon_small = small_hicon;
}

static void
cdk_win32_window_set_icon_name (CdkWindow   *window,
			  const gchar *name)
{
  /* In case I manage to confuse this again (or somebody else does):
   * Please note that "icon name" here really *does* mean the name or
   * title of an window minimized as an icon on the desktop, or in the
   * taskbar. It has nothing to do with the freedesktop.org icon
   * naming stuff.
   */

  g_return_if_fail (CDK_IS_WINDOW (window));

  if (CDK_WINDOW_DESTROYED (window))
    return;

#if 0
  /* This is not the correct thing to do. We should keep both the
   * "normal" window title, and the icon name. When the window is
   * minimized, call SetWindowText() with the icon name, and when the
   * window is restored, with the normal window title. Also, the name
   * is in UTF-8, so we should do the normal conversion to either wide
   * chars or system codepage, and use either the W or A version of
   * SetWindowText(), depending on Windows version.
   */
  API_CALL (SetWindowText, (CDK_WINDOW_HWND (window), name));
#endif
}

static CdkWindow *
cdk_win32_window_get_group (CdkWindow *window)
{
  g_return_val_if_fail (CDK_IS_WINDOW (window), NULL);
  g_return_val_if_fail (CDK_WINDOW_TYPE (window) != CDK_WINDOW_CHILD, NULL);

  if (CDK_WINDOW_DESTROYED (window))
    return NULL;

  g_warning ("cdk_window_get_group not yet implemented");

  return NULL;
}

static void
cdk_win32_window_set_group (CdkWindow *window,
		      CdkWindow *leader)
{
  g_return_if_fail (CDK_IS_WINDOW (window));
  g_return_if_fail (CDK_WINDOW_TYPE (window) != CDK_WINDOW_CHILD);
  g_return_if_fail (leader == NULL || CDK_IS_WINDOW (leader));

  if (CDK_WINDOW_DESTROYED (window) || CDK_WINDOW_DESTROYED (leader))
    return;

  g_warning ("cdk_window_set_group not implemented");
}

static void
update_single_bit (LONG    *style,
                   gboolean all,
		   int      cdk_bit,
		   int      style_bit)
{
  /* all controls the interpretation of cdk_bit -- if all is TRUE,
   * cdk_bit indicates whether style_bit is off; if all is FALSE, cdk
   * bit indicate whether style_bit is on
   */
  if ((!all && cdk_bit) || (all && !cdk_bit))
    *style |= style_bit;
  else
    *style &= ~style_bit;
}

/*
 * Returns TRUE if window has no decorations.
 * Usually it means CSD windows, because CTK
 * calls cdk_window_set_decorations (window, 0);
 * This is used to decide whether a toplevel should
 * be made layered, thus it
 * only returns TRUE for toplevels (until CTK minimal
 * system requirements are lifted to Windows 8 or newer,
 * because only toplevels can be layered).
 */
gboolean
_cdk_win32_window_lacks_wm_decorations (CdkWindow *window)
{
  CdkWindowImplWin32 *impl;
  LONG style;
  gboolean has_any_decorations;

  if (CDK_WINDOW_DESTROYED (window))
    return FALSE;

  /* only toplevels can be layered */
  if (!WINDOW_IS_TOPLEVEL (window))
    return FALSE;

  impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  /* This is because CTK calls cdk_window_set_decorations (window, 0),
   * even though CdkWMDecoration docs indicate that 0 does NOT mean
   * "no decorations".
   */
  if (impl->decorations &&
      *impl->decorations == 0)
    return TRUE;

  if (CDK_WINDOW_HWND (window) == 0)
    return FALSE;

  style = GetWindowLong (CDK_WINDOW_HWND (window), GWL_STYLE);

  if (style == 0)
    {
      DWORD w32_error = GetLastError ();

      CDK_NOTE (MISC, g_print ("Failed to get style of window %p (handle %p): %lu\n",
                               window, CDK_WINDOW_HWND (window), w32_error));
      return FALSE;
    }

  /* Keep this in sync with _cdk_win32_window_update_style_bits() */
  /* We don't check what get_effective_window_decorations()
   * has to say, because it gives suggestions based on
   * various hints, while we want *actual* decorations,
   * or their absence.
   */
  has_any_decorations = FALSE;

  if (style & (WS_BORDER | WS_THICKFRAME | WS_CAPTION |
               WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX))
    has_any_decorations = TRUE;
  else
    CDK_NOTE (MISC, g_print ("Window %p (handle %p): has no decorations (style %lx)\n",
                             window, CDK_WINDOW_HWND (window), style));

  return !has_any_decorations;
}

void
_cdk_win32_window_update_style_bits (CdkWindow *window)
{
  CdkWindowImplWin32 *impl = (CdkWindowImplWin32 *)window->impl;
  CdkWMDecoration decorations;
  LONG old_style, new_style, old_exstyle, new_exstyle;
  gboolean all;
  RECT rect, before, after;
  gboolean was_topmost;
  gboolean will_be_topmost;
  HWND insert_after;
  UINT flags;

  if (window->state & CDK_WINDOW_STATE_FULLSCREEN)
    return;

  old_style = GetWindowLong (CDK_WINDOW_HWND (window), GWL_STYLE);
  old_exstyle = GetWindowLong (CDK_WINDOW_HWND (window), GWL_EXSTYLE);

  GetClientRect (CDK_WINDOW_HWND (window), &before);
  after = before;
  AdjustWindowRectEx (&before, old_style, FALSE, old_exstyle);

  was_topmost = (old_exstyle & WS_EX_TOPMOST) ? TRUE : FALSE;
  will_be_topmost = was_topmost;

  old_exstyle &= ~WS_EX_TOPMOST;

  new_style = old_style;
  new_exstyle = old_exstyle;

  if (window->window_type == CDK_WINDOW_TEMP)
    {
      new_exstyle |= WS_EX_TOOLWINDOW;
      will_be_topmost = TRUE;
    }
  else if (impl->type_hint == CDK_WINDOW_TYPE_HINT_UTILITY)
    {
      new_exstyle |= WS_EX_TOOLWINDOW;
    }
  else
    {
      new_exstyle &= ~WS_EX_TOOLWINDOW;
    }

  /* We can get away with using layered windows
   * only when no decorations are needed. It can mean
   * CSD or borderless non-CSD windows (tooltips?).
   *
   * If this window cannot use layered windows, disable it always.
   * This currently applies to windows using OpenGL, which
   * does not work with layered windows.
   */
  if (impl->suppress_layered == 0)
    {
      if (_cdk_win32_window_lacks_wm_decorations (window))
        impl->layered = g_strcmp0 (g_getenv ("CDK_WIN32_LAYERED"), "0") != 0;
    }
  else
    impl->layered = FALSE;

  if (impl->layered)
    new_exstyle |= WS_EX_LAYERED;
  else
    new_exstyle &= ~WS_EX_LAYERED;

  if (get_effective_window_decorations (window, &decorations))
    {
      all = (decorations & CDK_DECOR_ALL);
      /* Keep this in sync with the test in _cdk_win32_window_lacks_wm_decorations() */
      update_single_bit (&new_style, all, decorations & CDK_DECOR_BORDER, WS_BORDER);
      update_single_bit (&new_style, all, decorations & CDK_DECOR_RESIZEH, WS_THICKFRAME);
      update_single_bit (&new_style, all, decorations & CDK_DECOR_TITLE, WS_CAPTION);
      update_single_bit (&new_style, all, decorations & CDK_DECOR_MENU, WS_SYSMENU);
      update_single_bit (&new_style, all, decorations & CDK_DECOR_MINIMIZE, WS_MINIMIZEBOX);
      update_single_bit (&new_style, all, decorations & CDK_DECOR_MAXIMIZE, WS_MAXIMIZEBOX);
    }

  if (old_style == new_style && old_exstyle == new_exstyle )
    {
      CDK_NOTE (MISC, g_print ("_cdk_win32_window_update_style_bits: %p: no change\n",
			       CDK_WINDOW_HWND (window)));
      return;
    }

  if (old_style != new_style)
    {
      CDK_NOTE (MISC, g_print ("_cdk_win32_window_update_style_bits: %p: STYLE: %s => %s\n",
			       CDK_WINDOW_HWND (window),
			       _cdk_win32_window_style_to_string (old_style),
			       _cdk_win32_window_style_to_string (new_style)));

      SetWindowLong (CDK_WINDOW_HWND (window), GWL_STYLE, new_style);
    }

  if (old_exstyle != new_exstyle)
    {
      CDK_NOTE (MISC, g_print ("_cdk_win32_window_update_style_bits: %p: EXSTYLE: %s => %s\n",
			       CDK_WINDOW_HWND (window),
			       _cdk_win32_window_exstyle_to_string (old_exstyle),
			       _cdk_win32_window_exstyle_to_string (new_exstyle)));

      SetWindowLong (CDK_WINDOW_HWND (window), GWL_EXSTYLE, new_exstyle);
    }

  AdjustWindowRectEx (&after, new_style, FALSE, new_exstyle);

  GetWindowRect (CDK_WINDOW_HWND (window), &rect);
  rect.left += after.left - before.left;
  rect.top += after.top - before.top;
  rect.right += after.right - before.right;
  rect.bottom += after.bottom - before.bottom;

  flags = SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOREPOSITION;

  if (will_be_topmost && !was_topmost)
    {
      insert_after = HWND_TOPMOST;
    }
  else if (was_topmost && !will_be_topmost)
    {
      insert_after = HWND_NOTOPMOST;
    }
  else
    {
      flags |= SWP_NOZORDER;
      insert_after = SWP_NOZORDER_SPECIFIED;
    }

  SetWindowPos (CDK_WINDOW_HWND (window), insert_after,
		rect.left, rect.top,
		rect.right - rect.left, rect.bottom - rect.top,
		flags);
}

static void
update_single_system_menu_entry (HMENU    hmenu,
				 gboolean all,
				 int      cdk_bit,
				 int      menu_entry)
{
  /* all controls the interpretation of cdk_bit -- if all is TRUE,
   * cdk_bit indicates whether menu entry is disabled; if all is
   * FALSE, cdk bit indicate whether menu entry is enabled
   */
  if ((!all && cdk_bit) || (all && !cdk_bit))
    EnableMenuItem (hmenu, menu_entry, MF_BYCOMMAND | MF_ENABLED);
  else
    EnableMenuItem (hmenu, menu_entry, MF_BYCOMMAND | MF_GRAYED);
}

static void
update_system_menu (CdkWindow *window)
{
  CdkWMFunction functions;
  BOOL all;

  if (_cdk_window_get_functions (window, &functions))
    {
      HMENU hmenu = GetSystemMenu (CDK_WINDOW_HWND (window), FALSE);

      all = (functions & CDK_FUNC_ALL);
      update_single_system_menu_entry (hmenu, all, functions & CDK_FUNC_RESIZE, SC_SIZE);
      update_single_system_menu_entry (hmenu, all, functions & CDK_FUNC_MOVE, SC_MOVE);
      update_single_system_menu_entry (hmenu, all, functions & CDK_FUNC_MINIMIZE, SC_MINIMIZE);
      update_single_system_menu_entry (hmenu, all, functions & CDK_FUNC_MAXIMIZE, SC_MAXIMIZE);
      update_single_system_menu_entry (hmenu, all, functions & CDK_FUNC_CLOSE, SC_CLOSE);
    }
}

static void
cdk_win32_window_set_decorations (CdkWindow      *window,
				  CdkWMDecoration decorations)
{
  CdkWindowImplWin32 *impl;

  g_return_if_fail (CDK_IS_WINDOW (window));

  impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  CDK_NOTE (MISC, g_print ("cdk_window_set_decorations: %p: %s %s%s%s%s%s%s\n",
			   CDK_WINDOW_HWND (window),
			   (decorations & CDK_DECOR_ALL ? "clearing" : "setting"),
			   (decorations & CDK_DECOR_BORDER ? "BORDER " : ""),
			   (decorations & CDK_DECOR_RESIZEH ? "RESIZEH " : ""),
			   (decorations & CDK_DECOR_TITLE ? "TITLE " : ""),
			   (decorations & CDK_DECOR_MENU ? "MENU " : ""),
			   (decorations & CDK_DECOR_MINIMIZE ? "MINIMIZE " : ""),
			   (decorations & CDK_DECOR_MAXIMIZE ? "MAXIMIZE " : "")));

  if (!impl->decorations)
    impl->decorations = g_malloc (sizeof (CdkWMDecoration));

  *impl->decorations = decorations;

  _cdk_win32_window_update_style_bits (window);
}

static gboolean
cdk_win32_window_get_decorations (CdkWindow       *window,
				  CdkWMDecoration *decorations)
{
  CdkWindowImplWin32 *impl;

  g_return_val_if_fail (CDK_IS_WINDOW (window), FALSE);

  impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  if (impl->decorations == NULL)
    return FALSE;

  *decorations = *impl->decorations;

  return TRUE;
}

static GQuark
get_functions_quark ()
{
  static GQuark quark = 0;

  if (!quark)
    quark = g_quark_from_static_string ("cdk-window-functions");

  return quark;
}

static void
cdk_win32_window_set_functions (CdkWindow    *window,
			  CdkWMFunction functions)
{
  CdkWMFunction* functions_copy;

  g_return_if_fail (CDK_IS_WINDOW (window));

  CDK_NOTE (MISC, g_print ("cdk_window_set_functions: %p: %s %s%s%s%s%s\n",
			   CDK_WINDOW_HWND (window),
			   (functions & CDK_FUNC_ALL ? "clearing" : "setting"),
			   (functions & CDK_FUNC_RESIZE ? "RESIZE " : ""),
			   (functions & CDK_FUNC_MOVE ? "MOVE " : ""),
			   (functions & CDK_FUNC_MINIMIZE ? "MINIMIZE " : ""),
			   (functions & CDK_FUNC_MAXIMIZE ? "MAXIMIZE " : ""),
			   (functions & CDK_FUNC_CLOSE ? "CLOSE " : "")));

  functions_copy = g_malloc (sizeof (CdkWMFunction));
  *functions_copy = functions;
  g_object_set_qdata_full (G_OBJECT (window), get_functions_quark (), functions_copy, g_free);

  update_system_menu (window);
}

gboolean
_cdk_window_get_functions (CdkWindow     *window,
		           CdkWMFunction *functions)
{
  CdkWMFunction* functions_set;

  functions_set = g_object_get_qdata (G_OBJECT (window), get_functions_quark ());
  if (functions_set)
    *functions = *functions_set;

  return (functions_set != NULL);
}

#if defined(MORE_AEROSNAP_DEBUGGING)
static void
log_region (gchar *prefix, AeroSnapEdgeRegion *region)
{
  CDK_NOTE (MISC, g_print ("Region %s:\n"
                           "edge %d x %d @ %d x %d\n"
                           "trig %d x %d @ %d x %d\n",
                           prefix,
                           region->edge.width,
                           region->edge.height,
                           region->edge.x,
                           region->edge.y,
                           region->trigger.width,
                           region->trigger.height,
                           region->trigger.x,
                           region->trigger.y));
}
#endif

static void
calculate_aerosnap_regions (CdkW32DragMoveResizeContext *context)
{
  CdkDisplay *display;
  CdkScreen *screen;
  gint n_monitors, monitor, other_monitor;
  CdkWindowImplWin32 *impl = CDK_WINDOW_IMPL_WIN32 (context->window->impl);
#if defined(MORE_AEROSNAP_DEBUGGING)
  gint i;
#endif

  display = cdk_display_get_default ();
  screen = cdk_display_get_default_screen (display);
  n_monitors = cdk_screen_get_n_monitors (screen);

#define _M_UP 0
#define _M_DOWN 1
#define _M_LEFT 2
#define _M_RIGHT 3

  for (monitor = 0; monitor < n_monitors; monitor++)
    {
      CdkRectangle wa;
      CdkRectangle geometry;
      AeroSnapEdgeRegion snap_region;
      gboolean move_edge[4] = { TRUE, FALSE, TRUE, TRUE };
      gboolean resize_edge[2] = { TRUE, TRUE };
      gint diff;
      gint thickness, trigger_thickness;

      cdk_screen_get_monitor_workarea (screen, monitor, &wa);
      cdk_screen_get_monitor_geometry (screen, monitor, &geometry);

      for (other_monitor = 0;
           other_monitor < n_monitors &&
           (move_edge[_M_UP] || move_edge[_M_LEFT] ||
           move_edge[_M_RIGHT] || resize_edge[_M_DOWN]);
           other_monitor++)
        {
          CdkRectangle other_wa;

          if (other_monitor == monitor)
            continue;

          cdk_screen_get_monitor_workarea (screen, other_monitor, &other_wa);

          /* An edge triggers AeroSnap only if there are no
           * monitors beyond that edge.
           * Even if there's another monitor, but it does not cover
           * the whole edge (it's smaller or is not aligned to
           * the corner of current monitor), that edge is still
           * removed from the trigger list.
           */
          if (other_wa.x >= wa.x + wa.width)
            move_edge[_M_RIGHT] = FALSE;

          if (other_wa.x + other_wa.width <= wa.x)
            move_edge[_M_LEFT] = FALSE;

          if (other_wa.y + other_wa.height <= wa.y)
            {
              move_edge[_M_UP] = FALSE;
              resize_edge[_M_UP] = FALSE;
            }

          if (other_wa.y >= wa.y + wa.height)
            {
              /* no move_edge for the bottom edge, just resize_edge */
              resize_edge[_M_DOWN] = FALSE;
            }
        }

      thickness = AEROSNAP_REGION_THICKNESS * impl->window_scale;
      trigger_thickness = AEROSNAP_REGION_TRIGGER_THICKNESS * impl->window_scale;

      snap_region.edge = wa;
      snap_region.trigger = wa;
      snap_region.edge.height = thickness;
      snap_region.trigger.height = trigger_thickness;

      /* Extend both regions into toolbar space.
       * When there's no toolbar, diff == 0.
       */
      diff = wa.y - geometry.y;
      snap_region.edge.height += diff;
      snap_region.edge.y -= diff;
      snap_region.trigger.height += diff;
      snap_region.trigger.y -= diff;

      if (move_edge[_M_UP])
        g_array_append_val (context->maximize_regions, snap_region);

      if (resize_edge[_M_UP])
        g_array_append_val (context->fullup_regions, snap_region);

      snap_region.edge = wa;
      snap_region.trigger = wa;
      snap_region.edge.width = thickness;
      snap_region.trigger.width = trigger_thickness;

      diff = wa.x - geometry.x;
      snap_region.edge.width += diff;
      snap_region.edge.x -= diff;
      snap_region.trigger.width += diff;
      snap_region.trigger.x -= diff;

      if (move_edge[_M_LEFT])
        g_array_append_val (context->halfleft_regions, snap_region);

      snap_region.edge = wa;
      snap_region.trigger = wa;
      snap_region.edge.x += wa.width - thickness;
      snap_region.edge.width = thickness;
      snap_region.trigger.x += wa.width - trigger_thickness;
      snap_region.trigger.width = trigger_thickness;

      diff = (geometry.x + geometry.width) - (wa.x + wa.width);
      snap_region.edge.width += diff;
      snap_region.trigger.width += diff;

      if (move_edge[_M_RIGHT])
        g_array_append_val (context->halfright_regions, snap_region);

      snap_region.edge = wa;
      snap_region.trigger = wa;
      snap_region.edge.y += wa.height - thickness;
      snap_region.edge.height = thickness;
      snap_region.trigger.y += wa.height - trigger_thickness;
      snap_region.trigger.height = trigger_thickness;

      diff = (geometry.y + geometry.height) - (wa.y + wa.height);
      snap_region.edge.height += diff;
      snap_region.trigger.height += diff;

      if (resize_edge[_M_DOWN])
        g_array_append_val (context->fullup_regions, snap_region);
    }

#undef _M_UP
#undef _M_DOWN
#undef _M_LEFT
#undef _M_RIGHT

#if defined(MORE_AEROSNAP_DEBUGGING)
  for (i = 0; i < context->maximize_regions->len; i++)
    log_region ("maximize", &g_array_index (context->maximize_regions, AeroSnapEdgeRegion, i));

  for (i = 0; i < context->halfleft_regions->len; i++)
    log_region ("halfleft", &g_array_index (context->halfleft_regions, AeroSnapEdgeRegion, i));

  for (i = 0; i < context->halfright_regions->len; i++)
    log_region ("halfright", &g_array_index (context->halfright_regions, AeroSnapEdgeRegion, i));

  for (i = 0; i < context->fullup_regions->len; i++)
    log_region ("fullup", &g_array_index (context->fullup_regions, AeroSnapEdgeRegion, i));
#endif
}

static void
discard_snapinfo (CdkWindow *window)
{
  CdkWindowImplWin32 *impl;

  impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  impl->snap_state = CDK_WIN32_AEROSNAP_STATE_UNDETERMINED;

  if (impl->snap_stash == NULL)
    return;

  g_clear_pointer (&impl->snap_stash, g_free);
  g_clear_pointer (&impl->snap_stash_int, g_free);
}

static void
unsnap (CdkWindow  *window,
        CdkScreen  *screen,
        gint        monitor)
{
  CdkWindowImplWin32 *impl;
  CdkRectangle rect;

  impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  impl->snap_state = CDK_WIN32_AEROSNAP_STATE_UNDETERMINED;

  if (impl->snap_stash == NULL)
    return;

  cdk_screen_get_monitor_workarea (screen, monitor, &rect);

  CDK_NOTE (MISC, g_print ("Monitor work area %d x %d @ %d : %d\n", rect.width, rect.height, rect.x, rect.y));

  if (rect.width >= impl->snap_stash_int->width &&
      rect.height >= impl->snap_stash_int->height)
    {
      /* If the window fits into new work area without resizing it,
       * place it into new work area without resizing it.
       */
      gdouble left, right, up, down, hratio, vratio;
      gdouble hscale, vscale;
      gdouble new_left, new_up;

      left = impl->snap_stash->x;
      right = 1.0 - (impl->snap_stash->x + impl->snap_stash->width);
      up = impl->snap_stash->y;
      down = 1.0 - (impl->snap_stash->y + impl->snap_stash->height);
      hscale = 1.0;

      if (right > 0.001)
        {
          hratio = left / right;
          hscale = hratio / (1.0 + hratio);
        }

      new_left = (gdouble) (rect.width - impl->snap_stash_int->width) * hscale;

      vscale = 1.0;

      if (down > 0.001)
        {
          vratio = up / down;
          vscale = vratio / (1.0 + vratio);
        }

      new_up = (gdouble) (rect.height - impl->snap_stash_int->height) * vscale;

      rect.x = round (rect.x + new_left);
      rect.y = round (rect.y + new_up);
      rect.width = impl->snap_stash_int->width;
      rect.height = impl->snap_stash_int->height;
    }
  else
    {
      /* Calculate actual unsnapped window size based on its
       * old relative size. Same for position.
       */
      rect.x += round (rect.width * impl->snap_stash->x);
      rect.y += round (rect.height * impl->snap_stash->y);
      rect.width = round (rect.width * impl->snap_stash->width);
      rect.height = round (rect.height * impl->snap_stash->height);
    }

  CDK_NOTE (MISC, g_print ("Unsnapped window size %d x %d @ %d : %d\n", rect.width, rect.height, rect.x, rect.y));

  cdk_window_move_resize (window, rect.x, rect.y,
                          rect.width, rect.height);

  g_clear_pointer (&impl->snap_stash, g_free);
  g_clear_pointer (&impl->snap_stash_int, g_free);
}

static void
stash_window (CdkWindow          *window,
              CdkWindowImplWin32 *impl,
              CdkScreen          *screen,
              gint                monitor)
{
  gint x, y;
  gint width, wwidth;
  gint height, wheight;
  WINDOWPLACEMENT placement;
  HMONITOR hmonitor;
  MONITORINFO hmonitor_info;

  placement.length = sizeof(WINDOWPLACEMENT);

  /* Use W32 API to get unmaximized window size, which CDK doesn't remember */
  if (!GetWindowPlacement (CDK_WINDOW_HWND (window), &placement))
    return;

  /* MSDN is very vague, but in practice rcNormalPosition is the same as GetWindowRect(),
   * only with adjustments for toolbars (which creates rather weird coodinate space issues).
   * We need to get monitor info and apply workarea vs monitorarea diff to turn
   * these into screen coordinates proper.
   */
  hmonitor = MonitorFromWindow (CDK_WINDOW_HWND (window), MONITOR_DEFAULTTONEAREST);
  hmonitor_info.cbSize = sizeof (hmonitor_info);

  if (!GetMonitorInfoA (hmonitor, &hmonitor_info))
    return;

  if (impl->snap_stash == NULL)
    impl->snap_stash = g_new0 (CdkRectangleDouble, 1);

  if (impl->snap_stash_int == NULL)
    impl->snap_stash_int = g_new0 (CdkRectangle, 1);

  CDK_NOTE (MISC, g_print ("monitor work area  %ld x %ld @ %ld : %ld\n",
                           (hmonitor_info.rcWork.right - hmonitor_info.rcWork.left) / impl->window_scale,
                           (hmonitor_info.rcWork.bottom - hmonitor_info.rcWork.top) / impl->window_scale,
                           hmonitor_info.rcWork.left,
                           hmonitor_info.rcWork.top));
  CDK_NOTE (MISC, g_print ("monitor      area  %ld x %ld @ %ld : %ld\n",
                           (hmonitor_info.rcMonitor.right - hmonitor_info.rcMonitor.left) / impl->window_scale,
                           (hmonitor_info.rcMonitor.bottom - hmonitor_info.rcMonitor.top) / impl->window_scale,
                           hmonitor_info.rcMonitor.left,
                           hmonitor_info.rcMonitor.top));
  CDK_NOTE (MISC, g_print ("window  work place %ld x %ld @ %ld : %ld\n",
                           (placement.rcNormalPosition.right - placement.rcNormalPosition.left) / impl->window_scale,
                           (placement.rcNormalPosition.bottom - placement.rcNormalPosition.top) / impl->window_scale,
                           placement.rcNormalPosition.left,
                           placement.rcNormalPosition.top));

  width = (placement.rcNormalPosition.right - placement.rcNormalPosition.left) / impl->window_scale;
  height = (placement.rcNormalPosition.bottom - placement.rcNormalPosition.top) / impl->window_scale;
  x = (placement.rcNormalPosition.left - hmonitor_info.rcMonitor.left) / impl->window_scale;
  y = (placement.rcNormalPosition.top - hmonitor_info.rcMonitor.top) / impl->window_scale;

  wwidth = (hmonitor_info.rcWork.right - hmonitor_info.rcWork.left) / impl->window_scale;
  wheight = (hmonitor_info.rcWork.bottom - hmonitor_info.rcWork.top) / impl->window_scale;

  impl->snap_stash->x = (gdouble) (x) / (gdouble) (wwidth);
  impl->snap_stash->y = (gdouble) (y) / (gdouble) (wheight);
  impl->snap_stash->width = (gdouble) width / (gdouble) (wwidth);
  impl->snap_stash->height = (gdouble) height / (gdouble) (wheight);

  impl->snap_stash_int->x = x;
  impl->snap_stash_int->y = y;
  impl->snap_stash_int->width = width;
  impl->snap_stash_int->height = height;

  CDK_NOTE (MISC, g_print ("Stashed window %d x %d @ %d : %d as %f x %f @ %f : %f\n",
                           width, height, x, y,
                           impl->snap_stash->width, impl->snap_stash->height, impl->snap_stash->x, impl->snap_stash->y));
}

static void
snap_up (CdkWindow *window,
         CdkScreen *screen,
         gint       monitor)
{
  SHORT maxysize;
  gint x, y;
  gint width, height;
  CdkWindowImplWin32 *impl;

  impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  impl->snap_state = CDK_WIN32_AEROSNAP_STATE_FULLUP;

  stash_window (window, impl, screen, monitor);

  maxysize = GetSystemMetrics (SM_CYVIRTUALSCREEN) / impl->window_scale;
  cdk_window_get_position (window, &x, &y);
  width = cdk_window_get_width (window);

  y = 0;
  height = maxysize;

  x = x - impl->margins.left;
  y = y - impl->margins.top;
  width += impl->margins_x;
  height += impl->margins_y;

  cdk_window_move_resize (window, x, y, width, height);
}

static void
snap_left (CdkWindow *window,
           CdkScreen *screen,
           gint       monitor,
           gint       snap_monitor)
{
  CdkRectangle rect;
  CdkWindowImplWin32 *impl;

  impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  impl->snap_state = CDK_WIN32_AEROSNAP_STATE_HALFLEFT;

  cdk_screen_get_monitor_workarea (screen, snap_monitor, &rect);

  stash_window (window, impl, screen, monitor);

  rect.width = rect.width / 2;

  rect.x = rect.x - impl->margins.left;
  rect.y = rect.y - impl->margins.top;
  rect.width = rect.width + impl->margins_x;
  rect.height = rect.height + impl->margins_y;

  cdk_window_move_resize (window, rect.x, rect.y, rect.width, rect.height);
}

static void
snap_right (CdkWindow *window,
            CdkScreen *screen,
            gint       monitor,
            gint       snap_monitor)
{
  CdkRectangle rect;
  CdkWindowImplWin32 *impl;

  impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  impl->snap_state = CDK_WIN32_AEROSNAP_STATE_HALFRIGHT;

  cdk_screen_get_monitor_workarea (screen, snap_monitor, &rect);

  stash_window (window, impl, screen, monitor);

  rect.width = rect.width / 2;
  rect.x += rect.width;

  rect.x = rect.x - impl->margins.left;
  rect.y = rect.y - impl->margins.top;
  rect.width = rect.width + impl->margins_x;
  rect.height = rect.height + impl->margins_y;

  cdk_window_move_resize (window, rect.x, rect.y, rect.width, rect.height);
}

void
_cdk_win32_window_handle_aerosnap (CdkWindow            *window,
                                   CdkWin32AeroSnapCombo combo)
{
  CdkWindowImplWin32 *impl;
  CdkDisplay *display;
  CdkScreen *screen;
  gint n_monitors, monitor;
  CdkWindowState window_state = cdk_window_get_state (window);
  gboolean minimized = window_state & CDK_WINDOW_STATE_ICONIFIED;
  gboolean maximized = window_state & CDK_WINDOW_STATE_MAXIMIZED;
  gboolean halfsnapped;

  impl = CDK_WINDOW_IMPL_WIN32 (window->impl);
  display = cdk_window_get_display (window);
  screen = cdk_display_get_default_screen (display);
  n_monitors = cdk_screen_get_n_monitors (screen);
  monitor = cdk_screen_get_monitor_at_window (screen, window);

  if (minimized && maximized)
    minimized = FALSE;

  halfsnapped = (impl->snap_state == CDK_WIN32_AEROSNAP_STATE_HALFRIGHT ||
                 impl->snap_state == CDK_WIN32_AEROSNAP_STATE_HALFLEFT ||
                 impl->snap_state == CDK_WIN32_AEROSNAP_STATE_FULLUP);

  switch (combo)
    {
    case CDK_WIN32_AEROSNAP_COMBO_NOTHING:
      /* Do nothing */
      break;
    case CDK_WIN32_AEROSNAP_COMBO_UP:
      if (!maximized)
        {
	  unsnap (window, screen, monitor);
          cdk_window_maximize (window);
        }
      break;
    case CDK_WIN32_AEROSNAP_COMBO_DOWN:
    case CDK_WIN32_AEROSNAP_COMBO_SHIFTDOWN:
      if (maximized)
        {
	  cdk_window_unmaximize (window);
	  unsnap (window, screen, monitor);
        }
      else if (halfsnapped)
	unsnap (window, screen, monitor);
      else if (!minimized)
	cdk_window_iconify (window);
      break;
    case CDK_WIN32_AEROSNAP_COMBO_LEFT:
      if (maximized)
        cdk_window_unmaximize (window);

      if (impl->snap_state == CDK_WIN32_AEROSNAP_STATE_UNDETERMINED ||
	  impl->snap_state == CDK_WIN32_AEROSNAP_STATE_FULLUP)
	{
	  unsnap (window, screen, monitor);
	  snap_left (window, screen, monitor, monitor);
	}
      else if (impl->snap_state == CDK_WIN32_AEROSNAP_STATE_HALFLEFT)
	{
	  unsnap (window, screen, monitor);
	  snap_right (window, screen, monitor, monitor - 1 >= 0 ? monitor - 1 : n_monitors - 1);
	}
      else if (impl->snap_state == CDK_WIN32_AEROSNAP_STATE_HALFRIGHT)
	{
	  unsnap (window, screen, monitor);
	}
      break;
    case CDK_WIN32_AEROSNAP_COMBO_RIGHT:
      if (maximized)
        cdk_window_unmaximize (window);

      if (impl->snap_state == CDK_WIN32_AEROSNAP_STATE_UNDETERMINED ||
	  impl->snap_state == CDK_WIN32_AEROSNAP_STATE_FULLUP)
	{
	  unsnap (window, screen, monitor);
	  snap_right (window, screen, monitor, monitor);
	}
      else if (impl->snap_state == CDK_WIN32_AEROSNAP_STATE_HALFLEFT)
	{
	  unsnap (window, screen, monitor);
	}
      else if (impl->snap_state == CDK_WIN32_AEROSNAP_STATE_HALFRIGHT)
	{
	  unsnap (window, screen, monitor);
	  snap_left (window, screen, monitor, monitor + 1 < n_monitors ? monitor + 1 : 0);
	}
      break;
    case CDK_WIN32_AEROSNAP_COMBO_SHIFTUP:
      if (!maximized &&
          impl->snap_state == CDK_WIN32_AEROSNAP_STATE_UNDETERMINED)
	{
	  snap_up (window, screen, monitor);
	}
      break;
    case CDK_WIN32_AEROSNAP_COMBO_SHIFTLEFT:
    case CDK_WIN32_AEROSNAP_COMBO_SHIFTRIGHT:
      /* No implementation needed at the moment */
      break;
    }
}

static void
apply_snap (CdkWindow             *window,
            CdkWin32AeroSnapState  snap)
{
  CdkScreen *screen;
  gint monitor;

  screen = cdk_display_get_default_screen (cdk_window_get_display (window));
  monitor = cdk_screen_get_monitor_at_window (screen, window);

  switch (snap)
    {
    case CDK_WIN32_AEROSNAP_STATE_UNDETERMINED:
      break;
    case CDK_WIN32_AEROSNAP_STATE_MAXIMIZE:
      unsnap (window, screen, monitor);
      cdk_window_maximize (window);
      break;
    case CDK_WIN32_AEROSNAP_STATE_HALFLEFT:
      unsnap (window, screen, monitor);
      snap_left (window, screen, monitor, monitor);
      break;
    case CDK_WIN32_AEROSNAP_STATE_HALFRIGHT:
      unsnap (window, screen, monitor);
      snap_right (window, screen, monitor, monitor);
      break;
    case CDK_WIN32_AEROSNAP_STATE_FULLUP:
      snap_up (window, screen, monitor);
      break;
    }
}

/* Registers a dumb window class. This window
 * has DefWindowProc() for a window procedure and
 * does not do anything that CdkWindow-bound HWNDs do.
 */
static ATOM
RegisterCdkDumbClass ()
{
  static ATOM klassDUMB = 0;
  static WNDCLASSEXW wcl;
  ATOM klass = 0;

  wcl.cbSize = sizeof (WNDCLASSEX);
  wcl.style = 0; /* DON'T set CS_<H,V>REDRAW. It causes total redraw
                  * on WM_SIZE and WM_MOVE. Flicker, Performance!
                  */
  wcl.lpfnWndProc = DefWindowProcW;
  wcl.cbClsExtra = 0;
  wcl.cbWndExtra = 0;
  wcl.hInstance = _cdk_app_hmodule;
  wcl.hIcon = 0;
  wcl.hIconSm = 0;
  wcl.lpszMenuName = NULL;
  wcl.hbrBackground = NULL;
  wcl.hCursor = LoadCursor (NULL, IDC_ARROW);
  wcl.style |= CS_OWNDC;
  wcl.lpszClassName = L"cdkWindowDumb";

  if (klassDUMB == 0)
    klassDUMB = RegisterClassExW (&wcl);

  klass = klassDUMB;

  if (klass == 0)
    {
      WIN32_API_FAILED ("RegisterClassExW");
      g_error ("That is a fatal error");
    }

  return klass;
}

static gboolean
ensure_snap_indicator_exists (CdkW32DragMoveResizeContext *context)
{
  if (context->shape_indicator == NULL)
    {
      HWND handle;
      ATOM klass;
      klass = RegisterCdkDumbClass ();

      handle = CreateWindowExW (WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_NOACTIVATE,
                                MAKEINTRESOURCEW (klass),
                                L"",
                                WS_POPUP,
                                0,
                                0,
                                0, 0,
                                NULL,
                                NULL,
                                _cdk_app_hmodule,
                                NULL);

      context->shape_indicator = handle;
    }

  return context->shape_indicator != NULL;
}

static gboolean
ensure_snap_indicator_surface (CdkW32DragMoveResizeContext *context,
                          gint                         width,
                          gint                         height,
                          guint                        scale)
{
  if (context->indicator_surface != NULL &&
      (context->indicator_surface_width < width ||
       context->indicator_surface_height < height))
    {
      cairo_surface_destroy (context->indicator_surface);
      context->indicator_surface = NULL;
    }

  if (context->indicator_surface == NULL)
    context->indicator_surface = cairo_win32_surface_create_with_dib (CAIRO_FORMAT_ARGB32,
                                                                      width * scale,
                                                                      height * scale);

  if (cairo_surface_status (context->indicator_surface) != CAIRO_STATUS_SUCCESS)
    {
      cairo_surface_destroy (context->indicator_surface);
      context->indicator_surface = NULL;

      return FALSE;
    }

  return TRUE;
}

/* Indicator is drawn with some inward offset, so that it does
 * not hug screen edges.
 */
static void
adjust_indicator_rectangle (CdkRectangle *rect,
                            gboolean      inward)
{
  gdouble inverter;
  const gint gap = AEROSNAP_INDICATOR_EDGE_GAP;
#if defined(MORE_AEROSNAP_DEBUGGING)
  CdkRectangle cache = *rect;
#endif

  if (inward)
    inverter = 1.0;
  else
    inverter = -1.0;

  rect->x += (gap * inverter);
  rect->y += (gap * inverter);
  rect->width -= (gap * 2 * inverter);
  rect->height -= (gap * 2 * inverter);

#if defined(MORE_AEROSNAP_DEBUGGING)
  CDK_NOTE (MISC, g_print ("Adjusted %d x %d @ %d : %d -> %d x %d @ %d : %d\n",
                           cache.width, cache.height, cache.x, cache.y,
                           rect->width, rect->height, rect->x, rect->y));
#endif
}

static void
rounded_rectangle (cairo_t  *cr,
                   gint      x,
                   gint      y,
                   gint      width,
                   gint      height,
                   gdouble   radius,
                   gdouble   line_width,
                   CdkRGBA  *fill,
                   CdkRGBA  *outline)
{
  gdouble degrees = M_PI / 180.0;

  if (fill == NULL && outline == NULL)
    return;

  cairo_save (cr);
  cairo_new_sub_path (cr);
  cairo_arc (cr, x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees);
  cairo_arc (cr, x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees);
  cairo_arc (cr, (x + radius), y + height - radius, radius, 90 * degrees, 180 * degrees);
  cairo_arc (cr, (x + radius), (y + radius), radius, 180 * degrees, 270 * degrees);
  cairo_close_path (cr);

  if (fill)
    {
      cairo_set_source_rgba (cr, fill->red, fill->green, fill->blue, fill->alpha);

      if (outline)
        cairo_fill_preserve (cr);
      else
        cairo_fill (cr);
    }

  if (outline)
    {
      cairo_set_source_rgba (cr, outline->red, outline->green, outline->blue, outline->alpha);
      cairo_set_line_width (cr, line_width);
      cairo_stroke (cr);
    }

  cairo_restore (cr);
}

/* Translates linear animation scale into some kind of curve */
static gdouble
curve (gdouble val)
{
  /* TODO: try different curves. For now it's just linear */
  return val;
}

static gboolean
draw_indicator (CdkW32DragMoveResizeContext *context,
                gint64                       timestamp)
{
  cairo_t *cr;
  CdkRGBA outline = {0, 0, 1.0, 1.0};
  CdkRGBA fill = {0, 0, 1.0, 0.8};
  CdkRectangle current_rect;
  gint64 current_time = g_get_monotonic_time ();
  gdouble animation_progress;
  gboolean last_draw;
  gdouble line_width;
  gdouble corner_radius;
  gint64 animation_duration;
  CdkWindowImplWin32 *impl = CDK_WINDOW_IMPL_WIN32 (context->window->impl);

  line_width = AEROSNAP_INDICATOR_LINE_WIDTH * impl->window_scale;
  corner_radius = AEROSNAP_INDICATOR_CORNER_RADIUS;
  animation_duration = AEROSNAP_INDICATOR_ANIMATION_DURATION;
  last_draw = FALSE;

  if (timestamp == 0 &&
      current_time - context->indicator_start_time > animation_duration)
    {
      timestamp = context->indicator_start_time + animation_duration;
      last_draw = TRUE;
    }

  if (timestamp != 0)
    current_time = timestamp;

  animation_progress = (gdouble) (current_time - context->indicator_start_time) / animation_duration;

  if (animation_progress > 1.0)
    animation_progress = 1.0;

  if (animation_progress < 0)
    animation_progress = 0;

  animation_progress = curve (animation_progress);

  current_rect = context->indicator_start;
  current_rect.x += (context->indicator_target.x - context->indicator_start.x) * animation_progress;
  current_rect.y += (context->indicator_target.y - context->indicator_start.y) * animation_progress;
  current_rect.width += (context->indicator_target.width - context->indicator_start.width) * animation_progress;
  current_rect.height += (context->indicator_target.height - context->indicator_start.height) * animation_progress;

  if (context->op == CDK_WIN32_DRAGOP_RESIZE && last_draw)
    {
      switch (context->edge)
        {
        case CDK_WINDOW_EDGE_NORTH_WEST:
          current_rect.x = context->indicator_target.x + (context->indicator_target.width - current_rect.width);
          current_rect.y = context->indicator_target.y + (context->indicator_target.height - current_rect.height);
          break;
        case CDK_WINDOW_EDGE_NORTH:
          current_rect.y = context->indicator_target.y + (context->indicator_target.height - current_rect.height);
          break;
        case CDK_WINDOW_EDGE_WEST:
          current_rect.x = context->indicator_target.x + (context->indicator_target.width - current_rect.width);
          break;
        case CDK_WINDOW_EDGE_SOUTH_WEST:
          current_rect.x = context->indicator_target.x + (context->indicator_target.width - current_rect.width);
          current_rect.y = context->indicator_target.y;
          break;
        case CDK_WINDOW_EDGE_NORTH_EAST:
          current_rect.x = context->indicator_target.x;
          current_rect.y = context->indicator_target.y + (context->indicator_target.height - current_rect.height);
          break;
        case CDK_WINDOW_EDGE_SOUTH_EAST:
          current_rect.x = context->indicator_target.x;
          current_rect.y = context->indicator_target.y;
          break;
        case CDK_WINDOW_EDGE_SOUTH:
          current_rect.y = context->indicator_target.y;
          break;
        case CDK_WINDOW_EDGE_EAST:
          current_rect.x = context->indicator_target.x;
          break;
        }
    }

  cr = cairo_create (context->indicator_surface);
  rounded_rectangle (cr,
                     (current_rect.x - context->indicator_window_rect.x) * impl->window_scale,
                     (current_rect.y - context->indicator_window_rect.y) * impl->window_scale,
                     current_rect.width * impl->window_scale,
                     current_rect.height * impl->window_scale,
                     corner_radius,
                     line_width,
                     &fill, &outline);
  cairo_destroy (cr);

#if defined(MORE_AEROSNAP_DEBUGGING)
  CDK_NOTE (MISC, g_print ("Indicator is %d x %d @ %d : %d; current time is %" G_GINT64_FORMAT "\n",
                           current_rect.width, current_rect.height,
                           current_rect.x - context->indicator_window_rect.x,
                           current_rect.y - context->indicator_window_rect.y,
                           current_time));
#endif

  return last_draw;
}

static gboolean
redraw_indicator (gpointer user_data)
{
  CdkW32DragMoveResizeContext *context = user_data;
  POINT window_position;
  SIZE window_size;
  BLENDFUNCTION blender;
  HDC hdc;
  POINT source_point = { 0, 0 };
  gboolean last_draw;
  gdouble indicator_opacity;
  CdkWindowImplWin32 *impl;
  gboolean do_source_remove = FALSE;

  indicator_opacity = AEROSNAP_INDICATOR_OPACITY;

  if (CDK_WINDOW_DESTROYED (context->window) ||
      !ensure_snap_indicator_exists (context))
    {
      do_source_remove = TRUE;
    }

  impl = CDK_WINDOW_IMPL_WIN32 (context->window->impl);

  if (!ensure_snap_indicator_surface (context,
                                      context->indicator_window_rect.width,
                                      context->indicator_window_rect.height,
                                      impl->window_scale))
    {
      do_source_remove = TRUE;
    }

  if (do_source_remove)
    {
      context->timer = 0;
      return G_SOURCE_REMOVE;
    }

  last_draw = draw_indicator (context, context->draw_timestamp);

  window_position.x = (context->indicator_window_rect.x - _cdk_offset_x) * impl->window_scale;
  window_position.y = (context->indicator_window_rect.y - _cdk_offset_y) * impl->window_scale;
  window_size.cx = context->indicator_window_rect.width * impl->window_scale;
  window_size.cy = context->indicator_window_rect.height * impl->window_scale;

  blender.BlendOp = AC_SRC_OVER;
  blender.BlendFlags = 0;
  blender.AlphaFormat = AC_SRC_ALPHA;
  blender.SourceConstantAlpha = 255 * indicator_opacity;

  hdc = cairo_win32_surface_get_dc (context->indicator_surface);

  API_CALL (SetWindowPos, (context->shape_indicator,
                           CDK_WINDOW_HWND (context->window),
                           0, 0, 0, 0,
                           SWP_NOMOVE | SWP_NOSIZE | SWP_NOREDRAW | SWP_SHOWWINDOW | SWP_NOACTIVATE));

#if defined(MORE_AEROSNAP_DEBUGGING)
  CDK_NOTE (MISC, g_print ("Indicator window position is %ld x %ld @ %ld : %ld\n",
                           window_size.cx, window_size.cy,
                           window_position.x, window_position.y));
#endif

  API_CALL (UpdateLayeredWindow, (context->shape_indicator, NULL,
                                  &window_position, &window_size,
                                  hdc, &source_point,
                                  0, &blender, ULW_ALPHA));

  if (last_draw)
    context->timer = 0;

  return last_draw ? G_SOURCE_REMOVE : G_SOURCE_CONTINUE;
}

static CdkRectangle
unity_of_rects (CdkRectangle a,
                CdkRectangle b)
{
  CdkRectangle u = b;

  if (a.x < u.x)
    {
      u.width += u.x - a.x;
      u.x = a.x;
    }

  if (a.y < u.y)
    {
      u.height += (u.y - a.y);
      u.y = a.y;
    }

  if (a.x + a.width > u.x + u.width)
    u.width += (a.x + a.width) - (u.x + u.width);

  if (a.y + a.height > u.y + u.height)
    u.height += (a.y + a.height) - (u.y + u.height);

#if defined(MORE_AEROSNAP_DEBUGGING)
  CDK_NOTE (MISC, g_print ("Unified 2 rects into %d x %d @ %d : %d\n",
                           u.width, u.height, u.x, u.y));
#endif

  return u;
}

static void
start_indicator_drawing (CdkW32DragMoveResizeContext *context,
                         CdkRectangle                 from,
                         CdkRectangle                 to,
                         guint                        scale)
{
  CdkRectangle to_adjusted, from_adjusted, from_or_to;
  gint64 indicator_animation_tick = AEROSNAP_INDICATOR_ANIMATION_TICK;

  CDK_NOTE (MISC, g_print ("Start drawing snap indicator %d x %d @ %d : %d -> %d x %d @ %d : %d\n",
                           from.width * scale, from.height * scale, from.x, from.y, to.width * scale, to.height * scale, to.x, to.y));

  if (CDK_WINDOW_DESTROYED (context->window))
    return;

  if (!ensure_snap_indicator_exists (context))
    return;

  from_or_to = unity_of_rects (from, to);

  if (!ensure_snap_indicator_surface (context, from_or_to.width, from_or_to.height, scale))
    return;

  to_adjusted = to;
  adjust_indicator_rectangle (&to_adjusted, TRUE);

  from_adjusted = from;
  adjust_indicator_rectangle (&from_adjusted, TRUE);

  context->draw_timestamp = 0;
  context->indicator_start = from_adjusted;
  context->indicator_target = to_adjusted;
  context->indicator_window_rect = from_or_to;
  context->indicator_start_time = g_get_monotonic_time ();

  if (context->timer)
    {
      g_source_remove (context->timer);
      context->timer = 0;
    }

  context->timer = g_timeout_add_full (G_PRIORITY_DEFAULT,
                                       indicator_animation_tick,
                                       redraw_indicator,
                                       context,
                                       NULL);
}

static void
update_fullup_indicator (CdkWindow                   *window,
                         CdkW32DragMoveResizeContext *context)
{
  SHORT maxysize;
  CdkRectangle from, to;
  CdkRectangle to_adjusted, from_adjusted, from_or_to;
  CdkWindowImplWin32 *impl;

  CDK_NOTE (MISC, g_print ("Update fullup indicator\n"));

  if (CDK_WINDOW_DESTROYED (context->window))
    return;

  if (context->shape_indicator == NULL)
    return;

  impl = CDK_WINDOW_IMPL_WIN32 (window->impl);
  maxysize = GetSystemMetrics (SM_CYVIRTUALSCREEN);
  cdk_window_get_position (window, &to.x, &to.y);
  to.width = cdk_window_get_width (window);
  to.height = cdk_window_get_height (window);

  to.y = 0;
  to.height = maxysize;
  from = context->indicator_target;

  if (context->timer == 0)
    {
      from_adjusted = from;
      adjust_indicator_rectangle (&from_adjusted, FALSE);

      CDK_NOTE (MISC, g_print ("Restart fullup animation from %d x %d @ %d : %d -> %d x %d @ %d x %d\n",
                               context->indicator_target.width, context->indicator_target.height,
                               context->indicator_target.x, context->indicator_target.y,
                               to.width, to.height, to.x, to.y));
      start_indicator_drawing (context, from_adjusted, to, impl->window_scale);

      return;
    }

  from_or_to = unity_of_rects (from, to);

  to_adjusted = to;
  adjust_indicator_rectangle (&to_adjusted, TRUE);

  CDK_NOTE (MISC, g_print ("Retarget fullup animation %d x %d @ %d : %d -> %d x %d @ %d x %d\n",
                           context->indicator_target.width, context->indicator_target.height,
                           context->indicator_target.x, context->indicator_target.y,
                           to_adjusted.width, to_adjusted.height, to_adjusted.x, to_adjusted.y));

  context->indicator_target = to_adjusted;
  context->indicator_window_rect = from_or_to;

  ensure_snap_indicator_surface (context, from_or_to.width, from_or_to.height, impl->window_scale);
}

static void
start_indicator (CdkWindow                   *window,
                 CdkW32DragMoveResizeContext *context,
                 gint                         x,
                 gint                         y,
                 CdkWin32AeroSnapState        state)
{
  CdkScreen *screen;
  gint monitor;
  CdkRectangle workarea;
  SHORT maxysize;
  CdkRectangle start_size, end_size;
  CdkWindowImplWin32 *impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  screen = cdk_window_get_screen (window);
  monitor = cdk_screen_get_monitor_at_point (screen, x, y);
  cdk_screen_get_monitor_workarea (screen, monitor, &workarea);

  maxysize = GetSystemMetrics (SM_CYVIRTUALSCREEN) / impl->window_scale;
  cdk_window_get_position (window, &start_size.x, &start_size.y);
  start_size.width = cdk_window_get_width (window);
  start_size.height = cdk_window_get_height (window);

  end_size = start_size;

  switch (state)
    {
    case CDK_WIN32_AEROSNAP_STATE_UNDETERMINED:
      return;
    case CDK_WIN32_AEROSNAP_STATE_MAXIMIZE:
      end_size.x = workarea.x;
      end_size.y = workarea.y;
      end_size.width = workarea.width;
      end_size.height = workarea.height;
      break;
    case CDK_WIN32_AEROSNAP_STATE_HALFLEFT:
      end_size.x = workarea.x;
      end_size.y = workarea.y;
      end_size.width = workarea.width / 2;
      end_size.height = workarea.height;
      break;
    case CDK_WIN32_AEROSNAP_STATE_HALFRIGHT:
      end_size.x = (workarea.x + workarea.width / 2);
      end_size.y = workarea.y;
      end_size.width = workarea.width / 2;
      end_size.height = workarea.height;
      break;
    case CDK_WIN32_AEROSNAP_STATE_FULLUP:
      end_size.y = 0;
      end_size.height = maxysize;
      break;
    }

  start_indicator_drawing (context, start_size, end_size, impl->window_scale);
}

static void
stop_indicator (CdkWindow                   *window,
                CdkW32DragMoveResizeContext *context)
{
  CDK_NOTE (MISC, g_print ("Stop drawing snap indicator\n"));

  if (context->timer)
    {
      g_source_remove (context->timer);
      context->timer = 0;
    }

  API_CALL (SetWindowPos, (context->shape_indicator,
                           SWP_NOZORDER_SPECIFIED,
                           0, 0, 0, 0,
                           SWP_NOZORDER | SWP_NOMOVE |
                           SWP_NOSIZE | SWP_NOREDRAW | SWP_HIDEWINDOW | SWP_NOACTIVATE));
}

static gint
point_in_aerosnap_region (gint                x,
                          gint                y,
                          AeroSnapEdgeRegion *region)
{
  gint edge, trigger;

  edge = (x >= region->edge.x &&
          y >= region->edge.y &&
          x <= region->edge.x + region->edge.width &&
          y <= region->edge.y + region->edge.height) ? 1 : 0;
  trigger = (x >= region->trigger.x &&
             y >= region->trigger.y &&
             x <= region->trigger.x + region->trigger.width &&
             y <= region->trigger.y + region->trigger.height) ? 1 : 0;
  return edge + trigger;
}

static void
handle_aerosnap_move_resize (CdkWindow                   *window,
                             CdkW32DragMoveResizeContext *context,
                             gint                         x,
                             gint                         y)
{
  gint i;
  AeroSnapEdgeRegion *reg;
  gint maximize = 0;
  gint halfleft = 0;
  gint halfright = 0;
  gint fullup = 0;
  gboolean fullup_edge = FALSE;
  CdkWindowImplWin32 *impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  if (context->op == CDK_WIN32_DRAGOP_RESIZE)
    switch (context->edge)
      {
      case CDK_WINDOW_EDGE_NORTH_WEST:
      case CDK_WINDOW_EDGE_NORTH_EAST:
      case CDK_WINDOW_EDGE_WEST:
      case CDK_WINDOW_EDGE_EAST:
      case CDK_WINDOW_EDGE_SOUTH_WEST:
      case CDK_WINDOW_EDGE_SOUTH_EAST:
        break;
      case CDK_WINDOW_EDGE_SOUTH:
      case CDK_WINDOW_EDGE_NORTH:
        fullup_edge = TRUE;
        break;
      }

  for (i = 0; i < context->maximize_regions->len && maximize == 0; i++)
    {
      reg = &g_array_index (context->maximize_regions, AeroSnapEdgeRegion, i);
      maximize = point_in_aerosnap_region (x, y, reg);
    }

  for (i = 0; i < context->halfleft_regions->len && halfleft == 0; i++)
    {
      reg = &g_array_index (context->halfleft_regions, AeroSnapEdgeRegion, i);
      halfleft = point_in_aerosnap_region (x, y, reg);
    }

  for (i = 0; i < context->halfright_regions->len && halfright == 0; i++)
    {
      reg = &g_array_index (context->halfright_regions, AeroSnapEdgeRegion, i);
      halfright = point_in_aerosnap_region (x, y, reg);
    }

  for (i = 0; i < context->fullup_regions->len && fullup == 0; i++)
    {
      reg = &g_array_index (context->fullup_regions, AeroSnapEdgeRegion, i);
      fullup = point_in_aerosnap_region (x, y, reg);
    }

#if defined(MORE_AEROSNAP_DEBUGGING)
  CDK_NOTE (MISC, g_print ("AeroSnap: point %d : %d - max: %d, left %d, right %d, up %d\n",
                           x, y, maximize, halfleft, halfright, fullup));
#endif

  if (!context->revealed)
    {
      if (context->op == CDK_WIN32_DRAGOP_MOVE && maximize == 2)
        {
          context->revealed = TRUE;
          context->current_snap = CDK_WIN32_AEROSNAP_STATE_MAXIMIZE;
          start_indicator (window, context, x, y, context->current_snap);
        }
      else if (context->op == CDK_WIN32_DRAGOP_MOVE && halfleft == 2)
        {
          context->revealed = TRUE;
          context->current_snap = CDK_WIN32_AEROSNAP_STATE_HALFLEFT;
          start_indicator (window, context, x, y, context->current_snap);
        }
      else if (context->op == CDK_WIN32_DRAGOP_MOVE && halfright == 2)
        {
          context->revealed = TRUE;
          context->current_snap = CDK_WIN32_AEROSNAP_STATE_HALFRIGHT;
          start_indicator (window, context, x, y, context->current_snap);
        }
      else if (context->op == CDK_WIN32_DRAGOP_RESIZE && fullup == 2 && fullup_edge)
        {
          context->revealed = TRUE;
          context->current_snap = CDK_WIN32_AEROSNAP_STATE_FULLUP;
          start_indicator (window, context, x, y, context->current_snap);
        }

      return;
    }

  switch (context->current_snap)
    {
    case CDK_WIN32_AEROSNAP_STATE_UNDETERMINED:
      if (context->op == CDK_WIN32_DRAGOP_RESIZE && fullup > 0)
        {
          context->current_snap = CDK_WIN32_AEROSNAP_STATE_FULLUP;
          start_indicator (window, context, x, y, context->current_snap);
        }
      break;
    case CDK_WIN32_AEROSNAP_STATE_MAXIMIZE:
      if (context->op == CDK_WIN32_DRAGOP_MOVE && maximize > 0)
        break;
      if (context->op == CDK_WIN32_DRAGOP_MOVE && halfleft > 0)
        {
          context->current_snap = CDK_WIN32_AEROSNAP_STATE_HALFLEFT;
          start_indicator (window, context, x, y, context->current_snap);
        }
      else if (context->op == CDK_WIN32_DRAGOP_MOVE && halfright > 0)
        {
          context->current_snap = CDK_WIN32_AEROSNAP_STATE_HALFRIGHT;
          start_indicator (window, context, x, y, context->current_snap);
        }
      else
        {
          context->current_snap = CDK_WIN32_AEROSNAP_STATE_UNDETERMINED;
          stop_indicator (window, context);
          context->revealed = FALSE;
        }
      break;
    case CDK_WIN32_AEROSNAP_STATE_HALFLEFT:
      if (context->op == CDK_WIN32_DRAGOP_MOVE && halfleft > 0)
        break;
      if (context->op == CDK_WIN32_DRAGOP_MOVE && maximize > 0)
        {
          context->current_snap = CDK_WIN32_AEROSNAP_STATE_MAXIMIZE;
          start_indicator (window, context, x, y, context->current_snap);
        }
      else if (context->op == CDK_WIN32_DRAGOP_MOVE && halfright > 0)
        {
          context->current_snap = CDK_WIN32_AEROSNAP_STATE_HALFRIGHT;
          start_indicator (window, context, x, y, context->current_snap);
        }
      else
        {
          context->current_snap = CDK_WIN32_AEROSNAP_STATE_UNDETERMINED;
          stop_indicator (window, context);
          context->revealed = FALSE;
        }
      break;
    case CDK_WIN32_AEROSNAP_STATE_HALFRIGHT:
      if (context->op == CDK_WIN32_DRAGOP_MOVE && halfright > 0)
        break;
      if (context->op == CDK_WIN32_DRAGOP_MOVE && maximize > 0)
        {
          context->current_snap = CDK_WIN32_AEROSNAP_STATE_MAXIMIZE;
          start_indicator (window, context, x, y, context->current_snap);
        }
      else if (context->op == CDK_WIN32_DRAGOP_MOVE && halfleft > 0)
        {
          context->current_snap = CDK_WIN32_AEROSNAP_STATE_HALFLEFT;
          start_indicator (window, context, x, y, context->current_snap);
        }
      else
        {
          context->current_snap = CDK_WIN32_AEROSNAP_STATE_UNDETERMINED;
          stop_indicator (window, context);
          context->revealed = FALSE;
        }
      break;
    case CDK_WIN32_AEROSNAP_STATE_FULLUP:
      if (context->op == CDK_WIN32_DRAGOP_RESIZE && fullup > 0 && fullup_edge)
        {
          update_fullup_indicator (window, context);
          break;
        }

      context->current_snap = CDK_WIN32_AEROSNAP_STATE_UNDETERMINED;
      stop_indicator (window, context);
      break;
    }
}


static const gchar *
get_cursor_name_from_op (CdkW32WindowDragOp op,
                         CdkWindowEdge      edge)
{
  switch (op)
    {
    case CDK_WIN32_DRAGOP_MOVE:
      return "move";
    case CDK_WIN32_DRAGOP_RESIZE:
      switch (edge)
        {
        case CDK_WINDOW_EDGE_NORTH_WEST:
          return "nw-resize";
        case CDK_WINDOW_EDGE_NORTH:
          return "n-resize";
        case CDK_WINDOW_EDGE_NORTH_EAST:
          return "ne-resize";
        case CDK_WINDOW_EDGE_WEST:
          return "w-resize";
        case CDK_WINDOW_EDGE_EAST:
          return "e-resize";
        case CDK_WINDOW_EDGE_SOUTH_WEST:
          return "sw-resize";
        case CDK_WINDOW_EDGE_SOUTH:
          return "s-resize";
        case CDK_WINDOW_EDGE_SOUTH_EAST:
          return "se-resize";
        }
      /* default: warn about unhandled enum values,
       * fallthrough to CDK_WIN32_DRAGOP_NONE case
       */
    case CDK_WIN32_DRAGOP_COUNT:
      g_assert_not_reached ();
    case CDK_WIN32_DRAGOP_NONE:
      return "default";
    /* default: warn about unhandled enum values */
    }

  g_assert_not_reached ();

  return NULL;
}

static gboolean
point_in_window (CdkWindow *window,
                 gdouble    x,
                 gdouble    y)
{
  return x >= 0 && x < window->width &&
         y >= 0 && y < window->height &&
         (window->shape == NULL ||
          cairo_region_contains_point (window->shape, x, y)) &&
         (window->input_shape == NULL ||
          cairo_region_contains_point (window->input_shape, x, y));
}

static CdkWindow *
child_window_at_coordinates (CdkWindow *window,
                             gint       root_x,
                             gint       root_y)
{
  gint x, y;
  GList *l;
  GList *children;

  children = cdk_window_peek_children (window);
  cdk_window_get_root_origin (window, &x, &y);
  x = root_x - x;
  y = root_y - y;

  for (l = children; l; l = g_list_next (l))
    {
      CdkWindow *child = CDK_WINDOW (l->data);

      if (point_in_window (child, x, y))
        return child;
    }

  return window;
}

static void
setup_drag_move_resize_context (CdkWindow                   *window,
                                CdkW32DragMoveResizeContext *context,
                                CdkW32WindowDragOp           op,
                                CdkWindowEdge                edge,
                                CdkDevice                   *device,
                                gint                         button,
                                gint                         root_x,
                                gint                         root_y,
                                guint32                      timestamp)
{
  RECT rect;
  const gchar *cursor_name;
  CdkWindow *pointer_window;
  CdkWindowImplWin32 *impl = CDK_WINDOW_IMPL_WIN32 (window->impl);
  CdkDisplay *display = cdk_device_get_display (device);
  gboolean maximized = cdk_window_get_state (window) & CDK_WINDOW_STATE_MAXIMIZED;

  /* Before we drag, we need to undo any maximization or snapping.
   * AeroSnap behaviour:
   *   If snapped halfleft/halfright:
   *     horizontal resize:
   *       resize
   *       don't unsnap
   *       keep stashed unsnapped size intact
   *     vertical resize:
   *       resize
   *       unsnap to new size (merge cached unsnapped state with current
   *                           snapped state in such a way that the gripped edge
   *                           does not move)
   *     diagonal resize:
   *       difficult to test (first move is usually either purely
   *                          horizontal or purely vertical, in which
   *                          case the above behaviour applies)
   *   If snapped up:
   *     horizontal resize:
   *       resize
   *       don't unsnap
   *       apply new width and x position to unsnapped cache,
   *         so that unsnapped window only regains its height
   *         and y position, but inherits x and width from
   *         the fullup snapped state
   *     vertical resize:
   *       unsnap to new size (merge cached unsnapped state with current
   *                           snapped state in such a way that the gripped edge
   *                           does not move)
   *
   * This implementation behaviour:
   *   If snapped halfleft/halfright/fullup:
   *     any resize:
   *       unsnap to current size, discard cached pre-snap state
   *
   * TODO: make this implementation behave as AeroSnap on resizes?
   * There's also the case where
   * a halfleft/halfright window isn't unsnapped when it's
   * being moved horizontally, but it's more difficult to implement.
   */
  if (op == CDK_WIN32_DRAGOP_RESIZE &&
      (impl->snap_state == CDK_WIN32_AEROSNAP_STATE_HALFRIGHT ||
       impl->snap_state == CDK_WIN32_AEROSNAP_STATE_HALFLEFT ||
       impl->snap_state == CDK_WIN32_AEROSNAP_STATE_FULLUP))
    {
      discard_snapinfo (window);
    }
  else if (maximized ||
           (impl->snap_state == CDK_WIN32_AEROSNAP_STATE_HALFRIGHT ||
            impl->snap_state == CDK_WIN32_AEROSNAP_STATE_HALFLEFT ||
            impl->snap_state == CDK_WIN32_AEROSNAP_STATE_FULLUP))
    {
      CdkScreen *screen;
      gint monitor;
      gint wx, wy, wwidth, wheight;
      gint swx, swy, swwidth, swheight;
      gboolean pointer_outside_of_window;
      gint offsetx, offsety;
      gboolean left_half;

      screen = cdk_display_get_default_screen (cdk_window_get_display (window));
      monitor = cdk_screen_get_monitor_at_window (screen, window);
      cdk_window_get_geometry (window, &wx, &wy, &wwidth, &wheight);

      swx = wx;
      swy = wy;
      swwidth = wwidth;
      swheight = wheight;

      /* Subtract window shadow. We don't want pointer to go outside of
       * the visible window during drag-move. For drag-resize it's OK.
       * Don't take shadow into account if the window is maximized -
       * maximized windows don't have shadows.
       */
      if (op == CDK_WIN32_DRAGOP_MOVE && !maximized)
        {
          swx += impl->margins.left / impl->window_scale;
          swy += impl->margins.top / impl->window_scale;
          swwidth -= impl->margins_x;
          swheight -= impl->margins_y;
        }

      pointer_outside_of_window = root_x < swx || root_x > swx + swwidth ||
                                  root_y < swy || root_y > swy + swheight;
      /* Calculate the offset of the pointer relative to the window */
      offsetx = root_x - swx;
      offsety = root_y - swy;

      /* Figure out in which half of the window the pointer is.
       * The code currently only concerns itself with horizontal
       * dimension (left/right halves).
       * There's no upper/lower half, because usually window
       * is dragged by its upper half anyway. If that changes, adjust
       * accordingly.
       */
      left_half = (offsetx < swwidth / 2);

      /* Inverse the offset for it to be from the right edge */
      if (!left_half)
        offsetx = swwidth - offsetx;

      CDK_NOTE (MISC, g_print ("Pointer at %d : %d, this is %d : %d relative to the window's %s\n",
                               root_x, root_y, offsetx, offsety,
                               left_half ? "left half" : "right half"));

      /* Move window in such a way that on unmaximization/unsnapping the pointer
       * is still pointing at the appropriate half of the window,
       * with the same offset from the left or right edge. If the new
       * window size is too small, and adding that offset puts the pointer
       * into the other half or even beyond, move the pointer to the middle.
       */
      if (!pointer_outside_of_window && maximized)
        {
          WINDOWPLACEMENT placement;
          gint unmax_width, unmax_height;
          gint shadow_unmax_width, shadow_unmax_height;

          placement.length = sizeof (placement);
          API_CALL (GetWindowPlacement, (CDK_WINDOW_HWND (window), &placement));

          CDK_NOTE (MISC, g_print ("W32 WM unmaximized window placement is %ld x %ld @ %ld : %ld\n",
                                   placement.rcNormalPosition.right - placement.rcNormalPosition.left,
                                   placement.rcNormalPosition.bottom - placement.rcNormalPosition.top,
                                   placement.rcNormalPosition.left + _cdk_offset_x * impl->window_scale,
                                   placement.rcNormalPosition.top + _cdk_offset_y * impl->window_scale));

          unmax_width = placement.rcNormalPosition.right - placement.rcNormalPosition.left;
          unmax_height = placement.rcNormalPosition.bottom - placement.rcNormalPosition.top;

          shadow_unmax_width = unmax_width - impl->margins_x * impl->window_scale;
          shadow_unmax_height = unmax_height - impl->margins_y * impl->window_scale;

          if (offsetx * impl->window_scale < (shadow_unmax_width / 2) &&
              offsety * impl->window_scale < (shadow_unmax_height / 2))
            {
              placement.rcNormalPosition.top = (root_y - offsety + impl->margins.top - _cdk_offset_y) * impl->window_scale;
              placement.rcNormalPosition.bottom = placement.rcNormalPosition.top + unmax_height;

              if (left_half)
                {
                  placement.rcNormalPosition.left = (root_x - offsetx + impl->margins.left - _cdk_offset_x) * impl->window_scale;
                  placement.rcNormalPosition.right = placement.rcNormalPosition.left + unmax_width;
                }
              else
                {
                  placement.rcNormalPosition.right = (root_x + offsetx + impl->margins.right - _cdk_offset_x) * impl->window_scale;
                  placement.rcNormalPosition.left = placement.rcNormalPosition.right - unmax_width;
                }
            }
          else
            {
              placement.rcNormalPosition.left = (root_x * impl->window_scale) -
                                                (unmax_width / 2) -
                                                (_cdk_offset_x * impl->window_scale);

              if (offsety * impl->window_scale < shadow_unmax_height / 2)
                placement.rcNormalPosition.top = (root_y - offsety + impl->margins.top - _cdk_offset_y) * impl->window_scale;
              else
                placement.rcNormalPosition.top = (root_y * impl->window_scale) -
                                                 (unmax_height / 2) -
                                                 (_cdk_offset_y * impl->window_scale);

              placement.rcNormalPosition.right = placement.rcNormalPosition.left + unmax_width;
              placement.rcNormalPosition.bottom = placement.rcNormalPosition.top + unmax_height;
            }

          CDK_NOTE (MISC, g_print ("Unmaximized window will be at %ld : %ld\n",
                                   placement.rcNormalPosition.left + _cdk_offset_x * impl->window_scale,
                                   placement.rcNormalPosition.top + _cdk_offset_y * impl->window_scale));

          API_CALL (SetWindowPlacement, (CDK_WINDOW_HWND (window), &placement));
        }
      else if (!pointer_outside_of_window && impl->snap_stash_int)
        {
          CdkRectangle new_pos;
          CdkRectangle snew_pos;

          new_pos.width = impl->snap_stash_int->width;
          new_pos.height = impl->snap_stash_int->height;
          snew_pos = new_pos;

          if (op == CDK_WIN32_DRAGOP_MOVE)
            {
              snew_pos.width -= impl->margins_x;
              snew_pos.height -= impl->margins_y;
            }

          if (offsetx < snew_pos.width / 2 && offsety < snew_pos.height / 2)
            {
              new_pos.y = root_y - offsety + impl->margins.top / impl->window_scale;

              if (left_half)
                new_pos.x = root_x - offsetx + impl->margins.left / impl->window_scale;
              else
                new_pos.x = root_x + offsetx + impl->margins.left / impl->window_scale - new_pos.width;
            }
          else
            {
              new_pos.x = root_x - new_pos.width / 2;
              new_pos.y = root_y - new_pos.height / 2;
            }

          CDK_NOTE (MISC, g_print ("Unsnapped window to %d : %d\n",
                                   new_pos.x, new_pos.y));
          discard_snapinfo (window);
          cdk_window_move_resize (window, new_pos.x, new_pos.y,
                                  new_pos.width, new_pos.height);
        }


      if (maximized)
        cdk_window_unmaximize (window);
      else
        unsnap (window, screen, monitor);

      if (pointer_outside_of_window)
        {
          /* Pointer outside of the window, move pointer into window */
          CDK_NOTE (MISC, g_print ("Pointer at %d : %d is outside of %d x %d @ %d : %d, move it to %d : %d\n",
                                   root_x, root_y, wwidth, wheight, wx, wy, wx + wwidth / 2, wy + wheight / 2));
          root_x = wx + wwidth / 2;
          /* This is Gnome behaviour. Windows WM would put the pointer
           * in the middle of the titlebar, but CDK doesn't know where
           * the titlebar is, if any.
           */
          root_y = wy + wheight / 2;
          cdk_device_warp (device, screen,
                           root_x, root_y);
        }
    }

  _cdk_win32_get_window_rect (window, &rect);

  cursor_name = get_cursor_name_from_op (op, edge);

  context->cursor = _cdk_win32_display_get_cursor_for_name (display, cursor_name);

  pointer_window = child_window_at_coordinates (window, root_x, root_y);

  /* Note: This triggers a WM_CAPTURECHANGED, which will trigger
   * cdk_win32_window_end_move_resize_drag(), which will end
   * our op before it even begins, but only if context->op is not NONE.
   * This is why we first do the grab, *then* set the op.
   */
  cdk_device_grab (device, pointer_window,
                   CDK_OWNERSHIP_NONE, FALSE,
                   CDK_ALL_EVENTS_MASK,
                   context->cursor,
                   timestamp);

  context->window = g_object_ref (window);
  context->op = op;
  context->edge = edge;
  context->device = device;
  context->button = button;
  context->start_root_x = root_x;
  context->start_root_y = root_y;
  context->timestamp = timestamp;
  context->start_rect = rect;

  context->shape_indicator = NULL;
  context->revealed = FALSE;
  context->halfleft_regions = g_array_new (FALSE, FALSE, sizeof (AeroSnapEdgeRegion));
  context->halfright_regions = g_array_new (FALSE, FALSE, sizeof (AeroSnapEdgeRegion));
  context->maximize_regions = g_array_new (FALSE, FALSE, sizeof (AeroSnapEdgeRegion));
  context->fullup_regions = g_array_new (FALSE, FALSE, sizeof (AeroSnapEdgeRegion));

  calculate_aerosnap_regions (context);

  CDK_NOTE (EVENTS,
            g_print ("begin drag moveresize: window %p, toplevel %p, "
                     "op %u, edge %d, device %p, "
                     "button %d, coord %d:%d, time %u\n",
                     pointer_window, cdk_window_get_toplevel (window),
                     context->op, context->edge, context->device,
                     context->button, context->start_root_x,
                     context->start_root_y, context->timestamp));
}

void
cdk_win32_window_end_move_resize_drag (CdkWindow *window)
{
  CdkWindowImplWin32 *impl = CDK_WINDOW_IMPL_WIN32 (window->impl);
  CdkW32DragMoveResizeContext *context = &impl->drag_move_resize_context;

  if (context->op == CDK_WIN32_DRAGOP_RESIZE)
    _cdk_win32_window_invalidate_egl_framebuffer (window);

  context->op = CDK_WIN32_DRAGOP_NONE;

  cdk_device_ungrab (context->device, CDK_CURRENT_TIME);

  g_clear_object (&context->cursor);

  context->revealed = FALSE;

  if (context->timer)
    {
      g_source_remove (context->timer);
      context->timer = 0;
    }

  g_clear_object (&context->window);

  if (context->indicator_surface)
    {
      cairo_surface_destroy (context->indicator_surface);
      context->indicator_surface = NULL;
    }

  if (context->shape_indicator)
    {
      stop_indicator (window, context);
      DestroyWindow (context->shape_indicator);
      context->shape_indicator = NULL;
    }

  g_clear_pointer (&context->halfleft_regions, g_array_unref);
  g_clear_pointer (&context->halfright_regions, g_array_unref);
  g_clear_pointer (&context->maximize_regions, g_array_unref);
  g_clear_pointer (&context->fullup_regions, g_array_unref);

  CDK_NOTE (EVENTS,
            g_print ("end drag moveresize: window %p, toplevel %p,"
                     "op %u, edge %d, device %p, "
                     "button %d, coord %d:%d, time %u\n",
                     window, cdk_window_get_toplevel (window),
                     context->op, context->edge, context->device,
                     context->button, context->start_root_x,
                     context->start_root_y, context->timestamp));

  if (context->current_snap != CDK_WIN32_AEROSNAP_STATE_UNDETERMINED)
    apply_snap (window, context->current_snap);

  context->current_snap = CDK_WIN32_AEROSNAP_STATE_UNDETERMINED;
}

static void
cdk_win32_get_window_size_and_position_from_client_rect (CdkWindow *window,
                                                         RECT      *window_rect,
                                                         SIZE      *window_size,
                                                         POINT     *window_position)
{
  CdkWindowImplWin32 *impl;

  impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  /* Turn client area into window area */
  _cdk_win32_adjust_client_rect (window, window_rect);

  /* Convert CDK screen coordinates to W32 desktop coordinates */
  window_rect->left -= _cdk_offset_x * impl->window_scale;
  window_rect->right -= _cdk_offset_x * impl->window_scale;
  window_rect->top -= _cdk_offset_y * impl->window_scale;
  window_rect->bottom -= _cdk_offset_y * impl->window_scale;

  window_position->x = window_rect->left;
  window_position->y = window_rect->top;
  window_size->cx = window_rect->right - window_rect->left;
  window_size->cy = window_rect->bottom - window_rect->top;
}

static void
cdk_win32_update_layered_window_from_cache (CdkWindow *window,
                                            RECT      *client_rect)
{
  POINT window_position;
  SIZE window_size;
  BLENDFUNCTION blender;
  HDC hdc;
  SIZE *window_size_ptr;
  POINT source_point = { 0, 0 };
  POINT *source_point_ptr;
  CdkWindowImplWin32 *impl;

  impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  cdk_win32_get_window_size_and_position_from_client_rect (window,
                                                           client_rect,
                                                           &window_size,
                                                           &window_position);

  blender.BlendOp = AC_SRC_OVER;
  blender.BlendFlags = 0;
  blender.AlphaFormat = AC_SRC_ALPHA;
  blender.SourceConstantAlpha = impl->layered_opacity * 255;

  /* Size didn't change, so move immediately, no need to wait for redraw */
  /* Strictly speaking, we don't need to supply hdc, source_point and
   * window_size here. However, without these arguments
   * the window moves but does not update its contents on Windows 7 when
   * desktop composition is off. This forces us to provide hdc and
   * source_point. window_size is here to avoid the function
   * inexplicably failing with error 317.
   */
  if (cdk_screen_is_composited (cdk_window_get_screen (window)))
    {
      hdc = NULL;
      window_size_ptr = NULL;
      source_point_ptr = NULL;
    }
  else
    {
      hdc = cairo_win32_surface_get_dc (impl->cache_surface);
      window_size_ptr = &window_size;
      source_point_ptr = &source_point;
    }

  API_CALL (UpdateLayeredWindow, (CDK_WINDOW_HWND (window), NULL,
                                  &window_position, window_size_ptr,
                                  hdc, source_point_ptr,
                                  0, &blender, ULW_ALPHA));
}

void
cdk_win32_window_do_move_resize_drag (CdkWindow *window,
                                      gint       x,
                                      gint       y)
{
  RECT rect;
  RECT new_rect;
  gint diffy, diffx;
  MINMAXINFO mmi;
  CdkWindowImplWin32 *impl;
  CdkW32DragMoveResizeContext *context;
  gint width;
  gint height;

  impl = CDK_WINDOW_IMPL_WIN32 (window->impl);
  context = &impl->drag_move_resize_context;

  if (!_cdk_win32_get_window_rect (window, &rect))
    return;

  new_rect = context->start_rect;
  diffx = (x - context->start_root_x) * impl->window_scale;
  diffy = (y - context->start_root_y) * impl->window_scale;

  switch (context->op)
    {
    case CDK_WIN32_DRAGOP_RESIZE:

      switch (context->edge)
        {
        case CDK_WINDOW_EDGE_NORTH_WEST:
          new_rect.left += diffx;
          new_rect.top += diffy;
          break;

        case CDK_WINDOW_EDGE_NORTH:
          new_rect.top += diffy;
          break;

        case CDK_WINDOW_EDGE_NORTH_EAST:
          new_rect.right += diffx;
          new_rect.top += diffy;
          break;

        case CDK_WINDOW_EDGE_WEST:
          new_rect.left += diffx;
          break;

        case CDK_WINDOW_EDGE_EAST:
          new_rect.right += diffx;
          break;

        case CDK_WINDOW_EDGE_SOUTH_WEST:
          new_rect.left += diffx;
          new_rect.bottom += diffy;
          break;

        case CDK_WINDOW_EDGE_SOUTH:
          new_rect.bottom += diffy;
          break;

        case CDK_WINDOW_EDGE_SOUTH_EAST:
        default:
          new_rect.right += diffx;
          new_rect.bottom += diffy;
          break;
        }

      /* When handling WM_GETMINMAXINFO, mmi is already populated
       * by W32 WM and we apply our stuff on top of that.
       * Here it isn't, so we should at least clear it.
       */
      memset (&mmi, 0, sizeof (mmi));

      if (!_cdk_win32_window_fill_min_max_info (window, &mmi))
        break;

      width = new_rect.right - new_rect.left;
      height = new_rect.bottom - new_rect.top;

      if (width > mmi.ptMaxTrackSize.x)
        {
          switch (context->edge)
            {
            case CDK_WINDOW_EDGE_NORTH_WEST:
            case CDK_WINDOW_EDGE_WEST:
            case CDK_WINDOW_EDGE_SOUTH_WEST:
              new_rect.left = new_rect.right - mmi.ptMaxTrackSize.x;
              break;

            case CDK_WINDOW_EDGE_NORTH_EAST:
            case CDK_WINDOW_EDGE_EAST:
            case CDK_WINDOW_EDGE_SOUTH_EAST:
            default:
              new_rect.right = new_rect.left + mmi.ptMaxTrackSize.x;
              break;
            }
        }
      else if (width < mmi.ptMinTrackSize.x)
        {
          switch (context->edge)
            {
            case CDK_WINDOW_EDGE_NORTH_WEST:
            case CDK_WINDOW_EDGE_WEST:
            case CDK_WINDOW_EDGE_SOUTH_WEST:
              new_rect.left = new_rect.right - mmi.ptMinTrackSize.x;
              break;

            case CDK_WINDOW_EDGE_NORTH_EAST:
            case CDK_WINDOW_EDGE_EAST:
            case CDK_WINDOW_EDGE_SOUTH_EAST:
            default:
              new_rect.right = new_rect.left + mmi.ptMinTrackSize.x;
              break;
            }
        }

      if (height > mmi.ptMaxTrackSize.y)
        {
          switch (context->edge)
            {
            case CDK_WINDOW_EDGE_NORTH_WEST:
            case CDK_WINDOW_EDGE_NORTH:
            case CDK_WINDOW_EDGE_NORTH_EAST:
              new_rect.top = new_rect.bottom - mmi.ptMaxTrackSize.y;

            case CDK_WINDOW_EDGE_SOUTH_WEST:
            case CDK_WINDOW_EDGE_SOUTH:
            case CDK_WINDOW_EDGE_SOUTH_EAST:
            default:
              new_rect.bottom = new_rect.top + mmi.ptMaxTrackSize.y;
              break;
            }
        }
      else if (height < mmi.ptMinTrackSize.y)
        {
          switch (context->edge)
            {
            case CDK_WINDOW_EDGE_NORTH_WEST:
            case CDK_WINDOW_EDGE_NORTH:
            case CDK_WINDOW_EDGE_NORTH_EAST:
              new_rect.top = new_rect.bottom - mmi.ptMinTrackSize.y;

            case CDK_WINDOW_EDGE_SOUTH_WEST:
            case CDK_WINDOW_EDGE_SOUTH:
            case CDK_WINDOW_EDGE_SOUTH_EAST:
            default:
              new_rect.bottom = new_rect.top + mmi.ptMinTrackSize.y;
              break;
            }
        }

      break;
    case CDK_WIN32_DRAGOP_MOVE:
      new_rect.left += diffx;
      new_rect.top += diffy;
      new_rect.right += diffx;
      new_rect.bottom += diffy;
      break;
    default:
      break;
    }

  if (context->op == CDK_WIN32_DRAGOP_RESIZE &&
      (rect.left != new_rect.left ||
       rect.right != new_rect.right ||
       rect.top != new_rect.top ||
       rect.bottom != new_rect.bottom))
    {
      context->native_move_resize_pending = TRUE;
      _cdk_win32_do_emit_configure_event (window, new_rect);
    }
  else if (context->op == CDK_WIN32_DRAGOP_MOVE &&
           (rect.left != new_rect.left ||
            rect.top != new_rect.top))
    {
      context->native_move_resize_pending = FALSE;

      _cdk_win32_do_emit_configure_event (window, new_rect);

      if (impl->layered)
        {
          cdk_win32_update_layered_window_from_cache (window, &new_rect);
        }
      else
        {
          SIZE window_size;
          POINT window_position;

          cdk_win32_get_window_size_and_position_from_client_rect (window,
                                                                   &new_rect,
                                                                   &window_size,
                                                                   &window_position);

          API_CALL (SetWindowPos, (CDK_WINDOW_HWND (window),
                                   SWP_NOZORDER_SPECIFIED,
                                   window_position.x, window_position.y,
                                   0, 0,
                                   SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE));
        }
    }

  if (context->op == CDK_WIN32_DRAGOP_RESIZE ||
      context->op == CDK_WIN32_DRAGOP_MOVE)
    handle_aerosnap_move_resize (window, context, x, y);
}

static void
cdk_win32_window_begin_resize_drag (CdkWindow     *window,
                                    CdkWindowEdge  edge,
                                    CdkDevice     *device,
                                    gint           button,
                                    gint           root_x,
                                    gint           root_y,
                                    guint32        timestamp)
{
  CdkWindowImplWin32 *impl;

  g_return_if_fail (CDK_IS_WINDOW (window));

  if (CDK_WINDOW_DESTROYED (window) ||
      CDK_WINDOW_TYPE (window) == CDK_WINDOW_CHILD ||
      IsIconic (CDK_WINDOW_HWND (window)))
    return;

  /* Tell Windows to start interactively resizing the window by pretending that
   * the left pointer button was clicked in the suitable edge or corner. This
   * will only work if the button is down when this function is called, and
   * will only work with button 1 (left), since Windows only allows window
   * dragging using the left mouse button.
   */

  if (button != 1)
    return;

  impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  if (impl->drag_move_resize_context.op != CDK_WIN32_DRAGOP_NONE)
    cdk_win32_window_end_move_resize_drag (window);

  setup_drag_move_resize_context (window, &impl->drag_move_resize_context,
                                  CDK_WIN32_DRAGOP_RESIZE, edge, device,
                                  button, root_x, root_y, timestamp);
}

static void
cdk_win32_window_begin_move_drag (CdkWindow *window,
                                  CdkDevice *device,
                                  gint       button,
                                  gint       root_x,
                                  gint       root_y,
                                  guint32    timestamp)
{
  CdkWindowImplWin32 *impl;

  g_return_if_fail (CDK_IS_WINDOW (window));

  if (CDK_WINDOW_DESTROYED (window) ||
      CDK_WINDOW_TYPE (window) == CDK_WINDOW_CHILD ||
      IsIconic (CDK_WINDOW_HWND (window)))
    return;

  /* Tell Windows to start interactively moving the window by pretending that
   * the left pointer button was clicked in the titlebar. This will only work
   * if the button is down when this function is called, and will only work
   * with button 1 (left), since Windows only allows window dragging using the
   * left mouse button.
   */
  if (button != 1)
    return;

  impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  if (impl->drag_move_resize_context.op != CDK_WIN32_DRAGOP_NONE)
    cdk_win32_window_end_move_resize_drag (window);

  setup_drag_move_resize_context (window, &impl->drag_move_resize_context,
                                  CDK_WIN32_DRAGOP_MOVE, CDK_WINDOW_EDGE_NORTH_WEST,
                                  device, button, root_x, root_y, timestamp);
}


/*
 * Setting window states
 */
static void
cdk_win32_window_iconify (CdkWindow *window)
{
  HWND old_active_window;

  g_return_if_fail (CDK_IS_WINDOW (window));

  if (CDK_WINDOW_DESTROYED (window))
    return;

  CDK_NOTE (MISC, g_print ("cdk_window_iconify: %p: %s\n",
			   CDK_WINDOW_HWND (window),
			   _cdk_win32_window_state_to_string (window->state)));

  if (CDK_WINDOW_IS_MAPPED (window))
    {
      old_active_window = GetActiveWindow ();
      CtkShowWindow (window, SW_MINIMIZE);
      if (old_active_window != CDK_WINDOW_HWND (window))
	SetActiveWindow (old_active_window);
    }
  else
    {
      cdk_synthesize_window_state (window,
                                   0,
                                   CDK_WINDOW_STATE_ICONIFIED);
    }
}

static void
cdk_win32_window_deiconify (CdkWindow *window)
{
  g_return_if_fail (CDK_IS_WINDOW (window));

  if (CDK_WINDOW_DESTROYED (window))
    return;

  CDK_NOTE (MISC, g_print ("cdk_window_deiconify: %p: %s\n",
			   CDK_WINDOW_HWND (window),
			   _cdk_win32_window_state_to_string (window->state)));

  if (CDK_WINDOW_IS_MAPPED (window))
    {
      show_window_internal (window, CDK_WINDOW_IS_MAPPED (window), TRUE);
    }
  else
    {
      cdk_synthesize_window_state (window,
                                   CDK_WINDOW_STATE_ICONIFIED,
                                   0);
    }
}

static void
cdk_win32_window_stick (CdkWindow *window)
{
  g_return_if_fail (CDK_IS_WINDOW (window));

  if (CDK_WINDOW_DESTROYED (window))
    return;

  /* FIXME: Do something? */
}

static void
cdk_win32_window_unstick (CdkWindow *window)
{
  g_return_if_fail (CDK_IS_WINDOW (window));

  if (CDK_WINDOW_DESTROYED (window))
    return;

  /* FIXME: Do something? */
}

static void
cdk_win32_window_maximize (CdkWindow *window)
{

  g_return_if_fail (CDK_IS_WINDOW (window));

  if (CDK_WINDOW_DESTROYED (window))
    return;

  CDK_NOTE (MISC, g_print ("cdk_window_maximize: %p: %s\n",
			   CDK_WINDOW_HWND (window),
			   _cdk_win32_window_state_to_string (window->state)));

  if (CDK_WINDOW_IS_MAPPED (window))
    CtkShowWindow (window, SW_MAXIMIZE);
  else
    cdk_synthesize_window_state (window,
				 0,
				 CDK_WINDOW_STATE_MAXIMIZED);
}

static void
cdk_win32_window_unmaximize (CdkWindow *window)
{
  g_return_if_fail (CDK_IS_WINDOW (window));

  if (CDK_WINDOW_DESTROYED (window))
    return;

  CDK_NOTE (MISC, g_print ("cdk_window_unmaximize: %p: %s\n",
			   CDK_WINDOW_HWND (window),
			   _cdk_win32_window_state_to_string (window->state)));

  _cdk_win32_window_invalidate_egl_framebuffer (window);

  if (CDK_WINDOW_IS_MAPPED (window))
    CtkShowWindow (window, SW_RESTORE);
  else
    cdk_synthesize_window_state (window,
				 CDK_WINDOW_STATE_MAXIMIZED,
				 0);
}

static void
cdk_win32_window_fullscreen (CdkWindow *window)
{
  gint x, y, width, height;
  FullscreenInfo *fi;
  HMONITOR monitor;
  MONITORINFO mi;
  DWORD extra_styles = WS_POPUP;
  gint workaround_padding = 0;

  g_return_if_fail (CDK_IS_WINDOW (window));

  fi = g_new (FullscreenInfo, 1);

  if (!GetWindowRect (CDK_WINDOW_HWND (window), &(fi->r)))
    g_free (fi);
  else
    {
      CdkWindowImplWin32 *impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

      monitor = MonitorFromWindow (CDK_WINDOW_HWND (window), MONITOR_DEFAULTTONEAREST);
      mi.cbSize = sizeof (mi);
      if (monitor && GetMonitorInfo (monitor, &mi))
	{
	  x = mi.rcMonitor.left;
	  y = mi.rcMonitor.top;
	  width = mi.rcMonitor.right - x;
	  height = mi.rcMonitor.bottom - y;
	}
      else
	{
	  x = y = 0;
	  width = GetSystemMetrics (SM_CXSCREEN);
	  height = GetSystemMetrics (SM_CYSCREEN);
	}

      /* remember for restoring */
      fi->hint_flags = impl->hint_flags;
      impl->hint_flags &= ~CDK_HINT_MAX_SIZE;
      g_object_set_data (G_OBJECT (window), "fullscreen-info", fi);
      fi->style = GetWindowLong (CDK_WINDOW_HWND (window), GWL_STYLE);

      /* Send state change before configure event */
      cdk_synthesize_window_state (window, 0, CDK_WINDOW_STATE_FULLSCREEN);

      /* If we are using GL windows, and we set the envvar CDK_WIN32_GL_FULLSCREEN_WORKAROUND,
       * set the WS_BORDER style so that DWM will not get deactivated.  This is necessary
       * when menus could not be shown correctly in fullscreen GL windows.  To avoid seeing
       * a border, we intentionally make the window bigger by 1px on all sides and place the
       * window just 1px outside the top left-hand coordinates outside the screen area.
       */
      if (window->gl_paint_context != NULL && g_getenv ("CDK_WIN32_GL_FULLSCREEN_WORKAROUND"))
        {
          extra_styles |= WS_BORDER;
          workaround_padding = 1;
          CDK_NOTE (MISC, g_print ("GL fullscreen workaround enabled for window [%p]\n",
                                   CDK_WINDOW_HWND (window)));
        }

      SetWindowLong (CDK_WINDOW_HWND (window), GWL_STYLE,
                     (fi->style & ~WS_OVERLAPPEDWINDOW) | extra_styles);

      API_CALL (SetWindowPos, (CDK_WINDOW_HWND (window), HWND_TOP,
                               x - workaround_padding,
                               y - workaround_padding,
                               width + (workaround_padding * 2),
                               height + (workaround_padding * 2),
                               SWP_NOCOPYBITS | SWP_SHOWWINDOW));
    }
}

static void
cdk_win32_window_unfullscreen (CdkWindow *window)
{
  FullscreenInfo *fi;

  g_return_if_fail (CDK_IS_WINDOW (window));

  fi = g_object_get_data (G_OBJECT (window), "fullscreen-info");
  if (fi)
    {
      CdkWindowImplWin32 *impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

      cdk_synthesize_window_state (window, CDK_WINDOW_STATE_FULLSCREEN, 0);

      impl->hint_flags = fi->hint_flags;
      SetWindowLong (CDK_WINDOW_HWND (window), GWL_STYLE, fi->style);
      _cdk_win32_window_invalidate_egl_framebuffer (window);
      API_CALL (SetWindowPos, (CDK_WINDOW_HWND (window), HWND_NOTOPMOST,
			       fi->r.left, fi->r.top,
			       fi->r.right - fi->r.left, fi->r.bottom - fi->r.top,
			       SWP_NOCOPYBITS | SWP_SHOWWINDOW));

      g_object_set_data (G_OBJECT (window), "fullscreen-info", NULL);
      g_free (fi);
      _cdk_win32_window_update_style_bits (window);
    }
}

static void
cdk_win32_window_set_keep_above (CdkWindow *window,
			   gboolean   setting)
{
  g_return_if_fail (CDK_IS_WINDOW (window));

  if (CDK_WINDOW_DESTROYED (window))
    return;

  CDK_NOTE (MISC, g_print ("cdk_window_set_keep_above: %p: %s\n",
			   CDK_WINDOW_HWND (window),
			   setting ? "YES" : "NO"));

  if (CDK_WINDOW_IS_MAPPED (window))
    {
      API_CALL (SetWindowPos, (CDK_WINDOW_HWND (window),
			       setting ? HWND_TOPMOST : HWND_NOTOPMOST,
			       0, 0, 0, 0,
			       SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE));
    }

  cdk_synthesize_window_state (window,
			       setting ? CDK_WINDOW_STATE_BELOW : CDK_WINDOW_STATE_ABOVE,
			       setting ? CDK_WINDOW_STATE_ABOVE : 0);
}

static void
cdk_win32_window_set_keep_below (CdkWindow *window,
			   gboolean   setting)
{
  g_return_if_fail (CDK_IS_WINDOW (window));

  if (CDK_WINDOW_DESTROYED (window))
    return;

  CDK_NOTE (MISC, g_print ("cdk_window_set_keep_below: %p: %s\n",
			   CDK_WINDOW_HWND (window),
			   setting ? "YES" : "NO"));

  if (CDK_WINDOW_IS_MAPPED (window))
    {
      API_CALL (SetWindowPos, (CDK_WINDOW_HWND (window),
			       setting ? HWND_BOTTOM : HWND_NOTOPMOST,
			       0, 0, 0, 0,
			       SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE));
    }

  cdk_synthesize_window_state (window,
			       setting ? CDK_WINDOW_STATE_ABOVE : CDK_WINDOW_STATE_BELOW,
			       setting ? CDK_WINDOW_STATE_BELOW : 0);
}

static void
cdk_win32_window_focus (CdkWindow *window,
			guint32    timestamp)
{
  g_return_if_fail (CDK_IS_WINDOW (window));

  if (CDK_WINDOW_DESTROYED (window))
    return;

  CDK_NOTE (MISC, g_print ("cdk_window_focus: %p: %s\n",
			   CDK_WINDOW_HWND (window),
			   _cdk_win32_window_state_to_string (window->state)));

  if (window->state & CDK_WINDOW_STATE_MAXIMIZED)
    CtkShowWindow (window, SW_SHOWMAXIMIZED);
  else if (window->state & CDK_WINDOW_STATE_ICONIFIED)
    CtkShowWindow (window, SW_RESTORE);
  else if (!IsWindowVisible (CDK_WINDOW_HWND (window)))
    CtkShowWindow (window, SW_SHOWNORMAL);
  else
    CtkShowWindow (window, SW_SHOW);

  SetFocus (CDK_WINDOW_HWND (window));
}

static void
cdk_win32_window_set_modal_hint (CdkWindow *window,
			   gboolean   modal)
{
  g_return_if_fail (CDK_IS_WINDOW (window));

  if (CDK_WINDOW_DESTROYED (window))
    return;

  CDK_NOTE (MISC, g_print ("cdk_window_set_modal_hint: %p: %s\n",
			   CDK_WINDOW_HWND (window),
			   modal ? "YES" : "NO"));

  if (modal == window->modal_hint)
    return;

  window->modal_hint = modal;

#if 0
  /* Not sure about this one.. -- Cody */
  if (CDK_WINDOW_IS_MAPPED (window))
    API_CALL (SetWindowPos, (CDK_WINDOW_HWND (window),
			     modal ? HWND_TOPMOST : HWND_NOTOPMOST,
			     0, 0, 0, 0,
			     SWP_NOMOVE | SWP_NOSIZE));
#else

  if (modal)
    {
      _cdk_push_modal_window (window);
      cdk_window_raise (window);
    }
  else
    {
      _cdk_remove_modal_window (window);
    }

#endif
}

static void
cdk_win32_window_set_skip_taskbar_hint (CdkWindow *window,
				  gboolean   skips_taskbar)
{
  static CdkWindow *owner = NULL;
  //CdkWindowAttr wa;

  g_return_if_fail (CDK_IS_WINDOW (window));

  CDK_NOTE (MISC, g_print ("cdk_window_set_skip_taskbar_hint: %p: %s, doing nothing\n",
			   CDK_WINDOW_HWND (window),
			   skips_taskbar ? "YES" : "NO"));

  // ### TODO: Need to figure out what to do here.
  return;

  if (skips_taskbar)
    {
#if 0
      if (owner == NULL)
		{
		  wa.window_type = CDK_WINDOW_TEMP;
		  wa.wclass = CDK_INPUT_OUTPUT;
		  wa.width = wa.height = 1;
		  wa.event_mask = 0;
		  owner = cdk_window_new_internal (NULL, &wa, 0, TRUE);
		}
#endif

      SetWindowLongPtr (CDK_WINDOW_HWND (window), GWLP_HWNDPARENT, (LONG_PTR) CDK_WINDOW_HWND (owner));

#if 0 /* Should we also turn off the minimize and maximize buttons? */
      SetWindowLong (CDK_WINDOW_HWND (window), GWL_STYLE,
		     GetWindowLong (CDK_WINDOW_HWND (window), GWL_STYLE) & ~(WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_SYSMENU));

      SetWindowPos (CDK_WINDOW_HWND (window), SWP_NOZORDER_SPECIFIED,
		    0, 0, 0, 0,
		    SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOMOVE |
		    SWP_NOREPOSITION | SWP_NOSIZE | SWP_NOZORDER);
#endif
    }
  else
    {
      SetWindowLongPtr (CDK_WINDOW_HWND (window), GWLP_HWNDPARENT, 0);
    }
}

static void
cdk_win32_window_set_skip_pager_hint (CdkWindow *window,
				gboolean   skips_pager)
{
  g_return_if_fail (CDK_IS_WINDOW (window));

  CDK_NOTE (MISC, g_print ("cdk_window_set_skip_pager_hint: %p: %s, doing nothing\n",
			   CDK_WINDOW_HWND (window),
			   skips_pager ? "YES" : "NO"));
}

static void
cdk_win32_window_set_type_hint (CdkWindow        *window,
			  CdkWindowTypeHint hint)
{
  g_return_if_fail (CDK_IS_WINDOW (window));

  if (CDK_WINDOW_DESTROYED (window))
    return;

  CDK_NOTE (MISC,
	    G_STMT_START{
	      static GEnumClass *class = NULL;
	      if (!class)
		class = g_type_class_ref (CDK_TYPE_WINDOW_TYPE_HINT);
	      g_print ("cdk_window_set_type_hint: %p: %s\n",
		       CDK_WINDOW_HWND (window),
		       g_enum_get_value (class, hint)->value_name);
	    }G_STMT_END);

  ((CdkWindowImplWin32 *)window->impl)->type_hint = hint;

  _cdk_win32_window_update_style_bits (window);
}

static CdkWindowTypeHint
cdk_win32_window_get_type_hint (CdkWindow *window)
{
  g_return_val_if_fail (CDK_IS_WINDOW (window), CDK_WINDOW_TYPE_HINT_NORMAL);

  if (CDK_WINDOW_DESTROYED (window))
    return CDK_WINDOW_TYPE_HINT_NORMAL;

  return CDK_WINDOW_IMPL_WIN32 (window->impl)->type_hint;
}

static HRGN
cairo_region_to_hrgn (const cairo_region_t *region,
		      gint                  x_origin,
		      gint                  y_origin,
		      guint                 scale)
{
  HRGN hrgn;
  RGNDATA *rgndata;
  RECT *rect;
  cairo_rectangle_int_t r;
  const int nrects = cairo_region_num_rectangles (region);
  guint nbytes =
    sizeof (RGNDATAHEADER) + (sizeof (RECT) * nrects);
  int i;

  rgndata = g_malloc (nbytes);
  rgndata->rdh.dwSize = sizeof (RGNDATAHEADER);
  rgndata->rdh.iType = RDH_RECTANGLES;
  rgndata->rdh.nCount = rgndata->rdh.nRgnSize = 0;
  SetRect (&rgndata->rdh.rcBound,
	   G_MAXLONG, G_MAXLONG, G_MINLONG, G_MINLONG);

  for (i = 0; i < nrects; i++)
    {
      rect = ((RECT *) rgndata->Buffer) + rgndata->rdh.nCount++;

      cairo_region_get_rectangle (region, i, &r);
      rect->left = (r.x + x_origin) * scale;
      rect->right = (rect->left + r.width) * scale;
      rect->top = (r.y + y_origin) * scale;
      rect->bottom = (rect->top + r.height) * scale;

      if (rect->left < rgndata->rdh.rcBound.left)
	rgndata->rdh.rcBound.left = rect->left;
      if (rect->right > rgndata->rdh.rcBound.right)
	rgndata->rdh.rcBound.right = rect->right;
      if (rect->top < rgndata->rdh.rcBound.top)
	rgndata->rdh.rcBound.top = rect->top;
      if (rect->bottom > rgndata->rdh.rcBound.bottom)
	rgndata->rdh.rcBound.bottom = rect->bottom;
    }
  if ((hrgn = ExtCreateRegion (NULL, nbytes, rgndata)) == NULL)
    WIN32_API_FAILED ("ExtCreateRegion");

  g_free (rgndata);

  return (hrgn);
}

static void
cdk_win32_window_shape_combine_region (CdkWindow       *window,
				       const cairo_region_t *shape_region,
				       gint             offset_x,
				       gint             offset_y)
{
  CdkWindowImplWin32 *impl;

  if (CDK_WINDOW_DESTROYED (window))
    return;

  if (!shape_region)
    {
      CDK_NOTE (MISC, g_print ("cdk_win32_window_shape_combine_region: %p: none\n",
			       CDK_WINDOW_HWND (window)));
      SetWindowRgn (CDK_WINDOW_HWND (window), NULL, TRUE);
    }
  else
    {
      HRGN hrgn;
      impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

      hrgn = cairo_region_to_hrgn (shape_region, 0, 0, impl->window_scale);

      CDK_NOTE (MISC, g_print ("cdk_win32_window_shape_combine_region: %p: %p\n",
			       CDK_WINDOW_HWND (window),
			       hrgn));

      do_shape_combine_region (window, hrgn, offset_x, offset_y);
    }
}

CdkWindow *
cdk_win32_window_lookup_for_display (CdkDisplay *display,
                                     HWND        anid)
{
  g_return_val_if_fail (display == cdk_display_get_default (), NULL);

  return (CdkWindow*) cdk_win32_handle_table_lookup (anid);
}

static void
cdk_win32_window_set_opacity (CdkWindow *window,
			gdouble    opacity)
{
  LONG exstyle;
  typedef BOOL (WINAPI *PFN_SetLayeredWindowAttributes) (HWND, COLORREF, BYTE, DWORD);
  PFN_SetLayeredWindowAttributes setLayeredWindowAttributes = NULL;
  CdkWindowImplWin32 *impl;

  g_return_if_fail (CDK_IS_WINDOW (window));

  if (!WINDOW_IS_TOPLEVEL (window) || CDK_WINDOW_DESTROYED (window))
    return;

  if (opacity < 0)
    opacity = 0;
  else if (opacity > 1)
    opacity = 1;

  impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  if (impl->layered)
    {
      if (impl->layered_opacity != opacity)
        {
          RECT window_rect;

          impl->layered_opacity = opacity;

          cdk_win32_get_window_client_area_rect (window, impl->window_scale, &window_rect);
          cdk_win32_update_layered_window_from_cache (window, &window_rect);
        }

      return;
    }

  exstyle = GetWindowLong (CDK_WINDOW_HWND (window), GWL_EXSTYLE);

  if (!(exstyle & WS_EX_LAYERED))
    SetWindowLong (CDK_WINDOW_HWND (window),
		    GWL_EXSTYLE,
		    exstyle | WS_EX_LAYERED);

  setLayeredWindowAttributes =
    (PFN_SetLayeredWindowAttributes)GetProcAddress (GetModuleHandle ("user32.dll"), "SetLayeredWindowAttributes");

  if (setLayeredWindowAttributes)
    {
      API_CALL (setLayeredWindowAttributes, (CDK_WINDOW_HWND (window),
					     0,
					     opacity * 0xff,
					     LWA_ALPHA));
    }
}

static cairo_region_t *
cdk_win32_window_get_shape (CdkWindow *window)
{
  HRGN hrgn = CreateRectRgn (0, 0, 0, 0);
  int  type = GetWindowRgn (CDK_WINDOW_HWND (window), hrgn);
  CdkWindowImplWin32 *impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  if (type == SIMPLEREGION || type == COMPLEXREGION)
    {
      cairo_region_t *region = _cdk_win32_hrgn_to_region (hrgn, impl->window_scale);

      DeleteObject (hrgn);
      return region;
    }

  return NULL;
}

static void
cdk_win32_input_shape_combine_region (CdkWindow *window,
				      const cairo_region_t *shape_region,
				      gint offset_x,
				      gint offset_y)
{
  /* Partial input shape support is implemented by handling the
   * WM_NCHITTEST message.
   */
}

gboolean
cdk_win32_window_is_win32 (CdkWindow *window)
{
  return CDK_WINDOW_IS_WIN32 (window);
}

static gboolean
cdk_win32_window_show_window_menu (CdkWindow *window,
                                   CdkEvent  *event)
{
  double event_x, event_y;
  gint x, y;
  CdkWindowImplWin32 *impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  switch (event->type)
    {
    case CDK_BUTTON_PRESS:
    case CDK_BUTTON_RELEASE:
    case CDK_TOUCH_BEGIN:
    case CDK_TOUCH_END:
      break;
    default:
      return FALSE;
    }

  cdk_event_get_root_coords (event, &event_x, &event_y);
  x = event_x - _cdk_offset_x;
  y = event_y - _cdk_offset_y;

  SendMessage (CDK_WINDOW_HWND (window),
               WM_SYSMENU,
               0,
               MAKELPARAM (x * impl->window_scale, y * impl->window_scale));

  return TRUE;
}

/**
 * _cdk_win32_acquire_dc
 * @impl: a Win32 #CdkWindowImplWin32 implementation
 *
 * Gets a DC with the given drawable selected into it.
 *
 * Returns: The DC, on success. Otherwise
 *  %NULL. If this function succeeded
 *  _cdk_win32_impl_release_dc()  must be called
 *  release the DC when you are done using it.
 **/
static HDC
_cdk_win32_impl_acquire_dc (CdkWindowImplWin32 *impl)
{
  if (CDK_IS_WINDOW_IMPL_WIN32 (impl) &&
      CDK_WINDOW_DESTROYED (impl->wrapper))
    return NULL;

  /* We don't call this function for layered windows, but
   * in case we do...
   */
  if (impl->layered)
    return NULL;

  if (!impl->hdc)
    {
      impl->hdc = GetDC (impl->handle);
      if (!impl->hdc)
	WIN32_GDI_FAILED ("GetDC");
    }

  if (impl->hdc)
    {
      impl->hdc_count++;
      return impl->hdc;
    }
  else
    {
      return NULL;
    }
}

/**
 * _cdk_win32_impl_release_dc
 * @impl: a Win32 #CdkWindowImplWin32 implementation
 *
 * Releases the reference count for the DC
 * from _cdk_win32_impl_acquire_dc()
 **/
static void
_cdk_win32_impl_release_dc (CdkWindowImplWin32 *impl)
{
  if (impl->layered)
    return;

  g_return_if_fail (impl->hdc_count > 0);

  impl->hdc_count--;
  if (impl->hdc_count == 0)
    {
      if (impl->saved_dc_bitmap)
	{
	  GDI_CALL (SelectObject, (impl->hdc, impl->saved_dc_bitmap));
	  impl->saved_dc_bitmap = NULL;
	}

      if (impl->hdc)
	{
	  GDI_CALL (ReleaseDC, (impl->handle, impl->hdc));
	  impl->hdc = NULL;
	}
    }
}

HWND
cdk_win32_window_get_impl_hwnd (CdkWindow *window)
{
  if (CDK_WINDOW_IS_WIN32 (window))
    return CDK_WINDOW_HWND (window);
  return NULL;
}

static void
cdk_win32_cairo_surface_destroy (void *data)
{
  CdkWindowImplWin32 *impl = data;

  _cdk_win32_impl_release_dc (impl);
  impl->cairo_surface = NULL;
}

static cairo_surface_t *
cdk_win32_ref_cairo_surface_layered (CdkWindow          *window,
                                     CdkWindowImplWin32 *impl)
{
  gint width, height;
  RECT window_rect;

  cdk_win32_get_window_client_area_rect (window, impl->window_scale, &window_rect);

  /* Turn client area into window area */
  _cdk_win32_adjust_client_rect (window, &window_rect);

  width = window_rect.right - window_rect.left;
  height = window_rect.bottom - window_rect.top;

  if (width > impl->dib_width ||
      height > impl->dib_height)
    {
      cairo_surface_t *new_cache;
      cairo_t *cr;

      /* Create larger cache surface, copy old cache surface over it */
      new_cache = cairo_win32_surface_create_with_dib (CAIRO_FORMAT_ARGB32,
                                                       width,
                                                       height);

      if (impl->cache_surface)
        {
          cr = cairo_create (new_cache);
          cairo_set_source_surface (cr, impl->cache_surface, 0, 0);
          cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
          cairo_paint (cr);
          cairo_destroy (cr);
          cairo_surface_flush (new_cache);

          cairo_surface_destroy (impl->cache_surface);
        }

      impl->cache_surface = new_cache;

      cairo_surface_set_device_scale (impl->cache_surface,
                                      impl->window_scale,
                                      impl->window_scale);

      if (impl->cairo_surface)
        cairo_surface_destroy (impl->cairo_surface);

      impl->cairo_surface = NULL;
    }

  /* This is separate, because cairo_surface gets killed
   * off frequently by outside code, whereas cache_surface
   * is only killed by us, above.
   */
  if (!impl->cairo_surface)
    {
      impl->cairo_surface = cairo_win32_surface_create_with_dib (CAIRO_FORMAT_ARGB32,
                                                                 width,
                                                                 height);
      impl->dib_width = width;
      impl->dib_height = height;

      cairo_surface_set_device_scale (impl->cairo_surface,
                                      impl->window_scale,
                                      impl->window_scale);

      cairo_surface_set_user_data (impl->cairo_surface, &cdk_win32_cairo_key,
				   impl, cdk_win32_cairo_surface_destroy);
    }
  else
    {
      cairo_surface_reference (impl->cairo_surface);
    }

  return impl->cairo_surface;
}

static cairo_surface_t *
cdk_win32_ref_cairo_surface (CdkWindow *window)
{
  CdkWindowImplWin32 *impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  if (CDK_IS_WINDOW_IMPL_WIN32 (impl) &&
      CDK_WINDOW_DESTROYED (impl->wrapper))
    return NULL;

  if (impl->layered)
    return cdk_win32_ref_cairo_surface_layered (window, impl);

  if (!impl->cairo_surface)
    {
      HDC hdc = _cdk_win32_impl_acquire_dc (impl);
      if (!hdc)
	return NULL;

      impl->cairo_surface = cairo_win32_surface_create_with_format (hdc, CAIRO_FORMAT_ARGB32);
      cairo_surface_set_device_scale (impl->cairo_surface,
                                      impl->window_scale,
                                      impl->window_scale);

      cairo_surface_set_user_data (impl->cairo_surface, &cdk_win32_cairo_key,
				   impl, cdk_win32_cairo_surface_destroy);
    }
  else
    cairo_surface_reference (impl->cairo_surface);

  return impl->cairo_surface;
}

BOOL WINAPI
CtkShowWindow (CdkWindow *window,
               int        cmd_show)
{
  cairo_t *cr;
  cairo_surface_t *surface;
  RECT window_rect;
  HDC hdc;
  POINT window_position;
  SIZE window_size;
  POINT source_point;
  BLENDFUNCTION blender;

  HWND hwnd = CDK_WINDOW_HWND (window);
  CdkWindowImplWin32 *impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  switch (cmd_show)
    {
    case SW_FORCEMINIMIZE:
    case SW_HIDE:
    case SW_MINIMIZE:
      break;
    case SW_MAXIMIZE:
    case SW_RESTORE:
    case SW_SHOW:
    case SW_SHOWDEFAULT:
    case SW_SHOWMINIMIZED:
    case SW_SHOWMINNOACTIVE:
    case SW_SHOWNA:
    case SW_SHOWNOACTIVATE:
    case SW_SHOWNORMAL:

      if (IsWindowVisible (hwnd))
        break;

      if ((WS_EX_LAYERED & GetWindowLongPtr (hwnd, GWL_EXSTYLE)) != WS_EX_LAYERED)
        break;

      /* Window was hidden, will be shown. Erase it, CDK will repaint soon,
       * but not soon enough, so it's possible to see old content before
       * the next redraw, unless we erase the window first.
       */
      GetWindowRect (hwnd, &window_rect);
      source_point.x = source_point.y = 0;

      window_position.x = window_rect.left;
      window_position.y = window_rect.top;
      window_size.cx = window_rect.right - window_rect.left;
      window_size.cy = window_rect.bottom - window_rect.top;

      blender.BlendOp = AC_SRC_OVER;
      blender.BlendFlags = 0;
      blender.AlphaFormat = AC_SRC_ALPHA;
      blender.SourceConstantAlpha = 255;

      /* Create a surface of appropriate size and clear it */
      surface = cairo_win32_surface_create_with_dib (CAIRO_FORMAT_ARGB32,
                                                     window_size.cx,
                                                     window_size.cy);
      cairo_surface_set_device_scale (surface, impl->window_scale, impl->window_scale);
      cr = cairo_create (surface);
      cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
      cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.0);
      cairo_paint (cr);
      cairo_destroy (cr);
      cairo_surface_flush (surface);
      hdc = cairo_win32_surface_get_dc (surface);

      /* No API_CALL() wrapper, don't check for errors */
      UpdateLayeredWindow (hwnd, NULL,
                           &window_position, &window_size,
                           hdc, &source_point,
                           0, &blender, ULW_ALPHA);

      cairo_surface_destroy (surface);

      break;
    }

  /* Ensure that maximized window size is corrected later on */
  if (cmd_show == SW_MAXIMIZE)
    impl->maximizing = TRUE;

  return ShowWindow (hwnd, cmd_show);
}

static void
cdk_win32_window_set_shadow_width (CdkWindow *window,
                                   gint       left,
                                   gint       right,
                                   gint       top,
                                   gint       bottom)
{
  CdkWindowImplWin32 *impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  if (CDK_WINDOW_DESTROYED (window))
    return;

  CDK_NOTE (MISC, g_print ("cdk_win32_window_set_shadow_width: window %p, "
                           "left %d, top %d, right %d, bottom %d\n",
                           window, left, top, right, bottom));

  impl->zero_margins = left == 0 && right == 0 && top == 0 && bottom == 0;

  if (impl->zero_margins)
    return;

  impl->margins.left = left;
  impl->margins.right = right * impl->window_scale;
  impl->margins.top = top;
  impl->margins.bottom = bottom * impl->window_scale;
  impl->margins_x = left + right;
  impl->margins_y = top + bottom;
}


gint
_cdk_win32_window_get_scale_factor (CdkWindow *window)
{
  CdkDisplay *display;
  CdkWindowImplWin32 *impl;

  CdkWin32Display *win32_display;
  UINT dpix, dpiy;
  gboolean is_scale_acquired;

  if (CDK_WINDOW_DESTROYED (window))
    return 1;

  g_return_val_if_fail (window != NULL, 1);

  display = cdk_window_get_display (window);
  impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  win32_display = CDK_WIN32_DISPLAY (display);

  if (win32_display->dpi_aware_type != PROCESS_DPI_UNAWARE)
    {
      if (win32_display->has_fixed_scale)
        impl->window_scale = win32_display->window_scale;
      else
        impl->window_scale = _cdk_win32_display_get_monitor_scale_factor (win32_display,
                                                                          NULL,
                                                                          CDK_WINDOW_HWND (window),
                                                                          NULL);

      return impl->window_scale;
    }
  else
    {
      if (win32_display->has_fixed_scale)
        {
          static gsize hidpi_msg_displayed = 0;

          if (g_once_init_enter (&hidpi_msg_displayed))
            {
              g_message ("Note: CDK_SCALE is ignored as HiDPI awareness is disabled.");
              g_once_init_leave (&hidpi_msg_displayed, 1);
            }
        }

      /* Application is not DPI aware, don't bother */
      return 1;
    }
}

void
_cdk_win32_window_get_unscaled_size (CdkWindow *window,
                                    gint      *unscaled_width,
                                    gint      *unscaled_height)
{
  CdkWindowImplWin32 *impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  if (unscaled_width)
    *unscaled_width = impl->unscaled_width;
  if (unscaled_height)
    *unscaled_height = impl->unscaled_height;
}

static void
cdk_window_impl_win32_class_init (CdkWindowImplWin32Class *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CdkWindowImplClass *impl_class = CDK_WINDOW_IMPL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = cdk_window_impl_win32_finalize;

  impl_class->ref_cairo_surface = cdk_win32_ref_cairo_surface;

  impl_class->show = cdk_win32_window_show;
  impl_class->hide = cdk_win32_window_hide;
  impl_class->withdraw = cdk_win32_window_withdraw;
  impl_class->set_events = cdk_win32_window_set_events;
  impl_class->get_events = cdk_win32_window_get_events;
  impl_class->raise = cdk_win32_window_raise;
  impl_class->lower = cdk_win32_window_lower;
  impl_class->restack_under = cdk_win32_window_restack_under;
  impl_class->restack_toplevel = cdk_win32_window_restack_toplevel;
  impl_class->move_resize = cdk_win32_window_move_resize;
  impl_class->set_background = cdk_win32_window_set_background;
  impl_class->reparent = cdk_win32_window_reparent;
  impl_class->set_device_cursor = cdk_win32_window_set_device_cursor;
  impl_class->get_geometry = cdk_win32_window_get_geometry;
  impl_class->get_device_state = cdk_window_win32_get_device_state;
  impl_class->get_root_coords = cdk_win32_window_get_root_coords;

  impl_class->shape_combine_region = cdk_win32_window_shape_combine_region;
  impl_class->input_shape_combine_region = cdk_win32_input_shape_combine_region;
  impl_class->destroy = cdk_win32_window_destroy;
  impl_class->destroy_foreign = cdk_win32_window_destroy_foreign;
  impl_class->get_shape = cdk_win32_window_get_shape;
  //FIXME?: impl_class->get_input_shape = cdk_win32_window_get_input_shape;
  impl_class->begin_paint = cdk_win32_window_begin_paint;
  impl_class->end_paint = cdk_win32_window_end_paint;

  //impl_class->beep = cdk_x11_window_beep;


  impl_class->show_window_menu = cdk_win32_window_show_window_menu;
  impl_class->focus = cdk_win32_window_focus;
  impl_class->set_type_hint = cdk_win32_window_set_type_hint;
  impl_class->get_type_hint = cdk_win32_window_get_type_hint;
  impl_class->set_modal_hint = cdk_win32_window_set_modal_hint;
  impl_class->set_skip_taskbar_hint = cdk_win32_window_set_skip_taskbar_hint;
  impl_class->set_skip_pager_hint = cdk_win32_window_set_skip_pager_hint;
  impl_class->set_urgency_hint = cdk_win32_window_set_urgency_hint;
  impl_class->set_geometry_hints = cdk_win32_window_set_geometry_hints;
  impl_class->set_title = cdk_win32_window_set_title;
  impl_class->set_role = cdk_win32_window_set_role;
  //impl_class->set_startup_id = cdk_x11_window_set_startup_id;
  impl_class->set_transient_for = cdk_win32_window_set_transient_for;
  impl_class->get_frame_extents = cdk_win32_window_get_frame_extents;
  impl_class->set_override_redirect = cdk_win32_window_set_override_redirect;
  impl_class->set_accept_focus = cdk_win32_window_set_accept_focus;
  impl_class->set_focus_on_map = cdk_win32_window_set_focus_on_map;
  impl_class->set_icon_list = cdk_win32_window_set_icon_list;
  impl_class->set_icon_name = cdk_win32_window_set_icon_name;
  impl_class->iconify = cdk_win32_window_iconify;
  impl_class->deiconify = cdk_win32_window_deiconify;
  impl_class->stick = cdk_win32_window_stick;
  impl_class->unstick = cdk_win32_window_unstick;
  impl_class->maximize = cdk_win32_window_maximize;
  impl_class->unmaximize = cdk_win32_window_unmaximize;
  impl_class->fullscreen = cdk_win32_window_fullscreen;
  impl_class->unfullscreen = cdk_win32_window_unfullscreen;
  impl_class->set_keep_above = cdk_win32_window_set_keep_above;
  impl_class->set_keep_below = cdk_win32_window_set_keep_below;
  impl_class->get_group = cdk_win32_window_get_group;
  impl_class->set_group = cdk_win32_window_set_group;
  impl_class->set_decorations = cdk_win32_window_set_decorations;
  impl_class->get_decorations = cdk_win32_window_get_decorations;
  impl_class->set_functions = cdk_win32_window_set_functions;

  impl_class->set_shadow_width = cdk_win32_window_set_shadow_width;
  impl_class->begin_resize_drag = cdk_win32_window_begin_resize_drag;
  impl_class->begin_move_drag = cdk_win32_window_begin_move_drag;
  impl_class->set_opacity = cdk_win32_window_set_opacity;
  //impl_class->set_composited = cdk_win32_window_set_composited;
  impl_class->destroy_notify = cdk_win32_window_destroy_notify;
  impl_class->get_drag_protocol = _cdk_win32_window_get_drag_protocol;
  impl_class->register_dnd = _cdk_win32_window_register_dnd;
  impl_class->drag_begin = _cdk_win32_window_drag_begin;
  //? impl_class->sync_rendering = _cdk_win32_window_sync_rendering;
  impl_class->simulate_key = _cdk_win32_window_simulate_key;
  impl_class->simulate_button = _cdk_win32_window_simulate_button;
  impl_class->get_property = _cdk_win32_window_get_property;
  impl_class->change_property = _cdk_win32_window_change_property;
  impl_class->delete_property = _cdk_win32_window_delete_property;
  impl_class->create_gl_context = _cdk_win32_window_create_gl_context;
  impl_class->invalidate_for_new_frame = _cdk_win32_window_invalidate_for_new_frame;
  impl_class->get_scale_factor = _cdk_win32_window_get_scale_factor;
  impl_class->get_unscaled_size = _cdk_win32_window_get_unscaled_size;
}

HGDIOBJ
cdk_win32_window_get_handle (CdkWindow *window)
{
  /* Try to ensure the window has a native window */
  if (!_cdk_window_has_impl (window))
    cdk_window_ensure_native (window);

  if (!CDK_WINDOW_IS_WIN32 (window))
    {
      g_warning (G_STRLOC " window is not a native Win32 window");
      return NULL;
    }

  return CDK_WINDOW_HWND (window);
}

#ifdef CDK_WIN32_ENABLE_EGL
EGLSurface
_cdk_win32_window_get_egl_surface (CdkWindow *window,
                                  EGLConfig  config,
                                  gboolean   is_dummy)
{
  EGLSurface surface;
  CdkWin32Display *display = CDK_WIN32_DISPLAY (cdk_window_get_display (window));
  CdkWindowImplWin32 *impl = CDK_WINDOW_IMPL_WIN32 (window->impl);

  if (is_dummy)
    {
      if (impl->egl_dummy_surface == EGL_NO_SURFACE)
        {
          EGLint attribs[] = {EGL_WIDTH, 1, EGL_WIDTH, 1, EGL_NONE};
          impl->egl_dummy_surface = eglCreatePbufferSurface (display->egl_disp,
                                                             config,
                                                             attribs);
        }
      return impl->egl_dummy_surface;
    }
  else
    {
      if (impl->egl_surface == EGL_NO_SURFACE)
        impl->egl_surface = eglCreateWindowSurface (display->egl_disp, config, display->gl_hwnd, NULL);

      return impl->egl_surface;
    }

}
#endif
