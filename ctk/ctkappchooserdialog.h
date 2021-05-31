/*
 * ctkappchooserdialog.h: an app-chooser dialog
 *
 * Copyright (C) 2004 Novell, Inc.
 * Copyright (C) 2007, 2010 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Dave Camp <dave@novell.com>
 *          Alexander Larsson <alexl@redhat.com>
 *          Cosimo Cecchi <ccecchi@redhat.com>
 */

#ifndef __CTK_APP_CHOOSER_DIALOG_H__
#define __CTK_APP_CHOOSER_DIALOG_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkdialog.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define CTK_TYPE_APP_CHOOSER_DIALOG            (ctk_app_chooser_dialog_get_type ())
#define CTK_APP_CHOOSER_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_APP_CHOOSER_DIALOG, CtkAppChooserDialog))
#define CTK_APP_CHOOSER_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_APP_CHOOSER_DIALOG, CtkAppChooserDialogClass))
#define CTK_IS_APP_CHOOSER_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_APP_CHOOSER_DIALOG))
#define CTK_IS_APP_CHOOSER_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_APP_CHOOSER_DIALOG))
#define CTK_APP_CHOOSER_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_APP_CHOOSER_DIALOG, CtkAppChooserDialogClass))

typedef struct _CtkAppChooserDialog        CtkAppChooserDialog;
typedef struct _CtkAppChooserDialogClass   CtkAppChooserDialogClass;
typedef struct _CtkAppChooserDialogPrivate CtkAppChooserDialogPrivate;

struct _CtkAppChooserDialog {
  CtkDialog parent;

  /*< private >*/
  CtkAppChooserDialogPrivate *priv;
};

/**
 * CtkAppChooserDialogClass:
 * @parent_class: The parent class.
 */
struct _CtkAppChooserDialogClass {
  CtkDialogClass parent_class;

  /*< private >*/

  /* padding for future class expansion */
  gpointer padding[16];
};

GDK_AVAILABLE_IN_ALL
GType         ctk_app_chooser_dialog_get_type             (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
CtkWidget *   ctk_app_chooser_dialog_new                  (CtkWindow           *parent,
                                                           CtkDialogFlags       flags,
                                                           GFile               *file);
GDK_AVAILABLE_IN_ALL
CtkWidget *   ctk_app_chooser_dialog_new_for_content_type (CtkWindow           *parent,
                                                           CtkDialogFlags       flags,
                                                           const gchar         *content_type);

GDK_AVAILABLE_IN_ALL
CtkWidget *   ctk_app_chooser_dialog_get_widget           (CtkAppChooserDialog *self);
GDK_AVAILABLE_IN_ALL
void          ctk_app_chooser_dialog_set_heading          (CtkAppChooserDialog *self,
                                                           const gchar         *heading);
GDK_AVAILABLE_IN_ALL
const gchar * ctk_app_chooser_dialog_get_heading          (CtkAppChooserDialog *self);

G_END_DECLS

#endif /* __CTK_APP_CHOOSER_DIALOG_H__ */
