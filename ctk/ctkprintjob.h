/* CtkPrintJob
 * Copyright (C) 2006 Red Hat,Inc.
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

#ifndef __CTK_PRINT_JOB_H__
#define __CTK_PRINT_JOB_H__

#if !defined (__CTK_UNIX_PRINT_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctkunixprint.h> can be included directly."
#endif

#include <cairo.h>

#include <ctk/ctk.h>
#include <ctk/ctkprinter.h>

G_BEGIN_DECLS

#define CTK_TYPE_PRINT_JOB                  (ctk_print_job_get_type ())
#define CTK_PRINT_JOB(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_PRINT_JOB, CtkPrintJob))
#define CTK_PRINT_JOB_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_PRINT_JOB, CtkPrintJobClass))
#define CTK_IS_PRINT_JOB(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_PRINT_JOB))
#define CTK_IS_PRINT_JOB_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_PRINT_JOB))
#define CTK_PRINT_JOB_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_PRINT_JOB, CtkPrintJobClass))

typedef struct _CtkPrintJob          CtkPrintJob;
typedef struct _CtkPrintJobClass     CtkPrintJobClass;
typedef struct _CtkPrintJobPrivate   CtkPrintJobPrivate;

/**
 * CtkPrintJobCompleteFunc:
 * @print_job: the #CtkPrintJob
 * @user_data: user data that has been passed to ctk_print_job_send()
 * @error: a #GError that contains error information if the sending
 *     of the print job failed, otherwise %NULL
 *
 * The type of callback that is passed to ctk_print_job_send().
 * It is called when the print job has been completely sent.
 */
typedef void (*CtkPrintJobCompleteFunc) (CtkPrintJob  *print_job,
                                         gpointer      user_data,
                                         const GError *error);

struct _CtkPrinter;

struct _CtkPrintJob
{
  GObject parent_instance;

  CtkPrintJobPrivate *priv;
};

struct _CtkPrintJobClass
{
  GObjectClass parent_class;

  void (*status_changed) (CtkPrintJob *job);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

CDK_AVAILABLE_IN_ALL
GType                    ctk_print_job_get_type               (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkPrintJob             *ctk_print_job_new                    (const gchar              *title,
							       CtkPrinter               *printer,
							       CtkPrintSettings         *settings,
							       CtkPageSetup             *page_setup);
CDK_AVAILABLE_IN_ALL
CtkPrintSettings        *ctk_print_job_get_settings           (CtkPrintJob              *job);
CDK_AVAILABLE_IN_ALL
CtkPrinter              *ctk_print_job_get_printer            (CtkPrintJob              *job);
CDK_AVAILABLE_IN_ALL
const gchar *            ctk_print_job_get_title              (CtkPrintJob              *job);
CDK_AVAILABLE_IN_ALL
CtkPrintStatus           ctk_print_job_get_status             (CtkPrintJob              *job);
CDK_AVAILABLE_IN_ALL
gboolean                 ctk_print_job_set_source_file        (CtkPrintJob              *job,
							       const gchar              *filename,
							       GError                  **error);
CDK_AVAILABLE_IN_3_22
gboolean                 ctk_print_job_set_source_fd          (CtkPrintJob              *job,
							       int                       fd,
							       GError                  **error);
CDK_AVAILABLE_IN_ALL
cairo_surface_t         *ctk_print_job_get_surface            (CtkPrintJob              *job,
							       GError                  **error);
CDK_AVAILABLE_IN_ALL
void                     ctk_print_job_set_track_print_status (CtkPrintJob              *job,
							       gboolean                  track_status);
CDK_AVAILABLE_IN_ALL
gboolean                 ctk_print_job_get_track_print_status (CtkPrintJob              *job);
CDK_AVAILABLE_IN_ALL
void                     ctk_print_job_send                   (CtkPrintJob              *job,
							       CtkPrintJobCompleteFunc   callback,
							       gpointer                  user_data,
							       GDestroyNotify            dnotify);

CDK_AVAILABLE_IN_ALL
CtkPrintPages     ctk_print_job_get_pages       (CtkPrintJob       *job);
CDK_AVAILABLE_IN_ALL
void              ctk_print_job_set_pages       (CtkPrintJob       *job,
                                                 CtkPrintPages      pages);
CDK_AVAILABLE_IN_ALL
CtkPageRange *    ctk_print_job_get_page_ranges (CtkPrintJob       *job,
                                                 gint              *n_ranges);
CDK_AVAILABLE_IN_ALL
void              ctk_print_job_set_page_ranges (CtkPrintJob       *job,
                                                 CtkPageRange      *ranges,
                                                 gint               n_ranges);
CDK_AVAILABLE_IN_ALL
CtkPageSet        ctk_print_job_get_page_set    (CtkPrintJob       *job);
CDK_AVAILABLE_IN_ALL
void              ctk_print_job_set_page_set    (CtkPrintJob       *job,
                                                 CtkPageSet         page_set);
CDK_AVAILABLE_IN_ALL
gint              ctk_print_job_get_num_copies  (CtkPrintJob       *job);
CDK_AVAILABLE_IN_ALL
void              ctk_print_job_set_num_copies  (CtkPrintJob       *job,
                                                 gint               num_copies);
CDK_AVAILABLE_IN_ALL
gdouble           ctk_print_job_get_scale       (CtkPrintJob       *job);
CDK_AVAILABLE_IN_ALL
void              ctk_print_job_set_scale       (CtkPrintJob       *job,
                                                 gdouble            scale);
CDK_AVAILABLE_IN_ALL
guint             ctk_print_job_get_n_up        (CtkPrintJob       *job);
CDK_AVAILABLE_IN_ALL
void              ctk_print_job_set_n_up        (CtkPrintJob       *job,
                                                 guint              n_up);
CDK_AVAILABLE_IN_ALL
CtkNumberUpLayout ctk_print_job_get_n_up_layout (CtkPrintJob       *job);
CDK_AVAILABLE_IN_ALL
void              ctk_print_job_set_n_up_layout (CtkPrintJob       *job,
                                                 CtkNumberUpLayout  layout);
CDK_AVAILABLE_IN_ALL
gboolean          ctk_print_job_get_rotate      (CtkPrintJob       *job);
CDK_AVAILABLE_IN_ALL
void              ctk_print_job_set_rotate      (CtkPrintJob       *job,
                                                 gboolean           rotate);
CDK_AVAILABLE_IN_ALL
gboolean          ctk_print_job_get_collate     (CtkPrintJob       *job);
CDK_AVAILABLE_IN_ALL
void              ctk_print_job_set_collate     (CtkPrintJob       *job,
                                                 gboolean           collate);
CDK_AVAILABLE_IN_ALL
gboolean          ctk_print_job_get_reverse     (CtkPrintJob       *job);
CDK_AVAILABLE_IN_ALL
void              ctk_print_job_set_reverse     (CtkPrintJob       *job,
                                                 gboolean           reverse);

G_END_DECLS

#endif /* __CTK_PRINT_JOB_H__ */
