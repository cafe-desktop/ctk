/* CTK - The GIMP Toolkit
 * ctktextview.h Copyright (C) 2000 Red Hat, Inc.
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

/*
 * Modified by the CTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the CTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * CTK+ at ftp://ftp.ctk.org/pub/ctk/.
 */

#ifndef __CTK_TEXT_VIEW_H__
#define __CTK_TEXT_VIEW_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <ctk/ctkcontainer.h>
#include <ctk/ctkimcontext.h>
#include <ctk/ctktextbuffer.h>
#include <ctk/ctkmenu.h>

G_BEGIN_DECLS

#define CTK_TYPE_TEXT_VIEW             (ctk_text_view_get_type ())
#define CTK_TEXT_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_TEXT_VIEW, CtkTextView))
#define CTK_TEXT_VIEW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_TEXT_VIEW, CtkTextViewClass))
#define CTK_IS_TEXT_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_TEXT_VIEW))
#define CTK_IS_TEXT_VIEW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_TEXT_VIEW))
#define CTK_TEXT_VIEW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_TEXT_VIEW, CtkTextViewClass))

/**
 * CtkTextWindowType:
 * @CTK_TEXT_WINDOW_PRIVATE: Invalid value, used as a marker
 * @CTK_TEXT_WINDOW_WIDGET: Window that floats over scrolling areas.
 * @CTK_TEXT_WINDOW_TEXT: Scrollable text window.
 * @CTK_TEXT_WINDOW_LEFT: Left side border window.
 * @CTK_TEXT_WINDOW_RIGHT: Right side border window.
 * @CTK_TEXT_WINDOW_TOP: Top border window.
 * @CTK_TEXT_WINDOW_BOTTOM: Bottom border window.
 *
 * Used to reference the parts of #CtkTextView.
 */
typedef enum
{
  CTK_TEXT_WINDOW_PRIVATE,
  CTK_TEXT_WINDOW_WIDGET,
  CTK_TEXT_WINDOW_TEXT,
  CTK_TEXT_WINDOW_LEFT,
  CTK_TEXT_WINDOW_RIGHT,
  CTK_TEXT_WINDOW_TOP,
  CTK_TEXT_WINDOW_BOTTOM
} CtkTextWindowType;

/**
 * CtkTextViewLayer:
 * @CTK_TEXT_VIEW_LAYER_BELOW: Old deprecated layer, use %CTK_TEXT_VIEW_LAYER_BELOW_TEXT instead
 * @CTK_TEXT_VIEW_LAYER_ABOVE: Old deprecated layer, use %CTK_TEXT_VIEW_LAYER_ABOVE_TEXT instead
 * @CTK_TEXT_VIEW_LAYER_BELOW_TEXT: The layer rendered below the text (but above the background).  Since: 3.20
 * @CTK_TEXT_VIEW_LAYER_ABOVE_TEXT: The layer rendered above the text.  Since: 3.20
 *
 * Used to reference the layers of #CtkTextView for the purpose of customized
 * drawing with the ::draw_layer vfunc.
 */
typedef enum
{
  CTK_TEXT_VIEW_LAYER_BELOW,
  CTK_TEXT_VIEW_LAYER_ABOVE,
  CTK_TEXT_VIEW_LAYER_BELOW_TEXT,
  CTK_TEXT_VIEW_LAYER_ABOVE_TEXT
} CtkTextViewLayer;

/**
 * CtkTextExtendSelection:
 * @CTK_TEXT_EXTEND_SELECTION_WORD: Selects the current word. It is triggered by
 *   a double-click for example.
 * @CTK_TEXT_EXTEND_SELECTION_LINE: Selects the current line. It is triggered by
 *   a triple-click for example.
 *
 * Granularity types that extend the text selection. Use the
 * #CtkTextView::extend-selection signal to customize the selection.
 *
 * Since: 3.16
 */
typedef enum
{
  CTK_TEXT_EXTEND_SELECTION_WORD,
  CTK_TEXT_EXTEND_SELECTION_LINE
} CtkTextExtendSelection;

/**
 * CTK_TEXT_VIEW_PRIORITY_VALIDATE: (value 125)
 *
 * The priority at which the text view validates onscreen lines
 * in an idle job in the background.
 */
#define CTK_TEXT_VIEW_PRIORITY_VALIDATE (CDK_PRIORITY_REDRAW + 5)

typedef struct _CtkTextView        CtkTextView;
typedef struct _CtkTextViewPrivate CtkTextViewPrivate;
typedef struct _CtkTextViewClass   CtkTextViewClass;

struct _CtkTextView
{
  CtkContainer parent_instance;

  /*< private >*/

  CtkTextViewPrivate *priv;
};

/**
 * CtkTextViewClass:
 * @parent_class: The object class structure needs to be the first
 * @populate_popup: The class handler for the #CtkTextView::populate-popup
 *   signal.
 * @move_cursor: The class handler for the #CtkTextView::move-cursor
 *   keybinding signal.
 * @set_anchor: The class handler for the #CtkTextView::set-anchor
 *   keybinding signal.
 * @insert_at_cursor: The class handler for the #CtkTextView::insert-at-cursor
 *   keybinding signal.
 * @delete_from_cursor: The class handler for the #CtkTextView::delete-from-cursor
 *   keybinding signal.
 * @backspace: The class handler for the #CtkTextView::backspace
 *   keybinding signal.
 * @cut_clipboard: The class handler for the #CtkTextView::cut-clipboard
 *   keybinding signal
 * @copy_clipboard: The class handler for the #CtkTextview::copy-clipboard
 *   keybinding signal.
 * @paste_clipboard: The class handler for the #CtkTextView::paste-clipboard
 *   keybinding signal.
 * @toggle_overwrite: The class handler for the #CtkTextView::toggle-overwrite
 *   keybinding signal.
 * @create_buffer: The create_buffer vfunc is called to create a #CtkTextBuffer
 *   for the text view. The default implementation is to just call
 *   ctk_text_buffer_new(). Since: 3.10
 * @draw_layer: The draw_layer vfunc is called before and after the text
 *   view is drawing its own text. Applications can override this vfunc
 *   in a subclass to draw customized content underneath or above the
 *   text. In the %CTK_TEXT_VIEW_LAYER_BELOW_TEXT and %CTK_TEXT_VIEW_LAYER_ABOVE_TEXT
 *   the drawing is done in the buffer coordinate space, but the older (deprecated)
 *   layers %CTK_TEXT_VIEW_LAYER_BELOW and %CTK_TEXT_VIEW_LAYER_ABOVE work in viewport
 *   coordinates, which makes them unnecessarily hard to use. Since: 3.14
 * @extend_selection: The class handler for the #CtkTextView::extend-selection
 *   signal. Since 3.16
 * @insert_emoji: The Emoji chooser.
 */
struct _CtkTextViewClass
{
  CtkContainerClass parent_class;

  /*< public >*/

  void (* populate_popup)        (CtkTextView      *text_view,
                                  CtkWidget        *popup);
  void (* move_cursor)           (CtkTextView      *text_view,
                                  CtkMovementStep   step,
                                  gint              count,
                                  gboolean          extend_selection);
  void (* set_anchor)            (CtkTextView      *text_view);
  void (* insert_at_cursor)      (CtkTextView      *text_view,
                                  const gchar      *str);
  void (* delete_from_cursor)    (CtkTextView      *text_view,
                                  CtkDeleteType     type,
                                  gint              count);
  void (* backspace)             (CtkTextView      *text_view);
  void (* cut_clipboard)         (CtkTextView      *text_view);
  void (* copy_clipboard)        (CtkTextView      *text_view);
  void (* paste_clipboard)       (CtkTextView      *text_view);
  void (* toggle_overwrite)      (CtkTextView      *text_view);
  CtkTextBuffer * (* create_buffer) (CtkTextView   *text_view);
  void (* draw_layer)            (CtkTextView      *text_view,
			          CtkTextViewLayer  layer,
			          cairo_t          *cr);
  gboolean (* extend_selection)  (CtkTextView            *text_view,
                                  CtkTextExtendSelection  granularity,
                                  const CtkTextIter      *location,
                                  CtkTextIter            *start,
                                  CtkTextIter            *end);
  void (* insert_emoji)          (CtkTextView      *text_view);

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

CDK_AVAILABLE_IN_ALL
GType          ctk_text_view_get_type              (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkWidget *    ctk_text_view_new                   (void);
CDK_AVAILABLE_IN_ALL
CtkWidget *    ctk_text_view_new_with_buffer       (CtkTextBuffer *buffer);
CDK_AVAILABLE_IN_ALL
void           ctk_text_view_set_buffer            (CtkTextView   *text_view,
                                                    CtkTextBuffer *buffer);
CDK_AVAILABLE_IN_ALL
CtkTextBuffer *ctk_text_view_get_buffer            (CtkTextView   *text_view);
CDK_AVAILABLE_IN_ALL
gboolean       ctk_text_view_scroll_to_iter        (CtkTextView   *text_view,
                                                    CtkTextIter   *iter,
                                                    gdouble        within_margin,
                                                    gboolean       use_align,
                                                    gdouble        xalign,
                                                    gdouble        yalign);
CDK_AVAILABLE_IN_ALL
void           ctk_text_view_scroll_to_mark        (CtkTextView   *text_view,
                                                    CtkTextMark   *mark,
                                                    gdouble        within_margin,
                                                    gboolean       use_align,
                                                    gdouble        xalign,
                                                    gdouble        yalign);
CDK_AVAILABLE_IN_ALL
void           ctk_text_view_scroll_mark_onscreen  (CtkTextView   *text_view,
                                                    CtkTextMark   *mark);
CDK_AVAILABLE_IN_ALL
gboolean       ctk_text_view_move_mark_onscreen    (CtkTextView   *text_view,
                                                    CtkTextMark   *mark);
CDK_AVAILABLE_IN_ALL
gboolean       ctk_text_view_place_cursor_onscreen (CtkTextView   *text_view);

CDK_AVAILABLE_IN_ALL
void           ctk_text_view_get_visible_rect      (CtkTextView   *text_view,
                                                    CdkRectangle  *visible_rect);
CDK_AVAILABLE_IN_ALL
void           ctk_text_view_set_cursor_visible    (CtkTextView   *text_view,
                                                    gboolean       setting);
CDK_AVAILABLE_IN_ALL
gboolean       ctk_text_view_get_cursor_visible    (CtkTextView   *text_view);

CDK_AVAILABLE_IN_3_20
void           ctk_text_view_reset_cursor_blink    (CtkTextView   *text_view);

CDK_AVAILABLE_IN_ALL
void           ctk_text_view_get_cursor_locations  (CtkTextView       *text_view,
                                                    const CtkTextIter *iter,
                                                    CdkRectangle      *strong,
                                                    CdkRectangle      *weak);
CDK_AVAILABLE_IN_ALL
void           ctk_text_view_get_iter_location     (CtkTextView   *text_view,
                                                    const CtkTextIter *iter,
                                                    CdkRectangle  *location);
CDK_AVAILABLE_IN_ALL
gboolean       ctk_text_view_get_iter_at_location  (CtkTextView   *text_view,
                                                    CtkTextIter   *iter,
                                                    gint           x,
                                                    gint           y);
CDK_AVAILABLE_IN_ALL
gboolean       ctk_text_view_get_iter_at_position  (CtkTextView   *text_view,
                                                    CtkTextIter   *iter,
						    gint          *trailing,
                                                    gint           x,
                                                    gint           y);
CDK_AVAILABLE_IN_ALL
void           ctk_text_view_get_line_yrange       (CtkTextView       *text_view,
                                                    const CtkTextIter *iter,
                                                    gint              *y,
                                                    gint              *height);

CDK_AVAILABLE_IN_ALL
void           ctk_text_view_get_line_at_y         (CtkTextView       *text_view,
                                                    CtkTextIter       *target_iter,
                                                    gint               y,
                                                    gint              *line_top);

CDK_AVAILABLE_IN_ALL
void ctk_text_view_buffer_to_window_coords (CtkTextView       *text_view,
                                            CtkTextWindowType  win,
                                            gint               buffer_x,
                                            gint               buffer_y,
                                            gint              *window_x,
                                            gint              *window_y);
CDK_AVAILABLE_IN_ALL
void ctk_text_view_window_to_buffer_coords (CtkTextView       *text_view,
                                            CtkTextWindowType  win,
                                            gint               window_x,
                                            gint               window_y,
                                            gint              *buffer_x,
                                            gint              *buffer_y);

CDK_DEPRECATED_IN_3_0_FOR(ctk_scrollable_get_hadjustment)
CtkAdjustment*   ctk_text_view_get_hadjustment (CtkTextView   *text_view);
CDK_DEPRECATED_IN_3_0_FOR(ctk_scrollable_get_vadjustment)
CtkAdjustment*   ctk_text_view_get_vadjustment (CtkTextView   *text_view);

CDK_AVAILABLE_IN_ALL
CdkWindow*        ctk_text_view_get_window      (CtkTextView       *text_view,
                                                 CtkTextWindowType  win);
CDK_AVAILABLE_IN_ALL
CtkTextWindowType ctk_text_view_get_window_type (CtkTextView       *text_view,
                                                 CdkWindow         *window);

CDK_AVAILABLE_IN_ALL
void ctk_text_view_set_border_window_size (CtkTextView       *text_view,
                                           CtkTextWindowType  type,
                                           gint               size);
CDK_AVAILABLE_IN_ALL
gint ctk_text_view_get_border_window_size (CtkTextView       *text_view,
					   CtkTextWindowType  type);

CDK_AVAILABLE_IN_ALL
gboolean ctk_text_view_forward_display_line           (CtkTextView       *text_view,
                                                       CtkTextIter       *iter);
CDK_AVAILABLE_IN_ALL
gboolean ctk_text_view_backward_display_line          (CtkTextView       *text_view,
                                                       CtkTextIter       *iter);
CDK_AVAILABLE_IN_ALL
gboolean ctk_text_view_forward_display_line_end       (CtkTextView       *text_view,
                                                       CtkTextIter       *iter);
CDK_AVAILABLE_IN_ALL
gboolean ctk_text_view_backward_display_line_start    (CtkTextView       *text_view,
                                                       CtkTextIter       *iter);
CDK_AVAILABLE_IN_ALL
gboolean ctk_text_view_starts_display_line            (CtkTextView       *text_view,
                                                       const CtkTextIter *iter);
CDK_AVAILABLE_IN_ALL
gboolean ctk_text_view_move_visually                  (CtkTextView       *text_view,
                                                       CtkTextIter       *iter,
                                                       gint               count);

CDK_AVAILABLE_IN_ALL
gboolean        ctk_text_view_im_context_filter_keypress        (CtkTextView       *text_view,
                                                                 CdkEventKey       *event);
CDK_AVAILABLE_IN_ALL
void            ctk_text_view_reset_im_context                  (CtkTextView       *text_view);

/* Adding child widgets */
CDK_AVAILABLE_IN_ALL
void ctk_text_view_add_child_at_anchor (CtkTextView          *text_view,
                                        CtkWidget            *child,
                                        CtkTextChildAnchor   *anchor);

CDK_AVAILABLE_IN_ALL
void ctk_text_view_add_child_in_window (CtkTextView          *text_view,
                                        CtkWidget            *child,
                                        CtkTextWindowType     which_window,
                                        /* window coordinates */
                                        gint                  xpos,
                                        gint                  ypos);

CDK_AVAILABLE_IN_ALL
void ctk_text_view_move_child          (CtkTextView          *text_view,
                                        CtkWidget            *child,
                                        /* window coordinates */
                                        gint                  xpos,
                                        gint                  ypos);

/* Default style settings (fallbacks if no tag affects the property) */

CDK_AVAILABLE_IN_ALL
void             ctk_text_view_set_wrap_mode          (CtkTextView      *text_view,
                                                       CtkWrapMode       wrap_mode);
CDK_AVAILABLE_IN_ALL
CtkWrapMode      ctk_text_view_get_wrap_mode          (CtkTextView      *text_view);
CDK_AVAILABLE_IN_ALL
void             ctk_text_view_set_editable           (CtkTextView      *text_view,
                                                       gboolean          setting);
CDK_AVAILABLE_IN_ALL
gboolean         ctk_text_view_get_editable           (CtkTextView      *text_view);
CDK_AVAILABLE_IN_ALL
void             ctk_text_view_set_overwrite          (CtkTextView      *text_view,
						       gboolean          overwrite);
CDK_AVAILABLE_IN_ALL
gboolean         ctk_text_view_get_overwrite          (CtkTextView      *text_view);
CDK_AVAILABLE_IN_ALL
void		 ctk_text_view_set_accepts_tab        (CtkTextView	*text_view,
						       gboolean		 accepts_tab);
CDK_AVAILABLE_IN_ALL
gboolean	 ctk_text_view_get_accepts_tab        (CtkTextView	*text_view);
CDK_AVAILABLE_IN_ALL
void             ctk_text_view_set_pixels_above_lines (CtkTextView      *text_view,
                                                       gint              pixels_above_lines);
CDK_AVAILABLE_IN_ALL
gint             ctk_text_view_get_pixels_above_lines (CtkTextView      *text_view);
CDK_AVAILABLE_IN_ALL
void             ctk_text_view_set_pixels_below_lines (CtkTextView      *text_view,
                                                       gint              pixels_below_lines);
CDK_AVAILABLE_IN_ALL
gint             ctk_text_view_get_pixels_below_lines (CtkTextView      *text_view);
CDK_AVAILABLE_IN_ALL
void             ctk_text_view_set_pixels_inside_wrap (CtkTextView      *text_view,
                                                       gint              pixels_inside_wrap);
CDK_AVAILABLE_IN_ALL
gint             ctk_text_view_get_pixels_inside_wrap (CtkTextView      *text_view);
CDK_AVAILABLE_IN_ALL
void             ctk_text_view_set_justification      (CtkTextView      *text_view,
                                                       CtkJustification  justification);
CDK_AVAILABLE_IN_ALL
CtkJustification ctk_text_view_get_justification      (CtkTextView      *text_view);
CDK_AVAILABLE_IN_ALL
void             ctk_text_view_set_left_margin        (CtkTextView      *text_view,
                                                       gint              left_margin);
CDK_AVAILABLE_IN_ALL
gint             ctk_text_view_get_left_margin        (CtkTextView      *text_view);
CDK_AVAILABLE_IN_ALL
void             ctk_text_view_set_right_margin       (CtkTextView      *text_view,
                                                       gint              right_margin);
CDK_AVAILABLE_IN_ALL
gint             ctk_text_view_get_right_margin       (CtkTextView      *text_view);
CDK_AVAILABLE_IN_3_18
void             ctk_text_view_set_top_margin         (CtkTextView      *text_view,
                                                       gint              top_margin);
CDK_AVAILABLE_IN_3_18
gint             ctk_text_view_get_top_margin         (CtkTextView      *text_view);
CDK_AVAILABLE_IN_3_18
void             ctk_text_view_set_bottom_margin      (CtkTextView      *text_view,
                                                       gint              bottom_margin);
CDK_AVAILABLE_IN_3_18
gint             ctk_text_view_get_bottom_margin       (CtkTextView      *text_view);
CDK_AVAILABLE_IN_ALL
void             ctk_text_view_set_indent             (CtkTextView      *text_view,
                                                       gint              indent);
CDK_AVAILABLE_IN_ALL
gint             ctk_text_view_get_indent             (CtkTextView      *text_view);
CDK_AVAILABLE_IN_ALL
void             ctk_text_view_set_tabs               (CtkTextView      *text_view,
                                                       PangoTabArray    *tabs);
CDK_AVAILABLE_IN_ALL
PangoTabArray*   ctk_text_view_get_tabs               (CtkTextView      *text_view);

/* note that the return value of this changes with the theme */
CDK_AVAILABLE_IN_ALL
CtkTextAttributes* ctk_text_view_get_default_attributes (CtkTextView    *text_view);

CDK_AVAILABLE_IN_3_6
void             ctk_text_view_set_input_purpose      (CtkTextView      *text_view,
                                                       CtkInputPurpose   purpose);
CDK_AVAILABLE_IN_3_6
CtkInputPurpose  ctk_text_view_get_input_purpose      (CtkTextView      *text_view);

CDK_AVAILABLE_IN_3_6
void             ctk_text_view_set_input_hints        (CtkTextView      *text_view,
                                                       CtkInputHints     hints);
CDK_AVAILABLE_IN_3_6
CtkInputHints    ctk_text_view_get_input_hints        (CtkTextView      *text_view);

CDK_AVAILABLE_IN_3_16
void             ctk_text_view_set_monospace          (CtkTextView      *text_view,
                                                       gboolean          monospace);
CDK_AVAILABLE_IN_3_16
gboolean         ctk_text_view_get_monospace          (CtkTextView      *text_view);

G_END_DECLS

#endif /* __CTK_TEXT_VIEW_H__ */
