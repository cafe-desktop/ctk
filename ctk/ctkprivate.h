/* GTK - The GIMP Toolkit
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
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_PRIVATE_H__
#define __CTK_PRIVATE_H__

#include <glib-object.h>
#include <gdk/gdk.h>

#include "ctkcsstypesprivate.h"
#include "ctktexthandleprivate.h"

G_BEGIN_DECLS

#define CTK_PARAM_READABLE G_PARAM_READABLE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB
#define CTK_PARAM_WRITABLE G_PARAM_WRITABLE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB
#define CTK_PARAM_READWRITE G_PARAM_READWRITE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB

#define OPPOSITE_ORIENTATION(_orientation) (1 - (_orientation))

#ifdef G_DISABLE_CAST_CHECKS
/* This is true for debug no and minimum */
#define ctk_internal_return_if_fail(__expr) G_STMT_START{ (void)0; }G_STMT_END
#define ctk_internal_return_val_if_fail(__expr, __val) G_STMT_START{ (void)0; }G_STMT_END
#else
/* This is true for debug yes */
#define ctk_internal_return_if_fail(__expr) g_return_if_fail(__expr)
#define ctk_internal_return_val_if_fail(__expr, __val) g_return_val_if_fail(__expr, __val)
#endif

const gchar * _ctk_get_datadir            (void);
const gchar * _ctk_get_libdir             (void);
const gchar * _ctk_get_sysconfdir         (void);
const gchar * _ctk_get_localedir          (void);
const gchar * _ctk_get_data_prefix        (void);

gboolean      _ctk_fnmatch                (const char *pattern,
                                           const char *string,
                                           gboolean    no_leading_period);

gchar       * _ctk_get_lc_ctype           (void);

void          _ctk_ensure_resources       (void);

gboolean _ctk_boolean_handled_accumulator (GSignalInvocationHint *ihint,
                                           GValue                *return_accu,
                                           const GValue          *handler_return,
                                           gpointer               dummy);

gboolean _ctk_single_string_accumulator   (GSignalInvocationHint *ihint,
                                           GValue                *return_accu,
                                           const GValue          *handler_return,
                                           gpointer               dummy);

GdkModifierType _ctk_replace_virtual_modifiers (GdkKeymap       *keymap,
                                                GdkModifierType  modifiers);
GdkModifierType _ctk_get_primary_accel_mod     (void);

gboolean _ctk_translate_keyboard_accel_state   (GdkKeymap       *keymap,
                                                guint            hardware_keycode,
                                                GdkModifierType  state,
                                                GdkModifierType  accel_mask,
                                                gint             group,
                                                guint           *keyval,
                                                gint            *effective_group,
                                                gint            *level,
                                                GdkModifierType *consumed_modifiers);

gboolean        _ctk_propagate_captured_event  (CtkWidget       *widget,
                                                GdkEvent        *event,
                                                CtkWidget       *topmost);


gdouble _ctk_get_slowdown ();
void    _ctk_set_slowdown (gdouble slowdown_factor);

gboolean ctk_should_use_portal (void);
char *ctk_get_portal_request_path (GDBusConnection  *connection,
                                   char            **token);
char *ctk_get_portal_session_path (GDBusConnection  *connection,
                                   char            **token);
guint ctk_get_portal_interface_version (GDBusConnection *connection,
                                        const char      *interface_name);

#ifdef G_OS_WIN32
void _ctk_load_dll_with_libctk3_manifest (const char *dllname);
#endif

gboolean        ctk_simulate_touchscreen (void);

guint ctk_get_display_debug_flags (GdkDisplay *display);

#ifdef G_ENABLE_DEBUG

#define CTK_DISPLAY_DEBUG_CHECK(display,type) G_UNLIKELY (ctk_get_display_debug_flags (display) & CTK_DEBUG_##type)

#else

#define CTK_DISPLAY_DEBUG_CHECK(display,type) 0

#endif /* G_ENABLE_DEBUG */

G_END_DECLS

#endif /* __CTK_PRIVATE_H__ */
