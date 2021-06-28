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

#ifndef __CTK_IMAGE_H__
#define __CTK_IMAGE_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <gio/gio.h>
#include <ctk/deprecated/ctkmisc.h>


G_BEGIN_DECLS

#define CTK_TYPE_IMAGE                  (ctk_image_get_type ())
#define CTK_IMAGE(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_IMAGE, CtkImage))
#define CTK_IMAGE_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_IMAGE, CtkImageClass))
#define CTK_IS_IMAGE(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_IMAGE))
#define CTK_IS_IMAGE_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_IMAGE))
#define CTK_IMAGE_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_IMAGE, CtkImageClass))


typedef struct _CtkImage              CtkImage;
typedef struct _CtkImagePrivate       CtkImagePrivate;
typedef struct _CtkImageClass         CtkImageClass;

/**
 * CtkImageType:
 * @CTK_IMAGE_EMPTY: there is no image displayed by the widget
 * @CTK_IMAGE_PIXBUF: the widget contains a #CdkPixbuf
 * @CTK_IMAGE_STOCK: the widget contains a [stock item name][ctkstock]
 * @CTK_IMAGE_ICON_SET: the widget contains a #CtkIconSet
 * @CTK_IMAGE_ANIMATION: the widget contains a #CdkPixbufAnimation
 * @CTK_IMAGE_ICON_NAME: the widget contains a named icon.
 *  This image type was added in CTK+ 2.6
 * @CTK_IMAGE_GICON: the widget contains a #GIcon.
 *  This image type was added in CTK+ 2.14
 * @CTK_IMAGE_SURFACE: the widget contains a #cairo_surface_t.
 *  This image type was added in CTK+ 3.10
 *
 * Describes the image data representation used by a #CtkImage. If you
 * want to get the image from the widget, you can only get the
 * currently-stored representation. e.g.  if the
 * ctk_image_get_storage_type() returns #CTK_IMAGE_PIXBUF, then you can
 * call ctk_image_get_pixbuf() but not ctk_image_get_stock().  For empty
 * images, you can request any storage type (call any of the "get"
 * functions), but they will all return %NULL values.
 */
typedef enum
{
  CTK_IMAGE_EMPTY,
  CTK_IMAGE_PIXBUF,
  CTK_IMAGE_STOCK,
  CTK_IMAGE_ICON_SET,
  CTK_IMAGE_ANIMATION,
  CTK_IMAGE_ICON_NAME,
  CTK_IMAGE_GICON,
  CTK_IMAGE_SURFACE
} CtkImageType;

/**
 * CtkImage:
 *
 * This struct contain private data only and should be accessed by the functions
 * below.
 */
struct _CtkImage
{
  CtkMisc misc;

  /*< private >*/
  CtkImagePrivate *priv;
};

struct _CtkImageClass
{
  CtkMiscClass parent_class;

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

CDK_AVAILABLE_IN_ALL
GType      ctk_image_get_type (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_ALL
CtkWidget* ctk_image_new                (void);
CDK_AVAILABLE_IN_ALL
CtkWidget* ctk_image_new_from_file      (const gchar     *filename);
CDK_AVAILABLE_IN_ALL
CtkWidget* ctk_image_new_from_resource  (const gchar     *resource_path);
CDK_AVAILABLE_IN_ALL
CtkWidget* ctk_image_new_from_pixbuf    (CdkPixbuf       *pixbuf);
CDK_DEPRECATED_IN_3_10_FOR(ctk_image_new_from_icon_name)
CtkWidget* ctk_image_new_from_stock     (const gchar     *stock_id,
                                         CtkIconSize      size);
CDK_DEPRECATED_IN_3_10_FOR(ctk_image_new_from_icon_name)
CtkWidget* ctk_image_new_from_icon_set  (CtkIconSet      *icon_set,
                                         CtkIconSize      size);
CDK_AVAILABLE_IN_ALL
CtkWidget* ctk_image_new_from_animation (CdkPixbufAnimation *animation);
CDK_AVAILABLE_IN_ALL
CtkWidget* ctk_image_new_from_icon_name (const gchar     *icon_name,
					 CtkIconSize      size);
CDK_AVAILABLE_IN_ALL
CtkWidget* ctk_image_new_from_gicon     (GIcon           *icon,
					 CtkIconSize      size);
CDK_AVAILABLE_IN_3_10
CtkWidget* ctk_image_new_from_surface   (cairo_surface_t *surface);

CDK_AVAILABLE_IN_ALL
void ctk_image_clear              (CtkImage        *image);
CDK_AVAILABLE_IN_ALL
void ctk_image_set_from_file      (CtkImage        *image,
                                   const gchar     *filename);
CDK_AVAILABLE_IN_ALL
void ctk_image_set_from_resource  (CtkImage        *image,
                                   const gchar     *resource_path);
CDK_AVAILABLE_IN_ALL
void ctk_image_set_from_pixbuf    (CtkImage        *image,
                                   CdkPixbuf       *pixbuf);
CDK_DEPRECATED_IN_3_10_FOR(ctk_image_set_from_icon_name)
void ctk_image_set_from_stock     (CtkImage        *image,
                                   const gchar     *stock_id,
                                   CtkIconSize      size);
CDK_DEPRECATED_IN_3_10_FOR(ctk_image_set_from_icon_name)
void ctk_image_set_from_icon_set  (CtkImage        *image,
                                   CtkIconSet      *icon_set,
                                   CtkIconSize      size);
CDK_AVAILABLE_IN_ALL
void ctk_image_set_from_animation (CtkImage           *image,
                                   CdkPixbufAnimation *animation);
CDK_AVAILABLE_IN_ALL
void ctk_image_set_from_icon_name (CtkImage        *image,
				   const gchar     *icon_name,
				   CtkIconSize      size);
CDK_AVAILABLE_IN_ALL
void ctk_image_set_from_gicon     (CtkImage        *image,
				   GIcon           *icon,
				   CtkIconSize      size);
CDK_AVAILABLE_IN_3_10
void ctk_image_set_from_surface   (CtkImage        *image,
				   cairo_surface_t *surface);
CDK_AVAILABLE_IN_ALL
void ctk_image_set_pixel_size     (CtkImage        *image,
				   gint             pixel_size);

CDK_AVAILABLE_IN_ALL
CtkImageType ctk_image_get_storage_type (CtkImage   *image);

CDK_AVAILABLE_IN_ALL
CdkPixbuf* ctk_image_get_pixbuf   (CtkImage         *image);
CDK_DEPRECATED_IN_3_10_FOR(ctk_image_get_icon_name)
void       ctk_image_get_stock    (CtkImage         *image,
                                   gchar           **stock_id,
                                   CtkIconSize      *size);
CDK_DEPRECATED_IN_3_10_FOR(ctk_image_get_icon_name)
void       ctk_image_get_icon_set (CtkImage         *image,
                                   CtkIconSet      **icon_set,
                                   CtkIconSize      *size);
CDK_AVAILABLE_IN_ALL
CdkPixbufAnimation* ctk_image_get_animation (CtkImage *image);
CDK_AVAILABLE_IN_ALL
void       ctk_image_get_icon_name (CtkImage     *image,
				    const gchar **icon_name,
				    CtkIconSize  *size);
CDK_AVAILABLE_IN_ALL
void       ctk_image_get_gicon     (CtkImage              *image,
				    GIcon                **gicon,
				    CtkIconSize           *size);
CDK_AVAILABLE_IN_ALL
gint       ctk_image_get_pixel_size (CtkImage             *image);

G_END_DECLS

#endif /* __CTK_IMAGE_H__ */
