/* GTK - The GIMP Toolkit
 * gtkprintoperation.h: Print Operation
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

#ifndef __CTK_PRINT_OPERATION_H__
#define __CTK_PRINT_OPERATION_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <cairo.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtkpagesetup.h>
#include <gtk/gtkprintsettings.h>
#include <gtk/gtkprintcontext.h>
#include <gtk/gtkprintoperationpreview.h>


G_BEGIN_DECLS

#define CTK_TYPE_PRINT_OPERATION                (ctk_print_operation_get_type ())
#define CTK_PRINT_OPERATION(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_PRINT_OPERATION, GtkPrintOperation))
#define CTK_PRINT_OPERATION_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_PRINT_OPERATION, GtkPrintOperationClass))
#define CTK_IS_PRINT_OPERATION(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_PRINT_OPERATION))
#define CTK_IS_PRINT_OPERATION_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_PRINT_OPERATION))
#define CTK_PRINT_OPERATION_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_PRINT_OPERATION, GtkPrintOperationClass))

typedef struct _GtkPrintOperationClass   GtkPrintOperationClass;
typedef struct _GtkPrintOperationPrivate GtkPrintOperationPrivate;
typedef struct _GtkPrintOperation        GtkPrintOperation;

/**
 * GtkPrintStatus:
 * @CTK_PRINT_STATUS_INITIAL: The printing has not started yet; this
 *     status is set initially, and while the print dialog is shown.
 * @CTK_PRINT_STATUS_PREPARING: This status is set while the begin-print
 *     signal is emitted and during pagination.
 * @CTK_PRINT_STATUS_GENERATING_DATA: This status is set while the
 *     pages are being rendered.
 * @CTK_PRINT_STATUS_SENDING_DATA: The print job is being sent off to the
 *     printer.
 * @CTK_PRINT_STATUS_PENDING: The print job has been sent to the printer,
 *     but is not printed for some reason, e.g. the printer may be stopped.
 * @CTK_PRINT_STATUS_PENDING_ISSUE: Some problem has occurred during
 *     printing, e.g. a paper jam.
 * @CTK_PRINT_STATUS_PRINTING: The printer is processing the print job.
 * @CTK_PRINT_STATUS_FINISHED: The printing has been completed successfully.
 * @CTK_PRINT_STATUS_FINISHED_ABORTED: The printing has been aborted.
 *
 * The status gives a rough indication of the completion of a running
 * print operation.
 */
typedef enum {
  CTK_PRINT_STATUS_INITIAL,
  CTK_PRINT_STATUS_PREPARING,
  CTK_PRINT_STATUS_GENERATING_DATA,
  CTK_PRINT_STATUS_SENDING_DATA,
  CTK_PRINT_STATUS_PENDING,
  CTK_PRINT_STATUS_PENDING_ISSUE,
  CTK_PRINT_STATUS_PRINTING,
  CTK_PRINT_STATUS_FINISHED,
  CTK_PRINT_STATUS_FINISHED_ABORTED
} GtkPrintStatus;

/**
 * GtkPrintOperationResult:
 * @CTK_PRINT_OPERATION_RESULT_ERROR: An error has occurred.
 * @CTK_PRINT_OPERATION_RESULT_APPLY: The print settings should be stored.
 * @CTK_PRINT_OPERATION_RESULT_CANCEL: The print operation has been canceled,
 *     the print settings should not be stored.
 * @CTK_PRINT_OPERATION_RESULT_IN_PROGRESS: The print operation is not complete
 *     yet. This value will only be returned when running asynchronously.
 *
 * A value of this type is returned by ctk_print_operation_run().
 */
typedef enum {
  CTK_PRINT_OPERATION_RESULT_ERROR,
  CTK_PRINT_OPERATION_RESULT_APPLY,
  CTK_PRINT_OPERATION_RESULT_CANCEL,
  CTK_PRINT_OPERATION_RESULT_IN_PROGRESS
} GtkPrintOperationResult;

/**
 * GtkPrintOperationAction:
 * @CTK_PRINT_OPERATION_ACTION_PRINT_DIALOG: Show the print dialog.
 * @CTK_PRINT_OPERATION_ACTION_PRINT: Start to print without showing
 *     the print dialog, based on the current print settings.
 * @CTK_PRINT_OPERATION_ACTION_PREVIEW: Show the print preview.
 * @CTK_PRINT_OPERATION_ACTION_EXPORT: Export to a file. This requires
 *     the export-filename property to be set.
 *
 * The @action parameter to ctk_print_operation_run()
 * determines what action the print operation should perform.
 */
typedef enum {
  CTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
  CTK_PRINT_OPERATION_ACTION_PRINT,
  CTK_PRINT_OPERATION_ACTION_PREVIEW,
  CTK_PRINT_OPERATION_ACTION_EXPORT
} GtkPrintOperationAction;


struct _GtkPrintOperation
{
  GObject parent_instance;

  /*< private >*/
  GtkPrintOperationPrivate *priv;
};

/**
 * GtkPrintOperationClass:
 * @parent_class: The parent class.
 * @done: Signal emitted when the print operation run has finished
 *    doing everything required for printing.
 * @begin_print: Signal emitted after the user has finished changing
 *    print settings in the dialog, before the actual rendering starts.
 * @paginate: Signal emitted after the “begin-print” signal, but
 *    before the actual rendering starts.
 * @request_page_setup: Emitted once for every page that is printed,
 *    to give the application a chance to modify the page setup.
 * @draw_page: Signal emitted for every page that is printed.
 * @end_print: Signal emitted after all pages have been rendered.
 * @status_changed: Emitted at between the various phases of the print
 *    operation.
 * @create_custom_widget: Signal emitted when displaying the print dialog.
 * @custom_widget_apply: Signal emitted right before “begin-print” if
 *    you added a custom widget in the “create-custom-widget” handler.
 * @preview: Signal emitted when a preview is requested from the
 *    native dialog.
 * @update_custom_widget: Emitted after change of selected printer.
 */
struct _GtkPrintOperationClass
{
  GObjectClass parent_class;

  /*< public >*/

  void     (*done)               (GtkPrintOperation *operation,
                                  GtkPrintOperationResult result);
  void     (*begin_print)        (GtkPrintOperation *operation,
                                  GtkPrintContext   *context);
  gboolean (*paginate)           (GtkPrintOperation *operation,
                                  GtkPrintContext   *context);
  void     (*request_page_setup) (GtkPrintOperation *operation,
                                  GtkPrintContext   *context,
                                  gint               page_nr,
                                  GtkPageSetup      *setup);
  void     (*draw_page)          (GtkPrintOperation *operation,
                                  GtkPrintContext   *context,
                                  gint               page_nr);
  void     (*end_print)          (GtkPrintOperation *operation,
                                  GtkPrintContext   *context);
  void     (*status_changed)     (GtkPrintOperation *operation);

  GtkWidget *(*create_custom_widget) (GtkPrintOperation *operation);
  void       (*custom_widget_apply)  (GtkPrintOperation *operation,
                                      GtkWidget         *widget);

  gboolean (*preview)        (GtkPrintOperation        *operation,
                              GtkPrintOperationPreview *preview,
                              GtkPrintContext          *context,
                              GtkWindow                *parent);

  void     (*update_custom_widget) (GtkPrintOperation *operation,
                                    GtkWidget         *widget,
                                    GtkPageSetup      *setup,
                                    GtkPrintSettings  *settings);

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
  void (*_ctk_reserved5) (void);
  void (*_ctk_reserved6) (void);
  void (*_ctk_reserved7) (void);
  void (*_ctk_reserved8) (void);
};

/**
 * CTK_PRINT_ERROR:
 *
 * The error domain for #GtkPrintError errors.
 */
#define CTK_PRINT_ERROR ctk_print_error_quark ()

/**
 * GtkPrintError:
 * @CTK_PRINT_ERROR_GENERAL: An unspecified error occurred.
 * @CTK_PRINT_ERROR_INTERNAL_ERROR: An internal error occurred.
 * @CTK_PRINT_ERROR_NOMEM: A memory allocation failed.
 * @CTK_PRINT_ERROR_INVALID_FILE: An error occurred while loading a page setup
 *     or paper size from a key file.
 *
 * Error codes that identify various errors that can occur while
 * using the GTK+ printing support.
 */
typedef enum
{
  CTK_PRINT_ERROR_GENERAL,
  CTK_PRINT_ERROR_INTERNAL_ERROR,
  CTK_PRINT_ERROR_NOMEM,
  CTK_PRINT_ERROR_INVALID_FILE
} GtkPrintError;

GDK_AVAILABLE_IN_ALL
GQuark ctk_print_error_quark (void);

GDK_AVAILABLE_IN_ALL
GType                   ctk_print_operation_get_type               (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
GtkPrintOperation *     ctk_print_operation_new                    (void);
GDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_set_default_page_setup (GtkPrintOperation  *op,
                                                                    GtkPageSetup       *default_page_setup);
GDK_AVAILABLE_IN_ALL
GtkPageSetup *          ctk_print_operation_get_default_page_setup (GtkPrintOperation  *op);
GDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_set_print_settings     (GtkPrintOperation  *op,
                                                                    GtkPrintSettings   *print_settings);
GDK_AVAILABLE_IN_ALL
GtkPrintSettings *      ctk_print_operation_get_print_settings     (GtkPrintOperation  *op);
GDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_set_job_name           (GtkPrintOperation  *op,
                                                                    const gchar        *job_name);
GDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_set_n_pages            (GtkPrintOperation  *op,
                                                                    gint                n_pages);
GDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_set_current_page       (GtkPrintOperation  *op,
                                                                    gint                current_page);
GDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_set_use_full_page      (GtkPrintOperation  *op,
                                                                    gboolean            full_page);
GDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_set_unit               (GtkPrintOperation  *op,
                                                                    GtkUnit             unit);
GDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_set_export_filename    (GtkPrintOperation  *op,
                                                                    const gchar        *filename);
GDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_set_track_print_status (GtkPrintOperation  *op,
                                                                    gboolean            track_status);
GDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_set_show_progress      (GtkPrintOperation  *op,
                                                                    gboolean            show_progress);
GDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_set_allow_async        (GtkPrintOperation  *op,
                                                                    gboolean            allow_async);
GDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_set_custom_tab_label   (GtkPrintOperation  *op,
                                                                    const gchar        *label);
GDK_AVAILABLE_IN_ALL
GtkPrintOperationResult ctk_print_operation_run                    (GtkPrintOperation  *op,
                                                                    GtkPrintOperationAction action,
                                                                    GtkWindow          *parent,
                                                                    GError            **error);
GDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_get_error              (GtkPrintOperation  *op,
                                                                    GError            **error);
GDK_AVAILABLE_IN_ALL
GtkPrintStatus          ctk_print_operation_get_status             (GtkPrintOperation  *op);
GDK_AVAILABLE_IN_ALL
const gchar *           ctk_print_operation_get_status_string      (GtkPrintOperation  *op);
GDK_AVAILABLE_IN_ALL
gboolean                ctk_print_operation_is_finished            (GtkPrintOperation  *op);
GDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_cancel                 (GtkPrintOperation  *op);
GDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_draw_page_finish       (GtkPrintOperation  *op);
GDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_set_defer_drawing      (GtkPrintOperation  *op);
GDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_set_support_selection  (GtkPrintOperation  *op,
                                                                    gboolean            support_selection);
GDK_AVAILABLE_IN_ALL
gboolean                ctk_print_operation_get_support_selection  (GtkPrintOperation  *op);
GDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_set_has_selection      (GtkPrintOperation  *op,
                                                                    gboolean            has_selection);
GDK_AVAILABLE_IN_ALL
gboolean                ctk_print_operation_get_has_selection      (GtkPrintOperation  *op);
GDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_set_embed_page_setup   (GtkPrintOperation  *op,
                                                                    gboolean            embed);
GDK_AVAILABLE_IN_ALL
gboolean                ctk_print_operation_get_embed_page_setup   (GtkPrintOperation  *op);
GDK_AVAILABLE_IN_ALL
gint                    ctk_print_operation_get_n_pages_to_print   (GtkPrintOperation  *op);

GDK_AVAILABLE_IN_ALL
GtkPageSetup           *ctk_print_run_page_setup_dialog            (GtkWindow          *parent,
                                                                    GtkPageSetup       *page_setup,
                                                                    GtkPrintSettings   *settings);

/**
 * GtkPageSetupDoneFunc:
 * @page_setup: the #GtkPageSetup that has been
 * @data: (closure): user data that has been passed to
 *     ctk_print_run_page_setup_dialog_async()
 *
 * The type of function that is passed to
 * ctk_print_run_page_setup_dialog_async().
 *
 * This function will be called when the page setup dialog
 * is dismissed, and also serves as destroy notify for @data.
 */
typedef void  (* GtkPageSetupDoneFunc) (GtkPageSetup *page_setup,
                                        gpointer      data);

GDK_AVAILABLE_IN_ALL
void                    ctk_print_run_page_setup_dialog_async      (GtkWindow            *parent,
                                                                    GtkPageSetup         *page_setup,
                                                                    GtkPrintSettings     *settings,
                                                                    GtkPageSetupDoneFunc  done_cb,
                                                                    gpointer              data);

G_END_DECLS

#endif /* __CTK_PRINT_OPERATION_H__ */