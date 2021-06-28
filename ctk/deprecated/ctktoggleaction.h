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

#ifndef __CTK_TOGGLE_ACTION_H__
#define __CTK_TOGGLE_ACTION_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/deprecated/ctkaction.h>

G_BEGIN_DECLS

#define CTK_TYPE_TOGGLE_ACTION            (ctk_toggle_action_get_type ())
#define CTK_TOGGLE_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_TOGGLE_ACTION, CtkToggleAction))
#define CTK_TOGGLE_ACTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_TOGGLE_ACTION, CtkToggleActionClass))
#define CTK_IS_TOGGLE_ACTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_TOGGLE_ACTION))
#define CTK_IS_TOGGLE_ACTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_TOGGLE_ACTION))
#define CTK_TOGGLE_ACTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), CTK_TYPE_TOGGLE_ACTION, CtkToggleActionClass))

typedef struct _CtkToggleAction        CtkToggleAction;
typedef struct _CtkToggleActionPrivate CtkToggleActionPrivate;
typedef struct _CtkToggleActionClass   CtkToggleActionClass;

struct _CtkToggleAction
{
  CtkAction parent;

  /*< private >*/
  CtkToggleActionPrivate *private_data;
};

struct _CtkToggleActionClass
{
  CtkActionClass parent_class;

  void (* toggled) (CtkToggleAction *action);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

CDK_DEPRECATED_IN_3_10
GType            ctk_toggle_action_get_type          (void) G_GNUC_CONST;
CDK_DEPRECATED_IN_3_10
CtkToggleAction *ctk_toggle_action_new               (const gchar     *name,
                                                      const gchar     *label,
                                                      const gchar     *tooltip,
                                                      const gchar     *stock_id);
CDK_DEPRECATED_IN_3_10
void             ctk_toggle_action_toggled           (CtkToggleAction *action);
CDK_DEPRECATED_IN_3_10
void             ctk_toggle_action_set_active        (CtkToggleAction *action,
                                                      gboolean         is_active);
CDK_DEPRECATED_IN_3_10
gboolean         ctk_toggle_action_get_active        (CtkToggleAction *action);
CDK_DEPRECATED_IN_3_10
void             ctk_toggle_action_set_draw_as_radio (CtkToggleAction *action,
                                                      gboolean         draw_as_radio);
CDK_DEPRECATED_IN_3_10
gboolean         ctk_toggle_action_get_draw_as_radio (CtkToggleAction *action);

/* private */
void             _ctk_toggle_action_set_active       (CtkToggleAction *toggle_action,
                                                      gboolean         is_active);


G_END_DECLS

#endif  /* __CTK_TOGGLE_ACTION_H__ */
