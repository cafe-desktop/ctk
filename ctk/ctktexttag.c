/* ctktexttag.c - text tag object
 *
 * Copyright (c) 1992-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 2000      Red Hat, Inc.
 * Tk -> Ctk port by Havoc Pennington <hp@redhat.com>
 *
 * This software is copyrighted by the Regents of the University of
 * California, Sun Microsystems, Inc., and other parties.  The
 * following terms apply to all files associated with the software
 * unless explicitly disclaimed in individual files.
 *
 * The authors hereby grant permission to use, copy, modify,
 * distribute, and license this software and its documentation for any
 * purpose, provided that existing copyright notices are retained in
 * all copies and that this notice is included verbatim in any
 * distributions. No written agreement, license, or royalty fee is
 * required for any of the authorized uses.  Modifications to this
 * software may be copyrighted by their authors and need not follow
 * the licensing terms described here, provided that the new terms are
 * clearly indicated on the first page of each file where they apply.
 *
 * IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY
 * PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
 * DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION,
 * OR ANY DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND
 * NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS,
 * AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
 * MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * GOVERNMENT USE: If you are acquiring this software on behalf of the
 * U.S. government, the Government shall have only "Restricted Rights"
 * in the software and related documentation as defined in the Federal
 * Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you
 * are acquiring the software on behalf of the Department of Defense,
 * the software shall be classified as "Commercial Computer Software"
 * and the Government shall have only "Restricted Rights" as defined
 * in Clause 252.227-7013 (c) (1) of DFARs.  Notwithstanding the
 * foregoing, the authors grant the U.S. Government and others acting
 * in its behalf permission to use and distribute the software in
 * accordance with the terms specified in this license.
 *
 */

/**
 * SECTION:ctktexttag
 * @Title: CtkTextTag
 * @Short_description: A tag that can be applied to text in a CtkTextBuffer
 *
 * You may wish to begin by reading the
 * [text widget conceptual overview][TextWidget]
 * which gives an overview of all the objects and
 * data types related to the text widget and how they work together.
 *
 * Tags should be in the #CtkTextTagTable for a given #CtkTextBuffer
 * before using them with that buffer.
 *
 * ctk_text_buffer_create_tag() is the best way to create tags.
 * See “ctk3-demo” for numerous examples.
 *
 * For each property of #CtkTextTag, there is a “set” property, e.g.
 * “font-set” corresponds to “font”. These “set” properties reflect
 * whether a property has been set or not.
 * They are maintained by CTK+ and you should not set them independently.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "ctktextattributesprivate.h"
#include "ctktexttag.h"
#include "ctktexttypes.h"
#include "ctktexttagtable.h"
#include "ctktexttagtableprivate.h"
#include "ctkintl.h"
#include "ctkmarshalers.h"
#include "ctkprivate.h"
#include "ctktypebuiltins.h"

enum {
  EVENT,
  LAST_SIGNAL
};

enum {
  PROP_0,
  /* Construct args */
  PROP_NAME,

  /* Style args */
  PROP_BACKGROUND,
  PROP_FOREGROUND,
  PROP_BACKGROUND_CDK,
  PROP_FOREGROUND_CDK,
  PROP_BACKGROUND_RGBA,
  PROP_FOREGROUND_RGBA,
  PROP_FONT,
  PROP_FONT_DESC,
  PROP_FAMILY,
  PROP_STYLE,
  PROP_VARIANT,
  PROP_WEIGHT,
  PROP_STRETCH,
  PROP_SIZE,
  PROP_SIZE_POINTS,
  PROP_SCALE,
  PROP_PIXELS_ABOVE_LINES,
  PROP_PIXELS_BELOW_LINES,
  PROP_PIXELS_INSIDE_WRAP,
  PROP_EDITABLE,
  PROP_WRAP_MODE,
  PROP_JUSTIFICATION,
  PROP_DIRECTION,
  PROP_LEFT_MARGIN,
  PROP_INDENT,
  PROP_STRIKETHROUGH,
  PROP_STRIKETHROUGH_RGBA,
  PROP_RIGHT_MARGIN,
  PROP_UNDERLINE,
  PROP_UNDERLINE_RGBA,
  PROP_RISE,
  PROP_BACKGROUND_FULL_HEIGHT,
  PROP_LANGUAGE,
  PROP_TABS,
  PROP_INVISIBLE,
  PROP_PARAGRAPH_BACKGROUND,
  PROP_PARAGRAPH_BACKGROUND_CDK,
  PROP_PARAGRAPH_BACKGROUND_RGBA,
  PROP_FALLBACK,
  PROP_LETTER_SPACING,
  PROP_FONT_FEATURES,

  /* Behavior args */
  PROP_ACCUMULATIVE_MARGIN,

  /* Whether-a-style-arg-is-set args */
  PROP_BACKGROUND_SET,
  PROP_FOREGROUND_SET,
  PROP_FAMILY_SET,
  PROP_STYLE_SET,
  PROP_VARIANT_SET,
  PROP_WEIGHT_SET,
  PROP_STRETCH_SET,
  PROP_SIZE_SET,
  PROP_SCALE_SET,
  PROP_PIXELS_ABOVE_LINES_SET,
  PROP_PIXELS_BELOW_LINES_SET,
  PROP_PIXELS_INSIDE_WRAP_SET,
  PROP_EDITABLE_SET,
  PROP_WRAP_MODE_SET,
  PROP_JUSTIFICATION_SET,
  PROP_LEFT_MARGIN_SET,
  PROP_INDENT_SET,
  PROP_STRIKETHROUGH_SET,
  PROP_STRIKETHROUGH_RGBA_SET,
  PROP_RIGHT_MARGIN_SET,
  PROP_UNDERLINE_SET,
  PROP_UNDERLINE_RGBA_SET,
  PROP_RISE_SET,
  PROP_BACKGROUND_FULL_HEIGHT_SET,
  PROP_LANGUAGE_SET,
  PROP_TABS_SET,
  PROP_INVISIBLE_SET,
  PROP_PARAGRAPH_BACKGROUND_SET,
  PROP_FALLBACK_SET,
  PROP_LETTER_SPACING_SET,
  PROP_FONT_FEATURES_SET,

  LAST_ARG
};
static void ctk_text_tag_finalize     (GObject         *object);
static void ctk_text_tag_set_property (GObject         *object,
                                       guint            prop_id,
                                       const GValue    *value,
                                       GParamSpec      *pspec);
static void ctk_text_tag_get_property (GObject         *object,
                                       guint            prop_id,
                                       GValue          *value,
                                       GParamSpec      *pspec);

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (CtkTextTag, ctk_text_tag, G_TYPE_OBJECT)

static void
ctk_text_tag_class_init (CtkTextTagClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = ctk_text_tag_set_property;
  object_class->get_property = ctk_text_tag_get_property;
  
  object_class->finalize = ctk_text_tag_finalize;

  /* Construct */
  g_object_class_install_property (object_class,
                                   PROP_NAME,
                                   g_param_spec_string ("name",
                                                        P_("Tag name"),
                                                        P_("Name used to refer to the text tag. NULL for anonymous tags"),
                                                        NULL,
                                                        CTK_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  /* Style args */

  g_object_class_install_property (object_class,
                                   PROP_BACKGROUND,
                                   g_param_spec_string ("background",
                                                        P_("Background color name"),
                                                        P_("Background color as a string"),
                                                        NULL,
                                                        CTK_PARAM_WRITABLE));

  /**
   * CtkTextTag:background-cdk:
   *
   * Background color as a #CdkColor.
   *
   * Deprecated: 3.4: Use #CtkTextTag:background-rgba instead.
   */
  g_object_class_install_property (object_class,
                                   PROP_BACKGROUND_CDK,
                                   g_param_spec_boxed ("background-cdk",
                                                       P_("Background color"),
                                                       P_("Background color as a CdkColor"),
                                                       g_type_from_name ("CdkColor"),
                                                       CTK_PARAM_READWRITE | G_PARAM_DEPRECATED));

  /**
   * CtkTextTag:background-rgba:
   *
   * Background color as a #CdkRGBA.
   *
   * Since: 3.2
   */
  g_object_class_install_property (object_class,
                                   PROP_BACKGROUND_RGBA,
                                   g_param_spec_boxed ("background-rgba",
                                                       P_("Background RGBA"),
                                                       P_("Background color as a CdkRGBA"),
                                                       CDK_TYPE_RGBA,
                                                       CTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_BACKGROUND_FULL_HEIGHT,
                                   g_param_spec_boolean ("background-full-height",
                                                         P_("Background full height"),
                                                         P_("Whether the background color fills the entire line height or only the height of the tagged characters"),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE));
  
  g_object_class_install_property (object_class,
                                   PROP_FOREGROUND,
                                   g_param_spec_string ("foreground",
                                                        P_("Foreground color name"),
                                                        P_("Foreground color as a string"),
                                                        NULL,
                                                        CTK_PARAM_WRITABLE));

  /**
   * CtkTextTag:foreground-cdk:
   *
   * Foreground color as a #CdkColor.
   *
   * Deprecated: 3.4: Use #CtkTextTag:foreground-rgba instead.
   */
  g_object_class_install_property (object_class,
                                   PROP_FOREGROUND_CDK,
                                   g_param_spec_boxed ("foreground-cdk",
                                                       P_("Foreground color"),
                                                       P_("Foreground color as a CdkColor"),
                                                       g_type_from_name ("CdkColor"),
                                                       CTK_PARAM_READWRITE | G_PARAM_DEPRECATED));

  /**
   * CtkTextTag:foreground-rgba:
   *
   * Foreground color as a #CdkRGBA.
   *
   * Since: 3.2
   */
  g_object_class_install_property (object_class,
                                   PROP_FOREGROUND_RGBA,
                                   g_param_spec_boxed ("foreground-rgba",
                                                       P_("Foreground RGBA"),
                                                       P_("Foreground color as a CdkRGBA"),
                                                       CDK_TYPE_RGBA,
                                                       CTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_DIRECTION,
                                   g_param_spec_enum ("direction",
                                                      P_("Text direction"),
                                                      P_("Text direction, e.g. right-to-left or left-to-right"),
                                                      CTK_TYPE_TEXT_DIRECTION,
                                                      CTK_TEXT_DIR_NONE,
                                                      CTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_EDITABLE,
                                   g_param_spec_boolean ("editable",
                                                         P_("Editable"),
                                                         P_("Whether the text can be modified by the user"),
                                                         TRUE,
                                                         CTK_PARAM_READWRITE));

  /**
   * CtkTextTag:font:
   *
   * Font description as string, e.g. \"Sans Italic 12\". 
   *
   * Note that the initial value of this property depends on
   * the internals of #PangoFontDescription.
   */
  g_object_class_install_property (object_class,
                                   PROP_FONT,
                                   g_param_spec_string ("font",
                                                        P_("Font"),
                                                        P_("Font description as a string, e.g. \"Sans Italic 12\""),
                                                        NULL,
                                                        CTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_FONT_DESC,
                                   g_param_spec_boxed ("font-desc",
                                                       P_("Font"),
                                                       P_("Font description as a PangoFontDescription struct"),
                                                       PANGO_TYPE_FONT_DESCRIPTION,
                                                       CTK_PARAM_READWRITE));
  
  g_object_class_install_property (object_class,
                                   PROP_FAMILY,
                                   g_param_spec_string ("family",
                                                        P_("Font family"),
                                                        P_("Name of the font family, e.g. Sans, Helvetica, Times, Monospace"),
                                                        NULL,
                                                        CTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_STYLE,
                                   g_param_spec_enum ("style",
                                                      P_("Font style"),
                                                      P_("Font style as a PangoStyle, e.g. PANGO_STYLE_ITALIC"),
                                                      PANGO_TYPE_STYLE,
                                                      PANGO_STYLE_NORMAL,
                                                      CTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_VARIANT,
                                   g_param_spec_enum ("variant",
                                                     P_("Font variant"),
                                                     P_("Font variant as a PangoVariant, e.g. PANGO_VARIANT_SMALL_CAPS"),
                                                      PANGO_TYPE_VARIANT,
                                                      PANGO_VARIANT_NORMAL,
                                                      CTK_PARAM_READWRITE));
  
  g_object_class_install_property (object_class,
                                   PROP_WEIGHT,
                                   g_param_spec_int ("weight",
                                                     P_("Font weight"),
                                                     P_("Font weight as an integer, see predefined values in PangoWeight; for example, PANGO_WEIGHT_BOLD"),
                                                     0,
                                                     G_MAXINT,
                                                     PANGO_WEIGHT_NORMAL,
                                                     CTK_PARAM_READWRITE));
  

  g_object_class_install_property (object_class,
                                   PROP_STRETCH,
                                   g_param_spec_enum ("stretch",
                                                      P_("Font stretch"),
                                                      P_("Font stretch as a PangoStretch, e.g. PANGO_STRETCH_CONDENSED"),
                                                      PANGO_TYPE_STRETCH,
                                                      PANGO_STRETCH_NORMAL,
                                                      CTK_PARAM_READWRITE));
  
  g_object_class_install_property (object_class,
                                   PROP_SIZE,
                                   g_param_spec_int ("size",
                                                     P_("Font size"),
                                                     P_("Font size in Pango units"),
                                                     0,
                                                     G_MAXINT,
                                                     0,
                                                     CTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_SCALE,
                                   g_param_spec_double ("scale",
                                                        P_("Font scale"),
                                                        P_("Font size as a scale factor relative to the default font size. This properly adapts to theme changes etc. so is recommended. Pango predefines some scales such as PANGO_SCALE_X_LARGE"),
                                                        0.0,
                                                        G_MAXDOUBLE,
                                                        1.0,
                                                        CTK_PARAM_READWRITE));
  
  g_object_class_install_property (object_class,
                                   PROP_SIZE_POINTS,
                                   g_param_spec_double ("size-points",
                                                        P_("Font points"),
                                                        P_("Font size in points"),
                                                        0.0,
                                                        G_MAXDOUBLE,
                                                        0.0,
                                                        CTK_PARAM_READWRITE));  

  g_object_class_install_property (object_class,
                                   PROP_JUSTIFICATION,
                                   g_param_spec_enum ("justification",
                                                      P_("Justification"),
                                                      P_("Left, right, or center justification"),
                                                      CTK_TYPE_JUSTIFICATION,
                                                      CTK_JUSTIFY_LEFT,
                                                      CTK_PARAM_READWRITE));

  /**
   * CtkTextTag:language:
   *
   * The language this text is in, as an ISO code. Pango can use this as a 
   * hint when rendering the text. If not set, an appropriate default will be 
   * used.
   *
   * Note that the initial value of this property depends on the current
   * locale, see also ctk_get_default_language().
   */
  g_object_class_install_property (object_class,
                                   PROP_LANGUAGE,
                                   g_param_spec_string ("language",
                                                        P_("Language"),
                                                        P_("The language this text is in, as an ISO code. Pango can use this as a hint when rendering the text. If not set, an appropriate default will be used."),
                                                        NULL,
                                                        CTK_PARAM_READWRITE));  

  g_object_class_install_property (object_class,
                                   PROP_LEFT_MARGIN,
                                   g_param_spec_int ("left-margin",
                                                     P_("Left margin"),
                                                     P_("Width of the left margin in pixels"),
                                                     0,
                                                     G_MAXINT,
                                                     0,
                                                     CTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_RIGHT_MARGIN,
                                   g_param_spec_int ("right-margin",
                                                     P_("Right margin"),
                                                     P_("Width of the right margin in pixels"),
                                                     0,
                                                     G_MAXINT,
                                                     0,
                                                     CTK_PARAM_READWRITE));

  
  g_object_class_install_property (object_class,
                                   PROP_INDENT,
                                   g_param_spec_int ("indent",
                                                     P_("Indent"),
                                                     P_("Amount to indent the paragraph, in pixels"),
                                                     G_MININT,
                                                     G_MAXINT,
                                                     0,
                                                     CTK_PARAM_READWRITE));

  
  g_object_class_install_property (object_class,
                                   PROP_RISE,
                                   g_param_spec_int ("rise",
                                                     P_("Rise"),
                                                     P_("Offset of text above the baseline (below the baseline if rise is negative) in Pango units"),
						     G_MININT,
                                                     G_MAXINT,
                                                     0,
                                                     CTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_PIXELS_ABOVE_LINES,
                                   g_param_spec_int ("pixels-above-lines",
                                                     P_("Pixels above lines"),
                                                     P_("Pixels of blank space above paragraphs"),
                                                     0,
                                                     G_MAXINT,
                                                     0,
                                                     CTK_PARAM_READWRITE));
  
  g_object_class_install_property (object_class,
                                   PROP_PIXELS_BELOW_LINES,
                                   g_param_spec_int ("pixels-below-lines",
                                                     P_("Pixels below lines"),
                                                     P_("Pixels of blank space below paragraphs"),
                                                     0,
                                                     G_MAXINT,
                                                     0,
                                                     CTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_PIXELS_INSIDE_WRAP,
                                   g_param_spec_int ("pixels-inside-wrap",
                                                     P_("Pixels inside wrap"),
                                                     P_("Pixels of blank space between wrapped lines in a paragraph"),
                                                     0,
                                                     G_MAXINT,
                                                     0,
                                                     CTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_STRIKETHROUGH,
                                   g_param_spec_boolean ("strikethrough",
                                                         P_("Strikethrough"),
                                                         P_("Whether to strike through the text"),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE));
  
  g_object_class_install_property (object_class,
                                   PROP_UNDERLINE,
                                   g_param_spec_enum ("underline",
                                                      P_("Underline"),
                                                      P_("Style of underline for this text"),
                                                      PANGO_TYPE_UNDERLINE,
                                                      PANGO_UNDERLINE_NONE,
                                                      CTK_PARAM_READWRITE));

  /**
   * CtkTextTag:underline-rgba:
   *
   * This property modifies the color of underlines. If not set, underlines
   * will use the forground color.
   *
   * If #CtkTextTag:underline is set to %PANGO_UNDERLINE_ERROR, an alternate
   * color may be applied instead of the foreground. Setting this property
   * will always override those defaults.
   *
   * Since: 3.16
   */
  g_object_class_install_property (object_class,
                                   PROP_UNDERLINE_RGBA,
                                   g_param_spec_boxed ("underline-rgba",
                                                       P_("Underline RGBA"),
                                                       P_("Color of underline for this text"),
                                                       CDK_TYPE_RGBA,
                                                       CTK_PARAM_READWRITE));

  /**
   * CtkTextTag:strikethrough-rgba:
   *
   * This property modifies the color of strikeouts. If not set, strikeouts
   * will use the forground color.
   *
   * Since: 3.16
   */
  g_object_class_install_property (object_class,
                                   PROP_STRIKETHROUGH_RGBA,
                                   g_param_spec_boxed ("strikethrough-rgba",
                                                       P_("Strikethrough RGBA"),
                                                       P_("Color of strikethrough for this text"),
                                                       CDK_TYPE_RGBA,
                                                       CTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_WRAP_MODE,
                                   g_param_spec_enum ("wrap-mode",
                                                     P_("Wrap mode"),
                                                     P_("Whether to wrap lines never, at word boundaries, or at character boundaries"),
                                                      CTK_TYPE_WRAP_MODE,
                                                      CTK_WRAP_NONE,
                                                      CTK_PARAM_READWRITE));
  

  g_object_class_install_property (object_class,
                                   PROP_TABS,
                                   g_param_spec_boxed ("tabs",
                                                       P_("Tabs"),
                                                       P_("Custom tabs for this text"),
                                                       PANGO_TYPE_TAB_ARRAY,
                                                       CTK_PARAM_READWRITE));
  
  /**
   * CtkTextTag:invisible:
   *
   * Whether this text is hidden.
   *
   * Note that there may still be problems with the support for invisible 
   * text, in particular when navigating programmatically inside a buffer
   * containing invisible segments. 
   *
   * Since: 2.8
   */
  g_object_class_install_property (object_class,
                                   PROP_INVISIBLE,
                                   g_param_spec_boolean ("invisible",
                                                         P_("Invisible"),
                                                         P_("Whether this text is hidden."),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE));

  /**
   * CtkTextTag:paragraph-background:
   *
   * The paragraph background color as a string.
   *
   * Since: 2.8
   */
  g_object_class_install_property (object_class,
                                   PROP_PARAGRAPH_BACKGROUND,
                                   g_param_spec_string ("paragraph-background",
                                                        P_("Paragraph background color name"),
                                                        P_("Paragraph background color as a string"),
                                                        NULL,
                                                        CTK_PARAM_WRITABLE));

  /**
   * CtkTextTag:paragraph-background-cdk:
   *
   * The paragraph background color as a #CdkColor.
   *
   * Since: 2.8
   *
   * Deprecated: 3.4: Use #CtkTextTag:paragraph-background-rgba instead.
   */
  g_object_class_install_property (object_class,
                                   PROP_PARAGRAPH_BACKGROUND_CDK,
                                   g_param_spec_boxed ("paragraph-background-cdk",
                                                       P_("Paragraph background color"),
                                                       P_("Paragraph background color as a CdkColor"),
                                                       g_type_from_name ("CdkColor"),
                                                       CTK_PARAM_READWRITE | G_PARAM_DEPRECATED));

  /**
   * CtkTextTag:paragraph-background-rgba:
   *
   * The paragraph background color as a #CdkRGBA.
   *
   * Since: 3.2
   */
  g_object_class_install_property (object_class,
                                   PROP_PARAGRAPH_BACKGROUND_RGBA,
                                   g_param_spec_boxed ("paragraph-background-rgba",
                                                       P_("Paragraph background RGBA"),
                                                       P_("Paragraph background RGBA as a CdkRGBA"),
                                                       CDK_TYPE_RGBA,
                                                       CTK_PARAM_READWRITE));

  /**
   * CtkTextTag:fallback:
   *
   * Whether font fallback is enabled.
   *
   * When set to %TRUE, other fonts will be substituted
   * where the current font is missing glyphs.
   *
   * Since: 3.16
   */
  g_object_class_install_property (object_class,
                                   PROP_FALLBACK,
                                   g_param_spec_boolean ("fallback",
                                                         P_("Fallback"),
                                                         P_("Whether font fallback is enabled."),
                                                         TRUE,
                                                         CTK_PARAM_READWRITE));

  /**
   * CtkTextTag:letter-spacing:
   *
   * Extra spacing between graphemes, in Pango units.
   *
   * Since: 3.16
   */
  g_object_class_install_property (object_class,
                                   PROP_LETTER_SPACING,
                                   g_param_spec_int ("letter-spacing",
                                                     P_("Letter Spacing"),
                                                     P_("Extra spacing between graphemes"),
                                                     0, G_MAXINT, 0,
                                                     CTK_PARAM_READWRITE));

  /**
   * CtkTextTag:font-features:
   *
   * OpenType font features, as a string.
   *
   * Since: 3.18
   */
  g_object_class_install_property (object_class,
                                   PROP_FONT_FEATURES,
                                   g_param_spec_string ("font-features",
                                                        P_("Font Features"),
                                                        P_("OpenType Font Features to use"),
                                                        NULL,
                                                        CTK_PARAM_READWRITE));

  /**
   * CtkTextTag:accumulative-margin:
   *
   * Whether the margins accumulate or override each other.
   *
   * When set to %TRUE the margins of this tag are added to the margins 
   * of any other non-accumulative margins present. When set to %FALSE 
   * the margins override one another (the default).
   *
   * Since: 2.12
   */
  g_object_class_install_property (object_class,
                                   PROP_ACCUMULATIVE_MARGIN,
                                   g_param_spec_boolean ("accumulative-margin",
                                                         P_("Margin Accumulates"),
                                                         P_("Whether left and right margins accumulate."),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE));

  /* Style props are set or not */

#define ADD_SET_PROP(propname, propval, nick, blurb) g_object_class_install_property (object_class, propval, g_param_spec_boolean (propname, nick, blurb, FALSE, CTK_PARAM_READWRITE))

  ADD_SET_PROP ("background-set", PROP_BACKGROUND_SET,
                P_("Background set"),
                P_("Whether this tag affects the background color"));
  
  ADD_SET_PROP ("background-full-height-set", PROP_BACKGROUND_FULL_HEIGHT_SET,
                P_("Background full height set"),
                P_("Whether this tag affects background height"));

  ADD_SET_PROP ("foreground-set", PROP_FOREGROUND_SET,
                P_("Foreground set"),
                P_("Whether this tag affects the foreground color"));

  ADD_SET_PROP ("editable-set", PROP_EDITABLE_SET,
                P_("Editability set"),
                P_("Whether this tag affects text editability"));

  ADD_SET_PROP ("family-set", PROP_FAMILY_SET,
                P_("Font family set"),
                P_("Whether this tag affects the font family"));  

  ADD_SET_PROP ("style-set", PROP_STYLE_SET,
                P_("Font style set"),
                P_("Whether this tag affects the font style"));

  ADD_SET_PROP ("variant-set", PROP_VARIANT_SET,
                P_("Font variant set"),
                P_("Whether this tag affects the font variant"));

  ADD_SET_PROP ("weight-set", PROP_WEIGHT_SET,
                P_("Font weight set"),
                P_("Whether this tag affects the font weight"));

  ADD_SET_PROP ("stretch-set", PROP_STRETCH_SET,
                P_("Font stretch set"),
                P_("Whether this tag affects the font stretch"));

  ADD_SET_PROP ("size-set", PROP_SIZE_SET,
                P_("Font size set"),
                P_("Whether this tag affects the font size"));

  ADD_SET_PROP ("scale-set", PROP_SCALE_SET,
                P_("Font scale set"),
                P_("Whether this tag scales the font size by a factor"));
  
  ADD_SET_PROP ("justification-set", PROP_JUSTIFICATION_SET,
                P_("Justification set"),
                P_("Whether this tag affects paragraph justification"));
  
  ADD_SET_PROP ("language-set", PROP_LANGUAGE_SET,
                P_("Language set"),
                P_("Whether this tag affects the language the text is rendered as"));

  ADD_SET_PROP ("left-margin-set", PROP_LEFT_MARGIN_SET,
                P_("Left margin set"),
                P_("Whether this tag affects the left margin"));

  ADD_SET_PROP ("indent-set", PROP_INDENT_SET,
                P_("Indent set"),
                P_("Whether this tag affects indentation"));

  ADD_SET_PROP ("rise-set", PROP_RISE_SET,
                P_("Rise set"),
                P_("Whether this tag affects the rise"));

  ADD_SET_PROP ("pixels-above-lines-set", PROP_PIXELS_ABOVE_LINES_SET,
                P_("Pixels above lines set"),
                P_("Whether this tag affects the number of pixels above lines"));

  ADD_SET_PROP ("pixels-below-lines-set", PROP_PIXELS_BELOW_LINES_SET,
                P_("Pixels below lines set"),
                P_("Whether this tag affects the number of pixels above lines"));

  ADD_SET_PROP ("pixels-inside-wrap-set", PROP_PIXELS_INSIDE_WRAP_SET,
                P_("Pixels inside wrap set"),
                P_("Whether this tag affects the number of pixels between wrapped lines"));

  ADD_SET_PROP ("strikethrough-set", PROP_STRIKETHROUGH_SET,
                P_("Strikethrough set"),
                P_("Whether this tag affects strikethrough"));
  
  ADD_SET_PROP ("right-margin-set", PROP_RIGHT_MARGIN_SET,
                P_("Right margin set"),
                P_("Whether this tag affects the right margin"));

  ADD_SET_PROP ("underline-set", PROP_UNDERLINE_SET,
                P_("Underline set"),
                P_("Whether this tag affects underlining"));

  /**
   * CtkTextTag:underline-rgba-set:
   *
   * If the #CtkTextTag:underline-rgba property has been set.
   *
   * Since: 3.16
   */
  ADD_SET_PROP ("underline-rgba-set", PROP_UNDERLINE_RGBA_SET,
                P_("Underline RGBA set"),
                P_("Whether this tag affects underlining color"));

  /**
   * CtkTextTag:strikethrough-rgba-set:
   *
   * If the #CtkTextTag:strikethrough-rgba property has been set.
   *
   * Since: 3.16
   */
  ADD_SET_PROP ("strikethrough-rgba-set", PROP_STRIKETHROUGH_RGBA_SET,
                P_("Strikethrough RGBA set"),
                P_("Whether this tag affects strikethrough color"));

  ADD_SET_PROP ("wrap-mode-set", PROP_WRAP_MODE_SET,
                P_("Wrap mode set"),
                P_("Whether this tag affects line wrap mode"));

  ADD_SET_PROP ("tabs-set", PROP_TABS_SET,
                P_("Tabs set"),
                P_("Whether this tag affects tabs"));

  ADD_SET_PROP ("invisible-set", PROP_INVISIBLE_SET,
                P_("Invisible set"),
                P_("Whether this tag affects text visibility"));

  ADD_SET_PROP ("paragraph-background-set", PROP_PARAGRAPH_BACKGROUND_SET,
                P_("Paragraph background set"),
                P_("Whether this tag affects the paragraph background color"));

  ADD_SET_PROP ("fallback-set", PROP_FALLBACK_SET,
                P_("Fallback set"),
                P_("Whether this tag affects font fallback"));

  ADD_SET_PROP ("letter-spacing-set", PROP_LETTER_SPACING_SET,
                P_("Letter spacing set"),
                P_("Whether this tag affects letter spacing"));

  ADD_SET_PROP ("font-features-set", PROP_FONT_FEATURES_SET,
                P_("Font features set"),
                P_("Whether this tag affects font features"));

  /**
   * CtkTextTag::event:
   * @tag: the #CtkTextTag on which the signal is emitted
   * @object: the object the event was fired from (typically a #CtkTextView)
   * @event: the event which triggered the signal
   * @iter: a #CtkTextIter pointing at the location the event occurred
   *
   * The ::event signal is emitted when an event occurs on a region of the
   * buffer marked with this tag.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the
   * event. %FALSE to propagate the event further.
   */
  signals[EVENT] =
    g_signal_new (I_("event"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (CtkTextTagClass, event),
                  _ctk_boolean_handled_accumulator, NULL,
                  _ctk_marshal_BOOLEAN__OBJECT_BOXED_BOXED,
                  G_TYPE_BOOLEAN,
                  3,
                  G_TYPE_OBJECT,
                  CDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE,
                  CTK_TYPE_TEXT_ITER);
  g_signal_set_va_marshaller (signals[EVENT],
                              G_OBJECT_CLASS_TYPE (object_class),
                              _ctk_marshal_BOOLEAN__OBJECT_BOXED_BOXEDv);
}

static void
ctk_text_tag_init (CtkTextTag *text_tag)
{
  text_tag->priv = ctk_text_tag_get_instance_private (text_tag);
  text_tag->priv->values = ctk_text_attributes_new ();
}

/**
 * ctk_text_tag_new:
 * @name: (allow-none): tag name, or %NULL
 * 
 * Creates a #CtkTextTag. Configure the tag using object arguments,
 * i.e. using g_object_set().
 * 
 * Returns: a new #CtkTextTag
 **/
CtkTextTag*
ctk_text_tag_new (const gchar *name)
{
  CtkTextTag *tag;

  tag = g_object_new (CTK_TYPE_TEXT_TAG, "name", name, NULL);

  return tag;
}

static void
ctk_text_tag_finalize (GObject *object)
{
  CtkTextTag *text_tag = CTK_TEXT_TAG (object);
  CtkTextTagPrivate *priv = text_tag->priv;

  if (priv->table)
    ctk_text_tag_table_remove (priv->table, text_tag);

  g_assert (priv->table == NULL);

  ctk_text_attributes_unref (priv->values);
  priv->values = NULL;

  g_free (priv->name);
  priv->name = NULL;

  G_OBJECT_CLASS (ctk_text_tag_parent_class)->finalize (object);
}

static void
copy_rgba_to_cdk_color (CdkRGBA  *src,
			CdkColor *dest)
{
  dest->red   = CLAMP (src->red,   0.0, 1.0) * 65535.0;
  dest->green = CLAMP (src->green, 0.0, 1.0) * 65535.0;
  dest->blue  = CLAMP (src->blue,  0.0, 1.0) * 65535.0;
}

static void
copy_cdk_color_to_rgba (CdkColor *src,
			CdkRGBA  *dest)
{
  dest->red   = src->red / 65535.;
  dest->green = src->green / 65535.;
  dest->blue  = src->blue / 65535.;
  dest->alpha = 1;
}

static void
set_underline_rgba (CtkTextTag    *tag,
                    const CdkRGBA *rgba)
{
  CtkTextTagPrivate *priv = tag->priv;

  if (rgba)
    {
      CTK_TEXT_APPEARANCE_SET_UNDERLINE_RGBA (&priv->values->appearance, rgba);

      if (!CTK_TEXT_APPEARANCE_GET_UNDERLINE_RGBA_SET (&priv->values->appearance))
        {
          CTK_TEXT_APPEARANCE_SET_UNDERLINE_RGBA_SET (&priv->values->appearance, TRUE);
          g_object_notify (G_OBJECT (tag), "underline-rgba-set");
        }
    }
  else
    {
      CdkRGBA black = { 0 };

      CTK_TEXT_APPEARANCE_SET_UNDERLINE_RGBA (&priv->values->appearance, &black);

      if (CTK_TEXT_APPEARANCE_GET_UNDERLINE_RGBA_SET (&priv->values->appearance))
        {
          CTK_TEXT_APPEARANCE_SET_UNDERLINE_RGBA_SET (&priv->values->appearance, FALSE);
          g_object_notify (G_OBJECT (tag), "underline-rgba-set");
        }
    }
}

static void
set_strikethrough_rgba (CtkTextTag    *tag,
                        const CdkRGBA *rgba)
{
  CtkTextTagPrivate *priv = tag->priv;

  if (rgba)
    {
      CTK_TEXT_APPEARANCE_SET_STRIKETHROUGH_RGBA (&priv->values->appearance, rgba);

      if (!CTK_TEXT_APPEARANCE_GET_STRIKETHROUGH_RGBA_SET (&priv->values->appearance))
        {
          CTK_TEXT_APPEARANCE_SET_STRIKETHROUGH_RGBA_SET (&priv->values->appearance, TRUE);
          g_object_notify (G_OBJECT (tag), "strikethrough-rgba-set");
        }
    }
  else
    {
      CdkRGBA black = { 0 };

      CTK_TEXT_APPEARANCE_SET_STRIKETHROUGH_RGBA (&priv->values->appearance, &black);

      if (CTK_TEXT_APPEARANCE_GET_STRIKETHROUGH_RGBA_SET (&priv->values->appearance))
        {
          CTK_TEXT_APPEARANCE_SET_STRIKETHROUGH_RGBA_SET (&priv->values->appearance, FALSE);
          g_object_notify (G_OBJECT (tag), "strikethrough-rgba-set");
        }
    }
}

static void
set_bg_rgba (CtkTextTag *tag, CdkRGBA *rgba)
{
  CtkTextTagPrivate *priv = tag->priv;

  if (priv->values->appearance.rgba[0])
    cdk_rgba_free (priv->values->appearance.rgba[0]);

  priv->values->appearance.rgba[0] = NULL;

  if (rgba)
    {
      if (!priv->bg_color_set)
        {
          priv->bg_color_set = TRUE;
          g_object_notify (G_OBJECT (tag), "background-set");
        }

      priv->values->appearance.rgba[0] = cdk_rgba_copy (rgba);

      copy_rgba_to_cdk_color (rgba, &priv->values->appearance.bg_color);
    }
  else
    {
      if (priv->bg_color_set)
        {
          priv->bg_color_set = FALSE;
          g_object_notify (G_OBJECT (tag), "background-set");
        }
    }
}

static void
set_fg_rgba (CtkTextTag *tag, CdkRGBA *rgba)
{
  CtkTextTagPrivate *priv = tag->priv;

  if (priv->values->appearance.rgba[1])
    cdk_rgba_free (priv->values->appearance.rgba[1]);

  priv->values->appearance.rgba[1] = NULL;

  if (rgba)
    {
      if (!priv->fg_color_set)
        {
          priv->fg_color_set = TRUE;
          g_object_notify (G_OBJECT (tag), "foreground-set");
        }

      priv->values->appearance.rgba[1] = cdk_rgba_copy (rgba);

      copy_rgba_to_cdk_color (rgba, &priv->values->appearance.fg_color);
    }
  else
    {
      if (priv->fg_color_set)
        {
          priv->fg_color_set = FALSE;
          g_object_notify (G_OBJECT (tag), "foreground-set");
        }
    }
}

static void
set_pg_bg_rgba (CtkTextTag *tag, CdkRGBA *rgba)
{
  CtkTextTagPrivate *priv = tag->priv;

  if (priv->values->pg_bg_rgba)
    cdk_rgba_free (priv->values->pg_bg_rgba);

  if (priv->values->pg_bg_color)
    cdk_color_free (priv->values->pg_bg_color);

  priv->values->pg_bg_rgba = NULL;
  priv->values->pg_bg_color = NULL;

  if (rgba)
    {
      CdkColor color = { 0, };

      if (!priv->pg_bg_color_set)
        {
          priv->pg_bg_color_set = TRUE;
          g_object_notify (G_OBJECT (tag), "paragraph-background-set");
        }

      priv->values->pg_bg_rgba = cdk_rgba_copy (rgba);

      copy_rgba_to_cdk_color (rgba, &color);
      priv->values->pg_bg_color = cdk_color_copy (&color);
    }
  else
    {
      if (priv->pg_bg_color_set)
        {
          priv->pg_bg_color_set = FALSE;
          g_object_notify (G_OBJECT (tag), "paragraph-background-set");
        }
    }
}


static void
set_bg_color (CtkTextTag *tag, CdkColor *color)
{
  if (color)
    {
      CdkRGBA rgba;

      copy_cdk_color_to_rgba (color, &rgba);
      set_bg_rgba (tag, &rgba);
    }
  else
    set_bg_rgba (tag, NULL);
}

static void
set_fg_color (CtkTextTag *tag, CdkColor *color)
{
  if (color)
    {
      CdkRGBA rgba;

      copy_cdk_color_to_rgba (color, &rgba);
      set_fg_rgba (tag, &rgba);
    }
  else
    set_fg_rgba (tag, NULL);
}

static void
set_pg_bg_color (CtkTextTag *tag, CdkColor *color)
{
  if (color)
    {
      CdkRGBA rgba;

      copy_cdk_color_to_rgba (color, &rgba);
      set_pg_bg_rgba (tag, &rgba);
    }
  else
    set_pg_bg_rgba (tag, NULL);
}

static PangoFontMask
get_property_font_set_mask (guint prop_id)
{
  switch (prop_id)
    {
    case PROP_FAMILY_SET:
      return PANGO_FONT_MASK_FAMILY;
    case PROP_STYLE_SET:
      return PANGO_FONT_MASK_STYLE;
    case PROP_VARIANT_SET:
      return PANGO_FONT_MASK_VARIANT;
    case PROP_WEIGHT_SET:
      return PANGO_FONT_MASK_WEIGHT;
    case PROP_STRETCH_SET:
      return PANGO_FONT_MASK_STRETCH;
    case PROP_SIZE_SET:
      return PANGO_FONT_MASK_SIZE;
    }

  return 0;
}

static PangoFontMask
set_font_desc_fields (PangoFontDescription *desc,
		      PangoFontMask         to_set)
{
  PangoFontMask changed_mask = 0;
  
  if (to_set & PANGO_FONT_MASK_FAMILY)
    {
      const char *family = pango_font_description_get_family (desc);
      if (!family)
	{
	  family = "sans";
	  changed_mask |= PANGO_FONT_MASK_FAMILY;
	}

      pango_font_description_set_family (desc, family);
    }
  if (to_set & PANGO_FONT_MASK_STYLE)
    pango_font_description_set_style (desc, pango_font_description_get_style (desc));
  if (to_set & PANGO_FONT_MASK_VARIANT)
    pango_font_description_set_variant (desc, pango_font_description_get_variant (desc));
  if (to_set & PANGO_FONT_MASK_WEIGHT)
    pango_font_description_set_weight (desc, pango_font_description_get_weight (desc));
  if (to_set & PANGO_FONT_MASK_STRETCH)
    pango_font_description_set_stretch (desc, pango_font_description_get_stretch (desc));
  if (to_set & PANGO_FONT_MASK_SIZE)
    {
      gint size = pango_font_description_get_size (desc);
      if (size <= 0)
	{
	  size = 10 * PANGO_SCALE;
	  changed_mask |= PANGO_FONT_MASK_SIZE;
	}
      
      pango_font_description_set_size (desc, size);
    }

  return changed_mask;
}

static void
notify_set_changed (GObject       *object,
		    PangoFontMask  changed_mask)
{
  if (changed_mask & PANGO_FONT_MASK_FAMILY)
    g_object_notify (object, "family-set");
  if (changed_mask & PANGO_FONT_MASK_STYLE)
    g_object_notify (object, "style-set");
  if (changed_mask & PANGO_FONT_MASK_VARIANT)
    g_object_notify (object, "variant-set");
  if (changed_mask & PANGO_FONT_MASK_WEIGHT)
    g_object_notify (object, "weight-set");
  if (changed_mask & PANGO_FONT_MASK_STRETCH)
    g_object_notify (object, "stretch-set");
  if (changed_mask & PANGO_FONT_MASK_SIZE)
    g_object_notify (object, "size-set");
}

static void
notify_fields_changed (GObject       *object,
		       PangoFontMask  changed_mask)
{
  if (changed_mask & PANGO_FONT_MASK_FAMILY)
    g_object_notify (object, "family");
  if (changed_mask & PANGO_FONT_MASK_STYLE)
    g_object_notify (object, "style");
  if (changed_mask & PANGO_FONT_MASK_VARIANT)
    g_object_notify (object, "variant");
  if (changed_mask & PANGO_FONT_MASK_WEIGHT)
    g_object_notify (object, "weight");
  if (changed_mask & PANGO_FONT_MASK_STRETCH)
    g_object_notify (object, "stretch");
  if (changed_mask & PANGO_FONT_MASK_SIZE)
    g_object_notify (object, "size");
}

static void
set_font_description (CtkTextTag           *text_tag,
                      PangoFontDescription *font_desc)
{
  CtkTextTagPrivate *priv = text_tag->priv;
  GObject *object = G_OBJECT (text_tag);
  PangoFontDescription *new_font_desc;
  PangoFontMask old_mask, new_mask, changed_mask, set_changed_mask;
  
  if (font_desc)
    new_font_desc = pango_font_description_copy (font_desc);
  else
    new_font_desc = pango_font_description_new ();

  if (priv->values->font)
    old_mask = pango_font_description_get_set_fields (priv->values->font);
  else
    old_mask = 0;
  
  new_mask = pango_font_description_get_set_fields (new_font_desc);

  changed_mask = old_mask | new_mask;
  set_changed_mask = old_mask ^ new_mask;

  if (priv->values->font)
    pango_font_description_free (priv->values->font);
  priv->values->font = new_font_desc;

  g_object_freeze_notify (object);

  g_object_notify (object, "font-desc");
  g_object_notify (object, "font");
  
  if (changed_mask & PANGO_FONT_MASK_FAMILY)
    g_object_notify (object, "family");
  if (changed_mask & PANGO_FONT_MASK_STYLE)
    g_object_notify (object, "style");
  if (changed_mask & PANGO_FONT_MASK_VARIANT)
    g_object_notify (object, "variant");
  if (changed_mask & PANGO_FONT_MASK_WEIGHT)
    g_object_notify (object, "weight");
  if (changed_mask & PANGO_FONT_MASK_STRETCH)
    g_object_notify (object, "stretch");
  if (changed_mask & PANGO_FONT_MASK_SIZE)
    {
      g_object_notify (object, "size");
      g_object_notify (object, "size-points");
    }

  notify_set_changed (object, set_changed_mask);
  
  g_object_thaw_notify (object);
}

static void
ctk_text_tag_ensure_font (CtkTextTag *text_tag)
{
  CtkTextTagPrivate *priv = text_tag->priv;

  if (!priv->values->font)
    priv->values->font = pango_font_description_new ();
}

static void
ctk_text_tag_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  CtkTextTag *text_tag = CTK_TEXT_TAG (object);
  CtkTextTagPrivate *priv = text_tag->priv;
  gboolean size_changed = FALSE;

  switch (prop_id)
    {
    case PROP_NAME:
      g_return_if_fail (priv->name == NULL);
      priv->name = g_value_dup_string (value);
      break;

    case PROP_BACKGROUND:
      {
        CdkRGBA rgba;

        if (!g_value_get_string (value))
          set_bg_rgba (text_tag, NULL);       /* reset background_set to FALSE */
        else if (cdk_rgba_parse (&rgba, g_value_get_string (value)))
          set_bg_rgba (text_tag, &rgba);
        else
          g_warning ("Don't know color '%s'", g_value_get_string (value));

        g_object_notify (object, "background-cdk");
      }
      break;

    case PROP_FOREGROUND:
      {
        CdkRGBA rgba;

        if (!g_value_get_string (value))
          set_fg_rgba (text_tag, NULL);       /* reset to foreground_set to FALSE */
        else if (cdk_rgba_parse (&rgba, g_value_get_string (value)))
          set_fg_rgba (text_tag, &rgba);
        else
          g_warning ("Don't know color '%s'", g_value_get_string (value));

        g_object_notify (object, "foreground-cdk");
      }
      break;

    case PROP_BACKGROUND_CDK:
      {
        CdkColor *color = g_value_get_boxed (value);

        set_bg_color (text_tag, color);
      }
      break;

    case PROP_FOREGROUND_CDK:
      {
        CdkColor *color = g_value_get_boxed (value);

        set_fg_color (text_tag, color);
      }
      break;

    case PROP_BACKGROUND_RGBA:
      {
        CdkRGBA *color = g_value_get_boxed (value);

        set_bg_rgba (text_tag, color);
      }
      break;

    case PROP_FOREGROUND_RGBA:
      {
        CdkRGBA *color = g_value_get_boxed (value);

        set_fg_rgba (text_tag, color);
      }
      break;

    case PROP_FONT:
      {
        PangoFontDescription *font_desc = NULL;
        const gchar *name;

        name = g_value_get_string (value);

        if (name)
          font_desc = pango_font_description_from_string (name);

        set_font_description (text_tag, font_desc);
	if (font_desc)
	  pango_font_description_free (font_desc);
        
        size_changed = TRUE;
      }
      break;

    case PROP_FONT_DESC:
      {
        PangoFontDescription *font_desc;

        font_desc = g_value_get_boxed (value);

        set_font_description (text_tag, font_desc);

        size_changed = TRUE;
      }
      break;

    case PROP_FAMILY:
    case PROP_STYLE:
    case PROP_VARIANT:
    case PROP_WEIGHT:
    case PROP_STRETCH:
    case PROP_SIZE:
    case PROP_SIZE_POINTS:
      {
	PangoFontMask old_set_mask;

	ctk_text_tag_ensure_font (text_tag);
	old_set_mask = pango_font_description_get_set_fields (priv->values->font);
 
	switch (prop_id)
	  {
	  case PROP_FAMILY:
	    pango_font_description_set_family (priv->values->font,
					       g_value_get_string (value));
	    break;
	  case PROP_STYLE:
	    pango_font_description_set_style (priv->values->font,
					      g_value_get_enum (value));
	    break;
	  case PROP_VARIANT:
	    pango_font_description_set_variant (priv->values->font,
						g_value_get_enum (value));
	    break;
	  case PROP_WEIGHT:
	    pango_font_description_set_weight (priv->values->font,
					       g_value_get_int (value));
	    break;
	  case PROP_STRETCH:
	    pango_font_description_set_stretch (priv->values->font,
						g_value_get_enum (value));
	    break;
	  case PROP_SIZE:
	    pango_font_description_set_size (priv->values->font,
					     g_value_get_int (value));
	    g_object_notify (object, "size-points");
	    break;
	  case PROP_SIZE_POINTS:
	    pango_font_description_set_size (priv->values->font,
					     g_value_get_double (value) * PANGO_SCALE);
	    g_object_notify (object, "size");
	    break;
	  }

	size_changed = TRUE;
	notify_set_changed (object, old_set_mask & pango_font_description_get_set_fields (priv->values->font));
	g_object_notify (object, "font-desc");
	g_object_notify (object, "font");

	break;
      }
      
    case PROP_SCALE:
      priv->values->font_scale = g_value_get_double (value);
      priv->scale_set = TRUE;
      g_object_notify (object, "scale-set");
      size_changed = TRUE;
      break;
      
    case PROP_PIXELS_ABOVE_LINES:
      priv->pixels_above_lines_set = TRUE;
      priv->values->pixels_above_lines = g_value_get_int (value);
      g_object_notify (object, "pixels-above-lines-set");
      size_changed = TRUE;
      break;

    case PROP_PIXELS_BELOW_LINES:
      priv->pixels_below_lines_set = TRUE;
      priv->values->pixels_below_lines = g_value_get_int (value);
      g_object_notify (object, "pixels-below-lines-set");
      size_changed = TRUE;
      break;

    case PROP_PIXELS_INSIDE_WRAP:
      priv->pixels_inside_wrap_set = TRUE;
      priv->values->pixels_inside_wrap = g_value_get_int (value);
      g_object_notify (object, "pixels-inside-wrap-set");
      size_changed = TRUE;
      break;

    case PROP_EDITABLE:
      priv->editable_set = TRUE;
      priv->values->editable = g_value_get_boolean (value);
      g_object_notify (object, "editable-set");
      break;

    case PROP_WRAP_MODE:
      priv->wrap_mode_set = TRUE;
      priv->values->wrap_mode = g_value_get_enum (value);
      g_object_notify (object, "wrap-mode-set");
      size_changed = TRUE;
      break;

    case PROP_JUSTIFICATION:
      priv->justification_set = TRUE;
      priv->values->justification = g_value_get_enum (value);
      g_object_notify (object, "justification-set");
      size_changed = TRUE;
      break;

    case PROP_DIRECTION:
      priv->values->direction = g_value_get_enum (value);
      break;

    case PROP_LEFT_MARGIN:
      priv->left_margin_set = TRUE;
      priv->values->left_margin = g_value_get_int (value);
      g_object_notify (object, "left-margin-set");
      size_changed = TRUE;
      break;

    case PROP_INDENT:
      priv->indent_set = TRUE;
      priv->values->indent = g_value_get_int (value);
      g_object_notify (object, "indent-set");
      size_changed = TRUE;
      break;

    case PROP_STRIKETHROUGH:
      priv->strikethrough_set = TRUE;
      priv->values->appearance.strikethrough = g_value_get_boolean (value);
      g_object_notify (object, "strikethrough-set");
      break;

    case PROP_STRIKETHROUGH_RGBA:
      {
        CdkRGBA *color = g_value_get_boxed (value);
        set_strikethrough_rgba (text_tag, color);
      }
      break;

    case PROP_RIGHT_MARGIN:
      priv->right_margin_set = TRUE;
      priv->values->right_margin = g_value_get_int (value);
      g_object_notify (object, "right-margin-set");
      size_changed = TRUE;
      break;

    case PROP_UNDERLINE:
      priv->underline_set = TRUE;
      priv->values->appearance.underline = g_value_get_enum (value);
      g_object_notify (object, "underline-set");
      break;

    case PROP_UNDERLINE_RGBA:
      {
        CdkRGBA *color = g_value_get_boxed (value);
        set_underline_rgba (text_tag, color);
      }
      break;

    case PROP_RISE:
      priv->rise_set = TRUE;
      priv->values->appearance.rise = g_value_get_int (value);
      g_object_notify (object, "rise-set");
      size_changed = TRUE;      
      break;

    case PROP_BACKGROUND_FULL_HEIGHT:
      priv->bg_full_height_set = TRUE;
      priv->values->bg_full_height = g_value_get_boolean (value);
      g_object_notify (object, "background-full-height-set");
      break;

    case PROP_LANGUAGE:
      priv->language_set = TRUE;
      priv->values->language = pango_language_from_string (g_value_get_string (value));
      g_object_notify (object, "language-set");
      break;

    case PROP_TABS:
      priv->tabs_set = TRUE;

      if (priv->values->tabs)
        pango_tab_array_free (priv->values->tabs);

      /* FIXME I'm not sure if this is a memleak or not */
      priv->values->tabs =
        pango_tab_array_copy (g_value_get_boxed (value));

      g_object_notify (object, "tabs-set");
      
      size_changed = TRUE;
      break;

    case PROP_INVISIBLE:
      priv->invisible_set = TRUE;
      priv->values->invisible = g_value_get_boolean (value);
      g_object_notify (object, "invisible-set");
      size_changed = TRUE;
      break;
      
    case PROP_PARAGRAPH_BACKGROUND:
      {
        CdkRGBA rgba;

        if (!g_value_get_string (value))
          set_pg_bg_rgba (text_tag, NULL);       /* reset paragraph_background_set to FALSE */
        else if (cdk_rgba_parse (&rgba, g_value_get_string (value)))
          set_pg_bg_rgba (text_tag, &rgba);
        else
          g_warning ("Don't know color '%s'", g_value_get_string (value));

        g_object_notify (object, "paragraph-background-cdk");
      }
      break;

    case PROP_PARAGRAPH_BACKGROUND_CDK:
      {
        CdkColor *color = g_value_get_boxed (value);

        set_pg_bg_color (text_tag, color);
      }
      break;

    case PROP_PARAGRAPH_BACKGROUND_RGBA:
      {
        CdkRGBA *color = g_value_get_boxed (value);

        set_pg_bg_rgba (text_tag, color);
      }
      break;

    case PROP_FALLBACK:
      priv->fallback_set = TRUE;
      priv->values->no_fallback = !g_value_get_boolean (value);
      g_object_notify (object, "fallback-set");
      break;

    case PROP_LETTER_SPACING:
      priv->letter_spacing_set = TRUE;
      priv->values->letter_spacing = g_value_get_int (value);
      g_object_notify (object, "letter-spacing-set");
      break;

    case PROP_FONT_FEATURES:
      priv->font_features_set = TRUE;
      priv->values->font_features = g_value_dup_string (value);
      g_object_notify (object, "font-features-set");
      break;

    case PROP_ACCUMULATIVE_MARGIN:
      priv->accumulative_margin = g_value_get_boolean (value);
      g_object_notify (object, "accumulative-margin");
      size_changed = TRUE;
      break;

      /* Whether the value should be used... */

    case PROP_BACKGROUND_SET:
      priv->bg_color_set = g_value_get_boolean (value);
      break;

    case PROP_FOREGROUND_SET:
      priv->fg_color_set = g_value_get_boolean (value);
      break;

    case PROP_FAMILY_SET:
    case PROP_STYLE_SET:
    case PROP_VARIANT_SET:
    case PROP_WEIGHT_SET:
    case PROP_STRETCH_SET:
    case PROP_SIZE_SET:
      if (!g_value_get_boolean (value))
	{
	  if (priv->values->font)
	    pango_font_description_unset_fields (priv->values->font,
						 get_property_font_set_mask (prop_id));
	}
      else
	{
	  PangoFontMask changed_mask;
	  
	  ctk_text_tag_ensure_font (text_tag);
	  changed_mask = set_font_desc_fields (priv->values->font,
					       get_property_font_set_mask (prop_id));
	  notify_fields_changed (G_OBJECT (text_tag), changed_mask);
	}
      break;

    case PROP_SCALE_SET:
      priv->scale_set = g_value_get_boolean (value);
      size_changed = TRUE;
      break;
      
    case PROP_PIXELS_ABOVE_LINES_SET:
      priv->pixels_above_lines_set = g_value_get_boolean (value);
      size_changed = TRUE;
      break;

    case PROP_PIXELS_BELOW_LINES_SET:
      priv->pixels_below_lines_set = g_value_get_boolean (value);
      size_changed = TRUE;
      break;

    case PROP_PIXELS_INSIDE_WRAP_SET:
      priv->pixels_inside_wrap_set = g_value_get_boolean (value);
      size_changed = TRUE;
      break;

    case PROP_EDITABLE_SET:
      priv->editable_set = g_value_get_boolean (value);
      break;

    case PROP_WRAP_MODE_SET:
      priv->wrap_mode_set = g_value_get_boolean (value);
      size_changed = TRUE;
      break;

    case PROP_JUSTIFICATION_SET:
      priv->justification_set = g_value_get_boolean (value);
      size_changed = TRUE;
      break;
      
    case PROP_LEFT_MARGIN_SET:
      priv->left_margin_set = g_value_get_boolean (value);
      size_changed = TRUE;
      break;

    case PROP_INDENT_SET:
      priv->indent_set = g_value_get_boolean (value);
      size_changed = TRUE;
      break;

    case PROP_STRIKETHROUGH_SET:
      priv->strikethrough_set = g_value_get_boolean (value);
      break;

    case PROP_STRIKETHROUGH_RGBA_SET:
      CTK_TEXT_APPEARANCE_SET_STRIKETHROUGH_RGBA_SET (&priv->values->appearance,
                                                      g_value_get_boolean (value));
      break;

    case PROP_RIGHT_MARGIN_SET:
      priv->right_margin_set = g_value_get_boolean (value);
      size_changed = TRUE;
      break;

    case PROP_UNDERLINE_SET:
      priv->underline_set = g_value_get_boolean (value);
      break;

    case PROP_UNDERLINE_RGBA_SET:
      CTK_TEXT_APPEARANCE_SET_UNDERLINE_RGBA_SET (&priv->values->appearance,
                                                  g_value_get_boolean (value));
      break;

    case PROP_RISE_SET:
      priv->rise_set = g_value_get_boolean (value);
      size_changed = TRUE;
      break;

    case PROP_BACKGROUND_FULL_HEIGHT_SET:
      priv->bg_full_height_set = g_value_get_boolean (value);
      break;

    case PROP_LANGUAGE_SET:
      priv->language_set = g_value_get_boolean (value);
      size_changed = TRUE;
      break;

    case PROP_TABS_SET:
      priv->tabs_set = g_value_get_boolean (value);
      size_changed = TRUE;
      break;

    case PROP_INVISIBLE_SET:
      priv->invisible_set = g_value_get_boolean (value);
      size_changed = TRUE;
      break;
      
    case PROP_PARAGRAPH_BACKGROUND_SET:
      priv->pg_bg_color_set = g_value_get_boolean (value);
      break;

    case PROP_FALLBACK_SET:
      priv->fallback_set = g_value_get_boolean (value);
      break;

    case PROP_LETTER_SPACING_SET:
      priv->letter_spacing_set = g_value_get_boolean (value);
      break;

    case PROP_FONT_FEATURES_SET:
      priv->font_features_set = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }

  /* The signal is emitted for each set_property(). A possible optimization is
   * to send the signal only once when several properties are set at the same
   * time with e.g. g_object_set(). The signal could be emitted when the notify
   * signal is thawed.
   */
  ctk_text_tag_changed (text_tag, size_changed);
}

static void
ctk_text_tag_get_property (GObject      *object,
                           guint         prop_id,
                           GValue       *value,
                           GParamSpec   *pspec)
{
  CtkTextTag *tag = CTK_TEXT_TAG (object);
  CtkTextTagPrivate *priv = tag->priv;

  switch (prop_id)
    {
    case PROP_NAME:
      g_value_set_string (value, priv->name);
      break;

    case PROP_BACKGROUND_CDK:
      g_value_set_boxed (value, &priv->values->appearance.bg_color);
      break;

    case PROP_BACKGROUND_RGBA:
      g_value_set_boxed (value, priv->values->appearance.rgba[0]);
      break;

    case PROP_FOREGROUND_CDK:
      g_value_set_boxed (value, &priv->values->appearance.fg_color);
      break;

    case PROP_FOREGROUND_RGBA:
      g_value_set_boxed (value, priv->values->appearance.rgba[1]);
      break;

    case PROP_FONT:
        {
          gchar *str;

	  ctk_text_tag_ensure_font (tag);

	  str = pango_font_description_to_string (priv->values->font);
          g_value_take_string (value, str);
        }
      break;

    case PROP_FONT_DESC:
      ctk_text_tag_ensure_font (tag);
      g_value_set_boxed (value, priv->values->font);
      break;

    case PROP_FAMILY:
    case PROP_STYLE:
    case PROP_VARIANT:
    case PROP_WEIGHT:
    case PROP_STRETCH:
    case PROP_SIZE:
    case PROP_SIZE_POINTS:
      ctk_text_tag_ensure_font (tag);
      switch (prop_id)
	{
	case PROP_FAMILY:
	  g_value_set_string (value, pango_font_description_get_family (priv->values->font));
	  break;
	  
	case PROP_STYLE:
	  g_value_set_enum (value, pango_font_description_get_style (priv->values->font));
	  break;
	  
	case PROP_VARIANT:
	  g_value_set_enum (value, pango_font_description_get_variant (priv->values->font));
	  break;
	  
	case PROP_WEIGHT:
	  g_value_set_int (value, pango_font_description_get_weight (priv->values->font));
	  break;
	  
	case PROP_STRETCH:
	  g_value_set_enum (value, pango_font_description_get_stretch (priv->values->font));
	  break;
	  
	case PROP_SIZE:
	  g_value_set_int (value, pango_font_description_get_size (priv->values->font));
	  break;
	  
	case PROP_SIZE_POINTS:
	  g_value_set_double (value, ((double)pango_font_description_get_size (priv->values->font)) / (double)PANGO_SCALE);
	  break;
	}
      break;
      
    case PROP_SCALE:
      g_value_set_double (value, priv->values->font_scale);
      break;
      
    case PROP_PIXELS_ABOVE_LINES:
      g_value_set_int (value,  priv->values->pixels_above_lines);
      break;

    case PROP_PIXELS_BELOW_LINES:
      g_value_set_int (value,  priv->values->pixels_below_lines);
      break;

    case PROP_PIXELS_INSIDE_WRAP:
      g_value_set_int (value,  priv->values->pixels_inside_wrap);
      break;

    case PROP_EDITABLE:
      g_value_set_boolean (value, priv->values->editable);
      break;

    case PROP_WRAP_MODE:
      g_value_set_enum (value, priv->values->wrap_mode);
      break;

    case PROP_JUSTIFICATION:
      g_value_set_enum (value, priv->values->justification);
      break;

    case PROP_DIRECTION:
      g_value_set_enum (value, priv->values->direction);
      break;
      
    case PROP_LEFT_MARGIN:
      g_value_set_int (value,  priv->values->left_margin);
      break;

    case PROP_INDENT:
      g_value_set_int (value,  priv->values->indent);
      break;

    case PROP_STRIKETHROUGH:
      g_value_set_boolean (value, priv->values->appearance.strikethrough);
      break;

    case PROP_STRIKETHROUGH_RGBA:
      if (CTK_TEXT_APPEARANCE_GET_STRIKETHROUGH_RGBA_SET (&priv->values->appearance))
        {
          CdkRGBA rgba;

          CTK_TEXT_APPEARANCE_GET_STRIKETHROUGH_RGBA (&priv->values->appearance, &rgba);
          g_value_set_boxed (value, &rgba);
        }
      break;

    case PROP_RIGHT_MARGIN:
      g_value_set_int (value, priv->values->right_margin);
      break;

    case PROP_UNDERLINE:
      g_value_set_enum (value, priv->values->appearance.underline);
      break;

    case PROP_UNDERLINE_RGBA:
      if (CTK_TEXT_APPEARANCE_GET_UNDERLINE_RGBA_SET (&priv->values->appearance))
        {
          CdkRGBA rgba;

          CTK_TEXT_APPEARANCE_GET_UNDERLINE_RGBA (&priv->values->appearance, &rgba);
          g_value_set_boxed (value, &rgba);
        }
      break;

    case PROP_RISE:
      g_value_set_int (value, priv->values->appearance.rise);
      break;

    case PROP_BACKGROUND_FULL_HEIGHT:
      g_value_set_boolean (value, priv->values->bg_full_height);
      break;

    case PROP_LANGUAGE:
      g_value_set_string (value, pango_language_to_string (priv->values->language));
      break;

    case PROP_TABS:
      if (priv->values->tabs)
        g_value_set_boxed (value, priv->values->tabs);
      break;

    case PROP_INVISIBLE:
      g_value_set_boolean (value, priv->values->invisible);
      break;
      
    case PROP_PARAGRAPH_BACKGROUND_CDK:
      g_value_set_boxed (value, priv->values->pg_bg_color);
      break;

    case PROP_PARAGRAPH_BACKGROUND_RGBA:
      g_value_set_boxed (value, priv->values->pg_bg_rgba);
      break;

    case PROP_FALLBACK:
      g_value_set_boolean (value, !priv->values->no_fallback);
      break;

    case PROP_LETTER_SPACING:
      g_value_set_int (value, priv->values->letter_spacing);
      break;

    case PROP_FONT_FEATURES:
      g_value_set_string (value, priv->values->font_features);
      break;

    case PROP_ACCUMULATIVE_MARGIN:
      g_value_set_boolean (value, priv->accumulative_margin);
      break;

    case PROP_BACKGROUND_SET:
      g_value_set_boolean (value, priv->bg_color_set);
      break;

    case PROP_FOREGROUND_SET:
      g_value_set_boolean (value, priv->fg_color_set);
      break;

    case PROP_FAMILY_SET:
    case PROP_STYLE_SET:
    case PROP_VARIANT_SET:
    case PROP_WEIGHT_SET:
    case PROP_STRETCH_SET:
    case PROP_SIZE_SET:
      {
	PangoFontMask set_mask = priv->values->font ? pango_font_description_get_set_fields (priv->values->font) : 0;
	PangoFontMask test_mask = get_property_font_set_mask (prop_id);
	g_value_set_boolean (value, (set_mask & test_mask) != 0);

	break;
      }

    case PROP_SCALE_SET:
      g_value_set_boolean (value, priv->scale_set);
      break;
      
    case PROP_PIXELS_ABOVE_LINES_SET:
      g_value_set_boolean (value, priv->pixels_above_lines_set);
      break;

    case PROP_PIXELS_BELOW_LINES_SET:
      g_value_set_boolean (value, priv->pixels_below_lines_set);
      break;

    case PROP_PIXELS_INSIDE_WRAP_SET:
      g_value_set_boolean (value, priv->pixels_inside_wrap_set);
      break;

    case PROP_EDITABLE_SET:
      g_value_set_boolean (value, priv->editable_set);
      break;

    case PROP_WRAP_MODE_SET:
      g_value_set_boolean (value, priv->wrap_mode_set);
      break;

    case PROP_JUSTIFICATION_SET:
      g_value_set_boolean (value, priv->justification_set);
      break;
      
    case PROP_LEFT_MARGIN_SET:
      g_value_set_boolean (value, priv->left_margin_set);
      break;

    case PROP_INDENT_SET:
      g_value_set_boolean (value, priv->indent_set);
      break;

    case PROP_STRIKETHROUGH_SET:
      g_value_set_boolean (value, priv->strikethrough_set);
      break;

    case PROP_STRIKETHROUGH_RGBA_SET:
      g_value_set_boolean (value,
                           CTK_TEXT_APPEARANCE_GET_STRIKETHROUGH_RGBA_SET (&priv->values->appearance));
      break;

    case PROP_RIGHT_MARGIN_SET:
      g_value_set_boolean (value, priv->right_margin_set);
      break;

    case PROP_UNDERLINE_SET:
      g_value_set_boolean (value, priv->underline_set);
      break;

    case PROP_UNDERLINE_RGBA_SET:
      g_value_set_boolean (value,
                           CTK_TEXT_APPEARANCE_GET_UNDERLINE_RGBA_SET (&priv->values->appearance));
      break;

    case PROP_RISE_SET:
      g_value_set_boolean (value, priv->rise_set);
      break;

    case PROP_BACKGROUND_FULL_HEIGHT_SET:
      g_value_set_boolean (value, priv->bg_full_height_set);
      break;

    case PROP_LANGUAGE_SET:
      g_value_set_boolean (value, priv->language_set);
      break;

    case PROP_TABS_SET:
      g_value_set_boolean (value, priv->tabs_set);
      break;

    case PROP_INVISIBLE_SET:
      g_value_set_boolean (value, priv->invisible_set);
      break;
      
    case PROP_PARAGRAPH_BACKGROUND_SET:
      g_value_set_boolean (value, priv->pg_bg_color_set);
      break;

    case PROP_FALLBACK_SET:
      g_value_set_boolean (value, priv->fallback_set);
      break;

    case PROP_LETTER_SPACING_SET:
      g_value_set_boolean (value, priv->letter_spacing_set);
      break;

    case PROP_FONT_FEATURES_SET:
      g_value_set_boolean (value, priv->font_features_set);
      break;

    case PROP_BACKGROUND:
    case PROP_FOREGROUND:
    case PROP_PARAGRAPH_BACKGROUND:
      g_warning ("'foreground', 'background' and 'paragraph_background' properties are not readable, use 'foreground_cdk', 'background_cdk' and 'paragraph_background_cdk'");
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/*
 * Tag operations
 */

typedef struct {
  gint high;
  gint low;
  gint delta;
} DeltaData;

static void
delta_priority_foreach (CtkTextTag *tag, gpointer user_data)
{
  CtkTextTagPrivate *priv = tag->priv;
  DeltaData *dd = user_data;

  if (priv->priority >= dd->low && priv->priority <= dd->high)
    priv->priority += dd->delta;
}

/**
 * ctk_text_tag_get_priority:
 * @tag: a #CtkTextTag
 * 
 * Get the tag priority.
 * 
 * Returns: The tag’s priority.
 **/
gint
ctk_text_tag_get_priority (CtkTextTag *tag)
{
  g_return_val_if_fail (CTK_IS_TEXT_TAG (tag), 0);

  return tag->priv->priority;
}

/**
 * ctk_text_tag_set_priority:
 * @tag: a #CtkTextTag
 * @priority: the new priority
 *
 * Sets the priority of a #CtkTextTag. Valid priorities
 * start at 0 and go to one less than ctk_text_tag_table_get_size().
 * Each tag in a table has a unique priority; setting the priority
 * of one tag shifts the priorities of all the other tags in the
 * table to maintain a unique priority for each tag. Higher priority
 * tags “win” if two tags both set the same text attribute. When adding
 * a tag to a tag table, it will be assigned the highest priority in
 * the table by default; so normally the precedence of a set of tags
 * is the order in which they were added to the table, or created with
 * ctk_text_buffer_create_tag(), which adds the tag to the buffer’s table
 * automatically.
 **/
void
ctk_text_tag_set_priority (CtkTextTag *tag,
                           gint        priority)
{
  CtkTextTagPrivate *priv;
  DeltaData dd;

  g_return_if_fail (CTK_IS_TEXT_TAG (tag));

  priv = tag->priv;

  g_return_if_fail (priv->table != NULL);
  g_return_if_fail (priority >= 0);
  g_return_if_fail (priority < ctk_text_tag_table_get_size (priv->table));

  if (priority == priv->priority)
    return;

  if (priority < priv->priority)
    {
      dd.low = priority;
      dd.high = priv->priority - 1;
      dd.delta = 1;
    }
  else
    {
      dd.low = priv->priority + 1;
      dd.high = priority;
      dd.delta = -1;
    }

  ctk_text_tag_table_foreach (priv->table,
                              delta_priority_foreach,
                              &dd);

  priv->priority = priority;
}

/**
 * ctk_text_tag_event:
 * @tag: a #CtkTextTag
 * @event_object: object that received the event, such as a widget
 * @event: the event
 * @iter: location where the event was received
 * 
 * Emits the “event” signal on the #CtkTextTag.
 * 
 * Returns: result of signal emission (whether the event was handled)
 **/
gboolean
ctk_text_tag_event (CtkTextTag        *tag,
                    GObject           *event_object,
                    CdkEvent          *event,
                    const CtkTextIter *iter)
{
  gboolean retval = FALSE;

  g_return_val_if_fail (CTK_IS_TEXT_TAG (tag), FALSE);
  g_return_val_if_fail (G_IS_OBJECT (event_object), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  g_signal_emit (tag,
                 signals[EVENT],
                 0,
                 event_object,
                 event,
                 iter,
                 &retval);

  return retval;
}

/**
 * ctk_text_tag_changed:
 * @tag: a #CtkTextTag.
 * @size_changed: whether the change affects the #CtkTextView layout.
 *
 * Emits the #CtkTextTagTable::tag-changed signal on the #CtkTextTagTable where
 * the tag is included.
 *
 * The signal is already emitted when setting a #CtkTextTag property. This
 * function is useful for a #CtkTextTag subclass.
 *
 * Since: 3.20
 */
void
ctk_text_tag_changed (CtkTextTag *tag,
                      gboolean    size_changed)
{
  CtkTextTagPrivate *priv;

  g_return_if_fail (CTK_IS_TEXT_TAG (tag));

  priv = tag->priv;

  /* This is somewhat weird since we emit another object's signal here, but the
   * two objects are already tightly bound. If a CtkTextTag::changed signal is
   * added, this would increase significantly the number of signal connections.
   */
  if (priv->table != NULL)
    _ctk_text_tag_table_tag_changed (priv->table, tag, size_changed);
}

static int
tag_sort_func (gconstpointer first, gconstpointer second)
{
  CtkTextTag *tag1, *tag2;

  tag1 = * (CtkTextTag **) first;
  tag2 = * (CtkTextTag **) second;
  return tag1->priv->priority - tag2->priv->priority;
}

void
_ctk_text_tag_array_sort (CtkTextTag** tag_array_p,
                          guint len)
{
  int i, j, prio;
  CtkTextTag **tag;
  CtkTextTag **maxPtrPtr, *tmp;

  g_return_if_fail (tag_array_p != NULL);
  g_return_if_fail (len > 0);

  if (len < 2) {
    return;
  }
  if (len < 20) {
    CtkTextTag **iter = tag_array_p;

    for (i = len-1; i > 0; i--, iter++) {
      maxPtrPtr = tag = iter;
      prio = tag[0]->priv->priority;
      for (j = i, tag++; j > 0; j--, tag++) {
        if (tag[0]->priv->priority < prio) {
          prio = tag[0]->priv->priority;
          maxPtrPtr = tag;
        }
      }
      tmp = *maxPtrPtr;
      *maxPtrPtr = *iter;
      *iter = tmp;
    }
  } else {
    qsort ((void *) tag_array_p, (unsigned) len, sizeof (CtkTextTag *),
           tag_sort_func);
  }

#if 0
  {
    printf ("Sorted tag array: \n");
    i = 0;
    while (i < len)
      {
        CtkTextTag *t = tag_array_p[i];
        printf ("  %s priority %d\n", t->name, t->priority);

        ++i;
      }
  }
#endif
}

