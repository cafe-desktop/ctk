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

#ifndef __CDK_WINDOW_H__
#define __CDK_WINDOW_H__

#if !defined (__CDK_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdk.h> can be included directly."
#endif

#include <cdk/cdkversionmacros.h>
#include <cdk/cdktypes.h>
#include <cdk/cdkdrawingcontext.h>
#include <cdk/cdkevents.h>
#include <cdk/cdkframeclock.h>

G_BEGIN_DECLS

typedef struct _CdkGeometry          CdkGeometry;
typedef struct _CdkWindowAttr        CdkWindowAttr;
typedef struct _CdkWindowRedirect    CdkWindowRedirect;

/**
 * CdkWindowWindowClass:
 * @CDK_INPUT_OUTPUT: window for graphics and events
 * @CDK_INPUT_ONLY: window for events only
 *
 * @CDK_INPUT_OUTPUT windows are the standard kind of window you might expect.
 * Such windows receive events and are also displayed on screen.
 * @CDK_INPUT_ONLY windows are invisible; they are usually placed above other
 * windows in order to trap or filter the events. You can’t draw on
 * @CDK_INPUT_ONLY windows.
 */
typedef enum
{
  CDK_INPUT_OUTPUT, /*< nick=input-output >*/
  CDK_INPUT_ONLY    /*< nick=input-only >*/
} CdkWindowWindowClass;

/**
 * CdkWindowType:
 * @CDK_WINDOW_ROOT: root window; this window has no parent, covers the entire
 *  screen, and is created by the window system
 * @CDK_WINDOW_TOPLEVEL: toplevel window (used to implement #CtkWindow)
 * @CDK_WINDOW_CHILD: child window (used to implement e.g. #CtkEntry)
 * @CDK_WINDOW_TEMP: override redirect temporary window (used to implement
 *  #CtkMenu)
 * @CDK_WINDOW_FOREIGN: foreign window (see cdk_window_foreign_new())
 * @CDK_WINDOW_OFFSCREEN: offscreen window (see
 *  [Offscreen Windows][OFFSCREEN-WINDOWS]). Since 2.18
 * @CDK_WINDOW_SUBSURFACE: subsurface-based window; This window is visually
 *  tied to a toplevel, and is moved/stacked with it. Currently this window
 *  type is only implemented in Wayland. Since 3.14
 *
 * Describes the kind of window.
 */
typedef enum
{
  CDK_WINDOW_ROOT,
  CDK_WINDOW_TOPLEVEL,
  CDK_WINDOW_CHILD,
  CDK_WINDOW_TEMP,
  CDK_WINDOW_FOREIGN,
  CDK_WINDOW_OFFSCREEN,
  CDK_WINDOW_SUBSURFACE
} CdkWindowType;

/**
 * CdkWindowAttributesType:
 * @CDK_WA_TITLE: Honor the title field
 * @CDK_WA_X: Honor the X coordinate field
 * @CDK_WA_Y: Honor the Y coordinate field
 * @CDK_WA_CURSOR: Honor the cursor field
 * @CDK_WA_VISUAL: Honor the visual field
 * @CDK_WA_WMCLASS: Honor the wmclass_class and wmclass_name fields
 * @CDK_WA_NOREDIR: Honor the override_redirect field
 * @CDK_WA_TYPE_HINT: Honor the type_hint field
 *
 * Used to indicate which fields in the #CdkWindowAttr struct should be honored.
 * For example, if you filled in the “cursor” and “x” fields of #CdkWindowAttr,
 * pass “@CDK_WA_X | @CDK_WA_CURSOR” to cdk_window_new(). Fields in
 * #CdkWindowAttr not covered by a bit in this enum are required; for example,
 * the @width/@height, @wclass, and @window_type fields are required, they have
 * no corresponding flag in #CdkWindowAttributesType.
 */
typedef enum
{
  CDK_WA_TITLE	   = 1 << 1,
  CDK_WA_X	   = 1 << 2,
  CDK_WA_Y	   = 1 << 3,
  CDK_WA_CURSOR	   = 1 << 4,
  CDK_WA_VISUAL	   = 1 << 5,
  CDK_WA_WMCLASS   = 1 << 6,
  CDK_WA_NOREDIR   = 1 << 7,
  CDK_WA_TYPE_HINT = 1 << 8
} CdkWindowAttributesType;

/* Size restriction enumeration.
 */
/**
 * CdkWindowHints:
 * @CDK_HINT_POS: indicates that the program has positioned the window
 * @CDK_HINT_MIN_SIZE: min size fields are set
 * @CDK_HINT_MAX_SIZE: max size fields are set
 * @CDK_HINT_BASE_SIZE: base size fields are set
 * @CDK_HINT_ASPECT: aspect ratio fields are set
 * @CDK_HINT_RESIZE_INC: resize increment fields are set
 * @CDK_HINT_WIN_GRAVITY: window gravity field is set
 * @CDK_HINT_USER_POS: indicates that the window’s position was explicitly set
 *  by the user
 * @CDK_HINT_USER_SIZE: indicates that the window’s size was explicitly set by
 *  the user
 *
 * Used to indicate which fields of a #CdkGeometry struct should be paid
 * attention to. Also, the presence/absence of @CDK_HINT_POS,
 * @CDK_HINT_USER_POS, and @CDK_HINT_USER_SIZE is significant, though they don't
 * directly refer to #CdkGeometry fields. @CDK_HINT_USER_POS will be set
 * automatically by #CtkWindow if you call ctk_window_move().
 * @CDK_HINT_USER_POS and @CDK_HINT_USER_SIZE should be set if the user
 * specified a size/position using a --geometry command-line argument;
 * ctk_window_parse_geometry() automatically sets these flags.
 */
typedef enum
{
  CDK_HINT_POS	       = 1 << 0,
  CDK_HINT_MIN_SIZE    = 1 << 1,
  CDK_HINT_MAX_SIZE    = 1 << 2,
  CDK_HINT_BASE_SIZE   = 1 << 3,
  CDK_HINT_ASPECT      = 1 << 4,
  CDK_HINT_RESIZE_INC  = 1 << 5,
  CDK_HINT_WIN_GRAVITY = 1 << 6,
  CDK_HINT_USER_POS    = 1 << 7,
  CDK_HINT_USER_SIZE   = 1 << 8
} CdkWindowHints;

/* The next two enumeration values current match the
 * Motif constants. If this is changed, the implementation
 * of cdk_window_set_decorations/cdk_window_set_functions
 * will need to change as well.
 */
/**
 * CdkWMDecoration:
 * @CDK_DECOR_ALL: all decorations should be applied.
 * @CDK_DECOR_BORDER: a frame should be drawn around the window.
 * @CDK_DECOR_RESIZEH: the frame should have resize handles.
 * @CDK_DECOR_TITLE: a titlebar should be placed above the window.
 * @CDK_DECOR_MENU: a button for opening a menu should be included.
 * @CDK_DECOR_MINIMIZE: a minimize button should be included.
 * @CDK_DECOR_MAXIMIZE: a maximize button should be included.
 *
 * These are hints originally defined by the Motif toolkit.
 * The window manager can use them when determining how to decorate
 * the window. The hint must be set before mapping the window.
 */
typedef enum
{
  CDK_DECOR_ALL		= 1 << 0,
  CDK_DECOR_BORDER	= 1 << 1,
  CDK_DECOR_RESIZEH	= 1 << 2,
  CDK_DECOR_TITLE	= 1 << 3,
  CDK_DECOR_MENU	= 1 << 4,
  CDK_DECOR_MINIMIZE	= 1 << 5,
  CDK_DECOR_MAXIMIZE	= 1 << 6
} CdkWMDecoration;

/**
 * CdkWMFunction:
 * @CDK_FUNC_ALL: all functions should be offered.
 * @CDK_FUNC_RESIZE: the window should be resizable.
 * @CDK_FUNC_MOVE: the window should be movable.
 * @CDK_FUNC_MINIMIZE: the window should be minimizable.
 * @CDK_FUNC_MAXIMIZE: the window should be maximizable.
 * @CDK_FUNC_CLOSE: the window should be closable.
 *
 * These are hints originally defined by the Motif toolkit. The window manager
 * can use them when determining the functions to offer for the window. The
 * hint must be set before mapping the window.
 */
typedef enum
{
  CDK_FUNC_ALL		= 1 << 0,
  CDK_FUNC_RESIZE	= 1 << 1,
  CDK_FUNC_MOVE		= 1 << 2,
  CDK_FUNC_MINIMIZE	= 1 << 3,
  CDK_FUNC_MAXIMIZE	= 1 << 4,
  CDK_FUNC_CLOSE	= 1 << 5
} CdkWMFunction;

/* Currently, these are the same values numerically as in the
 * X protocol. If you change that, cdkwindow-x11.c/cdk_window_set_geometry_hints()
 * will need fixing.
 */
/**
 * CdkGravity:
 * @CDK_GRAVITY_NORTH_WEST: the reference point is at the top left corner.
 * @CDK_GRAVITY_NORTH: the reference point is in the middle of the top edge.
 * @CDK_GRAVITY_NORTH_EAST: the reference point is at the top right corner.
 * @CDK_GRAVITY_WEST: the reference point is at the middle of the left edge.
 * @CDK_GRAVITY_CENTER: the reference point is at the center of the window.
 * @CDK_GRAVITY_EAST: the reference point is at the middle of the right edge.
 * @CDK_GRAVITY_SOUTH_WEST: the reference point is at the lower left corner.
 * @CDK_GRAVITY_SOUTH: the reference point is at the middle of the lower edge.
 * @CDK_GRAVITY_SOUTH_EAST: the reference point is at the lower right corner.
 * @CDK_GRAVITY_STATIC: the reference point is at the top left corner of the
 *  window itself, ignoring window manager decorations.
 *
 * Defines the reference point of a window and the meaning of coordinates
 * passed to ctk_window_move(). See ctk_window_move() and the "implementation
 * notes" section of the
 * [Extended Window Manager Hints](http://www.freedesktop.org/Standards/wm-spec)
 * specification for more details.
 */
typedef enum
{
  CDK_GRAVITY_NORTH_WEST = 1,
  CDK_GRAVITY_NORTH,
  CDK_GRAVITY_NORTH_EAST,
  CDK_GRAVITY_WEST,
  CDK_GRAVITY_CENTER,
  CDK_GRAVITY_EAST,
  CDK_GRAVITY_SOUTH_WEST,
  CDK_GRAVITY_SOUTH,
  CDK_GRAVITY_SOUTH_EAST,
  CDK_GRAVITY_STATIC
} CdkGravity;

/**
 * CdkAnchorHints:
 * @CDK_ANCHOR_FLIP_X: allow flipping anchors horizontally
 * @CDK_ANCHOR_FLIP_Y: allow flipping anchors vertically
 * @CDK_ANCHOR_SLIDE_X: allow sliding window horizontally
 * @CDK_ANCHOR_SLIDE_Y: allow sliding window vertically
 * @CDK_ANCHOR_RESIZE_X: allow resizing window horizontally
 * @CDK_ANCHOR_RESIZE_Y: allow resizing window vertically
 * @CDK_ANCHOR_FLIP: allow flipping anchors on both axes
 * @CDK_ANCHOR_SLIDE: allow sliding window on both axes
 * @CDK_ANCHOR_RESIZE: allow resizing window on both axes
 *
 * Positioning hints for aligning a window relative to a rectangle.
 *
 * These hints determine how the window should be positioned in the case that
 * the window would fall off-screen if placed in its ideal position.
 *
 * For example, %CDK_ANCHOR_FLIP_X will replace %CDK_GRAVITY_NORTH_WEST with
 * %CDK_GRAVITY_NORTH_EAST and vice versa if the window extends beyond the left
 * or right edges of the monitor.
 *
 * If %CDK_ANCHOR_SLIDE_X is set, the window can be shifted horizontally to fit
 * on-screen. If %CDK_ANCHOR_RESIZE_X is set, the window can be shrunken
 * horizontally to fit.
 *
 * In general, when multiple flags are set, flipping should take precedence over
 * sliding, which should take precedence over resizing.
 *
 * Since: 3.22
 * Stability: Unstable
 */
typedef enum
{
  CDK_ANCHOR_FLIP_X   = 1 << 0,
  CDK_ANCHOR_FLIP_Y   = 1 << 1,
  CDK_ANCHOR_SLIDE_X  = 1 << 2,
  CDK_ANCHOR_SLIDE_Y  = 1 << 3,
  CDK_ANCHOR_RESIZE_X = 1 << 4,
  CDK_ANCHOR_RESIZE_Y = 1 << 5,
  CDK_ANCHOR_FLIP     = CDK_ANCHOR_FLIP_X | CDK_ANCHOR_FLIP_Y,
  CDK_ANCHOR_SLIDE    = CDK_ANCHOR_SLIDE_X | CDK_ANCHOR_SLIDE_Y,
  CDK_ANCHOR_RESIZE   = CDK_ANCHOR_RESIZE_X | CDK_ANCHOR_RESIZE_Y
} CdkAnchorHints;

/**
 * CdkWindowEdge:
 * @CDK_WINDOW_EDGE_NORTH_WEST: the top left corner.
 * @CDK_WINDOW_EDGE_NORTH: the top edge.
 * @CDK_WINDOW_EDGE_NORTH_EAST: the top right corner.
 * @CDK_WINDOW_EDGE_WEST: the left edge.
 * @CDK_WINDOW_EDGE_EAST: the right edge.
 * @CDK_WINDOW_EDGE_SOUTH_WEST: the lower left corner.
 * @CDK_WINDOW_EDGE_SOUTH: the lower edge.
 * @CDK_WINDOW_EDGE_SOUTH_EAST: the lower right corner.
 *
 * Determines a window edge or corner.
 */
typedef enum
{
  CDK_WINDOW_EDGE_NORTH_WEST,
  CDK_WINDOW_EDGE_NORTH,
  CDK_WINDOW_EDGE_NORTH_EAST,
  CDK_WINDOW_EDGE_WEST,
  CDK_WINDOW_EDGE_EAST,
  CDK_WINDOW_EDGE_SOUTH_WEST,
  CDK_WINDOW_EDGE_SOUTH,
  CDK_WINDOW_EDGE_SOUTH_EAST  
} CdkWindowEdge;

/**
 * CdkFullscreenMode:
 * @CDK_FULLSCREEN_ON_CURRENT_MONITOR: Fullscreen on current monitor only.
 * @CDK_FULLSCREEN_ON_ALL_MONITORS: Span across all monitors when fullscreen.
 *
 * Indicates which monitor (in a multi-head setup) a window should span over
 * when in fullscreen mode.
 *
 * Since: 3.8
 **/
typedef enum
{
  CDK_FULLSCREEN_ON_CURRENT_MONITOR,
  CDK_FULLSCREEN_ON_ALL_MONITORS
} CdkFullscreenMode;

/**
 * CdkWindowAttr:
 * @title: title of the window (for toplevel windows)
 * @event_mask: event mask (see cdk_window_set_events())
 * @x: X coordinate relative to parent window (see cdk_window_move())
 * @y: Y coordinate relative to parent window (see cdk_window_move())
 * @width: width of window
 * @height: height of window
 * @wclass: #CDK_INPUT_OUTPUT (normal window) or #CDK_INPUT_ONLY (invisible
 *  window that receives events)
 * @visual: #CdkVisual for window
 * @window_type: type of window
 * @cursor: cursor for the window (see cdk_window_set_cursor())
 * @wmclass_name: don’t use (see ctk_window_set_wmclass())
 * @wmclass_class: don’t use (see ctk_window_set_wmclass())
 * @override_redirect: %TRUE to bypass the window manager
 * @type_hint: a hint of the function of the window
 *
 * Attributes to use for a newly-created window.
 */
struct _CdkWindowAttr
{
  gchar *title;
  gint event_mask;
  gint x, y;
  gint width;
  gint height;
  CdkWindowWindowClass wclass;
  CdkVisual *visual;
  CdkWindowType window_type;
  CdkCursor *cursor;
  gchar *wmclass_name;
  gchar *wmclass_class;
  gboolean override_redirect;
  CdkWindowTypeHint type_hint;
};

/**
 * CdkGeometry:
 * @min_width: minimum width of window (or -1 to use requisition, with
 *  #CtkWindow only)
 * @min_height: minimum height of window (or -1 to use requisition, with
 *  #CtkWindow only)
 * @max_width: maximum width of window (or -1 to use requisition, with
 *  #CtkWindow only)
 * @max_height: maximum height of window (or -1 to use requisition, with
 *  #CtkWindow only)
 * @base_width: allowed window widths are @base_width + @width_inc * N where N
 *  is any integer (-1 allowed with #CtkWindow)
 * @base_height: allowed window widths are @base_height + @height_inc * N where
 *  N is any integer (-1 allowed with #CtkWindow)
 * @width_inc: width resize increment
 * @height_inc: height resize increment
 * @min_aspect: minimum width/height ratio
 * @max_aspect: maximum width/height ratio
 * @win_gravity: window gravity, see ctk_window_set_gravity()
 *
 * The #CdkGeometry struct gives the window manager information about
 * a window’s geometry constraints. Normally you would set these on
 * the CTK+ level using ctk_window_set_geometry_hints(). #CtkWindow
 * then sets the hints on the #CdkWindow it creates.
 *
 * cdk_window_set_geometry_hints() expects the hints to be fully valid already
 * and simply passes them to the window manager; in contrast,
 * ctk_window_set_geometry_hints() performs some interpretation. For example,
 * #CtkWindow will apply the hints to the geometry widget instead of the
 * toplevel window, if you set a geometry widget. Also, the
 * @min_width/@min_height/@max_width/@max_height fields may be set to -1, and
 * #CtkWindow will substitute the size request of the window or geometry widget.
 * If the minimum size hint is not provided, #CtkWindow will use its requisition
 * as the minimum size. If the minimum size is provided and a geometry widget is
 * set, #CtkWindow will take the minimum size as the minimum size of the
 * geometry widget rather than the entire window. The base size is treated
 * similarly.
 *
 * The canonical use-case for ctk_window_set_geometry_hints() is to get a
 * terminal widget to resize properly. Here, the terminal text area should be
 * the geometry widget; #CtkWindow will then automatically set the base size to
 * the size of other widgets in the terminal window, such as the menubar and
 * scrollbar. Then, the @width_inc and @height_inc fields should be set to the
 * size of one character in the terminal. Finally, the base size should be set
 * to the size of one character. The net effect is that the minimum size of the
 * terminal will have a 1x1 character terminal area, and only terminal sizes on
 * the “character grid” will be allowed.
 *
 * Here’s an example of how the terminal example would be implemented, assuming
 * a terminal area widget called “terminal” and a toplevel window “toplevel”:
 *
 * |[<!-- language="C" -->
 * 	CdkGeometry hints;
 *
 * 	hints.base_width = terminal->char_width;
 *         hints.base_height = terminal->char_height;
 *         hints.min_width = terminal->char_width;
 *         hints.min_height = terminal->char_height;
 *         hints.width_inc = terminal->char_width;
 *         hints.height_inc = terminal->char_height;
 *
 *  ctk_window_set_geometry_hints (CTK_WINDOW (toplevel),
 *                                 CTK_WIDGET (terminal),
 *                                 &hints,
 *                                 CDK_HINT_RESIZE_INC |
 *                                 CDK_HINT_MIN_SIZE |
 *                                 CDK_HINT_BASE_SIZE);
 * ]|
 *
 * The other useful fields are the @min_aspect and @max_aspect fields; these
 * contain a width/height ratio as a floating point number. If a geometry widget
 * is set, the aspect applies to the geometry widget rather than the entire
 * window. The most common use of these hints is probably to set @min_aspect and
 * @max_aspect to the same value, thus forcing the window to keep a constant
 * aspect ratio.
 */
struct _CdkGeometry
{
  gint min_width;
  gint min_height;
  gint max_width;
  gint max_height;
  gint base_width;
  gint base_height;
  gint width_inc;
  gint height_inc;
  gdouble min_aspect;
  gdouble max_aspect;
  CdkGravity win_gravity;
};

typedef struct _CdkWindowClass CdkWindowClass;

#define CDK_TYPE_WINDOW              (cdk_window_get_type ())
#define CDK_WINDOW(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), CDK_TYPE_WINDOW, CdkWindow))
#define CDK_WINDOW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CDK_TYPE_WINDOW, CdkWindowClass))
#define CDK_IS_WINDOW(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), CDK_TYPE_WINDOW))
#define CDK_IS_WINDOW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CDK_TYPE_WINDOW))
#define CDK_WINDOW_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CDK_TYPE_WINDOW, CdkWindowClass))


struct _CdkWindowClass
{
  GObjectClass      parent_class;

  CdkWindow       * (* pick_embedded_child) (CdkWindow *window,
                                             gdouble    x,
                                             gdouble    y);

  /*  the following 3 signals will only be emitted by offscreen windows */
  void              (* to_embedder)         (CdkWindow *window,
                                             gdouble    offscreen_x,
                                             gdouble    offscreen_y,
                                             gdouble   *embedder_x,
                                             gdouble   *embedder_y);
  void              (* from_embedder)       (CdkWindow *window,
                                             gdouble    embedder_x,
                                             gdouble    embedder_y,
                                             gdouble   *offscreen_x,
                                             gdouble   *offscreen_y);
  cairo_surface_t * (* create_surface)      (CdkWindow *window,
                                             gint       width,
                                             gint       height);

  /* Padding for future expansion */
  void (*_cdk_reserved1) (void);
  void (*_cdk_reserved2) (void);
  void (*_cdk_reserved3) (void);
  void (*_cdk_reserved4) (void);
  void (*_cdk_reserved5) (void);
  void (*_cdk_reserved6) (void);
  void (*_cdk_reserved7) (void);
  void (*_cdk_reserved8) (void);
};

/* Windows
 */
CDK_AVAILABLE_IN_ALL
GType         cdk_window_get_type              (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CdkWindow*    cdk_window_new                   (CdkWindow     *parent,
                                                CdkWindowAttr *attributes,
                                                gint           attributes_mask);
CDK_AVAILABLE_IN_ALL
void          cdk_window_destroy               (CdkWindow     *window);
CDK_AVAILABLE_IN_ALL
CdkWindowType cdk_window_get_window_type       (CdkWindow     *window);
CDK_AVAILABLE_IN_ALL
gboolean      cdk_window_is_destroyed          (CdkWindow     *window);

CDK_AVAILABLE_IN_ALL
CdkVisual *   cdk_window_get_visual            (CdkWindow     *window);
CDK_AVAILABLE_IN_ALL
CdkScreen *   cdk_window_get_screen            (CdkWindow     *window);
CDK_AVAILABLE_IN_ALL
CdkDisplay *  cdk_window_get_display           (CdkWindow     *window);
#ifndef CDK_MULTIDEVICE_SAFE
CDK_AVAILABLE_IN_ALL
CdkWindow*    cdk_window_at_pointer            (gint          *win_x,
                                                gint          *win_y);
#endif /* CDK_MULTIDEVICE_SAFE */
CDK_AVAILABLE_IN_ALL
void          cdk_window_show                  (CdkWindow     *window);
CDK_AVAILABLE_IN_ALL
void          cdk_window_hide                  (CdkWindow     *window);
CDK_AVAILABLE_IN_ALL
void          cdk_window_withdraw              (CdkWindow     *window);
CDK_AVAILABLE_IN_ALL
void          cdk_window_show_unraised         (CdkWindow     *window);
CDK_AVAILABLE_IN_ALL
void          cdk_window_move                  (CdkWindow     *window,
                                                gint           x,
                                                gint           y);
CDK_AVAILABLE_IN_ALL
void          cdk_window_resize                (CdkWindow     *window,
                                                gint           width,
                                                gint           height);
CDK_AVAILABLE_IN_ALL
void          cdk_window_move_resize           (CdkWindow     *window,
                                                gint           x,
                                                gint           y,
                                                gint           width,
                                                gint           height);

CDK_AVAILABLE_IN_3_24
void          cdk_window_move_to_rect          (CdkWindow          *window,
                                                const CdkRectangle *rect,
                                                CdkGravity          rect_anchor,
                                                CdkGravity          window_anchor,
                                                CdkAnchorHints      anchor_hints,
                                                gint                rect_anchor_dx,
                                                gint                rect_anchor_dy);
CDK_AVAILABLE_IN_ALL
void          cdk_window_reparent              (CdkWindow     *window,
                                                CdkWindow     *new_parent,
                                                gint           x,
                                                gint           y);
CDK_AVAILABLE_IN_ALL
void          cdk_window_raise                 (CdkWindow     *window);
CDK_AVAILABLE_IN_ALL
void          cdk_window_lower                 (CdkWindow     *window);
CDK_AVAILABLE_IN_ALL
void          cdk_window_restack               (CdkWindow     *window,
						CdkWindow     *sibling,
						gboolean       above);
CDK_AVAILABLE_IN_ALL
void          cdk_window_focus                 (CdkWindow     *window,
                                                guint32        timestamp);
CDK_AVAILABLE_IN_ALL
void          cdk_window_set_user_data         (CdkWindow     *window,
                                                gpointer       user_data);
CDK_AVAILABLE_IN_ALL
void          cdk_window_set_override_redirect (CdkWindow     *window,
                                                gboolean       override_redirect);
CDK_AVAILABLE_IN_ALL
gboolean      cdk_window_get_accept_focus      (CdkWindow     *window);
CDK_AVAILABLE_IN_ALL
void          cdk_window_set_accept_focus      (CdkWindow     *window,
					        gboolean       accept_focus);
CDK_AVAILABLE_IN_ALL
gboolean      cdk_window_get_focus_on_map      (CdkWindow     *window);
CDK_AVAILABLE_IN_ALL
void          cdk_window_set_focus_on_map      (CdkWindow     *window,
					        gboolean       focus_on_map);
CDK_AVAILABLE_IN_ALL
void          cdk_window_add_filter            (CdkWindow     *window,
                                                CdkFilterFunc  function,
                                                gpointer       data);
CDK_AVAILABLE_IN_ALL
void          cdk_window_remove_filter         (CdkWindow     *window,
                                                CdkFilterFunc  function,
                                                gpointer       data);
CDK_AVAILABLE_IN_ALL
void          cdk_window_scroll                (CdkWindow     *window,
                                                gint           dx,
                                                gint           dy);
CDK_AVAILABLE_IN_ALL
void	      cdk_window_move_region           (CdkWindow       *window,
						const cairo_region_t *region,
						gint             dx,
						gint             dy);
CDK_AVAILABLE_IN_ALL
gboolean      cdk_window_ensure_native        (CdkWindow       *window);

/* 
 * This allows for making shaped (partially transparent) windows
 * - cool feature, needed for Drag and Drag for example.
 */
CDK_AVAILABLE_IN_ALL
void cdk_window_shape_combine_region (CdkWindow	      *window,
                                      const cairo_region_t *shape_region,
                                      gint	       offset_x,
                                      gint	       offset_y);

/*
 * This routine allows you to quickly take the shapes of all the child windows
 * of a window and use their shapes as the shape mask for this window - useful
 * for container windows that dont want to look like a big box
 * 
 * - Raster
 */
CDK_AVAILABLE_IN_ALL
void cdk_window_set_child_shapes (CdkWindow *window);

CDK_AVAILABLE_IN_ALL
gboolean cdk_window_get_composited (CdkWindow *window);
CDK_AVAILABLE_IN_ALL
void cdk_window_set_composited   (CdkWindow *window,
                                  gboolean   composited);

/*
 * This routine allows you to merge (ie ADD) child shapes to your
 * own window’s shape keeping its current shape and ADDING the child
 * shapes to it.
 * 
 * - Raster
 */
CDK_AVAILABLE_IN_ALL
void cdk_window_merge_child_shapes         (CdkWindow       *window);

CDK_AVAILABLE_IN_ALL
void cdk_window_input_shape_combine_region (CdkWindow       *window,
                                            const cairo_region_t *shape_region,
                                            gint             offset_x,
                                            gint             offset_y);
CDK_AVAILABLE_IN_ALL
void cdk_window_set_child_input_shapes     (CdkWindow       *window);
CDK_AVAILABLE_IN_ALL
void cdk_window_merge_child_input_shapes   (CdkWindow       *window);


CDK_AVAILABLE_IN_3_18
void cdk_window_set_pass_through (CdkWindow *window,
                                  gboolean   pass_through);
CDK_AVAILABLE_IN_3_18
gboolean cdk_window_get_pass_through (CdkWindow *window);

/*
 * Check if a window has been shown, and whether all its
 * parents up to a toplevel have been shown, respectively.
 * Note that a window that is_viewable below is not necessarily
 * viewable in the X sense.
 */
CDK_AVAILABLE_IN_ALL
gboolean cdk_window_is_visible     (CdkWindow *window);
CDK_AVAILABLE_IN_ALL
gboolean cdk_window_is_viewable    (CdkWindow *window);
CDK_AVAILABLE_IN_ALL
gboolean cdk_window_is_input_only  (CdkWindow *window);
CDK_AVAILABLE_IN_ALL
gboolean cdk_window_is_shaped      (CdkWindow *window);

CDK_AVAILABLE_IN_ALL
CdkWindowState cdk_window_get_state (CdkWindow *window);

/* Set static bit gravity on the parent, and static
 * window gravity on all children.
 */
CDK_DEPRECATED_IN_3_16
gboolean cdk_window_set_static_gravities (CdkWindow *window,
                                          gboolean   use_static);

/* CdkWindow */

/**
 * CdkWindowInvalidateHandlerFunc:
 * @window: a #CdkWindow
 * @region: a #cairo_region_t
 *
 * Whenever some area of the window is invalidated (directly in the
 * window or in a child window) this gets called with @region in
 * the coordinate space of @window. You can use @region to just
 * keep track of the dirty region, or you can actually change
 * @region in case you are doing display tricks like showing
 * a child in multiple places.
 *
 * Since: 3.10
 */
typedef void (*CdkWindowInvalidateHandlerFunc)  (CdkWindow      *window,
						 cairo_region_t *region);
CDK_AVAILABLE_IN_3_10
void cdk_window_set_invalidate_handler (CdkWindow                      *window,
					CdkWindowInvalidateHandlerFunc  handler);

CDK_AVAILABLE_IN_ALL
gboolean      cdk_window_has_native         (CdkWindow       *window);
CDK_AVAILABLE_IN_ALL
void              cdk_window_set_type_hint (CdkWindow        *window,
                                            CdkWindowTypeHint hint);
CDK_AVAILABLE_IN_ALL
CdkWindowTypeHint cdk_window_get_type_hint (CdkWindow        *window);

CDK_AVAILABLE_IN_ALL
gboolean      cdk_window_get_modal_hint   (CdkWindow       *window);
CDK_AVAILABLE_IN_ALL
void          cdk_window_set_modal_hint   (CdkWindow       *window,
                                           gboolean         modal);

CDK_AVAILABLE_IN_ALL
void cdk_window_set_skip_taskbar_hint (CdkWindow *window,
                                       gboolean   skips_taskbar);
CDK_AVAILABLE_IN_ALL
void cdk_window_set_skip_pager_hint   (CdkWindow *window,
                                       gboolean   skips_pager);
CDK_AVAILABLE_IN_ALL
void cdk_window_set_urgency_hint      (CdkWindow *window,
				       gboolean   urgent);

CDK_AVAILABLE_IN_ALL
void          cdk_window_set_geometry_hints (CdkWindow          *window,
					     const CdkGeometry  *geometry,
					     CdkWindowHints      geom_mask);

CDK_AVAILABLE_IN_ALL
cairo_region_t *cdk_window_get_clip_region  (CdkWindow          *window);
CDK_AVAILABLE_IN_ALL
cairo_region_t *cdk_window_get_visible_region(CdkWindow         *window);


CDK_DEPRECATED_IN_3_22_FOR(cdk_window_begin_draw_frame)
void	      cdk_window_begin_paint_rect   (CdkWindow          *window,
					     const CdkRectangle *rectangle);
CDK_AVAILABLE_IN_3_16
void	      cdk_window_mark_paint_from_clip (CdkWindow          *window,
					       cairo_t            *cr);
CDK_DEPRECATED_IN_3_22_FOR(cdk_window_begin_draw_frame)
void	      cdk_window_begin_paint_region (CdkWindow          *window,
					     const cairo_region_t    *region);
CDK_DEPRECATED_IN_3_22_FOR(cdk_window_end_draw_frame)
void	      cdk_window_end_paint          (CdkWindow          *window);

CDK_AVAILABLE_IN_3_22
CdkDrawingContext *cdk_window_begin_draw_frame  (CdkWindow            *window,
                                                 const cairo_region_t *region);
CDK_AVAILABLE_IN_3_22
void          cdk_window_end_draw_frame    (CdkWindow            *window,
                                            CdkDrawingContext    *context);

CDK_DEPRECATED_IN_3_14
void	      cdk_window_flush             (CdkWindow          *window);

CDK_AVAILABLE_IN_ALL
void	      cdk_window_set_title	   (CdkWindow	  *window,
					    const gchar	  *title);
CDK_AVAILABLE_IN_ALL
void          cdk_window_set_role          (CdkWindow     *window,
					    const gchar   *role);
CDK_AVAILABLE_IN_ALL
void          cdk_window_set_startup_id    (CdkWindow     *window,
					    const gchar   *startup_id);
CDK_AVAILABLE_IN_ALL
void          cdk_window_set_transient_for (CdkWindow     *window,
					    CdkWindow     *parent);
CDK_AVAILABLE_IN_ALL
void	      cdk_window_set_background	 (CdkWindow	  *window,
					  const CdkColor  *color);
CDK_AVAILABLE_IN_ALL
void          cdk_window_set_background_rgba (CdkWindow     *window,
                                              const CdkRGBA *rgba);
CDK_AVAILABLE_IN_ALL
void	      cdk_window_set_background_pattern (CdkWindow	 *window,
                                                 cairo_pattern_t *pattern);
CDK_AVAILABLE_IN_ALL
cairo_pattern_t *cdk_window_get_background_pattern (CdkWindow     *window);

CDK_AVAILABLE_IN_ALL
void	      cdk_window_set_cursor	 (CdkWindow	  *window,
					  CdkCursor	  *cursor);
CDK_AVAILABLE_IN_ALL
CdkCursor    *cdk_window_get_cursor      (CdkWindow       *window);
CDK_AVAILABLE_IN_ALL
void	      cdk_window_set_device_cursor (CdkWindow	  *window,
                                            CdkDevice     *device,
                                            CdkCursor	  *cursor);
CDK_AVAILABLE_IN_ALL
CdkCursor    *cdk_window_get_device_cursor (CdkWindow     *window,
                                            CdkDevice     *device);
CDK_AVAILABLE_IN_ALL
void	      cdk_window_get_user_data	 (CdkWindow	  *window,
					  gpointer	  *data);
CDK_AVAILABLE_IN_ALL
void	      cdk_window_get_geometry	 (CdkWindow	  *window,
					  gint		  *x,
					  gint		  *y,
					  gint		  *width,
					  gint		  *height);
CDK_AVAILABLE_IN_ALL
int           cdk_window_get_width       (CdkWindow       *window);
CDK_AVAILABLE_IN_ALL
int           cdk_window_get_height      (CdkWindow       *window);
CDK_AVAILABLE_IN_ALL
void	      cdk_window_get_position	 (CdkWindow	  *window,
					  gint		  *x,
					  gint		  *y);
CDK_AVAILABLE_IN_ALL
gint	      cdk_window_get_origin	 (CdkWindow	  *window,
					  gint		  *x,
					  gint		  *y);
CDK_AVAILABLE_IN_ALL
void	      cdk_window_get_root_coords (CdkWindow	  *window,
					  gint             x,
					  gint             y,
					  gint		  *root_x,
					  gint		  *root_y);
CDK_AVAILABLE_IN_ALL
void       cdk_window_coords_to_parent   (CdkWindow       *window,
                                          gdouble          x,
                                          gdouble          y,
                                          gdouble         *parent_x,
                                          gdouble         *parent_y);
CDK_AVAILABLE_IN_ALL
void       cdk_window_coords_from_parent (CdkWindow       *window,
                                          gdouble          parent_x,
                                          gdouble          parent_y,
                                          gdouble         *x,
                                          gdouble         *y);

CDK_AVAILABLE_IN_ALL
void	      cdk_window_get_root_origin (CdkWindow	  *window,
					  gint		  *x,
					  gint		  *y);
CDK_AVAILABLE_IN_ALL
void          cdk_window_get_frame_extents (CdkWindow     *window,
                                            CdkRectangle  *rect);

CDK_AVAILABLE_IN_3_10
gint          cdk_window_get_scale_factor  (CdkWindow     *window);

#ifndef CDK_MULTIDEVICE_SAFE
CDK_AVAILABLE_IN_ALL
CdkWindow *   cdk_window_get_pointer     (CdkWindow       *window,
                                          gint            *x,
                                          gint            *y,
                                          CdkModifierType *mask);
#endif /* CDK_MULTIDEVICE_SAFE */
CDK_AVAILABLE_IN_ALL
CdkWindow *   cdk_window_get_device_position (CdkWindow       *window,
                                              CdkDevice       *device,
                                              gint            *x,
                                              gint            *y,
                                              CdkModifierType *mask);
CDK_AVAILABLE_IN_3_10
CdkWindow *   cdk_window_get_device_position_double (CdkWindow       *window,
                                                     CdkDevice       *device,
                                                     gdouble         *x,
                                                     gdouble         *y,
                                                     CdkModifierType *mask);
CDK_AVAILABLE_IN_ALL
CdkWindow *   cdk_window_get_parent      (CdkWindow       *window);
CDK_AVAILABLE_IN_ALL
CdkWindow *   cdk_window_get_toplevel    (CdkWindow       *window);

CDK_AVAILABLE_IN_ALL
CdkWindow *   cdk_window_get_effective_parent   (CdkWindow *window);
CDK_AVAILABLE_IN_ALL
CdkWindow *   cdk_window_get_effective_toplevel (CdkWindow *window);

CDK_AVAILABLE_IN_ALL
GList *	      cdk_window_get_children	 (CdkWindow	  *window);
CDK_AVAILABLE_IN_ALL
GList *       cdk_window_peek_children   (CdkWindow       *window);
CDK_AVAILABLE_IN_3_10
GList *       cdk_window_get_children_with_user_data (CdkWindow *window,
						      gpointer   user_data);

CDK_AVAILABLE_IN_ALL
CdkEventMask  cdk_window_get_events	 (CdkWindow	  *window);
CDK_AVAILABLE_IN_ALL
void	      cdk_window_set_events	 (CdkWindow	  *window,
					  CdkEventMask	   event_mask);
CDK_AVAILABLE_IN_ALL
void          cdk_window_set_device_events (CdkWindow    *window,
                                            CdkDevice    *device,
                                            CdkEventMask  event_mask);
CDK_AVAILABLE_IN_ALL
CdkEventMask  cdk_window_get_device_events (CdkWindow    *window,
                                            CdkDevice    *device);

CDK_AVAILABLE_IN_ALL
void          cdk_window_set_source_events (CdkWindow      *window,
                                            CdkInputSource  source,
                                            CdkEventMask    event_mask);
CDK_AVAILABLE_IN_ALL
CdkEventMask  cdk_window_get_source_events (CdkWindow      *window,
                                            CdkInputSource  source);

CDK_AVAILABLE_IN_ALL
void          cdk_window_set_icon_list   (CdkWindow       *window,
					  GList           *pixbufs);
CDK_AVAILABLE_IN_ALL
void	      cdk_window_set_icon_name	 (CdkWindow	  *window, 
					  const gchar	  *name);
CDK_AVAILABLE_IN_ALL
void	      cdk_window_set_group	 (CdkWindow	  *window, 
					  CdkWindow	  *leader);
CDK_AVAILABLE_IN_ALL
CdkWindow*    cdk_window_get_group	 (CdkWindow	  *window);
CDK_AVAILABLE_IN_ALL
void	      cdk_window_set_decorations (CdkWindow	  *window,
					  CdkWMDecoration  decorations);
CDK_AVAILABLE_IN_ALL
gboolean      cdk_window_get_decorations (CdkWindow       *window,
					  CdkWMDecoration *decorations);
CDK_AVAILABLE_IN_ALL
void	      cdk_window_set_functions	 (CdkWindow	  *window,
					  CdkWMFunction	   functions);

CDK_AVAILABLE_IN_ALL
cairo_surface_t *
              cdk_window_create_similar_surface (CdkWindow *window,
                                          cairo_content_t  content,
                                          int              width,
                                          int              height);
CDK_AVAILABLE_IN_3_10
cairo_surface_t *
              cdk_window_create_similar_image_surface (CdkWindow *window,
						       cairo_format_t format,
						       int            width,
						       int            height,
						       int            scale);

CDK_AVAILABLE_IN_ALL
void          cdk_window_beep            (CdkWindow       *window);
CDK_AVAILABLE_IN_ALL
void          cdk_window_iconify         (CdkWindow       *window);
CDK_AVAILABLE_IN_ALL
void          cdk_window_deiconify       (CdkWindow       *window);
CDK_AVAILABLE_IN_ALL
void          cdk_window_stick           (CdkWindow       *window);
CDK_AVAILABLE_IN_ALL
void          cdk_window_unstick         (CdkWindow       *window);
CDK_AVAILABLE_IN_ALL
void          cdk_window_maximize        (CdkWindow       *window);
CDK_AVAILABLE_IN_ALL
void          cdk_window_unmaximize      (CdkWindow       *window);
CDK_AVAILABLE_IN_ALL
void          cdk_window_fullscreen      (CdkWindow       *window);
CDK_AVAILABLE_IN_3_18
void          cdk_window_fullscreen_on_monitor (CdkWindow      *window,
                                                gint            monitor);
CDK_AVAILABLE_IN_3_8
void          cdk_window_set_fullscreen_mode (CdkWindow   *window,
                                          CdkFullscreenMode mode);
CDK_AVAILABLE_IN_3_8
CdkFullscreenMode
              cdk_window_get_fullscreen_mode (CdkWindow   *window);
CDK_AVAILABLE_IN_ALL
void          cdk_window_unfullscreen    (CdkWindow       *window);
CDK_AVAILABLE_IN_ALL
void          cdk_window_set_keep_above  (CdkWindow       *window,
                                          gboolean         setting);
CDK_AVAILABLE_IN_ALL
void          cdk_window_set_keep_below  (CdkWindow       *window,
                                          gboolean         setting);
CDK_AVAILABLE_IN_ALL
void          cdk_window_set_opacity     (CdkWindow       *window,
                                          gdouble          opacity);
CDK_AVAILABLE_IN_ALL
void          cdk_window_register_dnd    (CdkWindow       *window);

CDK_AVAILABLE_IN_ALL
CdkDragProtocol
              cdk_window_get_drag_protocol(CdkWindow      *window,
                                           CdkWindow     **target);

CDK_AVAILABLE_IN_ALL
void cdk_window_begin_resize_drag            (CdkWindow     *window,
                                              CdkWindowEdge  edge,
                                              gint           button,
                                              gint           root_x,
                                              gint           root_y,
                                              guint32        timestamp);
CDK_AVAILABLE_IN_3_4
void cdk_window_begin_resize_drag_for_device (CdkWindow     *window,
                                              CdkWindowEdge  edge,
                                              CdkDevice     *device,
                                              gint           button,
                                              gint           root_x,
                                              gint           root_y,
                                              guint32        timestamp);
CDK_AVAILABLE_IN_ALL
void cdk_window_begin_move_drag              (CdkWindow     *window,
                                              gint           button,
                                              gint           root_x,
                                              gint           root_y,
                                              guint32        timestamp);
CDK_AVAILABLE_IN_3_4
void cdk_window_begin_move_drag_for_device   (CdkWindow     *window,
                                              CdkDevice     *device,
                                              gint           button,
                                              gint           root_x,
                                              gint           root_y,
                                              guint32        timestamp);

/* Interface for dirty-region queueing */
CDK_AVAILABLE_IN_ALL
void       cdk_window_invalidate_rect           (CdkWindow          *window,
					         const CdkRectangle *rect,
					         gboolean            invalidate_children);
CDK_AVAILABLE_IN_ALL
void       cdk_window_invalidate_region         (CdkWindow          *window,
					         const cairo_region_t    *region,
					         gboolean            invalidate_children);

/**
 * CdkWindowChildFunc:
 * @window: a #CdkWindow
 * @user_data: user data
 *
 * A function of this type is passed to cdk_window_invalidate_maybe_recurse().
 * It gets called for each child of the window to determine whether to
 * recursively invalidate it or now.
 *
 * Returns: %TRUE to invalidate @window recursively
 */
typedef gboolean (*CdkWindowChildFunc)          (CdkWindow *window,
                                                 gpointer   user_data);

CDK_AVAILABLE_IN_ALL
void       cdk_window_invalidate_maybe_recurse  (CdkWindow            *window,
						 const cairo_region_t *region,
						 CdkWindowChildFunc    child_func,
						 gpointer              user_data);
CDK_AVAILABLE_IN_ALL
cairo_region_t *cdk_window_get_update_area      (CdkWindow            *window);

CDK_AVAILABLE_IN_ALL
void       cdk_window_freeze_updates      (CdkWindow    *window);
CDK_AVAILABLE_IN_ALL
void       cdk_window_thaw_updates        (CdkWindow    *window);

CDK_DEPRECATED_IN_3_16
void       cdk_window_freeze_toplevel_updates_libctk_only (CdkWindow *window);
CDK_DEPRECATED_IN_3_16
void       cdk_window_thaw_toplevel_updates_libctk_only   (CdkWindow *window);

CDK_AVAILABLE_IN_ALL
void       cdk_window_process_all_updates (void);
CDK_AVAILABLE_IN_ALL
void       cdk_window_process_updates     (CdkWindow    *window,
					   gboolean      update_children);

/* Enable/disable flicker, so you can tell if your code is inefficient. */
CDK_AVAILABLE_IN_ALL
void       cdk_window_set_debug_updates   (gboolean      setting);

CDK_AVAILABLE_IN_ALL
void       cdk_window_constrain_size      (CdkGeometry    *geometry,
                                           CdkWindowHints  flags,
                                           gint            width,
                                           gint            height,
                                           gint           *new_width,
                                           gint           *new_height);

CDK_DEPRECATED_IN_3_8
void cdk_window_enable_synchronized_configure (CdkWindow *window);
CDK_DEPRECATED_IN_3_8
void cdk_window_configure_finished            (CdkWindow *window);

CDK_AVAILABLE_IN_ALL
CdkWindow *cdk_get_default_root_window (void);

/* Offscreen redirection */
CDK_AVAILABLE_IN_ALL
cairo_surface_t *
           cdk_offscreen_window_get_surface    (CdkWindow     *window);
CDK_AVAILABLE_IN_ALL
void       cdk_offscreen_window_set_embedder   (CdkWindow     *window,
						CdkWindow     *embedder);
CDK_AVAILABLE_IN_ALL
CdkWindow *cdk_offscreen_window_get_embedder   (CdkWindow     *window);
CDK_AVAILABLE_IN_ALL
void       cdk_window_geometry_changed         (CdkWindow     *window);

/* Multidevice support */
CDK_AVAILABLE_IN_ALL
void       cdk_window_set_support_multidevice (CdkWindow *window,
                                               gboolean   support_multidevice);
CDK_AVAILABLE_IN_ALL
gboolean   cdk_window_get_support_multidevice (CdkWindow *window);

/* Frame clock */
CDK_AVAILABLE_IN_3_8
CdkFrameClock* cdk_window_get_frame_clock      (CdkWindow     *window);

CDK_AVAILABLE_IN_3_10
void       cdk_window_set_opaque_region        (CdkWindow      *window,
                                                cairo_region_t *region);

CDK_AVAILABLE_IN_3_12
void       cdk_window_set_event_compression    (CdkWindow      *window,
                                                gboolean        event_compression);
CDK_AVAILABLE_IN_3_12
gboolean   cdk_window_get_event_compression    (CdkWindow      *window);

CDK_AVAILABLE_IN_3_12
void       cdk_window_set_shadow_width         (CdkWindow      *window,
                                                gint            left,
                                                gint            right,
                                                gint            top,
                                                gint            bottom);
CDK_AVAILABLE_IN_3_14
gboolean  cdk_window_show_window_menu          (CdkWindow      *window,
                                                CdkEvent       *event);

CDK_AVAILABLE_IN_3_16
CdkGLContext * cdk_window_create_gl_context    (CdkWindow      *window,
                                                GError        **error);

G_END_DECLS

#endif /* __CDK_WINDOW_H__ */
