/* CTK+: ctkfilechooserbutton.h
 *
 * Copyright (c) 2004 James M. Cape <jcape@ignore-your.tv>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CTK_FILE_CHOOSER_BUTTON_H__
#define __CTK_FILE_CHOOSER_BUTTON_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkbox.h>
#include <ctk/ctkfilechooser.h>

G_BEGIN_DECLS

#define CTK_TYPE_FILE_CHOOSER_BUTTON            (ctk_file_chooser_button_get_type ())
#define CTK_FILE_CHOOSER_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_FILE_CHOOSER_BUTTON, CtkFileChooserButton))
#define CTK_FILE_CHOOSER_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_FILE_CHOOSER_BUTTON, CtkFileChooserButtonClass))
#define CTK_IS_FILE_CHOOSER_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_FILE_CHOOSER_BUTTON))
#define CTK_IS_FILE_CHOOSER_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_FILE_CHOOSER_BUTTON))
#define CTK_FILE_CHOOSER_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_FILE_CHOOSER_BUTTON, CtkFileChooserButtonClass))

typedef struct _CtkFileChooserButton        CtkFileChooserButton;
typedef struct _CtkFileChooserButtonPrivate CtkFileChooserButtonPrivate;
typedef struct _CtkFileChooserButtonClass   CtkFileChooserButtonClass;

struct _CtkFileChooserButton
{
  CtkBox parent;

  /*< private >*/
  CtkFileChooserButtonPrivate *priv;
};

/**
 * CtkFileChooserButtonClass:
 * @parent_class: The parent class.
 * @file_set: Signal emitted when the user selects a file.
 */
struct _CtkFileChooserButtonClass
{
  CtkBoxClass parent_class;

  /*< public >*/

  void (* file_set) (CtkFileChooserButton *fc);

  /*< private >*/

  /* Padding for future expansion */
  void (*__ctk_reserved1) (void);
  void (*__ctk_reserved2) (void);
  void (*__ctk_reserved3) (void);
  void (*__ctk_reserved4) (void);
};


GDK_AVAILABLE_IN_ALL
GType                 ctk_file_chooser_button_get_type         (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
CtkWidget *           ctk_file_chooser_button_new              (const gchar          *title,
								CtkFileChooserAction  action);
GDK_AVAILABLE_IN_ALL
CtkWidget *           ctk_file_chooser_button_new_with_dialog  (CtkWidget            *dialog);
GDK_AVAILABLE_IN_ALL
const gchar *         ctk_file_chooser_button_get_title        (CtkFileChooserButton *button);
GDK_AVAILABLE_IN_ALL
void                  ctk_file_chooser_button_set_title        (CtkFileChooserButton *button,
								const gchar          *title);
GDK_AVAILABLE_IN_ALL
gint                  ctk_file_chooser_button_get_width_chars  (CtkFileChooserButton *button);
GDK_AVAILABLE_IN_ALL
void                  ctk_file_chooser_button_set_width_chars  (CtkFileChooserButton *button,
								gint                  n_chars);
GDK_DEPRECATED_IN_3_20_FOR(ctk_widget_get_focus_on_click)
gboolean              ctk_file_chooser_button_get_focus_on_click (CtkFileChooserButton *button);
GDK_DEPRECATED_IN_3_20_FOR(ctk_widget_set_focus_on_click)
void                  ctk_file_chooser_button_set_focus_on_click (CtkFileChooserButton *button,
                                                                  gboolean              focus_on_click);

G_END_DECLS

#endif /* !__CTK_FILE_CHOOSER_BUTTON_H__ */
