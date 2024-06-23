/* CDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/* needs to be first because any header might include gdk-pixbuf.h otherwise */
#define GDK_PIXBUF_ENABLE_BACKEND
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "cdkcursor.h"
#include "cdkcursorprivate.h"
#include "cdkprivate-x11.h"
#include "cdkdisplay-x11.h"

#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#ifdef HAVE_XCURSOR
#include <X11/Xcursor/Xcursor.h>
#endif
#ifdef HAVE_XFIXES
#include <X11/extensions/Xfixes.h>
#endif
#include <string.h>
#include <errno.h>
#include <math.h>

struct _CdkX11Cursor
{
  CdkCursor cursor;

  Cursor xcursor;
  gchar *name;
  guint serial;
};

struct _CdkX11CursorClass
{
  CdkCursorClass cursor_class;
};

static guint theme_serial = 0;

/* cursor_cache holds a cache of non-pixmap cursors to avoid expensive 
 * libXcursor searches, cursors are added to it but only removed when
 * their display is closed. We make the assumption that since there are 
 * a small number of display’s and a small number of cursor’s that this 
 * list will stay small enough not to be a problem.
 */
static GSList* cursor_cache = NULL;

struct cursor_cache_key
{
  CdkDisplay* display;
  CdkCursorType type;
  const char* name;
};

/* Caller should check if there is already a match first.
 * Cursor MUST be either a typed cursor or a pixmap with 
 * a non-NULL name.
 */
static void
add_to_cache (CdkX11Cursor* cursor)
{
  cursor_cache = g_slist_prepend (cursor_cache, cursor);

  /* Take a ref so that if the caller frees it we still have it */
  g_object_ref (cursor);
}

/* Returns 0 on a match
 */
static gint
cache_compare_func (gconstpointer listelem, 
                    gconstpointer target)
{
  CdkX11Cursor* cursor = (CdkX11Cursor*)listelem;
  struct cursor_cache_key* key = (struct cursor_cache_key*)target;

  if ((cursor->cursor.type != key->type) ||
      (cdk_cursor_get_display (CDK_CURSOR (cursor)) != key->display))
    return 1; /* No match */
  
  /* Elements marked as pixmap must be named cursors 
   * (since we don't store normal pixmap cursors 
   */
  if (key->type == CDK_CURSOR_IS_PIXMAP)
    return strcmp (key->name, cursor->name);

  return 0; /* Match */
}

/* Returns the cursor if there is a match, NULL if not
 * For named cursors type shall be CDK_CURSOR_IS_PIXMAP
 * For unnamed, typed cursors, name shall be NULL
 */
static CdkX11Cursor*
find_in_cache (CdkDisplay    *display, 
               CdkCursorType  type,
               const char    *name)
{
  GSList* res;
  struct cursor_cache_key key;

  key.display = display;
  key.type = type;
  key.name = name;

  res = g_slist_find_custom (cursor_cache, &key, cache_compare_func);

  if (res)
    return (CdkX11Cursor *) res->data;

  return NULL;
}

/* Called by cdk_x11_display_finalize to flush any cached cursors
 * for a dead display.
 */
void
_cdk_x11_cursor_display_finalize (CdkDisplay *display)
{
  GSList* item;
  GSList** itemp; /* Pointer to the thing to fix when we delete an item */
  item = cursor_cache;
  itemp = &cursor_cache;
  while (item)
    {
      CdkX11Cursor* cursor = (CdkX11Cursor*)(item->data);
      if (cdk_cursor_get_display (CDK_CURSOR (cursor)) == display)
        {
          GSList* olditem;
          g_object_unref ((CdkCursor*) cursor);
          /* Remove this item from the list */
          *(itemp) = item->next;
          olditem = item;
          item = item->next;
          g_slist_free_1 (olditem);
        }
      else
        {
          itemp = &(item->next);
          item = item->next;
        }
    }
}

/*** CdkX11Cursor ***/

G_DEFINE_TYPE (CdkX11Cursor, cdk_x11_cursor, CDK_TYPE_CURSOR)

static cairo_surface_t *cdk_x11_cursor_get_surface (CdkCursor *cursor,
						    gdouble   *x_hot,
						    gdouble   *y_hot);

static void
cdk_x11_cursor_finalize (GObject *object)
{
  CdkX11Cursor *private = CDK_X11_CURSOR (object);
  CdkDisplay *display;

  display = cdk_cursor_get_display (CDK_CURSOR (object));
  if (private->xcursor && !cdk_display_is_closed (display))
    XFreeCursor (CDK_DISPLAY_XDISPLAY (display), private->xcursor);

  g_free (private->name);

  G_OBJECT_CLASS (cdk_x11_cursor_parent_class)->finalize (object);
}

static void
cdk_x11_cursor_class_init (CdkX11CursorClass *xcursor_class)
{
  CdkCursorClass *cursor_class = CDK_CURSOR_CLASS (xcursor_class);
  GObjectClass *object_class = G_OBJECT_CLASS (xcursor_class);

  object_class->finalize = cdk_x11_cursor_finalize;

  cursor_class->get_surface = cdk_x11_cursor_get_surface;
}

static void
cdk_x11_cursor_init (CdkX11Cursor *cursor G_GNUC_UNUSED)
{
}

static Cursor
get_blank_cursor (CdkDisplay *display)
{
  CdkScreen *screen;
  Pixmap pixmap;
  XColor color;
  Cursor cursor;
  cairo_surface_t *surface;
  cairo_t *cr;

  screen = cdk_display_get_default_screen (display);
  surface = _cdk_x11_window_create_bitmap_surface (cdk_screen_get_root_window (screen), 1, 1);
  /* Clear surface */
  cr = cairo_create (surface);
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);
  cairo_destroy (cr);
 
  pixmap = cairo_xlib_surface_get_drawable (surface);

  color.pixel = 0; 
  color.red = color.blue = color.green = 0;

  if (cdk_display_is_closed (display))
    cursor = None;
  else
    cursor = XCreatePixmapCursor (CDK_DISPLAY_XDISPLAY (display),
                                  pixmap, pixmap,
                                  &color, &color, 1, 1);
  cairo_surface_destroy (surface);

  return cursor;
}

CdkCursor*
_cdk_x11_display_get_cursor_for_type (CdkDisplay    *display,
                                      CdkCursorType  cursor_type)
{
  CdkX11Cursor *private;
  Cursor xcursor;

  if (cdk_display_is_closed (display))
    {
      xcursor = None;
    }
  else
    {
      private = find_in_cache (display, cursor_type, NULL);

      if (private)
        {
          /* Cache had it, add a ref for this user */
          g_object_ref (private);

          return (CdkCursor*) private;
        }
      else
        {
          if (cursor_type != CDK_BLANK_CURSOR)
            xcursor = XCreateFontCursor (CDK_DISPLAY_XDISPLAY (display),
                                         cursor_type);
          else
            xcursor = get_blank_cursor (display);
       }
    }

  private = g_object_new (CDK_TYPE_X11_CURSOR,
                          "cursor-type", cursor_type,
                          "display", display,
                          NULL);
  private->xcursor = xcursor;
  private->name = NULL;
  private->serial = theme_serial;

  if (xcursor != None)
    add_to_cache (private);

  return CDK_CURSOR (private);
}

/**
 * cdk_x11_cursor_get_xdisplay:
 * @cursor: (type CdkX11Cursor): a #CdkCursor.
 * 
 * Returns the display of a #CdkCursor.
 * 
 * Returns: (transfer none): an Xlib Display*.
 **/
Display *
cdk_x11_cursor_get_xdisplay (CdkCursor *cursor)
{
  g_return_val_if_fail (cursor != NULL, NULL);

  return CDK_DISPLAY_XDISPLAY (cdk_cursor_get_display (cursor));
}

/**
 * cdk_x11_cursor_get_xcursor:
 * @cursor: (type CdkX11Cursor): a #CdkCursor.
 * 
 * Returns the X cursor belonging to a #CdkCursor.
 * 
 * Returns: an Xlib Cursor.
 **/
Cursor
cdk_x11_cursor_get_xcursor (CdkCursor *cursor)
{
  g_return_val_if_fail (cursor != NULL, None);

  return ((CdkX11Cursor *)cursor)->xcursor;
}

#if defined(HAVE_XCURSOR) && defined(HAVE_XFIXES) && XFIXES_MAJOR >= 2

static cairo_surface_t *
cdk_x11_cursor_get_surface (CdkCursor *cursor,
			    gdouble   *x_hot,
			    gdouble   *y_hot)
{
  CdkDisplay *display;
  Display *xdisplay;
  CdkX11Cursor *private;
  XcursorImages *images = NULL;
  XcursorImage *image;
  gint size;
  cairo_surface_t *surface;
  gint scale;
  gchar *theme;
  
  private = CDK_X11_CURSOR (cursor);

  display = cdk_cursor_get_display (cursor);
  xdisplay = CDK_DISPLAY_XDISPLAY (display);

  size = XcursorGetDefaultSize (xdisplay);
  theme = XcursorGetTheme (xdisplay);

  if (cursor->type == CDK_CURSOR_IS_PIXMAP)
    {
      if (private->name)
        images = XcursorLibraryLoadImages (private->name, theme, size);
    }
  else
    images = XcursorShapeLoadImages (cursor->type, theme, size);

  if (!images)
    return NULL;

  image = images->images[0];

  /* Assume the currently set cursor was defined for the screen
     scale */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  scale =
    cdk_screen_get_monitor_scale_factor (cdk_display_get_default_screen (display), 0);
G_GNUC_END_IGNORE_DEPRECATIONS

  surface = cdk_window_create_similar_image_surface (NULL,
						     CAIRO_FORMAT_ARGB32,
						     image->width,
						     image->height,
						     scale);

  memcpy (cairo_image_surface_get_data (surface),
	  image->pixels, 4 * image->width * image->height);

  cairo_surface_mark_dirty (surface);

  if (x_hot)
    *x_hot = (double)image->xhot / scale;
  if (y_hot)
    *y_hot = (double)image->yhot / scale;

  XcursorImagesDestroy (images);

  return surface;
}

void
_cdk_x11_cursor_update_theme (CdkCursor *cursor)
{
  Display *xdisplay;
  CdkX11Cursor *private;
  Cursor new_cursor = None;
  CdkX11Display *display_x11;

  private = (CdkX11Cursor *) cursor;
  display_x11 = CDK_X11_DISPLAY (cdk_cursor_get_display (cursor));
  xdisplay = CDK_DISPLAY_XDISPLAY (display_x11);

  if (!display_x11->have_xfixes)
    return;

  if (private->serial == theme_serial)
    return;

  private->serial = theme_serial;

  if (private->xcursor != None)
    {
      if (cursor->type == CDK_BLANK_CURSOR)
        return;

      if (cursor->type == CDK_CURSOR_IS_PIXMAP)
        {
          if (private->name)
            new_cursor = XcursorLibraryLoadCursor (xdisplay, private->name);
        }
      else 
        new_cursor = XcursorShapeLoadCursor (xdisplay, cursor->type);
      
      if (new_cursor != None)
        {
          XFixesChangeCursor (xdisplay, new_cursor, private->xcursor);
          private->xcursor = new_cursor;
        }
    }
}

static void
update_cursor (gpointer data,
               gpointer user_data G_GNUC_UNUSED)
{
  CdkCursor *cursor;

  cursor = (CdkCursor*)(data);

  if (!cursor)
    return;

  _cdk_x11_cursor_update_theme (cursor);
}

/**
 * cdk_x11_display_set_cursor_theme:
 * @display: (type CdkX11Display): a #CdkDisplay
 * @theme: (nullable): the name of the cursor theme to use, or %NULL to unset
 *         a previously set value
 * @size: the cursor size to use, or 0 to keep the previous size
 *
 * Sets the cursor theme from which the images for cursor
 * should be taken.
 *
 * If the windowing system supports it, existing cursors created
 * with cdk_cursor_new(), cdk_cursor_new_for_display() and
 * cdk_cursor_new_from_name() are updated to reflect the theme
 * change. Custom cursors constructed with
 * cdk_cursor_new_from_pixbuf() will have to be handled
 * by the application (CTK+ applications can learn about
 * cursor theme changes by listening for change notification
 * for the corresponding #CtkSetting).
 *
 * Since: 2.8
 */
void
cdk_x11_display_set_cursor_theme (CdkDisplay  *display,
                                  const gchar *theme,
                                  const gint   size)
{
  Display *xdisplay;
  gchar *old_theme;
  gint old_size;

  g_return_if_fail (CDK_IS_DISPLAY (display));

  xdisplay = CDK_DISPLAY_XDISPLAY (display);

  old_theme = XcursorGetTheme (xdisplay);
  old_size = XcursorGetDefaultSize (xdisplay);

  if (old_size == size &&
      (old_theme == theme ||
       (old_theme && theme && strcmp (old_theme, theme) == 0)))
    return;

  theme_serial++;

  XcursorSetTheme (xdisplay, theme);
  if (size > 0)
    XcursorSetDefaultSize (xdisplay, size);

  g_slist_foreach (cursor_cache, update_cursor, NULL);
}

#else

static cairo_surface_t *
cdk_x11_cursor_get_surface (CdkCursor *cursor,
			    gdouble *x_hot,
			    gdouble *y_hot)
{
  return NULL;
}

void
cdk_x11_display_set_cursor_theme (CdkDisplay  *display,
                                  const gchar *theme,
                                  const gint   size)
{
  g_return_if_fail (CDK_IS_DISPLAY (display));
}

void
_cdk_x11_cursor_update_theme (CdkCursor *cursor)
{
  g_return_if_fail (cursor != NULL);
}

#endif

#ifdef HAVE_XCURSOR

static void
get_surface_size (cairo_surface_t *surface,
		  int *width,
		  int *height)
{
  double x_scale, y_scale;

  x_scale = y_scale = 1;

  cairo_surface_get_device_scale (surface, &x_scale, &y_scale);

  /* Assume any set scaling is icon scale */
  *width =
    ceil (cairo_image_surface_get_width (surface) / x_scale);
  *height =
    ceil (cairo_image_surface_get_height (surface) / y_scale);
}

static XcursorImage*
create_cursor_image (cairo_surface_t *source_surface,
                     gint       x,
                     gint       y,
		     gint       scale)
{
  gint width, height;
  XcursorImage *xcimage;
  cairo_surface_t *surface;
  cairo_t *cr;

  get_surface_size (source_surface, &width, &height);

  width *= scale;
  height *= scale;
  
  xcimage = XcursorImageCreate (width, height);

  xcimage->xhot = x * scale;
  xcimage->yhot = y * scale;

  surface = cairo_image_surface_create_for_data ((guchar *) xcimage->pixels,
                                                 CAIRO_FORMAT_ARGB32,
                                                 width,
                                                 height,
                                                 width * 4);

  cairo_surface_set_device_scale (surface, scale, scale);

  cr = cairo_create (surface);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_surface (cr, source_surface, 0, 0);
  cairo_paint (cr);
  cairo_destroy (cr);
  cairo_surface_destroy (surface);

  return xcimage;
}

CdkCursor *
_cdk_x11_display_get_cursor_for_surface (CdkDisplay *display,
					 cairo_surface_t *surface,
					 gdouble     x,
					 gdouble     y)
{
  XcursorImage *xcimage;
  Cursor xcursor;
  CdkX11Cursor *private;
  int target_scale;

  if (cdk_display_is_closed (display))
    {
      xcursor = None;
    }
  else
    {
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      target_scale =
	cdk_screen_get_monitor_scale_factor (cdk_display_get_default_screen (display), 0);
G_GNUC_END_IGNORE_DEPRECATIONS

      xcimage = create_cursor_image (surface, x, y, target_scale);
      xcursor = XcursorImageLoadCursor (CDK_DISPLAY_XDISPLAY (display), xcimage);
      XcursorImageDestroy (xcimage);
    }

  private = g_object_new (CDK_TYPE_X11_CURSOR, 
                          "cursor-type", CDK_CURSOR_IS_PIXMAP,
                          "display", display,
                          NULL);
  private->xcursor = xcursor;
  private->name = NULL;
  private->serial = theme_serial;

  return CDK_CURSOR (private);
}

static const struct {
  const gchar *css_name, *traditional_name;
} name_map[] = {
  { "default",      "left_ptr" },
  { "help",         "left_ptr" },
  { "context-menu", "left_ptr" },
  { "pointer",      "hand" },
  { "progress",     "left_ptr_watch" },
  { "wait",         "watch" },
  { "cell",         "crosshair" },
  { "crosshair",    "cross" },
  { "text",         "xterm" },
  { "vertical-text","xterm" },
  { "alias",        "dnd-link" },
  { "copy",         "dnd-copy" },
  { "move",         "dnd-move" },
  { "no-drop",      "dnd-none" },
  { "dnd-ask",      "dnd-copy" }, /* not CSS, but we want to guarantee it anyway */
  { "not-allowed",  "crossed_circle" },
  { "grab",         "hand2" },
  { "grabbing",     "hand2" },
  { "all-scroll",   "left_ptr" },
  { "col-resize",   "h_double_arrow" },
  { "row-resize",   "v_double_arrow" },
  { "n-resize",     "top_side" },
  { "e-resize",     "right_side" },
  { "s-resize",     "bottom_side" },
  { "w-resize",     "left_side" },
  { "ne-resize",    "top_right_corner" },
  { "nw-resize",    "top_left_corner" },
  { "se-resize",    "bottom_right_corner" },
  { "sw-resize",    "bottom_left_corner" },
  { "ew-resize",    "h_double_arrow" },
  { "ns-resize",    "v_double_arrow" },
  { "nesw-resize",  "fd_double_arrow" },
  { "nwse-resize",  "bd_double_arrow" },
  { "zoom-in",      "left_ptr" },
  { "zoom-out",     "left_ptr" },
  { NULL, NULL }
};

static const gchar *
name_fallback (const gchar *name)
{
  gint i;

  for (i = 0; name_map[i].css_name; i++)
    {
      if (g_str_equal (name_map[i].css_name, name))
        return name_map[i].traditional_name;
    }

  return NULL;
}

CdkCursor*
_cdk_x11_display_get_cursor_for_name (CdkDisplay  *display,
                                      const gchar *name)
{
  Cursor xcursor;
  Display *xdisplay;
  CdkX11Cursor *private;

  if (cdk_display_is_closed (display))
    {
      xcursor = None;
    }
  else
    {
      if (strcmp (name, "none") == 0)
        return _cdk_x11_display_get_cursor_for_type (display, CDK_BLANK_CURSOR);

      private = find_in_cache (display, CDK_CURSOR_IS_PIXMAP, name);

      if (private)
        {
          /* Cache had it, add a ref for this user */
          g_object_ref (private);

          return (CdkCursor*) private;
        }

      xdisplay = CDK_DISPLAY_XDISPLAY (display);
      xcursor = XcursorLibraryLoadCursor (xdisplay, name);
      if (xcursor == None)
        {
          const char *fallback;

          fallback = name_fallback (name);
          if (fallback)
            {
              xcursor = XcursorLibraryLoadCursor (xdisplay, fallback);
              if (xcursor == None)
                xcursor = XcursorLibraryLoadCursor (xdisplay, "left_ptr");
            }
        }
      if (xcursor == None)
        return NULL;
    }

  private = g_object_new (CDK_TYPE_X11_CURSOR,
                          "cursor-type", CDK_CURSOR_IS_PIXMAP,
                          "display", display,
                          NULL);
  private->xcursor = xcursor;
  private->name = g_strdup (name);
  private->serial = theme_serial;

  add_to_cache (private);

  return CDK_CURSOR (private);
}

gboolean
_cdk_x11_display_supports_cursor_alpha (CdkDisplay *display)
{
  return XcursorSupportsARGB (CDK_DISPLAY_XDISPLAY (display));
}

gboolean
_cdk_x11_display_supports_cursor_color (CdkDisplay *display)
{
  return XcursorSupportsARGB (CDK_DISPLAY_XDISPLAY (display));
}

void
_cdk_x11_display_get_default_cursor_size (CdkDisplay *display,
                                          guint      *width,
                                          guint      *height)
{
  *width = *height = XcursorGetDefaultSize (CDK_DISPLAY_XDISPLAY (display));
}

#else

static CdkCursor*
cdk_cursor_new_from_pixmap (CdkDisplay     *display,
                            Pixmap          source_pixmap,
                            Pixmap          mask_pixmap,
                            const CdkRGBA  *fg,
                            const CdkRGBA  *bg,
                            gint            x,
                            gint            y)
{
  CdkX11Cursor *private;
  Cursor xcursor;
  XColor xfg, xbg;

  g_return_val_if_fail (fg != NULL, NULL);
  g_return_val_if_fail (bg != NULL, NULL);

  xfg.red = fg->red * 65535;
  xfg.blue = fg->blue * 65535;
  xfg.green = fg->green * 65535;

  xbg.red = bg->red * 65535;
  xbg.blue = bg->blue * 65535;
  xbg.green = bg->green * 65535;

  if (cdk_display_is_closed (display))
    xcursor = None;
  else
    xcursor = XCreatePixmapCursor (CDK_DISPLAY_XDISPLAY (display),
                                   source_pixmap, mask_pixmap, &xfg, &xbg, x, y);
  private = g_object_new (CDK_TYPE_X11_CURSOR,
                          "cursor-type", CDK_CURSOR_IS_PIXMAP,
                          "display", display,
                          NULL);
  private->xcursor = xcursor;
  private->name = NULL;
  private->serial = theme_serial;

  return CDK_CURSOR (private);
}

CdkCursor *
_cdk_x11_display_get_cursor_for_surface (CdkDisplay *display,
					 cairo_surface_t *surface,
					 gdouble     x,
					 gdouble     y)
{
  CdkCursor *cursor;
  cairo_surface_t *pixmap, *mask;
  guint width, height, n_channels, rowstride, data_stride, i, j;
  guint8 *data, *mask_data, *pixels;
  CdkRGBA fg = { 0, 0, 0, 1 };
  CdkRGBA bg = { 1, 1, 1, 1 };
  CdkScreen *screen;
  cairo_surface_t *image;
  cairo_t *cr;
  GdkPixbuf *pixbuf;

  width = cairo_image_surface_get_width (surface);
  height = cairo_image_surface_get_height (surface);

  g_return_val_if_fail (0 <= x && x < width, NULL);
  g_return_val_if_fail (0 <= y && y < height, NULL);

  /* Note: This does not support scaled surfaced, if you need that you
     want XCursor anyway */
  pixbuf = gdk_pixbuf_get_from_surface (surface, 0, 0, width, height);
  
  n_channels = gdk_pixbuf_get_n_channels (pixbuf);
  rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  pixels = gdk_pixbuf_get_pixels (pixbuf);

  data_stride = 4 * ((width + 31) / 32);
  data = g_new0 (guint8, data_stride * height);
  mask_data = g_new0 (guint8, data_stride * height);

  for (j = 0; j < height; j++)
    {
      guint8 *src = pixels + j * rowstride;
      guint8 *d = data + data_stride * j;
      guint8 *md = mask_data + data_stride * j;

      for (i = 0; i < width; i++)
        {
          if (src[1] < 0x80)
            *d |= 1 << (i % 8);

          if (n_channels == 3 || src[3] >= 0x80)
            *md |= 1 << (i % 8);

          src += n_channels;
          if (i % 8 == 7)
            {
              d++;
              md++;
            }
        }
    }

  g_object_unref (pixbuf);

  screen = cdk_display_get_default_screen (display);

  pixmap = _cdk_x11_window_create_bitmap_surface (cdk_screen_get_root_window (screen),
                                                  width, height);
  cr = cairo_create (pixmap);
  image = cairo_image_surface_create_for_data (data, CAIRO_FORMAT_A1,
                                               width, height, data_stride);
  cairo_set_source_surface (cr, image, 0, 0);
  cairo_surface_destroy (image);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_paint (cr);
  cairo_destroy (cr);

  mask = _cdk_x11_window_create_bitmap_surface (cdk_screen_get_root_window (screen),
                                                width, height);
  cr = cairo_create (mask);
  image = cairo_image_surface_create_for_data (mask_data, CAIRO_FORMAT_A1,
                                               width, height, data_stride);
  cairo_set_source_surface (cr, image, 0, 0);
  cairo_surface_destroy (image);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_paint (cr);
  cairo_destroy (cr);

  cursor = cdk_cursor_new_from_pixmap (display,
                                       cairo_xlib_surface_get_drawable (pixmap),
                                       cairo_xlib_surface_get_drawable (mask),
                                       &fg, &bg,
                                       x, y);

  cairo_surface_destroy (pixmap);
  cairo_surface_destroy (mask);
  
  g_free (data);
  g_free (mask_data);

  return cursor;
}

CdkCursor*
_cdk_x11_display_get_cursor_for_name (CdkDisplay  *display,
                                      const gchar *name)
{
  return NULL;
}

gboolean
_cdk_x11_display_supports_cursor_alpha (CdkDisplay *display)
{
  return FALSE;
}

gboolean
_cdk_x11_display_supports_cursor_color (CdkDisplay *display)
{
  return FALSE;
}

void
_cdk_x11_display_get_default_cursor_size (CdkDisplay *display,
                                          guint      *width,
                                          guint      *height)
{
  /* no idea, really */
  *width = *height = 20;
  return;
}

#endif

void
_cdk_x11_display_get_maximal_cursor_size (CdkDisplay *display,
                                          guint       *width,
                                          guint       *height)
{
  CdkScreen *screen;
  CdkWindow *window;

  g_return_if_fail (CDK_IS_DISPLAY (display));

  screen = cdk_display_get_default_screen (display);
  window = cdk_screen_get_root_window (screen);
  XQueryBestCursor (CDK_DISPLAY_XDISPLAY (display),
                    CDK_WINDOW_XID (window),
                    128, 128, width, height);
}
