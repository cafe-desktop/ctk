/* GTK - The GIMP Toolkit
 * Copyright (C) 2011      Alberto Ruiz <aruiz@gnome.org>
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

#ifndef __CTK_FONT_CHOOSER_DIALOG_H__
#define __CTK_FONT_CHOOSER_DIALOG_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gtk/gtkdialog.h>

G_BEGIN_DECLS

#define CTK_TYPE_FONT_CHOOSER_DIALOG              (ctk_font_chooser_dialog_get_type ())
#define CTK_FONT_CHOOSER_DIALOG(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_FONT_CHOOSER_DIALOG, GtkFontChooserDialog))
#define CTK_FONT_CHOOSER_DIALOG_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_FONT_CHOOSER_DIALOG, GtkFontChooserDialogClass))
#define CTK_IS_FONT_CHOOSER_DIALOG(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_FONT_CHOOSER_DIALOG))
#define CTK_IS_FONT_CHOOSER_DIALOG_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_FONT_CHOOSER_DIALOG))
#define CTK_FONT_CHOOSER_DIALOG_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_FONT_CHOOSER_DIALOG, GtkFontChooserDialogClass))

typedef struct _GtkFontChooserDialog              GtkFontChooserDialog;
typedef struct _GtkFontChooserDialogPrivate       GtkFontChooserDialogPrivate;
typedef struct _GtkFontChooserDialogClass         GtkFontChooserDialogClass;

struct _GtkFontChooserDialog
{
  GtkDialog parent_instance;

  /*< private >*/
  GtkFontChooserDialogPrivate *priv;
};

/**
 * GtkFontChooserDialogClass:
 * @parent_class: The parent class.
 */
struct _GtkFontChooserDialogClass
{
  GtkDialogClass parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_3_2
GType      ctk_font_chooser_dialog_get_type         (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_3_2
GtkWidget* ctk_font_chooser_dialog_new              (const gchar          *title,
                                                     GtkWindow            *parent);

G_END_DECLS

#endif /* __CTK_FONT_CHOOSER_DIALOG_H__ */
