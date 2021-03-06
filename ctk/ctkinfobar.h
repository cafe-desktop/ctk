/*
 * ctkinfobar.h
 * This file is part of CTK+
 *
 * Copyright (C) 2005 - Paolo Maggi
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
 * Modified by the gedit Team, 2005. See the gedit AUTHORS file for a
 * list of people on the gedit Team.
 * See the gedit ChangeLog files for a list of changes.
 *
 * Modified by the CTK+ Team, 2008-2009.
 */

#ifndef __CTK_INFO_BAR_H__
#define __CTK_INFO_BAR_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkbox.h>
#include <ctk/ctkenums.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define CTK_TYPE_INFO_BAR              (ctk_info_bar_get_type())
#define CTK_INFO_BAR(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), CTK_TYPE_INFO_BAR, CtkInfoBar))
#define CTK_INFO_BAR_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), CTK_TYPE_INFO_BAR, CtkInfoBarClass))
#define CTK_IS_INFO_BAR(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), CTK_TYPE_INFO_BAR))
#define CTK_IS_INFO_BAR_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_INFO_BAR))
#define CTK_INFO_BAR_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), CTK_TYPE_INFO_BAR, CtkInfoBarClass))


typedef struct _CtkInfoBarPrivate CtkInfoBarPrivate;
typedef struct _CtkInfoBarClass CtkInfoBarClass;
typedef struct _CtkInfoBar CtkInfoBar;


struct _CtkInfoBar
{
  CtkBox parent;

  /*< private > */
  CtkInfoBarPrivate *priv;
};


struct _CtkInfoBarClass
{
  CtkBoxClass parent_class;

  /* Signals */
  void (* response) (CtkInfoBar *info_bar, gint response_id);

  /* Keybinding signals */
  void (* close)    (CtkInfoBar *info_bar);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

CDK_AVAILABLE_IN_ALL
GType          ctk_info_bar_get_type               (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkWidget     *ctk_info_bar_new                    (void);

CDK_AVAILABLE_IN_ALL
CtkWidget     *ctk_info_bar_new_with_buttons       (const gchar    *first_button_text,
                                                    ...);

CDK_AVAILABLE_IN_ALL
CtkWidget     *ctk_info_bar_get_action_area        (CtkInfoBar     *info_bar);
CDK_AVAILABLE_IN_ALL
CtkWidget     *ctk_info_bar_get_content_area       (CtkInfoBar     *info_bar);
CDK_AVAILABLE_IN_ALL
void           ctk_info_bar_add_action_widget      (CtkInfoBar     *info_bar,
                                                    CtkWidget      *child,
                                                    gint            response_id);
CDK_AVAILABLE_IN_ALL
CtkWidget     *ctk_info_bar_add_button             (CtkInfoBar     *info_bar,
                                                    const gchar    *button_text,
                                                    gint            response_id);
CDK_AVAILABLE_IN_ALL
void           ctk_info_bar_add_buttons            (CtkInfoBar     *info_bar,
                                                    const gchar    *first_button_text,
                                                    ...);
CDK_AVAILABLE_IN_ALL
void           ctk_info_bar_set_response_sensitive (CtkInfoBar     *info_bar,
                                                    gint            response_id,
                                                    gboolean        setting);
CDK_AVAILABLE_IN_ALL
void           ctk_info_bar_set_default_response   (CtkInfoBar     *info_bar,
                                                    gint            response_id);

/* Emit response signal */
CDK_AVAILABLE_IN_ALL
void           ctk_info_bar_response               (CtkInfoBar     *info_bar,
                                                    gint            response_id);

CDK_AVAILABLE_IN_ALL
void           ctk_info_bar_set_message_type       (CtkInfoBar     *info_bar,
                                                    CtkMessageType  message_type);
CDK_AVAILABLE_IN_ALL
CtkMessageType ctk_info_bar_get_message_type       (CtkInfoBar     *info_bar);

CDK_AVAILABLE_IN_3_10
void           ctk_info_bar_set_show_close_button  (CtkInfoBar     *info_bar,
                                                    gboolean        setting);
CDK_AVAILABLE_IN_3_10
gboolean       ctk_info_bar_get_show_close_button  (CtkInfoBar     *info_bar);

CDK_AVAILABLE_IN_3_22
void           ctk_info_bar_set_revealed           (CtkInfoBar     *info_bar,
                                                    gboolean        revealed);
CDK_AVAILABLE_IN_3_22
gboolean       ctk_info_bar_get_revealed           (CtkInfoBar     *info_bar);

G_END_DECLS

#endif  /* __CTK_INFO_BAR_H__  */
