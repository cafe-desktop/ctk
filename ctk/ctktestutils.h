/* Ctk+ testing utilities
 * Copyright (C) 2007 Imendio AB
 * Authors: Tim Janik
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

#ifndef __CTK_TEST_UTILS_H__
#define __CTK_TEST_UTILS_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkwidget.h>
#include <ctk/ctkspinbutton.h>

G_BEGIN_DECLS

/* --- Ctk+ Test Utility API --- */
CDK_AVAILABLE_IN_ALL
void            ctk_test_init                   (int            *argcp,
                                                 char         ***argvp,
                                                 ...);
CDK_AVAILABLE_IN_ALL
void            ctk_test_register_all_types     (void);
CDK_AVAILABLE_IN_ALL
const GType*    ctk_test_list_all_types         (guint          *n_types);
CDK_AVAILABLE_IN_ALL
CtkWidget*      ctk_test_find_widget            (CtkWidget      *widget,
                                                 const gchar    *label_pattern,
                                                 GType           widget_type);
CDK_DEPRECATED_IN_3_20
CtkWidget*      ctk_test_create_widget          (GType           widget_type,
                                                 const gchar    *first_property_name,
                                                 ...);
CDK_DEPRECATED_IN_3_20
CtkWidget*      ctk_test_create_simple_window   (const gchar    *window_title,
                                                 const gchar    *dialog_text);
CDK_DEPRECATED_IN_3_20
CtkWidget*      ctk_test_display_button_window  (const gchar    *window_title,
                                                 const gchar    *dialog_text,
                                                 ...); /* NULL terminated list of (label, &int) pairs */
CDK_DEPRECATED_IN_3_20
void            ctk_test_slider_set_perc        (CtkWidget      *widget, /* CtkRange-alike */
                                                 double          percentage);
CDK_DEPRECATED_IN_3_20
double          ctk_test_slider_get_value       (CtkWidget      *widget);
CDK_DEPRECATED_IN_3_20
gboolean        ctk_test_spin_button_click      (CtkSpinButton  *spinner,
                                                 guint           button,
                                                 gboolean        upwards);
CDK_AVAILABLE_IN_3_10
void            ctk_test_widget_wait_for_draw   (CtkWidget      *widget);
CDK_DEPRECATED_IN_3_20
gboolean        ctk_test_widget_click           (CtkWidget      *widget,
                                                 guint           button,
                                                 CdkModifierType modifiers);
CDK_AVAILABLE_IN_ALL
gboolean        ctk_test_widget_send_key        (CtkWidget      *widget,
                                                 guint           keyval,
                                                 CdkModifierType modifiers);
/* operate on CtkEntry, CtkText, CtkTextView or CtkLabel */
CDK_DEPRECATED_IN_3_20
void            ctk_test_text_set               (CtkWidget      *widget,
                                                 const gchar    *string);
CDK_DEPRECATED_IN_3_20
gchar*          ctk_test_text_get               (CtkWidget      *widget);

/* --- Ctk+ Test low-level API --- */
CDK_AVAILABLE_IN_ALL
CtkWidget*      ctk_test_find_sibling           (CtkWidget      *base_widget,
                                                 GType           widget_type);
CDK_AVAILABLE_IN_ALL
CtkWidget*      ctk_test_find_label             (CtkWidget      *widget,
                                                 const gchar    *label_pattern);
G_END_DECLS

#endif /* __CTK_TEST_UTILS_H__ */
