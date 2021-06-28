/* CTK - The GIMP Toolkit
 * ctkprintoperation.h: Print Operation
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
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <cairo.h>
#include <ctk/ctkmain.h>
#include <ctk/ctkwindow.h>
#include <ctk/ctkpagesetup.h>
#include <ctk/ctkprintsettings.h>
#include <ctk/ctkprintcontext.h>
#include <ctk/ctkprintoperationpreview.h>


G_BEGIN_DECLS

#define CTK_TYPE_PRINT_OPERATION                (ctk_print_operation_get_type ())
#define CTK_PRINT_OPERATION(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_PRINT_OPERATION, CtkPrintOperation))
#define CTK_PRINT_OPERATION_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_PRINT_OPERATION, CtkPrintOperationClass))
#define CTK_IS_PRINT_OPERATION(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_PRINT_OPERATION))
#define CTK_IS_PRINT_OPERATION_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_PRINT_OPERATION))
#define CTK_PRINT_OPERATION_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_PRINT_OPERATION, CtkPrintOperationClass))

typedef struct _CtkPrintOperationClass   CtkPrintOperationClass;
typedef struct _CtkPrintOperationPrivate CtkPrintOperationPrivate;
typedef struct _CtkPrintOperation        CtkPrintOperation;

/**
 * CtkPrintStatus:
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
} CtkPrintStatus;

/**
 * CtkPrintOperationResult:
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
} CtkPrintOperationResult;

/**
 * CtkPrintOperationAction:
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
} CtkPrintOperationAction;


struct _CtkPrintOperation
{
  GObject parent_instance;

  /*< private >*/
  CtkPrintOperationPrivate *priv;
};

/**
 * CtkPrintOperationClass:
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
struct _CtkPrintOperationClass
{
  GObjectClass parent_class;

  /*< public >*/

  void     (*done)               (CtkPrintOperation *operation,
                                  CtkPrintOperationResult result);
  void     (*begin_print)        (CtkPrintOperation *operation,
                                  CtkPrintContext   *context);
  gboolean (*paginate)           (CtkPrintOperation *operation,
                                  CtkPrintContext   *context);
  void     (*request_page_setup) (CtkPrintOperation *operation,
                                  CtkPrintContext   *context,
                                  gint               page_nr,
                                  CtkPageSetup      *setup);
  void     (*draw_page)          (CtkPrintOperation *operation,
                                  CtkPrintContext   *context,
                                  gint               page_nr);
  void     (*end_print)          (CtkPrintOperation *operation,
                                  CtkPrintContext   *context);
  void     (*status_changed)     (CtkPrintOperation *operation);

  CtkWidget *(*create_custom_widget) (CtkPrintOperation *operation);
  void       (*custom_widget_apply)  (CtkPrintOperation *operation,
                                      CtkWidget         *widget);

  gboolean (*preview)        (CtkPrintOperation        *operation,
                              CtkPrintOperationPreview *preview,
                              CtkPrintContext          *context,
                              CtkWindow                *parent);

  void     (*update_custom_widget) (CtkPrintOperation *operation,
                                    CtkWidget         *widget,
                                    CtkPageSetup      *setup,
                                    CtkPrintSettings  *settings);

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
 * The error domain for #CtkPrintError errors.
 */
#define CTK_PRINT_ERROR ctk_print_error_quark ()

/**
 * CtkPrintError:
 * @CTK_PRINT_ERROR_GENERAL: An unspecified error occurred.
 * @CTK_PRINT_ERROR_INTERNAL_ERROR: An internal error occurred.
 * @CTK_PRINT_ERROR_NOMEM: A memory allocation failed.
 * @CTK_PRINT_ERROR_INVALID_FILE: An error occurred while loading a page setup
 *     or paper size from a key file.
 *
 * Error codes that identify various errors that can occur while
 * using the CTK+ printing support.
 */
typedef enum
{
  CTK_PRINT_ERROR_GENERAL,
  CTK_PRINT_ERROR_INTERNAL_ERROR,
  CTK_PRINT_ERROR_NOMEM,
  CTK_PRINT_ERROR_INVALID_FILE
} CtkPrintError;

CDK_AVAILABLE_IN_ALL
GQuark ctk_print_error_quark (void);

CDK_AVAILABLE_IN_ALL
GType                   ctk_print_operation_get_type               (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkPrintOperation *     ctk_print_operation_new                    (void);
CDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_set_default_page_setup (CtkPrintOperation  *op,
                                                                    CtkPageSetup       *default_page_setup);
CDK_AVAILABLE_IN_ALL
CtkPageSetup *          ctk_print_operation_get_default_page_setup (CtkPrintOperation  *op);
CDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_set_print_settings     (CtkPrintOperation  *op,
                                                                    CtkPrintSettings   *print_settings);
CDK_AVAILABLE_IN_ALL
CtkPrintSettings *      ctk_print_operation_get_print_settings     (CtkPrintOperation  *op);
CDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_set_job_name           (CtkPrintOperation  *op,
                                                                    const gchar        *job_name);
CDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_set_n_pages            (CtkPrintOperation  *op,
                                                                    gint                n_pages);
CDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_set_current_page       (CtkPrintOperation  *op,
                                                                    gint                current_page);
CDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_set_use_full_page      (CtkPrintOperation  *op,
                                                                    gboolean            full_page);
CDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_set_unit               (CtkPrintOperation  *op,
                                                                    CtkUnit             unit);
CDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_set_export_filename    (CtkPrintOperation  *op,
                                                                    const gchar        *filename);
CDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_set_track_print_status (CtkPrintOperation  *op,
                                                                    gboolean            track_status);
CDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_set_show_progress      (CtkPrintOperation  *op,
                                                                    gboolean            show_progress);
CDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_set_allow_async        (CtkPrintOperation  *op,
                                                                    gboolean            allow_async);
CDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_set_custom_tab_label   (CtkPrintOperation  *op,
                                                                    const gchar        *label);
CDK_AVAILABLE_IN_ALL
CtkPrintOperationResult ctk_print_operation_run                    (CtkPrintOperation  *op,
                                                                    CtkPrintOperationAction action,
                                                                    CtkWindow          *parent,
                                                                    GError            **error);
CDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_get_error              (CtkPrintOperation  *op,
                                                                    GError            **error);
CDK_AVAILABLE_IN_ALL
CtkPrintStatus          ctk_print_operation_get_status             (CtkPrintOperation  *op);
CDK_AVAILABLE_IN_ALL
const gchar *           ctk_print_operation_get_status_string      (CtkPrintOperation  *op);
CDK_AVAILABLE_IN_ALL
gboolean                ctk_print_operation_is_finished            (CtkPrintOperation  *op);
CDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_cancel                 (CtkPrintOperation  *op);
CDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_draw_page_finish       (CtkPrintOperation  *op);
CDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_set_defer_drawing      (CtkPrintOperation  *op);
CDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_set_support_selection  (CtkPrintOperation  *op,
                                                                    gboolean            support_selection);
CDK_AVAILABLE_IN_ALL
gboolean                ctk_print_operation_get_support_selection  (CtkPrintOperation  *op);
CDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_set_has_selection      (CtkPrintOperation  *op,
                                                                    gboolean            has_selection);
CDK_AVAILABLE_IN_ALL
gboolean                ctk_print_operation_get_has_selection      (CtkPrintOperation  *op);
CDK_AVAILABLE_IN_ALL
void                    ctk_print_operation_set_embed_page_setup   (CtkPrintOperation  *op,
                                                                    gboolean            embed);
CDK_AVAILABLE_IN_ALL
gboolean                ctk_print_operation_get_embed_page_setup   (CtkPrintOperation  *op);
CDK_AVAILABLE_IN_ALL
gint                    ctk_print_operation_get_n_pages_to_print   (CtkPrintOperation  *op);

CDK_AVAILABLE_IN_ALL
CtkPageSetup           *ctk_print_run_page_setup_dialog            (CtkWindow          *parent,
                                                                    CtkPageSetup       *page_setup,
                                                                    CtkPrintSettings   *settings);

/**
 * CtkPageSetupDoneFunc:
 * @page_setup: the #CtkPageSetup that has been
 * @data: (closure): user data that has been passed to
 *     ctk_print_run_page_setup_dialog_async()
 *
 * The type of function that is passed to
 * ctk_print_run_page_setup_dialog_async().
 *
 * This function will be called when the page setup dialog
 * is dismissed, and also serves as destroy notify for @data.
 */
typedef void  (* CtkPageSetupDoneFunc) (CtkPageSetup *page_setup,
                                        gpointer      data);

CDK_AVAILABLE_IN_ALL
void                    ctk_print_run_page_setup_dialog_async      (CtkWindow            *parent,
                                                                    CtkPageSetup         *page_setup,
                                                                    CtkPrintSettings     *settings,
                                                                    CtkPageSetupDoneFunc  done_cb,
                                                                    gpointer              data);

G_END_DECLS

#endif /* __CTK_PRINT_OPERATION_H__ */
