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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

#ifndef __CTK_RADIO_BUTTON_H__
#define __CTK_RADIO_BUTTON_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkcheckbutton.h>


G_BEGIN_DECLS

#define CTK_TYPE_RADIO_BUTTON		       (ctk_radio_button_get_type ())
#define CTK_RADIO_BUTTON(obj)		       (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_RADIO_BUTTON, CtkRadioButton))
#define CTK_RADIO_BUTTON_CLASS(klass)	       (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_RADIO_BUTTON, CtkRadioButtonClass))
#define CTK_IS_RADIO_BUTTON(obj)	       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_RADIO_BUTTON))
#define CTK_IS_RADIO_BUTTON_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_RADIO_BUTTON))
#define CTK_RADIO_BUTTON_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_RADIO_BUTTON, CtkRadioButtonClass))


typedef struct _CtkRadioButton              CtkRadioButton;
typedef struct _CtkRadioButtonPrivate       CtkRadioButtonPrivate;
typedef struct _CtkRadioButtonClass         CtkRadioButtonClass;

struct _CtkRadioButton
{
  CtkCheckButton check_button;

  /*< private >*/
  CtkRadioButtonPrivate *priv;
};

struct _CtkRadioButtonClass
{
  CtkCheckButtonClass parent_class;

  /* Signals */
  void (*group_changed) (CtkRadioButton *radio_button);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


CDK_AVAILABLE_IN_ALL
GType	   ctk_radio_button_get_type	     (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_ALL
CtkWidget* ctk_radio_button_new                           (GSList         *group);
CDK_AVAILABLE_IN_ALL
CtkWidget* ctk_radio_button_new_from_widget               (CtkRadioButton *radio_group_member);
CDK_AVAILABLE_IN_ALL
CtkWidget* ctk_radio_button_new_with_label                (GSList         *group,
                                                           const gchar    *label);
CDK_AVAILABLE_IN_ALL
CtkWidget* ctk_radio_button_new_with_label_from_widget    (CtkRadioButton *radio_group_member,
                                                           const gchar    *label);
CDK_AVAILABLE_IN_ALL
CtkWidget* ctk_radio_button_new_with_mnemonic             (GSList         *group,
                                                           const gchar    *label);
CDK_AVAILABLE_IN_ALL
CtkWidget* ctk_radio_button_new_with_mnemonic_from_widget (CtkRadioButton *radio_group_member,
                                                           const gchar    *label);
CDK_AVAILABLE_IN_ALL
GSList*    ctk_radio_button_get_group                     (CtkRadioButton *radio_button);
CDK_AVAILABLE_IN_ALL
void       ctk_radio_button_set_group                     (CtkRadioButton *radio_button,
                                                           GSList         *group);
CDK_AVAILABLE_IN_ALL
void            ctk_radio_button_join_group        (CtkRadioButton        *radio_button,
                                                    CtkRadioButton        *group_source);
G_END_DECLS

#endif /* __CTK_RADIO_BUTTON_H__ */
