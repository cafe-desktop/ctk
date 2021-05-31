/* CtkPrinterPapi
 * Copyright (C) 2006 John (J5) Palmieri  <johnp@redhat.com>
 * Copyright (C) 2009 Ghee Teo <ghee.teo@sun.com>
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

#include "config.h"
#include "ctkprinterpapi.h"

static void ctk_printer_papi_init       (CtkPrinterPapi      *printer);
static void ctk_printer_papi_class_init (CtkPrinterPapiClass *class);
static void ctk_printer_papi_finalize   (GObject             *object);

static CtkPrinterClass *ctk_printer_papi_parent_class;
static GType ctk_printer_papi_type = 0;

void 
ctk_printer_papi_register_type (GTypeModule *module)
{
  const GTypeInfo object_info =
  {
    sizeof (CtkPrinterPapiClass),
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) ctk_printer_papi_class_init,
    NULL,           /* class_finalize */
    NULL,           /* class_data */
    sizeof (CtkPrinterPapi),
    0,              /* n_preallocs */
    (GInstanceInitFunc) ctk_printer_papi_init,
  };

 ctk_printer_papi_type = g_type_module_register_type (module,
                                                      CTK_TYPE_PRINTER,
                                                      "CtkPrinterPapi",
                                                      &object_info, 0);
}

GType
ctk_printer_papi_get_type (void)
{
  return ctk_printer_papi_type;
}

static void
ctk_printer_papi_class_init (CtkPrinterPapiClass *class)
{
  GObjectClass *object_class = (GObjectClass *) class;
	
  ctk_printer_papi_parent_class = g_type_class_peek_parent (class);

  object_class->finalize = ctk_printer_papi_finalize;
}

static void
ctk_printer_papi_init (CtkPrinterPapi *printer)
{
  printer->printer_name = NULL;
}

static void
ctk_printer_papi_finalize (GObject *object)
{
  CtkPrinterPapi *printer;

  g_return_if_fail (object != NULL);

  printer = CTK_PRINTER_PAPI (object);

  g_free(printer->printer_name);

  G_OBJECT_CLASS (ctk_printer_papi_parent_class)->finalize (object);
}

/**
 * ctk_printer_papi_new:
 *
 * Creates a new #CtkPrinterPapi.
 *
 * Returns: a new #CtkPrinterPapi
 *
 * Since: 2.10
 **/
CtkPrinterPapi *
ctk_printer_papi_new (const char      *name,
		      CtkPrintBackend *backend)
{
  GObject *result;
  CtkPrinterPapi *pp;
  
  result = g_object_new (CTK_TYPE_PRINTER_PAPI,
			 "name", name,
			 "backend", backend,
			 "is-virtual", TRUE,
                         NULL);
  pp = CTK_PRINTER_PAPI(result);

  pp->printer_name = g_strdup (name);

  return (CtkPrinterPapi *) pp;
}

