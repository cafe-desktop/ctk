/* CtkPageSetupUnixDialog
 * Copyright (C) 2006 Alexander Larsson <alexl@redhat.com>
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

#ifndef __CTK_PAGE_SETUP_UNIX_DIALOG_H__
#define __CTK_PAGE_SETUP_UNIX_DIALOG_H__

#if !defined (__CTK_UNIX_PRINT_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctkunixprint.h> can be included directly."
#endif

#include <ctk/ctk.h>

G_BEGIN_DECLS

#define CTK_TYPE_PAGE_SETUP_UNIX_DIALOG                  (ctk_page_setup_unix_dialog_get_type ())
#define CTK_PAGE_SETUP_UNIX_DIALOG(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_PAGE_SETUP_UNIX_DIALOG, CtkPageSetupUnixDialog))
#define CTK_PAGE_SETUP_UNIX_DIALOG_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_PAGE_SETUP_UNIX_DIALOG, CtkPageSetupUnixDialogClass))
#define CTK_IS_PAGE_SETUP_UNIX_DIALOG(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_PAGE_SETUP_UNIX_DIALOG))
#define CTK_IS_PAGE_SETUP_UNIX_DIALOG_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_PAGE_SETUP_UNIX_DIALOG))
#define CTK_PAGE_SETUP_UNIX_DIALOG_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_PAGE_SETUP_UNIX_DIALOG, CtkPageSetupUnixDialogClass))


typedef struct _CtkPageSetupUnixDialog         CtkPageSetupUnixDialog;
typedef struct _CtkPageSetupUnixDialogClass    CtkPageSetupUnixDialogClass;
typedef struct _CtkPageSetupUnixDialogPrivate  CtkPageSetupUnixDialogPrivate;

struct _CtkPageSetupUnixDialog
{
  CtkDialog parent_instance;

  CtkPageSetupUnixDialogPrivate *priv;
};

/**
 * CtkPageSetupUnixDialogClass:
 * @parent_class: The parent class.
 */
struct _CtkPageSetupUnixDialogClass
{
  CtkDialogClass parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

CDK_AVAILABLE_IN_ALL
GType 		  ctk_page_setup_unix_dialog_get_type	        (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkWidget *       ctk_page_setup_unix_dialog_new                (const gchar            *title,
								 CtkWindow              *parent);
CDK_AVAILABLE_IN_ALL
void              ctk_page_setup_unix_dialog_set_page_setup     (CtkPageSetupUnixDialog *dialog,
								 CtkPageSetup           *page_setup);
CDK_AVAILABLE_IN_ALL
CtkPageSetup *    ctk_page_setup_unix_dialog_get_page_setup     (CtkPageSetupUnixDialog *dialog);
CDK_AVAILABLE_IN_ALL
void              ctk_page_setup_unix_dialog_set_print_settings (CtkPageSetupUnixDialog *dialog,
								 CtkPrintSettings       *print_settings);
CDK_AVAILABLE_IN_ALL
CtkPrintSettings *ctk_page_setup_unix_dialog_get_print_settings (CtkPageSetupUnixDialog *dialog);

G_END_DECLS

#endif /* __CTK_PAGE_SETUP_UNIX_DIALOG_H__ */
