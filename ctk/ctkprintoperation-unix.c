/* CTK - The GIMP Toolkit
 * ctkprintoperation-unix.c: Print Operation Details for Unix 
 *                           and Unix-like platforms
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

#include "config.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>       
#include <fcntl.h>

#include <glib/gstdio.h>
#include "ctkprintoperation-private.h"
#include "ctkprintoperation-portal.h"
#include "ctkmessagedialog.h"
#include "ctkprinter-private.h"

#include <cairo-pdf.h>
#include <cairo-ps.h>
#include "ctkprivate.h"
#include "ctkprintunixdialog.h"
#include "ctkpagesetupunixdialog.h"
#include "ctkprintbackend.h"
#include "ctkprinter.h"
#include "ctkprintjob.h"
#include "ctklabel.h"
#include "ctkintl.h"


typedef struct 
{
  CtkWindow *parent;        /* just in case we need to throw error dialogs */
  GMainLoop *loop;
  gboolean data_sent;

  /* Real printing (not preview) */
  CtkPrintJob *job;         /* the job we are sending to the printer */
  cairo_surface_t *surface;
  gulong job_status_changed_tag;

  
} CtkPrintOperationUnix;

typedef struct _PrinterFinder PrinterFinder;

static void printer_finder_free (PrinterFinder *finder);
static void find_printer        (const gchar   *printer,
				 GFunc          func,
				 gpointer       data);

static void
unix_start_page (CtkPrintOperation *op,
		 CtkPrintContext   *print_context G_GNUC_UNUSED,
		 CtkPageSetup      *page_setup)
{
  CtkPrintOperationUnix *op_unix;  
  CtkPaperSize *paper_size;
  cairo_surface_type_t type;
  gdouble w, h;

  op_unix = op->priv->platform_data;
  
  paper_size = ctk_page_setup_get_paper_size (page_setup);

  w = ctk_paper_size_get_width (paper_size, CTK_UNIT_POINTS);
  h = ctk_paper_size_get_height (paper_size, CTK_UNIT_POINTS);
  
  type = cairo_surface_get_type (op_unix->surface);

  if ((op->priv->manual_number_up < 2) ||
      (op->priv->page_position % op->priv->manual_number_up == 0))
    {
      if (type == CAIRO_SURFACE_TYPE_PS)
        {
          cairo_ps_surface_set_size (op_unix->surface, w, h);
          cairo_ps_surface_dsc_begin_page_setup (op_unix->surface);
          switch (ctk_page_setup_get_orientation (page_setup))
            {
              case CTK_PAGE_ORIENTATION_PORTRAIT:
              case CTK_PAGE_ORIENTATION_REVERSE_PORTRAIT:
                cairo_ps_surface_dsc_comment (op_unix->surface, "%%PageOrientation: Portrait");
                break;

              case CTK_PAGE_ORIENTATION_LANDSCAPE:
              case CTK_PAGE_ORIENTATION_REVERSE_LANDSCAPE:
                cairo_ps_surface_dsc_comment (op_unix->surface, "%%PageOrientation: Landscape");
                break;
            }
         }
      else if (type == CAIRO_SURFACE_TYPE_PDF)
        {
          if (!op->priv->manual_orientation)
            {
              w = ctk_page_setup_get_paper_width (page_setup, CTK_UNIT_POINTS);
              h = ctk_page_setup_get_paper_height (page_setup, CTK_UNIT_POINTS);
            }
          cairo_pdf_surface_set_size (op_unix->surface, w, h);
        }
    }
}

static void
unix_end_page (CtkPrintOperation *op,
	       CtkPrintContext   *print_context)
{
  cairo_t *cr;

  cr = ctk_print_context_get_cairo_context (print_context);

  if ((op->priv->manual_number_up < 2) ||
      ((op->priv->page_position + 1) % op->priv->manual_number_up == 0) ||
      (op->priv->page_position == op->priv->nr_of_pages_to_print - 1))
    cairo_show_page (cr);
}

static void
op_unix_free (CtkPrintOperationUnix *op_unix)
{
  if (op_unix->job)
    {
      if (op_unix->job_status_changed_tag > 0)
        g_signal_handler_disconnect (op_unix->job,
				     op_unix->job_status_changed_tag);
      g_object_unref (op_unix->job);
    }

  g_free (op_unix);
}

static gchar *
shell_command_substitute_file (const gchar *cmd,
			       const gchar *pdf_filename,
			       const gchar *settings_filename,
                               gboolean    *pdf_filename_replaced,
                               gboolean    *settings_filename_replaced)
{
  const gchar *inptr, *start;
  GString *final;

  g_return_val_if_fail (cmd != NULL, NULL);
  g_return_val_if_fail (pdf_filename != NULL, NULL);
  g_return_val_if_fail (settings_filename != NULL, NULL);

  final = g_string_new (NULL);

  *pdf_filename_replaced = FALSE;
  *settings_filename_replaced = FALSE;

  start = inptr = cmd;
  while ((inptr = strchr (inptr, '%')) != NULL) 
    {
      g_string_append_len (final, start, inptr - start);
      inptr++;
      switch (*inptr) 
        {
          case 'f':
            g_string_append (final, pdf_filename);
            *pdf_filename_replaced = TRUE;
            break;

          case 's':
            g_string_append (final, settings_filename);
            *settings_filename_replaced = TRUE;
            break;

          case '%':
            g_string_append_c (final, '%');
            break;

          default:
            g_string_append_c (final, '%');
            if (*inptr)
              g_string_append_c (final, *inptr);
            break;
        }
      if (*inptr)
        inptr++;
      start = inptr;
    }
  g_string_append (final, start);

  return g_string_free (final, FALSE);
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
static void
ctk_print_operation_unix_launch_preview (CtkPrintOperation *op,
                                         cairo_surface_t   *surface,
                                         CtkWindow         *parent,
                                         const gchar       *filename)
{
  GAppInfo *appinfo;
  CdkAppLaunchContext *context;
  gchar *cmd;
  gchar *preview_cmd;
  CtkSettings *settings;
  CtkPrintSettings *print_settings = NULL;
  CtkPageSetup *page_setup;
  GKeyFile *key_file = NULL;
  gchar *data = NULL;
  gsize data_len;
  gchar *settings_filename = NULL;
  gchar *quoted_filename;
  gchar *quoted_settings_filename;
  gboolean filename_used = FALSE;
  gboolean settings_used = FALSE;
  CdkScreen *screen;
  GError *error = NULL;
  gint fd;
  gboolean retval;

  cairo_surface_destroy (surface);
 
  if (parent)
    screen = ctk_window_get_screen (parent);
  else
    screen = cdk_screen_get_default ();

  fd = g_file_open_tmp ("settingsXXXXXX.ini", &settings_filename, &error);
  if (fd < 0) 
    goto out;

  key_file = g_key_file_new ();
  
  print_settings = ctk_print_settings_copy (ctk_print_operation_get_print_settings (op));

  if (print_settings != NULL)
    {
      ctk_print_settings_set_reverse (print_settings, FALSE);
      ctk_print_settings_set_page_set (print_settings, CTK_PAGE_SET_ALL);
      ctk_print_settings_set_scale (print_settings, 1.0);
      ctk_print_settings_set_number_up (print_settings, 1);
      ctk_print_settings_set_number_up_layout (print_settings, CTK_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_TOP_TO_BOTTOM);

      /*  These removals are neccessary because cups-* settings have higher priority
       *  than normal settings.
       */
      ctk_print_settings_unset (print_settings, "cups-reverse");
      ctk_print_settings_unset (print_settings, "cups-page-set");
      ctk_print_settings_unset (print_settings, "cups-scale");
      ctk_print_settings_unset (print_settings, "cups-number-up");
      ctk_print_settings_unset (print_settings, "cups-number-up-layout");

      ctk_print_settings_to_key_file (print_settings, key_file, NULL);
      g_object_unref (print_settings);
    }

  page_setup = ctk_print_context_get_page_setup (op->priv->print_context);
  ctk_page_setup_to_key_file (page_setup, key_file, NULL);

  g_key_file_set_string (key_file, "Print Job", "title", op->priv->job_name);

  data = g_key_file_to_data (key_file, &data_len, &error);
  if (!data)
    goto out;

  retval = g_file_set_contents (settings_filename, data, data_len, &error);
  if (!retval)
    goto out;

  settings = ctk_settings_get_for_screen (screen);
  g_object_get (settings, "ctk-print-preview-command", &preview_cmd, NULL);

  quoted_filename = g_shell_quote (filename);
  quoted_settings_filename = g_shell_quote (settings_filename);
  cmd = shell_command_substitute_file (preview_cmd, quoted_filename, quoted_settings_filename, &filename_used, &settings_used);

  appinfo = g_app_info_create_from_commandline (cmd,
                                                "Print Preview",
                                                G_APP_INFO_CREATE_NONE,
                                                &error);

  g_free (preview_cmd);
  g_free (quoted_filename);
  g_free (quoted_settings_filename);
  g_free (cmd);

  if (error != NULL)
    goto out;

  context = cdk_display_get_app_launch_context (cdk_screen_get_display (screen));
  cdk_app_launch_context_set_screen (context, screen);
  g_app_info_launch (appinfo, NULL, G_APP_LAUNCH_CONTEXT (context), &error);

  g_object_unref (context);
  g_object_unref (appinfo);

  if (error != NULL)
    {
      gchar* uri;

      g_warning ("Error launching preview: %s", error->message);

      g_error_free (error);
      error = NULL;
      uri = g_filename_to_uri (filename, NULL, NULL);
      ctk_show_uri (screen, uri, CDK_CURRENT_TIME, &error);
      g_free (uri);
    }

 out:
  if (error != NULL)
    {
      if (op->priv->error == NULL)
        op->priv->error = error;
      else
        g_error_free (error);

      filename_used = FALSE;
      settings_used = FALSE;
   }

  if (!filename_used)
    g_unlink (filename);

  if (!settings_used)
    g_unlink (settings_filename);

  if (fd > 0)
    close (fd);

  if (key_file)
    g_key_file_free (key_file);
  g_free (data);
  g_free (settings_filename);
}
G_GNUC_END_IGNORE_DEPRECATIONS

static void
unix_finish_send  (CtkPrintJob  *job G_GNUC_UNUSED,
                   gpointer      user_data, 
                   const GError *error)
{
  CtkPrintOperation *op = (CtkPrintOperation *) user_data;
  CtkPrintOperationUnix *op_unix = op->priv->platform_data;

  if (error != NULL && op->priv->error == NULL)
    op->priv->error = g_error_copy (error);

  op_unix->data_sent = TRUE;

  if (op_unix->loop)
    g_main_loop_quit (op_unix->loop);

  g_object_unref (op);
}

static void
unix_end_run (CtkPrintOperation *op,
	      gboolean           wait,
	      gboolean           cancelled)
{
  CtkPrintOperationUnix *op_unix = op->priv->platform_data;

  cairo_surface_finish (op_unix->surface);
  
  if (cancelled)
    return;

  if (wait)
    op_unix->loop = g_main_loop_new (NULL, FALSE);
  
  /* TODO: Check for error */
  if (op_unix->job != NULL)
    {
      g_object_ref (op);
      ctk_print_job_send (op_unix->job,
                          unix_finish_send, 
                          op, NULL);
    }

  if (wait)
    {
      g_object_ref (op);
      if (!op_unix->data_sent)
	{
	  cdk_threads_leave ();  
	  g_main_loop_run (op_unix->loop);
	  cdk_threads_enter ();  
	}
      g_main_loop_unref (op_unix->loop);
      op_unix->loop = NULL;
      g_object_unref (op);
    }
}

static void
job_status_changed_cb (CtkPrintJob       *job, 
		       CtkPrintOperation *op)
{
  _ctk_print_operation_set_status (op, ctk_print_job_get_status (job), NULL);
}


static void
print_setup_changed_cb (CtkPrintUnixDialog *print_dialog, 
                        GParamSpec         *pspec G_GNUC_UNUSED,
                        gpointer            user_data)
{
  CtkPageSetup             *page_setup;
  CtkPrintSettings         *print_settings;
  CtkPrintOperation        *op = user_data;
  CtkPrintOperationPrivate *priv = op->priv;

  page_setup = ctk_print_unix_dialog_get_page_setup (print_dialog);
  print_settings = ctk_print_unix_dialog_get_settings (print_dialog);

  g_signal_emit_by_name (op,
                         "update-custom-widget",
                         priv->custom_widget,
                         page_setup,
                         print_settings);
}

static CtkWidget *
get_print_dialog (CtkPrintOperation *op,
                  CtkWindow         *parent)
{
  CtkPrintOperationPrivate *priv = op->priv;
  CtkWidget *pd, *label;
  const gchar *custom_tab_label;

  pd = ctk_print_unix_dialog_new (NULL, parent);

  ctk_print_unix_dialog_set_manual_capabilities (CTK_PRINT_UNIX_DIALOG (pd),
						 CTK_PRINT_CAPABILITY_PAGE_SET |
						 CTK_PRINT_CAPABILITY_COPIES |
						 CTK_PRINT_CAPABILITY_COLLATE |
						 CTK_PRINT_CAPABILITY_REVERSE |
						 CTK_PRINT_CAPABILITY_SCALE |
						 CTK_PRINT_CAPABILITY_PREVIEW |
						 CTK_PRINT_CAPABILITY_NUMBER_UP |
						 CTK_PRINT_CAPABILITY_NUMBER_UP_LAYOUT);

  if (priv->print_settings)
    ctk_print_unix_dialog_set_settings (CTK_PRINT_UNIX_DIALOG (pd),
					priv->print_settings);

  if (priv->default_page_setup)
    ctk_print_unix_dialog_set_page_setup (CTK_PRINT_UNIX_DIALOG (pd), 
                                          priv->default_page_setup);

  ctk_print_unix_dialog_set_embed_page_setup (CTK_PRINT_UNIX_DIALOG (pd),
                                              priv->embed_page_setup);

  ctk_print_unix_dialog_set_current_page (CTK_PRINT_UNIX_DIALOG (pd), 
                                          priv->current_page);

  ctk_print_unix_dialog_set_support_selection (CTK_PRINT_UNIX_DIALOG (pd),
                                               priv->support_selection);

  ctk_print_unix_dialog_set_has_selection (CTK_PRINT_UNIX_DIALOG (pd),
                                           priv->has_selection);

  g_signal_emit_by_name (op, "create-custom-widget",
			 &priv->custom_widget);

  if (priv->custom_widget) 
    {
      custom_tab_label = priv->custom_tab_label;
      
      if (custom_tab_label == NULL)
	{
	  custom_tab_label = g_get_application_name ();
	  if (custom_tab_label == NULL)
	    custom_tab_label = _("Application");
	}

      label = ctk_label_new (custom_tab_label);
      
      ctk_print_unix_dialog_add_custom_tab (CTK_PRINT_UNIX_DIALOG (pd),
					    priv->custom_widget, label);

      g_signal_connect (pd, "notify::selected-printer", (GCallback) print_setup_changed_cb, op);
      g_signal_connect (pd, "notify::page-setup", (GCallback) print_setup_changed_cb, op);
    }
  
  return pd;
}
  
typedef struct 
{
  CtkPrintOperation           *op;
  gboolean                     do_print;
  gboolean                     do_preview;
  CtkPrintOperationResult      result;
  CtkPrintOperationPrintFunc   print_cb;
  GDestroyNotify               destroy;
  CtkWindow                   *parent;
  GMainLoop                   *loop;
} PrintResponseData;

static void
print_response_data_free (gpointer data)
{
  PrintResponseData *rdata = data;

  g_object_unref (rdata->op);
  g_free (rdata);
}

static void
finish_print (PrintResponseData *rdata,
	      CtkPrinter        *printer,
	      CtkPageSetup      *page_setup,
	      CtkPrintSettings  *settings,
	      gboolean           page_setup_set)
{
  CtkPrintOperation *op = rdata->op;
  CtkPrintOperationPrivate *priv = op->priv;
  CtkPrintJob *job;
  gdouble top, bottom, left, right;
  
  if (rdata->do_print)
    {
      ctk_print_operation_set_print_settings (op, settings);
      priv->print_context = _ctk_print_context_new (op);

      if (ctk_print_settings_get_number_up (settings) < 2)
        {
	  if (printer && (_ctk_printer_get_hard_margins_for_paper_size (printer, ctk_page_setup_get_paper_size (page_setup), &top, &bottom, &left, &right) ||
			  ctk_printer_get_hard_margins (printer, &top, &bottom, &left, &right)))
	    _ctk_print_context_set_hard_margins (priv->print_context, top, bottom, left, right);
	}
      else
        {
	  /* Pages do not have any unprintable area when printing n-up as each page on the
	   * sheet has been scaled down and translated to a position within the printable
	   * area of the sheet.
	   */
	  _ctk_print_context_set_hard_margins (priv->print_context, 0, 0, 0, 0);
	}

      if (page_setup != NULL &&
          (ctk_print_operation_get_default_page_setup (op) == NULL ||
           page_setup_set))
        ctk_print_operation_set_default_page_setup (op, page_setup);

      _ctk_print_context_set_page_setup (priv->print_context, page_setup);

      if (!rdata->do_preview)
        {
	  CtkPrintOperationUnix *op_unix;
	  cairo_t *cr;
	  
	  op_unix = g_new0 (CtkPrintOperationUnix, 1);
	  priv->platform_data = op_unix;
	  priv->free_platform_data = (GDestroyNotify) op_unix_free;
	  op_unix->parent = rdata->parent;
	  
	  priv->start_page = unix_start_page;
	  priv->end_page = unix_end_page;
	  priv->end_run = unix_end_run;
	  
	  job = ctk_print_job_new (priv->job_name, printer, settings, page_setup);
          op_unix->job = job;
          ctk_print_job_set_track_print_status (job, priv->track_print_status);
	  
	  op_unix->surface = ctk_print_job_get_surface (job, &priv->error);
	  if (op_unix->surface == NULL) 
            {
	      rdata->result = CTK_PRINT_OPERATION_RESULT_ERROR;
	      rdata->do_print = FALSE;
	      goto out;
            }
	  
	  cr = cairo_create (op_unix->surface);
	  ctk_print_context_set_cairo_context (priv->print_context, cr, 72, 72);
	  cairo_destroy (cr);

          _ctk_print_operation_set_status (op, ctk_print_job_get_status (job), NULL);
	  
          op_unix->job_status_changed_tag =
	    g_signal_connect (job, "status-changed",
			      G_CALLBACK (job_status_changed_cb), op);
	  
          priv->print_pages = ctk_print_job_get_pages (job);
          priv->page_ranges = ctk_print_job_get_page_ranges (job, &priv->num_page_ranges);
          priv->manual_num_copies = ctk_print_job_get_num_copies (job);
          priv->manual_collation = ctk_print_job_get_collate (job);
          priv->manual_reverse = ctk_print_job_get_reverse (job);
          priv->manual_page_set = ctk_print_job_get_page_set (job);
          priv->manual_scale = ctk_print_job_get_scale (job);
          priv->manual_orientation = ctk_print_job_get_rotate (job);
          priv->manual_number_up = ctk_print_job_get_n_up (job);
          priv->manual_number_up_layout = ctk_print_job_get_n_up_layout (job);
        }
    } 
 out:
  if (rdata->print_cb)
    rdata->print_cb (op, rdata->parent, rdata->do_print, rdata->result); 

  if (rdata->destroy)
    rdata->destroy (rdata);
}

static void 
handle_print_response (CtkWidget *dialog,
		       gint       response,
		       gpointer   data)
{
  CtkPrintUnixDialog *pd = CTK_PRINT_UNIX_DIALOG (dialog);
  PrintResponseData *rdata = data;
  CtkPrintSettings *settings = NULL;
  CtkPageSetup *page_setup = NULL;
  CtkPrinter *printer = NULL;
  gboolean page_setup_set = FALSE;

  if (response == CTK_RESPONSE_OK)
    {
      printer = ctk_print_unix_dialog_get_selected_printer (CTK_PRINT_UNIX_DIALOG (pd));

      rdata->result = CTK_PRINT_OPERATION_RESULT_APPLY;
      rdata->do_preview = FALSE;
      if (printer != NULL)
	rdata->do_print = TRUE;
    } 
  else if (response == CTK_RESPONSE_APPLY)
    {
      /* print preview */
      rdata->result = CTK_PRINT_OPERATION_RESULT_APPLY;
      rdata->do_preview = TRUE;
      rdata->do_print = TRUE;

      rdata->op->priv->action = CTK_PRINT_OPERATION_ACTION_PREVIEW;
    }

  if (rdata->do_print)
    {
      settings = ctk_print_unix_dialog_get_settings (CTK_PRINT_UNIX_DIALOG (pd));
      page_setup = ctk_print_unix_dialog_get_page_setup (CTK_PRINT_UNIX_DIALOG (pd));
      page_setup_set = ctk_print_unix_dialog_get_page_setup_set (CTK_PRINT_UNIX_DIALOG (pd));

      /* Set new print settings now so that custom-widget options
       * can be added to the settings in the callback
       */
      ctk_print_operation_set_print_settings (rdata->op, settings);
      g_signal_emit_by_name (rdata->op, "custom-widget-apply", rdata->op->priv->custom_widget);
    }
  
  finish_print (rdata, printer, page_setup, settings, page_setup_set);

  if (settings)
    g_object_unref (settings);
    
  ctk_widget_destroy (CTK_WIDGET (pd));
 
}


static void
found_printer (CtkPrinter        *printer,
	       PrintResponseData *rdata)
{
  CtkPrintOperation *op = rdata->op;
  CtkPrintOperationPrivate *priv = op->priv;
  CtkPrintSettings *settings = NULL;
  CtkPageSetup *page_setup = NULL;
  
  if (rdata->loop)
    g_main_loop_quit (rdata->loop);

  if (printer != NULL) 
    {
      rdata->result = CTK_PRINT_OPERATION_RESULT_APPLY;

      rdata->do_print = TRUE;

      if (priv->print_settings)
	settings = ctk_print_settings_copy (priv->print_settings);
      else
	settings = ctk_print_settings_new ();

      ctk_print_settings_set_printer (settings,
				      ctk_printer_get_name (printer));
      
      if (priv->default_page_setup)
	page_setup = ctk_page_setup_copy (priv->default_page_setup);
      else
	page_setup = ctk_page_setup_new ();
  }
  
  finish_print (rdata, printer, page_setup, settings, FALSE);

  if (settings)
    g_object_unref (settings);
  
  if (page_setup)
    g_object_unref (page_setup);
}

static void
ctk_print_operation_unix_run_dialog_async (CtkPrintOperation          *op,
                                           gboolean                    show_dialog,
                                           CtkWindow                  *parent,
                                           CtkPrintOperationPrintFunc  print_cb)
{
  CtkWidget *pd;
  PrintResponseData *rdata;
  const gchar *printer_name;

  rdata = g_new (PrintResponseData, 1);
  rdata->op = g_object_ref (op);
  rdata->do_print = FALSE;
  rdata->do_preview = FALSE;
  rdata->result = CTK_PRINT_OPERATION_RESULT_CANCEL;
  rdata->print_cb = print_cb;
  rdata->parent = parent;
  rdata->loop = NULL;
  rdata->destroy = print_response_data_free;
  
  if (show_dialog)
    {
      pd = get_print_dialog (op, parent);
      ctk_window_set_modal (CTK_WINDOW (pd), TRUE);

      g_signal_connect (pd, "response", 
			G_CALLBACK (handle_print_response), rdata);

      ctk_window_present (CTK_WINDOW (pd));
    }
  else
    {
      printer_name = NULL;
      if (op->priv->print_settings)
	printer_name = ctk_print_settings_get_printer (op->priv->print_settings);
      
      find_printer (printer_name, (GFunc) found_printer, rdata);
    }
}

static cairo_status_t
write_preview (void                *closure,
               const unsigned char *data,
               unsigned int         length)
{
  gint fd = GPOINTER_TO_INT (closure);
  gssize written;
  
  while (length > 0) 
    {
      written = write (fd, data, length);

      if (written == -1)
	{
	  if (errno == EAGAIN || errno == EINTR)
	    continue;
	  
	  return CAIRO_STATUS_WRITE_ERROR;
	}    

      data += written;
      length -= written;
    }

  return CAIRO_STATUS_SUCCESS;
}

static void
close_preview (void *data)
{
  gint fd = GPOINTER_TO_INT (data);

  close (fd);
}

static cairo_surface_t *
ctk_print_operation_unix_create_preview_surface (CtkPrintOperation *op G_GNUC_UNUSED,
                                                 CtkPageSetup      *page_setup,
                                                 gdouble           *dpi_x,
                                                 gdouble           *dpi_y,
                                                 gchar            **target)
{
  gchar *filename;
  gint fd;
  CtkPaperSize *paper_size;
  gdouble w, h;
  cairo_surface_t *surface;
  static cairo_user_data_key_t key;
  
  filename = g_build_filename (g_get_tmp_dir (), "previewXXXXXX.pdf", NULL);
  fd = g_mkstemp (filename);

  if (fd < 0)
    {
      g_free (filename);
      return NULL;
    }

  *target = filename;
  
  paper_size = ctk_page_setup_get_paper_size (page_setup);
  w = ctk_paper_size_get_width (paper_size, CTK_UNIT_POINTS);
  h = ctk_paper_size_get_height (paper_size, CTK_UNIT_POINTS);
    
  *dpi_x = *dpi_y = 72;
  surface = cairo_pdf_surface_create_for_stream (write_preview, GINT_TO_POINTER (fd), w, h);
 
  cairo_surface_set_user_data (surface, &key, GINT_TO_POINTER (fd), close_preview);

  return surface;
}

static void
ctk_print_operation_unix_preview_start_page (CtkPrintOperation *op G_GNUC_UNUSED,
                                             cairo_surface_t   *surface G_GNUC_UNUSED,
                                             cairo_t           *cr G_GNUC_UNUSED)
{
}

static void
ctk_print_operation_unix_preview_end_page (CtkPrintOperation *op G_GNUC_UNUSED,
                                           cairo_surface_t   *surface G_GNUC_UNUSED,
                                           cairo_t           *cr)
{
  cairo_show_page (cr);
}

static void
ctk_print_operation_unix_resize_preview_surface (CtkPrintOperation *op G_GNUC_UNUSED,
                                                 CtkPageSetup      *page_setup,
                                                 cairo_surface_t   *surface)
{
  gdouble w, h;
  
  w = ctk_page_setup_get_paper_width (page_setup, CTK_UNIT_POINTS);
  h = ctk_page_setup_get_paper_height (page_setup, CTK_UNIT_POINTS);
  cairo_pdf_surface_set_size (surface, w, h);
}

static CtkPrintOperationResult
ctk_print_operation_unix_run_dialog (CtkPrintOperation *op,
                                     gboolean           show_dialog,
                                     CtkWindow         *parent,
                                     gboolean          *do_print)
 {
  CtkWidget *pd;
  PrintResponseData rdata;
  gint response;  
  const gchar *printer_name;
   
  rdata.op = op;
  rdata.do_print = FALSE;
  rdata.do_preview = FALSE;
  rdata.result = CTK_PRINT_OPERATION_RESULT_CANCEL;
  rdata.print_cb = NULL;
  rdata.destroy = NULL;
  rdata.parent = parent;
  rdata.loop = NULL;

  if (show_dialog)
    {
      pd = get_print_dialog (op, parent);

      response = ctk_dialog_run (CTK_DIALOG (pd));
      handle_print_response (pd, response, &rdata);
    }
  else
    {
      printer_name = NULL;
      if (op->priv->print_settings)
	printer_name = ctk_print_settings_get_printer (op->priv->print_settings);
      
      rdata.loop = g_main_loop_new (NULL, FALSE);
      find_printer (printer_name,
		    (GFunc) found_printer, &rdata);

      cdk_threads_leave ();  
      g_main_loop_run (rdata.loop);
      cdk_threads_enter ();  

      g_main_loop_unref (rdata.loop);
      rdata.loop = NULL;
    }

  *do_print = rdata.do_print;
  
  return rdata.result;
}


typedef struct 
{
  CtkPageSetup         *page_setup;
  CtkPageSetupDoneFunc  done_cb;
  gpointer              data;
  GDestroyNotify        destroy;
} PageSetupResponseData;

static void
page_setup_data_free (gpointer data)
{
  PageSetupResponseData *rdata = data;

  if (rdata->page_setup)
    g_object_unref (rdata->page_setup);

  g_free (rdata);
}

static void
handle_page_setup_response (CtkWidget *dialog,
			    gint       response,
			    gpointer   data)
{
  CtkPageSetupUnixDialog *psd;
  PageSetupResponseData *rdata = data;

  psd = CTK_PAGE_SETUP_UNIX_DIALOG (dialog);
  if (response == CTK_RESPONSE_OK)
    rdata->page_setup = ctk_page_setup_unix_dialog_get_page_setup (psd);

  ctk_widget_destroy (dialog);

  if (rdata->done_cb)
    rdata->done_cb (rdata->page_setup, rdata->data);

  if (rdata->destroy)
    rdata->destroy (rdata);
}

static CtkWidget *
get_page_setup_dialog (CtkWindow        *parent,
		       CtkPageSetup     *page_setup,
		       CtkPrintSettings *settings)
{
  CtkWidget *dialog;

  dialog = ctk_page_setup_unix_dialog_new (NULL, parent);
  if (page_setup)
    ctk_page_setup_unix_dialog_set_page_setup (CTK_PAGE_SETUP_UNIX_DIALOG (dialog),
					       page_setup);
  ctk_page_setup_unix_dialog_set_print_settings (CTK_PAGE_SETUP_UNIX_DIALOG (dialog),
						 settings);

  return dialog;
}

/**
 * ctk_print_run_page_setup_dialog:
 * @parent: (allow-none): transient parent
 * @page_setup: (allow-none): an existing #CtkPageSetup
 * @settings: a #CtkPrintSettings
 *
 * Runs a page setup dialog, letting the user modify the values from
 * @page_setup. If the user cancels the dialog, the returned #CtkPageSetup
 * is identical to the passed in @page_setup, otherwise it contains the 
 * modifications done in the dialog.
 *
 * Note that this function may use a recursive mainloop to show the page
 * setup dialog. See ctk_print_run_page_setup_dialog_async() if this is 
 * a problem.
 * 
 * Returns: (transfer full): a new #CtkPageSetup
 *
 * Since: 2.10
 */
CtkPageSetup *
ctk_print_run_page_setup_dialog (CtkWindow        *parent,
				 CtkPageSetup     *page_setup,
				 CtkPrintSettings *settings)
{
  CtkWidget *dialog;
  gint response;
  PageSetupResponseData rdata;  
  
  rdata.page_setup = NULL;
  rdata.done_cb = NULL;
  rdata.data = NULL;
  rdata.destroy = NULL;

  dialog = get_page_setup_dialog (parent, page_setup, settings);
  response = ctk_dialog_run (CTK_DIALOG (dialog));
  handle_page_setup_response (dialog, response, &rdata);
 
  if (rdata.page_setup)
    return rdata.page_setup;
  else if (page_setup)
    return ctk_page_setup_copy (page_setup);
  else
    return ctk_page_setup_new ();
}

/**
 * ctk_print_run_page_setup_dialog_async:
 * @parent: (allow-none): transient parent, or %NULL
 * @page_setup: (allow-none): an existing #CtkPageSetup, or %NULL
 * @settings: a #CtkPrintSettings
 * @done_cb: (scope async): a function to call when the user saves
 *           the modified page setup
 * @data: user data to pass to @done_cb
 * 
 * Runs a page setup dialog, letting the user modify the values from @page_setup. 
 *
 * In contrast to ctk_print_run_page_setup_dialog(), this function  returns after 
 * showing the page setup dialog on platforms that support this, and calls @done_cb 
 * from a signal handler for the ::response signal of the dialog.
 *
 * Since: 2.10
 */
void
ctk_print_run_page_setup_dialog_async (CtkWindow            *parent,
				       CtkPageSetup         *page_setup,
				       CtkPrintSettings     *settings,
				       CtkPageSetupDoneFunc  done_cb,
				       gpointer              data)
{
  CtkWidget *dialog;
  PageSetupResponseData *rdata;
  
  dialog = get_page_setup_dialog (parent, page_setup, settings);
  ctk_window_set_modal (CTK_WINDOW (dialog), TRUE);
  
  rdata = g_new (PageSetupResponseData, 1);
  rdata->page_setup = NULL;
  rdata->done_cb = done_cb;
  rdata->data = data;
  rdata->destroy = page_setup_data_free;

  g_signal_connect (dialog, "response",
		    G_CALLBACK (handle_page_setup_response), rdata);

  ctk_window_present (CTK_WINDOW (dialog));
 }

struct _PrinterFinder 
{
  gboolean found_printer;
  GFunc func;
  gpointer data;
  gchar *printer_name;
  GList *backends;
  guint timeout_tag;
  CtkPrinter *printer;
  CtkPrinter *default_printer;
  CtkPrinter *first_printer;
};

static gboolean
find_printer_idle (gpointer data)
{
  PrinterFinder *finder = data;
  CtkPrinter *printer;

  if (finder->printer != NULL)
    printer = finder->printer;
  else if (finder->default_printer != NULL)
    printer = finder->default_printer;
  else if (finder->first_printer != NULL)
    printer = finder->first_printer;
  else
    printer = NULL;

  finder->func (printer, finder->data);
  
  printer_finder_free (finder);

  return G_SOURCE_REMOVE;
}

static void
printer_added_cb (CtkPrintBackend *backend G_GNUC_UNUSED,
                  CtkPrinter      *printer,
		  PrinterFinder   *finder)
{
  if (finder->found_printer)
    return;

  /* FIXME this skips "Print to PDF" - is this intentional ? */
  if (ctk_printer_is_virtual (printer))
    return;

  if (finder->printer_name != NULL &&
      strcmp (ctk_printer_get_name (printer), finder->printer_name) == 0)
    {
      finder->printer = g_object_ref (printer);
      finder->found_printer = TRUE;
    }
  else if (finder->default_printer == NULL &&
	   ctk_printer_is_default (printer))
    {
      finder->default_printer = g_object_ref (printer);
      if (finder->printer_name == NULL)
	finder->found_printer = TRUE;
    }
  else
    if (finder->first_printer == NULL)
      finder->first_printer = g_object_ref (printer);
  
  if (finder->found_printer)
    g_idle_add (find_printer_idle, finder);
}

static void
printer_list_done_cb (CtkPrintBackend *backend, 
		      PrinterFinder   *finder)
{
  finder->backends = g_list_remove (finder->backends, backend);
  
  g_signal_handlers_disconnect_by_func (backend, printer_added_cb, finder);
  g_signal_handlers_disconnect_by_func (backend, printer_list_done_cb, finder);
  
  ctk_print_backend_destroy (backend);
  g_object_unref (backend);

  if (finder->backends == NULL && !finder->found_printer)
    g_idle_add (find_printer_idle, finder);
}

static void
find_printer_init (PrinterFinder   *finder,
		   CtkPrintBackend *backend)
{
  GList *list;
  GList *node;

  list = ctk_print_backend_get_printer_list (backend);

  node = list;
  while (node != NULL)
    {
      printer_added_cb (backend, node->data, finder);
      node = node->next;

      if (finder->found_printer)
	break;
    }

  g_list_free (list);

  if (ctk_print_backend_printer_list_is_done (backend))
    {
      finder->backends = g_list_remove (finder->backends, backend);
      ctk_print_backend_destroy (backend);
      g_object_unref (backend);
    }
  else
    {
      g_signal_connect (backend, "printer-added", 
			(GCallback) printer_added_cb, 
			finder);
      g_signal_connect (backend, "printer-list-done", 
			(GCallback) printer_list_done_cb, 
			finder);
    }

}

static void
printer_finder_free (PrinterFinder *finder)
{
  GList *l;
  
  g_free (finder->printer_name);
  
  if (finder->printer)
    g_object_unref (finder->printer);
  
  if (finder->default_printer)
    g_object_unref (finder->default_printer);
  
  if (finder->first_printer)
    g_object_unref (finder->first_printer);

  for (l = finder->backends; l != NULL; l = l->next)
    {
      CtkPrintBackend *backend = l->data;
      g_signal_handlers_disconnect_by_func (backend, printer_added_cb, finder);
      g_signal_handlers_disconnect_by_func (backend, printer_list_done_cb, finder);
      ctk_print_backend_destroy (backend);
      g_object_unref (backend);
    }
  
  g_list_free (finder->backends);
  
  g_free (finder);
}

static void 
find_printer (const gchar *printer,
	      GFunc        func,
	      gpointer     data)
{
  GList *node, *next;
  PrinterFinder *finder;

  finder = g_new0 (PrinterFinder, 1);

  finder->printer_name = g_strdup (printer);
  finder->func = func;
  finder->data = data;
  
  finder->backends = NULL;
  if (g_module_supported ())
    finder->backends = ctk_print_backend_load_modules ();

  for (node = finder->backends; !finder->found_printer && node != NULL; node = next)
    {
      next = node->next;
      find_printer_init (finder, CTK_PRINT_BACKEND (node->data));
    }

  if (finder->backends == NULL && !finder->found_printer)
    g_idle_add (find_printer_idle, finder);
}


CtkPrintOperationResult
_ctk_print_operation_platform_backend_run_dialog (CtkPrintOperation *op,
						  gboolean           show_dialog,
						  CtkWindow         *parent,
						  gboolean          *do_print)
{
  if (ctk_should_use_portal ())
    return ctk_print_operation_portal_run_dialog (op, show_dialog, parent, do_print);
  else
    return ctk_print_operation_unix_run_dialog (op, show_dialog, parent, do_print);
}
void
_ctk_print_operation_platform_backend_run_dialog_async (CtkPrintOperation          *op,
							gboolean                    show_dialog,
                                                        CtkWindow                  *parent,
							CtkPrintOperationPrintFunc  print_cb)
{
  if (ctk_should_use_portal ())
    ctk_print_operation_portal_run_dialog_async (op, show_dialog, parent, print_cb);
  else
    ctk_print_operation_unix_run_dialog_async (op, show_dialog, parent, print_cb);
}

void
_ctk_print_operation_platform_backend_launch_preview (CtkPrintOperation *op,
                                                      cairo_surface_t   *surface,
                                                      CtkWindow         *parent,
                                                      const gchar       *filename)
{
  if (ctk_should_use_portal ())
    ctk_print_operation_portal_launch_preview (op, surface, parent, filename);
  else
    ctk_print_operation_unix_launch_preview (op, surface, parent, filename);
}

cairo_surface_t *
_ctk_print_operation_platform_backend_create_preview_surface (CtkPrintOperation *op,
							      CtkPageSetup      *page_setup,
							      gdouble           *dpi_x,
							      gdouble           *dpi_y,
							      gchar            **target)
{
  return ctk_print_operation_unix_create_preview_surface (op, page_setup, dpi_x, dpi_y, target);
}

void
_ctk_print_operation_platform_backend_resize_preview_surface (CtkPrintOperation *op,
							      CtkPageSetup      *page_setup,
							      cairo_surface_t   *surface)
{
  ctk_print_operation_unix_resize_preview_surface (op, page_setup, surface);
}

void
_ctk_print_operation_platform_backend_preview_start_page (CtkPrintOperation *op,
							  cairo_surface_t   *surface,
							  cairo_t           *cr)
{
  ctk_print_operation_unix_preview_start_page (op, surface, cr);
}

void
_ctk_print_operation_platform_backend_preview_end_page (CtkPrintOperation *op,
							cairo_surface_t   *surface,
							cairo_t           *cr)
{
  ctk_print_operation_unix_preview_end_page (op, surface, cr);
}
