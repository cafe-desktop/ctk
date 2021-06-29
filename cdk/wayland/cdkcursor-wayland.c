/* GDK - The GIMP Drawing Kit
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

#define GDK_PIXBUF_ENABLE_BACKEND

#include <string.h>

#include "cdkprivate-wayland.h"
#include "cdkcursorprivate.h"
#include "cdkdisplay-wayland.h"
#include "cdkwayland.h"
#include <cdk-pixbuf/cdk-pixbuf.h>

#include <wayland-cursor.h>

#define GDK_TYPE_WAYLAND_CURSOR              (_cdk_wayland_cursor_get_type ())
#define GDK_WAYLAND_CURSOR(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_WAYLAND_CURSOR, CdkWaylandCursor))
#define GDK_WAYLAND_CURSOR_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_WAYLAND_CURSOR, CdkWaylandCursorClass))
#define GDK_IS_WAYLAND_CURSOR(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_WAYLAND_CURSOR))
#define GDK_IS_WAYLAND_CURSOR_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_WAYLAND_CURSOR))
#define GDK_WAYLAND_CURSOR_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_WAYLAND_CURSOR, CdkWaylandCursorClass))

typedef struct _CdkWaylandCursor CdkWaylandCursor;
typedef struct _CdkWaylandCursorClass CdkWaylandCursorClass;

struct _CdkWaylandCursor
{
  CdkCursor cursor;
  gchar *name;

  struct
  {
    int hotspot_x, hotspot_y;
    int width, height, scale;
    cairo_surface_t *cairo_surface;
  } surface;

  struct wl_cursor *wl_cursor;
  int scale;
};

struct _CdkWaylandCursorClass
{
  CdkCursorClass cursor_class;
};

GType _cdk_wayland_cursor_get_type (void);

G_DEFINE_TYPE (CdkWaylandCursor, _cdk_wayland_cursor, GDK_TYPE_CURSOR)

void
_cdk_wayland_display_init_cursors (CdkWaylandDisplay *display)
{
  display->cursor_cache = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, g_object_unref);
}

void
_cdk_wayland_display_finalize_cursors (CdkWaylandDisplay *display)
{
  g_hash_table_destroy (display->cursor_cache);
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

static gboolean
_cdk_wayland_cursor_update (CdkWaylandDisplay *display_wayland,
                            CdkWaylandCursor  *cursor)
{
  struct wl_cursor *c;
  struct wl_cursor_theme *theme;

  /* Do nothing if this is not a wl_cursor cursor. */
  if (cursor->name == NULL)
    return FALSE;

  theme = _cdk_wayland_display_get_scaled_cursor_theme (display_wayland,
                                                        cursor->scale);
  c = wl_cursor_theme_get_cursor (theme, cursor->name);
  if (!c)
    {
      const char *fallback;

      fallback = name_fallback (cursor->name);
      if (fallback)
        {
          c = wl_cursor_theme_get_cursor (theme, name_fallback (cursor->name));
          if (!c)
            c = wl_cursor_theme_get_cursor (theme, "left_ptr");
        }
    }

  if (!c)
    {
      g_message ("Unable to load %s from the cursor theme", cursor->name);
      return FALSE;
    }

  cursor->wl_cursor = c;

  return TRUE;
}

void
_cdk_wayland_display_update_cursors (CdkWaylandDisplay *display)
{
  GHashTableIter iter;
  const char *name;
  CdkWaylandCursor *cursor;

  g_hash_table_iter_init (&iter, display->cursor_cache);

  while (g_hash_table_iter_next (&iter, (gpointer *) &name, (gpointer *) &cursor))
    _cdk_wayland_cursor_update (display, cursor);
}

static void
cdk_wayland_cursor_finalize (GObject *object)
{
  CdkWaylandCursor *cursor = GDK_WAYLAND_CURSOR (object);

  g_free (cursor->name);
  if (cursor->surface.cairo_surface)
    cairo_surface_destroy (cursor->surface.cairo_surface);

  G_OBJECT_CLASS (_cdk_wayland_cursor_parent_class)->finalize (object);
}

static cairo_surface_t *
cdk_wayland_cursor_get_surface (CdkCursor *cursor,
				gdouble *x_hot,
				gdouble *y_hot)
{
  return NULL;
}

struct wl_buffer *
_cdk_wayland_cursor_get_buffer (CdkCursor *cursor,
                                guint      image_index,
                                int       *hotspot_x,
                                int       *hotspot_y,
                                int       *w,
                                int       *h,
                                int       *scale)
{
  CdkWaylandCursor *wayland_cursor = GDK_WAYLAND_CURSOR (cursor);

  if (wayland_cursor->wl_cursor)
    {
      struct wl_cursor_image *image;

      if (image_index >= wayland_cursor->wl_cursor->image_count)
        {
          g_warning (G_STRLOC " out of bounds cursor image [%d / %d]",
                     image_index,
                     wayland_cursor->wl_cursor->image_count - 1);
          image_index = 0;
        }

      image = wayland_cursor->wl_cursor->images[image_index];

      *hotspot_x = image->hotspot_x / wayland_cursor->scale;
      *hotspot_y = image->hotspot_y / wayland_cursor->scale;

      *w = image->width / wayland_cursor->scale;
      *h = image->height / wayland_cursor->scale;
      *scale = wayland_cursor->scale;

      return wl_cursor_image_get_buffer (image);
    }
  else if (wayland_cursor->name == NULL) /* From surface */
    {
      *hotspot_x =
        wayland_cursor->surface.hotspot_x / wayland_cursor->surface.scale;
      *hotspot_y =
        wayland_cursor->surface.hotspot_y / wayland_cursor->surface.scale;

      *w = wayland_cursor->surface.width / wayland_cursor->surface.scale;
      *h = wayland_cursor->surface.height / wayland_cursor->surface.scale;
      *scale = wayland_cursor->surface.scale;

      cairo_surface_reference (wayland_cursor->surface.cairo_surface);

      if (wayland_cursor->surface.cairo_surface)
        return _cdk_wayland_shm_surface_get_wl_buffer (wayland_cursor->surface.cairo_surface);
    }
  else
    {
      *hotspot_x = 0;
      *hotspot_y = 0;
      *w = 0;
      *h = 0;
      *scale = 1;
    }

  return NULL;
}

guint
_cdk_wayland_cursor_get_next_image_index (CdkCursor *cursor,
                                          guint      current_image_index,
                                          guint     *next_image_delay)
{
  struct wl_cursor *wl_cursor = GDK_WAYLAND_CURSOR (cursor)->wl_cursor;

  if (wl_cursor && wl_cursor->image_count > 1)
    {
      if (current_image_index >= wl_cursor->image_count)
        {
          g_warning (G_STRLOC " out of bounds cursor image [%d / %d]",
                     current_image_index, wl_cursor->image_count - 1);
          current_image_index = 0;
        }

      /* Return the time to next image */
      if (next_image_delay)
        *next_image_delay = wl_cursor->images[current_image_index]->delay;

      return (current_image_index + 1) % wl_cursor->image_count;
    }
  else
    return current_image_index;
}

void
_cdk_wayland_cursor_set_scale (CdkCursor *cursor,
                               guint      scale)
{
  CdkWaylandDisplay *display_wayland =
    GDK_WAYLAND_DISPLAY (cdk_cursor_get_display (cursor));
  CdkWaylandCursor *wayland_cursor = GDK_WAYLAND_CURSOR (cursor);

  if (scale > GDK_WAYLAND_MAX_THEME_SCALE)
    {
      g_warning (G_STRLOC ": cursor theme size %u too large", scale);
      scale = GDK_WAYLAND_MAX_THEME_SCALE;
    }

  if (wayland_cursor->scale == scale)
    return;

  wayland_cursor->scale = scale;

  /* Blank cursor case */
  if (g_strcmp0 (wayland_cursor->name, "none") == 0)
    return;

  _cdk_wayland_cursor_update (display_wayland, wayland_cursor);
}

static void
_cdk_wayland_cursor_class_init (CdkWaylandCursorClass *wayland_cursor_class)
{
  CdkCursorClass *cursor_class = GDK_CURSOR_CLASS (wayland_cursor_class);
  GObjectClass *object_class = G_OBJECT_CLASS (wayland_cursor_class);

  object_class->finalize = cdk_wayland_cursor_finalize;

  cursor_class->get_surface = cdk_wayland_cursor_get_surface;
}

static void
_cdk_wayland_cursor_init (CdkWaylandCursor *cursor)
{
}

static CdkCursor *
_cdk_wayland_display_get_cursor_for_name_with_scale (CdkDisplay  *display,
                                                     const gchar *name,
                                                     guint        scale)
{
  CdkWaylandCursor *wayland_cursor;
  CdkWaylandDisplay *display_wayland = GDK_WAYLAND_DISPLAY (display);

  g_return_val_if_fail (GDK_IS_DISPLAY (display), NULL);

  wayland_cursor = g_hash_table_lookup (display_wayland->cursor_cache, name);
  if (wayland_cursor && wayland_cursor->scale == scale)
    return GDK_CURSOR (g_object_ref (wayland_cursor));

  wayland_cursor = g_object_new (GDK_TYPE_WAYLAND_CURSOR,
                                 "cursor-type", GDK_CURSOR_IS_PIXMAP,
                                 "display", display,
                                 NULL);

  /* Blank cursor case */
  if (!name || g_str_equal (name, "none") || g_str_equal (name, "blank_cursor"))
    {
      wayland_cursor->name = g_strdup ("none");
      wayland_cursor->scale = scale;

      return GDK_CURSOR (wayland_cursor);
    }

  wayland_cursor->name = g_strdup (name);
  wayland_cursor->scale = scale;

  if (!_cdk_wayland_cursor_update (display_wayland, wayland_cursor))
    {
      g_object_unref (wayland_cursor);
      return NULL;
    }

  /* Insert into cache. */
  g_hash_table_replace (display_wayland->cursor_cache,
                        wayland_cursor->name,
                        g_object_ref (wayland_cursor));
  return GDK_CURSOR (wayland_cursor);
}

CdkCursor *
_cdk_wayland_display_get_cursor_for_name (CdkDisplay  *display,
                                          const gchar *name)
{
  return _cdk_wayland_display_get_cursor_for_name_with_scale (display, name, 1);
}

CdkCursor *
_cdk_wayland_display_get_cursor_for_type_with_scale (CdkDisplay    *display,
                                                     CdkCursorType  cursor_type,
                                                     guint          scale)
{
  GEnumClass *enum_class;
  GEnumValue *enum_value;
  gchar *cursor_name;
  CdkCursor *result;

  enum_class = g_type_class_ref (GDK_TYPE_CURSOR_TYPE);
  enum_value = g_enum_get_value (enum_class, cursor_type);
  cursor_name = g_strdup (enum_value->value_nick);
  g_strdelimit (cursor_name, "-", '_');
  g_type_class_unref (enum_class);

  result = _cdk_wayland_display_get_cursor_for_name_with_scale (display,
                                                                cursor_name,
                                                                scale);

  g_free (cursor_name);

  return result;
}

CdkCursor *
_cdk_wayland_display_get_cursor_for_type (CdkDisplay    *display,
                                          CdkCursorType  cursor_type)
{
  return _cdk_wayland_display_get_cursor_for_type_with_scale (display,
                                                              cursor_type,
                                                              1);
}

static void
buffer_release_callback (void             *_data,
                         struct wl_buffer *wl_buffer)
{
  cairo_surface_t *cairo_surface = _data;

  cairo_surface_destroy (cairo_surface);
}

static const struct wl_buffer_listener buffer_listener = {
  buffer_release_callback
};

CdkCursor *
_cdk_wayland_display_get_cursor_for_surface (CdkDisplay *display,
					     cairo_surface_t *surface,
					     gdouble     x,
					     gdouble     y)
{
  CdkWaylandCursor *cursor;
  CdkWaylandDisplay *display_wayland = GDK_WAYLAND_DISPLAY (display);
  struct wl_buffer *buffer;
  cairo_t *cr;

  cursor = g_object_new (GDK_TYPE_WAYLAND_CURSOR,
			 "cursor-type", GDK_CURSOR_IS_PIXMAP,
			 "display", display_wayland,
			 NULL);
  cursor->name = NULL;
  cursor->surface.hotspot_x = x;
  cursor->surface.hotspot_y = y;

  cursor->surface.scale = 1;

  if (surface)
    {
      double sx, sy;
      cairo_surface_get_device_scale (surface, &sx, &sy);
      cursor->surface.scale = (int)sx;
      cursor->surface.width = cairo_image_surface_get_width (surface);
      cursor->surface.height = cairo_image_surface_get_height (surface);
    }
  else
    {
      cursor->surface.width = 1;
      cursor->surface.height = 1;
    }

  cursor->surface.cairo_surface =
    _cdk_wayland_display_create_shm_surface (display_wayland,
                                             cursor->surface.width,
                                             cursor->surface.height,
                                             cursor->surface.scale);

  buffer = _cdk_wayland_shm_surface_get_wl_buffer (cursor->surface.cairo_surface);
  wl_buffer_add_listener (buffer, &buffer_listener, cursor->surface.cairo_surface);

  if (surface)
    {
      cr = cairo_create (cursor->surface.cairo_surface);
      cairo_set_source_surface (cr, surface, 0, 0);
      cairo_paint (cr);
      cairo_destroy (cr);
    }

  return GDK_CURSOR (cursor);
}

void
_cdk_wayland_display_get_default_cursor_size (CdkDisplay *display,
					      guint       *width,
					      guint       *height)
{
  *width = 32;
  *height = 32;
}

void
_cdk_wayland_display_get_maximal_cursor_size (CdkDisplay *display,
					      guint       *width,
					      guint       *height)
{
  *width = 256;
  *height = 256;
}

gboolean
_cdk_wayland_display_supports_cursor_alpha (CdkDisplay *display)
{
  return TRUE;
}

gboolean
_cdk_wayland_display_supports_cursor_color (CdkDisplay *display)
{
  return TRUE;
}
