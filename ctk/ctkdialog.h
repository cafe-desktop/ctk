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
 * GTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_DIALOG_H__
#define __CTK_DIALOG_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkwindow.h>

G_BEGIN_DECLS

/**
 * CtkDialogFlags:
 * @CTK_DIALOG_MODAL: Make the constructed dialog modal,
 *     see ctk_window_set_modal()
 * @CTK_DIALOG_DESTROY_WITH_PARENT: Destroy the dialog when its
 *     parent is destroyed, see ctk_window_set_destroy_with_parent()
 * @CTK_DIALOG_USE_HEADER_BAR: Create dialog with actions in header
 *     bar instead of action area. Since 3.12.
 *
 * Flags used to influence dialog construction.
 */
typedef enum
{
  CTK_DIALOG_MODAL               = 1 << 0,
  CTK_DIALOG_DESTROY_WITH_PARENT = 1 << 1,
  CTK_DIALOG_USE_HEADER_BAR      = 1 << 2
} CtkDialogFlags;

/**
 * CtkResponseType:
 * @CTK_RESPONSE_NONE: Returned if an action widget has no response id,
 *     or if the dialog gets programmatically hidden or destroyed
 * @CTK_RESPONSE_REJECT: Generic response id, not used by GTK+ dialogs
 * @CTK_RESPONSE_ACCEPT: Generic response id, not used by GTK+ dialogs
 * @CTK_RESPONSE_DELETE_EVENT: Returned if the dialog is deleted
 * @CTK_RESPONSE_OK: Returned by OK buttons in GTK+ dialogs
 * @CTK_RESPONSE_CANCEL: Returned by Cancel buttons in GTK+ dialogs
 * @CTK_RESPONSE_CLOSE: Returned by Close buttons in GTK+ dialogs
 * @CTK_RESPONSE_YES: Returned by Yes buttons in GTK+ dialogs
 * @CTK_RESPONSE_NO: Returned by No buttons in GTK+ dialogs
 * @CTK_RESPONSE_APPLY: Returned by Apply buttons in GTK+ dialogs
 * @CTK_RESPONSE_HELP: Returned by Help buttons in GTK+ dialogs
 *
 * Predefined values for use as response ids in ctk_dialog_add_button().
 * All predefined values are negative; GTK+ leaves values of 0 or greater for
 * application-defined response ids.
 */
typedef enum
{
  CTK_RESPONSE_NONE         = -1,
  CTK_RESPONSE_REJECT       = -2,
  CTK_RESPONSE_ACCEPT       = -3,
  CTK_RESPONSE_DELETE_EVENT = -4,
  CTK_RESPONSE_OK           = -5,
  CTK_RESPONSE_CANCEL       = -6,
  CTK_RESPONSE_CLOSE        = -7,
  CTK_RESPONSE_YES          = -8,
  CTK_RESPONSE_NO           = -9,
  CTK_RESPONSE_APPLY        = -10,
  CTK_RESPONSE_HELP         = -11
} CtkResponseType;


#define CTK_TYPE_DIALOG                  (ctk_dialog_get_type ())
#define CTK_DIALOG(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_DIALOG, CtkDialog))
#define CTK_DIALOG_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_DIALOG, CtkDialogClass))
#define CTK_IS_DIALOG(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_DIALOG))
#define CTK_IS_DIALOG_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_DIALOG))
#define CTK_DIALOG_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_DIALOG, CtkDialogClass))


typedef struct _CtkDialog              CtkDialog;
typedef struct _CtkDialogPrivate       CtkDialogPrivate;
typedef struct _CtkDialogClass         CtkDialogClass;

/**
 * CtkDialog:
 *
 * The #CtkDialog-struct contains only private fields
 * and should not be directly accessed.
 */
struct _CtkDialog
{
  CtkWindow window;

  /*< private >*/
  CtkDialogPrivate *priv;
};

/**
 * CtkDialogClass:
 * @parent_class: The parent class.
 * @response: Signal emitted when an action widget is activated.
 * @close: Signal emitted when the user uses a keybinding to close the dialog.
 */
struct _CtkDialogClass
{
  CtkWindowClass parent_class;

  /*< public >*/

  void (* response) (CtkDialog *dialog, gint response_id);

  /* Keybinding signals */

  void (* close)    (CtkDialog *dialog);

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


GDK_AVAILABLE_IN_ALL
GType      ctk_dialog_get_type (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
CtkWidget* ctk_dialog_new      (void);

GDK_AVAILABLE_IN_ALL
CtkWidget* ctk_dialog_new_with_buttons (const gchar     *title,
                                        CtkWindow       *parent,
                                        CtkDialogFlags   flags,
                                        const gchar     *first_button_text,
                                        ...) G_GNUC_NULL_TERMINATED;

GDK_AVAILABLE_IN_ALL
void       ctk_dialog_add_action_widget (CtkDialog   *dialog,
                                         CtkWidget   *child,
                                         gint         response_id);
GDK_AVAILABLE_IN_ALL
CtkWidget* ctk_dialog_add_button        (CtkDialog   *dialog,
                                         const gchar *button_text,
                                         gint         response_id);
GDK_AVAILABLE_IN_ALL
void       ctk_dialog_add_buttons       (CtkDialog   *dialog,
                                         const gchar *first_button_text,
                                         ...) G_GNUC_NULL_TERMINATED;

GDK_AVAILABLE_IN_ALL
void ctk_dialog_set_response_sensitive (CtkDialog *dialog,
                                        gint       response_id,
                                        gboolean   setting);
GDK_AVAILABLE_IN_ALL
void ctk_dialog_set_default_response   (CtkDialog *dialog,
                                        gint       response_id);
GDK_AVAILABLE_IN_ALL
CtkWidget* ctk_dialog_get_widget_for_response (CtkDialog *dialog,
                                               gint       response_id);
GDK_AVAILABLE_IN_ALL
gint ctk_dialog_get_response_for_widget (CtkDialog *dialog,
                                         CtkWidget *widget);

GDK_DEPRECATED_IN_3_10
gboolean ctk_alternative_dialog_button_order (GdkScreen *screen);
GDK_DEPRECATED_IN_3_10
void     ctk_dialog_set_alternative_button_order (CtkDialog *dialog,
                                                  gint       first_response_id,
                                                  ...);
GDK_DEPRECATED_IN_3_10
void     ctk_dialog_set_alternative_button_order_from_array (CtkDialog *dialog,
                                                             gint       n_params,
                                                             gint      *new_order);

/* Emit response signal */
GDK_AVAILABLE_IN_ALL
void ctk_dialog_response           (CtkDialog *dialog,
                                    gint       response_id);

/* Returns response_id */
GDK_AVAILABLE_IN_ALL
gint ctk_dialog_run                (CtkDialog *dialog);

GDK_DEPRECATED_IN_3_10
CtkWidget * ctk_dialog_get_action_area  (CtkDialog *dialog);
GDK_AVAILABLE_IN_ALL
CtkWidget * ctk_dialog_get_content_area (CtkDialog *dialog);
GDK_AVAILABLE_IN_3_12
CtkWidget * ctk_dialog_get_header_bar   (CtkDialog *dialog);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(CtkDialog, g_object_unref)

G_END_DECLS

#endif /* __CTK_DIALOG_H__ */
