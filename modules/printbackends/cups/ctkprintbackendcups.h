/* CTK - The GIMP Toolkit
 * ctkprintbackendcups.h: Default implementation of CtkPrintBackend for the Common Unix Print System (CUPS)
 * Copyright (C) 2006, 2007 Red Hat, Inc.
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

#ifndef __CTK_PRINT_BACKEND_CUPS_H__
#define __CTK_PRINT_BACKEND_CUPS_H__

#include <glib-object.h>
#include "ctkprintbackend.h"

G_BEGIN_DECLS

#define CTK_TYPE_PRINT_BACKEND_CUPS             (ctk_print_backend_cups_get_type ())
#define CTK_PRINT_BACKEND_CUPS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_PRINT_BACKEND_CUPS, CtkPrintBackendCups))
#define CTK_IS_PRINT_BACKEND_CUPS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_PRINT_BACKEND_CUPS))

typedef struct _CtkPrintBackendCups      CtkPrintBackendCups;

CtkPrintBackend *ctk_print_backend_cups_new      (void);
GType          ctk_print_backend_cups_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __CTK_PRINT_BACKEND_CUPS_H__ */


