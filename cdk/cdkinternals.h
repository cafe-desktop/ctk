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

/* Uninstalled header defining types and functions internal to GDK */

#ifndef __GDK_INTERNALS_H__
#define __GDK_INTERNALS_H__

#include <gio/gio.h>
#include "cdkwindowimpl.h"
#include "cdkdisplay.h"
#include "cdkprivate.h"

G_BEGIN_DECLS

/**********************
 * General Facilities * 
 **********************/

/* Debugging support */

typedef struct _CdkColorInfo           CdkColorInfo;
typedef struct _CdkEventFilter         CdkEventFilter;
typedef struct _CdkClientFilter        CdkClientFilter;

typedef enum {
  GDK_COLOR_WRITEABLE = 1 << 0
} CdkColorInfoFlags;

struct _CdkColorInfo
{
  CdkColorInfoFlags flags;
  guint ref_count;
};

typedef enum {
  GDK_EVENT_FILTER_REMOVED = 1 << 0
} CdkEventFilterFlags;

struct _CdkEventFilter {
  CdkFilterFunc function;
  gpointer data;
  CdkEventFilterFlags flags;
  guint ref_count;
};

struct _CdkClientFilter {
  CdkAtom       type;
  CdkFilterFunc function;
  gpointer      data;
};

typedef enum {
  GDK_DEBUG_MISC          = 1 <<  0,
  GDK_DEBUG_EVENTS        = 1 <<  1,
  GDK_DEBUG_DND           = 1 <<  2,
  GDK_DEBUG_XIM           = 1 <<  3,
  GDK_DEBUG_NOGRABS       = 1 <<  4,
  GDK_DEBUG_INPUT         = 1 <<  5,
  GDK_DEBUG_CURSOR        = 1 <<  6,
  GDK_DEBUG_MULTIHEAD     = 1 <<  7,
  GDK_DEBUG_XINERAMA      = 1 <<  8,
  GDK_DEBUG_DRAW          = 1 <<  9,
  GDK_DEBUG_EVENTLOOP     = 1 << 10,
  GDK_DEBUG_FRAMES        = 1 << 11,
  GDK_DEBUG_SETTINGS      = 1 << 12,
  GDK_DEBUG_OPENGL        = 1 << 13,
} CdkDebugFlag;

typedef enum {
  GDK_RENDERING_MODE_SIMILAR = 0,
  GDK_RENDERING_MODE_IMAGE,
  GDK_RENDERING_MODE_RECORDING
} CdkRenderingMode;

typedef enum {
  GDK_GL_DISABLE                = 1 << 0,
  GDK_GL_ALWAYS                 = 1 << 1,
  GDK_GL_SOFTWARE_DRAW_GL       = 1 << 2,
  GDK_GL_SOFTWARE_DRAW_SURFACE  = 1 << 3,
  GDK_GL_TEXTURE_RECTANGLE      = 1 << 4,
  GDK_GL_LEGACY                 = 1 << 5,
  GDK_GL_GLES                   = 1 << 6
} CdkGLFlags;

extern GList            *_cdk_default_filters;
extern CdkWindow        *_cdk_parent_root;

extern guint _cdk_debug_flags;
extern guint _cdk_gl_flags;
extern CdkRenderingMode    _cdk_rendering_mode;
extern gboolean _cdk_debug_updates;

#ifdef G_ENABLE_DEBUG

#define GDK_DEBUG_CHECK(type) G_UNLIKELY (_cdk_debug_flags & GDK_DEBUG_##type)

#define GDK_NOTE(type,action)                G_STMT_START {     \
    if (GDK_DEBUG_CHECK (type))                                 \
       { action; };                          } G_STMT_END

#else /* !G_ENABLE_DEBUG */

#define GDK_DEBUG_CHECK(type) 0
#define GDK_NOTE(type,action)

#endif /* G_ENABLE_DEBUG */

/* Arg parsing */

typedef enum 
{
  GDK_ARG_STRING,
  GDK_ARG_INT,
  GDK_ARG_BOOL,
  GDK_ARG_NOBOOL,
  GDK_ARG_CALLBACK
} CdkArgType;

typedef struct _CdkArgContext CdkArgContext;
typedef struct _CdkArgDesc CdkArgDesc;

typedef void (*CdkArgFunc) (const char *name, const char *arg, gpointer data);

struct _CdkArgContext
{
  GPtrArray *tables;
  gpointer cb_data;
};

struct _CdkArgDesc
{
  const char *name;
  CdkArgType type;
  gpointer location;
  CdkArgFunc callback;
};

/* Event handling */

typedef struct _CdkEventPrivate CdkEventPrivate;

typedef enum
{
  /* Following flag is set for events on the event queue during
   * translation and cleared afterwards.
   */
  GDK_EVENT_PENDING = 1 << 0,

  /* The following flag is set for:
   * 1) touch events emulating pointer events
   * 2) pointer events being emulated by a touch sequence.
   */
  GDK_EVENT_POINTER_EMULATED = 1 << 1,

  /* When we are ready to draw a frame, we pause event delivery,
   * mark all events in the queue with this flag, and deliver
   * only those events until we finish the frame.
   */
  GDK_EVENT_FLUSHED = 1 << 2
} CdkEventFlags;

struct _CdkEventPrivate
{
  CdkEvent   event;
  guint      flags;
  CdkScreen *screen;
  gpointer   windowing_data;
  CdkDevice *device;
  CdkDevice *source_device;
  CdkSeat   *seat;
  CdkDeviceTool *tool;
  guint16    key_scancode;

#ifdef GDK_WINDOWING_WIN32
  gunichar2 *translation;
  guint      translation_len;
#endif
};

typedef struct _CdkWindowPaint CdkWindowPaint;

struct _CdkWindow
{
  GObject parent_instance;

  CdkWindowImpl *impl; /* window-system-specific delegate object */

  CdkWindow *parent;
  CdkWindow *transient_for;
  CdkVisual *visual;

  gpointer user_data;

  gint x;
  gint y;

  CdkEventMask event_mask;
  guint8 window_type;

  guint8 depth;
  guint8 resize_count;

  gint8 toplevel_window_type;

  GList *filters;
  GList *children;
  GList children_list_node;
  GList *native_children;


  cairo_pattern_t *background;

  /* The paint logic here is a bit complex because of our intermingling of
   * cairo and GL. Let's first go over the cairo-alone case:
   *
   *  1) cdk_window_begin_paint_region() is called with an update region. If
   *     the backend wants it, we redirect drawing to a temporary surface
   *     sized the same as the update region and set `surface_needs_composite`
   *     to TRUE. Otherwise, we paint directly onto the real server-side window.
   *
   *  2) Things paint with cairo using cdk_cairo_create().
   *
   *  3) When everything is painted, the user calls cdk_window_end_paint().
   *     If there was a temporary surface, this is composited back onto the
   *     real backing surface in the appropriate places.
   *
   * This is similar to double buffering, except we only have partial surfaces
   * of undefined contents, and instead of swapping between two buffers, we
   * create a new temporary buffer every time.
   *
   * When we add GL to the mix, we have this instead:
   *
   *  1) cdk_window_begin_paint_region() is called with an update region like
   *     before. We always redirect cairo drawing to a temporary surface when
   *     GL is enabled.
   *
   *  2) Things paint with cairo using cdk_cairo_create(). Whenever
   *     something paints, it calls cdk_window_mark_paint_from_clip() to mark
   *     which regions it has painted in software. We'll learn what this does
   *     soon.
   *
   *  3) Something paints with GL and uses cdk_cairo_draw_from_gl() to
   *     composite back into the scene. We paint this onto the backing
   *     store for the window *immediately* by using GL, rather than
   *     painting to the temporary surface, and keep track of the area that
   *     we've painted in `flushed_region`.
   *
   *  4) Something paints using software again. It calls
   *     cdk_window_mark_paint_from_clip(), which subtracts the region it
   *     has painted from `flushed_region` and adds the region to
   *     `needs_blended_region`.
   *
   *  5) Something paints using GL again, using cdk_cairo_draw_from_gl().
   *     It paints directly to the backing store, removes the region it
   *     painted from `needs_blended_region`, and adds to `flushed_region`.
   *
   *  6) cdk_window_end_paint() is called. It composites the temporary surface
   *     back to the window, using GL, except it doesn't bother copying
   *     `flushed_region`, and when it paints `needs_blended_region`, it also
   *     turns on GL blending.
   *
   * That means that at any point in time, we have three regions:
   *
   *   * `region` - This is the original invalidated region and is never
   *     touched.
   *
   *   * `flushed_region` - This is the portion of `region` that has GL
   *     contents that have been painted directly to the window, and
   *     doesn't have any cairo drawing painted over it.
   *
   *   * `needs_blended_region` - This is the portion of `region` that
   *     GL contents that have part cairo drawing painted over it.
   *     cdk_window_end_paint() will draw this region using blending.
   *
   * `flushed_region` and `needs_blended_region` never intersect, and the
   * rest of `region` that isn't covered by either is the "opaque region",
   * which is any area of cairo drawing that didn't ever intersect with GL.
   * We can paint these from GL without turning on blending.
   **/

  struct {
    /* The temporary surface that we're painting to. This will be composited
     * back into the window when we call end_paint. This is our poor-man's
     * way of doing double buffering. */
    cairo_surface_t *surface;

    cairo_region_t *region;
    cairo_region_t *flushed_region;
    cairo_region_t *need_blend_region;

    gboolean surface_needs_composite;
    gboolean use_gl;
  } current_paint;
  CdkGLContext *gl_paint_context;

  cairo_region_t *update_area;
  guint update_freeze_count;
  /* This is the update_area that was in effect when the current expose
     started. It may be smaller than the expose area if we'e painting
     more than we have to, but it represents the "true" damage. */
  cairo_region_t *active_update_area;
  /* We store the old expose areas to support buffer-age optimizations */
  cairo_region_t *old_updated_area[2];

  CdkWindowState old_state;
  CdkWindowState state;

  guint synthesized_crossing_event_id;

  guint8 alpha;
  guint8 fullscreen_mode;

  guint input_only : 1;
  guint pass_through : 1;
  guint modal_hint : 1;
  guint composited : 1;
  guint has_alpha_background : 1;

  guint destroyed : 2;

  guint accept_focus : 1;
  guint focus_on_map : 1;
  guint shaped : 1;
  guint support_multidevice : 1;
  guint effective_visibility : 2;
  guint visibility : 2; /* The visibility wrt the toplevel (i.e. based on clip_region) */
  guint native_visibility : 2; /* the native visibility of a impl windows */
  guint viewable : 1; /* mapped and all parents mapped */
  guint applied_shape : 1;
  guint in_update : 1;
  guint geometry_dirty : 1;
  guint event_compression : 1;
  guint frame_clock_events_paused : 1;

  /* The CdkWindow that has the impl, ref:ed if another window.
   * This ref is required to keep the wrapper of the impl window alive
   * for as long as any CdkWindow references the impl. */
  CdkWindow *impl_window;

  guint update_and_descendants_freeze_count;

  gint abs_x, abs_y; /* Absolute offset in impl */
  gint width, height;
  gint shadow_top;
  gint shadow_left;
  gint shadow_right;
  gint shadow_bottom;

  guint num_offscreen_children;

  /* The clip region is the part of the window, in window coordinates
     that is fully or partially (i.e. semi transparently) visible in
     the window hierarchy from the toplevel and down */
  cairo_region_t *clip_region;

  CdkCursor *cursor;
  GHashTable *device_cursor;

  cairo_region_t *shape;
  cairo_region_t *input_shape;

  GList *devices_inside;
  GHashTable *device_events;

  GHashTable *source_event_masks;
  gulong device_added_handler_id;
  gulong device_changed_handler_id;

  CdkFrameClock *frame_clock; /* NULL to use from parent or default */
  CdkWindowInvalidateHandlerFunc invalidate_handler;

  CdkDrawingContext *drawing_context;

  cairo_region_t *opaque_region;
};

#define GDK_WINDOW_TYPE(d) ((((CdkWindow *)(d)))->window_type)
#define GDK_WINDOW_DESTROYED(d) (((CdkWindow *)(d))->destroyed)

extern gchar     *_cdk_display_name;
extern gint       _cdk_screen_number;
extern gchar     *_cdk_display_arg_name;
extern gboolean   _cdk_disable_multidevice;

CdkEvent* _cdk_event_unqueue (CdkDisplay *display);

void _cdk_event_filter_unref        (CdkWindow      *window,
				     CdkEventFilter *filter);

void     cdk_event_set_pointer_emulated (CdkEvent *event,
                                         gboolean  emulated);

void     cdk_event_set_scancode        (CdkEvent *event,
                                        guint16 scancode);

void     cdk_event_set_seat              (CdkEvent *event,
                                          CdkSeat  *seat);

/* The IME IM module needs this symbol exported. */
_GDK_EXTERN
gboolean cdk_event_is_allocated      (const CdkEvent *event);

void   _cdk_event_emit               (CdkEvent   *event);
GList* _cdk_event_queue_find_first   (CdkDisplay *display);
void   _cdk_event_queue_remove_link  (CdkDisplay *display,
                                      GList      *node);
GList* _cdk_event_queue_append       (CdkDisplay *display,
                                      CdkEvent   *event);
GList* _cdk_event_queue_insert_after (CdkDisplay *display,
                                      CdkEvent   *after_event,
                                      CdkEvent   *event);
GList* _cdk_event_queue_insert_before(CdkDisplay *display,
                                      CdkEvent   *after_event,
                                      CdkEvent   *event);

void    _cdk_event_queue_handle_motion_compression (CdkDisplay *display);
void    _cdk_event_queue_flush                     (CdkDisplay       *display);

void   _cdk_event_button_generate    (CdkDisplay *display,
                                      CdkEvent   *event);

void _cdk_windowing_event_data_copy (const CdkEvent *src,
                                     CdkEvent       *dst);
void _cdk_windowing_event_data_free (CdkEvent       *event);

void _cdk_set_window_state (CdkWindow *window,
                            CdkWindowState new_state);

gboolean        _cdk_cairo_surface_extents       (cairo_surface_t *surface,
                                                  CdkRectangle    *extents);
void            cdk_gl_texture_from_surface      (cairo_surface_t *surface,
                                                  cairo_region_t  *region);

typedef struct {
  float x1, y1, x2, y2;
  float u1, v1, u2, v2;
} CdkTexturedQuad;

void           cdk_gl_texture_quads               (CdkGLContext *paint_context,
                                                   guint texture_target,
                                                   int n_quads,
                                                   CdkTexturedQuad *quads,
                                                   gboolean flip_colors);

void            cdk_cairo_surface_mark_as_direct (cairo_surface_t *surface,
                                                  CdkWindow       *window);
cairo_region_t *cdk_cairo_region_from_clip       (cairo_t         *cr);

void            cdk_cairo_set_drawing_context    (cairo_t           *cr,
                                                  CdkDrawingContext *context);

/*************************************
 * Interfaces used by windowing code *
 *************************************/

cairo_surface_t *
           _cdk_window_ref_cairo_surface (CdkWindow *window);

void       _cdk_window_destroy           (CdkWindow      *window,
                                          gboolean        foreign_destroy);
void       _cdk_window_clear_update_area (CdkWindow      *window);
void       _cdk_window_update_size       (CdkWindow      *window);
gboolean   _cdk_window_update_viewable   (CdkWindow      *window);
CdkGLContext * cdk_window_get_paint_gl_context (CdkWindow *window,
                                                GError   **error);
void cdk_window_get_unscaled_size (CdkWindow *window,
                                   int *unscaled_width,
                                   int *unscaled_height);

CdkDrawingContext *cdk_window_get_drawing_context (CdkWindow *window);

cairo_region_t *cdk_window_get_current_paint_region (CdkWindow *window);

void       _cdk_window_process_updates_recurse (CdkWindow *window,
                                                cairo_region_t *expose_region);

void       _cdk_screen_set_resolution    (CdkScreen      *screen,
                                          gdouble         dpi);
void       _cdk_screen_close             (CdkScreen      *screen);

/*****************************************
 * Interfaces provided by windowing code *
 *****************************************/

/* Font/string functions implemented in module-specific code */

void _cdk_cursor_destroy (CdkCursor *cursor);

extern const GOptionEntry _cdk_windowing_args[];

void _cdk_windowing_got_event                (CdkDisplay       *display,
                                              GList            *event_link,
                                              CdkEvent         *event,
                                              gulong            serial);

#define GDK_WINDOW_IS_MAPPED(window) (((window)->state & GDK_WINDOW_STATE_WITHDRAWN) == 0)

void _cdk_window_invalidate_for_expose (CdkWindow       *window,
                                        cairo_region_t       *region);

CdkWindow * _cdk_window_find_child_at (CdkWindow *window,
                                       double x, double y);
CdkWindow * _cdk_window_find_descendant_at (CdkWindow *toplevel,
                                            double x, double y,
                                            double *found_x,
                                            double *found_y);

CdkEvent * _cdk_make_event (CdkWindow    *window,
                            CdkEventType  type,
                            CdkEvent     *event_in_queue,
                            gboolean      before_event);
gboolean _cdk_window_event_parent_of (CdkWindow *parent,
                                      CdkWindow *child);

void _cdk_synthesize_crossing_events (CdkDisplay                 *display,
                                      CdkWindow                  *src,
                                      CdkWindow                  *dest,
                                      CdkDevice                  *device,
                                      CdkDevice                  *source_device,
				      CdkCrossingMode             mode,
				      gdouble                     toplevel_x,
				      gdouble                     toplevel_y,
				      CdkModifierType             mask,
				      guint32                     time_,
				      CdkEvent                   *event_in_queue,
				      gulong                      serial,
				      gboolean                    non_linear);
void _cdk_display_set_window_under_pointer (CdkDisplay *display,
                                            CdkDevice  *device,
                                            CdkWindow  *window);


void _cdk_synthesize_crossing_events_for_geometry_change (CdkWindow *changed_window);

gboolean    _cdk_window_has_impl (CdkWindow *window);
CdkWindow * _cdk_window_get_impl_window (CdkWindow *window);

/*****************************
 * offscreen window routines *
 *****************************/
GType cdk_offscreen_window_get_type (void);
void       _cdk_offscreen_window_new                 (CdkWindow     *window,
                                                      CdkWindowAttr *attributes,
                                                      gint           attributes_mask);
cairo_surface_t * _cdk_offscreen_window_create_surface (CdkWindow *window,
                                                        gint       width,
                                                        gint       height);

PangoDirection cdk_unichar_direction (gunichar ch);

G_END_DECLS

#endif /* __GDK_INTERNALS_H__ */
