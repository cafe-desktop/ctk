/* cdkwindow-quartz.c
 *
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 2005-2007 Imendio AB
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

#include <cdk/cdk.h>
#include <cdk/cdkdeviceprivate.h>
#include <cdk/cdkdisplayprivate.h>

#include "cdkwindowimpl.h"
#include "cdkwindow-quartz.h"
#include "cdkprivate-quartz.h"
#include "cdkglcontext-quartz.h"
#include "cdkquartzglcontext.h"
#include "cdkquartzscreen.h"
#include "cdkquartzcursor.h"
#include "cdkquartz-ctk-only.h"

#include <Carbon/Carbon.h>
#include <AvailabilityMacros.h>

#include <sys/time.h>
#include <cairo-quartz.h>

static gpointer parent_class;
static gpointer root_window_parent_class;

static GSList   *update_nswindows;
static gboolean  in_process_all_updates = FALSE;

static GSList *main_window_stack;

void _cdk_quartz_window_flush (CdkWindowImplQuartz *window_impl);

typedef struct
{
  gint            x, y;
  gint            width, height;
  CdkWMDecoration decor;
} FullscreenSavedGeometry;

#if MAC_OS_X_VERSION_MIN_REQUIRED < 101200
typedef enum
{
 GDK_QUARTZ_BORDERLESS_WINDOW = NSBorderlessWindowMask,
 GDK_QUARTZ_CLOSABLE_WINDOW = NSClosableWindowMask,
#if MAC_OS_X_VERSION_MIN_REQUIRED > 1060
 /* Added in 10.7. Apple's docs are wrong to say it's from earlier. */
 GDK_QUARTZ_FULLSCREEN_WINDOW = NSFullScreenWindowMask,
#endif
 GDK_QUARTZ_MINIATURIZABLE_WINDOW = NSMiniaturizableWindowMask,
 GDK_QUARTZ_RESIZABLE_WINDOW = NSResizableWindowMask,
 GDK_QUARTZ_TITLED_WINDOW = NSTitledWindowMask,
} CdkQuartzWindowMask;
#else
typedef enum
{
 GDK_QUARTZ_BORDERLESS_WINDOW = NSWindowStyleMaskBorderless,
 GDK_QUARTZ_CLOSABLE_WINDOW = NSWindowStyleMaskClosable,
 GDK_QUARTZ_FULLSCREEN_WINDOW = NSWindowStyleMaskFullScreen,
 GDK_QUARTZ_MINIATURIZABLE_WINDOW = NSWindowStyleMaskMiniaturizable,
 GDK_QUARTZ_RESIZABLE_WINDOW = NSWindowStyleMaskResizable,
 GDK_QUARTZ_TITLED_WINDOW = NSWindowStyleMaskTitled,
} CdkQuartzWindowMask;
#endif

#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1070
static FullscreenSavedGeometry *get_fullscreen_geometry (CdkWindow *window);
#endif

#define FULLSCREEN_DATA "fullscreen-data"

static void update_toplevel_order (void);
static void clear_toplevel_order  (void);

#define WINDOW_IS_TOPLEVEL(window)		     \
  (GDK_WINDOW_TYPE (window) != GDK_WINDOW_CHILD &&   \
   GDK_WINDOW_TYPE (window) != GDK_WINDOW_FOREIGN && \
   GDK_WINDOW_TYPE (window) != GDK_WINDOW_OFFSCREEN)

/*
 * CdkQuartzWindow
 */

struct _CdkQuartzWindow
{
  CdkWindow parent;
};

struct _CdkQuartzWindowClass
{
  CdkWindowClass parent_class;
};

G_DEFINE_TYPE (CdkQuartzWindow, cdk_quartz_window, GDK_TYPE_WINDOW);

static void
cdk_quartz_window_class_init (CdkQuartzWindowClass *quartz_window_class)
{
}

static void
cdk_quartz_window_init (CdkQuartzWindow *quartz_window)
{
}


/*
 * CdkQuartzWindowImpl
 */

NSView *
cdk_quartz_window_get_nsview (CdkWindow *window)
{
  if (GDK_WINDOW_DESTROYED (window))
    return NULL;

  return ((CdkWindowImplQuartz *)window->impl)->view;
}

NSWindow *
cdk_quartz_window_get_nswindow (CdkWindow *window)
{
  if (GDK_WINDOW_DESTROYED (window))
    return NULL;

  return ((CdkWindowImplQuartz *)window->impl)->toplevel;
}

static CGContextRef
cdk_window_impl_quartz_get_context (CdkWindowImplQuartz *window_impl,
				    gboolean             antialias)
{
  CGContextRef cg_context = NULL;
  CGSize scale;

  if (GDK_WINDOW_DESTROYED (window_impl->wrapper))
    return NULL;

  /* Lock focus when not called as part of a drawRect call. This
   * is needed when called from outside "real" expose events, for
   * example for synthesized expose events when realizing windows
   * and for widgets that send fake expose events like the arrow
   * buttons in spinbuttons or the position marker in rulers.
   */
  if (window_impl->in_paint_rect_count == 0)
    {
      if (![window_impl->view lockFocusIfCanDraw])
        return NULL;
    }
#if MAC_OS_X_VERSION_MAX_ALLOWED < 101000
    cg_context = [[NSGraphicsContext currentContext] graphicsPort];
#else
  if (cdk_quartz_osx_version () < GDK_OSX_YOSEMITE)
    cg_context = [[NSGraphicsContext currentContext] graphicsPort];
  else
    cg_context = [[NSGraphicsContext currentContext] CGContext];
#endif

  if (!cg_context)
    return NULL;
  CGContextSaveGState (cg_context);
  CGContextSetAllowsAntialiasing (cg_context, antialias);

  /* Undo the default scaling transform, since we apply our own
   * in cdk_quartz_ref_cairo_surface () */
  scale = CGContextConvertSizeToDeviceSpace (cg_context,
                                             CGSizeMake (1.0, 1.0));
  CGContextScaleCTM (cg_context, 1.0 / scale.width, 1.0 / scale.height);

  return cg_context;
}

static void
cdk_window_impl_quartz_release_context (CdkWindowImplQuartz *window_impl,
                                        CGContextRef         cg_context)
{
  if (cg_context)
    {
      CGContextRestoreGState (cg_context);
      CGContextSetAllowsAntialiasing (cg_context, TRUE);
    }

  /* See comment in cdk_quartz_window_get_context(). */
  if (window_impl->in_paint_rect_count == 0)
    {
      _cdk_quartz_window_flush (window_impl);
      [window_impl->view unlockFocus];
    }
}

static void
cdk_window_impl_quartz_finalize (GObject *object)
{
  CdkWindowImplQuartz *impl = GDK_WINDOW_IMPL_QUARTZ (object);
  CdkDisplay *display = cdk_window_get_display (impl->wrapper);
  CdkSeat *seat = cdk_display_get_default_seat (display);

  cdk_seat_ungrab (seat);

  if (impl->transient_for)
    g_object_unref (impl->transient_for);

  if (impl->view)
    [[NSNotificationCenter defaultCenter] removeObserver: impl->toplevel
                                       name: @"NSViewFrameDidChangeNotification"
                                     object: impl->view];

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/* Help preventing "beam sync penalty" where CG makes all graphics code
 * block until the next vsync if we try to flush (including call display on
 * a view) too often. We do this by limiting the manual flushing done
 * outside of expose calls to less than some frequency when measured over
 * the last 4 flushes. This is a bit arbitray, but seems to make it possible
 * for some quick manual flushes (such as ctkruler or gimp’s marching ants)
 * without hitting the max flush frequency.
 *
 * If drawable NULL, no flushing is done, only registering that a flush was
 * done externally.
 *
 * Note: As of MacOS 10.14 NSWindow flushWindow is deprecated because
 * Quartz has the ability to handle deferred drawing on its own.
 */
void
_cdk_quartz_window_flush (CdkWindowImplQuartz *window_impl)
{
#if MAC_OS_X_VERSION_MIN_REQUIRED < 101400
  static struct timeval prev_tv;
  static gint intervals[4];
  static gint index;
  struct timeval tv;
  gint ms;

  gettimeofday (&tv, NULL);
  ms = (tv.tv_sec - prev_tv.tv_sec) * 1000 + (tv.tv_usec - prev_tv.tv_usec) / 1000;
  intervals[index++ % 4] = ms;

  if (window_impl)
    {
      ms = intervals[0] + intervals[1] + intervals[2] + intervals[3];

      /* ~25Hz on average. */
      if (ms > 4*40)
        {
          if (window_impl)
            [window_impl->toplevel flushWindow];

          prev_tv = tv;
        }
    }
  else
    prev_tv = tv;
#endif
}

static cairo_user_data_key_t cdk_quartz_cairo_key;

typedef struct {
  CdkWindowImplQuartz  *window_impl;
  CGContextRef  cg_context;
} CdkQuartzCairoSurfaceData;

static void
cdk_quartz_cairo_surface_destroy (void *data)
{
  CdkQuartzCairoSurfaceData *surface_data = data;

  surface_data->window_impl->cairo_surface = NULL;

  cdk_quartz_window_release_context (surface_data->window_impl,
                                     surface_data->cg_context);

  g_free (surface_data);
}

static cairo_surface_t *
cdk_quartz_create_cairo_surface (CdkWindowImplQuartz *impl,
				 int                  width,
				 int                  height)
{
  CGContextRef cg_context;
  CdkQuartzCairoSurfaceData *surface_data;
  cairo_surface_t *surface;

  cg_context = cdk_quartz_window_get_context (impl, TRUE);

  surface_data = g_new (CdkQuartzCairoSurfaceData, 1);
  surface_data->window_impl = impl;
  surface_data->cg_context = cg_context;

  if (cg_context)
    surface = cairo_quartz_surface_create_for_cg_context (cg_context,
                                                          width, height);
  else
    surface = cairo_quartz_surface_create(CAIRO_FORMAT_ARGB32, width, height);

  cairo_surface_set_user_data (surface, &cdk_quartz_cairo_key,
                               surface_data,
                               cdk_quartz_cairo_surface_destroy);

  return surface;
}

static cairo_surface_t *
cdk_quartz_ref_cairo_surface (CdkWindow *window)
{
  CdkWindowImplQuartz *impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);

  if (GDK_WINDOW_DESTROYED (window))
    return NULL;

  if (!impl->cairo_surface)
    {
      gint scale = cdk_window_get_scale_factor (impl->wrapper);

      impl->cairo_surface = 
          cdk_quartz_create_cairo_surface (impl,
                                           cdk_window_get_width (impl->wrapper) * scale,
                                           cdk_window_get_height (impl->wrapper) * scale);

      cairo_surface_set_device_scale (impl->cairo_surface, scale, scale);
    }
  else
    cairo_surface_reference (impl->cairo_surface);

  return impl->cairo_surface;
}

static void
cdk_window_impl_quartz_init (CdkWindowImplQuartz *impl)
{
  impl->type_hint = GDK_WINDOW_TYPE_HINT_NORMAL;
}

static gboolean
cdk_window_impl_quartz_begin_paint (CdkWindow *window)
{
  return FALSE;
}

static void
cdk_quartz_window_set_needs_display_in_region (CdkWindow    *window,
                                               cairo_region_t    *region)
{
  CdkWindowImplQuartz *impl;
  int i, n_rects;

  impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);

  if (!impl->needs_display_region)
    impl->needs_display_region = cairo_region_create ();

  cairo_region_union (impl->needs_display_region, region);

  n_rects = cairo_region_num_rectangles (region);
  for (i = 0; i < n_rects; i++)
    {
      cairo_rectangle_int_t rect;
      cairo_region_get_rectangle (region, i, &rect);
      [impl->view setNeedsDisplayInRect:NSMakeRect (rect.x, rect.y,
                                                    rect.width, rect.height)];
    }
}

void
_cdk_quartz_window_process_updates_recurse (CdkWindow *window,
                                            cairo_region_t *region)
{
  /* Make sure to only flush each toplevel at most once if we're called
   * from process_all_updates.
   */
  if (in_process_all_updates)
    {
      CdkWindow *toplevel;

      toplevel = cdk_window_get_effective_toplevel (window);
      if (toplevel && WINDOW_IS_TOPLEVEL (toplevel))
        {
          CdkWindowImplQuartz *toplevel_impl;
          NSWindow *nswindow;

          toplevel_impl = (CdkWindowImplQuartz *)toplevel->impl;
          nswindow = toplevel_impl->toplevel;
#if MAC_OS_X_VERSION_MIN_REQUIRED < 101400
          /* In theory, we could skip the flush disabling, since we only
           * have one NSView.
           */
          if (nswindow && ![nswindow isFlushWindowDisabled]) 
            {
              [nswindow retain];
              [nswindow disableFlushWindow];
              update_nswindows = g_slist_prepend (update_nswindows, nswindow);
            }
#endif
        }
    }

  if (WINDOW_IS_TOPLEVEL (window))
    cdk_quartz_window_set_needs_display_in_region (window, region);
  else
    _cdk_window_process_updates_recurse (window, region);

  /* NOTE: I'm not sure if we should displayIfNeeded here. It slows down a
   * lot (since it triggers the beam syncing) and things seem to work
   * without it.
   */
}

void
_cdk_quartz_display_before_process_all_updates (CdkDisplay *display)
{
  in_process_all_updates = TRUE;

  if (cdk_quartz_osx_version () >= GDK_OSX_EL_CAPITAN)
    {
      [NSAnimationContext endGrouping];
    }
#if MAC_OS_X_VERSION_MIN_REQUIRED < 101100
  else
    {
      NSDisableScreenUpdates ();
    }
#endif
}

void
_cdk_quartz_display_after_process_all_updates (CdkDisplay *display)
{
  GSList *tmp_list = update_nswindows;

  update_nswindows = NULL;

  while (tmp_list)
    {
      NSWindow *nswindow = tmp_list->data;

      [[nswindow contentView] displayIfNeeded];

      _cdk_quartz_window_flush (NULL);
#if MAC_OS_X_VERSION_MIN_REQUIRED < 101400
      [nswindow enableFlushWindow];
      [nswindow flushWindow];
#endif
      [nswindow release];

      tmp_list = g_slist_remove_link (tmp_list, tmp_list);
    }

  in_process_all_updates = FALSE;

  if (cdk_quartz_osx_version() >= GDK_OSX_EL_CAPITAN)
    {
      [NSAnimationContext beginGrouping];
    }
#if MAC_OS_X_VERSION_MIN_REQUIRED < 101100
  else
    {
      NSEnableScreenUpdates ();
    }
#endif
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

static void
get_ancestor_coordinates_from_child (CdkWindow *child_window,
				     gint       child_x,
				     gint       child_y,
				     CdkWindow *ancestor_window, 
				     gint      *ancestor_x, 
				     gint      *ancestor_y)
{
  while (child_window != ancestor_window)
    {
      child_x += child_window->x;
      child_y += child_window->y;

      child_window = child_window->parent;
    }

  *ancestor_x = child_x;
  *ancestor_y = child_y;
}

void
_cdk_quartz_window_debug_highlight (CdkWindow *window, gint number)
{
  gint x, y;
  gint gx, gy;
  CdkWindow *toplevel;
  gint tx, ty;
  static NSWindow *debug_window[10];
  static NSRect old_rect[10];
  NSRect rect;
  NSColor *color;

  g_return_if_fail (number >= 0 && number <= 9);

  if (window == _cdk_root)
    return;

  if (window == NULL)
    {
      if (debug_window[number])
        [debug_window[number] close];
      debug_window[number] = NULL;

      return;
    }

  toplevel = cdk_window_get_toplevel (window);
  get_ancestor_coordinates_from_child (window, 0, 0, toplevel, &x, &y);

  cdk_window_get_origin (toplevel, &tx, &ty);
  x += tx;
  y += ty;

  _cdk_quartz_window_cdk_xy_to_xy (x, y + window->height,
                                   &gx, &gy);

  rect = NSMakeRect (gx, gy, window->width, window->height);

  if (debug_window[number] && NSEqualRects (rect, old_rect[number]))
    return;

  old_rect[number] = rect;

  if (debug_window[number])
    [debug_window[number] close];

  debug_window[number] = [[NSWindow alloc] initWithContentRect:rect
#if MAC_OS_X_VERSION_MIN_REQUIRED < 101200
                                                     styleMask:(NSUInteger)GDK_QUARTZ_BORDERLESS_WINDOW
#else
                                                     styleMask:(NSWindowStyleMask)GDK_QUARTZ_BORDERLESS_WINDOW
#endif
			                               backing:NSBackingStoreBuffered
			                                 defer:NO];

  switch (number)
    {
    case 0:
      color = [NSColor redColor];
      break;
    case 1:
      color = [NSColor blueColor];
      break;
    case 2:
      color = [NSColor greenColor];
      break;
    case 3:
      color = [NSColor yellowColor];
      break;
    case 4:
      color = [NSColor brownColor];
      break;
    case 5:
      color = [NSColor purpleColor];
      break;
    default:
      color = [NSColor blackColor];
      break;
    }

  [debug_window[number] setBackgroundColor:color];
  [debug_window[number] setAlphaValue:0.4];
  [debug_window[number] setOpaque:NO];
  [debug_window[number] setReleasedWhenClosed:YES];
  [debug_window[number] setIgnoresMouseEvents:YES];
  [debug_window[number] setLevel:NSFloatingWindowLevel];

  [debug_window[number] orderFront:nil];
}

gboolean
_cdk_quartz_window_is_ancestor (CdkWindow *ancestor,
                                CdkWindow *window)
{
  if (ancestor == NULL || window == NULL)
    return FALSE;

  return (cdk_window_get_parent (window) == ancestor ||
          _cdk_quartz_window_is_ancestor (ancestor, 
                                          cdk_window_get_parent (window)));
}


/* See notes on top of cdkscreen-quartz.c */
void
_cdk_quartz_window_cdk_xy_to_xy (gint  cdk_x,
                                 gint  cdk_y,
                                 gint *ns_x,
                                 gint *ns_y)
{
  CdkQuartzScreen *screen_quartz = GDK_QUARTZ_SCREEN (_cdk_screen);

  if (ns_y)
    *ns_y = screen_quartz->orig_y - cdk_y;

  if (ns_x)
    *ns_x = cdk_x + screen_quartz->orig_x;
}

void
_cdk_quartz_window_xy_to_cdk_xy (gint  ns_x,
                                 gint  ns_y,
                                 gint *cdk_x,
                                 gint *cdk_y)
{
  CdkQuartzScreen *screen_quartz = GDK_QUARTZ_SCREEN (_cdk_screen);

  if (cdk_y)
    *cdk_y = screen_quartz->orig_y - ns_y;

  if (cdk_x)
    *cdk_x = ns_x - screen_quartz->orig_x;
}

void
_cdk_quartz_window_nspoint_to_cdk_xy (NSPoint  point,
                                      gint    *x,
                                      gint    *y)
{
  _cdk_quartz_window_xy_to_cdk_xy (point.x, point.y,
                                   x, y);
}

static CdkWindow *
find_child_window_helper (CdkWindow *window,
			  gint       x,
			  gint       y,
			  gint       x_offset,
			  gint       y_offset,
                          gboolean   get_toplevel)
{
  CdkWindowImplQuartz *impl;
  GList *l;

  impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);

  if (window == _cdk_root)
    update_toplevel_order ();

  for (l = impl->sorted_children; l; l = l->next)
    {
      CdkWindow *child = l->data;
      CdkWindowImplQuartz *child_impl = GDK_WINDOW_IMPL_QUARTZ (child->impl);
      int temp_x, temp_y;

      if (!GDK_WINDOW_IS_MAPPED (child))
	continue;

      temp_x = x_offset + child->x;
      temp_y = y_offset + child->y;

      /* Special-case the root window. We have to include the title
       * bar in the checks, otherwise the window below the title bar
       * will be found i.e. events punch through. (If we can find a
       * better way to deal with the events in cdkevents-quartz, this
       * might not be needed.)
       */
      if (window == _cdk_root)
        {
          NSRect frame = NSMakeRect (0, 0, 100, 100);
          NSRect content;
          NSUInteger mask;
          int titlebar_height;

          mask = [child_impl->toplevel styleMask];

          /* Get the title bar height. */
          content = [NSWindow contentRectForFrameRect:frame
                                            styleMask:mask];
          titlebar_height = frame.size.height - content.size.height;

          if (titlebar_height > 0 &&
              x >= temp_x && y >= temp_y - titlebar_height &&
              x < temp_x + child->width && y < temp_y)
            {
              /* The root means "unknown" i.e. a window not managed by
               * GDK.
               */
              return (CdkWindow *)_cdk_root;
            }
        }

      if ((!get_toplevel || (get_toplevel && window == _cdk_root)) &&
          x >= temp_x && y >= temp_y &&
	  x < temp_x + child->width && y < temp_y + child->height)
	{
	  /* Look for child windows. */
	  return find_child_window_helper (l->data,
					   x, y,
					   temp_x, temp_y,
                                           get_toplevel);
	}
    }
  
  return window;
}

/* Given a CdkWindow and coordinates relative to it, returns the
 * innermost subwindow that contains the point. If the coordinates are
 * outside the passed in window, NULL is returned.
 */
CdkWindow *
_cdk_quartz_window_find_child (CdkWindow *window,
			       gint       x,
			       gint       y,
                               gboolean   get_toplevel)
{
  if (x >= 0 && y >= 0 && x < window->width && y < window->height)
    return find_child_window_helper (window, x, y, 0, 0, get_toplevel);

  return NULL;
}

/* Raises a transient window.
 */
static void
raise_transient (CdkWindowImplQuartz *impl)
{
  /* In quartz the transient-for behavior is implemented by
   * attaching the transient-for CdkNSWindows to the parent's
   * CdkNSWindow. Stacking is managed by Quartz and the order
   * is that of the parent's childWindows array. The only way
   * to change that order is to remove the child from the
   * parent and then add it back in.
   */
  CdkWindowImplQuartz *parent_impl =
        GDK_WINDOW_IMPL_QUARTZ (impl->transient_for->impl);
  [parent_impl->toplevel removeChildWindow:impl->toplevel];
  [parent_impl->toplevel addChildWindow:impl->toplevel
                         ordered:NSWindowAbove];
}

void
_cdk_quartz_window_did_become_main (CdkWindow *window)
{
  CdkWindowImplQuartz *impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);

  main_window_stack = g_slist_remove (main_window_stack, window);

  if (window->window_type != GDK_WINDOW_TEMP)
    main_window_stack = g_slist_prepend (main_window_stack, window);

  if (impl->transient_for)
    raise_transient (impl);

  clear_toplevel_order ();
}

void
_cdk_quartz_window_did_resign_main (CdkWindow *window)
{
  CdkWindow *new_window = NULL;

  if (main_window_stack)
    new_window = main_window_stack->data;
  else
    {
      GList *toplevels;

      toplevels = cdk_screen_get_toplevel_windows (cdk_screen_get_default ());
      if (toplevels)
        new_window = toplevels->data;
      g_list_free (toplevels);
    }

  if (new_window &&
      new_window != window &&
      GDK_WINDOW_IS_MAPPED (new_window) &&
      WINDOW_IS_TOPLEVEL (new_window))
    {
      CdkWindowImplQuartz *impl = GDK_WINDOW_IMPL_QUARTZ (new_window->impl);

      [impl->toplevel makeKeyAndOrderFront:impl->toplevel];
    }

  clear_toplevel_order ();
}

static NSScreen *
get_nsscreen_for_point (gint x, gint y)
{
  int i;
  NSArray *screens;
  NSScreen *screen = NULL;

  GDK_QUARTZ_ALLOC_POOL;

  screens = [NSScreen screens];

  for (i = 0; i < [screens count]; i++)
    {
      NSRect rect = [[screens objectAtIndex:i] frame];

      if (x >= rect.origin.x && x <= rect.origin.x + rect.size.width &&
          y >= rect.origin.y && y <= rect.origin.y + rect.size.height)
        {
          screen = [screens objectAtIndex:i];
          break;
        }
    }

  GDK_QUARTZ_RELEASE_POOL;

  return screen;
}

void
_cdk_quartz_display_create_window_impl (CdkDisplay    *display,
                                        CdkWindow     *window,
                                        CdkWindow     *real_parent,
                                        CdkScreen     *screen,
                                        CdkEventMask   event_mask,
                                        CdkWindowAttr *attributes,
                                        gint           attributes_mask)
{
  CdkWindowImplQuartz *impl;
  CdkWindowImplQuartz *parent_impl;
  CdkWindowTypeHint    type_hint = GDK_WINDOW_TYPE_HINT_NORMAL;

  GDK_QUARTZ_ALLOC_POOL;

  impl = g_object_new (GDK_TYPE_WINDOW_IMPL_QUARTZ, NULL);
  window->impl = GDK_WINDOW_IMPL (impl);
  impl->wrapper = window;

  parent_impl = GDK_WINDOW_IMPL_QUARTZ (window->parent->impl);

  switch (window->window_type)
    {
    case GDK_WINDOW_TOPLEVEL:
    case GDK_WINDOW_TEMP:
      if (GDK_WINDOW_TYPE (window->parent) != GDK_WINDOW_ROOT)
	{
	  /* The common code warns for this case */
          parent_impl = GDK_WINDOW_IMPL_QUARTZ (_cdk_root->impl);
	}
    }

  /* Maintain the z-ordered list of children. */
  if (window->parent != _cdk_root)
    parent_impl->sorted_children = g_list_prepend (parent_impl->sorted_children, window);
  else
    clear_toplevel_order ();

  cdk_window_set_cursor (window, ((attributes_mask & GDK_WA_CURSOR) ?
				  (attributes->cursor) :
				  NULL));

  impl->view = NULL;

  if (attributes_mask & GDK_WA_TYPE_HINT)
    {
      type_hint = attributes->type_hint;
      cdk_window_set_type_hint (window, type_hint);
    }

  switch (window->window_type)
    {
    case GDK_WINDOW_TOPLEVEL:
    case GDK_WINDOW_TEMP:
      {
        NSScreen *screen;
        NSRect screen_rect;
        NSRect content_rect;
        NSUInteger style_mask;
        int nx, ny;
        const char *title;

        /* initWithContentRect will place on the mainScreen by default.
         * We want to select the screen to place on ourselves.  We need
         * to find the screen the window will be on and correct the
         * content_rect coordinates to be relative to that screen.
         */
        _cdk_quartz_window_cdk_xy_to_xy (window->x, window->y, &nx, &ny);

        screen = get_nsscreen_for_point (nx, ny);
        screen_rect = [screen frame];
        nx -= screen_rect.origin.x;
        ny -= screen_rect.origin.y;

        content_rect = NSMakeRect (nx, ny - window->height,
                                   window->width,
                                   window->height);

        if (window->window_type == GDK_WINDOW_TEMP ||
            type_hint == GDK_WINDOW_TYPE_HINT_SPLASHSCREEN)
          {
            style_mask = GDK_QUARTZ_BORDERLESS_WINDOW;
          }
        else
          {
            style_mask = (GDK_QUARTZ_TITLED_WINDOW |
                          GDK_QUARTZ_CLOSABLE_WINDOW |
                          GDK_QUARTZ_MINIATURIZABLE_WINDOW |
                          GDK_QUARTZ_RESIZABLE_WINDOW);
          }

	impl->toplevel = [[CdkQuartzNSWindow alloc] initWithContentRect:content_rect 
			                                      styleMask:style_mask
			                                        backing:NSBackingStoreBuffered
			                                          defer:NO
                                                                  screen:screen];

        if (type_hint != GDK_WINDOW_TYPE_HINT_NORMAL)
          impl->toplevel.excludedFromWindowsMenu = true;

	if (attributes_mask & GDK_WA_TITLE)
	  title = attributes->title;
	else
	  title = get_default_title ();

	cdk_window_set_title (window, title);
  
	if (cdk_window_get_visual (window) == cdk_screen_get_rgba_visual (_cdk_screen))
	  {
	    [impl->toplevel setOpaque:NO];
	    [impl->toplevel setBackgroundColor:[NSColor clearColor]];
	  }

        content_rect.origin.x = 0;
        content_rect.origin.y = 0;

	impl->view = [[CdkQuartzView alloc] initWithFrame:content_rect];
	[impl->view setCdkWindow:window];
	[impl->toplevel setContentView:impl->view];
        [[NSNotificationCenter defaultCenter] addObserver: impl->toplevel
                                      selector: @selector (windowDidResize:)
                                      name: @"NSViewFrameDidChangeNotification"
                                      object: impl->view];
	[impl->view release];
      }
      break;

    case GDK_WINDOW_CHILD:
      {
	CdkWindowImplQuartz *parent_impl = GDK_WINDOW_IMPL_QUARTZ (window->parent->impl);

	if (!window->input_only)
	  {
	    NSRect frame_rect = NSMakeRect (window->x + window->parent->abs_x,
                                            window->y + window->parent->abs_y,
                                            window->width,
                                            window->height);
	
	    impl->view = [[CdkQuartzView alloc] initWithFrame:frame_rect];
	    
	    [impl->view setCdkWindow:window];

	    /* CdkWindows should be hidden by default */
	    [impl->view setHidden:YES];
	    [parent_impl->view addSubview:impl->view];
	    [impl->view release];
	  }
      }
      break;

    default:
      g_assert_not_reached ();
    }

  GDK_QUARTZ_RELEASE_POOL;
}

void
_cdk_quartz_window_update_position (CdkWindow *window)
{
  NSRect frame_rect;
  NSRect content_rect;
  CdkWindowImplQuartz *impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);

  GDK_QUARTZ_ALLOC_POOL;

  frame_rect = [impl->toplevel frame];
  content_rect = [impl->toplevel contentRectForFrameRect:frame_rect];

  _cdk_quartz_window_xy_to_cdk_xy (content_rect.origin.x,
                                   content_rect.origin.y + content_rect.size.height,
                                   &window->x, &window->y);


  GDK_QUARTZ_RELEASE_POOL;
}

void
_cdk_quartz_window_init_windowing (CdkDisplay *display,
                                   CdkScreen  *screen)
{
  CdkWindowImplQuartz *impl;

  g_assert (_cdk_root == NULL);

  _cdk_root = _cdk_display_create_window (display);

  _cdk_root->impl = g_object_new (_cdk_root_window_impl_quartz_get_type (), NULL);
  _cdk_root->impl_window = _cdk_root;
  _cdk_root->visual = cdk_screen_get_system_visual (screen);

  impl = GDK_WINDOW_IMPL_QUARTZ (_cdk_root->impl);

  _cdk_quartz_screen_update_window_sizes (screen);

  _cdk_root->state = 0; /* We don't want GDK_WINDOW_STATE_WITHDRAWN here */
  _cdk_root->window_type = GDK_WINDOW_ROOT;
  _cdk_root->depth = 24;
  _cdk_root->viewable = TRUE;

  impl->wrapper = _cdk_root;
}

static void
cdk_quartz_window_destroy (CdkWindow *window,
                           gboolean   recursing,
                           gboolean   foreign_destroy)
{
  CdkWindowImplQuartz *impl;
  CdkWindow *parent;

  impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);

  main_window_stack = g_slist_remove (main_window_stack, window);

  g_list_free (impl->sorted_children);
  impl->sorted_children = NULL;

  parent = window->parent;
  if (parent)
    {
      CdkWindowImplQuartz *parent_impl = GDK_WINDOW_IMPL_QUARTZ (parent->impl);

      parent_impl->sorted_children = g_list_remove (parent_impl->sorted_children, window);
    }

  if (impl->cairo_surface)
    {
      cairo_surface_finish (impl->cairo_surface);
      cairo_surface_set_user_data (impl->cairo_surface, &cdk_quartz_cairo_key,
				   NULL, NULL);
      impl->cairo_surface = NULL;
    }

  if (!recursing && !foreign_destroy)
    {
      GDK_QUARTZ_ALLOC_POOL;

      if (impl->toplevel)
	[impl->toplevel close];
      else if (impl->view)
	[impl->view removeFromSuperview];

      GDK_QUARTZ_RELEASE_POOL;
    }
}

static void
cdk_quartz_window_destroy_foreign (CdkWindow *window)
{
  /* Foreign windows aren't supported in OSX. */
}

/* FIXME: This might be possible to simplify with client-side windows. Also
 * note that already_mapped is not used yet, see the x11 backend.
*/
static void
cdk_window_quartz_show (CdkWindow *window, gboolean already_mapped)
{
  CdkWindowImplQuartz *impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);
  gboolean focus_on_map;

  GDK_QUARTZ_ALLOC_POOL;

  if (!GDK_WINDOW_IS_MAPPED (window))
    focus_on_map = window->focus_on_map;
  else
    focus_on_map = TRUE;

  if (WINDOW_IS_TOPLEVEL (window) && impl->toplevel)
    {
      gboolean make_key;

      make_key = (window->accept_focus && focus_on_map &&
                  window->window_type != GDK_WINDOW_TEMP);

      [(CdkQuartzNSWindow*)impl->toplevel showAndMakeKey:make_key];
      clear_toplevel_order ();

      _cdk_quartz_events_send_map_event (window);
    }
  else
    {
      [impl->view setHidden:NO];
    }

  [impl->view setNeedsDisplay:YES];

  cdk_synthesize_window_state (window, GDK_WINDOW_STATE_WITHDRAWN, 0);

  if (window->state & GDK_WINDOW_STATE_MAXIMIZED)
    cdk_window_maximize (window);

  if (window->state & GDK_WINDOW_STATE_ICONIFIED)
    cdk_window_iconify (window);

  if (impl->transient_for && !GDK_WINDOW_DESTROYED (impl->transient_for))
    _cdk_quartz_window_attach_to_parent (window);

  GDK_QUARTZ_RELEASE_POOL;
}

/* Temporarily unsets the parent window, if the window is a
 * transient. 
 */
void
_cdk_quartz_window_detach_from_parent (CdkWindow *window)
{
  CdkWindowImplQuartz *impl;

  g_return_if_fail (GDK_IS_WINDOW (window));

  impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);
  
  g_return_if_fail (impl->toplevel != NULL);

  if (impl->transient_for && !GDK_WINDOW_DESTROYED (impl->transient_for))
    {
      CdkWindowImplQuartz *parent_impl;

      parent_impl = GDK_WINDOW_IMPL_QUARTZ (impl->transient_for->impl);
      [parent_impl->toplevel removeChildWindow:impl->toplevel];
      clear_toplevel_order ();
    }
}

/* Re-sets the parent window, if the window is a transient. */
void
_cdk_quartz_window_attach_to_parent (CdkWindow *window)
{
  CdkWindowImplQuartz *impl;

  g_return_if_fail (GDK_IS_WINDOW (window));

  impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);
  
  g_return_if_fail (impl->toplevel != NULL);

  if (impl->transient_for && !GDK_WINDOW_DESTROYED (impl->transient_for))
    {
      CdkWindowImplQuartz *parent_impl;

      parent_impl = GDK_WINDOW_IMPL_QUARTZ (impl->transient_for->impl);
      [parent_impl->toplevel addChildWindow:impl->toplevel ordered:NSWindowAbove];
      clear_toplevel_order ();
    }
}

void
cdk_window_quartz_hide (CdkWindow *window)
{
  CdkWindowImplQuartz *impl;
  CdkDisplay *display = cdk_window_get_display (window);
  CdkSeat *seat = cdk_display_get_default_seat (display);
  cdk_seat_ungrab (seat);

  /* Make sure we're not stuck in fullscreen mode. */
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1070
  if (get_fullscreen_geometry (window))
    SetSystemUIMode (kUIModeNormal, 0);
#endif

  _cdk_window_clear_update_area (window);

  impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);

  if (WINDOW_IS_TOPLEVEL (window)) 
    {
     /* Update main window. */
      main_window_stack = g_slist_remove (main_window_stack, window);
      if ([NSApp mainWindow] == impl->toplevel)
        _cdk_quartz_window_did_resign_main (window);

      if (impl->transient_for)
        _cdk_quartz_window_detach_from_parent (window);

      [(CdkQuartzNSWindow*)impl->toplevel hide];
    }
  else if (impl->view)
    {
      [impl->view setHidden:YES];
    }
}

void
cdk_window_quartz_withdraw (CdkWindow *window)
{
  cdk_window_hide (window);
}

static void
move_resize_window_internal (CdkWindow *window,
			     gint       x,
			     gint       y,
			     gint       width,
			     gint       height)
{
  CdkWindowImplQuartz *impl;
  CdkRectangle old_visible;
  CdkRectangle new_visible;
  CdkRectangle scroll_rect;
  cairo_region_t *old_region;
  cairo_region_t *expose_region;
  NSSize delta;

  if (GDK_WINDOW_DESTROYED (window))
    return;

  impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);

  if ((x == -1 || (x == window->x)) &&
      (y == -1 || (y == window->y)) &&
      (width == -1 || (width == window->width)) &&
      (height == -1 || (height == window->height)))
    {
      return;
    }

  if (!impl->toplevel)
    {
      /* The previously visible area of this window in a coordinate
       * system rooted at the origin of this window.
       */
      old_visible.x = -window->x;
      old_visible.y = -window->y;

      old_visible.width = window->width;
      old_visible.height = window->height;
    }

  if (x != -1)
    {
      delta.width = x - window->x;
      window->x = x;
    }
  else
    {
      delta.width = 0;
    }

  if (y != -1)
    {
      delta.height = y - window->y;
      window->y = y;
    }
  else
    {
      delta.height = 0;
    }

  if (width != -1)
    window->width = width;

  if (height != -1)
    window->height = height;

  GDK_QUARTZ_ALLOC_POOL;

  if (impl->toplevel)
    {
      NSRect content_rect;
      NSRect frame_rect;
      gint gx, gy;

      _cdk_quartz_window_cdk_xy_to_xy (window->x, window->y + window->height,
                                       &gx, &gy);

      content_rect = NSMakeRect (gx, gy, window->width, window->height);

      frame_rect = [impl->toplevel frameRectForContentRect:content_rect];
      [impl->toplevel setFrame:frame_rect display:YES];
    }
  else 
    {
      if (!window->input_only)
        {
          NSRect nsrect;

          nsrect = NSMakeRect (window->x, window->y, window->width, window->height);

          /* The newly visible area of this window in a coordinate
           * system rooted at the origin of this window.
           */
          new_visible.x = -window->x;
          new_visible.y = -window->y;
          new_visible.width = old_visible.width;   /* parent has not changed size */
          new_visible.height = old_visible.height; /* parent has not changed size */

          expose_region = cairo_region_create_rectangle (&new_visible);
          old_region = cairo_region_create_rectangle (&old_visible);
          cairo_region_subtract (expose_region, old_region);

          /* Determine what (if any) part of the previously visible
           * part of the window can be copied without a redraw
           */
          scroll_rect = old_visible;
          scroll_rect.x -= delta.width;
          scroll_rect.y -= delta.height;
          cdk_rectangle_intersect (&scroll_rect, &old_visible, &scroll_rect);

          if (!cairo_region_is_empty (expose_region))
            {
              if (scroll_rect.width != 0 && scroll_rect.height != 0)
                {
                  [impl->view scrollRect:NSMakeRect (scroll_rect.x,
                                                     scroll_rect.y,
                                                     scroll_rect.width,
                                                     scroll_rect.height)
			              by:delta];
                }

              [impl->view setFrame:nsrect];

              cdk_quartz_window_set_needs_display_in_region (window, expose_region);
            }
          else
            {
              [impl->view setFrame:nsrect];
              [impl->view setNeedsDisplay:YES];
            }

          cairo_region_destroy (expose_region);
          cairo_region_destroy (old_region);
        }
    }

  if (window->gl_paint_context != NULL)
    [GDK_QUARTZ_GL_CONTEXT (window->gl_paint_context)->gl_context update];

  GDK_QUARTZ_RELEASE_POOL;
}

static inline void
window_quartz_move (CdkWindow *window,
                    gint       x,
                    gint       y)
{
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (window->state & GDK_WINDOW_STATE_FULLSCREEN)
    return;

  move_resize_window_internal (window, x, y, -1, -1);
}

static inline void
window_quartz_resize (CdkWindow *window,
                      gint       width,
                      gint       height)
{
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (window->state & GDK_WINDOW_STATE_FULLSCREEN)
    return;

  if (width < 1)
    width = 1;
  if (height < 1)
    height = 1;

  move_resize_window_internal (window, -1, -1, width, height);
}

static inline void
window_quartz_move_resize (CdkWindow *window,
                           gint       x,
                           gint       y,
                           gint       width,
                           gint       height)
{
  if (width < 1)
    width = 1;
  if (height < 1)
    height = 1;

  move_resize_window_internal (window, x, y, width, height);
}

static void
cdk_window_quartz_move_resize (CdkWindow *window,
                               gboolean   with_move,
                               gint       x,
                               gint       y,
                               gint       width,
                               gint       height)
{
  if (with_move && (width < 0 && height < 0))
    window_quartz_move (window, x, y);
  else
    {
      if (with_move)
        window_quartz_move_resize (window, x, y, width, height);
      else
        window_quartz_resize (window, width, height);
    }
}

/* FIXME: This might need fixing (reparenting didn't work before client-side
 * windows either).
 */
static gboolean
cdk_window_quartz_reparent (CdkWindow *window,
                            CdkWindow *new_parent,
                            gint       x,
                            gint       y)
{
  CdkWindow *old_parent;
  CdkWindowImplQuartz *impl, *old_parent_impl, *new_parent_impl;
  NSView *view, *new_parent_view;

  if (new_parent == _cdk_root)
    {
      /* Could be added, just needs implementing. */
      g_warning ("Reparenting to root window is not supported yet in the Mac OS X backend");
      return FALSE;
    }

  impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);
  view = impl->view;

  new_parent_impl = GDK_WINDOW_IMPL_QUARTZ (new_parent->impl);
  new_parent_view = new_parent_impl->view;

  old_parent = window->parent;
  old_parent_impl = GDK_WINDOW_IMPL_QUARTZ (old_parent->impl);

  [view retain];

  [view removeFromSuperview];
  [new_parent_view addSubview:view];

  [view release];

  window->parent = new_parent;

  if (old_parent)
    {
      old_parent_impl->sorted_children = g_list_remove (old_parent_impl->sorted_children, window);
    }

  new_parent_impl->sorted_children = g_list_prepend (new_parent_impl->sorted_children, window);

  return FALSE;
}

/* Get the toplevel ordering from NSApp and update our own list. We do
 * this on demand since the NSApp’s list is not up to date directly
 * after we get windowDidBecomeMain.
 */
static void
update_toplevel_order (void)
{
  CdkWindowImplQuartz *root_impl;
  NSEnumerator *enumerator;
  id nswindow;
  GList *toplevels = NULL;

  root_impl = GDK_WINDOW_IMPL_QUARTZ (_cdk_root->impl);

  if (root_impl->sorted_children)
    return;

  GDK_QUARTZ_ALLOC_POOL;

  enumerator = [[NSApp orderedWindows] objectEnumerator];
  while ((nswindow = [enumerator nextObject]))
    {
      CdkWindow *window;

      if (![[nswindow contentView] isKindOfClass:[CdkQuartzView class]])
        continue;

      window = [(CdkQuartzView *)[nswindow contentView] cdkWindow];
      toplevels = g_list_prepend (toplevels, window);
    }

  GDK_QUARTZ_RELEASE_POOL;

  root_impl->sorted_children = g_list_reverse (toplevels);
}

static void
clear_toplevel_order (void)
{
  CdkWindowImplQuartz *root_impl;

  root_impl = GDK_WINDOW_IMPL_QUARTZ (_cdk_root->impl);

  g_list_free (root_impl->sorted_children);
  root_impl->sorted_children = NULL;
}

static void
cdk_window_quartz_raise (CdkWindow *window)
{
  if (GDK_WINDOW_DESTROYED (window))
    return;

  if (WINDOW_IS_TOPLEVEL (window))
    {
      CdkWindowImplQuartz *impl;

      impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);

      if (impl->transient_for)
        raise_transient (impl);
      else
        [impl->toplevel orderFront:impl->toplevel];

      clear_toplevel_order ();
    }
  else
    {
      CdkWindow *parent = window->parent;

      if (parent)
        {
          CdkWindowImplQuartz *impl;

          impl = (CdkWindowImplQuartz *)parent->impl;

          impl->sorted_children = g_list_remove (impl->sorted_children, window);
          impl->sorted_children = g_list_prepend (impl->sorted_children, window);
        }
    }
}

static void
cdk_window_quartz_lower (CdkWindow *window)
{
  if (GDK_WINDOW_DESTROYED (window))
    return;

  if (WINDOW_IS_TOPLEVEL (window))
    {
      CdkWindowImplQuartz *impl;

      impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);
      [impl->toplevel orderBack:impl->toplevel];

      clear_toplevel_order ();
    }
  else
    {
      CdkWindow *parent = window->parent;

      if (parent)
        {
          CdkWindowImplQuartz *impl;

          impl = (CdkWindowImplQuartz *)parent->impl;

          impl->sorted_children = g_list_remove (impl->sorted_children, window);
          impl->sorted_children = g_list_append (impl->sorted_children, window);
        }
    }
}

static void
cdk_window_quartz_restack_toplevel (CdkWindow *window,
				    CdkWindow *sibling,
				    gboolean   above)
{
  CdkWindowImplQuartz *impl;
  gint sibling_num;

  impl = GDK_WINDOW_IMPL_QUARTZ (sibling->impl);
  sibling_num = [impl->toplevel windowNumber];

  impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);

  if (above)
    [impl->toplevel orderWindow:NSWindowAbove relativeTo:sibling_num];
  else
    [impl->toplevel orderWindow:NSWindowBelow relativeTo:sibling_num];
}

static void
cdk_window_quartz_set_background (CdkWindow       *window,
                                  cairo_pattern_t *pattern)
{
  /* FIXME: We could theoretically set the background color for toplevels
   * here. (Currently we draw the background before emitting expose events)
   */
}

static void
cdk_window_quartz_set_device_cursor (CdkWindow *window,
                                     CdkDevice *device,
                                     CdkCursor *cursor)
{
  NSCursor *nscursor;

  if (GDK_WINDOW_DESTROYED (window))
    return;

  nscursor = _cdk_quartz_cursor_get_ns_cursor (cursor);

  [nscursor set];
}

static void
cdk_window_quartz_get_geometry (CdkWindow *window,
                                gint      *x,
                                gint      *y,
                                gint      *width,
                                gint      *height)
{
  CdkWindowImplQuartz *impl;
  NSRect ns_rect;

  if (GDK_WINDOW_DESTROYED (window))
    return;

  impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);
  if (window == _cdk_root)
    {
      if (x) 
        *x = 0;
      if (y) 
        *y = 0;

      if (width) 
        *width = window->width;
      if (height)
        *height = window->height;
    }
  else if (WINDOW_IS_TOPLEVEL (window))
    {
      ns_rect = [impl->toplevel contentRectForFrameRect:[impl->toplevel frame]];

      /* This doesn't work exactly as in X. There doesn't seem to be a
       * way to get the coords relative to the parent window (usually
       * the window frame), but that seems useless except for
       * borderless windows where it's relative to the root window. So
       * we return (0, 0) (should be something like (0, 22)) for
       * windows with borders and the root relative coordinates
       * otherwise.
       */
      if ([impl->toplevel styleMask] == GDK_QUARTZ_BORDERLESS_WINDOW)
        {
          _cdk_quartz_window_xy_to_cdk_xy (ns_rect.origin.x,
                                           ns_rect.origin.y + ns_rect.size.height,
                                           x, y);
        }
      else 
        {
          if (x)
            *x = 0;
          if (y)
            *y = 0;
        }

      if (width)
        *width = ns_rect.size.width;
      if (height)
        *height = ns_rect.size.height;
    }
  else
    {
      ns_rect = [impl->view frame];
      
      if (x)
        *x = ns_rect.origin.x;
      if (y)
        *y = ns_rect.origin.y;
      if (width)
        *width  = ns_rect.size.width;
      if (height)
        *height = ns_rect.size.height;
    }
}

static void
cdk_window_quartz_get_root_coords (CdkWindow *window,
                                   gint       x,
                                   gint       y,
                                   gint      *root_x,
                                   gint      *root_y)
{
  int tmp_x = 0, tmp_y = 0;
  CdkWindow *toplevel;
  NSRect content_rect;
  CdkWindowImplQuartz *impl;

  if (GDK_WINDOW_DESTROYED (window)) 
    {
      if (root_x)
	*root_x = 0;
      if (root_y)
	*root_y = 0;
      
      return;
    }

  if (window == _cdk_root)
    {
      if (root_x)
        *root_x = x;
      if (root_y)
        *root_y = y;

      return;
    }
  
  toplevel = cdk_window_get_toplevel (window);
  impl = GDK_WINDOW_IMPL_QUARTZ (toplevel->impl);

  content_rect = [impl->toplevel contentRectForFrameRect:[impl->toplevel frame]];

  _cdk_quartz_window_xy_to_cdk_xy (content_rect.origin.x,
                                   content_rect.origin.y + content_rect.size.height,
                                   &tmp_x, &tmp_y);

  tmp_x += x;
  tmp_y += y;

  while (window != toplevel)
    {
      if (_cdk_window_has_impl ((CdkWindow *)window))
        {
          tmp_x += window->x;
          tmp_y += window->y;
        }

      window = window->parent;
    }

  if (root_x)
    *root_x = tmp_x;
  if (root_y)
    *root_y = tmp_y;
}

/* Returns coordinates relative to the passed in window. */
static CdkWindow *
cdk_window_quartz_get_device_state_helper (CdkWindow       *window,
                                           CdkDevice       *device,
                                           gdouble         *x,
                                           gdouble         *y,
                                           CdkModifierType *mask)
{
  NSPoint point;
  gint x_tmp, y_tmp;
  CdkWindow *toplevel;
  CdkWindow *found_window;

  g_return_val_if_fail (window == NULL || GDK_IS_WINDOW (window), NULL);

  if (GDK_WINDOW_DESTROYED (window))
    {
      *x = 0;
      *y = 0;
      *mask = 0;
      return NULL;
    }
  
  toplevel = cdk_window_get_toplevel (window);

  *mask = _cdk_quartz_events_get_current_keyboard_modifiers () |
      _cdk_quartz_events_get_current_mouse_modifiers ();

  /* Get the y coordinate, needs to be flipped. */
  if (window == _cdk_root)
    {
      point = [NSEvent mouseLocation];
      _cdk_quartz_window_nspoint_to_cdk_xy (point, &x_tmp, &y_tmp);
    }
  else
    {
      CdkWindowImplQuartz *impl;
      NSWindow *nswindow;

      impl = GDK_WINDOW_IMPL_QUARTZ (toplevel->impl);
      nswindow = impl->toplevel;

      point = [nswindow mouseLocationOutsideOfEventStream];

      x_tmp = point.x;
      y_tmp = toplevel->height - point.y;

      window = (CdkWindow *)toplevel;
    }

  found_window = _cdk_quartz_window_find_child (window, x_tmp, y_tmp,
                                                FALSE);

  /* We never return the root window. */
  if (found_window == _cdk_root)
    found_window = NULL;

  *x = x_tmp;
  *y = y_tmp;

  return found_window;
}

static gboolean
cdk_window_quartz_get_device_state (CdkWindow       *window,
                                    CdkDevice       *device,
                                    gdouble          *x,
                                    gdouble          *y,
                                    CdkModifierType *mask)
{
  return cdk_window_quartz_get_device_state_helper (window,
                                                    device,
                                                    x, y, mask) != NULL;
}

static CdkEventMask
cdk_window_quartz_get_events (CdkWindow *window)
{
  if (GDK_WINDOW_DESTROYED (window))
    return 0;
  else
    return window->event_mask;
}

static void
cdk_window_quartz_set_events (CdkWindow       *window,
                              CdkEventMask     event_mask)
{
  /* The mask is set in the common code. */
}

static void
cdk_quartz_window_set_urgency_hint (CdkWindow *window,
                                    gboolean   urgent)
{
  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  /* FIXME: Implement */
}

static void
cdk_quartz_window_set_geometry_hints (CdkWindow         *window,
                                      const CdkGeometry *geometry,
                                      CdkWindowHints     geom_mask)
{
  CdkWindowImplQuartz *impl;

  g_return_if_fail (geometry != NULL);

  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;
  
  impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);
  if (!impl->toplevel)
    return;

  if (geom_mask & GDK_HINT_POS)
    {
      /* FIXME: Implement */
    }

  if (geom_mask & GDK_HINT_USER_POS)
    {
      /* FIXME: Implement */
    }

  if (geom_mask & GDK_HINT_USER_SIZE)
    {
      /* FIXME: Implement */
    }
  
  if (geom_mask & GDK_HINT_MIN_SIZE)
    {
      NSSize size;

      size.width = geometry->min_width;
      size.height = geometry->min_height;

      [impl->toplevel setContentMinSize:size];
    }
  
  if (geom_mask & GDK_HINT_MAX_SIZE)
    {
      NSSize size;

      size.width = geometry->max_width;
      size.height = geometry->max_height;

      [impl->toplevel setContentMaxSize:size];
    }
  
  if (geom_mask & GDK_HINT_BASE_SIZE)
    {
      /* FIXME: Implement */
    }
  
  if (geom_mask & GDK_HINT_RESIZE_INC)
    {
      NSSize size;

      size.width = geometry->width_inc;
      size.height = geometry->height_inc;

      [impl->toplevel setContentResizeIncrements:size];
    }
  
  if (geom_mask & GDK_HINT_ASPECT)
    {
      NSSize size;

      if (geometry->min_aspect != geometry->max_aspect)
        {
          g_warning ("Only equal minimum and maximum aspect ratios are supported on Mac OS. Using minimum aspect ratio...");
        }

      size.width = geometry->min_aspect;
      size.height = 1.0;

      [impl->toplevel setContentAspectRatio:size];
    }

  if (geom_mask & GDK_HINT_WIN_GRAVITY)
    {
      /* FIXME: Implement */
    }
}

static void
cdk_quartz_window_set_title (CdkWindow   *window,
                             const gchar *title)
{
  CdkWindowImplQuartz *impl;

  g_return_if_fail (title != NULL);

  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);

  if (impl->toplevel)
    {
      GDK_QUARTZ_ALLOC_POOL;
      [impl->toplevel setTitle:[NSString stringWithUTF8String:title]];
      GDK_QUARTZ_RELEASE_POOL;
    }
}

static void
cdk_quartz_window_set_role (CdkWindow   *window,
                            const gchar *role)
{
  if (GDK_WINDOW_DESTROYED (window) ||
      WINDOW_IS_TOPLEVEL (window))
    return;

  /* FIXME: Implement */
}

static void
cdk_quartz_window_set_startup_id (CdkWindow   *window,
                                  const gchar *startup_id)
{
  /* FIXME: Implement? */
}

static void
cdk_quartz_window_set_transient_for (CdkWindow *window,
                                     CdkWindow *parent)
{
  CdkWindowImplQuartz *window_impl;
  CdkWindowImplQuartz *parent_impl;

  if (GDK_WINDOW_DESTROYED (window)  || GDK_WINDOW_DESTROYED (parent) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  window_impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);
  if (!window_impl->toplevel)
    return;

  GDK_QUARTZ_ALLOC_POOL;

  if (window_impl->transient_for)
    {
      _cdk_quartz_window_detach_from_parent (window);

      g_object_unref (window_impl->transient_for);
      window_impl->transient_for = NULL;
    }

  parent_impl = GDK_WINDOW_IMPL_QUARTZ (parent->impl);
  if (parent_impl->toplevel)
    {
      /* We save the parent because it needs to be unset/reset when
       * hiding and showing the window. 
       */

      /* We don't set transients for tooltips, they are already
       * handled by the window level being the top one. If we do, then
       * the parent window will be brought to the top just because the
       * tooltip is, which is not what we want.
       */
      if (cdk_window_get_type_hint (window) != GDK_WINDOW_TYPE_HINT_TOOLTIP)
        {
          window_impl->transient_for = g_object_ref (parent);

          /* We only add the window if it is shown, otherwise it will
           * be shown unconditionally here. If it is not shown, the
           * window will be added in show() instead.
           */
          if (!(window->state & GDK_WINDOW_STATE_WITHDRAWN))
            _cdk_quartz_window_attach_to_parent (window);
        }
    }
  
  GDK_QUARTZ_RELEASE_POOL;
}

static void
cdk_window_quartz_shape_combine_region (CdkWindow       *window,
                                        const cairo_region_t *shape,
                                        gint             x,
                                        gint             y)
{
  /* FIXME: Implement */
}

static void
cdk_window_quartz_input_shape_combine_region (CdkWindow       *window,
                                              const cairo_region_t *shape_region,
                                              gint             offset_x,
                                              gint             offset_y)
{
  /* FIXME: Implement */
}

static void
cdk_quartz_window_set_override_redirect (CdkWindow *window,
                                         gboolean override_redirect)
{
  /* FIXME: Implement */
}

static void
cdk_quartz_window_set_accept_focus (CdkWindow *window,
                                    gboolean accept_focus)
{
  window->accept_focus = accept_focus != FALSE;
}

static void
cdk_quartz_window_set_focus_on_map (CdkWindow *window,
                                    gboolean focus_on_map)
{
  window->focus_on_map = focus_on_map != FALSE;
}

static void
cdk_quartz_window_set_icon_name (CdkWindow   *window,
                                 const gchar *name)
{
  /* FIXME: Implement */
}

static void
cdk_quartz_window_focus (CdkWindow *window,
                         guint32    timestamp)
{
  CdkWindowImplQuartz *impl;
	
  impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);

  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  if (window->accept_focus && window->window_type != GDK_WINDOW_TEMP)
    {
      GDK_QUARTZ_ALLOC_POOL;
      [impl->toplevel makeKeyAndOrderFront:impl->toplevel];
      clear_toplevel_order ();
      GDK_QUARTZ_RELEASE_POOL;
    }
}

static gint
window_type_hint_to_level (CdkWindowTypeHint hint)
{
  /*  the order in this switch statement corresponds to the actual
   *  stacking order: the first group is top, the last group is bottom
   */
  switch (hint)
    {
    case GDK_WINDOW_TYPE_HINT_POPUP_MENU:
    case GDK_WINDOW_TYPE_HINT_COMBO:
    case GDK_WINDOW_TYPE_HINT_DND:
    case GDK_WINDOW_TYPE_HINT_TOOLTIP:
      return NSPopUpMenuWindowLevel;

    case GDK_WINDOW_TYPE_HINT_NOTIFICATION:
    case GDK_WINDOW_TYPE_HINT_SPLASHSCREEN:
      return NSStatusWindowLevel;

    case GDK_WINDOW_TYPE_HINT_MENU: /* Torn-off menu */
    case GDK_WINDOW_TYPE_HINT_DROPDOWN_MENU: /* Menu from menubar */
      return NSTornOffMenuWindowLevel;

    case GDK_WINDOW_TYPE_HINT_DOCK:
      return NSFloatingWindowLevel; /* NSDockWindowLevel is deprecated, and not replaced */

    case GDK_WINDOW_TYPE_HINT_UTILITY:
    case GDK_WINDOW_TYPE_HINT_DIALOG:  /* Dialog window */
    case GDK_WINDOW_TYPE_HINT_NORMAL:  /* Normal toplevel window */
    case GDK_WINDOW_TYPE_HINT_TOOLBAR: /* Window used to implement toolbars */
      return NSNormalWindowLevel;

    case GDK_WINDOW_TYPE_HINT_DESKTOP:
      return kCGDesktopWindowLevelKey; /* doesn't map to any real Cocoa model */

    default:
      break;
    }

  return NSNormalWindowLevel;
}

static gboolean
window_type_hint_to_shadow (CdkWindowTypeHint hint)
{
  switch (hint)
    {
    case GDK_WINDOW_TYPE_HINT_NORMAL:  /* Normal toplevel window */
    case GDK_WINDOW_TYPE_HINT_DIALOG:  /* Dialog window */
    case GDK_WINDOW_TYPE_HINT_DOCK:
    case GDK_WINDOW_TYPE_HINT_UTILITY:
    case GDK_WINDOW_TYPE_HINT_MENU: /* Torn-off menu */
    case GDK_WINDOW_TYPE_HINT_DROPDOWN_MENU: /* Menu from menubar */
    case GDK_WINDOW_TYPE_HINT_SPLASHSCREEN:
    case GDK_WINDOW_TYPE_HINT_POPUP_MENU:
    case GDK_WINDOW_TYPE_HINT_COMBO:
    case GDK_WINDOW_TYPE_HINT_NOTIFICATION:
    case GDK_WINDOW_TYPE_HINT_TOOLTIP:
      return TRUE;

    case GDK_WINDOW_TYPE_HINT_TOOLBAR: /* Window used to implement toolbars */
    case GDK_WINDOW_TYPE_HINT_DESKTOP: /* N/A */
    case GDK_WINDOW_TYPE_HINT_DND:
      break;

    default:
      break;
    }

  return FALSE;
}

static gboolean
window_type_hint_to_hides_on_deactivate (CdkWindowTypeHint hint)
{
  switch (hint)
    {
    case GDK_WINDOW_TYPE_HINT_UTILITY:
    case GDK_WINDOW_TYPE_HINT_MENU: /* Torn-off menu */
    case GDK_WINDOW_TYPE_HINT_SPLASHSCREEN:
    case GDK_WINDOW_TYPE_HINT_NOTIFICATION:
    case GDK_WINDOW_TYPE_HINT_TOOLTIP:
      return TRUE;

    default:
      break;
    }

  return FALSE;
}

static void
_cdk_quartz_window_update_has_shadow (CdkWindowImplQuartz *impl)
{
    gboolean has_shadow;

    /* In case there is any shadow set we have to turn off the
     * NSWindow setHasShadow as the system drawn ones wont match our
     * window boundary anymore */
    has_shadow = (window_type_hint_to_shadow (impl->type_hint) && !impl->shadow_max);

    [impl->toplevel setHasShadow: has_shadow];
}

static void
_cdk_quartz_window_set_collection_behavior (NSWindow *nswindow,
                                            CdkWindowTypeHint hint)
{
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1070
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 10110
#define GDK_QUARTZ_ALLOWS_TILING NSWindowCollectionBehaviorFullScreenAllowsTiling
#define GDK_QUARTZ_DISALLOWS_TILING NSWindowCollectionBehaviorFullScreenDisallowsTiling
#else
#define GDK_QUARTZ_ALLOWS_TILING 1 << 11
#define GDK_QUARTZ_DISALLOWS_TILING 1 << 12
#endif
  if (cdk_quartz_osx_version() >= GDK_OSX_LION)
    {
      /* Fullscreen Collection Behavior */
      NSWindowCollectionBehavior behavior = [nswindow collectionBehavior];
      switch (hint)
        {
        case GDK_WINDOW_TYPE_HINT_NORMAL:
        case GDK_WINDOW_TYPE_HINT_SPLASHSCREEN:
          behavior &= ~(NSWindowCollectionBehaviorFullScreenAuxiliary &
                        GDK_QUARTZ_DISALLOWS_TILING);
          behavior |= (NSWindowCollectionBehaviorFullScreenPrimary |
                       GDK_QUARTZ_ALLOWS_TILING);

          break;
        default:
          behavior &= ~(NSWindowCollectionBehaviorFullScreenPrimary &
                        GDK_QUARTZ_ALLOWS_TILING);
          behavior |= (NSWindowCollectionBehaviorFullScreenAuxiliary |
                       GDK_QUARTZ_DISALLOWS_TILING);
          break;
        }
      [nswindow setCollectionBehavior:behavior];
    }
#undef GDK_QUARTZ_ALLOWS_TILING
#undef GDK_QUARTZ_DISALLOWS_TILING
#endif
}

static void
cdk_quartz_window_set_type_hint (CdkWindow        *window,
                                 CdkWindowTypeHint hint)
{
  CdkWindowImplQuartz *impl;

  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);

  impl->type_hint = hint;

  /* Match the documentation, only do something if we're not mapped yet. */
  if (GDK_WINDOW_IS_MAPPED (window))
    return;

  _cdk_quartz_window_update_has_shadow (impl);
  if (impl->toplevel)
    _cdk_quartz_window_set_collection_behavior (impl->toplevel, hint);
  [impl->toplevel setLevel: window_type_hint_to_level (hint)];
  [impl->toplevel setHidesOnDeactivate: window_type_hint_to_hides_on_deactivate (hint)];
}

static CdkWindowTypeHint
cdk_quartz_window_get_type_hint (CdkWindow *window)
{
  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return GDK_WINDOW_TYPE_HINT_NORMAL;
  
  return GDK_WINDOW_IMPL_QUARTZ (window->impl)->type_hint;
}

static void
cdk_quartz_window_set_modal_hint (CdkWindow *window,
                                  gboolean   modal)
{
  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  /* FIXME: Implement */
}

static void
cdk_quartz_window_set_skip_taskbar_hint (CdkWindow *window,
                                         gboolean   skips_taskbar)
{
  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  /* FIXME: Implement */
}

static void
cdk_quartz_window_set_skip_pager_hint (CdkWindow *window,
                                       gboolean   skips_pager)
{
  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  /* FIXME: Implement */
}

static void
cdk_quartz_window_begin_resize_drag (CdkWindow     *window,
                                     CdkWindowEdge  edge,
                                     CdkDevice     *device,
                                     gint           button,
                                     gint           root_x,
                                     gint           root_y,
                                     guint32        timestamp)
{
  CdkWindowImplQuartz *impl;

  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;

  impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);

  if (!impl->toplevel)
    {
      g_warning ("Can't call cdk_window_begin_resize_drag on non-toplevel window");
      return;
    }

  [(CdkQuartzNSWindow *)impl->toplevel beginManualResize:edge];
}

static void
cdk_quartz_window_begin_move_drag (CdkWindow *window,
                                   CdkDevice *device,
                                   gint       button,
                                   gint       root_x,
                                   gint       root_y,
                                   guint32    timestamp)
{
  CdkWindowImplQuartz *impl;

  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);

  if (!impl->toplevel)
    {
      g_warning ("Can't call cdk_window_begin_move_drag on non-toplevel window");
      return;
    }

  [(CdkQuartzNSWindow *)impl->toplevel beginManualMove];
}

static void
cdk_quartz_window_set_icon_list (CdkWindow *window,
                                 GList     *pixbufs)
{
  /* FIXME: Implement */
}

static void
cdk_quartz_window_get_frame_extents (CdkWindow    *window,
                                     CdkRectangle *rect)
{
  CdkWindow *toplevel;
  CdkWindowImplQuartz *impl;
  NSRect ns_rect;

  g_return_if_fail (rect != NULL);


  rect->x = 0;
  rect->y = 0;
  rect->width = 1;
  rect->height = 1;
  
  toplevel = cdk_window_get_effective_toplevel (window);
  impl = GDK_WINDOW_IMPL_QUARTZ (toplevel->impl);

  ns_rect = [impl->toplevel frame];

  _cdk_quartz_window_xy_to_cdk_xy (ns_rect.origin.x,
                                   ns_rect.origin.y + ns_rect.size.height,
                                   &rect->x, &rect->y);

  rect->width = ns_rect.size.width;
  rect->height = ns_rect.size.height;
}

/* Fake protocol to make gcc think that it's OK to call setStyleMask
   even if it isn't. We check to make sure before actually calling
   it. */

@protocol CanSetStyleMask
- (void)setStyleMask:(int)mask;
@end

static void
cdk_quartz_window_set_decorations (CdkWindow       *window,
			    CdkWMDecoration  decorations)
{
  CdkWindowImplQuartz *impl;
  NSUInteger old_mask, new_mask;
  NSView *old_view;

  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);

  if (decorations == 0 || GDK_WINDOW_TYPE (window) == GDK_WINDOW_TEMP ||
      impl->type_hint == GDK_WINDOW_TYPE_HINT_SPLASHSCREEN )
    {
      new_mask = GDK_QUARTZ_BORDERLESS_WINDOW;
    }
  else
    {
      /* FIXME: Honor other GDK_DECOR_* flags. */
      new_mask = (GDK_QUARTZ_TITLED_WINDOW | GDK_QUARTZ_CLOSABLE_WINDOW |
                  GDK_QUARTZ_MINIATURIZABLE_WINDOW |
                  GDK_QUARTZ_RESIZABLE_WINDOW);
    }

  GDK_QUARTZ_ALLOC_POOL;

  old_mask = [impl->toplevel styleMask];

  if (old_mask != new_mask)
    {
      NSRect rect;

      old_view = [[impl->toplevel contentView] retain];

      rect = [impl->toplevel frame];

      /* Properly update the size of the window when the titlebar is
       * added or removed.
       */
      if (old_mask == GDK_QUARTZ_BORDERLESS_WINDOW &&
          new_mask != GDK_QUARTZ_BORDERLESS_WINDOW)
        {
          rect = [NSWindow frameRectForContentRect:rect styleMask:new_mask];

        }
      else if (old_mask != GDK_QUARTZ_BORDERLESS_WINDOW &&
               new_mask == GDK_QUARTZ_BORDERLESS_WINDOW)
        {
          rect = [NSWindow contentRectForFrameRect:rect styleMask:old_mask];
        }

      /* Note, before OS 10.6 there doesn't seem to be a way to change this
       * without recreating the toplevel. From 10.6 onward, a simple call to
       * setStyleMask takes care of most of this, except for ensuring that the
       * title is set.
       */
      if ([impl->toplevel respondsToSelector:@selector(setStyleMask:)])
        {
          NSString *title = [impl->toplevel title];

          [(id<CanSetStyleMask>)impl->toplevel setStyleMask:new_mask];

          /* It appears that unsetting and then resetting
           * GDK_QUARTZ_TITLED_WINDOW does not reset the title in the
           * title bar as might be expected.
           *
           * In theory we only need to set this if new_mask includes
           * GDK_QUARTZ_TITLED_WINDOW. This behaved extremely oddly when
           * conditionalized upon that and since it has no side effects (i.e.
           * if GDK_QUARTZ_TITLED_WINDOW is not requested, the title will not be
           * displayed) just do it unconditionally. We also must null check
           * 'title' before setting it to avoid crashing.
           */
          if (title)
            [impl->toplevel setTitle:title];
        }
      else
        {
          NSString *title = [impl->toplevel title];
          NSColor *bg = [impl->toplevel backgroundColor];
          NSScreen *screen = [impl->toplevel screen];

          /* Make sure the old window is closed, recall that releasedWhenClosed
           * is set on CdkQuartzWindows.
           */
          [impl->toplevel close];

          impl->toplevel = [[CdkQuartzNSWindow alloc] initWithContentRect:rect
                                                                styleMask:new_mask
                                                                  backing:NSBackingStoreBuffered
                                                                    defer:NO
                                                                   screen:screen];
          _cdk_quartz_window_update_has_shadow (impl);

          [impl->toplevel setLevel: window_type_hint_to_level (impl->type_hint)];
          if (title)
            [impl->toplevel setTitle:title];
          [impl->toplevel setBackgroundColor:bg];
          [impl->toplevel setHidesOnDeactivate: window_type_hint_to_hides_on_deactivate (impl->type_hint)];
          [impl->toplevel setContentView:old_view];
        }

      if (new_mask == GDK_QUARTZ_BORDERLESS_WINDOW)
        {
          [impl->toplevel setContentSize:rect.size];
        }
      else
        [impl->toplevel setFrame:rect display:YES];

      /* Invalidate the window shadow for non-opaque views that have shadow
       * enabled, to get the shadow shape updated.
       */
      if (![old_view isOpaque] && [impl->toplevel hasShadow])
        [(CdkQuartzView*)old_view setNeedsInvalidateShadow:YES];

      [old_view release];
    }

  GDK_QUARTZ_RELEASE_POOL;
}

static gboolean
cdk_quartz_window_get_decorations (CdkWindow       *window,
                                   CdkWMDecoration *decorations)
{
  CdkWindowImplQuartz *impl;

  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return FALSE;

  impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);

  if (decorations)
    {
      /* Borderless is 0, so we can't check it as a bit being set. */
      if ([impl->toplevel styleMask] == GDK_QUARTZ_BORDERLESS_WINDOW)
        {
          *decorations = 0;
        }
      else
        {
          /* FIXME: Honor the other GDK_DECOR_* flags. */
          *decorations = GDK_DECOR_ALL;
        }
    }

  return TRUE;
}

static void
cdk_quartz_window_set_functions (CdkWindow    *window,
                                 CdkWMFunction functions)
{
  CdkWindowImplQuartz *impl;
  gboolean min, max, close;

  g_return_if_fail (GDK_IS_WINDOW (window));

  impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);

  if (functions & GDK_FUNC_ALL)
    {
      min = !(functions & GDK_FUNC_MINIMIZE);
      max = !(functions & GDK_FUNC_MAXIMIZE);
      close = !(functions & GDK_FUNC_CLOSE);
    }
  else
    {
      min = (functions & GDK_FUNC_MINIMIZE);
      max = (functions & GDK_FUNC_MAXIMIZE);
      close = (functions & GDK_FUNC_CLOSE);
    }

  if (impl->toplevel)
    {
      NSUInteger mask = [impl->toplevel styleMask];

      if (min)
        mask = mask | GDK_QUARTZ_MINIATURIZABLE_WINDOW;
      else
        mask = mask & ~GDK_QUARTZ_MINIATURIZABLE_WINDOW;

      if (max)
        mask = mask | GDK_QUARTZ_RESIZABLE_WINDOW;
      else
        mask = mask & ~GDK_QUARTZ_RESIZABLE_WINDOW;

      if (close)
        mask = mask | GDK_QUARTZ_CLOSABLE_WINDOW;
      else
        mask = mask & ~GDK_QUARTZ_CLOSABLE_WINDOW;

      [impl->toplevel setStyleMask:mask];
    }
}

static void
cdk_quartz_window_stick (CdkWindow *window)
{
  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;
}

static void
cdk_quartz_window_unstick (CdkWindow *window)
{
  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;
}

static void
cdk_quartz_window_maximize (CdkWindow *window)
{
  CdkWindowImplQuartz *impl;
  gboolean maximized;

  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);
  maximized = cdk_window_get_state (window) & GDK_WINDOW_STATE_MAXIMIZED;

  if (GDK_WINDOW_IS_MAPPED (window))
    {
      GDK_QUARTZ_ALLOC_POOL;

      if (impl->toplevel && !maximized)
        [impl->toplevel zoom:nil];

      GDK_QUARTZ_RELEASE_POOL;
    }
}

static void
cdk_quartz_window_unmaximize (CdkWindow *window)
{
  CdkWindowImplQuartz *impl;
  gboolean maximized;

  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);
  maximized = cdk_window_get_state (window) & GDK_WINDOW_STATE_MAXIMIZED;

  if (GDK_WINDOW_IS_MAPPED (window))
    {
      GDK_QUARTZ_ALLOC_POOL;

      if (impl->toplevel && maximized)
        [impl->toplevel zoom:nil];

      GDK_QUARTZ_RELEASE_POOL;
    }
}

static void
cdk_quartz_window_iconify (CdkWindow *window)
{
  CdkWindowImplQuartz *impl;

  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);

  if (GDK_WINDOW_IS_MAPPED (window))
    {
      GDK_QUARTZ_ALLOC_POOL;

      if (impl->toplevel)
	[impl->toplevel miniaturize:nil];

      GDK_QUARTZ_RELEASE_POOL;
    }
  else
    {
      cdk_synthesize_window_state (window,
				   0,
				   GDK_WINDOW_STATE_ICONIFIED);
    }
}

static void
cdk_quartz_window_deiconify (CdkWindow *window)
{
  CdkWindowImplQuartz *impl;

  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);

  if (GDK_WINDOW_IS_MAPPED (window))
    {
      GDK_QUARTZ_ALLOC_POOL;

      if (impl->toplevel)
	[impl->toplevel deminiaturize:nil];

      GDK_QUARTZ_RELEASE_POOL;
    }
  else
    {
      cdk_synthesize_window_state (window,
				   GDK_WINDOW_STATE_ICONIFIED,
				   0);
    }
}

static gboolean
window_is_fullscreen (CdkWindow *window)
{
  CdkWindowImplQuartz *impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);

#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1070
  if (cdk_quartz_osx_version() >= GDK_OSX_LION)
    return ([impl->toplevel styleMask] & GDK_QUARTZ_FULLSCREEN_WINDOW) != 0;
  else
#endif
    return g_object_get_data (G_OBJECT (window), FULLSCREEN_DATA) != NULL;
}

static void
cdk_quartz_window_fullscreen (CdkWindow *window)
{
  CdkWindowImplQuartz *impl;

  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);

#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1070
  if (cdk_quartz_osx_version() >= GDK_OSX_LION)
    {
      if (!window_is_fullscreen (window))
        [impl->toplevel toggleFullScreen:nil];
    }
  else
    {
#endif
      FullscreenSavedGeometry *geometry;
      CdkWindowImplQuartz *impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);
      NSRect frame;

      if (GDK_WINDOW_DESTROYED (window) ||
          !WINDOW_IS_TOPLEVEL (window))
        return;

      geometry = get_fullscreen_geometry (window);
      if (!geometry)
        {
          geometry = g_new (FullscreenSavedGeometry, 1);

          geometry->x = window->x;
          geometry->y = window->y;
          geometry->width = window->width;
          geometry->height = window->height;

          if (!cdk_window_get_decorations (window, &geometry->decor))
            geometry->decor = GDK_DECOR_ALL;

          g_object_set_data_full (G_OBJECT (window),
                                  FULLSCREEN_DATA, geometry, 
                                  g_free);

          cdk_window_set_decorations (window, 0);

          frame = [[impl->toplevel screen] frame];
          move_resize_window_internal (window,
                                       0, 0, 
                                       frame.size.width, frame.size.height);
          [impl->toplevel setContentSize:frame.size];
          [impl->toplevel makeKeyAndOrderFront:impl->toplevel];

          clear_toplevel_order ();
        }

      SetSystemUIMode (kUIModeAllHidden, kUIOptionAutoShowMenuBar);

      cdk_synthesize_window_state (window, 0, GDK_WINDOW_STATE_FULLSCREEN);
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1070
    }
#endif
}

static void
cdk_quartz_window_unfullscreen (CdkWindow *window)
{
  CdkWindowImplQuartz *impl;

  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1070
  if (cdk_quartz_osx_version() >= GDK_OSX_LION)
    {
      impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);

      if (window_is_fullscreen (window))
        [impl->toplevel toggleFullScreen:nil];
    }
  else
    {
#endif
      CdkWindowImplQuartz *impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);
      FullscreenSavedGeometry *geometry;

      if (GDK_WINDOW_DESTROYED (window) ||
          !WINDOW_IS_TOPLEVEL (window))
        return;

      geometry = get_fullscreen_geometry (window);
      if (geometry)
        {
          SetSystemUIMode (kUIModeNormal, 0);

          move_resize_window_internal (window,
                                       geometry->x,
                                       geometry->y,
                                       geometry->width,
                                       geometry->height);

          cdk_window_set_decorations (window, geometry->decor);

          g_object_set_data (G_OBJECT (window), FULLSCREEN_DATA, NULL);

          [impl->toplevel makeKeyAndOrderFront:impl->toplevel];
          clear_toplevel_order ();

          cdk_synthesize_window_state (window, GDK_WINDOW_STATE_FULLSCREEN, 0);
        }

#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1070
    }
#endif
}

#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1070
static FullscreenSavedGeometry *
get_fullscreen_geometry (CdkWindow *window)
{
  return g_object_get_data (G_OBJECT (window), FULLSCREEN_DATA);
}
#endif

void
_cdk_quartz_window_update_fullscreen_state (CdkWindow *window)
{
  if (GDK_WINDOW_DESTROYED (window) || !WINDOW_IS_TOPLEVEL (window))
    return;

#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1070
  if (cdk_quartz_osx_version() >= GDK_OSX_LION)
    {
      gboolean is_fullscreen = window_is_fullscreen (window);
      gboolean was_fullscreen = (cdk_window_get_state (window) &
                                 GDK_WINDOW_STATE_FULLSCREEN) != 0;

      if (is_fullscreen != was_fullscreen)
        {
          if (is_fullscreen)
            cdk_synthesize_window_state (window, 0, GDK_WINDOW_STATE_FULLSCREEN);
          else
            cdk_synthesize_window_state (window, GDK_WINDOW_STATE_FULLSCREEN, 0);
        }
    }
#endif
}

static void
cdk_quartz_window_set_keep_above (CdkWindow *window,
                                  gboolean   setting)
{
  CdkWindowImplQuartz *impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);
  gint level;

  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  level = window_type_hint_to_level (cdk_window_get_type_hint (window));
  
  /* Adjust normal window level by one if necessary. */
  [impl->toplevel setLevel: level + (setting ? 1 : 0)];
}

static void
cdk_quartz_window_set_keep_below (CdkWindow *window,
                                  gboolean   setting)
{
  CdkWindowImplQuartz *impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);
  gint level;

  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;
  
  level = window_type_hint_to_level (cdk_window_get_type_hint (window));
  
  /* Adjust normal window level by one if necessary. */
  [impl->toplevel setLevel: level - (setting ? 1 : 0)];
}

/* X11 "feature" not useful in other backends. */
static CdkWindow *
cdk_quartz_window_get_group (CdkWindow *window)
{
    return NULL;
}

/* X11 "feature" not useful in other backends. */
static void
cdk_quartz_window_set_group (CdkWindow *window,
                             CdkWindow *leader)
{
}

static void
cdk_quartz_window_destroy_notify (CdkWindow *window)
{
  CdkDisplay *display = cdk_window_get_display (window);
  CdkSeat *seat = cdk_display_get_default_seat (display);
  cdk_seat_ungrab (seat);
}

static void
cdk_quartz_window_set_opacity (CdkWindow *window,
                               gdouble    opacity)
{
  CdkWindowImplQuartz *impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);

  g_return_if_fail (GDK_IS_WINDOW (window));
  g_return_if_fail (WINDOW_IS_TOPLEVEL (window));

  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  if (opacity < 0)
    opacity = 0;
  else if (opacity > 1)
    opacity = 1;

  [impl->toplevel setAlphaValue: opacity];
}

static void
cdk_quartz_window_set_shadow_width (CdkWindow *window,
                                    gint       left,
                                    gint       right,
                                    gint       top,
                                    gint       bottom)
{
  CdkWindowImplQuartz *impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);

  g_return_if_fail (GDK_IS_WINDOW (window));
  g_return_if_fail (WINDOW_IS_TOPLEVEL (window));

  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  impl->shadow_top = top;
  impl->shadow_max = MAX (MAX (left, right), MAX (top, bottom));
  _cdk_quartz_window_update_has_shadow (impl);
}

static cairo_region_t *
cdk_quartz_window_get_shape (CdkWindow *window)
{
  /* FIXME: implement */
  return NULL;
}

static cairo_region_t *
cdk_quartz_window_get_input_shape (CdkWindow *window)
{
  /* FIXME: implement */
  return NULL;
}

/* Protocol to build cleanly for OSX < 10.7 */
@protocol ScaleFactor
- (CGFloat) backingScaleFactor;
@end

static gint
cdk_quartz_window_get_scale_factor (CdkWindow *window)
{
  CdkWindowImplQuartz *impl;

  if (GDK_WINDOW_DESTROYED (window))
    return 1;

  impl = GDK_WINDOW_IMPL_QUARTZ (window->impl);

  if (impl->toplevel != NULL && cdk_quartz_osx_version() >= GDK_OSX_LION)
    return [(id <ScaleFactor>) impl->toplevel backingScaleFactor];

  return 1;
}

static void
cdk_window_impl_quartz_class_init (CdkWindowImplQuartzClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CdkWindowImplClass *impl_class = GDK_WINDOW_IMPL_CLASS (klass);
  CdkWindowImplQuartzClass *impl_quartz_class = GDK_WINDOW_IMPL_QUARTZ_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = cdk_window_impl_quartz_finalize;

  impl_class->ref_cairo_surface = cdk_quartz_ref_cairo_surface;
  impl_class->show = cdk_window_quartz_show;
  impl_class->hide = cdk_window_quartz_hide;
  impl_class->withdraw = cdk_window_quartz_withdraw;
  impl_class->set_events = cdk_window_quartz_set_events;
  impl_class->get_events = cdk_window_quartz_get_events;
  impl_class->raise = cdk_window_quartz_raise;
  impl_class->lower = cdk_window_quartz_lower;
  impl_class->restack_toplevel = cdk_window_quartz_restack_toplevel;
  impl_class->move_resize = cdk_window_quartz_move_resize;
  impl_class->set_background = cdk_window_quartz_set_background;
  impl_class->reparent = cdk_window_quartz_reparent;
  impl_class->set_device_cursor = cdk_window_quartz_set_device_cursor;
  impl_class->get_geometry = cdk_window_quartz_get_geometry;
  impl_class->get_root_coords = cdk_window_quartz_get_root_coords;
  impl_class->get_device_state = cdk_window_quartz_get_device_state;
  impl_class->shape_combine_region = cdk_window_quartz_shape_combine_region;
  impl_class->input_shape_combine_region = cdk_window_quartz_input_shape_combine_region;
  impl_class->destroy = cdk_quartz_window_destroy;
  impl_class->destroy_foreign = cdk_quartz_window_destroy_foreign;
  impl_class->get_shape = cdk_quartz_window_get_shape;
  impl_class->get_input_shape = cdk_quartz_window_get_input_shape;
  impl_class->begin_paint = cdk_window_impl_quartz_begin_paint;
  impl_class->get_scale_factor = cdk_quartz_window_get_scale_factor;

  impl_class->focus = cdk_quartz_window_focus;
  impl_class->set_type_hint = cdk_quartz_window_set_type_hint;
  impl_class->get_type_hint = cdk_quartz_window_get_type_hint;
  impl_class->set_modal_hint = cdk_quartz_window_set_modal_hint;
  impl_class->set_skip_taskbar_hint = cdk_quartz_window_set_skip_taskbar_hint;
  impl_class->set_skip_pager_hint = cdk_quartz_window_set_skip_pager_hint;
  impl_class->set_urgency_hint = cdk_quartz_window_set_urgency_hint;
  impl_class->set_geometry_hints = cdk_quartz_window_set_geometry_hints;
  impl_class->set_title = cdk_quartz_window_set_title;
  impl_class->set_role = cdk_quartz_window_set_role;
  impl_class->set_startup_id = cdk_quartz_window_set_startup_id;
  impl_class->set_transient_for = cdk_quartz_window_set_transient_for;
  impl_class->get_frame_extents = cdk_quartz_window_get_frame_extents;
  impl_class->set_override_redirect = cdk_quartz_window_set_override_redirect;
  impl_class->set_accept_focus = cdk_quartz_window_set_accept_focus;
  impl_class->set_focus_on_map = cdk_quartz_window_set_focus_on_map;
  impl_class->set_icon_list = cdk_quartz_window_set_icon_list;
  impl_class->set_icon_name = cdk_quartz_window_set_icon_name;
  impl_class->iconify = cdk_quartz_window_iconify;
  impl_class->deiconify = cdk_quartz_window_deiconify;
  impl_class->stick = cdk_quartz_window_stick;
  impl_class->unstick = cdk_quartz_window_unstick;
  impl_class->maximize = cdk_quartz_window_maximize;
  impl_class->unmaximize = cdk_quartz_window_unmaximize;
  impl_class->fullscreen = cdk_quartz_window_fullscreen;
  impl_class->unfullscreen = cdk_quartz_window_unfullscreen;
  impl_class->set_keep_above = cdk_quartz_window_set_keep_above;
  impl_class->set_keep_below = cdk_quartz_window_set_keep_below;
  impl_class->get_group = cdk_quartz_window_get_group;
  impl_class->set_group = cdk_quartz_window_set_group;
  impl_class->set_decorations = cdk_quartz_window_set_decorations;
  impl_class->get_decorations = cdk_quartz_window_get_decorations;
  impl_class->set_functions = cdk_quartz_window_set_functions;
  impl_class->begin_resize_drag = cdk_quartz_window_begin_resize_drag;
  impl_class->begin_move_drag = cdk_quartz_window_begin_move_drag;
  impl_class->set_opacity = cdk_quartz_window_set_opacity;
  impl_class->set_shadow_width = cdk_quartz_window_set_shadow_width;
  impl_class->destroy_notify = cdk_quartz_window_destroy_notify;
  impl_class->register_dnd = _cdk_quartz_window_register_dnd;
  impl_class->drag_begin = _cdk_quartz_window_drag_begin;
  impl_class->process_updates_recurse = _cdk_quartz_window_process_updates_recurse;
  impl_class->sync_rendering = _cdk_quartz_window_sync_rendering;
  impl_class->simulate_key = _cdk_quartz_window_simulate_key;
  impl_class->simulate_button = _cdk_quartz_window_simulate_button;
  impl_class->get_property = _cdk_quartz_window_get_property;
  impl_class->change_property = _cdk_quartz_window_change_property;
  impl_class->delete_property = _cdk_quartz_window_delete_property;

  impl_class->create_gl_context = cdk_quartz_window_create_gl_context;
  impl_class->invalidate_for_new_frame = cdk_quartz_window_invalidate_for_new_frame;

  impl_quartz_class->get_context = cdk_window_impl_quartz_get_context;
  impl_quartz_class->release_context = cdk_window_impl_quartz_release_context;
}

GType
_cdk_window_impl_quartz_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      const GTypeInfo object_info =
	{
	  sizeof (CdkWindowImplQuartzClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) cdk_window_impl_quartz_class_init,
	  NULL,           /* class_finalize */
	  NULL,           /* class_data */
	  sizeof (CdkWindowImplQuartz),
	  0,              /* n_preallocs */
	  (GInstanceInitFunc) cdk_window_impl_quartz_init,
	};

      object_type = g_type_register_static (GDK_TYPE_WINDOW_IMPL,
                                            "CdkWindowImplQuartz",
                                            &object_info, 0);
    }

  return object_type;
}

CGContextRef
cdk_quartz_window_get_context (CdkWindowImplQuartz  *window,
                               gboolean             antialias)
{
  if (!GDK_WINDOW_IMPL_QUARTZ_GET_CLASS (window)->get_context)
    {
      g_warning ("%s doesn't implement CdkWindowImplQuartzClass::get_context()",
                 G_OBJECT_TYPE_NAME (window));
      return NULL;
    }

  return GDK_WINDOW_IMPL_QUARTZ_GET_CLASS (window)->get_context (window, antialias);
}

void
cdk_quartz_window_release_context (CdkWindowImplQuartz  *window,
                                   CGContextRef          cg_context)
{
  if (!GDK_WINDOW_IMPL_QUARTZ_GET_CLASS (window)->release_context)
    {
      g_warning ("%s doesn't implement CdkWindowImplQuartzClass::release_context()",
                 G_OBJECT_TYPE_NAME (window));
      return;
    }

  GDK_WINDOW_IMPL_QUARTZ_GET_CLASS (window)->release_context (window, cg_context);
}



static CGContextRef
cdk_root_window_impl_quartz_get_context (CdkWindowImplQuartz *window,
                                         gboolean             antialias)
{
  CGColorSpaceRef colorspace;
  CGContextRef cg_context;
  CdkWindowImplQuartz *window_impl = GDK_WINDOW_IMPL_QUARTZ (window);

  if (GDK_WINDOW_DESTROYED (window_impl->wrapper))
    return NULL;

  /* We do not have the notion of a root window on OS X.  We fake this
   * by creating a 1x1 bitmap and return a context to that.
   */
  colorspace = CGColorSpaceCreateWithName (kCGColorSpaceGenericRGB);
  cg_context = CGBitmapContextCreate (NULL,
                                      1, 1, 8, 4, colorspace,
                                      (CGBitmapInfo)kCGImageAlphaPremultipliedLast);
  CGColorSpaceRelease (colorspace);

  return cg_context;
}

static void
cdk_root_window_impl_quartz_release_context (CdkWindowImplQuartz *window,
                                             CGContextRef         cg_context)
{
  CGContextRelease (cg_context);
}

static void
cdk_root_window_impl_quartz_class_init (CdkRootWindowImplQuartzClass *klass)
{
  CdkWindowImplQuartzClass *window_quartz_class = GDK_WINDOW_IMPL_QUARTZ_CLASS (klass);

  root_window_parent_class = g_type_class_peek_parent (klass);

  window_quartz_class->get_context = cdk_root_window_impl_quartz_get_context;
  window_quartz_class->release_context = cdk_root_window_impl_quartz_release_context;
}

static void
cdk_root_window_impl_quartz_init (CdkRootWindowImplQuartz *impl)
{
}

GType
_cdk_root_window_impl_quartz_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      const GTypeInfo object_info =
        {
          sizeof (CdkRootWindowImplQuartzClass),
          (GBaseInitFunc) NULL,
          (GBaseFinalizeFunc) NULL,
          (GClassInitFunc) cdk_root_window_impl_quartz_class_init,
          NULL,           /* class_finalize */
          NULL,           /* class_data */
          sizeof (CdkRootWindowImplQuartz),
          0,              /* n_preallocs */
          (GInstanceInitFunc) cdk_root_window_impl_quartz_init,
        };

      object_type = g_type_register_static (GDK_TYPE_WINDOW_IMPL_QUARTZ,
                                            "CdkRootWindowQuartz",
                                            &object_info, 0);
    }

  return object_type;
}
