/* ctktextattributes.h - text attributes
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

#ifndef __CTK_TEXT_ATTRIBUTES_H__
#define __CTK_TEXT_ATTRIBUTES_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <cdk/cdk.h>
#include <ctk/ctkenums.h>


G_BEGIN_DECLS

typedef struct _CtkTextAttributes CtkTextAttributes;

#define CTK_TYPE_TEXT_ATTRIBUTES     (ctk_text_attributes_get_type ())

typedef struct _CtkTextAppearance CtkTextAppearance;

/**
 * CtkTextAppearance:
 * @bg_color: Background #CdkColor.
 * @fg_color: Foreground #CdkColor.
 * @rise: Super/subscript rise, can be negative.
 * @underline: #PangoUnderline
 * @strikethrough: Strikethrough style
 * @draw_bg: Whether to use background-related values; this is
 *   irrelevant for the values struct when in a tag, but is used for
 *   the composite values struct; it’s true if any of the tags being
 *   composited had background stuff set.
 * @inside_selection: This are only used when we are actually laying
 *   out and rendering a paragraph; not when a #CtkTextAppearance is
 *   part of a #CtkTextAttributes.
 * @is_text: This are only used when we are actually laying
 *   out and rendering a paragraph; not when a #CtkTextAppearance is
 *   part of a #CtkTextAttributes.
 * @rgba: #CdkRGBA
 */
struct _CtkTextAppearance
{
  /*< public >*/
  CdkColor bg_color; /* pixel is taken for underline color */
  CdkColor fg_color; /* pixel is taken for strikethrough color */

  /* super/subscript rise, can be negative */
  gint rise;

  guint underline : 4;          /* PangoUnderline */
  guint strikethrough : 1;

  /* Whether to use background-related values; this is irrelevant for
   * the values struct when in a tag, but is used for the composite
   * values struct; it's true if any of the tags being composited
   * had background stuff set.
   */
  guint draw_bg : 1;

  /* These are only used when we are actually laying out and rendering
   * a paragraph; not when a CtkTextAppearance is part of a
   * CtkTextAttributes.
   */
  guint inside_selection : 1;
  guint is_text : 1;

  /* For the sad story of this bit of code, see
   * https://bugzilla.gnome.org/show_bug.cgi?id=711158
   */
#ifdef __GI_SCANNER__
  /* The scanner should only see the transparent union, so that its
   * content does not vary across architectures.
   */
  union {
    CdkRGBA *rgba[2];
    /*< private >*/
    guint padding[4];
  };
#else
  CdkRGBA *rgba[2];
#if (defined(__SIZEOF_INT__) && defined(__SIZEOF_POINTER__)) && (__SIZEOF_INT__ == __SIZEOF_POINTER__)
  /* unusable, just for ABI compat */
  /*< private >*/
  guint padding[2];
#endif
#endif
};

/**
 * CtkTextAttributes:
 * @appearance: #CtkTextAppearance for text.
 * @justification: #CtkJustification for text.
 * @direction: #CtkTextDirection for text.
 * @font: #PangoFontDescription for text.
 * @font_scale: Font scale factor.
 * @left_margin: Width of the left margin in pixels.
 * @right_margin: Width of the right margin in pixels.
 * @indent: Amount to indent the paragraph, in pixels.
 * @pixels_above_lines: Pixels of blank space above paragraphs.
 * @pixels_below_lines: Pixels of blank space below paragraphs.
 * @pixels_inside_wrap: Pixels of blank space between wrapped lines in
 *   a paragraph.
 * @tabs: Custom #PangoTabArray for this text.
 * @wrap_mode: #CtkWrapMode for text.
 * @language: #PangoLanguage for text.
 * @invisible: Hide the text.
 * @bg_full_height: Background is fit to full line height rather than
 *    baseline +/- ascent/descent (font height).
 * @editable: Can edit this text.
 * @no_fallback: Whether to disable font fallback.
 * @letter_spacing: Extra space to insert between graphemes, in Pango units
 * @font_features: OpenType font features, as a string.
 *
 * Using #CtkTextAttributes directly should rarely be necessary.
 * It’s primarily useful with ctk_text_iter_get_attributes().
 * As with most CTK+ structs, the fields in this struct should only
 * be read, never modified directly.
 */
struct _CtkTextAttributes
{
  /*< private >*/
  guint refcount;

  /*< public >*/
  CtkTextAppearance appearance;

  CtkJustification justification;
  CtkTextDirection direction;

  PangoFontDescription *font;

  gdouble font_scale;

  gint left_margin;
  gint right_margin;
  gint indent;

  gint pixels_above_lines;
  gint pixels_below_lines;
  gint pixels_inside_wrap;

  PangoTabArray *tabs;

  CtkWrapMode wrap_mode;

  PangoLanguage *language;

  /*< private >*/
  CdkColor *pg_bg_color;

  /*< public >*/
  guint invisible : 1;
  guint bg_full_height : 1;
  guint editable : 1;
  guint no_fallback: 1;

  /*< private >*/
  CdkRGBA *pg_bg_rgba;

  /*< public >*/
  gint letter_spacing;

#ifdef __GI_SCANNER__
  /* The scanner should only see the transparent union, so that its
   * content does not vary across architectures.
   */
  union {
    gchar *font_features;
    /*< private >*/
    guint padding[2];
  };
#else
  gchar *font_features;
#if (defined(__SIZEOF_INT__) && defined(__SIZEOF_POINTER__)) && (__SIZEOF_INT__ == __SIZEOF_POINTER__)
  /* unusable, just for ABI compat */
  /*< private >*/
  guint padding[1];
#endif
#endif
};

CDK_AVAILABLE_IN_ALL
CtkTextAttributes* ctk_text_attributes_new         (void);
CDK_AVAILABLE_IN_ALL
CtkTextAttributes* ctk_text_attributes_copy        (CtkTextAttributes *src);
CDK_AVAILABLE_IN_ALL
void               ctk_text_attributes_copy_values (CtkTextAttributes *src,
                                                    CtkTextAttributes *dest);
CDK_AVAILABLE_IN_ALL
void               ctk_text_attributes_unref       (CtkTextAttributes *values);
CDK_AVAILABLE_IN_ALL
CtkTextAttributes *ctk_text_attributes_ref         (CtkTextAttributes *values);

CDK_AVAILABLE_IN_ALL
GType              ctk_text_attributes_get_type    (void) G_GNUC_CONST;


G_END_DECLS

#endif

