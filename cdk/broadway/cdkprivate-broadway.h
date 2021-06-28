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

#ifndef __CDK_PRIVATE_BROADWAY_H__
#define __CDK_PRIVATE_BROADWAY_H__

#include <cdk/cdkcursor.h>
#include <cdk/cdkprivate.h>
#include <cdk/cdkinternals.h>
#include "cdkwindow-broadway.h"
#include "cdkdisplay-broadway.h"

#include "cdkbroadwaycursor.h"
#include "cdkbroadwayvisual.h"
#include "cdkbroadwaywindow.h"

void _cdk_broadway_resync_windows (void);

void     _cdk_broadway_window_register_dnd (CdkWindow      *window);
CdkDragContext * _cdk_broadway_window_drag_begin (CdkWindow *window,
						  CdkDevice *device,
						  GList     *targets,
                                                  gint       x_root,
                                                  gint       y_root);
void     _cdk_broadway_window_translate         (CdkWindow *window,
						 cairo_region_t *area,
						 gint       dx,
						 gint       dy);
gboolean _cdk_broadway_window_get_property (CdkWindow   *window,
					    CdkAtom      property,
					    CdkAtom      type,
					    gulong       offset,
					    gulong       length,
					    gint         pdelete,
					    CdkAtom     *actual_property_type,
					    gint        *actual_format_type,
					    gint        *actual_length,
					    guchar     **data);
void _cdk_broadway_window_change_property (CdkWindow    *window,
					   CdkAtom       property,
					   CdkAtom       type,
					   gint          format,
					   CdkPropMode   mode,
					   const guchar *data,
					   gint          nelements);
void _cdk_broadway_window_delete_property (CdkWindow *window,
					   CdkAtom    property);
gboolean _cdk_broadway_moveresize_handle_event   (CdkDisplay *display,
						  BroadwayInputMsg *msg);
gboolean _cdk_broadway_moveresize_configure_done (CdkDisplay *display,
						  CdkWindow  *window);


void     _cdk_broadway_selection_window_destroyed (CdkWindow *window);
void     _cdk_broadway_window_grab_check_destroy (CdkWindow *window);
void     _cdk_broadway_window_grab_check_unmap (CdkWindow *window,
						gulong     serial);

void _cdk_keymap_keys_changed     (CdkDisplay      *display);
gint _cdk_broadway_get_group_for_state (CdkDisplay      *display,
					CdkModifierType  state);
void _cdk_keymap_add_virtual_modifiers_compat (CdkKeymap       *keymap,
                                               CdkModifierType *modifiers);
gboolean _cdk_keymap_key_is_modifier   (CdkKeymap       *keymap,
					guint            keycode);

void _cdk_broadway_screen_events_init   (CdkScreen *screen);
CdkVisual *_cdk_broadway_screen_get_system_visual (CdkScreen * screen);
gint _cdk_broadway_screen_visual_get_best_depth (CdkScreen * screen);
CdkVisualType _cdk_broadway_screen_visual_get_best_type (CdkScreen * screen);
CdkVisual *_cdk_broadway_screen_get_system_visual (CdkScreen * screen);
CdkVisual*_cdk_broadway_screen_visual_get_best (CdkScreen * screen);
CdkVisual*_cdk_broadway_screen_visual_get_best_with_depth (CdkScreen * screen,
							   gint depth);
CdkVisual*_cdk_broadway_screen_visual_get_best_with_type (CdkScreen * screen,
							  CdkVisualType visual_type);
CdkVisual*_cdk_broadway_screen_visual_get_best_with_both (CdkScreen * screen,
							  gint          depth,
							  CdkVisualType visual_type);
void _cdk_broadway_screen_query_depths  (CdkScreen * screen,
					 gint **depths,
					 gint  *count);
void _cdk_broadway_screen_query_visual_types (CdkScreen * screen,
					      CdkVisualType **visual_types,
					      gint           *count);
GList *_cdk_broadway_screen_list_visuals (CdkScreen *screen);
void _cdk_broadway_screen_size_changed (CdkScreen *screen, 
					BroadwayInputScreenResizeNotify *msg);

void _cdk_broadway_events_got_input      (BroadwayInputMsg *message);

void _cdk_broadway_screen_init_root_window (CdkScreen *screen);
void _cdk_broadway_screen_init_visuals (CdkScreen *screen);
void _cdk_broadway_display_init_dnd (CdkDisplay *display);
CdkDisplay * _cdk_broadway_display_open (const gchar *display_name);
void _cdk_broadway_display_queue_events (CdkDisplay *display);
CdkDragProtocol _cdk_broadway_window_get_drag_protocol (CdkWindow *window,
							CdkWindow **target);
CdkCursor*_cdk_broadway_display_get_cursor_for_type (CdkDisplay    *display,
						     CdkCursorType  cursor_type);
CdkCursor*_cdk_broadway_display_get_cursor_for_name (CdkDisplay  *display,
						     const gchar *name);
CdkCursor *_cdk_broadway_display_get_cursor_for_surface (CdkDisplay *display,
							 cairo_surface_t *surface,
							 gdouble     x,
							 gdouble     y);
gboolean _cdk_broadway_display_supports_cursor_alpha (CdkDisplay *display);
gboolean _cdk_broadway_display_supports_cursor_color (CdkDisplay *display);
void _cdk_broadway_display_get_default_cursor_size (CdkDisplay *display,
						    guint       *width,
						    guint       *height);
void _cdk_broadway_display_get_maximal_cursor_size (CdkDisplay *display,
						    guint       *width,
						    guint       *height);
void       _cdk_broadway_display_before_process_all_updates (CdkDisplay *display);
void       _cdk_broadway_display_after_process_all_updates  (CdkDisplay *display);
void       _cdk_broadway_display_create_window_impl     (CdkDisplay    *display,
							 CdkWindow     *window,
							 CdkWindow     *real_parent,
							 CdkScreen     *screen,
							 CdkEventMask   event_mask,
							 CdkWindowAttr *attributes,
							 gint           attributes_mask);
gboolean _cdk_broadway_display_set_selection_owner (CdkDisplay *display,
						    CdkWindow  *owner,
						    CdkAtom     selection,
						    guint32     time,
						    gboolean    send_event);
CdkWindow * _cdk_broadway_display_get_selection_owner (CdkDisplay *display,
						       CdkAtom     selection);
gint _cdk_broadway_display_get_selection_property (CdkDisplay *display,
						   CdkWindow  *requestor,
						   guchar    **data,
						   CdkAtom    *ret_type,
						   gint       *ret_format);
void _cdk_broadway_display_send_selection_notify (CdkDisplay       *display,
						  CdkWindow       *requestor,
						  CdkAtom          selection,
						  CdkAtom          target,
						  CdkAtom          property, 
						  guint32          time);
void _cdk_broadway_display_convert_selection (CdkDisplay *display,
					      CdkWindow *requestor,
					      CdkAtom    selection,
					      CdkAtom    target,
					      guint32    time);
gint _cdk_broadway_display_text_property_to_utf8_list (CdkDisplay    *display,
						       CdkAtom        encoding,
						       gint           format,
						       const guchar  *text,
						       gint           length,
						       gchar       ***list);
gchar *_cdk_broadway_display_utf8_to_string_target (CdkDisplay  *display,
						    const gchar *str);
CdkKeymap* _cdk_broadway_display_get_keymap (CdkDisplay *display);
void _cdk_broadway_display_consume_all_input (CdkDisplay *display);
BroadwayInputMsg * _cdk_broadway_display_block_for_input (CdkDisplay *display,
							  char op,
							  guint32 serial,
							  gboolean remove);

/* Window methods - testing */
void     _cdk_broadway_window_sync_rendering    (CdkWindow       *window);
gboolean _cdk_broadway_window_simulate_key      (CdkWindow       *window,
						 gint             x,
						 gint             y,
						 guint            keyval,
						 CdkModifierType  modifiers,
						 CdkEventType     key_pressrelease);
gboolean _cdk_broadway_window_simulate_button   (CdkWindow       *window,
						 gint             x,
						 gint             y,
						 guint            button,
						 CdkModifierType  modifiers,
						 CdkEventType     button_pressrelease);
void _cdk_broadway_window_resize_surface        (CdkWindow *window);

void _cdk_broadway_cursor_update_theme (CdkCursor *cursor);
void _cdk_broadway_cursor_display_finalize (CdkDisplay *display);

#define CDK_WINDOW_IS_BROADWAY(win)   (CDK_IS_WINDOW_IMPL_BROADWAY (((CdkWindow *)win)->impl))

#endif /* __CDK_PRIVATE_BROADWAY_H__ */
