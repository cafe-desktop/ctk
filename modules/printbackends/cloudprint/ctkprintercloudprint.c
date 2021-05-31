/* ctkprintercloudprint.c: Google Cloud Print -specific Printer class,
 * CtkPrinterCloudprint
 * Copyright (C) 2014, Red Hat, Inc.
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

#include <glib/gi18n-lib.h>
#include <ctk/ctkintl.h>

#include "ctkprintercloudprint.h"
#include "ctkcloudprintaccount.h"

typedef struct _CtkPrinterCloudprintClass CtkPrinterCloudprintClass;

#define CTK_PRINTER_CLOUDPRINT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_PRINTER_CLOUDPRINT, CtkPrinterCloudprintClass))
#define CTK_IS_PRINTER_CLOUDPRINT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_PRINTER_CLOUDPRINT))
#define CTK_PRINTER_CLOUDPRINT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_PRINTER_CLOUDPRINT, CtkPrinterCloudprintClass))

static CtkPrinterClass *ctk_printer_cloudprint_parent_class;
static GType printer_cloudprint_type = 0;

struct _CtkPrinterCloudprintClass
{
  CtkPrinterClass parent_class;
};

struct _CtkPrinterCloudprint
{
  CtkPrinter parent_instance;

  CtkCloudprintAccount *account;
  gchar *id;
};

enum {
  PROP_0,
  PROP_CLOUDPRINT_ACCOUNT,
  PROP_PRINTER_ID
};

static void ctk_printer_cloudprint_class_init	(CtkPrinterCloudprintClass *class);
static void ctk_printer_cloudprint_init		(CtkPrinterCloudprint *impl);
static void ctk_printer_cloudprint_finalize	(GObject *object);
static void ctk_printer_cloudprint_set_property	(GObject *object,
						 guint prop_id,
						 const GValue *value,
						 GParamSpec *pspec);
static void ctk_printer_cloudprint_get_property	(GObject *object,
						 guint prop_id,
						 GValue *value,
						 GParamSpec *pspec);

void
ctk_printer_cloudprint_register_type (GTypeModule *module)
{
  const GTypeInfo printer_cloudprint_info =
  {
    sizeof (CtkPrinterCloudprintClass),
    NULL,		/* base_init */
    NULL,		/* base_finalize */
    (GClassInitFunc) ctk_printer_cloudprint_class_init,
    NULL,		/* class_finalize */
    NULL,		/* class_data */
    sizeof (CtkPrinterCloudprint),
    0,		/* n_preallocs */
    (GInstanceInitFunc) ctk_printer_cloudprint_init,
  };

  printer_cloudprint_type = g_type_module_register_type (module,
							 CTK_TYPE_PRINTER,
							 "CtkPrinterCloudprint",
							 &printer_cloudprint_info, 0);
}

/*
 * CtkPrinterCloudprint
 */
GType
ctk_printer_cloudprint_get_type (void)
{
  return printer_cloudprint_type;
}

/**
 * ctk_printer_cloudprint_new:
 *
 * Creates a new #CtkPrinterCloudprint object. #CtkPrinterCloudprint
 * implements the #CtkPrinter interface and stores a reference to the
 * #CtkCloudprintAccount object and the printer-id to use
 *
 * Returns: the new #CtkPrinterCloudprint object
 **/
CtkPrinterCloudprint *
ctk_printer_cloudprint_new (const char *name,
			    gboolean is_virtual,
			    CtkPrintBackend *backend,
			    CtkCloudprintAccount *account,
			    const gchar *id)
{
  return g_object_new (CTK_TYPE_PRINTER_CLOUDPRINT,
		       "name", name,
		       "backend", backend,
		       "is-virtual", is_virtual,
		       "accepts-pdf", TRUE,
		       "cloudprint-account", account,
		       "printer-id", id,
		       NULL);
}

static void
ctk_printer_cloudprint_class_init (CtkPrinterCloudprintClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  ctk_printer_cloudprint_parent_class = g_type_class_peek_parent (klass);
  gobject_class->finalize = ctk_printer_cloudprint_finalize;
  gobject_class->set_property = ctk_printer_cloudprint_set_property;
  gobject_class->get_property = ctk_printer_cloudprint_get_property;

  g_object_class_install_property (G_OBJECT_CLASS (klass),
				   PROP_CLOUDPRINT_ACCOUNT,
				   g_param_spec_object ("cloudprint-account",
							P_("Cloud Print account"),
							P_("CtkCloudprintAccount instance"),
							CTK_TYPE_CLOUDPRINT_ACCOUNT,
							G_PARAM_READWRITE |
							G_PARAM_STATIC_STRINGS |
							G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (G_OBJECT_CLASS (klass),
				   PROP_PRINTER_ID,
				   g_param_spec_string ("printer-id",
							P_("Printer ID"),
							P_("Cloud Print printer ID"),
							"",
							G_PARAM_READWRITE |
							G_PARAM_STATIC_STRINGS |
							G_PARAM_CONSTRUCT_ONLY));
}

static void
ctk_printer_cloudprint_init (CtkPrinterCloudprint *printer)
{
  printer->account = NULL;
  printer->id = NULL;

  CTK_NOTE (PRINTING,
	    g_print ("Cloud Print Backend: +CtkPrinterCloudprint(%p)\n",
		     printer));
}

static void
ctk_printer_cloudprint_finalize (GObject *object)
{
  CtkPrinterCloudprint *printer;

  printer = CTK_PRINTER_CLOUDPRINT (object);

  CTK_NOTE (PRINTING,
	    g_print ("Cloud Print Backend: -CtkPrinterCloudprint(%p)\n",
		     printer));

  if (printer->account != NULL)
    g_object_unref (printer->account);

  g_free (printer->id);

  G_OBJECT_CLASS (ctk_printer_cloudprint_parent_class)->finalize (object);
}

static void
ctk_printer_cloudprint_set_property (GObject *object,
				     guint prop_id,
				     const GValue *value,
				     GParamSpec *pspec)
{
  CtkPrinterCloudprint *printer = CTK_PRINTER_CLOUDPRINT (object);

  switch (prop_id)
    {
    case PROP_CLOUDPRINT_ACCOUNT:
      printer->account = g_value_dup_object (value);
      break;

    case PROP_PRINTER_ID:
      printer->id = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_printer_cloudprint_get_property (GObject *object,
				     guint prop_id,
				     GValue *value,
				     GParamSpec *pspec)
{
  CtkPrinterCloudprint *printer = CTK_PRINTER_CLOUDPRINT (object);

  switch (prop_id)
    {
    case PROP_CLOUDPRINT_ACCOUNT:
      g_value_set_object (value, printer->account);
      break;

    case PROP_PRINTER_ID:
      g_value_set_string (value, printer->id);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}
