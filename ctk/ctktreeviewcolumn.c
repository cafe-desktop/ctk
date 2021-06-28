/* ctktreeviewcolumn.c
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

#include "config.h"

#include "ctktreeviewcolumn.h"

#include <string.h>

#include "ctktreeview.h"
#include "ctktreeprivate.h"
#include "ctkcelllayout.h"
#include "ctkbutton.h"
#include "deprecated/ctkalignment.h"
#include "ctklabel.h"
#include "ctkbox.h"
#include "ctkmarshalers.h"
#include "ctkimage.h"
#include "ctkcellareacontext.h"
#include "ctkcellareabox.h"
#include "ctkprivate.h"
#include "ctkintl.h"
#include "ctktypebuiltins.h"
#include "a11y/ctktreeviewaccessibleprivate.h"


/**
 * SECTION:ctktreeviewcolumn
 * @Short_description: A visible column in a CtkTreeView widget
 * @Title: CtkTreeViewColumn
 * @See_also: #CtkTreeView, #CtkTreeSelection, #CtkTreeModel, #CtkTreeSortable,
 *   #CtkTreeModelSort, #CtkListStore, #CtkTreeStore, #CtkCellRenderer, #CtkCellEditable,
 *   #CtkCellRendererPixbuf, #CtkCellRendererText, #CtkCellRendererToggle,
 *   [CtkTreeView drag-and-drop][ctk3-CtkTreeView-drag-and-drop]
 *
 * The CtkTreeViewColumn object represents a visible column in a #CtkTreeView widget.
 * It allows to set properties of the column header, and functions as a holding pen for
 * the cell renderers which determine how the data in the column is displayed.
 *
 * Please refer to the [tree widget conceptual overview][TreeWidget]
 * for an overview of all the objects and data types related to the tree widget and how
 * they work together.
 */


/* Type methods */
static void ctk_tree_view_column_cell_layout_init              (CtkCellLayoutIface      *iface);

/* GObject methods */
static void ctk_tree_view_column_set_property                  (GObject                 *object,
								guint                    prop_id,
								const GValue            *value,
								GParamSpec              *pspec);
static void ctk_tree_view_column_get_property                  (GObject                 *object,
								guint                    prop_id,
								GValue                  *value,
								GParamSpec              *pspec);
static void ctk_tree_view_column_finalize                      (GObject                 *object);
static void ctk_tree_view_column_dispose                       (GObject                 *object);
static void ctk_tree_view_column_constructed                   (GObject                 *object);

/* CtkCellLayout implementation */
static void       ctk_tree_view_column_ensure_cell_area        (CtkTreeViewColumn      *column,
                                                                CtkCellArea            *cell_area);

static CtkCellArea *ctk_tree_view_column_cell_layout_get_area  (CtkCellLayout           *cell_layout);

/* Button handling code */
static void ctk_tree_view_column_create_button                 (CtkTreeViewColumn       *tree_column);
static void ctk_tree_view_column_update_button                 (CtkTreeViewColumn       *tree_column);

/* Button signal handlers */
static gint ctk_tree_view_column_button_event                  (CtkWidget               *widget,
								CdkEvent                *event,
								gpointer                 data);
static void ctk_tree_view_column_button_clicked                (CtkWidget               *widget,
								gpointer                 data);
static gboolean ctk_tree_view_column_mnemonic_activate         (CtkWidget *widget,
					                        gboolean   group_cycling,
								gpointer   data);

/* Property handlers */
static void ctk_tree_view_model_sort_column_changed            (CtkTreeSortable         *sortable,
								CtkTreeViewColumn       *tree_column);

/* CtkCellArea/CtkCellAreaContext callbacks */
static void ctk_tree_view_column_context_changed               (CtkCellAreaContext      *context,
								GParamSpec              *pspec,
								CtkTreeViewColumn       *tree_column);
static void ctk_tree_view_column_add_editable_callback         (CtkCellArea             *area,
								CtkCellRenderer         *renderer,
								CtkCellEditable         *edit_widget,
								CdkRectangle            *cell_area,
								const gchar             *path_string,
								gpointer                 user_data);
static void ctk_tree_view_column_remove_editable_callback      (CtkCellArea             *area,
								CtkCellRenderer         *renderer,
								CtkCellEditable         *edit_widget,
								gpointer                 user_data);

/* Internal functions */
static void ctk_tree_view_column_sort                          (CtkTreeViewColumn       *tree_column,
								gpointer                 data);
static void ctk_tree_view_column_setup_sort_column_id_callback (CtkTreeViewColumn       *tree_column);
static void ctk_tree_view_column_set_attributesv               (CtkTreeViewColumn       *tree_column,
								CtkCellRenderer         *cell_renderer,
								va_list                  args);

/* CtkBuildable implementation */
static void ctk_tree_view_column_buildable_init                 (CtkBuildableIface     *iface);


struct _CtkTreeViewColumnPrivate 
{
  CtkWidget *tree_view;
  CtkWidget *button;
  CtkWidget *child;
  CtkWidget *arrow;
  CtkWidget *alignment;
  CdkWindow *window;
  gulong property_changed_signal;
  gfloat xalign;

  /* Sizing fields */
  /* see ctk+/doc/tree-column-sizing.txt for more information on them */
  CtkTreeViewColumnSizing column_type;
  gint padding;
  gint x_offset;
  gint width;
  gint fixed_width;
  gint min_width;
  gint max_width;

  /* dragging columns */
  gint drag_x;
  gint drag_y;

  gchar *title;

  /* Sorting */
  gulong      sort_clicked_signal;
  gulong      sort_column_changed_signal;
  gint        sort_column_id;
  CtkSortType sort_order;

  /* Cell area */
  CtkCellArea        *cell_area;
  CtkCellAreaContext *cell_area_context;
  gulong              add_editable_signal;
  gulong              remove_editable_signal;
  gulong              context_changed_signal;

  /* Flags */
  guint visible             : 1;
  guint resizable           : 1;
  guint clickable           : 1;
  guint dirty               : 1;
  guint show_sort_indicator : 1;
  guint maybe_reordered     : 1;
  guint reorderable         : 1;
  guint expand              : 1;
};

enum
{
  PROP_0,
  PROP_VISIBLE,
  PROP_RESIZABLE,
  PROP_X_OFFSET,
  PROP_WIDTH,
  PROP_SPACING,
  PROP_SIZING,
  PROP_FIXED_WIDTH,
  PROP_MIN_WIDTH,
  PROP_MAX_WIDTH,
  PROP_TITLE,
  PROP_EXPAND,
  PROP_CLICKABLE,
  PROP_WIDGET,
  PROP_ALIGNMENT,
  PROP_REORDERABLE,
  PROP_SORT_INDICATOR,
  PROP_SORT_ORDER,
  PROP_SORT_COLUMN_ID,
  PROP_CELL_AREA,
  LAST_PROP
};

enum
{
  CLICKED,
  LAST_SIGNAL
};

static guint tree_column_signals[LAST_SIGNAL] = { 0 };
static GParamSpec *tree_column_props[LAST_PROP] = { NULL, };

G_DEFINE_TYPE_WITH_CODE (CtkTreeViewColumn, ctk_tree_view_column, G_TYPE_INITIALLY_UNOWNED,
                         G_ADD_PRIVATE (CtkTreeViewColumn)
			 G_IMPLEMENT_INTERFACE (CTK_TYPE_CELL_LAYOUT,
						ctk_tree_view_column_cell_layout_init)
			 G_IMPLEMENT_INTERFACE (CTK_TYPE_BUILDABLE,
						ctk_tree_view_column_buildable_init))


static void
ctk_tree_view_column_class_init (CtkTreeViewColumnClass *class)
{
  GObjectClass *object_class;

  object_class = (GObjectClass*) class;

  class->clicked = NULL;

  object_class->constructed = ctk_tree_view_column_constructed;
  object_class->finalize = ctk_tree_view_column_finalize;
  object_class->dispose = ctk_tree_view_column_dispose;
  object_class->set_property = ctk_tree_view_column_set_property;
  object_class->get_property = ctk_tree_view_column_get_property;
  
  tree_column_signals[CLICKED] =
    g_signal_new (I_("clicked"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkTreeViewColumnClass, clicked),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  tree_column_props[PROP_VISIBLE] =
      g_param_spec_boolean ("visible",
                            P_("Visible"),
                            P_("Whether to display the column"),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_column_props[PROP_RESIZABLE] =
      g_param_spec_boolean ("resizable",
                            P_("Resizable"),
                            P_("Column is user-resizable"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_column_props[PROP_X_OFFSET] =
      g_param_spec_int ("x-offset",
                        P_("X position"),
                        P_("Current X position of the column"),
                        -G_MAXINT, G_MAXINT,
                        0,
                        CTK_PARAM_READABLE);

  tree_column_props[PROP_WIDTH] =
      g_param_spec_int ("width",
                        P_("Width"),
                        P_("Current width of the column"),
                        0, G_MAXINT,
                        0,
                        CTK_PARAM_READABLE);

  tree_column_props[PROP_SPACING] =
      g_param_spec_int ("spacing",
                        P_("Spacing"),
                        P_("Space which is inserted between cells"),
                        0, G_MAXINT,
                        0,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_column_props[PROP_SIZING] =
      g_param_spec_enum ("sizing",
                         P_("Sizing"),
                         P_("Resize mode of the column"),
                         CTK_TYPE_TREE_VIEW_COLUMN_SIZING,
                         CTK_TREE_VIEW_COLUMN_GROW_ONLY,
                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_column_props[PROP_FIXED_WIDTH] =
      g_param_spec_int ("fixed-width",
                         P_("Fixed Width"),
                         P_("Current fixed width of the column"),
                         -1, G_MAXINT,
                         -1,
                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_column_props[PROP_MIN_WIDTH] =
      g_param_spec_int ("min-width",
                        P_("Minimum Width"),
                        P_("Minimum allowed width of the column"),
                        -1, G_MAXINT,
                        -1,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_column_props[PROP_MAX_WIDTH] =
      g_param_spec_int ("max-width",
                        P_("Maximum Width"),
                        P_("Maximum allowed width of the column"),
                        -1, G_MAXINT,
                        -1,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_column_props[PROP_TITLE] =
      g_param_spec_string ("title",
                           P_("Title"),
                           P_("Title to appear in column header"),
                           "",
                           CTK_PARAM_READWRITE);

  tree_column_props[PROP_EXPAND] =
      g_param_spec_boolean ("expand",
                            P_("Expand"),
                            P_("Column gets share of extra width allocated to the widget"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_column_props[PROP_CLICKABLE] =
      g_param_spec_boolean ("clickable",
                            P_("Clickable"),
                            P_("Whether the header can be clicked"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_column_props[PROP_WIDGET] =
      g_param_spec_object ("widget",
                           P_("Widget"),
                           P_("Widget to put in column header button instead of column title"),
                           CTK_TYPE_WIDGET,
                           CTK_PARAM_READWRITE);

  tree_column_props[PROP_ALIGNMENT] =
      g_param_spec_float ("alignment",
                          P_("Alignment"),
                          P_("X Alignment of the column header text or widget"),
                          0.0, 1.0, 0.0,
                          CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_column_props[PROP_REORDERABLE] =
      g_param_spec_boolean ("reorderable",
                            P_("Reorderable"),
                            P_("Whether the column can be reordered around the headers"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_column_props[PROP_SORT_INDICATOR] =
      g_param_spec_boolean ("sort-indicator",
                            P_("Sort indicator"),
                            P_("Whether to show a sort indicator"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  tree_column_props[PROP_SORT_ORDER] =
      g_param_spec_enum ("sort-order",
                         P_("Sort order"),
                         P_("Sort direction the sort indicator should indicate"),
                         CTK_TYPE_SORT_TYPE,
                         CTK_SORT_ASCENDING,
                         CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkTreeViewColumn:sort-column-id:
   *
   * Logical sort column ID this column sorts on when selected for sorting. Setting the sort column ID makes the column header
   * clickable. Set to -1 to make the column unsortable.
   *
   * Since: 2.18
   **/
  tree_column_props[PROP_SORT_COLUMN_ID] =
      g_param_spec_int ("sort-column-id",
                        P_("Sort column ID"),
                        P_("Logical sort column ID this column sorts on when selected for sorting"),
                        -1, G_MAXINT,
                        -1,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkTreeViewColumn:cell-area:
   *
   * The #CtkCellArea used to layout cell renderers for this column.
   *
   * If no area is specified when creating the tree view column with ctk_tree_view_column_new_with_area() 
   * a horizontally oriented #CtkCellAreaBox will be used.
   *
   * Since: 3.0
   */
  tree_column_props[PROP_CELL_AREA] =
      g_param_spec_object ("cell-area",
                           P_("Cell Area"),
                           P_("The CtkCellArea used to layout cells"),
                           CTK_TYPE_CELL_AREA,
                           CTK_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, LAST_PROP, tree_column_props);
}

static void
ctk_tree_view_column_custom_tag_end (CtkBuildable *buildable,
				     CtkBuilder   *builder,
				     GObject      *child,
				     const gchar  *tagname,
				     gpointer     *data)
{
  /* Just ignore the boolean return from here */
  _ctk_cell_layout_buildable_custom_tag_end (buildable, builder, child, tagname, data);
}

static void
ctk_tree_view_column_buildable_init (CtkBuildableIface *iface)
{
  iface->add_child = _ctk_cell_layout_buildable_add_child;
  iface->custom_tag_start = _ctk_cell_layout_buildable_custom_tag_start;
  iface->custom_tag_end = ctk_tree_view_column_custom_tag_end;
}

static void
ctk_tree_view_column_cell_layout_init (CtkCellLayoutIface *iface)
{
  iface->get_area = ctk_tree_view_column_cell_layout_get_area;
}

static void
ctk_tree_view_column_init (CtkTreeViewColumn *tree_column)
{
  CtkTreeViewColumnPrivate *priv;

  tree_column->priv = ctk_tree_view_column_get_instance_private (tree_column);
  priv = tree_column->priv;

  priv->button = NULL;
  priv->xalign = 0.0;
  priv->width = 0;
  priv->padding = 0;
  priv->min_width = -1;
  priv->max_width = -1;
  priv->column_type = CTK_TREE_VIEW_COLUMN_GROW_ONLY;
  priv->visible = TRUE;
  priv->resizable = FALSE;
  priv->expand = FALSE;
  priv->clickable = FALSE;
  priv->dirty = TRUE;
  priv->sort_order = CTK_SORT_ASCENDING;
  priv->show_sort_indicator = FALSE;
  priv->property_changed_signal = 0;
  priv->sort_clicked_signal = 0;
  priv->sort_column_changed_signal = 0;
  priv->sort_column_id = -1;
  priv->reorderable = FALSE;
  priv->maybe_reordered = FALSE;
  priv->fixed_width = -1;
  priv->title = g_strdup ("");

  ctk_tree_view_column_create_button (tree_column);
}

static void
ctk_tree_view_column_constructed (GObject *object)
{
  CtkTreeViewColumn *tree_column = CTK_TREE_VIEW_COLUMN (object);

  G_OBJECT_CLASS (ctk_tree_view_column_parent_class)->constructed (object);

  ctk_tree_view_column_ensure_cell_area (tree_column, NULL);
}

static void
ctk_tree_view_column_dispose (GObject *object)
{
  CtkTreeViewColumn        *tree_column = (CtkTreeViewColumn *) object;
  CtkTreeViewColumnPrivate *priv        = tree_column->priv;

  /* Remove this column from its treeview, 
   * in case this column is destroyed before its treeview.
   */ 
  if (priv->tree_view)
    ctk_tree_view_remove_column (CTK_TREE_VIEW (priv->tree_view), tree_column);
    
  if (priv->cell_area_context)
    { 
      g_signal_handler_disconnect (priv->cell_area_context,
				   priv->context_changed_signal);

      g_object_unref (priv->cell_area_context);

      priv->cell_area_context = NULL;
      priv->context_changed_signal = 0;
    }

  if (priv->cell_area)
    {
      g_signal_handler_disconnect (priv->cell_area,
				   priv->add_editable_signal);
      g_signal_handler_disconnect (priv->cell_area,
				   priv->remove_editable_signal);

      g_object_unref (priv->cell_area);
      priv->cell_area = NULL;
      priv->add_editable_signal = 0;
      priv->remove_editable_signal = 0;
    }

  if (priv->child)
    {
      g_object_unref (priv->child);
      priv->child = NULL;
    }

  g_clear_object (&priv->button);

  G_OBJECT_CLASS (ctk_tree_view_column_parent_class)->dispose (object);
}

static void
ctk_tree_view_column_finalize (GObject *object)
{
  CtkTreeViewColumn        *tree_column = (CtkTreeViewColumn *) object;
  CtkTreeViewColumnPrivate *priv        = tree_column->priv;

  g_free (priv->title);

  G_OBJECT_CLASS (ctk_tree_view_column_parent_class)->finalize (object);
}

static void
ctk_tree_view_column_set_property (GObject         *object,
                                   guint            prop_id,
                                   const GValue    *value,
                                   GParamSpec      *pspec)
{
  CtkTreeViewColumn *tree_column;
  CtkCellArea       *area;

  tree_column = CTK_TREE_VIEW_COLUMN (object);

  switch (prop_id)
    {
    case PROP_VISIBLE:
      ctk_tree_view_column_set_visible (tree_column,
                                        g_value_get_boolean (value));
      break;

    case PROP_RESIZABLE:
      ctk_tree_view_column_set_resizable (tree_column,
					  g_value_get_boolean (value));
      break;

    case PROP_SIZING:
      ctk_tree_view_column_set_sizing (tree_column,
                                       g_value_get_enum (value));
      break;

    case PROP_FIXED_WIDTH:
      ctk_tree_view_column_set_fixed_width (tree_column,
					    g_value_get_int (value));
      break;

    case PROP_MIN_WIDTH:
      ctk_tree_view_column_set_min_width (tree_column,
                                          g_value_get_int (value));
      break;

    case PROP_MAX_WIDTH:
      ctk_tree_view_column_set_max_width (tree_column,
                                          g_value_get_int (value));
      break;

    case PROP_SPACING:
      ctk_tree_view_column_set_spacing (tree_column,
					g_value_get_int (value));
      break;

    case PROP_TITLE:
      ctk_tree_view_column_set_title (tree_column,
                                      g_value_get_string (value));
      break;

    case PROP_EXPAND:
      ctk_tree_view_column_set_expand (tree_column,
				       g_value_get_boolean (value));
      break;

    case PROP_CLICKABLE:
      ctk_tree_view_column_set_clickable (tree_column,
                                          g_value_get_boolean (value));
      break;

    case PROP_WIDGET:
      ctk_tree_view_column_set_widget (tree_column,
                                       (CtkWidget*) g_value_get_object (value));
      break;

    case PROP_ALIGNMENT:
      ctk_tree_view_column_set_alignment (tree_column,
                                          g_value_get_float (value));
      break;

    case PROP_REORDERABLE:
      ctk_tree_view_column_set_reorderable (tree_column,
					    g_value_get_boolean (value));
      break;

    case PROP_SORT_INDICATOR:
      ctk_tree_view_column_set_sort_indicator (tree_column,
                                               g_value_get_boolean (value));
      break;

    case PROP_SORT_ORDER:
      ctk_tree_view_column_set_sort_order (tree_column,
                                           g_value_get_enum (value));
      break;
      
    case PROP_SORT_COLUMN_ID:
      ctk_tree_view_column_set_sort_column_id (tree_column,
                                               g_value_get_int (value));
      break;

    case PROP_CELL_AREA:
      /* Construct-only, can only be assigned once */
      area = g_value_get_object (value);

      if (area)
        {
          if (tree_column->priv->cell_area != NULL)
            {
              g_warning ("cell-area has already been set, ignoring construct property");
              g_object_ref_sink (area);
              g_object_unref (area);
            }
          else
            ctk_tree_view_column_ensure_cell_area (tree_column, area);
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_tree_view_column_get_property (GObject         *object,
                                   guint            prop_id,
                                   GValue          *value,
                                   GParamSpec      *pspec)
{
  CtkTreeViewColumn *tree_column;

  tree_column = CTK_TREE_VIEW_COLUMN (object);

  switch (prop_id)
    {
    case PROP_VISIBLE:
      g_value_set_boolean (value,
                           ctk_tree_view_column_get_visible (tree_column));
      break;

    case PROP_RESIZABLE:
      g_value_set_boolean (value,
                           ctk_tree_view_column_get_resizable (tree_column));
      break;

    case PROP_X_OFFSET:
      g_value_set_int (value,
                       ctk_tree_view_column_get_x_offset (tree_column));
      break;

    case PROP_WIDTH:
      g_value_set_int (value,
                       ctk_tree_view_column_get_width (tree_column));
      break;

    case PROP_SPACING:
      g_value_set_int (value,
                       ctk_tree_view_column_get_spacing (tree_column));
      break;

    case PROP_SIZING:
      g_value_set_enum (value,
                        ctk_tree_view_column_get_sizing (tree_column));
      break;

    case PROP_FIXED_WIDTH:
      g_value_set_int (value,
                       ctk_tree_view_column_get_fixed_width (tree_column));
      break;

    case PROP_MIN_WIDTH:
      g_value_set_int (value,
                       ctk_tree_view_column_get_min_width (tree_column));
      break;

    case PROP_MAX_WIDTH:
      g_value_set_int (value,
                       ctk_tree_view_column_get_max_width (tree_column));
      break;

    case PROP_TITLE:
      g_value_set_string (value,
                          ctk_tree_view_column_get_title (tree_column));
      break;

    case PROP_EXPAND:
      g_value_set_boolean (value,
                          ctk_tree_view_column_get_expand (tree_column));
      break;

    case PROP_CLICKABLE:
      g_value_set_boolean (value,
                           ctk_tree_view_column_get_clickable (tree_column));
      break;

    case PROP_WIDGET:
      g_value_set_object (value,
                          (GObject*) ctk_tree_view_column_get_widget (tree_column));
      break;

    case PROP_ALIGNMENT:
      g_value_set_float (value,
                         ctk_tree_view_column_get_alignment (tree_column));
      break;

    case PROP_REORDERABLE:
      g_value_set_boolean (value,
			   ctk_tree_view_column_get_reorderable (tree_column));
      break;

    case PROP_SORT_INDICATOR:
      g_value_set_boolean (value,
                           ctk_tree_view_column_get_sort_indicator (tree_column));
      break;

    case PROP_SORT_ORDER:
      g_value_set_enum (value,
                        ctk_tree_view_column_get_sort_order (tree_column));
      break;
      
    case PROP_SORT_COLUMN_ID:
      g_value_set_int (value,
                       ctk_tree_view_column_get_sort_column_id (tree_column));
      break;

    case PROP_CELL_AREA:
      g_value_set_object (value, tree_column->priv->cell_area);
      break;
      
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/* Implementation of CtkCellLayout interface
 */

static void
ctk_tree_view_column_ensure_cell_area (CtkTreeViewColumn *column,
                                       CtkCellArea       *cell_area)
{
  CtkTreeViewColumnPrivate *priv = column->priv;

  if (priv->cell_area)
    return;

  if (cell_area)
    priv->cell_area = cell_area;
  else
    priv->cell_area = ctk_cell_area_box_new ();

  g_object_ref_sink (priv->cell_area);

  priv->add_editable_signal =
    g_signal_connect (priv->cell_area, "add-editable",
                      G_CALLBACK (ctk_tree_view_column_add_editable_callback),
                      column);
  priv->remove_editable_signal =
    g_signal_connect (priv->cell_area, "remove-editable",
                      G_CALLBACK (ctk_tree_view_column_remove_editable_callback),
                      column);

  priv->cell_area_context = ctk_cell_area_create_context (priv->cell_area);

  priv->context_changed_signal =
    g_signal_connect (priv->cell_area_context, "notify",
                      G_CALLBACK (ctk_tree_view_column_context_changed),
                      column);
}

static CtkCellArea *
ctk_tree_view_column_cell_layout_get_area (CtkCellLayout   *cell_layout)
{
  CtkTreeViewColumn        *column = CTK_TREE_VIEW_COLUMN (cell_layout);
  CtkTreeViewColumnPrivate *priv   = column->priv;

  if (G_UNLIKELY (!priv->cell_area))
    ctk_tree_view_column_ensure_cell_area (column, NULL);

  return priv->cell_area;
}

/* Button handling code
 */
static void
ctk_tree_view_column_create_button (CtkTreeViewColumn *tree_column)
{
  CtkTreeViewColumnPrivate *priv = tree_column->priv;
  CtkWidget *child;
  CtkWidget *hbox;

  g_return_if_fail (priv->button == NULL);

  priv->button = ctk_button_new ();
  g_object_ref_sink (priv->button);
  ctk_widget_set_focus_on_click (priv->button, FALSE);

  ctk_widget_show (priv->button);
  ctk_widget_add_events (priv->button, GDK_POINTER_MOTION_MASK);

  g_signal_connect (priv->button, "event",
		    G_CALLBACK (ctk_tree_view_column_button_event),
		    tree_column);
  g_signal_connect (priv->button, "clicked",
		    G_CALLBACK (ctk_tree_view_column_button_clicked),
		    tree_column);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  priv->alignment = ctk_alignment_new (priv->xalign, 0.5, 0.0, 0.0);
G_GNUC_END_IGNORE_DEPRECATIONS

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 2);
  priv->arrow = ctk_image_new_from_icon_name ("pan-down-symbolic", CTK_ICON_SIZE_BUTTON);

  if (priv->child)
    child = priv->child;
  else
    {
      child = ctk_label_new (priv->title);
      ctk_widget_show (child);
    }

  g_signal_connect (child, "mnemonic-activate",
		    G_CALLBACK (ctk_tree_view_column_mnemonic_activate),
		    tree_column);

  if (priv->xalign <= 0.5)
    {
      ctk_box_pack_start (CTK_BOX (hbox), priv->alignment, TRUE, TRUE, 0);
      ctk_box_pack_start (CTK_BOX (hbox), priv->arrow, FALSE, FALSE, 0);
    }
  else
    {
      ctk_box_pack_start (CTK_BOX (hbox), priv->arrow, FALSE, FALSE, 0);
      ctk_box_pack_start (CTK_BOX (hbox), priv->alignment, TRUE, TRUE, 0);
    }

  ctk_container_add (CTK_CONTAINER (priv->alignment), child);
  ctk_container_add (CTK_CONTAINER (priv->button), hbox);

  ctk_widget_show (hbox);
  ctk_widget_show (priv->alignment);
}

static void 
ctk_tree_view_column_update_button (CtkTreeViewColumn *tree_column)
{
  CtkTreeViewColumnPrivate *priv = tree_column->priv;
  gint sort_column_id = -1;
  CtkWidget *hbox;
  CtkWidget *alignment;
  CtkWidget *arrow;
  CtkWidget *current_child;
  const gchar *icon_name = "missing-image";
  CtkTreeModel *model;

  if (priv->tree_view)
    model = ctk_tree_view_get_model (CTK_TREE_VIEW (priv->tree_view));
  else
    model = NULL;

  hbox = ctk_bin_get_child (CTK_BIN (priv->button));
  alignment = priv->alignment;
  arrow = priv->arrow;
  current_child = ctk_bin_get_child (CTK_BIN (alignment));

  /* Set up the actual button */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  ctk_alignment_set (CTK_ALIGNMENT (alignment), priv->xalign, 0.5, 0.0, 0.0);
G_GNUC_END_IGNORE_DEPRECATIONS
      
  if (priv->child)
    {
      if (current_child != priv->child)
	{
	  ctk_container_remove (CTK_CONTAINER (alignment),
				current_child);
	  ctk_container_add (CTK_CONTAINER (alignment),
			     priv->child);
	}
    }
  else 
    {
      if (current_child == NULL)
	{
	  current_child = ctk_label_new (NULL);
	  ctk_widget_show (current_child);
	  ctk_container_add (CTK_CONTAINER (alignment),
			     current_child);
	}

      g_return_if_fail (CTK_IS_LABEL (current_child));

      if (priv->title)
	ctk_label_set_text_with_mnemonic (CTK_LABEL (current_child),
					  priv->title);
      else
	ctk_label_set_text_with_mnemonic (CTK_LABEL (current_child),
					  "");
    }

  if (CTK_IS_TREE_SORTABLE (model))
    ctk_tree_sortable_get_sort_column_id (CTK_TREE_SORTABLE (model),
					  &sort_column_id,
					  NULL);

  if (priv->show_sort_indicator)
    {
      gboolean alternative;

      if (priv->tree_view)
        g_object_get (ctk_widget_get_settings (priv->tree_view),
                      "ctk-alternative-sort-arrows", &alternative,
                      NULL);
      else
        alternative = FALSE;

      switch (priv->sort_order)
        {
	  case CTK_SORT_ASCENDING:
            icon_name = alternative ? "pan-up-symbolic" : "pan-down-symbolic";
	    break;

	  case CTK_SORT_DESCENDING:
            icon_name = alternative ? "pan-down-symbolic" : "pan-up-symbolic";
	    break;

	  default:
	    g_warning (G_STRLOC": bad sort order");
	    break;
	}
    }

  ctk_image_set_from_icon_name (CTK_IMAGE (arrow), icon_name, CTK_ICON_SIZE_BUTTON);

  /* Put arrow on the right if the text is left-or-center justified, and on the
   * left otherwise; do this by packing boxes, so flipping text direction will
   * reverse things
   */
  if (priv->xalign <= 0.5)
    ctk_box_reorder_child (CTK_BOX (hbox), arrow, 1);
  else
    ctk_box_reorder_child (CTK_BOX (hbox), arrow, 0);

  if (priv->show_sort_indicator
      || (CTK_IS_TREE_SORTABLE (model) && priv->sort_column_id >= 0))
    ctk_widget_show (arrow);
  else
    ctk_widget_hide (arrow);

  if (priv->show_sort_indicator)
    ctk_widget_set_opacity (arrow, 1.0);
  else
    ctk_widget_set_opacity (arrow, 0.0);

  /* It's always safe to hide the button.  It isn't always safe to show it, as
   * if you show it before it's realized, it'll get the wrong window. */
  if (priv->tree_view != NULL &&
      ctk_widget_get_realized (priv->tree_view))
    {
      if (priv->visible &&
          cdk_window_is_visible (_ctk_tree_view_get_header_window (CTK_TREE_VIEW (priv->tree_view))))
	{
          ctk_widget_show (priv->button);

	  if (priv->window)
	    {
	      if (priv->resizable)
		{
		  cdk_window_show (priv->window);
		  cdk_window_raise (priv->window);
		}
	      else
		{
		  cdk_window_hide (priv->window);
		}
	    }
	}
      else
	{
	  ctk_widget_hide (priv->button);
	  if (priv->window)
	    cdk_window_hide (priv->window);
	}
    }
  
  if (priv->reorderable || priv->clickable)
    {
      ctk_widget_set_can_focus (priv->button, TRUE);
    }
  else
    {
      ctk_widget_set_can_focus (priv->button, FALSE);
      if (ctk_widget_has_focus (priv->button))
	{
	  CtkWidget *toplevel = ctk_widget_get_toplevel (priv->tree_view);
	  if (ctk_widget_is_toplevel (toplevel))
	    {
	      ctk_window_set_focus (CTK_WINDOW (toplevel), NULL);
	    }
	}
    }
  /* Queue a resize on the assumption that we always want to catch all changes
   * and columns don't change all that often.
   */
  if (priv->tree_view && ctk_widget_get_realized (priv->tree_view))
     ctk_widget_queue_resize (priv->tree_view);
}

/* Button signal handlers
 */

static gint
ctk_tree_view_column_button_event (CtkWidget *widget,
				   CdkEvent  *event,
				   gpointer   data)
{
  CtkTreeViewColumn        *column = (CtkTreeViewColumn *) data;
  CtkTreeViewColumnPrivate *priv   = column->priv;

  g_return_val_if_fail (event != NULL, FALSE);

  if (event->type == GDK_BUTTON_PRESS &&
      priv->reorderable &&
      ((CdkEventButton *)event)->button == GDK_BUTTON_PRIMARY)
    {
      priv->maybe_reordered = TRUE;
      priv->drag_x = event->button.x;
      priv->drag_y = event->button.y;
      ctk_widget_grab_focus (widget);
    }

  if (event->type == GDK_BUTTON_RELEASE ||
      event->type == GDK_LEAVE_NOTIFY)
    priv->maybe_reordered = FALSE;
  
  if (event->type == GDK_MOTION_NOTIFY &&
      priv->maybe_reordered &&
      (ctk_drag_check_threshold (widget,
				 priv->drag_x,
				 priv->drag_y,
				 (gint) ((CdkEventMotion *)event)->x,
				 (gint) ((CdkEventMotion *)event)->y)))
    {
      priv->maybe_reordered = FALSE;
      _ctk_tree_view_column_start_drag (CTK_TREE_VIEW (priv->tree_view), column,
                                        event->motion.device);
      return TRUE;
    }

  if (priv->clickable == FALSE)
    {
      switch (event->type)
	{
	case GDK_BUTTON_PRESS:
	case GDK_2BUTTON_PRESS:
	case GDK_3BUTTON_PRESS:
	case GDK_MOTION_NOTIFY:
	case GDK_BUTTON_RELEASE:
	case GDK_ENTER_NOTIFY:
	case GDK_LEAVE_NOTIFY:
	  return TRUE;
	default:
	  return FALSE;
	}
    }
  return FALSE;
}


static void
ctk_tree_view_column_button_clicked (CtkWidget *widget, gpointer data)
{
  g_signal_emit_by_name (data, "clicked");
}

static gboolean
ctk_tree_view_column_mnemonic_activate (CtkWidget *widget,
					gboolean   group_cycling,
					gpointer   data)
{
  CtkTreeViewColumn        *column = (CtkTreeViewColumn *)data;
  CtkTreeViewColumnPrivate *priv   = column->priv;

  g_return_val_if_fail (CTK_IS_TREE_VIEW_COLUMN (column), FALSE);

  _ctk_tree_view_set_focus_column (CTK_TREE_VIEW (priv->tree_view), column);

  if (priv->clickable)
    ctk_button_clicked (CTK_BUTTON (priv->button));
  else if (ctk_widget_get_can_focus (priv->button))
    ctk_widget_grab_focus (priv->button);
  else
    ctk_widget_grab_focus (priv->tree_view);

  return TRUE;
}

static void
ctk_tree_view_model_sort_column_changed (CtkTreeSortable   *sortable,
					 CtkTreeViewColumn *column)
{
  CtkTreeViewColumnPrivate *priv = column->priv;
  gint sort_column_id;
  CtkSortType order;

  if (ctk_tree_sortable_get_sort_column_id (sortable,
					    &sort_column_id,
					    &order))
    {
      if (sort_column_id == priv->sort_column_id)
	{
	  ctk_tree_view_column_set_sort_indicator (column, TRUE);
	  ctk_tree_view_column_set_sort_order (column, order);
	}
      else
	{
	  ctk_tree_view_column_set_sort_indicator (column, FALSE);
	}
    }
  else
    {
      ctk_tree_view_column_set_sort_indicator (column, FALSE);
    }
}

static void
ctk_tree_view_column_sort (CtkTreeViewColumn *tree_column,
			   gpointer           data)
{
  CtkTreeViewColumnPrivate *priv = tree_column->priv;
  CtkTreeModel *model;
  CtkTreeSortable *sortable;
  gint sort_column_id;
  CtkSortType order;
  gboolean has_sort_column;
  gboolean has_default_sort_func;

  g_return_if_fail (priv->tree_view != NULL);

  model    = ctk_tree_view_get_model (CTK_TREE_VIEW (priv->tree_view));
  sortable = CTK_TREE_SORTABLE (model);

  has_sort_column =
    ctk_tree_sortable_get_sort_column_id (sortable,
					  &sort_column_id,
					  &order);
  has_default_sort_func =
    ctk_tree_sortable_has_default_sort_func (sortable);

  if (has_sort_column &&
      sort_column_id == priv->sort_column_id)
    {
      if (order == CTK_SORT_ASCENDING)
	ctk_tree_sortable_set_sort_column_id (sortable,
					      priv->sort_column_id,
					      CTK_SORT_DESCENDING);
      else if (order == CTK_SORT_DESCENDING && has_default_sort_func)
	ctk_tree_sortable_set_sort_column_id (sortable,
					      CTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
					      CTK_SORT_ASCENDING);
      else
	ctk_tree_sortable_set_sort_column_id (sortable,
					      priv->sort_column_id,
					      CTK_SORT_ASCENDING);
    }
  else
    {
      ctk_tree_sortable_set_sort_column_id (sortable,
					    priv->sort_column_id,
					    CTK_SORT_ASCENDING);
    }
}

static void
ctk_tree_view_column_setup_sort_column_id_callback (CtkTreeViewColumn *tree_column)
{
  CtkTreeViewColumnPrivate *priv = tree_column->priv;
  CtkTreeModel *model;

  if (priv->tree_view == NULL)
    return;

  model = ctk_tree_view_get_model (CTK_TREE_VIEW (priv->tree_view));

  if (model == NULL)
    return;

  if (CTK_IS_TREE_SORTABLE (model) &&
      priv->sort_column_id != -1)
    {
      gint real_sort_column_id;
      CtkSortType real_order;

      if (priv->sort_column_changed_signal == 0)
        priv->sort_column_changed_signal =
	  g_signal_connect (model, "sort-column-changed",
			    G_CALLBACK (ctk_tree_view_model_sort_column_changed),
			    tree_column);

      if (ctk_tree_sortable_get_sort_column_id (CTK_TREE_SORTABLE (model),
						&real_sort_column_id,
						&real_order) &&
	  (real_sort_column_id == priv->sort_column_id))
	{
	  ctk_tree_view_column_set_sort_indicator (tree_column, TRUE);
	  ctk_tree_view_column_set_sort_order (tree_column, real_order);
 	}
      else 
	{
	  ctk_tree_view_column_set_sort_indicator (tree_column, FALSE);
	}
   }
}

static void
ctk_tree_view_column_context_changed  (CtkCellAreaContext      *context,
				       GParamSpec              *pspec,
				       CtkTreeViewColumn       *tree_column)
{
  /* Here we want the column re-requested if the underlying context was
   * actually reset for any reason, this can happen if the underlying
   * area/cell configuration changes (i.e. cell packing properties
   * or cell spacing and the like) 
   *
   * Note that we block this handler while requesting for sizes
   * so there is no need to check for the new context size being -1,
   * we also block the handler when explicitly resetting the context
   * so as to avoid some infinite stack recursion.
   */
  if (!strcmp (pspec->name, "minimum-width") ||
      !strcmp (pspec->name, "natural-width") ||
      !strcmp (pspec->name, "minimum-height") ||
      !strcmp (pspec->name, "natural-height"))
    _ctk_tree_view_column_cell_set_dirty (tree_column, TRUE);
}

static void
ctk_tree_view_column_add_editable_callback (CtkCellArea       *area,
                                            CtkCellRenderer   *renderer,
                                            CtkCellEditable   *edit_widget,
                                            CdkRectangle      *cell_area,
                                            const gchar       *path_string,
                                            gpointer           user_data)
{
  CtkTreeViewColumn        *column = user_data;
  CtkTreeViewColumnPrivate *priv   = column->priv;
  CtkTreePath              *path;

  if (priv->tree_view)
    {
      path = ctk_tree_path_new_from_string (path_string);
      
      _ctk_tree_view_add_editable (CTK_TREE_VIEW (priv->tree_view),
				   column,
				   path,
				   edit_widget,
				   cell_area);
      
      ctk_tree_path_free (path);
    }
}

static void
ctk_tree_view_column_remove_editable_callback (CtkCellArea     *area,
                                               CtkCellRenderer *renderer,
                                               CtkCellEditable *edit_widget,
                                               gpointer         user_data)
{
  CtkTreeViewColumn        *column = user_data;
  CtkTreeViewColumnPrivate *priv   = column->priv;

  if (priv->tree_view)
    _ctk_tree_view_remove_editable (CTK_TREE_VIEW (priv->tree_view),
				    column,
				    edit_widget);
}

/* Exported Private Functions.
 * These should only be called by ctktreeview.c or ctktreeviewcolumn.c
 */
void
_ctk_tree_view_column_realize_button (CtkTreeViewColumn *column)
{
  CtkTreeViewColumnPrivate *priv = column->priv;
  CtkAllocation allocation;
  CtkTreeView *tree_view;
  CdkWindowAttr attr;
  guint attributes_mask;
  gboolean rtl;
  CdkDisplay *display;

  tree_view = (CtkTreeView *)priv->tree_view;
  rtl       = (ctk_widget_get_direction (priv->tree_view) == CTK_TEXT_DIR_RTL);

  g_return_if_fail (CTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (ctk_widget_get_realized (priv->tree_view));
  g_return_if_fail (priv->button != NULL);

  g_return_if_fail (_ctk_tree_view_get_header_window (tree_view) != NULL);
  ctk_widget_set_parent_window (priv->button, _ctk_tree_view_get_header_window (tree_view));

  attr.window_type = GDK_WINDOW_CHILD;
  attr.wclass = GDK_INPUT_ONLY;
  attr.visual = ctk_widget_get_visual (CTK_WIDGET (tree_view));
  attr.event_mask = ctk_widget_get_events (CTK_WIDGET (tree_view)) |
                    (GDK_BUTTON_PRESS_MASK |
		     GDK_BUTTON_RELEASE_MASK |
		     GDK_POINTER_MOTION_MASK |
		     GDK_KEY_PRESS_MASK);
  attributes_mask = GDK_WA_CURSOR | GDK_WA_X | GDK_WA_Y;
  display = cdk_window_get_display (_ctk_tree_view_get_header_window (tree_view));
  attr.cursor = cdk_cursor_new_from_name (display, "col-resize");
  attr.y = 0;
  attr.width = TREE_VIEW_DRAG_WIDTH;
  attr.height = _ctk_tree_view_get_header_height (tree_view);

  ctk_widget_get_allocation (priv->button, &allocation);
  attr.x       = (allocation.x + (rtl ? 0 : allocation.width)) - TREE_VIEW_DRAG_WIDTH / 2;
  priv->window = cdk_window_new (_ctk_tree_view_get_header_window (tree_view),
				 &attr, attributes_mask);
  ctk_widget_register_window (CTK_WIDGET (tree_view), priv->window);

  ctk_tree_view_column_update_button (column);

  g_clear_object (&attr.cursor);
}

void
_ctk_tree_view_column_unrealize_button (CtkTreeViewColumn *column)
{
  CtkTreeViewColumnPrivate *priv;

  g_return_if_fail (column != NULL);

  priv = column->priv;
  g_return_if_fail (priv->window != NULL);

  ctk_widget_unregister_window (CTK_WIDGET (priv->tree_view), priv->window);
  cdk_window_destroy (priv->window);
  priv->window = NULL;
}

void
_ctk_tree_view_column_unset_model (CtkTreeViewColumn *column,
				   CtkTreeModel      *old_model)
{
  CtkTreeViewColumnPrivate *priv = column->priv;

  if (priv->sort_column_changed_signal)
    {
      g_signal_handler_disconnect (old_model,
				   priv->sort_column_changed_signal);
      priv->sort_column_changed_signal = 0;
    }
  ctk_tree_view_column_set_sort_indicator (column, FALSE);
}

void
_ctk_tree_view_column_set_tree_view (CtkTreeViewColumn *column,
				     CtkTreeView       *tree_view)
{
  CtkTreeViewColumnPrivate *priv = column->priv;

  g_assert (priv->tree_view == NULL);

  priv->tree_view = CTK_WIDGET (tree_view);

  /* make sure we own a reference to it as well. */
  if (_ctk_tree_view_get_header_window (tree_view))
    ctk_widget_set_parent_window (priv->button, _ctk_tree_view_get_header_window (tree_view));

  ctk_widget_set_parent (priv->button, CTK_WIDGET (tree_view));

  priv->property_changed_signal =
    g_signal_connect_swapped (tree_view,
			      "notify::model",
			      G_CALLBACK (ctk_tree_view_column_setup_sort_column_id_callback),
			      column);

  ctk_tree_view_column_setup_sort_column_id_callback (column);
}

void
_ctk_tree_view_column_unset_tree_view (CtkTreeViewColumn *column)
{
  CtkTreeViewColumnPrivate *priv = column->priv;

  if (priv->tree_view == NULL)
    return;

  ctk_container_remove (CTK_CONTAINER (priv->tree_view), priv->button);

  if (priv->property_changed_signal)
    {
      g_signal_handler_disconnect (priv->tree_view, priv->property_changed_signal);
      priv->property_changed_signal = 0;
    }

  if (priv->sort_column_changed_signal)
    {
      g_signal_handler_disconnect (ctk_tree_view_get_model (CTK_TREE_VIEW (priv->tree_view)),
                                   priv->sort_column_changed_signal);
      priv->sort_column_changed_signal = 0;
    }

  priv->tree_view = NULL;
}

gboolean
_ctk_tree_view_column_has_editable_cell (CtkTreeViewColumn *column)
{
  CtkTreeViewColumnPrivate *priv = column->priv;
  gboolean ret = FALSE;
  GList *list, *cells;

  cells = ctk_cell_layout_get_cells (CTK_CELL_LAYOUT (priv->cell_area));

  for (list = cells; list; list = list->next)
    {
      CtkCellRenderer *cell = list->data;
      CtkCellRendererMode mode;

      g_object_get (cell, "mode", &mode, NULL);

      if (mode == CTK_CELL_RENDERER_MODE_EDITABLE)
        {
          ret = TRUE;
          break;
        }
    }

  g_list_free (cells);

  return ret;
}

/* gets cell being edited */
CtkCellRenderer *
_ctk_tree_view_column_get_edited_cell (CtkTreeViewColumn *column)
{
  CtkTreeViewColumnPrivate *priv = column->priv;

  return ctk_cell_area_get_edited_cell (priv->cell_area);
}

CtkCellRenderer *
_ctk_tree_view_column_get_cell_at_pos (CtkTreeViewColumn *column,
                                       CdkRectangle      *cell_area,
                                       CdkRectangle      *background_area,
                                       gint               x,
                                       gint               y)
{
  CtkCellRenderer *match = NULL;
  CtkTreeViewColumnPrivate *priv = column->priv;

  /* If (x, y) is outside of the background area, immediately return */
  if (x < background_area->x ||
      x > background_area->x + background_area->width ||
      y < background_area->y ||
      y > background_area->y + background_area->height)
    return NULL;

  /* If (x, y) is inside the background area, clamp it to the cell_area
   * so that a cell is still returned.  The main reason for doing this
   * (on the x axis) is for handling clicks in the indentation area
   * (either at the left or right depending on RTL setting).  Another
   * reason is for handling clicks on the area where the focus rectangle
   * is drawn (this is outside of cell area), this manifests itself
   * mainly when a large setting is used for focus-line-width.
   */
  if (x < cell_area->x)
    x = cell_area->x;
  else if (x > cell_area->x + cell_area->width)
    x = cell_area->x + cell_area->width;

  if (y < cell_area->y)
    y = cell_area->y;
  else if (y > cell_area->y + cell_area->height)
    y = cell_area->y + cell_area->height;

  match = ctk_cell_area_get_cell_at_position (priv->cell_area,
                                              priv->cell_area_context,
                                              priv->tree_view,
                                              cell_area,
                                              x, y,
                                              NULL);

  return match;
}

gboolean
_ctk_tree_view_column_is_blank_at_pos (CtkTreeViewColumn *column,
                                       CdkRectangle      *cell_area,
                                       CdkRectangle      *background_area,
                                       gint               x,
                                       gint               y)
{
  CtkCellRenderer *match;
  CdkRectangle cell_alloc, aligned_area, inner_area;
  CtkTreeViewColumnPrivate *priv = column->priv;

  match = _ctk_tree_view_column_get_cell_at_pos (column,
                                                 cell_area,
                                                 background_area,
                                                 x, y);
  if (!match)
    return FALSE;

  ctk_cell_area_get_cell_allocation (priv->cell_area,
                                     priv->cell_area_context,
                                     priv->tree_view,
                                     match,
                                     cell_area,
                                     &cell_alloc);

  ctk_cell_area_inner_cell_area (priv->cell_area, priv->tree_view,
                                 &cell_alloc, &inner_area);
  ctk_cell_renderer_get_aligned_area (match, priv->tree_view, 0,
                                      &inner_area, &aligned_area);

  if (x < aligned_area.x ||
      x > aligned_area.x + aligned_area.width ||
      y < aligned_area.y ||
      y > aligned_area.y + aligned_area.height)
    return TRUE;

  return FALSE;
}

/* Public Functions */


/**
 * ctk_tree_view_column_new:
 * 
 * Creates a new #CtkTreeViewColumn.
 * 
 * Returns: A newly created #CtkTreeViewColumn.
 **/
CtkTreeViewColumn *
ctk_tree_view_column_new (void)
{
  CtkTreeViewColumn *tree_column;

  tree_column = g_object_new (CTK_TYPE_TREE_VIEW_COLUMN, NULL);

  return tree_column;
}

/**
 * ctk_tree_view_column_new_with_area:
 * @area: the #CtkCellArea that the newly created column should use to layout cells.
 * 
 * Creates a new #CtkTreeViewColumn using @area to render its cells.
 * 
 * Returns: A newly created #CtkTreeViewColumn.
 *
 * Since: 3.0
 */
CtkTreeViewColumn *
ctk_tree_view_column_new_with_area (CtkCellArea *area)
{
  CtkTreeViewColumn *tree_column;

  tree_column = g_object_new (CTK_TYPE_TREE_VIEW_COLUMN, "cell-area", area, NULL);

  return tree_column;
}


/**
 * ctk_tree_view_column_new_with_attributes:
 * @title: The title to set the header to
 * @cell: The #CtkCellRenderer
 * @...: A %NULL-terminated list of attributes
 *
 * Creates a new #CtkTreeViewColumn with a number of default values.
 * This is equivalent to calling ctk_tree_view_column_set_title(),
 * ctk_tree_view_column_pack_start(), and
 * ctk_tree_view_column_set_attributes() on the newly created #CtkTreeViewColumn.
 *
 * Here’s a simple example:
 * |[<!-- language="C" -->
 *  enum { TEXT_COLUMN, COLOR_COLUMN, N_COLUMNS };
 *  // ...
 *  {
 *    CtkTreeViewColumn *column;
 *    CtkCellRenderer   *renderer = ctk_cell_renderer_text_new ();
 *  
 *    column = ctk_tree_view_column_new_with_attributes ("Title",
 *                                                       renderer,
 *                                                       "text", TEXT_COLUMN,
 *                                                       "foreground", COLOR_COLUMN,
 *                                                       NULL);
 *  }
 * ]|
 * 
 * Returns: A newly created #CtkTreeViewColumn.
 **/
CtkTreeViewColumn *
ctk_tree_view_column_new_with_attributes (const gchar     *title,
					  CtkCellRenderer *cell,
					  ...)
{
  CtkTreeViewColumn *retval;
  va_list args;

  retval = ctk_tree_view_column_new ();

  ctk_tree_view_column_set_title (retval, title);
  ctk_tree_view_column_pack_start (retval, cell, TRUE);

  va_start (args, cell);
  ctk_tree_view_column_set_attributesv (retval, cell, args);
  va_end (args);

  return retval;
}

/**
 * ctk_tree_view_column_pack_start:
 * @tree_column: A #CtkTreeViewColumn.
 * @cell: The #CtkCellRenderer. 
 * @expand: %TRUE if @cell is to be given extra space allocated to @tree_column.
 *
 * Packs the @cell into the beginning of the column. If @expand is %FALSE, then
 * the @cell is allocated no more space than it needs. Any unused space is divided
 * evenly between cells for which @expand is %TRUE.
 **/
void
ctk_tree_view_column_pack_start (CtkTreeViewColumn *tree_column,
				 CtkCellRenderer   *cell,
				 gboolean           expand)
{
  ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (tree_column), cell, expand);
}

/**
 * ctk_tree_view_column_pack_end:
 * @tree_column: A #CtkTreeViewColumn.
 * @cell: The #CtkCellRenderer. 
 * @expand: %TRUE if @cell is to be given extra space allocated to @tree_column.
 *
 * Adds the @cell to end of the column. If @expand is %FALSE, then the @cell
 * is allocated no more space than it needs. Any unused space is divided
 * evenly between cells for which @expand is %TRUE.
 **/
void
ctk_tree_view_column_pack_end (CtkTreeViewColumn  *tree_column,
			       CtkCellRenderer    *cell,
			       gboolean            expand)
{
  ctk_cell_layout_pack_end (CTK_CELL_LAYOUT (tree_column), cell, expand);
}

/**
 * ctk_tree_view_column_clear:
 * @tree_column: A #CtkTreeViewColumn
 * 
 * Unsets all the mappings on all renderers on the @tree_column.
 **/
void
ctk_tree_view_column_clear (CtkTreeViewColumn *tree_column)
{
  ctk_cell_layout_clear (CTK_CELL_LAYOUT (tree_column));
}

/**
 * ctk_tree_view_column_add_attribute:
 * @tree_column: A #CtkTreeViewColumn.
 * @cell_renderer: the #CtkCellRenderer to set attributes on
 * @attribute: An attribute on the renderer
 * @column: The column position on the model to get the attribute from.
 * 
 * Adds an attribute mapping to the list in @tree_column.  The @column is the
 * column of the model to get a value from, and the @attribute is the
 * parameter on @cell_renderer to be set from the value. So for example
 * if column 2 of the model contains strings, you could have the
 * “text” attribute of a #CtkCellRendererText get its values from
 * column 2.
 **/
void
ctk_tree_view_column_add_attribute (CtkTreeViewColumn *tree_column,
				    CtkCellRenderer   *cell_renderer,
				    const gchar       *attribute,
				    gint               column)
{
  ctk_cell_layout_add_attribute (CTK_CELL_LAYOUT (tree_column),
                                 cell_renderer, attribute, column);
}

static void
ctk_tree_view_column_set_attributesv (CtkTreeViewColumn *tree_column,
				      CtkCellRenderer   *cell_renderer,
				      va_list            args)
{
  CtkTreeViewColumnPrivate *priv = tree_column->priv;
  gchar *attribute;
  gint column;

  attribute = va_arg (args, gchar *);

  ctk_cell_layout_clear_attributes (CTK_CELL_LAYOUT (priv->cell_area),
                                    cell_renderer);
  
  while (attribute != NULL)
    {
      column = va_arg (args, gint);
      ctk_cell_layout_add_attribute (CTK_CELL_LAYOUT (priv->cell_area),
                                     cell_renderer, attribute, column);
      attribute = va_arg (args, gchar *);
    }
}

/**
 * ctk_tree_view_column_set_attributes:
 * @tree_column: A #CtkTreeViewColumn
 * @cell_renderer: the #CtkCellRenderer we’re setting the attributes of
 * @...: A %NULL-terminated list of attributes
 *
 * Sets the attributes in the list as the attributes of @tree_column.
 * The attributes should be in attribute/column order, as in
 * ctk_tree_view_column_add_attribute(). All existing attributes
 * are removed, and replaced with the new attributes.
 */
void
ctk_tree_view_column_set_attributes (CtkTreeViewColumn *tree_column,
				     CtkCellRenderer   *cell_renderer,
				     ...)
{
  va_list args;

  g_return_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column));
  g_return_if_fail (CTK_IS_CELL_RENDERER (cell_renderer));

  va_start (args, cell_renderer);
  ctk_tree_view_column_set_attributesv (tree_column, cell_renderer, args);
  va_end (args);
}


/**
 * ctk_tree_view_column_set_cell_data_func:
 * @tree_column: A #CtkTreeViewColumn
 * @cell_renderer: A #CtkCellRenderer
 * @func: (allow-none): The #CtkTreeCellDataFunc to use. 
 * @func_data: (closure): The user data for @func.
 * @destroy: The destroy notification for @func_data
 * 
 * Sets the #CtkTreeCellDataFunc to use for the column.  This
 * function is used instead of the standard attributes mapping for
 * setting the column value, and should set the value of @tree_column's
 * cell renderer as appropriate.  @func may be %NULL to remove an
 * older one.
 **/
void
ctk_tree_view_column_set_cell_data_func (CtkTreeViewColumn   *tree_column,
					 CtkCellRenderer     *cell_renderer,
					 CtkTreeCellDataFunc  func,
					 gpointer             func_data,
					 GDestroyNotify       destroy)
{
  ctk_cell_layout_set_cell_data_func (CTK_CELL_LAYOUT (tree_column),
                                      cell_renderer,
                                      (CtkCellLayoutDataFunc)func,
                                      func_data, destroy);
}


/**
 * ctk_tree_view_column_clear_attributes:
 * @tree_column: a #CtkTreeViewColumn
 * @cell_renderer: a #CtkCellRenderer to clear the attribute mapping on.
 * 
 * Clears all existing attributes previously set with
 * ctk_tree_view_column_set_attributes().
 **/
void
ctk_tree_view_column_clear_attributes (CtkTreeViewColumn *tree_column,
				       CtkCellRenderer   *cell_renderer)
{
  ctk_cell_layout_clear_attributes (CTK_CELL_LAYOUT (tree_column),
                                    cell_renderer);
}

/**
 * ctk_tree_view_column_set_spacing:
 * @tree_column: A #CtkTreeViewColumn.
 * @spacing: distance between cell renderers in pixels.
 * 
 * Sets the spacing field of @tree_column, which is the number of pixels to
 * place between cell renderers packed into it.
 **/
void
ctk_tree_view_column_set_spacing (CtkTreeViewColumn *tree_column,
				  gint               spacing)
{
  CtkTreeViewColumnPrivate *priv;

  g_return_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column));
  g_return_if_fail (spacing >= 0);

  priv = tree_column->priv;

  if (ctk_cell_area_box_get_spacing (CTK_CELL_AREA_BOX (priv->cell_area)) != spacing)
    {
      ctk_cell_area_box_set_spacing (CTK_CELL_AREA_BOX (priv->cell_area), spacing);
      if (priv->tree_view)
        _ctk_tree_view_column_cell_set_dirty (tree_column, TRUE);
      g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_SPACING]);
    }
}

/**
 * ctk_tree_view_column_get_spacing:
 * @tree_column: A #CtkTreeViewColumn.
 * 
 * Returns the spacing of @tree_column.
 * 
 * Returns: the spacing of @tree_column.
 **/
gint
ctk_tree_view_column_get_spacing (CtkTreeViewColumn *tree_column)
{
  CtkTreeViewColumnPrivate *priv;

  g_return_val_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column), 0);

  priv = tree_column->priv;

  return ctk_cell_area_box_get_spacing (CTK_CELL_AREA_BOX (priv->cell_area));
}

/* Options for manipulating the columns */

/**
 * ctk_tree_view_column_set_visible:
 * @tree_column: A #CtkTreeViewColumn.
 * @visible: %TRUE if the @tree_column is visible.
 * 
 * Sets the visibility of @tree_column.
 */
void
ctk_tree_view_column_set_visible (CtkTreeViewColumn *tree_column,
                                  gboolean           visible)
{
  CtkTreeViewColumnPrivate *priv;

  g_return_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column));

  priv    = tree_column->priv;
  visible = !! visible;

  if (priv->visible == visible)
    return;

  priv->visible = visible;

  ctk_widget_set_visible (priv->button, visible);

  if (priv->visible)
    _ctk_tree_view_column_cell_set_dirty (tree_column, TRUE);

  if (priv->tree_view)
    {
      _ctk_tree_view_accessible_toggle_visibility (CTK_TREE_VIEW (priv->tree_view),
                                                   tree_column);
    }

  ctk_tree_view_column_update_button (tree_column);
  g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_VISIBLE]);
}

/**
 * ctk_tree_view_column_get_visible:
 * @tree_column: A #CtkTreeViewColumn.
 * 
 * Returns %TRUE if @tree_column is visible.
 * 
 * Returns: whether the column is visible or not.  If it is visible, then
 * the tree will show the column.
 **/
gboolean
ctk_tree_view_column_get_visible (CtkTreeViewColumn *tree_column)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column), FALSE);

  return tree_column->priv->visible;
}

/**
 * ctk_tree_view_column_set_resizable:
 * @tree_column: A #CtkTreeViewColumn
 * @resizable: %TRUE, if the column can be resized
 * 
 * If @resizable is %TRUE, then the user can explicitly resize the column by
 * grabbing the outer edge of the column button.  If resizable is %TRUE and
 * sizing mode of the column is #CTK_TREE_VIEW_COLUMN_AUTOSIZE, then the sizing
 * mode is changed to #CTK_TREE_VIEW_COLUMN_GROW_ONLY.
 **/
void
ctk_tree_view_column_set_resizable (CtkTreeViewColumn *tree_column,
				    gboolean           resizable)
{
  CtkTreeViewColumnPrivate *priv;

  g_return_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column));

  priv      = tree_column->priv;
  resizable = !! resizable;

  if (priv->resizable == resizable)
    return;

  priv->resizable = resizable;

  if (resizable && priv->column_type == CTK_TREE_VIEW_COLUMN_AUTOSIZE)
    ctk_tree_view_column_set_sizing (tree_column, CTK_TREE_VIEW_COLUMN_GROW_ONLY);

  ctk_tree_view_column_update_button (tree_column);

  g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_RESIZABLE]);
}

/**
 * ctk_tree_view_column_get_resizable:
 * @tree_column: A #CtkTreeViewColumn
 * 
 * Returns %TRUE if the @tree_column can be resized by the end user.
 * 
 * Returns: %TRUE, if the @tree_column can be resized.
 **/
gboolean
ctk_tree_view_column_get_resizable (CtkTreeViewColumn *tree_column)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column), FALSE);

  return tree_column->priv->resizable;
}


/**
 * ctk_tree_view_column_set_sizing:
 * @tree_column: A #CtkTreeViewColumn.
 * @type: The #CtkTreeViewColumnSizing.
 * 
 * Sets the growth behavior of @tree_column to @type.
 **/
void
ctk_tree_view_column_set_sizing (CtkTreeViewColumn       *tree_column,
                                 CtkTreeViewColumnSizing  type)
{
  CtkTreeViewColumnPrivate *priv;

  g_return_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column));

  priv      = tree_column->priv;

  if (type == priv->column_type)
    return;

  if (type == CTK_TREE_VIEW_COLUMN_AUTOSIZE)
    ctk_tree_view_column_set_resizable (tree_column, FALSE);

  priv->column_type = type;

  ctk_tree_view_column_update_button (tree_column);

  g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_SIZING]);
}

/**
 * ctk_tree_view_column_get_sizing:
 * @tree_column: A #CtkTreeViewColumn.
 * 
 * Returns the current type of @tree_column.
 * 
 * Returns: The type of @tree_column.
 **/
CtkTreeViewColumnSizing
ctk_tree_view_column_get_sizing (CtkTreeViewColumn *tree_column)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column), 0);

  return tree_column->priv->column_type;
}

/**
 * ctk_tree_view_column_get_width:
 * @tree_column: A #CtkTreeViewColumn.
 * 
 * Returns the current size of @tree_column in pixels.
 * 
 * Returns: The current width of @tree_column.
 **/
gint
ctk_tree_view_column_get_width (CtkTreeViewColumn *tree_column)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column), 0);

  return tree_column->priv->width;
}

/**
 * ctk_tree_view_column_get_x_offset:
 * @tree_column: A #CtkTreeViewColumn.
 * 
 * Returns the current X offset of @tree_column in pixels.
 * 
 * Returns: The current X offset of @tree_column.
 *
 * Since: 3.2
 */
gint
ctk_tree_view_column_get_x_offset (CtkTreeViewColumn *tree_column)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column), 0);

  return tree_column->priv->x_offset;
}

gint
_ctk_tree_view_column_request_width (CtkTreeViewColumn *tree_column)
{
  CtkTreeViewColumnPrivate *priv;
  gint real_requested_width;

  priv = tree_column->priv;

  if (priv->fixed_width != -1)
    {
      real_requested_width = priv->fixed_width;
    }
  else if (ctk_tree_view_get_headers_visible (CTK_TREE_VIEW (priv->tree_view)))
    {
      gint button_request;
      gint requested_width;

      ctk_cell_area_context_get_preferred_width (priv->cell_area_context, &requested_width, NULL);
      requested_width += priv->padding;

      ctk_widget_get_preferred_width (priv->button, &button_request, NULL);
      real_requested_width = MAX (requested_width, button_request);
    }
  else
    {
      gint requested_width;

      ctk_cell_area_context_get_preferred_width (priv->cell_area_context, &requested_width, NULL);
      requested_width += priv->padding;

      real_requested_width = requested_width;
      if (real_requested_width < 0)
        real_requested_width = 0;
    }

  if (priv->min_width != -1)
    real_requested_width = MAX (real_requested_width, priv->min_width);

  if (priv->max_width != -1)
    real_requested_width = MIN (real_requested_width, priv->max_width);

  return real_requested_width;
}

void
_ctk_tree_view_column_allocate (CtkTreeViewColumn *tree_column,
				int                x_offset,
				int                width)
{
  CtkTreeViewColumnPrivate *priv;
  gboolean                  rtl;
  CtkAllocation             allocation = { 0, 0, 0, 0 };

  g_return_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column));

  priv = tree_column->priv;

  if (priv->width != width)
    ctk_widget_queue_draw (priv->tree_view);

  priv->x_offset = x_offset;
  priv->width = width;

  ctk_cell_area_context_allocate (priv->cell_area_context, priv->width - priv->padding, -1);

  if (ctk_tree_view_get_headers_visible (CTK_TREE_VIEW (priv->tree_view)))
    {
      allocation.x      = x_offset;
      allocation.y      = 0;
      allocation.width  = width;
      allocation.height = _ctk_tree_view_get_header_height (CTK_TREE_VIEW (priv->tree_view));

      ctk_widget_size_allocate (priv->button, &allocation);
    }

  if (priv->window)
    {
      rtl = (ctk_widget_get_direction (priv->tree_view) == CTK_TEXT_DIR_RTL);
      cdk_window_move_resize (priv->window,
			      allocation.x + (rtl ? 0 : allocation.width) - TREE_VIEW_DRAG_WIDTH/2,
			      allocation.y,
			      TREE_VIEW_DRAG_WIDTH, allocation.height);
    }

  g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_X_OFFSET]);
  g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_WIDTH]);
}

/**
 * ctk_tree_view_column_set_fixed_width:
 * @tree_column: A #CtkTreeViewColumn.
 * @fixed_width: The new fixed width, in pixels, or -1.
 *
 * If @fixed_width is not -1, sets the fixed width of @tree_column; otherwise
 * unsets it.  The effective value of @fixed_width is clamped between the
 * minimum and maximum width of the column; however, the value stored in the
 * “fixed-width” property is not clamped.  If the column sizing is
 * #CTK_TREE_VIEW_COLUMN_GROW_ONLY or #CTK_TREE_VIEW_COLUMN_AUTOSIZE, setting
 * a fixed width overrides the automatically calculated width.  Note that
 * @fixed_width is only a hint to CTK+; the width actually allocated to the
 * column may be greater or less than requested.
 *
 * Along with “expand”, the “fixed-width” property changes when the column is
 * resized by the user.
 **/
void
ctk_tree_view_column_set_fixed_width (CtkTreeViewColumn *tree_column,
				      gint               fixed_width)
{
  CtkTreeViewColumnPrivate *priv;

  g_return_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column));
  g_return_if_fail (fixed_width >= -1);

  priv = tree_column->priv;

  if (priv->fixed_width != fixed_width)
    {
      priv->fixed_width = fixed_width;
      if (priv->visible &&
          priv->tree_view != NULL &&
          ctk_widget_get_realized (priv->tree_view))
        ctk_widget_queue_resize (priv->tree_view);

      g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_FIXED_WIDTH]);
    }
}

/**
 * ctk_tree_view_column_get_fixed_width:
 * @tree_column: A #CtkTreeViewColumn.
 *
 * Gets the fixed width of the column.  This may not be the actual displayed
 * width of the column; for that, use ctk_tree_view_column_get_width().
 *
 * Returns: The fixed width of the column.
 **/
gint
ctk_tree_view_column_get_fixed_width (CtkTreeViewColumn *tree_column)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column), 0);

  return tree_column->priv->fixed_width;
}

/**
 * ctk_tree_view_column_set_min_width:
 * @tree_column: A #CtkTreeViewColumn.
 * @min_width: The minimum width of the column in pixels, or -1.
 * 
 * Sets the minimum width of the @tree_column.  If @min_width is -1, then the
 * minimum width is unset.
 **/
void
ctk_tree_view_column_set_min_width (CtkTreeViewColumn *tree_column,
				    gint               min_width)
{
  CtkTreeViewColumnPrivate *priv;

  g_return_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column));
  g_return_if_fail (min_width >= -1);

  priv = tree_column->priv;

  if (min_width == priv->min_width)
    return;

  if (priv->visible &&
      priv->tree_view != NULL &&
      ctk_widget_get_realized (priv->tree_view))
    {
      if (min_width > priv->width)
	ctk_widget_queue_resize (priv->tree_view);
    }

  priv->min_width = min_width;
  g_object_freeze_notify (G_OBJECT (tree_column));
  if (priv->max_width != -1 && priv->max_width < min_width)
    {
      priv->max_width = min_width;
      g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_MAX_WIDTH]);
    }
  g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_MIN_WIDTH]);
  g_object_thaw_notify (G_OBJECT (tree_column));

  if (priv->column_type == CTK_TREE_VIEW_COLUMN_AUTOSIZE && priv->tree_view)
    _ctk_tree_view_column_autosize (CTK_TREE_VIEW (priv->tree_view),
				    tree_column);
}

/**
 * ctk_tree_view_column_get_min_width:
 * @tree_column: A #CtkTreeViewColumn.
 * 
 * Returns the minimum width in pixels of the @tree_column, or -1 if no minimum
 * width is set.
 * 
 * Returns: The minimum width of the @tree_column.
 **/
gint
ctk_tree_view_column_get_min_width (CtkTreeViewColumn *tree_column)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column), -1);

  return tree_column->priv->min_width;
}

/**
 * ctk_tree_view_column_set_max_width:
 * @tree_column: A #CtkTreeViewColumn.
 * @max_width: The maximum width of the column in pixels, or -1.
 * 
 * Sets the maximum width of the @tree_column.  If @max_width is -1, then the
 * maximum width is unset.  Note, the column can actually be wider than max
 * width if it’s the last column in a view.  In this case, the column expands to
 * fill any extra space.
 **/
void
ctk_tree_view_column_set_max_width (CtkTreeViewColumn *tree_column,
				    gint               max_width)
{
  CtkTreeViewColumnPrivate *priv;

  g_return_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column));
  g_return_if_fail (max_width >= -1);

  priv = tree_column->priv;

  if (max_width == priv->max_width)
    return;

  if (priv->visible &&
      priv->tree_view != NULL &&
      ctk_widget_get_realized (priv->tree_view))
    {
      if (max_width != -1 && max_width < priv->width)
	ctk_widget_queue_resize (priv->tree_view);
    }

  priv->max_width = max_width;
  g_object_freeze_notify (G_OBJECT (tree_column));
  if (max_width != -1 && max_width < priv->min_width)
    {
      priv->min_width = max_width;
      g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_MIN_WIDTH]);
    }
  g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_MAX_WIDTH]);
  g_object_thaw_notify (G_OBJECT (tree_column));

  if (priv->column_type == CTK_TREE_VIEW_COLUMN_AUTOSIZE && priv->tree_view)
    _ctk_tree_view_column_autosize (CTK_TREE_VIEW (priv->tree_view),
				    tree_column);
}

/**
 * ctk_tree_view_column_get_max_width:
 * @tree_column: A #CtkTreeViewColumn.
 * 
 * Returns the maximum width in pixels of the @tree_column, or -1 if no maximum
 * width is set.
 * 
 * Returns: The maximum width of the @tree_column.
 **/
gint
ctk_tree_view_column_get_max_width (CtkTreeViewColumn *tree_column)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column), -1);

  return tree_column->priv->max_width;
}

/**
 * ctk_tree_view_column_clicked:
 * @tree_column: a #CtkTreeViewColumn
 * 
 * Emits the “clicked” signal on the column.  This function will only work if
 * @tree_column is clickable.
 **/
void
ctk_tree_view_column_clicked (CtkTreeViewColumn *tree_column)
{
  CtkTreeViewColumnPrivate *priv;

  g_return_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column));

  priv = tree_column->priv;

  if (priv->visible &&
      priv->clickable)
    ctk_button_clicked (CTK_BUTTON (priv->button));
}

/**
 * ctk_tree_view_column_set_title:
 * @tree_column: A #CtkTreeViewColumn.
 * @title: The title of the @tree_column.
 * 
 * Sets the title of the @tree_column.  If a custom widget has been set, then
 * this value is ignored.
 **/
void
ctk_tree_view_column_set_title (CtkTreeViewColumn *tree_column,
				const gchar       *title)
{
  CtkTreeViewColumnPrivate *priv;
  gchar                    *new_title;

  g_return_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column));

  priv = tree_column->priv;

  new_title = g_strdup (title);
  g_free (priv->title);
  priv->title = new_title;

  ctk_tree_view_column_update_button (tree_column);
  g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_TITLE]);
}

/**
 * ctk_tree_view_column_get_title:
 * @tree_column: A #CtkTreeViewColumn.
 * 
 * Returns the title of the widget.
 * 
 * Returns: the title of the column. This string should not be
 * modified or freed.
 **/
const gchar *
ctk_tree_view_column_get_title (CtkTreeViewColumn *tree_column)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column), NULL);

  return tree_column->priv->title;
}

/**
 * ctk_tree_view_column_set_expand:
 * @tree_column: A #CtkTreeViewColumn.
 * @expand: %TRUE if the column should expand to fill available space.
 *
 * Sets the column to take available extra space.  This space is shared equally
 * amongst all columns that have the expand set to %TRUE.  If no column has this
 * option set, then the last column gets all extra space.  By default, every
 * column is created with this %FALSE.
 *
 * Along with “fixed-width”, the “expand” property changes when the column is
 * resized by the user.
 *
 * Since: 2.4
 **/
void
ctk_tree_view_column_set_expand (CtkTreeViewColumn *tree_column,
				 gboolean           expand)
{
  CtkTreeViewColumnPrivate *priv;

  g_return_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column));

  priv = tree_column->priv;

  expand = expand?TRUE:FALSE;
  if (priv->expand == expand)
    return;
  priv->expand = expand;

  if (priv->visible &&
      priv->tree_view != NULL &&
      ctk_widget_get_realized (priv->tree_view))
    {
      ctk_widget_queue_resize (priv->tree_view);
    }

  g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_EXPAND]);
}

/**
 * ctk_tree_view_column_get_expand:
 * @tree_column: A #CtkTreeViewColumn.
 *
 * Returns %TRUE if the column expands to fill available space.
 *
 * Returns: %TRUE if the column expands to fill available space.
 *
 * Since: 2.4
 **/
gboolean
ctk_tree_view_column_get_expand (CtkTreeViewColumn *tree_column)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column), FALSE);

  return tree_column->priv->expand;
}

/**
 * ctk_tree_view_column_set_clickable:
 * @tree_column: A #CtkTreeViewColumn.
 * @clickable: %TRUE if the header is active.
 * 
 * Sets the header to be active if @clickable is %TRUE.  When the header is
 * active, then it can take keyboard focus, and can be clicked.
 **/
void
ctk_tree_view_column_set_clickable (CtkTreeViewColumn *tree_column,
                                    gboolean           clickable)
{
  CtkTreeViewColumnPrivate *priv;

  g_return_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column));

  priv = tree_column->priv;

  clickable = !! clickable;
  if (priv->clickable == clickable)
    return;

  priv->clickable = clickable;
  ctk_tree_view_column_update_button (tree_column);
  g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_CLICKABLE]);
}

/**
 * ctk_tree_view_column_get_clickable:
 * @tree_column: a #CtkTreeViewColumn
 * 
 * Returns %TRUE if the user can click on the header for the column.
 * 
 * Returns: %TRUE if user can click the column header.
 **/
gboolean
ctk_tree_view_column_get_clickable (CtkTreeViewColumn *tree_column)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column), FALSE);

  return tree_column->priv->clickable;
}

/**
 * ctk_tree_view_column_set_widget:
 * @tree_column: A #CtkTreeViewColumn.
 * @widget: (allow-none): A child #CtkWidget, or %NULL.
 *
 * Sets the widget in the header to be @widget.  If widget is %NULL, then the
 * header button is set with a #CtkLabel set to the title of @tree_column.
 **/
void
ctk_tree_view_column_set_widget (CtkTreeViewColumn *tree_column,
				 CtkWidget         *widget)
{
  CtkTreeViewColumnPrivate *priv;

  g_return_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column));
  g_return_if_fail (widget == NULL || CTK_IS_WIDGET (widget));

  priv = tree_column->priv;

  if (widget)
    g_object_ref_sink (widget);

  if (priv->child)
    g_object_unref (priv->child);

  priv->child = widget;
  ctk_tree_view_column_update_button (tree_column);
  g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_WIDGET]);
}

/**
 * ctk_tree_view_column_get_widget:
 * @tree_column: A #CtkTreeViewColumn.
 *
 * Returns the #CtkWidget in the button on the column header.
 * If a custom widget has not been set then %NULL is returned.
 *
 * Returns: (nullable) (transfer none): The #CtkWidget in the column
 *     header, or %NULL
 **/
CtkWidget *
ctk_tree_view_column_get_widget (CtkTreeViewColumn *tree_column)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column), NULL);

  return tree_column->priv->child;
}

/**
 * ctk_tree_view_column_set_alignment:
 * @tree_column: A #CtkTreeViewColumn.
 * @xalign: The alignment, which is between [0.0 and 1.0] inclusive.
 * 
 * Sets the alignment of the title or custom widget inside the column header.
 * The alignment determines its location inside the button -- 0.0 for left, 0.5
 * for center, 1.0 for right.
 **/
void
ctk_tree_view_column_set_alignment (CtkTreeViewColumn *tree_column,
                                    gfloat             xalign)
{
  CtkTreeViewColumnPrivate *priv;

  g_return_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column));

  priv = tree_column->priv;

  xalign = CLAMP (xalign, 0.0, 1.0);

  if (priv->xalign == xalign)
    return;

  priv->xalign = xalign;
  ctk_tree_view_column_update_button (tree_column);
  g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_ALIGNMENT]);
}

/**
 * ctk_tree_view_column_get_alignment:
 * @tree_column: A #CtkTreeViewColumn.
 * 
 * Returns the current x alignment of @tree_column.  This value can range
 * between 0.0 and 1.0.
 * 
 * Returns: The current alignent of @tree_column.
 **/
gfloat
ctk_tree_view_column_get_alignment (CtkTreeViewColumn *tree_column)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column), 0.5);

  return tree_column->priv->xalign;
}

/**
 * ctk_tree_view_column_set_reorderable:
 * @tree_column: A #CtkTreeViewColumn
 * @reorderable: %TRUE, if the column can be reordered.
 * 
 * If @reorderable is %TRUE, then the column can be reordered by the end user
 * dragging the header.
 **/
void
ctk_tree_view_column_set_reorderable (CtkTreeViewColumn *tree_column,
				      gboolean           reorderable)
{
  CtkTreeViewColumnPrivate *priv;

  g_return_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column));

  priv = tree_column->priv;

  /*  if (reorderable)
      ctk_tree_view_column_set_clickable (tree_column, TRUE);*/

  if (priv->reorderable == (reorderable?TRUE:FALSE))
    return;

  priv->reorderable = (reorderable?TRUE:FALSE);
  ctk_tree_view_column_update_button (tree_column);
  g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_REORDERABLE]);
}

/**
 * ctk_tree_view_column_get_reorderable:
 * @tree_column: A #CtkTreeViewColumn
 * 
 * Returns %TRUE if the @tree_column can be reordered by the user.
 * 
 * Returns: %TRUE if the @tree_column can be reordered by the user.
 **/
gboolean
ctk_tree_view_column_get_reorderable (CtkTreeViewColumn *tree_column)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column), FALSE);

  return tree_column->priv->reorderable;
}


/**
 * ctk_tree_view_column_set_sort_column_id:
 * @tree_column: a #CtkTreeViewColumn
 * @sort_column_id: The @sort_column_id of the model to sort on.
 *
 * Sets the logical @sort_column_id that this column sorts on when this column 
 * is selected for sorting.  Doing so makes the column header clickable.
 **/
void
ctk_tree_view_column_set_sort_column_id (CtkTreeViewColumn *tree_column,
					 gint               sort_column_id)
{
  CtkTreeViewColumnPrivate *priv;

  g_return_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column));
  g_return_if_fail (sort_column_id >= -1);

  priv = tree_column->priv;

  if (priv->sort_column_id == sort_column_id)
    return;

  priv->sort_column_id = sort_column_id;

  /* Handle unsetting the id */
  if (sort_column_id == -1)
    {
      CtkTreeModel *model = ctk_tree_view_get_model (CTK_TREE_VIEW (priv->tree_view));

      if (priv->sort_clicked_signal)
	{
	  g_signal_handler_disconnect (tree_column, priv->sort_clicked_signal);
	  priv->sort_clicked_signal = 0;
	}

      if (priv->sort_column_changed_signal)
	{
	  g_signal_handler_disconnect (model, priv->sort_column_changed_signal);
	  priv->sort_column_changed_signal = 0;
	}

      ctk_tree_view_column_set_sort_order (tree_column, CTK_SORT_ASCENDING);
      ctk_tree_view_column_set_sort_indicator (tree_column, FALSE);
      ctk_tree_view_column_set_clickable (tree_column, FALSE);
      g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_SORT_COLUMN_ID]);
      return;
    }

  ctk_tree_view_column_set_clickable (tree_column, TRUE);

  if (! priv->sort_clicked_signal)
    priv->sort_clicked_signal = g_signal_connect (tree_column,
						  "clicked",
						  G_CALLBACK (ctk_tree_view_column_sort),
						  NULL);

  ctk_tree_view_column_setup_sort_column_id_callback (tree_column);
  g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_SORT_COLUMN_ID]);
}

/**
 * ctk_tree_view_column_get_sort_column_id:
 * @tree_column: a #CtkTreeViewColumn
 *
 * Gets the logical @sort_column_id that the model sorts on when this
 * column is selected for sorting.
 * See ctk_tree_view_column_set_sort_column_id().
 *
 * Returns: the current @sort_column_id for this column, or -1 if
 *               this column can’t be used for sorting.
 **/
gint
ctk_tree_view_column_get_sort_column_id (CtkTreeViewColumn *tree_column)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column), 0);

  return tree_column->priv->sort_column_id;
}

/**
 * ctk_tree_view_column_set_sort_indicator:
 * @tree_column: a #CtkTreeViewColumn
 * @setting: %TRUE to display an indicator that the column is sorted
 *
 * Call this function with a @setting of %TRUE to display an arrow in
 * the header button indicating the column is sorted. Call
 * ctk_tree_view_column_set_sort_order() to change the direction of
 * the arrow.
 * 
 **/
void
ctk_tree_view_column_set_sort_indicator (CtkTreeViewColumn     *tree_column,
                                         gboolean               setting)
{
  g_return_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column));

  setting = setting != FALSE;

  if (setting == tree_column->priv->show_sort_indicator)
    return;

  tree_column->priv->show_sort_indicator = setting;
  ctk_tree_view_column_update_button (tree_column);
  g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_SORT_INDICATOR]);
}

/**
 * ctk_tree_view_column_get_sort_indicator:
 * @tree_column: a #CtkTreeViewColumn
 * 
 * Gets the value set by ctk_tree_view_column_set_sort_indicator().
 * 
 * Returns: whether the sort indicator arrow is displayed
 **/
gboolean
ctk_tree_view_column_get_sort_indicator  (CtkTreeViewColumn     *tree_column)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column), FALSE);

  return tree_column->priv->show_sort_indicator;
}

/**
 * ctk_tree_view_column_set_sort_order:
 * @tree_column: a #CtkTreeViewColumn
 * @order: sort order that the sort indicator should indicate
 *
 * Changes the appearance of the sort indicator. 
 * 
 * This does not actually sort the model.  Use
 * ctk_tree_view_column_set_sort_column_id() if you want automatic sorting
 * support.  This function is primarily for custom sorting behavior, and should
 * be used in conjunction with ctk_tree_sortable_set_sort_column_id() to do
 * that. For custom models, the mechanism will vary. 
 * 
 * The sort indicator changes direction to indicate normal sort or reverse sort.
 * Note that you must have the sort indicator enabled to see anything when 
 * calling this function; see ctk_tree_view_column_set_sort_indicator().
 **/
void
ctk_tree_view_column_set_sort_order      (CtkTreeViewColumn     *tree_column,
                                          CtkSortType            order)
{
  g_return_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column));

  if (order == tree_column->priv->sort_order)
    return;

  tree_column->priv->sort_order = order;
  ctk_tree_view_column_update_button (tree_column);
  g_object_notify_by_pspec (G_OBJECT (tree_column), tree_column_props[PROP_SORT_ORDER]);
}

/**
 * ctk_tree_view_column_get_sort_order:
 * @tree_column: a #CtkTreeViewColumn
 * 
 * Gets the value set by ctk_tree_view_column_set_sort_order().
 * 
 * Returns: the sort order the sort indicator is indicating
 **/
CtkSortType
ctk_tree_view_column_get_sort_order      (CtkTreeViewColumn     *tree_column)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column), 0);

  return tree_column->priv->sort_order;
}

/**
 * ctk_tree_view_column_cell_set_cell_data:
 * @tree_column: A #CtkTreeViewColumn.
 * @tree_model: The #CtkTreeModel to to get the cell renderers attributes from.
 * @iter: The #CtkTreeIter to to get the cell renderer’s attributes from.
 * @is_expander: %TRUE, if the row has children
 * @is_expanded: %TRUE, if the row has visible children
 * 
 * Sets the cell renderer based on the @tree_model and @iter.  That is, for
 * every attribute mapping in @tree_column, it will get a value from the set
 * column on the @iter, and use that value to set the attribute on the cell
 * renderer.  This is used primarily by the #CtkTreeView.
 **/
void
ctk_tree_view_column_cell_set_cell_data (CtkTreeViewColumn *tree_column,
					 CtkTreeModel      *tree_model,
					 CtkTreeIter       *iter,
					 gboolean           is_expander,
					 gboolean           is_expanded)
{
  g_return_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column));

  if (tree_model == NULL)
    return;

  ctk_cell_area_apply_attributes (tree_column->priv->cell_area, tree_model, iter,
                                  is_expander, is_expanded);
}

/**
 * ctk_tree_view_column_cell_get_size:
 * @tree_column: A #CtkTreeViewColumn.
 * @cell_area: (allow-none): The area a cell in the column will be allocated, or %NULL
 * @x_offset: (out) (optional): location to return x offset of a cell relative to @cell_area, or %NULL
 * @y_offset: (out) (optional): location to return y offset of a cell relative to @cell_area, or %NULL
 * @width: (out) (optional): location to return width needed to render a cell, or %NULL
 * @height: (out) (optional): location to return height needed to render a cell, or %NULL
 * 
 * Obtains the width and height needed to render the column.  This is used
 * primarily by the #CtkTreeView.
 **/
void
ctk_tree_view_column_cell_get_size (CtkTreeViewColumn  *tree_column,
				    const CdkRectangle *cell_area,
				    gint               *x_offset,
				    gint               *y_offset,
				    gint               *width,
				    gint               *height)
{
  CtkTreeViewColumnPrivate *priv;
  gint min_width = 0, min_height = 0;

  g_return_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column));

  priv = tree_column->priv;

  g_signal_handler_block (priv->cell_area_context, 
			  priv->context_changed_signal);

  ctk_cell_area_get_preferred_width (priv->cell_area,
                                     priv->cell_area_context,
                                     priv->tree_view,
                                     NULL, NULL);

  ctk_cell_area_context_get_preferred_width (priv->cell_area_context, &min_width, NULL);

  ctk_cell_area_get_preferred_height_for_width (priv->cell_area,
                                                priv->cell_area_context,
                                                priv->tree_view,
                                                min_width,
                                                &min_height,
                                                NULL);

  g_signal_handler_unblock (priv->cell_area_context, 
			    priv->context_changed_signal);


  if (height)
    * height = min_height;
  if (width)
    * width = min_width;

}

/**
 * ctk_tree_view_column_cell_render:
 * @tree_column: A #CtkTreeViewColumn.
 * @cr: cairo context to draw to
 * @background_area: entire cell area (including tree expanders and maybe padding on the sides)
 * @cell_area: area normally rendered by a cell renderer
 * @flags: flags that affect rendering
 * 
 * Renders the cell contained by #tree_column. This is used primarily by the
 * #CtkTreeView.
 **/
void
_ctk_tree_view_column_cell_render (CtkTreeViewColumn  *tree_column,
				   cairo_t            *cr,
				   const CdkRectangle *background_area,
				   const CdkRectangle *cell_area,
				   guint               flags,
                                   gboolean            draw_focus)
{
  CtkTreeViewColumnPrivate *priv;

  g_return_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (background_area != NULL);
  g_return_if_fail (cell_area != NULL);

  priv = tree_column->priv;

  cairo_save (cr);

  ctk_cell_area_render (priv->cell_area, priv->cell_area_context,
                        priv->tree_view, cr,
                        background_area, cell_area, flags,
                        draw_focus);

  cairo_restore (cr);
}

gboolean
_ctk_tree_view_column_cell_event (CtkTreeViewColumn  *tree_column,
				  CdkEvent           *event,
				  const CdkRectangle *cell_area,
				  guint               flags)
{
  CtkTreeViewColumnPrivate *priv;

  g_return_val_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column), FALSE);

  priv = tree_column->priv;

  return ctk_cell_area_event (priv->cell_area,
                              priv->cell_area_context,
                              priv->tree_view,
                              event,
                              cell_area,
                              flags);
}

/**
 * ctk_tree_view_column_cell_is_visible:
 * @tree_column: A #CtkTreeViewColumn
 * 
 * Returns %TRUE if any of the cells packed into the @tree_column are visible.
 * For this to be meaningful, you must first initialize the cells with
 * ctk_tree_view_column_cell_set_cell_data()
 * 
 * Returns: %TRUE, if any of the cells packed into the @tree_column are currently visible
 **/
gboolean
ctk_tree_view_column_cell_is_visible (CtkTreeViewColumn *tree_column)
{
  GList *list;
  GList *cells;
  CtkTreeViewColumnPrivate *priv;

  g_return_val_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column), FALSE);

  priv = tree_column->priv;

  cells = ctk_cell_layout_get_cells (CTK_CELL_LAYOUT (priv->cell_area));
  for (list = cells; list; list = list->next)
    {
      if (ctk_cell_renderer_get_visible (list->data))
        {
          g_list_free (cells);
          return TRUE;
        }
    }

  g_list_free (cells);

  return FALSE;
}

/**
 * ctk_tree_view_column_focus_cell:
 * @tree_column: A #CtkTreeViewColumn
 * @cell: A #CtkCellRenderer
 *
 * Sets the current keyboard focus to be at @cell, if the column contains
 * 2 or more editable and activatable cells.
 *
 * Since: 2.2
 **/
void
ctk_tree_view_column_focus_cell (CtkTreeViewColumn *tree_column,
				 CtkCellRenderer   *cell)
{
  g_return_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column));
  g_return_if_fail (CTK_IS_CELL_RENDERER (cell));

  ctk_cell_area_set_focus_cell (tree_column->priv->cell_area, cell);
}

void
_ctk_tree_view_column_cell_set_dirty (CtkTreeViewColumn *tree_column,
				      gboolean           install_handler)
{
  CtkTreeViewColumnPrivate *priv = tree_column->priv;

  priv->dirty = TRUE;
  priv->padding = 0;
  priv->width = 0;

  /* Issue a manual reset on the context to have all
   * sizes re-requested for the context.
   */
  g_signal_handler_block (priv->cell_area_context, 
			  priv->context_changed_signal);
  ctk_cell_area_context_reset (priv->cell_area_context);
  g_signal_handler_unblock (priv->cell_area_context, 
			    priv->context_changed_signal);

  if (priv->tree_view &&
      ctk_widget_get_realized (priv->tree_view))
    {
      _ctk_tree_view_install_mark_rows_col_dirty (CTK_TREE_VIEW (priv->tree_view), install_handler);
      ctk_widget_queue_resize (priv->tree_view);
    }
}

gboolean
_ctk_tree_view_column_cell_get_dirty (CtkTreeViewColumn  *tree_column)
{
  return tree_column->priv->dirty;
}

/**
 * ctk_tree_view_column_cell_get_position:
 * @tree_column: a #CtkTreeViewColumn
 * @cell_renderer: a #CtkCellRenderer
 * @x_offset: (out) (allow-none): return location for the horizontal
 *            position of @cell within @tree_column, may be %NULL
 * @width: (out) (allow-none): return location for the width of @cell,
 *         may be %NULL
 *
 * Obtains the horizontal position and size of a cell in a column. If the
 * cell is not found in the column, @start_pos and @width are not changed and
 * %FALSE is returned.
 * 
 * Returns: %TRUE if @cell belongs to @tree_column.
 */
gboolean
ctk_tree_view_column_cell_get_position (CtkTreeViewColumn *tree_column,
					CtkCellRenderer   *cell_renderer,
					gint              *x_offset,
					gint              *width)
{
  CtkTreeViewColumnPrivate *priv;
  CdkRectangle cell_area;
  CdkRectangle allocation;

  g_return_val_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column), FALSE);
  g_return_val_if_fail (CTK_IS_CELL_RENDERER (cell_renderer), FALSE);

  priv = tree_column->priv;

  if (! ctk_cell_area_has_renderer (priv->cell_area, cell_renderer))
    return FALSE;

  ctk_tree_view_get_background_area (CTK_TREE_VIEW (priv->tree_view),
                                     NULL, tree_column, &cell_area);

  ctk_cell_area_get_cell_allocation (priv->cell_area,
                                     priv->cell_area_context,
                                     priv->tree_view,
                                     cell_renderer,
                                     &cell_area,
                                     &allocation);

  if (x_offset)
    *x_offset = allocation.x - cell_area.x;

  if (width)
    *width = allocation.width;

  return TRUE;
}

/**
 * ctk_tree_view_column_queue_resize:
 * @tree_column: A #CtkTreeViewColumn
 *
 * Flags the column, and the cell renderers added to this column, to have
 * their sizes renegotiated.
 *
 * Since: 2.8
 **/
void
ctk_tree_view_column_queue_resize (CtkTreeViewColumn *tree_column)
{
  g_return_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column));

  if (tree_column->priv->tree_view)
    _ctk_tree_view_column_cell_set_dirty (tree_column, TRUE);
}

/**
 * ctk_tree_view_column_get_tree_view:
 * @tree_column: A #CtkTreeViewColumn
 *
 * Returns the #CtkTreeView wherein @tree_column has been inserted.
 * If @column is currently not inserted in any tree view, %NULL is
 * returned.
 *
 * Returns: (nullable) (transfer none): The tree view wherein @column has
 *     been inserted if any, %NULL otherwise.
 *
 * Since: 2.12
 */
CtkWidget *
ctk_tree_view_column_get_tree_view (CtkTreeViewColumn *tree_column)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column), NULL);

  return tree_column->priv->tree_view;
}

/**
 * ctk_tree_view_column_get_button:
 * @tree_column: A #CtkTreeViewColumn
 *
 * Returns the button used in the treeview column header
 *
 * Returns: (transfer none): The button for the column header.
 *
 * Since: 3.0
 */
CtkWidget *
ctk_tree_view_column_get_button (CtkTreeViewColumn *tree_column)
{
  g_return_val_if_fail (CTK_IS_TREE_VIEW_COLUMN (tree_column), NULL);

  return tree_column->priv->button;
}

CdkWindow *
_ctk_tree_view_column_get_window (CtkTreeViewColumn  *column)
{
  return column->priv->window;
}

void
_ctk_tree_view_column_push_padding (CtkTreeViewColumn  *column,
				    gint                padding)
{
  column->priv->padding = MAX (column->priv->padding, padding);
}

gint
_ctk_tree_view_column_get_requested_width (CtkTreeViewColumn  *column)
{
  gint requested_width;

  ctk_cell_area_context_get_preferred_width (column->priv->cell_area_context, &requested_width, NULL);

  return requested_width + column->priv->padding;
}

gint
_ctk_tree_view_column_get_drag_x (CtkTreeViewColumn  *column)
{
  return column->priv->drag_x;
}

CtkCellAreaContext *
_ctk_tree_view_column_get_context (CtkTreeViewColumn  *column)
{
  return column->priv->cell_area_context;
}
