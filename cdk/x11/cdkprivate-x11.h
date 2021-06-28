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

/*
 * Private uninstalled header defining things local to X windowing code
 */

#ifndef __GDK_PRIVATE_X11_H__
#define __GDK_PRIVATE_X11_H__

#include "cdkcursor.h"
#include "cdkprivate.h"
#include "cdkinternals.h"
#include "cdkx.h"
#include "cdkwindow-x11.h"
#include "cdkscreen-x11.h"
#include "cdkdisplay-x11.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#ifdef XINPUT_2
#include <X11/extensions/XInput2.h>
#endif

#include <cairo-xlib.h>

void _cdk_x11_error_handler_push (void);
void _cdk_x11_error_handler_pop  (void);

Colormap _cdk_visual_get_x11_colormap (GdkVisual *visual);

gint          _cdk_x11_screen_visual_get_best_depth      (GdkScreen      *screen);
GdkVisualType _cdk_x11_screen_visual_get_best_type       (GdkScreen      *screen);
GdkVisual *   _cdk_x11_screen_get_system_visual          (GdkScreen      *screen);
GdkVisual*    _cdk_x11_screen_visual_get_best            (GdkScreen      *screen);
GdkVisual*    _cdk_x11_screen_visual_get_best_with_depth (GdkScreen      *screen,
                                                          gint            depth);
GdkVisual*    _cdk_x11_screen_visual_get_best_with_type  (GdkScreen      *screen,
                                                          GdkVisualType   visual_type);
GdkVisual*    _cdk_x11_screen_visual_get_best_with_both  (GdkScreen      *screen,
                                                          gint            depth,
                                                          GdkVisualType   visual_type);
void          _cdk_x11_screen_query_depths               (GdkScreen      *screen,
                                                          gint          **depths,
                                                          gint           *count);
void          _cdk_x11_screen_query_visual_types         (GdkScreen      *screen,
                                                          GdkVisualType **visual_types,
                                                          gint           *count);
GList *       _cdk_x11_screen_list_visuals               (GdkScreen      *screen);



void _cdk_x11_display_add_window    (GdkDisplay *display,
                                     XID        *xid,
                                     GdkWindow  *window);
void _cdk_x11_display_remove_window (GdkDisplay *display,
                                     XID         xid);

gint _cdk_x11_display_send_xevent (GdkDisplay *display,
                                   Window      window,
                                   gboolean    propagate,
                                   glong       event_mask,
                                   XEvent     *event_send);

/* Routines from cdkgeometry-x11.c */
void _cdk_x11_window_move_resize_child (GdkWindow     *window,
                                        gint           x,
                                        gint           y,
                                        gint           width,
                                        gint           height);
void _cdk_x11_window_process_expose    (GdkWindow     *window,
                                        gulong         serial,
                                        GdkRectangle  *area);

void     _cdk_x11_window_sync_rendering    (GdkWindow       *window);
gboolean _cdk_x11_window_simulate_key      (GdkWindow       *window,
                                            gint             x,
                                            gint             y,
                                            guint            keyval,
                                            GdkModifierType  modifiers,
                                            GdkEventType     key_pressrelease);
gboolean _cdk_x11_window_simulate_button   (GdkWindow       *window,
                                            gint             x,
                                            gint             y,
                                            guint            button,
                                            GdkModifierType  modifiers,
                                            GdkEventType     button_pressrelease);
gboolean _cdk_x11_window_get_property      (GdkWindow    *window,
                                            GdkAtom       property,
                                            GdkAtom       type,
                                            gulong        offset,
                                            gulong        length,
                                            gint          pdelete,
                                            GdkAtom      *actual_property_type,
                                            gint         *actual_format_type,
                                            gint         *actual_length,
                                            guchar      **data);
void     _cdk_x11_window_change_property   (GdkWindow    *window,
                                            GdkAtom       property,
                                            GdkAtom       type,
                                            gint          format,
                                            GdkPropMode   mode,
                                            const guchar *data,
                                            gint          nelements);
void     _cdk_x11_window_delete_property   (GdkWindow    *window,
                                            GdkAtom       property);

void     _cdk_x11_window_queue_antiexpose  (GdkWindow *window,
                                            cairo_region_t *area);
void     _cdk_x11_window_translate         (GdkWindow *window,
                                            cairo_region_t *area,
                                            gint       dx,
                                            gint       dy);

void     _cdk_x11_display_free_translate_queue (GdkDisplay *display);

void     _cdk_x11_selection_window_destroyed   (GdkWindow            *window);
gboolean _cdk_x11_selection_filter_clear_event (XSelectionClearEvent *event);

cairo_region_t* _cdk_x11_xwindow_get_shape  (Display *xdisplay,
                                             Window   window,
                                             gint     scale,
                                             gint     shape_type);

void     _cdk_x11_region_get_xrectangles   (const cairo_region_t  *region,
                                            gint                   x_offset,
                                            gint                   y_offset,
                                            gint                   scale,
                                            XRectangle           **rects,
                                            gint                  *n_rects);

gboolean _cdk_x11_moveresize_handle_event   (XEvent     *event);
gboolean _cdk_x11_moveresize_configure_done (GdkDisplay *display,
                                             GdkWindow  *window);

void     _cdk_x11_keymap_state_changed   (GdkDisplay      *display,
                                          XEvent          *event);
void     _cdk_x11_keymap_keys_changed    (GdkDisplay      *display);
void     _cdk_x11_keymap_add_virt_mods   (GdkKeymap       *keymap,
                                          GdkModifierType *modifiers);

void _cdk_x11_windowing_init    (void);

void _cdk_x11_window_grab_check_unmap   (GdkWindow *window,
                                         gulong     serial);
void _cdk_x11_window_grab_check_destroy (GdkWindow *window);

gboolean _cdk_x11_display_is_root_window (GdkDisplay *display,
                                          Window      xroot_window);

GdkDisplay * _cdk_x11_display_open            (const gchar *display_name);
void _cdk_x11_display_update_grab_info        (GdkDisplay *display,
                                               GdkDevice  *device,
                                               gint        status);
void _cdk_x11_display_update_grab_info_ungrab (GdkDisplay *display,
                                               GdkDevice  *device,
                                               guint32     time,
                                               gulong      serial);
void _cdk_x11_display_queue_events            (GdkDisplay *display);


GdkAppLaunchContext *_cdk_x11_display_get_app_launch_context (GdkDisplay *display);
Window      _cdk_x11_display_get_drag_protocol     (GdkDisplay      *display,
                                                    Window           xid,
                                                    GdkDragProtocol *protocol,
                                                    guint           *version);

gboolean    _cdk_x11_display_set_selection_owner   (GdkDisplay *display,
                                                    GdkWindow  *owner,
                                                    GdkAtom     selection,
                                                    guint32     time,
                                                    gboolean    send_event);
GdkWindow * _cdk_x11_display_get_selection_owner   (GdkDisplay *display,
                                                    GdkAtom     selection);
void        _cdk_x11_display_send_selection_notify (GdkDisplay       *display,
                                                    GdkWindow        *requestor,
                                                    GdkAtom          selection,
                                                    GdkAtom          target,
                                                    GdkAtom          property,
                                                    guint32          time);
gint        _cdk_x11_display_get_selection_property (GdkDisplay     *display,
                                                     GdkWindow      *requestor,
                                                     guchar        **data,
                                                     GdkAtom        *ret_type,
                                                     gint           *ret_format);
void        _cdk_x11_display_convert_selection      (GdkDisplay     *display,
                                                     GdkWindow      *requestor,
                                                     GdkAtom         selection,
                                                     GdkAtom         target,
                                                     guint32         time);

gint        _cdk_x11_display_text_property_to_utf8_list (GdkDisplay     *display,
                                                         GdkAtom         encoding,
                                                         gint            format,
                                                         const guchar   *text,
                                                         gint            length,
                                                         gchar        ***list);
gchar *     _cdk_x11_display_utf8_to_string_target      (GdkDisplay     *displayt,
                                                         const gchar    *str);

void _cdk_x11_device_check_extension_events   (GdkDevice  *device);

GdkDeviceManager *_cdk_x11_device_manager_new (GdkDisplay *display);

#ifdef XINPUT_2
guchar * _cdk_x11_device_xi2_translate_event_mask (GdkX11DeviceManagerXI2 *device_manager_xi2,
                                                   GdkEventMask            event_mask,
                                                   gint                   *len);
guint    _cdk_x11_device_xi2_translate_state      (XIModifierState *mods_state,
                                                   XIButtonState   *buttons_state,
                                                   XIGroupState    *group_state);
gint     _cdk_x11_device_xi2_get_id               (GdkX11DeviceXI2 *device);
void     _cdk_device_xi2_unset_scroll_valuators   (GdkX11DeviceXI2 *device);


GdkDevice * _cdk_x11_device_manager_xi2_lookup    (GdkX11DeviceManagerXI2 *device_manager_xi2,
                                                   gint                    device_id);
void     _cdk_x11_device_xi2_add_scroll_valuator  (GdkX11DeviceXI2    *device,
                                                   guint               n_valuator,
                                                   GdkScrollDirection  direction,
                                                   gdouble             increment);
gboolean  _cdk_x11_device_xi2_get_scroll_delta    (GdkX11DeviceXI2    *device,
                                                   guint               n_valuator,
                                                   gdouble             valuator_value,
                                                   GdkScrollDirection *direction_ret,
                                                   gdouble            *delta_ret);
void     _cdk_device_xi2_reset_scroll_valuators   (GdkX11DeviceXI2    *device);

gdouble  cdk_x11_device_xi2_get_last_axis_value (GdkX11DeviceXI2 *device,
                                                 gint             n_axis);

void     cdk_x11_device_xi2_store_axes          (GdkX11DeviceXI2 *device,
                                                 gdouble         *axes,
                                                 gint             n_axes);
#endif

void     _cdk_x11_event_translate_keyboard_string (GdkEventKey *event);

GdkAtom _cdk_x11_display_manager_atom_intern   (GdkDisplayManager *manager,
                                                const gchar       *atom_name,
                                                gboolean           copy_name);
gchar * _cdk_x11_display_manager_get_atom_name (GdkDisplayManager *manager,
                                                GdkAtom            atom);

GdkCursor *_cdk_x11_display_get_cursor_for_type     (GdkDisplay    *display,
                                                     GdkCursorType  type);
GdkCursor *_cdk_x11_display_get_cursor_for_name     (GdkDisplay    *display,
                                                     const gchar   *name);
GdkCursor *_cdk_x11_display_get_cursor_for_surface  (GdkDisplay    *display,
                                                     cairo_surface_t *surface,
                                                     gdouble        x,
                                                     gdouble        y);
gboolean   _cdk_x11_display_supports_cursor_alpha   (GdkDisplay    *display);
gboolean   _cdk_x11_display_supports_cursor_color   (GdkDisplay    *display);
void       _cdk_x11_display_get_default_cursor_size (GdkDisplay *display,
                                                     guint      *width,
                                                     guint      *height);
void       _cdk_x11_display_get_maximal_cursor_size (GdkDisplay *display,
                                                     guint      *width,
                                                     guint      *height);
void       _cdk_x11_display_before_process_all_updates (GdkDisplay *display);
void       _cdk_x11_display_after_process_all_updates  (GdkDisplay *display);
void       _cdk_x11_display_create_window_impl     (GdkDisplay    *display,
                                                    GdkWindow     *window,
                                                    GdkWindow     *real_parent,
                                                    GdkScreen     *screen,
                                                    GdkEventMask   event_mask,
                                                    GdkWindowAttr *attributes,
                                                    gint           attributes_mask);

void _cdk_x11_precache_atoms (GdkDisplay          *display,
                              const gchar * const *atom_names,
                              gint                 n_atoms);

Atom _cdk_x11_get_xatom_for_display_printf         (GdkDisplay    *display,
                                                    const gchar   *format,
                                                    ...) G_GNUC_PRINTF (2, 3);

GdkFilterReturn
_cdk_x11_dnd_filter (GdkXEvent *xev,
                     GdkEvent  *event,
                     gpointer   data);

void _cdk_x11_screen_init_root_window (GdkScreen *screen);
void _cdk_x11_screen_init_visuals     (GdkScreen *screen);

void _cdk_x11_cursor_update_theme (GdkCursor *cursor);
void _cdk_x11_cursor_display_finalize (GdkDisplay *display);

void _cdk_x11_window_register_dnd (GdkWindow *window);

GdkDragContext * _cdk_x11_window_drag_begin (GdkWindow *window,
                                             GdkDevice *device,
                                             GList     *targets,
                                             gint       x_root,
                                             gint       y_root);

gboolean _cdk_x11_get_xft_setting (GdkScreen   *screen,
                                   const gchar *name,
                                   GValue      *value);

GdkGrabStatus _cdk_x11_convert_grab_status (gint status);

cairo_surface_t * _cdk_x11_window_create_bitmap_surface (GdkWindow *window,
                                                         int        width,
                                                         int        height);

extern const gint        _cdk_x11_event_mask_table[];
extern const gint        _cdk_x11_event_mask_table_size;

#define GDK_SCREEN_DISPLAY(screen)    (GDK_X11_SCREEN (screen)->display)
#define GDK_SCREEN_XROOTWIN(screen)   (GDK_X11_SCREEN (screen)->xroot_window)
#define GDK_WINDOW_SCREEN(win)        (cdk_window_get_screen (win))
#define GDK_WINDOW_DISPLAY(win)       (GDK_X11_SCREEN (GDK_WINDOW_SCREEN (win))->display)
#define GDK_WINDOW_XROOTWIN(win)      (GDK_X11_SCREEN (GDK_WINDOW_SCREEN (win))->xroot_window)
#define GDK_WINDOW_IS_X11(win)        (GDK_IS_WINDOW_IMPL_X11 ((win)->impl))

/* override some macros from cdkx.h with direct-access variants */
#undef GDK_DISPLAY_XDISPLAY
#undef GDK_WINDOW_XDISPLAY
#undef GDK_WINDOW_XID
#undef GDK_SCREEN_XDISPLAY

#define GDK_DISPLAY_XDISPLAY(display) (GDK_X11_DISPLAY(display)->xdisplay)
#define GDK_WINDOW_XDISPLAY(win)      (GDK_X11_SCREEN (GDK_WINDOW_SCREEN (win))->xdisplay)
#define GDK_WINDOW_XID(win)           (GDK_WINDOW_IMPL_X11(GDK_WINDOW (win)->impl)->xid)
#define GDK_SCREEN_XDISPLAY(screen)   (GDK_X11_SCREEN (screen)->xdisplay)

#endif /* __GDK_PRIVATE_X11_H__ */
