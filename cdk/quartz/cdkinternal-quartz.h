/* cdkinternal-quartz.h
 *
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

#ifndef __CDK_INTERNAL_QUARTZ_H__
#define __CDK_INTERNAL_QUARTZ_H__

#include <AppKit/AppKit.h>

/* This is mostly a pot of function prototypes to avoid having
 * separate include file for each implementation file that exports
 * functions to one other file in CdkQuartz.
 */

/* NSInteger only exists in Leopard and newer.  This check has to be
 * done after inclusion of the system headers.  If NSInteger has not
 * been defined, we know for sure that we are on 32-bit.
 */
#ifndef NSINTEGER_DEFINED
typedef int NSInteger;
typedef unsigned int NSUInteger;
#endif

#ifndef CGFLOAT_DEFINED
typedef float CGFloat;
#endif

#define CDK_QUARTZ_ALLOC_POOL NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init]
#define CDK_QUARTZ_RELEASE_POOL [pool release]

#include <cdk/cdkprivate.h>
#include <cdk/quartz/cdkquartz.h>
#include <cdk/quartz/cdkdevicemanager-core-quartz.h>
#include <cdk/quartz/cdkdnd-quartz.h>
#include <cdk/quartz/cdkscreen-quartz.h>
#include <cdk/quartz/cdkwindow-quartz.h>

#include <cdk/cdk.h>

#include "config.h"

extern CdkDisplay *_cdk_display;
extern CdkScreen *_cdk_screen;
extern CdkWindow *_cdk_root;

extern CdkDragContext *_cdk_quartz_drag_source_context;

#define CDK_WINDOW_IS_QUARTZ(win)        (CDK_IS_WINDOW_IMPL_QUARTZ (((CdkWindow *)win)->impl))

/* Initialization */
void _cdk_quartz_window_init_windowing      (CdkDisplay *display,
                                             CdkScreen  *screen);
void _cdk_quartz_events_init                (void);
void _cdk_quartz_event_loop_init            (void);

/* Cursor */
NSCursor   *_cdk_quartz_cursor_get_ns_cursor        (CdkCursor *cursor);

/* Events */
typedef enum {
  CDK_QUARTZ_EVENT_SUBTYPE_EVENTLOOP
} CdkQuartzEventSubType;

#if MAC_OS_X_VERSION_MIN_REQUIRED >= 10130
#define CDK_QUARTZ_EVENT_TABLET_PROXIMITY NSEventTypeTabletProximity
#define CDK_QUARTZ_EVENT_SUBTYPE_TABLET_PROXIMITY NSEventSubtypeTabletProximity
#define CDK_QUARTZ_EVENT_SUBTYPE_TABLET_POINT NSEventSubtypeTabletPoint
#else
#define CDK_QUARTZ_EVENT_TABLET_PROXIMITY NSTabletProximity
#define CDK_QUARTZ_EVENT_SUBTYPE_TABLET_PROXIMITY NSTabletProximityEventSubtype
#define CDK_QUARTZ_EVENT_SUBTYPE_TABLET_POINT NSTabletPointEventSubtype
#endif

void         _cdk_quartz_events_update_focus_window    (CdkWindow *new_window,
                                                        gboolean   got_focus);
void         _cdk_quartz_events_send_map_event         (CdkWindow *window);

CdkModifierType _cdk_quartz_events_get_current_keyboard_modifiers (void);
CdkModifierType _cdk_quartz_events_get_current_mouse_modifiers    (void);

void         _cdk_quartz_events_break_all_grabs         (guint32    time);

/* Devices */
void       _cdk_quartz_device_core_set_active (CdkDevice  *device,
                                               gboolean    active,
                                               NSUInteger  device_id);

gboolean   _cdk_quartz_device_core_is_active (CdkDevice  *device,
                                              NSUInteger  device_id);

void       _cdk_quartz_device_core_set_unique (CdkDevice          *device,
                                               unsigned long long  unique_id);

unsigned long long _cdk_quartz_device_core_get_unique (CdkDevice *device);

/* Event loop */
gboolean   _cdk_quartz_event_loop_check_pending (void);
NSEvent *  _cdk_quartz_event_loop_get_pending   (void);
void       _cdk_quartz_event_loop_release_event (NSEvent *event);

/* Keys */
CdkEventType _cdk_quartz_keys_event_type  (NSEvent   *event);
gboolean     _cdk_quartz_keys_is_modifier (guint      keycode);
void         _cdk_quartz_synthesize_null_key_event (CdkWindow *window);

/* Drag and Drop */
void        _cdk_quartz_window_register_dnd      (CdkWindow   *window);
CdkDragContext * _cdk_quartz_window_drag_begin   (CdkWindow   *window,
                                                  CdkDevice   *device,
                                                  GList       *targets,
                                                  gint         x_root,
                                                  gint         y_root);

/* Display */

CdkDisplay *    _cdk_quartz_display_open (const gchar *name);

/* Display methods - events */
void     _cdk_quartz_display_queue_events (CdkDisplay *display);
gboolean _cdk_quartz_display_has_pending  (CdkDisplay *display);

void       _cdk_quartz_display_event_data_copy (CdkDisplay     *display,
                                                const CdkEvent *src,
                                                CdkEvent       *dst);
void       _cdk_quartz_display_event_data_free (CdkDisplay     *display,
                                                CdkEvent       *event);

/* Display methods - cursor */
CdkCursor *_cdk_quartz_display_get_cursor_for_type     (CdkDisplay      *display,
                                                        CdkCursorType    type);
CdkCursor *_cdk_quartz_display_get_cursor_for_name     (CdkDisplay      *display,
                                                        const gchar     *name);
CdkCursor *_cdk_quartz_display_get_cursor_for_surface  (CdkDisplay      *display,
                                                        cairo_surface_t *surface,
                                                        gdouble          x,
                                                        gdouble          y);
gboolean   _cdk_quartz_display_supports_cursor_alpha   (CdkDisplay    *display);
gboolean   _cdk_quartz_display_supports_cursor_color   (CdkDisplay    *display);
void       _cdk_quartz_display_get_default_cursor_size (CdkDisplay *display,
                                                        guint      *width,
                                                        guint      *height);
void       _cdk_quartz_display_get_maximal_cursor_size (CdkDisplay *display,
                                                        guint      *width,
                                                        guint      *height);

/* Display methods - keymap */
CdkKeymap * _cdk_quartz_display_get_keymap (CdkDisplay *display);

/* Display methods - selection */
gboolean    _cdk_quartz_display_set_selection_owner (CdkDisplay *display,
                                                     CdkWindow  *owner,
                                                     CdkAtom     selection,
                                                     guint32     time,
                                                     gboolean    send_event);
CdkWindow * _cdk_quartz_display_get_selection_owner (CdkDisplay *display,
                                                     CdkAtom     selection);
gint        _cdk_quartz_display_get_selection_property (CdkDisplay     *display,
                                                        CdkWindow      *requestor,
                                                        guchar        **data,
                                                        CdkAtom        *ret_type,
                                                        gint           *ret_format);
void        _cdk_quartz_display_convert_selection      (CdkDisplay     *display,
                                                        CdkWindow      *requestor,
                                                        CdkAtom         selection,
                                                        CdkAtom         target,
                                                        guint32         time);
gint        _cdk_quartz_display_text_property_to_utf8_list (CdkDisplay     *display,
                                                            CdkAtom         encoding,
                                                            gint            format,
                                                            const guchar   *text,
                                                            gint            length,
                                                            gchar        ***list);
gchar *     _cdk_quartz_display_utf8_to_string_target      (CdkDisplay     *displayt,
                                                            const gchar    *str);

/* Screen */
CdkScreen  *_cdk_quartz_screen_new                      (void);
void        _cdk_quartz_screen_update_window_sizes      (CdkScreen *screen);

/* Screen methods - visual */
CdkVisual *   _cdk_quartz_screen_get_rgba_visual            (CdkScreen      *screen);
CdkVisual *   _cdk_quartz_screen_get_system_visual          (CdkScreen      *screen);
gint          _cdk_quartz_screen_visual_get_best_depth      (CdkScreen      *screen);
CdkVisualType _cdk_quartz_screen_visual_get_best_type       (CdkScreen      *screen);
CdkVisual *   _cdk_quartz_screen_get_system_visual          (CdkScreen      *screen);
CdkVisual*    _cdk_quartz_screen_visual_get_best            (CdkScreen      *screen);
CdkVisual*    _cdk_quartz_screen_visual_get_best_with_depth (CdkScreen      *screen,
                                                             gint            depth);
CdkVisual*    _cdk_quartz_screen_visual_get_best_with_type  (CdkScreen      *screen,
                                                             CdkVisualType   visual_type);
CdkVisual*    _cdk_quartz_screen_visual_get_best_with_both  (CdkScreen      *screen,
                                                             gint            depth,
                                                             CdkVisualType   visual_type);
void          _cdk_quartz_screen_query_depths               (CdkScreen      *screen,
                                                             gint          **depths,
                                                             gint           *count);
void          _cdk_quartz_screen_query_visual_types         (CdkScreen      *screen,
                                                             CdkVisualType **visual_types,
                                                             gint           *count);
void          _cdk_quartz_screen_init_visuals               (CdkScreen      *screen);
GList *       _cdk_quartz_screen_list_visuals               (CdkScreen      *screen);

/* Screen methods - events */
void        _cdk_quartz_screen_broadcast_client_message (CdkScreen   *screen,
                                                         CdkEvent    *event);
gboolean    _cdk_quartz_screen_get_setting              (CdkScreen   *screen,
                                                         const gchar *name,
                                                         GValue      *value);

gboolean    _cdk_quartz_window_is_ancestor          (CdkWindow *ancestor,
                                                     CdkWindow *window);
void       _cdk_quartz_window_cdk_xy_to_xy          (gint       cdk_x,
                                                     gint       cdk_y,
                                                     gint      *ns_x,
                                                     gint      *ns_y);
void       _cdk_quartz_window_xy_to_cdk_xy          (gint       ns_x,
                                                     gint       ns_y,
                                                     gint      *cdk_x,
                                                     gint      *cdk_y);
void       _cdk_quartz_window_nspoint_to_cdk_xy     (NSPoint    point,
                                                     gint      *x,
                                                     gint      *y);
CdkWindow *_cdk_quartz_window_find_child            (CdkWindow *window,
                                                     gint       x,
                                                     gint       y,
                                                     gboolean   get_toplevel);
void       _cdk_quartz_window_attach_to_parent      (CdkWindow *window);
void       _cdk_quartz_window_detach_from_parent    (CdkWindow *window);
void       _cdk_quartz_window_did_become_main       (CdkWindow *window);
void       _cdk_quartz_window_did_resign_main       (CdkWindow *window);
void       _cdk_quartz_window_debug_highlight       (CdkWindow *window,
                                                     gint       number);

void       _cdk_quartz_window_update_position           (CdkWindow    *window);
void       _cdk_quartz_window_update_fullscreen_state   (CdkWindow    *window);

/* Window methods - testing */
void     _cdk_quartz_window_sync_rendering    (CdkWindow       *window);
gboolean _cdk_quartz_window_simulate_key      (CdkWindow       *window,
                                               gint             x,
                                               gint             y,
                                               guint            keyval,
                                               CdkModifierType  modifiers,
                                               CdkEventType     key_pressrelease);
gboolean _cdk_quartz_window_simulate_button   (CdkWindow       *window,
                                               gint             x,
                                               gint             y,
                                               guint            button,
                                               CdkModifierType  modifiers,
                                               CdkEventType     button_pressrelease);

/* Window methods - property */
gboolean _cdk_quartz_window_get_property      (CdkWindow    *window,
                                               CdkAtom       property,
                                               CdkAtom       type,
                                               gulong        offset,
                                               gulong        length,
                                               gint          pdelete,
                                               CdkAtom      *actual_property_type,
                                               gint         *actual_format_type,
                                               gint         *actual_length,
                                               guchar      **data);
void     _cdk_quartz_window_change_property   (CdkWindow    *window,
                                               CdkAtom       property,
                                               CdkAtom       type,
                                               gint          format,
                                               CdkPropMode   mode,
                                               const guchar *data,
                                               gint          nelements);
void     _cdk_quartz_window_delete_property   (CdkWindow    *window,
                                               CdkAtom       property);


#endif /* __CDK_INTERNAL_QUARTZ_H__ */
