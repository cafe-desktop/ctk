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

#include "config.h"

#include "ctkiconhelperprivate.h"

#include <math.h>

#include "ctkcssenumvalueprivate.h"
#include "ctkcssiconthemevalueprivate.h"
#include "ctkcssnodeprivate.h"
#include "ctkcssstyleprivate.h"
#include "ctkcssstylepropertyprivate.h"
#include "ctkcsstransientnodeprivate.h"
#include "ctkiconthemeprivate.h"
#include "ctkrendericonprivate.h"
#include "ctkiconfactoryprivate.h"
#include "ctkstock.h"

struct _CtkIconHelperPrivate {
  CtkImageDefinition *def;

  CtkIconSize icon_size;
  gint pixel_size;

  guint use_fallback : 1;
  guint force_scale_pixbuf : 1;
  guint rendered_surface_is_symbolic : 1;

  cairo_surface_t *rendered_surface;
};

G_DEFINE_TYPE_WITH_PRIVATE (CtkIconHelper, ctk_icon_helper, CTK_TYPE_CSS_GADGET)

static void
ctk_icon_helper_invalidate (CtkIconHelper *self)
{
  if (self->priv->rendered_surface != NULL)
    {
      cairo_surface_destroy (self->priv->rendered_surface);
      self->priv->rendered_surface = NULL;
      self->priv->rendered_surface_is_symbolic = FALSE;
    }

  if (!CTK_IS_CSS_TRANSIENT_NODE (ctk_css_gadget_get_node (CTK_CSS_GADGET (self))))
    ctk_widget_queue_resize (ctk_css_gadget_get_owner (CTK_CSS_GADGET (self)));
}

void
ctk_icon_helper_invalidate_for_change (CtkIconHelper     *self,
                                       CtkCssStyleChange *change)
{
  CtkIconHelperPrivate *priv = self->priv;

  if (change == NULL ||
      ((ctk_css_style_change_affects (change, CTK_CSS_AFFECTS_SYMBOLIC_ICON) &&
        priv->rendered_surface_is_symbolic) ||
       (ctk_css_style_change_affects (change, CTK_CSS_AFFECTS_ICON) &&
        !priv->rendered_surface_is_symbolic)))
    {
      ctk_icon_helper_invalidate (self);
    }
}

static void
ctk_icon_helper_take_definition (CtkIconHelper      *self,
                                 CtkImageDefinition *def)
{
  _ctk_icon_helper_clear (self);

  if (def == NULL)
    return;

  ctk_image_definition_unref (self->priv->def);
  self->priv->def = def;

  ctk_icon_helper_invalidate (self);
}

void
_ctk_icon_helper_clear (CtkIconHelper *self)
{
  g_clear_pointer (&self->priv->rendered_surface, cairo_surface_destroy);

  ctk_image_definition_unref (self->priv->def);
  self->priv->def = ctk_image_definition_new_empty ();

  self->priv->icon_size = CTK_ICON_SIZE_INVALID;

  ctk_icon_helper_invalidate (self);
}

static void
ctk_icon_helper_get_preferred_size (CtkCssGadget   *gadget,
                                    CtkOrientation  orientation,
                                    gint            for_size G_GNUC_UNUSED,
                                    gint           *minimum,
                                    gint           *natural,
                                    gint           *minimum_baseline G_GNUC_UNUSED,
                                    gint           *natural_baseline G_GNUC_UNUSED)
{
  CtkIconHelper *self = CTK_ICON_HELPER (gadget);
  int icon_width, icon_height;

  _ctk_icon_helper_get_size (self, &icon_width, &icon_height);

  if (orientation == CTK_ORIENTATION_HORIZONTAL)
    *minimum = *natural = icon_width;
  else
    *minimum = *natural = icon_height;
}

static void
ctk_icon_helper_allocate (CtkCssGadget        *gadget,
                          const CtkAllocation *allocation,
                          int                  baseline,
                          CtkAllocation       *out_clip)
{
  CTK_CSS_GADGET_CLASS (ctk_icon_helper_parent_class)->allocate (gadget, allocation, baseline, out_clip);
}

static gboolean
ctk_icon_helper_draw (CtkCssGadget *gadget,
                      cairo_t      *cr,
                      int           x,
                      int           y,
                      int           width,
                      int           height)
{
  CtkIconHelper *self = CTK_ICON_HELPER (gadget);
  int icon_width, icon_height;

  _ctk_icon_helper_get_size (self, &icon_width, &icon_height);
  _ctk_icon_helper_draw (self,
                         cr,
                         x + (width - icon_width) / 2,
                         y + (height - icon_height) / 2);

  return FALSE;
}

static void
ctk_icon_helper_style_changed (CtkCssGadget      *gadget,
                               CtkCssStyleChange *change)
{
  ctk_icon_helper_invalidate_for_change (CTK_ICON_HELPER (gadget), change);

  if (!CTK_IS_CSS_TRANSIENT_NODE (ctk_css_gadget_get_node (gadget)))
    CTK_CSS_GADGET_CLASS (ctk_icon_helper_parent_class)->style_changed (gadget, change);
}

static void
ctk_icon_helper_constructed (GObject *object)
{
  CtkIconHelper *self = CTK_ICON_HELPER (object);
  CtkWidget *widget;

  widget = ctk_css_gadget_get_owner (CTK_CSS_GADGET (self));

  g_signal_connect_swapped (widget, "direction-changed", G_CALLBACK (ctk_icon_helper_invalidate), self);
  g_signal_connect_swapped (widget, "notify::scale-factor", G_CALLBACK (ctk_icon_helper_invalidate), self);

  G_OBJECT_CLASS (ctk_icon_helper_parent_class)->constructed (object);
}

static void
ctk_icon_helper_finalize (GObject *object)
{
  CtkIconHelper *self = CTK_ICON_HELPER (object);
  CtkWidget *widget;

  widget = ctk_css_gadget_get_owner (CTK_CSS_GADGET (self));
  g_signal_handlers_disconnect_by_func (widget, G_CALLBACK (ctk_icon_helper_invalidate), self);

  _ctk_icon_helper_clear (self);
  ctk_image_definition_unref (self->priv->def);
  
  G_OBJECT_CLASS (ctk_icon_helper_parent_class)->finalize (object);
}

static void
ctk_icon_helper_class_init (CtkIconHelperClass *klass)
{
  CtkCssGadgetClass *gadget_class = CTK_CSS_GADGET_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  gadget_class->get_preferred_size = ctk_icon_helper_get_preferred_size;
  gadget_class->allocate = ctk_icon_helper_allocate;
  gadget_class->draw = ctk_icon_helper_draw;
  gadget_class->style_changed = ctk_icon_helper_style_changed;

  object_class->constructed = ctk_icon_helper_constructed;
  object_class->finalize = ctk_icon_helper_finalize;
}

static void
ctk_icon_helper_init (CtkIconHelper *self)
{
  self->priv = ctk_icon_helper_get_instance_private (self);

  self->priv->def = ctk_image_definition_new_empty ();

  self->priv->icon_size = CTK_ICON_SIZE_INVALID;
  self->priv->pixel_size = -1;
  self->priv->rendered_surface_is_symbolic = FALSE;
}

static void
ensure_icon_size (CtkIconHelper *self,
		  gint *width_out,
		  gint *height_out)
{
  gint width, height;

  if (self->priv->pixel_size != -1)
    {
      width = height = self->priv->pixel_size;
    }
  else if (!ctk_icon_size_lookup (self->priv->icon_size, &width, &height))
    {
      if (self->priv->icon_size == CTK_ICON_SIZE_INVALID)
        {
          width = height = 0;
        }
      else
        {
          g_warning ("Invalid icon size %d", self->priv->icon_size);
          width = height = 24;
        }
    }

  *width_out = width;
  *height_out = height;
}

static CtkIconLookupFlags
get_icon_lookup_flags (CtkIconHelper    *self,
                       CtkCssStyle      *style,
                       CtkTextDirection  dir)
{
  CtkIconLookupFlags flags;
  CtkCssIconStyle icon_style;

  flags = CTK_ICON_LOOKUP_USE_BUILTIN;

  if (self->priv->pixel_size != -1 || self->priv->force_scale_pixbuf)
    flags |= CTK_ICON_LOOKUP_FORCE_SIZE;

  icon_style = _ctk_css_icon_style_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_ICON_STYLE));

  switch (icon_style)
    {
    case CTK_CSS_ICON_STYLE_REGULAR:
      flags |= CTK_ICON_LOOKUP_FORCE_REGULAR;
      break;
    case CTK_CSS_ICON_STYLE_SYMBOLIC:
      flags |= CTK_ICON_LOOKUP_FORCE_SYMBOLIC;
      break;
    case CTK_CSS_ICON_STYLE_REQUESTED:
      break;
    default:
      g_assert_not_reached ();
      return 0;
    }

  if (dir == CTK_TEXT_DIR_LTR)
    flags |= CTK_ICON_LOOKUP_DIR_LTR;
  else if (dir == CTK_TEXT_DIR_RTL)
    flags |= CTK_ICON_LOOKUP_DIR_RTL;

  return flags;
}

static void
get_surface_size (CtkIconHelper   *self,
		  cairo_surface_t *surface,
		  int *width,
		  int *height)
{
  CdkRectangle clip;
  cairo_t *cr;

  cr = cairo_create (surface);
  if (cdk_cairo_get_clip_rectangle (cr, &clip))
    {
      if (clip.x != 0 || clip.y != 0)
        {
          g_warning ("origin of surface is %d %d, not supported", clip.x, clip.y);
        }
      *width = clip.width;
      *height = clip.height;
    }
  else
    {
      g_warning ("infinite surface size not supported");
      ensure_icon_size (self, width, height);
    }

  cairo_destroy (cr);
}

static cairo_surface_t *
ensure_surface_from_surface (CtkIconHelper   *self G_GNUC_UNUSED,
                             cairo_surface_t *orig_surface)
{
  return cairo_surface_reference (orig_surface);
}

static gboolean
get_pixbuf_size (CtkIconHelper   *self,
                 gint             scale,
                 GdkPixbuf       *orig_pixbuf,
                 gint             orig_scale,
                 gint *width_out,
                 gint *height_out,
                 gint *scale_out)
{
  gboolean scale_pixmap;
  gint width, height;

  scale_pixmap = FALSE;

  if (self->priv->force_scale_pixbuf &&
      (self->priv->pixel_size != -1 ||
       self->priv->icon_size != CTK_ICON_SIZE_INVALID))
    {
      ensure_icon_size (self, &width, &height);

      if (scale != orig_scale ||
	  width < gdk_pixbuf_get_width (orig_pixbuf) / orig_scale ||
          height < gdk_pixbuf_get_height (orig_pixbuf) / orig_scale)
	{
	  width = MIN (width * scale, gdk_pixbuf_get_width (orig_pixbuf) * scale / orig_scale);
	  height = MIN (height * scale, gdk_pixbuf_get_height (orig_pixbuf) * scale / orig_scale);

          scale_pixmap = TRUE;
	}
      else
	{
	  width = gdk_pixbuf_get_width (orig_pixbuf);
	  height = gdk_pixbuf_get_height (orig_pixbuf);
	  scale = orig_scale;
	}
    }
  else
    {
      width = gdk_pixbuf_get_width (orig_pixbuf);
      height = gdk_pixbuf_get_height (orig_pixbuf);
      scale = orig_scale;
    }

  *width_out = width;
  *height_out = height;
  *scale_out = scale;

  return scale_pixmap;
}

static cairo_surface_t *
ensure_surface_from_pixbuf (CtkIconHelper *self,
                            CtkCssStyle   *style,
                            gint           scale,
                            GdkPixbuf     *orig_pixbuf,
                            gint           orig_scale)
{
  gint width, height;
  cairo_surface_t *surface;
  GdkPixbuf *pixbuf;
  CtkCssIconEffect icon_effect;

  if (get_pixbuf_size (self,
                       scale,
                       orig_pixbuf,
                       orig_scale,
                       &width, &height, &scale))
    pixbuf = gdk_pixbuf_scale_simple (orig_pixbuf,
                                      width, height,
                                      GDK_INTERP_BILINEAR);
  else
    pixbuf = g_object_ref (orig_pixbuf);

  surface = cdk_cairo_surface_create_from_pixbuf (pixbuf, scale, ctk_widget_get_window (ctk_css_gadget_get_owner (CTK_CSS_GADGET (self))));
  icon_effect = _ctk_css_icon_effect_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_ICON_EFFECT));
  ctk_css_icon_effect_apply (icon_effect, surface);
  g_object_unref (pixbuf);

  return surface;
}

static cairo_surface_t *
ensure_surface_for_icon_set (CtkIconHelper    *self,
                             CtkCssStyle      *style,
                             CtkTextDirection  direction,
                             gint              scale,
			     CtkIconSet       *icon_set)
{
  cairo_surface_t *surface;
  GdkPixbuf *pixbuf;

  pixbuf = ctk_icon_set_render_icon_pixbuf_for_scale (icon_set,
                                                      style,
                                                      direction,
                                                      self->priv->icon_size,
                                                      scale);
  surface = cdk_cairo_surface_create_from_pixbuf (pixbuf,
                                                  scale,
                                                  ctk_widget_get_window (ctk_css_gadget_get_owner (CTK_CSS_GADGET (self))));
  g_object_unref (pixbuf);

  return surface;
}

static cairo_surface_t *
ensure_surface_for_gicon (CtkIconHelper    *self,
                          CtkCssStyle      *style,
                          CtkTextDirection  dir,
                          gint              scale,
                          GIcon            *gicon)
{
  CtkIconHelperPrivate *priv = self->priv;
  CtkIconTheme *icon_theme;
  gint width, height;
  CtkIconInfo *info;
  CtkIconLookupFlags flags;
  cairo_surface_t *surface;
  GdkPixbuf *destination;
  gboolean symbolic;

  icon_theme = ctk_css_icon_theme_value_get_icon_theme
    (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_ICON_THEME));
  flags = get_icon_lookup_flags (self, style, dir);

  ensure_icon_size (self, &width, &height);

  info = ctk_icon_theme_lookup_by_gicon_for_scale (icon_theme,
                                                   gicon,
                                                   MIN (width, height),
                                                   scale, flags);
  if (info)
    {
      symbolic = ctk_icon_info_is_symbolic (info);

      if (symbolic)
        {
          CdkRGBA fg, success_color, warning_color, error_color;

          ctk_icon_theme_lookup_symbolic_colors (style, &fg, &success_color, &warning_color, &error_color);

          destination = ctk_icon_info_load_symbolic (info,
                                                     &fg, &success_color,
                                                     &warning_color, &error_color,
                                                     NULL,
                                                     NULL);
        }
      else
        {
          destination = ctk_icon_info_load_icon (info, NULL);
        }

      g_object_unref (info);
    }
  else
    {
      destination = NULL;
    }

  if (destination == NULL)
    {
      GError *error = NULL;
      destination = ctk_icon_theme_load_icon (icon_theme,
                                              "image-missing",
                                              width,
                                              flags | CTK_ICON_LOOKUP_USE_BUILTIN | CTK_ICON_LOOKUP_GENERIC_FALLBACK,
                                              &error);
      /* We include this image as resource, so we always have it available or
       * the icontheme code is broken */
      g_assert_no_error (error);
      g_assert (destination);
      symbolic = FALSE;
    }

  surface = cdk_cairo_surface_create_from_pixbuf (destination, scale, ctk_widget_get_window (ctk_css_gadget_get_owner (CTK_CSS_GADGET (self))));

  if (!symbolic)
    {
      CtkCssIconEffect icon_effect;

      icon_effect = _ctk_css_icon_effect_value_get (ctk_css_style_get_value (style, CTK_CSS_PROPERTY_ICON_EFFECT));
      ctk_css_icon_effect_apply (icon_effect, surface);
    }
  else
    {
      priv->rendered_surface_is_symbolic = TRUE;
    }

  g_object_unref (destination);

  return surface;
}

cairo_surface_t *
ctk_icon_helper_load_surface (CtkIconHelper   *self,
                              int              scale)
{
  cairo_surface_t *surface;
  CtkIconSet *icon_set;
  GIcon *gicon;

  switch (ctk_image_definition_get_storage_type (self->priv->def))
    {
    case CTK_IMAGE_SURFACE:
      surface = ensure_surface_from_surface (self, ctk_image_definition_get_surface (self->priv->def));
      break;

    case CTK_IMAGE_PIXBUF:
      surface = ensure_surface_from_pixbuf (self,
                                            ctk_css_node_get_style (ctk_css_gadget_get_node (CTK_CSS_GADGET (self))),
                                            scale,
                                            ctk_image_definition_get_pixbuf (self->priv->def),
                                            ctk_image_definition_get_scale (self->priv->def));
      break;

    case CTK_IMAGE_STOCK:
      icon_set = ctk_icon_factory_lookup_default (ctk_image_definition_get_stock (self->priv->def));
      if (icon_set != NULL)
	surface = ensure_surface_for_icon_set (self,
                                               ctk_css_node_get_style (ctk_css_gadget_get_node (CTK_CSS_GADGET (self))),
                                               ctk_widget_get_direction (ctk_css_gadget_get_owner (CTK_CSS_GADGET (self))), 
                                               scale, icon_set);
      else
	surface = NULL;
      break;

    case CTK_IMAGE_ICON_SET:
      icon_set = ctk_image_definition_get_icon_set (self->priv->def);
      surface = ensure_surface_for_icon_set (self,
                                             ctk_css_node_get_style (ctk_css_gadget_get_node (CTK_CSS_GADGET (self))),
                                             ctk_widget_get_direction (ctk_css_gadget_get_owner (CTK_CSS_GADGET (self))), 
                                             scale, icon_set);
      break;

    case CTK_IMAGE_ICON_NAME:
      if (self->priv->use_fallback)
        gicon = g_themed_icon_new_with_default_fallbacks (ctk_image_definition_get_icon_name (self->priv->def));
      else
        gicon = g_themed_icon_new (ctk_image_definition_get_icon_name (self->priv->def));
      surface = ensure_surface_for_gicon (self,
                                          ctk_css_node_get_style (ctk_css_gadget_get_node (CTK_CSS_GADGET (self))),
                                          ctk_widget_get_direction (ctk_css_gadget_get_owner (CTK_CSS_GADGET (self))), 
                                          scale, 
                                          gicon);
      g_object_unref (gicon);
      break;

    case CTK_IMAGE_GICON:
      surface = ensure_surface_for_gicon (self, 
                                          ctk_css_node_get_style (ctk_css_gadget_get_node (CTK_CSS_GADGET (self))),
                                          ctk_widget_get_direction (ctk_css_gadget_get_owner (CTK_CSS_GADGET (self))), 
                                          scale,
                                          ctk_image_definition_get_gicon (self->priv->def));
      break;

    case CTK_IMAGE_ANIMATION:
    case CTK_IMAGE_EMPTY:
    default:
      surface = NULL;
      break;
    }

  return surface;

}

static void
ctk_icon_helper_ensure_surface (CtkIconHelper *self)
{
  int scale;

  if (self->priv->rendered_surface)
    return;

  scale = ctk_widget_get_scale_factor (ctk_css_gadget_get_owner (CTK_CSS_GADGET (self)));

  self->priv->rendered_surface = ctk_icon_helper_load_surface (self, scale);
}

void
_ctk_icon_helper_get_size (CtkIconHelper *self,
                           gint *width_out,
                           gint *height_out)
{
  gint width, height, scale;

  width = height = 0;

  /* Certain kinds of images are easy to calculate the size for, these
     we do immediately to avoid having to potentially load the image
     data for something that may not yet be visible */
  switch (ctk_image_definition_get_storage_type (self->priv->def))
    {
    case CTK_IMAGE_SURFACE:
      get_surface_size (self,
                        ctk_image_definition_get_surface (self->priv->def),
                        &width,
                        &height);
      break;

    case CTK_IMAGE_PIXBUF:
      get_pixbuf_size (self,
                       ctk_widget_get_scale_factor (ctk_css_gadget_get_owner (CTK_CSS_GADGET (self))),
                       ctk_image_definition_get_pixbuf (self->priv->def),
                       ctk_image_definition_get_scale (self->priv->def),
                       &width, &height, &scale);
      width = (width + scale - 1) / scale;
      height = (height + scale - 1) / scale;
      break;

    case CTK_IMAGE_ANIMATION:
      {
        GdkPixbufAnimation *animation = ctk_image_definition_get_animation (self->priv->def);
        width = gdk_pixbuf_animation_get_width (animation);
        height = gdk_pixbuf_animation_get_height (animation);
        break;
      }

    case CTK_IMAGE_ICON_NAME:
    case CTK_IMAGE_GICON:
      if (self->priv->pixel_size != -1 || self->priv->force_scale_pixbuf)
        ensure_icon_size (self, &width, &height);

      break;

    case CTK_IMAGE_STOCK:
    case CTK_IMAGE_ICON_SET:
    case CTK_IMAGE_EMPTY:
    default:
      break;
    }

  /* Otherwise we load the surface to guarantee we get a size */
  if (width == 0)
    {
      ctk_icon_helper_ensure_surface (self);

      if (self->priv->rendered_surface != NULL)
        {
          get_surface_size (self, self->priv->rendered_surface, &width, &height);
        }
      else if (self->priv->icon_size != CTK_ICON_SIZE_INVALID)
        {
          ensure_icon_size (self, &width, &height);
        }
    }

  if (width_out)
    *width_out = width;
  if (height_out)
    *height_out = height;
}

void
_ctk_icon_helper_set_definition (CtkIconHelper *self,
                                 CtkImageDefinition *def)
{
  if (def)
    ctk_icon_helper_take_definition (self, ctk_image_definition_ref (def));
  else
    _ctk_icon_helper_clear (self);
}

void 
_ctk_icon_helper_set_gicon (CtkIconHelper *self,
                            GIcon *gicon,
                            CtkIconSize icon_size)
{
  ctk_icon_helper_take_definition (self, ctk_image_definition_new_gicon (gicon));
  _ctk_icon_helper_set_icon_size (self, icon_size);
}

void 
_ctk_icon_helper_set_icon_name (CtkIconHelper *self,
                                const gchar *icon_name,
                                CtkIconSize icon_size)
{
  ctk_icon_helper_take_definition (self, ctk_image_definition_new_icon_name (icon_name));
  _ctk_icon_helper_set_icon_size (self, icon_size);
}

void 
_ctk_icon_helper_set_icon_set (CtkIconHelper *self,
                               CtkIconSet *icon_set,
                               CtkIconSize icon_size)
{
  ctk_icon_helper_take_definition (self, ctk_image_definition_new_icon_set (icon_set));
  _ctk_icon_helper_set_icon_size (self, icon_size);
}

void 
_ctk_icon_helper_set_pixbuf (CtkIconHelper *self,
                             GdkPixbuf *pixbuf)
{
  ctk_icon_helper_take_definition (self, ctk_image_definition_new_pixbuf (pixbuf, 1));
}

void 
_ctk_icon_helper_set_animation (CtkIconHelper *self,
                                GdkPixbufAnimation *animation)
{
  ctk_icon_helper_take_definition (self, ctk_image_definition_new_animation (animation, 1));
}

void 
_ctk_icon_helper_set_surface (CtkIconHelper *self,
			      cairo_surface_t *surface)
{
  ctk_icon_helper_take_definition (self, ctk_image_definition_new_surface (surface));
}

void 
_ctk_icon_helper_set_stock_id (CtkIconHelper *self,
                               const gchar *stock_id,
                               CtkIconSize icon_size)
{
  ctk_icon_helper_take_definition (self, ctk_image_definition_new_stock (stock_id));
  _ctk_icon_helper_set_icon_size (self, icon_size);
}

gboolean
_ctk_icon_helper_set_icon_size (CtkIconHelper *self,
                                CtkIconSize    icon_size)
{
  if (self->priv->icon_size != icon_size)
    {
      self->priv->icon_size = icon_size;
      ctk_icon_helper_invalidate (self);
      return TRUE;
    }
  return FALSE;
}

gboolean
_ctk_icon_helper_set_pixel_size (CtkIconHelper *self,
                                 gint           pixel_size)
{
  if (self->priv->pixel_size != pixel_size)
    {
      self->priv->pixel_size = pixel_size;
      ctk_icon_helper_invalidate (self);
      return TRUE;
    }
  return FALSE;
}

gboolean
_ctk_icon_helper_set_use_fallback (CtkIconHelper *self,
                                   gboolean       use_fallback)
{
  if (self->priv->use_fallback != use_fallback)
    {
      self->priv->use_fallback = use_fallback;
      ctk_icon_helper_invalidate (self);
      return TRUE;
    }
  return FALSE;
}

CtkImageType
_ctk_icon_helper_get_storage_type (CtkIconHelper *self)
{
  return ctk_image_definition_get_storage_type (self->priv->def);
}

gboolean
_ctk_icon_helper_get_use_fallback (CtkIconHelper *self)
{
  return self->priv->use_fallback;
}

CtkIconSize
_ctk_icon_helper_get_icon_size (CtkIconHelper *self)
{
  return self->priv->icon_size;
}

gint
_ctk_icon_helper_get_pixel_size (CtkIconHelper *self)
{
  return self->priv->pixel_size;
}

CtkImageDefinition *
ctk_icon_helper_get_definition (CtkIconHelper *self)
{
  return self->priv->def;
}

GdkPixbuf *
_ctk_icon_helper_peek_pixbuf (CtkIconHelper *self)
{
  return ctk_image_definition_get_pixbuf (self->priv->def);
}

GIcon *
_ctk_icon_helper_peek_gicon (CtkIconHelper *self)
{
  return ctk_image_definition_get_gicon (self->priv->def);
}

GdkPixbufAnimation *
_ctk_icon_helper_peek_animation (CtkIconHelper *self)
{
  return ctk_image_definition_get_animation (self->priv->def);
}

CtkIconSet *
_ctk_icon_helper_peek_icon_set (CtkIconHelper *self)
{
  return ctk_image_definition_get_icon_set (self->priv->def);
}

cairo_surface_t *
_ctk_icon_helper_peek_surface (CtkIconHelper *self)
{
  return ctk_image_definition_get_surface (self->priv->def);
}

const gchar *
_ctk_icon_helper_get_stock_id (CtkIconHelper *self)
{
  return ctk_image_definition_get_stock (self->priv->def);
}

const gchar *
_ctk_icon_helper_get_icon_name (CtkIconHelper *self)
{
  return ctk_image_definition_get_icon_name (self->priv->def);
}

CtkIconHelper *
ctk_icon_helper_new (CtkCssNode *node,
                     CtkWidget  *owner)
{
  g_return_val_if_fail (CTK_IS_CSS_NODE (node), NULL);
  g_return_val_if_fail (CTK_IS_WIDGET (owner), NULL);

  return g_object_new (CTK_TYPE_ICON_HELPER,
                       "node", node,
                       "owner", owner,
                       NULL);
}

CtkCssGadget *
ctk_icon_helper_new_named (const char *name,
                           CtkWidget  *owner)
{
  CtkIconHelper *result;
  CtkCssNode *node;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (CTK_IS_WIDGET (owner), NULL);

  node = ctk_css_node_new ();
  ctk_css_node_set_name (node, g_intern_string (name));

  result = ctk_icon_helper_new (node, owner);

  g_object_unref (node);

  return CTK_CSS_GADGET (result);
}

void
_ctk_icon_helper_draw (CtkIconHelper *self,
                       cairo_t *cr,
                       gdouble x,
                       gdouble y)
{
  CtkCssStyle *style = ctk_css_node_get_style (ctk_css_gadget_get_node (CTK_CSS_GADGET (self)));
  ctk_icon_helper_ensure_surface (self);

  if (self->priv->rendered_surface != NULL)
    {
      ctk_css_style_render_icon_surface (style,
                                         cr,
                                         self->priv->rendered_surface,
                                         x, y);
    }
}

gboolean
_ctk_icon_helper_get_is_empty (CtkIconHelper *self)
{
  return ctk_image_definition_get_storage_type (self->priv->def) == CTK_IMAGE_EMPTY;
}

gboolean
_ctk_icon_helper_get_force_scale_pixbuf (CtkIconHelper *self)
{
  return self->priv->force_scale_pixbuf;
}

void
_ctk_icon_helper_set_force_scale_pixbuf (CtkIconHelper *self,
                                         gboolean       force_scale)
{
  if (self->priv->force_scale_pixbuf != force_scale)
    {
      self->priv->force_scale_pixbuf = force_scale;
      ctk_icon_helper_invalidate (self);
    }
}

void 
_ctk_icon_helper_set_pixbuf_scale (CtkIconHelper *self,
				   int scale)
{
  switch (ctk_image_definition_get_storage_type (self->priv->def))
  {
    case CTK_IMAGE_PIXBUF:
      ctk_icon_helper_take_definition (self,
                                      ctk_image_definition_new_pixbuf (ctk_image_definition_get_pixbuf (self->priv->def),
                                                                       scale));
      break;

    case CTK_IMAGE_ANIMATION:
      ctk_icon_helper_take_definition (self,
                                      ctk_image_definition_new_animation (ctk_image_definition_get_animation (self->priv->def),
                                                                          scale));
      break;

    default:
      break;
  }
}
