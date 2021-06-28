/* CtkPrinter
 * Copyright (C) 2006 John (J5) Palmieri <johnp@redhat.com>
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

#ifndef __CTK_PRINTER_H__
#define __CTK_PRINTER_H__

#if !defined (__CTK_UNIX_PRINT_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctkunixprint.h> can be included directly."
#endif

#include <cairo.h>
#include <ctk/ctk.h>

G_BEGIN_DECLS

#define CTK_TYPE_PRINT_CAPABILITIES (ctk_print_capabilities_get_type ())

/* Note, this type is manually registered with GObject in ctkprinter.c
 * If you add any flags, update the registration as well!
 */
/**
 * CtkPrintCapabilities:
 * @CTK_PRINT_CAPABILITY_PAGE_SET: Print dialog will offer printing even/odd pages.
 * @CTK_PRINT_CAPABILITY_COPIES: Print dialog will allow to print multiple copies.
 * @CTK_PRINT_CAPABILITY_COLLATE: Print dialog will allow to collate multiple copies.
 * @CTK_PRINT_CAPABILITY_REVERSE: Print dialog will allow to print pages in reverse order.
 * @CTK_PRINT_CAPABILITY_SCALE: Print dialog will allow to scale the output.
 * @CTK_PRINT_CAPABILITY_GENERATE_PDF: The program will send the document to
 *   the printer in PDF format
 * @CTK_PRINT_CAPABILITY_GENERATE_PS: The program will send the document to
 *   the printer in Postscript format
 * @CTK_PRINT_CAPABILITY_PREVIEW: Print dialog will offer a preview
 * @CTK_PRINT_CAPABILITY_NUMBER_UP: Print dialog will offer printing multiple
 *   pages per sheet. Since 2.12
 * @CTK_PRINT_CAPABILITY_NUMBER_UP_LAYOUT: Print dialog will allow to rearrange
 *   pages when printing multiple pages per sheet. Since 2.14
 *
 * An enum for specifying which features the print dialog should offer.
 * If neither %CTK_PRINT_CAPABILITY_GENERATE_PDF nor
 * %CTK_PRINT_CAPABILITY_GENERATE_PS is specified, CTK+ assumes that all
 * formats are supported.
 */
typedef enum
{
  CTK_PRINT_CAPABILITY_PAGE_SET         = 1 << 0,
  CTK_PRINT_CAPABILITY_COPIES           = 1 << 1,
  CTK_PRINT_CAPABILITY_COLLATE          = 1 << 2,
  CTK_PRINT_CAPABILITY_REVERSE          = 1 << 3,
  CTK_PRINT_CAPABILITY_SCALE            = 1 << 4,
  CTK_PRINT_CAPABILITY_GENERATE_PDF     = 1 << 5,
  CTK_PRINT_CAPABILITY_GENERATE_PS      = 1 << 6,
  CTK_PRINT_CAPABILITY_PREVIEW          = 1 << 7,
  CTK_PRINT_CAPABILITY_NUMBER_UP        = 1 << 8,
  CTK_PRINT_CAPABILITY_NUMBER_UP_LAYOUT = 1 << 9
} CtkPrintCapabilities;

CDK_AVAILABLE_IN_ALL
GType ctk_print_capabilities_get_type (void) G_GNUC_CONST;

#define CTK_TYPE_PRINTER                  (ctk_printer_get_type ())
#define CTK_PRINTER(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_PRINTER, CtkPrinter))
#define CTK_PRINTER_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_PRINTER, CtkPrinterClass))
#define CTK_IS_PRINTER(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_PRINTER))
#define CTK_IS_PRINTER_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_PRINTER))
#define CTK_PRINTER_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_PRINTER, CtkPrinterClass))

typedef struct _CtkPrinter          CtkPrinter;
typedef struct _CtkPrinterClass     CtkPrinterClass;
typedef struct _CtkPrinterPrivate   CtkPrinterPrivate;
typedef struct _CtkPrintBackend     CtkPrintBackend;

struct _CtkPrintBackend;

struct _CtkPrinter
{
  GObject parent_instance;

  /*< private >*/
  CtkPrinterPrivate *priv;
};

struct _CtkPrinterClass
{
  GObjectClass parent_class;

  void (*details_acquired) (CtkPrinter *printer,
                            gboolean    success);

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

CDK_AVAILABLE_IN_ALL
GType                    ctk_printer_get_type              (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkPrinter              *ctk_printer_new                   (const gchar     *name,
							    CtkPrintBackend *backend,
							    gboolean         virtual_);
CDK_AVAILABLE_IN_ALL
CtkPrintBackend         *ctk_printer_get_backend           (CtkPrinter      *printer);
CDK_AVAILABLE_IN_ALL
const gchar *            ctk_printer_get_name              (CtkPrinter      *printer);
CDK_AVAILABLE_IN_ALL
const gchar *            ctk_printer_get_state_message     (CtkPrinter      *printer);
CDK_AVAILABLE_IN_ALL
const gchar *            ctk_printer_get_description       (CtkPrinter      *printer);
CDK_AVAILABLE_IN_ALL
const gchar *            ctk_printer_get_location          (CtkPrinter      *printer);
CDK_AVAILABLE_IN_ALL
const gchar *            ctk_printer_get_icon_name         (CtkPrinter      *printer);
CDK_AVAILABLE_IN_ALL
gint                     ctk_printer_get_job_count         (CtkPrinter      *printer);
CDK_AVAILABLE_IN_ALL
gboolean                 ctk_printer_is_active             (CtkPrinter      *printer);
CDK_AVAILABLE_IN_ALL
gboolean                 ctk_printer_is_paused             (CtkPrinter      *printer);
CDK_AVAILABLE_IN_ALL
gboolean                 ctk_printer_is_accepting_jobs     (CtkPrinter      *printer);
CDK_AVAILABLE_IN_ALL
gboolean                 ctk_printer_is_virtual            (CtkPrinter      *printer);
CDK_AVAILABLE_IN_ALL
gboolean                 ctk_printer_is_default            (CtkPrinter      *printer);
CDK_AVAILABLE_IN_ALL
gboolean                 ctk_printer_accepts_pdf           (CtkPrinter      *printer);
CDK_AVAILABLE_IN_ALL
gboolean                 ctk_printer_accepts_ps            (CtkPrinter      *printer);
CDK_AVAILABLE_IN_ALL
GList                   *ctk_printer_list_papers           (CtkPrinter      *printer);
CDK_AVAILABLE_IN_ALL
CtkPageSetup            *ctk_printer_get_default_page_size (CtkPrinter      *printer);
CDK_AVAILABLE_IN_ALL
gint                     ctk_printer_compare               (CtkPrinter *a,
						    	    CtkPrinter *b);
CDK_AVAILABLE_IN_ALL
gboolean                 ctk_printer_has_details           (CtkPrinter       *printer);
CDK_AVAILABLE_IN_ALL
void                     ctk_printer_request_details       (CtkPrinter       *printer);
CDK_AVAILABLE_IN_ALL
CtkPrintCapabilities     ctk_printer_get_capabilities      (CtkPrinter       *printer);
CDK_AVAILABLE_IN_ALL
gboolean                 ctk_printer_get_hard_margins      (CtkPrinter       *printer,
                                                            gdouble          *top,
                                                            gdouble          *bottom,
                                                            gdouble          *left,
                                                            gdouble          *right);

/**
 * CtkPrinterFunc:
 * @printer: a #CtkPrinter
 * @data: (closure): user data passed to ctk_enumerate_printers()
 *
 * The type of function passed to ctk_enumerate_printers().
 * Note that you need to ref @printer, if you want to keep
 * a reference to it after the function has returned.
 *
 * Returns: %TRUE to stop the enumeration, %FALSE to continue
 *
 * Since: 2.10
 */
typedef gboolean (*CtkPrinterFunc) (CtkPrinter *printer,
				    gpointer    data);

CDK_AVAILABLE_IN_ALL
void                     ctk_enumerate_printers        (CtkPrinterFunc   func,
							gpointer         data,
							GDestroyNotify   destroy,
							gboolean         wait);

G_END_DECLS

#endif /* __CTK_PRINTER_H__ */
