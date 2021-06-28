/*
 * ctkappchooserbutton.h: an app-chooser combobox
 *
 * Copyright (C) 2010 Red Hat, Inc.
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
 * Authors: Cosimo Cecchi <ccecchi@redhat.com>
 */

#ifndef __CTK_APP_CHOOSER_BUTTON_H__
#define __CTK_APP_CHOOSER_BUTTON_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkcombobox.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define CTK_TYPE_APP_CHOOSER_BUTTON            (ctk_app_chooser_button_get_type ())
#define CTK_APP_CHOOSER_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_APP_CHOOSER_BUTTON, CtkAppChooserButton))
#define CTK_APP_CHOOSER_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_APP_CHOOSER_BUTTON, CtkAppChooserButtonClass))
#define CTK_IS_APP_CHOOSER_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_APP_CHOOSER_BUTTON))
#define CTK_IS_APP_CHOOSER_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_APP_CHOOSER_BUTTON))
#define CTK_APP_CHOOSER_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_APP_CHOOSER_BUTTON, CtkAppChooserButtonClass))

typedef struct _CtkAppChooserButton        CtkAppChooserButton;
typedef struct _CtkAppChooserButtonClass   CtkAppChooserButtonClass;
typedef struct _CtkAppChooserButtonPrivate CtkAppChooserButtonPrivate;

struct _CtkAppChooserButton {
  CtkComboBox parent;

  /*< private >*/
  CtkAppChooserButtonPrivate *priv;
};

/**
 * CtkAppChooserButtonClass:
 * @parent_class: The parent class.
 * @custom_item_activated: Signal emitted when a custom item,
 *    previously added with ctk_app_chooser_button_append_custom_item(),
 *    is activated from the dropdown menu.
 */
struct _CtkAppChooserButtonClass {
  CtkComboBoxClass parent_class;

  /*< public >*/

  void (* custom_item_activated) (CtkAppChooserButton *self,
                                  const gchar *item_name);

  /*< private >*/

  /* padding for future class expansion */
  gpointer padding[16];
};

CDK_AVAILABLE_IN_ALL
GType       ctk_app_chooser_button_get_type           (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_ALL
CtkWidget * ctk_app_chooser_button_new                (const gchar         *content_type);

CDK_AVAILABLE_IN_ALL
void        ctk_app_chooser_button_append_separator   (CtkAppChooserButton *self);
CDK_AVAILABLE_IN_ALL
void        ctk_app_chooser_button_append_custom_item (CtkAppChooserButton *self,
                                                       const gchar         *name,
                                                       const gchar         *label,
                                                       GIcon               *icon);
CDK_AVAILABLE_IN_ALL
void     ctk_app_chooser_button_set_active_custom_item (CtkAppChooserButton *self,
                                                        const gchar         *name);

CDK_AVAILABLE_IN_ALL
void     ctk_app_chooser_button_set_show_dialog_item  (CtkAppChooserButton *self,
                                                       gboolean             setting);
CDK_AVAILABLE_IN_ALL
gboolean ctk_app_chooser_button_get_show_dialog_item  (CtkAppChooserButton *self);
CDK_AVAILABLE_IN_ALL
void     ctk_app_chooser_button_set_heading           (CtkAppChooserButton *self,
                                                       const gchar         *heading);
CDK_AVAILABLE_IN_ALL
const gchar *
         ctk_app_chooser_button_get_heading           (CtkAppChooserButton *self);
CDK_AVAILABLE_IN_3_2
void     ctk_app_chooser_button_set_show_default_item (CtkAppChooserButton *self,
                                                       gboolean             setting);
CDK_AVAILABLE_IN_3_2
gboolean ctk_app_chooser_button_get_show_default_item (CtkAppChooserButton *self);

G_END_DECLS

#endif /* __CTK_APP_CHOOSER_BUTTON_H__ */
