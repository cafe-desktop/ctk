/*
 * GTK - The GIMP Toolkit
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
 * Modified by the GTK+ Team and others 2003.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_RADIO_ACTION_H__
#define __CTK_RADIO_ACTION_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/deprecated/ctktoggleaction.h>

G_BEGIN_DECLS

#define CTK_TYPE_RADIO_ACTION            (ctk_radio_action_get_type ())
#define CTK_RADIO_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_RADIO_ACTION, GtkRadioAction))
#define CTK_RADIO_ACTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_RADIO_ACTION, GtkRadioActionClass))
#define CTK_IS_RADIO_ACTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_RADIO_ACTION))
#define CTK_IS_RADIO_ACTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_RADIO_ACTION))
#define CTK_RADIO_ACTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), CTK_TYPE_RADIO_ACTION, GtkRadioActionClass))

typedef struct _GtkRadioAction        GtkRadioAction;
typedef struct _GtkRadioActionPrivate GtkRadioActionPrivate;
typedef struct _GtkRadioActionClass   GtkRadioActionClass;

struct _GtkRadioAction
{
  GtkToggleAction parent;

  /*< private >*/
  GtkRadioActionPrivate *private_data;
};

struct _GtkRadioActionClass
{
  GtkToggleActionClass parent_class;

  void       (* changed) (GtkRadioAction *action, GtkRadioAction *current);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_DEPRECATED_IN_3_10
GType           ctk_radio_action_get_type          (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_3_10
GtkRadioAction *ctk_radio_action_new               (const gchar           *name,
                                                    const gchar           *label,
                                                    const gchar           *tooltip,
                                                    const gchar           *stock_id,
                                                    gint                   value);
GDK_DEPRECATED_IN_3_10
GSList         *ctk_radio_action_get_group         (GtkRadioAction        *action);
GDK_DEPRECATED_IN_3_10
void            ctk_radio_action_set_group         (GtkRadioAction        *action,
                                                    GSList                *group);
GDK_DEPRECATED_IN_3_10
void            ctk_radio_action_join_group        (GtkRadioAction        *action,
                                                    GtkRadioAction        *group_source);
GDK_DEPRECATED_IN_3_10
gint            ctk_radio_action_get_current_value (GtkRadioAction        *action);
GDK_DEPRECATED_IN_3_10
void            ctk_radio_action_set_current_value (GtkRadioAction        *action,
                                                    gint                   current_value);

G_END_DECLS

#endif  /* __CTK_RADIO_ACTION_H__ */
