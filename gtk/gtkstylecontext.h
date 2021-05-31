/* GTK - The GIMP Toolkit
 * Copyright (C) 2010 Carlos Garnacho <carlosg@gnome.org>
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

#ifndef __CTK_STYLE_CONTEXT_H__
#define __CTK_STYLE_CONTEXT_H__

#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#include <gtk/gtkborder.h>
#include <gtk/gtkcsssection.h>
#include <gtk/gtkstyleprovider.h>
#include <gtk/gtktypes.h>
#include <atk/atk.h>

G_BEGIN_DECLS

#define CTK_TYPE_STYLE_CONTEXT         (ctk_style_context_get_type ())
#define CTK_STYLE_CONTEXT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CTK_TYPE_STYLE_CONTEXT, GtkStyleContext))
#define CTK_STYLE_CONTEXT_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST    ((c), CTK_TYPE_STYLE_CONTEXT, GtkStyleContextClass))
#define CTK_IS_STYLE_CONTEXT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CTK_TYPE_STYLE_CONTEXT))
#define CTK_IS_STYLE_CONTEXT_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE    ((c), CTK_TYPE_STYLE_CONTEXT))
#define CTK_STYLE_CONTEXT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS  ((o), CTK_TYPE_STYLE_CONTEXT, GtkStyleContextClass))

typedef struct _GtkStyleContextClass GtkStyleContextClass;
typedef struct _GtkStyleContextPrivate GtkStyleContextPrivate;

struct _GtkStyleContext
{
  GObject parent_object;
  GtkStyleContextPrivate *priv;
};

struct _GtkStyleContextClass
{
  GObjectClass parent_class;

  void (* changed) (GtkStyleContext *context);

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

/* Default set of properties that GtkStyleContext may contain */

/**
 * CTK_STYLE_PROPERTY_BACKGROUND_COLOR:
 *
 * A property holding the background color of rendered elements as a #GdkRGBA.
 */
#define CTK_STYLE_PROPERTY_BACKGROUND_COLOR "background-color"

/**
 * CTK_STYLE_PROPERTY_COLOR:
 *
 * A property holding the foreground color of rendered elements as a #GdkRGBA.
 */
#define CTK_STYLE_PROPERTY_COLOR "color"

/**
 * CTK_STYLE_PROPERTY_FONT:
 *
 * A property holding the font properties used when rendering text
 * as a #PangoFontDescription.
 */
#define CTK_STYLE_PROPERTY_FONT "font"

/**
 * CTK_STYLE_PROPERTY_PADDING:
 *
 * A property holding the rendered element’s padding as a #GtkBorder. The
 * padding is defined as the spacing between the inner part of the element border
 * and its child. It’s the innermost spacing property of the padding/border/margin
 * series.
 */
#define CTK_STYLE_PROPERTY_PADDING "padding"

/**
 * CTK_STYLE_PROPERTY_BORDER_WIDTH:
 *
 * A property holding the rendered element’s border width in pixels as
 * a #GtkBorder. The border is the intermediary spacing property of the
 * padding/border/margin series.
 *
 * ctk_render_frame() uses this property to find out the frame line width,
 * so #GtkWidgets rendering frames may need to add up this padding when
 * requesting size
 */
#define CTK_STYLE_PROPERTY_BORDER_WIDTH "border-width"

/**
 * CTK_STYLE_PROPERTY_MARGIN:
 *
 * A property holding the rendered element’s margin as a #GtkBorder. The
 * margin is defined as the spacing between the border of the element
 * and its surrounding elements. It is external to #GtkWidget's
 * size allocations, and the most external spacing property of the
 * padding/border/margin series.
 */
#define CTK_STYLE_PROPERTY_MARGIN "margin"

/**
 * CTK_STYLE_PROPERTY_BORDER_RADIUS:
 *
 * A property holding the rendered element’s border radius in pixels as a #gint.
 */
#define CTK_STYLE_PROPERTY_BORDER_RADIUS "border-radius"

/**
 * CTK_STYLE_PROPERTY_BORDER_STYLE:
 *
 * A property holding the element’s border style as a #GtkBorderStyle.
 */
#define CTK_STYLE_PROPERTY_BORDER_STYLE "border-style"

/**
 * CTK_STYLE_PROPERTY_BORDER_COLOR:
 *
 * A property holding the element’s border color as a #GdkRGBA.
 */
#define CTK_STYLE_PROPERTY_BORDER_COLOR "border-color"

/**
 * CTK_STYLE_PROPERTY_BACKGROUND_IMAGE:
 *
 * A property holding the element’s background as a #cairo_pattern_t.
 */
#define CTK_STYLE_PROPERTY_BACKGROUND_IMAGE "background-image"

/* Predefined set of CSS classes */

/**
 * CTK_STYLE_CLASS_CELL:
 *
 * A CSS class to match content rendered in cell views.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_CELL "cell"

/**
 * CTK_STYLE_CLASS_DIM_LABEL:
 *
 * A CSS class to match dimmed labels.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_DIM_LABEL "dim-label"

/**
 * CTK_STYLE_CLASS_ENTRY:
 *
 * A CSS class to match text entries.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_ENTRY "entry"

/**
 * CTK_STYLE_CLASS_LABEL:
 *
 * A CSS class to match labels.
 *
 * Refer to individual widget documentation for used style classes.
 *
 * Since: 3.16
 */
#define CTK_STYLE_CLASS_LABEL "label"

/**
 * CTK_STYLE_CLASS_COMBOBOX_ENTRY:
 *
 * A CSS class to match combobox entries.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_COMBOBOX_ENTRY "combobox-entry"

/**
 * CTK_STYLE_CLASS_BUTTON:
 *
 * A CSS class to match buttons.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_BUTTON "button"

/**
 * CTK_STYLE_CLASS_LIST:
 *
 * A CSS class to match lists.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_LIST "list"

/**
 * CTK_STYLE_CLASS_LIST_ROW:
 *
 * A CSS class to match list rows.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_LIST_ROW "list-row"

/**
 * CTK_STYLE_CLASS_CALENDAR:
 *
 * A CSS class to match calendars.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_CALENDAR "calendar"

/**
 * CTK_STYLE_CLASS_SLIDER:
 *
 * A CSS class to match sliders.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_SLIDER "slider"

/**
 * CTK_STYLE_CLASS_BACKGROUND:
 *
 * A CSS class to match the window background.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_BACKGROUND "background"

/**
 * CTK_STYLE_CLASS_RUBBERBAND:
 *
 * A CSS class to match the rubberband selection rectangle.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_RUBBERBAND "rubberband"

/**
 * CTK_STYLE_CLASS_CSD:
 *
 * A CSS class that gets added to windows which have client-side decorations.
 *
 * Refer to individual widget documentation for used style classes.
 *
 * Since: 3.14
 */
#define CTK_STYLE_CLASS_CSD "csd"

/**
 * CTK_STYLE_CLASS_TOOLTIP:
 *
 * A CSS class to match tooltip windows.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_TOOLTIP "tooltip"

/**
 * CTK_STYLE_CLASS_MENU:
 *
 * A CSS class to match menus.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_MENU "menu"

/**
 * CTK_STYLE_CLASS_CONTEXT_MENU:
 *
 * A CSS class to match context menus.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_CONTEXT_MENU "context-menu"

/**
 * CTK_STYLE_CLASS_TOUCH_SELECTION:
 *
 * A CSS class for touch selection popups on entries
 * and text views.
 *
 * Refer to individual widget documentation for used style classes.
 *
 * Since: 3.16
 */
#define CTK_STYLE_CLASS_TOUCH_SELECTION "touch-selection"

/**
 * CTK_STYLE_CLASS_MENUBAR:
 *
 * A CSS class to menubars.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_MENUBAR "menubar"

/**
 * CTK_STYLE_CLASS_MENUITEM:
 *
 * A CSS class to match menu items.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_MENUITEM "menuitem"

/**
 * CTK_STYLE_CLASS_TOOLBAR:
 *
 * A CSS class to match toolbars.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_TOOLBAR "toolbar"

/**
 * CTK_STYLE_CLASS_PRIMARY_TOOLBAR:
 *
 * A CSS class to match primary toolbars.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_PRIMARY_TOOLBAR "primary-toolbar"

/**
 * CTK_STYLE_CLASS_INLINE_TOOLBAR:
 *
 * A CSS class to match inline toolbars.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_INLINE_TOOLBAR "inline-toolbar"

/**
 * CTK_STYLE_CLASS_STATUSBAR:
 *
 * A CSS class to match statusbars.
 *
 * Refer to individual widget documentation for used style classes.
 *
 * Since: 3.16
 */
#define CTK_STYLE_CLASS_STATUSBAR "statusbar"

/**
 * CTK_STYLE_CLASS_RADIO:
 *
 * A CSS class to match radio buttons.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_RADIO "radio"

/**
 * CTK_STYLE_CLASS_CHECK:
 *
 * A CSS class to match check boxes.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_CHECK "check"

/**
 * CTK_STYLE_CLASS_DEFAULT:
 *
 * A CSS class to match the default widget.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_DEFAULT "default"

/**
 * CTK_STYLE_CLASS_TROUGH:
 *
 * A CSS class to match troughs, as in scrollbars and progressbars.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_TROUGH "trough"

/**
 * CTK_STYLE_CLASS_SCROLLBAR:
 *
 * A CSS class to match scrollbars.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_SCROLLBAR "scrollbar"

/**
 * CTK_STYLE_CLASS_SCROLLBARS_JUNCTION:
 *
 * A CSS class to match the junction area between an horizontal
 * and vertical scrollbar, when they’re both shown.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_SCROLLBARS_JUNCTION "scrollbars-junction"

/**
 * CTK_STYLE_CLASS_SCALE:
 *
 * A CSS class to match scale widgets.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_SCALE "scale"

/**
 * CTK_STYLE_CLASS_SCALE_HAS_MARKS_ABOVE:
 *
 * A CSS class to match scale widgets with marks attached,
 * all the marks are above for horizontal #GtkScale.
 * left for vertical #GtkScale.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_SCALE_HAS_MARKS_ABOVE "scale-has-marks-above"

/**
 * CTK_STYLE_CLASS_SCALE_HAS_MARKS_BELOW:
 *
 * A CSS class to match scale widgets with marks attached,
 * all the marks are below for horizontal #GtkScale,
 * right for vertical #GtkScale.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_SCALE_HAS_MARKS_BELOW "scale-has-marks-below"

/**
 * CTK_STYLE_CLASS_HEADER:
 *
 * A CSS class to match a header element.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_HEADER "header"

/**
 * CTK_STYLE_CLASS_ACCELERATOR:
 *
 * A CSS class to match an accelerator.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_ACCELERATOR "accelerator"

/**
 * CTK_STYLE_CLASS_RAISED:
 *
 * A CSS class to match a raised control, such as a raised
 * button on a toolbar.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_RAISED "raised"

/**
 * CTK_STYLE_CLASS_LINKED:
 *
 * A CSS class to match a linked area, such as a box containing buttons
 * belonging to the same control.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_LINKED "linked"

/**
 * CTK_STYLE_CLASS_GRIP:
 *
 * A CSS class defining a resize grip.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_GRIP "grip"

/**
 * CTK_STYLE_CLASS_DOCK:
 *
 * A CSS class defining a dock area.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_DOCK "dock"

/**
 * CTK_STYLE_CLASS_PROGRESSBAR:
 *
 * A CSS class to use when rendering activity as a progressbar.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_PROGRESSBAR "progressbar"

/**
 * CTK_STYLE_CLASS_SPINNER:
 *
 * A CSS class to use when rendering activity as a “spinner”.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_SPINNER "spinner"

/**
 * CTK_STYLE_CLASS_MARK:
 *
 * A CSS class defining marks in a widget, such as in scales.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_MARK "mark"

/**
 * CTK_STYLE_CLASS_EXPANDER:
 *
 * A CSS class defining an expander, such as those in treeviews.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_EXPANDER "expander"

/**
 * CTK_STYLE_CLASS_SPINBUTTON:
 *
 * A CSS class defining an spinbutton.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_SPINBUTTON "spinbutton"

/**
 * CTK_STYLE_CLASS_NOTEBOOK:
 *
 * A CSS class defining a notebook.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_NOTEBOOK "notebook"

/**
 * CTK_STYLE_CLASS_VIEW:
 *
 * A CSS class defining a view, such as iconviews or treeviews.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_VIEW "view"

/**
 * CTK_STYLE_CLASS_SIDEBAR:
 *
 * A CSS class defining a sidebar, such as the left side in
 * a file chooser.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_SIDEBAR "sidebar"

/**
 * CTK_STYLE_CLASS_IMAGE:
 *
 * A CSS class defining an image, such as the icon in an entry.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_IMAGE "image"

/**
 * CTK_STYLE_CLASS_HIGHLIGHT:
 *
 * A CSS class defining a highlighted area, such as headings in
 * assistants and calendars.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_HIGHLIGHT "highlight"

/**
 * CTK_STYLE_CLASS_FRAME:
 *
 * A CSS class defining a frame delimiting content, such as
 * #GtkFrame or the scrolled window frame around the
 * scrollable area.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_FRAME "frame"

/**
 * CTK_STYLE_CLASS_DND:
 *
 * A CSS class for a drag-and-drop indicator.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_DND "dnd"

/**
 * CTK_STYLE_CLASS_PANE_SEPARATOR:
 *
 * A CSS class for a pane separator, such as those in #GtkPaned.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_PANE_SEPARATOR "pane-separator"

/**
 * CTK_STYLE_CLASS_SEPARATOR:
 *
 * A CSS class for a separator.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_SEPARATOR "separator"

/**
 * CTK_STYLE_CLASS_INFO:
 *
 * A CSS class for an area displaying an informational message,
 * such as those in infobars.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_INFO "info"

/**
 * CTK_STYLE_CLASS_WARNING:
 *
 * A CSS class for an area displaying a warning message,
 * such as those in infobars.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_WARNING "warning"

/**
 * CTK_STYLE_CLASS_QUESTION:
 *
 * A CSS class for an area displaying a question to the user,
 * such as those in infobars.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_QUESTION "question"

/**
 * CTK_STYLE_CLASS_ERROR:
 *
 * A CSS class for an area displaying an error message,
 * such as those in infobars.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_ERROR "error"

/**
 * CTK_STYLE_CLASS_HORIZONTAL:
 *
 * A CSS class for horizontally layered widgets.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_HORIZONTAL "horizontal"

/**
 * CTK_STYLE_CLASS_VERTICAL:
 *
 * A CSS class for vertically layered widgets.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_VERTICAL "vertical"

/**
 * CTK_STYLE_CLASS_TOP:
 *
 * A CSS class to indicate an area at the top of a widget.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_TOP "top"

/**
 * CTK_STYLE_CLASS_BOTTOM:
 *
 * A CSS class to indicate an area at the bottom of a widget.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_BOTTOM "bottom"

/**
 * CTK_STYLE_CLASS_LEFT:
 *
 * A CSS class to indicate an area at the left of a widget.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_LEFT "left"

/**
 * CTK_STYLE_CLASS_RIGHT:
 *
 * A CSS class to indicate an area at the right of a widget.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_RIGHT "right"

/**
 * CTK_STYLE_CLASS_PULSE:
 *
 * A CSS class to use when rendering a pulse in an indeterminate progress bar.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_PULSE "pulse"

/**
 * CTK_STYLE_CLASS_ARROW:
 *
 * A CSS class used when rendering an arrow element.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_ARROW "arrow"

/**
 * CTK_STYLE_CLASS_OSD:
 *
 * A CSS class used when rendering an OSD (On Screen Display) element,
 * on top of another container.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_OSD "osd"

/**
 * CTK_STYLE_CLASS_LEVEL_BAR:
 *
 * A CSS class used when rendering a level indicator, such
 * as a battery charge level, or a password strength.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_LEVEL_BAR "level-bar"

/**
 * CTK_STYLE_CLASS_CURSOR_HANDLE:
 *
 * A CSS class used when rendering a drag handle for
 * text selection.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_CURSOR_HANDLE "cursor-handle"

/**
 * CTK_STYLE_CLASS_INSERTION_CURSOR:
 *
 * A CSS class used when rendering a drag handle for
 * the insertion cursor position.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_INSERTION_CURSOR "insertion-cursor"

/**
 * CTK_STYLE_CLASS_TITLEBAR:
 *
 * A CSS class used when rendering a titlebar in a toplevel window.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_TITLEBAR "titlebar"

/**
 * CTK_STYLE_CLASS_TITLE:
 *
 * A CSS class used for the title label in a titlebar in
 * a toplevel window.
 *
 * Refer to individual widget documentation for used style classes.
 *
 * Since: 3.14
 */
#define CTK_STYLE_CLASS_TITLE "title"

/**
 * CTK_STYLE_CLASS_SUBTITLE:
 *
 * A CSS class used for the subtitle label in a titlebar in
 * a toplevel window.
 *
 * Refer to individual widget documentation for used style classes.
 *
 * Since: 3.14
 */
#define CTK_STYLE_CLASS_SUBTITLE "subtitle"

/**
 * CTK_STYLE_CLASS_NEEDS_ATTENTION:
 *
 * A CSS class used when an element needs the user attention,
 * for instance a button in a stack switcher corresponding to
 * a hidden page that changed state.
 *
 * Refer to individual widget documentation for used style classes.
 *
 * Since: 3.12
 */
#define CTK_STYLE_CLASS_NEEDS_ATTENTION "needs-attention"

/**
 * CTK_STYLE_CLASS_SUGGESTED_ACTION:
 *
 * A CSS class used when an action (usually a button) is the
 * primary suggested action in a specific context.
 *
 * Refer to individual widget documentation for used style classes.
 *
 * Since: 3.12
 */
#define CTK_STYLE_CLASS_SUGGESTED_ACTION "suggested-action"

/**
 * CTK_STYLE_CLASS_DESTRUCTIVE_ACTION:
 *
 * A CSS class used when an action (usually a button) is
 * one that is expected to remove or destroy something visible
 * to the user.
 *
 * Refer to individual widget documentation for used style classes.
 *
 * Since: 3.12
 */
#define CTK_STYLE_CLASS_DESTRUCTIVE_ACTION "destructive-action"

/**
 * CTK_STYLE_CLASS_POPOVER:
 *
 * A CSS class that matches popovers.
 *
 * Refer to individual widget documentation for used style classes.
 *
 * Since: 3.14
 */
#define CTK_STYLE_CLASS_POPOVER "popover"

/* Predefined set of widget regions */

/**
 * CTK_STYLE_CLASS_POPUP:
 *
 * A CSS class that is added to the toplevel windows used for menus.
 *
 * Refer to individual widget documentation for used style classes.
 *
 * Since: 3.14
 */
#define CTK_STYLE_CLASS_POPUP "popup"

/**
 * CTK_STYLE_CLASS_MESSAGE_DIALOG:
 *
 * A CSS class that is added to message dialogs.
 *
 * Refer to individual widget documentation for used style classes.
 *
 * Since: 3.14
 */
#define CTK_STYLE_CLASS_MESSAGE_DIALOG "message-dialog"

/**
 * CTK_STYLE_CLASS_FLAT:
 *
 * A CSS class that is added when widgets that usually have
 * a frame or border (like buttons or entries) should appear
 * without it.
 *
 * Refer to individual widget documentation for used style classes.
 *
 * Since: 3.14
 */
#define CTK_STYLE_CLASS_FLAT "flat"

/**
 * CTK_STYLE_CLASS_READ_ONLY:
 *
 * A CSS class used to indicate a read-only state.
 *
 * Refer to individual widget documentation for used style classes.
 */
#define CTK_STYLE_CLASS_READ_ONLY "read-only"

/**
 * CTK_STYLE_CLASS_OVERSHOOT:
 *
 * A CSS class that is added on the visual hints that happen
 * when scrolling is attempted past the limits of a scrollable
 * area.
 *
 * Refer to individual widget documentation for used style classes.
 *
 * Since: 3.14
 */
#define CTK_STYLE_CLASS_OVERSHOOT "overshoot"

/**
 * CTK_STYLE_CLASS_UNDERSHOOT:
 *
 * A CSS class that is added on the visual hints that happen
 * where content is 'scrolled off' and can be made visible
 * by scrolling.
 *
 * Refer to individual widget documentation for used style classes.
 *
 * Since: 3.16
 */
#define CTK_STYLE_CLASS_UNDERSHOOT "undershoot"

/**
 * CTK_STYLE_CLASS_PAPER:
 *
 * A CSS class that is added to areas that should look like paper.
 *
 * This is used in print previews and themes are encouraged to
 * style it as black text on white background.
 *
 * Refer to individual widget documentation for used style classes.
 *
 * Since: 3.16
 */
#define CTK_STYLE_CLASS_PAPER "paper"

/**
 * CTK_STYLE_CLASS_MONOSPACE:
 *
 * A CSS class that is added to text view that should use
 * a monospace font.
 *
 * Refer to individual widget documentation for used style classes.
 *
 * Since: 3.16
 */
#define CTK_STYLE_CLASS_MONOSPACE "monospace"

/**
 * CTK_STYLE_CLASS_WIDE:
 *
 * A CSS class to indicate that a UI element should be 'wide'.
 * Used by #GtkPaned.
 *
 * Refer to individual widget documentation for used style classes.
 *
 * Since: 3.16
 */
#define CTK_STYLE_CLASS_WIDE "wide"

/**
 * CTK_STYLE_REGION_ROW:
 *
 * A widget region name to define a treeview row.
 *
 * Deprecated: 3.20: Don't use regions.
 */
#define CTK_STYLE_REGION_ROW "row"

/**
 * CTK_STYLE_REGION_COLUMN:
 *
 * A widget region name to define a treeview column.
 *
 * Deprecated: 3.20: Don't use regions.
 */
#define CTK_STYLE_REGION_COLUMN "column"

/**
 * CTK_STYLE_REGION_COLUMN_HEADER:
 *
 * A widget region name to define a treeview column header.
 *
 * Deprecated: 3.20: Don't use regions.
 */
#define CTK_STYLE_REGION_COLUMN_HEADER "column-header"

/**
 * CTK_STYLE_REGION_TAB:
 *
 * A widget region name to define a notebook tab.
 *
 * Deprecated: 3.20: Don't use regions.
 */
#define CTK_STYLE_REGION_TAB "tab"

GDK_AVAILABLE_IN_ALL
GType ctk_style_context_get_type (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
GtkStyleContext * ctk_style_context_new (void);

GDK_AVAILABLE_IN_ALL
void ctk_style_context_add_provider_for_screen    (GdkScreen        *screen,
                                                   GtkStyleProvider *provider,
                                                   guint             priority);
GDK_AVAILABLE_IN_ALL
void ctk_style_context_remove_provider_for_screen (GdkScreen        *screen,
                                                   GtkStyleProvider *provider);

GDK_AVAILABLE_IN_ALL
void ctk_style_context_add_provider    (GtkStyleContext  *context,
                                        GtkStyleProvider *provider,
                                        guint             priority);

GDK_AVAILABLE_IN_ALL
void ctk_style_context_remove_provider (GtkStyleContext  *context,
                                        GtkStyleProvider *provider);

GDK_AVAILABLE_IN_ALL
void ctk_style_context_save    (GtkStyleContext *context);
GDK_AVAILABLE_IN_ALL
void ctk_style_context_restore (GtkStyleContext *context);

GDK_AVAILABLE_IN_ALL
GtkCssSection * ctk_style_context_get_section (GtkStyleContext *context,
                                               const gchar     *property);
GDK_AVAILABLE_IN_ALL
void ctk_style_context_get_property (GtkStyleContext *context,
                                     const gchar     *property,
                                     GtkStateFlags    state,
                                     GValue          *value);
GDK_AVAILABLE_IN_ALL
void ctk_style_context_get_valist   (GtkStyleContext *context,
                                     GtkStateFlags    state,
                                     va_list          args);
GDK_AVAILABLE_IN_ALL
void ctk_style_context_get          (GtkStyleContext *context,
                                     GtkStateFlags    state,
                                     ...) G_GNUC_NULL_TERMINATED;

GDK_AVAILABLE_IN_ALL
void          ctk_style_context_set_state    (GtkStyleContext *context,
                                              GtkStateFlags    flags);
GDK_AVAILABLE_IN_ALL
GtkStateFlags ctk_style_context_get_state    (GtkStyleContext *context);

GDK_AVAILABLE_IN_3_10
void          ctk_style_context_set_scale    (GtkStyleContext *context,
                                              gint             scale);
GDK_AVAILABLE_IN_3_10
gint          ctk_style_context_get_scale    (GtkStyleContext *context);

GDK_DEPRECATED_IN_3_6
gboolean      ctk_style_context_state_is_running (GtkStyleContext *context,
                                                  GtkStateType     state,
                                                  gdouble         *progress);

GDK_AVAILABLE_IN_ALL
void          ctk_style_context_set_path     (GtkStyleContext *context,
                                              GtkWidgetPath   *path);
GDK_AVAILABLE_IN_ALL
const GtkWidgetPath * ctk_style_context_get_path (GtkStyleContext *context);
GDK_AVAILABLE_IN_3_4
void          ctk_style_context_set_parent   (GtkStyleContext *context,
                                              GtkStyleContext *parent);
GDK_AVAILABLE_IN_ALL
GtkStyleContext *ctk_style_context_get_parent (GtkStyleContext *context);

GDK_AVAILABLE_IN_ALL
GList *  ctk_style_context_list_classes (GtkStyleContext *context);

GDK_AVAILABLE_IN_ALL
void     ctk_style_context_add_class    (GtkStyleContext *context,
                                         const gchar     *class_name);
GDK_AVAILABLE_IN_ALL
void     ctk_style_context_remove_class (GtkStyleContext *context,
                                         const gchar     *class_name);
GDK_AVAILABLE_IN_ALL
gboolean ctk_style_context_has_class    (GtkStyleContext *context,
                                         const gchar     *class_name);

GDK_DEPRECATED_IN_3_14
GList *  ctk_style_context_list_regions (GtkStyleContext *context);

GDK_DEPRECATED_IN_3_14
void     ctk_style_context_add_region    (GtkStyleContext    *context,
                                          const gchar        *region_name,
                                          GtkRegionFlags      flags);
GDK_DEPRECATED_IN_3_14
void     ctk_style_context_remove_region (GtkStyleContext    *context,
                                          const gchar        *region_name);
GDK_DEPRECATED_IN_3_14
gboolean ctk_style_context_has_region    (GtkStyleContext    *context,
                                          const gchar        *region_name,
                                          GtkRegionFlags     *flags_return);

GDK_AVAILABLE_IN_ALL
void ctk_style_context_get_style_property (GtkStyleContext *context,
                                           const gchar     *property_name,
                                           GValue          *value);
GDK_AVAILABLE_IN_ALL
void ctk_style_context_get_style_valist   (GtkStyleContext *context,
                                           va_list          args);
GDK_AVAILABLE_IN_ALL
void ctk_style_context_get_style          (GtkStyleContext *context,
                                           ...);

GDK_DEPRECATED_IN_3_10_FOR(ctk_icon_theme_lookup_icon)
GtkIconSet * ctk_style_context_lookup_icon_set (GtkStyleContext *context,
                                                const gchar     *stock_id);
GDK_DEPRECATED_IN_3_10
GdkPixbuf  * ctk_icon_set_render_icon_pixbuf   (GtkIconSet      *icon_set,
                                                GtkStyleContext *context,
                                                GtkIconSize      size);
GDK_DEPRECATED_IN_3_10
cairo_surface_t  *
ctk_icon_set_render_icon_surface               (GtkIconSet      *icon_set,
						GtkStyleContext *context,
						GtkIconSize      size,
						int              scale,
						GdkWindow       *for_window);

GDK_AVAILABLE_IN_ALL
void        ctk_style_context_set_screen (GtkStyleContext *context,
                                          GdkScreen       *screen);
GDK_AVAILABLE_IN_ALL
GdkScreen * ctk_style_context_get_screen (GtkStyleContext *context);

GDK_AVAILABLE_IN_3_8
void           ctk_style_context_set_frame_clock (GtkStyleContext *context,
                                                  GdkFrameClock   *frame_clock);
GDK_AVAILABLE_IN_3_8
GdkFrameClock *ctk_style_context_get_frame_clock (GtkStyleContext *context);

GDK_DEPRECATED_IN_3_8_FOR(ctk_style_context_set_state)
void             ctk_style_context_set_direction (GtkStyleContext  *context,
                                                  GtkTextDirection  direction);
GDK_DEPRECATED_IN_3_8_FOR(ctk_style_context_get_state)
GtkTextDirection ctk_style_context_get_direction (GtkStyleContext  *context);

GDK_AVAILABLE_IN_ALL
void             ctk_style_context_set_junction_sides (GtkStyleContext  *context,
                                                       GtkJunctionSides  sides);
GDK_AVAILABLE_IN_ALL
GtkJunctionSides ctk_style_context_get_junction_sides (GtkStyleContext  *context);

GDK_AVAILABLE_IN_ALL
gboolean ctk_style_context_lookup_color (GtkStyleContext *context,
                                         const gchar     *color_name,
                                         GdkRGBA         *color);

GDK_DEPRECATED_IN_3_6
void  ctk_style_context_notify_state_change (GtkStyleContext *context,
                                             GdkWindow       *window,
                                             gpointer         region_id,
                                             GtkStateType     state,
                                             gboolean         state_value);
GDK_DEPRECATED_IN_3_6
void  ctk_style_context_cancel_animations   (GtkStyleContext *context,
                                             gpointer         region_id);
GDK_DEPRECATED_IN_3_6
void  ctk_style_context_scroll_animations   (GtkStyleContext *context,
                                             GdkWindow       *window,
                                             gint             dx,
                                             gint             dy);

GDK_DEPRECATED_IN_3_6
void ctk_style_context_push_animatable_region (GtkStyleContext *context,
                                               gpointer         region_id);
GDK_DEPRECATED_IN_3_6
void ctk_style_context_pop_animatable_region  (GtkStyleContext *context);

/* Some helper functions to retrieve most common properties */
GDK_AVAILABLE_IN_ALL
void ctk_style_context_get_color            (GtkStyleContext *context,
                                             GtkStateFlags    state,
                                             GdkRGBA         *color);
GDK_DEPRECATED_IN_3_16_FOR(ctk_render_background)
void ctk_style_context_get_background_color (GtkStyleContext *context,
                                             GtkStateFlags    state,
                                             GdkRGBA         *color);
GDK_DEPRECATED_IN_3_16_FOR(ctk_render_frame)
void ctk_style_context_get_border_color     (GtkStyleContext *context,
                                             GtkStateFlags    state,
                                             GdkRGBA         *color);

GDK_DEPRECATED_IN_3_8_FOR(ctk_style_context_get)
const PangoFontDescription *
     ctk_style_context_get_font             (GtkStyleContext *context,
                                             GtkStateFlags    state);
GDK_AVAILABLE_IN_ALL
void ctk_style_context_get_border           (GtkStyleContext *context,
                                             GtkStateFlags    state,
                                             GtkBorder       *border);
GDK_AVAILABLE_IN_ALL
void ctk_style_context_get_padding          (GtkStyleContext *context,
                                             GtkStateFlags    state,
                                             GtkBorder       *padding);
GDK_AVAILABLE_IN_ALL
void ctk_style_context_get_margin           (GtkStyleContext *context,
                                             GtkStateFlags    state,
                                             GtkBorder       *margin);

GDK_DEPRECATED_IN_3_12
void ctk_style_context_invalidate           (GtkStyleContext *context);
GDK_AVAILABLE_IN_ALL
void ctk_style_context_reset_widgets        (GdkScreen       *screen);

GDK_DEPRECATED_IN_3_18_FOR(ctk_render_background)
void ctk_style_context_set_background       (GtkStyleContext *context,
                                             GdkWindow       *window);

GDK_AVAILABLE_IN_3_4
void        ctk_render_insertion_cursor
                                   (GtkStyleContext     *context,
                                    cairo_t             *cr,
                                    gdouble              x,
                                    gdouble              y,
                                    PangoLayout         *layout,
                                    int                  index,
                                    PangoDirection       direction);
GDK_DEPRECATED_IN_3_4
void   ctk_draw_insertion_cursor    (GtkWidget          *widget,
                                     cairo_t            *cr,
                                     const GdkRectangle *location,
                                     gboolean            is_primary,
                                     GtkTextDirection    direction,
                                     gboolean            draw_arrow);

typedef enum {
  CTK_STYLE_CONTEXT_PRINT_NONE         = 0,
  CTK_STYLE_CONTEXT_PRINT_RECURSE      = 1 << 0,
  CTK_STYLE_CONTEXT_PRINT_SHOW_STYLE   = 1 << 1
} GtkStyleContextPrintFlags;

GDK_AVAILABLE_IN_3_20
char * ctk_style_context_to_string (GtkStyleContext           *context,
                                    GtkStyleContextPrintFlags  flags);

G_END_DECLS

#endif /* __CTK_STYLE_CONTEXT_H__ */
