/* ctkcellrenderer.h
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

#ifndef __CTK_CELL_RENDERER_H__
#define __CTK_CELL_RENDERER_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkcelleditable.h>

G_BEGIN_DECLS


/**
 * CtkCellRendererState:
 * @CTK_CELL_RENDERER_SELECTED: The cell is currently selected, and
 *  probably has a selection colored background to render to.
 * @CTK_CELL_RENDERER_PRELIT: The mouse is hovering over the cell.
 * @CTK_CELL_RENDERER_INSENSITIVE: The cell is drawn in an insensitive manner
 * @CTK_CELL_RENDERER_SORTED: The cell is in a sorted row
 * @CTK_CELL_RENDERER_FOCUSED: The cell is in the focus row.
 * @CTK_CELL_RENDERER_EXPANDABLE: The cell is in a row that can be expanded. Since 3.4
 * @CTK_CELL_RENDERER_EXPANDED: The cell is in a row that is expanded. Since 3.4
 *
 * Tells how a cell is to be rendered.
 */
typedef enum
{
  CTK_CELL_RENDERER_SELECTED    = 1 << 0,
  CTK_CELL_RENDERER_PRELIT      = 1 << 1,
  CTK_CELL_RENDERER_INSENSITIVE = 1 << 2,
  /* this flag means the cell is in the sort column/row */
  CTK_CELL_RENDERER_SORTED      = 1 << 3,
  CTK_CELL_RENDERER_FOCUSED     = 1 << 4,
  CTK_CELL_RENDERER_EXPANDABLE  = 1 << 5,
  CTK_CELL_RENDERER_EXPANDED    = 1 << 6
} CtkCellRendererState;

/**
 * CtkCellRendererMode:
 * @CTK_CELL_RENDERER_MODE_INERT: The cell is just for display
 *  and cannot be interacted with.  Note that this doesn’t mean that eg. the
 *  row being drawn can’t be selected -- just that a particular element of
 *  it cannot be individually modified.
 * @CTK_CELL_RENDERER_MODE_ACTIVATABLE: The cell can be clicked.
 * @CTK_CELL_RENDERER_MODE_EDITABLE: The cell can be edited or otherwise modified.
 *
 * Identifies how the user can interact with a particular cell.
 */
typedef enum
{
  CTK_CELL_RENDERER_MODE_INERT,
  CTK_CELL_RENDERER_MODE_ACTIVATABLE,
  CTK_CELL_RENDERER_MODE_EDITABLE
} CtkCellRendererMode;

#define CTK_TYPE_CELL_RENDERER		  (ctk_cell_renderer_get_type ())
#define CTK_CELL_RENDERER(obj)		  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_CELL_RENDERER, CtkCellRenderer))
#define CTK_CELL_RENDERER_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_CELL_RENDERER, CtkCellRendererClass))
#define CTK_IS_CELL_RENDERER(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_CELL_RENDERER))
#define CTK_IS_CELL_RENDERER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_CELL_RENDERER))
#define CTK_CELL_RENDERER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CELL_RENDERER, CtkCellRendererClass))

typedef struct _CtkCellRenderer              CtkCellRenderer;
typedef struct _CtkCellRendererPrivate       CtkCellRendererPrivate;
typedef struct _CtkCellRendererClass         CtkCellRendererClass;
typedef struct _CtkCellRendererClassPrivate  CtkCellRendererClassPrivate;

struct _CtkCellRenderer
{
  GInitiallyUnowned parent_instance;

  /*< private >*/
  CtkCellRendererPrivate *priv;
};

/**
 * CtkCellRendererClass:
 * @get_request_mode: Called to gets whether the cell renderer prefers
 *    a height-for-width layout or a width-for-height layout.
 * @get_preferred_width: Called to get a renderer’s natural width.
 * @get_preferred_height_for_width: Called to get a renderer’s natural height for width.
 * @get_preferred_height: Called to get a renderer’s natural height.
 * @get_preferred_width_for_height: Called to get a renderer’s natural width for height.
 * @get_aligned_area: Called to get the aligned area used by @cell inside @cell_area.
 * @get_size: Called to get the width and height needed to render the cell. Deprecated: 3.0.
 * @render: Called to render the content of the #CtkCellRenderer.
 * @activate: Called to activate the content of the #CtkCellRenderer.
 * @start_editing: Called to initiate editing the content of the #CtkCellRenderer.
 * @editing_canceled: Signal gets emitted when the user cancels the process of editing a cell.
 * @editing_started: Signal gets emitted when a cell starts to be edited.
 */
struct _CtkCellRendererClass
{
  /*< private >*/
  GInitiallyUnownedClass parent_class;

  /*< public >*/

  /* vtable - not signals */
  CtkSizeRequestMode (* get_request_mode)                (CtkCellRenderer      *cell);
  void               (* get_preferred_width)             (CtkCellRenderer      *cell,
                                                          CtkWidget            *widget,
                                                          gint                 *minimum_size,
                                                          gint                 *natural_size);
  void               (* get_preferred_height_for_width)  (CtkCellRenderer      *cell,
                                                          CtkWidget            *widget,
                                                          gint                  width,
                                                          gint                 *minimum_height,
                                                          gint                 *natural_height);
  void               (* get_preferred_height)            (CtkCellRenderer      *cell,
                                                          CtkWidget            *widget,
                                                          gint                 *minimum_size,
                                                          gint                 *natural_size);
  void               (* get_preferred_width_for_height)  (CtkCellRenderer      *cell,
                                                          CtkWidget            *widget,
                                                          gint                  height,
                                                          gint                 *minimum_width,
                                                          gint                 *natural_width);
  void               (* get_aligned_area)                (CtkCellRenderer      *cell,
                                                          CtkWidget            *widget,
							  CtkCellRendererState  flags,
                                                          const GdkRectangle   *cell_area,
                                                          GdkRectangle         *aligned_area);
  void               (* get_size)                        (CtkCellRenderer      *cell,
                                                          CtkWidget            *widget,
                                                          const GdkRectangle   *cell_area,
                                                          gint                 *x_offset,
                                                          gint                 *y_offset,
                                                          gint                 *width,
                                                          gint                 *height);
  void               (* render)                          (CtkCellRenderer      *cell,
                                                          cairo_t              *cr,
                                                          CtkWidget            *widget,
                                                          const GdkRectangle   *background_area,
                                                          const GdkRectangle   *cell_area,
                                                          CtkCellRendererState  flags);
  gboolean           (* activate)                        (CtkCellRenderer      *cell,
                                                          GdkEvent             *event,
                                                          CtkWidget            *widget,
                                                          const gchar          *path,
                                                          const GdkRectangle   *background_area,
                                                          const GdkRectangle   *cell_area,
                                                          CtkCellRendererState  flags);
  CtkCellEditable *  (* start_editing)                   (CtkCellRenderer      *cell,
                                                          GdkEvent             *event,
                                                          CtkWidget            *widget,
                                                          const gchar          *path,
                                                          const GdkRectangle   *background_area,
                                                          const GdkRectangle   *cell_area,
                                                          CtkCellRendererState  flags);

  /* Signals */
  void (* editing_canceled) (CtkCellRenderer *cell);
  void (* editing_started)  (CtkCellRenderer *cell,
			     CtkCellEditable *editable,
			     const gchar     *path);

  /*< private >*/

  CtkCellRendererClassPrivate *priv;

  /* Padding for future expansion */
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
GType              ctk_cell_renderer_get_type       (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
CtkSizeRequestMode ctk_cell_renderer_get_request_mode               (CtkCellRenderer    *cell);
GDK_AVAILABLE_IN_ALL
void               ctk_cell_renderer_get_preferred_width            (CtkCellRenderer    *cell,
                                                                     CtkWidget          *widget,
                                                                     gint               *minimum_size,
                                                                     gint               *natural_size);
GDK_AVAILABLE_IN_ALL
void               ctk_cell_renderer_get_preferred_height_for_width (CtkCellRenderer    *cell,
                                                                     CtkWidget          *widget,
                                                                     gint                width,
                                                                     gint               *minimum_height,
                                                                     gint               *natural_height);
GDK_AVAILABLE_IN_ALL
void               ctk_cell_renderer_get_preferred_height           (CtkCellRenderer    *cell,
                                                                     CtkWidget          *widget,
                                                                     gint               *minimum_size,
                                                                     gint               *natural_size);
GDK_AVAILABLE_IN_ALL
void               ctk_cell_renderer_get_preferred_width_for_height (CtkCellRenderer    *cell,
                                                                     CtkWidget          *widget,
                                                                     gint                height,
                                                                     gint               *minimum_width,
                                                                     gint               *natural_width);
GDK_AVAILABLE_IN_ALL
void               ctk_cell_renderer_get_preferred_size             (CtkCellRenderer    *cell,
                                                                     CtkWidget          *widget,
                                                                     CtkRequisition     *minimum_size,
                                                                     CtkRequisition     *natural_size);
GDK_AVAILABLE_IN_ALL
void               ctk_cell_renderer_get_aligned_area               (CtkCellRenderer    *cell,
								     CtkWidget          *widget,
								     CtkCellRendererState flags,
								     const GdkRectangle *cell_area,
								     GdkRectangle       *aligned_area);

GDK_DEPRECATED_IN_3_0_FOR(ctk_cell_renderer_get_preferred_size)
void             ctk_cell_renderer_get_size       (CtkCellRenderer      *cell,
                                                   CtkWidget            *widget,
                                                   const GdkRectangle   *cell_area,
                                                   gint                 *x_offset,
                                                   gint                 *y_offset,
                                                   gint                 *width,
                                                   gint                 *height);
GDK_AVAILABLE_IN_ALL
void             ctk_cell_renderer_render         (CtkCellRenderer      *cell,
                                                   cairo_t              *cr,
						   CtkWidget            *widget,
						   const GdkRectangle   *background_area,
						   const GdkRectangle   *cell_area,
						   CtkCellRendererState  flags);
GDK_AVAILABLE_IN_ALL
gboolean         ctk_cell_renderer_activate       (CtkCellRenderer      *cell,
						   GdkEvent             *event,
						   CtkWidget            *widget,
						   const gchar          *path,
						   const GdkRectangle   *background_area,
						   const GdkRectangle   *cell_area,
						   CtkCellRendererState  flags);
GDK_AVAILABLE_IN_ALL
CtkCellEditable *ctk_cell_renderer_start_editing  (CtkCellRenderer      *cell,
						   GdkEvent             *event,
						   CtkWidget            *widget,
						   const gchar          *path,
						   const GdkRectangle   *background_area,
						   const GdkRectangle   *cell_area,
						   CtkCellRendererState  flags);

GDK_AVAILABLE_IN_ALL
void             ctk_cell_renderer_set_fixed_size (CtkCellRenderer      *cell,
						   gint                  width,
						   gint                  height);
GDK_AVAILABLE_IN_ALL
void             ctk_cell_renderer_get_fixed_size (CtkCellRenderer      *cell,
						   gint                 *width,
						   gint                 *height);

GDK_AVAILABLE_IN_ALL
void             ctk_cell_renderer_set_alignment  (CtkCellRenderer      *cell,
                                                   gfloat                xalign,
                                                   gfloat                yalign);
GDK_AVAILABLE_IN_ALL
void             ctk_cell_renderer_get_alignment  (CtkCellRenderer      *cell,
                                                   gfloat               *xalign,
                                                   gfloat               *yalign);

GDK_AVAILABLE_IN_ALL
void             ctk_cell_renderer_set_padding    (CtkCellRenderer      *cell,
                                                   gint                  xpad,
                                                   gint                  ypad);
GDK_AVAILABLE_IN_ALL
void             ctk_cell_renderer_get_padding    (CtkCellRenderer      *cell,
                                                   gint                 *xpad,
                                                   gint                 *ypad);

GDK_AVAILABLE_IN_ALL
void             ctk_cell_renderer_set_visible    (CtkCellRenderer      *cell,
                                                   gboolean              visible);
GDK_AVAILABLE_IN_ALL
gboolean         ctk_cell_renderer_get_visible    (CtkCellRenderer      *cell);

GDK_AVAILABLE_IN_ALL
void             ctk_cell_renderer_set_sensitive  (CtkCellRenderer      *cell,
                                                   gboolean              sensitive);
GDK_AVAILABLE_IN_ALL
gboolean         ctk_cell_renderer_get_sensitive  (CtkCellRenderer      *cell);

GDK_AVAILABLE_IN_ALL
gboolean         ctk_cell_renderer_is_activatable (CtkCellRenderer      *cell);

/* For use by cell renderer implementations only */
GDK_AVAILABLE_IN_ALL
void             ctk_cell_renderer_stop_editing   (CtkCellRenderer      *cell,
                                                   gboolean              canceled);


void            _ctk_cell_renderer_calc_offset    (CtkCellRenderer      *cell,
                                                   const GdkRectangle   *cell_area,
                                                   CtkTextDirection      direction,
                                                   gint                  width,
                                                   gint                  height,
                                                   gint                 *x_offset,
                                                   gint                 *y_offset);

GDK_AVAILABLE_IN_ALL
CtkStateFlags   ctk_cell_renderer_get_state       (CtkCellRenderer      *cell,
                                                   CtkWidget            *widget,
                                                   CtkCellRendererState  cell_state);

GDK_AVAILABLE_IN_ALL
void            ctk_cell_renderer_class_set_accessible_type 
                                                  (CtkCellRendererClass *renderer_class,
                                                   GType                 type);
GType           _ctk_cell_renderer_get_accessible_type
                                                  (CtkCellRenderer *     renderer);

G_END_DECLS

#endif /* __CTK_CELL_RENDERER_H__ */
