/* CTK - The GIMP Toolkit
 * Copyright (C) Christian Kellner <gicmo@gnome.org>
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
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_MOUNT_OPERATION_H__
#define __CTK_MOUNT_OPERATION_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

G_BEGIN_DECLS

#define CTK_TYPE_MOUNT_OPERATION         (ctk_mount_operation_get_type ())
#define CTK_MOUNT_OPERATION(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_MOUNT_OPERATION, CtkMountOperation))
#define CTK_MOUNT_OPERATION_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), CTK_TYPE_MOUNT_OPERATION, CtkMountOperationClass))
#define CTK_IS_MOUNT_OPERATION(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_MOUNT_OPERATION))
#define CTK_IS_MOUNT_OPERATION_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), CTK_TYPE_MOUNT_OPERATION))
#define CTK_MOUNT_OPERATION_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CTK_TYPE_MOUNT_OPERATION, CtkMountOperationClass))

typedef struct _CtkMountOperation         CtkMountOperation;
typedef struct _CtkMountOperationClass    CtkMountOperationClass;
typedef struct _CtkMountOperationPrivate  CtkMountOperationPrivate;

/**
 * CtkMountOperation:
 *
 * This should not be accessed directly. Use the accessor functions below.
 */
struct _CtkMountOperation
{
  GMountOperation parent_instance;

  CtkMountOperationPrivate *priv;
};

/**
 * CtkMountOperationClass:
 * @parent_class: The parent class.
 */
struct _CtkMountOperationClass
{
  GMountOperationClass parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


GDK_AVAILABLE_IN_ALL
GType            ctk_mount_operation_get_type   (void);
GDK_AVAILABLE_IN_ALL
GMountOperation *ctk_mount_operation_new        (CtkWindow         *parent);
GDK_AVAILABLE_IN_ALL
gboolean         ctk_mount_operation_is_showing (CtkMountOperation *op);
GDK_AVAILABLE_IN_ALL
void             ctk_mount_operation_set_parent (CtkMountOperation *op,
                                                 CtkWindow         *parent);
GDK_AVAILABLE_IN_ALL
CtkWindow *      ctk_mount_operation_get_parent (CtkMountOperation *op);
GDK_AVAILABLE_IN_ALL
void             ctk_mount_operation_set_screen (CtkMountOperation *op,
                                                 CdkScreen         *screen);
GDK_AVAILABLE_IN_ALL
CdkScreen       *ctk_mount_operation_get_screen (CtkMountOperation *op);

G_END_DECLS

#endif /* __CTK_MOUNT_OPERATION_H__ */
