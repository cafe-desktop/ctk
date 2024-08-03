/* ctkcellarea.c
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

/**
 * SECTION:ctkcellarea
 * @Short_Description: An abstract class for laying out CtkCellRenderers
 * @Title: CtkCellArea
 *
 * The #CtkCellArea is an abstract class for #CtkCellLayout widgets
 * (also referred to as "layouting widgets") to interface with an
 * arbitrary number of #CtkCellRenderers and interact with the user
 * for a given #CtkTreeModel row.
 *
 * The cell area handles events, focus navigation, drawing and
 * size requests and allocations for a given row of data.
 *
 * Usually users dont have to interact with the #CtkCellArea directly
 * unless they are implementing a cell-layouting widget themselves.
 *
 * # Requesting area sizes
 *
 * As outlined in
 * [CtkWidget’s geometry management section][geometry-management],
 * CTK+ uses a height-for-width
 * geometry management system to compute the sizes of widgets and user
 * interfaces. #CtkCellArea uses the same semantics to calculate the
 * size of an area for an arbitrary number of #CtkTreeModel rows.
 *
 * When requesting the size of a cell area one needs to calculate
 * the size for a handful of rows, and this will be done differently by
 * different layouting widgets. For instance a #CtkTreeViewColumn
 * always lines up the areas from top to bottom while a #CtkIconView
 * on the other hand might enforce that all areas received the same
 * width and wrap the areas around, requesting height for more cell
 * areas when allocated less width.
 *
 * It’s also important for areas to maintain some cell
 * alignments with areas rendered for adjacent rows (cells can
 * appear “columnized” inside an area even when the size of
 * cells are different in each row). For this reason the #CtkCellArea
 * uses a #CtkCellAreaContext object to store the alignments
 * and sizes along the way (as well as the overall largest minimum
 * and natural size for all the rows which have been calculated
 * with the said context).
 *
 * The #CtkCellAreaContext is an opaque object specific to the
 * #CtkCellArea which created it (see ctk_cell_area_create_context()).
 * The owning cell-layouting widget can create as many contexts as
 * it wishes to calculate sizes of rows which should receive the
 * same size in at least one orientation (horizontally or vertically),
 * However, it’s important that the same #CtkCellAreaContext which
 * was used to request the sizes for a given #CtkTreeModel row be
 * used when rendering or processing events for that row.
 *
 * In order to request the width of all the rows at the root level
 * of a #CtkTreeModel one would do the following:
 *
 * |[<!-- language="C" -->
 * CtkTreeIter iter;
 * gint        minimum_width;
 * gint        natural_width;
 *
 * valid = ctk_tree_model_get_iter_first (model, &iter);
 * while (valid)
 *   {
 *     ctk_cell_area_apply_attributes (area, model, &iter, FALSE, FALSE);
 *     ctk_cell_area_get_preferred_width (area, context, widget, NULL, NULL);
 *
 *     valid = ctk_tree_model_iter_next (model, &iter);
 *   }
 * ctk_cell_area_context_get_preferred_width (context, &minimum_width, &natural_width);
 * ]|
 *
 * Note that in this example it’s not important to observe the
 * returned minimum and natural width of the area for each row
 * unless the cell-layouting object is actually interested in the
 * widths of individual rows. The overall width is however stored
 * in the accompanying #CtkCellAreaContext object and can be consulted
 * at any time.
 *
 * This can be useful since #CtkCellLayout widgets usually have to
 * support requesting and rendering rows in treemodels with an
 * exceedingly large amount of rows. The #CtkCellLayout widget in
 * that case would calculate the required width of the rows in an
 * idle or timeout source (see g_timeout_add()) and when the widget
 * is requested its actual width in #CtkWidgetClass.get_preferred_width()
 * it can simply consult the width accumulated so far in the
 * #CtkCellAreaContext object.
 *
 * A simple example where rows are rendered from top to bottom and
 * take up the full width of the layouting widget would look like:
 *
 * |[<!-- language="C" -->
 * static void
 * foo_get_preferred_width (CtkWidget       *widget,
 *                          gint            *minimum_size,
 *                          gint            *natural_size)
 * {
 *   Foo        *foo  = FOO (widget);
 *   FooPrivate *priv = foo->priv;
 *
 *   foo_ensure_at_least_one_handfull_of_rows_have_been_requested (foo);
 *
 *   ctk_cell_area_context_get_preferred_width (priv->context, minimum_size, natural_size);
 * }
 * ]|
 *
 * In the above example the Foo widget has to make sure that some
 * row sizes have been calculated (the amount of rows that Foo judged
 * was appropriate to request space for in a single timeout iteration)
 * before simply returning the amount of space required by the area via
 * the #CtkCellAreaContext.
 *
 * Requesting the height for width (or width for height) of an area is
 * a similar task except in this case the #CtkCellAreaContext does not
 * store the data (actually, it does not know how much space the layouting
 * widget plans to allocate it for every row. It’s up to the layouting
 * widget to render each row of data with the appropriate height and
 * width which was requested by the #CtkCellArea).
 *
 * In order to request the height for width of all the rows at the
 * root level of a #CtkTreeModel one would do the following:
 *
 * |[<!-- language="C" -->
 * CtkTreeIter iter;
 * gint        minimum_height;
 * gint        natural_height;
 * gint        full_minimum_height = 0;
 * gint        full_natural_height = 0;
 *
 * valid = ctk_tree_model_get_iter_first (model, &iter);
 * while (valid)
 *   {
 *     ctk_cell_area_apply_attributes (area, model, &iter, FALSE, FALSE);
 *     ctk_cell_area_get_preferred_height_for_width (area, context, widget,
 *                                                   width, &minimum_height, &natural_height);
 *
 *     if (width_is_for_allocation)
 *        cache_row_height (&iter, minimum_height, natural_height);
 *
 *     full_minimum_height += minimum_height;
 *     full_natural_height += natural_height;
 *
 *     valid = ctk_tree_model_iter_next (model, &iter);
 *   }
 * ]|
 *
 * Note that in the above example we would need to cache the heights
 * returned for each row so that we would know what sizes to render the
 * areas for each row. However we would only want to really cache the
 * heights if the request is intended for the layouting widgets real
 * allocation.
 *
 * In some cases the layouting widget is requested the height for an
 * arbitrary for_width, this is a special case for layouting widgets
 * who need to request size for tens of thousands  of rows. For this
 * case it’s only important that the layouting widget calculate
 * one reasonably sized chunk of rows and return that height
 * synchronously. The reasoning here is that any layouting widget is
 * at least capable of synchronously calculating enough height to fill
 * the screen height (or scrolled window height) in response to a single
 * call to #CtkWidgetClass.get_preferred_height_for_width(). Returning
 * a perfect height for width that is larger than the screen area is
 * inconsequential since after the layouting receives an allocation
 * from a scrolled window it simply continues to drive the scrollbar
 * values while more and more height is required for the row heights
 * that are calculated in the background.
 *
 * # Rendering Areas
 *
 * Once area sizes have been aquired at least for the rows in the
 * visible area of the layouting widget they can be rendered at
 * #CtkWidgetClass.draw() time.
 *
 * A crude example of how to render all the rows at the root level
 * runs as follows:
 *
 * |[<!-- language="C" -->
 * CtkAllocation allocation;
 * CdkRectangle  cell_area = { 0, };
 * CtkTreeIter   iter;
 * gint          minimum_width;
 * gint          natural_width;
 *
 * ctk_widget_get_allocation (widget, &allocation);
 * cell_area.width = allocation.width;
 *
 * valid = ctk_tree_model_get_iter_first (model, &iter);
 * while (valid)
 *   {
 *     cell_area.height = get_cached_height_for_row (&iter);
 *
 *     ctk_cell_area_apply_attributes (area, model, &iter, FALSE, FALSE);
 *     ctk_cell_area_render (area, context, widget, cr,
 *                           &cell_area, &cell_area, state_flags, FALSE);
 *
 *     cell_area.y += cell_area.height;
 *
 *     valid = ctk_tree_model_iter_next (model, &iter);
 *   }
 * ]|
 *
 * Note that the cached height in this example really depends on how
 * the layouting widget works. The layouting widget might decide to
 * give every row its minimum or natural height or, if the model content
 * is expected to fit inside the layouting widget without scrolling, it
 * would make sense to calculate the allocation for each row at
 * #CtkWidget::size-allocate time using ctk_distribute_natural_allocation().
 *
 * # Handling Events and Driving Keyboard Focus
 *
 * Passing events to the area is as simple as handling events on any
 * normal widget and then passing them to the ctk_cell_area_event()
 * API as they come in. Usually #CtkCellArea is only interested in
 * button events, however some customized derived areas can be implemented
 * who are interested in handling other events. Handling an event can
 * trigger the #CtkCellArea::focus-changed signal to fire; as well as
 * #CtkCellArea::add-editable in the case that an editable cell was
 * clicked and needs to start editing. You can call
 * ctk_cell_area_stop_editing() at any time to cancel any cell editing
 * that is currently in progress.
 *
 * The #CtkCellArea drives keyboard focus from cell to cell in a way
 * similar to #CtkWidget. For layouting widgets that support giving
 * focus to cells it’s important to remember to pass %CTK_CELL_RENDERER_FOCUSED
 * to the area functions for the row that has focus and to tell the
 * area to paint the focus at render time.
 *
 * Layouting widgets that accept focus on cells should implement the
 * #CtkWidgetClass.focus() virtual method. The layouting widget is always
 * responsible for knowing where #CtkTreeModel rows are rendered inside
 * the widget, so at #CtkWidgetClass.focus() time the layouting widget
 * should use the #CtkCellArea methods to navigate focus inside the area
 * and then observe the CtkDirectionType to pass the focus to adjacent
 * rows and areas.
 *
 * A basic example of how the #CtkWidgetClass.focus() virtual method
 * should be implemented:
 *
 * |[<!-- language="C" -->
 * static gboolean
 * foo_focus (CtkWidget       *widget,
 *            CtkDirectionType direction)
 * {
 *   Foo        *foo  = FOO (widget);
 *   FooPrivate *priv = foo->priv;
 *   gint        focus_row;
 *   gboolean    have_focus = FALSE;
 *
 *   focus_row = priv->focus_row;
 *
 *   if (!ctk_widget_has_focus (widget))
 *     ctk_widget_grab_focus (widget);
 *
 *   valid = ctk_tree_model_iter_nth_child (priv->model, &iter, NULL, priv->focus_row);
 *   while (valid)
 *     {
 *       ctk_cell_area_apply_attributes (priv->area, priv->model, &iter, FALSE, FALSE);
 *
 *       if (ctk_cell_area_focus (priv->area, direction))
 *         {
 *            priv->focus_row = focus_row;
 *            have_focus = TRUE;
 *            break;
 *         }
 *       else
 *         {
 *           if (direction == CTK_DIR_RIGHT ||
 *               direction == CTK_DIR_LEFT)
 *             break;
 *           else if (direction == CTK_DIR_UP ||
 *                    direction == CTK_DIR_TAB_BACKWARD)
 *            {
 *               if (focus_row == 0)
 *                 break;
 *               else
 *                {
 *                   focus_row--;
 *                   valid = ctk_tree_model_iter_nth_child (priv->model, &iter, NULL, focus_row);
 *                }
 *             }
 *           else
 *             {
 *               if (focus_row == last_row)
 *                 break;
 *               else
 *                 {
 *                   focus_row++;
 *                   valid = ctk_tree_model_iter_next (priv->model, &iter);
 *                 }
 *             }
 *         }
 *     }
 *     return have_focus;
 * }
 * ]|
 *
 * Note that the layouting widget is responsible for matching the
 * CtkDirectionType values to the way it lays out its cells.
 *
 * # Cell Properties
 *
 * The #CtkCellArea introduces cell properties for #CtkCellRenderers
 * in very much the same way that #CtkContainer introduces
 * [child properties][child-properties]
 * for #CtkWidgets. This provides some general interfaces for defining
 * the relationship cell areas have with their cells. For instance in a
 * #CtkCellAreaBox a cell might “expand” and receive extra space when
 * the area is allocated more than its full natural request, or a cell
 * might be configured to “align” with adjacent rows which were requested
 * and rendered with the same #CtkCellAreaContext.
 *
 * Use ctk_cell_area_class_install_cell_property() to install cell
 * properties for a cell area class and ctk_cell_area_class_find_cell_property()
 * or ctk_cell_area_class_list_cell_properties() to get information about
 * existing cell properties.
 *
 * To set the value of a cell property, use ctk_cell_area_cell_set_property(),
 * ctk_cell_area_cell_set() or ctk_cell_area_cell_set_valist(). To obtain
 * the value of a cell property, use ctk_cell_area_cell_get_property(),
 * ctk_cell_area_cell_get() or ctk_cell_area_cell_get_valist().
 */

#include "config.h"

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "ctkintl.h"
#include "ctkcelllayout.h"
#include "ctkcellarea.h"
#include "ctkcellareacontext.h"
#include "ctkmarshalers.h"
#include "ctkprivate.h"
#include "ctkrender.h"

#include <gobject/gvaluecollector.h>


/* GObjectClass */
static void      ctk_cell_area_dispose                             (GObject            *object);
static void      ctk_cell_area_finalize                            (GObject            *object);
static void      ctk_cell_area_set_property                        (GObject            *object,
                                                                    guint               prop_id,
                                                                    const GValue       *value,
                                                                    GParamSpec         *pspec);
static void      ctk_cell_area_get_property                        (GObject            *object,
                                                                    guint               prop_id,
                                                                    GValue             *value,
                                                                    GParamSpec         *pspec);

/* CtkCellAreaClass */
static void      ctk_cell_area_real_add                            (CtkCellArea         *area,
								    CtkCellRenderer     *renderer);
static void      ctk_cell_area_real_remove                         (CtkCellArea         *area,
								    CtkCellRenderer     *renderer);
static void      ctk_cell_area_real_foreach                        (CtkCellArea         *area,
								    CtkCellCallback      callback,
								    gpointer             callback_data);
static void      ctk_cell_area_real_foreach_alloc                  (CtkCellArea         *area,
								    CtkCellAreaContext  *context,
								    CtkWidget           *widget,
								    const CdkRectangle  *cell_area,
								    const CdkRectangle  *background_area,
								    CtkCellAllocCallback callback,
								    gpointer             callback_data);
static gint      ctk_cell_area_real_event                          (CtkCellArea          *area,
                                                                    CtkCellAreaContext   *context,
                                                                    CtkWidget            *widget,
                                                                    CdkEvent             *event,
                                                                    const CdkRectangle   *cell_area,
                                                                    CtkCellRendererState  flags);
static void      ctk_cell_area_real_render                         (CtkCellArea          *area,
                                                                    CtkCellAreaContext   *context,
                                                                    CtkWidget            *widget,
                                                                    cairo_t              *cr,
                                                                    const CdkRectangle   *background_area,
                                                                    const CdkRectangle   *cell_area,
                                                                    CtkCellRendererState  flags,
                                                                    gboolean              paint_focus);
static void      ctk_cell_area_real_apply_attributes               (CtkCellArea           *area,
                                                                    CtkTreeModel          *tree_model,
                                                                    CtkTreeIter           *iter,
                                                                    gboolean               is_expander,
                                                                    gboolean               is_expanded);

static CtkCellAreaContext *ctk_cell_area_real_create_context       (CtkCellArea           *area);
static CtkCellAreaContext *ctk_cell_area_real_copy_context         (CtkCellArea           *area,
								    CtkCellAreaContext    *context);
static CtkSizeRequestMode  ctk_cell_area_real_get_request_mode     (CtkCellArea           *area);
static void      ctk_cell_area_real_get_preferred_width            (CtkCellArea           *area,
								    CtkCellAreaContext    *context,
								    CtkWidget             *widget,
								    gint                  *minimum_width,
								    gint                  *natural_width);
static void      ctk_cell_area_real_get_preferred_height           (CtkCellArea           *area,
								    CtkCellAreaContext    *context,
								    CtkWidget             *widget,
								    gint                  *minimum_height,
								    gint                  *natural_height);
static void      ctk_cell_area_real_get_preferred_height_for_width (CtkCellArea           *area,
                                                                    CtkCellAreaContext    *context,
                                                                    CtkWidget             *widget,
                                                                    gint                   width,
                                                                    gint                  *minimum_height,
                                                                    gint                  *natural_height);
static void      ctk_cell_area_real_get_preferred_width_for_height (CtkCellArea           *area,
                                                                    CtkCellAreaContext    *context,
                                                                    CtkWidget             *widget,
                                                                    gint                   height,
                                                                    gint                  *minimum_width,
                                                                    gint                  *natural_width);
static gboolean  ctk_cell_area_real_is_activatable                 (CtkCellArea           *area);
static gboolean  ctk_cell_area_real_activate                       (CtkCellArea           *area,
                                                                    CtkCellAreaContext    *context,
                                                                    CtkWidget             *widget,
                                                                    const CdkRectangle    *cell_area,
                                                                    CtkCellRendererState   flags,
                                                                    gboolean               edit_only);
static gboolean  ctk_cell_area_real_focus                          (CtkCellArea           *area,
								    CtkDirectionType       direction);

/* CtkCellLayoutIface */
static void      ctk_cell_area_cell_layout_init              (CtkCellLayoutIface    *iface);
static void      ctk_cell_area_pack_default                  (CtkCellLayout         *cell_layout,
                                                              CtkCellRenderer       *renderer,
                                                              gboolean               expand);
static void      ctk_cell_area_clear                         (CtkCellLayout         *cell_layout);
static void      ctk_cell_area_add_attribute                 (CtkCellLayout         *cell_layout,
                                                              CtkCellRenderer       *renderer,
                                                              const gchar           *attribute,
                                                              gint                   column);
static void      ctk_cell_area_set_cell_data_func            (CtkCellLayout         *cell_layout,
                                                              CtkCellRenderer       *cell,
                                                              CtkCellLayoutDataFunc  func,
                                                              gpointer               func_data,
                                                              GDestroyNotify         destroy);
static void      ctk_cell_area_clear_attributes              (CtkCellLayout         *cell_layout,
                                                              CtkCellRenderer       *renderer);
static void      ctk_cell_area_reorder                       (CtkCellLayout         *cell_layout,
                                                              CtkCellRenderer       *cell,
                                                              gint                   position);
static GList    *ctk_cell_area_get_cells                     (CtkCellLayout         *cell_layout);
static CtkCellArea *ctk_cell_area_get_area                   (CtkCellLayout         *cell_layout);

/* CtkBuildableIface */
static void      ctk_cell_area_buildable_init                (CtkBuildableIface     *iface);
static void      ctk_cell_area_buildable_custom_tag_end      (CtkBuildable          *buildable,
                                                              CtkBuilder            *builder,
                                                              GObject               *child,
                                                              const gchar           *tagname,
                                                              gpointer              *data);

/* Used in foreach loop to check if a child renderer is present */
typedef struct {
  CtkCellRenderer *renderer;
  gboolean         has_renderer;
} HasRendererCheck;

/* Used in foreach loop to get a cell's allocation */
typedef struct {
  CtkCellRenderer *renderer;
  CdkRectangle     allocation;
} RendererAllocationData;

/* Used in foreach loop to render cells */
typedef struct {
  CtkCellArea         *area;
  CtkWidget           *widget;
  cairo_t             *cr;
  CdkRectangle         focus_rect;
  CtkCellRendererState render_flags;
  guint                paint_focus : 1;
  guint                focus_all   : 1;
  guint                first_focus : 1;
} CellRenderData;

/* Used in foreach loop to get a cell by position */
typedef struct {
  gint             x;
  gint             y;
  CtkCellRenderer *renderer;
  CdkRectangle     cell_area;
} CellByPositionData;

/* Attribute/Cell metadata */
typedef struct {
  const gchar *attribute;
  gint         column;
} CellAttribute;

typedef struct {
  GSList          *attributes;

  CtkCellLayoutDataFunc  func;
  gpointer               data;
  GDestroyNotify         destroy;
  CtkCellLayout         *proxy;
} CellInfo;

static CellInfo       *cell_info_new       (CtkCellLayoutDataFunc  func,
                                            gpointer               data,
                                            GDestroyNotify         destroy);
static void            cell_info_free      (CellInfo              *info);
static CellAttribute  *cell_attribute_new  (CtkCellRenderer       *renderer,
                                            const gchar           *attribute,
                                            gint                   column);
static void            cell_attribute_free (CellAttribute         *attribute);
static gint            cell_attribute_find (CellAttribute         *cell_attribute,
                                            const gchar           *attribute);

/* Internal functions/signal emissions */
static void            ctk_cell_area_add_editable     (CtkCellArea        *area,
                                                       CtkCellRenderer    *renderer,
                                                       CtkCellEditable    *editable,
                                                       const CdkRectangle *cell_area);
static void            ctk_cell_area_remove_editable  (CtkCellArea        *area,
                                                       CtkCellRenderer    *renderer,
                                                       CtkCellEditable    *editable);
static void            ctk_cell_area_set_edit_widget  (CtkCellArea        *area,
                                                       CtkCellEditable    *editable);
static void            ctk_cell_area_set_edited_cell  (CtkCellArea        *area,
                                                       CtkCellRenderer    *renderer);


/* Struct to pass data along while looping over
 * cell renderers to apply attributes
 */
typedef struct {
  CtkCellArea  *area;
  CtkTreeModel *model;
  CtkTreeIter  *iter;
  gboolean      is_expander;
  gboolean      is_expanded;
} AttributeData;

struct _CtkCellAreaPrivate
{
  /* The CtkCellArea bookkeeps any connected
   * attributes in this hash table.
   */
  GHashTable      *cell_info;

  /* Current path is saved as a side-effect
   * of ctk_cell_area_apply_attributes()
   */
  gchar           *current_path;

  /* Current cell being edited and editable widget used */
  CtkCellEditable *edit_widget;
  CtkCellRenderer *edited_cell;

  /* Signal connections to the editable widget */
  gulong           remove_widget_id;

  /* Currently focused cell */
  CtkCellRenderer *focus_cell;

  /* Tracking which cells are focus siblings of focusable cells */
  GHashTable      *focus_siblings;
};

enum {
  PROP_0,
  PROP_FOCUS_CELL,
  PROP_EDITED_CELL,
  PROP_EDIT_WIDGET
};

enum {
  SIGNAL_APPLY_ATTRIBUTES,
  SIGNAL_ADD_EDITABLE,
  SIGNAL_REMOVE_EDITABLE,
  SIGNAL_FOCUS_CHANGED,
  LAST_SIGNAL
};

/* Keep the paramspec pool internal, no need to deliver notifications
 * on cells. at least no perceived need for now
 */
static GParamSpecPool *cell_property_pool = NULL;
static guint           cell_area_signals[LAST_SIGNAL] = { 0 };

#define PARAM_SPEC_PARAM_ID(pspec)              ((pspec)->param_id)
#define PARAM_SPEC_SET_PARAM_ID(pspec, id)      ((pspec)->param_id = (id))

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (CtkCellArea, ctk_cell_area, G_TYPE_INITIALLY_UNOWNED,
                                  G_ADD_PRIVATE (CtkCellArea)
                                  G_IMPLEMENT_INTERFACE (CTK_TYPE_CELL_LAYOUT,
                                                         ctk_cell_area_cell_layout_init)
                                  G_IMPLEMENT_INTERFACE (CTK_TYPE_BUILDABLE,
                                                         ctk_cell_area_buildable_init))

static void
ctk_cell_area_init (CtkCellArea *area)
{
  CtkCellAreaPrivate *priv;

  area->priv = ctk_cell_area_get_instance_private (area);
  priv = area->priv;

  priv->cell_info = g_hash_table_new_full (g_direct_hash,
                                           g_direct_equal,
                                           NULL,
                                           (GDestroyNotify)cell_info_free);

  priv->focus_siblings = g_hash_table_new_full (g_direct_hash,
                                                g_direct_equal,
                                                NULL,
                                                (GDestroyNotify)g_list_free);

  priv->focus_cell         = NULL;
  priv->edited_cell        = NULL;
  priv->edit_widget        = NULL;

  priv->remove_widget_id   = 0;
}

static void
ctk_cell_area_class_init (CtkCellAreaClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  /* GObjectClass */
  object_class->dispose      = ctk_cell_area_dispose;
  object_class->finalize     = ctk_cell_area_finalize;
  object_class->get_property = ctk_cell_area_get_property;
  object_class->set_property = ctk_cell_area_set_property;

  /* general */
  class->add              = ctk_cell_area_real_add;
  class->remove           = ctk_cell_area_real_remove;
  class->foreach          = ctk_cell_area_real_foreach;
  class->foreach_alloc    = ctk_cell_area_real_foreach_alloc;
  class->event            = ctk_cell_area_real_event;
  class->render           = ctk_cell_area_real_render;
  class->apply_attributes = ctk_cell_area_real_apply_attributes;

  /* geometry */
  class->create_context                 = ctk_cell_area_real_create_context;
  class->copy_context                   = ctk_cell_area_real_copy_context;
  class->get_request_mode               = ctk_cell_area_real_get_request_mode;
  class->get_preferred_width            = ctk_cell_area_real_get_preferred_width;
  class->get_preferred_height           = ctk_cell_area_real_get_preferred_height;
  class->get_preferred_height_for_width = ctk_cell_area_real_get_preferred_height_for_width;
  class->get_preferred_width_for_height = ctk_cell_area_real_get_preferred_width_for_height;

  /* focus */
  class->is_activatable = ctk_cell_area_real_is_activatable;
  class->activate       = ctk_cell_area_real_activate;
  class->focus          = ctk_cell_area_real_focus;

  /* Signals */
  /**
   * CtkCellArea::apply-attributes:
   * @area: the #CtkCellArea to apply the attributes to
   * @model: the #CtkTreeModel to apply the attributes from
   * @iter: the #CtkTreeIter indicating which row to apply the attributes of
   * @is_expander: whether the view shows children for this row
   * @is_expanded: whether the view is currently showing the children of this row
   *
   * This signal is emitted whenever applying attributes to @area from @model
   *
   * Since: 3.0
   */
  cell_area_signals[SIGNAL_APPLY_ATTRIBUTES] =
    g_signal_new (I_("apply-attributes"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (CtkCellAreaClass, apply_attributes),
                  NULL, NULL,
                  _ctk_marshal_VOID__OBJECT_BOXED_BOOLEAN_BOOLEAN,
                  G_TYPE_NONE, 4,
                  CTK_TYPE_TREE_MODEL,
                  CTK_TYPE_TREE_ITER,
                  G_TYPE_BOOLEAN,
                  G_TYPE_BOOLEAN);
  g_signal_set_va_marshaller (cell_area_signals[SIGNAL_APPLY_ATTRIBUTES], G_TYPE_FROM_CLASS (class),
                              _ctk_marshal_VOID__OBJECT_BOXED_BOOLEAN_BOOLEANv);

  /**
   * CtkCellArea::add-editable:
   * @area: the #CtkCellArea where editing started
   * @renderer: the #CtkCellRenderer that started the edited
   * @editable: the #CtkCellEditable widget to add
   * @cell_area: the #CtkWidget relative #CdkRectangle coordinates
   *             where @editable should be added
   * @path: the #CtkTreePath string this edit was initiated for
   *
   * Indicates that editing has started on @renderer and that @editable
   * should be added to the owning cell-layouting widget at @cell_area.
   *
   * Since: 3.0
   */
  cell_area_signals[SIGNAL_ADD_EDITABLE] =
    g_signal_new (I_("add-editable"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST,
                  0, /* No class closure here */
                  NULL, NULL,
                  _ctk_marshal_VOID__OBJECT_OBJECT_BOXED_STRING,
                  G_TYPE_NONE, 4,
                  CTK_TYPE_CELL_RENDERER,
                  CTK_TYPE_CELL_EDITABLE,
                  CDK_TYPE_RECTANGLE,
                  G_TYPE_STRING);


  /**
   * CtkCellArea::remove-editable:
   * @area: the #CtkCellArea where editing finished
   * @renderer: the #CtkCellRenderer that finished editeding
   * @editable: the #CtkCellEditable widget to remove
   *
   * Indicates that editing finished on @renderer and that @editable
   * should be removed from the owning cell-layouting widget.
   *
   * Since: 3.0
   */
  cell_area_signals[SIGNAL_REMOVE_EDITABLE] =
    g_signal_new (I_("remove-editable"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST,
                  0, /* No class closure here */
                  NULL, NULL,
                  _ctk_marshal_VOID__OBJECT_OBJECT,
                  G_TYPE_NONE, 2,
                  CTK_TYPE_CELL_RENDERER,
                  CTK_TYPE_CELL_EDITABLE);

  /**
   * CtkCellArea::focus-changed:
   * @area: the #CtkCellArea where focus changed
   * @renderer: the #CtkCellRenderer that has focus
   * @path: the current #CtkTreePath string set for @area
   *
   * Indicates that focus changed on this @area. This signal
   * is emitted either as a result of focus handling or event
   * handling.
   *
   * It's possible that the signal is emitted even if the
   * currently focused renderer did not change, this is
   * because focus may change to the same renderer in the
   * same cell area for a different row of data.
   *
   * Since: 3.0
   */
  cell_area_signals[SIGNAL_FOCUS_CHANGED] =
    g_signal_new (I_("focus-changed"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST,
                  0, /* No class closure here */
                  NULL, NULL,
                  _ctk_marshal_VOID__OBJECT_STRING,
                  G_TYPE_NONE, 2,
                  CTK_TYPE_CELL_RENDERER,
                  G_TYPE_STRING);

  /* Properties */
  /**
   * CtkCellArea:focus-cell:
   *
   * The cell in the area that currently has focus
   *
   * Since: 3.0
   */
  g_object_class_install_property (object_class,
                                   PROP_FOCUS_CELL,
                                   g_param_spec_object
                                   ("focus-cell",
                                    P_("Focus Cell"),
                                    P_("The cell which currently has focus"),
                                    CTK_TYPE_CELL_RENDERER,
                                    CTK_PARAM_READWRITE));

  /**
   * CtkCellArea:edited-cell:
   *
   * The cell in the area that is currently edited
   *
   * This property is read-only and only changes as
   * a result of a call ctk_cell_area_activate_cell().
   *
   * Since: 3.0
   */
  g_object_class_install_property (object_class,
                                   PROP_EDITED_CELL,
                                   g_param_spec_object
                                   ("edited-cell",
                                    P_("Edited Cell"),
                                    P_("The cell which is currently being edited"),
                                    CTK_TYPE_CELL_RENDERER,
                                    G_PARAM_READABLE));

  /**
   * CtkCellArea:edit-widget:
   *
   * The widget currently editing the edited cell
   *
   * This property is read-only and only changes as
   * a result of a call ctk_cell_area_activate_cell().
   *
   * Since: 3.0
   */
  g_object_class_install_property (object_class,
                                   PROP_EDIT_WIDGET,
                                   g_param_spec_object
                                   ("edit-widget",
                                    P_("Edit Widget"),
                                    P_("The widget currently editing the edited cell"),
                                    CTK_TYPE_CELL_EDITABLE,
                                    G_PARAM_READABLE));

  /* Pool for Cell Properties */
  if (!cell_property_pool)
    cell_property_pool = g_param_spec_pool_new (FALSE);
}

/*************************************************************
 *                    CellInfo Basics                        *
 *************************************************************/
static CellInfo *
cell_info_new (CtkCellLayoutDataFunc  func,
               gpointer               data,
               GDestroyNotify         destroy)
{
  CellInfo *info = g_slice_new0 (CellInfo);

  info->func     = func;
  info->data     = data;
  info->destroy  = destroy;

  return info;
}

static void
cell_info_free (CellInfo *info)
{
  if (info->destroy)
    info->destroy (info->data);

  g_slist_free_full (info->attributes, (GDestroyNotify)cell_attribute_free);

  g_slice_free (CellInfo, info);
}

static CellAttribute  *
cell_attribute_new  (CtkCellRenderer       *renderer,
                     const gchar           *attribute,
                     gint                   column)
{
  GParamSpec *pspec;

  /* Check if the attribute really exists and point to
   * the property string installed on the cell renderer
   * class (dont dup the string)
   */
  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (renderer), attribute);

  if (pspec)
    {
      CellAttribute *cell_attribute = g_slice_new (CellAttribute);

      cell_attribute->attribute = pspec->name;
      cell_attribute->column    = column;

      return cell_attribute;
    }

  return NULL;
}

static void
cell_attribute_free (CellAttribute *attribute)
{
  g_slice_free (CellAttribute, attribute);
}

/* GCompareFunc for g_slist_find_custom() */
static gint
cell_attribute_find (CellAttribute *cell_attribute,
                     const gchar   *attribute)
{
  return g_strcmp0 (cell_attribute->attribute, attribute);
}

/*************************************************************
 *                      GObjectClass                         *
 *************************************************************/
static void
ctk_cell_area_finalize (GObject *object)
{
  CtkCellArea        *area   = CTK_CELL_AREA (object);
  CtkCellAreaPrivate *priv   = area->priv;

  /* All cell renderers should already be removed at this point,
   * just kill our (empty) hash tables here.
   */
  g_hash_table_destroy (priv->cell_info);
  g_hash_table_destroy (priv->focus_siblings);

  g_free (priv->current_path);

  G_OBJECT_CLASS (ctk_cell_area_parent_class)->finalize (object);
}


static void
ctk_cell_area_dispose (GObject *object)
{
  /* This removes every cell renderer that may be added to the CtkCellArea,
   * subclasses should be breaking references to the CtkCellRenderers
   * at this point.
   */
  ctk_cell_layout_clear (CTK_CELL_LAYOUT (object));

  /* Remove any ref to a focused/edited cell */
  ctk_cell_area_set_focus_cell (CTK_CELL_AREA (object), NULL);
  ctk_cell_area_set_edited_cell (CTK_CELL_AREA (object), NULL);
  ctk_cell_area_set_edit_widget (CTK_CELL_AREA (object), NULL);

  G_OBJECT_CLASS (ctk_cell_area_parent_class)->dispose (object);
}

static void
ctk_cell_area_set_property (GObject       *object,
                            guint          prop_id,
                            const GValue  *value,
                            GParamSpec    *pspec)
{
  CtkCellArea *area = CTK_CELL_AREA (object);

  switch (prop_id)
    {
    case PROP_FOCUS_CELL:
      ctk_cell_area_set_focus_cell (area, (CtkCellRenderer *)g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_cell_area_get_property (GObject     *object,
                            guint        prop_id,
                            GValue      *value,
                            GParamSpec  *pspec)
{
  CtkCellArea        *area = CTK_CELL_AREA (object);
  CtkCellAreaPrivate *priv = area->priv;

  switch (prop_id)
    {
    case PROP_FOCUS_CELL:
      g_value_set_object (value, priv->focus_cell);
      break;
    case PROP_EDITED_CELL:
      g_value_set_object (value, priv->edited_cell);
      break;
    case PROP_EDIT_WIDGET:
      g_value_set_object (value, priv->edit_widget);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/*************************************************************
 *                    CtkCellAreaClass                       *
 *************************************************************/
static void
ctk_cell_area_real_add (CtkCellArea         *area,
			CtkCellRenderer     *renderer G_GNUC_UNUSED)
{
    g_warning ("CtkCellAreaClass::add not implemented for '%s'",
               g_type_name (G_TYPE_FROM_INSTANCE (area)));
}

static void      
ctk_cell_area_real_remove (CtkCellArea         *area,
			   CtkCellRenderer     *renderer G_GNUC_UNUSED)
{
    g_warning ("CtkCellAreaClass::remove not implemented for '%s'",
               g_type_name (G_TYPE_FROM_INSTANCE (area)));
}

static void
ctk_cell_area_real_foreach (CtkCellArea         *area,
			    CtkCellCallback      callback G_GNUC_UNUSED,
			    gpointer             callback_data G_GNUC_UNUSED)
{
    g_warning ("CtkCellAreaClass::foreach not implemented for '%s'",
               g_type_name (G_TYPE_FROM_INSTANCE (area)));
}

static void
ctk_cell_area_real_foreach_alloc (CtkCellArea         *area,
				  CtkCellAreaContext  *context G_GNUC_UNUSED,
				  CtkWidget           *widget G_GNUC_UNUSED,
				  const CdkRectangle  *cell_area G_GNUC_UNUSED,
				  const CdkRectangle  *background_area G_GNUC_UNUSED,
				  CtkCellAllocCallback callback G_GNUC_UNUSED,
				  gpointer             callback_data G_GNUC_UNUSED)
{
    g_warning ("CtkCellAreaClass::foreach_alloc not implemented for '%s'",
               g_type_name (G_TYPE_FROM_INSTANCE (area)));
}

static gint
ctk_cell_area_real_event (CtkCellArea          *area,
                          CtkCellAreaContext   *context,
                          CtkWidget            *widget,
                          CdkEvent             *event,
                          const CdkRectangle   *cell_area,
                          CtkCellRendererState  flags)
{
  CtkCellAreaPrivate *priv = area->priv;
  gboolean            retval = FALSE;

  if (event->type == CDK_KEY_PRESS && (flags & CTK_CELL_RENDERER_FOCUSED) != 0)
    {
      CdkEventKey *key_event = (CdkEventKey *)event;

      /* Cancel any edits in progress */
      if (priv->edited_cell && (key_event->keyval == CDK_KEY_Escape))
        {
          ctk_cell_area_stop_editing (area, TRUE);
          retval = TRUE;
        }
    }
  else if (event->type == CDK_BUTTON_PRESS)
    {
      CdkEventButton *button_event = (CdkEventButton *)event;

      if (button_event->button == CDK_BUTTON_PRIMARY)
        {
          CtkCellRenderer *renderer = NULL;
          CtkCellRenderer *focus_renderer;
          CdkRectangle     alloc_area;
          gint             event_x, event_y;

          /* We may need some semantics to tell us the offset of the event
           * window we are handling events for (i.e. CtkTreeView has a bin_window) */
          event_x = button_event->x;
          event_y = button_event->y;

          /* Dont try to search for an event coordinate that is not in the area, that will
           * trigger a runtime warning.
           */
          if (event_x >= cell_area->x && event_x <= cell_area->x + cell_area->width &&
              event_y >= cell_area->y && event_y <= cell_area->y + cell_area->height)
            renderer =
              ctk_cell_area_get_cell_at_position (area, context, widget,
                                                  cell_area, event_x, event_y,
                                                  &alloc_area);

          if (renderer)
            {
              focus_renderer = ctk_cell_area_get_focus_from_sibling (area, renderer);
              if (!focus_renderer)
                focus_renderer = renderer;

              /* If we're already editing, cancel it and set focus */
              if (ctk_cell_area_get_edited_cell (area))
                {
                  /* XXX Was it really canceled in this case ? */
                  ctk_cell_area_stop_editing (area, TRUE);
                  ctk_cell_area_set_focus_cell (area, focus_renderer);
                  retval = TRUE;
                }
              else
                {
                  /* If we are activating via a focus sibling,
                   * we need to fetch the right cell area for the real event renderer */
                  if (focus_renderer != renderer)
                    ctk_cell_area_get_cell_allocation (area, context, widget, focus_renderer,
                                                       cell_area, &alloc_area);

                  ctk_cell_area_set_focus_cell (area, focus_renderer);
                  retval = ctk_cell_area_activate_cell (area, widget, focus_renderer,
                                                        event, &alloc_area, flags);
                }
            }
        }
    }

  return retval;
}

static gboolean
render_cell (CtkCellRenderer        *renderer,
             const CdkRectangle     *cell_area,
             const CdkRectangle     *cell_background,
             CellRenderData         *data)
{
  CtkCellRenderer      *focus_cell;
  CtkCellRendererState  flags;
  CdkRectangle          inner_area;

  focus_cell = ctk_cell_area_get_focus_cell (data->area);
  flags      = data->render_flags;

  ctk_cell_area_inner_cell_area (data->area, data->widget, cell_area, &inner_area);

  if ((flags & CTK_CELL_RENDERER_FOCUSED) &&
      (data->focus_all ||
       (focus_cell &&
        (renderer == focus_cell ||
         ctk_cell_area_is_focus_sibling (data->area, focus_cell, renderer)))))
    {
      CdkRectangle cell_focus;

      ctk_cell_renderer_get_aligned_area (renderer, data->widget, flags, &inner_area, &cell_focus);

      if (data->first_focus)
        {
          data->first_focus = FALSE;
          data->focus_rect  = cell_focus;
        }
      else
        {
          cdk_rectangle_union (&data->focus_rect, &cell_focus, &data->focus_rect);
        }
    }

  ctk_cell_renderer_render (renderer, data->cr, data->widget,
                            cell_background, &inner_area, flags);

  return FALSE;
}

static void
ctk_cell_area_real_render (CtkCellArea          *area,
                           CtkCellAreaContext   *context,
                           CtkWidget            *widget,
                           cairo_t              *cr,
                           const CdkRectangle   *background_area,
                           const CdkRectangle   *cell_area,
                           CtkCellRendererState  flags,
                           gboolean              paint_focus)
{
  CellRenderData render_data =
    {
      area,
      widget,
      cr,
      { 0, },
      flags,
      paint_focus,
      FALSE, TRUE
    };

  /* Make sure we dont paint a focus rectangle while there
   * is an editable widget in play
   */
  if (ctk_cell_area_get_edited_cell (area))
    render_data.paint_focus = FALSE;

  if (!ctk_widget_has_visible_focus (widget))
    render_data.paint_focus = FALSE;

  /* If no cell can activate but the caller wants focus painted,
   * then we paint focus around all cells */
  if ((flags & CTK_CELL_RENDERER_FOCUSED) != 0 && paint_focus &&
      !ctk_cell_area_is_activatable (area))
    render_data.focus_all = TRUE;

  ctk_cell_area_foreach_alloc (area, context, widget, cell_area, background_area,
                               (CtkCellAllocCallback)render_cell, &render_data);

  if (render_data.paint_focus &&
      render_data.focus_rect.width != 0 &&
      render_data.focus_rect.height != 0)
    {
      CtkStyleContext *style_context;
      CtkStateFlags renderer_state = 0;

      style_context = ctk_widget_get_style_context (widget);
      ctk_style_context_save (style_context);

      renderer_state = ctk_cell_renderer_get_state (NULL, widget, flags);
      ctk_style_context_set_state (style_context, renderer_state);

      cairo_save (cr);

      cdk_cairo_rectangle (cr, background_area);
      cairo_clip (cr);

      ctk_render_focus (style_context, cr,
                        render_data.focus_rect.x,     render_data.focus_rect.y,
                        render_data.focus_rect.width, render_data.focus_rect.height);

      ctk_style_context_restore (style_context);
      cairo_restore (cr);
    }
}

static void
apply_cell_attributes (CtkCellRenderer *renderer,
                       CellInfo        *info,
                       AttributeData   *data)
{
  CellAttribute *attribute;
  GSList        *list;
  GValue         value = G_VALUE_INIT;
  gboolean       is_expander;
  gboolean       is_expanded;

  g_object_freeze_notify (G_OBJECT (renderer));

  /* Whether a row expands or is presently expanded can only be
   * provided by the view (as these states can vary across views
   * accessing the same model).
   */
  g_object_get (renderer, "is-expander", &is_expander, NULL);
  if (is_expander != data->is_expander)
    g_object_set (renderer, "is-expander", data->is_expander, NULL);

  g_object_get (renderer, "is-expanded", &is_expanded, NULL);
  if (is_expanded != data->is_expanded)
    g_object_set (renderer, "is-expanded", data->is_expanded, NULL);

  /* Apply the attributes directly to the renderer */
  for (list = info->attributes; list; list = list->next)
    {
      attribute = list->data;

      ctk_tree_model_get_value (data->model, data->iter, attribute->column, &value);
      g_object_set_property (G_OBJECT (renderer), attribute->attribute, &value);
      g_value_unset (&value);
    }

  /* Call any CtkCellLayoutDataFunc that may have been set by the user
   */
  if (info->func)
    info->func (info->proxy ? info->proxy : CTK_CELL_LAYOUT (data->area), renderer,
		data->model, data->iter, info->data);

  g_object_thaw_notify (G_OBJECT (renderer));
}

static void
ctk_cell_area_real_apply_attributes (CtkCellArea           *area,
                                     CtkTreeModel          *tree_model,
                                     CtkTreeIter           *iter,
                                     gboolean               is_expander,
                                     gboolean               is_expanded)
{

  CtkCellAreaPrivate *priv;
  AttributeData       data;
  CtkTreePath        *path;

  priv = area->priv;

  /* Feed in data needed to apply to every renderer */
  data.area        = area;
  data.model       = tree_model;
  data.iter        = iter;
  data.is_expander = is_expander;
  data.is_expanded = is_expanded;

  /* Go over any cells that have attributes or custom CtkCellLayoutDataFuncs and
   * apply the data from the treemodel */
  g_hash_table_foreach (priv->cell_info, (GHFunc)apply_cell_attributes, &data);

  /* Update the currently applied path */
  g_free (priv->current_path);
  path               = ctk_tree_model_get_path (tree_model, iter);
  priv->current_path = ctk_tree_path_to_string (path);
  ctk_tree_path_free (path);
}

static CtkCellAreaContext *
ctk_cell_area_real_create_context (CtkCellArea *area)
{
  g_warning ("CtkCellAreaClass::create_context not implemented for '%s'",
             g_type_name (G_TYPE_FROM_INSTANCE (area)));

  return NULL;
}

static CtkCellAreaContext *
ctk_cell_area_real_copy_context (CtkCellArea        *area,
				 CtkCellAreaContext *context G_GNUC_UNUSED)
{
  g_warning ("CtkCellAreaClass::copy_context not implemented for '%s'",
             g_type_name (G_TYPE_FROM_INSTANCE (area)));

  return NULL;
}

static CtkSizeRequestMode
ctk_cell_area_real_get_request_mode (CtkCellArea *area G_GNUC_UNUSED)
{
  /* By default cell areas are height-for-width. */
  return CTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}

static void
ctk_cell_area_real_get_preferred_width (CtkCellArea        *area,
					CtkCellAreaContext *context G_GNUC_UNUSED,
					CtkWidget          *widget G_GNUC_UNUSED,
					gint               *minimum_width G_GNUC_UNUSED,
					gint               *natural_width G_GNUC_UNUSED)
{
  g_warning ("CtkCellAreaClass::get_preferred_width not implemented for '%s'",
	     g_type_name (G_TYPE_FROM_INSTANCE (area)));
}

static void
ctk_cell_area_real_get_preferred_height (CtkCellArea        *area,
					 CtkCellAreaContext *context G_GNUC_UNUSED,
					 CtkWidget          *widget G_GNUC_UNUSED,
					 gint               *minimum_height G_GNUC_UNUSED,
					 gint               *natural_height G_GNUC_UNUSED)
{
  g_warning ("CtkCellAreaClass::get_preferred_height not implemented for '%s'",
	     g_type_name (G_TYPE_FROM_INSTANCE (area)));
}

static void
ctk_cell_area_real_get_preferred_height_for_width (CtkCellArea        *area,
                                                   CtkCellAreaContext *context,
                                                   CtkWidget          *widget,
                                                   gint                width G_GNUC_UNUSED,
                                                   gint               *minimum_height,
                                                   gint               *natural_height)
{
  /* If the area doesnt do height-for-width, fallback on base preferred height */
  CTK_CELL_AREA_GET_CLASS (area)->get_preferred_height (area, context, widget, minimum_height, natural_height);
}

static void
ctk_cell_area_real_get_preferred_width_for_height (CtkCellArea        *area,
                                                   CtkCellAreaContext *context,
                                                   CtkWidget          *widget,
                                                   gint                height G_GNUC_UNUSED,
                                                   gint               *minimum_width,
                                                   gint               *natural_width)
{
  /* If the area doesnt do width-for-height, fallback on base preferred width */
  CTK_CELL_AREA_GET_CLASS (area)->get_preferred_width (area, context, widget, minimum_width, natural_width);
}

static gboolean
get_is_activatable (CtkCellRenderer *renderer,
                    gboolean        *activatable)
{

  if (ctk_cell_renderer_is_activatable (renderer))
    *activatable = TRUE;

  return *activatable;
}

static gboolean
ctk_cell_area_real_is_activatable (CtkCellArea *area)
{
  gboolean activatable = FALSE;

  /* Checks if any renderer can focus for the currently applied
   * attributes.
   *
   * Subclasses can override this in the case that they are also
   * rendering widgets as well as renderers.
   */
  ctk_cell_area_foreach (area, (CtkCellCallback)get_is_activatable, &activatable);

  return activatable;
}

static gboolean
ctk_cell_area_real_activate (CtkCellArea         *area,
                             CtkCellAreaContext  *context,
                             CtkWidget           *widget,
                             const CdkRectangle  *cell_area,
                             CtkCellRendererState flags,
                             gboolean             edit_only)
{
  CtkCellAreaPrivate *priv = area->priv;
  CdkRectangle        renderer_area;
  CtkCellRenderer    *activate_cell = NULL;
  CtkCellRendererMode mode;

  if (priv->focus_cell)
    {
      g_object_get (priv->focus_cell, "mode", &mode, NULL);

      if (ctk_cell_renderer_get_visible (priv->focus_cell) &&
          (edit_only ? mode == CTK_CELL_RENDERER_MODE_EDITABLE :
           mode != CTK_CELL_RENDERER_MODE_INERT))
        activate_cell = priv->focus_cell;
    }
  else
    {
      GList *cells, *l;

      /* CtkTreeView sometimes wants to activate a cell when no
       * cells are in focus.
       */
      cells = ctk_cell_layout_get_cells (CTK_CELL_LAYOUT (area));
      for (l = cells; l && !activate_cell; l = l->next)
        {
          CtkCellRenderer *renderer = l->data;

          g_object_get (renderer, "mode", &mode, NULL);

          if (ctk_cell_renderer_get_visible (renderer) &&
              (edit_only ? mode == CTK_CELL_RENDERER_MODE_EDITABLE :
               mode != CTK_CELL_RENDERER_MODE_INERT))
            activate_cell = renderer;
        }
      g_list_free (cells);
    }

  if (activate_cell)
    {
      /* Get the allocation of the focused cell.
       */
      ctk_cell_area_get_cell_allocation (area, context, widget, activate_cell,
                                         cell_area, &renderer_area);

      /* Activate or Edit the cell
       *
       * Currently just not sending an event, renderers afaics dont use
       * the event argument anyway, worst case is we can synthesize one.
       */
      if (ctk_cell_area_activate_cell (area, widget, activate_cell, NULL,
                                       &renderer_area, flags))
        return TRUE;
    }

  return FALSE;
}

static gboolean
ctk_cell_area_real_focus (CtkCellArea           *area,
			  CtkDirectionType       direction G_GNUC_UNUSED)
{
  g_warning ("CtkCellAreaClass::focus not implemented for '%s'",
             g_type_name (G_TYPE_FROM_INSTANCE (area)));
  return FALSE;
}

/*************************************************************
 *                   CtkCellLayoutIface                      *
 *************************************************************/
static void
ctk_cell_area_cell_layout_init (CtkCellLayoutIface *iface)
{
  iface->pack_start         = ctk_cell_area_pack_default;
  iface->pack_end           = ctk_cell_area_pack_default;
  iface->clear              = ctk_cell_area_clear;
  iface->add_attribute      = ctk_cell_area_add_attribute;
  iface->set_cell_data_func = ctk_cell_area_set_cell_data_func;
  iface->clear_attributes   = ctk_cell_area_clear_attributes;
  iface->reorder            = ctk_cell_area_reorder;
  iface->get_cells          = ctk_cell_area_get_cells;
  iface->get_area           = ctk_cell_area_get_area;
}

static void
ctk_cell_area_pack_default (CtkCellLayout         *cell_layout,
                            CtkCellRenderer       *renderer,
                            gboolean               expand G_GNUC_UNUSED)
{
  ctk_cell_area_add (CTK_CELL_AREA (cell_layout), renderer);
}

static void
ctk_cell_area_clear (CtkCellLayout *cell_layout)
{
  CtkCellArea *area = CTK_CELL_AREA (cell_layout);
  GList *l, *cells  =
    ctk_cell_layout_get_cells (cell_layout);

  for (l = cells; l; l = l->next)
    {
      CtkCellRenderer *renderer = l->data;
      ctk_cell_area_remove (area, renderer);
    }

  g_list_free (cells);
}

static void
ctk_cell_area_add_attribute (CtkCellLayout         *cell_layout,
                             CtkCellRenderer       *renderer,
                             const gchar           *attribute,
                             gint                   column)
{
  ctk_cell_area_attribute_connect (CTK_CELL_AREA (cell_layout),
                                   renderer, attribute, column);
}

static void
ctk_cell_area_set_cell_data_func (CtkCellLayout         *cell_layout,
                                  CtkCellRenderer       *renderer,
                                  CtkCellLayoutDataFunc  func,
                                  gpointer               func_data,
                                  GDestroyNotify         destroy)
{
  CtkCellArea *area   = CTK_CELL_AREA (cell_layout);

  _ctk_cell_area_set_cell_data_func_with_proxy (area, renderer, (GFunc)func, func_data, destroy, NULL);
}

static void
ctk_cell_area_clear_attributes (CtkCellLayout         *cell_layout,
                                CtkCellRenderer       *renderer)
{
  CtkCellArea        *area = CTK_CELL_AREA (cell_layout);
  CtkCellAreaPrivate *priv = area->priv;
  CellInfo           *info;

  info = g_hash_table_lookup (priv->cell_info, renderer);

  if (info)
    {
      g_slist_free_full (info->attributes, (GDestroyNotify)cell_attribute_free);
      info->attributes = NULL;
    }
}

static void
ctk_cell_area_reorder (CtkCellLayout   *cell_layout,
                       CtkCellRenderer *cell G_GNUC_UNUSED,
                       gint             position G_GNUC_UNUSED)
{
  g_warning ("CtkCellLayout::reorder not implemented for '%s'",
             g_type_name (G_TYPE_FROM_INSTANCE (cell_layout)));
}

static gboolean
accum_cells (CtkCellRenderer *renderer,
             GList          **accum)
{
  *accum = g_list_prepend (*accum, renderer);

  return FALSE;
}

static GList *
ctk_cell_area_get_cells (CtkCellLayout *cell_layout)
{
  GList *cells = NULL;

  ctk_cell_area_foreach (CTK_CELL_AREA (cell_layout),
                         (CtkCellCallback)accum_cells,
                         &cells);

  return g_list_reverse (cells);
}

static CtkCellArea *
ctk_cell_area_get_area (CtkCellLayout *cell_layout)
{
  return CTK_CELL_AREA (cell_layout);
}

/*************************************************************
 *                   CtkBuildableIface                       *
 *************************************************************/
static void
ctk_cell_area_buildable_init (CtkBuildableIface *iface)
{
  iface->add_child = _ctk_cell_layout_buildable_add_child;
  iface->custom_tag_start = _ctk_cell_layout_buildable_custom_tag_start;
  iface->custom_tag_end = ctk_cell_area_buildable_custom_tag_end;
}

static void
ctk_cell_area_buildable_custom_tag_end (CtkBuildable *buildable,
                                        CtkBuilder   *builder,
                                        GObject      *child,
                                        const gchar  *tagname,
                                        gpointer     *data)
{
  /* Just ignore the boolean return from here */
  _ctk_cell_layout_buildable_custom_tag_end (buildable, builder, child, tagname, data);
}

/*************************************************************
 *                            API                            *
 *************************************************************/

/**
 * ctk_cell_area_add:
 * @area: a #CtkCellArea
 * @renderer: the #CtkCellRenderer to add to @area
 *
 * Adds @renderer to @area with the default child cell properties.
 *
 * Since: 3.0
 */
void
ctk_cell_area_add (CtkCellArea        *area,
                   CtkCellRenderer    *renderer)
{
  g_return_if_fail (CTK_IS_CELL_AREA (area));
  g_return_if_fail (CTK_IS_CELL_RENDERER (renderer));

  CTK_CELL_AREA_GET_CLASS (area)->add (area, renderer);
}

/**
 * ctk_cell_area_remove:
 * @area: a #CtkCellArea
 * @renderer: the #CtkCellRenderer to remove from @area
 *
 * Removes @renderer from @area.
 *
 * Since: 3.0
 */
void
ctk_cell_area_remove (CtkCellArea        *area,
                      CtkCellRenderer    *renderer)
{
  CtkCellAreaPrivate *priv;
  GList              *renderers, *l;

  g_return_if_fail (CTK_IS_CELL_AREA (area));
  g_return_if_fail (CTK_IS_CELL_RENDERER (renderer));

  priv  = area->priv;

  /* Remove any custom attributes and custom cell data func here first */
  g_hash_table_remove (priv->cell_info, renderer);

  /* Remove focus siblings of this renderer */
  g_hash_table_remove (priv->focus_siblings, renderer);

  /* Remove this renderer from any focus renderer's sibling list */
  renderers = ctk_cell_layout_get_cells (CTK_CELL_LAYOUT (area));

  for (l = renderers; l; l = l->next)
    {
      CtkCellRenderer *focus_renderer = l->data;

      if (ctk_cell_area_is_focus_sibling (area, focus_renderer, renderer))
        {
          ctk_cell_area_remove_focus_sibling (area, focus_renderer, renderer);
          break;
        }
    }

  g_list_free (renderers);

  CTK_CELL_AREA_GET_CLASS (area)->remove (area, renderer);
}

static gboolean
get_has_renderer (CtkCellRenderer  *renderer,
                  HasRendererCheck *check)
{
  if (renderer == check->renderer)
    check->has_renderer = TRUE;

  return check->has_renderer;
}

/**
 * ctk_cell_area_has_renderer:
 * @area: a #CtkCellArea
 * @renderer: the #CtkCellRenderer to check
 *
 * Checks if @area contains @renderer.
 *
 * Returns: %TRUE if @renderer is in the @area.
 *
 * Since: 3.0
 */
gboolean
ctk_cell_area_has_renderer (CtkCellArea     *area,
                            CtkCellRenderer *renderer)
{
  HasRendererCheck check = { renderer, FALSE };

  g_return_val_if_fail (CTK_IS_CELL_AREA (area), FALSE);
  g_return_val_if_fail (CTK_IS_CELL_RENDERER (renderer), FALSE);

  ctk_cell_area_foreach (area, (CtkCellCallback)get_has_renderer, &check);

  return check.has_renderer;
}

/**
 * ctk_cell_area_foreach:
 * @area: a #CtkCellArea
 * @callback: (scope call): the #CtkCellCallback to call
 * @callback_data: user provided data pointer
 *
 * Calls @callback for every #CtkCellRenderer in @area.
 *
 * Since: 3.0
 */
void
ctk_cell_area_foreach (CtkCellArea        *area,
                       CtkCellCallback     callback,
                       gpointer            callback_data)
{
  g_return_if_fail (CTK_IS_CELL_AREA (area));
  g_return_if_fail (callback != NULL);

  CTK_CELL_AREA_GET_CLASS (area)->foreach (area, callback, callback_data);
}

/**
 * ctk_cell_area_foreach_alloc:
 * @area: a #CtkCellArea
 * @context: the #CtkCellAreaContext for this row of data.
 * @widget: the #CtkWidget that @area is rendering to
 * @cell_area: the @widget relative coordinates and size for @area
 * @background_area: the @widget relative coordinates of the background area
 * @callback: (scope call): the #CtkCellAllocCallback to call
 * @callback_data: user provided data pointer
 *
 * Calls @callback for every #CtkCellRenderer in @area with the
 * allocated rectangle inside @cell_area.
 *
 * Since: 3.0
 */
void
ctk_cell_area_foreach_alloc (CtkCellArea          *area,
                             CtkCellAreaContext   *context,
                             CtkWidget            *widget,
                             const CdkRectangle   *cell_area,
                             const CdkRectangle   *background_area,
                             CtkCellAllocCallback  callback,
                             gpointer              callback_data)
{
  g_return_if_fail (CTK_IS_CELL_AREA (area));
  g_return_if_fail (CTK_IS_CELL_AREA_CONTEXT (context));
  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (cell_area != NULL);
  g_return_if_fail (callback != NULL);

  CTK_CELL_AREA_GET_CLASS (area)->foreach_alloc (area, context, widget, 
						 cell_area, background_area, 
						 callback, callback_data);
}

/**
 * ctk_cell_area_event:
 * @area: a #CtkCellArea
 * @context: the #CtkCellAreaContext for this row of data.
 * @widget: the #CtkWidget that @area is rendering to
 * @event: the #CdkEvent to handle
 * @cell_area: the @widget relative coordinates for @area
 * @flags: the #CtkCellRendererState for @area in this row.
 *
 * Delegates event handling to a #CtkCellArea.
 *
 * Returns: %TRUE if the event was handled by @area.
 *
 * Since: 3.0
 */
gint
ctk_cell_area_event (CtkCellArea          *area,
                     CtkCellAreaContext   *context,
                     CtkWidget            *widget,
                     CdkEvent             *event,
                     const CdkRectangle   *cell_area,
                     CtkCellRendererState  flags)
{
  CtkCellAreaClass *class;

  g_return_val_if_fail (CTK_IS_CELL_AREA (area), 0);
  g_return_val_if_fail (CTK_IS_CELL_AREA_CONTEXT (context), 0);
  g_return_val_if_fail (CTK_IS_WIDGET (widget), 0);
  g_return_val_if_fail (event != NULL, 0);
  g_return_val_if_fail (cell_area != NULL, 0);

  class = CTK_CELL_AREA_GET_CLASS (area);

  if (class->event)
    return class->event (area, context, widget, event, cell_area, flags);

  g_warning ("CtkCellAreaClass::event not implemented for '%s'",
             g_type_name (G_TYPE_FROM_INSTANCE (area)));
  return 0;
}

/**
 * ctk_cell_area_render:
 * @area: a #CtkCellArea
 * @context: the #CtkCellAreaContext for this row of data.
 * @widget: the #CtkWidget that @area is rendering to
 * @cr: the #cairo_t to render with
 * @background_area: the @widget relative coordinates for @area’s background
 * @cell_area: the @widget relative coordinates for @area
 * @flags: the #CtkCellRendererState for @area in this row.
 * @paint_focus: whether @area should paint focus on focused cells for focused rows or not.
 *
 * Renders @area’s cells according to @area’s layout onto @widget at
 * the given coordinates.
 *
 * Since: 3.0
 */
void
ctk_cell_area_render (CtkCellArea          *area,
                      CtkCellAreaContext   *context,
                      CtkWidget            *widget,
                      cairo_t              *cr,
                      const CdkRectangle   *background_area,
                      const CdkRectangle   *cell_area,
                      CtkCellRendererState  flags,
                      gboolean              paint_focus)
{
  CtkCellAreaClass *class;

  g_return_if_fail (CTK_IS_CELL_AREA (area));
  g_return_if_fail (CTK_IS_CELL_AREA_CONTEXT (context));
  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (background_area != NULL);
  g_return_if_fail (cell_area != NULL);

  class = CTK_CELL_AREA_GET_CLASS (area);

  if (class->render)
    class->render (area, context, widget, cr, background_area, cell_area, flags, paint_focus);
  else
    g_warning ("CtkCellAreaClass::render not implemented for '%s'",
               g_type_name (G_TYPE_FROM_INSTANCE (area)));
}

static gboolean
get_cell_allocation (CtkCellRenderer        *renderer,
                     const CdkRectangle     *cell_area,
                     const CdkRectangle     *cell_background G_GNUC_UNUSED,
                     RendererAllocationData *data)
{
  if (data->renderer == renderer)
    data->allocation = *cell_area;

  return (data->renderer == renderer);
}

/**
 * ctk_cell_area_get_cell_allocation:
 * @area: a #CtkCellArea
 * @context: the #CtkCellAreaContext used to hold sizes for @area.
 * @widget: the #CtkWidget that @area is rendering on
 * @renderer: the #CtkCellRenderer to get the allocation for
 * @cell_area: the whole allocated area for @area in @widget
 *             for this row
 * @allocation: (out): where to store the allocation for @renderer
 *
 * Derives the allocation of @renderer inside @area if @area
 * were to be renderered in @cell_area.
 *
 * Since: 3.0
 */
void
ctk_cell_area_get_cell_allocation (CtkCellArea          *area,
                                   CtkCellAreaContext   *context,
                                   CtkWidget            *widget,
                                   CtkCellRenderer      *renderer,
                                   const CdkRectangle   *cell_area,
                                   CdkRectangle         *allocation)
{
  RendererAllocationData data = { renderer, { 0, } };

  g_return_if_fail (CTK_IS_CELL_AREA (area));
  g_return_if_fail (CTK_IS_CELL_AREA_CONTEXT (context));
  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (CTK_IS_CELL_RENDERER (renderer));
  g_return_if_fail (cell_area != NULL);
  g_return_if_fail (allocation != NULL);

  ctk_cell_area_foreach_alloc (area, context, widget, cell_area, cell_area,
                               (CtkCellAllocCallback)get_cell_allocation, &data);

  *allocation = data.allocation;
}

static gboolean
get_cell_by_position (CtkCellRenderer     *renderer,
                      const CdkRectangle  *cell_area,
                      const CdkRectangle  *cell_background G_GNUC_UNUSED,
                      CellByPositionData  *data)
{
  if (data->x >= cell_area->x && data->x < cell_area->x + cell_area->width &&
      data->y >= cell_area->y && data->y < cell_area->y + cell_area->height)
    {
      data->renderer  = renderer;
      data->cell_area = *cell_area;
    }

  return (data->renderer != NULL);
}

/**
 * ctk_cell_area_get_cell_at_position:
 * @area: a #CtkCellArea
 * @context: the #CtkCellAreaContext used to hold sizes for @area.
 * @widget: the #CtkWidget that @area is rendering on
 * @cell_area: the whole allocated area for @area in @widget
 *             for this row
 * @x: the x position
 * @y: the y position
 * @alloc_area: (out) (allow-none): where to store the inner allocated area of the
 *                                  returned cell renderer, or %NULL.
 *
 * Gets the #CtkCellRenderer at @x and @y coordinates inside @area and optionally
 * returns the full cell allocation for it inside @cell_area.
 *
 * Returns: (transfer none): the #CtkCellRenderer at @x and @y.
 *
 * Since: 3.0
 */
CtkCellRenderer *
ctk_cell_area_get_cell_at_position (CtkCellArea          *area,
                                    CtkCellAreaContext   *context,
                                    CtkWidget            *widget,
                                    const CdkRectangle   *cell_area,
                                    gint                  x,
                                    gint                  y,
                                    CdkRectangle         *alloc_area)
{
  CellByPositionData data = { x, y, NULL, { 0, } };

  g_return_val_if_fail (CTK_IS_CELL_AREA (area), NULL);
  g_return_val_if_fail (CTK_IS_CELL_AREA_CONTEXT (context), NULL);
  g_return_val_if_fail (CTK_IS_WIDGET (widget), NULL);
  g_return_val_if_fail (cell_area != NULL, NULL);
  g_return_val_if_fail (x >= cell_area->x && x <= cell_area->x + cell_area->width, NULL);
  g_return_val_if_fail (y >= cell_area->y && y <= cell_area->y + cell_area->height, NULL);

  ctk_cell_area_foreach_alloc (area, context, widget, cell_area, cell_area,
                               (CtkCellAllocCallback)get_cell_by_position, &data);

  if (alloc_area)
    *alloc_area = data.cell_area;

  return data.renderer;
}

/*************************************************************
 *                      API: Geometry                        *
 *************************************************************/
/**
 * ctk_cell_area_create_context:
 * @area: a #CtkCellArea
 *
 * Creates a #CtkCellAreaContext to be used with @area for
 * all purposes. #CtkCellAreaContext stores geometry information
 * for rows for which it was operated on, it is important to use
 * the same context for the same row of data at all times (i.e.
 * one should render and handle events with the same #CtkCellAreaContext
 * which was used to request the size of those rows of data).
 *
 * Returns: (transfer full): a newly created #CtkCellAreaContext which can be used with @area.
 *
 * Since: 3.0
 */
CtkCellAreaContext *
ctk_cell_area_create_context (CtkCellArea *area)
{
  g_return_val_if_fail (CTK_IS_CELL_AREA (area), NULL);

  return CTK_CELL_AREA_GET_CLASS (area)->create_context (area);
}

/**
 * ctk_cell_area_copy_context:
 * @area: a #CtkCellArea
 * @context: the #CtkCellAreaContext to copy
 *
 * This is sometimes needed for cases where rows need to share
 * alignments in one orientation but may be separately grouped
 * in the opposing orientation.
 *
 * For instance, #CtkIconView creates all icons (rows) to have
 * the same width and the cells theirin to have the same
 * horizontal alignments. However each row of icons may have
 * a separate collective height. #CtkIconView uses this to
 * request the heights of each row based on a context which
 * was already used to request all the row widths that are
 * to be displayed.
 *
 * Returns: (transfer full): a newly created #CtkCellAreaContext copy of @context.
 *
 * Since: 3.0
 */
CtkCellAreaContext *
ctk_cell_area_copy_context (CtkCellArea        *area,
                            CtkCellAreaContext *context)
{
  g_return_val_if_fail (CTK_IS_CELL_AREA (area), NULL);
  g_return_val_if_fail (CTK_IS_CELL_AREA_CONTEXT (context), NULL);

  return CTK_CELL_AREA_GET_CLASS (area)->copy_context (area, context);
}

/**
 * ctk_cell_area_get_request_mode:
 * @area: a #CtkCellArea
 *
 * Gets whether the area prefers a height-for-width layout
 * or a width-for-height layout.
 *
 * Returns: The #CtkSizeRequestMode preferred by @area.
 *
 * Since: 3.0
 */
CtkSizeRequestMode
ctk_cell_area_get_request_mode (CtkCellArea *area)
{
  g_return_val_if_fail (CTK_IS_CELL_AREA (area),
                        CTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH);

  return CTK_CELL_AREA_GET_CLASS (area)->get_request_mode (area);
}

/**
 * ctk_cell_area_get_preferred_width:
 * @area: a #CtkCellArea
 * @context: the #CtkCellAreaContext to perform this request with
 * @widget: the #CtkWidget where @area will be rendering
 * @minimum_width: (out) (allow-none): location to store the minimum width, or %NULL
 * @natural_width: (out) (allow-none): location to store the natural width, or %NULL
 *
 * Retrieves a cell area’s initial minimum and natural width.
 *
 * @area will store some geometrical information in @context along the way;
 * when requesting sizes over an arbitrary number of rows, it’s not important
 * to check the @minimum_width and @natural_width of this call but rather to
 * consult ctk_cell_area_context_get_preferred_width() after a series of
 * requests.
 *
 * Since: 3.0
 */
void
ctk_cell_area_get_preferred_width (CtkCellArea        *area,
                                   CtkCellAreaContext *context,
                                   CtkWidget          *widget,
                                   gint               *minimum_width,
                                   gint               *natural_width)
{
  g_return_if_fail (CTK_IS_CELL_AREA (area));
  g_return_if_fail (CTK_IS_WIDGET (widget));

  CTK_CELL_AREA_GET_CLASS (area)->get_preferred_width (area, context, widget, 
						       minimum_width, natural_width);
}

/**
 * ctk_cell_area_get_preferred_height_for_width:
 * @area: a #CtkCellArea
 * @context: the #CtkCellAreaContext which has already been requested for widths.
 * @widget: the #CtkWidget where @area will be rendering
 * @width: the width for which to check the height of this area
 * @minimum_height: (out) (allow-none): location to store the minimum height, or %NULL
 * @natural_height: (out) (allow-none): location to store the natural height, or %NULL
 *
 * Retrieves a cell area’s minimum and natural height if it would be given
 * the specified @width.
 *
 * @area stores some geometrical information in @context along the way
 * while calling ctk_cell_area_get_preferred_width(). It’s important to
 * perform a series of ctk_cell_area_get_preferred_width() requests with
 * @context first and then call ctk_cell_area_get_preferred_height_for_width()
 * on each cell area individually to get the height for width of each
 * fully requested row.
 *
 * If at some point, the width of a single row changes, it should be
 * requested with ctk_cell_area_get_preferred_width() again and then
 * the full width of the requested rows checked again with
 * ctk_cell_area_context_get_preferred_width().
 *
 * Since: 3.0
 */
void
ctk_cell_area_get_preferred_height_for_width (CtkCellArea        *area,
                                              CtkCellAreaContext *context,
                                              CtkWidget          *widget,
                                              gint                width,
                                              gint               *minimum_height,
                                              gint               *natural_height)
{
  CtkCellAreaClass *class;

  g_return_if_fail (CTK_IS_CELL_AREA (area));
  g_return_if_fail (CTK_IS_WIDGET (widget));

  class = CTK_CELL_AREA_GET_CLASS (area);
  class->get_preferred_height_for_width (area, context, widget, width, minimum_height, natural_height);
}


/**
 * ctk_cell_area_get_preferred_height:
 * @area: a #CtkCellArea
 * @context: the #CtkCellAreaContext to perform this request with
 * @widget: the #CtkWidget where @area will be rendering
 * @minimum_height: (out) (allow-none): location to store the minimum height, or %NULL
 * @natural_height: (out) (allow-none): location to store the natural height, or %NULL
 *
 * Retrieves a cell area’s initial minimum and natural height.
 *
 * @area will store some geometrical information in @context along the way;
 * when requesting sizes over an arbitrary number of rows, it’s not important
 * to check the @minimum_height and @natural_height of this call but rather to
 * consult ctk_cell_area_context_get_preferred_height() after a series of
 * requests.
 *
 * Since: 3.0
 */
void
ctk_cell_area_get_preferred_height (CtkCellArea        *area,
                                    CtkCellAreaContext *context,
                                    CtkWidget          *widget,
                                    gint               *minimum_height,
                                    gint               *natural_height)
{
  g_return_if_fail (CTK_IS_CELL_AREA (area));
  g_return_if_fail (CTK_IS_WIDGET (widget));

  CTK_CELL_AREA_GET_CLASS (area)->get_preferred_height (area, context, widget, 
							minimum_height, natural_height);
}

/**
 * ctk_cell_area_get_preferred_width_for_height:
 * @area: a #CtkCellArea
 * @context: the #CtkCellAreaContext which has already been requested for widths.
 * @widget: the #CtkWidget where @area will be rendering
 * @height: the height for which to check the width of this area
 * @minimum_width: (out) (allow-none): location to store the minimum width, or %NULL
 * @natural_width: (out) (allow-none): location to store the natural width, or %NULL
 *
 * Retrieves a cell area’s minimum and natural width if it would be given
 * the specified @height.
 *
 * @area stores some geometrical information in @context along the way
 * while calling ctk_cell_area_get_preferred_height(). It’s important to
 * perform a series of ctk_cell_area_get_preferred_height() requests with
 * @context first and then call ctk_cell_area_get_preferred_width_for_height()
 * on each cell area individually to get the height for width of each
 * fully requested row.
 *
 * If at some point, the height of a single row changes, it should be
 * requested with ctk_cell_area_get_preferred_height() again and then
 * the full height of the requested rows checked again with
 * ctk_cell_area_context_get_preferred_height().
 *
 * Since: 3.0
 */
void
ctk_cell_area_get_preferred_width_for_height (CtkCellArea        *area,
                                              CtkCellAreaContext *context,
                                              CtkWidget          *widget,
                                              gint                height,
                                              gint               *minimum_width,
                                              gint               *natural_width)
{
  CtkCellAreaClass *class;

  g_return_if_fail (CTK_IS_CELL_AREA (area));
  g_return_if_fail (CTK_IS_WIDGET (widget));

  class = CTK_CELL_AREA_GET_CLASS (area);
  class->get_preferred_width_for_height (area, context, widget, height, minimum_width, natural_width);
}

/*************************************************************
 *                      API: Attributes                      *
 *************************************************************/

/**
 * ctk_cell_area_attribute_connect:
 * @area: a #CtkCellArea
 * @renderer: the #CtkCellRenderer to connect an attribute for
 * @attribute: the attribute name
 * @column: the #CtkTreeModel column to fetch attribute values from
 *
 * Connects an @attribute to apply values from @column for the
 * #CtkTreeModel in use.
 *
 * Since: 3.0
 */
void
ctk_cell_area_attribute_connect (CtkCellArea        *area,
                                 CtkCellRenderer    *renderer,
                                 const gchar        *attribute,
                                 gint                column)
{
  CtkCellAreaPrivate *priv;
  CellInfo           *info;
  CellAttribute      *cell_attribute;

  g_return_if_fail (CTK_IS_CELL_AREA (area));
  g_return_if_fail (CTK_IS_CELL_RENDERER (renderer));
  g_return_if_fail (attribute != NULL);
  g_return_if_fail (ctk_cell_area_has_renderer (area, renderer));

  priv = area->priv;
  info = g_hash_table_lookup (priv->cell_info, renderer);

  if (!info)
    {
      info = cell_info_new (NULL, NULL, NULL);

      g_hash_table_insert (priv->cell_info, renderer, info);
    }
  else
    {
      GSList *node;

      /* Check we are not adding the same attribute twice */
      if ((node = g_slist_find_custom (info->attributes, attribute,
                                       (GCompareFunc)cell_attribute_find)) != NULL)
        {
          cell_attribute = node->data;

          g_warning ("Cannot connect attribute '%s' for cell renderer class '%s' "
                     "since '%s' is already attributed to column %d",
                     attribute,
                     G_OBJECT_TYPE_NAME (renderer),
                     attribute, cell_attribute->column);
          return;
        }
    }

  cell_attribute = cell_attribute_new (renderer, attribute, column);

  if (!cell_attribute)
    {
      g_warning ("Cannot connect attribute '%s' for cell renderer class '%s' "
                 "since attribute does not exist",
                 attribute,
                 G_OBJECT_TYPE_NAME (renderer));
      return;
    }

  info->attributes = g_slist_prepend (info->attributes, cell_attribute);
}

/**
 * ctk_cell_area_attribute_disconnect:
 * @area: a #CtkCellArea
 * @renderer: the #CtkCellRenderer to disconnect an attribute for
 * @attribute: the attribute name
 *
 * Disconnects @attribute for the @renderer in @area so that
 * attribute will no longer be updated with values from the
 * model.
 *
 * Since: 3.0
 */
void
ctk_cell_area_attribute_disconnect (CtkCellArea        *area,
                                    CtkCellRenderer    *renderer,
                                    const gchar        *attribute)
{
  CtkCellAreaPrivate *priv;
  CellInfo           *info;
  CellAttribute      *cell_attribute;
  GSList             *node;

  g_return_if_fail (CTK_IS_CELL_AREA (area));
  g_return_if_fail (CTK_IS_CELL_RENDERER (renderer));
  g_return_if_fail (attribute != NULL);
  g_return_if_fail (ctk_cell_area_has_renderer (area, renderer));

  priv = area->priv;
  info = g_hash_table_lookup (priv->cell_info, renderer);

  if (info)
    {
      node = g_slist_find_custom (info->attributes, attribute,
                                  (GCompareFunc)cell_attribute_find);
      if (node)
        {
          cell_attribute = node->data;

          cell_attribute_free (cell_attribute);

          info->attributes = g_slist_delete_link (info->attributes, node);
        }
    }
}

/**
 * ctk_cell_area_attribute_get_column:
 * @area: a #CtkCellArea
 * @renderer: a #CtkCellRenderer
 * @attribute: an attribute on the renderer
 *
 * Returns the model column that an attribute has been mapped to,
 * or -1 if the attribute is not mapped.
 *
 * Returns: the model column, or -1
 *
 * Since: 3.14
 */
gint
ctk_cell_area_attribute_get_column (CtkCellArea     *area,
                                    CtkCellRenderer *renderer,
                                    const gchar     *attribute)
{
  CtkCellAreaPrivate *priv;
  CellInfo           *info;
  CellAttribute      *cell_attribute;
  GSList             *node;

  priv = area->priv;
  info = g_hash_table_lookup (priv->cell_info, renderer);

  if (info)
    {
      node = g_slist_find_custom (info->attributes, attribute,
                                  (GCompareFunc)cell_attribute_find);
      if (node)
        {
          cell_attribute = node->data;
          return cell_attribute->column;
        }
    }

  return -1;
}

/**
 * ctk_cell_area_apply_attributes:
 * @area: a #CtkCellArea
 * @tree_model: the #CtkTreeModel to pull values from
 * @iter: the #CtkTreeIter in @tree_model to apply values for
 * @is_expander: whether @iter has children
 * @is_expanded: whether @iter is expanded in the view and
 *               children are visible
 *
 * Applies any connected attributes to the renderers in
 * @area by pulling the values from @tree_model.
 *
 * Since: 3.0
 */
void
ctk_cell_area_apply_attributes (CtkCellArea  *area,
                                CtkTreeModel *tree_model,
                                CtkTreeIter  *iter,
                                gboolean      is_expander,
                                gboolean      is_expanded)
{
  g_return_if_fail (CTK_IS_CELL_AREA (area));
  g_return_if_fail (CTK_IS_TREE_MODEL (tree_model));
  g_return_if_fail (iter != NULL);

  g_signal_emit (area, cell_area_signals[SIGNAL_APPLY_ATTRIBUTES], 0,
                 tree_model, iter, is_expander, is_expanded);
}

/**
 * ctk_cell_area_get_current_path_string:
 * @area: a #CtkCellArea
 *
 * Gets the current #CtkTreePath string for the currently
 * applied #CtkTreeIter, this is implicitly updated when
 * ctk_cell_area_apply_attributes() is called and can be
 * used to interact with renderers from #CtkCellArea
 * subclasses.
 *
 * Returns: The current #CtkTreePath string for the current
 * attributes applied to @area. This string belongs to the area and
 * should not be freed.
 *
 * Since: 3.0
 */
const gchar *
ctk_cell_area_get_current_path_string (CtkCellArea *area)
{
  CtkCellAreaPrivate *priv;

  g_return_val_if_fail (CTK_IS_CELL_AREA (area), NULL);

  priv = area->priv;

  return priv->current_path;
}


/*************************************************************
 *                    API: Cell Properties                   *
 *************************************************************/
/**
 * ctk_cell_area_class_install_cell_property:
 * @aclass: a #CtkCellAreaClass
 * @property_id: the id for the property
 * @pspec: the #GParamSpec for the property
 *
 * Installs a cell property on a cell area class.
 *
 * Since: 3.0
 */
void
ctk_cell_area_class_install_cell_property (CtkCellAreaClass   *aclass,
                                           guint               property_id,
                                           GParamSpec         *pspec)
{
  g_return_if_fail (CTK_IS_CELL_AREA_CLASS (aclass));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  if (pspec->flags & G_PARAM_WRITABLE)
    g_return_if_fail (aclass->set_cell_property != NULL);
  if (pspec->flags & G_PARAM_READABLE)
    g_return_if_fail (aclass->get_cell_property != NULL);
  g_return_if_fail (property_id > 0);
  g_return_if_fail (PARAM_SPEC_PARAM_ID (pspec) == 0);  /* paranoid */
  g_return_if_fail ((pspec->flags & (G_PARAM_CONSTRUCT | G_PARAM_CONSTRUCT_ONLY)) == 0);

  if (g_param_spec_pool_lookup (cell_property_pool, pspec->name, G_OBJECT_CLASS_TYPE (aclass), TRUE))
    {
      g_warning (G_STRLOC ": class '%s' already contains a cell property named '%s'",
                 G_OBJECT_CLASS_NAME (aclass), pspec->name);
      return;
    }
  g_param_spec_ref (pspec);
  g_param_spec_sink (pspec);
  PARAM_SPEC_SET_PARAM_ID (pspec, property_id);
  g_param_spec_pool_insert (cell_property_pool, pspec, G_OBJECT_CLASS_TYPE (aclass));
}

/**
 * ctk_cell_area_class_find_cell_property:
 * @aclass: a #CtkCellAreaClass
 * @property_name: the name of the child property to find
 *
 * Finds a cell property of a cell area class by name.
 *
 * Returns: (transfer none): the #GParamSpec of the child property
 *   or %NULL if @aclass has no child property with that name.
 *
 * Since: 3.0
 */
GParamSpec*
ctk_cell_area_class_find_cell_property (CtkCellAreaClass   *aclass,
                                        const gchar        *property_name)
{
  g_return_val_if_fail (CTK_IS_CELL_AREA_CLASS (aclass), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  return g_param_spec_pool_lookup (cell_property_pool,
                                   property_name,
                                   G_OBJECT_CLASS_TYPE (aclass),
                                   TRUE);
}

/**
 * ctk_cell_area_class_list_cell_properties:
 * @aclass: a #CtkCellAreaClass
 * @n_properties: (out): location to return the number of cell properties found
 *
 * Returns all cell properties of a cell area class.
 *
 * Returns: (array length=n_properties) (transfer container): a newly
 *     allocated %NULL-terminated array of #GParamSpec*.  The array
 *     must be freed with g_free().
 *
 * Since: 3.0
 */
GParamSpec**
ctk_cell_area_class_list_cell_properties (CtkCellAreaClass  *aclass,
                                          guint             *n_properties)
{
  GParamSpec **pspecs;
  guint n;

  g_return_val_if_fail (CTK_IS_CELL_AREA_CLASS (aclass), NULL);

  pspecs = g_param_spec_pool_list (cell_property_pool,
                                   G_OBJECT_CLASS_TYPE (aclass),
                                   &n);
  if (n_properties)
    *n_properties = n;

  return pspecs;
}

/**
 * ctk_cell_area_add_with_properties:
 * @area: a #CtkCellArea
 * @renderer: a #CtkCellRenderer to be placed inside @area
 * @first_prop_name: the name of the first cell property to set
 * @...: a %NULL-terminated list of property names and values, starting
 *     with @first_prop_name
 *
 * Adds @renderer to @area, setting cell properties at the same time.
 * See ctk_cell_area_add() and ctk_cell_area_cell_set() for more details.
 *
 * Since: 3.0
 */
void
ctk_cell_area_add_with_properties (CtkCellArea        *area,
                                   CtkCellRenderer    *renderer,
                                   const gchar        *first_prop_name,
                                   ...)
{
  CtkCellAreaClass *class;

  g_return_if_fail (CTK_IS_CELL_AREA (area));
  g_return_if_fail (CTK_IS_CELL_RENDERER (renderer));

  class = CTK_CELL_AREA_GET_CLASS (area);

  if (class->add)
    {
      va_list var_args;

      class->add (area, renderer);

      va_start (var_args, first_prop_name);
      ctk_cell_area_cell_set_valist (area, renderer, first_prop_name, var_args);
      va_end (var_args);
    }
  else
    g_warning ("CtkCellAreaClass::add not implemented for '%s'",
               g_type_name (G_TYPE_FROM_INSTANCE (area)));
}

/**
 * ctk_cell_area_cell_set:
 * @area: a #CtkCellArea
 * @renderer: a #CtkCellRenderer which is a cell inside @area
 * @first_prop_name: the name of the first cell property to set
 * @...: a %NULL-terminated list of property names and values, starting
 *           with @first_prop_name
 *
 * Sets one or more cell properties for @cell in @area.
 *
 * Since: 3.0
 */
void
ctk_cell_area_cell_set (CtkCellArea        *area,
                        CtkCellRenderer    *renderer,
                        const gchar        *first_prop_name,
                        ...)
{
  va_list var_args;

  g_return_if_fail (CTK_IS_CELL_AREA (area));
  g_return_if_fail (CTK_IS_CELL_RENDERER (renderer));

  va_start (var_args, first_prop_name);
  ctk_cell_area_cell_set_valist (area, renderer, first_prop_name, var_args);
  va_end (var_args);
}

/**
 * ctk_cell_area_cell_get:
 * @area: a #CtkCellArea
 * @renderer: a #CtkCellRenderer which is inside @area
 * @first_prop_name: the name of the first cell property to get
 * @...: return location for the first cell property, followed
 *     optionally by more name/return location pairs, followed by %NULL
 *
 * Gets the values of one or more cell properties for @renderer in @area.
 *
 * Since: 3.0
 */
void
ctk_cell_area_cell_get (CtkCellArea        *area,
                        CtkCellRenderer    *renderer,
                        const gchar        *first_prop_name,
                        ...)
{
  va_list var_args;

  g_return_if_fail (CTK_IS_CELL_AREA (area));
  g_return_if_fail (CTK_IS_CELL_RENDERER (renderer));

  va_start (var_args, first_prop_name);
  ctk_cell_area_cell_get_valist (area, renderer, first_prop_name, var_args);
  va_end (var_args);
}

static inline void
area_get_cell_property (CtkCellArea     *area,
                        CtkCellRenderer *renderer,
                        GParamSpec      *pspec,
                        GValue          *value)
{
  CtkCellAreaClass *class = g_type_class_peek (pspec->owner_type);

  class->get_cell_property (area, renderer, PARAM_SPEC_PARAM_ID (pspec), value, pspec);
}

static inline void
area_set_cell_property (CtkCellArea     *area,
                        CtkCellRenderer *renderer,
                        GParamSpec      *pspec,
                        const GValue    *value)
{
  GValue tmp_value = G_VALUE_INIT;
  CtkCellAreaClass *class = g_type_class_peek (pspec->owner_type);

  /* provide a copy to work from, convert (if necessary) and validate */
  g_value_init (&tmp_value, G_PARAM_SPEC_VALUE_TYPE (pspec));
  if (!g_value_transform (value, &tmp_value))
    g_warning ("unable to set cell property '%s' of type '%s' from value of type '%s'",
               pspec->name,
               g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)),
               G_VALUE_TYPE_NAME (value));
  else if (g_param_value_validate (pspec, &tmp_value) && !(pspec->flags & G_PARAM_LAX_VALIDATION))
    {
      gchar *contents = g_strdup_value_contents (value);

      g_warning ("value \"%s\" of type '%s' is invalid for property '%s' of type '%s'",
                 contents,
                 G_VALUE_TYPE_NAME (value),
                 pspec->name,
                 g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)));
      g_free (contents);
    }
  else
    {
      class->set_cell_property (area, renderer, PARAM_SPEC_PARAM_ID (pspec), &tmp_value, pspec);
    }
  g_value_unset (&tmp_value);
}

/**
 * ctk_cell_area_cell_set_valist:
 * @area: a #CtkCellArea
 * @renderer: a #CtkCellRenderer which inside @area
 * @first_property_name: the name of the first cell property to set
 * @var_args: a %NULL-terminated list of property names and values, starting
 *           with @first_prop_name
 *
 * Sets one or more cell properties for @renderer in @area.
 *
 * Since: 3.0
 */
void
ctk_cell_area_cell_set_valist (CtkCellArea        *area,
                               CtkCellRenderer    *renderer,
                               const gchar        *first_property_name,
                               va_list             var_args)
{
  const gchar *name;

  g_return_if_fail (CTK_IS_CELL_AREA (area));
  g_return_if_fail (CTK_IS_CELL_RENDERER (renderer));

  name = first_property_name;
  while (name)
    {
      GValue value = G_VALUE_INIT;
      gchar *error = NULL;
      GParamSpec *pspec =
        g_param_spec_pool_lookup (cell_property_pool, name,
                                  G_OBJECT_TYPE (area), TRUE);
      if (!pspec)
        {
          g_warning ("%s: cell area class '%s' has no cell property named '%s'",
                     G_STRLOC, G_OBJECT_TYPE_NAME (area), name);
          break;
        }
      if (!(pspec->flags & G_PARAM_WRITABLE))
        {
          g_warning ("%s: cell property '%s' of cell area class '%s' is not writable",
                     G_STRLOC, pspec->name, G_OBJECT_TYPE_NAME (area));
          break;
        }

      G_VALUE_COLLECT_INIT (&value, G_PARAM_SPEC_VALUE_TYPE (pspec),
                            var_args, 0, &error);
      if (error)
        {
          g_warning ("%s: %s", G_STRLOC, error);
          g_free (error);

          /* we purposely leak the value here, it might not be
           * in a sane state if an error condition occoured
           */
          break;
        }
      area_set_cell_property (area, renderer, pspec, &value);
      g_value_unset (&value);
      name = va_arg (var_args, gchar*);
    }
}

/**
 * ctk_cell_area_cell_get_valist:
 * @area: a #CtkCellArea
 * @renderer: a #CtkCellRenderer inside @area
 * @first_property_name: the name of the first property to get
 * @var_args: return location for the first property, followed
 *     optionally by more name/return location pairs, followed by %NULL
 *
 * Gets the values of one or more cell properties for @renderer in @area.
 *
 * Since: 3.0
 */
void
ctk_cell_area_cell_get_valist (CtkCellArea        *area,
                               CtkCellRenderer    *renderer,
                               const gchar        *first_property_name,
                               va_list             var_args)
{
  const gchar *name;

  g_return_if_fail (CTK_IS_CELL_AREA (area));
  g_return_if_fail (CTK_IS_CELL_RENDERER (renderer));

  name = first_property_name;
  while (name)
    {
      GValue value = G_VALUE_INIT;
      GParamSpec *pspec;
      gchar *error;

      pspec = g_param_spec_pool_lookup (cell_property_pool, name,
                                        G_OBJECT_TYPE (area), TRUE);
      if (!pspec)
        {
          g_warning ("%s: cell area class '%s' has no cell property named '%s'",
                     G_STRLOC, G_OBJECT_TYPE_NAME (area), name);
          break;
        }
      if (!(pspec->flags & G_PARAM_READABLE))
        {
          g_warning ("%s: cell property '%s' of cell area class '%s' is not readable",
                     G_STRLOC, pspec->name, G_OBJECT_TYPE_NAME (area));
          break;
        }

      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      area_get_cell_property (area, renderer, pspec, &value);
      G_VALUE_LCOPY (&value, var_args, 0, &error);
      if (error)
        {
          g_warning ("%s: %s", G_STRLOC, error);
          g_free (error);
          g_value_unset (&value);
          break;
        }
      g_value_unset (&value);
      name = va_arg (var_args, gchar*);
    }
}

/**
 * ctk_cell_area_cell_set_property:
 * @area: a #CtkCellArea
 * @renderer: a #CtkCellRenderer inside @area
 * @property_name: the name of the cell property to set
 * @value: the value to set the cell property to
 *
 * Sets a cell property for @renderer in @area.
 *
 * Since: 3.0
 */
void
ctk_cell_area_cell_set_property (CtkCellArea        *area,
                                 CtkCellRenderer    *renderer,
                                 const gchar        *property_name,
                                 const GValue       *value)
{
  GParamSpec *pspec;

  g_return_if_fail (CTK_IS_CELL_AREA (area));
  g_return_if_fail (CTK_IS_CELL_RENDERER (renderer));
  g_return_if_fail (property_name != NULL);
  g_return_if_fail (G_IS_VALUE (value));

  pspec = g_param_spec_pool_lookup (cell_property_pool, property_name,
                                    G_OBJECT_TYPE (area), TRUE);
  if (!pspec)
    g_warning ("%s: cell area class '%s' has no cell property named '%s'",
               G_STRLOC, G_OBJECT_TYPE_NAME (area), property_name);
  else if (!(pspec->flags & G_PARAM_WRITABLE))
    g_warning ("%s: cell property '%s' of cell area class '%s' is not writable",
               G_STRLOC, pspec->name, G_OBJECT_TYPE_NAME (area));
  else
    {
      area_set_cell_property (area, renderer, pspec, value);
    }
}

/**
 * ctk_cell_area_cell_get_property:
 * @area: a #CtkCellArea
 * @renderer: a #CtkCellRenderer inside @area
 * @property_name: the name of the property to get
 * @value: a location to return the value
 *
 * Gets the value of a cell property for @renderer in @area.
 *
 * Since: 3.0
 */
void
ctk_cell_area_cell_get_property (CtkCellArea        *area,
                                 CtkCellRenderer    *renderer,
                                 const gchar        *property_name,
                                 GValue             *value)
{
  GParamSpec *pspec;

  g_return_if_fail (CTK_IS_CELL_AREA (area));
  g_return_if_fail (CTK_IS_CELL_RENDERER (renderer));
  g_return_if_fail (property_name != NULL);
  g_return_if_fail (G_IS_VALUE (value));

  pspec = g_param_spec_pool_lookup (cell_property_pool, property_name,
                                    G_OBJECT_TYPE (area), TRUE);
  if (!pspec)
    g_warning ("%s: cell area class '%s' has no cell property named '%s'",
               G_STRLOC, G_OBJECT_TYPE_NAME (area), property_name);
  else if (!(pspec->flags & G_PARAM_READABLE))
    g_warning ("%s: cell property '%s' of cell area class '%s' is not readable",
               G_STRLOC, pspec->name, G_OBJECT_TYPE_NAME (area));
  else
    {
      GValue *prop_value, tmp_value = G_VALUE_INIT;

      /* auto-conversion of the callers value type
       */
      if (G_VALUE_TYPE (value) == G_PARAM_SPEC_VALUE_TYPE (pspec))
        {
          g_value_reset (value);
          prop_value = value;
        }
      else if (!g_value_type_transformable (G_PARAM_SPEC_VALUE_TYPE (pspec), G_VALUE_TYPE (value)))
        {
          g_warning ("can't retrieve cell property '%s' of type '%s' as value of type '%s'",
                     pspec->name,
                     g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)),
                     G_VALUE_TYPE_NAME (value));
          return;
        }
      else
        {
          g_value_init (&tmp_value, G_PARAM_SPEC_VALUE_TYPE (pspec));
          prop_value = &tmp_value;
        }

      area_get_cell_property (area, renderer, pspec, prop_value);

      if (prop_value != value)
        {
          g_value_transform (prop_value, value);
          g_value_unset (&tmp_value);
        }
    }
}

/*************************************************************
 *                         API: Focus                        *
 *************************************************************/

/**
 * ctk_cell_area_is_activatable:
 * @area: a #CtkCellArea
 *
 * Returns whether the area can do anything when activated,
 * after applying new attributes to @area.
 *
 * Returns: whether @area can do anything when activated.
 *
 * Since: 3.0
 */
gboolean
ctk_cell_area_is_activatable (CtkCellArea *area)
{
  g_return_val_if_fail (CTK_IS_CELL_AREA (area), FALSE);

  return CTK_CELL_AREA_GET_CLASS (area)->is_activatable (area);
}

/**
 * ctk_cell_area_focus:
 * @area: a #CtkCellArea
 * @direction: the #CtkDirectionType
 *
 * This should be called by the @area’s owning layout widget
 * when focus is to be passed to @area, or moved within @area
 * for a given @direction and row data.
 *
 * Implementing #CtkCellArea classes should implement this
 * method to receive and navigate focus in its own way particular
 * to how it lays out cells.
 *
 * Returns: %TRUE if focus remains inside @area as a result of this call.
 *
 * Since: 3.0
 */
gboolean
ctk_cell_area_focus (CtkCellArea      *area,
                     CtkDirectionType  direction)
{
  g_return_val_if_fail (CTK_IS_CELL_AREA (area), FALSE);

  return CTK_CELL_AREA_GET_CLASS (area)->focus (area, direction);
}

/**
 * ctk_cell_area_activate:
 * @area: a #CtkCellArea
 * @context: the #CtkCellAreaContext in context with the current row data
 * @widget: the #CtkWidget that @area is rendering on
 * @cell_area: the size and location of @area relative to @widget’s allocation
 * @flags: the #CtkCellRendererState flags for @area for this row of data.
 * @edit_only: if %TRUE then only cell renderers that are %CTK_CELL_RENDERER_MODE_EDITABLE
 *             will be activated.
 *
 * Activates @area, usually by activating the currently focused
 * cell, however some subclasses which embed widgets in the area
 * can also activate a widget if it currently has the focus.
 *
 * Returns: Whether @area was successfully activated.
 *
 * Since: 3.0
 */
gboolean
ctk_cell_area_activate (CtkCellArea         *area,
                        CtkCellAreaContext  *context,
                        CtkWidget           *widget,
                        const CdkRectangle  *cell_area,
                        CtkCellRendererState flags,
                        gboolean             edit_only)
{
  g_return_val_if_fail (CTK_IS_CELL_AREA (area), FALSE);

  return CTK_CELL_AREA_GET_CLASS (area)->activate (area, context, widget, cell_area, flags, edit_only);
}


/**
 * ctk_cell_area_set_focus_cell:
 * @area: a #CtkCellArea
 * @renderer: the #CtkCellRenderer to give focus to
 *
 * Explicitly sets the currently focused cell to @renderer.
 *
 * This is generally called by implementations of
 * #CtkCellAreaClass.focus() or #CtkCellAreaClass.event(),
 * however it can also be used to implement functions such
 * as ctk_tree_view_set_cursor_on_cell().
 *
 * Since: 3.0
 */
void
ctk_cell_area_set_focus_cell (CtkCellArea     *area,
                              CtkCellRenderer *renderer)
{
  CtkCellAreaPrivate *priv;

  g_return_if_fail (CTK_IS_CELL_AREA (area));
  g_return_if_fail (renderer == NULL || CTK_IS_CELL_RENDERER (renderer));

  priv = area->priv;

  if (priv->focus_cell != renderer)
    {
      if (priv->focus_cell)
        g_object_unref (priv->focus_cell);

      priv->focus_cell = renderer;

      if (priv->focus_cell)
        g_object_ref (priv->focus_cell);

      g_object_notify (G_OBJECT (area), "focus-cell");
    }

  /* Signal that the current focus renderer for this path changed
   * (it may be that the focus cell did not change, but the row
   * may have changed so we need to signal it) */
  g_signal_emit (area, cell_area_signals[SIGNAL_FOCUS_CHANGED], 0,
                 priv->focus_cell, priv->current_path);

}

/**
 * ctk_cell_area_get_focus_cell:
 * @area: a #CtkCellArea
 *
 * Retrieves the currently focused cell for @area
 *
 * Returns: (transfer none): the currently focused cell in @area.
 *
 * Since: 3.0
 */
CtkCellRenderer *
ctk_cell_area_get_focus_cell (CtkCellArea *area)
{
  CtkCellAreaPrivate *priv;

  g_return_val_if_fail (CTK_IS_CELL_AREA (area), NULL);

  priv = area->priv;

  return priv->focus_cell;
}


/*************************************************************
 *                    API: Focus Siblings                    *
 *************************************************************/

/**
 * ctk_cell_area_add_focus_sibling:
 * @area: a #CtkCellArea
 * @renderer: the #CtkCellRenderer expected to have focus
 * @sibling: the #CtkCellRenderer to add to @renderer’s focus area
 *
 * Adds @sibling to @renderer’s focusable area, focus will be drawn
 * around @renderer and all of its siblings if @renderer can
 * focus for a given row.
 *
 * Events handled by focus siblings can also activate the given
 * focusable @renderer.
 *
 * Since: 3.0
 */
void
ctk_cell_area_add_focus_sibling (CtkCellArea     *area,
                                 CtkCellRenderer *renderer,
                                 CtkCellRenderer *sibling)
{
  CtkCellAreaPrivate *priv;
  GList              *siblings;

  g_return_if_fail (CTK_IS_CELL_AREA (area));
  g_return_if_fail (CTK_IS_CELL_RENDERER (renderer));
  g_return_if_fail (CTK_IS_CELL_RENDERER (sibling));
  g_return_if_fail (renderer != sibling);
  g_return_if_fail (ctk_cell_area_has_renderer (area, renderer));
  g_return_if_fail (ctk_cell_area_has_renderer (area, sibling));
  g_return_if_fail (!ctk_cell_area_is_focus_sibling (area, renderer, sibling));

  /* XXX We should also check that sibling is not in any other renderer's sibling
   * list already, a renderer can be sibling of only one focusable renderer
   * at a time.
   */

  priv = area->priv;

  siblings = g_hash_table_lookup (priv->focus_siblings, renderer);

  if (siblings)
    siblings = g_list_append (siblings, sibling);
  else
    {
      siblings = g_list_append (siblings, sibling);
      g_hash_table_insert (priv->focus_siblings, renderer, siblings);
    }
}

/**
 * ctk_cell_area_remove_focus_sibling:
 * @area: a #CtkCellArea
 * @renderer: the #CtkCellRenderer expected to have focus
 * @sibling: the #CtkCellRenderer to remove from @renderer’s focus area
 *
 * Removes @sibling from @renderer’s focus sibling list
 * (see ctk_cell_area_add_focus_sibling()).
 *
 * Since: 3.0
 */
void
ctk_cell_area_remove_focus_sibling (CtkCellArea     *area,
                                    CtkCellRenderer *renderer,
                                    CtkCellRenderer *sibling)
{
  CtkCellAreaPrivate *priv;
  GList              *siblings;

  g_return_if_fail (CTK_IS_CELL_AREA (area));
  g_return_if_fail (CTK_IS_CELL_RENDERER (renderer));
  g_return_if_fail (CTK_IS_CELL_RENDERER (sibling));
  g_return_if_fail (ctk_cell_area_is_focus_sibling (area, renderer, sibling));

  priv = area->priv;

  siblings = g_hash_table_lookup (priv->focus_siblings, renderer);

  siblings = g_list_copy (siblings);
  siblings = g_list_remove (siblings, sibling);

  if (!siblings)
    g_hash_table_remove (priv->focus_siblings, renderer);
  else
    g_hash_table_insert (priv->focus_siblings, renderer, siblings);
}

/**
 * ctk_cell_area_is_focus_sibling:
 * @area: a #CtkCellArea
 * @renderer: the #CtkCellRenderer expected to have focus
 * @sibling: the #CtkCellRenderer to check against @renderer’s sibling list
 *
 * Returns whether @sibling is one of @renderer’s focus siblings
 * (see ctk_cell_area_add_focus_sibling()).
 *
 * Returns: %TRUE if @sibling is a focus sibling of @renderer
 *
 * Since: 3.0
 */
gboolean
ctk_cell_area_is_focus_sibling (CtkCellArea     *area,
                                CtkCellRenderer *renderer,
                                CtkCellRenderer *sibling)
{
  CtkCellAreaPrivate *priv;
  GList              *siblings, *l;

  g_return_val_if_fail (CTK_IS_CELL_AREA (area), FALSE);
  g_return_val_if_fail (CTK_IS_CELL_RENDERER (renderer), FALSE);
  g_return_val_if_fail (CTK_IS_CELL_RENDERER (sibling), FALSE);

  priv = area->priv;

  siblings = g_hash_table_lookup (priv->focus_siblings, renderer);

  for (l = siblings; l; l = l->next)
    {
      CtkCellRenderer *a_sibling = l->data;

      if (a_sibling == sibling)
        return TRUE;
    }

  return FALSE;
}

/**
 * ctk_cell_area_get_focus_siblings:
 * @area: a #CtkCellArea
 * @renderer: the #CtkCellRenderer expected to have focus
 *
 * Gets the focus sibling cell renderers for @renderer.
 *
 * Returns: (element-type CtkCellRenderer) (transfer none): A #GList of #CtkCellRenderers.
 *       The returned list is internal and should not be freed.
 *
 * Since: 3.0
 */
const GList *
ctk_cell_area_get_focus_siblings (CtkCellArea     *area,
                                  CtkCellRenderer *renderer)
{
  CtkCellAreaPrivate *priv;

  g_return_val_if_fail (CTK_IS_CELL_AREA (area), NULL);
  g_return_val_if_fail (CTK_IS_CELL_RENDERER (renderer), NULL);

  priv = area->priv;

  return g_hash_table_lookup (priv->focus_siblings, renderer);
}

/**
 * ctk_cell_area_get_focus_from_sibling:
 * @area: a #CtkCellArea
 * @renderer: the #CtkCellRenderer
 *
 * Gets the #CtkCellRenderer which is expected to be focusable
 * for which @renderer is, or may be a sibling.
 *
 * This is handy for #CtkCellArea subclasses when handling events,
 * after determining the renderer at the event location it can
 * then chose to activate the focus cell for which the event
 * cell may have been a sibling.
 *
 * Returns: (nullable) (transfer none): the #CtkCellRenderer for which @renderer
 *    is a sibling, or %NULL.
 *
 * Since: 3.0
 */
CtkCellRenderer *
ctk_cell_area_get_focus_from_sibling (CtkCellArea          *area,
                                      CtkCellRenderer      *renderer)
{
  CtkCellRenderer *ret_renderer = NULL;
  GList           *renderers, *l;

  g_return_val_if_fail (CTK_IS_CELL_AREA (area), NULL);
  g_return_val_if_fail (CTK_IS_CELL_RENDERER (renderer), NULL);

  renderers = ctk_cell_layout_get_cells (CTK_CELL_LAYOUT (area));

  for (l = renderers; l; l = l->next)
    {
      CtkCellRenderer *a_renderer = l->data;
      const GList     *list;

      for (list = ctk_cell_area_get_focus_siblings (area, a_renderer);
           list; list = list->next)
        {
          CtkCellRenderer *sibling_renderer = list->data;

          if (sibling_renderer == renderer)
            {
              ret_renderer = a_renderer;
              break;
            }
        }
    }
  g_list_free (renderers);

  return ret_renderer;
}

/*************************************************************
 *              API: Cell Activation/Editing                 *
 *************************************************************/
static void
ctk_cell_area_add_editable (CtkCellArea        *area,
                            CtkCellRenderer    *renderer,
                            CtkCellEditable    *editable,
                            const CdkRectangle *cell_area)
{
  g_signal_emit (area, cell_area_signals[SIGNAL_ADD_EDITABLE], 0,
                 renderer, editable, cell_area, area->priv->current_path);
}

static void
ctk_cell_area_remove_editable  (CtkCellArea        *area,
                                CtkCellRenderer    *renderer,
                                CtkCellEditable    *editable)
{
  g_signal_emit (area, cell_area_signals[SIGNAL_REMOVE_EDITABLE], 0, renderer, editable);
}

static void
cell_area_remove_widget_cb (CtkCellEditable *editable,
                            CtkCellArea     *area)
{
  CtkCellAreaPrivate *priv = area->priv;

  g_assert (priv->edit_widget == editable);
  g_assert (priv->edited_cell != NULL);

  ctk_cell_area_remove_editable (area, priv->edited_cell, priv->edit_widget);

  /* Now that we're done with editing the widget and it can be removed,
   * remove our references to the widget and disconnect handlers */
  ctk_cell_area_set_edited_cell (area, NULL);
  ctk_cell_area_set_edit_widget (area, NULL);
}

static void
ctk_cell_area_set_edited_cell (CtkCellArea     *area,
                               CtkCellRenderer *renderer)
{
  CtkCellAreaPrivate *priv;

  g_return_if_fail (CTK_IS_CELL_AREA (area));
  g_return_if_fail (renderer == NULL || CTK_IS_CELL_RENDERER (renderer));

  priv = area->priv;

  if (priv->edited_cell != renderer)
    {
      if (priv->edited_cell)
        g_object_unref (priv->edited_cell);

      priv->edited_cell = renderer;

      if (priv->edited_cell)
        g_object_ref (priv->edited_cell);

      g_object_notify (G_OBJECT (area), "edited-cell");
    }
}

static void
ctk_cell_area_set_edit_widget (CtkCellArea     *area,
                               CtkCellEditable *editable)
{
  CtkCellAreaPrivate *priv;

  g_return_if_fail (CTK_IS_CELL_AREA (area));
  g_return_if_fail (editable == NULL || CTK_IS_CELL_EDITABLE (editable));

  priv = area->priv;

  if (priv->edit_widget != editable)
    {
      if (priv->edit_widget)
        {
          g_signal_handler_disconnect (priv->edit_widget, priv->remove_widget_id);

          g_object_unref (priv->edit_widget);
        }

      priv->edit_widget = editable;

      if (priv->edit_widget)
        {
          priv->remove_widget_id =
            g_signal_connect (priv->edit_widget, "remove-widget",
                              G_CALLBACK (cell_area_remove_widget_cb), area);

          g_object_ref (priv->edit_widget);
        }

      g_object_notify (G_OBJECT (area), "edit-widget");
    }
}

/**
 * ctk_cell_area_get_edited_cell:
 * @area: a #CtkCellArea
 *
 * Gets the #CtkCellRenderer in @area that is currently
 * being edited.
 *
 * Returns: (transfer none): The currently edited #CtkCellRenderer
 *
 * Since: 3.0
 */
CtkCellRenderer   *
ctk_cell_area_get_edited_cell (CtkCellArea *area)
{
  CtkCellAreaPrivate *priv;

  g_return_val_if_fail (CTK_IS_CELL_AREA (area), NULL);

  priv = area->priv;

  return priv->edited_cell;
}

/**
 * ctk_cell_area_get_edit_widget:
 * @area: a #CtkCellArea
 *
 * Gets the #CtkCellEditable widget currently used
 * to edit the currently edited cell.
 *
 * Returns: (transfer none): The currently active #CtkCellEditable widget
 *
 * Since: 3.0
 */
CtkCellEditable *
ctk_cell_area_get_edit_widget (CtkCellArea *area)
{
  CtkCellAreaPrivate *priv;

  g_return_val_if_fail (CTK_IS_CELL_AREA (area), NULL);

  priv = area->priv;

  return priv->edit_widget;
}

/**
 * ctk_cell_area_activate_cell:
 * @area: a #CtkCellArea
 * @widget: the #CtkWidget that @area is rendering onto
 * @renderer: the #CtkCellRenderer in @area to activate
 * @event: the #CdkEvent for which cell activation should occur
 * @cell_area: the #CdkRectangle in @widget relative coordinates
 *             of @renderer for the current row.
 * @flags: the #CtkCellRendererState for @renderer
 *
 * This is used by #CtkCellArea subclasses when handling events
 * to activate cells, the base #CtkCellArea class activates cells
 * for keyboard events for free in its own CtkCellArea->activate()
 * implementation.
 *
 * Returns: whether cell activation was successful
 *
 * Since: 3.0
 */
gboolean
ctk_cell_area_activate_cell (CtkCellArea          *area,
                             CtkWidget            *widget,
                             CtkCellRenderer      *renderer,
                             CdkEvent             *event,
                             const CdkRectangle   *cell_area,
                             CtkCellRendererState  flags)
{
  CtkCellRendererMode mode;
  CtkCellAreaPrivate *priv;

  g_return_val_if_fail (CTK_IS_CELL_AREA (area), FALSE);
  g_return_val_if_fail (CTK_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (CTK_IS_CELL_RENDERER (renderer), FALSE);
  g_return_val_if_fail (cell_area != NULL, FALSE);

  priv = area->priv;

  if (!ctk_cell_renderer_get_sensitive (renderer))
    return FALSE;

  g_object_get (renderer, "mode", &mode, NULL);

  if (mode == CTK_CELL_RENDERER_MODE_ACTIVATABLE)
    {
      if (ctk_cell_renderer_activate (renderer,
                                      event, widget,
                                      priv->current_path,
                                      cell_area,
                                      cell_area,
                                      flags))
        return TRUE;
    }
  else if (mode == CTK_CELL_RENDERER_MODE_EDITABLE)
    {
      CtkCellEditable *editable_widget;
      CdkRectangle inner_area;

      ctk_cell_area_inner_cell_area (area, widget, cell_area, &inner_area);

      editable_widget =
        ctk_cell_renderer_start_editing (renderer,
                                         event, widget,
                                         priv->current_path,
                                         &inner_area,
                                         &inner_area,
                                         flags);

      if (editable_widget != NULL)
        {
          g_return_val_if_fail (CTK_IS_CELL_EDITABLE (editable_widget), FALSE);

          ctk_cell_area_set_edited_cell (area, renderer);
          ctk_cell_area_set_edit_widget (area, editable_widget);

          /* Signal that editing started so that callers can get
           * a handle on the editable_widget */
          ctk_cell_area_add_editable (area, priv->focus_cell, editable_widget, cell_area);

          /* If the signal was successfully handled start the editing */
          if (ctk_widget_get_parent (CTK_WIDGET (editable_widget)))
            {
              ctk_cell_editable_start_editing (editable_widget, event);
              ctk_widget_grab_focus (CTK_WIDGET (editable_widget));
            }
          else
            {
              /* Otherwise clear the editing state and fire a warning */
              ctk_cell_area_set_edited_cell (area, NULL);
              ctk_cell_area_set_edit_widget (area, NULL);

              g_warning ("CtkCellArea::add-editable fired in the dark, no cell editing was started.");
            }

          return TRUE;
        }
    }

  return FALSE;
}

/**
 * ctk_cell_area_stop_editing:
 * @area: a #CtkCellArea
 * @canceled: whether editing was canceled.
 *
 * Explicitly stops the editing of the currently edited cell.
 *
 * If @canceled is %TRUE, the currently edited cell renderer
 * will emit the ::editing-canceled signal, otherwise the
 * the ::editing-done signal will be emitted on the current
 * edit widget.
 *
 * See ctk_cell_area_get_edited_cell() and ctk_cell_area_get_edit_widget().
 *
 * Since: 3.0
 */
void
ctk_cell_area_stop_editing (CtkCellArea *area,
                            gboolean     canceled)
{
  CtkCellAreaPrivate *priv;

  g_return_if_fail (CTK_IS_CELL_AREA (area));

  priv = area->priv;

  if (priv->edited_cell)
    {
      CtkCellEditable *edit_widget = g_object_ref (priv->edit_widget);
      CtkCellRenderer *edit_cell   = g_object_ref (priv->edited_cell);

      /* Stop editing of the cell renderer */
      ctk_cell_renderer_stop_editing (priv->edited_cell, canceled);

      /* When editing is explicitly halted either
       * the "editing-canceled" signal is emitted on the cell 
       * renderer or the "editing-done" signal on the CtkCellEditable widget
       */
      if (!canceled)
	ctk_cell_editable_editing_done (edit_widget);

      /* Remove any references to the editable widget */
      ctk_cell_area_set_edited_cell (area, NULL);
      ctk_cell_area_set_edit_widget (area, NULL);

      /* Send the remove-widget signal explicitly (this is done after setting
       * the edit cell/widget NULL to avoid feedback)
       */
      ctk_cell_area_remove_editable (area, edit_cell, edit_widget);
      g_object_unref (edit_cell);
      g_object_unref (edit_widget);
    }
}

/*************************************************************
 *         API: Convenience for area implementations         *
 *************************************************************/

/**
 * ctk_cell_area_inner_cell_area:
 * @area: a #CtkCellArea
 * @widget: the #CtkWidget that @area is rendering onto
 * @cell_area: the @widget relative coordinates where one of @area’s cells
 *             is to be placed
 * @inner_area: (out): the return location for the inner cell area
 *
 * This is a convenience function for #CtkCellArea implementations
 * to get the inner area where a given #CtkCellRenderer will be
 * rendered. It removes any padding previously added by ctk_cell_area_request_renderer().
 *
 * Since: 3.0
 */
void
ctk_cell_area_inner_cell_area (CtkCellArea        *area,
                               CtkWidget          *widget,
                               const CdkRectangle *cell_area,
                               CdkRectangle       *inner_area)
{
  CtkBorder border;
  CtkStyleContext *context;
  CtkStateFlags state;

  g_return_if_fail (CTK_IS_CELL_AREA (area));
  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (cell_area != NULL);
  g_return_if_fail (inner_area != NULL);

  context = ctk_widget_get_style_context (widget);
  state = ctk_style_context_get_state (context);
  ctk_style_context_get_padding (context, state, &border);

  *inner_area = *cell_area;

  inner_area->x += border.left;
  inner_area->width -= border.left + border.right;
  inner_area->y += border.top;
  inner_area->height -= border.top + border.bottom;
}

/**
 * ctk_cell_area_request_renderer:
 * @area: a #CtkCellArea
 * @renderer: the #CtkCellRenderer to request size for
 * @orientation: the #CtkOrientation in which to request size
 * @widget: the #CtkWidget that @area is rendering onto
 * @for_size: the allocation contextual size to request for, or -1 if
 * the base request for the orientation is to be returned.
 * @minimum_size: (out) (allow-none): location to store the minimum size, or %NULL
 * @natural_size: (out) (allow-none): location to store the natural size, or %NULL
 *
 * This is a convenience function for #CtkCellArea implementations
 * to request size for cell renderers. It’s important to use this
 * function to request size and then use ctk_cell_area_inner_cell_area()
 * at render and event time since this function will add padding
 * around the cell for focus painting.
 *
 * Since: 3.0
 */
void
ctk_cell_area_request_renderer (CtkCellArea        *area,
                                CtkCellRenderer    *renderer,
                                CtkOrientation      orientation,
                                CtkWidget          *widget,
                                gint                for_size,
                                gint               *minimum_size,
                                gint               *natural_size)
{
  CtkBorder border;
  CtkStyleContext *context;
  CtkStateFlags state;

  g_return_if_fail (CTK_IS_CELL_AREA (area));
  g_return_if_fail (CTK_IS_CELL_RENDERER (renderer));
  g_return_if_fail (CTK_IS_WIDGET (widget));
  g_return_if_fail (minimum_size != NULL);
  g_return_if_fail (natural_size != NULL);

  context = ctk_widget_get_style_context (widget);
  state = ctk_style_context_get_state (context);
  ctk_style_context_get_padding (context, state, &border);

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      if (for_size < 0)
          ctk_cell_renderer_get_preferred_width (renderer, widget, minimum_size, natural_size);
      else
        {
          for_size = MAX (0, for_size - border.left - border.right);

          ctk_cell_renderer_get_preferred_width_for_height (renderer, widget, for_size,
                                                            minimum_size, natural_size);
        }

      *minimum_size += border.left + border.right;
      *natural_size += border.left + border.right;
    }
  else /* CTK_ORIENTATION_VERTICAL */
    {
      if (for_size < 0)
        ctk_cell_renderer_get_preferred_height (renderer, widget, minimum_size, natural_size);
      else
        {
          for_size = MAX (0, for_size - border.top - border.bottom);

          ctk_cell_renderer_get_preferred_height_for_width (renderer, widget, for_size,
                                                            minimum_size, natural_size);
        }

      *minimum_size += border.top + border.bottom;
      *natural_size += border.top + border.bottom;
    }
}

void
_ctk_cell_area_set_cell_data_func_with_proxy (CtkCellArea           *area,
					      CtkCellRenderer       *cell,
					      GFunc                  func,
					      gpointer               func_data,
					      GDestroyNotify         destroy,
					      gpointer               proxy)
{
  CtkCellAreaPrivate *priv;
  CellInfo           *info;

  g_return_if_fail (CTK_IS_CELL_AREA (area));
  g_return_if_fail (CTK_IS_CELL_RENDERER (cell));

  priv = area->priv;

  info = g_hash_table_lookup (priv->cell_info, cell);

  /* Note we do not take a reference to the proxy, the proxy is a CtkCellLayout
   * that is forwarding its implementation to a delegate CtkCellArea therefore
   * its life-cycle is longer than the area's life cycle. 
   */
  if (info)
    {
      if (info->destroy && info->data)
	info->destroy (info->data);

      if (func)
	{
	  info->func    = (CtkCellLayoutDataFunc)func;
	  info->data    = func_data;
	  info->destroy = destroy;
	  info->proxy   = proxy;
	}
      else
	{
	  info->func    = NULL;
	  info->data    = NULL;
	  info->destroy = NULL;
	  info->proxy   = NULL;
	}
    }
  else
    {
      info = cell_info_new ((CtkCellLayoutDataFunc)func, func_data, destroy);
      info->proxy = proxy;

      g_hash_table_insert (priv->cell_info, cell, info);
    }
}
