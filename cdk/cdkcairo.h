/* CDK - The GIMP Drawing Kit
 * Copyright (C) 2005 Red Hat, Inc. 
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CDK_CAIRO_H__
#define __CDK_CAIRO_H__

#if !defined (__CDK_H_INSIDE__) && !defined (CDK_COMPILATION)
#error "Only <cdk/cdk.h> can be included directly."
#endif

#include <cdk/cdkversionmacros.h>
#include <cdk/cdkcolor.h>
#include <cdk/cdkrgba.h>
#include <cdk/cdkdrawingcontext.h>
#include <cdk/gdkpixbuf.h>
#include <pango/pangocairo.h>

G_BEGIN_DECLS

CDK_AVAILABLE_IN_ALL
cairo_t  * cdk_cairo_create             (CdkWindow          *window);

CDK_AVAILABLE_IN_ALL
gboolean   cdk_cairo_get_clip_rectangle (cairo_t            *cr,
                                         CdkRectangle       *rect);

CDK_AVAILABLE_IN_ALL
void       cdk_cairo_set_source_rgba    (cairo_t              *cr,
                                         const CdkRGBA        *rgba);
CDK_AVAILABLE_IN_ALL
void       cdk_cairo_set_source_pixbuf  (cairo_t              *cr,
                                         const GdkPixbuf      *pixbuf,
                                         gdouble               pixbuf_x,
                                         gdouble               pixbuf_y);
CDK_AVAILABLE_IN_ALL
void       cdk_cairo_set_source_window  (cairo_t              *cr,
                                         CdkWindow            *window,
                                         gdouble               x,
                                         gdouble               y);

CDK_AVAILABLE_IN_ALL
void       cdk_cairo_rectangle          (cairo_t              *cr,
                                         const CdkRectangle   *rectangle);
CDK_AVAILABLE_IN_ALL
void       cdk_cairo_region             (cairo_t              *cr,
                                         const cairo_region_t *region);

CDK_AVAILABLE_IN_ALL
cairo_region_t *
           cdk_cairo_region_create_from_surface
                                        (cairo_surface_t      *surface);

CDK_AVAILABLE_IN_ALL
void       cdk_cairo_set_source_color   (cairo_t              *cr,
                                         const CdkColor       *color);

CDK_AVAILABLE_IN_3_10
cairo_surface_t * cdk_cairo_surface_create_from_pixbuf      (const GdkPixbuf *pixbuf,
                                                             int scale,
                                                             CdkWindow *for_window);
CDK_AVAILABLE_IN_3_16
void       cdk_cairo_draw_from_gl (cairo_t              *cr,
                                   CdkWindow            *window,
                                   int                   source,
                                   int                   source_type,
                                   int                   buffer_scale,
                                   int                   x,
                                   int                   y,
                                   int                   width,
                                   int                   height);

CDK_AVAILABLE_IN_3_22
CdkDrawingContext *     cdk_cairo_get_drawing_context   (cairo_t *cr);

G_END_DECLS

#endif /* __CDK_CAIRO_H__ */
