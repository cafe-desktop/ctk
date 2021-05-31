/* CTK - The GIMP Toolkit
 *
 * Copyright (C) 2012 Red Hat, Inc.
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

#ifndef __CTK_COLOR_CHOOSER_H__
#define __CTK_COLOR_CHOOSER_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkwidget.h>

G_BEGIN_DECLS

#define CTK_TYPE_COLOR_CHOOSER                  (ctk_color_chooser_get_type ())
#define CTK_COLOR_CHOOSER(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_COLOR_CHOOSER, CtkColorChooser))
#define CTK_IS_COLOR_CHOOSER(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_COLOR_CHOOSER))
#define CTK_COLOR_CHOOSER_GET_IFACE(inst)       (G_TYPE_INSTANCE_GET_INTERFACE ((inst), CTK_TYPE_COLOR_CHOOSER, CtkColorChooserInterface))

typedef struct _CtkColorChooser          CtkColorChooser;
typedef struct _CtkColorChooserInterface CtkColorChooserInterface;

struct _CtkColorChooserInterface
{
  GTypeInterface base_interface;

  /* Methods */
  void (* get_rgba)    (CtkColorChooser *chooser,
                        GdkRGBA         *color);
  void (* set_rgba)    (CtkColorChooser *chooser,
                        const GdkRGBA   *color);

  void (* add_palette) (CtkColorChooser *chooser,
                        CtkOrientation   orientation,
                        gint             colors_per_line,
                        gint             n_colors,
                        GdkRGBA         *colors);

  /* Signals */
  void (* color_activated) (CtkColorChooser *chooser,
                            const GdkRGBA   *color);

  /* Padding */
  gpointer padding[12];
};

GDK_AVAILABLE_IN_3_4
GType    ctk_color_chooser_get_type        (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_3_4
void     ctk_color_chooser_get_rgba       (CtkColorChooser *chooser,
                                           GdkRGBA         *color);
GDK_AVAILABLE_IN_3_4
void     ctk_color_chooser_set_rgba       (CtkColorChooser *chooser,
                                           const GdkRGBA   *color);
GDK_AVAILABLE_IN_3_4
gboolean ctk_color_chooser_get_use_alpha  (CtkColorChooser *chooser);

GDK_AVAILABLE_IN_3_4
void     ctk_color_chooser_set_use_alpha  (CtkColorChooser *chooser,
                                           gboolean         use_alpha);

GDK_AVAILABLE_IN_3_4
void     ctk_color_chooser_add_palette    (CtkColorChooser *chooser,
                                           CtkOrientation   orientation,
                                           gint             colors_per_line,
                                           gint             n_colors,
                                           GdkRGBA         *colors);

G_END_DECLS

#endif /* __CTK_COLOR_CHOOSER_H__ */
