/* CtkPrinterCups
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

#ifndef __CTK_PRINTER_CUPS_H__
#define __CTK_PRINTER_CUPS_H__

#include <glib-object.h>
#include <cups/cups.h>
#include <cups/ppd.h>
#include "ctkcupsutils.h"

#include <ctk/ctkunixprint.h>
#include <ctk/ctkprinter-private.h>

#ifdef HAVE_COLORD
#include <colord.h>
#endif

G_BEGIN_DECLS

#define CTK_TYPE_PRINTER_CUPS                  (ctk_printer_cups_get_type ())
#define CTK_PRINTER_CUPS(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_PRINTER_CUPS, CtkPrinterCups))
#define CTK_PRINTER_CUPS_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_PRINTER_CUPS, CtkPrinterCupsClass))
#define CTK_IS_PRINTER_CUPS(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_PRINTER_CUPS))
#define CTK_IS_PRINTER_CUPS_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_PRINTER_CUPS))
#define CTK_PRINTER_CUPS_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_PRINTER_CUPS, CtkPrinterCupsClass))

typedef struct _CtkPrinterCups	        CtkPrinterCups;
typedef struct _CtkPrinterCupsClass     CtkPrinterCupsClass;
typedef struct _CtkPrinterCupsPrivate   CtkPrinterCupsPrivate;

struct _CtkPrinterCups
{
  CtkPrinter parent_instance;

  gchar *device_uri;
  gchar *original_device_uri;
  gchar *printer_uri;
  gchar *hostname;
  gint port;
  gchar **auth_info_required;
  gchar *original_hostname;
  gchar *original_resource;
  gint original_port;
  gboolean request_original_uri;     /* Request PPD from original hostname */
  gboolean is_temporary;             /* This printer is temporary queue */
  gchar *temporary_queue_device_uri; /* Device uri of temporary queue for this printer */

  ipp_pstate_t state;
  gboolean reading_ppd;
  gchar      *ppd_name;
  ppd_file_t *ppd_file;

  gchar    *media_default;
  GList    *media_supported;
  GList    *media_size_supported;
  gint      media_bottom_margin_default;
  gint      media_top_margin_default;
  gint      media_left_margin_default;
  gint      media_right_margin_default;
  gboolean  media_margin_default_set;
  gchar    *sides_default;
  GList    *sides_supported;
  gchar    *output_bin_default;
  GList    *output_bin_supported;

  gchar  *default_cover_before;
  gchar  *default_cover_after;

  gint    default_number_up;

  gboolean remote;
  guint get_remote_ppd_poll;
  gint  get_remote_ppd_attempts;
  CtkCupsConnectionTest *remote_cups_connection_test;

#ifdef HAVE_COLORD
  CdClient     *colord_client;
  CdDevice     *colord_device;
  CdProfile    *colord_profile;
  GCancellable *colord_cancellable;
  gchar        *colord_title;
  gchar        *colord_qualifier;
#endif

  gboolean  avahi_browsed;
  char *avahi_name;
  char *avahi_type;
  char *avahi_domain;

  guchar ipp_version_major;
  guchar ipp_version_minor;
  gboolean supports_copies;
  gboolean supports_collate;
  gboolean supports_number_up;
  char   **covers;
  int      number_of_covers;
};

struct _CtkPrinterCupsClass
{
  CtkPrinterClass parent_class;

};

GType                    ctk_printer_cups_get_type      (void) G_GNUC_CONST;
void                     ctk_printer_cups_register_type (GTypeModule     *module);

CtkPrinterCups          *ctk_printer_cups_new           (const char      *name,
                                                         CtkPrintBackend *backend,
                                                         gpointer         colord_client);
ppd_file_t 		*ctk_printer_cups_get_ppd       (CtkPrinterCups  *printer);
const gchar		*ctk_printer_cups_get_ppd_name  (CtkPrinterCups  *printer);

#ifdef HAVE_COLORD
void                     ctk_printer_cups_update_settings (CtkPrinterCups *printer,
                                                         CtkPrintSettings *settings,
                                                         CtkPrinterOptionSet *set);
#endif

G_END_DECLS

#endif /* __CTK_PRINTER_CUPS_H__ */
