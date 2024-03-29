/* CTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat, Inc.
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

/*
 * Modified by the CTK+ Team and others 1997-2003.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_MESSAGE_DIALOG_H__
#define __CTK_MESSAGE_DIALOG_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkdialog.h>
#include <ctk/ctkenums.h>

G_BEGIN_DECLS


#define CTK_TYPE_MESSAGE_DIALOG                  (ctk_message_dialog_get_type ())
#define CTK_MESSAGE_DIALOG(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_MESSAGE_DIALOG, CtkMessageDialog))
#define CTK_MESSAGE_DIALOG_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_MESSAGE_DIALOG, CtkMessageDialogClass))
#define CTK_IS_MESSAGE_DIALOG(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_MESSAGE_DIALOG))
#define CTK_IS_MESSAGE_DIALOG_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_MESSAGE_DIALOG))
#define CTK_MESSAGE_DIALOG_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_MESSAGE_DIALOG, CtkMessageDialogClass))

typedef struct _CtkMessageDialog              CtkMessageDialog;
typedef struct _CtkMessageDialogPrivate       CtkMessageDialogPrivate;
typedef struct _CtkMessageDialogClass         CtkMessageDialogClass;

struct _CtkMessageDialog
{
  CtkDialog parent_instance;

  /*< private >*/
  CtkMessageDialogPrivate *priv;
};

struct _CtkMessageDialogClass
{
  CtkDialogClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

/**
 * CtkButtonsType:
 * @CTK_BUTTONS_NONE: no buttons at all
 * @CTK_BUTTONS_OK: an OK button
 * @CTK_BUTTONS_CLOSE: a Close button
 * @CTK_BUTTONS_CANCEL: a Cancel button
 * @CTK_BUTTONS_YES_NO: Yes and No buttons
 * @CTK_BUTTONS_OK_CANCEL: OK and Cancel buttons
 *
 * Prebuilt sets of buttons for the dialog. If
 * none of these choices are appropriate, simply use %CTK_BUTTONS_NONE
 * then call ctk_dialog_add_buttons().
 *
 * > Please note that %CTK_BUTTONS_OK, %CTK_BUTTONS_YES_NO
 * > and %CTK_BUTTONS_OK_CANCEL are discouraged by the
 * > [GNOME Human Interface Guidelines](http://library.gnome.org/devel/hig-book/stable/).
 */
typedef enum
{
  CTK_BUTTONS_NONE,
  CTK_BUTTONS_OK,
  CTK_BUTTONS_CLOSE,
  CTK_BUTTONS_CANCEL,
  CTK_BUTTONS_YES_NO,
  CTK_BUTTONS_OK_CANCEL
} CtkButtonsType;

CDK_AVAILABLE_IN_ALL
GType      ctk_message_dialog_get_type (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_ALL
CtkWidget* ctk_message_dialog_new      (CtkWindow      *parent,
                                        CtkDialogFlags  flags,
                                        CtkMessageType  type,
                                        CtkButtonsType  buttons,
                                        const gchar    *message_format,
                                        ...) G_GNUC_PRINTF (5, 6);

CDK_AVAILABLE_IN_ALL
CtkWidget* ctk_message_dialog_new_with_markup   (CtkWindow      *parent,
                                                 CtkDialogFlags  flags,
                                                 CtkMessageType  type,
                                                 CtkButtonsType  buttons,
                                                 const gchar    *message_format,
                                                 ...) G_GNUC_PRINTF (5, 6);

CDK_AVAILABLE_IN_ALL
void       ctk_message_dialog_set_image    (CtkMessageDialog *dialog,
					    CtkWidget        *image);

CDK_AVAILABLE_IN_ALL
CtkWidget * ctk_message_dialog_get_image   (CtkMessageDialog *dialog);

CDK_AVAILABLE_IN_ALL
void       ctk_message_dialog_set_markup  (CtkMessageDialog *message_dialog,
                                           const gchar      *str);

CDK_AVAILABLE_IN_ALL
void       ctk_message_dialog_format_secondary_text (CtkMessageDialog *message_dialog,
                                                     const gchar      *message_format,
                                                     ...) G_GNUC_PRINTF (2, 3);

CDK_AVAILABLE_IN_ALL
void       ctk_message_dialog_format_secondary_markup (CtkMessageDialog *message_dialog,
                                                       const gchar      *message_format,
                                                       ...) G_GNUC_PRINTF (2, 3);

CDK_AVAILABLE_IN_ALL
CtkWidget *ctk_message_dialog_get_message_area (CtkMessageDialog *message_dialog);

G_END_DECLS

#endif /* __CTK_MESSAGE_DIALOG_H__ */
