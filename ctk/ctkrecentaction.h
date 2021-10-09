/* CTK - The GIMP Toolkit
 * Recent chooser action for CtkUIManager
 *
 * Copyright (C) 2007, Emmanuele Bassi
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

#ifndef __CTK_RECENT_ACTION_H__
#define __CTK_RECENT_ACTION_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkaction.h>
#include <ctk/ctkrecentmanager.h>

G_BEGIN_DECLS

#define CTK_TYPE_RECENT_ACTION                  (ctk_recent_action_get_type ())
#define CTK_RECENT_ACTION(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_RECENT_ACTION, CtkRecentAction))
#define CTK_IS_RECENT_ACTION(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_RECENT_ACTION))
#define CTK_RECENT_ACTION_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_RECENT_ACTION, CtkRecentActionClass))
#define CTK_IS_RECENT_ACTION_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_RECENT_ACTION))
#define CTK_RECENT_ACTION_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_RECENT_ACTION, CtkRecentActionClass))

typedef struct _CtkRecentAction         CtkRecentAction;
typedef struct _CtkRecentActionPrivate  CtkRecentActionPrivate;
typedef struct _CtkRecentActionClass    CtkRecentActionClass;

struct _CtkRecentAction
{
  CtkAction parent_instance;

  /*< private >*/
  CtkRecentActionPrivate *priv;
};

struct _CtkRecentActionClass
{
  CtkActionClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

CDK_AVAILABLE_IN_ALL
GType      ctk_recent_action_get_type         (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkAction *ctk_recent_action_new              (const gchar      *name,
                                               const gchar      *label,
                                               const gchar      *tooltip,
                                               const gchar      *stock_id);
CDK_AVAILABLE_IN_ALL
CtkAction *ctk_recent_action_new_for_manager  (const gchar      *name,
                                               const gchar      *label,
                                               const gchar      *tooltip,
                                               const gchar      *stock_id,
                                               CtkRecentManager *manager);
CDK_AVAILABLE_IN_ALL
gboolean   ctk_recent_action_get_show_numbers (CtkRecentAction  *action);
CDK_AVAILABLE_IN_ALL
void       ctk_recent_action_set_show_numbers (CtkRecentAction  *action,
                                               gboolean          show_numbers);

G_END_DECLS

#endif /* __CTK_RECENT_ACTION_H__ */
