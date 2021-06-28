/* Cdk testing utilities
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

#ifndef __CDK_TEST_UTILS_H__
#define __CDK_TEST_UTILS_H__

#if !defined (__CDK_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdk.h> can be included directly."
#endif

#include <cdk/cdkwindow.h>

G_BEGIN_DECLS


/**
 * SECTION:cdktestutils
 * @Short_description: Test utilities
 * @Title: Testing
 *
 * The functions in this section are intended to be used in test programs.
 * They allow to simulate some user input.
 */


/* --- Cdk Test Utility API --- */
CDK_AVAILABLE_IN_ALL
void            cdk_test_render_sync            (CdkWindow      *window);
CDK_AVAILABLE_IN_ALL
gboolean        cdk_test_simulate_key           (CdkWindow      *window,
                                                 gint            x,
                                                 gint            y,
                                                 guint           keyval,
                                                 CdkModifierType modifiers,
                                                 CdkEventType    key_pressrelease);
CDK_AVAILABLE_IN_ALL
gboolean        cdk_test_simulate_button        (CdkWindow      *window,
                                                 gint            x,
                                                 gint            y,
                                                 guint           button, /*1..3*/
                                                 CdkModifierType modifiers,
                                                 CdkEventType    button_pressrelease);

G_END_DECLS

#endif /* __CDK_TEST_UTILS_H__ */
