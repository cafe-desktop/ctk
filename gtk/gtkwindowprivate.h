
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

#ifndef __GTK_WINDOW_PRIVATE_H__
#define __GTK_WINDOW_PRIVATE_H__

#include <gdk/gdk.h>

#include "gtkwindow.h"

G_BEGIN_DECLS

void            _ctk_window_internal_set_focus (GtkWindow *window,
                                                GtkWidget *focus);
void            _ctk_window_reposition         (GtkWindow *window,
                                                gint       x,
                                                gint       y);
void            _ctk_window_group_add_grab    (GtkWindowGroup *window_group,
                                               GtkWidget      *widget);
void            _ctk_window_group_remove_grab (GtkWindowGroup *window_group,
                                               GtkWidget      *widget);
void            _ctk_window_group_add_device_grab    (GtkWindowGroup   *window_group,
                                                      GtkWidget        *widget,
                                                      GdkDevice        *device,
                                                      gboolean          block_others);
void            _ctk_window_group_remove_device_grab (GtkWindowGroup   *window_group,
                                                      GtkWidget        *widget,
                                                      GdkDevice        *device);

gboolean        _ctk_window_group_widget_is_blocked_for_device (GtkWindowGroup *window_group,
                                                                GtkWidget      *widget,
                                                                GdkDevice      *device);

void            _ctk_window_set_has_toplevel_focus (GtkWindow *window,
                                                    gboolean   has_toplevel_focus);
void            _ctk_window_unset_focus_and_default (GtkWindow *window,
                                                     GtkWidget *widget);

void            _ctk_window_set_is_active          (GtkWindow *window,
                                                    gboolean   is_active);

void            _ctk_window_set_is_toplevel        (GtkWindow *window,
                                                    gboolean   is_toplevel);

void            _ctk_window_get_wmclass            (GtkWindow  *window,
                                                    gchar     **wmclass_name,
                                                    gchar     **wmclass_class);

void            _ctk_window_set_allocation         (GtkWindow           *window,
                                                    const GtkAllocation *allocation,
                                                    GtkAllocation       *allocation_out);

typedef void (*GtkWindowKeysForeachFunc) (GtkWindow      *window,
                                          guint           keyval,
                                          GdkModifierType modifiers,
                                          gboolean        is_mnemonic,
                                          gpointer        data);

void _ctk_window_keys_foreach (GtkWindow               *window,
                               GtkWindowKeysForeachFunc func,
                               gpointer                 func_data);

gboolean _ctk_window_check_handle_wm_event (GdkEvent  *event);

/* --- internal (GtkAcceleratable) --- */
gboolean        _ctk_window_query_nonaccels     (GtkWindow      *window,
                                                 guint           accel_key,
                                                 GdkModifierType accel_mods);

void            _ctk_window_schedule_mnemonics_visible (GtkWindow *window);

void            _ctk_window_notify_keys_changed (GtkWindow *window);

gboolean        _ctk_window_titlebar_shows_app_menu (GtkWindow *window);

void            _ctk_window_get_shadow_width (GtkWindow *window,
                                              GtkBorder *border);

void            _ctk_window_toggle_maximized (GtkWindow *window);

void            _ctk_window_request_csd (GtkWindow *window);

/* Window groups */

GtkWindowGroup *_ctk_window_get_window_group (GtkWindow *window);

void            _ctk_window_set_window_group (GtkWindow      *window,
                                              GtkWindowGroup *group);

/* Popovers */
void    _ctk_window_add_popover          (GtkWindow                   *window,
                                          GtkWidget                   *popover,
                                          GtkWidget                   *popover_parent,
                                          gboolean                     clamp_allocation);
void    _ctk_window_remove_popover       (GtkWindow                   *window,
                                          GtkWidget                   *popover);
void    _ctk_window_set_popover_position (GtkWindow                   *window,
                                          GtkWidget                   *popover,
                                          GtkPositionType              pos,
                                          const cairo_rectangle_int_t *rect);
void    _ctk_window_get_popover_position (GtkWindow                   *window,
                                          GtkWidget                   *popover,
                                          GtkPositionType             *pos,
                                          cairo_rectangle_int_t       *rect);
void    _ctk_window_raise_popover        (GtkWindow                   *window,
                                          GtkWidget                   *popover);

GtkWidget * _ctk_window_get_popover_parent (GtkWindow *window,
                                            GtkWidget *popover);
gboolean    _ctk_window_is_popover_widget  (GtkWindow *window,
                                            GtkWidget *popover);

GdkPixbuf *ctk_window_get_icon_for_size (GtkWindow *window,
                                         gint       size);

void       ctk_window_set_use_subsurface (GtkWindow *window,
                                          gboolean   use_subsurface);
void       ctk_window_set_hardcoded_window (GtkWindow *window,
                                            GdkWindow *gdk_window);

GdkScreen *_ctk_window_get_screen (GtkWindow *window);

void       ctk_window_set_unlimited_guessed_size (GtkWindow *window,
                                                  gboolean   x,
                                                  gboolean   y);
void       ctk_window_force_resize (GtkWindow *window);
void       ctk_window_fixate_size (GtkWindow *window);
void       ctk_window_move_resize (GtkWindow *window);

/* Exported handles */

typedef void (*GtkWindowHandleExported)  (GtkWindow               *window,
                                          const char              *handle,
                                          gpointer                 user_data);

gboolean      ctk_window_export_handle   (GtkWindow               *window,
                                          GtkWindowHandleExported  callback,
                                          gpointer                 user_data);
void          ctk_window_unexport_handle (GtkWindow               *window);

G_END_DECLS

#endif /* __GTK_WINDOW_PRIVATE_H__ */
