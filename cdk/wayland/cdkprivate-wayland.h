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

/*
 * Private uninstalled header defining things local to the Wayland backend
 */

#ifndef __CDK_PRIVATE_WAYLAND_H__
#define __CDK_PRIVATE_WAYLAND_H__

#include <cdk/cdkcursor.h>
#include <cdk/cdkprivate.h>
#include <cdk/wayland/cdkwayland.h>
#include <cdk/wayland/cdkdisplay-wayland.h>

#include <xkbcommon/xkbcommon.h>

#include "cdkinternals.h"
#include "wayland/ctk-primary-selection-client-protocol.h"

#include "config.h"

#define WL_SURFACE_HAS_BUFFER_SCALE 3
#define WL_POINTER_HAS_FRAME 5

#define CDK_WINDOW_IS_WAYLAND(win)    (CDK_IS_WINDOW_IMPL_WAYLAND (((CdkWindow *)win)->impl))

CdkKeymap *_cdk_wayland_keymap_new (void);
void       _cdk_wayland_keymap_update_from_fd (CdkKeymap *keymap,
                                               uint32_t   format,
                                               uint32_t   fd,
                                               uint32_t   size);
struct xkb_state *_cdk_wayland_keymap_get_xkb_state (CdkKeymap *keymap);
struct xkb_keymap *_cdk_wayland_keymap_get_xkb_keymap (CdkKeymap *keymap);
gboolean           _cdk_wayland_keymap_key_is_modifier (CdkKeymap *keymap,
                                                        guint      keycode);

void       _cdk_wayland_display_init_cursors (CdkWaylandDisplay *display);
void       _cdk_wayland_display_finalize_cursors (CdkWaylandDisplay *display);
void       _cdk_wayland_display_update_cursors (CdkWaylandDisplay *display);

struct wl_cursor_theme * _cdk_wayland_display_get_scaled_cursor_theme (CdkWaylandDisplay *display_wayland,
                                                                       guint              scale);

CdkCursor *_cdk_wayland_display_get_cursor_for_type (CdkDisplay    *display,
						     CdkCursorType  cursor_type);
CdkCursor *_cdk_wayland_display_get_cursor_for_type_with_scale (CdkDisplay    *display,
                                                                CdkCursorType  cursor_type,
                                                                guint          scale);
CdkCursor *_cdk_wayland_display_get_cursor_for_name (CdkDisplay  *display,
						     const gchar *name);
CdkCursor *_cdk_wayland_display_get_cursor_for_surface (CdkDisplay *display,
							cairo_surface_t *surface,
							gdouble     x,
							gdouble     y);
void       _cdk_wayland_display_get_default_cursor_size (CdkDisplay *display,
							 guint       *width,
							 guint       *height);
void       _cdk_wayland_display_get_maximal_cursor_size (CdkDisplay *display,
							 guint       *width,
							 guint       *height);
gboolean   _cdk_wayland_display_supports_cursor_alpha (CdkDisplay *display);
gboolean   _cdk_wayland_display_supports_cursor_color (CdkDisplay *display);

void       cdk_wayland_display_system_bell (CdkDisplay *display,
                                            CdkWindow  *window);

struct wl_buffer *_cdk_wayland_cursor_get_buffer (CdkCursor *cursor,
                                                  guint      image_index,
                                                  int       *hotspot_x,
                                                  int       *hotspot_y,
                                                  int       *w,
                                                  int       *h,
						  int       *scale);
guint      _cdk_wayland_cursor_get_next_image_index (CdkCursor *cursor,
                                                     guint      current_image_index,
                                                     guint     *next_image_delay);

void       _cdk_wayland_cursor_set_scale (CdkCursor *cursor,
                                          guint      scale);

CdkDragProtocol _cdk_wayland_window_get_drag_protocol (CdkWindow *window,
						       CdkWindow **target);

void            _cdk_wayland_window_register_dnd (CdkWindow *window);
CdkDragContext *_cdk_wayland_window_drag_begin (CdkWindow *window,
						CdkDevice *device,
						GList     *targets,
                                                gint       x_root,
                                                gint       y_root);
void            _cdk_wayland_window_offset_next_wl_buffer (CdkWindow *window,
                                                           int        x,
                                                           int        y);
CdkDragContext * _cdk_wayland_drop_context_new (CdkDisplay            *display,
                                                struct wl_data_device *data_device);
void _cdk_wayland_drag_context_set_source_window (CdkDragContext *context,
                                                  CdkWindow      *window);
void _cdk_wayland_drag_context_set_dest_window (CdkDragContext *context,
                                                CdkWindow      *dest_window,
                                                uint32_t        serial);
void _cdk_wayland_drag_context_emit_event (CdkDragContext *context,
                                           CdkEventType    type,
                                           guint32         time_);
void _cdk_wayland_drag_context_set_coords (CdkDragContext *context,
                                           gdouble         x,
                                           gdouble         y);

void cdk_wayland_drag_context_set_action (CdkDragContext *context,
                                          CdkDragAction   action);

CdkDragContext * cdk_wayland_drag_context_lookup_by_data_source   (struct wl_data_source *source);
CdkDragContext * cdk_wayland_drag_context_lookup_by_source_window (CdkWindow *window);
struct wl_data_source * cdk_wayland_drag_context_get_data_source  (CdkDragContext *context);

void cdk_wayland_drop_context_update_targets (CdkDragContext *context);

void _cdk_wayland_display_create_window_impl (CdkDisplay    *display,
					      CdkWindow     *window,
					      CdkWindow     *real_parent,
					      CdkScreen     *screen,
					      CdkEventMask   event_mask,
					      CdkWindowAttr *attributes,
					      gint           attributes_mask);

CdkWindow *_cdk_wayland_display_get_selection_owner (CdkDisplay *display,
						 CdkAtom     selection);
gboolean   _cdk_wayland_display_set_selection_owner (CdkDisplay *display,
						     CdkWindow  *owner,
						     CdkAtom     selection,
						     guint32     time,
						     gboolean    send_event);
void       _cdk_wayland_display_send_selection_notify (CdkDisplay *dispay,
						       CdkWindow        *requestor,
						       CdkAtom          selection,
						       CdkAtom          target,
						       CdkAtom          property,
						       guint32          time);
gint       _cdk_wayland_display_get_selection_property (CdkDisplay  *display,
							CdkWindow   *requestor,
							guchar     **data,
							CdkAtom     *ret_type,
							gint        *ret_format);
void       _cdk_wayland_display_convert_selection (CdkDisplay *display,
						   CdkWindow  *requestor,
						   CdkAtom     selection,
						   CdkAtom     target,
						   guint32     time);
gint        _cdk_wayland_display_text_property_to_utf8_list (CdkDisplay    *display,
							     CdkAtom        encoding,
							     gint           format,
							     const guchar  *text,
							     gint           length,
							     gchar       ***list);
gchar *     _cdk_wayland_display_utf8_to_string_target (CdkDisplay  *display,
							const gchar *str);

CdkDeviceManager *_cdk_wayland_device_manager_new (CdkDisplay *display);
void              _cdk_wayland_device_manager_add_seat (CdkDeviceManager *device_manager,
                                                        guint32           id,
						        struct wl_seat   *seat);
void              _cdk_wayland_device_manager_remove_seat (CdkDeviceManager *device_manager,
                                                           guint32           id);

CdkKeymap *_cdk_wayland_device_get_keymap (CdkDevice *device);
uint32_t _cdk_wayland_device_get_implicit_grab_serial(CdkWaylandDevice *device,
                                                      const CdkEvent   *event);
uint32_t _cdk_wayland_seat_get_last_implicit_grab_serial (CdkSeat           *seat,
                                                          CdkEventSequence **seqence);
struct wl_data_device * cdk_wayland_device_get_data_device (CdkDevice *cdk_device);
void cdk_wayland_seat_set_selection (CdkSeat               *seat,
                                     struct wl_data_source *source);

void cdk_wayland_seat_set_primary (CdkSeat  *seat,
                                   gpointer  source);

CdkDragContext * cdk_wayland_device_get_drop_context (CdkDevice *cdk_device);

void cdk_wayland_device_unset_touch_grab (CdkDevice        *device,
                                          CdkEventSequence *sequence);

void     _cdk_wayland_display_deliver_event (CdkDisplay *display, CdkEvent *event);
GSource *_cdk_wayland_display_event_source_new (CdkDisplay *display);
void     _cdk_wayland_display_queue_events (CdkDisplay *display);

CdkAppLaunchContext *_cdk_wayland_display_get_app_launch_context (CdkDisplay *display);

CdkDisplay *_cdk_wayland_display_open (const gchar *display_name);

CdkWindow *_cdk_wayland_screen_create_root_window (CdkScreen *screen,
						   int width,
						   int height);

CdkScreen *_cdk_wayland_screen_new (CdkDisplay *display);
void _cdk_wayland_screen_add_output (CdkScreen        *screen,
                                     guint32           id,
                                     struct wl_output *output,
				     guint32           version);
void _cdk_wayland_screen_remove_output (CdkScreen *screen,
                                        guint32 id);
int _cdk_wayland_screen_get_output_refresh_rate (CdkScreen        *screen,
                                                 struct wl_output *output);
guint32 _cdk_wayland_screen_get_output_scale (CdkScreen        *screen,
					      struct wl_output *output);
struct wl_output *_cdk_wayland_screen_get_wl_output (CdkScreen *screen,
                                                     gint monitor_num);

void _cdk_wayland_screen_set_has_ctk_shell (CdkScreen       *screen);

void _cdk_wayland_screen_init_xdg_output (CdkScreen *screen);

void _cdk_wayland_window_set_grab_seat (CdkWindow      *window,
                                        CdkSeat        *seat);

guint32 _cdk_wayland_display_get_serial (CdkWaylandDisplay *display_wayland);
void _cdk_wayland_display_update_serial (CdkWaylandDisplay *display_wayland,
                                         guint32            serial);

cairo_surface_t * _cdk_wayland_display_create_shm_surface (CdkWaylandDisplay *display,
                                                           int                width,
                                                           int                height,
                                                           guint              scale);
struct wl_buffer *_cdk_wayland_shm_surface_get_wl_buffer (cairo_surface_t *surface);
gboolean _cdk_wayland_is_shm_surface (cairo_surface_t *surface);

CdkWaylandSelection * cdk_wayland_display_get_selection (CdkDisplay *display);
CdkWaylandSelection * cdk_wayland_selection_new (void);
void cdk_wayland_selection_free (CdkWaylandSelection *selection);

void cdk_wayland_selection_ensure_offer (CdkDisplay           *display,
                                         struct wl_data_offer *wl_offer);
void cdk_wayland_selection_ensure_primary_offer (CdkDisplay *display,
                                                 gpointer    wp_offer);

void cdk_wayland_selection_set_offer (CdkDisplay           *display,
                                      CdkAtom               selection,
                                      gpointer              offer);
gpointer cdk_wayland_selection_get_offer (CdkDisplay *display,
                                          CdkAtom     selection);
GList * cdk_wayland_selection_get_targets (CdkDisplay *display,
                                           CdkAtom     selection);

void     cdk_wayland_selection_store   (CdkWindow    *window,
                                        CdkAtom       type,
                                        CdkPropMode   mode,
                                        const guchar *data,
                                        gint          len);
struct wl_data_source * cdk_wayland_selection_get_data_source (CdkWindow *owner,
                                                               CdkAtom    selection);
void cdk_wayland_selection_unset_data_source (CdkDisplay *display, CdkAtom selection);
gboolean cdk_wayland_selection_set_current_offer_actions (CdkDisplay *display,
                                                          uint32_t    actions);

EGLSurface cdk_wayland_window_get_egl_surface (CdkWindow *window,
                                               EGLConfig config);
EGLSurface cdk_wayland_window_get_dummy_egl_surface (CdkWindow *window,
						     EGLConfig config);

struct ctk_surface1 * cdk_wayland_window_get_ctk_surface (CdkWindow *window);

void cdk_wayland_seat_set_global_cursor (CdkSeat   *seat,
                                         CdkCursor *cursor);

struct wl_output *cdk_wayland_window_get_wl_output (CdkWindow *window);

void cdk_wayland_window_inhibit_shortcuts (CdkWindow *window,
                                           CdkSeat   *cdk_seat);
void cdk_wayland_window_restore_shortcuts (CdkWindow *window,
                                           CdkSeat   *cdk_seat);

#endif /* __CDK_PRIVATE_WAYLAND_H__ */
