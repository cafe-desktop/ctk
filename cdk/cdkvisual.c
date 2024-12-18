/* CDK - The GIMP Drawing Kit
 * cdkvisual.c
 *
 * Copyright 2001 Sun Microsystems Inc.
 *
 * Erwann Chenede <erwann.chenede@sun.com>
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

#include "cdkvisualprivate.h"
#include "cdkscreenprivate.h"


/**
 * SECTION:visuals
 * @Short_description: Low-level display hardware information
 * @Title: Visuals
 *
 * A #CdkVisual describes a particular video hardware display format.
 * It includes information about the number of bits used for each color,
 * the way the bits are translated into an RGB value for display, and
 * the way the bits are stored in memory. For example, a piece of display
 * hardware might support 24-bit color, 16-bit color, or 8-bit color;
 * meaning 24/16/8-bit pixel sizes. For a given pixel size, pixels can
 * be in different formats; for example the “red” element of an RGB pixel
 * may be in the top 8 bits of the pixel, or may be in the lower 4 bits.
 *
 * There are several standard visuals. The visual returned by
 * cdk_screen_get_system_visual() is the system’s default visual, and
 * the visual returned by cdk_screen_get_rgba_visual() should be used for
 * creating windows with an alpha channel.
 *
 * A number of functions are provided for determining the “best” available
 * visual. For the purposes of making this determination, higher bit depths
 * are considered better, and for visuals of the same bit depth,
 * %CDK_VISUAL_PSEUDO_COLOR is preferred at 8bpp, otherwise, the visual
 * types are ranked in the order of(highest to lowest)
 * %CDK_VISUAL_DIRECT_COLOR, %CDK_VISUAL_TRUE_COLOR,
 * %CDK_VISUAL_PSEUDO_COLOR, %CDK_VISUAL_STATIC_COLOR,
 * %CDK_VISUAL_GRAYSCALE, then %CDK_VISUAL_STATIC_GRAY.
 */

G_DEFINE_TYPE (CdkVisual, cdk_visual, G_TYPE_OBJECT)

static void
cdk_visual_init (CdkVisual *visual G_GNUC_UNUSED)
{
}

static void
cdk_visual_finalize (GObject *object)
{
  G_OBJECT_CLASS (cdk_visual_parent_class)->finalize (object);
}

static void
cdk_visual_class_init (CdkVisualClass *visual_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (visual_class);

  object_class->finalize = cdk_visual_finalize;
}

/**
 * cdk_list_visuals:
 *
 * Lists the available visuals for the default screen.
 * (See cdk_screen_list_visuals())
 * A visual describes a hardware image data format.
 * For example, a visual might support 24-bit color, or 8-bit color,
 * and might expect pixels to be in a certain format.
 *
 * Call g_list_free() on the return value when you’re finished with it.
 *
 * Returns: (transfer container) (element-type CdkVisual):
 *     a list of visuals; the list must be freed, but not its contents
 * 
 * Deprecated: 3.22: Use cdk_screen_list_visuals (cdk_screen_get_default ()).
 */
GList*
cdk_list_visuals (void)
{
  return cdk_screen_list_visuals (cdk_screen_get_default ());
}

/**
 * cdk_visual_get_system:
 *
 * Get the system’s default visual for the default CDK screen.
 * This is the visual for the root window of the display.
 * The return value should not be freed.
 *
 * Returns: (transfer none): system visual
 *
 * Deprecated: 3.22: Use cdk_screen_get_system_visual (cdk_screen_get_default ()).
 */
CdkVisual*
cdk_visual_get_system (void)
{
  return cdk_screen_get_system_visual (cdk_screen_get_default());
}

/**
 * cdk_visual_get_best_depth:
 *
 * Get the best available depth for the default CDK screen.  “Best”
 * means “largest,” i.e. 32 preferred over 24 preferred over 8 bits
 * per pixel.
 *
 * Returns: best available depth
 *
 * Deprecated: 3.22: Visual selection should be done using
 *     cdk_screen_get_system_visual() and cdk_screen_get_rgba_visual()
 */
gint
cdk_visual_get_best_depth (void)
{
  CdkScreen *screen = cdk_screen_get_default();

  return CDK_SCREEN_GET_CLASS(screen)->visual_get_best_depth (screen);
}

/**
 * cdk_visual_get_best_type:
 *
 * Return the best available visual type for the default CDK screen.
 *
 * Returns: best visual type
 *
 * Deprecated: 3.22: Visual selection should be done using
 *     cdk_screen_get_system_visual() and cdk_screen_get_rgba_visual()
 */
CdkVisualType
cdk_visual_get_best_type (void)
{
  CdkScreen *screen = cdk_screen_get_default();

  return CDK_SCREEN_GET_CLASS(screen)->visual_get_best_type (screen);
}

/**
 * cdk_visual_get_best:
 *
 * Get the visual with the most available colors for the default
 * CDK screen. The return value should not be freed.
 *
 * Returns: (transfer none): best visual
 *
 * Deprecated: 3.22: Visual selection should be done using
 *     cdk_screen_get_system_visual() and cdk_screen_get_rgba_visual()
 */
CdkVisual*
cdk_visual_get_best (void)
{
  CdkScreen *screen = cdk_screen_get_default();

  return CDK_SCREEN_GET_CLASS(screen)->visual_get_best (screen);
}

/**
 * cdk_visual_get_best_with_depth:
 * @depth: a bit depth
 *
 * Get the best visual with depth @depth for the default CDK screen.
 * Color visuals and visuals with mutable colormaps are preferred
 * over grayscale or fixed-colormap visuals. The return value should
 * not be freed. %NULL may be returned if no visual supports @depth.
 *
 * Returns: (transfer none): best visual for the given depth
 *
 * Deprecated: 3.22: Visual selection should be done using
 *     cdk_screen_get_system_visual() and cdk_screen_get_rgba_visual()
 */
CdkVisual*
cdk_visual_get_best_with_depth (gint depth)
{
  CdkScreen *screen = cdk_screen_get_default();

  return CDK_SCREEN_GET_CLASS(screen)->visual_get_best_with_depth (screen, depth);
}

/**
 * cdk_visual_get_best_with_type:
 * @visual_type: a visual type
 *
 * Get the best visual of the given @visual_type for the default CDK screen.
 * Visuals with higher color depths are considered better. The return value
 * should not be freed. %NULL may be returned if no visual has type
 * @visual_type.
 *
 * Returns: (transfer none): best visual of the given type
 *
 * Deprecated: 3.22: Visual selection should be done using
 *     cdk_screen_get_system_visual() and cdk_screen_get_rgba_visual()
 */
CdkVisual*
cdk_visual_get_best_with_type (CdkVisualType visual_type)
{
  CdkScreen *screen = cdk_screen_get_default();

  return CDK_SCREEN_GET_CLASS(screen)->visual_get_best_with_type (screen,
                                                                  visual_type);
}

/**
 * cdk_visual_get_best_with_both:
 * @depth: a bit depth
 * @visual_type: a visual type
 *
 * Combines cdk_visual_get_best_with_depth() and
 * cdk_visual_get_best_with_type().
 *
 * Returns: (nullable) (transfer none): best visual with both @depth
 *     and @visual_type, or %NULL if none
 *
 * Deprecated: 3.22: Visual selection should be done using
 *     cdk_screen_get_system_visual() and cdk_screen_get_rgba_visual()
 */
CdkVisual*
cdk_visual_get_best_with_both (gint          depth,
			       CdkVisualType visual_type)
{
  CdkScreen *screen = cdk_screen_get_default();

  return CDK_SCREEN_GET_CLASS(screen)->visual_get_best_with_both (screen, depth, visual_type);
}

/**
 * cdk_query_depths:
 * @depths: (out) (array length=count) (transfer none): return
 *     location for available depths
 * @count: return location for number of available depths
 *
 * This function returns the available bit depths for the default
 * screen. It’s equivalent to listing the visuals
 * (cdk_list_visuals()) and then looking at the depth field in each
 * visual, removing duplicates.
 *
 * The array returned by this function should not be freed.
 *
 * Deprecated: 3.22: Visual selection should be done using
 *     cdk_screen_get_system_visual() and cdk_screen_get_rgba_visual()
 */
void
cdk_query_depths (gint **depths,
                  gint  *count)
{
  CdkScreen *screen = cdk_screen_get_default();

  CDK_SCREEN_GET_CLASS(screen)->query_depths (screen, depths, count);
}

/**
 * cdk_query_visual_types:
 * @visual_types: (out) (array length=count) (transfer none): return
 *     location for the available visual types
 * @count: return location for the number of available visual types
 *
 * This function returns the available visual types for the default
 * screen. It’s equivalent to listing the visuals
 * (cdk_list_visuals()) and then looking at the type field in each
 * visual, removing duplicates.
 *
 * The array returned by this function should not be freed.
 *
 * Deprecated: 3.22: Visual selection should be done using
 *     cdk_screen_get_system_visual() and cdk_screen_get_rgba_visual()
 */
void
cdk_query_visual_types (CdkVisualType **visual_types,
                        gint           *count)
{
  CdkScreen *screen = cdk_screen_get_default();

  CDK_SCREEN_GET_CLASS(screen)->query_visual_types (screen, visual_types, count);
}

/**
 * cdk_visual_get_visual_type:
 * @visual: A #CdkVisual.
 *
 * Returns the type of visual this is (PseudoColor, TrueColor, etc).
 *
 * Returns: A #CdkVisualType stating the type of @visual.
 *
 * Since: 2.22
 */
CdkVisualType
cdk_visual_get_visual_type (CdkVisual *visual)
{
  g_return_val_if_fail (CDK_IS_VISUAL (visual), 0);

  return visual->type;
}

/**
 * cdk_visual_get_depth:
 * @visual: A #CdkVisual.
 *
 * Returns the bit depth of this visual.
 *
 * Returns: The bit depth of this visual.
 *
 * Since: 2.22
 */
gint
cdk_visual_get_depth (CdkVisual *visual)
{
  g_return_val_if_fail (CDK_IS_VISUAL (visual), 0);

  return visual->depth;
}

/**
 * cdk_visual_get_byte_order:
 * @visual: A #CdkVisual.
 *
 * Returns the byte order of this visual.
 *
 * The information returned by this function is only relevant
 * when working with XImages, and not all backends return
 * meaningful information for this.
 *
 * Returns: A #CdkByteOrder stating the byte order of @visual.
 *
 * Since: 2.22
 *
 * Deprecated: 3.22: This information is not useful
 */
CdkByteOrder
cdk_visual_get_byte_order (CdkVisual *visual)
{
  g_return_val_if_fail (CDK_IS_VISUAL (visual), 0);

  return visual->byte_order;
}

/**
 * cdk_visual_get_colormap_size:
 * @visual: A #CdkVisual.
 *
 * Returns the size of a colormap for this visual.
 *
 * You have to use platform-specific APIs to manipulate colormaps.
 *
 * Returns: The size of a colormap that is suitable for @visual.
 *
 * Since: 2.22
 *
 * Deprecated: 3.22: This information is not useful, since CDK does not
 *     provide APIs to operate on colormaps.
 */
gint
cdk_visual_get_colormap_size (CdkVisual *visual)
{
  g_return_val_if_fail (CDK_IS_VISUAL (visual), 0);

  return visual->colormap_size;
}

/**
 * cdk_visual_get_bits_per_rgb:
 * @visual: a #CdkVisual
 *
 * Returns the number of significant bits per red, green and blue value.
 *
 * Not all CDK backend provide a meaningful value for this function.
 *
 * Returns: The number of significant bits per color value for @visual.
 *
 * Since: 2.22
 *
 * Deprecated: 3.22. Use cdk_visual_get_red_pixel_details() and its variants to
 *     learn about the pixel layout of TrueColor and DirectColor visuals
 */
gint
cdk_visual_get_bits_per_rgb (CdkVisual *visual)
{
  g_return_val_if_fail (CDK_IS_VISUAL (visual), 0);

  return visual->bits_per_rgb;
}

static void
cdk_visual_get_pixel_details (CdkVisual *visual G_GNUC_UNUSED,
                              gulong     pixel_mask,
                              guint32   *mask,
                              gint      *shift,
                              gint      *precision)
{
  gulong m = 0;
  gint s = 0;
  gint p = 0;

  if (pixel_mask != 0)
    {
      m = pixel_mask;
      while (!(m & 0x1))
        {
          s++;
          m >>= 1;
        }

      while (m & 0x1)
        {
          p++;
          m >>= 1;
        }
    }

  if (mask)
    *mask = pixel_mask;

  if (shift)
    *shift = s;

  if (precision)
    *precision = p;
}

/**
 * cdk_visual_get_red_pixel_details:
 * @visual: A #CdkVisual
 * @mask: (out) (allow-none): A pointer to a #guint32 to be filled in, or %NULL
 * @shift: (out) (allow-none): A pointer to a #gint to be filled in, or %NULL
 * @precision: (out) (allow-none): A pointer to a #gint to be filled in, or %NULL
 *
 * Obtains values that are needed to calculate red pixel values in TrueColor
 * and DirectColor. The “mask” is the significant bits within the pixel.
 * The “shift” is the number of bits left we must shift a primary for it
 * to be in position (according to the "mask"). Finally, "precision" refers
 * to how much precision the pixel value contains for a particular primary.
 *
 * Since: 2.22
 */
void
cdk_visual_get_red_pixel_details (CdkVisual *visual,
                                  guint32   *mask,
                                  gint      *shift,
                                  gint      *precision)
{
  g_return_if_fail (CDK_IS_VISUAL (visual));

  cdk_visual_get_pixel_details (visual, visual->red_mask, mask, shift, precision);
}

/**
 * cdk_visual_get_green_pixel_details:
 * @visual: a #CdkVisual
 * @mask: (out) (allow-none): A pointer to a #guint32 to be filled in, or %NULL
 * @shift: (out) (allow-none): A pointer to a #gint to be filled in, or %NULL
 * @precision: (out) (allow-none): A pointer to a #gint to be filled in, or %NULL
 *
 * Obtains values that are needed to calculate green pixel values in TrueColor
 * and DirectColor. The “mask” is the significant bits within the pixel.
 * The “shift” is the number of bits left we must shift a primary for it
 * to be in position (according to the "mask"). Finally, "precision" refers
 * to how much precision the pixel value contains for a particular primary.
 *
 * Since: 2.22
 */
void
cdk_visual_get_green_pixel_details (CdkVisual *visual,
                                    guint32   *mask,
                                    gint      *shift,
                                    gint      *precision)
{
  g_return_if_fail (CDK_IS_VISUAL (visual));

  cdk_visual_get_pixel_details (visual, visual->green_mask, mask, shift, precision);
}

/**
 * cdk_visual_get_blue_pixel_details:
 * @visual: a #CdkVisual
 * @mask: (out) (allow-none): A pointer to a #guint32 to be filled in, or %NULL
 * @shift: (out) (allow-none): A pointer to a #gint to be filled in, or %NULL
 * @precision: (out) (allow-none): A pointer to a #gint to be filled in, or %NULL
 *
 * Obtains values that are needed to calculate blue pixel values in TrueColor
 * and DirectColor. The “mask” is the significant bits within the pixel.
 * The “shift” is the number of bits left we must shift a primary for it
 * to be in position (according to the "mask"). Finally, "precision" refers
 * to how much precision the pixel value contains for a particular primary.
 *
 * Since: 2.22
 */
void
cdk_visual_get_blue_pixel_details (CdkVisual *visual,
                                   guint32   *mask,
                                   gint      *shift,
                                   gint      *precision)
{
  g_return_if_fail (CDK_IS_VISUAL (visual));

  cdk_visual_get_pixel_details (visual, visual->blue_mask, mask, shift, precision);
}

/**
 * cdk_visual_get_screen:
 * @visual: a #CdkVisual
 *
 * Gets the screen to which this visual belongs
 *
 * Returns: (transfer none): the screen to which this visual belongs.
 *
 * Since: 2.2
 */
CdkScreen *
cdk_visual_get_screen (CdkVisual *visual)
{
  g_return_val_if_fail (CDK_IS_VISUAL (visual), NULL);

  return visual->screen;
}
