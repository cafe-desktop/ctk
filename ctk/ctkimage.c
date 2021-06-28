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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
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

#include <math.h>
#include <string.h>
#include <cairo-gobject.h>

#include "ctkcontainer.h"
#include "ctkcssstylepropertyprivate.h"
#include "ctkiconhelperprivate.h"
#include "ctkimageprivate.h"
#include "deprecated/ctkiconfactory.h"
#include "deprecated/ctkstock.h"
#include "ctkicontheme.h"
#include "ctksizerequest.h"
#include "ctkintl.h"
#include "ctkprivate.h"
#include "ctktypebuiltins.h"
#include "ctkcssshadowsvalueprivate.h"
#include "ctkstylecontextprivate.h"
#include "ctkwidgetprivate.h"
#include "ctkcsscustomgadgetprivate.h"

#include "a11y/ctkimageaccessible.h"

/**
 * SECTION:ctkimage
 * @Short_description: A widget displaying an image
 * @Title: CtkImage
 * @See_also:#GdkPixbuf
 *
 * The #CtkImage widget displays an image. Various kinds of object
 * can be displayed as an image; most typically, you would load a
 * #GdkPixbuf ("pixel buffer") from a file, and then display that.
 * There’s a convenience function to do this, ctk_image_new_from_file(),
 * used as follows:
 * |[<!-- language="C" -->
 *   CtkWidget *image;
 *   image = ctk_image_new_from_file ("myfile.png");
 * ]|
 * If the file isn’t loaded successfully, the image will contain a
 * “broken image” icon similar to that used in many web browsers.
 * If you want to handle errors in loading the file yourself,
 * for example by displaying an error message, then load the image with
 * gdk_pixbuf_new_from_file(), then create the #CtkImage with
 * ctk_image_new_from_pixbuf().
 *
 * The image file may contain an animation, if so the #CtkImage will
 * display an animation (#GdkPixbufAnimation) instead of a static image.
 *
 * #CtkImage is a subclass of #CtkMisc, which implies that you can
 * align it (center, left, right) and add padding to it, using
 * #CtkMisc methods.
 *
 * #CtkImage is a “no window” widget (has no #CdkWindow of its own),
 * so by default does not receive events. If you want to receive events
 * on the image, such as button clicks, place the image inside a
 * #CtkEventBox, then connect to the event signals on the event box.
 *
 * ## Handling button press events on a #CtkImage.
 *
 * |[<!-- language="C" -->
 *   static gboolean
 *   button_press_callback (CtkWidget      *event_box,
 *                          CdkEventButton *event,
 *                          gpointer        data)
 *   {
 *     g_print ("Event box clicked at coordinates %f,%f\n",
 *              event->x, event->y);
 *
 *     // Returning TRUE means we handled the event, so the signal
 *     // emission should be stopped (don’t call any further callbacks
 *     // that may be connected). Return FALSE to continue invoking callbacks.
 *     return TRUE;
 *   }
 *
 *   static CtkWidget*
 *   create_image (void)
 *   {
 *     CtkWidget *image;
 *     CtkWidget *event_box;
 *
 *     image = ctk_image_new_from_file ("myfile.png");
 *
 *     event_box = ctk_event_box_new ();
 *
 *     ctk_container_add (CTK_CONTAINER (event_box), image);
 *
 *     g_signal_connect (G_OBJECT (event_box),
 *                       "button_press_event",
 *                       G_CALLBACK (button_press_callback),
 *                       image);
 *
 *     return image;
 *   }
 * ]|
 *
 * When handling events on the event box, keep in mind that coordinates
 * in the image may be different from event box coordinates due to
 * the alignment and padding settings on the image (see #CtkMisc).
 * The simplest way to solve this is to set the alignment to 0.0
 * (left/top), and set the padding to zero. Then the origin of
 * the image will be the same as the origin of the event box.
 *
 * Sometimes an application will want to avoid depending on external data
 * files, such as image files. CTK+ comes with a program to avoid this,
 * called “gdk-pixbuf-csource”. This library
 * allows you to convert an image into a C variable declaration, which
 * can then be loaded into a #GdkPixbuf using
 * gdk_pixbuf_new_from_inline().
 *
 * # CSS nodes
 *
 * CtkImage has a single CSS node with the name image. The style classes
 * may appear on image CSS nodes: .icon-dropshadow, .lowres-icon.
 */


struct _CtkImagePrivate
{
  CtkIconHelper *icon_helper;

  GdkPixbufAnimationIter *animation_iter;
  gint animation_timeout;

  CtkCssGadget *gadget;

  float baseline_align;

  gchar                *filename;       /* Only used with CTK_IMAGE_ANIMATION, CTK_IMAGE_PIXBUF */
  gchar                *resource_path;  /* Only used with CTK_IMAGE_PIXBUF */
};


#define DEFAULT_ICON_SIZE CTK_ICON_SIZE_BUTTON
static gint ctk_image_draw                 (CtkWidget    *widget,
                                            cairo_t      *cr);
static void ctk_image_size_allocate        (CtkWidget    *widget,
                                            CtkAllocation*allocation);
static void ctk_image_unmap                (CtkWidget    *widget);
static void ctk_image_unrealize            (CtkWidget    *widget);
static void ctk_image_get_preferred_width  (CtkWidget    *widget,
                                            gint         *minimum,
                                            gint         *natural);
static void ctk_image_get_preferred_height (CtkWidget    *widget,
                                            gint         *minimum,
                                            gint         *natural);
static void ctk_image_get_preferred_height_and_baseline_for_width (CtkWidget *widget,
								   gint       width,
								   gint      *minimum,
								   gint      *natural,
								   gint      *minimum_baseline,
								   gint      *natural_baseline);

static void ctk_image_get_content_size (CtkCssGadget   *gadget,
                                        CtkOrientation  orientation,
                                        gint            for_size,
                                        gint           *minimum,
                                        gint           *natural,
                                        gint           *minimum_baseline,
                                        gint           *natural_baseline,
                                        gpointer        unused);
static gboolean ctk_image_render_contents (CtkCssGadget *gadget,
                                           cairo_t      *cr,
                                           int           x,
                                           int           y,
                                           int           width,
                                           int           height,
                                           gpointer      data);

static void ctk_image_style_updated        (CtkWidget    *widget);
static void ctk_image_finalize             (GObject      *object);
static void ctk_image_reset                (CtkImage     *image);

static void ctk_image_set_property         (GObject      *object,
                                            guint         prop_id,
                                            const GValue *value,
                                            GParamSpec   *pspec);
static void ctk_image_get_property         (GObject      *object,
                                            guint         prop_id,
                                            GValue       *value,
                                            GParamSpec   *pspec);

enum
{
  PROP_0,
  PROP_PIXBUF,
  PROP_SURFACE,
  PROP_FILE,
  PROP_STOCK,
  PROP_ICON_SET,
  PROP_ICON_SIZE,
  PROP_PIXEL_SIZE,
  PROP_PIXBUF_ANIMATION,
  PROP_ICON_NAME,
  PROP_STORAGE_TYPE,
  PROP_GICON,
  PROP_RESOURCE,
  PROP_USE_FALLBACK,
  NUM_PROPERTIES
};

static GParamSpec *image_props[NUM_PROPERTIES] = { NULL, };

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
G_DEFINE_TYPE_WITH_PRIVATE (CtkImage, ctk_image, CTK_TYPE_MISC)
G_GNUC_END_IGNORE_DEPRECATIONS

static void
ctk_image_class_init (CtkImageClass *class)
{
  GObjectClass *gobject_class;
  CtkWidgetClass *widget_class;

  gobject_class = G_OBJECT_CLASS (class);
  
  gobject_class->set_property = ctk_image_set_property;
  gobject_class->get_property = ctk_image_get_property;
  gobject_class->finalize = ctk_image_finalize;

  widget_class = CTK_WIDGET_CLASS (class);
  widget_class->draw = ctk_image_draw;
  widget_class->get_preferred_width = ctk_image_get_preferred_width;
  widget_class->get_preferred_height = ctk_image_get_preferred_height;
  widget_class->get_preferred_height_and_baseline_for_width = ctk_image_get_preferred_height_and_baseline_for_width;
  widget_class->size_allocate = ctk_image_size_allocate;
  widget_class->unmap = ctk_image_unmap;
  widget_class->unrealize = ctk_image_unrealize;
  widget_class->style_updated = ctk_image_style_updated;

  image_props[PROP_PIXBUF] =
      g_param_spec_object ("pixbuf",
                           P_("Pixbuf"),
                           P_("A GdkPixbuf to display"),
                           CDK_TYPE_PIXBUF,
                           CTK_PARAM_READWRITE);

  image_props[PROP_SURFACE] =
      g_param_spec_boxed ("surface",
                          P_("Surface"),
                          P_("A cairo_surface_t to display"),
                          CAIRO_GOBJECT_TYPE_SURFACE,
                          CTK_PARAM_READWRITE);

  image_props[PROP_FILE] =
      g_param_spec_string ("file",
                           P_("Filename"),
                           P_("Filename to load and display"),
                           NULL,
                           CTK_PARAM_READWRITE);

  /**
   * CtkImage:stock:
   *
   * Deprecated: 3.10: Use #CtkImage:icon-name instead.
   */
  image_props[PROP_STOCK] =
      g_param_spec_string ("stock",
                           P_("Stock ID"),
                           P_("Stock ID for a stock image to display"),
                           NULL,
                           CTK_PARAM_READWRITE | G_PARAM_DEPRECATED);

  /**
   * CtkImage:icon-set:
   *
   * Deprecated: 3.10: Use #CtkImage:icon-name instead.
   */
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  image_props[PROP_ICON_SET] =
      g_param_spec_boxed ("icon-set",
                          P_("Icon set"),
                          P_("Icon set to display"),
                          CTK_TYPE_ICON_SET,
                          CTK_PARAM_READWRITE | G_PARAM_DEPRECATED);
  G_GNUC_END_IGNORE_DEPRECATIONS;

  image_props[PROP_ICON_SIZE] =
      g_param_spec_int ("icon-size",
                        P_("Icon size"),
                        P_("Symbolic size to use for stock icon, icon set or named icon"),
                        0, G_MAXINT,
                        DEFAULT_ICON_SIZE,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * CtkImage:pixel-size:
   *
   * The "pixel-size" property can be used to specify a fixed size
   * overriding the #CtkImage:icon-size property for images of type
   * %CTK_IMAGE_ICON_NAME.
   *
   * Since: 2.6
   */
  image_props[PROP_PIXEL_SIZE] =
      g_param_spec_int ("pixel-size",
                        P_("Pixel size"),
                        P_("Pixel size to use for named icon"),
                        -1, G_MAXINT,
                        -1,
                        CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  image_props[PROP_PIXBUF_ANIMATION] =
      g_param_spec_object ("pixbuf-animation",
                           P_("Animation"),
                           P_("GdkPixbufAnimation to display"),
                           CDK_TYPE_PIXBUF_ANIMATION,
                           CTK_PARAM_READWRITE);

  /**
   * CtkImage:icon-name:
   *
   * The name of the icon in the icon theme. If the icon theme is
   * changed, the image will be updated automatically.
   *
   * Since: 2.6
   */
  image_props[PROP_ICON_NAME] =
      g_param_spec_string ("icon-name",
                           P_("Icon Name"),
                           P_("The name of the icon from the icon theme"),
                           NULL,
                           CTK_PARAM_READWRITE);

  /**
   * CtkImage:gicon:
   *
   * The GIcon displayed in the CtkImage. For themed icons,
   * If the icon theme is changed, the image will be updated
   * automatically.
   *
   * Since: 2.14
   */
  image_props[PROP_GICON] =
      g_param_spec_object ("gicon",
                           P_("Icon"),
                           P_("The GIcon being displayed"),
                           G_TYPE_ICON,
                           CTK_PARAM_READWRITE);

  /**
   * CtkImage:resource:
   *
   * A path to a resource file to display.
   *
   * Since: 3.8
   */
  image_props[PROP_RESOURCE] =
      g_param_spec_string ("resource",
                           P_("Resource"),
                           P_("The resource path being displayed"),
                           NULL,
                           CTK_PARAM_READWRITE);

  image_props[PROP_STORAGE_TYPE] =
      g_param_spec_enum ("storage-type",
                         P_("Storage type"),
                         P_("The representation being used for image data"),
                         CTK_TYPE_IMAGE_TYPE,
                         CTK_IMAGE_EMPTY,
                         CTK_PARAM_READABLE);

  /**
   * CtkImage:use-fallback:
   *
   * Whether the icon displayed in the CtkImage will use
   * standard icon names fallback. The value of this property
   * is only relevant for images of type %CTK_IMAGE_ICON_NAME
   * and %CTK_IMAGE_GICON.
   *
   * Since: 3.0
   */
  image_props[PROP_USE_FALLBACK] =
      g_param_spec_boolean ("use-fallback",
                            P_("Use Fallback"),
                            P_("Whether to use icon names fallback"),
                            FALSE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class, NUM_PROPERTIES, image_props);

  ctk_widget_class_set_accessible_type (widget_class, CTK_TYPE_IMAGE_ACCESSIBLE);
  ctk_widget_class_set_css_name (widget_class, "image");
}

static void
ctk_image_init (CtkImage *image)
{
  CtkImagePrivate *priv;
  CtkCssNode *widget_node;

  image->priv = ctk_image_get_instance_private (image);
  priv = image->priv;

  widget_node = ctk_widget_get_css_node (CTK_WIDGET (image));
  ctk_widget_set_has_window (CTK_WIDGET (image), FALSE);

  priv->icon_helper = ctk_icon_helper_new (widget_node, CTK_WIDGET (image));
  _ctk_icon_helper_set_icon_size (priv->icon_helper, DEFAULT_ICON_SIZE);

  priv->gadget = ctk_css_custom_gadget_new_for_node (widget_node,
                                                     CTK_WIDGET (image),
                                                     ctk_image_get_content_size,
                                                     NULL,
                                                     ctk_image_render_contents,
                                                     NULL, NULL);

}

static void
ctk_image_finalize (GObject *object)
{
  CtkImage *image = CTK_IMAGE (object);

  g_clear_object (&image->priv->icon_helper);
  g_clear_object (&image->priv->gadget);

  g_free (image->priv->filename);
  g_free (image->priv->resource_path);

  G_OBJECT_CLASS (ctk_image_parent_class)->finalize (object);
};

static void
ctk_image_set_property (GObject      *object,
			guint         prop_id,
			const GValue *value,
			GParamSpec   *pspec)
{
  CtkImage *image = CTK_IMAGE (object);
  CtkImagePrivate *priv = image->priv;
  CtkIconSize icon_size = _ctk_icon_helper_get_icon_size (priv->icon_helper);

  if (icon_size == CTK_ICON_SIZE_INVALID)
    icon_size = DEFAULT_ICON_SIZE;

  switch (prop_id)
    {
    case PROP_PIXBUF:
      ctk_image_set_from_pixbuf (image, g_value_get_object (value));
      break;
    case PROP_SURFACE:
      ctk_image_set_from_surface (image, g_value_get_boxed (value));
      break;
    case PROP_FILE:
      ctk_image_set_from_file (image, g_value_get_string (value));
      break;
    case PROP_STOCK:
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      ctk_image_set_from_stock (image, g_value_get_string (value), icon_size);
      G_GNUC_END_IGNORE_DEPRECATIONS;
      break;
    case PROP_ICON_SET:
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      ctk_image_set_from_icon_set (image, g_value_get_boxed (value), icon_size);
      G_GNUC_END_IGNORE_DEPRECATIONS;
      break;
    case PROP_ICON_SIZE:
      if (_ctk_icon_helper_set_icon_size (priv->icon_helper, g_value_get_int (value)))
        {
          g_object_notify_by_pspec (object, pspec);
          ctk_widget_queue_resize (CTK_WIDGET (image));
        }
      break;
    case PROP_PIXEL_SIZE:
      ctk_image_set_pixel_size (image, g_value_get_int (value));
      break;
    case PROP_PIXBUF_ANIMATION:
      ctk_image_set_from_animation (image, g_value_get_object (value));
      break;
    case PROP_ICON_NAME:
      ctk_image_set_from_icon_name (image, g_value_get_string (value), icon_size);
      break;
    case PROP_GICON:
      ctk_image_set_from_gicon (image, g_value_get_object (value), icon_size);
      break;
    case PROP_RESOURCE:
      ctk_image_set_from_resource (image, g_value_get_string (value));
      break;

    case PROP_USE_FALLBACK:
      if (_ctk_icon_helper_set_use_fallback (priv->icon_helper, g_value_get_boolean (value)))
        g_object_notify_by_pspec (object, pspec);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void 
ctk_image_get_property (GObject     *object,
			guint        prop_id,
			GValue      *value,
			GParamSpec  *pspec)
{
  CtkImage *image = CTK_IMAGE (object);
  CtkImagePrivate *priv = image->priv;

  switch (prop_id)
    {
    case PROP_PIXBUF:
      g_value_set_object (value, _ctk_icon_helper_peek_pixbuf (priv->icon_helper));
      break;
    case PROP_SURFACE:
      g_value_set_boxed (value, _ctk_icon_helper_peek_surface (priv->icon_helper));
      break;
    case PROP_FILE:
      g_value_set_string (value, priv->filename);
      break;
    case PROP_STOCK:
      g_value_set_string (value, _ctk_icon_helper_get_stock_id (priv->icon_helper));
      break;
    case PROP_ICON_SET:
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      g_value_set_boxed (value, _ctk_icon_helper_peek_icon_set (priv->icon_helper));
      G_GNUC_END_IGNORE_DEPRECATIONS;
      break;
    case PROP_ICON_SIZE:
      g_value_set_int (value, _ctk_icon_helper_get_icon_size (priv->icon_helper));
      break;
    case PROP_PIXEL_SIZE:
      g_value_set_int (value, _ctk_icon_helper_get_pixel_size (priv->icon_helper));
      break;
    case PROP_PIXBUF_ANIMATION:
      g_value_set_object (value, _ctk_icon_helper_peek_animation (priv->icon_helper));
      break;
    case PROP_ICON_NAME:
      g_value_set_string (value, _ctk_icon_helper_get_icon_name (priv->icon_helper));
      break;
    case PROP_GICON:
      g_value_set_object (value, _ctk_icon_helper_peek_gicon (priv->icon_helper));
      break;
    case PROP_RESOURCE:
      g_value_set_string (value, priv->resource_path);
      break;
    case PROP_USE_FALLBACK:
      g_value_set_boolean (value, _ctk_icon_helper_get_use_fallback (priv->icon_helper));
      break;
    case PROP_STORAGE_TYPE:
      g_value_set_enum (value, _ctk_icon_helper_get_storage_type (priv->icon_helper));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


/**
 * ctk_image_new_from_file:
 * @filename: (type filename): a filename
 * 
 * Creates a new #CtkImage displaying the file @filename. If the file
 * isn’t found or can’t be loaded, the resulting #CtkImage will
 * display a “broken image” icon. This function never returns %NULL,
 * it always returns a valid #CtkImage widget.
 *
 * If the file contains an animation, the image will contain an
 * animation.
 *
 * If you need to detect failures to load the file, use
 * gdk_pixbuf_new_from_file() to load the file yourself, then create
 * the #CtkImage from the pixbuf. (Or for animations, use
 * gdk_pixbuf_animation_new_from_file()).
 *
 * The storage type (ctk_image_get_storage_type()) of the returned
 * image is not defined, it will be whatever is appropriate for
 * displaying the file.
 * 
 * Returns: a new #CtkImage
 **/
CtkWidget*
ctk_image_new_from_file   (const gchar *filename)
{
  CtkImage *image;

  image = g_object_new (CTK_TYPE_IMAGE, NULL);

  ctk_image_set_from_file (image, filename);

  return CTK_WIDGET (image);
}

/**
 * ctk_image_new_from_resource:
 * @resource_path: a resource path
 *
 * Creates a new #CtkImage displaying the resource file @resource_path. If the file
 * isn’t found or can’t be loaded, the resulting #CtkImage will
 * display a “broken image” icon. This function never returns %NULL,
 * it always returns a valid #CtkImage widget.
 *
 * If the file contains an animation, the image will contain an
 * animation.
 *
 * If you need to detect failures to load the file, use
 * gdk_pixbuf_new_from_file() to load the file yourself, then create
 * the #CtkImage from the pixbuf. (Or for animations, use
 * gdk_pixbuf_animation_new_from_file()).
 *
 * The storage type (ctk_image_get_storage_type()) of the returned
 * image is not defined, it will be whatever is appropriate for
 * displaying the file.
 *
 * Returns: a new #CtkImage
 *
 * Since: 3.4
 **/
CtkWidget*
ctk_image_new_from_resource (const gchar *resource_path)
{
  CtkImage *image;

  image = g_object_new (CTK_TYPE_IMAGE, NULL);

  ctk_image_set_from_resource (image, resource_path);

  return CTK_WIDGET (image);
}

/**
 * ctk_image_new_from_pixbuf:
 * @pixbuf: (allow-none): a #GdkPixbuf, or %NULL
 *
 * Creates a new #CtkImage displaying @pixbuf.
 * The #CtkImage does not assume a reference to the
 * pixbuf; you still need to unref it if you own references.
 * #CtkImage will add its own reference rather than adopting yours.
 * 
 * Note that this function just creates an #CtkImage from the pixbuf. The
 * #CtkImage created will not react to state changes. Should you want that, 
 * you should use ctk_image_new_from_icon_name().
 * 
 * Returns: a new #CtkImage
 **/
CtkWidget*
ctk_image_new_from_pixbuf (GdkPixbuf *pixbuf)
{
  CtkImage *image;

  image = g_object_new (CTK_TYPE_IMAGE, NULL);

  ctk_image_set_from_pixbuf (image, pixbuf);

  return CTK_WIDGET (image);  
}

/**
 * ctk_image_new_from_surface:
 * @surface: (allow-none): a #cairo_surface_t, or %NULL
 *
 * Creates a new #CtkImage displaying @surface.
 * The #CtkImage does not assume a reference to the
 * surface; you still need to unref it if you own references.
 * #CtkImage will add its own reference rather than adopting yours.
 * 
 * Returns: a new #CtkImage
 *
 * Since: 3.10
 **/
CtkWidget*
ctk_image_new_from_surface (cairo_surface_t *surface)
{
  CtkImage *image;

  image = g_object_new (CTK_TYPE_IMAGE, NULL);

  ctk_image_set_from_surface (image, surface);

  return CTK_WIDGET (image);  
}

/**
 * ctk_image_new_from_stock:
 * @stock_id: a stock icon name
 * @size: (type int): a stock icon size (#CtkIconSize)
 * 
 * Creates a #CtkImage displaying a stock icon. Sample stock icon
 * names are #CTK_STOCK_OPEN, #CTK_STOCK_QUIT. Sample stock sizes
 * are #CTK_ICON_SIZE_MENU, #CTK_ICON_SIZE_SMALL_TOOLBAR. If the stock
 * icon name isn’t known, the image will be empty.
 * You can register your own stock icon names, see
 * ctk_icon_factory_add_default() and ctk_icon_factory_add().
 * 
 * Returns: a new #CtkImage displaying the stock icon
 *
 * Deprecated: 3.10: Use ctk_image_new_from_icon_name() instead.
 **/
CtkWidget*
ctk_image_new_from_stock (const gchar    *stock_id,
                          CtkIconSize     size)
{
  CtkImage *image;

  image = g_object_new (CTK_TYPE_IMAGE, NULL);

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  ctk_image_set_from_stock (image, stock_id, size);
  G_GNUC_END_IGNORE_DEPRECATIONS;

  return CTK_WIDGET (image);
}

/**
 * ctk_image_new_from_icon_set:
 * @icon_set: a #CtkIconSet
 * @size: (type int): a stock icon size (#CtkIconSize)
 *
 * Creates a #CtkImage displaying an icon set. Sample stock sizes are
 * #CTK_ICON_SIZE_MENU, #CTK_ICON_SIZE_SMALL_TOOLBAR. Instead of using
 * this function, usually it’s better to create a #CtkIconFactory, put
 * your icon sets in the icon factory, add the icon factory to the
 * list of default factories with ctk_icon_factory_add_default(), and
 * then use ctk_image_new_from_stock(). This will allow themes to
 * override the icon you ship with your application.
 *
 * The #CtkImage does not assume a reference to the
 * icon set; you still need to unref it if you own references.
 * #CtkImage will add its own reference rather than adopting yours.
 * 
 * Returns: a new #CtkImage
 *
 * Deprecated: 3.10: Use ctk_image_new_from_icon_name() instead.
 **/
CtkWidget*
ctk_image_new_from_icon_set (CtkIconSet     *icon_set,
                             CtkIconSize     size)
{
  CtkImage *image;

  image = g_object_new (CTK_TYPE_IMAGE, NULL);

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;

  ctk_image_set_from_icon_set (image, icon_set, size);

  G_GNUC_END_IGNORE_DEPRECATIONS;

  return CTK_WIDGET (image);
}

/**
 * ctk_image_new_from_animation:
 * @animation: an animation
 * 
 * Creates a #CtkImage displaying the given animation.
 * The #CtkImage does not assume a reference to the
 * animation; you still need to unref it if you own references.
 * #CtkImage will add its own reference rather than adopting yours.
 *
 * Note that the animation frames are shown using a timeout with
 * #G_PRIORITY_DEFAULT. When using animations to indicate busyness,
 * keep in mind that the animation will only be shown if the main loop
 * is not busy with something that has a higher priority.
 *
 * Returns: a new #CtkImage widget
 **/
CtkWidget*
ctk_image_new_from_animation (GdkPixbufAnimation *animation)
{
  CtkImage *image;

  g_return_val_if_fail (CDK_IS_PIXBUF_ANIMATION (animation), NULL);
  
  image = g_object_new (CTK_TYPE_IMAGE, NULL);

  ctk_image_set_from_animation (image, animation);

  return CTK_WIDGET (image);
}

/**
 * ctk_image_new_from_icon_name:
 * @icon_name: (nullable): an icon name or %NULL
 * @size: (type int): a stock icon size (#CtkIconSize)
 * 
 * Creates a #CtkImage displaying an icon from the current icon theme.
 * If the icon name isn’t known, a “broken image” icon will be
 * displayed instead.  If the current icon theme is changed, the icon
 * will be updated appropriately.
 * 
 * Returns: a new #CtkImage displaying the themed icon
 *
 * Since: 2.6
 **/
CtkWidget*
ctk_image_new_from_icon_name (const gchar    *icon_name,
			      CtkIconSize     size)
{
  CtkImage *image;

  image = g_object_new (CTK_TYPE_IMAGE, NULL);

  ctk_image_set_from_icon_name (image, icon_name, size);

  return CTK_WIDGET (image);
}

/**
 * ctk_image_new_from_gicon:
 * @icon: an icon
 * @size: (type int): a stock icon size (#CtkIconSize)
 * 
 * Creates a #CtkImage displaying an icon from the current icon theme.
 * If the icon name isn’t known, a “broken image” icon will be
 * displayed instead.  If the current icon theme is changed, the icon
 * will be updated appropriately.
 * 
 * Returns: a new #CtkImage displaying the themed icon
 *
 * Since: 2.14
 **/
CtkWidget*
ctk_image_new_from_gicon (GIcon *icon,
			  CtkIconSize     size)
{
  CtkImage *image;

  image = g_object_new (CTK_TYPE_IMAGE, NULL);

  ctk_image_set_from_gicon (image, icon, size);

  return CTK_WIDGET (image);
}

typedef struct {
  CtkImage *image;
  gint scale_factor;
} LoaderData;

static void
on_loader_size_prepared (GdkPixbufLoader *loader,
			 gint             width,
			 gint             height,
			 gpointer         user_data)
{
  LoaderData *loader_data = user_data;
  gint scale_factor;
  GdkPixbufFormat *format;

  /* Let the regular icon helper code path handle non-scalable images */
  format = gdk_pixbuf_loader_get_format (loader);
  if (!gdk_pixbuf_format_is_scalable (format))
    {
      loader_data->scale_factor = 1;
      return;
    }

  scale_factor = ctk_widget_get_scale_factor (CTK_WIDGET (loader_data->image));
  gdk_pixbuf_loader_set_size (loader, width * scale_factor, height * scale_factor);
  loader_data->scale_factor = scale_factor;
}

static GdkPixbufAnimation *
load_scalable_with_loader (CtkImage    *image,
			   const gchar *file_path,
			   const gchar *resource_path,
			   gint        *scale_factor_out)
{
  GdkPixbufLoader *loader;
  GBytes *bytes;
  char *contents;
  gsize length;
  gboolean res;
  GdkPixbufAnimation *animation;
  LoaderData loader_data;

  animation = NULL;
  bytes = NULL;

  loader = gdk_pixbuf_loader_new ();
  loader_data.image = image;

  g_signal_connect (loader, "size-prepared", G_CALLBACK (on_loader_size_prepared), &loader_data);

  if (resource_path != NULL)
    {
      bytes = g_resources_lookup_data (resource_path, G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
    }
  else if (file_path != NULL)
    {
      res = g_file_get_contents (file_path, &contents, &length, NULL);
      if (res)
	bytes = g_bytes_new_take (contents, length);
    }
  else
    {
      g_assert_not_reached ();
    }

  if (!bytes)
    goto out;

  if (!gdk_pixbuf_loader_write_bytes (loader, bytes, NULL))
    goto out;

  if (!gdk_pixbuf_loader_close (loader, NULL))
    goto out;

  animation = gdk_pixbuf_loader_get_animation (loader);
  if (animation != NULL)
    {
      g_object_ref (animation);
      if (scale_factor_out != NULL)
	*scale_factor_out = loader_data.scale_factor;
    }

 out:
  gdk_pixbuf_loader_close (loader, NULL);
  g_object_unref (loader);
  g_bytes_unref (bytes);

  return animation;
}

/**
 * ctk_image_set_from_file:
 * @image: a #CtkImage
 * @filename: (type filename) (allow-none): a filename or %NULL
 *
 * See ctk_image_new_from_file() for details.
 **/
void
ctk_image_set_from_file   (CtkImage    *image,
                           const gchar *filename)
{
  CtkImagePrivate *priv;
  GdkPixbufAnimation *anim;
  gint scale_factor;
  
  g_return_if_fail (CTK_IS_IMAGE (image));

  priv = image->priv;

  g_object_freeze_notify (G_OBJECT (image));
  
  ctk_image_clear (image);

  if (filename == NULL)
    {
      priv->filename = NULL;
      g_object_thaw_notify (G_OBJECT (image));
      return;
    }

  anim = load_scalable_with_loader (image, filename, NULL, &scale_factor);

  if (anim == NULL)
    {
      ctk_image_set_from_icon_name (image,
                                    "image-missing",
                                    DEFAULT_ICON_SIZE);
      g_object_thaw_notify (G_OBJECT (image));
      return;
    }

  /* We could just unconditionally set_from_animation,
   * but it's nicer for memory if we toss the animation
   * if it's just a single pixbuf
   */

  if (gdk_pixbuf_animation_is_static_image (anim))
    ctk_image_set_from_pixbuf (image,
			       gdk_pixbuf_animation_get_static_image (anim));
  else
    ctk_image_set_from_animation (image, anim);

  _ctk_icon_helper_set_pixbuf_scale (priv->icon_helper, scale_factor);

  g_object_unref (anim);

  priv->filename = g_strdup (filename);
  
  g_object_thaw_notify (G_OBJECT (image));
}

#ifndef GDK_PIXBUF_MAGIC_NUMBER
#define GDK_PIXBUF_MAGIC_NUMBER (0x47646b50)    /* 'CdkP' */
#endif

static gboolean
resource_is_pixdata (const gchar *resource_path)
{
  const guint8 *stream;
  guint32 magic;
  gsize data_size;
  GBytes *bytes;
  gboolean ret = FALSE;

  bytes = g_resources_lookup_data (resource_path, 0, NULL);
  if (bytes == NULL)
    return FALSE;

  stream = g_bytes_get_data (bytes, &data_size);
  if (data_size < sizeof(guint32))
    goto out;

  magic = (stream[0] << 24) + (stream[1] << 16) + (stream[2] << 8) + stream[3];
  if (magic == GDK_PIXBUF_MAGIC_NUMBER)
    ret = TRUE;

out:
  g_bytes_unref (bytes);
  return ret;
}

/**
 * ctk_image_set_from_resource:
 * @image: a #CtkImage
 * @resource_path: (allow-none): a resource path or %NULL
 *
 * See ctk_image_new_from_resource() for details.
 **/
void
ctk_image_set_from_resource (CtkImage    *image,
			     const gchar *resource_path)
{
  CtkImagePrivate *priv;
  GdkPixbufAnimation *animation;
  gint scale_factor = 1;

  g_return_if_fail (CTK_IS_IMAGE (image));

  priv = image->priv;

  g_object_freeze_notify (G_OBJECT (image));

  ctk_image_clear (image);

  if (resource_path == NULL)
    {
      g_object_thaw_notify (G_OBJECT (image));
      return;
    }

  if (resource_is_pixdata (resource_path))
    animation = gdk_pixbuf_animation_new_from_resource (resource_path, NULL);
  else
    animation = load_scalable_with_loader (image, NULL, resource_path, &scale_factor);

  if (animation == NULL)
    {
      ctk_image_set_from_icon_name (image,
                                    "image-missing",
                                    DEFAULT_ICON_SIZE);
      g_object_thaw_notify (G_OBJECT (image));
      return;
    }

  if (gdk_pixbuf_animation_is_static_image (animation))
    ctk_image_set_from_pixbuf (image, gdk_pixbuf_animation_get_static_image (animation));
  else
    ctk_image_set_from_animation (image, animation);

  _ctk_icon_helper_set_pixbuf_scale (priv->icon_helper, scale_factor);

  priv->resource_path = g_strdup (resource_path);

  g_object_notify_by_pspec (G_OBJECT (image), image_props[PROP_RESOURCE]);

  g_object_unref (animation);

  g_object_thaw_notify (G_OBJECT (image));
}


/**
 * ctk_image_set_from_pixbuf:
 * @image: a #CtkImage
 * @pixbuf: (allow-none): a #GdkPixbuf or %NULL
 *
 * See ctk_image_new_from_pixbuf() for details.
 **/
void
ctk_image_set_from_pixbuf (CtkImage  *image,
                           GdkPixbuf *pixbuf)
{
  CtkImagePrivate *priv;

  g_return_if_fail (CTK_IS_IMAGE (image));
  g_return_if_fail (pixbuf == NULL ||
                    CDK_IS_PIXBUF (pixbuf));

  priv = image->priv;

  g_object_freeze_notify (G_OBJECT (image));
  
  ctk_image_clear (image);

  if (pixbuf != NULL)
    _ctk_icon_helper_set_pixbuf (priv->icon_helper, pixbuf);

  g_object_notify_by_pspec (G_OBJECT (image), image_props[PROP_PIXBUF]);
  
  g_object_thaw_notify (G_OBJECT (image));
}

/**
 * ctk_image_set_from_stock:
 * @image: a #CtkImage
 * @stock_id: a stock icon name
 * @size: (type int): a stock icon size (#CtkIconSize)
 *
 * See ctk_image_new_from_stock() for details.
 *
 * Deprecated: 3.10: Use ctk_image_set_from_icon_name() instead.
 **/
void
ctk_image_set_from_stock  (CtkImage       *image,
                           const gchar    *stock_id,
                           CtkIconSize     size)
{
  CtkImagePrivate *priv;
  gchar *new_id;

  g_return_if_fail (CTK_IS_IMAGE (image));

  priv = image->priv;

  g_object_freeze_notify (G_OBJECT (image));

  new_id = g_strdup (stock_id);
  ctk_image_clear (image);

  if (new_id)
    {
      _ctk_icon_helper_set_stock_id (priv->icon_helper, new_id, size);
      g_free (new_id);
    }

  g_object_notify_by_pspec (G_OBJECT (image), image_props[PROP_STOCK]);
  g_object_notify_by_pspec (G_OBJECT (image), image_props[PROP_ICON_SIZE]);

  g_object_thaw_notify (G_OBJECT (image));
}

/**
 * ctk_image_set_from_icon_set:
 * @image: a #CtkImage
 * @icon_set: a #CtkIconSet
 * @size: (type int): a stock icon size (#CtkIconSize)
 *
 * See ctk_image_new_from_icon_set() for details.
 *
 * Deprecated: 3.10: Use ctk_image_set_from_icon_name() instead.
 **/
void
ctk_image_set_from_icon_set  (CtkImage       *image,
                              CtkIconSet     *icon_set,
                              CtkIconSize     size)
{
  CtkImagePrivate *priv;

  g_return_if_fail (CTK_IS_IMAGE (image));

  priv = image->priv;

  g_object_freeze_notify (G_OBJECT (image));

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;

  if (icon_set)
    ctk_icon_set_ref (icon_set);

  ctk_image_clear (image);

  if (icon_set)
    {
      _ctk_icon_helper_set_icon_set (priv->icon_helper, icon_set, size);
      ctk_icon_set_unref (icon_set);
    }

  G_GNUC_END_IGNORE_DEPRECATIONS;

  g_object_notify_by_pspec (G_OBJECT (image), image_props[PROP_ICON_SET]);
  g_object_notify_by_pspec (G_OBJECT (image), image_props[PROP_ICON_SIZE]);

  g_object_thaw_notify (G_OBJECT (image));
}

/**
 * ctk_image_set_from_animation:
 * @image: a #CtkImage
 * @animation: the #GdkPixbufAnimation
 * 
 * Causes the #CtkImage to display the given animation (or display
 * nothing, if you set the animation to %NULL).
 **/
void
ctk_image_set_from_animation (CtkImage           *image,
                              GdkPixbufAnimation *animation)
{
  CtkImagePrivate *priv;

  g_return_if_fail (CTK_IS_IMAGE (image));
  g_return_if_fail (animation == NULL ||
                    CDK_IS_PIXBUF_ANIMATION (animation));

  priv = image->priv;

  g_object_freeze_notify (G_OBJECT (image));
  
  if (animation)
    g_object_ref (animation);

  ctk_image_clear (image);

  if (animation != NULL)
    {
      _ctk_icon_helper_set_animation (priv->icon_helper, animation);
      g_object_unref (animation);
    }

  g_object_notify_by_pspec (G_OBJECT (image), image_props[PROP_PIXBUF_ANIMATION]);

  g_object_thaw_notify (G_OBJECT (image));
}

/**
 * ctk_image_set_from_icon_name:
 * @image: a #CtkImage
 * @icon_name: (nullable): an icon name or %NULL
 * @size: (type int): an icon size (#CtkIconSize)
 *
 * See ctk_image_new_from_icon_name() for details.
 * 
 * Since: 2.6
 **/
void
ctk_image_set_from_icon_name  (CtkImage       *image,
			       const gchar    *icon_name,
			       CtkIconSize     size)
{
  CtkImagePrivate *priv;

  g_return_if_fail (CTK_IS_IMAGE (image));

  priv = image->priv;

  g_object_freeze_notify (G_OBJECT (image));

  ctk_image_clear (image);

  if (icon_name)
    _ctk_icon_helper_set_icon_name (priv->icon_helper, icon_name, size);

  g_object_notify_by_pspec (G_OBJECT (image), image_props[PROP_ICON_NAME]);
  g_object_notify_by_pspec (G_OBJECT (image), image_props[PROP_ICON_SIZE]);

  g_object_thaw_notify (G_OBJECT (image));
}

/**
 * ctk_image_set_from_gicon:
 * @image: a #CtkImage
 * @icon: an icon
 * @size: (type int): an icon size (#CtkIconSize)
 *
 * See ctk_image_new_from_gicon() for details.
 * 
 * Since: 2.14
 **/
void
ctk_image_set_from_gicon  (CtkImage       *image,
			   GIcon          *icon,
			   CtkIconSize     size)
{
  CtkImagePrivate *priv;

  g_return_if_fail (CTK_IS_IMAGE (image));

  priv = image->priv;

  g_object_freeze_notify (G_OBJECT (image));

  if (icon)
    g_object_ref (icon);

  ctk_image_clear (image);

  if (icon)
    {
      _ctk_icon_helper_set_gicon (priv->icon_helper, icon, size);
      g_object_unref (icon);
    }

  g_object_notify_by_pspec (G_OBJECT (image), image_props[PROP_GICON]);
  g_object_notify_by_pspec (G_OBJECT (image), image_props[PROP_ICON_SIZE]);
  
  g_object_thaw_notify (G_OBJECT (image));
}

/**
 * ctk_image_set_from_surface:
 * @image: a #CtkImage
 * @surface: (nullable): a cairo_surface_t or %NULL
 *
 * See ctk_image_new_from_surface() for details.
 * 
 * Since: 3.10
 **/
void
ctk_image_set_from_surface (CtkImage       *image,
			    cairo_surface_t *surface)
{
  CtkImagePrivate *priv;

  g_return_if_fail (CTK_IS_IMAGE (image));

  priv = image->priv;

  g_object_freeze_notify (G_OBJECT (image));

  if (surface)
    cairo_surface_reference (surface);

  ctk_image_clear (image);

  if (surface)
    {
      _ctk_icon_helper_set_surface (priv->icon_helper, surface);
      cairo_surface_destroy (surface);
    }

  g_object_notify_by_pspec (G_OBJECT (image), image_props[PROP_SURFACE]);
  
  g_object_thaw_notify (G_OBJECT (image));
}

/**
 * ctk_image_get_storage_type:
 * @image: a #CtkImage
 * 
 * Gets the type of representation being used by the #CtkImage
 * to store image data. If the #CtkImage has no image data,
 * the return value will be %CTK_IMAGE_EMPTY.
 * 
 * Returns: image representation being used
 **/
CtkImageType
ctk_image_get_storage_type (CtkImage *image)
{
  g_return_val_if_fail (CTK_IS_IMAGE (image), CTK_IMAGE_EMPTY);

  return _ctk_icon_helper_get_storage_type (image->priv->icon_helper);
}

/**
 * ctk_image_get_pixbuf:
 * @image: a #CtkImage
 *
 * Gets the #GdkPixbuf being displayed by the #CtkImage.
 * The storage type of the image must be %CTK_IMAGE_EMPTY or
 * %CTK_IMAGE_PIXBUF (see ctk_image_get_storage_type()).
 * The caller of this function does not own a reference to the
 * returned pixbuf.
 * 
 * Returns: (nullable) (transfer none): the displayed pixbuf, or %NULL if
 * the image is empty
 **/
GdkPixbuf*
ctk_image_get_pixbuf (CtkImage *image)
{
  g_return_val_if_fail (CTK_IS_IMAGE (image), NULL);

  return _ctk_icon_helper_peek_pixbuf (image->priv->icon_helper);
}

/**
 * ctk_image_get_stock:
 * @image: a #CtkImage
 * @stock_id: (out) (transfer none) (allow-none): place to store a
 *     stock icon name, or %NULL
 * @size: (out) (allow-none) (type int): place to store a stock icon
 *     size (#CtkIconSize), or %NULL
 *
 * Gets the stock icon name and size being displayed by the #CtkImage.
 * The storage type of the image must be %CTK_IMAGE_EMPTY or
 * %CTK_IMAGE_STOCK (see ctk_image_get_storage_type()).
 * The returned string is owned by the #CtkImage and should not
 * be freed.
 *
 * Deprecated: 3.10: Use ctk_image_get_icon_name() instead.
 **/
void
ctk_image_get_stock  (CtkImage        *image,
                      gchar          **stock_id,
                      CtkIconSize     *size)
{
  CtkImagePrivate *priv;

  g_return_if_fail (CTK_IS_IMAGE (image));

  priv = image->priv;

  if (stock_id)
    *stock_id = (gchar *) _ctk_icon_helper_get_stock_id (priv->icon_helper);

  if (size)
    *size = _ctk_icon_helper_get_icon_size (priv->icon_helper);
}

/**
 * ctk_image_get_icon_set:
 * @image: a #CtkImage
 * @icon_set: (out) (transfer none) (allow-none): location to store a
 *     #CtkIconSet, or %NULL
 * @size: (out) (allow-none) (type int): location to store a stock
 *     icon size (#CtkIconSize), or %NULL
 *
 * Gets the icon set and size being displayed by the #CtkImage.
 * The storage type of the image must be %CTK_IMAGE_EMPTY or
 * %CTK_IMAGE_ICON_SET (see ctk_image_get_storage_type()).
 *
 * Deprecated: 3.10: Use ctk_image_get_icon_name() instead.
 **/
void
ctk_image_get_icon_set  (CtkImage        *image,
                         CtkIconSet     **icon_set,
                         CtkIconSize     *size)
{
  CtkImagePrivate *priv;

  g_return_if_fail (CTK_IS_IMAGE (image));

  priv = image->priv;

  if (icon_set)
    *icon_set = _ctk_icon_helper_peek_icon_set (priv->icon_helper);

  if (size)
    *size = _ctk_icon_helper_get_icon_size (priv->icon_helper);
}

/**
 * ctk_image_get_animation:
 * @image: a #CtkImage
 *
 * Gets the #GdkPixbufAnimation being displayed by the #CtkImage.
 * The storage type of the image must be %CTK_IMAGE_EMPTY or
 * %CTK_IMAGE_ANIMATION (see ctk_image_get_storage_type()).
 * The caller of this function does not own a reference to the
 * returned animation.
 * 
 * Returns: (nullable) (transfer none): the displayed animation, or %NULL if
 * the image is empty
 **/
GdkPixbufAnimation*
ctk_image_get_animation (CtkImage *image)
{
  CtkImagePrivate *priv;

  g_return_val_if_fail (CTK_IS_IMAGE (image), NULL);

  priv = image->priv;

  return _ctk_icon_helper_peek_animation (priv->icon_helper);
}

/**
 * ctk_image_get_icon_name:
 * @image: a #CtkImage
 * @icon_name: (out) (transfer none) (allow-none): place to store an
 *     icon name, or %NULL
 * @size: (out) (allow-none) (type int): place to store an icon size
 *     (#CtkIconSize), or %NULL
 *
 * Gets the icon name and size being displayed by the #CtkImage.
 * The storage type of the image must be %CTK_IMAGE_EMPTY or
 * %CTK_IMAGE_ICON_NAME (see ctk_image_get_storage_type()).
 * The returned string is owned by the #CtkImage and should not
 * be freed.
 * 
 * Since: 2.6
 **/
void
ctk_image_get_icon_name  (CtkImage     *image,
			  const gchar **icon_name,
			  CtkIconSize  *size)
{
  CtkImagePrivate *priv;

  g_return_if_fail (CTK_IS_IMAGE (image));

  priv = image->priv;

  if (icon_name)
    *icon_name = _ctk_icon_helper_get_icon_name (priv->icon_helper);

  if (size)
    *size = _ctk_icon_helper_get_icon_size (priv->icon_helper);
}

/**
 * ctk_image_get_gicon:
 * @image: a #CtkImage
 * @gicon: (out) (transfer none) (allow-none): place to store a
 *     #GIcon, or %NULL
 * @size: (out) (allow-none) (type int): place to store an icon size
 *     (#CtkIconSize), or %NULL
 *
 * Gets the #GIcon and size being displayed by the #CtkImage.
 * The storage type of the image must be %CTK_IMAGE_EMPTY or
 * %CTK_IMAGE_GICON (see ctk_image_get_storage_type()).
 * The caller of this function does not own a reference to the
 * returned #GIcon.
 * 
 * Since: 2.14
 **/
void
ctk_image_get_gicon (CtkImage     *image,
		     GIcon       **gicon,
		     CtkIconSize  *size)
{
  CtkImagePrivate *priv;

  g_return_if_fail (CTK_IS_IMAGE (image));

  priv = image->priv;

  if (gicon)
    *gicon = _ctk_icon_helper_peek_gicon (priv->icon_helper);

  if (size)
    *size = _ctk_icon_helper_get_icon_size (priv->icon_helper);
}

/**
 * ctk_image_new:
 * 
 * Creates a new empty #CtkImage widget.
 * 
 * Returns: a newly created #CtkImage widget. 
 **/
CtkWidget*
ctk_image_new (void)
{
  return g_object_new (CTK_TYPE_IMAGE, NULL);
}

static void
ctk_image_reset_anim_iter (CtkImage *image)
{
  CtkImagePrivate *priv = image->priv;

  if (ctk_image_get_storage_type (image) == CTK_IMAGE_ANIMATION)
    {
      /* Reset the animation */
      if (priv->animation_timeout)
        {
          g_source_remove (priv->animation_timeout);
          priv->animation_timeout = 0;
        }

      g_clear_object (&priv->animation_iter);
    }
}

static void
ctk_image_size_allocate (CtkWidget     *widget,
                         CtkAllocation *allocation)
{
  CtkAllocation clip;
  CdkRectangle extents;

  ctk_widget_set_allocation (widget, allocation);
  ctk_css_gadget_allocate (CTK_IMAGE (widget)->priv->gadget,
                           allocation,
                           ctk_widget_get_allocated_baseline (widget),
                           &clip);

  _ctk_style_context_get_icon_extents (ctk_widget_get_style_context (widget),
                                       &extents,
                                       allocation->x,
                                       allocation->y,
                                       allocation->width,
                                       allocation->height);

  cdk_rectangle_union (&clip, &extents, &clip);

  ctk_widget_set_clip (widget, &clip);
}

static void
ctk_image_unmap (CtkWidget *widget)
{
  ctk_image_reset_anim_iter (CTK_IMAGE (widget));

  CTK_WIDGET_CLASS (ctk_image_parent_class)->unmap (widget);
}

static void
ctk_image_unrealize (CtkWidget *widget)
{
  ctk_image_reset_anim_iter (CTK_IMAGE (widget));

  CTK_WIDGET_CLASS (ctk_image_parent_class)->unrealize (widget);
}

static gint
animation_timeout (gpointer data)
{
  CtkImage *image = CTK_IMAGE (data);
  CtkImagePrivate *priv = image->priv;
  int delay;

  priv->animation_timeout = 0;

  gdk_pixbuf_animation_iter_advance (priv->animation_iter, NULL);

  delay = gdk_pixbuf_animation_iter_get_delay_time (priv->animation_iter);
  if (delay >= 0)
    {
      CtkWidget *widget = CTK_WIDGET (image);

      priv->animation_timeout =
        cdk_threads_add_timeout (delay, animation_timeout, image);
      g_source_set_name_by_id (priv->animation_timeout, "[ctk+] animation_timeout");

      ctk_widget_queue_draw (widget);
    }

  return FALSE;
}

static GdkPixbuf *
get_animation_frame (CtkImage *image)
{
  CtkImagePrivate *priv = image->priv;

  if (priv->animation_iter == NULL)
    {
      int delay;

      priv->animation_iter = 
        gdk_pixbuf_animation_get_iter (_ctk_icon_helper_peek_animation (priv->icon_helper), NULL);

      delay = gdk_pixbuf_animation_iter_get_delay_time (priv->animation_iter);
      if (delay >= 0) {
        priv->animation_timeout =
          cdk_threads_add_timeout (delay, animation_timeout, image);
        g_source_set_name_by_id (priv->animation_timeout, "[ctk+] animation_timeout");
      }
    }

  /* don't advance the anim iter here, or we could get frame changes between two
   * exposes of different areas.
   */
  return g_object_ref (gdk_pixbuf_animation_iter_get_pixbuf (priv->animation_iter));
}

static float
ctk_image_get_baseline_align (CtkImage *image)
{
  PangoContext *pango_context;
  PangoFontMetrics *metrics;

  if (image->priv->baseline_align == 0.0)
    {
      pango_context = ctk_widget_get_pango_context (CTK_WIDGET (image));
      metrics = pango_context_get_metrics (pango_context,
					   pango_context_get_font_description (pango_context),
					   pango_context_get_language (pango_context));
      image->priv->baseline_align =
	(float)pango_font_metrics_get_ascent (metrics) /
	(pango_font_metrics_get_ascent (metrics) + pango_font_metrics_get_descent (metrics));

      pango_font_metrics_unref (metrics);
    }

  return image->priv->baseline_align;
}

static void
ctk_image_get_content_size (CtkCssGadget   *gadget,
                            CtkOrientation  orientation,
                            gint            for_size,
                            gint           *minimum,
                            gint           *natural,
                            gint           *minimum_baseline,
                            gint           *natural_baseline,
                            gpointer        unused)
{
  CtkWidget *widget;
  gint width, height;
  float baseline_align;
  gint xpad, ypad;

  widget = ctk_css_gadget_get_owner (gadget);

  _ctk_icon_helper_get_size (CTK_IMAGE (widget)->priv->icon_helper,
                             &width, &height);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  ctk_misc_get_padding (CTK_MISC (widget), &xpad, &ypad);
  width += 2 * xpad;
  height += 2 * ypad;
G_GNUC_END_IGNORE_DEPRECATIONS

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    {
      *minimum = *natural = width;
    }
  else
    {
      baseline_align = ctk_image_get_baseline_align (CTK_IMAGE (widget));
      *minimum = *natural = height;
      if (minimum_baseline)
        *minimum_baseline = height * baseline_align;
      if (natural_baseline)
        *natural_baseline = height * baseline_align;
    }

}

static gboolean
ctk_image_draw (CtkWidget *widget,
                cairo_t   *cr)
{
  ctk_css_gadget_draw (CTK_IMAGE (widget)->priv->gadget,
                       cr);

  return FALSE;
}

static gboolean
ctk_image_render_contents (CtkCssGadget *gadget,
                           cairo_t      *cr,
                           int           x,
                           int           y,
                           int           width,
                           int           height,
                           gpointer      data)
{
  CtkWidget *widget;
  CtkImage *image;
  CtkImagePrivate *priv;
  gint w, h, baseline;
  gfloat xalign, yalign;
  gint xpad, ypad;

  widget = ctk_css_gadget_get_owner (gadget);
  image = CTK_IMAGE (widget);
  priv = image->priv;

  _ctk_icon_helper_get_size (priv->icon_helper, &w, &h);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  ctk_misc_get_alignment (CTK_MISC (image), &xalign, &yalign);
  ctk_misc_get_padding (CTK_MISC (image), &xpad, &ypad);
G_GNUC_END_IGNORE_DEPRECATIONS

  if (ctk_widget_get_direction (widget) != CTK_TEXT_DIR_LTR)
    xalign = 1.0 - xalign;

  baseline = ctk_widget_get_allocated_baseline (widget);

  x += floor ((width - 2 * xpad - w) * xalign + xpad);
  if (baseline == -1)
    y += floor ((height - 2 * ypad - h) * yalign + ypad);
  else
    y += CLAMP (baseline - h * ctk_image_get_baseline_align (image), ypad, height - 2 * ypad - h);

  if (ctk_image_get_storage_type (image) == CTK_IMAGE_ANIMATION)
    {
      CtkStyleContext *context = ctk_widget_get_style_context (widget);
      GdkPixbuf *pixbuf = get_animation_frame (image);

      ctk_render_icon (context, cr, pixbuf, x, y);
      g_object_unref (pixbuf);
    }
  else
    {
      _ctk_icon_helper_draw (priv->icon_helper, cr, x, y);
    }

  return FALSE;
}

static void
ctk_image_notify_for_storage_type (CtkImage     *image,
                                   CtkImageType  storage_type)
{
  switch (storage_type)
    {
    case CTK_IMAGE_PIXBUF:
      g_object_notify_by_pspec (G_OBJECT (image), image_props[PROP_PIXBUF]);
      break;
    case CTK_IMAGE_STOCK:
      g_object_notify_by_pspec (G_OBJECT (image), image_props[PROP_STOCK]);
      break;
    case CTK_IMAGE_ICON_SET:
      g_object_notify_by_pspec (G_OBJECT (image), image_props[PROP_ICON_SET]);
      break;
    case CTK_IMAGE_ANIMATION:
      g_object_notify_by_pspec (G_OBJECT (image), image_props[PROP_PIXBUF_ANIMATION]);
      break;
    case CTK_IMAGE_ICON_NAME:
      g_object_notify_by_pspec (G_OBJECT (image), image_props[PROP_ICON_NAME]);
      break;
    case CTK_IMAGE_GICON:
      g_object_notify_by_pspec (G_OBJECT (image), image_props[PROP_GICON]);
      break;
    case CTK_IMAGE_EMPTY:
    default:
      break;
    }
}

void
ctk_image_set_from_definition (CtkImage           *image,
                               CtkImageDefinition *def,
                               CtkIconSize         icon_size)
{
  CtkImagePrivate *priv;

  g_return_if_fail (CTK_IS_IMAGE (image));

  priv = image->priv;

  g_object_freeze_notify (G_OBJECT (image));
  
  ctk_image_clear (image);

  if (def != NULL)
    {
      _ctk_icon_helper_set_definition (priv->icon_helper, def);

      ctk_image_notify_for_storage_type (image, ctk_image_definition_get_storage_type (def));
    }

  _ctk_icon_helper_set_icon_size (priv->icon_helper, icon_size);
  
  g_object_thaw_notify (G_OBJECT (image));
}

static void
ctk_image_reset (CtkImage *image)
{
  CtkImagePrivate *priv = image->priv;
  CtkImageType storage_type;

  g_object_freeze_notify (G_OBJECT (image));
  storage_type = ctk_image_get_storage_type (image);

  if (storage_type != CTK_IMAGE_EMPTY)
    g_object_notify_by_pspec (G_OBJECT (image), image_props[PROP_STORAGE_TYPE]);

  g_object_notify_by_pspec (G_OBJECT (image), image_props[PROP_ICON_SIZE]);

  ctk_image_reset_anim_iter (image);

  ctk_image_notify_for_storage_type (image, storage_type);

  if (priv->filename)
    {
      g_free (priv->filename);
      priv->filename = NULL;
      g_object_notify_by_pspec (G_OBJECT (image), image_props[PROP_FILE]);
    }

  if (priv->resource_path)
    {
      g_free (priv->resource_path);
      priv->resource_path = NULL;
      g_object_notify_by_pspec (G_OBJECT (image), image_props[PROP_RESOURCE]);
    }

  _ctk_icon_helper_clear (priv->icon_helper);

  g_object_thaw_notify (G_OBJECT (image));
}

/**
 * ctk_image_clear:
 * @image: a #CtkImage
 *
 * Resets the image to be empty.
 *
 * Since: 2.8
 */
void
ctk_image_clear (CtkImage *image)
{
  ctk_image_reset (image);

  if (ctk_widget_get_visible (CTK_WIDGET (image)))
      ctk_widget_queue_resize (CTK_WIDGET (image));
}

static void
ctk_image_get_preferred_width (CtkWidget *widget,
                               gint      *minimum,
                               gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CTK_IMAGE (widget)->priv->gadget,
                                     CTK_ORIENTATION_HORIZONTAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_image_get_preferred_height (CtkWidget *widget,
                                gint      *minimum,
                                gint      *natural)
{
  ctk_css_gadget_get_preferred_size (CTK_IMAGE (widget)->priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     -1,
                                     minimum, natural,
                                     NULL, NULL);
}

static void
ctk_image_get_preferred_height_and_baseline_for_width (CtkWidget *widget,
						       gint       width,
						       gint      *minimum,
						       gint      *natural,
						       gint      *minimum_baseline,
						       gint      *natural_baseline)
{
  ctk_css_gadget_get_preferred_size (CTK_IMAGE (widget)->priv->gadget,
                                     CTK_ORIENTATION_VERTICAL,
                                     width,
                                     minimum, natural,
                                     minimum_baseline, natural_baseline);
}

static void
ctk_image_style_updated (CtkWidget *widget)
{
  CtkImage *image = CTK_IMAGE (widget);
  CtkImagePrivate *priv = image->priv;
  CtkStyleContext *context = ctk_widget_get_style_context (widget);
  CtkCssStyleChange *change = ctk_style_context_get_change (context);

  ctk_icon_helper_invalidate_for_change (priv->icon_helper, change);

  CTK_WIDGET_CLASS (ctk_image_parent_class)->style_updated (widget);

  priv->baseline_align = 0.0;
}

/**
 * ctk_image_set_pixel_size:
 * @image: a #CtkImage
 * @pixel_size: the new pixel size
 * 
 * Sets the pixel size to use for named icons. If the pixel size is set
 * to a value != -1, it is used instead of the icon size set by
 * ctk_image_set_from_icon_name().
 *
 * Since: 2.6
 */
void 
ctk_image_set_pixel_size (CtkImage *image,
			  gint      pixel_size)
{
  g_return_if_fail (CTK_IS_IMAGE (image));

  if (_ctk_icon_helper_set_pixel_size (image->priv->icon_helper, pixel_size)) 
    {
      if (ctk_widget_get_visible (CTK_WIDGET (image)))
        ctk_widget_queue_resize (CTK_WIDGET (image));
      g_object_notify_by_pspec (G_OBJECT (image), image_props[PROP_PIXEL_SIZE]);
    }
}

/**
 * ctk_image_get_pixel_size:
 * @image: a #CtkImage
 * 
 * Gets the pixel size used for named icons.
 *
 * Returns: the pixel size used for named icons.
 *
 * Since: 2.6
 */
gint
ctk_image_get_pixel_size (CtkImage *image)
{
  g_return_val_if_fail (CTK_IS_IMAGE (image), -1);

  return _ctk_icon_helper_get_pixel_size (image->priv->icon_helper);
}
