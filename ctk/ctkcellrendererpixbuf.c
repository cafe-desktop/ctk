/* ctkcellrendererpixbuf.c
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
#include <stdlib.h>
#include <cairo-gobject.h>
#include "ctkcellrendererpixbuf.h"
#include "ctkiconfactory.h"
#include "ctkiconhelperprivate.h"
#include "ctkicontheme.h"
#include "ctkintl.h"
#include "ctkprivate.h"
#include "ctkstylecontextprivate.h"
#include "a11y/ctkimagecellaccessible.h"


/**
 * SECTION:ctkcellrendererpixbuf
 * @Short_description: Renders a pixbuf in a cell
 * @Title: CtkCellRendererPixbuf
 *
 * A #CtkCellRendererPixbuf can be used to render an image in a cell. It allows
 * to render either a given #GdkPixbuf (set via the
 * #CtkCellRendererPixbuf:pixbuf property) or a named icon (set via the
 * #CtkCellRendererPixbuf:icon-name property).
 *
 * To support the tree view, #CtkCellRendererPixbuf also supports rendering two
 * alternative pixbufs, when the #CtkCellRenderer:is-expander property is %TRUE.
 * If the #CtkCellRenderer:is-expanded property is %TRUE and the
 * #CtkCellRendererPixbuf:pixbuf-expander-open property is set to a pixbuf, it
 * renders that pixbuf, if the #CtkCellRenderer:is-expanded property is %FALSE
 * and the #CtkCellRendererPixbuf:pixbuf-expander-closed property is set to a
 * pixbuf, it renders that one.
 */


static void ctk_cell_renderer_pixbuf_get_property  (GObject                    *object,
						    guint                       param_id,
						    GValue                     *value,
						    GParamSpec                 *pspec);
static void ctk_cell_renderer_pixbuf_set_property  (GObject                    *object,
						    guint                       param_id,
						    const GValue               *value,
						    GParamSpec                 *pspec);
static void ctk_cell_renderer_pixbuf_get_size   (CtkCellRenderer            *cell,
						 CtkWidget                  *widget,
						 const CdkRectangle         *rectangle,
						 gint                       *x_offset,
						 gint                       *y_offset,
						 gint                       *width,
						 gint                       *height);
static void ctk_cell_renderer_pixbuf_render     (CtkCellRenderer            *cell,
						 cairo_t                    *cr,
						 CtkWidget                  *widget,
						 const CdkRectangle         *background_area,
						 const CdkRectangle         *cell_area,
						 CtkCellRendererState        flags);


enum {
  PROP_0,
  PROP_PIXBUF,
  PROP_PIXBUF_EXPANDER_OPEN,
  PROP_PIXBUF_EXPANDER_CLOSED,
  PROP_SURFACE,
  PROP_STOCK_ID,
  PROP_STOCK_SIZE,
  PROP_STOCK_DETAIL,
  PROP_FOLLOW_STATE,
  PROP_ICON_NAME,
  PROP_GICON
};


struct _CtkCellRendererPixbufPrivate
{
  CtkImageDefinition *image_def;
  CtkIconSize         icon_size;

  GdkPixbuf *pixbuf_expander_open;
  GdkPixbuf *pixbuf_expander_closed;

  gboolean follow_state;

  gchar *stock_detail;
};

G_DEFINE_TYPE_WITH_PRIVATE (CtkCellRendererPixbuf, ctk_cell_renderer_pixbuf, CTK_TYPE_CELL_RENDERER)

static void
ctk_cell_renderer_pixbuf_init (CtkCellRendererPixbuf *cellpixbuf)
{
  CtkCellRendererPixbufPrivate *priv;

  cellpixbuf->priv = ctk_cell_renderer_pixbuf_get_instance_private (cellpixbuf);
  priv = cellpixbuf->priv;

  priv->image_def = ctk_image_definition_new_empty ();
  priv->icon_size = CTK_ICON_SIZE_MENU;
  priv->follow_state = TRUE;
}

static void
ctk_cell_renderer_pixbuf_finalize (GObject *object)
{
  CtkCellRendererPixbuf *cellpixbuf = CTK_CELL_RENDERER_PIXBUF (object);
  CtkCellRendererPixbufPrivate *priv = cellpixbuf->priv;

  ctk_image_definition_unref (priv->image_def);

  if (priv->pixbuf_expander_open)
    g_object_unref (priv->pixbuf_expander_open);
  if (priv->pixbuf_expander_closed)
    g_object_unref (priv->pixbuf_expander_closed);

  g_free (priv->stock_detail);

  G_OBJECT_CLASS (ctk_cell_renderer_pixbuf_parent_class)->finalize (object);
}

static void
ctk_cell_renderer_pixbuf_class_init (CtkCellRendererPixbufClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  CtkCellRendererClass *cell_class = CTK_CELL_RENDERER_CLASS (class);

  object_class->finalize = ctk_cell_renderer_pixbuf_finalize;

  object_class->get_property = ctk_cell_renderer_pixbuf_get_property;
  object_class->set_property = ctk_cell_renderer_pixbuf_set_property;

  cell_class->get_size = ctk_cell_renderer_pixbuf_get_size;
  cell_class->render = ctk_cell_renderer_pixbuf_render;

  g_object_class_install_property (object_class,
				   PROP_PIXBUF,
				   g_param_spec_object ("pixbuf",
							P_("Pixbuf Object"),
							P_("The pixbuf to render"),
							GDK_TYPE_PIXBUF,
							CTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_PIXBUF_EXPANDER_OPEN,
				   g_param_spec_object ("pixbuf-expander-open",
							P_("Pixbuf Expander Open"),
							P_("Pixbuf for open expander"),
							GDK_TYPE_PIXBUF,
							CTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_PIXBUF_EXPANDER_CLOSED,
				   g_param_spec_object ("pixbuf-expander-closed",
							P_("Pixbuf Expander Closed"),
							P_("Pixbuf for closed expander"),
							GDK_TYPE_PIXBUF,
							CTK_PARAM_READWRITE));
  /**
   * CtkCellRendererPixbuf:surface:
   *
   * Since: 3.10
   */
  g_object_class_install_property (object_class,
				   PROP_SURFACE,
				   g_param_spec_boxed ("surface",
						       P_("surface"),
						       P_("The surface to render"),
						       CAIRO_GOBJECT_TYPE_SURFACE,
						       CTK_PARAM_READWRITE));

  /**
   * CtkCellRendererPixbuf:stock-id:
   *
   * Since: 2.2
   *
   * Deprecated: 3.10: Use #CtkCellRendererPixbuf:icon-name instead.
   */
  g_object_class_install_property (object_class,
				   PROP_STOCK_ID,
				   g_param_spec_string ("stock-id",
							P_("Stock ID"),
							P_("The stock ID of the stock icon to render"),
							NULL,
							CTK_PARAM_READWRITE | G_PARAM_DEPRECATED));

  /**
   * CtkCellRendererPixbuf:stock-size:
   *
   * The #CtkIconSize value that specifies the size of the rendered icon.
   *
   * Since: 2.2
   */
  g_object_class_install_property (object_class,
				   PROP_STOCK_SIZE,
				   g_param_spec_uint ("stock-size",
						      P_("Size"),
						      P_("The CtkIconSize value that specifies the size of the rendered icon"),
						      0,
						      G_MAXUINT,
						      CTK_ICON_SIZE_MENU,
						      CTK_PARAM_READWRITE));

  /*
   * CtkCellRendererPixbuf:stock-detail:
   *
   * Since: 2.2
   *
   * Deprecated: 3.22: This property does nothing. Use CSS to theme widgets.
   */
  g_object_class_install_property (object_class,
				   PROP_STOCK_DETAIL,
				   g_param_spec_string ("stock-detail",
							P_("Detail"),
							P_("Render detail to pass to the theme engine"),
							NULL,
							CTK_PARAM_READWRITE));

  
  /**
   * CtkCellRendererPixbuf:icon-name:
   *
   * The name of the themed icon to display.
   * This property only has an effect if not overridden by "stock_id" 
   * or "pixbuf" properties.
   *
   * Since: 2.8 
   */
  g_object_class_install_property (object_class,
				   PROP_ICON_NAME,
				   g_param_spec_string ("icon-name",
							P_("Icon Name"),
							P_("The name of the icon from the icon theme"),
							NULL,
							CTK_PARAM_READWRITE));

  /**
   * CtkCellRendererPixbuf:follow-state:
   *
   * Specifies whether the rendered pixbuf should be colorized
   * according to the #CtkCellRendererState.
   *
   * Since: 2.8
   *
   * Deprecated: 3.16: Cell renderers always follow state.
   */
  g_object_class_install_property (object_class,
				   PROP_FOLLOW_STATE,
				   g_param_spec_boolean ("follow-state",
 							 P_("Follow State"),
 							 P_("Whether the rendered pixbuf should be "
							    "colorized according to the state"),
 							 TRUE,
 							 CTK_PARAM_READWRITE | G_PARAM_DEPRECATED));

  /**
   * CtkCellRendererPixbuf:gicon:
   *
   * The GIcon representing the icon to display.
   * If the icon theme is changed, the image will be updated
   * automatically.
   *
   * Since: 2.14
   */
  g_object_class_install_property (object_class,
                                   PROP_GICON,
                                   g_param_spec_object ("gicon",
                                                        P_("Icon"),
                                                        P_("The GIcon being displayed"),
                                                        G_TYPE_ICON,
                                                        CTK_PARAM_READWRITE));



  ctk_cell_renderer_class_set_accessible_type (cell_class, CTK_TYPE_IMAGE_CELL_ACCESSIBLE);
}

static void
ctk_cell_renderer_pixbuf_get_property (GObject        *object,
				       guint           param_id,
				       GValue         *value,
				       GParamSpec     *pspec)
{
  CtkCellRendererPixbuf *cellpixbuf = CTK_CELL_RENDERER_PIXBUF (object);
  CtkCellRendererPixbufPrivate *priv = cellpixbuf->priv;

  switch (param_id)
    {
    case PROP_PIXBUF:
      g_value_set_object (value, ctk_image_definition_get_pixbuf (priv->image_def));
      break;
    case PROP_PIXBUF_EXPANDER_OPEN:
      g_value_set_object (value, priv->pixbuf_expander_open);
      break;
    case PROP_PIXBUF_EXPANDER_CLOSED:
      g_value_set_object (value, priv->pixbuf_expander_closed);
      break;
    case PROP_SURFACE:
      g_value_set_boxed (value, ctk_image_definition_get_surface (priv->image_def));
      break;
    case PROP_STOCK_ID:
      g_value_set_string (value, ctk_image_definition_get_stock (priv->image_def));
      break;
    case PROP_STOCK_SIZE:
      g_value_set_uint (value, priv->icon_size);
      break;
    case PROP_STOCK_DETAIL:
      g_value_set_string (value, priv->stock_detail);
      break;
    case PROP_FOLLOW_STATE:
      g_value_set_boolean (value, priv->follow_state);
      break;
    case PROP_ICON_NAME:
      g_value_set_string (value, ctk_image_definition_get_icon_name (priv->image_def));
      break;
    case PROP_GICON:
      g_value_set_object (value, ctk_image_definition_get_gicon (priv->image_def));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
notify_storage_type (CtkCellRendererPixbuf *cellpixbuf,
                     CtkImageType           storage_type)
{
  switch (storage_type)
    {
    case CTK_IMAGE_SURFACE:
      g_object_notify (G_OBJECT (cellpixbuf), "surface");
      break;
    case CTK_IMAGE_PIXBUF:
      g_object_notify (G_OBJECT (cellpixbuf), "pixbuf");
      break;
    case CTK_IMAGE_STOCK:
      g_object_notify (G_OBJECT (cellpixbuf), "stock-id");
      break;
    case CTK_IMAGE_ICON_NAME:
      g_object_notify (G_OBJECT (cellpixbuf), "icon-name");
      break;
    case CTK_IMAGE_GICON:
      g_object_notify (G_OBJECT (cellpixbuf), "gicon");
      break;
    default:
    case CTK_IMAGE_ANIMATION:
      g_assert_not_reached ();
    case CTK_IMAGE_EMPTY:
      break;
    }
}

static void
take_image_definition (CtkCellRendererPixbuf *cellpixbuf,
                       CtkImageDefinition    *def)
{
  CtkCellRendererPixbufPrivate *priv;
  CtkImageType old_storage_type, new_storage_type;
  
  if (def == NULL)
    def = ctk_image_definition_new_empty ();

  priv = cellpixbuf->priv;
  old_storage_type = ctk_image_definition_get_storage_type (priv->image_def);
  new_storage_type = ctk_image_definition_get_storage_type (def);
 
  if (new_storage_type != old_storage_type)
    notify_storage_type (cellpixbuf, old_storage_type);

  ctk_image_definition_unref (priv->image_def);
  priv->image_def = def;
}

static void
ctk_cell_renderer_pixbuf_set_property (GObject      *object,
				       guint         param_id,
				       const GValue *value,
				       GParamSpec   *pspec)
{
  CtkCellRendererPixbuf *cellpixbuf = CTK_CELL_RENDERER_PIXBUF (object);
  CtkCellRendererPixbufPrivate *priv = cellpixbuf->priv;

  switch (param_id)
    {
    case PROP_PIXBUF:
      take_image_definition (cellpixbuf, ctk_image_definition_new_pixbuf (g_value_get_object (value), 1));
      break;
    case PROP_PIXBUF_EXPANDER_OPEN:
      if (priv->pixbuf_expander_open)
        g_object_unref (priv->pixbuf_expander_open);
      priv->pixbuf_expander_open = (GdkPixbuf*) g_value_dup_object (value);
      break;
    case PROP_PIXBUF_EXPANDER_CLOSED:
      if (priv->pixbuf_expander_closed)
        g_object_unref (priv->pixbuf_expander_closed);
      priv->pixbuf_expander_closed = (GdkPixbuf*) g_value_dup_object (value);
      break;
    case PROP_SURFACE:
      take_image_definition (cellpixbuf, ctk_image_definition_new_surface (g_value_get_boxed (value)));
      break;
    case PROP_STOCK_ID:
      take_image_definition (cellpixbuf, ctk_image_definition_new_stock (g_value_get_string (value)));
      break;
    case PROP_STOCK_SIZE:
      priv->icon_size = g_value_get_uint (value);
      break;
    case PROP_STOCK_DETAIL:
      g_free (priv->stock_detail);
      priv->stock_detail = g_value_dup_string (value);
      break;
    case PROP_ICON_NAME:
      take_image_definition (cellpixbuf, ctk_image_definition_new_icon_name (g_value_get_string (value)));
      break;
    case PROP_FOLLOW_STATE:
      priv->follow_state = g_value_get_boolean (value);
      break;
    case PROP_GICON:
      take_image_definition (cellpixbuf, ctk_image_definition_new_gicon (g_value_get_object (value)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

/**
 * ctk_cell_renderer_pixbuf_new:
 * 
 * Creates a new #CtkCellRendererPixbuf. Adjust rendering
 * parameters using object properties. Object properties can be set
 * globally (with g_object_set()). Also, with #CtkTreeViewColumn, you
 * can bind a property to a value in a #CtkTreeModel. For example, you
 * can bind the “pixbuf” property on the cell renderer to a pixbuf value
 * in the model, thus rendering a different image in each row of the
 * #CtkTreeView.
 * 
 * Returns: the new cell renderer
 **/
CtkCellRenderer *
ctk_cell_renderer_pixbuf_new (void)
{
  return g_object_new (CTK_TYPE_CELL_RENDERER_PIXBUF, NULL);
}

static CtkIconHelper *
create_icon_helper (CtkCellRendererPixbuf *cellpixbuf,
                    CtkWidget             *widget)
{
  CtkCellRendererPixbufPrivate *priv = cellpixbuf->priv;
  CtkIconHelper *helper;

  helper = ctk_icon_helper_new (ctk_style_context_get_node (ctk_widget_get_style_context (widget)), widget);
  _ctk_icon_helper_set_use_fallback (helper, TRUE);
  _ctk_icon_helper_set_force_scale_pixbuf (helper, TRUE);
  _ctk_icon_helper_set_definition (helper, priv->image_def);
  if (ctk_image_definition_get_storage_type (priv->image_def) != CTK_IMAGE_PIXBUF)
    _ctk_icon_helper_set_icon_size (helper, priv->icon_size);

  return helper;
}

static void
ctk_cell_renderer_pixbuf_get_size (CtkCellRenderer    *cell,
				   CtkWidget          *widget,
				   const CdkRectangle *cell_area,
				   gint               *x_offset,
				   gint               *y_offset,
				   gint               *width,
				   gint               *height)
{
  CtkCellRendererPixbuf *cellpixbuf = (CtkCellRendererPixbuf *) cell;
  CtkCellRendererPixbufPrivate *priv = cellpixbuf->priv;
  gint pixbuf_width  = 0;
  gint pixbuf_height = 0;
  gint calc_width;
  gint calc_height;
  gint xpad, ypad;
  CtkStyleContext *context;
  CtkIconHelper *icon_helper;

  context = ctk_widget_get_style_context (widget);
  ctk_style_context_save (context);
  ctk_style_context_add_class (context, CTK_STYLE_CLASS_IMAGE);
  icon_helper = create_icon_helper (cellpixbuf, widget);

  if (!_ctk_icon_helper_get_is_empty (icon_helper))
    _ctk_icon_helper_get_size (icon_helper, 
                               &pixbuf_width, &pixbuf_height);

  g_object_unref (icon_helper);
  ctk_style_context_restore (context);

  if (priv->pixbuf_expander_open)
    {
      pixbuf_width  = MAX (pixbuf_width, gdk_pixbuf_get_width (priv->pixbuf_expander_open));
      pixbuf_height = MAX (pixbuf_height, gdk_pixbuf_get_height (priv->pixbuf_expander_open));
    }
  if (priv->pixbuf_expander_closed)
    {
      pixbuf_width  = MAX (pixbuf_width, gdk_pixbuf_get_width (priv->pixbuf_expander_closed));
      pixbuf_height = MAX (pixbuf_height, gdk_pixbuf_get_height (priv->pixbuf_expander_closed));
    }

  ctk_cell_renderer_get_padding (cell, &xpad, &ypad);
  calc_width  = (gint) xpad * 2 + pixbuf_width;
  calc_height = (gint) ypad * 2 + pixbuf_height;
  
  if (cell_area && pixbuf_width > 0 && pixbuf_height > 0)
    {
      gfloat xalign, yalign;

      ctk_cell_renderer_get_alignment (cell, &xalign, &yalign);
      if (x_offset)
	{
	  *x_offset = (((ctk_widget_get_direction (widget) == CTK_TEXT_DIR_RTL) ?
                        (1.0 - xalign) : xalign) *
                       (cell_area->width - calc_width));
	  *x_offset = MAX (*x_offset, 0);
	}
      if (y_offset)
	{
	  *y_offset = (yalign *
                       (cell_area->height - calc_height));
          *y_offset = MAX (*y_offset, 0);
	}
    }
  else
    {
      if (x_offset) *x_offset = 0;
      if (y_offset) *y_offset = 0;
    }

  if (width)
    *width = calc_width;
  
  if (height)
    *height = calc_height;
}

static void
ctk_cell_renderer_pixbuf_render (CtkCellRenderer      *cell,
                                 cairo_t              *cr,
				 CtkWidget            *widget,
				 const CdkRectangle   *background_area G_GNUC_UNUSED,
				 const CdkRectangle   *cell_area,
				 CtkCellRendererState  flags G_GNUC_UNUSED)

{
  CtkCellRendererPixbuf *cellpixbuf = (CtkCellRendererPixbuf *) cell;
  CtkCellRendererPixbufPrivate *priv = cellpixbuf->priv;
  CtkStyleContext *context;
  CdkRectangle pix_rect;
  CdkRectangle draw_rect;
  gboolean is_expander;
  gint xpad, ypad;
  CtkIconHelper *icon_helper = NULL;

  ctk_cell_renderer_pixbuf_get_size (cell, widget, (CdkRectangle *) cell_area,
				     &pix_rect.x, 
                                     &pix_rect.y,
                                     &pix_rect.width,
                                     &pix_rect.height);

  ctk_cell_renderer_get_padding (cell, &xpad, &ypad);
  pix_rect.x += cell_area->x + xpad;
  pix_rect.y += cell_area->y + ypad;
  pix_rect.width -= xpad * 2;
  pix_rect.height -= ypad * 2;

  if (!cdk_rectangle_intersect (cell_area, &pix_rect, &draw_rect))
    return;

  context = ctk_widget_get_style_context (widget);
  ctk_style_context_save (context);

  ctk_style_context_add_class (context, CTK_STYLE_CLASS_IMAGE);

  g_object_get (cell, "is-expander", &is_expander, NULL);
  if (is_expander)
    {
      gboolean is_expanded;

      g_object_get (cell, "is-expanded", &is_expanded, NULL);

      if (is_expanded && priv->pixbuf_expander_open != NULL)
        {
          icon_helper = ctk_icon_helper_new (ctk_style_context_get_node (context), widget);
          _ctk_icon_helper_set_pixbuf (icon_helper, priv->pixbuf_expander_open);
        }
      else if (!is_expanded && priv->pixbuf_expander_closed != NULL)
        {
          icon_helper = ctk_icon_helper_new (ctk_style_context_get_node (context), widget);
          _ctk_icon_helper_set_pixbuf (icon_helper, priv->pixbuf_expander_closed);
        }
    }

  if (icon_helper == NULL)
    icon_helper = create_icon_helper (cellpixbuf, widget);

  _ctk_icon_helper_draw (icon_helper,
                         cr,
                         pix_rect.x, pix_rect.y);
  g_object_unref (icon_helper);

  ctk_style_context_restore (context);
}
