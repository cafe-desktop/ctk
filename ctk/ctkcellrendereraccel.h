/* ctkcellrendereraccel.h
 * Copyright (C) 2000  Red Hat, Inc.,  Jonathan Blandford <jrb@redhat.com>
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

#ifndef __CTK_CELL_RENDERER_ACCEL_H__
#define __CTK_CELL_RENDERER_ACCEL_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkcellrenderertext.h>

G_BEGIN_DECLS

#define CTK_TYPE_CELL_RENDERER_ACCEL            (ctk_cell_renderer_accel_get_type ())
#define CTK_CELL_RENDERER_ACCEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_CELL_RENDERER_ACCEL, CtkCellRendererAccel))
#define CTK_CELL_RENDERER_ACCEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_CELL_RENDERER_ACCEL, CtkCellRendererAccelClass))
#define CTK_IS_CELL_RENDERER_ACCEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_CELL_RENDERER_ACCEL))
#define CTK_IS_CELL_RENDERER_ACCEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_CELL_RENDERER_ACCEL))
#define CTK_CELL_RENDERER_ACCEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CELL_RENDERER_ACCEL, CtkCellRendererAccelClass))

typedef struct _CtkCellRendererAccel              CtkCellRendererAccel;
typedef struct _CtkCellRendererAccelPrivate       CtkCellRendererAccelPrivate;
typedef struct _CtkCellRendererAccelClass         CtkCellRendererAccelClass;

/**
 * CtkCellRendererAccelMode:
 * @CTK_CELL_RENDERER_ACCEL_MODE_CTK: CTK+ accelerators mode
 * @CTK_CELL_RENDERER_ACCEL_MODE_OTHER: Other accelerator mode
 *
 * Determines if the edited accelerators are CTK+ accelerators. If
 * they are, consumed modifiers are suppressed, only accelerators
 * accepted by CTK+ are allowed, and the accelerators are rendered
 * in the same way as they are in menus.
 */
typedef enum
{
  CTK_CELL_RENDERER_ACCEL_MODE_CTK,
  CTK_CELL_RENDERER_ACCEL_MODE_OTHER
} CtkCellRendererAccelMode;


struct _CtkCellRendererAccel
{
  CtkCellRendererText parent;

  /*< private >*/
  CtkCellRendererAccelPrivate *priv;
};

struct _CtkCellRendererAccelClass
{
  CtkCellRendererTextClass parent_class;

  void (* accel_edited)  (CtkCellRendererAccel *accel,
		 	  const gchar          *path_string,
			  guint                 accel_key,
			  CdkModifierType       accel_mods,
			  guint                 hardware_keycode);

  void (* accel_cleared) (CtkCellRendererAccel *accel,
			  const gchar          *path_string);

  /* Padding for future expansion */
  void (*_ctk_reserved0) (void);
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

CDK_AVAILABLE_IN_ALL
GType            ctk_cell_renderer_accel_get_type        (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkCellRenderer *ctk_cell_renderer_accel_new             (void);


G_END_DECLS


#endif /* __CTK_CELL_RENDERER_ACCEL_H__ */
