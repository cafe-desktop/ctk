/* CTK - The GIMP Toolkit
 * ctkfontchooser.h - Abstract interface for font file selectors GUIs
 *
 * Copyright (C) 2006, Emmanuele Bassi
 * Copyright (C) 2011 Alberto Ruiz <aruiz@gnome.org>
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

#ifndef __CTK_FONT_CHOOSER_H__
#define __CTK_FONT_CHOOSER_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkwidget.h>

G_BEGIN_DECLS

/**
 * CtkFontFilterFunc:
 * @family: a #PangoFontFamily
 * @face: a #PangoFontFace belonging to @family
 * @data: (closure): user data passed to ctk_font_chooser_set_filter_func()
 *
 * The type of function that is used for deciding what fonts get
 * shown in a #CtkFontChooser. See ctk_font_chooser_set_filter_func().
 *
 * Returns: %TRUE if the font should be displayed
 */
typedef gboolean (*CtkFontFilterFunc) (const PangoFontFamily *family,
                                       const PangoFontFace   *face,
                                       gpointer               data);

/**
 * CtkFontChooserLevel:
 * @CTK_FONT_CHOOSER_LEVEL_FAMILY: Allow selecting a font family
 * @CTK_FONT_CHOOSER_LEVEL_STYLE: Allow selecting a specific font face
 * @CTK_FONT_CHOOSER_LEVEL_SIZE: Allow selecting a specific font size
 * @CTK_FONT_CHOOSER_LEVEL_VARIATION: Allow changing OpenType font variation axes
 * @CTK_FONT_CHOOSER_LEVEL_FEATURES: Allow selecting specific OpenType font features
 *
 * This enumeration specifies the granularity of font selection
 * that is desired in a font chooser.
 *
 * This enumeration may be extended in the future; applications should
 * ignore unknown values.
 */
typedef enum {
  CTK_FONT_CHOOSER_LEVEL_FAMILY     = 0,
  CTK_FONT_CHOOSER_LEVEL_STYLE      = 1 << 0,
  CTK_FONT_CHOOSER_LEVEL_SIZE       = 1 << 1,
  CTK_FONT_CHOOSER_LEVEL_VARIATIONS = 1 << 2,
  CTK_FONT_CHOOSER_LEVEL_FEATURES   = 1 << 3
} CtkFontChooserLevel;

#define CTK_TYPE_FONT_CHOOSER			(ctk_font_chooser_get_type ())
#define CTK_FONT_CHOOSER(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_FONT_CHOOSER, CtkFontChooser))
#define CTK_IS_FONT_CHOOSER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_FONT_CHOOSER))
#define CTK_FONT_CHOOSER_GET_IFACE(inst)	(G_TYPE_INSTANCE_GET_INTERFACE ((inst), CTK_TYPE_FONT_CHOOSER, CtkFontChooserIface))

typedef struct _CtkFontChooser      CtkFontChooser; /* dummy */
typedef struct _CtkFontChooserIface CtkFontChooserIface;

struct _CtkFontChooserIface
{
  GTypeInterface base_iface;

  /* Methods */
  PangoFontFamily * (* get_font_family)         (CtkFontChooser  *fontchooser);
  PangoFontFace *   (* get_font_face)           (CtkFontChooser  *fontchooser);
  gint              (* get_font_size)           (CtkFontChooser  *fontchooser);

  void              (* set_filter_func)         (CtkFontChooser   *fontchooser,
                                                 CtkFontFilterFunc filter,
                                                 gpointer          user_data,
                                                 GDestroyNotify    destroy);

  /* Signals */
  void (* font_activated) (CtkFontChooser *chooser,
                           const gchar    *fontname);

  /* More methods */
  void              (* set_font_map)            (CtkFontChooser   *fontchooser,
                                                 PangoFontMap     *fontmap);
  PangoFontMap *    (* get_font_map)            (CtkFontChooser   *fontchooser);

   /* Padding */
  gpointer padding[10];
};

CDK_AVAILABLE_IN_3_2
GType            ctk_font_chooser_get_type                 (void) G_GNUC_CONST;

CDK_AVAILABLE_IN_3_2
PangoFontFamily *ctk_font_chooser_get_font_family          (CtkFontChooser   *fontchooser);
CDK_AVAILABLE_IN_3_2
PangoFontFace   *ctk_font_chooser_get_font_face            (CtkFontChooser   *fontchooser);
CDK_AVAILABLE_IN_3_2
gint             ctk_font_chooser_get_font_size            (CtkFontChooser   *fontchooser);

CDK_AVAILABLE_IN_3_2
PangoFontDescription *
                 ctk_font_chooser_get_font_desc            (CtkFontChooser             *fontchooser);
CDK_AVAILABLE_IN_3_2
void             ctk_font_chooser_set_font_desc            (CtkFontChooser             *fontchooser,
                                                            const PangoFontDescription *font_desc);

CDK_AVAILABLE_IN_3_2
gchar*           ctk_font_chooser_get_font                 (CtkFontChooser   *fontchooser);

CDK_AVAILABLE_IN_3_2
void             ctk_font_chooser_set_font                 (CtkFontChooser   *fontchooser,
                                                            const gchar      *fontname);
CDK_AVAILABLE_IN_3_2
gchar*           ctk_font_chooser_get_preview_text         (CtkFontChooser   *fontchooser);
CDK_AVAILABLE_IN_3_2
void             ctk_font_chooser_set_preview_text         (CtkFontChooser   *fontchooser,
                                                            const gchar      *text);
CDK_AVAILABLE_IN_3_2
gboolean         ctk_font_chooser_get_show_preview_entry   (CtkFontChooser   *fontchooser);
CDK_AVAILABLE_IN_3_2
void             ctk_font_chooser_set_show_preview_entry   (CtkFontChooser   *fontchooser,
                                                            gboolean          show_preview_entry);
CDK_AVAILABLE_IN_3_2
void             ctk_font_chooser_set_filter_func          (CtkFontChooser   *fontchooser,
                                                            CtkFontFilterFunc filter,
                                                            gpointer          user_data,
                                                            GDestroyNotify    destroy);
CDK_AVAILABLE_IN_3_18
void             ctk_font_chooser_set_font_map             (CtkFontChooser   *fontchooser,
                                                            PangoFontMap     *fontmap);
CDK_AVAILABLE_IN_3_18
PangoFontMap *   ctk_font_chooser_get_font_map             (CtkFontChooser   *fontchooser);

CDK_AVAILABLE_IN_3_24
void             ctk_font_chooser_set_level                (CtkFontChooser   *fontchooser,
                                                            CtkFontChooserLevel level);
CDK_AVAILABLE_IN_3_24
CtkFontChooserLevel
                 ctk_font_chooser_get_level                (CtkFontChooser   *fontchooser);
CDK_AVAILABLE_IN_3_24
char *           ctk_font_chooser_get_font_features        (CtkFontChooser   *fontchooser);
CDK_AVAILABLE_IN_3_24
char *           ctk_font_chooser_get_language             (CtkFontChooser   *fontchooser);
CDK_AVAILABLE_IN_3_24
void             ctk_font_chooser_set_language             (CtkFontChooser   *fontchooser,
                                                            const char       *language);

G_END_DECLS

#endif /* __CTK_FONT_CHOOSER_H__ */
