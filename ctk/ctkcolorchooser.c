/* CTK - The GIMP Toolkit
 *
 * Copyright (C) 2012, Red Hat, Inc.
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

#include "ctkcolorchooser.h"
#include "ctkcolorchooserprivate.h"
#include "ctkintl.h"
#include "ctktypebuiltins.h"
#include "ctkprivate.h"

/**
 * SECTION:ctkcolorchooser
 * @Short_description: Interface implemented by widgets for choosing colors
 * @Title: CtkColorChooser
 * @See_also: #CtkColorChooserDialog, #CtkColorChooserWidget, #CtkColorButton
 *
 * #CtkColorChooser is an interface that is implemented by widgets
 * for choosing colors. Depending on the situation, colors may be
 * allowed to have alpha (translucency).
 *
 * In CTK+, the main widgets that implement this interface are
 * #CtkColorChooserWidget, #CtkColorChooserDialog and #CtkColorButton.
 *
 * Since: 3.4
 */

enum
{
  COLOR_ACTIVATED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

G_DEFINE_INTERFACE (CtkColorChooser, ctk_color_chooser, G_TYPE_OBJECT);

static void
ctk_color_chooser_default_init (CtkColorChooserInterface *iface)
{
  /**
   * CtkColorChooser:rgba:
   *
   * The ::rgba property contains the currently selected color,
   * as a #CdkRGBA struct. The property can be set to change
   * the current selection programmatically.
   *
   * Since: 3.4
   */
  g_object_interface_install_property (iface,
      g_param_spec_boxed ("rgba",
                          P_("Color"),
                          P_("Current color, as a CdkRGBA"),
                          CDK_TYPE_RGBA,
                          CTK_PARAM_READWRITE));

  /**
   * CtkColorChooser:use-alpha:
   *
   * When ::use-alpha is %TRUE, colors may have alpha (translucency)
   * information. When it is %FALSE, the #CdkRGBA struct obtained
   * via the #CtkColorChooser:rgba property will be forced to have
   * alpha == 1.
   *
   * Implementations are expected to show alpha by rendering the color
   * over a non-uniform background (like a checkerboard pattern).
   *
   * Since: 3.4
   */
  g_object_interface_install_property (iface,
      g_param_spec_boolean ("use-alpha",
                            P_("Use alpha"),
                            P_("Whether alpha should be shown"),
                            TRUE,
                            CTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * CtkColorChooser::color-activated:
   * @chooser: the object which received the signal
   * @color: the color
   *
   * Emitted when a color is activated from the color chooser.
   * This usually happens when the user clicks a color swatch,
   * or a color is selected and the user presses one of the keys
   * Space, Shift+Space, Return or Enter.
   *
   * Since: 3.4
   */
  signals[COLOR_ACTIVATED] =
    g_signal_new (I_("color-activated"),
                  CTK_TYPE_COLOR_CHOOSER,
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (CtkColorChooserInterface, color_activated),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  1, CDK_TYPE_RGBA);
}

void
_ctk_color_chooser_color_activated (CtkColorChooser *chooser,
                                    const CdkRGBA   *color)
{
  g_signal_emit (chooser, signals[COLOR_ACTIVATED], 0, color);
}

/**
 * ctk_color_chooser_get_rgba:
 * @chooser: a #CtkColorChooser
 * @color: (out): a #CdkRGBA to fill in with the current color
 *
 * Gets the currently-selected color.
 *
 * Since: 3.4
 */
void
ctk_color_chooser_get_rgba (CtkColorChooser *chooser,
                            CdkRGBA         *color)
{
  g_return_if_fail (CTK_IS_COLOR_CHOOSER (chooser));

  CTK_COLOR_CHOOSER_GET_IFACE (chooser)->get_rgba (chooser, color);
}

/**
 * ctk_color_chooser_set_rgba:
 * @chooser: a #CtkColorChooser
 * @color: the new color
 *
 * Sets the color.
 *
 * Since: 3.4
 */
void
ctk_color_chooser_set_rgba (CtkColorChooser *chooser,
                            const CdkRGBA   *color)
{
  g_return_if_fail (CTK_IS_COLOR_CHOOSER (chooser));
  g_return_if_fail (color != NULL);

  CTK_COLOR_CHOOSER_GET_IFACE (chooser)->set_rgba (chooser, color);
}

/**
 * ctk_color_chooser_get_use_alpha:
 * @chooser: a #CtkColorChooser
 *
 * Returns whether the color chooser shows the alpha channel.
 *
 * Returns: %TRUE if the color chooser uses the alpha channel,
 *     %FALSE if not
 *
 * Since: 3.4
 */
gboolean
ctk_color_chooser_get_use_alpha (CtkColorChooser *chooser)
{
  gboolean use_alpha;

  g_return_val_if_fail (CTK_IS_COLOR_CHOOSER (chooser), TRUE);

  g_object_get (chooser, "use-alpha", &use_alpha, NULL);

  return use_alpha;
}

/**
 * ctk_color_chooser_set_use_alpha:
 * @chooser: a #CtkColorChooser
 * @use_alpha: %TRUE if color chooser should use alpha channel, %FALSE if not
 *
 * Sets whether or not the color chooser should use the alpha channel.
 *
 * Since: 3.4
 */
void
ctk_color_chooser_set_use_alpha (CtkColorChooser *chooser,
                                 gboolean         use_alpha)
{

  g_return_if_fail (CTK_IS_COLOR_CHOOSER (chooser));

  g_object_set (chooser, "use-alpha", use_alpha, NULL);
}

/**
 * ctk_color_chooser_add_palette:
 * @chooser: a #CtkColorChooser
 * @orientation: %CTK_ORIENTATION_HORIZONTAL if the palette should
 *     be displayed in rows, %CTK_ORIENTATION_VERTICAL for columns
 * @colors_per_line: the number of colors to show in each row/column
 * @n_colors: the total number of elements in @colors
 * @colors: (allow-none) (array length=n_colors): the colors of the palette, or %NULL
 *
 * Adds a palette to the color chooser. If @orientation is horizontal,
 * the colors are grouped in rows, with @colors_per_line colors
 * in each row. If @horizontal is %FALSE, the colors are grouped
 * in columns instead.
 *
 * The default color palette of #CtkColorChooserWidget has
 * 27 colors, organized in columns of 3 colors. The default gray
 * palette has 9 grays in a single row.
 *
 * The layout of the color chooser widget works best when the
 * palettes have 9-10 columns.
 *
 * Calling this function for the first time has the
 * side effect of removing the default color and gray palettes
 * from the color chooser.
 *
 * If @colors is %NULL, removes all previously added palettes.
 *
 * Since: 3.4
 */
void
ctk_color_chooser_add_palette (CtkColorChooser *chooser,
                               CtkOrientation   orientation,
                               gint             colors_per_line,
                               gint             n_colors,
                               CdkRGBA         *colors)
{
  g_return_if_fail (CTK_IS_COLOR_CHOOSER (chooser));

  if (CTK_COLOR_CHOOSER_GET_IFACE (chooser)->add_palette)
    CTK_COLOR_CHOOSER_GET_IFACE (chooser)->add_palette (chooser, orientation, colors_per_line, n_colors, colors);
}

cairo_pattern_t *
_ctk_color_chooser_get_checkered_pattern (void)
{
  static cairo_surface_t *checkered = NULL;
  cairo_pattern_t *pattern;

  if (checkered == NULL)
    {
      /* need to respect pixman's stride being a multiple of 4 */
      static unsigned char data[8] = { 0xFF, 0x00, 0x00, 0x00,
                                       0x00, 0xFF, 0x00, 0x00 };

      checkered = cairo_image_surface_create_for_data (data,
                                                       CAIRO_FORMAT_A8,
                                                       2, 2, 4);
    }

  pattern = cairo_pattern_create_for_surface (checkered);
  cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);
  cairo_pattern_set_filter (pattern, CAIRO_FILTER_NEAREST);

  return pattern;
}
