/*
 * CTK - The GIMP Toolkit
 * Copyright (C) 1998, 1999 Red Hat, Inc.
 * All rights reserved.
 *
 * This Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Author: James Henstridge <james@daa.com.au>
 *
 * Modified by the CTK+ Team and others 2003.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_RADIO_ACTION_H__
#define __CTK_RADIO_ACTION_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctktoggleaction.h>

G_BEGIN_DECLS

#define CTK_TYPE_RADIO_ACTION            (ctk_radio_action_get_type ())
#define CTK_RADIO_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_RADIO_ACTION, CtkRadioAction))
#define CTK_RADIO_ACTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_RADIO_ACTION, CtkRadioActionClass))
#define CTK_IS_RADIO_ACTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_RADIO_ACTION))
#define CTK_IS_RADIO_ACTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_RADIO_ACTION))
#define CTK_RADIO_ACTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), CTK_TYPE_RADIO_ACTION, CtkRadioActionClass))

typedef struct _CtkRadioAction        CtkRadioAction;
typedef struct _CtkRadioActionPrivate CtkRadioActionPrivate;
typedef struct _CtkRadioActionClass   CtkRadioActionClass;

struct _CtkRadioAction
{
  CtkToggleAction parent;

  /*< private >*/
  CtkRadioActionPrivate *private_data;
};

struct _CtkRadioActionClass
{
  CtkToggleActionClass parent_class;

  void       (* changed) (CtkRadioAction *action, CtkRadioAction *current);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

CDK_AVAILABLE_IN_ALL
GType           ctk_radio_action_get_type          (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkRadioAction *ctk_radio_action_new               (const gchar           *name,
                                                    const gchar           *label,
                                                    const gchar           *tooltip,
                                                    const gchar           *stock_id,
                                                    gint                   value);
CDK_AVAILABLE_IN_ALL
GSList         *ctk_radio_action_get_group         (CtkRadioAction        *action);
CDK_AVAILABLE_IN_ALL
void            ctk_radio_action_set_group         (CtkRadioAction        *action,
                                                    GSList                *group);
CDK_AVAILABLE_IN_ALL
void            ctk_radio_action_join_group        (CtkRadioAction        *action,
                                                    CtkRadioAction        *group_source);
CDK_AVAILABLE_IN_ALL
gint            ctk_radio_action_get_current_value (CtkRadioAction        *action);
CDK_AVAILABLE_IN_ALL
void            ctk_radio_action_set_current_value (CtkRadioAction        *action,
                                                    gint                   current_value);

G_END_DECLS

#endif  /* __CTK_RADIO_ACTION_H__ */
