/* GTK - The GIMP Toolkit
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
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __CTK_COLOR_SELECTION_DIALOG_H__
#define __CTK_COLOR_SELECTION_DIALOG_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gtk/gtkdialog.h>
#include <gtk/deprecated/gtkcolorsel.h>

G_BEGIN_DECLS

#define CTK_TYPE_COLOR_SELECTION_DIALOG            (ctk_color_selection_dialog_get_type ())
#define CTK_COLOR_SELECTION_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_COLOR_SELECTION_DIALOG, GtkColorSelectionDialog))
#define CTK_COLOR_SELECTION_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_COLOR_SELECTION_DIALOG, GtkColorSelectionDialogClass))
#define CTK_IS_COLOR_SELECTION_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_COLOR_SELECTION_DIALOG))
#define CTK_IS_COLOR_SELECTION_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_COLOR_SELECTION_DIALOG))
#define CTK_COLOR_SELECTION_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_COLOR_SELECTION_DIALOG, GtkColorSelectionDialogClass))


typedef struct _GtkColorSelectionDialog              GtkColorSelectionDialog;
typedef struct _GtkColorSelectionDialogPrivate       GtkColorSelectionDialogPrivate;
typedef struct _GtkColorSelectionDialogClass         GtkColorSelectionDialogClass;


struct _GtkColorSelectionDialog
{
  GtkDialog parent_instance;

  /*< private >*/
  GtkColorSelectionDialogPrivate *priv;
};

struct _GtkColorSelectionDialogClass
{
  GtkDialogClass parent_class;

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
GtkWidget* ctk_color_selection_dialog_new                 (const gchar *title);
GDK_DEPRECATED_IN_3_4_FOR(GtkColorChooser)
GtkWidget* ctk_color_selection_dialog_get_color_selection (GtkColorSelectionDialog *colorsel);


G_END_DECLS

#endif /* __CTK_COLOR_SELECTION_DIALOG_H__ */
