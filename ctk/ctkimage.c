/* GTK - The GIMP Toolkit
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
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.ctk.org/pub/ctk/. 
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
 * @Title: GtkImage
 * @See_also:#GdkPixbuf
 *
 * The #GtkImage widget displays an image. Various kinds of object
 * can be displayed as an image; most typically, you would load a
 * #GdkPixbuf ("pixel buffer") from a file, and then display that.
 * There’s a convenience function to do this, ctk_image_new_from_file(),
 * used as follows:
 * |[<!-- language="C" -->
 *   GtkWidget *image;
 *   image = ctk_image_new_from_file ("myfile.png");
 * ]|
 * If the file isn’t loaded successfully, the image will contain a
 * “broken image” icon similar to that used in many web browsers.
 * If you want to handle errors in loading the file yourself,
 * for example by displaying an error message, then load the image with
 * gdk_pixbuf_new_from_file(), then create the #GtkImage with
 * ctk_image_new_from_pixbuf().
 *
 * The image file may contain an animation, if so the #GtkImage will
 * display an animation (#GdkPixbufAnimation) instead of a static image.
 *
 * #GtkImage is a subclass of #GtkMisc, which implies that you can
 * align it (center, left, right) and add padding to it, using
 * #GtkMisc methods.
 *
 * #GtkImage is a “no window” widget (has no #GdkWindow of its own),
 * so by default does not receive events. If you want to receive events
 * on the image, such as button clicks, place the image inside a
 * #GtkEventBox, then connect to the event signals on the event box.
 *
 * ## Handling button press events on a #GtkImage.
 *
 * |[<!-- language="C" -->
 *   static gboolean
 *   button_press_callback (GtkWidget      *event_box,
 *                          GdkEventButton *event,
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
 *   static GtkWidget*
 *   create_image (void)
 *   {
 *     GtkWidget *image;
 *     GtkWidget *event_box;
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
 * the alignment and padding settings on the image (see #GtkMisc).
 * The simplest way to solve this is to set the alignment to 0.0
 * (left/top), and set the padding to zero. Then the origin of
 * the image will be the same as the origin of the event box.
 *
 * Sometimes an application will want to avoid depending on external data
 * files, such as image files. GTK+ comes with a program to avoid this,
 * called “gdk-pixbuf-csource”. This library
 * allows you to convert an image into a C variable declaration, which
 * can then be loaded into a #GdkPixbuf using
 * gdk_pixbuf_new_from_inline().
 *
 * # CSS nodes
 *
 * GtkImage has a single CSS node with the name image. The style classes
 * may appear on image CSS nodes: .icon-dropshadow, .lowres-icon.
 */


struct _GtkImagePrivate
{
  GtkIconHelper *icon_helper;

  GdkPixbufAnimationIter *animation_iter;
  gint animation_timeout;

  GtkCssGadget *gadget;

  float baseline_align;

  gchar                *filename;       /* Only used with CTK_IMAGE_ANIMATION, CTK_IMAGE_PIXBUF */
  gchar                *resource_path;  /* Only used with CTK_IMAGE_PIXBUF */
};


#define DEFAULT_ICON_SIZE CTK_ICON_SIZE_BUTTON
static gint ctk_image_draw                 (GtkWidget    *widget,
                                            cairo_t      *cr);
static void ctk_image_size_allocate        (GtkWidget    *widget,
                                            GtkAllocation*allocation);
static void ctk_image_unmap                (GtkWidget    *widget);
static void ctk_image_unrealize            (GtkWidget    *widget);
static void ctk_image_get_preferred_width  (GtkWidget    *widget,
                                            gint         *minimum,
                                            gint         *natural);
static void ctk_image_get_preferred_height (GtkWidget    *widget,
                                            gint         *minimum,
                                            gint         *natural);
static void ctk_image_get_preferred_height_and_baseline_for_width (GtkWidget *widget,
								   gint       width,
								   gint      *minimum,
								   gint      *natural,
								   gint      *minimum_baseline,
								   gint      *natural_baseline);

static void ctk_image_get_content_size (GtkCssGadget   *gadget,
                                        GtkOrientation  orientation,
                                        gint            for_size,
                                        gint           *minimum,
                                        gint           *natural,
                                        gint           *minimum_baseline,
                                        gint           *natural_baseline,
                                        gpointer        unused);
static gboolean ctk_image_render_contents (GtkCssGadget *gadget,
                                           cairo_t      *cr,
                                           int           x,
                                           int           y,
                                           int           width,
                                           int           height,
                                           gpointer      data);

static void ctk_image_style_updated        (GtkWidget    *widget);
static void ctk_image_finalize             (GObject      *object);
static void ctk_image_reset                (GtkImage     *image);

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
G_DEFINE_TYPE_WITH_PRIVATE (GtkImage, ctk_image, CTK_TYPE_MISC)
G_GNUC_END_IGNORE_DEPRECATIONS

static void
ctk_image_class_init (GtkImageClass *class)
{
  GObjectClass *gobject_class;
  GtkWidgetClass *widget_class;

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
                           GDK_TYPE_PIXBUF,
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
   * GtkImage:stock:
   *
   * Deprecated: 3.10: Use #GtkImage:icon-name instead.
   */
  image_props[PROP_STOCK] =
      g_param_spec_string ("stock",
                           P_("Stock ID"),
                           P_("Stock ID for a stock image to display"),
                           NULL,
                           CTK_PARAM_READWRITE | G_PARAM_DEPRECATED);

  /**
   * GtkImage:icon-set:
   *
   * Deprecated: 3.10: Use #GtkImage:icon-name instead.
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
   * GtkImage:pixel-size:
   *
   * The "pixel-size" property can be used to specify a fixed size
   * overriding the #GtkImage:icon-size property for images of type
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
                           GDK_TYPE_PIXBUF_ANIMATION,
                           CTK_PARAM_READWRITE);

  /**
   * GtkImage:icon-name:
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
   * GtkImage:gicon:
   *
   * The GIcon displayed in the GtkImage. For themed icons,
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
   * GtkImage:resource:
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
   * GtkImage:use-fallback:
   *
   * Whether the icon displayed in the GtkImage will use
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
ctk_image_init (GtkImage *image)
{
  GtkImagePrivate *priv;
  GtkCssNode *widget_node;

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
  GtkImage *image = CTK_IMAGE (object);

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
  GtkImage *image = CTK_IMAGE (object);
  GtkImagePrivate *priv = image->priv;
  GtkIconSize icon_size = _ctk_icon_helper_get_icon_size (priv->icon_helper);

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
  GtkImage *image = CTK_IMAGE (object);
  GtkImagePrivate *priv = image->priv;

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
 * Creates a new #GtkImage displaying the file @filename. If the file
 * isn’t found or can’t be loaded, the resulting #GtkImage will
 * display a “broken image” icon. This function never returns %NULL,
 * it always returns a valid #GtkImage widget.
 *
 * If the file contains an animation, the image will contain an
 * animation.
 *
 * If you need to detect failures to load the file, use
 * gdk_pixbuf_new_from_file() to load the file yourself, then create
 * the #GtkImage from the pixbuf. (Or for animations, use
 * gdk_pixbuf_animation_new_from_file()).
 *
 * The storage type (ctk_image_get_storage_type()) of the returned
 * image is not defined, it will be whatever is appropriate for
 * displaying the file.
 * 
 * Returns: a new #GtkImage
 **/
GtkWidget*
ctk_image_new_from_file   (const gchar *filename)
{
  GtkImage *image;

  image = g_object_new (CTK_TYPE_IMAGE, NULL);

  ctk_image_set_from_file (image, filename);

  return CTK_WIDGET (image);
}

/**
 * ctk_image_new_from_resource:
 * @resource_path: a resource path
 *
 * Creates a new #GtkImage displaying the resource file @resource_path. If the file
 * isn’t found or can’t be loaded, the resulting #GtkImage will
 * display a “broken image” icon. This function never returns %NULL,
 * it always returns a valid #GtkImage widget.
 *
 * If the file contains an animation, the image will contain an
 * animation.
 *
 * If you need to detect failures to load the file, use
 * gdk_pixbuf_new_from_file() to load the file yourself, then create
 * the #GtkImage from the pixbuf. (Or for animations, use
 * gdk_pixbuf_animation_new_from_file()).
 *
 * The storage type (ctk_image_get_storage_type()) of the returned
 * image is not defined, it will be whatever is appropriate for
 * displaying the file.
 *
 * Returns: a new #GtkImage
 *
 * Since: 3.4
 **/
GtkWidget*
ctk_image_new_from_resource (const gchar *resource_path)
{
  GtkImage *image;

  image = g_object_new (CTK_TYPE_IMAGE, NULL);

  ctk_image_set_from_resource (image, resource_path);

  return CTK_WIDGET (image);
}

/**
 * ctk_image_new_from_pixbuf:
 * @pixbuf: (allow-none): a #GdkPixbuf, or %NULL
 *
 * Creates a new #GtkImage displaying @pixbuf.
 * The #GtkImage does not assume a reference to the
 * pixbuf; you still need to unref it if you own references.
 * #GtkImage will add its own reference rather than adopting yours.
 * 
 * Note that this function just creates an #GtkImage from the pixbuf. The
 * #GtkImage created will not react to state changes. Should you want that, 
 * you should use ctk_image_new_from_icon_name().
 * 
 * Returns: a new #GtkImage
 **/
GtkWidget*
ctk_image_new_from_pixbuf (GdkPixbuf *pixbuf)
{
  GtkImage *image;

  image = g_object_new (CTK_TYPE_IMAGE, NULL);

  ctk_image_set_from_pixbuf (image, pixbuf);

  return CTK_WIDGET (image);  
}

/**
 * ctk_image_new_from_surface:
 * @surface: (allow-none): a #cairo_surface_t, or %NULL
 *
 * Creates a new #GtkImage displaying @surface.
 * The #GtkImage does not assume a reference to the
 * surface; you still need to unref it if you own references.
 * #GtkImage will add its own reference rather than adopting yours.
 * 
 * Returns: a new #GtkImage
 *
 * Since: 3.10
 **/
GtkWidget*
ctk_image_new_from_surface (cairo_surface_t *surface)
{
  GtkImage *image;

  image = g_object_new (CTK_TYPE_IMAGE, NULL);

  ctk_image_set_from_surface (image, surface);

  return CTK_WIDGET (image);  
}

/**
 * ctk_image_new_from_stock:
 * @stock_id: a stock icon name
 * @size: (type int): a stock icon size (#GtkIconSize)
 * 
 * Creates a #GtkImage displaying a stock icon. Sample stock icon
 * names are #CTK_STOCK_OPEN, #CTK_STOCK_QUIT. Sample stock sizes
 * are #CTK_ICON_SIZE_MENU, #CTK_ICON_SIZE_SMALL_TOOLBAR. If the stock
 * icon name isn’t known, the image will be empty.
 * You can register your own stock icon names, see
 * ctk_icon_factory_add_default() and ctk_icon_factory_add().
 * 
 * Returns: a new #GtkImage displaying the stock icon
 *
 * Deprecated: 3.10: Use ctk_image_new_from_icon_name() instead.
 **/
GtkWidget*
ctk_image_new_from_stock (const gchar    *stock_id,
                          GtkIconSize     size)
{
  GtkImage *image;

  image = g_object_new (CTK_TYPE_IMAGE, NULL);

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  ctk_image_set_from_stock (image, stock_id, size);
  G_GNUC_END_IGNORE_DEPRECATIONS;

  return CTK_WIDGET (image);
}

/**
 * ctk_image_new_from_icon_set:
 * @icon_set: a #GtkIconSet
 * @size: (type int): a stock icon size (#GtkIconSize)
 *
 * Creates a #GtkImage displaying an icon set. Sample stock sizes are
 * #CTK_ICON_SIZE_MENU, #CTK_ICON_SIZE_SMALL_TOOLBAR. Instead of using
 * this function, usually it’s better to create a #GtkIconFactory, put
 * your icon sets in the icon factory, add the icon factory to the
 * list of default factories with ctk_icon_factory_add_default(), and
 * then use ctk_image_new_from_stock(). This will allow themes to
 * override the icon you ship with your application.
 *
 * The #GtkImage does not assume a reference to the
 * icon set; you still need to unref it if you own references.
 * #GtkImage will add its own reference rather than adopting yours.
 * 
 * Returns: a new #GtkImage
 *
 * Deprecated: 3.10: Use ctk_image_new_from_icon_name() instead.
 **/
GtkWidget*
ctk_image_new_from_icon_set (GtkIconSet     *icon_set,
                             GtkIconSize     size)
{
  GtkImage *image;

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
 * Creates a #GtkImage displaying the given animation.
 * The #GtkImage does not assume a reference to the
 * animation; you still need to unref it if you own references.
 * #GtkImage will add its own reference rather than adopting yours.
 *
 * Note that the animation frames are shown using a timeout with
 * #G_PRIORITY_DEFAULT. When using animations to indicate busyness,
 * keep in mind that the animation will only be shown if the main loop
 * is not busy with something that has a higher priority.
 *
 * Returns: a new #GtkImage widget
 **/
GtkWidget*
ctk_image_new_from_animation (GdkPixbufAnimation *animation)
{
  GtkImage *image;

  g_return_val_if_fail (GDK_IS_PIXBUF_ANIMATION (animation), NULL);
  
  image = g_object_new (CTK_TYPE_IMAGE, NULL);

  ctk_image_set_from_animation (image, animation);

  return CTK_WIDGET (image);
}

/**
 * ctk_image_new_from_icon_name:
 * @icon_name: (nullable): an icon name or %NULL
 * @size: (type int): a stock icon size (#GtkIconSize)
 * 
 * Creates a #GtkImage displaying an icon from the current icon theme.
 * If the icon name isn’t known, a “broken image” icon will be
 * displayed instead.  If the current icon theme is changed, the icon
 * will be updated appropriately.
 * 
 * Returns: a new #GtkImage displaying the themed icon
 *
 * Since: 2.6
 **/
GtkWidget*
ctk_image_new_from_icon_name (const gchar    *icon_name,
			      GtkIconSize     size)
{
  GtkImage *image;

  image = g_object_new (CTK_TYPE_IMAGE, NULL);

  ctk_image_set_from_icon_name (image, icon_name, size);

  return CTK_WIDGET (image);
}

/**
 * ctk_image_new_from_gicon:
 * @icon: an icon
 * @size: (type int): a stock icon size (#GtkIconSize)
 * 
 * Creates a #GtkImage displaying an icon from the current icon theme.
 * If the icon name isn’t known, a “broken image” icon will be
 * displayed instead.  If the current icon theme is changed, the icon
 * will be updated appropriately.
 * 
 * Returns: a new #GtkImage displaying the themed icon
 *
 * Since: 2.14
 **/
GtkWidget*
ctk_image_new_from_gicon (GIcon *icon,
			  GtkIconSize     size)
{
  GtkImage *image;

  image = g_object_new (CTK_TYPE_IMAGE, NULL);

  ctk_image_set_from_gicon (image, icon, size);

  return CTK_WIDGET (image);
}

typedef struct {
  GtkImage *image;
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
load_scalable_with_loader (GtkImage    *image,
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
 * @image: a #GtkImage
 * @filename: (type filename) (allow-none): a filename or %NULL
 *
 * See ctk_image_new_from_file() for details.
 **/
void
ctk_image_set_from_file   (GtkImage    *image,
                           const gchar *filename)
{
  GtkImagePrivate *priv;
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
#define GDK_PIXBUF_MAGIC_NUMBER (0x47646b50)    /* 'GdkP' */
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
 * @image: a #GtkImage
 * @resource_path: (allow-none): a resource path or %NULL
 *
 * See ctk_image_new_from_resource() for details.
 **/
void
ctk_image_set_from_resource (GtkImage    *image,
			     const gchar *resource_path)
{
  GtkImagePrivate *priv;
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
 * @image: a #GtkImage
 * @pixbuf: (allow-none): a #GdkPixbuf or %NULL
 *
 * See ctk_image_new_from_pixbuf() for details.
 **/
void
ctk_image_set_from_pixbuf (GtkImage  *image,
                           GdkPixbuf *pixbuf)
{
  GtkImagePrivate *priv;

  g_return_if_fail (CTK_IS_IMAGE (image));
  g_return_if_fail (pixbuf == NULL ||
                    GDK_IS_PIXBUF (pixbuf));

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
 * @image: a #GtkImage
 * @stock_id: a stock icon name
 * @size: (type int): a stock icon size (#GtkIconSize)
 *
 * See ctk_image_new_from_stock() for details.
 *
 * Deprecated: 3.10: Use ctk_image_set_from_icon_name() instead.
 **/
void
ctk_image_set_from_stock  (GtkImage       *image,
                           const gchar    *stock_id,
                           GtkIconSize     size)
{
  GtkImagePrivate *priv;
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
 * @image: a #GtkImage
 * @icon_set: a #GtkIconSet
 * @size: (type int): a stock icon size (#GtkIconSize)
 *
 * See ctk_image_new_from_icon_set() for details.
 *
 * Deprecated: 3.10: Use ctk_image_set_from_icon_name() instead.
 **/
void
ctk_image_set_from_icon_set  (GtkImage       *image,
                              GtkIconSet     *icon_set,
                              GtkIconSize     size)
{
  GtkImagePrivate *priv;

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
 * @image: a #GtkImage
 * @animation: the #GdkPixbufAnimation
 * 
 * Causes the #GtkImage to display the given animation (or display
 * nothing, if you set the animation to %NULL).
 **/
void
ctk_image_set_from_animation (GtkImage           *image,
                              GdkPixbufAnimation *animation)
{
  GtkImagePrivate *priv;

  g_return_if_fail (CTK_IS_IMAGE (image));
  g_return_if_fail (animation == NULL ||
                    GDK_IS_PIXBUF_ANIMATION (animation));

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
 * @image: a #GtkImage
 * @icon_name: (nullable): an icon name or %NULL
 * @size: (type int): an icon size (#GtkIconSize)
 *
 * See ctk_image_new_from_icon_name() for details.
 * 
 * Since: 2.6
 **/
void
ctk_image_set_from_icon_name  (GtkImage       *image,
			       const gchar    *icon_name,
			       GtkIconSize     size)
{
  GtkImagePrivate *priv;

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
 * @image: a #GtkImage
 * @icon: an icon
 * @size: (type int): an icon size (#GtkIconSize)
 *
 * See ctk_image_new_from_gicon() for details.
 * 
 * Since: 2.14
 **/
void
ctk_image_set_from_gicon  (GtkImage       *image,
			   GIcon          *icon,
			   GtkIconSize     size)
{
  GtkImagePrivate *priv;

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
 * @image: a #GtkImage
 * @surface: (nullable): a cairo_surface_t or %NULL
 *
 * See ctk_image_new_from_surface() for details.
 * 
 * Since: 3.10
 **/
void
ctk_image_set_from_surface (GtkImage       *image,
			    cairo_surface_t *surface)
{
  GtkImagePrivate *priv;

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
 * @image: a #GtkImage
 * 
 * Gets the type of representation being used by the #GtkImage
 * to store image data. If the #GtkImage has no image data,
 * the return value will be %CTK_IMAGE_EMPTY.
 * 
 * Returns: image representation being used
 **/
GtkImageType
ctk_image_get_storage_type (GtkImage *image)
{
  g_return_val_if_fail (CTK_IS_IMAGE (image), CTK_IMAGE_EMPTY);

  return _ctk_icon_helper_get_storage_type (image->priv->icon_helper);
}

/**
 * ctk_image_get_pixbuf:
 * @image: a #GtkImage
 *
 * Gets the #GdkPixbuf being displayed by the #GtkImage.
 * The storage type of the image must be %CTK_IMAGE_EMPTY or
 * %CTK_IMAGE_PIXBUF (see ctk_image_get_storage_type()).
 * The caller of this function does not own a reference to the
 * returned pixbuf.
 * 
 * Returns: (nullable) (transfer none): the displayed pixbuf, or %NULL if
 * the image is empty
 **/
GdkPixbuf*
ctk_image_get_pixbuf (GtkImage *image)
{
  g_return_val_if_fail (CTK_IS_IMAGE (image), NULL);

  return _ctk_icon_helper_peek_pixbuf (image->priv->icon_helper);
}

/**
 * ctk_image_get_stock:
 * @image: a #GtkImage
 * @stock_id: (out) (transfer none) (allow-none): place to store a
 *     stock icon name, or %NULL
 * @size: (out) (allow-none) (type int): place to store a stock icon
 *     size (#GtkIconSize), or %NULL
 *
 * Gets the stock icon name and size being displayed by the #GtkImage.
 * The storage type of the image must be %CTK_IMAGE_EMPTY or
 * %CTK_IMAGE_STOCK (see ctk_image_get_storage_type()).
 * The returned string is owned by the #GtkImage and should not
 * be freed.
 *
 * Deprecated: 3.10: Use ctk_image_get_icon_name() instead.
 **/
void
ctk_image_get_stock  (GtkImage        *image,
                      gchar          **stock_id,
                      GtkIconSize     *size)
{
  GtkImagePrivate *priv;

  g_return_if_fail (CTK_IS_IMAGE (image));

  priv = image->priv;

  if (stock_id)
    *stock_id = (gchar *) _ctk_icon_helper_get_stock_id (priv->icon_helper);

  if (size)
    *size = _ctk_icon_helper_get_icon_size (priv->icon_helper);
}

/**
 * ctk_image_get_icon_set:
 * @image: a #GtkImage
 * @icon_set: (out) (transfer none) (allow-none): location to store a
 *     #GtkIconSet, or %NULL
 * @size: (out) (allow-none) (type int): location to store a stock
 *     icon size (#GtkIconSize), or %NULL
 *
 * Gets the icon set and size being displayed by the #GtkImage.
 * The storage type of the image must be %CTK_IMAGE_EMPTY or
 * %CTK_IMAGE_ICON_SET (see ctk_image_get_storage_type()).
 *
 * Deprecated: 3.10: Use ctk_image_get_icon_name() instead.
 **/
void
ctk_image_get_icon_set  (GtkImage        *image,
                         GtkIconSet     **icon_set,
                         GtkIconSize     *size)
{
  GtkImagePrivate *priv;

  g_return_if_fail (CTK_IS_IMAGE (image));

  priv = image->priv;

  if (icon_set)
    *icon_set = _ctk_icon_helper_peek_icon_set (priv->icon_helper);

  if (size)
    *size = _ctk_icon_helper_get_icon_size (priv->icon_helper);
}

/**
 * ctk_image_get_animation:
 * @image: a #GtkImage
 *
 * Gets the #GdkPixbufAnimation being displayed by the #GtkImage.
 * The storage type of the image must be %CTK_IMAGE_EMPTY or
 * %CTK_IMAGE_ANIMATION (see ctk_image_get_storage_type()).
 * The caller of this function does not own a reference to the
 * returned animation.
 * 
 * Returns: (nullable) (transfer none): the displayed animation, or %NULL if
 * the image is empty
 **/
GdkPixbufAnimation*
ctk_image_get_animation (GtkImage *image)
{
  GtkImagePrivate *priv;

  g_return_val_if_fail (CTK_IS_IMAGE (image), NULL);

  priv = image->priv;

  return _ctk_icon_helper_peek_animation (priv->icon_helper);
}

/**
 * ctk_image_get_icon_name:
 * @image: a #GtkImage
 * @icon_name: (out) (transfer none) (allow-none): place to store an
 *     icon name, or %NULL
 * @size: (out) (allow-none) (type int): place to store an icon size
 *     (#GtkIconSize), or %NULL
 *
 * Gets the icon name and size being displayed by the #GtkImage.
 * The storage type of the image must be %CTK_IMAGE_EMPTY or
 * %CTK_IMAGE_ICON_NAME (see ctk_image_get_storage_type()).
 * The returned string is owned by the #GtkImage and should not
 * be freed.
 * 
 * Since: 2.6
 **/
void
ctk_image_get_icon_name  (GtkImage     *image,
			  const gchar **icon_name,
			  GtkIconSize  *size)
{
  GtkImagePrivate *priv;

  g_return_if_fail (CTK_IS_IMAGE (image));

  priv = image->priv;

  if (icon_name)
    *icon_name = _ctk_icon_helper_get_icon_name (priv->icon_helper);

  if (size)
    *size = _ctk_icon_helper_get_icon_size (priv->icon_helper);
}

/**
 * ctk_image_get_gicon:
 * @image: a #GtkImage
 * @gicon: (out) (transfer none) (allow-none): place to store a
 *     #GIcon, or %NULL
 * @size: (out) (allow-none) (type int): place to store an icon size
 *     (#GtkIconSize), or %NULL
 *
 * Gets the #GIcon and size being displayed by the #GtkImage.
 * The storage type of the image must be %CTK_IMAGE_EMPTY or
 * %CTK_IMAGE_GICON (see ctk_image_get_storage_type()).
 * The caller of this function does not own a reference to the
 * returned #GIcon.
 * 
 * Since: 2.14
 **/
void
ctk_image_get_gicon (GtkImage     *image,
		     GIcon       **gicon,
		     GtkIconSize  *size)
{
  GtkImagePrivate *priv;

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
 * Creates a new empty #GtkImage widget.
 * 
 * Returns: a newly created #GtkImage widget. 
 **/
GtkWidget*
ctk_image_new (void)
{
  return g_object_new (CTK_TYPE_IMAGE, NULL);
}

static void
ctk_image_reset_anim_iter (GtkImage *image)
{
  GtkImagePrivate *priv = image->priv;

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
ctk_image_size_allocate (GtkWidget     *widget,
                         GtkAllocation *allocation)
{
  GtkAllocation clip;
  GdkRectangle extents;

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

  gdk_rectangle_union (&clip, &extents, &clip);

  ctk_widget_set_clip (widget, &clip);
}

static void
ctk_image_unmap (GtkWidget *widget)
{
  ctk_image_reset_anim_iter (CTK_IMAGE (widget));

  CTK_WIDGET_CLASS (ctk_image_parent_class)->unmap (widget);
}

static void
ctk_image_unrealize (GtkWidget *widget)
{
  ctk_image_reset_anim_iter (CTK_IMAGE (widget));

  CTK_WIDGET_CLASS (ctk_image_parent_class)->unrealize (widget);
}

static gint
animation_timeout (gpointer data)
{
  GtkImage *image = CTK_IMAGE (data);
  GtkImagePrivate *priv = image->priv;
  int delay;

  priv->animation_timeout = 0;

  gdk_pixbuf_animation_iter_advance (priv->animation_iter, NULL);

  delay = gdk_pixbuf_animation_iter_get_delay_time (priv->animation_iter);
  if (delay >= 0)
    {
      GtkWidget *widget = CTK_WIDGET (image);

      priv->animation_timeout =
        gdk_threads_add_timeout (delay, animation_timeout, image);
      g_source_set_name_by_id (priv->animation_timeout, "[ctk+] animation_timeout");

      ctk_widget_queue_draw (widget);
    }

  return FALSE;
}

static GdkPixbuf *
get_animation_frame (GtkImage *image)
{
  GtkImagePrivate *priv = image->priv;

  if (priv->animation_iter == NULL)
    {
      int delay;

      priv->animation_iter = 
        gdk_pixbuf_animation_get_iter (_ctk_icon_helper_peek_animation (priv->icon_helper), NULL);

      delay = gdk_pixbuf_animation_iter_get_delay_time (priv->animation_iter);
      if (delay >= 0) {
        priv->animation_timeout =
          gdk_threads_add_timeout (delay, animation_timeout, image);
        g_source_set_name_by_id (priv->animation_timeout, "[ctk+] animation_timeout");
      }
    }

  /* don't advance the anim iter here, or we could get frame changes between two
   * exposes of different areas.
   */
  return g_object_ref (gdk_pixbuf_animation_iter_get_pixbuf (priv->animation_iter));
}

static float
ctk_image_get_baseline_align (GtkImage *image)
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
ctk_image_get_content_size (GtkCssGadget   *gadget,
                            GtkOrientation  orientation,
                            gint            for_size,
                            gint           *minimum,
                            gint           *natural,
                            gint           *minimum_baseline,
                            gint           *natural_baseline,
                            gpointer        unused)
{
  GtkWidget *widget;
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
ctk_image_draw (GtkWidget *widget,
                cairo_t   *cr)
{
  ctk_css_gadget_draw (CTK_IMAGE (widget)->priv->gadget,
                       cr);

  return FALSE;
}

static gboolean
ctk_image_render_contents (GtkCssGadget *gadget,
                           cairo_t      *cr,
                           int           x,
                           int           y,
                           int           width,
                           int           height,
                           gpointer      data)
{
  GtkWidget *widget;
  GtkImage *image;
  GtkImagePrivate *priv;
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
      GtkStyleContext *context = ctk_widget_get_style_context (widget);
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
ctk_image_notify_for_storage_type (GtkImage     *image,
                                   GtkImageType  storage_type)
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
ctk_image_set_from_definition (GtkImage           *image,
                               GtkImageDefinition *def,
                               GtkIconSize         icon_size)
{
  GtkImagePrivate *priv;

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
ctk_image_reset (GtkImage *image)
{
  GtkImagePrivate *priv = image->priv;
  GtkImageType storage_type;

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
 * @image: a #GtkImage
 *
 * Resets the image to be empty.
 *
 * Since: 2.8
 */
void
ctk_image_clear (GtkImage *image)
{
  ctk_image_reset (image);

  if (ctk_widget_get_visible (CTK_WIDGET (image)))
      ctk_widget_queue_resize (CTK_WIDGET (image));
}

static void
ctk_image_get_preferred_width (GtkWidget *widget,
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
ctk_image_get_preferred_height (GtkWidget *widget,
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
ctk_image_get_preferred_height_and_baseline_for_width (GtkWidget *widget,
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
ctk_image_style_updated (GtkWidget *widget)
{
  GtkImage *image = CTK_IMAGE (widget);
  GtkImagePrivate *priv = image->priv;
  GtkStyleContext *context = ctk_widget_get_style_context (widget);
  GtkCssStyleChange *change = ctk_style_context_get_change (context);

  ctk_icon_helper_invalidate_for_change (priv->icon_helper, change);

  CTK_WIDGET_CLASS (ctk_image_parent_class)->style_updated (widget);

  priv->baseline_align = 0.0;
}

/**
 * ctk_image_set_pixel_size:
 * @image: a #GtkImage
 * @pixel_size: the new pixel size
 * 
 * Sets the pixel size to use for named icons. If the pixel size is set
 * to a value != -1, it is used instead of the icon size set by
 * ctk_image_set_from_icon_name().
 *
 * Since: 2.6
 */
void 
ctk_image_set_pixel_size (GtkImage *image,
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
 * @image: a #GtkImage
 * 
 * Gets the pixel size used for named icons.
 *
 * Returns: the pixel size used for named icons.
 *
 * Since: 2.6
 */
gint
ctk_image_get_pixel_size (GtkImage *image)
{
  g_return_val_if_fail (CTK_IS_IMAGE (image), -1);

  return _ctk_icon_helper_get_pixel_size (image->priv->icon_helper);
}
