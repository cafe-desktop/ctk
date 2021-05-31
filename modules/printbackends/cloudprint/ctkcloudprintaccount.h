/* gtkcloudprintaccount.h: Google Cloud Print account class
 * Copyright (C) 2014, Red Hat, Inc.
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

#ifndef __CTK_CLOUDPRINT_ACCOUNT_H__
#define __CTK_CLOUDPRINT_ACCOUNT_H__

#include <glib-object.h>
#include <json-glib/json-glib.h>

#include "gtkprintbackendcloudprint.h"

G_BEGIN_DECLS

#define CTK_TYPE_CLOUDPRINT_ACCOUNT	(ctk_cloudprint_account_get_type ())
#define CTK_CLOUDPRINT_ACCOUNT(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_CLOUDPRINT_ACCOUNT, GtkCloudprintAccount))
#define CTK_IS_CLOUDPRINT_ACCOUNT(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_CLOUDPRINT_ACCOUNT))

typedef struct _GtkPrinterCloudprint	GtkPrinterCloudprint;
typedef struct _GtkCloudprintAccount	GtkCloudprintAccount;

void	ctk_cloudprint_account_register_type		(GTypeModule *module);
GtkCloudprintAccount *ctk_cloudprint_account_new	(const gchar *id,
							 const gchar *path,
							 const gchar *presentation_identity);
GType	ctk_cloudprint_account_get_type			(void) G_GNUC_CONST;

void	ctk_cloudprint_account_search		(GtkCloudprintAccount *account,
						 GDBusConnection *connection,
						 GCancellable *cancellable,
						 GAsyncReadyCallback callback,
						 gpointer user_data);
JsonNode *ctk_cloudprint_account_search_finish	(GtkCloudprintAccount *account,
						 GAsyncResult *result,
						 GError **error);

void	ctk_cloudprint_account_printer		(GtkCloudprintAccount *account,
						 const gchar *printerid,
						 GCancellable *cancellable,
						 GAsyncReadyCallback callback,
						 gpointer user_data);
JsonObject *ctk_cloudprint_account_printer_finish (GtkCloudprintAccount *account,
						   GAsyncResult *result,
						   GError **error);

void	ctk_cloudprint_account_submit		(GtkCloudprintAccount *account,
						 GtkPrinterCloudprint *printer,
						 GMappedFile *file,
						 const gchar *title,
						 GCancellable *cancellable,
						 GAsyncReadyCallback callback,
						 gpointer user_data);
JsonObject *ctk_cloudprint_account_submit_finish (GtkCloudprintAccount *account,
						  GAsyncResult *result,
						  GError **error);

const gchar *ctk_cloudprint_account_get_presentation_identity (GtkCloudprintAccount *account);

G_END_DECLS

#endif /* __CTK_CLOUDPRINT_ACCOUNT_H__ */