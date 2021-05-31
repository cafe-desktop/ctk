/* GTK - The GIMP Toolkit
 * Copyright (C) 2011 Red Hat, Inc.
 *
 * Authors: Cosimo Cecchi <cosimoc@gnome.org>
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

#ifndef __CTK_ICON_HELPER_H__
#define __CTK_ICON_HELPER_H__

#include "ctk/ctkimage.h"
#include "ctk/ctktypes.h"

#include "ctkcssgadgetprivate.h"
#include "ctkimagedefinitionprivate.h"

G_BEGIN_DECLS

#define CTK_TYPE_ICON_HELPER ctk_icon_helper_get_type()

#define CTK_ICON_HELPER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
   CTK_TYPE_ICON_HELPER, GtkIconHelper))

#define CTK_ICON_HELPER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
   CTK_TYPE_ICON_HELPER, GtkIconHelperClass))

#define CTK_IS_ICON_HELPER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
   CTK_TYPE_ICON_HELPER))

#define CTK_IS_ICON_HELPER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
   CTK_TYPE_ICON_HELPER))

#define CTK_ICON_HELPER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
   CTK_TYPE_ICON_HELPER, GtkIconHelperClass))

typedef struct _GtkIconHelper GtkIconHelper;
typedef struct _GtkIconHelperClass GtkIconHelperClass;
typedef struct _GtkIconHelperPrivate GtkIconHelperPrivate;

struct _GtkIconHelper
{
  GtkCssGadget parent;

  GtkIconHelperPrivate *priv;
};

struct _GtkIconHelperClass
{
  GtkCssGadgetClass parent_class;
};

GType ctk_icon_helper_get_type (void) G_GNUC_CONST;

GtkIconHelper *ctk_icon_helper_new (GtkCssNode *node,
                                    GtkWidget  *owner);
GtkCssGadget *ctk_icon_helper_new_named (const char *name,
                                          GtkWidget  *owner);

void _ctk_icon_helper_clear (GtkIconHelper *self);

gboolean _ctk_icon_helper_get_is_empty (GtkIconHelper *self);

void _ctk_icon_helper_set_definition (GtkIconHelper *self,
                                      GtkImageDefinition *def);
void _ctk_icon_helper_set_gicon (GtkIconHelper *self,
                                 GIcon *gicon,
                                 GtkIconSize icon_size);
void _ctk_icon_helper_set_pixbuf (GtkIconHelper *self,
				  GdkPixbuf *pixbuf);
void _ctk_icon_helper_set_pixbuf_scale (GtkIconHelper *self,
					int scale);
void _ctk_icon_helper_set_animation (GtkIconHelper *self,
                                     GdkPixbufAnimation *animation);
void _ctk_icon_helper_set_icon_set (GtkIconHelper *self,
                                    GtkIconSet *icon_set,
                                    GtkIconSize icon_size);

void _ctk_icon_helper_set_icon_name (GtkIconHelper *self,
                                     const gchar *icon_name,
                                     GtkIconSize icon_size);
void _ctk_icon_helper_set_stock_id (GtkIconHelper *self,
                                    const gchar *stock_id,
                                    GtkIconSize icon_size);
void _ctk_icon_helper_set_surface (GtkIconHelper *self,
				   cairo_surface_t *surface);

gboolean _ctk_icon_helper_set_icon_size    (GtkIconHelper *self,
                                            GtkIconSize    icon_size);
gboolean _ctk_icon_helper_set_pixel_size   (GtkIconHelper *self,
                                            gint           pixel_size);
gboolean _ctk_icon_helper_set_use_fallback (GtkIconHelper *self,
                                            gboolean       use_fallback);

GtkImageType _ctk_icon_helper_get_storage_type (GtkIconHelper *self);
GtkIconSize _ctk_icon_helper_get_icon_size (GtkIconHelper *self);
gint _ctk_icon_helper_get_pixel_size (GtkIconHelper *self);
gboolean _ctk_icon_helper_get_use_fallback (GtkIconHelper *self);

GdkPixbuf *_ctk_icon_helper_peek_pixbuf (GtkIconHelper *self);
GIcon *_ctk_icon_helper_peek_gicon (GtkIconHelper *self);
GtkIconSet *_ctk_icon_helper_peek_icon_set (GtkIconHelper *self);
GdkPixbufAnimation *_ctk_icon_helper_peek_animation (GtkIconHelper *self);
cairo_surface_t *_ctk_icon_helper_peek_surface (GtkIconHelper *self);

GtkImageDefinition *ctk_icon_helper_get_definition (GtkIconHelper *self);
const gchar *_ctk_icon_helper_get_stock_id (GtkIconHelper *self);
const gchar *_ctk_icon_helper_get_icon_name (GtkIconHelper *self);

cairo_surface_t *ctk_icon_helper_load_surface (GtkIconHelper *self,
                                               int              scale);
void _ctk_icon_helper_get_size (GtkIconHelper *self,
                                gint *width_out,
                                gint *height_out);

void _ctk_icon_helper_draw (GtkIconHelper *self,
                            cairo_t *cr,
                            gdouble x,
                            gdouble y);

gboolean _ctk_icon_helper_get_force_scale_pixbuf (GtkIconHelper *self);
void     _ctk_icon_helper_set_force_scale_pixbuf (GtkIconHelper *self,
                                                  gboolean       force_scale);

void      ctk_icon_helper_invalidate_for_change (GtkIconHelper     *self,
                                                 GtkCssStyleChange *change);

G_END_DECLS

#endif /* __CTK_ICON_HELPER_H__ */
