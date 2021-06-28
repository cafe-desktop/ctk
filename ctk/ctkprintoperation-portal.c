/* CTK - The GIMP Toolkit
 * ctkprintoperation-portal.c: Print Operation Details for sandboxed apps
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

#include "config.h"

#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <cairo-pdf.h>
#include <cairo-ps.h>

#include <gio/gunixfdlist.h>

#include "ctkprintoperation-private.h"
#include "ctkprintoperation-portal.h"
#include "ctkprintsettings.h"
#include "ctkpagesetup.h"
#include "ctkprintbackend.h"
#include "ctkshow.h"
#include "ctkintl.h"
#include "ctkwindowprivate.h"
#include "ctkprivate.h"


typedef struct {
  CtkPrintOperation *op;
  GDBusProxy *proxy;
  guint response_signal_id;
  gboolean do_print;
  CtkPrintOperationResult result;
  CtkPrintOperationPrintFunc print_cb;
  CtkWindow *parent;
  GMainLoop *loop;
  guint32 token;
  GDestroyNotify destroy;
  GVariant *settings;
  GVariant *setup;
  GVariant *options;
  char *prepare_print_handle;
} PortalData;

static void
portal_data_free (gpointer data)
{
  PortalData *portal = data;

  g_object_unref (portal->op);
  g_object_unref (portal->proxy);
  if (portal->loop)
    g_main_loop_unref (portal->loop);
  if (portal->settings)
    g_variant_unref (portal->settings);
  if (portal->setup)
    g_variant_unref (portal->setup);
  if (portal->options)
    g_variant_unref (portal->options);
  g_free (portal->prepare_print_handle);
  g_free (portal);
}

typedef struct {
  GDBusProxy *proxy;
  CtkPrintJob *job;
  guint32 token;
  cairo_surface_t *surface;
  GMainLoop *loop;
  gboolean file_written;
} CtkPrintOperationPortal;

static void
op_portal_free (CtkPrintOperationPortal *op_portal)
{
  g_clear_object (&op_portal->proxy);
  g_clear_object (&op_portal->job);
  if (op_portal->loop)
    g_main_loop_unref (op_portal->loop);
  g_free (op_portal);
}

static void
portal_start_page (CtkPrintOperation *op,
                   CtkPrintContext   *print_context,
                   CtkPageSetup      *page_setup)
{
  CtkPrintOperationPortal *op_portal = op->priv->platform_data;
  CtkPaperSize *paper_size;
  cairo_surface_type_t type;
  gdouble w, h;

  paper_size = ctk_page_setup_get_paper_size (page_setup);

  w = ctk_paper_size_get_width (paper_size, CTK_UNIT_POINTS);
  h = ctk_paper_size_get_height (paper_size, CTK_UNIT_POINTS);

  type = cairo_surface_get_type (op_portal->surface);

  if ((op->priv->manual_number_up < 2) ||
      (op->priv->page_position % op->priv->manual_number_up == 0))
    {
      if (type == CAIRO_SURFACE_TYPE_PS)
        {
          cairo_ps_surface_set_size (op_portal->surface, w, h);
          cairo_ps_surface_dsc_begin_page_setup (op_portal->surface);
          switch (ctk_page_setup_get_orientation (page_setup))
            {
              case CTK_PAGE_ORIENTATION_PORTRAIT:
              case CTK_PAGE_ORIENTATION_REVERSE_PORTRAIT:
                cairo_ps_surface_dsc_comment (op_portal->surface, "%%PageOrientation: Portrait");
                break;

              case CTK_PAGE_ORIENTATION_LANDSCAPE:
              case CTK_PAGE_ORIENTATION_REVERSE_LANDSCAPE:
                cairo_ps_surface_dsc_comment (op_portal->surface, "%%PageOrientation: Landscape");
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
          cairo_pdf_surface_set_size (op_portal->surface, w, h);
        }
    }
}

static void
portal_end_page (CtkPrintOperation *op,
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
print_file_done (GObject *source,
                 GAsyncResult *result,
                 gpointer data)
{
  CtkPrintOperation *op = data;
  CtkPrintOperationPortal *op_portal = op->priv->platform_data;
  GError *error = NULL;
  GVariant *ret;

  ret = g_dbus_proxy_call_finish (op_portal->proxy,
                                  result,
                                  &error);
  if (ret == NULL)
    {
      if (op->priv->error == NULL)
        op->priv->error = g_error_copy (error);
      g_warning ("Print file failed: %s", error->message);
      g_error_free (error);
    }
  else
    g_variant_unref (ret);

  if (op_portal->loop)
    g_main_loop_quit (op_portal->loop);

  g_object_unref (op);
}

static void
portal_job_complete (CtkPrintJob  *job,
                     gpointer      data,
                     const GError *error)
{
  CtkPrintOperation *op = data;
  CtkPrintOperationPortal *op_portal = op->priv->platform_data;
  CtkPrintSettings *settings;
  const char *uri;
  char *filename;
  int fd, idx;
  GVariantBuilder opt_builder;
  GUnixFDList *fd_list;

  if (error != NULL && op->priv->error == NULL)
    {
      g_warning ("Print job failed: %s", error->message);
      op->priv->error = g_error_copy (error);
      return;
    }

  op_portal->file_written = TRUE;

  settings = ctk_print_job_get_settings (job);
  uri = ctk_print_settings_get (settings, CTK_PRINT_SETTINGS_OUTPUT_URI);
  filename = g_filename_from_uri (uri, NULL, NULL);

  fd = open (filename, O_RDONLY|O_CLOEXEC);
  fd_list = g_unix_fd_list_new ();
  idx = g_unix_fd_list_append (fd_list, fd, NULL);
  close (fd);

  g_free (filename);

  g_variant_builder_init (&opt_builder, G_VARIANT_TYPE_VARDICT);
  g_variant_builder_add (&opt_builder, "{sv}",  "token", g_variant_new_uint32 (op_portal->token));

  g_dbus_proxy_call_with_unix_fd_list (op_portal->proxy,
                                       "Print",
                                       g_variant_new ("(ssh@a{sv})",
                                                      "", /* window */
                                                      _("Print"), /* title */
                                                      idx,
                                                      g_variant_builder_end (&opt_builder)),
                                       G_DBUS_CALL_FLAGS_NONE,
                                       -1,
                                       fd_list,
                                       NULL,
                                       print_file_done,
                                       op);
  g_object_unref (fd_list);
}

static void
portal_end_run (CtkPrintOperation *op,
                gboolean           wait,
                gboolean           cancelled)
{
  CtkPrintOperationPortal *op_portal = op->priv->platform_data;

  cairo_surface_finish (op_portal->surface);

  if (cancelled)
    return;

  if (wait)
    op_portal->loop = g_main_loop_new (NULL, FALSE);

  /* TODO: Check for error */
  if (op_portal->job != NULL)
    {
      g_object_ref (op);
      ctk_print_job_send (op_portal->job, portal_job_complete, op, NULL);
    }

  if (wait)
    {
      g_object_ref (op);
      if (!op_portal->file_written)
        {
          cdk_threads_leave ();
          g_main_loop_run (op_portal->loop);
          cdk_threads_enter ();
        }
      g_object_unref (op);
    }
}

static void
finish_print (PortalData        *portal,
              CtkPrinter        *printer,
              CtkPageSetup      *page_setup,
              CtkPrintSettings  *settings)
{
  CtkPrintOperation *op = portal->op;
  CtkPrintOperationPrivate *priv = op->priv;
  CtkPrintJob *job;
  CtkPrintOperationPortal *op_portal;
  cairo_t *cr;

  if (portal->do_print)
    {
      ctk_print_operation_set_print_settings (op, settings);
      priv->print_context = _ctk_print_context_new (op);

      _ctk_print_context_set_hard_margins (priv->print_context, 0, 0, 0, 0);

      ctk_print_operation_set_default_page_setup (op, page_setup);
      _ctk_print_context_set_page_setup (priv->print_context, page_setup);

      op_portal = g_new0 (CtkPrintOperationPortal, 1);
      priv->platform_data = op_portal;
      priv->free_platform_data = (GDestroyNotify) op_portal_free;

      priv->start_page = portal_start_page;
      priv->end_page = portal_end_page;
      priv->end_run = portal_end_run;

      job = ctk_print_job_new (priv->job_name, printer, settings, page_setup);
      op_portal->job = job;

      op_portal->proxy = g_object_ref (portal->proxy);
      op_portal->token = portal->token;

      op_portal->surface = ctk_print_job_get_surface (job, &priv->error);
      if (op_portal->surface == NULL)
        {
          portal->result = CTK_PRINT_OPERATION_RESULT_ERROR;
          portal->do_print = FALSE;
          goto out;
        }

      cr = cairo_create (op_portal->surface);
      ctk_print_context_set_cairo_context (priv->print_context, cr, 72, 72);
      cairo_destroy (cr);

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

out:
  if (portal->print_cb)
    portal->print_cb (op, portal->parent, portal->do_print, portal->result);

  if (portal->destroy)
    portal->destroy (portal);
}

static CtkPrinter *
find_file_printer (void)
{
  GList *backends, *l, *printers;
  CtkPrinter *printer;

  printer = NULL;

  backends = ctk_print_backend_load_modules ();
  for (l = backends; l; l = l->next)
    {
      CtkPrintBackend *backend = l->data;
      if (strcmp (G_OBJECT_TYPE_NAME (backend), "CtkPrintBackendFile") == 0)
        {
          printers = ctk_print_backend_get_printer_list (backend);
          printer = printers->data;
          g_list_free (printers);
          break;
        }
    }
  g_list_free (backends);

  return printer;
}

static void
prepare_print_response (GDBusConnection *connection,
                        const char      *sender_name,
                        const char      *object_path,
                        const char      *interface_name,
                        const char      *signal_name,
                        GVariant        *parameters,
                        gpointer         data)
{
  PortalData *portal = data;
  guint32 response;
  GVariant *options;

  if (portal->response_signal_id != 0)
    {
      g_dbus_connection_signal_unsubscribe (connection,
                                            portal->response_signal_id);
      portal->response_signal_id = 0;
    }

  g_variant_get (parameters, "(u@a{sv})", &response, &options);

  portal->do_print = (response == 0);

  if (portal->do_print)
    {
      GVariant *v;
      CtkPrintSettings *settings;
      CtkPageSetup *page_setup;
      CtkPrinter *printer;
      char *filename;
      char *uri;
      int fd;

      portal->result = CTK_PRINT_OPERATION_RESULT_APPLY;

      v = g_variant_lookup_value (options, "settings", G_VARIANT_TYPE_VARDICT);
      settings = ctk_print_settings_new_from_gvariant (v);
      g_variant_unref (v);

      v = g_variant_lookup_value (options, "page-setup", G_VARIANT_TYPE_VARDICT);
      page_setup = ctk_page_setup_new_from_gvariant (v);
      g_variant_unref (v);

      g_variant_lookup (options, "token", "u", &portal->token);

      printer = find_file_printer ();

      fd = g_file_open_tmp ("ctkprintXXXXXX", &filename, NULL);
      uri = g_filename_to_uri (filename, NULL, NULL);
      ctk_print_settings_set (settings, CTK_PRINT_SETTINGS_OUTPUT_URI, uri);
      g_free (uri);
      close (fd);

      finish_print (portal, printer, page_setup, settings);
      g_free (filename);
    }
  else
    {
      portal->result = CTK_PRINT_OPERATION_RESULT_CANCEL;

      if (portal->print_cb)
	  portal->print_cb (portal->op, portal->parent, portal->do_print, portal->result);

      if (portal->destroy)
	  portal->destroy (portal);
    }
  if (portal->loop)
    g_main_loop_quit (portal->loop);
}

static void
prepare_print_called (GObject      *source,
                      GAsyncResult *result,
                      gpointer      data)
{
  PortalData *portal = data;
  GError *error = NULL;
  const char *handle = NULL;
  GVariant *ret;

  ret = g_dbus_proxy_call_finish (portal->proxy, result, &error);
  if (ret == NULL)
    {
      if (portal->op->priv->error == NULL)
        portal->op->priv->error = g_error_copy (error);
      g_error_free (error);
      if (portal->loop)
        g_main_loop_quit (portal->loop);
      return;
    }
  else
    g_variant_get (ret, "(&o)", &handle);

  if (strcmp (portal->prepare_print_handle, handle) != 0)
    {
      g_free (portal->prepare_print_handle);
      portal->prepare_print_handle = g_strdup (handle);
      g_dbus_connection_signal_unsubscribe (g_dbus_proxy_get_connection (G_DBUS_PROXY (portal->proxy)),
                                            portal->response_signal_id);
      portal->response_signal_id =
        g_dbus_connection_signal_subscribe (g_dbus_proxy_get_connection (G_DBUS_PROXY (portal->proxy)),
                                            "org.freedesktop.portal.Desktop",
                                            "org.freedesktop.portal.Request",
                                            "Response",
                                            handle,
                                            NULL,
                                            G_DBUS_SIGNAL_FLAGS_NO_MATCH_RULE,
                                            prepare_print_response,
                                            portal, NULL);
     }

  g_variant_unref (ret);
}

PortalData *
create_portal_data (CtkPrintOperation          *op,
                    CtkWindow                  *parent,
                    CtkPrintOperationPrintFunc  print_cb)
{
  GDBusProxy *proxy;
  PortalData *portal;
  guint signal_id;
  GError *error = NULL;

  signal_id = g_signal_lookup ("create-custom-widget", CTK_TYPE_PRINT_OPERATION);
  if (g_signal_has_handler_pending (op, signal_id, 0, TRUE))
    g_warning ("CtkPrintOperation::create-custom-widget not supported with portal");

  proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                         G_DBUS_PROXY_FLAGS_NONE,
                                         NULL,
                                         "org.freedesktop.portal.Desktop",
                                         "/org/freedesktop/portal/desktop",
                                         "org.freedesktop.portal.Print",
                                         NULL,
                                         &error);

  if (proxy == NULL)
    {
      if (op->priv->error == NULL)
        op->priv->error = g_error_copy (error);
      g_error_free (error);
      return NULL;
    }

  portal = g_new0 (PortalData, 1);
  portal->proxy = proxy;
  portal->op = g_object_ref (op);
  portal->parent = parent;
  portal->result = CTK_PRINT_OPERATION_RESULT_CANCEL;
  portal->print_cb = print_cb;

  if (print_cb) /* async case */
    {
      portal->loop = NULL;
      portal->destroy = portal_data_free;
    }
  else
    {
      portal->loop = g_main_loop_new (NULL, FALSE);
      portal->destroy = NULL;
    }

  return portal;
}

static void
window_handle_exported (CtkWindow  *window,
                        const char *handle_str,
                        gpointer    user_data)
{
  PortalData *portal = user_data;

  g_dbus_proxy_call (portal->proxy,
                     "PreparePrint",
                     g_variant_new ("(ss@a{sv}@a{sv}@a{sv})",
                                    handle_str,
                                    _("Print"), /* title */
                                    portal->settings,
                                    portal->setup,
                                    portal->options),
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     NULL,
                     prepare_print_called,
                     portal);
}

static void
call_prepare_print (CtkPrintOperation *op,
                    PortalData        *portal)
{
  CtkPrintOperationPrivate *priv = op->priv;
  GVariantBuilder opt_builder;
  char *token;

  portal->prepare_print_handle =
      ctk_get_portal_request_path (g_dbus_proxy_get_connection (portal->proxy), &token);

  portal->response_signal_id =
    g_dbus_connection_signal_subscribe (g_dbus_proxy_get_connection (G_DBUS_PROXY (portal->proxy)),
                                        "org.freedesktop.portal.Desktop",
                                        "org.freedesktop.portal.Request",
                                        "Response",
                                        portal->prepare_print_handle,
                                        NULL,
                                        G_DBUS_SIGNAL_FLAGS_NO_MATCH_RULE,
                                        prepare_print_response,
                                        portal, NULL);

  g_variant_builder_init (&opt_builder, G_VARIANT_TYPE_VARDICT);
  g_variant_builder_add (&opt_builder, "{sv}", "handle_token", g_variant_new_string (token));
  g_free (token);
  portal->options = g_variant_builder_end (&opt_builder);

  if (priv->print_settings)
    portal->settings = ctk_print_settings_to_gvariant (priv->print_settings);
  else
    {
      GVariantBuilder builder;
      g_variant_builder_init (&builder, G_VARIANT_TYPE_VARDICT);
      portal->settings = g_variant_builder_end (&builder);
    }

  if (priv->default_page_setup)
    portal->setup = ctk_page_setup_to_gvariant (priv->default_page_setup);
  else
    {
      CtkPageSetup *page_setup = ctk_page_setup_new ();
      portal->setup = ctk_page_setup_to_gvariant (page_setup);
      g_object_unref (page_setup);
    }

  g_variant_ref_sink (portal->options);
  g_variant_ref_sink (portal->settings);
  g_variant_ref_sink (portal->setup);

  if (portal->parent != NULL &&
      ctk_widget_is_visible (CTK_WIDGET (portal->parent)) &&
      ctk_window_export_handle (portal->parent, window_handle_exported, portal))
    return;

  g_dbus_proxy_call (portal->proxy,
                     "PreparePrint",
                     g_variant_new ("(ss@a{sv}@a{sv}@a{sv})",
                                    "",
                                    _("Print"), /* title */
                                    portal->settings,
                                    portal->setup,
                                    portal->options),
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     NULL,
                     prepare_print_called,
                     portal);
}

CtkPrintOperationResult
ctk_print_operation_portal_run_dialog (CtkPrintOperation *op,
                                       gboolean           show_dialog,
                                       CtkWindow         *parent,
                                       gboolean          *do_print)
{
  PortalData *portal;
  CtkPrintOperationResult result;

  portal = create_portal_data (op, parent, NULL);
  if (portal == NULL)
    return CTK_PRINT_OPERATION_RESULT_ERROR;

  call_prepare_print (op, portal);

  cdk_threads_leave ();
  g_main_loop_run (portal->loop);
  cdk_threads_enter ();

  *do_print = portal->do_print;
  result = portal->result;

  portal_data_free (portal);

  return result;
}

void
ctk_print_operation_portal_run_dialog_async (CtkPrintOperation          *op,
                                             gboolean                    show_dialog,
                                             CtkWindow                  *parent,
                                             CtkPrintOperationPrintFunc  print_cb)
{
  PortalData *portal;

  portal = create_portal_data (op, parent, print_cb);
  if (portal == NULL)
    return;

  call_prepare_print (op, portal);
}

void
ctk_print_operation_portal_launch_preview (CtkPrintOperation *op,
                                           cairo_surface_t   *surface,
                                           CtkWindow         *parent,
                                           const char        *filename)
{
  char *uri;

  uri = g_filename_to_uri (filename, NULL, NULL);
  ctk_show_uri_on_window (parent, uri, GDK_CURRENT_TIME, NULL);
  g_free (uri);
}
