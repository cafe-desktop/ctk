/* CTK - The GIMP Toolkit
 * Copyright (C) 2010 Red Hat, Inc.
 * Author: Matthias Clasen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CTK_LOCK_BUTTON_H__
#define __CTK_LOCK_BUTTON_H__

#include <ctk/ctkbutton.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define CTK_TYPE_LOCK_BUTTON         (ctk_lock_button_get_type ())
#define CTK_LOCK_BUTTON(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_LOCK_BUTTON, CtkLockButton))
#define CTK_LOCK_BUTTON_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), CTK_LOCK_BUTTON,  CtkLockButtonClass))
#define CTK_IS_LOCK_BUTTON(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_LOCK_BUTTON))
#define CTK_IS_LOCK_BUTTON_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), CTK_TYPE_LOCK_BUTTON))
#define CTK_LOCK_BUTTON_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CTK_TYPE_LOCK_BUTTON, CtkLockButtonClass))

typedef struct _CtkLockButton        CtkLockButton;
typedef struct _CtkLockButtonClass   CtkLockButtonClass;
typedef struct _CtkLockButtonPrivate CtkLockButtonPrivate;

struct _CtkLockButton
{
  CtkButton parent;

  CtkLockButtonPrivate *priv;
};

/**
 * CtkLockButtonClass:
 * @parent_class: The parent class.
 */
struct _CtkLockButtonClass
{
  CtkButtonClass parent_class;

  /*< private >*/

  void (*reserved0) (void);
  void (*reserved1) (void);
  void (*reserved2) (void);
  void (*reserved3) (void);
  void (*reserved4) (void);
  void (*reserved5) (void);
  void (*reserved6) (void);
  void (*reserved7) (void);
};

GDK_AVAILABLE_IN_3_2
GType        ctk_lock_button_get_type       (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_3_2
CtkWidget   *ctk_lock_button_new            (GPermission   *permission);
GDK_AVAILABLE_IN_3_2
GPermission *ctk_lock_button_get_permission (CtkLockButton *button);
GDK_AVAILABLE_IN_3_2
void         ctk_lock_button_set_permission (CtkLockButton *button,
                                             GPermission   *permission);


G_END_DECLS

#endif  /* __CTK_LOCK_BUTTON_H__ */
