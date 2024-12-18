/* CtkPrinter
 * Copyright (C) 2006 John (J5) Palmieri  <johnp@redhat.com>
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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "ctkintl.h"
#include "ctkprivate.h"

#include "ctkprinter.h"
#include "ctkprinter-private.h"
#include "ctkprintbackend.h"
#include "ctkprintjob.h"

/**
 * SECTION:ctkprinter
 * @Short_description: Represents a printer
 * @Title: CtkPrinter
 *
 * A #CtkPrinter object represents a printer. You only need to
 * deal directly with printers if you use the non-portable
 * #CtkPrintUnixDialog API.
 *
 * A #CtkPrinter allows to get status information about the printer,
 * such as its description, its location, the number of queued jobs,
 * etc. Most importantly, a #CtkPrinter object can be used to create
 * a #CtkPrintJob object, which lets you print to the printer.
 *
 * Printing support was added in CTK+ 2.10.
 */


static void ctk_printer_finalize     (GObject *object);

struct _CtkPrinterPrivate
{
  gchar *name;
  gchar *location;
  gchar *description;
  gchar *icon_name;

  guint is_active         : 1;
  guint is_paused         : 1;
  guint is_accepting_jobs : 1;
  guint is_new            : 1;
  guint is_virtual        : 1;
  guint is_default        : 1;
  guint has_details       : 1;
  guint accepts_pdf       : 1;
  guint accepts_ps        : 1;

  gchar *state_message;  
  gint job_count;

  CtkPrintBackend *backend;
};

enum {
  DETAILS_ACQUIRED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_NAME,
  PROP_BACKEND,
  PROP_IS_VIRTUAL,
  PROP_STATE_MESSAGE,
  PROP_LOCATION,
  PROP_ICON_NAME,
  PROP_JOB_COUNT,
  PROP_ACCEPTS_PDF,
  PROP_ACCEPTS_PS,
  PROP_PAUSED,
  PROP_ACCEPTING_JOBS
};

static guint signals[LAST_SIGNAL] = { 0 };

static void ctk_printer_set_property (GObject      *object,
				      guint         prop_id,
				      const GValue *value,
				      GParamSpec   *pspec);
static void ctk_printer_get_property (GObject      *object,
				      guint         prop_id,
				      GValue       *value,
				      GParamSpec   *pspec);

G_DEFINE_TYPE_WITH_PRIVATE (CtkPrinter, ctk_printer, G_TYPE_OBJECT)

static void
ctk_printer_class_init (CtkPrinterClass *class)
{
  GObjectClass *object_class;
  object_class = (GObjectClass *) class;

  object_class->finalize = ctk_printer_finalize;

  object_class->set_property = ctk_printer_set_property;
  object_class->get_property = ctk_printer_get_property;

  g_object_class_install_property (G_OBJECT_CLASS (class),
                                   PROP_NAME,
                                   g_param_spec_string ("name",
						        P_("Name"),
						        P_("Name of the printer"),
						        "",
							CTK_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (G_OBJECT_CLASS (class),
                                   PROP_BACKEND,
                                   g_param_spec_object ("backend",
						        P_("Backend"),
						        P_("Backend for the printer"),
						        CTK_TYPE_PRINT_BACKEND,
							CTK_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (G_OBJECT_CLASS (class),
                                   PROP_IS_VIRTUAL,
                                   g_param_spec_boolean ("is-virtual",
							 P_("Is Virtual"),
							 P_("FALSE if this represents a real hardware printer"),
							 FALSE,
							 CTK_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (G_OBJECT_CLASS (class),
                                   PROP_ACCEPTS_PDF,
                                   g_param_spec_boolean ("accepts-pdf",
							 P_("Accepts PDF"),
							 P_("TRUE if this printer can accept PDF"),
							 FALSE,
							 CTK_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (G_OBJECT_CLASS (class),
                                   PROP_ACCEPTS_PS,
                                   g_param_spec_boolean ("accepts-ps",
							 P_("Accepts PostScript"),
							 P_("TRUE if this printer can accept PostScript"),
							 TRUE,
							 CTK_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (G_OBJECT_CLASS (class),
                                   PROP_STATE_MESSAGE,
                                   g_param_spec_string ("state-message",
						        P_("State Message"),
						        P_("String giving the current state of the printer"),
						        "",
							CTK_PARAM_READABLE));
  g_object_class_install_property (G_OBJECT_CLASS (class),
                                   PROP_LOCATION,
                                   g_param_spec_string ("location",
						        P_("Location"),
						        P_("The location of the printer"),
						        "",
							CTK_PARAM_READABLE));
  g_object_class_install_property (G_OBJECT_CLASS (class),
                                   PROP_ICON_NAME,
                                   g_param_spec_string ("icon-name",
						        P_("Icon Name"),
						        P_("The icon name to use for the printer"),
						        "",
							CTK_PARAM_READABLE));
  g_object_class_install_property (G_OBJECT_CLASS (class),
                                   PROP_JOB_COUNT,
				   g_param_spec_int ("job-count",
 						     P_("Job Count"),
 						     P_("Number of jobs queued in the printer"),
 						     0,
 						     G_MAXINT,
 						     0,
 						     CTK_PARAM_READABLE));

  /**
   * CtkPrinter:paused:
   *
   * This property is %TRUE if this printer is paused. 
   * A paused printer still accepts jobs, but it does 
   * not print them.
   *
   * Since: 2.14
   */
  g_object_class_install_property (G_OBJECT_CLASS (class),
                                   PROP_PAUSED,
                                   g_param_spec_boolean ("paused",
							 P_("Paused Printer"),
							 P_("TRUE if this printer is paused"),
							 FALSE,
							 CTK_PARAM_READABLE));
  /**
   * CtkPrinter:accepting-jobs:
   *
   * This property is %TRUE if the printer is accepting jobs.
   *
   * Since: 2.14
   */ 
  g_object_class_install_property (G_OBJECT_CLASS (class),
                                   PROP_ACCEPTING_JOBS,
                                   g_param_spec_boolean ("accepting-jobs",
							 P_("Accepting Jobs"),
							 P_("TRUE if this printer is accepting new jobs"),
							 TRUE,
							 CTK_PARAM_READABLE));

  /**
   * CtkPrinter::details-acquired:
   * @printer: the #CtkPrinter on which the signal is emitted
   * @success: %TRUE if the details were successfully acquired
   *
   * Gets emitted in response to a request for detailed information
   * about a printer from the print backend. The @success parameter
   * indicates if the information was actually obtained.
   *
   * Since: 2.10
   */
  signals[DETAILS_ACQUIRED] =
    g_signal_new (I_("details-acquired"),
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkPrinterClass, details_acquired),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
}

static void
ctk_printer_init (CtkPrinter *printer)
{
  CtkPrinterPrivate *priv;

  priv = printer->priv = ctk_printer_get_instance_private (printer);

  priv->name = NULL;
  priv->location = NULL;
  priv->description = NULL;
  priv->icon_name = g_strdup ("printer");

  priv->is_active = TRUE;
  priv->is_paused = FALSE;
  priv->is_accepting_jobs = TRUE;
  priv->is_new = TRUE;
  priv->has_details = FALSE;
  priv->accepts_pdf = FALSE;
  priv->accepts_ps = TRUE;

  priv->state_message = NULL;  
  priv->job_count = 0;
}

static void
ctk_printer_finalize (GObject *object)
{
  CtkPrinter *printer = CTK_PRINTER (object);
  CtkPrinterPrivate *priv = printer->priv;

  g_free (priv->name);
  g_free (priv->location);
  g_free (priv->description);
  g_free (priv->state_message);
  g_free (priv->icon_name);

  if (priv->backend)
    g_object_unref (priv->backend);

  G_OBJECT_CLASS (ctk_printer_parent_class)->finalize (object);
}

static void
ctk_printer_set_property (GObject         *object,
			  guint            prop_id,
			  const GValue    *value,
			  GParamSpec      *pspec)
{
  CtkPrinter *printer = CTK_PRINTER (object);
  CtkPrinterPrivate *priv = printer->priv;

  switch (prop_id)
    {
    case PROP_NAME:
      priv->name = g_value_dup_string (value);
      break;
    
    case PROP_BACKEND:
      priv->backend = CTK_PRINT_BACKEND (g_value_dup_object (value));
      break;

    case PROP_IS_VIRTUAL:
      priv->is_virtual = g_value_get_boolean (value);
      break;

    case PROP_ACCEPTS_PDF:
      priv->accepts_pdf = g_value_get_boolean (value);
      break;

    case PROP_ACCEPTS_PS:
      priv->accepts_ps = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_printer_get_property (GObject    *object,
			  guint       prop_id,
			  GValue     *value,
			  GParamSpec *pspec)
{
  CtkPrinter *printer = CTK_PRINTER (object);
  CtkPrinterPrivate *priv = printer->priv;

  switch (prop_id)
    {
    case PROP_NAME:
      if (priv->name)
	g_value_set_string (value, priv->name);
      else
	g_value_set_static_string (value, "");
      break;
    case PROP_BACKEND:
      g_value_set_object (value, priv->backend);
      break;
    case PROP_STATE_MESSAGE:
      if (priv->state_message)
	g_value_set_string (value, priv->state_message);
      else
	g_value_set_static_string (value, "");
      break;
    case PROP_LOCATION:
      if (priv->location)
	g_value_set_string (value, priv->location);
      else
	g_value_set_static_string (value, "");
      break;
    case PROP_ICON_NAME:
      if (priv->icon_name)
	g_value_set_string (value, priv->icon_name);
      else
	g_value_set_static_string (value, "");
      break;
    case PROP_JOB_COUNT:
      g_value_set_int (value, priv->job_count);
      break;
    case PROP_IS_VIRTUAL:
      g_value_set_boolean (value, priv->is_virtual);
      break;
    case PROP_ACCEPTS_PDF:
      g_value_set_boolean (value, priv->accepts_pdf);
      break;
    case PROP_ACCEPTS_PS:
      g_value_set_boolean (value, priv->accepts_ps);
      break;
    case PROP_PAUSED:
      g_value_set_boolean (value, priv->is_paused);
      break;
    case PROP_ACCEPTING_JOBS:
      g_value_set_boolean (value, priv->is_accepting_jobs);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * ctk_printer_new:
 * @name: the name of the printer
 * @backend: a #CtkPrintBackend
 * @virtual_: whether the printer is virtual
 *
 * Creates a new #CtkPrinter.
 *
 * Returns: a new #CtkPrinter
 *
 * Since: 2.10
 **/
CtkPrinter *
ctk_printer_new (const gchar     *name,
		 CtkPrintBackend *backend,
		 gboolean         virtual_)
{
  GObject *result;
  
  result = g_object_new (CTK_TYPE_PRINTER,
			 "name", name,
			 "backend", backend,
			 "is-virtual", virtual_,
                         NULL);

  return (CtkPrinter *) result;
}

/**
 * ctk_printer_get_backend:
 * @printer: a #CtkPrinter
 * 
 * Returns the backend of the printer.
 * 
 * Returns: (transfer none): the backend of @printer
 * 
 * Since: 2.10
 */
CtkPrintBackend *
ctk_printer_get_backend (CtkPrinter *printer)
{
  g_return_val_if_fail (CTK_IS_PRINTER (printer), NULL);
  
  return printer->priv->backend;
}

/**
 * ctk_printer_get_name:
 * @printer: a #CtkPrinter
 * 
 * Returns the name of the printer.
 * 
 * Returns: the name of @printer
 *
 * Since: 2.10
 */
const gchar *
ctk_printer_get_name (CtkPrinter *printer)
{
  g_return_val_if_fail (CTK_IS_PRINTER (printer), NULL);

  return printer->priv->name;
}

/**
 * ctk_printer_get_description:
 * @printer: a #CtkPrinter
 * 
 * Gets the description of the printer.
 * 
 * Returns: the description of @printer
 *
 * Since: 2.10
 */
const gchar *
ctk_printer_get_description (CtkPrinter *printer)
{
  g_return_val_if_fail (CTK_IS_PRINTER (printer), NULL);
  
  return printer->priv->description;
}

gboolean
ctk_printer_set_description (CtkPrinter  *printer,
			     const gchar *description)
{
  CtkPrinterPrivate *priv;

  g_return_val_if_fail (CTK_IS_PRINTER (printer), FALSE);

  priv = printer->priv;

  if (g_strcmp0 (priv->description, description) == 0)
    return FALSE;

  g_free (priv->description);
  priv->description = g_strdup (description);
  
  return TRUE;
}

/**
 * ctk_printer_get_state_message:
 * @printer: a #CtkPrinter
 * 
 * Returns the state message describing the current state
 * of the printer.
 * 
 * Returns: the state message of @printer
 *
 * Since: 2.10
 */
const gchar *
ctk_printer_get_state_message (CtkPrinter *printer)
{
  g_return_val_if_fail (CTK_IS_PRINTER (printer), NULL);

  return printer->priv->state_message;
}

gboolean
ctk_printer_set_state_message (CtkPrinter  *printer,
			       const gchar *message)
{
  CtkPrinterPrivate *priv;

  g_return_val_if_fail (CTK_IS_PRINTER (printer), FALSE);

  priv = printer->priv;

  if (g_strcmp0 (priv->state_message, message) == 0)
    return FALSE;

  g_free (priv->state_message);
  priv->state_message = g_strdup (message);
  g_object_notify (G_OBJECT (printer), "state-message");

  return TRUE;
}

/**
 * ctk_printer_get_location:
 * @printer: a #CtkPrinter
 * 
 * Returns a description of the location of the printer.
 * 
 * Returns: the location of @printer
 *
 * Since: 2.10
 */
const gchar *
ctk_printer_get_location (CtkPrinter *printer)
{
  g_return_val_if_fail (CTK_IS_PRINTER (printer), NULL);

  return printer->priv->location;
}

gboolean
ctk_printer_set_location (CtkPrinter  *printer,
			  const gchar *location)
{
  CtkPrinterPrivate *priv;

  g_return_val_if_fail (CTK_IS_PRINTER (printer), FALSE);

  priv = printer->priv;

  if (g_strcmp0 (priv->location, location) == 0)
    return FALSE;

  g_free (priv->location);
  priv->location = g_strdup (location);
  g_object_notify (G_OBJECT (printer), "location");
  
  return TRUE;
}

/**
 * ctk_printer_get_icon_name:
 * @printer: a #CtkPrinter
 * 
 * Gets the name of the icon to use for the printer.
 * 
 * Returns: the icon name for @printer
 *
 * Since: 2.10
 */
const gchar *
ctk_printer_get_icon_name (CtkPrinter *printer)
{
  g_return_val_if_fail (CTK_IS_PRINTER (printer), NULL);

  return printer->priv->icon_name;
}

void
ctk_printer_set_icon_name (CtkPrinter  *printer,
			   const gchar *icon)
{
  CtkPrinterPrivate *priv;

  g_return_if_fail (CTK_IS_PRINTER (printer));

  priv = printer->priv;

  g_free (priv->icon_name);
  priv->icon_name = g_strdup (icon);
  g_object_notify (G_OBJECT (printer), "icon-name");
}

/**
 * ctk_printer_get_job_count:
 * @printer: a #CtkPrinter
 * 
 * Gets the number of jobs currently queued on the printer.
 * 
 * Returns: the number of jobs on @printer
 *
 * Since: 2.10
 */
gint 
ctk_printer_get_job_count (CtkPrinter *printer)
{
  g_return_val_if_fail (CTK_IS_PRINTER (printer), 0);

  return printer->priv->job_count;
}

gboolean
ctk_printer_set_job_count (CtkPrinter *printer,
			   gint        count)
{
  CtkPrinterPrivate *priv;

  g_return_val_if_fail (CTK_IS_PRINTER (printer), FALSE);

  priv = printer->priv;

  if (priv->job_count == count)
    return FALSE;

  priv->job_count = count;
  
  g_object_notify (G_OBJECT (printer), "job-count");
  
  return TRUE;
}

/**
 * ctk_printer_has_details:
 * @printer: a #CtkPrinter
 * 
 * Returns whether the printer details are available.
 * 
 * Returns: %TRUE if @printer details are available
 *
 * Since: 2.12
 */
gboolean
ctk_printer_has_details (CtkPrinter *printer)
{
  g_return_val_if_fail (CTK_IS_PRINTER (printer), FALSE);

  return printer->priv->has_details;
}

void
ctk_printer_set_has_details (CtkPrinter *printer,
			     gboolean val)
{
  printer->priv->has_details = val;
}

/**
 * ctk_printer_is_active:
 * @printer: a #CtkPrinter
 * 
 * Returns whether the printer is currently active (i.e. 
 * accepts new jobs).
 * 
 * Returns: %TRUE if @printer is active
 *
 * Since: 2.10
 */
gboolean
ctk_printer_is_active (CtkPrinter *printer)
{
  g_return_val_if_fail (CTK_IS_PRINTER (printer), TRUE);
  
  return printer->priv->is_active;
}

void
ctk_printer_set_is_active (CtkPrinter *printer,
			   gboolean val)
{
  g_return_if_fail (CTK_IS_PRINTER (printer));

  printer->priv->is_active = val;
}

/**
 * ctk_printer_is_paused:
 * @printer: a #CtkPrinter
 * 
 * Returns whether the printer is currently paused.
 * A paused printer still accepts jobs, but it is not
 * printing them.
 * 
 * Returns: %TRUE if @printer is paused
 *
 * Since: 2.14
 */
gboolean
ctk_printer_is_paused (CtkPrinter *printer)
{
  g_return_val_if_fail (CTK_IS_PRINTER (printer), TRUE);
  
  return printer->priv->is_paused;
}

gboolean
ctk_printer_set_is_paused (CtkPrinter *printer,
			   gboolean    val)
{
  CtkPrinterPrivate *priv;

  g_return_val_if_fail (CTK_IS_PRINTER (printer), FALSE);

  priv = printer->priv;

  if (val == priv->is_paused)
    return FALSE;

  priv->is_paused = val;

  return TRUE;
}

/**
 * ctk_printer_is_accepting_jobs:
 * @printer: a #CtkPrinter
 * 
 * Returns whether the printer is accepting jobs
 * 
 * Returns: %TRUE if @printer is accepting jobs
 *
 * Since: 2.14
 */
gboolean
ctk_printer_is_accepting_jobs (CtkPrinter *printer)
{
  g_return_val_if_fail (CTK_IS_PRINTER (printer), TRUE);
  
  return printer->priv->is_accepting_jobs;
}

gboolean
ctk_printer_set_is_accepting_jobs (CtkPrinter *printer,
				   gboolean val)
{
  CtkPrinterPrivate *priv;

  g_return_val_if_fail (CTK_IS_PRINTER (printer), FALSE);

  priv = printer->priv;

  if (val == priv->is_accepting_jobs)
    return FALSE;

  priv->is_accepting_jobs = val;

  return TRUE;
}

/**
 * ctk_printer_is_virtual:
 * @printer: a #CtkPrinter
 * 
 * Returns whether the printer is virtual (i.e. does not
 * represent actual printer hardware, but something like 
 * a CUPS class).
 * 
 * Returns: %TRUE if @printer is virtual
 *
 * Since: 2.10
 */
gboolean
ctk_printer_is_virtual (CtkPrinter *printer)
{
  g_return_val_if_fail (CTK_IS_PRINTER (printer), TRUE);
  
  return printer->priv->is_virtual;
}

/**
 * ctk_printer_accepts_pdf:
 * @printer: a #CtkPrinter
 *
 * Returns whether the printer accepts input in
 * PDF format.  
 *
 * Returns: %TRUE if @printer accepts PDF
 *
 * Since: 2.10
 */
gboolean 
ctk_printer_accepts_pdf (CtkPrinter *printer)
{ 
  g_return_val_if_fail (CTK_IS_PRINTER (printer), TRUE);
  
  return printer->priv->accepts_pdf;
}

void
ctk_printer_set_accepts_pdf (CtkPrinter *printer,
			     gboolean val)
{
  g_return_if_fail (CTK_IS_PRINTER (printer));

  printer->priv->accepts_pdf = val;
}

/**
 * ctk_printer_accepts_ps:
 * @printer: a #CtkPrinter
 *
 * Returns whether the printer accepts input in
 * PostScript format.  
 *
 * Returns: %TRUE if @printer accepts PostScript
 *
 * Since: 2.10
 */
gboolean 
ctk_printer_accepts_ps (CtkPrinter *printer)
{ 
  g_return_val_if_fail (CTK_IS_PRINTER (printer), TRUE);
  
  return printer->priv->accepts_ps;
}

void
ctk_printer_set_accepts_ps (CtkPrinter *printer,
			    gboolean val)
{
  g_return_if_fail (CTK_IS_PRINTER (printer));

  printer->priv->accepts_ps = val;
}

gboolean
ctk_printer_is_new (CtkPrinter *printer)
{
  g_return_val_if_fail (CTK_IS_PRINTER (printer), FALSE);
  
  return printer->priv->is_new;
}

void
ctk_printer_set_is_new (CtkPrinter *printer,
			gboolean val)
{
  g_return_if_fail (CTK_IS_PRINTER (printer));

  printer->priv->is_new = val;
}


/**
 * ctk_printer_is_default:
 * @printer: a #CtkPrinter
 * 
 * Returns whether the printer is the default printer.
 * 
 * Returns: %TRUE if @printer is the default
 *
 * Since: 2.10
 */
gboolean
ctk_printer_is_default (CtkPrinter *printer)
{
  g_return_val_if_fail (CTK_IS_PRINTER (printer), FALSE);
  
  return printer->priv->is_default;
}

void
ctk_printer_set_is_default (CtkPrinter *printer,
			    gboolean    val)
{
  g_return_if_fail (CTK_IS_PRINTER (printer));

  printer->priv->is_default = val;
}

/**
 * ctk_printer_request_details:
 * @printer: a #CtkPrinter
 * 
 * Requests the printer details. When the details are available,
 * the #CtkPrinter::details-acquired signal will be emitted on @printer.
 * 
 * Since: 2.12
 */
void
ctk_printer_request_details (CtkPrinter *printer)
{
  CtkPrintBackendClass *backend_class;

  g_return_if_fail (CTK_IS_PRINTER (printer));

  backend_class = CTK_PRINT_BACKEND_GET_CLASS (printer->priv->backend);
  backend_class->printer_request_details (printer);
}

CtkPrinterOptionSet *
_ctk_printer_get_options (CtkPrinter           *printer,
			  CtkPrintSettings     *settings,
			  CtkPageSetup         *page_setup,
			  CtkPrintCapabilities  capabilities)
{
  CtkPrintBackendClass *backend_class = CTK_PRINT_BACKEND_GET_CLASS (printer->priv->backend);
  return backend_class->printer_get_options (printer, settings, page_setup, capabilities);
}

gboolean
_ctk_printer_mark_conflicts (CtkPrinter          *printer,
			     CtkPrinterOptionSet *options)
{
  CtkPrintBackendClass *backend_class = CTK_PRINT_BACKEND_GET_CLASS (printer->priv->backend);
  return backend_class->printer_mark_conflicts (printer, options);
}
  
void
_ctk_printer_get_settings_from_options (CtkPrinter          *printer,
					CtkPrinterOptionSet *options,
					CtkPrintSettings    *settings)
{
  CtkPrintBackendClass *backend_class = CTK_PRINT_BACKEND_GET_CLASS (printer->priv->backend);
  backend_class->printer_get_settings_from_options (printer, options, settings);
}

void
_ctk_printer_prepare_for_print (CtkPrinter       *printer,
				CtkPrintJob      *print_job,
				CtkPrintSettings *settings,
				CtkPageSetup     *page_setup)
{
  CtkPrintBackendClass *backend_class = CTK_PRINT_BACKEND_GET_CLASS (printer->priv->backend);
  backend_class->printer_prepare_for_print (printer, print_job, settings, page_setup);
}

cairo_surface_t *
_ctk_printer_create_cairo_surface (CtkPrinter       *printer,
				   CtkPrintSettings *settings,
				   gdouble           width, 
				   gdouble           height,
				   GIOChannel       *cache_io)
{
  CtkPrintBackendClass *backend_class = CTK_PRINT_BACKEND_GET_CLASS (printer->priv->backend);

  return backend_class->printer_create_cairo_surface (printer, settings,
						      width, height, cache_io);
}

gboolean
_ctk_printer_get_hard_margins_for_paper_size (CtkPrinter   *printer,
					      CtkPaperSize *paper_size,
					      gdouble      *top,
					      gdouble      *bottom,
					      gdouble      *left,
					      gdouble      *right)
{
  CtkPrintBackendClass *backend_class = CTK_PRINT_BACKEND_GET_CLASS (printer->priv->backend);

  return backend_class->printer_get_hard_margins_for_paper_size (printer, paper_size, top, bottom, left, right);
}

/**
 * ctk_printer_list_papers:
 * @printer: a #CtkPrinter
 * 
 * Lists all the paper sizes @printer supports.
 * This will return and empty list unless the printer’s details are 
 * available, see ctk_printer_has_details() and ctk_printer_request_details().
 *
 * Returns: (element-type CtkPageSetup) (transfer full): a newly allocated list of newly allocated #CtkPageSetup s.
 *
 * Since: 2.12
 */
GList  *
ctk_printer_list_papers (CtkPrinter *printer)
{
  CtkPrintBackendClass *backend_class;

  g_return_val_if_fail (CTK_IS_PRINTER (printer), NULL);

  backend_class = CTK_PRINT_BACKEND_GET_CLASS (printer->priv->backend);
  return backend_class->printer_list_papers (printer);
}

/**
 * ctk_printer_get_default_page_size:
 * @printer: a #CtkPrinter
 *
 * Returns default page size of @printer.
 * 
 * Returns: a newly allocated #CtkPageSetup with default page size of the printer.
 *
 * Since: 2.14
 */
CtkPageSetup  *
ctk_printer_get_default_page_size (CtkPrinter *printer)
{
  CtkPrintBackendClass *backend_class;

  g_return_val_if_fail (CTK_IS_PRINTER (printer), NULL);

  backend_class = CTK_PRINT_BACKEND_GET_CLASS (printer->priv->backend);
  return backend_class->printer_get_default_page_size (printer);
}

/**
 * ctk_printer_get_hard_margins:
 * @printer: a #CtkPrinter
 * @top: (out): a location to store the top margin in
 * @bottom: (out): a location to store the bottom margin in
 * @left: (out): a location to store the left margin in
 * @right: (out): a location to store the right margin in
 *
 * Retrieve the hard margins of @printer, i.e. the margins that define
 * the area at the borders of the paper that the printer cannot print to.
 *
 * Note: This will not succeed unless the printer’s details are available,
 * see ctk_printer_has_details() and ctk_printer_request_details().
 *
 * Returns: %TRUE iff the hard margins were retrieved
 *
 * Since: 2.20
 */
gboolean
ctk_printer_get_hard_margins (CtkPrinter *printer,
			      gdouble    *top,
			      gdouble    *bottom,
			      gdouble    *left,
			      gdouble    *right)
{
  CtkPrintBackendClass *backend_class = CTK_PRINT_BACKEND_GET_CLASS (printer->priv->backend);

  return backend_class->printer_get_hard_margins (printer, top, bottom, left, right);
}

/**
 * ctk_printer_get_capabilities:
 * @printer: a #CtkPrinter
 * 
 * Returns the printer’s capabilities.
 *
 * This is useful when you’re using #CtkPrintUnixDialog’s manual-capabilities 
 * setting and need to know which settings the printer can handle and which 
 * you must handle yourself.
 *
 * This will return 0 unless the printer’s details are available, see
 * ctk_printer_has_details() and ctk_printer_request_details().
 *
 * Returns: the printer’s capabilities
 *
 * Since: 2.12
 */
CtkPrintCapabilities
ctk_printer_get_capabilities (CtkPrinter *printer)
{
  CtkPrintBackendClass *backend_class;

  g_return_val_if_fail (CTK_IS_PRINTER (printer), 0);

  backend_class = CTK_PRINT_BACKEND_GET_CLASS (printer->priv->backend);
  return backend_class->printer_get_capabilities (printer);
}

/**
 * ctk_printer_compare:
 * @a: a #CtkPrinter
 * @b: another #CtkPrinter
 *
 * Compares two printers.
 * 
 * Returns: 0 if the printer match, a negative value if @a < @b, 
 *   or a positive value if @a > @b
 *
 * Since: 2.10
 */
gint
ctk_printer_compare (CtkPrinter *a, 
                     CtkPrinter *b)
{
  const char *name_a, *name_b;
  
  g_assert (CTK_IS_PRINTER (a) && CTK_IS_PRINTER (b));
  
  name_a = ctk_printer_get_name (a);
  name_b = ctk_printer_get_name (b);
  if (name_a == NULL  && name_b == NULL)
    return 0;
  else if (name_a == NULL)
    return G_MAXINT;
  else if (name_b == NULL)
    return G_MININT;
  else
    return g_ascii_strcasecmp (name_a, name_b);
}


typedef struct 
{
  GList *backends;
  CtkPrinterFunc func;
  gpointer data;
  GDestroyNotify destroy;
  GMainLoop *loop;
} PrinterList;

static void list_done_cb (CtkPrintBackend *backend, 
			  PrinterList     *printer_list);

static void
stop_enumeration (PrinterList *printer_list)
{
  GList *list, *next;

  for (list = printer_list->backends; list; list = next)
    {
      CtkPrintBackend *backend;

      next = list->next;
      backend = CTK_PRINT_BACKEND (list->data);
      list_done_cb (backend, printer_list);
    }
}

static void 
free_printer_list (PrinterList *printer_list)
{
  if (printer_list->destroy)
    printer_list->destroy (printer_list->data);

  if (printer_list->loop)
    {    
      g_main_loop_quit (printer_list->loop);
      g_main_loop_unref (printer_list->loop);
    }

  g_free (printer_list);
}

static gboolean
list_added_cb (CtkPrintBackend *backend G_GNUC_UNUSED,
	       CtkPrinter      *printer,
	       PrinterList     *printer_list)
{
  if (printer_list->func (printer, printer_list->data))
    {
      stop_enumeration (printer_list);
      return TRUE;
    }

  return FALSE;
}

static void
backend_status_changed (GObject    *object,
                        GParamSpec *pspec G_GNUC_UNUSED,
                        gpointer    data)
{
  CtkPrintBackend *backend = CTK_PRINT_BACKEND (object);
  PrinterList *printer_list = data;
  CtkPrintBackendStatus status;

  g_object_get (backend, "status", &status, NULL);
 
  if (status == CTK_PRINT_BACKEND_STATUS_UNAVAILABLE)
    list_done_cb (backend, printer_list);  
}

static gboolean
list_printers_remove_backend (PrinterList     *printer_list,
                              CtkPrintBackend *backend)
{
  printer_list->backends = g_list_remove (printer_list->backends, backend);
  ctk_print_backend_destroy (backend);
  g_object_unref (backend);

  if (printer_list->backends == NULL)
    {
      free_printer_list (printer_list);
      return TRUE;
    }

  return FALSE;
}

static void
list_done_cb (CtkPrintBackend *backend,
	      PrinterList     *printer_list)
{
  g_signal_handlers_disconnect_by_func (backend, list_added_cb, printer_list);
  g_signal_handlers_disconnect_by_func (backend, list_done_cb, printer_list);
  g_signal_handlers_disconnect_by_func (backend, backend_status_changed, printer_list);

  list_printers_remove_backend(printer_list, backend);
}

static gboolean
list_printers_init (PrinterList     *printer_list,
		    CtkPrintBackend *backend)
{
  GList *list, *node;
  CtkPrintBackendStatus status;

  list = ctk_print_backend_get_printer_list (backend);

  for (node = list; node != NULL; node = node->next)
    {
      if (list_added_cb (backend, node->data, printer_list))
        {
          g_list_free (list);
          return TRUE;
        }
    }

  g_list_free (list);

  g_object_get (backend, "status", &status, NULL);
  
  if (status == CTK_PRINT_BACKEND_STATUS_UNAVAILABLE || 
      ctk_print_backend_printer_list_is_done (backend))
    {
      if (list_printers_remove_backend (printer_list, backend))
        return TRUE;
    }
  else
    {
      g_signal_connect (backend, "printer-added", 
			(GCallback) list_added_cb, 
			printer_list);
      g_signal_connect (backend, "printer-list-done", 
			(GCallback) list_done_cb, 
			printer_list);
      g_signal_connect (backend, "notify::status", 
                        (GCallback) backend_status_changed,
                        printer_list);     
    }

  return FALSE;
}

/**
 * ctk_enumerate_printers:
 * @func: a function to call for each printer
 * @data: user data to pass to @func
 * @destroy: function to call if @data is no longer needed
 * @wait: if %TRUE, wait in a recursive mainloop until
 *    all printers are enumerated; otherwise return early
 *
 * Calls a function for all #CtkPrinters. 
 * If @func returns %TRUE, the enumeration is stopped.
 *
 * Since: 2.10
 */
void
ctk_enumerate_printers (CtkPrinterFunc func,
			gpointer       data,
			GDestroyNotify destroy,
			gboolean       wait)
{
  PrinterList *printer_list;
  GList *node, *next;

  printer_list = g_new0 (PrinterList, 1);

  printer_list->func = func;
  printer_list->data = data;
  printer_list->destroy = destroy;

  if (g_module_supported ())
    printer_list->backends = ctk_print_backend_load_modules ();
  
  if (printer_list->backends == NULL)
    {
      free_printer_list (printer_list);
      return;
    }

  for (node = printer_list->backends; node != NULL; node = next)
    {
      CtkPrintBackend *backend;

      next = node->next;
      backend = CTK_PRINT_BACKEND (node->data);
      if (list_printers_init (printer_list, backend))
        return;
    }

  if (wait && printer_list->backends)
    {
      printer_list->loop = g_main_loop_new (NULL, FALSE);

      cdk_threads_leave ();  
      g_main_loop_run (printer_list->loop);
      cdk_threads_enter ();  
    }
}

GType
ctk_print_capabilities_get_type (void)
{
  static GType etype = 0;

  if (G_UNLIKELY (etype == 0))
    {
      static const GFlagsValue values[] = {
        { CTK_PRINT_CAPABILITY_PAGE_SET, "CTK_PRINT_CAPABILITY_PAGE_SET", "page-set" },
        { CTK_PRINT_CAPABILITY_COPIES, "CTK_PRINT_CAPABILITY_COPIES", "copies" },
        { CTK_PRINT_CAPABILITY_COLLATE, "CTK_PRINT_CAPABILITY_COLLATE", "collate" },
        { CTK_PRINT_CAPABILITY_REVERSE, "CTK_PRINT_CAPABILITY_REVERSE", "reverse" },
        { CTK_PRINT_CAPABILITY_SCALE, "CTK_PRINT_CAPABILITY_SCALE", "scale" },
        { CTK_PRINT_CAPABILITY_GENERATE_PDF, "CTK_PRINT_CAPABILITY_GENERATE_PDF", "generate-pdf" },
        { CTK_PRINT_CAPABILITY_GENERATE_PS, "CTK_PRINT_CAPABILITY_GENERATE_PS", "generate-ps" },
        { CTK_PRINT_CAPABILITY_PREVIEW, "CTK_PRINT_CAPABILITY_PREVIEW", "preview" },
	{ CTK_PRINT_CAPABILITY_NUMBER_UP, "CTK_PRINT_CAPABILITY_NUMBER_UP", "number-up"},
        { CTK_PRINT_CAPABILITY_NUMBER_UP_LAYOUT, "CTK_PRINT_CAPABILITY_NUMBER_UP_LAYOUT", "number-up-layout" },
        { 0, NULL, NULL }
      };

      etype = g_flags_register_static (I_("CtkPrintCapabilities"), values);
    }

  return etype;
}
