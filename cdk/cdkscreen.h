/*
 * cdkscreen.h
 *
 * Copyright 2001 Sun Microsystems Inc.
 *
 * Erwann Chenede <erwann.chenede@sun.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CDK_SCREEN_H__
#define __CDK_SCREEN_H__

#if !defined (__CDK_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdk.h> can be included directly."
#endif

#include <cairo.h>
#include <cdk/cdkversionmacros.h>
#include <cdk/cdktypes.h>
#include <cdk/cdkdisplay.h>

G_BEGIN_DECLS

#define CDK_TYPE_SCREEN            (cdk_screen_get_type ())
#define CDK_SCREEN(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), CDK_TYPE_SCREEN, CdkScreen))
#define CDK_IS_SCREEN(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), CDK_TYPE_SCREEN))


CDK_AVAILABLE_IN_ALL
GType        cdk_screen_get_type              (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_ALL
CdkVisual *  cdk_screen_get_system_visual     (CdkScreen   *screen);
CDK_AVAILABLE_IN_ALL
CdkVisual *  cdk_screen_get_rgba_visual       (CdkScreen   *screen);
CDK_AVAILABLE_IN_ALL
gboolean     cdk_screen_is_composited         (CdkScreen   *screen);

CDK_AVAILABLE_IN_ALL
CdkWindow *  cdk_screen_get_root_window       (CdkScreen   *screen);
CDK_AVAILABLE_IN_ALL
CdkDisplay * cdk_screen_get_display           (CdkScreen   *screen);
CDK_DEPRECATED_IN_3_22
gint         cdk_screen_get_number            (CdkScreen   *screen);
CDK_DEPRECATED_IN_3_22
gint         cdk_screen_get_width             (CdkScreen   *screen);
CDK_DEPRECATED_IN_3_22
gint         cdk_screen_get_height            (CdkScreen   *screen);
CDK_DEPRECATED_IN_3_22
gint         cdk_screen_get_width_mm          (CdkScreen   *screen);
CDK_DEPRECATED_IN_3_22
gint         cdk_screen_get_height_mm         (CdkScreen   *screen);

CDK_AVAILABLE_IN_ALL
GList *      cdk_screen_list_visuals          (CdkScreen   *screen);
CDK_AVAILABLE_IN_ALL
GList *      cdk_screen_get_toplevel_windows  (CdkScreen   *screen);
CDK_DEPRECATED_IN_3_22
gchar *      cdk_screen_make_display_name     (CdkScreen   *screen);

CDK_DEPRECATED_IN_3_22_FOR(cdk_display_get_n_monitors)
gint         cdk_screen_get_n_monitors        (CdkScreen    *screen);
CDK_DEPRECATED_IN_3_22_FOR(cdk_display_get_primary_monitor)
gint         cdk_screen_get_primary_monitor   (CdkScreen    *screen);
CDK_DEPRECATED_IN_3_22_FOR(cdk_monitor_get_geometry)
void         cdk_screen_get_monitor_geometry  (CdkScreen    *screen,
                                               gint          monitor_num,
                                               CdkRectangle *dest);
CDK_DEPRECATED_IN_3_22_FOR(cdk_monitor_get_workarea)
void         cdk_screen_get_monitor_workarea  (CdkScreen    *screen,
                                               gint          monitor_num,
                                               CdkRectangle *dest);

CDK_DEPRECATED_IN_3_22_FOR(cdk_display_get_monitor_at_point)
gint          cdk_screen_get_monitor_at_point  (CdkScreen *screen,
                                                gint       x,
                                                gint       y);
CDK_AVAILABLE_IN_ALL
gint          cdk_screen_get_monitor_at_window (CdkScreen *screen,
                                                CdkWindow *window);
CDK_DEPRECATED_IN_3_22_FOR(cdk_monitor_get_width_mm)
gint          cdk_screen_get_monitor_width_mm  (CdkScreen *screen,
                                                gint       monitor_num);
CDK_DEPRECATED_IN_3_22_FOR(cdk_monitor_get_height_mm)
gint          cdk_screen_get_monitor_height_mm (CdkScreen *screen,
                                                gint       monitor_num);
CDK_DEPRECATED_IN_3_22_FOR(cdk_monitor_get_model)
gchar *       cdk_screen_get_monitor_plug_name (CdkScreen *screen,
                                                gint       monitor_num);
CDK_DEPRECATED_IN_3_22_FOR(cdk_monitor_get_scale_factor)
gint          cdk_screen_get_monitor_scale_factor (CdkScreen *screen,
                                                   gint       monitor_num);

CDK_AVAILABLE_IN_ALL
CdkScreen *cdk_screen_get_default (void);

CDK_AVAILABLE_IN_ALL
gboolean   cdk_screen_get_setting (CdkScreen   *screen,
                                   const gchar *name,
                                   GValue      *value);

CDK_AVAILABLE_IN_ALL
void                        cdk_screen_set_font_options (CdkScreen                  *screen,
                                                         const cairo_font_options_t *options);
CDK_AVAILABLE_IN_ALL
const cairo_font_options_t *cdk_screen_get_font_options (CdkScreen                  *screen);

CDK_AVAILABLE_IN_ALL
void    cdk_screen_set_resolution (CdkScreen *screen,
                                   gdouble    dpi);
CDK_AVAILABLE_IN_ALL
gdouble cdk_screen_get_resolution (CdkScreen *screen);

CDK_DEPRECATED_IN_3_22
CdkWindow *cdk_screen_get_active_window (CdkScreen *screen);
CDK_AVAILABLE_IN_ALL
GList     *cdk_screen_get_window_stack  (CdkScreen *screen);

G_END_DECLS

#endif  /* __CDK_SCREEN_H__ */
