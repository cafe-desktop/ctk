/* ctktextdisplay.c - display layed-out text
 *
 * Copyright (c) 1992-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 2000 Red Hat, Inc.
 * Tk->Ctk port by Havoc Pennington
 *
 * This file can be used under your choice of two licenses, the LGPL
 * and the original Tk license.
 *
 * LGPL:
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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.Free
 *
 * Original Tk license:
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
/*
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#define CTK_TEXT_USE_INTERNAL_UNSUPPORTED_API
#include "config.h"
#include "ctktextattributesprivate.h"
#include "ctktextdisplay.h"
#include "ctkwidgetprivate.h"
#include "ctkstylecontextprivate.h"
#include "ctkintl.h"

/* DO NOT go putting private headers in here. This file should only
 * use the semi-public headers, as with ctktextview.c.
 */

#define CTK_TYPE_TEXT_RENDERER            (_ctk_text_renderer_get_type())
#define CTK_TEXT_RENDERER(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), CTK_TYPE_TEXT_RENDERER, CtkTextRenderer))
#define CTK_IS_TEXT_RENDERER(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), CTK_TYPE_TEXT_RENDERER))
#define CTK_TEXT_RENDERER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_TEXT_RENDERER, CtkTextRendererClass))
#define CTK_IS_TEXT_RENDERER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_TEXT_RENDERER))
#define CTK_TEXT_RENDERER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_TEXT_RENDERER, CtkTextRendererClass))

typedef struct _CtkTextRenderer      CtkTextRenderer;
typedef struct _CtkTextRendererClass CtkTextRendererClass;

enum {
  NORMAL,
  SELECTED,
  CURSOR
};

struct _CtkTextRenderer
{
  PangoRenderer parent_instance;

  CtkWidget *widget;
  cairo_t *cr;

  CdkRGBA *error_color;	/* Error underline color for this widget */
  GList *widgets;      	/* widgets encountered when drawing */

  guint state : 2;
};

struct _CtkTextRendererClass
{
  PangoRendererClass parent_class;
};

GType _ctk_text_renderer_get_type (void);

G_DEFINE_TYPE (CtkTextRenderer, _ctk_text_renderer, PANGO_TYPE_RENDERER)

static void
text_renderer_set_rgba (CtkTextRenderer *text_renderer,
			PangoRenderPart  part,
			const CdkRGBA   *rgba)
{
  PangoRenderer *renderer = PANGO_RENDERER (text_renderer);
  PangoColor color = { 0, };
  guint16 alpha;

  if (rgba)
    {
      color.red = (guint16)(rgba->red * 65535);
      color.green = (guint16)(rgba->green * 65535);
      color.blue = (guint16)(rgba->blue * 65535);
      alpha = (guint16)(rgba->alpha * 65535);
      pango_renderer_set_color (renderer, part, &color);
      pango_renderer_set_alpha (renderer, part, alpha);
    }
  else
    {
      pango_renderer_set_color (renderer, part, NULL);
      pango_renderer_set_alpha (renderer, part, 0);
    }
}

static CtkTextAppearance *
get_item_appearance (PangoItem *item)
{
  GSList *tmp_list = item->analysis.extra_attrs;

  while (tmp_list)
    {
      PangoAttribute *attr = tmp_list->data;

      if (attr->klass->type == ctk_text_attr_appearance_type)
	return &((CtkTextAttrAppearance *)attr)->appearance;

      tmp_list = tmp_list->next;
    }

  return NULL;
}

extern CtkCssNode *ctk_text_view_get_text_node      (CtkTextView *text_view);
extern CtkCssNode *ctk_text_view_get_selection_node (CtkTextView *text_view);

static void
ctk_text_renderer_prepare_run (PangoRenderer  *renderer,
			       PangoLayoutRun *run)
{
  CtkStyleContext *context;
  CtkTextRenderer *text_renderer = CTK_TEXT_RENDERER (renderer);
  CdkRGBA *bg_rgba = NULL;
  CdkRGBA *fg_rgba = NULL;
  CtkTextAppearance *appearance;

  PANGO_RENDERER_CLASS (_ctk_text_renderer_parent_class)->prepare_run (renderer, run);

  appearance = get_item_appearance (run->item);
  g_assert (appearance != NULL);

  context = ctk_widget_get_style_context (text_renderer->widget);

  if (appearance->draw_bg && text_renderer->state == NORMAL)
    bg_rgba = appearance->rgba[0];
  else
    bg_rgba = NULL;

  text_renderer_set_rgba (text_renderer, PANGO_RENDER_PART_BACKGROUND, bg_rgba);

  if (text_renderer->state == SELECTED)
    {
      CtkCssNode *selection_node;

      selection_node = ctk_text_view_get_selection_node ((CtkTextView *)text_renderer->widget);
      ctk_style_context_save_to_node (context, selection_node);

      ctk_style_context_get (context, ctk_style_context_get_state (context),
                             "color", &fg_rgba,
                             NULL);

      ctk_style_context_restore (context);
    }
  else if (text_renderer->state == CURSOR && ctk_widget_has_focus (text_renderer->widget))
    {
      ctk_style_context_get (context, ctk_style_context_get_state (context),
                             "background-color", &fg_rgba,
                              NULL);
    }
  else
    fg_rgba = appearance->rgba[1];

  text_renderer_set_rgba (text_renderer, PANGO_RENDER_PART_FOREGROUND, fg_rgba);

  if (CTK_TEXT_APPEARANCE_GET_STRIKETHROUGH_RGBA_SET (appearance))
    {
      CdkRGBA rgba;

      CTK_TEXT_APPEARANCE_GET_STRIKETHROUGH_RGBA (appearance, &rgba);
      text_renderer_set_rgba (text_renderer, PANGO_RENDER_PART_STRIKETHROUGH, &rgba);
    }
  else
    text_renderer_set_rgba (text_renderer, PANGO_RENDER_PART_STRIKETHROUGH, fg_rgba);

  if (CTK_TEXT_APPEARANCE_GET_UNDERLINE_RGBA_SET (appearance))
    {
      CdkRGBA rgba;

      CTK_TEXT_APPEARANCE_GET_UNDERLINE_RGBA (appearance, &rgba);
      text_renderer_set_rgba (text_renderer, PANGO_RENDER_PART_UNDERLINE, &rgba);
    }
  else if (appearance->underline == PANGO_UNDERLINE_ERROR)
    {
      if (!text_renderer->error_color)
        {
	  CdkColor *color = NULL;

          ctk_style_context_get_style (context,
                                       "error-underline-color", &color,
                                       NULL);

	  if (color)
	    {
	      CdkRGBA rgba;

	      rgba.red = color->red / 65535.;
	      rgba.green = color->green / 65535.;
	      rgba.blue = color->blue / 65535.;
	      rgba.alpha = 1;
	      cdk_color_free (color);

	      text_renderer->error_color = cdk_rgba_copy (&rgba);
	    }
	  else
	    {
	      static const CdkRGBA red = { 1, 0, 0, 1 };
	      text_renderer->error_color = cdk_rgba_copy (&red);
	    }
        }

      text_renderer_set_rgba (text_renderer, PANGO_RENDER_PART_UNDERLINE, text_renderer->error_color);
    }
  else
    text_renderer_set_rgba (text_renderer, PANGO_RENDER_PART_UNDERLINE, fg_rgba);

  if (fg_rgba != appearance->rgba[1])
    cdk_rgba_free (fg_rgba);
}

static void
set_color (CtkTextRenderer *text_renderer,
           PangoRenderPart  part)
{
  PangoColor *color;
  CdkRGBA rgba;
  guint16 alpha;

  cairo_save (text_renderer->cr);

  color = pango_renderer_get_color (PANGO_RENDERER (text_renderer), part);
  alpha = pango_renderer_get_alpha (PANGO_RENDERER (text_renderer), part);
  if (color)
    {
      rgba.red = color->red / 65535.;
      rgba.green = color->green / 65535.;
      rgba.blue = color->blue / 65535.;
      rgba.alpha = alpha / 65535.;
      cdk_cairo_set_source_rgba (text_renderer->cr, &rgba);
    }
}

static void
unset_color (CtkTextRenderer *text_renderer)
{
  cairo_restore (text_renderer->cr);
}

static void
ctk_text_renderer_draw_glyphs (PangoRenderer     *renderer,
                               PangoFont         *font,
                               PangoGlyphString  *glyphs,
                               int                x,
                               int                y)
{
  CtkTextRenderer *text_renderer = CTK_TEXT_RENDERER (renderer);

  set_color (text_renderer, PANGO_RENDER_PART_FOREGROUND);

  cairo_move_to (text_renderer->cr, (double)x / PANGO_SCALE, (double)y / PANGO_SCALE);
  pango_cairo_show_glyph_string (text_renderer->cr, font, glyphs);

  unset_color (text_renderer);
}

static void
ctk_text_renderer_draw_glyph_item (PangoRenderer     *renderer,
                                   const char        *text,
                                   PangoGlyphItem    *glyph_item,
                                   int                x,
                                   int                y)
{
  CtkTextRenderer *text_renderer = CTK_TEXT_RENDERER (renderer);

  set_color (text_renderer, PANGO_RENDER_PART_FOREGROUND);

  cairo_move_to (text_renderer->cr, (double)x / PANGO_SCALE, (double)y / PANGO_SCALE);
  pango_cairo_show_glyph_item (text_renderer->cr, text, glyph_item);

  unset_color (text_renderer);
}

static void
ctk_text_renderer_draw_rectangle (PangoRenderer     *renderer,
				  PangoRenderPart    part,
				  int                x,
				  int                y,
				  int                width,
				  int                height)
{
  CtkTextRenderer *text_renderer = CTK_TEXT_RENDERER (renderer);

  set_color (text_renderer, part);

  cairo_rectangle (text_renderer->cr,
                   (double)x / PANGO_SCALE, (double)y / PANGO_SCALE,
		   (double)width / PANGO_SCALE, (double)height / PANGO_SCALE);
  cairo_fill (text_renderer->cr);

  unset_color (text_renderer);
}

static void
ctk_text_renderer_draw_trapezoid (PangoRenderer     *renderer,
				  PangoRenderPart    part,
				  double             y1_,
				  double             x11,
				  double             x21,
				  double             y2,
				  double             x12,
				  double             x22)
{
  CtkTextRenderer *text_renderer = CTK_TEXT_RENDERER (renderer);
  cairo_t *cr;
  cairo_matrix_t matrix;

  set_color (text_renderer, part);

  cr = text_renderer->cr;

  cairo_get_matrix (cr, &matrix);
  matrix.xx = matrix.yy = 1.0;
  matrix.xy = matrix.yx = 0.0;
  cairo_set_matrix (cr, &matrix);

  cairo_move_to (cr, x11, y1_);
  cairo_line_to (cr, x21, y1_);
  cairo_line_to (cr, x22, y2);
  cairo_line_to (cr, x12, y2);
  cairo_close_path (cr);

  cairo_fill (cr);

  unset_color (text_renderer);
}

static void
ctk_text_renderer_draw_error_underline (PangoRenderer *renderer,
					int            x,
					int            y,
					int            width,
					int            height)
{
  CtkTextRenderer *text_renderer = CTK_TEXT_RENDERER (renderer);

  set_color (text_renderer, PANGO_RENDER_PART_UNDERLINE);

  pango_cairo_show_error_underline (text_renderer->cr,
                                    (double)x / PANGO_SCALE, (double)y / PANGO_SCALE,
                                    (double)width / PANGO_SCALE, (double)height / PANGO_SCALE);

  unset_color (text_renderer);
}

static void
ctk_text_renderer_draw_shape (PangoRenderer   *renderer,
			      PangoAttrShape  *attr,
			      int              x,
			      int              y)
{
  CtkTextRenderer *text_renderer = CTK_TEXT_RENDERER (renderer);

  if (attr->data == NULL)
    {
      /* This happens if we have an empty widget anchor. Draw
       * something empty-looking.
       */
      CdkRectangle shape_rect;
      cairo_t *cr;

      shape_rect.x = PANGO_PIXELS (x);
      shape_rect.y = PANGO_PIXELS (y + attr->logical_rect.y);
      shape_rect.width = PANGO_PIXELS (x + attr->logical_rect.width) - shape_rect.x;
      shape_rect.height = PANGO_PIXELS (y + attr->logical_rect.y + attr->logical_rect.height) - shape_rect.y;

      set_color (text_renderer, PANGO_RENDER_PART_FOREGROUND);

      cr = text_renderer->cr;

      cairo_set_line_width (cr, 1.0);

      cairo_rectangle (cr,
                       shape_rect.x + 0.5, shape_rect.y + 0.5,
                       shape_rect.width - 1, shape_rect.height - 1);
      cairo_move_to (cr, shape_rect.x + 0.5, shape_rect.y + 0.5);
      cairo_line_to (cr, 
                     shape_rect.x + shape_rect.width - 0.5,
                     shape_rect.y + shape_rect.height - 0.5);
      cairo_move_to (cr, shape_rect.x + 0.5,
                     shape_rect.y + shape_rect.height - 0.5);
      cairo_line_to (cr, shape_rect.x + shape_rect.width - 0.5,
                     shape_rect.y + 0.5);

      cairo_stroke (cr);

      unset_color (text_renderer);
    }
  else if (GDK_IS_PIXBUF (attr->data))
    {
      cairo_t *cr = text_renderer->cr;
      GdkPixbuf *pixbuf = GDK_PIXBUF (attr->data);
      
      cairo_save (cr);

      cdk_cairo_set_source_pixbuf (cr, pixbuf,
                                   PANGO_PIXELS (x),
                                   PANGO_PIXELS (y) -  gdk_pixbuf_get_height (pixbuf));
      cairo_paint (cr);

      cairo_restore (cr);
    }
  else if (CTK_IS_WIDGET (attr->data))
    {
      CtkWidget *widget;
      
      widget = CTK_WIDGET (attr->data);

      text_renderer->widgets = g_list_prepend (text_renderer->widgets,
					       g_object_ref (widget));
    }
  else
    g_assert_not_reached (); /* not a pixbuf or widget */
}

static void
ctk_text_renderer_finalize (GObject *object)
{
  G_OBJECT_CLASS (_ctk_text_renderer_parent_class)->finalize (object);
}

static void
_ctk_text_renderer_init (CtkTextRenderer *renderer G_GNUC_UNUSED)
{
}

static void
_ctk_text_renderer_class_init (CtkTextRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  PangoRendererClass *renderer_class = PANGO_RENDERER_CLASS (klass);
  
  renderer_class->prepare_run = ctk_text_renderer_prepare_run;
  renderer_class->draw_glyphs = ctk_text_renderer_draw_glyphs;
  renderer_class->draw_glyph_item = ctk_text_renderer_draw_glyph_item;
  renderer_class->draw_rectangle = ctk_text_renderer_draw_rectangle;
  renderer_class->draw_trapezoid = ctk_text_renderer_draw_trapezoid;
  renderer_class->draw_error_underline = ctk_text_renderer_draw_error_underline;
  renderer_class->draw_shape = ctk_text_renderer_draw_shape;

  object_class->finalize = ctk_text_renderer_finalize;
}

static void
text_renderer_set_state (CtkTextRenderer *text_renderer,
			 int              state)
{
  text_renderer->state = state;
}

static void
text_renderer_begin (CtkTextRenderer *text_renderer,
                     CtkWidget       *widget,
                     cairo_t         *cr)
{
  CtkStyleContext *context;
  CtkStateFlags state;
  CdkRGBA color;
  CtkCssNode *text_node;

  text_renderer->widget = widget;
  text_renderer->cr = cr;

  context = ctk_widget_get_style_context (widget);

  text_node = ctk_text_view_get_text_node ((CtkTextView *)widget);
  ctk_style_context_save_to_node (context, text_node);

  state = ctk_style_context_get_state (context);
  ctk_style_context_get_color (context, state, &color);

  cairo_save (cr);

  cdk_cairo_set_source_rgba (cr, &color);
}

/* Returns a GSList of (referenced) widgets encountered while drawing.
 */
static GList *
text_renderer_end (CtkTextRenderer *text_renderer)
{
  CtkStyleContext *context;
  GList *widgets = text_renderer->widgets;

  cairo_restore (text_renderer->cr);

  context = ctk_widget_get_style_context (text_renderer->widget);

  ctk_style_context_restore (context);

  text_renderer->widget = NULL;
  text_renderer->cr = NULL;

  text_renderer->widgets = NULL;

  if (text_renderer->error_color)
    {
      cdk_rgba_free (text_renderer->error_color);
      text_renderer->error_color = NULL;
    }

  return widgets;
}

static cairo_region_t *
get_selected_clip (CtkTextRenderer *text_renderer G_GNUC_UNUSED,
                   PangoLayout     *layout G_GNUC_UNUSED,
                   PangoLayoutLine *line,
                   int              x,
                   int              y,
                   int              height,
                   int              start_index,
                   int              end_index)
{
  gint *ranges;
  gint n_ranges, i;
  cairo_region_t *clip_region = cairo_region_create ();

  pango_layout_line_get_x_ranges (line, start_index, end_index, &ranges, &n_ranges);

  for (i=0; i < n_ranges; i++)
    {
      CdkRectangle rect;

      rect.x = x + PANGO_PIXELS (ranges[2*i]);
      rect.y = y;
      rect.width = PANGO_PIXELS (ranges[2*i + 1]) - PANGO_PIXELS (ranges[2*i]);
      rect.height = height;
      
      cairo_region_union_rectangle (clip_region, &rect);
    }

  g_free (ranges);
  return clip_region;
}

static void
render_para (CtkTextRenderer    *text_renderer,
             CtkTextLineDisplay *line_display,
             int                 selection_start_index,
             int                 selection_end_index)
{
  CtkStyleContext *context;
  PangoLayout *layout = line_display->layout;
  int byte_offset = 0;
  PangoLayoutIter *iter;
  int screen_width;
  CdkRGBA selection;
  gboolean first = TRUE;
  CtkCssNode *selection_node;

  iter = pango_layout_get_iter (layout);
  screen_width = line_display->total_width;

  context = ctk_widget_get_style_context (text_renderer->widget);
  selection_node = ctk_text_view_get_selection_node ((CtkTextView*)text_renderer->widget);
  ctk_style_context_save_to_node (context, selection_node);

  ctk_style_context_get_background_color (context, ctk_style_context_get_state (context), &selection);

  ctk_style_context_restore (context);

  do
    {
      PangoLayoutLine *line = pango_layout_iter_get_line_readonly (iter);
      int selection_y, selection_height;
      int first_y, last_y;
      PangoRectangle line_rect;
      int baseline;
      gboolean at_last_line;
      
      pango_layout_iter_get_line_extents (iter, NULL, &line_rect);
      baseline = pango_layout_iter_get_baseline (iter);
      pango_layout_iter_get_line_yrange (iter, &first_y, &last_y);
      
      /* Adjust for margins */

      line_rect.x += line_display->x_offset * PANGO_SCALE;
      line_rect.y += line_display->top_margin * PANGO_SCALE;
      baseline += line_display->top_margin * PANGO_SCALE;

      /* Selection is the height of the line, plus top/bottom
       * margin if we're the first/last line
       */
      selection_y = PANGO_PIXELS (first_y) + line_display->top_margin;
      selection_height = PANGO_PIXELS (last_y) - PANGO_PIXELS (first_y);

      if (first)
        {
          selection_y -= line_display->top_margin;
          selection_height += line_display->top_margin;
        }

      at_last_line = pango_layout_iter_at_last_line (iter);
      if (at_last_line)
        selection_height += line_display->bottom_margin;
      
      first = FALSE;

      if (selection_start_index < byte_offset &&
          selection_end_index > line->length + byte_offset) /* All selected */
        {
          cairo_t *cr = text_renderer->cr;

          cairo_save (cr);
          cdk_cairo_set_source_rgba (cr, &selection);
          cairo_rectangle (cr, 
                           line_display->left_margin, selection_y,
                           screen_width, selection_height);
          cairo_fill (cr);
          cairo_restore(cr);

	  text_renderer_set_state (text_renderer, SELECTED);
	  pango_renderer_draw_layout_line (PANGO_RENDERER (text_renderer),
					   line, 
					   line_rect.x,
					   baseline);
        }
      else
        {
          if (line_display->pg_bg_rgba)
            {
              cairo_t *cr = text_renderer->cr;

              cairo_save (cr);
 
	      cdk_cairo_set_source_rgba (text_renderer->cr, line_display->pg_bg_rgba);
              cairo_rectangle (cr, 
                               line_display->left_margin, selection_y,
                               screen_width, selection_height);
              cairo_fill (cr);

              cairo_restore (cr);
            }
        
	  text_renderer_set_state (text_renderer, NORMAL);
	  pango_renderer_draw_layout_line (PANGO_RENDERER (text_renderer),
					   line, 
					   line_rect.x,
					   baseline);

	  /* Check if some part of the line is selected; the newline
	   * that is after line->length for the last line of the
	   * paragraph counts as part of the line for this
	   */
          if ((selection_start_index < byte_offset + line->length ||
	       (selection_start_index == byte_offset + line->length && pango_layout_iter_at_last_line (iter))) &&
	      selection_end_index > byte_offset)
            {
              cairo_t *cr = text_renderer->cr;
              cairo_region_t *clip_region = get_selected_clip (text_renderer, layout, line,
                                                          line_display->x_offset,
                                                          selection_y,
                                                          selection_height,
                                                          selection_start_index, selection_end_index);

              cairo_save (cr);
              cdk_cairo_region (cr, clip_region);
              cairo_clip (cr);
              cairo_region_destroy (clip_region);

              cdk_cairo_set_source_rgba (cr, &selection);
              cairo_rectangle (cr,
                               PANGO_PIXELS (line_rect.x),
                               selection_y,
                               PANGO_PIXELS (line_rect.width),
                               selection_height);
              cairo_fill (cr);

	      text_renderer_set_state (text_renderer, SELECTED);
	      pango_renderer_draw_layout_line (PANGO_RENDERER (text_renderer),
					       line, 
					       line_rect.x,
					       baseline);

              cairo_restore (cr);

              /* Paint in the ends of the line */
              if (line_rect.x > line_display->left_margin * PANGO_SCALE &&
                  ((line_display->direction == CTK_TEXT_DIR_LTR && selection_start_index < byte_offset) ||
                   (line_display->direction == CTK_TEXT_DIR_RTL && selection_end_index > byte_offset + line->length)))
                {
                  cairo_save (cr);

                  cdk_cairo_set_source_rgba (cr, &selection);
                  cairo_rectangle (cr,
                                   line_display->left_margin,
                                   selection_y,
                                   PANGO_PIXELS (line_rect.x) - line_display->left_margin,
                                   selection_height);
                  cairo_fill (cr);

                  cairo_restore (cr);
                }

              if (line_rect.x + line_rect.width <
                  (screen_width + line_display->left_margin) * PANGO_SCALE &&
                  ((line_display->direction == CTK_TEXT_DIR_LTR && selection_end_index > byte_offset + line->length) ||
                   (line_display->direction == CTK_TEXT_DIR_RTL && selection_start_index < byte_offset)))
                {
                  int nonlayout_width;

                  nonlayout_width =
                    line_display->left_margin + screen_width -
                    PANGO_PIXELS (line_rect.x) - PANGO_PIXELS (line_rect.width);

                  cairo_save (cr);

                  cdk_cairo_set_source_rgba (cr, &selection);
                  cairo_rectangle (cr,
                                   PANGO_PIXELS (line_rect.x) + PANGO_PIXELS (line_rect.width),
                                   selection_y,
                                   nonlayout_width,
                                   selection_height);
                  cairo_fill (cr);

                  cairo_restore (cr);
                }
            }
	  else if (line_display->has_block_cursor &&
		   ctk_widget_has_focus (text_renderer->widget) &&
		   byte_offset <= line_display->insert_index &&
		   (line_display->insert_index < byte_offset + line->length ||
		    (at_last_line && line_display->insert_index == byte_offset + line->length)))
	    {
	      CdkRectangle cursor_rect;
              CdkRGBA cursor_color;
              cairo_t *cr = text_renderer->cr;

              /* we draw text using base color on filled cursor rectangle of cursor color
               * (normally white on black) */
              _ctk_style_context_get_cursor_color (context, &cursor_color, NULL);

	      cursor_rect.x = line_display->x_offset + line_display->block_cursor.x;
	      cursor_rect.y = line_display->block_cursor.y + line_display->top_margin;
	      cursor_rect.width = line_display->block_cursor.width;
	      cursor_rect.height = line_display->block_cursor.height;

              cairo_save (cr);

              cdk_cairo_rectangle (cr, &cursor_rect);
              cairo_clip (cr);

              cdk_cairo_set_source_rgba (cr, &cursor_color);
              cairo_paint (cr);

              /* draw text under the cursor if any */
              if (!line_display->cursor_at_line_end)
                {
                  CdkRGBA color;

                  ctk_style_context_get_background_color (context, ctk_style_context_get_state (context), &color);

                  cdk_cairo_set_source_rgba (cr, &color);

		  text_renderer_set_state (text_renderer, CURSOR);

		  pango_renderer_draw_layout_line (PANGO_RENDERER (text_renderer),
						   line,
						   line_rect.x,
						   baseline);
                }

              cairo_restore (cr);
	    }
        }

      byte_offset += line->length;
    }
  while (pango_layout_iter_next_line (iter));

  pango_layout_iter_free (iter);
}

static CtkTextRenderer *
get_text_renderer (void)
{
  static CtkTextRenderer *text_renderer = NULL;

  if (!text_renderer)
    text_renderer = g_object_new (CTK_TYPE_TEXT_RENDERER, NULL);

  return text_renderer;
}

void
ctk_text_layout_draw (CtkTextLayout *layout,
                      CtkWidget *widget,
                      cairo_t *cr,
                      GList **widgets)
{
  CtkStyleContext *context;
  gint offset_y;
  CtkTextRenderer *text_renderer;
  CtkTextIter selection_start, selection_end;
  gboolean have_selection;
  GSList *line_list;
  GSList *tmp_list;
  GList *tmp_widgets;
  CdkRectangle clip;

  g_return_if_fail (CTK_IS_TEXT_LAYOUT (layout));
  g_return_if_fail (layout->default_style != NULL);
  g_return_if_fail (layout->buffer != NULL);
  g_return_if_fail (cr != NULL);

  if (!cdk_cairo_get_clip_rectangle (cr, &clip))
    return;

  context = ctk_widget_get_style_context (widget);

  line_list = ctk_text_layout_get_lines (layout, clip.y, clip.y + clip.height, &offset_y);

  if (line_list == NULL)
    return; /* nothing on the screen */

  text_renderer = get_text_renderer ();
  text_renderer_begin (text_renderer, widget, cr);

  /* text_renderer_begin/end does cairo_save/restore */
  cairo_translate (cr, 0, offset_y);

  ctk_text_layout_wrap_loop_start (layout);

  have_selection = ctk_text_buffer_get_selection_bounds (layout->buffer,
                                                         &selection_start,
                                                         &selection_end);

  tmp_list = line_list;
  while (tmp_list != NULL)
    {
      CtkTextLineDisplay *line_display;
      gint selection_start_index = -1;
      gint selection_end_index = -1;

      CtkTextLine *line = tmp_list->data;

      line_display = ctk_text_layout_get_line_display (layout, line, FALSE);

      if (line_display->height > 0)
        {
          g_assert (line_display->layout != NULL);
          
          if (have_selection)
            {
              CtkTextIter line_start, line_end;
              gint byte_count;
              
              ctk_text_layout_get_iter_at_line (layout,
                                                &line_start,
                                                line, 0);
              line_end = line_start;
	      if (!ctk_text_iter_ends_line (&line_end))
		ctk_text_iter_forward_to_line_end (&line_end);
              byte_count = ctk_text_iter_get_visible_line_index (&line_end);     

              if (ctk_text_iter_compare (&selection_start, &line_end) <= 0 &&
                  ctk_text_iter_compare (&selection_end, &line_start) >= 0)
                {
                  if (ctk_text_iter_compare (&selection_start, &line_start) >= 0)
                    selection_start_index = ctk_text_iter_get_visible_line_index (&selection_start);
                  else
                    selection_start_index = -1;

                  if (ctk_text_iter_compare (&selection_end, &line_end) <= 0)
                    selection_end_index = ctk_text_iter_get_visible_line_index (&selection_end);
                  else
                    selection_end_index = byte_count + 1; /* + 1 to flag past-the-end */
                }
            }

          render_para (text_renderer, line_display,
                       selection_start_index, selection_end_index);

          /* We paint the cursors last, because they overlap another chunk
           * and need to appear on top.
           */
          if (line_display->cursors != NULL)
            {
              int i;

              for (i = 0; i < line_display->cursors->len; i++)
                {
                  int index;
                  PangoDirection dir;

                  index = g_array_index(line_display->cursors, int, i);
                  dir = (line_display->direction == CTK_TEXT_DIR_RTL) ? PANGO_DIRECTION_RTL : PANGO_DIRECTION_LTR;
                  ctk_render_insertion_cursor (context, cr,
                                               line_display->x_offset, line_display->top_margin,
                                               line_display->layout, index, dir);
                }
            }
        } /* line_display->height > 0 */

      cairo_translate (cr, 0, line_display->height);
      ctk_text_layout_free_line_display (layout, line_display);
      
      tmp_list = tmp_list->next;
    }

  ctk_text_layout_wrap_loop_end (layout);

  tmp_widgets = text_renderer_end (text_renderer);
  if (widgets)
    *widgets = tmp_widgets;
  else
    g_list_free_full (tmp_widgets, g_object_unref);

  g_slist_free (line_list);
}
