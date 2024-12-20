/* CTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/. 
 */

#include "config.h"

#define CDK_DISABLE_DEPRECATION_WARNINGS

#include "ctktable.h"

#include "ctktypebuiltins.h"
#include "ctkprivate.h"
#include "ctkintl.h"


/**
 * SECTION:ctktable
 * @Short_description: Pack widgets in regular patterns
 * @Title: CtkTable
 * @See_also: #CtkGrid
 *
 * The #CtkTable functions allow the programmer to arrange widgets in rows and
 * columns, making it easy to align many widgets next to each other,
 * horizontally and vertically.
 *
 * Tables are created with a call to ctk_table_new(), the size of which can
 * later be changed with ctk_table_resize().
 *
 * Widgets can be added to a table using ctk_table_attach() or the more
 * convenient (but slightly less flexible) ctk_table_attach_defaults().
 *
 * To alter the space next to a specific row, use ctk_table_set_row_spacing(),
 * and for a column, ctk_table_set_col_spacing().
 * The gaps between all rows or columns can be changed by
 * calling ctk_table_set_row_spacings() or ctk_table_set_col_spacings()
 * respectively. Note that spacing is added between the
 * children, while padding added by ctk_table_attach() is added on
 * either side of the widget it belongs to.
 *
 * ctk_table_set_homogeneous(), can be used to set whether all cells in the
 * table will resize themselves to the size of the largest widget in the table.
 *
 * > #CtkTable has been deprecated. Use #CtkGrid instead. It provides the same
 * > capabilities as CtkTable for arranging widgets in a rectangular grid, but
 * > does support height-for-width geometry management.
 */


struct _CtkTablePrivate
{
  CtkTableRowCol *cols;
  CtkTableRowCol *rows;

  GList          *children;

  guint16         column_spacing;
  guint16         ncols;
  guint16         nrows;
  guint16         row_spacing;

  guint           homogeneous : 1;
};

enum
{
  PROP_0,
  PROP_N_ROWS,
  PROP_N_COLUMNS,
  PROP_COLUMN_SPACING,
  PROP_ROW_SPACING,
  PROP_HOMOGENEOUS
};

enum
{
  CHILD_PROP_0,
  CHILD_PROP_LEFT_ATTACH,
  CHILD_PROP_RIGHT_ATTACH,
  CHILD_PROP_TOP_ATTACH,
  CHILD_PROP_BOTTOM_ATTACH,
  CHILD_PROP_X_OPTIONS,
  CHILD_PROP_Y_OPTIONS,
  CHILD_PROP_X_PADDING,
  CHILD_PROP_Y_PADDING
};
  

static void ctk_table_finalize	    (GObject	    *object);
static void ctk_table_get_preferred_width  (CtkWidget *widget,
                                            gint      *minimum,
                                            gint      *natural);
static void ctk_table_get_preferred_height (CtkWidget *widget,
                                            gint      *minimum,
                                            gint      *natural);
static void ctk_table_size_allocate (CtkWidget	    *widget,
				     CtkAllocation  *allocation);
static void ctk_table_add	    (CtkContainer   *container,
				     CtkWidget	    *widget);
static void ctk_table_remove	    (CtkContainer   *container,
				     CtkWidget	    *widget);
static void ctk_table_forall	    (CtkContainer   *container,
				     gboolean	     include_internals,
				     CtkCallback     callback,
				     gpointer	     callback_data);
static void ctk_table_get_property  (GObject         *object,
				     guint            prop_id,
				     GValue          *value,
				     GParamSpec      *pspec);
static void ctk_table_set_property  (GObject         *object,
				     guint            prop_id,
				     const GValue    *value,
				     GParamSpec      *pspec);
static void ctk_table_set_child_property (CtkContainer    *container,
					  CtkWidget       *child,
					  guint            property_id,
					  const GValue    *value,
					  GParamSpec      *pspec);
static void ctk_table_get_child_property (CtkContainer    *container,
					  CtkWidget       *child,
					  guint            property_id,
					  GValue          *value,
					  GParamSpec      *pspec);
static GType ctk_table_child_type   (CtkContainer   *container);


static void ctk_table_size_request_init	 (CtkTable *table);
static void ctk_table_size_request_pass1 (CtkTable *table);
static void ctk_table_size_request_pass2 (CtkTable *table);
static void ctk_table_size_request_pass3 (CtkTable *table);

static void ctk_table_size_allocate_init  (CtkTable *table);
static void ctk_table_size_allocate_pass1 (CtkTable *table);
static void ctk_table_size_allocate_pass2 (CtkTable *table);


G_DEFINE_TYPE_WITH_PRIVATE (CtkTable, ctk_table, CTK_TYPE_CONTAINER)

static void
ctk_table_class_init (CtkTableClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (class);
  CtkContainerClass *container_class = CTK_CONTAINER_CLASS (class);
  
  gobject_class->finalize = ctk_table_finalize;

  gobject_class->get_property = ctk_table_get_property;
  gobject_class->set_property = ctk_table_set_property;
  
  widget_class->get_preferred_width = ctk_table_get_preferred_width;
  widget_class->get_preferred_height = ctk_table_get_preferred_height;
  widget_class->size_allocate = ctk_table_size_allocate;
  
  container_class->add = ctk_table_add;
  container_class->remove = ctk_table_remove;
  container_class->forall = ctk_table_forall;
  container_class->child_type = ctk_table_child_type;
  container_class->set_child_property = ctk_table_set_child_property;
  container_class->get_child_property = ctk_table_get_child_property;
  ctk_container_class_handle_border_width (container_class);

  g_object_class_install_property (gobject_class,
                                   PROP_N_ROWS,
                                   g_param_spec_uint ("n-rows",
						     P_("Rows"),
						     P_("The number of rows in the table"),
						     1,
						     65535,
						     1,
						     CTK_PARAM_READWRITE));
  g_object_class_install_property (gobject_class,
                                   PROP_N_COLUMNS,
                                   g_param_spec_uint ("n-columns",
						     P_("Columns"),
						     P_("The number of columns in the table"),
						     1,
						     65535,
						     1,
						     CTK_PARAM_READWRITE));
  g_object_class_install_property (gobject_class,
                                   PROP_ROW_SPACING,
                                   g_param_spec_uint ("row-spacing",
						     P_("Row spacing"),
						     P_("The amount of space between two consecutive rows"),
						     0,
						     65535,
						     0,
						     CTK_PARAM_READWRITE));
  g_object_class_install_property (gobject_class,
                                   PROP_COLUMN_SPACING,
                                   g_param_spec_uint ("column-spacing",
						     P_("Column spacing"),
						     P_("The amount of space between two consecutive columns"),
						     0,
						     65535,
						     0,
						     CTK_PARAM_READWRITE));
  g_object_class_install_property (gobject_class,
                                   PROP_HOMOGENEOUS,
                                   g_param_spec_boolean ("homogeneous",
							 P_("Homogeneous"),
							 P_("If TRUE, the table cells are all the same width/height"),
							 FALSE,
							 CTK_PARAM_READWRITE));

  ctk_container_class_install_child_property (container_class,
					      CHILD_PROP_LEFT_ATTACH,
					      g_param_spec_uint ("left-attach", 
								 P_("Left attachment"), 
								 P_("The column number to attach the left side of the child to"),
								 0, 65535, 0,
								 CTK_PARAM_READWRITE));
  ctk_container_class_install_child_property (container_class,
					      CHILD_PROP_RIGHT_ATTACH,
					      g_param_spec_uint ("right-attach", 
								 P_("Right attachment"), 
								 P_("The column number to attach the right side of a child widget to"),
								 1, 65535, 1,
								 CTK_PARAM_READWRITE));
  ctk_container_class_install_child_property (container_class,
					      CHILD_PROP_TOP_ATTACH,
					      g_param_spec_uint ("top-attach", 
								 P_("Top attachment"), 
								 P_("The row number to attach the top of a child widget to"),
								 0, 65535, 0,
								 CTK_PARAM_READWRITE));
  ctk_container_class_install_child_property (container_class,
					      CHILD_PROP_BOTTOM_ATTACH,
					      g_param_spec_uint ("bottom-attach",
								 P_("Bottom attachment"), 
								 P_("The row number to attach the bottom of the child to"),
								 1, 65535, 1,
								 CTK_PARAM_READWRITE));
  ctk_container_class_install_child_property (container_class,
					      CHILD_PROP_X_OPTIONS,
					      g_param_spec_flags ("x-options", 
								  P_("Horizontal options"), 
								  P_("Options specifying the horizontal behaviour of the child"),
								  CTK_TYPE_ATTACH_OPTIONS, CTK_EXPAND | CTK_FILL,
								  CTK_PARAM_READWRITE));
  ctk_container_class_install_child_property (container_class,
					      CHILD_PROP_Y_OPTIONS,
					      g_param_spec_flags ("y-options", 
								  P_("Vertical options"), 
								  P_("Options specifying the vertical behaviour of the child"),
								  CTK_TYPE_ATTACH_OPTIONS, CTK_EXPAND | CTK_FILL,
								  CTK_PARAM_READWRITE));
  ctk_container_class_install_child_property (container_class,
					      CHILD_PROP_X_PADDING,
					      g_param_spec_uint ("x-padding", 
								 P_("Horizontal padding"), 
								 P_("Extra space to put between the child and its left and right neighbors, in pixels"),
								 0, 65535, 0,
								 CTK_PARAM_READWRITE));
  ctk_container_class_install_child_property (container_class,
					      CHILD_PROP_Y_PADDING,
					      g_param_spec_uint ("y-padding", 
								 P_("Vertical padding"), 
								 P_("Extra space to put between the child and its upper and lower neighbors, in pixels"),
								 0, 65535, 0,
								 CTK_PARAM_READWRITE));
}

static GType
ctk_table_child_type (CtkContainer *container G_GNUC_UNUSED)
{
  return CTK_TYPE_WIDGET;
}

static void
ctk_table_get_property (GObject      *object,
			guint         prop_id,
			GValue       *value,
			GParamSpec   *pspec)
{
  CtkTable *table = CTK_TABLE (object);
  CtkTablePrivate *priv = table->priv;

  switch (prop_id)
    {
    case PROP_N_ROWS:
      g_value_set_uint (value, priv->nrows);
      break;
    case PROP_N_COLUMNS:
      g_value_set_uint (value, priv->ncols);
      break;
    case PROP_ROW_SPACING:
      g_value_set_uint (value, priv->row_spacing);
      break;
    case PROP_COLUMN_SPACING:
      g_value_set_uint (value, priv->column_spacing);
      break;
    case PROP_HOMOGENEOUS:
      g_value_set_boolean (value, priv->homogeneous);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_table_set_property (GObject      *object,
			guint         prop_id,
			const GValue *value,
			GParamSpec   *pspec)
{
  CtkTable *table = CTK_TABLE (object);
  CtkTablePrivate *priv = table->priv;

  switch (prop_id)
    {
    case PROP_N_ROWS:
      ctk_table_resize (table, g_value_get_uint (value), priv->ncols);
      break;
    case PROP_N_COLUMNS:
      ctk_table_resize (table, priv->nrows, g_value_get_uint (value));
      break;
    case PROP_ROW_SPACING:
      ctk_table_set_row_spacings (table, g_value_get_uint (value));
      break;
    case PROP_COLUMN_SPACING:
      ctk_table_set_col_spacings (table, g_value_get_uint (value));
      break;
    case PROP_HOMOGENEOUS:
      ctk_table_set_homogeneous (table, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ctk_table_set_child_property (CtkContainer    *container,
			      CtkWidget       *child,
			      guint            property_id,
			      const GValue    *value,
			      GParamSpec      *pspec)
{
  CtkTable *table = CTK_TABLE (container);
  CtkTablePrivate *priv = table->priv;
  CtkTableChild *table_child;
  GList *list;

  table_child = NULL;
  for (list = priv->children; list; list = list->next)
    {
      table_child = list->data;

      if (table_child->widget == child)
	break;
    }
  if (!list)
    {
      CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      return;
    }

  switch (property_id)
    {
    case CHILD_PROP_LEFT_ATTACH:
      table_child->left_attach = g_value_get_uint (value);
      if (table_child->right_attach <= table_child->left_attach)
	table_child->right_attach = table_child->left_attach + 1;
      if (table_child->right_attach >= priv->ncols)
	ctk_table_resize (table, priv->nrows, table_child->right_attach);
      break;
    case CHILD_PROP_RIGHT_ATTACH:
      table_child->right_attach = g_value_get_uint (value);
      if (table_child->right_attach <= table_child->left_attach)
	table_child->left_attach = table_child->right_attach - 1;
      if (table_child->right_attach >= priv->ncols)
	ctk_table_resize (table, priv->nrows, table_child->right_attach);
      break;
    case CHILD_PROP_TOP_ATTACH:
      table_child->top_attach = g_value_get_uint (value);
      if (table_child->bottom_attach <= table_child->top_attach)
	table_child->bottom_attach = table_child->top_attach + 1;
      if (table_child->bottom_attach >= priv->nrows)
	ctk_table_resize (table, table_child->bottom_attach, priv->ncols);
      break;
    case CHILD_PROP_BOTTOM_ATTACH:
      table_child->bottom_attach = g_value_get_uint (value);
      if (table_child->bottom_attach <= table_child->top_attach)
	table_child->top_attach = table_child->bottom_attach - 1;
      if (table_child->bottom_attach >= priv->nrows)
	ctk_table_resize (table, table_child->bottom_attach, priv->ncols);
      break;
    case CHILD_PROP_X_OPTIONS:
      {
        table_child->xexpand = (g_value_get_flags (value) & CTK_EXPAND) != 0;
        table_child->xshrink = (g_value_get_flags (value) & CTK_SHRINK) != 0;
        table_child->xfill = (g_value_get_flags (value) & CTK_FILL) != 0;
      }
      break;
    case CHILD_PROP_Y_OPTIONS:
      {
        table_child->yexpand = (g_value_get_flags (value) & CTK_EXPAND) != 0;
        table_child->yshrink = (g_value_get_flags (value) & CTK_SHRINK) != 0;
        table_child->yfill = (g_value_get_flags (value) & CTK_FILL) != 0;
      }
      break;
    case CHILD_PROP_X_PADDING:
      table_child->xpadding = g_value_get_uint (value);
      break;
    case CHILD_PROP_Y_PADDING:
      table_child->ypadding = g_value_get_uint (value);
      break;
    default:
      CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
  if (ctk_widget_get_visible (child) &&
      ctk_widget_get_visible (CTK_WIDGET (table)))
    ctk_widget_queue_resize (child);
}

static void
ctk_table_get_child_property (CtkContainer    *container,
			      CtkWidget       *child,
			      guint            property_id,
			      GValue          *value,
			      GParamSpec      *pspec)
{
  CtkTable *table = CTK_TABLE (container);
  CtkTablePrivate *priv = table->priv;
  CtkTableChild *table_child;
  GList *list;

  table_child = NULL;
  for (list = priv->children; list; list = list->next)
    {
      table_child = list->data;

      if (table_child->widget == child)
	break;
    }
  if (!list)
    {
      CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      return;
    }

  switch (property_id)
    {
    case CHILD_PROP_LEFT_ATTACH:
      g_value_set_uint (value, table_child->left_attach);
      break;
    case CHILD_PROP_RIGHT_ATTACH:
      g_value_set_uint (value, table_child->right_attach);
      break;
    case CHILD_PROP_TOP_ATTACH:
      g_value_set_uint (value, table_child->top_attach);
      break;
    case CHILD_PROP_BOTTOM_ATTACH:
      g_value_set_uint (value, table_child->bottom_attach);
      break;
    case CHILD_PROP_X_OPTIONS:
      g_value_set_flags (value, (table_child->xexpand * CTK_EXPAND |
				 table_child->xshrink * CTK_SHRINK |
				 table_child->xfill * CTK_FILL));
      break;
    case CHILD_PROP_Y_OPTIONS:
      g_value_set_flags (value, (table_child->yexpand * CTK_EXPAND |
				 table_child->yshrink * CTK_SHRINK |
				 table_child->yfill * CTK_FILL));
      break;
    case CHILD_PROP_X_PADDING:
      g_value_set_uint (value, table_child->xpadding);
      break;
    case CHILD_PROP_Y_PADDING:
      g_value_set_uint (value, table_child->ypadding);
      break;
    default:
      CTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
ctk_table_init (CtkTable *table)
{
  CtkTablePrivate *priv;

  table->priv = ctk_table_get_instance_private (table);
  priv = table->priv;

  ctk_widget_set_has_window (CTK_WIDGET (table), FALSE);

  priv->children = NULL;
  priv->rows = NULL;
  priv->cols = NULL;
  priv->nrows = 0;
  priv->ncols = 0;
  priv->column_spacing = 0;
  priv->row_spacing = 0;
  priv->homogeneous = FALSE;

  ctk_table_resize (table, 1, 1);
}

/**
 * ctk_table_new:
 * @rows: The number of rows the new table should have.
 * @columns: The number of columns the new table should have.
 * @homogeneous: If set to %TRUE, all table cells are resized to the size of
 *   the cell containing the largest widget.
 *
 * Used to create a new table widget. An initial size must be given by
 * specifying how many rows and columns the table should have, although
 * this can be changed later with ctk_table_resize().  @rows and @columns
 * must both be in the range 1 .. 65535. For historical reasons, 0 is accepted
 * as well and is silently interpreted as 1.
 *
 * Returns: A pointer to the newly created table widget.
 *
 * Deprecated: 3.4: Use ctk_grid_new().
 */
CtkWidget*
ctk_table_new (guint	rows,
	       guint	columns,
	       gboolean homogeneous)
{
  CtkTable *table;
  CtkTablePrivate *priv;

  if (rows == 0)
    rows = 1;
  if (columns == 0)
    columns = 1;
  
  table = g_object_new (CTK_TYPE_TABLE, NULL);
  priv = table->priv;

  priv->homogeneous = (homogeneous ? TRUE : FALSE);

  ctk_table_resize (table, rows, columns);
  
  return CTK_WIDGET (table);
}

/**
 * ctk_table_resize:
 * @table: The #CtkTable you wish to change the size of.
 * @rows: The new number of rows.
 * @columns: The new number of columns.
 *
 * If you need to change a table’s size after
 * it has been created, this function allows you to do so.
 *
 * Deprecated: 3.4: #CtkGrid resizes automatically.
 */
void
ctk_table_resize (CtkTable *table,
		  guint     n_rows,
		  guint     n_cols)
{
  CtkTablePrivate *priv;

  g_return_if_fail (CTK_IS_TABLE (table));
  g_return_if_fail (n_rows > 0 && n_rows <= 65535);
  g_return_if_fail (n_cols > 0 && n_cols <= 65535);

  priv = table->priv;

  n_rows = MAX (n_rows, 1);
  n_cols = MAX (n_cols, 1);

  if (n_rows != priv->nrows ||
      n_cols != priv->ncols)
    {
      GList *list;
      
      for (list = priv->children; list; list = list->next)
	{
	  CtkTableChild *child;
	  
	  child = list->data;
	  
	  n_rows = MAX (n_rows, child->bottom_attach);
	  n_cols = MAX (n_cols, child->right_attach);
	}

      if (n_rows != priv->nrows)
	{
	  guint i;

	  i = priv->nrows;
	  priv->nrows = n_rows;
	  priv->rows = g_realloc (priv->rows, priv->nrows * sizeof (CtkTableRowCol));

	  for (; i < priv->nrows; i++)
	    {
	      priv->rows[i].requisition = 0;
	      priv->rows[i].allocation = 0;
	      priv->rows[i].spacing = priv->row_spacing;
	      priv->rows[i].need_expand = 0;
	      priv->rows[i].need_shrink = 0;
	      priv->rows[i].expand = 0;
	      priv->rows[i].shrink = 0;
	    }

	  g_object_notify (G_OBJECT (table), "n-rows");
	}

      if (n_cols != priv->ncols)
	{
	  guint i;

	  i = priv->ncols;
	  priv->ncols = n_cols;
	  priv->cols = g_realloc (priv->cols, priv->ncols * sizeof (CtkTableRowCol));

	  for (; i < priv->ncols; i++)
	    {
	      priv->cols[i].requisition = 0;
	      priv->cols[i].allocation = 0;
	      priv->cols[i].spacing = priv->column_spacing;
	      priv->cols[i].need_expand = 0;
	      priv->cols[i].need_shrink = 0;
	      priv->cols[i].expand = 0;
	      priv->cols[i].shrink = 0;
	    }

	  g_object_notify (G_OBJECT (table), "n-columns");
	}
    }
}

/**
 * ctk_table_attach:
 * @table: The #CtkTable to add a new widget to.
 * @child: The widget to add.
 * @left_attach: the column number to attach the left side of a child widget to.
 * @right_attach: the column number to attach the right side of a child widget to.
 * @top_attach: the row number to attach the top of a child widget to.
 * @bottom_attach: the row number to attach the bottom of a child widget to.
 * @xoptions: Used to specify the properties of the child widget when the table is resized.
 * @yoptions: The same as xoptions, except this field determines behaviour of vertical resizing.
 * @xpadding: An integer value specifying the padding on the left and right of the widget being added to the table.
 * @ypadding: The amount of padding above and below the child widget.
 *
 * Adds a widget to a table. The number of “cells” that a widget will occupy is
 * specified by @left_attach, @right_attach, @top_attach and @bottom_attach.
 * These each represent the leftmost, rightmost, uppermost and lowest column
 * and row numbers of the table. (Columns and rows are indexed from zero).
 *
 * To make a button occupy the lower right cell of a 2x2 table, use
 * |[
 * ctk_table_attach (table, button,
 *                   1, 2, // left, right attach
 *                   1, 2, // top, bottom attach
 *                   xoptions, yoptions,
 *                   xpadding, ypadding);
 * ]|
 * If you want to make the button span the entire bottom row, use @left_attach == 0 and @right_attach = 2 instead.
 *
 * Deprecated: 3.4: Use ctk_grid_attach() with #CtkGrid. Note that the attach
 *     arguments differ between those two functions.
 */
void
ctk_table_attach (CtkTable	  *table,
		  CtkWidget	  *child,
		  guint		   left_attach,
		  guint		   right_attach,
		  guint		   top_attach,
		  guint		   bottom_attach,
		  CtkAttachOptions xoptions,
		  CtkAttachOptions yoptions,
		  guint		   xpadding,
		  guint		   ypadding)
{
  CtkTablePrivate *priv;
  CtkTableChild *table_child;

  g_return_if_fail (CTK_IS_TABLE (table));
  g_return_if_fail (CTK_IS_WIDGET (child));
  g_return_if_fail (ctk_widget_get_parent (child) == NULL);

  /* g_return_if_fail (left_attach >= 0); */
  g_return_if_fail (left_attach < right_attach);
  /* g_return_if_fail (top_attach >= 0); */
  g_return_if_fail (top_attach < bottom_attach);

  priv = table->priv;

  if (right_attach >= priv->ncols)
    ctk_table_resize (table, priv->nrows, right_attach);

  if (bottom_attach >= priv->nrows)
    ctk_table_resize (table, bottom_attach, priv->ncols);

  table_child = g_new (CtkTableChild, 1);
  table_child->widget = child;
  table_child->left_attach = left_attach;
  table_child->right_attach = right_attach;
  table_child->top_attach = top_attach;
  table_child->bottom_attach = bottom_attach;
  table_child->xexpand = (xoptions & CTK_EXPAND) != 0;
  table_child->xshrink = (xoptions & CTK_SHRINK) != 0;
  table_child->xfill = (xoptions & CTK_FILL) != 0;
  table_child->xpadding = xpadding;
  table_child->yexpand = (yoptions & CTK_EXPAND) != 0;
  table_child->yshrink = (yoptions & CTK_SHRINK) != 0;
  table_child->yfill = (yoptions & CTK_FILL) != 0;
  table_child->ypadding = ypadding;

  priv->children = g_list_prepend (priv->children, table_child);

  ctk_widget_set_parent (child, CTK_WIDGET (table));
}

/**
 * ctk_table_attach_defaults:
 * @table: The table to add a new child widget to.
 * @widget: The child widget to add.
 * @left_attach: The column number to attach the left side of the child widget to.
 * @right_attach: The column number to attach the right side of the child widget to.
 * @top_attach: The row number to attach the top of the child widget to.
 * @bottom_attach: The row number to attach the bottom of the child widget to.
 *
 * As there are many options associated with ctk_table_attach(), this convenience
 * function provides the programmer with a means to add children to a table with
 * identical padding and expansion options. The values used for the #CtkAttachOptions
 * are `CTK_EXPAND | CTK_FILL`, and the padding is set to 0.
 *
 * Deprecated: 3.4: Use ctk_grid_attach() with #CtkGrid. Note that the attach
 *     arguments differ between those two functions.
 */
void
ctk_table_attach_defaults (CtkTable  *table,
			   CtkWidget *widget,
			   guint      left_attach,
			   guint      right_attach,
			   guint      top_attach,
			   guint      bottom_attach)
{
  ctk_table_attach (table, widget,
		    left_attach, right_attach,
		    top_attach, bottom_attach,
		    CTK_EXPAND | CTK_FILL,
		    CTK_EXPAND | CTK_FILL,
		    0, 0);
}

/**
 * ctk_table_set_row_spacing:
 * @table: a #CtkTable containing the row whose properties you wish to change.
 * @row: row number whose spacing will be changed.
 * @spacing: number of pixels that the spacing should take up.
 *
 * Changes the space between a given table row and the subsequent row.
 *
 * Deprecated: 3.4: Use ctk_widget_set_margin_top() and
 *     ctk_widget_set_margin_bottom() on the widgets contained in the row if
 *     you need this functionality. #CtkGrid does not support per-row spacing.
 */
void
ctk_table_set_row_spacing (CtkTable *table,
			   guint     row,
			   guint     spacing)
{
  CtkTablePrivate *priv;

  g_return_if_fail (CTK_IS_TABLE (table));

  priv = table->priv;

  g_return_if_fail (row < priv->nrows);

  if (priv->rows[row].spacing != spacing)
    {
      priv->rows[row].spacing = spacing;

      if (ctk_widget_get_visible (CTK_WIDGET (table)))
	ctk_widget_queue_resize (CTK_WIDGET (table));
    }
}

/**
 * ctk_table_get_row_spacing:
 * @table: a #CtkTable
 * @row: a row in the table, 0 indicates the first row
 *
 * Gets the amount of space between row @row, and
 * row @row + 1. See ctk_table_set_row_spacing().
 *
 * Returns: the row spacing
 *
 * Deprecated: 3.4: #CtkGrid does not offer a replacement for this
 *     functionality.
 **/
guint
ctk_table_get_row_spacing (CtkTable *table,
			   guint     row)
{
  CtkTablePrivate *priv;

  g_return_val_if_fail (CTK_IS_TABLE (table), 0);

  priv = table->priv;

  g_return_val_if_fail (row < priv->nrows - 1, 0);

  return priv->rows[row].spacing;
}

/**
 * ctk_table_set_col_spacing:
 * @table: a #CtkTable.
 * @column: the column whose spacing should be changed.
 * @spacing: number of pixels that the spacing should take up.
 *
 * Alters the amount of space between a given table column and the following
 * column.
 *
 * Deprecated: 3.4: Use ctk_widget_set_margin_start() and
 *     ctk_widget_set_margin_end() on the widgets contained in the row if
 *     you need this functionality. #CtkGrid does not support per-row spacing.
 */
void
ctk_table_set_col_spacing (CtkTable *table,
			   guint     column,
			   guint     spacing)
{
  CtkTablePrivate *priv;

  g_return_if_fail (CTK_IS_TABLE (table));

  priv = table->priv;

  g_return_if_fail (column < priv->ncols);

  if (priv->cols[column].spacing != spacing)
    {
      priv->cols[column].spacing = spacing;

      if (ctk_widget_get_visible (CTK_WIDGET (table)))
	ctk_widget_queue_resize (CTK_WIDGET (table));
    }
}

/**
 * ctk_table_get_col_spacing:
 * @table: a #CtkTable
 * @column: a column in the table, 0 indicates the first column
 *
 * Gets the amount of space between column @col, and
 * column @col + 1. See ctk_table_set_col_spacing().
 *
 * Returns: the column spacing
 *
 * Deprecated: 3.4: #CtkGrid does not offer a replacement for this
 *     functionality.
 **/
guint
ctk_table_get_col_spacing (CtkTable *table,
			   guint     column)
{
  CtkTablePrivate *priv;

  g_return_val_if_fail (CTK_IS_TABLE (table), 0);

  priv = table->priv;

  g_return_val_if_fail (column < priv->ncols, 0);

  return priv->cols[column].spacing;
}

/**
 * ctk_table_set_row_spacings:
 * @table: a #CtkTable.
 * @spacing: the number of pixels of space to place between every row in the table.
 *
 * Sets the space between every row in @table equal to @spacing.
 *
 * Deprecated: 3.4: Use ctk_grid_set_row_spacing() with #CtkGrid.
 */
void
ctk_table_set_row_spacings (CtkTable *table,
			    guint     spacing)
{
  CtkTablePrivate *priv;
  guint row;
  
  g_return_if_fail (CTK_IS_TABLE (table));

  priv = table->priv;

  priv->row_spacing = spacing;
  for (row = 0; row < priv->nrows; row++)
    priv->rows[row].spacing = spacing;

  if (ctk_widget_get_visible (CTK_WIDGET (table)))
    ctk_widget_queue_resize (CTK_WIDGET (table));

  g_object_notify (G_OBJECT (table), "row-spacing");
}

/**
 * ctk_table_get_default_row_spacing:
 * @table: a #CtkTable
 *
 * Gets the default row spacing for the table. This is
 * the spacing that will be used for newly added rows.
 * (See ctk_table_set_row_spacings())
 *
 * Returns: the default row spacing
 *
 * Deprecated: 3.4: Use ctk_grid_get_row_spacing() with #CtkGrid.
 **/
guint
ctk_table_get_default_row_spacing (CtkTable *table)
{
  g_return_val_if_fail (CTK_IS_TABLE (table), 0);

  return table->priv->row_spacing;
}

/**
 * ctk_table_set_col_spacings:
 * @table: a #CtkTable.
 * @spacing: the number of pixels of space to place between every column
 *   in the table.
 *
 * Sets the space between every column in @table equal to @spacing.
 *
 * Deprecated: 3.4: Use ctk_grid_set_column_spacing() with #CtkGrid.
 */
void
ctk_table_set_col_spacings (CtkTable *table,
			    guint     spacing)
{
  CtkTablePrivate *priv;
  guint col;

  g_return_if_fail (CTK_IS_TABLE (table));

  priv = table->priv;

  priv->column_spacing = spacing;
  for (col = 0; col < priv->ncols; col++)
    priv->cols[col].spacing = spacing;

  if (ctk_widget_get_visible (CTK_WIDGET (table)))
    ctk_widget_queue_resize (CTK_WIDGET (table));

  g_object_notify (G_OBJECT (table), "column-spacing");
}

/**
 * ctk_table_get_default_col_spacing:
 * @table: a #CtkTable
 *
 * Gets the default column spacing for the table. This is
 * the spacing that will be used for newly added columns.
 * (See ctk_table_set_col_spacings())
 *
 * Returns: the default column spacing
 *
 * Deprecated: 3.4: Use ctk_grid_get_column_spacing() with #CtkGrid.
 **/
guint
ctk_table_get_default_col_spacing (CtkTable *table)
{
  g_return_val_if_fail (CTK_IS_TABLE (table), 0);

  return table->priv->column_spacing;
}

/**
 * ctk_table_set_homogeneous:
 * @table: The #CtkTable you wish to set the homogeneous properties of.
 * @homogeneous: Set to %TRUE to ensure all table cells are the same size. Set
 *   to %FALSE if this is not your desired behaviour.
 *
 * Changes the homogenous property of table cells, ie. whether all cells are
 * an equal size or not.
 *
 * Deprecated: 3.4: Use ctk_grid_set_row_homogeneous() and
 *     ctk_grid_set_column_homogeneous() with #CtkGrid.
 */
void
ctk_table_set_homogeneous (CtkTable *table,
			   gboolean  homogeneous)
{
  CtkTablePrivate *priv;

  g_return_if_fail (CTK_IS_TABLE (table));

  priv = table->priv;

  homogeneous = (homogeneous != 0);
  if (homogeneous != priv->homogeneous)
    {
      priv->homogeneous = homogeneous;
      
      if (ctk_widget_get_visible (CTK_WIDGET (table)))
	ctk_widget_queue_resize (CTK_WIDGET (table));

      g_object_notify (G_OBJECT (table), "homogeneous");
    }
}

/**
 * ctk_table_get_homogeneous:
 * @table: a #CtkTable
 *
 * Returns whether the table cells are all constrained to the same
 * width and height. (See ctk_table_set_homogeneous ())
 *
 * Returns: %TRUE if the cells are all constrained to the same size
 *
 * Deprecated: 3.4: Use ctk_grid_get_row_homogeneous() and
 *     ctk_grid_get_column_homogeneous() with #CtkGrid.
 **/
gboolean
ctk_table_get_homogeneous (CtkTable *table)
{
  g_return_val_if_fail (CTK_IS_TABLE (table), FALSE);

  return table->priv->homogeneous;
}

/**
 * ctk_table_get_size:
 * @table: a #CtkTable
 * @rows: (out) (allow-none): return location for the number of
 *   rows, or %NULL
 * @columns: (out) (allow-none): return location for the number
 *   of columns, or %NULL
 *
 * Gets the number of rows and columns in the table.
 *
 * Since: 2.22
 *
 * Deprecated: 3.4: #CtkGrid does not expose the number of columns and
 *     rows.
 **/
void
ctk_table_get_size (CtkTable *table,
                    guint    *rows,
                    guint    *columns)
{
  CtkTablePrivate *priv;

  g_return_if_fail (CTK_IS_TABLE (table));

  priv = table->priv;

  if (rows)
    *rows = priv->nrows;

  if (columns)
    *columns = priv->ncols;
}

static void
ctk_table_finalize (GObject *object)
{
  CtkTable *table = CTK_TABLE (object);
  CtkTablePrivate *priv = table->priv;

  g_free (priv->rows);
  g_free (priv->cols);

  G_OBJECT_CLASS (ctk_table_parent_class)->finalize (object);
}

static void
ctk_table_get_preferred_width (CtkWidget *widget,
                               gint      *minimum,
                               gint      *natural)
{
  CtkTable *table = CTK_TABLE (widget);
  CtkTablePrivate *priv = table->priv;
  gint col;

  ctk_table_size_request_init (table);
  ctk_table_size_request_pass1 (table);
  ctk_table_size_request_pass2 (table);
  ctk_table_size_request_pass3 (table);
  ctk_table_size_request_pass2 (table);

  *minimum = 0;

  for (col = 0; col < priv->ncols; col++)
    *minimum += priv->cols[col].requisition;
  for (col = 0; col + 1 < priv->ncols; col++)
    *minimum += priv->cols[col].spacing;

  *natural = *minimum;
}

static void
ctk_table_get_preferred_height (CtkWidget *widget,
                                gint      *minimum,
                                gint      *natural)
{
  CtkTable *table = CTK_TABLE (widget);
  CtkTablePrivate *priv = table->priv;
  gint row;

  ctk_table_size_request_init (table);
  ctk_table_size_request_pass1 (table);
  ctk_table_size_request_pass2 (table);
  ctk_table_size_request_pass3 (table);
  ctk_table_size_request_pass2 (table);

  *minimum = 0;
  for (row = 0; row < priv->nrows; row++)
    *minimum += priv->rows[row].requisition;
  for (row = 0; row + 1 < priv->nrows; row++)
    *minimum += priv->rows[row].spacing;

  *natural = *minimum;
}

static void
ctk_table_size_allocate (CtkWidget     *widget,
			 CtkAllocation *allocation)
{
  CtkTable *table = CTK_TABLE (widget);

  ctk_widget_set_allocation (widget, allocation);

  ctk_table_size_allocate_init (table);
  ctk_table_size_allocate_pass1 (table);
  ctk_table_size_allocate_pass2 (table);
}

static void
ctk_table_add (CtkContainer *container,
	       CtkWidget    *widget)
{
  ctk_table_attach_defaults (CTK_TABLE (container), widget, 0, 1, 0, 1);
}

static void
ctk_table_remove (CtkContainer *container,
		  CtkWidget    *widget)
{
  CtkTable *table = CTK_TABLE (container);
  CtkTablePrivate *priv = table->priv;
  CtkTableChild *child;
  CtkWidget *widget_container = CTK_WIDGET (container);
  GList *children;

  children = priv->children;

  while (children)
    {
      child = children->data;
      children = children->next;
      
      if (child->widget == widget)
	{
	  gboolean was_visible = ctk_widget_get_visible (widget);
	  
	  ctk_widget_unparent (widget);

	  priv->children = g_list_remove (priv->children, child);
	  g_free (child);
	  
	  if (was_visible && ctk_widget_get_visible (widget_container))
	    ctk_widget_queue_resize (widget_container);
	  break;
	}
    }
}

static void
ctk_table_forall (CtkContainer *container,
		  gboolean      include_internals G_GNUC_UNUSED,
		  CtkCallback   callback,
		  gpointer      callback_data)
{
  CtkTable *table = CTK_TABLE (container);
  CtkTablePrivate *priv = table->priv;
  CtkTableChild *child;
  GList *children;

  children = priv->children;

  while (children)
    {
      child = children->data;
      children = children->next;
      
      (* callback) (child->widget, callback_data);
    }
}

static void
ctk_table_size_request_init (CtkTable *table)
{
  CtkTablePrivate *priv = table->priv;
  CtkTableChild *child;
  GList *children;
  gint row, col;

  for (row = 0; row < priv->nrows; row++)
    {
      priv->rows[row].requisition = 0;
      priv->rows[row].expand = FALSE;
    }
  for (col = 0; col < priv->ncols; col++)
    {
      priv->cols[col].requisition = 0;
      priv->cols[col].expand = FALSE;
    }
  
  children = priv->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      if (child->left_attach == (child->right_attach - 1) &&
          (child->xexpand || ctk_widget_compute_expand (child->widget, CTK_ORIENTATION_HORIZONTAL)))
	priv->cols[child->left_attach].expand = TRUE;
      
      if (child->top_attach == (child->bottom_attach - 1) &&
          (child->yexpand || ctk_widget_compute_expand (child->widget, CTK_ORIENTATION_VERTICAL)))
	priv->rows[child->top_attach].expand = TRUE;
    }
}

static void
ctk_table_size_request_pass1 (CtkTable *table)
{
  CtkTablePrivate *priv = table->priv;
  CtkTableChild *child;
  GList *children;
  gint width;
  gint height;

  children = priv->children;
  while (children)
    {
      child = children->data;
      children = children->next;
      
      if (ctk_widget_get_visible (child->widget))
	{
	  CtkRequisition child_requisition;

          ctk_widget_get_preferred_size (child->widget, &child_requisition, NULL);

	  /* Child spans a single column.
	   */
	  if (child->left_attach == (child->right_attach - 1))
	    {
	      width = child_requisition.width + child->xpadding * 2;
	      priv->cols[child->left_attach].requisition = MAX (priv->cols[child->left_attach].requisition, width);
	    }
	  
	  /* Child spans a single row.
	   */
	  if (child->top_attach == (child->bottom_attach - 1))
	    {
	      height = child_requisition.height + child->ypadding * 2;
	      priv->rows[child->top_attach].requisition = MAX (priv->rows[child->top_attach].requisition, height);
	    }
	}
    }
}

static void
ctk_table_size_request_pass2 (CtkTable *table)
{
  CtkTablePrivate *priv = table->priv;
  gint max_width;
  gint max_height;
  gint row, col;

  if (priv->homogeneous)
    {
      max_width = 0;
      max_height = 0;

      for (col = 0; col < priv->ncols; col++)
	max_width = MAX (max_width, priv->cols[col].requisition);
      for (row = 0; row < priv->nrows; row++)
	max_height = MAX (max_height, priv->rows[row].requisition);

      for (col = 0; col < priv->ncols; col++)
	priv->cols[col].requisition = max_width;
      for (row = 0; row < priv->nrows; row++)
	priv->rows[row].requisition = max_height;
    }
}

static void
ctk_table_size_request_pass3 (CtkTable *table)
{
  CtkTablePrivate *priv = table->priv;
  CtkTableChild *child;
  GList *children;
  gint width, height;
  gint row, col;
  gint extra;

  children = priv->children;
  while (children)
    {
      child = children->data;
      children = children->next;
      
      if (ctk_widget_get_visible (child->widget))
	{
	  /* Child spans multiple columns.
	   */
	  if (child->left_attach != (child->right_attach - 1))
	    {
	      CtkRequisition child_requisition;

              ctk_widget_get_preferred_size (child->widget,
                                             &child_requisition, NULL);

	      /* Check and see if there is already enough space
	       *  for the child.
	       */
	      width = 0;
	      for (col = child->left_attach; col < child->right_attach; col++)
		{
		  width += priv->cols[col].requisition;
		  if ((col + 1) < child->right_attach)
		    width += priv->cols[col].spacing;
		}
	      
	      /* If we need to request more space for this child to fill
	       *  its requisition, then divide up the needed space amongst the
	       *  columns it spans, favoring expandable columns if any.
	       */
	      if (width < child_requisition.width + child->xpadding * 2)
		{
		  gint n_expand = 0;
		  gboolean force_expand = FALSE;
		  
		  width = child_requisition.width + child->xpadding * 2 - width;

		  for (col = child->left_attach; col < child->right_attach; col++)
		    if (priv->cols[col].expand)
		      n_expand++;

		  if (n_expand == 0)
		    {
		      n_expand = (child->right_attach - child->left_attach);
		      force_expand = TRUE;
		    }
		    
		  for (col = child->left_attach; col < child->right_attach; col++)
		    if (force_expand || priv->cols[col].expand)
		      {
			extra = width / n_expand;
			priv->cols[col].requisition += extra;
			width -= extra;
			n_expand--;
		      }
		}
	    }
	  
	  /* Child spans multiple rows.
	   */
	  if (child->top_attach != (child->bottom_attach - 1))
	    {
	      CtkRequisition child_requisition;

              ctk_widget_get_preferred_size (child->widget,
                                             &child_requisition, NULL);

	      /* Check and see if there is already enough space
	       *  for the child.
	       */
	      height = 0;
	      for (row = child->top_attach; row < child->bottom_attach; row++)
		{
		  height += priv->rows[row].requisition;
		  if ((row + 1) < child->bottom_attach)
		    height += priv->rows[row].spacing;
		}
	      
	      /* If we need to request more space for this child to fill
	       *  its requisition, then divide up the needed space amongst the
	       *  rows it spans, favoring expandable rows if any.
	       */
	      if (height < child_requisition.height + child->ypadding * 2)
		{
		  gint n_expand = 0;
		  gboolean force_expand = FALSE;
		  
		  height = child_requisition.height + child->ypadding * 2 - height;
		  
		  for (row = child->top_attach; row < child->bottom_attach; row++)
		    {
		      if (priv->rows[row].expand)
			n_expand++;
		    }

		  if (n_expand == 0)
		    {
		      n_expand = (child->bottom_attach - child->top_attach);
		      force_expand = TRUE;
		    }
		    
		  for (row = child->top_attach; row < child->bottom_attach; row++)
		    if (force_expand || priv->rows[row].expand)
		      {
			extra = height / n_expand;
			priv->rows[row].requisition += extra;
			height -= extra;
			n_expand--;
		      }
		}
	    }
	}
    }
}

static void
ctk_table_size_allocate_init (CtkTable *table)
{
  CtkTablePrivate *priv = table->priv;
  CtkTableChild *child;
  GList *children;
  gint row, col;
  gint has_expand;
  gint has_shrink;
  
  /* Initialize the rows and cols.
   *  By default, rows and cols do not expand and do shrink.
   *  Those values are modified by the children that occupy
   *  the rows and cols.
   */
  for (col = 0; col < priv->ncols; col++)
    {
      priv->cols[col].allocation = priv->cols[col].requisition;
      priv->cols[col].need_expand = FALSE;
      priv->cols[col].need_shrink = TRUE;
      priv->cols[col].expand = FALSE;
      priv->cols[col].shrink = TRUE;
      priv->cols[col].empty = TRUE;
    }
  for (row = 0; row < priv->nrows; row++)
    {
      priv->rows[row].allocation = priv->rows[row].requisition;
      priv->rows[row].need_expand = FALSE;
      priv->rows[row].need_shrink = TRUE;
      priv->rows[row].expand = FALSE;
      priv->rows[row].shrink = TRUE;
      priv->rows[row].empty = TRUE;
    }
  
  /* Loop over all the children and adjust the row and col values
   *  based on whether the children want to be allowed to expand
   *  or shrink. This loop handles children that occupy a single
   *  row or column.
   */
  children = priv->children;
  while (children)
    {
      child = children->data;
      children = children->next;
      
      if (ctk_widget_get_visible (child->widget))
	{
	  if (child->left_attach == (child->right_attach - 1))
	    {
	      if (child->xexpand || ctk_widget_compute_expand (child->widget, CTK_ORIENTATION_HORIZONTAL))
		priv->cols[child->left_attach].expand = TRUE;

	      if (!child->xshrink)
		priv->cols[child->left_attach].shrink = FALSE;

	      priv->cols[child->left_attach].empty = FALSE;
	    }
	  
	  if (child->top_attach == (child->bottom_attach - 1))
	    {
	      if (child->yexpand || ctk_widget_compute_expand (child->widget, CTK_ORIENTATION_VERTICAL))
		priv->rows[child->top_attach].expand = TRUE;
	      
	      if (!child->yshrink)
		priv->rows[child->top_attach].shrink = FALSE;

	      priv->rows[child->top_attach].empty = FALSE;
	    }
	}
    }
  
  /* Loop over all the children again and this time handle children
   *  which span multiple rows or columns.
   */
  children = priv->children;
  while (children)
    {
      child = children->data;
      children = children->next;
      
      if (ctk_widget_get_visible (child->widget))
	{
	  if (child->left_attach != (child->right_attach - 1))
	    {
	      for (col = child->left_attach; col < child->right_attach; col++)
		priv->cols[col].empty = FALSE;

	      if (child->xexpand)
		{
		  has_expand = FALSE;
		  for (col = child->left_attach; col < child->right_attach; col++)
		    if (priv->cols[col].expand)
		      {
			has_expand = TRUE;
			break;
		      }
		  
		  if (!has_expand)
		    for (col = child->left_attach; col < child->right_attach; col++)
		      priv->cols[col].need_expand = TRUE;
		}
	      
	      if (!child->xshrink)
		{
		  has_shrink = TRUE;
		  for (col = child->left_attach; col < child->right_attach; col++)
		    if (!priv->cols[col].shrink)
		      {
			has_shrink = FALSE;
			break;
		      }
		  
		  if (has_shrink)
		    for (col = child->left_attach; col < child->right_attach; col++)
		      priv->cols[col].need_shrink = FALSE;
		}
	    }
	  
	  if (child->top_attach != (child->bottom_attach - 1))
	    {
	      for (row = child->top_attach; row < child->bottom_attach; row++)
		priv->rows[row].empty = FALSE;

	      if (child->yexpand)
		{
		  has_expand = FALSE;
		  for (row = child->top_attach; row < child->bottom_attach; row++)
		    if (priv->rows[row].expand)
		      {
			has_expand = TRUE;
			break;
		      }
		  
		  if (!has_expand)
		    for (row = child->top_attach; row < child->bottom_attach; row++)
		      priv->rows[row].need_expand = TRUE;
		}
	      
	      if (!child->yshrink)
		{
		  has_shrink = TRUE;
		  for (row = child->top_attach; row < child->bottom_attach; row++)
		    if (!priv->rows[row].shrink)
		      {
			has_shrink = FALSE;
			break;
		      }
		  
		  if (has_shrink)
		    for (row = child->top_attach; row < child->bottom_attach; row++)
		      priv->rows[row].need_shrink = FALSE;
		}
	    }
	}
    }
  
  /* Loop over the columns and set the expand and shrink values
   *  if the column can be expanded or shrunk.
   */
  for (col = 0; col < priv->ncols; col++)
    {
      if (priv->cols[col].empty)
	{
	  priv->cols[col].expand = FALSE;
	  priv->cols[col].shrink = FALSE;
	}
      else
	{
	  if (priv->cols[col].need_expand)
	    priv->cols[col].expand = TRUE;
	  if (!priv->cols[col].need_shrink)
	    priv->cols[col].shrink = FALSE;
	}
    }
  
  /* Loop over the rows and set the expand and shrink values
   *  if the row can be expanded or shrunk.
   */
  for (row = 0; row < priv->nrows; row++)
    {
      if (priv->rows[row].empty)
	{
	  priv->rows[row].expand = FALSE;
	  priv->rows[row].shrink = FALSE;
	}
      else
	{
	  if (priv->rows[row].need_expand)
	    priv->rows[row].expand = TRUE;
	  if (!priv->rows[row].need_shrink)
	    priv->rows[row].shrink = FALSE;
	}
    }
}

static void
ctk_table_size_allocate_pass1 (CtkTable *table)
{
  CtkTablePrivate *priv = table->priv;
  CtkAllocation allocation;
  gint real_width;
  gint real_height;
  gint width, height;
  gint row, col;
  gint nexpand;
  gint nshrink;
  gint extra;

  /* If we were allocated more space than we requested
   *  then we have to expand any expandable rows and columns
   *  to fill in the extra space.
   */
  ctk_widget_get_allocation (CTK_WIDGET (table), &allocation);
  real_width = allocation.width;
  real_height = allocation.height;

  if (priv->homogeneous)
    {
      if (!priv->children)
	nexpand = 1;
      else
	{
	  nexpand = 0;
	  for (col = 0; col < priv->ncols; col++)
	    if (priv->cols[col].expand)
	      {
		nexpand += 1;
		break;
	      }
	}
      if (nexpand)
	{
	  width = real_width;
	  for (col = 0; col + 1 < priv->ncols; col++)
	    width -= priv->cols[col].spacing;

	  for (col = 0; col < priv->ncols; col++)
	    {
	      extra = width / (priv->ncols - col);
	      priv->cols[col].allocation = MAX (1, extra);
	      width -= extra;
	    }
	}
    }
  else
    {
      width = 0;
      nexpand = 0;
      nshrink = 0;

      for (col = 0; col < priv->ncols; col++)
	{
	  width += priv->cols[col].requisition;
	  if (priv->cols[col].expand)
	    nexpand += 1;
	  if (priv->cols[col].shrink)
	    nshrink += 1;
	}
      for (col = 0; col + 1 < priv->ncols; col++)
	width += priv->cols[col].spacing;

      /* Check to see if we were allocated more width than we requested.
       */
      if ((width < real_width) && (nexpand >= 1))
	{
	  width = real_width - width;

	  for (col = 0; col < priv->ncols; col++)
	    if (priv->cols[col].expand)
	      {
		extra = width / nexpand;
		priv->cols[col].allocation += extra;

		width -= extra;
		nexpand -= 1;
	      }
	}
      
      /* Check to see if we were allocated less width than we requested,
       * then shrink until we fit the size give.
       */
      if (width > real_width)
	{
	  gint total_nshrink = nshrink;

	  extra = width - real_width;
	  while (total_nshrink > 0 && extra > 0)
	    {
	      nshrink = total_nshrink;
	      for (col = 0; col < priv->ncols; col++)
		if (priv->cols[col].shrink)
		  {
		    gint alloc = priv->cols[col].allocation;

		    priv->cols[col].allocation = MAX (1, (gint) priv->cols[col].allocation - extra / nshrink);
		    extra -= alloc - priv->cols[col].allocation;
		    nshrink -= 1;
		    if (priv->cols[col].allocation < 2)
		      {
			total_nshrink -= 1;
			priv->cols[col].shrink = FALSE;
		      }
		  }
	    }
	}
    }

  if (priv->homogeneous)
    {
      if (!priv->children)
	nexpand = 1;
      else
	{
	  nexpand = 0;
	  for (row = 0; row < priv->nrows; row++)
	    if (priv->rows[row].expand)
	      {
		nexpand += 1;
		break;
	      }
	}
      if (nexpand)
	{
	  height = real_height;

	  for (row = 0; row + 1 < priv->nrows; row++)
	    height -= priv->rows[row].spacing;

	  for (row = 0; row < priv->nrows; row++)
	    {
	      extra = height / (priv->nrows - row);
	      priv->rows[row].allocation = MAX (1, extra);
	      height -= extra;
	    }
	}
    }
  else
    {
      height = 0;
      nexpand = 0;
      nshrink = 0;

      for (row = 0; row < priv->nrows; row++)
	{
	  height += priv->rows[row].requisition;
	  if (priv->rows[row].expand)
	    nexpand += 1;
	  if (priv->rows[row].shrink)
	    nshrink += 1;
	}
      for (row = 0; row + 1 < priv->nrows; row++)
	height += priv->rows[row].spacing;

      /* Check to see if we were allocated more height than we requested.
       */
      if ((height < real_height) && (nexpand >= 1))
	{
	  height = real_height - height;

	  for (row = 0; row < priv->nrows; row++)
	    if (priv->rows[row].expand)
	      {
		extra = height / nexpand;
		priv->rows[row].allocation += extra;

		height -= extra;
		nexpand -= 1;
	      }
	}
      
      /* Check to see if we were allocated less height than we requested.
       * then shrink until we fit the size give.
       */
      if (height > real_height)
	{
	  gint total_nshrink = nshrink;
	  
	  extra = height - real_height;
	  while (total_nshrink > 0 && extra > 0)
	    {
	      nshrink = total_nshrink;
	      for (row = 0; row < priv->nrows; row++)
		if (priv->rows[row].shrink)
		  {
		    gint alloc = priv->rows[row].allocation;

		    priv->rows[row].allocation = MAX (1, (gint) priv->rows[row].allocation - extra / nshrink);
		    extra -= alloc - priv->rows[row].allocation;
		    nshrink -= 1;
		    if (priv->rows[row].allocation < 2)
		      {
			total_nshrink -= 1;
			priv->rows[row].shrink = FALSE;
		      }
		  }
	    }
	}
    }
}

static void
ctk_table_size_allocate_pass2 (CtkTable *table)
{
  CtkTablePrivate *priv = table->priv;
  CtkTableChild *child;
  GList *children;
  gint max_width;
  gint max_height;
  gint x, y;
  gint row, col;
  CtkAllocation allocation;
  CtkAllocation table_allocation, widget_allocation;
  CtkWidget *widget = CTK_WIDGET (table);

  children = priv->children;
  while (children)
    {
      child = children->data;
      children = children->next;
      
      if (ctk_widget_get_visible (child->widget))
	{
	  CtkRequisition child_requisition;

          ctk_widget_get_preferred_size (child->widget,
                                         &child_requisition, NULL);

          ctk_widget_get_allocation (CTK_WIDGET (table), &table_allocation);
	  x = table_allocation.x;
	  y = table_allocation.y;
	  max_width = 0;
	  max_height = 0;
	  
	  for (col = 0; col < child->left_attach; col++)
	    {
	      x += priv->cols[col].allocation;
	      x += priv->cols[col].spacing;
	    }
	  
	  for (col = child->left_attach; col < child->right_attach; col++)
	    {
	      max_width += priv->cols[col].allocation;
	      if ((col + 1) < child->right_attach)
		max_width += priv->cols[col].spacing;
	    }
	  
	  for (row = 0; row < child->top_attach; row++)
	    {
	      y += priv->rows[row].allocation;
	      y += priv->rows[row].spacing;
	    }
	  
	  for (row = child->top_attach; row < child->bottom_attach; row++)
	    {
	      max_height += priv->rows[row].allocation;
	      if ((row + 1) < child->bottom_attach)
		max_height += priv->rows[row].spacing;
	    }
	  
	  if (child->xfill)
	    {
	      allocation.width = MAX (1, max_width - (gint)child->xpadding * 2);
	      allocation.x = x + (max_width - allocation.width) / 2;
	    }
	  else
	    {
	      allocation.width = child_requisition.width;
	      allocation.x = x + (max_width - allocation.width) / 2;
	    }
	  
	  if (child->yfill)
	    {
	      allocation.height = MAX (1, max_height - (gint)child->ypadding * 2);
	      allocation.y = y + (max_height - allocation.height) / 2;
	    }
	  else
	    {
	      allocation.height = child_requisition.height;
	      allocation.y = y + (max_height - allocation.height) / 2;
	    }

          ctk_widget_get_allocation (widget, &widget_allocation);
	  if (ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL)
            allocation.x = widget_allocation.x + widget_allocation.width
                           - (allocation.x - widget_allocation.x) - allocation.width;

	  ctk_widget_size_allocate (child->widget, &allocation);
	}
    }
}
