/* GTK - The GIMP Toolkit
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

#include <gtk/gtkbutton.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define CTK_TYPE_LOCK_BUTTON         (ctk_lock_button_get_type ())
#define CTK_LOCK_BUTTON(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_LOCK_BUTTON, GtkLockButton))
#define CTK_LOCK_BUTTON_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), CTK_LOCK_BUTTON,  GtkLockButtonClass))
#define CTK_IS_LOCK_BUTTON(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_LOCK_BUTTON))
#define CTK_IS_LOCK_BUTTON_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), CTK_TYPE_LOCK_BUTTON))
#define CTK_LOCK_BUTTON_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CTK_TYPE_LOCK_BUTTON, GtkLockButtonClass))

typedef struct _GtkLockButton        GtkLockButton;
typedef struct _GtkLockButtonClass   GtkLockButtonClass;
typedef struct _GtkLockButtonPrivate GtkLockButtonPrivate;

struct _GtkLockButton
{
  GtkButton parent;

  GtkLockButtonPrivate *priv;
};

/**
 * GtkLockButtonClass:
 * @parent_class: The parent class.
 */
struct _GtkLockButtonClass
{
  GtkButtonClass parent_class;

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
GtkWidget   *ctk_lock_button_new            (GPermission   *permission);
GDK_AVAILABLE_IN_3_2
GPermission *ctk_lock_button_get_permission (GtkLockButton *button);
GDK_AVAILABLE_IN_3_2
void         ctk_lock_button_set_permission (GtkLockButton *button,
                                             GPermission   *permission);


G_END_DECLS

#endif  /* __CTK_LOCK_BUTTON_H__ */
