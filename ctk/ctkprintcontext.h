/* CTK - The GIMP Toolkit
 * ctkprintcontext.h: Print Context
 * Copyright (C) 2006, Red Hat, Inc.
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

#ifndef __CTK_PRINT_CONTEXT_H__
#define __CTK_PRINT_CONTEXT_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <pango/pango.h>
#include <ctk/ctkpagesetup.h>


G_BEGIN_DECLS

typedef struct _CtkPrintContext CtkPrintContext;

#define CTK_TYPE_PRINT_CONTEXT    (ctk_print_context_get_type ())
#define CTK_PRINT_CONTEXT(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_PRINT_CONTEXT, CtkPrintContext))
#define CTK_IS_PRINT_CONTEXT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_PRINT_CONTEXT))

CDK_AVAILABLE_IN_ALL
GType          ctk_print_context_get_type (void) G_GNUC_CONST;


/* Rendering */
CDK_AVAILABLE_IN_ALL
cairo_t      *ctk_print_context_get_cairo_context    (CtkPrintContext *context);

CDK_AVAILABLE_IN_ALL
CtkPageSetup *ctk_print_context_get_page_setup       (CtkPrintContext *context);
CDK_AVAILABLE_IN_ALL
gdouble       ctk_print_context_get_width            (CtkPrintContext *context);
CDK_AVAILABLE_IN_ALL
gdouble       ctk_print_context_get_height           (CtkPrintContext *context);
CDK_AVAILABLE_IN_ALL
gdouble       ctk_print_context_get_dpi_x            (CtkPrintContext *context);
CDK_AVAILABLE_IN_ALL
gdouble       ctk_print_context_get_dpi_y            (CtkPrintContext *context);
CDK_AVAILABLE_IN_ALL
gboolean      ctk_print_context_get_hard_margins     (CtkPrintContext *context,
						      gdouble         *top,
						      gdouble         *bottom,
						      gdouble         *left,
						      gdouble         *right);

/* Fonts */
CDK_AVAILABLE_IN_ALL
PangoFontMap *ctk_print_context_get_pango_fontmap    (CtkPrintContext *context);
CDK_AVAILABLE_IN_ALL
PangoContext *ctk_print_context_create_pango_context (CtkPrintContext *context);
CDK_AVAILABLE_IN_ALL
PangoLayout  *ctk_print_context_create_pango_layout  (CtkPrintContext *context);

/* Needed for preview implementations */
CDK_AVAILABLE_IN_ALL
void         ctk_print_context_set_cairo_context     (CtkPrintContext *context,
						      cairo_t         *cr,
						      double           dpi_x,
						      double           dpi_y);

G_END_DECLS

#endif /* __CTK_PRINT_CONTEXT_H__ */
