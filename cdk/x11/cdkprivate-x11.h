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
 * Private uninstalled header defining things local to X windowing code
 */

#ifndef __CDK_PRIVATE_X11_H__
#define __CDK_PRIVATE_X11_H__

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

Colormap _cdk_visual_get_x11_colormap (CdkVisual *visual);

gint          _cdk_x11_screen_visual_get_best_depth      (CdkScreen      *screen);
CdkVisualType _cdk_x11_screen_visual_get_best_type       (CdkScreen      *screen);
CdkVisual *   _cdk_x11_screen_get_system_visual          (CdkScreen      *screen);
CdkVisual*    _cdk_x11_screen_visual_get_best            (CdkScreen      *screen);
CdkVisual*    _cdk_x11_screen_visual_get_best_with_depth (CdkScreen      *screen,
                                                          gint            depth);
CdkVisual*    _cdk_x11_screen_visual_get_best_with_type  (CdkScreen      *screen,
                                                          CdkVisualType   visual_type);
CdkVisual*    _cdk_x11_screen_visual_get_best_with_both  (CdkScreen      *screen,
                                                          gint            depth,
                                                          CdkVisualType   visual_type);
void          _cdk_x11_screen_query_depths               (CdkScreen      *screen,
                                                          gint          **depths,
                                                          gint           *count);
void          _cdk_x11_screen_query_visual_types         (CdkScreen      *screen,
                                                          CdkVisualType **visual_types,
                                                          gint           *count);
GList *       _cdk_x11_screen_list_visuals               (CdkScreen      *screen);



void _cdk_x11_display_add_window    (CdkDisplay *display,
                                     XID        *xid,
                                     CdkWindow  *window);
void _cdk_x11_display_remove_window (CdkDisplay *display,
                                     XID         xid);

gint _cdk_x11_display_send_xevent (CdkDisplay *display,
                                   Window      window,
                                   gboolean    propagate,
                                   glong       event_mask,
                                   XEvent     *event_send);

/* Routines from cdkgeometry-x11.c */
void _cdk_x11_window_move_resize_child (CdkWindow     *window,
                                        gint           x,
                                        gint           y,
                                        gint           width,
                                        gint           height);
void _cdk_x11_window_process_expose    (CdkWindow     *window,
                                        gulong         serial,
                                        CdkRectangle  *area);

void     _cdk_x11_window_sync_rendering    (CdkWindow       *window);
gboolean _cdk_x11_window_simulate_key      (CdkWindow       *window,
                                            gint             x,
                                            gint             y,
                                            guint            keyval,
                                            CdkModifierType  modifiers,
                                            CdkEventType     key_pressrelease);
gboolean _cdk_x11_window_simulate_button   (CdkWindow       *window,
                                            gint             x,
                                            gint             y,
                                            guint            button,
                                            CdkModifierType  modifiers,
                                            CdkEventType     button_pressrelease);
gboolean _cdk_x11_window_get_property      (CdkWindow    *window,
                                            CdkAtom       property,
                                            CdkAtom       type,
                                            gulong        offset,
                                            gulong        length,
                                            gint          pdelete,
                                            CdkAtom      *actual_property_type,
                                            gint         *actual_format_type,
                                            gint         *actual_length,
                                            guchar      **data);
void     _cdk_x11_window_change_property   (CdkWindow    *window,
                                            CdkAtom       property,
                                            CdkAtom       type,
                                            gint          format,
                                            CdkPropMode   mode,
                                            const guchar *data,
                                            gint          nelements);
void     _cdk_x11_window_delete_property   (CdkWindow    *window,
                                            CdkAtom       property);

void     _cdk_x11_window_queue_antiexpose  (CdkWindow *window,
                                            cairo_region_t *area);
void     _cdk_x11_window_translate         (CdkWindow *window,
                                            cairo_region_t *area,
                                            gint       dx,
                                            gint       dy);

void     _cdk_x11_display_free_translate_queue (CdkDisplay *display);

void     _cdk_x11_selection_window_destroyed   (CdkWindow            *window);
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
gboolean _cdk_x11_moveresize_configure_done (CdkDisplay *display,
                                             CdkWindow  *window);

void     _cdk_x11_keymap_state_changed   (CdkDisplay      *display,
                                          XEvent          *event);
void     _cdk_x11_keymap_keys_changed    (CdkDisplay      *display);
void     _cdk_x11_keymap_add_virt_mods   (CdkKeymap       *keymap,
                                          CdkModifierType *modifiers);

void _cdk_x11_windowing_init    (void);

void _cdk_x11_window_grab_check_unmap   (CdkWindow *window,
                                         gulong     serial);
void _cdk_x11_window_grab_check_destroy (CdkWindow *window);

gboolean _cdk_x11_display_is_root_window (CdkDisplay *display,
                                          Window      xroot_window);

CdkDisplay * _cdk_x11_display_open            (const gchar *display_name);
void _cdk_x11_display_update_grab_info        (CdkDisplay *display,
                                               CdkDevice  *device,
                                               gint        status);
void _cdk_x11_display_update_grab_info_ungrab (CdkDisplay *display,
                                               CdkDevice  *device,
                                               guint32     time,
                                               gulong      serial);
void _cdk_x11_display_queue_events            (CdkDisplay *display);


CdkAppLaunchContext *_cdk_x11_display_get_app_launch_context (CdkDisplay *display);
Window      _cdk_x11_display_get_drag_protocol     (CdkDisplay      *display,
                                                    Window           xid,
                                                    CdkDragProtocol *protocol,
                                                    guint           *version);

gboolean    _cdk_x11_display_set_selection_owner   (CdkDisplay *display,
                                                    CdkWindow  *owner,
                                                    CdkAtom     selection,
                                                    guint32     time,
                                                    gboolean    send_event);
CdkWindow * _cdk_x11_display_get_selection_owner   (CdkDisplay *display,
                                                    CdkAtom     selection);
void        _cdk_x11_display_send_selection_notify (CdkDisplay       *display,
                                                    CdkWindow        *requestor,
                                                    CdkAtom          selection,
                                                    CdkAtom          target,
                                                    CdkAtom          property,
                                                    guint32          time);
gint        _cdk_x11_display_get_selection_property (CdkDisplay     *display,
                                                     CdkWindow      *requestor,
                                                     guchar        **data,
                                                     CdkAtom        *ret_type,
                                                     gint           *ret_format);
void        _cdk_x11_display_convert_selection      (CdkDisplay     *display,
                                                     CdkWindow      *requestor,
                                                     CdkAtom         selection,
                                                     CdkAtom         target,
                                                     guint32         time);

gint        _cdk_x11_display_text_property_to_utf8_list (CdkDisplay     *display,
                                                         CdkAtom         encoding,
                                                         gint            format,
                                                         const guchar   *text,
                                                         gint            length,
                                                         gchar        ***list);
gchar *     _cdk_x11_display_utf8_to_string_target      (CdkDisplay     *displayt,
                                                         const gchar    *str);

void _cdk_x11_device_check_extension_events   (CdkDevice  *device);

CdkDeviceManager *_cdk_x11_device_manager_new (CdkDisplay *display);

#ifdef XINPUT_2
guchar * _cdk_x11_device_xi2_translate_event_mask (CdkX11DeviceManagerXI2 *device_manager_xi2,
                                                   CdkEventMask            event_mask,
                                                   gint                   *len);
guint    _cdk_x11_device_xi2_translate_state      (XIModifierState *mods_state,
                                                   XIButtonState   *buttons_state,
                                                   XIGroupState    *group_state);
gint     _cdk_x11_device_xi2_get_id               (CdkX11DeviceXI2 *device);
void     _cdk_device_xi2_unset_scroll_valuators   (CdkX11DeviceXI2 *device);


CdkDevice * _cdk_x11_device_manager_xi2_lookup    (CdkX11DeviceManagerXI2 *device_manager_xi2,
                                                   gint                    device_id);
void     _cdk_x11_device_xi2_add_scroll_valuator  (CdkX11DeviceXI2    *device,
                                                   guint               n_valuator,
                                                   CdkScrollDirection  direction,
                                                   gdouble             increment);
gboolean  _cdk_x11_device_xi2_get_scroll_delta    (CdkX11DeviceXI2    *device,
                                                   guint               n_valuator,
                                                   gdouble             valuator_value,
                                                   CdkScrollDirection *direction_ret,
                                                   gdouble            *delta_ret);
void     _cdk_device_xi2_reset_scroll_valuators   (CdkX11DeviceXI2    *device);

gdouble  cdk_x11_device_xi2_get_last_axis_value (CdkX11DeviceXI2 *device,
                                                 gint             n_axis);

void     cdk_x11_device_xi2_store_axes          (CdkX11DeviceXI2 *device,
                                                 gdouble         *axes,
                                                 gint             n_axes);
#endif

void     _cdk_x11_event_translate_keyboard_string (CdkEventKey *event);

CdkAtom _cdk_x11_display_manager_atom_intern   (CdkDisplayManager *manager,
                                                const gchar       *atom_name,
                                                gboolean           copy_name);
gchar * _cdk_x11_display_manager_get_atom_name (CdkDisplayManager *manager,
                                                CdkAtom            atom);

CdkCursor *_cdk_x11_display_get_cursor_for_type     (CdkDisplay    *display,
                                                     CdkCursorType  type);
CdkCursor *_cdk_x11_display_get_cursor_for_name     (CdkDisplay    *display,
                                                     const gchar   *name);
CdkCursor *_cdk_x11_display_get_cursor_for_surface  (CdkDisplay    *display,
                                                     cairo_surface_t *surface,
                                                     gdouble        x,
                                                     gdouble        y);
gboolean   _cdk_x11_display_supports_cursor_alpha   (CdkDisplay    *display);
gboolean   _cdk_x11_display_supports_cursor_color   (CdkDisplay    *display);
void       _cdk_x11_display_get_default_cursor_size (CdkDisplay *display,
                                                     guint      *width,
                                                     guint      *height);
void       _cdk_x11_display_get_maximal_cursor_size (CdkDisplay *display,
                                                     guint      *width,
                                                     guint      *height);
void       _cdk_x11_display_before_process_all_updates (CdkDisplay *display);
void       _cdk_x11_display_after_process_all_updates  (CdkDisplay *display);
void       _cdk_x11_display_create_window_impl     (CdkDisplay    *display,
                                                    CdkWindow     *window,
                                                    CdkWindow     *real_parent,
                                                    CdkScreen     *screen,
                                                    CdkEventMask   event_mask,
                                                    CdkWindowAttr *attributes,
                                                    gint           attributes_mask);

void _cdk_x11_precache_atoms (CdkDisplay          *display,
                              const gchar * const *atom_names,
                              gint                 n_atoms);

Atom _cdk_x11_get_xatom_for_display_printf         (CdkDisplay    *display,
                                                    const gchar   *format,
                                                    ...) G_GNUC_PRINTF (2, 3);

CdkFilterReturn
_cdk_x11_dnd_filter (CdkXEvent *xev,
                     CdkEvent  *event,
                     gpointer   data);

void _cdk_x11_screen_init_root_window (CdkScreen *screen);
void _cdk_x11_screen_init_visuals     (CdkScreen *screen);

void _cdk_x11_cursor_update_theme (CdkCursor *cursor);
void _cdk_x11_cursor_display_finalize (CdkDisplay *display);

void _cdk_x11_window_register_dnd (CdkWindow *window);

CdkDragContext * _cdk_x11_window_drag_begin (CdkWindow *window,
                                             CdkDevice *device,
                                             GList     *targets,
                                             gint       x_root,
                                             gint       y_root);

gboolean _cdk_x11_get_xft_setting (CdkScreen   *screen,
                                   const gchar *name,
                                   GValue      *value);

CdkGrabStatus _cdk_x11_convert_grab_status (gint status);

cairo_surface_t * _cdk_x11_window_create_bitmap_surface (CdkWindow *window,
                                                         int        width,
                                                         int        height);

extern const gint        _cdk_x11_event_mask_table[];
extern const gint        _cdk_x11_event_mask_table_size;

#define CDK_SCREEN_DISPLAY(screen)    (CDK_X11_SCREEN (screen)->display)
#define CDK_SCREEN_XROOTWIN(screen)   (CDK_X11_SCREEN (screen)->xroot_window)
#define CDK_WINDOW_SCREEN(win)        (cdk_window_get_screen (win))
#define CDK_WINDOW_DISPLAY(win)       (CDK_X11_SCREEN (CDK_WINDOW_SCREEN (win))->display)
#define CDK_WINDOW_XROOTWIN(win)      (CDK_X11_SCREEN (CDK_WINDOW_SCREEN (win))->xroot_window)
#define CDK_WINDOW_IS_X11(win)        (CDK_IS_WINDOW_IMPL_X11 ((win)->impl))

/* override some macros from cdkx.h with direct-access variants */
#undef CDK_DISPLAY_XDISPLAY
#undef CDK_WINDOW_XDISPLAY
#undef CDK_WINDOW_XID
#undef CDK_SCREEN_XDISPLAY

#define CDK_DISPLAY_XDISPLAY(display) (CDK_X11_DISPLAY(display)->xdisplay)
#define CDK_WINDOW_XDISPLAY(win)      (CDK_X11_SCREEN (CDK_WINDOW_SCREEN (win))->xdisplay)
#define CDK_WINDOW_XID(win)           (CDK_WINDOW_IMPL_X11(CDK_WINDOW (win)->impl)->xid)
#define CDK_SCREEN_XDISPLAY(screen)   (CDK_X11_SCREEN (screen)->xdisplay)

#endif /* __CDK_PRIVATE_X11_H__ */
