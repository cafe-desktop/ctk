/* CTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_COLOR_SELECTION_DIALOG_H__
#define __CTK_COLOR_SELECTION_DIALOG_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkdialog.h>
#include <ctk/deprecated/ctkcolorsel.h>

G_BEGIN_DECLS

#define CTK_TYPE_COLOR_SELECTION_DIALOG            (ctk_color_selection_dialog_get_type ())
#define CTK_COLOR_SELECTION_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_COLOR_SELECTION_DIALOG, CtkColorSelectionDialog))
#define CTK_COLOR_SELECTION_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_COLOR_SELECTION_DIALOG, CtkColorSelectionDialogClass))
#define CTK_IS_COLOR_SELECTION_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_COLOR_SELECTION_DIALOG))
#define CTK_IS_COLOR_SELECTION_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_COLOR_SELECTION_DIALOG))
#define CTK_COLOR_SELECTION_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_COLOR_SELECTION_DIALOG, CtkColorSelectionDialogClass))


typedef struct _CtkColorSelectionDialog              CtkColorSelectionDialog;
typedef struct _CtkColorSelectionDialogPrivate       CtkColorSelectionDialogPrivate;
typedef struct _CtkColorSelectionDialogClass         CtkColorSelectionDialogClass;


struct _CtkColorSelectionDialog
{
  CtkDialog parent_instance;

  /*< private >*/
  CtkColorSelectionDialogPrivate *priv;
};

struct _CtkColorSelectionDialogClass
{
  CtkDialogClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


/* ColorSelectionDialog */
GDK_DEPRECATED_IN_3_4
GType      ctk_color_selection_dialog_get_type            (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_3_4_FOR(ctk_color_chooser_dialog_new)
CtkWidget* ctk_color_selection_dialog_new                 (const gchar *title);
GDK_DEPRECATED_IN_3_4_FOR(CtkColorChooser)
CtkWidget* ctk_color_selection_dialog_get_color_selection (CtkColorSelectionDialog *colorsel);


G_END_DECLS

#endif /* __CTK_COLOR_SELECTION_DIALOG_H__ */
