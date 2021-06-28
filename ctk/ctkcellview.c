/* ctkellview.c
 * Copyright (C) 2002, 2003  Kristian Rietveld <kris@ctk.org>
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
#include "ctkcellview.h"
#include "ctkcelllayout.h"
#include "ctkcellareabox.h"
#include "ctkintl.h"
#include "ctkcellrenderertext.h"
#include "ctkcellrendererpixbuf.h"
#include "ctkprivate.h"
#include "ctkorientableprivate.h"
#include "ctkrender.h"
#include "ctkcsscustomgadgetprivate.h"
#include "ctkwidgetprivate.h"
#include <gobject/gmarshal.h>
#include "ctkbuildable.h"


/**
 * SECTION:ctkcellview
 * @Short_description: A widget displaying a single row of a CtkTreeModel
 * @Title: CtkCellView
 *
 * A #CtkCellView displays a single row of a #CtkTreeModel using a #CtkCellArea
 * and #CtkCellAreaContext. A #CtkCellAreaContext can be provided to the 
 * #CtkCellView at construction time in order to keep the cellview in context
 * of a group of cell views, this ensures that the renderers displayed will
 * be properly aligned with eachother (like the aligned cells in the menus
 * of #CtkComboBox).
 *
 * #CtkCellView is #CtkOrientable in order to decide in which orientation
 * the underlying #CtkCellAreaContext should be allocated. Taking the #CtkComboBox
 * menu as an example, cellviews should be oriented horizontally if the menus are
 * listed top-to-bottom and thus all share the same width but may have separate
 * individual heights (left-to-right menus should be allocated vertically since
 * they all share the same height but may have variable widths).
 *
 * # CSS nodes
 *
 * CtkCellView has a single CSS node with name cellview.
 */

static void        ctk_cell_view_constructed              (GObject          *object);
static void        ctk_cell_view_get_property             (GObject          *object,
                                                           guint             param_id,
                                                           GValue           *value,
                                                           GParamSpec       *pspec);
static void        ctk_cell_view_set_property             (GObject          *object,
                                                           guint             param_id,
                                                           const GValue     *value,
                                                           GParamSpec       *pspec);
static void        ctk_cell_view_finalize                 (GObject          *object);
static void        ctk_cell_view_dispose                  (GObject          *object);
static void        ctk_cell_view_size_allocate            (CtkWidget        *widget,
                                                           CtkAllocation    *allocation);
static gboolean    ctk_cell_view_draw                     (CtkWidget        *widget,
                                                           cairo_t          *cr);
static void        ctk_cell_view_set_value                (CtkCellView     *cell_view,
                                                           CtkCellRenderer *renderer,
                                                           gchar           *property,
                                                           GValue          *value);
static void        ctk_cell_view_set_cell_data            (CtkCellView      *cell_view);

/* celllayout */
static void        ctk_cell_view_cell_layout_init         (CtkCellLayoutIface *iface);
static CtkCellArea *ctk_cell_view_cell_layout_get_area         (CtkCellLayout         *layout);


/* buildable */
static void       ctk_cell_view_buildable_init                 (CtkBuildableIface     *iface);
static gboolean   ctk_cell_view_buildable_custom_tag_start     (CtkBuildable  	      *buildable,
								CtkBuilder    	      *builder,
								GObject       	      *child,
								const gchar   	      *tagname,
								GMarkupParser 	      *parser,
								gpointer      	      *data);
static void       ctk_cell_view_buildable_custom_tag_end       (CtkBuildable  	      *buildable,
								CtkBuilder    	      *builder,
								GObject       	      *child,
								const gchar   	      *tagname,
								gpointer      	      *data);

static CtkSizeRequestMode ctk_cell_view_get_request_mode       (CtkWidget             *widget);
static void       ctk_cell_view_get_preferred_width            (CtkWidget             *widget,
								gint                  *minimum_size,
								gint                  *natural_size);
static void       ctk_cell_view_get_preferred_height           (CtkWidget             *widget,
								gint                  *minimum_size,
								gint                  *natural_size);
static void       ctk_cell_view_get_preferred_width_for_height (CtkWidget             *widget,
								gint                   avail_size,
								gint                  *minimum_size,
								gint                  *natural_size);
static void       ctk_cell_view_get_preferred_height_for_width (CtkWidget             *widget,
								gint                   avail_size,
								gint                  *minimum_size,
								gint                  *natural_size);

static void       context_size_changed_cb                      (CtkCellAreaContext   *context,
								GParamSpec           *pspec,
								CtkWidget            *view);
static void       row_changed_cb                               (CtkTreeModel         *model,
								CtkTreePath          *path,
								CtkTreeIter          *iter,
								CtkCellView          *view);

static void     ctk_cell_view_measure  (CtkCssGadget        *gadget,
                                        CtkOrientation       orientation,
                                        int                  for_size,
                                        int                 *minimum,
                                        int                 *natural,
                                        int                 *minimum_baseline,
                                        int                 *natural_baseline,
                                        gpointer             data);
static void     ctk_cell_view_allocate (CtkCssGadget        *gadget,
                                        const CtkAllocation *allocation,
                                        int                  baseline,
                                        CtkAllocation       *out_clip,
                                        gpointer             data);
static gboolean ctk_cell_view_render   (CtkCssGadget        *gadget,
                                        cairo_t             *cr,
                                        int                  x,
                                        int                  y,
                                        int                  width,
                                        int                  height,
                                        gpointer             data);

struct _CtkCellViewPrivate
{
  CtkTreeModel        *model;
  CtkTreeRowReference *displayed_row;

  CtkCellArea         *area;
  CtkCellAreaContext  *context;

  CtkCssGadget        *gadget;

  GdkRGBA              background;

  gulong               size_changed_id;
  gulong               row_changed_id;

  CtkOrientation       orientation;

  guint                background_set : 1;
  guint                draw_sensitive : 1;
  guint                fit_model      : 1;
};

static CtkBuildableIface *parent_buildable_iface;

enum
{
  PROP_0,
  PROP_ORIENTATION,
  PROP_BACKGROUND,
  PROP_BACKGROUND_GDK,
  PROP_BACKGROUND_RGBA,
  PROP_BACKGROUND_SET,
  PROP_MODEL,
  PROP_CELL_AREA,
  PROP_CELL_AREA_CONTEXT,
  PROP_DRAW_SENSITIVE,
  PROP_FIT_MODEL
};

G_DEFINE_TYPE_WITH_CODE (CtkCellView, ctk_cell_view, CTK_TYPE_WIDGET,
                         G_ADD_PRIVATE (CtkCellView)
			 G_IMPLEMENT_INTERFACE (CTK_TYPE_CELL_LAYOUT,
						ctk_cell_view_cell_layout_init)
			 G_IMPLEMENT_INTERFACE (CTK_TYPE_BUILDABLE,
						ctk_cell_view_buildable_init)
			 G_IMPLEMENT_INTERFACE (CTK_TYPE_ORIENTABLE, NULL))

static void
ctk_cell_view_class_init (CtkCellViewClass *klass)
{
  GObjectClass   *gobject_class = G_OBJECT_CLASS (klass);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);

  gobject_class->constructed = ctk_cell_view_constructed;
  gobject_class->get_property = ctk_cell_view_get_property;
  gobject_class->set_property = ctk_cell_view_set_property;
  gobject_class->finalize = ctk_cell_view_finalize;
  gobject_class->dispose = ctk_cell_view_dispose;

  widget_class->draw                           = ctk_cell_view_draw;
  widget_class->size_allocate                  = ctk_cell_view_size_allocate;
  widget_class->get_request_mode               = ctk_cell_view_get_request_mode;
  widget_class->get_preferred_width            = ctk_cell_view_get_preferred_width;
  widget_class->get_preferred_height           = ctk_cell_view_get_preferred_height;
  widget_class->get_preferred_width_for_height = ctk_cell_view_get_preferred_width_for_height;
  widget_class->get_preferred_height_for_width = ctk_cell_view_get_preferred_height_for_width;

  /* properties */
  g_object_class_override_property (gobject_class, PROP_ORIENTATION, "orientation");

  g_object_class_install_property (gobject_class,
                                   PROP_BACKGROUND,
                                   g_param_spec_string ("background",
                                                        P_("Background color name"),
                                                        P_("Background color as a string"),
                                                        NULL,
                                                        CTK_PARAM_WRITABLE));

  /**
   * CtkCellView:background-cdk:
   *
   * The background color as a #GdkColor
   *
   * Deprecated: 3.4: Use #CtkCellView:background-rgba instead.
   */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  g_object_class_install_property (gobject_class,
                                   PROP_BACKGROUND_GDK,
                                   g_param_spec_boxed ("background-cdk",
                                                      P_("Background color"),
                                                      P_("Background color as a GdkColor"),
                                                      GDK_TYPE_COLOR,
                                                      CTK_PARAM_READWRITE | G_PARAM_DEPRECATED));
G_GNUC_END_IGNORE_DEPRECATIONS
  /**
   * CtkCellView:background-rgba:
   *
   * The background color as a #GdkRGBA
   *
   * Since: 3.0
   */
  g_object_class_install_property (gobject_class,
                                   PROP_BACKGROUND_RGBA,
                                   g_param_spec_boxed ("background-rgba",
                                                      P_("Background RGBA color"),
                                                      P_("Background color as a GdkRGBA"),
                                                      GDK_TYPE_RGBA,
                                                      CTK_PARAM_READWRITE));

  /**
   * CtkCellView:model:
   *
   * The model for cell view
   *
   * since 2.10
   */
  g_object_class_install_property (gobject_class,
				   PROP_MODEL,
				   g_param_spec_object  ("model",
							 P_("CellView model"),
							 P_("The model for cell view"),
							 CTK_TYPE_TREE_MODEL,
							 CTK_PARAM_READWRITE));


  /**
   * CtkCellView:cell-area:
   *
   * The #CtkCellArea rendering cells
   *
   * If no area is specified when creating the cell view with ctk_cell_view_new_with_context() 
   * a horizontally oriented #CtkCellAreaBox will be used.
   *
   * since 3.0
   */
   g_object_class_install_property (gobject_class,
                                    PROP_CELL_AREA,
                                    g_param_spec_object ("cell-area",
							 P_("Cell Area"),
							 P_("The CtkCellArea used to layout cells"),
							 CTK_TYPE_CELL_AREA,
							 CTK_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  /**
   * CtkCellView:cell-area-context:
   *
   * The #CtkCellAreaContext used to compute the geometry of the cell view.
   *
   * A group of cell views can be assigned the same context in order to
   * ensure the sizes and cell alignments match across all the views with
   * the same context.
   *
   * #CtkComboBox menus uses this to assign the same context to all cell views
   * in the menu items for a single menu (each submenu creates its own
   * context since the size of each submenu does not depend on parent
   * or sibling menus).
   *
   * since 3.0
   */
   g_object_class_install_property (gobject_class,
                                    PROP_CELL_AREA_CONTEXT,
                                    g_param_spec_object ("cell-area-context",
							 P_("Cell Area Context"),
							 P_("The CtkCellAreaContext used to "
							    "compute the geometry of the cell view"),
							 CTK_TYPE_CELL_AREA_CONTEXT,
							 CTK_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  /**
   * CtkCellView:draw-sensitive:
   *
   * Whether all cells should be draw as sensitive for this view regardless
   * of the actual cell properties (used to make menus with submenus appear
   * sensitive when the items in submenus might be insensitive).
   *
   * since 3.0
   */
   g_object_class_install_property (gobject_class,
                                    PROP_DRAW_SENSITIVE,
                                    g_param_spec_boolean ("draw-sensitive",
							  P_("Draw Sensitive"),
							  P_("Whether to force cells to be drawn in a "
							     "sensitive state"),
							  FALSE,
							  CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkCellView:fit-model:
   *
   * Whether the view should request enough space to always fit
   * the size of every row in the model (used by the combo box to
   * ensure the combo box size doesnt change when different items
   * are selected).
   *
   * since 3.0
   */
   g_object_class_install_property (gobject_class,
                                    PROP_FIT_MODEL,
                                    g_param_spec_boolean ("fit-model",
							  P_("Fit Model"),
							  P_("Whether to request enough space for "
							     "every row in the model"),
							  FALSE,
							  CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  
#define ADD_SET_PROP(propname, propval, nick, blurb) g_object_class_install_property (gobject_class, propval, g_param_spec_boolean (propname, nick, blurb, FALSE, CTK_PARAM_READWRITE))

  ADD_SET_PROP ("background-set", PROP_BACKGROUND_SET,
                P_("Background set"),
                P_("Whether this tag affects the background color"));

  ctk_widget_class_set_css_name (widget_class, "cellview");
}

static void
ctk_cell_view_buildable_init (CtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->add_child = _ctk_cell_layout_buildable_add_child;
  iface->custom_tag_start = ctk_cell_view_buildable_custom_tag_start;
  iface->custom_tag_end = ctk_cell_view_buildable_custom_tag_end;
}

static void
ctk_cell_view_cell_layout_init (CtkCellLayoutIface *iface)
{
  iface->get_area = ctk_cell_view_cell_layout_get_area;
}

static void
ctk_cell_view_constructed (GObject *object)
{
  CtkCellView *view = CTK_CELL_VIEW (object);
  CtkCellViewPrivate *priv = view->priv;

  G_OBJECT_CLASS (ctk_cell_view_parent_class)->constructed (object);

  if (!priv->area)
    {
      priv->area = ctk_cell_area_box_new ();
      g_object_ref_sink (priv->area);
    }

  if (!priv->context)
    priv->context = ctk_cell_area_create_context (priv->area);

  priv->size_changed_id =
    g_signal_connect (priv->context, "notify",
		      G_CALLBACK (context_size_changed_cb), view);
}

static void
ctk_cell_view_get_property (GObject    *object,
                            guint       param_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  CtkCellView *view = CTK_CELL_VIEW (object);

  switch (param_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, view->priv->orientation);
      break;
    case PROP_BACKGROUND_GDK:
      {
	GdkColor color;
	
	color.red = (guint) (view->priv->background.red * 65535);
	color.green = (guint) (view->priv->background.green * 65535);
	color.blue = (guint) (view->priv->background.blue * 65535);
	color.pixel = 0;
	
	g_value_set_boxed (value, &color);
      }
      break;
    case PROP_BACKGROUND_RGBA:
      g_value_set_boxed (value, &view->priv->background);
      break;
    case PROP_BACKGROUND_SET:
      g_value_set_boolean (value, view->priv->background_set);
      break;
    case PROP_MODEL:
      g_value_set_object (value, view->priv->model);
      break;
    case PROP_CELL_AREA:
      g_value_set_object (value, view->priv->area);
      break;
    case PROP_CELL_AREA_CONTEXT:
      g_value_set_object (value, view->priv->context);
      break;
    case PROP_DRAW_SENSITIVE:
      g_value_set_boolean (value, view->priv->draw_sensitive);
      break;
    case PROP_FIT_MODEL:
      g_value_set_boolean (value, view->priv->fit_model);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
ctk_cell_view_set_property (GObject      *object,
                            guint         param_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  CtkCellView *view = CTK_CELL_VIEW (object);
  CtkCellViewPrivate *priv = view->priv;
  CtkCellArea *area;
  CtkCellAreaContext *context;

  switch (param_id)
    {
    case PROP_ORIENTATION:
      if (priv->orientation != g_value_get_enum (value))
        {
          priv->orientation = g_value_get_enum (value);
          if (priv->context)
            ctk_cell_area_context_reset (priv->context);
          _ctk_orientable_set_style_classes (CTK_ORIENTABLE (object));
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_BACKGROUND:
      {
        GdkRGBA color;

	if (!g_value_get_string (value))
          ctk_cell_view_set_background_rgba (view, NULL);
	else if (cdk_rgba_parse (&color, g_value_get_string (value)))
          ctk_cell_view_set_background_rgba (view, &color);
	else
	  g_warning ("Don't know color '%s'", g_value_get_string (value));

        g_object_notify (object, "background-rgba");
        g_object_notify (object, "background-cdk");
      }
      break;
    case PROP_BACKGROUND_GDK:
      {
        GdkColor *color;
        GdkRGBA rgba;

        color = g_value_get_boxed (value);

        rgba.red = color->red / 65535.0;
        rgba.green = color->green / 65535.0;
        rgba.blue = color->blue / 65535.0;
        rgba.alpha = 1.0;

        ctk_cell_view_set_background_rgba (view, &rgba);
      }
      break;
    case PROP_BACKGROUND_RGBA:
      ctk_cell_view_set_background_rgba (view, g_value_get_boxed (value));
      break;
    case PROP_BACKGROUND_SET:
      view->priv->background_set = g_value_get_boolean (value);
      break;
    case PROP_MODEL:
      ctk_cell_view_set_model (view, g_value_get_object (value));
      break;
    case PROP_CELL_AREA:
      /* Construct-only, can only be assigned once */
      area = g_value_get_object (value);

      if (area)
        {
          if (priv->area != NULL)
            {
              g_warning ("cell-area has already been set, ignoring construct property");
              g_object_ref_sink (area);
              g_object_unref (area);
            }
          else
            priv->area = g_object_ref_sink (area);
        }
      break;
    case PROP_CELL_AREA_CONTEXT:
      /* Construct-only, can only be assigned once */
      context = g_value_get_object (value);

      if (context)
        {
          if (priv->context != NULL)
            {
              g_warning ("cell-area-context has already been set, ignoring construct property");
              g_object_ref_sink (context);
              g_object_unref (context);
            }
          else
            priv->context = g_object_ref (context);
        }
      break;

    case PROP_DRAW_SENSITIVE:
      ctk_cell_view_set_draw_sensitive (view, g_value_get_boolean (value));
      break;

    case PROP_FIT_MODEL:
      ctk_cell_view_set_fit_model (view, g_value_get_boolean (value));
      break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
        break;
    }
}

static void
ctk_cell_view_init (CtkCellView *cellview)
{
  CtkCssNode *widget_node;

  cellview->priv = ctk_cell_view_get_instance_private (cellview);
  cellview->priv->orientation = CTK_ORIENTATION_HORIZONTAL;

  ctk_widget_set_has_window (CTK_WIDGET (cellview), FALSE);

  widget_node = ctk_widget_get_css_node (CTK_WIDGET (cellview));
  cellview->priv->gadget = ctk_css_custom_gadget_new_for_node (widget_node,
                                                               CTK_WIDGET (cellview),
                                                               ctk_cell_view_measure,
                                                               ctk_cell_view_allocate,
                                                               ctk_cell_view_render,
                                                               NULL,
                                                               NULL);
}

static void
ctk_cell_view_finalize (GObject *object)
{
  CtkCellView *cellview = CTK_CELL_VIEW (object);

  if (cellview->priv->displayed_row)
     ctk_tree_row_reference_free (cellview->priv->displayed_row);

  g_clear_object (&cellview->priv->gadget);

  G_OBJECT_CLASS (ctk_cell_view_parent_class)->finalize (object);
}

static void
ctk_cell_view_dispose (GObject *object)
{
  CtkCellView *cellview = CTK_CELL_VIEW (object);

  ctk_cell_view_set_model (cellview, NULL);

  if (cellview->priv->area)
    {
      g_object_unref (cellview->priv->area);
      cellview->priv->area = NULL;
    }

  if (cellview->priv->context)
    {
      g_signal_handler_disconnect (cellview->priv->context, cellview->priv->size_changed_id);

      g_object_unref (cellview->priv->context);
      cellview->priv->context = NULL;
      cellview->priv->size_changed_id = 0;
    }

  G_OBJECT_CLASS (ctk_cell_view_parent_class)->dispose (object);
}

static void
ctk_cell_view_size_allocate (CtkWidget     *widget,
                             CtkAllocation *allocation)
{
  CtkAllocation clip;

  ctk_widget_set_allocation (widget, allocation);

  ctk_css_gadget_allocate (CTK_CELL_VIEW (widget)->priv->gadget,
                           allocation,
                           ctk_widget_get_allocated_baseline (widget),
                           &clip);

  ctk_widget_set_clip (widget, &clip);
}

static void
ctk_cell_view_allocate (CtkCssGadget        *gadget,
                        const CtkAllocation *allocation,
                        int                  baseline,
                        CtkAllocation       *out_clip,
                        gpointer             data)
{
  CtkWidget *widget;
  CtkCellView *cellview;
  CtkCellViewPrivate *priv;
  gint alloc_width, alloc_height, width, height;

  widget = ctk_css_gadget_get_owner (gadget);
  cellview = CTK_CELL_VIEW (widget);
  priv = cellview->priv;

  width = allocation->width;
  height = allocation->height;

  ctk_cell_area_context_get_allocation (priv->context, &alloc_width, &alloc_height);

  /* The first cell view in context is responsible for allocating the context at
   * allocate time (or the cellview has its own context and is not grouped with
   * any other cell views)
   *
   * If the cellview is in "fit model" mode, we assume it's not in context and
   * needs to allocate every time.
   */
  if (priv->fit_model)
    ctk_cell_area_context_allocate (priv->context, width, height);
  else if (alloc_width != allocation->width && priv->orientation == CTK_ORIENTATION_HORIZONTAL)
    ctk_cell_area_context_allocate (priv->context, width, -1);
  else if (alloc_height != allocation->height && priv->orientation == CTK_ORIENTATION_VERTICAL)
    ctk_cell_area_context_allocate (priv->context, -1, height);

  *out_clip = *allocation;
}

static void
ctk_cell_view_request_model (CtkCellView        *cellview,
			     CtkTreeIter        *parent,
			     CtkOrientation      orientation,
			     gint                for_size,
			     gint               *minimum_size,
			     gint               *natural_size)
{
  CtkCellViewPrivate *priv = cellview->priv;
  CtkTreeIter         iter;
  gboolean            valid;

  if (!priv->model)
    return;

  valid = ctk_tree_model_iter_children (priv->model, &iter, parent);
  while (valid)
    {
      gint min, nat;

      ctk_cell_area_apply_attributes (priv->area, priv->model, &iter, FALSE, FALSE);

      if (orientation == CTK_ORIENTATION_HORIZONTAL)
	{
	  if (for_size < 0)
	    ctk_cell_area_get_preferred_width (priv->area, priv->context, 
					       CTK_WIDGET (cellview), &min, &nat);
	  else
	    ctk_cell_area_get_preferred_width_for_height (priv->area, priv->context, 
							  CTK_WIDGET (cellview), for_size, &min, &nat);
	}
      else
	{
	  if (for_size < 0)
	    ctk_cell_area_get_preferred_height (priv->area, priv->context, 
						CTK_WIDGET (cellview), &min, &nat);
	  else
	    ctk_cell_area_get_preferred_height_for_width (priv->area, priv->context, 
							  CTK_WIDGET (cellview), for_size, &min, &nat);
	}

      *minimum_size = MAX (min, *minimum_size);
      *natural_size = MAX (nat, *natural_size);

      /* Recurse into children when they exist */
      ctk_cell_view_request_model (cellview, &iter, orientation, for_size, minimum_size, natural_size);

      valid = ctk_tree_model_iter_next (priv->model, &iter);
    }
}

static CtkSizeRequestMode 
ctk_cell_view_get_request_mode (CtkWidget *widget)
{
  CtkCellView        *cellview = CTK_CELL_VIEW (widget);
  CtkCellViewPrivate *priv = cellview->priv;

  return ctk_cell_area_get_request_mode (priv->area);
}

static void
ctk_cell_view_get_preferred_width (CtkWidget *widget,
                                   gint      *minimum,
                                   gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CTK_CELL_VIEW (widget)->priv->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_cell_view_get_preferred_width_for_height (CtkWidget *widget,
                                              gint       height,
                                              gint      *minimum,
                                              gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CTK_CELL_VIEW (widget)->priv->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     height,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_cell_view_get_preferred_height (CtkWidget *widget,
                                    gint      *minimum,
                                    gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CTK_CELL_VIEW (widget)->priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_cell_view_get_preferred_height_for_width (CtkWidget *widget,
                                              gint       width,
                                              gint      *minimum,
                                              gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CTK_CELL_VIEW (widget)->priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     width,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_cell_view_measure (CtkCssGadget   *gadget,
                       CtkOrientation  orientation,
                       int             for_size,
                       int            *minimum,
                       int            *natural,
                       int            *minimum_baseline,
                       int            *natural_baseline,
                       gpointer        data)
{
  CtkWidget *widget;
  CtkCellView *cellview;
  CtkCellViewPrivate *priv;

  widget = ctk_css_gadget_get_owner (gadget);
  cellview = CTK_CELL_VIEW (widget);
  priv = cellview->priv;

  g_signal_handler_block (priv->context, priv->size_changed_id);

  if (orientation == CTK_ORIENTATION_HORIZONTAL && for_size == -1)
    {
      if (priv->fit_model)
        {
          gint min = 0, nat = 0;
          ctk_cell_view_request_model (cellview, NULL, CTK_ORIENTATION_HORIZONTAL, -1, &min, &nat);
        }
      else
        {
          if (cellview->priv->displayed_row)
            ctk_cell_view_set_cell_data (cellview);

          ctk_cell_area_get_preferred_width (priv->area, priv->context, widget, NULL, NULL);
        }

      ctk_cell_area_context_get_preferred_width (priv->context, minimum, natural);
    }
  else if (orientation == CTK_ORIENTATION_VERTICAL && for_size == -1)
    {
      if (priv->fit_model)
        {
          gint min = 0, nat = 0;
          ctk_cell_view_request_model (cellview, NULL, CTK_ORIENTATION_VERTICAL, -1, &min, &nat);
        }
      else
        {
          if (cellview->priv->displayed_row)
            ctk_cell_view_set_cell_data (cellview);

          ctk_cell_area_get_preferred_height (priv->area, priv->context, widget, NULL, NULL);
        }

      ctk_cell_area_context_get_preferred_height (priv->context, minimum, natural);
    }
  else if (orientation == CTK_ORIENTATION_HORIZONTAL && for_size >= 0)
    {
      if (priv->fit_model)
        {
          gint min = 0, nat = 0;
          ctk_cell_view_request_model (cellview, NULL, CTK_ORIENTATION_HORIZONTAL, for_size, &min, &nat);

          *minimum = min;
          *natural = nat;
        }
      else
        {
          if (cellview->priv->displayed_row)
            ctk_cell_view_set_cell_data (cellview);

          ctk_cell_area_get_preferred_width_for_height (priv->area, priv->context, widget,
                                                        for_size, minimum, natural);
        }
    }
  else
   {
      if (priv->fit_model)
        {
          gint min = 0, nat = 0;
          ctk_cell_view_request_model (cellview, NULL, CTK_ORIENTATION_VERTICAL, for_size, &min, &nat);

          *minimum = min;
          *natural = nat;
        }
      else
        {
          if (cellview->priv->displayed_row)
            ctk_cell_view_set_cell_data (cellview);

          ctk_cell_area_get_preferred_height_for_width (priv->area, priv->context, widget,
                                                        for_size, minimum, natural);
        }
    }

  g_signal_handler_unblock (priv->context, priv->size_changed_id);
}

static gboolean
ctk_cell_view_draw (CtkWidget *widget,
                    cairo_t   *cr)
{
  ctk_css_gadget_draw (CTK_CELL_VIEW (widget)->priv->gadget, cr);

  return FALSE;
}

static gboolean
ctk_cell_view_render (CtkCssGadget *gadget,
                      cairo_t      *cr,
                      int           x,
                      int           y,
                      int           width,
                      int           height,
                      gpointer      data)
{
  CtkWidget *widget;
  CtkCellView *cellview;
  GdkRectangle area;
  CtkCellRendererState state;

  widget = ctk_css_gadget_get_owner (gadget);
  cellview = CTK_CELL_VIEW (widget);

  /* render cells */
  area.x = x;
  area.y = y;
  area.width  = width;
  area.height = height;

  /* "blank" background */
  if (cellview->priv->background_set)
    {
      cdk_cairo_rectangle (cr, &area);
      cdk_cairo_set_source_rgba (cr, &cellview->priv->background);
      cairo_fill (cr);
    }

  /* set cell data (if available) */
  if (cellview->priv->displayed_row)
    ctk_cell_view_set_cell_data (cellview);
  else if (cellview->priv->model)
    return FALSE;

  if (ctk_widget_get_state_flags (widget) & CTK_STATE_FLAG_PRELIGHT)
    state = CTK_CELL_RENDERER_PRELIT;
  else
    state = 0;

  /* Render the cells */
  ctk_cell_area_render (cellview->priv->area, cellview->priv->context,
			widget, cr, &area, &area, state, FALSE);

  return FALSE;
}

static void
ctk_cell_view_set_cell_data (CtkCellView *cell_view)
{
  CtkTreeIter iter;
  CtkTreePath *path;

  g_return_if_fail (cell_view->priv->displayed_row != NULL);

  path = ctk_tree_row_reference_get_path (cell_view->priv->displayed_row);
  if (!path)
    return;

  ctk_tree_model_get_iter (cell_view->priv->model, &iter, path);
  ctk_tree_path_free (path);

  ctk_cell_area_apply_attributes (cell_view->priv->area, 
				  cell_view->priv->model, 
				  &iter, FALSE, FALSE);

  if (cell_view->priv->draw_sensitive)
    {
      GList *l, *cells = 
	ctk_cell_layout_get_cells (CTK_CELL_LAYOUT (cell_view->priv->area));

      for (l = cells; l; l = l->next)
	{
	  GObject *renderer = l->data;

	  g_object_set (renderer, "sensitive", TRUE, NULL);
	}
      g_list_free (cells);
    }
}

/* CtkCellLayout implementation */
static CtkCellArea *
ctk_cell_view_cell_layout_get_area (CtkCellLayout   *layout)
{
  CtkCellView *cellview = CTK_CELL_VIEW (layout);
  CtkCellViewPrivate *priv = cellview->priv;

  if (G_UNLIKELY (!priv->area))
    {
      priv->area = ctk_cell_area_box_new ();
      g_object_ref_sink (priv->area);
    }

  return priv->area;
}

/* CtkBuildable implementation */
static gboolean
ctk_cell_view_buildable_custom_tag_start (CtkBuildable  *buildable,
					  CtkBuilder    *builder,
					  GObject       *child,
					  const gchar   *tagname,
					  GMarkupParser *parser,
					  gpointer      *data)
{
  if (parent_buildable_iface->custom_tag_start &&
      parent_buildable_iface->custom_tag_start (buildable, builder, child,
						tagname, parser, data))
    return TRUE;

  return _ctk_cell_layout_buildable_custom_tag_start (buildable, builder, child,
						      tagname, parser, data);
}

static void
ctk_cell_view_buildable_custom_tag_end (CtkBuildable *buildable,
					CtkBuilder   *builder,
					GObject      *child,
					const gchar  *tagname,
					gpointer     *data)
{
  if (_ctk_cell_layout_buildable_custom_tag_end (buildable, builder, child, tagname,
						 data))
    return;
  else if (parent_buildable_iface->custom_tag_end)
    parent_buildable_iface->custom_tag_end (buildable, builder, child, tagname,
					    data);
}

static void
context_size_changed_cb (CtkCellAreaContext  *context,
			 GParamSpec          *pspec,
			 CtkWidget           *view)
{
  if (!strcmp (pspec->name, "minimum-width") ||
      !strcmp (pspec->name, "natural-width") ||
      !strcmp (pspec->name, "minimum-height") ||
      !strcmp (pspec->name, "natural-height"))
    ctk_widget_queue_resize (view);
}

static void
row_changed_cb (CtkTreeModel         *model,
		CtkTreePath          *path,
		CtkTreeIter          *iter,
		CtkCellView          *view)
{
  CtkTreePath *row_path;

  if (view->priv->displayed_row)
    {
      row_path = 
	ctk_tree_row_reference_get_path (view->priv->displayed_row);

      if (row_path)
	{
	  /* Resize everything in our context if our row changed */
	  if (ctk_tree_path_compare (row_path, path) == 0)
	    ctk_cell_area_context_reset (view->priv->context);

	  ctk_tree_path_free (row_path);
	}
    }
}

/**
 * ctk_cell_view_new:
 *
 * Creates a new #CtkCellView widget.
 *
 * Returns: A newly created #CtkCellView widget.
 *
 * Since: 2.6
 */
CtkWidget *
ctk_cell_view_new (void)
{
  CtkCellView *cellview;

  cellview = g_object_new (ctk_cell_view_get_type (), NULL);

  return CTK_WIDGET (cellview);
}


/**
 * ctk_cell_view_new_with_context:
 * @area: the #CtkCellArea to layout cells
 * @context: the #CtkCellAreaContext in which to calculate cell geometry
 *
 * Creates a new #CtkCellView widget with a specific #CtkCellArea
 * to layout cells and a specific #CtkCellAreaContext.
 *
 * Specifying the same context for a handfull of cells lets
 * the underlying area synchronize the geometry for those cells,
 * in this way alignments with cellviews for other rows are
 * possible.
 *
 * Returns: A newly created #CtkCellView widget.
 *
 * Since: 2.6
 */
CtkWidget *
ctk_cell_view_new_with_context (CtkCellArea        *area,
				CtkCellAreaContext *context)
{
  g_return_val_if_fail (CTK_IS_CELL_AREA (area), NULL);
  g_return_val_if_fail (context == NULL || CTK_IS_CELL_AREA_CONTEXT (context), NULL);

  return (CtkWidget *)g_object_new (CTK_TYPE_CELL_VIEW, 
				    "cell-area", area,
				    "cell-area-context", context,
				    NULL);
}

/**
 * ctk_cell_view_new_with_text:
 * @text: the text to display in the cell view
 *
 * Creates a new #CtkCellView widget, adds a #CtkCellRendererText 
 * to it, and makes it show @text.
 *
 * Returns: A newly created #CtkCellView widget.
 *
 * Since: 2.6
 */
CtkWidget *
ctk_cell_view_new_with_text (const gchar *text)
{
  CtkCellView *cellview;
  CtkCellRenderer *renderer;
  GValue value = G_VALUE_INIT;

  cellview = CTK_CELL_VIEW (ctk_cell_view_new ());

  renderer = ctk_cell_renderer_text_new ();
  ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (cellview),
			      renderer, TRUE);

  g_value_init (&value, G_TYPE_STRING);
  g_value_set_string (&value, text);
  ctk_cell_view_set_value (cellview, renderer, "text", &value);
  g_value_unset (&value);

  return CTK_WIDGET (cellview);
}

/**
 * ctk_cell_view_new_with_markup:
 * @markup: the text to display in the cell view
 *
 * Creates a new #CtkCellView widget, adds a #CtkCellRendererText 
 * to it, and makes it show @markup. The text can be
 * marked up with the [Pango text markup language][PangoMarkupFormat].
 *
 * Returns: A newly created #CtkCellView widget.
 *
 * Since: 2.6
 */
CtkWidget *
ctk_cell_view_new_with_markup (const gchar *markup)
{
  CtkCellView *cellview;
  CtkCellRenderer *renderer;
  GValue value = G_VALUE_INIT;

  cellview = CTK_CELL_VIEW (ctk_cell_view_new ());

  renderer = ctk_cell_renderer_text_new ();
  ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (cellview),
			      renderer, TRUE);

  g_value_init (&value, G_TYPE_STRING);
  g_value_set_string (&value, markup);
  ctk_cell_view_set_value (cellview, renderer, "markup", &value);
  g_value_unset (&value);

  return CTK_WIDGET (cellview);
}

/**
 * ctk_cell_view_new_with_pixbuf:
 * @pixbuf: the image to display in the cell view
 *
 * Creates a new #CtkCellView widget, adds a #CtkCellRendererPixbuf
 * to it, and makes it show @pixbuf.
 *
 * Returns: A newly created #CtkCellView widget.
 *
 * Since: 2.6
 */
CtkWidget *
ctk_cell_view_new_with_pixbuf (GdkPixbuf *pixbuf)
{
  CtkCellView *cellview;
  CtkCellRenderer *renderer;
  GValue value = G_VALUE_INIT;

  cellview = CTK_CELL_VIEW (ctk_cell_view_new ());

  renderer = ctk_cell_renderer_pixbuf_new ();
  ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (cellview),
			      renderer, TRUE);

  g_value_init (&value, GDK_TYPE_PIXBUF);
  g_value_set_object (&value, pixbuf);
  ctk_cell_view_set_value (cellview, renderer, "pixbuf", &value);
  g_value_unset (&value);

  return CTK_WIDGET (cellview);
}

/**
 * ctk_cell_view_set_value:
 * @cell_view: a #CtkCellView widget
 * @renderer: one of the renderers of @cell_view
 * @property: the name of the property of @renderer to set
 * @value: the new value to set the property to
 * 
 * Sets a property of a cell renderer of @cell_view, and
 * makes sure the display of @cell_view is updated.
 *
 * Since: 2.6
 */
static void
ctk_cell_view_set_value (CtkCellView     *cell_view,
                         CtkCellRenderer *renderer,
                         gchar           *property,
                         GValue          *value)
{
  g_object_set_property (G_OBJECT (renderer), property, value);

  /* force resize and redraw */
  ctk_widget_queue_resize (CTK_WIDGET (cell_view));
  ctk_widget_queue_draw (CTK_WIDGET (cell_view));
}

/**
 * ctk_cell_view_set_model:
 * @cell_view: a #CtkCellView
 * @model: (allow-none): a #CtkTreeModel
 *
 * Sets the model for @cell_view.  If @cell_view already has a model
 * set, it will remove it before setting the new model.  If @model is
 * %NULL, then it will unset the old model.
 *
 * Since: 2.6
 */
void
ctk_cell_view_set_model (CtkCellView  *cell_view,
                         CtkTreeModel *model)
{
  g_return_if_fail (CTK_IS_CELL_VIEW (cell_view));
  g_return_if_fail (model == NULL || CTK_IS_TREE_MODEL (model));

  if (cell_view->priv->model)
    {
      g_signal_handler_disconnect (cell_view->priv->model, 
				   cell_view->priv->row_changed_id);
      cell_view->priv->row_changed_id = 0;

      if (cell_view->priv->displayed_row)
        ctk_tree_row_reference_free (cell_view->priv->displayed_row);
      cell_view->priv->displayed_row = NULL;

      g_object_unref (cell_view->priv->model);
    }

  cell_view->priv->model = model;

  if (cell_view->priv->model)
    {
      g_object_ref (cell_view->priv->model);

      cell_view->priv->row_changed_id = 
	g_signal_connect (cell_view->priv->model, "row-changed",
			  G_CALLBACK (row_changed_cb), cell_view);
    }
}

/**
 * ctk_cell_view_get_model:
 * @cell_view: a #CtkCellView
 *
 * Returns the model for @cell_view. If no model is used %NULL is
 * returned.
 *
 * Returns: (nullable) (transfer none): a #CtkTreeModel used or %NULL
 *
 * Since: 2.16
 **/
CtkTreeModel *
ctk_cell_view_get_model (CtkCellView *cell_view)
{
  g_return_val_if_fail (CTK_IS_CELL_VIEW (cell_view), NULL);

  return cell_view->priv->model;
}

/**
 * ctk_cell_view_set_displayed_row:
 * @cell_view: a #CtkCellView
 * @path: (allow-none): a #CtkTreePath or %NULL to unset.
 *
 * Sets the row of the model that is currently displayed
 * by the #CtkCellView. If the path is unset, then the
 * contents of the cellview “stick” at their last value;
 * this is not normally a desired result, but may be
 * a needed intermediate state if say, the model for
 * the #CtkCellView becomes temporarily empty.
 *
 * Since: 2.6
 **/
void
ctk_cell_view_set_displayed_row (CtkCellView *cell_view,
                                 CtkTreePath *path)
{
  g_return_if_fail (CTK_IS_CELL_VIEW (cell_view));
  g_return_if_fail (CTK_IS_TREE_MODEL (cell_view->priv->model));

  if (cell_view->priv->displayed_row)
    ctk_tree_row_reference_free (cell_view->priv->displayed_row);

  if (path)
    {
      cell_view->priv->displayed_row =
	ctk_tree_row_reference_new (cell_view->priv->model, path);
    }
  else
    cell_view->priv->displayed_row = NULL;

  /* force resize and redraw */
  ctk_widget_queue_resize (CTK_WIDGET (cell_view));
  ctk_widget_queue_draw (CTK_WIDGET (cell_view));
}

/**
 * ctk_cell_view_get_displayed_row:
 * @cell_view: a #CtkCellView
 *
 * Returns a #CtkTreePath referring to the currently 
 * displayed row. If no row is currently displayed, 
 * %NULL is returned.
 *
 * Returns: (nullable) (transfer full): the currently displayed row or %NULL
 *
 * Since: 2.6
 */
CtkTreePath *
ctk_cell_view_get_displayed_row (CtkCellView *cell_view)
{
  g_return_val_if_fail (CTK_IS_CELL_VIEW (cell_view), NULL);

  if (!cell_view->priv->displayed_row)
    return NULL;

  return ctk_tree_row_reference_get_path (cell_view->priv->displayed_row);
}

/**
 * ctk_cell_view_get_size_of_row:
 * @cell_view: a #CtkCellView
 * @path: a #CtkTreePath 
 * @requisition: (out): return location for the size 
 *
 * Sets @requisition to the size needed by @cell_view to display 
 * the model row pointed to by @path.
 * 
 * Returns: %TRUE
 *
 * Since: 2.6
 * 
 * Deprecated: 3.0: Combo box formerly used this to calculate the
 * sizes for cellviews, now you can achieve this by either using
 * the #CtkCellView:fit-model property or by setting the currently
 * displayed row of the #CtkCellView and using ctk_widget_get_preferred_size().
 */
gboolean
ctk_cell_view_get_size_of_row (CtkCellView    *cell_view,
                               CtkTreePath    *path,
                               CtkRequisition *requisition)
{
  CtkTreeRowReference *tmp;
  CtkRequisition req;

  g_return_val_if_fail (CTK_IS_CELL_VIEW (cell_view), FALSE);
  g_return_val_if_fail (path != NULL, FALSE);

  tmp = cell_view->priv->displayed_row;
  cell_view->priv->displayed_row =
    ctk_tree_row_reference_new (cell_view->priv->model, path);

  ctk_widget_get_preferred_width (CTK_WIDGET (cell_view), &req.width, NULL);
  ctk_widget_get_preferred_height_for_width (CTK_WIDGET (cell_view), req.width, &req.height, NULL);

  ctk_tree_row_reference_free (cell_view->priv->displayed_row);
  cell_view->priv->displayed_row = tmp;

  if (requisition)
    *requisition = req;

  return TRUE;
}

/**
 * ctk_cell_view_set_background_color:
 * @cell_view: a #CtkCellView
 * @color: the new background color
 *
 * Sets the background color of @view.
 *
 * Since: 2.6
 *
 * Deprecated: 3.4: Use ctk_cell_view_set_background_rgba() instead.
 */
void
ctk_cell_view_set_background_color (CtkCellView    *cell_view,
                                    const GdkColor *color)
{
  g_return_if_fail (CTK_IS_CELL_VIEW (cell_view));

  if (color)
    {
      if (!cell_view->priv->background_set)
        {
          cell_view->priv->background_set = TRUE;
          g_object_notify (G_OBJECT (cell_view), "background-set");
        }

      cell_view->priv->background.red = color->red / 65535.;
      cell_view->priv->background.green = color->green / 65535.;
      cell_view->priv->background.blue = color->blue / 65535.;
      cell_view->priv->background.alpha = 1;
    }
  else
    {
      if (cell_view->priv->background_set)
        {
          cell_view->priv->background_set = FALSE;
          g_object_notify (G_OBJECT (cell_view), "background-set");
        }
    }

  ctk_widget_queue_draw (CTK_WIDGET (cell_view));
}

/**
 * ctk_cell_view_set_background_rgba:
 * @cell_view: a #CtkCellView
 * @rgba: the new background color
 *
 * Sets the background color of @cell_view.
 *
 * Since: 3.0
 */
void
ctk_cell_view_set_background_rgba (CtkCellView   *cell_view,
                                   const GdkRGBA *rgba)
{
  g_return_if_fail (CTK_IS_CELL_VIEW (cell_view));

  if (rgba)
    {
      if (!cell_view->priv->background_set)
        {
          cell_view->priv->background_set = TRUE;
          g_object_notify (G_OBJECT (cell_view), "background-set");
        }

      cell_view->priv->background = *rgba;
    }
  else
    {
      if (cell_view->priv->background_set)
        {
          cell_view->priv->background_set = FALSE;
          g_object_notify (G_OBJECT (cell_view), "background-set");
        }
    }

  ctk_widget_queue_draw (CTK_WIDGET (cell_view));
}

/**
 * ctk_cell_view_get_draw_sensitive:
 * @cell_view: a #CtkCellView
 *
 * Gets whether @cell_view is configured to draw all of its
 * cells in a sensitive state.
 *
 * Returns: whether @cell_view draws all of its
 * cells in a sensitive state
 *
 * Since: 3.0
 */
gboolean
ctk_cell_view_get_draw_sensitive (CtkCellView     *cell_view)
{
  CtkCellViewPrivate *priv;

  g_return_val_if_fail (CTK_IS_CELL_VIEW (cell_view), FALSE);

  priv = cell_view->priv;

  return priv->draw_sensitive;
}

/**
 * ctk_cell_view_set_draw_sensitive:
 * @cell_view: a #CtkCellView
 * @draw_sensitive: whether to draw all cells in a sensitive state.
 *
 * Sets whether @cell_view should draw all of its
 * cells in a sensitive state, this is used by #CtkComboBox menus
 * to ensure that rows with insensitive cells that contain
 * children appear sensitive in the parent menu item.
 *
 * Since: 3.0
 */
void
ctk_cell_view_set_draw_sensitive (CtkCellView     *cell_view,
				  gboolean         draw_sensitive)
{
  CtkCellViewPrivate *priv;

  g_return_if_fail (CTK_IS_CELL_VIEW (cell_view));

  priv = cell_view->priv;

  if (priv->draw_sensitive != draw_sensitive)
    {
      priv->draw_sensitive = draw_sensitive;

      g_object_notify (G_OBJECT (cell_view), "draw-sensitive");
    }
}

/**
 * ctk_cell_view_get_fit_model:
 * @cell_view: a #CtkCellView
 *
 * Gets whether @cell_view is configured to request space
 * to fit the entire #CtkTreeModel.
 *
 * Returns: whether @cell_view requests space to fit
 * the entire #CtkTreeModel.
 *
 * Since: 3.0
 */
gboolean
ctk_cell_view_get_fit_model (CtkCellView     *cell_view)
{
  CtkCellViewPrivate *priv;

  g_return_val_if_fail (CTK_IS_CELL_VIEW (cell_view), FALSE);

  priv = cell_view->priv;

  return priv->fit_model;
}

/**
 * ctk_cell_view_set_fit_model:
 * @cell_view: a #CtkCellView
 * @fit_model: whether @cell_view should request space for the whole model.
 *
 * Sets whether @cell_view should request space to fit the entire #CtkTreeModel.
 *
 * This is used by #CtkComboBox to ensure that the cell view displayed on
 * the combo box’s button always gets enough space and does not resize
 * when selection changes.
 *
 * Since: 3.0
 */
void
ctk_cell_view_set_fit_model (CtkCellView *cell_view,
                             gboolean     fit_model)
{
  CtkCellViewPrivate *priv;

  g_return_if_fail (CTK_IS_CELL_VIEW (cell_view));

  priv = cell_view->priv;

  if (priv->fit_model != fit_model)
    {
      priv->fit_model = fit_model;

      ctk_cell_area_context_reset (cell_view->priv->context);

      g_object_notify (G_OBJECT (cell_view), "fit-model");
    }
}
