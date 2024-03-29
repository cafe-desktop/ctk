/* ctktextattributes.c - text attributes
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

#include "config.h"

#include "ctktextattributes.h"
#include "ctktextattributesprivate.h"
#include "ctktexttagprivate.h"

/**
 * ctk_text_attributes_new:
 * 
 * Creates a #CtkTextAttributes, which describes
 * a set of properties on some text.
 * 
 * Returns: a new #CtkTextAttributes,
 *     free with ctk_text_attributes_unref().
 */
CtkTextAttributes*
ctk_text_attributes_new (void)
{
  CtkTextAttributes *values;

  values = g_slice_new0 (CtkTextAttributes);

  /* 0 is a valid value for most of the struct */
  values->refcount = 1;

  values->language = ctk_get_default_language ();

  values->font_scale = 1.0;

  values->editable = TRUE;

  return values;
}

/**
 * ctk_text_attributes_copy:
 * @src: a #CtkTextAttributes to be copied
 *
 * Copies @src and returns a new #CtkTextAttributes.
 *
 * Returns: a copy of @src,
 *     free with ctk_text_attributes_unref()
 */
CtkTextAttributes*
ctk_text_attributes_copy (CtkTextAttributes *src)
{
  CtkTextAttributes *dest;

  dest = ctk_text_attributes_new ();
  ctk_text_attributes_copy_values (src, dest);

  return dest;
}

G_DEFINE_BOXED_TYPE (CtkTextAttributes, ctk_text_attributes,
                     ctk_text_attributes_ref,
                     ctk_text_attributes_unref)

/**
 * ctk_text_attributes_copy_values:
 * @src: a #CtkTextAttributes
 * @dest: another #CtkTextAttributes
 *
 * Copies the values from @src to @dest so that @dest has
 * the same values as @src. Frees existing values in @dest.
 */
void
ctk_text_attributes_copy_values (CtkTextAttributes *src,
                                 CtkTextAttributes *dest)
{
  guint orig_refcount;

  if (src == dest)
    return;

  /* Remove refs */
  if (dest->tabs)
    pango_tab_array_free (dest->tabs);

  if (dest->font)
    pango_font_description_free (dest->font);

  if (dest->pg_bg_color)
    cdk_color_free (dest->pg_bg_color);

  if (dest->pg_bg_rgba)
    cdk_rgba_free (dest->pg_bg_rgba);

  if (dest->appearance.rgba[0])
    cdk_rgba_free (dest->appearance.rgba[0]);

  if (dest->appearance.rgba[1])
    cdk_rgba_free (dest->appearance.rgba[1]);

  if (dest->font_features)
    g_free (dest->font_features);

  /* Copy */
  orig_refcount = dest->refcount;

  *dest = *src;

  if (src->tabs)
    dest->tabs = pango_tab_array_copy (src->tabs);

  dest->language = src->language;

  if (src->font)
    dest->font = pango_font_description_copy (src->font);

  if (src->pg_bg_color)
    dest->pg_bg_color = cdk_color_copy (src->pg_bg_color);

  if (src->pg_bg_rgba)
    dest->pg_bg_rgba = cdk_rgba_copy (src->pg_bg_rgba);

  if (src->appearance.rgba[0])
    dest->appearance.rgba[0] = cdk_rgba_copy (src->appearance.rgba[0]);

  if (src->appearance.rgba[1])
    dest->appearance.rgba[1] = cdk_rgba_copy (src->appearance.rgba[1]);

  if (src->font_features)
    dest->font_features = g_strdup (src->font_features);

  dest->refcount = orig_refcount;
}

/**
 * ctk_text_attributes_ref:
 * @values: a #CtkTextAttributes
 *
 * Increments the reference count on @values.
 *
 * Returns: the #CtkTextAttributes that were passed in
 **/
CtkTextAttributes *
ctk_text_attributes_ref (CtkTextAttributes *values)
{
  g_return_val_if_fail (values != NULL, NULL);

  values->refcount += 1;

  return values;
}

/**
 * ctk_text_attributes_unref:
 * @values: a #CtkTextAttributes
 * 
 * Decrements the reference count on @values, freeing the structure
 * if the reference count reaches 0.
 **/
void
ctk_text_attributes_unref (CtkTextAttributes *values)
{
  g_return_if_fail (values != NULL);
  g_return_if_fail (values->refcount > 0);

  values->refcount -= 1;

  if (values->refcount == 0)
    {
      if (values->tabs)
        pango_tab_array_free (values->tabs);

      if (values->font)
	pango_font_description_free (values->font);

      if (values->pg_bg_color)
	cdk_color_free (values->pg_bg_color);

      if (values->pg_bg_rgba)
	cdk_rgba_free (values->pg_bg_rgba);

      if (values->appearance.rgba[0])
	cdk_rgba_free (values->appearance.rgba[0]);

      if (values->appearance.rgba[1])
	cdk_rgba_free (values->appearance.rgba[1]);

      if (values->font_features)
        g_free (values->font_features);

      g_slice_free (CtkTextAttributes, values);
    }
}

void
_ctk_text_attributes_fill_from_tags (CtkTextAttributes *dest,
                                     CtkTextTag**       tags,
                                     guint              n_tags)
{
  guint n = 0;

  guint left_margin_accumulative = 0;
  guint right_margin_accumulative = 0;

  while (n < n_tags)
    {
      CtkTextTag *tag = tags[n];
      CtkTextAttributes *vals = tag->priv->values;

      g_assert (tag->priv->table != NULL);
      if (n > 0)
        g_assert (tags[n]->priv->priority > tags[n-1]->priv->priority);

      if (tag->priv->bg_color_set)
        {
	  if (dest->appearance.rgba[0])
	    {
	      cdk_rgba_free (dest->appearance.rgba[0]);
	      dest->appearance.rgba[0] = NULL;
	    }

	  if (vals->appearance.rgba[0])
	    dest->appearance.rgba[0] = cdk_rgba_copy (vals->appearance.rgba[0]);

          dest->appearance.draw_bg = TRUE;
        }

      if (tag->priv->fg_color_set)
	{
	  if (dest->appearance.rgba[1])
	    {
	      cdk_rgba_free (dest->appearance.rgba[1]);
	      dest->appearance.rgba[1] = NULL;
	    }

	  if (vals->appearance.rgba[1])
	    dest->appearance.rgba[1] = cdk_rgba_copy (vals->appearance.rgba[1]);
	}

      if (tag->priv->pg_bg_color_set)
        {
	  if (dest->pg_bg_rgba)
	    {
	      cdk_rgba_free (dest->pg_bg_rgba);
	      dest->pg_bg_rgba = NULL;
	    }

	  if (dest->pg_bg_color)
	    {
	      cdk_color_free (dest->pg_bg_color);
	      dest->pg_bg_color = NULL;
	    }

	  if (vals->pg_bg_rgba)
	    dest->pg_bg_rgba = cdk_rgba_copy (vals->pg_bg_rgba);

	  if (vals->pg_bg_color)
	    dest->pg_bg_color = cdk_color_copy (vals->pg_bg_color);
        }

      if (vals->font)
	{
	  if (dest->font)
	    pango_font_description_merge (dest->font, vals->font, TRUE);
	  else
	    dest->font = pango_font_description_copy (vals->font);
	}

      /* multiply all the scales together to get a composite */
      if (tag->priv->scale_set)
        dest->font_scale *= vals->font_scale;

      if (tag->priv->justification_set)
        dest->justification = vals->justification;

      if (vals->direction != CTK_TEXT_DIR_NONE)
        dest->direction = vals->direction;

      if (tag->priv->left_margin_set)
        {
          if (tag->priv->accumulative_margin)
            left_margin_accumulative += vals->left_margin;
          else
            dest->left_margin = vals->left_margin;
        }

      if (tag->priv->indent_set)
        dest->indent = vals->indent;

      if (tag->priv->rise_set)
        dest->appearance.rise = vals->appearance.rise;

      if (tag->priv->right_margin_set)
        {
          if (tag->priv->accumulative_margin)
            right_margin_accumulative += vals->right_margin;
          else
            dest->right_margin = vals->right_margin;
        }

      if (tag->priv->pixels_above_lines_set)
        dest->pixels_above_lines = vals->pixels_above_lines;

      if (tag->priv->pixels_below_lines_set)
        dest->pixels_below_lines = vals->pixels_below_lines;

      if (tag->priv->pixels_inside_wrap_set)
        dest->pixels_inside_wrap = vals->pixels_inside_wrap;

      if (tag->priv->tabs_set)
        {
          if (dest->tabs)
            pango_tab_array_free (dest->tabs);
          dest->tabs = pango_tab_array_copy (vals->tabs);
        }

      if (tag->priv->wrap_mode_set)
        dest->wrap_mode = vals->wrap_mode;

      if (tag->priv->underline_set)
        dest->appearance.underline = vals->appearance.underline;

      if (CTK_TEXT_APPEARANCE_GET_UNDERLINE_RGBA_SET (&vals->appearance))
        {
          CdkRGBA rgba;

          CTK_TEXT_APPEARANCE_GET_UNDERLINE_RGBA (&vals->appearance, &rgba);
          CTK_TEXT_APPEARANCE_SET_UNDERLINE_RGBA (&dest->appearance, &rgba);
          CTK_TEXT_APPEARANCE_SET_UNDERLINE_RGBA_SET (&dest->appearance, TRUE);
        }

      if (tag->priv->strikethrough_set)
        dest->appearance.strikethrough = vals->appearance.strikethrough;

      if (CTK_TEXT_APPEARANCE_GET_STRIKETHROUGH_RGBA_SET (&vals->appearance))
        {
          CdkRGBA rgba;

          CTK_TEXT_APPEARANCE_GET_STRIKETHROUGH_RGBA (&vals->appearance, &rgba);
          CTK_TEXT_APPEARANCE_SET_STRIKETHROUGH_RGBA (&dest->appearance, &rgba);
          CTK_TEXT_APPEARANCE_SET_STRIKETHROUGH_RGBA_SET (&dest->appearance, TRUE);
        }

      if (tag->priv->invisible_set)
        dest->invisible = vals->invisible;

      if (tag->priv->editable_set)
        dest->editable = vals->editable;

      if (tag->priv->bg_full_height_set)
        dest->bg_full_height = vals->bg_full_height;

      if (tag->priv->language_set)
	dest->language = vals->language;

      if (tag->priv->fallback_set)
        dest->no_fallback = vals->no_fallback;

      if (tag->priv->letter_spacing_set)
        dest->letter_spacing = vals->letter_spacing;

      if (tag->priv->font_features_set)
        dest->font_features = g_strdup (vals->font_features);

      ++n;
    }

  dest->left_margin += left_margin_accumulative;
  dest->right_margin += right_margin_accumulative;
}

gboolean
_ctk_text_tag_affects_size (CtkTextTag *tag)
{
  CtkTextTagPrivate *priv = tag->priv;

  return
    (priv->values->font && pango_font_description_get_set_fields (priv->values->font) != 0) ||
    priv->scale_set ||
    priv->justification_set ||
    priv->left_margin_set ||
    priv->indent_set ||
    priv->rise_set ||
    priv->right_margin_set ||
    priv->pixels_above_lines_set ||
    priv->pixels_below_lines_set ||
    priv->pixels_inside_wrap_set ||
    priv->tabs_set ||
    priv->underline_set ||
    priv->wrap_mode_set ||
    priv->invisible_set ||
    priv->font_features_set ||
    priv->letter_spacing_set;
}

gboolean
_ctk_text_tag_affects_nonsize_appearance (CtkTextTag *tag)
{
  CtkTextTagPrivate *priv = tag->priv;

  return
    priv->bg_color_set ||
    priv->fg_color_set ||
    priv->strikethrough_set ||
    priv->bg_full_height_set ||
    priv->pg_bg_color_set ||
    priv->fallback_set ||
    CTK_TEXT_APPEARANCE_GET_UNDERLINE_RGBA_SET (&priv->values->appearance) ||
    CTK_TEXT_APPEARANCE_GET_STRIKETHROUGH_RGBA_SET (&priv->values->appearance);
}
