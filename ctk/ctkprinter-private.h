/* GTK - The GIMP Toolkit
 * ctkprintoperation.h: Print Operation
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

#ifndef __CTK_PRINTER_PRIVATE_H__
#define __CTK_PRINTER_PRIVATE_H__

#include <ctk/ctk.h>
#include <ctk/ctkunixprint.h>
#include "ctkprinteroptionset.h"

G_BEGIN_DECLS

CtkPrinterOptionSet *_ctk_printer_get_options               (CtkPrinter          *printer,
							     CtkPrintSettings    *settings,
							     CtkPageSetup        *page_setup,
							     CtkPrintCapabilities capabilities);
gboolean             _ctk_printer_mark_conflicts            (CtkPrinter          *printer,
							     CtkPrinterOptionSet *options);
void                 _ctk_printer_get_settings_from_options (CtkPrinter          *printer,
							     CtkPrinterOptionSet *options,
							     CtkPrintSettings    *settings);
void                 _ctk_printer_prepare_for_print         (CtkPrinter          *printer,
							     CtkPrintJob         *print_job,
							     CtkPrintSettings    *settings,
							     CtkPageSetup        *page_setup);
cairo_surface_t *    _ctk_printer_create_cairo_surface      (CtkPrinter          *printer,
							     CtkPrintSettings    *settings,
							     gdouble              width,
							     gdouble              height,
							     GIOChannel          *cache_io);
GHashTable *         _ctk_printer_get_custom_widgets        (CtkPrinter          *printer);
gboolean             _ctk_printer_get_hard_margins_for_paper_size (CtkPrinter       *printer,
								   CtkPaperSize     *paper_size,
								   gdouble          *top,
								   gdouble          *bottom,
								   gdouble          *left,
								   gdouble          *right);

/* CtkPrintJob private methods: */
GDK_AVAILABLE_IN_ALL
void ctk_print_job_set_status (CtkPrintJob   *job,
			       CtkPrintStatus status);

G_END_DECLS
#endif /* __CTK_PRINT_OPERATION_PRIVATE_H__ */
