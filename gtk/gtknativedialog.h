/* GTK - The GIMP Toolkit
 * gtknativedialog.h: Native dialog
 * Copyright (C) 2015, Red Hat, Inc.
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

#ifndef __CTK_NATIVE_DIALOG_H__
#define __CTK_NATIVE_DIALOG_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gtk/gtkwindow.h>

G_BEGIN_DECLS

#define CTK_TYPE_NATIVE_DIALOG             (ctk_native_dialog_get_type ())

GDK_AVAILABLE_IN_3_20
G_DECLARE_DERIVABLE_TYPE (GtkNativeDialog, ctk_native_dialog, GTK, NATIVE_DIALOG, GObject)

struct _GtkNativeDialogClass
{
  GObjectClass parent_class;

  void (* response) (GtkNativeDialog *self, gint response_id);

  /* <private> */
  void (* show) (GtkNativeDialog *self);
  void (* hide) (GtkNativeDialog *self);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_3_20
void                  ctk_native_dialog_show (GtkNativeDialog *self);
GDK_AVAILABLE_IN_3_20
void                  ctk_native_dialog_hide (GtkNativeDialog *self);
GDK_AVAILABLE_IN_3_20
void                  ctk_native_dialog_destroy (GtkNativeDialog *self);
GDK_AVAILABLE_IN_3_20
gboolean              ctk_native_dialog_get_visible (GtkNativeDialog *self);
GDK_AVAILABLE_IN_3_20
void                  ctk_native_dialog_set_modal (GtkNativeDialog *self,
                                                   gboolean modal);
GDK_AVAILABLE_IN_3_20
gboolean              ctk_native_dialog_get_modal (GtkNativeDialog *self);
GDK_AVAILABLE_IN_3_20
void                  ctk_native_dialog_set_title (GtkNativeDialog *self,
                                                   const char *title);
GDK_AVAILABLE_IN_3_20
const char *          ctk_native_dialog_get_title (GtkNativeDialog *self);
GDK_AVAILABLE_IN_3_20
void                  ctk_native_dialog_set_transient_for (GtkNativeDialog *self,
                                                           GtkWindow *parent);
GDK_AVAILABLE_IN_3_20
GtkWindow *           ctk_native_dialog_get_transient_for (GtkNativeDialog *self);

GDK_AVAILABLE_IN_3_20
gint                  ctk_native_dialog_run (GtkNativeDialog *self);

G_END_DECLS

#endif /* __CTK_NATIVE_DIALOG_H__ */
