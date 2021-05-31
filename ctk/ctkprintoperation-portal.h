/* CTK - The GIMP Toolkit
 * Copyright (C) 2016, Red Hat, Inc.
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

#ifndef __CTK_PRINT_OPERATION_PORTAL_H__
#define __CTK_PRINT_OPERATION_PORTAL_H__

#include "ctkprintoperation.h"

G_BEGIN_DECLS

CtkPrintOperationResult ctk_print_operation_portal_run_dialog             (CtkPrintOperation           *op,
                                                                           gboolean                     show_dialog,
                                                                           CtkWindow                   *parent,
                                                                           gboolean                    *do_print);
void                    ctk_print_operation_portal_run_dialog_async       (CtkPrintOperation           *op,
                                                                           gboolean                     show_dialog,
                                                                           CtkWindow                   *parent,
                                                                           CtkPrintOperationPrintFunc   print_cb);
void                    ctk_print_operation_portal_launch_preview         (CtkPrintOperation           *op,
                                                                           cairo_surface_t             *surface,
                                                                           CtkWindow                   *parent,
                                                                           const char                  *filename);

G_END_DECLS

#endif /* __CTK_PRINT_OPERATION_PORTAL_H__ */
