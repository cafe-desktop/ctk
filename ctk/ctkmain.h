/* CTK - The GIMP Toolkit
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

#ifndef __CTK_MAIN_H__
#define __CTK_MAIN_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <cdk/cdk.h>
#include <ctk/ctkwidget.h>
#ifdef G_PLATFORM_WIN32
#include <ctk/ctkbox.h>
#include <ctk/ctkwindow.h>
#endif

G_BEGIN_DECLS

/**
 * CTK_PRIORITY_RESIZE: (value 110)
 *
 * Use this priority for functionality related to size allocation.
 *
 * It is used internally by CTK+ to compute the sizes of widgets.
 * This priority is higher than %CDK_PRIORITY_REDRAW to avoid
 * resizing a widget which was just redrawn.
 */
#define CTK_PRIORITY_RESIZE (G_PRIORITY_HIGH_IDLE + 10)

/**
 * CtkKeySnoopFunc:
 * @grab_widget: the widget to which the event will be delivered
 * @event: the key event
 * @func_data: (closure): data supplied to ctk_key_snooper_install()
 *
 * Key snooper functions are called before normal event delivery.
 * They can be used to implement custom key event handling.
 *
 * Returns: %TRUE to stop further processing of @event, %FALSE to continue.
 */
typedef gint (*CtkKeySnoopFunc) (CtkWidget   *grab_widget,
                                 CdkEventKey *event,
                                 gpointer     func_data);

/* CTK+ version
 */
CDK_AVAILABLE_IN_ALL
guint ctk_get_major_version (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
guint ctk_get_minor_version (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
guint ctk_get_micro_version (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
guint ctk_get_binary_age    (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
guint ctk_get_interface_age (void) G_GNUC_CONST;

#define ctk_major_version ctk_get_major_version ()
#define ctk_minor_version ctk_get_minor_version ()
#define ctk_micro_version ctk_get_micro_version ()
#define ctk_binary_age ctk_get_binary_age ()
#define ctk_interface_age ctk_get_interface_age ()

CDK_AVAILABLE_IN_ALL
const gchar* ctk_check_version (guint   required_major,
                                guint   required_minor,
                                guint   required_micro);


/* Initialization, exit, mainloop and miscellaneous routines
 */

CDK_AVAILABLE_IN_ALL
gboolean ctk_parse_args           (int    *argc,
                                   char ***argv);

CDK_AVAILABLE_IN_ALL
void     ctk_init                 (int    *argc,
                                   char ***argv);

CDK_AVAILABLE_IN_ALL
gboolean ctk_init_check           (int    *argc,
                                   char ***argv);

CDK_AVAILABLE_IN_ALL
gboolean ctk_init_with_args       (gint                 *argc,
                                   gchar              ***argv,
                                   const gchar          *parameter_string,
                                   const GOptionEntry   *entries,
                                   const gchar          *translation_domain,
                                   GError              **error);

CDK_AVAILABLE_IN_ALL
GOptionGroup *ctk_get_option_group (gboolean open_default_display);

#ifdef G_OS_WIN32

/* Variants that are used to check for correct struct packing
 * when building CTK+-using code.
 */
CDK_AVAILABLE_IN_ALL
void     ctk_init_abi_check       (int    *argc,
                                   char ***argv,
                                   int     num_checks,
                                   size_t  sizeof_CtkWindow,
                                   size_t  sizeof_CtkBox);
CDK_AVAILABLE_IN_ALL
gboolean ctk_init_check_abi_check (int    *argc,
                                   char ***argv,
                                   int     num_checks,
                                   size_t  sizeof_CtkWindow,
                                   size_t  sizeof_CtkBox);

#define ctk_init(argc, argv) ctk_init_abi_check (argc, argv, 2, sizeof (CtkWindow), sizeof (CtkBox))
#define ctk_init_check(argc, argv) ctk_init_check_abi_check (argc, argv, 2, sizeof (CtkWindow), sizeof (CtkBox))

#endif

CDK_AVAILABLE_IN_ALL
void           ctk_disable_setlocale    (void);
CDK_AVAILABLE_IN_ALL
PangoLanguage *ctk_get_default_language (void);
CDK_AVAILABLE_IN_3_12
CtkTextDirection ctk_get_locale_direction (void);
CDK_AVAILABLE_IN_ALL
gboolean       ctk_events_pending       (void);

CDK_AVAILABLE_IN_ALL
void       ctk_main_do_event       (CdkEvent           *event);
CDK_AVAILABLE_IN_ALL
void       ctk_main                (void);
CDK_AVAILABLE_IN_ALL
guint      ctk_main_level          (void);
CDK_AVAILABLE_IN_ALL
void       ctk_main_quit           (void);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_main_iteration      (void);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_main_iteration_do   (gboolean            blocking);

CDK_AVAILABLE_IN_ALL
gboolean   ctk_true                (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
gboolean   ctk_false               (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_ALL
void       ctk_grab_add            (CtkWidget          *widget);
CDK_AVAILABLE_IN_ALL
CtkWidget* ctk_grab_get_current    (void);
CDK_AVAILABLE_IN_ALL
void       ctk_grab_remove         (CtkWidget          *widget);

CDK_AVAILABLE_IN_ALL
void       ctk_device_grab_add     (CtkWidget          *widget,
                                    CdkDevice          *device,
                                    gboolean            block_others);
CDK_AVAILABLE_IN_ALL
void       ctk_device_grab_remove  (CtkWidget          *widget,
                                    CdkDevice          *device);

CDK_DEPRECATED_IN_3_4
guint      ctk_key_snooper_install (CtkKeySnoopFunc snooper,
                                    gpointer        func_data);
CDK_DEPRECATED_IN_3_4
void       ctk_key_snooper_remove  (guint           snooper_handler_id);

CDK_AVAILABLE_IN_ALL
CdkEvent * ctk_get_current_event        (void);
CDK_AVAILABLE_IN_ALL
guint32    ctk_get_current_event_time   (void);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_get_current_event_state  (CdkModifierType *state);
CDK_AVAILABLE_IN_ALL
CdkDevice *ctk_get_current_event_device (void);

CDK_AVAILABLE_IN_ALL
CtkWidget *ctk_get_event_widget         (CdkEvent        *event);

CDK_AVAILABLE_IN_ALL
void       ctk_propagate_event          (CtkWidget       *widget,
                                         CdkEvent        *event);


G_END_DECLS

#endif /* __CTK_MAIN_H__ */
