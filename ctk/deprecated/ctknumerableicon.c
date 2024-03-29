/*
 * ctknumerableicon.c: an emblemed icon with number emblems
 *
 * Copyright (C) 2010 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Cosimo Cecchi <cosimoc@redhat.com>
 */

/**
 * SECTION:ctknumerableicon
 * @Title: CtkNumerableIcon
 * @Short_description: A GIcon that allows numbered emblems
 *
 * CtkNumerableIcon is a subclass of #GEmblemedIcon that can
 * show a number or short string as an emblem. The number can
 * be overlayed on top of another emblem, if desired.
 *
 * It supports theming by taking font and color information
 * from a provided #CtkStyleContext; see
 * ctk_numerable_icon_set_style_context().
 *
 * Typical numerable icons:
 * ![](numerableicon.png)
 * ![](numerableicon2.png)
 */
#include <config.h>

#include "ctknumerableicon.h"
#include "ctknumerableiconprivate.h"

#include "ctkcssiconthemevalueprivate.h"
#include "ctkicontheme.h"
#include "ctkintl.h"
#include "ctkstylepropertyprivate.h"
#include "ctkwidget.h"
#include "ctkwidgetpath.h"
#include "ctkwindow.h"

#include <cdk/cdk.h>
#include <pango/pango.h>
#include <math.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

struct _CtkNumerableIconPrivate {
  gint count;
  gint icon_size;

  gchar *label;

  GIcon *background_icon;
  gchar *background_icon_name;

  CdkRGBA *background;
  CdkRGBA *foreground;

  PangoFontDescription *font;
  cairo_pattern_t *background_image;
  gint border_size;

  CtkStyleContext *style;
  gulong style_changed_id;

  gchar *rendered_string;
};

enum {
  PROP_COUNT = 1,
  PROP_LABEL,
  PROP_STYLE,
  PROP_BACKGROUND_ICON,
  PROP_BACKGROUND_ICON_NAME,
  NUM_PROPERTIES
};

#define DEFAULT_SURFACE_SIZE 256
#define DEFAULT_BORDER_SIZE DEFAULT_SURFACE_SIZE * 0.06
#define DEFAULT_RADIUS DEFAULT_SURFACE_SIZE / 2

#define DEFAULT_BACKGROUND "#000000"
#define DEFAULT_FOREGROUND "#ffffff"

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };

G_DEFINE_TYPE_WITH_PRIVATE (CtkNumerableIcon, ctk_numerable_icon, G_TYPE_EMBLEMED_ICON)

static gint
get_surface_size (cairo_surface_t *surface)
{
  return MAX (cairo_image_surface_get_width (surface), cairo_image_surface_get_height (surface));
}

static gdouble
get_border_size (CtkNumerableIcon *self)
{
  return self->priv->border_size;
}

static cairo_surface_t *
draw_default_surface (CtkNumerableIcon *self)
{
  cairo_surface_t *surface;
  cairo_t *cr;

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                        DEFAULT_SURFACE_SIZE, DEFAULT_SURFACE_SIZE);

  cr = cairo_create (surface);

  cairo_arc (cr, DEFAULT_SURFACE_SIZE / 2., DEFAULT_SURFACE_SIZE / 2.,
             DEFAULT_RADIUS, 0., 2 * G_PI);

  cdk_cairo_set_source_rgba (cr, self->priv->background);
  cairo_fill (cr);

  cairo_arc (cr, DEFAULT_SURFACE_SIZE / 2., DEFAULT_SURFACE_SIZE / 2.,
             DEFAULT_RADIUS - DEFAULT_BORDER_SIZE, 0., 2 * G_PI);
  cdk_cairo_set_source_rgba  (cr, self->priv->foreground);
  cairo_fill (cr);

  cairo_arc (cr, DEFAULT_SURFACE_SIZE / 2., DEFAULT_SURFACE_SIZE / 2.,
             DEFAULT_RADIUS - 2 * DEFAULT_BORDER_SIZE, 0., 2 * G_PI);
  cdk_cairo_set_source_rgba  (cr, self->priv->background);
  cairo_fill (cr);

  cairo_destroy (cr);

  return surface;
}

static cairo_surface_t *
draw_from_gradient (cairo_pattern_t *pattern)
{
  cairo_surface_t *surface;
  cairo_matrix_t matrix;
  cairo_t *cr;

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                        DEFAULT_SURFACE_SIZE, DEFAULT_SURFACE_SIZE);

  cr = cairo_create (surface);

  /* scale the gradient points to the user space coordinates */
  cairo_matrix_init_scale (&matrix,
                           1. / (double) DEFAULT_SURFACE_SIZE,
                           1. / (double) DEFAULT_SURFACE_SIZE);
  cairo_pattern_set_matrix (pattern, &matrix);

  cairo_arc (cr, DEFAULT_SURFACE_SIZE / 2., DEFAULT_SURFACE_SIZE / 2.,
             DEFAULT_RADIUS, 0., 2 * G_PI);

  cairo_set_source (cr, pattern);
  cairo_fill (cr);

  cairo_destroy (cr);

  return surface;
}

/* copy the surface */
static cairo_surface_t *
draw_from_image (cairo_surface_t *image)
{
  cairo_surface_t *surface;
  cairo_t *cr;

  surface = cairo_surface_create_similar (image, CAIRO_CONTENT_COLOR_ALPHA,
                                          cairo_image_surface_get_width (image),
                                          cairo_image_surface_get_height (image));
  cr = cairo_create (surface);

  cairo_set_source_surface (cr, image, 0, 0);
  cairo_paint (cr);

  cairo_destroy (cr);

  return surface;
}

static cairo_surface_t *
draw_from_gicon (CtkNumerableIcon *self)
{
  CtkIconTheme *theme;
  CtkIconInfo *info;
  GdkPixbuf *pixbuf;
  cairo_surface_t *surface;

  if (self->priv->style != NULL)
    {
      theme = ctk_css_icon_theme_value_get_icon_theme
          (_ctk_style_context_peek_property (self->priv->style, CTK_CSS_PROPERTY_ICON_THEME));
    }
  else
    {
      theme = ctk_icon_theme_get_default ();
    }

  info = ctk_icon_theme_lookup_by_gicon (theme, self->priv->background_icon,
                                         self->priv->icon_size,
                                         CTK_ICON_LOOKUP_GENERIC_FALLBACK);
  if (info == NULL)
    return NULL;

  pixbuf = ctk_icon_info_load_icon (info, NULL);
  g_object_unref (info);

  if (pixbuf == NULL)
    return NULL;

  surface = cdk_cairo_surface_create_from_pixbuf (pixbuf, 1, NULL);
  g_object_unref (pixbuf);

  return surface;
}

static cairo_surface_t *
get_image_surface (CtkNumerableIcon *self)
{
  cairo_surface_t *retval = NULL, *image;

  if (self->priv->background_icon != NULL)
    {
      retval = draw_from_gicon (self);
      self->priv->border_size = 0;
    }
  else if (self->priv->background_image != NULL)
    {
      if (cairo_pattern_get_surface (self->priv->background_image, &image) == CAIRO_STATUS_SUCCESS)
        retval = draw_from_image (image);
      else
        retval = draw_from_gradient (self->priv->background_image);

      self->priv->border_size = 0;
    }

  if (retval == NULL)
    {
      retval = draw_default_surface (self);
      self->priv->border_size = DEFAULT_BORDER_SIZE;
    }

  return retval;
}

static PangoLayout *
get_pango_layout (CtkNumerableIcon *self)
{
  PangoLayout *layout;

  if (self->priv->style != NULL)
    {
      PangoContext *context;
      CdkScreen *screen;

      screen = ctk_style_context_get_screen (self->priv->style);
      context = cdk_pango_context_get_for_screen (screen);
      layout = pango_layout_new (context);

      if (self->priv->font != NULL)
        pango_layout_set_font_description (layout, self->priv->font);

      pango_layout_set_text (layout, self->priv->rendered_string, -1);

      g_object_unref (context);
    }
  else
    {
      CtkWidget *fake;

      /* steal ctk text settings from the window */
      fake = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      layout = ctk_widget_create_pango_layout (fake, self->priv->rendered_string);
      ctk_widget_destroy (fake);
    }

  return layout;
}

static void
ctk_numerable_icon_ensure_emblem (CtkNumerableIcon *self)
{
  cairo_t *cr;
  cairo_surface_t *surface;
  PangoLayout *layout;
  GEmblem *emblem;
  gint width, height;
  gdouble scale;
  PangoAttrList *attr_list;
  PangoAttribute *attr;
  GdkPixbuf *pixbuf;

  /* don't draw anything if the count is zero */
  if (self->priv->rendered_string == NULL)
    {
      g_emblemed_icon_clear_emblems (G_EMBLEMED_ICON (self));
      return;
    }

  surface = get_image_surface (self);
  cr = cairo_create (surface);

  layout = get_pango_layout (self);
  pango_layout_get_pixel_size (layout, &width, &height);

  /* scale the layout to be 0.75 of the size still available for drawing */
  scale = ((get_surface_size (surface) - 2 * get_border_size (self)) * 0.75) / (MAX (height, width));
  attr_list = pango_attr_list_new ();

  attr = pango_attr_scale_new (scale);
  pango_attr_list_insert (attr_list, attr);

  attr = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
  pango_attr_list_insert (attr_list, attr);

  pango_layout_set_attributes (layout, attr_list);

  /* update these values */
  pango_layout_get_pixel_size (layout, &width, &height);

  /* move to the center */
  cairo_move_to (cr,
                 get_surface_size (surface) / 2. - (gdouble) width / 2.,
                 get_surface_size (surface) / 2. - (gdouble) height / 2.);

  cdk_cairo_set_source_rgba (cr, self->priv->foreground);
  pango_cairo_show_layout (cr, layout);

  cairo_destroy (cr);

  pixbuf =
    gdk_pixbuf_get_from_surface (surface, 0, 0,
                                 get_surface_size (surface), get_surface_size (surface));

  emblem = g_emblem_new (G_ICON (pixbuf));
  g_emblemed_icon_clear_emblems (G_EMBLEMED_ICON (self));
  g_emblemed_icon_add_emblem (G_EMBLEMED_ICON (self), emblem);

  g_object_unref (layout);
  g_object_unref (emblem);
  g_object_unref (pixbuf);

  cairo_surface_destroy (surface);
  pango_attr_list_unref (attr_list);
}

static void
ctk_numerable_icon_update_properties_from_style (CtkNumerableIcon *self)
{
  CtkStyleContext *style = self->priv->style;
  CtkWidgetPath *path, *saved;
  cairo_pattern_t *pattern = NULL;
  CdkRGBA background, foreground;
  PangoFontDescription *font = NULL;

  /* save an unmodified copy of the original widget path, in order
   * to restore it later */
  path = ctk_widget_path_copy (ctk_style_context_get_path (style));
  saved = ctk_widget_path_copy (path);

  if (!ctk_widget_path_is_type (path, CTK_TYPE_NUMERABLE_ICON))
    {
      /* append our GType to the style context to fetch appropriate colors */
      ctk_widget_path_append_type (path, CTK_TYPE_NUMERABLE_ICON);
      ctk_style_context_set_path (style, path);
    }

  ctk_style_context_get_background_color (style, ctk_style_context_get_state (style),
                                          &background);
  ctk_style_context_get_color (style, ctk_style_context_get_state (style),
                               &foreground);

  if (self->priv->background != NULL)
    cdk_rgba_free (self->priv->background);

  self->priv->background = cdk_rgba_copy (&background);

  if (self->priv->foreground != NULL)
    cdk_rgba_free (self->priv->foreground);

  self->priv->foreground = cdk_rgba_copy (&foreground);

  ctk_style_context_get (style, ctk_style_context_get_state (style),
                         CTK_STYLE_PROPERTY_BACKGROUND_IMAGE, &pattern,
                         NULL);

  if (pattern != NULL)
    {
      if (self->priv->background_image != NULL)
        cairo_pattern_destroy (self->priv->background_image);

      self->priv->background_image = pattern;
    }

  ctk_style_context_get (style, ctk_style_context_get_state (style),
                         CTK_STYLE_PROPERTY_FONT, &font,
                         NULL);

  if (font != NULL)
    {
      if (self->priv->font != NULL)
        pango_font_description_free (self->priv->font);

      self->priv->font = font;
    }

  ctk_numerable_icon_ensure_emblem (self);

  /* restore original widget path */
  ctk_style_context_set_path (style, saved);

  ctk_widget_path_free (path);
  ctk_widget_path_free (saved);
}

static void
ctk_numerable_icon_init_style (CtkNumerableIcon *self)
{
  CtkStyleContext *style = self->priv->style;

  if (style == NULL)
    return;

  ctk_numerable_icon_update_properties_from_style (self);

  self->priv->style_changed_id =
    g_signal_connect_swapped (style, "changed",
                              G_CALLBACK (ctk_numerable_icon_update_properties_from_style), self);
}

static void
ctk_numerable_icon_ensure_and_replace_label (CtkNumerableIcon *self,
                                             gint              count,
                                             const gchar      *label)
{
  g_assert (!(label != NULL && count != 0));

  g_free (self->priv->rendered_string);
  self->priv->rendered_string = NULL;

  if (count != 0)
    {
      if (self->priv->label != NULL)
        {
          g_free (self->priv->label);
          self->priv->label = NULL;

          g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_LABEL]);
        }

      if (count > 99)
        count = 99;

      if (count < -99)
        count = -99;

      self->priv->count = count;

      /* Translators: the format here is used to build the string that will be rendered
       * in the number emblem.
       */
      self->priv->rendered_string = g_strdup_printf (C_("Number format", "%d"), count);

      return;
    }

  if (label != NULL)
    {
      if (self->priv->count != 0)
        {
          self->priv->count = 0;

          g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_COUNT]);
        }

      g_free (self->priv->label);

      if (g_strcmp0 (label, "") == 0)
        {
          self->priv->label = NULL;
          return;
        }

      self->priv->label = g_strdup (label);
      self->priv->rendered_string = g_strdup (label);
    }
}

static gboolean
real_set_background_icon (CtkNumerableIcon *self,
                          GIcon            *icon)
{
  if (!g_icon_equal (self->priv->background_icon, icon))
    {
      g_clear_object (&self->priv->background_icon);

      if (icon != NULL)
        self->priv->background_icon = g_object_ref (icon);

      ctk_numerable_icon_ensure_emblem (self);

      return TRUE;
    }

  return FALSE;
}

static void
ctk_numerable_icon_constructed (GObject *object)
{
  CtkNumerableIcon *self = CTK_NUMERABLE_ICON (object);

  if (G_OBJECT_CLASS (ctk_numerable_icon_parent_class)->constructed != NULL)
    G_OBJECT_CLASS (ctk_numerable_icon_parent_class)->constructed (object);

  ctk_numerable_icon_ensure_emblem (self);
}

static void
ctk_numerable_icon_finalize (GObject *object)
{
  CtkNumerableIcon *self = CTK_NUMERABLE_ICON (object);

  g_free (self->priv->label);
  g_free (self->priv->rendered_string);

  cdk_rgba_free (self->priv->background);
  cdk_rgba_free (self->priv->foreground);

  pango_font_description_free (self->priv->font);

  cairo_pattern_destroy (self->priv->background_image);

  G_OBJECT_CLASS (ctk_numerable_icon_parent_class)->finalize (object);
}

static void
ctk_numerable_icon_dispose (GObject *object)
{
  CtkNumerableIcon *self = CTK_NUMERABLE_ICON (object);

  if (self->priv->style_changed_id != 0)
    {
      g_signal_handler_disconnect (self->priv->style,
                                   self->priv->style_changed_id);
      self->priv->style_changed_id = 0;
    }

  g_clear_object (&self->priv->style);
  g_clear_object (&self->priv->background_icon);

  G_OBJECT_CLASS (ctk_numerable_icon_parent_class)->dispose (object);
}

static void
ctk_numerable_icon_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  CtkNumerableIcon *self = CTK_NUMERABLE_ICON (object);

  switch (property_id)
    {
    case PROP_COUNT:
      ctk_numerable_icon_set_count (self, g_value_get_int (value));
      break;
    case PROP_LABEL:
      ctk_numerable_icon_set_label (self, g_value_get_string (value));
      break;
    case PROP_STYLE:
      ctk_numerable_icon_set_style_context (self, g_value_get_object (value));
      break;
    case PROP_BACKGROUND_ICON:
      ctk_numerable_icon_set_background_gicon (self, g_value_get_object (value));
      break;
    case PROP_BACKGROUND_ICON_NAME:
      ctk_numerable_icon_set_background_icon_name (self, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ctk_numerable_icon_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  CtkNumerableIcon *self = CTK_NUMERABLE_ICON (object);

  switch (property_id)
    {
    case PROP_COUNT:
      g_value_set_int (value, self->priv->count);
      break;
    case PROP_LABEL:
      g_value_set_string (value, self->priv->label);
      break;
    case PROP_STYLE:
      g_value_set_object (value, self->priv->style);
      break;
    case PROP_BACKGROUND_ICON:
      if (self->priv->background_icon != NULL)
        g_value_set_object (value, self->priv->background_icon);
      break;
    case PROP_BACKGROUND_ICON_NAME:
      g_value_set_string (value, self->priv->background_icon_name);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ctk_numerable_icon_class_init (CtkNumerableIconClass *klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);

  oclass->get_property = ctk_numerable_icon_get_property;
  oclass->set_property = ctk_numerable_icon_set_property;
  oclass->constructed = ctk_numerable_icon_constructed;
  oclass->dispose = ctk_numerable_icon_dispose;
  oclass->finalize = ctk_numerable_icon_finalize;

  properties[PROP_COUNT] =
    g_param_spec_int ("count",
                      P_("Icon's count"),
                      P_("The count of the emblem currently displayed"),
                      -99, 99, 0,
                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  properties[PROP_LABEL] =
    g_param_spec_string ("label",
                         P_("Icon's label"),
                         P_("The label to be displayed over the icon"),
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  properties[PROP_STYLE] =
    g_param_spec_object ("style-context",
                         P_("Icon's style context"),
                         P_("The style context to theme the icon appearance"),
                         CTK_TYPE_STYLE_CONTEXT,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  properties[PROP_BACKGROUND_ICON] =
    g_param_spec_object ("background-icon",
                         P_("Background icon"),
                         P_("The icon for the number emblem background"),
                         G_TYPE_ICON,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  properties[PROP_BACKGROUND_ICON_NAME] =
    g_param_spec_string ("background-icon-name",
                         P_("Background icon name"),
                         P_("The icon name for the number emblem background"),
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (oclass, NUM_PROPERTIES, properties);
}

static void
ctk_numerable_icon_init (CtkNumerableIcon *self)
{
  CdkRGBA bg;
  CdkRGBA fg;

  self->priv = ctk_numerable_icon_get_instance_private (self);

  cdk_rgba_parse (&bg, DEFAULT_BACKGROUND);
  cdk_rgba_parse (&fg, DEFAULT_FOREGROUND);

  self->priv->background = cdk_rgba_copy (&bg);
  self->priv->foreground = cdk_rgba_copy (&fg);

  self->priv->icon_size = 48;
}

/* private */
void
_ctk_numerable_icon_set_background_icon_size (CtkNumerableIcon *self,
                                              gint              icon_size)
{
  if (self->priv->background_icon == NULL)
    return;

  if (self->priv->icon_size != icon_size)
    {
      self->priv->icon_size = icon_size;
      ctk_numerable_icon_ensure_emblem (self);
    }
}

/**
 * ctk_numerable_icon_get_label:
 * @self: a #CtkNumerableIcon
 *
 * Returns the currently displayed label of the icon, or %NULL.
 *
 * Returns: (nullable): the currently displayed label
 *
 * Since: 3.0
 *
 * Deprecated: 3.14
 */
const gchar *
ctk_numerable_icon_get_label (CtkNumerableIcon *self)
{
  g_return_val_if_fail (CTK_IS_NUMERABLE_ICON (self), NULL);

  return self->priv->label;
}

/**
 * ctk_numerable_icon_set_label:
 * @self: a #CtkNumerableIcon
 * @label: (allow-none): a short label, or %NULL
 *
 * Sets the currently displayed value of @self to the string
 * in @label. Setting an empty label removes the emblem.
 *
 * Note that this is meant for displaying short labels, such as
 * roman numbers, or single letters. For roman numbers, consider
 * using the Unicode characters U+2160 - U+217F. Strings longer
 * than two characters will likely not be rendered very well.
 *
 * If this method is called, and a number was already set on the
 * icon, it will automatically be reset to zero before rendering
 * the label, i.e. the last method called between
 * ctk_numerable_icon_set_label() and ctk_numerable_icon_set_count()
 * has always priority.
 *
 * Since: 3.0
 *
 * Deprecated: 3.14
 */
void
ctk_numerable_icon_set_label (CtkNumerableIcon *self,
                              const gchar      *label)
{
  g_return_if_fail (CTK_IS_NUMERABLE_ICON (self));

  if (g_strcmp0 (label, self->priv->label) != 0)
    {
      ctk_numerable_icon_ensure_and_replace_label (self, 0, label);
      ctk_numerable_icon_ensure_emblem (self);

      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_LABEL]);
    }
}

/**
 * ctk_numerable_icon_get_count:
 * @self: a #CtkNumerableIcon
 *
 * Returns the value currently displayed by @self.
 *
 * Returns: the currently displayed value
 *
 * Since: 3.0
 *
 * Deprecated: 3.14
 */
gint
ctk_numerable_icon_get_count (CtkNumerableIcon *self)
{
  g_return_val_if_fail (CTK_IS_NUMERABLE_ICON (self), 0);

  return self->priv->count;
}

/**
 * ctk_numerable_icon_set_count:
 * @self: a #CtkNumerableIcon
 * @count: a number between -99 and 99
 *
 * Sets the currently displayed value of @self to @count.
 *
 * The numeric value is always clamped to make it two digits, i.e.
 * between -99 and 99. Setting a count of zero removes the emblem.
 * If this method is called, and a label was already set on the icon,
 * it will automatically be reset to %NULL before rendering the number,
 * i.e. the last method called between ctk_numerable_icon_set_count()
 * and ctk_numerable_icon_set_label() has always priority.
 *
 * Since: 3.0
 *
 * Deprecated: 3.14
 */
void
ctk_numerable_icon_set_count (CtkNumerableIcon *self,
                              gint              count)
{
  g_return_if_fail (CTK_IS_NUMERABLE_ICON (self));

  if (count != self->priv->count)
    {
      ctk_numerable_icon_ensure_and_replace_label (self, count, NULL);
      ctk_numerable_icon_ensure_emblem (self);

      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_COUNT]);
    }
}

/**
 * ctk_numerable_icon_get_style_context:
 * @self: a #CtkNumerableIcon
 *
 * Returns the #CtkStyleContext used by the icon for theming,
 * or %NULL if there’s none.
 *
 * Returns: (nullable) (transfer none): a #CtkStyleContext, or %NULL.
 *     This object is internal to CTK+ and should not be unreffed.
 *     Use g_object_ref() if you want to keep it around
 *
 * Since: 3.0
 *
 * Deprecated: 3.14
 */
CtkStyleContext *
ctk_numerable_icon_get_style_context (CtkNumerableIcon *self)
{
  g_return_val_if_fail (CTK_IS_NUMERABLE_ICON (self), NULL);

  return self->priv->style;
}

/**
 * ctk_numerable_icon_set_style_context:
 * @self: a #CtkNumerableIcon
 * @style: a #CtkStyleContext
 *
 * Updates the icon to fetch theme information from the
 * given #CtkStyleContext.
 *
 * Since: 3.0
 *
 * Deprecated: 3.14
 */
void
ctk_numerable_icon_set_style_context (CtkNumerableIcon *self,
                                      CtkStyleContext  *style)
{
  g_return_if_fail (CTK_IS_NUMERABLE_ICON (self));
  g_return_if_fail (CTK_IS_STYLE_CONTEXT (style));

  if (style != self->priv->style)
    {
      if (self->priv->style_changed_id != 0)
        g_signal_handler_disconnect (self->priv->style,
                                     self->priv->style_changed_id);

      if (self->priv->style != NULL)
        g_object_unref (self->priv->style);

      self->priv->style = g_object_ref (style);

      ctk_numerable_icon_init_style (self);

      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_STYLE]);
    }
}

/**
 * ctk_numerable_icon_set_background_gicon:
 * @self: a #CtkNumerableIcon
 * @icon: (allow-none): a #GIcon, or %NULL
 *
 * Updates the icon to use @icon as the base background image.
 * If @icon is %NULL, @self will go back using style information
 * or default theming for its background image.
 *
 * If this method is called and an icon name was already set as
 * background for the icon, @icon will be used, i.e. the last method
 * called between ctk_numerable_icon_set_background_gicon() and
 * ctk_numerable_icon_set_background_icon_name() has always priority.
 *
 * Since: 3.0
 *
 * Deprecated: 3.14
 */
void
ctk_numerable_icon_set_background_gicon (CtkNumerableIcon *self,
                                         GIcon            *icon)
{
  gboolean res;

  g_return_if_fail (CTK_IS_NUMERABLE_ICON (self));

  g_clear_pointer (&self->priv->background_icon_name, g_free);

  res = real_set_background_icon (self, icon);

  if (res)
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_BACKGROUND_ICON]);
}

/**
 * ctk_numerable_icon_get_background_gicon:
 * @self: a #CtkNumerableIcon
 *
 * Returns the #GIcon that was set as the base background image, or
 * %NULL if there’s none. The caller of this function does not own
 * a reference to the returned #GIcon.
 *
 * Returns: (nullable) (transfer none): a #GIcon, or %NULL
 *
 * Since: 3.0
 *
 * Deprecated: 3.14
 */
GIcon *
ctk_numerable_icon_get_background_gicon (CtkNumerableIcon *self)
{
  GIcon *retval = NULL;

  g_return_val_if_fail (CTK_IS_NUMERABLE_ICON (self), NULL);

  /* return the GIcon only if it wasn't created from an icon name */
  if (self->priv->background_icon_name == NULL)
    retval = self->priv->background_icon;

  return retval;
}

/**
 * ctk_numerable_icon_set_background_icon_name:
 * @self: a #CtkNumerableIcon
 * @icon_name: (allow-none): an icon name, or %NULL
 *
 * Updates the icon to use the icon named @icon_name from the
 * current icon theme as the base background image. If @icon_name
 * is %NULL, @self will go back using style information or default
 * theming for its background image.
 *
 * If this method is called and a #GIcon was already set as
 * background for the icon, @icon_name will be used, i.e. the
 * last method called between ctk_numerable_icon_set_background_icon_name()
 * and ctk_numerable_icon_set_background_gicon() has always priority.
 *
 * Since: 3.0
 *
 * Deprecated: 3.14
 */
void
ctk_numerable_icon_set_background_icon_name (CtkNumerableIcon *self,
                                             const gchar      *icon_name)
{
  GIcon *icon = NULL;
  gboolean res;

  g_return_if_fail (CTK_IS_NUMERABLE_ICON (self));

  if (g_strcmp0 (icon_name, self->priv->background_icon_name) != 0)
    {
      g_free (self->priv->background_icon_name);
      self->priv->background_icon_name = g_strdup (icon_name);
    }

  if (icon_name != NULL)
    icon = g_themed_icon_new_with_default_fallbacks (icon_name);

  res = real_set_background_icon (self, icon);

  if (res)
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_BACKGROUND_ICON_NAME]);

  if (icon != NULL)
    g_object_unref (icon);
}

/**
 * ctk_numerable_icon_get_background_icon_name:
 * @self: a #CtkNumerableIcon
 *
 * Returns the icon name used as the base background image,
 * or %NULL if there’s none.
 *
 * Returns: (nullable): an icon name, or %NULL
 *
 * Since: 3.0
 *
 * Deprecated: 3.14
 */
const gchar *
ctk_numerable_icon_get_background_icon_name (CtkNumerableIcon *self)
{
  g_return_val_if_fail (CTK_IS_NUMERABLE_ICON (self), NULL);

  return self->priv->background_icon_name;
}

/**
 * ctk_numerable_icon_new:
 * @base_icon: a #GIcon to overlay on
 *
 * Creates a new unthemed #CtkNumerableIcon.
 *
 * Returns: (transfer full): a new #GIcon
 *
 * Since: 3.0
 *
 * Deprecated: 3.14
 */
GIcon *
ctk_numerable_icon_new (GIcon *base_icon)
{
  g_return_val_if_fail (G_IS_ICON (base_icon), NULL);

  return g_object_new (CTK_TYPE_NUMERABLE_ICON,
                       "gicon", base_icon,
                       NULL);
}

/**
 * ctk_numerable_icon_new_with_style_context:
 * @base_icon: a #GIcon to overlay on
 * @context: a #CtkStyleContext
 *
 * Creates a new #CtkNumerableIcon which will themed according
 * to the passed #CtkStyleContext. This is a convenience constructor
 * that calls ctk_numerable_icon_set_style_context() internally.
 *
 * Returns: (transfer full): a new #GIcon
 *
 * Since: 3.0
 *
 * Deprecated: 3.14
 */
GIcon *
ctk_numerable_icon_new_with_style_context (GIcon           *base_icon,
                                           CtkStyleContext *context)
{
  g_return_val_if_fail (G_IS_ICON (base_icon), NULL);

  return g_object_new (CTK_TYPE_NUMERABLE_ICON,
                       "gicon", base_icon,
                       "style-context", context,
                       NULL);
}
