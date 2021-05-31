/* GTK - The GIMP Toolkit
 * ctkprintbackend.h: Abstract printer backend interfaces
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

#ifndef __CTK_PRINT_BACKEND_H__
#define __CTK_PRINT_BACKEND_H__

/* This is a "semi-private" header; it is meant only for
 * alternate CtkPrintDialog backend modules; no stability guarantees
 * are made at this point
 */
#ifndef CTK_PRINT_BACKEND_ENABLE_UNSUPPORTED
#error "CtkPrintBackend is not supported API for general use"
#endif

#include <ctk/ctk.h>
#include <ctk/ctkunixprint.h>
#include <ctk/ctkprinteroptionset.h>

G_BEGIN_DECLS

typedef struct _CtkPrintBackendClass    CtkPrintBackendClass;
typedef struct _CtkPrintBackendPrivate  CtkPrintBackendPrivate;

#define CTK_PRINT_BACKEND_ERROR (ctk_print_backend_error_quark ())

typedef enum
{
  /* TODO: add specific errors */
  CTK_PRINT_BACKEND_ERROR_GENERIC
} CtkPrintBackendError;

GDK_AVAILABLE_IN_ALL
GQuark     ctk_print_backend_error_quark      (void);

#define CTK_TYPE_PRINT_BACKEND                  (ctk_print_backend_get_type ())
#define CTK_PRINT_BACKEND(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_PRINT_BACKEND, CtkPrintBackend))
#define CTK_PRINT_BACKEND_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_PRINT_BACKEND, CtkPrintBackendClass))
#define CTK_IS_PRINT_BACKEND(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_PRINT_BACKEND))
#define CTK_IS_PRINT_BACKEND_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_PRINT_BACKEND))
#define CTK_PRINT_BACKEND_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_PRINT_BACKEND, CtkPrintBackendClass))

typedef enum 
{
  CTK_PRINT_BACKEND_STATUS_UNKNOWN,
  CTK_PRINT_BACKEND_STATUS_OK,
  CTK_PRINT_BACKEND_STATUS_UNAVAILABLE
} CtkPrintBackendStatus;

struct _CtkPrintBackend
{
  GObject parent_instance;

  CtkPrintBackendPrivate *priv;
};

struct _CtkPrintBackendClass
{
  GObjectClass parent_class;

  /* Global backend methods: */
  void                   (*request_printer_list)            (CtkPrintBackend        *backend);
  void                   (*print_stream)                    (CtkPrintBackend        *backend,
							     CtkPrintJob            *job,
							     GIOChannel             *data_io,
							     CtkPrintJobCompleteFunc callback,
							     gpointer                user_data,
							     GDestroyNotify          dnotify);

  /* Printer methods: */
  void                  (*printer_request_details)           (CtkPrinter          *printer);
  cairo_surface_t *     (*printer_create_cairo_surface)      (CtkPrinter          *printer,
							      CtkPrintSettings    *settings,
							      gdouble              height,
							      gdouble              width,
							      GIOChannel          *cache_io);
  CtkPrinterOptionSet * (*printer_get_options)               (CtkPrinter          *printer,
							      CtkPrintSettings    *settings,
							      CtkPageSetup        *page_setup,
							      CtkPrintCapabilities capabilities);
  gboolean              (*printer_mark_conflicts)            (CtkPrinter          *printer,
							      CtkPrinterOptionSet *options);
  void                  (*printer_get_settings_from_options) (CtkPrinter          *printer,
							      CtkPrinterOptionSet *options,
							      CtkPrintSettings    *settings);
  void                  (*printer_prepare_for_print)         (CtkPrinter          *printer,
							      CtkPrintJob         *print_job,
							      CtkPrintSettings    *settings,
							      CtkPageSetup        *page_setup);
  GList  *              (*printer_list_papers)               (CtkPrinter          *printer);
  CtkPageSetup *        (*printer_get_default_page_size)     (CtkPrinter          *printer);
  gboolean              (*printer_get_hard_margins)          (CtkPrinter          *printer,
							      gdouble             *top,
							      gdouble             *bottom,
							      gdouble             *left,
							      gdouble             *right);
  CtkPrintCapabilities  (*printer_get_capabilities)          (CtkPrinter          *printer);

  /* Signals */
  void                  (*printer_list_changed)              (CtkPrintBackend     *backend);
  void                  (*printer_list_done)                 (CtkPrintBackend     *backend);
  void                  (*printer_added)                     (CtkPrintBackend     *backend,
							      CtkPrinter          *printer);
  void                  (*printer_removed)                   (CtkPrintBackend     *backend,
							      CtkPrinter          *printer);
  void                  (*printer_status_changed)            (CtkPrintBackend     *backend,
							      CtkPrinter          *printer);
  void                  (*request_password)                  (CtkPrintBackend     *backend,
                                                              gpointer             auth_info_required,
                                                              gpointer             auth_info_default,
                                                              gpointer             auth_info_display,
                                                              gpointer             auth_info_visible,
                                                              const gchar         *prompt,
                                                              gboolean             can_store_auth_info);

  /* not a signal */
  void                  (*set_password)                      (CtkPrintBackend     *backend,
                                                              gchar              **auth_info_required,
                                                              gchar              **auth_info,
                                                              gboolean             store_auth_info);

  gboolean              (*printer_get_hard_margins_for_paper_size) (CtkPrinter    *printer,
								    CtkPaperSize  *paper_size,
								    gdouble       *top,
								    gdouble       *bottom,
								    gdouble       *left,
								    gdouble       *right);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
};

GDK_AVAILABLE_IN_ALL
GType   ctk_print_backend_get_type       (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
GList      *ctk_print_backend_get_printer_list     (CtkPrintBackend         *print_backend);
GDK_AVAILABLE_IN_ALL
gboolean    ctk_print_backend_printer_list_is_done (CtkPrintBackend         *print_backend);
GDK_AVAILABLE_IN_ALL
CtkPrinter *ctk_print_backend_find_printer         (CtkPrintBackend         *print_backend,
						    const gchar             *printer_name);
GDK_AVAILABLE_IN_ALL
void        ctk_print_backend_print_stream         (CtkPrintBackend         *print_backend,
						    CtkPrintJob             *job,
						    GIOChannel              *data_io,
						    CtkPrintJobCompleteFunc  callback,
						    gpointer                 user_data,
						    GDestroyNotify           dnotify);
GDK_AVAILABLE_IN_ALL
GList *     ctk_print_backend_load_modules         (void);
GDK_AVAILABLE_IN_ALL
void        ctk_print_backend_destroy              (CtkPrintBackend         *print_backend);
GDK_AVAILABLE_IN_ALL
void        ctk_print_backend_set_password         (CtkPrintBackend         *backend, 
                                                    gchar                  **auth_info_required,
                                                    gchar                  **auth_info,
                                                    gboolean                 can_store_auth_info);

/* Backend-only functions for CtkPrintBackend */

GDK_AVAILABLE_IN_ALL
void        ctk_print_backend_add_printer          (CtkPrintBackend         *print_backend,
						    CtkPrinter              *printer);
GDK_AVAILABLE_IN_ALL
void        ctk_print_backend_remove_printer       (CtkPrintBackend         *print_backend,
						    CtkPrinter              *printer);
GDK_AVAILABLE_IN_ALL
void        ctk_print_backend_set_list_done        (CtkPrintBackend         *backend);


/* Backend-only functions for CtkPrinter */
GDK_AVAILABLE_IN_ALL
gboolean    ctk_printer_is_new                (CtkPrinter      *printer);
GDK_AVAILABLE_IN_ALL
void        ctk_printer_set_accepts_pdf       (CtkPrinter      *printer,
					       gboolean         val);
GDK_AVAILABLE_IN_ALL
void        ctk_printer_set_accepts_ps        (CtkPrinter      *printer,
					       gboolean         val);
GDK_AVAILABLE_IN_ALL
void        ctk_printer_set_is_new            (CtkPrinter      *printer,
					       gboolean         val);
GDK_AVAILABLE_IN_ALL
void        ctk_printer_set_is_active         (CtkPrinter      *printer,
					       gboolean         val);
GDK_AVAILABLE_IN_ALL
gboolean    ctk_printer_set_is_paused         (CtkPrinter      *printer,
					       gboolean         val);
GDK_AVAILABLE_IN_ALL
gboolean    ctk_printer_set_is_accepting_jobs (CtkPrinter      *printer,
					       gboolean         val);
GDK_AVAILABLE_IN_ALL
void        ctk_printer_set_has_details       (CtkPrinter      *printer,
					       gboolean         val);
GDK_AVAILABLE_IN_ALL
void        ctk_printer_set_is_default        (CtkPrinter      *printer,
					       gboolean         val);
GDK_AVAILABLE_IN_ALL
void        ctk_printer_set_icon_name         (CtkPrinter      *printer,
					       const gchar     *icon);
GDK_AVAILABLE_IN_ALL
gboolean    ctk_printer_set_job_count         (CtkPrinter      *printer,
					       gint             count);
GDK_AVAILABLE_IN_ALL
gboolean    ctk_printer_set_location          (CtkPrinter      *printer,
					       const gchar     *location);
GDK_AVAILABLE_IN_ALL
gboolean    ctk_printer_set_description       (CtkPrinter      *printer,
					       const gchar     *description);
GDK_AVAILABLE_IN_ALL
gboolean    ctk_printer_set_state_message     (CtkPrinter      *printer,
					       const gchar     *message);


G_END_DECLS

#endif /* __CTK_PRINT_BACKEND_H__ */
