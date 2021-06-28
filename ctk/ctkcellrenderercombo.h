/* CtkCellRendererCombo
 * Copyright (C) 2004 Lorenzo Gil Sanchez
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

#ifndef __CTK_CELL_RENDERER_COMBO_H__
#define __CTK_CELL_RENDERER_COMBO_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctktreemodel.h>
#include <ctk/ctkcellrenderertext.h>

G_BEGIN_DECLS

#define CTK_TYPE_CELL_RENDERER_COMBO		(ctk_cell_renderer_combo_get_type ())
#define CTK_CELL_RENDERER_COMBO(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_CELL_RENDERER_COMBO, CtkCellRendererCombo))
#define CTK_CELL_RENDERER_COMBO_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_CELL_RENDERER_COMBO, CtkCellRendererComboClass))
#define CTK_IS_CELL_RENDERER_COMBO(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_CELL_RENDERER_COMBO))
#define CTK_IS_CELL_RENDERER_COMBO_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_CELL_RENDERER_COMBO))
#define CTK_CELL_RENDERER_COMBO_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CELL_RENDERER_COMBO, CtkCellRendererTextClass))

typedef struct _CtkCellRendererCombo              CtkCellRendererCombo;
typedef struct _CtkCellRendererComboPrivate       CtkCellRendererComboPrivate;
typedef struct _CtkCellRendererComboClass         CtkCellRendererComboClass;

struct _CtkCellRendererCombo
{
  CtkCellRendererText parent;

  /*< private >*/
  CtkCellRendererComboPrivate *priv;
};

struct _CtkCellRendererComboClass
{
  CtkCellRendererTextClass parent;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

CDK_AVAILABLE_IN_ALL
GType            ctk_cell_renderer_combo_get_type (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkCellRenderer *ctk_cell_renderer_combo_new      (void);

G_END_DECLS

#endif /* __CTK_CELL_RENDERER_COMBO_H__ */
