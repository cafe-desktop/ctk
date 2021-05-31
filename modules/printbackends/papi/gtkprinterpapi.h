/* GtkPrinterPapi
 * Copyright (C) 2006 John (J5) Palmieri <johnp@redhat.com>
 * Copyright (C) 2009 Ghee Teo <ghee.teo@sun.com>
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
#ifndef __CTK_PRINTER_PAPI_H__
#define __CTK_PRINTER_PAPI_H__

#include <glib.h>
#include <glib-object.h>
#include <papi.h>

#include "gtkunixprint.h"

G_BEGIN_DECLS

#define CTK_TYPE_PRINTER_PAPI                  (ctk_printer_papi_get_type ())
#define CTK_PRINTER_PAPI(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_PRINTER_PAPI, GtkPrinterPapi))
#define CTK_PRINTER_PAPI_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_PRINTER_PAPI, GtkPrinterPapiClass))
#define CTK_IS_PRINTER_PAPI(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_PRINTER_PAPI))
#define CTK_IS_PRINTER_PAPI_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_PRINTER_PAPI))
#define CTK_PRINTER_PAPI_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_PRINTER_PAPI, GtkPrinterPapiClass))

typedef struct _GtkPrinterPapi	        GtkPrinterPapi;
typedef struct _GtkPrinterPapiClass     GtkPrinterPapiClass;
typedef struct _GtkPrinterPapiPrivate   GtkPrinterPapiPrivate;

struct _GtkPrinterPapi
{
  GtkPrinter parent_instance;

  gchar *printer_name;
};

struct _GtkPrinterPapiClass
{
  GtkPrinterClass parent_class;

};

GType                    ctk_printer_papi_get_type      (void) G_GNUC_CONST;
void                     ctk_printer_papi_register_type (GTypeModule     *module);
GtkPrinterPapi          *ctk_printer_papi_new           (const char      *name, GtkPrintBackend *backend);

G_END_DECLS

#endif /* __CTK_PRINTER_PAPI_H__ */
