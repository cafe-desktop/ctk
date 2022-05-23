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

#ifndef __CDK_MAIN_H__
#define __CDK_MAIN_H__

#if !defined (__CDK_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdk.h> can be included directly."
#endif

#include <cdk/cdkversionmacros.h>
#include <cdk/cdktypes.h>

G_BEGIN_DECLS


/* Initialization, exit and events
 */

#define CDK_PRIORITY_EVENTS (G_PRIORITY_DEFAULT)

CDK_AVAILABLE_IN_ALL
void                  cdk_parse_args                      (gint           *argc,
                                                           gchar        ***argv);
CDK_AVAILABLE_IN_ALL
void                  cdk_init                            (gint           *argc,
                                                           gchar        ***argv);
CDK_AVAILABLE_IN_ALL
gboolean              cdk_init_check                      (gint           *argc,
                                                           gchar        ***argv);
CDK_DEPRECATED_IN_3_16
void                  cdk_add_option_entries_libctk_only  (GOptionGroup   *group);
CDK_DEPRECATED_IN_3_16
void                  cdk_pre_parse_libctk_only           (void);

CDK_AVAILABLE_IN_ALL
const gchar *         cdk_get_program_class               (void);
CDK_AVAILABLE_IN_ALL
void                  cdk_set_program_class               (const gchar    *program_class);

CDK_AVAILABLE_IN_ALL
void                  cdk_notify_startup_complete         (void);
CDK_AVAILABLE_IN_ALL
void                  cdk_notify_startup_complete_with_id (const gchar* startup_id);

/* Push and pop error handlers for X errors
 */
CDK_DEPRECATED_IN_3_22_FOR(cdk_x11_display_error_trap_push)
void                           cdk_error_trap_push        (void);
/* warn unused because you could use pop_ignored otherwise */
CDK_DEPRECATED_IN_3_22_FOR(cdk_x11_display_error_trap_pop)
G_GNUC_WARN_UNUSED_RESULT gint cdk_error_trap_pop         (void);
CDK_DEPRECATED_IN_3_22_FOR(cdk_x11_display_error_trap_pop_ignored)
void                           cdk_error_trap_pop_ignored (void);


CDK_AVAILABLE_IN_ALL
const gchar *         cdk_get_display_arg_name (void);

CDK_DEPRECATED_IN_3_8_FOR(cdk_display_get_name (cdk_display_get_default ()))
gchar*        cdk_get_display        (void);

#ifndef CDK_MULTIDEVICE_SAFE
CDK_DEPRECATED_IN_3_0_FOR(cdk_device_grab)
CdkGrabStatus cdk_pointer_grab       (CdkWindow    *window,
                                      gboolean      owner_events,
                                      CdkEventMask  event_mask,
                                      CdkWindow    *confine_to,
                                      CdkCursor    *cursor,
                                      guint32       time_);
CDK_AVAILABLE_IN_ALL
CdkGrabStatus cdk_keyboard_grab      (CdkWindow    *window,
                                      gboolean      owner_events,
                                      guint32       time_);
#endif /* CDK_MULTIDEVICE_SAFE */

#ifndef CDK_MULTIDEVICE_SAFE
CDK_DEPRECATED_IN_3_0_FOR(cdk_device_ungrab)
void          cdk_pointer_ungrab     (guint32       time_);
CDK_DEPRECATED_IN_3_0_FOR(cdk_device_ungrab)
void          cdk_keyboard_ungrab    (guint32       time_);
CDK_DEPRECATED_IN_3_0_FOR(cdk_display_device_is_grabbed)
gboolean      cdk_pointer_is_grabbed (void);
#endif /* CDK_MULTIDEVICE_SAFE */

CDK_DEPRECATED_IN_3_22
gint cdk_screen_width  (void) G_GNUC_CONST;
CDK_DEPRECATED_IN_3_22
gint cdk_screen_height (void) G_GNUC_CONST;

CDK_DEPRECATED_IN_3_22
gint cdk_screen_width_mm  (void) G_GNUC_CONST;
CDK_DEPRECATED_IN_3_22
gint cdk_screen_height_mm (void) G_GNUC_CONST;

CDK_DEPRECATED_IN_3_22_FOR(cdk_display_set_double_click_time)
void cdk_set_double_click_time (guint msec);

CDK_DEPRECATED_IN_3_22_FOR(cdk_display_beep)
void cdk_beep (void);

CDK_DEPRECATED_IN_3_22_FOR(cdk_display_flush)
void cdk_flush (void);

CDK_AVAILABLE_IN_ALL
void cdk_disable_multidevice (void);

CDK_AVAILABLE_IN_3_10
void cdk_set_allowed_backends (const gchar *backends);

G_END_DECLS

#endif /* __CDK_MAIN_H__ */
