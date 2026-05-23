/* CTK - The GIMP Toolkit
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
   CTK_TYPE_ICON_HELPER, CtkIconHelper))

#define CTK_ICON_HELPER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
   CTK_TYPE_ICON_HELPER, CtkIconHelperClass))

#define CTK_IS_ICON_HELPER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
   CTK_TYPE_ICON_HELPER))

#define CTK_IS_ICON_HELPER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
   CTK_TYPE_ICON_HELPER))

#define CTK_ICON_HELPER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
   CTK_TYPE_ICON_HELPER, CtkIconHelperClass))

typedef struct _CtkIconHelper CtkIconHelper;
typedef struct _CtkIconHelperClass CtkIconHelperClass;
typedef struct _CtkIconHelperPrivate CtkIconHelperPrivate;

struct _CtkIconHelper
{
  CtkCssGadget parent;

  CtkIconHelperPrivate *priv;
};

struct _CtkIconHelperClass
{
  CtkCssGadgetClass parent_class;
};

GType ctk_icon_helper_get_type (void) G_GNUC_CONST;

CtkIconHelper *ctk_icon_helper_new (CtkCssNode *node,
                                    CtkWidget  *owner);
CtkCssGadget *ctk_icon_helper_new_named (const char *name,
                                          CtkWidget  *owner);

void _ctk_icon_helper_clear (CtkIconHelper *self);

gboolean _ctk_icon_helper_get_is_empty (CtkIconHelper *self);

void _ctk_icon_helper_set_definition (CtkIconHelper *self,
                                      CtkImageDefinition *def);
void _ctk_icon_helper_set_gicon (CtkIconHelper *self,
                                 GIcon *gicon,
                                 CtkIconSize icon_size);
void _ctk_icon_helper_set_pixbuf (CtkIconHelper *self,
				  CdkPixbuf *pixbuf);
void _ctk_icon_helper_set_pixbuf_scale (CtkIconHelper *self,
					int scale);
void _ctk_icon_helper_set_animation (CtkIconHelper *self,
                                     CdkPixbufAnimation *animation);
void _ctk_icon_helper_set_icon_set (CtkIconHelper *self,
                                    CtkIconSet *icon_set,
                                    CtkIconSize icon_size);

void _ctk_icon_helper_set_icon_name (CtkIconHelper *self,
                                     const gchar *icon_name,
                                     CtkIconSize icon_size);
void _ctk_icon_helper_set_stock_id (CtkIconHelper *self,
                                    const gchar *stock_id,
                                    CtkIconSize icon_size);
void _ctk_icon_helper_set_surface (CtkIconHelper *self,
				   cairo_surface_t *surface);

gboolean _ctk_icon_helper_set_icon_size    (CtkIconHelper *self,
                                            CtkIconSize    icon_size);
gboolean _ctk_icon_helper_set_pixel_size   (CtkIconHelper *self,
                                            gint           pixel_size);
gboolean _ctk_icon_helper_set_use_fallback (CtkIconHelper *self,
                                            gboolean       use_fallback);

CtkImageType _ctk_icon_helper_get_storage_type (CtkIconHelper *self);
CtkIconSize _ctk_icon_helper_get_icon_size (CtkIconHelper *self);
gint _ctk_icon_helper_get_pixel_size (CtkIconHelper *self);
gboolean _ctk_icon_helper_get_use_fallback (CtkIconHelper *self);

CdkPixbuf *_ctk_icon_helper_peek_pixbuf (CtkIconHelper *self);
GIcon *_ctk_icon_helper_peek_gicon (CtkIconHelper *self);
CtkIconSet *_ctk_icon_helper_peek_icon_set (CtkIconHelper *self);
CdkPixbufAnimation *_ctk_icon_helper_peek_animation (CtkIconHelper *self);
cairo_surface_t *_ctk_icon_helper_peek_surface (CtkIconHelper *self);

CtkImageDefinition *ctk_icon_helper_get_definition (CtkIconHelper *self);
const gchar *_ctk_icon_helper_get_stock_id (CtkIconHelper *self);
const gchar *_ctk_icon_helper_get_icon_name (CtkIconHelper *self);

cairo_surface_t *ctk_icon_helper_load_surface (CtkIconHelper *self,
                                               int              scale);
void _ctk_icon_helper_get_size (CtkIconHelper *self,
                                gint *width_out,
                                gint *height_out);

void _ctk_icon_helper_draw (CtkIconHelper *self,
                            cairo_t *cr,
                            gdouble x,
                            gdouble y);

gboolean _ctk_icon_helper_get_force_scale_pixbuf (CtkIconHelper *self);
void     _ctk_icon_helper_set_force_scale_pixbuf (CtkIconHelper *self,
                                                  gboolean       force_scale);

void      ctk_icon_helper_invalidate_for_change (CtkIconHelper     *self,
                                                 CtkCssStyleChange *change);

G_END_DECLS

#endif /* __CTK_ICON_HELPER_H__ */
