
/* GTK - The GIMP Toolkit
 * Copyright (C) 2010 Red Hat, Inc.
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

#ifndef __CTK_WINDOW_PRIVATE_H__
#define __CTK_WINDOW_PRIVATE_H__

#include <gdk/gdk.h>

#include "ctkwindow.h"

G_BEGIN_DECLS

void            _ctk_window_internal_set_focus (CtkWindow *window,
                                                CtkWidget *focus);
void            _ctk_window_reposition         (CtkWindow *window,
                                                gint       x,
                                                gint       y);
void            _ctk_window_group_add_grab    (CtkWindowGroup *window_group,
                                               CtkWidget      *widget);
void            _ctk_window_group_remove_grab (CtkWindowGroup *window_group,
                                               CtkWidget      *widget);
void            _ctk_window_group_add_device_grab    (CtkWindowGroup   *window_group,
                                                      CtkWidget        *widget,
                                                      GdkDevice        *device,
                                                      gboolean          block_others);
void            _ctk_window_group_remove_device_grab (CtkWindowGroup   *window_group,
                                                      CtkWidget        *widget,
                                                      GdkDevice        *device);

gboolean        _ctk_window_group_widget_is_blocked_for_device (CtkWindowGroup *window_group,
                                                                CtkWidget      *widget,
                                                                GdkDevice      *device);

void            _ctk_window_set_has_toplevel_focus (CtkWindow *window,
                                                    gboolean   has_toplevel_focus);
void            _ctk_window_unset_focus_and_default (CtkWindow *window,
                                                     CtkWidget *widget);

void            _ctk_window_set_is_active          (CtkWindow *window,
                                                    gboolean   is_active);

void            _ctk_window_set_is_toplevel        (CtkWindow *window,
                                                    gboolean   is_toplevel);

void            _ctk_window_get_wmclass            (CtkWindow  *window,
                                                    gchar     **wmclass_name,
                                                    gchar     **wmclass_class);

void            _ctk_window_set_allocation         (CtkWindow           *window,
                                                    const CtkAllocation *allocation,
                                                    CtkAllocation       *allocation_out);

typedef void (*CtkWindowKeysForeachFunc) (CtkWindow      *window,
                                          guint           keyval,
                                          GdkModifierType modifiers,
                                          gboolean        is_mnemonic,
                                          gpointer        data);

void _ctk_window_keys_foreach (CtkWindow               *window,
                               CtkWindowKeysForeachFunc func,
                               gpointer                 func_data);

gboolean _ctk_window_check_handle_wm_event (GdkEvent  *event);

/* --- internal (CtkAcceleratable) --- */
gboolean        _ctk_window_query_nonaccels     (CtkWindow      *window,
                                                 guint           accel_key,
                                                 GdkModifierType accel_mods);

void            _ctk_window_schedule_mnemonics_visible (CtkWindow *window);

void            _ctk_window_notify_keys_changed (CtkWindow *window);

gboolean        _ctk_window_titlebar_shows_app_menu (CtkWindow *window);

void            _ctk_window_get_shadow_width (CtkWindow *window,
                                              CtkBorder *border);

void            _ctk_window_toggle_maximized (CtkWindow *window);

void            _ctk_window_request_csd (CtkWindow *window);

/* Window groups */

CtkWindowGroup *_ctk_window_get_window_group (CtkWindow *window);

void            _ctk_window_set_window_group (CtkWindow      *window,
                                              CtkWindowGroup *group);

/* Popovers */
void    _ctk_window_add_popover          (CtkWindow                   *window,
                                          CtkWidget                   *popover,
                                          CtkWidget                   *popover_parent,
                                          gboolean                     clamp_allocation);
void    _ctk_window_remove_popover       (CtkWindow                   *window,
                                          CtkWidget                   *popover);
void    _ctk_window_set_popover_position (CtkWindow                   *window,
                                          CtkWidget                   *popover,
                                          CtkPositionType              pos,
                                          const cairo_rectangle_int_t *rect);
void    _ctk_window_get_popover_position (CtkWindow                   *window,
                                          CtkWidget                   *popover,
                                          CtkPositionType             *pos,
                                          cairo_rectangle_int_t       *rect);
void    _ctk_window_raise_popover        (CtkWindow                   *window,
                                          CtkWidget                   *popover);

CtkWidget * _ctk_window_get_popover_parent (CtkWindow *window,
                                            CtkWidget *popover);
gboolean    _ctk_window_is_popover_widget  (CtkWindow *window,
                                            CtkWidget *popover);

GdkPixbuf *ctk_window_get_icon_for_size (CtkWindow *window,
                                         gint       size);

void       ctk_window_set_use_subsurface (CtkWindow *window,
                                          gboolean   use_subsurface);
void       ctk_window_set_hardcoded_window (CtkWindow *window,
                                            GdkWindow *gdk_window);

GdkScreen *_ctk_window_get_screen (CtkWindow *window);

void       ctk_window_set_unlimited_guessed_size (CtkWindow *window,
                                                  gboolean   x,
                                                  gboolean   y);
void       ctk_window_force_resize (CtkWindow *window);
void       ctk_window_fixate_size (CtkWindow *window);
void       ctk_window_move_resize (CtkWindow *window);

/* Exported handles */

typedef void (*CtkWindowHandleExported)  (CtkWindow               *window,
                                          const char              *handle,
                                          gpointer                 user_data);

gboolean      ctk_window_export_handle   (CtkWindow               *window,
                                          CtkWindowHandleExported  callback,
                                          gpointer                 user_data);
void          ctk_window_unexport_handle (CtkWindow               *window);

G_END_DECLS

#endif /* __CTK_WINDOW_PRIVATE_H__ */
