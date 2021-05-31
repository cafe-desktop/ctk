/* ctkcellareacontext.h
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

#ifndef __CTK_CELL_AREA_CONTEXT_H__
#define __CTK_CELL_AREA_CONTEXT_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkcellarea.h>

G_BEGIN_DECLS

#define CTK_TYPE_CELL_AREA_CONTEXT            (ctk_cell_area_context_get_type ())
#define CTK_CELL_AREA_CONTEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_CELL_AREA_CONTEXT, CtkCellAreaContext))
#define CTK_CELL_AREA_CONTEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_CELL_AREA_CONTEXT, CtkCellAreaContextClass))
#define CTK_IS_CELL_AREA_CONTEXT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_CELL_AREA_CONTEXT))
#define CTK_IS_CELL_AREA_CONTEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_CELL_AREA_CONTEXT))
#define CTK_CELL_AREA_CONTEXT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CELL_AREA_CONTEXT, CtkCellAreaContextClass))

typedef struct _CtkCellAreaContextPrivate       CtkCellAreaContextPrivate;
typedef struct _CtkCellAreaContextClass         CtkCellAreaContextClass;

struct _CtkCellAreaContext
{
  /*< private >*/
  GObject parent_instance;

  CtkCellAreaContextPrivate *priv;
};

/**
 * CtkCellAreaContextClass:
 * @allocate: This tells the context that an allocation width or height
 *     (or both) have been decided for a group of rows. The context should
 *     store any allocations for internally aligned cells at this point so
 *     that they dont need to be recalculated at ctk_cell_area_render() time.
 * @reset: Clear any previously stored information about requested and
 *     allocated sizes for the context.
 * @get_preferred_height_for_width: Returns the aligned height for the given
 *     width that context must store while collecting sizes for it’s rows.
 * @get_preferred_width_for_height: Returns the aligned width for the given
 *     height that context must store while collecting sizes for it’s rows.
 */
struct _CtkCellAreaContextClass
{
  /*< private >*/
  GObjectClass parent_class;

  /*< public >*/
  void    (* allocate)                       (CtkCellAreaContext *context,
                                              gint                width,
                                              gint                height);
  void    (* reset)                          (CtkCellAreaContext *context);
  void    (* get_preferred_height_for_width) (CtkCellAreaContext *context,
                                              gint                width,
                                              gint               *minimum_height,
                                              gint               *natural_height);
  void    (* get_preferred_width_for_height) (CtkCellAreaContext *context,
                                              gint                height,
                                              gint               *minimum_width,
                                              gint               *natural_width);

  /*< private >*/
  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
  void (*_ctk_reserved5) (void);
  void (*_ctk_reserved6) (void);
};

GDK_AVAILABLE_IN_ALL
GType        ctk_cell_area_context_get_type              (void) G_GNUC_CONST;

/* Main apis */
GDK_AVAILABLE_IN_ALL
CtkCellArea *ctk_cell_area_context_get_area                        (CtkCellAreaContext *context);
GDK_AVAILABLE_IN_ALL
void         ctk_cell_area_context_allocate                        (CtkCellAreaContext *context,
                                                                    gint                width,
                                                                    gint                height);
GDK_AVAILABLE_IN_ALL
void         ctk_cell_area_context_reset                           (CtkCellAreaContext *context);

/* Apis for CtkCellArea clients to consult cached values
 * for a series of CtkTreeModel rows
 */
GDK_AVAILABLE_IN_ALL
void         ctk_cell_area_context_get_preferred_width            (CtkCellAreaContext *context,
                                                                   gint               *minimum_width,
                                                                   gint               *natural_width);
GDK_AVAILABLE_IN_ALL
void         ctk_cell_area_context_get_preferred_height           (CtkCellAreaContext *context,
                                                                   gint               *minimum_height,
                                                                   gint               *natural_height);
GDK_AVAILABLE_IN_ALL
void         ctk_cell_area_context_get_preferred_height_for_width (CtkCellAreaContext *context,
                                                                   gint                width,
                                                                   gint               *minimum_height,
                                                                   gint               *natural_height);
GDK_AVAILABLE_IN_ALL
void         ctk_cell_area_context_get_preferred_width_for_height (CtkCellAreaContext *context,
                                                                   gint                height,
                                                                   gint               *minimum_width,
                                                                   gint               *natural_width);
GDK_AVAILABLE_IN_ALL
void         ctk_cell_area_context_get_allocation                 (CtkCellAreaContext *context,
                                                                   gint               *width,
                                                                   gint               *height);

/* Apis for CtkCellArea implementations to update cached values
 * for multiple CtkTreeModel rows
 */
GDK_AVAILABLE_IN_ALL
void         ctk_cell_area_context_push_preferred_width  (CtkCellAreaContext *context,
                                                          gint                minimum_width,
                                                          gint                natural_width);
GDK_AVAILABLE_IN_ALL
void         ctk_cell_area_context_push_preferred_height (CtkCellAreaContext *context,
                                                          gint                minimum_height,
                                                          gint                natural_height);

G_END_DECLS

#endif /* __CTK_CELL_AREA_CONTEXT_H__ */
