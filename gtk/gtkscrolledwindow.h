/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __CTK_SCROLLED_WINDOW_H__
#define __CTK_SCROLLED_WINDOW_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gtk/gtkbin.h>

G_BEGIN_DECLS


#define CTK_TYPE_SCROLLED_WINDOW            (ctk_scrolled_window_get_type ())
#define CTK_SCROLLED_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_SCROLLED_WINDOW, GtkScrolledWindow))
#define CTK_SCROLLED_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_SCROLLED_WINDOW, GtkScrolledWindowClass))
#define CTK_IS_SCROLLED_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_SCROLLED_WINDOW))
#define CTK_IS_SCROLLED_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_SCROLLED_WINDOW))
#define CTK_SCROLLED_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_SCROLLED_WINDOW, GtkScrolledWindowClass))


typedef struct _GtkScrolledWindow              GtkScrolledWindow;
typedef struct _GtkScrolledWindowPrivate       GtkScrolledWindowPrivate;
typedef struct _GtkScrolledWindowClass         GtkScrolledWindowClass;

struct _GtkScrolledWindow
{
  GtkBin container;

  GtkScrolledWindowPrivate *priv;
};

/**
 * GtkScrolledWindowClass:
 * @parent_class: The parent class.
 * @scrollbar_spacing: 
 * @scroll_child: Keybinding signal which gets emitted when a
 *    keybinding that scrolls is pressed.
 * @move_focus_out: Keybinding signal which gets emitted when focus is
 *    moved away from the scrolled window by a keybinding.
 */
struct _GtkScrolledWindowClass
{
  GtkBinClass parent_class;

  gint scrollbar_spacing;

  /*< public >*/

  /* Action signals for keybindings. Do not connect to these signals
   */

  /* Unfortunately, GtkScrollType is deficient in that there is
   * no horizontal/vertical variants for CTK_SCROLL_START/END,
   * so we have to add an additional boolean flag.
   */
  gboolean (*scroll_child) (GtkScrolledWindow *scrolled_window,
	  		    GtkScrollType      scroll,
			    gboolean           horizontal);

  void (* move_focus_out) (GtkScrolledWindow *scrolled_window,
			   GtkDirectionType   direction);

  /*< private >*/

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};


/**
 * GtkCornerType:
 * @CTK_CORNER_TOP_LEFT: Place the scrollbars on the right and bottom of the
 *  widget (default behaviour).
 * @CTK_CORNER_BOTTOM_LEFT: Place the scrollbars on the top and right of the
 *  widget.
 * @CTK_CORNER_TOP_RIGHT: Place the scrollbars on the left and bottom of the
 *  widget.
 * @CTK_CORNER_BOTTOM_RIGHT: Place the scrollbars on the top and left of the
 *  widget.
 *
 * Specifies which corner a child widget should be placed in when packed into
 * a #GtkScrolledWindow. This is effectively the opposite of where the scroll
 * bars are placed.
 */
typedef enum
{
  CTK_CORNER_TOP_LEFT,
  CTK_CORNER_BOTTOM_LEFT,
  CTK_CORNER_TOP_RIGHT,
  CTK_CORNER_BOTTOM_RIGHT
} GtkCornerType;


/**
 * GtkPolicyType:
 * @CTK_POLICY_ALWAYS: The scrollbar is always visible. The view size is
 *  independent of the content.
 * @CTK_POLICY_AUTOMATIC: The scrollbar will appear and disappear as necessary.
 *  For example, when all of a #GtkTreeView can not be seen.
 * @CTK_POLICY_NEVER: The scrollbar should never appear. In this mode the
 *  content determines the size.
 * @CTK_POLICY_EXTERNAL: Don't show a scrollbar, but don't force the
 *  size to follow the content. This can be used e.g. to make multiple
 *  scrolled windows share a scrollbar. Since: 3.16
 *
 * Determines how the size should be computed to achieve the one of the
 * visibility mode for the scrollbars.
 */
typedef enum
{
  CTK_POLICY_ALWAYS,
  CTK_POLICY_AUTOMATIC,
  CTK_POLICY_NEVER,
  CTK_POLICY_EXTERNAL
} GtkPolicyType;


GDK_AVAILABLE_IN_ALL
GType          ctk_scrolled_window_get_type          (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
GtkWidget*     ctk_scrolled_window_new               (GtkAdjustment     *hadjustment,
						      GtkAdjustment     *vadjustment);
GDK_AVAILABLE_IN_ALL
void           ctk_scrolled_window_set_hadjustment   (GtkScrolledWindow *scrolled_window,
						      GtkAdjustment     *hadjustment);
GDK_AVAILABLE_IN_ALL
void           ctk_scrolled_window_set_vadjustment   (GtkScrolledWindow *scrolled_window,
						      GtkAdjustment     *vadjustment);
GDK_AVAILABLE_IN_ALL
GtkAdjustment* ctk_scrolled_window_get_hadjustment   (GtkScrolledWindow *scrolled_window);
GDK_AVAILABLE_IN_ALL
GtkAdjustment* ctk_scrolled_window_get_vadjustment   (GtkScrolledWindow *scrolled_window);
GDK_AVAILABLE_IN_ALL
GtkWidget*     ctk_scrolled_window_get_hscrollbar    (GtkScrolledWindow *scrolled_window);
GDK_AVAILABLE_IN_ALL
GtkWidget*     ctk_scrolled_window_get_vscrollbar    (GtkScrolledWindow *scrolled_window);
GDK_AVAILABLE_IN_ALL
void           ctk_scrolled_window_set_policy        (GtkScrolledWindow *scrolled_window,
						      GtkPolicyType      hscrollbar_policy,
						      GtkPolicyType      vscrollbar_policy);
GDK_AVAILABLE_IN_ALL
void           ctk_scrolled_window_get_policy        (GtkScrolledWindow *scrolled_window,
						      GtkPolicyType     *hscrollbar_policy,
						      GtkPolicyType     *vscrollbar_policy);
GDK_AVAILABLE_IN_ALL
void           ctk_scrolled_window_set_placement     (GtkScrolledWindow *scrolled_window,
						      GtkCornerType      window_placement);
GDK_AVAILABLE_IN_ALL
void           ctk_scrolled_window_unset_placement   (GtkScrolledWindow *scrolled_window);

GDK_AVAILABLE_IN_ALL
GtkCornerType  ctk_scrolled_window_get_placement     (GtkScrolledWindow *scrolled_window);
GDK_AVAILABLE_IN_ALL
void           ctk_scrolled_window_set_shadow_type   (GtkScrolledWindow *scrolled_window,
						      GtkShadowType      type);
GDK_AVAILABLE_IN_ALL
GtkShadowType  ctk_scrolled_window_get_shadow_type   (GtkScrolledWindow *scrolled_window);
GDK_DEPRECATED_IN_3_8_FOR(ctk_container_add)
void	       ctk_scrolled_window_add_with_viewport (GtkScrolledWindow *scrolled_window,
						      GtkWidget		*child);

GDK_AVAILABLE_IN_ALL
gint           ctk_scrolled_window_get_min_content_width  (GtkScrolledWindow *scrolled_window);
GDK_AVAILABLE_IN_ALL
void           ctk_scrolled_window_set_min_content_width  (GtkScrolledWindow *scrolled_window,
                                                           gint               width);
GDK_AVAILABLE_IN_ALL
gint           ctk_scrolled_window_get_min_content_height (GtkScrolledWindow *scrolled_window);
GDK_AVAILABLE_IN_ALL
void           ctk_scrolled_window_set_min_content_height (GtkScrolledWindow *scrolled_window,
                                                           gint               height);
GDK_AVAILABLE_IN_3_4
void           ctk_scrolled_window_set_kinetic_scrolling  (GtkScrolledWindow *scrolled_window,
                                                           gboolean           kinetic_scrolling);
GDK_AVAILABLE_IN_3_4
gboolean       ctk_scrolled_window_get_kinetic_scrolling  (GtkScrolledWindow *scrolled_window);

GDK_AVAILABLE_IN_3_4
void           ctk_scrolled_window_set_capture_button_press (GtkScrolledWindow *scrolled_window,
                                                             gboolean           capture_button_press);
GDK_AVAILABLE_IN_3_4
gboolean       ctk_scrolled_window_get_capture_button_press (GtkScrolledWindow *scrolled_window);

GDK_AVAILABLE_IN_3_16
void           ctk_scrolled_window_set_overlay_scrolling  (GtkScrolledWindow *scrolled_window,
                                                           gboolean           overlay_scrolling);
GDK_AVAILABLE_IN_3_16
gboolean       ctk_scrolled_window_get_overlay_scrolling (GtkScrolledWindow   *scrolled_window);

GDK_AVAILABLE_IN_3_22
void           ctk_scrolled_window_set_max_content_width  (GtkScrolledWindow *scrolled_window,
                                                           gint               width);
GDK_AVAILABLE_IN_3_22
gint           ctk_scrolled_window_get_max_content_width  (GtkScrolledWindow *scrolled_window);

GDK_AVAILABLE_IN_3_22
void           ctk_scrolled_window_set_max_content_height (GtkScrolledWindow *scrolled_window,
                                                           gint               height);
GDK_AVAILABLE_IN_3_22
gint           ctk_scrolled_window_get_max_content_height (GtkScrolledWindow *scrolled_window);

GDK_AVAILABLE_IN_3_22
void           ctk_scrolled_window_set_propagate_natural_width  (GtkScrolledWindow *scrolled_window,
								 gboolean           propagate);
GDK_AVAILABLE_IN_3_22
gboolean       ctk_scrolled_window_get_propagate_natural_width  (GtkScrolledWindow *scrolled_window);

GDK_AVAILABLE_IN_3_22
void           ctk_scrolled_window_set_propagate_natural_height (GtkScrolledWindow *scrolled_window,
								 gboolean           propagate);
GDK_AVAILABLE_IN_3_22
gboolean       ctk_scrolled_window_get_propagate_natural_height (GtkScrolledWindow *scrolled_window);

G_END_DECLS


#endif /* __CTK_SCROLLED_WINDOW_H__ */
