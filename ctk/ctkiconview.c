/* ctkiconview.c
 * Copyright (C) 2002, 2004  Anders Carlsson <andersca@gnu.org>
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

#include "config.h"

#include <string.h>

#include "ctkiconview.h"
#include "ctkiconviewprivate.h"

#include "ctkadjustmentprivate.h"
#include "ctkcelllayout.h"
#include "ctkcellrenderer.h"
#include "ctkcellareabox.h"
#include "ctkcellareacontext.h"
#include "ctkcellrenderertext.h"
#include "ctkcellrendererpixbuf.h"
#include "ctkorientable.h"
#include "ctkmarshalers.h"
#include "ctkbindings.h"
#include "ctkdnd.h"
#include "ctkmain.h"
#include "ctkintl.h"
#include "ctkaccessible.h"
#include "ctkwindow.h"
#include "ctkentry.h"
#include "ctkcombobox.h"
#include "ctkscrollable.h"
#include "ctksizerequest.h"
#include "ctktreednd.h"
#include "ctktypebuiltins.h"
#include "ctkprivate.h"
#include "ctkcssnodeprivate.h"
#include "ctkwidgetprivate.h"
#include "ctkstylecontextprivate.h"
#include "a11y/ctkiconviewaccessibleprivate.h"

/**
 * SECTION:ctkiconview
 * @title: CtkIconView
 * @short_description: A widget which displays a list of icons in a grid
 *
 * #CtkIconView provides an alternative view on a #CtkTreeModel.
 * It displays the model as a grid of icons with labels. Like
 * #CtkTreeView, it allows to select one or multiple items
 * (depending on the selection mode, see ctk_icon_view_set_selection_mode()).
 * In addition to selection with the arrow keys, #CtkIconView supports
 * rubberband selection, which is controlled by dragging the pointer.
 *
 * Note that if the tree model is backed by an actual tree store (as
 * opposed to a flat list where the mapping to icons is obvious),
 * #CtkIconView will only display the first level of the tree and
 * ignore the tree’s branches.
 *
 * # CSS nodes
 *
 * |[<!-- language="plain" -->
 * iconview.view
 * ╰── [rubberband]
 * ]|
 *
 * CtkIconView has a single CSS node with name iconview and style class .view.
 * For rubberband selection, a subnode with name rubberband is used.
 */

#define SCROLL_EDGE_SIZE 15

typedef struct _CtkIconViewChild CtkIconViewChild;
struct _CtkIconViewChild
{
  CtkWidget    *widget;
  CdkRectangle  area;
};

/* Signals */
enum
{
  ITEM_ACTIVATED,
  SELECTION_CHANGED,
  SELECT_ALL,
  UNSELECT_ALL,
  SELECT_CURSOR_ITEM,
  TOGGLE_CURSOR_ITEM,
  MOVE_CURSOR,
  ACTIVATE_CURSOR_ITEM,
  LAST_SIGNAL
};

/* Properties */
enum
{
  PROP_0,
  PROP_PIXBUF_COLUMN,
  PROP_TEXT_COLUMN,
  PROP_MARKUP_COLUMN,
  PROP_SELECTION_MODE,
  PROP_ITEM_ORIENTATION,
  PROP_MODEL,
  PROP_COLUMNS,
  PROP_ITEM_WIDTH,
  PROP_SPACING,
  PROP_ROW_SPACING,
  PROP_COLUMN_SPACING,
  PROP_MARGIN,
  PROP_REORDERABLE,
  PROP_TOOLTIP_COLUMN,
  PROP_ITEM_PADDING,
  PROP_CELL_AREA,
  PROP_HADJUSTMENT,
  PROP_VADJUSTMENT,
  PROP_HSCROLL_POLICY,
  PROP_VSCROLL_POLICY,
  PROP_ACTIVATE_ON_SINGLE_CLICK
};

/* GObject vfuncs */
static void             ctk_icon_view_cell_layout_init          (CtkCellLayoutIface *iface);
static void             ctk_icon_view_dispose                   (GObject            *object);
static void             ctk_icon_view_constructed               (GObject            *object);
static void             ctk_icon_view_set_property              (GObject            *object,
								 guint               prop_id,
								 const GValue       *value,
								 GParamSpec         *pspec);
static void             ctk_icon_view_get_property              (GObject            *object,
								 guint               prop_id,
								 GValue             *value,
								 GParamSpec         *pspec);
/* CtkWidget vfuncs */
static void             ctk_icon_view_destroy                   (CtkWidget          *widget);
static void             ctk_icon_view_realize                   (CtkWidget          *widget);
static void             ctk_icon_view_unrealize                 (CtkWidget          *widget);
static CtkSizeRequestMode ctk_icon_view_get_request_mode        (CtkWidget          *widget);
static void             ctk_icon_view_get_preferred_width       (CtkWidget          *widget,
								 gint               *minimum,
								 gint               *natural);
static void             ctk_icon_view_get_preferred_width_for_height
                                                                (CtkWidget          *widget,
                                                                 gint                height,
								 gint               *minimum,
								 gint               *natural);
static void             ctk_icon_view_get_preferred_height      (CtkWidget          *widget,
								 gint               *minimum,
								 gint               *natural);
static void             ctk_icon_view_get_preferred_height_for_width
                                                                (CtkWidget          *widget,
                                                                 gint                width,
								 gint               *minimum,
								 gint               *natural);
static void             ctk_icon_view_size_allocate             (CtkWidget          *widget,
								 CtkAllocation      *allocation);
static gboolean         ctk_icon_view_draw                      (CtkWidget          *widget,
                                                                 cairo_t            *cr);
static gboolean         ctk_icon_view_motion                    (CtkWidget          *widget,
								 CdkEventMotion     *event);
static gboolean         ctk_icon_view_leave                     (CtkWidget          *widget,
								 CdkEventCrossing   *event);
static gboolean         ctk_icon_view_button_press              (CtkWidget          *widget,
								 CdkEventButton     *event);
static gboolean         ctk_icon_view_button_release            (CtkWidget          *widget,
								 CdkEventButton     *event);
static gboolean         ctk_icon_view_key_press                 (CtkWidget          *widget,
								 CdkEventKey        *event);
static gboolean         ctk_icon_view_key_release               (CtkWidget          *widget,
								 CdkEventKey        *event);


/* CtkContainer vfuncs */
static void             ctk_icon_view_remove                    (CtkContainer       *container,
								 CtkWidget          *widget);
static void             ctk_icon_view_forall                    (CtkContainer       *container,
								 gboolean            include_internals,
								 CtkCallback         callback,
								 gpointer            callback_data);

/* CtkIconView vfuncs */
static void             ctk_icon_view_real_select_all           (CtkIconView        *icon_view);
static void             ctk_icon_view_real_unselect_all         (CtkIconView        *icon_view);
static void             ctk_icon_view_real_select_cursor_item   (CtkIconView        *icon_view);
static void             ctk_icon_view_real_toggle_cursor_item   (CtkIconView        *icon_view);
static gboolean         ctk_icon_view_real_activate_cursor_item (CtkIconView        *icon_view);

 /* Internal functions */
static void                 ctk_icon_view_set_hadjustment_values         (CtkIconView            *icon_view);
static void                 ctk_icon_view_set_vadjustment_values         (CtkIconView            *icon_view);
static void                 ctk_icon_view_set_hadjustment                (CtkIconView            *icon_view,
                                                                          CtkAdjustment          *adjustment);
static void                 ctk_icon_view_set_vadjustment                (CtkIconView            *icon_view,
                                                                          CtkAdjustment          *adjustment);
static void                 ctk_icon_view_adjustment_changed             (CtkAdjustment          *adjustment,
									  CtkIconView            *icon_view);
static void                 ctk_icon_view_layout                         (CtkIconView            *icon_view);
static void                 ctk_icon_view_paint_item                     (CtkIconView            *icon_view,
									  cairo_t                *cr,
									  CtkIconViewItem        *item,
									  gint                    x,
									  gint                    y,
									  gboolean                draw_focus);
static void                 ctk_icon_view_paint_rubberband               (CtkIconView            *icon_view,
								          cairo_t                *cr);
static void                 ctk_icon_view_queue_draw_path                (CtkIconView *icon_view,
									  CtkTreePath *path);
static void                 ctk_icon_view_queue_draw_item                (CtkIconView            *icon_view,
									  CtkIconViewItem        *item);
static void                 ctk_icon_view_start_rubberbanding            (CtkIconView            *icon_view,
                                                                          CdkDevice              *device,
									  gint                    x,
									  gint                    y);
static void                 ctk_icon_view_stop_rubberbanding             (CtkIconView            *icon_view);
static void                 ctk_icon_view_update_rubberband_selection    (CtkIconView            *icon_view);
static gboolean             ctk_icon_view_item_hit_test                  (CtkIconView            *icon_view,
									  CtkIconViewItem        *item,
									  gint                    x,
									  gint                    y,
									  gint                    width,
									  gint                    height);
static gboolean             ctk_icon_view_unselect_all_internal          (CtkIconView            *icon_view);
static void                 ctk_icon_view_update_rubberband              (gpointer                data);
static void                 ctk_icon_view_item_invalidate_size           (CtkIconViewItem        *item);
static void                 ctk_icon_view_invalidate_sizes               (CtkIconView            *icon_view);
static void                 ctk_icon_view_add_move_binding               (CtkBindingSet          *binding_set,
									  guint                   keyval,
									  guint                   modmask,
									  CtkMovementStep         step,
									  gint                    count);
static gboolean             ctk_icon_view_real_move_cursor               (CtkIconView            *icon_view,
									  CtkMovementStep         step,
									  gint                    count);
static void                 ctk_icon_view_move_cursor_up_down            (CtkIconView            *icon_view,
									  gint                    count);
static void                 ctk_icon_view_move_cursor_page_up_down       (CtkIconView            *icon_view,
									  gint                    count);
static void                 ctk_icon_view_move_cursor_left_right         (CtkIconView            *icon_view,
									  gint                    count);
static void                 ctk_icon_view_move_cursor_start_end          (CtkIconView            *icon_view,
									  gint                    count);
static void                 ctk_icon_view_scroll_to_item                 (CtkIconView            *icon_view,
									  CtkIconViewItem        *item);
static gboolean             ctk_icon_view_select_all_between             (CtkIconView            *icon_view,
									  CtkIconViewItem        *anchor,
									  CtkIconViewItem        *cursor);

static void                 ctk_icon_view_ensure_cell_area               (CtkIconView            *icon_view,
                                                                          CtkCellArea            *cell_area);

static CtkCellArea         *ctk_icon_view_cell_layout_get_area           (CtkCellLayout          *layout);

static void                 ctk_icon_view_item_selected_changed          (CtkIconView            *icon_view,
		                                                          CtkIconViewItem        *item);

static void                 ctk_icon_view_add_editable                   (CtkCellArea            *area,
									  CtkCellRenderer        *renderer,
									  CtkCellEditable        *editable,
									  CdkRectangle           *cell_area,
									  const gchar            *path,
									  CtkIconView            *icon_view);
static void                 ctk_icon_view_remove_editable                (CtkCellArea            *area,
									  CtkCellRenderer        *renderer,
									  CtkCellEditable        *editable,
									  CtkIconView            *icon_view);
static void                 update_text_cell                             (CtkIconView            *icon_view);
static void                 update_pixbuf_cell                           (CtkIconView            *icon_view);

/* Source side drag signals */
static void ctk_icon_view_drag_begin       (CtkWidget        *widget,
                                            CdkDragContext   *context);
static void ctk_icon_view_drag_end         (CtkWidget        *widget,
                                            CdkDragContext   *context);
static void ctk_icon_view_drag_data_get    (CtkWidget        *widget,
                                            CdkDragContext   *context,
                                            CtkSelectionData *selection_data,
                                            guint             info,
                                            guint             time);
static void ctk_icon_view_drag_data_delete (CtkWidget        *widget,
                                            CdkDragContext   *context);

/* Target side drag signals */
static void     ctk_icon_view_drag_leave         (CtkWidget        *widget,
                                                  CdkDragContext   *context,
                                                  guint             time);
static gboolean ctk_icon_view_drag_motion        (CtkWidget        *widget,
                                                  CdkDragContext   *context,
                                                  gint              x,
                                                  gint              y,
                                                  guint             time);
static gboolean ctk_icon_view_drag_drop          (CtkWidget        *widget,
                                                  CdkDragContext   *context,
                                                  gint              x,
                                                  gint              y,
                                                  guint             time);
static void     ctk_icon_view_drag_data_received (CtkWidget        *widget,
                                                  CdkDragContext   *context,
                                                  gint              x,
                                                  gint              y,
                                                  CtkSelectionData *selection_data,
                                                  guint             info,
                                                  guint             time);
static gboolean ctk_icon_view_maybe_begin_drag   (CtkIconView             *icon_view,
					   	  CdkEventMotion          *event);

static void     remove_scroll_timeout            (CtkIconView *icon_view);

/* CtkBuildable */
static CtkBuildableIface *parent_buildable_iface;
static void     ctk_icon_view_buildable_init             (CtkBuildableIface *iface);
static gboolean ctk_icon_view_buildable_custom_tag_start (CtkBuildable  *buildable,
							  CtkBuilder    *builder,
							  GObject       *child,
							  const gchar   *tagname,
							  GMarkupParser *parser,
							  gpointer      *data);
static void     ctk_icon_view_buildable_custom_tag_end   (CtkBuildable  *buildable,
							  CtkBuilder    *builder,
							  GObject       *child,
							  const gchar   *tagname,
							  gpointer      *data);

static guint icon_view_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_CODE (CtkIconView, ctk_icon_view, CTK_TYPE_CONTAINER,
                         G_ADD_PRIVATE (CtkIconView)
			 G_IMPLEMENT_INTERFACE (CTK_TYPE_CELL_LAYOUT,
						ctk_icon_view_cell_layout_init)
			 G_IMPLEMENT_INTERFACE (CTK_TYPE_BUILDABLE,
						ctk_icon_view_buildable_init)
			 G_IMPLEMENT_INTERFACE (CTK_TYPE_SCROLLABLE, NULL))

static void
ctk_icon_view_class_init (CtkIconViewClass *klass)
{
  GObjectClass *gobject_class;
  CtkWidgetClass *widget_class;
  CtkContainerClass *container_class;
  CtkBindingSet *binding_set;
  
  binding_set = ctk_binding_set_by_class (klass);

  gobject_class = (GObjectClass *) klass;
  widget_class = (CtkWidgetClass *) klass;
  container_class = (CtkContainerClass *) klass;

  gobject_class->constructed = ctk_icon_view_constructed;
  gobject_class->dispose = ctk_icon_view_dispose;
  gobject_class->set_property = ctk_icon_view_set_property;
  gobject_class->get_property = ctk_icon_view_get_property;

  widget_class->destroy = ctk_icon_view_destroy;
  widget_class->realize = ctk_icon_view_realize;
  widget_class->unrealize = ctk_icon_view_unrealize;
  widget_class->get_request_mode = ctk_icon_view_get_request_mode;
  widget_class->get_preferred_width = ctk_icon_view_get_preferred_width;
  widget_class->get_preferred_height = ctk_icon_view_get_preferred_height;
  widget_class->get_preferred_width_for_height = ctk_icon_view_get_preferred_width_for_height;
  widget_class->get_preferred_height_for_width = ctk_icon_view_get_preferred_height_for_width;
  widget_class->size_allocate = ctk_icon_view_size_allocate;
  widget_class->draw = ctk_icon_view_draw;
  widget_class->motion_notify_event = ctk_icon_view_motion;
  widget_class->leave_notify_event = ctk_icon_view_leave;
  widget_class->button_press_event = ctk_icon_view_button_press;
  widget_class->button_release_event = ctk_icon_view_button_release;
  widget_class->key_press_event = ctk_icon_view_key_press;
  widget_class->key_release_event = ctk_icon_view_key_release;
  widget_class->drag_begin = ctk_icon_view_drag_begin;
  widget_class->drag_end = ctk_icon_view_drag_end;
  widget_class->drag_data_get = ctk_icon_view_drag_data_get;
  widget_class->drag_data_delete = ctk_icon_view_drag_data_delete;
  widget_class->drag_leave = ctk_icon_view_drag_leave;
  widget_class->drag_motion = ctk_icon_view_drag_motion;
  widget_class->drag_drop = ctk_icon_view_drag_drop;
  widget_class->drag_data_received = ctk_icon_view_drag_data_received;

  container_class->remove = ctk_icon_view_remove;
  container_class->forall = ctk_icon_view_forall;

  klass->select_all = ctk_icon_view_real_select_all;
  klass->unselect_all = ctk_icon_view_real_unselect_all;
  klass->select_cursor_item = ctk_icon_view_real_select_cursor_item;
  klass->toggle_cursor_item = ctk_icon_view_real_toggle_cursor_item;
  klass->activate_cursor_item = ctk_icon_view_real_activate_cursor_item;  
  klass->move_cursor = ctk_icon_view_real_move_cursor;
  
  /* Properties */
  /**
   * CtkIconView:selection-mode:
   * 
   * The ::selection-mode property specifies the selection mode of
   * icon view. If the mode is #CTK_SELECTION_MULTIPLE, rubberband selection
   * is enabled, for the other modes, only keyboard selection is possible.
   *
   * Since: 2.6
   */
  g_object_class_install_property (gobject_class,
				   PROP_SELECTION_MODE,
				   g_param_spec_enum ("selection-mode",
						      P_("Selection mode"),
						      P_("The selection mode"),
						      CTK_TYPE_SELECTION_MODE,
						      CTK_SELECTION_SINGLE,
						      CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkIconView:pixbuf-column:
   *
   * The ::pixbuf-column property contains the number of the model column
   * containing the pixbufs which are displayed. The pixbuf column must be 
   * of type #GDK_TYPE_PIXBUF. Setting this property to -1 turns off the
   * display of pixbufs.
   *
   * Since: 2.6
   */
  g_object_class_install_property (gobject_class,
				   PROP_PIXBUF_COLUMN,
				   g_param_spec_int ("pixbuf-column",
						     P_("Pixbuf column"),
						     P_("Model column used to retrieve the icon pixbuf from"),
						     -1, G_MAXINT, -1,
						     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkIconView:text-column:
   *
   * The ::text-column property contains the number of the model column
   * containing the texts which are displayed. The text column must be 
   * of type #G_TYPE_STRING. If this property and the :markup-column 
   * property are both set to -1, no texts are displayed.   
   *
   * Since: 2.6
   */
  g_object_class_install_property (gobject_class,
				   PROP_TEXT_COLUMN,
				   g_param_spec_int ("text-column",
						     P_("Text column"),
						     P_("Model column used to retrieve the text from"),
						     -1, G_MAXINT, -1,
						     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  
  /**
   * CtkIconView:markup-column:
   *
   * The ::markup-column property contains the number of the model column
   * containing markup information to be displayed. The markup column must be 
   * of type #G_TYPE_STRING. If this property and the :text-column property 
   * are both set to column numbers, it overrides the text column.
   * If both are set to -1, no texts are displayed.   
   *
   * Since: 2.6
   */
  g_object_class_install_property (gobject_class,
				   PROP_MARKUP_COLUMN,
				   g_param_spec_int ("markup-column",
						     P_("Markup column"),
						     P_("Model column used to retrieve the text if using Pango markup"),
						     -1, G_MAXINT, -1,
						     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
  
  g_object_class_install_property (gobject_class,
                                   PROP_MODEL,
                                   g_param_spec_object ("model",
							P_("Icon View Model"),
							P_("The model for the icon view"),
							CTK_TYPE_TREE_MODEL,
							CTK_PARAM_READWRITE));
  
  /**
   * CtkIconView:columns:
   *
   * The columns property contains the number of the columns in which the
   * items should be displayed. If it is -1, the number of columns will
   * be chosen automatically to fill the available area.
   *
   * Since: 2.6
   */
  g_object_class_install_property (gobject_class,
				   PROP_COLUMNS,
				   g_param_spec_int ("columns",
						     P_("Number of columns"),
						     P_("Number of columns to display"),
						     -1, G_MAXINT, -1,
						     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
  

  /**
   * CtkIconView:item-width:
   *
   * The item-width property specifies the width to use for each item. 
   * If it is set to -1, the icon view will automatically determine a 
   * suitable item size.
   *
   * Since: 2.6
   */
  g_object_class_install_property (gobject_class,
				   PROP_ITEM_WIDTH,
				   g_param_spec_int ("item-width",
						     P_("Width for each item"),
						     P_("The width used for each item"),
						     -1, G_MAXINT, -1,
						     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkIconView:spacing:
   *
   * The spacing property specifies the space which is inserted between
   * the cells (i.e. the icon and the text) of an item.
   *
   * Since: 2.6
   */
  g_object_class_install_property (gobject_class,
                                   PROP_SPACING,
                                   g_param_spec_int ("spacing",
						     P_("Spacing"),
						     P_("Space which is inserted between cells of an item"),
						     0, G_MAXINT, 0,
						     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkIconView:row-spacing:
   *
   * The row-spacing property specifies the space which is inserted between
   * the rows of the icon view.
   *
   * Since: 2.6
   */
  g_object_class_install_property (gobject_class,
                                   PROP_ROW_SPACING,
                                   g_param_spec_int ("row-spacing",
						     P_("Row Spacing"),
						     P_("Space which is inserted between grid rows"),
						     0, G_MAXINT, 6,
						     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkIconView:column-spacing:
   *
   * The column-spacing property specifies the space which is inserted between
   * the columns of the icon view.
   *
   * Since: 2.6
   */
  g_object_class_install_property (gobject_class,
                                   PROP_COLUMN_SPACING,
                                   g_param_spec_int ("column-spacing",
						     P_("Column Spacing"),
						     P_("Space which is inserted between grid columns"),
						     0, G_MAXINT, 6,
						     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkIconView:margin:
   *
   * The margin property specifies the space which is inserted 
   * at the edges of the icon view.
   *
   * Since: 2.6
   */
  g_object_class_install_property (gobject_class,
                                   PROP_MARGIN,
                                   g_param_spec_int ("margin",
						     P_("Margin"),
						     P_("Space which is inserted at the edges of the icon view"),
						     0, G_MAXINT, 6,
						     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkIconView:item-orientation:
   *
   * The item-orientation property specifies how the cells (i.e. the icon and
   * the text) of the item are positioned relative to each other.
   *
   * Since: 2.6
   */
  g_object_class_install_property (gobject_class,
				   PROP_ITEM_ORIENTATION,
				   g_param_spec_enum ("item-orientation",
						      P_("Item Orientation"),
						      P_("How the text and icon of each item are positioned relative to each other"),
						      CTK_TYPE_ORIENTATION,
						      CTK_ORIENTATION_VERTICAL,
						      CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkIconView:reorderable:
   *
   * The reorderable property specifies if the items can be reordered
   * by DND.
   *
   * Since: 2.8
   */
  g_object_class_install_property (gobject_class,
                                   PROP_REORDERABLE,
                                   g_param_spec_boolean ("reorderable",
							 P_("Reorderable"),
							 P_("View is reorderable"),
							 FALSE,
							 G_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

    g_object_class_install_property (gobject_class,
                                     PROP_TOOLTIP_COLUMN,
                                     g_param_spec_int ("tooltip-column",
                                                       P_("Tooltip Column"),
                                                       P_("The column in the model containing the tooltip texts for the items"),
                                                       -1,
                                                       G_MAXINT,
                                                       -1,
                                                       CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkIconView:item-padding:
   *
   * The item-padding property specifies the padding around each
   * of the icon view's item.
   *
   * Since: 2.18
   */
  g_object_class_install_property (gobject_class,
                                   PROP_ITEM_PADDING,
                                   g_param_spec_int ("item-padding",
						     P_("Item Padding"),
						     P_("Padding around icon view items"),
						     0, G_MAXINT, 6,
						     CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkIconView:cell-area:
   *
   * The #CtkCellArea used to layout cell renderers for this view.
   *
   * If no area is specified when creating the icon view with ctk_icon_view_new_with_area() 
   * a #CtkCellAreaBox will be used.
   *
   * Since: 3.0
   */
  g_object_class_install_property (gobject_class,
				   PROP_CELL_AREA,
				   g_param_spec_object ("cell-area",
							P_("Cell Area"),
							P_("The CtkCellArea used to layout cells"),
							CTK_TYPE_CELL_AREA,
							CTK_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  /**
   * CtkIconView:activate-on-single-click:
   *
   * The activate-on-single-click property specifies whether the "item-activated" signal
   * will be emitted after a single click.
   *
   * Since: 3.8
   */
  g_object_class_install_property (gobject_class,
                                   PROP_ACTIVATE_ON_SINGLE_CLICK,
                                   g_param_spec_boolean ("activate-on-single-click",
							 P_("Activate on Single Click"),
							 P_("Activate row on a single click"),
							 FALSE,
							 CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /* Scrollable interface properties */
  g_object_class_override_property (gobject_class, PROP_HADJUSTMENT,    "hadjustment");
  g_object_class_override_property (gobject_class, PROP_VADJUSTMENT,    "vadjustment");
  g_object_class_override_property (gobject_class, PROP_HSCROLL_POLICY, "hscroll-policy");
  g_object_class_override_property (gobject_class, PROP_VSCROLL_POLICY, "vscroll-policy");

  /* Style properties */
  /**
   * CtkIconView:selection-box-color:
   *
   * The color of the selection box.
   *
   * Deprecated: 3.20: The color of the selection box is determined by CSS;
   *     the value of this style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_boxed ("selection-box-color",
                                                               P_("Selection Box Color"),
                                                               P_("Color of the selection box"),
                                                               g_type_from_name ("CdkColor"),
                                                               CTK_PARAM_READABLE|G_PARAM_DEPRECATED));


  /**
   * CtkIconView:selection-box-alpha:
   *
   * The opacity of the selection box.
   *
   * Deprecated: 3.20: The opacity of the selection box is determined by CSS;
   *     the value of this style property is ignored.
   */
  ctk_widget_class_install_style_property (widget_class,
                                           g_param_spec_uchar ("selection-box-alpha",
                                                               P_("Selection Box Alpha"),
                                                               P_("Opacity of the selection box"),
                                                               0, 0xff,
                                                               0x40,
                                                               CTK_PARAM_READABLE|G_PARAM_DEPRECATED));

  /* Signals */
  /**
   * CtkIconView::item-activated:
   * @iconview: the object on which the signal is emitted
   * @path: the #CtkTreePath for the activated item
   *
   * The ::item-activated signal is emitted when the method
   * ctk_icon_view_item_activated() is called, when the user double
   * clicks an item with the "activate-on-single-click" property set
   * to %FALSE, or when the user single clicks an item when the
   * "activate-on-single-click" property set to %TRUE. It is also
   * emitted when a non-editable item is selected and one of the keys:
   * Space, Return or Enter is pressed.
   */
  icon_view_signals[ITEM_ACTIVATED] =
    g_signal_new (I_("item-activated"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (CtkIconViewClass, item_activated),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1,
		  CTK_TYPE_TREE_PATH);

  /**
   * CtkIconView::selection-changed:
   * @iconview: the object on which the signal is emitted
   *
   * The ::selection-changed signal is emitted when the selection
   * (i.e. the set of selected items) changes.
   */
  icon_view_signals[SELECTION_CHANGED] =
    g_signal_new (I_("selection-changed"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (CtkIconViewClass, selection_changed),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);
  
  /**
   * CtkIconView::select-all:
   * @iconview: the object on which the signal is emitted
   *
   * A [keybinding signal][CtkBindingSignal]
   * which gets emitted when the user selects all items.
   *
   * Applications should not connect to it, but may emit it with
   * g_signal_emit_by_name() if they need to control selection
   * programmatically.
   * 
   * The default binding for this signal is Ctrl-a.
   */
  icon_view_signals[SELECT_ALL] =
    g_signal_new (I_("select-all"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkIconViewClass, select_all),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);
  
  /**
   * CtkIconView::unselect-all:
   * @iconview: the object on which the signal is emitted
   *
   * A [keybinding signal][CtkBindingSignal]
   * which gets emitted when the user unselects all items.
   *
   * Applications should not connect to it, but may emit it with
   * g_signal_emit_by_name() if they need to control selection
   * programmatically.
   * 
   * The default binding for this signal is Ctrl-Shift-a. 
   */
  icon_view_signals[UNSELECT_ALL] =
    g_signal_new (I_("unselect-all"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkIconViewClass, unselect_all),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * CtkIconView::select-cursor-item:
   * @iconview: the object on which the signal is emitted
   *
   * A [keybinding signal][CtkBindingSignal]
   * which gets emitted when the user selects the item that is currently
   * focused.
   *
   * Applications should not connect to it, but may emit it with
   * g_signal_emit_by_name() if they need to control selection
   * programmatically.
   * 
   * There is no default binding for this signal.
   */
  icon_view_signals[SELECT_CURSOR_ITEM] =
    g_signal_new (I_("select-cursor-item"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkIconViewClass, select_cursor_item),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * CtkIconView::toggle-cursor-item:
   * @iconview: the object on which the signal is emitted
   *
   * A [keybinding signal][CtkBindingSignal]
   * which gets emitted when the user toggles whether the currently
   * focused item is selected or not. The exact effect of this 
   * depend on the selection mode.
   *
   * Applications should not connect to it, but may emit it with
   * g_signal_emit_by_name() if they need to control selection
   * programmatically.
   * 
   * There is no default binding for this signal is Ctrl-Space.
   */
  icon_view_signals[TOGGLE_CURSOR_ITEM] =
    g_signal_new (I_("toggle-cursor-item"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkIconViewClass, toggle_cursor_item),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * CtkIconView::activate-cursor-item:
   * @iconview: the object on which the signal is emitted
   *
   * A [keybinding signal][CtkBindingSignal]
   * which gets emitted when the user activates the currently 
   * focused item. 
   *
   * Applications should not connect to it, but may emit it with
   * g_signal_emit_by_name() if they need to control activation
   * programmatically.
   * 
   * The default bindings for this signal are Space, Return and Enter.
   */
  icon_view_signals[ACTIVATE_CURSOR_ITEM] =
    g_signal_new (I_("activate-cursor-item"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkIconViewClass, activate_cursor_item),
		  NULL, NULL,
		  _ctk_marshal_BOOLEAN__VOID,
		  G_TYPE_BOOLEAN, 0);
  g_signal_set_va_marshaller (icon_view_signals[ACTIVATE_CURSOR_ITEM],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__VOIDv);
  
  /**
   * CtkIconView::move-cursor:
   * @iconview: the object which received the signal
   * @step: the granularity of the move, as a #CtkMovementStep
   * @count: the number of @step units to move
   *
   * The ::move-cursor signal is a
   * [keybinding signal][CtkBindingSignal]
   * which gets emitted when the user initiates a cursor movement.
   *
   * Applications should not connect to it, but may emit it with
   * g_signal_emit_by_name() if they need to control the cursor
   * programmatically.
   *
   * The default bindings for this signal include
   * - Arrow keys which move by individual steps
   * - Home/End keys which move to the first/last item
   * - PageUp/PageDown which move by "pages"
   * All of these will extend the selection when combined with
   * the Shift modifier.
   */
  icon_view_signals[MOVE_CURSOR] =
    g_signal_new (I_("move-cursor"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (CtkIconViewClass, move_cursor),
		  NULL, NULL,
		  _ctk_marshal_BOOLEAN__ENUM_INT,
		  G_TYPE_BOOLEAN, 2,
		  CTK_TYPE_MOVEMENT_STEP,
		  G_TYPE_INT);
  g_signal_set_va_marshaller (icon_view_signals[MOVE_CURSOR],
                              G_TYPE_FROM_CLASS (klass),
                              _ctk_marshal_BOOLEAN__ENUM_INTv);

  /* Key bindings */
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_a, GDK_CONTROL_MASK, 
				"select-all", 0);
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_a, GDK_CONTROL_MASK | GDK_SHIFT_MASK, 
				"unselect-all", 0);
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_space, GDK_CONTROL_MASK, 
				"toggle-cursor-item", 0);
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Space, GDK_CONTROL_MASK,
				"toggle-cursor-item", 0);

  ctk_binding_entry_add_signal (binding_set, GDK_KEY_space, 0, 
				"activate-cursor-item", 0);
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Space, 0,
				"activate-cursor-item", 0);
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_Return, 0, 
				"activate-cursor-item", 0);
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_ISO_Enter, 0, 
				"activate-cursor-item", 0);
  ctk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Enter, 0, 
				"activate-cursor-item", 0);

  ctk_icon_view_add_move_binding (binding_set, GDK_KEY_Up, 0,
				  CTK_MOVEMENT_DISPLAY_LINES, -1);
  ctk_icon_view_add_move_binding (binding_set, GDK_KEY_KP_Up, 0,
				  CTK_MOVEMENT_DISPLAY_LINES, -1);

  ctk_icon_view_add_move_binding (binding_set, GDK_KEY_Down, 0,
				  CTK_MOVEMENT_DISPLAY_LINES, 1);
  ctk_icon_view_add_move_binding (binding_set, GDK_KEY_KP_Down, 0,
				  CTK_MOVEMENT_DISPLAY_LINES, 1);

  ctk_icon_view_add_move_binding (binding_set, GDK_KEY_p, GDK_CONTROL_MASK,
				  CTK_MOVEMENT_DISPLAY_LINES, -1);

  ctk_icon_view_add_move_binding (binding_set, GDK_KEY_n, GDK_CONTROL_MASK,
				  CTK_MOVEMENT_DISPLAY_LINES, 1);

  ctk_icon_view_add_move_binding (binding_set, GDK_KEY_Home, 0,
				  CTK_MOVEMENT_BUFFER_ENDS, -1);
  ctk_icon_view_add_move_binding (binding_set, GDK_KEY_KP_Home, 0,
				  CTK_MOVEMENT_BUFFER_ENDS, -1);

  ctk_icon_view_add_move_binding (binding_set, GDK_KEY_End, 0,
				  CTK_MOVEMENT_BUFFER_ENDS, 1);
  ctk_icon_view_add_move_binding (binding_set, GDK_KEY_KP_End, 0,
				  CTK_MOVEMENT_BUFFER_ENDS, 1);

  ctk_icon_view_add_move_binding (binding_set, GDK_KEY_Page_Up, 0,
				  CTK_MOVEMENT_PAGES, -1);
  ctk_icon_view_add_move_binding (binding_set, GDK_KEY_KP_Page_Up, 0,
				  CTK_MOVEMENT_PAGES, -1);

  ctk_icon_view_add_move_binding (binding_set, GDK_KEY_Page_Down, 0,
				  CTK_MOVEMENT_PAGES, 1);
  ctk_icon_view_add_move_binding (binding_set, GDK_KEY_KP_Page_Down, 0,
				  CTK_MOVEMENT_PAGES, 1);

  ctk_icon_view_add_move_binding (binding_set, GDK_KEY_Right, 0, 
				  CTK_MOVEMENT_VISUAL_POSITIONS, 1);
  ctk_icon_view_add_move_binding (binding_set, GDK_KEY_Left, 0, 
				  CTK_MOVEMENT_VISUAL_POSITIONS, -1);

  ctk_icon_view_add_move_binding (binding_set, GDK_KEY_KP_Right, 0, 
				  CTK_MOVEMENT_VISUAL_POSITIONS, 1);
  ctk_icon_view_add_move_binding (binding_set, GDK_KEY_KP_Left, 0, 
				  CTK_MOVEMENT_VISUAL_POSITIONS, -1);

  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_ICON_VIEW_ACCESSIBLE);
  ctk_widget_class_set_css_name (widget_class, "iconview");
}

static void
ctk_icon_view_buildable_init (CtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->add_child = _ctk_cell_layout_buildable_add_child;
  iface->custom_tag_start = ctk_icon_view_buildable_custom_tag_start;
  iface->custom_tag_end = ctk_icon_view_buildable_custom_tag_end;
}

static void
ctk_icon_view_cell_layout_init (CtkCellLayoutIface *iface)
{
  iface->get_area = ctk_icon_view_cell_layout_get_area;
}

static void
ctk_icon_view_init (CtkIconView *icon_view)
{
  icon_view->priv = ctk_icon_view_get_instance_private (icon_view);

  icon_view->priv->width = 0;
  icon_view->priv->height = 0;
  icon_view->priv->selection_mode = CTK_SELECTION_SINGLE;
  icon_view->priv->pressed_button = -1;
  icon_view->priv->press_start_x = -1;
  icon_view->priv->press_start_y = -1;
  icon_view->priv->text_column = -1;
  icon_view->priv->markup_column = -1;  
  icon_view->priv->pixbuf_column = -1;
  icon_view->priv->text_cell = NULL;
  icon_view->priv->pixbuf_cell = NULL;  
  icon_view->priv->tooltip_column = -1;  

  ctk_widget_set_can_focus (CTK_WIDGET (icon_view), TRUE);

  icon_view->priv->item_orientation = CTK_ORIENTATION_VERTICAL;

  icon_view->priv->columns = -1;
  icon_view->priv->item_width = -1;
  icon_view->priv->spacing = 0;
  icon_view->priv->row_spacing = 6;
  icon_view->priv->column_spacing = 6;
  icon_view->priv->margin = 6;
  icon_view->priv->item_padding = 6;
  icon_view->priv->activate_on_single_click = FALSE;

  icon_view->priv->draw_focus = TRUE;

  icon_view->priv->row_contexts = 
    g_ptr_array_new_with_free_func ((GDestroyNotify)g_object_unref);

  ctk_style_context_add_class (ctk_widget_get_style_context (CTK_WIDGET (icon_view)),
                               CTK_STYLE_CLASS_VIEW);
}

/* GObject methods */

static void
ctk_icon_view_constructed (GObject *object)
{
  CtkIconView *icon_view = CTK_ICON_VIEW (object);

  G_OBJECT_CLASS (ctk_icon_view_parent_class)->constructed (object);

  ctk_icon_view_ensure_cell_area (icon_view, NULL);
}

static void
ctk_icon_view_dispose (GObject *object)
{
  CtkIconView *icon_view;
  CtkIconViewPrivate *priv;

  icon_view = CTK_ICON_VIEW (object);
  priv      = icon_view->priv;

  if (priv->cell_area_context)
    {
      g_object_unref (priv->cell_area_context);
      priv->cell_area_context = NULL;
    }

  if (priv->row_contexts)
    {
      g_ptr_array_free (priv->row_contexts, TRUE);
      priv->row_contexts = NULL;
    }

  if (priv->cell_area)
    {
      ctk_cell_area_stop_editing (icon_view->priv->cell_area, TRUE);

      g_signal_handler_disconnect (priv->cell_area, priv->add_editable_id);
      g_signal_handler_disconnect (priv->cell_area, priv->remove_editable_id);
      priv->add_editable_id = 0;
      priv->remove_editable_id = 0;

      g_object_unref (priv->cell_area);
      priv->cell_area = NULL;
    }

  G_OBJECT_CLASS (ctk_icon_view_parent_class)->dispose (object);
}

static void
ctk_icon_view_set_property (GObject      *object,
			    guint         prop_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
  CtkIconView *icon_view;
  CtkCellArea *area;

  icon_view = CTK_ICON_VIEW (object);

  switch (prop_id)
    {
    case PROP_SELECTION_MODE:
      ctk_icon_view_set_selection_mode (icon_view, g_value_get_enum (value));
      break;
    case PROP_PIXBUF_COLUMN:
      ctk_icon_view_set_pixbuf_column (icon_view, g_value_get_int (value));
      break;
    case PROP_TEXT_COLUMN:
      ctk_icon_view_set_text_column (icon_view, g_value_get_int (value));
      break;
    case PROP_MARKUP_COLUMN:
      ctk_icon_view_set_markup_column (icon_view, g_value_get_int (value));
      break;
    case PROP_MODEL:
      ctk_icon_view_set_model (icon_view, g_value_get_object (value));
      break;
    case PROP_ITEM_ORIENTATION:
      ctk_icon_view_set_item_orientation (icon_view, g_value_get_enum (value));
      break;
    case PROP_COLUMNS:
      ctk_icon_view_set_columns (icon_view, g_value_get_int (value));
      break;
    case PROP_ITEM_WIDTH:
      ctk_icon_view_set_item_width (icon_view, g_value_get_int (value));
      break;
    case PROP_SPACING:
      ctk_icon_view_set_spacing (icon_view, g_value_get_int (value));
      break;
    case PROP_ROW_SPACING:
      ctk_icon_view_set_row_spacing (icon_view, g_value_get_int (value));
      break;
    case PROP_COLUMN_SPACING:
      ctk_icon_view_set_column_spacing (icon_view, g_value_get_int (value));
      break;
    case PROP_MARGIN:
      ctk_icon_view_set_margin (icon_view, g_value_get_int (value));
      break;
    case PROP_REORDERABLE:
      ctk_icon_view_set_reorderable (icon_view, g_value_get_boolean (value));
      break;
      
    case PROP_TOOLTIP_COLUMN:
      ctk_icon_view_set_tooltip_column (icon_view, g_value_get_int (value));
      break;

    case PROP_ITEM_PADDING:
      ctk_icon_view_set_item_padding (icon_view, g_value_get_int (value));
      break;

    case PROP_ACTIVATE_ON_SINGLE_CLICK:
      ctk_icon_view_set_activate_on_single_click (icon_view, g_value_get_boolean (value));
      break;

    case PROP_CELL_AREA:
      /* Construct-only, can only be assigned once */
      area = g_value_get_object (value);
      if (area)
        {
          if (icon_view->priv->cell_area != NULL)
            {
              g_warning ("cell-area has already been set, ignoring construct property");
              g_object_ref_sink (area);
              g_object_unref (area);
            }
          else
            ctk_icon_view_ensure_cell_area (icon_view, area);
        }
      break;

    case PROP_HADJUSTMENT:
      ctk_icon_view_set_hadjustment (icon_view, g_value_get_object (value));
      break;
    case PROP_VADJUSTMENT:
      ctk_icon_view_set_vadjustment (icon_view, g_value_get_object (value));
      break;
    case PROP_HSCROLL_POLICY:
      if (icon_view->priv->hscroll_policy != g_value_get_enum (value))
        {
          icon_view->priv->hscroll_policy = g_value_get_enum (value);
          ctk_widget_queue_resize (CTK_WIDGET (icon_view));
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_VSCROLL_POLICY:
      if (icon_view->priv->vscroll_policy != g_value_get_enum (value))
        {
          icon_view->priv->vscroll_policy = g_value_get_enum (value);
          ctk_widget_queue_resize (CTK_WIDGET (icon_view));
          g_object_notify_by_pspec (object, pspec);
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_icon_view_get_property (GObject      *object,
			    guint         prop_id,
			    GValue       *value,
			    GParamSpec   *pspec)
{
  CtkIconView *icon_view;

  icon_view = CTK_ICON_VIEW (object);

  switch (prop_id)
    {
    case PROP_SELECTION_MODE:
      g_value_set_enum (value, icon_view->priv->selection_mode);
      break;
    case PROP_PIXBUF_COLUMN:
      g_value_set_int (value, icon_view->priv->pixbuf_column);
      break;
    case PROP_TEXT_COLUMN:
      g_value_set_int (value, icon_view->priv->text_column);
      break;
    case PROP_MARKUP_COLUMN:
      g_value_set_int (value, icon_view->priv->markup_column);
      break;
    case PROP_MODEL:
      g_value_set_object (value, icon_view->priv->model);
      break;
    case PROP_ITEM_ORIENTATION:
      g_value_set_enum (value, icon_view->priv->item_orientation);
      break;
    case PROP_COLUMNS:
      g_value_set_int (value, icon_view->priv->columns);
      break;
    case PROP_ITEM_WIDTH:
      g_value_set_int (value, icon_view->priv->item_width);
      break;
    case PROP_SPACING:
      g_value_set_int (value, icon_view->priv->spacing);
      break;
    case PROP_ROW_SPACING:
      g_value_set_int (value, icon_view->priv->row_spacing);
      break;
    case PROP_COLUMN_SPACING:
      g_value_set_int (value, icon_view->priv->column_spacing);
      break;
    case PROP_MARGIN:
      g_value_set_int (value, icon_view->priv->margin);
      break;
    case PROP_REORDERABLE:
      g_value_set_boolean (value, icon_view->priv->reorderable);
      break;
    case PROP_TOOLTIP_COLUMN:
      g_value_set_int (value, icon_view->priv->tooltip_column);
      break;

    case PROP_ITEM_PADDING:
      g_value_set_int (value, icon_view->priv->item_padding);
      break;

    case PROP_ACTIVATE_ON_SINGLE_CLICK:
      g_value_set_boolean (value, icon_view->priv->activate_on_single_click);
      break;

    case PROP_CELL_AREA:
      g_value_set_object (value, icon_view->priv->cell_area);
      break;

    case PROP_HADJUSTMENT:
      g_value_set_object (value, icon_view->priv->hadjustment);
      break;
    case PROP_VADJUSTMENT:
      g_value_set_object (value, icon_view->priv->vadjustment);
      break;
    case PROP_HSCROLL_POLICY:
      g_value_set_enum (value, icon_view->priv->hscroll_policy);
      break;
    case PROP_VSCROLL_POLICY:
      g_value_set_enum (value, icon_view->priv->vscroll_policy);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/* CtkWidget methods */
static void
ctk_icon_view_destroy (CtkWidget *widget)
{
  CtkIconView *icon_view = CTK_ICON_VIEW (widget);

  ctk_icon_view_set_model (icon_view, NULL);

  if (icon_view->priv->scroll_to_path != NULL)
    {
      ctk_tree_row_reference_free (icon_view->priv->scroll_to_path);
      icon_view->priv->scroll_to_path = NULL;
    }

  remove_scroll_timeout (icon_view);

  if (icon_view->priv->hadjustment != NULL)
    {
      g_object_unref (icon_view->priv->hadjustment);
      icon_view->priv->hadjustment = NULL;
    }

  if (icon_view->priv->vadjustment != NULL)
    {
      g_object_unref (icon_view->priv->vadjustment);
      icon_view->priv->vadjustment = NULL;
    }

  CTK_WIDGET_CLASS (ctk_icon_view_parent_class)->destroy (widget);
}

static void
ctk_icon_view_realize (CtkWidget *widget)
{
  CtkIconView *icon_view = CTK_ICON_VIEW (widget);
  CtkAllocation allocation;
  CdkWindow *window;
  CdkWindowAttr attributes;
  gint attributes_mask;

  ctk_widget_set_realized (widget, TRUE);

  ctk_widget_get_allocation (widget, &allocation);

  /* Make the main, clipping window */
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = ctk_widget_get_visual (widget);
  attributes.event_mask = GDK_VISIBILITY_NOTIFY_MASK;

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;

  window = cdk_window_new (ctk_widget_get_parent_window (widget),
                           &attributes, attributes_mask);
  ctk_widget_set_window (widget, window);
  ctk_widget_register_window (widget, window);

  ctk_widget_get_allocation (widget, &allocation);

  /* Make the window for the icon view */
  attributes.x = 0;
  attributes.y = 0;
  attributes.width = MAX (icon_view->priv->width, allocation.width);
  attributes.height = MAX (icon_view->priv->height, allocation.height);
  attributes.event_mask = (GDK_SCROLL_MASK |
                           GDK_SMOOTH_SCROLL_MASK |
                           GDK_POINTER_MOTION_MASK |
                           GDK_LEAVE_NOTIFY_MASK |
                           GDK_BUTTON_PRESS_MASK |
                           GDK_BUTTON_RELEASE_MASK |
                           GDK_KEY_PRESS_MASK |
                           GDK_KEY_RELEASE_MASK) |
    ctk_widget_get_events (widget);
  
  icon_view->priv->bin_window = cdk_window_new (window,
						&attributes, attributes_mask);
  ctk_widget_register_window (widget, icon_view->priv->bin_window);
  cdk_window_show (icon_view->priv->bin_window);
}

static void
ctk_icon_view_unrealize (CtkWidget *widget)
{
  CtkIconView *icon_view;

  icon_view = CTK_ICON_VIEW (widget);

  ctk_widget_unregister_window (widget, icon_view->priv->bin_window);
  cdk_window_destroy (icon_view->priv->bin_window);
  icon_view->priv->bin_window = NULL;

  CTK_WIDGET_CLASS (ctk_icon_view_parent_class)->unrealize (widget);
}

static gint
ctk_icon_view_get_n_items (CtkIconView *icon_view)
{
  CtkIconViewPrivate *priv = icon_view->priv;

  if (priv->model == NULL)
    return 0;

  return ctk_tree_model_iter_n_children (priv->model, NULL);
}

static void
adjust_wrap_width (CtkIconView *icon_view)
{
  if (icon_view->priv->text_cell)
    {
      gint pixbuf_width, wrap_width;

      if (icon_view->priv->items && icon_view->priv->pixbuf_cell)
        {
          ctk_cell_renderer_get_preferred_width (icon_view->priv->pixbuf_cell,
                                                 CTK_WIDGET (icon_view),
                                                 &pixbuf_width, NULL);
        }
      else
        {
          pixbuf_width = 0;
        }

      if (icon_view->priv->item_width >= 0)
        {
          if (icon_view->priv->item_orientation == CTK_ORIENTATION_VERTICAL)
            {
              wrap_width = icon_view->priv->item_width;
            }
          else
            { 
              wrap_width = icon_view->priv->item_width - pixbuf_width;
            }

          wrap_width -= 2 * icon_view->priv->item_padding * 2;
        }
      else
        {
          wrap_width = MAX (pixbuf_width * 2, 50);
        }

      if (icon_view->priv->items && icon_view->priv->pixbuf_cell)
	{
          /* Here we go with the same old guess, try the icon size and set double
           * the size of the first icon found in the list, naive but works much
           * of the time */
	  
	  wrap_width = MAX (wrap_width * 2, 50);
	}
      
      g_object_set (icon_view->priv->text_cell, "wrap-width", wrap_width, NULL);
      g_object_set (icon_view->priv->text_cell, "width", wrap_width, NULL);
    }
}

/* General notes about layout
 *
 * The icon view is layouted like this:
 *
 * +----------+  s  +----------+
 * | padding  |  p  | padding  |
 * | +------+ |  a  | +------+ |
 * | | cell | |  c  | | cell | |
 * | +------+ |  i  | +------+ |
 * |          |  n  |          |
 * +----------+  g  +----------+
 *
 * In size request and allocation code, there are 3 sizes that are used:
 * * cell size
 *   This is the size returned by ctk_cell_area_get_preferred_foo(). In places
 *   where code is interacting with the cell area and renderers this is useful.
 * * padded size
 *   This is the cell size plus the item padding on each side.
 * * spaced size
 *   This is the padded size plus the spacing. This is what’s used for most
 *   calculations because it can (ab)use the following formula:
 *   iconview_size = 2 * margin + n_items * spaced_size - spacing
 * So when reading this code and fixing my bugs where I confuse these two, be
 * aware of this distinction.
 */
static void
cell_area_get_preferred_size (CtkIconView        *icon_view,
                              CtkCellAreaContext *context,
                              CtkOrientation      orientation,
                              gint                for_size,
                              gint               *minimum,
                              gint               *natural)
{
  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      if (for_size > 0)
        ctk_cell_area_get_preferred_width_for_height (icon_view->priv->cell_area,
                                                      context,
                                                      CTK_WIDGET (icon_view),
                                                      for_size,
                                                      minimum, natural);
      else
        ctk_cell_area_get_preferred_width (icon_view->priv->cell_area,
                                           context,
                                           CTK_WIDGET (icon_view),
                                           minimum, natural);
    }
  else
    {
      if (for_size > 0)
        ctk_cell_area_get_preferred_height_for_width (icon_view->priv->cell_area,
                                                      context,
                                                      CTK_WIDGET (icon_view),
                                                      for_size,
                                                      minimum, natural);
      else
        ctk_cell_area_get_preferred_height (icon_view->priv->cell_area,
                                            context,
                                            CTK_WIDGET (icon_view),
                                            minimum, natural);
    }
}

static gboolean
ctk_icon_view_is_empty (CtkIconView *icon_view)
{
  return icon_view->priv->items == NULL;
}

static void
ctk_icon_view_get_preferred_item_size (CtkIconView    *icon_view,
                                       CtkOrientation  orientation,
                                       gint            for_size,
                                       gint           *minimum,
                                       gint           *natural)
{
  CtkIconViewPrivate *priv = icon_view->priv;
  CtkCellAreaContext *context;
  GList *items;

  g_assert (!ctk_icon_view_is_empty (icon_view));

  context = ctk_cell_area_create_context (priv->cell_area);

  for_size -= 2 * priv->item_padding;

  if (for_size > 0)
    {
      /* This is necessary for the context to work properly */
      for (items = priv->items; items; items = items->next)
        {
          CtkIconViewItem *item = items->data;

          _ctk_icon_view_set_cell_data (icon_view, item);
          cell_area_get_preferred_size (icon_view, context, 1 - orientation, -1, NULL, NULL);
        }
    }

  for (items = priv->items; items; items = items->next)
    {
      CtkIconViewItem *item = items->data;

      _ctk_icon_view_set_cell_data (icon_view, item);
      if (items == priv->items)
        adjust_wrap_width (icon_view);
      cell_area_get_preferred_size (icon_view, context, orientation, for_size, NULL, NULL);
    }

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      if (for_size > 0)
        ctk_cell_area_context_get_preferred_width_for_height (context,
                                                              for_size,
                                                              minimum, natural);
      else
        ctk_cell_area_context_get_preferred_width (context,
                                                   minimum, natural);
    }
  else
    {
      if (for_size > 0)
        ctk_cell_area_context_get_preferred_height_for_width (context,
                                                              for_size,
                                                              minimum, natural);
      else
        ctk_cell_area_context_get_preferred_height (context,
                                                    minimum, natural);
    }

  if (orientation == CTK_ORIENTATION_HORIZONTAL && priv->item_width >= 0)
    {
      if (minimum)
        *minimum = MAX (*minimum, priv->item_width);
      if (natural)
        *natural = *minimum;
    }

  if (minimum)
    *minimum = MAX (1, *minimum + 2 * priv->item_padding);
  if (natural)
    *natural = MAX (1, *natural + 2 * priv->item_padding);

  g_object_unref (context);
}

static void
ctk_icon_view_compute_n_items_for_size (CtkIconView    *icon_view,
                                        CtkOrientation  orientation,
                                        gint            size,
                                        gint           *min_items,
                                        gint           *min_item_size,
                                        gint           *max_items,
                                        gint           *max_item_size)
{
  CtkIconViewPrivate *priv = icon_view->priv;
  int minimum, natural, spacing;

  g_return_if_fail (min_item_size == NULL || min_items != NULL);
  g_return_if_fail (max_item_size == NULL || max_items != NULL);
  g_return_if_fail (!ctk_icon_view_is_empty (icon_view));

  ctk_icon_view_get_preferred_item_size (icon_view, orientation, -1, &minimum, &natural);

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    spacing = priv->column_spacing;
  else
    spacing = priv->row_spacing;
  
  size -= 2 * priv->margin;
  size += spacing;
  minimum += spacing;
  natural += spacing;

  if (priv->columns > 0)
    {
      if (orientation == CTK_ORIENTATION_HORIZONTAL)
        {
          if (min_items)
            *min_items = priv->columns;
          if (max_items)
            *max_items = priv->columns;
        }
      else
        {
          int n_items = ctk_icon_view_get_n_items (icon_view);

          if (min_items)
            *min_items = (n_items + priv->columns - 1) / priv->columns;
          if (max_items)
            *max_items = (n_items + priv->columns - 1) / priv->columns;
        }
    }
  else
    {
      if (max_items)
        {
          if (size <= minimum)
            *max_items = 1;
          else
            *max_items = size / minimum;
        }

      if (min_items)
        {
          if (size <= natural)
            *min_items = 1;
          else
            *min_items = size / natural;
        }
    }

  if (min_item_size)
    {
      *min_item_size = size / *min_items;
      *min_item_size = CLAMP (*min_item_size, minimum, natural);
      *min_item_size -= spacing;
      *min_item_size -= 2 * priv->item_padding;
    }

  if (max_item_size)
    {
      *max_item_size = size / *max_items;
      *max_item_size = CLAMP (*max_item_size, minimum, natural);
      *max_item_size -= spacing;
      *max_item_size -= 2 * priv->item_padding;
    }
}

static CtkSizeRequestMode
ctk_icon_view_get_request_mode (CtkWidget *widget)
{
  return CTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}

static void
ctk_icon_view_get_preferred_width (CtkWidget *widget,
				   gint      *minimum,
				   gint      *natural)
{
  CtkIconView *icon_view = CTK_ICON_VIEW (widget);
  CtkIconViewPrivate *priv = icon_view->priv;
  int item_min, item_nat;

  if (ctk_icon_view_is_empty (icon_view))
    {
      *minimum = *natural = 2 * priv->margin;
      return;
    }

  ctk_icon_view_get_preferred_item_size (icon_view, CTK_ORIENTATION_HORIZONTAL, -1, &item_min, &item_nat);

  if (priv->columns > 0)
    {
      *minimum = item_min * priv->columns + priv->column_spacing * (priv->columns - 1);
      *natural = item_nat * priv->columns + priv->column_spacing * (priv->columns - 1);
    }
  else
    {
      int n_items = ctk_icon_view_get_n_items (icon_view);

      *minimum = item_min;
      *natural = item_nat * n_items + priv->column_spacing * (n_items - 1);
    }

  *minimum += 2 * priv->margin;
  *natural += 2 * priv->margin;
}

static void
ctk_icon_view_get_preferred_width_for_height (CtkWidget *widget,
                                              gint       height,
                                              gint      *minimum,
                                              gint      *natural)
{
  CtkIconView *icon_view = CTK_ICON_VIEW (widget);
  CtkIconViewPrivate *priv = icon_view->priv;
  int item_min, item_nat, rows, row_height, n_items;

  if (ctk_icon_view_is_empty (icon_view))
    {
      *minimum = *natural = 2 * priv->margin;
      return;
    }

  ctk_icon_view_compute_n_items_for_size (icon_view, CTK_ORIENTATION_VERTICAL, height, &rows, &row_height, NULL, NULL);
  n_items = ctk_icon_view_get_n_items (icon_view);

  ctk_icon_view_get_preferred_item_size (icon_view, CTK_ORIENTATION_HORIZONTAL, row_height, &item_min, &item_nat);
  *minimum = (item_min + priv->column_spacing) * ((n_items + rows - 1) / rows) - priv->column_spacing;
  *natural = (item_nat + priv->column_spacing) * ((n_items + rows - 1) / rows) - priv->column_spacing;

  *minimum += 2 * priv->margin;
  *natural += 2 * priv->margin;
}

static void
ctk_icon_view_get_preferred_height (CtkWidget *widget,
				    gint      *minimum,
				    gint      *natural)
{
  CtkIconView *icon_view = CTK_ICON_VIEW (widget);
  CtkIconViewPrivate *priv = icon_view->priv;
  int item_min, item_nat, n_items;

  if (ctk_icon_view_is_empty (icon_view))
    {
      *minimum = *natural = 2 * priv->margin;
      return;
    }

  ctk_icon_view_get_preferred_item_size (icon_view, CTK_ORIENTATION_VERTICAL, -1, &item_min, &item_nat);
  n_items = ctk_icon_view_get_n_items (icon_view);

  if (priv->columns > 0)
    {
      int n_rows = (n_items + priv->columns - 1) / priv->columns;

      *minimum = item_min * n_rows + priv->row_spacing * (n_rows - 1);
      *natural = item_nat * n_rows + priv->row_spacing * (n_rows - 1);
    }
  else
    {
      *minimum = item_min;
      *natural = item_nat * n_items + priv->row_spacing * (n_items - 1);
    }

  *minimum += 2 * priv->margin;
  *natural += 2 * priv->margin;
}

static void
ctk_icon_view_get_preferred_height_for_width (CtkWidget *widget,
                                              gint       width,
                                              gint      *minimum,
                                              gint      *natural)
{
  CtkIconView *icon_view = CTK_ICON_VIEW (widget);
  CtkIconViewPrivate *priv = icon_view->priv;
  int item_min, item_nat, columns, column_width, n_items;

  if (ctk_icon_view_is_empty (icon_view))
    {
      *minimum = *natural = 2 * priv->margin;
      return;
    }

  ctk_icon_view_compute_n_items_for_size (icon_view, CTK_ORIENTATION_HORIZONTAL, width, NULL, NULL, &columns, &column_width);
  n_items = ctk_icon_view_get_n_items (icon_view);

  ctk_icon_view_get_preferred_item_size (icon_view, CTK_ORIENTATION_VERTICAL, column_width, &item_min, &item_nat);
  *minimum = (item_min + priv->row_spacing) * ((n_items + columns - 1) / columns) - priv->row_spacing;
  *natural = (item_nat + priv->row_spacing) * ((n_items + columns - 1) / columns) - priv->row_spacing;

  *minimum += 2 * priv->margin;
  *natural += 2 * priv->margin;
}

static void
ctk_icon_view_allocate_children (CtkIconView *icon_view)
{
  GList *list;

  for (list = icon_view->priv->children; list; list = list->next)
    {
      CtkIconViewChild *child = list->data;

      /* totally ignore our child's requisition */
      ctk_widget_size_allocate (child->widget, &child->area);
    }
}

static void
ctk_icon_view_size_allocate (CtkWidget      *widget,
			     CtkAllocation  *allocation)
{
  CtkIconView *icon_view = CTK_ICON_VIEW (widget);

  ctk_widget_set_allocation (widget, allocation);

  ctk_icon_view_layout (icon_view);

  if (ctk_widget_get_realized (widget))
    {
      cdk_window_move_resize (ctk_widget_get_window (widget),
			      allocation->x, allocation->y,
			      allocation->width, allocation->height);
      cdk_window_resize (icon_view->priv->bin_window,
			 MAX (icon_view->priv->width, allocation->width),
			 MAX (icon_view->priv->height, allocation->height));
    }

  ctk_icon_view_allocate_children (icon_view);

  /* Delay signal emission */
  g_object_freeze_notify (G_OBJECT (icon_view->priv->hadjustment));
  g_object_freeze_notify (G_OBJECT (icon_view->priv->vadjustment));

  ctk_icon_view_set_hadjustment_values (icon_view);
  ctk_icon_view_set_vadjustment_values (icon_view);

  if (ctk_widget_get_realized (widget) &&
      icon_view->priv->scroll_to_path)
    {
      CtkTreePath *path;
      path = ctk_tree_row_reference_get_path (icon_view->priv->scroll_to_path);
      ctk_tree_row_reference_free (icon_view->priv->scroll_to_path);
      icon_view->priv->scroll_to_path = NULL;

      ctk_icon_view_scroll_to_path (icon_view, path,
				    icon_view->priv->scroll_to_use_align,
				    icon_view->priv->scroll_to_row_align,
				    icon_view->priv->scroll_to_col_align);
      ctk_tree_path_free (path);
    }

  /* Emit any pending signals now */
  g_object_thaw_notify (G_OBJECT (icon_view->priv->hadjustment));
  g_object_thaw_notify (G_OBJECT (icon_view->priv->vadjustment));
}

static gboolean
ctk_icon_view_draw (CtkWidget *widget,
                    cairo_t   *cr)
{
  CtkIconView *icon_view;
  GList *icons;
  CtkTreePath *path;
  gint dest_index;
  CtkIconViewDropPosition dest_pos;
  CtkIconViewItem *dest_item = NULL;
  CtkStyleContext *context;

  icon_view = CTK_ICON_VIEW (widget);

  context = ctk_widget_get_style_context (widget);
  ctk_render_background (context, cr,
                         0, 0,
                         ctk_widget_get_allocated_width (widget),
                         ctk_widget_get_allocated_height (widget));

  if (!ctk_cairo_should_draw_window (cr, icon_view->priv->bin_window))
    return FALSE;

  cairo_save (cr);

  ctk_cairo_transform_to_window (cr, widget, icon_view->priv->bin_window);

  cairo_set_line_width (cr, 1.);

  ctk_icon_view_get_drag_dest_item (icon_view, &path, &dest_pos);

  if (path)
    {
      dest_index = ctk_tree_path_get_indices (path)[0];
      ctk_tree_path_free (path);
    }
  else
    dest_index = -1;

  for (icons = icon_view->priv->items; icons; icons = icons->next)
    {
      CtkIconViewItem *item = icons->data;
      CdkRectangle paint_area;

      paint_area.x      = item->cell_area.x      - icon_view->priv->item_padding;
      paint_area.y      = item->cell_area.y      - icon_view->priv->item_padding;
      paint_area.width  = item->cell_area.width  + icon_view->priv->item_padding * 2;
      paint_area.height = item->cell_area.height + icon_view->priv->item_padding * 2;

      cairo_save (cr);

      cairo_rectangle (cr, paint_area.x, paint_area.y, paint_area.width, paint_area.height);
      cairo_clip (cr);

      if (cdk_cairo_get_clip_rectangle (cr, NULL))
        {
          ctk_icon_view_paint_item (icon_view, cr, item,
                                    item->cell_area.x, item->cell_area.y,
                                    icon_view->priv->draw_focus);

          if (dest_index == item->index)
            dest_item = item;
        }

      cairo_restore (cr);
    }

  if (dest_item &&
      dest_pos != CTK_ICON_VIEW_NO_DROP)
    {
      CdkRectangle rect = { 0 };

      switch (dest_pos)
	{
	case CTK_ICON_VIEW_DROP_INTO:
          rect = dest_item->cell_area;
	  break;
	case CTK_ICON_VIEW_DROP_ABOVE:
          rect.x = dest_item->cell_area.x;
          rect.y = dest_item->cell_area.y - 1;
          rect.width = dest_item->cell_area.width;
          rect.height = 2;
	  break;
	case CTK_ICON_VIEW_DROP_LEFT:
          rect.x = dest_item->cell_area.x - 1;
          rect.y = dest_item->cell_area.y;
          rect.width = 2;
          rect.height = dest_item->cell_area.height;
	  break;
	case CTK_ICON_VIEW_DROP_BELOW:
          rect.x = dest_item->cell_area.x;
          rect.y = dest_item->cell_area.y + dest_item->cell_area.height - 1;
          rect.width = dest_item->cell_area.width;
          rect.height = 2;
	  break;
	case CTK_ICON_VIEW_DROP_RIGHT:
          rect.x = dest_item->cell_area.x + dest_item->cell_area.width - 1;
          rect.y = dest_item->cell_area.y;
          rect.width = 2;
          rect.height = dest_item->cell_area.height;
	case CTK_ICON_VIEW_NO_DROP: ;
	  break;
        }

      ctk_render_focus (context, cr,
                        rect.x, rect.y,
                        rect.width, rect.height);
    }

  if (icon_view->priv->doing_rubberband)
    ctk_icon_view_paint_rubberband (icon_view, cr);

  cairo_restore (cr);

  return CTK_WIDGET_CLASS (ctk_icon_view_parent_class)->draw (widget, cr);
}

static gboolean
rubberband_scroll_timeout (gpointer data)
{
  CtkIconView *icon_view = data;

  ctk_adjustment_set_value (icon_view->priv->vadjustment,
                            ctk_adjustment_get_value (icon_view->priv->vadjustment) +
                            icon_view->priv->scroll_value_diff);

  ctk_icon_view_update_rubberband (icon_view);
  
  return TRUE;
}

static gboolean
ctk_icon_view_motion (CtkWidget      *widget,
		      CdkEventMotion *event)
{
  CtkAllocation allocation;
  CtkIconView *icon_view;
  gint abs_y;
  
  icon_view = CTK_ICON_VIEW (widget);

  ctk_icon_view_maybe_begin_drag (icon_view, event);

  if (icon_view->priv->doing_rubberband)
    {
      ctk_icon_view_update_rubberband (widget);
      
      abs_y = event->y - icon_view->priv->height *
	(ctk_adjustment_get_value (icon_view->priv->vadjustment) /
	 (ctk_adjustment_get_upper (icon_view->priv->vadjustment) -
	  ctk_adjustment_get_lower (icon_view->priv->vadjustment)));

      ctk_widget_get_allocation (widget, &allocation);

      if (abs_y < 0 || abs_y > allocation.height)
	{
	  if (abs_y < 0)
	    icon_view->priv->scroll_value_diff = abs_y;
	  else
	    icon_view->priv->scroll_value_diff = abs_y - allocation.height;

	  icon_view->priv->event_last_x = event->x;
	  icon_view->priv->event_last_y = event->y;

	  if (icon_view->priv->scroll_timeout_id == 0) {
	    icon_view->priv->scroll_timeout_id = cdk_threads_add_timeout (30, rubberband_scroll_timeout, 
								icon_view);
	    g_source_set_name_by_id (icon_view->priv->scroll_timeout_id, "[ctk+] rubberband_scroll_timeout");
	  }
 	}
      else 
	remove_scroll_timeout (icon_view);
    }
  else
    {
      CtkIconViewItem *item, *last_prelight_item;
      CtkCellRenderer *cell = NULL;

      last_prelight_item = icon_view->priv->last_prelight;
      item = _ctk_icon_view_get_item_at_coords (icon_view,
                                               event->x, event->y,
                                               FALSE,
                                               &cell);

      if (item != last_prelight_item)
        {
          if (item != NULL)
            {
              ctk_icon_view_queue_draw_item (icon_view, item);
            }

          if (last_prelight_item != NULL)
            {
              ctk_icon_view_queue_draw_item (icon_view,
                                             icon_view->priv->last_prelight);
            }

          icon_view->priv->last_prelight = item;
        }
    }
  
  return TRUE;
}

static gboolean
ctk_icon_view_leave (CtkWidget        *widget,
                     CdkEventCrossing *event)
{
  CtkIconView *icon_view;
  CtkIconViewPrivate *priv;

  icon_view = CTK_ICON_VIEW (widget);
  priv = icon_view->priv;

  if (priv->last_prelight)
    {
      ctk_icon_view_queue_draw_item (icon_view, priv->last_prelight);
      priv->last_prelight = NULL;
    }

  return FALSE;
}

static void
ctk_icon_view_remove (CtkContainer *container,
		      CtkWidget    *widget)
{
  CtkIconView *icon_view;
  CtkIconViewChild *child = NULL;
  GList *tmp_list;

  icon_view = CTK_ICON_VIEW (container);
  
  tmp_list = icon_view->priv->children;
  while (tmp_list)
    {
      child = tmp_list->data;
      if (child->widget == widget)
	{
	  ctk_widget_unparent (widget);

	  icon_view->priv->children = g_list_remove_link (icon_view->priv->children, tmp_list);
	  g_list_free_1 (tmp_list);
	  g_free (child);
	  return;
	}

      tmp_list = tmp_list->next;
    }
}

static void
ctk_icon_view_forall (CtkContainer *container,
		      gboolean      include_internals,
		      CtkCallback   callback,
		      gpointer      callback_data)
{
  CtkIconView *icon_view;
  CtkIconViewChild *child = NULL;
  GList *tmp_list;

  icon_view = CTK_ICON_VIEW (container);

  tmp_list = icon_view->priv->children;
  while (tmp_list)
    {
      child = tmp_list->data;
      tmp_list = tmp_list->next;

      (* callback) (child->widget, callback_data);
    }
}

static void 
ctk_icon_view_item_selected_changed (CtkIconView      *icon_view,
                                     CtkIconViewItem  *item)
{
  AtkObject *obj;
  AtkObject *item_obj;

  obj = ctk_widget_get_accessible (CTK_WIDGET (icon_view));
  if (obj != NULL)
    {
      item_obj = atk_object_ref_accessible_child (obj, item->index);
      if (item_obj != NULL)
        {
          atk_object_notify_state_change (item_obj, ATK_STATE_SELECTED, item->selected);
          g_object_unref (item_obj);
        }
    }
}

static void
ctk_icon_view_add_editable (CtkCellArea            *area,
			    CtkCellRenderer        *renderer,
			    CtkCellEditable        *editable,
			    CdkRectangle           *cell_area,
			    const gchar            *path,
			    CtkIconView            *icon_view)
{
  CtkIconViewChild *child;
  CtkWidget *widget = CTK_WIDGET (editable);
  
  child = g_new (CtkIconViewChild, 1);
  
  child->widget      = widget;
  child->area.x      = cell_area->x;
  child->area.y      = cell_area->y;
  child->area.width  = cell_area->width;
  child->area.height = cell_area->height;

  icon_view->priv->children = g_list_append (icon_view->priv->children, child);

  if (ctk_widget_get_realized (CTK_WIDGET (icon_view)))
    ctk_widget_set_parent_window (child->widget, icon_view->priv->bin_window);
  
  ctk_widget_set_parent (widget, CTK_WIDGET (icon_view));
}

static void
ctk_icon_view_remove_editable (CtkCellArea            *area,
			       CtkCellRenderer        *renderer,
			       CtkCellEditable        *editable,
			       CtkIconView            *icon_view)
{
  CtkTreePath *path;

  if (ctk_widget_has_focus (CTK_WIDGET (editable)))
    ctk_widget_grab_focus (CTK_WIDGET (icon_view));
  
  ctk_container_remove (CTK_CONTAINER (icon_view),
			CTK_WIDGET (editable));  

  path = ctk_tree_path_new_from_string (ctk_cell_area_get_current_path_string (area));
  ctk_icon_view_queue_draw_path (icon_view, path);
  ctk_tree_path_free (path);
}

/**
 * ctk_icon_view_set_cursor:
 * @icon_view: A #CtkIconView
 * @path: A #CtkTreePath
 * @cell: (allow-none): One of the cell renderers of @icon_view, or %NULL
 * @start_editing: %TRUE if the specified cell should start being edited.
 *
 * Sets the current keyboard focus to be at @path, and selects it.  This is
 * useful when you want to focus the user’s attention on a particular item.
 * If @cell is not %NULL, then focus is given to the cell specified by 
 * it. Additionally, if @start_editing is %TRUE, then editing should be 
 * started in the specified cell.  
 *
 * This function is often followed by `ctk_widget_grab_focus 
 * (icon_view)` in order to give keyboard focus to the widget.  
 * Please note that editing can only happen when the widget is realized.
 *
 * Since: 2.8
 **/
void
ctk_icon_view_set_cursor (CtkIconView     *icon_view,
			  CtkTreePath     *path,
			  CtkCellRenderer *cell,
			  gboolean         start_editing)
{
  CtkIconViewItem *item = NULL;

  g_return_if_fail (CTK_IS_ICON_VIEW (icon_view));
  g_return_if_fail (path != NULL);
  g_return_if_fail (cell == NULL || CTK_IS_CELL_RENDERER (cell));

  if (icon_view->priv->cell_area)
    ctk_cell_area_stop_editing (icon_view->priv->cell_area, TRUE);

  if (ctk_tree_path_get_depth (path) == 1)
    item = g_list_nth_data (icon_view->priv->items,
			    ctk_tree_path_get_indices(path)[0]);
  
  if (!item)
    return;
  
  _ctk_icon_view_set_cursor_item (icon_view, item, cell);
  ctk_icon_view_scroll_to_path (icon_view, path, FALSE, 0.0, 0.0);

  if (start_editing && 
      icon_view->priv->cell_area)
    {
      CtkCellAreaContext *context;

      context = g_ptr_array_index (icon_view->priv->row_contexts, item->row);
      _ctk_icon_view_set_cell_data (icon_view, item);
      ctk_cell_area_activate (icon_view->priv->cell_area, context, 
			      CTK_WIDGET (icon_view), &item->cell_area, 
			      0 /* XXX flags */, TRUE);
    }
}

/**
 * ctk_icon_view_get_cursor:
 * @icon_view: A #CtkIconView
 * @path: (out) (allow-none) (transfer full): Return location for the current
 *        cursor path, or %NULL
 * @cell: (out) (allow-none) (transfer none): Return location the current
 *        focus cell, or %NULL
 *
 * Fills in @path and @cell with the current cursor path and cell. 
 * If the cursor isn’t currently set, then *@path will be %NULL.  
 * If no cell currently has focus, then *@cell will be %NULL.
 *
 * The returned #CtkTreePath must be freed with ctk_tree_path_free().
 *
 * Returns: %TRUE if the cursor is set.
 *
 * Since: 2.8
 **/
gboolean
ctk_icon_view_get_cursor (CtkIconView      *icon_view,
			  CtkTreePath     **path,
			  CtkCellRenderer **cell)
{
  CtkIconViewItem *item;

  g_return_val_if_fail (CTK_IS_ICON_VIEW (icon_view), FALSE);

  item = icon_view->priv->cursor_item;

  if (path != NULL)
    {
      if (item != NULL)
	*path = ctk_tree_path_new_from_indices (item->index, -1);
      else
	*path = NULL;
    }

  if (cell != NULL && item != NULL && icon_view->priv->cell_area != NULL)
    *cell = ctk_cell_area_get_focus_cell (icon_view->priv->cell_area);

  return (item != NULL);
}

static gboolean
ctk_icon_view_button_press (CtkWidget      *widget,
			    CdkEventButton *event)
{
  CtkIconView *icon_view;
  CtkIconViewItem *item;
  gboolean dirty = FALSE;
  CtkCellRenderer *cell = NULL, *cursor_cell = NULL;

  icon_view = CTK_ICON_VIEW (widget);

  if (event->window != icon_view->priv->bin_window)
    return FALSE;

  if (!ctk_widget_has_focus (widget))
    ctk_widget_grab_focus (widget);

  if (event->button == GDK_BUTTON_PRIMARY && event->type == GDK_BUTTON_PRESS)
    {
      CdkModifierType extend_mod_mask;
      CdkModifierType modify_mod_mask;

      extend_mod_mask =
        ctk_widget_get_modifier_mask (widget, GDK_MODIFIER_INTENT_EXTEND_SELECTION);

      modify_mod_mask =
        ctk_widget_get_modifier_mask (widget, GDK_MODIFIER_INTENT_MODIFY_SELECTION);

      item = _ctk_icon_view_get_item_at_coords (icon_view, 
					       event->x, event->y,
					       FALSE,
					       &cell);

      /*
       * We consider only the cells' area as the item area if the
       * item is not selected, but if it *is* selected, the complete
       * selection rectangle is considered to be part of the item.
       */
      if (item != NULL && (cell != NULL || item->selected))
	{
	  if (cell != NULL)
	    {
	      if (ctk_cell_renderer_is_activatable (cell))
		cursor_cell = cell;
	    }

	  ctk_icon_view_scroll_to_item (icon_view, item);
	  
	  if (icon_view->priv->selection_mode == CTK_SELECTION_NONE)
	    {
	      _ctk_icon_view_set_cursor_item (icon_view, item, cursor_cell);
	    }
	  else if (icon_view->priv->selection_mode == CTK_SELECTION_MULTIPLE &&
		   (event->state & extend_mod_mask))
	    {
	      ctk_icon_view_unselect_all_internal (icon_view);

	      _ctk_icon_view_set_cursor_item (icon_view, item, cursor_cell);
	      if (!icon_view->priv->anchor_item)
		icon_view->priv->anchor_item = item;
	      else 
		ctk_icon_view_select_all_between (icon_view,
						  icon_view->priv->anchor_item,
						  item);
	      dirty = TRUE;
	    }
	  else 
	    {
	      if ((icon_view->priv->selection_mode == CTK_SELECTION_MULTIPLE ||
		  ((icon_view->priv->selection_mode == CTK_SELECTION_SINGLE) && item->selected)) &&
		  (event->state & modify_mod_mask))
		{
		  item->selected = !item->selected;
		  ctk_icon_view_queue_draw_item (icon_view, item);
		  dirty = TRUE;
		}
	      else
		{
		  ctk_icon_view_unselect_all_internal (icon_view);

		  item->selected = TRUE;
		  ctk_icon_view_queue_draw_item (icon_view, item);
		  dirty = TRUE;
		}
	      _ctk_icon_view_set_cursor_item (icon_view, item, cursor_cell);
	      icon_view->priv->anchor_item = item;
	    }

	  /* Save press to possibly begin a drag */
	  if (icon_view->priv->pressed_button < 0)
	    {
	      icon_view->priv->pressed_button = event->button;
	      icon_view->priv->press_start_x = event->x;
	      icon_view->priv->press_start_y = event->y;
	    }

          icon_view->priv->last_single_clicked = item;

	  /* cancel the current editing, if it exists */
	  ctk_cell_area_stop_editing (icon_view->priv->cell_area, TRUE);

	  if (cell != NULL && ctk_cell_renderer_is_activatable (cell))
	    {
	      CtkCellAreaContext *context;

	      context = g_ptr_array_index (icon_view->priv->row_contexts, item->row);

	      _ctk_icon_view_set_cell_data (icon_view, item);
	      ctk_cell_area_activate (icon_view->priv->cell_area, context,
				      CTK_WIDGET (icon_view),
				      &item->cell_area, 0/* XXX flags */, FALSE);
	    }
	}
      else
	{
	  if (icon_view->priv->selection_mode != CTK_SELECTION_BROWSE &&
	      !(event->state & modify_mod_mask))
	    {
	      dirty = ctk_icon_view_unselect_all_internal (icon_view);
	    }
	  
	  if (icon_view->priv->selection_mode == CTK_SELECTION_MULTIPLE)
            ctk_icon_view_start_rubberbanding (icon_view, event->device, event->x, event->y);
	}

      /* don't draw keyboard focus around an clicked-on item */
      icon_view->priv->draw_focus = FALSE;
    }

  if (!icon_view->priv->activate_on_single_click
      && event->button == GDK_BUTTON_PRIMARY
      && event->type == GDK_2BUTTON_PRESS)
    {
      item = _ctk_icon_view_get_item_at_coords (icon_view,
					       event->x, event->y,
					       FALSE,
					       NULL);

      if (item && item == icon_view->priv->last_single_clicked)
	{
	  CtkTreePath *path;

	  path = ctk_tree_path_new_from_indices (item->index, -1);
	  ctk_icon_view_item_activated (icon_view, path);
	  ctk_tree_path_free (path);
	}

      icon_view->priv->last_single_clicked = NULL;
      icon_view->priv->pressed_button = -1;
    }
  
  if (dirty)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);

  return event->button == GDK_BUTTON_PRIMARY;
}

static gboolean
button_event_modifies_selection (CdkEventButton *event)
{
        return (event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) != 0;
}

static gboolean
ctk_icon_view_button_release (CtkWidget      *widget,
			      CdkEventButton *event)
{
  CtkIconView *icon_view;

  icon_view = CTK_ICON_VIEW (widget);
  
  if (icon_view->priv->pressed_button == event->button)
    icon_view->priv->pressed_button = -1;

  ctk_icon_view_stop_rubberbanding (icon_view);

  remove_scroll_timeout (icon_view);

  if (event->button == GDK_BUTTON_PRIMARY
      && icon_view->priv->activate_on_single_click
      && !button_event_modifies_selection (event)
      && icon_view->priv->last_single_clicked != NULL)
    {
      CtkIconViewItem *item;

      item = _ctk_icon_view_get_item_at_coords (icon_view,
                                                event->x, event->y,
                                                FALSE,
                                                NULL);
      if (item == icon_view->priv->last_single_clicked)
        {
          CtkTreePath *path;
          path = ctk_tree_path_new_from_indices (item->index, -1);
          ctk_icon_view_item_activated (icon_view, path);
          ctk_tree_path_free (path);
        }

      icon_view->priv->last_single_clicked = NULL;
    }

  return TRUE;
}

static gboolean
ctk_icon_view_key_press (CtkWidget      *widget,
			 CdkEventKey    *event)
{
  CtkIconView *icon_view = CTK_ICON_VIEW (widget);

  if (icon_view->priv->doing_rubberband)
    {
      if (event->keyval == GDK_KEY_Escape)
	ctk_icon_view_stop_rubberbanding (icon_view);

      return TRUE;
    }

  return CTK_WIDGET_CLASS (ctk_icon_view_parent_class)->key_press_event (widget, event);
}

static gboolean
ctk_icon_view_key_release (CtkWidget      *widget,
			   CdkEventKey    *event)
{
  CtkIconView *icon_view = CTK_ICON_VIEW (widget);

  if (icon_view->priv->doing_rubberband)
    return TRUE;

  return CTK_WIDGET_CLASS (ctk_icon_view_parent_class)->key_release_event (widget, event);
}

static void
ctk_icon_view_update_rubberband (gpointer data)
{
  CtkIconView *icon_view;
  gint x, y;
  CdkRectangle old_area;
  CdkRectangle new_area;
  cairo_region_t *invalid_region;
  
  icon_view = CTK_ICON_VIEW (data);

  cdk_window_get_device_position (icon_view->priv->bin_window,
                                  icon_view->priv->rubberband_device,
                                  &x, &y, NULL);

  x = MAX (x, 0);
  y = MAX (y, 0);

  old_area.x = MIN (icon_view->priv->rubberband_x1,
		    icon_view->priv->rubberband_x2);
  old_area.y = MIN (icon_view->priv->rubberband_y1,
		    icon_view->priv->rubberband_y2);
  old_area.width = ABS (icon_view->priv->rubberband_x2 -
			icon_view->priv->rubberband_x1) + 1;
  old_area.height = ABS (icon_view->priv->rubberband_y2 -
			 icon_view->priv->rubberband_y1) + 1;
  
  new_area.x = MIN (icon_view->priv->rubberband_x1, x);
  new_area.y = MIN (icon_view->priv->rubberband_y1, y);
  new_area.width = ABS (x - icon_view->priv->rubberband_x1) + 1;
  new_area.height = ABS (y - icon_view->priv->rubberband_y1) + 1;

  invalid_region = cairo_region_create_rectangle (&old_area);
  cairo_region_union_rectangle (invalid_region, &new_area);

  cdk_window_invalidate_region (icon_view->priv->bin_window, invalid_region, TRUE);
    
  cairo_region_destroy (invalid_region);

  icon_view->priv->rubberband_x2 = x;
  icon_view->priv->rubberband_y2 = y;  

  ctk_icon_view_update_rubberband_selection (icon_view);
}

static void
ctk_icon_view_start_rubberbanding (CtkIconView  *icon_view,
                                   CdkDevice    *device,
				   gint          x,
				   gint          y)
{
  CtkIconViewPrivate *priv = icon_view->priv;
  GList *items;
  CtkCssNode *widget_node;

  if (priv->rubberband_device)
    return;

  for (items = priv->items; items; items = items->next)
    {
      CtkIconViewItem *item = items->data;

      item->selected_before_rubberbanding = item->selected;
    }

  priv->rubberband_x1 = x;
  priv->rubberband_y1 = y;
  priv->rubberband_x2 = x;
  priv->rubberband_y2 = y;

  priv->doing_rubberband = TRUE;
  priv->rubberband_device = device;

  widget_node = ctk_widget_get_css_node (CTK_WIDGET (icon_view));
  priv->rubberband_node = ctk_css_node_new ();
  ctk_css_node_set_name (priv->rubberband_node, I_("rubberband"));
  ctk_css_node_set_parent (priv->rubberband_node, widget_node);
  ctk_css_node_set_state (priv->rubberband_node, ctk_css_node_get_state (widget_node));
  g_object_unref (priv->rubberband_node);
}

static void
ctk_icon_view_stop_rubberbanding (CtkIconView *icon_view)
{
  CtkIconViewPrivate *priv = icon_view->priv;

  if (!priv->doing_rubberband)
    return;

  priv->doing_rubberband = FALSE;
  priv->rubberband_device = NULL;
  ctk_css_node_set_parent (priv->rubberband_node, NULL);
  priv->rubberband_node = NULL;

  ctk_widget_queue_draw (CTK_WIDGET (icon_view));
}

static void
ctk_icon_view_update_rubberband_selection (CtkIconView *icon_view)
{
  GList *items;
  gint x, y, width, height;
  gboolean dirty = FALSE;
  
  x = MIN (icon_view->priv->rubberband_x1,
	   icon_view->priv->rubberband_x2);
  y = MIN (icon_view->priv->rubberband_y1,
	   icon_view->priv->rubberband_y2);
  width = ABS (icon_view->priv->rubberband_x1 - 
	       icon_view->priv->rubberband_x2);
  height = ABS (icon_view->priv->rubberband_y1 - 
		icon_view->priv->rubberband_y2);
  
  for (items = icon_view->priv->items; items; items = items->next)
    {
      CtkIconViewItem *item = items->data;
      gboolean is_in;
      gboolean selected;
      
      is_in = ctk_icon_view_item_hit_test (icon_view, item, 
					   x, y, width, height);

      selected = is_in ^ item->selected_before_rubberbanding;

      if (item->selected != selected)
	{
	  item->selected = selected;
	  dirty = TRUE;
	  ctk_icon_view_queue_draw_item (icon_view, item);
	}
    }

  if (dirty)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}


typedef struct {
  CdkRectangle hit_rect;
  gboolean     hit;
} HitTestData;

static gboolean 
hit_test (CtkCellRenderer    *renderer,
	  const CdkRectangle *cell_area,
	  const CdkRectangle *cell_background,
	  HitTestData        *data)
{
  if (MIN (data->hit_rect.x + data->hit_rect.width, cell_area->x + cell_area->width) - 
      MAX (data->hit_rect.x, cell_area->x) > 0 &&
      MIN (data->hit_rect.y + data->hit_rect.height, cell_area->y + cell_area->height) - 
      MAX (data->hit_rect.y, cell_area->y) > 0)
    data->hit = TRUE;
  
  return (data->hit != FALSE);
}

static gboolean
ctk_icon_view_item_hit_test (CtkIconView      *icon_view,
			     CtkIconViewItem  *item,
			     gint              x,
			     gint              y,
			     gint              width,
			     gint              height)
{
  HitTestData data = { { x, y, width, height }, FALSE };
  CtkCellAreaContext *context;
  CdkRectangle *item_area = &item->cell_area;
   
  if (MIN (x + width, item_area->x + item_area->width) - MAX (x, item_area->x) <= 0 ||
      MIN (y + height, item_area->y + item_area->height) - MAX (y, item_area->y) <= 0)
    return FALSE;

  context = g_ptr_array_index (icon_view->priv->row_contexts, item->row);

  _ctk_icon_view_set_cell_data (icon_view, item);
  ctk_cell_area_foreach_alloc (icon_view->priv->cell_area, context,
			       CTK_WIDGET (icon_view),
			       item_area, item_area,
			       (CtkCellAllocCallback)hit_test, &data);

  return data.hit;
}

static gboolean
ctk_icon_view_unselect_all_internal (CtkIconView  *icon_view)
{
  gboolean dirty = FALSE;
  GList *items;

  if (icon_view->priv->selection_mode == CTK_SELECTION_NONE)
    return FALSE;

  for (items = icon_view->priv->items; items; items = items->next)
    {
      CtkIconViewItem *item = items->data;

      if (item->selected)
	{
	  item->selected = FALSE;
	  dirty = TRUE;
	  ctk_icon_view_queue_draw_item (icon_view, item);
	  ctk_icon_view_item_selected_changed (icon_view, item);
	}
    }

  return dirty;
}


/* CtkIconView signals */
static void
ctk_icon_view_real_select_all (CtkIconView *icon_view)
{
  ctk_icon_view_select_all (icon_view);
}

static void
ctk_icon_view_real_unselect_all (CtkIconView *icon_view)
{
  ctk_icon_view_unselect_all (icon_view);
}

static void
ctk_icon_view_real_select_cursor_item (CtkIconView *icon_view)
{
  ctk_icon_view_unselect_all (icon_view);

  if (icon_view->priv->cursor_item != NULL)
    _ctk_icon_view_select_item (icon_view, icon_view->priv->cursor_item);
}

static gboolean
ctk_icon_view_real_activate_cursor_item (CtkIconView *icon_view)
{
  CtkTreePath *path;
  CtkCellAreaContext *context;

  if (!icon_view->priv->cursor_item)
    return FALSE;

  context = g_ptr_array_index (icon_view->priv->row_contexts, icon_view->priv->cursor_item->row);

  _ctk_icon_view_set_cell_data (icon_view, icon_view->priv->cursor_item);
  ctk_cell_area_activate (icon_view->priv->cell_area, context,
                          CTK_WIDGET (icon_view),
                          &icon_view->priv->cursor_item->cell_area,
                          0 /* XXX flags */,
                          FALSE);

  path = ctk_tree_path_new_from_indices (icon_view->priv->cursor_item->index, -1);
  ctk_icon_view_item_activated (icon_view, path);
  ctk_tree_path_free (path);

  return TRUE;
}

static void
ctk_icon_view_real_toggle_cursor_item (CtkIconView *icon_view)
{
  if (!icon_view->priv->cursor_item)
    return;

  switch (icon_view->priv->selection_mode)
    {
    case CTK_SELECTION_NONE:
      break;
    case CTK_SELECTION_BROWSE:
      _ctk_icon_view_select_item (icon_view, icon_view->priv->cursor_item);
      break;
    case CTK_SELECTION_SINGLE:
      if (icon_view->priv->cursor_item->selected)
	_ctk_icon_view_unselect_item (icon_view, icon_view->priv->cursor_item);
      else
	_ctk_icon_view_select_item (icon_view, icon_view->priv->cursor_item);
      break;
    case CTK_SELECTION_MULTIPLE:
      icon_view->priv->cursor_item->selected = !icon_view->priv->cursor_item->selected;
      g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0); 
      
      ctk_icon_view_item_selected_changed (icon_view, icon_view->priv->cursor_item);      
      ctk_icon_view_queue_draw_item (icon_view, icon_view->priv->cursor_item);
      break;
    }
}

static void
ctk_icon_view_set_hadjustment_values (CtkIconView *icon_view)
{
  CtkAllocation  allocation;
  CtkAdjustment *adj = icon_view->priv->hadjustment;
  gdouble old_page_size;
  gdouble old_upper;
  gdouble old_value;
  gdouble new_value;
  gdouble new_upper;

  ctk_widget_get_allocation (CTK_WIDGET (icon_view), &allocation);

  old_value = ctk_adjustment_get_value (adj);
  old_upper = ctk_adjustment_get_upper (adj);
  old_page_size = ctk_adjustment_get_page_size (adj);
  new_upper = MAX (allocation.width, icon_view->priv->width);

  if (ctk_widget_get_direction (CTK_WIDGET (icon_view)) == CTK_TEXT_DIR_RTL)
    {
      /* Make sure no scrolling occurs for RTL locales also (if possible) */
      /* Quick explanation:
       *   In LTR locales, leftmost portion of visible rectangle should stay
       *   fixed, which means left edge of scrollbar thumb should remain fixed
       *   and thus adjustment's value should stay the same.
       *
       *   In RTL locales, we want to keep rightmost portion of visible
       *   rectangle fixed. This means right edge of thumb should remain fixed.
       *   In this case, upper - value - page_size should remain constant.
       */
      new_value = (new_upper - allocation.width) -
                  (old_upper - old_value - old_page_size);
      new_value = CLAMP (new_value, 0, new_upper - allocation.width);
    }
  else
    new_value = CLAMP (old_value, 0, new_upper - allocation.width);

  ctk_adjustment_configure (adj,
                            new_value,
                            0.0,
                            new_upper,
                            allocation.width * 0.1,
                            allocation.width * 0.9,
                            allocation.width);
}

static void
ctk_icon_view_set_vadjustment_values (CtkIconView *icon_view)
{
  CtkAllocation  allocation;
  CtkAdjustment *adj = icon_view->priv->vadjustment;

  ctk_widget_get_allocation (CTK_WIDGET (icon_view), &allocation);

  ctk_adjustment_configure (adj,
                            ctk_adjustment_get_value (adj),
                            0.0,
                            MAX (allocation.height, icon_view->priv->height),
                            allocation.height * 0.1,
                            allocation.height * 0.9,
                            allocation.height);
}

static void
ctk_icon_view_set_hadjustment (CtkIconView   *icon_view,
                               CtkAdjustment *adjustment)
{
  CtkIconViewPrivate *priv = icon_view->priv;

  if (adjustment && priv->hadjustment == adjustment)
    return;

  if (priv->hadjustment != NULL)
    {
      g_signal_handlers_disconnect_matched (priv->hadjustment,
                                            G_SIGNAL_MATCH_DATA,
                                            0, 0, NULL, NULL, icon_view);
      g_object_unref (priv->hadjustment);
    }

  if (!adjustment)
    adjustment = ctk_adjustment_new (0.0, 0.0, 0.0,
                                     0.0, 0.0, 0.0);

  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (ctk_icon_view_adjustment_changed), icon_view);
  priv->hadjustment = g_object_ref_sink (adjustment);
  ctk_icon_view_set_hadjustment_values (icon_view);

  g_object_notify (G_OBJECT (icon_view), "hadjustment");
}

static void
ctk_icon_view_set_vadjustment (CtkIconView   *icon_view,
                               CtkAdjustment *adjustment)
{
  CtkIconViewPrivate *priv = icon_view->priv;

  if (adjustment && priv->vadjustment == adjustment)
    return;

  if (priv->vadjustment != NULL)
    {
      g_signal_handlers_disconnect_matched (priv->vadjustment,
                                            G_SIGNAL_MATCH_DATA,
                                            0, 0, NULL, NULL, icon_view);
      g_object_unref (priv->vadjustment);
    }

  if (!adjustment)
    adjustment = ctk_adjustment_new (0.0, 0.0, 0.0,
                                     0.0, 0.0, 0.0);

  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (ctk_icon_view_adjustment_changed), icon_view);
  priv->vadjustment = g_object_ref_sink (adjustment);
  ctk_icon_view_set_vadjustment_values (icon_view);

  g_object_notify (G_OBJECT (icon_view), "vadjustment");
}

static void
ctk_icon_view_adjustment_changed (CtkAdjustment *adjustment,
                                  CtkIconView   *icon_view)
{
  CtkIconViewPrivate *priv = icon_view->priv;

  if (ctk_widget_get_realized (CTK_WIDGET (icon_view)))
    {
      cdk_window_move (priv->bin_window,
                       - ctk_adjustment_get_value (priv->hadjustment),
                       - ctk_adjustment_get_value (priv->vadjustment));

      if (icon_view->priv->doing_rubberband)
        ctk_icon_view_update_rubberband (CTK_WIDGET (icon_view));

      _ctk_icon_view_accessible_adjustment_changed (icon_view);
    }
}

static gint
compare_sizes (gconstpointer p1,
	       gconstpointer p2,
               gpointer      unused)
{
  return GPOINTER_TO_INT (((const CtkRequestedSize *) p1)->data)
       - GPOINTER_TO_INT (((const CtkRequestedSize *) p2)->data);
}

static void
ctk_icon_view_layout (CtkIconView *icon_view)
{
  CtkIconViewPrivate *priv = icon_view->priv;
  CtkWidget *widget = CTK_WIDGET (icon_view);
  GList *items;
  gint item_width; /* this doesn't include item_padding */
  gint n_columns, n_rows, n_items;
  gint col, row;
  CtkRequestedSize *sizes;
  gboolean rtl;

  if (ctk_icon_view_is_empty (icon_view))
    return;

  rtl = ctk_widget_get_direction (CTK_WIDGET (icon_view)) == CTK_TEXT_DIR_RTL;
  n_items = ctk_icon_view_get_n_items (icon_view);

  ctk_icon_view_compute_n_items_for_size (icon_view, 
                                          CTK_ORIENTATION_HORIZONTAL,
                                          ctk_widget_get_allocated_width (widget),
                                          NULL, NULL,
                                          &n_columns, &item_width);
  n_rows = (n_items + n_columns - 1) / n_columns;

  priv->width = n_columns * (item_width + 2 * priv->item_padding + priv->column_spacing) - priv->column_spacing;
  priv->width += 2 * priv->margin;
  priv->width = MAX (priv->width, ctk_widget_get_allocated_width (widget));

  /* Clear the per row contexts */
  g_ptr_array_set_size (icon_view->priv->row_contexts, 0);

  ctk_cell_area_context_reset (priv->cell_area_context);
  /* because layouting is complicated. We designed an API
   * that is O(N²) and nonsensical.
   * And we're proud of it. */
  for (items = priv->items; items; items = items->next)
    {
      _ctk_icon_view_set_cell_data (icon_view, items->data);
      ctk_cell_area_get_preferred_width (priv->cell_area,
                                         priv->cell_area_context,
                                         widget,
                                         NULL, NULL);
    }

  sizes = g_newa (CtkRequestedSize, n_rows);
  items = priv->items;
  priv->height = priv->margin;

  /* Collect the heights for all rows */
  for (row = 0; row < n_rows; row++)
    {
      CtkCellAreaContext *context = ctk_cell_area_copy_context (priv->cell_area, priv->cell_area_context);
      g_ptr_array_add (priv->row_contexts, context);

      for (col = 0; col < n_columns && items; col++, items = items->next)
        {
          CtkIconViewItem *item = items->data;

          _ctk_icon_view_set_cell_data (icon_view, item);
          ctk_cell_area_get_preferred_height_for_width (priv->cell_area,
                                                        context,
                                                        widget,
                                                        item_width, 
                                                        NULL, NULL);
        }
      
      sizes[row].data = GINT_TO_POINTER (row);
      ctk_cell_area_context_get_preferred_height_for_width (context,
                                                            item_width,
                                                            &sizes[row].minimum_size,
                                                            &sizes[row].natural_size);
      priv->height += sizes[row].minimum_size + 2 * priv->item_padding + priv->row_spacing;
    }

  priv->height -= priv->row_spacing;
  priv->height += priv->margin;
  priv->height = MIN (priv->height, ctk_widget_get_allocated_height (widget));

  ctk_distribute_natural_allocation (ctk_widget_get_allocated_height (widget) - priv->height,
                                     n_rows,
                                     sizes);

  /* Actually allocate the rows */
  g_qsort_with_data (sizes, n_rows, sizeof (CtkRequestedSize), compare_sizes, NULL);
  
  items = priv->items;
  priv->height = priv->margin;

  for (row = 0; row < n_rows; row++)
    {
      CtkCellAreaContext *context = g_ptr_array_index (priv->row_contexts, row);
      ctk_cell_area_context_allocate (context, item_width, sizes[row].minimum_size);

      priv->height += priv->item_padding;

      for (col = 0; col < n_columns && items; col++, items = items->next)
        {
          CtkIconViewItem *item = items->data;

          item->cell_area.x = priv->margin + (col * 2 + 1) * priv->item_padding + col * (priv->column_spacing + item_width);
          item->cell_area.width = item_width;
          item->cell_area.y = priv->height;
          item->cell_area.height = sizes[row].minimum_size;
          item->row = row;
          item->col = col;
          if (rtl)
            {
              item->cell_area.x = priv->width - item_width - item->cell_area.x;
              item->col = n_columns - 1 - col;
            }
        }

      priv->height += sizes[row].minimum_size + priv->item_padding + priv->row_spacing;
    }

  priv->height -= priv->row_spacing;
  priv->height += priv->margin;
  priv->height = MAX (priv->height, ctk_widget_get_allocated_height (widget));
}

static void
ctk_icon_view_invalidate_sizes (CtkIconView *icon_view)
{
  /* Clear all item sizes */
  g_list_foreach (icon_view->priv->items,
		  (GFunc)ctk_icon_view_item_invalidate_size, NULL);

  /* Re-layout the items */
  ctk_widget_queue_resize (CTK_WIDGET (icon_view));
}

static void
ctk_icon_view_item_invalidate_size (CtkIconViewItem *item)
{
  item->cell_area.width = -1;
  item->cell_area.height = -1;
}

static void
ctk_icon_view_paint_item (CtkIconView     *icon_view,
                          cairo_t         *cr,
                          CtkIconViewItem *item,
                          gint             x,
                          gint             y,
                          gboolean         draw_focus)
{
  CdkRectangle cell_area;
  CtkStateFlags state = 0;
  CtkCellRendererState flags = 0;
  CtkStyleContext *style_context;
  CtkWidget *widget = CTK_WIDGET (icon_view);
  CtkIconViewPrivate *priv = icon_view->priv;
  CtkCellAreaContext *context;

  if (priv->model == NULL || item->cell_area.width <= 0 || item->cell_area.height <= 0)
    return;

  _ctk_icon_view_set_cell_data (icon_view, item);

  style_context = ctk_widget_get_style_context (widget);
  state = ctk_widget_get_state_flags (widget);

  ctk_style_context_save (style_context);
  ctk_style_context_add_class (style_context, CTK_STYLE_CLASS_CELL);

  state &= ~(CTK_STATE_FLAG_SELECTED | CTK_STATE_FLAG_PRELIGHT);

  if ((state & CTK_STATE_FLAG_FOCUSED) &&
      item == icon_view->priv->cursor_item)
    flags |= CTK_CELL_RENDERER_FOCUSED;

  if (item->selected)
    {
      state |= CTK_STATE_FLAG_SELECTED;
      flags |= CTK_CELL_RENDERER_SELECTED;
    }

  if (item == priv->last_prelight)
    {
      state |= CTK_STATE_FLAG_PRELIGHT;
      flags |= CTK_CELL_RENDERER_PRELIT;
    }

  ctk_style_context_set_state (style_context, state);

  ctk_render_background (style_context, cr,
                         x - priv->item_padding,
                         y - priv->item_padding,
                         item->cell_area.width  + priv->item_padding * 2,
                         item->cell_area.height + priv->item_padding * 2);
  ctk_render_frame (style_context, cr,
                    x - priv->item_padding,
                    y - priv->item_padding,
                    item->cell_area.width  + priv->item_padding * 2,
                    item->cell_area.height + priv->item_padding * 2);

  cell_area.x      = x;
  cell_area.y      = y;
  cell_area.width  = item->cell_area.width;
  cell_area.height = item->cell_area.height;

  context = g_ptr_array_index (priv->row_contexts, item->row);
  ctk_cell_area_render (priv->cell_area, context,
                        widget, cr, &cell_area, &cell_area, flags,
                        draw_focus);

  ctk_style_context_restore (style_context);
}

static void
ctk_icon_view_paint_rubberband (CtkIconView *icon_view,
				cairo_t     *cr)
{
  CtkIconViewPrivate *priv = icon_view->priv;
  CtkStyleContext *context;
  CdkRectangle rect;

  cairo_save (cr);

  rect.x = MIN (priv->rubberband_x1, priv->rubberband_x2);
  rect.y = MIN (priv->rubberband_y1, priv->rubberband_y2);
  rect.width = ABS (priv->rubberband_x1 - priv->rubberband_x2) + 1;
  rect.height = ABS (priv->rubberband_y1 - priv->rubberband_y2) + 1;

  context = ctk_widget_get_style_context (CTK_WIDGET (icon_view));

  ctk_style_context_save_to_node (context, priv->rubberband_node);

  cdk_cairo_rectangle (cr, &rect);
  cairo_clip (cr);

  ctk_render_background (context, cr,
                         rect.x, rect.y,
                         rect.width, rect.height);
  ctk_render_frame (context, cr,
                    rect.x, rect.y,
                    rect.width, rect.height);

  ctk_style_context_restore (context);
  cairo_restore (cr);
}

static void
ctk_icon_view_queue_draw_path (CtkIconView *icon_view,
			       CtkTreePath *path)
{
  GList *l;
  gint index;

  index = ctk_tree_path_get_indices (path)[0];

  for (l = icon_view->priv->items; l; l = l->next) 
    {
      CtkIconViewItem *item = l->data;

      if (item->index == index)
	{
	  ctk_icon_view_queue_draw_item (icon_view, item);
	  break;
	}
    }
}

static void
ctk_icon_view_queue_draw_item (CtkIconView     *icon_view,
			       CtkIconViewItem *item)
{
  CdkRectangle  rect;
  CdkRectangle *item_area = &item->cell_area;

  rect.x      = item_area->x - icon_view->priv->item_padding;
  rect.y      = item_area->y - icon_view->priv->item_padding;
  rect.width  = item_area->width  + icon_view->priv->item_padding * 2;
  rect.height = item_area->height + icon_view->priv->item_padding * 2;

  if (icon_view->priv->bin_window)
    cdk_window_invalidate_rect (icon_view->priv->bin_window, &rect, TRUE);
}

void
_ctk_icon_view_set_cursor_item (CtkIconView     *icon_view,
                                CtkIconViewItem *item,
                                CtkCellRenderer *cursor_cell)
{
  AtkObject *obj;
  AtkObject *item_obj;
  AtkObject *cursor_item_obj;

  /* When hitting this path from keynav, the focus cell is
   * already set, we dont need to notify the atk object
   * but we still need to queue the draw here (in the case
   * that the focus cell changes but not the cursor item).
   */
  ctk_icon_view_queue_draw_item (icon_view, item);

  if (icon_view->priv->cursor_item == item &&
      (cursor_cell == NULL || cursor_cell == ctk_cell_area_get_focus_cell (icon_view->priv->cell_area)))
    return;

  obj = ctk_widget_get_accessible (CTK_WIDGET (icon_view));
  if (icon_view->priv->cursor_item != NULL)
    {
      ctk_icon_view_queue_draw_item (icon_view, icon_view->priv->cursor_item);
      if (obj != NULL)
        {
          cursor_item_obj = atk_object_ref_accessible_child (obj, icon_view->priv->cursor_item->index);
          if (cursor_item_obj != NULL)
            atk_object_notify_state_change (cursor_item_obj, ATK_STATE_FOCUSED, FALSE);
        }
    }
  icon_view->priv->cursor_item = item;

  if (cursor_cell)
    ctk_cell_area_set_focus_cell (icon_view->priv->cell_area, cursor_cell);
  else
    {
      /* Make sure there is a cell in focus initially */
      if (!ctk_cell_area_get_focus_cell (icon_view->priv->cell_area))
	ctk_cell_area_focus (icon_view->priv->cell_area, CTK_DIR_TAB_FORWARD);
    }
  
  /* Notify that accessible focus object has changed */
  item_obj = atk_object_ref_accessible_child (obj, item->index);

  if (item_obj != NULL)
    {
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      atk_focus_tracker_notify (item_obj);
      G_GNUC_END_IGNORE_DEPRECATIONS;
      atk_object_notify_state_change (item_obj, ATK_STATE_FOCUSED, TRUE);
      g_object_unref (item_obj); 
    }
}


static CtkIconViewItem *
ctk_icon_view_item_new (void)
{
  CtkIconViewItem *item;

  item = g_slice_new0 (CtkIconViewItem);

  item->cell_area.width  = -1;
  item->cell_area.height = -1;
  
  return item;
}

static void
ctk_icon_view_item_free (CtkIconViewItem *item)
{
  g_return_if_fail (item != NULL);

  g_slice_free (CtkIconViewItem, item);
}

CtkIconViewItem *
_ctk_icon_view_get_item_at_coords (CtkIconView          *icon_view,
                                   gint                  x,
                                   gint                  y,
                                   gboolean              only_in_cell,
                                   CtkCellRenderer     **cell_at_pos)
{
  GList *items;

  if (cell_at_pos)
    *cell_at_pos = NULL;

  for (items = icon_view->priv->items; items; items = items->next)
    {
      CtkIconViewItem *item = items->data;
      CdkRectangle    *item_area = &item->cell_area;

      if (x >= item_area->x - icon_view->priv->column_spacing/2 && 
	  x <= item_area->x + item_area->width + icon_view->priv->column_spacing/2 &&
	  y >= item_area->y - icon_view->priv->row_spacing/2 && 
	  y <= item_area->y + item_area->height + icon_view->priv->row_spacing/2)
	{
	  if (only_in_cell || cell_at_pos)
	    {
	      CtkCellRenderer *cell = NULL;
	      CtkCellAreaContext *context;

	      context = g_ptr_array_index (icon_view->priv->row_contexts, item->row);
	      _ctk_icon_view_set_cell_data (icon_view, item);

	      if (x >= item_area->x && x <= item_area->x + item_area->width &&
		  y >= item_area->y && y <= item_area->y + item_area->height)
		cell = ctk_cell_area_get_cell_at_position (icon_view->priv->cell_area, context,
							   CTK_WIDGET (icon_view),
							   item_area,
							   x, y, NULL);

	      if (cell_at_pos)
		*cell_at_pos = cell;

	      if (only_in_cell)
		return cell != NULL ? item : NULL;
	      else
		return item;
	    }
	  return item;
	}
    }
  return NULL;
}

void
_ctk_icon_view_select_item (CtkIconView      *icon_view,
                            CtkIconViewItem  *item)
{
  g_return_if_fail (CTK_IS_ICON_VIEW (icon_view));
  g_return_if_fail (item != NULL);

  if (item->selected)
    return;
  
  if (icon_view->priv->selection_mode == CTK_SELECTION_NONE)
    return;
  else if (icon_view->priv->selection_mode != CTK_SELECTION_MULTIPLE)
    ctk_icon_view_unselect_all_internal (icon_view);

  item->selected = TRUE;

  ctk_icon_view_item_selected_changed (icon_view, item);
  g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);

  ctk_icon_view_queue_draw_item (icon_view, item);
}


void
_ctk_icon_view_unselect_item (CtkIconView      *icon_view,
                              CtkIconViewItem  *item)
{
  g_return_if_fail (CTK_IS_ICON_VIEW (icon_view));
  g_return_if_fail (item != NULL);

  if (!item->selected)
    return;
  
  if (icon_view->priv->selection_mode == CTK_SELECTION_NONE ||
      icon_view->priv->selection_mode == CTK_SELECTION_BROWSE)
    return;
  
  item->selected = FALSE;

  ctk_icon_view_item_selected_changed (icon_view, item);
  g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);

  ctk_icon_view_queue_draw_item (icon_view, item);
}

static void
verify_items (CtkIconView *icon_view)
{
  GList *items;
  int i = 0;

  for (items = icon_view->priv->items; items; items = items->next)
    {
      CtkIconViewItem *item = items->data;

      if (item->index != i)
	g_error ("List item does not match its index: "
		 "item index %d and list index %d\n", item->index, i);

      i++;
    }
}

static void
ctk_icon_view_row_changed (CtkTreeModel *model,
                           CtkTreePath  *path,
                           CtkTreeIter  *iter,
                           gpointer      data)
{
  CtkIconView *icon_view = CTK_ICON_VIEW (data);

  /* ignore changes in branches */
  if (ctk_tree_path_get_depth (path) > 1)
    return;

  /* An icon view subclass might add it's own model and populate
   * things at init() time instead of waiting for the constructor() 
   * to be called 
   */
  if (icon_view->priv->cell_area)
    ctk_cell_area_stop_editing (icon_view->priv->cell_area, TRUE);

  /* Here we can use a "grow-only" strategy for optimization
   * and only invalidate a single item and queue a relayout
   * instead of invalidating the whole thing.
   *
   * For now CtkIconView still cant deal with huge models
   * so just invalidate the whole thing when the model
   * changes.
   */
  ctk_icon_view_invalidate_sizes (icon_view);

  verify_items (icon_view);
}

static void
ctk_icon_view_row_inserted (CtkTreeModel *model,
			    CtkTreePath  *path,
			    CtkTreeIter  *iter,
			    gpointer      data)
{
  CtkIconView *icon_view = CTK_ICON_VIEW (data);
  gint index;
  CtkIconViewItem *item;
  GList *list;

  /* ignore changes in branches */
  if (ctk_tree_path_get_depth (path) > 1)
    return;

  ctk_tree_model_ref_node (model, iter);

  index = ctk_tree_path_get_indices(path)[0];

  item = ctk_icon_view_item_new ();

  item->index = index;

  /* FIXME: We can be more efficient here,
     we can store a tail pointer and use that when
     appending (which is a rather common operation)
  */
  icon_view->priv->items = g_list_insert (icon_view->priv->items,
					 item, index);
  
  list = g_list_nth (icon_view->priv->items, index + 1);
  for (; list; list = list->next)
    {
      item = list->data;

      item->index++;
    }
    
  verify_items (icon_view);

  ctk_widget_queue_resize (CTK_WIDGET (icon_view));
}

static void
ctk_icon_view_row_deleted (CtkTreeModel *model,
			   CtkTreePath  *path,
			   gpointer      data)
{
  CtkIconView *icon_view = CTK_ICON_VIEW (data);
  gint index;
  CtkIconViewItem *item;
  GList *list, *next;
  gboolean emit = FALSE;
  CtkTreeIter iter;

  /* ignore changes in branches */
  if (ctk_tree_path_get_depth (path) > 1)
    return;

  if (ctk_tree_model_get_iter (model, &iter, path))
    ctk_tree_model_unref_node (model, &iter);

  index = ctk_tree_path_get_indices(path)[0];

  list = g_list_nth (icon_view->priv->items, index);
  item = list->data;

  if (icon_view->priv->cell_area)
    ctk_cell_area_stop_editing (icon_view->priv->cell_area, TRUE);

  if (item == icon_view->priv->anchor_item)
    icon_view->priv->anchor_item = NULL;

  if (item == icon_view->priv->cursor_item)
    icon_view->priv->cursor_item = NULL;

  if (item == icon_view->priv->last_prelight)
    icon_view->priv->last_prelight = NULL;

  if (item->selected)
    emit = TRUE;
  
  ctk_icon_view_item_free (item);

  for (next = list->next; next; next = next->next)
    {
      item = next->data;

      item->index--;
    }
  
  icon_view->priv->items = g_list_delete_link (icon_view->priv->items, list);

  verify_items (icon_view);  
  
  ctk_widget_queue_resize (CTK_WIDGET (icon_view));

  if (emit)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}

static void
ctk_icon_view_rows_reordered (CtkTreeModel *model,
			      CtkTreePath  *parent,
			      CtkTreeIter  *iter,
			      gint         *new_order,
			      gpointer      data)
{
  CtkIconView *icon_view = CTK_ICON_VIEW (data);
  int i;
  int length;
  GList *items = NULL, *list;
  CtkIconViewItem **item_array;
  gint *order;

  /* ignore changes in branches */
  if (iter != NULL)
    return;

  if (icon_view->priv->cell_area)
    ctk_cell_area_stop_editing (icon_view->priv->cell_area, TRUE);

  length = ctk_tree_model_iter_n_children (model, NULL);

  order = g_new (gint, length);
  for (i = 0; i < length; i++)
    order [new_order[i]] = i;

  item_array = g_new (CtkIconViewItem *, length);
  for (i = 0, list = icon_view->priv->items; list != NULL; list = list->next, i++)
    item_array[order[i]] = list->data;
  g_free (order);

  for (i = length - 1; i >= 0; i--)
    {
      item_array[i]->index = i;
      items = g_list_prepend (items, item_array[i]);
    }
  
  g_free (item_array);
  g_list_free (icon_view->priv->items);
  icon_view->priv->items = items;

  ctk_widget_queue_resize (CTK_WIDGET (icon_view));

  verify_items (icon_view);  
}

static void
ctk_icon_view_build_items (CtkIconView *icon_view)
{
  CtkTreeIter iter;
  int i;
  GList *items = NULL;

  if (!ctk_tree_model_get_iter_first (icon_view->priv->model,
				      &iter))
    return;

  i = 0;
  
  do
    {
      CtkIconViewItem *item = ctk_icon_view_item_new ();

      item->index = i;
      
      i++;

      items = g_list_prepend (items, item);
      
    } while (ctk_tree_model_iter_next (icon_view->priv->model, &iter));

  icon_view->priv->items = g_list_reverse (items);
}

static void
ctk_icon_view_add_move_binding (CtkBindingSet  *binding_set,
				guint           keyval,
				guint           modmask,
				CtkMovementStep step,
				gint            count)
{
  
  ctk_binding_entry_add_signal (binding_set, keyval, modmask,
                                I_("move-cursor"), 2,
                                G_TYPE_ENUM, step,
                                G_TYPE_INT, count);

  ctk_binding_entry_add_signal (binding_set, keyval, GDK_SHIFT_MASK,
                                "move-cursor", 2,
                                G_TYPE_ENUM, step,
                                G_TYPE_INT, count);

  if ((modmask & GDK_CONTROL_MASK) == GDK_CONTROL_MASK)
   return;

  ctk_binding_entry_add_signal (binding_set, keyval, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
                                "move-cursor", 2,
                                G_TYPE_ENUM, step,
                                G_TYPE_INT, count);

  ctk_binding_entry_add_signal (binding_set, keyval, GDK_CONTROL_MASK,
                                "move-cursor", 2,
                                G_TYPE_ENUM, step,
                                G_TYPE_INT, count);
}

static gboolean
ctk_icon_view_real_move_cursor (CtkIconView     *icon_view,
				CtkMovementStep  step,
				gint             count)
{
  CdkModifierType state;

  g_return_val_if_fail (CTK_ICON_VIEW (icon_view), FALSE);
  g_return_val_if_fail (step == CTK_MOVEMENT_LOGICAL_POSITIONS ||
			step == CTK_MOVEMENT_VISUAL_POSITIONS ||
			step == CTK_MOVEMENT_DISPLAY_LINES ||
			step == CTK_MOVEMENT_PAGES ||
			step == CTK_MOVEMENT_BUFFER_ENDS, FALSE);

  if (!ctk_widget_has_focus (CTK_WIDGET (icon_view)))
    return FALSE;

  ctk_cell_area_stop_editing (icon_view->priv->cell_area, FALSE);
  ctk_widget_grab_focus (CTK_WIDGET (icon_view));

  if (ctk_get_current_event_state (&state))
    {
      CdkModifierType extend_mod_mask;
      CdkModifierType modify_mod_mask;

      extend_mod_mask =
        ctk_widget_get_modifier_mask (CTK_WIDGET (icon_view),
                                      GDK_MODIFIER_INTENT_EXTEND_SELECTION);
      modify_mod_mask =
        ctk_widget_get_modifier_mask (CTK_WIDGET (icon_view),
                                      GDK_MODIFIER_INTENT_MODIFY_SELECTION);

      if ((state & modify_mod_mask) == modify_mod_mask)
        icon_view->priv->modify_selection_pressed = TRUE;
      if ((state & extend_mod_mask) == extend_mod_mask)
        icon_view->priv->extend_selection_pressed = TRUE;
    }
  /* else we assume not pressed */

  switch (step)
    {
    case CTK_MOVEMENT_LOGICAL_POSITIONS:
    case CTK_MOVEMENT_VISUAL_POSITIONS:
      ctk_icon_view_move_cursor_left_right (icon_view, count);
      break;
    case CTK_MOVEMENT_DISPLAY_LINES:
      ctk_icon_view_move_cursor_up_down (icon_view, count);
      break;
    case CTK_MOVEMENT_PAGES:
      ctk_icon_view_move_cursor_page_up_down (icon_view, count);
      break;
    case CTK_MOVEMENT_BUFFER_ENDS:
      ctk_icon_view_move_cursor_start_end (icon_view, count);
      break;
    default:
      g_assert_not_reached ();
    }

  icon_view->priv->modify_selection_pressed = FALSE;
  icon_view->priv->extend_selection_pressed = FALSE;

  icon_view->priv->draw_focus = TRUE;

  return TRUE;
}

static CtkIconViewItem *
find_item (CtkIconView     *icon_view,
	   CtkIconViewItem *current,
	   gint             row_ofs,
	   gint             col_ofs)
{
  gint row, col;
  GList *items;
  CtkIconViewItem *item;

  /* FIXME: this could be more efficient 
   */
  row = current->row + row_ofs;
  col = current->col + col_ofs;

  for (items = icon_view->priv->items; items; items = items->next)
    {
      item = items->data;
      if (item->row == row && item->col == col)
	return item;
    }
  
  return NULL;
}

static CtkIconViewItem *
find_item_page_up_down (CtkIconView     *icon_view,
			CtkIconViewItem *current,
			gint             count)
{
  GList *item, *next;
  gint y, col;
  
  col = current->col;
  y = current->cell_area.y + count * ctk_adjustment_get_page_size (icon_view->priv->vadjustment);

  item = g_list_find (icon_view->priv->items, current);
  if (count > 0)
    {
      while (item)
	{
	  for (next = item->next; next; next = next->next)
	    {
	      if (((CtkIconViewItem *)next->data)->col == col)
		break;
	    }
	  if (!next || ((CtkIconViewItem *)next->data)->cell_area.y > y)
	    break;

	  item = next;
	}
    }
  else 
    {
      while (item)
	{
	  for (next = item->prev; next; next = next->prev)
	    {
	      if (((CtkIconViewItem *)next->data)->col == col)
		break;
	    }
	  if (!next || ((CtkIconViewItem *)next->data)->cell_area.y < y)
	    break;

	  item = next;
	}
    }

  if (item)
    return item->data;

  return NULL;
}

static gboolean
ctk_icon_view_select_all_between (CtkIconView     *icon_view,
				  CtkIconViewItem *anchor,
				  CtkIconViewItem *cursor)
{
  GList *items;
  CtkIconViewItem *item;
  gint row1, row2, col1, col2;
  gboolean dirty = FALSE;
  
  if (anchor->row < cursor->row)
    {
      row1 = anchor->row;
      row2 = cursor->row;
    }
  else
    {
      row1 = cursor->row;
      row2 = anchor->row;
    }

  if (anchor->col < cursor->col)
    {
      col1 = anchor->col;
      col2 = cursor->col;
    }
  else
    {
      col1 = cursor->col;
      col2 = anchor->col;
    }

  for (items = icon_view->priv->items; items; items = items->next)
    {
      item = items->data;

      if (row1 <= item->row && item->row <= row2 &&
	  col1 <= item->col && item->col <= col2)
	{
	  if (!item->selected)
	    {
	      dirty = TRUE;
	      item->selected = TRUE;
	      ctk_icon_view_item_selected_changed (icon_view, item);
	    }
	  ctk_icon_view_queue_draw_item (icon_view, item);
	}
    }

  return dirty;
}

static void 
ctk_icon_view_move_cursor_up_down (CtkIconView *icon_view,
				   gint         count)
{
  CtkIconViewItem *item;
  CtkCellRenderer *cell = NULL;
  gboolean dirty = FALSE;
  gint step;
  CtkDirectionType direction;

  if (!ctk_widget_has_focus (CTK_WIDGET (icon_view)))
    return;

  direction = count < 0 ? CTK_DIR_UP : CTK_DIR_DOWN;

  if (!icon_view->priv->cursor_item)
    {
      GList *list;

      if (count > 0)
	list = icon_view->priv->items;
      else
	list = g_list_last (icon_view->priv->items);

      if (list)
        {
          item = list->data;

          /* Give focus to the first cell initially */
          _ctk_icon_view_set_cell_data (icon_view, item);
          ctk_cell_area_focus (icon_view->priv->cell_area, direction);
        }
      else
        {
          item = NULL;
        }
    }
  else
    {
      item = icon_view->priv->cursor_item;
      step = count > 0 ? 1 : -1;      

      /* Save the current focus cell in case we hit the edge */
      cell = ctk_cell_area_get_focus_cell (icon_view->priv->cell_area);

      while (item)
	{
	  _ctk_icon_view_set_cell_data (icon_view, item);

	  if (ctk_cell_area_focus (icon_view->priv->cell_area, direction))
	    break;

	  item = find_item (icon_view, item, step, 0);
	}
    }

  if (!item)
    {
      if (!ctk_widget_keynav_failed (CTK_WIDGET (icon_view), direction))
        {
          CtkWidget *toplevel = ctk_widget_get_toplevel (CTK_WIDGET (icon_view));
          if (toplevel)
            ctk_widget_child_focus (toplevel,
                                    direction == CTK_DIR_UP ?
                                    CTK_DIR_TAB_BACKWARD :
                                    CTK_DIR_TAB_FORWARD);

        }

      ctk_cell_area_set_focus_cell (icon_view->priv->cell_area, cell);
      return;
    }

  if (icon_view->priv->modify_selection_pressed ||
      !icon_view->priv->extend_selection_pressed ||
      !icon_view->priv->anchor_item ||
      icon_view->priv->selection_mode != CTK_SELECTION_MULTIPLE)
    icon_view->priv->anchor_item = item;

  cell = ctk_cell_area_get_focus_cell (icon_view->priv->cell_area);
  _ctk_icon_view_set_cursor_item (icon_view, item, cell);

  if (!icon_view->priv->modify_selection_pressed &&
      icon_view->priv->selection_mode != CTK_SELECTION_NONE)
    {
      dirty = ctk_icon_view_unselect_all_internal (icon_view);
      dirty = ctk_icon_view_select_all_between (icon_view, 
						icon_view->priv->anchor_item,
						item) || dirty;
    }

  ctk_icon_view_scroll_to_item (icon_view, item);

  if (dirty)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}

static void 
ctk_icon_view_move_cursor_page_up_down (CtkIconView *icon_view,
					gint         count)
{
  CtkIconViewItem *item;
  gboolean dirty = FALSE;
  
  if (!ctk_widget_has_focus (CTK_WIDGET (icon_view)))
    return;
  
  if (!icon_view->priv->cursor_item)
    {
      GList *list;

      if (count > 0)
	list = icon_view->priv->items;
      else
	list = g_list_last (icon_view->priv->items);

      item = list ? list->data : NULL;
    }
  else
    item = find_item_page_up_down (icon_view, 
				   icon_view->priv->cursor_item,
				   count);

  if (item == icon_view->priv->cursor_item)
    ctk_widget_error_bell (CTK_WIDGET (icon_view));

  if (!item)
    return;

  if (icon_view->priv->modify_selection_pressed ||
      !icon_view->priv->extend_selection_pressed ||
      !icon_view->priv->anchor_item ||
      icon_view->priv->selection_mode != CTK_SELECTION_MULTIPLE)
    icon_view->priv->anchor_item = item;

  _ctk_icon_view_set_cursor_item (icon_view, item, NULL);

  if (!icon_view->priv->modify_selection_pressed &&
      icon_view->priv->selection_mode != CTK_SELECTION_NONE)
    {
      dirty = ctk_icon_view_unselect_all_internal (icon_view);
      dirty = ctk_icon_view_select_all_between (icon_view, 
						icon_view->priv->anchor_item,
						item) || dirty;
    }

  ctk_icon_view_scroll_to_item (icon_view, item);

  if (dirty)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);  
}

static void 
ctk_icon_view_move_cursor_left_right (CtkIconView *icon_view,
				      gint         count)
{
  CtkIconViewItem *item;
  CtkCellRenderer *cell = NULL;
  gboolean dirty = FALSE;
  gint step;
  CtkDirectionType direction;

  if (!ctk_widget_has_focus (CTK_WIDGET (icon_view)))
    return;

  direction = count < 0 ? CTK_DIR_LEFT : CTK_DIR_RIGHT;

  if (!icon_view->priv->cursor_item)
    {
      GList *list;

      if (count > 0)
	list = icon_view->priv->items;
      else
	list = g_list_last (icon_view->priv->items);

      if (list)
        {
          item = list->data;

          /* Give focus to the first cell initially */
          _ctk_icon_view_set_cell_data (icon_view, item);
          ctk_cell_area_focus (icon_view->priv->cell_area, direction);
        }
      else
        {
          item = NULL;
        }
    }
  else
    {
      item = icon_view->priv->cursor_item;
      step = count > 0 ? 1 : -1;

      /* Save the current focus cell in case we hit the edge */
      cell = ctk_cell_area_get_focus_cell (icon_view->priv->cell_area);

      while (item)
	{
	  _ctk_icon_view_set_cell_data (icon_view, item);

	  if (ctk_cell_area_focus (icon_view->priv->cell_area, direction))
	    break;
	  
	  item = find_item (icon_view, item, 0, step);
	}
    }

  if (!item)
    {
      if (!ctk_widget_keynav_failed (CTK_WIDGET (icon_view), direction))
        {
          CtkWidget *toplevel = ctk_widget_get_toplevel (CTK_WIDGET (icon_view));
          if (toplevel)
            ctk_widget_child_focus (toplevel,
                                    direction == CTK_DIR_LEFT ?
                                    CTK_DIR_TAB_BACKWARD :
                                    CTK_DIR_TAB_FORWARD);

        }

      ctk_cell_area_set_focus_cell (icon_view->priv->cell_area, cell);
      return;
    }

  if (icon_view->priv->modify_selection_pressed ||
      !icon_view->priv->extend_selection_pressed ||
      !icon_view->priv->anchor_item ||
      icon_view->priv->selection_mode != CTK_SELECTION_MULTIPLE)
    icon_view->priv->anchor_item = item;

  cell = ctk_cell_area_get_focus_cell (icon_view->priv->cell_area);
  _ctk_icon_view_set_cursor_item (icon_view, item, cell);

  if (!icon_view->priv->modify_selection_pressed &&
      icon_view->priv->selection_mode != CTK_SELECTION_NONE)
    {
      dirty = ctk_icon_view_unselect_all_internal (icon_view);
      dirty = ctk_icon_view_select_all_between (icon_view, 
						icon_view->priv->anchor_item,
						item) || dirty;
    }

  ctk_icon_view_scroll_to_item (icon_view, item);

  if (dirty)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}

static void 
ctk_icon_view_move_cursor_start_end (CtkIconView *icon_view,
				     gint         count)
{
  CtkIconViewItem *item;
  GList *list;
  gboolean dirty = FALSE;
  
  if (!ctk_widget_has_focus (CTK_WIDGET (icon_view)))
    return;
  
  if (count < 0)
    list = icon_view->priv->items;
  else
    list = g_list_last (icon_view->priv->items);
  
  item = list ? list->data : NULL;

  if (item == icon_view->priv->cursor_item)
    ctk_widget_error_bell (CTK_WIDGET (icon_view));

  if (!item)
    return;

  if (icon_view->priv->modify_selection_pressed ||
      !icon_view->priv->extend_selection_pressed ||
      !icon_view->priv->anchor_item ||
      icon_view->priv->selection_mode != CTK_SELECTION_MULTIPLE)
    icon_view->priv->anchor_item = item;

  _ctk_icon_view_set_cursor_item (icon_view, item, NULL);

  if (!icon_view->priv->modify_selection_pressed &&
      icon_view->priv->selection_mode != CTK_SELECTION_NONE)
    {
      dirty = ctk_icon_view_unselect_all_internal (icon_view);
      dirty = ctk_icon_view_select_all_between (icon_view, 
						icon_view->priv->anchor_item,
						item) || dirty;
    }

  ctk_icon_view_scroll_to_item (icon_view, item);

  if (dirty)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}

/**
 * ctk_icon_view_scroll_to_path:
 * @icon_view: A #CtkIconView.
 * @path: The path of the item to move to.
 * @use_align: whether to use alignment arguments, or %FALSE.
 * @row_align: The vertical alignment of the item specified by @path.
 * @col_align: The horizontal alignment of the item specified by @path.
 *
 * Moves the alignments of @icon_view to the position specified by @path.  
 * @row_align determines where the row is placed, and @col_align determines 
 * where @column is placed.  Both are expected to be between 0.0 and 1.0. 
 * 0.0 means left/top alignment, 1.0 means right/bottom alignment, 0.5 means 
 * center.
 *
 * If @use_align is %FALSE, then the alignment arguments are ignored, and the
 * tree does the minimum amount of work to scroll the item onto the screen.
 * This means that the item will be scrolled to the edge closest to its current
 * position.  If the item is currently visible on the screen, nothing is done.
 *
 * This function only works if the model is set, and @path is a valid row on 
 * the model. If the model changes before the @icon_view is realized, the 
 * centered path will be modified to reflect this change.
 *
 * Since: 2.8
 **/
void
ctk_icon_view_scroll_to_path (CtkIconView *icon_view,
			      CtkTreePath *path,
			      gboolean     use_align,
			      gfloat       row_align,
			      gfloat       col_align)
{
  CtkIconViewItem *item = NULL;
  CtkWidget *widget;

  g_return_if_fail (CTK_IS_ICON_VIEW (icon_view));
  g_return_if_fail (path != NULL);
  g_return_if_fail (row_align >= 0.0 && row_align <= 1.0);
  g_return_if_fail (col_align >= 0.0 && col_align <= 1.0);

  widget = CTK_WIDGET (icon_view);

  if (ctk_tree_path_get_depth (path) > 0)
    item = g_list_nth_data (icon_view->priv->items,
			    ctk_tree_path_get_indices(path)[0]);
  
  if (!item || item->cell_area.width < 0 ||
      !ctk_widget_get_realized (widget))
    {
      if (icon_view->priv->scroll_to_path)
	ctk_tree_row_reference_free (icon_view->priv->scroll_to_path);

      icon_view->priv->scroll_to_path = NULL;

      if (path)
	icon_view->priv->scroll_to_path = ctk_tree_row_reference_new_proxy (G_OBJECT (icon_view), icon_view->priv->model, path);

      icon_view->priv->scroll_to_use_align = use_align;
      icon_view->priv->scroll_to_row_align = row_align;
      icon_view->priv->scroll_to_col_align = col_align;

      return;
    }

  if (use_align)
    {
      CtkAllocation allocation;
      gint x, y;
      gfloat offset;
      CdkRectangle item_area = 
	{ 
	  item->cell_area.x - icon_view->priv->item_padding, 
	  item->cell_area.y - icon_view->priv->item_padding, 
	  item->cell_area.width  + icon_view->priv->item_padding * 2, 
	  item->cell_area.height + icon_view->priv->item_padding * 2 
	};

      cdk_window_get_position (icon_view->priv->bin_window, &x, &y);

      ctk_widget_get_allocation (widget, &allocation);

      offset = y + item_area.y - row_align * (allocation.height - item_area.height);

      ctk_adjustment_set_value (icon_view->priv->vadjustment,
                                ctk_adjustment_get_value (icon_view->priv->vadjustment) + offset);

      offset = x + item_area.x - col_align * (allocation.width - item_area.width);

      ctk_adjustment_set_value (icon_view->priv->hadjustment,
                                ctk_adjustment_get_value (icon_view->priv->hadjustment) + offset);
    }
  else
    ctk_icon_view_scroll_to_item (icon_view, item);
}


static void
ctk_icon_view_scroll_to_item (CtkIconView     *icon_view,
                              CtkIconViewItem *item)
{
  CtkIconViewPrivate *priv = icon_view->priv;
  CtkWidget *widget = CTK_WIDGET (icon_view);
  CtkAdjustment *hadj, *vadj;
  CtkAllocation allocation;
  gint x, y;
  CdkRectangle item_area;

  item_area.x = item->cell_area.x - priv->item_padding;
  item_area.y = item->cell_area.y - priv->item_padding;
  item_area.width = item->cell_area.width  + priv->item_padding * 2;
  item_area.height = item->cell_area.height + priv->item_padding * 2;

  cdk_window_get_position (icon_view->priv->bin_window, &x, &y);
  ctk_widget_get_allocation (widget, &allocation);

  hadj = icon_view->priv->hadjustment;
  vadj = icon_view->priv->vadjustment;

  if (y + item_area.y < 0)
    ctk_adjustment_animate_to_value (vadj,
                                     ctk_adjustment_get_value (vadj)
                                     + y + item_area.y);
  else if (y + item_area.y + item_area.height > allocation.height)
    ctk_adjustment_animate_to_value (vadj,
                                     ctk_adjustment_get_value (vadj)
                                     + y + item_area.y + item_area.height - allocation.height);

  if (x + item_area.x < 0)
    ctk_adjustment_animate_to_value (hadj,
                                     ctk_adjustment_get_value (hadj)
                                     + x + item_area.x);
  else if (x + item_area.x + item_area.width > allocation.width)
    ctk_adjustment_animate_to_value (hadj,
                                     ctk_adjustment_get_value (hadj)
                                     + x + item_area.x + item_area.width - allocation.width);
}

/* CtkCellLayout implementation */

static void
ctk_icon_view_ensure_cell_area (CtkIconView *icon_view,
                                CtkCellArea *cell_area)
{
  CtkIconViewPrivate *priv = icon_view->priv;

  if (priv->cell_area)
    return;

  if (cell_area)
    priv->cell_area = cell_area;
  else
    priv->cell_area = ctk_cell_area_box_new ();

  g_object_ref_sink (priv->cell_area);

  if (CTK_IS_ORIENTABLE (priv->cell_area))
    ctk_orientable_set_orientation (CTK_ORIENTABLE (priv->cell_area), priv->item_orientation);

  priv->cell_area_context = ctk_cell_area_create_context (priv->cell_area);

  priv->add_editable_id =
    g_signal_connect (priv->cell_area, "add-editable",
                      G_CALLBACK (ctk_icon_view_add_editable), icon_view);
  priv->remove_editable_id =
    g_signal_connect (priv->cell_area, "remove-editable",
                      G_CALLBACK (ctk_icon_view_remove_editable), icon_view);

  update_text_cell (icon_view);
  update_pixbuf_cell (icon_view);
}

static CtkCellArea *
ctk_icon_view_cell_layout_get_area (CtkCellLayout *cell_layout)
{
  CtkIconView *icon_view = CTK_ICON_VIEW (cell_layout);
  CtkIconViewPrivate *priv = icon_view->priv;

  if (G_UNLIKELY (!priv->cell_area))
    ctk_icon_view_ensure_cell_area (icon_view, NULL);

  return icon_view->priv->cell_area;
}

void
_ctk_icon_view_set_cell_data (CtkIconView     *icon_view,
			      CtkIconViewItem *item)
{
  CtkTreeIter iter;
  CtkTreePath *path;

  path = ctk_tree_path_new_from_indices (item->index, -1);
  if (!ctk_tree_model_get_iter (icon_view->priv->model, &iter, path))
    return;
  ctk_tree_path_free (path);

  ctk_cell_area_apply_attributes (icon_view->priv->cell_area, 
				  icon_view->priv->model,
				  &iter, FALSE, FALSE);
}



/* Public API */


/**
 * ctk_icon_view_new:
 * 
 * Creates a new #CtkIconView widget
 * 
 * Returns: A newly created #CtkIconView widget
 *
 * Since: 2.6
 **/
CtkWidget *
ctk_icon_view_new (void)
{
  return g_object_new (CTK_TYPE_ICON_VIEW, NULL);
}

/**
 * ctk_icon_view_new_with_area:
 * @area: the #CtkCellArea to use to layout cells
 * 
 * Creates a new #CtkIconView widget using the
 * specified @area to layout cells inside the icons.
 * 
 * Returns: A newly created #CtkIconView widget
 *
 * Since: 3.0
 **/
CtkWidget *
ctk_icon_view_new_with_area (CtkCellArea *area)
{
  return g_object_new (CTK_TYPE_ICON_VIEW, "cell-area", area, NULL);
}

/**
 * ctk_icon_view_new_with_model:
 * @model: The model.
 * 
 * Creates a new #CtkIconView widget with the model @model.
 * 
 * Returns: A newly created #CtkIconView widget.
 *
 * Since: 2.6 
 **/
CtkWidget *
ctk_icon_view_new_with_model (CtkTreeModel *model)
{
  return g_object_new (CTK_TYPE_ICON_VIEW, "model", model, NULL);
}

/**
 * ctk_icon_view_convert_widget_to_bin_window_coords:
 * @icon_view: a #CtkIconView 
 * @wx: X coordinate relative to the widget
 * @wy: Y coordinate relative to the widget
 * @bx: (out): return location for bin_window X coordinate
 * @by: (out): return location for bin_window Y coordinate
 * 
 * Converts widget coordinates to coordinates for the bin_window,
 * as expected by e.g. ctk_icon_view_get_path_at_pos(). 
 *
 * Since: 2.12
 */
void
ctk_icon_view_convert_widget_to_bin_window_coords (CtkIconView *icon_view,
                                                   gint         wx,
                                                   gint         wy, 
                                                   gint        *bx,
                                                   gint        *by)
{
  gint x, y;

  g_return_if_fail (CTK_IS_ICON_VIEW (icon_view));

  if (icon_view->priv->bin_window) 
    cdk_window_get_position (icon_view->priv->bin_window, &x, &y);
  else
    x = y = 0;
 
  if (bx)
    *bx = wx - x;
  if (by)
    *by = wy - y;
}

/**
 * ctk_icon_view_get_path_at_pos:
 * @icon_view: A #CtkIconView.
 * @x: The x position to be identified
 * @y: The y position to be identified
 * 
 * Finds the path at the point (@x, @y), relative to bin_window coordinates.
 * See ctk_icon_view_get_item_at_pos(), if you are also interested in
 * the cell at the specified position. 
 * See ctk_icon_view_convert_widget_to_bin_window_coords() for converting
 * widget coordinates to bin_window coordinates.
 * 
 * Returns: (nullable) (transfer full): The #CtkTreePath corresponding
 * to the icon or %NULL if no icon exists at that position.
 *
 * Since: 2.6 
 **/
CtkTreePath *
ctk_icon_view_get_path_at_pos (CtkIconView *icon_view,
			       gint         x,
			       gint         y)
{
  CtkIconViewItem *item;
  CtkTreePath *path;
  
  g_return_val_if_fail (CTK_IS_ICON_VIEW (icon_view), NULL);

  item = _ctk_icon_view_get_item_at_coords (icon_view, x, y, TRUE, NULL);

  if (!item)
    return NULL;

  path = ctk_tree_path_new_from_indices (item->index, -1);

  return path;
}

/**
 * ctk_icon_view_get_item_at_pos:
 * @icon_view: A #CtkIconView.
 * @x: The x position to be identified
 * @y: The y position to be identified
 * @path: (out) (allow-none): Return location for the path, or %NULL
 * @cell: (out) (allow-none) (transfer none): Return location for the renderer
 *   responsible for the cell at (@x, @y), or %NULL
 * 
 * Finds the path at the point (@x, @y), relative to bin_window coordinates.
 * In contrast to ctk_icon_view_get_path_at_pos(), this function also 
 * obtains the cell at the specified position. The returned path should
 * be freed with ctk_tree_path_free().
 * See ctk_icon_view_convert_widget_to_bin_window_coords() for converting
 * widget coordinates to bin_window coordinates.
 * 
 * Returns: %TRUE if an item exists at the specified position
 *
 * Since: 2.8
 **/
gboolean 
ctk_icon_view_get_item_at_pos (CtkIconView      *icon_view,
			       gint              x,
			       gint              y,
			       CtkTreePath     **path,
			       CtkCellRenderer **cell)
{
  CtkIconViewItem *item;
  CtkCellRenderer *renderer = NULL;
  
  g_return_val_if_fail (CTK_IS_ICON_VIEW (icon_view), FALSE);

  item = _ctk_icon_view_get_item_at_coords (icon_view, x, y, TRUE, &renderer);

  if (path != NULL)
    {
      if (item != NULL)
	*path = ctk_tree_path_new_from_indices (item->index, -1);
      else
	*path = NULL;
    }

  if (cell != NULL)
    *cell = renderer;

  return (item != NULL);
}

/**
 * ctk_icon_view_get_cell_rect:
 * @icon_view: a #CtkIconView
 * @path: a #CtkTreePath
 * @cell: (allow-none): a #CtkCellRenderer or %NULL
 * @rect: (out): rectangle to fill with cell rect
 *
 * Fills the bounding rectangle in widget coordinates for the cell specified by
 * @path and @cell. If @cell is %NULL the main cell area is used.
 *
 * This function is only valid if @icon_view is realized.
 *
 * Returns: %FALSE if there is no such item, %TRUE otherwise
 *
 * Since: 3.6
 */
gboolean
ctk_icon_view_get_cell_rect (CtkIconView     *icon_view,
                             CtkTreePath     *path,
                             CtkCellRenderer *cell,
                             CdkRectangle    *rect)
{
  CtkIconViewItem *item = NULL;
  gint x, y;

  g_return_val_if_fail (CTK_IS_ICON_VIEW (icon_view), FALSE);
  g_return_val_if_fail (cell == NULL || CTK_IS_CELL_RENDERER (cell), FALSE);

  if (ctk_tree_path_get_depth (path) > 0)
    item = g_list_nth_data (icon_view->priv->items,
			    ctk_tree_path_get_indices(path)[0]);

  if (!item)
    return FALSE;

  if (cell)
    {
      CtkCellAreaContext *context;

      context = g_ptr_array_index (icon_view->priv->row_contexts, item->row);
      _ctk_icon_view_set_cell_data (icon_view, item);
      ctk_cell_area_get_cell_allocation (icon_view->priv->cell_area, context,
					 CTK_WIDGET (icon_view),
					 cell, &item->cell_area, rect);
    }
  else
    {
      rect->x = item->cell_area.x - icon_view->priv->item_padding;
      rect->y = item->cell_area.y - icon_view->priv->item_padding;
      rect->width  = item->cell_area.width  + icon_view->priv->item_padding * 2;
      rect->height = item->cell_area.height + icon_view->priv->item_padding * 2;
    }

  if (icon_view->priv->bin_window)
    {
      cdk_window_get_position (icon_view->priv->bin_window, &x, &y);
      rect->x += x;
      rect->y += y;
    }

  return TRUE;
}

/**
 * ctk_icon_view_set_tooltip_item:
 * @icon_view: a #CtkIconView
 * @tooltip: a #CtkTooltip
 * @path: a #CtkTreePath
 * 
 * Sets the tip area of @tooltip to be the area covered by the item at @path.
 * See also ctk_icon_view_set_tooltip_column() for a simpler alternative.
 * See also ctk_tooltip_set_tip_area().
 * 
 * Since: 2.12
 */
void 
ctk_icon_view_set_tooltip_item (CtkIconView     *icon_view,
                                CtkTooltip      *tooltip,
                                CtkTreePath     *path)
{
  g_return_if_fail (CTK_IS_ICON_VIEW (icon_view));
  g_return_if_fail (CTK_IS_TOOLTIP (tooltip));

  ctk_icon_view_set_tooltip_cell (icon_view, tooltip, path, NULL);
}

/**
 * ctk_icon_view_set_tooltip_cell:
 * @icon_view: a #CtkIconView
 * @tooltip: a #CtkTooltip
 * @path: a #CtkTreePath
 * @cell: (allow-none): a #CtkCellRenderer or %NULL
 *
 * Sets the tip area of @tooltip to the area which @cell occupies in
 * the item pointed to by @path. See also ctk_tooltip_set_tip_area().
 *
 * See also ctk_icon_view_set_tooltip_column() for a simpler alternative.
 *
 * Since: 2.12
 */
void
ctk_icon_view_set_tooltip_cell (CtkIconView     *icon_view,
                                CtkTooltip      *tooltip,
                                CtkTreePath     *path,
                                CtkCellRenderer *cell)
{
  CdkRectangle rect;

  g_return_if_fail (CTK_IS_ICON_VIEW (icon_view));
  g_return_if_fail (CTK_IS_TOOLTIP (tooltip));
  g_return_if_fail (cell == NULL || CTK_IS_CELL_RENDERER (cell));

  if (!ctk_icon_view_get_cell_rect (icon_view, path, cell, &rect))
    return;

  ctk_tooltip_set_tip_area (tooltip, &rect);
}


/**
 * ctk_icon_view_get_tooltip_context:
 * @icon_view: an #CtkIconView
 * @x: (inout): the x coordinate (relative to widget coordinates)
 * @y: (inout): the y coordinate (relative to widget coordinates)
 * @keyboard_tip: whether this is a keyboard tooltip or not
 * @model: (out) (allow-none) (transfer none): a pointer to receive a
 *         #CtkTreeModel or %NULL
 * @path: (out) (allow-none): a pointer to receive a #CtkTreePath or %NULL
 * @iter: (out) (allow-none): a pointer to receive a #CtkTreeIter or %NULL
 *
 * This function is supposed to be used in a #CtkWidget::query-tooltip
 * signal handler for #CtkIconView.  The @x, @y and @keyboard_tip values
 * which are received in the signal handler, should be passed to this
 * function without modification.
 *
 * The return value indicates whether there is an icon view item at the given
 * coordinates (%TRUE) or not (%FALSE) for mouse tooltips. For keyboard
 * tooltips the item returned will be the cursor item. When %TRUE, then any of
 * @model, @path and @iter which have been provided will be set to point to
 * that row and the corresponding model. @x and @y will always be converted
 * to be relative to @icon_view’s bin_window if @keyboard_tooltip is %FALSE.
 *
 * Returns: whether or not the given tooltip context points to a item
 *
 * Since: 2.12
 */
gboolean
ctk_icon_view_get_tooltip_context (CtkIconView   *icon_view,
                                   gint          *x,
                                   gint          *y,
                                   gboolean       keyboard_tip,
                                   CtkTreeModel **model,
                                   CtkTreePath  **path,
                                   CtkTreeIter   *iter)
{
  CtkTreePath *tmppath = NULL;

  g_return_val_if_fail (CTK_IS_ICON_VIEW (icon_view), FALSE);
  g_return_val_if_fail (x != NULL, FALSE);
  g_return_val_if_fail (y != NULL, FALSE);

  if (keyboard_tip)
    {
      ctk_icon_view_get_cursor (icon_view, &tmppath, NULL);

      if (!tmppath)
        return FALSE;
    }
  else
    {
      ctk_icon_view_convert_widget_to_bin_window_coords (icon_view, *x, *y,
                                                         x, y);

      if (!ctk_icon_view_get_item_at_pos (icon_view, *x, *y, &tmppath, NULL))
        return FALSE;
    }

  if (model)
    *model = ctk_icon_view_get_model (icon_view);

  if (iter)
    ctk_tree_model_get_iter (ctk_icon_view_get_model (icon_view),
                             iter, tmppath);

  if (path)
    *path = tmppath;
  else
    ctk_tree_path_free (tmppath);

  return TRUE;
}

static gboolean
ctk_icon_view_set_tooltip_query_cb (CtkWidget  *widget,
                                    gint        x,
                                    gint        y,
                                    gboolean    keyboard_tip,
                                    CtkTooltip *tooltip,
                                    gpointer    data)
{
  gchar *str;
  CtkTreeIter iter;
  CtkTreePath *path;
  CtkTreeModel *model;
  CtkIconView *icon_view = CTK_ICON_VIEW (widget);

  if (!ctk_icon_view_get_tooltip_context (CTK_ICON_VIEW (widget),
                                          &x, &y,
                                          keyboard_tip,
                                          &model, &path, &iter))
    return FALSE;

  ctk_tree_model_get (model, &iter, icon_view->priv->tooltip_column, &str, -1);

  if (!str)
    {
      ctk_tree_path_free (path);
      return FALSE;
    }

  ctk_tooltip_set_markup (tooltip, str);
  ctk_icon_view_set_tooltip_item (icon_view, tooltip, path);

  ctk_tree_path_free (path);
  g_free (str);

  return TRUE;
}


/**
 * ctk_icon_view_set_tooltip_column:
 * @icon_view: a #CtkIconView
 * @column: an integer, which is a valid column number for @icon_view’s model
 *
 * If you only plan to have simple (text-only) tooltips on full items, you
 * can use this function to have #CtkIconView handle these automatically
 * for you. @column should be set to the column in @icon_view’s model
 * containing the tooltip texts, or -1 to disable this feature.
 *
 * When enabled, #CtkWidget:has-tooltip will be set to %TRUE and
 * @icon_view will connect a #CtkWidget::query-tooltip signal handler.
 *
 * Note that the signal handler sets the text with ctk_tooltip_set_markup(),
 * so &, <, etc have to be escaped in the text.
 *
 * Since: 2.12
 */
void
ctk_icon_view_set_tooltip_column (CtkIconView *icon_view,
                                  gint         column)
{
  g_return_if_fail (CTK_IS_ICON_VIEW (icon_view));

  if (column == icon_view->priv->tooltip_column)
    return;

  if (column == -1)
    {
      g_signal_handlers_disconnect_by_func (icon_view,
                                            ctk_icon_view_set_tooltip_query_cb,
                                            NULL);
      ctk_widget_set_has_tooltip (CTK_WIDGET (icon_view), FALSE);
    }
  else
    {
      if (icon_view->priv->tooltip_column == -1)
        {
          g_signal_connect (icon_view, "query-tooltip",
                            G_CALLBACK (ctk_icon_view_set_tooltip_query_cb), NULL);
          ctk_widget_set_has_tooltip (CTK_WIDGET (icon_view), TRUE);
        }
    }

  icon_view->priv->tooltip_column = column;
  g_object_notify (G_OBJECT (icon_view), "tooltip-column");
}

/**
 * ctk_icon_view_get_tooltip_column:
 * @icon_view: a #CtkIconView
 *
 * Returns the column of @icon_view’s model which is being used for
 * displaying tooltips on @icon_view’s rows.
 *
 * Returns: the index of the tooltip column that is currently being
 * used, or -1 if this is disabled.
 *
 * Since: 2.12
 */
gint
ctk_icon_view_get_tooltip_column (CtkIconView *icon_view)
{
  g_return_val_if_fail (CTK_IS_ICON_VIEW (icon_view), 0);

  return icon_view->priv->tooltip_column;
}

/**
 * ctk_icon_view_get_visible_range:
 * @icon_view: A #CtkIconView
 * @start_path: (out) (allow-none): Return location for start of region,
 *              or %NULL
 * @end_path: (out) (allow-none): Return location for end of region, or %NULL
 * 
 * Sets @start_path and @end_path to be the first and last visible path. 
 * Note that there may be invisible paths in between.
 * 
 * Both paths should be freed with ctk_tree_path_free() after use.
 * 
 * Returns: %TRUE, if valid paths were placed in @start_path and @end_path
 *
 * Since: 2.8
 **/
gboolean
ctk_icon_view_get_visible_range (CtkIconView  *icon_view,
				 CtkTreePath **start_path,
				 CtkTreePath **end_path)
{
  gint start_index = -1;
  gint end_index = -1;
  GList *icons;

  g_return_val_if_fail (CTK_IS_ICON_VIEW (icon_view), FALSE);

  if (icon_view->priv->hadjustment == NULL ||
      icon_view->priv->vadjustment == NULL)
    return FALSE;

  if (start_path == NULL && end_path == NULL)
    return FALSE;
  
  for (icons = icon_view->priv->items; icons; icons = icons->next) 
    {
      CtkIconViewItem *item = icons->data;
      CdkRectangle    *item_area = &item->cell_area;

      if ((item_area->x + item_area->width >= (int)ctk_adjustment_get_value (icon_view->priv->hadjustment)) &&
	  (item_area->y + item_area->height >= (int)ctk_adjustment_get_value (icon_view->priv->vadjustment)) &&
	  (item_area->x <= 
	   (int) (ctk_adjustment_get_value (icon_view->priv->hadjustment) + 
		  ctk_adjustment_get_page_size (icon_view->priv->hadjustment))) &&
	  (item_area->y <= 
	   (int) (ctk_adjustment_get_value (icon_view->priv->vadjustment) + 
		  ctk_adjustment_get_page_size (icon_view->priv->vadjustment))))
	{
	  if (start_index == -1)
	    start_index = item->index;
	  end_index = item->index;
	}
    }

  if (start_path && start_index != -1)
    *start_path = ctk_tree_path_new_from_indices (start_index, -1);
  if (end_path && end_index != -1)
    *end_path = ctk_tree_path_new_from_indices (end_index, -1);
  
  return start_index != -1;
}

/**
 * ctk_icon_view_selected_foreach:
 * @icon_view: A #CtkIconView.
 * @func: (scope call): The function to call for each selected icon.
 * @data: User data to pass to the function.
 * 
 * Calls a function for each selected icon. Note that the model or
 * selection cannot be modified from within this function.
 *
 * Since: 2.6 
 **/
void
ctk_icon_view_selected_foreach (CtkIconView           *icon_view,
				CtkIconViewForeachFunc func,
				gpointer               data)
{
  GList *list;
  
  for (list = icon_view->priv->items; list; list = list->next)
    {
      CtkIconViewItem *item = list->data;
      CtkTreePath *path = ctk_tree_path_new_from_indices (item->index, -1);

      if (item->selected)
	(* func) (icon_view, path, data);

      ctk_tree_path_free (path);
    }
}

/**
 * ctk_icon_view_set_selection_mode:
 * @icon_view: A #CtkIconView.
 * @mode: The selection mode
 * 
 * Sets the selection mode of the @icon_view.
 *
 * Since: 2.6 
 **/
void
ctk_icon_view_set_selection_mode (CtkIconView      *icon_view,
				  CtkSelectionMode  mode)
{
  g_return_if_fail (CTK_IS_ICON_VIEW (icon_view));

  if (mode == icon_view->priv->selection_mode)
    return;
  
  if (mode == CTK_SELECTION_NONE ||
      icon_view->priv->selection_mode == CTK_SELECTION_MULTIPLE)
    ctk_icon_view_unselect_all (icon_view);
  
  icon_view->priv->selection_mode = mode;

  g_object_notify (G_OBJECT (icon_view), "selection-mode");
}

/**
 * ctk_icon_view_get_selection_mode:
 * @icon_view: A #CtkIconView.
 * 
 * Gets the selection mode of the @icon_view.
 *
 * Returns: the current selection mode
 *
 * Since: 2.6 
 **/
CtkSelectionMode
ctk_icon_view_get_selection_mode (CtkIconView *icon_view)
{
  g_return_val_if_fail (CTK_IS_ICON_VIEW (icon_view), CTK_SELECTION_SINGLE);

  return icon_view->priv->selection_mode;
}

/**
 * ctk_icon_view_set_model:
 * @icon_view: A #CtkIconView.
 * @model: (allow-none): The model.
 *
 * Sets the model for a #CtkIconView.
 * If the @icon_view already has a model set, it will remove
 * it before setting the new model.  If @model is %NULL, then
 * it will unset the old model.
 *
 * Since: 2.6 
 **/
void
ctk_icon_view_set_model (CtkIconView *icon_view,
			 CtkTreeModel *model)
{
  gboolean dirty;

  g_return_if_fail (CTK_IS_ICON_VIEW (icon_view));
  g_return_if_fail (model == NULL || CTK_IS_TREE_MODEL (model));
  
  if (icon_view->priv->model == model)
    return;

  if (icon_view->priv->scroll_to_path)
    {
      ctk_tree_row_reference_free (icon_view->priv->scroll_to_path);
      icon_view->priv->scroll_to_path = NULL;
    }

  /* The area can be NULL while disposing */
  if (icon_view->priv->cell_area)
    ctk_cell_area_stop_editing (icon_view->priv->cell_area, TRUE);

  dirty = ctk_icon_view_unselect_all_internal (icon_view);

  if (model)
    {
      GType column_type;

      if (icon_view->priv->pixbuf_column != -1)
	{
	  column_type = ctk_tree_model_get_column_type (model,
							icon_view->priv->pixbuf_column);	  

	  g_return_if_fail (column_type == GDK_TYPE_PIXBUF);
	}

      if (icon_view->priv->text_column != -1)
	{
	  column_type = ctk_tree_model_get_column_type (model,
							icon_view->priv->text_column);	  

	  g_return_if_fail (column_type == G_TYPE_STRING);
	}

      if (icon_view->priv->markup_column != -1)
	{
	  column_type = ctk_tree_model_get_column_type (model,
							icon_view->priv->markup_column);	  

	  g_return_if_fail (column_type == G_TYPE_STRING);
	}
      
    }
  
  if (icon_view->priv->model)
    {
      g_signal_handlers_disconnect_by_func (icon_view->priv->model,
					    ctk_icon_view_row_changed,
					    icon_view);
      g_signal_handlers_disconnect_by_func (icon_view->priv->model,
					    ctk_icon_view_row_inserted,
					    icon_view);
      g_signal_handlers_disconnect_by_func (icon_view->priv->model,
					    ctk_icon_view_row_deleted,
					    icon_view);
      g_signal_handlers_disconnect_by_func (icon_view->priv->model,
					    ctk_icon_view_rows_reordered,
					    icon_view);

      g_object_unref (icon_view->priv->model);
      
      g_list_free_full (icon_view->priv->items, (GDestroyNotify) ctk_icon_view_item_free);
      icon_view->priv->items = NULL;
      icon_view->priv->anchor_item = NULL;
      icon_view->priv->cursor_item = NULL;
      icon_view->priv->last_single_clicked = NULL;
      icon_view->priv->last_prelight = NULL;
      icon_view->priv->width = 0;
      icon_view->priv->height = 0;
    }

  icon_view->priv->model = model;

  if (icon_view->priv->model)
    {
      g_object_ref (icon_view->priv->model);
      g_signal_connect (icon_view->priv->model,
			"row-changed",
			G_CALLBACK (ctk_icon_view_row_changed),
			icon_view);
      g_signal_connect (icon_view->priv->model,
			"row-inserted",
			G_CALLBACK (ctk_icon_view_row_inserted),
			icon_view);
      g_signal_connect (icon_view->priv->model,
			"row-deleted",
			G_CALLBACK (ctk_icon_view_row_deleted),
			icon_view);
      g_signal_connect (icon_view->priv->model,
			"rows-reordered",
			G_CALLBACK (ctk_icon_view_rows_reordered),
			icon_view);

      ctk_icon_view_build_items (icon_view);
    }

  g_object_notify (G_OBJECT (icon_view), "model");  

  if (dirty)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);

  ctk_widget_queue_resize (CTK_WIDGET (icon_view));
}

/**
 * ctk_icon_view_get_model:
 * @icon_view: a #CtkIconView
 *
 * Returns the model the #CtkIconView is based on.  Returns %NULL if the
 * model is unset.
 *
 * Returns: (nullable) (transfer none): A #CtkTreeModel, or %NULL if none is
 *     currently being used.
 *
 * Since: 2.6 
 **/
CtkTreeModel *
ctk_icon_view_get_model (CtkIconView *icon_view)
{
  g_return_val_if_fail (CTK_IS_ICON_VIEW (icon_view), NULL);

  return icon_view->priv->model;
}

static void
update_text_cell (CtkIconView *icon_view)
{
  if (!icon_view->priv->cell_area)
    return;

  if (icon_view->priv->text_column == -1 &&
      icon_view->priv->markup_column == -1)
    {
      if (icon_view->priv->text_cell != NULL)
	{
	  ctk_cell_area_remove (icon_view->priv->cell_area, 
				icon_view->priv->text_cell);
	  icon_view->priv->text_cell = NULL;
	}
    }
  else 
    {
      if (icon_view->priv->text_cell == NULL)
	{
	  icon_view->priv->text_cell = ctk_cell_renderer_text_new ();

	  ctk_cell_layout_pack_end (CTK_CELL_LAYOUT (icon_view), icon_view->priv->text_cell, FALSE);
	}

      if (icon_view->priv->markup_column != -1)
	ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (icon_view),
					icon_view->priv->text_cell, 
					"markup", icon_view->priv->markup_column, 
					NULL);
      else
	ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (icon_view),
					icon_view->priv->text_cell, 
					"text", icon_view->priv->text_column, 
					NULL);

      if (icon_view->priv->item_orientation == CTK_ORIENTATION_VERTICAL)
	g_object_set (icon_view->priv->text_cell,
                      "alignment", PANGO_ALIGN_CENTER,
		      "wrap-mode", PANGO_WRAP_WORD_CHAR,
		      "xalign", 0.5,
		      "yalign", 0.0,
		      NULL);
      else
	g_object_set (icon_view->priv->text_cell,
                      "alignment", PANGO_ALIGN_LEFT,
		      "wrap-mode", PANGO_WRAP_WORD_CHAR,
		      "xalign", 0.0,
		      "yalign", 0.5,
		      NULL);
    }
}

static void
update_pixbuf_cell (CtkIconView *icon_view)
{
  if (!icon_view->priv->cell_area)
    return;

  if (icon_view->priv->pixbuf_column == -1)
    {
      if (icon_view->priv->pixbuf_cell != NULL)
	{
	  ctk_cell_area_remove (icon_view->priv->cell_area, 
				icon_view->priv->pixbuf_cell);

	  icon_view->priv->pixbuf_cell = NULL;
	}
    }
  else 
    {
      if (icon_view->priv->pixbuf_cell == NULL)
	{
	  icon_view->priv->pixbuf_cell = ctk_cell_renderer_pixbuf_new ();
	  
	  ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (icon_view), icon_view->priv->pixbuf_cell, FALSE);
	}
      
      ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (icon_view),
				      icon_view->priv->pixbuf_cell, 
				      "pixbuf", icon_view->priv->pixbuf_column, 
				      NULL);

      if (icon_view->priv->item_orientation == CTK_ORIENTATION_VERTICAL)
	g_object_set (icon_view->priv->pixbuf_cell,
		      "xalign", 0.5,
		      "yalign", 1.0,
		      NULL);
      else
	g_object_set (icon_view->priv->pixbuf_cell,
		      "xalign", 0.0,
		      "yalign", 0.0,
		      NULL);
    }
}

/**
 * ctk_icon_view_set_text_column:
 * @icon_view: A #CtkIconView.
 * @column: A column in the currently used model, or -1 to display no text
 * 
 * Sets the column with text for @icon_view to be @column. The text
 * column must be of type #G_TYPE_STRING.
 *
 * Since: 2.6 
 **/
void
ctk_icon_view_set_text_column (CtkIconView *icon_view,
			       gint          column)
{
  if (column == icon_view->priv->text_column)
    return;
  
  if (column == -1)
    icon_view->priv->text_column = -1;
  else
    {
      if (icon_view->priv->model != NULL)
	{
	  GType column_type;
	  
	  column_type = ctk_tree_model_get_column_type (icon_view->priv->model, column);

	  g_return_if_fail (column_type == G_TYPE_STRING);
	}
      
      icon_view->priv->text_column = column;
    }

  if (icon_view->priv->cell_area)
    ctk_cell_area_stop_editing (icon_view->priv->cell_area, TRUE);

  update_text_cell (icon_view);

  ctk_icon_view_invalidate_sizes (icon_view);
  
  g_object_notify (G_OBJECT (icon_view), "text-column");
}

/**
 * ctk_icon_view_get_text_column:
 * @icon_view: A #CtkIconView.
 *
 * Returns the column with text for @icon_view.
 *
 * Returns: the text column, or -1 if it’s unset.
 *
 * Since: 2.6
 */
gint
ctk_icon_view_get_text_column (CtkIconView  *icon_view)
{
  g_return_val_if_fail (CTK_IS_ICON_VIEW (icon_view), -1);

  return icon_view->priv->text_column;
}

/**
 * ctk_icon_view_set_markup_column:
 * @icon_view: A #CtkIconView.
 * @column: A column in the currently used model, or -1 to display no text
 * 
 * Sets the column with markup information for @icon_view to be
 * @column. The markup column must be of type #G_TYPE_STRING.
 * If the markup column is set to something, it overrides
 * the text column set by ctk_icon_view_set_text_column().
 *
 * Since: 2.6
 **/
void
ctk_icon_view_set_markup_column (CtkIconView *icon_view,
				 gint         column)
{
  if (column == icon_view->priv->markup_column)
    return;
  
  if (column == -1)
    icon_view->priv->markup_column = -1;
  else
    {
      if (icon_view->priv->model != NULL)
	{
	  GType column_type;
	  
	  column_type = ctk_tree_model_get_column_type (icon_view->priv->model, column);

	  g_return_if_fail (column_type == G_TYPE_STRING);
	}
      
      icon_view->priv->markup_column = column;
    }

  if (icon_view->priv->cell_area)
    ctk_cell_area_stop_editing (icon_view->priv->cell_area, TRUE);

  update_text_cell (icon_view);

  ctk_icon_view_invalidate_sizes (icon_view);
  
  g_object_notify (G_OBJECT (icon_view), "markup-column");
}

/**
 * ctk_icon_view_get_markup_column:
 * @icon_view: A #CtkIconView.
 *
 * Returns the column with markup text for @icon_view.
 *
 * Returns: the markup column, or -1 if it’s unset.
 *
 * Since: 2.6
 */
gint
ctk_icon_view_get_markup_column (CtkIconView  *icon_view)
{
  g_return_val_if_fail (CTK_IS_ICON_VIEW (icon_view), -1);

  return icon_view->priv->markup_column;
}

/**
 * ctk_icon_view_set_pixbuf_column:
 * @icon_view: A #CtkIconView.
 * @column: A column in the currently used model, or -1 to disable
 * 
 * Sets the column with pixbufs for @icon_view to be @column. The pixbuf
 * column must be of type #GDK_TYPE_PIXBUF
 *
 * Since: 2.6 
 **/
void
ctk_icon_view_set_pixbuf_column (CtkIconView *icon_view,
				 gint         column)
{
  if (column == icon_view->priv->pixbuf_column)
    return;
  
  if (column == -1)
    icon_view->priv->pixbuf_column = -1;
  else
    {
      if (icon_view->priv->model != NULL)
	{
	  GType column_type;
	  
	  column_type = ctk_tree_model_get_column_type (icon_view->priv->model, column);

	  g_return_if_fail (column_type == GDK_TYPE_PIXBUF);
	}
      
      icon_view->priv->pixbuf_column = column;
    }

  if (icon_view->priv->cell_area)
    ctk_cell_area_stop_editing (icon_view->priv->cell_area, TRUE);

  update_pixbuf_cell (icon_view);

  ctk_icon_view_invalidate_sizes (icon_view);
  
  g_object_notify (G_OBJECT (icon_view), "pixbuf-column");
  
}

/**
 * ctk_icon_view_get_pixbuf_column:
 * @icon_view: A #CtkIconView.
 *
 * Returns the column with pixbufs for @icon_view.
 *
 * Returns: the pixbuf column, or -1 if it’s unset.
 *
 * Since: 2.6
 */
gint
ctk_icon_view_get_pixbuf_column (CtkIconView  *icon_view)
{
  g_return_val_if_fail (CTK_IS_ICON_VIEW (icon_view), -1);

  return icon_view->priv->pixbuf_column;
}

/**
 * ctk_icon_view_select_path:
 * @icon_view: A #CtkIconView.
 * @path: The #CtkTreePath to be selected.
 * 
 * Selects the row at @path.
 *
 * Since: 2.6
 **/
void
ctk_icon_view_select_path (CtkIconView *icon_view,
			   CtkTreePath *path)
{
  CtkIconViewItem *item = NULL;

  g_return_if_fail (CTK_IS_ICON_VIEW (icon_view));
  g_return_if_fail (icon_view->priv->model != NULL);
  g_return_if_fail (path != NULL);

  if (ctk_tree_path_get_depth (path) > 0)
    item = g_list_nth_data (icon_view->priv->items,
			    ctk_tree_path_get_indices(path)[0]);

  if (item)
    _ctk_icon_view_select_item (icon_view, item);
}

/**
 * ctk_icon_view_unselect_path:
 * @icon_view: A #CtkIconView.
 * @path: The #CtkTreePath to be unselected.
 * 
 * Unselects the row at @path.
 *
 * Since: 2.6
 **/
void
ctk_icon_view_unselect_path (CtkIconView *icon_view,
			     CtkTreePath *path)
{
  CtkIconViewItem *item;
  
  g_return_if_fail (CTK_IS_ICON_VIEW (icon_view));
  g_return_if_fail (icon_view->priv->model != NULL);
  g_return_if_fail (path != NULL);

  item = g_list_nth_data (icon_view->priv->items,
			  ctk_tree_path_get_indices(path)[0]);

  if (!item)
    return;
  
  _ctk_icon_view_unselect_item (icon_view, item);
}

/**
 * ctk_icon_view_get_selected_items:
 * @icon_view: A #CtkIconView.
 *
 * Creates a list of paths of all selected items. Additionally, if you are
 * planning on modifying the model after calling this function, you may
 * want to convert the returned list into a list of #CtkTreeRowReferences.
 * To do this, you can use ctk_tree_row_reference_new().
 *
 * To free the return value, use:
 * |[<!-- language="C" -->
 * g_list_free_full (list, (GDestroyNotify) ctk_tree_path_free);
 * ]|
 *
 * Returns: (element-type CtkTreePath) (transfer full): A #GList containing a #CtkTreePath for each selected row.
 *
 * Since: 2.6
 **/
GList *
ctk_icon_view_get_selected_items (CtkIconView *icon_view)
{
  GList *list;
  GList *selected = NULL;
  
  g_return_val_if_fail (CTK_IS_ICON_VIEW (icon_view), NULL);
  
  for (list = icon_view->priv->items; list != NULL; list = list->next)
    {
      CtkIconViewItem *item = list->data;

      if (item->selected)
	{
	  CtkTreePath *path = ctk_tree_path_new_from_indices (item->index, -1);

	  selected = g_list_prepend (selected, path);
	}
    }

  return selected;
}

/**
 * ctk_icon_view_select_all:
 * @icon_view: A #CtkIconView.
 * 
 * Selects all the icons. @icon_view must has its selection mode set
 * to #CTK_SELECTION_MULTIPLE.
 *
 * Since: 2.6
 **/
void
ctk_icon_view_select_all (CtkIconView *icon_view)
{
  GList *items;
  gboolean dirty = FALSE;
  
  g_return_if_fail (CTK_IS_ICON_VIEW (icon_view));

  if (icon_view->priv->selection_mode != CTK_SELECTION_MULTIPLE)
    return;

  for (items = icon_view->priv->items; items; items = items->next)
    {
      CtkIconViewItem *item = items->data;
      
      if (!item->selected)
	{
	  dirty = TRUE;
	  item->selected = TRUE;
	  ctk_icon_view_queue_draw_item (icon_view, item);
	}
    }

  if (dirty)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}

/**
 * ctk_icon_view_unselect_all:
 * @icon_view: A #CtkIconView.
 * 
 * Unselects all the icons.
 *
 * Since: 2.6
 **/
void
ctk_icon_view_unselect_all (CtkIconView *icon_view)
{
  gboolean dirty = FALSE;
  
  g_return_if_fail (CTK_IS_ICON_VIEW (icon_view));

  if (icon_view->priv->selection_mode == CTK_SELECTION_BROWSE)
    return;

  dirty = ctk_icon_view_unselect_all_internal (icon_view);

  if (dirty)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}

/**
 * ctk_icon_view_path_is_selected:
 * @icon_view: A #CtkIconView.
 * @path: A #CtkTreePath to check selection on.
 * 
 * Returns %TRUE if the icon pointed to by @path is currently
 * selected. If @path does not point to a valid location, %FALSE is returned.
 * 
 * Returns: %TRUE if @path is selected.
 *
 * Since: 2.6
 **/
gboolean
ctk_icon_view_path_is_selected (CtkIconView *icon_view,
				CtkTreePath *path)
{
  CtkIconViewItem *item;
  
  g_return_val_if_fail (CTK_IS_ICON_VIEW (icon_view), FALSE);
  g_return_val_if_fail (icon_view->priv->model != NULL, FALSE);
  g_return_val_if_fail (path != NULL, FALSE);
  
  item = g_list_nth_data (icon_view->priv->items,
			  ctk_tree_path_get_indices(path)[0]);

  if (!item)
    return FALSE;
  
  return item->selected;
}

/**
 * ctk_icon_view_get_item_row:
 * @icon_view: a #CtkIconView
 * @path: the #CtkTreePath of the item
 *
 * Gets the row in which the item @path is currently
 * displayed. Row numbers start at 0.
 *
 * Returns: The row in which the item is displayed
 *
 * Since: 2.22
 */
gint
ctk_icon_view_get_item_row (CtkIconView *icon_view,
                            CtkTreePath *path)
{
  CtkIconViewItem *item;

  g_return_val_if_fail (CTK_IS_ICON_VIEW (icon_view), -1);
  g_return_val_if_fail (icon_view->priv->model != NULL, -1);
  g_return_val_if_fail (path != NULL, -1);

  item = g_list_nth_data (icon_view->priv->items,
                          ctk_tree_path_get_indices(path)[0]);

  if (!item)
    return -1;

  return item->row;
}

/**
 * ctk_icon_view_get_item_column:
 * @icon_view: a #CtkIconView
 * @path: the #CtkTreePath of the item
 *
 * Gets the column in which the item @path is currently
 * displayed. Column numbers start at 0.
 *
 * Returns: The column in which the item is displayed
 *
 * Since: 2.22
 */
gint
ctk_icon_view_get_item_column (CtkIconView *icon_view,
                               CtkTreePath *path)
{
  CtkIconViewItem *item;

  g_return_val_if_fail (CTK_IS_ICON_VIEW (icon_view), -1);
  g_return_val_if_fail (icon_view->priv->model != NULL, -1);
  g_return_val_if_fail (path != NULL, -1);

  item = g_list_nth_data (icon_view->priv->items,
                          ctk_tree_path_get_indices(path)[0]);

  if (!item)
    return -1;

  return item->col;
}

/**
 * ctk_icon_view_item_activated:
 * @icon_view: A #CtkIconView
 * @path: The #CtkTreePath to be activated
 * 
 * Activates the item determined by @path.
 *
 * Since: 2.6
 **/
void
ctk_icon_view_item_activated (CtkIconView      *icon_view,
			      CtkTreePath      *path)
{
  g_return_if_fail (CTK_IS_ICON_VIEW (icon_view));
  g_return_if_fail (path != NULL);
  
  g_signal_emit (icon_view, icon_view_signals[ITEM_ACTIVATED], 0, path);
}

/**
 * ctk_icon_view_set_item_orientation:
 * @icon_view: a #CtkIconView
 * @orientation: the relative position of texts and icons 
 * 
 * Sets the ::item-orientation property which determines whether the labels 
 * are drawn beside the icons instead of below.
 *
 * Since: 2.6
 **/
void 
ctk_icon_view_set_item_orientation (CtkIconView    *icon_view,
                                    CtkOrientation  orientation)
{
  g_return_if_fail (CTK_IS_ICON_VIEW (icon_view));

  if (icon_view->priv->item_orientation != orientation)
    {
      icon_view->priv->item_orientation = orientation;

      if (icon_view->priv->cell_area)
	{
	  if (CTK_IS_ORIENTABLE (icon_view->priv->cell_area))
	    ctk_orientable_set_orientation (CTK_ORIENTABLE (icon_view->priv->cell_area), 
					    icon_view->priv->item_orientation);

	  ctk_cell_area_stop_editing (icon_view->priv->cell_area, TRUE);
	}

      ctk_icon_view_invalidate_sizes (icon_view);

      update_text_cell (icon_view);
      update_pixbuf_cell (icon_view);
      
      g_object_notify (G_OBJECT (icon_view), "item-orientation");
    }
}

/**
 * ctk_icon_view_get_item_orientation:
 * @icon_view: a #CtkIconView
 * 
 * Returns the value of the ::item-orientation property which determines 
 * whether the labels are drawn beside the icons instead of below. 
 * 
 * Returns: the relative position of texts and icons 
 *
 * Since: 2.6
 **/
CtkOrientation
ctk_icon_view_get_item_orientation (CtkIconView *icon_view)
{
  g_return_val_if_fail (CTK_IS_ICON_VIEW (icon_view), 
			CTK_ORIENTATION_VERTICAL);

  return icon_view->priv->item_orientation;
}

/**
 * ctk_icon_view_set_columns:
 * @icon_view: a #CtkIconView
 * @columns: the number of columns
 * 
 * Sets the ::columns property which determines in how
 * many columns the icons are arranged. If @columns is
 * -1, the number of columns will be chosen automatically 
 * to fill the available area. 
 *
 * Since: 2.6
 */
void 
ctk_icon_view_set_columns (CtkIconView *icon_view,
			   gint         columns)
{
  g_return_if_fail (CTK_IS_ICON_VIEW (icon_view));
  
  if (icon_view->priv->columns != columns)
    {
      icon_view->priv->columns = columns;

      if (icon_view->priv->cell_area)
	ctk_cell_area_stop_editing (icon_view->priv->cell_area, TRUE);

      ctk_widget_queue_resize (CTK_WIDGET (icon_view));
      
      g_object_notify (G_OBJECT (icon_view), "columns");
    }  
}

/**
 * ctk_icon_view_get_columns:
 * @icon_view: a #CtkIconView
 * 
 * Returns the value of the ::columns property.
 * 
 * Returns: the number of columns, or -1
 *
 * Since: 2.6
 */
gint
ctk_icon_view_get_columns (CtkIconView *icon_view)
{
  g_return_val_if_fail (CTK_IS_ICON_VIEW (icon_view), -1);

  return icon_view->priv->columns;
}

/**
 * ctk_icon_view_set_item_width:
 * @icon_view: a #CtkIconView
 * @item_width: the width for each item
 * 
 * Sets the ::item-width property which specifies the width 
 * to use for each item. If it is set to -1, the icon view will 
 * automatically determine a suitable item size.
 *
 * Since: 2.6
 */
void 
ctk_icon_view_set_item_width (CtkIconView *icon_view,
			      gint         item_width)
{
  g_return_if_fail (CTK_IS_ICON_VIEW (icon_view));
  
  if (icon_view->priv->item_width != item_width)
    {
      icon_view->priv->item_width = item_width;
      
      if (icon_view->priv->cell_area)
	ctk_cell_area_stop_editing (icon_view->priv->cell_area, TRUE);

      ctk_icon_view_invalidate_sizes (icon_view);
      
      update_text_cell (icon_view);

      g_object_notify (G_OBJECT (icon_view), "item-width");
    }  
}

/**
 * ctk_icon_view_get_item_width:
 * @icon_view: a #CtkIconView
 * 
 * Returns the value of the ::item-width property.
 * 
 * Returns: the width of a single item, or -1
 *
 * Since: 2.6
 */
gint
ctk_icon_view_get_item_width (CtkIconView *icon_view)
{
  g_return_val_if_fail (CTK_IS_ICON_VIEW (icon_view), -1);

  return icon_view->priv->item_width;
}


/**
 * ctk_icon_view_set_spacing:
 * @icon_view: a #CtkIconView
 * @spacing: the spacing
 * 
 * Sets the ::spacing property which specifies the space 
 * which is inserted between the cells (i.e. the icon and 
 * the text) of an item.
 *
 * Since: 2.6
 */
void 
ctk_icon_view_set_spacing (CtkIconView *icon_view,
			   gint         spacing)
{
  g_return_if_fail (CTK_IS_ICON_VIEW (icon_view));
  
  if (icon_view->priv->spacing != spacing)
    {
      icon_view->priv->spacing = spacing;

      if (icon_view->priv->cell_area)
	ctk_cell_area_stop_editing (icon_view->priv->cell_area, TRUE);

      ctk_icon_view_invalidate_sizes (icon_view);

      g_object_notify (G_OBJECT (icon_view), "spacing");
    }  
}

/**
 * ctk_icon_view_get_spacing:
 * @icon_view: a #CtkIconView
 * 
 * Returns the value of the ::spacing property.
 * 
 * Returns: the space between cells 
 *
 * Since: 2.6
 */
gint
ctk_icon_view_get_spacing (CtkIconView *icon_view)
{
  g_return_val_if_fail (CTK_IS_ICON_VIEW (icon_view), -1);

  return icon_view->priv->spacing;
}

/**
 * ctk_icon_view_set_row_spacing:
 * @icon_view: a #CtkIconView
 * @row_spacing: the row spacing
 * 
 * Sets the ::row-spacing property which specifies the space 
 * which is inserted between the rows of the icon view.
 *
 * Since: 2.6
 */
void 
ctk_icon_view_set_row_spacing (CtkIconView *icon_view,
			       gint         row_spacing)
{
  g_return_if_fail (CTK_IS_ICON_VIEW (icon_view));
  
  if (icon_view->priv->row_spacing != row_spacing)
    {
      icon_view->priv->row_spacing = row_spacing;

      if (icon_view->priv->cell_area)
	ctk_cell_area_stop_editing (icon_view->priv->cell_area, TRUE);

      ctk_icon_view_invalidate_sizes (icon_view);

      g_object_notify (G_OBJECT (icon_view), "row-spacing");
    }  
}

/**
 * ctk_icon_view_get_row_spacing:
 * @icon_view: a #CtkIconView
 * 
 * Returns the value of the ::row-spacing property.
 * 
 * Returns: the space between rows
 *
 * Since: 2.6
 */
gint
ctk_icon_view_get_row_spacing (CtkIconView *icon_view)
{
  g_return_val_if_fail (CTK_IS_ICON_VIEW (icon_view), -1);

  return icon_view->priv->row_spacing;
}

/**
 * ctk_icon_view_set_column_spacing:
 * @icon_view: a #CtkIconView
 * @column_spacing: the column spacing
 * 
 * Sets the ::column-spacing property which specifies the space 
 * which is inserted between the columns of the icon view.
 *
 * Since: 2.6
 */
void 
ctk_icon_view_set_column_spacing (CtkIconView *icon_view,
				  gint         column_spacing)
{
  g_return_if_fail (CTK_IS_ICON_VIEW (icon_view));
  
  if (icon_view->priv->column_spacing != column_spacing)
    {
      icon_view->priv->column_spacing = column_spacing;

      if (icon_view->priv->cell_area)
	ctk_cell_area_stop_editing (icon_view->priv->cell_area, TRUE);

      ctk_icon_view_invalidate_sizes (icon_view);

      g_object_notify (G_OBJECT (icon_view), "column-spacing");
    }  
}

/**
 * ctk_icon_view_get_column_spacing:
 * @icon_view: a #CtkIconView
 * 
 * Returns the value of the ::column-spacing property.
 * 
 * Returns: the space between columns
 *
 * Since: 2.6
 */
gint
ctk_icon_view_get_column_spacing (CtkIconView *icon_view)
{
  g_return_val_if_fail (CTK_IS_ICON_VIEW (icon_view), -1);

  return icon_view->priv->column_spacing;
}

/**
 * ctk_icon_view_set_margin:
 * @icon_view: a #CtkIconView
 * @margin: the margin
 * 
 * Sets the ::margin property which specifies the space 
 * which is inserted at the top, bottom, left and right 
 * of the icon view.
 *
 * Since: 2.6
 */
void 
ctk_icon_view_set_margin (CtkIconView *icon_view,
			  gint         margin)
{
  g_return_if_fail (CTK_IS_ICON_VIEW (icon_view));
  
  if (icon_view->priv->margin != margin)
    {
      icon_view->priv->margin = margin;

      if (icon_view->priv->cell_area)
	ctk_cell_area_stop_editing (icon_view->priv->cell_area, TRUE);

      ctk_icon_view_invalidate_sizes (icon_view);

      g_object_notify (G_OBJECT (icon_view), "margin");
    }  
}

/**
 * ctk_icon_view_get_margin:
 * @icon_view: a #CtkIconView
 * 
 * Returns the value of the ::margin property.
 * 
 * Returns: the space at the borders 
 *
 * Since: 2.6
 */
gint
ctk_icon_view_get_margin (CtkIconView *icon_view)
{
  g_return_val_if_fail (CTK_IS_ICON_VIEW (icon_view), -1);

  return icon_view->priv->margin;
}

/**
 * ctk_icon_view_set_item_padding:
 * @icon_view: a #CtkIconView
 * @item_padding: the item padding
 *
 * Sets the #CtkIconView:item-padding property which specifies the padding
 * around each of the icon view’s items.
 *
 * Since: 2.18
 */
void
ctk_icon_view_set_item_padding (CtkIconView *icon_view,
				gint         item_padding)
{
  g_return_if_fail (CTK_IS_ICON_VIEW (icon_view));
  
  if (icon_view->priv->item_padding != item_padding)
    {
      icon_view->priv->item_padding = item_padding;

      if (icon_view->priv->cell_area)
	ctk_cell_area_stop_editing (icon_view->priv->cell_area, TRUE);

      ctk_icon_view_invalidate_sizes (icon_view);

      g_object_notify (G_OBJECT (icon_view), "item-padding");
    }  
}

/**
 * ctk_icon_view_get_item_padding:
 * @icon_view: a #CtkIconView
 * 
 * Returns the value of the ::item-padding property.
 * 
 * Returns: the padding around items
 *
 * Since: 2.18
 */
gint
ctk_icon_view_get_item_padding (CtkIconView *icon_view)
{
  g_return_val_if_fail (CTK_IS_ICON_VIEW (icon_view), -1);

  return icon_view->priv->item_padding;
}

/* Get/set whether drag_motion requested the drag data and
 * drag_data_received should thus not actually insert the data,
 * since the data doesn’t result from a drop.
 */
static void
set_status_pending (CdkDragContext *context,
                    CdkDragAction   suggested_action)
{
  g_object_set_data (G_OBJECT (context),
                     I_("ctk-icon-view-status-pending"),
                     GINT_TO_POINTER (suggested_action));
}

static CdkDragAction
get_status_pending (CdkDragContext *context)
{
  return GPOINTER_TO_INT (g_object_get_data (G_OBJECT (context),
                                             "ctk-icon-view-status-pending"));
}

static void
unset_reorderable (CtkIconView *icon_view)
{
  if (icon_view->priv->reorderable)
    {
      icon_view->priv->reorderable = FALSE;
      g_object_notify (G_OBJECT (icon_view), "reorderable");
    }
}

static void
set_source_row (CdkDragContext *context,
                CtkTreeModel   *model,
                CtkTreePath    *source_row)
{
  if (source_row)
    g_object_set_data_full (G_OBJECT (context),
			    I_("ctk-icon-view-source-row"),
			    ctk_tree_row_reference_new (model, source_row),
			    (GDestroyNotify) ctk_tree_row_reference_free);
  else
    g_object_set_data_full (G_OBJECT (context),
			    I_("ctk-icon-view-source-row"),
			    NULL, NULL);
}

static CtkTreePath*
get_source_row (CdkDragContext *context)
{
  CtkTreeRowReference *ref;

  ref = g_object_get_data (G_OBJECT (context), "ctk-icon-view-source-row");

  if (ref)
    return ctk_tree_row_reference_get_path (ref);
  else
    return NULL;
}

typedef struct
{
  CtkTreeRowReference *dest_row;
  gboolean             empty_view_drop;
  gboolean             drop_append_mode;
} DestRow;

static void
dest_row_free (gpointer data)
{
  DestRow *dr = (DestRow *)data;

  ctk_tree_row_reference_free (dr->dest_row);
  g_free (dr);
}

static void
set_dest_row (CdkDragContext *context,
	      CtkTreeModel   *model,
	      CtkTreePath    *dest_row,
	      gboolean        empty_view_drop,
	      gboolean        drop_append_mode)
{
  DestRow *dr;

  if (!dest_row)
    {
      g_object_set_data_full (G_OBJECT (context),
			      I_("ctk-icon-view-dest-row"),
			      NULL, NULL);
      return;
    }
  
  dr = g_new0 (DestRow, 1);
     
  dr->dest_row = ctk_tree_row_reference_new (model, dest_row);
  dr->empty_view_drop = empty_view_drop;
  dr->drop_append_mode = drop_append_mode;
  g_object_set_data_full (G_OBJECT (context),
                          I_("ctk-icon-view-dest-row"),
                          dr, (GDestroyNotify) dest_row_free);
}

static CtkTreePath*
get_dest_row (CdkDragContext *context)
{
  DestRow *dr;

  dr = g_object_get_data (G_OBJECT (context), "ctk-icon-view-dest-row");

  if (dr)
    {
      CtkTreePath *path = NULL;
      
      if (dr->dest_row)
	path = ctk_tree_row_reference_get_path (dr->dest_row);
      else if (dr->empty_view_drop)
	path = ctk_tree_path_new_from_indices (0, -1);
      else
	path = NULL;

      if (path && dr->drop_append_mode)
	ctk_tree_path_next (path);

      return path;
    }
  else
    return NULL;
}

static gboolean
check_model_dnd (CtkTreeModel *model,
                 GType         required_iface,
                 const gchar  *signal)
{
  if (model == NULL || !G_TYPE_CHECK_INSTANCE_TYPE ((model), required_iface))
    {
      g_warning ("You must override the default '%s' handler "
                 "on CtkIconView when using models that don't support "
                 "the %s interface and enabling drag-and-drop. The simplest way to do this "
                 "is to connect to '%s' and call "
                 "g_signal_stop_emission_by_name() in your signal handler to prevent "
                 "the default handler from running. Look at the source code "
                 "for the default handler in ctkiconview.c to get an idea what "
                 "your handler should do. (ctkiconview.c is in the CTK+ source "
                 "code.) If you're using CTK+ from a language other than C, "
                 "there may be a more natural way to override default handlers, e.g. via derivation.",
                 signal, g_type_name (required_iface), signal);
      return FALSE;
    }
  else
    return TRUE;
}

static void
remove_scroll_timeout (CtkIconView *icon_view)
{
  if (icon_view->priv->scroll_timeout_id != 0)
    {
      g_source_remove (icon_view->priv->scroll_timeout_id);

      icon_view->priv->scroll_timeout_id = 0;
    }
}

static void
ctk_icon_view_autoscroll (CtkIconView *icon_view)
{
  CdkWindow *window;
  gint px, py, width, height;
  gint hoffset, voffset;

  window = ctk_widget_get_window (CTK_WIDGET (icon_view));

  px = icon_view->priv->event_last_x;
  py = icon_view->priv->event_last_y;
  cdk_window_get_geometry (window, NULL, NULL, &width, &height);

  /* see if we are near the edge. */
  voffset = py - 2 * SCROLL_EDGE_SIZE;
  if (voffset > 0)
    voffset = MAX (py - (height - 2 * SCROLL_EDGE_SIZE), 0);

  hoffset = px - 2 * SCROLL_EDGE_SIZE;
  if (hoffset > 0)
    hoffset = MAX (px - (width - 2 * SCROLL_EDGE_SIZE), 0);

  if (voffset != 0)
    ctk_adjustment_set_value (icon_view->priv->vadjustment,
                              ctk_adjustment_get_value (icon_view->priv->vadjustment) + voffset);

  if (hoffset != 0)
    ctk_adjustment_set_value (icon_view->priv->hadjustment,
                              ctk_adjustment_get_value (icon_view->priv->hadjustment) + hoffset);
}

static gboolean
drag_scroll_timeout (gpointer data)
{
  ctk_icon_view_autoscroll (data);

  return TRUE;
}

static gboolean
set_destination (CtkIconView    *icon_view,
		 CdkDragContext *context,
		 gint            x,
		 gint            y,
		 CdkDragAction  *suggested_action,
		 CdkAtom        *target)
{
  CtkWidget *widget;
  CtkTreePath *path = NULL;
  CtkIconViewDropPosition pos;
  CtkIconViewDropPosition old_pos;
  CtkTreePath *old_dest_path = NULL;
  gboolean can_drop = FALSE;

  widget = CTK_WIDGET (icon_view);

  *suggested_action = 0;
  *target = GDK_NONE;

  if (!icon_view->priv->dest_set)
    {
      /* someone unset us as a drag dest, note that if
       * we return FALSE drag_leave isn't called
       */

      ctk_icon_view_set_drag_dest_item (icon_view,
					NULL,
					CTK_ICON_VIEW_DROP_LEFT);

      remove_scroll_timeout (CTK_ICON_VIEW (widget));

      return FALSE; /* no longer a drop site */
    }

  *target = ctk_drag_dest_find_target (widget, context,
                                       ctk_drag_dest_get_target_list (widget));
  if (*target == GDK_NONE)
    return FALSE;

  if (!ctk_icon_view_get_dest_item_at_pos (icon_view, x, y, &path, &pos)) 
    {
      gint n_children;
      CtkTreeModel *model;
      
      /* the row got dropped on empty space, let's setup a special case
       */

      if (path)
	ctk_tree_path_free (path);

      model = ctk_icon_view_get_model (icon_view);

      n_children = ctk_tree_model_iter_n_children (model, NULL);
      if (n_children)
        {
          pos = CTK_ICON_VIEW_DROP_BELOW;
          path = ctk_tree_path_new_from_indices (n_children - 1, -1);
        }
      else
        {
          pos = CTK_ICON_VIEW_DROP_ABOVE;
          path = ctk_tree_path_new_from_indices (0, -1);
        }

      can_drop = TRUE;

      goto out;
    }

  g_assert (path);

  ctk_icon_view_get_drag_dest_item (icon_view,
				    &old_dest_path,
				    &old_pos);
  
  if (old_dest_path)
    ctk_tree_path_free (old_dest_path);
  
  if (TRUE /* FIXME if the location droppable predicate */)
    {
      can_drop = TRUE;
    }

out:
  if (can_drop)
    {
      CtkWidget *source_widget;

      *suggested_action = cdk_drag_context_get_suggested_action (context);
      source_widget = ctk_drag_get_source_widget (context);

      if (source_widget == widget)
        {
          /* Default to MOVE, unless the user has
           * pressed ctrl or shift to affect available actions
           */
          if ((cdk_drag_context_get_actions (context) & GDK_ACTION_MOVE) != 0)
            *suggested_action = GDK_ACTION_MOVE;
        }

      ctk_icon_view_set_drag_dest_item (CTK_ICON_VIEW (widget),
					path, pos);
    }
  else
    {
      /* can't drop here */
      ctk_icon_view_set_drag_dest_item (CTK_ICON_VIEW (widget),
					NULL,
					CTK_ICON_VIEW_DROP_LEFT);
    }
  
  if (path)
    ctk_tree_path_free (path);
  
  return TRUE;
}

static CtkTreePath*
get_logical_destination (CtkIconView *icon_view,
			 gboolean    *drop_append_mode)
{
  /* adjust path to point to the row the drop goes in front of */
  CtkTreePath *path = NULL;
  CtkIconViewDropPosition pos;
  
  *drop_append_mode = FALSE;

  ctk_icon_view_get_drag_dest_item (icon_view, &path, &pos);

  if (path == NULL)
    return NULL;

  if (pos == CTK_ICON_VIEW_DROP_RIGHT || 
      pos == CTK_ICON_VIEW_DROP_BELOW)
    {
      CtkTreeIter iter;
      CtkTreeModel *model = icon_view->priv->model;

      if (!ctk_tree_model_get_iter (model, &iter, path) ||
          !ctk_tree_model_iter_next (model, &iter))
        *drop_append_mode = TRUE;
      else
        {
          *drop_append_mode = FALSE;
          ctk_tree_path_next (path);
        }      
    }

  return path;
}

static gboolean
ctk_icon_view_maybe_begin_drag (CtkIconView    *icon_view,
				CdkEventMotion *event)
{
  CtkWidget *widget = CTK_WIDGET (icon_view);
  CdkDragContext *context;
  CtkTreePath *path = NULL;
  gint button;
  CtkTreeModel *model;
  gboolean retval = FALSE;

  if (!icon_view->priv->source_set)
    goto out;

  if (icon_view->priv->pressed_button < 0)
    goto out;

  if (!ctk_drag_check_threshold (CTK_WIDGET (icon_view),
                                 icon_view->priv->press_start_x,
                                 icon_view->priv->press_start_y,
                                 event->x, event->y))
    goto out;

  model = ctk_icon_view_get_model (icon_view);

  if (model == NULL)
    goto out;

  button = icon_view->priv->pressed_button;
  icon_view->priv->pressed_button = -1;

  path = ctk_icon_view_get_path_at_pos (icon_view,
					icon_view->priv->press_start_x,
					icon_view->priv->press_start_y);

  if (path == NULL)
    goto out;

  if (!CTK_IS_TREE_DRAG_SOURCE (model) ||
      !ctk_tree_drag_source_row_draggable (CTK_TREE_DRAG_SOURCE (model),
					   path))
    goto out;

  /* FIXME Check whether we're a start button, if not return FALSE and
   * free path
   */

  /* Now we can begin the drag */
  
  retval = TRUE;

  context = ctk_drag_begin_with_coordinates (widget,
                                             ctk_drag_source_get_target_list (widget),
                                             icon_view->priv->source_actions,
                                             button,
                                             (CdkEvent*)event,
                                             icon_view->priv->press_start_x,
                                             icon_view->priv->press_start_y);

  set_source_row (context, model, path);
  
 out:
  if (path)
    ctk_tree_path_free (path);

  return retval;
}

/* Source side drag signals */
static void 
ctk_icon_view_drag_begin (CtkWidget      *widget,
			  CdkDragContext *context)
{
  CtkIconView *icon_view;
  CtkIconViewItem *item;
  cairo_surface_t *icon;
  gint x, y;
  CtkTreePath *path;
  double sx, sy;

  icon_view = CTK_ICON_VIEW (widget);

  /* if the user uses a custom DnD impl, we don't set the icon here */
  if (!icon_view->priv->dest_set && !icon_view->priv->source_set)
    return;

  item = _ctk_icon_view_get_item_at_coords (icon_view,
					   icon_view->priv->press_start_x,
					   icon_view->priv->press_start_y,
					   TRUE,
					   NULL);

  g_return_if_fail (item != NULL);

  x = icon_view->priv->press_start_x - item->cell_area.x + icon_view->priv->item_padding;
  y = icon_view->priv->press_start_y - item->cell_area.y + icon_view->priv->item_padding;
  
  path = ctk_tree_path_new_from_indices (item->index, -1);
  icon = ctk_icon_view_create_drag_icon (icon_view, path);
  ctk_tree_path_free (path);

  cairo_surface_get_device_scale (icon, &sx, &sy);
  cairo_surface_set_device_offset (icon, -x * sx, -y * sy);

  ctk_drag_set_icon_surface (context, icon);

  cairo_surface_destroy (icon);
}

static void 
ctk_icon_view_drag_end (CtkWidget      *widget,
			CdkDragContext *context)
{
  /* do nothing */
}

static void 
ctk_icon_view_drag_data_get (CtkWidget        *widget,
			     CdkDragContext   *context,
			     CtkSelectionData *selection_data,
			     guint             info,
			     guint             time)
{
  CtkIconView *icon_view;
  CtkTreeModel *model;
  CtkTreePath *source_row;

  icon_view = CTK_ICON_VIEW (widget);
  model = ctk_icon_view_get_model (icon_view);

  if (model == NULL)
    return;

  if (!icon_view->priv->source_set)
    return;

  source_row = get_source_row (context);

  if (source_row == NULL)
    return;

  /* We can implement the CTK_TREE_MODEL_ROW target generically for
   * any model; for DragSource models there are some other targets
   * we also support.
   */

  if (CTK_IS_TREE_DRAG_SOURCE (model) &&
      ctk_tree_drag_source_drag_data_get (CTK_TREE_DRAG_SOURCE (model),
                                          source_row,
                                          selection_data))
    goto done;

  /* If drag_data_get does nothing, try providing row data. */
  if (ctk_selection_data_get_target (selection_data) == cdk_atom_intern_static_string ("CTK_TREE_MODEL_ROW"))
    ctk_tree_set_row_drag_data (selection_data,
				model,
				source_row);

 done:
  ctk_tree_path_free (source_row);
}

static void 
ctk_icon_view_drag_data_delete (CtkWidget      *widget,
				CdkDragContext *context)
{
  CtkTreeModel *model;
  CtkIconView *icon_view;
  CtkTreePath *source_row;

  icon_view = CTK_ICON_VIEW (widget);
  model = ctk_icon_view_get_model (icon_view);

  if (!check_model_dnd (model, CTK_TYPE_TREE_DRAG_SOURCE, "drag-data-delete"))
    return;

  if (!icon_view->priv->source_set)
    return;

  source_row = get_source_row (context);

  if (source_row == NULL)
    return;

  ctk_tree_drag_source_drag_data_delete (CTK_TREE_DRAG_SOURCE (model),
                                         source_row);

  ctk_tree_path_free (source_row);

  set_source_row (context, NULL, NULL);
}

/* Target side drag signals */
static void
ctk_icon_view_drag_leave (CtkWidget      *widget,
			  CdkDragContext *context,
			  guint           time)
{
  CtkIconView *icon_view;

  icon_view = CTK_ICON_VIEW (widget);

  /* unset any highlight row */
  ctk_icon_view_set_drag_dest_item (icon_view,
				    NULL,
				    CTK_ICON_VIEW_DROP_LEFT);

  remove_scroll_timeout (icon_view);
}

static gboolean 
ctk_icon_view_drag_motion (CtkWidget      *widget,
			   CdkDragContext *context,
			   gint            x,
			   gint            y,
			   guint           time)
{
  CtkTreePath *path = NULL;
  CtkIconViewDropPosition pos;
  CtkIconView *icon_view;
  CdkDragAction suggested_action = 0;
  CdkAtom target;
  gboolean empty;

  icon_view = CTK_ICON_VIEW (widget);

  if (!set_destination (icon_view, context, x, y, &suggested_action, &target))
    return FALSE;

  icon_view->priv->event_last_x = x;
  icon_view->priv->event_last_y = y;

  ctk_icon_view_get_drag_dest_item (icon_view, &path, &pos);

  /* we only know this *after* set_desination_row */
  empty = icon_view->priv->empty_view_drop;

  if (path == NULL && !empty)
    {
      /* Can't drop here. */
      cdk_drag_status (context, 0, time);
    }
  else
    {
      if (icon_view->priv->scroll_timeout_id == 0)
	{
	  icon_view->priv->scroll_timeout_id =
	    cdk_threads_add_timeout (50, drag_scroll_timeout, icon_view);
	  g_source_set_name_by_id (icon_view->priv->scroll_timeout_id, "[ctk+] drag_scroll_timeout");
	}

      if (target == cdk_atom_intern_static_string ("CTK_TREE_MODEL_ROW"))
        {
          /* Request data so we can use the source row when
           * determining whether to accept the drop
           */
          set_status_pending (context, suggested_action);
          ctk_drag_get_data (widget, context, target, time);
        }
      else
        {
          set_status_pending (context, 0);
          cdk_drag_status (context, suggested_action, time);
        }
    }

  if (path)
    ctk_tree_path_free (path);

  return TRUE;
}

static gboolean 
ctk_icon_view_drag_drop (CtkWidget      *widget,
			 CdkDragContext *context,
			 gint            x,
			 gint            y,
			 guint           time)
{
  CtkIconView *icon_view;
  CtkTreePath *path;
  CdkDragAction suggested_action = 0;
  CdkAtom target = GDK_NONE;
  CtkTreeModel *model;
  gboolean drop_append_mode;

  icon_view = CTK_ICON_VIEW (widget);
  model = ctk_icon_view_get_model (icon_view);

  remove_scroll_timeout (CTK_ICON_VIEW (widget));

  if (!icon_view->priv->dest_set)
    return FALSE;

  if (!check_model_dnd (model, CTK_TYPE_TREE_DRAG_DEST, "drag-drop"))
    return FALSE;

  if (!set_destination (icon_view, context, x, y, &suggested_action, &target))
    return FALSE;
  
  path = get_logical_destination (icon_view, &drop_append_mode);

  if (target != GDK_NONE && path != NULL)
    {
      /* in case a motion had requested drag data, change things so we
       * treat drag data receives as a drop.
       */
      set_status_pending (context, 0);
      set_dest_row (context, model, path, 
		    icon_view->priv->empty_view_drop, drop_append_mode);
    }

  if (path)
    ctk_tree_path_free (path);

  /* Unset this thing */
  ctk_icon_view_set_drag_dest_item (icon_view, NULL, CTK_ICON_VIEW_DROP_LEFT);

  if (target != GDK_NONE)
    {
      ctk_drag_get_data (widget, context, target, time);
      return TRUE;
    }
  else
    return FALSE;
}

static void
ctk_icon_view_drag_data_received (CtkWidget        *widget,
				  CdkDragContext   *context,
				  gint              x,
				  gint              y,
				  CtkSelectionData *selection_data,
				  guint             info,
				  guint             time)
{
  CtkTreePath *path;
  gboolean accepted = FALSE;
  CtkTreeModel *model;
  CtkIconView *icon_view;
  CtkTreePath *dest_row;
  CdkDragAction suggested_action;
  gboolean drop_append_mode;
  
  icon_view = CTK_ICON_VIEW (widget);  
  model = ctk_icon_view_get_model (icon_view);

  if (!check_model_dnd (model, CTK_TYPE_TREE_DRAG_DEST, "drag-data-received"))
    return;

  if (!icon_view->priv->dest_set)
    return;

  suggested_action = get_status_pending (context);

  if (suggested_action)
    {
      /* We are getting this data due to a request in drag_motion,
       * rather than due to a request in drag_drop, so we are just
       * supposed to call drag_status, not actually paste in the
       * data.
       */
      path = get_logical_destination (icon_view, &drop_append_mode);

      if (path == NULL)
        suggested_action = 0;

      if (suggested_action)
        {
	  if (!ctk_tree_drag_dest_row_drop_possible (CTK_TREE_DRAG_DEST (model),
						     path,
						     selection_data))
	    suggested_action = 0;
        }

      cdk_drag_status (context, suggested_action, time);

      if (path)
        ctk_tree_path_free (path);

      /* If you can't drop, remove user drop indicator until the next motion */
      if (suggested_action == 0)
        ctk_icon_view_set_drag_dest_item (icon_view,
					  NULL,
					  CTK_ICON_VIEW_DROP_LEFT);
      return;
    }
  

  dest_row = get_dest_row (context);

  if (dest_row == NULL)
    return;

  if (ctk_selection_data_get_length (selection_data) >= 0)
    {
      if (ctk_tree_drag_dest_drag_data_received (CTK_TREE_DRAG_DEST (model),
                                                 dest_row,
                                                 selection_data))
        accepted = TRUE;
    }

  ctk_drag_finish (context,
                   accepted,
                   (cdk_drag_context_get_selected_action (context) == GDK_ACTION_MOVE),
                   time);

  ctk_tree_path_free (dest_row);

  /* drop dest_row */
  set_dest_row (context, NULL, NULL, FALSE, FALSE);
}

/* Drag-and-Drop support */
/**
 * ctk_icon_view_enable_model_drag_source:
 * @icon_view: a #CtkIconView
 * @start_button_mask: Mask of allowed buttons to start drag
 * @targets: (array length=n_targets): the table of targets that the drag will
 *           support
 * @n_targets: the number of items in @targets
 * @actions: the bitmask of possible actions for a drag from this
 *    widget
 *
 * Turns @icon_view into a drag source for automatic DND. Calling this
 * method sets #CtkIconView:reorderable to %FALSE.
 *
 * Since: 2.8
 **/
void
ctk_icon_view_enable_model_drag_source (CtkIconView              *icon_view,
					CdkModifierType           start_button_mask,
					const CtkTargetEntry     *targets,
					gint                      n_targets,
					CdkDragAction             actions)
{
  g_return_if_fail (CTK_IS_ICON_VIEW (icon_view));

  ctk_drag_source_set (CTK_WIDGET (icon_view), 0, targets, n_targets, actions);

  icon_view->priv->start_button_mask = start_button_mask;
  icon_view->priv->source_actions = actions;

  icon_view->priv->source_set = TRUE;

  unset_reorderable (icon_view);
}

/**
 * ctk_icon_view_enable_model_drag_dest:
 * @icon_view: a #CtkIconView
 * @targets: (array length=n_targets): the table of targets that the drag will
 *           support
 * @n_targets: the number of items in @targets
 * @actions: the bitmask of possible actions for a drag to this
 *    widget
 *
 * Turns @icon_view into a drop destination for automatic DND. Calling this
 * method sets #CtkIconView:reorderable to %FALSE.
 *
 * Since: 2.8
 **/
void 
ctk_icon_view_enable_model_drag_dest (CtkIconView          *icon_view,
				      const CtkTargetEntry *targets,
				      gint                  n_targets,
				      CdkDragAction         actions)
{
  g_return_if_fail (CTK_IS_ICON_VIEW (icon_view));

  ctk_drag_dest_set (CTK_WIDGET (icon_view), 0, targets, n_targets, actions);

  icon_view->priv->dest_actions = actions;

  icon_view->priv->dest_set = TRUE;

  unset_reorderable (icon_view);  
}

/**
 * ctk_icon_view_unset_model_drag_source:
 * @icon_view: a #CtkIconView
 * 
 * Undoes the effect of ctk_icon_view_enable_model_drag_source(). Calling this
 * method sets #CtkIconView:reorderable to %FALSE.
 *
 * Since: 2.8
 **/
void
ctk_icon_view_unset_model_drag_source (CtkIconView *icon_view)
{
  g_return_if_fail (CTK_IS_ICON_VIEW (icon_view));

  if (icon_view->priv->source_set)
    {
      ctk_drag_source_unset (CTK_WIDGET (icon_view));
      icon_view->priv->source_set = FALSE;
    }

  unset_reorderable (icon_view);
}

/**
 * ctk_icon_view_unset_model_drag_dest:
 * @icon_view: a #CtkIconView
 * 
 * Undoes the effect of ctk_icon_view_enable_model_drag_dest(). Calling this
 * method sets #CtkIconView:reorderable to %FALSE.
 *
 * Since: 2.8
 **/
void
ctk_icon_view_unset_model_drag_dest (CtkIconView *icon_view)
{
  g_return_if_fail (CTK_IS_ICON_VIEW (icon_view));

  if (icon_view->priv->dest_set)
    {
      ctk_drag_dest_unset (CTK_WIDGET (icon_view));
      icon_view->priv->dest_set = FALSE;
    }

  unset_reorderable (icon_view);
}

/* These are useful to implement your own custom stuff. */
/**
 * ctk_icon_view_set_drag_dest_item:
 * @icon_view: a #CtkIconView
 * @path: (allow-none): The path of the item to highlight, or %NULL.
 * @pos: Specifies where to drop, relative to the item
 *
 * Sets the item that is highlighted for feedback.
 *
 * Since: 2.8
 */
void
ctk_icon_view_set_drag_dest_item (CtkIconView              *icon_view,
				  CtkTreePath              *path,
				  CtkIconViewDropPosition   pos)
{
  /* Note; this function is exported to allow a custom DND
   * implementation, so it can't touch TreeViewDragInfo
   */

  g_return_if_fail (CTK_IS_ICON_VIEW (icon_view));

  if (icon_view->priv->dest_item)
    {
      CtkTreePath *current_path;
      current_path = ctk_tree_row_reference_get_path (icon_view->priv->dest_item);
      ctk_tree_row_reference_free (icon_view->priv->dest_item);
      icon_view->priv->dest_item = NULL;      

      ctk_icon_view_queue_draw_path (icon_view, current_path);
      ctk_tree_path_free (current_path);
    }
  
  /* special case a drop on an empty model */
  icon_view->priv->empty_view_drop = FALSE;
  if (pos == CTK_ICON_VIEW_DROP_ABOVE && path
      && ctk_tree_path_get_depth (path) == 1
      && ctk_tree_path_get_indices (path)[0] == 0)
    {
      gint n_children;

      n_children = ctk_tree_model_iter_n_children (icon_view->priv->model,
                                                   NULL);

      if (n_children == 0)
        icon_view->priv->empty_view_drop = TRUE;
    }

  icon_view->priv->dest_pos = pos;

  if (path)
    {
      icon_view->priv->dest_item =
        ctk_tree_row_reference_new_proxy (G_OBJECT (icon_view), 
					  icon_view->priv->model, path);
      
      ctk_icon_view_queue_draw_path (icon_view, path);
    }
}

/**
 * ctk_icon_view_get_drag_dest_item:
 * @icon_view: a #CtkIconView
 * @path: (out) (allow-none): Return location for the path of
 *        the highlighted item, or %NULL.
 * @pos: (out) (allow-none): Return location for the drop position, or %NULL
 * 
 * Gets information about the item that is highlighted for feedback.
 *
 * Since: 2.8
 **/
void
ctk_icon_view_get_drag_dest_item (CtkIconView              *icon_view,
				  CtkTreePath             **path,
				  CtkIconViewDropPosition  *pos)
{
  g_return_if_fail (CTK_IS_ICON_VIEW (icon_view));

  if (path)
    {
      if (icon_view->priv->dest_item)
        *path = ctk_tree_row_reference_get_path (icon_view->priv->dest_item);
      else
	*path = NULL;
    }

  if (pos)
    *pos = icon_view->priv->dest_pos;
}

/**
 * ctk_icon_view_get_dest_item_at_pos:
 * @icon_view: a #CtkIconView
 * @drag_x: the position to determine the destination item for
 * @drag_y: the position to determine the destination item for
 * @path: (out) (allow-none): Return location for the path of the item,
 *    or %NULL.
 * @pos: (out) (allow-none): Return location for the drop position, or %NULL
 * 
 * Determines the destination item for a given position.
 * 
 * Returns: whether there is an item at the given position.
 *
 * Since: 2.8
 **/
gboolean
ctk_icon_view_get_dest_item_at_pos (CtkIconView              *icon_view,
				    gint                      drag_x,
				    gint                      drag_y,
				    CtkTreePath             **path,
				    CtkIconViewDropPosition  *pos)
{
  CtkIconViewItem *item;

  /* Note; this function is exported to allow a custom DND
   * implementation, so it can't touch TreeViewDragInfo
   */

  g_return_val_if_fail (CTK_IS_ICON_VIEW (icon_view), FALSE);
  g_return_val_if_fail (drag_x >= 0, FALSE);
  g_return_val_if_fail (drag_y >= 0, FALSE);
  g_return_val_if_fail (icon_view->priv->bin_window != NULL, FALSE);


  if (path)
    *path = NULL;

  item = _ctk_icon_view_get_item_at_coords (icon_view, 
					   drag_x + ctk_adjustment_get_value (icon_view->priv->hadjustment), 
					   drag_y + ctk_adjustment_get_value (icon_view->priv->vadjustment),
					   FALSE, NULL);

  if (item == NULL)
    return FALSE;

  if (path)
    *path = ctk_tree_path_new_from_indices (item->index, -1);

  if (pos)
    {
      if (drag_x < item->cell_area.x + item->cell_area.width / 4)
	*pos = CTK_ICON_VIEW_DROP_LEFT;
      else if (drag_x > item->cell_area.x + item->cell_area.width * 3 / 4)
	*pos = CTK_ICON_VIEW_DROP_RIGHT;
      else if (drag_y < item->cell_area.y + item->cell_area.height / 4)
	*pos = CTK_ICON_VIEW_DROP_ABOVE;
      else if (drag_y > item->cell_area.y + item->cell_area.height * 3 / 4)
	*pos = CTK_ICON_VIEW_DROP_BELOW;
      else
	*pos = CTK_ICON_VIEW_DROP_INTO;
    }

  return TRUE;
}

/**
 * ctk_icon_view_create_drag_icon:
 * @icon_view: a #CtkIconView
 * @path: a #CtkTreePath in @icon_view
 *
 * Creates a #cairo_surface_t representation of the item at @path.  
 * This image is used for a drag icon.
 *
 * Returns: (transfer full): a newly-allocated surface of the drag icon.
 * 
 * Since: 2.8
 **/
cairo_surface_t *
ctk_icon_view_create_drag_icon (CtkIconView *icon_view,
				CtkTreePath *path)
{
  CtkWidget *widget;
  cairo_t *cr;
  cairo_surface_t *surface;
  GList *l;
  gint index;

  g_return_val_if_fail (CTK_IS_ICON_VIEW (icon_view), NULL);
  g_return_val_if_fail (path != NULL, NULL);

  widget = CTK_WIDGET (icon_view);

  if (!ctk_widget_get_realized (widget))
    return NULL;

  index = ctk_tree_path_get_indices (path)[0];

  for (l = icon_view->priv->items; l; l = l->next) 
    {
      CtkIconViewItem *item = l->data;
      
      if (index == item->index)
	{
	  CdkRectangle rect = { 
	    item->cell_area.x - icon_view->priv->item_padding, 
	    item->cell_area.y - icon_view->priv->item_padding, 
	    item->cell_area.width  + icon_view->priv->item_padding * 2, 
	    item->cell_area.height + icon_view->priv->item_padding * 2 
	  };

	  surface = cdk_window_create_similar_surface (icon_view->priv->bin_window,
                                                       CAIRO_CONTENT_COLOR_ALPHA,
                                                       rect.width,
                                                       rect.height);

	  cr = cairo_create (surface);

	  ctk_icon_view_paint_item (icon_view, cr, item, 
				    icon_view->priv->item_padding,
				    icon_view->priv->item_padding,
                                    FALSE);

	  cairo_destroy (cr);

	  return surface;
	}
    }
  
  return NULL;
}

/**
 * ctk_icon_view_get_reorderable:
 * @icon_view: a #CtkIconView
 *
 * Retrieves whether the user can reorder the list via drag-and-drop. 
 * See ctk_icon_view_set_reorderable().
 *
 * Returns: %TRUE if the list can be reordered.
 *
 * Since: 2.8
 **/
gboolean
ctk_icon_view_get_reorderable (CtkIconView *icon_view)
{
  g_return_val_if_fail (CTK_IS_ICON_VIEW (icon_view), FALSE);

  return icon_view->priv->reorderable;
}

static const CtkTargetEntry item_targets[] = {
  { "CTK_TREE_MODEL_ROW", CTK_TARGET_SAME_WIDGET, 0 }
};


/**
 * ctk_icon_view_set_reorderable:
 * @icon_view: A #CtkIconView.
 * @reorderable: %TRUE, if the list of items can be reordered.
 *
 * This function is a convenience function to allow you to reorder models that
 * support the #CtkTreeDragSourceIface and the #CtkTreeDragDestIface.  Both
 * #CtkTreeStore and #CtkListStore support these.  If @reorderable is %TRUE, then
 * the user can reorder the model by dragging and dropping rows.  The
 * developer can listen to these changes by connecting to the model's
 * row_inserted and row_deleted signals. The reordering is implemented by setting up
 * the icon view as a drag source and destination. Therefore, drag and
 * drop can not be used in a reorderable view for any other purpose.
 *
 * This function does not give you any degree of control over the order -- any
 * reordering is allowed.  If more control is needed, you should probably
 * handle drag and drop manually.
 *
 * Since: 2.8
 **/
void
ctk_icon_view_set_reorderable (CtkIconView *icon_view,
			       gboolean     reorderable)
{
  g_return_if_fail (CTK_IS_ICON_VIEW (icon_view));

  reorderable = reorderable != FALSE;

  if (icon_view->priv->reorderable == reorderable)
    return;

  if (reorderable)
    {
      ctk_icon_view_enable_model_drag_source (icon_view,
					      GDK_BUTTON1_MASK,
					      item_targets,
					      G_N_ELEMENTS (item_targets),
					      GDK_ACTION_MOVE);
      ctk_icon_view_enable_model_drag_dest (icon_view,
					    item_targets,
					    G_N_ELEMENTS (item_targets),
					    GDK_ACTION_MOVE);
    }
  else
    {
      ctk_icon_view_unset_model_drag_source (icon_view);
      ctk_icon_view_unset_model_drag_dest (icon_view);
    }

  icon_view->priv->reorderable = reorderable;

  g_object_notify (G_OBJECT (icon_view), "reorderable");
}

/**
 * ctk_icon_view_set_activate_on_single_click:
 * @icon_view: a #CtkIconView
 * @single: %TRUE to emit item-activated on a single click
 *
 * Causes the #CtkIconView::item-activated signal to be emitted on
 * a single click instead of a double click.
 *
 * Since: 3.8
 **/
void
ctk_icon_view_set_activate_on_single_click (CtkIconView *icon_view,
                                            gboolean     single)
{
  g_return_if_fail (CTK_IS_ICON_VIEW (icon_view));

  single = single != FALSE;

  if (icon_view->priv->activate_on_single_click == single)
    return;

  icon_view->priv->activate_on_single_click = single;
  g_object_notify (G_OBJECT (icon_view), "activate-on-single-click");
}

/**
 * ctk_icon_view_get_activate_on_single_click:
 * @icon_view: a #CtkIconView
 *
 * Gets the setting set by ctk_icon_view_set_activate_on_single_click().
 *
 * Returns: %TRUE if item-activated will be emitted on a single click
 *
 * Since: 3.8
 **/
gboolean
ctk_icon_view_get_activate_on_single_click (CtkIconView *icon_view)
{
  g_return_val_if_fail (CTK_IS_ICON_VIEW (icon_view), FALSE);

  return icon_view->priv->activate_on_single_click;
}

static gboolean
ctk_icon_view_buildable_custom_tag_start (CtkBuildable  *buildable,
                                          CtkBuilder    *builder,
                                          GObject       *child,
                                          const gchar   *tagname,
                                          GMarkupParser *parser,
                                          gpointer      *data)
{
  if (parent_buildable_iface->custom_tag_start (buildable, builder, child,
                                                tagname, parser, data))
    return TRUE;

  return _ctk_cell_layout_buildable_custom_tag_start (buildable, builder, child,
                                                      tagname, parser, data);
}

static void
ctk_icon_view_buildable_custom_tag_end (CtkBuildable *buildable,
                                        CtkBuilder   *builder,
                                        GObject      *child,
                                        const gchar  *tagname,
                                        gpointer     *data)
{
  if (!_ctk_cell_layout_buildable_custom_tag_end (buildable, builder,
                                                  child, tagname, data))
    parent_buildable_iface->custom_tag_end (buildable, builder,
                                            child, tagname, data);
}
