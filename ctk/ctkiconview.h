/* ctkiconview.h
 * Copyright (C) 2002, 2004  Anders Carlsson <andersca@gnome.org>
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

#ifndef __CTK_ICON_VIEW_H__
#define __CTK_ICON_VIEW_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkcontainer.h>
#include <ctk/ctktreemodel.h>
#include <ctk/ctkcellrenderer.h>
#include <ctk/ctkcellarea.h>
#include <ctk/ctkselection.h>
#include <ctk/ctktooltip.h>

G_BEGIN_DECLS

#define CTK_TYPE_ICON_VIEW            (ctk_icon_view_get_type ())
#define CTK_ICON_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_ICON_VIEW, CtkIconView))
#define CTK_ICON_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_ICON_VIEW, CtkIconViewClass))
#define CTK_IS_ICON_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_ICON_VIEW))
#define CTK_IS_ICON_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_ICON_VIEW))
#define CTK_ICON_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_ICON_VIEW, CtkIconViewClass))

typedef struct _CtkIconView           CtkIconView;
typedef struct _CtkIconViewClass      CtkIconViewClass;
typedef struct _CtkIconViewPrivate    CtkIconViewPrivate;

/**
 * CtkIconViewForeachFunc:
 * @icon_view: a #CtkIconView
 * @path: The #CtkTreePath of a selected row
 * @data: (closure): user data
 *
 * A function used by ctk_icon_view_selected_foreach() to map all
 * selected rows.  It will be called on every selected row in the view.
 */
typedef void (* CtkIconViewForeachFunc)     (CtkIconView      *icon_view,
					     CtkTreePath      *path,
					     gpointer          data);

/**
 * CtkIconViewDropPosition:
 * @CTK_ICON_VIEW_NO_DROP: no drop possible
 * @CTK_ICON_VIEW_DROP_INTO: dropped item replaces the item
 * @CTK_ICON_VIEW_DROP_LEFT: droppped item is inserted to the left
 * @CTK_ICON_VIEW_DROP_RIGHT: dropped item is inserted to the right
 * @CTK_ICON_VIEW_DROP_ABOVE: dropped item is inserted above
 * @CTK_ICON_VIEW_DROP_BELOW: dropped item is inserted below
 *
 * An enum for determining where a dropped item goes.
 */
typedef enum
{
  CTK_ICON_VIEW_NO_DROP,
  CTK_ICON_VIEW_DROP_INTO,
  CTK_ICON_VIEW_DROP_LEFT,
  CTK_ICON_VIEW_DROP_RIGHT,
  CTK_ICON_VIEW_DROP_ABOVE,
  CTK_ICON_VIEW_DROP_BELOW
} CtkIconViewDropPosition;

struct _CtkIconView
{
  CtkContainer parent;

  /*< private >*/
  CtkIconViewPrivate *priv;
};

struct _CtkIconViewClass
{
  CtkContainerClass parent_class;

  void    (* item_activated)         (CtkIconView      *icon_view,
				      CtkTreePath      *path);
  void    (* selection_changed)      (CtkIconView      *icon_view);

  /* Key binding signals */
  void    (* select_all)             (CtkIconView      *icon_view);
  void    (* unselect_all)           (CtkIconView      *icon_view);
  void    (* select_cursor_item)     (CtkIconView      *icon_view);
  void    (* toggle_cursor_item)     (CtkIconView      *icon_view);
  gboolean (* move_cursor)           (CtkIconView      *icon_view,
				      CtkMovementStep   step,
				      gint              count);
  gboolean (* activate_cursor_item)  (CtkIconView      *icon_view);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

CDK_AVAILABLE_IN_ALL
GType          ctk_icon_view_get_type          (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkWidget *    ctk_icon_view_new               (void);
CDK_AVAILABLE_IN_ALL
CtkWidget *    ctk_icon_view_new_with_area     (CtkCellArea    *area);
CDK_AVAILABLE_IN_ALL
CtkWidget *    ctk_icon_view_new_with_model    (CtkTreeModel   *model);

CDK_AVAILABLE_IN_ALL
void           ctk_icon_view_set_model         (CtkIconView    *icon_view,
 					        CtkTreeModel   *model);
CDK_AVAILABLE_IN_ALL
CtkTreeModel * ctk_icon_view_get_model         (CtkIconView    *icon_view);
CDK_AVAILABLE_IN_ALL
void           ctk_icon_view_set_text_column   (CtkIconView    *icon_view,
	 	 			        gint            column);
CDK_AVAILABLE_IN_ALL
gint           ctk_icon_view_get_text_column   (CtkIconView    *icon_view);
CDK_AVAILABLE_IN_ALL
void           ctk_icon_view_set_markup_column (CtkIconView    *icon_view,
					        gint            column);
CDK_AVAILABLE_IN_ALL
gint           ctk_icon_view_get_markup_column (CtkIconView    *icon_view);
CDK_AVAILABLE_IN_ALL
void           ctk_icon_view_set_pixbuf_column (CtkIconView    *icon_view,
					        gint            column);
CDK_AVAILABLE_IN_ALL
gint           ctk_icon_view_get_pixbuf_column (CtkIconView    *icon_view);

CDK_AVAILABLE_IN_ALL
void           ctk_icon_view_set_item_orientation (CtkIconView    *icon_view,
                                                   CtkOrientation  orientation);
CDK_AVAILABLE_IN_ALL
CtkOrientation ctk_icon_view_get_item_orientation (CtkIconView    *icon_view);
CDK_AVAILABLE_IN_ALL
void           ctk_icon_view_set_columns       (CtkIconView    *icon_view,
		 			        gint            columns);
CDK_AVAILABLE_IN_ALL
gint           ctk_icon_view_get_columns       (CtkIconView    *icon_view);
CDK_AVAILABLE_IN_ALL
void           ctk_icon_view_set_item_width    (CtkIconView    *icon_view,
					        gint            item_width);
CDK_AVAILABLE_IN_ALL
gint           ctk_icon_view_get_item_width    (CtkIconView    *icon_view);
CDK_AVAILABLE_IN_ALL
void           ctk_icon_view_set_spacing       (CtkIconView    *icon_view, 
		 			        gint            spacing);
CDK_AVAILABLE_IN_ALL
gint           ctk_icon_view_get_spacing       (CtkIconView    *icon_view);
CDK_AVAILABLE_IN_ALL
void           ctk_icon_view_set_row_spacing   (CtkIconView    *icon_view, 
					        gint            row_spacing);
CDK_AVAILABLE_IN_ALL
gint           ctk_icon_view_get_row_spacing   (CtkIconView    *icon_view);
CDK_AVAILABLE_IN_ALL
void           ctk_icon_view_set_column_spacing (CtkIconView    *icon_view, 
					        gint            column_spacing);
CDK_AVAILABLE_IN_ALL
gint           ctk_icon_view_get_column_spacing (CtkIconView    *icon_view);
CDK_AVAILABLE_IN_ALL
void           ctk_icon_view_set_margin        (CtkIconView    *icon_view, 
					        gint            margin);
CDK_AVAILABLE_IN_ALL
gint           ctk_icon_view_get_margin        (CtkIconView    *icon_view);
CDK_AVAILABLE_IN_ALL
void           ctk_icon_view_set_item_padding  (CtkIconView    *icon_view, 
					        gint            item_padding);
CDK_AVAILABLE_IN_ALL
gint           ctk_icon_view_get_item_padding  (CtkIconView    *icon_view);

CDK_AVAILABLE_IN_ALL
CtkTreePath *  ctk_icon_view_get_path_at_pos   (CtkIconView     *icon_view,
						gint             x,
						gint             y);
CDK_AVAILABLE_IN_ALL
gboolean       ctk_icon_view_get_item_at_pos   (CtkIconView     *icon_view,
						gint              x,
						gint              y,
						CtkTreePath     **path,
						CtkCellRenderer **cell);
CDK_AVAILABLE_IN_ALL
gboolean       ctk_icon_view_get_visible_range (CtkIconView      *icon_view,
						CtkTreePath     **start_path,
						CtkTreePath     **end_path);
CDK_AVAILABLE_IN_3_8
void           ctk_icon_view_set_activate_on_single_click (CtkIconView  *icon_view,
                                                           gboolean      single);
CDK_AVAILABLE_IN_3_8
gboolean       ctk_icon_view_get_activate_on_single_click (CtkIconView  *icon_view);

CDK_AVAILABLE_IN_ALL
void           ctk_icon_view_selected_foreach   (CtkIconView            *icon_view,
						 CtkIconViewForeachFunc  func,
						 gpointer                data);
CDK_AVAILABLE_IN_ALL
void           ctk_icon_view_set_selection_mode (CtkIconView            *icon_view,
						 CtkSelectionMode        mode);
CDK_AVAILABLE_IN_ALL
CtkSelectionMode ctk_icon_view_get_selection_mode (CtkIconView            *icon_view);
CDK_AVAILABLE_IN_ALL
void             ctk_icon_view_select_path        (CtkIconView            *icon_view,
						   CtkTreePath            *path);
CDK_AVAILABLE_IN_ALL
void             ctk_icon_view_unselect_path      (CtkIconView            *icon_view,
						   CtkTreePath            *path);
CDK_AVAILABLE_IN_ALL
gboolean         ctk_icon_view_path_is_selected   (CtkIconView            *icon_view,
						   CtkTreePath            *path);
CDK_AVAILABLE_IN_ALL
gint             ctk_icon_view_get_item_row       (CtkIconView            *icon_view,
                                                   CtkTreePath            *path);
CDK_AVAILABLE_IN_ALL
gint             ctk_icon_view_get_item_column    (CtkIconView            *icon_view,
                                                   CtkTreePath            *path);
CDK_AVAILABLE_IN_ALL
GList           *ctk_icon_view_get_selected_items (CtkIconView            *icon_view);
CDK_AVAILABLE_IN_ALL
void             ctk_icon_view_select_all         (CtkIconView            *icon_view);
CDK_AVAILABLE_IN_ALL
void             ctk_icon_view_unselect_all       (CtkIconView            *icon_view);
CDK_AVAILABLE_IN_ALL
void             ctk_icon_view_item_activated     (CtkIconView            *icon_view,
						   CtkTreePath            *path);
CDK_AVAILABLE_IN_ALL
void             ctk_icon_view_set_cursor         (CtkIconView            *icon_view,
						   CtkTreePath            *path,
						   CtkCellRenderer        *cell,
						   gboolean                start_editing);
CDK_AVAILABLE_IN_ALL
gboolean         ctk_icon_view_get_cursor         (CtkIconView            *icon_view,
						   CtkTreePath           **path,
						   CtkCellRenderer       **cell);
CDK_AVAILABLE_IN_ALL
void             ctk_icon_view_scroll_to_path     (CtkIconView            *icon_view,
                                                   CtkTreePath            *path,
						   gboolean                use_align,
						   gfloat                  row_align,
                                                   gfloat                  col_align);

/* Drag-and-Drop support */
CDK_AVAILABLE_IN_ALL
void                   ctk_icon_view_enable_model_drag_source (CtkIconView              *icon_view,
							       CdkModifierType           start_button_mask,
							       const CtkTargetEntry     *targets,
							       gint                      n_targets,
							       CdkDragAction             actions);
CDK_AVAILABLE_IN_ALL
void                   ctk_icon_view_enable_model_drag_dest   (CtkIconView              *icon_view,
							       const CtkTargetEntry     *targets,
							       gint                      n_targets,
							       CdkDragAction             actions);
CDK_AVAILABLE_IN_ALL
void                   ctk_icon_view_unset_model_drag_source  (CtkIconView              *icon_view);
CDK_AVAILABLE_IN_ALL
void                   ctk_icon_view_unset_model_drag_dest    (CtkIconView              *icon_view);
CDK_AVAILABLE_IN_ALL
void                   ctk_icon_view_set_reorderable          (CtkIconView              *icon_view,
							       gboolean                  reorderable);
CDK_AVAILABLE_IN_ALL
gboolean               ctk_icon_view_get_reorderable          (CtkIconView              *icon_view);


/* These are useful to implement your own custom stuff. */
CDK_AVAILABLE_IN_ALL
void                   ctk_icon_view_set_drag_dest_item       (CtkIconView              *icon_view,
							       CtkTreePath              *path,
							       CtkIconViewDropPosition   pos);
CDK_AVAILABLE_IN_ALL
void                   ctk_icon_view_get_drag_dest_item       (CtkIconView              *icon_view,
							       CtkTreePath             **path,
							       CtkIconViewDropPosition  *pos);
CDK_AVAILABLE_IN_ALL
gboolean               ctk_icon_view_get_dest_item_at_pos     (CtkIconView              *icon_view,
							       gint                      drag_x,
							       gint                      drag_y,
							       CtkTreePath             **path,
							       CtkIconViewDropPosition  *pos);
CDK_AVAILABLE_IN_ALL
cairo_surface_t       *ctk_icon_view_create_drag_icon         (CtkIconView              *icon_view,
							       CtkTreePath              *path);

CDK_AVAILABLE_IN_ALL
void    ctk_icon_view_convert_widget_to_bin_window_coords     (CtkIconView *icon_view,
                                                               gint         wx,
                                                               gint         wy,
                                                               gint        *bx,
                                                               gint        *by);
CDK_AVAILABLE_IN_3_6
gboolean ctk_icon_view_get_cell_rect                          (CtkIconView     *icon_view,
							       CtkTreePath     *path,
							       CtkCellRenderer *cell,
							       CdkRectangle    *rect);


CDK_AVAILABLE_IN_ALL
void    ctk_icon_view_set_tooltip_item                        (CtkIconView     *icon_view,
                                                               CtkTooltip      *tooltip,
                                                               CtkTreePath     *path);
CDK_AVAILABLE_IN_ALL
void    ctk_icon_view_set_tooltip_cell                        (CtkIconView     *icon_view,
                                                               CtkTooltip      *tooltip,
                                                               CtkTreePath     *path,
                                                               CtkCellRenderer *cell);
CDK_AVAILABLE_IN_ALL
gboolean ctk_icon_view_get_tooltip_context                    (CtkIconView       *icon_view,
                                                               gint              *x,
                                                               gint              *y,
                                                               gboolean           keyboard_tip,
                                                               CtkTreeModel     **model,
                                                               CtkTreePath      **path,
                                                               CtkTreeIter       *iter);
CDK_AVAILABLE_IN_ALL
void     ctk_icon_view_set_tooltip_column                     (CtkIconView       *icon_view,
                                                               gint               column);
CDK_AVAILABLE_IN_ALL
gint     ctk_icon_view_get_tooltip_column                     (CtkIconView       *icon_view);


G_END_DECLS

#endif /* __CTK_ICON_VIEW_H__ */
