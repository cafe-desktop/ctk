/* ctkcellareabox.h
 *
 * Copyright (C) 2010 Openismus GmbH
 *
 * Authors:
 *      Tristan Van Berkom <tristanvb@openismus.com>
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

#ifndef __CTK_CELL_AREA_BOX_H__
#define __CTK_CELL_AREA_BOX_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkcellarea.h>

G_BEGIN_DECLS

#define CTK_TYPE_CELL_AREA_BOX            (ctk_cell_area_box_get_type ())
#define CTK_CELL_AREA_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_CELL_AREA_BOX, CtkCellAreaBox))
#define CTK_CELL_AREA_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_CELL_AREA_BOX, CtkCellAreaBoxClass))
#define CTK_IS_CELL_AREA_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_CELL_AREA_BOX))
#define CTK_IS_CELL_AREA_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_CELL_AREA_BOX))
#define CTK_CELL_AREA_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CELL_AREA_BOX, CtkCellAreaBoxClass))

typedef struct _CtkCellAreaBox              CtkCellAreaBox;
typedef struct _CtkCellAreaBoxClass         CtkCellAreaBoxClass;
typedef struct _CtkCellAreaBoxPrivate       CtkCellAreaBoxPrivate;

struct _CtkCellAreaBox
{
  /*< private >*/
  CtkCellArea parent_instance;

  CtkCellAreaBoxPrivate *priv;
};

/**
 * CtkCellAreaBoxClass:
 */
struct _CtkCellAreaBoxClass
{
  /*< private >*/
  CtkCellAreaClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

CDK_AVAILABLE_IN_ALL
GType        ctk_cell_area_box_get_type    (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_ALL
CtkCellArea *ctk_cell_area_box_new         (void);
CDK_AVAILABLE_IN_ALL
void         ctk_cell_area_box_pack_start  (CtkCellAreaBox  *box,
                                            CtkCellRenderer *renderer,
                                            gboolean         expand,
                                            gboolean         align,
                                            gboolean         fixed);
CDK_AVAILABLE_IN_ALL
void         ctk_cell_area_box_pack_end    (CtkCellAreaBox  *box,
                                            CtkCellRenderer *renderer,
                                            gboolean         expand,
                                            gboolean         align,
                                            gboolean         fixed);
CDK_AVAILABLE_IN_ALL
gint         ctk_cell_area_box_get_spacing (CtkCellAreaBox  *box);
CDK_AVAILABLE_IN_ALL
void         ctk_cell_area_box_set_spacing (CtkCellAreaBox  *box,
                                            gint             spacing);

/* Private interaction with CtkCellAreaBoxContext */
gboolean    _ctk_cell_area_box_group_visible (CtkCellAreaBox  *box,
                                              gint             group_idx);

G_END_DECLS

#endif /* __CTK_CELL_AREA_BOX_H__ */
