/* CTK - The GIMP Toolkit
 * ctktextlayout.h
 *
 * Copyright (c) 1992-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 2000 Red Hat, Inc.
 * Tk->Ctk port by Havoc Pennington
 * Pango support by Owen Taylor
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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
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

#ifndef __CTK_TEXT_LAYOUT_H__
#define __CTK_TEXT_LAYOUT_H__

/* This is a "semi-private" header; it is intended for
 * use by the text widget, and the text canvas item,
 * but that’s all. We may have to install it so the
 * canvas item can use it, but users are not supposed
 * to use it.
 */
#ifndef CTK_TEXT_USE_INTERNAL_UNSUPPORTED_API
#error "You are not supposed to be including this file; the equivalent public API is in ctktextview.h"
#endif

#include <ctk/ctk.h>

G_BEGIN_DECLS

/* forward declarations that have to be here to avoid including
 * ctktextbtree.h
 */
typedef struct _CtkTextLine     CtkTextLine;
typedef struct _CtkTextLineData CtkTextLineData;

#define CTK_TYPE_TEXT_LAYOUT             (ctk_text_layout_get_type ())
#define CTK_TEXT_LAYOUT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_TEXT_LAYOUT, CtkTextLayout))
#define CTK_TEXT_LAYOUT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_TEXT_LAYOUT, CtkTextLayoutClass))
#define CTK_IS_TEXT_LAYOUT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_TEXT_LAYOUT))
#define CTK_IS_TEXT_LAYOUT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_TEXT_LAYOUT))
#define CTK_TEXT_LAYOUT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_TEXT_LAYOUT, CtkTextLayoutClass))

typedef struct _CtkTextLayout         CtkTextLayout;
typedef struct _CtkTextLayoutClass    CtkTextLayoutClass;
typedef struct _CtkTextLineDisplay    CtkTextLineDisplay;
typedef struct _CtkTextAttrAppearance CtkTextAttrAppearance;

struct _CtkTextLayout
{
  GObject parent_instance;

  /* width of the display area on-screen,
   * i.e. pixels we should wrap to fit inside. */
  gint screen_width;

  /* width/height of the total logical area being layed out */
  gint width;
  gint height;

  /* Pixel offsets from the left and from the top to be used when we
   * draw; these allow us to create left/top margins. We don't need
   * anything special for bottom/right margins, because those don't
   * affect drawing.
   */
  /* gint left_edge; */
  /* gint top_edge; */

  CtkTextBuffer *buffer;

  gint left_padding;
  gint right_padding;

  /* Default style used if no tags override it */
  CtkTextAttributes *default_style;

  /* Pango contexts used for creating layouts */
  PangoContext *ltr_context;
  PangoContext *rtl_context;

  /* A cache of one style; this is used to ensure
   * we don't constantly regenerate the style
   * over long runs with the same style. */
  CtkTextAttributes *one_style_cache;

  /* A cache of one line display. Getting the same line
   * many times in a row is the most common case.
   */
  CtkTextLineDisplay *one_display_cache;

  /* Whether we are allowed to wrap right now */
  gint wrap_loop_count;
  
  /* Whether to show the insertion cursor */
  guint cursor_visible : 1;

  /* For what CtkTextDirection to draw cursor CTK_TEXT_DIR_NONE -
   * means draw both cursors.
   */
  guint cursor_direction : 2;

  /* The keyboard direction is used to default the alignment when
     there are no strong characters.
  */
  guint keyboard_direction : 2;

  /* The preedit string and attributes, if any */

  gchar *preedit_string;
  PangoAttrList *preedit_attrs;
  gint preedit_len;
  gint preedit_cursor;

  guint overwrite_mode : 1;
};

struct _CtkTextLayoutClass
{
  GObjectClass parent_class;

  /* Some portion of the layout was invalidated
   */
  void  (*invalidated)  (CtkTextLayout *layout);

  /* A range of the layout changed appearance and possibly height
   */
  void  (*changed)              (CtkTextLayout     *layout,
                                 gint               y,
                                 gint               old_height,
                                 gint               new_height);
  CtkTextLineData* (*wrap)      (CtkTextLayout     *layout,
                                 CtkTextLine       *line,
                                 CtkTextLineData   *line_data); /* may be NULL */
  void  (*get_log_attrs)        (CtkTextLayout     *layout,
                                 CtkTextLine       *line,
                                 PangoLogAttr     **attrs,
                                 gint              *n_attrs);
  void  (*invalidate)           (CtkTextLayout     *layout,
                                 const CtkTextIter *start,
                                 const CtkTextIter *end);
  void  (*free_line_data)       (CtkTextLayout     *layout,
                                 CtkTextLine       *line,
                                 CtkTextLineData   *line_data);

  void (*allocate_child)        (CtkTextLayout     *layout,
                                 CtkWidget         *child,
                                 gint               x,
                                 gint               y);

  void (*invalidate_cursors)    (CtkTextLayout     *layout,
                                 const CtkTextIter *start,
                                 const CtkTextIter *end);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
};

struct _CtkTextAttrAppearance
{
  PangoAttribute attr;
  CtkTextAppearance appearance;
};

struct _CtkTextLineDisplay
{
  PangoLayout *layout;
  GArray *cursors;      /* indexes of cursors in the PangoLayout */

  CtkTextDirection direction;

  gint width;                   /* Width of layout */
  gint total_width;             /* width - margins, if no width set on layout, if width set on layout, -1 */
  gint height;
  /* Amount layout is shifted from left edge - this is the left margin
   * plus any other factors, such as alignment or indentation.
   */
  gint x_offset;
  gint left_margin;
  gint right_margin;
  gint top_margin;
  gint bottom_margin;
  gint insert_index;		/* Byte index of insert cursor within para or -1 */

  CtkTextLine *line;
  
  CdkColor *pg_bg_color;

  CdkRectangle block_cursor;
  guint cursors_invalid : 1;
  guint has_block_cursor : 1;
  guint cursor_at_line_end : 1;
  guint size_only : 1;

  CdkRGBA *pg_bg_rgba;
};

#ifdef CTK_COMPILATION
extern G_GNUC_INTERNAL PangoAttrType ctk_text_attr_appearance_type;
#endif

GDK_AVAILABLE_IN_ALL
GType         ctk_text_layout_get_type    (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
CtkTextLayout*     ctk_text_layout_new                   (void);
GDK_AVAILABLE_IN_ALL
void               ctk_text_layout_set_buffer            (CtkTextLayout     *layout,
							  CtkTextBuffer     *buffer);
GDK_AVAILABLE_IN_ALL
CtkTextBuffer     *ctk_text_layout_get_buffer            (CtkTextLayout     *layout);
GDK_AVAILABLE_IN_ALL
void               ctk_text_layout_set_default_style     (CtkTextLayout     *layout,
							  CtkTextAttributes *values);
GDK_AVAILABLE_IN_ALL
void               ctk_text_layout_set_contexts          (CtkTextLayout     *layout,
							  PangoContext      *ltr_context,
							  PangoContext      *rtl_context);
GDK_AVAILABLE_IN_ALL
void               ctk_text_layout_set_cursor_direction  (CtkTextLayout     *layout,
                                                          CtkTextDirection   direction);
GDK_AVAILABLE_IN_ALL
void		   ctk_text_layout_set_overwrite_mode	 (CtkTextLayout     *layout,
							  gboolean           overwrite);
GDK_AVAILABLE_IN_ALL
void               ctk_text_layout_set_keyboard_direction (CtkTextLayout     *layout,
							   CtkTextDirection keyboard_dir);
GDK_AVAILABLE_IN_ALL
void               ctk_text_layout_default_style_changed (CtkTextLayout     *layout);

GDK_AVAILABLE_IN_ALL
void ctk_text_layout_set_screen_width       (CtkTextLayout     *layout,
                                             gint               width);
GDK_AVAILABLE_IN_ALL
void ctk_text_layout_set_preedit_string     (CtkTextLayout     *layout,
 					     const gchar       *preedit_string,
 					     PangoAttrList     *preedit_attrs,
 					     gint               cursor_pos);

GDK_AVAILABLE_IN_ALL
void     ctk_text_layout_set_cursor_visible (CtkTextLayout     *layout,
                                             gboolean           cursor_visible);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_layout_get_cursor_visible (CtkTextLayout     *layout);

/* Getting the size or the lines potentially results in a call to
 * recompute, which is pretty massively expensive. Thus it should
 * basically only be done in an idle handler.
 *
 * Long-term, we would really like to be able to do these without
 * a full recompute so they may get cheaper over time.
 */
GDK_AVAILABLE_IN_ALL
void    ctk_text_layout_get_size  (CtkTextLayout  *layout,
                                   gint           *width,
                                   gint           *height);
GDK_AVAILABLE_IN_ALL
GSList* ctk_text_layout_get_lines (CtkTextLayout  *layout,
                                   /* [top_y, bottom_y) */
                                   gint            top_y,
                                   gint            bottom_y,
                                   gint           *first_line_y);

GDK_AVAILABLE_IN_ALL
void ctk_text_layout_wrap_loop_start (CtkTextLayout *layout);
GDK_AVAILABLE_IN_ALL
void ctk_text_layout_wrap_loop_end   (CtkTextLayout *layout);

GDK_AVAILABLE_IN_ALL
CtkTextLineDisplay* ctk_text_layout_get_line_display  (CtkTextLayout      *layout,
                                                       CtkTextLine        *line,
                                                       gboolean            size_only);
GDK_AVAILABLE_IN_ALL
void                ctk_text_layout_free_line_display (CtkTextLayout      *layout,
                                                       CtkTextLineDisplay *display);

GDK_AVAILABLE_IN_ALL
void ctk_text_layout_get_line_at_y     (CtkTextLayout     *layout,
                                        CtkTextIter       *target_iter,
                                        gint               y,
                                        gint              *line_top);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_layout_get_iter_at_pixel (CtkTextLayout     *layout,
                                            CtkTextIter       *iter,
                                            gint               x,
                                            gint               y);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_layout_get_iter_at_position (CtkTextLayout     *layout,
                                               CtkTextIter       *iter,
                                               gint              *trailing,
                                               gint               x,
                                               gint               y);
GDK_AVAILABLE_IN_ALL
void ctk_text_layout_invalidate        (CtkTextLayout     *layout,
                                        const CtkTextIter *start,
                                        const CtkTextIter *end);
GDK_AVAILABLE_IN_ALL
void ctk_text_layout_invalidate_cursors(CtkTextLayout     *layout,
                                        const CtkTextIter *start,
                                        const CtkTextIter *end);
GDK_AVAILABLE_IN_ALL
void ctk_text_layout_free_line_data    (CtkTextLayout     *layout,
                                        CtkTextLine       *line,
                                        CtkTextLineData   *line_data);

GDK_AVAILABLE_IN_ALL
gboolean ctk_text_layout_is_valid        (CtkTextLayout *layout);
GDK_AVAILABLE_IN_ALL
void     ctk_text_layout_validate_yrange (CtkTextLayout *layout,
                                          CtkTextIter   *anchor_line,
                                          gint           y0_,
                                          gint           y1_);
GDK_AVAILABLE_IN_ALL
void     ctk_text_layout_validate        (CtkTextLayout *layout,
                                          gint           max_pixels);

/* This function should return the passed-in line data,
 * OR remove the existing line data from the line, and
 * return a NEW line data after adding it to the line.
 * That is, invariant after calling the callback is that
 * there should be exactly one line data for this view
 * stored on the btree line.
 */
GDK_AVAILABLE_IN_ALL
CtkTextLineData* ctk_text_layout_wrap  (CtkTextLayout   *layout,
                                        CtkTextLine     *line,
                                        CtkTextLineData *line_data); /* may be NULL */
GDK_AVAILABLE_IN_ALL
void     ctk_text_layout_changed              (CtkTextLayout     *layout,
                                               gint               y,
                                               gint               old_height,
                                               gint               new_height);
GDK_AVAILABLE_IN_ALL
void     ctk_text_layout_cursors_changed      (CtkTextLayout     *layout,
                                               gint               y,
                                               gint               old_height,
                                               gint               new_height);
GDK_AVAILABLE_IN_ALL
void     ctk_text_layout_get_iter_location    (CtkTextLayout     *layout,
                                               const CtkTextIter *iter,
                                               CdkRectangle      *rect);
GDK_AVAILABLE_IN_ALL
void     ctk_text_layout_get_line_yrange      (CtkTextLayout     *layout,
                                               const CtkTextIter *iter,
                                               gint              *y,
                                               gint              *height);
GDK_AVAILABLE_IN_ALL
void     ctk_text_layout_get_cursor_locations (CtkTextLayout     *layout,
                                               CtkTextIter       *iter,
                                               CdkRectangle      *strong_pos,
                                               CdkRectangle      *weak_pos);
gboolean _ctk_text_layout_get_block_cursor    (CtkTextLayout     *layout,
					       CdkRectangle      *pos);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_layout_clamp_iter_to_vrange (CtkTextLayout     *layout,
                                               CtkTextIter       *iter,
                                               gint               top,
                                               gint               bottom);

GDK_AVAILABLE_IN_ALL
gboolean ctk_text_layout_move_iter_to_line_end      (CtkTextLayout *layout,
                                                     CtkTextIter   *iter,
                                                     gint           direction);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_layout_move_iter_to_previous_line (CtkTextLayout *layout,
                                                     CtkTextIter   *iter);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_layout_move_iter_to_next_line     (CtkTextLayout *layout,
                                                     CtkTextIter   *iter);
GDK_AVAILABLE_IN_ALL
void     ctk_text_layout_move_iter_to_x             (CtkTextLayout *layout,
                                                     CtkTextIter   *iter,
                                                     gint           x);
GDK_AVAILABLE_IN_ALL
gboolean ctk_text_layout_move_iter_visually         (CtkTextLayout *layout,
                                                     CtkTextIter   *iter,
                                                     gint           count);

GDK_AVAILABLE_IN_ALL
gboolean ctk_text_layout_iter_starts_line           (CtkTextLayout       *layout,
                                                     const CtkTextIter   *iter);

GDK_AVAILABLE_IN_ALL
void     ctk_text_layout_get_iter_at_line           (CtkTextLayout *layout,
                                                     CtkTextIter    *iter,
                                                     CtkTextLine    *line,
                                                     gint            byte_offset);

/* Don't use these. Use ctk_text_view_add_child_at_anchor().
 * These functions are defined in ctktextchild.c, but here
 * since they are semi-public and require CtkTextLayout to
 * be declared.
 */
GDK_AVAILABLE_IN_ALL
void ctk_text_child_anchor_register_child   (CtkTextChildAnchor *anchor,
                                             CtkWidget          *child,
                                             CtkTextLayout      *layout);
GDK_AVAILABLE_IN_ALL
void ctk_text_child_anchor_unregister_child (CtkTextChildAnchor *anchor,
                                             CtkWidget          *child);

GDK_AVAILABLE_IN_ALL
void ctk_text_child_anchor_queue_resize     (CtkTextChildAnchor *anchor,
                                             CtkTextLayout      *layout);

GDK_AVAILABLE_IN_ALL
void ctk_text_anchored_child_set_layout     (CtkWidget          *child,
                                             CtkTextLayout      *layout);

GDK_AVAILABLE_IN_ALL
void ctk_text_layout_spew (CtkTextLayout *layout);

G_END_DECLS

#endif  /* __CTK_TEXT_LAYOUT_H__ */
