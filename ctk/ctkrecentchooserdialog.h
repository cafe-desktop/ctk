/* GTK - The GIMP Toolkit
 * ctkrecentchooserdialog.h: Recent files selector dialog
 * Copyright (C) 2006 Emmanuele Bassi
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

#ifndef __CTK_RECENT_CHOOSER_DIALOG_H__
#define __CTK_RECENT_CHOOSER_DIALOG_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkdialog.h>
#include <ctk/ctkrecentchooser.h>

G_BEGIN_DECLS

#define CTK_TYPE_RECENT_CHOOSER_DIALOG		  (ctk_recent_chooser_dialog_get_type ())
#define CTK_RECENT_CHOOSER_DIALOG(obj)		  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_RECENT_CHOOSER_DIALOG, CtkRecentChooserDialog))
#define CTK_IS_RECENT_CHOOSER_DIALOG(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_RECENT_CHOOSER_DIALOG))
#define CTK_RECENT_CHOOSER_DIALOG_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_RECENT_CHOOSER_DIALOG, CtkRecentChooserDialogClass))
#define CTK_IS_RECENT_CHOOSER_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_RECENT_CHOOSER_DIALOG))
#define CTK_RECENT_CHOOSER_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_RECENT_CHOOSER_DIALOG, CtkRecentChooserDialogClass))

typedef struct _CtkRecentChooserDialog        CtkRecentChooserDialog;
typedef struct _CtkRecentChooserDialogClass   CtkRecentChooserDialogClass;

typedef struct _CtkRecentChooserDialogPrivate CtkRecentChooserDialogPrivate;


struct _CtkRecentChooserDialog
{
  CtkDialog parent_instance;

  /*< private >*/
  CtkRecentChooserDialogPrivate *priv;
};

struct _CtkRecentChooserDialogClass
{
  CtkDialogClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


GDK_AVAILABLE_IN_ALL
GType      ctk_recent_chooser_dialog_get_type        (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
CtkWidget *ctk_recent_chooser_dialog_new             (const gchar      *title,
					              CtkWindow        *parent,
					              const gchar      *first_button_text,
					              ...) G_GNUC_NULL_TERMINATED;
GDK_AVAILABLE_IN_ALL
CtkWidget *ctk_recent_chooser_dialog_new_for_manager (const gchar      *title,
						      CtkWindow        *parent,
						      CtkRecentManager *manager,
						      const gchar      *first_button_text,
						      ...) G_GNUC_NULL_TERMINATED;

G_END_DECLS

#endif /* __CTK_RECENT_CHOOSER_DIALOG_H__ */
