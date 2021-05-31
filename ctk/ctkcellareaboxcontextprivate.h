/* ctkcellareaboxcontext.h
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

#ifndef __CTK_CELL_AREA_BOX_CONTEXT_H__
#define __CTK_CELL_AREA_BOX_CONTEXT_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkcellareacontext.h>
#include <ctk/ctkcellareabox.h>
#include <ctk/ctkcellrenderer.h>
#include <ctk/ctksizerequest.h>

G_BEGIN_DECLS

#define CTK_TYPE_CELL_AREA_BOX_CONTEXT            (_ctk_cell_area_box_context_get_type ())
#define CTK_CELL_AREA_BOX_CONTEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_CELL_AREA_BOX_CONTEXT, CtkCellAreaBoxContext))
#define CTK_CELL_AREA_BOX_CONTEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_CELL_AREA_BOX_CONTEXT, CtkCellAreaBoxContextClass))
#define CTK_IS_CELL_AREA_BOX_CONTEXT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_CELL_AREA_BOX_CONTEXT))
#define CTK_IS_CELL_AREA_BOX_CONTEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_CELL_AREA_BOX_CONTEXT))
#define CTK_CELL_AREA_BOX_CONTEXT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CELL_AREA_BOX_CONTEXT, CtkCellAreaBoxContextClass))

typedef struct _CtkCellAreaBoxContext              CtkCellAreaBoxContext;
typedef struct _CtkCellAreaBoxContextClass         CtkCellAreaBoxContextClass;
typedef struct _CtkCellAreaBoxContextPrivate       CtkCellAreaBoxContextPrivate;

struct _CtkCellAreaBoxContext
{
  CtkCellAreaContext parent_instance;

  CtkCellAreaBoxContextPrivate *priv;
};

struct _CtkCellAreaBoxContextClass
{
  CtkCellAreaContextClass parent_class;

};

GType   _ctk_cell_area_box_context_get_type                     (void) G_GNUC_CONST;


/* Create a duplicate of the context */
CtkCellAreaBoxContext *_ctk_cell_area_box_context_copy          (CtkCellAreaBox        *box,
                                                                CtkCellAreaBoxContext *box_context);

/* Initialize group array dimensions */
void    _ctk_cell_area_box_init_groups                         (CtkCellAreaBoxContext *box_context,
                                                                guint                  n_groups,
                                                                gboolean              *expand_groups,
                                                                gboolean              *align_groups);

/* Update cell-group sizes */
void    _ctk_cell_area_box_context_push_group_width             (CtkCellAreaBoxContext *box_context,
                                                                gint                   group_idx,
                                                                gint                   minimum_width,
                                                                gint                   natural_width);

void    _ctk_cell_area_box_context_push_group_height_for_width  (CtkCellAreaBoxContext *box_context,
                                                                gint                   group_idx,
                                                                gint                   for_width,
                                                                gint                   minimum_height,
                                                                gint                   natural_height);

void    _ctk_cell_area_box_context_push_group_height            (CtkCellAreaBoxContext *box_context,
                                                                gint                   group_idx,
                                                                gint                   minimum_height,
                                                                gint                   natural_height);

void    _ctk_cell_area_box_context_push_group_width_for_height  (CtkCellAreaBoxContext *box_context,
                                                                gint                   group_idx,
                                                                gint                   for_height,
                                                                gint                   minimum_width,
                                                                gint                   natural_width);

/* Fetch cell-group sizes */
void    _ctk_cell_area_box_context_get_group_width              (CtkCellAreaBoxContext *box_context,
                                                                gint                   group_idx,
                                                                gint                  *minimum_width,
                                                                gint                  *natural_width);

void    _ctk_cell_area_box_context_get_group_height_for_width   (CtkCellAreaBoxContext *box_context,
                                                                gint                   group_idx,
                                                                gint                   for_width,
                                                                gint                  *minimum_height,
                                                                gint                  *natural_height);

void    _ctk_cell_area_box_context_get_group_height             (CtkCellAreaBoxContext *box_context,
                                                                gint                   group_idx,
                                                                gint                  *minimum_height,
                                                                gint                  *natural_height);

void    _ctk_cell_area_box_context_get_group_width_for_height   (CtkCellAreaBoxContext *box_context,
                                                                gint                   group_idx,
                                                                gint                   for_height,
                                                                gint                  *minimum_width,
                                                                gint                  *natural_width);

CtkRequestedSize *_ctk_cell_area_box_context_get_widths         (CtkCellAreaBoxContext *box_context,
                                                                gint                  *n_widths);
CtkRequestedSize *_ctk_cell_area_box_context_get_heights        (CtkCellAreaBoxContext *box_context,
                                                                gint                  *n_heights);

/* Private context/area interaction */
typedef struct {
  gint group_idx; /* Groups containing only invisible cells are not allocated */
  gint position;  /* Relative group allocation position in the orientation of the box */
  gint size;      /* Full allocated size of the cells in this group spacing inclusive */
} CtkCellAreaBoxAllocation;

CtkCellAreaBoxAllocation *
_ctk_cell_area_box_context_get_orientation_allocs (CtkCellAreaBoxContext *context,
                                                  gint                  *n_allocs);

G_END_DECLS

#endif /* __CTK_CELL_AREA_BOX_CONTEXT_H__ */
