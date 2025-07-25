/* ctkcelllayout.c
 * Copyright (C) 2003  Kristian Rietveld  <kris@gtk.org>
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
 * SECTION:ctkcelllayout
 * @Short_Description: An interface for packing cells
 * @Title: CtkCellLayout
 *
 * #CtkCellLayout is an interface to be implemented by all objects which
 * want to provide a #CtkTreeViewColumn like API for packing cells,
 * setting attributes and data funcs.
 *
 * One of the notable features provided by implementations of 
 * CtkCellLayout are attributes. Attributes let you set the properties
 * in flexible ways. They can just be set to constant values like regular
 * properties. But they can also be mapped to a column of the underlying
 * tree model with ctk_cell_layout_set_attributes(), which means that the value
 * of the attribute can change from cell to cell as they are rendered by
 * the cell renderer. Finally, it is possible to specify a function with
 * ctk_cell_layout_set_cell_data_func() that is called to determine the
 * value of the attribute for each cell that is rendered.
 *
 * # CtkCellLayouts as CtkBuildable
 *
 * Implementations of CtkCellLayout which also implement the CtkBuildable
 * interface (#CtkCellView, #CtkIconView, #CtkComboBox,
 * #CtkEntryCompletion, #CtkTreeViewColumn) accept CtkCellRenderer objects
 * as <child> elements in UI definitions. They support a custom <attributes>
 * element for their children, which can contain multiple <attribute>
 * elements. Each <attribute> element has a name attribute which specifies
 * a property of the cell renderer; the content of the element is the
 * attribute value.
 *
 * This is an example of a UI definition fragment specifying attributes:
 * |[
 * <object class="CtkCellView">
 *   <child>
 *     <object class="CtkCellRendererText"/>
 *     <attributes>
 *       <attribute name="text">0</attribute>
 *     </attributes>
 *   </child>"
 * </object>
 * ]|
 *
 * Furthermore for implementations of CtkCellLayout that use a #CtkCellArea
 * to lay out cells (all CtkCellLayouts in CTK+ use a CtkCellArea)
 * [cell properties][cell-properties] can also be defined in the format by
 * specifying the custom <cell-packing> attribute which can contain multiple
 * <property> elements defined in the normal way.
 *
 * Here is a UI definition fragment specifying cell properties:
 *
 * |[
 * <object class="CtkTreeViewColumn">
 *   <child>
 *     <object class="CtkCellRendererText"/>
 *     <cell-packing>
 *       <property name="align">True</property>
 *       <property name="expand">False</property>
 *     </cell-packing>
 *   </child>"
 * </object>
 * ]|
 *
 * # Subclassing CtkCellLayout implementations
 *
 * When subclassing a widget that implements #CtkCellLayout like
 * #CtkIconView or #CtkComboBox, there are some considerations related
 * to the fact that these widgets internally use a #CtkCellArea.
 * The cell area is exposed as a construct-only property by these
 * widgets. This means that it is possible to e.g. do
 *
 * |[<!-- language="C" -->
 * combo = g_object_new (CTK_TYPE_COMBO_BOX, "cell-area", my_cell_area, NULL);
 * ]|
 *
 * to use a custom cell area with a combo box. But construct properties
 * are only initialized after instance init()
 * functions have run, which means that using functions which rely on
 * the existence of the cell area in your subclass’ init() function will
 * cause the default cell area to be instantiated. In this case, a provided
 * construct property value will be ignored (with a warning, to alert
 * you to the problem).
 *
 * |[<!-- language="C" -->
 * static void
 * my_combo_box_init (MyComboBox *b)
 * {
 *   CtkCellRenderer *cell;
 *
 *   cell = ctk_cell_renderer_pixbuf_new ();
 *   // The following call causes the default cell area for combo boxes,
 *   // a CtkCellAreaBox, to be instantiated
 *   ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (b), cell, FALSE);
 *   ...
 * }
 *
 * CtkWidget *
 * my_combo_box_new (CtkCellArea *area)
 * {
 *   // This call is going to cause a warning about area being ignored
 *   return g_object_new (MY_TYPE_COMBO_BOX, "cell-area", area, NULL);
 * }
 * ]|
 *
 * If supporting alternative cell areas with your derived widget is
 * not important, then this does not have to concern you. If you want
 * to support alternative cell areas, you can do so by moving the
 * problematic calls out of init() and into a constructor()
 * for your class.
 */

#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "ctkcelllayout.h"
#include "ctkbuilderprivate.h"
#include "ctkintl.h"

#define warn_no_cell_area(func)					\
  g_critical ("%s: Called but no CtkCellArea is available yet", func)

typedef CtkCellLayoutIface CtkCellLayoutInterface;
G_DEFINE_INTERFACE (CtkCellLayout, ctk_cell_layout, G_TYPE_OBJECT);

static void   ctk_cell_layout_default_pack_start         (CtkCellLayout         *cell_layout,
							  CtkCellRenderer       *cell,
							  gboolean               expand);
static void   ctk_cell_layout_default_pack_end           (CtkCellLayout         *cell_layout,
							  CtkCellRenderer       *cell,
							  gboolean               expand);
static void   ctk_cell_layout_default_clear              (CtkCellLayout         *cell_layout);
static void   ctk_cell_layout_default_add_attribute      (CtkCellLayout         *cell_layout,
							  CtkCellRenderer       *cell,
							  const gchar           *attribute,
							  gint                   column);
static void   ctk_cell_layout_default_set_cell_data_func (CtkCellLayout         *cell_layout,
							  CtkCellRenderer       *cell,
							  CtkCellLayoutDataFunc  func,
							  gpointer               func_data,
							  GDestroyNotify         destroy);
static void   ctk_cell_layout_default_clear_attributes   (CtkCellLayout         *cell_layout,
							  CtkCellRenderer       *cell);
static void   ctk_cell_layout_default_reorder            (CtkCellLayout         *cell_layout,
							  CtkCellRenderer       *cell,
							  gint                   position);
static GList *ctk_cell_layout_default_get_cells          (CtkCellLayout         *cell_layout);


static void
ctk_cell_layout_default_init (CtkCellLayoutIface *iface)
{
  iface->pack_start         = ctk_cell_layout_default_pack_start;
  iface->pack_end           = ctk_cell_layout_default_pack_end;
  iface->clear              = ctk_cell_layout_default_clear;
  iface->add_attribute      = ctk_cell_layout_default_add_attribute;
  iface->set_cell_data_func = ctk_cell_layout_default_set_cell_data_func;
  iface->clear_attributes   = ctk_cell_layout_default_clear_attributes;
  iface->reorder            = ctk_cell_layout_default_reorder;
  iface->get_cells          = ctk_cell_layout_default_get_cells;
}

/* Default implementation is to fall back on an underlying cell area */
static void
ctk_cell_layout_default_pack_start (CtkCellLayout         *cell_layout,
				    CtkCellRenderer       *cell,
				    gboolean               expand)
{
  CtkCellLayoutIface *iface;

  iface = CTK_CELL_LAYOUT_GET_IFACE (cell_layout);

  if (iface->get_area)
    {
      CtkCellArea *area;

      area = iface->get_area (cell_layout);

      if (area)
	ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (area), cell, expand);
      else
	warn_no_cell_area ("CtkCellLayoutIface->pack_start()");
    }
}

static void
ctk_cell_layout_default_pack_end (CtkCellLayout         *cell_layout,
				  CtkCellRenderer       *cell,
				  gboolean               expand)
{
  CtkCellLayoutIface *iface;

  iface = CTK_CELL_LAYOUT_GET_IFACE (cell_layout);

  if (iface->get_area)
    {
      CtkCellArea *area;

      area = iface->get_area (cell_layout);

      if (area)
	ctk_cell_layout_pack_end (CTK_CELL_LAYOUT (area), cell, expand);
      else
	warn_no_cell_area ("CtkCellLayoutIface->pack_end()");
    }
}

static void
ctk_cell_layout_default_clear (CtkCellLayout *cell_layout)
{
  CtkCellLayoutIface *iface;

  iface = CTK_CELL_LAYOUT_GET_IFACE (cell_layout);

  if (iface->get_area)
    {
      CtkCellArea *area;

      area = iface->get_area (cell_layout);

      if (area)
	ctk_cell_layout_clear (CTK_CELL_LAYOUT (area));
      else
	warn_no_cell_area ("CtkCellLayoutIface->clear()");
    }
}

static void
ctk_cell_layout_default_add_attribute (CtkCellLayout         *cell_layout,
				       CtkCellRenderer       *cell,
				       const gchar           *attribute,
				       gint                   column)
{
  CtkCellLayoutIface *iface;

  iface = CTK_CELL_LAYOUT_GET_IFACE (cell_layout);

  if (iface->get_area)
    {
      CtkCellArea *area;

      area = iface->get_area (cell_layout);

      if (area)
	ctk_cell_layout_add_attribute (CTK_CELL_LAYOUT (area), cell, attribute, column);
      else
	warn_no_cell_area ("CtkCellLayoutIface->add_attribute()");
    }
}

static void
ctk_cell_layout_default_set_cell_data_func (CtkCellLayout         *cell_layout,
					    CtkCellRenderer       *cell,
					    CtkCellLayoutDataFunc  func,
					    gpointer               func_data,
					    GDestroyNotify         destroy)
{
  CtkCellLayoutIface *iface;

  iface = CTK_CELL_LAYOUT_GET_IFACE (cell_layout);

  if (iface->get_area)
    {
      CtkCellArea *area;

      area = iface->get_area (cell_layout);

      if (area)
	_ctk_cell_area_set_cell_data_func_with_proxy (area, cell, 
						      (GFunc)func, func_data, destroy, 
						      cell_layout);
      else
	warn_no_cell_area ("CtkCellLayoutIface->set_cell_data_func()");
    }
}

static void
ctk_cell_layout_default_clear_attributes (CtkCellLayout         *cell_layout,
					  CtkCellRenderer       *cell)
{
  CtkCellLayoutIface *iface;

  iface = CTK_CELL_LAYOUT_GET_IFACE (cell_layout);

  if (iface->get_area)
    {
      CtkCellArea *area;

      area = iface->get_area (cell_layout);

      if (area)
	ctk_cell_layout_clear_attributes (CTK_CELL_LAYOUT (area), cell);
      else
	warn_no_cell_area ("CtkCellLayoutIface->clear_attributes()");
    }
}

static void
ctk_cell_layout_default_reorder (CtkCellLayout         *cell_layout,
				 CtkCellRenderer       *cell,
				 gint                   position)
{
  CtkCellLayoutIface *iface;

  iface = CTK_CELL_LAYOUT_GET_IFACE (cell_layout);

  if (iface->get_area)
    {
      CtkCellArea *area;

      area = iface->get_area (cell_layout);

      if (area)
	ctk_cell_layout_reorder (CTK_CELL_LAYOUT (area), cell, position);
      else
	warn_no_cell_area ("CtkCellLayoutIface->reorder()");
    }
}

static GList *
ctk_cell_layout_default_get_cells (CtkCellLayout *cell_layout)
{
  CtkCellLayoutIface *iface;

  iface = CTK_CELL_LAYOUT_GET_IFACE (cell_layout);

  if (iface->get_area)
    {
      CtkCellArea *area;

      area = iface->get_area (cell_layout);

      if (area)
	return ctk_cell_layout_get_cells (CTK_CELL_LAYOUT (area));
      else
	warn_no_cell_area ("CtkCellLayoutIface->get_cells()");
    }
  return NULL;
}


/**
 * ctk_cell_layout_pack_start:
 * @cell_layout: a #CtkCellLayout
 * @cell: a #CtkCellRenderer
 * @expand: %TRUE if @cell is to be given extra space allocated to @cell_layout
 *
 * Packs the @cell into the beginning of @cell_layout. If @expand is %FALSE,
 * then the @cell is allocated no more space than it needs. Any unused space
 * is divided evenly between cells for which @expand is %TRUE.
 *
 * Note that reusing the same cell renderer is not supported.
 *
 * Since: 2.4
 */
void
ctk_cell_layout_pack_start (CtkCellLayout   *cell_layout,
                            CtkCellRenderer *cell,
                            gboolean         expand)
{
  g_return_if_fail (CTK_IS_CELL_LAYOUT (cell_layout));
  g_return_if_fail (CTK_IS_CELL_RENDERER (cell));

  CTK_CELL_LAYOUT_GET_IFACE (cell_layout)->pack_start (cell_layout, cell, expand);
}

/**
 * ctk_cell_layout_pack_end:
 * @cell_layout: a #CtkCellLayout
 * @cell: a #CtkCellRenderer
 * @expand: %TRUE if @cell is to be given extra space allocated to @cell_layout
 *
 * Adds the @cell to the end of @cell_layout. If @expand is %FALSE, then the
 * @cell is allocated no more space than it needs. Any unused space is
 * divided evenly between cells for which @expand is %TRUE.
 *
 * Note that reusing the same cell renderer is not supported.
 *
 * Since: 2.4
 */
void
ctk_cell_layout_pack_end (CtkCellLayout   *cell_layout,
                          CtkCellRenderer *cell,
                          gboolean         expand)
{
  g_return_if_fail (CTK_IS_CELL_LAYOUT (cell_layout));
  g_return_if_fail (CTK_IS_CELL_RENDERER (cell));

  CTK_CELL_LAYOUT_GET_IFACE (cell_layout)->pack_end (cell_layout, cell, expand);
}

/**
 * ctk_cell_layout_clear:
 * @cell_layout: a #CtkCellLayout
 *
 * Unsets all the mappings on all renderers on @cell_layout and
 * removes all renderers from @cell_layout.
 *
 * Since: 2.4
 */
void
ctk_cell_layout_clear (CtkCellLayout *cell_layout)
{
  g_return_if_fail (CTK_IS_CELL_LAYOUT (cell_layout));

  CTK_CELL_LAYOUT_GET_IFACE (cell_layout)->clear (cell_layout);
}

static void
ctk_cell_layout_set_attributesv (CtkCellLayout   *cell_layout,
                                 CtkCellRenderer *cell,
                                 va_list          args)
{
  gchar *attribute;
  gint column;

  attribute = va_arg (args, gchar *);

  ctk_cell_layout_clear_attributes (cell_layout, cell);

  while (attribute != NULL)
    {
      column = va_arg (args, gint);

      ctk_cell_layout_add_attribute (cell_layout, cell, attribute, column);

      attribute = va_arg (args, gchar *);
    }
}

/**
 * ctk_cell_layout_set_attributes:
 * @cell_layout: a #CtkCellLayout
 * @cell: a #CtkCellRenderer
 * @...: a %NULL-terminated list of attributes
 *
 * Sets the attributes in list as the attributes of @cell_layout.
 *
 * The attributes should be in attribute/column order, as in
 * ctk_cell_layout_add_attribute(). All existing attributes are
 * removed, and replaced with the new attributes.
 *
 * Since: 2.4
 */
void
ctk_cell_layout_set_attributes (CtkCellLayout   *cell_layout,
                                CtkCellRenderer *cell,
                                ...)
{
  va_list args;

  g_return_if_fail (CTK_IS_CELL_LAYOUT (cell_layout));
  g_return_if_fail (CTK_IS_CELL_RENDERER (cell));

  va_start (args, cell);
  ctk_cell_layout_set_attributesv (cell_layout, cell, args);
  va_end (args);
}

/**
 * ctk_cell_layout_add_attribute:
 * @cell_layout: a #CtkCellLayout
 * @cell: a #CtkCellRenderer
 * @attribute: an attribute on the renderer
 * @column: the column position on the model to get the attribute from
 *
 * Adds an attribute mapping to the list in @cell_layout.
 *
 * The @column is the column of the model to get a value from, and the
 * @attribute is the parameter on @cell to be set from the value. So for
 * example if column 2 of the model contains strings, you could have the
 * “text” attribute of a #CtkCellRendererText get its values from column 2.
 *
 * Since: 2.4
 */
void
ctk_cell_layout_add_attribute (CtkCellLayout   *cell_layout,
                               CtkCellRenderer *cell,
                               const gchar     *attribute,
                               gint             column)
{
  g_return_if_fail (CTK_IS_CELL_LAYOUT (cell_layout));
  g_return_if_fail (CTK_IS_CELL_RENDERER (cell));
  g_return_if_fail (attribute != NULL);
  g_return_if_fail (column >= 0);

  CTK_CELL_LAYOUT_GET_IFACE (cell_layout)->add_attribute (cell_layout, cell, attribute, column);
}

/**
 * ctk_cell_layout_set_cell_data_func:
 * @cell_layout: a #CtkCellLayout
 * @cell: a #CtkCellRenderer
 * @func: (allow-none): the #CtkCellLayoutDataFunc to use, or %NULL
 * @func_data: (closure): user data for @func
 * @destroy: destroy notify for @func_data
 *
 * Sets the #CtkCellLayoutDataFunc to use for @cell_layout.
 *
 * This function is used instead of the standard attributes mapping
 * for setting the column value, and should set the value of @cell_layout’s
 * cell renderer(s) as appropriate.
 *
 * @func may be %NULL to remove a previously set function.
 *
 * Since: 2.4
 */
void
ctk_cell_layout_set_cell_data_func (CtkCellLayout         *cell_layout,
                                    CtkCellRenderer       *cell,
                                    CtkCellLayoutDataFunc  func,
                                    gpointer               func_data,
                                    GDestroyNotify         destroy)
{
  g_return_if_fail (CTK_IS_CELL_LAYOUT (cell_layout));
  g_return_if_fail (CTK_IS_CELL_RENDERER (cell));

  CTK_CELL_LAYOUT_GET_IFACE 
    (cell_layout)->set_cell_data_func (cell_layout, cell, func, func_data, destroy);
}

/**
 * ctk_cell_layout_clear_attributes:
 * @cell_layout: a #CtkCellLayout
 * @cell: a #CtkCellRenderer to clear the attribute mapping on
 *
 * Clears all existing attributes previously set with
 * ctk_cell_layout_set_attributes().
 *
 * Since: 2.4
 */
void
ctk_cell_layout_clear_attributes (CtkCellLayout   *cell_layout,
                                  CtkCellRenderer *cell)
{
  g_return_if_fail (CTK_IS_CELL_LAYOUT (cell_layout));
  g_return_if_fail (CTK_IS_CELL_RENDERER (cell));

  CTK_CELL_LAYOUT_GET_IFACE (cell_layout)->clear_attributes (cell_layout, cell);
}

/**
 * ctk_cell_layout_reorder:
 * @cell_layout: a #CtkCellLayout
 * @cell: a #CtkCellRenderer to reorder
 * @position: new position to insert @cell at
 *
 * Re-inserts @cell at @position.
 *
 * Note that @cell has already to be packed into @cell_layout
 * for this to function properly.
 *
 * Since: 2.4
 */
void
ctk_cell_layout_reorder (CtkCellLayout   *cell_layout,
                         CtkCellRenderer *cell,
                         gint             position)
{
  g_return_if_fail (CTK_IS_CELL_LAYOUT (cell_layout));
  g_return_if_fail (CTK_IS_CELL_RENDERER (cell));

  CTK_CELL_LAYOUT_GET_IFACE (cell_layout)->reorder (cell_layout, cell, position);
}

/**
 * ctk_cell_layout_get_cells:
 * @cell_layout: a #CtkCellLayout
 *
 * Returns the cell renderers which have been added to @cell_layout.
 *
 * Returns: (element-type CtkCellRenderer) (transfer container):
 *     a list of cell renderers. The list, but not the renderers has
 *     been newly allocated and should be freed with g_list_free()
 *     when no longer needed.
 *
 * Since: 2.12
 */
GList *
ctk_cell_layout_get_cells (CtkCellLayout *cell_layout)
{
  g_return_val_if_fail (CTK_IS_CELL_LAYOUT (cell_layout), NULL);

  return CTK_CELL_LAYOUT_GET_IFACE (cell_layout)->get_cells (cell_layout);
}

/**
 * ctk_cell_layout_get_area:
 * @cell_layout: a #CtkCellLayout
 *
 * Returns the underlying #CtkCellArea which might be @cell_layout
 * if called on a #CtkCellArea or might be %NULL if no #CtkCellArea
 * is used by @cell_layout.
 *
 * Returns: (transfer none) (nullable): the cell area used by @cell_layout,
 * or %NULL in case no cell area is used.
 *
 * Since: 3.0
 */
CtkCellArea *
ctk_cell_layout_get_area (CtkCellLayout *cell_layout)
{
  CtkCellLayoutIface *iface;

  g_return_val_if_fail (CTK_IS_CELL_LAYOUT (cell_layout), NULL);

  iface = CTK_CELL_LAYOUT_GET_IFACE (cell_layout);  
  if (iface->get_area)
    return iface->get_area (cell_layout);

  return NULL;
}

/* Attribute parsing */
typedef struct {
  CtkCellLayout   *cell_layout;
  CtkCellRenderer *renderer;
  CtkBuilder      *builder;
  gchar           *attr_name;
  GString         *string;
} AttributesSubParserData;

static void
attributes_start_element (GMarkupParseContext *context,
			  const gchar         *element_name,
			  const gchar        **names,
			  const gchar        **values,
			  gpointer             user_data,
			  GError             **error)
{
  AttributesSubParserData *data = (AttributesSubParserData*)user_data;

  if (strcmp (element_name, "attribute") == 0)
    {
      const gchar *name;

      if (!_ctk_builder_check_parent (data->builder, context, "attributes", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_STRING, "name", &name,
                                        G_MARKUP_COLLECT_INVALID))
        {
          _ctk_builder_prefix_error (data->builder, context, error);
          return;
        }

      data->attr_name = g_strdup (name);
    }
  else if (strcmp (element_name, "attributes") == 0)
    {
      if (!_ctk_builder_check_parent (data->builder, context, "child", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_INVALID, NULL, NULL,
                                        G_MARKUP_COLLECT_INVALID))
        _ctk_builder_prefix_error (data->builder, context, error);
    }
  else
    {
      _ctk_builder_error_unhandled_tag (data->builder, context,
                                        "CtkCellLayout", element_name,
                                        error);
    }
}

static void
attributes_text_element (GMarkupParseContext  *context G_GNUC_UNUSED,
			 const gchar          *text,
			 gsize                 text_len,
			 gpointer              user_data,
			 GError              **error G_GNUC_UNUSED)
{
  AttributesSubParserData *data = (AttributesSubParserData*)user_data;

  if (data->attr_name)
    g_string_append_len (data->string, text, text_len);
}

static void
attributes_end_element (GMarkupParseContext  *context,
			const gchar          *element_name G_GNUC_UNUSED,
			gpointer              user_data,
			GError              **error)
{
  AttributesSubParserData *data = (AttributesSubParserData*)user_data;
  GValue val = G_VALUE_INIT;

  if (!data->attr_name)
    return;

  if (!ctk_builder_value_from_string_type (data->builder, G_TYPE_INT, data->string->str, &val, error))
    {
      _ctk_builder_prefix_error (data->builder, context, error);
       return;
    }

  ctk_cell_layout_add_attribute (data->cell_layout,
				 data->renderer,
				 data->attr_name,
                                 g_value_get_int (&val));

  g_free (data->attr_name);
  data->attr_name = NULL;

  g_string_set_size (data->string, 0);
}

static const GMarkupParser attributes_parser =
  {
    .start_element = attributes_start_element,
    .end_element = attributes_end_element,
    .text = attributes_text_element
  };


/* Cell packing parsing */
static void
ctk_cell_layout_buildable_set_cell_property (CtkCellArea     *area,
					     CtkBuilder      *builder,
					     CtkCellRenderer *cell,
					     gchar           *name,
					     const gchar     *value)
{
  GParamSpec *pspec;
  GValue gvalue = G_VALUE_INIT;
  GError *error = NULL;

  pspec = ctk_cell_area_class_find_cell_property (CTK_CELL_AREA_GET_CLASS (area), name);
  if (!pspec)
    {
      g_warning ("%s does not have a property called %s",
		 g_type_name (G_OBJECT_TYPE (area)), name);
      return;
    }

  if (!ctk_builder_value_from_string (builder, pspec, value, &gvalue, &error))
    {
      g_warning ("Could not read property %s:%s with value %s of type %s: %s",
		 g_type_name (G_OBJECT_TYPE (area)),
		 name,
		 value,
		 g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)),
		 error->message);
      g_error_free (error);
      return;
    }

  ctk_cell_area_cell_set_property (area, cell, name, &gvalue);
  g_value_unset (&gvalue);
}

typedef struct {
  CtkBuilder      *builder;
  CtkCellLayout   *cell_layout;
  CtkCellRenderer *renderer;
  GString         *string;
  gchar           *cell_prop_name;
  gchar           *context;
  gboolean         translatable;
} CellPackingSubParserData;

static void
cell_packing_start_element (GMarkupParseContext *context,
			    const gchar         *element_name,
			    const gchar        **names,
			    const gchar        **values,
			    gpointer             user_data,
			    GError             **error)
{
  CellPackingSubParserData *data = (CellPackingSubParserData*)user_data;

  if (strcmp (element_name, "property") == 0)
    {
      const gchar *name;
      gboolean translatable = FALSE;
      const gchar *ctx = NULL;

      if (!_ctk_builder_check_parent (data->builder, context, "cell-packing", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_STRING, "name", &name,
                                        G_MARKUP_COLLECT_BOOLEAN|G_MARKUP_COLLECT_OPTIONAL, "translatable", &translatable,
                                        G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, "comments", NULL,
                                        G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, "context", &ctx,
                                        G_MARKUP_COLLECT_INVALID))
       {
         _ctk_builder_prefix_error (data->builder, context, error);
         return;
       }

      data->cell_prop_name = g_strdup (name);
      data->translatable = translatable;
      data->context = g_strdup (ctx);
    }
  else if (strcmp (element_name, "cell-packing") == 0)
    {
      if (!_ctk_builder_check_parent (data->builder, context, "child", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_INVALID, NULL, NULL,
                                        G_MARKUP_COLLECT_INVALID))
        _ctk_builder_prefix_error (data->builder, context, error);
    }
  else
    {
      _ctk_builder_error_unhandled_tag (data->builder, context,
                                        "CtkCellLayout", element_name,
                                        error);
    }
}

static void
cell_packing_text_element (GMarkupParseContext *context G_GNUC_UNUSED,
			   const gchar         *text,
			   gsize                text_len,
			   gpointer             user_data,
			   GError             **error G_GNUC_UNUSED)
{
  CellPackingSubParserData *data = (CellPackingSubParserData*)user_data;

  if (data->cell_prop_name)
    g_string_append_len (data->string, text, text_len);
}

static void
cell_packing_end_element (GMarkupParseContext *context G_GNUC_UNUSED,
			  const gchar         *element_name G_GNUC_UNUSED,
			  gpointer             user_data,
			  GError             **error G_GNUC_UNUSED)
{
  CellPackingSubParserData *data = (CellPackingSubParserData*)user_data;
  CtkCellArea *area;

  area = ctk_cell_layout_get_area (data->cell_layout);

  if (area)
    {
      /* translate the string */
      if (data->string->len && data->translatable)
	{
	  const gchar *translated;
	  const gchar* domain;

	  domain = ctk_builder_get_translation_domain (data->builder);

	  translated = _ctk_builder_parser_translate (domain,
						      data->context,
						      data->string->str);
	  g_string_assign (data->string, translated);
	}

      if (data->cell_prop_name)
	ctk_cell_layout_buildable_set_cell_property (area,
						     data->builder,
						     data->renderer,
						     data->cell_prop_name,
						     data->string->str);
    }
  else
    g_warning ("%s does not have an internal CtkCellArea class and cannot apply child cell properties",
	       g_type_name (G_OBJECT_TYPE (data->cell_layout)));

  g_string_set_size (data->string, 0);
  g_free (data->cell_prop_name);
  g_free (data->context);
  data->cell_prop_name = NULL;
  data->context = NULL;
  data->translatable = FALSE;
}


static const GMarkupParser cell_packing_parser =
  {
    .start_element = cell_packing_start_element,
    .end_element = cell_packing_end_element,
    .text = cell_packing_text_element
  };

gboolean
_ctk_cell_layout_buildable_custom_tag_start (CtkBuildable  *buildable,
					     CtkBuilder    *builder,
					     GObject       *child,
					     const gchar   *tagname,
					     GMarkupParser *parser,
					     gpointer      *data)
{
  AttributesSubParserData  *attr_data;
  CellPackingSubParserData *packing_data;

  if (!child)
    return FALSE;

  if (strcmp (tagname, "attributes") == 0)
    {
      attr_data = g_slice_new0 (AttributesSubParserData);
      attr_data->cell_layout = CTK_CELL_LAYOUT (buildable);
      attr_data->renderer = CTK_CELL_RENDERER (child);
      attr_data->builder = builder;
      attr_data->attr_name = NULL;
      attr_data->string = g_string_new ("");

      *parser = attributes_parser;
      *data = attr_data;

      return TRUE;
    }
  else if (strcmp (tagname, "cell-packing") == 0)
    {
      packing_data = g_slice_new0 (CellPackingSubParserData);
      packing_data->string = g_string_new ("");
      packing_data->builder = builder;
      packing_data->cell_layout = CTK_CELL_LAYOUT (buildable);
      packing_data->renderer = CTK_CELL_RENDERER (child);

      *parser = cell_packing_parser;
      *data = packing_data;

      return TRUE;
    }

  return FALSE;
}

gboolean
_ctk_cell_layout_buildable_custom_tag_end (CtkBuildable *buildable G_GNUC_UNUSED,
					   CtkBuilder   *builder G_GNUC_UNUSED,
					   GObject      *child G_GNUC_UNUSED,
					   const gchar  *tagname,
					   gpointer     *data)
{
  if (strcmp (tagname, "attributes") == 0)
    {
      AttributesSubParserData *attr_data;

      attr_data = (AttributesSubParserData*)data;
      g_assert (!attr_data->attr_name);
      g_string_free (attr_data->string, TRUE);
      g_slice_free (AttributesSubParserData, attr_data);

      return TRUE;
    }
  else if (strcmp (tagname, "cell-packing") == 0)
    {
      CellPackingSubParserData *packing_data;

      packing_data = (CellPackingSubParserData *)data;
      g_string_free (packing_data->string, TRUE);
      g_slice_free (CellPackingSubParserData, packing_data);

      return TRUE;
    }
  return FALSE;
}

void
_ctk_cell_layout_buildable_add_child (CtkBuildable      *buildable,
				      CtkBuilder        *builder G_GNUC_UNUSED,
				      GObject           *child,
				      const gchar       *type G_GNUC_UNUSED)
{
  g_return_if_fail (CTK_IS_CELL_LAYOUT (buildable));
  g_return_if_fail (CTK_IS_CELL_RENDERER (child));

  ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (buildable), CTK_CELL_RENDERER (child), FALSE);
}
