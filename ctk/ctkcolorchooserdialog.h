/* CTK - The GIMP Toolkit
 * Copyright (C) 2012 Red Hat, Inc.
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

#ifndef __CTK_COLOR_CHOOSER_DIALOG_H__
#define __CTK_COLOR_CHOOSER_DIALOG_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkdialog.h>

G_BEGIN_DECLS

#define CTK_TYPE_COLOR_CHOOSER_DIALOG              (ctk_color_chooser_dialog_get_type ())
#define CTK_COLOR_CHOOSER_DIALOG(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_COLOR_CHOOSER_DIALOG, CtkColorChooserDialog))
#define CTK_COLOR_CHOOSER_DIALOG_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_COLOR_CHOOSER_DIALOG, CtkColorChooserDialogClass))
#define CTK_IS_COLOR_CHOOSER_DIALOG(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_COLOR_CHOOSER_DIALOG))
#define CTK_IS_COLOR_CHOOSER_DIALOG_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_COLOR_CHOOSER_DIALOG))
#define CTK_COLOR_CHOOSER_DIALOG_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_COLOR_CHOOSER_DIALOG, CtkColorChooserDialogClass))

typedef struct _CtkColorChooserDialog        CtkColorChooserDialog;
typedef struct _CtkColorChooserDialogPrivate CtkColorChooserDialogPrivate;
typedef struct _CtkColorChooserDialogClass   CtkColorChooserDialogClass;

struct _CtkColorChooserDialog
{
  CtkDialog parent_instance;

  /*< private >*/
  CtkColorChooserDialogPrivate *priv;
};

struct _CtkColorChooserDialogClass
{
  CtkDialogClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_3_4
GType       ctk_color_chooser_dialog_get_type (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_3_4
CtkWidget * ctk_color_chooser_dialog_new      (const gchar *title,
                                               CtkWindow   *parent);

G_END_DECLS

#endif /* __CTK_COLOR_CHOOSER_DIALOG_H__ */
