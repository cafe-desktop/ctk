/* GTK - The GIMP Toolkit
 * ctkprintbackendlpr.c: LPR implementation of GtkPrintBackend 
 * for printing to lpr 
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

#include "config.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <cairo.h>
#include <cairo-ps.h>

#include <glib/gi18n-lib.h>

#include <ctk/ctk.h>
#include "ctkprinter-private.h"

#include "ctkprintbackendlpr.h"

typedef struct _GtkPrintBackendLprClass GtkPrintBackendLprClass;

#define CTK_PRINT_BACKEND_LPR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_PRINT_BACKEND_LPR, GtkPrintBackendLprClass))
#define CTK_IS_PRINT_BACKEND_LPR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_PRINT_BACKEND_LPR))
#define CTK_PRINT_BACKEND_LPR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_PRINT_BACKEND_LPR, GtkPrintBackendLprClass))

#define _LPR_MAX_CHUNK_SIZE 8192

static GType print_backend_lpr_type = 0;

struct _GtkPrintBackendLprClass
{
  GtkPrintBackendClass parent_class;
};

struct _GtkPrintBackendLpr
{
  GtkPrintBackend parent_instance;
};

static GObjectClass *backend_parent_class;

static void                 ctk_print_backend_lpr_class_init      (GtkPrintBackendLprClass *class);
static void                 ctk_print_backend_lpr_init            (GtkPrintBackendLpr      *impl);
static void                 lpr_printer_get_settings_from_options (GtkPrinter              *printer,
								   GtkPrinterOptionSet     *options,
								   GtkPrintSettings        *settings);
static GtkPrinterOptionSet *lpr_printer_get_options               (GtkPrinter              *printer,
								   GtkPrintSettings        *settings,
								   GtkPageSetup            *page_setup,
								   GtkPrintCapabilities     capabilities);
static void                 lpr_printer_prepare_for_print         (GtkPrinter              *printer,
								   GtkPrintJob             *print_job,
								   GtkPrintSettings        *settings,
								   GtkPageSetup            *page_setup);
static cairo_surface_t *    lpr_printer_create_cairo_surface      (GtkPrinter              *printer,
								   GtkPrintSettings        *settings,
								   gdouble                  width,
								   gdouble                  height,
								   GIOChannel              *cache_io);
static void                 ctk_print_backend_lpr_print_stream    (GtkPrintBackend         *print_backend,
								   GtkPrintJob             *job,
								   GIOChannel              *data_io,
								   GtkPrintJobCompleteFunc  callback,
								   gpointer                 user_data,
								   GDestroyNotify           dnotify);

static void
ctk_print_backend_lpr_register_type (GTypeModule *module)
{
  const GTypeInfo print_backend_lpr_info =
  {
    sizeof (GtkPrintBackendLprClass),
    NULL,		/* base_init */
    NULL,		/* base_finalize */
    (GClassInitFunc) ctk_print_backend_lpr_class_init,
    NULL,		/* class_finalize */
    NULL,		/* class_data */
    sizeof (GtkPrintBackendLpr),
    0,		/* n_preallocs */
    (GInstanceInitFunc) ctk_print_backend_lpr_init,
  };

  print_backend_lpr_type = g_type_module_register_type (module,
                                                        CTK_TYPE_PRINT_BACKEND,
                                                        "GtkPrintBackendLpr",
                                                        &print_backend_lpr_info, 0);
}

G_MODULE_EXPORT void 
pb_module_init (GTypeModule *module)
{
  ctk_print_backend_lpr_register_type (module);
}

G_MODULE_EXPORT void 
pb_module_exit (void)
{

}
  
G_MODULE_EXPORT GtkPrintBackend * 
pb_module_create (void)
{
  return ctk_print_backend_lpr_new ();
}

/*
 * GtkPrintBackendLpr
 */
GType
ctk_print_backend_lpr_get_type (void)
{
  return print_backend_lpr_type;
}

/**
 * ctk_print_backend_lpr_new:
 *
 * Creates a new #GtkPrintBackendLpr object. #GtkPrintBackendLpr
 * implements the #GtkPrintBackend interface with direct access to
 * the filesystem using Unix/Linux API calls
 *
 * Returns: the new #GtkPrintBackendLpr object
 **/
GtkPrintBackend *
ctk_print_backend_lpr_new (void)
{
  return g_object_new (CTK_TYPE_PRINT_BACKEND_LPR, NULL);
}

static void
ctk_print_backend_lpr_class_init (GtkPrintBackendLprClass *class)
{
  GtkPrintBackendClass *backend_class = CTK_PRINT_BACKEND_CLASS (class);
  
  backend_parent_class = g_type_class_peek_parent (class);

  backend_class->print_stream = ctk_print_backend_lpr_print_stream;
  backend_class->printer_create_cairo_surface = lpr_printer_create_cairo_surface;
  backend_class->printer_get_options = lpr_printer_get_options;
  backend_class->printer_get_settings_from_options = lpr_printer_get_settings_from_options;
  backend_class->printer_prepare_for_print = lpr_printer_prepare_for_print;
}

static cairo_status_t
_cairo_write (void                *closure,
              const unsigned char *data,
              unsigned int         length)
{
  GIOChannel *io = (GIOChannel *)closure;
  gsize written;
  GError *error;

  error = NULL;

  CTK_NOTE (PRINTING,
            g_print ("LPR Backend: Writting %i byte chunk to temp file\n", length));

  while (length > 0) 
    {
      g_io_channel_write_chars (io, (const gchar*)data, length, &written, &error);

      if (error != NULL)
	{
	  CTK_NOTE (PRINTING,
                     g_print ("LPR Backend: Error writting to temp file, %s\n", error->message));

          g_error_free (error);
	  return CAIRO_STATUS_WRITE_ERROR;
	}    

      CTK_NOTE (PRINTING,
                g_print ("LPR Backend: Wrote %" G_GSIZE_FORMAT " bytes to temp file\n", written));

      data += written;
      length -= written;
    }

  return CAIRO_STATUS_SUCCESS;
}

static cairo_surface_t *
lpr_printer_create_cairo_surface (GtkPrinter       *printer,
				  GtkPrintSettings *settings,
				  gdouble           width, 
				  gdouble           height,
				  GIOChannel       *cache_io)
{
  cairo_surface_t *surface;
  
  surface = cairo_ps_surface_create_for_stream (_cairo_write, cache_io, width, height);

  cairo_surface_set_fallback_resolution (surface,
                                         2.0 * ctk_print_settings_get_printer_lpi (settings),
                                         2.0 * ctk_print_settings_get_printer_lpi (settings));

  return surface;
}

typedef struct {
  GtkPrintBackend *backend;
  GtkPrintJobCompleteFunc callback;
  GtkPrintJob *job;
  gpointer user_data;
  GDestroyNotify dnotify;

  GIOChannel *in;
} _PrintStreamData;

static void
lpr_print_cb (GtkPrintBackendLpr *print_backend,
              GError             *error,
              gpointer            user_data)
{
  _PrintStreamData *ps = (_PrintStreamData *) user_data;

  if (ps->in != NULL) 
    g_io_channel_unref (ps->in);

  if (ps->callback)
    ps->callback (ps->job, ps->user_data, error);

  if (ps->dnotify)
    ps->dnotify (ps->user_data);

  ctk_print_job_set_status (ps->job, 
			    error ? CTK_PRINT_STATUS_FINISHED_ABORTED 
			          : CTK_PRINT_STATUS_FINISHED);

  if (ps->job)
    g_object_unref (ps->job);
  
  g_free (ps);
}

static gboolean
lpr_write (GIOChannel   *source,
           GIOCondition  con,
           gpointer      user_data)
{
  gchar buf[_LPR_MAX_CHUNK_SIZE];
  gsize bytes_read;
  GError *error;
  GIOStatus status;
  _PrintStreamData *ps = (_PrintStreamData *) user_data;

  error = NULL;

  status = 
    g_io_channel_read_chars (source,
                             buf,
                             _LPR_MAX_CHUNK_SIZE,
                             &bytes_read,
                             &error);

  if (status != G_IO_STATUS_ERROR)
    {
      gsize bytes_written;

      g_io_channel_write_chars (ps->in,
                                buf, 
				bytes_read, 
				&bytes_written, 
				&error);
    }

  if (error != NULL || status == G_IO_STATUS_EOF)
    {
      lpr_print_cb (CTK_PRINT_BACKEND_LPR (ps->backend), 
		    error, user_data);


      if (error != NULL)
        {
          CTK_NOTE (PRINTING,
                    g_print ("LPR Backend: %s\n", error->message));

          g_error_free (error);
        } 

      return FALSE;
    }

  CTK_NOTE (PRINTING,
            g_print ("LPR Backend: Writting %" G_GSIZE_FORMAT " byte chunk to lpr pipe\n", bytes_read));


  return TRUE;
}

#define LPR_COMMAND "lpr"

static void
ctk_print_backend_lpr_print_stream (GtkPrintBackend        *print_backend,
				    GtkPrintJob            *job,
				    GIOChannel             *data_io,
				    GtkPrintJobCompleteFunc callback,
				    gpointer                user_data,
				    GDestroyNotify          dnotify)
{
  GError *print_error = NULL;
  _PrintStreamData *ps;
  GtkPrintSettings *settings;
  gint argc;
  gint in_fd;
  gchar **argv = NULL;
  const char *cmd_line;

  settings = ctk_print_job_get_settings (job);

  cmd_line = ctk_print_settings_get (settings, "lpr-commandline");
  if (cmd_line == NULL)
    cmd_line = LPR_COMMAND;

  ps = g_new0 (_PrintStreamData, 1);
  ps->callback = callback;
  ps->user_data = user_data;
  ps->dnotify = dnotify;
  ps->job = g_object_ref (job);
  ps->in = NULL;

 /* spawn lpr with pipes and pipe ps file to lpr */
  if (!g_shell_parse_argv (cmd_line, &argc, &argv, &print_error))
    goto out;

  if (!g_spawn_async_with_pipes (NULL,
                                 argv,
                                 NULL,
                                 G_SPAWN_SEARCH_PATH,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &in_fd,
                                 NULL,
                                 NULL,
                                 &print_error))
      goto out;

  ps->in = g_io_channel_unix_new (in_fd);

  g_io_channel_set_encoding (ps->in, NULL, &print_error);
  if (print_error != NULL)
    {
      if (ps->in != NULL)
        g_io_channel_unref (ps->in);

      goto out;
    }

  g_io_channel_set_close_on_unref (ps->in, TRUE);

  g_io_add_watch (data_io,
                  G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP,
                  (GIOFunc) lpr_write,
                  ps);

 out:
  if (argv != NULL)
    g_strfreev (argv);

  if (print_error != NULL)
    {
      lpr_print_cb (CTK_PRINT_BACKEND_LPR (print_backend),
		    print_error, ps);
      g_error_free (print_error);
    }
}

static void
ctk_print_backend_lpr_init (GtkPrintBackendLpr *backend)
{
  GtkPrinter *printer;

  printer = g_object_new (CTK_TYPE_PRINTER,
			  "name", _("Print to LPR"),
			  "backend", backend,
			  "is-virtual", FALSE,
			  "accepts-pdf", TRUE,
			  "accepts-ps", TRUE,
			  NULL);
  ctk_printer_set_has_details (printer, TRUE);
  ctk_printer_set_icon_name (printer, "printer");
  ctk_printer_set_is_active (printer, TRUE);
  ctk_printer_set_is_default (printer, TRUE);

  ctk_print_backend_add_printer (CTK_PRINT_BACKEND (backend), printer);
  g_object_unref (printer);
  ctk_print_backend_set_list_done (CTK_PRINT_BACKEND (backend));
}

static GtkPrinterOptionSet *
lpr_printer_get_options (GtkPrinter           *printer,
			 GtkPrintSettings     *settings,
			 GtkPageSetup         *page_setup,
			 GtkPrintCapabilities  capabilities)
{
  GtkPrinterOptionSet *set;
  GtkPrinterOption *option;
  const char *command;
  char *n_up[] = {"1", "2", "4", "6", "9", "16" };

  set = ctk_printer_option_set_new ();

  option = ctk_printer_option_new ("ctk-n-up", _("Pages Per Sheet"), CTK_PRINTER_OPTION_TYPE_PICKONE);
  ctk_printer_option_choices_from_array (option, G_N_ELEMENTS (n_up),
					 n_up, n_up);
  ctk_printer_option_set (option, "1");
  ctk_printer_option_set_add (set, option);
  g_object_unref (option);

  option = ctk_printer_option_new ("ctk-main-page-custom-input", _("Command Line"), CTK_PRINTER_OPTION_TYPE_STRING);
  ctk_printer_option_set_activates_default (option, TRUE);
  option->group = g_strdup ("GtkPrintDialogExtension");
  if (settings != NULL &&
      (command = ctk_print_settings_get (settings, "lpr-commandline"))!= NULL)
    ctk_printer_option_set (option, command);
  else
    ctk_printer_option_set (option, LPR_COMMAND);
  ctk_printer_option_set_add (set, option);
    
  return set;
}

static void
lpr_printer_get_settings_from_options (GtkPrinter          *printer,
				       GtkPrinterOptionSet *options,
				       GtkPrintSettings    *settings)
{
  GtkPrinterOption *option;

  option = ctk_printer_option_set_lookup (options, "ctk-main-page-custom-input");
  if (option)
    ctk_print_settings_set (settings, "lpr-commandline", option->value);

  option = ctk_printer_option_set_lookup (options, "ctk-n-up");
  if (option)
    ctk_print_settings_set (settings, CTK_PRINT_SETTINGS_NUMBER_UP, option->value);

  option = ctk_printer_option_set_lookup (options, "ctk-n-up-layout");
  if (option)
    ctk_print_settings_set (settings, CTK_PRINT_SETTINGS_NUMBER_UP_LAYOUT, option->value);
}

static void
lpr_printer_prepare_for_print (GtkPrinter       *printer,
			       GtkPrintJob      *print_job,
			       GtkPrintSettings *settings,
			       GtkPageSetup     *page_setup)
{
  double scale;
  GtkPrintPages pages;
  GtkPageRange *ranges;
  gint n_ranges;

  pages = ctk_print_settings_get_print_pages (settings);
  ctk_print_job_set_pages (print_job, pages);

  if (pages == CTK_PRINT_PAGES_RANGES)
    ranges = ctk_print_settings_get_page_ranges (settings, &n_ranges);
  else
    {
      ranges = NULL;
      n_ranges = 0;
    }

  ctk_print_job_set_page_ranges (print_job, ranges, n_ranges);
  ctk_print_job_set_collate (print_job, ctk_print_settings_get_collate (settings));
  ctk_print_job_set_reverse (print_job, ctk_print_settings_get_reverse (settings));
  ctk_print_job_set_num_copies (print_job, ctk_print_settings_get_n_copies (settings));
  ctk_print_job_set_n_up (print_job, ctk_print_settings_get_number_up (settings));
  ctk_print_job_set_n_up_layout (print_job, ctk_print_settings_get_number_up_layout (settings));

  scale = ctk_print_settings_get_scale (settings);
  if (scale != 100.0)
    ctk_print_job_set_scale (print_job, scale / 100.0);

  ctk_print_job_set_page_set (print_job, ctk_print_settings_get_page_set (settings));
  ctk_print_job_set_rotate (print_job, TRUE);
}
