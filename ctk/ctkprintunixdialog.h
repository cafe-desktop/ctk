/* CtkPrintUnixDialog
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

#ifndef __CTK_PRINT_UNIX_DIALOG_H__
#define __CTK_PRINT_UNIX_DIALOG_H__

#if !defined (__CTK_UNIX_PRINT_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctkunixprint.h> can be included directly."
#endif

#include <ctk/ctk.h>
#include <ctk/ctkprinter.h>
#include <ctk/ctkprintjob.h>

G_BEGIN_DECLS

#define CTK_TYPE_PRINT_UNIX_DIALOG                  (ctk_print_unix_dialog_get_type ())
#define CTK_PRINT_UNIX_DIALOG(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_PRINT_UNIX_DIALOG, CtkPrintUnixDialog))
#define CTK_PRINT_UNIX_DIALOG_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_PRINT_UNIX_DIALOG, CtkPrintUnixDialogClass))
#define CTK_IS_PRINT_UNIX_DIALOG(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_PRINT_UNIX_DIALOG))
#define CTK_IS_PRINT_UNIX_DIALOG_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_PRINT_UNIX_DIALOG))
#define CTK_PRINT_UNIX_DIALOG_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_PRINT_UNIX_DIALOG, CtkPrintUnixDialogClass))


typedef struct _CtkPrintUnixDialog         CtkPrintUnixDialog;
typedef struct _CtkPrintUnixDialogClass    CtkPrintUnixDialogClass;
typedef struct CtkPrintUnixDialogPrivate   CtkPrintUnixDialogPrivate;

struct _CtkPrintUnixDialog
{
  CtkDialog parent_instance;

  /*< private >*/
  CtkPrintUnixDialogPrivate *priv;
};

struct _CtkPrintUnixDialogClass
{
  CtkDialogClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
GType                ctk_print_unix_dialog_get_type                (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
CtkWidget *          ctk_print_unix_dialog_new                     (const gchar *title,
                                                                    CtkWindow   *parent);

GDK_AVAILABLE_IN_ALL
void                 ctk_print_unix_dialog_set_page_setup          (CtkPrintUnixDialog *dialog,
								    CtkPageSetup       *page_setup);
GDK_AVAILABLE_IN_ALL
CtkPageSetup *       ctk_print_unix_dialog_get_page_setup          (CtkPrintUnixDialog *dialog);
GDK_AVAILABLE_IN_ALL
void                 ctk_print_unix_dialog_set_current_page        (CtkPrintUnixDialog *dialog,
								    gint                current_page);
GDK_AVAILABLE_IN_ALL
gint                 ctk_print_unix_dialog_get_current_page        (CtkPrintUnixDialog *dialog);
GDK_AVAILABLE_IN_ALL
void                 ctk_print_unix_dialog_set_settings            (CtkPrintUnixDialog *dialog,
								    CtkPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
CtkPrintSettings *   ctk_print_unix_dialog_get_settings            (CtkPrintUnixDialog *dialog);
GDK_AVAILABLE_IN_ALL
CtkPrinter *         ctk_print_unix_dialog_get_selected_printer    (CtkPrintUnixDialog *dialog);
GDK_AVAILABLE_IN_ALL
void                 ctk_print_unix_dialog_add_custom_tab          (CtkPrintUnixDialog *dialog,
								    CtkWidget          *child,
								    CtkWidget          *tab_label);
GDK_AVAILABLE_IN_ALL
void                 ctk_print_unix_dialog_set_manual_capabilities (CtkPrintUnixDialog *dialog,
								    CtkPrintCapabilities capabilities);
GDK_AVAILABLE_IN_ALL
CtkPrintCapabilities ctk_print_unix_dialog_get_manual_capabilities (CtkPrintUnixDialog  *dialog);
GDK_AVAILABLE_IN_ALL
void                 ctk_print_unix_dialog_set_support_selection   (CtkPrintUnixDialog  *dialog,
								    gboolean             support_selection);
GDK_AVAILABLE_IN_ALL
gboolean             ctk_print_unix_dialog_get_support_selection   (CtkPrintUnixDialog  *dialog);
GDK_AVAILABLE_IN_ALL
void                 ctk_print_unix_dialog_set_has_selection       (CtkPrintUnixDialog  *dialog,
								    gboolean             has_selection);
GDK_AVAILABLE_IN_ALL
gboolean             ctk_print_unix_dialog_get_has_selection       (CtkPrintUnixDialog  *dialog);
GDK_AVAILABLE_IN_ALL
void                 ctk_print_unix_dialog_set_embed_page_setup    (CtkPrintUnixDialog *dialog,
								    gboolean            embed);
GDK_AVAILABLE_IN_ALL
gboolean             ctk_print_unix_dialog_get_embed_page_setup    (CtkPrintUnixDialog *dialog);
GDK_AVAILABLE_IN_ALL
gboolean             ctk_print_unix_dialog_get_page_setup_set      (CtkPrintUnixDialog *dialog);

G_END_DECLS

#endif /* __CTK_PRINT_UNIX_DIALOG_H__ */
